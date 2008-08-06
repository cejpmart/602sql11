/* TABLE TYPE used in table catalog functions: */
#define TABLE_TYPE_TABLE         1
#define TABLE_TYPE_VIEW          2
#define TABLE_TYPE_SYSTEM_TABLE  4
#define TABLE_TYPE_SYNONYM       8
#define TABLE_TYPE_LIST          (unsigned)-1

/* CATALOG function numbers: */
#define CTLG_TYPE_INFO           0

#define CTLG_COLUMN_PRIVILEGES   1
#define CTLG_COLUMNS             2
#define CTLG_FOREIGN_KEYS        3
#define CTLG_PRIMARY_KEYS        4
#define CTLG_PROCEDURE_COLUMNS   5
#define CTLG_PROCEDURES          6
#define CTLG_SPECIAL_COLUMNS     7
#define CTLG_STATISTICS          8
#define CTLG_TABLE_PRIVILEGES    9
#define CTLG_TABLES             10

#define PATT_NAME_LEN (2*OBJ_NAME_LEN)

/* Send_parameters options: */
#define SEND_PAR_PULL_1         -17
#define SEND_EMB_PULL_EXT       -16
#define SEND_EMB_TYPED_EXT      -15
#define SEND_PAR_PAR_INFO       -14
#define SEND_PAR_RS_INFO        -13
#define SEND_PAR_PREPARE        -12
#define SEND_PAR_EXEC_PREP      -11
#define SEND_EMB_PULL           -10
#define SEND_EMB_TYPED          -9
#define SEND_PAR_PULL           -8
#define SEND_PAR_POS            -6
#define SEND_PAR_NAME           -5
#define SEND_PAR_CREATE_STMT    -4
#define SEND_PAR_DROP_STMT      -3
#define SEND_PAR_DROP           -2
#define SEND_PAR_DELIMITER      -1

#define SEND_PAR_COMMON_LIMIT 1024
#define SEND_PAR_PART_SIZE    30000

#define WB_type_scale(type) ((type)==ATT_MONEY ? 2 : (type)==ATT_DATIM ? 3 : NONESHORT)  // ODBC 3.0 name is decimal digits
#define WB_type_numeric(type) ((type)==ATT_MONEY || (type)==ATT_FLOAT || (type)==ATT_INT16 || (type)==ATT_INT32 || (type)==ATT_INT8 || (type)==ATT_INT64)
UDWORD WB_type_precision(int type);
CFNC DllKernel SWORD WINAPI type_WB_to_sql(int WBtype, t_specif specif, BOOL odbc2);
void type_WB_to_3(int WBtype, t_specif specif, SQLSMALLINT & type, SQLSMALLINT & concise, SQLSMALLINT & code, BOOL odbc2);
int simple_column_size(int concise_type, t_specif specif);

