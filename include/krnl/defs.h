// defs.h - Main declarations for clients
#include "wbdefs.h" // declarations common for client and server

#if WBVERS>=1000
#define WIDE_ODBC  // trying to solve the encoding problems in ODBC - using wide chars on the application side
#endif

//////////////////////////////////// client compiler structures //////////////////////////////////
struct objdef    /* object definition */
{ tname name;          /* object's name */
  union object * descr;      /* pointer to the description of the object */
};
#pragma pack(1)
typedef struct {
  uns8 typecateg;
  unsigned valsize;
               } anytype;
struct simpletype {
  uns8 typecateg;
  unsigned valsize;
  uns8 typenum;
//  simpletype(uns8 typecategIn, unsigned valsizeIn, uns8 typenumIn) : 
//    typecateg(typecategIn), valsize(valsizeIn), typenum(typenumIn) { }
};
struct simpledbtype {
  uns8 typecateg;
  unsigned valsize;
  uns8 typenum;
};
typedef struct {
  uns8 typecateg;
  unsigned valsize;
  sig32    lowerbound;
  uns32    elemcount;
  struct typeobj * elemtype;
               } arraytype;
typedef struct {
  uns8 typecateg;
  unsigned valsize;   /* limited to 0x7ffff */
  uns32 specif;   // t_specif in fact
               } stringtype;
typedef struct {
  uns8 typecateg;
  unsigned valsize;   /* limited to 0xff */
               } binarytype;
typedef struct {
  uns8 typecateg;
  unsigned valsize;
  struct objtable * items;
               } recordtype;
typedef struct {
  uns8 typecateg;
  unsigned valsize;
  struct typeobj * domaintype;
               } ptrtype;
typedef struct {
  uns8 typecateg;
  unsigned valsize;
  struct typeobj * domaintype;
  tobjname domainname;
               } fwdptrtype;
struct viewtype 
{ uns8     typecateg;
  unsigned valsize;
  tobjname viewname;
  tobjnum  objnum; // used by compiler only, not valid at run time
};
// values if typecateg:
#define T_ARRAY  128
#define T_RECORD 129
#define T_PTR    130
#define T_FILE   131
#define T_STR    132
#define T_RECCUR 135
#define T_BINARY 136
#define T_PTRFWD 137
#define T_VIEW   138
#define T_SIMPLE    139
#define T_SIMPLE_DB 140

typedef union typedescr {
  arraytype    array;
  anytype      anyt;
  recordtype   record;
  stringtype   string;
  binarytype   binary;
  ptrtype      ptr;
  fwdptrtype   fwdptr;
  viewtype     view;
  simpledbtype simpledb;
  simpletype   simple;
              } typedescr;
#define O_CONST   1
#define O_TYPE    2
#define O_PROC    3
#define O_SPROC   4
#define O_FUNCT   5
#define O_VAR     6
#define O_VALPAR  7
#define O_REFPAR  8
#define O_RECELEM 9
#define O_unused  10
#define O_CURSOR  11
#define O_AGGREG  12
#define O_TABLE   13
#define O_METHOD  14
#define O_PROPERTY 15

typedef struct { /* beginning of the description of any object */
  uns8 categ;
               } anyobj;
typedef struct typeobj { /* description of a type */
  uns8 categ;
  typedescr type;
               } typeobj;
typedef struct { /* description of a cursor or a table */
  uns8 categ;
  code_addr offset;  /* this is code offset! */
  tobjnum defnum;
  typeobj * type;
               } cursobj;
typedef struct { /* description of a variable, parameter, recelem */
  uns8 categ;
  typeobj * type;
  unsigned offset;
  uns8 ordr;
  wbbool multielem;   /* used only in cursor describing records */
               } varobj;
typedef struct { /* description of a user-defined front-end subroutine - not used on server */
  uns8 categ;
  code_addr code;
  unsigned localsize;
  struct objtable * locals;
  typeobj * retvaltype;
  uns16 saved_ordr;
               } subrobj;
typedef struct {
  uns8 categ;
  uns8 dtype;
               } spar;

//#ifdef LINUX
//#pragma pack()
//#endif
typedef struct { /* description of a constant */
  uns8 categ;
  basicval0 value;
  typeobj * type;
               } constobj;

