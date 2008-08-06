/****************************************************************************/
/* Btree.cpp - operations on B-trees                                        */
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
#include "netapi.h"

/* Every inner node contains one more pointers than key values. The number
   of key values may be 0 (possible in the root only). */
/* The stack: stack[0] points to the root of the tree, stack[sp].nn to a leaf.
   posit is the index of the current pointer (in an inner node) or value (in
   a leaf). */
/* When passing a tree, all nodes in the stack are read-locked. */
/* When updating a tree, nodes along the stack are read-locked first, then
   some are unlocked and others tmp-write-locked. */

BOOL is_null_key(const char * key, const dd_index * idx_descr)
{ unsigned i=0;  const part_desc * apart = idx_descr->parts;
  do
  { if (!IS_NULL(key, apart->type, apart->specif)) return FALSE;
    // (when partsize is reduced to MAX_INDPARTVAL_LEN, then specif.length is reduced too)
    if (++i==idx_descr->partnum) break;
   // go to next part:
    key+=apart->partsize;  apart++;
  } while (TRUE);
  return TRUE;
}

static sig32 node_seek(const char * key, dd_index * idx_descr, const nodebeg * node,
                             unsigned * posit)
/* searches the given key in the given node. Returns the offset of chosen
   branch pointer and TRUE iff match is exact */
{ unsigned step;  sig32 res;  BOOL leaf;
  const char * keystart;  unsigned lind, rind, mind, maxrind;  BOOL is_lcmp;
  BOOL ignore_rec = DISTINCT(idx_descr);
  leaf=node->filled & LEAF_MARK;
  if (leaf)
  { step=idx_descr->keysize;
    rind=((node->filled & SIZE_MASK)                  ) / step;
    keystart=(const char *)&node->ptr1;
  }
  else
  { step=idx_descr->keysize+sizeof(tblocknum);
    rind=((node->filled & SIZE_MASK)-sizeof(tblocknum)) / step;
    keystart=node->key1;
  }
  lind=0;
  if (!rind) { *posit=0;  return 1; } /* node empty */
  maxrind=rind;
  /* invariant: searching <lind, rind) interval */
  is_lcmp=FALSE;  res=0;  // res initialised only to avoid a warning
  while (lind+1 < rind)  /* more than 1 key in the interval */
  { mind=(lind+rind)/2;
    res=cmp_keys(key, keystart+mind*step, idx_descr);
    if (HIWORD(res) && ignore_rec)  // key found
      { lind=mind;  is_lcmp=TRUE;  break; }
    if ((short)res >= 0)
    { lind=mind;  is_lcmp=TRUE;
      if (!(short)res) break;  /* faster end if exact record found */
    }
    else
    { rind=mind;  is_lcmp=FALSE;
    }
  }
  if (!is_lcmp) res=cmp_keys(key, keystart+lind*step, idx_descr);
  if (((short)res<=0) || HIWORD(res) && ignore_rec)
    { *posit=lind;  return res; }
  *posit=rind;   /* now rind==lind+1 */
  return (rind==maxrind) ? 1 :
    cmp_keys(key, keystart+rind*step, idx_descr);  /* may be, I could return -1 here, but the HIWORD is not clear */
}

void t_dumper::out(const char * str)
{ int len = (int)strlen(str);
  do
  { unsigned space = sizeof(buf) - bufcont;
    unsigned mv = len < space ? len : space;
    memcpy(buf+bufcont, str, mv);
    bufcont += mv;  len-=mv;  str+=mv;
    if (bufcont==sizeof(buf))
    { DWORD wr;
      WriteFile(hnd, buf, bufcont, &wr, NULL);
      bufcont=0;
    }
  } while (len);
}

inline void verify_node(dd_index * idx_descr, const nodebeg * node)
{}
#ifdef STOP
{ unsigned step;  BOOL leaf;
  const char * keystart;  unsigned lind, rind;  
  leaf=node->filled & LEAF_MARK;
  if (leaf)
  { step=idx_descr->keysize;
    rind=((node->filled & SIZE_MASK)                  ) / step;
    keystart=(const char *)&node->ptr1;
  }
  else
  { step=idx_descr->keysize+sizeof(tblocknum);
    rind=((node->filled & SIZE_MASK)-sizeof(tblocknum)) / step;
    keystart=node->key1;
  }
  lind=0;
  while (lind+1 < rind)
  { if ((short)cmp_keys(keystart+lind*step, keystart+(lind+1)*step, idx_descr) >= 0)
      { dbg_err("Index node damaged internally");  break; }
    lind++;
  }
}
#endif

void t_dumper::dump_key(const dd_index * idx_descr, const char * key)
{ unsigned i=0;  const part_desc * apart = idx_descr->parts;
  char buf[2*MAX_INDPARTVAL_LEN+1];  // space for MAX_INDPARTVAL_LEN converted to text
  do
  { //convert2string(key, apart->type, buf, SQL_LITERAL_PREZ, apart->specif);
    display_value(cdp, apart->type, apart->specif, -1, key, buf);
    out(buf);
    // (when partsize is reduced to MAX_INDPARTVAL_LEN, then specif.length is reduced too)
    if (++i==idx_descr->partnum) break;
   // go to next part:
    key+=apart->partsize;  apart++;
    out(" : ");
  } while (true);
}

void t_dumper::dump_tree(cdp_t cdp, const dd_index * idx_descr, tblocknum node_num)
{ t_page_rdaccess rda(cdp);
  outi(node_num);  out(" node --------------------------:\n");
  const nodebeg * node = (const nodebeg*)rda.fix(node_num);
  if (!node) 
    out("Fix error\n");
  else if (node->index_id != INDEX_ID)
    out("Node error\n");  
  else
  { bool leaf = (node->filled & LEAF_MARK) != 0;
    unsigned step, lind, rind;  const char * keystart;
    if (leaf)
    { step=idx_descr->keysize;
      rind=((node->filled & SIZE_MASK)                  ) / step;
      keystart=(const char *)&node->ptr1;
    }
    else
    { step=idx_descr->keysize+sizeof(tblocknum);
      rind=((node->filled & SIZE_MASK)-sizeof(tblocknum)) / step;
      keystart=node->key1;
      outi(node->ptr1);  out(" = preptr\n");
    }
    for (lind=0;  lind<rind;  lind++)
    { dump_key(idx_descr, keystart);
      keystart += idx_descr->keysize;
      if (!leaf) 
      { out("   Ptr: ");  outi(*(tblocknum*)keystart);
        keystart+=sizeof(tblocknum);
      }
      out("\n");
    }
    if (!leaf)
    { keystart=(const char*)&node->ptr1;
      for (lind=0;  lind<=rind;  lind++)
      { dump_tree(cdp, idx_descr, *(tblocknum*)keystart);
        keystart+=step;
      }
    }
  }
}

bool t_btree_read_lock::lock_and_update(cdp_t cdpIn, tblocknum rootIn)
// Returns false and signals a rollback exception if cannot lock or update the index. 
{ cdp=cdpIn;  root=rootIn;
  if (wait_record_lock(cdp, NULL, (trecnum)root, RLOCK, -1))  // infinite waiting
    return false;  // wait cancelled
 // search the log if the private index pages are invalid (other logs contain only private indexes, they are never invalid):
  t_index_uad * iuad = cdp->tl.find_index_uad(root);
  if (iuad && iuad->invalid)
    if (!update_private_changes_in_index(cdp, iuad))
    { page_unlock(cdp, root, RLOCK);
      return false;
    }
  locked=true;
  return true;
}

