// PREPARE.CPP - preparing SQL Commands and other functions prior to execution.
#include "wbdrv.h"
#pragma hdrstop

//#define DISPLAY_SIZE_NO_TOTAL
//#define COLUMN_SIZE_NO_TOTAL
///////////////////////// creting parametr & column descriptors /////////////////
SQLINTEGER SQL_type_display_size(SQLSMALLINT SQLtype, unsigned length)
// SQLtype is the concise type.
{ switch (SQLtype)
  { case SQL_CHAR: case SQL_WCHAR:         return 1;  // WB specific
    case SQL_VARCHAR:  case SQL_WVARCHAR:  return length;
    case SQL_DECIMAL:  
    case SQL_NUMERIC:  return 15+2;  // precision+2
    case SQL_BIT:      return 1;
    case SQL_SMALLINT: return 6;
    case SQL_INTEGER:  return 11;
    case SQL_TINYINT:  return 4;
    case SQL_BIGINT:   return 20;
    case SQL_REAL:
    case SQL_DOUBLE:
    case SQL_FLOAT:    return 22;
    case SQL_BINARY:   return 2*length;
    case SQL_TYPE_DATE:       case SQL_DATE:      return 10;
    case SQL_TYPE_TIME:       case SQL_TIME:      return 12;
    case SQL_TYPE_TIMESTAMP:  case SQL_TIMESTAMP: return 19;
#ifdef DISPLAY_SIZE_NO_TOTAL
    case SQL_VARBINARY:       return SQL_NO_TOTAL;
    case SQL_LONGVARCHAR:  case SQL_WLONGVARCHAR:
    case SQL_LONGVARBINARY:   return SQL_NO_TOTAL;
#else
    case SQL_VARBINARY:       return 200000; 
    case SQL_LONGVARCHAR:  case SQL_WLONGVARCHAR:
    case SQL_LONGVARBINARY:   return 2000000;
#endif
  }
  return 1;  // error
}

const char * SQL_type_name(SQLSMALLINT SQLtype)
// SQLtype is the concise type.
{ switch (SQLtype)
  { case SQL_CHAR:     return "CHAR";
    case SQL_WCHAR:    return "NCHAR";
    case SQL_VARCHAR:  return "VARCHAR";
    case SQL_WVARCHAR: return "NCHAR VARYING";
    case SQL_DECIMAL:  return "DECIMAL";
    case SQL_NUMERIC:  return "NUMERIC";
    case SQL_BIT:      return "BIT";
    case SQL_SMALLINT: return "SMALLINT";
    case SQL_INTEGER:  return "INTEGER";
    case SQL_TINYINT:  return "TINYINT";
    case SQL_BIGINT:   return "BIGINT";
    case SQL_REAL:     return "REAL";
    case SQL_DOUBLE:   return "DOUBLE PRECISION";
    case SQL_FLOAT:    return "FLOAT";
    case SQL_BINARY:   return "BINARY";
    case SQL_TYPE_DATE:       case SQL_DATE:      return "DATE";
    case SQL_TYPE_TIME:       case SQL_TIME:      return "TIME";
    case SQL_TYPE_TIMESTAMP:  case SQL_TIMESTAMP: return "TIMESTAMP";
    case SQL_LONGVARCHAR:     return "LONGVARCHAR";
    case SQL_WLONGVARCHAR:    return "NCLOB";
    case SQL_LONGVARBINARY:   return "LONGVARBINARY";
  }
  return "";  // error
}

const char * literal_prefix(int type)
{ switch (type)
  { case ATT_DATE: return "DATE\'";
    case ATT_TIME: return "TIME\'";
    case ATT_TIMESTAMP: return "TIMESTAMP\'";
    case ATT_STRING:  case ATT_TEXT:    case ATT_CHAR:
      return "\'";
    case ATT_BINARY:  case ATT_NOSPEC:  case ATT_RASTER:
      return "X\'";
   }
   return "";
}

const char * literal_suffix(int type)
{ switch (type)
  { case ATT_DATE:    case ATT_TIME:      case ATT_TIMESTAMP: 
    case ATT_STRING:  case ATT_TEXT:    case ATT_CHAR:
    case ATT_BINARY:  case ATT_NOSPEC:  case ATT_RASTER:
      return "\'";
   }
   return "";
}

BOOL define_result_set_from_td(STMT * stmt, const d_table * td)
// td==NULL: result without a result set, row count only
{ DESC * desc = &stmt->impl_ird;
  BOOL odbc2 = stmt->my_dbc->my_env->odbc_version == SQL_OV_ODBC2;
  stmt->rowbufsize=0;  /* Read_record on cursor does not read DELETED attribute */
  if (td==NULL) { desc->count=0;  return TRUE; } // no columns in the result (result with row count, without a result set)
  int rownum = td->attrcnt-1;
  desc->count=rownum;
  if (desc->drecs.acc(rownum)==NULL)  // out of memory
    return FALSE;
  for (int i=1;  i<=rownum;  i++)
  { t_desc_rec * drec = desc->drecs.acc0(i);
    const d_attr * att  = ATTR_N(td,i);
    int type=att->type;
    bool integral_type = type==ATT_INT16 || type==ATT_INT32 || type==ATT_INT8 || type==ATT_INT64 || type==ATT_MONEY;
    drec->auto_unique_value = FALSE;  // info in not in the d_table, but can be added
    strcpy(drec->base_column_name, att->name);
    if (!memcmp(drec->base_column_name, "EXPR", 4) || !memcmp(drec->base_column_name, "COUNT", 5) ||
        !memcmp(drec->base_column_name, "MIN",  3) || !memcmp(drec->base_column_name, "MAX", 3) ||
        !memcmp(drec->base_column_name, "SUM",  3) || !memcmp(drec->base_column_name, "AVG", 3))
      *drec->base_column_name=0;  // is a derived column, return empty
    strcpy(drec->base_table_name, td->selfname);
    drec->case_sensitive = type==ATT_STRING || type==ATT_TEXT || type==ATT_CHAR;
    *drec->catalog_name=0;
    type_WB_to_3(type, att->specif, drec->type, drec->concise_type, drec->datetime_int_code, odbc2);
    drec->specif=att->specif;
    drec->data_ptr=NULL;  // not defined for IRD
    drec->datetime_int_prec=0;
    drec->display_size=SQL_type_display_size(drec->concise_type, type==ATT_CHAR ? 1 : att->specif.length);
    drec->indicator_ptr=NULL;
    strcpy(drec->label, att->name);
    drec->length = type==ATT_DATE ? 10 : type==ATT_TIME ? 12 :
                   type==ATT_TIMESTAMP || type==ATT_DATIM ? 19 : 
                   type==ATT_CHAR ? (att->specif.wide_char ? 2 : 1) : att->specif.length;
    strcpy(drec->literal_prefix, literal_prefix(type));
    strcpy(drec->literal_suffix, literal_suffix(type));
    strcpy(drec->local_type_name, WB_type_name(type));
    strcpy(drec->name, att->name);
    drec->nullable=att->nullable ? SQL_NULLABLE : SQL_NO_NULLS;
    drec->num_prec_radix = type==ATT_FLOAT ? 2 : integral_type ? 10 : 0;
    drec->octet_length=SPECIFLEN(type) ? att->specif.length : WB_type_length(type);
    drec->octet_length_ptr=NULL;
    drec->precision = (uns16)WB_type_precision(type);
    drec->fixed_prec_scale = type==ATT_MONEY || integral_type && att->specif.scale!=0;  // true when has a nonzero fixed scale
    drec->scale = type==ATT_MONEY ? 2 : integral_type ? att->specif.scale : 0;
    strcpy(drec->schema_name, td->schemaname);
    drec->searchable=
      type==ATT_STRING || type==ATT_TEXT || type==ATT_CHAR ? SQL_PRED_SEARCHABLE :
      type==ATT_BOOLEAN || integral_type || type==ATT_FLOAT || 
      type==ATT_DATE || type==ATT_TIME || type==ATT_TIMESTAMP || 
      type==ATT_BINARY ? SQL_PRED_BASIC : SQL_PRED_NONE;
    strcpy(drec->base_table_name, td->selfname);
    strcpy(drec->type_name, SQL_type_name(drec->concise_type));
    drec->unnamed = SQL_NAMED;
    drec->_unsigned = !integral_type && type!=ATT_FLOAT;
    drec->updatable = att->a_flags & RI_UPDATE_FLAG ? SQL_ATTR_WRITE : SQL_ATTR_READONLY;
   // special treatment for ATT_AUTOR field not translated to string:
    if (type==ATT_AUTOR)
    { drec->type=drec->concise_type=SQL_BINARY;
      drec->precision=0;  drec->octet_length=drec->length=UUID_SIZE;
      drec->display_size=2*UUID_SIZE;
    }
   // special fields:
    if (att->multi==1)
    { if (IS_HEAP_TYPE(att->type))
        drec->offset=-1;
      else
      { drec->offset=stmt->rowbufsize;
        stmt->rowbufsize+=(unsigned)drec->octet_length;
        if (is_string(type)) att->specif.wide_char ? stmt->rowbufsize+=2 : stmt->rowbufsize++;
      }
      drec->special=FALSE;
    }
    else
    { drec->special=TRUE;
      drec->offset=-1;
    }
  }
  return TRUE;
}

