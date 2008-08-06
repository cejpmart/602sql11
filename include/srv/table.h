/****************************************************************************/
/* Table.h - table operations                                               */
/****************************************************************************/
/* the table definition should be locked as a record in the major table */
/* the tabla basic block may be locked as any other page, the locks are
   expected to be short-term */
/* data contained in the table may be locked in whole or per records
   via lockbase */

struct bas_tbl_block     // beginning of the basic table block on disc
{ wbbool  diraddr;      /* direct data blocks numbers in the basic block */
  wbbool  unused;
  uns16   nbnums;       /* number of block numbers here */
  trecnum recnum;       /* number of records in the table */
  tblocknum free_rec_list;  // head of the linked list of blocks containing free record numbers, -1 if list owned by the cache, never used in the memory copy of the basic block
  uns32 unique_counter; // unique key counter for this table, from 8.0 never used in the memory copy of the basic block
  tblocknum indroot[INDX_MAX]; /* array of index roots */
  tblocknum databl[1];         /* array of direct or indirect data blocks */
  void init(void);
};

struct bas_tbl_block_info  // memory copy of some info from the bas_tbl_block
{ long      ref_cnt;         // reference counter (must be aligned!)
  bool      diraddr;         // direct data blocks numbers in databl[] iff true
  unsigned  nbnums;          // number of block numbers in databl[]
  trecnum   recnum;          // number of records in the table 
  tblocknum databl[1];       // array of direct or indirect numbers of data blocks
  inline unsigned AddRef(void)  { return InterlockedIncrement(&ref_cnt); }
  inline unsigned Release(void) 
  { long res = InterlockedDecrement(&ref_cnt);
    if (res<=0) { corefree(this);  return 0; }  
    return res; 
  }
};

#define MAX_BAS_ADDRS ((BLOCKSIZE-sizeof(bas_tbl_block)+sizeof(tblocknum))/sizeof(tblocknum) )
#define MAX_BLOCK_COUNT_PER_TABLE (MAX_BAS_ADDRS * (BLOCKSIZE/sizeof(tblocknum)))

struct dd_index // version without constructor and destructor
{ tobjname constr_name;
  t_atrmap_flex atrmap;
  uns8 ccateg;
  bool has_nulls;
  bool deferred;
  bool disabled;
  unsigned partnum;
  unsigned keysize;
  part_desc * parts;      // array of part descriptors
  ttablenum * reftabnums; // numbers of referencig tables
  tblocknum root;
  inline void constr_index(void)
    { parts=NULL;  partnum=0;  reftabnums=NULL;  root=0;  disabled=false; }
  void destr_index(void);
  void compile(CIp_t CI);
  BOOL add_index_part(void);
  void mark_core(void);
};

#define DISTINCT(ind) ((ind)->ccateg <= INDEX_UNIQUE)  // PRIMARY or UNIQUE

struct dd_index_sep : dd_index // version with constructor and destructor
{ inline dd_index_sep(void)
   { constr_index(); }
  inline ~dd_index_sep(void)
   { destr_index(); }
  bool same(const dd_index & oth, bool comp_exprs) const;
};

struct dd_check
{ tobjname constr_name;
  t_atrmap_flex atrmap;
  bool deferred;
  t_express * expr;        /* expression code */
  inline dd_check(void) { expr=NULL; }   // used by new []
  inline dd_check(unsigned column_count) : atrmap(column_count) { expr=NULL; }
  ~dd_check(void);
  void mark_core(void);
};

struct dd_forkey
// Must change mark() to mark_array() in table_descr::mark_core() if destructor is added!
{ tobjname constr_name;
  t_atrmap_flex atrmap;
  bool deferred;
  ttablenum desttabnum;
  uns16 loc_index_num;
  sig16 par_index_num;  // -1 if parent index not found
  uns8  update_rule;
  uns8  delete_rule;
  inline dd_forkey(void) { }   // used by new []
  inline dd_forkey(unsigned column_count) : atrmap(column_count) { }
  void mark_core(void);
};

