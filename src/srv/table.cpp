/****************************************************************************/
/* Table.cpp - table operations                                             */
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
#include "nlmtexts.h"
#include "worm.h"
#include "opstr.h"
#include "replic.h"
#include <stddef.h>
#include <stdlib.h>
#define CHECK_TBLOCK  1

CRITICAL_SECTION cs_tables;
CRITICAL_SECTION cs_deadlock;
LocEvent hTableDescriptorProgressEvent = 0;

char anonymous_name[OBJ_NAME_LEN+1] = "Anonymous"; 
char nobody_name   [OBJ_NAME_LEN+1] = "Nobody";
char unknown_name  [OBJ_NAME_LEN+1] = "Unknown";

static const uns8 append_opcode=OP_APPEND, insert_opcode=OP_INSERT,
                  del_opc      =OP_DELETE, undel_opc    =OP_UNDEL,
                  opcode_write =OP_WRITE,
                  insdel_opcode=OP_INSDEL;
static const uns8 mod_len  = MODLEN, mod_int = MODINT, mod_ind = MODIND,
                  mod_stop = MODSTOP;
static BOOL tb_write_ind_pure(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, const void * ptr);
static BOOL tb_write_ind_var_pure(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 start, uns32 size, const void * ptr);

inline tblocknum ACCESS_COL_VAL(cdp_t cdp, table_descr * tbdf, trecnum recnum, int col_page, unsigned & chstart, unsigned & pgrec) 
{ tblocknum blnum, datadadr;   
  if (tbdf->multpage)              
  { blnum=(tblocknum)(recnum*tbdf->rectopage + col_page);
    chstart=pgrec=0;                                 
  }                                                  
  else /* record not bigger than a page */           
  { blnum=(tblocknum)(recnum / tbdf->rectopage);     
    pgrec=(unsigned) (recnum % tbdf->rectopage);     
    chstart=tbdf->recordsize * pgrec;                
  }
  bas_tbl_block_info * bbl = tbdf->get_basblock();
  if (bbl->diraddr) 
  { if (blnum >= bbl->nbnums) 
      { bbl->Release();  request_error(cdp, RECORD_DOES_NOT_EXIST);  return 0; }
    datadadr=bbl->databl[blnum]; 
  }
  else  /* indirect block addressing */              
  { unsigned section = blnum/DADRS_PER_BLOCK;
    if (section >= bbl->nbnums) 
      { bbl->Release();  request_error(cdp, RECORD_DOES_NOT_EXIST);  return 0; }
    tfcbnum fcbn0;
    const tblocknum * datadadrs = (const tblocknum *)fix_any_page(cdp, bbl->databl[section], &fcbn0);
    if (!datadadrs) { bbl->Release();  return 0; }
    datadadr = datadadrs[blnum % DADRS_PER_BLOCK];
    UNFIX_PAGE(cdp, fcbn0);                                 
  } 
  bbl->Release();                                                   
  if (!datadadr)   /* used after applying BERLE when recnum is too big */ 
    { request_error(cdp, RECORD_DOES_NOT_EXIST);  return 0; }
  return datadadr;
}

tptr fix_attr_x(cdp_t cdp, table_descr * tbdf, trecnum recnum, tfcbnum * pfcbn);

BOOL link_table_in(cdp_t cdp, table_descr * tbdf, ttablenum tbnum)
{ char * names;  ttablenum tabnum;  attribdef * att;
  BOOL linked=TRUE;  unsigned atr;
  names=(tptr)(tbdf->attrs+tbdf->attrcnt);
 /* linking tables */
  for (atr=1; atr<tbdf->attrcnt; atr++)
  { att=tbdf->attrs+atr;
    if ((att->attrtype==ATT_PTR) || (att->attrtype==ATT_BIPTR))
    { if (att->attrspecif.destination_table==SELFPTR) att->attrspecif.destination_table=tbnum;
      else if (!att->attrspecif.destination_table || (att->attrspecif.destination_table==NOT_LINKED)) /* not linked yet */
      /* is never 0, I hope */
      { if (!stricmp(names+sizeof(tobjname), "SELFPTR"))
          att->attrspecif.destination_table=tbnum;
        else if (name_find2_obj(cdp, names+sizeof(tobjname), names, CATEG_TABLE, &tabnum))
          { att->attrspecif.destination_table=NOT_LINKED;  linked=FALSE; } /* destin. table not found */
        else att->attrspecif.destination_table=tabnum;  /* store the destin. table num */
      }
      names+=2*sizeof(tobjname); /* to the next name */
    }
  }
  return linked;
}

static BOOL bi_tables(cdp_t cdp, ttablenum tb1, tattrib attr1, ttablenum * tb2, tattrib * attr2)
/* Finds dual table & attribute, warns & returns TRUE if not found.
   Returns TRUE if fails, otherwise installs tb2. Supposes table_descr of tb1 to be locked. */
{ table_descr * td1, * td2;  uns8 i, ord=0;
  td1=tables[tb1];  /* tb1 is supposed to be installed */
  if (td1->attrs[attr1].attrspecif.destination_table==NOT_LINKED)
  { link_table_in(cdp, td1, tb1); /* return value not important */
    if (td1->attrs[attr1].attrspecif.destination_table==NOT_LINKED)
      { warn(cdp, NO_BIPTR);  return TRUE; }
  }
  *tb2=td1->attrs[attr1].attrspecif.destination_table;
  td2=install_table(cdp, *tb2);
  if (!td2) return TRUE;  /* error, no warning */
 /* searching the bi-attribute */
  for (ord=0, i=1; i<attr1; i++)
    if ((td1->attrs[i].attrtype==ATT_BIPTR)&&(td1->attrs[i].attrspecif.destination_table==*tb2))
      ord++;
  i=1;
  do
  { if (i>=td2->attrcnt)
      { warn(cdp, NO_BIPTR);  unlock_tabdescr(td2);  return TRUE; }
    if (td2->attrs[i].attrtype==ATT_BIPTR)
    { if (td2->attrs[i].attrspecif.destination_table==NOT_LINKED)
        link_table_in(cdp, td2, *tb2); /* return value not important */
      if (td2->attrs[i].attrspecif.destination_table==tb1)
        if (ord) ord--;
        else { *attr2=i;  break; }
    }
    i++;
  } while (TRUE);
  return FALSE;
}

////////////////////////////// list of changes in columns ////////////////////
// Changes in temporary tables not registered
// For PARIND contains references to deleted record and to records with no deferred constrains.

void t_record_hash::reinit(void)
{ if (allocated_htable)
    { corefree(allocated_htable);  allocated_htable=NULL; }
  tablesize=MINIMAL_HTABLE_SIZE;
  limitsize=MINIMAL_HTABLE_SIZE-1;
  memset(minimal_htable, 0xff, MINIMAL_HTABLE_SIZE * sizeof(trecnum));  // sets NORECNUM
}

bool t_record_hash::grow_table(void)
{// allocate the new htable:
  int new_tablesize = 3*tablesize;
  trecnum * new_htable = (trecnum*)corealloc(new_tablesize * sizeof(trecnum), 81);
  if (!new_htable) return true; 
  memset(new_htable, 0xff, new_tablesize * sizeof(trecnum));  // sets NORECNUM
 // move the old contents to the new htable
  int new_contains=0;
  int old_tablesize=tablesize;
  tablesize=new_tablesize;  // this changes the hashf()
  trecnum * htable = allocated_htable ? allocated_htable : minimal_htable;
  for (int ind=0;  ind<old_tablesize;  ind++)
    if (htable[ind]!=NORECNUM && htable[ind]!=INVALID_HASHED_RECNUM)
    { int nind=hashf(htable[ind]);
      while (new_htable[nind]!=NORECNUM) if (++nind >= new_tablesize) nind=0;
      new_htable[nind]=htable[ind];
      new_contains++;
    }
 // replace the old table by the new one:
  if (allocated_htable) corefree(allocated_htable);
  allocated_htable=new_htable;  
  limitsize=tablesize * 4 / 5;
  contains=new_contains;
  return false;
}

bool t_record_hash::insert_record(trecnum rec)
// Does not guarantee avoiding duplicities in stored record numbers
{ do
  { trecnum * htable = allocated_htable ? allocated_htable : minimal_htable;
    int ind=hashf(rec);
    while (htable[ind]!=NORECNUM)
    { if (htable[ind]==INVALID_HASHED_RECNUM) 
        { htable[ind]=rec;  return false; }
      if (htable[ind]==rec)  return false; 
      if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
    }
   // write to an empty cell
    if (contains >= limitsize) 
    { if (grow_table()) return true;
    }
    else  // can simply add into the empty cell [ind] in the table
    { htable[ind]=rec;
      contains++;
      return false;  
    }
  } while (TRUE);  // cycles only if grow_table() called and !error
}

void t_record_hash::remove_record(trecnum rec)
{ int ind=hashf(rec);
  trecnum * htable = allocated_htable ? allocated_htable : minimal_htable;
  while (htable[ind]!=NORECNUM)
  { if (htable[ind]==rec) htable[ind]=INVALID_HASHED_RECNUM;  // must not stop, duplicities are possible
    if (++ind >= tablesize) ind=0;  // will not cycle because htable is not full
  }
}

void register_change(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib atr)
{ if (atr==NOATTRIB)
    register_change_map(cdp, tbdf, recnum, NULL);
  else
  { t_atrmap_flex atrmap(tbdf->attrcnt);
    atrmap.clear();
    atrmap.add(atr);
    register_change_map(cdp, tbdf, recnum, &atrmap);
  }
}

void register_change_map(cdp_t cdp, table_descr * tbdf, trecnum recnum, const t_atrmap_flex * column_map)
// column_map==NULL means the full map.
{ BOOL found_deferred;  int i;
  if (recnum==NORECNUM) return; // fuse
 // make the immediate checks:
  if (table_record_state(cdp, tbdf->me(), recnum) == NOT_DELETED)
    check_constrains(cdp, tbdf, recnum, column_map, TRUE, &found_deferred, NULL);  // originally 4th param was NULL instead of column_map, error?
 // register the change for later checking of deferred constrains:
  {// find the transaction log:
    t_translog * translog = table_translog(cdp, tbdf);
    t_delayed_change * ch;
    for (i=0;  i<translog->ch_s.count();  i++)
    { ch = translog->ch_s.acc0(i);
      if (ch->ch_tabnum==tbdf->tbnum)
      { if (ch->ch_recnums.insert_record(recnum)) goto memory_error;
        if (column_map==NULL) 
          ch->ch_map.set_all();
        else 
          ch->ch_map.union_(column_map);  
        return;
      }
    }
   // table not found, add it (must update pmap pointers!!!):
    ch = translog->ch_s.next();
    if (!ch) goto memory_error;
    ch->ch_tabnum=tbdf->tbnum;  
    ch->ch_map.init(tbdf->attrcnt);
    ch->ch_recnums.init();
    ch->ch_recnums.insert_record(recnum);
    if (column_map==NULL) 
      ch->ch_map.set_all();
    else 
      ch->ch_map = *column_map;  
  }
  return;
 memory_error:
  request_error(cdp, OUT_OF_KERNEL_MEMORY);
}

void t_translog::changes_mark_core(void)
{ for (int i=0;  i<ch_s.count();  i++)
  { t_delayed_change * ch = ch_s.acc0(i);
    ch->ch_recnums.mark_core();
    ch->ch_map.mark_core();
  }
  ch_s.mark_core();
}

void t_translog::destruct_changes(void)
// Clears and deallocates the list of changes
{ for (int i=0;  i<ch_s.count();  i++)
  { t_delayed_change * ch = ch_s.acc0(i);
    ch->ch_recnums.reinit();
    ch->ch_map.release();
  }
  ch_s.ch_dynar::~ch_dynar();
}

void t_translog::clear_changes(void)
// Makes the list of changes empty, but may not deallocate everything.
{ if (ch_s.count() > 2) destruct_changes();
  else
  { for (int i=0;  i<ch_s.count();  i++)
    { t_delayed_change * ch = ch_s.acc0(i);
      ch->ch_recnums.reinit();
      ch->ch_map.release();
    }
    ch_s.clear();
  }
}

#ifdef STOP
void replace_table_in_changes(cdp_t cdp, ttablenum from, ttablenum to)
{ for (int i=0;  i<cdp->ch_s.count();  i++)
    if (cdp->ch_s.acc0(i)->ch_tabnum==from)
      cdp->ch_s.acc0(i)->ch_tabnum=to;
}
#endif

void t_translog::unregister_change(ttablenum tbnum, trecnum recnum)
// Called when deleting a record. Prevents checking constrains on deleted records.
{ 
}

void t_translog::unregister_changes_on_table(ttablenum tbnum)
// Called when deleting a table. Prevents checking constrains on deleted table
{ for (int i=0;  i<ch_s.count();  i++)
  { t_delayed_change * ch = ch_s.acc0(i);
    if (ch->ch_tabnum==tbnum)
    { ch->ch_recnums.reinit();
      ch->ch_map.release();
      ch_s.delet(i);
      return;
    }
  }
}

/****************************** new & null **********************************/
void set_null(tptr adr, const attribdef * att)
{ if (IS_HEAP_TYPE(att->attrtype))
    { ((heapacc*)adr)->pc=NULLPIECE;  ((heapacc*)adr)->len=0L; }
  else switch (att->attrtype)
  { case ATT_STRING:   case ATT_CHAR:    
                      *adr=0;                                break;
    case ATT_BINARY:  memset(adr, 0, att->attrspecif.length);break;
    case ATT_FLOAT:   *(double*)adr=NULLREAL;                break;
    case ATT_BOOLEAN:  case ATT_INT8:
                      *adr=(uns8)NONEBOOLEAN;                break;
    case ATT_AUTOR:   memset(adr, 0, UUID_SIZE);             break;
    case ATT_INT16:   *(sig16*)adr=NONESHORT;                break;
    case ATT_MONEY:   *(uns16*)adr=0;
                      *(sig32*)(adr+2)=NONEINTEGER;          break;
    case ATT_INT32:   *(sig32*)adr=NONEINTEGER;              break;
    case ATT_DATE:     case ATT_DATIM:  case ATT_TIMESTAMP:
    case ATT_TIME:    *(uns32*)adr=NONEDATE;                 break;
    case ATT_INT64:   *(sig64*)adr=NONEBIGINT;               break;
    case ATT_PTR:
    case ATT_BIPTR:   *(trecnum*)adr=NORECNUM;               break;
    case ATT_EXTCLOB:  case ATT_EXTBLOB:
                      *(uns64*)adr=0;                        break;
  }
}

const heapacc tNONEHEAPACC = { { 0, 0, 0 }, 0L };
const uns64   tNONEEXTLOB  = 0;
const wbbool  tNONEBOOLEAN = NONEBOOLEAN;
const uns32   tNONEPTR     = NONEPTR;
const sig32   tNONE4       = NONEINTEGER;
const sig64   tNONE8       = NONEBIGINT;
const sig16   tNONE2       = NONESHORT;
const uns8    tNONEMONEY[6]= { 0, 0, 0, 0, 0, 0x80 };
const WBUUID  tNONEAUTOR   = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const uns8    tNONEBINARY[MAX_FIXED_STRING_LENGTH]  = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
};

const void * _null_value(int tp)  // for fast access when inserting a record
{ 
  if (IS_HEAP_TYPE(tp)) 
    return &tNONEHEAPACC;
  else switch (tp)
  { case ATT_CHAR:      case ATT_STRING:    default:
                                             return NULLSTRING;
    case ATT_FLOAT:                          return &NULLREAL;
    case ATT_BOOLEAN:   case ATT_INT8:       return &tNONEBOOLEAN;
    case ATT_AUTOR:                          return tNONEAUTOR;
    case ATT_INT16:                          return &tNONE2;
    case ATT_MONEY:                          return tNONEMONEY;
    case ATT_INT32:     case ATT_DATE:      case ATT_TIMESTAMP:
    case ATT_DATIM:     case ATT_TIME:       return &tNONE4;
    case ATT_INT64:                          return &tNONE8;
    case ATT_PTR:       case ATT_BIPTR:      return &tNONEPTR;
    case ATT_BINARY:                         return tNONEBINARY;
    case ATT_EXTCLOB:   case ATT_EXTBLOB:    return &tNONEEXTLOB;
  }
}

const void * null_value(int tp)  
{ return _null_value(tp); }

void invalidate_signature(cdp_t cdp, const table_descr * tbdf, trecnum recnum, tattrib atr)
{ uns8 state = sigstate_changed;
  uns32 offset = offsetof(t_signature, state);
  do
  { tb_write_var(cdp, (table_descr *)tbdf, recnum, atr, offset, sizeof(uns8), &state);
    atr=tbdf->attrs[atr].signat;
  } while (atr!=NOATTRIB);
}

static BOOL write_trace(cdp_t cdp, table_descr * tbdf, trecnum recnum,
                               tattrib attrib, tptr info, unsigned attsz, BOOL new_rec = FALSE);

const t_colval default_colval = { NOATTRIB, NULL, DATAPTR_DEFAULT, NULL, NULL, 0, NULL };
static const t_vector_descr default_vector(&default_colval);

void t_vector_descr::release_colval_exprs(void)
{ t_colval * colval = (t_colval *)colvals;
  if (colval)
    while (colval->attr!=NOATTRIB)
    { if (colval->expr)  { delete colval->expr;   colval->expr =NULL; }
      if (colval->index) { delete colval->index;  colval->index=NULL; }
      colval++;
    }
}

void t_vector_descr::mark_core(void)  // does not mark itself, nor vals
{ t_colval * colval = (t_colval *)colvals;
  if (colval)
  { mark(colval);
    while (colval->attr!=NOATTRIB)
    { if (colval->expr)  colval->expr->mark_core();
      if (colval->index) colval->index->mark_core();  
      colval++;
    }
  }
}

/* INSERT/UPDATE zaznamu nebo skupiny sloupcu:
- pokud se vklada zaznam, nutno mit write lock na basblocku
- prepisy hodnot se musi delat na soukromych strankach
- pri vlozeni zaznamu nutno inicializovat vsechny sloupce, vcetne vsech predalokovanych hodnot multiatributu
- musi branit prepisu sledovacich atributu
- musi zneplatnit podpisy nasledujici za prepsanym nesystemovym sloupcem (hromadne, staci provest pro prvni prepsany sloupec, ktery neni systemovy)

Systemove atributy:
- pri vlozeni zaznamu nutno nastavit sloupec DELETED, pri prepisu nutno kontrolovat, zda zaznam neni uvolneny
- je-li tabulka replikovatelna (sloupec ZCR), nutno zapsat GCR do ZCR a inkrementovat GCR
-- pri vkladani nutno pridelit a zapsat UNIKEY
-- je-li token, musi se testovat jeho pritomnost
-- je/li LUO, musi se do nej zapsat aktualni origin
*/

inline bool writing_to_replicable_table(cdp_t cdp, table_descr * tbdf, char * dtz)
// Called only when tbdf->zcr_attr!=NOATTRIB. dtz is the pointer to the start of the record.
{ if (tbdf->token_attr!=NOATTRIB)
    if (!(*(uns16*)(dtz+tbdf->attrs[tbdf->token_attr].attroffset) & 0x8000))
      if (!cdp->mask_token)
      { request_error(cdp, NO_WRITE_TOKEN);
        return true;
      }
  write_gcr(cdp, dtz+tbdf->attrs[tbdf->zcr_attr].attroffset);
  if (tbdf->luo_attr!=NOATTRIB)
    memcpy(dtz+tbdf->attrs[tbdf->luo_attr].attroffset, cdp->luo, UUID_SIZE);
  return false;
}

/*inline */bool writing_to_replicable_table2(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, char * dta, bool update_indicies)
// Checks token, writes ZCR, LUO. ZCR update must be before inditem_add!
// With [update_indicies] must not be used for attributes which may be in index (because NULL specified in inditem_remove).
// Called only when tbdf->zcr_attr!=NOATTRIB. dta is the pointer to the value of attrib (if dta!=NULL).
// When dta==NULL then attrib must not be tbdf->token_attr.
{ tfcbnum fcbnz;  tptr dtz;  bool new_page_fixed;
  if (dta!=NULL && !tbdf->attrs[attrib].attrpage) 
  { dtz = dta - tbdf->attrs[attrib].attroffset;
    new_page_fixed=false;
  }
  else
  { dtz=fix_attr_write(cdp, tbdf, recnum, DEL_ATTR_NUM, &fcbnz);
    if (dtz==NULL) return true;
    new_page_fixed=true;
  }
  t_simple_update_context sic(cdp, tbdf, attrib);
  if (update_indicies && *dtz==NOT_DELETED)
  { if (!sic.lock_roots()) goto err;
    if (!sic.inditem_remove(recnum, NULL)) /* remove record from system indexes (ZCR, UNIKEY, TOKEN) */
      goto err;
  }
  if (tbdf->token_attr!=attrib || dta==NULL || (*(uns16*)dta & 0xc000) == 0x8000) // unless token released (must replicate released if not DP)
    if (writing_to_replicable_table(cdp, tbdf, dtz))
      goto err;
  if (update_indicies && *dtz==NOT_DELETED)
    if (!sic.inditem_add(recnum))  /* add record into indexes */
      goto err;
  if (new_page_fixed) unfix_page(cdp, fcbnz);
  return false;
 err:
  if (new_page_fixed) unfix_page(cdp, fcbnz);
  return true;
}

static BOOL changing_master_key(cdp_t cdp, dd_index * indx, tptr oldval, const char * newval, ttablenum par_tbnum, int par_indnum);

BOOL tb_write_vector(cdp_t cdp, table_descr * tbdf, trecnum recnum, const t_vector_descr * vector, BOOL newrec, t_tab_trigger * ttr)
// Writes all the vector values to the record of the table. 
// Does not necessarily load all pages unless newrec.
// Does not implement special actions when writing to the token-attribute
// Bipointers not supported.
// basbl: sdilena kopie, zmena counteru se hned promitne na disk
// supports max 255 items in the vector
{ BOOL res=TRUE;  tattrib first_overwritten_column = 0;  bool record_is_deleted;  
  tptr old_keyval[INDX_MAX];    // must be init before any goto ex;
  uns16 * attr_refs = NULL;     // must be init before any goto ex;
  t_btree_read_lock btrl[INDX_MAX];
  int expr_array_size = 0;
  t_value * precomp_vals=NULL;  // must be init before any goto ex;
                
  t_translog * translog = table_translog(cdp, tbdf);
  if (!newrec) 
  { memset(old_keyval, 0, sizeof(tptr)*tbdf->indx_cnt);  // must be init before any goto ex;
    if (recnum >= tbdf->Recnum())
      { request_error(cdp, RECORD_DOES_NOT_EXIST);  return TRUE; }
  }
 // locking the record:
  if (wait_record_lock_error(cdp, tbdf, recnum, TMPWLOCK)) return TRUE;
  bool saved_in_expr_eval;
  saved_in_expr_eval=cdp->in_expr_eval;  

 // fix the 1st target page:
  tfcbnum fcbn;  tblocknum datadadr;  tptr dt;  unsigned offset_in_the_page, pgrec;  unsigned page_of_the_record = 0;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,recnum,0,offset_in_the_page,pgrec);
  if (!datadadr) return TRUE;  // error detected in ACCESS_COL_VAL
  dt=fix_priv_page(cdp, datadadr, &fcbn, translog);
  if (!dt) return TRUE;
  translog->log.rec_changed(fcbn, datadadr, tbdf->recordsize, pgrec);
  dt += offset_in_the_page;

 // update/check the system columns independent of indices (incl. record tracing attributes):
  if (newrec) 
  { *dt=vector->delete_record ? DELETED : NOT_DELETED;
    if (tbdf->rec_privils_atr!=NOATTRIB)
    { heapacc * tmphp = (heapacc*)(dt+tbdf->attrs[tbdf->rec_privils_atr].attroffset);
      tmphp->len=0;  tmphp->pc=NULLPIECE;
    }
    record_is_deleted = false;
  }
  else  // updating an existing record
  { if (*dt==RECORD_EMPTY)
    // this situation if the result of not locking records in a cursor - the record has been deleted by somebody
    // EMPTY replaced by CONDITION_COMPLETION now, but request_error would prevent continuing the SQL procedure
#ifdef STOP  
      { request_error(cdp, EMPTY);  goto ex; }  
#else  // this version (silently) skips such record
      goto ex;
#endif      
    record_is_deleted = *dt==DELETED;
#if WBVERS>=810
    if (record_is_deleted) 
      { warn(cdp, WORKING_WITH_DELETED_RECORD);  /*goto ex;*/ }  // both is possible, allowing the write was normal
#endif
  }
  cdp->in_expr_eval=true;  // prevents rollback or commit in the statement (in unindexing before indexing the FT document)
 // pre-evaluate expressions, before locking indexes:
  int i;
  for (i=0;  vector->colvals[i].attr!=NOATTRIB;  i++)
    if (vector->colvals[i].table_number==vector->sel_table_number)
    { const t_colval * colval = &vector->colvals[i];
      if (!(ttr && ttr->has_new_values() && ttr->active_rscope_usage_map()->has(colval->attr)))  // new value is in the bef_rscope, ignoring expression
        if (colval->expr!=NULL && !vector->hide_exprs ||
            colval->dataptr==DATAPTR_DEFAULT && tbdf->attrs[colval->attr].defvalexpr!=NULL && !(cdp->ker_flags & KFL_DISABLE_DEFAULT_VALUES))
          expr_array_size=i+1;  
    }
  if (expr_array_size)
  { precomp_vals = new t_value[expr_array_size];
    for (i=0;  vector->colvals[i].attr!=NOATTRIB;  i++)
      if (vector->colvals[i].table_number==vector->sel_table_number)
      { const t_colval * colval = &vector->colvals[i];
        if (!(ttr && ttr->has_new_values() && ttr->active_rscope_usage_map()->has(colval->attr)))  // new value is in the bef_rscope, ignoring expression
        { const attribdef * att = tbdf->attrs+colval->attr;
          if (colval->expr!=NULL && !vector->hide_exprs)
          { expr_evaluate(cdp, colval->expr, precomp_vals+i);
            precomp_vals[i].load(cdp);
          }  
          else if (colval->dataptr==DATAPTR_DEFAULT && att->defvalexpr!=NULL && !(cdp->ker_flags & KFL_DISABLE_DEFAULT_VALUES))
            set_default_value(cdp, precomp_vals+i, att, tbdf);
        }
      }      
  }
  
  {
   // BEFORE triggers:
    if (ttr && !vector->delete_record && !record_is_deleted) if (ttr->pre_row(cdp, tbdf, recnum, vector)) goto ex;

   // prepare a map of directly updated columns:
    t_atrmap_flex atrmap(tbdf->attrcnt);  
    atrmap.clear();
    for (i=0;  vector->colvals[i].attr!=NOATTRIB;  i++)
      if (vector->colvals[i].table_number==vector->sel_table_number)
        atrmap.add(vector->colvals[i].attr);
   // add columns updated in triggers to the map:
    if (ttr && ttr->has_new_values())
      atrmap.union_(ttr->active_rscope_usage_map());
   // add system columns which will be overwritten (sp. importance when no columns are explicitly updated!)
    if (tbdf->zcr_attr!=NOATTRIB)
      atrmap.add(tbdf->zcr_attr);
   // calculate the keys of indicies for the old values (must be after executing the BEFORE triggers, they can change the key values indirectly):
   // lock the index roots (must be done before changing of the record starts):
    if (!(cdp->ker_flags & KFL_STOP_INDEX_UPDATE) && !record_is_deleted)
    { dd_index * cc;   int ind;
      cdp->index_owner = tbdf->tbnum;
      for (ind=0, cc=tbdf->indxs;  ind<tbdf->indx_cnt;  ind++, cc++)
      { if (!cc->disabled)
        { bool index_depends_on_changed_column = atrmap.intersects(&cc->atrmap);
          if (newrec || index_depends_on_changed_column)
            if (!btrl[ind].lock_and_update(cdp, cc->root))  
              goto ex;
          if (index_depends_on_changed_column && !newrec)
          { old_keyval[ind]=(tptr)corealloc(cc->keysize, 81);
            if (old_keyval[ind]==NULL) goto ex;
            /* prepare the key value */
            compute_key(cdp, tbdf, recnum, cc, old_keyval[ind], false);
          }
        }
      }
    }
   // remove the old contents from fulltext indexes:
    if (tbdf->fulltext_info_list!=NULL)
      if (!newrec && !(cdp->ker_flags & KFL_STOP_INDEX_UPDATE) && !record_is_deleted)  // KFL_STOP_INDEX_UPDATE ???
      { int pos=-1;  t_ftx_info info;
        while (tbdf->get_next_table_ft(pos, atrmap, info))
        { t_fulltext2 * ftx = get_fulltext_system(cdp, info.ftx_objnum, NULL);
          if (ftx)
          {// read id:
            sig64 idval=0;  // value may be sig32 or sig64
            tb_read_atr(cdp, tbdf, recnum, info.id_col, (char*)&idval);
            ftx->remove_doc(cdp, (t_docid)idval);
            ftx->Release();
          }
        }
      }

   // update system columns used in indicies:
    if (tbdf->zcr_attr!=NOATTRIB && !record_is_deleted)
    { if (newrec)
      { if (tbdf->unikey_attr!=NOATTRIB)
        { const attribdef * att = tbdf->attrs+tbdf->unikey_attr;
          memcpy(dt+att->attroffset, header.server_uuid, UUID_SIZE);
          *(uns32*)(dt+att->attroffset+UUID_SIZE)=((table_descr*)tbdf)->unique_value_cache.get_next_available_value(cdp);
        }
        if (tbdf->detect_attr!=NOATTRIB)
        { const attribdef * att = tbdf->attrs+tbdf->detect_attr;
          memset(dt+att->attroffset, 0, att->attrspecif.length);
        }
        if (tbdf->token_attr !=NOATTRIB)
        { const attribdef * att = tbdf->attrs+tbdf->token_attr;
          *(uns16*)(dt+att->attroffset) = 0x8000; // owning the token
        }
      }
      if (cdp->replication_change_detector==NULL)  // otherwise processing a replication packet, different strategy
        if (writing_to_replicable_table(cdp, tbdf, dt))  // must be called AFTER copying the old index keys
          goto ex;
    }

   // prepare the array of column references indexed by column number, has 0xff value for columns not present in the vector:
    attr_refs = (uns16*)corealloc(sizeof(uns16)*tbdf->attrcnt, 46);
    if (!attr_refs) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto ex; }
    memset(attr_refs, 0xff, sizeof(uns16)*tbdf->attrcnt);  // sets all to (uns16)-1
    for (unsigned pos=0;  vector->colvals[pos].attr!=NOATTRIB;  pos++) 
    { const t_colval * colval = &vector->colvals[pos];
      if (colval->table_number==vector->sel_table_number)
        if (colval->attr<tbdf->attrcnt)  // protecting the server againts client errors
          attr_refs[colval->attr]=pos;
    }
   // write the new values per page:

    //if (newrec) // skip system columns, have already been initialized
    //  while (!memcmp(tbdf->attrs[page_stopatr].name, "_W5_", 4)) page_stopatr++;
    // must not skip, when defined in the vector, but should not write the dafault value for newrec!
   //#INV if (fcbn!=NOFCBNUM) then it is the fcbn of the page containing [page_of_the_record] of the current record.
   //#INV if (continuation_values) then must initialize continuation_values more multiattribute values
   //#INV page_stopatr is the 1st column of the page or one more iff continuation_values
    for (unsigned atr=1;  atr<tbdf->attrcnt;  atr++)
    { const t_colval * colval;  t_value val; 
     // find the source of the value:
      uns16 vector_index = attr_refs[atr];
      if (vector_index!=(uns16)-1)
      { colval=&vector->colvals[vector_index];  
        if (colval->dataptr!=DATAPTR_DEFAULT || atr!=tbdf->zcr_attr && atr!=tbdf->unikey_attr && atr!=tbdf->detect_attr)
          goto will_write;
      }
      if (newrec)  // column not specified, write default
        if (memcmp(tbdf->attrs[atr].name, "_W5_", 4)) // unless system - default defined above
          { colval=&default_colval;  goto will_write; } 
      if (ttr && ttr->has_new_values() && atrmap.has(atr) && atr!=tbdf->zcr_attr) // is in the map but is not in the vector - must be in the rscope!
        { colval=&default_colval;  goto will_write; }  // colval=&default_colval is not necessary here
      continue;

     will_write:  // source found write colval or ttr value

     // fix the current page, if not fixed before:
      if (tbdf->attrs[atr].attrpage!=page_of_the_record)
      { UNFIX_PAGE(cdp, fcbn);
        page_of_the_record=tbdf->attrs[atr].attrpage;
        datadadr=ACCESS_COL_VAL(cdp,tbdf,recnum,page_of_the_record,offset_in_the_page,pgrec);
        if (!datadadr) goto ex;  // error detected in ACCESS_COL_VAL
        dt=fix_priv_page(cdp, datadadr, &fcbn, translog);
        if (!dt) goto ex;
        translog->log.rec_changed(fcbn, datadadr, tbdf->recordsize, pgrec);
        dt += offset_in_the_page;
      }

     // get the value to be written:
      const attribdef * att = tbdf->attrs+atr;
      uns8 tp=att->attrtype;
      if (tp>=ATT_FIRSTSPEC && tp<=ATT_LASTSPEC && !newrec && !(cdp->ker_flags & KFL_ALLOW_TRACK_WRITE))
      { t_exkont_table ekt(cdp, tbdf->tbnum, recnum, atr, NOINDEX, OP_WRITE, NULL, 0);
        request_error(cdp, NO_RIGHT);  
        goto ex; 
      }
      if (att->signat!=NOATTRIB && !newrec && !first_overwritten_column)
        first_overwritten_column=atr;
      unsigned sz=TYPESIZE(att), size=sz, offset; // offset used by var-len values only, sz used by exlobs when importing refids
      // size: will be set for var-len values and t_express, size=sz used by ATT_BINARY type from vector
      const char * src;  
      offset=0;
      if (ttr && ttr->has_new_values() && ttr->active_rscope_usage_map()->has(atr))  // new values are in the bef_rscope
      { src=ttr->new_value(atr);
        if (IS_HEAP_TYPE(tp) || IS_EXTLOB_TYPE(tp))
        { t_value * val = (t_value*)src;
          if (val->is_null())  // other members may be undefined 
            { size=0;  src=NULL; }
          else
            { size = val->length;  src = val->valptr(); }
        }
      }
      else if (vector_index!=(uns16)-1 &&
               (colval->expr!=NULL && !vector->hide_exprs ||
                colval->dataptr==DATAPTR_DEFAULT && att->defvalexpr!=NULL && !(cdp->ker_flags & KFL_DISABLE_DEFAULT_VALUES)))
           // new values are pre-evaluated         
      { if (precomp_vals[vector_index].is_null()) qlset_null(&precomp_vals[vector_index], att->attrtype, TYPESIZE(att));  
        src =precomp_vals[vector_index].valptr(); // must load indir first!!
        size=precomp_vals[vector_index].length;
      }     
      else // new values are in the vector
        vector->get_column_value(cdp, colval, (table_descr*)tbdf, att, src, size, offset, val);

      tptr dest = dt+att->attroffset;
     // tracing 
      if (att->attrspec2 & IS_TRACED) /* must be called before overwriting the old value */
        write_trace(cdp, tbdf, recnum, atr, dest, sz, newrec);

     // write the value:
      if (att->attrmult > 1)  // cannot have non-null default value, not affected by triggers
      { if (newrec)
        { if (att->attrmult & 0x80) *(tpiece*)(dest-sizeof(tpiece))=NULLPIECE;
          *(t_mult_size*)dest=0;     /* count of values */
         // init all preallocated values for the new record:
          unsigned locvals, ind, prevals;
          prevals = att->attrmult & 0x7f;
          locvals = (BLOCKSIZE-att->attroffset-CNTRSZ) / sz;  /* max. vals in 1st page */
          if (locvals > prevals) locvals=prevals;
          for (ind=0;        ind < locvals;  ind++) 
            set_null(dest+sizeof(t_mult_size)+ind*sz, att);  // init local values
          for (ind=locvals;  ind < prevals;  ind++) 
            tb_write_ind_pure(cdp, tbdf, recnum, atr, ind, _null_value(tp));
        }
        if (attr_refs[atr] != (uns16)-1)  // there is at least one value
          if (!colval->index)  // used by t_packet_writer (TDT import, replication)
          { if (colval->lengthptr!=NULL)  // all values specified here, used by TDT import
            { t_mult_size cntval = *(t_mult_size*)colval->lengthptr; // count stored there
              t_varval_list * list = (t_varval_list*)src;  // used for heap types only
              for (t_mult_size ind=0;  ind<cntval;  ind++)
                if (!IS_HEAP_TYPE(tp))
                { tb_write_ind_pure    (cdp, tbdf, recnum, atr, ind, src);
                  src+=sz;
                }
                else
                { tb_write_ind_var_pure(cdp, tbdf, recnum, atr, ind, 0, list->length, list->val);
                  list++;
                }
            }
            else ;  // vales not specified, used by ALTER TABLE with a multiattribute
          }
          else // the same column may be specified multiple times
            for (unsigned pos=0;  vector->colvals[pos].attr!=NOATTRIB;  pos++) 
              if (vector->colvals[pos].attr==atr &&
                  vector->colvals[pos].table_number==vector->sel_table_number)
              { vector->get_column_value(cdp, &vector->colvals[pos], (table_descr*)tbdf, att, src, size, offset, val);
                t_mult_size ind;  t_value indval;
                expr_evaluate(cdp, colval->index, &indval);  ind=(t_mult_size)indval.intval;
                if (!IS_HEAP_TYPE(tp))
                  tb_write_ind_pure    (cdp, tbdf, recnum, atr, ind, src);
                else
                  tb_write_ind_var_pure(cdp, tbdf, recnum, atr, ind, offset, size, src);
              }
      }
      else if (IS_HEAP_TYPE(tp))  // write "size" bytes from offset
      { heapacc * tmphp = (heapacc*)dest;
       // init piece for the new record, define the new size:
        if (newrec) tmphp->pc=NULLPIECE;
        else if (vector->setsize && offset+size < tmphp->len)  // cut the end if the original size was bigger
        { if (cdp->replication_change_detector) *cdp->replication_change_detector=true;
          hp_shrink_len(cdp, &tmphp->pc, 1, offset+size, translog);
        }
          // must not do this when writing compiled code: is not sequential!!!
        if (vector->setsize || offset+size > tmphp->len)
        { if (cdp->replication_change_detector) *cdp->replication_change_detector=true;
          tmphp->len=offset+size;  /* new length */
        }
       // write the value:
        if (the_sp.WriteJournal.val() && !newrec && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
          var_jour(cdp, tbdf->tbnum, recnum, atr, NOINDEX, offset, size, (void*)src);
        if (hp_write(cdp, &tmphp->pc, 1, offset, size, src, translog)) goto ex;
      }
      else if (IS_EXTLOB_TYPE(tp) && !(cdp->ker_flags & KFL_EXPORT_LOBREFS))  // write "size" bytes from offset
      { if (newrec) *(uns64*)dest=0;  // init
        if (write_ext_lob(cdp, (uns64*)dest, offset, size, src, vector->setsize, NULL))
          goto ex;
      }
      else // copy the simple value
      { if (cdp->replication_change_detector && !*cdp->replication_change_detector)  // no change so far, must check for a change
        { if (tp==ATT_BINARY && size<sz)
          { if (memcmp(dest, src, size)) *cdp->replication_change_detector=true;
            for (int i = size;  i<sz;  i++) if (dest[i]) *cdp->replication_change_detector=true;
          }
          else 
            if (memcmp(dest, src, sz)) *cdp->replication_change_detector=true;
        }
        if (tp==ATT_BINARY && size<sz)
        { memcpy(dest, src, size);
          memset(dest+size, 0, sz-size);
        }
        else if (tp==ATT_STRING)  // the source string may not have the full length
          if (att->attrspecif.wide_char)
          { size = sizeof(wuchar) * (int)wuclen((const wuchar*)src);
            memcpy((wuchar*)dest, (const wuchar*)src, size < sz ? size : sz);
            if (size+1 < sz)  // both values are even, space is for wide zero or nothing
              dest[size]=dest[size+1]=0;
          }
          else
          { size = (int)strlen(src)+1;
            memcpy(dest, src, size < sz ? size : sz);
            if (size < sz) dest[size]=0;
          }
        else memcpy(dest, src, sz);
        if (att->defvalexpr==COUNTER_DEFVAL)   /* UNIQUE value */
          if (colval->dataptr!=DATAPTR_DEFAULT || !(cdp->ker_flags & KFL_DISABLE_DEFAULT_VALUES)) // DEFAULT is NULL
            tbdf->unique_value_cache.register_used_value(cdp, get_typed_unique_value(dest, att->attrtype));
            // must not register NONEINTEGER value used when inserting empty record during TDT import
       // journaling
        if (the_sp.WriteJournal.val() && !newrec && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
        { worm_add(cdp, &cdp->jourworm, &opcode_write     ,1                );
          worm_add(cdp, &cdp->jourworm, &tbdf->tbnum      ,sizeof(ttablenum));
          worm_add(cdp, &cdp->jourworm, &recnum           ,sizeof(trecnum)  );
          worm_add(cdp, &cdp->jourworm, &atr              ,sizeof(tattrib)  );
          worm_add(cdp, &cdp->jourworm, &mod_stop         ,1                );
          worm_add(cdp, &cdp->jourworm, src               ,sz               );
        }

      }
      val.set_null();  // must release the possible indirect value
    } // cycle on columns
   // unfix the current page, if fixed:
    UNFIX_PAGE(cdp, fcbn);  fcbn=NOFCBNUM; 

    if (!vector->delete_record)
    { 
     // update indicies and perform active referential integrity:
      if (!tbdf->referencing_tables_listed) 
        if (!prepare_list_of_referencing_tables(cdp, (table_descr*)tbdf))
          goto ex;
      if (!(cdp->ker_flags & KFL_STOP_INDEX_UPDATE) && !record_is_deleted)
      { dd_index * cc;   char keyval[MAX_KEY_LEN];  int ind;
        for (ind=0, cc=tbdf->indxs;  ind<tbdf->indx_cnt;  ind++, cc++)
        { if (!cc->disabled)
            if (newrec || atrmap.intersects(&cc->atrmap))
            { t_exkont_index ekt(cdp, tbdf->tbnum, ind);
              cdp->index_owner = tbdf->tbnum;
              /* prepare the key value */
              if (compute_key(cdp, tbdf, recnum, cc, keyval, false))
              { if (newrec)
                {  if (bt_insert(cdp, keyval, cc, cc->root, tbdf->selfname, translog))
                     goto ex;
                }
                else if (!HIWORD(cmp_keys(old_keyval[ind], keyval, cc))) // key values different!
                { bt_remove(cdp, old_keyval[ind], cc, cc->root, translog);
                  if (cc->reftabnums && !(cdp->ker_flags & KFL_DISABLE_REFINT))
                    if (changing_master_key(cdp, cc, old_keyval[ind], keyval, tbdf->tbnum, ind))
                      goto ex;
                  cdp->index_owner = tbdf->tbnum;
                  if (bt_insert(cdp, keyval, cc, cc->root, tbdf->selfname, translog))
                    goto ex;
                }
              }
            }
        }
     // add the new contents to fulltext indexes:
        if (tbdf->fulltext_info_list!=NULL)
        { int pos=-1;  t_ftx_info info;
          while (tbdf->get_next_table_ft2(pos, atrmap, info))
          { t_fulltext2 * ftx = get_fulltext_system(cdp, info.ftx_objnum, NULL);
            if (ftx)
            { fulltext_index_column(cdp, ftx->ftk, tbdf, recnum, info.text_col, info.id_col,
                                    ftx->items.acc(info.item_index)->format, ftx->items.acc(info.item_index)->mode);
              ftx->Release();
            }
          }
        }
      }
     // invalidate signatures (must be AFTER inserting the update record into indexes, because it updates REPL indexes):
      if (!newrec && first_overwritten_column)
        invalidate_signature(cdp, tbdf, recnum, tbdf->attrs[first_overwritten_column].signat);
     // unlock the indexes (must be done BEFORE post triggers, otherwise complex deasdlock is possible:
     //  client 1 owns TMPWLOCK on T1 and waits for index WLOCK on T2
     //  client 2 is in tb_write_vector, has RLOCK on T2 and starts the trigger updating T1, which needs the lock owned by clinet 1)
      for (int ind=0;  ind<tbdf->indx_cnt;  ind++)
        btrl[ind].unlock();
     // call post-triggers:
      if (ttr && !record_is_deleted) if (ttr->post_row(cdp, tbdf, recnum)) goto ex;
     // perform and register integrity constrains (should be done on temp. tables too, used in ALTER TABLE):
        if (!record_is_deleted)
          register_change_map(cdp, tbdf, recnum, newrec ? NULL : &atrmap); // for delayed checking constrains
    }

    res=FALSE;
  } // constructed objects
 ex:
  cdp->in_expr_eval=saved_in_expr_eval;
  if (precomp_vals) delete [] precomp_vals;
  unfix_page(cdp, fcbn);  // may but may not be fixed
  if (!newrec) 
    for (int i=0;  i<tbdf->indx_cnt;  i++) corefree(old_keyval[i]);
  corefree(attr_refs);
  return res;
}

