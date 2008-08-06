// sdefs.h - Main declarations for the server
#include "wbdefs.h"

#if WBVERS<=1000
#ifdef WINS
#define ANY_SERVER_GUI
#endif
#endif

#undef  IS_STRING
#undef  ATT_CSSTRING
#undef  ATT_CSISTRING
#define IS_STRING(tp) ((tp) == ATT_STRING)

#define FLUSH_OPTIZATION        

struct t_query_expr;
struct elem_descr;
class  t_trigger;

int simple_type_size(int type, t_specif specif);
const void * null_value(int tp);

/*****************************************************************************/
struct dd_index;
bool get_database_file_password(char * password);

#if defined(ANY_SERVER_GUI) || !defined(WINS)
void WriteSlaveThreadCounter(void);
#else
inline void WriteSlaveThreadCounter(void) { }
#endif
///////////////////////////// from cint.h in 5.1 ///////////////////////////
#define SEARCH_CASE_SENSITIVE   1
#define SEARCH_WHOLE_WORDS      2
#define SEARCH_BACKWARDS        4
#define SEARCH_WIDE_CHAR        8
#define SEARCH_IN_BLOCK         0x100
#define SEARCH_FROM_CURSOR      0x200
#define SEARCH_AUTOREPEAT       0x400
#define SEARCH_DOREPLACE        0x4000
#define SEARCH_NOCONT           0x8000
CFNC BOOL WINAPI Search_in_block(const char * block, uns32 blocksize,
                         const char * next, const char * patt, uns16 pattsize,
                         uns16 flags, uns32 * respos);

CFNC int   WINAPI interf_init     (cdp_t cdp);
CFNC void  WINAPI interf_close    (void);
CFNC void  WINAPI cd_interf_close   (cdp_t cdp);
CFNC sig32  WINAPI WinBase602_version(void);
CFNC sig32  WINAPI cd_WinBase602_version(cdp_t cdp);
CFNC void WINAPI make_apl_query(char * query, const char * tabname, const uns8 * uuid);

//////////////////////////// wbcdp ////////////////////////////////////////////
typedef uns16      t_client_number; 
#define NOAPPLNUM (t_client_number)-1

#include "swbcdp.h"

void fatal_error(cdp_t cdp, const char * txt);
/************************** configuration constants *************************/
#define HEADER_RESERVED  1 /* number of blocks reserved for 1-block header */
#define REMAPBLOCKSNUM   2 /* number of blocks containing remap information */
#define TRANS_JOURNAL_BYTES  (16*1024)  /* size of transaction journal */

/* implementation defined constants */
#define NOTBNUM   (ttablenum)0x7fffffff  /* must be different from all tables nums, incl. temps */
#define FLEXIBLE_BLOCKSIZE
#ifdef FLEXIBLE_BLOCKSIZE
extern unsigned BLOCKSIZE;
#else
#define BLOCKSIZE 4096 /*header.blocksize */  /* size of the physical block */
#endif
#define NUM_OF_PIECE_SIZES  5  /* BLOCKSIZE/2, /4, /8, /16, /32; BLOCKSIZE not incl. */
typedef uns32 piecemap;
#define k32       header.koef_k32      /* piece size unit */

extern HINSTANCE hKernelLibInst;

#define MAX_MAX_TASKS (1000+3)
extern cd_t * cds[MAX_MAX_TASKS];   /* must not be based here */

/////////////////////////////// database file structures: packed ///////////////////////////////////
#pragma pack(1)
struct tpiece
{ tblocknum dadr;
  uns8 offs32;
  uns8 len32;
};

struct heapacc
{ tpiece     pc;
  theapsize  len;
};