static BOOL define_parameters(STMT * stmt, int * pars)
// defines total IPD from pars
{ int parnum = *pars; 
  MARKER * marker = (MARKER*)(pars+1);
  BOOL odbc2 = stmt->my_dbc->my_env->odbc_version == SQL_OV_ODBC2;
  DESC * desc = &stmt->impl_ipd;
  desc->count=parnum;
  if (desc->drecs.acc(parnum)==NULL)  // out of memory
    return FALSE;
  for (int i=1;  i<=parnum;  i++, marker++)
  { t_desc_rec * drec = desc->drecs.acc0(i);
    int type=marker->type;  t_specif specif=marker->specif;
    bool integral_type = type==ATT_INT16 || type==ATT_INT32 || type==ATT_INT8 || type==ATT_INT64 || type==ATT_MONEY;
    drec->case_sensitive = type==ATT_STRING || type==ATT_TEXT || type==ATT_CHAR;
    type_WB_to_3(type, marker->specif, drec->type, drec->concise_type, drec->datetime_int_code, odbc2);
    drec->specif=marker->specif;
    drec->data_ptr=NULL;  // not used, consistency check only
    //drec->display_size=SQL_type_display_size(drec->concise_type, specif);
    drec->length = type==ATT_DATE ? 10 : type==ATT_TIME ? 12 :
                   type==ATT_TIMESTAMP || type==ATT_DATIM ? 19 : 
                   type==ATT_CHAR ? 1 : specif.length;
    strcpy(drec->local_type_name, WB_type_name(type));
    drec->name[0]=0;  // named parameters not supported
    drec->nullable=SQL_NULLABLE;  // always nullable
    drec->num_prec_radix = type==ATT_FLOAT ? 2 : integral_type ? 10 : 0;
    drec->octet_length=SPECIFLEN(type) ? specif.length : WB_type_length(type);
    drec->parameter_type = marker->input_output<=MODE_IN  ? SQL_PARAM_INPUT :
                           marker->input_output==MODE_OUT ? SQL_PARAM_OUTPUT : SQL_PARAM_INPUT_OUTPUT;
    drec->precision = (short)WB_type_precision(type);
    drec->fixed_prec_scale = type==ATT_MONEY || integral_type && marker->specif.scale!=0;  // true when has a nonzero fixed scale
    drec->scale = type==ATT_MONEY ? 2 : integral_type ? marker->specif.scale : 0;
    strcpy(drec->type_name, SQL_type_name(drec->concise_type));
    drec->unnamed = SQL_UNNAMED;
    drec->_unsigned = !integral_type && type!=ATT_FLOAT;
  }
  stmt->param_info_retrieved=TRUE;  stmt->impl_ipd.populated=TRUE;
  return TRUE;
}

void define_catalog_result_set(STMT * stmt, tcursnum curs)
/* Sets new rowbufsize and resatrs for catalog function result in curs. */
{ const d_table * td = cd_get_table_d(stmt->cdp, curs, CATEG_DIRCUR);
  define_result_set_from_td(stmt, td);
  if (td!=NULL) release_table_d(td);
 // simulate "prepare":
  stmt->impl_ipd.count=0;   
  stmt->param_info_retrieved=TRUE;  stmt->impl_ipd.populated=TRUE;
  stmt->is_prepared=TRUE;
}

void un_prepare(STMT * stmt)  // DROPs statement handle in the server
{ if (stmt->prep_handle)
    cd_SQL_drop(stmt->cdp, stmt->prep_handle);
  stmt->prep_handle=0;
  stmt->param_info_retrieved=FALSE;   stmt->impl_ipd.populated=FALSE;
  stmt->is_prepared=FALSE;
}

//	Perform a Prepare on the SQL statement

SQLRETURN SQL_API SQLPrepareW(SQLHSTMT StatementHandle, SQLWCHAR *StatementText, SQLINTEGER TextLength)
// Parameters checked by DM. StatementText!=NULL, cbSqlStr>0 or SQL_NTS
/* has_result_set==FALSE on entry */
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (stmt->has_result_set)  /* previous result set must be closed before */
  { raise_err(stmt, SQL_24000);  /* Invalid cursor state */
    return SQL_ERROR;
  }
  wide2ansi xStmt(StatementText, TextLength, stmt->cdp->sys_charset);
 /* un-prepare the old statement first: */
  un_prepare(stmt);
 /* compile statements and load result/param info: */
  d_table * td;  int * pars = NULL;
  cd_start_package(stmt->cdp);
  cd_SQL_prepare(stmt->cdp, xStmt.get_string(), &stmt->prep_handle);
  cd_SQL_get_result_set_info(stmt->cdp, stmt->prep_handle, 0, &td);
  // When stmt->auto_ipd is false then retreiving parameter information is not required. 
  // It is useful to do this because SQLBindParameter can ignore some of its parameterts then.
  //if (stmt->auto_ipd)
    cd_SQL_get_param_info(stmt->cdp, stmt->prep_handle, (void**)&pars);
  cd_send_package(stmt->cdp);
  if (cd_Sz_error(stmt->cdp))  // pars and td not allocated
  { raise_db_err(stmt);  // must be before SQL_drop which clears the error number!
    cd_SQL_drop(stmt->cdp, stmt->prep_handle); // may or may not be prepared but handle is allocated
    stmt->prep_handle=0;  
    return SQL_ERROR; 
  }
  //if (stmt->auto_ipd) 
    { define_parameters(stmt, pars);  corefree(pars); }
 // define the preliminary result set:
  define_result_set_from_td(stmt, td);  corefree(td);
 /* enter the "prepared" state: */
  stmt->is_prepared=TRUE;
  return SQL_SUCCESS;
}

