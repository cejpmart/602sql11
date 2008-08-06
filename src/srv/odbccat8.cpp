// odbccat.cpp - ODBC catalog function implementation
#include "odbc.h"
#include <stddef.h>

#define USE_OWNER
#ifdef USE_OWNER
#define OWNER_LEN  OBJ_NAME_LEN
#define QUALIF_LEN 1        /* string attribute lenght, must not be 0 */
#define applname ownername
#define noname   qualif
#else
#define OWNER_LEN  1   /* string attribute lenght, must not be 0 */
#define QUALIF_LEN OBJ_NAME_LEN
#define applname qualif
#define noname   ownername
#endif



#define PRIVILEGE_NAME_ATTR_SIZE 14  /* strlen(REFERENCES) */
#define INDEX_COLUMN_NAME_LEN  80    /* expressions supported */
#define TYPE_NAME_LEN          14

BOOL match(const char * patt, const char * tx)
{ if (patt==NULL) return TRUE;
  do
  { switch (*patt)
    { case '_': if (!*tx) return FALSE;  break;
      case '%': if (!*(++patt)) return TRUE;  /* speed-up */
        while (!match(patt, tx))
        { if (!*tx) return FALSE;
          tx++;
        }
        return TRUE;
      case 0: return *tx==0;
      case '\\': if (!*(++patt)) return FALSE;  /* ESC on the patter end */
        /* cont. */
      default: if (*patt != *tx) return FALSE;  break;
    }
    patt++;  tx++;
  } while (TRUE);
}

BOOL mmatch(BOOL metadata, const char * patt, const char * tx)
{ if (metadata) return !strcmp(patt, tx);
  return match(patt, tx);
}

/////////////////////////////////////////// type info ////////////////////////////////////////
typedef struct
{ uns8 del;
  char  type_name[TYPE_NAME_LEN+1];
  sig16 data_type;
  sig32 column_size;
  char  literal_prefix[11+1];
  char  literal_suffix[2+1];
  char  create_params[10+1];
  sig16 nullable;
  sig16 case_sensitive;
  sig16 searchable;
  sig16 unsigned_attribute;
  sig16 fixed_prec_scale;
  sig16 auto_unique_value;
  char  local_type_name[OBJ_NAME_LEN+1];
  sig16 minimum_scale;
  sig16 maximum_scale;
  sig16 sql_data_type;
  sig16 sql_datetime_sub;
  sig32 num_prec_radix;
  sig16 interval_precision;
} result_type_info;

static const t_colval type_info_coldescr[] = {
{  1, NULL, (void*)offsetof(result_type_info, type_name), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_type_info, data_type), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_type_info, column_size), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_type_info, literal_prefix), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_type_info, literal_suffix), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_type_info, create_params), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_type_info, nullable), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_type_info, case_sensitive), NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_type_info, searchable), NULL, NULL, 0, NULL },
{ 10, NULL, (void*)offsetof(result_type_info, unsigned_attribute), NULL, NULL, 0, NULL },
{ 11, NULL, (void*)offsetof(result_type_info, fixed_prec_scale), NULL, NULL, 0, NULL },
{ 12, NULL, (void*)offsetof(result_type_info, auto_unique_value), NULL, NULL, 0, NULL },
{ 13, NULL, (void*)offsetof(result_type_info, local_type_name), NULL, NULL, 0, NULL },
{ 14, NULL, (void*)offsetof(result_type_info, minimum_scale), NULL, NULL, 0, NULL },
{ 15, NULL, (void*)offsetof(result_type_info, maximum_scale), NULL, NULL, 0, NULL },
{ 16, NULL, (void*)offsetof(result_type_info, sql_data_type), NULL, NULL, 0, NULL },
{ 17, NULL, (void*)offsetof(result_type_info, sql_datetime_sub), NULL, NULL, 0, NULL },
{ 18, NULL, (void*)offsetof(result_type_info, num_prec_radix), NULL, NULL, 0, NULL },
{ 19, NULL, (void*)offsetof(result_type_info, interval_precision), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_type_info[] = {
 { "TYPE_NAME",          ATT_STRING, TYPE_NAME_LEN, 2, },
 { "DATA_TYPE",          ATT_INT16,  0, 1, },
 { "COLUMN_SIZE",        ATT_INT32,  0, 0, },
 { "LITERAL_PREFIX",     ATT_STRING, 11,0, },
 { "LITERAL_SUFFIX",     ATT_STRING, 2, 0, },
 { "CREATE_PARAMS",      ATT_STRING, 10,0, },
 { "NULLABLE",           ATT_INT16,  0, 0, },
 { "CASE_SENSITIVE",     ATT_INT16,  0, 0, },
 { "SEARCHABLE",         ATT_INT16,  0, 0, },
 { "UNSIGNED_ATTRIBUTE", ATT_INT16,  0, 0, },
 { "FIXED_PREC_SCALE",   ATT_INT16,  0, 0, },
 { "AUTO_UNIQUE_VALUE",  ATT_INT16,  0, 0, },
 { "LOCAL_TYPE_NAME",    ATT_STRING, OBJ_NAME_LEN, 0, },
 { "MINIMUM_SCALE",      ATT_INT16,  0, 0, },
 { "MAXIMUM_SCALE",      ATT_INT16,  0, 0, },
 { "SQL_DATA_TYPE",	     ATT_INT16,  0, 0, },
 { "SQL_DATETIME_SUB",	 ATT_INT16,  0, 0, },
 { "NUM_PREC_RADIX",     ATT_INT32,  0, 0, },
 { "INTERVAL_PRECISION", ATT_INT16,  0, 0, },
 { "", 0, 0, 0 } };

///////////////////////////////////////// column privileges //////////////////////////////////
typedef struct
{ uns8 del;
  char qualif   [QUALIF_LEN+1];
  char ownername[OWNER_LEN+1];
  char tabname  [OBJ_NAME_LEN+1];
  char colname  [ATTRNAMELEN+1];
  char grantor  [OBJ_NAME_LEN+1];
  char grantee  [OBJ_NAME_LEN+1];
  char privilege[PRIVILEGE_NAME_ATTR_SIZE+1];
  char is_grantable[3+1];
} result_column_privileges;

static const t_colval column_privileges_coldescr[] = {
{  1, NULL, (void*)offsetof(result_column_privileges, qualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_column_privileges, ownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_column_privileges, tabname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_column_privileges, colname), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_column_privileges, grantor), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_column_privileges, grantee), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_column_privileges, privilege), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_column_privileges, is_grantable), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_column_privileges[] = {
 { "TABLE_CAT",    ATT_STRING, QUALIF_LEN  ,1 },
 { "TABLE_SCHEM",  ATT_STRING, OWNER_LEN   ,2 },
 { "TABLE_NAME",   ATT_STRING, OBJ_NAME_LEN,3 },
 { "COLUMN_NAME",  ATT_STRING, ATTRNAMELEN ,4 },
 { "GRANTOR",      ATT_STRING, OBJ_NAME_LEN,0 },
 { "GRANTEE",      ATT_STRING, OBJ_NAME_LEN,0 },
 { "PRIVILEGE",    ATT_STRING, PRIVILEGE_NAME_ATTR_SIZE,5 },
 { "IS_GRANTABLE", ATT_STRING, 3           ,0 },
 { "", 0, 0, 0 } };

/////////////////////////////////////// columns ////////////////////////////////////////
#define COLUMN_DEF_MAX 40

typedef struct
{ uns8 del;
  char qualif   [QUALIF_LEN+1];
  char ownername[OWNER_LEN+1];
  char tabname  [OBJ_NAME_LEN+1];
  char colname  [ATTRNAMELEN+1];
  sig16 data_type;
  char type_name[TYPE_NAME_LEN+1];
  sig32 column_size;
  sig32 buffer_length;
  sig16 decimal_digits;
  sig16 num_prec_radix;
  sig16 nullable;
  char remarks  [1+1];
  char column_def[COLUMN_DEF_MAX+1];
  sig16 sql_data_type;
  sig16 sql_datetime_sub;
  sig32 char_octet_length;
  sig32 ordinal_position;
  char  is_nullable[3+1];
} result_columns;

