/****************************************************************************/
/* Frame.c - Core frames management                                         */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#pragma hdrstop
#include "profiler.h"
#ifndef WINS
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#endif

#define gettext_noop(string) string
#define CHECK_NONEX_PRIVATE_PAGE
#define PRE_DADR_LOCK  // must be defined, error otherwise!

#define BCHNG_ALLOC_STEP 10
CRITICAL_SECTION cs_frame;   /* initialized iff fcbs */

void locking_error(void)
{
  dbg_err("index_locking_error");
}
///////////////////////////////////////////////////////////////////////////////////
// Locking a disc block number during the I/O operation in the block
// dadr is locked iff it is in [dadrs] array. Only items [0..last_used] are valid/
// Free cells have the 0 value.
// Race conditions: - when I start waiting for the semaphore, it may have already been released - OK
//                  - when my waiting for the semaphore ends, the block may be locked again before I enter cs_frame
// AXIOMs: When I'waiting, I'm in the list: I put myself in the list before I started waiting and my semaphore will
//         be released when I'm being removed
// AXIOMs: My waiting will end, because every lock will be removed and I will register for releasing before starting to wait.

class dadr_locks_class // all data contents is protected by cs_frame
{ enum { MAX_DADR_LOCKS = 60 };
  tblocknum dadrs[MAX_DADR_LOCKS];  // list of locked block numbers
  int last_used;  // >=last valid item in the dadrs array (invalid items inside [0..last_used] have 0 value)
  cdp_t waiting_for_dadr;  // linked list of client threads waiting for a block number
  friend void dump_frames(cdp_t cdp);
 public:
  void init(void);
  void deinit(void);

  inline void unlock(int ind) // called inside cs_frame
  {// remove the [ind] item from [dadrs], but store its contents to [dadr].
    tblocknum dadr = dadrs[ind];
    dadrs[ind]=0;  if (ind==last_used) last_used--;
   // release all threads waiting in [waiting_for_dadr] for this [dadr] value:
    cdp_t * pcdp = &waiting_for_dadr;
    cdp_t acdp = *pcdp;
    while (acdp)
    { if (acdp->dws.locking_dadr==dadr)
      { *pcdp=acdp->dws.next;
        // if acdp did not start waitning for its semaphore, it will do this soon - no problem
#ifdef WINS      
        ReleaseSemaphore(acdp->dws.hSemaphore, 1, NULL);
#else  // Unix
        sem_post(&acdp->dws.hSemaphore);
#endif        
      }
      else
        pcdp=&acdp->dws.next;
      acdp = *pcdp;  
    }
  }
  
  inline int wait_lock(cdp_t cdp, tblocknum dadr, BOOL * waited) // called from cs_frame
  /* If dadr is locked, waits. Locks dadr, returns lock index. */
  /* In *waited returns info if leaved cs_frame. */
  { int i, fr;
    if (waited!=NULL) *waited=FALSE;
   // wait for the "not locked" state:
   lock_retry:
    fr=-1;
    for (i=0;  i<=last_used;  i++)
      if (dadrs[i]==dadr) /* locked, wait */
      {// put itself into the waiting list:
        cdp->dws.locking_dadr=dadr;
        cdp->dws.next=waiting_for_dadr;
        waiting_for_dadr=cdp;
       // leave the CS and wait: 
        LeaveCriticalSection(&cs_frame);
        cdp->wait_type=WAIT_DADR_LOCK;
        cdp->dlock_cycles++;
#ifdef WINS      
        DWORD res=WaitForSingleObject(cdp->dws.hSemaphore, INFINITE);
#else  // Unix
        sem_wait(&cdp->dws.hSemaphore);
#endif        
        cdp->wait_type=WAIT_CS_FRAME;
        EnterCriticalSection(&cs_frame);
        cdp->wait_type=100+WAIT_CS_FRAME;
        if (waited!=NULL) *waited=TRUE;
        goto lock_retry;  // the block number may have been locked again before I entered cs_frame
      }
      else 
        if (!dadrs[i]) fr=i;  /* free space found */
   // is not locked by any other thread, enter the locked state:
    if (fr==-1) 
    { if (last_used>=MAX_DADR_LOCKS-1)  // this happened! The array is full, just wait (outside the CS)
      { LeaveCriticalSection(&cs_frame);
        cdp->wait_type=WAIT_DADR_LOCK;
        Sleep(2000);
        cdp->wait_type=WAIT_CS_FRAME;
        EnterCriticalSection(&cs_frame);
        cdp->wait_type=100+WAIT_CS_FRAME;
        if (waited!=NULL) *waited=TRUE;
        goto lock_retry;
      }
      fr=++last_used;
    }
   // insert the lock and return the index to the array:
    dadrs[fr]=dadr;
    return fr;
  }    

  inline bool is_locked(tblocknum dadr)  // just checks the state, does not lock nor unlock
  { if (!dadr) return false;  // get_frame may call it with dadr==0
    for (int i=0;  i<=last_used;  i++)
      if (dadrs[i]==dadr) return true;
    return false;
  }
};

void dadr_locks_class::init(void)
{ last_used=-1;  // the array is made empty
  waiting_for_dadr=NULL;
}

void dadr_locks_class::deinit(void)
{ }

static dadr_locks_class dadr_locks;
/////////////////////////// frames and control blocks ////////////////////////
#define FCBS_ALLOC_STEP   100
t_fcb * fcbs = NULL;  /* pointer to the array of core frame control blocks */
static unsigned fcbs_count = 0;     /* allocated core & swap control blocks */
static tfcbnum  free_swap_cbs;  /* head of the linked list of free swap cbs */

static void * frameshandle = 0;
char *  framesptr;       /* core pointer to the region */
static tfcbnum frame_count = 0;
#define FCB_HASH_SIZE 503
static tfcbnum fcb_hash_table[FCB_HASH_SIZE];

inline void RELEASE_SWAP(cdp_t cdp, tfcbnum scbn) 
// [scbn] is supposed to be removed from the hashed list. Called inside cs_frame.
{// return the disc block:
  transactor.free_swap_block(fcbs[scbn].fc.swapdadr);
 // return [scbn] to the list of free swap control blocks:
  fcbs[scbn].nextfcbnum=free_swap_cbs;
  free_swap_cbs=scbn;
}

inline tfcbnum drop_swap(cdp_t cdp, tfcbnum * pscbn)  // called inside cs_frame
// Removes from the list and realases the *pscbn and returns the next fcbn in the list.
{ tfcbnum fcbn=*pscbn;
  *pscbn=fcbs[fcbn].nextfcbnum;  /* swap cb unchained */
  RELEASE_SWAP(cdp,fcbn);        /* releasing the swap block & cb */
  return *pscbn;
}

#define hash_fnc(dadr) ((unsigned)((dadr) % FCB_HASH_SIZE))

static void insert_fcb(tfcbnum fcbn)
/* Inserts frame&swap control blocks into their chain depending on blockdadr. */
/* Called inside cs_frame! */
{ unsigned h=hash_fnc(fcbs[fcbn].blockdadr);
  fcbs[fcbn].nextfcbnum=fcb_hash_table[h];
  fcb_hash_table[h]=fcbn;
}

t_locktable pg_lockbase;
/* locks on pages not containing table records */

void frame_mark_core(void)
{ pg_lockbase.mark_core();
  transactor.mark_core();
  mark(fcbs);
}

BOOL realloc_scbs(int new_fcbs_count)  // Called inside cs_frame
// Increases the nubmer of swap control blocks, but does not affect existing control blocks.
// Returns FALSE iff cannot increase.
{// reallocate fcbs table:
  t_fcb * new_fcb = (t_fcb*)corealloc((new_fcbs_count) * sizeof(t_fcb), 47);
  if (!new_fcb) return FALSE;
  memcpy(new_fcb, fcbs, fcbs_count * sizeof(t_fcb));
  corefree(fcbs);  fcbs=new_fcb;
 // add the new scbs into the list of free_swap_cbs:
  for (int i=fcbs_count;  i<new_fcbs_count-1;  i++)
		fcbs[i].nextfcbnum=i+1;
  fcbs[new_fcbs_count-1].nextfcbnum=0;
	free_swap_cbs=fcbs_count;
  fcbs_count = new_fcbs_count;
  return TRUE;
}

void destroy_frames(void)
{ if (frameshandle)
    { aligned_free(frameshandle);  frameshandle=0; }
  if (fcbs)
    { corefree(fcbs);   fcbs=NULL;  }
}

BOOL frame_management_reset(unsigned kernel_cache_size)
// Inits or re-inits frame structures, must not be called when any scb is used.
{ int i;
  int current_scbs = fcbs_count - frame_count;
  if (current_scbs<FCBS_ALLOC_STEP) current_scbs=FCBS_ALLOC_STEP;
 // drop the old objects:
  destroy_frames();
 // allocate space for frames:
  frame_count=(1024L*kernel_cache_size) / BLOCKSIZE;
  if (frame_count < 35) frame_count = 35;  /* low limit */
  frameshandle=aligned_alloc((BLOCKSIZE+1)*frame_count+1, &framesptr);
  if (!frameshandle) return FALSE;
 // set bytes after blocks for the overrun checking
  for (i=0;  i<frame_count;  i++)  
    FRAMEADR(i)[BLOCKSIZE]=(uns8)i;
 // allocating frame info table: array of fcbs 
  fcbs_count = frame_count + current_scbs;
  fcbs=(t_fcb*)corealloc(fcbs_count * sizeof(t_fcb), 47);
  if (!fcbs) { destroy_frames();  return FALSE; }
 // create the list of free swap control blocks:
  for (i=frame_count;  i<fcbs_count-1;  i++)
	  fcbs[i].nextfcbnum=i+1;
  fcbs[fcbs_count-1].nextfcbnum=0;
  free_swap_cbs=frame_count;
 // init the hash table:
  for (i=0;  i<FCB_HASH_SIZE;  i++) fcb_hash_table[i]=NOFCBNUM;
 // insert all frame control blocks into the hash table with empty contents:
  for (i=0;  i<frame_count;    i++)
  { fcbs[i].blockdadr=0;
    fcbs[i].dirty=FALSE;
    fcbs[i].fixnum=0;  /* not fixed, ready for use */
    fcbs[i].set_shared_page();
    fcbs[i].fc.lru=0;
    insert_fcb(i);
  }
  return TRUE;
}

/****************************** init & deinit *******************************/
BOOL frame_management_init(unsigned kernel_cache_size)
{ if (frame_management_reset(kernel_cache_size))
  { dadr_locks.init();
    InitializeCriticalSection(&cs_frame);
    return TRUE;
  }
  return FALSE;
}

void frame_management_close(void)
// Can be called only after a suffessfull frame_management_init
{ destroy_frames();
  dadr_locks.deinit();
  DeleteCriticalSection(&cs_frame);  
}

/********************************* page swapping ****************************/
static tblocknum swap_block(cdp_t cdp, tfcbnum fcbn)
// Copies the block to the swap block. Returns swap blocknum or 0 on error. 
// Called from cs_frame with dadr-lock. Temporarily leaves cs_frame.
// Does not request error, errors in make_swapped are ignored.
{/* get a swap control block to scbn */
  if (!free_swap_cbs)  /* must reallocate the fcbs */
    if (!realloc_scbs(fcbs_count + FCBS_ALLOC_STEP)) return 0;
  tfcbnum scbn=free_swap_cbs;  free_swap_cbs=fcbs[free_swap_cbs].nextfcbnum;
 /* copy the info between fcbs */
  fcbs[scbn]=fcbs[fcbn];  // copying the [dirty] member too
  fcbs[scbn].fc.swapdadr=0;  // swap is incomplete!
  insert_fcb(scbn);  // must be before leaving the CS, otherwise the owner of the swapped page would not find it!
 /* write the block to disc */
  LeaveCriticalSection(&cs_frame);
  cdp->wait_type=140+WAIT_CS_FRAME;
  tblocknum swapblock=transactor.save_block(FRAMEADR(fcbn));
  cdp->wait_type=WAIT_CS_FRAME;
  EnterCriticalSection(&cs_frame);
  cdp->wait_type=100+WAIT_CS_FRAME;
 /* register the new swap control block */
  if (swapblock)
    fcbs[scbn].fc.swapdadr=swapblock;  // swap completed
  else  // not swapped, remove and release scbn
  {// remove from the hashed list: 
    tfcbnum * pfcbn=fcb_hash_table+hash_fnc(fcbs[scbn].blockdadr);
    while (*pfcbn!=scbn) pfcbn=&fcbs[*pfcbn].nextfcbnum;
    *pfcbn=fcbs[scbn].nextfcbnum;   /* minfcbn removed from the list */
   // release the swap cb:
    fcbs[scbn].nextfcbnum=free_swap_cbs;  free_swap_cbs=scbn; 
  }
  return swapblock;
}

/***************************** core frame control blocks ********************/
unsigned fcb_time = 0;

static void null_fcb_time(void)  // called from cs_frame
{ tfcbnum fcbn;
  for (fcbn=0;  fcbn<frame_count;  fcbn++) fcbs[fcbn].fc.lru=0;
  fcb_time=0;
}

#define GET_FRAME_RETRY_LIMIT 100
#define FIX_LIMIT_1 50
#define FIX_LIMIT_2 60

void print_frame_stats(cdp_t cdp)  // called in cs_frame
{ char msg[200];
  int fixes=0, locknum=0;
  for (tfcbnum fcbn=0;  fcbn<frame_count;  fcbn++)
    if (fcbs[fcbn].fixnum) fixes++;
    else
      if (dadr_locks.is_locked(fcbs[fcbn].blockdadr)) // the locking state will not change while I am in the cs_frame
        locknum++;
  sprintf(msg, "Pages: Fixed %u, locked %u, count %u.", fixes, locknum, frame_count);
  cd_dbg_err(cdp, msg);
}

void dump_frames(cdp_t cdp)
{
  char msg[100];
  for (tfcbnum fcbn=0;  fcbn<frame_count;  fcbn++)
  { sprintf(msg, "%04u: %06u %03u %u, %c ->%04u", 
    fcbn, fcbs[fcbn].blockdadr, fcbs[fcbn].owner, fcbs[fcbn].fixnum, fcbs[fcbn].dirty ? 'D' : '.', fcbs[fcbn].nextfcbnum);
    direct_to_log(msg);
  }  
  for (int i=0;  i<=dadr_locks.last_used;  i++)
    if (dadr_locks.dadrs[i])
    { sprintf(msg, "Locked DADR: %u", dadr_locks.dadrs[i]);
      direct_to_log(msg);
    }  
}

static timestamp last_frames_dump = 0;

void dump_frames_cond(cdp_t cdp)
{
  timestamp ts = stamp_now();
  if (ts>last_frames_dump+10)
  { dump_frames(cdp);
    last_frames_dump=ts;
  }
}
  
static tfcbnum get_frame(cdp_t cdp)
/* Returns an empty frame number, out of chains.
   Called insisde cs_frame. Temporarily locks the swapped page. */
// Sets fixnum==1 for the returned page
{ tfcbnum minfcbn;  
  if (cdp->fixnum>FIX_LIMIT_1)
  { cd_dbg_err(cdp, "Too many fixed pages for the client");
    dump_frames(cdp);
    dump_thread_overview(cdp);
    if (cdp->fixnum>FIX_LIMIT_2)
    { LeaveCriticalSection(&cs_frame);
      Sleep(10000);
      EnterCriticalSection(&cs_frame);
    }  
  }  
  int retry_count = 0;
  do  /* searching until a page is available */
  {/* find the last recently used page */
    unsigned mintime=0xffffffff;
    minfcbn=0;  // init. to avoid a warning
    for (tfcbnum fcbn=0;  fcbn<frame_count;  fcbn++)
      if (!fcbs[fcbn].fixnum)
        if (fcbs[fcbn].fc.lru < mintime)
          if (!dadr_locks.is_locked(fcbs[fcbn].blockdadr)) // the locking state will not change while I am in the cs_frame
            { mintime=fcbs[fcbn].fc.lru;  minfcbn=fcbn; }
    if (mintime!=0xffffffff) break;   /* a page found */
   /* waiting for a free frame */
    if (cdp->break_request) 
      { request_error(cdp, REQUEST_BREAKED);  return NOFCBNUM; }
    if (retry_count++ < GET_FRAME_RETRY_LIMIT)
    { cd_dbg_err(cdp, "Sleep for frame");
      dump_frames(cdp);
      //dump_thread_overview(cdp);
      LeaveCriticalSection(&cs_frame);
      cdp->wait_type=WAIT_GET_FRAME;
      Sleep(10);
      cdp->wait_type=WAIT_CS_FRAME;
      EnterCriticalSection(&cs_frame);
      cdp->wait_type=100+WAIT_CS_FRAME;
    }
    else
    { dump_frames(cdp);
      dump_thread_overview(cdp);
      LeaveCriticalSection(&cs_frame);
      request_error(cdp, FRAMES_EXHAUSTED);  
      EnterCriticalSection(&cs_frame);
      return NOFCBNUM; 
    }
  } while (true);

 /* removing the minfcbn page from the list: */
  tfcbnum * pfcbn=fcb_hash_table+hash_fnc(fcbs[minfcbn].blockdadr);
  while (*pfcbn!=minfcbn)
  { if (*pfcbn==NOFCBNUM)  /* internal error, page not in any chain */
      { request_error(cdp, IE_PAGING);  return NOFCBNUM; }
    pfcbn=&fcbs[*pfcbn].nextfcbnum;
  }
  *pfcbn=fcbs[minfcbn].nextfcbnum;   /* minfcbn removed from the list */

 /* swapping, if any changes registered for the page: */
  fcbs[minfcbn].fixnum=1;  cdp->fixnum++;
  if (!fcbs[minfcbn].is_shared_page() && fcbs[minfcbn].dirty)  /* must save as a swaped page */
  {// check if the page being remomed is not swapped (it may be swapped and in core in the same time) - must not create second swap!
    tfcbnum fcbn=fcb_hash_table[hash_fnc(fcbs[minfcbn].blockdadr)];
    while (fcbn!=NOFCBNUM)
    { if (fcbs[fcbn].blockdadr==fcbs[minfcbn].blockdadr)
        if (fcbs[fcbn].owner==fcbs[minfcbn].owner)
          if (fcbn >= frame_count) break;  // found swapped
      fcbn=fcbs[fcbn].nextfcbnum;
    }
    if (fcbn==NOFCBNUM)  // unless a swap found
    { int dl_ind;
      if (fcbs[minfcbn].blockdadr) 
        dl_ind=dadr_locks.wait_lock(cdp, fcbs[minfcbn].blockdadr, NULL); // does not wait because is_locked checked above & dadr-locks not changed
      else dl_ind=-1;  
      tblocknum swdadr=swap_block(cdp, minfcbn);  // leaves cs_frame inside, must set fixnum before in order to prevent returning the same frame again
      if (dl_ind!=-1) dadr_locks.unlock(dl_ind);
      if (!swdadr) /* not swapped, prob. out of fcbs */
      { fcbs[minfcbn].fixnum=0;  cdp->fixnum--;  insert_fcb(minfcbn);  // back into the chain
        request_error(cdp, TOO_COMPLEX_TRANS);  return NOFCBNUM;
      }
    }
  }
  fcbs[minfcbn].blockdadr=0;         /* must not be done before swap! */
  return minfcbn;
}