void t_btree_read_lock::unlock(void)
{ 
  if (locked)
  { page_unlock(cdp, root, RLOCK);
    locked=false;
  }
}

bt_result t_btree_acc::build_stack(cdp_t cdp, tblocknum root, dd_index * indx_descr, const char * key)
// Builds path for given [key] and returns info if the last match was exact.
// On exit the leaf page is fixed (leaf == stack top == stack[sp]).
// The root must be read-locked outside, iff working on a shared index. 
// Stack is empty iff sp==-1. On error, nothig is locked nor fixed.
{ sig32 res;  unsigned posit;  BOOL /*!*/ leaf;
  fix_cdp=cdp;
  fcbn=NOFCBNUM;  sp=-1;  // init [this] state
  skip_current=false;      // starting the passing
  idx_descr = indx_descr;  // saving the index descr for btree_step()
  if (!root) return BT_ERROR;   // possible in old FILs - when a new index was added to the table and its definition was the same as of an existing index
  unsigned step=sizeof(tblocknum)+idx_descr->keysize;
  tblocknum node_num=root;  /* go down to the leaf */
  do  /* sp points to stack top, node_num is the next node, no page is fixed, fcbn==NOFCBNUM */
  { const nodebeg * node;  
    if (sp+1>=BSTACK_LEN)
      { request_error(cdp,IE_OUT_OF_BSTACK);  goto err1; }
    if (push(cdp, node_num)) goto err1;
    if (!(node=(const nodebeg*)fix_any_page(cdp, node_num, &fcbn))) goto err1;
    if (node->index_id != INDEX_ID)
    { tobjname buf;  int2str(cdp->index_owner, buf);  SET_ERR_MSG(cdp, buf);  request_error(cdp, INDEX_DAMAGED);
      goto err1;
    }
    verify_node(idx_descr, node);
   /* now, all stack <0..sp> is locked and its top is fixed */
    leaf=node->filled & LEAF_MARK;
    if (leaf) step-=sizeof(tblocknum);
   /* this is not necessary if no update follows (!pfcbn): */
    if ((node->filled & SIZE_MASK) + step >= IBLOCKSIZE-IHDRSIZE)
         stack[sp].nstate=NS_FULL;
    else if ((node->filled & SIZE_MASK) <= (IBLOCKSIZE-IHDRSIZE)/2 + step)
         stack[sp].nstate=NS_HALF;
    else stack[sp].nstate=NS_NORMAL;
    res=node_seek(key, idx_descr, node, &posit);
    stack[sp].posit=posit;
   /* stack info completed */
    if (leaf) break;
    node_num=*(tblocknum*)((tptr)&node->ptr1+posit*step);
    UNFIX_PAGE(cdp, fcbn);  fcbn=NOFCBNUM;  // making [this] consistent
  } while (true);
 /* sp points to the last used cell */
  return HIWORD(res) ? ( (short)res      ? BT_EXACT_KEY : BT_EXACT_REC) :
                       (((short)res < 0) ? BT_CLOSEST   : BT_NONEX);
 err1:
 /* sp points to the last used cell, necessary for proper unlocking! */
  close(cdp);
  return BT_ERROR;
}

BOOL get_first_value(cdp_t cdp, tblocknum root, const dd_index * idx_descr, tptr key)
// Returns the 1st value in the index and TRUE or FALSE if the index is empty (or on error)
// Returns only the 1st part of the key value!
{ 
  if (!root) return FALSE;   // possible in old FILs - when a new index was added to the table and its definition was the same as of an existing index
  t_btree_read_lock btrl;
  if (!btrl.lock_and_update(cdp, root))
    return FALSE;
  tblocknum node_num = root;  /* go down to the leaf */
  do  // goes down the tree
  {// access the current node: 
    { t_page_rdaccess rda(cdp);
      const nodebeg * node = (const nodebeg*)rda.fix(node_num);
      if (!node) break;
      if (node->index_id != INDEX_ID)
      { tobjname buf;  int2str(cdp->index_owner, buf);  SET_ERR_MSG(cdp, buf);  request_error(cdp, INDEX_DAMAGED);
        break;
      }
     // go down or copy the value:
      if (node->filled & LEAF_MARK) 
      { if (!(node->filled & SIZE_MASK))
          break;  // this is not an error, index is empty
        memcpy(key, &node->ptr1, idx_descr->parts[0].partsize);
        return TRUE;
      }
     // not a leaf, going down:
      node_num = node->ptr1;
    } // unfix 
  } while (true);
  return FALSE;
}

BOOL get_last_value(cdp_t cdp, tblocknum root, const dd_index * idx_descr, tptr key)
// Returns the last value in the index and TRUE or FALSE if the index is empty (or on error)
// Returns only the 1st part of the key value!
{ 
  if (!root) return FALSE;   // possible in old FILs - when a new index was added to the table and its definition was the same as of an existing index
  t_btree_read_lock btrl;
  if (!btrl.lock_and_update(cdp, root))
    return FALSE;
  tblocknum node_num=root;  /* go down to the leaf */
  do  // goes down the tree
  {// access the current node: 
    { t_page_rdaccess rda(cdp);
      const nodebeg * node = (const nodebeg*)rda.fix(node_num);
      if (!node) break;
      if (node->index_id != INDEX_ID)
      { tobjname buf;  int2str(cdp->index_owner, buf);  SET_ERR_MSG(cdp, buf);  request_error(cdp, INDEX_DAMAGED);
        break;
      }
     // go down or copy the value:
      if (node->filled & LEAF_MARK) 
      { if (!(node->filled & SIZE_MASK))
          break;  // this is not an error, index is empty
        const char * valpos = (const char*)&node->ptr1+(node->filled & SIZE_MASK)-idx_descr->keysize;
        memcpy(key, valpos, idx_descr->parts[0].partsize);
        return TRUE;
      }
     // not a leaf, going down:
      node_num=*(tblocknum*)((const char*)&node->ptr1+(node->filled & SIZE_MASK)-sizeof(tblocknum));
    } // unfix
  } while (true);
  return FALSE;
}

BOOL t_btree_acc::btree_step(cdp_t cdp, tptr key)
/* False returned on btree end & on error. Updates stack, psp & fcbn.
   Supposes *psp >= 0, fcbn valid.
   if (presetp) then makes a step in the btree.
   Then returns the current key in [key].
   On entry & on exit (even if error occured) the full stack in locked,
   pfcbn is fixed or equal to NOFCBNUM. */
{ int step;  unsigned cnt;  const nodebeg * node;  tblocknum dadr;
  if (fcbn==NOFCBNUM) return FALSE;  // validating the entry condition, possible after error in prev. call of this function
  if (sp < 0) return FALSE;  /* this is used in outer joins, btree_step may be called again after it returned FALSE! */
  step=idx_descr->keysize;  /* step for leaf */
  node=(nodebeg*)FRAMEADR(fcbn);  // this node has already been checked
  cnt=(node->filled & SIZE_MASK) / step; // count of keys in the leaf
  // ATTN: build_stack() may position on the end of the node, must return a key even without [skip_current]!
  if (skip_current) stack[sp].posit++;
  else skip_current=true;
  while (stack[sp].posit>=cnt) /* goes upwards */
  { if (!drop()) return FALSE; /* *pfcbn will be unfixed by caller */
    UNFIX_PAGE(cdp, fcbn);  
    if (!(node=(const nodebeg*)fix_any_page(cdp, stack[sp].nn, &fcbn)))
      goto err;
    cnt=((node->filled & SIZE_MASK)+idx_descr->keysize)/(step+sizeof(tblocknum));
    stack[sp].posit++;
  }
 /* stack[sp] is the last node which is locked & the only which is fixed */
  while (!(node->filled & LEAF_MARK))
  { dadr=*(tblocknum*)((tptr)&node->ptr1+stack[sp].posit*(step+sizeof(tblocknum)));
    UNFIX_PAGE(cdp, fcbn);
    if (push(cdp, dadr)) goto err;  /* waiting breaked */
    stack[sp].posit=0;
    if (!(node=(const nodebeg*)fix_any_page(cdp, dadr, &fcbn))) goto err;
    if (node->index_id != INDEX_ID)
    { tobjname buf;  int2str(cdp->index_owner, buf);  SET_ERR_MSG(cdp, buf);  request_error(cdp, INDEX_DAMAGED);
      UNFIX_PAGE(cdp, fcbn);  goto err;
    }
  }
  memcpy(key, (tptr)&node->ptr1 + stack[sp].posit * step, step);
  return TRUE;
 err:
  fcbn=NOFCBNUM;
  return FALSE; 
}

