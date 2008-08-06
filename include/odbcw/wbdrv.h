/*
  wbdrv.h - This is the 602SQL ODBC driver main include file.
*/

#ifdef WINS
#include  <windows.h>                     // Windows include file
#include <sqlucode.h>

extern HINSTANCE s_hModule;	// DLL handle.
#include "wbodbcrc.h"

#else ///////////////////////////////// LINUX

#include "winrepl.h"

#define ODBCVER 0x0300

#define WPARAM SQLWPARAM
#define DWORD SQLDWORD
#define LPDWORD SQLLPDWORD
#define UNICODE
#include <sql.h>   // includes sqltypes.h
#include <sqlext.h>
#include <sqlucode.h>
#undef DWORD
#undef LPDWORD
#undef WPARAM
#include <malloc.h>
#include <assert.h>

#define CONNECT_KEY_COUNT 6

extern char DEFAULT[];

#define DllExport
#define DllKernel
#define DllWbed
#define DllPrezen

#endif ////////////////////////////////////// LINUX

/******************************** WinBase ***********************************/
#include "cint.h"
#include "compil.h"
#include "odbc.h"

#define TAG_ENV   69
#define TAG_DBC   73
#define TAG_STMT  49
#define TAG_DESC  81

#define SQL_C_FIRST_TYPE  -100
#define SQL_C_LAST_TYPE    100
#define SQL_FIRST_TYPE    -100
#define SQL_LAST_TYPE      100

#define IS_LONG_SQL_TYPE(type) \
  ((type==SQL_VARBINARY) || (type==SQL_LONGVARBINARY) ||\
   (type==SQL_LONGVARCHAR) || (type==SQL_WLONGVARCHAR))

struct STMT;
/******************************* statement & connection *********************/
#define MAX_ERR_CNT 5
struct DESC;
struct DBC;

struct err_t
{ uns16 errnums[MAX_ERR_CNT];
  int   errcnt;
  short return_code;  // the last return code
  err_t(void)
  { errcnt=0;  return_code=SQL_SUCCESS; }
};

struct t_odbc_obj
{ int   tag;  // const during the lifetime of the object, but changed when destructed
  err_t err;
  t_odbc_obj(int tagIn) : tag(tagIn)
    { }
};

#define CLEAR_ERR(x) (x)->err.errcnt=0
#define SUCCESS(x)   (x)->err.errcnt ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS

struct ENV : public t_odbc_obj
{ int    connection_cnt;   /* number of allocated connections in this env. */
  DBC *  dbcs;
  SQLUINTEGER connection_pooling;
  SQLUINTEGER cp_match;
  SQLINTEGER  odbc_version;
  ENV(void) : t_odbc_obj(TAG_ENV)
  { dbcs=NULL;  connection_cnt=0;
    connection_pooling=SQL_CP_OFF;  cp_match=SQL_CP_STRICT_MATCH;
    odbc_version=SQL_OV_ODBC3;
  }
  ~ENV(void)
    { tag=0; }
};

typedef struct STMTOPT    /* copy of the option part in STMT */
{ BOOL   async_enabled;
  UDWORD max_data_len;    /* retrieved data length restriction, zero iff not restr. */
  BOOL   no_scan;         /* prevents scanning statements for escapes not used */
  UDWORD query_timeout;
  UDWORD max_rows;        /* max. query result size, zero iff not restricted */
  BOOL   retrieve_data;
  UDWORD sim_cursor;
  BOOL   use_bkmarks;
  SQLLEN crowKeyset;
  UDWORD cursor_type;
  UWORD  concurrency;
  BOOL   metadata;
} STMTOPT;

struct DBC : public t_odbc_obj
{ ENV *  my_env;
  DBC *  next_dbc;   // list of DBCs in the same ENV
  int    browse_connect_status;
  BOOL   connected;
  BOOL   in_trans;
  struct STMT * stmts;  /* one of statements on this connection (any) */
  cdp_t  cdp;
  char   dsn[SQL_MAX_DSN_LENGTH+1];
 /* connection options: */
  sig32  access_mode;
  BOOL   async_enabled;
  sig32  autocommit;
  sig32  conn_timeout;
  sig32  login_timeout;
  sig32  txn_isolation;
  sig32  translate_option;
  tobjname server_name;  // current catalog, qualifier
  tobjname appl_name;    // current schema, owner
  STMTOPT stmtopt;
 // allocated descriptors:
  DESC * descs;          // implicit descriptores are not inserted into this list
  void remove_descr_from_list_and_statements(DESC * desc);   // not called for implicit descriptors