#if 0
void check_uniq(tblocknum dadr)
{ int cnt=0;   tfcbnum fcbn, * pfcbn;
  pfcbn=&fcb_hash_table[hash_fnc(dadr)];  fcbn=*pfcbn;
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)       /* unprobable */
      cnt++;
    pfcbn=&fcbs[fcbn].nextfcbnum;  fcbn=*pfcbn;
  }
  if (cnt>1)
    cnt++;
}
#endif

inline tfcbnum remove_page_contents(tfcbnum * pfcbn)
// Removes the page thom the chain and clears its contents info but does not change fixnum.
{ tfcbnum fcbn = *pfcbn;
  *pfcbn=fcbs[fcbn].nextfcbnum; /* removed from the chain */
  fcbs[fcbn].blockdadr=0;      /* destroy contents */
  fcbs[fcbn].dirty=FALSE;
  fcbs[fcbn].fc.lru=0;    /* preference */
  fcbs[fcbn].set_shared_page();
  insert_fcb(fcbn);
  return *pfcbn;
}

bool releasing_the_page(cdp_t cdp, tblocknum dadr)
// Called when the page is immediately released - the ownership must be cleared, otherwise commit would write my values over the proper ones
{ tfcbnum fcbn, * pfcbn;
  bool ok=true;
  ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
 /* searching for the page */
  pfcbn=&fcb_hash_table[hash_fnc(dadr)];  fcbn=*pfcbn;
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)       // happens when a page is allocated and deallocated in one transaction, including index pages
      if (fcbs[fcbn].owner == cdp->applnum_perm)
      { if (fcbn < frame_count)   /* core frame */
          remove_page_contents(pfcbn);
          //fcbs[fcbn].fixnum=0; -- must not do this, may be fixed and warns when unfixed later (when releasing the piece)
        else  // swapped page: must release the swap, otherwise it would overwrite the data on commit!!!
          drop_swap(cdp, pfcbn);
        break;
      }
      else if (!fcbs[fcbn].is_shared_page())  // releasing private page: should be among invalid index pages
      { cdp_t acdp = cds[fcbs[fcbn].owner];
#ifdef STOP  // not possible any more, private pages are removed when index is made invalid
        if (acdp->tl.is_invalid_index_updated_page(dadr))  // remove now (would be removed later when updating the index but may cause diagnostic messages when using the page in another place)
          ::remove_changes(acdp, &t_logitem(CHANGE_TAG, dadr, (trecnum)-1), true);
        else
#endif
        { char buf[200];
          sprintf(buf, fcbs[fcbn].dirty ? "releasing dirty page %u of client %d" : "releasing private page %u of client %d", 
            dadr, fcbs[fcbn].owner); 
          cd_dbg_err(cdp, buf);
          ok=false;
        }
      }
        
    pfcbn=&fcbs[fcbn].nextfcbnum;  fcbn=*pfcbn;
  }
  return ok;
}


#ifdef STOP  // makes no sense any more, when an index is made invalid, its private pages are removed immediately
bool t_translog_main::is_invalid_index_updated_page(tblocknum dadr)   // called on commit
{
 // add index changes to the main log:
  for (t_index_uad * iuad = index_uad;  iuad;  iuad=iuad->next)
    if (iuad->invalid)
    { t_hash_item_iuad * itm = iuad->info.find(t_hash_item_iuad(dadr));  
      if (itm->block==dadr)  // found
        if (itm->oper & (IND_PAGE_UPDATED|IND_PAGE_PRIVATE|IND_PAGE_FREED))
          return true;
    }
  return false;
}
#endif    

tfcbnum fix_new_index_page(cdp_t cdp, tblocknum dadr)
{ tfcbnum fcbn, * pfcbn, shared_fcbn=NOFCBNUM;  /* fixes a frame without loading it */
  ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
 /* searching for the page */
  pfcbn=&fcb_hash_table[hash_fnc(dadr)];  fcbn=*pfcbn;
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)       /* unprobable */
    { if (fcbs[fcbn].is_shared_page())   /* this page must be in core */
        shared_fcbn=fcbn;
      else if (fcbs[fcbn].owner == cdp->applnum)
        if (fcbn < frame_count) 
        { if (fcbs[fcbn].fixnum)
            cd_dbg_err(cdp, "new page is fixed 1");
          fcbs[fcbn].fixnum=1;  cdp->fixnum++;
          goto found;  /* core frame */
        }
        else  // swapped page: must release the swap, otherwise it would overwrite the data on commit!!!
        { fcbn=drop_swap(cdp, pfcbn);  // I want to continue the cycle
          continue;
        }
      else  
        if (fcbn < frame_count) 
          cd_dbg_err(cdp, "new index page is somebody's private");
        else  
          cd_dbg_err(cdp, "new index page is somebody's swapped");
    }
    pfcbn=&fcbs[fcbn].nextfcbnum;  fcbn=*pfcbn;
  }
 /* owned page not found */
  if (shared_fcbn!=NOFCBNUM)    /* shared page found, use it */
  { fcbn=shared_fcbn;
    fcbs[fcbn].owner = cdp->applnum;  /* page is made owned */
    if (fcbs[fcbn].fixnum)
      cd_dbg_err(cdp, "new page is fixed 2");
    fcbs[fcbn].fixnum=1;  cdp->fixnum++;
  }
  else /* page not found, create it */
  { fcbn=get_frame(cdp);   
    if (fcbn==NOFCBNUM) return NOFCBNUM;  /* DEADLOCK result */
    fcbs[fcbn].blockdadr=dadr;
    fcbs[fcbn].dirty    =FALSE;
    fcbs[fcbn].owner    =cdp->applnum;
    insert_fcb(fcbn);
  }

 found:
 /* lru actions: */
  if (fcb_time>=0xfffffffe) null_fcb_time();
  fcbs[fcbn].fc.lru=++fcb_time;
 /* OK result: */
  return fcbn;
}

tfcbnum fix_new_shared_page(cdp_t cdp, tblocknum dadr)
{ tfcbnum fcbn, * pfcbn, shared_fcbn=NOFCBNUM;  /* fixes a frame without loading it */
  ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
 /* searching for the page */
  pfcbn=&fcb_hash_table[hash_fnc(dadr)];  fcbn=*pfcbn;
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)       /* unprobable */
    { if (fcbn < frame_count) 
        shared_fcbn=fcbn;
      else  // swapped page: must release the swap, otherwise it would overwrite the data on commit!!!
      { fcbn=drop_swap(cdp, pfcbn);  // I want to continue the cycle
        continue;
      }
    }
    pfcbn=&fcbs[fcbn].nextfcbnum;  fcbn=*pfcbn;
  }
 /* owned page not found */
  if (shared_fcbn!=NOFCBNUM)    /* shared page found, use it */
  { fcbn=shared_fcbn;
    if (fcbs[fcbn].fixnum)
      cd_dbg_err(cdp, "new page is fixed 3");
    fcbs[fcbn].fixnum=1;  cdp->fixnum++;
  }
  else /* page not found, create it */
  { fcbn=get_frame(cdp);   
    if (fcbn==NOFCBNUM) return NOFCBNUM;  /* DEADLOCK result */
    fcbs[fcbn].blockdadr=dadr;
    insert_fcb(fcbn);
  }
  fcbs[fcbn].set_shared_page();
  fcbs[fcbn].dirty = FALSE;
 /* lru actions: */
  if (fcb_time>=0xfffffffe) null_fcb_time();
  fcbs[fcbn].fc.lru=++fcb_time;
 /* OK result: */
  return fcbn;
}

static tfcbnum unswap_block(cdp_t cdp, tfcbnum * pfcbn)
/* Called inside cs_frame when dadr is locked. */
{ tfcbnum scbn, fcbn;
 /* remove the swap cb from the hash list */
  scbn=*pfcbn;
  *pfcbn=fcbs[scbn].nextfcbnum;
 /* get core page and reload the swapped page into it: */
  fcbn=get_frame(cdp);
  if (fcbn==NOFCBNUM) return NOFCBNUM;   /* DEADLOCK result */
  LeaveCriticalSection(&cs_frame);
  cdp->wait_type=140+WAIT_CS_FRAME;
  transactor.load_block(FRAMEADR(fcbn), fcbs[scbn].fc.swapdadr);
  cdp->wait_type=WAIT_CS_FRAME;
  EnterCriticalSection(&cs_frame);
  cdp->wait_type=100+WAIT_CS_FRAME;
#ifdef STOP  // read errors not detected
  if (res)
  { request_error(cdp, UNREADABLE_BLOCK);
    insert_fcb(fcbn);
    return NOFCBNUM;
  }
#endif
 /* setting up the new fcb */
  fcbs[scbn].fixnum=1;   // the next assignment must not clear this in fcbs[fcbn]
  fcbs[fcbn]=fcbs[scbn]; // [dirty] is supposed to be set
  insert_fcb(fcbn);
  RELEASE_SWAP(cdp,scbn);  /* releasing the swap block & cb */
  return fcbn;
}

const char * fix_any_page(cdp_t cdp, tblocknum dadr, tfcbnum * pfcb)
/* fixes a page and returns its address and fcb number,
   returns NULL on error. */
/* Preferences:
    ANY required:
      1. my owned page, even if swapped (I must see my previous changes!)
      2. shared page
      3. loading */
{ tfcbnum fcbn, * pfcbn, shared_fcbn;  BOOL res;  int dl_ind;
  BOOL waited;
  int hash_val=hash_fnc(dadr);
  ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
#ifndef PRE_DADR_LOCK
  dl_ind=-1;  
 fix_retry:
#else
  dl_ind=dadr_locks.wait_lock(cdp, dadr, &waited);
#endif
 /* ANY page: */
  shared_fcbn=NOFCBNUM;
  pfcbn=fcb_hash_table+hash_val;  fcbn=*pfcbn;  /* 1st in chain */
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)
    { if (fcbs[fcbn].owner == cdp->applnum)     /* my owned page */
        if (fcbn < frame_count) 
        { fcbs[fcbn].fixnum++;  cdp->fixnum++;
          goto have_core; /* my core page */
        }
        else                    goto unswap;    /* my swapped page */
      if (fcbs[fcbn].is_shared_page())
        shared_fcbn=fcbn;
    }
    pfcbn=&fcbs[fcbn].nextfcbnum;  fcbn=*pfcbn;  /* to the next in chain */
  }
  if (shared_fcbn!=NOFCBNUM) /* found shared page */ 
  { fcbn=shared_fcbn;  
    fcbs[fcbn].fixnum++;  cdp->fixnum++;
    goto have_core; 
  }

 /* the page is not in core nor swapped, load it from database */
#ifndef PRE_DADR_LOCK
  if (dl_ind==-1)  /* not locked yet */
  { dl_ind=dadr_locks.wait_lock(cdp, dadr, &waited);
    if (waited) goto fix_retry;
  }
#endif  
  fcbn=get_frame(cdp); 
  if (fcbn==NOFCBNUM) goto fix_err;  /* deadlock result */
 /* loading the page into core */
  fcbs[fcbn].blockdadr=dadr;   /* this locks the frame against get_frame */
  fcbs[fcbn].dirty    =FALSE;
  LeaveCriticalSection(&cs_frame);
  cdp->wait_type=140+WAIT_CS_FRAME;
  res=ffa.read_block(cdp, dadr, FRAMEADR(fcbn));
  cdp->wait_type=WAIT_CS_FRAME;
  EnterCriticalSection(&cs_frame);
  cdp->wait_type=100+WAIT_CS_FRAME;
  if (res)
  { request_error(cdp, UNREADABLE_BLOCK);
    fcbs[fcbn].fixnum--;  cdp->fixnum--;  fcbs[fcbn].blockdadr=0;  fcbs[fcbn].set_shared_page();  
    insert_fcb(fcbn);
    goto fix_err;
  }
 /* setting up the new fcbn */
  fcbs[fcbn].set_shared_page();
  insert_fcb(fcbn);
  goto have_core;

 unswap:
#ifndef PRE_DADR_LOCK
  if (dl_ind==-1)  /* not locked yet */
  { dl_ind=dadr_locks.wait_lock(cdp, dadr, &waited);
    if (waited) goto fix_retry;
  }
#endif
  fcbn=unswap_block(cdp, pfcbn);
  if (fcbn==NOFCBNUM) goto fix_err;  /* deadlock result */

 have_core:   /* now fcbn points to the correct core page */
  if (dl_ind!=-1) dadr_locks.unlock(dl_ind);
 /* lru actions: */
  if (fcb_time>=0xfffffffe) null_fcb_time();
  fcbs[fcbn].fc.lru=++fcb_time;
 /* OK result: */
  *pfcb=fcbn;
  tptr fadr;
  fadr=FRAMEADR(fcbn);
#if 1
  if ((uns8)fadr[BLOCKSIZE]!=(uns8)fcbn)   /* I am outside the cs_frame! */
    { request_error(cdp, IE_FRAME_OVERRUN);  *pfcb=NOFCBNUM;  return NULL; }
#endif
  return fadr;

 fix_err:
  if (dl_ind!=-1) dadr_locks.unlock(dl_ind);
  *pfcb=NOFCBNUM;
  return NULL;
}

tptr fix_priv_page(cdp_t cdp, tblocknum dadr, tfcbnum * pfcb, t_translog * translog)
/* fixes a page and returns its address and fcb number,
   returns NULL on error. */
/* Preferences:
    Owned required:
      1. my owned page, even if swapped
      2. shared page - copy if fixed
      3. loading
*/
{ tfcbnum fcbn, shared_fcbn, * pfcbn;  BOOL res;  int dl_ind;
  BOOL waited;
  int hash_val=hash_fnc(dadr);
  ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
#ifndef PRE_DADR_LOCK
  dl_ind=-1;  
 fix_retry:
#else
  dl_ind=dadr_locks.wait_lock(cdp, dadr, &waited);
#endif
 /* ANY page: */
  pfcbn=fcb_hash_table+hash_val;  fcbn=*pfcbn;  /* 1st in chain */
  shared_fcbn=NOFCBNUM;
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)
    { if (fcbs[fcbn].owner == cdp->applnum)     /* my owned page */
        if (fcbn < frame_count) 
        { fcbs[fcbn].fixnum++;  cdp->fixnum++;
          goto have_core; /* my core page */
        }
        else goto unswap;    /* my swapped page */
      if (fcbs[fcbn].is_shared_page() && !fcbs[fcbn].fixnum)   /* this page must be in core */
        shared_fcbn=fcbn;
    }
    pfcbn=&fcbs[fcbn].nextfcbnum;  fcbn=*pfcbn;  /* to the next in chain */
  }

 // the page is not in core nor swapped, change the shared unfixed page into private page 
  if (shared_fcbn!=NOFCBNUM)
  { fcbn=shared_fcbn;
    fcbs[fcbn].fixnum=1;  cdp->fixnum++;
    fcbs[fcbn].dirty=FALSE;  // must be define because the next line makes [dirty] valid
    fcbs[fcbn].owner = cdp->applnum;   /* page is made owned */
    translog->page_private(dadr);
    goto have_core;
  }

 // the page is not in core nor swapped, load it from database:
#ifndef PRE_DADR_LOCK
  if (dl_ind==-1)  /* not locked yet */
  { dl_ind=dadr_locks.wait_lock(cdp, dadr, &waited);
    if (waited) goto fix_retry;
  }
#endif
  fcbn=get_frame(cdp);  /* chlist==-1 */
  if (fcbn==NOFCBNUM) goto fix_err;   /* deadlock result */
 /* loading the page into core */
  fcbs[fcbn].blockdadr=dadr;   /* this locks the frame against get_frame */
  fcbs[fcbn].dirty    =FALSE;
  if (shared_fcbn!=NOFCBNUM)
    memcpy(FRAMEADR(fcbn), FRAMEADR(shared_fcbn), BLOCKSIZE);
  else
  { LeaveCriticalSection(&cs_frame);
    cdp->wait_type=140+WAIT_CS_FRAME;
    res=ffa.read_block(cdp, dadr, FRAMEADR(fcbn));
    cdp->wait_type=WAIT_CS_FRAME;
    EnterCriticalSection(&cs_frame);
    cdp->wait_type=100+WAIT_CS_FRAME;
    if (res)
    { request_error(cdp,UNREADABLE_BLOCK);
      fcbs[fcbn].fixnum--;  cdp->fixnum--;  fcbs[fcbn].blockdadr=0;  fcbs[fcbn].set_shared_page();
      insert_fcb(fcbn);
      goto fix_err;
    }
  }
 /* setting up the new fcbn */
  fcbs[fcbn].owner=cdp->applnum;
  if (cdp->applnum!=ANY_PAGE)  // unless calculating the committed value of index key
    translog->page_private(dadr);
  insert_fcb(fcbn);
  goto have_core;

 unswap:
#ifndef PRE_DADR_LOCK
  if (dl_ind==-1)  /* not locked yet */
  { dl_ind=dadr_locks.wait_lock(cdp, dadr, &waited);
    if (waited) goto fix_retry;
  }
#endif
  fcbn=unswap_block(cdp, pfcbn);
  if (fcbn==NOFCBNUM) goto fix_err;  /* deadlock result */

 have_core:   /* now fcbn points to the correct core page */
  if (dl_ind!=-1) dadr_locks.unlock(dl_ind);
 /* lru actions: */
  if (fcb_time>=0xfffffffe) null_fcb_time();
  fcbs[fcbn].fc.lru=++fcb_time;
 /* OK result: */
  *pfcb=fcbn;
  tptr fadr;
  fadr=FRAMEADR(fcbn);
#if 1
  if ((uns8)fadr[BLOCKSIZE]!=(uns8)fcbn)   /* I am outside the cs_frame! */
    { request_error(cdp,IE_FRAME_OVERRUN);  *pfcb=NOFCBNUM;  return NULL; }
#endif
  return fadr;

 fix_err:
  if (dl_ind!=-1) dadr_locks.unlock(dl_ind);
  *pfcb=NOFCBNUM;
  return NULL;
}

void unfix_page(cdp_t cdp, tfcbnum fcbn)
{ if (fcbn!=NOFCBNUM) 
    UNFIX_PAGE(cdp, fcbn);
}

void check_nonex_private_page(cdp_t cdp, tblocknum dadr)
#ifdef CHECK_NONEX_PRIVATE_PAGE
{ ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
  for (tfcbnum fcbn=fcb_hash_table[hash_fnc(dadr)];  fcbn!=NOFCBNUM;  fcbn=fcbs[fcbn].nextfcbnum)
    if (fcbs[fcbn].blockdadr==dadr && !fcbs[fcbn].is_shared_page())
      dbg_err("INDEPENDENT PAGE IS PRIVATE");
}
#else
{}
#endif