BOOL t_btree_acc::get_key_rec(cdp_t cdp, tptr key, trecnum * precnum) const
// Called when the stack is build for a key. Does not change the stack,
// searches and read the record with the given key. Returns TRUE iff key found.
{ int step;  unsigned cnt;  tblocknum dadr;
  if (sp < 0) return FALSE;  /* this may be used in outer joins, may be called again after it returned FALSE? */
  step=idx_descr->keysize;  /* step for leaf */
  const nodebeg * node = (const nodebeg*)FRAMEADR(fcbn);  // this node has already been checked
  cnt=(node->filled & SIZE_MASK) / step; // count of keys in the leaf
  int lsp=sp;  // must not change the sp!
  t_page_rdaccess rda(cdp);  // used only sometimes (must not have a shorter lifetime than [node] above!

 // while positioned after the node end, go upwards:
  while (stack[lsp].posit>=cnt)
  { if (!lsp || DISTINCT(idx_descr)) return FALSE;  // end of btree
    lsp--;
    node = (const nodebeg*)rda.fix(stack[lsp].nn);
    if (!node) return FALSE;
    cnt=((node->filled & SIZE_MASK)+idx_descr->keysize)/(step+sizeof(tblocknum));
  }

 // go down to the leaf:
  int curr_posit=stack[lsp].posit;
  while (!(node->filled & LEAF_MARK))
  { dadr=*(tblocknum*)((tptr)&node->ptr1+curr_posit*(step+sizeof(tblocknum)));
    node = (const nodebeg*)rda.fix(dadr);
    if (node->index_id != INDEX_ID)
    { tobjname buf;  int2str(cdp->index_owner, buf);  SET_ERR_MSG(cdp, buf);  request_error(cdp, INDEX_DAMAGED);
      return FALSE;
    }
    curr_posit=0;
  }

 // check the current key in the current leaf:
  LONG res=cmp_keys(key, (tptr)&node->ptr1+curr_posit*step, idx_descr);
  if (HIWORD(res))
  { tptr akey = (tptr)&node->ptr1+curr_posit*step;
    if (precnum)
      *precnum=*(trecnum*)(akey+step-sizeof(trecnum));
   // copy the recnums to the pattern key (used by confirm_record):
    unsigned i=0, off=0;
    while (i<idx_descr->partnum) off+=idx_descr->parts[i++].partsize;
    memcpy(key+off, akey+off, step-off);
    return TRUE;
  }
  else return FALSE;
}

static bool insert_item(cdp_t cdp, tptr key, dd_index * idx_descr,
                      tblocknum * pval, tfcbnum fcbn, unsigned posit, t_translog * translog, tblocknum root)
/* inserts key & *pval into node fcbn. In overflovs, returns TRUE and new key & *pval, otherwise returns FALSE.
   [fcbn] node is fixed, marks the [fcbn] page changed. */
{ unsigned keysize;  nodebeg *node, *node1;
  bool dbl, leaf;  tblocknum dadr1;  tfcbnum fcbn1;
  unsigned step, cnt, isleave, dtsize;  tptr nptr, nptr1, destin;

  node=(nodebeg*)FRAMEADR(fcbn);
  nptr=(tptr)&node->ptr1;
  step=keysize=idx_descr->keysize;
  leaf = (node->filled & LEAF_MARK) != 0;
  if (!leaf) step+=sizeof(tblocknum);
  dtsize=node->filled & SIZE_MASK;
  dbl=dtsize+step > IBLOCKSIZE-IHDRSIZE;
  if (dbl)  // must divide the node!
  { dadr1 = translog->alloc_index_block(cdp, root);  
    if (!dadr1) return FALSE;
    fcbn1=fix_new_page(cdp, dadr1, translog);
    if (fcbn1==NOFCBNUM) return FALSE;
    node1=(nodebeg*)FRAMEADR(fcbn1);
    node1->index_id = INDEX_ID;
    nptr1=(tptr)&node1->ptr1;
    translog->index_page_changed(cdp, root, fcbn1);
    cnt=(dtsize - (leaf ? 0 : sizeof(tblocknum))) / step; // total # of keys
    isleave=cnt/2;  // # of keys to be left in the original node
    if (cnt & 1 && posit>=isleave) isleave++;
   /* division */
    memcpy(nptr1, nptr+isleave*step, dtsize-isleave*step);
    if (posit>=isleave) /* add into the new node */
    { posit-=isleave;
      destin=nptr1+step*posit;
      if (!leaf) destin+=sizeof(tblocknum);
      memmov(destin+step,destin,step*(cnt-isleave-posit) );
    }
    else
    { destin=nptr+step*posit;
      if (!leaf) destin+=sizeof(tblocknum);
      memmov(destin+step,destin,step*(isleave-posit) );
      isleave++;
    }
    memcpy(destin, key, keysize);
    cnt++;
    if (leaf)
    { node ->filled=(uns16)(LEAF_MARK | step*isleave);
      node1->filled=(uns16)(LEAF_MARK | step*(cnt-isleave));
    }
    else /* inner node */
    { *(tblocknum*)(destin+keysize)=*pval;
      node ->filled=(uns16)(step*isleave-keysize);
      node1->filled=(uns16)(step*(cnt-isleave)+sizeof(tblocknum));
    }
   /* upwards: */
    memcpy(key,nptr+(leaf ? 0 : sizeof(tblocknum)) + step*(isleave-1), keysize);
    verify_node(idx_descr, node);
    verify_node(idx_descr, node1);
    *pval=dadr1;
    unfix_page(cdp, fcbn1);
  }
  else /* without dividing nodes */
  { unsigned in_offset = leaf ? step*posit : step*posit+sizeof(tblocknum);
    destin=nptr+in_offset;
    memmov(destin+step, destin, dtsize-in_offset);
    memcpy(destin, key, keysize);
    node->filled+=step;
    if (!leaf) *(tblocknum*)(destin+keysize)=*pval;
  }
  translog->index_page_changed(cdp, root, fcbn);
  return dbl;
}

