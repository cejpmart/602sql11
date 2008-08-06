/*
** CATALOG.C - ODBC driver code for executing Data Dictionary functions.
*/
// Ve jmenu aplikace nelze pouzivat vyhledavani. Pokud se zada NULL nebo %, vezme
//  se otevrena aplikace (pouze neni-li otevrena, vsechny). Jinak se hleda 
//  v zadane aplikaci. Nekteri klienti totiz zadavaji vzdy NULL.

// Jmena se zadavaji a vraceji bez quotes. 
// Paramater prazdny string oznacuje prazdny string a nic jineho
// Paramatr NULL oznacuje %.
// V paramaterech mohou byt ecsape sekvence JMENO\_TAB, proto parametr 
//  muze byt delsi nez OBJ_NAME_LEN.
// Vypnuti ecsape sekvenci pomoci MEDATADA_ID funguje.
// Pattern se pripousti ve vsech argumentech, nad pozadavek normy. Muze pak ale
//  vratit vice, kdyz bude _ v argumentu, ktery neni pattern.

// Special columns/best_rowid vraci sloupce prvniho unikatniho indexu, ktery
//  obsahuje pouze sloupce (respektuje pozadavek NULLABLE).
// Special columns/best_rowid vraci _WB_ZCR, pokud existuje
// Default value pro sloupec/parametr: vraci pouze NULL nebo TRUNCATED.

// Neimplementovano: grantor, foreign keys, primary keys

#include "wbdrv.h"               
#pragma hdrstop

