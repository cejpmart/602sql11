// wbdefs.h - common definitions for server and client

// configuring the compilers
#ifdef _MSC_VER
#pragma warning(disable:4200)  // zero-sized array in union/struct
#pragma warning(disable:4018)  // signed/unsigned mismatch
#pragma warning(disable:4100)  // unreferenced formal parameter (L4)
#pragma warning(disable:4127)  // conditional expression is constant (L4)
#pragma warning(disable:4310)  // cast truncates constant value (L4)
#pragma warning(disable:4355)  // 'this' : used in base member initializer list (L4)
#pragma warning(disable:4512)  // assignment operator could not be generated (L4)
#pragma warning(disable:4710)  // ... not inlined
//#pragma warning( disable : 4200 4201 4237 4245 4514 4244 4305 4306)
#pragma warning(disable:4251)  // class with DLL interface has a member without the DLL interface
#pragma warning(3 : 4121 4505)
#pragma warning(disable:4996)  // using insecure functions like strcpy, strcat,... (#define _CRT_SECURE_NO_DEPRECATE)
#endif

#ifdef LINUX
#if WBVERS>=1100
#include "symvers11.h"
#else
#include "apsymbols.h"
#endif
#else // WINS
#endif

#ifndef GENERAL_DEF
#define GENERAL_DEF  // signal for general.h that it is included as a part of other header file
#include "general.h"
#endif   // !GENERAL_DEF

#include "wbvers.h"  // the version-dependent defines and configuration

#ifndef WBDEFS
#define WBDEFS
// Doplnky ke general.h:
/*************** Atributy systemovych tabulek TABTAB A OBJTAB ***************/
#define SYSTAB_PRIVILS_ATR 2

/******************** zde konci uzivatelske kernel.h ************************/
#define CD_DIALOG_HELP  255   /* universal in dialogs */

#ifdef __BORLANDC__
#define __try        try
#define _tell        tell
#define _diskfree_t  diskfree_t
#define _find_t      find_t
#endif
#ifdef __WATCOMC__
#define _tell        tell
#endif

#ifdef WINS
#undef  FreeProcInstance
#define FreeProcInstance(x)
#endif

#define GlobalPtrHandle(lp) ((HGLOBAL)GlobalHandle(lp))

#ifdef UNIX
#define EMPTY_ARRAY 0
#if WBVERS<900
#define USE_KERBEROS
#endif
#else
#define EMPTY_ARRAY 
#endif

#ifdef WINS
typedef HANDLE FHANDLE;   // file handle
typedef DWORD    t_io_oper_size;  // type of the parameter of the read and write operations
#else
typedef unsigned t_io_oper_size;
#endif

#define OUT_OF_CURSOR  3   /* interni pouziti (returned only by Recstate) */
#if WBVERS>=950
#define MAX_TABLE_COLUMNS 65535
#else
#define MAX_TABLE_COLUMNS 255
#endif
#define WB_DMS_ID     "602SQL8"  // ODMA Data Management System ID - key in the registry under HKEY_Classes_Root\ODMA32

#define LINK_MASK     0x7f
#define IS_STARTER    0x40
typedef uns32 timestamp;

enum { APLOBJ_NAME=1, APLOBJ_CATEG, APLOBJ_UUID, APLOBJ_DEFIN, APLOBJ_FLAGS, APLOBJ_FOLDER, APLOBJ_MODIF };

#define NOT_LINKED ((uns16)-2)
#define NW_OBJ_LEN     48

#define PASSWORD_CYCLES 10000

extern byte tNONEREAL[8];
#define NULLREAL (*(double*)tNONEREAL)
extern char NULLSTRING[2];
#define MONEY_SIZE  6
#define EXTLOBSIZE sizeof(uns64)

#define IS_NULLMONEY(m) (((m)->money_hi4==NONEINTEGER) && !(m)->money_lo2)

struct  xcd_t;
typedef xcd_t * xcdp_t;
struct  t_connection;
typedef t_connection * odbccdp_t;
struct  cd_t;
typedef cd_t * cdp_t;
struct  run_state;
struct t_express;

#define MAX_OBJECT_NAME_LEN  255   // for 602SQL and ODBC objects
typedef char tgenobjname[MAX_OBJECT_NAME_LEN+1];

#define CATEG2SYSTAB(cat) (ttablenum)(((cat) & CATEG_MASK)==CATEG_TABLE ?\
  TAB_TABLENUM :\
  (cat)==CATEG_USER || (cat)==CATEG_GROUP ? USER_TABLENUM : OBJ_TABLENUM)

extern "C" {
  DllKernel uns8  WINAPI cUPCASE  (uns8 ch); /* converts CS letters to upper-case */
  DllKernel BOOL  WINAPI UPLETTER(uns8 ch);
  DllKernel BOOL  WINAPI SYMCHAR (uns8 ch); /* is UPLETTER or digit ? */
  DllKernel BOOL  WINAPI is_string(unsigned tp);
  DllKernel BOOL  WINAPI speciflen(unsigned tp);
  DllKernel void  WINAPI small_string(tptr s, BOOL captfirst);
  DllKernel BOOL  WINAPI specname(cdp_t cdp, const char * p);
  DllKernel void  WINAPI copy_name(cdp_t cdp, char * dest, const char * name);
}
extern const uns8 csconv[256];
extern uns8 letters[128];
#define mcUPCASE(c) csconv[c]
#define mUPLETTER(ch) (((unsigned)ch<128) ? ((ch>='A') && (ch<='Z') || (ch=='_')) : \
                           letters[(unsigned)ch-128])

#include <string.h>
#include <stdio.h>

#define MAX_INDEX_PARTS    8   // used by client, unlited on th server side
#define MAX_KEY_LEN     1000
#define REC_REMAP_SIZE    70   /* size of recnum remap array */
#define SELFPTR           ((ttablenum)-1)
#define RELOC_MARK      0xdf
#define INDX_MAX          24
#define ATTRNAMELEN       31  /* the max. number of characters in an attribute name */
#define UUID_SIZE         12
typedef uns8 WBUUID[UUID_SIZE];
#define IDNAME_LEN        31          // length of identifiers on the server (exception: embedded variables use NAMELEN/tname)
typedef char t_idname[IDNAME_LEN+1];  // type of identifiers on the server: labels, savepoints, locally declared objects (exceptions, cursors, variables, routines)

#define SYSTEM_TABLE(tbnum) ((tbnum)<=REL_TABLENUM && (sig32)(tbnum)>=TAB_TABLENUM)
 /* Must recognize cursor numbers and temporary tables! */
#define REL_CLASS_RI     0 // value of the CLASSNUM column in __RELTAB system table: referential integrity relation
#define REL_CLASS_TRIG   1 // value of the CLASSNUM column in __RELTAB system table: relation between a table and its trigger
#define REL_CLASS_DOM    2 // value of the CLASSNUM column in __RELTAB system table: relation between a table and a domain used in it
#define REL_CLASS_FTX    3 // value of the CLASSNUM column in __RELTAB system table: relation between a fulltext system and a table
/********************** the attribute types and its codes ********************/
#define ATT_UNTYPED   23
#define ATT_MARKER    24
#define ATT_INDNUM    25
#define ATT_INTERVAL  26
#define ATT_NIL       27  /* any value distinct from other ATTs */
#define ATT_FILE      28
#define ATT_TABLE     29
#define ATT_CURSOR    30
#define ATT_DBREC     31
#define ATT_VARLEN    32
#define ATT_STATCURS  33
#define ATT_UNUSED    34
#define IS_STRING(tp)    ((tp) == ATT_STRING)
#define SPECIFLEN(tp)    ((tp) == ATT_STRING || (tp) == ATT_BINARY)
#define IS_CHAR_TYPE(tp) ((tp) == ATT_STRING || (tp) == ATT_CHAR || (tp) == ATT_TEXT || (tp) == ATT_EXTCLOB)
#define TYPESIZE(att) (SPECIFLEN(att->attrtype) ? att->attrspecif.length : tpsize[att->attrtype])
extern uns8 tpsize[];  /* located in comm0 and dataproc */
#define ATT_ODBC_NUMERIC   40
#define ATT_ODBC_DECIMAL   41
#define ATT_ODBC_TIME      42
#define ATT_ODBC_DATE      43
#define ATT_ODBC_TIMESTAMP 44
#define IS_HEAP_TYPE(tp)   ((tp)>=ATT_FIRST_HEAPTYPE && (tp)<=ATT_LAST_HEAPTYPE)
#define IS_EXTLOB_TYPE(tp) ((tp)==ATT_EXTCLOB || (tp)==ATT_EXTBLOB)
#define IS_ODBC_TYPE(tp)   ((tp)>=ATT_ODBC_NUMERIC   && (tp)<=ATT_ODBC_TIMESTAMP)
#define SPECIF_VARLEN 0xffff  // value of specif.length saying that the length (of LOB) was not specified explicitly

#define i64_precision  18
#define i32_precision   9
#define i16_precision   4
#define  i8_precision   2
#ifdef X64ARCH
#define handle_precision i64_precision 
#else
#define handle_precision i32_precision 
#endif

/* lock numbers */
#define RLOCK    0x80
#define WLOCK    0x40
#define TMPWLOCK 0x20  /* remove this lock at commit time */
#define TMPRLOCK 0x10  /* remove this lock at commit time */
#define ANY_LOCK 0xf0  // removes all types of locks

typedef struct
{ ttablenum tabnum;
  trecnum   recnum;
  tobjnum   usernum;
  uns8      locktype;
} lockinfo;

#define IS_BACK_END_CATEGORY(cat) ((cat)==CATEG_CURSOR || (cat)==CATEG_TRIGGER || (cat)==CATEG_PROC || (cat)==CATEG_SEQ || (cat)==CATEG_REPLREL || (cat)==CATEG_DOMAIN)
///////////////////////////////// memory ////////////////////////////////////
// Other functions are in wbapiex.h
typedef void CALLBACK t_print_info_line(const char * msg);
extern "C" {
  DllKernel void   WINAPI safefree(void * * px);
  DllKernel void * WINAPI corerealloc(void * x, unsigned y);
  DllKernel void   WINAPI core_disable(void);
  DllKernel void   WINAPI memmov(void * destin, const void * source, size_t size);
  DllKernel int    WINAPI total_used_memory(void);
#if WBVERS<=810
  DllKernel BOOL   WINAPI is_core_management_open(void);
#endif
  DllKernel void   WINAPI heap_test(void);
  DllKernel void   WINAPI PrintHeapState(t_print_info_line * print_line);
  DllKernel void * WINAPI aligned_alloc(size_t size, char ** ptr);
  DllKernel void   WINAPI aligned_free(void * alloc);
}

struct t_mem_info
{ uns32 size;
  uns32 items;
};  

bool create_memory_stats(t_mem_info mi[256]);

#define OW_CINT       1
#define OW_QBE        2
#define OW_COMPIL     3
#define OW_VDEF       4
#define OW_GENINC     5
#define OW_TABDEF     6
#define OW_FSED       7
#define OW_OBJDEF     8
#define OW_PTRS       9
#define OW_PREZ      10
#define OW_CURDEF    11
#define OW_IMPORT    12
#define OW_PGRUN     13
#define OW_WINDOWS   14
#define OW_CUROP     15
#define OW_CODE      16
#define OW_ASC       17
#define OW_PRINT     18
#define OW_CONSTRAIN 19
#define OW_FILESEL   20
#define OW_DIROP     21
#define OW_HELP      22
#define OW_MENU      23
#define OW_NETWORK   24

#define OW_WORM     101
#define OW_FRAMES   102
#define OW_TABLE    103
#define OW_REMAP    104
#define OW_BTREE    105
#define OW_RIGHT    106
#define OW_ANSW     107
#define OW_SEEK     108
#define OW_CURSOR   109
#define OW_MAINT    110
#define OW_MAINTAN  111
#define OW_HIST     112

#include "wbapiex.h"
//////////////////////////// dynar //////////////////////////////////////
class DllKernel t_dynar
{
protected:   
  unsigned elcnt;    /* number of array elements in use */
  unsigned arrsize;  /* number of allocated array elements */
  unsigned elsize;   /* array element size in bytes */
  unsigned step;     /* allocation step (number of new elements) */
  tptr elems;
 public:
  void * acc2(unsigned index);
  inline void * acc(unsigned index)
    { return (index < elcnt) ? elems+(index*elsize) : acc2(index); }
  inline void * acc0(unsigned index) const
    { return elems+(index*elsize); }
  void   minimize(void);
  int    find(void * pattern);
  inline int count(void) const   { return elcnt; }
  inline unsigned el_size(void) const { return elsize; }
  void   init(unsigned c_elsize, unsigned c_arrsize = 0, unsigned c_step = 1);
  inline t_dynar(unsigned c_elsize, unsigned c_arrsize = 0, unsigned c_step = 1)
    { init(c_elsize, c_arrsize, c_step); }
  inline void destruct(void)  
    { if (arrsize) { corefree(elems);  elems=NULL;  arrsize=elcnt=0; } }
  inline ~t_dynar(void)
    { destruct(); }
  BOOL   delet(unsigned index);
  BOOL   ord_delet(unsigned index);
  void * insert(unsigned index);
  inline void * next(void) { return acc(elcnt); }
  void init_elem(void * elptr);
  void drop(void);
  inline void clear(void) { elcnt=0; }
#ifdef SRVR
  void mark_core(void);
#else
  void mark_core(void) { }
#endif
/* Axioms:
  1. sizeof(*elems) == arrsize*elsize;
  2. arrsize >= elcnt;
  3. elems!=NULL iff arrsize!=0;
  4. < elems[elsize*elcnt] , elems[elsize*arrsize] ) is nulled;
*/
};

