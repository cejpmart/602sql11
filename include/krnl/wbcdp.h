#ifndef __CDP_H__
#define __CDP_H__

/***************************** interp vars **********************************/
typedef struct {    /* subroutine full address */
//  tptr   code;
  uns32  offset;
     } subracc;

#pragma pack(1)
typedef union {
  struct s0
  { uns8   type;
    uns32  specif;
    uns8   val[8];
  } uany;
  struct s1
  { uns8   type;
    uns32  specif;
    double val;
  } udouble;
  struct s2
  { uns8   type;
    uns32  specif;
    sig32 val;
  } usig32;
  struct s3
  { uns8   type;
    uns32  specif;
    monstr val;
  } umonstr;
  struct s4
  { uns8   type;
    uns32  specif;
    tptr   val;
    uns32  valsize;
  } uptr;
} untstr;
#pragma pack()
#define UNT_SRCDB   1   /* flags for untyped assignment */
#define UNT_DESTDB  2
#define UNT_DESTPAR 4

typedef struct {
  trecnum  position; /* this order is important, position may be used as sig32 */
  tcursnum cursnum;
  tattrib  attr;
  uns16    index;
     } curpos;

typedef union {   /* the basic simple value - 8 bytes */
  sig64   int64;
  sig32   int32;
  byte    money[6];
  monstr  moneys;
  double  real;
  sig16   int16;
  uns8    uint8;
  sig8    int8;
  tptr    ptr;
  curpos  wbacc;
  subracc subr;
  untstr * unt;
              } basicval;

#define GR_LEVEL_CNT 6  /* number of aggregating levels, 0=report */

struct aggr_info
{ aggr_info * next_info;
  uns16 codesize;
  uns16 valsize;
  t_specif specif;
  trecnum count[GR_LEVEL_CNT]; /* number of non-none values */
  uns8  type;
  char  vls[1];
};

typedef struct
{ aggr_info * aggr_head;
  wbbool  aggr_completed [GR_LEVEL_CNT];
  trecnum aggr_added     [GR_LEVEL_CNT];
  trecnum aggr_count     [GR_LEVEL_CNT];
  uns16   grp_val_offs   [GR_LEVEL_CNT];
    /* total size & partic. offsets of group determining values */
  tptr grp_vals;  /* the group values */
  uns8 glob_level;
} aggr_descr;

typedef struct scopevars    /* space for variables of a scope */
{ struct   scopevars * next_scope;
  unsigned retadr;
  uns8 *   retcode;
  uns8     wexception;
  unsigned localsize;
  char     vars[1];
} scopevars;

#define I_STACK_SIZE 20
#define MAX_CURS_TABLES   32
#define MAX_KONT_SP  MAX_CURS_TABLES /* must not be less than MAX_CURS_TABLES */

struct objtable;
struct compil_info;

#define MAX_SOURCE_NUMS 50
struct pgm_header_type
{ uns16 version;     // compilation and bytecode version
  uns16 platform;    // platform the EXX has been created on
  uns32 main_entry;  // offset of the program entry point
  unsigned proj_vars_size;   /* -1 if not a base program (for starter) */
  uns32 code_size;   /* incl. header */
  uns32 decl_size;   /* in the file-form */
  uns32 pd_offset;   /* logical offset of the main table */
  int   language;    // the language the program is compiled for
  uns32 compil_timestamp;
  uns32 unpacked_decl_size;
  tobjnum sources[MAX_SOURCE_NUMS]; // object numbers of all includes
};

struct global_project_type
{ tobjname   project_name;
  char     * proj_decls;   // owning ptr to global decls 
  objtable * global_decls; // non-owning ptr to the main objtable of global decls
  pgm_header_type * common_proj_code;
  char     * prev_proj_decls;   // owning ptr to invalid global decls 
  objtable * prev_global_decls; // non-owning ptr to the invalid main objtable of global decls
  compil_info * running_program_compilation;
};

/* Values of bp_type */
#define BP_NORMAL      0
#define BP_TEMPORARY   1

typedef struct
{ uns32 bp_offset;
  unsigned bp_line;
  uns8  bp_file;
  uns8  bp_byte;
  uns8  bp_type;
} bp_info;

typedef struct
{ unsigned bp_count;
  unsigned bp_space;  /* total space: existing bps & free cells */
  unsigned current_line;  /* used for single-line stepping & for returning ... */
  uns8  current_file;  /* ... the current line/file on any break */
  BOOL  realized;
  uns8 * code_addr;
  bp_info bps[1];
} breakpoints;

