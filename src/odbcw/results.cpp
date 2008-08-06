/*
** RESULTS.C - This is the ODBC sample driver code for
** returning results and information about results.
*/

#include "wbdrv.h"
#pragma hdrstop
#include <wchar.h>

BOOL write_string(const char * text, tptr szColName,
                  SWORD cbColNameMax, SWORD FAR *pcbColName)
/* writes null-terminated string in a standard way, return TRUE on trunc. */
{ unsigned len=(unsigned)strlen(text);
  if (pcbColName != NULL) *pcbColName=len;
  if ((szColName == NULL) || (cbColNameMax <= 0)) return FALSE;
  strmaxcpy(szColName, text, cbColNameMax);
  return len >= cbColNameMax;
}

SWORD type_C_to_WB(int type)
{ switch (type)
  { case SQL_C_BIT:      return ATT_BOOLEAN;
    case SQL_C_CHAR:     return ATT_STRING;
    case SQL_C_SHORT:   case SQL_C_SSHORT:    return ATT_INT16;
    case SQL_C_LONG:    case SQL_C_SLONG:     return ATT_INT32;
    case SQL_C_TINYINT: case SQL_C_STINYINT:  return ATT_INT8;
                        case SQL_C_SBIGINT:   return ATT_INT64;
    case SQL_C_DOUBLE:   return ATT_FLOAT;
    case SQL_C_BINARY:   return ATT_BINARY;
    case SQL_C_DATE:     return ATT_DATE;
    case SQL_C_TIME:     return ATT_TIME;
    case SQL_C_TIMESTAMP:return ATT_TIMESTAMP;
    case SQL_C_UBIGINT: case SQL_C_UTINYINT:  case SQL_C_ULONG:  case SQL_C_USHORT:  case SQL_C_FLOAT: // types not supported by 602SQL
      return 0;
  }
  return 0;  
}

char * WB_type_name(uns8 type)
{ switch (type)
  { case ATT_BOOLEAN:  return "Boolean";
    case ATT_CHAR:     return "Char";
    case ATT_INT8:     return "TinyInt";
    case ATT_INT16:    return "Short";
    case ATT_INT32:    return "Integer";
    case ATT_INT64:    return "BigInt";
    case ATT_MONEY:    return "Money";
    case ATT_FLOAT:    return "Real";
    case ATT_STRING:   return "String";
    case ATT_BINARY:   return "Binary";
    case ATT_DATE:     return "Date";
    case ATT_TIME:     return "Time";
    case ATT_TIMESTAMP:return "Timestamp";
    case ATT_PTR:      return "Pointer";
    case ATT_BIPTR:    return "Biptr";
    case ATT_AUTOR:    return "Autor";
    case ATT_DATIM:    return "Datumovka";
    case ATT_HIST:     return "Historie";
    case ATT_TEXT:     return "Text";
    case ATT_RASTER:   return "Raster";
    case ATT_NOSPEC:   return "OLE";
    case ATT_SIGNAT:   return "Signature";
    case 0:            return "NULL type";
    default:           return "Unknown";  /* this is an error */
  }
}

void odbc_date2str(uns32 dt, char * tx)
{ if (dt==NONEDATE) { *tx=0;  return; }
  sprintf(tx,"%04u-%02u-%02u", Year(dt), Month(dt), Day(dt));
}

static int get_unsig(char ** tx)
{ int val=0;
  while (**tx>='0' && **tx<='9') { val = 10*val + **tx - '0';  (*tx)++; }
  return val;
}

BOOL odbc_str2date(char * tx, uns32 * dt)
{ while (*tx==' ') tx++;
  if (*tx=='{')
  { tx++;
    if (*tx=='d') tx++;
    while (*tx==' ') tx++;
    if (*tx=='\'') tx++;
  }
  while (*tx==' ') tx++;
  if (!*tx) { *dt=NONEDATE;  return TRUE; }
  if (*tx<'0' || *tx>'9') return FALSE;
  int year =get_unsig(&tx);
  if (*tx=='-') tx++; else return FALSE;
  if (*tx<'0' || *tx>'9') return FALSE;
  int month=get_unsig(&tx);
  if (*tx=='-') tx++; else return FALSE;
  if (*tx<'0' || *tx>'9') return FALSE;
  int day  =get_unsig(&tx);
  if (*tx=='\'') tx++;
  while (*tx==' ') tx++;
  if (*tx=='}') tx++;
  *dt=Make_date(day, month, year);
  return TRUE;
}

void odbc_datim2str(uns32 ts, char * tx)
{ if (ts==NONETIME) { *tx=0;  return; }
  odbc_date2str(timestamp2date(ts), tx);
  tx+=strlen(tx);
  *(tx++)=' ';
  time2str(timestamp2time(ts), tx, 1);
}

BOOL odbc_str2datim(char * tx, uns32 * ts)
{ while (*tx==' ') tx++;
  if (*tx=='{')
  { tx++;
    if (*tx=='t') tx++;  if (*tx=='s') tx++;
    while (*tx==' ') tx++;
    if (*tx=='\'') tx++;
  }
  while (*tx==' ') tx++;
  if (!*tx) { *ts=NONEDATE;  return TRUE; }
  uns32 dt, tm;
  if (!odbc_str2date(tx, &dt)) return FALSE;
  while (*tx!=' ' && *tx) tx++;
  while (*tx==' ') tx++;
  if (!str2time(tx, &tm)) return FALSE;
  if (*tx=='\'') tx++;
  while (*tx==' ') tx++;
  if (*tx=='}') tx++;
  *ts=datetime2timestamp(dt, tm);
  return TRUE;
}

BOOL x_str2odbc_timestamp(char * tx, TIMESTAMP_STRUCT * pdtm)
{ uns32 ts;
  if (!odbc_str2datim(tx, &ts)) return FALSE;
  datim2TIMESTAMP(ts, pdtm);
  return TRUE;
}

BOOL odbc_real2str(double val, tptr txt, int space)
{ char buf[60];
  if (val > 1e40 || val && val < 1e-40 && val > -1e-40 || val < -1e40)
       sprintf(buf, "%e", val);
  else sprintf(buf, "%f", val);
  if (strlen(buf) > space) 
    { strcpy(txt,  "*");  return FALSE; }
  strcpy(txt, buf);
  return TRUE;
}


SQLRETURN ret_string(STMT * stmt, const char * tx, char * CDest, SQLLEN cbValueMax, SQLLEN * pcbValue)
{ int len = (int)strlen(tx);
  if (pcbValue) *pcbValue=len; // Must assign even on error. I do not know if the termiator should be included. Perhaps no.
  if (cbValueMax <= len)
    { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
  strcpy(CDest, tx);
  return SQL_SUCCESS;
}

SQLRETURN ret_wstring(STMT * stmt, const SQLWCHAR * tx, SQLWCHAR * CDest, SQLLEN cbValueMax, SQLLEN * pcbValue)
// Writes SQL_C_WCHAR which is supposed to have 2 byte charactes like SQLWCHAR.
{ int len = (int)sizeof(wuchar) * (int)wuclen(tx);
  if (pcbValue) *pcbValue=len; // Must assign even on error. I do not know if the termiator should be included. Perhaps no.
  if (cbValueMax < len+sizeof(wuchar))
    { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
  wuccpy(CDest, tx);
  return SQL_SUCCESS;
}

#ifdef STOP
#include <iconv.h>

int ODBCWideCharToMultiByte(const char *charset, const char *wstr, size_t len, char *buffer)
// Returns 0 in case of failure, number of written bytes otherwise
{
  char *from=(char *)wstr;
  char *to=buffer;
  size_t tolen=len;
  size_t fromlen=2*tolen;
  iconv_t conv=iconv_open(charset, "UCS-2");
  if (conv==(iconv_t)-1) goto err;
  while (fromlen>0){
    int nconv=iconv(conv, &from, &fromlen, &to, &tolen);
    if (nconv==(size_t)-1){
      iconv_close(conv);
      goto err;
    }
  }
  *to='\0';  // adding a terminator - not counted
  iconv_close(conv);
  return to-buffer;
 err:
  if (len>=4) strcpy(buffer, "!cnv");
  else *buffer=0;
  return 0;
}
#endif

// Znakova data vraci vzdy s terminatorem, leda ze cte var-len po castech.
// Nevim, zda terminator se pocita do pcbValue. Nepocitam ho.

SQLRETURN convert_SQL_to_C(SWORD CType, SWORD SqlType, t_specif sql_specif, tptr SqlSource, tptr CDest,
  SQLLEN cbValueMax, SQLLEN *pcbValue, SQLULEN SqlSize, STMT * stmt,
  unsigned attr, BOOL multiattribute)
// Uses and updates get_data_offset! Uses db_recnum == current record number when reading var-len database types.
// Uses SqlSize, is supposed not to be -1 and not include the char string terminator.
{ char auxstr[MAX_FIXED_STRING_LENGTH+1];  uns16 index = NOINDEX;
  int sqlwbtype;
  if (multiattribute)
#ifdef NOMULTI
  { if (pcbValue!=NULL) *pcbValue=0;
    return SQL_SUCCESS;
  }
#else
  { index = 0;
    if (SqlType != SQL_LONGVARCHAR || SqlType != SQL_VARBINARY || SqlType != SQL_LONGVARBINARY)
    { if (cd_Read_ind(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index, auxstr))
        { raise_db_err(stmt);  return SQL_ERROR; }
      SqlSource=auxstr;
    }
  }
#endif
  if (CType==SQL_C_DEFAULT) CType=default_C_type(SqlType);
  if (CType==SQL_C_BINARY)   /* direct transfer, no conversion: */
  { if (pcbValue) *pcbValue=SqlSize;
    switch (SqlType)
    { case SQL_LONGVARCHAR:  case SQL_WLONGVARCHAR:  case SQL_VARBINARY:  case SQL_LONGVARBINARY:
      // read and store per partes, current offset is in stmt->get_data_offset:
      { uns32 len, rd;
        if (cd_Read_len(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index, &len))
          { raise_db_err(stmt);  return SQL_ERROR; }
        if (pcbValue) 
          if (!len)
            *pcbValue=SQL_NULL_DATA;
          else 
            *pcbValue=len>stmt->get_data_offset ? len-stmt->get_data_offset : 0;
        if (stmt->get_data_offset && len<=stmt->get_data_offset)
          return SQL_NO_DATA;  // after the end when reading in parts - interferes with stmt->get_data_offset=1 above - is it OK?
        if (!len)
        { stmt->get_data_offset=1;  // next call will return SQL_NO_DATA
          break;  // return SQL_SUCCESS
        }
        uns32 rest = len-stmt->get_data_offset;
        uns32 toread = rest > cbValueMax ? (uns32)cbValueMax : rest;
        if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index, stmt->get_data_offset, toread, CDest, &rd) ||
            rd != toread)
          { raise_db_err(stmt);  return SQL_ERROR; }
        stmt->get_data_offset+=rd;
        if (rd<rest)
        { raise_err(stmt, SQL_01004);  /* data will be truncated */
          return SQL_SUCCESS_WITH_INFO;
        }
        break;  // return SQL_SUCCESS
      }
      case SQL_VARCHAR:  case SQL_CHAR:  
        if (cbValueMax <= SqlSize)
        { memcpy(CDest, SqlSource, cbValueMax-1);
          CDest[cbValueMax-1]=0;
          raise_err(stmt, SQL_01004);  /* data will be truncated */
          return SQL_SUCCESS_WITH_INFO;
        }
        else
        { memcpy(CDest, SqlSource, SqlSize);
          CDest[SqlSize]=0;
        }
        break;  // return SQL_SUCCESS
      case SQL_WVARCHAR:  case SQL_WCHAR: // like above, but requires 2 bytes for the terminator
        if (cbValueMax < SqlSize+2)
        { memcpy(CDest, SqlSource, cbValueMax-2);
          CDest[cbValueMax-1]=CDest[cbValueMax-2]=0;
          raise_err(stmt, SQL_01004);  /* data will be truncated */
          return SQL_SUCCESS_WITH_INFO;
        }
        else
        { memcpy(CDest, SqlSource, SqlSize);
          CDest[SqlSize]=0;  CDest[SqlSize+1]=0;
        }
        break;  // return SQL_SUCCESS
      default:  // includes case SQL_BINARY:
        if (cbValueMax < SqlSize)
          { raise_err(stmt, SQL_22003);  return SQL_ERROR; }  /* overflow */
        memcpy(CDest, SqlSource, SqlSize);
        break;  // return SQL_SUCCESS
    }
    return SQL_SUCCESS;
  }
  else switch (SqlType)   /* switch on source type */
  { case SQL_LONGVARCHAR:   /* text of variable length: */
    { uns32 len, rd;
      if (cd_Read_len(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index, &len))
        { raise_db_err(stmt);  return SQL_ERROR; }
      if (CType==SQL_C_CHAR)  // read and store per partes
      { if (pcbValue)   // the length of the full rest of the value must be returned
          *pcbValue=len>stmt->get_data_offset ? len-stmt->get_data_offset : 0;
        if (stmt->get_data_offset && len<=stmt->get_data_offset)
          return SQL_NO_DATA;  // after the end when reading in parts
        if (cbValueMax)
          if (len > stmt->get_data_offset + cbValueMax - 1)  // read and store the next (incomplete) part:
          { if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index,
                         stmt->get_data_offset, (t_varcol_size)(cbValueMax-1), CDest, &rd) ||
                (rd != cbValueMax-1))
              { raise_db_err(stmt);  return SQL_ERROR; }
            CDest[rd]=0;  // each part must be terminated!
            stmt->get_data_offset+=rd;
            raise_err(stmt, SQL_01004);  /* data will be truncated */
            return SQL_SUCCESS_WITH_INFO;
          }
          else   // Read and store the full length, incl. terminator.
          { if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index,
                         stmt->get_data_offset, len-stmt->get_data_offset, CDest, &rd) ||
                (rd != len-stmt->get_data_offset))
              { raise_db_err(stmt);  return SQL_ERROR; }
            CDest[rd]=0;  // terminator added but not included into *pcbValue
            stmt->get_data_offset+=rd;
          }
      }
      else if (CType==SQL_C_WCHAR)  
      { if (pcbValue)   // the length of the full rest of the value must be returned
          *pcbValue=len>stmt->get_data_offset ? 2*(len-stmt->get_data_offset) : 0;
        if (stmt->get_data_offset && len<=stmt->get_data_offset)
          return SQL_NO_DATA;  // after the end when reading in parts
        if (cbValueMax>=2+2)
        { bool incomplete=false;
          int to_read = len - stmt->get_data_offset;
          if (to_read > (cbValueMax - 2)/2)  // read and store the next (incomplete) part:
            { to_read = (int)(cbValueMax - 2)/2;  incomplete=true; }
          char * buffer = (char*)corealloc(to_read+1, 78);
          if (!buffer) { raise_err(stmt, SQL_HY001);  return SQL_ERROR; } /* Memory allocation failure */
          if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index,
                     stmt->get_data_offset, to_read, buffer, &rd) || (rd != to_read))
            { raise_db_err(stmt);  return SQL_ERROR; }
         // convert 1->2: 
