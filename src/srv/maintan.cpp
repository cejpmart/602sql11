/****************************************************************************/
/* maintan.c - maintanance of kernel structures                             */
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
#include "nlmtexts.h"
#include "opstr.h"
#ifndef WINS
#ifdef UNIX
#include <unistd.h>
#else
#include <io.h>
#endif
#include <fcntl.h>
#endif

/****************************************************************************/
/* copy of fix_attr(_ind): uses private page but does not mark changed */

static tptr fix_priv_attr(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib,
              tfcbnum * pfcbn)
{ tfcbnum fcbn0;  tptr dt;
  tblocknum blnum, datadadr;  unsigned offs, chstart, pgrec;

  bas_tbl_block_info * basbl=tbdf->get_basblock();
  if (recnum >= basbl->recnum) { request_error(cdp, RECORD_DOES_NOT_EXIST);  basbl->Release();  return NULL; }
  if (tbdf->multpage)
  { blnum=(tblocknum)(recnum*tbdf->rectopage + tbdf->attrs[attrib].attrpage);
    chstart=pgrec=0;
    offs=tbdf->attrs[attrib].attroffset;
  }
  else /* record not bigger than a page */
  { blnum=(tblocknum)(recnum / tbdf->rectopage);
    pgrec=(unsigned) (recnum % tbdf->rectopage);
    chstart=tbdf->recordsize * pgrec;
    offs=chstart + tbdf->attrs[attrib].attroffset;
  }
  if (basbl->diraddr) datadadr=basbl->databl[(uns16)blnum];
  else  /* indirect block addressing */
  { const tblocknum * datadadrs = (const tblocknum *)fix_any_page(cdp, basbl->databl[blnum/DADRS_PER_BLOCK], &fcbn0); 
    if (!datadadrs) { basbl->Release();  return NULL; }
    datadadr=datadadrs[blnum % DADRS_PER_BLOCK];
    UNFIX_PAGE(cdp, fcbn0);
  }
  basbl->Release();
 /* fix the target page */
  if (!(dt=fix_priv_page(cdp, datadadr, pfcbn, &cdp->tl))) return NULL;
  if (!tbdf->multpage || !tbdf->attrs[attrib].attrpage)  /* 1st page fixed */
    if (dt[chstart]==RECORD_EMPTY)
      { unfix_page(cdp, *pfcbn);  request_error(cdp,EMPTY);  return NULL; }   // not from SQL, can raise error
  return dt+offs;  /* attribute address */
}

static tptr fix_priv_attr_ind(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, uns16 index,
                  tfcbnum * pfcbn)
/* if the attr doesn't exist, it will be created (if possible) on overwrite,
   or the NONE value will be returned otherwise. */
/* returns NULL on error */
{ tptr dt;  tfcbnum fcbn0;
  tblocknum datadadr, blnum;  uns16 offs, p1, as;  attribdef * att;

  // recnum range check is in fix_priv_attr
  att=tbdf->attrs+attrib;
  if (att->attrmult <= 1) { request_error(cdp,BAD_MODIF);  return NULL; }
 /* fixing the starting page (will lock it & marked as chnged on overwrite) */
  dt=fix_priv_attr(cdp, tbdf, recnum, attrib, &fcbn0);
  if (!dt) return NULL;

  if (index < (att->attrmult & 0x7f))  /* value located in the fixed part */
  { if (tbdf->multpage)
    { blnum=(tblocknum)(recnum * tbdf->rectopage + att->attrpage);
      offs=att->attroffset+CNTRSZ;
      as=TYPESIZE(att);
      p1=(BLOCKSIZE-offs) / as; /* p1-number of values in the first page */
      if (index < p1)  /* value is in the same page! */
        { *pfcbn=fcbn0;  return dt + CNTRSZ + index*as; }
     /* another page */
      UNFIX_PAGE(cdp, fcbn0);
      index -= p1;
      p1=BLOCKSIZE/as; /* p1-number of values per full page */
      blnum += index/p1 + 1;
      offs = as * (index % p1);
     /* find the blnum-th block of the table tb */
      bas_tbl_block_info * basbl=tbdf->get_basblock();
      if (basbl->diraddr) datadadr=basbl->databl[blnum];
      else  /* indirect block addressing */
      { const tblocknum * datadadrs = (const tblocknum *)fix_any_page(cdp, basbl->databl[blnum/DADRS_PER_BLOCK], &fcbn0);
        if (!datadadrs) { basbl->Release();  return NULL; }
        datadadr=datadadrs[blnum % DADRS_PER_BLOCK];
        UNFIX_PAGE(cdp, fcbn0);
      }
      basbl->Release();
     /* fix the target page */
      if (!(dt=fix_priv_page(cdp, datadadr, pfcbn, &cdp->tl))) return NULL;
      return dt+offs;  /* attribute address */
    }
    else /* record not bigger than a page */
      { *pfcbn=fcbn0;  return dt + CNTRSZ + index*TYPESIZE(att); }
  }
  else /* accessing the variable-size part of the multiattribute */
  { if (!(att->attrmult & 0x80))
      { unfix_page(cdp, fcbn0); request_error(cdp,INDEX_OUT_OF_RANGE);  return NULL; }
    uns16 sz=(uns16)(TYPESIZE(att));
    if (!(dt=hp_locate_write(cdp, (tpiece*)(dt-sizeof(tpiece)),
       sz, (uns32)(index-(att->attrmult & 0x7f)), pfcbn, table_translog(cdp, tbdf))))
      { unfix_page(cdp, fcbn0);  return NULL; }
    unfix_page(cdp, fcbn0);
    return dt;
  }
}


/****************************************************************************/

static unsigned mpl_size;
static tblocknum mpl_start, dadrlimit;
static piecemap * mpl;
static bool mpl_over;
static di_params * gdi = NULL;
static BOOL contains_big_block;
static bool glob_repair;

struct t_cross_info
{ tblocknum dadr;
  piecemap map;
};
#define MAX_CROSS_INFO 50
t_cross_info cross_info[MAX_CROSS_INFO];
int cross_info_cnt = 1;
int using_crosss_info = -1;  // 0 creating, 1 using

static bool mark_space(cdp_t cdp, tblocknum dadr, piecemap pcs)
// On error the function increaces gdi->cross_link and return true.
{ bool res = false;
  if (dadr<mpl_start) return false;
  dadr-=mpl_start;
  if (dadr>=mpl_size) { mpl_over=true; return false; }
 // chec if the space is not already occupied:
  if ((mpl[dadr] & pcs) != pcs) 
    if (gdi->cross_link!=NULL)
    { uns32 diff = ~mpl[dadr] & pcs;
      if (using_crosss_info==0 && !glob_repair)
      { if (cross_info_cnt<MAX_CROSS_INFO)
        { cross_info[cross_info_cnt].dadr=dadr;
          cross_info[cross_info_cnt].map=diff;
          cross_info_cnt++;
        }
        request_error(cdp, IE_OUT_OF_DWORM);
      }
      while (diff)
      { if (diff & 1) (*gdi->cross_link)++;
        diff>>=1;
      }
      res=true;
    }
  if (using_crosss_info==1 && !glob_repair)
    for (int i=0;  i<cross_info_cnt;  i++)
      if (cross_info[i].dadr==dadr)
        if (cross_info[i].map & pcs)
          request_error(cdp, IE_OUT_OF_DWORM);
 // register the occupied space in the map:
  mpl[dadr] &= ~pcs;
  return res;
}