typedef struct
{ uns16 linenum;
  uns8  filenum;
#ifndef WINS
  uns8  padding;
#endif
  uns32 codeoff;
} lnitem;

#define DBGI_ALLOC_STEP  300

typedef struct dbg_info
{ unsigned filecnt;
  unsigned itemcnt;
  unsigned freecells;
#ifndef WINS
  uns8  padding[6];
#endif
  lnitem * lastitem;
  lnitem   items[2];
  void *operator new(size_t size)
    { return corealloc(size, 255); }
  void operator delete(void * ptr)
    { corefree(ptr); }
  dbg_info(uns32 code_offset, tobjnum src_objnum)
  { freecells=0;  filecnt=1;  itemcnt=1;
    items[0].codeoff=src_objnum;
    items[1].filenum=0;  items[1].linenum=1;  items[1].codeoff=code_offset;
    lastitem=items+1;
  }
  inline tobjnum srcobjs(unsigned filenum)
    { return (tobjnum)items[filenum].codeoff; }
  inline void set_srcobj(unsigned filenum, tobjnum objnum)
    { items[filenum].codeoff=objnum; }
  inline lnitem * lnitems(void)
    { return (lnitem *)(items+filecnt); }
} dbg_info;

#define MAX_CURS_EVID  10
#define MAX_OPEN_FILES  5

/****************************** dynamic array *******************************/
SPECIF_DYNAR(usptr_dynar,untstr*,2);

typedef struct key_trap
{ unsigned keycode;
  unsigned msgnum;
  struct key_trap * next;
  BOOL with_shift;
  BOOL with_ctrl;
  BOOL with_alt;
} key_trap;

//////////////////////////// message queue //////////////////////////////////
// Queue is an array of fixed head (ind. 0) and terminating 0 in msg the field
// after the tail (if not full)

class 
#if WBVERS<=810
      DllPrezen 
#endif
                 t_msg_queue
{
 private:
  typedef struct
  { unsigned msg;    /* zero iff terminating item (no message) */
    HWND     hWnd;
  } t_intern_msg;
  enum { INT_MSG_QUEUE_SIZE = 8 };  // queue size
  t_intern_msg queue[INT_MSG_QUEUE_SIZE];

 public:
  t_msg_queue(void) { queue[0].msg=0; }   // not cenessary, done by cdp_init
  void clear(void)  { queue[0].msg=0; }
  BOOL empty(void)  { return !queue[0].msg; }
  BOOL insert       (unsigned msgIn, HWND hWndIn);
  BOOL insert_nodupl(unsigned msgIn, HWND hWndIn);
  BOOL pick_msg     (unsigned * msgOut, HWND * hWndOut);
  void remove_msg   (unsigned msg);
};

///////////////////////////// view synchronization //////////////////////////
#define MAX_REL_ATTRS 4

typedef struct
{ HWND hWndFrom;
  HWND hWndTo;
  UINT ctId;       /* Id of the synchronizing control in the source view */
  tptr dest_expr;  /* NULL for record synchronization, expr for relational */
  int  owner_offset;  /* -1 if synchronized with sel_rec, otherwise synchro with toprec+owner_offset */
  bool by_relation;
  bool by_user;
  tattrib atrsFrom[MAX_REL_ATTRS], // synchronizing attribute numbers in view cursors
          atrsTo  [MAX_REL_ATTRS];
} t_synchro;

SPECIF_DYNAR(synchro_dynar,t_synchro,1);

#define STR_BUF_SIZE (2*(MAX_FIXED_STRING_LENGTH+1))
struct view_dyn;