/******* variables protected by the CRITICAL_SECTION cs_tables: *************/
table_descr **tables;  /* ptr to array of ptrs to table descriptors */
table_descr * tabtab_descr=NULL, * objtab_descr=NULL, * usertab_descr=NULL; // descriptors of permanently installed tables
static table_descr **prev_tables = NULL;  /* previous copy of tables */
static unsigned installed_tables = 0;  /* number of installed tables (incl. temp.) */

int last_temp_tabnum = 0;  /* is always <= 0 */
static int tables_last = 0;       /* is always >  0 */
#define TABLES_ALLOC_STEP 80
// tables[] array is <last_temp_tabnum..tables_last>
/* All allocated items in "tables" are valid or NULL. */
/* Must never shrink tables[]. tltab1 and tltab2 contain numbers that must remain valid! */

static bool realloc_tables(cdp_t cdp, int trec)
// Reallocates "tables" so that it contains trec. Must be called inside cs_tables.
// Called should call request_error if true returned.
{// find the new boudaries:
  int new_last_temp=last_temp_tabnum;
  int new_last     =tables_last;
  if (trec < 0)
  { if (trec < last_temp_tabnum) 
      new_last_temp = last_temp_tabnum-TABLES_ALLOC_STEP < trec ? last_temp_tabnum-TABLES_ALLOC_STEP : trec;
    else return false;
  }
  else
  { if (trec > tables_last)      
      new_last = trec > tables_last+TABLES_ALLOC_STEP ? trec : tables_last+TABLES_ALLOC_STEP;
    else return false;
  }
 // alloc the new array and copy the contents from the old one:
  unsigned toalloc = (new_last-new_last_temp+1) * sizeof(table_descr*);
  table_descr ** tabs=(table_descr**)corealloc(toalloc, 91);
  if (!tabs) return true;
  memset(tabs, 0, toalloc);
  memcpy(tabs+(last_temp_tabnum-new_last_temp), tables+last_temp_tabnum,
    (tables_last-last_temp_tabnum+1) * sizeof(table_descr*));
 // replace (this form is preemptive):
  if (prev_tables!=NULL) corefree(prev_tables);
  prev_tables = tables+last_temp_tabnum;
  tables=tabs-new_last_temp;
  last_temp_tabnum=new_last_temp;  tables_last=new_last;
  return false;
}

static BOOL initial_rights(cdp_t cdp, table_descr * tbdf, trecnum rec)
/* tbdf is supposed to be locked. Writen to the journal depending
   on the ker_flags (not for tables, yes for other objects). */
// Gives the all-access to the author and specified access to EVERYBODY.
{ 
  if (tbdf->tbnum<0) return FALSE;
  t_privils_flex priv_val(tbdf->attrcnt);
 // privileges to the author:
  int bytecnt=(tbdf->attrcnt-2) / 4 + 1;
  priv_val.set_all_rw();
  if (tbdf->tbnum==OBJ_TABLENUM)  // disable changing the category
    priv_val.the_map()[1+(OBJ_CATEG_ATR-1) / 4] &= ~(1 << ((2*((OBJ_CATEG_ATR-1)%4))+1));
  *priv_val.the_map() = RIGHT_DEL|RIGHT_GRANT; // global read & write will be added
  cdp->prvs.set_own_privils(tbdf, rec, priv_val);
 // privileges to the EVERYBODY group:
  if (tbdf->tbnum!=OBJ_TABLENUM)
  { cdp->prvs.get_effective_privils(tbdf, NORECNUM, priv_val); // derived from EVERYBODY's privileges
    uns8 bas_priv = *priv_val.the_map();
    if (bas_priv & (RIGHT_NEW_READ | RIGHT_NEW_WRITE | RIGHT_NEW_DEL))
    { privils prvs(cdp);
      prvs.set_user(EVERYBODY_GROUP, CATEG_GROUP, FALSE);
      if (bas_priv & RIGHT_NEW_READ)
        if (bas_priv & RIGHT_NEW_WRITE)
        { *priv_val.the_map()=RIGHT_READ | RIGHT_WRITE;
          priv_val.set_all_rw();
        }
        else
        { *priv_val.the_map()=RIGHT_READ;
          priv_val.set_all_r();
        }
      else
        if (bas_priv & RIGHT_NEW_WRITE)
        { *priv_val.the_map()=RIGHT_WRITE;
          priv_val.set_all_w();
        }
        else
          priv_val.clear();
      if (bas_priv & RIGHT_NEW_DEL) *priv_val.the_map() |= RIGHT_DEL;
      prvs.set_own_privils(tbdf, rec, priv_val);
    }
  }
  return FALSE;
}

////////////////////////////////////// t_unique_value_cache ////////////////////////////////////////
void table_descr::t_unique_value_cache::load_values_to_cache(cdp_t cdp)
// Allocates a chunk of consecutive values from the unique counter associated with the table
{ ProtectedByCriticalSection cs(&containing_tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
  t_versatile_block bas_acc(cdp, containing_tbdf->independent_alpha());
 // t_translog * translog = table_translog(cdp, containing_tbdf); -- no, must work independent on ordinaty tables!
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(containing_tbdf->basblockn, containing_tbdf->translog);
  if (basbl)
  { next_available_value=basbl->unique_counter;
    cached_values=NUMBER_OF_ALLOCATED_VALUES_PER_CHUNK;
    basbl->unique_counter+=NUMBER_OF_ALLOCATED_VALUES_PER_CHUNK;
    bas_acc.data_changed(containing_tbdf->translog);  // destructor will write the changes
  }
}

uns32 get_typed_unique_value(void * valptr, int type)
{ switch (type)
  { case ATT_INT32:  case ATT_INT64:  case ATT_MONEY:
      return *(uns32*)valptr;
    case ATT_INT16:
      return *(sig16*)valptr!=NONESHORT ? (uns32)*(uns16*)valptr : NONEINTEGER;
    case ATT_INT8:
      return *(sig8*)valptr!=NONETINY ? (uns32)*(uns8*)valptr : NONEINTEGER;
  }
  return NONEINTEGER;
}

void table_descr::t_unique_value_cache::register_used_value(cdp_t cdp, uns32 value)
// Called when a value is written or imported to a column with default value from a UNIQUE generator.
// Updates the generator so that it cannot return the same value.
{ if (value==NONEINTEGER) return;  // ignore NULL values
  ProtectedByCriticalSection cs(&containing_tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
  if (cached_values)
  { if (value < next_available_value) return;  // does not affect unique counter
    if (value < next_available_value+cached_values) // colliding cache values removed
    { cached_values-=(value-next_available_value)+1;
      next_available_value=value+1;
      return;
    }
    cached_values=0;
  }
 // must check and possibly update the basic block:
  t_versatile_block bas_acc(cdp, containing_tbdf->independent_alpha());
 // t_translog * translog = table_translog(cdp, containing_tbdf);
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(containing_tbdf->basblockn, containing_tbdf->translog);
  if (basbl)
  { if (value >= basbl->unique_counter)
    { basbl->unique_counter = value+1;
      bas_acc.data_changed(containing_tbdf->translog);  // destructor will write the changes
    }
  }
}

void table_descr::t_unique_value_cache::close_cache(cdp_t cdp)
{ if (cached_values)
  { ProtectedByCriticalSection cs(&containing_tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    t_versatile_block bas_acc(cdp, containing_tbdf->independent_alpha());
    //t_translog * translog = table_translog(cdp, containing_tbdf);
    bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(containing_tbdf->basblockn, containing_tbdf->translog);
    if (basbl)
    { cached_values=0;
      basbl->unique_counter=next_available_value;
      bas_acc.data_changed(containing_tbdf->translog);  // destructor will write the changes
    }
  }
}

void table_descr::t_unique_value_cache::reset_counter(cdp_t cdp)
{ ProtectedByCriticalSection cs(&containing_tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
  cached_values=0;
  t_versatile_block bas_acc(cdp, containing_tbdf->independent_alpha());
  //t_translog * translog = table_translog(cdp, containing_tbdf);
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(containing_tbdf->basblockn, containing_tbdf->translog);
  if (basbl && basbl->unique_counter)
  { basbl->unique_counter=0;
    bas_acc.data_changed(containing_tbdf->translog);  // destructor will write the changes
  }
}

tblocknum table_descr::alloc_and_format_data_block(cdp_t cdp, unsigned blocknum)
{ tblocknum data_dadr;
  if (multpage && (blocknum % rectopage)) // not initialising
    data_dadr = alloc_alpha_block(cdp);
  else  // allocate and initialize:
  { t_versatile_block data_acc(cdp, translog==NULL);
    char * dt = data_acc.access_new_block(data_dadr, translog);
    if (dt==NULL) return 0;
    if (multpage) *dt = RECORD_EMPTY;
    else
      for (unsigned rec_in_page = 0;  rec_in_page < rectopage;  rec_in_page++)
        dt[recordsize*rec_in_page] = RECORD_EMPTY;
    data_acc.data_changed(translog);  // write changes in the destructor or on commit
  }
  return data_dadr;
}  

bool t_free_rec_pers_cache::store_to_cache(cdp_t cdp, trecnum rec, table_descr * tbdf) // returns false on error. Called from table's CS
{ if (cnt_records_in_cache==cache_size)
  { deflate_cache(cdp, tbdf, false);
    if (cnt_records_in_cache==cache_size)  // deflation error
      return false;
  }
  if (disc_container_is_empty())  // must preserve the sorting
  { int i=0;
    while (i<cnt_records_in_cache && reccache[i]>rec) i++;
    if (i<cnt_records_in_cache)
    { if (reccache[i]==rec) return true;  // masking the internal error
      memmove(reccache+i+1, reccache+i, sizeof(trecnum)*(cnt_records_in_cache-i));
    }
    reccache[i]=rec;
  }
  else  // appending
    reccache[cnt_records_in_cache]=rec;
  cnt_records_in_cache++;
  return true;
}

bool t_free_rec_pers_cache::store_to_cache_ordered(cdp_t cdp, trecnum rec, table_descr * tbdf) // returns false on error. Called from table's CS
// Adding from max to min, order is preserved automatically
{ if (cnt_records_in_cache==cache_size)
  { deflate_cache(cdp, tbdf, false);
    if (cnt_records_in_cache==cache_size)  // deflation error
      return false;
  }
  reccache[cnt_records_in_cache++]=rec;
  return true;
}

BOOL table_descr::expand_table_blocks(cdp_t cdp, unsigned new_record_space)
// Must be called from the own critical section of the table
{ unsigned new_block_count = record_count2block_count(new_record_space);
  if (new_block_count > MAX_BLOCK_COUNT_PER_TABLE)
    { request_error(cdp, TABLE_IS_FULL);  return TRUE; }
  t_versatile_block bas_acc(cdp, translog==NULL);
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(basblockn, translog);
  if (!basbl) return TRUE;
  unsigned old_block_count = record_count2block_count(basbl->recnum);
  t_versatile_block b2_acc(cdp, translog==NULL);  // closed, so far
  if (tbnum>=0)
  { unsigned need_blocks = new_block_count-old_block_count;
    if (new_block_count>MAX_BAS_ADDRS)
      need_blocks += (need_blocks / DADRS_PER_BLOCK) + 3;  // +1  should be sufficient
    if (!disksp.lock_and_alloc(cdp, need_blocks))
      return TRUE;  // not locked
  }    
  // now, I must call unlock_allocation(cdp); before return!!   
  tblocknum * blocknums;
  int blocknums_num = -1;  // number of block open in b2_acc and pointed by [blocknums], -1 iff none
 // reorganize the table, if necessary, do not change unique counter! 
  if (new_block_count>MAX_BAS_ADDRS && basbl->diraddr)
  {// allocate 1st b2 block:
    tblocknum blocknums_dadr;  
    blocknums = (tblocknum*)b2_acc.access_new_block(blocknums_dadr, translog);
    if (blocknums==NULL) goto err1;
   // move the list of data block to the b2 block, null the rest:
    memcpy(blocknums, basbl->databl, sizeof(tblocknum)*basbl->nbnums);
    memset(blocknums+basbl->nbnums, 0, (DADRS_PER_BLOCK-basbl->nbnums) * sizeof(tblocknum));
    memset(basbl->databl, 0, sizeof(tblocknum)*basbl->nbnums);
    blocknums_num=0; // number of fixed page containing block numbers
    b2_acc.data_changed(translog);
   // store the b2 block to basblock:
    basbl->databl[0] = blocknums_dadr;
    basbl->nbnums=1;
    basbl->diraddr=wbfalse;
    bas_acc.data_changed(translog);
  }
 // allocate new data pages:
  unsigned blocknum;  
  for (blocknum = old_block_count;  blocknum<new_block_count;  blocknum++)
  {// allocate a data page, initialize the DELETED columns, if containted in it: 
    tblocknum data_dadr = alloc_and_format_data_block(cdp, blocknum);
    if (!data_dadr) goto err2;
   // store the allocated block number into [basbl] or [blocknums]
    if (basbl->diraddr)
    { basbl->databl[basbl->nbnums++]=data_dadr;
      bas_acc.data_changed(translog);
    }
    else
    { int curr_blocknums_num = blocknum / DADRS_PER_BLOCK;
     // make the proper [blocknums] accessible:
      if (curr_blocknums_num != blocknums_num)
      { b2_acc.close();
        if (curr_blocknums_num < basbl->nbnums) // read the new [blocknums] if exists
        { blocknums = (tblocknum*)b2_acc.access_block(basbl->databl[curr_blocknums_num], translog);
          if (blocknums==NULL) goto err1;
        }
        else
        { tblocknum blocknums_dadr;  
          blocknums = (tblocknum*)b2_acc.access_new_block(blocknums_dadr, translog);
          if (blocknums==NULL) goto err1;
          memset(blocknums, 0, DADRS_PER_BLOCK*sizeof(tblocknum));
          basbl->databl[curr_blocknums_num]=blocknums_dadr;
          basbl->nbnums++;
          bas_acc.data_changed(translog);
        }
        blocknums_num = curr_blocknums_num;
      }
     // store the data page number to [blocknums]:
      blocknums[blocknum % DADRS_PER_BLOCK] = data_dadr;
      b2_acc.data_changed(translog);
    }
  }
  b2_acc.close();
  trecnum old_record_space;  old_record_space=basbl->recnum;
  basbl->recnum=new_record_space;
  bas_acc.data_changed(translog);
 // zoom the "tables" array if a new table added:
  if (tbnum==TAB_TABLENUM)
  { bool err;
    { ProtectedByCriticalSection cs(&cs_tables, cdp, WAIT_CS_TABLES);
      err = realloc_tables(cdp, (int)new_record_space-1);  // request_error called
    }
    if (err) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err1; }
  }
 // update the memory copy basblock:
  if (!update_basblock_copy(basbl))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err1; }
 // store the updated basblock:
  bas_acc.close();
 // add the new records to the cache, from max to min:
  while (old_record_space<new_record_space)
  { if (!free_rec_cache.store_to_cache_ordered(cdp, --new_record_space, this))
      break;
  }
  if (tbnum>=0)
  { disksp.flush_request();
    disksp.unlock_allocation(cdp);
  }  
  return FALSE;
 err2:
  disksp.unlock_allocation(cdp);
  b2_acc.close_damaged();
 err1:
  bas_acc.close_damaged();
  return TRUE;
}

void free_alpha_block(cdp_t cdp, tblocknum dadr, t_translog * translog)
{ if (translog && translog->tl_type!=TL_CLIENT) 
    translog->free_block(cdp, dadr);
  else // independent allocation
  { cdp->tl.log.drop_page_changes(cdp, dadr);  // should not be necessary?
    page_unlock(cdp, dadr, ANY_LOCK);  // remove all own locks from the block
    disksp.release_block_safe(cdp, dadr);   
    trace_alloc(cdp, dadr, false, "free alpha");
  }
}

tblocknum table_descr::alloc_alpha_block(cdp_t cdp)
{ if (translog) // TL_CLIENT translog is never stored in table_decsr
    return translog->alloc_block(cdp);
  else
  { tblocknum dadr = disksp.get_block(cdp);
    trace_alloc(cdp, dadr, true, "alpha");
    return dadr;
  }
}

void delete_free_record_list(cdp_t cdp, tblocknum head)
// head!=-1 supposed.
{ t_independent_block cont_acc(cdp);
  while (head)
  { t_free_rec_pers_cache::t_recnum_containter * contr = (t_free_rec_pers_cache::t_recnum_containter*)cont_acc.access_block(head);
    if (contr==NULL) return;
    if (!contr->valid()) return;  // internal error
   // delete the container block
    tblocknum next_contr = contr->next;
    cont_acc.close();  // not saving
    free_alpha_block(cdp, head, NULL);
    head=next_contr;
  }
}

BOOL drop_basic_table_allocation(cdp_t cdp, tblocknum basblockn, wbbool multpage, unsigned rectopage, unsigned new_record_space, t_translog * translog)
// Drops data blocks and b2-blocks and lists of free records.
// The list of free records is owned by disc basblock (made explicitly or implicitly when the table is uninstalled)
{ unsigned new_block_count = RECORD_COUNT2BLOCK_COUNT(new_record_space, multpage, rectopage);
  t_versatile_block bas_acc(cdp, !translog || translog->tl_type==TL_CLIENT);
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(basblockn, translog);
  if (!basbl) return TRUE;
  unsigned old_block_count = RECORD_COUNT2BLOCK_COUNT(basbl->recnum, multpage, rectopage);
  if (new_block_count<old_block_count)
  { unsigned ind, start;
    if (basbl->diraddr)
    { if (basbl->diraddr!=wbtrue)
      { cd_dbg_err(cdp, "Damaged basic block in the table");
        return TRUE;
      }
      for (ind=new_block_count;  ind<old_block_count;  ind++)
      { free_alpha_block(cdp, basbl->databl[ind], translog);
        basbl->databl[ind]=0;
      }
      basbl->nbnums=(uns16)new_block_count;
    }
    else
    { ind  =new_block_count / DADRS_PER_BLOCK;
      start=new_block_count % DADRS_PER_BLOCK;
      unsigned blocks_to_be_deleted = old_block_count - new_block_count;
      for (;  ind<basbl->nbnums;  ind++)  // cycle on secondary blocks
      { t_versatile_block b2_acc(cdp, !translog || translog->tl_type==TL_CLIENT);
        tblocknum * blocknums = (tblocknum*)b2_acc.access_block(basbl->databl[ind], translog);
        if (!blocknums) break;
        for (unsigned j=start;  j<DADRS_PER_BLOCK;  j++)
          if (blocks_to_be_deleted--)
          { free_alpha_block(cdp, blocknums[j], translog); 
            blocknums[j]=0;  
          }
          else break;
       // delete the secondary block iff it is empty
        if (!start)
        { b2_acc.close();  // not saving, must unfix BEFORE releasing!
          free_alpha_block(cdp, basbl->databl[ind], translog);
          basbl->databl[ind]=0;
        }
        else // otherwise store changes
        { b2_acc.data_changed(translog);
          b2_acc.close();
        }
        start=0;
      }
      if (new_block_count)
        basbl->nbnums=(uns16)((new_block_count-1)/DADRS_PER_BLOCK + 1);
      else
      { basbl->nbnums=0;
        basbl->diraddr=wbtrue;  /* otherwise error in tb_new */
      }
    }
    basbl->recnum=multpage ? new_block_count / rectopage : new_block_count * rectopage;
  }
 // drop the caches of the free records (the list of free records is owned by disc basblock):
 // caches will be restored explicitly, must not!!! mark the list as lost
  if (basbl->free_rec_list!=(tblocknum)-1)  // unless the list is lost
      delete_free_record_list(cdp, basbl->free_rec_list);   // delete_free_record_list added after error found
  basbl->free_rec_list=0;  // list is empty, will be restored explicitly by the caller
  bas_acc.data_changed(translog);
  return FALSE;
}

BOOL table_descr::reduce_table_blocks(cdp_t cdp, unsigned new_record_space)
// Decreases the number of data blocks allocated for the table.
// Deletes both lists of free records, they have to be restored later
// Free record cache contents is dropped, containers are released. Must re-create the cache, if new_record_space!=0.
{ BOOL res=FALSE;  
  //if (cdp->is_an_error()) return TRUE;
  // The above statement created lost blocks when an uncommitted cursor with temp. table was being destroyed in rollback because of error.
  ProtectedByCriticalSection cs(&lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
 // move the container of free records to basblock so that it can be released in drop_basic_table_allocation:
  free_rec_cache.return_free_record_list(cdp, this);  
  free_rec_cache.drop_caches();
 // reduce blocks:
  res=drop_basic_table_allocation(cdp, basblockn, multpage, rectopage, new_record_space, translog);
 // update_basblock_copy:
  { t_versatile_block bas_acc(cdp, !translog);
    bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(basblockn, translog);
    if (!update_basblock_copy(basbl))
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  res=TRUE; }
  }
  return res;
}

bool t_free_rec_pers_cache::t_recnum_containter::valid(void) const
  { return this!=NULL && contains<=recmax() && header.bpool_first+next < ffa.get_fil_size(); }

bool t_free_rec_pers_cache::take_free_record_list(cdp_t cdp, table_descr * td, bool uninstalling_table)  
// Takes the list of free records from the basblock or creates it. Supposes !owning_free_record_list(). Called from table's CS
{ if (td->tbnum<0)
    free_rec_list_head=0; // free record list is never stored to disc
  else  // permanent table
  { { t_independent_block bas_acc(cdp);
      bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(td->basblockn);
      if (basbl==NULL) return false;
      free_rec_list_head=basbl->free_rec_list;
     // store the basblock in the open state (unless it has already been open):
      if (basbl->free_rec_list!=(tblocknum)-1)
      { basbl->free_rec_list=(tblocknum)-1;
        bas_acc.data_changed();  // will be saved in destructor
      }
    } // destructs basblock access

    if (free_rec_list_head==(tblocknum)-1)  // the list has been lost, must create a new one
    { if (uninstalling_table)  // cannot search deleted records now, stop the opertation
        return false;  // cache contents will not be saved into the non-existing record list, it will be restored when necessary
     // restoring the list of free records (must not be called inside commit() when released record numbers are in the transaction log)
      free_rec_list_head=0;  // init [free_rec_list_head] now, store_to_cache() may call deflate_cache() which uses this!
      t_table_record_rdaccess table_record_rdaccess(cdp, td);
     // from MAX to MIN: 
      trecnum rec = td->Recnum();
      while (rec)
      { rec--;
        const char * dt = table_record_rdaccess.position_on_record(rec, 0);
        if (!dt) return false; // error
        if (*dt==RECORD_EMPTY) 
          store_to_cache_ordered(cdp, rec, td);
      }
    }
    else 
    { ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS);
      disksp.flush_and_time(cdp);
    }  
  }
  return true;
}

void t_free_rec_pers_cache::return_free_record_list(cdp_t cdp, table_descr * tbdf)  
{ if (free_rec_list_head!=(tblocknum)-1)
  { if (tbdf->tbnum<0) // free record list is never stored to disc, dropping it instead
      drop_free_rec_list(cdp);
    else  // store the head to baslblock
    { t_independent_block bas_acc(cdp);
      bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tbdf->basblockn);
      if (basbl==NULL) return;
     // store the basblock in the open state:
      basbl->free_rec_list=free_rec_list_head;
      bas_acc.data_changed();  // will be saved in destructor
    }
   // take the list head:
    free_rec_list_head=(tblocknum)-1;
  } // destructs basblock access
}

void t_free_rec_pers_cache::inflate_cache(cdp_t cdp, table_descr * tbdf, unsigned cache_limit)
// Moves record numbers from the list of free records to the cache. Called from table's CS
{ //if (tbdf->tbnum<0) return;
  if (!make_free_record_list_accessible(cdp, tbdf))
    return;
  t_independent_block cont_acc(cdp);
  while (!disc_container_is_empty())
  { t_recnum_containter * contr = (t_recnum_containter*)cont_acc.access_block(free_rec_list_head);
    if (contr==NULL) return;
    if (!contr->valid()) 
      { free_rec_list_head=0;  return; }
    while (contr->contains)
    { reccache[cnt_records_in_cache++]=contr->recs[--contr->contains];
      if (cnt_records_in_cache>=cache_limit) break;
    }
    if (contr->contains)
    { cont_acc.data_changed(); // will be saved in destructor
      break;
    }
   // delete the container block
    tblocknum next_contr = contr->next;
    cont_acc.close();  // not saving
    free_alpha_block(cdp, free_rec_list_head, NULL);
    free_rec_list_head=next_contr;
    if (cnt_records_in_cache>=cache_limit) break;
  }
  if (disc_container_is_empty())  // then the cache must be sorted
    sort_cache();
}

void t_free_rec_pers_cache::sort_cache(void)
// Bad algorithm, but it is not used extensively.
{ bool changed;
  do
  { changed=false;
    for (int i=1;  i<cnt_records_in_cache;  i++)
      if (reccache[i-1]<reccache[i])
      { trecnum r;
        r=reccache[i-1];  reccache[i-1]=reccache[i];  reccache[i]=r;
        changed=true;
      }  
  } while (changed);
}

bool t_free_rec_pers_cache::list_records(cdp_t cdp, table_descr * tbdf, uns32 * map, trecnum * count)
{ bool doublerec = false;
  if (count) *count=cnt_records_in_cache;
  if (map)
    for (int j = 0;  j<cnt_records_in_cache;  j++)
    { trecnum rec = reccache[j];
        if (rec>tbdf->Recnum())
          doublerec=true;
        else if (map[rec/32] & (1<<(rec % 32))) 
          doublerec=true;
        else map[rec/32] |= (1<<(rec % 32));
    }
  if (!make_free_record_list_accessible(cdp, tbdf))
    return doublerec;
  t_independent_block cont_acc(cdp);
  tblocknum next_contr = free_rec_list_head;
  while (next_contr)
  { t_recnum_containter * contr = (t_recnum_containter*)cont_acc.access_block(next_contr);
    if (contr==NULL) return doublerec;
    if (!contr->valid()) return true;
    if (count) *count+=contr->contains;
    if (map)
      for (int i=0;  i<contr->contains;  i++)
      { trecnum rec = contr->recs[i];
        if (rec>tbdf->Recnum())
          doublerec=true;
        else if (map[rec/32] & (1<<(rec % 32))) 
          doublerec=true;
        else map[rec/32] |= (1<<(rec % 32));
      }
   // go to the next container block:
    next_contr = contr->next;
    cont_acc.close();  // not saving
  }
  return doublerec;
}

void t_free_rec_pers_cache::deflate_cache(cdp_t cdp, table_descr * tbdf, bool uninstalling_table)
// Moves record numbers from the cache to the list of free records. Called from table's CS
{ //if (tbdf->tbnum<0) return;
  unsigned cache_limit = 0; // cache_size / 6;   full deflating used when uninstalling the table
  if (!owning_free_record_list())
    if (!take_free_record_list(cdp, tbdf, uninstalling_table))
      return;
  t_independent_block cont_acc(cdp);
  if (free_rec_list_head)  // list exists, start by adding records to it
  { t_recnum_containter * contr = (t_recnum_containter*)cont_acc.access_block(free_rec_list_head);
    if (contr==NULL) return;
    if (!contr->valid()) // internal error
      free_rec_list_head=0;  
    else  
    { unsigned limit = t_recnum_containter::recmax();
      while (contr->contains<limit)
      { contr->recs[contr->contains++]=reccache[--cnt_records_in_cache];
        cont_acc.data_changed(); // will be saved below
        if (cnt_records_in_cache<=cache_limit) break;
      }
      cont_acc.close();
    }  
  }
 // adding new container blocks:
  while (cnt_records_in_cache>cache_limit) 
  { tblocknum dadr;
    t_recnum_containter * contr = (t_recnum_containter*)cont_acc.access_new_block(dadr);
    if (contr==NULL) return;
    contr->contains=0;   contr->next=free_rec_list_head;  free_rec_list_head=dadr;
    unsigned limit = t_recnum_containter::recmax();
    while (contr->contains<limit)
    { contr->recs[contr->contains++]=reccache[--cnt_records_in_cache];
      if (cnt_records_in_cache<=cache_limit) break;
    }
    cont_acc.data_changed();
    cont_acc.close();
  }
}

void t_free_rec_pers_cache::close_caching(cdp_t cdp, table_descr * tbdf)
{ if (tbdf->tbnum<0)  // table is being destroyed
    cnt_records_in_cache=0;
  else // table is being uninstalled
  { if (!cache_empty()) // without this test it may take&return the list without any reason
      deflate_cache(cdp, tbdf, true);
  }
  return_free_record_list(cdp, tbdf);
}