  DBC(ENV *  my_envIn) : t_odbc_obj(TAG_DBC), my_env(my_envIn)
  { descs=NULL;  next_dbc=NULL;  browse_connect_status=0;  connected=in_trans=FALSE; stmts=NULL;  cdp=NULL;  *dsn=0; }
  ~DBC(void)
    { tag=0; }
};
////////////////////////////// descriptor /////////////////////////////////////
#define PREFIX_MAX   11
#define SUFFIX_MAX    3
#define LABEL_MAX    31

struct t_desc_rec  // a descriptor record structure
{// IRD specific, read-only:
  BOOL        auto_unique_value;
  tobjname    base_column_name;
  tobjname    base_table_name;
  BOOL        case_sensitive;    // +IPD population
  tobjname    catalog_name;
  SQLINTEGER  display_size;
  BOOL        fixed_prec_scale; // +IPD population
  char        label[LABEL_MAX+1];
  char        literal_prefix[PREFIX_MAX+1];
  char        literal_suffix[SUFFIX_MAX+1];
  tobjname    local_type_name;     // +IPD population
  tobjname    schema_name;
  SQLSMALLINT searchable;
  tobjname    table_name;
  tobjname    type_name;           // +IPD population
  SQLSMALLINT updatable;
 // IPD+IRD
  tobjname    name;               
  SQLSMALLINT nullable;         // read-only
  SQLSMALLINT unnamed;
  BOOL        _unsigned;        // read_only
 // IPD only:
  SQLSMALLINT parameter_type;
 // general:
  SQLSMALLINT type;
  t_specif    specif;  // additional string information for IPD and IRD (scale and length moved to [scale] and [octet_length])
  SQLSMALLINT concise_type;
  SQLSMALLINT datetime_int_code;
  SQLULEN     length;
  SQLSMALLINT precision;
  SQLINTEGER  num_prec_radix;
  SQLSMALLINT scale;
  SQLINTEGER  datetime_int_prec;
  SQLLEN  octet_length;      // APD: step in column-wise binding, defined only for char and binary data
  SQLLEN * octet_length_ptr; // application only
  SQLLEN * indicator_ptr;    // application only
  SQLPOINTER  data_ptr;          // application only, may be param ID for AE param in APD
 // private, in IRD only:
  BOOL special;
  SQLLEN offset;
 // private, in APD only:
  BOOL   is_AE;                  // param if AE, defined in prepare_params
  tptr   buf;                    // the AE value, unless sent directly
  SQLLEN valsize;                // size of AE value or SQL_NULL_DATA or SQL_DEFAULT_PARAM
  SQLLEN value_step;             // step in column-wise binding, not for C_CHAR & C_BINARY
  SQLLEN * curr_octet_len;   // data for the current parameter set
  SQLLEN * curr_indicator;   // data for the current parameter set
  tptr     curr_data;        // data for the current parameter set

  void init(void);
  void clear(void)  // deleting descriptor record
  { init(); }
  int column_size(void);
  int decimal_digits(void);
};

SPECIF_DYNAR(t_desc_rec_dynar, t_desc_rec, 3); 

struct DESC : public t_odbc_obj
{ const BOOL is_implem;
  const BOOL is_implicit;
  const BOOL is_param;    // false for explicit DESCs
  const BOOL is_row;      // false for explicit DESCs
 // for allocated DESCs only
  DBC * my_dbc;    // originally NULL iff implicit, now always valid
  DESC * next_desc_in_dbc;
  BOOL populated;  // valid in IPD only
 // header fields:
  SQLUINTEGER array_size;
  SQLUSMALLINT * array_status_ptr;
  SQLINTEGER * bind_offset_ptr;
  SQLUINTEGER bind_type;
  SQLSMALLINT count;
  SQLULEN * rows_procsd_ptr;
 // records:
  t_desc_rec_dynar drecs;