static const t_colval columns_coldescr[] = {
{  1, NULL, (void*)offsetof(result_columns, qualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_columns, ownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_columns, tabname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_columns, colname), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_columns, data_type), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_columns, type_name), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_columns, column_size), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_columns, buffer_length), NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_columns, decimal_digits), NULL, NULL, 0, NULL },
{ 10, NULL, (void*)offsetof(result_columns, num_prec_radix), NULL, NULL, 0, NULL },
{ 11, NULL, (void*)offsetof(result_columns, nullable), NULL, NULL, 0, NULL },
{ 12, NULL, (void*)offsetof(result_columns, remarks), NULL, NULL, 0, NULL },
{ 13, NULL, (void*)offsetof(result_columns, column_def), NULL, NULL, 0, NULL },
{ 14, NULL, (void*)offsetof(result_columns, sql_data_type), NULL, NULL, 0, NULL },
{ 15, NULL, (void*)offsetof(result_columns, sql_datetime_sub), NULL, NULL, 0, NULL },
{ 16, NULL, (void*)offsetof(result_columns, char_octet_length), NULL, NULL, 0, NULL },
{ 17, NULL, (void*)offsetof(result_columns, ordinal_position), NULL, NULL, 0, NULL },
{ 18, NULL, (void*)offsetof(result_columns, is_nullable), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_columns[] = {
 { "TABLE_CAT",   ATT_STRING, QUALIF_LEN  ,1 },
 { "TABLE_SCHEM", ATT_STRING, OWNER_LEN   ,2 },
 { "TABLE_NAME",  ATT_STRING, OBJ_NAME_LEN,3 },
 { "COLUMN_NAME", ATT_STRING, ATTRNAMELEN ,0 },
 { "DATA_TYPE",      ATT_INT16,  0           ,0 },
 { "TYPE_NAME",      ATT_STRING, TYPE_NAME_LEN,0 },
 { "COLUMN_SIZE",    ATT_INT32,  0           ,0 },
 { "BUFFER_LENGTH",  ATT_INT32,  0           ,0 },
 { "DECIMAL_DIGITS", ATT_INT16,  0           ,0 },
 { "NUM_PREC_RADIX", ATT_INT16,  0           ,0 },
 { "NULLABLE",   ATT_INT16,  0           ,0 },
 { "REMARKS",    ATT_STRING, 1           ,0 },
 { "COLUMN_DEF",	   ATT_STRING, COLUMN_DEF_MAX, 0 },
 { "SQL_DATA_TYPE",  ATT_INT16,  0,  0 },  
 { "SQL_DATETIME_SUB",ATT_INT16, 0, 0 },
 { "CHAR_OCTET_LENGTH", ATT_INT32, 0, 0 }, 	
 { "ORDINAL_POSITION", ATT_INT32, 0, 4 },
 { "IS_NULLABLE", ATT_STRING, 3, 0 },
 { "", 0, 0, 0 } };

//////////////////////////////////////////// foreign keys /////////////////////////////
typedef struct
{ uns8 del;
  char pqualif   [QUALIF_LEN+1];
  char pownername[OWNER_LEN+1];
  char ptabname  [OBJ_NAME_LEN+1];
  char pcolname  [ATTRNAMELEN+1];
  char fqualif   [QUALIF_LEN+1];
  char fownername[OWNER_LEN+1];
  char ftabname  [OBJ_NAME_LEN+1];
  char fcolname  [ATTRNAMELEN+1];
  sig16 key_seq;
  sig16 update_rule;
  sig16 delete_rule;
  char fk_name   [OBJ_NAME_LEN+1];
  char pk_name   [OBJ_NAME_LEN+1];
  sig16 deferrability;
} result_foreign_keys;

static const t_colval foreign_keys_coldescr[] = {
{  1, NULL, (void*)offsetof(result_foreign_keys, pqualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_foreign_keys, pownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_foreign_keys, ptabname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_foreign_keys, pcolname), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_foreign_keys, fqualif), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_foreign_keys, fownername), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_foreign_keys, ftabname), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_foreign_keys, fcolname), NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_foreign_keys, key_seq), NULL, NULL, 0, NULL },
{ 10, NULL, (void*)offsetof(result_foreign_keys, update_rule), NULL, NULL, 0, NULL },
{ 11, NULL, (void*)offsetof(result_foreign_keys, delete_rule), NULL, NULL, 0, NULL },
{ 12, NULL, (void*)offsetof(result_foreign_keys, fk_name), NULL, NULL, 0, NULL },
{ 13, NULL, (void*)offsetof(result_foreign_keys, pk_name), NULL, NULL, 0, NULL },
{ 14, NULL, (void*)offsetof(result_foreign_keys, deferrability), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_foreign_keys[] = {
 { "PKTABLE_CAT",    ATT_STRING, QUALIF_LEN  ,1 },
 { "PKTABLE_SCHEM",  ATT_STRING, OWNER_LEN   ,2 },
 { "PKTABLE_NAME",   ATT_STRING, OBJ_NAME_LEN,3 },
 { "PKCOLUMN_NAME",  ATT_STRING, ATTRNAMELEN ,0 },
 { "FKTABLE_CAT",    ATT_STRING, QUALIF_LEN  ,4 },
 { "FKTABLE_SCHEM",  ATT_STRING, OWNER_LEN   ,5 },
 { "FKTABLE_NAME",   ATT_STRING, OBJ_NAME_LEN,6 },
 { "FKCOLUMN_NAME",  ATT_STRING, ATTRNAMELEN ,0 },
 { "KEY_SEQ",     ATT_INT16,  0           ,7 },
 { "UPDATE_RULE", ATT_INT16,  0           ,0 },
 { "DELETE_RULE", ATT_INT16,  0           ,0 },
 { "FK_NAME",     ATT_STRING, OBJ_NAME_LEN,0 },
 { "PK_NAME",     ATT_STRING, OBJ_NAME_LEN,0 },
 { "", 0, 0, 0 } };

///////////////////////////////////////////// primary keys /////////////////////////////
typedef struct
{ uns8 del;
  char qualif   [QUALIF_LEN+1];
  char ownername[OWNER_LEN+1];
  char tabname  [OBJ_NAME_LEN+1];
  char colname  [ATTRNAMELEN+1];
  sig16 key_seq;
  char pk_name  [OBJ_NAME_LEN+1];
} result_primary_keys;

static const t_colval primary_keys_coldescr[] = {
{  1, NULL, (void*)offsetof(result_primary_keys, qualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_primary_keys, ownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_primary_keys, tabname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_primary_keys, colname), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_primary_keys, key_seq), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_primary_keys, pk_name), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_primary_keys[] = {
 { "TABLE_CAT",   ATT_STRING, QUALIF_LEN  ,1 },
 { "TABLE_SCHEM", ATT_STRING, OWNER_LEN   ,2 },
 { "TABLE_NAME",  ATT_STRING, OBJ_NAME_LEN,3 },
 { "COLUMN_NAME", ATT_STRING, ATTRNAMELEN ,0 },
 { "KEY_SEQ",     ATT_INT16,  0           ,4 },
 { "PK_NAME",     ATT_STRING, OBJ_NAME_LEN,0 },
 { "", 0, 0, 0 } };

/////////////////////////////////////// special columns //////////////////////////
typedef struct
{ uns8 del;
  sig16 scope;
  char  colname  [ATTRNAMELEN+1];
  sig16 data_type;
  char  type_name[TYPE_NAME_LEN+1];
  sig32 column_size;
  sig32 buffer_length;
  sig16 decimal_digits;
  sig16 pseudo_column;
} result_special_columns;

static const t_colval special_columns_coldescr[] = {
{  1, NULL, (void*)offsetof(result_special_columns, scope), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_special_columns, colname), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_special_columns, data_type), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_special_columns, type_name), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_special_columns, column_size), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_special_columns, buffer_length), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_special_columns, decimal_digits), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_special_columns, pseudo_column), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_special_columns[] = {
 { "SCOPE",         ATT_INT16,  0           ,1 },
 { "COLUMN_NAME",   ATT_STRING, ATTRNAMELEN ,0 },
 { "DATA_TYPE",     ATT_INT16,  0           ,0 },
 { "TYPE_NAME",     ATT_STRING, TYPE_NAME_LEN,0 },
 { "COLUMN_SIZE",   ATT_INT32,  0           ,0 },
 { "BUFFER_LENGTH", ATT_INT32,  0           ,0 },
 { "DECIMAL_DIGITS",ATT_INT16,  0           ,0 },
 { "PSEUDO_COLUMN", ATT_INT16,  0           ,0 },
 { "", 0, 0, 0 } };

////////////////////////////////////////// statistics /////////////////////////////
typedef struct
{ uns8 del;
  char qualif   [QUALIF_LEN+1];
  char ownername[OWNER_LEN+1];
  char tabname  [OBJ_NAME_LEN+1];
  sig16 non_unique;
  char index_qualifier[OBJ_NAME_LEN+1];
  char index_name[OBJ_NAME_LEN+1];
  sig16 type;
  sig16 ordinal_position;
  char column_name[INDEX_COLUMN_NAME_LEN+1];
  char asc_or_desc;
  sig32 cardinality;
  sig32 pages;
  char filter_condition[1+1];
} result_statistics;

static const t_colval statistics_coldescr[] = {
{  1, NULL, (void*)offsetof(result_statistics, qualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_statistics, ownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_statistics, tabname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_statistics, non_unique), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_statistics, index_qualifier), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_statistics, index_name), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_statistics, type), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_statistics, ordinal_position), NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_statistics, column_name), NULL, NULL, 0, NULL },
{ 10, NULL, (void*)offsetof(result_statistics, asc_or_desc), NULL, NULL, 0, NULL },
{ 11, NULL, (void*)offsetof(result_statistics, cardinality), NULL, NULL, 0, NULL },
{ 12, NULL, (void*)offsetof(result_statistics, pages), NULL, NULL, 0, NULL },
{ 13, NULL, (void*)offsetof(result_statistics, filter_condition), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_statistics[] = {
 { "TABLE_CAT",        ATT_STRING, QUALIF_LEN  ,0 },
 { "TABLE_SCHEM",      ATT_STRING, OWNER_LEN   ,0 },
 { "TABLE_NAME",       ATT_STRING, OBJ_NAME_LEN,0 },
 { "NON_UNIQUE",       ATT_INT16,  0           ,1 },
 { "INDEX_QUALIFIER",  ATT_STRING, OBJ_NAME_LEN,3 },
 { "INDEX_NAME",       ATT_STRING, OBJ_NAME_LEN,4 },
 { "TYPE",             ATT_INT16,  0           ,2 },
 { "ORDINAL_POSITION", ATT_INT16,  0           ,5 },
 { "COLUMN_NAME",      ATT_STRING, INDEX_COLUMN_NAME_LEN,0 },
 { "ASC_OR_DESC",      ATT_STRING, 1           ,0 },
 { "CARDINALITY",      ATT_INT32,  0           ,0 },
 { "PAGES",            ATT_INT32,  0           ,0 },
 { "FILTER_CONDITION", ATT_STRING, 1           ,0 },
 { "", 0, 0, 0 } };

//////////////////////////////////////////// table privileges ////////////////////////////
typedef struct
{ uns8 del;
  char qualif   [QUALIF_LEN+1];
  char ownername[OWNER_LEN+1];
  char tabname  [OBJ_NAME_LEN+1];
  char grantor  [OBJ_NAME_LEN+1];
  char grantee  [OBJ_NAME_LEN+1];
  char privilege[PRIVILEGE_NAME_ATTR_SIZE+1];
  char is_grantable[3];
} result_table_privileges;

static const t_colval table_privileges_coldescr[] = {
{  1, NULL, (void*)offsetof(result_table_privileges, qualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_table_privileges, ownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_table_privileges, tabname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_table_privileges, grantor), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_table_privileges, grantee), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_table_privileges, privilege), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_table_privileges, is_grantable), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_table_privileges[] = {
 { "TABLE_CAT",       ATT_STRING, QUALIF_LEN  ,1 },
 { "TABLE_SCHEM",     ATT_STRING, OWNER_LEN   ,2 },
 { "TABLE_NAME",      ATT_STRING, OBJ_NAME_LEN,3 },
 { "GRANTOR",         ATT_STRING, OBJ_NAME_LEN,0 },
 { "GRANTEE",         ATT_STRING, OBJ_NAME_LEN,0 },
 { "PRIVILEGE",       ATT_STRING, PRIVILEGE_NAME_ATTR_SIZE,4 },
 { "IS_GRANTABLE",    ATT_STRING, 3           ,0 },
 { "", 0, 0, 0 } };