/*************************** users management *******************************/
struct header_structure
{ char      signature[26];// string "WinBase602 Database File\x1a"
  byte      fil_version;  // value CURRENT_FIL_VERSION_NUM (or 8+1 for extended databases)
  wbbool    sezam_open;   // indicates that Sezam is in open state
  uns32     unused0;      // kernel parameters
  unsigned  blocksize;
  tblocknum jourfirst;    /* address of the first transaction journal block */
  tblocknum joursize;     /* number of blocks in transaction journal */
  tblocknum bpool_first;  /* address of the first bpool block (dadr base) */
  unsigned  bpool_count;  /* number of block pool blocks */
  unsigned  remapnum;     // number of remaped blocks
  tblocknum remapblocks[REMAPBLOCKSNUM];  /* physical block numbers ! */
  tblocknum tabtabbasbl;  /* base block of the tables table */
  tblocknum max_blocknum; // the last block contained in the FIL
  tpiece    pcpcs[NUM_OF_PIECE_SIZES];   /* free pieces by sizes */
  unsigned  pccnt[NUM_OF_PIECE_SIZES];   /* numbers of free pieces */
  timestamp closetime;
  unsigned  koef_k32;
  t_zcr     gcr;           // global counter of replicable updates
  uns32     prealloc_gcr;  
  WBUUID    server_uuid;
 // security info:
  uns8      unused1;
  wbbool    unused2;
  uns32     unused3;
  char      unused4[MAX_LOGIN_SERVER+1];
  WBUUID    top_CA;
  uns8      fil_crypt;
  uns8      unused5[MD5_SIZE];
  uns8      unused6[MD5_SIZE];
  unsigned  cd_rom_offset;  // number of locked bpools
  BOOL      berle_used;
  unsigned  creation_date;  // FIL creation date, used by TRIAL licences on non-windows platforms
  tpiece    srvkey;         // secret key of the SQL server
  unsigned  srvkeylen;      // length of the secret key
  t_specif  sys_spec;       // specifies charset and collation for system names (not specified when 0)
  uns64     lob_id_gen;     // the last used lob_id
  unsigned  reserve[10];    // not used but set to 0 
};
extern header_structure header;
extern t_specif sys_spec;  // copied from the header and converted 0->1, 255->0

struct ttrec   // description of a TABTAB (and OBJTAB) record 
{ uns8   del_mark;
  t_zcr  _zcr;
  tpiece rigpiece;
  uns32  rigsize;
  char   name[OBJ_NAME_LEN];
  uns8   categ;
  uns8   apluuid[UUID_SIZE];
  tpiece defpiece;
  uns32  defsize;
  uns16  flags;
  char   folder_name[OBJ_NAME_LEN];
  uns32  last_modif;
 // OBJTAB record ends here, the following members are contained in the TABTAB record only
  tpiece drigpiece;
  uns32  drigsize;
  tblocknum basblockn;
};
#pragma pack()

SPECIF_DYNAR(piece_dynar, tpiece, 50);

// index key of system tables: aligned structures
struct objtab_key
{ char objname[OBJ_NAME_LEN];
  char categ;
  uns8 appl_id[UUID_SIZE];
  trecnum recnum;
};

struct usertab_key
{ char objname[OBJ_NAME_LEN];
  char categ;
  trecnum recnum;
};

struct uuid_key
{ uns8 uuid[UUID_SIZE];
  trecnum recnum;
};


/*************** Atributy systemove tabulky TABTAB ***************/
#define TAB_DRIGHT_ATR     10   /* data rights */
#define TAB_BASBL_ATR      11   /* basblock number */
//////////////////////////////////////////////////////////////////////////////////////////////////////////
extern tpiece NULLPIECE;

typedef struct
{ uns32 * lost_blocks;
  uns32 * lost_dheap;
  uns32 * nonex_blocks;
  uns32 * cross_link;
  uns32 * damaged_tabdef;
} di_params;
BOOL db_integrity(cdp_t cdp, BOOL repair, di_params * di);

void new_request(cdp_t cdp, struct request_frame * req_data);
BOOL is_null(const char * val, int type, t_specif specif);
BOOL name_find2_obj(cdp_t cdp, const char * name, const char * schemaname, tcateg categ, tobjnum * pobjnum);
BOOL find_obj      (cdp_t cdp, const char * name,                          tcateg categ, tobjnum * pobjnum);