////////////////////////////////// cursor names /////////////////////////////////
// Cursor name persists until statement is dropped!

static BOOL name_duplicity(tptr cname, STMT * stmt)
{ STMT * astmt = stmt->next_stmt;
  while (astmt != stmt)
  { if (!stricmp(cname, astmt->cursor_name)) return TRUE;
    astmt=astmt->next_stmt;
  }
  return FALSE;
}

void assign_cursor_name(STMT * stmt)
// Called when executing a SELECT statement if name not assigned 
// and when retrieving undefined name 
{ unsigned counter;
  strcpy(stmt->cursor_name, "SQL_CUR");
  counter=0;
  do int2str(counter++, stmt->cursor_name+7);
  while (name_duplicity(stmt->cursor_name, stmt));
}

//	Set the cursor name on a statement handle
SQLRETURN SQL_API SQLSetCursorNameW(SQLHSTMT hstmt,SQLWCHAR *szCursor, SQLSMALLINT cbCursor)
// DM checks that CursorName is not a NULL pointer and NameLenght is valid.
{ if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)hstmt;
  CLEAR_ERR(stmt);
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (stmt->has_result_set) /* there is a pending result set */
  { raise_err(stmt, SQL_24000);  /* Invalid cursor state */
    return SQL_ERROR;
  }
#if 0
  if (NameLength==SQL_NTS) NameLength=strlen((tptr)CursorName);
 /* name syntax test: */
  while (NameLength && *CursorName==' ') { CursorName++;  NameLength--; }
  while (NameLength && CursorName[NameLength-1]==' ')     NameLength--; 
  if    (NameLength && *CursorName=='`') { CursorName++;  NameLength--; }
  if    (NameLength && CursorName[NameLength-1]=='`')     NameLength--; 
  BOOL err;  tobjname cname; 
  if (NameLength > OBJ_NAME_LEN || !NameLength) err=TRUE;
  else
  { memcpy(cname, CursorName, NameLength);  cname[NameLength]=0;
    Upcase(cname);
    err = !UPLETTER(cname[0]);
    for (int i=1;  i<NameLength;  i++)  
      if (!SYMCHAR(cname[i])) { err=TRUE;  break; }
    if (!memcmp(cname, "SQLCUR", 6) || !memcmp(cname, "SQL_CUR", 7)) err=TRUE;
  }
  if (err)
  { raise_err(stmt, SQL_34000);  /* Invalid cursor name */
    return SQL_ERROR;
  }
#endif
  wide2ansi xCurs(szCursor, cbCursor, stmt->cdp->sys_charset);
  if (xCurs.get_len() > OBJ_NAME_LEN)
  { raise_err(stmt, SQL_34000);  /* Invalid cursor name */
    return SQL_ERROR;
  }
 /* duplicity test: */
  if (name_duplicity(xCurs.get_string(), stmt))
  { raise_err(stmt, SQL_3C000);  /* Duplicate cursor name */
    return SQL_ERROR;
  }
 /* write cursor name: */
  strcpy(stmt->cursor_name, xCurs.get_string());
  return SQL_SUCCESS;
}

//	Return the cursor name for a statement handle
SQLRETURN SQL_API SQLGetCursorNameW(SQLHSTMT hstmt, SQLWCHAR *szCursor, SQLSMALLINT cbCursorMax, SQLSMALLINT *pcbCursor)
// DM checks BufferLength>=0.
{ if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)hstmt;
  CLEAR_ERR(stmt);
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
 // in ODBC 3.0, if no name has been assigned (impl. or expl.), a name is created
  if (!*stmt->cursor_name) assign_cursor_name(stmt);
 // write the name:
  if (write_string_wide_b(stmt->cursor_name, (SQLWCHAR*)szCursor, cbCursorMax, pcbCursor))
  { raise_err(stmt, SQL_01004);  /* Data truncated */
    return SQL_SUCCESS_WITH_INFO;
  }
  return SQL_SUCCESS;
}