#define mark_block(cdp, dadr) mark_space(cdp, dadr, (piecemap)-1L)

void mark_used_block(cdp_t cdp, tblocknum dadr)
// Used for marking from other modules
{ mark_block(cdp, dadr); }

bool xmark_piece(cdp_t cdp, const tpiece * pc)
{ return mark_space(cdp, pc->dadr,
        pc->len32 > 16 ? (piecemap)-1L :  /* error in Watcom! */
        (piecemap)((1L << pc->len32) -1)  << pc->offs32 );
};

void mark_basrecs(cdp_t cdp, table_descr * tbdf, di_params * di)
// Marks blocks used by the fixed pars of table records.
// If [glob_repair] then replaces the missing blocks.
{ unsigned i, j;
  bool basblock_changed=false;
  t_versatile_block bas_acc(cdp, tbdf->translog==NULL);
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tbdf->basblockn, tbdf->translog);
  if (basbl->nbnums > MAX_BAS_ADDRS)
  { basbl->nbnums = (uns16)MAX_BAS_ADDRS;
    if (glob_repair) basblock_changed=true;
  }  
  unsigned block_count = tbdf->record_count2block_count(basbl->recnum);
  if (basbl->diraddr)
  { for (i=0;  i<basbl->nbnums;  i++) 
    { if (!basbl->databl[i])
      { if (di->nonex_blocks) (*di->nonex_blocks)++;
        if (glob_repair)  // missing data block 
        { basbl->databl[i] = tbdf->alloc_and_format_data_block(cdp, i);
          basblock_changed=true;
        }  
      }
      if (basbl->databl[i]) 
        mark_block(cdp, basbl->databl[i]);
    }   
  }    
  else  // indirect data blocks
  { unsigned blocknum=0;  tblocknum * dtd;
    for (i=0;  i<basbl->nbnums;  i++) 
    { if (!basbl->databl[i]) 
      { if (di->nonex_blocks) (*di->nonex_blocks)++;  
        if (glob_repair)  // missing b2-block 
        { t_versatile_block b2_acc(cdp, tbdf->translog==NULL);  
          dtd = (tblocknum*)b2_acc.access_new_block(basbl->databl[i], tbdf->translog);
          if (dtd)
          { memset(dtd, 0, BLOCKSIZE);
            basblock_changed=true;
          }  
        }
      }
      if (basbl->databl[i]) 
      { mark_block(cdp, basbl->databl[i]);
        t_versatile_block b2_acc(cdp, tbdf->translog==NULL);  
        dtd = (tblocknum*)b2_acc.access_block(basbl->databl[i], tbdf->translog);
        if (dtd)
        { for (j=0;  j<DADRS_PER_BLOCK;  j++)
            if (blocknum < block_count) 
            { if (!dtd[j])
              { if (di->nonex_blocks) (*di->nonex_blocks)++;  
                if (glob_repair)  // missing data block
                { dtd[j] = tbdf->alloc_and_format_data_block(cdp, blocknum);
                  b2_acc.data_changed(tbdf->translog);
                }
              }
              if (dtd[j]) 
                mark_block(cdp, dtd[j]);
              blocknum++;  
            }
            else break;
        }
      }  
    }  
  }
  if (basblock_changed)  // something repaired, save changes
  { bas_acc.data_changed(tbdf->translog);
    tbdf->update_basblock_copy(basbl);
  }  
}

static unsigned index_size;

static bool mark_index_subtree(cdp_t cdp, tblocknum root, unsigned bigstep)
/* supposes root !=0 */
{ bool res=false;
  if (mark_block(cdp, root)) res=true;
  index_size++;
  t_page_rdaccess rda(cdp);
  const nodebeg * node = (const nodebeg*)rda.fix(root);
  if (!node) return 0;
  if ((node->filled & SIZE_MASK) > IBLOCKSIZE-IHDRSIZE || node->index_id != INDEX_ID) 
  { if (gdi->nonex_blocks) (*gdi->nonex_blocks)++;  // not exactly this error but I must report something, cross_link is too dangerous to be used here
    res=true;
  }
  else  // pass the subtrees:
    if (!(node->filled & LEAF_MARK))
    { int posit=0;
      while ((uns16)(posit * bigstep) < node->filled)
      { tblocknum dadr = *(tblocknum*)((tptr)&node->ptr1+posit*bigstep);
        if (dadr >= dadrlimit) 
          { if (gdi->nonex_blocks) (*gdi->nonex_blocks)++;  res=true; }
        else res|=mark_index_subtree(cdp, dadr, bigstep);
        posit++;
      }
    }
  return res;
}

bool mark_index(cdp_t cdp, const dd_index * cc)
// Used for marking from other modules
{ if (!cc->root) return 0;
  return mark_index_subtree(cdp, cc->root, cc->keysize+sizeof(tblocknum)); 
}

static bool mark_hp(cdp_t cdp, const tpiece * pc)
// If an error occurs and this type of error skould be detected then returns true and increases error counts.
{ bool res=false;
  if (!pc) return 0;
  if (!pc->dadr) return 0;
  if (pc->dadr >= dadrlimit         ||
      pc->offs32 >= BLOCKSIZE / k32 ||
      pc->len32  >  BLOCKSIZE / k32) return 1;
  if (xmark_piece(cdp, pc)) res=true;

  t_dworm_access dwa(cdp, pc, 1, FALSE, NULL);
  if (!dwa.is_open() || dwa.error()) 
  { if (gdi->nonex_blocks) (*gdi->nonex_blocks)++;  
    return true;
  }  
  if (!dwa.null())
  { tblocknum * pdadr, dadr;  unsigned i=0;
    do
    { pdadr = dwa.get_small_block(i++);
      if (!pdadr) break;
      dadr=*pdadr;
      if (dadr >= dadrlimit) 
        { if (gdi->nonex_blocks) (*gdi->nonex_blocks)++;  res=true; }
      else res|=mark_block(cdp, dadr);
    } while (true);
    i=0;
    do
    { dadr=dwa.get_big_block(i++);
      if (!dadr) break;
      if (dadr+SMALL_BLOCKS_PER_BIG_BLOCK-1 >= dadrlimit) 
        { if (gdi->nonex_blocks) (*gdi->nonex_blocks)++;  res=true; }
      else 
      { contains_big_block=TRUE;
        for (unsigned j=0;  j<SMALL_BLOCKS_PER_BIG_BLOCK;  j++)
          res|=mark_block(cdp, dadr+j);
      }
    } while (true);
  }  
  return res;
}