BOOL bt_insert(cdp_t cdp, tptr key, dd_index * idx_descr, tblocknum root, const char * tabname, t_translog * translog)
/* inserts a new item into B-tree, returns TRUE on error */
/* will overwrite the key ! */
// If the index is shared, it must be locked! when (translog->tl_type==TL_CLIENT)
{ tblocknum bl=0;  
  t_btree_acc bac;
  bt_result res;  uns16 posit;  BOOL nullkey;
  if (!root) return TRUE;   // possible in old FILs - when a new index was added to the table and its definition was the same as of an existing index
  nullkey=is_null_key(key, idx_descr);
  if (nullkey && !idx_descr->has_nulls) return FALSE;   /* never inserted */
  trecnum recnum = *(trecnum*)(key+idx_descr->keysize-sizeof(trecnum));
  res=bac.build_stack(cdp, root, idx_descr, key);
  if (res==BT_ERROR) return TRUE;
  if (DISTINCT(idx_descr))  /* check if same key already exists */
  { trecnum xrec;
    if (bac.get_key_rec(cdp, key, &xrec))
    // will not make any step for DISTINCT
    { bac.close(cdp);  // the rollback is simpler when no node is fixed
      t_exkont_key ekt(cdp, idx_descr, key);
      char buf[30+OBJ_NAME_LEN+20];
      if (cdp->ker_flags & KFL_MASK_DUPLICITY_ERR)
      { sprintf(buf, "Key duplicity in %s (%u)", tabname==NULL ? "(TEMPORARY)" : tabname, xrec);
        cd_dbg_err(cdp, buf);
        return FALSE;  // not rolled back, but the record is not inserted into the index
      }
      sprintf(buf, "%s (%u)", tabname==NULL ? "(TEMPORARY)" : tabname, xrec);
      buf[OBJ_NAME_LEN]=0; // must trim the length!
      SET_ERR_MSG(cdp, buf);  request_error(cdp, KEY_DUPLICITY);
      return TRUE;  // very important: must not continue index opertions after rollback!
    }
  }
  if (res==BT_EXACT_REC)  // EXACT_REC - strange internal error 
  { 
#ifdef STOP  // this is a type of index error which can and should be immediately repaired instead of causing error and rollback  
    char msg[100];
    sprintf(msg, "Btree node duplicity on table %d.", cdp->index_owner);
    cd_dbg_err(cdp, msg);
   // prevent returning the record to the cache of free recods on rollback:
    if (cdp->index_owner!=NOOBJECT)
      if (translog->tl_type!=TL_CURSOR)  // not rolling back
        translog->log.drop_record_ins_info(cdp->index_owner, recnum); 
    SET_ERR_MSG(cdp, tabname);  request_error(cdp, KEY_DUPLICITY);   // rolling back is beter than invalid index state
    goto error;
#else  // the error is repaired by "doing nothing" because the index is exactly in the state we wanted it to be when the function returns
  // the only drawback is that rollback would restore the incosistent state.
  bac.close(cdp);
  return FALSE;
#endif
  }
  /* from now, unlocking on error is not necessary */
  posit=bac.stack[bac.sp].posit;
 /* All the path including the leaf is in the stack. Nothing is fixed. */
 /* inserting "val & key" */
  bac.unfix(cdp);
  if (!fix_priv_page(cdp, bac.stack[bac.sp].nn, &bac.fcbn, translog)) goto error;
  translog->index_page_private(cdp, root, bac.stack[bac.sp].nn);
  while (insert_item(cdp, key, idx_descr, &bl, bac.fcbn, posit, translog, root)) /* marks changed & unfixes */
  { if (bac.sp > 0) /* pop father node from the stack */
    { bac.unfix(cdp);
      bac.sp--;  // not calling drop() because the read-locks along the path are converted
      if (!fix_priv_page(cdp, bac.stack[bac.sp].nn, &bac.fcbn, translog)) /* the page must be locked */
        return TRUE;
      translog->index_page_private(cdp, root, bac.stack[bac.sp].nn);
      posit=bac.stack[bac.sp].posit;
    }
    else /* tree grows, must copy the current root */
    { nodebeg * root_node = (nodebeg*)FRAMEADR(bac.fcbn);
     // alloc a new node and copy the root contents into it:
      tblocknum node_num = translog->alloc_index_block(cdp, root);  
      if (!node_num) goto error;
      tfcbnum new_fcbn=fix_new_page(cdp, node_num, translog);
      if (new_fcbn==NOFCBNUM) goto error;
      nodebeg * new_node=(nodebeg *)FRAMEADR(new_fcbn);
      memcpy(new_node, root_node, IBLOCKSIZE);  /* full root copied to a new node */
      translog->index_page_changed(cdp, root, new_fcbn);
      unfix_page(cdp, new_fcbn);
     // create (almost) empty root in the original root page (changes are already registered for this index node):
      root_node->ptr1=node_num;  /* root is made almost empty */
      root_node->filled=sizeof(tblocknum);
      posit=0;  
    }
  }
  bac.close(cdp);
  return FALSE;
 error:
  bac.close(cdp);
  return TRUE;
}

static BOOL remove_item(cdp_t cdp, t_btree_acc * bac, int keysize, BOOL * to_left, t_translog * translog, tblocknum root)
/* Returns TRUE iff removing must continue (in the direction to_left);
   unfixes bac->fcbn but returns fixed node above in bac->fcbn iff returns TRUE. */
{ BOOL leaf;  unsigned dtsize, br_size, step, bigstep, off;  BOOL joined=FALSE;
  tfcbnum up_fcbn, br_fcbn;  unsigned posit;  tblocknum br_dadr;
  nodebeg * node, * up_node, * br_node;
  node=(nodebeg*)FRAMEADR(bac->fcbn);
  dtsize=node->filled & SIZE_MASK;
  leaf=node->filled & LEAF_MARK;
  bigstep=(step=keysize)+sizeof(tblocknum);  if (!leaf) step=bigstep;
  off=bac->stack[bac->sp].posit*step;
  if (!leaf)  /* inner node */
    if (*to_left || (off+step>dtsize)) off-=keysize;
  if (dtsize>step)
  { dtsize-=step;
    memmov((tptr)&node->ptr1+off, (tptr)&node->ptr1+off+step, dtsize-off);
  }
  else dtsize=0;
  node->filled=dtsize | leaf;
  if (bac->sp && (dtsize < (IBLOCKSIZE-IHDRSIZE)/2))  /* check joining of nodes */
  { posit=bac->stack[bac->sp-1].posit;
    if (!(up_node=(nodebeg*)fix_priv_page(cdp, bac->stack[bac->sp-1].nn, &up_fcbn, translog)))
      return FALSE;
    translog->index_page_private(cdp, root, bac->stack[bac->sp-1].nn);
    if (!dtsize)
    { joined=TRUE;
      *to_left=FALSE;
    }
    else if (posit)  /* check the left brother */
    { br_dadr=*(tblocknum*)((tptr)&up_node->ptr1+(posit-1)*bigstep);
      if (!(br_node=(nodebeg*)fix_priv_page(cdp, br_dadr, &br_fcbn, translog)))
        { unfix_page(cdp, up_fcbn);  return FALSE; }
      translog->index_page_private(cdp, root, br_dadr);
      br_size=br_node->filled & SIZE_MASK;
      if ((br_node->filled & LEAF_MARK) == leaf)
      if (br_size+dtsize+2*keysize+sizeof(tblocknum)<=IBLOCKSIZE-IHDRSIZE)
      { if (!leaf)
        { memcpy((tptr)&br_node->ptr1+br_size,
                 (tptr)&up_node->ptr1+posit*bigstep-keysize, keysize);
          br_size+=keysize;
        }
        memcpy((tptr)&br_node->ptr1+br_size,(tptr)&node->ptr1,dtsize);
        br_node->filled=(uns16)(leaf | (br_size+dtsize));
        translog->index_page_changed(cdp, root, br_fcbn);
        verify_node(bac->idx_descr, br_node);
        joined=TRUE;  *to_left=TRUE;
      }
      unfix_page(cdp, br_fcbn);
    }
    if (!joined && ((uns16)((posit+1)*bigstep) < (up_node->filled & SIZE_MASK)))
    { br_dadr=*(tblocknum*)((tptr)&up_node->ptr1+(posit+1)*bigstep);
      if (!(br_node=(nodebeg*)fix_priv_page(cdp, br_dadr, &br_fcbn, translog)))
        { unfix_page(cdp, up_fcbn);  return FALSE; }
      translog->index_page_private(cdp, root, br_dadr);
      br_size=br_node->filled & SIZE_MASK;
      if ((br_node->filled & LEAF_MARK) == leaf)
      if (br_size+dtsize+2*keysize+sizeof(tblocknum)<=IBLOCKSIZE-IHDRSIZE)
      { memmov((tptr)&br_node->ptr1+dtsize+(leaf ? 0 : keysize),
               (tptr)&br_node->ptr1, br_size);
        if (!leaf)
        { memcpy((tptr)&br_node->ptr1+dtsize,
               (tptr)&up_node->ptr1+posit*bigstep+sizeof(tblocknum),keysize);
          br_size+=keysize;
        }
        memcpy((tptr)&br_node->ptr1,(tptr)&node->ptr1,dtsize);
        br_node->filled=(uns16)(leaf | (br_size+dtsize));
        translog->index_page_changed(cdp, root, br_fcbn);
        verify_node(bac->idx_descr, br_node);
        joined=TRUE;  *to_left=FALSE;
      }
      unfix_page(cdp, br_fcbn);
    }
    if (!joined) unfix_page(cdp, up_fcbn);
  }
  if (joined)
  { tblocknum dadr = bac->stack[bac->sp].nn;
    translog->index_page_changed(cdp, root, bac->fcbn);  // must be called before free_block, otherwise RB considers the page contents valid!!
    bac->unfix(cdp); 
    bac->fcbn=up_fcbn;
    translog->free_index_block(cdp, root, dadr);  // free_block has less to do with removing changes when the block is unfixed before
    return TRUE;
  }
  else
  { translog->index_page_changed(cdp, root, bac->fcbn);  
    bac->unfix(cdp); 
    return FALSE; 
  }
}