/////////////////////////////// descriptors /////////////////////////////////////
SQLRETURN  SQL_API SQLGetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
           SQLPOINTER Value, SQLINTEGER BufferLength,
           SQLINTEGER *StringLength)
{ if (!IsValidHDESC(DescriptorHandle)) return SQL_INVALID_HANDLE;
  DESC * desc = (DESC*)DescriptorHandle;
  CLEAR_ERR(desc);
  //if (Value==NULL) return SQL_SUCCESS;
  BOOL truncated=FALSE;
  switch (FieldIdentifier)
  { case SQL_DESC_ALLOC_TYPE:      // read only
      *(SQLSMALLINT  *)Value = desc->is_implicit ? SQL_DESC_ALLOC_AUTO : SQL_DESC_ALLOC_USER;
      if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
    case SQL_DESC_ARRAY_SIZE:      // application only 
      if (desc->is_implem) goto err;
      *(SQLULEN      *)Value = desc->array_size;        
      if (StringLength!=NULL) *StringLength=sizeof(SQLULEN);  break;
    case SQL_DESC_ARRAY_STATUS_PTR:
      *(SQLUSMALLINT**)Value = desc->array_status_ptr;  
      if (StringLength!=NULL) *StringLength=sizeof(SQLUSMALLINT*);  break;
    case SQL_DESC_BIND_OFFSET_PTR: // application only
      if (desc->is_implem) goto err;
      *(SQLINTEGER  **)Value = desc->bind_offset_ptr;   
      if (StringLength!=NULL) *StringLength=sizeof(SQLINTEGER*);  break;
    case SQL_DESC_BIND_TYPE:       // application only
      if (desc->is_implem) goto err;
      *(SQLUINTEGER  *)Value = desc->bind_type;       
      if (StringLength!=NULL) *StringLength=sizeof(SQLUINTEGER);  break;
    case SQL_DESC_COUNT:       
#ifdef X64ARCH
      *(SQLULEN      *)Value = desc->count;            
      if (StringLength!=NULL) *StringLength=sizeof(SQLULEN);  break;
#else
      *(SQLSMALLINT  *)Value = desc->count;            
      if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
#endif
    case SQL_DESC_ROWS_PROCESSED_PTR: // implementation only
      if (!desc->is_implem) goto err;
      *(SQLULEN**)Value = desc->rows_procsd_ptr;   
      if (StringLength!=NULL) *StringLength=sizeof(SQLINTEGER*);  break;
    default:
    { if (RecNumber<0 || !RecNumber && desc->is_param) 
        { raise_err_desc(desc, SQL_07009);  return SQL_ERROR; }
      if (RecNumber > desc->count) return SQL_NO_DATA;
      for (int i=desc->drecs.count();  i<=RecNumber;  i++)
      { t_desc_rec * drec = desc->drecs.acc(i);
        if (drec==NULL) { raise_err_desc(desc, SQL_HY001);  return SQL_ERROR; }
        drec->init();
      }
      t_desc_rec * drec = desc->drecs.acc0(RecNumber);
      switch (FieldIdentifier)
      {// read-only IRD or IRD+IPD fields:
        case SQL_DESC_AUTO_UNIQUE_VALUE:
          if (!desc->is_implem || !desc->is_row) goto err;
          *(SQLINTEGER *)Value = drec->auto_unique_value ? SQL_TRUE : SQL_FALSE;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLINTEGER);  break;
        case SQL_DESC_BASE_COLUMN_NAME:
          if (!desc->is_implem || !desc->is_row) goto err;
          truncated=write_string_wide_b4(drec->base_column_name, (SQLWCHAR*)Value, BufferLength, StringLength, desc->my_dbc->cdp->sys_charset);  break;
        case SQL_DESC_BASE_TABLE_NAME:
          if (!desc->is_implem || !desc->is_row) goto err;
          truncated=write_string_wide_b4(drec->base_table_name,  (SQLWCHAR*)Value, BufferLength, StringLength, desc->my_dbc->cdp->sys_charset);  break;
        case SQL_DESC_CASE_SENSITIVE:
          if (!desc->is_implem) goto err;
          if (desc->is_param && !desc->populated) goto err;
          *(SQLINTEGER *)Value = drec->case_sensitive    ? SQL_TRUE : SQL_FALSE;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLINTEGER);  break;
        case SQL_DESC_CATALOG_NAME:
          if (!desc->is_implem || !desc->is_row) goto err;
          truncated=write_string_wide_b4(drec->catalog_name,     (SQLWCHAR*)Value, BufferLength, StringLength);  break;
        case SQL_DESC_DISPLAY_SIZE:
          if (!desc->is_implem || !desc->is_row) goto err;
          *(SQLLEN *)Value = drec->display_size; 
          if (StringLength!=NULL) *StringLength=sizeof(SQLLEN);  break;
        case SQL_DESC_FIXED_PREC_SCALE:  
          if (!desc->is_implem) goto err;
          if (desc->is_param && !desc->populated) goto err;
          *(SQLSMALLINT*)Value = drec->fixed_prec_scale  ? SQL_TRUE : SQL_FALSE;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_LABEL:
          if (!desc->is_implem || !desc->is_row) goto err;
          truncated=write_string_wide_b4(drec->label,            (SQLWCHAR*)Value, BufferLength, StringLength, desc->my_dbc->cdp->sys_charset);  break;
        case SQL_DESC_LITERAL_PREFIX:
          if (!desc->is_implem || !desc->is_row) goto err;
          truncated=write_string_wide_b4(drec->literal_prefix,   (SQLWCHAR*)Value, BufferLength, StringLength);  break;
        case SQL_DESC_LITERAL_SUFFIX:
          if (!desc->is_implem || !desc->is_row) goto err;
          truncated=write_string_wide_b4(drec->literal_suffix,   (SQLWCHAR*)Value, BufferLength, StringLength);  break;
        case SQL_DESC_LOCAL_TYPE_NAME:
          if (!desc->is_implem) goto err;
          if (desc->is_param && (!desc->populated || !RecNumber)) goto err;
          truncated=write_string_wide_b4(drec->local_type_name,  (SQLWCHAR*)Value, BufferLength, StringLength);  break;
        case SQL_COLUMN_NULLABLE:  // ODBC 2.0 field, must be supported
        case SQL_DESC_NULLABLE:
          if (!desc->is_implem) goto err;
          *(SQLSMALLINT*)Value = drec->nullable;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_SCHEMA_NAME:
          if (!desc->is_implem || !desc->is_row) goto err;
          truncated=write_string_wide_b4(drec->schema_name,      (SQLWCHAR*)Value, BufferLength, StringLength, desc->my_dbc->cdp->sys_charset);  break;
        case SQL_DESC_SEARCHABLE:
          if (!desc->is_implem || !desc->is_row) goto err;
          *(SQLSMALLINT*)Value = drec->searchable;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_TABLE_NAME:
          if (!desc->is_implem || !desc->is_row) goto err;
          truncated=write_string_wide_b4(drec->table_name,       (SQLWCHAR*)Value, BufferLength, StringLength, desc->my_dbc->cdp->sys_charset);  break;
        case SQL_DESC_TYPE_NAME:
          if (!desc->is_implem) goto err;
          if (desc->is_param && !desc->populated) goto err;
          truncated=write_string_wide_b4(drec->type_name,        (SQLWCHAR*)Value, BufferLength, StringLength);  break;
        case SQL_DESC_UNSIGNED:
          if (!desc->is_implem) goto err;
          if (desc->is_param && !desc->populated) goto err;
          *(SQLSMALLINT*)Value = drec->_unsigned         ? SQL_TRUE : SQL_FALSE;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_UPDATABLE:
          if (!desc->is_implem || !desc->is_row) goto err;
          *(SQLSMALLINT*)Value = drec->updatable; 
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
       // implementation only, read-write:
        case SQL_DESC_NAME:
          if (!desc->is_implem) goto err;
          truncated=write_string_wide_b4(drec->name,             (SQLWCHAR*)Value, BufferLength, StringLength);  break;
        case SQL_DESC_UNNAMED:
          if (!desc->is_implem) goto err;
          *(SQLSMALLINT*)Value = drec->unnamed;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_PARAMETER_TYPE:  // IPD only
          if (!desc->is_implem || !desc->is_param) goto err;
          *(SQLSMALLINT*)Value = drec->parameter_type;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
       // read-write fields:
        case SQL_DESC_TYPE:
          *(SQLSMALLINT*)Value = drec->type;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_CONCISE_TYPE:
          *(SQLSMALLINT*)Value = drec->concise_type;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_DATETIME_INTERVAL_CODE:
          *(SQLSMALLINT*)Value = drec->datetime_int_code;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_COLUMN_LENGTH:     // ODBC 2.0 field, must be supported
           switch (drec->concise_type)  // used by the cursor library even in ODBC 3.0
           { case SQL_CHAR:  
               *(SQLULEN*)Value=1;  break;  // WB specif
             case SQL_WCHAR:  
               *(SQLULEN*)Value=2;  break;  // WB specif
             case SQL_VARCHAR:  case SQL_WVARCHAR:  case SQL_BINARY:  case SQL_VARBINARY: 
               *(SQLULEN*)Value=drec->length;  break;
             case SQL_BIT:  case SQL_TINYINT:
               *(SQLULEN*)Value=1;  break;
             case SQL_SMALLINT:
               *(SQLULEN*)Value=2;  break;
             case SQL_INTEGER:
               *(SQLULEN*)Value=4;  break;
             case SQL_REAL:  case SQL_FLOAT:  case SQL_DOUBLE:  case SQL_BIGINT:
               *(SQLULEN*)Value=8;  break;
             case SQL_LONGVARCHAR:  case SQL_WLONGVARCHAR:  case SQL_LONGVARBINARY:  
#ifdef COLUMN_SIZE_NO_TOTAL
               *(SQLULEN*)Value=SQL_NO_TOTAL;  
#else
               *(SQLULEN*)Value=2000000;
#endif
               break;
             case SQL_NUMERIC:  case SQL_DECIMAL:
               *(SQLULEN*)Value=17;  break;
             case SQL_TYPE_DATE:       case SQL_DATE:
               *(SQLULEN*)Value=10;  break;
             case SQL_TYPE_TIME:       case SQL_TIME:
               *(SQLULEN*)Value=12;  break;
             case SQL_TYPE_TIMESTAMP:  case SQL_TIMESTAMP:
               *(SQLULEN*)Value=19;  break;
           }
           if (StringLength!=NULL) *StringLength=sizeof(SQLULEN);  break;
        case SQL_DESC_LENGTH:
          *(SQLULEN*)Value = drec->length;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLULEN);  break;
        case SQL_COLUMN_PRECISION:  // ODBC 2.0 field, must be supported
          switch (drec->concise_type)
          { case SQL_BIT:      *(SQLSMALLINT*)Value=1;
            case SQL_CHAR:     *(SQLSMALLINT*)Value=1;
            case SQL_WCHAR:    *(SQLSMALLINT*)Value=1;
            case SQL_SMALLINT: *(SQLSMALLINT*)Value=5;
            case SQL_INTEGER:  *(SQLSMALLINT*)Value=10;
            case SQL_TINYINT:  *(SQLSMALLINT*)Value=3;
            case SQL_BIGINT:   *(SQLSMALLINT*)Value=19;
            case SQL_NUMERIC:    case SQL_DECIMAL:  *(SQLSMALLINT*)Value=15;
            case SQL_REAL:       case SQL_FLOAT:  case SQL_DOUBLE:  
                               *(SQLSMALLINT*)Value=15;
            case SQL_DATE:       case SQL_TYPE_DATE:*(SQLSMALLINT*)Value=10;
            case SQL_TIME:       case SQL_TYPE_TIME:*(SQLSMALLINT*)Value=8;
            case SQL_TIMESTAMP:  case SQL_TYPE_TIMESTAMP: *(SQLSMALLINT*)Value=19;
            case SQL_VARCHAR:    case SQL_WVARCHAR:    case SQL_BINARY:  case SQL_VARBINARY:
            case SQL_LONGVARCHAR:case SQL_WLONGVARCHAR:case SQL_LONGVARBINARY:  
              *(SQLSMALLINT*)Value=(SQLSMALLINT)drec->length;  break;
          }
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_PRECISION:
          *(SQLSMALLINT*)Value = drec->precision;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_NUM_PREC_RADIX:
          *(SQLINTEGER *)Value = drec->num_prec_radix;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLINTEGER);  break;
        case SQL_COLUMN_SCALE:      // ODBC 2.0 field, must be supported
        case SQL_DESC_SCALE:
          *(SQLSMALLINT*)Value = drec->scale; 
          if (StringLength!=NULL) *StringLength=sizeof(SQLSMALLINT);  break;
        case SQL_DESC_DATETIME_INTERVAL_PRECISION:
          *(SQLINTEGER *)Value = drec->datetime_int_prec;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLINTEGER);  break;
        case SQL_DESC_OCTET_LENGTH:
          *(SQLLEN *)Value = drec->octet_length;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLLEN);  break;
        case SQL_DESC_OCTET_LENGTH_PTR:  // application only
          if (desc->is_implem) goto err;
          *(SQLLEN**)Value = drec->octet_length_ptr;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLINTEGER);  break;
        case SQL_DESC_INDICATOR_PTR:     // application only
          if (desc->is_implem) goto err;
          *(SQLLEN**)Value = drec->indicator_ptr;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLINTEGER);  break;
        case SQL_DESC_DATA_PTR:          // application nad IPD check
          if (desc->is_implem) goto err;
          *(SQLPOINTER *)Value = drec->data_ptr;  
          if (StringLength!=NULL) *StringLength=sizeof(SQLPOINTER);  break;

        default: goto err;
      } // switch
    } // record field
  } // switch
  if (truncated)
  { raise_err_desc(desc, SQL_01004); // string right truncated
    return SQL_SUCCESS_WITH_INFO;
  }
  return SQL_SUCCESS;
 err:
  raise_err_desc(desc, SQL_HY091);  return SQL_ERROR;
}

