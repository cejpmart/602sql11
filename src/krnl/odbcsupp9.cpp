#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include "wprez9.h"
#include "flstr.h"

#ifdef LINUX
#define SQLExecDirectA SQLExecDirect
#define SQLDataSourcesA SQLDataSources
#define SQLDriverConnectA SQLDriverConnect
#define SQLGetInfoA SQLGetInfo
#define SQLErrorA SQLError
#endif

CFNC DllExport void WINAPI no_memory();

static BOOL grow_alloc(void ** ptr, unsigned oldsize, unsigned step)
{ void * p = corealloc(oldsize+step, 94);
  if (p==NULL) return FALSE;
  if (*ptr)
  { memcpy(p, *ptr, oldsize);
    corefree(*ptr);
  }
  *ptr=p;
  return TRUE;
}

CFNC DllKernel bool WINAPI odbc_error(t_connection * conn, HSTMT hStmt)
// Transforms error reported by the ODBC driver into a generic error in the ODBC connection.
// Error is immediately logged via the client_error_callback and is made available for the GUI reporting.
{ SDWORD native;  RETCODE retcode;  bool any_error_found=false;
  unsigned char text[SQL_MAX_MESSAGE_LENGTH+1], state[SQL_SQLSTATE_SIZE+1];
  do
  { if (hStmt)
      retcode=SQLError(SQL_NULL_HENV, SQL_NULL_HDBC, hStmt,       state, &native, text, sizeof(text), NULL);
    else
      retcode=SQLError(SQL_NULL_HENV, conn->hDbc, SQL_NULL_HSTMT, state, &native, text, sizeof(text), NULL);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) break;  // no (more) errors
    client_error_generic(conn, GENERR_ODBC_DRIVER, (char*)text);
    any_error_found=true;
  } while (true);
  return any_error_found;
}

CFNC DllKernel void WINAPI pass_error_info(t_connection * conn, cdp_t cdp)
// Moves error information from conn to cdp.
{
  if (cdp->generic_error_message) corefree(cdp->generic_error_message);
  cdp->generic_error_message = conn->generic_error_message;
  conn->generic_error_message = NULL;
  if (cdp->generic_error_messageW) corefree(cdp->generic_error_messageW);
  cdp->generic_error_messageW = conn->generic_error_messageW;
  conn->generic_error_messageW = NULL;
  cdp->RV.report_number = GENERR_ODBC_DRIVER;
}

#if 0
CFNC DllKernel void WINAPI log_odbc_stmt_error(HSTMT hStmt, cdp_t cdp)
{ SDWORD native;  RETCODE retcode;  
  unsigned char text[SQL_MAX_MESSAGE_LENGTH+1], state[SQL_SQLSTATE_SIZE+1];
  do
  { retcode=SQLError(SQL_NULL_HENV, SQL_NULL_HDBC, hStmt, state, &native, text, sizeof(text), NULL);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) break;
#ifdef STOP
    if (client_error_callback)
  	  (*client_error_callback)((char*)text);
#else  // replaced by a version which allows to dispay the error in a box besides adding the messages to the log
    client_error_generic(cdp, GENERR_ODBC_DRIVER, (char*)text);
#endif
  } while (true);
}

CFNC DllKernel void WINAPI log_odbc_dbc_error(t_connection * conn)
{ SDWORD native;  RETCODE retcode;
  unsigned char text[SQL_MAX_MESSAGE_LENGTH+1], state[SQL_SQLSTATE_SIZE+1];
  do
  { retcode=SQLError(SQL_NULL_HENV, conn->hDbc, SQL_NULL_HSTMT, state, &native, text, sizeof(text), NULL);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) break;
    if (client_error_callback)
  	  (*client_error_callback)((char*)text);
  } while (true);
}
#endif