bool t_free_rec_pers_cache::make_cache(unsigned size)
{ if (reccache)   // this happens when calling prepare() again, after rolling back the cration of a local table
    corefree(reccache);  // .. or when resizing the empty cache
  reccache=(trecnum*)corealloc(sizeof(trecnum) * size, 65);
  if (!reccache) return false;
  cache_size=size;
  return true;
}

trecnum t_free_rec_pers_cache::get_from_cache(cdp_t cdp, table_descr * tbdf)  // returns NORECNUM iff cannot allocate, called from table's CS
{ if (cache_empty())  // core cache empty, inflating from the disc cache
  { if (tbdf->Recnum() > 500000 && cache_size<5000)
      make_cache(5000);
    else if (tbdf->Recnum() > 50000 && cache_size<1000)  
      make_cache(1000);
    inflate_cache(cdp, tbdf, inflate_step());
    if (cache_empty()) return NORECNUM;  // both caches are empty
  }
  return reccache[--cnt_records_in_cache];  // if the cache is ordered, takes the minimal record
}

trecnum t_free_rec_pers_cache::extract_for_append(cdp_t cdp, table_descr * tbdf)  // returns NORECNUM iff cannot find
// called from table's CS
{ if (!tbdf->Recnum()) return NORECNUM;  // table empty
  if (cache_empty())  // core cache empty, inflating from the disc cache
  { inflate_cache(cdp, tbdf, cache_size);  // inflating as much as possible, makes the successfull extracting more probable
    if (cache_empty()) return NORECNUM;  // both caches are empty
  }
  // I do not have to check if disc_container_is_empty(), but the best append record will probably not be found
  trecnum rec = tbdf->Recnum()-1;
 // cache is not empty: 
  if (reccache[0]!=rec)
    return NORECNUM;  // the last record of the table is not in the cache
  int i = 1;
  while (i<cnt_records_in_cache && reccache[i]==rec-1)
    { i++; rec--; }
 // return (i-1)th record:
  memcpy(reccache+i-1, reccache+i, sizeof(trecnum)*(cnt_records_in_cache-i));
  cnt_records_in_cache--;
  return rec;
}

trecnum tb_new(cdp_t cdp, table_descr * tbdf, BOOL app, const t_vector_descr * recval, t_tab_trigger * ttr)
/* Adds a new record into the tb table (table_descr locked). */
/* Returns NORECNUM on error, returns new record number otherwise. */
{ trecnum therec;
  { ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    do
    { if (app) // must append
      { therec=tbdf->free_rec_cache.extract_for_append(cdp, tbdf);
        if (therec==NORECNUM)
        {
#ifdef FLUSH_OPTIZATION        
         // first I must make space for a appended records in the cache:
          if (!tbdf->free_rec_cache.cache_is_empty())
            tbdf->free_rec_cache.deflate_cache(cdp, tbdf, false);
          if (tbdf->expand_table_blocks(cdp, tbdf->Recnum() + tbdf->free_rec_cache.space_in_cache() - 1))
          // -1 for on-line deleting records during import in internal format
            return NORECNUM;
          therec=tbdf->free_rec_cache.get_from_cache(cdp, tbdf);  // gets the record with the min number
          if (therec==NORECNUM) return NORECNUM;  // never
#else
          if (tbdf->expand_table_blocks(cdp, tbdf->Recnum()+1))
            return NORECNUM;
          therec=tbdf->free_rec_cache.get_from_cache(cdp, tbdf);
#endif            
        }
      }
      else
      { therec=tbdf->free_rec_cache.get_from_cache(cdp, tbdf);
        if (therec==NORECNUM) // cache empty and cannot inflate, must expand the allocation
        { unsigned new_record_space = tbdf->Recnum()+tbdf->free_rec_cache.inflate_step();
          if (tbdf->expand_table_blocks(cdp, new_record_space))
            return NORECNUM;
          therec=tbdf->free_rec_cache.get_from_cache(cdp, tbdf);
        }
      }
      // testing the state of the record would be more effective in tb_write_vector() but it must be done before storing the record in the log
      if (table_record_state(cdp, tbdf, therec) == RECORD_EMPTY)
        break;  // record is OK
      { char msg[100+OBJ_NAME_LEN];
        sprintf(msg, "Warning: an invalid record %u eliminated from the list of free records on table %s.", therec, tbdf->selfname);
        cd_dbg_err(cdp, msg);
      }
    } while (true);
  }
  t_translog * translog = table_translog(cdp, tbdf);
  if (translog->tl_type!=TL_CURSOR)  // not rolling back
    translog->log.record_inserted(tbdf->tbnum, therec); 

  t_exkont_table ekt(cdp, tbdf->tbnum, therec, NOATTRIB, NOINDEX, OP_INSERT, NULL, 0);
  if (cdp->trace_enabled & TRACE_INSERT) 
  { char buf[81];
    trace_msg_general(cdp, TRACE_INSERT, form_message(buf, sizeof(buf), msg_trace_insert), tbdf->tbnum); // detailed info is in the kontext
  }  
 /* record into the journal (must be before initial_rights!, must be before journaling the values) */
  if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
  { worm_add(cdp, &cdp->jourworm, recval && recval->delete_record ? &insdel_opcode : app ? &append_opcode : &insert_opcode, 1);
    worm_add(cdp, &cdp->jourworm, &tbdf->tbnum, sizeof(ttablenum));
  }
 /* initializing the record: */
  const t_vector_descr * pvector;
  pvector = recval ? recval : &default_vector;
  if (tb_write_vector(cdp, tbdf, therec, pvector, TRUE, ttr))
    { therec=NORECNUM;  goto ex; }
 // write the implicit record privileges:
  if (tbdf->rec_privils_atr!=NOATTRIB && !(recval && recval->delete_record))
    initial_rights(cdp, tbdf, therec);
 // if (tbdf->tbnum==TAB_TABLENUM) table_initial_rights(cdp, (ttablenum)therec); -- called after def is stored
 ex:
  return therec;
}

/***************************************************************************/
void table_info(cdp_t cdp, table_descr * tbdf, trecnum * counts)
/* Calculates table statistics. Supposes table_descr to be locked. */
{ trecnum lim, del=0, nd=0, fr=0, rec;  byte delbyte;
  lim=tbdf->Recnum();  
  for (rec=0;  rec<lim;  rec++)
  { delbyte=table_record_state(cdp, tbdf, rec);
    if (!delbyte) nd++;
    else if (delbyte==DELETED) del++;
    else /*if (delbyte==RECORD_EMPTY) */ fr++;
    if (cdp->break_request)
      { request_error(cdp, REQUEST_BREAKED);  break; }
    report_step(cdp, rec, lim);
  }
  report_total(cdp, rec);
  counts[0]=nd; counts[1]=del; counts[2]=fr;
}

typedef struct   /* a local structure in "delete_history" */
{ tattrib atr;   // traced attribute
  uns16 itmsize; // history item size
  uns16 dtmoffs; // datim offset in the history item
} hist_info;
#define MAX_HIST_ATTRS 20

void delete_history(cdp_t cdp, table_descr * tbdf, BOOL datim_type, timestamp dtm)
/* table_descr of tb is supposed to be locked */
{ hist_info hst[MAX_HIST_ATTRS];   int hist_num;  
  int i,j;  BOOL isdat;  unsigned sz;  tfcbnum fcbn;
  uns32 origsize, newsize; uns8 del; tptr p, buf;  const char * dt;
  unsigned origitm; trecnum rec;
  t_translog * translog = table_translog(cdp, tbdf);
 // find the history attributes:
  hist_num=0;
  for (i=1;  (uns8)i < tbdf->attrcnt;  i++)
    if (tbdf->attrs[i].attrtype==ATT_HIST)
    {// find preceeding autor and datim attributes:
      isdat=FALSE;  sz=0;
      for (j=i-1;  j;  j--)
      { if      (tbdf->attrs[j].attrtype==ATT_AUTOR)
          sz+=UUID_SIZE;
        else if (tbdf->attrs[j].attrtype==ATT_DATIM)
          { sz+=sizeof(timestamp);  isdat=TRUE; }
        else break;
      }
      if (j)
      { hst[hist_num].itmsize=sz+
         (hst[hist_num].dtmoffs=typesize(tbdf->attrs+j));
        hst[hist_num].atr=(tattrib)i;
        if ((tbdf->attrs[i-2].attrtype==ATT_AUTOR) &&
            (tbdf->attrs[i-1].attrtype==ATT_DATIM))
                         hst[hist_num].dtmoffs += UUID_SIZE;
        if (isdat || !datim_type) hist_num++;
        if (hist_num>=MAX_HIST_ATTRS) break;
      }
    }
  if (!hist_num) return;  // no history attributes found

  for (rec=0;  rec<tbdf->Recnum();  rec++)
  { del=table_record_state(cdp, tbdf, rec);
    if (del==OUT_OF_CURSOR) return;
    if (del!=RECORD_EMPTY)
      for (i=0; i<hist_num; i++)
      { dt=fix_attr_read(cdp, tbdf, rec, hst[i].atr, &fcbn);
        if (!dt) return;
        origitm=(origsize=*(const uns32*)(dt+sizeof(tpiece))) / hst[i].itmsize;
        if (datim_type || ((timestamp)origitm > dtm))
        { buf=(tptr)corealloc((uns16)origsize,OW_HIST);
          if (buf!=NULL)
          { if (hp_read(cdp, (const tpiece*)dt,1,0,hst[i].itmsize * origitm,buf))
              { corefree(buf);  unfix_page(cdp, fcbn);  return; }
           /* find the div-point in the history (j) */
            if (datim_type)
            { p=buf+hst[i].dtmoffs;  j=0;
              while (*(timestamp*)p < dtm && j < origitm)
                { p+=hst[i].itmsize;  j++; }
              p-=hst[i].dtmoffs;
            }
            else
              { j=origitm-(uns16)dtm; p=buf+j*hst[i].itmsize; }
           // remove j items:
            if (j)
            { unfix_page(cdp, fcbn);  tptr dtw;
              if (!(dtw=fix_attr_write(cdp, tbdf, rec, hst[i].atr, &fcbn)))
                { corefree(buf);  return; }
              newsize=(origitm-j)*hst[i].itmsize;
              *(uns32*)(dtw+sizeof(tpiece))=newsize;
              if (newsize)
                if (hp_write(cdp, (tpiece*)dtw, 1, 0, newsize, p, translog))
                  { corefree(buf);  unfix_page(cdp, fcbn);  return; }
              if (hp_shrink_len(cdp, (tpiece*)dtw, 1, newsize, translog))
                { corefree(buf);  unfix_page(cdp, fcbn);  return; }
            }
            corefree(buf);
          }
        } // history must be checked
        unfix_page(cdp, fcbn);
      } // cycle on history attributes
  } // cycle on table records
}

/********************* release the table space ******************************/
static void release_record(cdp_t cdp, table_descr * tbdf, trecnum rec)
// Releases var. size parts of the record, does not overwrite the record.
// Has to be called for tables with has_variable_allocation().
{ tfcbnum fcbn; 
  t_translog * translog = table_translog(cdp, tbdf);
  for (tattrib atr=1;  atr<tbdf->attrcnt;  atr++)
  { attribdef * att = tbdf->attrs+atr;
    if (IS_HEAP_TYPE(att->attrtype))
      if (att->attrmult==1)
      { const heapacc * ptmphp = (const heapacc*)fix_attr_read_naked(cdp, tbdf, rec, atr, &fcbn);
        if (ptmphp)
        { heapacc tmphp=*ptmphp;
          hp_free(cdp, &tmphp.pc, translog);
          unfix_page(cdp, fcbn);
        }
      }
      else
      { t_mult_size ind, num;  
        if (!tb_read_ind_num(cdp, tbdf, rec, atr, &num))
          for (ind=0; ind<num; ind++)
          { const heapacc * ptmphp=(const heapacc*)fix_attr_ind_read(cdp, tbdf, rec, atr, ind, &fcbn); // is not "naked" and will generate warning when releasing a deleted record
            if (ptmphp)
            { heapacc tmphp;
              tmphp=*ptmphp;
              hp_free(cdp, &tmphp.pc, translog);
              unfix_page(cdp, fcbn);
            }
          }
      }
    else if (IS_EXTLOB_TYPE(att->attrtype))  
    { const char * dt = fix_attr_read_naked(cdp, tbdf, rec, atr, &fcbn);
      if (dt)
      { ext_lob_free(cdp, *(uns64*)dt);
        unfix_page(cdp, fcbn);
      }
    }
   // release multiattribute extension (for both heap and simple types):
    if (att->attrmult & 0x80)
    { const char * dt = fix_attr_read_naked(cdp, tbdf, rec, atr, &fcbn);
      if (dt)
      { tpiece tmppc = *(const tpiece*)(dt-sizeof(tpiece));
        hp_free(cdp, &tmppc, translog);
        unfix_page(cdp, fcbn);
      }
    }
  } // cycle on columns
}

static void release_variable_data(cdp_t cdp, table_descr * tbdf)
// Releases variable data from all records to the translog of tbdf. Does not overwrite the records.
{ if (tbdf->has_variable_allocation())
  { t_table_record_rdaccess table_record_rdaccess(cdp, tbdf);
    for (trecnum rec=0;  rec<tbdf->Recnum();  rec++)
    { const char * dt = table_record_rdaccess.position_on_record(rec, 0);
      if (!dt) break; // error
      if (*dt==NOT_DELETED || *dt==DELETED)
        release_record(cdp, tbdf, rec);
    }
  }
}

bool t_exclusive_table_access::open_excl_access(table_descr * tbdfIn, bool allow_for_reftab)
{ tbdf=tbdfIn;
  if (!record_lock_error(cdp, tbdf, FULLTABLE, WLOCK))
  { if (!record_lock_error(cdp, tabtab_descr, tbdf->tbnum, changing_tabdef ? WLOCK : RLOCK))
    { ProtectedByCriticalSection cs(&cs_tables);
      if (tbdf->deflock_counter==1 || allow_for_reftab && !memcmp(tbdf->selfname, "FTX_REFTAB", 10)) // "FTX_REFTAB..." may be locked by the fulltext context
      { tables[tbdf->tbnum]=(table_descr*)-1;
        has_access = true;
       // create private access to the table:
        upper_exclusively_accessed_table = cdp->exclusively_accessed_table;
        cdp->exclusively_accessed_table = tbdf;
        return true;
      }
      SET_ERR_MSG(cdp, tbdf->selfname);  request_error(cdp, NOT_LOCKED);  
      record_unlock(cdp, tabtab_descr, tbdf->tbnum, changing_tabdef ? WLOCK : RLOCK);
    }
    record_unlock(cdp, tbdf, FULLTABLE, WLOCK);
  }
  return false;
}

t_exclusive_table_access::~t_exclusive_table_access(void)
{ if (has_access)
  { record_unlock(cdp, tabtab_descr, tbdf->tbnum, changing_tabdef ? WLOCK : RLOCK);
    record_unlock(cdp, tbdf, FULLTABLE, WLOCK);
    if (!drop_descr)
    { ProtectedByCriticalSection cs(&cs_tables);
      tables[tbdf->tbnum]=tbdf;
    }
    else
    { if (tables[tbdf->tbnum]==(table_descr*)-1) tables[tbdf->tbnum]=NULL;  // a new descriptor may already be there!
      tbdf->destroy_descr(cdp);
    }
   // restore the previous private access to a table:
    cdp->exclusively_accessed_table = upper_exclusively_accessed_table;
    PulseEvent(hTableDescriptorProgressEvent);
  }
}

static void destroy_indexes(cdp_t cdp, table_descr * tbdf)
// Destroys all indexes of tbdf to its translog but does not overwrite anything.
{ int i;  dd_index * cc;
  t_translog * translog = table_translog(cdp, tbdf);
  for (i=0, cc=tbdf->indxs;  i<tbdf->indx_cnt;  i++, cc++)
    if (cc->root)
      drop_index(cdp, cc->root, cc->keysize+sizeof(tblocknum), translog);
}

bool can_delete_record(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ t_privils_flex priv_val;
  cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
  return (*priv_val.the_map() & RIGHT_DEL) != 0; 
}

bool can_insert_record(cdp_t cdp, table_descr * tbdf)
{ t_privils_flex priv_val;
  cdp->prvs.get_effective_privils(tbdf, NORECNUM, priv_val);
  return (*priv_val.the_map() & RIGHT_INSERT) != 0; 
}

bool can_read_objdef(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ t_privils_flex priv_val;
  cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
  return priv_val.has_read(OBJ_DEF_ATR); 
}

bool can_write_objdef(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ t_privils_flex priv_val;
  cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
  return priv_val.has_write(OBJ_DEF_ATR); 
}

bool truncate_table(cdp_t cdp, table_descr * tbdf)
// Commits the transaction. 
// Releases variable data, drops indices except for roots.
// Re-creates index roots. 
// If OK, commits and deallocates data blocks.
{ commit(cdp);  // must not call when there is an exclusive access
 // cannot truncate system tables:
  if (SYSTEM_TABLE(tbdf->tbnum)) { request_error(cdp, NO_RIGHT);  return true; }
 // check privileges:
  if (!can_delete_record(cdp, tbdf, NORECNUM))
    { request_error(cdp, NO_RIGHT);  return true; }
 // truncate in exclusive access:
  { t_exclusive_table_access eta(cdp, false);
    if (!eta.open_excl_access(tbdf, true)) return true; 
   // release pieces from var.len attr. & multiattributes:
    release_variable_data(cdp, tbdf);
   // release indicies, reinit index roots:
    int i;  dd_index * cc;
    for (i=0, cc=tbdf->indxs;  i<tbdf->indx_cnt;  i++, cc++)
      truncate_index(cdp, cc->root, cc->keysize+sizeof(tblocknum), table_translog(cdp, tbdf)); 
    tbdf->unique_value_cache.reset_counter(cdp);
    if (commit(cdp)) return true;  // error when truncating
   // release data blocks:
    tbdf->reduce_table_blocks(cdp, 0);
  }
  commit(cdp); // should not be necessary, no transactional changes generated
  return false;
}

void destroy_table_data(cdp_t cdp, table_descr * tbdf)
{/* release pieces from var.len attr. & multiattributes */
  release_variable_data(cdp, tbdf);
 /* release indicies, delete index roots */
  destroy_indexes(cdp, tbdf);
 /* release data blocks */
  tbdf->reduce_table_blocks(cdp, 0);
  free_alpha_block(cdp, tbdf->basblockn, NULL);
}

void table_rel(cdp_t cdp, ttablenum tb)
// Normal deleting of a permanent table: alpha and beta
{ table_descr * tbdf;
  tbdf=install_table(cdp, tb);
  if (tbdf!=NULL)
  { release_variable_data(cdp, tbdf);
    unregister_index_changes_on_table(cdp, tbdf);  // called before destroy_indexes() which may overwrite roots, destroy_indexes() does not write to the index log.
    destroy_indexes(cdp, tbdf);
    register_basic_deallocation(cdp, tbdf->basblockn, tbdf->multpage, tbdf->rectopage, 0);  // postponed to commit
    unlock_tabdescr(tbdf);
    force_uninst_table(cdp, tb);  // must be done before the bacblock is deallocated because it returns free rec list to the basblock!
  }
}

BOOL tb_del(cdp_t cdp, table_descr * tbdf, trecnum recnum)
/* Supposes that table_descr of tb is locked */
{ tfcbnum fcbn;  tptr dt;  
  if (recnum==NORECNUM) return FALSE;   /* it's possible via cursor */
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, NOATTRIB, NOINDEX, OP_DELETE, NULL, 0);
  t_translog * translog = table_translog(cdp, tbdf);
  if (cdp->trace_enabled & TRACE_DELETE) 
  { char buf[81];
    trace_msg_general(cdp, TRACE_DELETE, form_message(buf, sizeof(buf), msg_trace_delete), tbdf->tbnum); // detailed info is in the kontext
  }  
 // If deleted records are immediately released in the commit, take the record list now: doing it in the commit may create inconsistency
 //  if the list has been lost and must be restored.
  if (!(cdp->sqloptions & SQLOPT_EXPLIC_FREE) || translog->tl_type==TL_LOCTAB)
  { ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    tbdf->free_rec_cache.make_free_record_list_accessible(cdp, tbdf);
  }
  if (!(dt=fix_attr_x(cdp, tbdf, recnum, &fcbn))) 
    return TRUE;
 // now the record is write-locked, nobody can be overwriting/deleting it simultaneously:
  if (*dt!=NOT_DELETED) 
    warn(cdp, IS_DEL);   // the record is not a valid on, do nothing
  else
  {// check for the token: 
    if (tbdf->token_attr!=NOATTRIB)
      if (!(*(uns16*)(dt+tbdf->attrs[tbdf->token_attr].attroffset) & 0x8000))
        if (!cdp->mask_token)
        { request_error(cdp, NO_WRITE_TOKEN);
          unfix_page(cdp, fcbn);  return TRUE;
        }
   // remove the old contents from fulltext indexes:
    tbdf->prepare_trigger_info(cdp);  // create the list of fulltexts
    if (tbdf->fulltext_info_list!=NULL)
      if (!(cdp->ker_flags & KFL_STOP_INDEX_UPDATE))
      { int pos=-1;  t_ftx_info info;  
        bool saved_in_expr_eval;
        saved_in_expr_eval=cdp->in_expr_eval;  
        cdp->in_expr_eval=true;  // prevents rollback or commit in the statement
        t_atrmap_flex atrmap(tbdf->attrcnt);  atrmap.set_all();
        while (tbdf->get_next_table_ft(pos, atrmap, info))
        { t_fulltext2 * ftx = get_fulltext_system(cdp, info.ftx_objnum, NULL);
          if (ftx)
          {// read id:
            sig64 idval = 0;  // value may be sig32 or sig64
            tb_read_atr(cdp, tbdf, recnum, info.id_col, (char*)&idval);
            ftx->remove_doc(cdp, (t_docid)idval);
            ftx->Release();
          }
        }
        cdp->in_expr_eval=saved_in_expr_eval;
      }
    { t_simple_update_context sic(cdp, tbdf, 0);    
      if (!sic.lock_roots() || !sic.inditem_remove(recnum, (tptr)1))  /* remove from ALL indicies */
        { unfix_page(cdp, fcbn);  return TRUE; }
    }
   // delete it (must be after inditem_remove, otherwise reading from deleted record warns):
    if (!(cdp->sqloptions & SQLOPT_EXPLIC_FREE) || translog->tl_type==TL_LOCTAB)
    { if (tbdf->has_variable_allocation()) release_record(cdp, tbdf, recnum);
      *dt=RECORD_EMPTY;
      if (translog->tl_type!=TL_CURSOR)
        translog->log.record_released(tbdf->tbnum, recnum);
    }
    else
    { *dt=DELETED; 
     // store token, ZCR, LUO (only if deleted records remain accesible):
      if (tbdf->zcr_attr!=NOATTRIB)  // ZCR update must be before inditem_add!
      { write_gcr(cdp, dt+tbdf->attrs[tbdf->zcr_attr].attroffset);
        if (tbdf->luo_attr!=NOATTRIB)
          memcpy(dt+tbdf->attrs[tbdf->luo_attr].attroffset, cdp->luo, UUID_SIZE);
      }
    }
    if (tbdf->zcr_attr!=NOATTRIB && !SYSTEM_TABLE(tbdf->tbnum))
    { table_descr *tdr = GetReplDelRecsTable(cdp);
      if (tdr)
      { uns8 attype;
        t_specif atspec;
        if (RDR_UNIKEY != (tattrib)-1)
        { attype = tdr->attrs[RDR_UNIKEY].attrtype;
          atspec = tdr->attrs[RDR_UNIKEY].attrspecif;
        }
        int i;
        char buf[MAX_KEY_LEN];
        dd_index *ic;
        dd_index *rc = NULL;    // _W5_UNIKEY
        dd_index *uc = NULL;    // Primarni klic
        dd_index *cc = NULL;
        for (i = 0, ic = tbdf->indxs; i < tbdf->indx_cnt; i++, ic++)
        { if (strcmp(ic->constr_name, "_W5_UNIKEY") == 0)
          { cc = ic;
            rc = ic;
            break;
          }
          if (RDR_UNIKEY != (tattrib)-1 && (ic->ccateg == INDEX_PRIMARY || ic->ccateg == INDEX_UNIQUE) && ic->partnum == 1 && ic->parts[0].type == attype && ic->parts[0].specif == atspec)
            uc = ic;
        }
        if (!cc)
            cc = uc;
        if (!cc)
          trace_msg_general(cdp, TRACE_REPLICATION, form_message(buf, sizeof(buf), CantWriteDelRecs, tbdf->selfname), tbdf->tbnum);
        else
        { BOOL ok = true;
          char *piv;
          if (cc->parts[0].colnum)
            piv = dt+tbdf->attrs[cc->parts[0].colnum].attroffset;
          else
          { ok = compute_key(cdp, tbdf, recnum, cc, buf, false);
            piv = buf;
          }
          if (ok)
          { trecnum Pos = tb_new(cdp, tdr, FALSE, NULL);
            if (Pos != NORECNUM)
            { timestamp ts = stamp_now();
              tb_write_atr(cdp, tdr, Pos, RDR_SCHEMAUUID, tbdf->schema_uuid); 
              tb_write_atr(cdp, tdr, Pos, RDR_TABLENAME,  tbdf->selfname);
              if (rc && RDR_W5_UNIKEY != (tattrib)-1)
                  tb_write_atr(cdp, tdr, Pos, RDR_W5_UNIKEY, piv);
              else
                  tb_write_atr(cdp, tdr, Pos, RDR_UNIKEY,    piv);
              tb_write_atr(cdp, tdr, Pos, RDR_ZCR,        dt+tbdf->attrs[tbdf->zcr_attr].attroffset);
              tb_write_atr(cdp, tdr, Pos, RDR_DATETIME,   &ts);
            }
          }
        }
      }
    }
   // transaction log of changes:
    register_change_map(cdp, tbdf, recnum, NULL);  // must add the deleted record into the list of changes!
    if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
    { worm_add(cdp, &cdp->jourworm,&del_opc,     1                );
      worm_add(cdp, &cdp->jourworm,&tbdf->tbnum, sizeof(ttablenum));
      worm_add(cdp, &cdp->jourworm,&recnum ,     sizeof(trecnum)  );
    }
  }
  unfix_page(cdp, fcbn);
  return FALSE;
}

BOOL tb_undel(cdp_t cdp, table_descr * tbdf, trecnum recnum)
/* Supposes that table_descr of tb is locked */
{ tfcbnum fcbn;  tptr dt;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, NOATTRIB, NOINDEX, OP_UNDEL, NULL, 0);
  if (tbdf->tbnum==TAB_TABLENUM)   /* cannot undelete tables, but can table links! */
  { tcateg categ;
    if (fast_table_read(cdp, tabtab_descr, recnum, OBJ_CATEG_ATR, &categ))
      return TRUE;
    if (!(categ & IS_LINK))   /* is a real table, not a link */
      { request_error(cdp, EMPTY);  return TRUE; }    // error, calling from API only
  }
  if (!(dt=fix_attr_x(cdp, tbdf, recnum, &fcbn))) return TRUE;
  if (*dt==NOT_DELETED) warn(cdp, IS_NOT_DEL);
  else if (*dt==RECORD_EMPTY)
    { request_error(cdp, EMPTY);  unfix_page(cdp, fcbn);  return TRUE; }  // error, calling from API only
  else
  { t_simple_update_context sic(cdp, tbdf, 0);
    if (sic.lock_roots())
    { *dt=NOT_DELETED;
      sic.inditem_add(recnum);  /* add into ALL indicies */
      if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
      { worm_add(cdp, &cdp->jourworm,&undel_opc,  1                );
        worm_add(cdp, &cdp->jourworm,&tbdf->tbnum,sizeof(ttablenum));
        worm_add(cdp, &cdp->jourworm,&recnum   ,  sizeof(trecnum)  );
      }
      register_change_map(cdp, tbdf, recnum, NULL); // make checks like after inserting a new record (parent tables may have changed when the record was deleted etc.)
    }
  }
  unfix_page(cdp, fcbn);
  return FALSE;
}

BOOL tb_rlock(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ 
  if (tbdf->tbnum!=TAB_TABLENUM || !SYSTEM_TABLE(recnum))  // definition of system tables can be always locked (system table views need this)
    if (!cdp->prvs.any_eff_privil(tbdf, recnum, FALSE))
      { request_error(cdp, NO_RIGHT);  return TRUE; }
  return record_lock_error(cdp, tbdf, recnum, RLOCK);
}

BOOL tb_wlock(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ if (!cdp->prvs.any_eff_privil(tbdf, recnum, TRUE))
    { request_error(cdp, NO_RIGHT);  return TRUE; }
  if (record_lock_error(cdp, tbdf, recnum, WLOCK))
    return TRUE;
  if (tbdf->tbnum==TAB_TABLENUM && recnum!=NORECNUM)  /* lock all records if table def. is to be changed */
  { table_descr_auto tbdf2(cdp, (ttablenum)recnum);
    if (!tbdf2->me() || record_lock_error(cdp, tbdf2->me(), FULLTABLE, WLOCK))
      { record_unlock(cdp, tbdf, recnum, WLOCK);  return TRUE; }
  }
  return FALSE;
}

BOOL tb_unrlock(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ if (record_unlock(cdp, tbdf, recnum,RLOCK))
    { /*SET_ERR_MSG(cdp, NULLSTRING);  request_error(cdp, NOT_LOCKED);*/  request_error(cdp, ERROR_IN_FUNCTION_ARG);  return TRUE; }
  return FALSE;
}

BOOL tb_unwlock(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ if (record_unlock(cdp, tbdf, recnum, WLOCK))
    { /*SET_ERR_MSG(cdp, NULLSTRING);  request_error(cdp, NOT_LOCKED);*/  request_error(cdp, ERROR_IN_FUNCTION_ARG);  return TRUE; }
  if (tbdf->tbnum==TAB_TABLENUM && recnum!=NORECNUM)  /* unlock records if table def. unlocked */
  { table_descr_auto tbdf2(cdp, (ttablenum)recnum);
    if (!tbdf2->me()) return TRUE;
    record_unlock(cdp, tbdf2->me(), FULLTABLE, WLOCK);
  }
  return FALSE;
}

/***************************** indicies *************************************/
SPECIF_DYNAR(t_record_dynar, trecnum, 200);

static BOOL changing_master_key(cdp_t cdp, dd_index * indx, tptr oldval, const char * newval, ttablenum par_tbnum, int par_indnum)
// indx->reftabnums!=NULL supposed
{ BOOL res=FALSE;
  if (!is_null_key(oldval, indx)) // NULL key does not link
  { ttablenum *ptabnum;
    uns8 saved_in_active_ri = cdp->in_active_ri;
    for (ptabnum = indx->reftabnums;  *ptabnum;  ptabnum++) // cycle on referencing tables
    { ttablenum chtabnum = *ptabnum;
      if (chtabnum!=(ttablenum)-1)  /* table exists */
      { table_descr_auto ch_tbdf(cdp, chtabnum);
        if (!ch_tbdf->me()) return TRUE;
        int j;  dd_forkey * ch_cc;
        for (j=0, ch_cc=ch_tbdf->forkeys;  j<ch_tbdf->forkey_cnt;  j++, ch_cc++) // search for the proper constrain in the referencing table
          if (ch_cc->desttabnum==par_tbnum && ch_cc->par_index_num==par_indnum)
          { dd_index * ch_ind=&ch_tbdf->indxs[ch_cc->loc_index_num];
            bool deleting = newval==(tptr)1;
            uns8 rule = deleting ? ch_cc->delete_rule : ch_cc->update_rule;
            t_record_dynar list;
           // search the old key in the referencing table index ch_ind, create list of records with the same key: 
            { t_btree_read_lock btrl;
              if (!btrl.lock_and_update(cdp, indx->root))  // covers the whole function, used by t_btree_acc and by locked_find_key()
                { res=TRUE;  break; }
              bt_result bres;  t_btree_acc bac;
              *(trecnum*)(oldval+ch_ind->keysize-sizeof(trecnum))=0; // prepare searching the key
              cdp->index_owner = ch_tbdf->tbnum;
              bres=bac.build_stack(cdp, ch_ind->root, ch_ind, oldval);
              if (bres==BT_ERROR) { res=TRUE;  break; }
              if (bres==BT_EXACT_KEY || bres==BT_EXACT_REC)
              {// add the records with the same key value to the list
                char key2[MAX_KEY_LEN];  
                memcpy(key2, oldval, ch_ind->keysize);
                do 
                { if (!bac.btree_step(cdp, key2)) break;
                  sig32 res=cmp_keys(key2, oldval, ch_ind);
                  if (!HIWORD(res)) break;
                  trecnum * prec = list.next();
                  if (prec==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  res=TRUE;  break; }
                  *prec = *(trecnum*)(key2+ch_ind->keysize-sizeof(trecnum));
                } while (rule!=REFRULE_NO_ACTION);  // for NO_ACTION the list is not necessary, only its non-emptyness is relevant
              }
            } // destructors unlock the index here

           // process records in the child table:
            if (list.count())
            { if (rule==REFRULE_NO_ACTION) // integrity violated, set error
              { t_exkont_table ekt(cdp, ch_tbdf->tbnum, NORECNUM, NOATTRIB, NOINDEX, OP_PTRSTEPS, NULL, 0);
                { t_exkont_key ekt(cdp, ch_ind, oldval);
                  SET_ERR_MSG(cdp, ch_cc->constr_name);  request_error(cdp, REFERENTIAL_CONSTRAINT);
                }
                res=TRUE;  break;
              }
              else  // active rule
              {// process the records:
                int k;
                cdp->in_active_ri=1;
                if (rule==REFRULE_CASCADE && deleting)
                { if (cdp->sqloptions & SQLOPT_NO_REFINT_TRIGGERS)
                    for (k=0;  k<list.count();  k++)
                      tb_del(cdp, ch_tbdf->me(), *list.acc0(k));
                  else // deleting with triggers
                  { t_tab_trigger ttr;
                    if (!ttr.prepare_rscopes(cdp, ch_tbdf->me(), TGA_DELETE) ||
                         ttr.pre_stmt (cdp, ch_tbdf->me()))            { res=TRUE;  goto err1; }
                    for (k=0;  k<list.count() && !cdp->roll_back_error;  k++)
                    { trecnum rec = *list.acc0(k);
                      if (ttr.pre_row (cdp, ch_tbdf->me(), rec, NULL)) { res=TRUE;  goto err1; }
                     // delete the record:
                      if (tb_del      (cdp, ch_tbdf->me(), rec))       { res=TRUE;  goto err1; }
                     // AFT triggers:
                      if (ttr.post_row(cdp, ch_tbdf->me(), rec))       { res=TRUE;  goto err1; }
                    }
                    if (ttr.post_stmt (cdp, ch_tbdf->me()))            { res=TRUE;  goto err1; }
                  }
                }
                else // write the new values (NULL, default, newval) to the record of the referencing table
                { t_colval * colvals = new t_colval[ch_ind->partnum+1];
                  if (colvals)
                  { t_vector_descr vector(colvals);  
                   // prepare structures for vectorised write:
                    t_tab_trigger ttr;
                    ttr.prepare_rscopes(cdp, ch_tbdf->me(), TGA_UPDATE);
                    unsigned offset=0;
                    for (int part=0;  part<ch_ind->partnum;  part++)
                    { t_express * ex = ch_ind->parts[part].expr;
                      if (ex->sqe!=SQE_ATTR) 
                      { SET_ERR_MSG(cdp, ch_cc->constr_name);  request_error(cdp, REFERENTIAL_CONSTRAINT);  
                        delete [] colvals;
                        res=TRUE;  goto err1; 
                      }
                      colvals[part].attr=((t_expr_attr*)ex)->elemnum;
                      colvals[part].index=colvals[part].expr=NULL;
                      colvals[part].table_number=0;
                      if      (rule==REFRULE_SET_DEFAULT) colvals[part].dataptr=DATAPTR_DEFAULT;
                      else if (rule==REFRULE_SET_NULL   ) colvals[part].dataptr=DATAPTR_NULL;
                      else // update cascade
                      { colvals[part].dataptr=newval+offset;  offset+=ch_ind->parts[part].partsize; }
                      ttr.add_column_to_map(colvals[part].attr);
                    }
                    colvals[ch_ind->partnum].attr=NOATTRIB; // the delimiter
                   // write the new values:
                    uns32 saved_ker_flags=cdp->ker_flags;  cdp->ker_flags |= KFL_DISABLE_REFCHECKS;
                    ttr.pre_stmt (cdp, ch_tbdf->me());  // BEF UPD STM triggers
                    for (k=0;  k<list.count() && !cdp->roll_back_error;  k++)
                      tb_write_vector(cdp, ch_tbdf->me(), *list.acc0(k), &vector, FALSE, (cdp->sqloptions & SQLOPT_NO_REFINT_TRIGGERS) ? NULL : &ttr);
                    ttr.post_stmt(cdp, ch_tbdf->me());  // AFT UPD STM triggers
                    cdp->ker_flags=saved_ker_flags;
                    delete [] colvals;
                  } // colvals allocated OK
                  else request_error(cdp, OUT_OF_KERNEL_MEMORY);
                } // writing cascaded or NULL or default values
              } // active rule
            } // any referencing record exists
          }  // index in referencing table found
       err1: 
          cdp->in_active_ri=saved_in_active_ri;  // restoring saved value after goto
      } // referencing table found
    } // cycle on referencing tables
  } // old key is not null
  return res;
}

