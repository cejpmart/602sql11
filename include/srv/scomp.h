/****************************************************************************/
/* comp.h - compiler-related defs for server                                */
/****************************************************************************/
#define CPPEXCEPTIONS
#ifdef CPPEXCEPTIONS
#define c_error(mess) throw(mess)
#define c_err(mess)   throw(mess)
#else
#ifdef WINS
#define c_error(mess) RaiseException(mess,0,0,NULL)
#define c_err(mess)   RaiseException(mess,0,0,NULL)
#else
#include <setjmp.h>
#define c_error(mess) MyThrow(CI->comp_jmp,mess)
#define c_err(mess)   MyThrow(comp_jmp,mess)
#endif
#endif

struct compil_info;  /* the compiler interface */
typedef struct compil_info * CIp_t;
typedef void (WINAPI * LPSTNETFCE)(CIp_t CI);

struct t_sql_kont;

#define VALID_COMP_ERR_CODE(code) ((code)>=FIRST_COMP_ERROR && (code)<=LAST_COMP_ERROR || (code)>=128 && (code)<=LAST_DB_ERROR || (code)==99)

struct compil_info  /* the compiler interface */
{ ctptr univ_source;/* full source text */
  LPSTNETFCE startnet;   /* procedure compiling the starting nonterminal */
#ifndef CPPEXCEPTIONS
#ifndef WINS
  LONGJMPBUF comp_jmp;     /* long jump on compiler error */
#endif
#endif
  ctptr compil_ptr;  /* source pointer */
  uns32  curchar;    // can be 8-bit character, 16-bit or 32-bit unicode character
  symbol cursym;
  sig64  curr_int;
  uns8   curr_tp, curr_sz;
  int    curr_scale; // defined for integer constants only
  double curr_real;
  tobjname  curr_name;
  tobjname  name_with_case; /* copy of CI->curr_name with case preserverd */
  t_dynar string_dynar;
  tptr curr_string(void)  { return (tptr)string_dynar.acc0(0); }
  uns32 loc_line,     // current line num, from 1
        prev_line;    // line containing the previous symbol
  uns16 loc_col;      // current column
  uns16 sc_col;            // column with the start of the last symbol 
  void * univ_ptr;         /* universal pointer (menu structure etc.) */
  void * stmt_ptr;         /* sql statement sequence pointer */
  t_atrmap_flex * atrmap;  // not owning!
  ctptr prepos;
  cdp_t cdp;
  t_last_comment last_comment;
  int recursion_counter;

  t_specif src_specif;
  int charsize;  // derived from src_specif in set_src_specif()
  void set_src_specif(t_specif specif)
    { src_specif=specif;  charsize = specif.wide_char==2 ? sizeof(uns32) : specif.wide_char==1 ? sizeof(wuchar) : 1; }
  t_sql_kont * sql_kont;

  void null_it(void)
    { univ_ptr=stmt_ptr=NULL;  atrmap=NULL;  sql_kont=NULL;  univ_source=NULL;  set_src_specif(sys_spec);  recursion_counter=0; }
  compil_info(cdp_t cdpIn, ctptr univ_sourceIn, LPSTNETFCE startnetIn) : string_dynar(1,30,30)
  { null_it();
    cdp=cdpIn;  univ_source=univ_sourceIn;  
    startnet=startnetIn;
  }
  compil_info(void) : string_dynar(1,30,30)  { null_it(); }
  void propagate_error(void) 
  { int err=cdp->get_return_code(); 
    if (!VALID_COMP_ERR_CODE(err)) err=GENERAL_SQL_SYNTAX;  // error may have been masked but I must throw a valid code
    c_err(err);
  }
  inline bool is_ident(const char * name) const
    { return cursym==S_IDENT && !strcmp(curr_name, name); }
};

class CIpos
{ uns32 loc_line;  uns16 loc_col;
  uns32 curchar;
  const char * compil_ptr;
 public:
  CIpos(CIp_t CI)
  { loc_line=CI->loc_line;  loc_col=CI->loc_col;
    curchar=CI->curchar;  compil_ptr=CI->compil_ptr;
  }
  void restore(CIp_t CI)
  { CI->loc_line=loc_line;  CI->loc_col=loc_col;
    CI->curchar=curchar;  CI->compil_ptr=compil_ptr;
  }
};

extern "C" {
void compiler_init(void);  /* called only once before 1st compilation */
void c_end_check(CIp_t CI);
int   WINAPI check_atr_name(tptr name);
}
void WINAPI sql_statement_seq(CIp_t CI);
void WINAPI replrel_comp    (CIp_t CI);
uns32 GetReplInfo(void);

void table_def_comp(CIp_t CI, table_all * ta, table_all * ta_old, table_statement_types statement_type);
uns32 WINAPI next_char(CIp_t CI);

int WINAPI compile(CIp_t CI);        /* the compilation procedure */
int WINAPI compile_masked(CIp_t CI);
void create_error_contect(CIp_t CI);
object * search_obj(const char * name);