int t_free_rec_pers_cache::mark_free_rec_list(cdp_t cdp, tblocknum basblockn) const
{ tblocknum list_head=free_rec_list_head;  int errs=0;
  if (list_head==(tblocknum)-1) 
  { tfcbnum fcbn;
    const bas_tbl_block * basbl=(const bas_tbl_block*)fix_any_page(cdp, basblockn, &fcbn);
    if (basbl) list_head=basbl->free_rec_list;
    unfix_page(cdp, fcbn);
  }
  if (list_head!=(tblocknum)-1)
    while (list_head)
    { if (list_head>=dadrlimit) { errs=1;  break; }
      mark_block(cdp, list_head);
      tfcbnum fcbn;
      const t_recnum_containter * contr=(const t_recnum_containter*)fix_any_page(cdp, list_head, &fcbn); // non-transactional access
      if (contr==NULL) break;
      list_head=contr->next;
      unfix_page(cdp, fcbn);
    }
  return errs;
}

static BOOL check_all(cdp_t cdp, BOOL repair, di_params * di)
/* removes free pieces from the selected interval from free worms! */
{ int i, indnum;  table_descr * tbdf;  uns8 dltd;
  BOOL var_parts;  tfcbnum tfcbn, fcbn;  trecnum rec;  int errcnt;
  uns16 num, ind;  tptr dt;  heapacc * tmphp;  uns32 roots[INDX_MAX];
  tcateg categ;  attribdef * att;  tattrib atr;  
  cdp->break_request=wbfalse;  contains_big_block=FALSE;
  bool breaked=false;
 /* mark the contents of the tables */
  for (ttablenum tb=last_temp_tabnum;  tb < (ttablenum)tabtab_descr->Recnum();  tb++)
  { if (integrity_check_planning.cancel_request_pending()) { breaked=true;  break; }
    if (tb<0)  // a temporary table
    { tbdf=tables[tb];
      if (!tbdf || tbdf==(table_descr*)-1) continue;
    }
    else  // permanent table
    { const char * tbrec = fix_attr_read(cdp, tabtab_descr, tb, DEL_ATTR_NUM, &tfcbn);
      if (!tbrec) continue;
      if (tbrec[0]) { unfix_page(cdp, tfcbn);  continue; }  // deleted
      unfix_page(cdp, tfcbn);
      report_state(cdp, tb, tabtab_descr->Recnum()-1);
      fast_table_read(cdp, tabtab_descr, tb, OBJ_CATEG_ATR, &categ);
      if (categ!=CATEG_TABLE) continue;   /* a link table: nothing to do */
      tbdf=install_table(cdp, tb);
      if (tbdf==NULL)
      { if (di->damaged_tabdef!=NULL)
        {// write message to the log:
          tobjname tabname;  char msg[50+OBJ_NAME_LEN];
          fast_table_read(cdp, tabtab_descr, tb, OBJ_NAME_ATR, tabname);
          sprintf(msg, damagedTabdef, tabname, tb);
          dbg_err(msg);
          (*di->damaged_tabdef)++;
          if (repair)
          { // I must not free this blocks. Damaged basblock often contains invalid block
            // numbers. When they are released, they may be allocated and may overwrite valid data!!
            char * tbwrrec=fix_priv_attr(cdp,tabtab_descr,tb,DEL_ATTR_NUM,&tfcbn);
            if (tbwrrec)
            { { t_simple_update_context sic(cdp, tabtab_descr, 0);
                sic.inditem_remove(tb, (tptr)1);  // remove from ALL indicies -- otherwise this version creates a TABTAB index inconsistency!
              }
              tbwrrec[0]=DELETED;  // "deleted" attribute
              cdp->tl.page_changed(tfcbn);
              unfix_page(cdp, tfcbn);
              request_error(cdp, 0);
              commit(cdp);
            }
          }  // removing the damaged table
        }
        continue;
      }
    } // permanent table

    { t_exkont_table ekt(cdp, tbdf->tbnum, NORECNUM, NOATTRIB, NOINDEX, OP_READ, NULL, 0);
      ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
     // tbdf is valid now
      mark_block(cdp, tbdf->basblockn);
      mark_basrecs(cdp, tbdf, di);
     /********************** mark indicies **********************************/
      BOOL changed=FALSE;  dd_index * cc;
      for (i=indnum=0, cc=tbdf->indxs;  i<tbdf->indx_cnt;  i++, cc++)
        { if (cc->root)
          { if (integrity_check_planning.cancel_request_pending()) { breaked=true;  break; }
            t_exkont_index ekt(cdp, tbdf->tbnum, indnum);
            index_size=0;
            roots[indnum]=cc->root;
            if (mark_index(cdp, cc))
            { char msg[2*OBJ_NAME_LEN+50];
              sprintf(msg, indexDamaged, cc->constr_name, tbdf->selfname, tb);
              dbg_err(msg);
              if (repair)
              { roots[indnum]=cc->root=0;  /* destroy index root */
                changed=TRUE;
              } 
            }
          }
          indnum++;
        }
      if (changed)  /* save new index roots: */
      { t_independent_block bas_acc(cdp);
        bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tbdf->basblockn);
        if (basbl!=NULL)
        { memcpy(basbl->indroot, roots, sizeof(tblocknum)*INDX_MAX);
          tbdf->update_basblock_copy(basbl);
          bas_acc.data_changed();  // will be saved in destructor
        }
      }
      // mark free record lists:
      errcnt=tbdf->free_rec_cache.mark_free_rec_list(cdp, tbdf->basblockn);
      if (errcnt>0 && di->nonex_blocks!=NULL)
      { *di->nonex_blocks += errcnt;  
        if (repair)
        { tbdf->free_rec_cache.drop_caches();
          t_independent_block bas_acc(cdp);
          bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tbdf->basblockn);
          if (basbl!=NULL)
          { basbl->free_rec_list=(tblocknum)-1;
            bas_acc.data_changed();  // will be saved in destructor
          }
        }
      }
     // mark variable parts of records:
      var_parts=FALSE;
      for (i=1; i<tbdf->attrcnt; i++)
        if (IS_HEAP_TYPE(tbdf->attrs[i].attrtype) || (tbdf->attrs[i].attrmult & 0x80))
          var_parts=TRUE;
      if (var_parts)
        for (rec=0; rec<tbdf->Recnum(); rec++)
        { if (integrity_check_planning.cancel_request_pending()) { breaked=true;  break; }
          if (!tb) report_state(cdp, rec, NORECNUM);
          BOOL data_error = FALSE;
          dltd=table_record_state(cdp, tbdf, rec);
          if ((dltd==NOT_DELETED) || (dltd==DELETED))
          /* mark all dworms in the record: */
            for (atr=1; atr<tbdf->attrcnt; atr++)
            { t_exkont_table ekt(cdp, tbdf->tbnum, rec, atr, NOINDEX, OP_READ, NULL, 0);
              att=tbdf->attrs+atr;
              if (IS_HEAP_TYPE(att->attrtype))
                if (att->attrmult==1)
                { tmphp=(heapacc*)fix_priv_attr(cdp, tbdf, rec, atr, &fcbn);
                  if (tmphp)
                  { if (mark_hp(cdp, &tmphp->pc))
                    { data_error=TRUE;
                      if (repair)
                      { tmphp->len=0;  tmphp->pc.dadr=0;  tmphp->pc.len32=tmphp->pc.offs32=0;
                        cdp->tl.page_changed(fcbn);
                      }
                    }
                    unfix_page(cdp, fcbn);
                  }
                }
                else
                { if (!tb_read_ind_num(cdp, tbdf, rec, atr, &num))
                  {// mark the extended part: 
                    if (att->attrmult & 0x80)
                    { dt=fix_priv_attr(cdp, tbdf, rec, atr, &fcbn);
                      if (dt)
                      { tpiece * apc = (tpiece*)(dt-sizeof(tpiece));
                        if (mark_hp(cdp, apc))
                        { data_error=TRUE;
                          if (repair)
                          { apc->dadr=0;  apc->len32=apc->offs32=0;
                            if (*(uns16*)dt > (att->attrmult & 0x7f))
                                *(uns16*)dt = (att->attrmult & 0x7f);
                            cdp->tl.page_changed(fcbn);
                          }
                        }
                        unfix_page(cdp, fcbn);
                      }
                    }
                   // mark the values
                    for (ind=0; ind<num; ind++)
                    { tmphp=(heapacc*)fix_priv_attr_ind(cdp, tbdf, rec, atr, ind, &fcbn);
                      if (tmphp)
                      { if (mark_hp(cdp, &tmphp->pc))
                        { data_error=TRUE;
                          if (repair)
                          { tmphp->len=0;  tmphp->pc.dadr=0;  tmphp->pc.len32=tmphp->pc.offs32=0;
                            cdp->tl.page_changed(fcbn);
                          }
                        }
                        unfix_page(cdp, fcbn);
                      }
                    }
                  }
                }
              else
                if (att->attrmult & 0x80)
                { dt=fix_priv_attr(cdp, tbdf, rec, atr, &fcbn);
                  if (dt)
                  { tpiece * apc = (tpiece*)(dt-sizeof(tpiece));
                    if (mark_hp(cdp, apc))
                    { data_error=TRUE;
                      if (repair)
                      { apc->dadr=0;  apc->len32=apc->offs32=0;
                        if (*(uns16*)dt > (att->attrmult & 0x7f))
                            *(uns16*)dt = (att->attrmult & 0x7f);
                        cdp->tl.page_changed(fcbn);
                      }
                    }
                    unfix_page(cdp, fcbn);
                  }
                }
            }
          if (data_error)
          { char msg[OBJ_NAME_LEN+50];
            sprintf(msg, dataDamaged, tbdf->selfname, tb, rec);
            dbg_err(msg);
          }
        }
    } // table protected by cs_locks 
    unlock_tabdescr(tb);
    if (repair) commit(cdp);  // prevent errors from discarding the changes
    if (tb>=0) try_uninst_table(cdp, tb);  /* nobody else can be logged */

   /* no dispatch_kernel_control here, this is the only process in the kernel */
    if (cdp->break_request) { breaked=true;  break; }
  }
  disksp.deallocation_move(cdp);
 // mark pages used by open cursors (the control console usualy has some cursors open):
  for (i=0;  i<crs.count();  i++)  /* autom. closing of cursors */
    if (*crs.acc0(i))
    { cur_descr * cd = *crs.acc0(i);
      cd->mark_disk_space(cdp);
    }
 /* mark the worms */
  for (i=0; i<NUM_OF_PIECE_SIZES; i++) mark_hp(cdp, header.pcpcs+i);
  mark_hp(cdp, &header.srvkey);
  mark_free_pieces(cdp, repair!=0);
 /* mark special blocks */
  for (i=0; i<header.bpool_count; i++) mark_block(cdp, 8L*BLOCKSIZE*i);
  for (i=0; i<REMAPBLOCKSNUM;     i++) if (header.remapblocks[i]) // if allocated
    mark_block(cdp, header.remapblocks[i]);
 // mark remaps:
  unsigned count;  tblocknum * list;
  ffa.get_remap_to(&list, &count);
  for (i=0;  i<count;  i++) mark_block(cdp, list[i]-header.bpool_first);
 /* mark cached free blocks: */
  disksp.get_block_buf(&list, &count);
  for (i=0;  i<count;  i++) if (list[i]) mark_block(cdp, list[i]);
  if (breaked)
    { request_error(cdp, REQUEST_BREAKED);  return TRUE; }
  return FALSE;
}