t_simple_update_context::t_simple_update_context(cdp_t cdpIn, const table_descr * tbdfIn, tattrib attribIn) : 
    cdp(cdpIn), tbdf(tbdfIn), attrib(attribIn), indmap(INDX_MAX)
{ dd_index * cc;  int ind;
  update_disabled = true;
  if (!(cdp->ker_flags & KFL_STOP_INDEX_UPDATE))
  { indmap.clear();
    for (ind=0, cc=tbdf->indxs;  ind<tbdf->indx_cnt;  ind++, cc++)
      if (!cc->disabled)
        if (!attribIn || cc->atrmap.has(attribIn))
          { indmap.add(ind);  update_disabled=false; }
    if (!update_disabled)
      if (!tbdf->referencing_tables_listed) 
        prepare_list_of_referencing_tables(cdpIn, (table_descr*)tbdfIn);
  }
}

bool t_simple_update_context::lock_roots(void)
// Returns false on error, must not call inditem_remove/add then.
{ dd_index * cc;  int ind;
  if (update_disabled) return true;
  for (ind=0, cc=tbdf->indxs;  ind<tbdf->indx_cnt;  ind++, cc++)
    if (indmap.has(ind))
      if (!btrl[ind].lock_and_update(cdp, cc->root))
        return false;
  return true;
}

void t_simple_update_context::unlock_roots(void)
// Does not need to be called, unlocked in the destructor.
{ dd_index * cc;  int ind;
  if (update_disabled) return;
  for (ind=0, cc=tbdf->indxs;  ind<tbdf->indx_cnt;  ind++, cc++)
    if (indmap.has(ind))
      btrl[ind].unlock();  // some may have not been locked if an error occurred
}


bool t_simple_update_context::inditem_remove(trecnum recnum, const char * newval)
// Removes record from indicies dependent on attrib (from all if !attrib).
// If newval!=NULL then performs ref.int. actions on depending tables.
// The index must be locked.
{ dd_index * cc;  char keyval[MAX_KEY_LEN];  int ind;
  if (update_disabled) return true;
  for (ind=0, cc=tbdf->indxs;  ind<tbdf->indx_cnt;  ind++, cc++)
    if (indmap.has(ind))
    { t_exkont_index ekt(cdp, tbdf->tbnum, ind);
      cdp->index_owner = tbdf->tbnum;
      /* prepare the key value */
      if (compute_key(cdp, tbdf, recnum, cc, keyval, false))
      { bt_remove(cdp, keyval, cc, cc->root, table_translog(cdp, tbdf));
        if (cc->reftabnums && newval!=NULL && !(cdp->ker_flags & KFL_DISABLE_REFINT))
        { if (newval==(tptr)1)
          { if (changing_master_key(cdp, cc, keyval, newval, tbdf->tbnum, ind)) 
              return false;
          }
          else // new value specified, create the new key
          {// create the new key: 
            char new_keyval[MAX_KEY_LEN];
            memcpy(new_keyval, keyval, cc->keysize);
            unsigned offset=0;
            for (int part=0;  part<cc->partnum;  part++)
            { t_express * ex = cc->parts[part].expr;
              if (ex->sqe==SQE_ATTR && ((t_expr_attr*)ex)->elemnum==attrib) 
              { memcpy(new_keyval+offset, newval, cc->parts[part].partsize);
                break;
              }
              offset+=cc->parts[part].partsize;
            }
            if (changing_master_key(cdp, cc, keyval, new_keyval, tbdf->tbnum, ind)) 
              return false;
          }
        }
      }
    }
  return true;
}

bool t_simple_update_context::inditem_add(trecnum recnum)
// The index must be locked.
{ dd_index * cc;   char keyval[MAX_KEY_LEN];  int ind;
  if (update_disabled) return true;
  for (ind=0, cc=tbdf->indxs;  ind<tbdf->indx_cnt;  ind++, cc++)
    if (indmap.has(ind))
    { t_exkont_index ekt(cdp, tbdf->tbnum, ind);
      cdp->index_owner = tbdf->tbnum;
      /* prepare the key value */
      if (compute_key(cdp, tbdf, recnum, cc, keyval, false))
      { if (bt_insert(cdp, keyval, cc, cc->root, tbdf->selfname, table_translog(cdp, tbdf))) 
          break;
      }
    }
  return true;
}

void enable_index(cdp_t cdp, table_descr * tbdf, int which, bool enable)
// Only for permanent tables (create_btree needs this)
/* table_descr of table is locked. */
// Updating indices is transactional. If succeeds, memory and basblock are updated too.
// Data, table definition and indices are locked in order to prevent other threads using them.
// basblock is protected by the table CS.
// Locking when disabling, too, in order to prevent problems when trying to enable indexes.
// NEW since 22.2.2008: not releasing the old index if it is not disabled (safer)
{ int i;  dd_index * cc;
 /* lock table contents and def: */
  if (!record_lock_error(cdp, tbdf, FULLTABLE, WLOCK))
  { if (!record_lock_error(cdp, tabtab_descr, tbdf->tbnum, RLOCK))
    { if (enable)  // drop & re-create indicies: 
      { ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
        t_independent_block bas_acc(cdp);
        bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tbdf->basblockn);
        if (basbl!=NULL)
        {// prevent downing the server here: 
          int orig_conditions = the_sp.down_conditions.val();
          the_sp.down_conditions.friendly_save(0);
          for (i=0, cc=tbdf->indxs;  i<tbdf->indx_cnt;  i++, cc++)
          { if (which==-1 || which==i)
            { tblocknum new_root=0, old_root=basbl->indroot[i];
              bool unlock_old_root = false;
              if (old_root && cc->disabled)
              { cdp->index_owner = tbdf->tbnum;
                if (wait_page_lock(cdp, old_root, WLOCK)) continue;
                unlock_old_root=true;
                drop_index(cdp, old_root, cc->keysize+sizeof(tblocknum), &cdp->tl);
              }
              new_root=create_btree(cdp, tbdf, cc);
              cc->disabled=false;
             // commit every index separately in order to limit the transaction size:
              if (!cdp->is_an_error() && !commit(cdp)) 
              {// saving the new index roots: 
                basbl->indroot[i]=new_root;
                bas_acc.data_changed();
                tbdf->update_basblock_copy(basbl);
                cc->root=new_root;
              }
              else roll_back(cdp);  // basblock not changed, cc->root not changed, 
              if (unlock_old_root)
                page_unlock(cdp, old_root, WLOCK);
            }
          }
          the_sp.down_conditions.friendly_save(orig_conditions);
        }  // basblock fixed
      }
      else  // disable
        for (i=0, cc=tbdf->indxs;  i<tbdf->indx_cnt;  i++, cc++)
          if (which==-1 || which==i) cc->disabled=true;
      record_unlock(cdp, tabtab_descr, tbdf->tbnum, RLOCK);
    }  // table def locked
    record_unlock(cdp, tbdf, FULLTABLE, WLOCK);
  }  // all records locked
}

BOOL free_deleted(cdp_t cdp, table_descr * tbdf)
// Reviewing records starts on the end of the table because the biggest continous block of deleted records 
//   on the end of the table will be released and need not to be overwritten.
// The other records must be marked as released and put into the cached list of free records.
// The lists of free records are dropped and re-created, because it is easier than removing invalid records from them.
{ trecnum rec, cnt, lastvalid;
  bool repl_temp_stopped = false;
  if (GetReplInfo())
    if (tbdf->tbnum==REPL_TABLENUM || tbdf->tbnum==SRV_TABLENUM)  // tabdescr locked by the replicator
    { repl_stop();         // stops iff running
      repl_temp_stopped=true;
    }
  commit(cdp);  // must not call when there is an exclusive access
  t_translog * translog = table_translog(cdp, tbdf);
  //ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
  BOOL err=FALSE;
  { t_exclusive_table_access eta(cdp, false);
    if (!eta.open_excl_access(tbdf, true)) err=TRUE; 
    else
    { if (tbdf->Recnum())
      {  /* ATTN: It will free all records it cannot access!!! Locks can damage it! */
         // In the 1st phase t_table_record_rdaccess may have been used, but t_table_record_wraccess is not significantly slower.
          cnt=0;  // progress notification counter
         // releasing but not overwriting records until the 1st valid record is found:
          bool valid_record_found = false;
          do
          { trecnum partend = 0;
            if (tbdf->Recnum() > 70000)
            { partend = tbdf->Recnum() - 50000;
              if (!tbdf->multpage)  // align it to the block boundary
                partend = partend / tbdf->rectopage * tbdf->rectopage;
            }
           // releases data in deleted records, stops on last valid record:
            { t_table_record_wraccess table_record_wraccess(cdp, tbdf);
              rec=tbdf->Recnum();
              do
              { if (rec-- <= partend) break;  // all records passed
                tptr dt = table_record_wraccess.position_on_record(rec, 0);
                if (!dt) break; // error
                if (*dt==NOT_DELETED) 
                { valid_record_found=true;
                  break;  // the record [rec] is not deleted
                }
                if (*dt==DELETED)
                  if (tbdf->has_variable_allocation())
                    release_record(cdp, tbdf, rec);   /* overwriting DEL_ATR not necess. */
                cnt++;
                report_step(cdp, cnt);
              } while (true);
             // now rec is the last valid record or (trecnum)-1 or the record with access error
            }
           // t_table_record_wraccess must be closed before reduce_table_blocks, otherwise page fixed by access may be unfixed by reduce.
            tbdf->reduce_table_blocks(cdp, rec+1);  // rec+1 is [partend] or last_valid_record_number+1
            commit(cdp);  // must first recude and then commit, otherwise the table would contain valid records with released contents
          } while (rec!=(trecnum)-1 && !valid_record_found);
          lastvalid=rec;  // no valid records when rec==(trecnum)-1 

         // processing the reduced blocks:
          tbdf->free_rec_cache.make_free_record_list_accessible(cdp, tbdf);  // without this cannot return the records to the cache
          { t_table_record_wraccess table_record_wraccess(cdp, tbdf);
           // mark as empty the free records in the last used block, but do not release them (they have been released before)
            if (lastvalid+1 < tbdf->Recnum())
            { for (rec=lastvalid+1;  rec<tbdf->Recnum();  rec++)
              { tptr dt = table_record_wraccess.position_on_record(rec, 0);
                if (!dt) break;  // error
                if (*dt==DELETED) 
                { *dt=RECORD_EMPTY;
                  table_record_wraccess.positioned_record_changed();
                }
                translog->log.record_released(tbdf->tbnum, rec);
              }
            }
           // releasing the deleted records located between valid records:
            if (valid_record_found)
              for (rec=0;  rec<lastvalid;  rec++)  /* lastvalid is valid, doesn't need to be checked */
              { tptr dt = table_record_wraccess.position_on_record(rec, 0);
                if (!dt) break; // error
                if (*dt==DELETED)
                { if (tbdf->has_variable_allocation()) release_record(cdp, tbdf, rec);       
                  *dt=RECORD_EMPTY;
                  table_record_wraccess.positioned_record_changed();
                }
                if (*dt==RECORD_EMPTY)  // released either now or before, but is a free record
                  translog->log.record_released(tbdf->tbnum, rec);
                if (!(cnt % 100000))
                { table_record_wraccess.unposition();  // commit would chage the positioned private page into shared!
                  commit(cdp);
                }
                report_step(cdp, cnt);
                if (!(++cnt % 64))
                { if (cdp->break_request)
                    { request_error(cdp,REQUEST_BREAKED);  err=TRUE;  goto errexit; }
                }
              }
          }
          report_total(cdp, cnt);
          if (!valid_record_found) // reset counter if freed all
            tbdf->unique_value_cache.reset_counter(cdp);
      } // table nonempty
    } // table not used, exclusive access granted
  } // t_exclusive_table_access scope
 errexit:
  commit(cdp);  // must not call then there is an exclusive access
  if (repl_temp_stopped) repl_start(cdp);  // restart
  return err;
}

#ifdef STOP
tptr position_page(cdp_t cdp, table_descr * tbdf, trecnum rec, int pagenum, tfcbnum * fcbn, bool changing)
{ unsigned offset, pgrec;
  t_translog * translog = table_translog(cdp, tbdf);
  tblocknum datadadr = ACCESS_COL_VAL(cdp, tbdf, rec, pagenum, offset, pgrec);
  if (!datadadr) return NULL;  // error detected in ACCESS_COL_VAL
  tptr dt=fix_priv_page(cdp, datadadr, fcbn, translog) + offset;
  if (changing && dt!=NULL)
    translog->log.rec_changed(*fcbn, datadadr, tbdf->recordsize, pgrec);
  return dt;
}
#endif

BOOL compact_table(cdp_t cdp, table_descr * tbdf)
// tabdescr is supposed to be locked, indicies to be disabled
{ BOOL err=FALSE;
  //ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
  bool repl_temp_stopped = false;
  if (GetReplInfo())
    if (tbdf->tbnum==REPL_TABLENUM || tbdf->tbnum==SRV_TABLENUM)  // tabdescr locked by the replicator
    { repl_stop();         // stops iff running
      repl_temp_stopped=true;
    }
  t_translog * translog = table_translog(cdp, tbdf);
  if (tbdf->Recnum())
  { t_exclusive_table_access eta(cdp, false);
    if (!eta.open_excl_access(tbdf, true)) err=TRUE; 
    else
    { int bufsize, recsperbuf;
      //if (tbdf->recordsize >= 0xfff0)
        recsperbuf=1; 
      //else
        //recsperbuf=0xfff0 / tbdf->recordsize;  
      bufsize = recsperbuf * tbdf->recordsize;
      tptr buf = (tptr)corealloc(bufsize, 42);
      if (buf==NULL) request_error(cdp, OUT_OF_KERNEL_MEMORY);
      else
      { trecnum destrec, srcrec;  int moved=0;  uns8 dltd;
        destrec=0;  srcrec=tbdf->Recnum()-1;
        trecnum last_valid=NORECNUM;
       // compacting:
        do // all records below destrec are valid, above srcrec are free
        {// find empty record: 
          while (destrec < srcrec)
          { dltd=table_record_state(cdp, tbdf, destrec);
            if (dltd==RECORD_EMPTY) break;
            if (dltd==DELETED)  /* record not empty yet, make it empty */
            { if (tbdf->has_variable_allocation()) release_record(cdp, tbdf, destrec);  
              break; 
            }
            last_valid=destrec++;
          }
          if (destrec == srcrec) 
          { dltd=table_record_state(cdp, tbdf, srcrec);
            if (!dltd) last_valid=srcrec;
            break;
          }
         // I have valid free space on destrec now
         // find valid record:
          while (destrec < srcrec)
          { dltd=table_record_state(cdp, tbdf, srcrec);
            if (!dltd) break;
            if (dltd==DELETED)  /* record not empty yet, make it empty */
              if (tbdf->has_variable_allocation()) release_record(cdp, tbdf, srcrec);  // destrec < srcrec -> not releasig twice the same record
            srcrec--;
          }
          if (destrec >= srcrec) break;
          int pagenum=0;  
         // moving the record from srcrec to destrec:
          { t_table_record_wraccess destacc(cdp, tbdf);
            t_table_record_rdaccess srcacc(cdp, tbdf);
            const char * srcdt;  char * destdt;  
            if (tbdf->multpage) // copy the full blocks first
            { do
              { srcdt =srcacc.position_on_record(srcrec, pagenum);
                if (srcdt ==NULL) { destrec=0;  break; }
                destdt=destacc.position_on_record(destrec, pagenum);
                if (destdt==NULL) { destrec=0;  break; }
                memcpy(destdt, srcdt, BLOCKSIZE);
                destacc.positioned_record_changed();
              } while (++pagenum+1 < tbdf->rectopage);
            }
           // the last or the partial block:
            srcdt =srcacc.position_on_record(srcrec, pagenum);
            if (srcdt ==NULL) break; 
            destdt=destacc.position_on_record(destrec, pagenum);
            if (destdt==NULL) break;
            memcpy(destdt, srcdt, tbdf->recordsize % BLOCKSIZE);
            destacc.positioned_record_changed();
          }  
          if (moved+=pagenum > 20) { moved=0;  commit(cdp); }
          last_valid=destrec;
          destrec++;  srcrec--;
        } while (TRUE);
       // reduce the number of table blocks, drop caches of free records:
        tbdf->reduce_table_blocks(cdp, last_valid+1);
       // mark as empty the free records in the last used block, but do not release them (they have been released before)
        t_table_record_wraccess table_record_wraccess(cdp, tbdf);
        if (last_valid+1 < tbdf->Recnum())
        { for (trecnum rec=last_valid+1;  rec<tbdf->Recnum();  rec++)
          { tptr dt = table_record_wraccess.position_on_record(rec, 0);
            if (!dt) break;  // error
            if (*dt!=RECORD_EMPTY)
            { *dt=RECORD_EMPTY;
              table_record_wraccess.positioned_record_changed();
            }
            translog->log.record_released(tbdf->tbnum, rec);
          }
        }
        if (last_valid==NORECNUM) // reset counter if freed all
            tbdf->unique_value_cache.reset_counter(cdp);
        corefree(buf);
      }
    } // excl. acces granted
  } // table nonempty
  if (repl_temp_stopped) repl_start(cdp);  // restart
  return err;
}

// index names must not start with _W5_, CREATE TABLE execution would not store them!
static char tabtab_sqldef[] =
 "CREATE TABLE TABTAB ZCR REC_PRIVILS("
 "TAB_NAME CHAR(31) COLLATE CSISTRING, CATEGORY CHAR, APL_UUID BINARY(12),"
 "DEFIN LONG VARCHAR,FLAGS SMALLINT,FOLDER_NAME CHAR(31) COLLATE CSISTRING,LAST_MODIF TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
 "DRIGHTS LONG VARBINARY, BASBL INTEGER,"
 "CONSTRAINT _WX_prim_key PRIMARY KEY (TAB_NAME,CATEGORY,APL_UUID))";
#define TABTAB_INDEX_COUNT 2 // ZCR and PRIMARY
static char objtab_sqldef[] =
 "CREATE TABLE OBJTAB ZCR REC_PRIVILS("
 "OBJ_NAME CHAR(31) COLLATE CSISTRING, CATEGORY CHAR, APL_UUID BINARY(12),"
 "DEFIN LONG VARCHAR,FLAGS SMALLINT,FOLDER_NAME CHAR(31) COLLATE CSISTRING,LAST_MODIF TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
 "CONSTRAINT _WX_prim_key PRIMARY KEY (OBJ_NAME,CATEGORY,APL_UUID))";
static char usertab_sqldef[] =
 "CREATE TABLE USERTAB ZCR REC_PRIVILS("
 "LOGNAME CHAR(31) COLLATE CSISTRING, CATEGORY CHAR, USR_UUID BINARY(12),"
 "UPS_DEFIN LONG VARBINARY,"
 "IDENTIF BINARY(122), HOME_SERVER BINARY(12),"
 "PASSWORD BINARY(20), PASSCRE DATE, STOPPED BIT, CERTIFIC BIT,"
 "CONSTRAINT _WX_prim_key PRIMARY KEY (USR_UUID),"
 "CONSTRAINT _WX_logname  UNIQUE (LOGNAME,CATEGORY))";
static char srvtab_sqldef[] =
 "CREATE TABLE SRVTAB(SRVNAME CHAR(31),SRV_UUID BINARY(12),"
 "PUBKEY LONG VARBINARY,ADDR1 CHAR(254),ADDR2 CHAR(254),APPL_INFO LONG VARCHAR,"
 "RECV_NUM INT,F_OFFSET INT,RECV_FNAME CHAR(12),"
 "SEND_NUM INT,ACKN BIT,SEND_FNAME CHAR(12),"
 "REPLTAB_GCR INT,USERTAB_GCR INT,USE_ALT BIT)";
static char repltab_sqldef[] =
 "CREATE TABLE REPLTAB ZCR(SOURCE_UUID BINARY(12),DEST_UUID BINARY(12),"
 "APPL_UUID BINARY(12),TABNAME CHAR(31),ATTRMAP BINARY(32),REPLCOND LONG VARCHAR,"
 "NEXT_REPL_TIME INT,GCR_LIMIT BINARY(6))";
static char keytab_sqldef[] =
 "CREATE TABLE KEYTAB REC_PRIVILS(KEY_UUID BINARY(12),USER_UUID BINARY(12),"
 "PUBKEYVAL LONG VARBINARY,CREDATE TIMESTAMP,EXPDATE TIMESTAMP,"
 "IDENTIF BINARY(122), CERTIFIC BIT,"
 "CERTIFICS LONG VARBINARY,"
 "CONSTRAINT _WX_prim_key PRIMARY KEY(KEY_UUID),"
 "CONSTRAINT _WX_user INDEX(USER_UUID))";
static char proptab_sqldef[] =
 "CREATE TABLE __PROPTAB("
 "PROP_NAME CHAR(36) COLLATE CSISTRING,OWNER CHAR(32) COLLATE CSISTRING,NUM SMALLINT,VAL CHAR(254),"
 "CONSTRAINT _WX_prim_key PRIMARY KEY(OWNER,PROP_NAME,NUM))";
static char reltab_sqldef[] =
 "CREATE TABLE __RELTAB("
 "NAME1 CHAR(31) COLLATE CSISTRING,APL_UUID1 BINARY(12),NAME2 CHAR(31) COLLATE CSISTRING,APL_UUID2 BINARY(12),"
 "CLASSNUM SMALLINT,INFO INT,"
 "CONSTRAINT _WX_prim_key PRIMARY KEY(CLASSNUM,NAME1,APL_UUID1,NAME2,APL_UUID2),"
 "CONSTRAINT _WX_21 INDEX(CLASSNUM,NAME2,APL_UUID2))";
////////////////////////////// tbdf methods ////////////////////////////////
void * table_descr::operator new(size_t size, unsigned column_cnt, unsigned name_cnt)
  { return corealloc(size + (column_cnt-1) * sizeof(attribdef) + name_cnt * 2*sizeof(tobjname), 92); }

table_descr::table_descr(void) : unique_value_cache(this)
{ attrcnt=0;    // attrcnt increated on each call of add_column
  schemaname[0]=selfname[0]=0;
  indx_cnt=check_cnt=forkey_cnt=0;
  indxs=NULL;  checks=NULL;  forkeys=NULL;
  notnulls     =NULL;
  basblock     =NULL;
  triggers.init();
  translog     =NULL;
 // not used by destructor, but necessary otherwise:
  trigger_map  =0;
  released=FALSE;
  deflock_counter=0; 
  referencing_tables_listed=FALSE;
  trigger_info_valid=FALSE;
  trigger_scope=NULL;
  owning_cdp=NULL;
  fulltext_info_generation=-1;  // none
  fulltext_info_list=NULL;
}

void table_descr::add_column(const char * nameIn, uns8 typeIn, uns8 multiIn, wbbool nullableIn, t_specif specifIn)
// table_descr is supposed to have space for the column. "name" is supposed to have the proper length.
{ attribdef * att = attrs+attrcnt++;
  strcpy(att->name, nameIn);  
  att->attrtype=typeIn;  att->attrmult=multiIn;  att->nullable=nullableIn;  att->attrspecif=specifIn;
  att->defvalexpr=NULL; // used by free_descr in release_properties!
}

void table_descr::release_properties(void)  // release table definition related objects contained in the table_descr
{ int i;  dd_index * cc;  attribdef * att;
  for (i=0, att=attrs;  i<attrcnt;    i++, att++)
    if (att->defvalexpr && att->defvalexpr!=COUNTER_DEFVAL) delete att->defvalexpr;
  for (i=0, cc=indxs;  i<indx_cnt;  i++, cc++)
    cc->root=0;  // prevents reporting "data_not_closed"
 // this calls individual destructors:
  if (indxs)   { delete [] indxs;    indxs  =NULL; }
  if (checks)  { delete [] checks;   checks =NULL; }
  if (forkeys) { delete [] forkeys;  forkeys=NULL; }
  corefree(notnulls);  notnulls=NULL;
  triggers.~trig_type_and_num_dynar();
  if (trigger_scope) { trigger_scope->Release();  trigger_scope=NULL; }  // may be still used in: detached procedure -> compiled UPDATE statement -> trigger support -> run scope for trigger -> sscope reference
  corefree(fulltext_info_list);  fulltext_info_list=NULL;
}

void table_descr::free_descr(cdp_t cdp)
/* Does not free locks on the table. */
{ release_properties();
  if (basblock)
  { free_rec_cache.close_caching(cdp, this);
    corefree(basblock);
  }
  if (!deflock_counter || tbnum<0) delete this;
  else 
    released=TRUE;  // delayed
}

void table_descr::destroy_descr(cdp_t cdp)
{ release_properties();
  if (basblock)
  { free_rec_cache.close_caching(cdp, this);
    corefree(basblock);
  }
  delete this;
}

static table_descr * create_table_descr(cdp_t cdp, ttablenum tb); // called inside cs_tables

table_descr * reinstall_table(cdp_t cdp, ttablenum tb)
/* table_descr is re-created according to the new table definition.
   Locks on the table are preserved.  -- no more, I do not know why
   Does not change deflock_counter.
   table_descr is supposed to be locked. */
{ EnterCriticalSection(&cs_tables);
  table_descr * old_tbdf=tables[tb];
  table_descr * new_tbdf = create_table_descr(cdp, tb);  // this exits cs_tables
  if (new_tbdf!=NULL)
  { EnterCriticalSection(&old_tbdf->lockbase.cs_locks);
    if (new_tbdf->attrcnt <= old_tbdf->attrcnt)  // preserve the pointers to the tbdf
    { old_tbdf->release_properties();
     // copy info:
      old_tbdf->is_traced       = new_tbdf->is_traced;
      bas_tbl_block_info * bb = old_tbdf->basblock;  old_tbdf->basblock = new_tbdf->basblock;  new_tbdf->basblock = bb;
      old_tbdf->basblockn       = new_tbdf->basblockn;
      old_tbdf->trigger_map     = new_tbdf->trigger_map;
      old_tbdf->level           = new_tbdf->level;
      old_tbdf->recordsize      = new_tbdf->recordsize;
      old_tbdf->multpage        = new_tbdf->multpage;
      old_tbdf->rectopage       = new_tbdf->rectopage;
      old_tbdf->indx_cnt        = new_tbdf->indx_cnt;    new_tbdf->indx_cnt  =0;
      old_tbdf->check_cnt       = new_tbdf->check_cnt;   new_tbdf->check_cnt =0;
      old_tbdf->forkey_cnt      = new_tbdf->forkey_cnt;  new_tbdf->forkey_cnt=0;
      old_tbdf->attrcnt         = new_tbdf->attrcnt;
      old_tbdf->tabdef_flags    = new_tbdf->tabdef_flags;
      old_tbdf->rec_privils_atr = new_tbdf->rec_privils_atr;
      old_tbdf->zcr_attr        = new_tbdf->zcr_attr;
      old_tbdf->luo_attr        = new_tbdf->luo_attr;
      old_tbdf->unikey_attr     = new_tbdf->unikey_attr;
      old_tbdf->token_attr      = new_tbdf->token_attr;
      old_tbdf->detect_attr     = new_tbdf->detect_attr;
      strcpy(old_tbdf->selfname,  new_tbdf->selfname);
      strcpy(old_tbdf->schemaname,new_tbdf->schemaname);
      memcpy(old_tbdf->schema_uuid, new_tbdf->schema_uuid, UUID_SIZE);

     // copy column info:
      memcpy(old_tbdf->attrs, new_tbdf->attrs, new_tbdf->attrcnt * sizeof(attribdef));
     // clear moved defvals:
      for (int i=0;  i<new_tbdf->attrcnt;  i++)
        new_tbdf->attrs[i].defvalexpr=NULL;
     // move constrains, notnulls, triggers:
      old_tbdf->indxs   =new_tbdf->indxs;     new_tbdf->indxs   =NULL;
      old_tbdf->checks  =new_tbdf->checks;    new_tbdf->checks  =NULL;
      old_tbdf->forkeys =new_tbdf->forkeys;   new_tbdf->forkeys =NULL;
      old_tbdf->notnulls=new_tbdf->notnulls;  new_tbdf->notnulls=NULL;
      old_tbdf->triggers=new_tbdf->triggers;  new_tbdf->triggers.drop();
     // delete new_tbdf
      unlock_tabdescr(new_tbdf);
      new_tbdf->free_descr(cdp);
     // deflock_counter, locks, CS, basblock pointer: preserved in the old_tbdf
      tables[tb]=old_tbdf; // must return this, replaced by new_tbdf in create_table_descr
      new_tbdf=old_tbdf;
      LeaveCriticalSection(&old_tbdf->lockbase.cs_locks);
    }
    else
    {// copy some data, must stop ownership of objects copied to new tbdf:
      new_tbdf->lockbase.move_locks(old_tbdf->lockbase);
      new_tbdf->deflock_counter=old_tbdf->deflock_counter;
      tables[tb]=new_tbdf;
      LeaveCriticalSection(&old_tbdf->lockbase.cs_locks);
     // delete old_tbdf
      old_tbdf->free_descr(cdp);
    }
  }
  LeaveCriticalSection(&cs_tables);
  return new_tbdf;
}

#define NBNUMS_ALLOC_STEP 8  // space for additional nbnums in basblock copy

BOOL table_descr::update_basblock_copy(const bas_tbl_block * basbl)
// Synchronizes basblock contents (fixed in basbl) with basblock copy in tbdf.
// Error posible only if nbnums in basblock growed -> then returns FALSE.
{ bas_tbl_block_info * bbl = (bas_tbl_block_info*)corealloc(sizeof(bas_tbl_block_info) + (basbl->nbnums-1) * sizeof(tblocknum), 44);
  if (bbl==NULL) return FALSE;
  bbl->ref_cnt=1;
  bbl->diraddr=basbl->diraddr==wbtrue;  bbl->nbnums=basbl->nbnums;  bbl->recnum=basbl->recnum;
  memcpy(bbl->databl, basbl->databl, basbl->nbnums*sizeof(tblocknum));
  if (basblock) basblock->Release();  // synchronised with get_basblock()
  basblock=bbl;
  return TRUE;
}

BOOL table_descr::prepare(cdp_t cdp, tblocknum basblocknIn, t_translog * translog)
/* Prepares table_descr for normal operation.
On entry: table_descr contains basic description of columns and constrains.
  indicies are compiled (including automatic ones).
On exit:  table descr is ready for use.
Returns FALSE on error. 
*/
{ int atr;  attribdef * att;
 // find special (system) columns:
  rec_privils_atr=zcr_attr=luo_attr=unikey_attr=token_attr=detect_attr=NOATTRIB;
  for (atr=1, att=attrs+atr;  !memcmp(att->name, "_W5_", 4);  atr++, att++)
  { if      (!strcmp(att->name, "_W5_RIGHTS" )) rec_privils_atr=atr;
    else if (!strcmp(att->name, "_W5_ZCR"    )) zcr_attr       =atr;
    else if (!strcmp(att->name, "_W5_LUO"    )) luo_attr       =atr;
    else if (!strcmp(att->name, "_W5_UNIKEY" )) unikey_attr    =atr;
    else if (!strcmp(att->name, "_W5_TOKEN"  )) token_attr     =atr;
    else if (!strcmp(att->name, "_W5_DETECT1")) detect_attr    =atr;
    else if (strcmp(att->name, "_W5_VARINAT") && strcmp(att->name, "_W5_DOCFLOW")) 
      break;
  } 
 // the record is traced iff the 1st columns after the last system column is a tracing column:
  is_traced = (att->attrtype >= ATT_FIRSTSPEC) && (att->attrtype <= ATT_LASTSPEC);
 // set attrspec2=IS_TRACED for columns traced by other columns:
  attrs[0].attrspec2=0;
  for (atr=1, att=attrs+atr;  atr<attrcnt-1;  atr++, att++)
    if (is_traced ||
        att->attrtype!=ATT_AUTOR && att->attrtype!=ATT_DATIM &&
        att[1].attrtype >= ATT_FIRSTSPEC && att[1].attrtype <= ATT_LASTSPEC)
      att->attrspec2=IS_TRACED;
    else att->attrspec2=0;
  att->attrspec2=is_traced ? IS_TRACED : 0;  // the last column is traced only if the whole record is traced
 // mark columns which are in any index:
  dd_index * cc;  int i;
  for (i=0, cc=indxs;  i<indx_cnt;  i++, cc++)
    for (atr=0;  atr < attrcnt;  atr++)
      if (cc->atrmap.has(atr))
        attrs[atr].attrspec2 |= IS_IN_INDEX;
 // create linked references to the following signature column:
  tattrib foll_signat=NOATTRIB;
  for (atr=attrcnt-1, att=attrs+atr;  atr;  atr--, att--)
  { att->signat=foll_signat;
    if (att->attrtype==ATT_SIGNAT) foll_signat=atr;
  }
  att->signat=foll_signat;  // DELETED attribute must have it too!
 // calculate offsets of columns in the record: attrpage, attroffset
  _has_variable_allocation=false;
  unsigned page=0, offset=0, in1stpage;
  for (atr=0, att=attrs;  atr<attrcnt;  atr++, att++)
  { unsigned valsize=typesize(att);
    unsigned off=0;  /* allocation size before attroffset */
    if (att->attrmult != 1)
    { in1stpage=CNTRSZ;
      if (att->attrmult & 0x80) in1stpage+=(off=sizeof(tpiece));
    }
    else in1stpage=valsize;
    if (offset+in1stpage > BLOCKSIZE) { page++; offset=0; }  // put the column into the next page 
    att->attrpage   = page;
    att->attroffset = (uns16)(offset+off);
    offset+=in1stpage;
   // allocate space for multiattribute values:
    if (att->attrmult != 1)
    { unsigned ind=att->attrmult & 0x7f;   // number of preallocated values
      do
      { unsigned loc=(BLOCKSIZE-offset) / valsize;   // values in the current page
        if (ind<=loc) break;
        ind-=loc;
        page++;  offset=0;
      } while (TRUE);
      offset+=valsize*ind;
    }
   // test for variable allocation for the column:
    if (IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype) || att->attrmult & 0x80)
      _has_variable_allocation=true;
  }
 // set the data describing record sizes:
  recordsize=BLOCKSIZE * page + offset;
  if (recordsize < 4) recordsize=offset=4;
  multpage  =(wbbool)(page>0);
  rectopage =page ? (offset ? page+1 : page)  /* pages per record */
                  : (BLOCKSIZE / offset);     /* records per page */
#if WBVERS<=900
  if (page > 255)  // error if record requires more than 255 pages
    { SET_ERR_MSG(cdp, selfname);  request_error(cdp, TABLE_DAMAGED);  return FALSE; }
#endif
 // read data from basblock and create basblock copy:
  { t_versatile_block bas_acc(cdp, translog==NULL || translog->tl_type==TL_CLIENT); // basblcok will not be changed here
    const bas_tbl_block * basbl = (const bas_tbl_block*)bas_acc.access_block(basblocknIn, translog);
    if (basbl==NULL) return FALSE;
    if (basbl->diraddr!=wbfalse && basbl->diraddr!=wbtrue)
      { SET_ERR_MSG(cdp, selfname);  request_error(cdp, TABLE_DAMAGED);  return FALSE; }
    basblockn=basblocknIn;
    if (!update_basblock_copy(basbl))
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
   // load index numbers:
    for (i=0, cc=indxs;  i<indx_cnt;  i++, cc++)
      cc->root=basbl->indroot[i]; // copy root
  }
 // init cache:
  int req_cache_size = 0;
  if (!multpage) 
    req_cache_size=10*rectopage;
  if (req_cache_size>1000) req_cache_size=1000;
  if (req_cache_size<t_free_rec_pers_cache::default_cache_size) 
    req_cache_size=t_free_rec_pers_cache::default_cache_size;
  if (!free_rec_cache.make_cache(req_cache_size))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  return TRUE;
}

void table_descr::mark_core(void)  // marks itself, too
{ unsigned i;  dd_index * cc;  dd_check * ccc;  attribdef * att;
  if (basblock) mark(basblock);  // basblock may be NULL when level==TABLE_DESCR_LEVEL_COLUMNS_INDS
  lockbase.mark_core();
  for (i=0, att=attrs;  i<attrcnt;  i++, att++)
    if (att->defvalexpr && att->defvalexpr!=COUNTER_DEFVAL) att->defvalexpr->mark_core();
  for (i=0, cc=indxs;  i<indx_cnt;  i++, cc++)
    cc->mark_core();
  for (i=0, ccc=checks;  i<check_cnt;  i++, ccc++)
    ccc->mark_core();
  if (indxs)    mark_array(indxs);
  if (checks)   mark_array(checks);
  if (forkeys)  mark_array(forkeys);  // forkey does not have item destructors and the array does not have the counter on its beginning -- no more!
  if (notnulls) mark(notnulls);
  triggers.mark_core();
  free_rec_cache.mark_core();
  if (trigger_scope) { trigger_scope->mark_core();  mark(trigger_scope); }
  if (fulltext_info_list) mark(fulltext_info_list);
  mark(this);
}

