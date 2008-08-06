/*
** INFO.C - This is the ODBC driver code for executing information functions.
*/

#include "wbdrv.h"
#pragma hdrstop

SQLRETURN SQL_API SQLGetInfoW(SQLHDBC hdbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue, SQLSMALLINT cbInfoValueMax,
                              SQLSMALLINT *pcbInfoValue)
{ if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  DBC * dbc=(DBC*)hdbc;
  CLEAR_ERR(dbc);
  BOOL truncated = FALSE;  SQLSMALLINT aux_pcb;
  if (pcbInfoValue==NULL) pcbInfoValue=&aux_pcb;

  if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  if (!dbc->connected &&
      (fInfoType != SQL_ODBC_VER) && (fInfoType != SQL_DRIVER_ODBC_VER))
  { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
    return SQL_ERROR;
  }
  if (cbInfoValueMax < 0)
  { raise_err_dbc(dbc, SQL_HY090);  /* Invalid string or buffer length */
    return SQL_ERROR;
  }

  switch (fInfoType)
  { 
    case SQL_ACCESSIBLE_PROCEDURES:  /* some procs may not be accessible */
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_ACCESSIBLE_TABLES:   /* some tables may not be accessible */
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_ACTIVE_ENVIRONMENTS:
      *(SQLUSMALLINT*)rgbInfoValue=0;   /* not limited */
      *pcbInfoValue=sizeof(SQLUSMALLINT);  break;
    case SQL_AGGREGATE_FUNCTIONS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_AF_ALL;
      *pcbInfoValue=sizeof(SQLUINTEGER);  break;
    case SQL_ALTER_DOMAIN:  // using an alternative syntax
      *(SQLUINTEGER*)rgbInfoValue=0;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_ALTER_TABLE:              // approximate info
      *(SQLUINTEGER*)rgbInfoValue=SQL_AT_ADD_COLUMN_SINGLE | SQL_AT_DROP_COLUMN_RESTRICT | 
        SQL_AT_DROP_COLUMN_CASCADE | SQL_AT_CONSTRAINT_INITIALLY_IMMEDIATE |
        SQL_AT_CONSTRAINT_NON_DEFERRABLE;
      *pcbInfoValue=sizeof(SQLUINTEGER);  /* can use both ADD and DROP */
      break;
    case SQL_ASYNC_MODE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_AM_STATEMENT; //SQL_AM_NONE; changed in WB 6.0c, async still not implemented but I can say this
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_BATCH_ROW_COUNT:          // not rolled up
      *(SQLUINTEGER*)rgbInfoValue=SQL_BRC_EXPLICIT | SQL_BRC_PROCEDURES;  // PROCEDURES added in WB6.0c
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_BATCH_SUPPORT:            // batching procedures not supported
      *(SQLUINTEGER*)rgbInfoValue=SQL_BS_SELECT_EXPLICIT | SQL_BS_ROW_COUNT_EXPLICIT | SQL_BS_SELECT_PROC | SQL_BS_ROW_COUNT_PROC; // PROC added in WB6.0c
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_BOOKMARK_PERSISTENCE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_BP_CLOSE | SQL_BP_DELETE | SQL_BP_DROP |
        SQL_BP_TRANSACTION | SQL_BP_UPDATE | SQL_BP_OTHER_HSTMT;
      *pcbInfoValue=sizeof(SQLUINTEGER);
      break;
    case SQL_CATALOG_LOCATION:
      *(uns16*)rgbInfoValue=0; //SQL_CL_START;
      *pcbInfoValue=2;
      break;
    case SQL_CATALOG_NAME:
#ifdef USE_OWNER
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
#else
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
#endif
      break;
    case SQL_CATALOG_NAME_SEPARATOR:
#ifdef USE_OWNER  // no separator when not using catalogs (MS Excel 2003 crashes when empty, can use "." instead)
      if (write_string_wide_b("", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
#else
      if (write_string_wide_b(".", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
#endif
      break;
    case SQL_CATALOG_TERM:
#ifdef USE_OWNER
      if (write_string_wide_b("", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
#else
      if (write_string_wide_b("Application", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
#endif
      break;
    case SQL_CATALOG_USAGE:
#ifdef USE_OWNER
      *(uns32*)rgbInfoValue=0;
#else
      *(uns32*)rgbInfoValue=SQL_CU_DML_STATEMENTS;
#endif
      *pcbInfoValue=4;
      break;
    case SQL_COLLATION_SEQ: 
    { const char * name;
      switch (dbc->cdp->sys_charset)
      { case 1:  case 2:                   name="Code page 1250";  break;
        case 3:  case 4:  case 5:          name="Code page 1252";  break;
        case 0x81:  case 0x82:             name="ISO 8859-2";  break;
        case 0x83:  case 0x84:  case 0x85: name="ISO 8859-1";  break;
        default:                           name="ANSI";  break;
      }
      if (write_string_wide_b(name, (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;    
    }
    case SQL_COLUMN_ALIAS:  /* column aliases supported */
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_CONCAT_NULL_BEHAVIOR:
      *(uns16*)rgbInfoValue=SQL_CB_NULL; /* result is the NULL column (since 5.1) */
      *pcbInfoValue=2;
      break;
    case SQL_CONVERT_FUNCTIONS:
      *(uns32*)rgbInfoValue=SQL_FN_CVT_CAST; 
      *pcbInfoValue=4;
      break;
    case SQL_CONVERT_BIGINT:  // CONVERT function not supported
    case SQL_CONVERT_BINARY:
    case SQL_CONVERT_BIT:
    case SQL_CONVERT_CHAR:
    case SQL_CONVERT_DATE:
    case SQL_CONVERT_DECIMAL:
    case SQL_CONVERT_DOUBLE:
    case SQL_CONVERT_FLOAT:
    case SQL_CONVERT_INTEGER:
    case SQL_CONVERT_LONGVARCHAR:
    case SQL_CONVERT_NUMERIC:
    case SQL_CONVERT_REAL:
    case SQL_CONVERT_SMALLINT:
    case SQL_CONVERT_TIME:
    case SQL_CONVERT_TIMESTAMP:
    case SQL_CONVERT_TINYINT:
    case SQL_CONVERT_VARBINARY:
    case SQL_CONVERT_VARCHAR:
    case SQL_CONVERT_LONGVARBINARY:
      *(uns32*)rgbInfoValue=0;
      *pcbInfoValue=4;
      break;
    case SQL_CORRELATION_NAME:
      *(uns16*)rgbInfoValue=SQL_CN_ANY;
      *pcbInfoValue=2;
      break;
    case SQL_CREATE_ASSERTION:
    case SQL_CREATE_CHARACTER_SET:
    case SQL_CREATE_COLLATION:
    case SQL_CREATE_DOMAIN:
    case SQL_CREATE_SCHEMA:
    case SQL_CREATE_TRANSLATION:
      *(SQLUINTEGER*)rgbInfoValue=0;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_CREATE_TABLE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_CT_CREATE_TABLE | SQL_CT_TABLE_CONSTRAINT | 
        SQL_CT_CONSTRAINT_NAME_DEFINITION | SQL_CT_COLUMN_CONSTRAINT | 
        SQL_CT_COLUMN_DEFAULT | SQL_CT_COLUMN_COLLATION;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_CREATE_VIEW:
      *(SQLUINTEGER*)rgbInfoValue=SQL_CV_CREATE_VIEW;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_CURSOR_COMMIT_BEHAVIOR:
      *(uns16*)rgbInfoValue=SQL_CC_PRESERVE; /* COMMIT preserves cursors & prepared stmts */
      *pcbInfoValue=2;
      break;
    case SQL_CURSOR_ROLLBACK_BEHAVIOR:
      *(uns16*)rgbInfoValue=SQL_CR_PRESERVE; /* ROLLBACK preserves cursors & prepared stmts */
      *pcbInfoValue=2;
      break;
    case SQL_CURSOR_SENSITIVITY:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SENSITIVE | SQL_INSENSITIVE;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DATA_SOURCE_NAME:
      if (write_string_wide_b(dbc->dsn, (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_DATA_SOURCE_READ_ONLY:
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_DATABASE_NAME:           // same as current catalog
      if (write_string_wide_b(dbc->server_name, (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_DATETIME_LITERALS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_DL_SQL92_DATE|SQL_DL_SQL92_TIME|SQL_DL_SQL92_TIMESTAMP;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DBMS_NAME:
      if (write_string_wide_b("602SQL", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_DBMS_VER:
    { char verstr[16];
#ifdef STOP  // obsolete    
      DWORD ver; 
      ver=cd_WinBase602_version(dbc->cdp);
      sprintf(verstr, "%02u.%02u.0000", HIWORD(ver), LOWORD(ver));
#else
      uns32 vers1, vers2, vers4;
      cd_Get_server_info(dbc->cdp, OP_GI_VERSION_1, &vers1, sizeof(vers1));
      cd_Get_server_info(dbc->cdp, OP_GI_VERSION_2, &vers2, sizeof(vers2));
      cd_Get_server_info(dbc->cdp, OP_GI_VERSION_4, &vers4, sizeof(vers4));
      sprintf(verstr, "%02u.%02u.%04u", vers1, vers2, vers4);
#endif      
      if (write_string_wide_b(verstr, (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    }
    case SQL_DDL_INDEX:
      *(SQLUINTEGER*)rgbInfoValue=SQL_DI_CREATE_INDEX|SQL_DI_DROP_INDEX;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DEFAULT_TXN_ISOLATION:
      *(uns32*)rgbInfoValue=SQL_TXN_READ_COMMITTED;
      *pcbInfoValue=4;
      break;
    case SQL_DESCRIBE_PARAMETER:
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;

//    case SQL_DM_VER:
//    case SQL_DRIVER_HENV:   -- impl. by Driver Manager
//    case SQL_DRIVER_HDBC:
//    case SQL_DRIVER_HSTMT:
//    case SQL_DRIVER_HLIB:

    case SQL_DRIVER_NAME:
      if (write_string_wide_b(ODBC_DRIVER_FILE, (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_DRIVER_ODBC_VER: // must be 3.00 even in 3.50
      if (write_string_wide_b("03.00", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_DRIVER_VER:
      if (write_string_wide_b("03.50.1000", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_DROP_ASSERTION:
    case SQL_DROP_CHARACTER_SET:
    case SQL_DROP_COLLATION:
      *(SQLUINTEGER*)rgbInfoValue=0;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DROP_DOMAIN:
      *(SQLUINTEGER*)rgbInfoValue=SQL_DD_DROP_DOMAIN | SQL_DD_CASCADE | SQL_DD_RESTRICT;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DROP_SCHEMA:
      *(SQLUINTEGER*)rgbInfoValue=SQL_DS_DROP_SCHEMA | SQL_DS_CASCADE | SQL_DS_RESTRICT;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DROP_TRANSLATION:
      *(SQLUINTEGER*)rgbInfoValue=0;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DROP_TABLE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_DT_DROP_TABLE|SQL_DT_CASCADE|SQL_DT_RESTRICT;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DROP_VIEW:
      *(SQLUINTEGER*)rgbInfoValue=SQL_DV_DROP_VIEW;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
    case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
    case SQL_KEYSET_CURSOR_ATTRIBUTES1:
    case SQL_STATIC_CURSOR_ATTRIBUTES1:
      *(SQLUINTEGER*)rgbInfoValue=SQL_CA1_NEXT|SQL_CA1_ABSOLUTE|SQL_CA1_RELATIVE|
        SQL_CA1_BOOKMARK|SQL_CA1_LOCK_EXCLUSIVE|SQL_CA1_LOCK_NO_CHANGE|
        SQL_CA1_LOCK_UNLOCK|SQL_CA1_POS_POSITION|SQL_CA1_POS_UPDATE|
        SQL_CA1_POS_DELETE|SQL_CA1_POS_REFRESH|SQL_CA1_POSITIONED_UPDATE|
        SQL_CA1_POSITIONED_DELETE|SQL_CA1_SELECT_FOR_UPDATE;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
    case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
    case SQL_KEYSET_CURSOR_ATTRIBUTES2:
    case SQL_STATIC_CURSOR_ATTRIBUTES2:
      *(SQLUINTEGER*)rgbInfoValue=SQL_CA2_READ_ONLY_CONCURRENCY|SQL_CA2_LOCK_CONCURRENCY|
        SQL_CA2_OPT_ROWVER_CONCURRENCY|SQL_CA2_SENSITIVITY_ADDITIONS|
        SQL_CA2_SENSITIVITY_UPDATES|SQL_CA2_CRC_EXACT|SQL_CA2_SIMULATE_TRY_UNIQUE;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_EXPRESSIONS_IN_ORDERBY:
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_FETCH_DIRECTION:  /* all directions */
      *(uns32*)rgbInfoValue=
        SQL_FD_FETCH_NEXT     | SQL_FD_FETCH_FIRST    |
        SQL_FD_FETCH_LAST     | SQL_FD_FETCH_PREV     |
        SQL_FD_FETCH_ABSOLUTE | SQL_FD_FETCH_RELATIVE |
        SQL_FD_FETCH_BOOKMARK;
      *pcbInfoValue=4;
      break;
    case SQL_FILE_USAGE:
      *(sig16*)rgbInfoValue=SQL_FILE_NOT_SUPPORTED;
      *pcbInfoValue=2;
      break;
    case SQL_GETDATA_EXTENSIONS:
      *(uns32*)rgbInfoValue=SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER |
                            SQL_GD_BLOCK      | SQL_GD_BOUND;
      *pcbInfoValue=4;
      break;
    case SQL_GROUP_BY:
      *(sig16*)rgbInfoValue=SQL_GB_GROUP_BY_CONTAINS_SELECT;
      *pcbInfoValue=2;
      break;
    case SQL_IDENTIFIER_CASE:
      *(uns16*)rgbInfoValue=SQL_IC_UPPER;   // not case sensitive, stored in upper case 
      *pcbInfoValue=2;
      break;
    case SQL_IDENTIFIER_QUOTE_CHAR:
      if (write_string_wide_b("`", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_INDEX_KEYWORDS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_IK_ALL;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_INFO_SCHEMA_VIEWS:
      *(SQLUINTEGER*)rgbInfoValue=0;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_INSERT_STATEMENT:
      *(SQLUINTEGER*)rgbInfoValue=SQL_IS_INSERT_LITERALS|SQL_IS_INSERT_SEARCHED|SQL_IS_SELECT_INTO;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_INTEGRITY:  /* ??? */
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;


    case SQL_KEYWORDS:
      if (write_string_wide_b("AUTOR,HISTORY,DATIM,PTR,BIPTR", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_LIKE_ESCAPE_CLAUSE:
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_LOCK_TYPES:
      *(uns32*)rgbInfoValue=SQL_LCK_NO_CHANGE |
                            SQL_LCK_EXCLUSIVE | SQL_LCK_UNLOCK;
      *pcbInfoValue=4;
      break;
    case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
      *(SQLUINTEGER*)rgbInfoValue=0;  // no specific limit
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_MAX_BINARY_LITERAL_LEN:
      *(SQLUINTEGER*)rgbInfoValue=0;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_MAX_CATALOG_NAME_LEN:
      *(uns16*)rgbInfoValue=INFO_QUALIF_LEN;
      *pcbInfoValue=2;
      break;
    case SQL_MAX_CHAR_LITERAL_LEN:
      *(uns32*)rgbInfoValue=0;
      *pcbInfoValue=4;
      break;
    case SQL_MAX_COLUMN_NAME_LEN:
      *(uns16*)rgbInfoValue=ATTRNAMELEN;
      *pcbInfoValue=2;
      break;
    case SQL_MAX_COLUMNS_IN_GROUP_BY:
    case SQL_MAX_COLUMNS_IN_INDEX:
    case SQL_MAX_COLUMNS_IN_ORDER_BY:
      *(uns16*)rgbInfoValue=MAX_INDEX_PARTS;
      *pcbInfoValue=2;
      break;
    case SQL_MAX_COLUMNS_IN_SELECT:
    case SQL_MAX_COLUMNS_IN_TABLE:
      *(uns16*)rgbInfoValue=MAX_TABLE_COLUMNS-1;  // the "DELETED" column not counted here
      *pcbInfoValue=2;
      break;
    case SQL_MAX_CONCURRENT_ACTIVITIES:
      *(uns16*)rgbInfoValue=0;   /* not limited */
      *pcbInfoValue=2;
      break;
    case SQL_MAX_CURSOR_NAME_LEN:
      *(uns16*)rgbInfoValue=OBJ_NAME_LEN;
      *pcbInfoValue=2;
      break;
    case SQL_MAX_DRIVER_CONNECTIONS:
    { uns32 cnt;
      if (cd_Get_server_info(dbc->cdp, OP_GI_LICS_CLIENT, &cnt, sizeof(cnt)))
        *(uns16*)rgbInfoValue=0;  /* unknown */
      else
        *(uns16*)rgbInfoValue=(uns16)cnt;
      *pcbInfoValue=2;
      break;
    }
    case SQL_MAX_IDENTIFIER_LEN:
      *(SQLUSMALLINT*)rgbInfoValue=OBJ_NAME_LEN;
      *pcbInfoValue=sizeof(SQLUSMALLINT);  break;

    case SQL_MAX_INDEX_SIZE:
      *(uns32*)rgbInfoValue=MAX_KEY_LEN;
      *pcbInfoValue=4;
      break;
    case SQL_MAX_PROCEDURE_NAME_LEN:   
      *(uns16*)rgbInfoValue=OBJ_NAME_LEN;
      *pcbInfoValue=2;
      break;
    case SQL_MAX_ROW_SIZE:
      *(uns32*)rgbInfoValue=0;
      *pcbInfoValue=4;
      break;
    case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_MAX_SCHEMA_NAME_LEN:   /* owner name not supported */
      *(uns16*)rgbInfoValue=INFO_OWNER_LEN;  /* equal to string lenght, must not be 0 */
      *pcbInfoValue=2;
      break;
    case SQL_MAX_STATEMENT_LEN:
      *(uns32*)rgbInfoValue=0;
      *pcbInfoValue=4;
      break;
    case SQL_MAX_TABLE_NAME_LEN:
      *(uns16*)rgbInfoValue=OBJ_NAME_LEN;
      *pcbInfoValue=2;
      break;
    case SQL_MAX_TABLES_IN_SELECT:
      *(uns16*)rgbInfoValue=32;
      *pcbInfoValue=2;
      break;
    case SQL_MAX_USER_NAME_LEN:
      *(uns16*)rgbInfoValue=OBJ_NAME_LEN;
      *pcbInfoValue=2;
      break;
    case SQL_MULT_RESULT_SETS:
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_MULTIPLE_ACTIVE_TXN:
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_NEED_LONG_DATA_LEN:
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_NON_NULLABLE_COLUMNS:
      *(uns16*)rgbInfoValue=SQL_NNC_NON_NULL;
      *pcbInfoValue=2;
      break;
    case SQL_NULL_COLLATION:
      *(uns16*)rgbInfoValue=SQL_NC_LOW;
      *pcbInfoValue=2;
      break;
    case SQL_NUMERIC_FUNCTIONS:
      *(uns32*)rgbInfoValue=
        SQL_FN_NUM_ABS   | //SQL_FN_NUM_ACOS  | SQL_FN_NUM_ASIN    |
        //SQL_FN_NUM_ATAN  | SQL_FN_NUM_ATAN2 | SQL_FN_NUM_CEILING |
        SQL_FN_NUM_COS   | //SQL_FN_NUM_COT   |
        SQL_FN_NUM_EXP     |
        //SQL_FN_NUM_FLOOR |
        SQL_FN_NUM_LOG   | SQL_FN_NUM_MOD     | SQL_FN_NUM_ROUND   |
        //SQL_FN_NUM_RAND  | SQL_FN_NUM_PI    | SQL_FN_NUM_SIGN    |
        SQL_FN_NUM_SIN   | SQL_FN_NUM_SQRT;  //| SQL_FN_NUM_TAN;
      *pcbInfoValue=4;
      break;
    case SQL_ODBC_API_CONFORMANCE:
      *(uns16*)rgbInfoValue=SQL_OAC_LEVEL2;  /* API Level 2 */
      *pcbInfoValue=2;
      break;
    case SQL_ODBC_SQL_CONFORMANCE:
      *(uns16*)rgbInfoValue=SQL_OSC_EXTENDED; /* extended grammar supported */
      *pcbInfoValue=2;
      break;
    case SQL_ODBC_INTERFACE_CONFORMANCE:
      *(uns16*)rgbInfoValue=SQL_OIC_LEVEL2;
      *pcbInfoValue=2;
      break;
//  case SQL_ODBC_VER:   should be implemented by Driver Manager only
    case SQL_OJ_CAPABILITIES:
      *(uns32*)rgbInfoValue=SQL_OJ_LEFT | SQL_OJ_RIGHT | SQL_OJ_FULL |
        SQL_OJ_NESTED | SQL_OJ_INNER | SQL_OJ_NOT_ORDERED | SQL_OJ_ALL_COMPARISON_OPS;
      *pcbInfoValue=4;
      break;
    case SQL_ORDER_BY_COLUMNS_IN_SELECT:
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_OUTER_JOINS: // only for compatibility with ODBC 2
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_PARAM_ARRAY_ROW_COUNTS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_PARC_BATCH;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_PARAM_ARRAY_SELECTS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_PAS_BATCH;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_POS_OPERATIONS:
      *(uns32*)rgbInfoValue=SQL_POS_POSITION | SQL_POS_REFRESH |
                      SQL_POS_UPDATE | SQL_POS_DELETE | SQL_POS_ADD;
      *pcbInfoValue=4;
      break;
    case SQL_POSITIONED_STATEMENTS:
      *(uns32*)rgbInfoValue=SQL_PS_POSITIONED_DELETE |
        SQL_PS_POSITIONED_UPDATE | SQL_PS_SELECT_FOR_UPDATE;
      *pcbInfoValue=4;
      break;
    case SQL_PROCEDURE_TERM:
      if (write_string_wide_b("Procedure", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_PROCEDURES: 
      if (write_string_wide_b("Y", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_QUOTED_IDENTIFIER_CASE:
      *(uns16*)rgbInfoValue=SQL_IC_UPPER;   // not case sensitive, stored in upper case 
      *pcbInfoValue=2;
      break;
    case SQL_ROW_UPDATES:
      if (write_string_wide_b("N", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_SCHEMA_TERM:
#ifdef USE_OWNER
      if (write_string_wide_b("Schema", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
#else
      if (write_string_wide_b("", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
#endif
      break;
    case SQL_SCHEMA_USAGE:
#ifdef USE_OWNER
      *(uns32*)rgbInfoValue=SQL_OU_DML_STATEMENTS |
         SQL_OU_INDEX_DEFINITION | SQL_OU_PRIVILEGE_DEFINITION;
#else
      *(uns32*)rgbInfoValue=0;
#endif
      *pcbInfoValue=4;
      break;
    case SQL_SCROLL_CONCURRENCY:
      *(uns32*)rgbInfoValue=SQL_SCCO_READ_ONLY | SQL_SCCO_LOCK;
      *pcbInfoValue=4;
      break;
    case SQL_SCROLL_OPTIONS:
      *(uns32*)rgbInfoValue=SQL_SO_FORWARD_ONLY | SQL_SO_KEYSET_DRIVEN |
                            SQL_SO_MIXED        | SQL_SO_STATIC | SQL_SO_DYNAMIC;
      *pcbInfoValue=4;
      break;
    case SQL_SEARCH_PATTERN_ESCAPE:  // no default escape char
      if (write_string_wide_b("\\", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_SERVER_NAME:
      if (write_string_wide_b(dbc->cdp->conn_server_name, (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_SPECIAL_CHARACTERS:
      if (write_string_wide_b("_áÁèÈïÏéÉìÌíÍòÒóÓøØšŠúÚùÙýÝžŽ", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_SQL_CONFORMANCE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SC_SQL92_ENTRY;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_DATETIME_FUNCTIONS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SDF_CURRENT_DATE|SQL_SDF_CURRENT_TIME|SQL_SDF_CURRENT_TIMESTAMP;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SFKD_CASCADE|SQL_SFKD_NO_ACTION|SQL_SFKD_SET_DEFAULT|SQL_SFKD_SET_NULL;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SFKU_CASCADE|SQL_SFKU_NO_ACTION|SQL_SFKU_SET_DEFAULT|SQL_SFKU_SET_NULL;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_GRANT:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SG_DELETE_TABLE|SQL_SG_INSERT_TABLE|
        SQL_SG_REFERENCES_TABLE|SQL_SG_REFERENCES_COLUMN|SQL_SG_SELECT_TABLE|
        SQL_SG_UPDATE_COLUMN|SQL_SG_UPDATE_TABLE;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SNVF_BIT_LENGTH|SQL_SNVF_CHAR_LENGTH|
        SQL_SNVF_CHARACTER_LENGTH|SQL_SNVF_EXTRACT|SQL_SNVF_OCTET_LENGTH|
        SQL_SNVF_POSITION;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_PREDICATES:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SP_BETWEEN|SQL_SP_COMPARISON|SQL_SP_EXISTS|
        SQL_SP_IN|SQL_SP_ISNOTNULL|SQL_SP_ISNULL|SQL_SP_LIKE|
        SQL_SP_QUANTIFIED_COMPARISON|SQL_SP_UNIQUE;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SRJO_CORRESPONDING_CLAUSE|
        SQL_SRJO_CROSS_JOIN|SQL_SRJO_FULL_OUTER_JOIN|SQL_SRJO_INNER_JOIN|
        SQL_SRJO_LEFT_OUTER_JOIN|SQL_SRJO_NATURAL_JOIN|
        SQL_SRJO_RIGHT_OUTER_JOIN|SQL_SRJO_INNER_JOIN;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_REVOKE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SR_DELETE_TABLE|SQL_SR_INSERT_TABLE|
        SQL_SR_REFERENCES_TABLE|SQL_SR_REFERENCES_COLUMN|SQL_SR_SELECT_TABLE|
        SQL_SR_UPDATE_COLUMN|SQL_SR_UPDATE_TABLE|SQL_SR_GRANT_OPTION_FOR;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_ROW_VALUE_CONSTRUCTOR:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SRVC_VALUE_EXPRESSION|SQL_SRVC_NULL|SQL_SRVC_DEFAULT|SQL_SRVC_ROW_SUBQUERY;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_STRING_FUNCTIONS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SSF_LOWER|SQL_SSF_UPPER|SQL_SSF_SUBSTRING;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_SQL92_VALUE_EXPRESSIONS:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SVE_CASE|SQL_SVE_CAST|SQL_SVE_COALESCE|
        SQL_SVE_NULLIF;
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_STANDARD_CLI_CONFORMANCE:
      *(SQLUINTEGER*)rgbInfoValue=SQL_SCC_ISO92_CLI;  // I am not sure
      *pcbInfoValue=sizeof(SQLUINTEGER);   break;
    case SQL_STATIC_SENSITIVITY:
      *(uns32*)rgbInfoValue=
        SQL_SS_ADDITIONS | SQL_SS_DELETIONS | SQL_SS_UPDATES;
      *pcbInfoValue=4;
      break;
    case SQL_STRING_FUNCTIONS:
      *(uns32*)rgbInfoValue=SQL_FN_STR_BIT_LENGTH|SQL_FN_STR_CHAR_LENGTH|
        SQL_FN_STR_CONCAT|SQL_FN_STR_LENGTH|SQL_FN_STR_OCTET_LENGTH|
        SQL_FN_STR_POSITION|SQL_FN_STR_SUBSTRING;
//        SQL_FN_STR_ASCII  | SQL_FN_STR_CHAR      | 
//        SQL_FN_STR_INSERT | SQL_FN_STR_LEFT      | SQL_FN_STR_LTRIM  |
//        SQL_FN_STR_LENGTH | SQL_FN_STR_LOCATE    | SQL_FN_STR_LCASE  |
//        SQL_FN_STR_REPEAT | SQL_FN_STR_REPLACE   | SQL_FN_STR_RIGHT  |
//        SQL_FN_STR_RTRIM  | SQL_FN_STR_UCASE;
      *pcbInfoValue=4;
      break;
    case SQL_SUBQUERIES:
      *(uns32*)rgbInfoValue=SQL_SQ_CORRELATED_SUBQUERIES|SQL_SQ_COMPARISON|
        SQL_SQ_EXISTS|SQL_SQ_IN|SQL_SQ_QUANTIFIED;
      *pcbInfoValue=4;
      break;
    case SQL_SYSTEM_FUNCTIONS:
      *(uns32*)rgbInfoValue=SQL_FN_SYS_IFNULL;
      *pcbInfoValue=4;
      break;
    case SQL_TABLE_TERM:
      if (write_string_wide_b("Table", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    case SQL_TIMEDATE_ADD_INTERVALS:
    case SQL_TIMEDATE_DIFF_INTERVALS:
      *(uns32*)rgbInfoValue=0;
      *pcbInfoValue=4;
      break;
    case SQL_TIMEDATE_FUNCTIONS:
      *(uns32*)rgbInfoValue=SQL_FN_TD_CURRENT_DATE|SQL_FN_TD_CURRENT_TIME|
        SQL_FN_TD_CURRENT_TIMESTAMP|SQL_FN_TD_EXTRACT|SQL_FN_TD_QUARTER|
        SQL_FN_TD_NOW | //SQL_FN_TD_CURDATE | SQL_FN_TD_DAYOFMONTH |
        SQL_FN_TD_DAYOFWEEK | //SQL_FN_TD_DAYOFYEAR |
        SQL_FN_TD_MONTH |
        //SQL_FN_TD_QUARTER | SQL_FN_TD_WEEK |
        SQL_FN_TD_YEAR |
        //SQL_FN_TD_CURTIME |
        SQL_FN_TD_HOUR | SQL_FN_TD_MINUTE |
        SQL_FN_TD_SECOND;
      *pcbInfoValue=4;
      break;
    case SQL_TXN_CAPABLE:    /* DML and DDL */
      *(uns16*)rgbInfoValue=SQL_TC_DML;
      *pcbInfoValue=2;
      break;
    case SQL_TXN_ISOLATION_OPTION:
      *(uns32*)rgbInfoValue=SQL_TXN_READ_UNCOMMITTED |SQL_TXN_READ_COMMITTED | SQL_TXN_REPEATABLE_READ | SQL_TXN_SERIALIZABLE;
      *pcbInfoValue=4;
      break;
    case SQL_UNION:
      *(uns32*)rgbInfoValue=SQL_U_UNION | SQL_U_UNION_ALL;
      *pcbInfoValue=4;
      break;
    case SQL_USER_NAME:
      if (write_string_wide_b(cd_Who_am_I(dbc->cdp), (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue, dbc->cdp->sys_charset))
        truncated=TRUE;
      break;
    case SQL_XOPEN_CLI_YEAR:    // I am not sure, 1995 is used by MS SQL
      if (write_string_wide_b("1995", (SQLWCHAR*)rgbInfoValue, cbInfoValueMax, pcbInfoValue))
        truncated=TRUE;
      break;
    default:
      raise_err_dbc(dbc, SQL_HY096);  /* Information type out of range */
      return SQL_ERROR;
  }
  if (truncated)
  { raise_err_dbc(dbc, SQL_01004);  /* Data truncated */
    return SQL_SUCCESS_WITH_INFO;
  }
  return SQL_SUCCESS;
}

//	-	-	-	-	-	-	-	-	-

#if 0 /* The driver manager implementation will be sufficient */

RETCODE SQL_API SQLGetFunctions(HDBC lpdbc,	UWORD fFunction, UWORD FAR *pfExists)
{
	if (fFunction == SQL_API_ALL_FUNCTIONS)
	{	int i;
		memset (pfExists, 0, sizeof(UWORD)*100);
		for (i = SQL_API_SQLALLOCCONNECT; i <= SQL_NUM_FUNCTIONS; i++)
			pfExists[i] = TRUE;
		for (i = SQL_EXT_API_START; i <= SQL_EXT_API_LAST; i++)
			pfExists[i] = TRUE;
	}
	else
		*pfExists = TRUE;
	return SQL_SUCCESS;
}
#endif