  DESC(DBC * dbc, bool implicitIn, BOOL implem, int rowpar) : t_odbc_obj(TAG_DESC),
    is_implem(implem), is_implicit(implicitIn), 
    is_param(rowpar==1), is_row(rowpar==2), my_dbc(dbc)
  { if (!is_implicit)  // only explicit descriptors are in this list
      { next_desc_in_dbc=dbc->descs;  dbc->descs=this; }
   // setting defaults:
    array_size=1;
    array_status_ptr=NULL;  bind_offset_ptr=NULL;
    bind_type=SQL_BIND_BY_COLUMN;
    count=0;
    rows_procsd_ptr=NULL;
  }
  void clear_AE_params(void);

  ~DESC(void)
  { tag=0;
    if (is_implem) clear_AE_params();
    if (!is_implicit) my_dbc->remove_descr_from_list_and_statements(this);
  }
};

#define MAX_RESULT_COUNT  20   // max statements in 1 iteration

typedef enum { AE_NO=0, AE_PARAMS, AE_COLUMNS, AE_COLUMNS_START } AE_type;

struct STMT : public t_odbc_obj
{ BOOL   async_run;
  BOOL   cancelled;
  struct STMT * next_stmt, * prev_stmt; /* bidir. list of stmts on the same connection */
  struct DBC * my_dbc;
  cdp_t  cdp;
 // descriptors:
  DESC *  apd;
  DESC *  ard;
  DESC    impl_apd; 
  DESC    impl_ard; 
  DESC    impl_ipd; 
  DESC    impl_ird; 
 /* statement options: */
  BOOL   async_enabled;
  UDWORD max_data_len;    /* retrieved data length restriction, zero iff not restr. */
  BOOL   no_scan;         /* prevents scanning statements for escapes, not used */
  UDWORD query_timeout;
  UDWORD max_rows;        /* max. query result size, zero iff not restricted */
  UDWORD sim_cursor;
  UDWORD use_bkmarks;
  SQLLEN crowKeyset;
  UDWORD cursor_type;
  UWORD  concurrency;
  BOOL   scrollable;
  uns32  sensitivity;
  UDWORD auto_ipd;        // get parameter info when preparing statement
  BOOL   metadata;
  BOOL   retrieve_data;
  void * bookmark_ptr;
 /* prepare info: */
  BOOL   is_prepared;   
  BOOL   param_info_retrieved; // parameter description from data source loaded to IPD
  uns32  prep_handle;
 /* statement parameters: */
  UDWORD exec_iter;     /* 0-based execution iteration counter */
  AE_type AE_mode;
  int    last_par_ind;  /* index of the last parameter provided, valid in AE_mode */
 /* result set: */
  t_dynar result_sets;  /* uns32 for every result set on this statement */
  unsigned sql_stmt_cnt;/* number of avaliable result sets in result_sets */
  unsigned sql_stmt_ind;/* index of the current result set */
  unsigned rs_per_iter; // number of result sets createad by 1 iteration
  tobjname cursor_name;
  tcursnum cursnum;     /* cursor number if regular result set available */
  sig32  row_cnt;       /* -1 if not counted */
  sig32  curr_rowset_start; /* -1 after SELECT is executed, valid iff has_result_set */
  BOOL   is_bulk_position;   /* TRUE iff positioned on the entire rowset */
  BOOL   has_result_set;
  UWORD  get_data_column;
  UDWORD get_data_offset;
  tptr   rowbuf;        /* buffer for a single row (position) */
  unsigned rowbufsize;  /* size of the row buffer */
  int    prev_array_size;
 /* update progress state variables: */
  int    num_in_rowset; /* current row in the rowset, AE mode */
  trecnum db_recnum;    /* positioned database recnum used in db access */
  int    curr_column;   /* current column, AE mode */
  uns32  curr_col_off;  /* offset in the current column */
  UWORD  fOption;       /* SetPos parameter */
  UWORD  fLock;         /* SetPos parameter */
  BOOL   was_any_info;  /* cumulating row errors and all infos */
  trecnum preloaded_recnum;  // recnum of the 1st record in [rowbuf]
  unsigned preloaded_count;  // number of records in [rowbuf]