/* kernel flags */
#define KFL_NO_JOUR            1   /* do not write to the journal */
#define KFL_ALLOW_TRACK_WRITE  2   /* allows explicit write to tracking attrs, disables translating authors to strings */
#define KFL_NO_TRACE           4   /* do not write to the tracing attributes */
#define KFL_STOP_INDEX_UPDATE  8   // disables inditem_remove & ..._add
#define KFL_DISABLE_REFINT     0x10 // disables referential integrity (for replicating threads)
#define KFL_NO_NEXT_SEQ_VAL    0x20 // disables getting next value from a sequence, used in move_data only
#define KFL_HIDE_ERROR         0x40 // disables logging errors (used when evaluating expressions from debugger)
#define KFL_DISABLE_REFCHECKS  0x80 // disables registering changes for checking referential constrains
#define KFL_DISABLE_INTEGRITY  0x100 // disables integrity checking and NOT NULL checking
#define KFL_DISABLE_DEFAULT_VALUES 0x200 // replaces the default value by NULL in tb_write_vector, used to stop sequence values when loading or altering a table
#define KFL_BI_STOP            0x400 // disables updating bi-pointers in the other direction
#define KSE_FORCE_DEFERRED_CHECK 0x800  // makes all constrains deferred
#define KFL_DISABLE_ROLLBACK   0x1000  // used during the creation of a cursor, replaces explicit rollback by an error
#define KFL_NO_OPTIM           0x2000  // used when creating the cursor descriptor
#define KFL_PASSED_FLAG        0x4000  // used temporarily in the deadlock detection
#define KFL_MASK_DUPLICITY_ERR 0x8000  // index key duplicity errors are reported in the log, but no error occurs (index will not be OK)
#define KFL_EXPORT_LOBREFS     0x10000 // in the TDT export use LOB IDs instead of LOB contents

/* various: */
d_table * kernel_get_descr(cdp_t cdp, tcateg cat, tobjnum tbnum, unsigned *psize);
int d_curs_size(d_table * tdd);
tptr maxval(uns8 tp);
void add_val(tptr sum, const char * val, uns8 type);
void WINAPI dbg_err(const char * text);
void WINAPI cd_dbg_err(cdp_t cdp, const char * text);
void WINAPI trace_info(const char * text);
void WINAPI display_server_info(const char * text);
void client_start_error(cdp_t cdp, const char * message_start, int error_code);
int compare_values(int valtype, t_specif specif, const char * key1, const char * key2);

tobjnum find_object_by_id(cdp_t cdp, tcateg categ, const WBUUID uuid);
trecnum find_record_by_primary_key(cdp_t cdp, table_descr * tbdf, char * strval);

inline BOOL IS_NULL(const char * val, int type, t_specif specif)
{ switch (type)
  { case ATT_BOOLEAN:  case ATT_INT8:
                      return *val==(char)NONEBOOLEAN;
    case ATT_INT16:   return *(sig16*)val==NONESHORT;
    case ATT_MONEY:   return !*(uns16*)val && (*(sig32*)(val+2)==NONEINTEGER);
    case ATT_INT32:    case ATT_DATE: case ATT_TIME:  case ATT_TIMESTAMP:
                      return *(sig32*)val==NONEINTEGER;
    case ATT_INT64:   return *(sig64*)val==NONEBIGINT;
    case ATT_PTR:      case ATT_BIPTR:
                      return *(trecnum*)val==NORECNUM;
    case ATT_FLOAT:   return *(double*)val==NULLREAL;
    case ATT_STRING:   case ATT_CHAR:
      if (specif.wide_char) return !*(const wuchar*)val;
      else                  return !*val;
    case ATT_BINARY:  
      while (specif.length--) if (*(val++)) return FALSE;
      return TRUE;
    case ATT_EXTCLOB:  case ATT_EXTBLOB:
                      return *(uns64*)val==0;
    default:          return !memcmp(val, &NULLPIECE, sizeof(tpiece));
  }
}

char * append(char * dest, const char * src);  // appends with single '\'

void get_repl_state(int * ip_transport, int * repl_in, int * repl_out);
void register_tables_in_ftx(cdp_t cdp, tobjnum objnum);