SQLRETURN catalog_emit(STMT * stmt, unsigned ctlg_type, tptr inpars, unsigned parsize)
// Common point of checking the state, sending catalog request & creating catalog result set
{ if (stmt->my_dbc->my_env->odbc_version == SQL_OV_ODBC2) ctlg_type+=1000;
#ifdef SEQ_CHECKING
  if (stmt->async_run)
  { corefree(inpars);
    raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
#endif
  if (stmt->has_result_set)  /* must close the result set before */
  { corefree(inpars);
    raise_err(stmt, SQL_24000);  /* Invalid cursor state */
    return SQL_ERROR;
  }
  if (!stmt->result_sets.acc(0))
  { corefree(inpars);
    raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  stmt->async_run=TRUE;
  BOOL res=cd_SQL_catalog(stmt->cdp, stmt->metadata, ctlg_type, parsize, inpars, (uns32*)stmt->result_sets.acc(0));
  corefree(inpars);
  if (res)
  { raise_db_err(stmt);
    stmt->async_run=FALSE;
    return SQL_ERROR;
  }
  define_catalog_result_set(stmt, *(uns32*)stmt->result_sets.acc(0));
  stmt->sql_stmt_cnt=1;   /* result set extended */
  stmt->sql_stmt_ind=0;   /* open the 1st result set */
  stmt->rs_per_iter =1;   // value not used
  BOOL ok=open_result_set(stmt);
  stmt->async_run=FALSE;
  return ok ? SQL_SUCCESS : SQL_ERROR;
}

static void store_param(tptr inpars, const char * data, unsigned limit, bool null)
// Trims spaces in data, replaces data==NULL by 0xff, supports lenght==SQL_NTS,
// limits the lenght to limit.
{ if (null) *inpars=(uns8)0xff;  
  else
  { while (*data==' ') data++;
    int length = (int)strlen(data);
    while (length && data[length-1]==' ') length--; 
    memcpy(inpars, data, length<limit ? length : limit);
  }
}

//      Have DBMS set up result set of Tables.

SQLRETURN SQL_API SQLTablesW(HSTMT hstmt, SQLWCHAR *szTableQualifier,
        SWORD   cbTableQualifier,       SQLWCHAR *szTableOwner, SWORD cbTableOwner,
        SQLWCHAR *szTableName, SWORD   cbTableName, SQLWCHAR *szTableType,
        SWORD   cbTableType)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
  wide2ansi xQual(szTableQualifier, cbTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema(szTableOwner,   cbTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab(szTableName,       cbTableName, stmt->cdp->sys_charset);
  wide2ansi xType(szTableType,      cbTableType, stmt->cdp->sys_charset);
 /* table type: */
  int rest = xType.get_len();
  unsigned table_type_set;
  if (xType.get_string()[0]=='%')
    table_type_set = TABLE_TYPE_LIST;
  else
  { tptr p=xType.get_string();  table_type_set = 0;
    do
    { while (rest && (*p==' ')) { rest--;  p++; }
      if (!rest) break;
      if (*p=='\'') { rest--;  p++; }
      if (!rest) break;
      else if ((rest >=  5) && !strnicmp(p, "TABLE",         5))
        { p+= 5;  rest-= 5;  table_type_set|=TABLE_TYPE_TABLE; }
      else if ((rest >=  4) && !strnicmp(p, "VIEW",          4))
        { p+= 4;  rest-= 4;  table_type_set|=TABLE_TYPE_VIEW; }
      else if ((rest >= 12) && !strnicmp(p, "SYSTEM TABLE", 12))
        { p+=12;  rest-=12;  table_type_set|=TABLE_TYPE_SYSTEM_TABLE; }
      else if ((rest >=  7) && !strnicmp(p, "SYNONYM",       7))
        { p+= 7;  rest-= 7;  table_type_set|=TABLE_TYPE_SYNONYM; }
      else /* ODBC 2.0: no error if table type not recognized - skip it */
        while (rest && (*p!=',') && (*p!='\'')) { rest--;  p++; }
     /* go to the next item: */
      if    (rest && (*p=='\'')) { rest--;  p++; }
      while (rest && (*p==' '))  { rest--;  p++; }
      if    (rest && (*p==','))  { rest--;  p++; }
    } while (rest);
  if (!table_type_set)  /* logical, but not in the documentation */
    /* examples and MS QUERY use this */
    table_type_set=TABLE_TYPE_TABLE        | TABLE_TYPE_VIEW |
                   TABLE_TYPE_SYSTEM_TABLE | TABLE_TYPE_SYNONYM;
  }
 /* pack parameters: */
  unsigned parsize=2*(PATT_NAME_LEN+1)+sizeof(unsigned);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !szTableOwner);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !szTableQualifier);
#endif
  store_param(inpars+PATT_NAME_LEN+1, xTab.get_string(), PATT_NAME_LEN, !szTableName);
  *(unsigned*)(inpars+2*(PATT_NAME_LEN+1))=table_type_set;
  return catalog_emit(stmt, CTLG_TABLES, inpars, parsize);
}

//      Have DBMS set up result set of Columns.

SQLRETURN SQL_API SQLColumnsW(HSTMT hstmt, SQLWCHAR *szTableQualifier,
        SWORD   cbTableQualifier,       SQLWCHAR *szTableOwner, SWORD  cbTableOwner,
        SQLWCHAR *szTableName, SWORD   cbTableName, SQLWCHAR *szColumnName,
        SWORD   cbColumnName)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
  wide2ansi xQual(szTableQualifier, cbTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema(szTableOwner,   cbTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab(szTableName,       cbTableName, stmt->cdp->sys_charset);
  wide2ansi xCol(szColumnName,      cbColumnName, stmt->cdp->sys_charset);
 /* pack parameters: */
  unsigned parsize=3*(PATT_NAME_LEN+1);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !szTableOwner);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !szTableQualifier);
#endif
  store_param(inpars+   PATT_NAME_LEN+1,  xTab.get_string(), PATT_NAME_LEN, !szTableName);
  store_param(inpars+2*(PATT_NAME_LEN+1), xCol.get_string(), PATT_NAME_LEN, !szColumnName);
  return catalog_emit(stmt, CTLG_COLUMNS, inpars, parsize);
}

