/****************************************************************************/
/* The S-Pascal compiler header file                                        */
/****************************************************************************/
#ifndef IS_COMPIL_H
#define IS_COMPIL_H
#include <setjmp.h>
#ifdef WINS
#if WBVERS<900
#include <olectl.h>  // problems in Express VC with old SDK
#include "ansiapi.h"
#include "axinfo.h"
#else
typedef void * LPACTIVEXINFO;
#endif
#else
typedef void * LPACTIVEXINFO;
#endif
/****************************** global defs *********************************/
/************************ aggregates in reports ****************************/
/* Values of wexception: */
#define NO_EXCEPTION    0
#define FP_ERROR        1
#define DIV0_ERROR      2
#define STEP_AND_BP     10
#define TRACE_AND_BP    11
#define SET_BP          12
#define IN_STEP         13
#define IN_TRACE        14
#define BREAK_EXCEPTION 15

/* Values of global_state: */
#define DEVELOP_MENU     0
#define DESIGN_TABLE     1
#define DESIGN_VIEW      2
#define DESIGN_MENU      3
#define DESIGN_CURSOR    4
#define PROG_RUNNING     20
#define MENU_RUNNING     21
#define VIEW_RUNNING     22
#define DEBUG_BREAK      30

#define MAX_INDEX_CODE_SIZE   200
//////////////////////////////////// preprocessor /////////////////////////////
struct t_define
{ tname name;
  t_define * params;
  tptr expansion;
  t_define * next;
  t_define(void) { params=next=NULL;  expansion=NULL; }
  void del_recurs(void)
  { if (next) next->del_recurs();
    delete this;
  }
  ~t_define(void) 
  { corefree(expansion); 
    if (params) params->del_recurs();
  }
};

struct t_expand
{ const char * prev_univ_source;
  const char * prev_compil_ptr;
  tobjnum      prev_src_objnum;
  int          prev_src_line;
  tobjname     prev_src_name;
  BOOL         include;
  tptr         source;
  t_define * other_defines;
  t_expand * next;
  t_expand(void)
    { next=NULL;  source=NULL; }
  ~t_expand(void)
  { if (other_defines) other_defines->del_recurs();
    if (include) corefree(source);
  }
  void del_recurs(void)
  { if (next) next->del_recurs();
    delete this; 
  }    
  void save_state(CIp_t CI, BOOL is_include);
  void restore_state(CIp_t CI);
};

enum t_cc_state { CCS_ACTIVE, CCS_POSTACTIVE, CCS_PREACTIVE };

struct t_condcomp
{ int startline;
  t_cc_state state;
  t_condcomp * next;
  t_condcomp(CIp_t CI);
};
/*************************** Compiler interface *****************************/
#define DUMMY_OUT      0
#define MEMORY_OUT     1
#define DB_OUT         2  // write output to CI->cb_object (defined, object locked)
/************************* global project strucures *************************/
struct objtable;
typedef short symbol;
#define MIN_STRING_LEN   50
#define MAX_STRING_LEN 4000
typedef char tname[NAMELEN+1];
typedef uns32 code_addr;

typedef enum { no_temp_object=0, temp_menu } temp_type;

struct compil_info;  /* the compiler interface */
typedef compil_info * CIp_t;

typedef void (WINAPI * LPSTNETFCE)(CIp_t CI);

struct t_sql_kont;

typedef void t_compile_callback(int local_lines, int total_lines, const char * name);

struct compil_info  /* the compiler interface */
{ t_compile_callback * comp_callback; // reporting compilation progress if not NULL
  ctptr univ_source;                  // full source text
  kont_descr * kntx;       /* compilation kontext */
  uns8 output_type;
  tobjname src_name;
  ctptr outname;          /* name of the compiled object */
  tptr univ_code;          /* returned code for output_type==TO_MEMORY */
  LPSTNETFCE startnet;   /* procedure compiling the starting nonterminal */
  uns16 sc_col;
  code_addr code_offset;       /* the main program counter */
  tobjnum cb_object;       // database object number for the code, defined on entry iff output_type==DB_OUT
#ifndef WINS
  LONGJMPBUF comp_jmp;     /* long jump on compiler error */
#endif
  ctptr compil_ptr;  // current source pointer 
  struct objtable * id_tables;
  aggr_info * aggr;       /* aggregate structure */
  global_project_type * curr_project;
  unsigned char curchar;
  symbol cursym;
  sig64  curr_int;
  int    curr_scale; // defined for integer constants only
  monstr curr_money;
  double curr_real;
  tobjname  curr_name;
  tobjname  name_with_case; /* copy of CI->curr_name with case preserverd */
  t_dynar string_dynar;
  tptr curr_string(void)  { return (tptr)string_dynar.acc0(0); }
  t_specif glob_db_specif;
  t_specif prev_db_specif;
  uns8  inaggreg;              /* aggregate function nesting counter */
  wbbool any_alloc;
  uns32 loc_line, src_line; // src_line increased to loc_line after generating dng info!
  uns16 loc_col;
  uns16 err_object, err_subobj;
  unsigned cb_size;
  uns32 cb_start;
  void * univ_ptr;         /* universal pointer (menu structure etc.) */
  void * stmt_ptr;         /* sql statement sequence pointer */
  void * temp_object;     
  temp_type temp_object_type;  /* temporary object description */
  ctptr prepos;
  char genbuf;
 /* program compilation & includes */
  tobjnum src_objnum;  
 /* debugging info */
  wbbool gener_dbg_info;
  dbg_info * dbgi;
 /* keywords */
  ctptr keynames;
  unsigned key_count;
  cdp_t cdp;
  uns32 total_line;
  int aux_info;
  tobjnum thisform;     // form kontext or zero
  wbbool translation_disabled;
  t_define * defines;
  t_expand * expansion;
  t_condcomp * condcomp;
  int comp_flags;
  LPACTIVEXINFO AXInfo;
  int source_counter;  // number of source numbers in the list
  pgm_header_type * phdr; // pointer to program header whem compiling a program
  t_last_comment last_comment;
  int sys_charset;
  int last_column_number;  // number of the last database column found in the compilation