#define SPECIF_DYNAR(name,elemtype,step) class name : public t_dynar   \
{public:                                                          \
  inline elemtype * acc(unsigned index)                           \
    { return (elemtype *)t_dynar::acc(index); }                   \
  inline elemtype * acc0(unsigned index) const                    \
    { return (elemtype *)t_dynar::acc0(index); }                  \
  inline elemtype * acc2(unsigned index)                          \
    { return (elemtype *)t_dynar::acc2(index); }                  \
  inline elemtype * next(void)                                    \
    { return (elemtype *)t_dynar::next(); }                       \
  inline void init(unsigned c_arrsize = 0, unsigned c_step = step)\
    { t_dynar::init(sizeof(elemtype),c_arrsize,c_step); }         \
  inline name(unsigned c_arrsize = 0, unsigned c_step = step) :   \
    t_dynar(sizeof(elemtype),c_arrsize,c_step) {}                 \
}
/////////////////////////////// client - server communication ////////////////////////////
// dircomm values:
#define NORMAL_MSG                  0
#define INTRA_REQUEST_DATA          17
#define NONFINAL_MSG_PART_NO_LEN    19
#define MSG_PART_WITH_LEN           20
#define SERVER_DOWN_NOT_REPORTED    123
#define SERVER_DOWN_REPORTED        124

#pragma pack(1)
#define MIN_ANS_SIZE   24
struct answer_cb      /* the beginning of a typical answer */
{ uns16 return_code;
  uns8  flags;    // direct communication controlling value
  char  return_data[MIN_ANS_SIZE];
};
#define ANSWER_PREFIX_SIZE (sizeof(uns16)+sizeof(uns8))

struct request_frame {   /* request_frame */
  char  areq[1];
};

struct t_request_struct
{ uns8 comm_flag;
  request_frame req;
};
#pragma pack()

/**************************** table definition in table_all *****************************/
// flags stored in tabdef_flags (common for table_all and table_descr):
#define TFL_NO_JOUR      0x0001
#define TFL_UNIKEY       0x0002
#define TFL_ZCR          0x0004
#define TFL_TOKEN        0x0008
#define TFL_DETECT       0x0010
#define TFL_VARIANT      0x0020
#define TFL_REC_PRIVILS  0x0040
#define TFL_DOCFLOW      0x0080
#define TFL_REPL_DEL     0x0100
#define TFL_LUO          0x0200
#define TFL_EXCLUDE      0x0400

// table constrain type (stored in ind_descr.index_type and dd_index.ccateg):
#define INDEX_PRIMARY    1
#define INDEX_UNIQUE     2
#define INDEX_NONUNIQUE  3

// values of update_rule and delete_rule in forkey_constr:
#define REFRULE_NO_ACTION   1
#define REFRULE_SET_NULL    2
#define REFRULE_SET_DEFAULT 3
#define REFRULE_CASCADE     4

// constraint characteristics constants
#define COCHA_UNSPECIF          0
#define COCHA_IMM_DEFERRABLE    1
#define COCHA_IMM_NONDEF        2
#define COCHA_DEF               3

typedef enum { ATR_NEW=0, ATR_OLD, ATR_DEL, ATR_MODIF, ATR_MINOR_MODIF } tag_atr_state;
#define INDEX_NEW -1
#define INDEX_DEL -2

struct double_name // object name possibly prefixed by a schema name
{ tobjname schema_name; // "" iff schema not specified
  tobjname name;
};

SPECIF_DYNAR(name_dynar, double_name, 3);

struct part_desc
{ tattrib  colnum;   // column number iff the part is a column, 0 otherwise
  uns8     type;
  wbbool   desc;
  t_specif specif;   // total number of digits not stored.
  int      partsize; // total length of the part's value (redundant but makes using indicies faster)
  t_express * expr;  // the expression constituting the part of the index
};

struct ind_descr
{ tobjname constr_name;
  uns8  index_type;         // INDEX_PRIMARY, INDEX_UNIQUE or INDEX_NONUNIQUE
  bool  has_nulls;
  tptr  text;               /* expression text */
  uns8  co_cha;             // constraint characteristics
  int   state;              // INDEX_NEW, INDEX_DEL or the number of the original index
  inline ind_descr(void) { text=NULL; }
  ~ind_descr(void);        /* destructor for reftables called automatically */
  void mark_core(void);
};

struct check_constr
{ tobjname constr_name;
  tptr text;             /* expression text */
  uns8  co_cha;          // constraint characteristics
  uns8  state;           // altering state, type tag_atr_state
  int   column_context;  // -1 when CHECK is not a part of a column definition, column number otherwise.
  ~check_constr(void);
  void mark_core(void);
};

class index_dynar : public t_dynar
{
 public:
  inline ind_descr * acc(unsigned index)
    { return (ind_descr*)t_dynar::acc(index); }
  inline ind_descr * acc0(unsigned index)
    { return (ind_descr*)t_dynar::acc0(index); }
  inline const ind_descr * acc0(unsigned index) const
    { return (const ind_descr*)t_dynar::acc0(index); }
  BOOL delet(unsigned index);
  inline ind_descr * next(void)
    { return (ind_descr*)t_dynar::next(); }
  inline void init(unsigned c_arrsize = 0, unsigned c_step = 1)
    { t_dynar::init(sizeof(ind_descr),c_arrsize,c_step); }
  inline index_dynar(unsigned c_arrsize = 0, unsigned c_step = 1) :
    t_dynar(sizeof(ind_descr),c_arrsize,c_step) {}
  ~index_dynar(void);
};

struct forkey_constr
{ tobjname constr_name;
  tobjname desttab_name;
  tobjname desttab_schema;
  tptr  text;            /* expression text */
  tptr  par_text;        /* expression text or NULL iff using the primary key of the parent */
  int   loc_index_num;   // defined when creating ta, then moved to td
  uns8  update_rule, delete_rule;
  uns8  co_cha;          // constraint characteristics
  uns8  state;           // altering state, type tag_atr_state
  ~forkey_constr(void)
    { corefree(text);  corefree(par_text);  text=par_text=NULL; }
  void mark_core(void);
};

class check_dynar : public t_dynar
{
 public:
  inline check_constr * acc(unsigned index)
    { return (check_constr*)t_dynar::acc(index); }
  inline check_constr * acc0(unsigned index)
    { return (check_constr*)t_dynar::acc0(index); }
  inline const check_constr * acc0(unsigned index) const
    { return (const check_constr*)t_dynar::acc0(index); }
  BOOL delet(unsigned index);
  inline check_constr * next(void)
    { return (check_constr*)t_dynar::next(); }
  inline void init(unsigned c_arrsize = 0, unsigned c_step = 1)
    { t_dynar::init(sizeof(check_constr),c_arrsize,c_step); }
  inline check_dynar(unsigned c_arrsize = 0, unsigned c_step = 1) :
    t_dynar(sizeof(check_constr),c_arrsize,c_step) {}
  ~check_dynar(void);
};

class refer_dynar : public t_dynar
{
 public:
  inline forkey_constr * acc(unsigned index)
    { return (forkey_constr*)t_dynar::acc(index); }
  inline forkey_constr * acc0(unsigned index)
    { return (forkey_constr*)t_dynar::acc0(index); }
  inline const forkey_constr * acc0(unsigned index) const
    { return (const forkey_constr*)t_dynar::acc0(index); }
  BOOL delet(unsigned index);
  inline forkey_constr * next(void)
    { return (forkey_constr*)t_dynar::next(); }
  inline void init(unsigned c_arrsize = 0, unsigned c_step = 1)
    { t_dynar::init(sizeof(forkey_constr),c_arrsize,c_step); }
  inline refer_dynar(unsigned c_arrsize = 0, unsigned c_step = 1) :
    t_dynar(sizeof(forkey_constr),c_arrsize,c_step) {}
  ~refer_dynar(void);
};

typedef struct atr_all
{ char  name[ATTRNAMELEN+1];
  uns8  type;
  uns8  multi;
  wbbool  nullable;
  t_specif specif;        /* string size or destination table number */
  uns8  state;            /* altering state */
  uns8  orig_type;        /* original type */
#if WBVERS<=810
  uns8  own_check;        /* used by the table designer only: number of the own check rule */
#else
  wbbool expl_val;        // used by the table designer only: alt_value has been explicitly specified by the user
  wbbool fsexclude;
#endif
  tptr  defval;           /* pointer to default value text iff not NULL */
  tptr  alt_value;        /* value used when altering the table   */
  tptr  comment;          /* comment attached to the column */

  atr_all(void) : specif(0) 
  {
      alt_value=defval=comment=NULL; 
#if WBVERS>810
      fsexclude=false;
#endif
  }
  ~atr_all(void) { safefree((void**)&alt_value);  safefree((void**)&defval);  safefree((void**)&comment); }
} atr_all;

class attr_dynar: public t_dynar
{public:
  inline atr_all* acc(unsigned index)
    { return (atr_all*)t_dynar::acc(index); }
  inline atr_all* acc0(unsigned index)
    { return (atr_all*)t_dynar::acc0(index); }
  inline const atr_all* acc0(unsigned index) const
    { return (const atr_all*)t_dynar::acc0(index); }
  inline atr_all* next(void)
    { return (atr_all*)t_dynar::next(); }
  inline void init(unsigned c_arrsize = 0, unsigned c_step = 20)  // small c_step make compiling tables with many columns very slow
    { t_dynar::init(sizeof(atr_all),c_arrsize,c_step); }
  inline attr_dynar(unsigned c_arrsize = 0, unsigned c_step = 20) :
    t_dynar(sizeof(atr_all),c_arrsize,c_step) {}
  BOOL delet(unsigned index);
  ~attr_dynar(void);
  inline atr_all * insert(unsigned index)
    { return (atr_all*)t_dynar::insert(index); }
};

class table_all // table definition, CREATE TABLE and ALTER TABLE statements are compiled into it
{public:
  unsigned tabdef_flags; // 602SQL private flags
  BOOL     attr_changed; // any column changed (defined when compiling ALTER TABLE statement)
  tobjname selfname;     // table name
  tobjname schemaname;   // schema the table belongs to
  attr_dynar  attrs;  // list of columns, starts with the DELETED column
  name_dynar  names;  // list of destination table names for pointer type columns
  index_dynar indxs;  // list of indicies
  check_dynar checks; // list of check constrains
  refer_dynar refers; // list of foreign keys defined in the table
  BOOL     cascade;      // CASCADE flag, used only in ALTER statement
//#ifdef SRVR--pokus
  void mark_core(void);
//#endif
  table_all(void)
    { *schemaname=0; }
};

enum table_statement_types { CREATE_TABLE, ALTER_TABLE, CREATE_INDEX, CREATE_INDEX_COND, DROP_INDEX, DROP_INDEX_COND };

enum statement_types
{ SQL_STAT_NO, SQL_STAT_DROP,
  SQL_STAT_ALTER_TABLE,  SQL_STAT_CREATE_TABLE, 
  SQL_STAT_CREATE_INDEX, SQL_STAT_DROP_INDEX,
  SQL_STAT_CREATE_VIEW,  
  SQL_STAT_CREATE_ROUTINE, 
  SQL_STAT_GRANT,        SQL_STAT_REVOKE,
  SQL_STAT_INSERT,       SQL_STAT_DELETE, SQL_STAT_UPDATE, SQL_STAT_SELECT,
  SQL_STAT_BLOCK,        SQL_STAT_SET,    SQL_STAT_IF,     SQL_STAT_LOOP,
  SQL_STAT_LEAVE,        SQL_STAT_CALL,   SQL_STAT_RETURN,
  SQL_STAT_OPEN,         SQL_STAT_CLOSE,  SQL_STAT_FETCH,
  SQL_STAT_SIGNAL,       SQL_STAT_RESIGNAL,
  SQL_STAT_START_TRA,    SQL_STAT_COMMIT, SQL_STAT_ROLLBACK,
  SQL_STAT_SAPO,         SQL_STAT_REL_SAPO,
  SQL_STAT_CASE,         SQL_STAT_FOR,
  SQL_STAT_CREATE_TRIGGER, SQL_STAT_SET_TRANS,
  SQL_STAT_CREATE_SCHEMA,
  SQL_STAT_CREATE_SEQ,   SQL_STAT_CRE_DROP_SUBJ,
  SQL_STAT_CREATE_DOMAIN,SQL_STAT_CRE_FRONT_OBJ, SQL_STAT_RENAME_OBJECT,
  SQL_STAT_CREATE_FT
};

/******************************** d_table ***********************************/
/* flags used in aflags in d_table: */
#define ROWID_FLAG       1
#define ROWVER_FLAG      2
#define NOEDIT_FLAG      4
#define RI_SELECT_FLAG   0x10
#define RI_INSERT_FLAG   0x20
#define RI_UPDATE_FLAG   0x40
#define RI_REFERS_FLAG   0x80

struct d_attr
{ char  name[ATTRNAMELEN+1];
  uns8  type;
  uns8  multi;
  wbbool  nullable;
  t_specif specif;        /* string size or destination table number */
  wbbool  needs_prefix;
  uns8  prefnum;          /* prefix number iff needs_prefix       */
  tattrib unused1;        /* mapping cursor attributes to table attributes */
  uns8    unused2;        /* ... for updatable cursors */
  uns8  a_flags;          /* privilege, rowid, dependent flags */
  inline tattrib get_num(void) const
    { return *(tattrib*)name; }
  inline void set_num(tattrib num) 
    { *(tattrib*)name = num; }
  inline void base_copy(const d_attr * source, int num) 
  { memcpy(this, source, sizeof(d_attr));
    set_num(num);
  }
  inline void base_copy(const atr_all * source, int num) 
  { set_num(num);
    type=source->type;  multi=source->multi;  nullable=source->nullable;
    specif=source->specif;
  }
};

#if WBVERS>=950
struct d_attr_0  // from protocol version 0
{ char  name[ATTRNAMELEN+1];
  uns8  type;
  uns8  multi;
  wbbool  nullable;
  t_specif specif;        /* string size or destination table number */
  wbbool  needs_prefix;
  uns8  prefnum;          /* prefix number iff needs_prefix (protocol 0!) */
  uns8  unused1;          /* mapping cursor attributes to table attributes */
  uns8  unused2;          /* ... for updatable cursors */
  uns8  a_flags;          /* privilege, rowid, dependent flags */
};
#else
struct d_attr_1  // from protocol version 1
{ char  name[ATTRNAMELEN+1];
  uns8  type;
  uns8  multi;
  wbbool  nullable;
  t_specif specif;        /* string size or destination table number */
  wbbool  needs_prefix;
  uns8  prefnum;          /* prefix number iff needs_prefix (protocol 0!) */
  uns16 unused1;          /* mapping cursor attributes to table attributes */
  uns8  unused2;          /* ... for updatable cursors */
  uns8  a_flags;          /* privilege, rowid, dependent flags */
};
#endif