CFNC DllKernel d_table * WINAPI make_odbc_descriptor9(t_connection * conn, const char * tabname, const char * prefix, BOOL WB_types)
{ 
#ifdef CLIENT_ODBC_9
  bool use_qualif, use_schema;
  if (conn->special_flags & 2)
  { use_qualif=false;  use_schema=true;
  }
  else
  { use_qualif=conn->qualifier_usage && !conn->owner_usage;  
    use_schema=conn->owner_usage!=0;
  }
  RETCODE retcode;  d_table * td;  int i;   
  bool err=false;
  retcode=SQLColumns(conn->hStmt, use_qualif ? (UCHAR*)prefix : NULL, SQL_NTS, use_schema ? (UCHAR*)prefix : NULL, SQL_NTS, 
                     (UCHAR*)tabname, SQL_NTS, (UCHAR*)"%", SQL_NTS);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) return NULL;
  td=(d_table*)corealloc(sizeof(d_table), 49);
  if (td==NULL) err=true;
  else
  { char colname[128+1], type_name[128+1];
    SWORD type, scale, nullable;  SDWORD precision, length;
    SQLLEN colname_size, type_size, type_name_size, precision_size,
           length_size, scale_size, nullable_size;
    // Access 7.0 requires ?_size indicator to be used for each column which ca have NULL data!
    td->attrcnt=1;  td->tabcnt=1;  td->updatable=QE_IS_DELETABLE | QE_IS_INSERTABLE | QE_IS_UPDATABLE;  td->tabdef_flags=0;
    strmaxcpy(td->selfname, tabname, sizeof(td->selfname));
    td->schemaname[0]=0;
    strcpy(td->attribs[0].name, NULLSTRING);
    td->attribs[0].type=ATT_BOOLEAN;  td->attribs[0].multi=1;
    retcode=SQLBindCol(conn->hStmt, 4, SQL_C_CHAR, colname, sizeof(colname), &colname_size);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) err=true;
    retcode=SQLBindCol(conn->hStmt, 5, SQL_C_SHORT, &type,  sizeof(type), &type_size);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) err=true;
    retcode=SQLBindCol(conn->hStmt, 6, SQL_C_CHAR, type_name, sizeof(type_name), &type_name_size);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) err=true;
    retcode=SQLBindCol(conn->hStmt, 7, SQL_C_LONG,  &precision, sizeof(precision), &precision_size);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) err=true;
    retcode=SQLBindCol(conn->hStmt, 8, SQL_C_LONG,  &length,    sizeof(length), &length_size);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) err=true;
    retcode=SQLBindCol(conn->hStmt, 9, SQL_C_SHORT, &scale, sizeof(scale), &scale_size);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) err=true;
    retcode=SQLBindCol(conn->hStmt,11, SQL_C_SHORT, &nullable, sizeof(nullable), &nullable_size);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) err=true;
    unsigned td_oldsize=sizeof(d_table);
    do
    { scale=0;   /* SQLFetch may not null this */
      retcode=SQLFetch(conn->hStmt);
      if (retcode==SQL_NO_DATA) break;
      if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
        { err=true;  break; }
      if (colname_size==SQL_NULL_DATA ||
          type_size   ==SQL_NULL_DATA) break;  /* Impossible, I thing */
      if (colname_size >= 0) colname[(unsigned)colname_size]=0;
      if (type_name_size >= 0) type_name[(unsigned)type_name_size]=0;
      else *type_name=0;
     /* reallocate td and add original column name: */
      if (!grow_alloc((void**)&td, td_oldsize, sizeof(d_attr)+(unsigned)colname_size+1))
        { safefree((void**)&td);  err=true;  break; }
      unsigned basesize=sizeof(d_table) + (td->attrcnt-1) * sizeof(d_attr);
      memmov((tptr)td+basesize+sizeof(d_attr), (tptr)td+basesize, td_oldsize-basesize);
      strcpy((tptr)td+td_oldsize+sizeof(d_attr), colname);
      td_oldsize+=sizeof(d_attr)+(unsigned)colname_size+1;
     /* fill d_attr: */
      d_attr * att = &td->attribs[td->attrcnt++];
      if (precision_size==SQL_NULL_DATA) precision=0;
      else if (precision > 255) precision = 255;
      if (scale_size==SQL_NULL_DATA) scale=0;
      else if (scale > 255) scale = 255;
      if (length_size==SQL_NULL_DATA) length=1;
      strmaxcpy(att->name, colname, sizeof(att->name));
      Upcase(att->name);
      for (i=0;  att->name[i];  i++)
//         if (!SYMCHAR(att->name[i])) att->name[i]='_';  // not necessary any more
         if (att->name[i]=='`') att->name[i]='_';
      if (WB_types)
      { att->specif.set_num(scale, precision); /* will be overwitten for some types */
        /* used to recognize SQL_TINYINT, SQL_REAL */
        switch (type)
        { case SQL_BIT:      att->type=ATT_BOOLEAN;  break;
          case SQL_CHAR:  case SQL_VARCHAR:
            if (precision<=0) precision=length;
            if (precision==0) { att->type=ATT_STRING;  att->specif.length=MAX_FIXED_STRING_LENGTH; }  // Access 97 returns 0 - probably for function results
            else
            { att->specif.length = (uns16)precision;
              att->type = precision==1 ? ATT_CHAR : ATT_STRING;
#ifdef WIDE_ODBC
              att->specif.length = (uns16)(2*att->specif.length);
            }
            att->specif.wide_char=1;
#else
            }
#endif
            break;
          case SQL_WCHAR:  case SQL_WVARCHAR:  // MS SQL, precision is number of characters
            if (precision<=0) precision=length;
            if (precision==0) { att->type=ATT_STRING;  att->specif.length=MAX_FIXED_STRING_LENGTH; }  // Access 97 returns 0 - probably for function results
            else 
            { att->specif.length = (uns16)(2*precision);
              att->type = precision==1 ? ATT_CHAR : ATT_STRING;
            }
            att->specif.wide_char=1;
            break;
          case SQL_BINARY:
            att->type = (uns8)(length>MAX_FIXED_STRING_LENGTH ? ATT_NOSPEC : ATT_BINARY);
            att->specif.length=(uns16)length;
            break;
          case SQL_TINYINT:
          case SQL_SMALLINT:      att->type=ATT_INT16;  break;
          case SQL_INTEGER:       att->type=ATT_INT32;  break;
          case SQL_BIGINT:        att->type=ATT_INT64;  break;
          case SQL_NUMERIC:       att->type=ATT_ODBC_NUMERIC;  break;
          case SQL_DECIMAL:       att->type=ATT_ODBC_DECIMAL;  break;
          case SQL_REAL:
          case SQL_FLOAT: /* no back_conversion for this type */
            att->specif.set_num(0, 9);  att->type=ATT_FLOAT;  break;
          case SQL_DOUBLE:        
            att->specif.set_num(0,18);  att->type=ATT_FLOAT;  break;
          case SQL_DATE:       case SQL_TYPE_DATE:        
                                  att->type=ATT_ODBC_DATE;  break;
          case SQL_TIME:       case SQL_TYPE_TIME:        
                                  att->type=ATT_ODBC_TIME;  break;
          case SQL_TIMESTAMP:  case SQL_TYPE_TIMESTAMP:           
                                  att->type=ATT_ODBC_TIMESTAMP; break;
          case SQL_LONGVARCHAR:
            att->type=ATT_TEXT;   att->specif.length=(uns16)length;  
#ifdef WIDE_ODBC
            att->specif.wide_char=1;
#endif
            break;
          case SQL_WLONGVARCHAR:
            att->type=ATT_TEXT;   att->specif.length=(uns16)length;  att->specif.wide_char=1;  break;
          case SQL_LONGVARBINARY:  case SQL_VARBINARY:
            att->type=ATT_NOSPEC; att->specif.length=(uns16)length;  break;
          default:    att->type=0;  break;
        }
      }
      else
      { att->type=1;
        while (stricmp(type_name, conn->ti[att->type-1].strngs))
          if (conn->ti[att->type].strngs == NULL) break;
          else att->type++;
        if (type==SQL_NUMERIC || type==SQL_DECIMAL)
          att->specif.set_num(scale, precision);
        else if ((type==SQL_CHAR || type==SQL_VARCHAR) && length==2*precision)  // Access 97 driver returns double length
          att->specif.length=(uns16)precision;
        else att->specif.length=(uns16)length;
      }
      att->nullable=nullable_size==SQL_NULL_DATA || nullable!=SQL_NO_NULLS;
      att->multi=1;
      att->needs_prefix=wbfalse;
      att->a_flags=0;
    } while (true);
    SQLFreeStmt(conn->hStmt, SQL_UNBIND);
  }
  SQLFreeStmt(conn->hStmt, SQL_CLOSE);

  if (err) { corefree(td);  return NULL; }

  if (td!=NULL)
  { char colname[128+1], ri_name[128+1];  int i, flag;
   /* find special attributes identifying the record: */
    BOOL any_rowid=FALSE;
    retcode=SQLSpecialColumns(conn->hStmt, SQL_BEST_ROWID, use_qualif ? (UCHAR*)prefix : NULL, SQL_NTS, use_schema ? (UCHAR*)prefix : NULL, SQL_NTS,
        (UCHAR*)tabname, SQL_NTS, SQL_SCOPE_CURROW, SQL_NULLABLE);
    if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
    { SQLBindCol(conn->hStmt, 2, SQL_C_CHAR, colname, sizeof(colname), NULL);
      do
      { retcode=SQLFetch(conn->hStmt);
        if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
          break;
       /* find and mark the atribute: */
        for (i=1;  i<td->attrcnt; i++)
          if (!my_stricmp(colname, td->attribs[i].name))
          { td->attribs[i].a_flags|=ROWID_FLAG;
            any_rowid=TRUE;
            break;
          }
      } while (TRUE);
      SQLFreeStmt(conn->hStmt, SQL_UNBIND);
    }
    SQLFreeStmt(conn->hStmt, SQL_CLOSE);
    if (!any_rowid)
      for (i=1;  i<td->attrcnt; i++)
        if (!IS_HEAP_TYPE(td->attribs[i].type))
          td->attribs[i].a_flags|=ROWID_FLAG;

   /* find special attributes containing record version: */
    retcode=SQLSpecialColumns(conn->hStmt, SQL_ROWVER, use_qualif ? (UCHAR*)prefix : NULL, SQL_NTS, use_schema ? (UCHAR*)prefix : NULL, SQL_NTS, 
        (UCHAR*)tabname, SQL_NTS, SQL_SCOPE_CURROW, SQL_NULLABLE);
    if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
    { SQLBindCol(conn->hStmt, 2, SQL_C_CHAR, colname, sizeof(colname), NULL);
      do
      { retcode=SQLFetch(conn->hStmt);
        if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
          break;
       /* find and mark the atribute: */
        for (i=1;  i<td->attrcnt; i++)
          if (!my_stricmp(colname, td->attribs[i].name))
          { td->attribs[i].a_flags|=ROWVER_FLAG;
            break;
          }
      } while (TRUE);
      SQLFreeStmt(conn->hStmt, SQL_UNBIND);
    }
//    else /* create full identification */
//    { for (i=1;  i<td->attrcnt; i++)
//        if (!IS_HEAP_TYPE(td->attribs[i].type))
//          td->attribs[i].a_flags|=ROWVER_FLAG;
//    }
    SQLFreeStmt(conn->hStmt, SQL_CLOSE);

   /* find column privileges: */
    if (conn->flags & CONN_FL_DISABLE_COLUMN_PRIVILS) retcode=SQL_ERROR;
    else retcode=SQLColumnPrivileges(conn->hStmt, use_qualif ? (UCHAR*)prefix : NULL, SQL_NTS, use_schema ? (UCHAR*)prefix : NULL, SQL_NTS, 
        (UCHAR*)tabname, SQL_NTS, (UCHAR*)"%", SQL_NTS);
    if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
    { SQLBindCol(conn->hStmt, 4, SQL_C_CHAR, colname, sizeof(colname), NULL);
      SQLBindCol(conn->hStmt, 7, SQL_C_CHAR, ri_name, sizeof(ri_name), NULL);
      do
      { retcode=SQLFetch(conn->hStmt);
        if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
          break;
        if (!stricmp(ri_name, "SELECT"))     flag=RI_SELECT_FLAG; else
        if (!stricmp(ri_name, "INSERT"))     flag=RI_INSERT_FLAG; else
        if (!stricmp(ri_name, "UPDATE"))     flag=RI_UPDATE_FLAG|RI_SELECT_FLAG; else  // Oracle 8 declares only UPDATE 
        if (!stricmp(ri_name, "REFERENCES")) flag=RI_REFERS_FLAG; else
          continue;   /* other (ds dependent) privileges ignored */
       /* find and mark the atribute: */
        for (i=1;  i<td->attrcnt; i++)
          if (!my_stricmp(colname, td->attribs[i].name))
          { td->attribs[i].a_flags|=flag;
            break;
          }
      } while (TRUE);
      SQLFreeStmt(conn->hStmt, SQL_UNBIND);
    }
    else  /* assume maximal rights: */
      for (i=1;  i<td->attrcnt; i++)
        td->attribs[i].a_flags |=
          RI_SELECT_FLAG | RI_INSERT_FLAG | RI_UPDATE_FLAG | RI_REFERS_FLAG;
    SQLFreeStmt(conn->hStmt, SQL_CLOSE);
  }
  return td;