typedef struct { /* description of a standard procedure/function - on client and server sides */
  uns8 categ;
  uns16 sprocnum;
  uns16 localsize;  /* never is more for std. PF */
  spar * pars;
  uns8 retvaltype;  /* 0 iff no return value, cannot be composed */
               } procobj;
//#ifdef LINUX
//#pragma pack(1)
//#endif

typedef struct {
  uns8 categ;
  uns8 ag_type;
               } aggrobj;
struct propobj
{ uns8 categ;
  uns16 method_number;
  uns8  dirtype;
};
struct methobj  // beginning is the same as for procobj
{ uns8 categ;
  uns16 sprocnum;
  uns16 localsize;  
  spar * pars;
  uns8 retvaltype;  /* 0 iff no return value, cannot be composed */
  char * pardescr;
};
//struct infoviewobj
//{ uns8 categ;
//  uns16 ivnum;
//  const t_ivcols * cols;
//};

union object {   /* various object categories */
  anyobj   any;
  constobj cons;
  typeobj  type;
  subrobj  subr;
  procobj  proc;
  varobj   var;
  cursobj  curs;
  aggrobj  aggr;
  propobj  prop;
  methobj  meth;
//  infoviewobj iv;
};
#pragma pack()

struct objtable    /* table of object names */
{ objtable * next_table; /* ptr to the table on the higher level */
  unsigned   objnum;
  unsigned   tabsize;
  objdef     objects[1];            /* an array of object definitions */
};

DllKernel extern typeobj 
  att_boolean,
  att_char,
  att_int16,
  att_int32,
  att_money,
  att_float,
  att_string,
  att_binary,
  att_time,
  att_date,
  att_timestamp,
  att_ptr,
  att_biptr,
  att_autor,
  att_datim,
  att_hist,
  att_raster,
  att_text,
  att_nospec,
  att_signat,
  att_untyped,
  att_marker,
  att_indnum,
  att_interval,
  att_nil,
  att_file,
  att_table,
  att_cursor,
  att_dbrec,
  att_varlen,
  att_statcurs,
  att_int8,
  att_int64,
  att_domain;
DllKernel extern typeobj * simple_types[ATT_AFTERLAST];

#define DIRECT(x)     ((x)->type.anyt.typecateg==T_SIMPLE || (x)->type.anyt.typecateg==T_SIMPLE_DB)
#define DIRTYPE(x)    ((x)->type.simple.typenum)
#define ATT_TYPE(x)   DIRTYPE(x)

////////////////////////////// client-specific global declarations: //////////////////////////////
#ifdef WINS
#define READ_WRITE OF_READWRITE
#define WRITE      OF_WRITE
#define READ       OF_READ
#endif

#define SEEK_SET        0         /* seek from start of file      */
#define SEEK_CUR        1       /* relative to current position */
#define SEEK_END        2       /* relative to end of file      */

#if WBVERS>=900
#define EXX_CODE_VERSION (900*10+0)
#else
#define EXX_CODE_VERSION (800*10+9)
#endif
/******************************** kontexts **********************************/
#define CATEG_RECCUR     28
#define CATEG_D_TABLE    30
#define CATEG_TABLE_ALL  31

typedef struct kont_descr
{ struct    kont_descr * next_kont;
  tcateg    kontcateg;  /* CATEG_TABLE, CATEG_CURSOR, CATEG_DIRCUR, CATEG_RECCUR, etc. */
  tobjnum   kontobj;    /* table or cursor object number or open cursor */
  char      tabname[OBJ_NAME_LEN+1];    /* used only by SELECT compilation */
  char      schemaname[OBJ_NAME_LEN+1]; /* used only by SELECT compilation */
  void    * record_tobj;
} kont_descr;

struct dllprocdef
{ FARPROC procadr;
  uns16   parsize;
  uns8    fnctype;
  char    names[EMPTY_ARRAY_INDEX];
};

/******************************* Kategorie: *********************************/
#define XCATEG_SERVER     28
#define XCATEG_UNCONN     29
#define XCATEG_CATEG      30
#define XCATEG_SYSTEM     31
#define XCATEG_ODBCDS     32

#define POI_NOOBJECT      0x0007FFFF

