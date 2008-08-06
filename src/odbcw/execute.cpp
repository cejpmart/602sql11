/*
** EXECUTE.C - This is the ODBC driver code for executing SQL Commands.
*/

#include "wbdrv.h"
#pragma hdrstop
#include "opstr.h"

SWORD default_C_type(SWORD sqltype)
{ switch (sqltype)
  { case SQL_CHAR:            case SQL_VARCHAR:    case SQL_LONGVARCHAR:
    case SQL_DECIMAL:         case SQL_NUMERIC:
      return SQL_C_CHAR;
    case SQL_WCHAR:           case SQL_WVARCHAR:   
      return /*SQL_C_WCHAR*/SQL_C_CHAR;  // MS Access reading data through ODBC has problems with this default (reading table with primary key of NCHAR type)
    case SQL_WLONGVARCHAR:
      return SQL_C_WCHAR;
    case SQL_BIT:      return SQL_C_BIT;
    case SQL_TINYINT:  return SQL_C_STINYINT;
    case SQL_SMALLINT: return SQL_C_SSHORT;
    case SQL_INTEGER:  return SQL_C_SLONG;
    case SQL_BIGINT:   return SQL_C_SBIGINT;
    case SQL_REAL:     return SQL_C_FLOAT;
    case SQL_FLOAT:           case SQL_DOUBLE:
      return SQL_C_DOUBLE;
    case SQL_BINARY:          case SQL_VARBINARY:  case SQL_LONGVARBINARY:
      return SQL_C_BINARY;
    case SQL_TYPE_DATE:       case SQL_DATE:
      return SQL_C_TYPE_DATE;
    case SQL_TYPE_TIME:       case SQL_TIME:
      return SQL_C_TYPE_TIME;
    case SQL_TYPE_TIMESTAMP:  case SQL_TIMESTAMP:
      return SQL_C_TYPE_TIMESTAMP;
  }
  return sqltype;
}

SDWORD C_type_size(SWORD ctype)
{ switch (ctype)
  { case SQL_C_BIT:    return 1;
    case SQL_C_CHAR:  case SQL_C_WCHAR:    
      return SQL_NTS;  /* should not be called but may be */
    case SQL_C_DOUBLE: return 8;
    case SQL_C_FLOAT:  return 4;
    case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
      return 4;
    case SQL_C_SHORT:   case SQL_C_SSHORT:   case SQL_C_USHORT:
      return 2;
    case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
      return 1;
    case SQL_C_SBIGINT: case SQL_C_UBIGINT:
      return 8;
    case SQL_C_TYPE_DATE:       case SQL_C_DATE:   
      return sizeof(DATE_STRUCT);
    case SQL_C_TYPE_TIME:       case SQL_C_TIME:   
      return sizeof(TIME_STRUCT);
    case SQL_C_TYPE_TIMESTAMP:  case SQL_C_TIMESTAMP:
      return sizeof(TIMESTAMP_STRUCT);
  }
  return 0;
}

static void _fastcall __near set_SQL_null(tptr adr, SWORD SqlType)
{ switch (SqlType)
  { case SQL_CHAR:   case SQL_VARCHAR:   case SQL_LONGVARCHAR:
      *adr=0;                                              break;
    case SQL_WCHAR:  case SQL_WVARCHAR:  case SQL_WLONGVARCHAR:
      *(uns16*)adr=0;                                      break;
    case SQL_REAL:   case SQL_FLOAT:     case SQL_DOUBLE:
      *(double*)adr=NONEREAL;                              break;
    case SQL_NUMERIC:  case SQL_DECIMAL:
      *(uns16*)adr=0;  *(sig32*)(adr+2)=NONEINTEGER;       break;
    case SQL_BIT: *adr=(uns8)0x80;                         break;
    case SQL_SMALLINT: *(uns16*)adr=0x8000;                break;
    case SQL_INTEGER:
    case SQL_TYPE_DATE:  case SQL_TYPE_TIMESTAMP:  case SQL_TYPE_TIME:
    case SQL_DATE:       case SQL_TIMESTAMP:       case SQL_TIME:
      *(sig32*)adr=NONEINTEGER;                            break;
    case SQL_TINYINT: *(uns8*)adr=0x80;                    break;
    case SQL_BIGINT:  *(sig64*)adr=NONEBIGINT;             break;
//    case ATT_AUTOR: *(uns16*)adr=0xffff;                   break;
//    case ATT_PTR:
//    case ATT_BIPTR: *(trecnum*)adr=NORECNUM;               break;
  }
}

#ifdef STOP
#include <iconv.h>

int ODBCMultiByteToWideChar(const char *charset, const char *str, size_t len, wuchar*buffer)
// Returns 0 in case of failure, number of written wide chars otherwise
{
  char *from=(char *)str;
  wuchar *to=buffer;
  size_t fromlen=len;
  size_t tolen=2*fromlen;
  iconv_t conv=iconv_open("UCS-2", charset);
  if (conv==(iconv_t)-1) goto err;
  while (fromlen>0){
    int nconv=iconv(conv, &from, &fromlen, (char **)&to, &tolen);
    if (nconv==(size_t)-1){
      iconv_close(conv);
      goto err;
    }
  }
  *to='\0';  // adding a wide terminator - not counted
  iconv_close(conv);
  return to-buffer;
 err:
  if (len>=4) memcpy(buffer, "!\000c\000n\000v\000\000\000", 10);
  else *buffer=0;  // wide
  return 0;

}
#endif

char auxbuf2[MAX_FIXED_STRING_LENGTH+2];

BOOL convert_C_to_SQL(SWORD CType, SWORD SqlType, t_specif sql_specif, tptr CSource, tptr SqlDest,
                      SQLLEN CSize, SQLULEN SqlSize, STMT * stmt/*, bool c_type_converted*/)