// common conversion functions:
extern "C" {
uns32 WINAPI Make_date  (int day, int month, int year);
int   WINAPI Day        (uns32 dt);
int   WINAPI Month      (uns32 dt);
int   WINAPI Year       (uns32 dt);
int   WINAPI Quarter    (uns32 dt);
uns32 WINAPI Today      (void);
uns32 WINAPI Make_time  (int hour, int minute, int second, int sec1000);
int   WINAPI Day_of_week(uns32 dt);
int   WINAPI Hours      (uns32 tm);
int   WINAPI Minutes    (uns32 tm);
int   WINAPI Seconds    (uns32 tm);
int   WINAPI Sec1000    (uns32 tm);
uns32 WINAPI Now        (void);
//BOOL  WINAPI Like       (const char * s1, const char * s2); -- made local in sql2.cpp
BOOL  WINAPI Pref       (const char * s1, const char * s2);
BOOL  WINAPI Substr     (const char * s1, const char * s2);
void  WINAPI Upcase     (char * str);
double WINAPI money2real(monstr * m);
BOOL   WINAPI real2money(double d, monstr * m);
uns32 WINAPI timestamp2date    (uns32 dtm);
uns32 WINAPI timestamp2time    (uns32 dtm);
uns32 WINAPI datetime2timestamp(uns32 dt, uns32 tm);
void  WINAPI time2str     (uns32 tm,  char * txt, int param);
void  WINAPI date2str     (uns32 dt,  char * txt, int param);
void  WINAPI timestamp2str(uns32 dtm, char * txt, int param);
BOOL  WINAPI str2time     (const char * txt, uns32 * tm);
BOOL  WINAPI str2date     (const char * txt, uns32 * dt);
BOOL  WINAPI str2timestamp(const char * txt, uns32 * dtm);
}
bool read_displ(const char ** pstr, int * displ);
bool whitespace_string(const char * p);
bool sql92_str2date(const char * str, uns32 * dt);
BOOL sql92_str2time(const char * str, uns32 * tm, bool UTC, int default_tzd);
BOOL sql92_str2timestamp(const char * str, uns32 * ts, bool UTC, int default_tzd);
bool str2numeric(tptr ptr, sig64 * i64val, int scale);
void sys_Upcase(char * str);
int  sys_stricmp(const char * str1, const char * str2);
bool sys_Alpha(char ch);
bool sys_Alphanum(char ch);

// server-specific memory management functions:
unsigned WINAPI leak_test(void);
void extlibs_mark_core(void);  // WINS only
void prof_mark_core(void);

// SQL functions:
uns32 WINAPI Current_date(cdp_t cdp);   
uns32 WINAPI Current_time(cdp_t cdp);
uns32 WINAPI Current_timestamp(cdp_t cdp);
typedef struct
{ WBUUID id_user;
  WBUUID id_home;
} val2uuid;
CFNC BOOL WINAPI Waits_for_me(cdp_t cdp, val2uuid uuids);

BOOL null_uuid(const WBUUID uuid);
BOOL get_info_value(const char * info, const char * section, const char * keyname, int type, void * value);
void WINAPI safe_create_uuid(WBUUID appl_id);

#define PROP_NAME_ATR 1
#define PROP_OWNER_ATR 2
#define PROP_NUM_ATR 3
#define PROP_VAL_ATR 4
//////////////////////////////////// __RELTAB ///////////////////////////////
#define REL_PAR_NAME_COL  1
#define REL_PAR_UUID_COL  2
#define REL_CLD_NAME_COL  3
#define REL_CLD_UUID_COL  4
#define REL_CLASSNUM_COL  5
#define REL_INFO_COL      6

#define REL_TAB_PRIM_KEY  0
#define REL_TAB_INDEX2    1

struct t_reltab_primary_key
{ sig16 classnum;
  char name1[OBJ_NAME_LEN];  // without the terminating space
  WBUUID uuid1;
  char name2[OBJ_NAME_LEN];  // without the terminating space
  WBUUID uuid2;
  trecnum rec;
  t_reltab_primary_key(sig16 classnumIn, const tobjname name1In, const WBUUID uuid1In, const tobjname name2In, const WBUUID uuid2In)
  { classnum=classnumIn;
    strcpy(name1, name1In);  // overwrites the following member!!
    memcpy(uuid1, uuid1In, UUID_SIZE);
    strcpy(name2, name2In);  // overwrites the following member!!
    memcpy(uuid2, uuid2In, UUID_SIZE);
  }
};

struct t_reltab_index2
{ sig16 classnum;
  char name[OBJ_NAME_LEN];  // without the terminating space
  WBUUID uuid;
  trecnum rec;
  t_reltab_index2(sig16 classnumIn, const tobjname nameIn, const WBUUID uuidIn)
  { classnum=classnumIn;
    strcpy(name, nameIn);  // overwrites the following member!!
    memcpy(uuid, uuidIn, UUID_SIZE);
  }
};

struct t_reltab_vector0
{ tobjname name1;
  WBUUID uuid1;
  tobjname name2;
  WBUUID uuid2;
  sig16 classnum;
  sig32 info;
};
  