void WINAPI domain_comp_link(CIp_t CI)
{ t_type_specif_info * tsi = (t_type_specif_info*)CI->univ_ptr;   table_all ta;
 // skip "[CREATE|DECLARE] DOMAIN [schema.]name", if present:
  if (CI->cursym==SQ_DECLARE || CI->cursym==S_CREATE) next_sym(CI); // for wider compatibility
  if (CI->cursym==SQ_DOMAIN)
  { tobjname name, schema;
    get_schema_and_name(CI, name, schema);
  }
  if (CI->cursym==S_AS) next_sym(CI);  // skips optional "AS"
  analyse_type(CI, tsi->tp, tsi->specif, &ta, 0);
  if (tsi->tp==ATT_PTR || tsi->tp==ATT_BIPTR || tsi->tp>=ATT_FIRSTSPEC && tsi->tp<=ATT_LASTSPEC)
    c_error(THIS_TYPE_NOT_ENABLED);
  if (ta.names.count())
    { strcpy(tsi->schema, ta.names.acc0(0)->schema_name);  strcpy(tsi->domname, ta.names.acc0(0)->name); }
  compile_domain_ext(CI, tsi);
  if (CI->cursym==';') next_sym(CI);
  if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);
}

bool compile_domain_to_type(cdp_t cdp, tobjnum objnum, t_type_specif_info & tsi)
{ do  // processing the recursion
  { tptr defin = ker_load_objdef(cdp, objtab_descr, objnum);
    if (!defin) return false;
    compil_info xCI(cdp, defin, domain_comp_link);  
    xCI.univ_ptr=&tsi;
    if (compile(&xCI)) { corefree(defin);  return false; }
    if (tsi.tp!=ATT_DOMAIN)
      { corefree(defin);  return true; }
   // recurse:
    c_error(THIS_TYPE_NOT_ENABLED);
#ifdef STOP  // Recursive domains not implemented yet. Registering the domain usage would be more complicated.
    if (*tsi.schema)
      reduce_schema_name(cdp, tsi.schema);
    if (name_find2_obj(cdp, tsi.domname, tsi.schema, CATEG_DOMAIN, &objnum))
      { SET_ERR_MSG(cdp, tsi.domname);  request_error(cdp, OBJECT_NOT_FOUND);  return false; }
#endif
  } while (true);
}

table_descr * make_td_from_ta(cdp_t cdp, table_all * pta, ttablenum tbnum)
// Creates the initial table_descr from ta. 
// The table_descr is installed during the compilation of other parts the table_descr (there may be cyclic references to it)
// Updates ta: adds checks and defvals from domains, replaces domain types by the translated types (compilation of constrains needs this)
// [tbnum] is the table number for system tables and differs from a system table numbers for other tables (often is -1).
{ int i;
 // count pointers and notnulls:
  int namecnt=0, notnull_cnt=0;
  for (i=0;  i<pta->attrs.count();  i++)
  { const atr_all * al = pta->attrs.acc0(i);
    if (al->type==ATT_PTR || al->type==ATT_BIPTR) namecnt++;
    if (i && !al->nullable || al->type==ATT_DOMAIN) notnull_cnt++;  // approximation
  }
 // allocate table_descr and notnulls array:
  table_descr * td = new(pta->attrs.count(), namecnt) table_descr;
  if (td==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  if (notnull_cnt)
  { td->notnulls=(tattrib*)corealloc(sizeof(tattrib)*(notnull_cnt+1), 92);
    if (!td->notnulls) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errexit; }
  }
 // move basic data from table_all to table_descr: 
  td->tabdef_flags=pta->tabdef_flags;
  strcpy(td->selfname, pta->selfname);
 // move column info (td->attrcnt is incremented per attribute, used in td destructor):
  tptr nameptr;  nameptr = (tptr)(td->attrs+pta->attrs.count());
  notnull_cnt=0;
  for (i=0;  i<pta->attrs.count();  i++)
  { atr_all * al = pta->attrs.acc0(i);
    td->add_column(al->name, al->type, al->multi, al->nullable, al->specif);  // added into td->attrs[i]
   // move names (later add names to the type-specific information in the column descriptor): 
    if (al->type==ATT_PTR || al->type==ATT_BIPTR) 
    { memcpy(nameptr, pta->names.acc0(i), 2*sizeof(tobjname));
      nameptr+=2*sizeof(tobjname);
    }
    else if (al->type==ATT_DOMAIN)  // translate the domain (type is not pointer, does not affect namecnt)
    { t_type_specif_info tsi;
      if (!compile_domain_to_type(cdp, al->specif.domain_num, tsi))
        goto errexit;
      td->attrs[i].attrtype=al->type=tsi.tp;  td->attrs[i].attrspecif=al->specif=tsi.specif;
      if (!tsi.nullable) td->attrs[i].nullable = al->nullable = wbfalse;  // NOT NULLs from both levels merged
      if (tsi.check!=NULL) 
      { check_constr * check = pta->checks.next();
        if (!check) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errexit; }
        *check->constr_name=0;   check->state=ATR_NEW;
        check->co_cha=tsi.co_cha;  check->column_context=i;
        check->text=tsi.check;   tsi.check=NULL;
      }
      if (tsi.defval!=NULL && al->defval==NULL)   // outer DEFAULT prevails
        { al->defval=tsi.defval;  tsi.defval=NULL; }
    } // domain
    else if (al->type==ATT_HIST)  // must calculate specif again, because of domains in history
    { unsigned info = 0;
      int pos = i;
      while (--pos>0)  // analyse preceeding columns
      { if      (td->attrs[pos].attrtype==ATT_AUTOR) info=4*info+0x10;
        else if (td->attrs[pos].attrtype==ATT_DATIM) info=4*info+0x20;
        else
        { info += (td->attrs[pos].attrtype & 0xf) + (typesize(/*(const d_attr*)*/&td->attrs[pos])<<8); 
          break;
        }
      }
      td->attrs[i].attrspecif.opqval=info;
    } // history
   // add to the notnull list (must be after domain translation):
    if (!td->attrs[i].nullable) td->notnulls[notnull_cnt++]=(tattrib)i;
  }
 // add notnull list terminator:
  if (td->notnulls) td->notnulls[notnull_cnt]=0;  
 // replace system string charsets in the system tables:
  if (SYSTEM_TABLE(tbnum))
    switch (tbnum)
    { case TAB_TABLENUM:  case OBJ_TABLENUM:
        td->attrs[OBJ_NAME_ATR  ].attrspecif.charset=pta->attrs.acc0(OBJ_NAME_ATR  )->specif.charset=
        td->attrs[OBJ_FOLDER_ATR].attrspecif.charset=pta->attrs.acc0(OBJ_FOLDER_ATR)->specif.charset=
        td->attrs[OBJ_DEF_ATR   ].attrspecif.charset=pta->attrs.acc0(OBJ_DEF_ATR   )->specif.charset=
        sys_spec.charset;  break;
      case USER_TABLENUM:
        td->attrs[USER_ATR_LOGNAME].attrspecif.charset=pta->attrs.acc0(USER_ATR_LOGNAME)->specif.charset=
        sys_spec.charset;  break;
      case PROP_TABLENUM:
        td->attrs[PROP_OWNER_ATR].attrspecif.charset=pta->attrs.acc0(PROP_OWNER_ATR)->specif.charset=  
        td->attrs[PROP_NAME_ATR ].attrspecif.charset=pta->attrs.acc0(PROP_NAME_ATR )->specif.charset=
        sys_spec.charset;  break;
      case REL_TABLENUM:
        td->attrs[REL_PAR_NAME_COL].attrspecif.charset=pta->attrs.acc0(REL_PAR_NAME_COL)->specif.charset=  
        td->attrs[REL_CLD_NAME_COL].attrspecif.charset=pta->attrs.acc0(REL_CLD_NAME_COL)->specif.charset=
        sys_spec.charset;  break;
    }

 // allocate constrain arrays, must be after translation domains (automatic indicies have been added before):
  if (pta->indxs.count()) 
  { td->indxs   = new_dd_index(pta->indxs .count(), td->attrcnt);
    if (!td->indxs   ) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errexit; }
  }
  if (pta->checks.count())   // domain translation may have increased this!
  { td->checks  = new_dd_check(pta->checks.count(), td->attrcnt);
    if (!td->checks  ) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errexit; }
  }
  if (pta->refers.count()) 
  { td->forkeys = new_dd_forkey(pta->refers.count(), td->attrcnt);
    if (!td->forkeys ) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errexit; }
  }
 // compile indicies (used by other tables looking for parent index) and local parts of foreign keys:
  for (i=0;  i<pta->indxs.count();  i++)
  { const ind_descr * indx = pta->indxs.acc0(i);
    if (indx->state==INDEX_DEL) continue;  // in ALTER TABLE some indexes are deleted (_W5_ZCR, @)
    dd_index * cc = &td->indxs[td->indx_cnt++];
    int res=compile_index(cdp, indx->text, pta, indx->constr_name, indx->index_type, indx->has_nulls, cc);
    if (res) { request_error(cdp, res);  goto errexit; }
    cc->deferred=indx->co_cha==COCHA_UNSPECIF && (cdp->sqloptions & SQLOPT_CONSTRS_DEFERRED) 
              || indx->co_cha==COCHA_DEF;
    if (!strcmp(indx->constr_name, "_W5_ZCR_KEY"))
      cc->atrmap.set_all();  // all column changes affect it!
  }
 // move foreign references from ta to td:
  for (i=0;  i<pta->refers.count();  i++)
  { const forkey_constr * ref = pta->refers.acc0(i);
    dd_forkey * cc = &td->forkeys[td->forkey_cnt++];
    strcpy(cc->constr_name, ref->constr_name);
    cc->desttabnum   =NOOBJECT;  // not searched yet
    cc->update_rule  =ref->update_rule;
    cc->delete_rule  =ref->delete_rule;
    cc->deferred     =ref->co_cha==COCHA_UNSPECIF && (cdp->sqloptions & SQLOPT_CONSTRS_DEFERRED)
                   || ref->co_cha==COCHA_DEF;
    cc->loc_index_num=ref->loc_index_num;  // loc_index_num found in compile_table_to_all
   // copy attribute map from local index to the RI rule:
    cc->atrmap=td->indxs[cc->loc_index_num].atrmap;
  }
 // OK return:
  return td;
 errexit:
  td->free_descr(cdp);  
  return NULL;
}

tptr ker_load_objdef2(cdp_t cdp, table_descr * tbdf, trecnum recnum);

static table_descr * create_table_descr(cdp_t cdp, ttablenum tb) 
/* Loads & compiles the table definition, inits table_descr fields.
   Loads basblock number. Then calls init_tbdf.
   Used when installing or re-installing a table. */
{ tptr defin;  tblocknum basblockn;  table_all ta;  table_descr * tbdf;
 // get info from TABTAB record (may be installing TABTAB):
 // create tbdf:
  if (tb==TAB_TABLENUM)
  { defin=tabtab_sqldef;
    basblockn=header.tabtabbasbl;
  }
  else // load definition and basblock number from the table record in TABTAB
  { tfcbnum fcbn;  
    const ttrec * def=(const ttrec*)fix_attr_read(cdp, tables[TAB_TABLENUM], tb, DEL_ATTR_NUM, &fcbn);
    if (def==NULL)                                           goto err_leave;
    basblockn = def->basblockn;
    uns8 del = def->del_mark;
    unfix_page(cdp, fcbn);
    if (del) { request_error(cdp, IS_DELETED);               goto err_leave; }
    if (header.berle_used)
      defin=ker_load_objdef2(cdp, tabtab_descr, tb);
    else
      defin=ker_load_objdef(cdp, tabtab_descr, tb);
    if (defin==NULL)                                         goto err_leave;
  }

 // Creates and installs table_descr. It may contain links to itself or cyclic links to other tables.
 // Phases: basic column list created - table preinstalled - constrains compiled using the column list (directly or indirectly via other tables)
 // attroffset, attrpage, attrspec2 will not be defined here.
  BOOL res;
 // create initial td:
 // setup the aplication kontext of the table (used when looking for domains)
  { WBUUID table_schema_uuid;
    if (!tabtab_descr) // unless just creating the tabtab_descr
      memset(table_schema_uuid, 0, UUID_SIZE);  // null uuid for the TABTAB
    else 
      tb_read_atr(cdp, tabtab_descr, tb, APPL_ID_ATR, (tptr)table_schema_uuid);
    t_short_term_schema_context stsc(cdp, table_schema_uuid);
    int i=compile_table_to_all(cdp, defin, &ta);
    if (tb!=TAB_TABLENUM) corefree(defin);
    if (i) { request_error(cdp, i);                            goto err_leave; }
    tbdf = make_td_from_ta(cdp, &ta, tb);
    if (!tbdf)                                                 goto err_leave;
    tbdf->tbnum=tb;  tbdf->deflock_counter=1;
    memcpy(tbdf->schema_uuid, table_schema_uuid, UUID_SIZE);  // used by compile_constrains
  }
 // install initial tbdf:
  tbdf->level=TABLE_DESCR_LEVEL_COLUMNS_INDS; // partially compiled table!!
  EnterCriticalSection(&cs_tables);
  tables[tb]=tbdf;
  LeaveCriticalSection(&cs_tables);
  PulseEvent(hTableDescriptorProgressEvent);

 // compile the other parts of table declaration
 // must not be inside cs_tables, because compile_constrains looks for objects and object index may be locked by a thread waiting form cs_tables ->deadlock!!
  res=compile_constrains(cdp, &ta, tbdf);
  if (res)                                                   goto err;

 // updating the inter-table pointers:
  link_table_in(cdp, tbdf, tb);   /* no error reported if not linked */
 // initialize tbdf:
  if (!tbdf->prepare(cdp, basblockn, NULL)) goto err;
  tbdf->level=MAX_TABLE_DESCR_LEVEL;
  PulseEvent(hTableDescriptorProgressEvent);
  return tbdf;

 err_leave:
  return NULL;
 err:
 // error after initial installation: remove the initial td:
  if (tables[tb]==tbdf) tables[tb]=NULL;
 // destroy td:
  if (tbdf->deflock_counter==1) // nobody is waiting
    { LeaveCriticalSection(&tbdf->lockbase.cs_locks);  tbdf->free_descr(cdp); }
  else // a process waiting for this tbdf in install_table
    { tbdf->deflock_counter--;  LeaveCriticalSection(&tbdf->lockbase.cs_locks);  }
  return NULL;
}

#define INSTALLED_TABLES_LIMIT 2000

table_descr * access_installed_table(cdp_t cdp, ttablenum tb)
{ if (tb<0 || tabtab_descr && tb >= tabtab_descr->Recnum())
    return install_table(cdp, tb);
  { ProtectedByCriticalSection cs(&cs_tables, cdp, WAIT_CS_TABLES);
    table_descr * tbdf=tables[tb];
    if (tbdf==NULL || tbdf==(table_descr*)-1) return NULL;  // not installed
    InterlockedIncrement(&tbdf->deflock_counter);
    return tbdf;
  }
}

table_descr_cond::table_descr_cond(cdp_t cdp, ttablenum tb)
{ tbdf_int = NULL;
  EnterCriticalSection(&cs_tables);
  if (tb!=NOTBNUM && tb >= last_temp_tabnum && tb < tabtab_descr->Recnum() && tables[tb])
  { tbdf_int=tables[tb];
    InterlockedIncrement(&tbdf_int->deflock_counter);
  }
  LeaveCriticalSection(&cs_tables);
}

table_descr * install_table(cdp_t cdp, ttablenum tb, int level)
/* do not use the tabtab definition! */
/* When the table is installed, its existence must not be rolled back !! */
/* Must not install the deleted table, its basblock does not exist */
/* Must create index roots if the do not exist */
{ table_descr * tbdf;
 /* checking to see if the table is installed, locking & returning if it is */
  EnterCriticalSection(&cs_tables);
  if (tb < 0)            /* this is a temporary table: installed or deleted */
  { if (tb < last_temp_tabnum || tables[tb]==NULL)
      { LeaveCriticalSection(&cs_tables);  request_error(cdp, IS_DELETED);  return NULL; }
    tbdf=tables[tb];
    InterlockedIncrement(&tbdf->deflock_counter);
    LeaveCriticalSection(&cs_tables);
    return tbdf;
  }
  if (tabtab_descr && tb >= tabtab_descr->Recnum())  /* table does not exist */
    { LeaveCriticalSection(&cs_tables);  request_error(cdp, IS_DELETED);  return NULL; }

  do
  { tbdf=tables[tb];
    if (tbdf==NULL)   // must create
    { WBUUID table_schema_uuid;
      if (!tabtab_descr) // unless just creating the tabtab_descr
        memset(table_schema_uuid, 0, UUID_SIZE);  // null uuid for the TABTAB
      else 
        tb_read_atr(cdp, tabtab_descr, tb, APPL_ID_ATR, (tptr)table_schema_uuid);
      if (!memcmp(cdp->current_schema_uuid, table_schema_uuid, UUID_SIZE)) break;
     // Must compile module_globals before locking the table access. Otherwise the reference to the same table used in module globals would deadlock it.
      LeaveCriticalSection(&cs_tables);
      { t_short_term_schema_context stsc(cdp, table_schema_uuid);
      }
      //find_global_sscope(cdp, table_schema_uuid); -- does not compile because current_schema_uuid is not set properly
      EnterCriticalSection(&cs_tables);
      if (tables[tb]) continue;
      break;
    }
    if (tbdf==(table_descr*)-1 || level > tbdf->level) // installation of the table in progress, must wait
      if (cdp->exclusively_accessed_table && cdp->exclusively_accessed_table->tbnum==tb) // ... unless I am working on it
      { LeaveCriticalSection(&cs_tables);
        tbdf=cdp->exclusively_accessed_table;
        InterlockedIncrement(&tbdf->deflock_counter);
        return tbdf;
      }
      else
      { LeaveCriticalSection(&cs_tables);
        WaitLocalManualEvent(&hTableDescriptorProgressEvent, INFINITE);
        EnterCriticalSection(&cs_tables);
      }
    else // tbdf is OK, return it
    { LeaveCriticalSection(&cs_tables);
      InterlockedIncrement(&tbdf->deflock_counter);
      return tbdf;
    }
  } while (true);

 // must create the descriptor:
  tables[tb]=(table_descr*)-1;
  LeaveCriticalSection(&cs_tables);
  tbdf=create_table_descr(cdp, tb);
  if (tbdf != NULL)  // this exits cs_tables
  { if (installed_tables>INSTALLED_TABLES_LIMIT) 
      uninst_tables(cdp);
    installed_tables++;
  }
  else  // failed!
  { ProtectedByCriticalSection cs(&cs_tables, cdp, WAIT_CS_TABLES);
    tables[tb]=NULL;
    PulseEvent(hTableDescriptorProgressEvent);
  }
  return tbdf;
}

void WINAPI unlock_tabdescr(ttablenum tb)
{ if (tb < last_temp_tabnum || tables[tb]==NULL ||  tables[tb]==(table_descr*)-1) return;
  // tables[tb] is NULL when force_uninst_table destructs triggers of the table
  if (InterlockedDecrement(&tables[tb]->deflock_counter) < 0) tables[tb]->deflock_counter=0;
}

void WINAPI unlock_tabdescr(table_descr * tbdf)
{ if (InterlockedDecrement(&tbdf->deflock_counter) < 0) tbdf->deflock_counter=0;
  if (tbdf->released && !tbdf->deflock_counter)
    delete tbdf;  // delayed destruction
}

void force_uninst_table(cdp_t cdp, ttablenum tb)
/* Called when: table is deleted (normal & temp), table creation is rolled back. */
{ EnterCriticalSection(&cs_tables);
  table_descr * tbdf=tables[tb];
  if (tbdf!=NULL && tbdf!=(table_descr*)-1)
  { tables[tb]=NULL;
    tbdf->unique_value_cache.close_cache(cdp);  // must be called before free_descr which deletes cs_locks
    tbdf->free_descr(cdp);
    installed_tables--;
  }
  LeaveCriticalSection(&cs_tables);
}

void try_uninst_table(cdp_t cdp, ttablenum tb)
/* Must be (and is) called only for tb>=0! */
{ if (tb >= tabtab_descr->Recnum()) return;  /* table not installed yet */
  EnterCriticalSection(&cs_tables);
  table_descr * tbdf = tables[tb];
  if (can_uninstall(cdp, tbdf)) force_uninst_table(cdp, tb);
  LeaveCriticalSection(&cs_tables);
}

void uninst_tables(cdp_t cdp)
/* Called when uninst_all_tables_flag set when exiting kernel */
{ ProtectedByCriticalSection cs(&cs_tables, cdp, WAIT_CS_TABLES);
  for (ttablenum tb=OBJ_TABLENUM+1;  tb<tabtab_descr->Recnum();  tb++) 
    if (tables[tb]!=NULL && tables[tb]!=(table_descr*)-1)
      try_uninst_table(cdp, tb);
}

void tables_stat(uns16 * installed, uns16 * locked, uns16 * locks, uns16 * temp_tables)
{ ttablenum tb;
  *installed=*locked=*locks=*temp_tables=0;
  EnterCriticalSection(&cs_tables);
  for (tb=0;  tb<tabtab_descr->Recnum();  tb++) if (tables[tb]!=NULL && tables[tb]!=(table_descr*)-1)
  { (*installed)++;
    if (tables[tb]->deflock_counter)
    { (*locked)++;
      *locks+=(uns16)tables[tb]->deflock_counter;
    }
  }
  for (tb=-1;  tb>=last_temp_tabnum;  tb--)
    if (tables[tb]!=NULL && tables[tb]!=(table_descr*)-1) (*temp_tables)++;
  LeaveCriticalSection(&cs_tables);
}

BOOL init_tables_1(cdp_t cdp) // returns TRUE on error
{// determine number of all tables and init TABTAB indices, if missing:
  { t_independent_block bas_acc(cdp);
    bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(header.tabtabbasbl);
    if (basbl==NULL) return TRUE;
    tables_last=(int)basbl->recnum-1;
    if (!basbl->indroot[0]) // must create indicies
    { for (int i = 0;  i<TABTAB_INDEX_COUNT;  i++)
        if (create_empty_index(cdp, basbl->indroot[i], &cdp->tl)) return TRUE;
      bas_acc.data_changed();
    }
  }
 // initialize tables[]:
  if (!(tables=(table_descr**)corealloc((tables_last+3) * sizeof(void *), OW_TABLE)))
    return TRUE;
  memset(tables, 0, sizeof(void*) * (tables_last+1));
  last_temp_tabnum = 0;  prev_tables=NULL;
 // install TABTAB:
  tabtab_descr = install_table(cdp, TAB_TABLENUM);  /* basblocks of these tables become fixed */
  if (tabtab_descr==NULL) return TRUE;
  if (tabtab_descr->Recnum() < tabtab_descr->rectopage)  
  // 1st TABTAB page may not be init properly, mark unused records as empty now!
  { tfcbnum fcbn;
    tptr dt = fix_priv_page(cdp, tabtab_descr->basblock->databl[0], &fcbn, &cdp->tl);
    for (int i=tabtab_descr->Recnum();  i<tabtab_descr->rectopage;  i++)
      dt[i*tabtab_descr->recordsize]=RECORD_EMPTY;
    cdp->tl.page_changed(fcbn);
    unfix_page(cdp, fcbn);
    commit(cdp);
  }
  return FALSE;
}

void conv_pass(char * p);

static char sysext[]="_SYSEXT";
WBUUID sysext_schema_uuid = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };  // must avoid conflict with non-schema uuid when cresting system tables!

void create_system_triggers(cdp_t cdp)
{ tobjnum aplobj;
  if (exec_direct(cdp, "CREATE SCHEMA _SYSEXT")) return;
  ker_set_application(cdp, sysext, aplobj);
  memcpy(sysext_schema_uuid, cdp->top_appl_uuid, UUID_SIZE);
 // make the CONF ADMIN the Administrator and Author:
  uns8 state = wbtrue;
  user_and_group(cdp, CONF_ADM_GROUP, cdp->admin_role,  CATEG_ROLE, OPER_SET, &state); 
  user_and_group(cdp, CONF_ADM_GROUP, cdp->author_role, CATEG_ROLE, OPER_SET, &state); 
 // creating the triggers:
  exec_direct(cdp, "CREATE PROCEDURE _ON_SERVER_START();\r\nBEGIN\r\nEND\r\n");
  exec_direct(cdp, "CREATE PROCEDURE _ON_SERVER_STOP();\r\nBEGIN\r\nEND\r\n");
  exec_direct(cdp, "CREATE PROCEDURE _ON_BACKUP(IN pathname CHAR(254), IN success Boolean);\r\nBEGIN\r\nEND\r\n");
  if (!exec_direct(cdp, "CREATE PROCEDURE _ON_LOGIN_CHANGE(IN old_logname CHAR(31), IN new_logname CHAR(31));\r\nBEGIN\r\nEND\r\n") && cdp->sql_result_count())
  { tobjnum procobj = find2_object(cdp, CATEG_PROC, NULL, "_ON_LOGIN_CHANGE");
    if (procobj != NOOBJECT)
    { privils prvs(cdp);  t_privils_flex priv_val(objtab_descr->attrcnt);
      prvs.set_user(EVERYBODY_GROUP, CATEG_GROUP, FALSE);
      prvs.get_own_privils(objtab_descr, procobj, priv_val);
      priv_val.add_read(OBJ_DEF_ATR);  // EVERYBODY can run tris trigger
      prvs.set_own_privils(objtab_descr, procobj, priv_val);
    }
  }
  exec_direct(cdp, "CREATE FUNCTION _ON_SYSTEM_EVENT(IN event_num Int, IN ipar Int, IN spar Char(255)) RETURNS Int;\r\nBEGIN\r\n  RETURN 0;\r\nEND\r\n");
 // remove DB_ADMIN (the creator) from the standard roles in _SYSEXT:
  state = wbfalse;
  user_and_group(cdp, DB_ADMIN_GROUP, cdp->admin_role,  CATEG_ROLE, OPER_SET, &state); 
  user_and_group(cdp, DB_ADMIN_GROUP, cdp->author_role, CATEG_ROLE, OPER_SET, &state); 
 // commit 
  ker_set_application(cdp, "", aplobj);
  commit(cdp);
}

static void set_main_read_privils_for_objects(t_privils_flex & priv_val)
{ priv_val.clear();
  priv_val.add_read(OBJ_NAME_ATR);  // EVERYBODY can read public attrs
  priv_val.add_read(OBJ_CATEG_ATR);
  priv_val.add_read(OBJ_FLAGS_ATR);
  priv_val.add_read(APPL_ID_ATR);
  priv_val.add_read(OBJ_FOLDER_ATR);
  priv_val.add_read(OBJ_MODIF_ATR);
  priv_val.add_read(1);  // ZCR
  priv_val.add_read(2);  // privils
}

BOOL init_tables_2(cdp_t cdp)
// Install or create other system tables.
{ cdp->prvs.is_data_admin=cdp->prvs.is_secur_admin=cdp->prvs.is_conf_admin=TRUE;  // preliminary
  if (!tables_last)
  { table_descr * srvtab_descr, * repltab_descr, * keytab_descr;
   // put TABTAB into its own index:
    { t_simple_update_context sic(cdp, tabtab_descr, 0);
      if (sic.lock_roots())
        sic.inditem_add(TAB_TABLENUM);  // locking the index is not necessary here
    }
   // create system tables:
    if (sql_exec_direct(cdp,  objtab_sqldef)) return TRUE;
    objtab_descr = install_table(cdp, OBJ_TABLENUM);
    if (sql_exec_direct(cdp, usertab_sqldef)) return TRUE;
    usertab_descr= install_table(cdp, USER_TABLENUM);
    if (objtab_descr==NULL || usertab_descr==NULL) return TRUE;
    if (sql_exec_direct(cdp,  srvtab_sqldef)) return TRUE;
    srvtab_descr = install_table(cdp, SRV_TABLENUM);
    if (!srvtab_descr) return TRUE;
    if (sql_exec_direct(cdp, repltab_sqldef)) return TRUE;
    repltab_descr= install_table(cdp, REPL_TABLENUM);
    if (!repltab_descr) return TRUE;
    if (sql_exec_direct(cdp,  keytab_sqldef)) return TRUE;
    keytab_descr = install_table(cdp, KEY_TABLENUM);
    if (!keytab_descr) return TRUE;
   // create property table:
    if (sql_exec_direct(cdp,  proptab_sqldef)) return TRUE;
   // create relation table:
    if (sql_exec_direct(cdp,  reltab_sqldef)) return TRUE;
   // add standard users:
    if (new_user_or_group(cdp, "DB_ADMIN",       CATEG_GROUP, data_adm_uuid) ==NORECNUM) return TRUE;
    cdp->prvs.set_user(DB_ADMIN_GROUP, CATEG_USER, FALSE);  // continue with ADMIN privils
    cdp->prvs.is_data_admin=cdp->prvs.is_secur_admin=cdp->prvs.is_conf_admin=TRUE;  // preliminary 2
    trecnum rec=new_user_or_group(cdp, "ANONYMOUS", CATEG_USER, anonymous_uuid);
    if (rec==NORECNUM) return TRUE;
    wbbool certif=wbfalse;
    tb_write_atr(cdp, usertab_descr, rec, USER_ATR_CERTIF, &certif);
    if (new_user_or_group(cdp, "EVERYBODY",      CATEG_GROUP, everybody_uuid)==NORECNUM) return TRUE;
    if (new_user_or_group(cdp, "CONFIG_ADMIN",   CATEG_GROUP, conf_adm_uuid) ==NORECNUM) return TRUE;
    if (new_user_or_group(cdp, "SECURITY_ADMIN", CATEG_GROUP, secur_adm_uuid)==NORECNUM) return TRUE;

   // add ANONYMOUS to the DB_ADMIN, CONFIG_ADMIN, SECURITY_ADMIN and EVERYBODY groups:
    uns8 state_true = wbtrue;
    user_and_group(cdp, ANONYMOUS_USER, EVERYBODY_GROUP, CATEG_GROUP, OPER_SET, &state_true); // everybody group must have been created
    user_and_group(cdp, ANONYMOUS_USER, DB_ADMIN_GROUP,  CATEG_GROUP, OPER_SET, &state_true); // everybody group must have been created
    user_and_group(cdp, ANONYMOUS_USER, CONF_ADM_GROUP,  CATEG_GROUP, OPER_SET, &state_true); // everybody group must have been created
    user_and_group(cdp, ANONYMOUS_USER, SECUR_ADM_GROUP, CATEG_GROUP, OPER_SET, &state_true); // everybody group must have been created
   // set initial privileges for EVERYBODY_GROUP:
    privils prvs(cdp);  t_privils_flex priv_val;  // will be initialised for every table
    prvs.set_user(EVERYBODY_GROUP, CATEG_GROUP, FALSE);
    // TABTAB:
    priv_val.init(tabtab_descr->attrcnt);
    set_main_read_privils_for_objects(priv_val);
    *priv_val.the_map() = RIGHT_NEW_READ | RIGHT_INSERT; // must have INSERT privilege, clients test it and use it when creating links
    priv_val.add_read(TAB_DRIGHT_ATR);
    prvs.set_own_privils(tabtab_descr,  NORECNUM, priv_val);
    // OBJTAB:
    priv_val.init(objtab_descr->attrcnt);
    set_main_read_privils_for_objects(priv_val);
    *priv_val.the_map() = RIGHT_NEW_READ | RIGHT_INSERT; // must have INSERT privilege, clients test it and use it when creating links
    prvs.set_own_privils(objtab_descr,  NORECNUM, priv_val);
    // USERTAB:
    priv_val.init(usertab_descr->attrcnt);
    priv_val.set_all_r();
    *priv_val.the_map() = RIGHT_READ;
    prvs.set_own_privils(usertab_descr, NORECNUM, priv_val);  // cannot create users by default
    // KEYTAB:
    priv_val.init(keytab_descr->attrcnt);
    priv_val.set_all_r();
    *priv_val.the_map() = RIGHT_READ | RIGHT_INSERT;          // can read and insert new certificates
    priv_val.add_write(KEY_ATR_CREDATE);   // this info is provided after certification
    priv_val.add_write(KEY_ATR_EXPIRES);   // this info is provided after certification
    priv_val.add_write(KEY_ATR_CERTIFS);   // the certificate can be written
    priv_val.add_write(KEY_ATR_CERTIF);    // this info is provided after certification
    priv_val.add_write(KEY_ATR_PUBKEYVAL); // the state can be changed
    prvs.set_own_privils(keytab_descr,  NORECNUM, priv_val);
    // SRVTAB:
    priv_val.init(srvtab_descr->attrcnt);
    priv_val.set_all_r();
    *priv_val.the_map() = RIGHT_READ;
    prvs.set_own_privils(srvtab_descr,  NORECNUM, priv_val);
    // REPLTAB:
    priv_val.init(repltab_descr->attrcnt);
    priv_val.set_all_rw();  // can do everything (temp.)
    prvs.set_own_privils(repltab_descr, NORECNUM, priv_val);
   // privils for ANONYMOUS user (which may not be in the EVERYBODY group):
   // without INSERT privils
    prvs.set_user(ANONYMOUS_USER, CATEG_USER, FALSE);
    // TABTAB:
    priv_val.init(tabtab_descr->attrcnt);
    set_main_read_privils_for_objects(priv_val);
    priv_val.add_read(TAB_DRIGHT_ATR);
    prvs.set_own_privils(tabtab_descr,  NORECNUM, priv_val);
    // OBJTAB:
    priv_val.init(objtab_descr->attrcnt);
    set_main_read_privils_for_objects(priv_val);
    prvs.set_own_privils(objtab_descr,  NORECNUM, priv_val);
    // USERTAB:
    priv_val.init(usertab_descr->attrcnt);
    priv_val.set_all_r();
    *priv_val.the_map() = RIGHT_READ;
    prvs.set_own_privils(usertab_descr, NORECNUM, priv_val);  // cannot create users by default
    // KEYTAB:
    priv_val.init(keytab_descr->attrcnt);
    priv_val.set_all_r();
    *priv_val.the_map() = RIGHT_READ;          // can read and insert new certificates
    prvs.set_own_privils(keytab_descr,  NORECNUM, priv_val);
    // SRVTAB:
    priv_val.init(srvtab_descr->attrcnt);
    priv_val.set_all_r();
    *priv_val.the_map() = RIGHT_READ;
    prvs.set_own_privils(srvtab_descr,  NORECNUM, priv_val);
   // privils for CONFIG_ADMIN: grant create objects, create user, disable/enable account, drop any user
    prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, FALSE);
    // TABTAB:
    priv_val.init(tabtab_descr->attrcnt);
    *priv_val.the_map() = RIGHT_INSERT | RIGHT_GRANT;
    prvs.set_own_privils(tabtab_descr,  NORECNUM, priv_val);
    // OBJTAB:
    priv_val.init(objtab_descr->attrcnt);
    *priv_val.the_map() = RIGHT_INSERT | RIGHT_GRANT;
    prvs.set_own_privils(objtab_descr,  NORECNUM, priv_val);
    // USERTAB:
    priv_val.init(usertab_descr->attrcnt);
    priv_val.set_all_r();
    priv_val.add_write(USER_ATR_STOPPED);
    *priv_val.the_map() = RIGHT_INSERT | RIGHT_DEL | RIGHT_GRANT;
    prvs.set_own_privils(usertab_descr, NORECNUM, priv_val);
   // ... and configure local and remote server info:
    priv_val.init(srvtab_descr->attrcnt);
    priv_val.set_all_rw();
    *priv_val.the_map() = RIGHT_INSERT | RIGHT_DEL | RIGHT_GRANT | RIGHT_READ | RIGHT_WRITE;
    prvs.set_own_privils(srvtab_descr,  NORECNUM, priv_val);
   // privils for SECUR_ADMIN: create user, disable/enable account, drop any user
    priv_val.init(usertab_descr->attrcnt);
    prvs.set_user(SECUR_ADM_GROUP, CATEG_GROUP, FALSE);
    priv_val.clear();
    priv_val.add_write(USER_ATR_LOGNAME);
    priv_val.add_write(USER_ATR_INFO);
    priv_val.add_write(USER_ATR_STOPPED);
    *priv_val.the_map() = RIGHT_INSERT | RIGHT_DEL | RIGHT_GRANT;
    prvs.set_own_privils(usertab_descr, NORECNUM, priv_val);
   // add own server info:
    rec=tb_new(cdp, srvtab_descr, FALSE, NULL);
    if (rec==NORECNUM) return TRUE;
    tb_write_atr(cdp, srvtab_descr, rec, SRV_ATR_NAME, server_name); // server name
    tb_write_atr(cdp, srvtab_descr, rec, SRV_ATR_UUID, header.server_uuid); // UUID
    wbbool ackn = wbfalse;  
    tb_write_atr(cdp, srvtab_descr, rec, SRV_ATR_ACKN, &ackn); // replication disabled
    t_my_server_info lsrv;
    memset(&lsrv, 0, sizeof(t_my_server_info));
    strcpy(lsrv.input_dir,  ffa.last_fil_path());  strcat(lsrv.input_dir,  "InQueue");
    strcpy(lsrv.output_dir, ffa.last_fil_path());  strcat(lsrv.output_dir, "OutQueue");
    lsrv.input_threads=1;
    lsrv.scanning_period=2;
    tb_write_var(cdp, srvtab_descr, rec, SRV_ATR_APPLS, 0, sizeof(lsrv), &lsrv);
    char addr[254+1];
    *addr='>';  strcpy(addr+1, lsrv.input_dir);
    tb_write_atr(cdp, srvtab_descr, rec, SRV_ATR_ADDR1, addr);
    create_system_triggers(cdp);
   // unlock created tables:
    unlock_tabdescr(srvtab_descr);
    unlock_tabdescr(keytab_descr);
    unlock_tabdescr(repltab_descr);
  }
  else
  { objtab_descr = install_table(cdp,  OBJ_TABLENUM);
    usertab_descr= install_table(cdp, USER_TABLENUM);
    if (objtab_descr==NULL || usertab_descr==NULL) return TRUE;
   // synchronize server uuid - header contains the original, but it may have been changed
   // (for compatibility with older servers, when the SrvUUID property is used then both copies of UUID are the same)
    { table_descr_auto srvtab_descr(cdp, SRV_TABLENUM);
      if (!srvtab_descr->me()) return TRUE;
      tb_read_atr(cdp, srvtab_descr->me(), 0, SRV_ATR_UUID, (tptr)header.server_uuid); // UUID
    }
   // read sysext_schema_uuid:
    ker_apl_name2id(cdp, sysext, sysext_schema_uuid, NULL);
  }
  commit(cdp);
  return FALSE;
}