  compil_info(cdp_t cdpIn, ctptr univ_sourceIn, uns8 output_typeIn, LPSTNETFCE startnetIn) : string_dynar(1,0,0)
  { memset(this, 0, sizeof(compil_info));
    last_comment.init();
    string_dynar.init(1,30,30); // above construction damaged by memset()
    cdp=cdpIn;  univ_source=univ_sourceIn;  output_type=output_typeIn;
    if (cdp) sys_charset=cdp->sys_charset; else sys_charset=0;
    startnet=startnetIn;
  }
  compil_info(void) : string_dynar(1,0,0)
  { memset(this, 0, sizeof(compil_info)); 
    last_comment.init();
    string_dynar.init(1,30,30); // above construction damaged by memset()
  }
  inline bool is_ident(const char * name) const
    { return cursym==S_IDENT && !strcmp(curr_name, name); }
};

class CIpos
{ uns32 loc_line;  uns16 loc_col;
  char curchar;
  const char * compil_ptr;
  const char * prepos;
  symbol cursym;
 public:
  CIpos(CIp_t CI)
  { loc_line=CI->loc_line;  loc_col=CI->loc_col;
    curchar=CI->curchar;  compil_ptr=CI->compil_ptr;
    prepos = CI->prepos; cursym = CI->cursym;
  }
  void restore(CIp_t CI)
  { CI->loc_line=loc_line;  CI->loc_col=loc_col;
    CI->curchar=curchar;  CI->compil_ptr=compil_ptr;
    CI->prepos = prepos; CI->cursym = cursym;
  }
};

// comp_flags values:
#define COMP_FLAG_VIEW               1
#define COMP_FLAG_TABLE              2
#define COMP_FLAG_NO_COLOR_TRANSLATE 4
#define COMP_FLAG_DISABLE_PROJECT_CHANGE 8

extern "C" {
DllPrezen void WINAPI free_project(cdp_t cdp, BOOL total);
DllKernel void WINAPI exec_constructors(cdp_t cdp, BOOL constructing);
DllKernel void WINAPI update_object_nums(cdp_t cdp);
void compiler_init(void);  /* called only once before 1st compilation */
DllKernel int  WINAPI compile(CIp_t CI);        /* the compilation procedure */
DllKernel void WINAPI c_end_check(CIp_t CI);
DllKernel void  set_project_decls(CIp_t CI);
DllKernel int   WINAPI check_atr_name(tptr name);
DllKernel void WINAPI make_safe_ident(const char * pattern, char * ident, int maxlen);
#define IDCL_IPL_KEY    1
#define IDCL_SQL_KEY    2
#define IDCL_STDID      4
#define IDCL_VIEW       8
#define IDCL_SQL_STDID 16
DllKernel void  WINAPI set_compiler_keywords(CIp_t CI,
                           char * keynames, unsigned key_count, int keyset);

DllKernel void WINAPI program       (CIp_t CI);
DllKernel void WINAPI statement_seq (CIp_t CI);
DllKernel void WINAPI select_compile_entry(CIp_t CI);
DllKernel void WINAPI event_handler(CIp_t CI);
}


#define START_LOCAL_IDS 10
#define NEXT_LOCAL_IDS   7
#define ACT_NAMES_CNT   11

#define S_NEXTEQU    20
/************************ interpreter interface *****************************/
#define EXECUTED_OK              1
#define PGM_BREAKED              2

#define UNDEF_INSTR              2200
#define STACK_OVFL               2201
#define FUNCTION_NOT_FOUND       2202
#define DLL_NOT_FOUND            2203
#define KERNEL_CALL_ERROR        2204 // will be translated to server error
#define ACTION_BREAK             2205 // breaks the action sequence, not reported 
#define NIL_POINTER              2206
#define UNSPECIFIED_ERROR        2207 // internal inconsistencies
#define ATTRIBUTE_DOESNT_EXIST   2208
#define OUT_OF_R_MEMORY          2209
#define MATHERR                  2210
#define PGM_BREAK                2211
#define OBJECT_DOESNT_EXIST      2212
#define OUT_OF_DYNAMIC_MEMORY    2213
#define NO_RUN_RIGHT             2214
#define PROJECT_CHANGED          2215
#define UNCONVERTIBLE_UNTYPED    2216
#define ARRAY_BOUND_EXCEEDED     2217
#define EAX_FAIL                 2218  // ActiveX operation errors:
#define EAX_BADPARAMCOUNT        2219
#define EAX_TYPEMISMATCH         2220
#define EAX_EXCEPTION            2221
#define EAX_MEMBERNOTFOUND       2222
#define EAX_OVERFLOW             2223

