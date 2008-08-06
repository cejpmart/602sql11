#if WBVERS>=900
#define SQL3 1
#endif
#define SQL3 1

#ifdef LINUX
#include <wchar.h>
#endif
//struct t_express;
struct t_query_specif;
struct t_query_expr;
class t_flstr;

typedef enum { SQLPH_NORMAL, SQLPH_SELECT, SQLPH_WHERE,
         SQLPH_GROUP, SQLPH_HAVING, SQLPH_ORDER, SQLPH_INAGGR } t_comp_phase;
#define SaferGetCurrTaskPtr (GetCurrTaskPtr() ? GetCurrTaskPtr() : cds[0])
void data_not_closed(void);
struct db_kontext;
struct t_materialization;
struct t_mater_table;
struct t_mater_ptrs;
struct t_optim;
struct row_distinctor;
class variant_dispatcher;
struct t_ft_kontext;
struct t_ft_node;
struct t_ivcols;

SPECIF_DYNAR(objnum_dynar, tobjnum, 3);

////////////////////////////// db_kontext ///////////////////////////////////
// Priviledge flags can be combined by bitwise AND:
#define PRFL_READ_TAB    0x3
#define PRFL_READ_REC    0x1
#define PRFL_READ_NO     0
#define PRFL_WRITE_TAB   0xc
#define PRFL_WRITE_REC   0x4
#define PRFL_WRITE_NO    0
#define PRFL_ALL         0xf

#define ELFL_NOEDIT      0x10  // indicates that the value is not editable (independent of privils)

enum t_elem_link { ELK_NO, ELK_KONTEXT, ELK_KONT2U, ELK_KONT21, ELK_EXPR, ELK_AGGREXPR };

struct elem_descr
{ tobjname name;
  //tobjname prefix;
  uns8     type;
  uns8     multi;
  t_specif specif;  
  BOOL     nullable;
  int      pr_flags; // priviledge flags & editability flags
 // evaluation information:
  t_elem_link link_type;
  union
  { struct // when link_type==ELK_KONTEXT
    { int          down_elemnum;
      db_kontext * kont;   
    } kontext;
    struct // when link_type==ELK_KONT2x, used by UNION, EXCEPT, INTERSECT
    { int          down_elemnum1;
      db_kontext * kont1;  
      int          down_elemnum2;
      db_kontext * kont2;  
    } kontdbl;
    struct // when link_type==ELK_EXPR or ELK_AGGREXPR
    { t_express * eval;
    } expr;
    struct // when link_type==ELK_NO
    { ttablenum tabnum;  // table number for permanent tables, NOTBNUM for system views, undefined otherwise
    } dirtab;
  };
  void store_double_kontext(const elem_descr * el1, const elem_descr * el2, 
                            db_kontext * k1, db_kontext * k2, int elemnum1, int elemnum2, bool is_union);
  void store_kontext(const elem_descr * el1, db_kontext * k1, int elemnum1);
  void mark_core(void);
};

SPECIF_DYNAR(elem_dynar, elem_descr, 3);

struct db_kontext
{ tobjname   prefix_name;
  bool       prefix_is_compulsory;
  int        ordnum; // index of "position" value in the record number array
  elem_dynar elems;
  db_kontext * next;        // next active kontext in the scope (not owning), defined only for kontexts in active_kontext_list
  db_kontext * replacement; // replacing kontext (not owning)
  BOOL       referenced;

  void         set_correlation_name(const char * name);
  void         set_compulsory_prefix(const char * name);

  elem_descr * find(const char * prefix, const char * name, int & elemnum);
  BOOL         fill_from_td(cdp_t cdp, table_descr * td);
  BOOL         fill_from_ta(cdp_t cdp, const table_all * ta);
  BOOL         fill_from_ivcols(const t_ivcols * cols, const char * name);

  elem_descr * copy_elem(CIp_t CI, int delemnum, db_kontext * dkont, int common_limit);
  BOOL         copy_kont(CIp_t CI, db_kontext * patt, int start=0);
  void         init_curs(CIp_t CI);

 // run time:
  trecnum position;  // used on material levels only (permanent table or materialized)
#if SQL3
  bool grouping_here;
  trecnum pmat_pos;  // stored untranslated position, used get_position in INTERSECT / EXCEPT
  t_mater_table * t_mat;
  t_mater_ptrs  * p_mat;  // if both t_mat and p_mat exist then p_mat is created over t_mat
#else
  t_materialization * mat;
#endif
  t_privil_cache * privil_cache;
  bool virtually_completed;  // true if mater does not exist because of OJ, but should materialize here

  db_kontext(void)
  { prefix_is_compulsory=false;  prefix_name[0]=0;  virtually_completed=false;
    replacement=NULL;  privil_cache=NULL;
#if SQL3
    t_mat=NULL;  p_mat=NULL;  grouping_here=false;
#else
    mat=NULL;  
#endif
  }
  ~db_kontext(void);  // cannot be inline
  void mark_core(void);
  int first_nonsys_col(void);
  bool is_mater_completed(void) const;
};

void is_referenced(db_kontext * akont, int elemnum);

////////////////////////// interval lists ///////////////////////////////////

typedef enum { ILIM_UNLIM, ILIM_OPEN, ILIM_CLOSED, ILIM_EMPTY } t_ilim;

typedef enum { ICOMP_1THN2, ICOMP_2THN1, ICOMP_1CO2, ICOMP_2CO1,
               ICOMP_1CTS2, ICOMP_2CTS1, } t_icomp;

typedef struct t_interval
{ t_ilim llim, rlim;
  t_specif valspecif;
  t_interval * next;
  char values[1];
  void mark_core(void)
    { mark(this);  if (next) next->mark_core(); }
} t_interval;

///////////////////////////// scalar evaluation //////////////////////////////
enum t_valmode { mode_null, mode_simple, mode_indirect, mode_access };
#define MAX_DIRECT_VAL 34  // must be able to contain user name

struct t_value
// Rules: 1. indir is used only for var-len, long strings & binaries
//        2. dbacc is used only for multiattribs & var-len (strings & fixed binaries are always loaded).
{ int length;  // real length for string, binary & var len attributes only (used when making copy without type knowledge). Is in bytes, even for UNICODE strings.
  t_valmode vmode;
  union
  { sig32  intval;
    uns32  unsval;
    double realval;
    sig64  i64val;
    char   strval[MAX_DIRECT_VAL+2]; // +2 - for the delimiter of wide chars
    struct  // database access, (used as table access only, not the cursor access)
    { ttablenum tb;
      trecnum rec;
      tattrib attr;
      uns16 index;
    } dbacc;
    struct  // indirect value
    { int vspace;
      tptr val;
    } indir;
  };

  inline void * malloc(int size) { return corealloc(size, 45); }
  inline void free(void * ptr) { corefree(ptr); }

  inline bool is_null(void)  const      { return vmode==mode_null; }
  inline bool is_true(void)  const      { return vmode!=mode_null && intval; }
  inline bool is_false(void) const      { return vmode!=mode_null && !intval; }
  inline void init(void)                { vmode = mode_null; } // like the constructor, does not suppose anything about *this 
 private:
  inline void free_ival(void) { if (vmode==mode_indirect) { free(indir.val);  indir.val=NULL;  indir.vspace=0; } }
    // Must be called whenever new value is stored into t_value containing a possibly indirect value
    // Caution: Call set_null() or set_simple_not_null() instead of free_ival() because if [vmode] remains mode_indirect
    //  then the next set_null() will overwrite the value!
 public:   
  inline void set_null(void)            
    { free_ival();  vmode = mode_null; }
  inline void set_simple_not_null(void) 
    { free_ival();  vmode = mode_simple; } 
  inline void set_db_acc(ttablenum tabnum,  trecnum recnum, tattrib elemnum, uns16 indxval)  // used as table access only, not the cursor access
    { free_ival();  vmode = mode_access;  dbacc.tb=tabnum;  dbacc.rec=recnum;  dbacc.attr=elemnum;  dbacc.index=indxval; }
  inline char   * valptr(void) { return vmode==mode_indirect ? indir.val : strval; }
  inline wuchar * wchptr(void) { return (wuchar*)valptr(); }
  BOOL allocate(int space);
  BOOL reallocate(int space);
  BOOL load(cdp_t cdp);
  t_value & operator=(const t_value & src);
  inline t_value(void) { init(); }
  inline ~t_value(void) { free_ival(); }
  void mark_core(void)
    { if (vmode==mode_indirect && indir.val) mark(indir.val); }
};

////////////////////////////// compiled query ///////////////////////////////

#define MRG(pure_oper,arg_type) (256*(arg_type)+(pure_oper))

enum  // t_compoper.pure_oper
{ PUOP_EQ=1, PUOP_GT, PUOP_LE, PUOP_GE, PUOP_LT, PUOP_NE,  // starts at 1 in order to prevent wanings in expressions like (puop>=PUOP_EQ)
  PUOP_PLUS, PUOP_MINUS, PUOP_TIMES, PUOP_DIV, PUOP_MOD,
  PUOP_PREF, PUOP_SUBSTR, PUOP_LIKE, 
  PUOP_BETWEEN, PUOP_MULTIN,
  PUOP_AND, PUOP_OR, PUOP_IS_NULL, PUOP_COMMA, // from PUOP_AND operations on NULL values allowed
  PUOP_FULLTEXT };
enum  // t_compoper.arg_type
 { F_OPBASE, M_OPBASE, I_OPBASE, U_OPBASE, S_OPBASE, BI_OPBASE, DT_OPBASE, TM_OPBASE, TS_OPBASE };
enum {  // unary operation codes
 UNOP_INEG=3000, UNOP_MNEG, UNOP_FNEG, UNOP_EXISTS, UNOP_UNIQUE,
 /*UNOP_IS_TRUE, UNOP_IS_FALSE, UNOP_IS_UNKNOWN*/ };

enum t_conv_type { CONV_NO=0, CONV_NEGATE=1,
  CONV_CONV,  // fictive, real conversions follow
  CONV_S2S, CONV_I2I, CONV_M2M, CONV_F2F, // value checking and possible rotation only
  CONV_S2SEXPL, CONV_W2S, CONV_W2SEXPL,
  CONV_I2M, CONV_I2F, CONV_M2F, CONV_F2M, CONV_F2I, CONV_M2I,
  CONV_I2S, CONV_M2S, CONV_F2S, CONV_B2S, CONV_D2S, CONV_T2S, CONV_TS2S,
  CONV_S2I, CONV_S2M, CONV_S2F, CONV_S2D, CONV_S2T, CONV_S2TS,
  CONV_W2I, CONV_W2M, CONV_W2F, CONV_W2D, CONV_W2T, CONV_W2TS,
  CONV_D2TS,CONV_TS2D,CONV_T2TS,CONV_TS2T, CONV_T2T, CONV_TS2TS,
  CONV_I2BOOL, CONV_BOOL2S, CONV_S2BOOL, CONV_W2BOOL,
  CONV_NULL,  // using when converting the NULL value, no action, but materializes the NULL value in the final type
  CONV_VAR2BIN,  // converts strings and heap types into binary type of specified length
  CONV_ERROR = 9999 };

struct t_convert
{ t_conv_type conv : 16;
  union
  { sig16 scale_shift;
    struct
    { uns8 src_charset;
      uns8 dest_charset;
    } chs;
    struct
    { uns8 src_wtz;
      uns8 dest_wtz;
    } wtz;  // with time zone
    uns16 opq;
  };
  inline t_convert(t_conv_type convIn)
    { conv=convIn;  opq=0; }  // defined [opq] makes comparison simpler
  inline t_convert(t_conv_type convIn, sig16 scale_shiftIn)
    { conv=convIn;  scale_shift=scale_shiftIn; }
  inline t_convert(t_conv_type convIn, uns8 src_charsetIn, uns8 dest_charsetIn) // used for wtz, too
    { conv=convIn;  chs.src_charset=src_charsetIn;  chs.dest_charset=dest_charsetIn; }
  inline BOOL operator ==(const t_convert & second) const
    { return conv==second.conv && opq==second.opq; }
};

typedef enum { SQE_SCONST, SQE_LCONST, SQE_TERNARY, SQE_BINARY, SQE_UNARY,
               SQE_AGGR, SQE_FUNCT, SQE_ATTR, SQE_DYNPAR, SQE_EMBED, SQE_NULL,
               SQE_CASE, SQE_SUBQ, SQE_MULT_COUNT, SQE_VAR_LEN, SQE_DBLIND,
               SQE_CAST, SQE_VAR, SQE_ROUTINE, SQE_CURRREC, SQE_SEQ, SQE_POINTED_ATTR, SQE_ATTR_REF
             } t_expr_types;

#define OR_OR     0  // evaluate as OR
#define OR_DIVIDE 1  // divide into variants
#define OR_JOIN   2  // join into combined interval

struct t_express
{ const t_expr_types sqe;
  int type;
  t_specif specif;
  t_convert convert;
  int pr_flags;
  const db_kontext * tabkont; // !=NULL iff refers to attributes of single table
  virtual ~t_express(void) { }
  t_express(t_expr_types sqeIn) : sqe(sqeIn), specif(0), convert(CONV_NO)
    { tabkont=NULL;  pr_flags=PRFL_ALL | ELFL_NOEDIT; }
  void dump_expr(cdp_t cdp, t_flstr * dmp, bool eval);
  void *operator new(size_t size)
    { return corealloc(size, 201); }
  void operator delete(void * ptr)
    { corefree(ptr); }
  virtual void mark_core(void)  // marks itself, too
    { mark(this); }
  virtual void mark_disk_space(cdp_t cdp) const
    { }
  virtual void is_referenced(void)
    { }
  virtual void close_data(cdp_t cdp)
    { }
};

struct t_expr_binary : t_express
{ t_compoper oper;
  t_express *op1, *op2;  // owning
  BOOL direction;  // used when optimizing OR
  t_expr_binary(void) : t_express(SQE_BINARY)
    { op1=op2=NULL; }
  virtual ~t_expr_binary(void)
    { if (op1!=NULL) delete op1;  if (op2!=NULL) delete op2; }

  int opt_info;  // information for optimization phase (joining & dividing ORs)
  virtual void mark_core(void)
  { t_express::mark_core();
    if (op1) op1->mark_core();
    if (op2) op2->mark_core();
  }
  virtual void is_referenced(void)
  { if (op1) op1->is_referenced();
    if (op2) op2->is_referenced();
  }
  virtual void close_data(cdp_t cdp)
  { if (op1) op1->close_data(cdp);
    if (op2) op2->close_data(cdp);
  }
};

struct t_expr_ternary : t_express
{ t_compoper oper;
  t_express *op1, *op2, *op3;  // owning
  t_expr_ternary(void) : t_express(SQE_TERNARY)
    { op1=op2=op3=NULL; }
  virtual ~t_expr_ternary(void)
   { if (op1!=NULL) delete op1;  if (op2!=NULL) delete op2;
     if (op3!=NULL) delete op3;
   }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (op1) op1->mark_core();
    if (op2) op2->mark_core();
    if (op3) op3->mark_core();
  }
  virtual void is_referenced(void)
  { if (op1) op1->is_referenced();
    if (op2) op2->is_referenced();
    if (op3) op3->is_referenced();
  }
  virtual void close_data(cdp_t cdp)
  { if (op1) op1->close_data(cdp);
    if (op2) op2->close_data(cdp);
    if (op3) op3->close_data(cdp);
  }
};

struct t_expr_unary : t_express
{ int oper;
  t_express *op;  // owning
  t_expr_unary(int operIn) : t_express(SQE_UNARY)
    { oper=operIn;  op=NULL; }
  virtual ~t_expr_unary(void)
    { if (op!=NULL) delete op; }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (op) op->mark_core();
  }
  virtual void is_referenced(void)
    { if (op) op->is_referenced(); }
  virtual void close_data(cdp_t cdp)
    { if (op) op->close_data(cdp); }
};

struct t_expr_currrec : t_express
{ db_kontext * kont;  // not owning
  t_expr_currrec(db_kontext * kontIn) : t_express(SQE_CURRREC)
  { kont=kontIn;  type=ATT_INT32;  specif.set_num(0, 9); }
};

struct t_expr_seq : t_express
{ tobjnum seq_obj;
  int nextval;
  t_expr_seq(tobjnum seq_objIn, int nextvalIn) : t_express(SQE_SEQ)
  { seq_obj=seq_objIn;  nextval=nextvalIn;  type=ATT_INT64;  specif.set_num(0, 18); }
};

struct t_expr_sconst : t_express
// used only for simple types, not for Char, String, Binary, var-len!
{ uns8 val[8];  // extract_integer_condition supposes the basic value here
  int origtype;  BOOL const_is_null;
  t_expr_sconst(int typeIn, t_specif specifIn, const void * valIn, int valsize) : t_express(SQE_SCONST)
  { memcpy(val, valIn, valsize);  origtype=type=typeIn;  specif=specifIn;   
    const_is_null=is_null((const char *)val, origtype, specif);  // last parametr not used!
  }
};

struct t_expr_lconst : t_express
// used for Char, String, Binary, var-len!
{ tptr lval;  // owning
  int valsize;
  t_expr_lconst(int typeIn, void * valIn, int valsizeIn, int charset) : t_express(SQE_LCONST)
  { lval=(tptr)corealloc(valsizeIn+1, 88);
    if (lval) { memcpy(lval, valIn, valsizeIn);  lval[valsizeIn]=0; }
    valsize=valsizeIn;  type=typeIn;
    specif.set_string(valsize, charset, 0, 0);  // for long values the valsize may not be stored in specif
  }
  virtual ~t_expr_lconst(void)
    { if (lval!=NULL) corefree(lval); }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (lval!=NULL) mark(lval);
  }
};

struct t_expr_aggr : t_express
{ int          ag_type;        // S_SUM etc.
  BOOL         distinct;
  t_express  * arg;            // is NULL for COUNT(*)!
  int          agnum, agnum2;  // indicies in the aggregate function list
  db_kontext * kont;
  int          restype;        // type of the aggregation result

  t_expr_aggr(int ag_typeIn, db_kontext * kontIn) : t_express(SQE_AGGR)
  { ag_type=ag_typeIn;  arg=NULL;  distinct=FALSE;
    kont=kontIn;
  }
  virtual ~t_expr_aggr(void)
    { if (arg!=NULL) delete arg; }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (arg) arg->mark_core();
  }
  virtual void is_referenced(void)
    { if (arg) arg->is_referenced(); }
  virtual void close_data(cdp_t cdp)
    { if (arg) arg->close_data(cdp); }
};