void deinit_tables(cdp_t cdp)
{ if (tables!=NULL)
  { ttablenum tb;
    for (tb=OBJ_TABLENUM+1;  tb<tabtab_descr->Recnum();  tb++) if (tables[tb]!=NULL && tables[tb]!=(table_descr*)-1)
      force_uninst_table(cdp, tb);
    if (OBJ_TABLENUM<tabtab_descr->Recnum()) force_uninst_table(cdp, OBJ_TABLENUM);
    if (TAB_TABLENUM<tabtab_descr->Recnum()) force_uninst_table(cdp, TAB_TABLENUM);
    corefree(prev_tables);  corefree(tables+last_temp_tabnum);
    prev_tables=NULL;   tables=NULL;  last_temp_tabnum=0;  // prevents passing temp tables in commit_transaction_objects_post
  }
}

void mark_installed_tables(void)
{ ProtectedByCriticalSection cs(&cs_tables);
  for (ttablenum tab=last_temp_tabnum;  tab < (ttablenum)tabtab_descr->Recnum();  tab++)
    if (tables[tab] != NULL && tables[tab]!=(table_descr*)-1)
      tables[tab]->mark_core();
  if (prev_tables) mark(prev_tables);  mark(tables+last_temp_tabnum);
}

void bas_tbl_block::init(void)
{ memset(this, 0, BLOCKSIZE);  /* inits index roots & table block numbers */
  diraddr=wbtrue;  free_rec_list=(tblocknum)-1;  
  // nbnums=0;  recnum=0;  unique_counter=0;  table_open=wbfalse; -- done by memset
}

tblocknum create_table_data(cdp_t cdp, unsigned index_count, t_translog * translog)
/* Called for ordinary and temporary tables. Creates basblock & indicies. 
   Returns basblock dadr or 0 on error. */
{ t_versatile_block bas_acc(cdp, !translog || translog->tl_type==TL_CLIENT);  
 // create basblock:
  tblocknum dadr;
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_new_block(dadr, translog);
  basbl->init();
 // create index roots (temp. tables need nontrans init. of the index root because they continue to exist after their creation is rolled back):
  for (unsigned i=0;  i<index_count;  i++)
    if (create_empty_index(cdp, basbl->indroot[i], translog)) return 0;
  bas_acc.data_changed(translog);  // basblock will be saved in destructor
  return dadr;
}

ttablenum create_temporary_table(cdp_t cdp, table_descr * tbdf, t_translog * translog)
/* Creates and installs a temporary table, returns its number (0 on error) */
{ tbdf->tabdef_flags=TFL_NO_JOUR;
 // must disable the special attributes found in allocate_attribs:
  tblocknum basblockn=create_table_data(cdp, tbdf->indx_cnt, translog); // indicies may exist when altering a table
  if (!basblockn) return 0;
  if (!tbdf->prepare(cdp, basblockn, translog)) return 0;
 // table exists, allocate a table number and install it: 
  EnterCriticalSection(&cs_tables);
  ttablenum tabnum=0;
  for (int i=-1;  i>=last_temp_tabnum;  i--)
    if (tables[i]==NULL) { tabnum=i;  break; }
  if (!tabnum)
  { tabnum=last_temp_tabnum-1;
    if (realloc_tables(cdp, tabnum))
      { LeaveCriticalSection(&cs_tables);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return 0; }
  }
  tbdf->tbnum=tabnum;
  tbdf->deflock_counter=1;
  tbdf->owning_cdp=cdp;
  tables[tabnum]=tbdf;    /* must not assign before everything completed! */
  installed_tables++;
  LeaveCriticalSection(&cs_tables);
  return tabnum;
}

void t_translog_main::drop_iuad_by_root(tblocknum root)
{
  t_index_uad ** piuad = &index_uad;
  while (*piuad && (*piuad)->root!=root)
    piuad = &(*piuad)->next;
  if (*piuad)
  { t_index_uad * iuad = *piuad;
    *piuad = iuad->next;
    delete iuad;
  }
}

void unregister_index_changes_on_table(cdp_t cdp, table_descr * tbdf)
{ dd_index * cc;   int indnum;
  for (indnum=0, cc=tbdf->indxs;  indnum<tbdf->indx_cnt;  indnum++, cc++)
    cdp->tl.drop_iuad_by_root(cc->root);
}

void destroy_temporary_table(cdp_t cdp, ttablenum tabnum)
// For temporary tables used in ALTER TABLE the translog is NULL!
{ if (tabnum < last_temp_tabnum) return;    /* fuse */
 // uninstalling:
  table_descr * tbdf = tables[tabnum];
  if (tbdf==NULL) return;                   /* fuse */
  t_translog * translog = tbdf->translog;
  tables[tabnum]=NULL;
  installed_tables--;
 // remove records from index log:
  unregister_index_changes_on_table(cdp, tbdf);  // called before destroy_indexes() which may overwrite roots, destroy_indexes() does not write to the index log.
 // releasing data and descr:
  release_variable_data(cdp, tbdf);
  destroy_indexes(cdp, tbdf);
  drop_basic_table_allocation(cdp, tbdf->basblockn, tbdf->multpage, tbdf->rectopage, 0, translog);
  free_alpha_block(cdp, tbdf->basblockn, translog);
  tbdf->free_descr(cdp);
 // transaction logs:
  if (translog && translog->tl_type==TL_LOCTAB)
  { destroy_subtrans(cdp, translog);  // releasing pages and pieces, removing "private" status nad changed contents
    delete translog;
  }
  else if (translog) // table belongs to a cursor, the translog continues to exist
  { translog->unregister_changes_on_table(tabnum);  // prevents checking deferred constrains (should be empty because the table does not have deferred constrains)
    translog->log.table_deleted(tabnum);  // removes references to the records of the deleted table from the transaction log (empty, this data is not stored for cursor tables)
    // when pages and pieces were released, they were removed from "changes" log
  }
  else  // temp. table from ALTER TABLE
    cdp->tl.unregister_changes_on_table(tabnum);
}

void destroy_subtrans(cdp_t cdp, t_translog * translog)
// Closes the subtransaction [translog] but does not call delete on it
{// remove [translog] from the list of subtransactions:
  t_translog ** subtl = &cdp->subtrans;
  while (*subtl && (*subtl)!=translog)
    subtl=&(*subtl)->next; 
  *subtl = translog->next;
 // destroy the subtransaction: commit releasing data, remove changes etc.
  translog->destroying_subtrans(cdp);
}

bool same_index(int oind, int nind, table_all * ta_old, table_all * ta_new, table_descr * tbdf_old, table_descr * tbdf_new) 
// Compares 2 indexes in the original and updated table
{
  dd_index * oindx = &tbdf_old->indxs[oind], * nindx = &tbdf_new->indxs[nind];
  if (oindx->partnum!=nindx->partnum || oindx->keysize!=nindx->keysize)  // second condition just makes the comparison faster
    return false;
  int p=0;
  while (p<oindx->partnum)
  { if (oindx->parts[p].expr->sqe==SQE_ATTR)
    { if (nindx->parts[p].expr->sqe!=SQE_ATTR) return false;
      t_expr_attr * oatex = (t_expr_attr*)oindx->parts[p].expr;
      t_expr_attr * natex = (t_expr_attr*)nindx->parts[p].expr;
      if (oatex->type != natex->type || oatex->specif.opqval!=natex->specif.opqval)
        return false;
      atr_all * oatt=ta_old->attrs.acc0(oatex->elemnum);
      atr_all * natt=ta_new->attrs.acc0(natex->elemnum);
      if (natt->alt_value==NULL)  // moving value from the column with the same name
      { if (strcmp(oatt->name, natt->name))
          return false;
      }
      else // source of the value is specified
        if (strcmp(oatt->name, natt->alt_value))  // different, check for quoting
        { char qname[2+ATTRNAMELEN+1];
          sprintf(qname, "`%s`", oatt->name);
          if (strcmp(qname, natt->alt_value))
            return false;
        }
    }
    else   // if (!same_expr(oindx->parts[p].expr, nindx->parts[p].expr)) -- simplification: expressions are considered to be different (otherwise a copy of same_expr must be written)
      return false;
    p++;
  }  
  return true;
}

void change_indicies(cdp_t cdp, table_descr * tbdf, const table_all * ta, int new_index_cnt, const ind_descr * new_indxs)
// Called with the old tbdf (keysizes taken from it)
// ta can be old or new, used to compile new indicies only.
// Only for permanent tables (create_btree needs this)
// tbdf is supposed to be removed from tables[].
// new_index_cnt is the size of new_indxs array, not the number of new indicies.
// new_indxs[i].state is the original number of the i-th index, or INDX_NEW if the i-th index is new, or INDEX_DEL.
// Updating indices is transactional. If succeeds, basblock is updated too. Memory has not to be updated because tbdf is destructed here.
{ int i, new_index_num, old_index_num;  
 // read the original index roots:
  tblocknum old_roots[INDX_MAX], new_roots[INDX_MAX];
  // using table CS should not be necessary, table has exclusive access
  t_independent_block bas_acc(cdp); // must not save changes before successfull commit!
  bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tbdf->basblockn);
  if (basbl==NULL) return;
  memcpy(old_roots, basbl->indroot, INDX_MAX*sizeof(tblocknum));
  memset(new_roots, 0,              INDX_MAX*sizeof(tblocknum));  // new_index_cnt would be sufficient but I prefer zero-initializing of unused index roots
 // make the new index roots: either take the old root or create a new btree:
  for (i=new_index_num=0;  i<new_index_cnt;  i++)
    if (new_indxs[i].state==INDEX_NEW || // creating a new index
        new_indxs[i].state!=INDEX_DEL && old_roots[new_indxs[i].state]==0)  // referencing an old index, but it has been used before
    { dd_index_sep indx;  // atrmap.init() is in compile_index()
      int res=compile_index(cdp, new_indxs[i].text, ta, NULLSTRING, new_indxs[i].index_type, new_indxs[i].has_nulls, &indx);
      if (res) { request_error(cdp, res);  return; }
      if (!(new_roots[new_index_num++]=create_btree(cdp, tbdf, &indx)))
        return;
    }
    else if (new_indxs[i].state!=INDEX_DEL) // taking an old index
    { new_roots[new_index_num++]=old_roots[new_indxs[i].state];
      old_roots[new_indxs[i].state]=0;  // marks the old index as "used", will not be reused nor deleted
    }
 // deleting the old unused indicies 
  for (i=old_index_num=0;  i<tbdf->indx_cnt;  i++)
  { if (old_roots[old_index_num])  // unless the index is used in the new version
      drop_index(cdp, old_roots[old_index_num], tbdf->indxs[i].keysize+sizeof(tblocknum), &cdp->tl);
    old_index_num++;
  }
 // up to this point everything was transactional, should commit before the non-tranns save:
  if (!cdp->is_an_error() && !commit(cdp)) 
  { // saving the new index roots: 
    memcpy(basbl->indroot, new_roots, INDX_MAX*sizeof(tblocknum)); // roots of non-existing indicies are 0
    bas_acc.data_changed();
  }
}

bool transform_indexes(cdp_t cdp, table_descr * tbdf_old, table_descr * tbdf_new, table_all * ta_old, table_all * ta_new)
// Moves identical indexes from ta_old to ta_new, deletes unused indexes in ta_old and creates new indexes in ta_new.
// Updates basblocks
{ int i, j, new_index_num;  
  tblocknum old_roots[INDX_MAX], new_roots[INDX_MAX];
  memset(new_roots, 0, INDX_MAX*sizeof(tblocknum));  // zero-init all possible roots for stability
 // read old index roots from basblock: 
  { t_independent_block bas_acc(cdp); // must not save changes before successfull commit!
    bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(tbdf_old->basblockn);
    if (basbl==NULL) return false;
    memcpy(old_roots, basbl->indroot, INDX_MAX*sizeof(tblocknum));
  }  
 // make the new index roots: either take the old root or create a new btree:
  for (i=new_index_num=0;  i<ta_new->indxs.count();  i++)
  { ind_descr * nind = ta_new->indxs.acc0(i);
    if (nind->state==INDEX_DEL) continue;  // new_index_num not incremented!
   // find identical old index, get or create index:
    int same_old_index=-1;
    for (j=0;  j<ta_old->indxs.count();  j++)
      if (same_index(j, new_index_num, ta_old, ta_new, tbdf_old, tbdf_new))
        { same_old_index=j;  break; }
    if (same_old_index==-1 || old_roots[same_old_index]==0)
    { new_roots[new_index_num]=create_btree(cdp, tbdf_new, &tbdf_new->indxs[new_index_num]);
      if (!new_roots[new_index_num]) return false;
    }
    else  // taking an old index
    { new_roots[new_index_num]=old_roots[same_old_index];
      old_roots[same_old_index]=0;  // marks the old index as "used", will not be reused nor deleted
      tbdf_old->indxs[same_old_index].root=0;  // prevents dropping the index when destroying the old table
    }
    tbdf_new->indxs[new_index_num].disabled=false;  // enabling new index
    new_index_num++;  
  }  
 // up to this point everything was transactional, should commit before the non-trans save:
  if (cdp->is_an_error() || commit(cdp)) 
    return false;  // rollback
 //////////////////////////////////// turning point, no rollback possible ///////////////////////////////   
 // drop the original new indexes (not used, but the root exists), replace by old or created ones:
  for (i=new_index_num=0;  i<ta_new->indxs.count();  i++)
  { ind_descr * nind = ta_new->indxs.acc0(i);
    if (nind->state==INDEX_DEL) continue;  // new_index_num not incremented!
    if (tbdf_new->indxs[new_index_num].root)  // unless the index is used in the new version
      drop_index(cdp, tbdf_new->indxs[new_index_num].root, tbdf_new->indxs[new_index_num].keysize+sizeof(tblocknum), &cdp->tl);
    tbdf_new->indxs[new_index_num].root=new_roots[new_index_num];
    new_index_num++;  
  }  
 // deleting the old unused indicies 
  for (i=0;  i<tbdf_old->indx_cnt;  i++)
  { if (old_roots[i])  // unless the index is used in the new version
      drop_index(cdp, old_roots[i], tbdf_old->indxs[i].keysize+sizeof(tblocknum), &cdp->tl);
    tbdf_old->indxs[i].root=0;  // prevents dropping the index again when destroying the old table
  }    
 // saving the new index roots, saving the old non-existing index roots: 
  t_independent_block bas_acc_new(cdp); 
  bas_tbl_block * basbl_new = (bas_tbl_block*)bas_acc_new.access_block(tbdf_new->basblockn);
  if (basbl_new==NULL) return false;
  if (tbdf_new->basblockn!=tbdf_old->basblockn)
  { t_independent_block bas_acc_old(cdp); 
    bas_tbl_block * basbl_old = (bas_tbl_block*)bas_acc_old.access_block(tbdf_old->basblockn);
    if (basbl_old==NULL) return false;
    memset(basbl_old->indroot, 0, INDX_MAX*sizeof(tblocknum)); // roots of non-existing indicies are 0
    bas_acc_old.data_changed(); 
  }  
  memcpy(basbl_new->indroot, new_roots, INDX_MAX*sizeof(tblocknum)); // roots of non-existing indicies are 0
  bas_acc_new.data_changed();
  return true;    
}

/****************************************************************************/
const char * fix_attr_read(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tfcbnum * pfcbn)
{ tblocknum datadadr;  unsigned chstart, pgrec;
  if (recnum >= tbdf->Recnum()) { request_error(cdp, RECORD_DOES_NOT_EXIST);  return NULL; }
  if (cdp->isolation_level >= REPEATABLE_READ && !SYSTEM_TABLE(tbdf->tbnum))
    if (wait_record_lock_error(cdp, (table_descr*)tbdf, recnum, TMPRLOCK)) return NULL;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,recnum,tbdf->attrs[attrib].attrpage,chstart,pgrec);
  if (!datadadr) return NULL;  // error detected in ACCESS_COL_VAL
 /* fix the target page */
  const char * dt=fix_any_page(cdp, datadadr, pfcbn);
  if (!dt) return NULL;
 // detect operation with a free record:
  if (attrib!=DEL_ATTR_NUM)
    if (!tbdf->multpage || !tbdf->attrs[attrib].attrpage)  /* 1st page fixed */
    { if (dt[chstart]==RECORD_EMPTY)
#ifdef STOP
        { unfix_page(cdp, *pfcbn);  request_error(cdp, EMPTY);  return NULL; }
#else
        { unfix_page(cdp, *pfcbn);  warn(cdp, WORKING_WITH_EMPTY_RECORD);  return NULL; }  // will be converted to EMPTY in API calls
#endif        
#if WBVERS>=810
      if (dt[chstart]==DELETED) warn(cdp, WORKING_WITH_DELETED_RECORD);
#endif
    }
  return dt + chstart + tbdf->attrs[attrib].attroffset;  /* attribute address */
}

const char * fix_attr_read_naked(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tfcbnum * pfcbn)
// Not checking the record number. Not read locking for REPEATABLE_READ. Not checking record state.
{ tblocknum datadadr;  unsigned chstart, pgrec;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,recnum,tbdf->attrs[attrib].attrpage,chstart,pgrec);
  if (!datadadr) return NULL;  // error detected in ACCESS_COL_VAL
 /* fix the target page */
  const char * dt=fix_any_page(cdp, datadadr, pfcbn);
  if (!dt) return NULL;
  return dt + chstart + tbdf->attrs[attrib].attroffset;  /* attribute address */
}

BOOL fast_table_read(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, void * buf)
/* reads attribute value into buf, reads size for multiattributes, reads
        hpobj+hpsize for variable-size attributes. 
        Returns TRUE on error. */
/* Supposes that table_descr tbdf is locked */
{ tfcbnum fcbn;
  const char * dt = fix_attr_read(cdp, tbdf, recnum, attrib, &fcbn);
  if (!dt) return TRUE;
  int tp;  unsigned size;
  if (tbdf->attrs[attrib].attrmult > 1) size=CNTRSZ;
  else  /* not a multiattribute */
  { tp=tbdf->attrs[attrib].attrtype;
    if (tp==ATT_BINARY)
      size=tbdf->attrs[attrib].attrspecif.length;
    else if (IS_STRING(tp))
    { size=tbdf->attrs[attrib].attrspecif.length;
      ((tptr)buf)[size]=0;
    }
    else size=tpsize[tp];
  }
  switch (size)   /* fast I hope */
  { case 1: *(uns8 *)buf=*        dt;  break;
    case 2: *(uns16*)buf=*(uns16*)dt;  break;
    case 4: *(uns32*)buf=*(uns32*)dt;  break;
    default: memcpy(buf, dt, size);    break;
  }
  UNFIX_PAGE(cdp, fcbn);
  return FALSE;
}

uns8 table_record_state(cdp_t cdp, table_descr * tbdf, trecnum recnum)
/* returns the status of the record if a table */
{ tblocknum datadadr;  unsigned chstart, pgrec;  tfcbnum fcbn;  uns8 del;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  if (recnum >= tbdf->Recnum()) return OUT_OF_CURSOR;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,recnum,0,chstart,pgrec);
  if (!datadadr) return OUT_OF_CURSOR;  // error detected in ACCESS_COL_VAL
 /* fix the target page */
  const char * dt=fix_any_page(cdp, datadadr, &fcbn);
  if (!dt) return OUT_OF_CURSOR;
  del=dt[chstart];
  UNFIX_PAGE(cdp, fcbn);
  return del;
}

uns8 table_record_state2(cdp_t cdp, table_descr * tbdf, trecnum recnum)
// Returns the status of the record if a table. Does not check the record number.
{ tblocknum datadadr;  unsigned chstart, pgrec;  tfcbnum fcbn;  uns8 del;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,recnum,0,chstart,pgrec);
  if (!datadadr) return OUT_OF_CURSOR;  // error detected in ACCESS_COL_VAL
 /* fix the target page */
  const char * dt=fix_any_page(cdp, datadadr, &fcbn);
  if (!dt) return OUT_OF_CURSOR;
  del=dt[chstart];
  UNFIX_PAGE(cdp, fcbn);
  return del;
}

tptr fix_attr_write(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tfcbnum * pfcbn)
{ tblocknum datadadr;  unsigned chstart, pgrec;
  if (recnum >= tbdf->Recnum()) { request_error(cdp, RECORD_DOES_NOT_EXIST);  return NULL; }
  t_translog * translog = table_translog(cdp, tbdf);
  if (wait_record_lock_error(cdp, (table_descr*)tbdf, recnum, TMPWLOCK)) return NULL;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,recnum,tbdf->attrs[attrib].attrpage,chstart,pgrec);
  if (!datadadr) return NULL;  // error detected in ACCESS_COL_VAL
 /* fix the target page */
  tptr dt=fix_priv_page(cdp, datadadr, pfcbn, translog);
  if (!dt) return NULL;
  translog->log.rec_changed(*pfcbn, datadadr, tbdf->recordsize, pgrec);
 // detect operation with a free record:
  if (!tbdf->multpage || !tbdf->attrs[attrib].attrpage)  /* 1st page fixed */
  { if (dt[chstart]==RECORD_EMPTY)
      { unfix_page(cdp, *pfcbn);  request_error(cdp, EMPTY);  return NULL; }
#if WBVERS>=810
    if (dt[chstart]==DELETED) warn(cdp, WORKING_WITH_DELETED_RECORD);
#endif
  }
  return dt + chstart + tbdf->attrs[attrib].attroffset;  /* attribute address */
}

tptr fix_attr_x(cdp_t cdp, table_descr * tbdf, trecnum recnum, tfcbnum * pfcbn)
/* fixing DEL_ATTRIB without EMPTY check */
{ tblocknum datadadr;  unsigned chstart, pgrec;
  if (recnum >= tbdf->Recnum()) { request_error(cdp, RECORD_DOES_NOT_EXIST);  return NULL; }
  t_translog * translog = table_translog(cdp, tbdf);
  if (wait_record_lock_error(cdp, tbdf, recnum, TMPWLOCK)) return NULL;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,recnum,0,chstart,pgrec);
  if (!datadadr) return NULL;  // error detected in ACCESS_COL_VAL
 /* fix the target page */
  tptr dt=fix_priv_page(cdp, datadadr, pfcbn, translog);
  if (!dt) return NULL;
  translog->log.rec_changed(*pfcbn, datadadr, tbdf->recordsize, pgrec);
  return dt + chstart;  /* record start address */
}

const char * t_page_rdaccess::fix(tblocknum dadr)
{ if (fcbn!=NOFCBNUM) unfix_page(cdp, fcbn);  // double test on order to make it faster
  const char * dt = fix_any_page(cdp, dadr, &fcbn);
  if (!dt) fcbn=NOFCBNUM;
  return dt;
}  

t_table_record_rdaccess::t_table_record_rdaccess(cdp_t cdpIn, table_descr * tbdfIn) : cdp(cdpIn), tbdf(tbdfIn)
{ datafcbn=NOFCBNUM;  current_datadadr=0; }

t_table_record_rdaccess::~t_table_record_rdaccess(void)
{ 
  unfix_page(cdp, datafcbn);  // datafcbn!=NOFCBNUM tested inside
}

const char * t_table_record_rdaccess::position_on_record(trecnum rec, unsigned page)
{ tblocknum datadadr;  unsigned chstart, pgrec;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,rec,page,chstart,pgrec);  // can be faster
  if (datadadr!=current_datadadr)
  { unfix_page(cdp, datafcbn);  // datafcbn!=NOFCBNUM tested inside
    if (!datadadr) { datafcbn=NOFCBNUM;  current_datadadr=0;  return NULL; }  // error detected in ACCESS_COL_VAL
    current_datablock = fix_any_page(cdp, datadadr, &datafcbn);
    if (!current_datablock) { datafcbn=NOFCBNUM;  current_datadadr=0;  return NULL; }
    current_datadadr=datadadr;
  }
  return current_datablock + chstart;  /* record start address */
}

t_table_record_wraccess::t_table_record_wraccess(cdp_t cdpIn, table_descr * tbdfIn) : cdp(cdpIn), tbdf(tbdfIn)
{ datafcbn=NOFCBNUM;  current_datadadr=0; }

t_table_record_wraccess::~t_table_record_wraccess(void)
{ 
  unfix_page(cdp, datafcbn);  // datafcbn!=NOFCBNUM tested inside
}

char * t_table_record_wraccess::position_on_record(trecnum rec, unsigned page)
{ tblocknum datadadr;  unsigned chstart;
  datadadr=ACCESS_COL_VAL(cdp,tbdf,rec,page,chstart,pgrec);  // can be faster
  if (datadadr!=current_datadadr)  // current_datadadr is 0 initially
  { unfix_page(cdp, datafcbn);  // datafcbn!=NOFCBNUM tested inside
    if (!datadadr) { current_datablock=NULL;  datafcbn=NOFCBNUM;  current_datadadr=0;  return NULL; }  // error detected in ACCESS_COL_VAL
    current_datablock = fix_priv_page(cdp, datadadr, &datafcbn, table_translog(cdp, tbdf));
    if (!current_datablock) { datafcbn=NOFCBNUM;  current_datadadr=0;  return NULL; }
    current_datadadr=datadadr;
  }
  return current_datablock + chstart;  /* record start address */
}

void t_table_record_wraccess::unposition(void)
{ unfix_page(cdp, datafcbn);  // datafcbn!=NOFCBNUM tested inside
  datafcbn=NOFCBNUM;  current_datadadr=0; 
}

#define ACCESS_INDEXED_COL_VAL(cdp,tbdf,recnum,att,index,dt,datadadr,offs)\
/* returns datadadr and offs for the indexed value, may immediately return if value located in the dt page*/\
{ int as=TYPESIZE(att);\
  if (!tbdf->multpage) /* record not bigger than a page */\
    return dt + CNTRSZ + index*as; \
  int p1=(BLOCKSIZE-att->attroffset+CNTRSZ) / as; /* p1-number of values in the first page */\
  if (index < p1)  /* value is in the same page! */\
    return dt + CNTRSZ + index*as;\
 /* another page */\
  index -= p1;\
  p1=BLOCKSIZE/as; /* p1-number of values per full page */\
  tblocknum blnum = (tblocknum)(recnum * tbdf->rectopage + att->attrpage);\
  blnum += index/p1 + 1;\
  offs = as * (index % p1);\
 /* find the blnum-th block of the table tb */\
  bas_tbl_block_info * bbl = tbdf->get_basblock();\
  if (bbl->diraddr) datadadr=bbl->databl[blnum];\
  else  /* indirect block addressing */\
  { tfcbnum fcbn0;\
    const tblocknum * datadadrs = (const tblocknum *)fix_any_page(cdp, bbl->databl[blnum/DADRS_PER_BLOCK], &fcbn0);\
    if (!datadadrs) { bbl->Release();  return NULL; }\
    datadadr=(datadadrs)[blnum % DADRS_PER_BLOCK];\
    UNFIX_PAGE(cdp, fcbn0);\
  }\
  bbl->Release();\
}

const char * fix_attr_ind_read(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, tfcbnum * pfcbn)
/* if the attr doesn't exist, the NONE value will be returned. */
/* returns NULL on error */
{ const char * dt;  tblocknum datadadr;  unsigned offs;  
  // recnum range check is in fix_attr
  const attribdef * att = tbdf->attrs+attrib;
  if (att->attrmult <= 1) { request_error(cdp, BAD_MODIF);  return NULL; }
 // fixing the starting page:
  dt=fix_attr_read(cdp, tbdf, recnum, attrib, pfcbn);
  if (!dt) return NULL;
  if (index >= *(t_mult_size*)dt)
  { warn(cdp, INDEX_OOR);
    return (const char *)null_value(att->attrtype);
  }

  if (index < (att->attrmult & 0x7f))  /* value located in the fixed part */
  { ACCESS_INDEXED_COL_VAL(cdp,tbdf,recnum,att,index,dt,datadadr,offs);
    // returned if valued located in the fixed page
    UNFIX_PAGE(cdp, *pfcbn);
   /* fix the target page */
    dt=fix_any_page(cdp, datadadr, pfcbn);
    if (!dt) return NULL;
    return dt+offs;  /* attribute address */
  }
  else /* accessing the variable-size part of the multiattribute */
  { if (!(att->attrmult & 0x80))
      { unfix_page(cdp, *pfcbn); request_error(cdp, INDEX_OUT_OF_RANGE);  return NULL; }
    tfcbnum base_fcbn = *pfcbn;  *pfcbn=NOFCBNUM;
    dt=hp_locate_read(cdp, (const tpiece *)(dt-sizeof(tpiece)), TYPESIZE(att), index-(att->attrmult & 0x7f), pfcbn);
    unfix_page(cdp, base_fcbn);
    return dt; // may be NULL on error
  }
}

tptr fix_attr_ind_write(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, tfcbnum * pfcbn)
/* if the attr doesn't exist, it will be created (if possible) */
/* returns NULL on error */
{ tptr dt;  tblocknum datadadr;  unsigned offs;  
  // recnum range check is in fix_attr
  t_translog * translog = table_translog(cdp, tbdf);
  const attribdef * att = tbdf->attrs+attrib;
  if (att->attrmult <= 1) { request_error(cdp, BAD_MODIF);  return NULL; }
 // fixing the starting page (will lock it & marked as chnged, used when writing into it AND when count increases):
  dt=fix_attr_write(cdp, tbdf, recnum, attrib, pfcbn);
  if (!dt) return NULL;
  
  if (index < (att->attrmult & 0x7f))  /* value located in the fixed part */
  { if (index >= *(t_mult_size*)dt) *(t_mult_size*)dt=index+1;
    ACCESS_INDEXED_COL_VAL(cdp,tbdf,recnum,att,index,dt,datadadr,offs);
    // returned if valued located in the fixed page
    UNFIX_PAGE(cdp, *pfcbn);
   /* fix the target page */
    dt=fix_priv_page(cdp, datadadr, pfcbn, translog);
    if (!dt) return NULL;
    translog->log.rec_changed(*pfcbn, datadadr, tbdf->recordsize, 0);
    return dt+offs;  /* attribute address */
  }
  else /* accessing the variable-size part of the multiattribute */
  { if (!(att->attrmult & 0x80))
      { unfix_page(cdp, *pfcbn); request_error(cdp, INDEX_OUT_OF_RANGE);  return NULL; }
    if (index >= *(t_mult_size*)dt) *(t_mult_size*)dt=index+1;
    tfcbnum base_fcbn = *pfcbn;  *pfcbn=NOFCBNUM;
    dt=hp_locate_write(cdp, (tpiece*)(dt-sizeof(tpiece)), TYPESIZE(att), index-(att->attrmult & 0x7f), pfcbn, translog);
    unfix_page(cdp, base_fcbn);
    return dt; // may be NULL on error
  }
}

BOOL tb_read_atr(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attrib, tptr buf_or_null)
{ table_descr * tbdf = tables[tb];
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  return tb_read_atr(cdp, tbdf, recnum, attrib, buf_or_null);
}

BOOL tb_read_atr(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tptr buf_or_null)
{ tfcbnum fcbn;  const char * dt;  unsigned attsz;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_READ, NULL, 0);
  if (cdp->trace_enabled & TRACE_READ)
  { char buf[81];
    trace_msg_general(cdp, TRACE_READ, form_message(buf, sizeof(buf), msg_trace_read), tbdf->tbnum); // detailed info is in the kontext
  }
  if (!(dt=fix_attr_read(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE;
  const attribdef * att=tbdf->attrs + attrib;
  if (IS_STRING(att->attrtype))
  { attsz=att->attrspecif.length;
    if (buf_or_null==NULL)
    { if (att->attrspecif.wide_char)
      { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+attsz+2);
        if (buf_or_null==NULL)  { unfix_page(cdp, fcbn);  return TRUE; }
        *(uns32*)buf_or_null=attsz+2;  buf_or_null+=sizeof(uns32);
      }
      else
      { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+attsz+1);
        if (buf_or_null==NULL)  { unfix_page(cdp, fcbn);  return TRUE; }
        *(uns32*)buf_or_null=attsz+1;  buf_or_null+=sizeof(uns32);
      }
    }
    memcpy(buf_or_null, dt, attsz);  buf_or_null[attsz]=0;
    if (att->attrspecif.wide_char)  buf_or_null[attsz+1]=0;
  }
  else if (att->attrtype==ATT_AUTOR && !(cdp->ker_flags & KFL_ALLOW_TRACK_WRITE))
  { attsz=OBJ_NAME_LEN;
    if (buf_or_null==NULL)
    { if (!(buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+1+OBJ_NAME_LEN)))
        { unfix_page(cdp, fcbn);  return TRUE; }
      *(uns32*)buf_or_null=OBJ_NAME_LEN+1;  buf_or_null+=sizeof(uns32);
    }
    if (!memcmp(dt, tNONEAUTOR, UUID_SIZE))
      strcpy(buf_or_null, nobody_name);
    else
    { tobjnum objnum = find_object_by_id(cdp, CATEG_USER, (uns8*)dt);
      if (objnum==NOOBJECT) strcpy(buf_or_null, unknown_name);
      else
      { if (fast_table_read(cdp, usertab_descr, (trecnum)objnum, USER_ATR_LOGNAME, buf_or_null))
          { unfix_page(cdp, fcbn);  return TRUE; }
        buf_or_null[OBJ_NAME_LEN]=0;   /* string delimiter */
      }
    }
    unfix_page(cdp, fcbn);
    return FALSE;
  }
  else /* normal attributes */
  { if (IS_HEAP_TYPE(att->attrtype))  /* I am not sure if this can be here */
      { unfix_page(cdp, fcbn);  request_error(cdp, BAD_MODIF);  return TRUE; }
    attsz=att->attrtype==ATT_BINARY ? att->attrspecif.length : tpsize[att->attrtype];
    if (buf_or_null==NULL)
    { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+attsz);
      if (buf_or_null==NULL)  { unfix_page(cdp, fcbn);  return TRUE; }
#ifndef __ia64__
      *(uns32*)buf_or_null=attsz; 
#else
      memcpy(buf_or_null, &attsz, sizeof(uns32));
#endif
      buf_or_null+=sizeof(uns32);
    }
#ifndef __ia64__
    switch (attsz)   /* fast I hope */
    { case 1:  *buf_or_null        =*dt;          break;
      case 2:  *(uns16*)buf_or_null=*(uns16*)dt;  break;
      case 4:  *(uns32*)buf_or_null=*(uns32*)dt;  break;
      default: memcpy(buf_or_null, dt, attsz);    break;
    }
#else
    memcpy(buf_or_null, dt, attsz);
#endif
  }
 /* common for all except autor: */
  UNFIX_PAGE(cdp, fcbn);
  return FALSE;
}