#if NUM_OF_PIECE_SIZES==4
static piecemap patterns[4] = { 0x00ff, 0x000f, 0x0003, 0x0001 };
static uns8     patlen  [4] = { 8,      4,      2,      1      };
#else
static piecemap patterns[5] = { 0xffffL, 0x00ff, 0x000fL, 0x0003L, 0x0001L };
static uns8     patlen  [5] = { 16,      8,      4,       2,       1       };
#endif

BOOL db_integrity(cdp_t cdp, BOOL repair, di_params * di)
/* nonex_blocks and damaged_tabdef are checked/repaired in the 1st pass. */
{ uns8 p,off;
  tblocknum bl, dadr;  tpiece apiece;  piecemap patt;
  unsigned stop, inbl;
  gdi=di;
  if (di->lost_blocks   !=NULL) *di->lost_blocks=0;
  if (di->cross_link    !=NULL) *di->cross_link=0;
  if (di->lost_dheap    !=NULL) *di->lost_dheap=0;
  if (di->nonex_blocks  !=NULL) *di->nonex_blocks=0;
  if (di->damaged_tabdef!=NULL) *di->damaged_tabdef=0;
  glob_repair = repair!=FALSE;
  if (using_crosss_info) 
  { using_crosss_info = 0;
    cross_info_cnt = 0;
    display_server_info("Starting consistency check.");

  }
  else
  { using_crosss_info = 1;
    display_server_info("Starting consistency check - pass 2.");
  }

  dadrlimit = ffa.get_fil_size() - header.bpool_first;
  mpl_start=ffa.first_allocable_block();
  mpl_size = (dadrlimit - mpl_start) * sizeof(piecemap);  // will not set mpl_over if will allocate this amount
  while (!(mpl=(piecemap*)corealloc(mpl_size,OW_MAINT))) 
  { mpl_size=mpl_size/5*4;
    if (mpl_size < 16*1024)
      { corefree(mpl);  return FALSE; }
  }
  mpl_size/=sizeof(piecemap);  /* the number of piecemaps available */
 // must remove allocations of pages in triggers & cached stored procedures:
  uninst_tables(cdp);
  psm_cache.destroy();
  uninst_tables(cdp);
  psm_cache.destroy();
  commit(cdp);  // possible transctional deallocations confirmed
  do
  { memset(mpl,0xff,sizeof(piecemap)*mpl_size);    
    mpl_over=wbfalse;
    BOOL chres=check_all(cdp, repair, di);  /* operation breaked */
    if (chres) goto break_label;
    if (!mpl_start) *mpl=0; // fuse
   /* free blocks must be stored before pieces are! (Storing pieces may
      allocate blocks) */
    int curr_pool=-1;  // number of the pool loaded into corepool, -1 iff not loaded
	  stop=mpl_size;
    if (mpl_start+stop >= dadrlimit) stop=dadrlimit-mpl_start;
    for (bl=0;  bl<stop;  bl++)
    { dadr=mpl_start+bl;
      int pool = dadr / (8L*BLOCKSIZE);
     // if [pool] is not the loaded pool, load it!
      if (pool!=curr_pool)
      { if ((curr_pool!=-1) && repair)
          disksp.write_bpool(corepool, curr_pool);
        curr_pool=pool;
        if (disksp.read_bpool(curr_pool, corepool))
          { corefree(mpl);  return FALSE; }
      }
      inbl=(unsigned)(dadr % (8L*BLOCKSIZE));
      if (mpl[bl]==(piecemap)-1L)  /* the full block is free... */
      { if (!(corepool[inbl/8] & (1 << (inbl%8)))) /* but is not in the pool */
          if (di->lost_blocks!=NULL)
          { (*di->lost_blocks)++;
            if (repair) corepool[inbl/8] |= (1 << (inbl%8));
          }
      }
      else   /* block is not free... */
      { if (corepool[inbl/8] & (1 << (inbl%8)))  /* but is in the pool */
          if (di->cross_link!=NULL)
          { (*di->cross_link)++;
			      if (repair) corepool[inbl/8] &= ~(1 << (inbl%8));
          }
      }
    }
    if ((curr_pool!=-1) && repair)
      disksp.write_bpool(corepool, curr_pool);

   /* storing free pieces */
    if (di->lost_dheap!=NULL)
      for (bl=0;  bl<stop;  bl++)
        if (mpl[bl]!=(piecemap)-1L && mpl[bl])  /* the block is partially free */
          for (p=0;  mpl[bl] && (p<NUM_OF_PIECE_SIZES);  p++)
          { patt=patterns[p];
            off=0;
            do
            { if ((mpl[bl] & patt) == patt)
              { mpl[bl] ^= patt;
                (*di->lost_dheap)+=patlen[p];
                if (repair)
                { apiece.dadr=bl+mpl_start;
                  apiece.len32=patlen[p];
                  apiece.offs32=off;
                  if (is_trace_enabled_global(TRACE_PIECES)) 
                  { char msg[81];
                    trace_msg_general(cdp, TRACE_PIECES, form_message(msg, sizeof(msg), "%d,%d,%d,piece released in repair", apiece.dadr, apiece.offs32, apiece.len32), NOOBJECT);
                  }
                  release_piece(cdp, &apiece, true);
                }
              }
#if NUM_OF_PIECE_SIZES==4
              if (patt & 0x8000) break;
#else
              if (patt & 0x80000000UL) break;
#endif
              patt *= (patterns[p]+1); off+=patlen[p];
            } while (TRUE);
          }

   /* to the next cycle: */
    mpl_start+=mpl_size;
    if (integrity_check_planning.cancel_request_pending())
      { request_error(cdp, REQUEST_BREAKED);  goto break_label; }
  } while (mpl_over && mpl_start < dadrlimit);
  corefree(mpl);
  request_error(cdp, 0);   /* resets errors */
  if (di->lost_dheap && *di->lost_dheap <= 2)
    *di->lost_dheap=0; /* initially 2 lost pieces */
  return FALSE;
 break_label:
  corefree(mpl);
  return TRUE;
}
//////////////////////////////// compacting the database ////////////////////////
/* Postup:
- najdu nejvyse MAX_TRANS_COMP volnych bloku na zacatku FILu a obszenych na konci
- vytvorim pritom prevodni tabulku
- prepisu bloky a uzavru transakci
- projdu vse a nahrazuji odkazy na bloky, pritom musim napred prelozit cislo
  bloku a az pak pripadne blok cist
- uzavru transakci
- zaznamenam zmeny do poolu, kdyz transkace dopadle dobre (staci uvolneni, 
  alokace zaznamena v predchozi transakci)
To opakuji, dokud se uplatnilo omezeni MAX_TRANS_COMP

Pri prochazeni databaze, abych napred menil odkazy na bloky a pak tyto bloky
(a tedy updatoval jejich presunutou verzi), postupuji takto:
- cisla basbloku nahradim pri pruchodu zaznamu TABTAB, ostatni tabulky 
  v te dobe nejsou instalovany
- pri prochazeni hodnot atributu napred nahradim extenzi multiatributu 
  a az pak prochazim hodnoty multiatributu,
- pri prochazeni objektu promenne velikosti napred nahradim cislo bloku 
  v zakladim piece a az pak v tomto piece nahrazuji pripadna cisla celych bloku

*/

