#ifndef CDP_INCLUDED
#define CDP_INCLUDED

typedef uns32 tblocknum;          /* type of physical block number */
typedef int   tfcbnum;            // type of frame control block number
#define NOFCBNUM (tfcbnum)(-1)    // value different from all valid fcbnum values

/******************************** ODBC ***************************************/
#define DWORD sql_dword
#define LPDWORD sql_lpdword
#include <sqltypes.h>
#undef DWORD
#undef LPDWORD
#include <sql.h>      // in WIN32 must not be taken from ., NLM needs .
#include <sqlext.h>

#ifndef SQL_TYPE_DATE   // NLM with old header files
#define SQL_TYPE_DATE      91
#define SQL_TYPE_TIME      92
#define SQL_TYPE_TIMESTAMP 93

#define SQL_DATETIME        9
#define SQL_CODE_DATE       1
#define SQL_CODE_TIME       2
#define SQL_CODE_TIMESTAMP  3

#define SQL_PRED_NONE     0
#define SQL_PRED_BASIC    2
#endif

/***************************** interp vars **********************************/

struct t_scope;
struct t_shared_scope;
struct t_rscope;
struct t_global_scope;
struct db_kontext;
struct table_descr;

enum t_linechngstop { LCHNGSTOP_NEVER=0, LCHNGSTOP_ALWAYS=1, LCHNGSTOP_SAME_LEVEL=2 };

struct t_rscope
// run-time frame for local variables in SQL compound statement
{ t_rscope * next_rscope;
  int  level;              // static nesting level, -1 for parameters outside the routine
  const t_scope * const sscope;    // static scope ptr, not owning
  t_shared_scope * sh_scope;  // just for calling Release when the scope is shared (sscope==sh_scope), NULL otherwise
  //tobjnum upper_objnum;    // for debugging
  //uns32   upper_line;      // for debugging
  //t_linechngstop upper_linechngstop; // for debugging
  char data[1];
 // when new and delete are located here they are accessible to the XML extension 
  void *operator new(size_t size, unsigned data_size)
    { return corealloc(size+data_size-1, 97); }
#if defined(WINS) 
  void  operator delete(void * ptr, unsigned data_size)
    { corefree(ptr); }
#endif
#if !defined(WINS) || defined(X64ARCH)
  void  operator delete(void * ptr)
    { corefree(ptr); }
#endif
  void mark_core(void);
  t_rscope(const t_scope  * sscopeIn) : sscope(sscopeIn)
    { sh_scope=NULL;  next_rscope=NULL; }  
  //t_rscope(const t_scope  * sscopeIn);
  t_rscope(t_shared_scope  * sh_scopeIn);
  ~t_rscope(void);
};

#define MAX_CURS_TABLES   32

/////////////////////////////////////// cache for privils inside [privils] ///////////////////////////////
#define PRIVIL_CACHE_SIZE  800

typedef uns16 t_size_info;  // for tables with many columns

#pragma pack(1)
typedef struct
{ t_size_info entry_size; /* 2+4+4+undef */
  ttablenum tb;
  trecnum record;
  uns8 priv_val[1];
} privil_cache_entry;
#pragma pack()

//////////////////////////////////////// privils ////////////////////////////////////////////
SPECIF_DYNAR(uuid_dynar, WBUUID, 4);
class cur_descr;

typedef class privils
/* Each logged user has this object, temporary created when performing
   priviledges API. */
{ cdp_t cdp;
  unsigned id_count;  // number of UUIDs in groupdef
  uns8 * groupdef;    // EVERYBODY, me, groups & roles: list of privildges used in effective construction
  unsigned cache_start, cache_stop;
  uns8 p_cache[PRIVIL_CACHE_SIZE+sizeof(t_size_info)];
  uns8 cache_invalid; // 0=is valid, 1=empty cache,  2=empty cache & create hierarchy
  tobjnum  user_objnum;  // copy of creating userobj
  tobjname user_objname; // name of the user specified by user_objnum
  tobjnum  temp_author;  // object number of the "author" role the client is temporarily playing

  int find_uuid_pos(uns8 * priv_def, unsigned def_size, uns8 * uuid, int step);
  BOOL cache_next(unsigned &pos);
  void cache_insert(ttablenum tb, trecnum record, int descr_size, uns8 * priv_val);
  void sumarize_privils(unsigned descr_size, uns8 * priv_val, const uns8 * priv_def, unsigned def_size);
  // Adds privileges from priv_def to priv_val according to groupdef
  // Supposed priv_def!=NULL, def_size > 0

  BOOL copy_up_uuids(uuid_dynar * uuids, tobjnum userobj);
  bool am_I_effectively_in_role(tobjnum roleobj);

 public:
  WBUUID user_uuid;  // undef for role
  BOOL is_data_admin;   // is in the DB_ADMIN group
  BOOL is_conf_admin;   // is in the CONF_ADMIN group
  BOOL is_secur_admin;  // is in the SECUR_ADMIN group
  privils(cdp_t par_cdp)
  { cdp=par_cdp;
    groupdef=NULL;  id_count=cache_start=cache_stop=0;  cache_invalid=0;
    user_objnum=ANONYMOUS_USER;  strcpy(user_objname, "ANONYMOUS");  temp_author=NOOBJECT;
  }
  ~privils(void)
    { corefree(groupdef);  }
  inline tobjnum luser(void) const
    { return user_objnum; }
  inline const char * luser_name(void) const
    { return user_objname; }

  bool is_in_groupdef(const WBUUID uuid) const;
  BOOL set_user(tobjnum userobj, tcateg subject_categ, BOOL create_hierarchy);
  bool get_effective_privils(table_descr * tbdf, trecnum rec, t_privils_flex & priv_flex);
  bool get_own_privils      (table_descr * tbdf, trecnum rec, t_privils_flex & priv_flex);
  bool set_own_privils      (table_descr * tbdf, trecnum rec, t_privils_flex & priv_val);
  bool get_cursor_privils   (cur_descr * cd, trecnum rec, t_privils_flex & priv_val, bool effective);
  BOOL any_eff_privil       (table_descr * tbdf, trecnum rec, BOOL write_privil);
  BOOL can_grant_privils    (cdp_t cdp, privils * prvs, table_descr * tbdf, trecnum rec, t_privils_flex & a_priv_val);
  BOOL convert_list         (table_descr * tbdf, trecnum rec, tattrib atr, tattrib * map, int old_atr_count, int new_atr_count, bool global);
  BOOL convert_privils      (table_descr * tbdf, table_all * ta_old, table_all * ta_new);

  inline void reset_cache    (void)      { cache_start=cache_stop=0; }
  inline void reset_hierarchy(void)   // re-set the original user
    { set_user(user_objnum, CATEG_USER, TRUE); }
  inline void set_temp_author(tobjnum temp_authorIn)  // temp. authoring stopped when called with NOOBJECT
    { temp_author=temp_authorIn;  reset_cache(); } // new_privil_version() should not be necessary
  void invalidate_cache(uns8 inv)  { cache_invalid=inv; }
  BOOL am_I_appl_admin (cdp_t cdp, WBUUID apl_uuid);
  BOOL am_I_appl_author(cdp_t cdp, const WBUUID apl_uuid);
  void mark_core(void);
} privils;

/******************************* worm ***************************************/
struct wormbeg;
struct cd_t;
typedef cd_t * cdp_t;

typedef struct         /* worm control block */
{ unsigned  partsize;          /* nominal number of items in a core part */
  wormbeg * firstpart;
  wormbeg * curr_coreptr; /* the current part is always in core - where */
  unsigned  currpos;           /* position in the current part */
  cdp_t     cdp;
  tfcbnum   curr_fcbn;          /* NOFCBNUM iff the current part is core resident */
  wbbool    curr_changed;
} worm;

/********************************** odbc *************************************/
typedef struct
{ tcursnum cursnum;
  trecnum  recnum;
} ins_curs;
SPECIF_DYNAR(ins_curs_dynar, ins_curs, 1);
/////////////////////////////// synchronization /////////////////////////////////
#ifdef WINS