#ifdef STOP         
#ifdef WINS		
          int cnt = MultiByteToWideChar(wbcharset_t(sql_specif.charset).cp(), 0 /*MB_PRECOMPOSED gives ERROR_INVALID_FLAGS for UTF-8 on XP*/, buffer, to_read, (wuchar*)CDest, to_read);
#else
		  int cnt = ODBCMultiByteToWideChar(wbcharset_t(sql_specif.charset), buffer, to_read, (wuchar*)CDest);  
#endif		
          CDest[2*cnt]=CDest[2*cnt+1]=0;  // each part must be terminated!
#endif 
          if (conv1to2(buffer, to_read, (wuchar*)CDest, wbcharset_t(sql_specif.charset), (int)cbValueMax-2)==SC_STRING_TRIMMED)
            incomplete=true;
          corefree(buffer);
          stmt->get_data_offset+=to_read;
          if (incomplete)
            { raise_err(stmt, SQL_01004);  return SQL_SUCCESS_WITH_INFO; } /* data will be truncated */
        }
      }
      else  /* read only the 1st part, sufficient for conversion */
      { if (len > MAX_FIXED_STRING_LENGTH)
          len=MAX_FIXED_STRING_LENGTH;  // will be converted to non-char type, beginning must be sufficient
        if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index,
                     0, len, auxstr, &rd) || (rd != len))
          { raise_db_err(stmt);  return SQL_ERROR; }
        /* must be longer than the longest value in order to recognize the truncation */
        auxstr[rd]=0;
        goto conv_from_string;
      }
      break;
    }
    case SQL_WLONGVARCHAR:   /* text of variable length: */
    { uns32 len, rd;
      if (cd_Read_len(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index, &len))
        { raise_db_err(stmt);  return SQL_ERROR; }
      if (CType==SQL_C_WCHAR)  // read and store per partes
      { if (pcbValue) 
          *pcbValue=len>stmt->get_data_offset ? len-stmt->get_data_offset : 0;
        if (stmt->get_data_offset && len<=stmt->get_data_offset)
          return SQL_NO_DATA;  // after the end when reading in parts
        if (cbValueMax>=2)
          if (len > stmt->get_data_offset + cbValueMax)  // read and store the next (incomplete) part:
          { if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index,
                         stmt->get_data_offset, (t_varcol_size)cbValueMax-2, CDest, &rd) ||
                (rd != cbValueMax-2))
              { raise_db_err(stmt);  return SQL_ERROR; }
            CDest[rd]=CDest[rd+1]=0;  // each part must be terminated!
            stmt->get_data_offset+=rd;
            raise_err(stmt, SQL_01004);  /* data will be truncated */
            return SQL_SUCCESS_WITH_INFO;
          }
          else   // Read and store the full length, incl. terminator.
          { if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index,
                         stmt->get_data_offset, len-stmt->get_data_offset, CDest, &rd) ||
                (rd != len-stmt->get_data_offset))
              { raise_db_err(stmt);  return SQL_ERROR; }
            CDest[rd]=CDest[rd+1]=0;  // terminator added but not included into *pcbValue
            stmt->get_data_offset+=rd;
          }
        break;
      }
      else if (CType==SQL_C_CHAR)  // MS Query uses this when reading NCLOBs. Cannot determine the proper C-encoding, but partial support implemented (using the SQL-side language)
      //{ raise_err(stmt, SQL_22003);  return SQL_ERROR; }
      { if (!len)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  /* NULL string */
          if (cbValueMax>=2) CDest[0]=CDest[1]=0;   /* Delphi requires this for SQLTables result */
          break;
        }
        if (stmt->get_data_offset && len<=stmt->get_data_offset)
          { if (pcbValue) *pcbValue=0;  return SQL_NO_DATA; }  // after the end when reading in parts
       // converting to 1-byte encoding (never UTF-8), so I need to read 2*cbValueMax bytes max
        int bufsize = len-stmt->get_data_offset;
        bool incomplete=false;
        if (bufsize > 2*cbValueMax-1) { bufsize = 2*(int)cbValueMax-1;  incomplete=true; }
        char * buf = (char*)corealloc(bufsize+1, 78);
        if (!buf)
          { raise_err(stmt, SQL_HY001);  return SQL_ERROR; } /* Memory allocation failure */
        if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index, stmt->get_data_offset, bufsize, buf, &rd) || rd != bufsize)
          { corefree(buf);  raise_db_err(stmt);  return SQL_ERROR; }
        stmt->get_data_offset+=rd;
        if (conv2to1((const wuchar *)buf, rd/2, CDest, wbcharset_t(sql_specif.charset), (int)cbValueMax-1)==SC_STRING_TRIMMED)
          incomplete=true;
#ifdef STOP          
#ifdef WINS
        int clen=WideCharToMultiByte(wbcharset_t(sql_specif.charset).cp(), 0, (const wuchar *)buf, rd/2, CDest, cbValueMax, NULL, NULL);
#else
        int clen=ODBCWideCharToMultiByte(wbcharset_t(sql_specif.charset), (const char*)buf, rd/2, CDest);  // C charset not specified, using ISO 8859-1 (Latin 1)
#endif
        CDest[clen]=0;
#endif        
        if (pcbValue) *pcbValue=(len-stmt->get_data_offset)/2;
        corefree(buf);
        if (incomplete)
          { raise_err(stmt, SQL_01004);  return SQL_SUCCESS_WITH_INFO; }  /* data will be truncated */
        break;    
      }
      else  /* read only the 1st part, sufficient for conversion */
      { wuchar wauxstr[MAX_FIXED_STRING_LENGTH+1];
        if (len > 2*MAX_FIXED_STRING_LENGTH)
          len=2*MAX_FIXED_STRING_LENGTH;  // will be converted to non-char type, beginning must be sufficient
        if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index,
                     0, len, wauxstr, &rd) || (rd != len))
          { raise_db_err(stmt);  return SQL_ERROR; }
        /* must be longer than the longest value in order to recognize the truncation */
        conv2to1(wauxstr, rd/2, auxstr, wbcharset_t(131), sizeof(auxstr)-1);
#ifdef STOP
#ifdef WINS
		len=WideCharToMultiByte(1252, 0, wauxstr, rd/2, auxstr, sizeof(auxstr), NULL, NULL);
#else
		len=ODBCWideCharToMultiByte(wbcharset_t(131), (const char*)wauxstr, rd/2, auxstr);  // C charset not specified, using ISO 8859-1 (Latin 1)
#endif
        auxstr[len]=0;