void get_string(tptr dest, unsigned destsize, SQLPOINTER Value, SQLINTEGER BufferLength)
{ wide2ansi xVal((SQLWCHAR*)Value, BufferLength);
  strmaxcpy(dest, xVal.get_string(), destsize);
}

SQLRETURN  SQL_API SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
           SQLPOINTER Value, SQLINTEGER BufferLength)
{ if (!IsValidHDESC(DescriptorHandle)) return SQL_INVALID_HANDLE;
  DESC * desc = (DESC*)DescriptorHandle;
  CLEAR_ERR(desc);
  if (desc->is_row && desc->is_implem) // cannot modify most fields
    if (FieldIdentifier!=SQL_DESC_ARRAY_STATUS_PTR && FieldIdentifier!=SQL_DESC_ROWS_PROCESSED_PTR)
      { raise_err_desc(desc, SQL_HY016);  return SQL_ERROR; }
  switch (FieldIdentifier)
  { case SQL_DESC_ARRAY_SIZE:      // application only 
      if (desc->is_implem) goto err;
      desc->array_size       = (SQLUINTEGER  )(size_t)Value;  break;
    case SQL_DESC_ARRAY_STATUS_PTR:
      desc->array_status_ptr = (SQLUSMALLINT*)Value;  break;
    case SQL_DESC_BIND_OFFSET_PTR: // application only
      if (desc->is_implem) goto err;
      desc->bind_offset_ptr  = (SQLINTEGER  *)Value;  break;
    case SQL_DESC_BIND_TYPE:       // application only
      if (desc->is_implem) goto err;
      desc->bind_type        = (SQLUINTEGER  )(size_t)Value;  break;
    case SQL_DESC_COUNT:           // Set disabled for IRD
    { SQLSMALLINT new_count  = (SQLSMALLINT  )(size_t)Value;
      for (int i=desc->count+1;  i<=new_count;  i++)
      { t_desc_rec * drec = desc->drecs.acc(i);
        if (drec==NULL) { raise_err_desc(desc, SQL_HY001);  return SQL_ERROR; }
        drec->init();
      }
      desc->count = new_count;           
      break;
    }
    case SQL_DESC_ROWS_PROCESSED_PTR: // implementation only
      if (!desc->is_implem) goto err;
      desc->rows_procsd_ptr  = (SQLULEN*)Value;  break;

    default:
    { if (RecNumber<0 || !RecNumber && desc->is_param) 
        { raise_err_desc(desc, SQL_07009);  return SQL_ERROR; }
      for (int i=desc->count+1;  i<=RecNumber;  i++)
      { t_desc_rec * drec = desc->drecs.acc(i);
        if (drec==NULL) { raise_err_desc(desc, SQL_HY001);  return SQL_ERROR; }
        drec->init();
      }
      if (RecNumber > desc->count) desc->count=RecNumber;
      t_desc_rec * drec = desc->drecs.acc0(RecNumber);
      switch (FieldIdentifier)
      {// implementation only:
        case SQL_DESC_NAME:
          if (!desc->is_implem) goto err;
          get_string(drec->name, sizeof(drec->name), Value, BufferLength);  break;
        case SQL_DESC_UNNAMED:
          if (!desc->is_implem) goto err;
          drec->unnamed           = (SQLSMALLINT)(size_t)Value;  break;
        case SQL_DESC_PARAMETER_TYPE:  // IPD only
          if (!desc->is_implem || !desc->is_param) goto err;
          drec->parameter_type    = (SQLSMALLINT)(size_t)Value;  break;
       // general fields:
        case SQL_DESC_TYPE:
          drec->type              = (SQLSMALLINT)(size_t)Value;  break;
        case SQL_DESC_CONCISE_TYPE:
          drec->concise_type      = (SQLSMALLINT)(size_t)Value;  break;
        case SQL_DESC_DATETIME_INTERVAL_CODE:
          drec->datetime_int_code = (SQLSMALLINT)(size_t)Value;  break;
        case SQL_DESC_LENGTH:
          drec->length            = (SQLUINTEGER)(size_t)Value;  break;
        case SQL_DESC_PRECISION:
          drec->precision         = (SQLSMALLINT)(size_t)Value;  break;
        case SQL_DESC_NUM_PREC_RADIX:
          drec->num_prec_radix    = (SQLINTEGER) (size_t)Value;  break;
        case SQL_DESC_SCALE:
          drec->scale             = (SQLSMALLINT)(size_t)Value;  break;
        case SQL_DESC_DATETIME_INTERVAL_PRECISION:
          drec->datetime_int_prec = (SQLINTEGER) (size_t)Value;  break;
        case SQL_DESC_OCTET_LENGTH:
          drec->octet_length      = (SQLLEN)     (size_t)Value;  break;
        case SQL_DESC_OCTET_LENGTH_PTR:  // application only
          if (desc->is_implem) goto err;
          drec->octet_length_ptr  = (SQLLEN*)Value;  break;
        case SQL_DESC_INDICATOR_PTR:     // application only
          if (desc->is_implem) goto err;
          drec->indicator_ptr     = (SQLLEN*)Value;  break;
        case SQL_DESC_DATA_PTR:          // application nad IPD check
          if (desc->is_implem && desc->is_row) goto err;
          drec->data_ptr          = (SQLPOINTER) Value;  
         // implicit "count" resetting when unbinding:
          if (Value==NULL && desc->count == RecNumber)
            while (desc->count && desc->drecs.acc0(desc->count)->data_ptr==NULL)
              desc->count--;
          break;

        default: goto err;
      } // switch
    } // record field
  } // switch
  return SQL_SUCCESS;
 err:
   raise_err_desc(desc, SQL_HY091);  return SQL_ERROR;
}