BOOL t_btree_acc::bt_remove2(cdp_t cdp, tblocknum root, t_translog * translog)
// Removes the positioned item. Either [shared] is false or RLOCKs have been converted to TMPWLOCKS
// access is open on entry
{ nodebeg * node;  BOOL to_left;
  if (!fix_priv_page(cdp, stack[sp].nn, &fcbn, translog)) goto error;
  translog->index_page_private(cdp, root,  stack[sp].nn);
  to_left=FALSE;
  while (remove_item(cdp, this, idx_descr->keysize, &to_left, translog, root)) 
    drop();  

  if (!sp) /* something removed from the root */
  { if (!(node=(nodebeg*)fix_priv_page(cdp, root, &fcbn, translog)))
      goto error;
    translog->index_page_private(cdp, root, root);
    if (node->filled == sizeof(tblocknum)) /* not leaf && contains 1 ptr */
    { tblocknum sdadr;  tfcbnum sfcbn;  const nodebeg * snode;
      sdadr=node->ptr1;
      snode=(const nodebeg *)fix_any_page(cdp, sdadr, &sfcbn);
      if (!snode) goto error;
      memcpy(node, snode, IBLOCKSIZE);
      unfix_page(cdp, sfcbn);  
      translog->free_index_block(cdp, root, sdadr);
      translog->index_page_changed(cdp, root, fcbn);  
    }
  }
  close(cdp);  // unfixing only when !sp 
  return TRUE;
 error:
  close(cdp);  // unfixing only sometimes 
  return FALSE;
}

BOOL bt_remove(cdp_t cdp, tptr key, dd_index * idx_descr, tblocknum root, t_translog * translog)
/* removes specified item from a B-tree, returns TRUE iff found */
// If the index is shared, it must be locked! when (translog->tl_type==TL_CLIENT)
{ t_btree_acc bac;  
  bt_result res;
  if (!root) return FALSE;   // possible in old FILs - when a new index was added to the table and its definition was the same as of an existing index
  if (!idx_descr->has_nulls)
    if (is_null_key(key, idx_descr)) return TRUE;
  res=bac.build_stack(cdp, root, idx_descr, key);
  if (res!=BT_EXACT_REC)
  { bac.close(cdp);  /* not necessary for BT_ERROR */
    char msg[100];
    sprintf(msg, "Btree node not found on table %d.", cdp->index_owner);
    cd_dbg_err(cdp, msg);
#if 0
    t_dumper dumper(cdp, "c:\\btree.log");
    dumper.out("Key not found: ");
    dumper.dump_key(idx_descr, key);
    dumper.out("\n");
    dumper.dump_tree(cdp, idx_descr, root);
#endif
    
    return FALSE;  /* EXACT_KEY or CLOSEST: key not found */
  }
  bac.unfix(cdp);
  return bac.bt_remove2(cdp, root, translog);
}

static nodebeg * new_btree_block_to_stack(cdp_t cdp, t_btree_stack * stack, int stackpos, t_translog * translog)
{ tblocknum dadr=translog->alloc_block(cdp);
  if (!dadr) return NULL;
  tfcbnum fcbn=fix_new_page(cdp, dadr, translog);
  if (fcbn==NOFCBNUM) return NULL;
  stack[stackpos].dadr=dadr;
  stack[stackpos].fcbn=fcbn;
  nodebeg * node=(nodebeg*)FRAMEADR(fcbn);
  node->filled=0;
  node->index_id=INDEX_ID;
  return node;
}

void add_ordered(cdp_t cdp, tptr key, unsigned keysize, t_btree_stack * stack, int * top, t_translog * translog)
/* stack[0] is the leaf page, stack[top] is the root */
/* LEAF_MARK is not set until the page is saved */
/* Start with *top==-1 */
{ uns8 my_key[MAX_KEY_LEN], div_key[MAX_KEY_LEN];  nodebeg * node;
  tblocknum wr_dadr, olddadr;
  int sp;  BOOL newpage;  unsigned step;
  sp=0;
  memcpy(my_key,key,step=keysize);
  wr_dadr=0;  olddadr=0;  // olddadr init. to avoid a warning
  do /* on the sp level add key & olddadr (iff sp>0) */
  {
    if (sp>=BSTACK_LEN) { request_error(cdp,IE_OUT_OF_BSTACK); return; }
    if (sp>*top)
    { node=new_btree_block_to_stack(cdp, stack, sp, translog);
      if (!node) return;
      *top=sp;
      if (wr_dadr) { node->ptr1=wr_dadr; node->filled+=sizeof(tblocknum); }
    }
    node=(nodebeg*)FRAMEADR(stack[sp].fcbn);
    newpage=node->filled+step > IBLOCKSIZE-IHDRSIZE;
    if (newpage)
    { if (!sp)
      { memcpy(div_key,(tptr)&node->ptr1 + (node->filled-keysize), keysize);
        node->filled |= LEAF_MARK;
      }
      wr_dadr=stack[sp].dadr;
      translog->page_changed(stack[sp].fcbn);  
      unfix_page(cdp, stack[sp].fcbn); 
      node=new_btree_block_to_stack(cdp, stack, sp, translog);
      if (!node) return;
    }
    if (node->filled || !sp)
    { memcpy((tptr)&node->ptr1 + node->filled, my_key, keysize);
      node->filled+=keysize;
    }
    if (sp)
    { *(tblocknum*)((tptr)&node->ptr1 + node->filled)=olddadr;
      node->filled+=sizeof(tblocknum);
    }
    olddadr=stack[sp].dadr;
    memcpy(my_key, div_key, keysize);
    step=keysize+sizeof(tblocknum);
    sp++;
  } while (newpage);
}