struct t_expr_funct : t_express
{ int num;
  int origtype;  // function result type, not converted
  t_specif origspecif;
  t_express *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8;  // owning
  t_expr_funct(int numIn) : t_express(SQE_FUNCT), origspecif(0)
    { arg1=arg2=arg3=arg4=arg5=arg6=arg7=arg8=NULL;  num=numIn; }
  virtual ~t_expr_funct(void)
  { if (arg1!=NULL) delete arg1;
    if (arg2!=NULL) delete arg2;
    if (arg3!=NULL) delete arg3;
    if (arg4!=NULL) delete arg4;
    if (arg5!=NULL) delete arg5;
    if (arg6!=NULL) delete arg6;
    if (arg7!=NULL) delete arg7;
    if (arg8!=NULL) delete arg8;
  }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (arg1) arg1->mark_core();
    if (arg2) arg2->mark_core();
    if (arg3) arg3->mark_core();
    if (arg4) arg4->mark_core();
    if (arg5) arg5->mark_core();
    if (arg6) arg6->mark_core();
    if (arg7) arg7->mark_core();
    if (arg8) arg8->mark_core();
  }
  virtual void is_referenced(void)
  { if (arg1 && num!=FCNUM_SELPRIV) arg1->is_referenced();
    if (arg2) arg2->is_referenced();
    if (arg3) arg3->is_referenced();
    if (arg4) arg4->is_referenced();
    if (arg5) arg5->is_referenced();
    if (arg6) arg6->is_referenced();
    if (arg7) arg7->is_referenced();
    if (arg8) arg8->is_referenced();
  }
  virtual void close_data(cdp_t cdp)
  { if (arg1 && num!=FCNUM_SELPRIV) arg1->close_data(cdp);
    if (arg2) arg2->close_data(cdp);
    if (arg3) arg3->close_data(cdp);
    if (arg4) arg4->close_data(cdp);
    if (arg5) arg5->close_data(cdp);
    if (arg6) arg6->close_data(cdp);
    if (arg7) arg7->close_data(cdp);
    if (arg8) arg8->close_data(cdp);
  }
};

#define PCRE_STATIC
#include "pcre.h"

struct t_expr_funct_regex : t_expr_funct
{
  char * compiled_pattern;
  int compiled_options;
  pcre * code;
  t_expr_funct_regex(void) : t_expr_funct(FCNUM_REGEXPR)
    { compiled_pattern=NULL;  code=NULL; }
  virtual void mark_core(void)
  { t_expr_funct::mark_core();
    if (compiled_pattern) mark(compiled_pattern);
  }
  virtual ~t_expr_funct_regex(void)
  { corefree(compiled_pattern);
    if (code) /*(*pcre_*/free(code);
  }
  bool compile_pattern(cdp_t cdp, const char * patt, const char * options);
};

struct t_expr_attr : t_express
{ int elemnum;       // kontext element number or attribute number when kont==NULL
  db_kontext * kont; // NULL iff compiled in kont_ta kontext, uses kont_tbdf, kont_recnum then
  int origtype;      // expression type may be converted, origtype used when reading
  t_specif  origspecif;
  BOOL nullable;
  tobjname name;     // name used when comparing indicies (attr numbers may have changed) - type from elem_descr
  t_express * indexexpr; // index expression or NULL iff index not present 

  void get_kontext_info(int elemnumIn, db_kontext * kontIn)
  // stores kont and elemnum, loads column info from the kontext
  { elemnum=elemnumIn;  kont=kontIn;  
    elem_descr * el = kont->elems.acc0(elemnum);
    pr_flags=el->pr_flags;
    type=origtype=el->type;  specif=origspecif=el->specif;
    if (el->multi != 1) origtype = type |= 0x80;
    nullable=el->nullable;
    strcpy(name, el->name);
  }

  t_expr_attr(int elemnumIn, db_kontext * kontIn, const char * nameIn=NULL) : t_express(SQE_ATTR)
  { elemnum=elemnumIn;  kont=kontIn;  indexexpr=NULL;
    if (kont!=NULL && elemnum!=-1)
      get_kontext_info(elemnumIn, kontIn);
    else if (nameIn) strcpy(name, nameIn);
  }
  t_expr_attr(t_expr_types sqeIn) : t_express(sqeIn)  // called with SQE_POINTED_ATTR
    { elemnum=-1;  kont=NULL;  indexexpr=NULL; }
  virtual ~t_expr_attr(void)
    { delete indexexpr; }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (indexexpr) indexexpr->mark_core();
  }
  virtual void is_referenced(void)
  { if (indexexpr) indexexpr->is_referenced();
    if (kont!=NULL && elemnum!=-1)
      ::is_referenced(kont, elemnum);
  }
};

struct t_expr_pointed_attr : t_expr_attr
{ t_expr_attr * pointer_attr;  // owning
  ttablenum desttb;
  db_kontext dest_table_kont;
  t_expr_pointed_attr(t_expr_attr * pointer_attrIn) : t_expr_attr(SQE_POINTED_ATTR)
    { pointer_attr=pointer_attrIn; }
  ~t_expr_pointed_attr(void)
    { delete pointer_attr; }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (pointer_attr) pointer_attr->mark_core();
    dest_table_kont.mark_core();
  }
  virtual void is_referenced(void)
  { if (pointer_attr) pointer_attr->is_referenced();
  }
};

struct t_expr_dynpar : t_express
{ int num;
  int origtype; 
  t_specif origspecif;
  t_expr_dynpar(void) : t_express(SQE_DYNPAR) { }
};

struct t_expr_embed : t_express
{ tname name;  // the only place tname is used on server
  t_expr_embed(void) : t_express(SQE_EMBED) { }
};

struct t_kontpos  // position in a kontext
{ db_kontext * kont;
  trecnum position;
};

struct t_expr_subquery : t_express
{ t_query_expr * qe;
  t_optim * opt;
  row_distinctor * rowdist;
 // result caching:
  bool cachable;
  uns64 cached_value_stmt_counter;  // undefined for cached_value_client==-1
  uns64 cached_value_cursor_inst_num;
  int cached_value_client;          // -1 if there is no value in the cache
  int cache_size;
  t_kontpos * kontpos;
  t_value val;
 // protection against reentrancy (necessary in default values, check conditions):
  CRITICAL_SECTION cs;
 // profile:
  trecnum used_count, evaluated_count;

  t_expr_subquery(void) : t_express(SQE_SUBQ)
  { qe=NULL;  opt=NULL;  rowdist=NULL; 
    cachable=true;  cached_value_client=-1;  cache_size=0;  kontpos=NULL;
    used_count=evaluated_count=0;
    InitializeCriticalSection(&cs);
  }
  virtual ~t_expr_subquery(void);  // cannot be inline, type incomplete
  void lock(void)
  {
    EnterCriticalSection(&cs);
  }
  void unlock(void)
  {
    LeaveCriticalSection(&cs);
  }
  
  bool same_position(void) const
  { for (int i=0;  i<cache_size;  i++)
      if (kontpos[i].kont->position != kontpos[i].position) return false;
    return true;
  }
  void save_position(void)
  { for (int i=0;  i<cache_size;  i++)
      kontpos[i].position=kontpos[i].kont->position;
  }
  bool have_value_in_cache(cdp_t cdp) const;
  void value_saved_to_cache(cdp_t cdp);
  inline void no_value_in_cache(void)
    { cached_value_client=-1; }
  virtual void close_data(cdp_t cdp);
  virtual void mark_core(void);
  virtual void mark_disk_space(cdp_t cdp) const;
};

class subquery_locker
{ t_expr_subquery * subq;
 public:
   subquery_locker(t_expr_subquery * subqIn) : subq(subqIn)
     { subq->lock(); }
   ~subquery_locker(void)
     { subq->unlock(); }
};

struct t_expr_null : t_express
{ t_expr_null(void) : t_express(SQE_NULL)
    { type=0; }
};

struct t_expr_mult_count : t_express
{ t_expr_attr * attrex;  // not-indexed multiatribute
  t_expr_mult_count(t_expr_attr * attrexIn) : t_express(SQE_MULT_COUNT)
    { attrex=attrexIn;  type=ATT_INT32;   specif.set_num(0, 9); }
  virtual ~t_expr_mult_count(void)
    { delete attrex; }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (attrex) attrex->mark_core(); 
  }
  virtual void is_referenced(void)
    { if (attrex) attrex->is_referenced(); }
};

struct t_expr_var_len : t_express
{ t_expr_attr * attrex;  // AX: heap-type
  t_expr_var_len(t_expr_attr * attrexIn) : t_express(SQE_VAR_LEN)
    { attrex=attrexIn;  type=ATT_INT32;  specif.set_num(0, 9); }
  virtual ~t_expr_var_len(void)
    { delete attrex; }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (attrex) attrex->mark_core(); 
  }
  virtual void is_referenced(void)
    { if (attrex) attrex->is_referenced(); }
};

struct t_expr_dblind : t_express
{ t_expr_attr * attrex;
  t_express * start, * size;
  t_expr_dblind(t_expr_attr * attrexIn, t_express * startIn) : t_express(SQE_DBLIND)
  { attrex=attrexIn;  start=startIn;  size=NULL;
    type   = attrex->type;    // will be converted to string or binary for constant size
    // must not use origtype because it may have the multiattribute flag, which has been removed by index
    specif = attrex->specif;  
  }
  virtual ~t_expr_dblind(void)
    { delete attrex;  delete start;  delete size; }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (attrex) attrex->mark_core(); 
    if (start)  start ->mark_core(); 
    if (size)   size  ->mark_core(); 
  }
  virtual void is_referenced(void)
  { if (attrex) attrex->is_referenced();
    if (start)  start->is_referenced();
    if (size)   size->is_referenced();
  }
};

struct t_expr_cast : t_express
{ t_express * arg;
  t_expr_cast(void) : t_express(SQE_CAST)
    { arg=NULL; }
  virtual ~t_expr_cast(void)
    { delete arg; }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (arg) arg->mark_core(); 
  }
  virtual void is_referenced(void)
    { if (arg) arg->is_referenced(); }
  virtual void close_data(cdp_t cdp)
    { if (arg) arg->close_data(cdp); }
};

struct t_expr_var : t_express
{ int level, offset;
  int origtype;
  t_specif origspecif;
  t_expr_var(int typeIn, t_specif specifIn, int levelIn, int offsetIn) : t_express(SQE_VAR)
    { type=origtype=typeIn;  specif=origspecif=specifIn;  level=levelIn;  offset=offsetIn; }
  virtual ~t_expr_var(void)
    { }
};


typedef enum { CASE_NULLIF, CASE_COALESCE, CASE_SMPL1, CASE_SMPL2,
               CASE_SRCH } t_case_type;

struct t_expr_case : t_express
{ t_compoper oper;  // comparison operator for NULLIF, SMPL
  t_case_type case_type;
  t_express * val1, * val2, * contin;
  t_expr_case(t_case_type case_typeIn) : t_express(SQE_CASE)
    { case_type=case_typeIn;  val1=val2=contin=NULL; }
  virtual ~t_expr_case(void)
   { if (val1!=NULL) delete val1;  if (val2!=NULL) delete val2;
     if (contin!=NULL) delete contin;
   }
  virtual void mark_core(void)
  { t_express::mark_core();
    if (val1)   val1  ->mark_core(); 
    if (val2)   val2  ->mark_core(); 
    if (contin) contin->mark_core(); 
  }
  virtual void is_referenced(void)
  { if (val1)   val1  ->is_referenced(); 
    if (val2)   val2  ->is_referenced(); 
    if (contin) contin->is_referenced(); 
  }
  virtual void close_data(cdp_t cdp)
  { if (val1)   val1  ->close_data(cdp); 
    if (val2)   val2  ->close_data(cdp); 
    if (contin) contin->close_data(cdp); 
  }
};
////////////////////////////// grouping /////////////////////////////////////
struct t_agr_reg
{ int         ag_type;  // S_SUM etc.
  BOOL        distinct;
  t_express * arg;      // NULL for COUNT(*)
 // derived info:
  int         argsize;  // arg value size, extracted from arg for faster access
  int         ressize;  // res value size, >argsize for SUM(ATT_INT16, ATT_INT8)
  int         restype;  // ATT_INT32 for SUM(ATT_INT16, ATT_INT8), argument type otherwise
 // obtaining the value from an index:
  int         index_number;  // -1 iff cannot obtain from index
  BOOL        first_value;   // defined only when [index_number]>=0, take the last value iff FALSE
 // run-time index, used if distinct==TRUE
  dd_index  * indx;
};

SPECIF_DYNAR(t_agr_dynar, t_agr_reg, 2);

enum t_grouping_type { GRP_NO, GRP_SINGLE, GRP_BY, GRP_HAVING };

struct row_distinctor
{ dd_index_sep rowdist_index;
  int rowdist_limit;
  tptr rowkey;
  BOOL orig_unique;
  int flag_offset;
  BOOL was_null;
  trecnum found_distinct_records;
  const bool ignore_null_rows;

  void reset_index(cdp_t cdp);
  void close_index(cdp_t cdp);
  BOOL add_to_rowdist(cdp_t cdp);
  BOOL prepare_rowdist_index(t_query_expr * qe, BOOL unique, int rec_count,
                             BOOL add_flag);
  void remove_record(cdp_t cdp);
  void confirm_record(cdp_t cdp);

  BOOL is_unique(void)
  { return rowdist_index.ccateg==INDEX_UNIQUE; }

  row_distinctor(bool ignore_null_rowsIn = false) : ignore_null_rows(ignore_null_rowsIn)
  { rowkey=NULL; }
  ~row_distinctor(void)
  { corefree(rowkey); }
  void mark_core(void)  // not marking itself!!
   { if (rowkey) mark(rowkey);
     rowdist_index.mark_core();
   }
  void mark_disk_space(cdp_t cdp) const
    { mark_index(cdp, &rowdist_index); }
  void *operator new(size_t size)
    { return corealloc(size, 202); }
  void operator delete(void * ptr)
    { corefree(ptr); }
};
/////////////////////////////////////////////////////////////////////////////
struct t_assgn
{ t_express * dest;     // destination of the assignment
  t_express * source;   // assigned value, NULL for assignment of the DEFAULT value
  void mark_core(void)
  { dest->mark_core();
    if (source) source->mark_core(); 
  }
};

SPECIF_DYNAR(t_assgn_dynar, t_assgn, 3);

BOOL execute_assignment(cdp_t cdp, const t_express * dest, t_express * source, BOOL check_wr_right, BOOL check_rd_right, BOOL protected_execution);
void delete_assgns(t_assgn_dynar * assgns);

#define COLVAL_OFFSET_NULL    -1
#define COLVAL_OFFSET_DEFAULT -2

struct t_vector_descr  // descriptor of a vector of values
{ const char * const vals;  // direct values, may be NULL if expressions or pointers used instead
  const t_colval * colvals; // list of column values, must not be NULL
  BOOL hide_exprs;          // use offsets instead of expressions in colvals
  int sel_table_number;     // selects a subset of colvals[] with the same table_number
  bool  setsize;
  bool  delete_record;      // deleted the record immediately after inserting (prevents cnoflicts on unique indicies)
  t_vector_descr(const t_colval * colvalsIn, const void * valsIn = NULL) : vals((const char *)valsIn)
    { colvals = colvalsIn;  sel_table_number=0;  hide_exprs=FALSE;  setsize=true;  delete_record=false; }
  inline void get_column_value(cdp_t cdp, const t_colval * colval, table_descr * tbdf, const attribdef * att,
    const char * & src, unsigned & size, unsigned & offset, t_value & val) const;
  void release_colval_exprs(void);
  void mark_core(void);     // does not mark itself
};

////////////////////////////// query expression /////////////////////////////
struct t_order_by
{ dd_index_sep order_indx;
  const db_kontext * order_tabkont;
  int          order_indnum;
  BOOL         post_sort;  // post sorting necessary

  t_order_by(void) { post_sort=TRUE; }
  void mark_disk_space(cdp_t cdp)
    { mark_index(cdp, &order_indx); }
};

typedef enum { QE_TABLE,
  QE_JT_INNER, QE_JT_LEFT, QE_JT_RIGHT, QE_JT_FULL, QE_JT_UNION,
  QE_SPECIF,   QE_UNION,   QE_EXCEPT,   QE_INTERS,
  QE_IDENT,    QE_SYSVIEW } t_qe_types;

enum t_maters { MAT_NO,    // table or safe operation on tables
                MAT_HERE,  // t_query_specif with grouping
                MAT_HERE2, // grouping covered by t_query_specifs only
                MAT_BELOW, // materialization in bin oper or 1st join operand
                MAT_INSTABLE, // materialization in non-1st join operand
                MAT_INSENSITIVE }; // INSENSITIVE CURSOR specified
// Describes the materialization status.

struct t_query_expr
{ const t_qe_types qe_type;
  int ref_cnt;
  db_kontext kont;
  int qe_oper;

 // description of interval in ptrs_mater:
  int rec_extent, // max. number of recnums allocated for the operands
      kont_start, // position of the 1st recnum of the operands
      mat_extent; // rec_extent of the materialisation tree below, 0 if not in the mater. root
  t_order_by * order_by;
  t_maters maters; // materialisation status
  bool lock_rec;
  t_express * limit_offset, * limit_count;
 // profile:
  trecnum limit_result_count;

  t_query_expr(t_qe_types qe_typeIn) : qe_type(qe_typeIn)  
  { order_by=NULL;  maters=MAT_NO;  lock_rec=false;  limit_offset=limit_count=NULL;
    ref_cnt=1;  limit_result_count=NORECNUM;
  }
  virtual ~t_query_expr(void);

 // run_time:
  void write_kont_list(int & kont_cnt, BOOL is_top);
  void get_table_numbers(ttablenum * tabnums, int & count);
  BOOL locking(cdp_t cdp, const trecnum * recnums, int opc, int & limit);
  void set_outer_null_position(void);
  bool lock_unlock_tabdescrs(cdp_t cdp, bool locking);
#if SQL3
  int  get_pmat_extent(void);
  void get_position_indep(trecnum * recnums, int & ordnum, bool pmap_down = false);
  void set_position_indep(cdp_t cdp, const trecnum * recnums, int & ordnum);
  void get_extent_indep(int & ordnum);
#else
  void set_position(cdp_t cdp, const trecnum * recnums);
  void get_position(trecnum * recnums);
  void set_position_0(cdp_t cdp, trecnum * recnums)
    { set_position(cdp, mat_extent ? recnums : recnums-kont_start); }
  void get_position_0(trecnum * recnums)
    { get_position(mat_extent ? recnums : recnums-kont_start); }
#endif

  void AddRef(void)  { ref_cnt++; }
  void Release(void) { if (--ref_cnt<=0) delete this; }
  virtual void mark_core(void)  // marks itself, too
  { mark(this);
    kont.mark_core();
    if (order_by) { mark(order_by);  order_by->order_indx.mark_core(); }
    if (limit_offset) limit_offset->mark_core();
    if (limit_count ) limit_count ->mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
    { if (order_by) order_by->mark_disk_space(cdp); }
  void *operator new(size_t size)
    { return corealloc(size, 203); }
  void operator delete(void * ptr)
    { corefree(ptr); }
};

struct t_qe_bin : t_query_expr   // binary query expression (union, except, intersect)
{ t_query_expr * op1, * op2;
  BOOL all_spec,  // ALL specified, set by read_all_and_corresp()
       corr_spec; // CORRESPONDING specified, set by read_all_and_corresp()
  void read_all_and_corresp(CIp_t CI);
  void process_corresp(CIp_t CI, bool is_union);
  t_qe_bin(t_qe_types qe_typeIn) : t_query_expr(qe_typeIn)
    { op1=op2=NULL; }
  virtual ~t_qe_bin(void)
    { if (op1!=NULL) op1->Release();  if (op2!=NULL) op2->Release(); }
  virtual void mark_core(void)
  { t_query_expr::mark_core();
    if (op1) op1->mark_core();
    if (op2) op2->mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_query_expr::mark_disk_space(cdp);
    if (op1) op1->mark_disk_space(cdp);
    if (op2) op2->mark_disk_space(cdp);
  }
};

struct t_qe_specif : t_query_expr  // direct specification, no expression
{ bool user_order, distinct;
  t_query_expr * from_tables;
  t_express * where_expr;
  t_grouping_type grouping_type;
  dd_index_sep group_indx;
  t_express * having_expr;
  t_agr_dynar agr_list;    // list of aggregate functions used
  t_assgn_dynar into;      // list of INTO assignments