union t_pckobjinfo
{private: 
  uns32 pckinfo;
  struct
  { uns32 objnum : 19;
    uns32 categ  :  5;
    uns32 flags  :  8;
  };
 public:
  inline t_pckobjinfo(uns32 pckinfoIn) : pckinfo(pckinfoIn) { }
  inline t_pckobjinfo(uns32 objnumIn, uns32 categIn, uns32 flagsIn = 0) :
    objnum(objnumIn), categ(categIn), flags(flagsIn) { }
  inline void set(uns32 pckinfoIn) { pckinfo=pckinfoIn; }
  inline void set(uns32 objnumIn, uns32 categIn, uns32 flagsIn) 
    { objnum=objnumIn;  categ=categIn;  flags=flagsIn; }
  inline uns32    get_pck   (void) const { return pckinfo; }
  inline tobjnum  get_objnum(void) const // table should be sign-extended, used by ODBC table numbers
    { return  categ==CATEG_TABLE && (objnum&0x7c000)==0x74000 ? (objnum|ODBC_TABLE) : objnum; }
  inline tcateg   get_categ (void) const { return (tcateg)categ; }
  inline unsigned get_flags (void) const { return (unsigned)flags; }
};
  
CFNC DllKernel tptr WINAPI load_blob(cdp_t cdp, tcursnum cursnum, trecnum rec, tattrib attr, t_mult_size index, uns32 * length);
CFNC DllKernel tptr WINAPI cd_Load_objdef(cdp_t cdp, tobjnum objnum, tcateg category, uns16 *pFlags = NULL);
CFNC DllKernel BOOL WINAPI cd_Load_prefix(cdp_t cdp, tobjnum objnum, tptr descr);
CFNC DllKernel void WINAPI enc_buffer_total(cdp_t cdp, tptr buf, int size, tobjnum objnum);
CFNC DllKernel BOOL WINAPI enc_copy_to_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
                           uns16 index, FHANDLE hFile, const char * objname);
CFNC DllKernel BOOL WINAPI enc2_copy_to_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
                           uns16 index, FHANDLE hFile, const char * objname);
CFNC DllKernel tptr WINAPI enc_read_from_file(cdp_t cdp, FHANDLE hFile, WBUUID uuid, const char * objname, int * objsize);
#define ENCRYPTED_CATEG(cat) ((cat)==CATEG_TABLE || (cat)==CATEG_VIEW || (cat)==CATEG_CURSOR ||\
  (cat)==CATEG_MENU || (cat)==CATEG_PGMSRC || (cat)==CATEG_WWW || \
  (cat)==CATEG_DRAWING || (cat)==CATEG_REPLREL || \
  (cat)==CATEG_PROC || (cat)==CATEG_TRIGGER || (cat)==CATEG_XMLFORM)

typedef struct
{ uns8 astate;
  tptr newval;
  uns8 orig_type;
} atr_state;

extern "C" {
DllKernel const d_table * WINAPI cd_get_table_d(cdp_t cdp, tobjnum tb, tcateg cat);
DllKernel void WINAPI release_table_d(const d_table * td);
DllKernel BOOL WINAPI find_attr(cdp_t cdp, tobjnum tb, tcateg cat,
   const char * name1, const char * name2, const char * name3, d_attr * info);
DllKernel int  WINAPI first_user_attr(const d_table * td);
DllKernel const char * WINAPI set_this_form_source(cdp_t cdp, const char * source);

DllKernel BOOL WINAPI convert_from_string(int tp, const char * txt, void * dest, t_specif specif);
DllKernel void WINAPI conv2str(basicval0 * bas, int tp, tptr txt, int prez, t_specif specif = t_specif());
DllKernel void WINAPI convert2string(const void * val, int tp, char * txt, int prez, t_specif specif);
DllKernel char * WINAPI convert_w2s(const wuchar * wstr, t_specif specif);
DllKernel wuchar * WINAPI convert_s2w(const char * str, t_specif specif);
DllKernel int WINAPI wb_stricmp(cdp_t cdp, const char * str1, const char * str2);
DllKernel int WINAPI wb2_stricmp(int charset, const char * str1, const char * str2);
DllKernel void WINAPI wb_small_string(cdp_t cdp, char * s, bool captfirst);
}
void WINAPI destroy_cursor_table_d(cdp_t cdp);