SQLRETURN  SQL_API SQLGetDescRecW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLWCHAR *Name,
           SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
           SQLSMALLINT *Type, SQLSMALLINT *SubType, 
           SQLLEN *Length, SQLSMALLINT *Precision, 
           SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{ if (!IsValidHDESC(DescriptorHandle)) return SQL_INVALID_HANDLE;
  DESC * desc = (DESC*)DescriptorHandle;
  CLEAR_ERR(desc);
  if (RecNumber<0 || !RecNumber && desc->is_param && desc->is_implem) 
    { raise_err_desc(desc, SQL_07009);  return SQL_ERROR; }
  if (RecNumber > desc->count) return SQL_NO_DATA;
  t_desc_rec * drec = desc->drecs.acc0(RecNumber);
  BOOL truncated=write_string_wide_b(drec->name, Name, BufferLength, StringLength); 
  if (Type     !=NULL) *Type      = drec->type;  
  if (SubType  !=NULL) *SubType   = drec->datetime_int_code;
  if (Length   !=NULL) *Length    = drec->octet_length;
  if (Precision!=NULL) *Precision = drec->precision;
  if (Scale    !=NULL) *Scale     = drec->scale;
  if (Nullable !=NULL) *Nullable  = drec->nullable;
  if (truncated)
  { raise_err_desc(desc, SQL_01004); // string right truncated
    return SQL_SUCCESS_WITH_INFO;
  }
  return SQL_SUCCESS;
}

SQLRETURN  SQL_API SQLSetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLSMALLINT Type,
           SQLSMALLINT SubType, SQLLEN Length,
           SQLSMALLINT Precision, SQLSMALLINT Scale,
           SQLPOINTER Data, SQLLEN *StringLength,
           SQLLEN *Indicator)
{ if (!IsValidHDESC(DescriptorHandle)) return SQL_INVALID_HANDLE;
  DESC * desc = (DESC*)DescriptorHandle;
  CLEAR_ERR(desc);
  if (desc->is_row && desc->is_implem) // cannot modify record fields
    { raise_err_desc(desc, SQL_HY016);  return SQL_ERROR; }
  if (RecNumber<0) { raise_err_desc(desc, SQL_07009);  return SQL_ERROR; }
  for (int i=desc->count+1;  i<=RecNumber;  i++)
  { t_desc_rec * drec = desc->drecs.acc(i);
    if (drec==NULL) { raise_err_desc(desc, SQL_HY001);  return SQL_ERROR; }
    drec->init();
  }
  if (RecNumber > desc->count) desc->count=RecNumber;
  t_desc_rec * drec = desc->drecs.acc0(RecNumber);
  drec->type              = Type;  
  drec->datetime_int_code = SubType;
  if (desc->is_implem) 
    drec->length            = Length;  // specified in Release notes!
  else
    drec->octet_length      = Length;
  drec->precision         = Precision;
  drec->scale             = Scale;
  drec->data_ptr          = Data;  
  drec->octet_length_ptr  = StringLength;  
  drec->indicator_ptr     = Indicator;  
  return SQL_SUCCESS;
}

SQLRETURN  SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{ if (!IsValidHDESC(SourceDescHandle)) return SQL_INVALID_HANDLE;
  if (!IsValidHDESC(TargetDescHandle)) return SQL_INVALID_HANDLE;
  DESC * sdesc = (DESC*)SourceDescHandle;
  DESC * tdesc = (DESC*)TargetDescHandle;
  CLEAR_ERR(sdesc);  CLEAR_ERR(tdesc);
  if (tdesc->drecs.acc(sdesc->count) == NULL)
    { raise_err_desc(tdesc, SQL_HY001);  return SQL_ERROR; }
  tdesc->array_size      =sdesc->array_size;
  tdesc->array_status_ptr=sdesc->array_status_ptr;
  tdesc->bind_offset_ptr =sdesc->bind_offset_ptr;
  tdesc->bind_type       =sdesc->bind_type;
  tdesc->count           =sdesc->count;
  tdesc->rows_procsd_ptr =sdesc->rows_procsd_ptr;
  memcpy(tdesc->drecs.acc0(0), sdesc->drecs.acc0(0), (sdesc->count+1)*sizeof(t_desc_rec));
  return SQL_SUCCESS;
}

////////////////////////// concise function on params ////////////////////////////
int t_desc_rec::column_size(void)
// Precision or length, depending on type.
{ switch (concise_type)
  { case SQL_BIT:            return 1;
    case SQL_TINYINT:        return 3;
    case SQL_SMALLINT:       return 5;
    case SQL_INTEGER:        return 10;
    case SQL_BIGINT:         return 19;
    case SQL_REAL:           return 15;
    case SQL_FLOAT:          return 15;
    case SQL_DOUBLE:         return 15;
    case SQL_TYPE_DATE:      case SQL_DATE:      return 10;
    case SQL_TYPE_TIME:      case SQL_TIME:      return 12;
    case SQL_TYPE_TIMESTAMP: case SQL_TIMESTAMP: return 19;
    case SQL_DECIMAL:
    case SQL_NUMERIC:        return precision; // 15
    case SQL_CHAR:     case SQL_WCHAR:
    case SQL_VARCHAR:  case SQL_WVARCHAR:
    case SQL_BINARY:         return (int)length;
#ifdef COLUMN_SIZE_NO_TOTAL
    case SQL_VARBINARY:      return SQL_NO_TOTAL;
    case SQL_LONGVARCHAR:  case SQL_WLONGVARCHAR:
    case SQL_LONGVARBINARY:  return SQL_NO_TOTAL;
#else
    case SQL_VARBINARY:      return 200000;
    case SQL_LONGVARCHAR:  case SQL_WLONGVARCHAR:
    case SQL_LONGVARBINARY:  return 2000000;
#endif
  }
  return 0; // error
}

int t_desc_rec::decimal_digits(void)
// Precision or scale, depending on type
{ if (concise_type==SQL_INTEGER || concise_type==SQL_SMALLINT ||
      concise_type==SQL_TINYINT || concise_type==SQL_BIGINT)   return 0; // ==scale
  if (concise_type==SQL_NUMERIC || concise_type==SQL_DECIMAL ) return 2; // ==scale
  if (concise_type==SQL_TYPE_TIME || concise_type==SQL_TIME) return 3; // ==precision
  if (concise_type==SQL_TYPE_DATE || concise_type==SQL_TYPE_TIMESTAMP ||
      concise_type==SQL_DATE      || concise_type==SQL_TIMESTAMP) 
    return 0; // ==precision
  return 0;  // value N/A
}