#endif        
        goto conv_from_string;
      }
    }
    case SQL_WVARCHAR:  case SQL_WCHAR:
    { int sqllen = (int)wuclen((wuchar*)SqlSource);
      if (sqllen>SqlSize/2) sqllen=(int)SqlSize/2;
      if (CType==SQL_C_WCHAR)
      { if (!sqllen)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  /* NULL string */
          if (cbValueMax>=2) CDest[0]=CDest[1]=0;   /* Delphi requires this for SQLTables result */
        }
        else  // 4-byte chars not supported
        { if (pcbValue) *pcbValue = sqllen*sizeof(SQLWCHAR);
          if ((sqllen+1)*sizeof(SQLWCHAR) > cbValueMax)
          { sqllen = (int)(cbValueMax/sizeof(SQLWCHAR) - 1);
            if (sqllen>=0)  // copy truncated
            { memcpy(CDest, SqlSource, sqllen*sizeof(SQLWCHAR));  
              ((SQLWCHAR*)CDest)[sqllen]=0; 
            }
            raise_err(stmt, SQL_01004);  /* data will be truncated */
            return SQL_SUCCESS_WITH_INFO;
          }
          else // copy all
          { memcpy(CDest, SqlSource, sqllen*sizeof(SQLWCHAR));  
            ((SQLWCHAR*)CDest)[sqllen]=0; 
          }
        }
      }
      else  // convert to bytestring and continue converting
      { conv2to1((const wuchar*)SqlSource, sqllen, auxstr, wbcharset_t(envi_charset), sizeof(auxstr)-1);
#ifdef STOP
#ifdef WINS
		uns32 len=WideCharToMultiByte(wbcharset_t(envi_charset).cp(), 0, (wuchar*)SqlSource, sqllen, auxstr, sizeof(auxstr), NULL, NULL);
#else
		uns32 len=ODBCWideCharToMultiByte(wbcharset_t(envi_charset), SqlSource, sqllen, auxstr);  // C charset not specified, using ISO 8859-1 (Latin 1)
#endif
        auxstr[len]=0;
#endif        
        goto conv_from_string;
      }
      break;
    }
    case SQL_VARCHAR:  /* type not used in the WinBase */
    case SQL_CHAR:     /* converting from the char string: */
      if (SqlSize>MAX_FIXED_STRING_LENGTH)  // internal error
        { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
      memcpy(auxstr, SqlSource, SqlSize);
      auxstr[SqlSize]=0;   /* converted into NTS */
     conv_from_string:
      if (CType==SQL_C_CHAR)
      { unsigned len=(unsigned)strlen(auxstr);
        if (!len)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  /* NULL string */
          if (cbValueMax) CDest[0]=0;   /* Delphi requires this for SQLTables result */
        }
        else
        { if (pcbValue) *pcbValue=len;
          if (len >= cbValueMax)
          { strmaxcpy(CDest, auxstr, (unsigned)cbValueMax);
            raise_err(stmt, SQL_01004);  /* data will be truncated */
            return SQL_SUCCESS_WITH_INFO;
          }
          else strcpy(CDest, auxstr);
        }
      }
      else if (CType==SQL_C_WCHAR)            
      { t_specif c_specif;
        c_specif = sql_specif;  c_specif.wide_char=1;  c_specif.length=(uns16)(cbValueMax-2);
        int retcode = superconv(ATT_STRING, ATT_STRING, SqlSource, CDest, sql_specif, c_specif, NULL);
        if (pcbValue) *pcbValue = (int)sizeof(wuchar) * (int)wuclen((const wuchar*)CDest);
        if (retcode==SC_STRING_TRIMMED)
        { raise_err(stmt, SQL_01004);  /* data will be truncated */
          return SQL_SUCCESS_WITH_INFO;
        }
        if (retcode<0)
          { raise_err(stmt, SQL_22018);  return SQL_ERROR; }
      } 
      else
      { cutspaces(auxstr);
        if (!*auxstr)
          { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
        else switch (CType)
        { case SQL_C_BIT:
          { char d1;
            if (pcbValue) *pcbValue=sizeof(uns8); /* must write even on error */
            d1=*auxstr;
            if ((d1=='A') || (d1=='Y') || (d1=='a') || (d1=='y') || (d1=='1'))
              *CDest='\x1';
            else if ((d1=='n') || (d1=='N') || (d1=='0'))
              *CDest='\x0';
            else
              { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
            break;
          }
//#if 0
          case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
          { sig32 ival;
            if (pcbValue) *pcbValue=sizeof(sig8); /* must write even on error */
            if (!str2int(auxstr, &ival))   /* not a numeric value */
              { raise_err(stmt, SQL_22018);  return SQL_ERROR; }
            if ((ival < -127) || (ival > 127))
              { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
            *(sig8*)CDest=(sig8)ival;
            break;
          }
          case SQL_C_SHORT:    case SQL_C_SSHORT:  case SQL_C_USHORT:
          { sig32 ival;
            if (pcbValue) *pcbValue=sizeof(sig16); /* must write even on error */
            if (!str2int(auxstr, &ival))   /* not a numeric value */
              { raise_err(stmt, SQL_22018);  return SQL_ERROR; }
            if ((ival < -0x7fffL) || (ival > 0x7ffffL))
              { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
            *(sig16*)CDest=(sig16)ival;
            break;
          }
          case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
            if (pcbValue) *pcbValue=sizeof(sig32); /* must write even on error */
            if (!str2int(auxstr, (sig32*)CDest))  /* not a numeric value */
              { raise_err(stmt, SQL_22018);  return SQL_ERROR; }
            break;
          case SQL_C_SBIGINT:    case SQL_C_UBIGINT:
            if (pcbValue) *pcbValue=sizeof(sig64); /* must write even on error */
            if (!str2int64(auxstr, (sig64*)CDest))  /* not a numeric value */
              { raise_err(stmt, SQL_22018);  return SQL_ERROR; }
            break;
          case SQL_C_FLOAT:
          { double r;
            if (pcbValue) *pcbValue=sizeof(float); /* must write even on error */
            if (!str2real(auxstr, &r))  /* not a numeric value */
              { raise_err(stmt, SQL_22018);  return SQL_ERROR; }
            if ((r < -3.4E38) || (r > 3.4E38))  /* underflow not checked */
              { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
            *(float*)CDest=(float)r;
            break;
          }
          case SQL_C_DOUBLE:
            if (pcbValue) *pcbValue=sizeof(double); /* must write even on error */
            if (!str2real(auxstr, (double*)CDest)) /* not a numeric value */
              { raise_err(stmt, SQL_22018);  return SQL_ERROR; }
            break;
//#else
//#endif
          case SQL_C_TYPE_DATE:   case SQL_C_DATE:
          { uns32 dt;  DATE_STRUCT * pdt = (DATE_STRUCT *)CDest;
            if (pcbValue) *pcbValue=sizeof(DATE_STRUCT); /* must write even on error */
            if (!odbc_str2date(auxstr, &dt))
              { raise_err(stmt, SQL_22008);  return SQL_ERROR; }
            pdt->day=Day(dt);  pdt->month=Month(dt);  pdt->year=Year(dt);
            break;
          }
          case SQL_C_TYPE_TIME:   case SQL_C_TIME:
          { uns32 tm;  TIME_STRUCT * ptm = (TIME_STRUCT *)CDest;
            if (pcbValue) *pcbValue=sizeof(TIME_STRUCT);
            if (!str2time(auxstr, &tm))
              { raise_err(stmt, SQL_22008);  return SQL_ERROR; }
            ptm->hour=Hours(tm); ptm->minute=Minutes(tm); ptm->second=Seconds(tm);
            break;
          }
          case SQL_C_TYPE_TIMESTAMP:  case SQL_C_TIMESTAMP:
          { TIMESTAMP_STRUCT * pdtm = (TIMESTAMP_STRUCT *)CDest;
            if (pcbValue) *pcbValue=sizeof(TIME_STRUCT);
            if (!x_str2odbc_timestamp(auxstr, pdtm))
              { raise_err(stmt, SQL_22008);  return SQL_ERROR; }
            break;
          }
          default:
            raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
            return SQL_ERROR;
        }  /* C type switch */
      } /* not string */
      break;
    case SQL_BIT:  /* WinBase Boolean type */
    { uns8 b = *SqlSource;
      if (b==NONEBOOLEAN)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else switch (CType)
      { case SQL_C_BIT:
          if (pcbValue) *pcbValue=sizeof(uns8);
          *CDest=b;
          break;
        case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
          if (pcbValue) *pcbValue=sizeof(sig8);
          *CDest=b;
          break;
        case SQL_C_SHORT:    case SQL_C_SSHORT:  case SQL_C_USHORT:
          if (pcbValue) *pcbValue=sizeof(sig16);
          *(sig16*)CDest=(sig16)b;
          break;
        case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
          if (pcbValue) *pcbValue=sizeof(sig32);
          *(sig32*)CDest=(sig32)b;
          break;
        case SQL_C_SBIGINT:    case SQL_C_UBIGINT:
          if (pcbValue) *pcbValue=sizeof(sig64);
          *(sig64*)CDest=(sig64)b;
          break;
        case SQL_C_FLOAT:
          if (pcbValue) *pcbValue=sizeof(float);
          *(float*)CDest=(float)b;
          break;
        case SQL_C_DOUBLE:
          if (pcbValue) *pcbValue=sizeof(double);
          *(double*)CDest=(double)b;
          break;
        case SQL_C_CHAR:
          if (pcbValue) *pcbValue=2;  /* must assign even on error */
          if (cbValueMax < 2)
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          CDest[0]=b ? '1' : '0';  CDest[1]=0;
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return SQL_ERROR;
      }
      break;
    }
#if 0
    case SQL_SMALLINT:  /* WinBase Short type */
    { sig16 b = *(sig16*)SqlSource;
      if (b==NONESHORT)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else switch (CType)
      { case SQL_C_BIT:
          if (pcbValue) *pcbValue=sizeof(uns8); /* must assign even on error */
          if (b && (b!=1)) { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *CDest=(uns8)b;
          break;
        case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
          if (pcbValue) *pcbValue=sizeof(sig8); /* must assign even on error */
          if ((b < -127) || (b>127))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *CDest=(sig8)b;
          break;
        case SQL_C_SHORT:    case SQL_C_SSHORT:  case SQL_C_USHORT:
          if (pcbValue) *pcbValue=sizeof(sig16);
          *(sig16*)CDest=b;
          break;
        case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
          if (pcbValue) *pcbValue=sizeof(sig32);
          *(sig32*)CDest=(sig32)b;
          break;
        case SQL_C_SBIGINT:    case SQL_C_UBIGINT:
          if (pcbValue) *pcbValue=sizeof(sig64);
          *(sig64*)CDest=(sig64)b;
          break;
        case SQL_C_FLOAT:
          if (pcbValue) *pcbValue=sizeof(float);
          *(float*)CDest=(float)b;
          break;
        case SQL_C_DOUBLE:
          if (pcbValue) *pcbValue=sizeof(double);
          *(double*)CDest=(double)b;
          break;
        case SQL_C_CHAR:
        { char tx[1+5+1];  int2str(b, tx);
          return ret_string(stmt, tx, CDest, cbValueMax, pcbValue);
        }
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return SQL_ERROR;
      }
      break;
    }
    case SQL_INTEGER:   /* WinBase Integer type */
    { sig32 b = *(sig32*)SqlSource;
      if (b==NONEINTEGER)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else switch (CType)
      { case SQL_C_BIT:
          if (pcbValue) *pcbValue=sizeof(uns8); /* must assign even on error */
          if (b && (b!=1)) { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *CDest=(uns8)b;
          break;
        case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
          if (pcbValue) *pcbValue=sizeof(sig8); /* must assign even on error */
          if ((b < -127) || (b > 127))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *CDest=(sig8)b;
          break;
        case SQL_C_SHORT:    case SQL_C_SSHORT:  case SQL_C_USHORT:
          if (pcbValue) *pcbValue=sizeof(sig16); /* must assign even on error */
          if ((b < -0x7fff) || (b > 0x7fff))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *(sig16*)CDest=(sig16)b;
          break;
        case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
          if (pcbValue) *pcbValue=sizeof(sig32);
          *(sig32*)CDest=b;
          break;
        case SQL_C_SBIGINT:    case SQL_C_UBIGINT:
          if (pcbValue) *pcbValue=sizeof(sig64);
          *(sig64*)CDest=(sig64)b;
          break;
        case SQL_C_FLOAT:
          if (pcbValue) *pcbValue=sizeof(float);
          *(float*)CDest=(float)b;
          break;
        case SQL_C_DOUBLE:
          if (pcbValue) *pcbValue=sizeof(double);
          *(double*)CDest=(double)b;
          break;
        case SQL_C_CHAR:
        { char tx[1+10+1];  int2str(b, tx);
          return ret_string(stmt, tx, CDest, cbValueMax, pcbValue);
        }
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return SQL_ERROR;
      }
      break;
    }
    case SQL_TINYINT:  
    { sig8 b = *SqlSource;
      if (b==(sig8)0x80)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else switch (CType)
      { case SQL_C_BIT:
          if (pcbValue) *pcbValue=sizeof(uns8);
          *CDest=b;
          break;
        case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
          if (pcbValue) *pcbValue=sizeof(sig8);
          *CDest=b;
          break;
        case SQL_C_SHORT:    case SQL_C_SSHORT:  case SQL_C_USHORT:
          if (pcbValue) *pcbValue=sizeof(sig16);
          *(sig16*)CDest=(sig16)b;
          break;
        case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
          if (pcbValue) *pcbValue=sizeof(sig32);
          *(sig32*)CDest=(sig32)b;
          break;
        case SQL_C_SBIGINT:    case SQL_C_UBIGINT:
          if (pcbValue) *pcbValue=sizeof(sig64);
          *(sig64*)CDest=(sig64)b;
          break;
        case SQL_C_FLOAT:
          if (pcbValue) *pcbValue=sizeof(float);
          *(float*)CDest=(float)b;
          break;
        case SQL_C_DOUBLE:
          if (pcbValue) *pcbValue=sizeof(double);
          *(double*)CDest=(double)b;
          break;
        case SQL_C_CHAR:
        { char tx[1+3+1];  int2str(b, tx);
          return ret_string(stmt, tx, CDest, cbValueMax, pcbValue);
        }
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return SQL_ERROR;
      }
      break;
    }
    case SQL_BIGINT:  
    { sig64 b = *(sig64*)SqlSource;
      if (b==NONEBIGINT)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else switch (CType)
      { case SQL_C_BIT:
          if (pcbValue) *pcbValue=sizeof(uns8); /* must assign even on error */
          if (b && (b!=1)) { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *CDest=(uns8)b;
          break;
        case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
          if (pcbValue) *pcbValue=sizeof(sig8); /* must assign even on error */
          if ((b < -127) || (b > 127))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *CDest=(sig8)b;
          break;
        case SQL_C_SHORT:    case SQL_C_SSHORT:  case SQL_C_USHORT:
          if (pcbValue) *pcbValue=sizeof(sig16); /* must assign even on error */
          if ((b < -0x7fff) || (b > 0x7fff))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *(sig16*)CDest=(sig16)b;
          break;
        case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
          if (pcbValue) *pcbValue=sizeof(sig32);
          if (b < -0x7fffffffL || b > 0x7fffffffL)
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *(sig32*)CDest=(sig32)b;
          break;
        case SQL_C_SBIGINT:    case SQL_C_UBIGINT:
          if (pcbValue) *pcbValue=sizeof(sig64);
          *(sig64*)CDest=b;
          break;
        case SQL_C_FLOAT:
          if (pcbValue) *pcbValue=sizeof(float);
          *(float*)CDest=(float)b;
          break;
        case SQL_C_DOUBLE:
          if (pcbValue) *pcbValue=sizeof(double);
          *(double*)CDest=(double)b;
          break;
        case SQL_C_CHAR:
        { char tx[1+19+1];  int64tostr(b, tx);
          return ret_string(stmt, tx, CDest, cbValueMax, pcbValue);
        }
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return SQL_ERROR;
      }
      break;
    }
    case SQL_FLOAT:  case SQL_DOUBLE:  case SQL_REAL:  /* WinBase Real type */
      db = *(double*)SqlSource;
     realconv:
      if (db==NONEREAL)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else switch (CType)
      { case SQL_C_BIT:
          if (pcbValue) *pcbValue=sizeof(uns8); /* must assign even on error */
          if (db && (db!=1.0)) { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *CDest=(uns8)db;
          break;
        case SQL_C_TINYINT: case SQL_C_STINYINT: case SQL_C_UTINYINT:
          if (pcbValue) *pcbValue=sizeof(sig8); /* must assign even on error */
          if ((db < -127.0) || (db > 127.0))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *CDest=(sig8)db;
          break;
        case SQL_C_SHORT:    case SQL_C_SSHORT:  case SQL_C_USHORT:
          if (pcbValue) *pcbValue=sizeof(sig16); /* must assign even on error */
          if ((db < -0x7fff) || (db > 0x7fff))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *(sig16*)CDest=(sig16)db;
          break;
        case SQL_C_LONG:    case SQL_C_SLONG:    case SQL_C_ULONG:
          if (pcbValue) *pcbValue=sizeof(sig32); /* must assign even on error */
          if ((db < -0x7fffffffL) || (db > 0x7fffffffL))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *(sig32*)CDest=(sig32)db;
          break;
        case SQL_C_SBIGINT:    case SQL_C_UBIGINT:
          if (pcbValue) *pcbValue=sizeof(sig64);
          if ((db < -MAXBIGINT) || (db > MAXBIGINT))
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *(sig64*)CDest=(sig64)db;
          break;
        case SQL_C_FLOAT:
          if (pcbValue) *pcbValue=sizeof(float); /* must assign even on error */
          if ((db < -3.4E38) || (db > 3.4E38))  /* underflow not checked */
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          *(float*)CDest=(float)db;
          break;
        case SQL_C_DOUBLE:
          if (pcbValue) *pcbValue=sizeof(double);
          *(double*)CDest=db;
          break;
        case SQL_C_CHAR:
//          if (cbValueMax <= 7)
//          { if (pcbValue) *pcbValue=7+14+1;
//            raise_err(stmt, SQL_22003);
//            return SQL_ERROR;
//          }
#ifdef STOP
          if (cbValueMax <= 7+14+1)   /* space limited, truncation */
          { char buf[30];
            if (pcbValue) *pcbValue=7+14+1;
            real2str(db, buf, (cbValueMax <= 7) ? 0 : cbValueMax-7-1);
            if (strlen(buf) >= cbValueMax)
            { raise_err(stmt, SQL_22003);
              return SQL_ERROR;
            }
            strcpy(CDest, buf);
            raise_err(stmt, SQL_01004);
            return SQL_SUCCESS_WITH_INFO;
          }
          else   /* space big enough */
#endif
          { if (!odbc_real2str(db, CDest, cbValueMax))
            { raise_err(stmt, SQL_01004);
              return SQL_SUCCESS_WITH_INFO;
            }
            if (pcbValue) *pcbValue=strlen(CDest);
          }
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return SQL_ERROR;
      }
      break;
    case SQL_DECIMAL:  case SQL_NUMERIC:  /* WinBase Money type */
      if (IS_NULLMONEY((monstr*)SqlSource))
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else if (CType==SQL_C_CHAR)
      { char tx[30];  money2str((monstr*)SqlSource, tx, 1);  /* MS Query does not accept 12,- format */
        return ret_string(stmt, tx, CDest, cbValueMax, pcbValue);
      }
      else
      { db=money2real((monstr*)SqlSource);
        goto realconv;
      }
      break;
#else
    case SQL_SMALLINT:  /* WinBase Short type */
      if (*(sig16*)SqlSource==NONESHORT)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  break; }
      sqlwbtype=ATT_INT16;
      goto integral;
    case SQL_INTEGER:   /* WinBase Integer type */
      if (*(sig32*)SqlSource==NONEINTEGER)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  break; }
      sqlwbtype=ATT_INT32;
      goto integral;
    case SQL_TINYINT:  
      if (*(sig8 *)SqlSource==(sig8)0x80)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  break; }
      sqlwbtype=ATT_INT8;
      goto integral;
    case SQL_BIGINT:  
      if (*(sig64*)SqlSource==NONEBIGINT)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  break; }
      sqlwbtype=ATT_INT64;
      goto integral;
    case SQL_FLOAT:  case SQL_DOUBLE:  case SQL_REAL:  /* WinBase Real type */
      if (*(double*)SqlSource==NONEREAL)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  break; }
      sqlwbtype=ATT_FLOAT;
      goto integral;
    case SQL_DECIMAL:  case SQL_NUMERIC:  /* WinBase Money type */
      if (IS_NULLMONEY((monstr*)SqlSource))
        { if (pcbValue) *pcbValue=SQL_NULL_DATA;  break; }
      sqlwbtype=ATT_MONEY;
     integral:
      { int convres;  const char * format = "";
        if (sqlwbtype==ATT_FLOAT)
		{ double val = *(double*)SqlSource;
		  if (val<0) val=-val;
		  if (val>1e8 || val>0 && val<1e-8) format=".5e";
		  else format=".5f";
		}
		if (CType==SQL_C_CHAR)
        { char strbuf[50];
          convres = superconv(sqlwbtype, ATT_STRING, SqlSource, strbuf, sql_specif, t_specif(50, 0, 1, 0), format);
          return ret_string(stmt, strbuf, CDest, cbValueMax, pcbValue);
        }
        else if (CType==SQL_C_WCHAR)
        { SQLWCHAR strbuf[50];
          convres = superconv(sqlwbtype, ATT_STRING, SqlSource, (char*)strbuf, sql_specif, t_specif(50, 0, 1, sizeof(SQLWCHAR)==2 ? 1 : 2), format);
          return ret_wstring(stmt, strbuf, (SQLWCHAR*)CDest, cbValueMax, pcbValue);
        }
        else
        { int cwbtype = type_C_to_WB(CType);  int valsize;
          if (cwbtype && cwbtype!=ATT_BOOLEAN)
          {   
		    convres = superconv(sqlwbtype, cwbtype, SqlSource, CDest, sql_specif, t_specif(), "");
            valsize = tpsize[cwbtype];
          }
          else if (CType==SQL_C_FLOAT) // special C types not supported by 602SQL
          { double dval;  
            convres = superconv(sqlwbtype, ATT_FLOAT, SqlSource, (char*)&dval, sql_specif, t_specif(), "");
            if (convres>=0)
              *(float*)CDest = (float)dval;
            valsize=sizeof(float);
          }
          else  // special C types not supported by 602SQL, and SQL_C_BIT
          { sig64 sval64;  
            convres = superconv(sqlwbtype, ATT_INT64, SqlSource, (char*)&sval64, sql_specif, t_specif(), "");
            if (convres>=0) 
            { switch (CType)
              { case SQL_C_UBIGINT: 
                  if (sval64 < 0)
                    { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
                  valsize=8;  break;
                case SQL_C_UTINYINT:  
                  if (sval64 < 0 || sval64 > 0xff)
                    { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
                  valsize=1;  break;
                case SQL_C_ULONG:  
                  if (sval64 < 0 || sval64 > 0xffffffff)
                    { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
                  valsize=4;  break;
                case SQL_C_USHORT:
                  if (sval64 < 0 || sval64 > 0xffff)
                    { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
                  valsize=2;  break;
                case SQL_C_BIT:
                  if (sval64 && sval64 != 1)
                    { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
                  valsize=1;  break;
                default:
                  raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
                  return SQL_ERROR;
              }
              memcpy(CDest, &sval64, valsize);
            }
          }
          if (pcbValue) *pcbValue=valsize; 
          if (convres==SC_VALUE_OUT_OF_RANGE)
            { raise_err(stmt, SQL_22003);  return TRUE; }
        }
      }
      break;
#endif
    case SQL_TYPE_DATE:   case SQL_DATE: /* WinBase Date type */
    { uns32 dt = *(uns32*)SqlSource;  /* internal date value */
      if (dt==NONEDATE)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else switch (CType)
      { case SQL_C_TYPE_DATE:       case SQL_C_DATE:
        { DATE_STRUCT * pdt = (DATE_STRUCT *)CDest;
          pdt->day=Day(dt);  pdt->month=Month(dt);  pdt->year=Year(dt);
          if (pcbValue) *pcbValue=sizeof(DATE_STRUCT);
          break;
        }
        case SQL_C_TYPE_TIMESTAMP:  case SQL_C_TIMESTAMP:
        { TIMESTAMP_STRUCT * pts = (TIMESTAMP_STRUCT *)CDest;
          pts->day=Day(dt);  pts->month=Month(dt);  pts->year=Year(dt);
          pts->hour=pts->minute=pts->second=0;  pts->fraction=0;
          if (pcbValue) *pcbValue=sizeof(TIMESTAMP_STRUCT);
          break;
        }
        case SQL_C_CHAR:
          if (pcbValue) *pcbValue=10; /* must assign even on error */
          if (cbValueMax<11)
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          odbc_date2str(dt, CDest);
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return SQL_ERROR;
      }
      break;
    }
    case SQL_TYPE_TIME:   case SQL_TIME:  /* WinBase Time type */
    { uns32 tm = *(uns32*)SqlSource;  /* internal time value */
      if (tm==NONETIME)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else switch (CType)
      { case SQL_C_TYPE_TIME:       case SQL_C_TIME:
        { TIME_STRUCT * ptm = (TIME_STRUCT *)CDest;
          ptm->hour=Hours(tm); ptm->minute=Minutes(tm); ptm->second=Seconds(tm);  
		  // ptm->fraction=Sec1000(tm); no such member in TIME_STRUCT
          if (pcbValue) *pcbValue=sizeof(TIME_STRUCT);
          break;
        }
        case SQL_C_TYPE_TIMESTAMP:  case SQL_C_TIMESTAMP:
        { TIMESTAMP_STRUCT * pts = (TIMESTAMP_STRUCT *)CDest;
          pts->hour=Hours(tm);      pts->minute=Minutes(tm);
          pts->second=Seconds(tm);  pts->fraction=Sec1000(tm);
          pts->day=pts->month=pts->year=0;
          if (pcbValue) *pcbValue=sizeof(TIMESTAMP_STRUCT);
          break;
        }
        case SQL_C_CHAR:
          if (pcbValue) *pcbValue=8; /* must assign even on error */
          if (cbValueMax<9)
            { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
          time2str(tm, CDest, 2);  // change: supporting fractional part
          break;
        default:
          raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
          return SQL_ERROR;
      }
      break;
    }
    case SQL_TYPE_TIMESTAMP:  case SQL_TIMESTAMP:  /* WinBase Datim type */
    { int year, month, day, hour, minute, second;
      uns32 clock, dt, ts = *(uns32*)SqlSource;  /* internal datim value */
      if (ts==NONEINTEGER)
        { if (pcbValue) *pcbValue=SQL_NULL_DATA; }
      else
      { clock  = ts    % 86400L;
        dt     = ts    / 86400L;
        day    = (int)(dt    % 31) + 1;  dt /= 31;
        month  = (int)(dt    % 12) + 1;
        year   = (int)(dt    / 12) + 1900;
        second = (int)(clock % 60);  clock /= 60;
        minute = (int)(clock % 60);
        hour   = (int)(clock / 60);
        switch (CType)
        { case SQL_C_TYPE_TIME:  case SQL_C_TIME:
          { TIME_STRUCT * ptm = (TIME_STRUCT *)CDest;
            ptm->hour=hour;  ptm->minute=minute;  ptm->second=second;
            if (pcbValue) *pcbValue=sizeof(TIME_STRUCT);
            raise_err(stmt, SQL_01004);   /* Data truncated */
            return SQL_SUCCESS_WITH_INFO;
          }
          case SQL_C_TYPE_DATE:  case SQL_C_DATE:
          { DATE_STRUCT * pdt = (DATE_STRUCT *)CDest;
            pdt->day=day;  pdt->month=month;  pdt->year=year;
            if (pcbValue) *pcbValue=sizeof(DATE_STRUCT);
            raise_err(stmt, SQL_01004);   /* Data truncated */
            return SQL_SUCCESS_WITH_INFO;
          }
          case SQL_C_TYPE_TIMESTAMP:  case SQL_C_TIMESTAMP:
          { TIMESTAMP_STRUCT * pts = (TIMESTAMP_STRUCT *)CDest;
            datim2TIMESTAMP(ts, pts);
            if (pcbValue) *pcbValue=sizeof(TIMESTAMP_STRUCT);
            break;
          }
          case SQL_C_CHAR:
            if (pcbValue) *pcbValue=18; /* must assign even on error */
            if (cbValueMax<19)
              { raise_err(stmt, SQL_22003);  return SQL_ERROR; }
            odbc_datim2str(ts, CDest);
            break;
          default:
            raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
            return SQL_ERROR;
        }
      }
      break;
    }
    case SQL_BINARY:
    /* conversion to SQL_C_BINARY is handled above */
      if (CType==SQL_C_CHAR)
      { if (pcbValue) *pcbValue=2*SqlSize;
        BOOL trunc=FALSE;  unsigned dg;  tptr p;
        if (2*SqlSize+1 > cbValueMax)
          { SqlSize=(int)(cbValueMax-1)/2;  trunc=TRUE; }
        p=CDest;
        for (int i=0;  i<SqlSize;  i++)
        { dg=(unsigned char)SqlSource[i]>>4;
          *(p++)=(char) ((dg > 9) ? 'A'+dg-10 : '0'+dg);
          dg=SqlSource[i] & 0xf;
          *(p++)=(char) ((dg > 9) ? 'A'+dg-10 : '0'+dg);
        }
        *p=0;
        if (trunc)
        { raise_err(stmt, SQL_01004);  /* data will be truncated */
          return SQL_SUCCESS_WITH_INFO;
        }
      }
      else
      { raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
        return SQL_ERROR;
      }
      break;
    case SQL_VARBINARY:  case SQL_LONGVARBINARY:
    /* conversion to SQL_C_BINARY is handled above */
      if (CType==SQL_C_CHAR)
      { char buf[256];  uns32 rd, rdstep, space, rest;  int i;
        unsigned dg;  tptr p;
        uns32 len;
        if (cd_Read_len(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index, &len))
          { raise_db_err(stmt);  return SQL_ERROR; }
        space = (uns32)cbValueMax-1 / 2;  p=CDest;
        if (pcbValue) 
          *pcbValue = len>stmt->get_data_offset ? 2*(len-stmt->get_data_offset) : 0;  /* must set even on error */
        if (stmt->get_data_offset && len<=stmt->get_data_offset)
          return SQL_NO_DATA;  // after the end when reading in parts
        while (stmt->get_data_offset < len && space)
        { rest=len - stmt->get_data_offset;
          rdstep = (rest < space) ? rest : space;
          if (rdstep > 256) rdstep=256;
          if (cd_Read_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, attr, index,
                  stmt->get_data_offset, rdstep, buf, &rd) || (rd != rdstep))
            { raise_db_err(stmt);  return SQL_ERROR; }
          stmt->get_data_offset+=rdstep;  space-=rdstep;
          for (i=0;  i<rd;  i++)
          { dg=(unsigned char)buf[i]>>4;
            *(p++)=(char) ((dg > 9) ? 'A'+dg-10 : '0'+dg);
            dg=buf[i] & 0xf;
            *(p++)=(char) ((dg > 9) ? 'A'+dg-10 : '0'+dg);
          }
        }
        *p=0;   /* terminating byte */
        if (stmt->get_data_offset < len)
        { raise_err(stmt, SQL_01004);  /* data will be truncated */
          return SQL_SUCCESS_WITH_INFO;
        }
      }
      else
      { raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
        return SQL_ERROR;
      }
      break;
    case 0: // NULL type:
      if (pcbValue) *pcbValue=SQL_NULL_DATA;
      break;
    default:   /* unknown SQL type */
      raise_err(stmt, SQL_07006); /* Restricted data type attribute violation */
      return SQL_ERROR;
  }
  return SQL_SUCCESS;
}

void get_bound_access(STMT * stmt, t_desc_rec * adrec, t_desc_rec * idrec, int iter, tptr & data, SQLLEN *& octet_len, SQLLEN *& indicator,
                     int & ctype, SQLLEN & bufsize)
{ data      = (tptr)adrec->data_ptr;
  if (data==NULL) return;  // column not bound -> no operation
  octet_len = adrec->octet_length_ptr;
  indicator = adrec->indicator_ptr;
 // temp. update of cbValueMax: 
  bufsize=adrec->octet_length;
  ctype  =adrec->concise_type;
  if (ctype==SQL_C_DEFAULT)
    ctype=default_C_type(idrec->concise_type);
  if (ctype!=SQL_C_CHAR && ctype!=SQL_C_WCHAR && ctype!=SQL_C_BINARY)
    bufsize=C_type_size(ctype); /* C type size must be used, not the vale specified in BindCol! */
 // offsetting the pointers by bind_offset_ptr:
  if (stmt->ard->bind_offset_ptr!=NULL)
  { unsigned disp = *stmt->ard->bind_offset_ptr;
    data += disp;
    if (octet_len!=NULL) octet_len=(SQLLEN*)((tptr)octet_len+disp);
    if (indicator!=NULL) indicator=(SQLLEN*)((tptr)indicator+disp);
  }
 // offsetting the pointers by set_pos (loading array):
  if (iter)
    if (stmt->ard->bind_type!=SQL_BIND_BY_COLUMN)  /* row-wise rowset */
    { unsigned disp = iter * stmt->ard->bind_type;
      if (octet_len!=NULL) octet_len=(SQLLEN*)((tptr)octet_len+disp);
      if (indicator!=NULL) indicator=(SQLLEN*)((tptr)indicator+disp);
      data     += disp;
    }
    else   /* column-wise rowset */
    { if (octet_len!=NULL) octet_len+=iter;
      if (indicator!=NULL) indicator+=iter;
      data += iter * bufsize;
    }
}

#define MULTILOAD  // tries to load multiple records a time
#define PACKET_SIZE_LIMIT 64000  // if a single record is bigger, the buffer is bigger!

bool load_records(STMT * stmt)
// Loads record specified by stmt->db_recnum and successive records (if possible).
{
#ifdef MULTILOAD
  cd_start_package(stmt->cdp);  
  int pack_len=0;  int pack_count=0;  trecnum rec=stmt->db_recnum;  char * buf = stmt->rowbuf;
  while (pack_count<MAX_PACKAGED_REQS && 
         (pack_len==0 || pack_len+stmt->rowbufsize<=PACKET_SIZE_LIMIT) && 
         (stmt->row_cnt==-1 || rec<stmt->row_cnt))
  { cd_Read_record(stmt->cdp, stmt->cursnum, rec, buf, stmt->rowbufsize);
    rec++;  buf+=stmt->rowbufsize;  pack_count++;  pack_len+=stmt->rowbufsize;
  }  
  cd_send_package(stmt->cdp);
  if (!cd_Sz_error(stmt->cdp))   // package loaded ok
  { stmt->preloaded_recnum=stmt->db_recnum;
    stmt->preloaded_count=pack_count;
    return true;
  }
#endif
 // load only 1 record:
  if (stmt->row_cnt!=-1 && stmt->row_cnt<=stmt->db_recnum ||  // record does not exist
      cd_Read_record(stmt->cdp, stmt->cursnum, stmt->db_recnum, stmt->rowbuf, stmt->rowbufsize)) // loading error
  { stmt->preloaded_recnum=NORECNUM;
    stmt->preloaded_count=0;
    return false;
  }
  stmt->preloaded_recnum=stmt->db_recnum;
  stmt->preloaded_count=1;
  return true;
}

BOOL db_read(STMT * stmt, BOOL * row_ok)
// Loads (if necessary) and writes into the bound buffers the record specified by stmt->db_recnum.
{ BOOL res=FALSE;
  UWORD * RowStatus = stmt->impl_ird.array_status_ptr;  /* may be overwritten if stmt nesting */
 // load the values unless preloaded:
  if (stmt->preloaded_recnum==NORECNUM ||                            // nothing preloaded
      stmt->db_recnum< stmt->preloaded_recnum ||                     // preloaded values not usable
      stmt->db_recnum>=stmt->preloaded_recnum+stmt->preloaded_count) // preloaded values not usable
    if (!load_records(stmt) || stmt->preloaded_count==0)  // load the records(s)
    {// process the loading error: end of records or reading error:
      if (stmt->row_cnt==-1)
        if (cd_Rec_cnt(stmt->cdp, stmt->cursnum, (uns32*)&stmt->row_cnt))
          stmt->row_cnt=stmt->db_recnum;
      if (stmt->db_recnum >= stmt->row_cnt)
      { if (RowStatus!=NULL) RowStatus[stmt->num_in_rowset]=SQL_ROW_NOROW;
        return TRUE;
      }
      else  /* reading error */
      { raise_err(stmt, SQL_01S01);   /* Error in row */
        res=TRUE;
      }
    }
  if (!res)  // values are available
  { char * rowvals = stmt->rowbuf + stmt->rowbufsize * (stmt->db_recnum - stmt->preloaded_recnum);
    SQLLEN * octet_len, * indicator;  tptr data;
    unsigned i;  RETCODE result2;  int ctype;  SQLLEN bufsize;
    if (row_ok) *row_ok=TRUE;
    res=FALSE;
   // write bookmark, if bound
    t_desc_rec * idrec = stmt->impl_ird.drecs.acc(0);
    t_desc_rec * adrec = stmt->    ard->drecs.acc(0);  adrec->concise_type=SQL_C_ULONG;
    get_bound_access(stmt, adrec, idrec, stmt->num_in_rowset, data, octet_len, indicator, ctype, bufsize);
    if (data!=NULL) // store the record values into bound columns locations: 
    { *(uns32*)data=stmt->db_recnum;
      if (octet_len!=NULL) *octet_len=sizeof(uns32);
      if (indicator!=NULL && indicator!=octet_len) *indicator=0;
    }

   // write bound data: 
    for (i=1;  i<=stmt->ard->count;  i++)
    { t_desc_rec * idrec = stmt->impl_ird.drecs.acc0(i);
      t_desc_rec * adrec = stmt->    ard->drecs.acc0(i);
      get_bound_access(stmt, adrec, idrec, stmt->num_in_rowset, data, octet_len, indicator, ctype, bufsize);
      if (data!=NULL) // store the record values into bound columns locations: 
      { stmt->get_data_offset=0;
        result2=convert_SQL_to_C(ctype, idrec->concise_type, idrec->specif, idrec->offset!=-1 ? rowvals+idrec->offset : NULL,
          data, bufsize, octet_len, idrec->length, stmt, i, idrec->special);
        if (result2==SQL_ERROR) res=TRUE;
        else if (result2==SQL_SUCCESS_WITH_INFO)
          stmt->was_any_info=TRUE;
        if (indicator!=NULL && indicator!=octet_len) 
          *indicator = *octet_len==SQL_NULL_DATA ? SQL_NULL_DATA : 0;
      }
    }
  }
  if (RowStatus!=NULL)
    RowStatus[stmt->num_in_rowset]=res ? SQL_ROW_ERROR : SQL_ROW_SUCCESS;
  return res;
}

static SQLRETURN general_fetch(STMT * stmt)
// Supposes curr_rowset_start defined. Defines num_in_rowset and db_recnum.
{ stmt->get_data_column=(UWORD)-1;  /* no column can continue reading */
  stmt->is_bulk_position=FALSE;
  SQLRETURN result=SQL_SUCCESS;
  stmt->async_run=TRUE;
  if (stmt->impl_ird.rows_procsd_ptr!=NULL) *stmt->impl_ird.rows_procsd_ptr=0;

  BOOL any_row_ok = FALSE;
  if (!stmt->retrieve_data) // retrieving OFF, define array_status_ptr only
  { if (stmt->row_cnt==-1)
      if (cd_Rec_cnt(stmt->cdp, stmt->cursnum, (uns32*)&stmt->row_cnt))
        stmt->row_cnt=0;
    if (stmt->impl_ird.array_status_ptr!=NULL) 
      for (stmt->num_in_rowset=0;  stmt->num_in_rowset<stmt->ard->array_size;  
           stmt->num_in_rowset++)
        stmt->impl_ird.array_status_ptr[stmt->num_in_rowset] = 
          stmt->curr_rowset_start+stmt->num_in_rowset < stmt->row_cnt ? 
            SQL_ROW_SUCCESS : SQL_ROW_NOROW;
    if (stmt->curr_rowset_start < stmt->row_cnt) any_row_ok=TRUE;
  }
  else  // retrieving data is ON
    for (stmt->num_in_rowset=0;  stmt->num_in_rowset<stmt->ard->array_size;  
         stmt->num_in_rowset++)
    { stmt->db_recnum=stmt->curr_rowset_start+stmt->num_in_rowset;
      if (!db_read(stmt, &any_row_ok))
        if (stmt->impl_ird.rows_procsd_ptr!=NULL) (*stmt->impl_ird.rows_procsd_ptr)++;
    }
  stmt->db_recnum=stmt->curr_rowset_start;  // I do not know if this is necessary
  stmt->async_run=FALSE;
  return any_row_ok ? SQL_SUCCESS : SQL_NO_DATA;
}

//      Returns data for bound columns in the current row ("hstmt->iCursor"),
//      advances the cursor.

SQLRETURN SQL_API SQLFetch(SQLHSTMT	StatementHandle)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (!stmt->has_result_set)
  { raise_err(stmt, SQL_24000);  /* Invalid cursor state */
    return SQL_ERROR;
  }
  if (stmt->ard->count > stmt->impl_ird.count)
  { raise_err(stmt, SQL_HY000);  /* Invalid column number */
    return SQL_ERROR;
  }
 // find the new start:
  if (stmt->curr_rowset_start==-1) stmt->curr_rowset_start=0;
  else stmt->curr_rowset_start+=stmt->ard->array_size;
 // fetch from stmt->curr_rowset_start, stmt->ard->array_size records.
  return general_fetch(stmt);
}

//      Returns result data for a single column in the current row.
SQLRETURN SQL_API SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
 SQLSMALLINT TargetType, SQLPOINTER TargetValuePtr,
 SQLLEN	BufferLength, SQLLEN * StrLen_or_IndPtr)
// Pri ziskavani dat po castech se v StrLen_or_IndPtr vraci velikost cele zbyle casti.
// Krome cteni posledni casti se muze vratit take SQL_NO_TOTAL
// Kdyz se vrati SQL_NO_TOTAL nebo vice, nez velikost bufferu (-1 pro char),
//  pak v buffer je plny dat.
// SQL_NO_DATA to vrati pri cteni po castech,kdyz je dalsi cast prazdna.
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (stmt->async_run || !stmt->has_result_set)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  } /* may be, on !has_result_set it should return state 24000 */
  if (stmt->curr_rowset_start==-1)
  { raise_err(stmt, SQL_24000);  /* Invalid cursor state */
    return SQL_ERROR;
  }
  if (ColumnNumber > stmt->impl_ird.count)
  { raise_err(stmt, SQL_07009);  /* Invalid column number */
    return SQL_ERROR;
  }
  if (TargetValuePtr == NULL)   // NOT OK, should return StrLen, no error for NULL I think
  { raise_err(stmt, SQL_01004);  
    return SQL_SUCCESS_WITH_INFO;
  }
  if (TargetType==SQL_ARD_TYPE)
  { if (ColumnNumber > stmt->ard->count)
    { raise_err(stmt, SQL_07009);  /* Invalid column number */
      return SQL_ERROR;
    }
    TargetType=stmt->ard->drecs.acc0(ColumnNumber)->concise_type;
  }
#ifdef STOP
  else if (TargetType==SQL_APD_TYPE)
  { if (ColumnNumber > stmt->apd->count)
    { raise_err(stmt, SQL_07009);  /* Invalid column number */
      return SQL_ERROR;
    }
    TargetType=stmt->apd->drecs.acc0(ColumnNumber)->concise_type;
  }
#endif

  if (!ColumnNumber)  /* reading the bookmark */
  { *(uns32*)TargetValuePtr=stmt->db_recnum;
    if (StrLen_or_IndPtr!=NULL) *StrLen_or_IndPtr=sizeof(uns32);
    return SQL_SUCCESS;
  }
  else   /* copy the value: */
  { if (stmt->preloaded_recnum==NORECNUM ||                            // nothing preloaded
        stmt->db_recnum< stmt->preloaded_recnum ||                     // preloaded values not usable
        stmt->db_recnum>=stmt->preloaded_recnum+stmt->preloaded_count) // preloaded values not usable
    { raise_err(stmt, SQL_HY010);  /* Invalid function sequence, probably impossible */
      return SQL_ERROR;
    }
    char * rowvals = stmt->rowbuf + stmt->rowbufsize * (stmt->db_recnum - stmt->preloaded_recnum);
    t_desc_rec * idrec = stmt->impl_ird.drecs.acc0(ColumnNumber);
    //t_desc_rec * adrec = stmt->    ard->drecs.acc (ColumnNumber);
   // check to see if continuing reading the same column:
    if (ColumnNumber!=stmt->get_data_column) stmt->get_data_offset=0;
    stmt->get_data_column=ColumnNumber;
    SQLRETURN result=convert_SQL_to_C(TargetType, idrec->concise_type, idrec->specif, rowvals+idrec->offset,
      (tptr)TargetValuePtr, BufferLength, StrLen_or_IndPtr, idrec->length, stmt, ColumnNumber, idrec->special);
    return result;
  }
}

//      -       -       -       -       -       -       -       -       -

//      This returns the number of rows associated with the database
//      attached to "hstmt".
SQLRETURN SQL_API SQLRowCount(SQLHSTMT StatementHandle, SQLLEN * RowCountPtr)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (stmt->sql_stmt_ind >= stmt->sql_stmt_cnt) // no row count available (after error or not generated)
    *RowCountPtr=-1;
  else
  { if (stmt->has_result_set) /* SELECT or catalog function result set */
    { if (stmt->row_cnt == -1) // asigned by open_result_set(), count "unknown"
      { stmt->async_run=TRUE;
        if (cd_Rec_cnt(stmt->cdp, stmt->cursnum, (uns32*)&stmt->row_cnt))
          stmt->row_cnt=0;
        stmt->async_run=FALSE;
      }
    }
    // else SQL statement other than SELECT: use statement result assigned by open_result_set
    *RowCountPtr=stmt->row_cnt; 
  }
  return SQL_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -

BOOL row_error(STMT * stmt)
{ if (stmt->impl_ird.array_status_ptr!=NULL)
    stmt->impl_ird.array_status_ptr[stmt->num_in_rowset]=SQL_ROW_ERROR;
  raise_db_err(stmt);
  if (stmt->is_bulk_position)
  { stmt->was_any_info=TRUE;
    if (++stmt->num_in_rowset >= stmt->ard->array_size)   /* end of bulk */
      return SQL_SUCCESS_WITH_INFO;
    stmt->curr_column=0;  /* to the next row */
    return FALSE;
  }
  else return TRUE;  /* return SQL_ERROR */
}

char auxbuf[MAX_FIXED_STRING_LENGTH+2]; /* used for writing strings (may be shorter than database size) */

BOOL write_db_data(STMT * stmt, tptr data, SDWORD len, bool from_put_data)
// [from_put_data] - when true, wide char data and ctype are converted to 8-bit.
//                 - when false, ctype may be converted but the data are not.
{ uns16 index = NOINDEX;  BOOL result=FALSE;
  t_desc_rec * idrec = stmt->impl_ird.drecs.acc(stmt->curr_column+1);
  t_desc_rec * adrec = stmt->    ard->drecs.acc(stmt->curr_column+1);
  int ctype = adrec->concise_type;
  int sqltype = idrec->concise_type;
#ifdef STOP  // used by 8-bit driver only
  if (!from_put_data)
    if (ctype == SQL_C_CHAR)
      if (sqltype==SQL_WCHAR || sqltype==SQL_WVARCHAR || sqltype==SQL_WLONGVARCHAR)
        ctype = SQL_C_WCHAR;  // ctype has been converted by DM, but data did not
#endif
  if (data==NULL) len=SQL_NULL_DATA;
  if (idrec->special)
#ifdef NOMULTI
    return FALSE;
#else
    index=0;
#endif
  if (len==SQL_NTS) len=(int)strlen(data);
  char * cdata;  bool c_buffer_allocated = false;
  if (!(ctype==SQL_C_BINARY ||  /* unless direct write */
        ctype==SQL_C_DEFAULT && IS_HEAP_TYPE(type_sql_to_WB(sqltype))))
  { //if (!IS_HEAP_TYPE(type_sql_to_WB(idrec->concise_type)) ||
    //    adrec->concise_type != SQL_C_CHAR && adrec->concise_type != SQL_C_WCHAR)
    // -- should convert char types too
    if (IS_HEAP_TYPE(type_sql_to_WB(sqltype)))
    { int space = 2*len+2;
      if (space < MAX_FIXED_STRING_LENGTH+2) space = MAX_FIXED_STRING_LENGTH+2;
      cdata=(char*)corealloc(space, 77);  c_buffer_allocated=true;
    }
    else
      cdata=auxbuf;
    if (convert_C_to_SQL(ctype, sqltype, idrec->specif, data, cdata, len, idrec->length, stmt))
    { result=row_error(stmt);
      goto ex;
    }
    if      (sqltype==SQL_LONGVARCHAR)  len=  (int)strlen(cdata);
    else if (sqltype==SQL_WLONGVARCHAR) len=2*(int)wuclen((wuchar*)cdata);
  }
  else  /* direct binary write: */
  { cdata=data;
    if (len==SQL_NULL_DATA) len=0;
    if (sqltype==SQL_VARCHAR)
    { if (len > idrec->length) len = (SDWORD)idrec->length;
      memcpy(auxbuf, data, (unsigned)len);  auxbuf[(unsigned)len]=0;
      cdata=auxbuf;
    }
  }
 // write cdata (len):
  if (IS_HEAP_TYPE(type_sql_to_WB(sqltype)))
  { if (len>0)  /* implies not NULL_DATA, cdata!=NULL */
    { if (cd_Write_var(stmt->cdp, stmt->cursnum, stmt->db_recnum, stmt->curr_column+1, index, stmt->curr_col_off, len, cdata))
        if (row_error(stmt)) { result=TRUE;  goto ex; }
    }
    else len=0;
    stmt->curr_col_off+=len;
    cd_Write_len(stmt->cdp, stmt->cursnum, stmt->db_recnum, stmt->curr_column+1, index, stmt->curr_col_off);
  }
  else
  { if (stmt->curr_col_off)
    { raise_err(stmt, SQL_22003);   /* Numeric value out of range */
      if (row_error(stmt)) { result=TRUE;  goto ex; }
    }
    else
      if (cd_Write_ind(stmt->cdp, stmt->cursnum, stmt->db_recnum, stmt->curr_column+1,
                       index, cdata ? cdata : NULLSTRING, (uns32)idrec->octet_length))
        if (row_error(stmt)) { result=TRUE;  goto ex; }
    stmt->curr_col_off=1;  /* prevents repeated write */
  }
 ex:
  if (c_buffer_allocated) corefree(cdata);
  return result;
}

RETCODE SetPos_steps(STMT * stmt, BOOL start)
/* Continues SetPos operation, breaks on the next AE column. Called by
   SetPos and ParamData.
- locks the row, if necessary;
- starts transaction, if AUTOCOMMIT;
- adds a new row, if ADD operation;
- writes all data (breaked by AE columns), goes to next row on error;
- commits, if AUTOCOMMIT;
- unlocks the row, if necessary. */
{ tptr data;  SDWORD mode;  
  int ctype;  SQLLEN bufsize;  SQLLEN * octet_len, * indicator;
  do
  {/* end of row */
    if (stmt->curr_column >= stmt->impl_ird.count)
    {
      if (stmt->my_dbc->autocommit)
        if (cd_Commit(stmt->cdp))
          if (row_error(stmt)) return SQL_ERROR;
     /* success in the row operation: */
      if (stmt->impl_ird.array_status_ptr!=NULL)
        stmt->impl_ird.array_status_ptr[stmt->num_in_rowset] =
          (stmt->fOption==SQL_ADD) ? SQL_ROW_ADDED : SQL_ROW_UPDATED;
      if (stmt->fLock==SQL_LOCK_UNLOCK)
        cd_Write_unlock_record(stmt->cdp, stmt->cursnum, stmt->db_recnum);
      if (stmt->is_bulk_position)
      { if (++stmt->num_in_rowset >= stmt->ard->array_size)   /* end of bulk */
          return stmt->was_any_info ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;
      }
      else /* no bulk */
        return stmt->was_any_info ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;
      stmt->curr_column=0;
    }
   /* starting a new row: */
    if (stmt->curr_column==0)
    {
      if (stmt->my_dbc->autocommit) cd_Start_transaction(stmt->cdp);
     /* find the database recnum: */
      if (stmt->fOption==SQL_ADD)
      { stmt->db_recnum=cd_Insert(stmt->cdp, stmt->cursnum);
        if (stmt->db_recnum==NORECNUM)
          if (row_error(stmt)) return SQL_ERROR;
      }
      else stmt->db_recnum=stmt->curr_rowset_start+stmt->num_in_rowset;
     /* locking: */
      if (stmt->fLock==SQL_LOCK_EXCLUSIVE)
        if (cd_Write_lock_record(stmt->cdp, stmt->cursnum, stmt->db_recnum))
          if (row_error(stmt)) return SQL_ERROR;
    }
   /* writing a column: */
    stmt->curr_col_off=0;
    if (stmt->ard->count >= stmt->curr_column+1)   /* must not call acc() otherwise */
    { t_desc_rec * idrec = stmt->impl_ird.drecs.acc0(stmt->curr_column+1);
      t_desc_rec * adrec = stmt->    ard->drecs.acc0(stmt->curr_column+1);
      get_bound_access(stmt, adrec, idrec, stmt->num_in_rowset, data, octet_len, indicator, ctype, bufsize);
      if (data!=NULL) // store the record values into bound columns locations: 
      { mode=octet_len ? (SDWORD)*octet_len : 0;
        if ((mode <= SQL_LEN_DATA_AT_EXEC_OFFSET) && (mode > -10000000L) ||
            (mode == SQL_DATA_AT_EXEC))  /* AE data */
        { stmt->AE_mode=start ? AE_COLUMNS_START : AE_COLUMNS;
          return SQL_NEED_DATA;
        }
        if (mode != SQL_IGNORE)  /* write to this column */
          if (write_db_data(stmt, data, mode, false)) return SQL_ERROR;
      } /* column bound */
    }
    stmt->curr_column++;
  } while (TRUE);
}

static void del_rec(STMT * stmt)
{ BOOL err=FALSE;
  stmt->db_recnum = stmt->curr_rowset_start + stmt->num_in_rowset;
  if (stmt->fLock==SQL_LOCK_UNLOCK)
    cd_Write_unlock_record(stmt->cdp, stmt->cursnum, stmt->db_recnum);
  else if (stmt->fLock==SQL_LOCK_EXCLUSIVE)
    err=cd_Write_lock_record(stmt->cdp, stmt->cursnum, stmt->db_recnum);
  if (!err) err=cd_Delete(stmt->cdp, stmt->cursnum, stmt->db_recnum);
  if (err)
  { if (stmt->impl_ird.array_status_ptr!=NULL)
      stmt->impl_ird.array_status_ptr[stmt->num_in_rowset]=SQL_ROW_ERROR;
    raise_db_err(stmt);
    stmt->was_any_info=TRUE;
  }
  else
    if (stmt->impl_ird.array_status_ptr!=NULL)
      stmt->impl_ird.array_status_ptr[stmt->num_in_rowset]=SQL_ROW_DELETED;
}

static void refr_rec(STMT * stmt)
{ BOOL err=FALSE;
  stmt->db_recnum = stmt->curr_rowset_start + stmt->num_in_rowset;
  if (stmt->fLock==SQL_LOCK_UNLOCK)
    cd_Write_unlock_record(stmt->cdp, stmt->cursnum, stmt->db_recnum);
  else if (stmt->fLock==SQL_LOCK_EXCLUSIVE)
    err=cd_Write_lock_record(stmt->cdp, stmt->cursnum, stmt->db_recnum);
  if (!err)
    err=db_read(stmt, NULL);
  if (err)
  { if (stmt->impl_ird.array_status_ptr!=NULL)
      stmt->impl_ird.array_status_ptr[stmt->num_in_rowset]=SQL_ROW_ERROR;
    raise_db_err(stmt);
    stmt->was_any_info=TRUE;
  }
  else
    if (stmt->impl_ird.array_status_ptr!=NULL)
      stmt->impl_ird.array_status_ptr[stmt->num_in_rowset]=SQL_ROW_SUCCESS;
}

void pos_rec(STMT * stmt)
{ stmt->db_recnum = stmt->curr_rowset_start + stmt->num_in_rowset;
  send_cursor_pos(stmt, stmt->db_recnum);
  if (stmt->fLock==SQL_LOCK_UNLOCK)
    cd_Write_unlock_record(stmt->cdp, stmt->cursnum, stmt->db_recnum);
  else if (stmt->fLock==SQL_LOCK_EXCLUSIVE)
    if (cd_Write_lock_record(stmt->cdp, stmt->cursnum, stmt->db_recnum))
    { raise_err(stmt, SQL_42000); /* Syntax error or access violation */
      stmt->was_any_info=TRUE;
    }
}

//      This positions the cursor within a block of data.

RETCODE SQL_API SQLSetPos(HSTMT hstmt, SQLSETPOSIROW irow, UWORD fOption, UWORD fLock)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;
  CLEAR_ERR(stmt);
#ifdef SEQ_CHECKING
  if (stmt->AE_mode)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
#endif
  if (fOption!=SQL_ADD)
  { if (irow > stmt->ard->array_size)
    { raise_err(stmt, SQL_HY107);  /* Row value out of range */
      return SQL_ERROR;
    }
    if (irow)
      stmt->db_recnum=stmt->curr_rowset_start+(trecnum)irow-1; /* 1-based to 0-based */
  }
  if (irow)
  { stmt->num_in_rowset=(int)irow-1;                    /* 1-based to 0-based */
    stmt->is_bulk_position=FALSE;
  }
  else
  { stmt->num_in_rowset=0;
    stmt->is_bulk_position=TRUE;
  }
 /* operating: */
  stmt->was_any_info=FALSE;  stmt->fLock=fLock;
  switch (fOption)
  { case SQL_REFRESH:
      stmt->async_run=TRUE;
      if (stmt->is_bulk_position)
        for (stmt->num_in_rowset=0;  stmt->num_in_rowset<stmt->ard->array_size;
             stmt->num_in_rowset++)
           refr_rec(stmt);
      else refr_rec(stmt);
      stmt->async_run=FALSE;
      return !stmt->was_any_info    ? SQL_SUCCESS :
             stmt->is_bulk_position ? SQL_SUCCESS_WITH_INFO : SQL_ERROR;
    case SQL_DELETE:
      stmt->get_data_column=(UWORD)-1;  /* no column can continue reading */
      stmt->async_run=TRUE;
      if (stmt->is_bulk_position)
        for (stmt->num_in_rowset=0;  stmt->num_in_rowset<stmt->ard->array_size;
             stmt->num_in_rowset++)
           del_rec(stmt);
      else del_rec(stmt);
      stmt->async_run=FALSE;
      return !stmt->was_any_info    ? SQL_SUCCESS :
             stmt->is_bulk_position ? SQL_SUCCESS_WITH_INFO : SQL_ERROR;
    case SQL_POSITION:   /* locking only */
      stmt->get_data_column=(UWORD)-1;  /* no column can continue reading */
      stmt->async_run=TRUE;
      if (stmt->is_bulk_position)
        for (stmt->num_in_rowset=0;  stmt->num_in_rowset<stmt->ard->array_size;
             stmt->num_in_rowset++)
           pos_rec(stmt);
      else pos_rec(stmt);
      stmt->async_run=FALSE;
      return !stmt->was_any_info    ? SQL_SUCCESS :
             stmt->is_bulk_position ? SQL_SUCCESS_WITH_INFO : SQL_ERROR;
    case SQL_UPDATE:
    case SQL_ADD:
      stmt->get_data_column=(UWORD)-1;  /* no column can continue reading */
      stmt->fOption=fOption;
      stmt->curr_column=0;
      stmt->async_run=TRUE;
      RETCODE res=SetPos_steps(stmt, TRUE);
      stmt->async_run=FALSE;
      return res;
  }
  return SQL_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -

//      This fetchs a block of data (rowset).

SQLRETURN SQL_API SQLExtendedFetch(SQLHSTMT StatementHandle, UWORD fFetchType, SQLLEN irow,
                                   SQLULEN *pcrow, UWORD *rgfRowStatus)
// Should be implemented for backward compatibility with 2.0 applications.
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
 // parameters to descriptors & statement attributes, save the original values:
  SQLULEN * procsd  = stmt->impl_ird.rows_procsd_ptr;
  stmt->impl_ird.rows_procsd_ptr  = pcrow;
  SQLUSMALLINT * rowstat = stmt->impl_ird.array_status_ptr;
  stmt->impl_ird.array_status_ptr = rgfRowStatus;
  void * bookmark;
  if (fFetchType==SQL_FETCH_BOOKMARK)
  { bookmark = stmt->bookmark_ptr;
    stmt->bookmark_ptr = &irow;
  }
 // map the call:
  SQLRETURN ret = SQLFetchScroll(StatementHandle, fFetchType, irow);
 // restore the original values in descriptors & statement attributes:
  if (fFetchType==SQL_FETCH_BOOKMARK)
    stmt->bookmark_ptr = bookmark;
  stmt->impl_ird.array_status_ptr = rowstat;
  stmt->impl_ird.rows_procsd_ptr  = procsd;
  return ret;
}

SQLRETURN SQL_API SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation,
                         SQLLEN FetchOffset)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (!stmt->has_result_set)
  { raise_err(stmt, SQL_24000);  /* Invalid cursor state */
    return SQL_ERROR;
  }
  if (stmt->ard->count > stmt->impl_ird.count)
  { raise_err(stmt, SQL_HY000);  /* Invalid column number */
    return SQL_ERROR;
  }

  sig32 pos;
  switch (FetchOrientation)
  { case SQL_FETCH_NEXT:
      if (stmt->curr_rowset_start < 0) pos=0;   /* same as FIRST */
      else pos=stmt->curr_rowset_start+stmt->prev_array_size;       
      break;
    case SQL_FETCH_PRIOR:
      if (stmt->curr_rowset_start <= 0) 
        { stmt->curr_rowset_start=-1;  return SQL_NO_DATA; }
      pos = stmt->curr_rowset_start-stmt->ard->array_size; // new array size!     
      if (pos<0) pos=0;  // aligned
      break;
    case SQL_FETCH_FIRST:
      pos=0;                                    break;
    case SQL_FETCH_LAST:
      if (stmt->row_cnt==-1)
        if (cd_Rec_cnt(stmt->cdp, stmt->cursnum, (uns32*)&stmt->row_cnt))
          { stmt->row_cnt=0;  return SQL_NO_DATA; }
      pos=stmt->row_cnt-stmt->ard->array_size;  break;
    case SQL_FETCH_BOOKMARK:
    { pos = *(sig32*)stmt->bookmark_ptr;
      if (pos<0)
      { raise_err(stmt, SQL_HY111);  /* Invalid bookmark value */
        return SQL_ERROR;
      }
      break;
    }
    case SQL_FETCH_ABSOLUTE:
      if (!FetchOffset) { stmt->curr_rowset_start=-1;  return SQL_NO_DATA; }
      if (FetchOffset < 0)
      { if (stmt->row_cnt==-1)
          if (cd_Rec_cnt(stmt->cdp, stmt->cursnum, (uns32*)&stmt->row_cnt))
            { stmt->row_cnt=0;  return SQL_NO_DATA; }
        pos=stmt->row_cnt+(sig32)FetchOffset;
        if (pos<0) 
          if (-FetchOffset <= stmt->ard->array_size) pos=0;
          else return SQL_NO_DATA;
      }
      else pos=(sig32)FetchOffset-1;              break;  /* 1-based to 0-based */
    case SQL_FETCH_RELATIVE:
     /* special cases: same as absolute (cannot be relative to a point outside records) */
      if ((FetchOffset> 0) && (stmt->curr_rowset_start < 0)) { pos=(sig32)FetchOffset-1;  break; }
      if (FetchOffset< 0)
      { if (stmt->row_cnt==-1)
          if (cd_Rec_cnt(stmt->cdp, stmt->cursnum, (uns32*)&stmt->row_cnt))
            { stmt->row_cnt=0;  return SQL_NO_DATA; }
        if (stmt->curr_rowset_start >= stmt->row_cnt)
          { pos=stmt->row_cnt+(sig32)FetchOffset;  break; }
      }
     /* normal case: */
      pos=stmt->curr_rowset_start+(sig32)FetchOffset;      break;
    default:
      raise_err(stmt, SQL_HY106);  /* Fetch type out of range */
      return SQL_ERROR;
  }
  stmt->prev_array_size=stmt->ard->array_size;
  if (pos < 0)
  { if (pos + (sig32)stmt->ard->array_size <= 0) return SQL_NO_DATA;
    pos=0;  /* return the 1st rowset if rowset overlaps with it */
  }
  stmt->curr_rowset_start=pos; /* This should be done even on error, I think */

  return general_fetch(stmt);
}