#define QE_IS_UPDATABLE   1
#define QE_IS_DELETABLE   2
#define QE_IS_INSERTABLE  4

struct d_table
{ tattrib attrcnt;   /* number of attributes */
  uns8  tabcnt;    /* number of prefixes and tables (0 for table descriptor) */
  uns16 tabdef_flags;/* flags derived from the table definition */
  uns16 updatable;  // combination of flags: QE_IS_UPDATABLE, QE_IS_DELETABLE, QE_IS_INSERTABLE
  tobjname selfname;
  tobjname schemaname;
  d_attr attribs[1];
};

#if WBVERS>=950
struct d_table_0  // from protocol version 0
{ uns8  attrcnt;   /* number of attributes (protocol 0!) */
  uns8  tabcnt;    /* number of prefixes and tables (0 for table descriptor) */
  uns16 tabdef_flags;/* flags derived from the table definition */
  uns16 updatable;  // combination of flags: QE_IS_UPDATABLE, QE_IS_DELETABLE, QE_IS_INSERTABLE
  tobjname selfname;
  tobjname schemaname;
  d_attr_0 attribs[1];
};
#else
struct d_table_1  // from protocol version 1
{ uns16  attrcnt;   /* number of attributes (protocol 1!) */
  uns8  tabcnt;    /* number of prefixes and tables (0 for table descriptor) */
  uns16 tabdef_flags;/* flags derived from the table definition */
  uns16 updatable;  // combination of flags: QE_IS_UPDATABLE, QE_IS_DELETABLE, QE_IS_INSERTABLE
  tobjname selfname;
  tobjname schemaname;
  d_attr_1 attribs[1];
};
#endif

#define FIRST_ATTR(td)       ((td)->attribs)
#define NEXT_ATTR(td,pattr)  ((pattr)+1)
#define ATTR_N(td,n)         ((td)->attribs+(n))
#define COLNAME_PREFIX(td,i) ((tptr)((td)->attribs+(td)->attrcnt)+2*i*(OBJ_NAME_LEN+1))
#define COUNTER_DEFVAL       (t_express*)(size_t)0x80000001LU

void client_deliver_messages(cdp_t cdp, BOOL waitcurs);
extern "C" {
DllKernel void set_table_def_invalid(ttablenum tabnum);         /* to messages */
DllKernel void  WINAPI xgettime(uns8 * ti_sec, uns8 * ti_min, uns8 * ti_hour);
DllKernel void  WINAPI xgetdate(uns8 * da_day, uns8 * da_mon, int  * da_year);
DllKernel void WINAPI strmaxcpy(char    * destin, const char    * source, size_t maxsize);
DllKernel void WINAPI wcsmaxcpy(wchar_t * destin, const wchar_t * source, unsigned maxchars);
DllKernel unsigned WINAPI strmaxlen(const char * str, unsigned maxsize);
DllKernel int      WINAPI my_stricmp(const char * str1, const char * str2);
DllKernel void WINAPI deliver_messages(cdp_t cdp, BOOL waitcurs);
DllKernel cdp_t WINAPI GetCurrTaskPtr(void);
DllKernel int   WINAPI compile_table_to_all(cdp_t cdp, const char * defin, table_all * pta);
DllKernel uns8  WINAPI small_char(unsigned char c);
DllKernel int WINAPI type_specif_size(int tp, t_specif specif);
#if WBVERS<=810
DllKernel void WINAPI md5_digest(uns8 * input, unsigned len, uns8 * digest);
#endif
}
DllKernel int WINAPI typesize(const d_attr * pdatr);

///////////////////////// users & crypt //////////////////

#define DB_ADMIN_GROUP   0
#define ANONYMOUS_USER   1
#define EVERYBODY_GROUP  2
#define CONF_ADM_GROUP   3
#define SECUR_ADM_GROUP  4
#define SPECIAL_USER_COUNT 5

#define PUK_NAME1  16
#define PUK_NAME2   2
#define PUK_NAME3  20
#define PUK_IDENT  80

#pragma pack(1)
struct t_user_ident
{ char name1[PUK_NAME1+1];
  char name2[PUK_NAME2+1];
  char name3[PUK_NAME3+1];
  char identif[PUK_IDENT+1];
};
#pragma pack()

#define RSA_KEY_SIZE  (256+8)  /* maximum */

#define MD5_SIZE        16
#define MAX_DIGEST_SIZE 20
typedef uns8 t_hash[MD5_SIZE];
typedef uns8 t_hash_ex[MAX_DIGEST_SIZE];
#define ENC_HASH_SIZE   (256+8)  // MAX_BYTE_PRECISION
typedef uns8 t_enc_hash[ENC_HASH_SIZE];

enum t_sigstate { sigstate_notsigned, sigstate_invalid,  sigstate_changed,
  sigstate_unknown, sigstate_uncertif, sigstate_time, sigstate_notca,
  sigstate_valid, sigstate_error, sigstate_validchng,
  sigstate_rootcertif, sigstate_ca_certificate, sigstate_certificate, sigstate_request  // used only in the keytable
};

#if WBVERS<=810
#include "../wbed/wbed.h"
#endif

#pragma pack(1)  // enables using signatures from versions previous to 8.0
typedef struct
{ uns8         state;       // no reason to sigh this
  WBUUID       key_uuid;    // not signed, used when verifying
  t_user_ident user_ident;  // signed, kvuli zobrazovani
  uns32        dtm;         // date of signing, signed, very important
  t_enc_hash   enc_hash;
} t_signature;

#if WBVERS<=810
typedef struct
{ uns32    version_id;
  uns8     key_uuid[UUID_SIZE];
  CKeyPair KeyPair;
  uns8     pass_xor;
} t_priv_key_file;
#endif
#pragma pack()

typedef enum { PKTTP_REPL_DATA=1, PKTTP_REPL_REQ,
               PKTTP_REPL_ACK,    PKTTP_REPL_NOACK,
               PKTTP_STATE_REQ,   PKTTP_STATE_INFO,
               PKTTP_SERVER_INFO, PKTTP_SERVER_REQ,
               PKTTP_REL_REQ,     PKTTP_REL_ACK,
               PKTTP_REL_NOACK,   PKTTP_REPL_UNBLOCK,
               PKTTP_REPL_STOP,   PKTTP_NOTH_TOREPL,
               OPER_RESET_COMM=100, OPER_SKIP_REPL=101,
               OPER_RESEND_PACKET=102, OPER_CANCEL_PACKET=103
             } t_packet_type;

typedef struct
{ WBUUID srv_uuid;
  WBUUID apl_uuid;
  char   relname[OBJ_NAME_LEN+1];
  uns32  get_def;
  WBUUID dp_uuid, os_uuid, vs_uuid;
} t_relreq_pars;

typedef enum { REPLAPL_NOTHING=0, REPLAPL_REQPOSTED,
               REPLAPL_REQRECV, REPLAPL_IS, REPLAPL_REQREJ, REPLAPL_BLOCKED,
               REPLAPL_INTERR, REPLAPL_DELETED, REPLAPL_CONFLICT, REPLAPL_SRVRREG }
             t_replapl_states;
typedef struct t_replapl
{ t_replapl_states state;
  char relname[OBJ_NAME_LEN+1];
  uns32 want_def;
  t_replapl(t_replapl_states i_state, const char * i_relname, BOOL i_want_def)
    { state=i_state;  strcpy(relname, i_relname);  want_def=i_want_def; }
  t_replapl(void) {}
} t_replapl;

#define USER_ATR_LOGNAME 3
#define USER_ATR_CATEG   4
#define USER_ATR_UUID    5
#define USER_ATR_UPS     6
#define USER_ATR_INFO         7 // readable names, indentification, t_user_ident
#define USER_ATR_SERVER  8
#define USER_ATR_PASSWD       9 // last password + counter
#define USER_ATR_PASSCREDATE 10 // password creation date
#define USER_ATR_STOPPED     11
#define USER_ATR_CERTIF  12

typedef enum { SRV_ATR_NAME=1, SRV_ATR_UUID, SRV_ATR_PUBKEY,
 SRV_ATR_ADDR1, SRV_ATR_ADDR2, SRV_ATR_APPLS, SRV_ATR_RECVNUM, SRV_ATR_FOFFSET,
 SRV_ATR_RECVFNAME, SRV_ATR_SENDNUM, SRV_ATR_ACKN,
 SRV_ATR_SENDFNAME, SRV_ATR_REPLGCR, SRV_ATR_USRGCR, SRV_ATR_USEALT
 } srvtab_attrs;
typedef enum { REPL_ATR_SOURCE=2, REPL_ATR_DEST, REPL_ATR_APPL, REPL_ATR_TAB,
 REPL_ATR_ATTRMAP, REPL_ATR_REPLCOND, REPL_ATR_NEXTTIME, REPL_ATR_GCRLIMIT
 } repltab_attrs;
typedef enum { KEY_ATR_UUID=2, KEY_ATR_USER, KEY_ATR_PUBKEYVAL,
 KEY_ATR_CREDATE, KEY_ATR_EXPIRES, KEY_ATR_IDENTIF, KEY_ATR_CERTIF,
 KEY_ATR_CERTIFS
 } keytab_attrs;

#define ZCR_SIZE 6
typedef uns8 t_zcr[ZCR_SIZE];

extern "C" {
DllKernel void WINAPI create_uuid(WBUUID appl_id);
DllKernel timestamp WINAPI stamp_now(void);
DllKernel void WINAPI bin2hex(char * dest, const uns8 * data, unsigned len);
}
uns32 clock_now(void);

/* security */
#define MAX_LOGIN_SERVER  79
#define MAX_FIL_PASSWORD  31
#define MAX_SA_PASSWORD  100
typedef enum
{ fil_crypt_no=0, fil_crypt_simple, fil_crypt_fast, fil_crypt_slow, fil_crypt_des
} t_fil_crypt;
#ifdef STOP
typedef  struct
{ uns32  passminlen;
  uns32  passexpires;
  uns32  anonymous_ri;
  char   login_server[MAX_LOGIN_SERVER+1];
  WBUUID top_CA;
  uns32  fil_crypt;
  char   fil_password[MAX_FIL_PASSWORD+1];
  uns8   old_sa_pass[MD5_SIZE];
  uns8   new_sa_pass[MD5_SIZE];
} t_security;
#endif
/////////////////////////////// links ////////////////////////////////////
#define LINKTYPE_WB   1
#define LINKTYPE_ODBC 2

typedef struct
{ uns8 link_type; // ==LINKTYPE_WB
  tobjname destobjname;
  uns8 dest_uuid[UUID_SIZE];
} t_wb_link;

extern const char * db_errors[];
extern const char * db_warnings[];
extern const int warning_messages[];
extern const char * compil_errors[];
extern const char * kse_errors[];
#ifndef SRVR
extern const char * client_errors[];
extern const char * ipjrt_errors[];
#endif
extern const char * mail_errors[];

#define PRE_KONTEXT  60
#define POST_KONTEXT 40

#define TEMP_TABLE_MARK 4

inline DWORD GetTickDifference (DWORD dwTick, DWORD dwTickRef)
/************************************************************/
{ return dwTick >= dwTickRef ?
         dwTick - dwTickRef :
   0xffffffffL - dwTickRef + dwTick;
}

enum        { MAX_SQL_RESULTS=10 };