#define CheckCriticalSection(cs) TRUE
extern  SECURITY_ATTRIBUTES  SA;

#define CloseLocalManualEvent(x)       CloseHandle(*x)
#define SetLocalManualEvent(x)         SetEvent(*x)
#define ResetLocalManualEvent(x)       ResetEvent(*x)
#define CreateLocalManualEvent(init,x) ((*x=CreateEvent(&SA, TRUE, init, NULL))!=0)
#define WaitLocalManualEvent(x,tm)     WaitForSingleObject(*x,tm)
#define WaitPulsedManualEvent(x,tm)    WaitForSingleObject(*x,tm)

typedef HANDLE Semaph;
#define CloseSemaph(s)          CloseHandle(*s)
#define CreateSemaph(ini,max,s) ((*s=CreateSemaphore(&SA,ini,max,NULL))!=0) // avoiding name conflict on NLM
#define ReleaseSemaph(s,cnt)    ReleaseSemaphore(*s, cnt, NULL)
#define WaitSemaph(s,tm)        WaitForSingleObject(*s,tm)

#else // !WINS
#ifdef UNIX
#include <semaphore.h>

#else // !WINS, !UNIX: NLM

#include <nwsemaph.h>
#include <process.h>

#endif
#endif

/////////////////////////////// transactor /////////////////////////////////////////
SPECIF_DYNAR(map_dynar,uns32,10);
#define VALID_TJC_LEADER     0x5f
#define VALID_TJC_DEPENDENT  0xc7
#define TJC_INVALID          0

struct t_chain_link;

class t_transactor
{ CRITICAL_SECTION cs_trans;
  map_dynar alloc_map;  // 1-used, 0-free
  tblocknum srch_start; // axiom: no free blocks in alloc_map below srch_start
  tblocknum tjc_init_limit;  // axiom: all tjc block below it are initialized
 public:
  tblocknum get_swap_block(void);
  void free_swap_block(tblocknum block);
  tblocknum save_block(const void * blockadr);
  BOOL load_block(void * blockadr, tblocknum block);

  void free_tjc_blocks(tblocknum * blocks, int count);
  tblocknum save_tjc_block(const void * blockadr);
  void invalidate_tjc_block(tblocknum block);

  tblocknum save_chain_block(tptr nodes, unsigned bytes, t_chain_link * chl);
  void load_chain_block(tptr nodes, tblocknum block);
  void save_a_block(tptr nodes, tblocknum block);

  t_transactor(void);
  ~t_transactor(void);
  void construct(void);
  void mark_core(void)
    { alloc_map.mark_core(); }
};

extern t_transactor transactor;
/////////////////////////////////// gen_hash /////////////////////////////////////
template <class t_hash_item, int min_size> class t_gen_hash
{
  t_hash_item min_table[min_size];
  t_hash_item * table;  // pointer to the actual hash table, it may be ==min_table or allocated
  int tablesize, contains, limitsize;
  bool error;
 public:
  void add(const t_hash_item * itm);
  t_gen_hash(void)
    { table=min_table;  tablesize=min_size;  contains=0;  limitsize=min_size-1;  error=false; }
  ~t_gen_hash(void)
    { if (table!=min_table) delete [] table; }
  t_hash_item * find(const t_hash_item & itm);
  bool inflate(void);
  bool is_full(void) const
    { return contains==limitsize; }
  void clear(void);
  void mark_core(void)  // does not mark [this]!
    { if (table!=min_table) mark_array(table); 
      for (int i=0;  i<tablesize;  i++) table[i].mark_core();
    }
  void inserted(void)
    { contains++; }
 // Passing all valid items in the table. Set pos=0 before 1st call. Returns NULL after passing all items.
  const t_hash_item * get_next(unsigned & pos) const
    { while (pos<tablesize)
        if (!table[pos].is_empty() && !table[pos].is_invalid())
          return &table[pos++];
        else pos++;
      return NULL;
    }
};
/* NOTE: t_hash_item class must not have destructor. It must have explicit destructing method destruct().
   Both constructor and destruct() must set the empty state.
*/

/////////////////////////////////// index synchronization //////////////////////////////////////////
enum { IND_PAGE_UPDATED=1, IND_PAGE_ALLOCATED=2, IND_PAGE_FREED=4, IND_PAGE_PRIVATE=8 };

class t_hash_item_iuad
// Record about updating/allocation/deallocation/"making private" of a page in an index.
{public:
  tblocknum block;
  int oper;
 public:
  t_hash_item_iuad(void)  // must set the empty state
    { block=0; }
  void destruct(void)     // called for invalid and empty items, too!
    { block=0; }          // must set the empty state
  bool is_empty(void) const
    { return block==0; }
  bool is_invalid(void) const
    { return block==(tblocknum)-1; }
  bool same_key(const t_hash_item_iuad & itm) const
    { return block==itm.block; }
  unsigned hash(void) const
    { return block; }
  void mark_core(void)  // called for invalid and empty items, too!
    { }
  void invalidate(void)
    { block=(tblocknum)-1; }
 // methods not related to the template:
  t_hash_item_iuad(tblocknum blockIn)  
    { block=blockIn; }
  void set(tblocknum blockIn, int operIn)  
    { block=blockIn;  oper=operIn; }
};

typedef t_gen_hash<t_hash_item_iuad, 7> t_gen_hash_iuad; 

class t_log;

struct t_index_uad
// Description of all uncommitted changes made on index identfied by [root] in a transaction log owning the object
// Access protection: accessed only by owning client with RL on the [root] or by committing client with [WL] on the [root].
{ const tblocknum root;    // index is identified by its root number
  const tblocknum tabnum;  // used for replaying
  bool invalid;            // set by other clients when my chnages become invalid because of committed changes in an intersecting set of pages
  bool write_locked;       // owning a write-lock on the index root
  t_gen_hash_iuad info;    // Record about updating/allocation/deallocation/"making private" of a page in an index.
  t_index_uad * next;
  t_index_uad(tblocknum rootIn, ttablenum tbnumIn) : root(rootIn), tabnum(tbnumIn)
    { invalid=false;  write_locked=false;  next=NULL; }
  void mark_core(void)
  { mark(this);
    info.mark_core();
  }
 // registering operations on pages:
  void page_allocated(tblocknum page);
  bool page_freed    (tblocknum page);
  void page_updated  (tblocknum page);
  void page_private  (tblocknum page);
 // committing or rolling-back:
  void wr_changes     (cdp_t cdp, t_log * log);
  void remove_changes_and_rollback_allocs(cdp_t cdp, t_client_number owner);
  void remove_my_changes_and_rollback_allocs(cdp_t cdp);
  void commit_allocs  (cdp_t cdp);
};

//////////////////////////////////// Transakcni log ////////////////////////////////
#define EMPTY_TAG           0xffffffff  // this value can be easilly set by memset
#define LOCK_TAG            0xfffffffd  // objnum contains table number (says that there is a temp. lock on a permanent table)
#define INSREC_TAG          0xfffffffb    // record specified by recnum inserted into the tabke
#define DELREC_TAG          0xfffffffa    // record specified by recnum deleted from the tabke
#define INVAL_TAG           0xfffffff9
#define CHANGE_TAG          0xfffffff7
#define TAG_ALLOC_PAGE      0xfffffff6
#define TAG_FREE_PAGE       0xfffffff5
#define TAG_ALLOC_PIECE     0xfffffff4
#define TAG_FREE_PIECE      0xfffffff3

struct t_chngmap  // map of changed records in a page
{ int   recsize;
  uns32 map[1];
};

struct t_logitem
{ int tag;
  union
  { tobjnum   objnum;
    tblocknum block;
  };
  union
  { struct
    { uns8  offs32;
      uns8  len32;
    } act;
    t_chngmap * chngmap;  // NULL iff no changes so far, -1 if changed all, map pointer if some changes
                          // valid only if is_change()
    trecnum recnum;
  };