tblocknum close_creating(cdp_t cdp, t_btree_stack * stack, int top, t_translog * translog)
{/* add the root block, if it does not exist: */
  if (top < 0)   /* no one record has been added */
  { if (!new_btree_block_to_stack(cdp, stack, 0, translog))
      return 0;
    top=0;
  }
  ((nodebeg*)FRAMEADR(stack[0].fcbn))->filled |= LEAF_MARK;
 /* save all blocks: */
  for (int sp=0;  sp<=top;  sp++)
  { translog->page_changed(stack[sp].fcbn);  
    unfix_page(cdp, stack[sp].fcbn);
  }
  return stack[top].dadr;
}

void nodebeg::init(void)
{ memset(this, 0, IBLOCKSIZE);  /* not necessary */
  filled   = LEAF_MARK;
  index_id = INDEX_ID;
}

bool create_empty_index(cdp_t cdp, tblocknum & dadr, t_translog * translog)
// Allocates & initializes a new empty index root, returns TRUE on error.
{ t_versatile_block ind_acc(cdp, !translog || translog->tl_type==TL_CLIENT);  
  nodebeg * node = (nodebeg *)ind_acc.access_new_block(dadr, translog);
  if (!node) return true;
  node->init();
  ind_acc.data_changed(translog);  // root node will be saved in destructor or on commit
  return false;
}

void drop_index(cdp_t cdp, tblocknum root, unsigned bigstep, t_translog * translog)
// Releases the whole index, including its root
// Ignores invalid block numbers and damaged index nodes in order to make possible to delete/repair damaged tables/indexes.
{ if (root && ffa.is_valid_block(root)) 
  { 
    { t_page_rdaccess rda(cdp);
      const nodebeg * node = (const nodebeg*)rda.fix(root);
      if (node)
      { if ((node->filled & SIZE_MASK) <= IBLOCKSIZE-IHDRSIZE && /* fuse */
            node->index_id == INDEX_ID) 
          if (!(node->filled & LEAF_MARK))
            for (unsigned posit=0;  posit * bigstep < node->filled;  posit++)
            { tblocknum subroot = *(tblocknum*)((tptr)&node->ptr1+posit*bigstep);
              drop_index(cdp, subroot, bigstep, translog);
            }
      }
    } // unfix before free_block()! 
    translog->free_block(cdp, root);  
  }
}

void drop_index_idempotent(cdp_t cdp, tblocknum & root, unsigned bigstep, t_translog * translog)
// Releases the whole index, including its root, transactionally. Stores the fact in the tranaction log.
// Is idempotent!
{ drop_index(cdp, root, bigstep, translog);
  root=0;
}

void truncate_index(cdp_t cdp, tblocknum root, unsigned bigstep, t_translog * translog)
// Releases the index except the root. Inits the root as empty.
{ if (root) 
  { tfcbnum fcbn;  
    nodebeg * node = (nodebeg*)fix_priv_page(cdp, root, &fcbn, translog);
    if (node) 
    { if ((node->filled & SIZE_MASK) <= IBLOCKSIZE-IHDRSIZE && /* fuse */
          node->index_id == INDEX_ID) 
        if (!(node->filled & LEAF_MARK))
          for (unsigned posit=0;  posit * bigstep < node->filled;  posit++)
          { tblocknum subroot = *(tblocknum*)((tptr)&node->ptr1+posit*bigstep);
            drop_index(cdp, subroot, bigstep, translog);
          }
      node->init();
      translog->page_changed(fcbn);
      unfix_page(cdp, fcbn);
    }
  }
}

void count_index_space(cdp_t cdp, tblocknum root, unsigned bigstep, unsigned & block_cnt, unsigned & used_space)
{ 
  if (root && ffa.is_valid_block(root)) 
  { t_page_rdaccess rda(cdp);
    const nodebeg * node = (const nodebeg*)rda.fix(root);
    if (node)
    { if ((node->filled & SIZE_MASK) <= IBLOCKSIZE-IHDRSIZE && /* fuse */
          node->index_id == INDEX_ID) 
      { block_cnt++;
        used_space += (node->filled & SIZE_MASK);
        if (!(node->filled & LEAF_MARK))
          for (unsigned posit=0;  posit * bigstep < node->filled;  posit++)
          { tblocknum subroot = *(tblocknum*)((tptr)&node->ptr1+posit*bigstep);
            count_index_space(cdp, subroot, bigstep, block_cnt, used_space);
          }
      }
    }
  }
}


/**************************** cursor services *********************************/
bool locked_find_key(cdp_t cdp, tblocknum root, dd_index * idx_descr, tptr key, trecnum * precnum)
// Like find_key(), supposes that the index is read-locked.
{ t_btree_acc bac;
  if (bac.build_stack(cdp, root, idx_descr, key)==BT_ERROR) 
    return false;
  return bac.get_key_rec(cdp, key, precnum)!=0;
}

BOOL find_key(cdp_t cdp, tblocknum root, dd_index * idx_descr, tptr key, trecnum * precnum, bool shared_index)
// Searches the index for the given [key] and returns the record number in [*precnum].
// Returns TRUE iff found.
{ t_btree_read_lock btrl;
  if (shared_index)
    if (!btrl.lock_and_update(cdp, root))
      return FALSE;
  return locked_find_key(cdp, root, idx_descr, key, precnum);
}

BOOL distinctor_add(cdp_t cdp, dd_index * index, tptr key, t_translog * translog)
// Not locking the index. Returns TRUE if can add, FALSE if duplicate.
{ BOOL fnd;
  cdp->index_owner = NOOBJECT;
 // search the distinctor:
  { t_btree_acc bac;  
    if (bac.build_stack(cdp, index->root, index, key)==BT_ERROR) 
      return TRUE;
    fnd=bac.get_key_rec(cdp, key, NULL);
  }
 // exit if found:
  if (fnd) return FALSE;
 // add to the distinctor:
  { char keycopy[MAX_KEY_LEN];  
    memcpy(keycopy, key, index->keysize);
    bt_insert(cdp, keycopy, index, index->root, NULL, translog);  // overwrites keycopy!
  }
  return TRUE;
}

///////////////////////////////// find closest key ////////////////////////////////
BOOL find_closest_key(cdp_t cdp, table_descr * td, int indnum, void * search_key, void * found_key)
// Returns number of key parts which are the same.  ??
{ BOOL fnd = FALSE;
  dd_index * indx = &td->indxs[indnum];
  *(trecnum*)((tptr)search_key+indx->keysize-sizeof(trecnum))=0;
  t_btree_read_lock btrl;
  if (!btrl.lock_and_update(cdp, indx->root))
    return FALSE;
  bt_result bres;  t_btree_acc bac;
  bres=bac.build_stack(cdp, indx->root, indx, (const char *)search_key);
  if (bres!=BT_ERROR) 
    if (bac.btree_step(cdp, (char*)found_key)) 
      fnd=TRUE;
  return fnd;
}
////////////////////////////// index interval iterator ////////////////////////////

