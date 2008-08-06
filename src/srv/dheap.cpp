/****************************************************************************/
/* Dheap.c - disc heap & heap structures management                         */
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

static void free_piece_t(cdp_t cdp, tpiece * pc,                t_translog * translog);
static BOOL get_piece_t (cdp_t cdp, tpiece * pc, unsigned size, t_translog * translog);
wbbool check_dworms = wbfalse;  // no checking in old FILs

BOOL copy_block_to_new(cdp_t cdp, tblocknum dest_dadr, tblocknum src_dadr, t_translog * translog)
{ BOOL err=FALSE;
  tfcbnum src_fcbn, dest_fcbn=fix_new_page(cdp, dest_dadr, translog);
  if (dest_fcbn==NOFCBNUM) err=TRUE; /* deadlock result */
  else
  { tptr dest_dt=FRAMEADR(dest_fcbn);
    const char * src_dt=fix_any_page(cdp, src_dadr, &src_fcbn);
    if (!src_dt) err=TRUE;
    else
    { memcpy(dest_dt, src_dt, BLOCKSIZE);
      unfix_page(cdp, src_fcbn);
      translog->page_changed(dest_fcbn); 
    }
    unfix_page(cdp, dest_fcbn);
  }
  return err;
}

#define MAX_GENERAL_BLOCKS         1022  // (BLOCKSIZE-DWORM2_PREF_SIZE)/sizeof(tblocknum)

#pragma pack(1)
struct dworm2_beg 
{ uns16 state;             /* DWORM_MARK or DWORM2_MARK, for safety and for distigushing the union branch */
  uns16 small_blockcnt;    /* number of small data block numbers */
  union 
  { struct
    { uns32 big_blockcnt;  /* number of big data block numbers */
      tblocknum new_blocks[1];
    } blidx;
    tblocknum old_blocks[1];
  };
};
#pragma pack()

t_dworm_access::t_dworm_access(cdp_t cdp, const tpiece * pc, unsigned elemlenIn, BOOL write_access, t_translog * translogIn)
{ is_error=false;
  elemlen=elemlenIn;
  translog=translogIn;
  perblock=BLOCKSIZE/elemlen;
  piece_size = k32*pc->len32;
  fix_cdp=cdp;
  if (!pc->dadr || !piece_size)
  { is_null=TRUE;  big_blockcnt = small_blockcnt = 0;
    goto err0; 
  }
  is_null=FALSE;
  { tptr dt = write_access ? fix_priv_page(cdp, pc->dadr, &bg_fcbn, translog) : (tptr)fix_any_page(cdp, pc->dadr, &bg_fcbn);
    if (!dt) goto err0;
    opened=TRUE;
    bg=(dworm2_beg*)(dt+k32*pc->offs32);
  }
  if (bg->state==DWORM2_MARK)
  { small_blockcnt=bg->small_blockcnt;
    big_blockcnt  =bg->blidx.big_blockcnt;
    big_blocks    =bg->blidx.new_blocks;
    small_blocks  =big_blocks+big_blockcnt;
    if (DWORM2_PREF_SIZE+(small_blockcnt+big_blockcnt)*sizeof(tblocknum) > BLOCKSIZE)
    { is_error=true;  dbg_err("Dworm error detected"); /*request_error(cdp, IE_OUT_OF_DWORM);*/  big_blockcnt=small_blockcnt=0;  goto err1; }
  }
  else if (bg->state==DWORM_MARK || !check_dworms)  // !check_dworms in old fil, no big blocks
  { small_blockcnt=bg->small_blockcnt;
    big_blockcnt  =0;
    small_blocks  =bg->old_blocks;
    if (DWORM_PREF_SIZE+small_blockcnt*sizeof(tblocknum) > k32*pc->len32)
    { is_error=true;  dbg_err("Dworm error detected"); /*request_error(cdp, IE_OUT_OF_DWORM);*/  big_blockcnt=small_blockcnt=0;  goto err1; }
  }
  else // error
    { is_error=true;  dbg_err("Dworm error detected"); /*request_error(cdp, IE_OUT_OF_DWORM);*/  big_blockcnt=small_blockcnt=0;  goto err1; }
  return;
 err1:
  unfix_page(cdp, bg_fcbn);  
 err0:
  opened=FALSE;  
}

void t_dworm_access::piece_changed(cdp_t cdp, const tpiece * pc)
{ translog->log.piece_changed(bg_fcbn, pc); }

t_dworm_access::~t_dworm_access(void)
{ if (opened)
    UNFIX_PAGE(fix_cdp, bg_fcbn);
}

tfcbnum t_dworm_access::take_fcbn(void)
{ tfcbnum fcbn = bg_fcbn;
  bg_fcbn = NOFCBNUM;  opened=FALSE;
  return fcbn;
}