#else
  return NULL;
#endif
}

CFNC DllExport d_table * WINAPI make_result_descriptor9(t_connection * conn, HSTMT hStmt)
{ RETCODE retcode;  d_table * td;  int i;  SWORD pccol;
  retcode=SQLNumResultCols(hStmt, &pccol);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    { odbc_error(conn, hStmt);  return NULL; }
  td=(d_table*)corealloc(sizeof(d_table), 49);
  if (td==NULL)  { no_memory();  return NULL; }
  char colname[128+1];  SWORD colname_size;
  SWORD type, scale, nullable;  SQLULEN precision;
  td->attrcnt=1;  td->tabcnt=1;  td->updatable=QE_IS_DELETABLE | QE_IS_INSERTABLE;  td->tabdef_flags=0;
  td->schemaname[0]=0;  td->selfname[0]=0;
  strcpy(td->attribs[0].name, NULLSTRING);
  td->attribs[0].type=ATT_BOOLEAN;  td->attribs[0].multi=1;

  unsigned td_oldsize=sizeof(d_table);
  while (td->attrcnt <= pccol)
  { retcode=SQLDescribeCol(hStmt, td->attrcnt, (UCHAR*)colname, sizeof(colname),
      &colname_size, &type, &precision, &scale, &nullable);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
      { odbc_error(conn, hStmt);  break; }
    if (colname_size==SQL_NULL_DATA) break;  /* Impossible, I hope */
    if (colname_size>=sizeof(colname)) colname_size=sizeof(colname)-1;
    if (colname_size>=0) colname[(unsigned)colname_size]=0;
//    if (type_name_size >= 0) type_name[(unsigned)type_name_size]=0;
   /* reallocate td and add original column name: */
    if (!grow_alloc((void**)&td, td_oldsize, sizeof(d_attr)+(unsigned)colname_size+1))
      { no_memory();  safefree((void**)&td);  break; }
    unsigned basesize=sizeof(d_table) + (td->attrcnt-1) * sizeof(d_attr);
    memmov((tptr)td+basesize+sizeof(d_attr), (tptr)td+basesize, td_oldsize-basesize);
    strcpy((tptr)td+td_oldsize+sizeof(d_attr), colname);
    td_oldsize+=sizeof(d_attr)+(unsigned)colname_size+1;
   /* fill d_attr: */
    d_attr * att = &td->attribs[td->attrcnt];
    if (scale     > 255) scale = 255;
    strmaxcpy(att->name, colname, sizeof(att->name));
    //Upcase9(ltab->cdp, att->name);
    for (i=0;  att->name[i];  i++)
//    if (!SYMCHAR(att->name[i])) att->name[i]='_';  // not necessary any more
      if (att->name[i]=='`') att->name[i]='_';
    att->specif.set_num(scale, (int)precision);   /* will be overwitten for some types */
    /* used to recognize SQL_TINYINT, SQL_REAL */
    switch (type)
    { case SQL_BIT:      att->type=ATT_BOOLEAN;  break;
      case SQL_WCHAR:  case SQL_WVARCHAR:  // MS SQL
        att->specif.wide_char=1;  
        if (precision==1)   att->type=ATT_CHAR;
        else                att->type=ATT_STRING;  
        att->specif.length=(uns16)(2*precision); 
        break;
      case SQL_CHAR:  case SQL_VARCHAR:
#ifdef WIDE_ODBC
        att->specif.wide_char=1;  
        att->specif.length=(uns16)(2*precision); 
#else
        att->specif.length=(uns16)precision; 
#endif
        if (precision==1)   att->type=ATT_CHAR;
        else                att->type=ATT_STRING;  
        break;
      case SQL_BINARY:
        att->type = precision>MAX_FIXED_STRING_LENGTH ? ATT_NOSPEC : ATT_BINARY;
        att->specif.length=(uns16)precision;
        break;
      case SQL_TINYINT:       att->type=ATT_INT8;   break;
      case SQL_SMALLINT:      att->type=ATT_INT16;  break;
      case SQL_INTEGER:       att->type=ATT_INT32;  break;
      case SQL_BIGINT:        att->type=ATT_INT64;  break;
      case SQL_NUMERIC:       att->type=ATT_ODBC_NUMERIC;  break;
      case SQL_DECIMAL:       att->type=ATT_ODBC_DECIMAL;  break;
      case SQL_REAL:
      case SQL_FLOAT:  /* no back_conversion for this type */
      case SQL_DOUBLE:        att->type=ATT_FLOAT;  break;
      case SQL_DATE:       case SQL_TYPE_DATE:        
                              att->type=ATT_ODBC_DATE;  break;
      case SQL_TIME:       case SQL_TYPE_TIME:
                              att->type=ATT_ODBC_TIME;  break;
      case SQL_TIMESTAMP:  case SQL_TYPE_TIMESTAMP:   
                              att->type=ATT_ODBC_TIMESTAMP; break;
      case SQL_LONGVARCHAR:
        att->type=ATT_TEXT;   att->specif.length=(uns16)precision;  
#ifdef WIDE_ODBC
        att->specif.wide_char=1;
#endif
        break;
      case SQL_WLONGVARCHAR:
        att->type=ATT_TEXT;   att->specif.length=(uns16)precision;  att->specif.wide_char=1;  break;
      case SQL_LONGVARBINARY:  
        att->type=ATT_NOSPEC; att->specif.length=(uns16)precision;  break;
      case SQL_VARBINARY:
        att->type=ATT_RASTER; att->specif.length=(uns16)precision;  break;
      default:   /* SQL_BIGINT */ att->type=0;  break;
    }
    att->nullable=nullable!=SQL_NO_NULLS;
    att->multi=1;  att->needs_prefix=wbfalse;  att->a_flags=0;

    SQLLEN info;
    retcode=SQLColAttributes(hStmt, td->attrcnt, SQL_COLUMN_UPDATABLE, NULL, 0, NULL, &info);
    if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
      if (info!=SQL_ATTR_READONLY)
        td->updatable|=QE_IS_UPDATABLE;  /* an attribute than can be readable */
    td->attrcnt++;
  }

  for (i=1;  i<td->attrcnt; i++)
  { if (!IS_HEAP_TYPE(td->attribs[i].type))
      td->attribs[i].a_flags|=ROWVER_FLAG;
    td->attribs[i].a_flags |=
      RI_SELECT_FLAG | RI_INSERT_FLAG | RI_UPDATE_FLAG | RI_REFERS_FLAG;
  }
  return td;
}