SQLRETURN	SQL_API	SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT	Operation)
{ return SQL_ERROR; }

//      This determines whether there are more results sets available for
//      the "hstmt".

SQLRETURN SQL_API SQLMoreResults(SQLHSTMT StatementHandle)
/* Implies closing the current result set */
// All bindings remain valid. Must skip statements not returning results
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }

  close_result_set(stmt);
 // find the next SELECT, INSERT, UPDATE or DELETE result:
  do
  { if (stmt->sql_stmt_ind+1 >= stmt->sql_stmt_cnt)
      return SQL_NO_DATA_FOUND; /* all result sets have already been processed */
    stmt->sql_stmt_ind++;
  } while (*(uns32*)stmt->result_sets.acc(stmt->sql_stmt_ind) == 0xffffffff);
 // create the new result set description:
#ifdef OLD_VERSION
  d_table * td;
  int descr_index = stmt->sql_stmt_ind % stmt->rs_per_iter;
  if (cd_SQL_get_result_set_info(stmt->cdp, stmt->prep_handle, descr_index, &td))
    return SQL_ERROR;
  define_result_set_from_td(stmt, td);  corefree(td);
#else
  uns32 result_info=*(uns32*)stmt->result_sets.acc(stmt->sql_stmt_ind);
  if (IS_CURSOR_NUM(result_info))   
  { tcursnum curs = (tcursnum)result_info;
    const d_table * td = cd_get_table_d(stmt->cdp, curs, CATEG_DIRCUR);
    define_result_set_from_td(stmt, td);
    if (td!=NULL) release_table_d(td);
  }
  else
    define_result_set_from_td(stmt, NULL);