///////////////////////////////////////////// tables ///////////////////////////////////
#define TABLE_TYPE_ATTR_SIZE 14  /* strlen('SYSTEM_TABLE') */
typedef struct
{ uns8 del;
  char qualif   [QUALIF_LEN+1];
  char ownername[OWNER_LEN+1];
  char tabname  [OBJ_NAME_LEN+1];
  char tabtype  [TABLE_TYPE_ATTR_SIZE+1];
  char remarks  [1+1];
} result_tables;

static const t_colval tables_coldescr[] = {
{  1, NULL, (void*)offsetof(result_tables, qualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_tables, ownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_tables, tabname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_tables, tabtype), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_tables, remarks), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_tables[] = {
 { "TABLE_CAT",       ATT_STRING, QUALIF_LEN  ,2 },
 { "TABLE_SCHEM",     ATT_STRING, OWNER_LEN   ,3 },
 { "TABLE_NAME",      ATT_STRING, OBJ_NAME_LEN,4 },
 { "TABLE_TYPE",      ATT_STRING, TABLE_TYPE_ATTR_SIZE,1 },
 { "REMARKS",         ATT_STRING, 1           ,0 },
 { "", 0, 0, 0 } };

/////////////////////////////////////// procedures ///////////////////////////////
typedef struct
{ uns8  del;
  char  qualif   [QUALIF_LEN+1];
  char  ownername[OWNER_LEN+1];
  char  procname [OBJ_NAME_LEN+1];
  sig32 num_input_params, num_output_params, num_result_sets;
  char  remarks  [1+1];
  sig16 procedure_type;
} result_procedures;

static const t_colval procedure_coldescr[] = {
{  1, NULL, (void*)offsetof(result_procedures, qualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_procedures, ownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_procedures, procname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_procedures, num_input_params), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_procedures, num_output_params), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_procedures, num_result_sets), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_procedures, remarks), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_procedures, procedure_type), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_procedures[] = {
 { "PROCEDURE_CAT",       ATT_STRING, QUALIF_LEN  ,1 },
 { "PROCEDURE_SCHEM",     ATT_STRING, OWNER_LEN   ,2 },
 { "PROCEDURE_NAME",      ATT_STRING, OBJ_NAME_LEN,3 },
 { "NUM_INPUT_PARAMS",    ATT_INT32,  0           ,0 },
 { "NUM_OUTPUT_PARAMS",   ATT_INT32,  0           ,0 },
 { "NUM_RESULT_SETS",     ATT_INT32,  0           ,0 },
 { "REMARKS",             ATT_STRING, 1           ,0 },
 { "PROCEDURE_TYPE",      ATT_INT16,  0           ,0 },
 { "", 0, 0, 0 } };

////////////////////////////////////////// procedure columns ///////////////////////////////
typedef struct
{ uns8 del;
  char qualif   [QUALIF_LEN+1];
  char ownername[OWNER_LEN+1];
  char procname [OBJ_NAME_LEN+1];
  char colname  [ATTRNAMELEN+1];
  sig16 column_type;
  sig16 data_type;
  char type_name[TYPE_NAME_LEN+1];
  sig32 column_size;
  sig32 buffer_length;
  sig16 decimal_digits;
  sig16 num_prec_radix;
  sig16 nullable;
  char remarks  [1+1];
  char column_def[COLUMN_DEF_MAX+1];
  sig16 sql_data_type;
  sig16 sql_datetime_sub;
  sig32 char_octet_length;
  sig32 ordinal_position;
  char  is_nullable[3+1];
} result_procedure_columns;

static const t_colval procedure_columns_coldescr[] = {
{  1, NULL, (void*)offsetof(result_procedure_columns, qualif), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_procedure_columns, ownername), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_procedure_columns, procname), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_procedure_columns, colname), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_procedure_columns, column_type), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_procedure_columns, data_type), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_procedure_columns, type_name), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_procedure_columns, column_size), NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_procedure_columns, buffer_length), NULL, NULL, 0, NULL },
{ 10, NULL, (void*)offsetof(result_procedure_columns, decimal_digits), NULL, NULL, 0, NULL },
{ 11, NULL, (void*)offsetof(result_procedure_columns, num_prec_radix), NULL, NULL, 0, NULL },
{ 12, NULL, (void*)offsetof(result_procedure_columns, nullable), NULL, NULL, 0, NULL },
{ 13, NULL, (void*)offsetof(result_procedure_columns, remarks), NULL, NULL, 0, NULL },
{ 14, NULL, (void*)offsetof(result_procedure_columns, column_def), NULL, NULL, 0, NULL },
{ 15, NULL, (void*)offsetof(result_procedure_columns, sql_data_type), NULL, NULL, 0, NULL },
{ 16, NULL, (void*)offsetof(result_procedure_columns, sql_datetime_sub), NULL, NULL, 0, NULL },
{ 17, NULL, (void*)offsetof(result_procedure_columns, char_octet_length), NULL, NULL, 0, NULL },
{ 18, NULL, (void*)offsetof(result_procedure_columns, ordinal_position), NULL, NULL, 0, NULL },
{ 19, NULL, (void*)offsetof(result_procedure_columns, is_nullable), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL }
};

const t_ivcols ctlg_procedure_columns[] = {
 { "PROCEDURE_CAT",  ATT_STRING, QUALIF_LEN  ,1 },
 { "PROCEDURE_SCHEM",ATT_STRING, OWNER_LEN   ,2 },
 { "PROCEDURE_NAME", ATT_STRING, OBJ_NAME_LEN,3 },
 { "COLUMN_NAME",    ATT_STRING, OBJ_NAME_LEN,0 },
 { "COLUMN_TYPE",    ATT_INT16,  0           ,4 },
 { "DATA_TYPE",      ATT_INT16,  0           ,0 },
 { "TYPE_NAME",      ATT_STRING, TYPE_NAME_LEN,0 },
 { "COLUMN_SIZE",    ATT_INT32,  0           ,0 },
 { "BUFFER_LENGTH",  ATT_INT32,  0           ,0 },
 { "DECIMAL_DIGITS", ATT_INT16,  0           ,0 },
 { "NUM_PREC_RADIX", ATT_INT16,  0           ,0 },
 { "NULLABLE",       ATT_INT16,  0           ,0 },
 { "REMARKS",        ATT_STRING, 1           ,0 },
 { "COLUMN_DEF",	   ATT_STRING, COLUMN_DEF_MAX, 0 },
 { "SQL_DATA_TYPE",    ATT_INT16,  0, 0 },  
 { "SQL_DATETIME_SUB", ATT_INT16,  0, 0 },
 { "CHAR_OCTET_LENGTH",ATT_INT32,  0, 0 }, 	
 { "ORDINAL_POSITION", ATT_INT32,  0, 5 },
 { "IS_NULLABLE",      ATT_STRING, 3, 0 },
 { "", 0, 0, 0 } };
 
///////////////////////////////////////////////////////////////////////////////////////////
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

void anal_odbc_param(cdp_t cdp, unsigned & ctlg_type, BOOL & meta, BOOL & odbc2, char *& specif_params)
{ char * request = cdp->temp_param;
  if (!request) specif_params=NULL;
  else
  { meta       = *request>0;  request++;
    ctlg_type  = *(uns16*)request;  request+=sizeof(uns16);
    uns16 size = *(uns16*)request;  request+=sizeof(uns16);
   // determine ODBC 2 or ODBC 3
    if (ctlg_type>=1000) { odbc2=TRUE;  ctlg_type-=1000; }
    else odbc2=FALSE;
    specif_params = request;
  }
}       