dd_index_sep * new_dd_index(unsigned count, unsigned columns);
dd_check * new_dd_check(unsigned count, unsigned columns);
dd_forkey * new_dd_forkey(unsigned count, unsigned columns);

typedef struct    /* definition of an attribute */
{ char  name[ATTRNAMELEN+1]; /* the name of the attribute */
  uns8  attrtype;            /* type - see attribute type codes */
  uns8  attrmult;            /* number of values contained in the basic record */
                             /* 0x80 bit: further values follow on heap */
  wbbool nullable;
  t_specif attrspecif;       /* type specific information about the attribute */
  uns16 attroffset;       /* attribute offset in its record page  */
  uns16 attrpage;         /* record page containing the attribute */
  uns8  attrspec2;        /* IS_TRACED etc. */
  tattrib signat;         /* number of the following signature attribute or NOATTRIB */
  t_express * defvalexpr; /* pointer to default value if not NULL */
               } attribdef;

struct t_trigger_type_and_num // element of the list of triggers defined for a table
{ tobjnum trigger_objnum;  // object number of the trigger
  int trigger_type;        // type of the trigger
};

SPECIF_DYNAR(trig_type_and_num_dynar, t_trigger_type_and_num, 2);

void delete_free_record_list(cdp_t cdp, tblocknum head);

class t_free_rec_pers_cache
// Table must not be installed if the cache is not created by make_cache.
{ unsigned cnt_records_in_cache;  // number of records in the cache
  unsigned cache_size;
  trecnum * reccache;   // free records, ordered from max to min when free_rec_list_head==0
  tblocknum free_rec_list_head;   // -1 if not owning the list, 0 if list is empty, the list head otherwise
 public:
  enum { default_cache_size = 150 };
  struct t_recnum_containter
  { tblocknum next;
    unsigned contains;
    static inline unsigned recmax(void)  // number of records stored in a single disc block in the recs[] array
      { return (BLOCKSIZE-sizeof(tblocknum)-sizeof(unsigned)) / sizeof(trecnum); }
    bool valid(void) const;
    trecnum recs[1]; // in fact [recmax] records are contained in the array
  };
  void return_free_record_list(cdp_t cdp, table_descr * tbdf);
  inline bool owning_free_record_list(void) const
    { return free_rec_list_head!=(tblocknum)-1; }
  bool disc_container_is_empty(void) const
    { return free_rec_list_head==0; }
 private:
  inline bool cache_empty(void) const
    { return cnt_records_in_cache==0; }
  bool take_free_record_list(cdp_t cdp, table_descr * td, bool uninstalling_table);  // takes from basblock or creates
  void inflate_cache(cdp_t cdp, table_descr * tbdf, unsigned cache_limit);
  void sort_cache(void);

 public:
  bool store_to_cache(cdp_t cdp, trecnum rec, table_descr * tbdf); // returns false on error. Called from table's CS
  bool store_to_cache_ordered(cdp_t cdp, trecnum rec, table_descr * tbdf); // returns false on error. Called from table's CS
  void deflate_cache(cdp_t cdp, table_descr * tbdf, bool uninstalling_table);
  inline bool make_free_record_list_accessible(cdp_t cdp, table_descr * tbdf)
  { 
    if (owning_free_record_list()) return true;
    return take_free_record_list(cdp, tbdf, false);
  }
  unsigned inflate_step(void) const
    { return cache_size / 6 * 5; }
  t_free_rec_pers_cache(void)
  { free_rec_list_head=(tblocknum)-1;  
    reccache=NULL;  cache_size=0;  cnt_records_in_cache=0; 
  }
  ~t_free_rec_pers_cache(void)
  { corefree(reccache); }
  inline void mark_core(void)
  { if (reccache) mark(reccache); }
  bool make_cache(unsigned size);
  void close_caching(cdp_t cdp, table_descr * tbdf);
  inline void drop_caches(void)
    { free_rec_list_head=(tblocknum)-1;  cnt_records_in_cache=0; }
  trecnum get_from_cache(cdp_t cdp, table_descr * tbdf);  // returns NORECNUM iff cannot allocate, called from table's CS
  trecnum extract_for_append(cdp_t cdp, table_descr * tbdf);
  int  mark_free_rec_list(cdp_t cdp, tblocknum basblockn) const;
  BOOL translate_free_rec_list(cdp_t cdp, trecnum & free_rec_list);
  bool list_records(cdp_t cdp, table_descr * tbdf, uns32 * map, trecnum * count);
  void drop_free_rec_list(cdp_t cdp)
  { if (free_rec_list_head!=-1)
      delete_free_record_list(cdp, free_rec_list_head);
    free_rec_list_head=0;
  }
  inline bool cache_is_empty(void) const
    { return cnt_records_in_cache==0; }
  inline unsigned space_in_cache(void) const
    { return cache_size - cnt_records_in_cache; }
};