#endif
  if (!open_result_set(stmt)) return SQL_ERROR;
  return SQL_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -

void close_result_set(STMT * stmt)
{ stmt->curr_rowset_start=-1;  /* not necessary, but to be safe... */
  if (stmt->has_result_set)
  { if (stmt->rowbuf) safefree((void**)&stmt->rowbuf);
    if (stmt->cursnum!=NOOBJECT)
      { cd_Close_cursor(stmt->cdp, stmt->cursnum);  stmt->cursnum=NOOBJECT; }
    stmt->has_result_set=FALSE;
  }
}

BOOL discard_other_result_sets(STMT * stmt)
/* Close the cursors in the result sets not opened yet. */
{ BOOL any_result_set_closed=FALSE;
  for (unsigned i=stmt->sql_stmt_ind+1;  i<stmt->sql_stmt_cnt;  i++)
  { uns32 result_info=*(uns32*)stmt->result_sets.acc(i);
    if (IS_CURSOR_NUM(result_info))   /* this is a cursor opened by SELECT */
    { cd_Close_cursor(stmt->cdp, (tcursnum)result_info);
      any_result_set_closed=TRUE;
    }
  }
  return any_result_set_closed;
}

BOOL open_result_set(STMT * stmt)   /* return FALSE on error */
/* Previous result set is supposed to be closed before */
{ uns32 result_info;
  result_info=*(uns32*)stmt->result_sets.acc(stmt->sql_stmt_ind);
  if (IS_CURSOR_NUM(result_info))   
  /* this is a cursor opened by SELECT */
  { stmt->cursnum=(tcursnum)result_info;
    stmt->get_data_column=(UWORD)-1;  /* no column can continue reading */
    stmt->db_recnum        =(trecnum)-1;    /* cursor not positioned yet, to be safe */
    stmt->curr_rowset_start=-1;    /* cursor not positioned yet */
    stmt->is_bulk_position =FALSE;
    if (!*stmt->cursor_name) assign_cursor_name(stmt);  /* cursor name (unless assigned before) */
    send_cursor_name(stmt);  // sends cursor_name & cursnum
    /* rowbufsize defined by the last call to define_result_set */
    if (stmt->rowbufsize)
    { 
#ifdef MULTILOAD
      int ext = stmt->rowbufsize * MAX_PACKAGED_REQS;
      if (ext>PACKET_SIZE_LIMIT) 
        ext = stmt->rowbufsize>PACKET_SIZE_LIMIT ? stmt->rowbufsize : PACKET_SIZE_LIMIT;
#else
      int ext = stmt->rowbufsize;
#endif      
      stmt->rowbuf=(tptr)corealloc(ext, 79);
      if (!stmt->rowbuf)
      { cd_Close_cursor(stmt->cdp, stmt->cursnum);  stmt->cursnum=NOOBJECT;
        raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
        return FALSE;
      }
    }
    else stmt->rowbuf=NULL;
    stmt->row_cnt =-1;   /* recnum unknown yet */
    stmt->has_result_set=TRUE;
  }
  else  /* non-SELECT sql statement */
  { stmt->row_cnt=result_info;  /* number of processed records */
    stmt->has_result_set=FALSE;
    stmt->rowbuf=NULL;       // not necessary
    stmt->cursnum=NOOBJECT;  // not necessary
  }
  return TRUE;
}