typedef unsigned (WINAPI *BEGINTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
#define LPTHREAD_START_ROUTINE BEGINTHREAD_START_ROUTINE  // overriding std. typedef, CreateThread replaced by _beginthreadex

enum t_cdp_type   { PT_EMPTY=0x00, PT_DIRECT=0x80, PT_REMOTE=0x40, PT_SLAVE=0x20, PT_SERVER=0x10,
                    PT_MAPSLAVE=0x21, PT_WORKER=0x22, PT_KERDIR=PT_MAPSLAVE };

#define LONGJMPBUF CATCHBUF
#define DEFAULT_RQ_FRAME_SIZE  27

#define MAX_LIBRARY_NAME  71
struct libinfo  // linked list of loaded libraries in the project on the client side
{ HINSTANCE hLibInst;
  libinfo * next_lib;
  char      libname[MAX_LIBRARY_NAME+1];
};

void WINAPI mark(const void * block);       // on server only
void WINAPI mark_array(const void * block); // on server only

class MsgInfo
{ enum t_buftype { BT_NONE, BT_DEFAULT, BT_DYNALLOC };
  unsigned char * set;     /* start of the full message buffer */
  uns32           offset;  /* offset of the next message part  */
  uns32           total;   /* total size of the received parts */
  unsigned char   default_rq_frame[DEFAULT_RQ_FRAME_SIZE+1];
  t_buftype       reqBuffType;  // type of buffer in "set"
 public:
  inline MsgInfo(void)
    { reqBuffType = BT_NONE; }
  inline bool has_buffer(void)
    { return reqBuffType != BT_NONE; }
  inline bool has_all(void)
    { return offset == total; }
  inline unsigned char * _set(void) { return set; }
  inline uns32 _total(void) { return total; }
  inline unsigned char * create_buffer(uns32 size)
  { if (size > sizeof(default_rq_frame))
    { set = (unsigned char*)corealloc(size, OW_NETWORK);
      if (!set) return NULL;
      reqBuffType = BT_DYNALLOC;
    }
    else
    { set = default_rq_frame;
      reqBuffType = BT_DEFAULT;
    }
    offset = 0;
    total  = size;
    return set;
  }
  inline void release(void)
  { if (reqBuffType == BT_DYNALLOC) corefree(set);
    set = NULL;
    reqBuffType = BT_NONE;
  }
  inline void add_data(unsigned char * pData, unsigned DataSize)
  { memcpy(set + offset, pData, DataSize);
    offset += DataSize;
  }
#ifdef SRVR
  void mark(void)
    { if (reqBuffType == BT_DYNALLOC) ::mark(set); }
#endif
};

#ifdef WINS
typedef DWORD     t_thread;   /* WINS */
#else
#ifdef UNIX
typedef pthread_t t_thread;   /* UNIX */
#else
typedef int       t_thread;   /* for NLM */
#endif
#endif

typedef t_varcol_size theapsize;
#define CNTRSZ   sizeof(t_mult_size) /* size of the multiatribute counter */
#define HPCNTRSZ sizeof(t_varcol_size) /* size of the heap object byte counter */

#define REPLAY_NO_JOURNAL          1    // used by ADMIN.CPP
#define REPLAY_NO_MEMORY           2
#define REPLAY_NO_VALID_RECORD     3
#define REPLAY_BAD_JOURNAL         4
#define REPLAY_LIMIT_REACHED       5

#define RTP_RDONLY     8                // used by ADMIN.CPP
#define RTP_JOUR      16
#define RTP_TRAN      32
#define RTP_SMRT      64
#define RTP_FLUSH    128

struct MARKER  // info about a DP marker in SQL statement, created during compilation
{ int       type;          // 0 if not known
  t_specif  specif;        // type-specific info
  t_parmode input_output;  // bit 1 for INPUT use, bit 2 for OUTPUT use
  int       position;      // offset in the statement, used by server only
};

//////////////////////////// compiler interface ////////////////////////////////
#define FIRST_CLIENT_ERROR       5000
// compiler error numbers
#define COMPILED_OK              0   /* values returned by "compile" */
#define FIRST_COMP_ERROR         1000
#define EOF_IN_COMMENT           1003
#define STRING_EXCEEDS_LINE      1004
#define LOCAL_RECURSION          1005
#define TYPE_EXPECTED            1006
#define LEFT_BRACE_EXPECTED      1007
#define RIGHT_BRACE_EXPECTED     1008
#define DOUBLE_DOT_EXPECTED      1009
#define INTEGER_EXPECTED         1010
#define OF_EXPECTED              1011
#define ARRAY_BOUNDS             1012
#define ARRAY_BIGGER_64          1013
#define IDENT_EXPECTED           1014
#define EQUAL_EXPECTED           1015
#define BAD_CHAR                 1016
#define COMMA_EXPECTED           1017
#define END_OR_SEMICOLON_EXPECTED 1018
#define SEMICOLON_EXPECTED       1019
#define BEGIN_EXPECTED           1020
#define NESTED_SUBROUTINES       1021
#define LEFT_PARENT_EXPECTED     1022
#define RIGHT_PARENT_EXPECTED    1023
#define COLON_EXPECTED           1024
#define BAD_IDENT_CLASS          1025
#define MUST_BE_BOOL             1026
#define DO_EXPECTED              1027
#define ASSIGN_EXPECTED          1028
#define MUST_BE_NUMERIC          1029
#define MUST_BE_INTEGER          1030
#define BAD_OPERATION            1031
#define INCOMPATIBLE_TYPES       1032
#define MONEY_MULTIPLY           1033
#define NOT_A_REC                1034
#define NOT_A_PTR                1035
#define NOT_AN_ARRAY             1036
#define IDENT_NOT_DEFINED        1037
#define ITEM_IDENT_EXPECTED      1038
#define INDEX_NOT_INT            1039
#define THEN_EXPECTED            1040
#define COMMA_OR_SEMIC_EXPECTED  1041
#define TOO_MANY_PARAMS          1042
#define DOT_EXPECTED             1043
#define NUMBER_TOO_BIG           1044
#define STATEMENT_EXPECTED       1045
#define TOO_MANY_ALLOC           1046
#define TOO_BIG_TO_BE_STAT       1047
#define BAD_TAIL                 1048
#define ID_DECLARED_TWICE        1049
#define BAD_FUNCTION_TYPE        1050
#define CODE_GEN                 1051
#define GARBAGE_AFTER_END        1052
#define NESTED_DB_ACCESS         1053
#define NO_LENGTH                1054
#define INDEX_MISSING            1055
#define NOT_STRUCTURED_TYPE      1056
#define VIEW_DOES_NOT_EXIST      1057
#define DEF_READ_ERROR           1058
#define OUT_OF_MEMORY            1059 
#define TO_OR_DOWNTO_EXPECTED    1060
#define UNDEF_ATTRIBUTE          1061
#define FAKTOR_EXPECTED          1062
#define CURSOR_DOES_NOT_EXIST    1063
#define BAD_AGGREG               1064
#define BAD_TIME                 1065
#define BAD_DATE                 1066
#define BAD_DB_ASSGN             1067
#define NOT_VAR_LEN              1068
#define TABLE_DOES_NOT_EXIST     1069
#define UNDEF_ACTION             1070
#define BACKSLASH_EXPECTED       1071
#define UNTIL_EXPECTED           1072
#define DIFFERENT_PROJECT        1073
#define END_EXPECTED             1074
#define VIEW_COMP                1075   /* copy in messages.c */
#define STRING_EXPECTED          1076
#define NOSPEC_ERROR             1077
#define INT_OUT_OF_BOUND         1078
#define TOO_COMPLEX_VIEW         1079
#define DEF_KEY_EXPECTED         1080
#define BAD_IDENT                1081
#define GROUP_ORDER              1082
#define TOO_MANY_ITEMS           1083
#define ITEM_EXPECTED            1084
#define BAD_CURSOR_DEF           1085
#define TOO_MANY_MENU_ITEMS      1086
#define INCLUDE_NOT_FOUND        1087
#define CANNOT_DETACH            1088
#define ID_DUPLICITY             1089
#define SEMICOLON_ELSE           1090
#define TOO_MANY_TABLE_ATTRS     1091
#define TOO_MANY_INDXS           1092
#define NO_ATTRS_DEFINED         1093
#define BRACE_NOT_ALLOWED        1094
#define DUPLICATE_NAME           1095    // constrain name duplicity
#define TOO_MANY_INDEX_PARTS     1096
#define BAD_TYPE_IN_INDEX        1097
#define INDEX_KEY_TOO_LONG       1098
#define BAD_TABLE_SYNTAX         1099
#define INDEX_TOO_COMPLEX       1100
#define TOO_MANY_CURS_TABLES    1101
#define FROM_OMITTED            1102
#define TABLE_NAME_EXPECTED     1103
#define SQL_SYNTAX              1104
#define BAD_SQL_PREDICATE       1105
#define SELECT_EXPECTED         1106
#define FROM_EXPECTED           1107
#define AGGR_IN_WHERE           1108
#define AGGR_NESTED             1109
#define ONLY_FOR_COUNT          1110     /* SUM(*) */
#define GENERAL_SQL_SYNTAX      1111
#define AND_EXPECTED            1112     /* Y BETWEEN X ... */
#define OUTER_EXPECTED          1113
#define JOIN_EXPECTED           1114
#define ERROR_IN_FROM           1115     /* bad object in FROM clause */
#define BY_EXPECTED             1116     /* ORDER ..., GROUP ... */
#define UNKNOWN_TYPE_NAME       1117     /* may be misformed, e.g. LONG CHAR */
#define GENERAL_TABDEF_ERROR    1118     /* altering bad table */
#define KEY_EXPECTED            1119     /* FOREING ..., PRIMARY ... */
#define CURSOR_STRUCTURE_UNKNOWN 1120
#define REFERENCES_EXPECTED     1121     /* tabdef */
#define ATTRIBUTE_NOT_FOUND     1122     /* dropping or altering attr. in tabdef */
#define PRECISION_EXPECTED      1123     /* DOUBLE ... */
#define NULL_EXPECTED           1124     /* NOT ... constrain in tabdef, IS NULL */
#define NOT_AN_SQL_STATEMENT    1125
#define TO_EXPECTED             1126
#define ON_EXPECTED             1127
#define INTO_EXPECTED           1128
#define VALUES_EXPECTED         1129
#define BAD_AGGR_FNC_ARG_TYPE   1130
#define SET_EXPECTED            1131     /* UPDATE sql statement */
#define CANNOT_DELETE_IN_THIS_VIEW 1132
#define CANNOT_UPDATE_IN_THIS_VIEW 1133
#define TOO_MANY_INSERT_VALUES  1134
#define SELECT_LISTS_NOT_COMPATIBLE 1135
#define SELECT_LISTS_TOO_MUCH_DIFFERENT 1136
#define BAD_NUMBER_IN_ORDER_BY  1137
#define AGGR_IN_ORDER           1138
#define UPDATE_EXPECTED         1139
#define NOT_EXPECTED            1140
#define NO_PARENT_INDEX         1141
#define NAME_DUPLICITY          1142     // column name duplicity
#define TWO_PRIMARY_KEYS        1143
#define INDEX_EXPECTED          1144
#define AS_EXPECTED             1145
#define NO_READ_RIGHT           1146
#define NO_LOCAL_INDEX          1147
#define COMBO_SELECT            1148
#define SUBROUTINE_TOO_BIG      1149
#define CANNOT_INSERT_INTO_THIS_VIEW 1150
#define ERROR_IN_DEFAULT_VALUE  1151
#define NO_WRITE_RIGHT          1152
#define CURSOR_RECURSION        1153
#define BAD_CHAR_IN_HEX_LITERAL 1154
#define FORWARD_OPEN            1155
#define PTR_OPEN                1156
#define NO_COMMON_COLUMN_NAME   1157  // WB 5.1 errors
#define NO_REFERENCE_PRIVILEDGE 1158
#define SUBQUERY_IN_AGGR        1159
#define IN_EXPECTED             1160
#define DATETIME_FIELD_EXPECTED 1161
#define BOOLEAN_CONSTANT_EXPECTED 1162
#define NOT_NOT_ALLOWED         1163
#define NOT_SCALAR_SUBQ         1164
#define INVALID_IN_ATOMIC_BLOCK 1165
#define IF_EXPECTED             1166
#define LOOP_EXPECTED           1167
#define RETURNS_EXPECTED        1168
#define PROCEDURE_EXPECTED      1169
#define KEYWORD_PARAM_EXPECTED  1170
#define DUPLICATE_ACTUAL_PARAM  1171
#define BAD_PARAMETER_MODE      1172
#define RETURN_OUTSIDE_FUNCTION 1173
#define CANNOT_CHANGE_CONSTANT  1174
#define CONSTANT_MUST_HAVE_VAL  1175
#define SQLSTATE_EXPECTED       1176
#define FOR_EXPECTED            1177
#define SQLSTATE_FORMAT         1178
#define FOUND_EXPECTED          1179
#define INTERNAL_COMP_ERROR     1180
#define BAD_INTO_OBJECT         1181
#define HANDLER_EXPECTED        1182
#define REDO_UNDO_ATOMIC        1183
#define REPEAT_EXPECTED         1184
#define WHILE_EXPECTED          1185
#define CURSOR_EXPECTED         1186
#define SAVEPOINT_EXPECTED      1187
#define CHAIN_EXPECTED          1188
#define TRANSACTION_EXPECTED    1189
#define DUPLICATE_SPECIFICATION 1190
#define SPECIF_CONFLICT         1191
#define CASE_EXPECTED           1192
#define FOR_CURSOR_INVALID      1193
#define CURSOR_IS_INSENSITIVE   1194
#define AGGR_NO_KONTEXT         1195
#define NAME_EXPECTED           1196  // keyword NAME meant
#define THIS_TYPE_NOT_ENABLED   1197
#define OBJECT_EXPECTED         1198
#define BEFORE_OR_AFTER_EXPECTED 1199
#define TRIGGER_EVENT_EXPECTED  1200
#define OLD_OR_NEW_EXPECTED     1201
#define TRANSIENT_TABLES_NOT_SUPP 1202
#define EACH_EXPECTED           1203
#define ROW_OR_STAT_EXPECTED    1204
#define TRIGGER_EXPECTED        1205
#define BAD_TARGET              1206
#define FEATURE_NOT_SUPPORTED   1207
#define OTHER_COLS_NOT_ALLOWED  1208
#define ABSTRACT_CLASS_INSTANCE 1209
#define CANNOT_SET_PROJECT      1210
#define BAD_METHOD_OR_PROPERTY  1211
#define CONTROLS_NOT_KNOWN      1212
#define THISFORM_UNDEFINED      1213
#define UNDEFINED_DIRECTIVE     1214
#define EOF_IN_MACRO_EXPANSION  1215
#define UNEXPECTED_DIRECTIVE    1216
#define EOF_IN_CONDITIONAL      1217
#define APPL_IS_LOCKED          1218
#define TYPELIB_ERROR           1219
#define WITH_EXPECTED           1220
#define BAD_SEQUENCE_PARAMS     1221
#define INCOMPATIBLE_SYNTAX     1222
#define EMB_VAR_OR_DP_IN_PROC   1223
#define CANNOT_HANDLE_THE_ERROR 1224
#define CANNOT_ON_GLOBAL_LEVEL  1225
#define REQUIRES_AUTHOR_PRIVILS 1226
#define EXISTS_EXPECTED         1227
#define TOO_COMPLEX_QUERY       1228
#define CANNOT_IN_DEBUGGER      1229
#define LIMITS_CLAUSE_SYNTAX_ERROR 1230
#define LABEL_OUTSIDE_BLOCK     1231

#define LAST_COMP_ERROR         1299

#define ERR_WITH_MSG(err)        ((err)==BAD_IDENT_CLASS      ||\
  (err)==ATTRIBUTE_NOT_FOUND   || (err)==TABLE_DOES_NOT_EXIST ||\
  (err)==ID_DECLARED_TWICE     || (err)==IDENT_NOT_DEFINED    ||\
  (err)==CURSOR_DOES_NOT_EXIST || (err)==UNDEF_ATTRIBUTE      ||\
  (err)==REFERENTIAL_CONSTRAINT|| (err)==CHECK_CONSTRAINT     ||\
  (err)==CANNOT_SET_PROJECT    || (err)==BAD_METHOD_OR_PROPERTY ||\
  (err)==MUST_NOT_BE_NULL      || (err)==KEY_DUPLICITY        ||\
  (err)==OBJECT_NOT_FOUND      || (err)==TABLE_DAMAGED        ||\
  (err)==CONTROLS_NOT_KNOWN    || (err)==IE_DOUBLE_PAGE       ||\
  (err)==SEQUENCE_EXHAUSTED    || (err)==NO_CURRENT_VAL       ||\
  (err)==EOF_IN_MACRO_EXPANSION|| (err)==EOF_IN_CONDITIONAL   ||\
  (err)==LIBRARY_NOT_FOUND     || (err)==LANG_SUPP_NOT_AVAIL  ||\
  (err)==CONVERSION_NOT_SUPPORTED || (err)==NO_REPL_UNIKEY    ||\
  (err)==INDEX_NOT_FOUND       || (err)==SQ_TRIGGERED_ACTION  ||\
  (err)==SQ_UNHANDLED_USER_EXCEPT || (err)==OBJECT_DOES_NOT_EXIST ||\
  (err)==NOT_LOCKED            || (err)==COLUMN_NOT_EDITABLE  ||\
  (err)==REFERENCED_BY_OTHER_OBJECT || (err)==BAD_PASSWORD    ||\
  (err)==STRING_CONV_NOT_AVAIL || (err)==NO_REFERENCE_PRIVILEDGE ||\
  (err)==INDEX_DAMAGED         || (err)==LIMIT_EXCEEDED )