typedef struct saved_run_state
{ scopevars    * auto_vars;
  scopevars    * prep_vars;
  scopevars    * glob_vars;
  HGLOBAL      glob_vars_handle;  // not used
  unsigned     pc;
  uns8         * runned_code;
  basicval     st[I_STACK_SIZE];
  basicval     * stt;  /* pointer to the stack top (to full cell) */
#ifndef WINS
  LONGJMPBUF   run_jmp;
#endif
  wbbool       mask;
  tcursnum     kont_c[MAX_KONT_SP];
  trecnum      kont_p[MAX_KONT_SP];
  tptr         index_ptr;
  void         * view_ptr;
  basicval     testedval;    /* for constrains testing & history descr. */
  global_project_type run_project;
  uns8         volatile wexception;
  wbbool       prog_running;
  char         hyperbuf[2*(MAX_FIXED_STRING_LENGTH+1)];  /* database access buffer */
  aggr_descr   * aggr;
  uns16        sys_index;    /* must be saved because a view action may open another view */
  uns32        view_position;
  uns16        current_page_number;
  view_dyn *   inst;  // changed from hWnd for 9.0!
  wbbool       appended;
  trecnum      append_posit;
  char         strbuf[STR_BUF_SIZE];  /* for strcat & I_MOVESTR */
  char         charstr[2];
  struct saved_run_state * prev_run_state;
} saved_run_state;

typedef struct run_state
{ scopevars    * auto_vars;
  scopevars    * prep_vars;
  scopevars    * glob_vars;
  HGLOBAL      glob_vars_handle;  // not used
  unsigned     pc;
  uns8         * runned_code;
  basicval     st[I_STACK_SIZE];
  basicval     * stt;  /* pointer to the stack top (to full cell) */
#ifndef WINS
  LONGJMPBUF   run_jmp;
#endif
  wbbool       mask;
  wbbool       non_mdi_client;
  tcursnum     kont_c[MAX_KONT_SP];
  trecnum      kont_p[MAX_KONT_SP];
  tptr         index_ptr;
  void         * view_ptr;
  basicval     testedval;    /* for constrains testing & history descr. */
  global_project_type run_project;
  uns8         volatile wexception;
  wbbool       prog_running;
  char         hyperbuf[2*(MAX_FIXED_STRING_LENGTH+1)];  /* database access buffer */
  aggr_descr   * aggr;
  uns16        sys_index;
  uns32        view_position;
  uns16        current_page_number;
  view_dyn *   inst;  // changed from hWnd for 9.0!
  wbbool       appended;
  trecnum      append_posit;
  char         strbuf[STR_BUF_SIZE];  /* for strcat & I_MOVESTR */
  char         charstr[2];
  saved_run_state * prev_run_state;
 /* not saved vars: */
  void *       files[MAX_OPEN_FILES];
  wbbool       holding;  /* =waiting on server response, used only on client side */
  wbbool       volatile break_request;
  uns8         global_state;
  bool         interrupt_disabled;
  t_msg_queue  int_msg_queue;  // internal message queue for apps in IPL
  HWND         hClient;
  libinfo      * dllinfo;  /* list of installed DLLs */
  BOOL         unloadable_libs;
  breakpoints  * bpi;
  dbg_info     * dbgi;
  void *       default_tool_bar;
  char         helpfile[80];       /* help file pathname */
  BOOL         htmlhelp;           // current helpfile is CHM-file for HtmlHelp
  int          report_number;
  uns32        comp_err_line;
  uns16        comp_err_column;
  usptr_dynar  registry;
  key_trap *   key_registry;
  synchro_dynar synchros; // is not destructed id the proper way!!
} run_state;

/********************************** odbc *************************************/
#ifndef WINS
#define WINDOWS
#endif

#define DWORD sql_DWORD
#define LPDWORD sql_LPDWORD
#include <sqltypes.h>
#undef LPDWORD
#undef DWORD
#include <sql.h>
#include <sqlext.h>

#ifdef WINS
#include <sqlucode.h>
#endif

#if !defined(_WIN64) && !defined(SQLLEN)  // using old sqltypes.h, adding missing decls.
#define SQLLEN SQLINTEGER
#define SQLULEN SQLUINTEGER
#endif

//#ifdef WINS
#define TI_POINTER     1    /* type_info flags */
#define TI_INDEXABLE   2
#define TI_TRACKING    4
#define TI_IS_MONEY    8
#define TI_IS_AUTOINCR 16

typedef struct
{ tptr  strngs; /* ds type name, local type name, prefix, suffix, create pars */
  SWORD sql_type;
  uns32 maxprec;
  sig16 nullable;
  sig16 searchable;
  unsigned flags;
  int   wbtype;
} xtype_info;
CFNC DllKernel int type_WB_2_ODBC(xtype_info * ti, int wbtype);

typedef struct
{ theapsize maxsize;
  char      data[1];
} cached_longobj;
typedef struct
{ SQLLEN actsize;  // 64-bit ODBC needs 64-bit length
  cached_longobj * obj;
} indir_obj;