#define MAX_TRANS_COMP 60000
struct t_comp_pair
{ tblocknum origblock;
  tblocknum newblock;
};
static t_comp_pair * acomp;
static unsigned pairs;

BOOL translate(tblocknum & dadr)
// Translates *dadr, return TRUE iff translated.
{ for (unsigned ind=0;  ind < pairs;  ind++) 
    if (acomp[ind].origblock==dadr)
      { dadr=acomp[ind].newblock;  return TRUE; }
  return FALSE;
}

static void translate_index(cdp_t cdp, tblocknum root, unsigned bigstep)
/* supposes root !=0 */
{ tfcbnum fcbn;  nodebeg * node;  BOOL tr;
  node=(nodebeg*)fix_priv_page(cdp, root, &fcbn, &cdp->tl);
  if (!node) return;
  if (!(node->filled & LEAF_MARK))
  { if (node->filled > IBLOCKSIZE-IHDRSIZE ||
        node->index_id != INDEX_ID) { unfix_page(cdp, fcbn);  return; }
    tr=FALSE;
    for (int posit=0;  (uns16)(posit * bigstep) < node->filled;  posit++)
    { tblocknum * pdadr = (tblocknum*)((tptr)&node->ptr1+posit*bigstep);
      if (*pdadr < dadrlimit)
      { tr|=translate(*pdadr);
        translate_index(cdp, *pdadr, bigstep);
      }
    }
    if (tr) cdp->tl.page_changed(fcbn);
  }
  unfix_page(cdp, fcbn);
}