#define SET_ERR_MSG(cdp, msg) { if (cdp) strmaxcpy((cdp)->errmsg, msg, sizeof(tobjname)); }

struct objtable;
union  typedescr;
struct typeobj;
union  object;
struct compil_info;
typedef compil_info * CIp_t;
typedef short symbol;
typedef uns32 code_addr;

union basicval0    /* the basic simple value - 8 bytes */
{ sig32   int32;
  byte    money[6];
  monstr  moneys;
  double  real;
  sig16   int16;
  sig64   int64;
  uns8    uint8;
  sig8    int8;
  tptr    ptr;
};

#define END_OF_SOURCE_PART  1  /*  separator of source parts */
#define ORDER_BYTEMARK      1
#define QUERY_TEST_MARK     0xdc

CFNC DllKernel symbol WINAPI next_sym(CIp_t CI);
CFNC DllKernel void   WINAPI next_and_test(CIp_t CI, symbol sym, short message);
CFNC DllKernel void   WINAPI test_and_next(CIp_t CI, symbol sym, short message);

object * search_obj(CIp_t CI, ctptr name, uns8 * obj_level);

#define ATT_WINID ATT_INT32

#define S_SOURCE_END  0
#define S_IDENT       1
#define S_INTEGER     2
#define S_REAL        3
#define S_STRING      4
#define S_CHAR        5
#define S_DATE        6
#define S_TIME        7
#define S_MONEY       8
#define S_TIMESTAMP   9

#define S_LESS_OR_EQ 10
#define S_GR_OR_EQ   11
#define S_NOT_EQ     12
#define S_ASSIGN     13
#define S_DDOT       14
#define S_DBL_AT     15
#define S_PAGENUM    16
#define S_PREFIX     17
#define S_SUBSTR     18
#define S_CURRVIEW   19
#define S_NEXTEQU    20  
#define S_BINLIT     21
#define S_SQLSTRING  22
/* keywords will have their symbol numbers from 0x80 up */
enum keywords { S_AND=128, S_ARRAY, S_BEGIN, S_BINARY, S_CASE, SC_COLLATE, S_CONST,
                S_CSISTRNG, S_CSSTRNG,
                S_CURSOR, S_DIV, S_DO, S_DOWNTO,
                S_ELSE, S_END, S_FILE, S_FOR, S_FORM, S_FUNCTION, S_HALT, S_IF,
                S_INCLUDE, S_MOD,
                S_NOT, S_OF, S_OR, S_PROCEDURE, S_RECORD,
                S_REPEAT, S_RETURN, S_STRNG, S_TABLE, S_THEN, S_THISFORM, S_TO, S_TYPE,
                S_UNTIL, S_VAR, S_WHILE, S_WITH };
enum { S_ADD=128, SQ_AFTER, S_ALL, S_ALTER, SQ_AND, SQ_ANY,
  S_AS, S_ASC, S_AUTHOR, S_AUTOR, S_AVG,
  SQ_BEFORE, SQ_BEGIN, S_BETWEEN, S_BIGINT, SQ_BINARY,
  S_BIPTR, S_BIT, SQ_BLOB, SQ_BOOLEAN, S_BY,
  SQ_CALL, S_CASCADE, SQ_CASE, SQ_CAST, SQ_CHAR, S_CHARACTER,
  S_CHECK, SQ_CLOB, SQ_CLOSE, SQ_COALESCE, S_COLLATE, SQ_COMMIT,
  SQ_CONCAT, SQ_CONDITION, SQ_CONSTANT, S_CONSTRAINT, SQ_CONTINUE,
  SQ_CORRESPONDING, S_COUNT, S_CREATE, SQ_CROSS, S_CURRENT, SQ_CURSOR,
  SQ_DATE, S_DATIM, S_DEC, S_DECIMAL, SQ_DECLARE, S_DEFAULT, S_DELETE,
  S_DESC, S_DISTINCT, SQ_DIV, SQ_DO, SQ_DOMAIN, S_DOUBLE, S_DROP,
  SQ_ELSE, SQ_ELSEIF, SQ_END, SQ_ESCAPE, SQ_EXCEPT,
  S_EXISTS, SQ_EXIT, SQ_EXTERNAL, SQ_FETCH, S_FLOAT, SQ_FOR,
  S_FOREIGN, S_FROM, SQ_FULL, SQ_FUNCTION, S_GRANT, S_GROUP,
  SQ_HANDLER, S_HAVING, S_HISTORY,
  SQ_IF, SQ_IN, S_INDEX, SQ_INNER, SQ_INOUT, S_INSERT, S_INT,
  SQ_INTEGER, SQ_INTERS, S_INTO, S_IS, S_JOIN, S_KEY,
  SQ_LARGE, SQ_LEAVE, S_LEFT, SQ_LIKE, /*SQ_LIMIT, */S_LONG, SQ_LOOP,
  S_MAX, S_MIN, SQ_MOD, S_NATIONAL, SQ_NATURAL, S_NCHAR, SQ_NCLOB, SQ_NOT,
  S_NULL, SQ_NULLIF, S_NUMERIC, SQ_OBJECT, 
  SQ_OF, S_ON, SQ_OPEN, SQ_OR, S_ORDER, SQ_OTHERS, SQ_OUT, S_OUTER,
  S_POINTER, S_PRECISION, S_PRIMARY, SQ_PROCEDURE, S_PUBLIC, SQ_REAL,
  SQ_REDO, S_REFERENCES, SQ_REFERENCING, SQ_RELEASE, SQ_REPEAT, SQ_RESIGNAL, S_RESTRICT,
  SQ_RETURN, SQ_RETURNS, S_REVOKE, S_RIGHT, SQ_ROLLBACK, SQ_SAVEPOINT,
  SQ_SCHEMA, S_SELECT, SQ_SEQUENCE, S_SET, SQ_SIGNAL, S_SIGNATURE, S_SMALLINT, SQ_SOME,
  SQ_SQLEXCEPTION, SQ_SQLSTATE, SQ_SQLWARNING, SQ_START, S_SUM,
  SQ_TABLE, SQ_THEN, SQ_TIME, 
  SQ_TIMESTAMP, SQ_TINYINT, SQ_TO, SQ_TRIGGER, SQ_TUPLE, SQ_UNDO, S_UNION, S_UNIQUE,
  SQ_UNTIL, SQ_UPDATABLE, S_UPDATE, S_USER, SQ_USING,
  S_VALUES, S_VARBINARY, S_VARCHAR, S_VARYING,
  S_VIEW, SQ_WHEN, S_WHERE, SQ_WHILE, SQ_WITH };
// S_IS a SQ_IN fixed in curedit.rc in RCDATA for query designer!

//////////////////////////// conversion functions //////////////////////////////
#define SQL_LITERAL_PREZ 99  // conversion parameter value suitable to SQL literal format
extern "C" {
DllKernel void WINAPI int8tostr(sig8 val, tptr txt);
DllKernel void WINAPI int16tostr(sig16 val, tptr txt);
DllKernel void WINAPI int32tostr(sig32 val, tptr txt);
DllKernel void WINAPI int64tostr(sig64 val, tptr txt);
DllKernel BOOL WINAPI str2uns(const char ** ptxt, uns32 * val);  /* without end-check */
DllKernel BOOL WINAPI str2int64(const char * txt, sig64 * val);
DllKernel BOOL WINAPI str2uns64(const char ** ptxt, sig64 * val);  /* without end-check */
DllKernel void WINAPI money_neg(monstr * m);

extern char explic_decim_separ[2];

DllKernel void  WINAPI encode     (char * tx, unsigned size, BOOL inputing, int recode);
bool WINAPI scale_int64(sig64 & intval, int scale_disp);
bool WINAPI scale_integer(sig32 & intval, int scale_disp);
void WINAPI scale_real(double & realval, int scale_disp);

int    WINAPI sp_ord(uns8 c);
uns8   WINAPI sp_chr(int i);
BOOL   WINAPI sp_odd(sig32 i);
double WINAPI sp_abs(double r);
sig32  WINAPI sp_iabs(sig32 i);
sig32  WINAPI sp_round(double r);
sig32  WINAPI sp_trunc(double r);
double WINAPI sp_sqr(double r);
sig32  WINAPI sp_isqr(sig32 i);

sig32    WINAPI sp_str2int(char * txt);
sig64    WINAPI sp_str2bigint(char * txt);
monstr * WINAPI sp_str2money(char * txt);
double   WINAPI sp_str2real(char * txt);
uns32    WINAPI sp_str2date(char * txt);
uns32    WINAPI sp_str2time(char * txt);
uns32    WINAPI sp_str2timestamp(char * txt);

double WINAPI sp_sin (double arg);
double WINAPI sp_cos (double arg);
double WINAPI sp_atan(double arg);
double WINAPI sp_exp (double arg);
double WINAPI sp_log (double arg);
double WINAPI sp_sqrt(double arg);

DllKernel BOOL WINAPI general_Pref(const char * s1, const char * s2, t_specif specif);
DllKernel int  WINAPI general_Substr(const char * s1, const char * s2, t_specif specif);
}

#define SC_INCONVERTIBLE_WIDE_CHARACTER -1
#define SC_INCONVERTIBLE_INPUT_STRING   -2
#define SC_INPUT_STRING_TOO_LONG        -3
#define SC_VALUE_OUT_OF_RANGE           -4
#define SC_BAD_TYPE                     -5
#define SC_BINARY_VALUE_TOO_LONG        -6
#define SC_STRING_TRIMMED               -7
CFNC DllKernel int WINAPI superconv(int stype, int dtype, const void * sbuf, char * dbuf, t_specif sspec, t_specif dspec, const char * string_format);
CFNC DllKernel int WINAPI space_estimate(int stype, int dtype, int sbytes, t_specif sspec, t_specif dspec);

/////////////////////////////// charsets, collation and Unicode //////////////////////////
#define CHARSET_NUM_IBM852 6
#define CHARSET_NUM_UTF8   7
#define CHARSET_COUNT 8  // 6=ibm852, 7=UTF-8

#ifdef LINUX
typedef struct __locale_struct * charset_t;  // replaces including locale.h
typedef uns16 wuchar;
CFNC DllKernel size_t   wuclen(const wuchar * wstr);
CFNC DllKernel wuchar * wuccpy(wuchar * wdest, const wuchar * wsrc);
int wucnicmp(const wuchar * s1, const wuchar * s2, int maxlength);
int wucncmp (const wuchar * s1, const wuchar * s2, int maxlength);
wuchar *wusrchr(const wuchar *s1, int c);
extern "C" {
int WideCharToMultiByte(const char *charset, const char *wstr, size_t len, char *buffer);
int MultiByteToWideChar(const char *charset, const char *str, size_t len, wuchar *buffer);
}
#define ext_unicode_specif t_specif(0, 0, 0, 2)

#else // WINS

typedef wchar_t wuchar;
#define wuclen(wstr) wcslen(wstr)
#define wuccpy(wdest,wsrc) wcscpy(wdest,wsrc)
#define wucnicmp(s1, s2, maxlen) _wcsnicmp(s1, s2, maxlen)
#define wucncmp(s1, s2, maxlen) wcsncmp(s1, s2, maxlen)
#define wusrchr(s1, c) wcsrchr(s1, c)

#define vsnprintf(buffer, count, format, argptr) _vsnprintf(buffer, count, format, argptr)
#define snprintf _snprintf
#define ext_unicode_specif t_specif(0, 0, 0, 1)
#endif

CFNC DllKernel void WINAPI wucmaxcpy(wuchar  * destin, const wuchar  * source, unsigned maxchars);
CFNC DllKernel unsigned WINAPI wucmaxlen(const wuchar * source, unsigned maxchars);

struct t_charset_data
{ const unsigned char * pcre_table;
};

void init_charset_data(void);

class wbcharset_t
{ uns8 code;  // internal representation of charset
	static const char* code_name_win[CHARSET_COUNT]; // on LINUX the iconv parameter, on Windows used in error messages only
	static const char* code_name_iso[CHARSET_COUNT]; // on LINUX the iconv parameter, on Windows used in error messages only
  static t_charset_data charset_data_win[CHARSET_COUNT];
  static t_charset_data charset_data_iso[CHARSET_COUNT];
 public:
  operator const char *()                          // on LINUX the iconv parameter, on Windows used in error messages only
    { return (code & 0x80) ? code_name_iso [code & 0x7f] : code_name_win [code]; }
  t_charset_data * charset_data(void)
    { return (code & 0x80) ? &charset_data_iso[code & 0x7f] : &charset_data_win[code]; }
  bool charset_available(void) const;

#ifdef LINUX
 public:
	static charset_t code_table_win[CHARSET_COUNT];
	static charset_t code_table_iso[CHARSET_COUNT];
  static bool prepare_conversions(void);
  charset_t locales8(void);
  charset_t locales16(void);
  wbcharset_t(charset_t){};
#else // WINS
 private:
	static DWORD lcid_table[CHARSET_COUNT];  // used for comparison and case-changes by Unicode
	static int cp_table_win[CHARSET_COUNT];  // used in conversions between MBCS and UNICODE
	static int cp_table_iso[CHARSET_COUNT];  // used in conversions between MBCS and UNICODE
 public:
  DWORD lcid(void) const { return lcid_table[code & 0x7f]; }  // used by Unicode
  DWORD cp  (void) const 
    { return (code & 0x80) ? cp_table_iso[code & 0x7f] : cp_table_win[code & 0x7f]; }
#endif