BOOL fast_page_read(cdp_t cdp, tblocknum dadr, unsigned offs, unsigned size, void * buf)
{ tfcbnum fcbn;  
  const char * dt = fix_any_page(cdp, dadr, &fcbn);
  if (dt==NULL) return TRUE;
  memcpy(buf, dt+offs, size);
  UNFIX_PAGE(cdp, fcbn);
  return FALSE;
}

/************************** write or remove changes *************************/
//#define IMAGE_WRITER

static BOOL wr_changes(cdp_t cdp, const t_logitem * logitem)
/* Called only at commit, updates the other copies of the same page.
   The page may be both in core and swapped (if transactions are secured). */
// Must delete swapped page for the client.
// Called from inside of cs_frame
{ tfcbnum fcbn, * pfcbn, * pscbn, thefcbn;
  tptr dt0;  unsigned off, pos, poscnt;
  tblocknum dadr = logitem->block;  t_chngmap * chngmap;
  int dl_ind=dadr_locks.wait_lock(cdp, dadr, NULL);
  bool found_other_owner = false;
  bool limited_update = false;
 /* find the page & destroy the swapped copy of the written page */
  thefcbn=NOFCBNUM;  pscbn=NULL;
  fcbn=*(pfcbn=fcb_hash_table+hash_fnc(dadr));
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)
      if (fcbs[fcbn].owner == cdp->applnum_perm)
        if (fcbn >= frame_count) 
        { if (pscbn) dbg_err("Double swap");
          pscbn=pfcbn;  /* swapped page */
        }
        else
        { thefcbn=fcbn;
          if (fcbs[fcbn].fixnum)
            if (fcbs[fcbn].owner)  // possible when periodic thread is backuping a fulltext and other thread allocates a value from a sequence -> owner==0
              cd_dbg_err(cdp, "INTERNAL WARNING: committing a fixed page");  // suspicious: when is private and fixed, may be overwritten later, in the "shared" state! changemap is not important
          fcbs[fcbn].fixnum++;  cdp->fixnum++;  /* fix the page! */
        }
      else
        found_other_owner=true;  // the ther copy may be private or shared
    fcbn=*(pfcbn=&fcbs[fcbn].nextfcbnum);
  }

  poscnt=0;  // init to avoid a warning
  if (logitem->chngmap==FULL_CHNGMAP)
    chngmap=NULL;  // all page changed
  else if (logitem->chngmap==INDEX_CHNGMAP) 
  { chngmap=NULL;  // all page changed
    limited_update=true;
  }
  else if (logitem->chngmap==NULL) 
  { trace_alloc(cdp, dadr, true, "no changes");
    if (pscbn) drop_swap(cdp, pscbn);  // perhaps impossible because the page cannot be dirty
    if (thefcbn==NOFCBNUM) goto ex; /* deadlock result */
    else goto ex1;  // nothing changed in the page
  }
  else 
  { chngmap=logitem->chngmap;  poscnt = BLOCKSIZE/chngmap->recsize; }
  trace_alloc(cdp, dadr, limited_update, "wr changes");

#ifdef IMAGE_WRITER
  if (!found_other_owner)  // no need to unswap it, if swapped
  { if (thefcbn!=NOFCBNUM)
    { if (pscbn) /* delete the swapped copy if the original page remained in memory */
        drop_swap(cdp, pscbn);  
      if (logitem->chngmap==NULL) goto ex1;  // nothing changed in the page -> make it shared
      the_image_writer.add_item(thefcbn, true);
    }
    else if (pscbn)
    { if (logitem->chngmap==NULL)   // nothing changed in the page
        { drop_swap(cdp, pscbn);  goto ex; }
      the_image_writer.add_item(*pscbn, false);

    }
  }
  else
#endif
  if (thefcbn==NOFCBNUM)   /* page is not in core */
  { if (!pscbn) goto ex; /* page not found: was not changed & was removed */
    thefcbn=unswap_block(cdp, pscbn); // fixes the page
    if (thefcbn==NOFCBNUM) goto ex; /* deadlock result */
  }
  else if (pscbn) /* delete the swapped copy if the original page remained in memory */
    drop_swap(cdp, pscbn);  
 // thefcbn is the page, it is in core and is fixed, no swap exists (index changes have been merged before)

 /* write the page to the database */
  dt0=FRAMEADR(thefcbn);
  LeaveCriticalSection(&cs_frame);
  cdp->wait_type=140+WAIT_CS_FRAME;
  ffa.write_block(cdp, dt0, dadr);
  cdp->wait_type=WAIT_CS_FRAME;
  EnterCriticalSection(&cs_frame);
  cdp->wait_type=100+WAIT_CS_FRAME;
  fcbs[thefcbn].dirty=FALSE;

 /* update other copies of the same page */
  bool found_swapped;  found_swapped=false;
 /* writing into not-swapped pages */
  fcbn=fcb_hash_table[hash_fnc(dadr)];
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)
      if (fcbs[fcbn].owner != cdp->applnum_perm)
        if (!limited_update || fcbs[fcbn].is_shared_page())
          if (fcbn < frame_count)
          { tptr dest = FRAMEADR(fcbn);
            if (chngmap==NULL) memcpy(dest, dt0, BLOCKSIZE);
            else for (pos=0;  pos<poscnt;  pos++)
              if (!chngmap->map[pos / 32]) pos+=31;
              else if (chngmap->map[pos / 32] & (1<<(pos % 32)))
              { off=chngmap->recsize*pos;
                memcpy(dest+off, dt0+off, chngmap->recsize);
              }
          }
          else found_swapped=true;
    fcbn=fcbs[fcbn].nextfcbnum;
  }
 /* writing into swapped pages */
  if (!limited_update)
    while (found_swapped)
    { found_swapped=false;
      fcbn=*(pfcbn=fcb_hash_table+hash_fnc(dadr));
      while (fcbn!=NOFCBNUM)
      { if (fcbs[fcbn].blockdadr==dadr)
          if (fcbs[fcbn].owner != cdp->applnum_perm)
            if (fcbn >= frame_count)
            { found_swapped=true;
              fcbn=unswap_block(cdp, pfcbn);
              if (fcbn!=NOFCBNUM)  /* == only as a result of deadlock */
              { tptr dest = FRAMEADR(fcbn);
                if (chngmap==NULL) memcpy(dest, dt0, BLOCKSIZE);
                else for (pos=0;  pos<poscnt;  pos++)
                  if (!chngmap->map[pos / 32]) pos+=31;
                  else if (chngmap->map[pos / 32] & (1<<(pos % 32)))
                  { off=chngmap->recsize*pos;
                    memcpy(dest+off, dt0+off, chngmap->recsize);
                  }
                UNFIX_PAGE_in_cs(cdp, fcbn);
              }
              break;  /* the chain may have been damaged */
            }
        fcbn=*(pfcbn=&fcbs[fcbn].nextfcbnum);
      }
    }
 ex1:
  fcbs[thefcbn].set_shared_page();
  UNFIX_PAGE_in_cs(cdp, thefcbn);
 ex:
  dadr_locks.unlock(dl_ind);
  return FALSE;
}

static void remove_changes(cdp_t cdp, t_logitem * logitem, bool releasing_page)
// Called 1. on roll_back for every changed page. 
//        2. when a page is directly deallocated (before commit) - must not leave it private nor dirty!
//        3. When subtransaction terminates (local table is destructed or cursor is closed)
// Must delete swapped page for the client.
// When rollback is called after unsucessful commit, same pages may be both in core and swapped!
// Called from inside of cs_frame
{ tfcbnum fcbn, * pfcbn, *my_core=NULL;   
  tblocknum dadr = logitem->block;
  trace_alloc(cdp, dadr, false, "remove changes");
  int dl_ind=dadr_locks.wait_lock(cdp, dadr, NULL);  // protects against calling this when unswapping the page by other wr_changes
  tfcbnum shared_page=NOFCBNUM;
  fcbn=*(pfcbn=fcb_hash_table+hash_fnc(dadr));
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr == dadr)
      if (fcbs[fcbn].owner == cdp->applnum_perm)
        if (fcbn < frame_count) 
          my_core=pfcbn;
        else /* changed page has been swapped: */
        { fcbn=drop_swap(cdp, pfcbn);  // I want to continue the cycle
          continue;
        }
      else if (fcbs[fcbn].is_shared_page())
        shared_page=fcbn;
    fcbn=*(pfcbn=&fcbs[fcbn].nextfcbnum);
  }
  if (my_core!=NULL) // may not exist: page may have been reistered as private but not changed and removed from memory later
  { fcbn=*my_core;
    if (fcbs[fcbn].fixnum && !releasing_page) // this is possible e.g. on error in the trigger called by active referential integrity caused by deleting record. The page with the original deleted record is fixed. If the error is CONDITION_ROLLBACK and in handled, immediate rollback is performed.
    { cd_dbg_err(cdp, "WARNING: removing changes on a fixed page");
      if (logitem->chngmap!=NULL)  // must restore the contents
        if (shared_page!=NOFCBNUM)
          memcpy(FRAMEADR(fcbn), FRAMEADR(shared_page), BLOCKSIZE);
        else
          ffa.read_block(cdp, fcbs[fcbn].blockdadr, FRAMEADR(fcbn)); // this blocks everything, but it is rare
      // not changing into a shared page because writing into it may continue above.
    }
    else // can destroy the contents
      remove_page_contents(my_core);
  }
  dadr_locks.unlock(dl_ind);
}

static void remove_private_page(cdp_t cdp, tblocknum dadr, t_client_number owner)
// Removes private page belonging to [owner], if it exists.  Called from cs_frame.
{ int dl_ind=dadr_locks.wait_lock(cdp, dadr, NULL);  // thread must never use cdp of another thread!
  tfcbnum fcbn, * pfcbn;
  fcbn=*(pfcbn=fcb_hash_table+hash_fnc(dadr));
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr == dadr)
      if (fcbs[fcbn].owner == owner)
      { if (fcbn < frame_count) 
          fcbn=remove_page_contents(pfcbn);  // I want to continue the cycle
        else /* changed page has been swapped: */
          fcbn=drop_swap(cdp, pfcbn);  // I want to continue the cycle
        continue;
      }  
    fcbn=*(pfcbn=&fcbs[fcbn].nextfcbnum);
  }
  dadr_locks.unlock(dl_ind);
}

void unfix_page_and_drop(cdp_t cdp, tfcbnum fcbn)
// Removes changes from the fixed private page by removing its contents
{ tfcbnum * pfcbn;
  ProtectedByCriticalSection cs(&cs_frame);
 // remove fcbn from the chain:
  pfcbn=fcb_hash_table+hash_fnc(fcbs[fcbn].blockdadr);
  while (*pfcbn!=NOFCBNUM)
    if (*pfcbn==fcbn)  // mark as empty and return to the chain:
    { cdp->fixnum -= fcbs[fcbn].fixnum;
      fcbs[fcbn].fixnum=0;    // safe unfixing
      remove_page_contents(pfcbn);
      break; 
    }
    else pfcbn=&fcbs[*pfcbn].nextfcbnum;
}

BOOL make_swapped(cdp_t cdp, tblocknum dadr, tblocknum * swdadr)
/* Ensures that private copy of dadr is swapped, stores its swapdadr.
   Called only on secured commit for every changed page. FALSE if swapped. */
// Swapping error masked, returned TRUE -> no swap created.
{ tfcbnum fcbn;  BOOL res=TRUE;
  ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
  int dl_ind=dadr_locks.wait_lock(cdp, dadr, NULL);
  fcbn=fcb_hash_table[hash_fnc(dadr)];
  while (fcbn!=NOFCBNUM)
  { if (fcbs[fcbn].blockdadr==dadr)
      if (fcbs[fcbn].owner == cdp->applnum_perm)
        if (fcbn < frame_count)   /* page is not swapped, swap it */
        { *swdadr=swap_block(cdp, fcbn);
          res=*swdadr==0;  
        }
        else   /* page is already swapped! */
        { *swdadr=fcbs[fcbn].fc.swapdadr;
          res=FALSE;
        }
    fcbn=fcbs[fcbn].nextfcbnum;
  }
  /* page not found: page was not changed and was removed */
  dadr_locks.unlock(dl_ind);
  return res;  /* page not found */
}
/////////////////////////////// transactor ///////////////////////////////////////
t_transactor::t_transactor(void)
{ srch_start=tjc_init_limit=0;
  InitializeCriticalSection(&cs_trans);
}

t_transactor::~t_transactor(void)
{ DeleteCriticalSection(&cs_trans); }

void t_transactor::construct(void)
{ srch_start=tjc_init_limit=0;
  InitializeCriticalSection(&cs_trans);
  alloc_map.init(0,10);
}

tblocknum t_transactor::get_swap_block()
{ tblocknum block;  uns32 * mapelem;  int bit;
  int cell = srch_start / 32;
  ProtectedByCriticalSection cs(&cs_trans);
  do
  { mapelem = alloc_map.acc(cell);
    if (mapelem==NULL) return 0;
    if (*mapelem < 0xfffffffe) break;
    cell++;
  } while (TRUE);
  block = 32*cell+1;  bit=2;
  while (bit & *mapelem) { bit<<=1;  block++; }
  *mapelem |= bit;
  srch_start=block+1;
  return block;
}

void t_transactor::free_swap_block(tblocknum block)
{ ProtectedByCriticalSection cs(&cs_trans);
  *alloc_map.acc(block / 32) &= ~(1 << (block % 32));
  if (block < srch_start) srch_start=block;
}

bool WriteFileAt(FHANDLE fhandle, tblocknum blocknum, const void * data, int datasize)
{ DWORD wr;
#ifdef WINS
  unsigned _int64 off64 = (unsigned _int64)blocknum*BLOCKSIZE;
  return (SetFilePointer(fhandle, (DWORD)off64, ((long*)&off64)+1, FILE_BEGIN) != (DWORD)-1 || GetLastError()==NO_ERROR) &&
#elif defined LINUX
  return  SetFilePointer(fhandle, (uns64)blocknum*BLOCKSIZE, NULL, FILE_BEGIN) != (DWORD)-1 &&
#else
  return  SetFilePointer(fhandle, blocknum*BLOCKSIZE,        NULL, FILE_BEGIN) != (DWORD)-1 &&
#endif
    WriteFile(fhandle, data, datasize, &wr, NULL) && wr==datasize;
}

bool ReadFileAt(FHANDLE fhandle, tblocknum blocknum, void * data, int datasize)
{ DWORD rd;
#ifdef WINS
  unsigned _int64 off64 = (unsigned _int64)blocknum*BLOCKSIZE;
  return (SetFilePointer(fhandle, (DWORD)off64, ((long*)&off64)+1, FILE_BEGIN) != (DWORD)-1 || GetLastError()==NO_ERROR) &&
#elif defined LINUX
  return  SetFilePointer(fhandle, (uns64)blocknum*BLOCKSIZE, NULL, FILE_BEGIN) != (DWORD)-1 &&
#else
  return  SetFilePointer(fhandle, blocknum*BLOCKSIZE,        NULL, FILE_BEGIN) != (DWORD)-1 &&
#endif
    ReadFile(fhandle, data, datasize, &rd, NULL) && rd==datasize;
}

tblocknum t_transactor::save_block(const void * blockadr)
// Saves the block from blockadr to an allocated transaction file block.
// Returns transaction block number or 0 on error.
{ tblocknum block = get_swap_block();
  if (!block) return 0;
  ProtectedByCriticalSection cs(&cs_trans);
  if (!WriteFileAt(ffa.t_handle, block, blockadr, BLOCKSIZE))
    { free_swap_block(block);  block=0; }
 // init the newly created tjc block:
  if ((block & 0xffffffe0) > tjc_init_limit)
 	{ tblocknum tjc_invalid = TJC_INVALID;
    WriteFileAt(ffa.t_handle, (block & 0xffffffe0), &tjc_invalid, sizeof(tblocknum));
    tjc_init_limit=block;
  }
  return block;
}

BOOL t_transactor::load_block(void * blockadr, tblocknum block)
{ ProtectedByCriticalSection cs(&cs_trans);
  return !ReadFileAt(ffa.t_handle, block, blockadr, BLOCKSIZE);
}

void t_transactor::free_tjc_blocks(tblocknum * blocks, int count)
// marks the tcj block as free
{ ProtectedByCriticalSection cs(&cs_trans);
  while (count--)
    *alloc_map.acc0(blocks[count] / 32) &= 0xfffffffe; // dynar access needs cs_
}

tblocknum t_transactor::save_tjc_block(const void * blockadr)
// Allocates tjc block number and saves block to it. Returns -1 or error.
{ tblocknum block;  uns32 * mapelem;  
 // allocate tjc block number:
  int cell = 0;
  ProtectedByCriticalSection cs(&cs_trans);
  do
  { mapelem = alloc_map.acc(cell);
    if (mapelem==NULL) return (tblocknum)-1;
    if (!(*mapelem & 1)) break;
    cell++;
  } while (TRUE);
  block = 32*cell;
  *mapelem |= 1;
 // save block;
  if (!WriteFileAt(ffa.t_handle, block, blockadr, BLOCKSIZE))
    { *mapelem &= 0xfffffffe;  block=(tblocknum)-1; }
  tjc_init_limit=block+1;  // prevents rewriting the tjc block in save_block
  return block;
}

void t_transactor::invalidate_tjc_block(tblocknum block)
{ tblocknum inval = TJC_INVALID;
  ProtectedByCriticalSection cs(&cs_trans);
  WriteFileAt(ffa.t_handle, block, &inval, sizeof(tblocknum));
}

t_transactor transactor;

/////////////////////////////////// gen_hash /////////////////////////////////////

template <class t_hash_item, int min_size> t_hash_item * t_gen_hash<t_hash_item, min_size>::find(const t_hash_item & itm)
// Returns the found item or space for the new item.
{
  unsigned hs = itm.hash() % tablesize;
  int inv=-1;
  do 
  { if (table[hs].is_invalid()) inv=hs;
    if (table[hs].same_key(itm)) return &table[hs];
    if (table[hs].is_empty()) return &table[inv==-1 ? hs : inv];
    if (++hs>=tablesize) hs=0;
  } while (true);
}

template <class t_hash_item, int min_size> bool t_gen_hash<t_hash_item, min_size>::inflate(void)
{ int old_tablesize = tablesize;  
  tablesize *= 3;
  t_hash_item * old_table = table;
  table = new t_hash_item[tablesize];
  if (!table) { error=true;  return false; }
  limitsize = 4*tablesize/5;
 // move items (uses the new [table] and [tablesize]):
  for (int i=0;  i<old_tablesize;  i++)
    if (!old_table[i].is_empty() && !old_table[i].is_invalid())
      memcpy(find(old_table[i]), old_table+i, sizeof(t_hash_item));
  if (old_table!=min_table) delete [] old_table;
  return true;
}

template <class t_hash_item, int min_size> void t_gen_hash<t_hash_item, min_size>::clear(void)
{
  for (int i=0;  i<tablesize;  i++) table[i].destruct();  // deallocation
  if (table!=min_table) 
  { delete [] table;  table=min_table;  tablesize=min_size;  limitsize=min_size-1; 
    for (int i=0;  i<tablesize;  i++) table[i].destruct();  // re-initialization
  }
  contains=0; error=false;
}