  STMT(DBC * dbc);
  ~STMT(void);
  void *operator new(size_t size) throw ();
  void operator delete(void * ptr);
};

/* Execution of the stamement:
   - adds info about result info result_sets dynar;
   - opens the 1st result set;
   Openning a result set:
   - sets cursnum & has_result_set;
   - sets position & row_cnt to -1;
   - allocates rowbuf;
   - resets get_data_column & get_data_offset.
   Preparing a statement:
   - frees old statement source and compiled info;
   - sets result set, computes offsets & rowbufsize;
   Direct execution:
   - prepares the new statement (see above);
   - executes;
   - frees the statement source and compiled info;
   CLOSE statement (and closing the result set in MoreResults):
   - closes cursor;
   - deallocates rowbuf;
   - resets has_result_set;
*/
/* Statement source & compiled form are saved until:
   - new statement is prepared (from Prepare or DirectExec);
   - execution of DirectExec is completed (may be in ParamData);
   - catalog function is executed;
   - statement is dropped.
   Axiom: is_prepared iff (stmt_source != NULL) iff (compiled_stmts != NULL)
*/
/* rowbufsize and resatrs are defined by define_result_set:
   - in prepare for the 1st result set;
   - in MoreResults for the other result sets.
   define_result_set must be called before open_result_set!
*/

#define IsValidHENV(henv)   (henv  && ((ENV *)henv )->tag==TAG_ENV)
#define IsValidHDBC(hdbc)   (hdbc  && ((DBC *)hdbc )->tag==TAG_DBC)
#define IsValidHSTMT(hstmt) (hstmt && ((STMT*)hstmt)->tag==TAG_STMT)
#define IsValidHDESC(hdesc) (hdesc && ((DESC*)hdesc)->tag==TAG_DESC)
void raise_err(STMT * stmt, uns16 errnum);
void raise_err_dbc(DBC * dbc, uns16 errnum);
void raise_err_desc(DESC * desc, uns16 errnum);
void raise_err_env(ENV * env, uns16 errnum);
void raise_db_err(STMT * stmt);
void raise_db_err_dbc(DBC * dbc);
/* Removing old errors in each function is required since ODBC 2.0 */

#define SQL_00000   0
#define SQL_01002   1
#define SQL_01004   2
#define SQL_01006   3
#define SQL_01S00   4
#define SQL_07002   5
#define SQL_07006   6
#define SQL_08001   7
#define SQL_08002   8
#define SQL_08003   9
#define SQL_25000  10
#define SQL_28000  11
//#define SQL_S1001  12
//#define SQL_S1009  13
//#define SQL_S1010  14
//#define SQL_S1090  15
//#define SQL_S1092  16
//#define SQL_S1093  17
//#define SQL_S1107  18
//#define SQL_S1003  19
//#define SQL_S1004  20
//#define SQL_S1094  21
#define SQL_22001  22
#define SQL_22003  23
#define SQL_22018  24
#define SQL_22008  25
#define SQL_HYC00  26
#define SQL_HY00X  27
#define SQL_24000  28
#define SQL_HY009  29
#define SQL_34000  30
#define SQL_3C000  31
#define SQL_HY015  32
//#define SQL_S1108  33
#define SQL_HY000  34
#define SQL_HY106  35
#define SQL_HY096  36
#define SQL_37000  37
//#define SQL_S1012  38
#define SQL_IM001  39
//#define SQL_S1102  40
#define SQL_HY111  41
#define SQL_01S01  42
#define SQL_42000  43
#define SQL_HY109  44
#define SQL_01S02  45
#define SQL_W0001  46
#define SQL_W0002  47
#define SQL_W0003  48

#define SQL_HY001  51
#define SQL_07009  52
#define SQL_HY091  53
#define SQL_07005  54
#define SQL_HY010  55
#define SQL_HY016  56
#define SQL_HY107  57
#define SQL_HY104  58
#define SQL_HY090  59
#define SQL_HY092  60
#define SQL_IM007  61

#define WB_ERROR_BASE     1000

BOOL write_string(const char * text, char * szColName,
                  SWORD cbColNameMax, SWORD *pcbColName);
BOOL put_string(const char * text, SQLPOINTER Value,
                SQLINTEGER BufferLength, SQLINTEGER *StringLength);