CFNC DllExport ltable * WINAPI create_cached_access_for_stmt(t_connection * conn, HSTMT hStmt, const d_table * td)
// Takes ownership of [hStmt] and [td].
{ ltable * ltab = new ltable(NULL, conn);
  if (ltab==NULL) { release_table_d(td);  return NULL; }
  ltab->hStmt = hStmt;
  if (ltab->describe(td, true))  // ltab will co-own the [td]
    if (alloc_data_cache(ltab, 1))
    { ltab->cursnum=(tcursnum)-3;  // signal for views: ODBC (different from NOOBJECT)
      return ltab;
    }
  delete ltab;
  return NULL;
}

CFNC DllExport RETCODE WINAPI create_odbc_statement(t_connection *	conn, HSTMT * hStmt)
{ RETCODE retcode, retcode2;
  retcode = SQLAllocStmt(conn->hDbc, hStmt);
  if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
  { uns32 opt;
         if (conn->scroll_options & SQL_SO_KEYSET_DRIVEN) opt=SQL_CURSOR_KEYSET_DRIVEN;
    else if (conn->scroll_options & SQL_SO_MIXED)         opt=SQL_CURSOR_KEYSET_DRIVEN;
    else if (conn->scroll_options & SQL_SO_DYNAMIC)       opt=SQL_CURSOR_DYNAMIC;
    else if (conn->scroll_options & SQL_SO_STATIC)        opt=SQL_CURSOR_STATIC;
    else if (conn->scroll_options & SQL_SO_FORWARD_ONLY)  opt=SQL_CURSOR_FORWARD_ONLY;
    else opt=12345;
    if (opt==12345) retcode=SQL_SUCCESS;
    else retcode2=SQLSetStmtOption(*hStmt, SQL_CURSOR_TYPE, opt);
    if (retcode2==SQL_SUCCESS || retcode2==SQL_SUCCESS_WITH_INFO || !conn->absolute_fetch)
    {      if (conn->scroll_concur & SQL_SCCO_OPT_VALUES) opt=SQL_CONCUR_VALUES;
      else if (conn->scroll_concur & SQL_SCCO_OPT_ROWVER) opt=SQL_CONCUR_ROWVER;
      else if (conn->scroll_concur & SQL_SCCO_LOCK)       opt=SQL_CONCUR_LOCK;
      else opt=SQL_CONCUR_READ_ONLY;
      retcode2=SQLSetStmtOption(*hStmt, SQL_ATTR_CONCURRENCY, opt);
      if (retcode2==SQL_SUCCESS || retcode2==SQL_SUCCESS_WITH_INFO || !conn->absolute_fetch)
      { retcode2=SQLSetStmtAttr(*hStmt, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER)SQL_SCROLLABLE, SQL_IS_INTEGER);  // MS SQL 7.0 cursor are not scrollable without this
        // if (retcode2!=SQL_SUCCESS) odbc_error(conn, *hStmt);  -- may not be implemented and may return error
      }
    }

  }
  return retcode;
}