  inline bool invalid  (void) const { return tag == INVAL_TAG; }
  inline bool empty    (void) const { return tag == EMPTY_TAG; }
  inline bool nonempty (void) const { return tag != EMPTY_TAG; }
  inline bool is_change(void) const { return tag == CHANGE_TAG; }
  inline bool is_alloc (void) const { return tag <= TAG_ALLOC_PAGE && tag >= TAG_FREE_PIECE; }
  inline void invalidate(void) { tag = INVAL_TAG; }
 // constructors defining the hash-key:
  inline t_logitem(int tagIn, tblocknum blockIn)  // used for setting the [objnum] too
    { tag=tagIn;  block=blockIn;  recnum=0; }
  inline t_logitem(int tagIn, tblocknum blockIn, uns8 offs32In, uns8 len32In)
    { tag=tagIn;  block=blockIn;  recnum=0;  act.offs32=offs32In;  act.len32=len32In; }
  inline t_logitem(int tagIn, tblocknum blockIn, trecnum recnumIn)
    { tag=tagIn;  block=blockIn;  recnum=recnumIn; }
  inline t_logitem(void)
    { tag=EMPTY_TAG; }
 // hash:
  inline int hashf(int tablesize) const
  { unsigned x = tag ^ block;    
    if (tag==INSREC_TAG || tag==DELREC_TAG) x ^= recnum;
    return (117 * x) % tablesize;  // when allocating & deallocating pieces same dadr used many times: must multiply by a big number
  }
};

struct tpiece;
#define FULL_CHNGMAP  (t_chngmap*)-1
#define INDEX_CHNGMAP (t_chngmap*)-2

class t_log // log of transaction changes (block&piece alloc/dealloc, 
            // changes in pages, new tables & cursors
// Hash logic: removed items are replaced by items marked as invalid
{ enum { MINIMAL_HTABLE_SIZE=7, HTABLE_PRESERVE_LIMIT=7 };
  int contains,   // nonempty entries in the table, including invalid
      limitsize;  // maximal entries in the table, < tablesize
  t_logitem minimal_htable[MINIMAL_HTABLE_SIZE];
  BOOL error;     // accumulated error
  //inline int hashf(tblocknum dadr) { return 32*dadr % tablesize; }  // 32: fast multiply, when allocating pieces same dadr used many times
  void * htable_handle;
 public:
  int tablesize;  // cells in the table
  t_logitem * htable;
  BOOL any_sapos;
  bool has_page_temp_lock;

  void grow_table(void);
  void minimize(void);
  BOOL is_error(void) const { return error; }
  bool nonempty(void) const { return contains!=0; }
  void clear_error(void) { error=FALSE; }

  t_log(void)
  { htable = minimal_htable;  // prevents minimize to do anything
    htable_handle=0;
    tablesize=MINIMAL_HTABLE_SIZE;
    limitsize=MINIMAL_HTABLE_SIZE-1;
    contains=0;
    error=FALSE;  any_sapos=FALSE;
    has_page_temp_lock=false; 
  }
  ~t_log(void) 
  { if (htable != minimal_htable) aligned_free(htable_handle); 
  }

  void register_object(tobjnum objnum, int tag);
  void unregister_object(tobjnum objnum, int tag);
  uns32 * rec_changed(tfcbnum fcbn, tblocknum dadr, int recsize, int recpos);
  void piece_changed(tfcbnum fcbn, const tpiece * pc);
  void page_private(tblocknum dadr);
  void page_changed(tfcbnum fcbn);
  void page_changed2(tblocknum dadr, t_chngmap * achngmap);
  void page_allocated(tblocknum dadr);
  bool page_deallocated(tblocknum dadr);
  void drop_page_changes(cdp_t cdp, tblocknum dadr);
  void piece_allocated(tblocknum dadr, int len32, int offs32);
  BOOL piece_deallocated(tblocknum dadr, int len32, int offs32);
  void drop_piece_changes(tpiece * pc);
  void write_all_changes(cdp_t cdp, bool & written);
  void remove_all_changes(cdp_t cdp);
  void commit_allocs(cdp_t cdp);
  void rollback_allocs(cdp_t cdp);
  int  total_records(void) { return contains; }
  void mark_core(void);
  //void page_perm_allocated(tblocknum dadr);
  //BOOL page_perm_deallocated(tblocknum dadr);
  void record_released(ttablenum tabnum, trecnum rec);
  void record_inserted(ttablenum tabnum, trecnum rec);
  void drop_record_ins_info(ttablenum tabnum, trecnum rec);
  void table_deleted(ttablenum tabnum); // removed data stored by record_released and record_inserted
  void destroying_subtrans(cdp_t cdp);
  bool has_change(tblocknum dadr) const;
};

void make_conflicting_indexes_invalid(cdp_t cdp);
/////////////////////////// registering transaction-dependent objects ///////////////
// List of objects affected in the tranaction which have to be cleared/changed on rollback
class cur_descr;
enum t_trans_obj_type { TRANS_OBJ_TABLE, TRANS_OBJ_CURSOR, TRANS_OBJ_INDEX, TRANS_OBJ_BASBL };
struct t_trans_obj
{ const t_trans_obj_type trans_obj_type;
  t_trans_obj * next;
  t_trans_obj(t_trans_obj_type trans_obj_typeIn) : trans_obj_type(trans_obj_typeIn) 
   { }
  void add_to_list(cdp_t cdp);
};

struct t_trans_table : t_trans_obj
{ ttablenum tbnum;
  t_trans_table(void) : t_trans_obj(TRANS_OBJ_TABLE) { }
};

struct t_trans_cursor : t_trans_obj
{ cur_descr * cd;  
  t_trans_cursor(void) : t_trans_obj(TRANS_OBJ_CURSOR) { }
};

#ifdef STOP
struct t_trans_index : t_trans_obj
{ tblocknum root;
  unsigned bigstep;
  t_trans_index(tblocknum rootIn, unsigned bigstepIn) : t_trans_obj(TRANS_OBJ_INDEX),
    root(rootIn), bigstep(bigstepIn) { }
};
#endif

struct t_trans_basbl : t_trans_obj
{ tblocknum basblockn;
  wbbool multpage;
  bool created;
  uns16 rectopage;
  int indxcnt;
  t_trans_basbl(tblocknum basblocknIn, wbbool multpageIn, uns16 rectopageIn, int indxcntIn, bool createdIn) : 
    t_trans_obj(TRANS_OBJ_BASBL),
    basblockn(basblocknIn), multpage(multpageIn), rectopage(rectopageIn), indxcnt(indxcntIn), created(createdIn) { }
};


void register_cursor_creation(cdp_t cdp, cur_descr * cd);
void unregister_cursor_creation(cdp_t cdp, cur_descr * cd);
void register_table_creation(cdp_t cdp, ttablenum tbnum);
void unregister_table_creation(cdp_t cdp, ttablenum tbnum);
void register_basic_allocation(cdp_t cdp, tblocknum basblockn, wbbool multpage, uns16 rectopage, int indxcnt);
void register_basic_deallocation(cdp_t cdp, tblocknum basblockn, wbbool multpage, uns16 rectopage, int indxcnt);

///////////////////////////////// Savepoints //////////////////////////////////
class t_savepoints;

struct dadrpair
{ tblocknum fil_dadr,
            tra_dadr;
  struct  // same structure as action map
  { uns8 offs32;
    uns8 len32;
    uns16 sapo_oper;  // values of action_tag or 0 
  } act;
};

class t_savepoint
{ friend class t_savepoints;
  t_idname name; // unnamed savepoints are referenced by number (when *name==0)
  int   num;  // from <1..0x7fff>, 0 when referenced by name
  int   pairscnt;
  BOOL  atomic;  // created implicitly by BEGIN ATOMIC
  dadrpair * pairs;
  ~t_savepoint(void)  { delete [] pairs; }
};

SPECIF_DYNAR(savepoint_dynar,t_savepoint,1);