struct t_ftx_info
{ tattrib text_col;
  tattrib id_col;
  tobjnum ftx_objnum;  // the NOOBJECT value terminates the list
  int item_index;
};

extern int global_fulltext_info_generation;
extern WBUUID sysext_schema_uuid;

struct t_scope;

#define RECORD_COUNT2BLOCK_COUNT(record_count, multpage, rectopage) (!(record_count) ? 0 :\
  (multpage) ? (rectopage)*(record_count) : ((record_count)-1)/(rectopage) + 1)

struct table_descr    /* table core descriptor */
{ friend BOOL init_tables_1(cdp_t cdp);
  friend table_descr * reinstall_table(cdp_t cdp, ttablenum tb);
  friend void mark_basrecs(cdp_t cdp, table_descr * tbdf);

  long       deflock_counter;   // must be on an aligned address, used in InterlockedDecrement
  t_locktable lockbase;
  tobjnum    tbnum;
  BOOL       is_traced;
 private:
  bas_tbl_block_info * volatile basblock;  // pointer to the memory copy of the basic block of the table
 public:
  inline bas_tbl_block_info * get_basblock(void)
  { ProtectedByCriticalSection cs(&lockbase.cs_locks);
    do
    { bas_tbl_block_info * basblock_ptr = basblock;
      basblock_ptr->AddRef();  
      if (basblock_ptr->ref_cnt > 1) return basblock_ptr;  // must not check the result of AddRef, not valid on W95!
    } while (true);
  }
  inline unsigned record_count2block_count(unsigned record_count) const
  // Computes allocated block number form the number of allocated records in [record_count].
    { return RECORD_COUNT2BLOCK_COUNT(record_count, multpage, rectopage); }

  tblocknum  basblockn;      // the block number of the basic block of the table 