struct t_reltab_vector : public t_reltab_vector0
{ t_reltab_vector(sig16 classnumIn, const tobjname name1In, const WBUUID uuid1In, const tobjname name2In, const WBUUID uuid2In, sig32 infoIn)
  { classnum=classnumIn;
    strcpy(name1, name1In);
    memcpy(uuid1, uuid1In, UUID_SIZE);
    strcpy(name2, name2In);  // overwrites the following member!!
    memcpy(uuid2, uuid2In, UUID_SIZE);
    info=infoIn;
  }
};

struct t_express;
#define DATAPTR_DEFAULT ((void*)-1)  // Denotes the default value for the column. Used in the dataptr member of t_colval
#define DATAPTR_NULL    ((void*)-2)  // Denotes the NULL    value for the column. Used in the dataptr member of t_colval
struct t_colval
{ tattrib      attr;    // table column number, NOATTRIB is the delimiter
  t_express  * index;   // index of the column value, NULL if column is not a multiattribute
  const void * dataptr; // pointer to the data, DATAPTR_NULL for the NULL value, DATAPTR_DEFAULT for the default value
                        // offset to vector->vals iff vector->vals!=NULL
  uns32      * lengthptr; // pointer to the length of variable-size data (in bytes)
  t_express  * expr;    // expression for evaluation, when not NULL, offset not used 
  int table_number;     // table number the column belongs to, 0 if there is only one table, compared to t_vector.selected_table_number
  uns32      * offsetptr; // pointer to the offset of variable-size data (NULL for offset 0)  (in bytes)
};

extern const t_colval reltab_coldescr[];

#define IP_length 6  // IP address length in the _IV_
extern int system_initialized;  // static class constructors competed

#ifdef LINUX
extern int alternate_sip;
void no_diacrit(char * str, size_t bufsize);
extern unsigned char mac_address[6];    // defined on server start
#endif

/////////////////////////////////////////// licences /////////////////////////////////////////////
CFNC DllSpecial BOOL WINAPI get_any_valid_ik(char * ik, const char * env_info);
bool check_lcs(const char * RegistrationKey, const char * InstallationKey);
CFNC DllSpecial bool WINAPI insert_server_licence(const char * number);
CFNC DllSpecial bool WINAPI drop_server_licence(const char * number);
bool store_licence_number(const char * section, const char * number);
bool find_licence_number(const char * section, const char * number);
bool get_server_registration(const char * ik, char * srvreg, int * year, int * month, int * day);
extern const char section_ik           [],
                  section_server_reg   [],
                  section_client_access[];
CFNC DllSpecial bool WINAPI verify_ik(const char * InstallationKey, const char * env_info, int * year=NULL, int * month=NULL, int * day=NULL);

BOOL WINAPI Get_server_error_text(cdp_t cdp, int err, char * buf, unsigned buflen);
#if WBVERS>=900
bool store_server_licence(const char * number);
int get_licence_information(const char * path, char * installation_key, char * ServerLicenceNumber, 
                            int * year, int * month, int * day);
#endif
////////////////////////////////////////// IP address validation ///////////////////////////////
#define IP_ADDR_LEN 4

struct t_ip_masked
{private: 
  unsigned char addr[IP_ADDR_LEN];  // network byte order!
  unsigned char mask[IP_ADDR_LEN];  // network byte order!
 public:
  bool is_masked_addr(const unsigned char * ip) const
  { for (unsigned i = 0;  i<IP_ADDR_LEN;  i++)
      if ((ip[i] & mask[i]) != (addr[i] & mask[i])) return false;
    return true;
  }
  void init_mask(void)
  { for (unsigned i = 0;  i<IP_ADDR_LEN;  i++)
      mask[i] = 0xff;
  }
  void store_addr(const unsigned char * addrIn)
  { for (unsigned i = 0;  i<IP_ADDR_LEN;  i++)
      addr[i] = addrIn[i];
  }
  void store_mask(const unsigned char * maskIn)
  { for (unsigned i = 0;  i<IP_ADDR_LEN;  i++)
      mask[i] = maskIn[i];
  }
  bool is_empty_addr(void) const
  { for (unsigned i = 0;  i<IP_ADDR_LEN;  i++)
      if (addr[i]) return false;
    return true;
  }
};

int ValidatePeerAddress(cdp_t cdp, const unsigned char * addr, int connection_type); // parameter in the network order
void communic_replay(void);

#ifdef WINS
void client_terminating(void);  // support for dependent servers
#endif