class t_savepoints
{ int counter;           // used for sapo number allocation (last used value)
  savepoint_dynar sapos; // list of active sapos, bigger index => later creation
  int get_number(void);
 public:
  t_savepoints(void) { counter=0; }
  ~t_savepoints(void)
  { for (int i=0;  i<sapos.count();  i++)
      sapos.acc0(i)->t_savepoint::~t_savepoint();
  }
  int  find_sapo(const char * name, int num);
  int  find_atomic_index(void);
  void destroy(cdp_t cdp, int ind, BOOL all);
  void rollback(cdp_t cdp, int ind);
  BOOL create(cdp_t cdp, const char * name, int & num, BOOL atomic);
  void mark_core(void)
  { for (int i=0;  i<sapos.count();  i++)
      mark(sapos.acc0(i)->pairs);
    sapos.mark_core();
  }
};
/////////////////////////////// t_trigger_info ///////////////////////////////
struct t_trigger_info
{ ttablenum tb;
  tobjnum   trigobj;
  int       trigger_type;
};
SPECIF_DYNAR(t_trigger_info_dynar,t_trigger_info,4);
////////////////////////////// DLLs //////////////////////////////////////////
struct t_library
{ enum { MAX_LIB_NAME_LEN=255 };
  char name[MAX_LIB_NAME_LEN+1];
  HINSTANCE hLibInst;
};

SPECIF_DYNAR(libs_dynar,t_library,1);
////////////////////////////// dbginfo /////////////////////////////////////////
struct t_bp
{ tobjnum objnum;
  uns32   line;
  BOOL    temporary;
};
SPECIF_DYNAR(bps_dynar,t_bp,1);

struct t_stack_item : public t_stack_info9
{ t_linechngstop linechngstop;
  t_stack_item * next;
};

class t_dbginfo
{ bps_dynar bps;
  Semaph hDebugSem;
  cdp_t cdp;  // the cdp pointing to and owning this t_dbginfo
  int disabled;
 public:
  bool    breaked;
  bool    was_linechange;  // ...since the last break, allows stopping on a BP even if the previous stop was on the same line

  void go(void);
  void bp_add(tobjnum objnum, uns32 line, BOOL temp);
  BOOL bp_remove(tobjnum objnum, uns32 line, BOOL temp);
  void bp_remove_bps(BOOL temp_only);
  void check_break(uns32 line);
  void eval(cdp_t rep_cdp, int level_limit, tptr source, cdp_t dbg_cdp, int dbgop);
  void stack(cdp_t rep_cdp, cdp_t dbg_cdp);
  void stack9(cdp_t rep_cdp);
  void assign(cdp_t cdp, int level_limit, tptr source, tptr value, cdp_t dbg_cdp);
  void mark_core(void)
  { mark(this);  bps.mark_core(); 
  }
  void close(void);
  void kill(void);
  void enable(void)
    { disabled--; }
  void disable(void)
    { disabled++; }

  t_dbginfo(cdp_t cdpIn) : cdp(cdpIn)
  { breaked=false;  CreateSemaph(0, 100, &hDebugSem);  /*linechngstop=LCHNGSTOP_NEVER;*/  
    was_linechange=true;  disabled=0;
  }
  ~t_dbginfo(void)
  { CloseSemaph(&hDebugSem); 
  }
};
/****************************** cdp ******************************************/
enum t_except { // execution state
  EXC_NO,           // normal execution
  EXC_LEAVE,        // leaving until block/loop/routine named [except_name] leaved, tested by stop_leaving_for_name()
  EXC_EXC,          // exception [except_name] occured, not handled so far, looking for a handler,
                    // When except_name[0]==SQLSTATE_EXCEPTION_MARK, except_name+1 is sqlstate and return_code is defined
  EXC_RESIGNAL,     // RESIGNAL executed, to be converted into EXC_EXC
  EXC_HANDLED_RB    // rollback exception has been handled, but leaving continues (chain mode)
};

/***************************** wbstmt ****************************************/
struct WBPARAM
{ uns32 len;  // 0 means the NULL value, but NULL value does not imply len==0
  tptr  val;
  tptr realloc(unsigned size);
  void free(void);
  void mark_core(void);
};

struct t_emparam
{ tname     name; // the only place "tname" is used on the server
  int       type;
  t_specif  specif;
  int       length;  // in bytes
  t_parmode mode;
  bool      val_is_indep;  // false on init
  tptr      val;  // owned iff !val_is_indep
  tptr  realloc(unsigned size);
  void  free(void);
  void  mark_core(void);
};

SPECIF_DYNAR(marker_dynar,MARKER,3);
SPECIF_DYNAR(wbparam_dynar,WBPARAM,3);
SPECIF_DYNAR(emparam_dynar,t_emparam,1);

class sql_statement;

struct wbstmt
{ const uns32 hstmt;
  marker_dynar markers;
  int disp;  // marker position displacement
  tcursnum cursor_num;
  wbstmt * next_stmt;
  wbparam_dynar dparams;
  emparam_dynar eparams;
  sql_statement * so;
  const char * source;
  wbstmt(cdp_t cdp, uns32 handle);
  ~wbstmt(void);
  void mark_core(void);
  t_emparam * find_eparam_by_name(const char * name)  const;
};
//////////////////////////// sequence store ////////////////////////////////
struct t_seq_list
{ tobjnum seq_obj;
  uns32 statement_cnt_val;
  t_seq_value val;
  t_seq_list * next;
  inline t_seq_list(tobjnum seq_objIn) 
    { seq_obj=seq_objIn;  statement_cnt_val=0; }
  ~t_seq_list(void);
  void mark_core(void)
    { mark(this);  if (next) next->mark_core(); }
};
/////////////////////////// list of changes in the columns //////////////////
SPECIF_DYNAR(recnum_dynar, trecnum, 4);

#define INVALID_HASHED_RECNUM ((trecnum)-2)

class t_record_hash
{ enum { MINIMAL_HTABLE_SIZE=4 };
  int contains,   // nonempty entries in the table, including invalid
      limitsize;  // maximal entries in the table, < tablesize
  trecnum minimal_htable[MINIMAL_HTABLE_SIZE];
  int tablesize;  // cells in the table
  trecnum * allocated_htable;
  inline int hashf(trecnum rec) { return rec % tablesize; }
  bool grow_table(void);
  t_record_hash(void) { init(); }  // not used!
 public:
  void reinit(void);
  inline void init(void)
    { allocated_htable=NULL;  reinit(); }
  inline bool get_next(int & ind, trecnum & rec)
  { if (ind>=tablesize) return false;
    rec=(allocated_htable ? allocated_htable : minimal_htable)[ind];
    ind++;
    return true;
  }

  bool insert_record(trecnum rec);
  void remove_record(trecnum rec);
  void mark_core(void)
    { if (allocated_htable) mark(allocated_htable); }
};

struct t_delayed_change
{ ttablenum     ch_tabnum;
  t_record_hash ch_recnums;
  t_atrmap_flex ch_map;
};
SPECIF_DYNAR(ch_dynar, t_delayed_change, 2);

////////////////////////////////////// class t_translog ///////////////////////////////////////
enum t_translog_types { // types of transaction logs
  TL_CLIENT,    // basic log of the client (every client has one in cd.tl)
  TL_CURSOR,    // log of a cursor (for all temp. tables and indexes in the cursor)
  TL_LOCTAB };  // log of a local table

class t_translog
{public:
  const t_translog_types tl_type;  // type of the transaction log
  t_translog * next;  // forms the list of secondary logs beloging to a client, not used for TL_CLIENT
  t_log log;
  ch_dynar ch_s; // changes record: used by deferred checking of constrains and PARIND,
  t_trans_obj * trans_objs;

  t_translog(t_translog_types tl_typeIn) : tl_type(tl_typeIn)
    { next=NULL;  trans_objs=NULL; }

  void destruct(void);
  virtual ~t_translog(void)
    { destruct(); }

 // API for deferred constrains:
  void changes_mark_core(void);
  void destruct_changes(void);
  void clear_changes(void);
  void unregister_changes_on_table(ttablenum tbnum);
  void unregister_change(ttablenum tbnum, trecnum recnum);

  bool pre_commit(cdp_t cdp);

