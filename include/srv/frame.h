/****************************************************************************/
/* Frame.h - Core frames management                                         */
/****************************************************************************/
#define ANY_PAGE  (t_client_number)0x7fff

typedef struct    /* ALIGNED, core frame control block */
{ tblocknum blockdadr;   /* number of block contained in the frame */
  tfcbnum   nextfcbnum;  /* number of the next fbc in the hash list */
  t_client_number owner; /* owning user number or ANY_PAGE if shared */
  long      fixnum;      /* number of fixes on the page */
  BOOL      dirty;       // TRUE iff page contents differs from the disc contents, possible only if owner!=ANY_PAGE
  union
  { unsigned  lru;         /* time of the last page access2 */
    tblocknum swapdadr;    /* disc address where swapped to */
  } fc;
  inline bool is_shared_page(void) const
    { return owner==ANY_PAGE; }
  inline void set_shared_page(void)
    { owner=ANY_PAGE; }
} t_fcb;

extern t_fcb * fcbs;
extern CRITICAL_SECTION cs_deadlock, cs_frame;
#define MAX_FIXES   0xfe   /* maximal number of fixes on a page */

tfcbnum fix_new_index_page(cdp_t cdp, tblocknum dadr);
inline tfcbnum fix_new_page(cdp_t cdp, tblocknum dadr, t_translog * translog)
{ 
  tfcbnum fcbn = fix_new_index_page(cdp, dadr);
  if (fcbn!=NOFCBNUM) translog->page_private(dadr);
  return fcbn;
}
tfcbnum fix_new_shared_page(cdp_t cdp, tblocknum dadr);
void unfix_page(cdp_t cdp, tfcbnum fcbn);
tptr fix_priv_page(cdp_t cdp, tblocknum dadr, tfcbnum * pfcb, t_translog * translog);
const char * fix_any_page(cdp_t cdp, tblocknum dadr, tfcbnum * pfcb);
BOOL fast_page_read(cdp_t cdp, tblocknum dadr, unsigned offs, unsigned size, void * buf);
#ifdef STOP
void unfix_page(tfcbnum fcbn);
inline void UNFIX_PAGE(tfcbnum fcbn)
{ 
  EnterCriticalSection(&cs_frame);  // without the CS this function crashes when [fcbs] is reallocated in the same time
  if (--fcbs[fcbn].fixnum<0) 
    { fcbs[fcbn].fixnum=0;  dbg_err("Unfix state 1"); }
  LeaveCriticalSection(&cs_frame);
}
#endif
inline void UNFIX_PAGE(cdp_t cdp, tfcbnum fcbn)
{ 
  EnterCriticalSection(&cs_frame);  // without the CS this function crashes when [fcbs] is reallocated in the same time
  if (--fcbs[fcbn].fixnum<0) 
    { fcbs[fcbn].fixnum=0;  dbg_err("Unfix state 1"); }
  LeaveCriticalSection(&cs_frame);
  cdp->fixnum--;
}
inline void UNFIX_PAGE_in_cs(cdp_t cdp, tfcbnum fcbn)  // called from inside cs_frame
{ 
  if (--fcbs[fcbn].fixnum<0) 
    { fcbs[fcbn].fixnum=0;  dbg_err("Unfix state 1"); }
  cdp->fixnum--;
}
/********** frameadr **********/
extern char * framesptr;       /* core pointer to the region */
extern void * regionhandle;
#define FRAMEADR(fcbn) (framesptr + (BLOCKSIZE+1)*(fcbn))
tptr frameadr(tfcbnum fcbn);

BOOL frame_management_init(unsigned kernel_cache_size);
void frame_management_close(void);
BOOL frame_management_reset(unsigned kernel_cache_size);

#define BPOOL1DADR header.bpool_first
#define BPOOL_CNT  header.bpool_count

BOOL make_swapped(cdp_t cdp, tblocknum dadr, tblocknum * swdadr);
void free_all_pages(void);
void get_fixes(unsigned * pages,  unsigned * fixes, unsigned * frames);
void unfix_page_and_drop(cdp_t cdp, tfcbnum fcbn);
bool releasing_the_page(cdp_t cdp, tblocknum dadr);

////////////////////////// INDEPENDENT disc access model /////////////////////////
void check_nonex_private_page(cdp_t cdp, tblocknum dadr);

inline 
void trace_alloc(cdp_t cdp, tblocknum dadr, bool alloc, const char * descr)
{}