tblocknum * t_dworm_access::get_small_block(unsigned ind)
{ return ind<small_blockcnt ? &small_blocks[ind] : NULL; }

tblocknum t_dworm_access::get_big_block(unsigned ind)
{ return ind<big_blockcnt ? big_blocks[ind] : 0; }

const char * t_dworm_access::pointer_rd(cdp_t cdp, unsigned start, tfcbnum * fcbn, unsigned * local)
// Returns read pointer to dworm elements starting with start. There are at most *local elemtnst there.
// If fcbn!=NOFCBNUM on exit, then must unfix it later.
{ const char * dt;  
  unsigned block_index = start / perblock;   
  unsigned bloffs      = start % perblock;  // offset of the 1st element in the 1st block
  if (block_index < big_blockcnt*SMALL_BLOCKS_PER_BIG_BLOCK)
  { tblocknum dadr = big_blocks[block_index/SMALL_BLOCKS_PER_BIG_BLOCK]+block_index%SMALL_BLOCKS_PER_BIG_BLOCK;
    dt = fix_any_page(cdp, dadr, fcbn);
    *local=perblock-bloffs;
  }
  else 
  { block_index -= big_blockcnt*SMALL_BLOCKS_PER_BIG_BLOCK; // block_index made relative to small_blocks
    if (block_index < small_blockcnt)
    { tblocknum dadr = small_blocks[block_index];
      dt = fix_any_page(cdp, dadr, fcbn);
      *local=perblock-bloffs;
    }
    else
    { dt = dirdata_pos();  
      *fcbn = NOFCBNUM; 
      *local = dirdata_maxsize() / elemlen;
    }
  }
  return dt+bloffs*elemlen;
}

char * t_dworm_access::pointer_wr(cdp_t cdp, unsigned start, tfcbnum * fcbn, unsigned * local)
// Returns write pointer to dworm elements starting with start. There are at most *local elemtnst there.
// If fcbn!=NOFCBNUM on exit, then must unfix it later.
// Caution: piece_changed not called here!
{ tptr dt;  
  unsigned block_index = start / perblock;   
  unsigned bloffs      = start % perblock;  // offset of the 1st element in the 1st block
  if (block_index < big_blockcnt*SMALL_BLOCKS_PER_BIG_BLOCK)
  { tblocknum dadr = big_blocks[block_index/SMALL_BLOCKS_PER_BIG_BLOCK]+block_index%SMALL_BLOCKS_PER_BIG_BLOCK;
    dt = fix_priv_page(cdp, dadr, fcbn, translog);  
    translog->page_changed(*fcbn); 
    *local=perblock-bloffs;
  }
  else 
  { block_index -= big_blockcnt*SMALL_BLOCKS_PER_BIG_BLOCK; // block_index made relative to small_blocks
    if (block_index < small_blockcnt)
    { tblocknum dadr = small_blocks[block_index];
      dt = fix_priv_page(cdp, dadr, fcbn, translog);  
      translog->page_changed(*fcbn); 
      *local=perblock-bloffs;
    }
    else
    { dt = dirdata_pos();  
      *fcbn = NOFCBNUM; 
      *local = dirdata_maxsize() / elemlen;
    }
  }
  return dt+bloffs*elemlen;
}

BOOL t_dworm_access::read(cdp_t cdp, unsigned firstel, unsigned elemcnt, tptr data)
{ if (!elemcnt) return FALSE;
  if (is_error)  // masking error
  { memset(data, 0, elemcnt*elemlen);
    return FALSE;
  }  
  if (is_null) return TRUE; // I'm not sure about this situation: should never occur, because size tested
  while (elemcnt)
  { tfcbnum fcbn;  unsigned local;
    const char * dt = pointer_rd(cdp, firstel, &fcbn, &local);
	  if (!dt) return TRUE; 
	  if (local > elemcnt) local=elemcnt;
    memcpy(data, dt, local*elemlen);  data+=local*elemlen;
    if (fcbn != NOFCBNUM)
      unfix_page(cdp, fcbn);
    firstel += local;  elemcnt -= local;
  }
  return FALSE;
}

BOOL t_dworm_access::write(cdp_t cdp, unsigned firstel, unsigned elemcnt, const char * data, const tpiece * pc)
{ if (!elemcnt) return FALSE;
  if (is_null) return TRUE; // I'm not sure about this situation: should never occur
  while (elemcnt)
  { tfcbnum fcbn;  unsigned local;
    tptr dt = pointer_wr(cdp, firstel, &fcbn, &local);
	  if (!dt) return TRUE; 
	  if (local > elemcnt) local=elemcnt;
    if (cdp->replication_change_detector && !*cdp->replication_change_detector)  // no change so far, must check for a change
      if (memcmp(dt, data, local*elemlen)) *cdp->replication_change_detector=true;
    memcpy(dt, data, local*elemlen);  data+=local*elemlen;
    if (fcbn != NOFCBNUM)
      unfix_page(cdp, fcbn);
    else
      piece_changed(cdp, pc);
    firstel += local;  elemcnt -= local;
  }
  return FALSE;
}