  bool commit(cdp_t cdp);
  void rollback(cdp_t cdp);
  void destroying_subtrans(cdp_t cdp)  // deferred constrains and transaction objects will be dropped in the destructor
    { log.destroying_subtrans(cdp); }

  void rollback_transaction_objects_post(cdp_t cdp);
  void commit_transaction_objects_post(cdp_t cdp);
  void drop_transaction_objects(void);
  virtual void mark_core(void)
  { log.mark_core();
    changes_mark_core();
    for (t_trans_obj * to = trans_objs;  to;  to=to->next)
      mark(to);
  }

  inline void page_changed(tfcbnum fcbn)
    { log.page_changed(fcbn); }
  inline void page_private(tblocknum dadr)
    { log.page_private(dadr); }
 // implementations used by t_translog_cursor and t_translog_loctab
  virtual void index_page_changed(cdp_t cdp, tblocknum root, tfcbnum fcbn)
    { log.page_changed(fcbn); }
  virtual void index_page_private(cdp_t cdp, tblocknum root, tblocknum page)
    { log.page_private(page); }

  virtual tblocknum alloc_block      (cdp_t cdp);
  virtual void      free_block       (cdp_t cdp, tblocknum dadr);
  virtual tblocknum alloc_big_block  (cdp_t cdp);
  virtual void      free_big_block   (cdp_t cdp, tblocknum dadr);
  virtual tblocknum alloc_index_block(cdp_t cdp, tblocknum root) = 0;
  virtual void      free_index_block (cdp_t cdp, tblocknum root, tblocknum dadr) = 0;
};

void destroy_subtrans(cdp_t cdp, t_translog * translog);

class t_translog_main : public t_translog
{
  friend void make_conflicting_indexes_invalid(cdp_t cdp);
  t_index_uad * index_uad;
  t_index_uad * get_uiad(cdp_t cdp, tblocknum root);
  void index_page_updated  (cdp_t cdp, tblocknum root, tblocknum page);
 public:
  t_index_uad * find_index_uad(tblocknum root);
  tblocknum alloc_index_block(cdp_t cdp, tblocknum root);
  void      free_index_block (cdp_t cdp, tblocknum root, tblocknum dadr);
  void index_page_private  (cdp_t cdp, tblocknum root, tblocknum page);
  void index_page_changed  (cdp_t cdp, tblocknum root, tfcbnum fcbn);
  void mark_core(void)
  { t_translog::mark_core();
    for (t_index_uad * iuad = index_uad;  iuad;  iuad=iuad->next)
      iuad->mark_core();
  }

  void clear_index_info(cdp_t cdp);
  void index_page_allocated(cdp_t cdp, tblocknum root, tblocknum page);
  bool index_page_freed    (cdp_t cdp, tblocknum root, tblocknum page);
  void drop_iuad_by_root   (tblocknum root);
  int  lock_all_index_roots(cdp_t cdp, bool locking);
  bool update_all_indexes  (cdp_t cdp);
  void write_all_changes   (cdp_t cdp, bool & written);   // called on commit
  void rollback            (cdp_t cdp);
  void commit_index_allocs (cdp_t cdp);
  inline bool nonempty(void) const 
   { return log.nonempty() || index_uad!=NULL; }

  t_translog_main(cdp_t cdp) : t_translog(TL_CLIENT)
  { index_uad=NULL; };
  ~t_translog_main(void)
  {// delete the [index_uad] list: 
    clear_index_info(NULL);
  }
};

class t_translog_loctab : public t_translog
// Translog of a local table
{public:
  table_descr * tbdf;   // not owning
  bool table_comitted;  // false on init, set to true on 1st commit
  t_translog_loctab(cdp_t cdp);

  tblocknum alloc_index_block(cdp_t cdp, tblocknum root);
  void      free_index_block (cdp_t cdp, tblocknum root, tblocknum dadr);
};

class t_translog_cursor : public t_translog
{
  tblocknum alloc_block      (cdp_t cdp);
  void      free_block       (cdp_t cdp, tblocknum dadr);
  tblocknum alloc_big_block  (cdp_t cdp);
  tblocknum alloc_index_block(cdp_t cdp, tblocknum root);
  void      free_index_block (cdp_t cdp, tblocknum root, tblocknum dadr);
 public:
  t_translog_cursor(cdp_t cdp);
};

///////////////////////////////////// execution kontext /////////////////////////////////////////////
enum t_exkont_type { EKT_NO, EKT_TABLE, EKT_INDEX, EKT_PROCEDURE, EKT_TRIGGER, EKT_SQLSTMT, EKT_LOCK, EKT_COMP, EKT_RECORD, EKT_KEY };

#define MAX_EXKONT_TEXT  255

struct t_exkont_info
{ t_exkont_type type;
  int par1, par2, par3, par4;
  char text[MAX_EXKONT_TEXT+1];
};

class t_exkont
{ t_exkont * next;
  t_exkont_type type;
 protected:
  cdp_t cdp;  // used in the destructor
 public:
  t_exkont(cdp_t cdpIn, t_exkont_type typeIn); // inserts itself on the top of the list
  virtual ~t_exkont(void);                             // removes itself from the top of the list
  virtual void get_descr(cdp_t cdp, char * buf, int buflen) 
    { *buf=0; }  // is called when the context is partially destructed and waits
  t_exkont * _next(void) { return next; }
  unsigned count(void)  // returns the length of the context chain
  { t_exkont * kont = this;  unsigned cnt=1;
    while (kont->next) { kont=kont->next;  cnt++; }
    return cnt;
  }
  virtual void get_info(cdp_t cdp, t_exkont_info * info)
  { info->type=type; }
};

t_exkont * lock_and_get_execution_kontext(cdp_t acdp);
void unlock_execution_kontext(cdp_t acdp);

class t_exkont_table : public t_exkont
{ ttablenum tbnum;  trecnum recnum;  tattrib attr;  uns16 index;  int oper;
  const void * val;  uns32 vallength;
 public:
  t_exkont_table(cdp_t cdpIn, ttablenum tbnumIn, trecnum recnumIn, tattrib attrIn, uns16 indexIn, int operIn, const void * valIn, unsigned vallenIn);
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
  virtual void get_info(cdp_t cdp, t_exkont_info * info)
  { t_exkont::get_info(cdp, info);  info->par1=tbnum;  info->par2=recnum;  info->par3=attr;  info->par4=index; } 
};

class t_exkont_record : public t_exkont
{ ttablenum tbnum;  trecnum recnum;  
 public:
  t_exkont_record(cdp_t cdpIn, ttablenum tbnumIn, trecnum recnumIn) : t_exkont(cdpIn, EKT_RECORD)
    { tbnum=tbnumIn;  recnum=recnumIn; }
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
  virtual void get_info(cdp_t cdp, t_exkont_info * info)
  { t_exkont::get_info(cdp, info);  info->par1=tbnum;  info->par2=recnum; } 
};

class t_exkont_key : public t_exkont
{ const dd_index * idx_descr;  const char * key;  // short-term ptrs
 public:
  t_exkont_key(cdp_t cdpIn, const dd_index * idx_descrIn, const char * keyIn) : t_exkont(cdpIn, EKT_KEY)
    { idx_descr=idx_descrIn;  key=keyIn; }
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
};

class t_exkont_comp : public t_exkont
{ 
 public:
  t_exkont_comp(cdp_t cdpIn) : t_exkont(cdpIn, EKT_COMP)
  {  }
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
  virtual void get_info(cdp_t cdp, t_exkont_info * info)
  { t_exkont::get_info(cdp, info); } 
};

class t_exkont_index : public t_exkont
{ ttablenum tbnum;  int indnum;
 public:
  t_exkont_index(cdp_t cdpIn, ttablenum tbnumIn, int indnumIn) : t_exkont(cdpIn, EKT_INDEX)
  { tbnum=tbnumIn;  indnum=indnumIn; }
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
  virtual void get_info(cdp_t cdp, t_exkont_info * info)
  { t_exkont::get_info(cdp, info);  info->par1=tbnum;  info->par2=indnum; } 
};