CFNC DllExport ltable * WINAPI create_cached_access9(cdp_t cdp, t_connection *	conn, const char * source, const char * tablename, const char * odbc_prefix, tcursnum cursnum)
// Creates the initial cache with size 1. For ODBC, create the statement.
{
  ltable * ltab = new ltable(cdp, conn);
  if (ltab==NULL) return NULL;
#ifdef CLIENT_ODBC_9
  if (conn!=NULL)   /* ODBC access: */
  { RETCODE retcode = create_odbc_statement(conn, &ltab->hStmt);
    if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
    {/* open ODBC cursor: */
      if (source)
        ltab->select_st=(char*)corealloc(strlen(source)+1, 87);  // will be owned by ltab
      else
      { ltab->fulltablename=(char*)corealloc(strlen(tablename)+strlen(odbc_prefix)+6, 87);  // will be owned by ltab
        ltab->select_st=(char*)corealloc(strlen(tablename)+strlen(odbc_prefix)+25+30, 87);  // will be owned by ltab
      }
      if (ltab->select_st!=NULL && (source || ltab->fulltablename!=NULL))
      { if (source)
          strcpy(ltab->select_st, source);
        else 
        { { t_flstr fullname;
            make_full_table_name(conn, fullname, tablename, odbc_prefix);
            strcpy(ltab->fulltablename, fullname.get());
          }
          sprintf(ltab->select_st, "SELECT * FROM %s", ltab->fulltablename);
#if 0
         // without columns it is stupid
          if (conn->sql_conform==SQL_OSC_EXTENDED)
          { char dbms_name[50];
            SQLGetInfo(conn->hDbc, SQL_DBMS_NAME, dbms_name, sizeof(dbms_name), NULL);
            if (strnicmp(dbms_name, "Sybase", 6))
              strcat(ltab->select_st, " FOR UPDATE OF");
          }
#endif
        }

        retcode=SQLExecDirect(ltab->hStmt, (UCHAR*)ltab->select_st, SQL_NTS);
        if (retcode!=SQL_SUCCESS) odbc_error(conn, ltab->hStmt);  // MS SQL reportsts changing the cursor type
        if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
        { const d_table * td = make_odbc_descriptor9(conn, tablename, odbc_prefix, true);
          SQLLEN opt = 0;  // MS Native Driver error: 64-bit version stores 8 bytes for SQL_ATTR_CONCURRENCY!
          if (SQL_SUCCEEDED(SQLGetStmtAttr(ltab->hStmt, SQL_ATTR_CONCURRENCY, &opt, SQL_IS_UINTEGER, NULL)) 
              && opt==SQL_CONCUR_READ_ONLY && !(conn->special_flags & 1))
            ((d_table *)td)->updatable=0;  // changed by the SQLExecDirect, may be masked by (special_flags & 1)
          if (ltab->describe(td, true))  // ltab will co-own the [td]
            if (alloc_data_cache(ltab, 1))
            { ltab->cursnum=(tcursnum)-3;  // signal for views: ODBC (different from NOOBJECT)
              return ltab;
            }
        }
      }
    }
    else odbc_error(conn, 0);
  }
  else  // conn==NULL
#endif // CLIENT_ODBC_9
  // 602SQL access:
  { const d_table * td = cd_get_table_d(cdp, cursnum, IS_CURSOR_NUM(cursnum) ? CATEG_DIRCUR : CATEG_TABLE);
    if (td)
    { if (ltab->describe(td, false))    // decsribe will release the [td]
        if (alloc_data_cache(ltab, 1))
          { ltab->cursnum=cursnum;  return ltab; }
    }
  }
  delete ltab;
  return NULL;
}

CFNC DllExport void WINAPI destroy_cached_access(ltable * dt)
{ delete dt; }

SWORD catr_to_c_type(atr_info * catr)
{ if (catr->flags & CA_INDIRECT)
    return catr->type==ATT_RASTER || catr->type==ATT_NOSPEC || catr->type==ATT_SIGNAT
             ? SQL_C_BINARY : catr->specif.wide_char ? SQL_C_WCHAR : SQL_C_CHAR;
  switch (catr->type)
  { case ATT_INT8:    return SQL_C_TINYINT;
    case ATT_INT16:   return SQL_C_SHORT;  /* must use ODBC 1.0 types due to old drivers */
    case ATT_INT32:   return SQL_C_LONG;   /* must use ODBC 1.0 types due to old drivers */
    case ATT_INT64:   return SQL_C_SBIGINT;
    case ATT_FLOAT:   return SQL_C_DOUBLE;
    case ATT_CHAR:  case ATT_STRING:
                      return catr->specif.wide_char ? SQL_C_WCHAR : SQL_C_CHAR;
    case ATT_BINARY:  return SQL_C_BINARY;
    case ATT_BOOLEAN: return SQL_C_BIT;
    case ATT_ODBC_NUMERIC:  case ATT_ODBC_DECIMAL:  return SQL_C_CHAR;
    case ATT_ODBC_DATE:       return SQL_C_DATE;
    case ATT_ODBC_TIME:       return SQL_C_TIME;
    case ATT_ODBC_TIMESTAMP:  return SQL_C_TIMESTAMP;
  }
  return 0;
}

#ifdef CLIENT_ODBC_9