 // information about triggers defined for the table:
  t_ftx_info * fulltext_info_list;
  int   fulltext_info_generation;
  void load_list_of_fulltexts(cdp_t cdp);
  bool get_next_table_ft(int & pos, const t_atrmap_flex & atrmap, t_ftx_info & info);
  bool get_next_table_ft2(int & pos, const t_atrmap_flex & atrmap, t_ftx_info & info);
  BOOL  trigger_info_valid; // TRUE iff trigger_map and triggers contain valid information
  uns16 trigger_map;    // map of triggers of the table
  trig_type_and_num_dynar triggers;     // list of triggers of the table
  void load_list_od_triggers(cdp_t cdp);
  inline void invalidate_trigger_info(void)
    { trigger_info_valid=FALSE; }
  inline void prepare_trigger_info(cdp_t cdp)  // called often, enterning CS only when necessary
  { if (!trigger_info_valid)  // another test is inside the CS
    { ProtectedByCriticalSection cs(&lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
      load_list_od_triggers(cdp); 
    }  
    if (fulltext_info_generation<global_fulltext_info_generation)  // another test is inside the CS
    { ProtectedByCriticalSection cs(&lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
      load_list_of_fulltexts(cdp);  
    }
  }
  t_shared_scope * trigger_scope;  // scope used by running triggers to work with their rscopes
  t_translog * translog;    // own transaction log or NULL, not owning
  bool independent_alpha(void) const
    { return translog==NULL; }
  tblocknum alloc_alpha_block(cdp_t cdp);

  int level;  // structure completion level
  BOOL       released;       // all contents released, but not released itself
  t_free_rec_pers_cache free_rec_cache;
  class t_unique_value_cache
  /* Caches the values of the unique counter associated with the table. 
     Allocates chunks of values from the basic block.
  */
  { table_descr * const containing_tbdf;
    enum { NUMBER_OF_ALLOCATED_VALUES_PER_CHUNK=100 };
    uns32    next_available_value;  // next UNIQUE value to be used, valid iff [cached_values]>0
    unsigned cached_values;         // number of consecutive values (staring with [next_available_value]) stored in the cache
    void     load_values_to_cache(cdp_t cdp);
   public:
    inline t_unique_value_cache(table_descr * const containing_tbdfIn) : containing_tbdf(containing_tbdfIn)
      { cached_values=0; }
    inline uns32 get_next_available_value(cdp_t cdp)
    { if (!cached_values)
      { load_values_to_cache(cdp);
        if (!cached_values) return NONEINTEGER;  // error reported inside
      }
      cached_values--;
      return (uns32)(cdp->last_identity_value = next_available_value++);
    }
    void register_used_value(cdp_t cdp, uns32 value);
    void reset_counter(cdp_t cdp);
    void close_cache(cdp_t cdp);
  };
  t_unique_value_cache unique_value_cache;
 // information about record sizes, constant:
  unsigned recordsize;
  wbbool  multpage;          /* record bigger than a page */
  uns16 rectopage;           /* number of records in a page or vice versa */

  int indx_cnt;              // number of indicies 
  dd_index_sep * indxs;      // array of indicies
  int check_cnt;             // number of checks
  dd_check * checks;         // array of checks
  int forkey_cnt;            // number of foreign keys
  dd_forkey * forkeys;       // array of foreign keys
  tattrib * notnulls;        // list of columns with the NOT NULL flag, terminated by 0 

  tattrib    attrcnt;        /* the total count of attributes */
  uns16     tabdef_flags;    /* flags derived from the definition */
 // information about special columns:
  tattrib   rec_privils_atr; // column containing record priviledges or NOATTRIB
  tattrib   zcr_attr;
  tattrib   luo_attr;
  tattrib   unikey_attr;
  tattrib   token_attr;
  tattrib   detect_attr;
  bool _has_variable_allocation;
  inline bool has_variable_allocation(void) const
    { return _has_variable_allocation; }
  BOOL referencing_tables_listed;
  cdp_t     owning_cdp;  // !=NULL only for non-permanent tables

  tobjname  selfname;
  tobjname  schemaname;  // usually not defined, empty
  WBUUID    schema_uuid;
  attribdef attrs[1];   /* definitions of table attributes */
 // methods:
  void * operator new(size_t size, unsigned column_cnt, unsigned name_cnt);
#ifdef WINS
  void operator delete(void * ptr, unsigned attrcnt, unsigned namecnt)
    { corefree(ptr); }
#endif
  void operator delete(void * ptr)
    { corefree(ptr); }
  table_descr(void);
  void add_column(const char * nameIn, uns8 typeIn, uns8 multiIn, wbbool nullableIn, t_specif specifIn);
  void free_descr(cdp_t cdp); // like destructor
  void destroy_descr(cdp_t cdp);
  void release_properties(void);  // destruction of definiton-related objects
  BOOL prepare(cdp_t cdp, tblocknum basblocknIn, t_translog * translog);
  BOOL update_basblock_copy(const bas_tbl_block * basbl);
  inline trecnum Recnum(void) const
    { return basblock->recnum; }
  void mark_core(void);  // marks itself, too
  inline table_descr * me(void) { return this; }
  BOOL expand_table_blocks(cdp_t cdp, unsigned new_record_space);
  BOOL reduce_table_blocks(cdp_t cdp, unsigned new_record_space);
  tblocknum alloc_and_format_data_block(cdp_t cdp, unsigned blocknum);
  inline unsigned privil_descr_size(void) const
    { return SIZE_OF_PRIVIL_DESCR(attrcnt); }
};

class t_table_record_rdaccess
// supports fast sequential passing of table records
{ cdp_t cdp;
  table_descr * tbdf;
  tfcbnum datafcbn;
  tblocknum current_datadadr;  
  const char * current_datablock;
 public:
  t_table_record_rdaccess(cdp_t cdpIn, table_descr * tbdfIn);
  ~t_table_record_rdaccess(void);
  const char * position_on_record(trecnum rec, unsigned page);
};

class t_page_rdaccess
{ cdp_t cdp;
  tfcbnum fcbn;
 public:
  t_page_rdaccess(cdp_t cdpIn) : cdp(cdpIn)
    { fcbn=NOFCBNUM; }
  ~t_page_rdaccess(void)
    { unfix_page(cdp, fcbn); }
  const char * fix(tblocknum dadr);
};

inline t_translog * table_translog(cdp_t cdp, const table_descr * tbdf)
{ if (tbdf->translog) return tbdf->translog;
  return &cdp->tl;
}

void free_alpha_block(cdp_t cdp, tblocknum dadr, t_translog * translog);
uns32 get_typed_unique_value(void * valptr, int type);

class t_table_record_wraccess
// supports fast sequential writing to table records
{ cdp_t cdp;
  table_descr * tbdf;
  tfcbnum datafcbn;
  tblocknum current_datadadr;  
  char * current_datablock;
  unsigned pgrec;  // pgrec of the positioned record
 public:
  t_table_record_wraccess(cdp_t cdpIn, table_descr * tbdfIn);
  ~t_table_record_wraccess(void);
  char * position_on_record(trecnum rec, unsigned page);
  void unposition(void);
  inline void positioned_record_changed(void)
    { table_translog(cdp, tbdf)->log.rec_changed(datafcbn, current_datadadr, tbdf->recordsize, pgrec); }
  inline unsigned _pgrec(void) const
    { return pgrec; }
  inline tfcbnum _fcbn(void) const
    { return datafcbn; }
};

int WINAPI typesize(const attribdef * pdatr);

extern table_descr **tables; /* ptr to array of ptrs to table descriptors */
extern table_descr * tabtab_descr, * objtab_descr, * usertab_descr;
extern CRITICAL_SECTION cs_tables;
extern LocEvent hTableDescriptorProgressEvent;

struct t_vector_descr;
class t_tab_trigger;
trecnum tb_new(cdp_t cdp, table_descr * tbdf, BOOL app, const t_vector_descr * recval, t_tab_trigger * ttr = NULL);
//BOOL initial_rights(cdp_t cdp, ttablenum tb, trecnum therec, BOOL newuser);
#define TABLE_DESCR_LEVEL_COLUMNS_INDS 1
#define MAX_TABLE_DESCR_LEVEL 2
table_descr * install_table(cdp_t cdp, ttablenum tb, int level = MAX_TABLE_DESCR_LEVEL);
table_descr * reinstall_table(cdp_t cdp, ttablenum tb);
void WINAPI unlock_tabdescr(ttablenum tb);
void WINAPI unlock_tabdescr(table_descr * tbdf);
void try_uninst_table(cdp_t cdp, ttablenum tb);
void force_uninst_table(cdp_t cdp, ttablenum tb);
void uninst_tables(cdp_t cdp);
BOOL init_tables_1(cdp_t cdp);
BOOL init_tables_2(cdp_t cdp);
void deinit_tables(cdp_t cdp);
tblocknum create_table_data(cdp_t cdp, unsigned index_count, t_translog * translog);
BOOL prepare_list_of_referencing_tables(cdp_t cdp, table_descr * td);
BOOL fast_table_read(cdp_t cdp, table_descr * tbdf, trecnum rec, tattrib attrib, void * buf);
void enable_index(cdp_t cdp, table_descr * tbdf, int which, bool enable);
BOOL free_deleted(cdp_t cdp, table_descr * tbdf);
BOOL compact_table(cdp_t cdp, table_descr * tbdf);
void change_indicies(cdp_t cdp, table_descr * tbdf, const table_all * ta, int new_index_cnt, const ind_descr * new_indxs);
bool transform_indexes(cdp_t cdp, table_descr * tbdf_old, table_descr * tbdf_new, table_all * ta_old, table_all * ta_new);
void copy_var_len(cdp_t cdp, table_descr * tbdf1, trecnum r1, tattrib a1, uns16 ind1,
                             table_descr * tbdf2, trecnum r2, tattrib a2, uns16 ind2,
                             elem_descr * attracc);
void table_info(cdp_t cdp, table_descr * tbdf, trecnum * counts);
void delete_history(cdp_t cdp, table_descr * tbdf, BOOL datim_type, timestamp dtm);
void table_rel(cdp_t cdp, ttablenum tb);
BOOL link_table_in(cdp_t cdp, table_descr * tbdf, ttablenum tbnum);
void destroy_table_data(cdp_t cdp, table_descr * tbdf);
void set_null(tptr adr, const attribdef * att);
BOOL drop_basic_table_allocation(cdp_t cdp, tblocknum basblockn, wbbool multpage, unsigned rectopage, unsigned new_record_space, t_translog * translog);
table_descr * access_installed_table(cdp_t cdp, ttablenum tb);

bool compile_domain_to_type(cdp_t cdp, tobjnum objnum, t_type_specif_info & tsi);

#define DADRS_PER_BLOCK (BLOCKSIZE / sizeof(tblocknum))
void os_load_table(cdp_t cdp, tcurstab tb);
void os_save_table(cdp_t cdp, ttablenum tb, trecnum recnum);
void os_load_user(cdp_t cdp);

BOOL multi_in(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, void * arg);

void destroy_temporary_table(cdp_t cdp, ttablenum tabnum);
ttablenum create_temporary_table(cdp_t cdp, table_descr * tbdf, t_translog * translog);
tptr fix_attr_x(cdp_t cdp, table_descr * tbdf, trecnum recnum, tfcbnum * pfcbn);
void register_unregister_trigger(cdp_t cdp, tobjnum objnum, BOOL registering);
bool truncate_table(cdp_t cdp, table_descr * tbdf);
void fulltext_index_column(cdp_t cdp, t_ft_kontext * ftk, table_descr * tbdf, trecnum recnum, tattrib text_col, tattrib id_col, const char * format, int mode);
tattrib find_attrib(const table_descr * td, const char * name);

/***************************************************************************/
const char * fix_attr_read    (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tfcbnum * pfcbn);
tptr         fix_attr_write    (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tfcbnum * pfcbn);
const char * fix_attr_ind_read(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, tfcbnum * pfcbn);
tptr         fix_attr_ind_write(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, tfcbnum * pfcbn);
uns8 table_record_state(cdp_t cdp, table_descr * tbdf, trecnum recnum);
uns8 table_record_state2(cdp_t cdp, table_descr * tbdf, trecnum recnum);
const char * fix_attr_read_naked(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tfcbnum * pfcbn);

#define IS_TRACED        1
#define IS_IN_INDEX      2
#define IS_IN_FULLTEXT   4

/* Pred vstupem do tb_ je overeno cislo atributu, neni overen typ ani multi.
   Prilis se neoveruje recnum, bylo by lepsi, kdyby to nebylo potreba.
   Tabulka je instalovana. */
/* on entry: atr is checked to exist, recnum is not(?) */
BOOL tb_del   (cdp_t cdp, table_descr * tbdf, trecnum recnum);
BOOL tb_undel (cdp_t cdp, table_descr * tbdf, trecnum recnum);

BOOL tb_rlock  (cdp_t cdp, table_descr * tbdf, trecnum recnum);
BOOL tb_wlock  (cdp_t cdp, table_descr * tbdf, trecnum recnum);
BOOL tb_unrlock(cdp_t cdp, table_descr * tbdf, trecnum recnum);
BOOL tb_unwlock(cdp_t cdp, table_descr * tbdf, trecnum recnum);

BOOL tb_read_atr (cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attrib, tptr buf_or_null);

BOOL tb_read_atr     (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib,                                       tptr buf_or_null);
BOOL tb_write_atr    (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib,                                       const void * ptr);
BOOL tb_read_ind     (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index,                          tptr buf_or_null);
BOOL tb_write_ind    (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index,                          const void * ptr);
BOOL tb_read_var     (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib,             uns32 start, uns32 size, tptr buf_or_null);
BOOL tb_write_var    (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib,             uns32 start, uns32 size, const void * ptr);
BOOL tb_read_ind_var (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 start, uns32 size, tptr buf_or_null);
BOOL tb_write_ind_var(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 start, uns32 size, const void * ptr);
BOOL tb_read_atr_len (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib,             uns32 * len);
BOOL tb_write_atr_len(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib,             uns32 len);
BOOL tb_read_ind_len (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 * len);
BOOL tb_write_ind_len(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 len);
BOOL tb_read_ind_num (cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size * num);
BOOL tb_write_ind_num(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size num);

BOOL tb_add_value(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, char ** val);
BOOL tb_del_value(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tptr * val);
BOOL tb_rem_inval(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib);

// low-level extlob
FHANDLE open_extlob(uns64 lob_id);
void close_extlob(FHANDLE hnd);
uns32 read_extlob_part(FHANDLE hnd, uns32 start, uns32 length, void * buf);
uns32 read_extlob_length(FHANDLE hnd);

// upper-level extlob
void read_ext_lob(cdp_t cdp, uns64 lob_id, uns32 offset, uns32 size, char * buf_or_null);
bool write_ext_lob(cdp_t cdp, uns64 * lob_id, uns32 offset, uns32 size, const char * data, bool setsize, bool * dbdata_changed);
bool ext_lob_free(cdp_t cdp, uns64 lob_id);
bool ext_lob_set_length(cdp_t cdp, uns64 lob_id, uns32 len);
uns32 read_ext_lob_length(cdp_t cdp, uns64 lob_id);
void lob_id2pathname(uns64 lob_id, char * pathname, bool new_id);
const char * extlob_directory(void);

class t_btree_read_lock
// The class holds the read lock on the root node of a btree.
// Updates the index when locking - unlocks when destructed.
{ cdp_t cdp;       // owner of the lock, if [locked]
  tblocknum root;  // root of the btree, if [locked]
  bool locked;
 public:
  bool lock_and_update(cdp_t cdpIn, tblocknum rootIn);  // returns false on error
  void unlock(void);  // unlocks only if locked
  t_btree_read_lock(void)
    { locked=false; }
  ~t_btree_read_lock(void)
    { unlock(); }
};

class t_simple_update_context
{ cdp_t cdp;      
  const table_descr * tbdf;
  tattrib attrib;
  bool update_disabled;
  t_atrmap_flex indmap;
  t_btree_read_lock btrl[INDX_MAX];
 public:
  bool lock_roots(void);
  void unlock_roots(void);
  t_simple_update_context(cdp_t cdpIn, const table_descr * tbdfIn, tattrib attribIn);
  ~t_simple_update_context(void)
    { } // unlocking made in destructors of [btrl]
  bool inditem_remove(trecnum recnum, const char * newval);
  bool inditem_add(trecnum recnum);
};

extern int last_temp_tabnum;  /* is always <= 0 */
void tables_stat(uns16 * installed, uns16 * locked, uns16 * locks, uns16 * temp_tables);

t_sigstate check_signature_state(cdp_t cdp, ttablenum tb, trecnum rec, tattrib attr, BOOL trace);
BOOL write_signature(cdp_t cdp, table_descr * keytab_descr, ttablenum tb, trecnum rec, tattrib attr, t_enc_hash * enc_hash);
BOOL write_signature_ex(cdp_t cdp, table_descr * keytab_descr, ttablenum tb, trecnum rec, tattrib attr, t_enc_hash * enc_hash,
                        trecnum keyrec, uns32 timestamp);
BOOL prepare_signing(cdp_t cdp, table_descr * keytab_descr, ttablenum tb, trecnum rec, tattrib attr,
                   t_hash hsh, WBUUID key_uuid, uns32 expdate);
BOOL prepare_signing_ex(cdp_t cdp, table_descr * keytab_descr, ttablenum tb, trecnum rec, tattrib attr,
                   trecnum keyrec, t_hash_ex hsh, uns32 * timestamp);
void trace_sig(cdp_t cdp, t_user_ident * usid, char * dest, unsigned level);
void register_change_map(cdp_t cdp, table_descr * tbdf, trecnum recnum, const t_atrmap_flex * column_map);
void register_change(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib atr);
int check_constrains(cdp_t cdp, table_descr * tbdf, trecnum check_recnum, const t_atrmap_flex * ch_map, int immediate_check, BOOL * contains_other_check, char * err_constr_name);
trecnum new_user_or_group(cdp_t cdp, const tobjname name, tcateg categ, const WBUUID uuid, bool no_privil_check=false);
void unregister_index_changes_on_table(cdp_t cdp, table_descr * tbdf);

class table_descr_auto
{ table_descr * tbdf_int;
 public:
  table_descr_auto(cdp_t cdp, ttablenum tb, int level = MAX_TABLE_DESCR_LEVEL)
    { tbdf_int = tb==NOTBNUM ? NULL : install_table(cdp, tb, level); }
  table_descr * operator ->() const
    { return tbdf_int; }
  table_descr * tbdf(void)
    { return tbdf_int; }
  ~table_descr_auto(void)
    { if (tbdf_int) unlock_tabdescr(tbdf_int); }
};

class table_descr_cond
{ table_descr * tbdf_int;
 public:
  table_descr_cond(cdp_t cdp, ttablenum tb);
  table_descr * operator ->() const
    { return tbdf_int; }
  table_descr * tbdf(void)
    { return tbdf_int; }
  ~table_descr_cond(void)
    { if (tbdf_int) unlock_tabdescr(tbdf_int); }
};

void mark_installed_tables(void);

class cur_descr;
void read_record(cdp_t cdp, BOOL std, table_descr * tbdf, trecnum recnum, cur_descr * cd, tptr & request);
void write_record(cdp_t cdp, BOOL std, table_descr * tbdf, trecnum recnum, cur_descr * cd, tptr & request);

class t_exclusive_table_access
{ bool has_access, changing_tabdef;
  table_descr * tbdf;
  cdp_t cdp;
  table_descr * upper_exclusively_accessed_table;  // valid only when has_access==true
 public:
  bool drop_descr;
  t_exclusive_table_access(cdp_t cdpIn, bool changing_tabdefIn) : 
      cdp(cdpIn), changing_tabdef(changing_tabdefIn)
    { has_access = drop_descr = false; }
  bool open_excl_access(table_descr * tbdfIn, bool allow_for_reftab = false);
  ~t_exclusive_table_access(void);
};

class t_dumper
{ FHANDLE hnd;
  unsigned bufcont;
  char buf[4096];
  cdp_t cdp;
  public:
    t_dumper(cdp_t cdpIn, const char * fname) : cdp(cdpIn)
    { hnd=CreateFile(fname, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, 0); 
      bufcont=0;
    }
    ~t_dumper(void)
    { DWORD wr;
      WriteFile(hnd, buf, bufcont, &wr, NULL);
      CloseFile(hnd); 
    }
    void out(const char * str);
    void outi(unsigned val)
    { char buf[30];
      int2str(val, buf);  out(buf); 
    }
    void dump_key(const dd_index * idx_descr, const char * key);
    void dump_tree(cdp_t cdp, const dd_index * idx_descr, tblocknum node_num);
};

#define IS_SYSTEM_COLUMN(name) (!memcmp(name, "_W5_", 4))