typedef struct
{ unsigned offset;
  unsigned size;
  t_specif specif;
  uns16    flags;
  uns8     type;
  uns8     multi;
  uns16    odbc_precision;
  uns16    odbc_scale;
  uns16    odbc_sqltype;
  wbbool   changed;  /* changed && identif implies copied */
} atr_info;

struct t_connection;

typedef struct
{ uns8 link_type;  // = LINKTYPE_ODBC
  char dsn[SQL_MAX_DSN_LENGTH+1];
  char tabname  [128+1];
  char owner    [128+1];
  char qualifier[128+1];
} t_odbc_link;

typedef uns16 RPART;
#define BITSPERPART 16

#define CA_INDIRECT   1
#define CA_MULTIATTR  2
#define CA_NO_READ    4
#define CA_NO_WRITE   8
#define CA_ROWID      0x10
#define CA_ROWVER     0x20
#define CA_NOEDIT     0x40
#define CA_RI_SELECT  0x100
#define CA_RI_INSERT  0x200
#define CA_RI_UPDATE  0x400
#define CA_RI_REFERS  0x800
#define IS_CA_MULTVAR(flags) (((flags) & (CA_MULTIATTR|CA_INDIRECT))==(CA_MULTIATTR|CA_INDIRECT))

#define REC_NONEX     1
#define REC_EXISTS    2
#define REC_FICTIVE   3
#define REC_RELEASED  4   // temp during loading

typedef struct DllPrezen ltable
/* cached access structure (for view and for odbc cursor) */
{ cdp_t     cdp;
  ULONG     m_cRef;
  struct t_connection * conn; /* non NULL for ODBC, NULL for WB */
  tcursnum  cursnum;
  HSTMT     hStmt;            /* for ODBC only */
  uns32     supercurs;        // upper cursnum (WB) or hStmt (ODBC)
  BOOL      close_cursor;     // closing cursor requred when deleting (WB only)
  tptr      select_st;        /* for ODBC an cursor reset only */
 // cache:
  trecnum   cache_top;        /* number of the record in cache(top), NORECNUM iff none */
  unsigned  cache_reccnt;
  LONG      rec_cache_size;    /* actual cache buffer size, >= rec_cache_real */
  unsigned  rec_cache_real;    /* real record size */
  HGLOBAL   cache_hnd;         /* cache buffer handle */
  tptr      cache_ptr;     /* cache pointer */
  tptr      odbc_buf;      /* aux ODBC buffer for 1 record, used by BindColumn */
  trecnum   edt_rec;   /* number of record being edited, NORECNUM iff synchronized */
  unsigned  rr_datasize;
  unsigned  attrcnt;
  atr_info * attrs;
  bool      recprivs;
  unsigned  rec_priv_off;  // offset of record privils in the cache, valid iff recprivs
  unsigned  rec_priv_size;
 /* curspor position, not used by views: */

 // remap:
  RPART *   remap_a;       /* remap bitmap or NULL iff no remap */
#if WBVERS<900
  HGLOBAL   remap_handle;  /* handle of remap_a or NULL iff no remap */
#endif  
  uns32     remap_size;    /* size of remap_a in bytes */
  trecnum   rm_ext_recnum; /* number of ext. records covered in remap_a */
  trecnum   rm_int_recnum; /* number of remapped internal records */
  bool      rm_completed;  /* wbtrue iff remap_a contains all records */
  trecnum   rm_curr_ext;   /* current record: external number */
  trecnum   rm_curr_int;   /* current record: internal number */
  trecnum   fi_recnum;     /* replaces "recnum" */
 // write record ex:
  t_column_val_descr * colvaldescr; 

  ltable(cdp_t cdpIn, struct t_connection * connIn);
  ~ltable(void);

  ULONG AddRef(void) { return ++m_cRef; }
  ULONG Release(void);
  void  load(void * inst, unsigned offset, unsigned count, trecnum new_top);
  void  loadstep(void * inst, unsigned offset, unsigned count, trecnum new_top);
  void  free(unsigned offset, unsigned count);
  BOOL  write(tcursnum cursnum, trecnum * erec, tptr cache_rec, BOOL global_save);
  BOOL  privils_ok(int attr, tptr buf, BOOL write_priv);
#if WBVERS<900
  BOOL  describe(tcursnum cursnum);
#else
  char * fulltablename;
  BOOL  describe(const d_table * td, bool is_odbc);
#endif

#ifdef CLIENT_ODBC_9
  HSTMT hInsertStmt;
  const d_table * co_owned_td;
#endif
} ltable;