//      -       -       -       -       -       -       -       -       -

void raise_err(STMT * stmt, uns16 errnum)
{ if (stmt->err.errcnt < MAX_ERR_CNT)
    stmt->err.errnums[stmt->err.errcnt++]=errnum;
}
void raise_err_desc(DESC * desc, uns16 errnum)
{ if (desc->err.errcnt < MAX_ERR_CNT)
    desc->err.errnums[desc->err.errcnt++]=errnum;
}
void raise_err_dbc(DBC * dbc, uns16 errnum)
{ if (dbc->err.errcnt < MAX_ERR_CNT)
    dbc->err.errnums[dbc->err.errcnt++]=errnum;
}
void raise_err_env(ENV * env, uns16 errnum)
{ if (env->err.errcnt < MAX_ERR_CNT)
    env->err.errnums[env->err.errcnt++]=errnum;
}

void raise_db_err(STMT * stmt)
{ int err=cd_Sz_error(stmt->cdp);
  if (!err) raise_err(stmt, SQL_HY000);   /* General error */
  else raise_err(stmt, WB_ERROR_BASE+err);
}

void raise_db_err_dbc(DBC * dbc)
{ int err;
//  err=NO_ERROR
//  STMT * stmt=dbc->stmts;
//  if (stmt) do
//  { err=cd_Sz_error(stmt->cdp);
//    if (err != NO_ERROR) break;
//    stmt=stmt->next_stmt;
//  } while (stmt != dbc->stmts);
// statements may not exist now, but all statements share dbc->cdp!
  err=cd_Sz_error(dbc->cdp);
  raise_err_dbc(dbc, !err ? SQL_HY000 : WB_ERROR_BASE+err);
}