BOOL tb_read_ind(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, tptr buf_or_null)
{ tfcbnum fcbn;  const char * dt;  unsigned attsz;
  const attribdef * att=tbdf->attrs + attrib;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, index, OP_READ, NULL, 0);
  if (cdp->trace_enabled & TRACE_READ)
  { char buf[81];
    trace_msg_general(cdp, TRACE_READ, form_message(buf, sizeof(buf), msg_trace_read), tbdf->tbnum); // detailed info is in the kontext
  }
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  if (!(dt=fix_attr_ind_read(cdp, tbdf, recnum, attrib, index, &fcbn)))
    return TRUE;
  if (IS_STRING(att->attrtype))
  { attsz=att->attrspecif.length;
    if (buf_or_null==NULL)
      if (att->attrspecif.wide_char)
      { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+attsz+2);
        if (buf_or_null==NULL)  { unfix_page(cdp, fcbn);  return TRUE; }
        *(uns32*)buf_or_null=attsz+2;  buf_or_null+=sizeof(uns32);
      }
      else
      { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+attsz+1);
        if (buf_or_null==NULL)  { unfix_page(cdp, fcbn);  return TRUE; }
        *(uns32*)buf_or_null=attsz+1;  buf_or_null+=sizeof(uns32);
      }
    buf_or_null[attsz]=0;  if (att->attrspecif.wide_char)  buf_or_null[attsz+1]=0;
  }
  else /* normal attributes */
  { attsz=att->attrtype==ATT_BINARY ? att->attrspecif.length : tpsize[att->attrtype];
    if (buf_or_null==NULL)
    { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+attsz);
      if (buf_or_null==NULL)  { unfix_page(cdp, fcbn);  return TRUE; }
      *(uns32*)buf_or_null=attsz;  buf_or_null+=sizeof(uns32);
    }
  }
  memcpy(buf_or_null, dt, attsz);
  UNFIX_PAGE(cdp, fcbn);
  return FALSE;
}

BOOL tb_read_var(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, uns32 start, uns32 size, tptr buf_or_null)
// Reads an interval of a var-len column value.
// if (buf_or_null!=NULL) stores the result into it and its length into cdp->tb_read_var_result.
// if (buf_or_null==NULL) puts 2 bytes of length and the result into the answer (length<64KB!!).
{ tfcbnum fcbn;  const heapacc * tmphp;  const char * dt;  
  BOOL res=FALSE;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_READ, NULL, 0);
  if (cdp->trace_enabled & TRACE_READ)
  { char buf[81];
    trace_msg_general(cdp, TRACE_READ, form_message(buf, sizeof(buf), msg_trace_read), tbdf->tbnum); // detailed info is in the kontext
  }
  const attribdef * att = tbdf->attrs+attrib;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  if (IS_HEAP_TYPE(att->attrtype))
  { if (att->attrmult > 1) { request_error(cdp,BAD_MODIF);  return TRUE; }
    if (!(tmphp=(const heapacc*)fix_attr_read(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE;
    if (start >= tmphp->len) size=0;
    else if (start+size > tmphp->len) size=tmphp->len-start;
    if (buf_or_null==NULL) // write the result to answer
    { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+size);
      if (buf_or_null==NULL) res=TRUE;
      else
      { *(uns32*)buf_or_null=(uns32)size;
        if (hp_read(cdp, &tmphp->pc, 1, start, size, buf_or_null+sizeof(uns32))) res=TRUE;
      }
    }
    else // buf_or_null provided externally
    { cdp->tb_read_var_result=size;
      if (hp_read(cdp, &tmphp->pc, 1, start, size, buf_or_null)) res=TRUE;
    }
    UNFIX_PAGE(cdp, fcbn);
  }
  else if (IS_EXTLOB_TYPE(att->attrtype))
  { if (!(dt=fix_attr_read(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE;
    read_ext_lob(cdp, *(uns64*)dt, start, size, buf_or_null);
    UNFIX_PAGE(cdp, fcbn);
  }
  else /* reading interval, used by tb_rem_inval, multi_in etc. */
  { tptr p;  t_mult_size i, num;  int attsz=TYPESIZE(att);
    if (att->attrmult <= 1) { request_error(cdp,BAD_MODIF);  return TRUE; }
    if (tb_read_ind_num(cdp, tbdf, recnum, attrib, &num)) return TRUE;
    if (num <= start) size=0;
    else if (num < start+size) size=num-(uns16)start;
    if (!buf_or_null)
      if (!(buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+size*attsz)))
        return TRUE;
    *(uns32*)buf_or_null=(uns32)(size*attsz);
    for (i=0, p=buf_or_null+sizeof(uns32);  i<size;  i++, p+=attsz)
    { dt=fix_attr_ind_read(cdp, tbdf, recnum, attrib, (uns16)start+i, &fcbn);
      if (!dt) return TRUE;
      memcpy(p, dt, attsz);
      UNFIX_PAGE(cdp, fcbn);
    }
  }
  return res;
}

BOOL tb_read_atr_len(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, uns32 * len)
{ tfcbnum fcbn;  const heapacc * tmphp;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_READ, NULL, 0);
  if (cdp->trace_enabled & TRACE_READ)
  { char buf[81];
    trace_msg_general(cdp, TRACE_READ, form_message(buf, sizeof(buf), msg_trace_read), tbdf->tbnum); // detailed info is in the kontext
  }
  const attribdef * att = tbdf->attrs+attrib;
  if (!(tmphp=(const heapacc*)fix_attr_read(cdp, tbdf, recnum, attrib, &fcbn)))
    return TRUE;
  if (IS_EXTLOB_TYPE(att->attrtype))
    *len = read_ext_lob_length(cdp, *(uns64*)tmphp);
  else
    *len = tmphp->len;
  UNFIX_PAGE(cdp, fcbn);
  return FALSE;
}

BOOL tb_read_ind_var(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 start, uns32 size, tptr buf_or_null)
// Reads an interval of a var-len indexed column value.
// if (buf_or_null!=NULL) stores the result into it and its length into cdp->tb_read_var_result.
// if (buf_or_null==NULL) puts 2 bytes of length and the result into the answer (length<64KB!!).
{ tfcbnum fcbn;  const heapacc * tmphp;  BOOL res=FALSE;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, index, OP_READ, NULL, 0);
  if (cdp->trace_enabled & TRACE_READ)
  { char buf[81];
    trace_msg_general(cdp, TRACE_READ, form_message(buf, sizeof(buf), msg_trace_read), tbdf->tbnum); // detailed info is in the kontext
  }
  if (!(tmphp=(const heapacc*)fix_attr_ind_read(cdp, tbdf, recnum, attrib, index, &fcbn)))
    return TRUE;
  if (start >= tmphp->len) size=0;
  else if (start+size > tmphp->len) size=tmphp->len-start;
  if (buf_or_null==NULL) // write the result to answer
  { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+size);
    if (buf_or_null==NULL) res=TRUE;
    else
    { *(uns32*)buf_or_null=size;
      if (hp_read(cdp, &tmphp->pc, 1, start, size, buf_or_null+sizeof(uns32))) res=TRUE;
    }
  }
  else // buf_or_null provided externally
  { cdp->tb_read_var_result=size;
    if (hp_read(cdp, &tmphp->pc, 1, start, size, buf_or_null)) res=TRUE;
  }
  UNFIX_PAGE(cdp, fcbn);
  return res;
}

BOOL tb_read_ind_len(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 * len)
{ tfcbnum fcbn;  const heapacc * tmphp;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_READ, NULL, 0);
  if (cdp->trace_enabled & TRACE_READ)
  { char buf[81];
    trace_msg_general(cdp, TRACE_READ, form_message(buf, sizeof(buf), msg_trace_read), tbdf->tbnum); // detailed info is in the kontext
  }
  if (!(tmphp=(const heapacc*)fix_attr_ind_read(cdp, tbdf, recnum, attrib, index, &fcbn)))
    return TRUE;
  *len=tmphp->len;
  UNFIX_PAGE(cdp, fcbn);
  return FALSE;
}

BOOL tb_read_ind_num(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size * num)
{ tfcbnum fcbn;  const char * dt;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_READ, NULL, 0);
  if (cdp->trace_enabled & TRACE_READ)
  { char buf[81];
    trace_msg_general(cdp, TRACE_READ, form_message(buf, sizeof(buf), msg_trace_read), tbdf->tbnum); // detailed info is in the kontext
  }
  if (!(dt=fix_attr_read(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE;
  *num=*(const t_mult_size*)dt;
  UNFIX_PAGE(cdp, fcbn);
  return FALSE;
}

/********************************* write ************************************/
#define MAX_HIST_ATTR_SIZE 256  // used only on the old history, wrong for the new long strings

static BOOL write_trace(cdp_t cdp, table_descr * tbdf, trecnum recnum,
                               tattrib attrib, tptr info, unsigned attsz, BOOL new_rec/* = FALSE*/)
{ char histval[MAX_HIST_ATTR_SIZE + UUID_SIZE + sizeof(timestamp)];
  tptr dt;  tfcbnum fcbn;
  if (attsz>MAX_HIST_ATTR_SIZE) attsz=MAX_HIST_ATTR_SIZE;  // for long char and binary strings
  unsigned fullsz=attsz;
  if (cdp->ker_flags & KFL_NO_TRACE) return FALSE;  /* e.g. ALTER TABLE, import */
  const attribdef * traced_att = &tbdf->attrs[attrib];
  t_translog * translog = table_translog(cdp, tbdf);
  int tp = traced_att->attrtype;
  if (tp==ATT_AUTOR || tp==ATT_DATIM)
    return FALSE; /* possible when modifying a table, would damage the history */
 /* Cannot use attrspec2 & IS_TRACED, it is set when record tracing is on, too */
 /* Authorization, date of the change & history for the current attribute: */
  while (++attrib < tbdf->attrcnt)
  { tp=tbdf->attrs[attrib].attrtype;
    if (tp==ATT_AUTOR)
    { if (!(dt=fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn)))
        return TRUE;
      memcpy(histval+fullsz, dt, UUID_SIZE);  // previous autor to history
      fullsz+=UUID_SIZE;
      memcpy(dt, cdp->prvs.user_uuid, UUID_SIZE);
      unfix_page(cdp, fcbn);
    }
    else if (tp==ATT_DATIM)
    { if (!(dt=fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn)))
        return TRUE;
      *(timestamp*)(histval+fullsz)=*(timestamp*)dt; // previous datim to history
      fullsz+=sizeof(timestamp);
      *(timestamp*)dt=stamp_now();
      unfix_page(cdp, fcbn);
    }
    else if (tp==ATT_HIST && info)
    { heapacc * tmphp = (heapacc*)fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn);
      if (!tmphp) return TRUE;
      if (new_rec)
      { tmphp->len=0;  tmphp->pc=NULLPIECE;
        set_null(histval, traced_att);
      }
      else
        memcpy(histval, info, attsz);
      hp_write(cdp, &tmphp->pc, 1, tmphp->len, fullsz, histval, translog); /* check not necessary */
      tmphp->len += fullsz;
      unfix_page(cdp, fcbn);
    }
    else break;
  }
 /* write the record tracing info: */
  if (tbdf->is_traced)
    for (attrib=1;  attrib<tbdf->attrcnt;  attrib++)
    { if (!memcmp(tbdf->attrs[attrib].name, "_W5_", 4)) continue;
      tp=tbdf->attrs[attrib].attrtype;
      if (tp==ATT_AUTOR)
      { if (!(dt=fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn)))
          return TRUE;
        memcpy(dt, cdp->prvs.user_uuid, UUID_SIZE);
        unfix_page(cdp, fcbn);
      }
      else if (tp==ATT_DATIM)
      { if (!(dt=fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn)))
          return TRUE;
        *(timestamp*)dt=stamp_now();
        unfix_page(cdp, fcbn);
      }
      else break;
    }
  return FALSE;
}

BOOL tb_add_value(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, char ** val)
{ attribdef * att;  tattrib attrib2;  char valbuf[256];
  trecnum recnum2;  uns16 size, index, num;  ttablenum tb2;
  att=tbdf->attrs + attrib;  size=typesize(att);
  if (IS_HEAP_TYPE(att->attrtype) || (att->attrmult <= 1))
    { request_error(cdp,BAD_MODIF);  return TRUE; }
 /* biponter check: */
  if (att->attrtype==ATT_BIPTR)
  { if (bi_tables(cdp, tbdf->tbnum, attrib, &tb2, &attrib2))   /* nothing to do */
      { *val+=size;  return FALSE; }
    unlock_tabdescr(tb2);
    recnum2=*(trecnum*)*val;
    if (recnum2==NORECNUM) return FALSE;
  }
 /* searching the value to be added */
  if (tb_read_ind_num(cdp, tbdf, recnum, attrib, &num)) return TRUE;
  index=0;
  while (index < num)
  { if (!tb_read_ind(cdp, tbdf, recnum, attrib, index, valbuf))
      if (!memcmp(valbuf, *val, size))  /* value found */
        { *val+=size;  return FALSE; } /* nothing to do */
    index++;
  }
  if (tb_write_ind(cdp, tbdf, recnum, attrib, num, *val)) return TRUE;
  *val += TYPESIZE(att);
  return FALSE;
}

BOOL tb_del_value(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, tptr * val)
{ attribdef * att;  tattrib attrib2;
  trecnum recnum2;  uns16 size, index, num;  ttablenum tb2;
  char valbuf[256];
  att=tbdf->attrs + attrib;  size=typesize(att);
  if (IS_HEAP_TYPE(att->attrtype) || (att->attrmult <= 1))
    { request_error(cdp,BAD_MODIF);  return TRUE; }
 /* biponter check: */
  if (att->attrtype==ATT_BIPTR)
  { if (bi_tables(cdp, tbdf->tbnum, attrib, &tb2, &attrib2)) /* nothing to do */
      { *val+=size;  return FALSE; }
    unlock_tabdescr(tb2);
    recnum2=*(trecnum*)*val;
    if (recnum2==NORECNUM) { *val+=size;  return FALSE; }
  }
 /* searching the value to be deleted */
  if (tb_read_ind_num(cdp, tbdf, recnum, attrib, &num)) return TRUE;
  index=0;
  do
  { if (index >= num) { *val+=size;  return FALSE; } /* nothing to do */
    if (!tb_read_ind(cdp, tbdf, recnum, attrib, index, valbuf))
      if (!memcmp(valbuf, *val, size)) break;  /* value found */
    index++;
  } while (TRUE);
 /* deleting the value */
  num--;
  if (index < num)  /* deleting from the middle */
  {/* deleting the value (with biptr actions) */
    if (tb_write_ind(cdp, tbdf, recnum, attrib, index, &tNONEPTR)) return TRUE;
   /* moving the last value (with no biptr actions!) */
    if (tb_read_ind(cdp, tbdf, recnum, attrib, num, valbuf)) return TRUE;
    uns32 saved_ker_flags = cdp->ker_flags;
    cdp->ker_flags |= KFL_BI_STOP;
    wbbool res = tb_write_ind(cdp, tbdf, recnum, attrib, index, valbuf) ||
                 tb_write_ind_num(cdp, tbdf, recnum, attrib, num);
    cdp->ker_flags = saved_ker_flags;
    return res;
  }
  else
    if (tb_write_ind_num(cdp, tbdf, recnum, attrib, num)) return TRUE;
  *val+=size;
  return FALSE;
}

BOOL tb_rem_inval(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib)
{ attribdef * att;  tattrib attrib2;  BOOL locked2;
  trecnum recnum2;  uns16 size, num;  ttablenum tb2;
  uns8 tp;  tptr buf;  trecnum destrec;  unsigned i;
  att=tbdf->attrs + attrib;  size=typesize(att);  tp=att->attrtype;
  if (IS_HEAP_TYPE(att->attrtype))
    { request_error(cdp,BAD_MODIF);  return TRUE; }
 /* biponter check: */
  if (bi_tables(cdp, tbdf->tbnum, attrib, &tb2, &attrib2))   /* nothing to do */
    if (att->attrtype==ATT_BIPTR) return FALSE;
    else locked2=FALSE;
  else locked2=TRUE;
  if (tbdf->attrs[attrib].attrspecif.destination_table==NOT_LINKED) return FALSE;
  if (att->attrmult <= 1)
    if ((tp==ATT_PTR) || (tp==ATT_BIPTR))
    { if (fast_table_read(cdp, tbdf, recnum, attrib, &recnum2))
        { if (locked2) unlock_tabdescr(tb2);  return TRUE; }
      if (table_record_state(cdp, tables[tb2], recnum2) == NOT_DELETED)
        { if (locked2) unlock_tabdescr(tb2);  return FALSE; }
      destrec=NORECNUM;
      if (locked2) unlock_tabdescr(tb2);
      uns32 saved_ker_flags = cdp->ker_flags;
      cdp->ker_flags |= KFL_BI_STOP;
      BOOL res = tb_write_atr(cdp, tbdf, recnum, attrib, &destrec);
      cdp->ker_flags = saved_ker_flags;
      return res;
    }
    else
    { if (locked2) unlock_tabdescr(tb2);
      request_error(cdp,BAD_MODIF);  return TRUE;  /* not a multiattribute */
    }
  else  /* multiattribute */
  { if (tb_read_ind_num(cdp, tbdf, recnum, attrib, &num))
      { if (locked2) unlock_tabdescr(tb2);  return TRUE; }
    if (!(buf=(tptr)corealloc(2+size*num+1, 73)))  /* +1 - must not allocate 0 size */
      { request_error(cdp,OUT_OF_KERNEL_MEMORY);  if (locked2) unlock_tabdescr(tb2);  return TRUE; }
    if (tb_read_var(cdp, tbdf, recnum, attrib, 0, num, buf)) // reading index interval, with count on the beginning
      { corefree(buf);  if (locked2) unlock_tabdescr(tb2);  return TRUE; }
    for (i=0; i<num; i++)
    { if (!is_null(buf+2+i*size, tp, att->attrspecif))
        if ((tp==ATT_PTR) || (tp==ATT_BIPTR))
        { destrec=((trecnum*)(buf+2))[i];
          if (table_record_state(cdp, tables[tb2], destrec) == NOT_DELETED) continue;
        }
        else continue;
     /* removing the invalid value */
      num--;   /* now, the last value has the num index */
      if (i==num)
      { uns32 saved_ker_flags = cdp->ker_flags;
        cdp->ker_flags |= KFL_BI_STOP;
        wbbool res=tb_write_ind_num(cdp, tbdf, recnum, attrib, num);
        cdp->ker_flags = saved_ker_flags;
        if (res)
          { corefree(buf);  if (locked2) unlock_tabdescr(tb2);  return TRUE; }
      }
      else
      { char * pvalbuf=buf+2+size*i;
        memcpy(pvalbuf, buf+2+size*num, size);
        uns32 saved_ker_flags = cdp->ker_flags;
        cdp->ker_flags |= KFL_BI_STOP;
        wbbool res = tb_write_ind(cdp, tbdf, recnum, attrib, i, pvalbuf) ||
                     tb_write_ind_num(cdp, tbdf, recnum, attrib, num);
        cdp->ker_flags = saved_ker_flags;
        if (res)
          { corefree(buf);  if (locked2) unlock_tabdescr(tb2);  return TRUE; }
      }
      i--;   /* re-check the same index */
    }
    corefree(buf);
  }
  if (locked2) unlock_tabdescr(tb2);
  return FALSE;
}

BOOL multi_in(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, void * arg)
// preforms set operation oper on multiattribute attrib in tbdf/recnum.
{ BOOL res=TRUE;  // TRUE means NOT IN
  if (recnum < tbdf->Recnum())  /* error not reported */
  { const attribdef * att = tbdf->attrs+attrib;
    int sz=typesize(att);  t_mult_size i, valnum;  
    if (att->attrmult>1)   // otherwise not IN 
      if (!tb_read_ind_num(cdp, tbdf, recnum, attrib, &valnum)) 
      { tptr buf = (tptr)corealloc(sz*valnum+1, 73);  // +1 - must not allocate 0 size 
        if (!buf)  
          request_error(cdp,OUT_OF_KERNEL_MEMORY);
        else
        { if (!tb_read_var(cdp, tbdf, recnum, attrib, 0, valnum, buf)) // index interval, count on the beginning of the result!
            for (i=0;  i<valnum;  i++)
              if (IS_STRING(att->attrtype) ?
                  !cmp_str((tptr)arg, buf+sizeof(uns32)+sz*i, att->attrspecif) :
                  !memcmp (      arg, buf+sizeof(uns32)+sz*i, sz))
                { res=FALSE; /* is IN! */ break; }
          corefree(buf);
        } // buffer allocated
      } // count read OK, is a multiattribute
  } // recnum OK
  return res;
}

BOOL tb_biptr_write(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attrib,
                    trecnum oldval, trecnum newval)
{ ttablenum tb2;  trecnum recnum3;  tattrib attrib2;
  if (bi_tables(cdp, tb, attrib, &tb2, &attrib2)) return TRUE; /* nothing to do */
 /* delete the old antilink */
  if ((oldval!=NORECNUM) && (oldval<tables[tb2]->Recnum()))
  { if (tables[tb2]->attrs[attrib2].attrmult > 1)
    { trecnum * precnum=&recnum;
      uns32 saved_ker_flags = cdp->ker_flags;
      cdp->ker_flags |= KFL_BI_STOP;
      wbbool res=tb_del_value(cdp, tables[tb2], oldval, attrib2, (tptr *)&precnum);
      cdp->ker_flags = saved_ker_flags;
      if (res)
        { unlock_tabdescr(tb2);  return TRUE; }
    }
    else
    { uns32 saved_ker_flags = cdp->ker_flags;
      cdp->ker_flags |= KFL_BI_STOP;
      wbbool res = tb_write_atr(cdp, tables[tb2], oldval, attrib2, &tNONEPTR);
      cdp->ker_flags = saved_ker_flags;
      if (res)
        { unlock_tabdescr(tb2);  return TRUE; }
    }
  }
 /* create the new antilink */
  trecnum * precnum=&recnum;
  if ((newval!=NORECNUM) && (newval<tables[tb2]->Recnum()))
    if (tables[tb2]->attrs[attrib2].attrmult > 1)
    { uns32 saved_ker_flags = cdp->ker_flags;
      cdp->ker_flags |= KFL_BI_STOP;
      wbbool res=tb_add_value(cdp, tables[tb2], newval, attrib2, (char **)&precnum);
      cdp->ker_flags = saved_ker_flags;
      if (res)
        { unlock_tabdescr(tb2);  return TRUE; }
    }
    else /* the most complex case: must deleted the anti-antilink */
    { if (fast_table_read(cdp, tables[tb2], newval, attrib2, (tptr)&recnum3))
        { unlock_tabdescr(tb2);  return TRUE; }
      if ((recnum3!=NORECNUM) && (recnum3<tables[tb]->Recnum()))
        if (tables[tb]->attrs[attrib].attrmult > 1)
        { trecnum * precnum2=&newval;
          uns32 saved_ker_flags = cdp->ker_flags;
          cdp->ker_flags |= KFL_BI_STOP;
          wbbool res = tb_del_value(cdp, tables[tb], recnum3, attrib, (tptr *)&precnum2);
          cdp->ker_flags = saved_ker_flags;
          if (res)
            { unlock_tabdescr(tb2);  return TRUE; }
        }
        else
        { uns32 saved_ker_flags = cdp->ker_flags;
          cdp->ker_flags |= KFL_BI_STOP;
          wbbool res=tb_write_atr(cdp, tables[tb], recnum3, attrib, &tNONEPTR);
          cdp->ker_flags = saved_ker_flags;
          if (res)
            { unlock_tabdescr(tb2);  return TRUE; }
        }
      uns32 saved_ker_flags = cdp->ker_flags;
      cdp->ker_flags |= KFL_BI_STOP;
      BOOL res = tb_write_atr(cdp, tables[tb2], newval, attrib2, &recnum);
      cdp->ker_flags = saved_ker_flags;
      if (res)
        { unlock_tabdescr(tb2);  return TRUE; }
    }
  unlock_tabdescr(tb2);
  return FALSE;
}

BOOL tb_write_atr(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, const void * ptr)
{ tfcbnum fcbn;  tptr dt;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_WRITE, ptr, 0);
  if (cdp->trace_enabled & TRACE_WRITE)
  { char buf[81];
    trace_msg_general(cdp, TRACE_WRITE, form_message(buf, sizeof(buf), msg_trace_write), tbdf->tbnum); // detailed info is in the kontext
  }
  const attribdef * att = tbdf->attrs + attrib;
  int attsz=TYPESIZE(att);
 /* processing of bipointers */
  if ((att->attrtype == ATT_BIPTR) && !(cdp->ker_flags & KFL_BI_STOP) && !replaying)
  { ttablenum tb2;  tattrib attrib2;  
    if (bi_tables(cdp, tbdf->tbnum, attrib, &tb2, &attrib2)) //return FALSE;
       /* used when redefining multibipointer into singlebipointer */
    { if (!(dt=fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE; }
    else
    { unlock_tabdescr(tb2);
       /* must check the bi-table before fixing index! */
      if (!(dt=fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE;
      if (tb_biptr_write(cdp, tbdf->tbnum, recnum, attrib, *(trecnum*)dt, *(trecnum*)ptr))
        return FALSE;
    }
  }
  else if (!(dt=fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE;
 /* normal write operation */
  { t_simple_update_context sic(cdp, tbdf, attrib);
    if (att->attrspec2 & IS_IN_INDEX)
      if (att->attrpage || dt[-att->attroffset]==NOT_DELETED)  // do not remove deleted records
        if (!sic.lock_roots() || !sic.inditem_remove(recnum, (const char *)ptr)) /* remove record from indexes */
          { unfix_page(cdp, fcbn);  return TRUE; }
    if (att->attrspec2 & IS_TRACED) /* must be called before overwriting the old value */
      write_trace(cdp, tbdf, recnum, attrib, dt, attsz);
    switch (attsz)
    { case 1: *dt        =*(uns8 *)ptr;  break;
      case 2: *(uns16*)dt=*(uns16*)ptr;  break;
      case 4: *(uns32*)dt=*(uns32*)ptr;  break;
      default: memcpy(dt, ptr, attsz);   break;
    }
   // unique test:
    if (att->defvalexpr==COUNTER_DEFVAL)   /* UNIQUE value */
      tbdf->unique_value_cache.register_used_value(cdp, get_typed_unique_value(dt, att->attrtype));
   // token, ZCR, LUO:
    if (tbdf->zcr_attr!=NOATTRIB)  // ZCR update must be before inditem_add!
      if (writing_to_replicable_table2(cdp, tbdf, recnum, attrib, dt, false))
        { unfix_page(cdp, fcbn);  return TRUE; }

    if (att->attrspec2 & IS_IN_INDEX)
      if (att->attrpage || dt[-att->attroffset]==NOT_DELETED)  // do not add deleted records
        if (!sic.inditem_add(recnum))  /* add record into indexes */
          { unfix_page(cdp, fcbn);  return TRUE; }
  }
  UNFIX_PAGE(cdp, fcbn);
 // constrains, change log:
  register_change(cdp, tbdf, recnum, attrib);
  if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
  { worm_add(cdp, &cdp->jourworm, &opcode_write     ,1                );
    worm_add(cdp, &cdp->jourworm, &tbdf->tbnum      ,sizeof(ttablenum));
    worm_add(cdp, &cdp->jourworm, &recnum           ,sizeof(trecnum)  );
    worm_add(cdp, &cdp->jourworm, &attrib           ,sizeof(tattrib)  );
    worm_add(cdp, &cdp->jourworm, &mod_stop         ,1                );
    worm_add(cdp, &cdp->jourworm, ptr              ,attsz            );
  }
  if (att->signat!=NOATTRIB)
    if (!IS_SYSTEM_COLUMN(att->name))
      invalidate_signature(cdp, tbdf, recnum, att->signat);
  return FALSE;
}

static BOOL tb_write_ind_pure(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, const void * ptr)
// Used by tb_write_vector, auxiliary operations are outside.
{ tfcbnum fcbn;  tptr dt;
  t_mult_size num;  ttablenum tb2;  tattrib attrib2;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, index, OP_WRITE, ptr, 0);
  if (cdp->trace_enabled & TRACE_WRITE)
  { char buf[81];
    trace_msg_general(cdp, TRACE_WRITE, form_message(buf, sizeof(buf), msg_trace_write), tbdf->tbnum); // detailed info is in the kontext
  }
  const attribdef * att = tbdf->attrs + attrib;
  int attsz=TYPESIZE(att);
 /* processing of bipointers */
  if ((att->attrtype == ATT_BIPTR) && !(cdp->ker_flags & KFL_BI_STOP) && !replaying)
  { if (bi_tables(cdp, tbdf->tbnum, attrib, &tb2, &attrib2)) //return FALSE;
       /* used when redefining multibipointer into singlebipointer */
      { if (!(dt=fix_attr_ind_write(cdp, tbdf, recnum, attrib, index, &fcbn))) return TRUE; }
    else
    { unlock_tabdescr(tb2);
       /* must check the bi-table before fixing index! */
      if (tb_read_ind_num(cdp, tbdf, recnum, attrib, &num)) return TRUE;
      if (!(dt=fix_attr_ind_write(cdp, tbdf, recnum, attrib, index, &fcbn))) return TRUE;
      if (index>=num) *(trecnum*)dt=NORECNUM;  /* the newly created multiattribute value may not be initialized properly! */
      if (tb_biptr_write(cdp, tbdf->tbnum, recnum, attrib, *(trecnum*)dt, *(const trecnum*)ptr))
        return FALSE;
    }
  }
  else if (!(dt=fix_attr_ind_write(cdp, tbdf, recnum, attrib, index, &fcbn))) return TRUE;
 /* normal write operation */
  if (cdp->replication_change_detector && !*cdp->replication_change_detector)  // no change so far, must check for a change
    if (memcmp(dt, ptr, attsz)) *cdp->replication_change_detector=true;
  memcpy(dt, ptr, attsz);
  UNFIX_PAGE(cdp, fcbn);
  if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
  { worm_add(cdp, &cdp->jourworm, &opcode_write     ,1                );
    worm_add(cdp, &cdp->jourworm, &tbdf->tbnum      ,sizeof(ttablenum));
    worm_add(cdp, &cdp->jourworm, &recnum           ,sizeof(trecnum)  );
    worm_add(cdp, &cdp->jourworm, &attrib           ,sizeof(tattrib)  );
    worm_add(cdp, &cdp->jourworm, &mod_ind          ,1                );
    worm_add(cdp, &cdp->jourworm, &index            ,sizeof(t_mult_size));
    worm_add(cdp, &cdp->jourworm, &mod_stop         ,1                );
    worm_add(cdp, &cdp->jourworm, ptr              ,attsz            );
  }
  return FALSE;
}

BOOL tb_write_ind(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, const void * ptr)
{ const attribdef * att = tbdf->attrs + attrib;
  if (tb_write_ind_pure(cdp, tbdf, recnum, attrib, index, ptr)) return TRUE;
 // constrains, change log:
  register_change(cdp, tbdf, recnum, attrib);
  if (tbdf->zcr_attr!=NOATTRIB)
    if (writing_to_replicable_table2(cdp, tbdf, recnum, attrib, NULL, true)) return TRUE;
  if (tbdf->is_traced) write_trace(cdp, tbdf, recnum, attrib, NULL, 0);
  if (att->signat!=NOATTRIB)
    if (!IS_SYSTEM_COLUMN(att->name))
      invalidate_signature(cdp, tbdf, recnum, att->signat);
  return FALSE;
}

void var_jour(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attrib, t_mult_size index,
              uns32 start, uns32 size, const void * data)
/* Writes journal record about variable-size write operation. */
{ worm_add(cdp, &cdp->jourworm,&opcode_write     ,1                );
  worm_add(cdp, &cdp->jourworm,&tb               ,sizeof(ttablenum));
  worm_add(cdp, &cdp->jourworm,&recnum           ,sizeof(trecnum)  );
  worm_add(cdp, &cdp->jourworm,&attrib           ,sizeof(tattrib)  );
  if (index!=NOINDEX)
  { worm_add(cdp, &cdp->jourworm,&mod_ind          ,1                );
    worm_add(cdp, &cdp->jourworm,&index            ,sizeof(t_mult_size));
  }
  worm_add(cdp, &cdp->jourworm,&mod_int          ,1                );
  worm_add(cdp, &cdp->jourworm,&start            ,sizeof(uns32)    );
  worm_add(cdp, &cdp->jourworm,&size             ,sizeof(uns32)    );
  worm_add(cdp, &cdp->jourworm,&mod_stop         ,1                );
  worm_add(cdp, &cdp->jourworm,data              ,size             );
}

BOOL tb_write_var(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, uns32 start, uns32 size, const void * ptr)
{ tfcbnum fcbn;  heapacc * tmphp;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  const attribdef * att = tbdf->attrs + attrib;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_WRITE, ptr, size);
  if (cdp->trace_enabled & TRACE_WRITE)
  { char buf[81];
    trace_msg_general(cdp, TRACE_WRITE, form_message(buf, sizeof(buf), msg_trace_write), tbdf->tbnum); // detailed info is in the kontext
  }
#ifdef STOP  // protected by privileges in WB 8.0
  if (tbdf->tbnum==OBJ_TABLENUM)
  { tcateg cat;
    tb_read_atr(cdp, objtab_descr, recnum, OBJ_CATEG_ATR, (tptr)&cat);
    if (cat==CATEG_TRIGGER)
    { WBUUID uuid;
      tb_read_atr(cdp, objtab_descr, recnum, APPL_ID_ATR, (tptr)uuid);
      if (!cdp->prvs.am_I_appl_admin(cdp, uuid))
        { request_error(cdp, NO_RIGHT);  return TRUE; }
    }
  }
#endif
  if (IS_HEAP_TYPE(att->attrtype))
  { if (tbdf->zcr_attr!=NOATTRIB)
      tmphp=(heapacc*)fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn);
    else
      tmphp=(heapacc*)fix_attr_read (cdp, tbdf, recnum, attrib, &fcbn);
    if (!tmphp) return TRUE;
    if (start+size > tmphp->len)
    { if (tbdf->zcr_attr==NOATTRIB)  // having the private page
      { UNFIX_PAGE(cdp, fcbn);
        if (!(tmphp=(heapacc*)fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn)))
          return TRUE;
      }
      tmphp->len=start+size;  /* new length */
    }
    else if (wait_record_lock_error(cdp, (table_descr *)tbdf, recnum, TMPWLOCK))  /* not locked by fix_attr! */
      { unfix_page(cdp, fcbn);  return TRUE; }
    if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
      var_jour(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, start, size, ptr);
    if (hp_write(cdp, &tmphp->pc, 1, start, size, (const char*)ptr, table_translog(cdp, tbdf)))
      { unfix_page(cdp, fcbn);  return TRUE; }
   }
   else if (IS_EXTLOB_TYPE(att->attrtype))  
   { uns64 * dt = (uns64*)fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn);
     if (!dt) return TRUE;
     if (write_ext_lob(cdp, dt, start, size, (const char*)ptr, false, NULL))
       { unfix_page(cdp, fcbn);  return TRUE; }
   }  
 // token, ZCR, LUO:
  if (tbdf->zcr_attr!=NOATTRIB)  // ZCR update must be before inditem_add!
    if (!(attrib==OBJ_DEF_ATR && tbdf->tbnum==OBJ_TABLENUM && cdp==cds[0]))
    // Deadlock when allocating new sequence values in cds[0] context, OBJTAB indicies are locked and updating the ZCR index
    // Must not write ZCR if cannot update the index.
      if (writing_to_replicable_table2(cdp, tbdf, recnum, attrib, (tptr)tmphp, true))
        { unfix_page(cdp, fcbn);  return TRUE; }

  UNFIX_PAGE(cdp, fcbn);
  if (tbdf->is_traced) write_trace(cdp, tbdf, recnum, attrib, NULL, 0);
  if (att->signat!=NOATTRIB)
    if (!IS_SYSTEM_COLUMN(att->name))
      invalidate_signature(cdp, tbdf, recnum, att->signat);
 // constrains, change log:
  register_change(cdp, tbdf, recnum, attrib);
  return FALSE;
}

static BOOL tb_write_ind_var_pure(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 start, uns32 size, const void * ptr)
{ tfcbnum fcbn;  heapacc * tmphp;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, index, OP_WRITE, ptr, size);
  if (cdp->trace_enabled & TRACE_WRITE)
  { char buf[81];
    trace_msg_general(cdp, TRACE_WRITE, form_message(buf, sizeof(buf), msg_trace_write), tbdf->tbnum); // detailed info is in the kontext
  }
  if (!(tmphp=(heapacc*)fix_attr_ind_write(cdp, tbdf, recnum, attrib, index, &fcbn)))
    return TRUE;
  if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
    var_jour(cdp, tbdf->tbnum, recnum, attrib, index, start, size, ptr);
  if (start+size > tmphp->len)
  { if (cdp->replication_change_detector) *cdp->replication_change_detector=true;
    tmphp->len=start+size;  /* new length */
  }
  if (hp_write(cdp, &tmphp->pc, 1, start, size, (const char*)ptr, table_translog(cdp, tbdf)))
    { unfix_page(cdp, fcbn);  return TRUE; }
  UNFIX_PAGE(cdp, fcbn);
  return FALSE;
}