typedef struct  /* registered odbc table or cursor */
{ tcursnum  odbc_num;
  BOOL      is_table;
  BOOL      mapped;
  union
  { tptr          source;     /* select statement for cursors (!is_table) */
    t_odbc_link * odbc_link;  /* iff is_table */
  }         s;
  struct t_connection * conn;
  ltable *  ltab;      /* used for accessing data */
} odbc_tabcurs;

SPECIF_DYNAR(odbc_tc_dynar,odbc_tabcurs,1);

#define CONN_FL_TEMPORARY               1
#define CONN_FL_NAMED_CONSTRS           2
#define CONN_FL_NON_NULL                4
#define CONN_FL_DISABLE_COLUMN_PRIVILS  8

typedef struct  // ODBC connection information stored in a connection object
{ char dsname[SQL_MAX_DSN_LENGTH+1];
  char conn_string[1024];
} t_conndef;

//#endif
struct t_folder_header
{ uns32 version;
};
struct t_folder_item
{ tobjname name;
  tcateg   categ;
  uns32    modif_timestamp;
};

typedef struct
{ tobjname name;
  tobjnum objnum;
  uns16   flags;
  t_folder_index folder;
  uns32          modif_timestamp;
} t_comp;

SPECIF_DYNAR(comp_dynar, t_comp, 6);

struct t_relation : public t_comp
{ uns8     refint, index1, index2, nic;
  tobjname object_folder; // duplicates info in "folder" but is necessary to be consistent (?)
  tobjname tab1name;
  tobjname tab2name;
  tobjnum  tab1num, tab2num;
  tattrib  atr1[MAX_REL_ATTRS];
  tattrib  atr2[MAX_REL_ATTRS];
};

SPECIF_DYNAR(relation_dynar, t_relation, 3);

typedef t_comp t_folder;

SPECIF_DYNAR(folder_dynar, t_folder, 3);

struct t_odbc
{ 
#ifdef CLIENT_ODBC
  HENV hEnv;
  t_connection * conns;
#endif
  BOOL mapping_on;  // permanently FALSE when !CLIENT_ODBC
 // object caches:
  comp_dynar      apl_tables;
  comp_dynar      apl_views;
  comp_dynar      apl_menus;
  comp_dynar      apl_queries;
  comp_dynar      apl_programs;
  comp_dynar      apl_pictures;
  comp_dynar      apl_drawings;
  comp_dynar      apl_replrels;
  comp_dynar      apl_wwws;
  relation_dynar  apl_relations;
  comp_dynar      apl_procs;
  comp_dynar      apl_triggers;
  comp_dynar      apl_seqs;
  folder_dynar    apl_folders;
  comp_dynar      apl_domains;
};

///////////////////// embedded vars, prepared stmts ///////////////////////

SPECIF_DYNAR(t_clivar_dynar, t_clivar, 2);

struct t_prep_stmt
{ BOOL is_prepared;       // FALSE in the free cells in the array of prepared statements
  t_clivar_dynar clivars; // containts hostvars if created locally by parsing the statement
  int hostvar_count;
  t_clivar * hostvars;    // not owning ptr, points into clivars or to hostvars passed to cd_SQL_host_prepare
};

SPECIF_DYNAR(t_prep_dynar, t_prep_stmt, 1);

/****************************** cdp ******************************************/
struct IStorage;
class cAddress;
class wxCSConv;

typedef short (WINAPI *LPPROGRESSCALLBACK)(void *cdp, trecnum num0, trecnum num1);
//typedef void (t_translation_callback)(wchar_t * message);

enum t_connection_types { CONN_TP_602, CONN_TP_ODBC };