class t_routine;

class t_exkont_proc : public t_exkont
{ const t_rscope * rscope;  const t_routine * routine;
 public:
  t_exkont_proc(cdp_t cdpIn, const t_rscope * rscopeIn, const t_routine * routineIn) : t_exkont(cdpIn, EKT_PROCEDURE)
  { rscope=rscopeIn;  routine=routineIn; }
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
  virtual void get_info(cdp_t cdp, t_exkont_info * info);
};

class t_exkont_trig : public t_exkont
{ const t_trigger * trigger;
 public:
  t_exkont_trig(cdp_t cdpIn, const t_trigger * triggerIn) : t_exkont(cdpIn, EKT_TRIGGER)
  { trigger=triggerIn; }
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
  virtual void get_info(cdp_t cdp, t_exkont_info * info);
};

class t_exkont_sqlstmt : public t_exkont
{ const wbstmt * stmt;  const char * source;
 public:
  t_exkont_sqlstmt(cdp_t cdpIn, const wbstmt * stmtIn, const char * sourceIn) : t_exkont(cdpIn, EKT_SQLSTMT)
  { stmt=stmtIn;  source=sourceIn; }
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
  virtual void get_info(cdp_t cdp, t_exkont_info * info)
  { t_exkont::get_info(cdp, info); } 
};

class t_exkont_lock : public t_exkont
{ ttablenum tbnum;  trecnum recnum;  int locktype;  tobjnum usernum;
 public:
  t_exkont_lock(cdp_t cdpIn, ttablenum tbnumIn, trecnum recnumIn, int locktypeIn, tobjnum usernumIn) : t_exkont(cdpIn, EKT_LOCK)
  { tbnum=tbnumIn;  recnum=recnumIn;  locktype=locktypeIn;  usernum=usernumIn; }
  virtual void get_descr(cdp_t cdp, char * buf, int buflen);
  virtual void get_info(cdp_t cdp, t_exkont_info * info)
  { t_exkont::get_info(cdp, info);  info->par1=tbnum;  info->par2=recnum;  info->par3=locktype;  info->par4=usernum; } 
};

//////////////////////////////////////////// events ////////////////////////////////////////////////
// Unregister_event is called by the same thread as Wait, so I do not have to Cancel waiting threads when unregistering
// Register, Unregister, Cancel and Invoke processed in a CS: prevents reallocation of dynar or deleting event
//  when somebody else is using it.
// EventName is case-insensitive, ParamStr is case-sensitive.

class t_invoked_ev
{public:
  tobjname EventName;  // in upper case
  char * ParamStr;     // NULL for empty string
  unsigned count;
};

struct t_event_occurrence
{ unsigned count;
  t_event_occurrence * next;
  char param[EMPTY_ARRAY];
  void *operator new(size_t size, unsigned data_size);
#ifdef WINS
  void  operator delete(void * ptr, unsigned data_size);
#endif
#if !defined(WINS) || defined(X64ARCH)
  void  operator delete(void * ptr);
#endif
  t_event_occurrence(const char * ParamStr);
};

class t_reg_ev
{public: 
  tobjname EventName;  // in upper case, unused when *EventName==0
  char * ParamStr;
  BOOL exact;
  uns32 handle;
  unsigned count;            // used as total count for all occurrences
  t_event_occurrence * occ;  // used only when exact==FALSE
  void clear(void);
};

SPECIF_DYNAR(t_invoked_ev_dynar, t_invoked_ev, 2);
SPECIF_DYNAR(t_reg_ev_dynar, t_reg_ev, 2);

class event_management
{public:
  int cancel_requested;           // 0=not or WAIT_EVENT_CANCELLED or WAIT_EVENT_SHUTDOWN
  t_invoked_ev_dynar invoked_ev;  // uncommitted events, accessed by the owner only
  t_reg_ev_dynar reg_ev;          // events registered by the owner
//  LocEvent event_signal;  // signalled on Invoke, Cancel, Shutdown
  Semaph event_semaphore;  // signalled on Invoke, Cancel, Shutdown
  BOOL is_init;
  void mark_core(void);
  void preinit(void)
    { is_init=FALSE; }
  void close(void);
  void cancel_events(void);
  void commit_events(cdp_t cdp);
  void register_event(cdp_t cdp, const char * EventName, const char * ParamStr, BOOL exact, uns32 * ev_handle);
  void unregister_event(uns32 ev_handle);
  int  wait_for_event(cdp_t cdp, int timeout, uns32 * EventHandle, uns32 * event_count, char * ParamStr, sig32 param_size);
  void shutdown(void);
  void invoke_event(cdp_t cdp, const char *  EventName, const char * ParamStr);
  event_management(void)
    { preinit(); }
  void cancel_event_if_waits(cdp_t cdp);
};

void cancel_event_wait(cdp_t cdp, uns32 ev_handle);

/***************************** proces type and state (values of cdp->in_use) *******************************/
enum t_appl_state { NOT_LOGGED=0 /*and not connected*/, WAITING_FOR_REQ=1, REQ_RUNS=3, SLAVESHUTDOWN=5 };
enum t_trans      { TRANS_NO, TRANS_IMPL, TRANS_EXPL };
typedef sig64 prof_time;   // time value used by the profiler

class cAddress;
enum t_wait { WAIT_NO=0, WAIT_PAGE_LOCK=1, WAIT_GET_FRAME=2, WAIT_RECORD_LOCK=3, 
              WAIT_FOR_REQUEST=4, WAIT_SENDING_ANS, WAIT_RECEIVING, WAIT_TERMINATING, WAIT_SEMAPHORE, WAIT_SLEEP, WAIT_EVENT,
              WAIT_ME=99,
              WAIT_CS_SEQUENCES=100, WAIT_CS_PIECES, WAIT_CS_CLIENT_LIST, WAIT_CS_TRACE_LOGS,
              WAIT_CS_CURSORS,       WAIT_CS_FIL,    WAIT_CS_COMMIT,      WAIT_CS_BPOOLS,    WAIT_CS_FRAME/*108*/, WAIT_CS_TRANS,
              WAIT_CS_PRIVIL,        WAIT_CS_MEMORY, WAIT_CS_TABLES,      WAIT_CS_PSM_CACHE, WAIT_CS_LOCKS, WAIT_CS_SHORT_TERM,
              WAIT_CS_DEADLOCK,      WAIT_EXTFT, 
              WAIT_DADR_LOCK/*118*/, WAIT_IMPOSS,    WAIT_MIWLOCK };

class t_answer  // the server's answer to the client's request
{ friend struct cd_t;
  unsigned    anssize;     // current size of the answer
  answer_cb * ans;         // pointer to the answer buffer
  unsigned    ansbufsize;  // size of the answer buffer
  answer_cb   min_answer;  // static buffer used for short answers
  BOOL        dyn_alloc;   // the ans structure is allocated dynamically and must be released after sending
 public:
  t_answer(void)
    { min_answer.flags=0; } // this value is constant, does not have to be reassigned in each ans_init()
  inline void ans_init(void) // inits the answer at the begining of the request processing
  { ans=&min_answer;  
    dyn_alloc =FALSE;
    ansbufsize=sizeof(answer_cb);
    anssize   =ANSWER_PREFIX_SIZE;  /* size of the prefix */
    min_answer.return_code=ANS_OK;
  }
  inline void ans_error(int error_code) // inits the answer with the error_code
  // Used when error occures before the answer is initialised by ans_init
    { ans_init();  min_answer.return_code=error_code; }
  inline const void * get_ans(void) const { return ans; }
  inline unsigned get_anssize(void) const { return anssize; }
  inline void sized_answer_start(void)
  { *(uns32 *)get_ans_ptr(sizeof(uns32)) = 0; } /* inits result size */
  inline void sized_answer_stop(void)
  { *(uns32 *)ans->return_data = anssize-ANSWER_PREFIX_SIZE-sizeof(uns32);
  }
  inline void free_answer(void) // Undefined state after this call! Must call ans_init
    { if (dyn_alloc) corefree(ans);  ans_init(); }  // must call ans_init!
  void ans_mark_core(void)
    { if (dyn_alloc) mark(ans); }
  tptr get_ans_ptr(unsigned size);
};