class t_independent_block
{private:
  cdp_t cdp;
  tfcbnum fcbn;
  tblocknum dadr;
  bool changed;  // may be true only if fcbn!=NOFCBNUM
 public:
  inline t_independent_block(cdp_t cdpIn) : cdp(cdpIn)
   { fcbn=NOFCBNUM;  changed=false; }
  inline void close(void)
  { if (fcbn!=NOFCBNUM)
    { if (changed) 
      { ffa.write_block(cdp, FRAMEADR(fcbn), dadr);
        changed=false;
      }
      UNFIX_PAGE(cdp, fcbn);  fcbn=NOFCBNUM;
    }
  }
  inline void close_damaged(void)
  { if (fcbn!=NOFCBNUM) 
      { unfix_page_and_drop(cdp, fcbn);  changed=false;  fcbn=NOFCBNUM; }
  }
  inline ~t_independent_block(void)
    { close(); }
  inline char * access_block(tblocknum adadr) // Closed state supposed
  { dadr=adadr;
    check_nonex_private_page(cdp, dadr);
    return (char*)fix_any_page(cdp, dadr, &fcbn); 
  }
  inline char * access_block_private(tblocknum adadr) // Closed state supposed
    { dadr=adadr;  return (char*)fix_any_page(cdp, dadr, &fcbn); }
  inline char * access_new_block(tblocknum & adadr) // Closed state supposed
  { adadr = dadr = disksp.get_block(cdp);
    if (!dadr) return NULL;
    trace_alloc(cdp, dadr, true, "new independent");
    check_nonex_private_page(cdp, dadr);
    fcbn=fix_new_shared_page(cdp, dadr);  // must not use fix_new_page, page must not be made private!
    if (fcbn==NOFCBNUM) return NULL;
    return (char*)FRAMEADR(fcbn);
  }
  inline void data_changed(void)
    { changed=true; }
  inline void data_unchanged(void)
    { changed=false; }
  inline char * current_ptr(void) const // Open state supposed
    { return FRAMEADR(fcbn); }
};

class t_versatile_block
{private:
  cdp_t cdp;
  tfcbnum fcbn;
  tblocknum dadr;
  bool changed;  // may be true only if fcbn!=NOFCBNUM, used only for indep==true
  const bool indep;
 public:
  inline t_versatile_block(cdp_t cdpIn, bool indepIn) : cdp(cdpIn), indep(indepIn)
   { fcbn=NOFCBNUM;  changed=false; }
  inline void close(void)
  { if (fcbn!=NOFCBNUM)
    { if (indep && changed) 
      { ffa.write_block(cdp, FRAMEADR(fcbn), dadr);
        changed=false;
      }
      UNFIX_PAGE(cdp, fcbn);  fcbn=NOFCBNUM;
    }
  }
  inline void close_damaged(void)
  { if (fcbn!=NOFCBNUM) 
      { unfix_page_and_drop(cdp, fcbn);  changed=false;  fcbn=NOFCBNUM; }
  }
  inline ~t_versatile_block(void)
    { close(); }
  inline char * access_block(tblocknum adadr, t_translog * translog) // Closed state supposed
  { dadr=adadr;
    if (indep)
    { check_nonex_private_page(cdp, dadr);
      return (char*)fix_any_page(cdp, dadr, &fcbn); 
    }
    else // translog!=NULL 
      return (char*)fix_priv_page(cdp, dadr, &fcbn, translog); 
  }
  inline char * access_block_private(tblocknum adadr, t_translog * translog) // Closed state supposed
  { dadr=adadr;
    if (indep)
      return (char*)fix_any_page(cdp, dadr, &fcbn); 
    else // translog!=NULL
      return (char*)fix_priv_page(cdp, dadr, &fcbn, translog); 
  }
  inline char * access_new_block(tblocknum & adadr, t_translog * translog) // Closed state supposed
  { // like alloc_alpha_block, the fixing new, private or shared
    if (indep)
    { adadr = dadr = disksp.get_block(cdp);
      if (!dadr) return NULL;
      trace_alloc(cdp, dadr, true, "new versatile");
      check_nonex_private_page(cdp, dadr);
      fcbn=fix_new_shared_page(cdp, dadr);  // must not use fix_new_page, page must not be made private!
    }
    else // translog!=NULL
    { adadr = dadr = translog->alloc_block(cdp);
      if (!dadr) return NULL;
      fcbn=fix_new_page(cdp, dadr, translog);
    }
    if (fcbn==NOFCBNUM) return NULL;
    return (char*)FRAMEADR(fcbn);
  }
  inline void data_changed(t_translog * translog)
  { if (indep)
      changed=true; 
    else // translog!=NULL
      translog->page_changed(fcbn);
  }
  inline void data_unchanged(void)
    { changed=false; }
  inline char * current_ptr(void) const // Open state supposed
    { return FRAMEADR(fcbn); }
};

 /* locking, data structures are in table.h */
struct table_descr;
int  record_lock           (cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type);
BOOL record_lock_error     (cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type);
BOOL record_unlock         (cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type);
int  wait_record_lock      (cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type, int waiting);
//BOOL wait_record_lock_error(cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type);
BOOL wait_record_lock_error(cdp_t cdp, table_descr * tbdf, trecnum recnum, uns8 lock_type, bool report_error = true);
BOOL wait_page_lock  (cdp_t cdp, tblocknum dadr, uns8 lock_type);
void unlock_user(cdp_t cdp, t_client_number user, bool include_object_locks);
int who_locked(cdp_t cdp, table_descr * tbdf, trecnum record, bool write_lock);
#define FULLTABLE  (trecnum)-1
void remove_the_lock(cdp_t cdp, lockinfo * llock);
BOOL is_all_locked(cdp_t cdp, table_descr * tbdf);
BOOL can_uninstall(cdp_t cdp, table_descr * tbdf);
unsigned page_locks_count(void);