//      Have DBMS set up result set of Statistics.

SQLRETURN SQL_API SQLStatisticsW(
        HSTMT   hstmt, SQLWCHAR *szTableQualifier,     SWORD   cbTableQualifier,
        SQLWCHAR *szTableOwner, SWORD cbTableOwner, SQLWCHAR *szTableName,
        SWORD   cbTableName, UWORD fUnique,     UWORD   fAccuracy)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
  wide2ansi xQual(szTableQualifier, cbTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema(szTableOwner,   cbTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab(szTableName,       cbTableName, stmt->cdp->sys_charset);
 /* pack parameters: */
  unsigned parsize=2*(PATT_NAME_LEN+1)+2*sizeof(unsigned);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !szTableOwner);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !szTableQualifier);
#endif
  store_param(inpars+PATT_NAME_LEN+1, xTab.get_string(), PATT_NAME_LEN, !szTableName);
  ((unsigned*)(inpars+2*(PATT_NAME_LEN+1)))[0]=fUnique;
  ((unsigned*)(inpars+2*(PATT_NAME_LEN+1)))[1]=fAccuracy;
  return catalog_emit(stmt, CTLG_STATISTICS, inpars, parsize);
}

//      Have DBMS set up result set of TablePrivileges.

SQLRETURN SQL_API SQLTablePrivilegesW(HSTMT hstmt, SQLWCHAR *szTableQualifier,
        SWORD   cbTableQualifier,       SQLWCHAR *szTableOwner, SWORD cbTableOwner,
        SQLWCHAR *szTableName, SWORD   cbTableName)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
 /* pack parameters: */
  wide2ansi xQual(szTableQualifier, cbTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema(szTableOwner,   cbTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab(szTableName,       cbTableName, stmt->cdp->sys_charset);
  unsigned parsize=2*(PATT_NAME_LEN+1);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !szTableOwner);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !szTableQualifier);
#endif
  store_param(inpars+PATT_NAME_LEN+1, xTab.get_string(), PATT_NAME_LEN, !szTableName);
  return catalog_emit(stmt, CTLG_TABLE_PRIVILEGES, inpars, parsize);
}

//      Have DBMS set up result set of ColumnPrivileges.

SQLRETURN SQL_API SQLColumnPrivilegesW(HSTMT       hstmt, SQLWCHAR *szTableQualifier,
        SWORD   cbTableQualifier,       SQLWCHAR *szTableOwner, SWORD cbTableOwner,
        SQLWCHAR *szTableName, SWORD   cbTableName, SQLWCHAR *szColumnName,
        SWORD   cbColumnName)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
  wide2ansi xQual(szTableQualifier, cbTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema(szTableOwner,   cbTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab(szTableName,       cbTableName, stmt->cdp->sys_charset);
  wide2ansi xCol(szColumnName,      cbColumnName, stmt->cdp->sys_charset);
 /* pack parameters: */
  unsigned parsize=3*(PATT_NAME_LEN+1);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !szTableOwner);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !szTableQualifier);
#endif
  store_param(inpars+   PATT_NAME_LEN+1,  xTab.get_string(), PATT_NAME_LEN, !szTableName);
  store_param(inpars+2*(PATT_NAME_LEN+1), xCol.get_string(), PATT_NAME_LEN, !szColumnName);
  return catalog_emit(stmt, CTLG_COLUMN_PRIVILEGES, inpars, parsize);
}

//      Have DBMS set up result set of SpecialColumns.

SQLRETURN SQL_API SQLSpecialColumnsW(HSTMT hstmt, UWORD fColType,
  SQLWCHAR *szTableQualifier, SWORD cbTableQualifier, SQLWCHAR *szTableOwner,
        SWORD   cbTableOwner,   SQLWCHAR *szTableName, SWORD   cbTableName,
  UWORD fScope, UWORD   fNullable)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
  wide2ansi xQual(szTableQualifier, cbTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema(szTableOwner,   cbTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab(szTableName,       cbTableName, stmt->cdp->sys_charset);
 /* pack parameters: */
  unsigned parsize=2*(PATT_NAME_LEN+1)+3*sizeof(unsigned);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !szTableOwner);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !szTableQualifier);