BOOL t_dworm_access::search(cdp_t cdp, t_mult_size start, t_mult_size size, int flags, 
                            wbcharset_t charset, int pattsize, char * pattern, t_varcol_size * pos)
// start + size is supposed not to exceed the dworm length!
// If not SEARCH_CASE_SENSITIVE then [pattern] is in upper case.
{ const char * dt0, * dt1;  tfcbnum fcbn0, fcbn1;  unsigned size0, size1;  uns32 respos;  BOOL result=FALSE;
  if (is_null || is_error) return FALSE;  // dworm empty
 /* 1st part */
  fcbn0=fcbn1=NOFCBNUM;   /* for errex */
  dt1 = pointer_rd(cdp, start, &fcbn1, &size1);
  if (!dt1) result=TRUE;
  else
  { if (size1 > size) size1=size;
    do // INV: size==bytes to be searched from dt1, start==dt1 position
    { unfix_page(cdp, fcbn0);  fcbn0=fcbn1;  dt0=dt1;  size0=size1;  fcbn1=NOFCBNUM;
      size -= size0;
     /* looking for the next part */
      if (size)
      { dt1 = pointer_rd(cdp, start+size0, &fcbn1, &size1);
        if (!dt1) { result=TRUE;  break; }
        if (size1 > size) size1=size;
      }
      else dt1=NULL;
	    if (flags & SEARCH_WIDE_CHAR ?
          Search_in_blockW((wuchar*)dt0, size0, (wuchar*)dt1, (wuchar*)pattern, pattsize, flags, charset, &respos) :
          Search_in_blockA(         dt0, size0,          dt1,          pattern, pattsize, flags, charset, &respos))
      { *pos = start + respos;
        break;
      }
      start+=size0; 
    } while (size);
  }
  unfix_page(cdp, fcbn0);  unfix_page(cdp, fcbn1);
  return result;
}