  t_qe_specif(void) : t_query_expr(QE_SPECIF)
  { where_expr=having_expr=NULL;  from_tables=NULL;
    grouping_type=GRP_NO;
  }
  virtual ~t_qe_specif(void)
  { if (where_expr !=NULL) delete where_expr;
    if (having_expr!=NULL) delete having_expr;
    if (from_tables!=NULL) from_tables->Release();
    delete_assgns(&into);
  }
  virtual void mark_core(void)
  { t_query_expr::mark_core();
    if (from_tables) from_tables->mark_core();
    if (where_expr)  where_expr ->mark_core();
    group_indx.mark_core();
    if (having_expr) having_expr->mark_core();
    agr_list.mark_core();
    for (int i=0;  i<into.count();  i++)
      into.acc0(i)->mark_core();
    into.mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_query_expr::mark_disk_space(cdp);
    if (from_tables) from_tables->mark_disk_space(cdp);
    mark_index(cdp, &group_indx); 
  }

  BOOL assign_into(cdp_t cdp);
};

struct t_qe_join : t_query_expr    // joined tables
{ t_query_expr *op1, *op2;
  t_express * join_cond;
  t_qe_join(t_qe_types qe_typeIn) : t_query_expr(qe_typeIn)
    { op1=op2=NULL;  join_cond=NULL; }
  virtual ~t_qe_join(void)
  { if (op1!=NULL) op1->Release();  if (op2!=NULL) op2->Release();
    if (join_cond!=NULL) delete join_cond;
  }
  virtual void mark_core(void)
  { t_query_expr::mark_core();
    if (op1) op1->mark_core();
    if (op2) op2->mark_core();
    if (join_cond) join_cond->mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_query_expr::mark_disk_space(cdp);
    if (op1) op1->mark_disk_space(cdp);
    if (op2) op2->mark_disk_space(cdp);
  }
};

struct t_qe_ident : t_query_expr   // sorted top
{ t_query_expr *op;
  t_qe_ident(t_qe_types qe_typeIn) : t_query_expr(qe_typeIn)
    { op=NULL; }
  virtual ~t_qe_ident(void)
    { if (op!=NULL) op->Release(); }
  virtual void mark_core(void)
  { t_query_expr::mark_core();
    if (op) op->mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_query_expr::mark_disk_space(cdp);
    if (op) op->mark_disk_space(cdp);
  }
};

struct t_qe_table : t_query_expr    // a table
{ ttablenum tabnum;
  bool is_local_table;
  table_descr * tbdf_holder;  // used only when the struct is in an open cursor
  int pref_index_num;   // -1 or the number of the preferred index
  trecnum extent;  // allocated space for records, defined when t_qe_table is created
  t_qe_table(ttablenum tabnumIn) : t_query_expr(QE_TABLE), tabnum(tabnumIn)
  { pref_index_num=-1;
    qe_oper=QE_IS_UPDATABLE|QE_IS_DELETABLE|QE_IS_INSERTABLE;
    tbdf_holder=NULL;
    is_local_table=false;
  }
  virtual ~t_qe_table(void) { }
};

#define MAX_INFOVIEW_PARAMS 2

struct t_qe_sysview : t_query_expr    // a system view
{ unsigned sysview_num;
  t_express * args[MAX_INFOVIEW_PARAMS];
  t_qe_sysview(unsigned sysview_numIn) : t_query_expr(QE_SYSVIEW)
  { sysview_num=sysview_numIn;
    for (int i=0;  i<MAX_INFOVIEW_PARAMS;  i++) args[i]=NULL;
    qe_oper=0; // not updatable, deletable, insertable
  }
  virtual ~t_qe_sysview(void) 
  { for (int i=0;  i<MAX_INFOVIEW_PARAMS;  i++)
      if (args[1]) delete args[i];
  }
  virtual void mark_core(void)
  { t_query_expr::mark_core();
    for (int i=0;  i<MAX_INFOVIEW_PARAMS;  i++)
      if (args[1]) args[i]->mark_core();
  }
};
/////////////////////////////// recdist /////////////////////////////////////
class t_recdist  // unique storage for arrays of record numbers
{private:
  int      recnum_cnt; // number of record numbers stored in the distinctior
  dd_index_sep indx;
  trecnum * keybuf;    // space for the full index key, including fictive recnum

 public:
  BOOL define   (cdp_t cdpIn, int recnum_cntIn);
  void clear    (cdp_t cdp);
  BOOL add      (cdp_t cdp, void * key);  // adds a key (unless already contained)
  BOOL contained(cdp_t cdp, void * key);  // searches for the key

  t_recdist(void)
    { keybuf=NULL; }
  ~t_recdist(void)
    { }
  void *operator new(size_t size)
    { return corealloc(size, 204); }
  void operator delete(void * ptr)
    { corefree(ptr); }
  void mark_core(void)  // does not mark itself
  { if (keybuf) mark(keybuf);
    indx.mark_core();
  }
  void mark_disk_space(cdp_t cdp) const
    { mark_index(cdp, &indx); }
};

/////////////////////////////// optimization ////////////////////////////////
struct t_condition
{ t_express * expr;
  BOOL is_relation;  // TRUE for a relation, 2 for and AND/OR tree containing
    // only relations with the same tab/ind/part access on one side
  uns32 use_map1;
  int tabnum1, indnum1, partnum1, valtype1; // indnum1==-1 -> fulltext index
  t_specif valspecif1;
  uns32 use_map2;
  int tabnum2, indnum2, partnum2, valtype2;
  t_specif valspecif2;
  BOOL generated;
};

SPECIF_DYNAR(t_cond_dynar, t_condition, 2);

enum { MAX_OPT_TABLES=32 };

struct t_linkedtab  // may use multiple conditions
{ int leavenum;
};

struct t_joinorder
{ uns32 map;
  unsigned tabcnt;
  double cost;     // estimated cost of creation
  unsigned rcount; // estimated record count
  bool expanded;
  t_linkedtab litab[MAX_OPT_TABLES];
};

SPECIF_DYNAR(t_joinorder_dynar, t_joinorder, 1);

SPECIF_DYNAR(t_expr_dynar, t_express*, 1);

struct t_variant
{ t_cond_dynar conds;
  int leave_cnt; // copy of leave_count
  t_optim * join_steps[MAX_OPT_TABLES];

  void init(unsigned leave_count);
  ~t_variant(void);
  t_condition * store_condition(t_express * ex);
  void mark_core(void);
  void mark_disk_space(cdp_t cdp) const;
};

SPECIF_DYNAR(t_variant_dynar, t_variant, 1);

enum t_optim_type { OPTIM_JOIN_INNER=0, OPTIM_JOIN_OUTER,
  OPTIM_UNION_DIST, OPTIM_UNION_ALL, OPTIM_EXC_INT, OPTIM_TABLE,
  OPTIM_GROUPING, OPTIM_SYSVIEW, OPTIM_NULL 
#if SQL3
  , OPTIM_ORDER, OPTIM_LIMIT
#endif
};
enum t_cond_state { COND_POST, COND_LIM1, COND_LIM2, COND_UNCLASSIF };

struct t_optim
{ const t_optim_type optim_type;
  t_query_expr * own_qe;    // top of the optimized subtree
  int ref_cnt;
  t_expr_dynar inherited_conds;  // conditions inherited from parent nodes
  BOOL eof;
  BOOL error;
  row_distinctor * main_rowdist;
 // order by:
  t_order_by * order_; // points to order_by from (almost) top qe (allocations marker by qe)

  void inherit_expr(cdp_t cdp, t_express * ex); // not used by OPTIM_JOIN_INNER

  virtual int  find_leave_by_kontext(db_kontext * kont, const t_query_expr ** pqe) const;
  virtual void push_condition(cdp_t cdp, t_condition * acond, t_cond_state cond_state);
  // The only way of moving conditions from top to leaves
  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table) = 0;
  virtual void reset(cdp_t cdp) = 0;
          void reset_distinct(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp) = 0;
          BOOL get_next_distinct_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  BOOL (t_optim::*p_get_next_record)(cdp_t cdp);
  void (t_optim::*p_reset          )(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval) = 0;

  void make_distinct(void)
#if 1
  { p_get_next_record=&t_optim::get_next_distinct_record;
    p_reset          =&t_optim::reset_distinct;
#else
  { p_get_next_record= get_next_distinct_record;
    p_reset          = reset_distinct;
#endif
  }
  void make_not_distinct(void)
#if 1
  { p_get_next_record=&t_optim::get_next_record;
    p_reset          =&t_optim::reset;
#else
  { p_get_next_record=get_next_record;
    p_reset          =reset;
#endif
  }

  t_optim(t_optim_type optim_typeIn, t_query_expr * own_qeIn, t_order_by * order_In) 
  : optim_type(optim_typeIn)
  { order_=order_In;  own_qe=own_qeIn;  error=FALSE;
    main_rowdist=NULL;
#if 1
    p_get_next_record=&t_optim::get_next_record;
    p_reset          =&t_optim::reset;
#else
    p_get_next_record= get_next_record;
    p_reset          = reset;
#endif
    ref_cnt=1;
  }
  virtual ~t_optim(void)
    { if (main_rowdist!=NULL) delete main_rowdist; }
  virtual void mark_core(void)  // marks itself, too
  { mark(this);
    if (main_rowdist!=NULL) { main_rowdist->mark_core();  mark(main_rowdist); }
    inherited_conds.mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
    { if (main_rowdist) main_rowdist->mark_disk_space(cdp); }
  void AddRef(void)  { ref_cnt++; }
  void Release(void) { if (--ref_cnt<=0) delete this; }
  void *operator new(size_t size)
    { return corealloc(size, 205); }
  void operator delete(void * ptr)
    { corefree(ptr); }
  void post_reset_action(cdp_t cdp);
};

struct t_optim_null : t_optim
{ int pos;
  t_optim_null(t_query_expr * own_qeIn) : t_optim(OPTIM_NULL, own_qeIn, NULL)
    { }
  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table)
    { }
  virtual void reset(cdp_t cdp)
    { eof=false;  pos=0; }
  virtual BOOL get_next_record(cdp_t cdp)
    { if (pos==0) { pos=1;  return TRUE; }
      else { eof=true;  return FALSE; }
    }  
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval)
    { }
};

#if SQL3
struct t_optim_limit : t_optim  // limit is defined in [own_qe]
{ t_optim * subopt;  // is never NULL
  trecnum limit_count, limit_offset;
  t_optim_limit(t_query_expr * own_qeIn, t_order_by * order_In, t_optim * suboptIn, t_optim_type optim_typeIn = OPTIM_LIMIT) 
    : t_optim(optim_typeIn, own_qeIn, order_In), subopt(suboptIn)
      { }
  virtual ~t_optim_limit(void)
    { subopt->Release(); }
  virtual void mark_core(void)
  { t_optim::mark_core();
    subopt->mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_optim::mark_disk_space(cdp);
    subopt->mark_disk_space(cdp);
  }
  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
};

struct t_optim_order : t_optim_limit   // sorting and limit are defined in [own_qe]
{ bool sort_disabled;
  t_optim_order(t_query_expr * own_qeIn, t_order_by * order_In, t_optim * suboptIn) 
    : t_optim_limit(own_qeIn, order_In, suboptIn, OPTIM_ORDER)
      { sort_disabled=false; }
  virtual ~t_optim_order(void)
    { }
  virtual void mark_core(void)
  { t_optim::mark_core();
    subopt->mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_optim::mark_disk_space(cdp);
    subopt->mark_disk_space(cdp);
  }
  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
};

#endif

struct t_optim_grouping : t_optim
{ t_qe_specif * qeg;
  t_optim * opopt;
#if SQL3
  trecnum position_for_next_record;     // position for get_next_record must be different than the last read position, otherwise reading in imcomplete cursor may damage is creation
  t_expr_dynar inherited_having_conds;  // conditions inherited from parent without outer references but using aggregates
#endif
 // profiling:
  trecnum having_result, output_records;

  t_optim_grouping(t_qe_specif * qegIn, t_order_by * order_In) 
  : t_optim(OPTIM_GROUPING, qegIn, order_In)
    { qeg=qegIn;  opopt=NULL;  having_result=output_records=0; }
  virtual ~t_optim_grouping(void)
    { if (opopt) opopt->Release(); }
  virtual void mark_core(void)
  { t_optim::mark_core();
    if (opopt) opopt->mark_core();
#if SQL3
    inherited_having_conds.mark_core();
#endif
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_optim::mark_disk_space(cdp);
    if (opopt) opopt->mark_disk_space(cdp);
  }
  trecnum group_count;  // number of records in the materialized table
  void repass(void);
  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
};

struct t_optim_bin : t_optim
{ t_qe_bin * top;
  t_optim * op1opt, * op2opt;

  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  t_optim_bin(t_optim_type optim_typeIn, t_qe_bin * topIn, t_order_by * order_In) 
  : t_optim(optim_typeIn, topIn, order_In)
    { top=topIn;  op1opt=op2opt=NULL; }
  virtual void mark_core(void)
  { t_optim::mark_core();
    if (op1opt) op1opt->mark_core();
    if (op2opt) op2opt->mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_optim::mark_disk_space(cdp);
    if (op1opt) op1opt->mark_disk_space(cdp);
    if (op2opt) op2opt->mark_disk_space(cdp);
  }
};

struct t_optim_union_dist : t_optim_bin
{ row_distinctor rowdist;
  trecnum position_for_next_record;  // position for get_next_record must be different than the last read position, otherwise reading in imcomplete cursor may damage is creation

  t_optim_union_dist(t_qe_bin * topIn, t_order_by * order_In) :
    t_optim_bin(OPTIM_UNION_DIST, topIn, order_In), rowdist()
      { }
  virtual ~t_optim_union_dist(void)
    { if (op1opt!=NULL) op1opt->Release();  if (op2opt!=NULL) op2opt->Release(); }

  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
  virtual void mark_core(void)
  { t_optim_bin::mark_core();
    rowdist.mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_optim_bin::mark_disk_space(cdp);
    rowdist.mark_disk_space(cdp); 
  }
};

struct t_optim_union_all : t_optim_bin
{ 
  trecnum position_for_next_record;  // position for get_next_record must be different than the last read position, otherwise reading in imcomplete cursor may damage is creation
  t_optim_union_all(t_qe_bin * topIn, t_order_by * order_In) :
    t_optim_bin(OPTIM_UNION_ALL, topIn, order_In)
      { }
  virtual ~t_optim_union_all(void)
    { if (op1opt!=NULL) op1opt->Release();  if (op2opt!=NULL) op2opt->Release(); }

  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
};

struct t_optim_exc_int : t_optim_bin
{ row_distinctor rowdist;
  t_btree_acc bac;  // passing the values in the index
  const t_qe_types qe_type;

  t_optim_exc_int(t_qe_bin * topIn, t_order_by * order_In) :
    t_optim_bin(OPTIM_EXC_INT, topIn, order_In), rowdist(), qe_type(topIn->qe_type)
      { }
  virtual ~t_optim_exc_int(void)
  { if (op1opt!=NULL) op1opt->Release();  if (op2opt!=NULL) op2opt->Release(); 
    if (bac.fcbn!=NOFCBNUM) data_not_closed();  // bac.close(SaferGetCurrTaskPtr);
  }

  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
  virtual void mark_core(void)
  { t_optim_bin::mark_core();
    rowdist.mark_core();
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_optim_bin::mark_disk_space(cdp);
    rowdist.mark_disk_space(cdp); 
  }
};
////////////////////////////////// table /////////////////////////////////////
class t_hashed_recordset 
{ unsigned table_size;
  char * table;  // NORECNUM is an empty cell in the table
  unsigned contains;
  unsigned limit;
  unsigned itemsize;  // sizeof(trecnum)+bitmapsize
  BOOL expand(void);
  inline unsigned hash(trecnum recnum)
    { return (13*recnum) % table_size; }
  unsigned passing_position;  // the next position to be analysed and returned
  int completed_position;  // bit position in the map of the completely passed index
  int fulltext_position;   // bit position in the map of the fulltext index, -1 if it does not exist
 public:
  inline void close_data(void)
  { if (table) { corefree(table);  table=NULL; }
    table_size=contains=limit=0;
  }
  inline t_hashed_recordset(void)
    { table_size=contains=limit=0;  table=NULL;  fulltext_position=-1; }
  inline ~t_hashed_recordset(void)
    { close_data(); }
  inline void set_mapsize(unsigned normal_indexcount, bool has_ft_index) // Must be called before using the class (indexcount>0)
  // Requirement: indexcount+(ft)<=32
  { if (has_ft_index) 
      { fulltext_position=normal_indexcount;  normal_indexcount++; }
    itemsize = sizeof(trecnum) + (normal_indexcount-1)/8*8+1; 
    if (has_ft_index) itemsize+=2*sizeof(uns32);
  }
  BOOL add_record(trecnum recnum, int position, uns32 ft_weight = 0, uns32 ft_pos = 0);
  inline void records_end(int position)
    { completed_position=position;  passing_position=0; }
  trecnum get_next(cdp_t cdp, uns32 & map);  // returns NORECNUM if no more records available
  void mark_core(void)
    { if (table) mark(table); }
};

struct t_table_index_part_conds // indexed by part number
{ t_expr_dynar pconds;       // not changed after optimize()
  t_interval * interv_list;  // created in reset(), updated during passing, deleted in close_data()
  inline t_table_index_part_conds(void)
    { interv_list=NULL; }
  void delete_interv_list(void);
  inline ~t_table_index_part_conds(void)
    { delete_interv_list(); }
  void mark_core(void);
};

struct t_table_index_conds
{// optimization-defined members:
  int index_number;   // number of the index in the table
  t_table_index_part_conds * part_conds;
  int partcnt;
 // run-time modified members:
  tptr startkey, stopkey;     // related to the current interval
  int open_start, open_stop;  // related to the last part and current interval
  dd_index * indx;  // valid only between reset() and close_data() when table is installed, not owning
  t_btree_acc bac;
  t_btree_read_lock btrl;
  bool continuing_same_interval;  // true when reopenning closed interval passing and the current record must be skipped
 // optimization methods:
  t_table_index_conds(void)
    { part_conds=NULL;  indx=NULL;  startkey=stopkey=NULL; }
  ~t_table_index_conds(void);
  void mark_core(void);
  BOOL init1(const table_descr * td, int indexnum);
  BOOL init2(const table_descr * td);
 // run-time methods:
  void compute_interval_lists(cdp_t cdp, db_kontext * tabkont);
  BOOL go_to_next_interval(void);
  BOOL interval2keys(cdp_t cdp);
  BOOL open_index_passing(cdp_t cdp);
  void close_index_passing(cdp_t cdp);
  trecnum next_record_from_index(cdp_t cdp);
  void close_data(cdp_t cdp);
  void close_data_with_warning(cdp_t cdp);
};

struct t_table_cond_info
{ int is_in_or;
  BOOL unused_part;
  int leading_index, leading_part;
  inline t_table_cond_info(void)
    { is_in_or = 0;  unused_part=FALSE;  leading_index=-1; }
};

class t_reclist  // list of records from a sliding window on an index
{protected: 
  enum { listsize=100 };   // smaller number helps to optimize the fulltext index usage
  trecnum list[listsize];
  unsigned count;  // valid records in the [list], defined by fill()
  bool index_exhausted;  // no more records can be obtained from the index (but [list] may contain not passed records)
  int passing_pos; // passing position in the [list]
 public:
  void restart(void)
    { index_exhausted=0; }
  bool fill(cdp_t cdp, t_table_index_conds * tic, ttablenum tabnum, bool index_passing_just_opened);
  trecnum get_record(void)  // return the next record from the [list] and advances the passing position, returns NORECNUM is [list] is exhausted
    { return passing_pos<count ? list[passing_pos++] : NORECNUM; }
};

struct t_optim_table;

class t_reclist_ft : public t_reclist  // list of records from a sliding window on an index
{ uns32 ft_weight[listsize];
  unsigned ft_pos[listsize];
 public:
  bool fill_ft(cdp_t cdp, t_optim_table * optt, bool index_passing_just_opened);
  trecnum get_record(uns32 & weight, unsigned & pos)  // return the next record from the [list] and advances the passing position, returns NORECNUM is [list] is exhausted
  { if (passing_pos<count) 
      { weight=ft_weight[passing_pos];  pos=ft_pos[passing_pos];  return list[passing_pos++]; }
    return NORECNUM; 
  }
};

struct t_optim_table : t_optim
{ t_qe_table * qet;
  t_expr_dynar post_conds;
  t_table_index_conds * tics;  // allocated for all indexes but it has [normal_index_count] initialized items
  int normal_index_count; // number of valid items in the tics[] array
 // passing all records in the table:
  trecnum exhaustive_passing_limit, exhaustive_passing_position;
 // passing using index and intervals:
  table_descr * td; // valid only between reset() and close_data() when table is installed, not owning
  bool table_read_locked;  // for fulltext index passing only
 // passing using fulltext index:
  t_expr_dynar fulltext_conds;
  t_ft_kontext * fulltext_kontext;  // must exist longer than fulltext_expr!
  t_ft_node * fulltext_expr;        
  int docid_index;                  // -1 iff not using fulltext condition
  bool locking;
  bool check_local_ref_privils;
  t_atrmap_flex ref_col_map;  // bitmap of refenced columns without global privileges, valid iff [check_local_ref_privils], init in can_reference_columns()