#endif
  store_param(inpars+PATT_NAME_LEN+1, xTab.get_string(), PATT_NAME_LEN, !szTableName);
  ((unsigned*)(inpars+2*(PATT_NAME_LEN+1)))[0]=fColType;
  ((unsigned*)(inpars+2*(PATT_NAME_LEN+1)))[1]=fScope;
  ((unsigned*)(inpars+2*(PATT_NAME_LEN+1)))[2]=fNullable;
  return catalog_emit(stmt, CTLG_SPECIAL_COLUMNS, inpars, parsize);
}

//      Have DBMS set up result set of PrimaryKeys.

SQLRETURN SQL_API SQLPrimaryKeysW(HSTMT hstmt,     SQLWCHAR *szTableQualifier,
        SWORD   cbTableQualifier,       SQLWCHAR *szTableOwner, SWORD  cbTableOwner,
        SQLWCHAR *szTableName, SWORD   cbTableName)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
  wide2ansi xQual(szTableQualifier, cbTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema(szTableOwner,   cbTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab(szTableName,       cbTableName, stmt->cdp->sys_charset);
 /* pack parameters: */
  unsigned parsize=2*(PATT_NAME_LEN+1);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !szTableOwner);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !szTableQualifier);
#endif
  store_param(inpars+PATT_NAME_LEN+1, xTab.get_string(), PATT_NAME_LEN, !szTableName);
  return catalog_emit(stmt, CTLG_PRIMARY_KEYS, inpars, parsize);
}

//      Have DBMS set up result set of ForeignKeys.

SQLRETURN SQL_API SQLForeignKeysW(HSTMT hstmt,     SQLWCHAR *szPkTableQualifier,
        SWORD   cbPkTableQualifier,     SQLWCHAR *szPkTableOwner, SWORD        cbPkTableOwner,
        SQLWCHAR *szPkTableName, SWORD cbPkTableName, SQLWCHAR *szFkTableQualifier,
        SWORD   cbFkTableQualifier,     SQLWCHAR *szFkTableOwner,      SWORD   cbFkTableOwner,
        SQLWCHAR *szFkTableName,       SWORD   cbFkTableName)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
  if (szPkTableName==NULL && szFkTableName==NULL) 
  { raise_err(stmt, SQL_HY009);  /* Invalid use of the NULL pointer */
    return SQL_ERROR;
  }
  wide2ansi xQual1(szPkTableQualifier, cbPkTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema1(szPkTableOwner,   cbPkTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab1(szPkTableName,       cbPkTableName, stmt->cdp->sys_charset);
  wide2ansi xQual2(szFkTableQualifier, cbFkTableQualifier, stmt->cdp->sys_charset);
  wide2ansi xSchema2(szFkTableOwner,   cbFkTableOwner, stmt->cdp->sys_charset);
  wide2ansi xTab2(szFkTableName,       cbFkTableName, stmt->cdp->sys_charset);
 /* pack parameters: */
  unsigned parsize=4*(PATT_NAME_LEN+1);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars,                     xSchema1.get_string(), PATT_NAME_LEN, !szPkTableOwner);
#else
  store_param(inpars,                     xQual1.get_string(),   PATT_NAME_LEN, !szPkTableQualifier);
#endif
  store_param(inpars+   PATT_NAME_LEN+1,  xTab1.get_string(),    PATT_NAME_LEN, !szPkTableName);
#ifdef USE_OWNER
  store_param(inpars+2*(PATT_NAME_LEN+1), xSchema2.get_string(), PATT_NAME_LEN, !szFkTableOwner);
#else
  store_param(inpars+2*(PATT_NAME_LEN+1), xQual2.get_string(),   PATT_NAME_LEN, !szFkTableQualifier);
#endif
  store_param(inpars+3*(PATT_NAME_LEN+1), xTab2.get_string(),    PATT_NAME_LEN, !szFkTableName);
  return catalog_emit(stmt, CTLG_FOREIGN_KEYS, inpars, parsize);
}