void bind_cache(cdp_t cdp, ltable * ltab)
{ unsigned i;  atr_info * catr;  RETCODE retcode;  
  SQLLEN * pcbValues = (SQLLEN*)(ltab->odbc_buf+ltab->rr_datasize);
  for (i=1, catr=ltab->attrs+1;  i<ltab->attrcnt;  i++, catr++)
    if (!(catr->flags & CA_MULTIATTR))
    { SWORD ctype=catr_to_c_type(catr);
      if (ctype)
      { if (!(catr->flags & CA_INDIRECT))
        { { 
#ifdef STOP  // no more, for ATT_CHAR the terminator size has been added to catr->size in describe()
            if (catr->type==ATT_CHAR)  /* must provide space for char terminator */
              retcode=SQLBindCol(ltab->hStmt, i, ctype, ltab->odbc_buf+catr->offset, 2, pcbValues);
            else 
#endif
              retcode=SQLBindCol(ltab->hStmt, i, ctype, ltab->odbc_buf+catr->offset, catr->size, pcbValues);
          }
          pcbValues++;
        }
        else /* SetPos updating needs binding all columns */
        /* must not bind it when using cursor library, the cache would have 0-size */
        if (ltab->conn->SetPos_support)
        { indir_obj * iobj = (indir_obj*)(ltab->odbc_buf+catr->offset);
          retcode=SQLBindCol(ltab->hStmt, i, ctype, NULL, 0, &iobj->actsize);  // in version 9 pcbValue returned to actsize, should work with NULL buffer
          //aux_counter++;  /* This solves an MS Access 2.0 problem */
        }
        else retcode=SQL_SUCCESS;
        if (retcode!=SQL_SUCCESS /*&& retcode!=SQL_SUCCESS_WITH_INFO*/)
          odbc_error(ltab->conn, ltab->hStmt);
      }
    }
  retcode=SQLSetStmtOption(ltab->hStmt, SQL_BIND_TYPE, ltab->rec_cache_real);
   /* for multirecord binding rec_cache_size must be used instead!! */
  //if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    //if (ltab->conn->absolute_fetch)  // otherwise no reporting
      //if (!GetPP()->NoBindTypeErrorReport)
        //odbc_stmt_error(cdp, ltab->hStmt);
  retcode=SQLSetStmtOption(ltab->hStmt, SQL_ROWSET_SIZE, 1);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    if (ltab->conn->absolute_fetch)  // otherwise no reporting
      odbc_error(ltab->conn, ltab->hStmt);
}

#endif

CFNC DllKernel void WINAPI ident_to_sql(xcdp_t xcdp, char * dest, const char * name)
{ if (*name=='`') 
    strcpy(dest, name);  // supposed to be quoted
  else
  { bool contains_non_alpha = false;
    int i=0;
    while (name[i])
    { char c=name[i];
      if (!(c>='A' && c<='Z' || c>='a' && c<='z' || c=='_' || c>='0' && c<='9' && i>0))
        { contains_non_alpha = true;  break; }
      i++;
    }
    if (xcdp->connection_type==CONN_TP_ODBC)
    { char q = xcdp->get_odbcconn()->identifier_quote_char;
      if (q!=' ' && (contains_non_alpha || (check_atr_name((tptr)name) & IDCL_SQL_KEY)))
        sprintf(dest, "%c%s%c", q, name, q);
      else
        strcpy(dest, name);
    }
    else  // 602SQL
    { if (specname(xcdp->get_cdp(), name))
        sprintf(dest, "`%s`", name);
      else
        strcpy(dest, name);
    }
  }
}

CFNC DllKernel void WINAPI make_full_table_name(xcdp_t xcdp, t_flstr & dest, const char * table_name, const char * schema_name)
{ char qname[1+MAX_OBJECT_NAME_LEN+1+1];
  if (schema_name && *schema_name && stricmp(schema_name, "<Default>"))
    if (xcdp->connection_type==CONN_TP_602 || !(xcdp->get_odbcconn()->special_flags & 2))  // unless MySQL, which must not be prefixed
    { ident_to_sql(xcdp, qname, schema_name);  
      dest.put(qname);  
     // add separator:
      if (xcdp->connection_type==CONN_TP_602) 
        dest.putc('.'); 
      else
        dest.put(xcdp->get_odbcconn()->qualifier_separator); 
    }
  ident_to_sql(xcdp, qname, table_name);  
  dest.put(qname);  // source table or cursor name
}

////////////////////////// environment and connection support //////////////////////////////////
HENV hEnv = 0;
#ifdef LINUX
extern int libodbc_is_present;  // defined by relay stub
#endif  

static bool allocate_env(void)
{ if (hEnv) return true;
#ifdef LINUX
  if (!libodbc_is_present) return false;  // libodbc.so found and loaded dynamically
#endif  
  if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv)))
  { SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    return true;
  }
  hEnv=0;
  return false;
}

CFNC DllKernel void WINAPI ODBC_release_env(void)
// closing the ODBC environment:
{
  if (hEnv)
  { SQLFreeHandle(SQL_HANDLE_ENV, hEnv);  // must not be called before all connections are closed
    hEnv=0;
  }
}

CFNC DllKernel BOOL WINAPI ODBC_data_sources(bool first, const char * dsn, int dsn_space, const char * descr, int descr_space)
{ if (!allocate_env()) return FALSE;
  return SQL_SUCCEEDED(SQLDataSourcesA(hEnv, first ? SQL_FETCH_FIRST : SQL_FETCH_NEXT, (SQLCHAR *)dsn, dsn_space, NULL, (SQLCHAR *)descr, descr_space, NULL)); 
}