BOOL t_index_interval_itertor::open(ttablenum tbnum, int indnum, void * startkeyIn, void * stopkeyIn)
{ td = install_table(cdp, tbnum);
  if (!td) return FALSE;
  indx=&td->indxs[indnum];
  startkey=(tptr)startkeyIn;
  stopkey =(tptr)stopkeyIn;
  *(trecnum*)(startkey+indx->keysize-sizeof(trecnum))=0;
  *(trecnum*)(stopkey +indx->keysize-sizeof(trecnum))=(trecnum)-1;
  cdp->index_owner = tbnum;
  if (!btrl.lock_and_update(cdp, indx->root))
    return FALSE;
  bt_result bres;
  bres=bac.build_stack(cdp, indx->root, indx, startkey);
  if (bres==BT_ERROR) eof=TRUE;
  else
  { if (!bac.btree_step(cdp, startkey))
      eof=TRUE;
    else // check the end of the interval (current key returned in stoppoint)
    { sig32 res=cmp_keys_partial(startkey, stopkey, indx, FALSE);
      if (HIWORD(res) ? FALSE : (sig16)res > 0) // close interval and try to open the next one
        eof=TRUE;
      else
      { last_position=*(trecnum*)(startkey+indx->keysize-sizeof(trecnum));
        eof=FALSE;
      }
    }
  } 
  return TRUE;
}

trecnum t_index_interval_itertor::next(void)
{ if (eof) return NORECNUM;
  else
  { trecnum pos=last_position;
    cdp->index_owner = td->tbnum;
    if (!bac.btree_step(cdp, startkey))
      eof=TRUE; 
    else // check the end of the interval (current key returned in stoppoint)
    { sig32 res=cmp_keys_partial(startkey, stopkey, indx, FALSE);
      if (HIWORD(res) ? FALSE : (sig16)res > 0) // close interval and try to open the next one
        eof=TRUE;
      else
      { last_position=*(trecnum*)(startkey+indx->keysize-sizeof(trecnum));
        eof=FALSE;
      }
    }
    return pos;
  }
}

void t_index_interval_itertor::close_passing(void)
// Terminates passing the index, but does not unlock the td.
{ bac.close(cdp); 
  btrl.unlock();
}

void t_index_interval_itertor::close(void)
{ close_passing();
  if (td) { unlock_tabdescr(td);  td=NULL; }
}

/////////////////////////////////////////////////////////////////////////////
int check_node_num(cdp_t cdp, dd_index * indx, tblocknum root)
{ int count;
  t_page_rdaccess rda(cdp);
  const nodebeg * node = (const nodebeg*)rda.fix(root);
  if (node)
  { if (node->filled & LEAF_MARK)
      count = (node->filled & SIZE_MASK) / indx->keysize;
    else
    { count=0;
      unsigned bigstep = indx->keysize+sizeof(tblocknum);
      for (unsigned posit=0;  posit * bigstep < node->filled;  posit++)
      { tblocknum subroot = *(tblocknum*)((tptr)&node->ptr1+posit*bigstep);
        count+=check_node_num(cdp, indx, subroot);
      }
    }
  }
  return count;
}

int check_index(cdp_t cdp, table_descr * tbdf, int indnum)
// returns 1 if OK, 0 for index error, -1 for fre_rec_map error (checked for index 0 only)
{ 
  if (indnum<0 || indnum>=tbdf->indx_cnt)
    return -2;  // index does not exist
  dd_index * indx = &tbdf->indxs[indnum];  
  t_btree_read_lock btrl;
  if (!btrl.lock_and_update(cdp, indx->root))  // covers the whole function, used by t_btree_acc and by locked_find_key()
    return 0;
  trecnum table_recnum = tbdf->Recnum(), aligned_recnum;
  aligned_recnum = tbdf->multpage || table_recnum==0 ? table_recnum : 
                   ((table_recnum-1) / tbdf->rectopage + 1) * tbdf->rectopage;
 // prepare map of records
  uns32 * map = NULL;
  if (indnum==0) 
  { int space = table_recnum / 8 + 4;
    map = (uns32*)corealloc(space, 55);
    if (map) memset(map, 0, space);
  }
  bool res=true;  int map_error=0;
#ifdef STOP // the original slow check          
  // pass all index records:
  char key[MAX_KEY_LEN], compkey[MAX_KEY_LEN];  trecnum position;  unsigned cnt1=0, cnt2=0;
  int part;  int offset;  const part_desc * pd = indx->parts;
  for (part=offset=0;  part<indx->partnum;  offset+=pd->partsize, part++, pd++)
    if (pd->desc)
      qset_max(key+offset, pd->type, pd->partsize);
    else
      qset_null(key+offset, pd->type, pd->partsize);
  *(trecnum*)(key+indx->keysize-sizeof(trecnum)) = 0;
  cdp->index_owner = tbdf->tbnum;
  { t_btree_acc bac;
    bt_result bres = bac.build_stack(cdp, indx->root, indx, key);
    if (bres==BT_ERROR) 
      res=false;
    else if (bac.btree_step(cdp, key))
    { do
      { position=*(trecnum*)(key+indx->keysize-sizeof(trecnum));
        if (table_record_state(cdp, tbdf, position))
          { res=false;  break; }
        if (!compute_key(cdp, tbdf, position, indx, compkey, false))
          { res=false;  break; }
        if (cmp_keys(key, compkey, indx) != MAKELONG(0, 1))
          { res=false;  break; }
        cnt1++;
      } while (bac.btree_step(cdp, key));  
    }
  }
 // find all table_records:
  for (position=0;  position < table_recnum;  position++)
  { uns8 del = table_record_state(cdp, tbdf, position);
    if (!del)
    { if (!compute_key(cdp, tbdf, position, indx, compkey, false))
      { res=false;  break; }
        if (indx->has_nulls || !is_null_key(compkey, indx)) 
        { trecnum frecnum;
          if (!locked_find_key(cdp, indx->root, indx, compkey, &frecnum) || frecnum!=position)
            { res=false;  break; }
          cnt2++;
        }
    }
    if (map && del!=RECORD_EMPTY)
      map[position / 32] |= 1<<(position % 32);
  }
  if (cnt1!=cnt2) 
    res=false;
#else // faster check using the key values loaded into the memory
  char * keybase = (char*)malloc(table_recnum * (1+indx->keysize));
  if (!keybase)
  { cd_dbg_err(cdp, "Cannot get enough memory for the index check");
    return 1; 
  }  
  unsigned keystep = 1+indx->keysize;
 // pass the table and load the key values from it:
  char * keyptr = keybase;  trecnum position;
  for (position=0;  position < table_recnum;  position++)
  { uns8 del = table_record_state(cdp, tbdf, position);
    *keyptr=del; 
    if (!del)
      compute_key(cdp, tbdf, position, indx, keyptr+1, false);
    keyptr+=keystep;
    if (map && del!=RECORD_EMPTY)
      map[position / 32] |= 1<<(position % 32);
  }
 // pass the index and compare values: 
  char key[MAX_KEY_LEN];  
  int part;  int offset;  const part_desc * pd = indx->parts;
  for (part=offset=0;  part<indx->partnum;  offset+=pd->partsize, part++, pd++)
    if (pd->desc)
      qset_max(key+offset, pd->type, pd->partsize);
    else
      qset_null(key+offset, pd->type, pd->partsize);
  *(trecnum*)(key+indx->keysize-sizeof(trecnum)) = 0;
  cdp->index_owner = tbdf->tbnum;
  { t_btree_acc bac;
    bt_result bres = bac.build_stack(cdp, indx->root, indx, key);
    if (bres==BT_ERROR) 
      res=false;
    else if (bac.btree_step(cdp, key))
    { do
      { position=*(trecnum*)(key+indx->keysize-sizeof(trecnum));
        if (position>=table_recnum)
          res=false;
        else  
        { keyptr = keybase + position * keystep;
          if (*keyptr!=NOT_DELETED)
            res=false;
          else if (cmp_keys(key, keyptr+1, indx) != MAKELONG(0, 1))
            res=false;
          else 
            *keyptr=5;  // "found in index" mark, will allow to discover both duplicities and not indexed records
        }    
      } while (bac.btree_step(cdp, key));  
    }
  }
 // looking for table records missing in the index:
  keyptr = keybase;
  for (position=0;  position < table_recnum;  position++)
  { if (*keyptr==NOT_DELETED)  // the key is not in the index
      if (indx->has_nulls || !is_null_key(keyptr+1, indx)) 
        res=false;
    keyptr+=keystep;
  }  
  free(keybase);
#endif
  if (map)  // checking only for index 0
  { if (tbdf->free_rec_cache.list_records(cdp, tbdf, map, NULL))
      map_error=-4;
#ifdef STOP  // this happens
    trecnum rec=0;  bool missing = false;
    while (rec+32 <= table_recnum)
    { if (map[rec / 32] !=0xffffffff) 
        missing=true;
      rec+=32;
    }
    while (rec < table_recnum)
    { if (!(map[rec / 32] & (1<<(rec % 32))))
        missing=true;
      rec++;
    }
    if (missing)
      map_error=-5;
#endif
    for (position=table_recnum+1;  position<aligned_recnum;  position++)
    { uns8 state = table_record_state2(cdp, tbdf, position);
      if (state!=RECORD_EMPTY)
        map_error=-6;
    }
    corefree(map);
  }
  return map_error ? map_error : res ? 1 : 0;
}