//      Have DBMS set up result set of Procedures.

SQLRETURN SQL_API SQLProceduresW(SQLHSTMT StatementHandle,
 SQLWCHAR *	CatalogName, SQLSMALLINT	NameLength1,
 SQLWCHAR *	SchemaName, SQLSMALLINT NameLength2,
 SQLWCHAR *	ProcName, SQLSMALLINT	NameLength3)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt = (STMT*)StatementHandle;  CLEAR_ERR(stmt);
  wide2ansi xQual(CatalogName,  NameLength1, stmt->cdp->sys_charset);
  wide2ansi xSchema(SchemaName, NameLength2, stmt->cdp->sys_charset);
  wide2ansi xProc(ProcName,     NameLength3, stmt->cdp->sys_charset);
 // pack parameters: 
  unsigned parsize=2*(PATT_NAME_LEN+1);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !SchemaName);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !CatalogName);
#endif
  store_param(inpars+PATT_NAME_LEN+1, xProc.get_string(), PATT_NAME_LEN, !ProcName);
  return catalog_emit(stmt, CTLG_PROCEDURES, inpars, parsize);
}

//      Have DBMS set up result set of ProcedureColumns.

SQLRETURN SQL_API SQLProcedureColumnsW(SQLHSTMT StatementHandle,
 SQLWCHAR *	CatalogName, SQLSMALLINT	NameLength1,
 SQLWCHAR *	SchemaName, SQLSMALLINT	NameLength2,
 SQLWCHAR *	ProcName, SQLSMALLINT	NameLength3,
 SQLWCHAR *	ColumnName, SQLSMALLINT	NameLength4)
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt = (STMT*)StatementHandle;  CLEAR_ERR(stmt);
  wide2ansi xQual(CatalogName,  NameLength1, stmt->cdp->sys_charset);
  wide2ansi xSchema(SchemaName, NameLength2, stmt->cdp->sys_charset);
  wide2ansi xProc(ProcName,     NameLength3, stmt->cdp->sys_charset);
  wide2ansi xCol(ColumnName,    NameLength4, stmt->cdp->sys_charset);
 // pack parameters: 
  unsigned parsize=3*(PATT_NAME_LEN+1);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  memset(inpars, 0, parsize);
#ifdef USE_OWNER
  store_param(inpars, xSchema.get_string(), PATT_NAME_LEN, !SchemaName);
#else
  store_param(inpars, xQual.get_string(),   PATT_NAME_LEN, !CatalogName);
#endif
  store_param(inpars+   PATT_NAME_LEN+1,  xProc.get_string(), PATT_NAME_LEN, !ProcName);
  store_param(inpars+2*(PATT_NAME_LEN+1), xCol.get_string(),  PATT_NAME_LEN, !ColumnName);
  return catalog_emit(stmt, CTLG_PROCEDURE_COLUMNS, inpars, parsize);
}

//////////////////////////////////// type info ////////////////////////////////

SQLRETURN SQL_API SQLGetTypeInfoW(HSTMT hstmt, SWORD fSqlType)
{ STMT * stmt;
  if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  stmt=(STMT*)hstmt;  CLEAR_ERR(stmt);
 /* pack parameters: */
  unsigned parsize=sizeof(int);
  tptr inpars=(tptr)corealloc(parsize, 39);
  if (inpars==NULL)
  { raise_err(stmt, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  *(int*)inpars=fSqlType;
  return catalog_emit(stmt, CTLG_TYPE_INFO, inpars, parsize);
}