 // used by 8-bit code pages:
  static const uns8 * case_tables_win[];
  static const uns8 * case_tables_iso[];
  static const uns8 * c7_tables_win[];
  static const uns8 * c7_tables_iso[];
  const uns8 * case_table(void) const
    { return (code & 0x80) ? case_tables_iso[code & 0x7f] : case_tables_win[code & 0x7f]; }
  static const wchar_t * unic_tables_iso[];
  const wchar_t * unic_table(void) const  // used only for ISO code pages
    { return unic_tables_iso[code & 0x7f]; }
  const uns8 * ascii_table(void) const
    { return (code & 0x80) ? c7_tables_iso[code & 0x7f] : c7_tables_win[code & 0x7f]; }

    wbcharset_t()            : code(0)      { }
	wbcharset_t(uns8 codeIn) : code(codeIn) { }
	wbcharset_t(int codeIn)  : code(codeIn) { }
	bool is_wb8 (void) const { return code>1; }
  bool is_ansi(void) const { return !code; }
  bool is_iso (void) const { return (code & 0x80)!=0; }
  static wbcharset_t get_host_charset();
  int get_code(){return code;}
  friend void init_charset_data(void);
};
#define charset_ansi (wbcharset_t((uns8)0))
#define charset_legacy (wbcharset_t((uns8)1))

CFNC DllKernel void prepare_charset_conversions(void);

CFNC DllKernel char WINAPI upcase_charA(char c, wbcharset_t charset);
BOOL like_esc(const char * str, const char * patt, char esc, BOOL ignore, wbcharset_t charset);
CFNC DllKernel void WINAPI convert_to_uppercaseA(const char * src, char * dest, wbcharset_t charset);
CFNC DllKernel void WINAPI convert_to_lowercaseA(const char * src, char * dest, wbcharset_t charset);
CFNC DllKernel wchar_t WINAPI upcase_charW(wchar_t c, wbcharset_t charset);
BOOL wlike_esc(const wuchar * str, const wuchar * patt, wuchar esc, BOOL ignore, wbcharset_t charset);
CFNC DllKernel void WINAPI convert_to_uppercaseW(const wuchar * src, wuchar * dest, wbcharset_t charset);
CFNC DllKernel void WINAPI convert_to_lowercaseW(const wuchar * src, wuchar * dest, wbcharset_t charset);
CFNC DllKernel bool WINAPI is_AlphaA(char ch, int charset);
CFNC DllKernel bool WINAPI is_AlphaW(wchar_t ch, int charset);
CFNC DllKernel bool WINAPI is_AlphanumA(char ch, int charset);
CFNC DllKernel bool WINAPI is_AlphanumW(wchar_t ch, int charset);
CFNC DllKernel BOOL charset_available(int charset);
CFNC DllKernel const char * WINAPI ToASCII(char *Src, int charset);
CFNC DllKernel const char * WINAPI ToASCII7(char *Src, int charset);
CFNC DllKernel int WINAPI conv2to1(const wuchar * source, int slen, char * target, wbcharset_t charset, int dbytelen);
CFNC DllKernel int WINAPI conv1to2(const char * source, int slen, wuchar * target, wbcharset_t charset, int dbytelen);
CFNC DllKernel int WINAPI conv1to4(const char * source, int slen, uns32 * target, wbcharset_t charset, int dbytelen);
CFNC DllKernel int WINAPI conv4to1(const uns32 * source, int slen, char * target, wbcharset_t charset, int dbytelen);

const char * get_collation_name(int charset);
int find_collation_name(const char * name);

int compare_values(int valtype, t_specif specif, const char * key1, const char * key2);
BOOL same_type(unsigned t1, unsigned t2);

int  cmpmoney (monstr * m1, monstr * m2);
void moneymult(monstr * m1, sig32 val);
void moneyadd (monstr * m1, monstr * m2);
void moneysub (monstr * m1, monstr * m2);
void moneydiv (monstr * m1, monstr * m2);

#define BAS_DATE_FORMATS      2
#define EXT_DATE_FORMATS      6
#define BAS_TIME_FORMATS      3
#define EXT_TIME_FORMATS      4
#define TIMESTAMP_FORMATS    18

//////////////////////////////////// server debugging //////////////////////////////////
#define DBGOP_GET_USER   1
#define DBGOP_GET_STATE  2
#define DBGOP_INIT       3
#define DBGOP_ADD_BP     4
#define DBGOP_REMOVE_BP  5
#define DBGOP_GO         6
#define DBGOP_STEP       7
#define DBGOP_TRACE      8
#define DBGOP_OUTER      9
#define DBGOP_GOTO       10
#define DBGOP_EVAL       11
#define DBGOP_CLOSE      12
#define DBGOP_STACK      13
#define DBGOP_ASSIGN     14
#define DBGOP_INIT2      15 // since version 9
#define DBGOP_BREAK      16
#define DBGOP_KILL       17 // since version 9
#define DBGOP_EVAL9      18 // since version 9
#define DBGOP_STACK9     19 // since version 9

#define DBGSTATE_NOT_FOUND  0   // debugged process not found
#define DBGSTATE_BREAKED    1
#define DBGSTATE_EXECUTING  2
#define DBGSTATE_NO_REQUEST 3

struct t_stack_info
{ tobjnum objnum;
  uns32 line;
};

struct t_stack_info9
{ tobjnum objnum;
  uns32 line;
  tobjname rout_name;  // cannot be derived from [objnum] for local routines and handlers
};
///////////////////////////// sequences ////////////////////////////////
typedef sig64 t_seq_value;

struct t_sequence
{ cdp_t cdp;                  // used only when modifying from GUI
  bool modifying, recursive;  // used only when analysing SQL statement (controls recursive analysis of original sequence)
                              // modifying used by GUI too, controls the dialog 
  tobjname schema,            // used only when analysing SQL statement
           name;  
  tobjnum objnum;
  t_seq_value currval,        // used only on the server
    startval, step, maxval, minval, restartval;
  int cache;
  bool hasmax, hasmin, cycles, hascache, ordered;
  bool start_specified, restart_specified, anything_specified;  // used only when executing the ALTER statement on server
  t_sequence(void)
    { *schema=0; }  // used by the recursive compilation
};

void WINAPI sequence_anal(CIp_t CI);
bool compile_sequence(cdp_t cdp, tptr defin, t_sequence * seq);
tptr sequence_to_source(cdp_t cdp, t_sequence * seq, bool altering);

//////////////////////////// fulltext systems ///////////////////////////
typedef unsigned t_wordid;
typedef uns64 t_docid;  // Changed inside version 10 to 64 bits. Should be unsigned because the NO_DOC_ID value must not be among valid values
enum { MAX_FT_WORD_LEN = 40 };  

#define FT_LANG_CZ  0
#define FT_LANG_SK  1
#define FT_LANG_GR  2
#define FT_LANG_US  3
#define FT_LANG_UK  4
#define FT_LANG_PL  5
#define FT_LANG_HU  6
#define FT_LANG_FR  7
#define NUMBER_OF_LANGUAGES 8
extern const char * ft_lang_name[];

#define MAX_FT_FORMAT 31

struct t_autoindex
{ tag_atr_state tag;
  tobjname doctable_schema;
  tobjname doctable_name;
  char id_column[ATTRNAMELEN+4+1];
  char text_expr[ATTRNAMELEN+1];
  int  mode;
  char format[MAX_FT_FORMAT+1];
};

SPECIF_DYNAR(t_autoindex_dynar, t_autoindex, 1);

struct t_fulltext  // compiled definition of a fulltext system
{ cdp_t cdp;                  // used only when modifying from GUI
  bool modifying, recursive;  // used only when analysing SQL statement (controls recursive analysis of original definition)
                              // modifying used by GUI too, controls the dialog 
  tobjname schema,            // used only when analysing SQL statement
           name;  
  tobjnum objnum;

  int  language;
  bool basic_form;
  bool weighted;
  bool with_substructures;
  bool separated;
  char * limits;        // limits on file sizes, by name patterns
  char * word_starters;
  char * word_insiders;
  bool bigint_id;
  bool rebuild;         // true for the empty ALTER FULLTEXT statement
  t_autoindex_dynar items;
  void compile_intern(CIp_t CI);
  int compile(cdp_t cdp, tptr defin);
  bool compile_flag(CIp_t CI, bool positive);
  tptr to_source(cdp_t cdp, bool altering);
  t_fulltext(void)
    { recursive=false;  modifying=false;  *schema=0;  limits=NULL;  word_starters=word_insiders=NULL;  rebuild=false; }
  ~t_fulltext(void)
    { corefree(limits);  corefree(word_starters);  corefree(word_insiders); }
};

CFNC DllKernel bool WINAPI validate_limits(cdp_t cdp,const char * limits);

struct t_ft_kontext;

struct t_fulltext2 : public t_fulltext  // t_fulltext linkable into a list, with pointer to the fulltext context
{ t_fulltext2 * next;
 private:
  int ref_cnt;
 public:
  void AddRef(void)  { ref_cnt++; }
  void Release(void) { if (--ref_cnt<=0) delete this; }
  t_fulltext2(void)
    { ref_cnt=1; }
  t_ft_kontext * ftk;  // not owning pointer
  void remove_doc(cdp_t cdp, t_docid doc_id);
  //void add_doc(cdp_t cdp, t_docid doc_id, const char * text, const char * format, int mode, t_specif specif);
};

////////////////////////////// various defines ///////////////////////////////////
#ifdef UNIX
#define PATH_SEPARATOR     '/'
#define PATH_SEPARATOR_STR "/"
#else
#define PATH_SEPARATOR     '\\'
#define PATH_SEPARATOR_STR "\\"
typedef int socklen_t;   // on LINUX defined as unsigned
#endif

#define DEFAULT_IP_SOCKET 5001

CFNC DllKernel void WINAPI enc_buffer_total(cdp_t cdp, tptr buf, int size, tobjnum objnum);
#define CLIENT_SERVER_FILE_MAPPING_SIZE (0x5000+30)  // slightly bigger than RD_STEP used in cint.cpp

////////////////////////////// threading /////////////////////////////////
#ifdef WINS

typedef                   unsigned WINAPI THREAD_PROC_TYPE(void *);
#define THREAD_PROC(name) unsigned WINAPI             name(void * data)
#define THREAD_RETURN     return 0
typedef HANDLE THREAD_HANDLE;
BOOL THREAD_START_JOINABLE(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle);
#define THREAD_HANDLE_SELF GetCurrentThread()  // returns pseudo-handle, not usable by other threads or processes

typedef HANDLE LocEvent;
#define CloseLocalAutoEvent(x)       CloseHandle(*x)
#define SetLocalAutoEvent(x)         SetEvent(*x)
#ifdef SRVR
#define CreateLocalAutoEvent(init,x) ((*x=CreateEvent(&SA, FALSE, init, NULL))!=0)
#else
#define CreateLocalAutoEvent(init,x) ((*x=CreateEvent(NULL, FALSE, init, NULL))!=0)
#endif
#define WaitLocalAutoEvent(x,tm)     WaitForSingleObject(*x,tm)

#else
#ifdef UNIX ///////////////////

typedef                   void * THREAD_PROC_TYPE(void *);
#define THREAD_PROC(name) void *             name(void * data)
#define THREAD_RETURN     return NULL
typedef pthread_t THREAD_HANDLE;
BOOL THREAD_START_JOINABLE(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle);
#define THREAD_HANDLE_SELF pthread_self()

#define RenameThread(i, str) /* for now, ignore that on Linux */

#define sigset sigset_system
#include <signal.h>
#undef sigset
extern sigset_t sigset;
inline void block_all_signals()
{
  pthread_sigmask(SIG_BLOCK, &sigset, NULL);
}
#else // NLM /////////////////

CFNC int _beginthread(void (*start_address)(void *), void *stack_bottom, unsigned stack_size, void *arglist);
CFNC void _endthread(void);

typedef                   void THREAD_PROC_TYPE(void *);
#define THREAD_PROC(name) void             name(void * data)
#define THREAD_RETURN     _endthread()  // zpusobuje, ze alokace pameti v CLIB neroste s kazdym spustenim a ukoncenim threadu
typedef HANDLE THREAD_HANDLE;
#define THREAD_START_JOINABLE(a,b,c,d) THREAD_START(a,b,c,d)
#define block_all_signals /* */

#endif
#endif ///////////////////////

BOOL THREAD_START(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle);
#define INVALID_THANDLE_VALUE ((THREAD_HANDLE)0)
void THREAD_JOIN(THREAD_HANDLE thread_handle);
void THREAD_DETACH(THREAD_HANDLE thread_handle);

//////////////////////////////// file I/O support //////////////////////////////////////////
#ifdef WINS
#define INVALID_FHANDLE_VALUE  INVALID_HANDLE_VALUE
#define CloseFile(h) CloseHandle(h)
#define SetLocEventInvalid(LocEvent) LocEvent=INVALID_HANDLE_VALUE
#define SetSemaphoreInvalid(hCheckSemaphore) hCheckSemaphore=INVALID_HANDLE_VALUE
#else
#define INVALID_FHANDLE_VALUE  (FHANDLE)-1
#define CloseFile(h) (::close(h)==0)
#define SetLocEventInvalid(LocEvent) 
#define SetSemaphoreInvalid(hCheckSemaphore) 
#endif
#define FILE_HANDLE_VALID(h) ((h)!=INVALID_FHANDLE_VALUE)  // argument should be of FHANDLE type
///////////////////////////////// database file names //////////////////////////////////////
#define JOURNAL_NAME      "journal.fil"
#define TRANS_NAME        "transact.fil"
#define COMM_JOURNAL_NAME "communic.fil"
#define MAIN_LOG_NAME     "wbsqllog.txt"

//////////////////////////////// auxiliary file structure //////////////////////////////////
#pragma pack(1)
struct t_amo
{ tcateg categ;
  tobjname objname;
};