#include "wbcdp.h"

#define MAX_MAX_TASKS (500+3)       // max connections from a client process (ISAPI needs many of them)
extern cdp_t cds[MAX_MAX_TASKS];   /* must not be based here */

#ifdef WINS
#define GetClientId GetCurrentThreadId   /* WINS */
#else
#ifdef UNIX
#define GetClientId pthread_self         /* UNIX */
#else
#define GetClientId GetThreadID          /* NLM */
#endif
#endif

void write_ans_proc(cd_t * cdp, answer_cb * ans);
extern HINSTANCE hKernelLibInst;
#if WBVERS<900
#ifdef CLIENT_GUI
void status_text(unsigned txid);
#endif
#endif

BOOL total_free(uns8 tp);
int get_client_protocol(const char * serv_name);

// conversions used in wprocs and stdcall:
struct valstring
  { char s[256]; };
tptr     WINAPI sp_strinsert(tptr source, tptr dest, unsigned index);
tptr     WINAPI sp_strdelete(tptr s, unsigned index, unsigned count);
tptr     WINAPI sp_strcat(tptr s1, tptr s2);
tptr     WINAPI sp_strcopy(cdp_t cdp, valstring s, unsigned index, unsigned count);
int      WINAPI sp_strpos(tptr subs, tptr s);
tptr     WINAPI sp_strtrim(tptr s);

tptr WINAPI sp_int2str(cdp_t cdp, sig32 val);
tptr WINAPI sp_bigint2str(cdp_t cdp, sig64 val);
tptr WINAPI sp_money2str(cdp_t cdp, monstr val, int prez);
tptr WINAPI sp_real2str(cdp_t cdp, double val, int prez);
tptr WINAPI sp_date2str(cdp_t cdp, uns32 val, int prez);
tptr WINAPI sp_time2str(cdp_t cdp, uns32 val, int prez);
tptr WINAPI sp_timestamp2str(cdp_t cdp, uns32 val, int prez);

struct val2uuid
{ WBUUID id_user;
  WBUUID id_home;
};
void WINAPI sp_memmov(void * dest, void * source, unsigned size);
BOOL WINAPI sp_open_curs(cdp_t cdp, tcursnum * cdf);
BOOL WINAPI sp_restore_curs(cdp_t cdp, tcursnum * cdf);
BOOL WINAPI sp_close_curs(cdp_t cdp, tcursnum * cdf);
BOOL WINAPI sp_restrict_curs(cdp_t cdp, tcursnum * cdf, char * constrain);
BOOL WINAPI sp_open_sql_c(cdp_t cdp, tcursnum * cursnum, char * def);
BOOL WINAPI sp_open_sql_p(cdp_t cdp, tcursnum * cursnum, valstring d1, valstring  d2, valstring d3, valstring d4);
BOOL WINAPI sp_user_in_group(cdp_t cdp, tobjnum user, tobjnum group, wbbool * state);
BOOL WINAPI sp_sql_submit(cdp_t cdp, tptr sql_stat);
BOOL WINAPI sp_c_avg(cdp_t cdp, tcursnum curs, tptr attrname, tptr condition, untstr * unt);
BOOL WINAPI sp_c_max(cdp_t cdp, tcursnum curs, tptr attrname, tptr condition, untstr * unt);
BOOL WINAPI sp_c_min(cdp_t cdp, tcursnum curs, tptr attrname, tptr condition, untstr * unt);
BOOL WINAPI sp_c_sum(cdp_t cdp, tcursnum curs, tptr attrname, tptr condition, untstr * unt);
trecnum WINAPI sp_look_up(cdp_t cdp, tcursnum curs, const char * attrname, untstr * unt);
void WINAPI sp_err_mask(cdp_t cdp, wbbool mask);
char * WINAPI Current_application(cdp_t cdp);
BOOL WINAPI sp_SQL_exec_prepared(cdp_t cdp, uns32 handle);
CFNC DllExport BOOL WINAPI Waits_for_me(val2uuid uuids);

// copies
#define STBR_IMPEXP      2002
#define FILTER_TDT          1813
#define EXT_DATE_FORMATS      6