BOOL t_dworm_access::expand(cdp_t cdp, tpiece * pc, unsigned elemcnt)
// Expands the dworm size so that it can contain elemcnt elements. May change the pc.
// Facts: If the number of blocks grows, direct data will be moved into them.
//        If the number of big blocks grows, all data from small block will be moved into them. 
/* Strategy:
- find the new number of big and small blocks
- find the necessary piece size
- reallocate the basic piece (if it grows) preserving the contents, reinit bg pointers
- if adding some small blocks and not adding any big block, move direct data to the new small block
- if the number of big blocks changes from 0 to to >=1, reorganize the dworm into dworm2
- if adding some big blocks, move data from small blocks and direct data to the new big block
- release unnecessary small blocks
- move small block numbers to their new position in the piece
- allocate new small blocks
- allocate new big blocks
*/
{ if (!elemcnt) return FALSE;
  BOOL piece_was_changed = FALSE, direct_data_moved = FALSE;
 // find the new numbers of blocks and new_local_bytes
  unsigned total_block_count = (elemcnt-1) / perblock;
  unsigned new_big_blockcnt   = total_block_count / SMALL_BLOCKS_PER_BIG_BLOCK;
  unsigned new_small_blockcnt = total_block_count % SMALL_BLOCKS_PER_BIG_BLOCK;
  if (new_big_blockcnt < big_blockcnt) 
    { new_big_blockcnt = big_blockcnt;  new_small_blockcnt=0; }
  else if (new_big_blockcnt == big_blockcnt && new_small_blockcnt < small_blockcnt) 
    new_small_blockcnt = small_blockcnt;
  unsigned elems_in_blocks, new_loc_bytes;
  do
  { if (new_big_blockcnt+new_small_blockcnt > MAX_GENERAL_BLOCKS)
      { new_big_blockcnt++;  new_small_blockcnt=0; }
    elems_in_blocks = perblock * (SMALL_BLOCKS_PER_BIG_BLOCK*new_big_blockcnt+new_small_blockcnt);
    unsigned rest = elemcnt > elems_in_blocks ? elemlen * (elemcnt - elems_in_blocks) : 0;
    new_loc_bytes = DWORM2_PREF_SIZE + sizeof(tblocknum)*(new_big_blockcnt+new_small_blockcnt) + rest;
    if (new_loc_bytes<=BLOCKSIZE) break;
    if (rest)
      new_small_blockcnt++;
    else // size owerflow
      { request_error(cdp, INDEX_OUT_OF_RANGE);  return TRUE; }
  } while (TRUE);

  if (new_loc_bytes > k32*pc->len32 || is_error)   /* reallocate the basic piece */
  { tfcbnum fcbn1;  tpiece pc1;
   // allocate and init the new piece:
    if (get_piece_t(cdp, &pc1, (new_loc_bytes-1)/k32+1, translog))
		  return TRUE;
	  tptr dt1=fix_priv_page(cdp, pc1.dadr, &fcbn1, translog);
	  if (!dt1) return TRUE;
    dworm2_beg * bg1 = (dworm2_beg*)(dt1+k32*pc1.offs32);  
    //cdp->log.piece_changed(pc1); -- in get_piece_t
    memset(bg1, 0, k32*pc1.len32); // NULLING
    if (is_null || is_error)  // free unless pc is empty
    { bg1->state=DWORM_MARK;
      bg1->blidx.big_blockcnt = bg1->small_blockcnt = 0; // must init, pc->len32 may be 0!
      is_null=FALSE;  opened=TRUE;  is_error=false;
    }
    else  // implies opened==TRUE, release the old piece
    { bg1->state = bg->state;  
      memcpy(bg1, bg, piece_size);
      unfix_page(fix_cdp, take_fcbn());  // unfixing before releasing
      free_piece_t(cdp, pc, translog);  
    }
	  bg_fcbn=fcbn1;  *pc=pc1;  bg=bg1;  opened=true;
   // reinit members:
    piece_size = k32*pc->len32;
    if (bg->state==DWORM_MARK)
    { small_blockcnt=bg->small_blockcnt;
      big_blockcnt  =0;
      small_blocks  =bg->old_blocks;
    }
    else 
    { small_blockcnt=bg->small_blockcnt;
      big_blockcnt  =bg->blidx.big_blockcnt;
      big_blocks    =bg->blidx.new_blocks;
      small_blocks  =big_blocks+big_blockcnt;
    }
    piece_was_changed=TRUE;
  }
  else if (big_blockcnt!=new_big_blockcnt || small_blockcnt != new_small_blockcnt)
    piece_was_changed=TRUE;

 // moving direct data to a new small block (when a new small block and no big blocks are to be created):
  if (big_blockcnt==new_big_blockcnt && small_blockcnt < new_small_blockcnt)
  { tblocknum dadr = translog->alloc_block(cdp);
    if (!dadr) return TRUE; 
    tfcbnum fcbn1=fix_new_page(cdp, dadr, translog);
    if (fcbn1==NOFCBNUM) return TRUE; /* deadlock result */
    translog->page_changed(fcbn1); 
    tptr dt1=FRAMEADR(fcbn1);
    memset(dt1, 0, BLOCKSIZE); // NULLING
    memcpy(dt1, dirdata_pos(), dirdata_maxsize());
    unfix_page(cdp, fcbn1);
    small_blocks[small_blockcnt++]=dadr;  /* added the new block */
    direct_data_moved=TRUE;
  } 
 // reorganize, iff adding the 1st big block:
  if (new_big_blockcnt && bg->state==DWORM_MARK)
  { bg->state=DWORM2_MARK;
    memmov(bg->blidx.new_blocks, bg->old_blocks, piece_size-DWORM2_PREF_SIZE);
    big_blockcnt  = bg->blidx.big_blockcnt = 0;
    big_blocks = small_blocks = bg->blidx.new_blocks;  // inconsistent for a momment
  }
 // creating the 1st new big block, moving data from small blocks and direct data into it:
  tblocknum new_big_block = 0;
  if (big_blockcnt < new_big_blockcnt)
  { new_big_block = translog->alloc_big_block(cdp);
    if (!new_big_block) return TRUE; 
    unsigned i;
    for (i=0;  i<small_blockcnt;  i++)
      if (copy_block_to_new(cdp, new_big_block+i, small_blocks[i], translog)) return TRUE;
    if (!direct_data_moved)
    { tfcbnum fcbn1=fix_new_page(cdp, new_big_block+i, translog);
      if (fcbn1==NOFCBNUM) return TRUE; /* deadlock result */
      translog->page_changed(fcbn1); 
      tptr dt1=FRAMEADR(fcbn1);
      memcpy(dt1, dirdata_pos(), dirdata_maxsize());
      unfix_page(cdp, fcbn1);
    }
  }
 // release unnecessary small blocks (only after their contents moved to big block, so new_blocks can be used):
  while (small_blockcnt > new_small_blockcnt)
    translog->free_block(cdp, small_blocks[--small_blockcnt]);
 // move small block numbers to their new position in the piece, update th small_blocks pointer:
  unsigned move_size = piece_size - (unsigned)((tptr)small_blocks-(tptr)bg);
  if (new_big_blockcnt>big_blockcnt) move_size -= sizeof(tblocknum) * (new_big_blockcnt-big_blockcnt);
  memmov(small_blocks+(new_big_blockcnt-big_blockcnt), small_blocks, move_size);
  small_blocks += new_big_blockcnt-big_blockcnt;
 // allocate new small blocks:
  while (small_blockcnt < new_small_blockcnt)
  { tblocknum dadr = translog->alloc_block(cdp);
    if (!dadr) return TRUE; 
    small_blocks[small_blockcnt++]=dadr;
   // NULLING:
    tfcbnum fcbn1=fix_new_page(cdp, dadr, translog);
    if (fcbn1==NOFCBNUM) return TRUE; /* deadlock result */
    translog->page_changed(fcbn1); 
    tptr dt1=FRAMEADR(fcbn1);
    memset(dt1, 0, BLOCKSIZE); // NULLING
    unfix_page(cdp, fcbn1);
  }
 // allocating the big blocks (first, store the 1st big block allocate above):
  if (big_blockcnt < new_big_blockcnt)
    do
    { big_blocks[big_blockcnt++]=new_big_block;
      if (big_blockcnt == new_big_blockcnt) break;
      new_big_block = translog->alloc_big_block(cdp);
      if (!new_big_block) return TRUE; 
    } while (TRUE);

  if (piece_was_changed)  /* mark basic piece if any block added */
  { if (big_blockcnt)
      bg->blidx.big_blockcnt=big_blockcnt;
    bg->small_blockcnt=small_blockcnt;
	  piece_changed(cdp, pc);
  }
  return FALSE;
}

