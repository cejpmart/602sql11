/*
** OPTIONS.C - This is the ODBC driver code for executing Set/GetConnect/StmtOption.
*/

#include "wbdrv.h"
#pragma hdrstop

BOOL set_all_stmt_option(DBC * dbc, UWORD fOption, UDWORD vParam)
{ STMT * stmt = dbc->stmts;
  if (stmt)
    do
    { //if (SQLSetStmtOption((HSTMT)stmt, fOption, vParam) == SQL_ERROR)
      //  return TRUE;
      stmt=stmt->next_stmt;
    } while (stmt != dbc->stmts);
  return FALSE;
}

SQLRETURN SQL_API SQLSetEnvAttr(SQLHENV	EnvironmentHandle,
	SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{ if (!IsValidHENV(EnvironmentHandle)) return SQL_INVALID_HANDLE;
  ENV * env = (ENV*)EnvironmentHandle;  
  CLEAR_ERR(env);
  switch (Attribute)
  { case SQL_ATTR_CONNECTION_POOLING:
      env->connection_pooling = (SQLUINTEGER)(size_t)ValuePtr;  break;
    case SQL_ATTR_CP_MATCH:
      env->cp_match = (SQLUINTEGER)(size_t)ValuePtr;  break;
    case SQL_ATTR_ODBC_VERSION:
      env->odbc_version = (SQLINTEGER)(size_t)ValuePtr;  break;
  }
  return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetEnvAttr(SQLHENV	EnvironmentHandle,
	SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
	SQLINTEGER * StringLengthPtr)
{ if (!IsValidHENV(EnvironmentHandle)) return SQL_INVALID_HANDLE;
  ENV * env = (ENV*)EnvironmentHandle;  
  CLEAR_ERR(env);
  if (ValuePtr==NULL) return SQL_SUCCESS;  // empty action
  switch (Attribute)
  { case SQL_ATTR_CONNECTION_POOLING:
      *(SQLUINTEGER*)ValuePtr = env->connection_pooling;  break;
    case SQL_ATTR_CP_MATCH:
      *(SQLUINTEGER*)ValuePtr = env->cp_match;  break;
    case SQL_ATTR_ODBC_VERSION:
      *(SQLINTEGER*)ValuePtr = env->odbc_version;  break;
  }
  if (StringLengthPtr!=NULL) *StringLengthPtr=sizeof(SQLINTEGER);
  return SQL_SUCCESS;
}

SQLRETURN  SQL_API SQLSetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, SQLPOINTER vParam,SQLINTEGER StringLength)
{ if (!IsValidHDBC(ConnectionHandle)) return SQL_INVALID_HANDLE;
  DBC * dbc = (DBC*)ConnectionHandle;  STMT * stmt;
  CLEAR_ERR(dbc);
 /* test asynchron. execution */
  stmt=dbc->stmts;
  if (stmt) do
  { if (stmt->async_run)  /* async. oper. running */
    { raise_err_dbc(dbc, SQL_HY010);  /* Function sequence error */
      return SQL_ERROR;
    }
    stmt=stmt->next_stmt;
  } while (stmt != dbc->stmts);
 /* test and save the proper option: */
  switch (Attribute)
  { case SQL_ATTR_ACCESS_MODE:
      dbc->access_mode=(sig32)(size_t)vParam;
      break;
    case SQL_ATTR_ASYNC_ENABLE:
      dbc->async_enabled = (SQLUINTEGER)(size_t)vParam==SQL_ASYNC_ENABLE_ON;
      break;
    case SQL_ATTR_AUTO_IPD:
      break;  // cannot be set
    case SQL_ATTR_AUTOCOMMIT:
      if ((sig32)(size_t)vParam==SQL_AUTOCOMMIT_ON && dbc->autocommit==SQL_AUTOCOMMIT_OFF)
        cd_Commit(dbc->cdp);   /* starting autocommit, commit the open transaction */
        /* should return SUCCESS_WITH_INFO, but no specific state exists */
      else
      if ((sig32)(size_t)vParam==SQL_AUTOCOMMIT_OFF && dbc->autocommit==SQL_AUTOCOMMIT_ON)
        cd_Start_transaction(dbc->cdp); /* starting manual commit, no tranaction pending */
      dbc->autocommit=(sig32)(size_t)vParam;
      break;
    case SQL_ATTR_CONNECTION_DEAD:  // ODBC 3.5
      break;  // cannot be set by definition
    case SQL_ATTR_CONNECTION_TIMEOUT:
      if (vParam)
        raise_err_dbc(dbc, SQL_01S02);  /* Option value changed */
      else dbc->conn_timeout=(sig32)(size_t)vParam;
      break;
    case SQL_ATTR_CURRENT_CATALOG:  // cannot change the server name when connected
    { 
#ifdef STOP  // just ignore this, MS Excel calls this and must not get the error 
      wide2ansi xCatalog((const SQLWCHAR *)vParam, StringLength);
      if (stricmp(xCatalog.get_string(), dbc->server_name))
      { raise_err_dbc(dbc, SQL_HYC00);  /* Optional feature not implemented */
        return SQL_ERROR;
      }
#endif      
      break;
    }
    case SQL_ATTR_LOGIN_TIMEOUT:
      if (vParam)
        raise_err_dbc(dbc, SQL_01S02);  /* Option value changed */
      else dbc->login_timeout=(sig32)(size_t)vParam;
      break;
    case SQL_ATTR_METADATA_ID:
      dbc->stmtopt.metadata = (uns32)(size_t)vParam==SQL_TRUE;
      break;
    case SQL_ATTR_ODBC_CURSORS:  // DM option, tested
      break;
    case SQL_ATTR_PACKET_SIZE:     // ignored
      raise_err_dbc(dbc, SQL_01S02);  /* Option value changed */
      return SQL_SUCCESS_WITH_INFO;                     
    case SQL_ATTR_QUIET_MODE:    // no action because no dialogs
      // driver receives 0 for every input parameter
      break;
    case SQL_ATTR_TRACE:         // DM option, tested
    case SQL_ATTR_TRACEFILE:
      break;  
    case SQL_ATTR_TRANSLATE_LIB:
      if (!dbc->connected)
      { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
        return SQL_ERROR;
      }
      raise_err_dbc(dbc, SQL_IM001);  /* Driver does not support this function */
      return SQL_ERROR;
    case SQL_ATTR_TRANSLATE_OPTION:
      if (!dbc->connected)
      { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
        return SQL_ERROR;
      }
      dbc->translate_option=(sig32)(size_t)vParam;
      raise_err_dbc(dbc, SQL_IM001);  /* Driver does not support this function */
      return SQL_ERROR;
    case SQL_ATTR_TXN_ISOLATION:
      if (!dbc->connected)
      { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
        return SQL_ERROR;
      }
      dbc->txn_isolation=(sig32)(size_t)vParam;
      cd_Set_transaction_isolation_level(dbc->cdp, 
        dbc->txn_isolation==SQL_TXN_READ_UNCOMMITTED ? READ_UNCOMMITTED :
        dbc->txn_isolation==SQL_TXN_READ_COMMITTED   ? READ_COMMITTED :
        dbc->txn_isolation==SQL_TXN_REPEATABLE_READ  ? REPEATABLE_READ :
        /*dbc->txn_isolation==SQL_TXN_SERIALIZABLE ?*/ SERIALIZABLE);
      break;
    default:
      return SUCCESS(dbc); // some systems use non-standard attrs and fail on this error
      //raise_err_dbc(dbc, SQL_HY092); /* Option type out of range */
      //return SQL_ERROR;
   /************************ statement options: *****************************/
    case SQL_MAX_LENGTH:
      if (vParam)
        raise_err_dbc(dbc, SQL_01S02);  /* Option value changed */
      else
      { dbc->stmtopt.max_data_len=(sig32)(size_t)vParam;    /* zero iff not restricted */
        //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      }
      break;
    case SQL_NOSCAN:
      dbc->stmtopt.no_scan=(sig32)(size_t)vParam==SQL_NOSCAN_ON;
      //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      break;
    case SQL_QUERY_TIMEOUT:
      if (vParam)
        raise_err_dbc(dbc, SQL_01S02);  /* Option value changed */
      else
      { dbc->stmtopt.query_timeout=(sig32)(size_t)vParam;
        //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      }
      break;
    case SQL_ATTR_MAX_ROWS:
      if (vParam)
        raise_err_dbc(dbc, SQL_01S02);  /* Option value changed */
      else
      { dbc->stmtopt.max_rows=(sig32)(size_t)vParam;
        //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      }
      break;
    case SQL_RETRIEVE_DATA:
      dbc->stmtopt.retrieve_data=(sig32)(size_t)vParam==SQL_RD_ON;
      //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      break;
    case SQL_SIMULATE_CURSOR:
      dbc->stmtopt.sim_cursor=(sig32)(size_t)vParam;
      //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      break;
    case SQL_USE_BOOKMARKS:
      dbc->stmtopt.use_bkmarks=(sig32)(size_t)vParam==SQL_UB_ON;
      //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      break;

    case SQL_CONCURRENCY: /* common with SetScrollOptions */
      dbc->stmtopt.concurrency=(UWORD)(size_t)vParam;
      //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      break;
    case SQL_CURSOR_TYPE: /* common with SetScrollOptions */
      dbc->stmtopt.cursor_type=(sig32)(size_t)vParam;
      //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      break;
    case SQL_KEYSET_SIZE: /* common with SetScrollOptions */
      dbc->stmtopt.crowKeyset=(SQLINTEGER)(size_t)vParam;
      //if (set_all_stmt_option(dbc, fOption, vParam)) return SQL_ERROR;
      break;
  }
  return SUCCESS(dbc);
}

//      -       -       -       -       -       -       -       -       -
BOOL put_string(const char * text, SQLPOINTER Value,
                SQLINTEGER BufferLength, SQLINTEGER *StringLength)
/* writes null-terminated string in a standard way, return TRUE on trunc. */
{ unsigned len=(unsigned)strlen(text);
  if (StringLength != NULL) *StringLength=len;  // the full length!
  if (Value == NULL || BufferLength <= 0) return FALSE;
  strmaxcpy((tptr)Value, text, BufferLength);
  return len >= BufferLength;
}

SQLRETURN SQL_API SQLGetConnectAttrW(SQLHDBC hdbc, SQLINTEGER fAttribute, SQLPOINTER rgbValue, SQLINTEGER cbValueMax,
    SQLINTEGER *pcbValue)
{ if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  DBC * dbc = (DBC*)hdbc;
  CLEAR_ERR(dbc);
 /* test asynchron. execution */
  STMT * stmt=dbc->stmts;
  if (stmt) do
  { if (stmt->async_run)  /* async. oper. running */
    { raise_err_dbc(dbc, SQL_HY010);  /* Function sequence error */
      return SQL_ERROR;
    }
    stmt=stmt->next_stmt;
  } while (stmt != dbc->stmts);
  if (rgbValue == NULL) return SQL_SUCCESS;  // empty action, should not write *pcbValue
  BOOL truncated=FALSE;
 /* test and save the proper option: */
  switch (fAttribute)
  { case SQL_ATTR_ACCESS_MODE:
      *(sig32*)rgbValue=dbc->access_mode;
      break;
    case SQL_ATTR_ASYNC_ENABLE:
      *(SQLUINTEGER*)rgbValue = dbc->async_enabled ? SQL_ASYNC_ENABLE_ON : SQL_ASYNC_ENABLE_OFF;
      break;
    case SQL_ATTR_AUTO_IPD:
      *(sig32*)rgbValue=SQL_TRUE;  // returns implementation status
      break;
    case SQL_ATTR_AUTOCOMMIT:
      *(sig32*)rgbValue=dbc->autocommit;
      break;
    case SQL_ATTR_CONNECTION_DEAD:  // ODBC 3.5
      *(sig32*)rgbValue=SQL_CD_FALSE;
      break;
    case SQL_ATTR_CONNECTION_TIMEOUT:
      *(sig32*)rgbValue=dbc->conn_timeout;
      break;
    case SQL_ATTR_CURRENT_CATALOG:
      if (!dbc->connected)
      { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
        return SQL_ERROR;
      }
      if (write_string_wide_b4(dbc->cdp->conn_server_name, (SQLWCHAR *)rgbValue, cbValueMax, pcbValue))
        truncated=TRUE;
      break;
    case SQL_ATTR_LOGIN_TIMEOUT:
      *(sig32*)rgbValue=dbc->login_timeout;
      break;
    case SQL_ATTR_METADATA_ID:
      *(sig32*)rgbValue=dbc->stmtopt.metadata ? SQL_TRUE : SQL_FALSE;
      break;
    case SQL_ATTR_ODBC_CURSORS:  // DM option, tested
      break;
    case SQL_ATTR_PACKET_SIZE:
      *(sig32*)rgbValue=0;
      break;                    
    case SQL_ATTR_QUIET_MODE:    // no action because no dialogs
      *(SQLLEN*)rgbValue=0;
      break;
    case SQL_ATTR_TRACE:     // DM option, tested
    case SQL_ATTR_TRACEFILE:
      break;
    case SQL_ATTR_TRANSLATE_LIB:
      if (!dbc->connected)
      { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
        return SQL_ERROR;
      }
      /* must not write result on error */
      raise_err_dbc(dbc, SQL_IM001);  /* Driver does not support this function */
      return SQL_ERROR;
    case SQL_ATTR_TRANSLATE_OPTION:
      if (!dbc->connected)
      { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
        return SQL_ERROR;
      }
      *(sig32*)rgbValue=dbc->translate_option;
      break;
    case SQL_ATTR_TXN_ISOLATION:
      *(sig32*)rgbValue=dbc->txn_isolation;
      break;
    default:
      raise_err_dbc(dbc, SQL_HY092); /* Option type out of range */
      return SQL_ERROR;
  }
  if (truncated)
  { raise_err_dbc(dbc, SQL_01004);  /* Data truncated */
    return SQL_SUCCESS_WITH_INFO;
  }
  return SUCCESS(dbc);
}

//      Sets options that control the behavior of cursors.
// Due to a bug in the DM drivers have to implement it. See Release notes.

SQLRETURN SQL_API SQLSetScrollOptions(HSTMT StatementHandle, UWORD Concurrency,
        SQLLEN KeysetSize, UWORD RowsetSize)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
 /* store parameters: */
  switch (KeysetSize)
  { case SQL_SCROLL_FORWARD_ONLY:  stmt->cursor_type=SQL_CURSOR_FORWARD_ONLY;  break;
    case SQL_SCROLL_STATIC:        stmt->cursor_type=SQL_CURSOR_STATIC;  break;
    case SQL_SCROLL_KEYSET_DRIVEN: stmt->cursor_type=SQL_CURSOR_KEYSET_DRIVEN;  break;
    case SQL_SCROLL_DYNAMIC:       stmt->cursor_type=SQL_CURSOR_DYNAMIC;  break;
    default:                       stmt->cursor_type=SQL_CURSOR_KEYSET_DRIVEN;  break;
  }
  stmt->concurrency     = Concurrency;
  stmt->ard->array_size = RowsetSize;
  if (KeysetSize>0) 
    stmt->crowKeyset    = KeysetSize;
  return SUCCESS(stmt);
}

//      -       -       -       -       -       -       -       -       -

SQLRETURN SQL_API SQLSetStmtAttrW(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (stmt->AE_mode)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  switch (Attribute)
  { default:
      return SUCCESS(stmt); // some systems use non-standard attrs and fail on this error
      //raise_err(stmt, SQL_HY092);  /* Option type out of range */
      //return SQL_ERROR;
    case SQL_ATTR_APP_PARAM_DESC:
      if (Value==SQL_NULL_HDESC) stmt->apd=&stmt->impl_apd;
      else stmt->apd=(DESC*)Value;
      break;
    case SQL_ATTR_APP_ROW_DESC:
      if (Value==SQL_NULL_HDESC) stmt->ard=&stmt->impl_ard;
      else stmt->ard=(DESC*)Value;
      break;
    case SQL_ASYNC_ENABLE:
#ifdef STOP // option enabled in WB 6.0c, no harm, some users have problems with the error message
      if ((uns32)Value==SQL_ASYNC_ENABLE_ON)
      { raise_err(stmt, SQL_HYC00);  /* Driver not capable */
        return SQL_ERROR;
      }
#endif
      stmt->async_enabled = (SQLUINTEGER)(size_t)Value==SQL_ASYNC_ENABLE_ON;
      break;
    case SQL_ATTR_CONCURRENCY: 
      stmt->concurrency=(UWORD)(size_t)Value;
      break;
    case SQL_ATTR_CURSOR_SCROLLABLE:
      stmt->scrollable = (uns32)(size_t)Value==SQL_SCROLLABLE;
      break;
    case SQL_ATTR_CURSOR_SENSITIVITY: 
      stmt->sensitivity=(uns32)(size_t)Value;
      break;
    case SQL_ATTR_CURSOR_TYPE:
      stmt->cursor_type=(uns32)(size_t)Value;
      break;
    case SQL_ATTR_ENABLE_AUTO_IPD:
      stmt->auto_ipd = (uns32)(size_t)Value == SQL_TRUE;
      break;
    case SQL_ATTR_FETCH_BOOKMARK_PTR:
      stmt->bookmark_ptr=(void*)Value;
      break;
    case SQL_ATTR_IMP_PARAM_DESC:  // cannot be set
    case SQL_ATTR_IMP_ROW_DESC:
      break;
    case SQL_ATTR_KEYSET_SIZE: /* common with SetScrollOptions */
      stmt->crowKeyset=(SQLINTEGER)(size_t)Value;
      break;
    case SQL_ATTR_MAX_LENGTH:
      if ((uns32)(size_t)Value)
        raise_err(stmt, SQL_01S02);  /* Option value changed */
      else
        stmt->max_data_len=(uns32)(size_t)Value;    /* zero iff not restricted */
      break;
    case SQL_ATTR_METADATA_ID:
      stmt->metadata = (uns32)(size_t)Value==SQL_TRUE;
      break;
    case SQL_ATTR_NOSCAN:  // disables converting escape seqs in stmts, not used
      stmt->no_scan=(uns32)(size_t)Value==SQL_NOSCAN_ON;
      break;
    case SQL_ATTR_PARAM_BIND_OFFSET_PTR: // ->APD
      stmt->     apd->bind_offset_ptr  = (SQLINTEGER*)  Value;  break;
    case SQL_ATTR_PARAM_BIND_TYPE:       // ->APD
      stmt->     apd->bind_type        = (SQLUINTEGER)  (size_t)Value;  break;
    case SQL_ATTR_PARAM_OPERATION_PTR:   // ->APD
      stmt->     apd->array_status_ptr = (SQLUSMALLINT*)Value;  break;
    case SQL_ATTR_PARAM_STATUS_PTR:      // ->IPD
      stmt->impl_ipd. array_status_ptr = (SQLUSMALLINT*)Value;  break;
    case SQL_ATTR_PARAMS_PROCESSED_PTR:  // ->IPD
      stmt->impl_ipd. rows_procsd_ptr  = (SQLULEN     *) Value;  break;
    case SQL_ATTR_PARAMSET_SIZE:   // ->APD
      stmt->apd->array_size            = (SQLUINTEGER)  (size_t)Value;  break;
    case SQL_ATTR_QUERY_TIMEOUT:
      if ((uns32)(size_t)Value) raise_err(stmt, SQL_01S02);  /* Option value changed */
      else stmt->query_timeout=(uns32)(size_t)Value;
      break;
    case SQL_ATTR_RETRIEVE_DATA:
      stmt->retrieve_data = (uns32)(size_t)Value==SQL_RD_ON;
      break;
    case SQL_ROWSET_SIZE: // should set rowset_size used by ExtendedFetch only
    case SQL_ATTR_ROW_ARRAY_SIZE:       // ->ARD
      stmt->     ard->array_size       = (SQLUINTEGER)  (size_t)Value;  break;
    case SQL_ATTR_ROW_BIND_OFFSET_PTR:  // ->ARD
      stmt->     ard->bind_offset_ptr  = (SQLINTEGER*)  Value;  break;
    case SQL_ATTR_ROW_BIND_TYPE:        // ->ARD
      stmt->     ard->bind_type        = (SQLUINTEGER)  (size_t)Value;  break;
    case SQL_ATTR_ROW_OPERATION_PTR:    // ->ARD
      stmt->     ard->array_status_ptr = (SQLUSMALLINT*)Value;  break;
    case SQL_ATTR_ROW_STATUS_PTR:       // ->IRD
      stmt->impl_ird. array_status_ptr = (SQLUSMALLINT*)Value;  break;
    case SQL_ATTR_ROWS_FETCHED_PTR:     // ->IRD
      stmt->impl_ird. rows_procsd_ptr  = (SQLULEN     *) Value;  break;
    case SQL_ATTR_MAX_ROWS:  // limit on rows number
      //if ((SQLUINTEGER)Value) raise_err(stmt, SQL_01S02); else /* Option value changed */
      stmt->max_rows=(SQLUINTEGER)(size_t)Value;
      break;
    case SQL_ATTR_SIMULATE_CURSOR:
      stmt->sim_cursor=(uns32)(size_t)Value;
      break;
    case SQL_ATTR_USE_BOOKMARKS:
      stmt->use_bkmarks=(uns32)(size_t)Value;
      break;
  }
  return SUCCESS(stmt);
}

//      -       -       -       -       -       -       -       -       -

SQLRETURN SQL_API SQLGetStmtAttrW(SQLHSTMT hstmt, SQLINTEGER fAttribute, SQLPOINTER rgbValue, SQLINTEGER cbValueMax,
    SQLINTEGER *pcbValue)
{ if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)hstmt;
  CLEAR_ERR(stmt);
  if (stmt->AE_mode)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (rgbValue == NULL) return SQL_SUCCESS;  // empty action, should not write *pcbValue
  switch (fAttribute)
  { default:
      raise_err(stmt, SQL_HY092);  /* Option type out of range */
      return SQL_ERROR;
    case SQL_ATTR_APP_PARAM_DESC:
      *(DESC**)rgbValue=stmt->apd;
      if (pcbValue!=NULL) *pcbValue=SQL_IS_POINTER;
      break;
    case SQL_ATTR_APP_ROW_DESC:
      *(DESC**)rgbValue=stmt->ard;
      if (pcbValue!=NULL) *pcbValue=SQL_IS_POINTER;
      break;
    case SQL_ASYNC_ENABLE:
      *(SQLUINTEGER*)rgbValue = stmt->async_enabled ? SQL_ASYNC_ENABLE_ON : SQL_ASYNC_ENABLE_OFF;
      break;
    case SQL_ATTR_CONCURRENCY: /* common with SetScrollOptions */
      *(UDWORD*)rgbValue = stmt->concurrency;
      break;
    case SQL_ATTR_CURSOR_SCROLLABLE:
      *(UDWORD*)rgbValue = stmt->scrollable ? SQL_SCROLLABLE : SQL_NONSCROLLABLE;
      break;
    case SQL_ATTR_CURSOR_SENSITIVITY: 
      *(UDWORD*)rgbValue = stmt->sensitivity;
      break;
    case SQL_ATTR_CURSOR_TYPE: /* common with SetScrollOptions */
      *(UDWORD*)rgbValue = stmt->cursor_type;
      break;
    case SQL_ATTR_ENABLE_AUTO_IPD:
      *(UDWORD*)rgbValue = stmt->auto_ipd ? SQL_TRUE : SQL_FALSE;
      break;
    case SQL_ATTR_FETCH_BOOKMARK_PTR:
      *(void**)rgbValue = stmt->bookmark_ptr;
      break;
    case SQL_ATTR_IMP_PARAM_DESC:
      *(DESC**)rgbValue=&stmt->impl_ipd;
      if (pcbValue!=NULL) *pcbValue=SQL_IS_POINTER;
      break;
    case SQL_ATTR_IMP_ROW_DESC:
      *(DESC**)rgbValue=&stmt->impl_ird;
      if (pcbValue!=NULL) *pcbValue=SQL_IS_POINTER;
      break;
    case SQL_ATTR_KEYSET_SIZE: 
      *(SQLULEN*)rgbValue = stmt->crowKeyset;
      break;
    case SQL_ATTR_MAX_LENGTH:
      *(SQLULEN*)rgbValue = stmt->max_data_len;
      break;
    case SQL_ATTR_METADATA_ID:
      *(UDWORD*)rgbValue = stmt->metadata ? SQL_TRUE : SQL_FALSE;
      break;
    case SQL_ATTR_NOSCAN:  // disables converting escape seqs in stmts, not used
      *(UDWORD*)rgbValue = stmt->no_scan ? SQL_NOSCAN_ON : SQL_NOSCAN_OFF;
      break;
    case SQL_ATTR_PARAM_BIND_OFFSET_PTR: // ->APD
      *(SQLINTEGER**)  rgbValue = stmt->     apd->bind_offset_ptr;   break;
    case SQL_ATTR_PARAM_BIND_TYPE:       // ->APD
      *(SQLUINTEGER*)  rgbValue = stmt->     apd->bind_type;         break;
    case SQL_ATTR_PARAM_OPERATION_PTR:   // ->APD
      *(SQLUSMALLINT**)rgbValue = stmt->     apd->array_status_ptr;  break;
    case SQL_ATTR_PARAM_STATUS_PTR:      // ->IPD
      *(SQLUSMALLINT**)rgbValue = stmt->impl_ipd. array_status_ptr;  break;
    case SQL_ATTR_PARAMS_PROCESSED_PTR:  // ->IPD
      *(SQLULEN     **)rgbValue = stmt->impl_ipd. rows_procsd_ptr;   break;
    case SQL_ATTR_PARAMSET_SIZE:         // ->APD
      *(SQLUINTEGER*)  rgbValue = stmt->apd->array_size;             break;
    case SQL_ATTR_QUERY_TIMEOUT:
      *(UDWORD*)rgbValue = stmt->query_timeout;
      break;
    case SQL_ATTR_RETRIEVE_DATA:
      *(UDWORD*)rgbValue = stmt->retrieve_data ? SQL_RD_ON : SQL_RD_OFF;
    case SQL_ROWSET_SIZE: // should get rowset_size used by ExtendedFetch only
    case SQL_ATTR_ROW_ARRAY_SIZE:       // ->ARD
      *(SQLULEN*)      rgbValue = stmt->     ard->array_size;        break;
    case SQL_ATTR_ROW_BIND_OFFSET_PTR:  // ->ARD
      *(SQLINTEGER**)  rgbValue = stmt->     ard->bind_offset_ptr;   break;
    case SQL_ATTR_ROW_BIND_TYPE:        // ->ARD
      *(SQLUINTEGER*)  rgbValue = stmt->     ard->bind_type;         break;
    case SQL_ATTR_ROW_OPERATION_PTR:    // ->ARD
      *(SQLUSMALLINT**)rgbValue = stmt->     ard->array_status_ptr;  break;
    case SQL_ATTR_ROW_STATUS_PTR:       // ->IRD
      *(SQLUSMALLINT**)rgbValue = stmt->impl_ird. array_status_ptr;  break;
    case SQL_ATTR_ROWS_FETCHED_PTR:     // ->IRD
      *(SQLULEN     **)rgbValue = stmt->impl_ird. rows_procsd_ptr;   break;

    case SQL_ATTR_MAX_ROWS:
      *(SQLULEN*)rgbValue = stmt->max_rows;
      break;
    case SQL_ATTR_SIMULATE_CURSOR:
      *(UDWORD*)rgbValue = stmt->sim_cursor;
      break;
    case SQL_ATTR_USE_BOOKMARKS:
      *(UDWORD*)rgbValue = stmt->use_bkmarks;
      break;
  //case SQL_GET_BOOKMARK: /* CANNOT SET THIS OPTION, ODBC 2.0 */
    case SQL_ATTR_ROW_NUMBER:   /* CANNOT SET THIS OPTION, ODBC 2.0 */
      if (!stmt->has_result_set || stmt->curr_rowset_start==-1)
      { raise_err(stmt, SQL_24000);  /* Invalid cursor state */
        return SQL_ERROR;
      }
      if (stmt->row_cnt==-1)
        if (cd_Rec_cnt(stmt->cdp, stmt->cursnum, (uns32*)&stmt->row_cnt))
          stmt->row_cnt=0;
      if (stmt->db_recnum >= stmt->row_cnt)
      { raise_err(stmt, SQL_24000);  /* Invalid cursor state */
        return SQL_ERROR;
      }
      if (stmt->db_recnum >= stmt->curr_rowset_start &&
          stmt->db_recnum <  stmt->curr_rowset_start+stmt->ard->array_size)
        if (stmt->impl_ird.array_status_ptr)
        { SWORD stat = stmt->impl_ird.array_status_ptr[(unsigned)(stmt->db_recnum-stmt->curr_rowset_start)];
          if (stat == SQL_ROW_DELETED || stat == SQL_ROW_ERROR)
          { raise_err(stmt, SQL_HY109);  /* Invalid cursor position */
            return SQL_ERROR;
          }
        }
      *(SQLULEN*)rgbValue = stmt->db_recnum+1;  /* row numbers are 1-based */
      break;
  }
  return SUCCESS(stmt);
}