extern "C" BOOL WINAPI request_error(cdp_t cdp, int i);  // forward declaration

struct t_dws
{ tblocknum locking_dadr;  // the block number I'm waiting for
  cdp_t next;    // next client thread in the waiting list
#ifdef WINS
  HANDLE hSemaphore;
  t_dws(void)
    { hSemaphore=CreateSemaphore(&SA, 0, 1, NULL); }
  ~t_dws(void)
    { CloseHandle(hSemaphore); }
#else
  sem_t  hSemaphore;
  t_dws(void)
    { sem_init(&hSemaphore, 0, 0); }
  ~t_dws(void)
    { sem_destroy(&hSemaphore); }
#endif  
};

struct cd_t
{ friend wbstmt::wbstmt(cdp_t cdp, uns32 handle);
  const t_cdp_type in_use;
 // slave identification:
  t_client_number applnum_perm;  // number identifying the client (in locks, frames, cursors, d_cached rights, cached subquery values, messages)
  t_client_number applnum;       // number identifying the client or ANY_PAGE in frame access requests
  uns32       session_number;
 // slave varibles initalized once when slave connected (kernel_cdp_init):
  uns32 ker_flags;   /* flags for request processing by the server */
  wbbool initialized; // state between kernel_cdp_init and kernel_cdp_free
  wbbool volatile break_request;
  t_trans expl_trans;  /* inside an explicit transaction */
  sig32 waiting;    // lock waiting time
  unsigned fixnum;  // number of fixes owned by the client
#ifdef FIX_CHECKS
  t_fix_info fix_info[MAX_APPL_FIXES];
#endif
  worm jourworm;
  WBUUID luo;       // "last update origin" value stored in updated records

 // transactions:
  t_translog_main tl;         // the main transaction log (gets TL_CLIENT in its ctor)
  t_translog * subtrans;  // list of subtransaction logs (does not contain the main log)
  cur_descr * current_cursor;  // allows access to the translog of the cursor

  privils  prvs; // privileges, caches, connected user object number etc.
  BOOL has_full_access(const WBUUID schema_uuid) const;
  ins_curs_dynar d_ins_curs;
  volatile t_appl_state appl_state; /* the request state, NOT_LOGGED iff not connected */
  uns8     wait_type;   // t_wait value
  wbbool   mask_token;  // true if write allowed without the token
 // waiting for a lock:
  t_client_number waits_for;   // number of client if waits for, 0 iff none
   // new semantics: 0 when not waiting, 1 when lock request stored, 2 when lock request stored and satisfied, 3 on mem. error, 4 on cancel, 5 on timeout
  table_descr * lock_tbdf;     // valid when waits_for!=0, NULL for page lock
  trecnum  record_or_page;     // valid when waits_for!=0
  uns8     lock_type;          // valid when waits_for!=0
  DWORD    time_limit;         // 0 == no limit 
 // waiting for a disc block number:
  t_dws    dws;
 // description of the connection between client and server:
#ifdef WINS
	HANDLE   hSlaveSem;
	DWORD		 LinkIdNumber;  /* used only by the direct client on Break */
  int      nextIndex;   /* establishes linked list of processes */
#else
  Semaph   hSlaveSem;			/* signaled by receiver from PacketIdentification */
#endif
  t_commenc * commenc;    // communication encryptor

 // temporary slave variables:
  tobjnum     cond_login;    // used during op_login
  wbbool      check_rights;  // on-line checking read rights, may be 0,1,2
  uns32       clientVersion; // received when connecting, check in login

 // slave variables initialized for each request:
  t_answer answer;
  inline BOOL is_an_error    (void) const { return answer.ans->return_code >= 0x80; }
  inline int  get_return_code(void) const { return answer.ans->return_code; }
  inline void set_return_code(int code)   {        answer.ans->return_code=code; }
  inline tptr get_ans_ptr(unsigned size)  
    { tptr ans = answer.get_ans_ptr(size); 
      if (!ans) request_error(this, OUT_OF_KERNEL_MEMORY);
      return ans;
    }

  wbbool      roll_back_error;
  tptr        comp_kontext;
  uns32       comp_err_line;
  uns16       comp_err_column;
  uns16       mask_compile_error;

 // long-term schema context:
  tobjname    sel_appl_name;  // top application name, not redirected to front nor back end
  WBUUID      top_appl_uuid;  // top application uuid, not redirected
  WBUUID      sel_appl_uuid;  // back-end application uuid
  WBUUID      front_end_uuid; // front-end application uuid
  tobjnum     admin_role;     // back-end roles used only when creating table from SQL
  tobjnum     senior_role;
  tobjnum     junior_role;
  tobjnum     author_role;
 // short-term schema context:
  const uns8 * current_schema_uuid; // not-owning pointer
  MsgInfo     msg;          // incomming message (request, answer)
  t_thread    threadID;     // thread or task ID
#if defined(WINS) && defined(_DEBUG) && !defined(X64ARCH) // handle for stackwalk
  HANDLE      thread_handle;
#endif
  cAddress *  pRemAddr;     // connection address
  uns32       auxinfo; // data import flag
  WBUUID      dest_srvr_id; // destination server ID when evaluating replication condition
  tobjname    errmsg;  // name in error message

  trecnum     kont_recnum;       // used when evaluating expressions compiled in the kont_ta kontext - position in the table
  const table_descr * kont_tbdf; // used when evaluating expressions compiled in the kont_ta kontext - referenced table
 // counting statements and in-statement-constant values:
  uns32       statement_counter, date_time_statement_cnt_val;
  uns32       curr_date, curr_time;
  t_seq_value last_identity_value;
  t_rscope *  rscope;
  t_except    except_type;
  t_idname    except_name;
  t_idname    processed_except_name;
  t_idname    last_exception_raised;
  t_rscope *  handler_rscope;   // when execuing a handler, the scope containing the handler definition, NULL otherwise
  inline void stop_leaving_for_name(const char * stop_name)
  { if (except_type==EXC_LEAVE)
      if (!strncmp(except_name, stop_name, sizeof(except_name)-1)) // for routines stop_name is longer than except_name 
        { except_type=EXC_NO;  except_name[0]=0; }
  }
  inline void clear_exception(void)
  { except_name[0] = 0;
    except_type = EXC_NO;
    set_return_code(0);
  }

  t_savepoints savepoints;
  t_isolation isolation_level;
  BOOL        trans_rw;     // READ WRITE access mode
  uns32       sqloptions;   // bitmap of sql compatibility options
  uns64       stmt_counter; // counter of some statements, used to validate subquery cache contents
  inline void exec_start(void) { stmt_counter++; }

  // array for row counts and result sets handles for result generating statements
 private:
  uns32       sql_results[MAX_SQL_RESULTS];  // internal array of results
 public:
  unsigned    sql_result_cnt;
  void        add_sql_result(uns32 sql_result)
    { if (sql_result_cnt<MAX_SQL_RESULTS) sql_results[sql_result_cnt++]=sql_result; }
  void        clear_sql_results(int cnt = 0)
    { sql_result_cnt=cnt; }
  unsigned    sql_result_count(void)
    { return sql_result_cnt; }
  const uns32 * sql_results_ptr(void)
    { return sql_results; }