#define page_lock(cdp, page,lock_type)   record_lock(  cdp,NULL,(trecnum)page,lock_type)
#define page_unlock(cdp, page,lock_type) record_unlock(cdp,NULL,(trecnum)page,lock_type)
void frame_mark_core(void);

#define NEW_LOCKS

#define EMPTY_CELL   (trecnum)-2
#define INVALID_CELL (trecnum)-3

struct t_lock_cell // structure describing a lock or a group of identical locks on consecutive records/pages
{ t_client_number owner;  // client owning this lock 
  uns8 locktype;          // type of the lock
  trecnum object;         // the (1st) locked object - record number or page dadr 
  //unsigned count;         // number of consecutive identical locks with the same owner

  inline bool is_valid(void) const
    { return object!=INVALID_CELL && object!=EMPTY_CELL; }
  inline void invalidate(void)
    { object=INVALID_CELL; }
  inline bool is_empty(void) const
    { return object==EMPTY_CELL; }
  inline void set_empty(void)
    { object=EMPTY_CELL; }
};

class t_locktable
// List of locks.
//  Locks are being removed by invalidating.
//  After removing temporary locks when the list contains small number of locks the hash table is downsized.
//  All operations are protected by a CS.
// Write-lock logic:
//  When a write-lock is being explicitly removed, it is changed into a temporary write lock.
//  When a temporary write-lock is being inserted a same or bigger wtite lock is present, the operation is dropped.
// All temporary lock are registered in the transaction log.
// Ordinary locks may have multiple occurences, temporary locks cannot.
{ unsigned table_size;
  unsigned valid_cells;  // not empty and valid
  unsigned used_cells;   // non-empty cells, includes invalid cells
  bool cs_is_valid;
  t_lock_cell * locks;
 public:
  enum { INITIAL_SIZE = 6 };
  CRITICAL_SECTION cs_locks;

  t_locktable(void);
  ~t_locktable(void);
  inline bool no_space_for_new_lock(void) const  // must work for INITIAL_SIZE and for 0:0
    { return valid_cells >= table_size / 4 * 3 || used_cells >= table_size / 5 * 4; }
  bool resize(unsigned table_sizeIn);
  inline unsigned bigger_table_size(void) const
    { return table_size ? 2*table_size : INITIAL_SIZE; }
  inline bool is_almost_empty(void) const
    { return valid_cells <= 3 && table_size>INITIAL_SIZE; }
  inline bool any_lock(void) const
    { return valid_cells > 0; }

  inline unsigned hash(trecnum object) // only object can be the key, other values may change
    { return (17*object) % table_size; }

 // passing cells for a given key (empty cell terminates passing, cells may have various keys):
  inline void find_first_cell(trecnum record, unsigned & ind, t_lock_cell *& lp)
    { ind = hash(record);  lp = locks + ind; }
  inline void find_next_cell(unsigned & ind, t_lock_cell *& lp) 
    { if (++ind>=table_size) { ind=0;  lp=locks; } else lp++; }
 // passing all valid cells (cells exhausted when next_cell() returns false):
  inline void pre_first_cell(unsigned & ind, t_lock_cell *& lp)
    { ind = (unsigned)-1;  lp = locks-1; }
  inline bool next_cell(unsigned & ind, t_lock_cell *& lp) 
  { while (++ind<table_size) 
    { lp++; 
      if (lp->is_valid()) return true;
    }
    return false;
  }
  inline void cell_invalidated(void)
    { valid_cells--; }
  bool remove_my_tmplocks(t_client_number applnum);

  inline void insert(trecnum object, /*unsigned count,*/ t_client_number owner, uns8 locktype);
  inline int try_locking(cdp_t cdp, bool lock_it, table_descr * tbdf, trecnum record, uns8 lock_type);
  inline int add_lock(cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type);
  inline BOOL remove_lock(cdp_t cdp, trecnum record, uns8 lock_type, table_descr * tbdf);
  bool simple_remove_lock(t_client_number owner, trecnum recnum, uns8 lock_type);
  bool unlock_user_tab(t_client_number owner);
  bool do_I_have_lock(cdp_t cdp, trecnum record, uns8 lock_type);

  void mark_core(void)
    { if (locks) mark(locks); }
  void move_locks(t_locktable & old_table)
  { table_size  = old_table.table_size;   old_table.table_size =0;
    valid_cells = old_table.valid_cells;  old_table.valid_cells=0;
    used_cells  = old_table.used_cells;   old_table.used_cells =0;
    locks = old_table.locks;              old_table.locks=NULL;  // prevents destroying the locks!
  }

  friend unsigned page_locks_count(void);
};

extern t_locktable pg_lockbase;
void lock_opened(void);
bool lock_all_index_roots(cdp_t cdp);