int check_index_set2(cdp_t cdp, table_descr * tbdf, int indnum, bool log_it)
{ int res=1;
  if (indnum==-1)
  { for (int i = 0;  i<tbdf->indx_cnt;  i++)
    { int ares = check_index(cdp, tbdf, i);
      if (cdp->get_return_code()==INDEX_DAMAGED)  // convert severe error into a reportable error
        { request_error(cdp, ANS_OK);  ares=0; }
      if (ares!=1) { res=ares;  indnum=i; }
    }
  }
  else  
    res=check_index(cdp, tbdf, indnum);
  if (res!=1)
    if (log_it)
    { char msg[100+2*OBJ_NAME_LEN];
      sprintf(msg, "Damaged index %u on table %s.%s.", indnum, tbdf->selfname, tbdf->schemaname);
      cd_dbg_err(cdp, msg);
    }  
  return res;    
}

int check_index_set(cdp_t cdp, uns8 * schema, ttablenum tabnum, int indnum)
// Verifies index(es): single table iff [tabnum]!=NOOBJECT, tables in schema when [schema]!=NULL, 
//  all tables otherwise. Index is specified by [indnum], checks all indexes when [indnum]==-1.
// Returns 1 iff ok, error number on error.
{
  if (tabnum!=NOOBJECT) 
  { table_descr_auto tbdf(cdp, tabnum);
    return check_index_set2(cdp, tbdf->me(), indnum, false);
  }
  else
  { t_table_record_rdaccess rda(cdp, tabtab_descr);
    for (trecnum rec=0;  rec < tabtab_descr->Recnum();  rec++)
    { const ttrec * dt = (const ttrec*)rda.position_on_record(rec, 0);
      if (dt!=NULL)
      { if (!dt->del_mark)
          if (dt->categ==CATEG_TABLE &&  
              (!schema || !memcmp(dt->apluuid, schema, UUID_SIZE)))
          { table_descr_auto tbdf(cdp, rec);
            int res=check_index_set2(cdp, tbdf->me(), indnum, indnum!=-1);
            if (res!=1) return res;
          }
      }
    }
  }
  return 1;
}      
     

bool check_all_indicies(cdp_t cdp, bool systab_only, int extent, bool repair, int & cache_error, int & index_error)
{ ttablenum tb;  tcateg categ;  
  index_error=0;  cache_error=0;
  char buf[50+2*OBJ_NAME_LEN];
  //EnterCriticalSection(&cs_tables);
  for (tb=0/*TAB_TABLENUM+1*/;  tb<tabtab_descr->Recnum();  tb++)  // TABTAB is the most important table, check added (old FILs had some problems with the self-index)
  { if (systab_only && !SYSTEM_TABLE(tb)) break;
    if (!table_record_state(cdp, tabtab_descr, tb))
      if (!fast_table_read(cdp, tabtab_descr, tb, OBJ_CATEG_ATR, &categ))
        if (!(categ & IS_LINK)) 
        { table_descr_auto tbdf(cdp, tb);
          if (tbdf->me())
          { 
            if (wait_record_lock_error(cdp, tbdf->me(), FULLTABLE, TMPRLOCK)) 
              continue;
            for (int i = 0;  i<tbdf->indx_cnt;  i++)
            { int res = check_index(cdp, tbdf->me(), i);
              if (cdp->get_return_code()==INDEX_DAMAGED)  // convert severe error into a reportable error
                { request_error(cdp, ANS_OK);  res=0; }
              if (res==1) continue;  // no error
              if (!*tbdf->schemaname) get_table_schema_name(cdp, tb, tbdf->schemaname);
              if (!res)
              { if (extent & INTEGR_CHECK_INDEXES)
                { sprintf(buf, "Index %u damaged on table %s.%s!!!", i, tbdf->schemaname, tbdf->selfname);
                  index_error++;
                  if (repair)
                  { enable_index(cdp, tbdf->me(), i, false);
                    enable_index(cdp, tbdf->me(), i, false);
                  }
                }
              }
              else if (res==-4)
              { if (extent & INTEGR_CHECK_CACHE)
                { sprintf(buf, "Free Rec Cache damaged on table %s.%s!!!", tbdf->schemaname, tbdf->selfname);
                  cache_error++;
                  if (repair) free_deleted(cdp, tbdf->me());
                }
              }
              else if (res==-5)
              { if (extent & INTEGR_CHECK_CACHE)
                { sprintf(buf, "Free Rec Cache incomplete on table %s.%s!!!", tbdf->schemaname, tbdf->selfname);
                  cache_error++;
                  if (repair) free_deleted(cdp, tbdf->me());
                }
              }
              else if (res==-6)
              { if (extent & INTEGR_CHECK_CACHE)
                { sprintf(buf, "Unaligned records are not free in table %s.%s!!!", tbdf->schemaname, tbdf->selfname);
                  cache_error++;
                }
              }
              else *buf=0;  // impossible
              dbg_err(buf);
            }
            record_unlock(cdp, tbdf->me(), FULLTABLE, TMPRLOCK);
          }
        }
  }
  //LeaveCriticalSection(&cs_tables);
  return index_error>0 || cache_error>0;
}