  uns32       tb_read_var_result;  // tb_read_var and tb_read_ind_var store the number of bytes read here
  int         in_trigger;
  int         in_routine;
  int         in_admin_mode;
  int         open_cursors;
  db_kontext * last_active_trigger_tabkont;  // tabkont of the most recent trigger, if it has any kontext
  unsigned    report_modulus;
  t_seq_list * seq_list;
  t_exkont * execution_kontext;
  uns16       execution_kontext_locked;
  bool        procedure_created_cursor;
  bool        in_expr_eval;
 // system variables:
  uns32       __rowcount;
  uns32       __fulltext_weight;
  unsigned    __fulltext_position;
 // statement kontexts:
 private:
  wbstmt * stmts;
 public:
  wbstmt * sel_stmt;
  void free_all_stmts(void);
  void drop_statement(uns32 handle);
  wbstmt * get_statement(uns32 handle)  // finds statement kontext by handle
  { wbstmt * stmt = stmts;
    while (stmt!=NULL && stmt->hstmt!=handle) stmt=stmt->next_stmt;
    return stmt;  // returns statement kontext or NULL iff handle not found
  }
  uns32 get_new_stmt_handle(void);
  
  uns32       random_login_key;
  int         dep_login_of;
  t_dbginfo   * dbginfo;
  bool        is_detached;
  t_routine * detached_routine;  // valid only when [is_detached] is true
  bool        conditional_attachment;  // used for PT_SLAVE and PT_KERDIR, says that client is not logged and does not have the licence
  bool        www_client;     // logged as __WEB
  bool        limited_login;  // defined when conditional_attachment goes to false, not changed on relogin
  uns32       last_request_time;
  unsigned    trace_enabled;
  BOOL        is_trace_enabled(unsigned trace_type) // checks if the situation is registered for any tracing
    { return (trace_enabled & trace_type) != 0; }
  void        *MailCtx;
  t_exkont_info * exkont_info;
  unsigned exkont_len;
  char * current_generic_error_message; 
  
 // global scopes:
  t_global_scope * global_sscope;  // selected static global scope from the [installed_global_scopes] list, not owning
  t_rscope * created_global_rscopes, // list of dynamic global scopes for the client, owning
           * global_rscope;        // selected dynamic global scope from the above list, not owning
  void drop_global_scopes(void);

  event_management cevs;
  int        tzd;  // time zone displacement in seconds
  bool       weak_link;
  uns8       in_active_ri;
  char * temp_param;  // not owning
  bool * replication_change_detector;  // not owning
  table_descr * exclusively_accessed_table;
  uns8       my_server_locks;
 // debugging and profiling info:
  char       thread_name[1+31+31+1];  // long name because is is used by the internal timer of the server
  bool       profiling_thread;
  prof_time  lower_level_time;  // time spent in objects called from the current object
  tobjnum    top_objnum;   // objnum of the executed code (moved from t_dbginfo)
  t_stack_item * stk;      // the head item contains current objnum and line (line updated on every break check)
  t_linechngstop last_linechngstop;
  tobjnum    index_owner;  // defined inside btree index functions, table number owning the index
  prof_time  time_since_last_line;  // time when the execution of [last_line_num] in [top_objnum] started
  uns32      last_line_num;         // the last line being executed, valid even if not debugging nor profiling (used in error context)
  prof_time  lock_waiting_time;     // time spent in waiting for locks in the current request
  uns32 lock_cycles, ilock_cycles, dlock_cycles, ft_cycles;  // statistics
  uns32 cnt_requests, cnt_pagewr, cnt_pagerd, cnt_cursors, cnt_sqlstmts;

  void stk_drop(void);
  void stk_add(tobjnum objnumIn, char * name);

  void cd_init(void);
  cd_t(t_cdp_type cdp_type) : in_use(cdp_type), tl(this), prvs(this)
    { cd_init(); }
  void mark_core(void);

  virtual void send_status_nums(trecnum rec1, trecnum rec2) { }   // does nothing (used by PT_WORKER, PT_SERVER)
  virtual void SendServerDownWarning(void) { }
  virtual void SendClientMessage(const char * msg)  { }
  virtual sig32 read_from_client(char * buf, unsigned uns32) { return -1; }          // returns reading error
  virtual bool  write_to_client(const void * buf, unsigned size) { return false; }  // returns writing error
  virtual ~cd_t(void) { }  // just to prevent warning
  void request_init(void)  // inits processing of a new request, from client or internal
  { answer.ans_init();
    break_request=wbfalse;
    roll_back_error=wbfalse;
    except_type=EXC_NO;  except_name[0]=0;  processed_except_name[0]=0;  // executing SQL statements during creating/expanding cursor and other operations need this
#ifdef FIX_CHECKS
    fix_info[0].fcbn=NOFCBNUM;
#endif
  }
  bool nonempty_transaction(void);
};

// Base class [cd_t] is used for threads without connection to the client: the system thread, 
//                                    worker threads, detached threads, comm log replay thread.
// Class [cddir_t] is used for threads with a direct local connection to the client.
// Class [cdnet_t] is used for threads with a network connection to the client.

#ifdef WINS
struct cddir_t : public cd_t  // slave with a direct communication with a client
{ cddir_t(void) : cd_t(PT_MAPSLAVE)
    { }
  t_dircomm dcm;
  virtual void  send_status_nums(trecnum rec1, trecnum rec2)
    { dcm.send_status_nums(rec1, rec2); }   // may be replaced by send_notification
  virtual void  SendServerDownWarning(void)
    { /*dcm.send_notification(NOTIF_SERVERDOWN, NULL, 0);*/ }  // interferes with the normal communication of the direct client
  virtual void  SendClientMessage(const char * msg)
    { /*dcm.send_notification(NOTIF_MESSAGE, msg, strlen(msg)+1);*/ }  // interferes with the normal communication of the direct client
  virtual sig32 read_from_client(char * buf, uns32 size)
    { return dcm.read_from_client(buf, size); }
  virtual bool  write_to_client(const void * buf, unsigned size)
    { return dcm.write_to_client(buf, size); }
  
};
#endif

struct cdnet_t : public cd_t  // slave with a network communication with a client
{ cdnet_t(void) : cd_t(PT_SLAVE)
    { }
  virtual void  send_status_nums(trecnum rec1, trecnum rec2);
  virtual void  SendServerDownWarning(void);
  virtual void  SendClientMessage(const char * msg);
  virtual sig32 read_from_client(char * buf, unsigned uns32);
  virtual bool  write_to_client(const void * buf, unsigned size);
 // import/export data variables:
  char *  pImpExpBuff;           // not owning!
  uns32   ImpExpDataSize;
  bool    data_import_completed;
};

struct cdhttp_t : public cd_t  // slave with a network communication in a HTTP tunnel with a client
{ cdhttp_t(void) : cd_t(PT_SLAVE)
    { }
  virtual void  send_status_nums(trecnum rec1, trecnum rec2);
  virtual void  SendServerDownWarning(void);
  virtual void  SendClientMessage(const char * msg);
  virtual sig32 read_from_client(char * buf, unsigned uns32);
  virtual bool  write_to_client(const void * buf, unsigned size);
 // import/export data variables:
  char *  pImpExpBuff;           // not owning!
  uns32   ImpExpDataSize;
  bool    data_import_completed;
};

class ProtectedByCriticalSection
// Critical section entering/leaving implemented as a class construction/destruction
{ CRITICAL_SECTION * const cs; // the critical section entered by constructing the class
  cdp_t const cdp;             // cdp of the thread entering the CS
  uns8  outer_wait_type;
 public:
  inline ProtectedByCriticalSection(CRITICAL_SECTION * csIn) 
    : cs(csIn), cdp(NULL)
    { EnterCriticalSection(cs); }
  ProtectedByCriticalSection(CRITICAL_SECTION * csIn, cdp_t cdpIn, t_wait cs_id);
  inline ~ProtectedByCriticalSection(void)
  { LeaveCriticalSection(cs); 
    if (cdp) cdp->wait_type = outer_wait_type;
  }
};

class t_temporary_current_cursor
// Constructor sets a new current_cursor in the cdp, destructor returns the original one.
{ cdp_t cdp;
  cur_descr * upper_current_cursor;
 public:
  t_temporary_current_cursor(cdp_t cdpIn, cur_descr * cdIn) : cdp(cdpIn)
    { upper_current_cursor = cdp->current_cursor;  cdp->current_cursor = cdIn; }
  ~t_temporary_current_cursor(void)
    { cdp->current_cursor = upper_current_cursor; }
};

#endif  /* ! CDP_INCLUDED */