/////////////////////////////////// index synchronization //////////////////////////////////////////
void t_index_uad::page_allocated(tblocknum page)   // sets ALLOCATED and UPDATED
{ 
  t_hash_item_iuad * itm = info.find(t_hash_item_iuad(page));  
 // if the page has been FREED before, change its state to UPDATED and exit:
  if (itm->block==page && (itm->oper & IND_PAGE_FREED)) // (this scenario is not possible, a page registered as IND_PAGE_FREED cannot be allocated)
  { itm->oper=IND_PAGE_UPDATED;  
    dbg_err("Allocated a page registred as freed from an index");
    return; 
  }
 // the page cound not be cound, inserting:
  if (info.is_full())
  { if (!info.inflate()) return;
    itm = info.find(t_hash_item_iuad(page));  
  }
  info.inserted();
  itm->set(page, IND_PAGE_ALLOCATED | IND_PAGE_UPDATED);
}

bool t_index_uad::page_freed    (tblocknum page)
// Returns true iff the page has been allocated in this transaction and should be deallocated immediately
//  instead of registering it for deallocation during commit. 
// Allocation record must be removed because of possible rollback.
{
  t_hash_item_iuad * itm = info.find(t_hash_item_iuad(page));  
 // if the page has been ALLOCATED before, remove the item:
  if (itm->block==page)
  { if (itm->oper & IND_PAGE_ALLOCATED)
      { itm->invalidate();  return true; }  // page will be released immediately and any private copy will be removed in releasing_the_page()
    else  // has an private/update record, overwriting it:
      { itm->oper = IND_PAGE_FREED;  return false; }
  }
 // inserting:
  if (info.is_full())
  { if (!info.inflate()) return false;
    itm = info.find(t_hash_item_iuad(page));  
  }
  info.inserted();
  itm->set(page, IND_PAGE_FREED);
  return false;
}

void t_index_uad::page_updated  (tblocknum page)
{
  t_hash_item_iuad * itm = info.find(t_hash_item_iuad(page));  
  if (itm->block==page)
  { itm->oper |= IND_PAGE_UPDATED;  
    return; 
  }
 // inserting:
  if (info.is_full())
  { if (!info.inflate()) return;
    itm = info.find(t_hash_item_iuad(page));  
  }
  info.inserted();
  itm->set(page, IND_PAGE_UPDATED);
}

void t_index_uad::page_private(tblocknum page)
// Registering private copies of pages is necessary even if the are not dirty.
// They must be removed when removing changes on the index!!! Otherwise they would be later used with an invalid contents.
// They must invalidate the index when they are changed/deallocated by other client!
{
  t_hash_item_iuad * itm = info.find(t_hash_item_iuad(page));  
  if (itm->block==page)
  { itm->oper |= IND_PAGE_PRIVATE;  
    return; 
  }
 // inserting:
  if (info.is_full())
  { if (!info.inflate()) return;
    itm = info.find(t_hash_item_iuad(page));  
  }
  info.inserted();
  itm->set(page, IND_PAGE_PRIVATE);
}


void t_index_uad::wr_changes(cdp_t cdp, t_log * log)
// Adds index changes no the normal transaction log.
{ unsigned pos=0;
  const t_hash_item_iuad * item;
  do
  { item = info.get_next(pos);
    if (!item) break;
    if (item->oper & IND_PAGE_UPDATED)
      log->page_changed2(item->block, INDEX_CHNGMAP);
  } while (true);
}

void t_index_uad::remove_my_changes_and_rollback_allocs(cdp_t cdp)
// The root of the index is write-locked.
{ unsigned pos=0;  const t_hash_item_iuad * item;
  do
  { item = info.get_next(pos);
    if (!item) break;
    if (item->oper & (IND_PAGE_UPDATED|IND_PAGE_PRIVATE|IND_PAGE_FREED))  // must remove private copies of all pages (changed and unchanged) when they become invalid!
    { t_logitem li(CHANGE_TAG, item->block, (trecnum)-1);
      ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);  // ::remove_changes() needs this
      ::remove_changes(cdp, &li, (item->oper & IND_PAGE_ALLOCATED) !=0);
    }  
    if (item->oper & IND_PAGE_ALLOCATED)
    { disksp.release_block_direct(cdp, item->block);  // can reuse immediately
      trace_alloc(cdp, item->block, false, "removing index alloc");
    }
  } while (true);
}

void t_index_uad::remove_changes_and_rollback_allocs(cdp_t cdp, t_client_number owner)
// The root of the index is write-locked: the owning client cannot use it.
{ unsigned pos=0;  const t_hash_item_iuad * item;
  do
  { item = info.get_next(pos);
    if (!item) break;
#ifdef STOP    // protocoling for debugging purposes
    char buf[200];
    sprintf(buf, "removing index changes of %u by %u, root %u, page %u, oper %u.\r\n", owner, cdp->applnum_perm, root, item->block, item->oper);
    ffa.to_protocol(buf, (int)strlen(buf));
#endif    
    if (item->oper & (IND_PAGE_UPDATED|IND_PAGE_PRIVATE|IND_PAGE_FREED))  // must remove private copies of all pages (changed and unchanged) when they become invalid!
    { ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);  // ::remove_private_page() needs this
      remove_private_page(cdp, item->block, owner);
    }  
    if (item->oper & IND_PAGE_ALLOCATED)
    { disksp.release_block_direct(cdp, item->block);  // cdp does not belong to the owner of the page, but this should cause no harm becuse private copy of [owner] does not exist
      trace_alloc(cdp, item->block, false, "removing foreign index alloc");
    }
  } while (true);
}

void t_index_uad::commit_allocs(cdp_t cdp)
{ unsigned pos=0;
  const t_hash_item_iuad * item;
  do
  { item = info.get_next(pos);
    if (!item) break;
    if (item->oper & IND_PAGE_FREED)
    { disksp.release_block_safe(cdp, item->block);
      trace_alloc(cdp, item->block, false, "commit index dealloc");
    }
  } while (true);
}

t_index_uad * t_translog_main::find_index_uad(tblocknum root)
// Returns existing index_uad for given [root] from [this] or NULL if not found.
{ t_index_uad * iuad = index_uad;
  while (iuad && iuad->root!=root) iuad=iuad->next;
  return iuad;
}

t_index_uad * t_translog_main::get_uiad(cdp_t cdp, tblocknum root)
// Returns existing index_uad or creates a new one for given [root].
{ 
  t_index_uad * iuad = find_index_uad(root);
  if (!iuad)
  { iuad=new t_index_uad(root, cdp->index_owner);
    if (iuad) // append to the list (other client may scan the list in make_conflicting_indexes_invalid())
    { ProtectedByCriticalSection cs(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
      iuad->next = index_uad;  index_uad=iuad; 
    }
    else
      request_error(cdp, OUT_OF_KERNEL_MEMORY);
  }
  return iuad;
}

void t_translog_main::index_page_allocated(cdp_t cdp, tblocknum root, tblocknum page)
{ 
#ifdef STOP  // verifying for debugging purposes
  if (!pg_lockbase.do_I_have_lock(cdp, root, RLOCK|TMPWLOCK))
    locking_error();
#endif
  t_index_uad * iuad = get_uiad(cdp, root);
  if (iuad) iuad->page_allocated(page);
}

bool t_translog_main::index_page_freed(cdp_t cdp, tblocknum root, tblocknum page)
// Returns true when releasing a page allocated in the same transaction
{ 
#ifdef STOP  // verifying for debugging purposes
  if (!pg_lockbase.do_I_have_lock(cdp, root, RLOCK|TMPWLOCK))
    locking_error();
#endif
  t_index_uad * iuad = get_uiad(cdp, root);
  if (iuad) return iuad->page_freed(page);
  return false;
}

void t_translog_main::index_page_updated(cdp_t cdp, tblocknum root, tblocknum page)
{ 
#ifdef STOP  // verifying for debugging purposes
  if (!pg_lockbase.do_I_have_lock(cdp, root, RLOCK|TMPWLOCK))
    locking_error();
#endif
  t_index_uad * iuad = get_uiad(cdp, root);
  if (iuad) iuad->page_updated(page);
}

void t_translog_main::index_page_private(cdp_t cdp, tblocknum root, tblocknum page)
{ 
#ifdef STOP  // verifying for debugging purposes
  if (!pg_lockbase.do_I_have_lock(cdp, root, RLOCK|TMPWLOCK))
    locking_error();
#endif
  t_index_uad * iuad = get_uiad(cdp, root);
  if (iuad) iuad->page_private(page);
}

void t_translog_main::clear_index_info(cdp_t cdp)
{ t_index_uad * iuad_list;
  if (kernel_is_init)
    EnterCriticalSection(&cs_short_term);
  iuad_list=index_uad;  index_uad=NULL;
  if (kernel_is_init)
    LeaveCriticalSection(&cs_short_term);
  while (iuad_list)
  {// first, remove from the list:
    t_index_uad * iuad = iuad_list;
    iuad_list = iuad_list->next;
   // then unlock (must not be in the list):
    if (iuad->write_locked && cdp)
      page_unlock(cdp, iuad->root, TMPWLOCK);
   // then delete:
    delete iuad;
  }
}

bool update_private_changes_in_index(cdp_t cdp, t_index_uad * iuad)
// Drops all private changed pages of the [iuad] index and replays all own uncommitted changes on the index.
// The [iuad] index is supposed to be locked (RL or WL).
// Returns true if updated, false on error.
{ int i;  bool ok=true;
  char buf[150];
  sprintf(buf, "Client %d updates index with root %d on table %u.", cdp->applnum_perm, iuad->root, iuad->tabnum);
  trace_msg_general(cdp, TRACE_USER_WARNING, buf, NOOBJECT); 
#ifdef STOP  // verifying for debugging purposes
  if (!pg_lockbase.do_I_have_lock(cdp, iuad->root, RLOCK|TMPWLOCK))
    locking_error();
#endif
  prof_time start;
  if (PROFILING_ON(cdp)) start = get_prof_time();  
#ifdef NEVER  // changes must be removed when the index is made invalid because now the pages may have a different use!!!
 // drop private pages belonging to the index:
  iuad->remove_changes_and_rollback_allocs(cdp);
 // clear the index operation information:
  iuad->info.clear();
#endif
 // replay changes:
  t_delayed_change * ch;
  for (i=0;  i<cdp->tl.ch_s.count();  i++)
  { ch = cdp->tl.ch_s.acc0(i);
    if (ch->ch_tabnum==iuad->tabnum) break;
  }
  iuad->invalid=false;  // must drop the flags BEFORE updating, otherwise index operation call updating recursively!
  if (i<cdp->tl.ch_s.count())  // the opposite may happen (I do not know how)
  { table_descr_auto tbdf(cdp, ch->ch_tabnum);
    if (!tbdf->me()) return false;
    // find the index:
    i=0;
    while (i<tbdf->indx_cnt && tbdf->indxs[i].root!=iuad->root) i++;
    assert(i<tbdf->indx_cnt);
    // indate the index:
    dd_index * cc = tbdf->indxs+i;
    int ind=0;  trecnum rec;  int removed=0, inserted=0;
    while (ch->ch_recnums.get_next(ind, rec))
      if (rec!=NORECNUM && rec!=INVALID_HASHED_RECNUM)
      { char own_keyval[MAX_KEY_LEN], orig_keyval[MAX_KEY_LEN];
       // get the own record state and the key value:
        uns8 own_state = table_record_state(cdp, tbdf->me(), rec);
        if (own_state==NOT_DELETED)
          compute_key(cdp, tbdf->me(), rec, cc, own_keyval, false);
       // get the committed record state and the key value:
        t_client_number cli_num = cdp->applnum;  cdp->applnum = ANY_PAGE;  // must not change applnum_perm!
        t_isolation orig_level = cdp->isolation_level;
        if (cdp->isolation_level >= REPEATABLE_READ) cdp->isolation_level=READ_COMMITTED; // prevents changing the locks
        uns8 orig_state = table_record_state(cdp, tbdf->me(), rec);
        if (orig_state==NOT_DELETED)
          compute_key(cdp, tbdf->me(), rec, cc, orig_keyval, false);
        cdp->applnum = cli_num;
        cdp->isolation_level = orig_level;
       // update the index:
        if (orig_state==NOT_DELETED)
        { if (bt_remove(cdp, orig_keyval, cc, iuad->root, table_translog(cdp, tbdf->me())))
            removed++;
        }
        if (own_state==NOT_DELETED)
        { if (bt_insert(cdp, own_keyval, cc, iuad->root, tbdf->selfname, table_translog(cdp, tbdf->me())))
            ok=false;
          else  
            inserted++;
        }
      }
      //if (/*removed && inserted && */removed!=inserted)
      //  dbg_err("confli");
  }
  if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_ACTIVITY, PROFACT_UPDATE_INDEX, 0, NULL);
  return ok;
}

bool t_translog_main::update_all_indexes(cdp_t cdp)
// Updates own invalid versions of all all shared indexes. Supposes that the index is locked.
{
  for (t_index_uad * iuad = index_uad;  iuad;  iuad=iuad->next)
    if (iuad->invalid)
      if (!update_private_changes_in_index(cdp, iuad))
        return false;
  return true;
}