struct xcd_t
{ t_connection_types connection_type;
  char conn_server_name[MAX_OBJECT_NAME_LEN+1]; // stored by interf_init, used by cd_unlink_kernel
  t_thread thread_id;                           // ID of the owning thread or 0 if shared
 // last error information:
  char    * generic_error_message;
  wchar_t * generic_error_messageW;

#if WBVERS>=900  
  char locid_server_name[MAX_OBJECT_NAME_LEN+1]; // unique local readable identification of the server, is the local registration name or "IP database_name" for unregistered servers
#endif
  xcd_t * next_xcd;
  cdp_t get_cdp(void)
    { return connection_type==CONN_TP_602 ? (cdp_t)this : NULL; }
  inline t_connection * get_odbcconn(void)
    { return connection_type==CONN_TP_ODBC ? (t_connection*)this : NULL; }
  inline xcd_t(t_connection_types connection_typeIn) : connection_type(connection_typeIn) 
    { next_xcd = NULL;  generic_error_message = NULL;  generic_error_messageW = NULL; }
  ~xcd_t(void)
  { if (generic_error_message)  corefree(generic_error_message);
    if (generic_error_messageW) corefree(generic_error_messageW);
  }
};

extern xcd_t * connected_xcds;
void add_to_list(xcd_t * xcdp);
void remove_from_list(xcd_t * xcdp);