void assign_cursor_name(STMT * stmt);
SWORD default_C_type(SWORD sqltype);
RETCODE prepare(STMT * stmt, tptr szSqlStr, SDWORD cbSqlStr);
BOOL open_result_set(STMT * stmt);
void close_result_set(STMT * stmt);
BOOL discard_other_result_sets(STMT * stmt);
BOOL define_result_set_from_td(STMT * stmt, const d_table * td);
void define_catalog_result_set(STMT * stmt, tcursnum curs);

BOOL drop_params(STMT * stmt);
BOOL send_cursor_name(STMT * stmt);
BOOL send_cursor_pos(STMT * stmt, trecnum pos);

BOOL convert_C_to_SQL(SWORD CType, SWORD SqlType, t_specif specif, tptr CSource, tptr SqlDest, SQLLEN CSize, SQLULEN SqlSize, STMT * stmt/*, bool c_type_converted*/);
SQLRETURN convert_SQL_to_C(SWORD CType, SWORD SqlType, t_specif sql_specif, tptr SqlSource, tptr CDest,
  SQLLEN cbValueMax, SQLLEN *pcbValue, SQLULEN SqlSize, STMT * stmt,  unsigned attr, BOOL multiattribute);
RETCODE SetPos_steps(STMT * stmt, BOOL start);
extern char auxbuf[MAX_FIXED_STRING_LENGTH+2]; /* used for writing strings (may be shorter than database size) */
BOOL write_db_data(STMT * stmt, tptr data, SDWORD len, bool from_put_data);
SDWORD C_type_size(SWORD ctype);
SWORD type_sql_to_WB(int type);
char * WB_type_name(uns8 type);
UDWORD WB_type_precision(int type);

void un_prepare(STMT * stmt);

void odbc_date2str(uns32 dt, char * tx);
BOOL odbc_str2date(char * tx, uns32 * dt);
void odbc_datim2str(uns32 ts, char * tx);
BOOL odbc_str2datim(char * tx, uns32 * ts);
BOOL x_str2odbc_timestamp(char * tx, TIMESTAMP_STRUCT * pdtm);


#define USE_OWNER

#ifdef USE_OWNER
#define INFO_OWNER_LEN  OBJ_NAME_LEN
#define INFO_QUALIF_LEN 0
#else
#define INFO_OWNER_LEN  0
#define INFO_QUALIF_LEN OBJ_NAME_LEN
#endif

#define US_PASSWD_LEN 31

extern char CONFIG_PATH[];
extern char LAST_USERNAME[];
extern char SCHEMANAME[];
extern char ODBC_INI[];
extern char DEFAULT[];

#define MAX_CPATH 170

//BOOL parse_string (const char *inp, int inplen, char **keynames, int keycount, char **values, int *lenlimits, char delimiter) ;
extern int envi_charset;
//////////////////////////////////// Unicode conversions ////////////////////////////////////////////////
class wide2ansi
{ SQLINTEGER alen;
  char * astr;
 public:
  wide2ansi(const SQLWCHAR * widestr, SQLINTEGER wlen, int charset = envi_charset);
  char * get_string(void)
    { return astr; }
  SQLINTEGER get_len(void) const
    { return alen; }
  ~wide2ansi(void)
    { if (astr!=NULLSTRING) corefree(astr); }
};

bool write_string_wide_ch(const char * text, SQLWCHAR *szStrOut, SQLSMALLINT cbStrOutMax, SQLSMALLINT *pcbStrOut, int charset = envi_charset);
bool write_string_wide_b (const char * text, SQLWCHAR *szStrOut, SQLSMALLINT cbStrOutMax, SQLSMALLINT *pcbStrOut, int charset = envi_charset);
bool write_string_wide_b4(const char * text, SQLWCHAR *szStrOut, SQLINTEGER cbStrOutMax,  SQLINTEGER *pcbStrOut,  int charset = envi_charset);
int odbclen(const SQLWCHAR * str);

#ifdef LINUX
int ODBCWideCharToMultiByte(const char *charset, const char *wstr, size_t len, char *buffer);
int ODBCMultiByteToWideChar(const char *charset, const char *str, size_t len, wuchar*buffer);
#endif