void make_conflicting_indexes_invalid(cdp_t cdp)
// For all clients other than [cdp] find indexes changed by both cdp and the other client and set their [invalid] flag.
// The indexes of [cdp] are write-locked -> other clients cannot commit on them.
// Must invalidate even for pages which are my "private" because they may have been changed/deallocated
{ if (!cdp->tl.index_uad) return;
  ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
  for (int i=1;  i <= max_task_index;  i++) if (cds[i]) 
  { cdp_t acdp = cds[i];
    if (acdp!=cdp && acdp->tl.index_uad)
    { for (t_index_uad * iuad = cdp->tl.index_uad;  iuad;  iuad=iuad->next)
      { bool found=false;   t_index_uad * aiuad;
#ifdef STOP  // verifying for debugging purposes
        if (!pg_lockbase.do_I_have_lock(cdp, iuad->root, TMPWLOCK))
          locking_error();
#endif
       // find same index:
        { ProtectedByCriticalSection cs(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
          aiuad = acdp->tl.find_index_uad(iuad->root);
          if (aiuad && !aiuad->invalid)  // same index found and not set [invalid] by other client
          // the aiuad is protected now by the WL on the index root
          {// for any changed page...:
            unsigned pos=0;
            const t_hash_item_iuad * item;
            do
            { item = iuad->info.get_next(pos);
              if (!item) break;
             // ...find same page:
  #if 0  // slow passing the table:         
              unsigned apos=0;
              const t_hash_item_iuad * aitem;
              do
              { aitem = aiuad->info.get_next(apos);
                if (!aitem) break;
                if (item->block == aitem->block)
                  found=true;
              } while (!found);
  #else  // fast hash-based search:
              t_hash_item_iuad * aitem = aiuad->info.find(t_hash_item_iuad(item->block));  
              if (aitem->block==item->block)  // found (find returns invalid/empty item otherwise)
                found=true;
  #endif            
            } while (!found);
          }
          if (found)  // aiuad will remain valid until I unlock its root because deleting it needs the WL on the root
          { aiuad->invalid=true;
           // acdp private pages of the invalidated index must be removed now because they may deallocated by cdp and reused by acdp:
           // drop private pages belonging to the index:
            aiuad->remove_changes_and_rollback_allocs(cdp, acdp->applnum_perm);
           // clear the index operation information:
            aiuad->info.clear();
          }
        }  // acdp changed same index
      }  // for any my changed index
    }  // the client with index changes
  }  // for any client
}
/////////////////////////////////////////////////////////////////////////////////
// Transakcni log
void t_log::minimize(void)
{ if (htable != minimal_htable) 
  { aligned_free(htable_handle);  htable_handle=0;
    htable = minimal_htable;  
    tablesize=MINIMAL_HTABLE_SIZE;
    limitsize=MINIMAL_HTABLE_SIZE-1;
    memset(htable, 0xff, MINIMAL_HTABLE_SIZE * sizeof(t_logitem));  // sets EMPTY_TAG
  }
}

void t_log::grow_table(void)
{// allocate the new htable:
  int new_tablesize = 3*tablesize;
  void * new_handle;   t_logitem * new_htable;
  new_handle=aligned_alloc(new_tablesize * sizeof(t_logitem), (tptr*)&new_htable);
  if (!new_handle) { error=TRUE;  return; }
  memset(new_htable, 0xff, new_tablesize * sizeof(t_logitem));
 // move the old contents to the new htable
  int new_contains=0;
  for (int ind=0;  ind<tablesize;  ind++)
  { if (htable[ind].nonempty() && !htable[ind].invalid())
    { int nind=htable[ind].hashf(new_tablesize);   // the NEW hash function, tablesize changed
      while (new_htable[nind].nonempty()) if (++nind >= new_tablesize) nind=0;
      new_htable[nind]=htable[ind];
      new_contains++;
    }
  }
 // replace the old table by the new one:
  if (htable != minimal_htable) aligned_free(htable_handle);
  htable=new_htable;  htable_handle=new_handle;  tablesize=new_tablesize;
  limitsize=tablesize * 4 / 5;
  contains=new_contains;
}

void t_log::register_object(tobjnum objnum, int tag)
// Writes object record to the transaction log
{redo:
    t_logitem itm(tag, objnum);
    int ind=itm.hashf(tablesize), inv=-1;
    while (htable[ind].nonempty())
    { if (htable[ind].invalid()) inv=ind;
      else if (htable[ind].tag==tag && htable[ind].objnum==objnum)
        return;  // this happens when a cursor is created and re-created in the same transaction
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
    if (inv!=-1) htable[inv]=itm;  // write to an invalid cell
    else  // write to an empty cell
    { if (contains >= limitsize) { grow_table();  if (!error) goto redo;  else return; }
      htable[ind]=itm;
      contains++;  
    }
}

void t_log::unregister_object(tobjnum objnum, int tag)
{ int ind=t_logitem(tag, objnum).hashf(tablesize);
  while (htable[ind].nonempty())
  { if (htable[ind].tag==tag && htable[ind].objnum==objnum)
      { htable[ind].invalidate();  return; } // removing
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
}

uns32 * t_log::rec_changed(tfcbnum fcbn, tblocknum dadr, int recsize, int recpos)
// Adds CHANGE_TAG record to the transaction log and marks the frame as "dirty".
{ bool err;
  { ProtectedByCriticalSection cs(&cs_frame);
    err=fcbs[fcbn].is_shared_page();
    fcbs[fcbn].dirty=TRUE;
  }  
  if (err) dbg_err("INTERNAL ERROR: writing to a shared page 1");
   redo:
    t_logitem itm(CHANGE_TAG, dadr);
    int ind=itm.hashf(tablesize), inv=-1;
    while (htable[ind].nonempty())
    { if (htable[ind].block==dadr && htable[ind].is_change())
      { if (htable[ind].chngmap!=NULL)
        { if (htable[ind].chngmap==FULL_CHNGMAP || htable[ind].chngmap==INDEX_CHNGMAP) return NULL;
          if (htable[ind].chngmap->recsize==recsize)
          { htable[ind].chngmap->map[recpos / 32] |= 1 << (recpos % 32);
            return htable[ind].chngmap->map;
          }
          else corefree(htable[ind].chngmap);
        }
        goto found; // already registered, allocate chngmap
      }
      else if (htable[ind].invalid()) inv=ind;
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
    if (inv!=-1) ind=inv;
    else
    { if (contains >= limitsize) { grow_table();  if (!error) goto redo;  else return NULL; }
      contains++;  
    }
   // write at ind position: 
    htable[ind]=itm;  
   found:
    if (recsize >= BLOCKSIZE) 
    { htable[ind].chngmap=FULL_CHNGMAP;
      return NULL;
    }
    else
    { int listsize = (BLOCKSIZE / recsize - 1) / 32 + 1;
      htable[ind].chngmap=(t_chngmap*)corealloc(sizeof(t_chngmap)+sizeof(uns32)*(listsize-1), 55);
      if (htable[ind].chngmap==NULL) { error=TRUE;  return NULL; }
      memset(htable[ind].chngmap->map, 0, sizeof(uns32)*listsize);
      htable[ind].chngmap->recsize=recsize;
      htable[ind].chngmap->map[recpos / 32] |= 1 << (recpos % 32);
      return htable[ind].chngmap->map;
    }
}

void t_log::mark_core(void)
{ ProtectedByCriticalSection cs(&cs_frame, cds[0], WAIT_CS_FRAME);
  for (int i=0;  i<tablesize;  i++)
    if (htable[i].is_change())
      if (htable[i].chngmap && htable[i].chngmap!=FULL_CHNGMAP && htable[i].chngmap!=INDEX_CHNGMAP)
        mark(htable[i].chngmap);
}

void t_log::page_changed(tfcbnum fcbn)
// Adds CHANGE_TAG record to the transaction log and marks the frame as "dirty".
{ bool err;  tblocknum dadr;
  { ProtectedByCriticalSection cs(&cs_frame);
    err=fcbs[fcbn].is_shared_page();
    dadr = fcbs[fcbn].blockdadr;
    fcbs[fcbn].dirty=TRUE;
  }  
  if (err) dbg_err("INTERNAL ERROR: writing to a shared page 2");
  page_changed2(dadr, FULL_CHNGMAP);
}

void t_log::page_changed2(tblocknum dadr, t_chngmap * achngmap)
// Adds CHANGE_TAG record to the transaction log.
{
  redo:
    t_logitem itm(CHANGE_TAG, dadr);
    int ind=itm.hashf(tablesize), inv=-1;
    while (htable[ind].nonempty())
    { if (htable[ind].block==dadr && htable[ind].is_change()) // implies valid chngmap
      { if (htable[ind].chngmap!=achngmap) // anything to do
          if (achngmap==FULL_CHNGMAP)  // list pointer or NULL
          { if (htable[ind].chngmap==INDEX_CHNGMAP)
              dbg_err("WARNING: page conversion 1");
            else
              corefree(htable[ind].chngmap);
            htable[ind].chngmap=FULL_CHNGMAP;
          }
          else
          { if (htable[ind].chngmap!=NULL)
              dbg_err("WARNING: page conversion 2");
            htable[ind].chngmap=INDEX_CHNGMAP;
          }
        return;
      }
      else if (htable[ind].invalid()) inv=ind;
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
    if (inv!=-1) ind=inv;
    else
    { if (contains >= limitsize) { grow_table();  if (!error) goto redo;  else return; }
      contains++;  
    }
   // write at ind position: 
    htable[ind]=itm;  
    htable[ind].chngmap=achngmap;
}

void t_translog_main::index_page_changed(cdp_t cdp, tblocknum root, tfcbnum fcbn)
{ bool err;  tblocknum dadr;
  { ProtectedByCriticalSection cs(&cs_frame);
    err=fcbs[fcbn].is_shared_page();
    dadr = fcbs[fcbn].blockdadr;
    fcbs[fcbn].dirty=TRUE;
  }  
  if (err) cd_dbg_err(cdp, "INTERNAL ERROR: writing to a shared page 22");
  index_page_updated(cdp, root, dadr);
}

void t_log::page_private(tblocknum dadr)
// Registers private page with empty change list.
{ redo:
    t_logitem itm(CHANGE_TAG, dadr);
    int ind=itm.hashf(tablesize), inv=-1;
    while (htable[ind].nonempty())
    { if (htable[ind].block==dadr && htable[ind].is_change()) return;
      else if (htable[ind].invalid()) inv=ind;
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
    if (inv!=-1) ind=inv;
    else
    { if (contains >= limitsize) { grow_table();  if (!error) goto redo;  else return; }
      contains++;  
    }
   // write at ind position: 
    htable[ind]=itm;  
    htable[ind].chngmap=NULL;
}

void t_log::page_allocated(tblocknum dadr)
{ redo:
   // look for deallocating of the same page:
    t_logitem itm(TAG_FREE_PAGE, dadr);
    int ind=itm.hashf(tablesize);
    if (!any_sapos)
      while (htable[ind].nonempty())
      { if (htable[ind].block==dadr && htable[ind].tag==TAG_FREE_PAGE) // freed before (impossible scenario)
        { htable[ind].invalidate();  
          dbg_err("Allocated a page registred as freed");
          return; 
        } 
        if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
      }
   // register the allocation:
    itm.tag=TAG_ALLOC_PAGE;
    ind=itm.hashf(tablesize);
    int inv=-1;
    while (htable[ind].nonempty())
    { if (htable[ind].invalid()) { inv=ind;  break; }
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
   // add the page to the list of allocated pages:
    if (inv!=-1) ind=inv;  // use the invalid record in the chain
    else                   // add new record to the chain
    { if (contains >= limitsize) { grow_table();  if (!error) goto redo;  else return; }
      contains++;  
    }
   // write at ind position: 
    htable[ind]=itm;  
}

bool t_log::page_deallocated(tblocknum dadr)
// drop_page_changed called before - no need to do this now
{ redo:
   // look for allocating of the same page:
    t_logitem itm(CHANGE_TAG, dadr);
    int ind;
    if (!any_sapos)
    {
#if 0 // drop_page_changed called before - no need to do this now
     // remove changes for the page:
      ind=itm.hashf(tablesize);
      while (htable[ind].nonempty())
      { if (htable[ind].block==dadr && htable[ind].tag==CHANGE_TAG) 
        // must not invalidate the item, page is private
        { if (htable[ind].chngmap!=FULL_CHNGMAP && htable[ind].chngmap!=INDEX_CHNGMAP) corefree(htable[ind].chngmap);
          htable[ind].chngmap=NULL;
        }
        if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
      }
#endif
     // look for allocation record of the same page: if found, invalidate it and return true
      itm.tag=TAG_ALLOC_PAGE;  ind=itm.hashf(tablesize);
      while (htable[ind].nonempty())
      { if (htable[ind].block==dadr && htable[ind].tag==TAG_ALLOC_PAGE) 
          { htable[ind].invalidate();  return true; } // allocated before
        if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
      }
    }
    itm.tag=TAG_FREE_PAGE;  ind=itm.hashf(tablesize);
    int inv=-1;
    while (htable[ind].nonempty())
    { if (htable[ind].invalid()) { inv=ind;  break; }
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
   // add the page to the list of deallocated pages:
    if (inv!=-1) ind=inv;  // use the invalid record in the chain
    else                   // add new record to the chain
    { if (contains >= limitsize) { grow_table();  if (!error) goto redo;  else return false; }
      contains++;  
    }
   // write at ind position: 
    htable[ind]=itm;  
    return false;
}

void t_log::record_inserted(ttablenum tabnum, trecnum rec)
{// check to see if the record has been deleted in the same transaction:
  t_logitem itm(DELREC_TAG, tabnum, rec);
  int ind=itm.hashf(tablesize);
  while (htable[ind].nonempty())
  { if (htable[ind].tag==DELREC_TAG && htable[ind].objnum==tabnum && htable[ind].recnum==rec)
      { htable[ind].invalidate();  return; } // deleted before
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
 // prepare adding a new item:
  if (contains >= limitsize) { grow_table();  if (error) return; }
  contains++;  
 // find space for insertion:
  itm.tag=INSREC_TAG;
  ind=itm.hashf(tablesize);
  while (htable[ind].nonempty())
  { if (htable[ind].invalid()) { contains--;  break; }
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
 // write at ind position: 
  htable[ind]=itm;
}

void t_log::drop_record_ins_info(ttablenum tabnum, trecnum rec)
// Prevents returning the record to the cache of free records on ROLLBACK.
{// If the record has been inserted in the same transaction, remove the record about it
  t_logitem itm(INSREC_TAG, tabnum, rec);
  int ind=itm.hashf(tablesize);
  while (htable[ind].nonempty())
  { if (htable[ind].tag==INSREC_TAG && htable[ind].objnum==tabnum && htable[ind].recnum==rec)
      { htable[ind].invalidate();  return; } // inserted in the same transaction
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
  return;
}

void t_log::record_released(ttablenum tabnum, trecnum rec)
// Adding the records to the list of released record in the transaction
// Important: must not remove the insertion record, if a record is inserted and deleted+freed, it must be returned to the cache both on COMMIT and ROLLBACK
{
 // prepare adding a new item:
  if (contains >= limitsize) { grow_table();  if (error) return; }
  contains++;  
 // find space for insertion:
  t_logitem itm(DELREC_TAG, tabnum, rec);
  int ind=itm.hashf(tablesize);
  while (htable[ind].nonempty())
  { if (htable[ind].invalid()) { contains--;  break; }
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
 // write at ind position: 
  htable[ind]=itm;
}



void t_log::table_deleted(ttablenum tabnum)
// Removes references to the records of the deleted table from the transaction log.
{ if (contains)
    for (int ind=0;  ind < tablesize;  ind++)
      if (htable[ind].nonempty())
        if (htable[ind].objnum==tabnum)
          if (htable[ind].tag==DELREC_TAG || htable[ind].tag==INSREC_TAG || htable[ind].tag==LOCK_TAG)
            htable[ind].invalidate();
}

void t_log::drop_page_changes(cdp_t cdp, tblocknum dadr)  
// Called ONLY when a page allocated directly is deallocated, prevents writing into it at commit
{// remove changes from the log:
  ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
  t_logitem itm(CHANGE_TAG, dadr);
  int ind=itm.hashf(tablesize);
  while (htable[ind].nonempty())
  { if (htable[ind].block==dadr && htable[ind].is_change()) // implies valid chngmap
    { remove_changes(cdp, &htable[ind], true);
      if (htable[ind].chngmap!=FULL_CHNGMAP && htable[ind].chngmap!=INDEX_CHNGMAP) corefree(htable[ind].chngmap);
      htable[ind].invalidate();
    }
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
#if 0
 // the page may be in index log, remove it! (root of index in a cursor log)

  t_index_uad ** piuad = &index_uad;
  while (*piuad && (*piuad)->)

  { if (index_uad->write_locked && cdp)
      page_unlock(cdp, index_uad->root, RLOCK);
    t_index_uad * next = index_uad->next;
    delete index_uad;
    index_uad=next;
  }
#endif
}

void t_log::piece_changed(tfcbnum fcbn, const tpiece * pc)
{//rec_changed(pc->dadr, k32*pc->len32, pc->offs32/pc->len32); -- error
  uns32 * map = rec_changed(fcbn, pc->dadr, k32, pc->offs32);
  if (pc->len32 > 1 && map)
  { int offs = pc->offs32+1, len=pc->len32-1;
    do { *map |= 1 << offs;  len--;  offs++; }
    while (len>0);
  }
}

void t_log::drop_piece_changes(tpiece * pc)  // called when a piece allocated directly is deallocated, prevents writing into it at commit
{ t_logitem itm(CHANGE_TAG, pc->dadr);
  int ind=itm.hashf(tablesize);
  while (htable[ind].nonempty())
  { if (htable[ind].block==pc->dadr && htable[ind].is_change() && htable[ind].chngmap && htable[ind].chngmap!=FULL_CHNGMAP && htable[ind].chngmap!=INDEX_CHNGMAP) 
    { uns32 * map = htable[ind].chngmap->map;
      if (map)
      { int offs, len;
        for (len=pc->len32, offs=pc->offs32;  len>0;  len--, offs++)
          *map &= ~(1 << offs);
      }
    }
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
}

void t_log::piece_allocated(tblocknum dadr, int len32, int offs32)
{ redo:
    t_logitem itm(TAG_ALLOC_PIECE, dadr, offs32, len32);
    int ind=itm.hashf(tablesize), inv=-1;
    while (htable[ind].nonempty())
    { 
#ifdef STOP // impossible because direct deallocation is disabled
      if (htable[ind].block==dadr && htable[ind].act.len32==len32 && htable[ind].act.offs32==offs32 &&
          htable[ind].act.action_tag==ACT_FREE_PIECE && !any_sapos)
        { htable[ind].invalidate();  return; } // freed before
      else 
#endif
      if (htable[ind].invalid()) { inv=ind;  break; }
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
    if (inv!=-1) ind=inv;
    else
    { if (contains >= limitsize) { grow_table();  if (!error) goto redo;  else return; }
      contains++;  
    }
   // write at ind position: 
    htable[ind]=itm;  
}

BOOL t_log::piece_deallocated(tblocknum dadr, int len32, int offs32)
{ redo:
    t_logitem itm(TAG_FREE_PIECE, dadr, offs32, len32);
    int ind=itm.hashf(tablesize), inv=-1;
    while (htable[ind].nonempty())
    { 
#ifdef STOP  // Direct deallocation disabled because my writing to the page is possible
      if (htable[ind].block==dadr && htable[ind].act.len32==len32 && htable[ind].act.offs32==offs32 &&
          htable[ind].act.action_tag==ACT_ALLOC_PIECE && !any_sapos)
        { htable[ind].invalidate();  return TRUE; } // allocated before, deallocate directly
      else 
#endif
      if (htable[ind].invalid()) { inv=ind;  break; }
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
    if (inv!=-1) ind=inv;
    else
    { if (contains >= limitsize) { grow_table();  if (!error) goto redo;  else return FALSE; }
      contains++;  
    }
   // write at ind position: 
    htable[ind]=itm;  
    return FALSE;
}

void t_log::write_all_changes(cdp_t cdp, bool & written)   // called on commit
{ ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
  for (int i=0;  i<tablesize;  i++)
    if (htable[i].is_change())
    { wr_changes(cdp, &htable[i]);
      written=true;
    }  
}

void t_translog_main::write_all_changes(cdp_t cdp, bool & written)   // called on commit
{
 // add index changes to the main log:
  for (t_index_uad * iuad = index_uad;  iuad;  iuad=iuad->next)
    iuad->wr_changes(cdp, &log);
 // write all changes: 
  log.write_all_changes(cdp, written);
}

void t_log::remove_all_changes(cdp_t cdp)  // called on rollback
{ ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
  for (int i=0;  i<tablesize;  i++)
    if (htable[i].is_change())
      remove_changes(cdp, &htable[i], false);
}

inline bool remove_my_tmplocks_if_installed(cdp_t cdp, ttablenum tbnum)
{ bool removed=false;
  table_descr * tbdf = access_installed_table(cdp, tbnum);
  if (tbdf)
  { ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    removed = tbdf->lockbase.remove_my_tmplocks(cdp->applnum_perm);
    unlock_tabdescr(tbdf);
  }  
  return removed;  
}

inline void return_record_to_free_cache(cdp_t cdp, ttablenum tbnum, trecnum recnum)
{ table_descr * tbdf = install_table(cdp, tbnum);
  // The list of free records must have been taken from the basblock in NEW or DELETE.
  // I must not restore the lost list now because the committed state of the table is not compatible with the released record numbers in the transaction log
  if (tbdf!=NULL)
  { if (tbdf->free_rec_cache.owning_free_record_list())  
    { ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
      tbdf->free_rec_cache.store_to_cache(cdp, recnum, tbdf);
    }
    unlock_tabdescr(tbdf);
  }
}

void t_log::commit_allocs(cdp_t cdp)
// Commits allocations & clears the log
// Removes all temporary locks.
{ bool any_lock_opened=false;
  if (contains)
  { for (int ind=0;  ind < tablesize;  ind++)
      if (htable[ind].nonempty())
      { if (htable[ind].tag==TAG_FREE_PIECE)
        { tpiece pc;  pc.dadr=htable[ind].block;
          pc.len32=htable[ind].act.len32;  pc.offs32=htable[ind].act.offs32;
          if (is_trace_enabled_global(TRACE_PIECES)) 
          { char msg[81];
            trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,piece released in commit", pc.dadr, pc.offs32, pc.len32), NOOBJECT);
          }
          disksp.release_piece_safe(cdp, &pc);
        }
        else if (htable[ind].tag==TAG_FREE_PAGE)  
        { disksp.release_block_safe(cdp, htable[ind].block);
          trace_alloc(cdp, htable[ind].block, false, "commit trans dealloc");
        }
        else if (htable[ind].is_change()) // implies valid chngmap
        { if (htable[ind].chngmap!=FULL_CHNGMAP && htable[ind].chngmap!=INDEX_CHNGMAP) 
            corefree(htable[ind].chngmap); 
        }
        else if (htable[ind].tag==LOCK_TAG)
          any_lock_opened |= remove_my_tmplocks_if_installed(cdp, htable[ind].objnum);
        else if (htable[ind].tag==DELREC_TAG) // updates copies of basblocks with inserted records
          return_record_to_free_cache(cdp, htable[ind].objnum, htable[ind].recnum);
        htable[ind].tag=EMPTY_TAG;  // made empty
      }
    contains=0;
  }
  if (tablesize>HTABLE_PRESERVE_LIMIT) minimize();
 // remove temp locks:
  if (has_page_temp_lock) 
  { ProtectedByCriticalSection cs(&pg_lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    any_lock_opened |= pg_lockbase.remove_my_tmplocks(cdp->applnum_perm);  
    has_page_temp_lock=false; 
  }
  if (any_lock_opened)
    lock_opened();
}

void t_translog_main::commit_index_allocs(cdp_t cdp)
{// commit index allocs:
  for (t_index_uad * iuad = index_uad;  iuad;  iuad=iuad->next)
    iuad->commit_allocs(cdp);
 // unlock root and drop index info:
  clear_index_info(cdp);
}

bool t_log::has_change(tblocknum dadr) const
{ t_logitem itm(CHANGE_TAG, dadr);
  int ind=itm.hashf(tablesize);
  while (htable[ind].nonempty())
  { if (htable[ind].block==dadr && htable[ind].is_change()) // implies valid chngmap
      return true;
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
  return false;        
}

bool any_other_change(cdp_t cdp, tblocknum dadr, t_log * log)
// Check if [dadr] is changed in any other log than [log].
{ if (cdp->tl.log.has_change(dadr)) return true;
  t_translog * subtl;
  for (subtl = cdp->subtrans;  subtl;  subtl=subtl->next)
    if (&subtl->log!=log) 
      if (subtl->log.has_change(dadr)) return true;
  return false;
}

void t_log::destroying_subtrans(cdp_t cdp)
{ 
  if (contains)
  { for (int ind=0;  ind < tablesize;  ind++)
      if (htable[ind].nonempty())
      { if (htable[ind].tag==TAG_FREE_PIECE)
        { tpiece pc;  pc.dadr=htable[ind].block;
          pc.len32=htable[ind].act.len32;  pc.offs32=htable[ind].act.offs32;
          if (is_trace_enabled_global(TRACE_PIECES)) 
          { char msg[81];
            trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,piece released in destroying subtrans", pc.dadr, pc.offs32, pc.len32), NOOBJECT);
          }
          release_piece(cdp, &pc, true);
        }
        else if (htable[ind].tag==TAG_FREE_PAGE)  
        { disksp.release_block_direct(cdp, htable[ind].block);
          trace_alloc(cdp, htable[ind].block, false, "destroy subtrans dealloc");
        }
        else if (htable[ind].is_change()) // implies valid chngmap, includes unchanged private pages
        { ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
#if 0  // cannot call remove_changes if there are any changes from the same client
          remove_changes(cdp, &htable[ind]);
#else
          if (htable[ind].chngmap==FULL_CHNGMAP || htable[ind].chngmap==INDEX_CHNGMAP || !any_other_change(cdp, htable[ind].block, this))
            remove_changes(cdp, &htable[ind], false);
#endif
          if (htable[ind].chngmap!=FULL_CHNGMAP && htable[ind].chngmap!=INDEX_CHNGMAP) corefree(htable[ind].chngmap); 
        }
      }
    contains=0;
  }
}

t_translog_loctab::t_translog_loctab(cdp_t cdp) : t_translog(TL_LOCTAB)
// Inits the new loctab translog and inserts it into the list
{ next=cdp->subtrans;  cdp->subtrans=this;
  tbdf=NULL;
  table_comitted=false;
}

t_translog_cursor::t_translog_cursor(cdp_t cdp) : t_translog(TL_CURSOR)
// Inits the new cursor translog by inserting it into the list
{ next=cdp->subtrans;  cdp->subtrans=this;}

//////////////////////////////////////// transaction objects ////////////////////////////////
void t_trans_obj::add_to_list(cdp_t cdp)
{ next=cdp->tl.trans_objs;  cdp->tl.trans_objs=this; }

// Registered cursors are those which have been created in this transaction and not closed in it.
// On rollback they must be dropped because they would be not returned to the client!
void register_cursor_creation(cdp_t cdp, cur_descr * cd)
{ t_trans_cursor * toc = new t_trans_cursor;
  if (!toc) return;
  toc->cd=cd;
  toc->add_to_list(cdp);
}

void unregister_cursor_creation(cdp_t cdp, cur_descr * cd)
/* Removes the records about creating the cursor from the transaction log. Called when:
1. creation has not been successfully completed
2. the cursor is closed
3. the cursor is owned by somebody who will close it even on error 
The function can be called twice for the same cursor (reason 3. and then 2.)
*/
{ t_trans_obj ** pto = &cdp->tl.trans_objs;
  while (*pto)
  { if ((*pto)->trans_obj_type==TRANS_OBJ_CURSOR)
    { t_trans_cursor * toc = (t_trans_cursor*)*pto;
      if (toc->cd==cd)
        { *pto=toc->next;  delete toc;  return; }
    }
    pto=&(*pto)->next;
  }
}

void register_table_creation(cdp_t cdp, ttablenum tbnum)
{ t_trans_table * tot = new t_trans_table;
  if (!tot) return;
  tot->tbnum=tbnum;
  tot->add_to_list(cdp);
}

void unregister_table_creation(cdp_t cdp, ttablenum tbnum)
{ t_trans_obj ** pto = &cdp->tl.trans_objs;
  while (*pto)
  { if ((*pto)->trans_obj_type==TRANS_OBJ_TABLE)
    { t_trans_table * tot = (t_trans_table*)*pto;
      if (tot->tbnum==tbnum)
        { *pto=tot->next;  delete tot;  return; }
    }
    pto=&(*pto)->next;
  }
}

void register_basic_allocation(cdp_t cdp, tblocknum basblockn, wbbool multpage, uns16 rectopage, int indxcnt)
{ t_trans_basbl * tob = new t_trans_basbl(basblockn, multpage, rectopage, indxcnt, true);
  if (!tob) return;
  tob->add_to_list(cdp);
}

void register_basic_deallocation(cdp_t cdp, tblocknum basblockn, wbbool multpage, uns16 rectopage, int indxcnt)
{ t_trans_basbl * tob = new t_trans_basbl(basblockn, multpage, rectopage, indxcnt, false);
  if (!tob) return;
  tob->add_to_list(cdp);
}

#ifdef STOP
void register_nontrans_index_drop(cdp_t cdp, tblocknum root, unsigned bigstep)
{ t_trans_index * toi = new t_trans_index(root, bigstep);
  if (!toi) return;
  toi->add_to_list(cdp);
}
#endif

void t_translog::drop_transaction_objects(void)
// Deletes the list of transaction objects in the transaction log.
{ while (trans_objs)
  { t_trans_obj * to = trans_objs;  
    trans_objs=to->next;
    delete to;
  }
}

void t_translog::destruct(void)  // may be called twice (2nd time in the destructor)
{ log.minimize();
  destruct_changes();  // deletes the list of deferred changes
  drop_transaction_objects();
}

#ifdef STOP
void rollback_transaction_objects_pre(cdp_t cdp)
// Musi se provest pred remove_all_changes, protoze kdyz pri ruseni kurzoru rusim docasne tabulky,
// tak musim videt ten obsah zaznamu, ktery jsem do nich zapsal (kvuli dealokaci objektu promenne velikosti).
{// close data in cursors:
  for (t_trans_obj * to = cdp->trans_objs;  to;  to=to->next)
    switch (to->trans_obj_type)
    { case TRANS_OBJ_CURSOR:
      { t_trans_cursor * toc = (t_trans_cursor*)to;
        if (toc->cd->opt) toc->cd->opt->close_data(cdp);
        toc->cd->complete=TRUE;
        toc->cd->recnum=0;
        break;
      }
    }
}
#endif

void t_translog::rollback_transaction_objects_post(cdp_t cdp)
// Must be called AFTER rolling back changes and allocations.
// If dropping index has been rolled back, must drop it again.
{ t_trans_obj * to;
 // uninstalling table writes to the record cache, table basblock must exist
  for (to = trans_objs;  to;  to=to->next)
    switch (to->trans_obj_type)
    { case TRANS_OBJ_TABLE: // table may be used during rollback_allocs for returning records to the cache
      { t_trans_table * tot = (t_trans_table*)to;
        force_uninst_table(cdp, tot->tbnum);
        break;
      }
    }
  for (to = trans_objs;  to;  to=to->next)
    switch (to->trans_obj_type)
    { 
#ifdef STOP
      case TRANS_OBJ_INDEX:
      { t_trans_index * toi = (t_trans_index *)to; 
        drop_index(cdp, toi->root, toi->bigstep, TRUE);  // now dropping non-transactionally!
        break;
      }
#endif
      case TRANS_OBJ_BASBL: // the table must not be installed!!
      // for permanent table only
      { t_trans_basbl * tob = (t_trans_basbl *)to; 
        if (tob->created) // table created
        {// drop data blocks, b2-blocks, free rec lists:
          drop_basic_table_allocation(cdp, tob->basblockn, tob->multpage, tob->rectopage, 0, NULL);
         // drop index roots allocated non-trans when the table was created:
          { t_independent_block bas_acc(cdp);
            bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tob->basblockn);
            if (basbl) 
              for (unsigned i=0;  i<tob->indxcnt;  i++)
                free_alpha_block(cdp, basbl->indroot[i], NULL);  
          }
         // drop basblock:
          free_alpha_block(cdp, tob->basblockn, NULL);
        }
        break;
      }
      case TRANS_OBJ_CURSOR:  // added, dropping cursors created but not returned to the client
      { t_trans_cursor * toc = (t_trans_cursor*)to;
        tcursnum crnm;
        { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
          for (crnm=0;  crnm<crs.count();  crnm++)
            if (*crs.acc0(crnm) == toc->cd)
            { // not calling free_cursor(cdp, crnm); because it updates the list of trans_obj and deletes [to].
              { t_temporary_current_cursor tcc(cdp, toc->cd);  // sets the temporary current_cursor in the cdp until destructed
                if (toc->cd->opt!=NULL) toc->cd->opt->close_data(cdp);
              }
              *crs.acc0(crnm) = NULL;
              delete toc->cd;
              cdp->open_cursors--;
              break; 
            }
        }
        break;
      }
    }
  drop_transaction_objects();
}

void t_translog::commit_transaction_objects_post(cdp_t cdp)
// Must be called AFTER committing changes and allocations.
{ for (t_trans_obj * to = trans_objs;  to;  to=to->next)
    switch (to->trans_obj_type)
    { case TRANS_OBJ_BASBL:
      { t_trans_basbl * tob = (t_trans_basbl *)to; 
        if (!tob->created)  // table dropped, indices dropped independently
        { drop_basic_table_allocation(cdp, tob->basblockn, tob->multpage, tob->rectopage, 0, &cdp->tl);
          free_alpha_block(cdp, tob->basblockn, NULL);
        }
        break;
      }
    }
  drop_transaction_objects();
}

void t_log::rollback_allocs(cdp_t cdp)
// Rollbacks allocations & clears the log
// Removes all temporary locks.
// Must be called after removing changes in pages
{ bool any_lock_opened=false;
  if (contains)
  { for (int ind=0;  ind < tablesize;  ind++)
      if (htable[ind].nonempty())
      { if (htable[ind].tag==TAG_ALLOC_PIECE)
        { tpiece pc;  pc.dadr=htable[ind].block;
          pc.len32=htable[ind].act.len32;  pc.offs32=htable[ind].act.offs32;
          if (is_trace_enabled_global(TRACE_PIECES)) 
          { char msg[81];
            trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,piece released in rollback", pc.dadr, pc.offs32, pc.len32), NOOBJECT);
          }
          release_piece(cdp, &pc, true);
        }
        else if (htable[ind].tag==TAG_ALLOC_PAGE)  
        { disksp.release_block_direct(cdp, htable[ind].block);
          trace_alloc(cdp, htable[ind].block, false, "rollback alloc");
        }
        else if (htable[ind].is_change()) // implies valid chngmap
        { if (htable[ind].chngmap!=FULL_CHNGMAP && htable[ind].chngmap!=INDEX_CHNGMAP) 
            corefree(htable[ind].chngmap); 
        }
        else if (htable[ind].tag==LOCK_TAG)
          any_lock_opened |= remove_my_tmplocks_if_installed(cdp, htable[ind].objnum);
        else if (htable[ind].tag==INSREC_TAG) // updates copies of basblocks with inserted records
          return_record_to_free_cache(cdp, htable[ind].objnum, htable[ind].recnum);
        htable[ind].tag=EMPTY_TAG;  // made empty
      }
    contains=0;
  }
  if (tablesize>HTABLE_PRESERVE_LIMIT) minimize();
 // remove temp. locks:
  if (has_page_temp_lock) 
  { ProtectedByCriticalSection cs(&pg_lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    any_lock_opened |= pg_lockbase.remove_my_tmplocks(cdp->applnum_perm);  
    has_page_temp_lock=false; 
  }
  if (any_lock_opened)
    lock_opened();
}

///////////////////////////////// Savepoints //////////////////////////////////
int t_savepoints::find_sapo(const char * name, int num)
// Returns savepoint index or -1 if not found
{ if (*name || num) // must not found when searching for unnamed sapo with 0 num
    for (int i=0;  i<sapos.count();  i++)
      if (*name ?
          !strcmp(name, sapos.acc0(i)->name) :  // identified by name
          num == sapos.acc0(i)->num)            // identified by number
        return i;
  return -1;
}

int t_savepoints::find_atomic_index(void)
// Returns index of the last atomic savepoint, or -1 iff none found
{ for (int i=sapos.count()-1;  i>=0;  i--)
    if (sapos.acc0(i)->atomic) return i;
  return -1;
}

int t_savepoints::get_number(void)
// Allocates number from <0..0x7fff> diffrerent from numbers of active sapos
// Sapo numbers can be stored in Smallint.
// Savepoint count limit cheched before, no error can occur.
{ int num;  BOOL conflict;
  do
  { if (++counter > 0x7fff) counter=1;
    num=counter;
   // check for conflict with the number of any other active savepoint:
    conflict=FALSE;
    for (int i=0;  i<sapos.count();  i++)
      if (num == sapos.acc0(i)->num) { conflict=TRUE;  break; }
  } while (conflict);
  return num;
}

void t_savepoints::destroy(cdp_t cdp, int ind, BOOL all)
// Destroys the savepoint with index ind, destroys all following iff "all".
// ind may not be a valid index (when rolling back to a savepoint
//  and deleting the following savepoints)!
{ do
  { if (ind >= sapos.count()) break;
    t_savepoint * sapo = sapos.acc0(ind);
    for (int i=0;  i<sapo->pairscnt;  i++)
      if (sapo->pairs[i].act.sapo_oper==(uns16)CHANGE_TAG)
        transactor.free_swap_block(sapo->pairs[i].tra_dadr);
    sapo->t_savepoint::~t_savepoint();
    sapos.delet(ind);
  } while (all);
  cdp->tl.log.any_sapos=sapos.count() > 0;
}

BOOL t_savepoints::create(cdp_t cdp, const char * name, int & num, BOOL atomic)
// Creates named sapo iff *name, uses given number iff num!=0,
// allocates & returns sapo number otherwise.
{// alocate the savepoint:
  if (cdp->savepoints.sapos.count() >= 1000)
    { request_error(cdp, SQ_SAVEPOINT_TOO_MANY);  return TRUE; }
  t_savepoint * sapo = cdp->savepoints.sapos.next();
  if (sapo==NULL)
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);   return TRUE; }
 // sapo_num: 0 iff named, given iff num!=0, allocate number otherwise:
  strcpy(sapo->name, name);
  sapo->num = *name ? 0 : num ? num : (num=get_number());
  sapo->atomic=atomic;
 // create the savepoint contents - count blocks and allocate decriptor:
  int i;
  sapo->pairscnt=0;
  for (i=0;  i < cdp->tl.log.tablesize;  i++)
    if (cdp->tl.log.htable[i].is_change() || cdp->tl.log.htable[i].is_alloc())
      sapo->pairscnt++;
  if (sapo->pairscnt)
  { sapo->pairs=new dadrpair[sapo->pairscnt];
    if (sapo->pairs==NULL)
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err1; }
   // write dirty blocks:
    dadrpair * sapoitem = sapo->pairs;  
    tfcbnum fcbn;  tptr dt;  tblocknum tblock;
    for (i=0;  i < cdp->tl.log.tablesize;  i++)
      if (cdp->tl.log.htable[i].is_change() || cdp->tl.log.htable[i].is_alloc())
      { t_logitem * logitem = &cdp->tl.log.htable[i];
        sapoitem->fil_dadr=logitem->block;
        if (logitem->is_change()) // changed page
        { sapoitem->act.sapo_oper=(uns16)CHANGE_TAG;  
          dt=fix_priv_page(cdp, logitem->block, &fcbn, &cdp->tl);
          if (dt==NULL) goto err2;
          tblock = transactor.save_block(dt);
          unfix_page(cdp, fcbn);
          if (!tblock) { request_error(cdp, OS_FILE_ERROR);  goto err2; }
          sapoitem->tra_dadr=tblock;
        }
        else   // allocation/deallocation
        { sapoitem->act.len32    =logitem->act.len32;
          sapoitem->act.offs32   =logitem->act.offs32;
          sapoitem->act.sapo_oper=(uns16)logitem->tag;
        }
        sapoitem++;
      } // is_block
  } // any log info
  cdp->tl.log.any_sapos=TRUE;
  return FALSE;

 err2:
  delete [] sapo->pairs;
 err1:
  cdp->savepoints.sapos.delet(cdp->savepoints.sapos.count()-1);
  cdp->tl.log.any_sapos=sapos.count() > 0;
  return TRUE;
}

void t_savepoints::rollback(cdp_t cdp, int ind)
// ind is the index of an existing savepoint.
// The savepoint is NOT destroyed.
// The list of changes in the page is not reduced (unless all changes removed) -> no harm
{ t_savepoint * sapo = cdp->savepoints.sapos.acc0(ind);
  int i;
  for (i=0;  i < cdp->tl.log.tablesize;  i++)
    if (cdp->tl.log.htable[i].is_change() || cdp->tl.log.htable[i].is_alloc())
    { t_logitem * logitem = &cdp->tl.log.htable[i];
     // search logitem in sapo description:
      dadrpair * sapoitem;  int j;
      for (j=0, sapoitem = sapo->pairs;  j<sapo->pairscnt;  j++, sapoitem++)
        if (sapoitem->fil_dadr==logitem->block)
           if (sapoitem->act.sapo_oper==(uns16)CHANGE_TAG)
           { if (logitem->is_change()) break; }
           else
           { if (sapoitem->act.len32    ==logitem->act.len32  &&
                 sapoitem->act.offs32   ==logitem->act.offs32 &&
                 sapoitem->act.sapo_oper==(uns16)logitem->tag)
               break;
           }
     // restore the situation:
      if (j<sapo->pairscnt)  // logitem found in sapo
      { if (logitem->is_change() && logitem->chngmap!=NULL)  // implies valid chngmap
        { tfcbnum fcbn;  tptr dt;
          dt=fix_priv_page(cdp, logitem->block, &fcbn, &cdp->tl);
          if (dt!=NULL)
          { tptr cpy=(tptr)corealloc(BLOCKSIZE, 48);
            if (cpy!=NULL)
            { transactor.load_block(cpy, sapoitem->tra_dadr);
              if (logitem->chngmap==FULL_CHNGMAP || logitem->chngmap==INDEX_CHNGMAP) 
                memcpy(dt, cpy, BLOCKSIZE);
              else 
              { int pos, off, poscnt = BLOCKSIZE/logitem->chngmap->recsize;
                for (pos=0;  pos<poscnt;  pos++)
                  if (!logitem->chngmap->map[pos / 32]) pos+=31;
                  else if (logitem->chngmap->map[pos / 32] & (1<<(pos % 32)))
                  { off=logitem->chngmap->recsize*pos;
                    memcpy(dt+off, cpy+off, logitem->chngmap->recsize);
                  }
              }
              corefree(cpy);
            }
            unfix_page(cdp, fcbn);
          }
        }
      }
      else // page not found, was not changed when sapo has been created
      { if (logitem->is_change()) // implies valid chngmap
        { ProtectedByCriticalSection cs(&cs_frame, cdp, WAIT_CS_FRAME);
          remove_changes(cdp, logitem, false);  
          if (logitem->chngmap!=FULL_CHNGMAP && logitem->chngmap!=INDEX_CHNGMAP) 
            corefree(logitem->chngmap);
        }
        else  // remove the allocation/deallocation
        { if (logitem->tag==TAG_ALLOC_PIECE)
          { tpiece pc;  pc.dadr=logitem->block;
            pc.len32=logitem->act.len32;  pc.offs32=logitem->act.offs32;
            if (is_trace_enabled_global(TRACE_PIECES)) 
            { char msg[81];
              trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,piece released in savepoint rollback", pc.dadr, pc.offs32, pc.len32), NOOBJECT);
            }
            release_piece(cdp, &pc, true);
          }
          else if (logitem->tag==TAG_ALLOC_PAGE)
            disksp.release_block_direct(cdp, logitem->block);
        }
        logitem->invalidate(); 
      }
    }
}

/********************************** locks ***********************************/
/* the page structure must be the same as for records, because the subroutine for locking is the same */
/* Problem with the following scenario:
Client 1 locks X
Client 2 wants to lock X and finds the conflicting lock
Client 1 unlocks X and pulses the event
Clietn 2 starts waiting for the event -> and waits infilitely.
Temporarty solution: waiting for the event with timeout (100 ms).
*/
#define LOCKING_WITH_EVENT

#ifdef LOCKING_WITH_EVENT
#ifdef LINUX
void ms2abs(uns32 miliseconds, timespec & ts);
#endif
#endif

void lock_opened(void)
{
#ifdef LOCKING_WITH_EVENT
#ifdef WINS
  PulseEvent(hLockOpenEvent);
#else
  int res=pthread_mutex_lock(&hLockOpenEvent.mutex);
  pthread_cond_broadcast(&hLockOpenEvent.cond); 
  pthread_mutex_unlock(&hLockOpenEvent.mutex);
#endif
#endif
}

bool wait_for_lock_opened(cdp_t cdp)  // cdp contains the time_limit
{
#ifdef LOCKING_WITH_EVENT
#ifdef WINS
   HANDLE hnds[2] = { hLockOpenEvent, hKernelShutdownEvent };
   DWORD curr_time=GetTickCount();
   DWORD res = WaitForMultipleObjects(2, hnds, FALSE, /*!cdp->time_limit ? INFINITE :*/ 100);
   return res==WAIT_OBJECT_0 || res==WAIT_TIMEOUT;
#else
   int res;
   pthread_mutex_lock(&hLockOpenEvent.mutex);
   if (!cdp->time_limit) res=pthread_cond_wait(&hLockOpenEvent.cond, &hLockOpenEvent.mutex);
   else 
   { struct timespec abstime;
	   ms2abs(/*cdp->time_limit>curr_time ? cdp->time_limit-curr_time : 0*/100, abstime);
     res=pthread_cond_timedwait(&hLockOpenEvent.cond, &hLockOpenEvent.mutex, &abstime);
   }
   pthread_mutex_unlock(&hLockOpenEvent.mutex);  
   return true;
#endif
#else
  Sleep(40);
#endif
}

t_locktable::t_locktable(void)
{ table_size=valid_cells=used_cells=0;
  locks=NULL;
  InitializeCriticalSection(&cs_locks);
  cs_is_valid=true;
}

t_locktable::~t_locktable(void)
{ if (locks)
    { corefree(locks);  locks=NULL; }
  if (cs_is_valid) 
    { DeleteCriticalSection(&cs_locks);  cs_is_valid=false; }
}

bool t_locktable::resize(unsigned table_sizeIn)
{ t_lock_cell * old_locks = locks;
  locks = (t_lock_cell *)corealloc(table_sizeIn*sizeof(t_lock_cell), 51);
  if (!locks)
    { locks=old_locks;  return false; }
  unsigned prev_table_size = table_size;
  bool has_old_items = valid_cells > 0;
  table_size=table_sizeIn;  valid_cells=used_cells=0;  // must be initialized before insering the data
  for (unsigned i=0;  i<table_size;  i++) locks[i].set_empty();
  if (has_old_items)
  { unsigned j;  t_lock_cell * plock;
    for (j=0, plock=old_locks;  j<prev_table_size;  j++, plock++)
      if (plock->is_valid())
        insert(plock->object, plock->owner, plock->locktype);
  }
  if (old_locks) corefree(old_locks);
  return true;
}

void t_locktable::insert(trecnum object, t_client_number owner, uns8 locktype)
{ unsigned ind = hash(object);
  while (locks[ind].is_valid())
    if (++ind>=table_size) ind=0;
  valid_cells++;
  if (locks[ind].is_empty()) used_cells++;  // must be before the object member is changed
  locks[ind].object=object;
  locks[ind].owner=owner;
  locks[ind].locktype=locktype;
}

/*inline */int t_locktable::try_locking(cdp_t cdp, bool lock_it, table_descr * tbdf, trecnum record, uns8 lock_type)
// Returns 0 if already locked, -1 if must add the lock, >0 if cannot add (returns blocking client number + 1)
{ unsigned ind;  t_lock_cell * lp;
  if (any_lock()) 
  {// first, looking for FULLTABLE lock:
    for (find_first_cell(FULLTABLE, ind, lp);  !lp->is_empty();  find_next_cell(ind, lp))
    { if (lp->object==FULLTABLE)  // implies lp->valid()
        if (lp->owner!=cdp->applnum_perm)
        { if (!(lp->locktype & (RLOCK | TMPRLOCK)) || !(lock_type & (RLOCK | TMPRLOCK)))
            return lp->owner+1;  // cannot lock, unless both existing and new locks are read locks
        }
        else /* my lock, no conflict, check if the contained */
        { if (lp->object==FULLTABLE)
            if (lock_type==TMPWLOCK && (lp->locktype & (WLOCK | TMPWLOCK)) ||
                lock_type==TMPRLOCK && (lp->locktype == TMPRLOCK) )
              return 0;   /* nothing to be done */
        }
    }
   // next part is depends on [record]:
    if (record==FULLTABLE)  // locking all records, check if there is any preventing lock
    { pre_first_cell(ind, lp);
      while (next_cell(ind, lp))
        if (lp->object!=FULLTABLE)  // FULLTABLE locks have been checkek above
          if (lp->owner!=cdp->applnum_perm)
            if (!(lp->locktype & (RLOCK | TMPRLOCK)) || !(lock_type & (RLOCK | TMPRLOCK)))
              return lp->owner+1;  // cannot lock, unless both existing and new locks are read locks
    }
    else  // looking for lock on a specific record
    { for (find_first_cell(record, ind, lp);  !lp->is_empty();  find_next_cell(ind, lp))
      { if (lp->object==record)  // implies lp->valid()
          if (lp->owner!=cdp->applnum_perm)
          { if (lp->locktype!=RLOCK && lp->locktype!=TMPRLOCK || lock_type!=RLOCK && lock_type!=TMPRLOCK)
              return lp->owner+1;  // cannot lock, unless both existing and new locks are read locks
          }
          else // my lock
          {// test for special conflict on TABTAB/OBJTAB - for version 9
           // (prevents multiple modifying of an object or modifying a table when a form or cursor using it is open):
            if (tbdf)
              if (tbdf->tbnum==TAB_TABLENUM || tbdf->tbnum==OBJ_TABLENUM)
                if (lock_type==WLOCK && (lp->locktype & RLOCK) || lock_type==RLOCK && (lp->locktype & WLOCK))
                // Must not disable WLOCKxWLOCK because designers set WLOCK and the call ALTER
                  return lp->owner+1;
           // test containment:
            if (lock_type==TMPWLOCK && (lp->locktype & (WLOCK | TMPWLOCK)) ||
                lock_type==TMPRLOCK && (lp->locktype == TMPRLOCK) )
              return 0;   /* nothing to be done */
          }
      }
    }
  }
  return -1;
}

int who_locked(cdp_t cdp, table_descr * tbdf, trecnum record, bool write_lock)
{ t_locktable * lb;  
  if (tbdf==NULL) lb=&pg_lockbase;  else lb = &tbdf->lockbase;
  ProtectedByCriticalSection cs(&lb->cs_locks, cdp, WAIT_CS_LOCKS);
  return lb->try_locking(cdp, false, tbdf, record, write_lock ? WLOCK : RLOCK);
}

bool t_locktable::do_I_have_lock(cdp_t cdp, trecnum record, uns8 lock_type)
{ unsigned ind;  t_lock_cell * lp;
  ProtectedByCriticalSection cs(&cs_locks, cdp, WAIT_CS_LOCKS);
  if (!any_lock()) return false;
  for (find_first_cell(record, ind, lp);  !lp->is_empty();  find_next_cell(ind, lp))
    if (lp->object==record)  // implies lp->valid()
      if (lp->owner==cdp->applnum_perm)
        if (lp->locktype & lock_type)
          return true;
  return false;
}
  
inline int t_locktable::add_lock(cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type)
// Returns 0 if locked, -1 on memory error, blocking applnum_perm+1 otherwise
{ 
  int result = try_locking(cdp, true, tbdf, record, lock_type);
  if (result==-1)  // can and must add the lock
    if (!no_space_for_new_lock() || resize(bigger_table_size()))
    { insert(record, /*1,*/ cdp->applnum_perm, lock_type);
      result=0;  // ok
    }
  return result;
}

int t_translog_main::lock_all_index_roots(cdp_t cdp, bool locking)
// Locks or unlocks all roots in the list. 
// Returns 0 if ok, -1 on memory error, applnum_perm+1 when applnum has interfering lock.
// Must be called inside pg_lockbase.cs_locks!
// TMPWLOCK must be used, not the WLOCK, because WLOCK can be removed only by commit (is converted to TMPWLOCK) 
// When cannot lock the root because of interfering lock, sets cdp->index_owner.
{ int res;  bool opened = false;
  for (t_index_uad * iuad = index_uad;  iuad;  iuad=iuad->next)
    if (locking)
    { res=pg_lockbase.add_lock(cdp, NULL, (trecnum)iuad->root, TMPWLOCK);
      if (!res)
        iuad->write_locked=true;
      else 
      { cdp->index_owner = iuad->tabnum;
        return res;
      }  
    }
    else
      if (iuad->write_locked)
      { pg_lockbase.remove_lock(cdp, iuad->root, TMPWLOCK, NULL);
        iuad->write_locked=false;
        opened=true;
      }
  if (opened) lock_opened();  // signal for thread waiting for any lock
  return 0;
}

bool lock_all_index_roots(cdp_t cdp)
// Locks the roots of all changed shared indexes atomically with TMPWLOCK. Waits indefinitely. Returns false on server shutdown.
// The [objnum] in the profile is not excact: it may wait for indexes in multiple tables and the set may change during the wait.
{ bool success, waited;  prof_time start;
  start = get_prof_time();
  cdp->index_owner=NOOBJECT;
  waited=false;
  do
  {// atomic locking:
    { ProtectedByCriticalSection cs(&pg_lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
      int res = cdp->tl.lock_all_index_roots(cdp, true); 
      success = res==0;
      if (!success)  // failed, unlocking:
        cdp->tl.lock_all_index_roots(cdp, false); 
    }
    if (success) 
    { cdp->tl.log.has_page_temp_lock=true;
      break;
    }  
   // I must wait for the lock: 
    cdp->ilock_cycles++;
    cdp->wait_type = WAIT_MIWLOCK;
    if (PROFILING_ON(cdp)) 
    { waited=true;
      add_hit_and_time(0, PROFCLASS_ACTIVITY, PROFACT_INDEX_LOCK_RETRY, 0, NULL);
    }  
  } while (wait_for_lock_opened(cdp));  // until shutdown
  cdp->wait_type = 0;
  if (waited) 
  { prof_time waited_time = get_prof_time()-start;  // used in the full protocol
    if (PROFILING_ON(cdp))
      add_hit_and_time(waited_time, PROFCLASS_PAGE_LOCK, cdp->index_owner, 0, NULL);
    cdp->lock_waiting_time += waited_time;
  }
  return success;
}

int record_lock(cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type)
// Locks a page iff tbdf==NULL.
// Returns 0 if locked, -1 on memory error, blocking applnum_perm+1 otherwise
/* Records and tables are locked independently */
/* Write locks & read locks for the some object & owner may & must be created independently */
/* The same application can set both read and write lock on the same object */
{ t_locktable * lb;  
  if (tbdf==NULL) lb=&pg_lockbase;
  else
  { if (tbdf->tbnum < 0) return 0;   // private temporary table, not locking anything
    lb = &tbdf->lockbase;
  }
  EnterCriticalSection(&lb->cs_locks);  // cannot use "ProtectedBy" because lb may change!
  int result = lb->add_lock(cdp, tbdf, record, lock_type);
 // store info about a temporarty lock in the transaction log:
  if (!result)
    if (lock_type==TMPWLOCK || lock_type==TMPRLOCK)
      if (tbdf) cdp->tl.log.register_object(tbdf->tbnum, LOCK_TAG);  else cdp->tl.log.has_page_temp_lock=true;
  LeaveCriticalSection(&lb->cs_locks);
  if (result==-1) request_error(cdp, OUT_OF_KERNEL_MEMORY);  // ouside CS
  return result;
}

BOOL t_locktable::remove_lock(cdp_t cdp, trecnum record, uns8 lock_type, table_descr * tbdf)
{ unsigned ind;  t_lock_cell * lp;  BOOL result=TRUE;
  if (any_lock()) // may crash if locks==NULL
    for (find_first_cell(record, ind, lp);  !lp->is_empty();  find_next_cell(ind, lp))
    { if (lp->object==record  // implies lp->valid()
          && lp->owner==cdp->applnum_perm && (lp->locktype==lock_type || lock_type==ANY_LOCK))  /* lock found */
      { if (lp->locktype==WLOCK)  // will be changed into TMPWLOCK, must register it
          if (tbdf) cdp->tl.log.register_object(tbdf->tbnum, LOCK_TAG);  else cdp->tl.log.has_page_temp_lock=true;
        if (lock_type!=WLOCK) { lp->invalidate();  cell_invalidated(); }
        else lp->locktype=TMPWLOCK;  // replace WLOCK by TMPWLOCK
        result=FALSE;  break;
      }
    }
  return result;  
}

BOOL record_unlock(cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type)
/* table is supposed to have table_descr locked. */
{ t_locktable * lb;  BOOL result;
  if (tbdf==NULL) lb=&pg_lockbase;
  else
  { if (tbdf->tbnum < 0) return 0;   // private temporary table, not locking
    lb = &tbdf->lockbase;
  }
  { ProtectedByCriticalSection cs(&lb->cs_locks);  
    result = lb->remove_lock(cdp, record, lock_type, tbdf);
  }
  lock_opened();
  return result;  /* TRUE when lock not found */
}

BOOL is_all_locked(cdp_t cdp, table_descr * tbdf)
/* tbdf is supposed to be locked. Used by tb_new. */
{ BOOL result=FALSE;
  t_locktable * lb = &tbdf->lockbase;
  ProtectedByCriticalSection cs(&lb->cs_locks, cdp, WAIT_CS_LOCKS);
  unsigned ind;  t_lock_cell * lp;
  if (lb->any_lock()) // may crash if locks==NULL
    for (lb->find_first_cell(FULLTABLE, ind, lp);  !lp->is_empty();  lb->find_next_cell(ind, lp))
      if (lp->object==FULLTABLE)  // implies lp->valid()
        if (lp->owner!=cdp->applnum_perm) { result=TRUE;  break; }
  return result;
}

unsigned page_locks_count(void)
{ return pg_lockbase.valid_cells; }

BOOL can_uninstall(cdp_t cdp, table_descr * tbdf)
/* Must be called inside cs_tables! Must be (and is) called only for tb>=0! */
{ BOOL result=TRUE;
  if (tbdf==NULL) return FALSE;           /* table not installed */
  if (SYSTEM_TABLE(tbdf->tbnum)) return FALSE;
  if (tbdf->deflock_counter) return FALSE;  /* table_descr is in use! */
 /* locks of other clients would be destroyed: check for them */
  { t_locktable * lb = &tbdf->lockbase;
    ProtectedByCriticalSection cs(&lb->cs_locks, cdp, WAIT_CS_LOCKS);
    unsigned ind;  t_lock_cell * lp;
    if (lb->any_lock()) // may crash if locks==NULL
    { lb->pre_first_cell(ind, lp);
      while (lb->next_cell(ind, lp))
      { if (lp->owner!=cdp->applnum_perm) 
          { result=FALSE;  break; }
        if (lp->locktype!=TMPWLOCK && lp->locktype!=TMPRLOCK)  
          { result=FALSE;  break; }
      } 
    }
  }
  if (!result) return FALSE;

 /* table definition must not be locked (open cursors and views lock it) */
  if (tbdf->tbnum >= 0)
  { t_locktable * lb = &tabtab_descr->lockbase;
    ProtectedByCriticalSection cs(&lb->cs_locks, cdp, WAIT_CS_LOCKS);
    unsigned ind;  t_lock_cell * lp;
    if (lb->any_lock()) // may crash if locks==NULL
      for (lb->find_first_cell(tbdf->tbnum, ind, lp);  !lp->is_empty();  lb->find_next_cell(ind, lp))
        if (lp->object==tbdf->tbnum)  // implies lp->valid()
          { result=FALSE;  break; }
  }
  if (!result) return FALSE;

 /* disables uninstalling when any index disabled */
  dd_index * cc;  int ind;
  for (ind=0, cc=tbdf->indxs;  ind<tbdf->indx_cnt;  ind++, cc++)
    if (cc->disabled) return FALSE;
  return TRUE;
}

bool t_locktable::remove_my_tmplocks(t_client_number applnum_perm)
// Removes all temp. lockes of the specified client. Called when the transaction ends.
{ unsigned ind;  t_lock_cell * lp;  bool any_removed = false;
  if (any_lock()) // may crash if locks==NULL
  { pre_first_cell(ind, lp);
    while (next_cell(ind, lp))
    { if (lp->owner==applnum_perm && (lp->locktype==TMPWLOCK || lp->locktype==TMPRLOCK))    
        { lp->invalidate();  cell_invalidated();  any_removed=true; }
    } 
    if (is_almost_empty()) resize(INITIAL_SIZE);
  }
  return any_removed;
}

void remove_user_rlocks(t_client_number user, t_locktable * lb)
// Removes all user's RLOCKs in lb. Called on pg_lockbase.
{ ProtectedByCriticalSection cs(&lb->cs_locks);
  if (lb->any_lock()) // may crash if locks==NULL
  { unsigned ind;  t_lock_cell * lp;  bool any_removed = false;
    lb->pre_first_cell(ind, lp);
    while (lb->next_cell(ind, lp))
    { if (lp->is_valid() && lp->owner==user && lp->locktype==RLOCK)    
        { lp->invalidate();  lb->cell_invalidated();  any_removed=true; }
    } 
   // signal for the waiting threads: 
    if (any_removed)
      lock_opened();
  }
}

BOOL record_lock_error(cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type)
// Locks the record, returns FALSE iff locked, return TRUE and reports the error otherwise.
{ int owner1=record_lock(cdp, tbdf, record, lock_type);
  if (!owner1) return FALSE;  // locked OK
 // report the error, with context if possible:
  if (owner1!=-1)  // for owner==-1 memory error reported in record_lock
  { tobjname buf;  sprintf(buf, "%d/%d", tbdf->tbnum, record);
    SET_ERR_MSG(cdp, buf);
    if (cds[owner1-1])  // can create the error context
    { t_exkont_lock ekt(cdp, tbdf->tbnum, record, lock_type, cds[owner1-1]->applnum_perm);
      request_error(cdp, NOT_LOCKED);
    }
    else
      request_error(cdp, NOT_LOCKED);
  }
  return TRUE;  // not locked
}

table_descr * get_next_installed_table(cdp_t cdp, ttablenum & tbnum)
// Passes all installed tables from [tbnum]. Returns NULL on the end. 
// Every returned table_descr must be unlocked!
{ ProtectedByCriticalSection cs(&cs_tables, cdp, WAIT_CS_TABLES);
  while (tbnum < tabtab_descr->Recnum())
  { table_descr * tbdf=tables[tbnum];
    tbnum++;  // for the next call
    if (tbdf!=NULL && tbdf!=(table_descr*)-1) // table is installed
    { InterlockedIncrement(&tbdf->deflock_counter);
      return tbdf;
    }  
  }
  return NULL;  // no more installed tables
}

bool t_locktable::unlock_user_tab(t_client_number owner)
// Removes all locks belonging to the [owner]. Returns true if any lock removed. 
// Must be called inside cs_locks.
{ unsigned ind;  t_lock_cell * lp;  bool removed=false;
  if (any_lock())  // otherwise may crash if locks==NULL
  { pre_first_cell(ind, lp);
    while (next_cell(ind, lp))
    { if (lp->owner==owner)    
        { lp->invalidate();  cell_invalidated();  removed=true; }
    } 
  }
  return removed;
}

void unlock_user(cdp_t cdp, t_client_number user, bool include_object_locks)
// Removes all locks belonging to a user. Called when user disconnects and through API.
{ bool removed=false;
  { ProtectedByCriticalSection cs(&pg_lockbase.cs_locks,            cdp, WAIT_CS_LOCKS);
    removed|=           pg_lockbase.unlock_user_tab(user);
  }
  { ProtectedByCriticalSection cs(&tabtab_descr->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    removed|=tabtab_descr->lockbase.unlock_user_tab(user);
  }
  if (include_object_locks)
  { ProtectedByCriticalSection cs(&objtab_descr->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    removed|=objtab_descr->lockbase.unlock_user_tab(user);
  }
 // pass all installed tables: 
  ttablenum tbnum = OBJ_TABLENUM+1;
  do
  { table_descr * tbdf = get_next_installed_table(cdp, tbnum);
    if (!tbdf) break;
    { ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks,       cdp, WAIT_CS_LOCKS);
      removed|=      tbdf->lockbase.unlock_user_tab(user);
      unlock_tabdescr(tbdf);
    }
  } while (true);    
 // signal for the waiting threads: 
  if (removed)
    lock_opened();
}

bool t_locktable::simple_remove_lock(t_client_number owner, trecnum recnum, uns8 lock_type)
// Removes the specified lock(s) belonging to the [owner]. Returns true if any lock removed. 
// Must be called inside cs_locks.
{ unsigned ind;  t_lock_cell * lp;  bool removed=false;
  if (any_lock())  // otherwise may crash if locks==NULL
  { pre_first_cell(ind, lp);
    while (next_cell(ind, lp))
    { if (lp->owner==owner &&
          lp->object==recnum  && // implies lp->valid()
           (lp->locktype==lock_type || lock_type==ANY_LOCK))  /* lock found */
        { lp->invalidate();  cell_invalidated();  removed=true; }
    } 
  }
  return removed;
}

void remove_the_lock(cdp_t cdp, lockinfo * llock)
// Directly removes the specified lock.
{ bool removed = false;
  if (llock->tabnum >= tabtab_descr->Recnum()) return;
  table_descr * tbdf = access_installed_table(cdp, llock->tabnum);
  if (tbdf)
  {// find the application owning the lock and use its [applnum_perm]:
    for (int i=1;  i <= max_task_index;  i++)
    { bool found=false;  t_client_number applnum;
      { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
        if (cds[i] && llock->usernum==cds[i]->prvs.luser())
          { applnum=cds[i]->applnum_perm;  found=true; }
      }
      if (found)  // must not be in [cs_client_list]
      { ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
        removed |= tbdf->lockbase.simple_remove_lock(applnum, llock->recnum, llock->locktype);  
      }  
    }
    unlock_tabdescr(tbdf);
  }
 // signal for the waiting threads: 
  if (removed)
    lock_opened();
}


/********************************* info *************************************/
void get_fixes(unsigned * pages,  unsigned * fixes, unsigned * frames)
{ *pages=*fixes=0;
  ProtectedByCriticalSection cs(&cs_frame);
  for (int j=0;  j<frame_count;  j++)
    if (fcbs[j].fixnum) { (*pages)++;  *fixes+=fcbs[j].fixnum; }
  *frames=frame_count;
}

BOOL check_fcb()
{ int h;  tfcbnum fcbn1, fcbn2;
  for (h=0;  h<FCB_HASH_SIZE;  h++)
  { fcbn1=fcb_hash_table[h];
    while (fcbn1!=NOFCBNUM)
    { fcbn2=fcbs[fcbn1].nextfcbnum;
		while (fcbn2!=NOFCBNUM)
      { if (fcbs[fcbn1].blockdadr)
          if (fcbs[fcbn1].blockdadr==fcbs[fcbn2].blockdadr)
            if (memcmp(FRAMEADR(fcbn1), FRAMEADR(fcbn2), BLOCKSIZE))
              return TRUE;
//            if (fcbs[fcbn1].owner == fcbs[fcbn2].owner)
//              return TRUE;
        fcbn2=fcbs[fcbn2].nextfcbnum;
      }
      fcbn1=fcbs[fcbn1].nextfcbnum;
    }
  }
  return FALSE;
}

#ifdef NEVER
void test_fcb()
{ tfcbnum i, j;
  for (i=0; i<frame_count; i++) if (fcbs[i].blockdadr)
    for (j=0; j<frame_count; j++) if (i!=j) if (fcbs[j].blockdadr)
      if (fcbs[i].blockdadr==fcbs[j].blockdadr)
        dbg_error("DBL PAGE");
}
#endif

#ifdef FIX_CHECKS

static void fix_msg(tfcbnum fcbn)
{ short i=0;
  while (i<MAX_APPL_FIXES)
  { if (curr_appl->fix_info[i].fcbn==NOFCBNUM)
    { curr_appl->fix_info[i].fcbn=fcbn; curr_appl->fix_info[i].fixes=1;
      if (++i<MAX_APPL_FIXES) curr_appl->fix_info[i].fcbn=NOFCBNUM;
      return;
    }
    if (curr_appl->fix_info[i].fcbn==fcbn)
      { curr_appl->fix_info[i].fixes++; return; }
    i++;
  }
}

static void unfix_msg(tfcbnum fcbn)
{ short i=0, j;
  while (i<MAX_APPL_FIXES)
  { if (curr_appl->fix_info[i].fcbn==NOFCBNUM) return;
    if (curr_appl->fix_info[i].fcbn==fcbn)
    { if (curr_appl->fix_info[i].fixes) curr_appl->fix_info[i].fixes--;
      if (!curr_appl->fix_info[i].fixes)  /* free the cell */
      { j=i+1;  /* find the last record */
        while ((j<MAX_APPL_FIXES) && (curr_appl->fix_info[j].fcbn!=NOFCBNUM))
          j++;
       /* now, j points after the last record */
        curr_appl->fix_info[i]=curr_appl->fix_info[j-1];
        curr_appl->fix_info[j-1].fcbn=NOFCBNUM;
      }
      return;
    }
    i++;
  }
}

void unfix_all()
{ while (curr_appl->fix_info[0].fcbn!=NOFCBNUM)
    unfix_page(curr_appl->fix_info[0].fcbn);
}
#endif

/*---------------------------------------------------------------------------
  deadlock detection and handling
  *******************************
*/

/* Other threads can only change [waits_for] from 1 to 2..5 - in cs_deadlock
0 - not waiting
1 - waiting for a lock (cdp->lock_tbdf,cdp->record_or_page,cdp->lock_type) which was not available, any thread
      may get it for me and change [waits_for]
2 - locked       
3 - memory error
4 - waiting cancelled
5 - timed out
*/

void trace_lock_chain(cdp_t cdp, char * initial_msg, char * final_msg)
{ char buf[200];
  trace_msg_general(cdp, TRACE_LOCK_ERROR, form_message(buf, sizeof(buf), initial_msg), NOOBJECT);
  cdp_t acdp = cdp;
  do
  { 
    if (acdp->waits_for!=1)  // this client is not waiting
    { form_message(buf, sizeof(buf), gettext_noop("Client %d(%s) in state %d."), acdp->session_number, acdp->thread_name, acdp->wait_type);
      trace_msg_general(acdp, TRACE_LOCK_ERROR, buf, NOOBJECT);
      break;
    }
   // describe waiting client:
    form_message(buf, sizeof(buf), gettext_noop("Client %d(%s) needs lock %s/%d/%d owned by (see next line)"), 
          acdp->session_number, acdp->thread_name, acdp->lock_tbdf ? acdp->lock_tbdf->selfname : "Page", 
          acdp->record_or_page, acdp->lock_type);
    trace_msg_general(acdp, TRACE_LOCK_ERROR, buf, NOOBJECT);
   // check for cycle:
    if (acdp->ker_flags & KFL_PASSED_FLAG) break;
    acdp->ker_flags |= KFL_PASSED_FLAG;
   // find the owner of the lock:
    int lock_result = record_lock(acdp, acdp->lock_tbdf, acdp->record_or_page, acdp->lock_type);
    if (!lock_result || lock_result==-1 || acdp->break_request || acdp->appl_state == SLAVESHUTDOWN)
      { trace_msg_general(acdp, TRACE_LOCK_ERROR, form_message(buf, sizeof(buf), gettext_noop("nobody")), NOOBJECT);  break; }
    acdp = cds[lock_result-1];
    if (!acdp) break;  // internal error
  } while (true);
 // remove cycle flags:
  for (int i=0;  i<=max_task_index;  i++)
    if (cds[i]) 
      cds[i]->ker_flags &= ~KFL_PASSED_FLAG;
 // final message:
  trace_msg_general(cdp, TRACE_LOCK_ERROR, form_message(buf, sizeof(buf), final_msg), NOOBJECT);
}

bool is_deadlock(cdp_t cdp)
// Tries to lock the own lock. When blocked by a thread which waits for a lock, tries to lock it etc.
// May change own state or the state of other waiting threads.
// When cycle detected, the DEADLOCK error occurs.
{ 
  ProtectedByCriticalSection cs1(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
  ProtectedByCriticalSection cs2(&cs_deadlock, cdp, WAIT_CS_DEADLOCK);  
  DWORD curr_time=GetTickCount();
  cdp_t acdp = cdp;  int counter=0;
  do
  {// check if acdp is waiting for a lock (may be false even when acdp==cdp!)
    if (acdp->waits_for!=1)
      return false;  // [acdp] is not waiting, no action 
   // try to obtain the lock:
    int lock_result = record_lock(acdp, acdp->lock_tbdf, acdp->record_or_page, acdp->lock_type);
    if (!lock_result)
      { acdp->waits_for = 2;  return false; }  // [acdp] can continue
    if (lock_result==-1) 
      { acdp->waits_for = 3;  return false; }  // [acdp] can continue
    if (acdp->break_request || acdp->appl_state == SLAVESHUTDOWN)
      { acdp->waits_for = 4;  return false; }  // [acdp] can continue
    if (acdp->time_limit && curr_time > acdp->time_limit)
      { trace_lock_chain(cdp, gettext_noop("Lock timeout description:"), gettext_noop("End of timeout description."));
        acdp->waits_for = 5;  
        return false; 
      }  // [acdp] can continue
   // cannot obtain the lock, check the state of the blocking client:
    acdp = cds[lock_result-1];
    if (!acdp) return false;  // internal error
  } while (acdp!=cdp && ++counter<10);
  trace_lock_chain(cdp, gettext_noop("Deadlock description:"), gettext_noop("End of deadlock description."));
  //cdp->waits_for=0;  // doing this in the CS prevents detecting the same deadlock in other threads
  acdp->waits_for=6;  // deadlock
  return true;  // actual cycle found, deadlock
}

void report_lock_error(cdp_t cdp, table_descr * tbdf, trecnum recnum, uns8 lock_type, int lock_result, int errnum)
{ tobjname buf;  
  if (tbdf) sprintf(buf, "%d/%d", tbdf->tbnum, recnum);
  else      sprintf(buf, "PAGE %d/%d", cdp->index_owner, recnum);
  SET_ERR_MSG(cdp, buf);
  if (tbdf && cds[lock_result-1])
  { t_exkont_lock ekt(cdp, tbdf->tbnum, recnum, lock_type, cds[lock_result-1]->applnum_perm);
    request_error(cdp, errnum);
  }
  else
    request_error(cdp, errnum);
}

BOOL wait_record_lock_error(cdp_t cdp, table_descr * tbdf, trecnum recnum, uns8 lock_type, bool report_error)
// Returns FALSE if locked, TRUE on error.
{ int lock_result;  
 // first attempt to lock: the lock request is not registered yet -> no common CS is necessary
  lock_result=record_lock(cdp, tbdf, recnum, lock_type);
  if (!lock_result) return FALSE; // locked OK
  if (lock_result==-1)            // memory error
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; } 
 // insert the lock request and prepare for waiting:
  uns8 orig_wait_state = cdp->wait_type;
  cdp->wait_type = tbdf ? WAIT_RECORD_LOCK : WAIT_PAGE_LOCK;
  cdp->lock_tbdf=tbdf;  cdp->record_or_page=recnum;  cdp->lock_type=lock_type;  
  cdp->time_limit = cdp->waiting==-1 ? 0 : GetTickCount()+100*cdp->waiting;
  cdp->waits_for=1;  // from now the thread may try to obtain the lock for me!
  //char msg[200];
  //sprintf(msg, "Client %d starts waiting for lock %s/%d/%d on %u.", cdp->applnum_perm, tbdf ? tbdf->selfname : "Page", recnum, lock_type, GetTickCount());
  //dbg_err(msg);
  prof_time start;
  start = get_prof_time();
  BOOL res = TRUE;
  do
  { is_deadlock(cdp);
    cdp->lock_cycles++;
    if (cdp->waits_for==6)  // deadlock detected
    { cdp->waits_for=0;  cdp->wait_type=orig_wait_state;  
      if (report_error) report_lock_error(cdp, tbdf, recnum, lock_type, lock_result, DEADLOCK);
      break; 
    }
    if (cdp->waits_for==2)  // locked OK
    { cdp->waits_for=0;  cdp->wait_type=orig_wait_state;  
      res=FALSE; 
      break;
    }
    if (cdp->waits_for==3)  // cannot lock because of memory error (always reported)
    { cdp->waits_for=0;  cdp->wait_type=orig_wait_state;  
      request_error(cdp, OUT_OF_KERNEL_MEMORY);  
      break; 
    } 
    if (cdp->waits_for==4)  // cancelled
    { cdp->waits_for=0;  cdp->wait_type=orig_wait_state;  
      break; 
    }
    if (cdp->waits_for==5)  // timeout
    { cdp->waits_for=0;  cdp->wait_type=orig_wait_state;
      if (report_error) report_lock_error(cdp, tbdf, recnum, lock_type, lock_result, NOT_LOCKED);
      break; 
    }
   // cdp->waits_for is still 1 -> wait and try again
    if (PROFILING_ON(cdp) && !tbdf) 
      add_hit_and_time(0, PROFCLASS_ACTIVITY, PROFACT_INDEX_LOCK_RETRY, 0, NULL);
    if (!wait_for_lock_opened(cdp))  // server shutdown
    { cdp->waits_for=0;  cdp->wait_type=orig_wait_state;  
      break; 
    }
  } while (true);
 // waiting terminated:
  prof_time waited_time = get_prof_time()-start;
  if (PROFILING_ON(cdp)) 
    add_hit_and_time(waited_time, tbdf ? PROFCLASS_REC_LOCK : PROFCLASS_PAGE_LOCK, tbdf ? tbdf->tbnum : cdp->index_owner, 0, NULL);
  cdp->lock_waiting_time += waited_time;
  return res;
}

int wait_record_lock(cdp_t cdp, table_descr * tbdf, trecnum record, uns8 lock_type, int waiting)
{ 
  int saved_waiting = cdp->waiting;
  cdp->waiting = waiting;
  BOOL res = wait_record_lock_error(cdp, tbdf, record, lock_type, false); 
  cdp->waiting = saved_waiting;
  return res;
}

BOOL wait_page_lock(cdp_t cdp, tblocknum dadr, uns8 lock_type)
{ return wait_record_lock_error(cdp, NULL, (trecnum)dadr, lock_type, true); }

ProtectedByCriticalSection::ProtectedByCriticalSection(CRITICAL_SECTION * csIn, cdp_t cdpIn, t_wait cs_id) 
// [cs_id] is wait_type for waiting before entering the CS, [cs_id]+100 is the wait_type inside the CS
  : cs(csIn), cdp(cdpIn)
{ prof_time start;
  if (cdp)
  { outer_wait_type = cdp->wait_type;
    cdp->wait_type = cs_id;
    if (PROFILING_ON(cdp)) start = get_prof_time();
  }
  EnterCriticalSection(cs); 
  if (cdp) 
  { cdp->wait_type = (t_wait)(100+cdp->wait_type);
    if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_CS, cs_id-100, 0, NULL);
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/* Problem zacinajiho swapu
Stare spatne chovani: 
Kdyz get_frame vybralo cizi dirty page a zacalo ji swapowat, pak ji nejprve vyradilo 
z hashovaneho seznamu, pak docasne opustilo cs_frame a pak zaradilo seznamu swp control block.
Pokud behem swapovani majitel hledal svoji zmenenou stranku, nenasel ji a misto ni dostal sdilenou stranku.

Oprava:
Swapovani stranky vlozi swap control blok jeste pred opustenim cs_frame, po navratu do cs_frame do nej doplni swapdadr
nebo (pri neuspechu) zrusi swap cb.
Ve wr_changes se dadr/lock umistuje predem, takze se nemuze stat, ze by se nasel swap cb s nedokoncenym swapem.
Ve fix_..._page se muze najit swap cb s nedokoncenym swapem. Pak se stranka zamyka. Je-li swap nedokonceny, bude 
zamceni CEKAT na odemceni probihajicim swapem, protoze swap nemuze stranku odemknout drive, nez se kvuli cekani opusti
cs_frame a sam do nej vstoupi. Proto zamceni stranky vrati priznak, ze cekalo, coz zpusobi nove hledani stranky.
Neuspesny swap proto nebude mit nasledky.
*/