BOOL t_free_rec_pers_cache::translate_free_rec_list(cdp_t cdp, trecnum & free_rec_list)
{ BOOL tr;  tblocknum list_head=0;
  if (free_rec_list && free_rec_list!=-1)
  { tr=translate(free_rec_list);
    list_head=free_rec_list; 
  }
  else if (free_rec_list_head && free_rec_list_head!=-1)
  { tr=translate(free_rec_list_head);
    list_head=free_rec_list_head; 
  }
  else tr=FALSE;
 // pass the list:
  if (list_head) // then !=-1
    while (list_head)
    { tfcbnum fcbn;
      t_recnum_containter * contr=(t_recnum_containter*)fix_priv_page(cdp, list_head, &fcbn, &cdp->tl); // non-transactional access
      if (contr==NULL) break;
      if (contr->next)
        if (translate(contr->next))
          cdp->tl.page_changed(fcbn);
      list_head=contr->next;
      unfix_page(cdp, fcbn);
    }
  return tr;
}

static void translate_base_blocks(cdp_t cdp, table_descr * tbdf)
{ unsigned i, nblimit;
  BOOL tr;
  ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
  t_independent_block bas_acc(cdp);
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tbdf->basblockn);
  if (basbl!=NULL)
  { nblimit=basbl->nbnums <= MAX_BAS_ADDRS ? basbl->nbnums : MAX_BAS_ADDRS;
    tr=FALSE;
   // translate near blocks:
    for (i=0;  i<nblimit;  i++) 
      tr|=translate(basbl->databl[i]);
   // translate indicies:
    dd_index * cc;  unsigned indnum;
    for (indnum=0, cc=tbdf->indxs;  indnum<tbdf->indx_cnt;  indnum++, cc++)
    { if (basbl->indroot[indnum])
      { tr|=translate(basbl->indroot[indnum]);
        translate_index(cdp, basbl->indroot[indnum], cc->keysize+sizeof(tblocknum));
        cc->root=basbl->indroot[indnum];
      }
    }
   // translate the head of free block list:
    tr|=tbdf->free_rec_cache.translate_free_rec_list(cdp, basbl->free_rec_list);
   // register the change in backblock, if any:
    if (tr) bas_acc.data_changed();  // will be saved in destructor
   // translate 2nd level data blocks:
    if (!basbl->diraddr)
    { unsigned block_count = tbdf->record_count2block_count(basbl->recnum);
      for (i=0;  i<nblimit;  i++)
      { t_independent_block b2_acc(cdp);
        tblocknum * dtd = (tblocknum*)b2_acc.access_block(basbl->databl[i]);
        if (dtd!=NULL)
        { tr=FALSE;
          for (int j=0;  j<DADRS_PER_BLOCK;  j++)
            if (block_count) 
            { tr|=translate(dtd[j]);
              block_count--; 
            }
            else break;
          if (tr) b2_acc.data_changed();
          b2_acc.close();  // saves iff [tr]
        }
      }
    }
    tbdf->update_basblock_copy(basbl);
  }
}

static BOOL translate_hp(cdp_t cdp, tpiece * pc)
// Returns TRUE iff pc changed!
{ if (!pc->dadr) return FALSE;
  if (pc->dadr >= dadrlimit         ||
      pc->offs32 >= BLOCKSIZE / k32 ||
      pc->len32  >  BLOCKSIZE / k32) return FALSE;
  BOOL ret=translate(pc->dadr);
  BOOL tr=FALSE;
  t_dworm_access dwa(cdp, pc, 1, TRUE, &cdp->tl);
  if (dwa.is_open() && !dwa.null())
  { tblocknum * pdadr;  unsigned i=0;
    do
    { pdadr = dwa.get_small_block(i++);
      if (!pdadr) break;
      if (*pdadr < dadrlimit) 
        tr |= translate(*pdadr);
    } while (TRUE);
    if (tr) //cdp->tl.log.page_changed(pc->dadr);
      dwa.piece_changed(cdp, pc);
  }
  return ret;
}

static BOOL translate_links(cdp_t cdp)
// Logic: the web of links between block is translated from its root to the leaves, so the changes are always written into 
//   the NEW location of the block.
//   Baseblock numbers are translated when processing TABTAB, must not use the numbers stored in the installed table descrs then.
// The function must be used with care, fcbs is not protected here by cs_frame.
{ table_descr * tbdf;  uns8 dltd;  tfcbnum tfcbn, fcbn; 
  ttablenum tb;  const char * tbrec;  tcateg categ;  int i;
  int tabcount=0;
 // translate all tables:
  for (tb=0;  tb < tabtab_descr->Recnum();  tb++)
  {// check if table tb exists and in not a link: 
    tbrec=fix_attr_read(cdp,tabtab_descr,tb,DEL_ATTR_NUM,&tfcbn);
    if (!tbrec) continue;
    if (tbrec[0]) { unfix_page(cdp, tfcbn);  continue; }  // deleted
    unfix_page(cdp, tfcbn);
    if (++tabcount>5) { commit(cdp);  tabcount=0; }
    report_state(cdp, tb, tabtab_descr->Recnum()-1);
    fast_table_read(cdp, tabtab_descr, tb, OBJ_CATEG_ATR, &categ);
    if (categ!=CATEG_TABLE) continue;   /* a link table: nothing to do */
   // re-install it:
    if (tb>USER_TABLENUM) force_uninst_table(cdp, tb);  // in installed, may contain the old tbdf->basblockn!
    tbdf=install_table(cdp, tb);
    if (tbdf!=NULL)
    { translate_base_blocks(cdp, tbdf);  // include translation of indicies
     /* variable parts of records */
      BOOL var_parts=FALSE;
      for (i=1; i<tbdf->attrcnt; i++)
        if (IS_HEAP_TYPE(tbdf->attrs[i].attrtype) || (tbdf->attrs[i].attrmult & 0x80))
          var_parts=TRUE;
      if (var_parts)
        for (trecnum rec=0;  rec<tbdf->Recnum();  rec++)
        { dltd=table_record_state(cdp, tbdf, rec);
          if ((dltd==NOT_DELETED) || (dltd==DELETED))
          { for (int atr=1;  atr<tbdf->attrcnt;  atr++)
            { attribdef * att=tbdf->attrs+atr;
             // expandable multiattribute:
              if (att->attrmult & 0x80)
              { tptr dt=fix_priv_attr(cdp, tbdf, rec, atr, &fcbn);
                if (dt!=NULL)
                { tpiece * apc = (tpiece*)(dt-sizeof(tpiece));
                  if (translate_hp(cdp, apc))
                    cdp->tl.log.rec_changed(fcbn, fcbs[fcbn].blockdadr, tbdf->recordsize, rec % tbdf->rectopage);
                  unfix_page(cdp, fcbn);
                }
              }
             // var-len attribute or multiattribute:
              if (IS_HEAP_TYPE(att->attrtype))
                if (att->attrmult==1)
                { heapacc * tmphp=(heapacc*)fix_priv_attr(cdp, tbdf, rec, atr, &fcbn);
                  if (tmphp!=NULL)
                  { if (translate_hp(cdp, &tmphp->pc))
                      cdp->tl.log.rec_changed(fcbn, fcbs[fcbn].blockdadr, tbdf->recordsize, rec % tbdf->rectopage);
                    unfix_page(cdp, fcbn);
                  }
                }
                else
                { t_mult_size num;  t_mult_size ind;
                  if (!tb_read_ind_num(cdp, tbdf, rec, atr, &num))
                    for (ind=0;  ind<num;  ind++)
                    { heapacc * tmphp=(heapacc*)fix_priv_attr_ind(cdp, tbdf, rec, atr, ind, &fcbn);
                      if (tmphp!=NULL)
                      { translate_hp(cdp, &tmphp->pc);
                          cdp->tl.log.rec_changed(fcbn, fcbs[fcbn].blockdadr, tbdf->recordsize, rec % tbdf->rectopage);
                        unfix_page(cdp, fcbn);
                      }
                    }
                }
            } // cycle on attributes
          } // record not empty
          if (!tb && dltd==NOT_DELETED)  // on tabtab translate basblock numbers & index roots
          { ttrec * tdf = (ttrec*)fix_priv_attr(cdp, tbdf, rec, DEL_ATTR_NUM, &fcbn);
            if (tdf!=NULL)
            { if (translate(tdf->basblockn))
              { cdp->tl.log.rec_changed(fcbn, fcbs[fcbn].blockdadr, tbdf->recordsize, rec % tbdf->rectopage);
                if (tables[rec]) 
                  tables[rec]->basblockn = tdf->basblockn;  // update basblock number in OBJTAB, USERTAB etc.
              }
              unfix_page(cdp, fcbn);
            }
          }
        } // cycle on records

      unlock_tabdescr(tb);
      try_uninst_table(cdp, tb);  /* nobody else can be logged */
    } // table installed
  } // cycle on tables

  disksp.deallocation_move(cdp);
 // translate free blocks in the buffer:
  unsigned count;  tblocknum * list;
  disksp.get_block_buf(&list, &count);
  for (i=0;  i<count;  i++) if (list[i]) translate(list[i]); // should be empty
  { ProtectedByCriticalSection cs(&ffa.remap_sem);  // protects the access to the [header]
   // translate lists of free pieces:
    BOOL tr=FALSE;
    for (i=0; i<NUM_OF_PIECE_SIZES; i++) 
      tr|=translate_hp(cdp, header.pcpcs+i);
    if (tr) ffa.write_header();
    translate_free_pieces(cdp);
   // translate remaps:
    tr=FALSE;
    for (i=0; i<REMAPBLOCKSNUM;  i++) if (header.remapblocks[i]) // if allocated
      tr|=translate(header.remapblocks[i]);
    if (tr) ffa.write_header();
   // translate remapped blocks:
    tr=FALSE;
    ffa.get_remap_to(&list, &count);
    for (i=0;  i<count;  i++) 
    { list[i]-=header.bpool_first;
      tr|=translate(list[i]);
      list[i]+=header.bpool_first;
    }
    if (tr) ffa.resave_remap();  // resave
  }
  return FALSE;
}

