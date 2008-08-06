/****************************************************************************/
/* Worm.c - Implementation part of the worm module                          */
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
#include "worm.h"
#ifdef NETWARE
#include <stdlib.h>
#endif

void worm_reset(cdp_t cdp, worm * w)
{
  if (w->curr_fcbn!=NOFCBNUM)
  { if (w->curr_changed)
      ffa.write_block(cdp, (tptr)w->curr_coreptr, fcbs[w->curr_fcbn].blockdadr);
    UNFIX_PAGE(cdp, w->curr_fcbn); /* The frame manager does not know about the change */
    w->curr_fcbn=NOFCBNUM;
  }
  w->curr_changed=wbfalse;
  w->currpos=0;
  w->curr_coreptr=w->firstpart;
}

BOOL worm_init(cdp_t cdp, worm * w, unsigned partsize, uns8 coreparts)
/* initializes a newly allocated worm */
{ wormbeg * fp;
//  w->cdp=cdp;  -- does not work when worm is shared
  w->partsize=partsize;
  w->firstpart=fp=(wormbeg*)corealloc(sizeof(wormbeg)-1+partsize, OW_WORM);
  if (!fp) { request_error(cdp,OUT_OF_KERNEL_MEMORY);  return TRUE; }
  w->curr_fcbn=NOFCBNUM;  /* it must be done before the call to worm_reset ! */
  worm_reset(cdp, w);
#if 0
  fp->count_down=coreparts;
#endif
  fp->partsize=partsize;
  fp->bytesused=0;
#if 0
  if (coreparts) 
#endif
    fp->w_next.partcadr=NULL; 
#if 0
  else fp->w_next.partdadr=0;
#endif
  return FALSE;
}

void worm_free(cdp_t cdp, worm * w)
{ wormbeg wb0, wb1;
//  w->curr_changed=wbfalse;  (in order reset does not write to disc) - big bug
  worm_reset(cdp, w);
  wb0=*(w->curr_coreptr);    /* points to the secord part */
  if (!worm_step(cdp, w, FALSE))    /* in the second part */
#if 0
   while (wb0.count_down ? (wb0.w_next.partcadr!=NULL) : wb0.w_next.partdadr)
#else
   while (wb0.w_next.partcadr!=NULL)
#endif
   /* wb0 describes the part I'm in */
   { wb1=*(w->curr_coreptr);
     if (worm_step(cdp, w, FALSE)) break;     /* goes to the next part */
#if 0
     if (wb0.count_down) 
#endif
       corefree(wb0.w_next.partcadr);
#if 0
     else ffa.release_block(cdp, wb0.w_next.partdadr);
#endif
     wb0=wb1;
   }
  worm_reset(cdp, w);
#if 0
  if (w->firstpart->count_down) 
#endif
    w->firstpart->w_next.partcadr=NULL;
#if 0
  else w->firstpart->w_next.partdadr=0;
#endif
  w->firstpart->bytesused=0;
}

void worm_mark(worm * w)
{ wormbeg * wb = w->firstpart;
  while (wb)
  { mark(wb);
#if 0
    if (wb->count_down)   /* next part is in core */
#endif
      wb=wb->w_next.partcadr;
#if 0
    else break;
#endif
  }
}

BOOL worm_end(worm * w)
{ if (w->currpos < w->curr_coreptr->bytesused) return FALSE;
#if 0
  if (w->curr_coreptr->count_down)
#endif
	 return w->curr_coreptr->w_next.partcadr==NULL;
#if 0
  else return w->curr_coreptr->w_next.partdadr==0;
#endif
}

void worm_close(cdp_t cdp, worm * w)
{ worm_free(cdp, w);
  safefree((void**)&w->firstpart);
}