void fill_type_info(cdp_t cdp, table_descr * tbdf)
// Should return each type sorted on closeness to the SQL type (closer first).
// Now, multiple WB type for an SQL type are only the strings.
// GetTypeInfo need not to return all types in the data source. Not returned types may be used in result sets.
{ int i;  result_type_info rss;  
  t_vector_descr vector(type_info_coldescr, &rss);
  rss.del=0;
  unsigned ctlg_type;  BOOL meta, odbc2;  char * specif_params;
  anal_odbc_param(cdp, ctlg_type, meta, odbc2, specif_params);
  int fSqlType = specif_params ? *(int*)specif_params : SQL_ALL_TYPES;
  for (i=ATT_BOOLEAN;  i<ATT_AFTERLAST;  i==ATT_LAST_HEAPTYPE ? i=ATT_INT8 : i++)
    if (i!=ATT_PTR && i!=ATT_BIPTR && i!=ATT_SIGNAT) 
    if (i!=ATT_DATIM && i!=ATT_AUTOR && i!=ATT_HIST)
    if (i!=ATT_STRING+1 && i!=ATT_STRING+2 && i!=ATT_RASTER && i!=ATT_DOMAIN)
    /* if ptr or biptr types are advertised, MS Query 32 cannot sort on Integer attribues! */
    if (fSqlType==SQL_ALL_TYPES || type_WB_to_sql(i, t_specif(), odbc2)==fSqlType)
    { sql_type_name(i, t_specif(), false, rss.type_name);
      rss.data_type=type_WB_to_sql(i, t_specif(), odbc2);
      rss.column_size=simple_column_size(rss.data_type, t_specif(254,0,FALSE,FALSE));
      strcpy(rss.literal_prefix, literal_prefix(i));
      strcpy(rss.literal_suffix, literal_suffix(i));
      if (is_string(i) || i==ATT_BINARY) strcpy(rss.create_params, "Délka");
      else if (i==ATT_RASTER) strcpy(rss.create_params, "MaxDélka");
      else if (i==ATT_MONEY) strcpy(rss.create_params, "Délka,Dec");
      //else if (i==ATT_PTR || i==ATT_BIPTR) strcpy(rss.create_params, "Cil");
      else rss.create_params[0]=0;
      rss.nullable=SQL_NULLABLE;
      rss.case_sensitive = i==ATT_CHAR || i==ATT_STRING || i==ATT_TEXT;
      rss.searchable=is_string(i) ? SQL_SEARCHABLE :
                        i<ATT_PTR || i==ATT_INT8 || i==ATT_INT64 ? SQL_PRED_BASIC : SQL_PRED_NONE;
      rss.unsigned_attribute=(sig16)(WB_type_numeric(i) ? FALSE : NONESHORT);
      rss.fixed_prec_scale=(sig16)(i==ATT_MONEY);
      rss.auto_unique_value =(sig16)(WB_type_numeric(i) ? FALSE : NONESHORT);
      sql_type_name(i, t_specif(), false, rss.local_type_name);
      rss.minimum_scale=rss.maximum_scale=
        i==ATT_MONEY ? 2 : i==ATT_TIME ? 3 : 
        WB_type_numeric(i) || i==ATT_TIMESTAMP ? 0 : NONESHORT;
      rss.sql_data_type = rss.data_type==SQL_TYPE_DATE || rss.data_type==SQL_TYPE_TIME || rss.data_type==SQL_TYPE_TIMESTAMP ? SQL_DATETIME : rss.data_type;
      rss.sql_datetime_sub = rss.data_type==SQL_TYPE_DATE ? SQL_CODE_DATE :
       rss.data_type==SQL_TYPE_TIME ? SQL_CODE_TIME :
       rss.data_type==SQL_TYPE_TIMESTAMP ? SQL_CODE_TIMESTAMP : NONESHORT;
      rss.num_prec_radix=WB_type_numeric(i) ? 10 : NONEINTEGER;
      rss.interval_precision=NONESHORT;
      tb_new(cdp, tbdf, FALSE, &vector);
    }
}

#define MAX_ROWID_PARTS 20  // technical limitation, no special significance

static int find_best_rowid(cdp_t cdp, ttablenum tb, tattrib * attrs, unsigned fnullable)
{ table_all ta;  int i, j;  char namebuf[OBJ_NAME_LEN+11];  int cnt, len;
  tptr p, atrstart;
  if (partial_compile_to_ta(cdp, tb, &ta)) return 0;
  for (i=0;  i<ta.indxs.count();  i++)
  { ind_descr * indx = ta.indxs.acc(i);
    if (indx->index_type==INDEX_PRIMARY || indx->index_type==INDEX_UNIQUE)
    { p=indx->text;
      cnt=0;
      do
      { if (cnt>=MAX_ROWID_PARTS) goto next_index;
        while (*p==' ') p++;
        atrstart=p;
        while (*p && *p!=',') p++;
        if (atrstart+OBJ_NAME_LEN+10 < p) goto next_index;
        memcpy(namebuf, atrstart, p-atrstart);  namebuf[p-atrstart]=0;
        cutspaces(namebuf);
        len=(int)strlen(namebuf);
        if (!memicmp(namebuf+len-3, "ASC",  3)) namebuf[len-3]=0;
        if (!memicmp(namebuf+len-4, "DESC", 4)) namebuf[len-4]=0;
        cutspaces(namebuf);
        len=(int)strlen(namebuf);
        if (namebuf[0]=='`') { memmov(namebuf, namebuf+1, len);  len--; }
        if (namebuf[len-1]=='`') namebuf[len-1]=0;
        cutspaces(namebuf);
        for (j=1;  j<ta.attrs.count();  j++)
          if (!sys_stricmp(ta.attrs.acc(j)->name, namebuf))
            break;
        if (j>=ta.attrs.count()) goto next_index;
        if (fnullable==SQL_NO_NULLS && ta.attrs.acc(j)->nullable) goto next_index;
        attrs[cnt++]=j;
      } while (*(p++)==',');
      return cnt;
    }
    next_index:;
  }
 /* no suitable index found */
  return 0;
}

static void column_privilege_record(cdp_t cdp, BOOL meta, table_descr * dest_tbdf, trecnum rec, tptr anamepatt, result_column_privileges * rs, BOOL is_table, void * def)
{ trecnum rec2;  char aname[OBJ_NAME_LEN+1];  BOOL rdri, wrri;  uns8 del;
  int attrcnt;  table_descr * tbdf;  d_table * td;  tcateg categ;
  t_vector_descr vector(column_privileges_coldescr, rs);
  t_privils_flex priv_val;
  if (is_table)
    { tbdf=(table_descr*)def;  attrcnt=tbdf->attrcnt; }
  else
    { td=(d_table*)def;  attrcnt=td->attrcnt; }

  for (rec2=0;  rec2<usertab_descr->Recnum();  rec2++)
    if (!tb_read_atr(cdp, usertab_descr, rec2, DEL_ATTR_NUM, (tptr)&del) && !del)
      if (!tb_read_atr(cdp, usertab_descr, rec2, USER_ATR_CATEG, (tptr)&categ))
        if (!tb_read_atr(cdp, usertab_descr, rec2, USER_ATR_LOGNAME, rs->grantee))
        { if (is_table)
          { privils prvs(cdp);
            prvs.set_user((tobjnum)rec2, categ, TRUE);
            prvs.get_effective_privils(tbdf, NORECNUM, priv_val);
            memcpy(rs->is_grantable, (*priv_val.the_map() & RIGHT_GRANT) ? "YES" : "NO", 3);
          }
          else rs->is_grantable[0]=0; // "grantable" undefined for cursor
          for (int i=1;  i<attrcnt;  i++)
          { if (is_table)
            { memcpy(aname, tbdf->attrs[i].name, ATTRNAMELEN);
              rdri=priv_val.has_read(i);
              wrri=priv_val.has_write(i);
            }
            else
            { memcpy(aname, ATTR_N(td,i)->name,  ATTRNAMELEN);
              rdri=ATTR_N(td,i)->a_flags & RI_SELECT_FLAG;
              wrri=ATTR_N(td,i)->a_flags & RI_UPDATE_FLAG;
            }
            if (mmatch(meta, anamepatt, aname))
            { memcpy(rs->colname, aname, ATTRNAMELEN);
              if (rdri)
              { memcpy(rs->privilege, "SELECT", PRIVILEGE_NAME_ATTR_SIZE);
                tb_new(cdp, dest_tbdf, FALSE, &vector);
                memcpy(rs->privilege, "REFERENCES", PRIVILEGE_NAME_ATTR_SIZE);
                tb_new(cdp, dest_tbdf, FALSE, &vector);
              }
              if (wrri)
              { memcpy(rs->privilege, "UPDATE", PRIVILEGE_NAME_ATTR_SIZE);
                tb_new(cdp, dest_tbdf, FALSE, &vector);
                memcpy(rs->privilege, "INSERT", PRIVILEGE_NAME_ATTR_SIZE);
                tb_new(cdp, dest_tbdf, FALSE, &vector);
              }
            }
          }
        }
}

void column_info1(int type, t_specif specif, sig16 * data_type, char * type_name,
    sig32 * column_size, sig32 * buffer_length, sig16 * decimal_digits, BOOL odbc2)
{ *data_type=type_WB_to_sql(type, specif, odbc2);  // the concise type
  sql_type_name(type, specif, false, type_name);
  *column_size=simple_column_size(*data_type, specif);
  *buffer_length=speciflen(type) ? specif.length : WB_type_length(type);
  *decimal_digits=type==ATT_MONEY ? 2 : type==ATT_TIME ? 3 : 
     type==ATT_INT8  || type==ATT_INT16 || type==ATT_INT32 || type==ATT_INT64 ? specif.scale :
     type==ATT_FLOAT || type==ATT_TIMESTAMP ? 0 : NONESHORT;
}

static void column_info2(int type, t_specif specif, int length, BOOL WBnullable, sig16 data_type,
  sig16 * num_prec_radix, sig16 * nullable, 
  char * column_def, sig16 * sql_data_type, sig16 * sql_datetime_sub, 
  sig32 * char_octet_length, char * is_nullable, const char * defval_spec)
{ *num_prec_radix=WB_type_numeric(type) ? 10 : NONESHORT;
  *nullable =WBnullable ? SQL_NULLABLE : SQL_NO_NULLS;
  strmaxcpy(column_def, defval_spec==NULL ? "NULL" : 
    !*defval_spec || strlen(defval_spec)>=COLUMN_DEF_MAX ? "TRUNCATED" : defval_spec, COLUMN_DEF_MAX);
  *sql_data_type = data_type==SQL_TYPE_DATE || data_type==SQL_TYPE_TIME || data_type==SQL_TYPE_TIMESTAMP ? SQL_DATETIME : data_type;
  *sql_datetime_sub = data_type==SQL_TYPE_DATE ? SQL_CODE_DATE :
    data_type==SQL_TYPE_TIME ? SQL_CODE_TIME :
    data_type==SQL_TYPE_TIMESTAMP ? SQL_CODE_TIMESTAMP : NONESHORT;
  *char_octet_length=IS_HEAP_TYPE(type) ? SQL_NO_TOTAL :
    is_string(type) || type==ATT_BINARY ? length :
    type==ATT_CHAR ? (specif.wide_char ? 2 : 1) : NONEINTEGER;
  memcpy(is_nullable, WBnullable ? "YES" : "NO", 3);
}