#define FIRST_RT_ERROR           UNDEF_INSTR

typedef unsigned char dbname[ATTRNAMELEN+1];

/************************* public functions *********************************/
#ifdef __cplusplus
extern "C" {
#endif
DllKernel void  WINAPI run_prep(cdp_t cdp, tptr code_buffer, uns32 start_pc, uns8 wexception=NO_EXCEPTION);
DllKernel int   WINAPI run_pgm(cdp_t cdp);
DllKernel BOOL  WINAPI wsave_run(cdp_t cdp);
DllKernel void  WINAPI wrestore_run(cdp_t cdp);
DllKernel void  WINAPI view_def_comp   (CIp_t CI);  /* "nonterminal" procedures */
DllKernel void  WINAPI view_struct_comp(CIp_t CI);
DllKernel void  WINAPI view_info_comp  (CIp_t CI);
DllKernel void  WINAPI replrel_comp    (CIp_t CI);
DllKernel void  WINAPI table_def_comp_link(CIp_t CI);
DllKernel void  WINAPI view_access     (CIp_t CI);
DllKernel void  WINAPI view_value      (CIp_t CI);
DllKernel void  WINAPI sql_from        (CIp_t CI);   /* cannot be called from kernel */
DllKernel void  WINAPI sql_analysis    (CIp_t CI);
DllKernel int   WINAPI close_run(cdp_t cdp);
DllKernel void  WINAPI set_record_cache(tcursnum cursnum, trecnum recnum);
DllKernel run_state * WINAPI get_RV(void);
void table_def_comp(CIp_t CI, table_all * ta, table_all * ta_old);
DllKernel void WINAPI sql_statement_seq(CIp_t CI);
BOOL get_table_name(CIp_t CI, char * tabname, char * schemaname, ttablenum * ptabobj);
DllKernel tptr WINAPI table_all_to_source(table_all * ta, BOOL altering);
DllKernel BOOL WINAPI TranslateRegisteredKeys(run_state * RV, MSG * msg);
DllKernel LONG WINAPI final_color(LONG color);

#define AG_TYPE_MIN 0
#define AG_TYPE_MAX 1
#define AG_TYPE_SUM 2
#define AG_TYPE_LCN 3
#define AG_TYPE_AVG 3
#define AG_TYPE_CNT 4
/****************************** aggregates **********************************/
DllKernel short WINAPI aggr_post    (cdp_t cdp, trecnum pos);
DllKernel void  WINAPI aggr_in      (cdp_t cdp, trecnum rec);
DllKernel void  WINAPI aggr_reset   (cdp_t cdp, int level);
DllKernel int   WINAPI test_grouping(cdp_t cdp, trecnum rec);
DllKernel void  WINAPI free_aggr_info(aggr_info ** paggr);
 
DllKernel int   WINAPI is_bp_set(cdp_t cdp, uns8 filenum, unsigned linenum);
DllKernel BOOL  WINAPI toggle_bp(cdp_t cdp, uns8 filenum, unsigned linenum);
DllKernel BOOL  WINAPI set_bp   (cdp_t cdp, uns8 filenum, unsigned linenum, uns32 hoffset, uns8 bp_type);
DllKernel uns8  WINAPI find_file_num   (cdp_t cdp, tobjnum objnum);
DllKernel void  WINAPI free_run_vars   (cdp_t cdp); /* used by EXIT in debugger */
DllKernel void  WINAPI line_from_offset(cdp_t cdp, unsigned locoffset, uns8 * loccode, unsigned * linenum, uns8 * filenum);
DllKernel BOOL  WINAPI offset_from_line(cdp_t cdp, unsigned linenum, uns8 filenum, uns32 * hoffset);

DllKernel UDWORD WINAPI WB_type_length   (int type);
DllKernel void WINAPI TIMESTAMP2datim(TIMESTAMP_STRUCT * ts, uns32 * dtm);
DllKernel void WINAPI datim2TIMESTAMP(uns32 dtm, TIMESTAMP_STRUCT * ts);
DllKernel BOOL WINAPI str2odbc_timestamp(const char * txt, TIMESTAMP_STRUCT * ts);
void time2TIME(uns32 tm, TIME_STRUCT * ts);
void date2DATE(uns32 dt, DATE_STRUCT * ts);
void TIME2time(TIME_STRUCT * ts, uns32 * tm);
void DATE2date(DATE_STRUCT * ts, uns32 * dt);
}

#endif  /* IS_COMPIL_H */