#ifndef __x86_64
#if defined(SRVR) || (WBVERS<900)
void *operator new(size_t size);
void operator delete(void * ptr);
#endif
#endif

extern  unsigned WB_COLOR_LTGRAY;
#define SYSCOLORPART(x) (((x)>>24)-1)
#define WBSYSCOLOR(x)   (((x)+1)<<24)
#define RGBCOLOR(x)     ((x) & 0xffffff)
#define DYNAMIC_COLOR   0x7f000000
#define DYNAMIC_SYSPART 0x7e

#define USERNAME_SIZE 30
#define PRODNUM_SIZE  21

//////////////////////////////// client errors //////////////////////////////
#define BARCODE_LIB_NOT_FOUND    5000  // ==FIRST_CLIENT_ERROR
#define BARCODE_BAD_LENGTH       5001
#define BARCODE_BAD_CHAR         5002
#define BARCODE_OTHER_ERR        5003
#define DATAPROC_NO_CONTENTS     5004
#define MEMORY_CRASH             5005
#define MEMORY_NOT_EXTENDED      5006
#define MEMORY_EXTENDED          5007
#define DTP_CTL_BAD_TYPE         5008
#define CAL_CTL_BAD_TYPE         5009
#define WBVIEWED_NOT_FOUND       5010
#define WBCEDIT_NOT_FOUND        5011 
#define BAD_WBVIEWED             5012
#define BAD_WBCEDIT              5013 
#define COMCTL_NOT_IMPLEM        5014
#define NO_SUBVIEW_NAME          5015
#define SUBVIEW_SAME_NAME        5016
#define RELATION_NOT_USABLE      5017
#define NOT_ON_FICT              5018
#define ANONYM_CANNOT_SIGN       5019
#define ENCRYPTED_NO_EXPORT      5020
#define NESTING_POPUP_MENUS      5021
#define DESCRIPTOR_CACHE_FAILURE 5022
#define CANNOT_IN_GENERAL_FORM   5023
#define CANNOT_IN_STD_FORM       5024
#define CANNOT_IN_CLOSED_FORM    5025
#define VIEW_NOT_FOUND           5026
#define XMLEXT_NOT_FOUND         5027
#define BAD_XMLEXT               5028
#define OBJ_CANNOT_RENAME        5029
#define REF_CANNOT_RENAME        5030
#define MEMO_FILE_NOT_FOUND9     5031
#define MOVE_DATA_ERROR          5032
#define DD_ERR_CLIENTS           5033  
#define DD_ERR_CANNOT_LOCK       5034
#define DD_ERR_NOT_CONFIG_ADMIN  5035
#define DD_ERR_NOT_DB_SECUR_ADMIN 5036
#define DD_ERR_CREATE_FILE       5037
#define DD_ERR_NEW_DB_NOT_EMPTY  5038
#define DD_ERR_CANNOT_MOVE_DELETE 5039

#define CLIENT_ERR_WITH_MSG(err) ((err)==VIEW_NOT_FOUND || (err)==MOVE_DATA_ERROR)

#define sys_stricmp(a,b) my_stricmp(a,b)
CFNC DllKernel tptr WINAPI get_separator(int num);

/////////////////////////////////// ODBC on Linux support /////////////////////////
#ifdef LINUX
#define SUCCEEDED(retcode) ((retcode)==SQL_SUCCESS || (retcode)==SQL_SUCCESS_WITH_INFO)
#endif
//////////////////////////// import - export container ///////////////////////////
struct 
#if WBVERS<=810
       DllKernel 
#endif
                 t_container
{ BOOL opened;      /* TRUE iff all vars valid */
  BOOL IsGlobStor;
  HGLOBAL hData;
  FHANDLE File;
  tptr buf;
  uns32 datapos;
  uns32 bufsize;
  uns32 rdpos;

  BOOL ropen(FHANDLE file);
  BOOL wopen(FHANDLE file);
  HGLOBAL close(void);
  t_container(void);
  ~t_container(void);
  unsigned write(tptr data, unsigned size);
  unsigned read (tptr dest, unsigned size);
  void put_handle(HGLOBAL hGlob);
};

#if WBVERS<=810
#include "rcdefs.h"
#endif