static char * SqlStateInfo[] = {
"",
"Disconnect error",
"Data truncated",
"Privilege not revoked",
"Invalid connection string attribute",
"COUNT field incorrect",
"Restricted data type attribute violation",
"Unable to connect to data source",
"Connection in use",
"Connection not open",
"Invalid transaction state",
"Invalid authorization specification",
"Memory allocation failure",
"Invalid argument value",
"Function sequence error",
"Invalid string or buffer length",
"Option type out of range",
"Invalid parameter number",
"Row value out of range",
"Program type out of range",
"SQL data type out of range",
"Invalid scale value",
"String data right truncation",
"Numeric value out of range",
"Invalid character value for cast specification",
"Datetime field overflow",
"Optional feature not implemented",
"Non-existing column bound",
"Invalid cursor state",
"Invalid use of null pointer",
"Invalid cursor name",
"Duplicate cursor name",
"No cursor name available",
"Concurrency option out of range",
"General error",
"Fetch type out of range",
"Invalid information type",
"Syntax error",
"Invalid transaction operation code",
"Driver does not support this function",
"Table type out of range",
"Invalid bookmark value",
"Error in row",
"Syntax error or access violation",
"Invalid cursor position",
"Option value changed",
"Nenalazeny knihovny 602SQL, neni nastavena cesta k nim",
"Nalezena jina verze 602SQL",
"V popisu zdroje dat chyb�jm�o aplikace, nutno doplnit",
"",
"",
"Memory allocation failure",
"Invalid descriptor index",
"Invalid descriptor field identifier",
"Prepared statement not a cursor specification",
"Function sequence error",
"Cannot modify an implementation row descriptor",
"Row value out of range",
"Invalid precision or scale value",
"Invalid string or buffer length",
"Invalid attribute/option identifier",
"Prompt disabled and no data source specified"
};
static char SqlStateName[][6] = {
"00000",
"01002",
"01004",
"01006",
"01S00",
"07002",
"07006",
"08001",
"08002",
"08003",
"25000",
"28000",
"S1001",
"S1009",
"S1010",
"S1090",
"S1092",
"S1093",
"S1107",
"S1003",
"S1004",
"S1094",
"22001",
"22003",
"22018",
"22008",
"HYC00",
"HY00X",
"24000",
"HY009",
"34000",
"3C000",
"S1015",
"S1108",
"HY000",
"HY106",
"HY096",
"37000",
"S1012",
"IM001",
"S1102",
"HY111",
"01S01",
"42000",
"HY109",
"01S02",
"W0001",
"W0002",
"W0003",
"",
"",
"HY001",
"07009",
"HY091",
"07005",
"HY010",
"HY016",
"HY107",
"HY104",
"HY090",
"HY092",
"IM007"
};