static void column_record(cdp_t cdp, BOOL meta, table_descr * dest_tbdf, tptr anamepatt, result_columns * rs, BOOL is_table, void * def, BOOL odbc2)
{ int i, attrcnt;  table_descr * tbdf;  d_table * td;
  char aname[ATTRNAMELEN+1];
  t_vector_descr vector(columns_coldescr, rs);
  if (is_table)
    { tbdf=(table_descr*)def;  attrcnt=tbdf->attrcnt; }
  else
    { td=(d_table*)def;  attrcnt=td->attrcnt; }
  for (i=1;  i<attrcnt;  i++)
  { attribdef *att;
    if (is_table) att=tbdf->attrs+i;  else att=(attribdef*)ATTR_N(td,i);
    strcpy(aname, att->name);
    if (mmatch(meta, anamepatt, aname))
    { memcpy(rs->colname, att->name, ATTRNAMELEN);
      column_info1(att->attrtype, att->attrspecif, &rs->data_type, rs->type_name,
        &rs->column_size, &rs->buffer_length, &rs->decimal_digits, odbc2);
      column_info2(att->attrtype, att->attrspecif, att->attrspecif.length, att->nullable, rs->data_type,
        &rs->num_prec_radix, &rs->nullable,
        rs->column_def, &rs->sql_data_type, &rs->sql_datetime_sub, 
        &rs->char_octet_length, rs->is_nullable, is_table && att->defvalexpr!=NULL ? NULLSTRING : NULL);
      rs->ordinal_position=i;
      tb_new(cdp, dest_tbdf, FALSE, &vector);
    }
  }
}

static void column_record_full(cdp_t cdp, BOOL meta, table_descr * tbdf, trecnum rec, tptr anamepatt, result_columns * rs, BOOL odbc2)
{ table_all ta;  
  t_vector_descr vector(columns_coldescr, rs);
  if (partial_compile_to_ta(cdp, rec, &ta)) return;  // compilation error
  for (int i=1;  i<ta.attrs.count();  i++)
  { atr_all * att = ta.attrs.acc0(i);
    if (att->type==ATT_DOMAIN)  // translate the domain
    { t_type_specif_info tsi;
      if (compile_domain_to_type(cdp, att->specif.domain_num, tsi))
        { att->type=tsi.tp;  att->specif=tsi.specif;  att->nullable=tsi.nullable; }
    }
    char aname[ATTRNAMELEN+1];  strcpy(aname, att->name);
    if (mmatch(meta, anamepatt, aname))
    { memcpy(rs->colname, att->name, ATTRNAMELEN);
      column_info1(att->type, att->specif, &rs->data_type, rs->type_name,
        &rs->column_size, &rs->buffer_length, &rs->decimal_digits, odbc2);
      column_info2(att->type, att->specif, att->specif.length, att->nullable, rs->data_type,
        &rs->num_prec_radix, &rs->nullable,
        rs->column_def, &rs->sql_data_type, &rs->sql_datetime_sub, 
        &rs->char_octet_length, rs->is_nullable, att->defval);
      rs->ordinal_position=i;
      tb_new(cdp, tbdf, FALSE, &vector);
    }
  }
}

class key_analysis
{ const char * text;
  int pos;
 public:
  key_analysis(const char * textIn)
    { text=textIn;  pos=0; }
  bool get_next_part(char * part_text, int maxlen, bool * desc);
};

bool key_analysis::get_next_part(char * part_text, int maxlen, bool * desc)
{ 
  while (text[pos]==' ') pos++;
  const char * start = text+pos, * stop = start;
 // find the next part in the key:
  int par_level=0;  bool siq_level=false, dblq_level=false, idnq_level=false;
  while (*stop && (*stop!=',' || par_level>0 || siq_level || dblq_level || idnq_level))
  { if      (*stop=='\'' && !dblq_level && !idnq_level) siq_level=!siq_level;
    else if (*stop=='\"' && !siq_level  && !idnq_level) dblq_level=!dblq_level;
    else if (*stop=='`'  && !siq_level  && !dblq_level) idnq_level=!idnq_level;
    else if (*stop=='('  && !dblq_level && !idnq_level && !siq_level) par_level++;
    else if (*stop==')'  && !dblq_level && !idnq_level && !siq_level) par_level--;
    stop++;
  }
 // find and remove the ASC or DESC specification:
  *desc=false;
  int sz = (int)(stop-start);
  while (sz && start[sz-1]==' ') sz--;
  if (sz > 4)
    if (!memicmp(start+sz-4, "DESC", 4))
      { *desc=true;  sz-=4; }
    else if (!memicmp(start+sz-3, "ASC", 3)) 
      sz-=3;
 // remove identifier quotes if specified:
  while (sz && start[sz-1]==' ') sz--;
  if (sz>2 && *start=='`' && start[sz-1]=='`')  // check for identifier quote inside
  { siq_level=false;  dblq_level=false;  idnq_level=false;  int i=1;
    while (i<sz-1)
    { if      (start[i]=='\'' && !dblq_level) siq_level=!siq_level;
      else if (start[i]=='\"' && !siq_level) dblq_level=!dblq_level;
      else if (start[i]=='`'  && !siq_level  && !dblq_level) 
        { idnq_level=true;  break; }
      i++;
    }
    if (!idnq_level)  // no ` inside
    { sz-=2;
      start++;
    }
  }
 // copy the result:
  if (sz>maxlen)
    strcpy(part_text, "*");
  else 
    { memcpy(part_text, start, sz);  part_text[sz]=0; }
 // prepare for the next part:
  if (*stop==',') stop++;
  pos = (int)(stop - text);
  return sz>0;
}

void add_foreign_key(cdp_t cdp, table_descr * dest_tbdf, t_vector_descr & vector, result_foreign_keys * rs, table_all & ta, forkey_constr * ref)
{ *rs->fqualif=*rs->pqualif=0;
  memcpy(rs->fownername, ta.schemaname, OBJ_NAME_LEN);
  memcpy(rs->ftabname,   ta.selfname,   OBJ_NAME_LEN);
  memcpy(rs->pownername, *ref->desttab_schema ? ref->desttab_schema : ta.schemaname, OBJ_NAME_LEN);
  memcpy(rs->ptabname,   ref->desttab_name, OBJ_NAME_LEN);
  key_analysis fk_ka(ref->text);
  key_analysis pk_ka(ref->par_text);
  bool desc;  int seq = 1;
  while (fk_ka.get_next_part(rs->fcolname, sizeof(rs->fcolname)-1, &desc) && 
         pk_ka.get_next_part(rs->pcolname, sizeof(rs->pcolname)-1, &desc))
  { rs->key_seq = seq++;    
    strmaxcpy(rs->fk_name, ref->constr_name, sizeof(rs->fk_name));
    *rs->pk_name=0;  // this is wrong but I suppose it is not important
    rs->delete_rule = ref->delete_rule==REFRULE_CASCADE   ? SQL_CASCADE   :
                      ref->delete_rule==REFRULE_NO_ACTION ? SQL_NO_ACTION :
                      ref->delete_rule==REFRULE_SET_NULL  ? SQL_SET_NULL  : SQL_SET_DEFAULT;
    rs->update_rule = ref->update_rule==REFRULE_CASCADE   ? SQL_CASCADE   :
                      ref->update_rule==REFRULE_NO_ACTION ? SQL_NO_ACTION :
                      ref->update_rule==REFRULE_SET_NULL  ? SQL_SET_NULL  : SQL_SET_DEFAULT;
    rs->deferrability=ref->co_cha==COCHA_UNSPECIF && (cdp->sqloptions & SQLOPT_CONSTRS_DEFERRED) || ref->co_cha==COCHA_DEF ?
                        SQL_INITIALLY_DEFERRED : SQL_INITIALLY_IMMEDIATE;
    tb_new(cdp, dest_tbdf, FALSE, &vector);
  }
}

void add_schema_name_to_ta(cdp_t cdp, table_all & ta, ttablenum tbnum)
// Adds explicit schema name to ta
{ WBUUID schema_uuid;
  if (!*ta.schemaname)  
  { fast_table_read(cdp, tabtab_descr, tbnum, APPL_ID_ATR, schema_uuid);
    ker_apl_id2name(cdp, schema_uuid, ta.schemaname, NULL);  // schema uuid of the table converted to the schema name
  }
}

#define MAX_RESULT_LEN 1+6*(OBJ_NAME_LEN+1)+2*(ATTRNAMELEN+1)+2+3*sizeof(uns16)+INDEX_COLUMN_NAME_LEN