CFNC DllKernel t_connection * WINAPI ODBC_connect(const char * dsn, const char * uid, const char * pwd, void * window_handle)
{
  if (!allocate_env()) return NULL;
  t_connection * conn = new t_connection;  RETCODE retcode;
  if (!conn) return NULL;
  if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &conn->hDbc)))
  { SQLSetConnectAttr(conn->hDbc, SQL_ATTR_ODBC_CURSORS, SQL_CUR_USE_IF_NEEDED, 0); 
    char inconn[4+SQL_MAX_DSN_LENGTH+1+2*OBJ_NAME_LEN+50];
    sprintf(inconn, "DSN=%s", dsn);
    if (uid)
      { strcat(inconn, ";UID=");  strcat(inconn, uid); }
    if (pwd)
      { strcat(inconn, ";PWD=");  strcat(inconn, pwd); }
#if 1
    char outconn[300];  SQLSMALLINT outlength;
	  retcode=SQLDriverConnectA(conn->hDbc, (SQLHWND)window_handle, (SQLCHAR *)inconn, SQL_NTS, (SQLCHAR *)outconn, sizeof(outconn), &outlength, window_handle ? SQL_DRIVER_COMPLETE : SQL_DRIVER_NOPROMPT);  // SQL_DRIVER_COMPLETE_REQUIRED disables some usefull fields
	  //retcode=SQLConnect(conn->hDbc, (SQLCHAR *)as->server_name, SQL_NTS, (SQLCHAR *)"", SQL_NTS, (SQLCHAR *)"", SQL_NTS);
    if (SQL_SUCCEEDED(retcode))
    { const char * uid = strstr(outconn, "UID=");
      if (uid)
      { uid+=4;  // skip "UID="
        const char * stop = strchr(uid, ';');
        int len = stop ? (int)(stop-uid) : (int)strlen(uid);
        conn->uid = (char*)corealloc(len+1, 78);
        if (conn->uid) strmaxcpy((char*)conn->uid, uid, len+1);
      }
#else
    wchar_t outconn[300];  SQLSMALLINT outlength;
    retcode=SQLDriverConnectW(conn->hDbc, (SQLHWND)frame->GetHandle(), (wchar_t*)wxString(inconn, *wxConvCurrent).c_str(), SQL_NTS, outconn, sizeof(outconn), &outlength, SQL_DRIVER_COMPLETE);  // SQL_DRIVER_COMPLETE_REQUIRED disables some usefull fields
    if (SQL_SUCCEEDED(retcode))
    { const wchar_t * uid = wcsstr(outconn, wxT("UID="));
      if (uid)
      { uid+=4;  // skip "UID="
        const wchar_t * stop = wcschr(uid, ';');
        int len = stop ? stop-uid : wcslen(uid);
        conn->uid = (char*)corealloc(len+1, 78);
        if (conn->uid) //strmaxcpy((char*)conn->uid, uid, len+1);
          for (int j=0;  j<=len;  j++)
             ((char*)conn->uid)[j]=(char)uid[j];
      }
#endif
      SQLAllocHandle(SQL_HANDLE_STMT, conn->hDbc, &conn->hStmt);

          conn->owner_usage=conn->qualifier_usage=0;
          if (SQLGetInfo(conn->hDbc, SQL_OWNER_USAGE,     &conn->owner_usage,     sizeof(uns32), NULL)==SQL_ERROR)
          { /* may be 1.0 ODBC driver */
            char owner_term[20] = "";
            if (SQLGetInfo(conn->hDbc, SQL_OWNER_TERM,     owner_term,  sizeof(owner_term),  NULL) != SQL_ERROR
                && *owner_term) conn->owner_usage=SQL_OU_DML_STATEMENTS | SQL_OU_TABLE_DEFINITION | SQL_OU_INDEX_DEFINITION | SQL_OU_PRIVILEGE_DEFINITION;
            else conn->owner_usage=0;
          }
          if (SQLGetInfo(conn->hDbc, SQL_QUALIFIER_USAGE, &conn->qualifier_usage, sizeof(uns32), NULL)==SQL_ERROR)
            conn->qualifier_usage=0;
          if (SQLGetInfo(conn->hDbc, SQL_QUALIFIER_LOCATION, &conn->qualifier_loc, sizeof(uns16), NULL)==SQL_ERROR)
            conn->qualifier_loc=0;
          if (SQLGetInfoA(conn->hDbc, SQL_QUALIFIER_NAME_SEPARATOR, conn->qualifier_separator, sizeof(conn->qualifier_separator), NULL)==SQL_ERROR)
            conn->qualifier_separator[0]=0;
          if (conn->owner_usage!=0 /*&& conn->qualifier_usage==0 && conn->qualifier_separator[0]==0*/)
            strcpy(conn->qualifier_separator, ".");  // the separator is used as owner separator -> should be '.'
          if (SQLGetInfo(conn->hDbc, SQL_ODBC_SQL_CONFORMANCE, &conn->sql_conform, sizeof(conn->sql_conform), NULL)==SQL_ERROR)
            conn->sql_conform=SQL_OSC_MINIMUM;
          char pom[3];  
          if (SQLGetInfoA(conn->hDbc, SQL_IDENTIFIER_QUOTE_CHAR, pom, sizeof(pom), NULL)==SQL_ERROR)
            conn->identifier_quote_char=' ';
          else conn->identifier_quote_char=pom[0] ? pom[0] : ' ';
         /* SetPos and locking support (ODBC 2.0): */
          uns32 pos_mask;
          if (SQLGetInfo(conn->hDbc, SQL_POS_OPERATIONS, &pos_mask, sizeof(pos_mask), NULL)==SQL_SUCCESS)
            conn->SetPos_support=(pos_mask & (SQL_POS_UPDATE|SQL_POS_DELETE|SQL_POS_ADD|SQL_POS_POSITION|SQL_POS_REFRESH)) ==
              (SQL_POS_UPDATE|SQL_POS_DELETE|SQL_POS_ADD|SQL_POS_POSITION|SQL_POS_REFRESH);
          else conn->SetPos_support=FALSE;
          if (SQLGetInfo(conn->hDbc, SQL_LOCK_TYPES, &pos_mask, sizeof(pos_mask), NULL)==SQL_SUCCESS)
            conn->can_lock=(pos_mask & (SQL_LCK_EXCLUSIVE|SQL_LCK_UNLOCK)) ==
              (SQL_LCK_EXCLUSIVE|SQL_LCK_UNLOCK);
          else conn->can_lock=FALSE;
          if (SQLGetInfo(conn->hDbc, SQL_FETCH_DIRECTION, &pos_mask, sizeof(pos_mask), NULL)==SQL_SUCCESS)
            conn->absolute_fetch=(pos_mask & SQL_FD_FETCH_ABSOLUTE) != 0;
          else conn->absolute_fetch=FALSE;
         // other info and flags: 
          if (!SQL_SUCCEEDED(SQLGetInfoA(conn->hDbc, SQL_DATA_SOURCE_NAME, conn->dsn,        sizeof(conn->dsn),           NULL)))
            *conn->dsn=0;
          if (!SQL_SUCCEEDED(SQLGetInfoA(conn->hDbc, SQL_DBMS_NAME,     conn->dbms_name,     sizeof(conn->dbms_name),     NULL)))
            *conn->dbms_name=0;
          if (!SQL_SUCCEEDED(SQLGetInfoA(conn->hDbc, SQL_DBMS_VER,      conn->dbms_ver,      sizeof(conn->dbms_ver),      NULL)))
            *conn->dbms_ver=0;
          if (!SQL_SUCCEEDED(SQLGetInfoA(conn->hDbc, SQL_DATABASE_NAME, conn->database_name, sizeof(conn->database_name), NULL)))
            *conn->database_name=0;
          //Upcase9(cdp, dbms_name);
          if (!stricmp(conn->dbms_name, "ACCESS"))    conn->flags|=CONN_FL_NAMED_CONSTRS;
          if (!memcmp (conn->dbms_name, "ORACLE", 6)) conn->flags|=CONN_FL_DISABLE_COLUMN_PRIVILS;  // very slow operation
          if (!stricmp(conn->dbms_name, "MySQL"))
            conn->SetPos_support=FALSE;  // crashes when editing CLOB objects in the grid
          //if (GetPP()->NoPrivils)               conn->flags|=CONN_FL_DISABLE_COLUMN_PRIVILS;  // using data source privils disabled (does not work e.g. in the new MySQL)
         /* scrolling & concurrency: */
          if (SQLGetInfo(conn->hDbc, SQL_SCROLL_OPTIONS, &conn->scroll_options, sizeof(conn->scroll_options), NULL)==SQL_ERROR)
            conn->scroll_options=0;
          if (SQLGetInfo(conn->hDbc, SQL_SCROLL_CONCURRENCY, &conn->scroll_concur, sizeof(conn->scroll_concur), NULL)==SQL_ERROR)
            conn->scroll_concur=0;
         /* commit and roll_back: */
          conn->can_transact=TRUE;
          uns16 info16;
          if (SQLGetInfo(conn->hDbc, SQL_CURSOR_COMMIT_BEHAVIOR,   &info16, sizeof(info16), NULL)==SQL_SUCCESS)
            if (info16 != SQL_CB_PRESERVE) conn->can_transact=FALSE;
          if (SQLGetInfo(conn->hDbc, SQL_CURSOR_ROLLBACK_BEHAVIOR, &info16, sizeof(info16), NULL)==SQL_SUCCESS)
            if (info16 != SQL_CB_PRESERVE) conn->can_transact=FALSE;
         /* other driver information: */
          if (SQLGetInfo(conn->hDbc, SQL_NON_NULLABLE_COLUMNS, &info16, sizeof(info16), NULL)==SQL_SUCCESS)
            if (info16==SQL_NNC_NON_NULL) conn->flags|=CONN_FL_NON_NULL;
          if (SQLGetInfo(conn->hDbc, SQL_FILE_USAGE, &conn->sql_file_usage, sizeof(conn->sql_file_usage), NULL)!=SQL_SUCCESS)
            conn->sql_file_usage=0;
          if (!stricmp(conn->dbms_name, "MySQL"))
          { conn->special_flags |= 2;  // inverted parameters to SQLTables
			conn->flags|=CONN_FL_DISABLE_COLUMN_PRIVILS;
#ifdef WINS
            char key[160];  HKEY hKey;
            strcpy(key, "SOFTWARE\\ODBC\\ODBC.INI\\");  strcat(key, dsn);
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey)==ERROR_SUCCESS ||
                RegOpenKeyEx(HKEY_CURRENT_USER,  key, 0, KEY_READ, &hKey)==ERROR_SUCCESS)
            { char buf[20];  DWORD key_len=sizeof(buf);
              if (RegQueryValueEx(hKey, "OPTION", NULL, NULL, (BYTE*)buf, &key_len)==ERROR_SUCCESS)
              { sig32 ival;  
                if (str2int(buf, &ival))
                  if (ival & 32)
                    conn->special_flags |= 1;
              }  
              RegCloseKey(hKey);
            }
#endif
          }
         // complete and insert into the list:
          strmaxcpy(conn->conn_server_name,  dsn, sizeof(conn->conn_server_name ));
          strmaxcpy(conn->locid_server_name, dsn, sizeof(conn->locid_server_name));
          add_to_list(conn);
          return conn;
    }
    else odbc_error(conn, 0);
    SQLFreeHandle(SQL_HANDLE_DBC, conn->hDbc);
  }
  delete conn;
  return NULL;
}