// Partially converted to superconv.
// CSize, SqlSize is in bytes!
{ unsigned i, j;  unsigned char d1, d2;  SQLULEN minsize;  t_specif c_string_specif;  sig64 sval64;  int wbtype;
  if (CType==SQL_C_DEFAULT) CType=default_C_type(SqlType);
  //if (c_type_converted)  // C type converted by DM
  //  if (CType==SQL_C_WCHAR) CType==SQL_C_CHAR;
  if (CSize==SQL_NULL_DATA)
    { set_SQL_null(SqlDest, SqlType);  return FALSE; }
  if (CSize==SQL_NTS) CSize = CType==SQL_C_WCHAR ? (int)wuclen((const wuchar*)CSource) : (int)strlen(CSource);
  switch (CType)
  { case SQL_C_BINARY:   /* direct binary transfer, no conversion: */
      if (CSize > SqlSize)
        if (SqlType==SQL_BINARY|| SqlType==SQL_VARBINARY|| SqlType==SQL_LONGVARBINARY||
            SqlType==SQL_CHAR  || SqlType==SQL_VARCHAR  || SqlType==SQL_LONGVARCHAR  )
          raise_err(stmt, SQL_22001);
        else
          { raise_err(stmt, SQL_22003);  return TRUE; }
      memcpy(SqlDest, CSource, CSize < SqlSize ? CSize : SqlSize);
      break;
    case SQL_C_WCHAR:     /* converting from the char string: */
      switch (SqlType)
      { case SQL_WCHAR:  case SQL_WVARCHAR:  case SQL_WLONGVARCHAR:  // just copying
          if (CSize > SqlSize)
          { raise_err(stmt, SQL_22001);
            minsize=SqlSize;
          }
          else minsize = CSize;
          memcpy(SqlDest, CSource, minsize);
          if (minsize<SqlSize) SqlDest[minsize]=0;
          if (minsize+1<SqlSize) SqlDest[minsize+1]=0; 
          return FALSE;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:  
        { t_specif c_specif;
          c_specif = sql_specif;  c_specif.wide_char=1;  
          if (SqlType==SQL_LONGVARCHAR) 
            if (CSize<0x20000) sql_specif.length=(uns16)(CSize/2);
            else sql_specif.length=0;  // this may convert too many characters because the source string may not be terminated
          else
            if (CSize/2<sql_specif.length) sql_specif.length=(uns16)(CSize/2);
          int retcode = superconv(ATT_STRING, ATT_STRING, CSource, SqlDest, c_specif, sql_specif, NULL);
          if (retcode==SC_STRING_TRIMMED)
          { raise_err(stmt, SQL_01004);  /* data will be truncated */
            return FALSE;
          }
          if (retcode<0)
            { raise_err(stmt, SQL_22018);  return TRUE; }
          return FALSE;
        } 

        default:
          raise_err(stmt, SQL_22003);  
          return TRUE;
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_TINYINT:   
        case SQL_BIGINT:
          break;  // processing is common with SQL_C_CHAR
      }
      // cont. for specified types
    case SQL_C_CHAR:     /* converting from the char string: */
      if (CType==SQL_C_WCHAR)  // wide strig
        c_string_specif = t_specif(100, 0, 1, sizeof(SQLWCHAR)==2 ? 1 : 2);
      else // 8-bit
        c_string_specif = t_specif(100, 0, 1, 0);
      if (CSize < MAX_FIXED_STRING_LENGTH)  /* make the string 0-terminated */
        { memcpy(auxbuf2, CSource, CSize);  auxbuf2[CSize]=auxbuf2[CSize+1]=0;  CSource=auxbuf2; }
      switch (SqlType)
      { case SQL_BINARY:  case SQL_VARBINARY:  case SQL_LONGVARBINARY:
          for (i=j=0;  i+1<CSize;  i+=2, j++)
          { if (j>=SqlSize)
              { raise_err(stmt, SQL_22001);  break; }
            d1=CSource[i];  d2=CSource[i+1];
            if ((d1<'0') || (d1>'9') && (d1<'A') ||
                (d1>'Z') && (d1<'a') || (d1>'z'))
              { raise_err(stmt, SQL_22018);  return TRUE; }
            if ((d2<'0') || (d2>'9') && (d2<'A') ||
                (d2>'Z') && (d2<'a') || (d2>'z'))
              { raise_err(stmt, SQL_22018);  return TRUE; }
            if (d1<='9') d1-='0'; else if (d1<='Z') d1-='A'-10;
                                  else d1-='a'-10;
            if (d2<='9') d2-='0'; else if (d2<='Z') d2-='A'-10;
                                  else d2-='a'-10;
            SqlDest[j]=(uns8)(16*d1+d2);
          }
          break;
        case SQL_BIT:
          d1=*CSource;
          if ((d1=='A') || (d1=='Y') || (d1=='a') || (d1=='y') || (d1=='1'))
            *SqlDest='\x1';
          else if ((d1=='n') || (d1=='N') || (d1=='0'))
            *SqlDest='\x0';
          else *SqlDest='\x80';
          break;
#if 0
        case SQL_SMALLINT:
        { sig32 ival;
          if (!str2int(CSource, &ival))   /* not a numeric value */
            { raise_err(stmt, SQL_22018);  return TRUE; }
          if (ival==NONEINTEGER) *(uns16*)SqlDest=0x8000;
          else if ((ival<=-0x8000L) || (ival>=0x8000L))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig16*)SqlDest=(sig16)ival;
          break;
        }
        case SQL_INTEGER:
          if (!str2int(CSource, (sig32*)SqlDest))  /* not a numeric value */
            { raise_err(stmt, SQL_22018);  return TRUE; }
          break;
        case SQL_TINYINT:   
        { sig32 ival;
          if (!str2int(CSource, &ival))   /* not a numeric value */
            { raise_err(stmt, SQL_22018);  return TRUE; }
          if (ival==NONEINTEGER) *(uns8*)SqlDest=0x80;
          else if (ival<-127 || ival>127)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig8*)SqlDest=(sig8)ival;
          break;
        }
        case SQL_BIGINT:
          if (!str2int64(CSource, (sig64*)SqlDest))  /* not a numeric value */
            { raise_err(stmt, SQL_22018);  return TRUE; }
          break;
#else
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_TINYINT:   
        case SQL_BIGINT:
        { int convres = superconv(ATT_STRING, type_sql_to_WB(SqlType), CSource, SqlDest, c_string_specif, sql_specif, "");
          if (convres==SC_INCONVERTIBLE_INPUT_STRING)
            { raise_err(stmt, SQL_22018);  return TRUE; }
          if (convres==SC_VALUE_OUT_OF_RANGE)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          break;
        }
#endif
        case SQL_REAL:  case SQL_FLOAT:  case SQL_DOUBLE:
          if (!str2real(CSource, (double*)SqlDest))  /* not a numeric value */
            { raise_err(stmt, SQL_22018);  return TRUE; }
          break;
        case SQL_DECIMAL:  case SQL_NUMERIC:
          if (!str2money(CSource, (monstr*)SqlDest))  /* not a numeric value */
            { raise_err(stmt, SQL_22018);  return TRUE; }
          break;
        case SQL_TYPE_DATE:  case SQL_DATE:
          if (!odbc_str2date(CSource, (uns32*)SqlDest))
            { raise_err(stmt, SQL_22008);  return TRUE; }
          break;
        case SQL_TYPE_TIME:  case SQL_TIME:
        { uns32 tm;
          if (!str2time(CSource, &tm))
            { raise_err(stmt, SQL_22008);  return TRUE; }
          //if (tm!=NONETIME) tm = tm / 1000 * 1000;  // remove fractional part  // change: supporting fractional part
          *(uns32*)SqlDest = tm; 
          break;
        }
        case SQL_TYPE_TIMESTAMP:  case SQL_TIMESTAMP:
          if (!odbc_str2datim(CSource, (uns32*)SqlDest))
            { raise_err(stmt, SQL_22008);  return TRUE; }
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          minsize = CSize;
          if (CSize > SqlSize)
          { raise_err(stmt, SQL_22001);
            minsize=SqlSize;
          }
          memcpy(SqlDest, CSource, minsize);
          if (minsize<SqlSize) SqlDest[minsize]=0;
          break;
        case SQL_WCHAR:  case SQL_WVARCHAR:  case SQL_WLONGVARCHAR:  // DM converts SQL_C_WCHAR to SQL_C_CHAR, must convert back
        { 
#ifdef STOP
#ifdef WINS		
		  int cnt = MultiByteToWideChar(wbcharset_t(envi_charset).cp(), 0 /*MB_PRECOMPOSED gives ERROR_INVALID_FLAGS for UTF-8 on XP*/, CSource, CSize, (wuchar*)SqlDest, SqlSize/2);
#else
		  int cnt = ODBCMultiByteToWideChar(wbcharset_t(envi_charset), CSource, CSize, (wuchar*)SqlDest);  // C charset not specified, using ISO 8859-1 (Latin 1)
#endif		
          if (2*cnt<SqlSize) SqlDest[2*cnt]=0;
          if (2*cnt+1<SqlSize) SqlDest[2*cnt+1]=0;
#endif
          conv1to2(CSource, (int)CSize, (wuchar*)SqlDest, wbcharset_t(envi_charset), (int)SqlSize-2);
          break;
        }
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    case SQL_C_BIT:
    { unsigned char b = *CSource;
      switch (SqlType)
      { case SQL_BIT:
          if (b && (b!=1))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *SqlDest=b;
          break;
        case SQL_SMALLINT:
          *(sig16*)SqlDest=(sig16)b;
          break;
        case SQL_INTEGER:
          *(sig32*)SqlDest=(sig32)b;
          break;
        case SQL_TINYINT:
          *(sig8 *)SqlDest=(sig8)b;
          break;
        case SQL_BIGINT:
          *(sig64*)SqlDest=(sig64)b;
          break;
        case SQL_REAL:  case SQL_FLOAT:  case SQL_DOUBLE:
          *(double*)SqlDest=(double)b;
          break;
        case SQL_DECIMAL:  case SQL_NUMERIC:
          int2money(b, (monstr*)SqlDest);
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          *SqlDest=b;
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
#if 0
    case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
    { signed char b = *CSource;
      switch (SqlType)
      { case SQL_BIT:
          if (b && (b!=1))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *SqlDest=b;
          break;
        case SQL_SMALLINT:
          *(sig16*)SqlDest=(sig16)b;
          break;
        case SQL_INTEGER:
          *(sig32*)SqlDest=(sig32)b;
          break;
        case SQL_TINYINT:
          *(sig8 *)SqlDest=b;
          break;
        case SQL_BIGINT:
          *(sig64*)SqlDest=(sig64)b;
          break;
        case SQL_REAL:  case SQL_FLOAT:  case SQL_DOUBLE:
          *(double*)SqlDest=(double)b;
          break;
        case SQL_DECIMAL:  case SQL_NUMERIC:
          int2money(b, (monstr*)SqlDest);
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          if (SqlSize < 4)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          int2str(b, SqlDest);
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
    case SQL_C_SHORT:    case SQL_C_SSHORT:  case SQL_C_USHORT:
    { sig16 b = *(sig16*)CSource;
      switch (SqlType)
      { case SQL_BIT:
          if (b && (b!=1))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *SqlDest=(char)b;
          break;
        case SQL_SMALLINT:
          *(sig16*)SqlDest=b;
          break;
        case SQL_INTEGER:
          *(sig32*)SqlDest=(sig32)b;
          break;
        case SQL_TINYINT:
          if ((b<-127) || (b>127))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig8 *)SqlDest=(sig8 )b;
          break;
        case SQL_BIGINT:
          *(sig64*)SqlDest=(sig64)b;
          break;
        case SQL_REAL:  case SQL_FLOAT:  case SQL_DOUBLE:
          *(double*)SqlDest=(double)b;
          break;
        case SQL_DECIMAL:  case SQL_NUMERIC:
          int2money(b, (monstr*)SqlDest);
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          if (SqlSize < 6)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          int2str(b, SqlDest);
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
    case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
    { sig32 b = *(sig32*)CSource;
      switch (SqlType)
      { case SQL_BIT:
          if (b && (b!=1))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *SqlDest=(char)b;
          break;
        case SQL_SMALLINT:
          if ((b<=-0x8000L) || (b>0x8000L))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig16*)SqlDest=(sig16)b;
          break;
        case SQL_INTEGER:
          *(sig32*)SqlDest=b;
          break;
        case SQL_TINYINT:
          if ((b<-127) || (b>127))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig8 *)SqlDest=(sig8 )b;
          break;
        case SQL_BIGINT:
          *(sig64*)SqlDest=(sig64)b;
          break;
        case SQL_REAL:  case SQL_FLOAT:  case SQL_DOUBLE:
          *(double*)SqlDest=(double)b;
          break;
        case SQL_DECIMAL:  case SQL_NUMERIC:
          int2money(b, (monstr*)SqlDest);
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          if (SqlSize < 11)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          int2str(b, SqlDest);
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
    case SQL_C_FLOAT:  case SQL_C_DOUBLE:
    { double b;
      if (CType==SQL_C_FLOAT) b = (double)*(float*)CSource;
      else b=*(double*)CSource;
      switch (SqlType)
      { case SQL_BIT:
          if (b && (b!=1.0))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *SqlDest=(char)(!b ? 0 : 1);
          break;
        case SQL_SMALLINT:
          if ((b<=-0x8000L) || (b>0x8000L))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig16*)SqlDest=(sig16)b;
          break;
        case SQL_INTEGER:
          if ((b<-0x7fffffffL) || (b>0x7fffffffL))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig32*)SqlDest=(sig32)b;
          break;
        case SQL_TINYINT:
          if ((b<-127) || (b>127))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig8 *)SqlDest=(sig8 )b;
          break;
        case SQL_BIGINT:
          if ((b<-MAXBIGINT) || (b>MAXBIGINT))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig64*)SqlDest=(sig64)b;
          break;
        case SQL_REAL:  case SQL_FLOAT:  case SQL_DOUBLE:
          *(double*)SqlDest=b;
          break;
        case SQL_DECIMAL:  case SQL_NUMERIC:
          if (!real2money(b, (monstr*)SqlDest))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          if (SqlSize < 20)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          real2str(b, SqlDest, 14);
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
    case SQL_C_SBIGINT:  case SQL_C_UBIGINT:
    { sig64 b;
      b=*(sig64*)CSource;
      switch (SqlType)
      { case SQL_BIT:
          if (b && (b!=1))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *SqlDest=(char)(!b ? 0 : 1);
          break;
        case SQL_SMALLINT:
          if ((b<=-0x8000L) || (b>0x8000L))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig16*)SqlDest=(sig16)b;
          break;
        case SQL_INTEGER:
          if ((b<-0x7fffffffL) || (b>0x7fffffffL))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig32*)SqlDest=(sig32)b;
          break;
        case SQL_TINYINT:
          if ((b<-127) || (b>127))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          *(sig8 *)SqlDest=(sig8 )b;
          break;
        case SQL_BIGINT:
          *(sig64*)SqlDest=b;
          break;
        case SQL_REAL:  case SQL_FLOAT:  case SQL_DOUBLE:
          *(double*)SqlDest=(double)b;
          break;
        case SQL_DECIMAL:  case SQL_NUMERIC:
          if (!real2money((double)b, (monstr*)SqlDest))
            { raise_err(stmt, SQL_22003);  return TRUE; }
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          if (SqlSize < 20)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          int64tostr(b, SqlDest);
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
#else
    case SQL_C_TINYINT: case SQL_C_STINYINT: 
      wbtype=ATT_INT8;  goto integral;
    case SQL_C_UTINYINT:  // NULL value cannot be represented
      sval64=(sig64)*(uns8 *)CSource;  CSource=(char*)&sval64;  wbtype=ATT_INT64;  goto integral;
    case SQL_C_SHORT:    case SQL_C_SSHORT:  
      wbtype=ATT_INT16;  goto integral;
    case SQL_C_USHORT:    // NULL value cannot be represented
      sval64=(sig64)*(uns16*)CSource;  CSource=(char*)&sval64;  wbtype=ATT_INT64;  goto integral;
    case SQL_C_LONG:    case SQL_C_SLONG:    
      wbtype=ATT_INT32;  goto integral;
    case SQL_C_ULONG:     // NULL value cannot be represented
      sval64=(sig64)*(uns32*)CSource;  CSource=(char*)&sval64;  wbtype=ATT_INT64;  goto integral;
    case SQL_C_SBIGINT:   
      wbtype=ATT_INT64;  goto integral;
    case SQL_C_UBIGINT:   // NULL value cannot be represented
      if (*(uns64*)CSource > MAXBIGINT)
        { raise_err(stmt, SQL_22003);  return TRUE; }
      wbtype=ATT_INT64;  goto integral;
    case SQL_C_FLOAT:     // NULL value cannot be represented 
      *(double*)&sval64 = (double)*(float*)CSource;  CSource=(char*)&sval64;  wbtype=ATT_FLOAT;  goto integral;
    case SQL_C_DOUBLE:
      wbtype=ATT_FLOAT;
     integral:
      { int convres;
        if (SqlType==SQL_CHAR || SqlType==SQL_VARCHAR || SqlType==SQL_LONGVARCHAR)
        { char strbuf[100];
          convres = superconv(wbtype, ATT_STRING, CSource, strbuf, t_specif(100, 0, 1, 0), sql_specif, "");
          if (convres>=0)
            if (strlen(strbuf)<SqlSize) strcpy(SqlDest, strbuf);
            else convres=SC_VALUE_OUT_OF_RANGE;  // not enough space for the output string
        }
        else if (SqlType==SQL_WCHAR || SqlType==SQL_WVARCHAR || SqlType==SQL_WLONGVARCHAR)
        { wuchar strbuf[100];  // for the SQL side, always is wuchar
          convres = superconv(wbtype, ATT_STRING, CSource, (char*)strbuf, t_specif(100, 0, 1, sizeof(SQLWCHAR)==2 ? 1 : 2), sql_specif, "");
          if (convres>=0)
            if (wuclen(strbuf)*sizeof(wuchar) < SqlSize) wuccpy((wuchar*)SqlDest, strbuf);
            else convres=SC_VALUE_OUT_OF_RANGE;  // not enough space for the output string
        }
        else if (SqlType==SQL_BIT)
        { convres = superconv(wbtype, ATT_INT8, CSource, SqlDest, t_specif(), t_specif(), "");  // converting to INT8 because conversions to Booean may not be supported by superconv
          if (convres>=0)
            if (*SqlDest && *SqlDest!=1 && *(unsigned char*)SqlDest!=128)
              convres=SC_VALUE_OUT_OF_RANGE;
        }
        else
          convres = superconv(wbtype, type_sql_to_WB(SqlType), CSource, SqlDest, t_specif(), sql_specif, "");
        if (convres==SC_BAD_TYPE)
          { raise_err(stmt, SQL_07006);  return TRUE; } /* Restricted data type attribute violation */
        if (convres==SC_VALUE_OUT_OF_RANGE)
          { raise_err(stmt, SQL_22003);  return TRUE; }
      }
      break;
#endif
    case SQL_C_TYPE_DATE:  case SQL_C_DATE:
    { DATE_STRUCT * dt = (DATE_STRUCT*)CSource;  uns32 wbdt;
      wbdt=Make_date(dt->day, dt->month, dt->year);
      if (wbdt==NONEDATE)
        { raise_err(stmt, SQL_22008);  return TRUE; }
      switch (SqlType)
      { case SQL_TYPE_DATE:  case SQL_DATE:
          *(uns32*)SqlDest=wbdt;
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          if (SqlSize<10)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          odbc_date2str(wbdt, SqlDest);
          break;
        case SQL_TYPE_TIMESTAMP:   case SQL_TIMESTAMP:
          wbdt=Make_date(dt->day, dt->month, dt->year<1900 ? 0 : dt->year-1900);
          *(uns32*)SqlDest=wbdt*86400;
          break;
          return TRUE;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
    case SQL_C_TYPE_TIME:    case SQL_C_TIME:
    { TIME_STRUCT * tm = (TIME_STRUCT*)CSource;  uns32 wbtm;
      wbtm=Make_time(tm->hour, tm->minute, tm->second, 0);
      if (wbtm==NONETIME)
        { raise_err(stmt, SQL_22008);  return TRUE; }
      switch (SqlType)
      { case SQL_TYPE_TIME:  case SQL_TIME:
          *(uns32*)SqlDest=wbtm;
          break;
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
          if (SqlSize<8)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          time2str(wbtm, SqlDest, 2);  /* hh:mm:ss - size 8 */ // change: supporting fractional part
          break;
        case SQL_TYPE_TIMESTAMP:    case SQL_TIMESTAMP:
          *(uns32*)SqlDest=wbtm/1000;
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
    case SQL_C_TYPE_TIMESTAMP:  case SQL_C_TIMESTAMP:
    { TIMESTAMP_STRUCT * ts = (TIMESTAMP_STRUCT*)CSource;
      switch (SqlType)
      { case SQL_TYPE_TIME:  case SQL_TIME:
        { uns32 wbtm=Make_time(ts->hour, ts->minute, ts->second, ts->fraction);
          if (wbtm==NONETIME)
            { raise_err(stmt, SQL_22008);  return TRUE; }
          *(uns32*)SqlDest=wbtm;
          break;
        }
        case SQL_TYPE_DATE:  case SQL_DATE:
        { uns32 wbdt=Make_date(ts->day, ts->month, ts->year);
          if (wbdt==NONEDATE)
            { raise_err(stmt, SQL_22008);  return TRUE; }
          *(uns32*)SqlDest=wbdt;
          break;
        }
        case SQL_CHAR:  case SQL_VARCHAR:  case SQL_LONGVARCHAR:
        { uns32 wbts;
          wbts=86400L*((ts->day-1)+31L*((ts->month-1)+12*(ts->year-1900)))+
               ts->hour + 24L*(ts->minute + 60*ts->second);
          if (SqlSize<19)
            { raise_err(stmt, SQL_22003);  return TRUE; }
          odbc_datim2str(wbts, SqlDest);
          break;
        }
        case SQL_TYPE_TIMESTAMP:  case SQL_TIMESTAMP:
          TIMESTAMP2datim(ts, (uns32*)SqlDest);
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return TRUE;
      }
      break;
    }
  }
  return FALSE;
}
//////////////////////////// statement parameters /////////////////////////////////////
static BOOL data_and_size(tptr & data, SQLLEN *& octet_len, SQLLEN *& indicator,
                          STMT * stmt, t_desc_rec * adrec)
// Gets param data pointer and value size.
// data_ptr may be NULL when indicator is SQL_DATA_AT_EXEC or SQL_NULL_DATA.
// Returns TRUE iff parameter is bound.
{ data      = (tptr)adrec->data_ptr;
  octet_len = adrec->octet_length_ptr;
  indicator = adrec->indicator_ptr;
 // offsetting the pointers by bind_offset_ptr:
  if (stmt->apd->bind_offset_ptr!=NULL)
  { unsigned disp = *stmt->apd->bind_offset_ptr;
    if (data     !=NULL) data += disp;
    if (octet_len!=NULL) octet_len=(SQLLEN*)((tptr)octet_len+disp);
    if (indicator!=NULL) indicator=(SQLLEN*)((tptr)indicator+disp);
  }
 // offsetting the pointers by set_pos (loading array):
  if (stmt->exec_iter)
    if (stmt->apd->bind_type!=SQL_BIND_BY_COLUMN)  /* row-wise rowset */
    { unsigned disp = stmt->exec_iter * stmt->apd->bind_type;
      if (octet_len!=NULL) octet_len=(SQLLEN*)((tptr)octet_len+disp);
      if (indicator!=NULL) indicator=(SQLLEN*)((tptr)indicator+disp);
      if (data     !=NULL) data += disp;
    }
    else   /* column-wise rowset */
    { if (octet_len!=NULL) octet_len+=stmt->exec_iter;
      if (indicator!=NULL) indicator+=stmt->exec_iter;
      if (data     !=NULL) data     +=stmt->exec_iter * adrec->value_step;
    }
  return data!=NULL;
}

static BOOL prepare_params(STMT * stmt)
// Retrieves information about the current parameter set and stores it into
//  curr_octet_len, curr_indicator, curr_data, valsize in adrec.
// Determines, if the current parameter is AE, and frees its buffers.
// The count in IPD is not less the the count in the APD
{ BOOL err=FALSE;
  for (int i=1;  i<=stmt->impl_ipd.count;  i++)
  { t_desc_rec * adrec = stmt->apd->drecs.acc(i);  // not NULL, test 07001 above
    data_and_size(adrec->curr_data, adrec->curr_octet_len, adrec->curr_indicator, 
                  stmt, adrec);
    adrec->is_AE = adrec->curr_octet_len!=NULL &&
       (*adrec->curr_octet_len == SQL_DATA_AT_EXEC || 
        *adrec->curr_octet_len <= SQL_LEN_DATA_AT_EXEC_OFFSET);
   /* important for AE params only: */
    safefree((void**)&adrec->buf);
    adrec->valsize=0;
   // determine the size of non-AE parameter value:
    if (!adrec->is_AE) 
      if (adrec->curr_indicator!=NULL &&
          (*adrec->curr_indicator==SQL_NULL_DATA || *adrec->curr_indicator==SQL_DEFAULT_PARAM))
        adrec->valsize=*adrec->curr_indicator;
      else if (adrec->curr_data==NULL) // data must not be NULL in this case
      { raise_err(stmt, SQL_07002);
        err=TRUE;
      }
      else if (adrec->concise_type==SQL_C_CHAR || adrec->concise_type==SQL_C_WCHAR || adrec->concise_type==SQL_C_BINARY)
        if (adrec->curr_octet_len==NULL) // used by MS cursor demo, correct
        { //adrec->valsize=adrec->octet_length;  // sometimes is zero! (BDE)
          //if (!adrec->valsize) // may not be null terminated! (BDE)
          // above lines removed in 8.1 in order to conform to the ODBC specification
          // octet_length is only a buffer size. For curr_octet_len==NULL strings are null terminated (even binary!)
            adrec->valsize = adrec->concise_type==SQL_C_WCHAR ? (int)wuclen((wuchar*)adrec->curr_data) : (int)strlen(adrec->curr_data);
        }
        else
        { adrec->valsize=*adrec->curr_octet_len;
          if (adrec->valsize==SQL_NTS) 
            adrec->valsize = adrec->concise_type==SQL_C_WCHAR ? (int)wuclen((wuchar*)adrec->curr_data) : (int)strlen(adrec->curr_data);
        }
      else adrec->valsize=adrec->value_step;
  }
  return err;
}

void DESC::clear_AE_params(void)
/* determines, if the current parameter is AE, and frees its buffers */
{ for (int i=0;  i<=count;  i++)
  { t_desc_rec * adrec = drecs.acc0(i);
    safefree((void**)&adrec->buf);
    adrec->valsize=0;
  }
}

BOOL drop_params(STMT * stmt)
{ sig16 oper = SEND_PAR_DROP;
  return cd_Send_params(stmt->cdp, stmt->prep_handle, sizeof(sig16), &oper);
}
BOOL send_cursor_name(STMT * stmt)
{ t_op_send_par_name buf;
  buf.id=SEND_PAR_NAME;  buf.cursnum=stmt->cursnum;
  strcpy(buf.name, stmt->cursor_name);
  return cd_Send_params(stmt->cdp, stmt->prep_handle, (DWORD)buf.size(), &buf);
}
BOOL send_cursor_pos(STMT * stmt, trecnum pos)
{ t_op_send_par_pos buf;
  buf.id=SEND_PAR_POS;
  buf.recnum=pos;
  return cd_Send_params(stmt->cdp, stmt->prep_handle, sizeof(buf), &buf);
}

#define LIMIT 256   /* values above this limit are sended separately */

SQLRETURN send_long_sql_data(STMT * stmt, int ctype, int sqltype, t_specif specif,
                             tptr CData, int csize, int offset, int column)
// Converts & sends data, which are var-len on WinBase side
// Allowed only for SQL_C_CHAR & SQL_C_BINARY
// Special memory allocation scheme used. IS_LONG_SQL_TYPE(idrec->concise_type)==TRUE.
// Not called for NULL data or DEFAULT param.
{ BOOL ret = SQL_SUCCESS;  tptr buf;  int bufsize, sqlsize;
  if (ctype==SQL_C_BINARY)  // Direct copy
    sqlsize=bufsize=csize;
  else switch (sqltype)
  { case SQL_LONGVARBINARY:  case SQL_VARBINARY:  // Nospec, Raster
      if (ctype!=SQL_C_CHAR)  // conversion not possible
        { raise_err(stmt, SQL_07006);  return SQL_ERROR; }
      bufsize=sqlsize=csize/2;
      break;
    case SQL_LONGVARCHAR:   case SQL_WLONGVARCHAR:  // Text
      if (csize < 256) bufsize=256;  else bufsize=csize;
      sqlsize=csize;
      if (sqltype==SQL_WLONGVARCHAR && ctype==SQL_C_CHAR)
        { bufsize*=2;  sqlsize*=2; }
      break;
  }
  if (bufsize)
  {// allocate buffer:
    buf=(tptr)corealloc(bufsize, 75);
    if (buf==NULL) { raise_err(stmt, SQL_HY001);  return SQL_ERROR; }
   // convert:
    if (convert_C_to_SQL(ctype, sqltype, specif, CData, buf, csize, bufsize, stmt)) 
      { corefree(buf);  return SQL_ERROR; }
  }
  if (cd_Send_parameter(stmt->cdp, stmt->prep_handle, column-1, sqlsize, offset, buf))
    { raise_db_err(stmt);  ret=SQL_ERROR; }
  if (bufsize) corefree(buf);  
  return ret;
}

static BOOL send_all_params(STMT * stmt)
/* Called after AE params are reetreived. Sends non-AE params and unsended AE
   params, which have non-null buf or valsize==SQL_NULL_DATA. */
/* After sending the parameters, relases the AE-buffers & reinits AE params */
/* On errors, raise called inside, TRUE returned. */
{ tptr buf;  int i;  BOOL res;  
 /* send long non-AE parameters: */
  for (i=1;  i<=stmt->impl_ipd.count;  i++)
  { t_desc_rec * adrec = stmt->    apd->drecs.acc0(i);  // not NULL, test 07001
    t_desc_rec * idrec = stmt->impl_ipd.drecs.acc0(i); 
    if (idrec->parameter_type==SQL_PARAM_OUTPUT) continue;
    if (IS_LONG_SQL_TYPE(idrec->concise_type) && !adrec->is_AE)
      if (adrec->valsize>LIMIT && 
          adrec->valsize!=SQL_NULL_DATA && adrec->valsize!=SQL_DEFAULT_PARAM)
        if (send_long_sql_data(stmt, adrec->concise_type, idrec->concise_type, idrec->specif,
                               adrec->curr_data, (int)adrec->valsize, 0, i)==SQL_ERROR)
            return TRUE;
  }
 // determine the buffer size:
  unsigned total_size=0;   
  for (i=1;  i<=stmt->impl_ipd.count;  i++)
  { t_desc_rec * adrec = stmt->    apd->drecs.acc0(i);  // not NULL, test 07001
    t_desc_rec * idrec = stmt->impl_ipd.drecs.acc0(i); 
    if (idrec->parameter_type==SQL_PARAM_OUTPUT) continue;
    if (adrec->valsize==SQL_NULL_DATA || adrec->valsize==SQL_DEFAULT_PARAM)
      total_size+=6;  /* NULL_DATA or DEFAULT */
    else /* not NULL_DATA nor DEFAULT */
    { SQLULEN space; 
      if (IS_LONG_SQL_TYPE(idrec->concise_type))  /* destination is var-len */
      { if (adrec->is_AE) continue;
        if (adrec->valsize>LIMIT) continue;  // has been sent above
        space=adrec->valsize;
        if (idrec->concise_type==SQL_LONGVARBINARY || idrec->concise_type==SQL_VARBINARY)
        { if (adrec->concise_type==SQL_C_CHAR) space/=2;
        }
#ifdef STOP  // used by 8-bit driver only
        else if (idrec->concise_type==SQL_WLONGVARCHAR || idrec->concise_type==SQL_WVARCHAR || idrec->concise_type==SQL_WCHAR)
        { if (adrec->concise_type==SQL_C_CHAR) space*=2;
        }
#endif
      }
      else if (idrec->concise_type==SQL_CHAR || idrec->concise_type==SQL_WCHAR || 
               idrec->concise_type==SQL_BINARY ||
               idrec->concise_type==SQL_VARCHAR || idrec->concise_type==SQL_WVARCHAR)
        space=idrec->length;  /* transfer length defined by BindParameter */
      else space=WB_type_length(type_sql_to_WB(idrec->concise_type)); /* WinBase data size */
      total_size += 10+(unsigned)space;
    }
  }
  if (!total_size)   /* no further parameters */
  { stmt->apd->clear_AE_params();  /* frees AE buffers */
    return FALSE;
  }
 /* allocate the buffer for parameters: */
  buf=(tptr)corealloc(total_size+sizeof(sig16), 52);
  if (!buf)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return TRUE;
  }

 // convert & store the parameters: 
  tptr pb=buf;   /* buffer pointer */
  for (i=1;  i<=stmt->impl_ipd.count;  i++)
  { t_desc_rec * adrec = stmt->    apd->drecs.acc0(i);  // not NULL, test 07001
    t_desc_rec * idrec = stmt->impl_ipd.drecs.acc0(i); 
    if (idrec->parameter_type==SQL_PARAM_OUTPUT) continue;
    if (adrec->valsize==SQL_NULL_DATA || adrec->valsize==SQL_DEFAULT_PARAM)
    { *(sig16*)pb=(sig16)i-1;             pb+=sizeof(sig16);
      *(uns32*)pb=(uns32)adrec->valsize;  pb+=sizeof(uns32);
    }
    else /* not NULL_DATA nor DEFAULT */
    { SQLULEN space;
      int ctype = adrec->concise_type;
      if (IS_LONG_SQL_TYPE(idrec->concise_type))  /* destination is var-len */
      { if (adrec->is_AE) continue;
        if (adrec->valsize>LIMIT) continue;  // has been sent above
        space=adrec->valsize;
        if (idrec->concise_type==SQL_LONGVARBINARY || idrec->concise_type==SQL_VARBINARY)
        { if (ctype==SQL_C_CHAR) space/=2;
        }
#ifdef STOP  // used by 8-bit driver only
        else if (idrec->concise_type==SQL_WLONGVARCHAR || idrec->concise_type==SQL_WVARCHAR || idrec->concise_type==SQL_WCHAR)
        { if (ctype==SQL_C_CHAR) space*=2;
        }
#endif
      }
      else if (idrec->concise_type==SQL_CHAR || idrec->concise_type==SQL_WCHAR || 
               idrec->concise_type==SQL_BINARY ||
               idrec->concise_type==SQL_VARCHAR || idrec->concise_type==SQL_WVARCHAR)
        space=idrec->length;  /* transfer length defined by BindParameter */
      else space=WB_type_length(type_sql_to_WB(idrec->concise_type)); /* WinBase data size */
      *(sig16*)pb=(sig16)(i-1);  pb+=sizeof(sig16);
      *(uns32*)pb=(uns32)space;  pb+=sizeof(uns32);
      *(uns32*)pb=0;             pb+=sizeof(uns32);
#ifdef STOP  // used by 8-bit driver only
      if (ctype==SQL_C_CHAR)
        if (idrec->concise_type==SQL_WCHAR || idrec->concise_type==SQL_WVARCHAR || idrec->concise_type==SQL_WLONGVARCHAR)
          ctype=SQL_C_WCHAR;  // DM translated the type, but data is not translated
#endif
//      if (IS_LONG_SQL_TYPE(idrec->concise_type))  /* destination is var-len */
//        memcpy(pb, adrec->curr_data, space);  // BAD, must convert long chr->bin
//      else
        if (convert_C_to_SQL(ctype, idrec->concise_type, idrec->specif, adrec->is_AE ? adrec->buf : adrec->curr_data, pb,
               adrec->valsize, (DWORD)space, stmt))
          { corefree(buf);  return TRUE; }
      pb+=space;
    }
  }
  *(sig16*)pb=SEND_PAR_DELIMITER;  pb+=sizeof(sig16);
  res=cd_Send_params(stmt->cdp, stmt->prep_handle, total_size+sizeof(sig16), buf);
  if (res) raise_db_err(stmt);
  corefree(buf);
  stmt->apd->clear_AE_params();  /* frees AE buffers */
  return res;
}

static RETCODE execute_stmt(STMT * stmt)
/* Execute stmt and extend the result set, called when all params posted */
// Pulls the output paameters after execution
{// allocate space for result: 
  if (!stmt->result_sets.acc(stmt->sql_stmt_cnt+MAX_RESULT_COUNT-1))
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  uns32 * sql_results=(uns32*)stmt->result_sets.acc(stmt->sql_stmt_cnt);

  BOOL has_output_params=FALSE;
  for (int i=1;  i<=stmt->impl_ipd.count;  i++)
  { t_desc_rec * idrec = stmt->impl_ipd.drecs.acc0(i); 
    if (idrec->parameter_type==SQL_PARAM_OUTPUT || idrec->parameter_type==SQL_PARAM_INPUT_OUTPUT)
      { has_output_params=TRUE;  break; }
  }
 /* submit statement: */
  int result_count;  BOOL err;  SQLRETURN ret=SQL_SUCCESS;
  if (has_output_params)
  { tptr outpars;
    cd_start_package(stmt->cdp);
    cd_SQL_exec_prepared(stmt->cdp, stmt->prep_handle, sql_results, &result_count);
    cd_Pull_params(stmt->cdp, stmt->prep_handle, SEND_PAR_PULL, (void**)&outpars);
    cd_send_package(stmt->cdp);
    err=cd_Sz_error(stmt->cdp) != 0;
   // write the values of output paramaters:
    if (!err && outpars!=NULL)
    { tptr pb=outpars;
      sig16 parnum;  uns32 length;
      parnum=*(sig16*)pb;  pb+=sizeof(sig16);
      while (parnum!=SEND_PAR_DELIMITER)
      { length=*(uns32*)pb;  pb+=sizeof(uns32);
        parnum++;  // 0-based to 1-based
        if (parnum<=stmt->impl_ipd.count && parnum<=stmt->apd->count)  
        // ignored otherwise, not bound at least
        { t_desc_rec * adrec = stmt->    apd->drecs.acc0(parnum);  // not NULL, test 07001
          t_desc_rec * idrec = stmt->impl_ipd.drecs.acc0(parnum); 
          if (adrec->curr_data!=NULL) 
          { if (length>256)
            { char * buf = (tptr)corealloc(length+1, 91);
              if (buf)
              { int offset = 0;
                do
                { int step= length-offset;
                  if (step>0xe000) step=0xe000;
                  if (cd_Pull_parameter(stmt->cdp, stmt->prep_handle, parnum-1, offset, step, buf+offset))
                    break;
                  offset+=step;
                } while (offset<length);
                buf[length]=0;
                SQLRETURN ret1=convert_SQL_to_C(adrec->concise_type, idrec->concise_type, idrec->specif, buf, adrec->curr_data,
                          adrec->octet_length, adrec->curr_octet_len, length, stmt, (unsigned)-1, FALSE);
                if (ret1==SQL_ERROR || ret1==SQL_SUCCESS_WITH_INFO && ret==SQL_SUCCESS)
                  ret=ret1;
                if (adrec->curr_indicator!=NULL && 
                    adrec->curr_indicator!=adrec->curr_octet_len) 
                  *adrec->curr_indicator = *adrec->curr_octet_len==SQL_NULL_DATA ? SQL_NULL_DATA : 0;
                corefree(buf);
              }
            } // long value
            else
            { SQLRETURN ret1=convert_SQL_to_C(adrec->concise_type, idrec->concise_type, idrec->specif, pb, adrec->curr_data,
                      adrec->octet_length, adrec->curr_octet_len, length, stmt, (unsigned)-1, FALSE);
              if (ret1==SQL_ERROR || ret1==SQL_SUCCESS_WITH_INFO && ret==SQL_SUCCESS)
                ret=ret1;
              if (adrec->curr_indicator!=NULL && 
                  adrec->curr_indicator!=adrec->curr_octet_len) 
                *adrec->curr_indicator = *adrec->curr_octet_len==SQL_NULL_DATA ? SQL_NULL_DATA : 0;
            } // short value
          } // is bound
        } // existing parameter
        if (length<=256) pb+=length;
       // next paramater:
        parnum=*(sig16*)pb;  pb+=sizeof(sig16);
      } // cycle on short parameters
      corefree(outpars);
    } // statement executed OK
  }
  else // statement without paramaters
    err=cd_SQL_exec_prepared(stmt->cdp, stmt->prep_handle, sql_results, &result_count);
 // error or OK:
  stmt->apd->clear_AE_params();
  if (err)
  { raise_db_err(stmt);
    return SQL_ERROR;
  }
  stmt->rs_per_iter  = result_count;  // not used any more
  stmt->sql_stmt_cnt+= result_count;  /* result set extended */
  return ret;
}
//	-	-	-	-	-	-	-	-	-

static BOOL find_next_parameter_set(STMT * stmt)
// Returns TRUE - paramset found & positioned,
//         FALSE - no more paramsets.
// Error in the paramset is stored to array_status_ptr, search continues.
{// cycle on param set elements, looking for a valid set:
  while (++stmt->exec_iter < stmt->apd->array_size)
  {// set the application parametr set number: 
    if (stmt->impl_ipd.rows_procsd_ptr!=NULL) 
       *stmt->impl_ipd.rows_procsd_ptr=stmt->exec_iter+1;
   // check the new parameter set status:
    if (stmt->apd->array_status_ptr!=NULL &&
        stmt->apd->array_status_ptr[stmt->exec_iter]==SQL_PARAM_IGNORE)
    // skipping the parameter set:
    { if (stmt->impl_ipd.array_status_ptr!=NULL)
          stmt->impl_ipd.array_status_ptr[stmt->exec_iter]=SQL_PARAM_UNUSED;
    }
   // prepare the paramset:
    else if (prepare_params(stmt)) 
    { if (stmt->impl_ipd.array_status_ptr!=NULL)
          stmt->impl_ipd.array_status_ptr[stmt->exec_iter]=SQL_PARAM_ERROR;
    }
    else // paramset OK
    { stmt->last_par_ind=0;  // back to the first parameter, search parameter starting with last+1 
      return TRUE;  // valid param set found
    }
  } 
  return FALSE;
}

static SQLRETURN find_next_AE_param_or_exec(STMT * stmt, SQLPOINTER * ValuePtrPtr, BOOL by_exec)
// stmt->exec_iter is the current iteration, 
// stmt->last_par_ind has already been processed.
// by_exec says it has been called by Exec or ExecDirect, called by ParamData otherwise
{ BOOL any_error=FALSE, any_info=FALSE;
  do  // cycle on paramter sets
  { int ind=stmt->last_par_ind;
    while (++ind <= stmt->impl_ipd.count)   // there is another parameter 
    { t_desc_rec * adrec = stmt->apd->drecs.acc0(ind);  // not NULL, test 07001 above
      if (adrec->is_AE)  // AE found!
      { if (by_exec)
        { stmt->last_par_ind=ind-1;  // must re-find this param in ParamData
          stmt->AE_mode=AE_PARAMS;
        }
        else
        { stmt->last_par_ind=ind;    // lastpar_ind will be used by PutData, then increased by ParamData
          if (ValuePtrPtr!=NULL) *ValuePtrPtr=adrec->data_ptr;   /* value identifying the parameter */
        }
        return SQL_NEED_DATA;
      }
    }
   /* all AE parameter values have already been provided, send param. values */
    if (send_all_params(stmt))
    { stmt->AE_mode=AE_NO;
      return SQL_ERROR;
    }
   // execute the statement: 
    RETCODE res=execute_stmt(stmt);
    if (res==SQL_ERROR) any_error=TRUE;
    else if (res==SQL_SUCCESS_WITH_INFO) any_info=TRUE;
   // write the execution result to the status array:
    if (stmt->impl_ipd.array_status_ptr!=NULL)
        stmt->impl_ipd.array_status_ptr[stmt->exec_iter]=
          res==SQL_SUCCESS           ? SQL_PARAM_SUCCESS :
          res==SQL_SUCCESS_WITH_INFO ? SQL_PARAM_SUCCESS_WITH_INFO : SQL_PARAM_ERROR;
   // go to the next valid iteration, if multiple parameter values provided: 
  } while (find_next_parameter_set(stmt));

  stmt->AE_mode=AE_NO;
  stmt->sql_stmt_ind=0;   /* open the 1st result set */
  if (any_error) return SQL_ERROR;
 // update the result set info / based on execution
  if (stmt->sql_stmt_cnt>0)  // unless empty statement
  { uns32 result_info=*(uns32*)stmt->result_sets.acc0(0);
    if (IS_CURSOR_NUM(result_info) && !stmt->impl_ird.count) // result set exists and not described in SQLPrepare  
    { tcursnum curs = (tcursnum)result_info;
      const d_table * td = cd_get_table_d(stmt->cdp, curs, CATEG_DIRCUR);
      define_result_set_from_td(stmt, td);
      if (td!=NULL) release_table_d(td);
    }
  }
 // open the 1st result set:
  if (!open_result_set(stmt)) return SQL_ERROR;
  return any_info ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;
}

static RETCODE cyc_exec(STMT * stmt)
// Starts the process of retreiving AE parameters and array execution.
// Breaked on the 1st AE parameter, continues in SQLParamData
{// actions common for all parameter sets: 
  if (stmt->apd->count < stmt->impl_ipd.count)
  { raise_err(stmt, SQL_07002);  /* COUNT field incorrect */
    return SQL_ERROR;
  }
 // define the C type and the value_step for column-wise binding:
  for (int i=1;  i<=stmt->impl_ipd.count;  i++)
  { t_desc_rec * adrec = stmt->    apd->drecs.acc0(i);  // not NULL, test 07001
    t_desc_rec * idrec = stmt->impl_ipd.drecs.acc0(i); 
    if (adrec->concise_type==SQL_C_DEFAULT)  // replace, used later
      adrec->concise_type=adrec->type=default_C_type(idrec->concise_type);
    if (adrec->concise_type==SQL_C_CHAR || adrec->concise_type==SQL_C_WCHAR || adrec->concise_type==SQL_C_BINARY)
      adrec->value_step = adrec->octet_length;  // includes terminator space
    else 
      adrec->value_step = C_type_size(adrec->concise_type);
  }                           
 // prepare the 1st valid parameter set:
  stmt->exec_iter=(UDWORD)-1;  
  if (!find_next_parameter_set(stmt)) SQL_SUCCESS; // no valid param sets found
 // start scanning for AE params:
  stmt->AE_mode=AE_PARAMS;
  return find_next_AE_param_or_exec(stmt, NULL, TRUE);
}

//	Execute a prepared SQL statement

RETCODE SQL_API SQLExecute(SQLHSTMT StatementHandle)		// statement to execute.
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (!stmt->is_prepared)  // checked by DM, too
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (stmt->has_result_set)  /* must close the result set before */
  { raise_err(stmt, SQL_24000);  /* Invalid cursor state */
    return SQL_ERROR;
  }
 /* discard all unopened result sets: */
  stmt->async_run=TRUE;
  if (stmt->sql_stmt_cnt > 1) discard_other_result_sets(stmt);
  stmt->sql_stmt_cnt=0;
  RETCODE res=cyc_exec(stmt);
  stmt->async_run=FALSE;
  return res;
}

//	Performs the equivalent of SQLPrepare, followed by SQLExecute.

RETCODE SQL_API SQLExecDirectW(SQLHSTMT StatementHandle, SQLWCHAR *	StatementText, SQLINTEGER TextLength)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  RETCODE res = SQLPrepareW(StatementHandle, StatementText, TextLength);
  if (res != SQL_SUCCESS && res != SQL_SUCCESS_WITH_INFO)
    return res; 
 /* discard all unopened result sets: */
  stmt->async_run=TRUE;
  if (stmt->sql_stmt_cnt > 1) discard_other_result_sets(stmt);
  stmt->sql_stmt_cnt=0;
  res=cyc_exec(stmt);
  stmt->async_run=FALSE;
  return res;
}

//	-	-	-	-	-	-	-	-	-

//	Returns the SQL string as modified by the driver.

SQLRETURN SQL_API SQLNativeSqlW(SQLHDBC ConnectionHandle, SQLWCHAR * InStatementText,
	SQLINTEGER TextLength1, SQLWCHAR *	OutStatementText, SQLINTEGER BufferLength,
	SQLINTEGER * TextLength2Ptr)
// DM checks InStatementText!=NULL and lengths
{ if (!IsValidHDBC(ConnectionHandle)) return SQL_INVALID_HANDLE;
  DBC * dbc=(DBC*)ConnectionHandle;
  CLEAR_ERR(dbc);
  if (!dbc->connected)
  { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
    return SQL_ERROR;
  }
  if (TextLength1==SQL_NTS) TextLength1 = sizeof(SQLWCHAR) * odbclen(InStatementText);
 // no conversion, copy the input string: */
  if (TextLength2Ptr != NULL) *TextLength2Ptr=TextLength1;
  if (OutStatementText != NULL && BufferLength > 0)
  { int cp = TextLength1<BufferLength-sizeof(SQLWCHAR) ? TextLength1 : BufferLength-sizeof(SQLWCHAR);
    memcpy(OutStatementText, InStatementText, cp);
    OutStatementText[cp/sizeof(SQLWCHAR)]=0;
    if (TextLength1 >= BufferLength)
    { raise_err_dbc(dbc, SQL_01004);  /* Data truncated */
      return SQL_SUCCESS_WITH_INFO;
    }
  }
	return SQL_SUCCESS;
}

//	-	-	-	-	-	-	-	-	-

//	Supplies parameter data at execution time.	Used in conjuction with
//	SQLPutData.

SQLRETURN SQL_API SQLParamData(SQLHSTMT	StatementHandle, SQLPOINTER *	ValuePtrPtr)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (!stmt->AE_mode)   /* not in the AE_mode! */
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  /* The S1DE0 error is never raised, it is replaced by S1010. */

  if (stmt->AE_mode == AE_PARAMS)  // searching the next AE parameter
    return find_next_AE_param_or_exec(stmt, ValuePtrPtr, FALSE);
  else
  { tptr data;  
    if (stmt->AE_mode==AE_COLUMNS)
    { stmt->curr_column++;
      RETCODE retcode=SetPos_steps(stmt, FALSE);
      if (retcode!=SQL_NEED_DATA)
        { stmt->AE_mode=AE_NO;  return retcode; }
    }
    else /* AE_COLUMNS_START */
      stmt->AE_mode=AE_COLUMNS;
   /* find and write the token: */
    if (ValuePtrPtr != NULL)
    { if (stmt->ard->count >= stmt->curr_column+1)   /* must not call acc() otherwise */
      { t_desc_rec * adrec = stmt->ard->drecs.acc(stmt->curr_column+1);
        data=(tptr)adrec->data_ptr;
      }
      else data=NULL;
      *ValuePtrPtr=(SQLPOINTER)data;
    }
    stmt->curr_col_off=0;
    return SQL_NEED_DATA;
  }
}

//	-	-	-	-	-	-	-	-	-

//	Supplies parameter data at execution time.	Used in conjunction with
//	SQLParamData.
/* Dle typu AE parametr je:
- hodnoty promenne delky: cbValue je dosavadni offset, rgbValue==NULL,
  kusy hodnoty se prubezne posilaji do serveru;
- string: cbValue je dosavadni offset, rgbValue je buffer,
  hodnoty se zretezuji;
- ostatni: cbValue je C-velikost, rgbValue je buffer s C-hodnotou.
Pro kazdy typ smi byt cbValue==SQL_NULL_DATA a rgbValue==NULL
cbValue a rgbValue musi byt inicializovany nulami!
*/


SQLRETURN SQL_API SQLPutData(SQLHSTMT	StatementHandle, SQLPOINTER DataPtr, SQLLEN StrLen_or_Ind)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
 /* validate the statement state: */
  if (!stmt->AE_mode)   /* not in the AE_mode! */
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (stmt->AE_mode == AE_PARAMS)
  { if (stmt->last_par_ind<=0 || stmt->last_par_ind>stmt->impl_ipd.count)
    /* PutData called without a preceeding ParamData! */
    { raise_err(stmt, SQL_HY010);  /* Function sequence error */
      stmt->AE_mode=AE_NO;
      return SQL_ERROR;
    }
    t_desc_rec * adrec = stmt->    apd->drecs.acc0(stmt->last_par_ind);  // not NULL, test 07001
    t_desc_rec * idrec = stmt->impl_ipd.drecs.acc0(stmt->last_par_ind); 
    if (adrec->concise_type!=SQL_C_BINARY && adrec->concise_type!=SQL_C_CHAR && adrec->concise_type!=SQL_C_WCHAR)
    { if (adrec->valsize) /* continuation data: allowed only for C char or binary data */
      { raise_err(stmt, SQL_22003);  /* Numeric value out of range */
        stmt->AE_mode=AE_NO;
        return SQL_ERROR;
      }
      if (StrLen_or_Ind != SQL_NULL_DATA)
        StrLen_or_Ind=adrec->value_step;   /* C type size: override! */
    }

   /* send the parameter to server or copy the parameter into a buffer: */
    if (StrLen_or_Ind == SQL_NULL_DATA || StrLen_or_Ind == SQL_DEFAULT_PARAM)
      adrec->valsize=StrLen_or_Ind;
    else /* non NULL value: */
    { if (StrLen_or_Ind==SQL_NTS) StrLen_or_Ind=(SQLINTEGER)strlen((tptr)DataPtr);
      if (IS_LONG_SQL_TYPE(idrec->concise_type))   /* send the part of the parameter value */
      { if (send_long_sql_data(stmt, adrec->concise_type, idrec->concise_type, idrec->specif,
            (tptr)DataPtr, (int)StrLen_or_Ind, (int)adrec->valsize, stmt->last_par_ind)==SQL_ERROR)
        { stmt->AE_mode=AE_NO;
          return SQL_ERROR;
        }
      }
      else /* short parameter, store it */
      { tptr buf=(tptr)corerealloc(adrec->buf, (unsigned)(adrec->valsize+StrLen_or_Ind+1));
        if (!buf)
        { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
          stmt->AE_mode=AE_NO;
          return SQL_ERROR;
        }
        memcpy(buf+adrec->valsize, DataPtr, StrLen_or_Ind);
        buf[adrec->valsize+StrLen_or_Ind]=0;  // necessary if C_CHAR data provided to be converted later
        adrec->buf=buf;
      }
      adrec->valsize+=StrLen_or_Ind;
    }
    return SQL_SUCCESS;
  }
  else   /* column data */
  { if (write_db_data(stmt, (tptr)DataPtr, (SDWORD)StrLen_or_Ind, true)) return SQL_ERROR;
    return SQL_SUCCESS;
  }
}

//	-	-	-	-	-	-	-	-	-

SQLRETURN SQL_API SQLCancel(HSTMT	hstmt)	// Statement to cancel.
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;
  CLEAR_ERR(stmt);
  if (stmt->AE_mode)
  { stmt->apd->clear_AE_params();
    stmt->AE_mode=AE_NO;
  }
  if (stmt->async_run)
  { stmt->cancelled=TRUE;   /* must wait to another call of same fn */
    cd_Break(stmt->cdp);
    close_result_set(stmt); 
    discard_other_result_sets(stmt); // must close all!
  }
	return SQL_SUCCESS;
}