sig32 get_num_val(CIp_t CI, sig32 lbd, sig32 hbd);
sig32 num_val(CIp_t CI, sig32 lbd, sig32 hbd);
/////////////////////////////////// server extensions /////////////////////////////////
struct ext_parsm
{ uns8 categ;
  uns8 dtype;
  int  specif;
};

struct extension_object
{ int category;
  int retvaltype;
  int retvalspecif;
  const char * expname;
  ext_parsm * pars;
};

struct ext_objdef             /* extension object definition */
{ tobjname name;              /* object's name */
  extension_object * descr;   /* pointer to the description of the object */
};

struct t_server_ext
{ char extname[20+1];
  HINSTANCE hInst;
  ext_objdef * objdefs;
  unsigned objcnt;
  t_server_ext * next;
};

t_scope * make_scope_from_cursor(cdp_t cdp, cur_descr * cd);
bool add_cursor_structure_to_scope(cdp_t cdp, CIp_t CI, t_scope & scope, db_kontext * kont, const char * name);
bool load_cursor_record_to_rscope(cdp_t cdp, cur_descr * cd, t_rscope * rscope);
void release_cursor_rscope(cdp_t cdp, cur_descr * cd, t_rscope * rscope);
t_rscope * create_and_activate_rscope(cdp_t cdp, t_scope & scope, int scope_level);
void deactivate_and_drop_rscope(cdp_t cdp);
const trecnum * get_recnum_list(cur_descr * cd, trecnum crec);
void destruct_scope(t_scope * scope);
struct t_locdecl;
struct t_row_elem;
void access_locdecl_struct(t_rscope * rscope, int column_num, t_locdecl *& locdecl, t_row_elem *& rel);
tptr ker_load_object(cdp_t cdp, trecnum recnum);
void ker_free(void * ptr);
void * ker_alloc(int size);
tcursnum open_cursor_in_current_kontext(cdp_t cdp, const char * query, cur_descr ** pcd);

extern t_server_ext * loaded_extensions;
void mark_extensions(void);
extension_object * search_obj_in_ext(cdp_t cdp, const char * name, const char * prefix, HINSTANCE * hInst);
bool load_server_extension(cdp_t cdp, const char * name);

///////////////////////////// 5.1 //////////////////////////////////////////
// Standard SQL functions with a non-standard syntax
#define FCNUM_POSITION          0
#define FCNUM_CHARLEN           1
#define FCNUM_BITLEN            2
#define FCNUM_EXTRACT           3
#define FCNUM_EXTRACT_YEAR      FCNUM_EXTRACT
#define FCNUM_EXTRACT_MONTH     4
#define FCNUM_EXTRACT_DAY       5
#define FCNUM_EXTRACT_HOUR      6
#define FCNUM_EXTRACT_MINUTE    7
#define FCNUM_EXTRACT_SECOND    8
#define FCNUM_SUBSTRING         9
#define FCNUM_LOWER            10
#define FCNUM_UPPER            11
#define FCNUM_USER             12
#define FCNUM_USER_BINARY      13
#define FCNUM_FULLTEXT         14
#define FCNUM_SELPRIV          15
#define FCNUM_UPDPRIV          16
#define FCNUM_DELPRIV          17
#define FCNUM_INSPRIV          18
#define FCNUM_GRTPRIV          19
#define FCNUM_ADMMODE          20
#define FCNUM_FULLTEXT_CONTEXT 21
#define FCNUM_OCTETLEN         22
#define FCNUM_CONSTRAINS_OK    23
#define FCNUM_FULLTEXT_CONTEXT2 24
#define FCNUM_COMP_TAB         25
#define FCNUM_GET_FULLTEXT_CONTEXT 26

#define FCNUM_CURR_SCHEMA      151

#define FCNUM_DEFINE_LOG       180
#define FCNUM_FT_CREATE        181
#define FCNUM_FT_INDEX         182
#define FCNUM_FT_REMOVE        183
#define FCNUM_TRACE            184
#define FCNUM_SQL_EXECUTE      185
#define FCNUM_FT_DESTROY       186