  BOOL precomputed;           // TRUE iff records moved to [recset] in reset()
  t_hashed_recordset recset;  // hashed table of records from all indicies
  t_reclist * reclist;
 // profile:
  trecnum reset_count, records_taken, post_passed;

  t_optim_table(t_qe_table * qetIn, t_order_by * order_In);
  virtual ~t_optim_table(void);

  virtual int  find_leave_by_kontext(db_kontext * kont, const t_query_expr ** pqe) const;
  virtual void push_condition(cdp_t cdp, t_condition * acond, t_cond_state cond_state);
  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
  virtual void mark_core(void);
  BOOL process_table_condition(cdp_t cdp, t_express * ex, t_table_cond_info * tci);
  void add_index_expression(const table_descr * td, t_express * ex, int indexnum, int partnum);
  void add_distinct_cond(t_express * ex);
  trecnum get_next_fulltext_rec(cdp_t cdp);
};

struct t_optim_sysview : t_optim
{ t_qe_sysview * qe;
 // conditions pushed from upper level:
  t_expr_dynar     post_conds;
  trecnum limit;   // number of records in the materialized table

  t_optim_sysview(t_qe_sysview * qeIn, t_order_by * order_In) : t_optim(OPTIM_SYSVIEW, qeIn, order_In)
    { qe=qeIn; }
  virtual ~t_optim_sysview(void) { }

  virtual int  find_leave_by_kontext(db_kontext * kont, const t_query_expr ** pqe) const;
  virtual void push_condition(cdp_t cdp, t_condition * acond, t_cond_state cond_state);
  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
  virtual void mark_core(void)
  { t_optim::mark_core();
    post_conds.mark_core();
  }
};

struct t_optim_join : t_optim
{ t_qe_types join_qe_type;
  int leave_count;
  t_query_expr * leaves[MAX_OPT_TABLES];
  bool user_order;

  t_optim_join(t_optim_type optim_typeIn, t_query_expr * qe, t_order_by * order_In) 
  : t_optim(optim_typeIn, qe, order_In)
    { leave_count=0;  join_qe_type=qe->qe_type;  user_order=false; }

  int  find_leave(t_query_expr * qe);
  void add_condition_ex(cdp_t cdp, t_express * ex, t_variant * variant);
#ifdef OLD_OR_OPTIMIZATION
  void copy_condition(cdp_t cdp, t_variant * variant, variant_dispatcher * vardisp, t_express * ex);
#else
  void copy_condition(cdp_t cdp, t_variant * variant, t_express * ex);
#endif
  void best_join_order(t_variant * variant, BOOL may_define_ordering,
                                            t_joinorder * jo_out);
  bool enlarge_join_order(t_variant * variant, const t_joinorder * jo_patt, t_joinorder_dynar * jod, bool orderdef_table);
  void distribute_conditions(cdp_t cdp, t_joinorder * jo, t_variant * variant, BOOL may_define_ordering);
//  BOOL same_expr(t_express * ex1, t_express * ex2);

  int gnr_pos;
  virtual int  find_leave_by_kontext(db_kontext * kont, const t_query_expr ** pqe) const;
};

struct t_optim_join_inner : t_optim_join
{ t_variant_dynar variants;
  t_recdist * variant_recdist; // storage for records generated in varinats
  t_optim_join_inner(t_query_expr * qe, t_order_by * order_In, bool user_orderIn = false) :
    t_optim_join(OPTIM_JOIN_INNER, qe, order_In)
      { variant_recdist=NULL;  user_order=user_orderIn; }
  virtual ~t_optim_join_inner(void);

#if SQL3
  void find_subtree_leaves(t_query_expr * qe);
#else
  void find_subtree_leaves(t_query_expr * qe, bool top);
#endif
  void add_condition_qe(cdp_t cdp, t_query_expr * qe);
  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);
  void optimize_variant(cdp_t cdp, t_variant * variant, BOOL may_define_ordering);

 // passing records in variants:
  int curr_variant_num;
  t_variant * curr_variant;
  void variant_reset(cdp_t cdp);
  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
  virtual void mark_core(void)
  { t_optim::mark_core();
    variants.mark_core();
    for (int i=0;  i<variants.count();  i++)
      variants.acc0(i)->mark_core();
    if (variant_recdist) { variant_recdist->mark_core();  mark(variant_recdist); }
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_optim_join::mark_disk_space(cdp);
    for (int i=0;  i<variants.count();  i++)
      variants.acc0(i)->mark_disk_space(cdp);
    if (variant_recdist) variant_recdist->mark_disk_space(cdp); 
  }
};

struct t_optim_join_outer : t_optim_join
{ t_variant variant;
  t_qe_join * ojtop;
  BOOL left_outer, right_outer;   // for FULL JOIN are both TRUE
  BOOL pair_generated;     // any pair for the left operand generated
  uns32 conds_outer;       // map of inherited conditions to be evaluated outside
 // right outer join data:
  BOOL right_post_phase;   // generating <NULL,op2> records, passing rightopt
  t_query_expr * oper1, * oper2;  // left and right OJ operands
  t_recdist right_storage; // storage for records of the second operand used (if oper2->maters==MAT_NO)
  t_optim * rightopt;      // optim for the right operand without join condition
  row_distinctor * roj_rowdist; // storage for rows    of the second operand used (if oper2->maters!=MAT_NO)

  virtual void optimize(cdp_t cdp, BOOL ordering_defining_table);

  t_optim_join_outer(t_qe_join * qe, t_order_by * order_In, bool user_orderIn = false) :
  // qe is outer join new, but may be replaced by specif later (subtree top)
    t_optim_join(OPTIM_JOIN_OUTER, qe, order_In)
    { rightopt=NULL;  ojtop=qe;  roj_rowdist=NULL;  user_order=user_orderIn;
      variant.init(2);
    }
  virtual ~t_optim_join_outer(void)
  { if (rightopt!=NULL) rightopt->Release(); 
    if (roj_rowdist!=NULL) delete roj_rowdist;
  }
  virtual void mark_core(void)
  { t_optim::mark_core();
    variant.mark_core();
    right_storage.mark_core();
    if (rightopt) rightopt->mark_core();
    if (roj_rowdist) { roj_rowdist->mark_core();  mark(roj_rowdist); }
  }
  virtual void mark_disk_space(cdp_t cdp) const
  { t_optim_join::mark_disk_space(cdp);
    variant.mark_disk_space(cdp);
    right_storage.mark_disk_space(cdp); 
    if (rightopt) rightopt->mark_disk_space(cdp);
    if (roj_rowdist) roj_rowdist->mark_disk_space(cdp); 
  }

  virtual void reset(cdp_t cdp);
  virtual BOOL get_next_record(cdp_t cdp);
  virtual void close_data(cdp_t cdp);
  virtual void dump(cdp_t cdp, t_flstr * dmp, bool eval);
};

////////////////////////////// materialization //////////////////////////////
struct t_materialization
{ BOOL is_ptrs;
  BOOL completed;  // if ![completed] the values/pointers in this materialization are ignored and are taken from the lower levels 
//  t_query_expr * * kont_list; // list of kontexts
  t_query_expr * top_qe;  // [top_qe] has this materialisation in its db_kontext
  virtual ~t_materialization(void) { }
  virtual BOOL init(t_query_expr * qe);  // stores [qe] into [top_qe]
  virtual void mark_core(void)   // marks itself, too
    { mark(this); }
  void *operator new(size_t size)
    { return corealloc(size, 206); }
  void operator delete(void * ptr)
    { corefree(ptr); }
#if SQL3
  virtual trecnum record_count(void) = 0;
#endif
};

struct t_mater_table : t_materialization
{ ttablenum tabnum;
  BOOL permanent; // local tables have permanent==TRUE because they are not created here nor destroyed in the destructor of t_mater_table
  int locdecl_level, locdecl_offset;   // locdecl_offset!=-1 only for local tables
  int aggr_base;  // number of the 1st aggregated attribute
  t_atrmap_flex refmap;
#if SQL3
  trecnum _record_count;
#endif
  t_mater_table(cdp_t cdp, BOOL permanentIn, ttablenum tabnumIn = NOTBNUM)
  { is_ptrs=FALSE;  permanent=permanentIn;  tabnum=tabnumIn;
    completed=permanent;
    table_descr_auto td(cdp, tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
    if (td->me())
    { refmap.init(td->attrcnt);
      refmap.clear();
    }
    locdecl_offset=-1;
  }
  virtual ~t_mater_table(void)
  { if (!permanent && tabnum != NOTBNUM)
      data_not_closed();
  }
  virtual BOOL init(cdp_t cdp, t_query_expr * qe);

  trecnum new_group(cdp_t cdp, t_qe_specif * qs);
  void record_in_group(cdp_t cdp, t_qe_specif * qs, trecnum recnum, tptr groupkey);
  void close_group(cdp_t cdp, t_qe_specif * qs, trecnum recnum);
  BOOL min_max_from_index(cdp_t cdp, t_qe_specif * qs, trecnum & recnum, tptr key);
  void is_referenced(int elemnum);
  void destroy_temp_materialisation(cdp_t cdp)
  { if (!permanent && tabnum != NOTBNUM)
    { destroy_temporary_table(cdp, tabnum); 
      tabnum = NOTBNUM;
    }
  }
  virtual void mark_core(void) 
  { t_materialization::mark_core();
    refmap.mark_core(); 
  }
#if SQL3
  virtual trecnum record_count(void)
    { return _record_count; }
#endif
};

struct t_mater_ptrs : t_materialization
{ 
#define PTRIND_BYTES   (BLOCKSIZE) /* axiom: PTRIND_BYTES <= BLOCKSIZE !!! */
#define MAX_DIRECT_SELECTION (PTRIND_BYTES / sizeof(trecnum))
  int kont_cnt;
  trecnum rec_cnt;   // stored items count
  bool    direct;    // data stored directly in selection, not in disc blocks
  t_dynar selection; // contains stored items or disc numbers
 // data valid only when !direct:
  t_independent_block pool_access;
  int     apoolnum,      // number of the fixed block iff any, -1 otherwise
          crecperblock;  // items per disc block

  trecnum * setpool2(cdp_t cdp, unsigned poolnum);

  inline trecnum * setpool(cdp_t cdp, int poolnum)
  // Must call pool_access.data_changed() when writing to the result pointer.
  { if (poolnum==apoolnum) return (trecnum*)pool_access.current_ptr();
    return setpool2(cdp, poolnum);
  }

  trecnum * recnums_space(cdp_t cdp);
  void curs_del(cdp_t cdp, trecnum crec);
  const trecnum * recnums_pos_const(cdp_t cdp, trecnum crec);
  trecnum * recnums_pos_writable(cdp_t cdp, trecnum crec);

  BOOL put_recnums(cdp_t cdp, trecnum * recnums);
  // Writes the position to all the contexts below
  void add_current_pos(cdp_t cdp);
  BOOL cursor_search(cdp_t cdp, trecnum * recnums, trecnum * crec, int cnt);
  
  int cursor_record_state(cdp_t cdp, trecnum rec)
  { const trecnum * recs = recnums_pos_const(cdp, rec);
    if (recs==NULL) return OUT_OF_CURSOR; // NULL returned if rec>=rec_cnt
    return *recs==NORECNUM ? DELETED : NOT_DELETED;
  }

  virtual BOOL init(t_query_expr * qe);

  void clear(cdp_t cdp);