BOOL t_dworm_access::shrink(cdp_t cdp, tpiece * pc, uns32 elemcnt) // Changes *pc iff elemcnt==0
{ if (is_error)
    { pc->dadr=0;  pc->len32=0; }  // repairing the error
  else if (!is_null)
  { unsigned newblk = elemcnt ? (elemcnt-1)/perblock+1 : 0;
    unsigned new_big_blockcnt   = newblk / SMALL_BLOCKS_PER_BIG_BLOCK;
    unsigned new_small_blockcnt = newblk % SMALL_BLOCKS_PER_BIG_BLOCK;
    if (new_big_blockcnt > big_blockcnt) return FALSE;
    if (new_big_blockcnt < big_blockcnt && new_small_blockcnt) { new_big_blockcnt++;  new_small_blockcnt=0; }
    else if (new_big_blockcnt == big_blockcnt && new_small_blockcnt >= small_blockcnt) goto skip;
   // releasing
    while (new_big_blockcnt < big_blockcnt) // new_small_blockcnt==0
      translog->free_big_block(cdp, big_blocks[--big_blockcnt]);
    while (new_small_blockcnt < small_blockcnt) 
      translog->free_block(cdp, small_blocks[--small_blockcnt]);
    if (bg->state==DWORM2_MARK)
      bg->blidx.big_blockcnt=big_blockcnt;
    bg->small_blockcnt=small_blockcnt;
	  piece_changed(cdp, pc);
   skip:
    if (!elemcnt) 
    { unfix_page(fix_cdp, take_fcbn());  // unfixing before releasing
      free_piece_t(cdp, pc, translog);
      pc->dadr=0;  pc->len32=0; 
    }
  }
  return FALSE;
}

BOOL hp_read(cdp_t cdp, const tpiece * pc, uns16 elemlen, uns32 firstel, uns32 elemcnt, tptr data)
{ t_dworm_access dwa(cdp, pc, elemlen, FALSE, NULL);
  if (!dwa.is_open()) return TRUE;
  return dwa.read(cdp, firstel, elemcnt, data);
}

BOOL hp_write(cdp_t cdp, tpiece * pc, uns16 elemlen, uns32 firstel, unsigned elemcnt, const char * data, t_translog * translog)
{ t_dworm_access dwa(cdp, pc, elemlen, TRUE, translog);
  if (!dwa.is_open()) return TRUE;
  if (dwa.expand(cdp, pc, firstel+elemcnt)) return TRUE;
  return dwa.write(cdp, firstel, elemcnt, data, pc);
}

BOOL hp_shrink_len(cdp_t cdp, tpiece * pc, uns16 elemlen, uns32 elemcnt, t_translog * translog)
{ t_dworm_access dwa(cdp, pc, elemlen, TRUE, translog);
  if (!dwa.is_open()) return TRUE;
  return dwa.shrink(cdp, pc, elemcnt);
}

BOOL hp_free(cdp_t cdp, tpiece * pc, t_translog * translog)
{ return hp_shrink_len(cdp, pc, 1, 0, translog); }

sig64 get_alloc_len(cdp_t cdp, tpiece * pc)
{ t_dworm_access dwa(cdp, pc, 1, FALSE, NULL);
  if (!dwa.is_open()) return -1;
  return dwa.allocated_space();
}