//	Returns the number of parameter markers.

SQLRETURN SQL_API SQLNumParams(SQLHSTMT StatementHandle, SQLSMALLINT *ParameterCountPtr)
/* Can be executed async., but statement must be prepared. */
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (!stmt->is_prepared)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (ParameterCountPtr!=NULL) // ODBC test DLL uses NULL value (no error expected)!
    *ParameterCountPtr=stmt->impl_ipd.count;
  return SQL_SUCCESS;
}

//	Returns the description of a parameter marker.

SQLRETURN SQL_API SQLDescribeParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, 
  SQLSMALLINT *DataTypePtr,	SQLULEN *ParameterSizePtr, SQLSMALLINT *DecimalDigitsPtr, SQLSMALLINT *NullablePtr)
/* Statement must be prepared. */
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (!stmt->is_prepared)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (!stmt->param_info_retrieved)
  { int * pars;
    if (cd_SQL_get_param_info(stmt->cdp, stmt->prep_handle, (void**)&pars))
      { raise_db_err(stmt);  return SQL_ERROR; } // pars not allocated
    if (!define_parameters(stmt, pars))
      { raise_err(stmt, SQL_HY001);  corefree(pars);  return SQL_ERROR; }
    corefree(pars);  
  }
  if (ParameterNumber > stmt->impl_ipd.count)
  { raise_err(stmt, SQL_07009);  /* Invalid parameter number */
    return SQL_ERROR;
  }
  t_desc_rec * drec=stmt->impl_ipd.drecs.acc(ParameterNumber);
  if (DataTypePtr     !=NULL) *DataTypePtr     =drec->concise_type;
  if (ParameterSizePtr!=NULL) *ParameterSizePtr=drec->column_size();
  if (DecimalDigitsPtr!=NULL) *DecimalDigitsPtr=drec->decimal_digits();
  if (NullablePtr     !=NULL) *NullablePtr     =drec->nullable;   // allways SQL_NULLABLE
  return SQL_SUCCESS;
}

//	-	-	-	-	-	-	-	-	-

//	Set parameters on a statement handle

SQLRETURN SQL_API SQLBindParameter(SQLHSTMT StatementHandle,
 SQLUSMALLINT ipar, SQLSMALLINT InputOutputType, SQLSMALLINT ValueType, SQLSMALLINT ParameterType,
 SQLULEN ColumnSize, SQLSMALLINT DecimalDigits, SQLPOINTER ParameterValue,
 SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;   
  CLEAR_ERR(stmt);
 /* check parameters: */
  if (DecimalDigits < 0)
  { raise_err(stmt, SQL_HY104);  /* Invalid scale value */
    return SQL_ERROR;
  }
 // allocate descriptor records:
  int i;  t_desc_rec * drec;
  for (i=stmt->apd->drecs.count();  i<=ipar;  i++)
  { drec = stmt->apd->drecs.acc(i);
    if (drec==NULL) { raise_err(stmt, SQL_HY001);  return SQL_ERROR; }
    drec->init();
  }
  if (stmt->apd->count < ipar) stmt->apd->count = ipar;
  for (i=stmt->impl_ipd.drecs.count();  i<=ipar;  i++)
  { drec = stmt->impl_ipd.drecs.acc(i);
    if (drec==NULL) { raise_err(stmt, SQL_HY001);  return SQL_ERROR; }
    drec->init();
  }
  if (stmt->impl_ipd.count < ipar) stmt->impl_ipd.count = ipar;
 // store APD info: 
  drec = stmt->apd->drecs.acc0(ipar);
  if (ValueType==SQL_C_DEFAULT) ValueType=default_C_type(ParameterType);
  drec->concise_type=ValueType;
  switch (ValueType)
  { case SQL_C_TYPE_DATE:        case SQL_C_DATE:     
      drec->type=SQL_DATETIME;  drec->datetime_int_code=SQL_CODE_DATE;  break;
    case SQL_C_TYPE_TIME:        case SQL_C_TIME:    
      drec->type=SQL_DATETIME;  drec->datetime_int_code=SQL_CODE_TIME;  break;
    case SQL_C_TYPE_TIMESTAMP:   case SQL_C_TIMESTAMP:
      drec->type=SQL_DATETIME;  drec->datetime_int_code=SQL_CODE_TIMESTAMP;  break;
    case SQL_C_NUMERIC:
      drec->precision=15;  drec->scale=0;  // setting defaults, cont.
    default:
      drec->type=ValueType;     drec->datetime_int_code=0;  break;
  }
  drec->data_ptr=ParameterValue;
  drec->octet_length_ptr=drec->indicator_ptr=StrLen_or_Ind;
  drec->octet_length=BufferLength;  // size of ParameterValue buffer (or cell)
 // store IPD info:
 // Some applications, e.g. DataManager from unixODBC, provide wrong values in [ParameterType] etc.
 // When [param_info_retrieved] is TRUE then these values can (and should) be ignored because the correct values are in [impl_ipd].
  if (!stmt->param_info_retrieved)
  { drec = stmt->impl_ipd.drecs.acc0(ipar);
    drec->specif=t_specif();  // the default
    drec->concise_type=ParameterType;
    switch (ParameterType)
    { case SQL_TYPE_DATE:        case SQL_DATE:
        drec->type=SQL_DATETIME;  drec->datetime_int_code=SQL_CODE_DATE;  break;
      case SQL_TYPE_TIME:        case SQL_TIME:
        drec->type=SQL_DATETIME;  drec->datetime_int_code=SQL_CODE_TIME;  break;
      case SQL_TYPE_TIMESTAMP:   case SQL_TIMESTAMP:
        drec->type=SQL_DATETIME;  drec->datetime_int_code=SQL_CODE_TIMESTAMP;  break;
      case SQL_NUMERIC:
        drec->precision=15;  drec->scale=0;  // setting defaults, cont.
      default:
        drec->type=ParameterType;     drec->datetime_int_code=0;  break;
    }
    if (ParameterType==SQL_CHAR        || ParameterType==SQL_VARCHAR ||
        ParameterType==SQL_WCHAR       || ParameterType==SQL_WVARCHAR ||
        ParameterType==SQL_LONGVARCHAR || ParameterType==SQL_WLONGVARCHAR || 
        ParameterType==SQL_BINARY ||
        ParameterType==SQL_VARBINARY   || ParameterType==SQL_LONGVARBINARY)
      drec->length=ColumnSize;
    else if (ParameterType==SQL_DECIMAL || ParameterType==SQL_NUMERIC ||
             ParameterType==SQL_FLOAT   || ParameterType==SQL_REAL ||
             ParameterType==SQL_DOUBLE)
      drec->precision=(short)ColumnSize;
    drec->parameter_type=InputOutputType;
    if (ParameterType==SQL_TYPE_TIME || ParameterType==SQL_TYPE_TIMESTAMP ||
        ParameterType==SQL_TIME      || ParameterType==SQL_TIMESTAMP ||
        ParameterType==SQL_NUMERIC   || ParameterType==SQL_DECIMAL)
      drec->precision=DecimalDigits;
  }
  return SQL_SUCCESS;
}

/////////////////////////// concise functions on columns //////////////////////////////