#define SQL_MAX_DSN_LENGTH               32     /* maximum data source name size*/
struct apx_header // valid from version 5.0
{ unsigned version;
  char   start_name[OBJ_NAME_LEN+1];
  tcateg start_categ;
  char   dest_dsn[SQL_MAX_DSN_LENGTH+1];
  WBUUID dp_uuid; // must be cleared when imported
  WBUUID os_uuid;
  WBUUID vs_uuid;
 // new in version 6.0
  int      sel_language;
  tobjname front_end_name; // "" iff not redirected
  tobjname back_end_name;  // "" iff not redirected
  BOOL     appl_locked;
 // new in version 8.0
  uns32    admin_mode_count;
  t_amo    amo[1];
};
#pragma pack()
///////////////////////////////// common compiler /////////////////////////////////
#define KEY_COUNT      40
#define SQL_KEY_COUNT  174
#ifdef SRVR
extern t_idname     keynames[KEY_COUNT];     /* ipl keyword table */
extern t_idname sql_keynames[SQL_KEY_COUNT]; /* sql keyword table */
#else
extern tname     keynames[KEY_COUNT];     /* ipl keyword table */
extern tname sql_keynames[SQL_KEY_COUNT]; /* sql keyword table */
#endif

#ifdef SRVR
#define COMP_SIGNIF_CHARS  IDNAME_LEN
#else
#define COMP_SIGNIF_CHARS  NAMELEN
#endif

bool search_table(const char * key, const char * patt, int pattnum, int pattstep, int * pos);

// Just for checking the duplicity of column names (in table or cursor)
class t_name_hash
{ unsigned table_size;
  char (*hash_table)[ATTRNAMELEN+1];
  unsigned hash(const char * name);
 public:
  t_name_hash(unsigned total_count);
  bool add(const char * name);  // return false on duplicity
  ~t_name_hash(void)
    { corefree(hash_table); }
};

//////////////////////////////////// general network API ///////////////////////////////////
// Packet types:
#define DT_REQUEST            0xff
#define DT_ANSWER             0xfe
#define DT_REQSIZE            0xfd
#define DT_ANSSIZE            0xfc
#define DT_NUMS               0xfb
#define DT_BREAK              0xfa
#define DT_MESSAGES           0xf9
#define DT_TAB_INVALID        0xf8
#define DT_CONNRESULT         0xf7
#define DT_SERVERDOWN         0xf6
#define DT_EXPORTRQ           0xf5
#define DT_EXPORTACK          0xf4
#define DT_IMPORTRQ           0xf3
#define DT_IMPORTACK          0xf2
#define DT_VERSION            0xf1
#define DT_KEEPALIVE					0xf0
#define DT_MESSAGE            0xef
#define DT_CLOSE_CON          0xee
#define TUNELLED_REQUEST(x)  ((unsigned char)x>=DT_CLOSE_CON)

struct t_message_fragment // fragment of a message to be sent between client and server
{ uns32 length;
  char * contents;  // may be modified by append_byte!
  bool  deallocate_contents;  // corefree(contents) should be called after sending the message
};

class cTcpIpAddress;

class t_fragmented_buffer
// Represents a list of fragments of a message and "current" pointer.
{ friend class cTcpIpAddress;
  t_message_fragment * fragments; // not owning the array
  unsigned count_fragments;   // AX: count_fragments==#of elements in "fragments" array
  unsigned current_fragment;  // AX: 0<=current_fragment<=count_fragments
  unsigned offset_in_current; // AX: 0<=offset_in_current<=fragments[current_fragment].length
 public:
  inline t_fragmented_buffer(t_message_fragment * fragmentsIn, unsigned count_fragmentsIn)
  // Every fragment is supposed to have space for another 1 byte on its end.
  { fragments=fragmentsIn;  count_fragments=count_fragmentsIn; }
  inline void reset_pointer(void)  // set the poistion of the pointer to the beginning of the 1st fragment
  { current_fragment=0;  offset_in_current=0; }
  inline uns32 total_length(void) const // returns the total length of all fragments
  { uns32 total=0;
    for (unsigned i=0;  i<count_fragments;  i++)
      total+=fragments[i].length;
    return total;
  }
  inline uns32 rest_length(void) const // returns the length from current position of the pointer to the end
  { uns32 total=0;
    for (unsigned i=current_fragment;  i<count_fragments;  i++)
      total+=fragments[i].length;
    return total-offset_in_current;
  }
  inline void append_byte(char bt) // adds byte to the last fragment, supposes that there is room for it
  { fragments[count_fragments-1].contents[fragments[count_fragments-1].length++] = bt; }
  inline void copy(char * dest, uns32 length)  // copies from the current position of the pointer and advances the pointer
  // AX: length<=rest_length()
  { uns32 step;
    if (!fragments) return;
    do // iterates on fragments
    { step=fragments[current_fragment].length - offset_in_current;  // - offset_in_current added 18.1.2006
      if (step>length) step=length;
      memcpy(dest, fragments[current_fragment].contents+offset_in_current, step);
      length-=step;
      if (!length)
      { offset_in_current += step;
        break;
      }
      dest+=step;
      offset_in_current=0;
      current_fragment++;
    } while (true);
  }
};

class cAddress;
BOOL Send(cAddress * pAddr, t_fragmented_buffer * frbu, BYTE bDataType);


#pragma pack(1)
struct sWbHeader  // header of each packet in the client-server communication (message may be divided into several packets when using IPX/SPX ot NetBEUI protocols)
{ uns32 wMsgDataTotal; // total length of the whole message (without any headers)
  BYTE  bDataType;
};
#pragma pack()

///////////////////////////////////// trigger types ///////////////////////////////////////////////////
// trigger types: enum not used because bitwise operations often used on it
#define TRG_BEF_INS_ROW   0x1    // bits indicating the presence of a trigger in trigger_map
#define TRG_AFT_INS_ROW   0x2
#define TRG_BEF_UPD_ROW   0x4
#define TRG_AFT_UPD_ROW   0x8
#define TRG_BEF_DEL_ROW   0x10
#define TRG_AFT_DEL_ROW   0x20
#define TRG_BEF_INS_STM   0x100
#define TRG_AFT_INS_STM   0x200
#define TRG_BEF_UPD_STM   0x400
#define TRG_AFT_UPD_STM   0x800
#define TRG_BEF_DEL_STM   0x1000
#define TRG_AFT_DEL_STM   0x2000
#define TRG_TYPE_MASK (TRG_BEF_INS_ROW | TRG_AFT_INS_ROW | TRG_BEF_UPD_ROW | TRG_AFT_DEL_STM |\
                       TRG_AFT_UPD_ROW | TRG_BEF_DEL_ROW | TRG_AFT_DEL_ROW | TRG_BEF_INS_STM |\
                       TRG_AFT_INS_STM | TRG_BEF_UPD_STM | TRG_AFT_UPD_STM | TRG_BEF_DEL_STM)
// trigger scope requirements:
#define TRG_AFT_UPD_OLD   0x40
#define TRG_AFT_DEL_OLD   0x80
#define TRG_BEF_UPD_NEW   0x4000
#define TRG_BEF_INS_NEW   0x8000

#define MODULE_GLOBAL_DECLS "MODULE_GLOBALS"  // name of the object containing global module declarations 
///////////////////////////////// communication encryption ////////////////////////////////////////////////
class t_commenc
{protected:
  bool disabled1;
 public: 
  inline t_commenc(bool disable1st) { disabled1=disable1st; }
  virtual void encrypt(unsigned char * str, unsigned length) = 0;
  virtual void decrypt(unsigned char * str, unsigned length) = 0;
  virtual ~t_commenc(void) { }
};

/* context of random number generator */
typedef uns32 ub4;   /* unsigned 4-byte quantities */
#define RANDSIZL   (8)  /* I recommend 8 for crypto, 4 for simulations */
#define RANDSIZ    (1<<RANDSIZL)

class randctx
{public: 
  friend void isaac(randctx *ctx);
  friend void radinit(randctx *ctx, int flag);
  ub4 randrsl[RANDSIZ];
  ub4 randmem[RANDSIZ];
  ub4 randa;
  ub4 randb;
  ub4 randc;
 public:
  int  bytecnt;
  void init(tptr keyval, int keylen);
  void rand_encode(tptr buf, int size);
};

#define COMM_ENC_KEY_SIZE   30  // must be smaller than RSA block size

class t_commenc_pseudo : public t_commenc
{ randctx ctx;
 public:
  inline t_commenc_pseudo(bool disable1st, unsigned char * key) : t_commenc(disable1st)
    { ctx.init((tptr)key, COMM_ENC_KEY_SIZE); }
  inline t_commenc_pseudo(void) : t_commenc(false)
    { }  // must be initialized by operator =
  inline void encrypt(unsigned char * str, unsigned length)
    { if (disabled1) disabled1=false;  else ctx.rand_encode((tptr)str, length); }
  inline void decrypt(unsigned char * str, unsigned length)
    { if (disabled1) disabled1=false;  else ctx.rand_encode((tptr)str, length); }
  t_commenc_pseudo &operator =(t_commenc & patt)
    { memcpy(&ctx, &((t_commenc_pseudo*)&patt)->ctx, sizeof(randctx)); return *this;}
};

#define MAX_LOG_FORMAT_LENGTH 50
#define FIXED_USER_TABLE_OBJECTS 5

#if WBVERS<900
class t_atrmap // bitmapa pro sloupce tabulky, prirazuje kazdemu sloupci booleovskou hodnotu
{ enum { bits_per_elem = 32, max_attrs = 256 };
  typedef uns32 elemtype;
  elemtype map[(max_attrs-1)/bits_per_elem+1];
 public:
  inline void clear(unsigned count) // vymaze vsechny bity pro "count" sloupcu
    { memset(map, 0, (count-1)/8+1); }
  inline void set_all(unsigned count) // nahodi vsechny bity pro "count" sloupcu
    { memset(map, 0xff, (count-1)/8+1); }
  inline void add(unsigned num)     // nahodi bit pro sloupec "num"
    { map[num / bits_per_elem] |= 1 << (num % bits_per_elem); }
  inline bool has(unsigned num) const
    { return (map[num / bits_per_elem] & 1 << (num % bits_per_elem)) != 0; }
  inline bool intersects(const t_atrmap * other, unsigned count) const  // zjisti neprazdnost pruniku dvou map
  { count=(count-1)/bits_per_elem+1;
    while (count--)
      if (map[count] & other->map[count]) return true;
    return false;
  }
  inline void union_(const t_atrmap * other, unsigned count) // unions the other map's contents into this
  { count=(count-1)/bits_per_elem+1;
    while (count--)
      map[count] |= other->map[count];
  }
  const elemtype * the_map(void) const { return map; }
};
#endif

class t_atrmap_flex // bitmap for any number of items (specified in the constructor)
// Optimized for fast operation on 32-bit units. Optimized for small numbers of bits in the bitmap.
// AXIOM: clear() clears all bits in [unitcount].
// Because the map may be part of a dynar, pmap must not point to static_map!
{ friend class t_atrmap_enum;
  unsigned bitcount, 
           unitcount;
  typedef uns32 map_unit;
  enum { unitbits=32, units_in_static_map = 4 };  // faster operation for most cases
  map_unit static_map[units_in_static_map];
  map_unit * pmap;  // map pointer, allocated when bytecount>bytes_in_static_map, ==static_map otherwise
  inline const map_unit * the_map(void) const { return pmap!=NULL ? pmap : static_map; }
 public:
  inline t_atrmap_flex(void)  
    { bitcount=unitbits*units_in_static_map;  unitcount=units_in_static_map;  pmap=NULL; }
  inline unsigned bytecount(void) const
    { return unitcount*(unitbits/8); }
  inline void init(unsigned bitcountIn)
    { bitcount=bitcountIn;  unitcount=bitcountIn ? (bitcountIn-1)/unitbits+1 : 0;
      corefree(pmap);   // may be initialized twice!
      if (unitcount>units_in_static_map)
        pmap=(map_unit*)corealloc(bytecount(), 53);
      else
        pmap=NULL;
    }
  void expand(unsigned bitcountIn);
  inline t_atrmap_flex(unsigned bitcountIn)
    { pmap=NULL;  init(bitcountIn); }
  inline void release(void)
    { corefree(pmap);
      bitcount=unitbits*units_in_static_map;  unitcount=units_in_static_map;  pmap=NULL;
    }
  inline ~t_atrmap_flex(void)
    { corefree(pmap); }
  inline map_unit * the_map_wr(void) { return pmap!=NULL ? pmap : static_map; }
  inline void clear(void) // vymaze vsechny bity pro "count" sloupcu
    { memset(the_map_wr(), 0,    bytecount()); }
  inline void set_all(void) // nahodi vsechny bity pro "count" sloupcu
    { memset(the_map_wr(), 0xff, bytecount()); }
  inline void add(unsigned num)     // nahodi bit pro sloupec "num"
    { the_map_wr()[num / unitbits] |= 1 << (num % unitbits); }
  void add_exp(unsigned num);     // nahodi bit pro sloupec "num"
  inline bool has(unsigned num) const
    { return (the_map()[num / unitbits] & 1 << (num % unitbits)) != 0; }
  inline bool intersects(const t_atrmap_flex * other) const  // zjisti neprazdnost pruniku dvou map
  { unsigned i = unitcount;
    while (i--)
      if (the_map()[i] & other->the_map()[i]) return true;
    return false;
  }
  inline void union_(const t_atrmap_flex * other) // unions the other map's contents into this, supposes that my map is not smaller
  { unsigned i = other->unitcount;
    while (i--)
      the_map_wr()[i] |= other->the_map()[i];
  }
  inline void union_exp(const t_atrmap_flex * other);
//#ifdef SRVR -- pokus
  inline void mark_core(void)
    { if (pmap) mark(pmap); }
//#endif
  t_atrmap_flex & operator =(const t_atrmap_flex & other)  // same size supposed
    { memcpy(the_map_wr(), other.the_map(), bytecount()); 
      return *this;
    }
  bool operator ==(const t_atrmap_flex & other) const // same size supposed
  { unsigned i = unitcount;
    while (i--)
      if (the_map()[i] != other.the_map()[i]) return false;
    return true;
  }
  inline unsigned get_bitcount(void) const
    { return bitcount; }
};

class t_atrmap_enum
{ int unit_ind, unit_pos;  // the last checked position
  t_atrmap_flex * map;
 public:
  t_atrmap_enum(t_atrmap_flex * mapIn)
    { map=mapIn;  unit_ind=-1;  unit_pos=t_atrmap_flex::unitbits-1; }
  int get_next_set(void)  // returns -1 if none more set
  { do
    { if (++unit_pos==t_atrmap_flex::unitbits)
      { unit_pos=0;
        do
        { unit_ind++;
          if (unit_ind>=map->unitcount) return -1;
        } while (!map->the_map()[unit_ind]);
      }
    } while (!(map->the_map()[unit_ind] & (1<<unit_pos)));
    return unit_ind*t_atrmap_flex::unitbits+unit_pos;
  }
};