BOOL hp_search(cdp_t cdp, const tpiece * pc, t_mult_size start, t_mult_size size, int flags, wbcharset_t charset, int pattsize, char * pattern, t_varcol_size * pos)
// Returns TRUE on error. *pos is supposed to be init to -1 (not found).
// If not SEARCH_CASE_SENSITIVE then [pattern] is in upper case.
{ t_dworm_access dwa(cdp, pc, 1, FALSE, NULL);
  if (!dwa.is_open()) return TRUE;
  return dwa.search(cdp, start, size, flags, charset, pattsize, pattern, pos);
}

tptr hp_locate_write(cdp_t cdp, tpiece * pc, uns16 elemlen, uns32 index, tfcbnum * pfcbn, t_translog * translog)
// Fixes the page containing the value and returns fcbn/pointer
{ t_dworm_access dwa(cdp, pc, elemlen, TRUE, translog);
  if (!dwa.is_open()) return NULL;
  if (dwa.expand(cdp, pc, index+1)) return NULL;
  unsigned local;
  tptr dt = dwa.pointer_wr(cdp, index, pfcbn, &local);
  if (!dt) return NULL; 
  if (*pfcbn==NOFCBNUM) // located in direct data, return fcnb owned by t_dworm_access
  { dwa.piece_changed(cdp, pc);
    *pfcbn = dwa.take_fcbn();
  }
  else
    translog->page_changed(*pfcbn);
  return dt;
}

const char * hp_locate_read(cdp_t cdp, const tpiece * pc, uns16 elemlen, uns32 index, tfcbnum * pfcbn)
// Fixes the page containing the value and returns fcbn/pointer
// The element is inside the dworm.
{ t_dworm_access dwa(cdp, pc, elemlen, FALSE, NULL);
  if (!dwa.is_open()) return NULL;
  unsigned local;
  const char * dt = dwa.pointer_rd(cdp, index, pfcbn, &local);
  if (!dt) return NULL; 
  if (*pfcbn==NOFCBNUM) // located in direct data, return fcnb owned by t_dworm_access
    *pfcbn = dwa.take_fcbn();
  return dt;
}