  t_mater_ptrs(cdp_t cdpIn);
  virtual ~t_mater_ptrs(void);
  virtual void mark_core(void) 
  { t_materialization::mark_core();
    selection.mark_core(); 
  }
  void mark_disk_space(cdp_t cdp) const;
#if SQL3
  virtual trecnum record_count(void)
    { return rec_cnt; }
#endif
};
//////////////////////////////// scope //////////////////////////////////////
enum t_handler_type { HND_REDO, HND_UNDO, HND_EXIT, HND_CONTINUE };

enum t_loc_categ
{ LC_LABEL,
  LC_EXCEPTION,
  LC_HANDLER,
  LC_CURSOR,
  LC_TABLE,
  // LC_VAR starts common name space
  LC_VAR, LC_CONST, LC_INPAR, LC_OUTPAR, LC_INOUTPAR,
  LC_FLEX,  // INOUT, flexible size in external DLLs
    LC_PAR,    // not specified
    LC_INPAR_, // not specified, used as input, may be changed to INOUT
    LC_OUTPAR_,// not specified, used as output, may be changed to INOUT
  LC_ROUTINE
  // must not add here new categories which are not in the common name space with variables
};
// LC_LABEL, LC_EXCEPTION, LC_HANDLER, LC_CURSOR, LC_VAR, LC_TABLE are namespace identifiers

class t_routine;

/////////////////////////// structured types ///////////////////////////////
// Used only as a local type of FOR block.

struct t_row_elem
{ tobjname name;  // same size as attribute name
  int      type;
  t_specif specif;
  int      offset;
  BOOL     used;
};

SPECIF_DYNAR(t_row_elem_dynar, t_row_elem, 4);

struct t_struct_type
{ int valsize;
  t_row_elem_dynar elems;
  BOOL fill_from_kontext(db_kontext * kont);
  BOOL fill_from_table_descr(const table_descr * tbdf);
};

struct t_locdecl
{ t_idname name;
  t_loc_categ loccat;
  union 
  { struct
    { union
      { int type;
        t_struct_type * stype;
      };
      uns32 specif;  // t_specif in fact
      int offset;
      t_express * defval;  // partially owning, multiple consecutive vars can own the same expression!!
      bool structured_type;
    } var; // local variable or parameter
    struct
    { t_routine * routine; // owning pointer!
    } rout;
    struct
    { char sqlstate[6];
    } exc;
    struct
    { sql_statement * stmt; // partially owning, multiple consecutive handlers can own the same stmt!!
      t_handler_type rtype;
      tobjnum container_objnum;
    } handler;
    struct
    { t_query_expr * qe; // co-owned 
      t_optim * opt;     // co-owned 
      int offset;        // open cursor number offset in the rscope
      BOOL for_cursor;   // TRUE if created by named FOR statement
    } cursor;
    struct
    { t_routine * rout;  // not owning, defined only for parameters, points to the containing routine
    } label;
    struct
    { table_all * ta;    // owning pointer
      int offset;        // table number offset in the rscope (tbnum must not be here because of posible recursion)
    } table;
  };
  void mark_core(void);
  void store_var(const char * nameIn, int typeIn, t_specif specifIn, int offsetIn, t_express * defvalIn);
};

SPECIF_DYNAR(t_locdecl_dynar, t_locdecl, 3);
BOOL get_table_name(CIp_t CI, char * tabname, char * schemaname, ttablenum * ptabobj,  const t_locdecl ** plocdecl = NULL, int * plevel = NULL);

struct t_scope
// Scope of function parameters starts with wbbool flag - "return performed".
// Scope of function contains return value space (after param values).
{protected:
  long ref_cnt;  // used only by trigger scopes stored in table_descr
  int stored_pos;
  BOOL system;
 public:
  t_locdecl_dynar locdecls;
  t_scope * next_scope;
  int extent;
  BOOL params;  // TRUE for routine parameter list, 2 for trigger outmost scope

  void store_pos(void)
    { stored_pos=locdecls.count(); }
  t_locdecl * add_name(CIp_t CI, const char * name, t_loc_categ loccat);
  void store_type(int type, t_specif specif, BOOL to_constant, t_express * defval);
  void store_statement(t_handler_type rt, sql_statement * stmt);
  void compile_declarations(CIp_t CI, int own_level, sql_statement ** p_init_stmt, BOOL in_atomic_block, BOOL routines_allowed, tobjnum container_objnum);
  t_locdecl * find_name(const char * name, t_loc_categ loccat) const;
  void activate(CIp_t CI);
  void deactivate(CIp_t CI);

  t_scope(BOOL paramsIn)
    { ref_cnt=1;  next_scope=NULL;  extent=0;  params=paramsIn;  system=FALSE; }
  t_scope(void); // creating the system scope
  virtual ~t_scope(void);
  void mark_core(void) // does not mark itself!
  { for (int i=0;  i<locdecls.count();  i++)
     locdecls.acc0(i)->mark_core();
    locdecls.mark_core();
    //if (next_scope) next_scope->mark_core();  -- not necessary, I think
  }
};

struct t_shared_scope : public t_scope
{ friend void dummy_friend(void);  // prevents warning in GCC
  t_shared_scope(BOOL paramsIn) : t_scope(paramsIn)
    { }
  void AddRef(void)  
    { InterlockedIncrement(&ref_cnt); }
  void Release(void) 
    { if (InterlockedDecrement(&ref_cnt)<=0) delete this; }
 private:  // disables direct destructing, must call Release() instead
  virtual ~t_shared_scope(void)
    { }
};

t_locdecl * find_name_in_scopes(CIp_t CI, const char * name, int & level, t_loc_categ loccat);

#define GLOBAL_DECLS_LEVEL -3

struct t_global_scope : public t_scope
{ WBUUID schema_uuid;
  sql_statement * init_stmt;
  t_global_scope * next_gscope;
  tobjnum objnum;                // objnum of the source module, used when debugging
  t_global_scope(const WBUUID schema_uuidIn) : t_scope(FALSE)
    { memcpy(schema_uuid, schema_uuidIn, UUID_SIZE);  init_stmt=NULL;  next_gscope=NULL; }
};

extern t_global_scope * installed_global_scopes;

void WINAPI compile_declarations_entry(CIp_t CI);
t_global_scope * create_global_sscope(cdp_t cdp, const WBUUID schema_uuid);
t_rscope * create_global_rscope(cdp_t cdp);

inline t_global_scope * look_global_sscope(cdp_t cdp, const WBUUID schema_uuid)
// Searches for existing global scope, returns it or NULL if it does not exist.
{ t_global_scope * gs = installed_global_scopes;
  while (gs!=NULL && memcmp(schema_uuid, gs->schema_uuid, UUID_SIZE))
    gs=gs->next_gscope;
  return gs; // found scope or NULL
}

inline t_global_scope * find_global_sscope(cdp_t cdp, const WBUUID schema_uuid)
{ 
  t_global_scope * gs = look_global_sscope(cdp, schema_uuid);
  if (!gs) gs=create_global_sscope(cdp, schema_uuid);
  return gs; // found scope or NULL
}

inline t_rscope * find_global_rscope(cdp_t cdp)
// Returns the global_rscope form the selected global_sscope.
{ if (cdp->global_sscope==NULL) return NULL;  // no schema open or error in global decls
  t_rscope * grs = cdp->created_global_rscopes;
  while (grs!=NULL && grs->sscope!=cdp->global_sscope)
    grs=grs->next_rscope;
  if (!grs) grs=create_global_rscope(cdp);
  return grs; // found scope or NULL
}

void mark_global_sscopes(void);
void refresh_global_scopes(cdp_t cdp);

// Class for temporary replacing the short-term schema context
class t_short_term_schema_context
{ const uns8 * orig_uuid;
  cdp_t cdp;  
  bool changed;
  t_global_scope * saved_sscope;
 public:
  t_short_term_schema_context(cdp_t cdpIn, const uns8 * new_uuid)
    { cdp=cdpIn;
      changed = memcmp(cdp->current_schema_uuid, new_uuid, UUID_SIZE)!=0;
      if (changed)
      {// save the current global sscope local if it is not in the global list
       // this prevents stack overflow when the scope is just being compiled and find_global_sscope would start another compilation
        if (!look_global_sscope(cdp, cdp->current_schema_uuid))
          saved_sscope=cdp->global_sscope;
        else
          saved_sscope=NULL;
        orig_uuid=cdp->current_schema_uuid;
        cdp->current_schema_uuid=new_uuid;
        cdp->global_sscope=find_global_sscope(cdp, new_uuid);
        cdp->global_rscope=find_global_rscope(cdp);
      }
    }
  ~t_short_term_schema_context(void)
    { if (changed)
      { cdp->current_schema_uuid=orig_uuid; 
       // restore the original sscope/rscope only if existed before - prevents stack overflow when just compiling the sscope
        if (saved_sscope)
          cdp->global_sscope=saved_sscope;
        else
          cdp->global_sscope=find_global_sscope(cdp, orig_uuid);
        cdp->global_rscope=find_global_rscope(cdp);
      }
    }
};

class t_ultralight_schema_context
{ const uns8 * orig_uuid;
  cdp_t cdp;  bool changed;
 public:
  t_ultralight_schema_context(cdp_t cdpIn, const uns8 * new_uuid)
    { cdp=cdpIn;
      changed = memcmp(cdp->current_schema_uuid, new_uuid, UUID_SIZE)!=0;
      if (changed)
      { orig_uuid=cdp->current_schema_uuid;
        cdp->current_schema_uuid=new_uuid;
      }
    }
  ~t_ultralight_schema_context(void)
    { if (changed)
        cdp->current_schema_uuid=orig_uuid; 
    }
};

/////////////////////// cacheable routines and triggers /////////////////////////
class t_trigger;

// Common superclass of routines and trigges, contains data indentifying and linking then in the cache
class t_cachable_psm
{public: 
  const tcateg category;          // CATEG_PROC or CATEG_TRIGGER or CATEG_INFO for incomplete PSM
  const tobjnum objnum; // number of object containing the routine (debugging), NOOBJECT for local routines 
  t_zcr version;        // version number of the object
 protected:
  t_cachable_psm * next_in_list;  // next in the same hash item
  friend class t_psm_cache;
  t_cachable_psm(tcateg categoryIn, tobjnum objnumIn) : objnum(objnumIn), category(categoryIn)
   { }
  virtual ~t_cachable_psm(void)
   { }
  virtual void mark_core(void) = 0;
};

#define PSM_CACHE_SIZE 37

class t_psm_cache
{ t_cachable_psm * tab[PSM_CACHE_SIZE];
  CRITICAL_SECTION cs;
  unsigned number_of_objects_in_cache;
  unsigned hash_fnc(tobjnum objnum)
    { return objnum % PSM_CACHE_SIZE; }
 public:
  void init(void);
  void destroy(void);
  t_psm_cache(void);
  ~t_psm_cache(void);
  t_cachable_psm * get(tobjnum objnum, tcateg category);
  t_routine * get_routine(tobjnum objnum)
    { return (t_routine *)get(objnum, CATEG_PROC); }
  t_routine * get_routine_info(tobjnum objnum)
    { return (t_routine *)get(objnum, CATEG_INFO); }
  t_trigger * get_trigger(tobjnum objnum)
    { return (t_trigger *)get(objnum, CATEG_TRIGGER); }
  void put(t_cachable_psm * compobj);
  void invalidate(tobjnum objnum, tcateg category);
  void mark_core(void);
};

extern t_psm_cache psm_cache;

///////////////////////////// routine ///////////////////////////////////////
class t_routine : public t_cachable_psm
// Routines are co-owned either by block (local routines) or
// by call-statement, call-expression and cache (global routines).
{ t_scope scope;
  FARPROC procptr;       // defined only for external routines
 public:
  tptr    names;         // function & library name, !=NULL for external
  int rettype;    // 0 for procedure
  t_specif retspecif;
  sql_statement * stmt;  // NULL for external routines
  tobjname name;
  WBUUID schema_uuid;    // schema the routine belongs (will belong) to
  int scope_level;
  unsigned external_frame_size;  // defined only for external routines, frame size is reduced for referential parameters
  bool extension_routine;

  t_routine(tcateg categoryIn, tobjnum objnumIn) : t_cachable_psm(categoryIn, objnumIn), scope(TRUE)
  { stmt=NULL;  names=NULL;  extension_routine=false;
    procptr=NULL;  
  }
  virtual ~t_routine(void);
  void mark_core(void);  // marks itself, too

  FARPROC proc_ptr(cdp_t cdp);
  void free_proc_ptr(void);
  const t_scope * param_scope(void) const { return &scope; }
  void compile_routine(CIp_t CI, symbol sym, tobjname nameIn, tobjname schemaIn);
  BOOL is_internal(void) const { return stmt!=NULL; }
  BOOL is_local   (void) const { return objnum==NOOBJECT; }
  void compute_external_frame_size(void);
  void extobj2routine(CIp_t CI, extension_object * obj, HINSTANCE hInst);
};

enum t_trigger_events { TGA_INSERT, TGA_DELETE, TGA_UPDATE };

enum t_trig_list_comp { TRIG_COMP_DEFAULT, TRIG_COMP_EQUAL, TRIG_COMP_ANY };

class t_trigger : public t_cachable_psm
{ friend BOOL find_break_line(cdp_t cdp, tobjnum objnum, uns32 & line);
  BOOL  time_before;
  t_trigger_events tevent;
  tobjname old_name, new_name;  // correlation names
  BOOL  row_granularity;
  t_express * condition;
  sql_statement * stmt;
 public:
  tobjname trigger_name, trigger_schema;
  db_kontext tabkont;           // references to attributes compiled in this kontext, must redirect when executing
  t_trig_list_comp list_comparison;
 private:
  t_scope scope;  // used only during the compilation of trigger, in run-time using trigger_scope from table_decsr
  int scope_level;
  inline int _trigger_type(void)
  { int ttp = row_granularity ?
      (time_before ?
        (tevent==TGA_INSERT ? TRG_BEF_INS_ROW : tevent==TGA_DELETE ? TRG_BEF_DEL_ROW : TRG_BEF_UPD_ROW) :
        (tevent==TGA_INSERT ? TRG_AFT_INS_ROW : tevent==TGA_DELETE ? TRG_AFT_DEL_ROW : TRG_AFT_UPD_ROW))
      : (time_before ?
        (tevent==TGA_INSERT ? TRG_BEF_INS_STM : tevent==TGA_DELETE ? TRG_BEF_DEL_STM : TRG_BEF_UPD_STM) :
        (tevent==TGA_INSERT ? TRG_AFT_INS_STM : tevent==TGA_DELETE ? TRG_AFT_DEL_STM : TRG_AFT_UPD_STM));
    if (*old_name && !time_before) ttp |= tevent==TGA_DELETE ? TRG_AFT_DEL_OLD : TRG_AFT_UPD_OLD;
    if (*new_name &&  time_before) ttp |= tevent==TGA_INSERT ? TRG_BEF_INS_NEW : TRG_BEF_UPD_NEW;
    return ttp;
  }
 // support for debugging without rscope:
  tobjnum noscope_upper_objnum;
  uns32   noscope_upper_line;
  t_linechngstop noscope_upper_linechngstop;
 public:
  t_atrmap_flex column_map;  // Initialized in compile_trigger() when !partial_comp && tevent==TGA_UPDATE && column_map_explicit
  BOOL  column_map_explicit;
  tobjname table_name;
  int trigger_type;
  t_atrmap_flex rscope_usage_map; // Initialized in compile_trigger() when !partial_comp. Column usage from the memory scope (NEW in BEF, OLD in AFT triggers)

  t_trigger(tobjnum objnumIn) : t_cachable_psm(CATEG_TRIGGER, objnumIn) , scope(2)
    { condition=NULL;  stmt=NULL;  list_comparison=TRIG_COMP_DEFAULT; }
  virtual ~t_trigger(void);
  void compile_trigger(CIp_t CI, tobjname nameIn, tobjname schemaIn, BOOL partial_comp);
  BOOL exec(cdp_t cdp);
  inline void position(trecnum pos) { tabkont.position=pos; }
  inline BOOL has_db_access(void) { return (time_before ? *old_name : *new_name)!=0; }
  void *operator new(size_t size)
    { return corealloc(size, 207); }
  void operator delete(void * ptr)
    { corefree(ptr); }
  void mark_core(void);  // marks itself too, does not mark recursivety in the list
};

t_trigger * obtain_trigger(cdp_t cdp, tobjnum objnum);
BOOL get_trigger_table(cdp_t cdp, WBUUID uuid, tobjnum objnum, ttablenum & tabobj, int & trigger_type);
BOOL execute_triggers(cdp_t cdp, table_descr * tbdf, int trigger_type, t_rscope * rscope, trecnum posrec, t_atrmap_flex * column_map = NULL);
t_shared_scope * get_trigger_scope(cdp_t cdp, table_descr * tbdf);
void rscope_init(t_rscope * rscope);
void rscope_clear(t_rscope * rscope);
void rscope_set_null(t_rscope * rscope); // affects the non-heap variables only!
BOOL load_record_to_rscope(cdp_t cdp, table_descr * tbdf, trecnum rec, 
       t_rscope * rscope1, const t_atrmap_flex * map1, t_rscope * rscope2 = NULL, const t_atrmap_flex * map2 = NULL);
//////////////////////////////// sql statement ///////////////////////////////
/************************ prepared stamements ********************************/

struct name_descr
{ char name  [OBJ_NAME_LEN+1];
  char schema[OBJ_NAME_LEN+1];
};

class sql_statement  // base class used as the "empty" statement
{public:
  const statement_types statement_type;
  sql_statement * next_statement;
  uns32 source_line;   // source line containing the executive part of the statement
  sql_statement(statement_types statement_typeIn) : statement_type(statement_typeIn)
    { next_statement=NULL; }
  virtual ~sql_statement(void)
    { if (next_statement!=NULL) delete next_statement; }
  virtual BOOL exec(cdp_t cdp)   // used by empty statement
    { return FALSE; }
  virtual void mark_core(void)
    { mark(this);  if (next_statement) next_statement->mark_core(); }
  void *operator new(size_t size)
    { return corealloc(size, 208); }
  void operator delete(void * ptr)
    { corefree(ptr); }
};

class sql_stmt_create_table : public sql_statement
{
 public:
  table_all * ta;  // owning and used when executing
  bool conditional;
  sql_stmt_create_table(bool conditionalIn) : sql_statement(SQL_STAT_CREATE_TABLE), conditional(conditionalIn)
    { ta=NULL; }
  virtual ~sql_stmt_create_table(void)
    { if (ta) delete ta; }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (ta) ta->mark_core();
  }
};

class sql_stmt_create_view : public sql_statement
{public:
  tobjname name, schema;
  tptr source;        // view source copy
  t_query_expr * qe;  // for short-term owning only
  bool conditional;
  sql_stmt_create_view(bool conditionalIn) : sql_statement(SQL_STAT_CREATE_VIEW), conditional(conditionalIn)
    { qe=NULL;  source=NULL; }
  virtual ~sql_stmt_create_view(void)
    { if (qe!=NULL) qe->Release();  corefree(source); }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_create_schema : public sql_statement
{
 public:
  tobjname schema_name;
  bool conditional;
  sql_stmt_create_schema(void) : sql_statement(SQL_STAT_CREATE_SCHEMA) 
    { conditional=false; }
  virtual ~sql_stmt_create_schema(void)  { }
  virtual BOOL exec(cdp_t cdp);
};

struct t_uname
{ tcateg category;               // 0 iff category not specified, CATEG_USER, CATEG_GROUP or CATEG_ROLE
  tobjname subject_name;
  tobjname subject_schema_name;  // defined only for category==CATEG_ROLE
};

SPECIF_DYNAR(uname_dynar, t_uname, 1);

class sql_stmt_grant_revoke : public sql_statement
{
  BOOL single_oper(cdp_t cdp, table_descr * tbdf, trecnum recnum, privils & prvs);
 public:
  t_privils_flex priv_val;
  //ttablenum tabobj;
  tobjname schemaname, objname;
  BOOL is_routine;
  uname_dynar user_names;

  tobjname current_cursor_name;        // "" unless WHERE CURRENT OF name
  t_query_expr * qe;                   // NULL for full table or CURRENT OF
  t_optim * opt;

  sql_stmt_grant_revoke(statement_types statement_typeIn) : sql_statement(statement_typeIn)
    { current_cursor_name[0]=0;  qe=NULL;  opt=NULL; }
  virtual ~sql_stmt_grant_revoke(void)
  { if (opt!=NULL) opt->Release();  // opt may have links to qe
    if (qe !=NULL) qe ->Release();  
  }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core();  user_names.mark_core(); 
    if (qe) qe->mark_core();
    if (opt) opt->mark_core();
    priv_val.mark_core();
  }
};

class sql_stmt_create_alter_rout : public sql_statement
{ bool creating;
  bool conditional;
  tobjname name, schema;
  tptr source;        // routine source copy
  t_routine * rout;   // for short-term owning only during test compilation
 public:
  sql_stmt_create_alter_rout(bool creatingIn, bool conditionalIn=false) : sql_statement(SQL_STAT_CREATE_ROUTINE),
    creating(creatingIn), conditional(conditionalIn)
      { rout=NULL;  source=NULL; }
  virtual ~sql_stmt_create_alter_rout(void)
    { if (rout!=NULL) delete rout;  corefree(source); }
  virtual BOOL exec(cdp_t cdp);
  void compile(CIp_t CI);
  void get_text(CIp_t CI);
};

class sql_stmt_create_alter_trig : public sql_statement
{ bool creating;
  bool conditional;
  tobjname name, schema;  // [schema]=="" iff schema not specified in the statement
  tptr source;        // trigger source copy
  t_trigger * trig;   // for short-term owning only during test compilation
 public:
  sql_stmt_create_alter_trig(bool creatingIn, bool conditionalIn=false) : sql_statement(SQL_STAT_CREATE_TRIGGER),
    creating(creatingIn), conditional(conditionalIn)
      { source=NULL;  trig=NULL; }
  virtual ~sql_stmt_create_alter_trig(void)
    { corefree(source);  delete trig; }
  virtual BOOL exec(cdp_t cdp);
  void compile(CIp_t CI);
  void get_text(CIp_t CI);
};

class sql_stmt_create_alter_dom : public sql_statement
{ bool creating;
  bool conditional;
  tobjname name, schema;  // [schema]=="" iff schema not specified in the statement
  tptr source;            // source copy
  int tp;
  t_specif specif;
 public:
  sql_stmt_create_alter_dom(bool creatingIn, bool conditionalIn=false) : sql_statement(SQL_STAT_CREATE_DOMAIN),
    creating(creatingIn), conditional(conditionalIn)
      { source=NULL; }
  virtual ~sql_stmt_create_alter_dom(void)
    { corefree(source); }
  virtual BOOL exec(cdp_t cdp);
  void compile(CIp_t CI);
  //void get_text(CIp_t CI);
};

class sql_stmt_create_alter_seq : public sql_statement
{ bool creating;
  bool conditional;
 public:
  t_sequence seq;
  sql_stmt_create_alter_seq(bool creatingIn, bool conditionalIn=false) : sql_statement(SQL_STAT_CREATE_SEQ),
    creating(creatingIn), conditional(conditionalIn)
      { }
  virtual ~sql_stmt_create_alter_seq(void)
    { }
  virtual BOOL exec(cdp_t cdp);
  void compile(CIp_t CI);
};

class sql_stmt_create_alter_fulltext : public sql_statement
{ bool creating;
  bool conditional;
 public:
  char * source;     // statement source copy
  t_fulltext ftdef;  // for short-term owning only during test compilation, statement recompiled on execution
  sql_stmt_create_alter_fulltext(bool creatingIn, bool conditionalIn=false) : sql_statement(SQL_STAT_CREATE_FT),
    creating(creatingIn), conditional(conditionalIn)
      { source=NULL; }
  virtual ~sql_stmt_create_alter_fulltext(void)
    { corefree(source); }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_create_drop_subject : public sql_statement
{ const bool creating;
 public:
  bool conditional;
  tcateg  categ;
  tobjname subject_name;
  tobjname subject_schema;  // defined only for CATEG_ROLE
  char * password;          // used only for CATEG_USER
  sql_stmt_create_drop_subject(bool creatingIn) : sql_statement(SQL_STAT_CRE_DROP_SUBJ), creating(creatingIn), password(NULL) 
    { conditional=false; }
  virtual ~sql_stmt_create_drop_subject(void) 
    { if (password) corefree(password); }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (password) mark(password);
  }
};

class sql_stmt_create_alter_front_object : public sql_statement
{public:
  bool creating;
  bool conditional;
  tcateg  categ;
  tobjname name;
  tobjname schema;
  char * definition;
  sql_stmt_create_alter_front_object(bool creatingIn) : 
    sql_statement(SQL_STAT_CRE_FRONT_OBJ), creating(creatingIn), definition(NULL) 
      { conditional=false; }
  virtual ~sql_stmt_create_alter_front_object(void) 
    { if (definition) corefree(definition); }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (definition) mark(definition);
  }
};

class sql_stmt_rename_object : public sql_statement
{public:
  bool conditional;
  tcateg  categ;
  tobjname orig_name;
  tobjname schema;
  tobjname new_name;
  sql_stmt_rename_object(void) : sql_statement(SQL_STAT_RENAME_OBJECT) 
    { conditional=false; }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_select : public sql_statement
{public:
  tptr source;   // source copy, passed to create_cursor, used by subcursors
  t_query_expr * qe;
  t_optim * opt;
  BOOL is_into, dynpar_referenced, embvar_referenced;
  sql_stmt_select(void) : sql_statement(SQL_STAT_SELECT)
  { qe=NULL;  opt=NULL;  is_into=TRUE;  source=NULL;
    dynpar_referenced = embvar_referenced = FALSE;
  }
  virtual ~sql_stmt_select(void)
  { if (opt!=NULL) opt->Release();    // opt may have links to qe
    if (qe !=NULL) qe ->Release();  
    corefree(source);
  }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (source) mark(source);
    if (qe) qe->mark_core();
    if (opt) opt->mark_core();
  }
};