struct t_connection
#if WBVERS<900
{ 
  t_connection * next_conn;
  BOOL  selected;
  tobjnum objnum;  // WB object number, GUI uses it
#else
                    : public xcd_t
{
  const char * uid;
  char  dbms_name[30];
  char  dbms_ver[30];
  char  database_name[256];  // may be long, may contain pathname
  uns32 special_flags;    // 1: cursors are editable even when SQLGetStmtOption(hStmt, SQL_ATTR_CONCURRENCY) returns SQL_CONCUR_READ_ONLY
#endif
  char  dsn[SQL_MAX_DSN_LENGTH+1];
  tptr  conn_string;
  HDBC  hDbc;
  HSTMT hStmt;
  uns32 owner_usage, qualifier_usage;
  uns16 qualifier_loc;
  char  qualifier_separator[5];  // the prefix separator: it is the '.' when using owner as prefix or the ODBC qualifier separator when using qualifier as prefix
  char  identifier_quote_char;
  BOOL  SetPos_support;   /* UPDATE, ADD and DELETE supported in SetPos */
  BOOL  can_lock;
  BOOL  absolute_fetch;
  BOOL  can_transact;
  uns32 scroll_options, scroll_concur;
  uns32 flags;
  uns16 sql_conform;
  xtype_info * ti;
  odbc_tc_dynar ltabs;
#if WBVERS<900
  inline t_connection(void)
    { conn_string=NULL;  ti=NULL;  selected=FALSE;  objnum=(tobjnum)-1; }
  inline ~t_connection(void)
    { corefree(conn_string);  corefree(ti); }
#else
  inline t_connection(void) : xcd_t(CONN_TP_ODBC)
    { conn_string=NULL;  uid=NULL;  ti=NULL;  special_flags=0; }
  inline ~t_connection(void)
    { corefree(conn_string);  corefree(uid);  corefree(ti); }
#endif
  int sql_file_usage;
};

typedef t_connection * t_pconnection;

typedef void CALLBACK t_client_error_callback2(cdp_t cdp, const char * text);
typedef void CALLBACK t_client_error_callbackW(cdp_t cdp, const wchar_t * text);

struct cd_t : public xcd_t
{ byte   in_use;  /* is PT_EMPTY, PT_DIRECT, PT_REMOTE, PT_SLAVE, PT_SERVER */
  uns8   server_index; // user in dual-access functions, may be removed in 32-bits (if 1 client cannot link to 2 servers simult.)
  wbbool mask_token;  // true if write allowed without the token
  wbbool non_mdi_client;
 // description of the connection between client and server:
#ifndef PEEKING_DURING_WAIT
#ifndef SINGLE_THREAD_CLIENT
  LocEvent hSlaveSem;
#if WBVERS<900
  trecnum server_progress_num1, server_progress_num2;   
#endif
#endif
#endif
	DWORD		 LinkIdNumber;  /* used only by the direct client on Break */
 // selected application:
  tobjname    sel_appl_name;  // NULLSTRING on no application selected
  WBUUID      sel_appl_uuid;
  WBUUID      front_end_uuid; // same as sel_appl_uuid if not redirected
  WBUUID      back_end_uuid;  // same as sel_appl_uuid if not redirected
  tobjnum     admin_role;     // NOOBJECT iff role does not exist
  tobjnum     author_role;
  tobjnum     senior_role;
  tobjnum     junior_role;
  wbbool      locked_appl;    // enciphered and editing disabled
  tcateg      appl_starter_categ;
  tobjname    appl_starter;  

  uns8        applnum;  /* number identifying the connection (in d_cached rights) */
  run_state   RV;
  MsgInfo     msg;          // incomming message (server answer) - released after the write_ans_proc is called
  t_thread    threadID;     // thread or task ID
  cAddress *  pRemAddr;     // connection address, assigned in interf_init_ex, deleted in Unlink
  BYTE *      pImpExpBuff;  // not used!
  WORD        wImpExpDataSize;  // not used!
  uns32       auxinfo; // client linking result, data import flag
  void *      auxptr;  // container
  view_dyn *  all_views;    // views list on client, dest server on slave
  tobjname    errmsg;

 // client only variables:
  tobjnum     applobj;  /* application object number (on the client side) */
  void *      OleMsgFilter;
  t_odbc      odbc;
  long        server_version;
  int         server_version3;
  inline int  protocol_version(void) const
    { return server_version<MAKELONG(5, 9) ? 0 : 1; }
  inline bool improved_multipart_comm(void) const
    { return server_version>MAKELONG(0, 11) || server_version==MAKELONG(0, 11) && server_version3>=2; }
  inline unsigned sizeof_tattrib(void) const
    { return this && protocol_version() ? sizeof(uns16) : sizeof(uns8); }
  inline unsigned sizeof_tname(void) const
    { return this && protocol_version() ? 31+1 : 18+1; }
 // client comm. vars:
  void init_client_server_comm(void);
  friend tptr get_space_op(cd_t * cdp, unsigned size, uns8 op);
 private: 
  t_message_fragment req_array[MAX_PACKAGED_REQS];  // client requests (may be in parts), valid is <0..req_ind)
  unsigned req_ind;  // number of valid items in the [req_array]
  char     default_rq_frame[DEFAULT_RQ_FRAME_SIZE+1];  // buffer for a short request
  bool     default_frame_busy;  // true iff [default_rq_frame] has already been used in [req_array]
  bool     has_persistent_part;
 public:
  void add_request_fragment(char * data, uns32 length, bool dealloc);
  void set_persistent_part(void)
    { has_persistent_part=true; }
  bool get_persistent_part(void) const
    { return has_persistent_part; }
  inline unsigned get_request_fragments_count(void) const { return req_ind; }
  inline t_message_fragment * get_request_fragments(void) { return req_array; }
  void free_request(void);  // clears the request in [req_array], [req_ind]

  void      * ans_array[MAX_PACKAGED_REQS]; // pointers for the results
#if WBVERS<950
  uns16       ans_type [MAX_PACKAGED_REQS];
#else
  uns32       ans_type [MAX_PACKAGED_REQS];  // cd_Read_record needs long answers
#endif
  wbbool      in_package, in_concurrent;
  unsigned    ans_ind;
  wbbool   last_error_is_from_client;
  bool        object_cache_disabled;
  t_commenc * commenc;
 // direct client synchonization handles
//  request_str * request_map;
//  answer_cb * answer_map;
#ifdef WINS
  t_dircomm dcm;
#endif  
 // saved private key:
#if WBVERS<=810
  t_priv_key_file * priv_key_file;
#endif
  t_prep_dynar prep_stmts;
  uns32    login_key;
  wbbool global_debug_mode;
 // multilingual support:
  ltable * lang_cache;
  trecnum cache_recs;
  int selected_lang;
#ifdef WINS
  IStorage * IStorageCache;
#endif
  WBUUID   server_uuid;
  uns32    obtained_login_key;  // login key obtained by this client and registered
  tobjnum  logged_as_user;
  void *   MailCtx;
#ifdef WINS
  UINT_PTR timer;
  LPPROGRESSCALLBACK lpProgressCallBack;
#endif
  int sys_charset;
  wxCSConv * sys_conv;
  int notification_type;
  void * notification_parameter;
  t_client_error_callback2 * client_error_callback2;
  t_client_error_callbackW * client_error_callbackW;

  inline cd_t(void) : xcd_t(CONN_TP_602) { }
};

class ProtectedByCriticalSection
// Critical section entering/leaving implemented as a class construction/destruction
{ CRITICAL_SECTION * const cs; // the critical section entered by constructing the class
 public:
  inline ProtectedByCriticalSection(CRITICAL_SECTION * csIn) : cs(csIn)
    { EnterCriticalSection(cs); }
  inline ~ProtectedByCriticalSection(void)
    { LeaveCriticalSection(cs); }
};


#endif  /* ! __CDP_H__ */