BOOL hp_copy(cdp_t cdp, const tpiece * pcfrom, tpiece * pcto, uns32 size, ttablenum tb, trecnum recnum, tattrib attrib, uns16 index, t_translog * translog)
{ t_dworm_access dwa_from(cdp, pcfrom, 1, FALSE, NULL);
  t_dworm_access dwa_to  (cdp, pcto,   1, TRUE,  translog);
  if (!dwa_from.is_open() || !dwa_to.is_open()) return TRUE;
  if (dwa_from.null())
    return dwa_to.shrink(cdp, pcto, 0);
  dwa_to.expand(cdp, pcto, size);
  unsigned pos=0;
  while (size)
  { tfcbnum fcbn_from, fcbn_to;  unsigned local_from, local_to;
    const char * dt_from = dwa_from.pointer_rd(cdp, pos, &fcbn_from, &local_from);
    tptr         dt_to   = dwa_to  .pointer_wr(cdp, pos, &fcbn_to,   &local_to);
	  if (!dt_from || !dt_to) return TRUE; 
	  if (local_from > size) local_from=size;
    memcpy(dt_to, dt_from, local_from);  
    if (the_sp.WriteJournal.val() && !(tables[tb]->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
      var_jour(cdp, tb, recnum, attrib, index, pos, size, dt_from);
    if (fcbn_from != NOFCBNUM) unfix_page(cdp, fcbn_from);
    if (fcbn_to   != NOFCBNUM) unfix_page(cdp, fcbn_to  );
    pos += local_from;  size -= local_from;
  }
  return FALSE;
}

void hp_make_null_bg(void * buf)
{ ((dworm2_beg*)buf)->state=DWORM_MARK; }

/************************* pieces allocation ********************************/
/* There is a global worm for each of piece sizes containing the free pieces.
   When kernel is started/closed, the worms are read from/written into heap objects. */
static const uns8 lencats[NUM_OF_PIECE_SIZES] = { 1, 2, 4, 8, 16 };

static piece_dynar pcstore[NUM_OF_PIECE_SIZES];

static int lencat(unsigned size)
{ for (int i=0;  i<NUM_OF_PIECE_SIZES;  i++)
    if (size <= lencats[i]) return i;
  return NUM_OF_PIECE_SIZES;
}

void reload_pieces(cdp_t cdp) /* load piece worms by free pieces stored in system */
{ for (int i=0;  i<NUM_OF_PIECE_SIZES;  i++)
  { pcstore[i].clear();
    if (header.pccnt[i])
    { tpiece * ppc = pcstore[i].acc(header.pccnt[i]-1);
      if (ppc!=NULL)
        hp_read(cdp, header.pcpcs+i, sizeof(tpiece), 0, header.pccnt[i],
                (tptr)pcstore[i].acc0(0));
    }
  }
}

BOOL init_pieces(cdp_t cdp, BOOL load_stored_pieces) /* init piece worms by free pieces stored in system */
{ tfcbnum fcbn;  
#ifdef LINUX
  for (int i=0;  i<NUM_OF_PIECE_SIZES;  i++)
    pcstore[i].init(0, 10);
#endif
  const dworm2_beg * bg = (const dworm2_beg*)fix_any_page(cdp, header.pcpcs[0].dadr, &fcbn);
  if (bg!=NULL)
  { check_dworms = (wbbool)(bg->state==DWORM_MARK || bg->state==DWORM2_MARK);  // check only in FILs with DWORM_MARKs!
    unfix_page(cdp, fcbn);
  }
  if (load_stored_pieces)   /* otherwise may contain allocated pieces */
    reload_pieces(cdp);
  return FALSE;
}

void save_pieces(cdp_t cdp) /* save free pieces to the system heaps */
/* must be followed by commit, should be foll. by write_header */
{ 
  disksp.deallocation_move(cdp);
  for (int i=0;  i<NUM_OF_PIECE_SIZES;  i++)
  { piece_dynar * pcs = &pcstore[i];
   // remove empty cells:
    int j=0;
    while (j < pcs->count())
      if (!pcs->acc0(j)->dadr) pcs->delet(j);
      else j++;
   // save all:
    tpiece * ppc = pcs->acc0(0);
    hp_write(cdp, header.pcpcs+i, sizeof(tpiece), 0, pcs->count(), (const char *)ppc, &cdp->tl);
    header.pccnt[i]=pcs->count();  /* save the number of pieces of this size */
    pcs->clear();
  }
}

int mark_free_pieces(cdp_t cdp, bool repair)
{ int errors = 0;
  for (int i=0;  i<NUM_OF_PIECE_SIZES;  i++)
  { piece_dynar * pcs = &pcstore[i];
    for (int j=0;  j<pcs->count();  j++)
    { tpiece * pc1 = pcs->acc0(j);
      if (pc1->dadr)
        if (xmark_piece(cdp, pc1)) 
        { errors++;  /* mark the piece iff cell not empty */
          if (repair) pc1->dadr=0;
        }
    }
  }
  return errors;
}

void mark_core_pieces(void)
{ for (int i=0;  i<NUM_OF_PIECE_SIZES;  i++)
    pcstore[i].mark_core();
}

void translate_free_pieces(cdp_t cdp)
{ for (int i=0;  i<NUM_OF_PIECE_SIZES;  i++)
  { piece_dynar * pcs = &pcstore[i];
    for (int j=0;  j<pcs->count();  j++)
    { tpiece * pc1 = pcs->acc0(j);
      if (pc1->dadr) translate(pc1->dadr);
    }
  }
}

#pragma optimize("g", off)

void locked_release_piece(cdp_t cdp, tpiece * pc, bool can_dealloc_direct)
{ if (pc->len32 > 16) 
  { page_unlock(cdp, pc->dadr, ANY_LOCK);  
    if (can_dealloc_direct)
      disksp.release_block_direct(cdp, pc->dadr);  
    else
      disksp.release_block_safe(cdp, pc->dadr);  
    trace_alloc(cdp, pc->dadr, false, "piece");
    if (is_trace_enabled_global(TRACE_PIECES)) 
    { char msg[81];
      trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,released block for pieces", pc->dadr, pc->offs32, pc->len32), NOOBJECT);
    }
    return; 
  }
  piece_dynar * pcs = &pcstore[lencat(pc->len32)];
  int spacepos = -1;
  int offs      = pc->offs32,
      twin_offs = ((offs / pc->len32) & 1) ? offs-pc->len32 : offs+pc->len32;
 // check for twin or same, find free space:
  for (int j=0;  j<pcs->count();  j++)
  { tpiece * pc1 = pcs->acc0(j);
    if (pc1->dadr==pc->dadr)
    { if (pc1->offs32==twin_offs)
      { pc1->dadr=0;  spacepos=j;
        tpiece apc;
        apc.offs32 = (uns8)(offs > twin_offs ? twin_offs : offs);
        apc.len32  = (uns8)(2*pc->len32);
        apc.dadr   = pc->dadr;
        locked_release_piece(cdp, &apc, can_dealloc_direct);
        return;
      }
      else if (pc1->offs32==offs)
        { dbg_err("Duplicate disk space unit");  return; } // DOUBLE_PIECE
    }
    else if (!pc1->dadr)
      spacepos=j;
  }
 // add to the dynar:
  if (spacepos==-1) // no fre space found, must append
  { tpiece * pc1 = pcs->next();
    if (pc1!=NULL) *pc1=*pc;
  }
  else              // write to a free cell in the dynar
    *pcs->acc0(spacepos)=*pc;
}

static BOOL locked_get_piece(cdp_t cdp, tpiece * pc, unsigned size)
{ if (size > 16)
  { if (!(pc->dadr=disksp.get_block(cdp))) return TRUE;
    trace_alloc(cdp, pc->dadr, true, "piece");
    pc->offs32=0;
    pc->len32=(uns8)(1<<NUM_OF_PIECE_SIZES);
    if (is_trace_enabled_global(TRACE_PIECES)) 
    { char msg[81];
      trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,allocated block for pieces", pc->dadr, pc->offs32, pc->len32), NOOBJECT);
    }
  }
  else
  { piece_dynar * pcs = &pcstore[lencat(size)];
    for (int j=0;  j<pcs->count();  j++)
    { tpiece * pc1 = pcs->acc0(j);
      if (pc1->dadr)  /* found a piece -> take it! */
      { *pc=*pc1;
        pc1->dadr=0;
        return FALSE;
      }
    } /* not found */
 	  if (locked_get_piece(cdp, pc, 2*size)) return TRUE; /* allocate twice bigger piece */
	  pc->len32 /= 2;
    locked_release_piece(cdp, pc, true);      /* and release half of it */
    pc->offs32 += pc->len32;
  }
  return FALSE;
}

#pragma optimize("", on)


void release_piece(cdp_t cdp, tpiece * pc, bool can_dealloc_direct)
{ ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS);
  locked_release_piece(cdp, pc, can_dealloc_direct);
}
/////////////////////////////////// listing free pieces to the trace log //////////////////////
void trace_free_pieces(cdp_t cdp, const char * label)
{ char msg[81];
  if (!is_trace_enabled_global(TRACE_PIECES)) return;
  for (int i=0;  i<NUM_OF_PIECE_SIZES;  i++)
  { piece_dynar * pcs = &pcstore[i];
   // remove empty cells:
    for (int j=0;  j < pcs->count();  j++)
    { tpiece * ppc = pcs->acc0(j);
      if (ppc->dadr) // unless empty cell
        trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,%s", ppc->dadr, ppc->offs32, ppc->len32, label), NOOBJECT);
    }
  }
}

/**************************** transaction level *****************************/
static void free_piece_t(cdp_t cdp, tpiece * pc, t_translog * translog)  
// Releasing a piece on the transaction level
{ char msg[81];
  if (translog->tl_type==TL_CURSOR)
  {
#ifdef STOP  // allocation not logged   
    if (pc->len32 > 16)
      translog->log.drop_page_changes(pc->dadr);
    else
      translog->log.drop_piece_changes(pc);
#endif      
    release_piece(cdp, pc, true);  // immediately, no logging, not delaying
  }  
  else  
    if (translog->log.piece_deallocated(pc->dadr, pc->len32, pc->offs32))  // allocated in the same transaction
    { if (is_trace_enabled_global(TRACE_PIECES)) 
        trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,piece released in expand/shrink", pc->dadr, pc->offs32, pc->len32), NOOBJECT);
      disksp.release_piece_safe(cdp, pc);  
    }
    else
      if (is_trace_enabled_global(TRACE_PIECES)) 
        trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,piece deallocated in expand/shrink", pc->dadr, pc->offs32, pc->len32), NOOBJECT);
}

static BOOL get_piece_t(cdp_t cdp, tpiece * pc, unsigned size, t_translog * translog)
// Allocating a piece on the transaction level, returns FALSE iff OK
{ tfcbnum fcbn;
  if (translog->tl_type==TL_CURSOR) size=32;  // cursor must not have pieces in the page which can be changed by rollback
  { ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS);
    if (locked_get_piece(cdp, pc, size)) return TRUE;
  }
 // init. the piece by 0's: 
 // page_lock(pc->dadr, TMPWLOCK);  -- no need to lock, bacause the owning record is locked
  tptr dt=fix_priv_page(cdp, pc->dadr, &fcbn, translog);
  if (dt==NULL) return TRUE;
  memset(dt+k32*pc->offs32, 0, k32*pc->len32);
  translog->log.piece_changed(fcbn, pc);  /*!*/
  unfix_page(cdp, fcbn);
  if (translog->tl_type!=TL_CURSOR)
  { translog->log.piece_allocated(pc->dadr, pc->len32, pc->offs32);
    if (is_trace_enabled_global(TRACE_PIECES)) 
    { char msg[81];
      trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,piece allocated", pc->dadr, pc->offs32, pc->len32), NOOBJECT);
    }
  }
  return FALSE;
}


