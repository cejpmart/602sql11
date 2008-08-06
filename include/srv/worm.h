/****************************************************************************/
/* Worm.h - Definition part of the worm module                              */
/****************************************************************************/
union next_worm_part   /* "pointer" to the next part */
{ 
#if 0
  tblocknum partdadr;
#endif  
  struct wormbeg * partcadr;
};

struct wormbeg   /* the beginning of a worm core piece */
{ unsigned partsize;      /* max. number of bytes in this part */
  unsigned bytesused;     /* actual number of bytes in this part */
#if 0
  uns8  count_down;       /* next part is disc-resident iff equal to 0 */
#endif  
  next_worm_part w_next;  /* the next, 0 iff not exist */
  char  items[1];         /* start of data items */
};

BOOL worm_init   (cdp_t cdp, worm * w, unsigned partsize, uns8 coreparts);
void worm_free   (cdp_t cdp, worm * w);
void worm_close  (cdp_t cdp, worm * w);
void worm_reset  (cdp_t cdp, worm * w);
BOOL worm_end    (worm * w);
BOOL worm_step   (cdp_t cdp, worm * w, BOOL cat);
uns32 worm_len   (cdp_t cdp, worm * w);
tptr worm_copy   (cdp_t cdp, worm * w, void * where, unsigned size);
void worm_add    (cdp_t cdp, worm * w, const void * object, unsigned size);
void worm_mark   (worm * w);