#define FCNUM_BIGINT2STR       199
#define FCNUM_STR2BIGINT       200
#define FCNUM_MBCRETBL         201
#define FCNUM_WB_BUILD         202
#define FCNUM_ENUMRECPR        203
#define FCNUM_ENUMTABPR        204
#define FCNUM_SETPASS          205
#define FCNUM_MEMBERSHIP       206
#define FCNUM_LOGWRITEEX       207
#define FCNUM_MBGETMSGEX       208
#define FCNUM_MBGETMSGEX       208
#define FCNUM_MLADDBLOBR       209
#define FCNUM_MLADDBLOBS       210
#define FCNUM_MBSAVETODBR      211
#define FCNUM_MBSAVETODBS      212
#define FCNUM_APPLSHARED       213
#define FCNUM_MEMBERSHIP_GET   214
#define FCNUM_ROUND64          215
#define FCNUM_GETSQLOPT        216
#define FCNUM_MCREATEPROF      217
#define FCNUM_MDELETEPROF      218
#define FCNUM_MSETPROF         219
#define FCNUM_MGETPROF         220
#define FCNUM_GETSERVERINFO    221
#define FCNUM_ACTIVE_ROUT      222
#define FCNUM_INVOKE_EVENT     223
#define FCNUM_GET_LIC_CNT      224
#define FCNUM_GET_PROP         225
#define FCNUM_SET_PROP         226
#define FCNUM_LOAD_EXT         227
#define FCNUM_TRUCT_TABLE      228
#define FCNUM_GETSRVERRCNT     229
#define FCNUM_SCHEMA_ID        230
#define FCNUM_LOCAL_SCHEMA     231
#define FCNUM_LOCK_BY_KEY      232
#define FCNUM_LETTERCREW       233
#define FCNUM_GETSRVERRCNTTX   234
#define FCNUM_LETTERADDADRW    235
#define FCNUM_LETTERADDFILEW   236
#define FCNUM_MLADDBLOBRW      237
#define FCNUM_MLADDBLOBSW      238
#define FCNUM_ENABLE_INDEX     239
#define FCNUM_REGEXPR          240
#define FCNUM_PROF_ALL         241
#define FCNUM_PROF_THR         242
#define FCNUM_PROF_RESET       243
#define FCNUM_PROF_LINES       244
#define FCNUM_SET_TH_NAME      245
#define FCNUM_MESSAGE_TO_CLI   246
#define FCNUM_CHECK_FT         247
#define FCNUM_INITWBMAILEX     248
#define FCNUM_FT_LOCKING       249
#define FCNUM_CRE_SYSEXT       250
#define FCNUM_BACKUP_FIL       251
#define FCNUM_REPLICATE        252
#define FCNUM_FATAL_ERR        253
#define FCNUM_INTERNAL         254
#define FCNUM_CHECK_INDEX      255
#define FCNUM_DB_INT           256
#define FCNUM_BCK              257
#define FCNUM_BCK_GET_PATT     258
#define FCNUM_BCK_REDUCE       259
#define FCNUM_BCK_ZIP          260
#define FCNUM_CONN_DISK        261
#define FCNUM_DISCONN_DISK     262
#define FCNUM_EXTLOB_FILE      263

int string_position(const char * s1, const char * s2, int type);

extern "C" {
UDWORD WINAPI WB_type_length   (int type);
void WINAPI TIMESTAMP2datim(TIMESTAMP_STRUCT * ts, uns32 * dtm);
void WINAPI datim2TIMESTAMP(uns32 dtm, TIMESTAMP_STRUCT * ts);
BOOL WINAPI str2odbc_timestamp(const char * txt, TIMESTAMP_STRUCT * ts);
void time2TIME(uns32 tm, TIME_STRUCT * ts);
void date2DATE(uns32 dt, DATE_STRUCT * ts);
void TIME2time(TIME_STRUCT * ts, uns32 * tm);
void DATE2date(DATE_STRUCT * ts, uns32 * dt);
}
sig32 check_integer(CIp_t CI, sig32 lbound, sig32 ubound);

////////////// system information view numbers ///////////////////////////////
#define IV_NUM_LOGGED_USERS   0
#define IV_TABLE_COLUMNS      1
#define IV_MAIL_PROFS         2
#define IV_RECENT_LOG         3
#define IV_LOG_REQS           4
#define IV_PROC_PARS          5
#define IV_LOCKS              6
#define IV_SUBJ_MEMB          7
#define IV_PRIVILEGES         8
#define IV_CHECK_CONS         9
#define IV_FORKEYS           10
#define IV_INDICIES          11
#define IV_CERTIFICATES      12
#define IV_IP_ADDRESSES      13
#define IV_VIEWED_COLUMNS    14
#define IV_ODBC_TYPES              15
#define IV_ODBC_COLUMNS            16
#define IV_ODBC_COLUMN_PRIVS       17
#define IV_ODBC_FOREIGN_KEYS       18
#define IV_ODBC_PRIMARY_KEYS       19
#define IV_ODBC_PROCEDURES         20
#define IV_ODBC_PROCEDURE_COLUMNS  21
#define IV_ODBC_SPECIAL_COLUMNS    22
#define IV_ODBC_STATISTICS         23
#define IV_ODBC_TABLES             24
#define IV_ODBC_TABLE_PRIVS        25
#define IV_PROFILE                 26
#define IV_TABLE_STAT              27
#define IV_OBJECT_ST               28
#define IV_MEMORY_USAGE            29
#define IV_CLIENT_ACTIV            30

// IVP_... value must be different from IV_.. values and FCNUM_... values
#define IVP_FT_WORDTAB  1000
#define IVP_FT_REFTAB   1001
#define IVP_FT_REFTAB2  1002

#define ATT_objnum ATT_INT32