class sql_stmt_drop : public sql_statement
{public:
  tcateg   categ;
  tobjname schema;
  tobjname main;  // name of the object
  bool conditional;
  bool cascade;
  sql_stmt_drop(tcateg categIn) : sql_statement(SQL_STAT_DROP), categ(categIn)
    { cascade=conditional=false; }
  virtual ~sql_stmt_drop(void) { }
  virtual BOOL exec(cdp_t cdp);
};

//////////////////////////////// trigger access encapsulation for single table ///////////////////////////////////
class t_tab_trigger
// Structure supporting calling triggers during tb_write_vector and in sql_stmt_delete
// Not reentrant, so cannot be owned by a table. 
// Created ad-hoc for cascaded update, a copy per table contained in t_ins_upd_trig_supp and sql_stmt_delete.
/* Usage: 1. prepare_rscopes() - creates rscopes and rscope usage maps.
          2. add_column_to_map for every column updated (UPDATE operation only)
            3. pre_stmt() calls BEFORE STMT triggers
              4. pre_row() loads old values into rscopes, updates bef_rscope and calls BEFORE ROW triggers
              5. If has_new_values() then new_value() return the new value from bef_rscope
              6. post_row() calls AFTER ROW triggers
            7. post_stmt() calls AFTER STMT triggers
*/
{ t_trigger_events event;   // operation the class is created for
  uns32 trigger_map;        // copy of trigger_map of the table
  t_atrmap_flex column_map;
  t_rscope * bef_rscope, * aft_rscope; // memory rscopes necessary for triggers
  t_atrmap_flex bef_rscope_usage_map,       // usage maps of columns in the memory rscopes.
                aft_rscope_usage_map;
  bool bef_rscope_contains_values, aft_rscope_contains_values; // if true the rscope may contain allocated long values
  bool bef_rscope_is_fictive, aft_rscope_is_fictive;           // is true when scope cannot contain any data
  t_rscope * prep_rscope(cdp_t cdp, table_descr * tbdf, int trigger_type, t_atrmap_flex * usage_map);
  t_rscope * prep_fictive_rscope(cdp_t cdp, table_descr * tbdf, t_atrmap_flex * usage_map, bool * is_fictive);
 public:
  inline t_tab_trigger(void)
  { bef_rscope=aft_rscope=NULL; 
    bef_rscope_contains_values=aft_rscope_contains_values=false;
    bef_rscope_is_fictive=aft_rscope_is_fictive=false;
  }

  ~t_tab_trigger(void)
  { if (bef_rscope)
      { if (!bef_rscope_is_fictive) rscope_clear(bef_rscope);  delete bef_rscope; }
    if (aft_rscope)
      { if (!aft_rscope_is_fictive) rscope_clear(aft_rscope);  delete aft_rscope; }
  }

  inline const t_atrmap_flex * active_rscope_usage_map(void)
    { return &bef_rscope_usage_map; }
  BOOL prepare_rscopes(cdp_t cdp, table_descr * tbdf, t_trigger_events eventIn);
  inline void add_column_to_map(tattrib colnum)   // must be called AFTER prepare_rscopes() which inits the map, and only for TGA_UPDATE
    { column_map.add(colnum); }

  inline BOOL pre_stmt(cdp_t cdp, table_descr * tbdf)  // run BEF STM triggers
  // rscopes do not have valid values and the values are not accessible from the triger. They are used only by the debugger.
  { if (event==TGA_INSERT)
    { if (trigger_map & TRG_BEF_INS_STM) 
        if (execute_triggers(cdp, tbdf, TRG_BEF_INS_STM, bef_rscope, NORECNUM             )) return TRUE; 
    }
    else if (event==TGA_UPDATE)
    { if (trigger_map & TRG_BEF_UPD_STM) 
        if (execute_triggers(cdp, tbdf, TRG_BEF_UPD_STM, bef_rscope, NORECNUM, &column_map)) return TRUE; 
    }
    else // TGA_DELETE
    { if (trigger_map & TRG_BEF_DEL_STM) 
        if (execute_triggers(cdp, tbdf, TRG_BEF_DEL_STM, bef_rscope, NORECNUM             )) return TRUE; 
    }
    return FALSE;
  }
  inline BOOL post_stmt(cdp_t cdp, table_descr * tbdf)  // run AFT STM triggers
  // rscopes do not have valid values and the values are not accessible from the triger. They are used only by the debugger.
  { if (event==TGA_INSERT)
    { if (trigger_map & TRG_AFT_INS_STM)  // AFT INS STM triggers:
        if (execute_triggers(cdp, tbdf, TRG_AFT_INS_STM, aft_rscope, NORECNUM             )) return TRUE;
    }
    else if (event==TGA_UPDATE)
    { if (trigger_map & TRG_AFT_UPD_STM)  // AFT UPD STM triggers:
        if (execute_triggers(cdp, tbdf, TRG_AFT_UPD_STM, aft_rscope, NORECNUM, &column_map)) return TRUE;
    }
    else // TGA_DELETE
    { if (trigger_map & TRG_AFT_DEL_STM)  // AFT DEL STM triggers:
        if (execute_triggers(cdp, tbdf, TRG_AFT_DEL_STM, aft_rscope, NORECNUM             )) return TRUE; 
    }
    return FALSE;
  }

  BOOL pre_row(cdp_t cdp, table_descr * tbdf, trecnum position, const t_vector_descr * vector);
  inline BOOL has_new_values(void) 
    { return bef_rscope!=NULL && !bef_rscope_is_fictive; }
  const char * new_value(int attr);
  BOOL post_row(cdp_t cdp, table_descr * tbdf, trecnum position);
  void mark_core(void) // Must not mark itself, may be a memeber. Should mark rscope contents unless the scope is fictive, too.
  { if (bef_rscope) bef_rscope->mark_core();
    if (aft_rscope) aft_rscope->mark_core();
    column_map.mark_core();
    bef_rscope_usage_map.mark_core();
    aft_rscope_usage_map.mark_core();
  }
};

///////////////////////////////// insert and update trigger support /////////////////////////////////////////
struct t_ins_upd_tab
{ int ordnum;        // number of the table in the cursor, 0 in single table oprerations (not used)
  t_tab_trigger ttr; // trigger access for one table
  db_kontext * tabkont;  // defined for operations on cursors only, not NULL for updatable tables in the cursor
  t_ins_upd_tab(void) : tabkont(NULL) { }
};

class t_ins_upd_trig_supp
// Supports vectorised operations on a table or cursor.
// Owned by UPDATE and INSERT statements, created ad-hoc for Write and Write_record_ex operations
{public: 
  t_colval * colvals;
 private:
  unsigned column_space, colval_pos;  // index of the 1st free colvals[] item, used when adding columns
  t_ins_upd_tab * ttrs;
  int table_count;
  BOOL is_init; 
 public:
  t_ins_upd_trig_supp(void)
    { colvals=NULL;  ttrs=NULL;  is_init=FALSE; }
  ~t_ins_upd_trig_supp(void)
    { if (colvals) 
      { for (t_colval * colval = colvals;  colval->attr!=NOATTRIB;  colval++)
          if (colval->expr) delete colval->expr;
        corefree(colvals);  
      }
      if (ttrs) delete [] ttrs;
    }
 // list of columns:
  BOOL start_columns(unsigned column_count);
  void stop_columns(void);
  BOOL register_column(int colnum, db_kontext * kont, t_express * indexexpr = NULL);
  inline void add_ptrs(char * dataptr, uns32 * lenptr, uns32 * offset)
  { colvals[colval_pos].dataptr=dataptr;
    colvals[colval_pos].lengthptr=lenptr;
    colvals[colval_pos].offsetptr=offset;
    colval_pos++;
    colvals[colval_pos].attr=NOATTRIB;  // the destructor needs the colvals array always to be terminated!
  }
  inline void add_expr(t_express * expr)
  { colvals[colval_pos].expr=expr;
    colval_pos++;
    colvals[colval_pos].attr=NOATTRIB;  // the destructor needs the colvals array always to be terminated!
  }
  void release_data(int pos, BOOL release_ptr)
  { if (colvals[pos].lengthptr) *colvals[pos].lengthptr=0;
    if (release_ptr && colvals[pos].dataptr)
      { corefree(colvals[pos].dataptr);  colvals[pos].dataptr=NULL; }
    colvals[pos].table_number=1;  // disable writing
  }
  bool store_data(int pos, char * dataptr, unsigned size, bool appending);  // appends data to a var-len buffer
  char * get_dataptr(int pos);
  void disable_pos(int pos)
  { colvals[pos].table_number|=0x80; }  // this disables writing the column unless its value is stored
  void enable_pos(int pos)
  { colvals[pos].table_number=0; }  // this enables writing the column 

 // privils:
  int is_globally_updatable_in_table(cdp_t cdp, table_descr * tbdf, db_kontext * kont);
  bool is_record_updatable_in_table(cdp_t cdp, table_descr * tbdf, db_kontext * kont, trecnum recnum);
  int is_globally_updatable_in_cursor(cdp_t cdp, cur_descr * cd);
  bool is_record_updatable_in_cursor(cdp_t cdp, cur_descr * cd, trecnum recnum);

 // ttrs and operations:
  BOOL init_ttrs(cdp_t cdp, table_descr * tbdf, cur_descr * cd, BOOL inserting);
  trecnum execute_insert(cdp_t cdp, table_descr * tbdf, BOOL append_it);
  BOOL execute_write_table(cdp_t cdp, table_descr * tbdf, trecnum recnum);
  BOOL execute_write_cursor(cdp_t cdp, cur_descr * cd, trecnum recnum);
  inline BOOL pre_stmt_table(cdp_t cdp, table_descr * tbdf)
    { return ttrs->ttr.pre_stmt(cdp, tbdf); }
  inline BOOL post_stmt_table(cdp_t cdp, table_descr * tbdf)
    { return ttrs->ttr.post_stmt(cdp, tbdf); }
  BOOL pre_stmt_cursor(cdp_t cdp, cur_descr * cd);
  BOOL post_stmt_cursor(cdp_t cdp, cur_descr * cd);
  void mark_core(void)
  { if (colvals) 
    { for (t_colval * colval = colvals;  colval->attr!=NOATTRIB;  colval++)
        if (colval->expr) colval->expr->mark_core();
      mark(colvals);
    }
    if (ttrs)
    { mark_array(ttrs);
      for (int i=0;  i<table_count;  i++) ttrs[i].ttr.mark_core();
    }
  }
};

class t_ins_upd_ext_supp
{public: 
  table_descr * tbdf;
  t_ins_upd_trig_supp iuts_insert;
  t_ins_upd_trig_supp iuts_update;
  char * databuf;
  wbstmt * stmt;  // statement searching the record by the key
  cdp_t cdp;  // used in the destructor, must remove the [stmt] from cdp
  tattrib * column_list;  // copy of the "update key column list", terminated by 0

  t_ins_upd_ext_supp(cdp_t cdpIn)
    { cdp=cdpIn;  tbdf=NULL;  databuf=NULL;  stmt=NULL;  column_list=NULL; }
  ~t_ins_upd_ext_supp(void);
  bool make_search_statement(cdp_t cdp, tattrib * attrs);
  trecnum find_record_by_key_columns(cdp_t cdp);
};

t_ins_upd_ext_supp * eius_prepare(cdp_t cdp, ttablenum tbnum, tattrib * attrs);
t_ins_upd_ext_supp * eius_prepare_orig(cdp_t cdp, ttablenum tbnum);
void eius_drop(t_ins_upd_ext_supp * eius);
void eius_clear(t_ins_upd_ext_supp * eius);
bool eius_store_data(t_ins_upd_ext_supp * eius, int pos, char * dataptr, unsigned size, bool appending);  // appends data to a var-len buffer
char * eius_get_dataptr(t_ins_upd_ext_supp * eius, int pos);
trecnum eius_insert(cdp_t cdp, t_ins_upd_ext_supp * eius);
bool eius_read_column_value(cdp_t cdp, t_ins_upd_ext_supp * eius, trecnum recnum, int pos);

class sql_stmt_delete : public sql_statement
{ t_privil_test_result_cache privil_test_result_cache;
  BOOL cached_global_privils_ok(cdp_t cdp, table_descr * tbdf, BOOL & check_rec_privils);
  inline BOOL record_privils_ok(cdp_t cdp, table_descr * tbdf, trecnum tr);
  tobjnum objnum;                      // used by full permanent table operation
  int locdecl_level, locdecl_offset;   // locdecl_offset!=-1 only when deleting in a local table
  tobjname current_cursor_name;        // "" unless WHERE CURRENT OF name
  t_query_expr * qe;                   // NULL for full table or CURRENT OF
  t_optim * opt;
  t_tab_trigger ttr;
  BOOL ttr_prepared;
 public:
  sql_stmt_delete(void) : sql_statement(SQL_STAT_DELETE)
  { current_cursor_name[0]=0;  locdecl_offset=-1;
    qe=NULL;  opt=NULL;  ttr_prepared=FALSE;
  }
  virtual ~sql_stmt_delete(void)
  { if (opt!=NULL) opt->Release();  // opt may have links to qe
    if (qe !=NULL) qe ->Release();  
  }
  void compile(CIp_t CI);
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (qe) qe->mark_core();
    if (opt) opt->mark_core();
    ttr.mark_core();
  }
  inline t_query_expr * _qe(void) const
    { return qe; }
};

class sql_stmt_update : public sql_statement
{ 
#ifdef STOP
  int trig_table_cnt;
  BOOL trigger_info_prepared;
  t_trigger_i * trigger_info;
  uns16 glob_trigger_map;
  BOOL prepare_trigger_info(cdp_t cdp);
  void clear_trigger_info(void);
  void destroy_trigger_info(void);
  BOOL update_positioned_record(cdp_t cdp);
#endif

  t_ins_upd_trig_supp iuts;
  tobjnum objnum;                      // used by full permanent table operation
  int locdecl_level, locdecl_offset;   // locdecl_offset!=-1 only when updating in a local table
  tobjname current_cursor_name;        // "" unless WHERE CURRENT OF name
  t_query_expr * qe;                   // NULL for full table or CURRENT OF
  t_optim * opt;
  db_kontext tabkont;                  // used only by full-table update, must exist during executiuon
  t_query_expr * aux_qe;               // used only as kontext holder for CURRENT OF

 public:
  void compile(CIp_t CI);
  sql_stmt_update(void) : sql_statement(SQL_STAT_UPDATE)
  { current_cursor_name[0]=0;  locdecl_offset=-1;
    qe=aux_qe=NULL;  opt=NULL;
    //trig_table_cnt=0;  trigger_info_prepared=FALSE;  trigger_info=NULL;
  }
  virtual ~sql_stmt_update(void)
  { if (opt!=NULL) opt->Release();  // opt may have links to qe
    if (qe !=NULL) qe ->Release();  
    if (aux_qe!=NULL) aux_qe->Release(); 
    //delete_assgns(&assgns);
    //destroy_trigger_info();
  }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    iuts.mark_core();
#ifdef STOP
    if (trigger_info) 
    { for (i=0;  i<trig_table_cnt;  i++)
      { if (trigger_info[i].aft_rscope) mark(trigger_info[i].aft_rscope);
        if (trigger_info[i].bef_rscope) mark(trigger_info[i].bef_rscope);
        trigger_info[i].vector.mark_core();
      }
      mark(trigger_info);
    }
#endif
    if (qe) qe->mark_core();
    if (opt) opt->mark_core();
    tabkont.mark_core();
    //for (i=0;  i<assgns.count();  i++)
    //  assgns.acc0(i)->mark_core();
    //assgns.mark_core();
  }
};

class cur_descr;

SPECIF_DYNAR(t_colnum_dynar, tattrib, 10);

class sql_stmt_insert : public sql_statement
{ t_privil_test_result_cache privil_test_result_cache;
  t_ins_upd_trig_supp iuts;
  tobjnum objnum;                      // used by permanent table insertion
  int locdecl_level, locdecl_offset;   // locdecl_offset!=-1 only when inserting to a local table
  t_query_expr * qe;                   // NULL for table destination
  t_optim * opt;
  db_kontext tabkont;                  // used only by table insert, must exist during executiuon
  t_query_expr * src_qe;               // SELECT contained in INSERT, NULL if none
  t_optim * src_opt;
 // auxiliary var:
  cur_descr * dcd;

 public:
  sql_stmt_insert(void) : sql_statement(SQL_STAT_INSERT)//, vector(NULL)
    { qe=src_qe=NULL;  opt=src_opt=NULL;  locdecl_offset=-1; }
  virtual ~sql_stmt_insert(void)
  { if (opt!=NULL) opt->Release();
    if (qe !=NULL) qe ->Release();  
    if (src_opt!=NULL) src_opt->Release();
    if (src_qe!=NULL) src_qe->Release();  
    //if (vector.colvals) 
    //{ vector.release_colval_exprs();
    //  corefree(vector.colvals);  vector.colvals=NULL; 
    //}
  }
  void compile(CIp_t CI);
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    iuts.mark_core();
    if (qe) qe->mark_core();
    if (opt) opt->mark_core();
    tabkont.mark_core();
    if (src_qe) src_qe->mark_core();
    if (src_opt) src_opt->mark_core();
    //vector.mark_core();
  }
  BOOL insert_record(cdp_t cdp, table_descr * tbdf, tcursnum dcursnum);
};

////////////////////////////// block ///////////////////////////////////////
class sql_stmt_block : public sql_statement
{
 public:
  BOOL atomic;
  t_idname label_name;
  sql_statement * body;
  t_scope scope;
  int scope_level;
  uns32 source_line2;

  sql_stmt_block(const char * label_nameIn) : sql_statement(SQL_STAT_BLOCK), scope(FALSE)
  { atomic=FALSE;  body=NULL;
    strmaxcpy(label_name, label_nameIn, sizeof(label_name));
  }
  virtual ~sql_stmt_block(void)
  { 
    delete body; 
  }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (body) body->mark_core();
    scope.mark_core();
  }
};

class sql_stmt_set : public sql_statement
{ BOOL owning;
 public:
  t_assgn assgn;
  sql_stmt_set(t_express * defval);
  virtual ~sql_stmt_set(void)
    { delete assgn.dest;  if (owning) delete assgn.source; }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    assgn.mark_core();
  }
};

class sql_stmt_return : public sql_stmt_set
{public:
  tobjname function_name;
  sql_stmt_return(void);
  virtual ~sql_stmt_return(void) { }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_if : public sql_statement
{
 public:
  t_express * condition;
  sql_statement * then_stmt, * else_stmt;
  sql_stmt_if(void) : sql_statement(SQL_STAT_IF)
    { condition=NULL;  then_stmt=else_stmt=NULL; }
  virtual ~sql_stmt_if(void)
    { delete condition;  delete then_stmt;  delete else_stmt; }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (condition) condition->mark_core();
    if (then_stmt) then_stmt->mark_core();
    if (else_stmt) else_stmt->mark_core();
  }
};