void ctlg_fill(cdp_t cdp, table_descr * dest_tbdf, unsigned ctlg_type)
// NULL param values are replaced by 0xff.
{ tptr tabpatt, schemapatt, anamepatt;  char result[MAX_RESULT_LEN];
  tobjname schemaname, tabname, aname;
  tfcbnum fcbn;
  unsigned ctlg_type2;  BOOL metadata, odbc2;  char * params;
  anal_odbc_param(cdp, ctlg_type2, metadata, odbc2, params);

 // schema name: (some applications like VB use NULL, replaced by current appl)
  BOOL restrict_appl;  
  WBUUID appl_uuid;  tobjname theschema;  // defined iff restrict_appl
  schemapatt=params;  sys_Upcase(schemapatt); 
  if (!strcmp(schemapatt, "%") || *schemapatt==(char)0xff) 
    if (*cdp->sel_appl_name) // WinBase specific
    { strcpy(theschema, cdp->sel_appl_name);
      memcpy(appl_uuid, cdp->sel_appl_uuid, UUID_SIZE);
      restrict_appl=TRUE;
    }
    else restrict_appl=FALSE;
  else  // schema name specified, restrict to it 
  { int i = 0, j = 0;
    while (i<OBJ_NAME_LEN && schemapatt[j])
    { if (schemapatt[j]!='\\') theschema[i++]=schemapatt[j];
      j++;
    }
    theschema[i]=0;
    if (ker_apl_name2id(cdp, theschema, appl_uuid, NULL))
    { // memset(appl_uuid, 0xff, UUID_SIZE);  // unknown  -- Delphi uses user name as owner name
      strcpy(theschema, cdp->sel_appl_name);  // ignore bad schema name, use current appl instead
      memcpy(appl_uuid, cdp->sel_appl_uuid, UUID_SIZE);
    }
    restrict_appl=TRUE;
  }

  if (params[PATT_NAME_LEN+1]==(char)0xff) tabpatt ="%";  // NULL encoded into 0xff
  else { tabpatt=params+PATT_NAME_LEN+1;  sys_Upcase(tabpatt); }
  schemaname[OBJ_NAME_LEN]=tabname[OBJ_NAME_LEN]=0;
  trecnum rec, rec2, limit;  int i;

  unsigned table_type_set, funique, fcoltype, fnullable;//,faccuracy, fscope;

  *result=0;   /* 'not deleted' attribute */
  switch (ctlg_type)
  { case CTLG_COLUMN_PRIVILEGES:
    { result_column_privileges * rs = (result_column_privileges *)result;
      rs->noname[0]=0;  /* constant */
      strcpy(rs->grantor, "_SYSTEM");
      if (params[2*(PATT_NAME_LEN+1)]==(char)0xff)  anamepatt=NULL;
      else { anamepatt=params+2*(PATT_NAME_LEN+1);  sys_Upcase(anamepatt); }
      aname[ATTRNAMELEN]=0;
      break;
    }
    case CTLG_COLUMNS:
    { result_columns * rs = (result_columns *)result;
      rs->noname[0]=rs->remarks[0]=0;  /* constant */
      if (params[2*(PATT_NAME_LEN+1)]==(char)0xff)  anamepatt=NULL;
      else { anamepatt=params+2*(PATT_NAME_LEN+1);  sys_Upcase(anamepatt); }
      aname[ATTRNAMELEN]=0;
      break;
    }
    case CTLG_FOREIGN_KEYS:
      break;
    case CTLG_PRIMARY_KEYS:
      break;
    case CTLG_PROCEDURE_COLUMNS:  
    { result_procedure_columns * rs = (result_procedure_columns *)result;
      rs->noname[0]=rs->remarks[0]=0;  /* constant */
      break;
    }
    case CTLG_PROCEDURES:
    { result_procedures * rs = (result_procedures *)result;
      rs->noname[0]=rs->remarks[0]=0;  /* constant */
      rs->num_input_params=rs->num_output_params=rs->num_result_sets=NONEINTEGER;
      break;
    }
    case CTLG_SPECIAL_COLUMNS:
    { result_special_columns * rs = (result_special_columns*)result;
      rs->pseudo_column=SQL_PC_NOT_PSEUDO;  /* constant */
      fcoltype =((unsigned*)(params+2*(PATT_NAME_LEN+1)))[0];
//    fscope   =((unsigned*)(params+2*(PATT_NAME_LEN+1)))[1];
      fnullable=((unsigned*)(params+2*(PATT_NAME_LEN+1)))[2];
      rs->scope = (sig16)((fcoltype==SQL_BEST_ROWID) ? SQL_SCOPE_SESSION : NONESHORT);
      break;
    }
    case CTLG_STATISTICS:
    { result_statistics * rs = (result_statistics*)result;
      rs->noname[0]=rs->filter_condition[0]=0;  /* constant */
      funique  =((unsigned*)(params+2*(PATT_NAME_LEN+1)))[0];
//    faccuracy=((unsigned*)(params+2*(PATT_NAME_LEN+1)))[1];
      break;
    }
    case CTLG_TABLE_PRIVILEGES:
    { result_table_privileges * rs = (result_table_privileges *)result;
      rs->noname[0]=0;  /* constant */
      strcpy(rs->grantor, "_SYSTEM");
      break;
    }
    case CTLG_TABLES:
    { result_tables * rs = (result_tables *)result;
      rs->noname[0]=rs->remarks[0]=0;  /* constant */
      table_type_set=*(unsigned*)(params+2*(PATT_NAME_LEN+1));
#ifdef STOP  // MS Excel 2003 asks for calalog names and must get at least one non-empty name
     // when asking for qualifiers, return only the empty one: 
      if (!*tabpatt && !*schemapatt) 
      { t_vector_descr vector(tables_coldescr, rs);
        rs->tabname[0]=rs->tabtype[0]=rs->applname[0]=0;    /* constant */
        strcpy(rs->noname, "A");  // max length is 1
        tb_new(cdp, dest_tbdf, FALSE, &vector);
        return;
      }
#endif      
     // provide all application names: 
      if (!*tabpatt && !strcmp(schemapatt, "%")) 
      { limit=(tobjnum)objtab_descr->Recnum();
        t_vector_descr vector(tables_coldescr, rs);
        rs->tabname[0]=rs->tabtype[0]=0;    /* constant */
        for (rec=0;  rec<limit;  rec++)
        { const ttrec * dt=(const ttrec*)fix_attr_read(cdp, objtab_descr, rec, DEL_ATTR_NUM, &fcbn);
          if (dt!=NULL)
          { if (!dt->del_mark)  /* not deleted */
              if (dt->categ==CATEG_APPL)
              { memcpy(rs->applname, dt->name, OBJ_NAME_LEN);
                tb_new(cdp, dest_tbdf, FALSE, &vector);
              }
            unfix_page(cdp, fcbn);
          }
        }
        return;
      }
     // provide list of table types:
      if (!*tabpatt && !*schemapatt && (table_type_set==TABLE_TYPE_LIST))
      { rs->applname[0]=rs->tabname[0]=0;  /* constant */
        t_vector_descr vector(tables_coldescr, rs);
        memcpy(rs->tabtype, "TABLE",        TABLE_TYPE_ATTR_SIZE);
        tb_new(cdp, dest_tbdf, FALSE, &vector);
        memcpy(rs->tabtype, "VIEW",         TABLE_TYPE_ATTR_SIZE);
        tb_new(cdp, dest_tbdf, FALSE, &vector);
        memcpy(rs->tabtype, "SYSTEM TABLE", TABLE_TYPE_ATTR_SIZE);
        tb_new(cdp, dest_tbdf, FALSE, &vector);
        memcpy(rs->tabtype, "SYNONYM",      TABLE_TYPE_ATTR_SIZE);
        tb_new(cdp, dest_tbdf, FALSE, &vector);
        return;
      }
      break;
    }
  } // switch
 ////////////////////////////// records in OBJTAB /////////////////////////////
  if (ctlg_type==CTLG_PROCEDURE_COLUMNS || ctlg_type==CTLG_PROCEDURES)
  { limit=(tobjnum)objtab_descr->Recnum();
    for (rec=0;  rec<limit;  rec++)
    { const ttrec * dt=(const ttrec*)fix_attr_read(cdp, objtab_descr, rec, DEL_ATTR_NUM, &fcbn);
      if (dt!=NULL)
      { if (!dt->del_mark && dt->categ==CATEG_PROC)
        { memcpy(tabname, dt->name, OBJ_NAME_LEN);
          if (mmatch(metadata, tabpatt, tabname) && //match(schemapatt, schemaname))
              (!restrict_appl || !memcmp(appl_uuid, dt->apluuid, UUID_SIZE)))
          { if (restrict_appl) strcpy(schemaname, theschema);
            else ker_apl_id2name(cdp, dt->apluuid, schemaname, NULL);
           // load & compile the routine:
            t_routine * rout = get_stored_routine(cdp, (tobjnum)rec, NULL, 2);
            if (rout!=NULL)
            { if (ctlg_type==CTLG_PROCEDURE_COLUMNS)
              { result_procedure_columns * rs = (result_procedure_columns *)result;
                t_vector_descr vector(procedure_columns_coldescr, rs);
                memcpy(rs->procname,  tabname,    OBJ_NAME_LEN);
                memcpy(rs->applname,  schemaname, OBJ_NAME_LEN);
               // result type:
                if (rout->rettype)
                { *rs->colname=0;
                  rs->column_type=SQL_RETURN_VALUE;
                  column_info1(rout->rettype, rout->retspecif, &rs->data_type, rs->type_name,
                    &rs->column_size, &rs->buffer_length, &rs->decimal_digits, odbc2);
                  column_info2(rout->rettype, rout->retspecif, rout->retspecif.length, TRUE, rs->data_type,
                    &rs->num_prec_radix, &rs->nullable,
                    rs->column_def, &rs->sql_data_type, &rs->sql_datetime_sub, 
                    &rs->char_octet_length, rs->is_nullable, NULL);
                  rs->ordinal_position=0; // defined in ODBC 3.0 Release notes!
                  tb_new(cdp, dest_tbdf, FALSE, &vector);
                }
               // parameters:
                const t_scope * pscope = rout->param_scope();
                for (int parnum=0;  parnum < pscope->locdecls.count();  parnum++)
                { t_locdecl * param = pscope->locdecls.acc0(parnum);
                  if (param->loccat==LC_LABEL) break;  // paramaters exhausted
                  strcpy(rs->colname, param->name);
                  rs->column_type = 
                    param->loccat==LC_INPAR || param->loccat==LC_INPAR_ ? SQL_PARAM_INPUT :
                    param->loccat==LC_OUTPAR|| param->loccat==LC_OUTPAR_? SQL_PARAM_OUTPUT :
                     SQL_PARAM_INPUT_OUTPUT;
                  column_info1(param->var.type, param->var.specif, &rs->data_type, rs->type_name,
                    &rs->column_size, &rs->buffer_length, &rs->decimal_digits, odbc2);
                  column_info2(param->var.type, t_specif(param->var.specif), t_specif(param->var.specif).length, TRUE, rs->data_type,
                    &rs->num_prec_radix, &rs->nullable,
                    rs->column_def, &rs->sql_data_type, &rs->sql_datetime_sub, 
                    &rs->char_octet_length, rs->is_nullable, param->var.defval!=NULL ? NULLSTRING : NULL); // defined default value always returned as TRUNCATED!
                  rs->ordinal_position=parnum+1;
                  tb_new(cdp, dest_tbdf, FALSE, &vector);
                }
              }
              else  // procedures
              { result_procedures * rs = (result_procedures *)result;
                t_vector_descr vector(procedure_coldescr, rs);
                memcpy(rs->procname,  tabname,    OBJ_NAME_LEN);
                memcpy(rs->applname,  schemaname, OBJ_NAME_LEN);
                rs->procedure_type = rout->rettype==0 ? SQL_PT_PROCEDURE : SQL_PT_FUNCTION;
                tb_new(cdp, dest_tbdf, FALSE, &vector);
              }
              delete rout; // no caching
            }
            else request_error(cdp, ANS_OK);  /* clear the compilation error */
          }
        }
        unfix_page(cdp, fcbn);
      }
      else break;
    } // cycle on OBJTAB records
  }
  else if (ctlg_type==CTLG_FOREIGN_KEYS)  // not iterating on tables
  { result_foreign_keys * rs = (result_foreign_keys *)result;
    t_vector_descr vector(foreign_keys_coldescr, rs);
    char * pk_schema, * pk_table, * fk_schema, * fk_table;  
    pk_schema=params;                      sys_Upcase(pk_schema);
    pk_table =params+1*(PATT_NAME_LEN+1);  sys_Upcase(pk_table);
    fk_schema=params+2*(PATT_NAME_LEN+1);  sys_Upcase(fk_schema);
    fk_table =params+3*(PATT_NAME_LEN+1);  sys_Upcase(fk_table);
    ttablenum pk_obj=NOOBJECT, fk_obj=NOOBJECT;
    if (*pk_table && (unsigned char)*pk_table!=0xff)  // 0xff when NULL specified in the catalog function call
      name_find2_obj(cdp, pk_table, (unsigned char)*pk_schema==0xff ? NULL : pk_schema, CATEG_TABLE, &pk_obj);
    if (*fk_table && (unsigned char)*fk_table!=0xff)  // 0xff when NULL specified in the catalog function call
      name_find2_obj(cdp, fk_table, (unsigned char)*fk_schema==0xff ? NULL : fk_schema, CATEG_TABLE, &fk_obj);
    if (fk_obj!=NOOBJECT) // insert references from the slave table
    { table_all ta;  
      if (!partial_compile_to_ta(cdp, fk_obj, &ta)) 
      { add_schema_name_to_ta(cdp, ta, fk_obj);
        for (i=0;  i<ta.refers.count();  i++)
        { forkey_constr * ref = ta.refers.acc0(i);
          if (*pk_table && (unsigned char)*pk_table!=0xff)  // check the PK table/schema
          { if (sys_stricmp(ref->desttab_name, pk_table)) continue;
            if (*pk_schema && (unsigned char)*pk_schema!=0xff)  // check the PK table/schema
              if (sys_stricmp(*ref->desttab_schema ? ref->desttab_schema : ta.schemaname, pk_schema)) continue;
          }
         // add this key pair:
          add_foreign_key(cdp, dest_tbdf, vector, rs, ta, ref);
        }
      }
    }
    else if (pk_obj!=NOOBJECT)  // insert references from the parent table
    { table_descr_auto pk_tbdf(cdp, pk_obj);
      if (pk_tbdf->me())
      { prepare_list_of_referencing_tables(cdp, pk_tbdf->me());
        for (int j=0;  j<pk_tbdf->indx_cnt;  j++)
        { dd_index * indx = &pk_tbdf->indxs[j];
          if (indx->reftabnums)
            for (ttablenum * ptabnum = indx->reftabnums;  *ptabnum;  ptabnum++) // cycle on referencing tables
            { ttablenum chtabnum = *ptabnum;
              if (chtabnum!=(ttablenum)-1)  /* table exists */
              { table_all ta;  
                if (partial_compile_to_ta(cdp, chtabnum, &ta)) break;
                add_schema_name_to_ta(cdp, ta, chtabnum);
                for (i=0;  i<ta.refers.count();  i++)
                { forkey_constr * ref = ta.refers.acc0(i);
                 // check the PK table/schema
                  if (sys_stricmp(ref->desttab_name, pk_table)) continue;
                  if (*pk_schema && (unsigned char)*pk_schema!=0xff)  // check the PK table/schema
                    if (sys_stricmp(*ref->desttab_schema ? ref->desttab_schema : ta.schemaname, pk_schema)) continue;
                 // check the FK table/schema
                  if (*fk_table && (unsigned char)*fk_table!=0xff)  
                  { if (sys_stricmp(ta.selfname, fk_table)) continue;
                    if (*fk_schema && (unsigned char)*fk_schema!=0xff)  // check the PK table/schema
                      if (sys_stricmp(ta.schemaname, fk_schema)) continue;
                  }
                // add this key pair:
                  add_foreign_key(cdp, dest_tbdf, vector, rs, ta, ref);
                }
              }
            }
        }
      }
    }
  }
  else
 ////////////////////////////// records from TABTAB ////////////////////////////
 {limit=(tobjnum)tabtab_descr->Recnum();
  for (rec=0;  rec<limit;  rec++)
  { const ttrec * dt=(const ttrec*)fix_attr_read(cdp, tabtab_descr, rec, DEL_ATTR_NUM, &fcbn);
    if (dt!=NULL)
    { if (!dt->del_mark &&  /* not deleted */
          dt->categ==CATEG_TABLE)   /* not a link object */
      { memcpy(tabname, dt->name, OBJ_NAME_LEN);
        if (mmatch(metadata, tabpatt, tabname) && //match(schemapatt, schemaname))
            (!restrict_appl || !memcmp(appl_uuid, dt->apluuid, UUID_SIZE)))
        { if (restrict_appl) strcpy(schemaname, theschema);
          else ker_apl_id2name(cdp, dt->apluuid, schemaname, NULL);
          table_descr_auto tbdf(cdp, (ttablenum)rec);
          if (tbdf->me()!=NULL)
          { switch (ctlg_type)
{ case CTLG_COLUMN_PRIVILEGES:
  { result_column_privileges * rs = (result_column_privileges *)result;
    memcpy(rs->tabname,  tabname,    OBJ_NAME_LEN);
    memcpy(rs->applname, schemaname, OBJ_NAME_LEN);
    column_privilege_record(cdp, metadata, dest_tbdf, rec, anamepatt, rs, TRUE, tbdf->me());
    break;
  }
  case CTLG_COLUMNS:
  { result_columns * rs = (result_columns *)result;
    memcpy(rs->tabname,  tabname,    OBJ_NAME_LEN);
    memcpy(rs->applname, schemaname, OBJ_NAME_LEN);
    column_record_full(cdp, metadata, dest_tbdf, rec, anamepatt, rs, odbc2);
    break;
  }
  case CTLG_PRIMARY_KEYS:
  { result_primary_keys * rs = (result_primary_keys *)result;
    t_vector_descr vector(primary_keys_coldescr, rs);
    *rs->qualif=0;
    memcpy(rs->ownername, schemaname, OBJ_NAME_LEN);
    memcpy(rs->tabname,   tabname,    OBJ_NAME_LEN);
   // find the definition of the primary key, if any:
    table_all ta;  
    if (partial_compile_to_ta(cdp, rec, &ta)) break;
    for (i=0;  i<ta.indxs.count();  i++)
    { ind_descr * indx = ta.indxs.acc(i);
      if (indx->index_type==INDEX_PRIMARY)
      { key_analysis ka(indx->text);
        bool desc;  int seq = 1;
        while (ka.get_next_part(rs->colname, sizeof(rs->colname)-1, &desc))
        { rs->key_seq = seq++;    
          strmaxcpy(rs->pk_name, indx->constr_name, sizeof(rs->pk_name));
          tb_new(cdp, dest_tbdf, FALSE, &vector);
        }
      }
    }
    break;
  }
  case CTLG_SPECIAL_COLUMNS:
  { result_special_columns * rs = (result_special_columns*)result;
    t_vector_descr vector(special_columns_coldescr, rs);
    if (fcoltype==SQL_BEST_ROWID)
    { tattrib attrs[MAX_ROWID_PARTS];
      int cnt=find_best_rowid(cdp, (ttablenum)rec, attrs, fnullable);
      for (i=0;  i<cnt;  i++)
      { attribdef * att=tbdf->attrs+attrs[i];
        memcpy(rs->colname, att->name, ATTRNAMELEN);
        column_info1(att->attrtype, att->attrspecif, &rs->data_type, rs->type_name,
          &rs->column_size, &rs->buffer_length, &rs->decimal_digits, odbc2);
        tb_new(cdp, dest_tbdf, FALSE, &vector);
      }
    }
    else
      for (i=1;  i<tbdf->attrcnt;  i++)
      { attribdef * att=tbdf->attrs+i;
        if (!strcmp(att->name, "_W5_ZCR"))
        { memcpy(rs->colname, att->name, ATTRNAMELEN);
          column_info1(att->attrtype, att->attrspecif, &rs->data_type, rs->type_name,
            &rs->column_size, &rs->buffer_length, &rs->decimal_digits, odbc2);
          tb_new(cdp, dest_tbdf, FALSE, &vector);
          break;
        }
      }
    break;
  }
  case CTLG_STATISTICS:
  { result_statistics * rs = (result_statistics*)result;
    t_vector_descr vector(statistics_coldescr, rs);
    memcpy(rs->tabname,  tabname,    OBJ_NAME_LEN);
    memcpy(rs->applname, schemaname, OBJ_NAME_LEN);
    /* write table statistics: */
    rs->non_unique=NONESHORT;
    memcpy(rs->index_qualifier, tabname, OBJ_NAME_LEN);
    rs->index_name[0]=0;
    rs->type=SQL_TABLE_STAT;
    rs->ordinal_position=NONESHORT;
    rs->column_name[0]=0;
    rs->asc_or_desc=0;
    rs->cardinality=tbdf->Recnum();
    rs->pages=tbdf->record_count2block_count(tbdf->Recnum()); 
    tb_new(cdp, dest_tbdf, FALSE, &vector);
    /* describe indicies: */
    table_all ta;  
    if (partial_compile_to_ta(cdp, rec, &ta)) break;
    for (i=0;  i<ta.indxs.count();  i++)
    { ind_descr * indx = ta.indxs.acc(i);
      bool disti = indx->index_type==INDEX_PRIMARY || indx->index_type==INDEX_UNIQUE;
      if (funique==SQL_INDEX_ALL || disti)
      { rs->non_unique=(sig16)!disti;
        memcpy(rs->index_name, indx->constr_name, OBJ_NAME_LEN);
        if (!*rs->index_name) int2str(i+1, rs->index_name);
        rs->type=SQL_INDEX_CLUSTERED;
        rs->ordinal_position=0;
        rs->cardinality=rs->pages=NONEINTEGER;   /* not known */
        key_analysis ka(indx->text);
        bool desc;  int seq = 1;
        while (ka.get_next_part(rs->column_name, sizeof(rs->column_name)-1, &desc))
        { rs->ordinal_position = seq++;    
          strmaxcpy(rs->index_name, indx->constr_name, sizeof(rs->index_name));
          rs->asc_or_desc=desc ? 'D' : 'A';
          tb_new(cdp, dest_tbdf, FALSE, &vector);
        }
      }
    }
    break;
  }
  case CTLG_TABLE_PRIVILEGES:
  { tcateg categ;  uns8 del;
    result_table_privileges * rs = (result_table_privileges *)result;
    memcpy(rs->tabname,  tabname,    OBJ_NAME_LEN);
    memcpy(rs->applname, schemaname, OBJ_NAME_LEN);
    t_vector_descr vector(table_privileges_coldescr, rs);

    for (rec2=0;  rec2<usertab_descr->Recnum();  rec2++)
      if (!tb_read_atr(cdp, usertab_descr, rec2, DEL_ATTR_NUM, (tptr)&del) && !del)
        if (!tb_read_atr(cdp, usertab_descr, rec2, USER_ATR_CATEG, (tptr)&categ))
          if (!tb_read_atr(cdp, usertab_descr, rec2, USER_ATR_LOGNAME, rs->grantee))
          { privils prvs(cdp);  t_privils_flex priv_val;
            prvs.set_user((tobjnum)rec2, categ, TRUE);
            prvs.get_effective_privils(tbdf->me(), NORECNUM, priv_val);
            memcpy(rs->is_grantable, (*priv_val.the_map() & RIGHT_GRANT)
                ? "YES" : "NO", 3);
            if (*priv_val.the_map() & RIGHT_INSERT)
            { memcpy(rs->privilege, "INSERT", PRIVILEGE_NAME_ATTR_SIZE);
              tb_new(cdp, dest_tbdf, FALSE, &vector);
            }
            if (*priv_val.the_map() & RIGHT_DEL)
            { memcpy(rs->privilege, "DELETE", PRIVILEGE_NAME_ATTR_SIZE);
              tb_new(cdp, dest_tbdf, FALSE, &vector);
            }
            if (prvs.any_eff_privil(tbdf->me(), NORECNUM, FALSE))
            { memcpy(rs->privilege, "SELECT", PRIVILEGE_NAME_ATTR_SIZE);
              tb_new(cdp, dest_tbdf, FALSE, &vector);
              memcpy(rs->privilege, "REFERENCES", PRIVILEGE_NAME_ATTR_SIZE);
              tb_new(cdp, dest_tbdf, FALSE, &vector);
            }
            if (prvs.any_eff_privil(tbdf->me(), NORECNUM, TRUE))
            { memcpy(rs->privilege, "UPDATE", PRIVILEGE_NAME_ATTR_SIZE);
              tb_new(cdp, dest_tbdf, FALSE, &vector);
            }
          }
    break;
  }
  case CTLG_TABLES:
    if (SYSTEM_TABLE(rec) ?
           (table_type_set & TABLE_TYPE_SYSTEM_TABLE) :
           (table_type_set & TABLE_TYPE_TABLE))
    { result_tables * rs = (result_tables *)result;
      t_vector_descr vector(tables_coldescr, rs);
      memcpy(rs->tabname, tabname,    OBJ_NAME_LEN);
      memcpy(rs->applname,schemaname, OBJ_NAME_LEN);
      memcpy(rs->tabtype, SYSTEM_TABLE(rec) ? "SYSTEM TABLE" : "TABLE", TABLE_TYPE_ATTR_SIZE);
      tb_new(cdp, dest_tbdf, FALSE, &vector);
    }
    break;
} /* end of switch */
          } /* table installed */
          else request_error(cdp, ANS_OK);  /* clear the error */
        }
      }
      unfix_page(cdp, fcbn);
    }
    else break;
  }
 } // not PROC
 /* scanning for cursors: */
  if ((ctlg_type!=CTLG_TABLES || !(table_type_set & TABLE_TYPE_VIEW)) &&
       ctlg_type!=CTLG_COLUMNS && ctlg_type!=CTLG_COLUMN_PRIVILEGES) return;
  limit=(tobjnum)objtab_descr->Recnum();
  for (rec=0;  rec<limit;  rec++)
  { const ttrec * dt=(const ttrec*)fix_attr_read(cdp, objtab_descr, rec, DEL_ATTR_NUM, &fcbn);
    if (dt!=NULL)
    { if (!dt->del_mark &&  /* not deleted */
          dt->categ==CATEG_CURSOR)   /* not a link object */
      { memcpy(tabname, dt->name, OBJ_NAME_LEN);
        if (mmatch(metadata, tabpatt, tabname) && //match(schemapatt, schemaname))
            (!restrict_appl || !memcmp(appl_uuid, dt->apluuid, UUID_SIZE)))
        { if (restrict_appl) strcpy(schemaname, theschema);
          else ker_apl_id2name(cdp, dt->apluuid, schemaname, NULL);
          unsigned psize;
          d_table * td = kernel_get_descr(cdp, CATEG_CURSOR, (tobjnum)rec, &psize);
          if (td!=NULL)
          { switch (ctlg_type)
            { case CTLG_COLUMN_PRIVILEGES:
              { result_column_privileges * rs = (result_column_privileges *)result;
                memcpy(rs->tabname,  tabname,    OBJ_NAME_LEN);
                memcpy(rs->applname, schemaname, OBJ_NAME_LEN);
                column_privilege_record(cdp, metadata, dest_tbdf, rec, anamepatt, rs, FALSE, td);
                break;
              }
              case CTLG_COLUMNS:
              { result_columns * rs = (result_columns *)result;
                memcpy(rs->tabname,  tabname,    OBJ_NAME_LEN);
                memcpy(rs->applname, schemaname, OBJ_NAME_LEN);
                column_record(cdp, metadata, dest_tbdf, anamepatt, rs, FALSE, td, odbc2);
                break;
              }
              case CTLG_TABLES:
              { result_tables * rs = (result_tables *)result;
                t_vector_descr vector(tables_coldescr, rs);
                memcpy(rs->tabname, tabname,    OBJ_NAME_LEN);
                memcpy(rs->applname,schemaname, OBJ_NAME_LEN);
                memcpy(rs->tabtype, "VIEW", TABLE_TYPE_ATTR_SIZE);
                tb_new(cdp, dest_tbdf, FALSE, &vector);
                break;
              }
            } /* end of switch */
            corefree(td);
          }
          else request_error(cdp, ANS_OK);  /* clear the error */
        }
      }
      unfix_page(cdp, fcbn);
    }
    else break;
  }
}