//      This returns the number of columns associated with the database
//      attached to "hstmt".

SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
/* Vraci nulu, pokud posledni prikaz (prep. nebo exec.) nemel result set */
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  *ColumnCount=stmt->impl_ird.count;
  return SQL_SUCCESS;
}

//      Return information about the database column the user wants
//      information about.

SQLRETURN SQL_API SQLDescribeColW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLWCHAR *ColumnName,
           SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
           SQLSMALLINT *DataType, SQLULEN *ColumnSize,
           SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (!stmt->impl_ird.count)
  { raise_err(stmt, SQL_07005);  // not a cursor specification
    return SQL_ERROR;
  }
  if (ColumnNumber > stmt->impl_ird.count)
  { raise_err(stmt, SQL_07009);  // Invalid column number 
    return SQL_ERROR;
  }
  t_desc_rec * drec = stmt->impl_ird.drecs.acc0(ColumnNumber);
  BOOL truncated = FALSE;
  if (ColumnName   !=NULL)
    truncated=write_string_wide_ch(drec->name, ColumnName, BufferLength*sizeof(SQLWCHAR), NameLength);
  // ATTN: SQLDescribeCol doc says that *NameLength is the number of bytes, but
  //       SQLDescribeColW doc says that *NameLength is the number of characters!!
  if (DataType     !=NULL) *DataType     =drec->concise_type;
  if (ColumnSize   !=NULL) *ColumnSize   =drec->column_size();
  if (DecimalDigits!=NULL) *DecimalDigits=drec->decimal_digits();
  if (Nullable     !=NULL) *Nullable     =drec->nullable;
  if (truncated)
  { raise_err(stmt, SQL_01004);  /* Data truncated */
    return SQL_SUCCESS_WITH_INFO;
  }
  return SQL_SUCCESS;
}

//      Returns result column descriptor information for a result set.

SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT StatementHandle,
 SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
 SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
 SQLSMALLINT *StringLength, SQLLEN * NumericAttribute)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  if (!stmt->impl_ird.count)
  { raise_err(stmt, SQL_07005);  // not a cursor specification
    return SQL_ERROR;
  }
  if (ColumnNumber > stmt->impl_ird.count)
  { raise_err(stmt, SQL_07009);  // Invalid column number 
    return SQL_ERROR;
  }
  if (FieldIdentifier==SQL_DESC_BASE_COLUMN_NAME || FieldIdentifier==SQL_DESC_BASE_TABLE_NAME ||
      FieldIdentifier==SQL_DESC_CATALOG_NAME || FieldIdentifier==SQL_DESC_LABEL ||
      FieldIdentifier==SQL_DESC_LITERAL_PREFIX || FieldIdentifier==SQL_DESC_LITERAL_SUFFIX ||
      FieldIdentifier==SQL_DESC_LOCAL_TYPE_NAME || FieldIdentifier==SQL_DESC_NAME ||
      FieldIdentifier==SQL_DESC_SCHEMA_NAME || FieldIdentifier==SQL_DESC_TABLE_NAME ||
      FieldIdentifier==SQL_DESC_TYPE_NAME)
  { SQLINTEGER Length;
    SQLRETURN res=SQLGetDescFieldW((SQLHDESC)&stmt->impl_ird, ColumnNumber, FieldIdentifier,
                           CharacterAttribute, BufferLength, &Length);
    if (StringLength!=NULL) *StringLength=(short)Length;
    return res;
  }
 // some FieldIdentifiers must be mapped explicitly:
  //if      (FieldIdentifier==SQL_COLUMN_LENGTH)    FieldIdentifier=SQL_DESC_LENGTH;
  //else if (FieldIdentifier==SQL_COLUMN_PRECISION) FieldIdentifier=SQL_DESC_PRECISION;
  //else if (FieldIdentifier==SQL_COLUMN_SCALE)     FieldIdentifier=SQL_DESC_SCALE;
  //else if (FieldIdentifier==SQL_COLUMN_NULLABLE)  FieldIdentifier=SQL_DESC_NULLABLE;
  SQLRETURN res=SQLGetDescFieldW((SQLHDESC)&stmt->impl_ird, ColumnNumber, FieldIdentifier,
                         NumericAttribute, sizeof(SQLINTEGER), NULL);
 // convert 16-bit results to 32-bit required by the old SQLColAttributes:
  if (FieldIdentifier==SQL_DESC_CONCISE_TYPE || FieldIdentifier==SQL_DESC_COUNT || 
      FieldIdentifier==SQL_DESC_FIXED_PREC_SCALE || FieldIdentifier==SQL_DESC_NULLABLE || 
      FieldIdentifier==SQL_DESC_PRECISION || FieldIdentifier==SQL_DESC_SCALE || 
      FieldIdentifier==SQL_DESC_SEARCHABLE || FieldIdentifier==SQL_DESC_TYPE || 
      FieldIdentifier==SQL_DESC_UNNAMED || FieldIdentifier==SQL_DESC_UNSIGNED || 
      FieldIdentifier==SQL_DESC_UPDATABLE ||
      FieldIdentifier==SQL_COLUMN_PRECISION || FieldIdentifier==SQL_COLUMN_SCALE || 
      FieldIdentifier==SQL_COLUMN_NULLABLE)
    *(SQLINTEGER*)NumericAttribute=(SQLINTEGER)*(SQLSMALLINT*)NumericAttribute;
  return res;
}

SQLRETURN SQL_API SQLColAttributesW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
    SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc, SQLLEN *pfDesc)
// MS Query calls SQLColAttributesW. When this function is not present, DM mapps the call to SQLColAttributeW
// and damages hstmt by the way. Patched by own implementation of the mappong
{ 
  if      (fDescType==SQL_COLUMN_COUNT)    fDescType=SQL_DESC_COUNT;
  else if (fDescType==SQL_COLUMN_NAME)     fDescType=SQL_DESC_NAME;
  else if (fDescType==SQL_COLUMN_NULLABLE) fDescType=SQL_DESC_NULLABLE;
  SQLRETURN res = SQLColAttributeW(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
 // if (fDescType==SQL_DESC_CONCISE_TYPE)  -- unclear
  return res;
}



//      Associate a user-supplied buffer with a database column.

SQLRETURN SQL_API SQLBindCol(SQLHSTMT StatementHandle, 
 SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, 
 SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
/* BindCol is independent of the result set. */
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  int i;  t_desc_rec * drec;
  for (i=stmt->ard->drecs.count();  i<=ColumnNumber;  i++)
  { drec = stmt->ard->drecs.acc(i);
    if (drec==NULL) { raise_err(stmt, SQL_HY001);  return SQL_ERROR; }
    drec->init();
  }
  if (stmt->ard->count < ColumnNumber) stmt->ard->count = ColumnNumber;
 // store ARD info: 
  drec = stmt->ard->drecs.acc0(ColumnNumber);
  drec->type=drec->concise_type=TargetType;
  drec->data_ptr=TargetValue;
  drec->octet_length_ptr=drec->indicator_ptr=StrLen_or_Ind;
  drec->octet_length=BufferLength;
 // implicit "count" resetting when unbinding:
  if (TargetValue==NULL && StrLen_or_Ind==NULL && stmt->apd->count == ColumnNumber)
    while (stmt->apd->count)
    { drec = stmt->ard->drecs.acc(stmt->ard->count);
      if (drec->data_ptr!=NULL || drec->octet_length_ptr!=NULL || drec->indicator_ptr!=NULL) break;
      stmt->ard->count--;
    }
  return SQL_SUCCESS;
}