CFNC DllKernel void WINAPI ODBC_disconnect(t_connection * conn)
{ SQLDisconnect(conn->hDbc);
  SQLFreeHandle(SQL_HANDLE_DBC, conn->hDbc);
  remove_from_list(conn);
  delete conn;
}

CFNC DllKernel tptr WINAPI srch(tptr source, tptr patt)
{ int i;
  while (*source)
  { i=0;
    while (patt[i])
      if (cUPCASE(source[i])==patt[i]) i++; else break;
    if (!patt[i]) return source;
    source++;
  }
  return NULL;
}

CFNC DllKernel int WINAPI odbc_delete_all9(t_connection * conn, HSTMT hStmt, tptr select_st)
{ tptr p_from, p_where, p_order, p_for, stmt;  unsigned l_where, l_from, pos;
  if (srch(select_st, "GROUP")   ||
      srch(select_st, "HAVING") ||
      srch(select_st, "UNION")) return 1;
  p_from =srch(select_st, "FROM");
  p_where=srch(select_st, "WHERE");
  p_order=srch(select_st, "ORDER");
  p_for  =srch(select_st, "FOR");
  if (p_for && !p_order) p_order=p_for;
  l_where = !p_where ? 0 :
            p_order ? (unsigned)(p_order-p_where) : (unsigned)strlen(p_where);
  l_from  = p_where ? (unsigned)(p_where-p_from ) :
            p_order ? (unsigned)(p_order-p_from)  : (unsigned)strlen(p_from);
  for (pos=0;  pos<l_from; pos++)
    if (p_from[pos]==',') return 1;  /* joined tables */
  stmt=(tptr)sigalloc(l_from+l_where+20, 73);
  if (stmt==NULL) return 0;
  memcpy(stmt, "DELETE ", 7);   pos=7;
  memcpy(stmt+pos, p_from,  l_from);   pos+=l_from;
  memcpy(stmt+pos, p_where, l_where);  pos+=l_where;
  stmt[pos]=0;
  RETCODE retcode=SQLExecDirect(hStmt, (UCHAR*)stmt, SQL_NTS);
  corefree(stmt);
  if (retcode!=SQL_SUCCESS) odbc_error(conn, hStmt);
  return retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO ? 2 : 0;
}