static char * ctlg_descrs[] =
{ "SELECT * FROM _IV_ODBC_TYPE_INFO ORDER BY DATA_TYPE,TYPE_NAME",
  "SELECT * FROM _IV_ODBC_COLUMN_PRIVS ORDER BY TABLE_CAT,TABLE_SCHEM,TABLE_NAME,COLUMN_NAME,PRIVILEGE",
  "SELECT * FROM _IV_ODBC_COLUMNS ORDER BY TABLE_CAT,TABLE_SCHEM,TABLE_NAME,ORDINAL_POSITION",
  "SELECT * FROM _IV_ODBC_FOREIGN_KEYS ORDER BY PKTABLE_CAT,PKTABLE_SCHEM,PKTABLE_NAME,FKTABLE_CAT,FKTABLE_SCHEM,FKTABLE_NAME,KEY_SEQ",
  "SELECT * FROM _IV_ODBC_PRIMARY_KEYS ORDER BY TABLE_CAT,TABLE_SCHEM,TABLE_NAME,KEY_SEQ",
  "SELECT * FROM _IV_ODBC_PROCEDURE_COLUMNS ORDER BY PROCEDURE_CAT,PROCEDURE_SCHEM,PROCEDURE_NAME,COLUMN_TYPE,ORDINAL_POSITION",
  "SELECT * FROM _IV_ODBC_PROCEDURES ORDER BY PROCEDURE_CAT,PROCEDURE_SCHEM,PROCEDURE_NAME",
  "SELECT * FROM _IV_ODBC_SPECIAL_COLUMNS ORDER BY SCOPE",
  "SELECT * FROM _IV_ODBC_STATISTICS ORDER BY NON_UNIQUE,TYPE,INDEX_QUALIFIER,INDEX_NAME,ORDINAL_POSITION",
  "SELECT * FROM _IV_ODBC_TABLE_PRIVS ORDER BY TABLE_CAT,TABLE_SCHEM,TABLE_NAME,PRIVILEGE",
  "SELECT * FROM _IV_ODBC_TABLES ORDER BY TABLE_TYPE,TABLE_CAT,TABLE_SCHEM,TABLE_NAME"
};

tcursnum sql_catalog_query(cdp_t cdp, tptr params)
// Creating non-standard cursor containing the answer to the ODBC catalog query
{// save params for inner procedures
  cdp->temp_param = params;
  unsigned ctlg_type;  BOOL meta, odbc2;  char * specif_params;
  anal_odbc_param(cdp, ctlg_type, meta, odbc2, specif_params);
  char * query = ctlg_descrs[ctlg_type];
  tcursnum cursnum;  cur_descr * cd;
  { t_exkont_sqlstmt ekt(cdp, cdp->sel_stmt, query);
    sql_open_cursor(cdp, query, cursnum, &cd, false);  // sets cursnum=NOCURSOR on error
  }
  cdp->temp_param = NULL;
  return cursnum;
}