class sql_stmt_loop : public sql_statement
{ bool precond;
 public:
  t_idname label_name;
  t_express * condition;
  sql_statement * stmt;
  uns32 source_line2;
  sql_stmt_loop(const char * label_nameIn, bool precondIn) : sql_statement(SQL_STAT_LOOP)
  { condition=NULL;  stmt=NULL;
    strmaxcpy(label_name, label_nameIn, sizeof(label_name));
    precond=precondIn;
  }
  virtual ~sql_stmt_loop(void)
    { delete condition;  delete stmt; }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (condition) condition->mark_core();
    if (stmt) stmt->mark_core();
  }
};

class sql_stmt_leave : public sql_statement
{
 public:
  t_idname label_name;
  sql_stmt_leave(const char * label_nameIn) : sql_statement(SQL_STAT_LEAVE)
    { strmaxcpy(label_name, label_nameIn, sizeof(label_name)); }
  virtual ~sql_stmt_leave(void) { }
  virtual BOOL exec(cdp_t cdp);
};

enum t_routine_ownership {  // used when compiling the routine invocation
  ROUT_GLOBAL_SHARED, // routine to be returned to the cache, temp. owned during the compilation of invocation
  ROUT_LOCAL,         // routine owned by the local declarations
  ROUT_EXTENSION };   // routine permanently owned by the invocation

class sql_stmt_call : public sql_statement, public t_express
{private:
  t_routine * temp_owned_routine; // holder of the called routine, used during the compilation, permanent for extension routines
  t_routine * called_local_routine; // not owning ptr, used when the called routine is local or extension, NULL otherwise
  t_routine * executed_global_routine;  // temporarily owning during the execution, used only by mark_core()
  tobjnum objnum;        // called routine, NOOBJECT for local routines
  t_zcr callee_version;  // called routine version, not used for local routines
  t_privil_test_result_cache privil_test_result_cache;
 public:
  BOOL calling_local_routine(void) 
    { return objnum==NOOBJECT; }
  BOOL detached;         // execute in a separate thread
  unsigned term_event;   // number representing termination event
  t_expr_dynar actpars;  // actual parameters, in the formal parameters order
  int origtype; 
  t_specif origspecif;
  t_value * result_ptr;  // additonal input parameter of exec() - result ptr
  t_expr_funct * std_routine;
  sql_stmt_call(void) : sql_statement(SQL_STAT_CALL), t_express(SQE_ROUTINE)
  { term_event=0;  objnum=NOOBJECT;
    called_local_routine=temp_owned_routine=executed_global_routine=NULL;  std_routine=NULL;  detached=FALSE;
    result_ptr=NULL;  // very important when function called as procedure
  }
  virtual ~sql_stmt_call(void);
  virtual BOOL exec(cdp_t cdp);
  void routine_invocation(CIp_t CI, t_routine * routine, t_routine_ownership routine_ownership);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (called_local_routine) called_local_routine->mark_core();
    if (executed_global_routine) executed_global_routine->mark_core();
    for (int i=0;  i<actpars.count();  i++)
    { t_express * ex = *actpars.acc0(i);
      if (ex) ex->mark_core();  // parameter 0 in extension procedures is NULL
    }
    actpars.mark_core();
    if (std_routine) std_routine->mark_core();
  }
  t_routine * get_called_routine(cdp_t cdp, t_zcr * required_version, BOOL complete);
  void release_called_routine(t_routine * rout);
  void *operator new(size_t size)
    { return corealloc(size, 209); }
  void operator delete(void * ptr)
    { corefree(ptr); }
};

void * call_external_routine(cdp_t cdp, t_routine * croutine, FARPROC procptr, const t_scope * pscope, t_rscope * rscope, void * valbuf);

class sql_stmt_alter_table : public sql_statement
// Storing the source form: it must be analysed and compared to the actual table definition in the run-time.
// Used for CREATE INDEX and DROP INDEX statements too.
{ 
 public:
  bool old_alter_table_syntax;  // the value of the SQL flag valid in the time of compilation
  tptr source;
  bool conditional;             // used only in CREATE INDEX and DROP INDEX
  sql_stmt_alter_table(statement_types statement_typeIn, bool conditionalIn) : 
                           sql_statement(statement_typeIn), conditional(conditionalIn)
    { source=NULL; }
  virtual ~sql_stmt_alter_table(void)
    { corefree(source); }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (source) mark(source);
  }
};

class sql_stmt_open : public sql_statement
{
 public:
  t_query_expr * loc_qe;  // not owning, not NULL iff local declared cursor
  t_optim      * loc_opt; // not owning, not NULL iff local declared cursor
  tobjname cursor_name;
  int level, offset;      // run-time position of open cursor number (offset==-1 for global cursor)
  sql_stmt_open(const char * cursor_nameIn) : sql_statement(SQL_STAT_OPEN)
  { loc_qe=NULL;  loc_opt=NULL;
    strmaxcpy(cursor_name, cursor_nameIn, sizeof(cursor_name));
  }
  virtual ~sql_stmt_open(void)    { }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_close : public sql_statement
{
 public:
  t_query_expr * loc_qe;  // not owning, not NULL iff local declared cursor
  t_optim      * loc_opt; // not owning, not NULL iff local declared cursor
  tobjname cursor_name;
  int level, offset;      // run-time position of open cursor number (offset==-1 for global cursor)
  sql_stmt_close(const char * cursor_nameIn) : sql_statement(SQL_STAT_CLOSE)
  { loc_qe=NULL;  loc_opt=NULL;
    strmaxcpy(cursor_name, cursor_nameIn, sizeof(cursor_name));
  }
  virtual ~sql_stmt_close(void)   { }
  virtual BOOL exec(cdp_t cdp);
};

enum t_fetch_type { FT_UNSPEC, FT_NEXT, FT_PRIOR, FT_FIRST, FT_LAST, FT_ABS, FT_REL };

class sql_stmt_fetch : public sql_statement
{
 public:
  t_fetch_type fetch_type;
  t_express * step;
  tobjname cursor_name;
  int level, offset;      // run-time position of open cursor number (offset==-1 for global cursor)
  t_assgn_dynar assgns;

  sql_stmt_fetch(void) : sql_statement(SQL_STAT_FETCH)
    { step=NULL; }
  virtual ~sql_stmt_fetch(void)
  { if (step!=NULL) delete step;
    delete_assgns(&assgns);
  }

  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (step) step->mark_core();
    for (int i=0;  i<assgns.count();  i++)
      assgns.acc0(i)->mark_core();
    assgns.mark_core();
  }
};

class sql_stmt_signal : public sql_statement
{
 public:
  t_idname name;
  BOOL is_sqlstate;
  sql_stmt_signal(void) : sql_statement(SQL_STAT_SIGNAL) { }
  virtual ~sql_stmt_signal(void)    { }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_resignal : public sql_statement
{
 public:
  t_idname name;  // "" iff condition not specified
  BOOL is_sqlstate;
  sql_stmt_resignal(void) : sql_statement(SQL_STAT_RESIGNAL) { }
  virtual ~sql_stmt_resignal(void)    { }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_start_tra : public sql_statement
{
 public:
  t_isolation isolation;
  BOOL rw;
  sql_stmt_start_tra(void) : sql_statement(SQL_STAT_START_TRA) { }
  virtual ~sql_stmt_start_tra(void)    { }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_commit : public sql_statement
{
 public:
  BOOL chain;
  sql_stmt_commit(void) : sql_statement(SQL_STAT_COMMIT) { }
  virtual ~sql_stmt_commit(void)    { }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_rollback : public sql_statement
{
 public:
  BOOL chain;
  t_idname sapo_name;
  t_express * sapo_target;
  sql_stmt_rollback(void) : sql_statement(SQL_STAT_ROLLBACK)
    { sapo_target=NULL; }
  virtual ~sql_stmt_rollback(void) { delete sapo_target; }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (sapo_target) sapo_target->mark_core();
  }
};

class sql_stmt_sapo : public sql_statement
{
 public:
  t_idname sapo_name;
  t_express * sapo_target;
  sql_stmt_sapo(void) : sql_statement(SQL_STAT_SAPO)
    { sapo_target=NULL; }
  virtual ~sql_stmt_sapo(void)     { delete sapo_target; }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (sapo_target) sapo_target->mark_core();
  }
};

class sql_stmt_rel_sapo : public sql_statement
{
 public:
  t_idname sapo_name;
  t_express * sapo_target;
  sql_stmt_rel_sapo(void) : sql_statement(SQL_STAT_REL_SAPO)
    { sapo_target=NULL; }
  virtual ~sql_stmt_rel_sapo(void) { delete sapo_target; }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (sapo_target) sapo_target->mark_core();
  }
};

class sql_stmt_case : public sql_statement
// CASE statement is complied into a sql_stmt_case list linked by contin.
// Simple CASE has a list header with controlling value in cond and branch==NULL.
// ELSE branch has cond==NULL.
{ const bool searched;
 public:
  t_express     * cond;   // boolean cond in searched, value in simple, NULL in ELSE branch
  sql_statement * branch; // branch statement list, NULL in simple starter
  sql_stmt_case * contin; // next branch pointer, if any
  t_compoper    eq_oper;  // comparison operator (in the top node of the simple CASE only)
  sql_stmt_case(bool searchedIn) : sql_statement(SQL_STAT_CASE), searched(searchedIn)
    { cond=NULL;  branch=NULL;  contin=NULL; }
  virtual ~sql_stmt_case(void)
    { delete cond;  delete branch;  delete contin; }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (cond)   cond  ->mark_core();
    if (branch) branch->mark_core();
    if (contin) contin->mark_core();
  }
};

class sql_stmt_for : public sql_statement
{
 public:
  t_idname label_name;
  t_query_expr  * qe;
  t_optim * opt;
  sql_statement * body;
  t_scope scope;       // scope for for-variable and possible cursnum
  int scope_level;
  t_idname cursor_name;   // "" for unnamed cursor
  int cursnum_offset;  // cursor number offset in the local scope, only if cursor_name[0]
  uns32 source_line2;

  sql_stmt_for(const char * label_nameIn) : sql_statement(SQL_STAT_FOR), scope(FALSE)
  { qe=NULL;  body=NULL;  opt=NULL;
    strmaxcpy(label_name, label_nameIn, sizeof(label_name));
  }
  virtual ~sql_stmt_for(void)
  { if (opt!=NULL) opt->Release();
    if (qe !=NULL) qe ->Release();  
    delete body; 
  }
  virtual BOOL exec(cdp_t cdp);
  virtual void mark_core(void)
  { sql_statement::mark_core(); 
    if (qe)   qe  ->mark_core();
    if (opt)  opt ->mark_core();
    if (body) body->mark_core();
    scope.mark_core();
  }
};

class sql_stmt_set_trans : public sql_statement
{public:
  t_isolation isolation;
  BOOL rw;
  sql_stmt_set_trans(void) : sql_statement(SQL_STAT_SET_TRANS) { }
  virtual ~sql_stmt_set_trans(void) { }
  virtual BOOL exec(cdp_t cdp);
};

class sql_stmt_ : public sql_statement
{
 public:
  sql_stmt_(void) : sql_statement(SQL_STAT_NO) { }
  virtual ~sql_stmt_(void) { }
  virtual BOOL exec(cdp_t cdp);
};


#define SQLSTATE_EXCEPTION_MARK 1 // any non-null non-alfanumeric character
// Starts sqlstate string in handler declaration

/////////////////////////// compiled procedure cache /////////////////////////
#ifdef STOP
#define PSM_CACHE_SIZE 10

struct psm_cache_el
{ tobjnum objnum;       // NOOBJECT iff entry if empty
  t_routine * routine;  // not NULL iff objnum!=NOOBJECT
};

class t_psm_cache
{ psm_cache_el els[PSM_CACHE_SIZE];
  int pos;
  CRITICAL_SECTION cs;
 public:
  void init(void);
  t_psm_cache(void);
  ~t_psm_cache(void);
  t_routine * find_and_lock(tobjnum obj);
  void add(tobjnum obj, t_routine * routine);
  void invalidate(tobjnum obj);
  void mark_core(void);
  BOOL is_in_cache(const t_routine * routine); // debug support
};

extern t_psm_cache psm_cache;
#endif
///////////////////////////// compilation info //////////////////////////////
struct t_sql_kont // SQL specific compilation info
{ db_kontext * active_kontext_list;
  const table_all * kont_ta;   // additonal special single-table kontext
  t_scope    * active_scope_list;
  t_comp_phase phase;
  int expr_counter;
  int aggr_counter[5];
  t_agr_dynar * agr_list;  // valid list of aggregates in current t_query_specif
  BOOL aggr_in_expr;
  t_query_expr * curr_qe;  // used by compilation of aggregate functions
  int atomic_nesting;
  BOOL volatile_var_ref;   // parameter, variable or other volatile var referenced (local use: optimizing subqueries)
  BOOL dynpar_referenced, embvar_referenced;
  BOOL instable_var;
  int in_procedure_compilation;  // counts nested procedures and functions
  BOOL min_max_optimisation_enabled;
  int column_number_for_value_keyword;  // -1 when VALUE cannot be used
//  t_idname object_name;       // view or cursor name
  bool from_debugger;      // compileg expression evaluated in the debugger
  tobjnum compiled_psm;    // used when a handler is declared - the container objnum is necessary

  t_sql_kont(void)
  { active_kontext_list=NULL;  kont_ta=NULL;  active_scope_list=NULL;
    phase=SQLPH_NORMAL;  agr_list=NULL;
    expr_counter=1;
    for (int i=0;  i<5;  i++) aggr_counter[i]=1;
    atomic_nesting=0;  in_procedure_compilation=0;
    min_max_optimisation_enabled=FALSE;
    dynpar_referenced=embvar_referenced=FALSE;
    column_number_for_value_keyword=-1;
    from_debugger=false;
    compiled_psm=NOOBJECT;
  }
};

class temp_sql_kont_add
// Temporary adding a kontext to the kontext list in a t_sql_kont.
{ t_sql_kont * list;
  bool added;
 public:
  temp_sql_kont_add(t_sql_kont * listIn, db_kontext * kont)
  { if (kont)
    { list=listIn;
      kont->next=list->active_kontext_list;  list->active_kontext_list=kont;
      added=true;
    }
    else added=false;  
  }
  ~temp_sql_kont_add(void)
  { if (added && list->active_kontext_list)  // may be NULL when compiling the INTO clause
      list->active_kontext_list=list->active_kontext_list->next; 
  }
};

class temp_sql_kont_replace
// Temporary replacing the kontext list in a t_sql_kont by a kontext.
{ t_sql_kont * list;  db_kontext * orig;
 public:
  temp_sql_kont_replace(t_sql_kont * listIn, db_kontext * kont)
  { list=listIn;
    orig=list->active_kontext_list;  kont->next=NULL;  list->active_kontext_list=kont;
  }
  ~temp_sql_kont_replace(void)
  { list->active_kontext_list=orig; }
};

BOOL compute_key(cdp_t cdp, const table_descr * tbdf, trecnum rec, dd_index * indx, tptr keyval, bool check_privils);
void stmt_expr_evaluate(cdp_t cdp, t_express * ex, t_value * res);
void expr_evaluate(cdp_t cdp, t_express * ex, t_value * res);
void qset_null(tptr val, int type, int size);
void qlset_null(t_value * val, int type, int size);
void qset_max(tptr val, int type, int size);
// used by SQLCOMP:
void convert_value(CIp_t CI, t_express * op, int type, t_specif specif);
void sql_primary(CIp_t CI, t_express * * pres);
void sql_expression(CIp_t CI, t_express * * pex);
void WINAPI sql_expression_entry(CIp_t CI);
void search_condition(CIp_t CI, t_express * * pex);
void top_query_expression(CIp_t CI, t_query_expr * * qe);
void sql_selector(CIp_t CI, t_express ** pres, bool value_sel, bool limit_db_kontext);
void sql_view(CIp_t CI, t_query_expr * * qe);
void add_type(CIp_t CI, t_express * op1, const t_express * op2);
void binary_conversion(CIp_t CI, int oper, t_express * op1, t_express * op2, t_express * res);
void binary_oper(cdp_t cdp, t_value * val1, t_value * val2, t_compoper oper);
void sqlbin_expression(CIp_t CI, t_express * * pex, BOOL negate);
void WINAPI table_def_comp_link(CIp_t CI);

BOOL extract_string_condition(cdp_t cdp, t_expr_dynar * post_conds, int elnum, char * val, int lengthlimit);

BOOL sort_cursor(cdp_t cdp, dd_index * indx, t_optim * opt, t_mater_ptrs * pmat, trecnum limit, db_kontext * kont, trecnum limit_count = NORECNUM, trecnum limit_offset = 0);
BOOL create_curs_sort(cdp_t cdp, dd_index * indx, t_query_expr * qe, t_optim * opt, trecnum limit_count, trecnum limit_offset, bool distinct);
BOOL write_elem(cdp_t cdp, t_query_expr * qe, int elemnum, table_descr * dest_tbdf, trecnum recnum, tattrib attr);
BOOL curs_mater_sort(cdp_t cdp, dd_index * indx, t_query_expr * qe, t_optim * opt, ttablenum mattab, trecnum limit_count, trecnum limit_offset);

BOOL attr_access(db_kontext * akont, int elemnum,
 ttablenum & tabnum, tattrib & attrnum, trecnum & recnum, elem_descr * & attracc);
void attr_access_light(const db_kontext *& akont, int & elemnum, trecnum & recnum);
void attr_value(cdp_t cdp, db_kontext * kont, int elemnum, t_value * res, int type, t_specif specif, t_express * indexexpr);
BOOL write_value_to_attribute(cdp_t cdp, t_value * res, table_descr * dest_tbdf, trecnum recnum, int attrnum, t_express * indexexpr, int stype, int dtype);
t_optim * alloc_opt(cdp_t cdp, t_query_expr * qe, t_order_by * order_In, bool user_order);
BOOL compile_query(cdp_t cdp, tptr source, t_sql_kont * sql_kont,
                   t_query_expr ** pqe, t_optim ** popt);
void supply_marker_type(CIp_t CI, t_expr_dynpar * dynpar, int type, t_specif specif, t_parmode use);
void get_dp_value(cdp_t cdp, int dpnum, t_value * res, int desttype, t_specif destspecif);
void set_dp_value(cdp_t cdp, int dpnum, t_value * val, int srctype);
void get_emb_value(cdp_t cdp, const char * name, t_value * res);
void set_emb_value(cdp_t cdp, const char * name, t_value * val);
t_routine * get_stored_routine(cdp_t cdp, tobjnum objnum, t_zcr * required_version, BOOL complete);
BOOL is_in_admin_mode(cdp_t cdp, tobjnum objnum, WBUUID schema_uuid);
BOOL set_admin_mode  (cdp_t cdp, tobjnum objnum, WBUUID schema_uuid);
BOOL clear_admin_mode(cdp_t cdp, tobjnum objnum, WBUUID schema_uuid);

void mark_admin_mode_list(void);
BOOL sql_exception(cdp_t cdp, int num);
void set_default_value(cdp_t cdp, t_value * res, const attribdef * att, table_descr * tbdf);
BOOL resolve_access(db_kontext * akont, int elemnum, db_kontext *& _kont, int & _elemnum);
t_optim * optimize_qe(cdp_t cdp, t_query_expr * qe);
d_table * WINAPI create_d_curs(cdp_t cdp, t_query_expr * qe);
BOOL WINAPI compile_constrains(cdp_t cdp, const table_all * ta, table_descr * td);
int find_index_for_key(cdp_t cdp, const char * key, const table_all * ta, const ind_descr * index_info = NULL, dd_index_sep * ext_indx = NULL);
void add_implicit_constraint_names(table_all * ta);
#define O_VALPAR  7
#define O_REFPAR  8
#define O_FLEXPAR 20