BOOL worm_step(cdp_t cdp, worm * w, BOOL cat)
/* goes to the beg. of the next part,
   or to the end of the current if it's the last one & !cat */
{ wormbeg * cp, * fp, * oldcore; tfcbnum oldfcbn;  BOOL oldchng;
  w->currpos=0;
  oldfcbn=w->curr_fcbn;
  oldchng=w->curr_changed;
  oldcore=w->curr_coreptr;
  cp=w->curr_coreptr;
#if 0
  tblocknum dadr;
  if (cp->count_down)   /* next part is in core */
#endif  
  { if (!cp->w_next.partcadr)
      if (cat)
      { fp=cp->w_next.partcadr=(wormbeg*)corealloc(sizeof(wormbeg)-1+w->partsize, OW_WORM);
        if (!fp) { request_error(cdp, OUT_OF_KERNEL_MEMORY); return TRUE; }
#if 0
        fp->count_down = (uns8)(cp->count_down-1);
        if (fp->count_down) 
#endif
          fp->w_next.partcadr=NULL;
#if 0
        else                fp->w_next.partdadr=0;
#endif
        fp->partsize=w->partsize;
        fp->bytesused=0;
      }
      else { w->currpos=cp->bytesused; return FALSE; } /* cannot make a step */
	 w->curr_coreptr=cp->w_next.partcadr;
    w->curr_fcbn=NOFCBNUM;
  }
#if 0
  else  /* the next is the disk part */
  { if (!cp->w_next.partdadr)  /* allocate new disc block */
      if (cat)
      { if (!(dadr=ffa.get_block(cdp))) return TRUE;
        if (!(fp=(wormbeg*)fix_any_page(cdp, cp->w_next.partdadr=dadr, // special case, private "any" page
                                &w->curr_fcbn))) return TRUE;
         /* not fix_new_page: must not reserve it for the application! */
        fp->count_down=0;
        fp->bytesused=0;
        fp->partsize=BLOCKSIZE-sizeof(wormbeg)+1;
        fp->w_next.partdadr=0;
        w->curr_changed=wbtrue;
        oldchng=TRUE;
      }
      else { w->currpos=cp->bytesused; return FALSE; }  /* cannot make a step */
    else  /* the next block exists */
    { if (!(fp=(wormbeg*)
        fix_any_page(cdp,cp->w_next.partdadr, &(w->curr_fcbn))))
        { w->currpos = cp->bytesused;  /* prevents infinite loop on disk error */
          cp->w_next.partdadr=0;
          return TRUE;
        }
      w->curr_changed=wbfalse;
    }
    w->curr_coreptr=fp;
	 if (oldfcbn!=NOFCBNUM) /* leave the current part */
	 { if (oldchng)
		   ffa.write_block(cdp, (tptr)oldcore, fcbs[oldfcbn].blockdadr);
		 UNFIX_PAGE(cdp, oldfcbn); /* The frame manager does not know about the change */
	 }
  }
#endif
  return FALSE;
}

void worm_add(cdp_t cdp, worm * w, const void * object, unsigned size)
{ unsigned local;
  while (size)
  { local=w->curr_coreptr->partsize - w->currpos;
    if (local>size) local=size;  /* local = bytes to copy form curr. part */
    memcpy(w->curr_coreptr->items + w->currpos, object, local);
    w->curr_changed=wbtrue;
    object = (const char *)object + local;
    size-=local;
    w->currpos += local;  /* the new part end */
    if (w->curr_coreptr->bytesused < w->currpos)
      w->curr_coreptr->bytesused = w->currpos;
    if (size) if (worm_step(cdp, w, TRUE)) break;
  }
}

uns32 worm_len(cdp_t cdp, worm * w)
/* returns the current worm data length */
{ uns32 len=0;
  worm_reset(cdp, w);
  while (!worm_end(w))
  { len += w->curr_coreptr->bytesused;
    if (worm_step(cdp, w, FALSE)) break;
  }
  return len;
}

tptr worm_copy(cdp_t cdp, worm * w, void * where, unsigned size)
/* copies size bytes of the worm contents from the current position to where */
{ unsigned local; tptr result=NULL;
  do
  { if (w->currpos >= w->curr_coreptr->bytesused)   /* testing worm end */
#if 0
		if (w->curr_coreptr->count_down)
#endif
		  { if (!w->curr_coreptr->w_next.partcadr) break; }
#if 0
		else
		  { if (!w->curr_coreptr->w_next.partdadr) break; }
#endif		  
	/* code above replaced calling worm_end(w) for efficiency reasons */
	 local=w->curr_coreptr->bytesused - w->currpos;
	 if (local>size) local=size;  /* local = bytes to copy form curr. part */
	 result=w->curr_coreptr->items + w->currpos;
	 if (where)
	 { memcpy(where, result, local);
		where = (tptr)where + local;
	 }
	 w->currpos += local;
	 if (!(size-=local)) break;
	 if (worm_step(cdp, w, FALSE)) break;
  } while (TRUE);
  return result;
}

/****************************************************************************/
#include "dynarbse.cpp"
#include "dynar.cpp"