#define SIZE_OF_PRIVIL_DESCR(column_count) (1+((column_count)-2)/4+1)

class t_privils_flex // bitmap privileges
{ unsigned column_count; 
 public:
  enum { static_map_col_cnt = 32 };
 private:
  uns8 static_map[1+static_map_col_cnt/4];
  uns8 * pmap;  // map pointer, allocated when column_count>static_map_col_cnt, ==static_map otherwise
 public:
  inline t_privils_flex(void)  
    { column_count=static_map_col_cnt;   pmap=static_map; }
  inline unsigned colbytes(void) const
    { return (column_count-2)/4+1; }
  inline void init(unsigned column_countIn)
    { column_count=column_countIn;
      if (pmap!=static_map) corefree(pmap);   // may be initialized twice!
      if (column_count>static_map_col_cnt)
        pmap=(uns8*)corealloc(colbytes()+1, 54);
      else
        pmap=static_map;
    }
  inline t_privils_flex(unsigned column_countIn)
    { pmap=static_map;  init(column_countIn); }
  inline void release(void)
    { if (pmap!=static_map) corefree(pmap);
      pmap=static_map;
    }
  inline ~t_privils_flex(void)
    { if (pmap!=static_map) corefree(pmap); }
  inline void clear(void) // vymaze vsechny bity pro "colbytes" sloupcu
    { memset(pmap, 0,    colbytes()+1); }
  inline void set_all_rw(void) // nahodi vsechny bity RW pro "colbytes" sloupcu
    { memset(pmap+1, 0xff, colbytes()); }
  inline void set_all_r(void) // nahodi vsechny bity READ pro "colbytes" sloupcu
    { memset(pmap+1, 0x55, colbytes()); }
  inline void set_all_w(void) // nahodi vsechny bity WRITE pro "colbytes" sloupcu
    { memset(pmap+1, 0xaa, colbytes()); }
  inline void add_read(unsigned column)  // nahodi bit READ pro sloupec "colnum"
    { pmap[1+(column-1) / 4] |= (1 << (2*((column-1)%4))); }
  inline void add_write(unsigned column)  // nahodi bit READ pro sloupec "colnum"
    { pmap[1+(column-1) / 4] |= (1 << (2*((column-1)%4)+1)); }
  inline bool has_read(unsigned column) const
    { return (*pmap & RIGHT_READ)!=0 || (pmap[1+(column-1) / 4] &  (1 << (2*((column-1)%4))  )) != 0; }
  inline bool has_write(unsigned column) const
    { return (*pmap & RIGHT_WRITE)!=0 || (pmap[1+(column-1) / 4] &  (1 << (2*((column-1)%4)+1)  )) != 0; }
  inline bool has_del(void) const
    { return (*pmap & RIGHT_DEL) != 0; }
  inline bool has_ins(void) const
    { return (*pmap & RIGHT_INSERT) != 0; }
  inline bool has_grant(void) const
    { return (*pmap & RIGHT_GRANT) != 0; }
  uns8 * the_map(void) { return pmap; }
//#ifdef SRVR pokus
  inline void mark_core(void)
    { if (pmap!=static_map) mark(pmap); }
//#endif
  inline unsigned get_column_count(void) const
    { return column_count; }
};

////////////////////////////////// common for compilers ///////////////////////////////
class t_last_comment
// The comment is always null-terminated
{ t_dynar comm;
 public:
  inline t_last_comment(void) : comm(1, 0, 50) { }
  inline void init(void)  // the effect of the constructor is discarded by memset!
    { comm.init(1,0,50); }
  inline void clear(void)
    { comm.clear(); }
  void add_to_comment(const char * start, const char * stop);
  inline char * get_comment(void) 
    { char * ret = (char*)(comm.count() ? comm.acc0(0) : NULL);  comm.drop();  return ret; }
  inline bool has_comment(void) const
    { return comm.count()>0; }
};

CFNC DllKernel void WINAPI analyse_type(CIp_t CI, int & tp, t_specif & specif, table_all * ta, int curr_attr_num);
CFNC DllKernel void WINAPI sql_type_name(int wbtype, t_specif specif, bool full_name, char * buf);
class t_flstr;
CFNC DllKernel void WINAPI write_collation(t_specif specif, t_flstr & src);
CFNC DllKernel void WINAPI write_constraint_characteristics(int co_cha, t_flstr * src);
const char * ref_rule_descr(int rule);

struct t_type_specif_info
{ int tp;
  t_specif specif;
  tobjname schema, domname;
  bool nullable;
  char * defval;
  char * check;
  uns8 co_cha;
  t_type_specif_info(void)
    { nullable=true;  defval=check=NULL;  co_cha=COCHA_UNSPECIF; }
  ~t_type_specif_info(void)
  { if (defval) corefree(defval);
    if (check)  corefree(check);
  }
};
CFNC DllKernel void WINAPI compile_domain_ext(CIp_t CI, t_type_specif_info * tsi);
////////////////////////////////////////// hints about columns ////////////////////////////////////////
// Names of hints in the definition of a table (ina comment):
#define HINT_CAPTION    "CAPTION"
#define HINT_HELPTEXT   "HELPTEXT"
#define HINT_CODEBOOK   "CODEBOOK"
#define HINT_CODETEXT   "CODETEXT"
#define HINT_CODEVAL    "CODEVAL"
#define HINT_OLESERVER  "OLESERVER"
#define HINT_HELPTEXT_LEN  300
#define HINT_CODEBOOK_LEN  300
#define HINT_OLESERVER_LEN 100
#define HINT_MAXLEN        HINT_HELPTEXT_LEN
CFNC DllKernel void WINAPI get_hint_from_comment(const char * comment, const char * hint_name, char * buffer, int bufsize);
CFNC DllKernel const char * WINAPI skip_hints_in_comment(const char * comment);
//////////////////////////////////////// direct client-server communication //////////////////////////
#define NOTIF_PROGRESS   1
#define NOTIF_SERVERDOWN 2
#define NOTIF_MESSAGE    3
#ifdef WINS
bool CreateIPComm0(const char * server_name, SECURITY_ATTRIBUTES * pSA, HANDLE * hMakerEvent, HANDLE * hMakeDoneEvent, HANDLE * hMemFile, DWORD ** comm_data, bool & mapped_long, bool try_global);
void CloseIPComm0(HANDLE hMakerEvent, HANDLE hMakeDoneEvent, HANDLE hMemFile, DWORD * comm_data);
bool is_server_in_global_namespace(const char * server_name);

class t_dircomm
{// members do not have to be initialised:
  HANDLE hDataEvent, hDoneEvent,  // communication events
         hReqMap, hAnsMap;        // communication space handles
  HANDLE hSrvWait[2];    // used by client and by server when it has process handle of the other side
 public:
  t_request_struct * rqst;   // communication space pointers
  answer_cb * dir_answer;
  unsigned request_part_size, answer_part_size;
  bool smart_wait;  // I have the thread handle of the other side
  bool OpenClientSlaveComm(DWORD id, const char * server_name, SECURITY_ATTRIBUTES * pSA, bool try_global);
  void CloseClientSlaveComm(void);

  bool SendAndWait(void)  // client
    {	SetEvent(hDataEvent);
      if (hSrvWait[1])
        return WaitForMultipleObjects(2, hSrvWait, FALSE, INFINITE) == WAIT_OBJECT_0;
      else
	    { WaitForSingleObject(hDoneEvent, INFINITE);
        return true;
      }
    }
  void PrepareSynchro(HANDLE LocalProcessHandle)  // called by the server
    { hSrvWait[0]=hDataEvent;  hSrvWait[1]=LocalProcessHandle;  smart_wait=LocalProcessHandle!=0; }
  void PrepareSynchroByClient(HANDLE LocalProcessHandle)  // called by the client
    { hSrvWait[0]=hDoneEvent;  hSrvWait[1]=LocalProcessHandle; }
  bool WaitForRequest2(void)  // server
    { if (smart_wait)
        return WaitForMultipleObjects(2, hSrvWait, FALSE, INFINITE) == WAIT_OBJECT_0; 
      WaitForSingleObject(hDataEvent, INFINITE);
      return true;
    }
  void RequestCompleted(void)  // server
    { SetEvent(hDoneEvent); }
  HANDLE DataEvent(void) const
    { return hDataEvent; }

  inline void SetServerDown(void)  // called by the server
    { rqst->comm_flag=SERVER_DOWN_NOT_REPORTED; }
  inline void ClearServerDownFlag(void)  // init, called by the client
    { rqst->comm_flag=NORMAL_MSG; }
  inline void SetServerDownReported(void)  // called by the client
    { rqst->comm_flag=SERVER_DOWN_REPORTED; }
  inline bool IsServerDownNotReported(void) const // checked by the client
    { return rqst && rqst->comm_flag==SERVER_DOWN_NOT_REPORTED; }
  inline bool IsServerDownReported(void) const // checked by the client
    { return rqst && rqst->comm_flag==SERVER_DOWN_REPORTED; }

  inline void SetPartialReq(bool is_partial)  // called by the client
    { rqst->comm_flag = is_partial ? NONFINAL_MSG_PART_NO_LEN : NORMAL_MSG; }
  inline bool IsPartialReq(void) const        // checked by the server
    { return rqst->comm_flag==NONFINAL_MSG_PART_NO_LEN; }
  inline void SetPartialAns(bool is_partial)  // called by the server
    { dir_answer->flags = is_partial ? NONFINAL_MSG_PART_NO_LEN : NORMAL_MSG; }
  inline bool IsPartialAns(void) const // checked by the client
    { return dir_answer->flags==NONFINAL_MSG_PART_NO_LEN; }

  inline void SetMultipartReq(bool is_partial)  // called by the client
    { rqst->comm_flag = is_partial ? MSG_PART_WITH_LEN : NORMAL_MSG; }
  inline bool IsMultipartReq(void) const        // checked by the server
    { return rqst->comm_flag==MSG_PART_WITH_LEN; }
  inline void SetMultipartAns(bool is_partial)  // called by the server
    { dir_answer->flags = is_partial ? MSG_PART_WITH_LEN : NORMAL_MSG; }
  inline bool IsMultipartAns(void) const        // checked by the client
    { return dir_answer->flags==MSG_PART_WITH_LEN; }


  void send_status_nums(trecnum rec1, trecnum rec2);
  void send_notification(int notification_type, const void * notification_parameter, unsigned param_size);

  bool  write_to_client(const void * buf, unsigned size);
  sig32 read_from_client(char * buf, uns32 size);
};

#endif
/////////////////////////////////////////// licences /////////////////////////////////////////////
#define MAX_LICENCE_LENGTH 27
bool get_client_licences(char * ik, const char * prefix, int * lics);
bool valid_addon_prefix(const char * lc);

#ifdef SRVR
#define DllSpecial
#elif defined(VIEW) || defined(INST)
#define DllSpecial DllExport
#else
#define DllSpecial DllImport
#endif
CFNC DllSpecial BOOL WINAPI check_server_registration(void);

CFNC DllKernel uns32 dt_plus (uns32 dt, sig32 diff);
uns32 dt_minus(uns32 dt, sig32 diff);
CFNC DllKernel sig32 dt_diff (uns32 dt1, uns32 dt2);
uns32 ts_plus (uns32 ts, sig32 diff);
uns32 ts_minus(uns32 ts, sig32 diff);
sig32 ts_diff (uns32 ts1, uns32 ts2);
uns32 tm_plus (uns32 tm, sig32 diff);
uns32 tm_minus(uns32 tm, sig32 diff);
sig32 tm_diff (uns32 tm1, uns32 tm2);
unsigned c_year(unsigned year);
unsigned c_month(unsigned month, unsigned year);

CFNC DllKernel uns32 WINAPI displace_time(uns32 tm, int add_this);
CFNC DllKernel uns32 WINAPI displace_timestamp(uns32 tms, int add_this);
CFNC DllKernel void  WINAPI timeUTC2str(uns32 tm, int tzd, tptr txt, int param);
CFNC DllKernel void  WINAPI timestampUTC2str(uns32 dtm, int tzd, tptr txt, int param);
CFNC DllKernel BOOL  WINAPI str2timeUTC(const char * txt, uns32 * tm, bool UTC, int default_tzd);
CFNC DllKernel BOOL  WINAPI str2timestampUTC(const char * txt, uns32 * dtm, bool UTC, int default_tzd);

#ifdef LINUX
extern const char * configfile;  // "/etc/602sql", used by the client and server
#endif

extern const char month_name[12][10];
extern const char mon3_name [12][4];
extern const char day_name  [7][10];
extern const char day3_name [7][4];

#define HTTP_METHOD_NONE  0
#define HTTP_METHOD_GET   1
#define HTTP_METHOD_POST  2
#define HTTP_METHOD_OTHER 3
#define HTTP_METHOD_RESP  10

//#if WBVERS>=900  // otherwise is in dad.h
class t_base64 // support for conversion from base64:
{ char ext_chars[4];
  int  ext_cnt;
 public:
  void reset(void)
    { ext_cnt=0; }
  int decode(unsigned char * tx);
};
//#endif
/////////////////////////////// dynamic libraries ///////////////////////////
#ifdef LINUX
#define LoadLibrary(name) dlopen(name, RTLD_LAZY)
#define FreeLibrary(a) (dlclose(a)==0)
#define GetProcAddress(handle, funcname) (void (*)(void))dlsym(handle, funcname)
#endif
CFNC DllKernel void get_path_prefix(char * path);

#define MAX_FIL_PARTS 4
#define BACKUP_DESCRIPTOR_SUFFIX ".bck"
#define LOB_ARCHIVE_SUFFIX       ".lobs"
#define EXT_FULLTEXT_SUFFIX      ".ftx"
#define BACKUP_NAME_PATTERN_LEN (8+1+2)

#if WBVERS<900
#define CopyFileEx(a,b,c,d,e,f) CopyFile(a,b,(f) & 1)
#endif

#endif // not def WBDEFS