void compact_database(cdp_t cdp, sig32 margin)
{ tblocknum startblk, stopblk, newend;
 // look for big blocks:
  di_params di;
  di.lost_blocks   =NULL;  di.cross_link    =NULL;
  di.lost_dheap    =NULL;  di.nonex_blocks  =NULL;  di.damaged_tabdef=NULL;
  if (db_integrity(cdp, FALSE, &di)) return;
  if (contains_big_block) 
  { dbg_err("Cannot compact the database file because it contais too big LOBs.");
    return;
  }
 // uninst tables, basblock number copy must not exist:
 // must be done before preparing block displacement because uninstalling allocates disk blocks for free record list
  for (ttablenum tb=USER_TABLENUM+1;  tb<tabtab_descr->Recnum();  tb++) 
    if (tables[tb]!=NULL && tables[tb]!=(table_descr*)-1) force_uninst_table(cdp, tb);

  disksp.close_block_cache(cdp);
  dadrlimit=ffa.get_fil_size() - header.bpool_first;
  newend=dadrlimit;
  stopblk=dadrlimit-1;  startblk=0;
  tblocknum bpool_mask = (BLOCKSIZE * 8) - 1;
  void * acomp_handle=aligned_alloc(MAX_TRANS_COMP * sizeof(t_comp_pair), (tptr*)&acomp);
  if (!acomp_handle) return; 
  bool firstpass = true;
  do
  { unsigned ind;
   // create the pairs:
    int curr_pool, pool, inpool;
    curr_pool=-1;
   // search the used blocks near the end:
    ind=0;
    while (ind<MAX_TRANS_COMP && startblk<stopblk)
    { pool = stopblk / (8L*BLOCKSIZE);  inpool = stopblk % (8L*BLOCKSIZE);
      if (pool!=curr_pool)
      { if (disksp.read_bpool(pool, corepool))
          goto err; 
        curr_pool=pool;
      }
      if (!(corepool[inpool / 8] & (1 << (inpool % 8)))) // block not in pool, is used
        if (stopblk & bpool_mask)  // not a bpool block
        { if (!ind && firstpass) newend=stopblk+1;  // can trim the db file even if no block moved
          acomp[ind++].origblock = stopblk;
        }
      stopblk--;
    }
   // search the free blocks near start, allocate them:
    pairs=0;  BOOL current_pool_changed=FALSE;
    while (pairs<ind && startblk<acomp[pairs].origblock) // searching below the next used block to be moved
    { pool = startblk / (8L*BLOCKSIZE);  inpool = startblk % (8L*BLOCKSIZE);
      if (pool!=curr_pool)
      { if (curr_pool!=-1 && current_pool_changed)
          disksp.write_bpool(corepool, curr_pool);
        if (disksp.read_bpool(pool, corepool))
          goto err; 
        curr_pool=pool;  current_pool_changed=FALSE;
      }
      if (corepool[inpool / 8] & (1 << (inpool % 8))) // block in pool, is free
        if (startblk & bpool_mask)  // not a bpool block (fuse only)
        { newend = acomp[pairs].origblock;  // will be unused
          acomp[pairs++].newblock = startblk;
          corepool[inpool / 8] &= ~(1 << (inpool % 8));
          current_pool_changed=TRUE; 
        }
      startblk++;
    }
    if (curr_pool!=-1 && current_pool_changed)
      disksp.write_bpool(corepool, curr_pool);
    if (pairs<ind)  // startblk>=acomp[pairs].origblock
      if (startblk==acomp[pairs].origblock) newend=startblk+1;
      else newend=startblk;  // startblk > acomp[pairs].origblock
   // move table completed, move block contents:
    for (ind=0;  ind < pairs;  ind++)
    { tfcbnum dstfcbn, srcfcbn;
      const char * dtsrc = fix_any_page(cdp, acomp[ind].origblock, &srcfcbn);
      if (dtsrc==NULL) goto err;
      dstfcbn = fix_new_page(cdp, acomp[ind].newblock, &cdp->tl);
      if (dstfcbn==NOFCBNUM) { unfix_page(cdp, srcfcbn);  goto err; }
      memcpy(FRAMEADR(dstfcbn), dtsrc, BLOCKSIZE);
      cdp->tl.page_changed(dstfcbn);
      unfix_page(cdp, srcfcbn);  unfix_page(cdp, dstfcbn);
    }
    commit(cdp);
   // update all database links:
    if (translate_links(cdp)) goto err;
    commit(cdp);
   // release the moved blocks: insert to the pools:
    curr_pool=-1;  current_pool_changed=FALSE;
    for (ind=0;  ind < pairs;  ind++)
    { pool = acomp[ind].origblock / (8L*BLOCKSIZE);  inpool = acomp[ind].origblock % (8L*BLOCKSIZE);
      if (pool!=curr_pool)
      { if (curr_pool!=-1 && current_pool_changed)
          disksp.write_bpool(corepool, curr_pool);
        if (disksp.read_bpool(pool, corepool))
          goto err; 
        curr_pool=pool;  current_pool_changed=FALSE;
      }
      corepool[inpool / 8] |= 1 << (inpool % 8);
      current_pool_changed=TRUE;
    }
    if (curr_pool!=-1 && current_pool_changed)
      disksp.write_bpool(corepool, curr_pool);
    firstpass = false;
  } while (pairs==MAX_TRANS_COMP);
 // make the database file shorter:
  if (cdp->is_an_error()) goto err;
  if (margin>0) newend+=margin;
  ffa.truncate_database_file(newend);
 err:
  aligned_free(acomp_handle);
}
///////////////////////// restoring the broken transaction /////////////////////////////////////
BOOL restoring_actions(cdp_t cdp)
// Moves the transaction blocks of open transactions.
// Does nothing if the block is unreadable (but doesn't write it). 
{ BOOL err=FALSE;
  DWORD length=GetFileSize(ffa.t_handle, NULL);
  length /= BLOCKSIZE;
  tblocknum tjc_block_limit = length ? ((length-1) & 0xffffffe0) + 1 : 0;
  if (tjc_block_limit)
  { tblocknum * core_tjc = (tblocknum*)corealloc(BLOCKSIZE, 88);
    tptr core_block = (tptr)corealloc(BLOCKSIZE, 88);
    if (core_tjc!=NULL && core_block!=NULL)
    { for (tblocknum tjc_block=0;  tjc_block<tjc_block_limit;  tjc_block+=32)
        if (!transactor.load_block(core_tjc, tjc_block))
          if (core_tjc[0]==VALID_TJC_LEADER)  // process the chain
          { int tjc_pos, tjc_max=BLOCKSIZE / sizeof(tblocknum);
            do
            {// process the pairs;
              for (tjc_pos=2;  tjc_pos<tjc_max && core_tjc[tjc_pos]!=(tblocknum)-1;  tjc_pos+=2)
                if (!transactor.load_block(core_block, core_tjc[tjc_pos+1]))
                  ffa.write_block(cdp, core_block, core_tjc[tjc_pos]);
             // find the next tjc block in the chain and load it:
              if (core_tjc[1]==(tblocknum)-1 || core_tjc[1]>=tjc_block_limit) break;
              if (transactor.load_block(core_tjc, core_tjc[1])) break;
            } while (core_tjc[0]==VALID_TJC_DEPENDENT);
           // transaction restored, invalidate tjc leader block:
            transactor.invalidate_tjc_block(tjc_block);
          }
    }
    else err=TRUE;
    corefree(core_tjc);  corefree(core_block);
  }
  return err; 
}     
  