static char error_prefix[] = "[Software602][ODBC 3.0 driver for 602SQL " WB_VERSION_STRING "]";
//      Returns the next SQL error information.

static void CRLF_to_colon(tptr p)
{ for (;  *p;  p++)
    if (*p=='\r') *p=':';
    else if (*p=='\n') *p=' ';
}

SQLRETURN  SQL_API SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
           SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength)
{ err_t * err;  BOOL truncated=FALSE;  STMT * stmt;  cdp_t cdp = NULL;
  switch (HandleType) 
	{ case SQL_HANDLE_ENV:  err=&((ENV *)Handle)->err;  break;
	  case SQL_HANDLE_DBC:  err=&((DBC *)Handle)->err;  cdp=((DBC *)Handle)->cdp;  break;
	  case SQL_HANDLE_STMT: stmt=(STMT*)Handle;  err=&stmt->err;  cdp=stmt->cdp;  break;
    case SQL_HANDLE_DESC: err=&((DESC*)Handle)->err;  break;
  }
  switch (DiagIdentifier)
  { case SQL_DIAG_CURSOR_ROW_COUNT:      // DM tests HandleType==SQL_HANDLE_STMT
      if (DiagInfo!=NULL) *(SQLULEN*)DiagInfo = stmt->row_cnt;  break;
    case SQL_DIAG_DYNAMIC_FUNCTION:      // DM tests HandleType==SQL_HANDLE_STMT
      truncated=write_string_wide_b("", (SQLWCHAR*)DiagInfo, BufferLength, StringLength);
      break; // empty result when unknown
    case SQL_DIAG_DYNAMIC_FUNCTION_CODE: // DM tests HandleType==SQL_HANDLE_STMT
      if (DiagInfo!=NULL) *(uns32*)DiagInfo = SQL_DIAG_UNKNOWN_STATEMENT;
      break;
    case SQL_DIAG_NUMBER:
      if (DiagInfo!=NULL) *(uns32*)DiagInfo = err->errcnt;  break;
    case SQL_DIAG_RETURNCODE:
      if (DiagInfo!=NULL) *(uns32*)DiagInfo = err->return_code;  break;
    case SQL_DIAG_ROW_COUNT:             // DM tests HandleType==SQL_HANDLE_STMT
      if (DiagInfo!=NULL) *(SQLULEN*)DiagInfo = stmt->row_cnt;  break;
    default:  // record-level: RecNumber defined
    { if (RecNumber <= 0) return SQL_ERROR;
      if (RecNumber > err->errcnt) return SQL_NO_DATA;
      int errnum=err->errnums[RecNumber-1];
      switch (DiagIdentifier)
      { case SQL_DIAG_CLASS_ORIGIN:
          truncated=write_string_wide_b(errnum >= WB_ERROR_BASE ? "602SQL" : "ISO 9075", (SQLWCHAR*)DiagInfo, BufferLength, StringLength);  
          break;
        case SQL_DIAG_COLUMN_NUMBER:
          if (DiagInfo!=NULL) *(uns32*)DiagInfo = (uns32)SQL_NO_COLUMN_NUMBER;  
          break;
        case SQL_DIAG_CONNECTION_NAME:  // empty
          truncated=write_string_wide_b("", (SQLWCHAR*)DiagInfo, BufferLength, StringLength);  break;
        case SQL_DIAG_MESSAGE_TEXT:
        { char errbuf[400];  strcpy(errbuf, error_prefix);  tptr p=errbuf+strlen(error_prefix);
          if (errnum >= WB_ERROR_BASE)   /* WinBase specific error */
          { Get_error_num_text(cdp ? cdp : GetCurrTaskPtr(), errnum-WB_ERROR_BASE, p, (int)sizeof(errbuf)-(int)(p-errbuf));
            CRLF_to_colon(p);
          }
          else // ODBC sqlstate
            strcpy(p, SqlStateInfo[errnum]);
          truncated=write_string_wide_b(errbuf, (SQLWCHAR*)DiagInfo, BufferLength, StringLength);  break;
          break;
        }
        case SQL_DIAG_NATIVE:
          if (DiagInfo!=NULL)
            *(uns32*)DiagInfo = errnum >= WB_ERROR_BASE ? errnum - WB_ERROR_BASE : 0;
          break;
        case SQL_DIAG_ROW_NUMBER:
          if (DiagInfo!=NULL) *(SQLLEN*)DiagInfo = SQL_NO_ROW_NUMBER;  
          break;
        case SQL_DIAG_SERVER_NAME:
          truncated=write_string_wide_b("", (SQLWCHAR*)DiagInfo, BufferLength, StringLength);  break;
        case SQL_DIAG_SQLSTATE:
        { char SqlState[6];
          if (errnum >= WB_ERROR_BASE) sprintf(SqlState, "W%04u", errnum);
          else strcpy(SqlState, SqlStateName[errnum]);
          truncated=write_string_wide_b(SqlState, (SQLWCHAR*)DiagInfo, BufferLength, StringLength);  
          break;
        }
        case SQL_DIAG_SUBCLASS_ORIGIN:
          truncated=write_string_wide_b(
            errnum >= WB_ERROR_BASE ? "602SQL" : 
            SqlStateInfo[errnum][0]=='H' && SqlStateInfo[errnum][1]=='Y' ||
            SqlStateInfo[errnum][2]=='S' ? "ODBC 3.0" : "ISO 9075", 
            (SQLWCHAR*)DiagInfo, BufferLength, StringLength);  
          break;
      } // switch ()
    } // diagnostic record
  } // switch ()
  return truncated ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS; 
}

SQLRETURN  SQL_API SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT RecNumber, SQLWCHAR *Sqlstate,
           SQLINTEGER *NativeError, SQLWCHAR *MessageText,
           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{ err_t * err;  BOOL truncated=FALSE;  STMT * stmt;  cdp_t cdp = NULL;
  switch (HandleType) 
	{ case SQL_HANDLE_ENV:  err=&((ENV *)Handle)->err;  break;
	  case SQL_HANDLE_DBC:  err=&((DBC *)Handle)->err;  cdp=((DBC *)Handle)->cdp;  break;
	  case SQL_HANDLE_STMT: stmt=(STMT*)Handle;  err=&stmt->err;  cdp=stmt->cdp;  break;
    case SQL_HANDLE_DESC: err=&((DESC*)Handle)->err;  break;
  }
  if (RecNumber <= 0 || BufferLength<0) return SQL_ERROR;
  if (RecNumber > err->errcnt) return SQL_NO_DATA;
 // write the information:
  int errnum=err->errnums[RecNumber-1];  
  char errbuf[400];  char statebuf[5+1];  
  strcpy(errbuf, error_prefix);  tptr p=errbuf+strlen(error_prefix);
  if (errnum >= WB_ERROR_BASE)   /* WinBase specific error */
  { Get_error_num_text(cdp ? cdp : GetCurrTaskPtr(), errnum-WB_ERROR_BASE, p, (int)sizeof(errbuf)-(int)(p-errbuf));
    CRLF_to_colon(p);
    if (NativeError!=NULL) *NativeError = errnum-WB_ERROR_BASE;
    sprintf(statebuf, "W%04u", errnum-WB_ERROR_BASE);
  }
  else // ODBC sqlstate
  { strcpy(p, SqlStateInfo[errnum]);
    if (NativeError!=NULL) *NativeError =  0;
    strcpy(statebuf, SqlStateName[errnum]);
  }
  if (Sqlstate!=NULL) write_string_wide_b(statebuf, Sqlstate, 6*sizeof(SQLWCHAR), NULL);  
  truncated=write_string_wide_b(errbuf, MessageText, BufferLength*sizeof(SQLWCHAR), TextLength);  
  return truncated ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS; 
}

#ifdef STOP  // not in 3.0
RETCODE SQL_API SQLError(HENV lpenv, HDBC lpdbc, HSTMT hstmt,
   UCHAR  FAR *szSqlState, SDWORD FAR *pfNativeError,
   UCHAR  FAR *szErrorMsg, SWORD cbErrorMsgMax, SWORD FAR *pcbErrorMsg)
{ int errnum;  BOOL res;  char full_err[130], errbuf[100];
  tptr errtext;  SDWORD native_num;  STMT * stmt;

  if (hstmt == SQL_NULL_HSTMT)
  { if (lpdbc == SQL_NULL_HDBC)
      if (lpenv == SQL_NULL_HENV)
        errnum=0; /* invalid call */
      else  /* return env-level error */
      { ENV * env = (ENV*)lpenv;
        if (!IsValidHENV(lpenv)) return SQL_INVALID_HANDLE;
        if (!env->err.errcnt) errnum=0;
        else errnum=env->err.errnums[--env->err.errcnt];
      }
    else  /* return connection-level error */
    { DBC * dbc = (DBC*)lpdbc;
      if (!IsValidHDBC(lpdbc)) return SQL_INVALID_HANDLE;
      if (!dbc->err.errcnt) errnum=0;
      else errnum=dbc->err.errnums[--dbc->err.errcnt];
    }
  }
  else  /* return statement-level error */
  { stmt = (STMT*)hstmt;
    if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
    if (!stmt->err.errcnt) errnum=0;
    else errnum=stmt->err.errnums[--stmt->err.errcnt];
  }
 /* write error info: */
  if (szSqlState != NULL)   /* must write even if no error */
  { memcpy(szSqlState,
           SqlStateName[errnum >= WB_ERROR_BASE ? SQL_HY000 : errnum], 5);
    szSqlState[5]=0;
  }
  if (!errnum)  /* must write szErrorMsg, pcbErrorMsg */
  { if (pfNativeError != NULL) *pfNativeError=0;
    write_string("", (tptr)szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
    return SQL_NO_DATA_FOUND;
  }

  if (errnum >= WB_ERROR_BASE)   /* WinBase specific error */
  { native_num=errnum;
    client_error(GetCurrTaskPtr(), errnum-WB_ERROR_BASE);
//    Get_error_text(errbuf, sizeof(errbuf));  /* uses GetCurrTaskPtr()! */
// ??
    errbuf[0]=0;
    errtext=errbuf;
  }
  else
  { native_num=1;
    errtext=SqlStateInfo[errnum];
  }
  if (pfNativeError != NULL) *pfNativeError=native_num;
  strcpy(full_err, error_prefix);
  strcat(full_err, errtext);
  res=write_string(full_err, (tptr)szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
  return res ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;
}
#endif