struct t_ivcols  // like ctlg_attr_descr
{ char name[ATTRNAMELEN+1];
  uns8 type;
  uns8 length;
  uns8 order;
};
extern const t_ivcols ctlg_type_info[];
void fill_type_info(cdp_t cdp, table_descr * tbdf);
extern const t_ivcols ctlg_column_privileges[];
extern const t_ivcols ctlg_columns[];
extern const t_ivcols ctlg_foreign_keys[];
extern const t_ivcols ctlg_primary_keys[];
extern const t_ivcols ctlg_special_columns[];
extern const t_ivcols ctlg_statistics[];
extern const t_ivcols ctlg_table_privileges[];
extern const t_ivcols ctlg_tables[];
extern const t_ivcols ctlg_procedures[];
extern const t_ivcols ctlg_procedure_columns[];
void ctlg_fill(cdp_t cdp, table_descr * tbdf, unsigned ctlg_type);

#define OBT_NONE      0  // object not accessible from the PSM
#define OBT_CONST     1
#define OBT_SPROC     4
#define OBT_INFOVIEW 16  
typedef struct { /* beginning of the description of any object */
  uns8 categ;
               } anyobj;
typedef struct { /* description of a constant */
  uns8 categ;
  basicval0 value;
  int  type;
               } constobj;
typedef struct {
  uns8 categ;
  uns8 dtype;
               } sspar;
typedef struct { /* description of a standard procedure/function - on client and server sides */
  uns8 categ;
  uns16 sprocnum;
  //uns16 xxlocalsize;  /* never is more for std. PF */
  sspar * pars;
  uns8 retvaltype;  /* 0 iff no return value, cannot be composed */
               } sprocobj;

struct infoviewobj  // (parametrized) standard information view
{ uns8 categ;
  uns16 ivnum;
  const t_ivcols * cols;
  sspar * pars;  // NULL when the view has no parameters
};

union object    /* various object categories */
{ anyobj   any;
  constobj cons;
  sprocobj  proc;
  infoviewobj iv;
};
struct sobjdef         /* object definition */
{ tobjname name;       /* object's name */
  object * descr;      /* pointer to the description of the object */
};

const char * get_function_name(int fcnum);
void call_gen(CIp_t CI, sprocobj * sb, t_expr_funct * fex);
void special_function(CIp_t CI, t_expr_funct * fex);
BOOL same_expr(const t_express * ex1, const t_express * ex2);
void WINAPI sql_statement_1(CIp_t CI);
void tb_attr_value(cdp_t cdp, table_descr * tbdf, trecnum recnum, int elemnum, int type, t_specif specif, t_express * indexexpr, t_value * res);

void analyse_variable_type(CIp_t CI, int & tp, t_specif & specif, t_express ** pdefval);
void WINAPI default_value_link(CIp_t CI);
#define SEARCH_CASE_SENSITIVE   1

inline t_rscope * get_rscope(cdp_t cdp, int level)
{ if (level==GLOBAL_DECLS_LEVEL) return cdp->global_rscope;
  t_rscope * rscope = cdp->rscope;
  while (rscope && rscope->level!=level) rscope=rscope->next_rscope;
  return rscope;
}

inline tptr variable_ptr(cdp_t cdp, int level, int offset)
{ t_rscope * rscope = get_rscope(cdp, level);
  if (!rscope) return NULL;  // local variable referenced in a select-expression used in a cursor outside the scope
  return rscope->data + offset; 
}
 
void get_sys_var_value(cdp_t cdp, int sysvar_ident, t_value * val);
BOOL set_sys_var_value(cdp_t cdp, int sysvar_ident, t_value * val);
BOOL WINAPI exec_process(cdp_t cdp, char * module, char * params, BOOL wait);
void mark_trace_logs(void);
BOOL Repl_appl_shared(cdp_t cdp, const char *Appl);

#define SYSVAR_ID_ROWCOUNT          0
#define SYSVAR_ID_FULLTEXT_WEIGHT   1
#define SYSVAR_ID_PLATFORM          2
#define SYSVAR_ID_SQLCODE           3
#define SYSVAR_ID_SQLSTATE          4
#define SYSVAR_ID_FULLTEXT_POSITION 5
#define SYSVAR_ID_WAITING           6
#define SYSVAR_ID_SQLOPTIONS        7
#define SYSVAR_ID_LAST_EXC          8
#define SYSVAR_ID_ROLLED_BACK       9
#define SYSVAR_ID_ACTIVE_RI        10
#define SYSVAR_ID_ERROR_MESSAGE    11
#define SYSVAR_ID_IDENTITY         12
#define SYSTEM_VARIABLE_COUNT 13
extern t_scope system_scope;
#define SYSTEM_VAR_LEVEL -2

extern const char sqlexception_class_name[], sqlwarning_class_name[], not_found_class_name[];

////////////////////////// sql interface //////////////////////////////////
#define MAX_INDPARTVAL_LEN 130  // limit on single value size in the key

sql_statement * sql_submitted_comp(cdp_t cdp, tptr sql_statements, t_scope * active_scope = NULL, bool mask_errors = false);
t_scope * create_scope_list_from_run_scopes(t_rscope *& rscope);
BOOL sql_exec_direct(cdp_t cdp, tptr source);
BOOL sql_open_cursor(cdp_t cdp, tptr curdef, tcursnum & cursnum, cur_descr ** pcd, bool direct_stmt);
tcursnum open_stored_cursor(cdp_t cdp, tobjnum cursobj, cur_descr ** pcd, bool direct_stmt);
BOOL sql_exec_top_list(cdp_t cdp, sql_statement * so);
BOOL sql_exec_top_list_res2client(cdp_t cdp, sql_statement * so);
//int WINAPI compile_table_to_all(cdp_t cdp, const char * defin, table_all * pta);
void WINAPI index_expr_comp2(CIp_t CI);
void WINAPI check_comp_link(CIp_t CI);
int compile_index(cdp_t cdp, const char * key, const table_all * ta, const char * name, int index_type, bool has_nulls, dd_index * result);
table_descr * make_td_from_ta(cdp_t cdp, table_all * pta, ttablenum tbnum);
int WINAPI partial_compile_to_ta(cdp_t cdp, ttablenum tabnum, table_all * ta);
void get_table_schema_name(cdp_t cdp, ttablenum tabnum, char * schemaname);
void mark_fulltext_kontext_list(void);
void fulltext_insert(cdp_t cdp, const char * ft_label, t_docid docid, unsigned startpos, unsigned bufsize, const char * buf);
void fulltext_docpart_start(cdp_t cdp, const char * ft_label, t_docid docid, unsigned position, const char * relative_path);
trecnum create_object(cdp_t cdp, const tobjname name, const WBUUID schema_uuid, const char * source, tcateg categ, BOOL limited);
BOOL create_application_object(cdp_t cdp, const char * name, tcateg categ, BOOL limited, tobjnum * objnum);
void fill_recent_log(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds);
void fill_log_reqs(cdp_t cdp, table_descr * tbdf);
void trace_sql_statement(cdp_t cdp, const char * request);
void display_value(cdp_t cdp, int type, t_specif specif, int length, const char * val, char * buf);
BOOL exec_direct(cdp_t cdp, char * stmt, t_value * resptr = NULL);
BOOL schema_uuid_for_new_object(cdp_t cdp, const char * specified_schema_name, WBUUID uuid);
t_convert get_type_cast(int srctp, int desttp, t_specif srcspecif, t_specif destspecif, bool implicit);
void exec_stmt_list(cdp_t cdp, sql_statement * so);
BOOL try_handling(cdp_t cdp);
void make_kontext_snapshot(cdp_t cdp);
void wb_error_to_sqlstate(int error_code, char * except_name);
BOOL sqlstate_to_wb_error(const char * sqlstate, int * error_code);
BOOL may_have_handler(int errnum, int in_trigger);
enum t_condition_category { CONDITION_COMPLETION, CONDITION_HANDLABLE, CONDITION_ROLLBACK, CONDITION_CRITICAL, CONDITION_HANDLABLE_NO_RB };
t_condition_category condition_category(int sqlcode);
bool find_table_in_qe(t_query_expr * qe, ttablenum tabnum);
CFNC DllKernel int WINAPI cmp_str(const char * ss1, const char * ss2, t_specif specif);
BOOL var_search(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attr, t_mult_size index,
       t_varcol_size start, int flags, wbcharset_t charset, int pattlen, char * pattern, uns32 * pos);
BOOL Search_in_blockA(const char * block, uns32 blocksize,
                      const char * next, const char * patt, int pattsize,
                      int flags, wbcharset_t charset, uns32 * respos);
BOOL Search_in_blockW(const wuchar * block, uns32 blocksize,
                      const wuchar * next, const wuchar * patt, int pattsize,
                      int flags, wbcharset_t charset, uns32 * respos);
void get_schema_and_name(CIp_t CI, tobjname name, tobjname schema);
tptr get_default_value(CIp_t CI);
tptr read_check_clause(CIp_t CI, uns8 & co_cha);
void compile_ext_object(CIp_t CI, sql_stmt_call ** pres, extension_object * obj, bool value_required, HINSTANCE hInst);
bool get_routine_name(const t_scope * sscope, tobjname name);
void process_rollback_conditions_by_cont_handler(cdp_t cdp);
                                                            
__forceinline void Fil2Mem_convert(t_value * res, tptr destptr, int type, t_specif specif)
// Converts a value after copying from FIL to t_value.
{ if (IS_NULL(destptr, type, specif)) 
    res->set_null();
  else // not NULL
  { switch (type)
    { case ATT_BINARY: res->length=specif.length;  break;  // must define res->length
      case ATT_STRING:  // must define res->length
        if (specif.wide_char) res->length=2*(int)wuclen((wuchar*)destptr);
        else                  res->length=(int)strlen(destptr);   break;
      case ATT_INT16:  res->intval=(sig32)(short)res->intval;  break; // must convert to sig32
      case ATT_INT8:   res->intval=(sig32)(sig8 )res->intval;  break; // must convert to sig32
      case ATT_BOOLEAN:res->intval=(sig32)res->strval[0];  break; // must convert to sig32
      case ATT_CHAR:  // must append the terminating 0 
        if (specif.wide_char) { ((wuchar*)res->strval)[1]=0;  res->length=2; } 
        else                  { res->strval[1]=0;             res->length=1; } 
        break;
      case ATT_MONEY:   // must convert to sig64
        *(uns16*)(res->strval+6) = (res->strval[5] & 0x80) ? 0xffff : 0;  break;
    }
  }
}

__forceinline void Fil2Mem_copy(char * destptr, const char * srcptr, int type, t_specif specif)
// Copies and converts a non-heap value from FIL to memory. Used when loading rscope values.
{ switch (type)
  { case ATT_BINARY: memcpy(destptr, srcptr, specif.length);  break;
    case ATT_STRING:
      if (specif.wide_char) 
      { int length=2*(int)wuclen((wuchar*)srcptr);
        if (length>specif.length) length=specif.length;
        memcpy(destptr, srcptr, length);  *(wuchar*)(destptr+length)=0;
      }
      else strmaxcpy(destptr, srcptr, specif.length+1);
      break;
    case ATT_CHAR:  // must append the terminating 0 
      if (specif.wide_char) 
        { *(wuchar*)destptr = *(wuchar*)srcptr;  *(wuchar*)(destptr+2)=0; } 
      else                  
        { *destptr = *srcptr;  *(destptr+1)=0; } 
      break;
    default:
      memcpy(destptr, srcptr, tpsize[type]);  break;
  }
}

struct t_ft_kontext;
BOOL tb_write_vector(cdp_t cdp, table_descr * tbdf, trecnum recnum, const t_vector_descr * vector, BOOL newrec, t_tab_trigger * ttr);
bool create_fulltext_system(cdp_t cdp, const char * ft_label, const char * info, 
       const char * fulltext_object_name = NULL, const char * fulltext_schema_name = NULL, int language = -1, bool with_substructures = true, bool separated = false, bool bigint_id = false);
bool ft_destroy(cdp_t cdp, const char * ft_label, const char * fulltext_object_name = NULL, const char * fulltext_schema_name = NULL);
t_ft_kontext * get_fulltext_kontext(cdp_t cdp, const char * ft_label, const char * fulltext_object_name = NULL, const char * fulltext_schema_name = NULL);
t_fulltext2 * get_fulltext_system(cdp_t cdp, tobjnum ftx_objnum, WBUUID * ftx_schema_id);
void update_ft_kontext(cdp_t cdp, t_fulltext * ftdef);
void mark_fulltext_list(void);
void ft_purge(cdp_t cdp, const char * fulltext_object_name, const char * fulltext_schema_name);
void refresh_index(cdp_t cdp, const t_fulltext * ft);
int  backup_ext_ft(cdp_t cdp, const char * fname, bool nonblocking, bool reduce);
bool conv_s2w(const char * from, wuchar * dest, int len, wbcharset_t charset) ;

void set_pmat_position(cdp_t cdp, t_query_expr * qe, trecnum recnum, t_mater_ptrs * pmat);

const void * _null_value(int tp);  // for fast access when inserting a record

inline void t_vector_descr::get_column_value(cdp_t cdp, const t_colval * colval, // returning value of colval
                 table_descr * tbdf, const attribdef * att, // AX: att==tbdf->attrs+colval->attr
                 const char * & src, unsigned & size, // column value and its size in bytes
                 unsigned & offset,  // offset of the variable-length value in bytes
                 t_value & val)  // temporary store for calculated value, must be destructed or released later
                   const
{ offset=0;
  if (colval->expr!=NULL && !hide_exprs)
  { expr_evaluate(cdp, colval->expr, &val);
    val.load(cdp);
    if (val.is_null()) qlset_null(&val, att->attrtype, TYPESIZE(att));  
    src=val.valptr(); // must load indir first!!
    size=val.length;
  }
  else if (colval->dataptr==DATAPTR_NULL)
  { src=(const char *)_null_value(att->attrtype);  
    size=0;
  }
  else if (colval->dataptr==DATAPTR_DEFAULT)
    if (att->defvalexpr==NULL || (cdp->ker_flags & KFL_DISABLE_DEFAULT_VALUES)) // DEFAULT is NULL
    { src=(const char *)_null_value(att->attrtype);  
      size=0;
    }
    else // take the default value specified in the definition of the table
    { set_default_value(cdp, &val, att, tbdf);
      src=val.valptr();  // may be NULL if default value not specified // must load indir first!!
      size=val.length;
    }
  else // take the value specified by colval->dataptr (which may be offset of vector->vals)
  { src=vals ? vals+(size_t)colval->dataptr : (const char *)colval->dataptr; 
    if (IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype) && !(cdp->ker_flags & KFL_EXPORT_LOBREFS))
    { size=*colval->lengthptr;
      if (colval->offsetptr) offset=*colval->offsetptr;
    }
    // for ATT_BINARY type size has been defined before
  }
}

extern const WBUUID tNONEAUTOR;
struct t_varval_list  // the list of values of a variable-length multiattribute
{ t_varcol_size length;
  char * val;
};

/////////////////////////////////// packet writer ///////////////////////////////////////////
extern const t_colval default_colval;

class t_packet_writer  // support for inserting the whole records into a table
{ cdp_t cdp;
  table_descr * tbdf;
  int attrcnt;  // may be from cursor!
 // holding vars:
  t_colval * colvals;  
  char * vals;
  t_vector_descr default_deleted_vector;
  t_vector_descr vector;

 public:
  bool extlobrefs;
  t_packet_writer(cdp_t cdpIn, table_descr * tbdfIn, int attrcntIn) : default_deleted_vector(&default_colval), vector(NULL)
  { cdp=cdpIn;  tbdf=tbdfIn;  attrcnt=attrcntIn;
    colvals = NULL;  vals = NULL;
    default_deleted_vector.delete_record=true;
    vector.colvals=colvals=(t_colval*)corealloc(attrcnt*sizeof(t_colval), 71);
    if (colvals) colvals[attrcnt-1].attr=NOATTRIB;  // terminator
    extlobrefs=false;
  }
  ~t_packet_writer(void)
    { corefree(vals);  corefree(colvals); }

  int get_space_in_main_buffer(tattrib attr)
  { attribdef * att = tbdf->attrs+attr;
    if (att->attrmult == 1)
      if (IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype) && !extlobrefs) 
           return sizeof(t_varcol_size);
      else return typesize(att);
    else return sizeof(t_mult_size);
  }
  bool allocate_main_buffer(int recsize)
 // allocate the buffer for record values and define pointers in the vector:
  { if (!colvals) return false;  // check for error in the construcotr
    vals=(char*)corealloc(recsize, 71);
    return vals!=NULL;
  }
  void store_column_reference(int column, tattrib attr, unsigned & recsize)
  { colvals[column].attr=attr;
    colvals[column].expr=NULL;  colvals[column].offsetptr=NULL;  colvals[column].table_number=0;  colvals[column].index=NULL;
    attribdef * att = tbdf->attrs+attr;
    int tpsz=typesize(att);
    int tp=att->attrtype;
    if (att->attrmult == 1)
      if (IS_HEAP_TYPE(tp) || IS_EXTLOB_TYPE(tp) && !extlobrefs)
        { colvals[column].dataptr=NULL;  colvals[column].lengthptr=(t_varcol_size*)(vals+recsize);  recsize+=sizeof(t_varcol_size); }
      else
        { colvals[column].dataptr=vals+recsize;  colvals[column].lengthptr=NULL;  recsize+=tpsz; }
    else  
      { colvals[column].dataptr=NULL;  colvals[column].lengthptr=(t_varcol_size*)(vals+recsize);  recsize+=sizeof(t_mult_size); }
  }
  t_colval * get_colval(int column)
    { return &colvals[column]; }

  void release_multibuf(int column, int tp)
  { if (IS_HEAP_TYPE(tp))  // not used for EXTLOBs
    { t_mult_size count = *(t_mult_size*)colvals[column].lengthptr;
      t_varval_list * list = (t_varval_list*)colvals[column].dataptr;
      if (list) 
      { for (t_varval_list * plist = list;  count;  count--, plist++)
           corefree(plist->val);
        corefree(list);
      }
    }
    else corefree(colvals[column].dataptr);  
  }
  void * allocate_multibuf(int column, t_mult_size count, int itemsize, bool init)
  { *(t_mult_size*)colvals[column].lengthptr = count;
    void * buf;
    colvals[column].dataptr = buf = corealloc(count*itemsize, 71); 
    if (init && buf) // contains pointers, must be init.
      memset(buf, 0, count*itemsize);
    return buf;
  }
  void release_heapbuf(int column)  // does not store NULL to the buffer pointer, new buffer must be allocated before any chance to error
    { corefree(colvals[column].dataptr); }
  char * allocate_heap_buffer(int column, int length)
  { *colvals[column].lengthptr = length;
    return (char*)(colvals[column].dataptr=corealloc(length, 71));
  }

  trecnum insert_deleted_record(void)
  { int saved_flags = cdp->ker_flags;
    cdp->ker_flags |= KFL_DISABLE_INTEGRITY | KFL_DISABLE_REFCHECKS;  // disable immediate checks for the deleted record
    trecnum rec=tb_new(cdp, tbdf, TRUE, &default_deleted_vector);
    cdp->ker_flags = saved_flags;
    return rec;
  }
  trecnum insert_record(void)
    { return tb_new(cdp, tbdf, TRUE, &vector, NULL); }
  bool write_record(trecnum recnum)
    { return tb_write_vector(cdp, tbdf, recnum, &vector, FALSE, NULL)==TRUE; }
};