//////////////////////////////////////////////////////////////////////////////
// Potrebuji: Najit vyskyty stejne stranky u vsech klientu ->proto neni
//  dobre oddelit swapovani kazdeho klienta.
// Potrebuji take najit vsechny zmenene bloky jednim klientem -> dodatecna
//  evidence chngblocks (rostouci hash tables).

///////////////// planning and synchonising the integrity checks of the database file ///////////////
void t_integrity_check_planning::open(void)
{ 
  last_check_or_server_startup=stamp_now(); 
  CreateLocalManualEvent(TRUE, &hCheckNotRunning);
  cancel_request_counter=running_threads_counter=0;
  CreateSemaph(1, 1, &hCheckSemaphore);
}

void t_integrity_check_planning::close(void)
{ CloseLocalManualEvent(&hCheckNotRunning);  SetLocEventInvalid(hCheckNotRunning);
  CloseSemaph(&hCheckSemaphore);             SetSemaphoreInvalid(hCheckSemaphore);
}

bool cd_t::nonempty_transaction(void)
{ if (tl.nonempty()) return true;
  for (t_translog * subtl = subtrans;  subtl;  subtl=subtl->next)
    if (subtl->log.nonempty()) return true;
  return false;
}

bool t_integrity_check_planning::uncheckable_state(void)
// Check cannot be performed if any transaction or cursor is open
{// check for open cursors:
  if (count_cursors((t_client_number)-1) > 0)
  { dbg_err("Cannot check the integrity, cursors are opened");
    return true;
  }
 // check for open transactions:
  char msg[150];
  *msg=0;
  { ProtectedByCriticalSection cs(&cs_client_list);  
    for (int i=1;  i <= max_task_index;  i++)
      if (cds[i])
        if (cds[i]->nonempty_transaction())
        { sprintf(msg, "Thread %s has uncommitted transaction.", cds[i]->thread_name);
          break;
        }  
  }
  if (*msg)
    dbg_err(msg);
  return *msg!=0;
}

BOOL t_integrity_check_planning::try_starting_check(bool strong, bool from_client)
{ int thread_limit = from_client ? 1 : 0;
 // guarantee the exclusivity of the check: 
  if (WaitSemaph(&hCheckSemaphore, 0)!=WAIT_OBJECT_0)
    return FALSE;
  is_strong_check=strong;
  if (running_threads_counter > thread_limit) 
  { dbg_err("Cannot check the integrity, a thread is running");
    goto cannot;
  }  
  ResetLocalManualEvent(&hCheckNotRunning);  // closes the gate for clients     
  if (uncheckable_state() || running_threads_counter > thread_limit) 
    { SetLocalManualEvent(&hCheckNotRunning); goto cannot; }
  last_check_cancelled=false;
  return TRUE;
 cannot:
  ReleaseSemaph(&hCheckSemaphore, 1);
  return FALSE;
}

void t_integrity_check_planning::check_completed(void)
{ if (!cancel_request_counter)
    last_check_or_server_startup=stamp_now();
  SetLocalManualEvent(&hCheckNotRunning); 
  ReleaseSemaph(&hCheckSemaphore, 1);  // must be after setting the event, otherwise other check may Reset it before the setting
}
  
t_integrity_check_planning integrity_check_planning;