BOOL tb_write_ind_var(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 start, uns32 size, const void * ptr)
{ const attribdef * att = tbdf->attrs + attrib;
  if (!tb_write_ind_var_pure(cdp, tbdf, recnum, attrib, index, start, size, ptr)) return TRUE;
 // constrains, change log:
  register_change(cdp, tbdf, recnum, attrib);
  if (tbdf->zcr_attr!=NOATTRIB)
    if (writing_to_replicable_table2(cdp, tbdf, recnum, attrib, NULL, true)) return TRUE;
  if (tbdf->is_traced) write_trace(cdp, tbdf, recnum, attrib, NULL, 0);
  if (att->signat!=NOATTRIB)
    if (!IS_SYSTEM_COLUMN(att->name))
      invalidate_signature(cdp, tbdf, recnum, att->signat);
  return FALSE;
}

BOOL tb_write_atr_len(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, uns32 len)
{ tfcbnum fcbn;  heapacc * tmphp;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  const attribdef * att=tbdf->attrs + attrib;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_WRITE, NULL, len);
  if (cdp->trace_enabled & TRACE_WRITE)
  { char buf[81];
    trace_msg_general(cdp, TRACE_WRITE, form_message(buf, sizeof(buf), msg_trace_write), tbdf->tbnum); // detailed info is in the kontext
  }
  if (!(tmphp=(heapacc*)fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn)))
    return TRUE;
    
  if (IS_HEAP_TYPE(att->attrtype))
  { if (len < tmphp->len)  /* no action if the real size is smaller */
    { hp_shrink_len(cdp, &tmphp->pc, 1, len, table_translog(cdp, tbdf));
      tmphp->len=len;
    }
  }  
  else if (IS_EXTLOB_TYPE(att->attrtype))
  { if (!*(uns64*)tmphp) return FALSE;  // no action
    if (!len)
      write_ext_lob(cdp, (uns64*)tmphp, 0, 0, NULL, true, NULL);
    else
      ext_lob_set_length(cdp, *(uns64*)tmphp, len);
  }
  
  if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
  { worm_add(cdp, &cdp->jourworm,&opcode_write     ,1                );
    worm_add(cdp, &cdp->jourworm,&tbdf->tbnum      ,sizeof(ttablenum));
    worm_add(cdp, &cdp->jourworm,&recnum           ,sizeof(trecnum)  );
    worm_add(cdp, &cdp->jourworm,&attrib           ,sizeof(tattrib)  );
    worm_add(cdp, &cdp->jourworm,&mod_len          ,1                );
    worm_add(cdp, &cdp->jourworm,&mod_stop         ,1                );
    worm_add(cdp, &cdp->jourworm,&len              ,sizeof(uns32)    );
  }
  if (tbdf->zcr_attr!=NOATTRIB)
    if (writing_to_replicable_table2(cdp, tbdf, recnum, attrib, (tptr)tmphp, true)) return TRUE;
  UNFIX_PAGE(cdp, fcbn);
 // constrains, change log:
  register_change(cdp, tbdf, recnum, attrib);
  if (tbdf->is_traced) write_trace(cdp, tbdf, recnum, attrib, NULL, 0);
  if (att->signat!=NOATTRIB)
    if (!IS_SYSTEM_COLUMN(att->name))
      invalidate_signature(cdp, tbdf, recnum, att->signat);
  return FALSE;
}

BOOL tb_write_ind_len(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size index, uns32 len)
{ tfcbnum fcbn;  heapacc * tmphp;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  const attribdef * att = tbdf->attrs + attrib;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, index, OP_WRITE, NULL, len);
  if (cdp->trace_enabled & TRACE_WRITE)
  { char buf[81];
    trace_msg_general(cdp, TRACE_WRITE, form_message(buf, sizeof(buf), msg_trace_write), tbdf->tbnum); // detailed info is in the kontext
  }
  if (!(tmphp=(heapacc*)fix_attr_ind_write(cdp, tbdf, recnum, attrib, index, &fcbn)))
    return TRUE;
  if (len < tmphp->len)  /* no action if the real size is smaller */
  { hp_shrink_len(cdp, &tmphp->pc, 1, len, table_translog(cdp, tbdf));
    tmphp->len=len;
  }
  UNFIX_PAGE(cdp, fcbn);
 // constrains, change log:
  register_change(cdp, tbdf, recnum, attrib);
  if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
  { worm_add(cdp, &cdp->jourworm,&opcode_write     ,1                );
    worm_add(cdp, &cdp->jourworm,&tbdf->tbnum      ,sizeof(ttablenum));
    worm_add(cdp, &cdp->jourworm,&recnum           ,sizeof(trecnum)  );
    worm_add(cdp, &cdp->jourworm,&attrib           ,sizeof(tattrib)  );
    worm_add(cdp, &cdp->jourworm,&mod_ind          ,1                );
    worm_add(cdp, &cdp->jourworm,&index            ,sizeof(t_mult_size));
    worm_add(cdp, &cdp->jourworm,&mod_len          ,1                );
    worm_add(cdp, &cdp->jourworm,&mod_stop         ,1                );
    worm_add(cdp, &cdp->jourworm,&len              ,sizeof(uns32)    );
  }
  if (tbdf->zcr_attr!=NOATTRIB)
    if (writing_to_replicable_table2(cdp, tbdf, recnum, attrib, NULL, true)) return TRUE;
  if (tbdf->is_traced) write_trace(cdp, tbdf, recnum, attrib, NULL, 0);
  if (att->signat!=NOATTRIB)
    if (!IS_SYSTEM_COLUMN(att->name))
      invalidate_signature(cdp, tbdf, recnum, att->signat);
  return FALSE;
}


BOOL tb_write_ind_num(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attrib, t_mult_size num)
{ tfcbnum fcbn;  tptr dt;  t_mult_size oldnum;
#ifdef CHECK_TBLOCK
  if (!tbdf->deflock_counter) { SET_ERR_MSG(cdp, tbdf->selfname);  err_msg(cdp, IE_DOUBLE_PAGE); }
#endif
  const attribdef * att=tbdf->attrs + attrib;
  t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attrib, NOINDEX, OP_WRITE, NULL, num);
  if (cdp->trace_enabled & TRACE_WRITE)
  { char buf[81];
    trace_msg_general(cdp, TRACE_WRITE, form_message(buf, sizeof(buf), msg_trace_write), tbdf->tbnum); // detailed info is in the kontext
  }
  { const char * dtc;
    if (!(dtc=fix_attr_read(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE;
    oldnum=*(const t_mult_size*)dtc;
    if (oldnum <= num)
      { unfix_page(cdp, fcbn);  return FALSE;  /* no action */ }
    UNFIX_PAGE(cdp, fcbn);
  }
 /* special actions for deleting the values above the new limit */
  if (IS_HEAP_TYPE(att->attrtype))  /* must deallocate things */
  { uns32 saved_ker_flags = cdp->ker_flags;
    cdp->ker_flags |= KFL_NO_JOUR;  /* do not write this to the journal */
    for (t_mult_size index=num; index < oldnum; index++)
      tb_write_ind_len(cdp, tbdf, recnum, attrib, index, 0);
    cdp->ker_flags=saved_ker_flags;   /* restore! */
  }
  else if ((att->attrtype == ATT_BIPTR) && !(cdp->ker_flags & KFL_BI_STOP) && !replaying)
  { for (t_mult_size index=num; index < oldnum; index++)
    { if (!(dt=fix_attr_ind_write(cdp, tbdf, recnum, attrib, index, &fcbn)))
        return TRUE;
      tb_biptr_write(cdp, tbdf->tbnum, recnum, attrib, *(trecnum*)dt, NORECNUM);
      unfix_page(cdp, fcbn);
    }
  }
 /* writing the new number of multiattribute values */
  if (!(dt=fix_attr_write(cdp, tbdf, recnum, attrib, &fcbn))) return TRUE;
  *(t_mult_size*)dt=num;
  UNFIX_PAGE(cdp, fcbn);
 // constrains, change log:
  register_change(cdp, tbdf, recnum, attrib);
  if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
  { worm_add(cdp, &cdp->jourworm,&opcode_write     ,1                );
    worm_add(cdp, &cdp->jourworm,&tbdf->tbnum      ,sizeof(ttablenum));
    worm_add(cdp, &cdp->jourworm,&recnum           ,sizeof(trecnum)  );
    worm_add(cdp, &cdp->jourworm,&attrib           ,sizeof(tattrib)  );
    worm_add(cdp, &cdp->jourworm,&mod_len          ,1                );
    worm_add(cdp, &cdp->jourworm,&mod_stop         ,1                );
    worm_add(cdp, &cdp->jourworm,&num              ,sizeof(t_mult_size));
  }
  if (tbdf->zcr_attr!=NOATTRIB)
    if (writing_to_replicable_table2(cdp, tbdf, recnum, attrib, NULL, true)) return TRUE;
  if (tbdf->is_traced) write_trace(cdp, tbdf, recnum, attrib, NULL, 0);
  if (att->signat!=NOATTRIB) invalidate_signature(cdp, tbdf, recnum, att->signat);
  return FALSE;
}

/************************** various *****************************************/
void copy_var_len(cdp_t cdp, table_descr * tbdf1, trecnum r1, tattrib a1, t_mult_size ind1,
                             table_descr * tbdf2, trecnum r2, tattrib a2, t_mult_size ind2,
                             elem_descr * attracc)
/* Can be used for installed TABLES only, not for cursors */
/* Used for copying attribute to attribute, incl. multiattributes */
/* Can convert string to var-len attr & vice versa */
{ const char * dt;  tfcbnum fcbn1, fcbn2;
  uns8 tp, tp2, ml1, ml2;
  uns16 i, counter;  int sz2;  char buf[2+256];  char * pbuf;  t_value exprval;
  heapacc * tmphp;  BOOL add_null, multimove;
  tp =tbdf1->attrs[a1].attrtype;   if (tp==ATT_BIPTR) tp=ATT_PTR;
  ml1=tbdf1->attrs[a1].attrmult;
#ifdef STOP  // to uz je asi stare
  if (t2==(ttablenum)-100)   /* write ODBC parameter value */
  { if (!IS_HEAP_TYPE(tp) || ml1 != 1) return;
    WBPARAM * wbpar = (WBPARAM*)r2;
    uns32 len, offset;  unsigned step;  char * hptr;  tptr ptr;
    len=wbpar->len;  hptr=(char *)wbpar->val;
    offset=0;
    while (len)
    { step=len > 0x2000 ? 0x2000 : (unsigned)len;
      ptr=(tptr)hptr;  /* must use separate pointer, tb_write_ cannot update huge pointer properly */
      if (ind1==NOINDEX)
      { if (tb_write_var(cdp, tbdf1, r1, a1, offset, step, &ptr, STD_OPT))
          break;
      }
      else
      { if (tb_write_ind_var(cdp, tbdf1, r1, a1, ind1, offset, step, &ptr, STD_OPT))
          break;
      }
      hptr+=step;  offset+=step;  len-=step;
    }
    if (ind1==NOINDEX)
      tb_write_atr_len(cdp, tbdf1, r1, a1, wbpar->len);
    else
      tb_write_ind_len(cdp, tbdf1, r1, a1, ind1, wbpar->len);
    return;
  }
#endif
  if (attracc==NULL)
  { tp2 =tbdf2->attrs[a2].attrtype;  if (tp2==ATT_BIPTR) tp2=ATT_PTR;
    ml2 =tbdf2->attrs[a2].attrmult;
    sz2 =tbdf2->attrs[a2].attrspecif.length;
  }
  else // is positioned, can directly evaluate
  { tp2 = attracc->type;
    ml2 = 1;//attracc->multi;
    sz2 = attracc->specif.length;
    expr_evaluate(cdp, attracc->expr.eval, &exprval);
    if (exprval.is_null()) qlset_null(&exprval, tp2, sz2);
  }

  if (tp != tp2)
    if ((!is_string(tp)  || !is_string(tp2) && !IS_HEAP_TYPE(tp2)) &&
        (!is_string(tp2) || !is_string(tp ) && !IS_HEAP_TYPE(tp)) &&
        (tp !=ATT_RASTER || tp2!=ATT_NOSPEC) &&
        (tp2!=ATT_RASTER || tp !=ATT_NOSPEC))
    return; /* uncompatible */
  if ((tp>=ATT_FIRSTSPEC) && (tp<=ATT_LASTSPEC))
    if (!(cdp->ker_flags & KFL_ALLOW_TRACK_WRITE))
      return;   /* write to tracking attributes not allowed */

  add_null=FALSE;
  if (is_string(tp2))
    if (is_string(tp))
      if (tbdf1->attrs[a1].attrspecif.length > sz2) add_null=TRUE;
 /* moving all items of a multiattribute */
  multimove=(ml1 > 1) && (ml2 > 1) && (ind1==NOINDEX) && (ind2==NOINDEX);
  if (multimove)
  { if (fast_table_read(cdp, tbdf2,r2,a2,&counter)) return;
    if (!(ml1 & 0x80))  /* transfer only a part of items */
      if (counter > ml1) counter=ml1;
  }
  else
  { counter=1;
    if (ml2 > 1 && ind2==NOINDEX) return;   /* multiattr -> attr */
    if (ml1 > 1 && ind1==NOINDEX) ind1=0;   /* attr -> multiattr: to 1st item */
  }
 /* copying */
  for (i=0; i<counter; i++)
  { if (attracc==NULL)
    { if (ml2 > 1)  /* source multiattribute */
      { if (multimove) ind2=i;
        dt=fix_attr_ind_read(cdp, tbdf2, r2, a2, ind2, &fcbn2);
      }
      else dt=fix_attr_read(cdp, tbdf2, r2, a2, &fcbn2);
      if (!dt) return;
      if (add_null)
        { memcpy(buf, dt, sz2);  buf[sz2]=0;  dt=buf; }
      pbuf=(tptr)dt;
    }
    else
      pbuf=exprval.strval;
    if (!IS_HEAP_TYPE(tp))  /* destination is simple */
    { if (IS_HEAP_TYPE(tp2))
      { memset(buf, 0, tbdf1->attrs[a1].attrspecif.length);   /* delimiter inside */
        if (hp_read(cdp, &((heapacc*)dt)->pc, 1, 0, tbdf1->attrs[a1].attrspecif.length, buf))
          goto err;
        pbuf=buf;
      }
     /* write the value: */
      if (ml1 > 1)  /* destination multiattribute */
      { if (multimove) ind1=i;
        tb_write_ind(cdp, tbdf1, r1, a1, ind1, pbuf);
      }
      else tb_write_atr(cdp, tbdf1, r1, a1, pbuf);
    }
    else  /* variable-size destination */
    { if (ml1 > 1)  /* destination multiattribute */
      { if (multimove) ind1=i;
        tmphp=(heapacc*)fix_attr_ind_write(cdp, tbdf1, r1, a1, ind1, &fcbn1);
      }
      else tmphp=(heapacc *)fix_attr_write(cdp, tbdf1, r1, a1, &fcbn1);
      if (tmphp)
      { if (IS_HEAP_TYPE(tp2))
        { hp_copy(cdp, &((heapacc*)dt)->pc, &tmphp->pc, ((heapacc*)dt)->len,
              tbdf1->tbnum, r1, a1, (ml1>0) ? ind1 : NOINDEX, table_translog(cdp, tbdf1)); /* no check necessary */
          tmphp->len=((heapacc*)dt)->len;
        }
        else  /* convert string -> var len */
        { sz2=strmaxlen(dt, sz2);  /* write the actual size, not maximal */
          hp_write(cdp, &tmphp->pc, 1, 0, sz2, pbuf, table_translog(cdp, tbdf1));
        }
        unfix_page(cdp, fcbn1);
      }
      register_change(cdp, tbdf1, r1, a1);  // using non-standard write, must register explicitly
    }
   err:
    UNFIX_PAGE(cdp, fcbn2);
  }
}
///////////////////////////////////////////////////////////////////
t_exkont * lock_and_get_execution_kontext(cdp_t acdp)
// Prevents dropping any segment of [execution_kontext] by its owner.
{ ProtectedByCriticalSection cs(&cs_short_term);  
  acdp->execution_kontext_locked++;  // prevents the context (if any) to be dropped
  return acdp->execution_kontext;
}  

void unlock_execution_kontext(cdp_t acdp)
{ ProtectedByCriticalSection cs(&cs_short_term);   // now it protects the access to the [execution_kontext_locked] variable only
  acdp->execution_kontext_locked--; 
}

t_exkont::t_exkont(cdp_t cdpIn, t_exkont_type typeIn) // inserts itself on the top of the list
{ cdp=cdpIn;
  type=typeIn;
  next=cdp->execution_kontext;
  cdp->execution_kontext=this;
}
t_exkont::~t_exkont(void) // removes itself from the top of the list
{// remove the context from the list: 
  cdp->execution_kontext=next; 
 // wait until the context is not locked by another client: 
  do
  { { ProtectedByCriticalSection cs(&cs_short_term);  // make changes in [execution_kontext] mutually exclusive with passing by other client
      if (!cdp->execution_kontext_locked) return;  // can drop the context now
    }  
    Sleep(100);  // other thread is passing the context list, must not destruct the object
  } while (true);  
}

t_exkont_table::t_exkont_table(cdp_t cdpIn, ttablenum tbnumIn, trecnum recnumIn, tattrib attrIn, t_mult_size indexIn, int operIn, const void * valIn, unsigned vallenIn) 
    : t_exkont(cdpIn, EKT_TABLE)
{ tbnum=tbnumIn;  recnum=recnumIn;  attr=attrIn;  index=indexIn;  oper=operIn;  val=valIn;  vallength=vallenIn; }

void t_exkont_table::get_descr(cdp_t cdp, char * buf, int buflen)
{ char mybuf[20+OBJ_NAME_LEN+ATTRNAMELEN+100+40];  
  tptr p;
  switch (oper)
  { case OP_READ :    p="[Reading from";    break;
    case OP_WRITE:    p="[Writing to";      break;
    case OP_INSERT:   p="[Inserting to";    break;
    case OP_DELETE:   p="[Deleting in";     break;
    case OP_UNDEL:    p="[Undeleting in";   break;
    case OP_PTRSTEPS: p="[Referenced from"; break;
    case OP_PRIVILS:  p="[Privils in";      break; 
    default:          p="[";                break;
  }
  strcpy(mybuf, p);  
  if (tbnum < -5000 && IS_CURSOR_NUM(tbnum))
  { strcat(mybuf, " cursor ");  p=mybuf+strlen(mybuf);
    int2str((uns16)tbnum, p);  p+=strlen(p);
   // like get_cursor(cdp, tbnum); but without calling request_error() -- deadlock possible
    tcursnum cursnum = GET_CURSOR_NUM(cursnum);
    if (cursnum < crs.count())
    { cur_descr * cd;
      { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
        cd = *crs.acc0(cursnum);
      }
      if (cd!=NULL && cd->owner == cdp->applnum_perm)
      { if (recnum!=NORECNUM)
        { sprintf(p, ", record %u", recnum);  
          p+=strlen(p);
        }
        if (attr!=NOATTRIB && attr<cd->qe->kont.elems.count())
        { sprintf(p, ", Column %d (%s)", attr, cd->qe->kont.elems.acc(attr)->name);
          p+=strlen(p);
        }
        if (index!=NOINDEX)
        { sprintf(p, ", Value# %d", index);
          p+=strlen(p);
        }
        if (oper==OP_WRITE && recnum!=NORECNUM && attr!=NOATTRIB && attr<cd->qe->kont.elems.count()) 
        { if (val)
          { strcpy(p, ", Value ");  p+=strlen(p);
            display_value(cdp, cd->qe->kont.elems.acc(attr)->type, cd->qe->kont.elems.acc(attr)->specif, vallength, (const char *)val, p);  
          }
          else // writing length or number of values
            sprintf(p, ", Length %u", vallength);
          p+=strlen(p);
        }
      }
    }
  }
  else
  { p=mybuf+strlen(mybuf);
    table_descr * td = access_installed_table(cdp, tbnum);  // must not call install_table because of CS ordering (deadlock risl)
    if (td)
    { sprintf(p, " table %d (%s)", tbnum, td->selfname);  p+=strlen(p);
      if (recnum!=NORECNUM)
      { sprintf(p, ", record %u", recnum);  p+=strlen(p);
        // do not add object name for records in object tables - daedlock can occur if reading object names now!
      }
      if (attr!=NOATTRIB)
      { sprintf(p, ", Column %d (%s)", attr, td->attrs[attr].name);
        p+=strlen(p);
      }
      if (index!=NOINDEX)
      { sprintf(p, ", Value# %d", index);
        p+=strlen(p);
      }
      if (oper==OP_WRITE && recnum!=NORECNUM && attr!=NOATTRIB) 
      { if (val)
        { strcpy(p, ", Value ");  p+=strlen(p);
          display_value(cdp, td->attrs[attr].attrtype, td->attrs[attr].attrspecif, vallength, (const char *)val, p);  
        }
        else // writing length or number of values
          sprintf(p, ", Length %u", vallength);
        p+=strlen(p);
      }
      unlock_tabdescr(td);
    }
    else  // table not available!
    { sprintf(p, " table %d", tbnum);  p+=strlen(p);
      if (recnum!=NORECNUM)
        { sprintf(p, ", record %u", recnum);  p+=strlen(p); }
      if (attr!=NOATTRIB)
        { sprintf(p, ", Column %d", attr);    p+=strlen(p); }
      if (index!=NOINDEX)
        { sprintf(p, ", Value# %d", index);   p+=strlen(p); }
    }
  } // cursor or table
  *(p++)=']';  *p=0;
  if (buflen>0) strmaxcpy(buf, mybuf, buflen);
}

void append_str(char * &buf, int &buflen, const char * src)
{ if (buflen<=0) return;
  strmaxcpy(buf, src, buflen);
  int cpy = (int)strlen(buf);
  buf+=cpy;  buflen-=cpy;
}

void t_exkont_record::get_descr(cdp_t cdp, char * buf, int buflen)
{ table_descr * tbdf = access_installed_table(cdp, tbnum);  // must not call install_table because of CS ordering (deadlock risl)
  append_str(buf, buflen, " [");
  if (tbdf)
  { append_str(buf, buflen, "Table ");
    append_str(buf, buflen, tbdf->selfname);
    append_str(buf, buflen, ", Record ");
    bool first = true;
    for (int a = 1;  a<tbdf->attrcnt && buflen;  a++)
    { const attribdef * att = &tbdf->attrs[a];
      if (!IS_HEAP_TYPE(att->attrtype) && att->attrmult==1)
      { if (first) first=false;
        else append_str(buf, buflen, ", ");
        append_str(buf, buflen, att->name);
        append_str(buf, buflen, "=");
        char rdbuf[MAX_FIXED_STRING_LENGTH+2];
        if (fast_table_read(cdp, tbdf, recnum, a, rdbuf)) strcpy(rdbuf, "??");
        rdbuf[MAX_FIXED_STRING_LENGTH]=rdbuf[MAX_FIXED_STRING_LENGTH+1]=0;
        char valbuf[45];
        display_value(cdp, att->attrtype, att->attrspecif, -1, rdbuf, valbuf);
        append_str(buf, buflen, valbuf);
      }
    }
    unlock_tabdescr(tbdf);
  }
  else append_str(buf, buflen, "(Table uninstalled)");
  append_str(buf, buflen, "]");
}

void t_exkont_key::get_descr(cdp_t cdp, char * buf, int buflen)
// Supposes that [idx_descr] and [key] temporary pointers are still valid!
{ append_str(buf, buflen, " [Key ");
  const part_desc * apart = idx_descr->parts;  int i = 0;  const char * pkey = key;
  do
  { char valbuf[45];
    display_value(cdp, apart->type, apart->specif, -1, pkey, valbuf);
    append_str(buf, buflen, valbuf);
    pkey+=apart->partsize;
    i++;  apart++;
    if (i>=idx_descr->partnum) break;
    append_str(buf, buflen, " : ");
  } while (true);
  append_str(buf, buflen, "]");
}

void t_exkont_index::get_descr(cdp_t cdp, char * buf, int buflen)
{ char mybuf[2*OBJ_NAME_LEN+50]; 
  table_descr * tbdf = access_installed_table(cdp, tbnum);  // must not call install_table because of CS ordering (deadlock risl)
  if (tbdf)
  { sprintf(mybuf, " [Table %i (%s), index %i (%s)]", tbnum, tbdf->selfname, indnum, tbdf->indxs[indnum].constr_name);
    unlock_tabdescr(tbdf);
  }  
  else sprintf(mybuf, " [Table %i, index %i]", tbnum, indnum);
  if (buflen>0) strmaxcpy(buf, mybuf, buflen);
}

void t_exkont_lock::get_descr(cdp_t cdp, char * buf, int buflen)
{ char mybuf[OBJ_NAME_LEN+90]; 
  strcpy(mybuf, " [Cannot set ");
  switch (locktype)
  { case WLOCK:    strcat(mybuf, "WRITE");        break;
    case TMPWLOCK: strcat(mybuf, "TEMP. WRITE");  break;
    case RLOCK:    strcat(mybuf, "READ");         break;
    case TMPRLOCK: strcat(mybuf, "TEMP. READ");   break;
  }
  strcat(mybuf, " lock on ");
  if (tbnum<0) // -1 for page lock
    sprintf(mybuf+strlen(mybuf), "page %u]", recnum);
  else
  { table_descr * tbdf = access_installed_table(cdp, tbnum);  // must not call install_table because of CS ordering (deadlock risl)
    if (tbdf)
    {  sprintf(mybuf+strlen(mybuf), "table %i (%s)", tbnum, tbdf->selfname);
       unlock_tabdescr(tbdf);
    }  
    else sprintf(mybuf+strlen(mybuf), "table %i", tbnum);
    if (recnum!=NORECNUM) sprintf(mybuf+strlen(mybuf), " record %u", recnum);
    sprintf(mybuf+strlen(mybuf), " conflicting with client %d", usernum);
    strcat(mybuf, "]");
  }
  if (buflen>0) strmaxcpy(buf, mybuf, buflen);
}

///////////////////////////////// old record operations //////////////////////////
void read_record(cdp_t cdp, BOOL std, table_descr * tbdf, trecnum recnum, cur_descr * cd, tptr & request)
{ unsigned sz;  int i;  uns8 tp, multi;  t_privils_flex priv_val;
  unsigned offs, pgrec;  tattrib attr;  elem_descr * attracc;
  int curr_page, curr_tb;  trecnum curr_rec;  tfcbnum fcbn;  const char * dt, * pval;  ttablenum tbnum;
  int attrnum;  bool has_privil;  t_value exprval;
  uns32 size=*(uns32*)request;  request+=sizeof(uns32);
  tptr p=cdp->get_ans_ptr(size);
  if (p==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
  memset(p, 0, size);
  if (std)
  { attrnum=tbdf->attrcnt;
    cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
    if (cdp->isolation_level >= REPEATABLE_READ)
      if (!SYSTEM_TABLE(tbdf->tbnum))
        if (wait_record_lock_error(cdp, tbdf, recnum, TMPRLOCK)) return;
  }
  //else if (cd->has_own_table)
  //{ tbdf=tables[cd->own_table];  attrnum=tbdf->attrcnt;
  //  has_privil=TRUE;
  //}
  else 
  { attrnum=cd->d_curs->attrcnt;
    if (cdp->isolation_level >= REPEATABLE_READ)
      if (cursor_locking(cdp, OP_READ, cd, recnum)) return;
    cd->position=recnum;  // used by ODBC, SQLFetch defines CURRENT OF in this way
  }
  curr_tb=-1;  curr_page=-1;  /* table & page described by dt, fcbn */
  for (i=1;  i<attrnum;  i++)  /* cycle on attributes */
  { if (std)
    { attr=i;
      has_privil=priv_val.has_read(attr);
      attracc=NULL;
    }
    else
    { attr_access(&cd->qe->kont, i, tbnum, attr, recnum, attracc);
      has_privil=true;
      if (!attracc) 
      { tbdf=tables[tbnum];
        if (!cd->insensitive)  /* check cursor access rights, unless checked when filling own table */
          if (!(cd->d_curs->attribs[i].a_flags & RI_SELECT_FLAG))
            if (!(cd->d_curs->tabdef_flags & TFL_REC_PRIVILS))
              has_privil=false;
            else
            { cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
              if (!priv_val.has_read(attr)) has_privil=false;
            }
      }
    }
    bool wide;

    if (attracc!=NULL)
    { tp=attracc->type;  multi=attracc->multi;
      sz=SPECIFLEN(tp) ? attracc->specif.length : tpsize[tp];
      wide=attracc->specif.wide_char!=0;
      prof_time start;
      if (cd->source_copy) start = get_prof_time();
      exprval.set_null();
      { t_temporary_current_cursor tcc(cdp, cd);  // sets the temporary current_cursor in the cdp until destructed
        expr_evaluate(cdp, attracc->expr.eval, &exprval);
      }
      if (exprval.is_null()) qlset_null(&exprval, tp, sz);
      if (PROFILING_ON(cdp) && cd->source_copy) add_prof_time(get_prof_time()-start, PROFCLASS_SELECT, 0, 0, cd->source_copy);
      pval=exprval.valptr();
    }
    else
    { attribdef * att=tbdf->attrs+attr;
      sz=TYPESIZE(att);  tp=att->attrtype;  multi=att->attrmult;  wide=att->attrspecif.wide_char!=0;
      if (recnum==OUTER_JOIN_NULL_POSITION)  // NULL part of the outer join result
      { exprval.set_simple_not_null();  // otherwiase valptr() is undefined
        qlset_null(&exprval, tp, sz);
        pval=exprval.valptr();
      }
      else  // read from the table
      { if (tbnum!=curr_tb || att->attrpage!=curr_page || recnum!=curr_rec)
        /* change fcbn, dt*/
        { if (curr_page!=-1) UNFIX_PAGE(cdp, fcbn);
          if (recnum >= tbdf->Recnum())
            { request_error(cdp, RECORD_DOES_NOT_EXIST);  return; }
          tblocknum datadadr = ACCESS_COL_VAL(cdp, tbdf, recnum, att->attrpage, offs, pgrec);
          if (!datadadr) return;  // error detected in ACCESS_COL_VAL
          dt=fix_any_page(cdp, datadadr, &fcbn);
          if (!dt) return;
          curr_page=att->attrpage;  curr_tb=tbnum;  curr_rec=recnum;
#if WBVERS>=810
          if (!curr_page)
            if (dt[offs]==DELETED)
              warn(cdp, WORKING_WITH_DELETED_RECORD);
#endif
        }
        pval=dt+offs+att->attroffset;
      }
    }

    if (multi==1 && !IS_HEAP_TYPE(tp) && !IS_EXTLOB_TYPE(tp))
      if (IS_STRING(tp))
        if (wide)
        { if (sz+2 > size) break;  /* size limit */
          if (has_privil) memcpy(p, pval, sz);  else { *p=0;  p[1]=0; }
          p+=sz;  *(p++)=0;  *(p++)=0;  size-=sz+2;
        }
        else
        { if (sz+1 > size) break;  /* size limit */
          if (has_privil) memcpy(p, pval, sz);  else *p=0;
          p+=sz;  *(p++)=0;  size-=sz+1;
        }
      else
      { if (sz > size) break;  /* size limit */
        if (has_privil) memcpy(p, pval, sz);  else qset_null(p, tp, sz);
        p+=sz;             size-=sz;
      }
  } /* end of attribute cycle */
  if (curr_page!=-1) UNFIX_PAGE(cdp, fcbn);
}

void write_record(cdp_t cdp, BOOL std, table_descr * tbdf, trecnum recnum, cur_descr * cd, tptr & request)
{ uns16 sz;  int i;  uns8 tp;  t_privils_flex priv_val;
  unsigned offs, pgrec;  tattrib attr;  elem_descr * attracc;
  int curr_page, curr_tb;  trecnum curr_rec;  tfcbnum fcbn;  tptr dt;
  int attrnum;  bool has_privil;  attribdef * att;
  t_value exprval; ttablenum tbnum;
  uns32 size=*(uns32*)request;  request+=sizeof(uns32);
  if (std)
  { attrnum=tbdf->attrcnt;
    cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
    if (wait_record_lock_error(cdp, tbdf, recnum, TMPWLOCK)) return;
  }
  else 
  { if (!(cd->qe->qe_oper & QE_IS_UPDATABLE))
      { request_error(cdp, CANNOT_UPDATE_IN_THIS_VIEW);  return; }
    attrnum=cd->d_curs->attrcnt;
    if (cursor_locking(cdp, OP_WRITE, cd, recnum)) return;
  }
  //{ tbdf=tables[cd->own_table];  attrnum=tbdf->attrcnt; }
  curr_tb=-1;  curr_page=-1; /* table & page described by dt, fcbn */
  for (i=1;  i<attrnum;  i++)  /* cycle on attributes */
  { if (std)
    { attr=i;
      att=tbdf->attrs+attr;
      has_privil=priv_val.has_write(attr);
    }
    else // cursor record write
    { attr_access(&cd->qe->kont, i, tbnum, attr, recnum, attracc);
      if (attracc!=NULL) // cannot write, skip the value (no error)
      { tp=attracc->type;
        if (attracc->multi!=1 || IS_HEAP_TYPE(tp)) continue;
       // skip the value:
        sz = IS_STRING(tp) ? attracc->specif.length+1 :
             tp==ATT_BINARY ? attracc->specif.length : tpsize[tp];
        if (sz > size) break;  /* size limit */
        request+=sz;  size-=sz;
        continue;
      }
      else  // maped to a table attribute
      { tbdf=tables[tbnum];
        att=tbdf->attrs+attr;
        if (cd->d_curs->attribs[i].a_flags & RI_UPDATE_FLAG)
          has_privil=true;
        else if (tbdf->tabdef_flags & TFL_REC_PRIVILS)
        { cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
          has_privil=priv_val.has_write(attr);
        }
        else has_privil=false;
      }
    }
    t_translog * translog = table_translog(cdp, tbdf);

    if (tbnum!=curr_tb || att->attrpage!=curr_page || curr_rec!=recnum)
    /* change fcbn, dt*/
    { if (curr_page!=-1) UNFIX_PAGE(cdp, fcbn);
      if (recnum >= tbdf->Recnum())
        { request_error(cdp, RECORD_DOES_NOT_EXIST);  return; }
      tblocknum datadadr = ACCESS_COL_VAL(cdp, tbdf, recnum, att->attrpage, offs, pgrec);
      if (!datadadr) return;  // error detected in ACCESS_COL_VAL
      dt=fix_priv_page(cdp, datadadr, &fcbn, translog);
      if (!dt) return;
      translog->log.rec_changed(fcbn, datadadr, tbdf->recordsize, pgrec);
      curr_page=att->attrpage;  curr_tb=tbnum;  curr_rec=recnum;
      register_change(cdp, tbdf, recnum, NOATTRIB); // for delayed checking of constrains
      if (!curr_page)
      { if (dt[offs]==RECORD_EMPTY)  // error, calling from API only
           { request_error(cdp, EMPTY);  unfix_page(cdp, fcbn);  return; }      
#if WBVERS>=810
        if (dt[offs]==DELETED)
          warn(cdp, WORKING_WITH_DELETED_RECORD);
#endif
      }
    }
    sz=TYPESIZE(att);  tp=att->attrtype;
    unsigned extsz=sz;
    if (IS_STRING(tp)) 
      if (att->attrspecif.wide_char) extsz+=2;  
      else                           extsz++;
    if (att->attrmult==1 && !IS_HEAP_TYPE(tp))
    { if (extsz > size) break;  /* size limit */
      if (attr && (tp!=ATT_AUTOR) && (tp!=ATT_DATIM))
      /* no write on DELETED attrib and AUTOR-DATIM */
      { t_simple_update_context sic(cdp, tbdf, attr);
        if (att->attrspec2 & IS_IN_INDEX && has_privil)
          if (!sic.lock_roots()) break;
            if (!sic.inditem_remove(recnum, request))
              break;
        if (has_privil)
        { memcpy(dt+offs+att->attroffset, request, sz);
          if (the_sp.WriteJournal.val() && !(tbdf->tabdef_flags & TFL_NO_JOUR) && !(cdp->ker_flags & KFL_NO_JOUR))
          { worm_add(cdp, &cdp->jourworm,&opcode_write     ,1                );
            worm_add(cdp, &cdp->jourworm,&tbnum            ,sizeof(ttablenum));
            worm_add(cdp, &cdp->jourworm,&recnum           ,sizeof(trecnum)  );
            worm_add(cdp, &cdp->jourworm,&attr             ,sizeof(tattrib)  );
            worm_add(cdp, &cdp->jourworm,&mod_stop         ,1                );
            worm_add(cdp, &cdp->jourworm,request           ,sz               );
          }
        }

        if (att->attrspec2 & IS_IN_INDEX && has_privil)
          sic.inditem_add(recnum); /* add record into indexes */
      }
      // else skip the attribute value 
      request+=extsz;  size-=extsz;
    }
  } /* end of attribute cycle */
  if (curr_page!=-1) UNFIX_PAGE(cdp, fcbn);
  request+=size;  // masks error if sent too much
}

