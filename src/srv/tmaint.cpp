/****************************************************************************/
/* tmaint.c - table maintenance                                             */
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
#ifndef WINS
#include <stdlib.h>
#endif
#include "wbsecur.h"

#ifndef WINS
struct SYSTEMTIME{
	int wHour, wMinute, wSecond, wMilliseconds;
	int wDay, wMonth, wYear;
};
#endif

#define OLD_TDT_VERSNUM (256*1+1)
#define TDT_VERSNUM_4   (256*2+0)
#define TDT_VERSNUM     (256*5+0)
#define TDT_EXTLOBREFS  (256*5+1)

#define RDSTEP        467  /* must not be bigger than packet contents */
/* Otherwise in NB mutiple packets may be processed by client in export
   causing freezing. */
#define FILE_BUF_SIZE 600  /* must be >= rdstep !! */

static tptr get_data(cdp_t cdp, unsigned size, tptr buf, unsigned * bufpos, unsigned * bufend)
/* returns ptr to the next "size" bytes from the file or NULL on eof */
/* size must be less than or equal to RDSTEP */
{ tptr ptr;  int rd;
  if (*bufpos+size > *bufend)
  { memmov(buf, buf+*bufpos, *bufend-*bufpos);
    *bufend-=*bufpos;  *bufpos=0;
    unsigned tord = FILE_BUF_SIZE-*bufend;
    if (tord > RDSTEP) tord = RDSTEP;
    rd = cdp->read_from_client(buf+*bufend, tord);
    if (rd==-1) return NULL;
    *bufend+=rd;
  }
  if (*bufpos+size > *bufend) return NULL;
  ptr=buf+*bufpos; *bufpos+=size;
  return ptr;
}

/* When restoring a table, KFL_ALLOW_TRACK_WRITE and KFL_NO_TRACE options have to
        be used. Using journal is possible I hope. */

bool load_long_data(cdp_t cdp, unsigned total_length, char * buf, unsigned & bufpos, unsigned & bufend, char * dest)
{ while (total_length)
  { unsigned loc = (total_length > RDSTEP) ? RDSTEP : total_length;
    const char * ptr=get_data(cdp, loc, buf, &bufpos, &bufend);
    if (!ptr) return false;
    memcpy(dest, ptr, loc);
    dest+=loc;  total_length-=loc;
  }
  return true;
}

void os_load_table(cdp_t cdp, tcurstab tb)
// tb is a table or a cursor with underlying table.
// The table (underlying table) has table_descr locked.
{ trecnum rec, rec_cnt;  tptr ptr;
  unsigned tp, tpsz;  uns64 space64 = 0;
  char buf[FILE_BUF_SIZE];  unsigned bufpos, bufend;  tattrib atr;
  uns8 new_money[MONEY_SIZE], new_nullmoney[MONEY_SIZE] = { 0,0,0,0,0,0x80 };
  ttablenum x_tb;  trecnum x_recnum;  elem_descr * attracc;
  uns32 max_unique_counter = 0;  bool unique_used=false;
  BOOL conv_money=FALSE, user_change=FALSE;  bool update_membership = false;  bool extlobrefs = false;

 // get data source basic info:
  table_descr * tbdf;  int attrcnt;
  BOOL is_cursor = IS_CURSOR_NUM(tb);  cur_descr * cd;  tcursnum crnm;
  if (is_cursor)
  { crnm = GET_CURSOR_NUM(tb);
    { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
      cd = *crs.acc(crnm);
    }
    attrcnt=cd->d_curs->attrcnt;
    if (cd->underlying_table==NULL)
      { request_error(cdp, CANNOT_APPEND);  return; }
    tb=cd->underlying_table->tabnum;
    tbdf=tables[tb];
  }
  else
  { tbdf=tables[tb];
    attrcnt=tbdf->attrcnt;
   // delete all records if importing all users or keys:
    if (tb==USER_TABLENUM || tb==KEY_TABLENUM)
    { for (trecnum xrec=0;  xrec<tbdf->Recnum();  xrec++)
        if (tb_del(cdp, tbdf, xrec)) return;
      if (free_deleted(cdp, tbdf)) return;
    }
  }

  t_packet_writer pawri(cdp, tbdf, attrcnt);
  
 // determine the subtype of the TDT file:
  bufpos=bufend=0;  /* buffer init. */
  ptr=get_data(cdp, 2, buf, &bufpos, &bufend);
  if (!ptr) goto file_err;
  if (*(uns16*)ptr==TDT_EXTLOBREFS)
  { extlobrefs=pawri.extlobrefs=true;  
    cdp->ker_flags |= KFL_EXPORT_LOBREFS;   // flag KFL_EXPORT_LOBREFS is used by tb_write_vector, it is reset on exit
  }
  else if (*(uns16*)ptr!=TDT_VERSNUM)
  { if (*(uns16*)ptr!=TDT_VERSNUM_4)
      if (*(uns16*)ptr!=OLD_TDT_VERSNUM)
        { request_error(cdp, INCOMPATIBLE_VERSION);  goto ex; }
      else conv_money=TRUE;
    user_change=TRUE;
  }

 // count the record size, check for non-writable cursor columns (needs extlobrefs info):
  tattrib attr;  unsigned recsize;
  recsize=0;  // main buffer size counter
  for (atr=1;  atr<attrcnt;  atr++)
  { if (is_cursor) 
    { attr_access(&cd->qe->kont, atr, x_tb, attr, x_recnum, attracc);
      if (attracc!=NULL)
        { request_error(cdp, CANNOT_APPEND);  goto ex; }
    }
    else attr=atr;
    recsize+=pawri.get_space_in_main_buffer(attr);
  }
  
  if (!pawri.allocate_main_buffer(recsize))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto ex; }

  recsize=0;  // now it is the offset counter
  for (atr=1;  atr<attrcnt;  atr++)
  { if (is_cursor) attr_access(&cd->qe->kont, atr, x_tb, attr, x_recnum, attracc);
    else attr=atr;
    pawri.store_column_reference(atr-1, attr, recsize);
  }
  
 // prepare importing records:
  disksp.disable_flush();
  rec_cnt=0;
  cdp->ker_flags|=KFL_ALLOW_TRACK_WRITE | KFL_NO_TRACE | KFL_DISABLE_DEFAULT_VALUES;
  cdp->mask_token=wbtrue;
  while (true)
  {// check the status og the record: 
    ptr=get_data(cdp, 1, buf, &bufpos, &bufend);
    if (!ptr || (*ptr==(char)0xff)) break;
    if (*ptr)
    { if (tb==USER_TABLENUM && tbdf->Recnum()==CONF_ADM_GROUP)
      { new_user_or_group(cdp, "CONFIG_ADMIN",   CATEG_GROUP, conf_adm_uuid);
        new_user_or_group(cdp, "SECURITY_ADMIN", CATEG_GROUP, secur_adm_uuid);
        update_membership=true;
      }
      rec=pawri.insert_deleted_record();
      if (rec==NORECNUM) goto dberr; /* error inside */
      //tb_del(cdp, tbdf, rec); /* a deleted record was saved */  -- done by default_deleted_vector, prevents conflict on unique indicies
    }
    else  // reading and inserting the record
    { for (atr=1;  atr<attrcnt;  atr++)
      { if (!is_cursor) attr=atr;
        else attr_access(&cd->qe->kont, atr, x_tb, attr, x_recnum, attracc);
        attribdef * att = tbdf->attrs+attr;
        tpsz=typesize(att);
        tp=att->attrtype;
        t_colval * pcolval = pawri.get_colval(atr-1);
        if (att->attrmult != 1)  /* read the counter */
        { if (!(ptr=get_data(cdp, CNTRSZ, buf, &bufpos, &bufend)))
            goto file_err;
          t_mult_size count = *(t_mult_size*)ptr;
          pawri.release_multibuf(atr-1, att->attrtype);
          t_varval_list * plist;  char * muvals;
          if (!IS_HEAP_TYPE(tp) && !IS_EXTLOB_TYPE(tp))
            muvals = (char*)pawri.allocate_multibuf(atr-1, count, tpsz, false);
          else
            plist = (t_varval_list*)pawri.allocate_multibuf(atr-1, count, sizeof(t_varval_list), true);
          if (count && !pcolval->dataptr) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto ex; }
          for (t_mult_size index=0;  index<count;  index++)
          { if (!IS_HEAP_TYPE(tp) && !IS_EXTLOB_TYPE(tp))
            { if (tp==ATT_MONEY && conv_money)
              { if (!(ptr=get_data(cdp, 4, buf, &bufpos, &bufend)))
                  goto file_err;
                if (*(sig32*)ptr == NONEINTEGER) ptr=(tptr)new_nullmoney;
                else
                { *(uns32*)new_money=*(uns32*)ptr;
                  *(uns16*)(new_money+4)= (uns16)((new_money[3] & 0x80) ? 0xffff : 0);
                  ptr=(tptr)new_money;
                }
                memcpy(muvals, ptr, tpsz);  
              }
              else 
                if (!load_long_data(cdp, tpsz, buf, bufpos, bufend, muvals)) goto file_err;
              muvals+=tpsz;
            }
            else /* write to a heap object */
            { if (!(ptr=get_data(cdp, HPCNTRSZ, buf, &bufpos, &bufend))) goto file_err;
              t_varcol_size hpsize = plist->length = *(t_varcol_size*)ptr;  
              plist->val=(char*)corealloc(hpsize, 71);
              if (hpsize && !plist->val) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto ex; }
              if (!load_long_data(cdp, hpsize, buf, bufpos, bufend, plist->val)) goto file_err;
              space64+=(uns64)hpsize;
              plist++;
            }
          }
        }
        else if (!IS_HEAP_TYPE(tp) && (!IS_EXTLOB_TYPE(tp) || extlobrefs))  // not multi
        { if (tp==ATT_AUTOR && user_change)  // drop old "author" values
          { get_data(cdp, 2, buf, &bufpos, &bufend);
            memcpy((char*)pcolval->dataptr, tNONEAUTOR, tpsz);
          }
          else if (tp==ATT_MONEY && conv_money)  // convert the old 4-byte [money] values
          { if (!(ptr=get_data(cdp, 4, buf, &bufpos, &bufend)))
              goto file_err;
            if (*(sig32*)ptr == NONEINTEGER) ptr=(tptr)new_nullmoney;
            else
            { *(uns32*)new_money=*(uns32*)ptr;
              *(uns16*)(new_money+4)= (uns16)((new_money[3] & 0x80) ? 0xffff : 0);
              ptr=(tptr)new_money;
            }
            memcpy((char*)pcolval->dataptr, ptr, tpsz);
          }
          else // direct value copy:
          { tptr dest = (char*)pcolval->dataptr;
            if (!load_long_data(cdp, tpsz, buf, bufpos, bufend, dest)) goto file_err;
            if (tbdf->attrs[attr].defvalexpr == COUNTER_DEFVAL)
            { uns32 val = get_typed_unique_value(dest, tbdf->attrs[attr].attrtype);
              if (val!=NONEINTEGER && val > max_unique_counter) { max_unique_counter=val;  unique_used=true; }
            }
            if (tbdf->unikey_attr==attr)
            { uns32 val = get_typed_unique_value(dest+UUID_SIZE, ATT_INT32);
              if (val!=NONEINTEGER && val > max_unique_counter) { max_unique_counter=val;  unique_used=true; }
            }
            else if (tbdf->zcr_attr==attr)
            { if (memcmp(dest, header.gcr, ZCR_SIZE) >= 0)
                // write_gcr(cdp, dest);  -- error, must update header.gcr
                update_gcr(cdp, dest);
            }
          }
        }
        else /* write to a heap object */
        { if (!(ptr=get_data(cdp, HPCNTRSZ, buf, &bufpos, &bufend))) goto file_err;
          t_varcol_size hpsize = *(t_varcol_size*)ptr;  
          pawri.release_heapbuf(atr-1);  
          char * bufptr = pawri.allocate_heap_buffer(atr-1, hpsize);
          if (hpsize && !bufptr) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto ex; }
          if (!load_long_data(cdp, hpsize, buf, bufpos, bufend, bufptr)) goto file_err;
          space64+=(uns64)hpsize;
          if (tp==ATT_HIST && user_change)
            if (tbdf->attrs[attr-1].attrtype==ATT_AUTOR ||
                tbdf->attrs[attr-1].attrtype==ATT_DATIM &&
                tbdf->attrs[attr-2].attrtype==ATT_AUTOR)
              *pcolval->lengthptr=0;  // drop the value
        } // heap
      } // cycle on columns

     // if users from a version previous to 8.0 have been imported, I must add new groups (does not work when records added in a batch):
      if (tb==USER_TABLENUM && tbdf->Recnum()==CONF_ADM_GROUP && strcmp((char*)pawri.get_colval(OBJ_NAME_ATR-1)->dataptr, "CONFIG_ADMIN"))
      { new_user_or_group(cdp, "CONFIG_ADMIN",   CATEG_GROUP, conf_adm_uuid);
        new_user_or_group(cdp, "SECURITY_ADMIN", CATEG_GROUP, secur_adm_uuid);
        update_membership=true;
      }

      rec=pawri.insert_record();
      if (rec==NORECNUM) goto dberr; /* error inside */
      if (is_cursor)
        if (new_record_in_cursor(cdp, rec, cd, crnm)==NORECNUM)
          goto dberr;
      if (tb==USER_TABLENUM)
        remove_refs_to_all_roles(cdp, rec);
    } // valid record
    if (!(rec % 50) || space64 > 1000000)   /* periodical commit */
    { if (cdp->is_an_error()) goto dberr;
      commit(cdp);
      dispatch_kernel_control(cdp);
      space64=0;
    }
    rec_cnt++;
    report_step(cdp, rec_cnt);
    if (cdp->break_request)
      { request_error(cdp, REQUEST_BREAKED);  goto dberr; }
  }

 // if users from a version previous to 8.0 have been imported, I must update the membership in administrative groups:
  if (update_membership)
  {// make all 7.0 admins 8.0 admins:
    for (trecnum xrec=0;  xrec<tbdf->Recnum();  xrec++)
      if (table_record_state(cdp, tbdf, xrec)==NOT_DELETED)
      { tcateg categ;  uns8 state;
        fast_table_read(cdp, tbdf, xrec, OBJ_CATEG_ATR, &categ);
        if (categ==CATEG_USER)
        { user_and_group(cdp, xrec, DB_ADMIN_GROUP, CATEG_GROUP, OPER_GET, &state);
          if (state)
          { user_and_group(cdp, xrec, CONF_ADM_GROUP,  CATEG_GROUP, OPER_SET, &state); 
            user_and_group(cdp, xrec, SECUR_ADM_GROUP, CATEG_GROUP, OPER_SET, &state); 
          }
        }
      }
   // make CONF_ADMIN the Administrator and Author of _sysext
    tobjnum sysext = find_object(cdp, CATEG_APPL, "_SYSEXT");
    if (sysext!=NOOBJECT)
    { WBUUID uuid;
      tb_read_atr(cdp, objtab_descr, sysext, APPL_ID_ATR, (tptr)uuid);
      uns8 state_true = wbtrue;  tobjnum role;
      BOOL was_sec_adm = cdp->prvs.is_secur_admin;  cdp->prvs.is_secur_admin=true;
      role = find2_object(cdp, CATEG_ROLE, uuid, "ADMINISTRATOR");
      if (role!=NOOBJECT)
        user_and_group(cdp, CONF_ADM_GROUP, role,  CATEG_ROLE, OPER_SET, &state_true); 
      role = find2_object(cdp, CATEG_ROLE, uuid, "AUTHOR");
      if (role!=NOOBJECT)
        user_and_group(cdp, CONF_ADM_GROUP, role,  CATEG_ROLE, OPER_SET, &state_true); 
      cdp->prvs.is_secur_admin=was_sec_adm;
    }
  }
  goto ex;

 file_err:
  request_error(cdp, OS_FILE_ERROR);
 dberr:
 ex:  // common exit
 // release buffers:
  for (atr=1;  atr<attrcnt;  atr++)
  { if (is_cursor) attr_access(&cd->qe->kont, atr, x_tb, attr, x_recnum, attracc);
    else attr=atr;
    attribdef * att = tbdf->attrs+attr;
    if (att->attrmult != 1) 
      pawri.release_multibuf(atr-1, att->attrtype);
    else if (IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype) && !extlobrefs) 
      pawri.release_heapbuf(atr-1);
  }
  if (unique_used)  // max_unique_counter is the biggest imported value 
    tbdf->unique_value_cache.register_used_value(cdp, max_unique_counter);
  report_total(cdp, rec_cnt);
  cdp->ker_flags &= ~(KFL_ALLOW_TRACK_WRITE | KFL_NO_TRACE | KFL_DISABLE_DEFAULT_VALUES | KFL_EXPORT_LOBREFS);  
  cdp->mask_token=wbfalse;
  disksp.enable_flush();
}

#pragma pack(1)
typedef struct
{ char logname[OBJ_NAME_LEN];
  char categ;
  WBUUID uuid;
} t_userdef_start;

typedef struct
{ struct end1
  { uns8 identif[122];
    WBUUID home_server;
  } e1;
  struct end2
  { uns8  passwd[20];
    uns32 pass_cre_date;
  } e2;
  struct end3
  { wbbool stopped;
    wbbool certific;
  } e3;
} t_userdef_end;

typedef struct
{ WBUUID key_uuid;
  WBUUID user_uuid;
} t_keydef_start;

typedef struct
{ uns32  credate;
  uns32  expdate;
  uns8   identif[122];
  wbbool certific;
} t_keydef_end;
#pragma pack()

static BOOL skip_var_len(cdp_t cdp, tptr buf, unsigned * bufpos, unsigned * bufend)
{ tptr ptr;  uns32 hpsize;
  if (!(ptr=get_data(cdp, HPCNTRSZ, buf, bufpos, bufend))) return FALSE;
  hpsize=*(uns32*)ptr;
  while (hpsize)
  { unsigned loc = (hpsize > RDSTEP) ? RDSTEP : hpsize;
    if (!(ptr=get_data(cdp, loc, buf, bufpos, bufend)))
      return FALSE;
    hpsize-=loc;
  }
  return TRUE;
}

BOOL read_server_uuid(cdp_t cdp, void * uuid)
{ table_descr_auto srv_tabdescr(cdp, SRV_TABLENUM);
  if (srv_tabdescr->me())
    return tb_read_atr(cdp, srv_tabdescr->me(), 0, SRV_ATR_UUID, (tptr)uuid);
  return TRUE;
}

void os_load_user(cdp_t cdp)
{ char buf[FILE_BUF_SIZE];  unsigned bufpos, bufend;
  tobjnum userobj, userobj2, keyobj;  uns32 hpsize, hpoff;  BOOL user_inserted;
  t_userdef_end * ue_ptr;
  table_descr_auto keytab_descr(cdp, KEY_TABLENUM);
  if (!keytab_descr->me()) return; 

  bufpos=bufend=0;  /* buffer init. */
  tptr ptr=get_data(cdp, 2, buf, &bufpos, &bufend);
  if (!ptr) goto file_err;
  if (*(uns16*)ptr!=TDT_VERSNUM)
    { request_error(cdp, INCOMPATIBLE_VERSION);  goto ex0; }

 // read user table data:
  tptr dt;  tfcbnum fcbn;
  ptr=get_data(cdp, 1, buf, &bufpos, &bufend);
  if (!ptr || *ptr) goto file_err;
  ptr=get_data(cdp, ZCR_SIZE, buf, &bufpos, &bufend);
  if (!skip_var_len(cdp, buf, &bufpos, &bufend)) goto file_err;
  ptr=get_data(cdp, sizeof(t_userdef_start), buf, &bufpos, &bufend);
  if (!ptr) goto file_err;
  userobj=find_object_by_id(cdp, CATEG_USER, ((t_userdef_start*)ptr)->uuid);
  userobj2=find_object(cdp, CATEG_USER, ((t_userdef_start*)ptr)->logname);
  if (userobj2!=NOOBJECT && userobj2!=userobj)
  // found user with same name but different UUID
    { SET_ERR_MSG(cdp, "USERTAB");  request_error(cdp, KEY_DUPLICITY);  goto ex0; }
  if (userobj==NOOBJECT)
  { userobj=tb_new(cdp, usertab_descr, FALSE, NULL);
    if (userobj==NOOBJECT) goto ex0; /* error inside */
    user_inserted=TRUE;
  }
  else
  { t_privils_flex priv_val;
    cdp->prvs.get_effective_privils(usertab_descr, userobj, priv_val);
    if (!(*priv_val.the_map() & RIGHT_WRITE))
      { request_error(cdp, NO_RIGHT);  goto ex0; }
    user_inserted=FALSE;
  }
  { t_simple_update_context sic(cdp, usertab_descr, 0);
    sic.inditem_remove(userobj, NULL);
  }
  dt=fix_attr_write(cdp, usertab_descr, userobj, USER_ATR_LOGNAME, &fcbn);
  if (!dt) goto ex0;
  memcpy(dt, ptr, sizeof(t_userdef_start));
  unfix_page(cdp, fcbn);
 // skip UPS:
  if (!skip_var_len(cdp, buf, &bufpos, &bufend)) goto file_err;
 // end of user data:
  ue_ptr = (t_userdef_end*)
    get_data(cdp, sizeof(t_userdef_end), buf, &bufpos, &bufend);
  if (!ptr) goto file_err;
  dt=fix_attr_write(cdp, usertab_descr, userobj, USER_ATR_INFO, &fcbn);
  if (!dt) goto ex0;

  memcpy(dt, &ue_ptr->e1, sizeof(ue_ptr->e1));
  if (user_inserted)  // set empty password, cannot copy the original one
  { uns8 passwd[sizeof(uns32)+MD5_SIZE];
    unsigned cycles = 1;
    *(uns32*)passwd=cycles;
    unsigned count=cycles+1;
    uns8 * digest = passwd+sizeof(uns32);
    uns8 mess[UUID_SIZE];
    read_server_uuid(cdp, (tptr)mess);
    md5_digest(mess, UUID_SIZE, digest);
    while (count-- > 1)
      md5_digest(digest, MD5_SIZE, digest);
    memcpy(dt+sizeof(ue_ptr->e1), passwd, sizeof(ue_ptr->e2.passwd));
    uns32 dtm = Today();
    memcpy(dt+sizeof(ue_ptr->e1)+sizeof(ue_ptr->e2.passwd), &dtm, sizeof(dtm));
  }
  else
    memcpy(dt+sizeof(ue_ptr->e1), &ue_ptr->e2, sizeof(ue_ptr->e2));
  memcpy(dt+sizeof(ue_ptr->e1)+sizeof(ue_ptr->e2), &ue_ptr->e3, sizeof(ue_ptr->e3));
  unfix_page(cdp, fcbn);
  { t_simple_update_context sic(cdp, usertab_descr, 0);
    sic.inditem_add(userobj);  // not locking the index, this is a versy special case
  }
  ptr=get_data(cdp, 1, buf, &bufpos, &bufend);  // reads terminator
  if (!ptr || *ptr!=(char)0xff) goto file_err;
 // privils:
  if (user_inserted)
  { t_privils_flex priv_descr(usertab_descr->attrcnt);
    priv_descr.clear();
    priv_descr.the_map()[0]=RIGHT_DEL|RIGHT_GRANT;
    priv_descr.add_write(USER_ATR_LOGNAME);
    priv_descr.add_write(USER_ATR_INFO);
    priv_descr.add_write(USER_ATR_CERTIF);
    priv_descr.add_write(USER_ATR_SERVER);
    privils prvs(cdp);
    prvs.set_user(userobj, CATEG_USER, FALSE);
    prvs.set_own_privils(usertab_descr, userobj, priv_descr);
    priv_descr.clear();
    cdp->prvs.set_own_privils(usertab_descr, userobj, priv_descr);
  }

 // read keys:
  ptr=get_data(cdp, 2, buf, &bufpos, &bufend);
  if (!ptr) goto file_err;
  do
  { ptr=get_data(cdp, 1, buf, &bufpos, &bufend);
    if (!ptr) goto file_err;
    if (*ptr==(char)0xff) break;
   // start of key data:
    if (!skip_var_len(cdp, buf, &bufpos, &bufend)) goto file_err;
    ptr=get_data(cdp, sizeof(t_keydef_start), buf, &bufpos, &bufend);
    if (!ptr) goto file_err;
    keyobj=find_object_by_id(cdp, CATEG_KEY, ((t_keydef_start*)ptr)->key_uuid);
    if (keyobj==NOOBJECT)
    { keyobj=tb_new(cdp, keytab_descr->me(), FALSE, NULL);
      if (keyobj==NOOBJECT) goto ex0; /* error inside */
    }
    { t_simple_update_context sic(cdp, keytab_descr->me(), 0);
      sic.inditem_remove(keyobj, NULL);
    }
    dt=fix_attr_write(cdp, keytab_descr->me(), keyobj, KEY_ATR_UUID, &fcbn);
    if (!dt) goto ex0;
    memcpy(dt, ptr, sizeof(t_keydef_start));
    unfix_page(cdp, fcbn);
   // public key value:
    if (!(ptr=get_data(cdp, HPCNTRSZ, buf, &bufpos, &bufend)))
      goto file_err;
    hpsize=*(uns32*)ptr;  hpoff=0;
    while (hpsize)
    { unsigned loc = (hpsize > RDSTEP) ? RDSTEP : hpsize;
      if (!(ptr=get_data(cdp, loc, buf, &bufpos, &bufend)))
        goto file_err;
      if (tb_write_var(cdp, keytab_descr->me(), keyobj, KEY_ATR_PUBKEYVAL, hpoff, (uns16)loc, ptr))
        goto ex0;
      hpoff+=loc;  hpsize-=loc;
    }
   // end of key data:
    ptr=get_data(cdp, sizeof(t_keydef_end), buf, &bufpos, &bufend);
    if (!ptr) goto file_err;
    dt=fix_attr_write(cdp, keytab_descr->me(), keyobj, KEY_ATR_CREDATE, &fcbn);
    if (!dt) goto ex0;
    memcpy(dt, ptr, sizeof(t_keydef_end));
    unfix_page(cdp, fcbn);
    { t_simple_update_context sic(cdp, keytab_descr->me(), 0);
      sic.inditem_add(keyobj);  // not locking the index, this is a versy special case
    }
   // certifs:
    if (!(ptr=get_data(cdp, HPCNTRSZ, buf, &bufpos, &bufend)))
      goto file_err;
    hpsize=*(uns32*)ptr;  hpoff=0;
    while (hpsize)
    { unsigned loc = (hpsize > RDSTEP) ? RDSTEP : hpsize;
      if (!(ptr=get_data(cdp, loc, buf, &bufpos, &bufend)))
        goto file_err;
      if (tb_write_var(cdp, keytab_descr->me(), keyobj, KEY_ATR_CERTIFS, hpoff, (uns16)loc, ptr))
        goto ex0;
      hpoff+=loc;  hpsize-=loc;
    }
  } while (TRUE);
  goto ex0;

 file_err:
  request_error(cdp, OS_FILE_ERROR);
 ex0: ;
}

void clear_all_passwords(cdp_t cdp)
{// prepare empty expiring password:
  uns8 passwd[sizeof(uns32)+MD5_SIZE];
  unsigned cycles = 1;
  *(uns32*)passwd=cycles;
  unsigned count=cycles+1;
  uns8 * digest = passwd+sizeof(uns32);
  uns8 mess[UUID_SIZE];
  read_server_uuid(cdp, mess);
  md5_digest(mess, UUID_SIZE, digest);
  while (count-- > 1)
    md5_digest(digest, MD5_SIZE, digest);
 // write it to all users:
  trecnum limit = usertab_descr->Recnum();
  for (trecnum r=3;  r<limit;  r++)
  { tcateg categ;
    if (tb_read_atr(cdp, usertab_descr, r, USER_ATR_CATEG, (tptr)&categ)) continue;
    if (categ==CATEG_USER)
      tb_write_atr(cdp, usertab_descr, r, USER_ATR_PASSWD, passwd);
  }
}

/***************************** exporter *************************************/
// Export is 
typedef class t_exporter
{ bool extlobrefs;
 protected:
  cdp_t cdp;
  bool database_error;
  // table or cursor description:
  tobjnum tb;
  BOOL is_cursor;
  table_descr * tbdf;  cur_descr * cd;
  int attrcnt, start_a;  trecnum reccnt;
  // buffer:
  char buf[FILE_BUF_SIZE];  unsigned bufpos;

  virtual bool output_it(void * adr, unsigned size) = 0;  // TRUE on error

  BOOL put_buf(const void * adr, unsigned size);                // TRUE on error
  BOOL hp_file_copy(const tpiece * pc, uns32 size);             // TRUE on error
  BOOL extclob_file_copy(cdp_t cdp, const uns64 lobid);         // TRUE on error

  BOOL put_empty(int tp, int sz);

 public:
  t_exporter(cdp_t init_cdp)
    { cdp=init_cdp;  database_error=false; 
      extlobrefs = (cdp->ker_flags &  KFL_EXPORT_LOBREFS) != 0;
    }
  virtual ~t_exporter(void)  { }

  BOOL open(tobjnum init_tb, tattrib start_attr, tattrib lim_attr); // FALSE on error
  // lim_attr is NOATTRIB or the number of the 1st not stored attribute
  // init_tb is supposed to have its descr locked.
  BOOL export_rec(trecnum rec);                           // FALSE on error
  BOOL export_all(void);                                  // FALSE on error
  BOOL export_single_rec(trecnum rec);                    // FALSE on error
  BOOL empty_buf(void);                                   // TRUE on error

} t_exporter;

BOOL t_exporter::open(tobjnum init_tb, tattrib start_attr, tattrib lim_attr)
{ tb=init_tb;  database_error=false;
  bufpos=0;  /* buffer init. */
  is_cursor = IS_CURSOR_NUM(tb);
  if (is_cursor)
  { tb = GET_CURSOR_NUM(tb);
    cd = get_cursor(cdp, tb);
    if (!cd) return FALSE; 
    make_complete(cd);
    attrcnt = cd->d_curs->attrcnt;
    reccnt=cd->recnum;
  }
  else
  { tbdf    = tables[tb];
    attrcnt = tbdf->attrcnt;
    reccnt  = tbdf->Recnum();
  }
  if (lim_attr  !=NOATTRIB && lim_attr  <attrcnt) attrcnt=lim_attr;
  if (start_attr!=NOATTRIB && start_attr<attrcnt) start_a=start_attr;
  else start_a=1;
  return TRUE;
}

BOOL t_exporter::put_buf(const void * adr, unsigned size)
/* buf contains bufpos bytes of data, no more than RDSTEP data are written
   at once. FILE_BUF_SIZE is supposed not to be twice bigger than RDSTEP
   (otherwise empty_buf must be changed). */
{ unsigned loc;
  do
  {// copy to the empty part og the buffer:
    loc=FILE_BUF_SIZE-bufpos;
    if (size < loc) loc=size;
    memcpy(buf+bufpos, adr, loc);
    size-=loc;  adr=(tptr)adr+loc;  bufpos+=loc;
    if (bufpos >= RDSTEP)
    { if (output_it(buf, RDSTEP))
        { request_error(cdp, OS_FILE_ERROR);  return TRUE; }
      memmov(buf, buf+RDSTEP, bufpos-RDSTEP);
      bufpos-=RDSTEP;
    }
  } while (size);
  return FALSE;
}

BOOL t_exporter::empty_buf(void)
{ unsigned loc;
  while (bufpos)
  { loc = bufpos < RDSTEP ? bufpos : RDSTEP;
    if (output_it(buf, loc))
      { request_error(cdp, OS_FILE_ERROR);  return TRUE; }
    memmov(buf, buf+loc, bufpos-loc);
    bufpos-=loc;
  }
  return FALSE;
}

BOOL t_exporter::extclob_file_copy(cdp_t cdp, const uns64 lobid)
{ uns32 length;  char buf[1024];
  FHANDLE hnd=open_extlob(lobid);
  if (hnd==INVALID_FHANDLE_VALUE) length=0;
  else length=read_extlob_length(hnd);
  put_buf(&length, HPCNTRSZ);
  uns32 start = 0;
  while (length)
  { uns32 part = length < sizeof(buf) ? length : sizeof(buf);
    uns32 rdpart = read_extlob_part(hnd, start, part, buf);
    put_buf(buf, rdpart);
    start+=rdpart;  length-=rdpart;
  }
  if (hnd!=INVALID_FHANDLE_VALUE) close_extlob(hnd);
  return FALSE;
}    

BOOL t_exporter::hp_file_copy(const tpiece * pc, uns32 size)
// Writes length and contents of a var-len column.
// Returns TRUE on file write errors, sets database_error and returns FAlSE on database errors.
{ if (size && pc->len32 && pc->dadr)
  { t_dworm_access dwa(cdp, pc, 1, FALSE, NULL);
    if (!dwa.is_open()) 
      { size=0;  database_error=true; }
    if (put_buf(&size, HPCNTRSZ)) return TRUE;
    unsigned pos = 0;
    while (size)
    { tfcbnum fcbn;  unsigned local;
      const char * dt = dwa.pointer_rd(cdp, pos, &fcbn, &local);
	    if (!dt) 
      { database_error=true;
       // white the rest:
        char buf[256];
        memset(buf, 0, sizeof(buf));
        while (size)
        { local=sizeof(buf);
     	    if (local > size) local=size;
          if (put_buf(buf, local)) return TRUE;
          size-=local;
        }
        return FALSE;
      }
	    if (local > size) local=size;
      if (put_buf(dt, local)) return TRUE;
      if (fcbn != NOFCBNUM)
        unfix_page(cdp, fcbn);
      pos += local;  size -= local;
    }
  }
  else
  { size=0; 
    if (put_buf(&size, HPCNTRSZ)) return TRUE;
  }
  return FALSE;
}

BOOL t_exporter::put_empty(int tp, int sz)
{ uns32 zero_size=0;
  if (IS_HEAP_TYPE(tp) || IS_EXTLOB_TYPE(tp))   
  { if (put_buf(&zero_size, HPCNTRSZ)) return TRUE;
  }
  else
  { char valbuf[256];
    qset_null(valbuf, tp, sz);
    if (put_buf(valbuf, sz)) return TRUE;
  }
  return FALSE;
}

BOOL t_exporter::export_rec(trecnum rec)
// Stops immediately on file write error, continues on database errors.
{ trecnum recnum;  const char * dt;  tfcbnum fcbn;  BOOL res;  int atr;
  elem_descr *attracc;  t_value exprval;  tattrib attr;  
 /* seek and check to see if deleted: */
  uns8 dltd;
  if (is_cursor)
  { cd->curs_seek(rec);
    dltd=cd->cursor_record_state(rec);
  }
  else
  { dltd=table_record_state(cdp, tbdf, rec);
    if (dltd==OUT_OF_CURSOR) return TRUE;
    if (dltd) dltd=DELETED;  /* must not save the RECORD_EMPTY value */
    recnum = rec;
    attracc = NULL;
  }
  if (put_buf(&dltd, 1)) return FALSE; /* DELETED attribute */
  if (dltd) return TRUE;

 /* saving the other attributes */
  for (atr=start_a;  atr<attrcnt;  atr++)
  { if (!is_cursor) attr=atr;
    else
    { attr_access(&cd->qe->kont, atr, tb, attr, recnum, attracc);
      tbdf=tables[tb];
    }
   // tbdf defined now for both table and cursor export, unless attracc
    BOOL mult;  int  sz, tp;
    if (attracc==NULL)
    { attribdef * att = tbdf->attrs+attr;
      mult = att->attrmult != 1;
      tp   = att->attrtype;
      sz   = typesize(att);
    }
    else
    { exprval.set_null();
      { t_temporary_current_cursor tcc(cdp, cd);  // sets the temporary current_cursor in the cdp until destructed
        expr_evaluate(cdp, attracc->expr.eval, &exprval);
      }
      tp=attracc->type;          // fixed type supposed!
      //mult=attracc->multi != 1;  // FALSE supposed!
      sz = SPECIFLEN(tp)  ? attracc->specif.length : tpsize[tp];
      if (exprval.is_null()) qlset_null(&exprval, tp, sz);
      if (put_buf(exprval.valptr(), sz)) return FALSE;
      continue;
    }

   // writing the non-calculated value:
    if (mult)  /* write the counter */
    { uns16 count;
      if (recnum==OUTER_JOIN_NULL_POSITION) count=0; // outer join NULL
      else if (fast_table_read(cdp, tbdf, recnum, attr, &count)) 
        { database_error=true;  count=0; }
      if (put_buf(&count, CNTRSZ)) return FALSE;
      for (int index=0;  index<count;  index++)
      { dt=fix_attr_ind_read(cdp, tbdf, recnum, attr, index, &fcbn);
        if (dt==NULL) 
        { database_error=true;
          put_empty(tp, sz);
        }
        else
        { if (IS_HEAP_TYPE(tp))
            res=hp_file_copy((const tpiece*)dt, *(const uns32*)(dt+sizeof(tpiece)));
          else res=put_buf(dt, sz);
          unfix_page(cdp, fcbn);
        }
        if (res) return FALSE;
      }
    }
    else if (recnum==OUTER_JOIN_NULL_POSITION)
      put_empty(tp, sz);
    else
    { dt=fix_attr_read(cdp, tbdf, recnum, attr, &fcbn);
      if (dt==NULL) 
      { database_error=true;
        put_empty(tp, sz);
      }
      else
      { if (IS_HEAP_TYPE(tp))
          res=hp_file_copy((const tpiece*)dt, *(const uns32*)(dt+sizeof(tpiece)));
        else if (IS_EXTLOB_TYPE(tp) && !extlobrefs)  
          res=extclob_file_copy(cdp, *(const uns64*)dt);
        else  // includes extlob IDs
          res=put_buf(dt, sz);
        unfix_page(cdp, fcbn);
        if (res) return FALSE;
      }
    }
  }
  return TRUE;  // OK
}

BOOL t_exporter::export_all(void)
{ trecnum rec;  BOOL ok=TRUE;
  uns8 terminator = 0xff;  
  uns16 version = extlobrefs ? TDT_EXTLOBREFS : TDT_VERSNUM;
  if (put_buf(&version, 2)) goto exit;  /* version number saved */

  for (rec=0;  rec<reccnt;  rec++)
  { if (!export_rec(rec)) { ok=FALSE;  goto exit; }
    report_step(cdp, rec);
    if (cdp->break_request)
      { request_error(cdp, REQUEST_BREAKED);  ok=FALSE;  goto exit; }
  }

  if (put_buf(&terminator, 1)) { ok=FALSE;  goto exit; }
  if (empty_buf()) ok=FALSE;
 exit:
  report_total(cdp, rec);
  return ok && !database_error;
}

BOOL t_exporter::export_single_rec(trecnum rec)
{ uns8 terminator = 0xff;  
  uns16 version = extlobrefs ? TDT_EXTLOBREFS : TDT_VERSNUM;
  if (put_buf(&version, 2)) return FALSE;  /* version number saved */
  if (!export_rec(rec)) return FALSE;
  if (put_buf(&terminator, 1)) return FALSE;
  if (empty_buf()) return FALSE;
  return !database_error;
}

/******************************* network exporter ***************************/
typedef class t_net_exporter : public t_exporter
{ inline bool output_it(void * adr, unsigned size)
    { return cdp->write_to_client(adr, size); }
 public:
  t_net_exporter(cdp_t cdp) : t_exporter(cdp) {}
  virtual ~t_net_exporter(void)  { }
} t_net_exporter;


/****************************** mapped file exporter ************************/
#ifdef WINS
typedef class t_mf_exporter : public t_exporter
{ inline bool output_it(void * adr, unsigned size)
    { return cdp->write_to_client(adr, size); }
 public:
  t_mf_exporter(cdp_t cdp) : t_exporter(cdp) {}
  virtual ~t_mf_exporter(void)  { }
} t_mf_exporter;
#endif

/****************************** contaier exporter ***************************/
#define CONTR_ALLOC_STEP  3000

class t_container
{ HGLOBAL hData;
  uns32 bufsize;
 public:
  char * buf;
  uns32 datapos;
  t_container(void)
    { hData=0;  buf=NULL;  datapos=bufsize=0; }
  ~t_container(void);
  unsigned write(void * data, unsigned size);
};

t_container::~t_container(void)
{ 
#ifdef WINS
  if (hData) { GlobalUnlock(hData);  GlobalFree(hData); } 
#else
  if (buf) free(buf); 
#endif
}

unsigned t_container::write(void * data, unsigned size)
{ if (datapos+size > bufsize)  /* reallocate */
  { uns32 newsize = bufsize+CONTR_ALLOC_STEP;
    if (newsize < datapos+size) newsize = datapos+size;
#ifdef WINS
    HGLOBAL new_data;
    if (hData)
    { GlobalUnlock(hData);
      new_data = GlobalReAlloc(hData, newsize, GMEM_MOVEABLE);
      if (!new_data) { buf=(char *)GlobalLock(hData);  return 0; }
    }
    else
    { new_data = GlobalAlloc(GMEM_MOVEABLE, newsize);
      if (!new_data) return 0;
    }
    hData=new_data;  buf=(char *)GlobalLock(hData);
#else
    tptr newbuf = (tptr)malloc(newsize);
    if (!newbuf) return 0;
    if (buf) { memcpy(newbuf, buf, bufsize);  free(buf); }
    buf=newbuf;
#endif
    bufsize=newsize;
  }
  memcpy(buf+datapos, data, size);
  datapos+=size;
  return size;
}

typedef class t_cnt_exporter : public t_exporter
{ t_container * contr;
  bool output_it(void * adr, unsigned size)
    { return contr->write((tptr)adr, size) != size; }
 public:
  t_cnt_exporter(cdp_t cdp, t_container * init_contr) : t_exporter(cdp)
    { contr=init_contr; }
  virtual ~t_cnt_exporter(void)  { }
} t_cnt_exporter;

/****************************** exporter clients ****************************/
void os_save_table(cdp_t cdp, ttablenum tb, trecnum recnum)
// Exports the table or cursor tb via mapped file or network.
// All records, all attributes.
{ t_exporter * exp = NULL;
#ifdef WINS
  if      (cdp->in_use==PT_KERDIR)
    exp = new t_mf_exporter(cdp);
  else if (cdp->in_use==PT_SLAVE)
#endif
    exp = new t_net_exporter(cdp);
  if (exp==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
  if (exp->open(tb, 1, NOATTRIB))
    if (tb==USER_TABLENUM && recnum!=NORECNUM)
      exp->export_single_rec(recnum);
    else exp->export_all();
  delete exp;
}

BOOL data_to_container(cdp_t cdp, t_container * contr, tobjnum tb,
                       trecnum rec, tattrib lim_attr)
{ BOOL ok=TRUE;
  t_cnt_exporter * exp = new t_cnt_exporter(cdp, contr);
  if (exp==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
 // find 1st user attribute:
  int start_attr=1;
  if (!IS_CURSOR_NUM(tb))  // cursor is supposed not to contain system attributes
  { table_descr_auto td(cdp, tb);
    if (td->me()==NULL) { delete exp;  return FALSE; }
    while (IS_SYSTEM_COLUMN(td->attrs[start_attr].name)) start_attr++;
  }
  if (!exp->open(tb, start_attr, lim_attr)) ok=FALSE;
  else
    ok = exp->export_rec(rec) && !exp->empty_buf();
  delete exp;
  return ok;
}


//////////////////////////////// signatures //////////////////////////////////
// Test CA: uzivatel, ktery certifikuje podpis, musi mit ve svem klici nastaveno CA

static BOOL calc_signature(cdp_t cdp, ttablenum tb, trecnum rec, tattrib attr,
                    t_user_ident * user_ident, uns32 dtm, t_hash md5)  // for old keys only!
// Calculates hash from signed data, user identif and date.
{ t_container contr;
  if (!data_to_container(cdp, &contr, tb, rec, attr)) return FALSE;
  if (contr.write((tptr)user_ident, sizeof(t_user_ident)) != sizeof(t_user_ident))
    return FALSE;
  if (contr.write((tptr)&dtm, sizeof(uns32)) != sizeof(uns32)) return FALSE;
  md5_digest((uns8 *)contr.buf, contr.datapos, md5);
  return TRUE;
}

static BOOL calc_signature_ex(cdp_t cdp, ttablenum tb, trecnum rec, tattrib attr,
                    t_user_ident * user_ident, uns32 dtm, t_hash_ex digest)
// Calculates hash from signed data, user identif and date.
{ t_container contr;
  if (!data_to_container(cdp, &contr, tb, rec, attr)) return FALSE;
  if (contr.write((tptr)user_ident, sizeof(t_user_ident)) != sizeof(t_user_ident))
    return FALSE;
  if (contr.write((tptr)&dtm, sizeof(uns32)) != sizeof(uns32)) return FALSE;
  unsigned digest_length;
  compute_digest((uns8 *)contr.buf, contr.datapos, digest, &digest_length);
  return TRUE;
}

static tcursnum GetKeyList(cdp_t cdp, tobjnum userobj, cur_descr ** pcd)  // for old keys only!
// Opens query for keys of the given user. Returns NOOBJECT on error.
{ WBUUID user_uuid;  char query[130];
  if (tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_UUID, (tptr)user_uuid))
    return NOCURSOR;
  strcpy(query, "SELECT * FROM KEYTAB WHERE USER_UUID=X\'");
  size_t len=strlen(query);
  bin2hex(query+len, user_uuid, UUID_SIZE);
  len+=2*UUID_SIZE;
  strcpy(query+len, "\' ORDER BY CREDATE DESC");
  return open_working_cursor(cdp, query, pcd);
}

static BOOL is_top_ca_key(cdp_t cdp, trecnum keyrec)
{ WBUUID CA_uuid, user_uuid;
  the_sp.get_top_CA_uuid(CA_uuid);
  if (tb_read_atr(cdp, KEY_TABLENUM, keyrec, KEY_ATR_USER, (tptr)user_uuid))
    return FALSE;
  return !memcmp(user_uuid, CA_uuid, UUID_SIZE);
}

t_sigstate verify_certificate(cdp_t cdp, const t_Certificate * cert, uns32 stamp, int trace_level)
// Verifies the certificate chain, does not delete the original certificate.
{ bool initial_cert = true;  // says that [cert] contains the original value - a certificate used to sign a document
  t_sigstate result = sigstate_valid;  
  WBUUID last_cert_uuid;  null_uuid(last_cert_uuid);
  tobjnum upper_cert_obj;
  do
  {// check info in the current certificate: 
    SYSTEMTIME starttime, stoptime;  bool is_ca;  WBUUID signer_uuid;
    get_certificate_info(cert, &starttime, &stoptime, &is_ca, signer_uuid);
    uns32 start=datetime2timestamp(Make_date(starttime.wDay, starttime.wMonth, starttime.wYear), Make_time(starttime.wHour, starttime.wMinute, starttime.wSecond, starttime.wMilliseconds));
    uns32 stop =datetime2timestamp(Make_date(stoptime .wDay, stoptime .wMonth, stoptime .wYear), Make_time(stoptime .wHour, stoptime .wMinute, stoptime .wSecond, stoptime .wMilliseconds));
    // if the certificate is the same as the last one, stop
    if (!memcmp(signer_uuid, last_cert_uuid, UUID_SIZE)) break;
    memcpy(last_cert_uuid, signer_uuid, UUID_SIZE);
    if (stamp < start || stamp>stop) { result=sigstate_time;  break; }
    if (!initial_cert && !is_ca) { result=sigstate_notca;  break; }
    upper_cert_obj = find_object_by_id(cdp, CATEG_KEY, signer_uuid);
    if (upper_cert_obj==NOOBJECT) { result=sigstate_unknown;  break; }
   // list the upper certificate in the trace:
    if (trace_level) 
    { t_user_ident identif;
      tb_read_atr(cdp, KEY_TABLENUM, upper_cert_obj, KEY_ATR_IDENTIF, (tptr)&identif);
      trace_sig(cdp, &identif, NULL, trace_level++);
    }
   // get the upper certificate:
    uns32 cert_str_len;  tptr cert_str;  
    cert_str = ker_load_blob(cdp, tables[KEY_TABLENUM], upper_cert_obj, KEY_ATR_PUBKEYVAL, &cert_str_len);
    if (!cert_str) { result=sigstate_error;  break; }
    t_Certificate * upper_cert;
    upper_cert = make_certificate((unsigned char *)cert_str, cert_str_len);
    corefree(cert_str);
    if (!upper_cert) { result=sigstate_error;  break; }
    if (!VerifyCertificateSignature(cert, upper_cert)) { DeleteCert(upper_cert);  result=sigstate_uncertif;  break; }
   // next step:
    if (!initial_cert) DeleteCert((t_Certificate *)cert);
    cert=upper_cert;
    initial_cert=false;
  } while (TRUE);
  if (!initial_cert) DeleteCert((t_Certificate *)cert);
 // check if the root certificate is owned by the RCA:
  if (result == sigstate_valid)
    if (!is_top_ca_key(cdp, upper_cert_obj)) result = sigstate_notca;
  return result;
}

static BOOL GetKeyState(cdp_t cdp, trecnum keyrec, BOOL trace)  // for old keys only!
{ if (is_top_ca_key(cdp, keyrec)) return TRUE;
  return check_signature_state(cdp, KEY_TABLENUM, keyrec, KEY_ATR_CERTIFS, trace) == sigstate_valid;
}

static void GetValidKey(cdp_t cdp, tobjnum userobj, trecnum * validkey)  // for old keys only!
// Find the key for signing
{ trecnum rec, trec;  cur_descr * cd;
  trecnum keyrec=NORECNUM;
  tcursnum cursnum = GetKeyList(cdp, userobj, &cd);
  if (cursnum==NOCURSOR) return;
  for (rec=0;  rec<cd->recnum;  rec++)
    if (!cd->curs_seek(rec, &trec))
    { if (GetKeyState(cdp, trec, FALSE))  // certified OK
        { keyrec=trec;  break; }
    }
  close_working_cursor(cdp, cursnum);
  if (keyrec!=NORECNUM)
  { t_user_ident uidentif, kidentif;  uns8 ucertif, kcertif;
    tb_read_atr(cdp, KEY_TABLENUM,  keyrec,  KEY_ATR_IDENTIF, (tptr)&kidentif);
    tb_read_atr(cdp, KEY_TABLENUM,  keyrec,  KEY_ATR_CERTIF,  (tptr)&kcertif);
    tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_INFO,   (tptr)&uidentif);
    tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_CERTIF, (tptr)&ucertif);
    if (kcertif!=ucertif || strcmp(uidentif.name1, kidentif.name1) ||
        strcmp(uidentif.name2, kidentif.name2) ||
        strcmp(uidentif.name3, kidentif.name3) ||
        strcmp(uidentif.identif, kidentif.identif))
      if (!is_top_ca_key(cdp, keyrec))
        keyrec=NORECNUM;
  }
  *validkey=keyrec;
}

static BOOL in_validity_period(cdp_t cdp, uns32 sigdtm, trecnum keyrec)
{ uns32 start, stop;
  return !tb_read_atr(cdp, KEY_TABLENUM, keyrec, KEY_ATR_CREDATE, (tptr)&start) &&
         !tb_read_atr(cdp, KEY_TABLENUM, keyrec, KEY_ATR_EXPIRES, (tptr)&stop) &&
         start <= sigdtm && sigdtm < stop;
}

BOOL prepare_signing(cdp_t cdp, table_descr * keytab_descr, ttablenum tb, trecnum rec, tattrib attr,
                   t_hash hsh, WBUUID key_uuid, uns32 expdate)  // for old keys only!
// Computes the signature for the logged user.
// When signing a key, verifies and updates its user info and expirdate,
//  deletes old certificates if changed.
{
 // find a valid key of the signing user and user info from this key:
  trecnum keyrec;  WBUUID key2_uuid;
  GetValidKey(cdp, cdp->prvs.luser(), &keyrec);
  if (keyrec==NORECNUM)  // cannot sign, no certified key
    { request_error(cdp, NO_KEY_FOUND);  return TRUE; }
  if (tb!=KEY_TABLENUM || !is_top_ca_key(cdp, keyrec))
    if (!in_validity_period(cdp, stamp_now(), keyrec))
      { request_error(cdp, NO_KEY_FOUND);  return TRUE; }
  if (tb_read_atr(cdp, keytab_descr, keyrec, KEY_ATR_UUID, (tptr)key2_uuid))
    return TRUE;
  if (memcmp(key_uuid, key2_uuid, UUID_SIZE)) // key replaced
    { request_error(cdp, DIFFERENT_KEY);  return TRUE; }

  if (tb==KEY_TABLENUM)  // certifying a key: add data about user to it, if missing or changed
  { t_user_ident user_ident, key_ident;  wbbool user_is_ca, key_is_ca;
   // find the owner of the key:
    WBUUID user_uuid;  uns32 key_expires;
    if (tb_read_atr(cdp, keytab_descr, rec, KEY_ATR_USER, (tptr)user_uuid))
      return TRUE;
    tobjnum userobj = find_object_by_id(cdp, CATEG_USER, user_uuid);
    if (userobj==NOOBJECT) { SET_ERR_MSG(cdp, "USER-ID");  request_error(cdp, OBJECT_NOT_FOUND);  return TRUE; }
   // read data about the user:
    if (tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_INFO,   (tptr)&user_ident) ||
        tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_CERTIF, (tptr)&user_is_ca))
      return TRUE;
   // read data about the key:
    if (tb_read_atr(cdp, keytab_descr, rec, KEY_ATR_IDENTIF,(tptr)&key_ident) ||
        tb_read_atr(cdp, keytab_descr, rec, KEY_ATR_CERTIF, (tptr)&key_is_ca)  ||
        tb_read_atr(cdp, keytab_descr, rec, KEY_ATR_EXPIRES,(tptr)&key_expires))
      return TRUE;
   // look for changes:
    BOOL chng=FALSE;
    if (user_is_ca!=key_is_ca || expdate!=key_expires ||
        strcmp(user_ident.name1  , key_ident.name1) ||
        strcmp(user_ident.name2  , key_ident.name2) ||
        strcmp(user_ident.name3  , key_ident.name3) ||
        strcmp(user_ident.identif, key_ident.identif)) chng=TRUE;
    if (chng)
    { tb_write_atr(cdp, keytab_descr, rec, KEY_ATR_IDENTIF, &user_ident);
      tb_write_atr(cdp, keytab_descr, rec, KEY_ATR_CERTIF,  &user_is_ca);
      tb_write_atr(cdp, keytab_descr, rec, KEY_ATR_EXPIRES, &expdate);
      tb_write_atr_len(cdp, keytab_descr, rec, KEY_ATR_CERTIFS, 0);
    }
  }
 // get info about the signing user:
  t_user_ident signing_user_ident;
  if (tb_read_atr(cdp, keytab_descr, keyrec, KEY_ATR_IDENTIF, (tptr)&signing_user_ident))
    return TRUE;
  calc_signature(cdp, tb, rec, attr, &signing_user_ident, cdp->auxinfo=stamp_now(), hsh);
  return FALSE;
}

BOOL prepare_signing_ex(cdp_t cdp, table_descr * keytab_descr, ttablenum tb, trecnum rec, tattrib attr,
                   trecnum keyrec, t_hash_ex hsh, uns32 * timestamp)
// Computes the signature for the logged user.
{
 // get info about the signing user:
  t_user_ident signing_user_ident;
  if (tb_read_atr(cdp, keytab_descr, keyrec, KEY_ATR_IDENTIF, (tptr)&signing_user_ident))
    return TRUE;
  *timestamp=stamp_now();  // now included among digested attributes, later stored into the signature
  calc_signature_ex(cdp, tb, rec, attr, &signing_user_ident, *timestamp, hsh);
  return FALSE;
}

BOOL write_signature(cdp_t cdp, table_descr * keytab_descr, ttablenum tb, trecnum rec, tattrib attr, 
       t_enc_hash * enc_hash)  // for old keys only!
{// find a valid key of the signing user and user info from this key:
  trecnum keyrec;
  GetValidKey(cdp, cdp->prvs.luser(), &keyrec);
  if (keyrec==NORECNUM)  // cannot sign, no certified key
    { request_error(cdp, NO_KEY_FOUND);  return TRUE; }
 // fill the signature contents:
  t_signature sig;
  sig.dtm=cdp->auxinfo;
  if (tb_read_atr(cdp, keytab_descr, keyrec, KEY_ATR_UUID, (tptr)sig.key_uuid))
    return TRUE;
  if (tb_read_atr(cdp, usertab_descr, cdp->prvs.luser(), USER_ATR_INFO, (tptr)&sig.user_ident))
    return TRUE;
  memcpy(sig.enc_hash, enc_hash, sizeof(t_enc_hash));
  sig.state=GetKeyState(cdp, keyrec, FALSE) ? sigstate_valid : sigstate_uncertif;
 // write the signature:
  uns32 pos=0;
  if (tb==KEY_TABLENUM)  // certifying a key
  { if (tb_read_atr_len(cdp, tables[tb], rec, KEY_ATR_CERTIFS, &pos)) return TRUE;
    if (pos % sizeof(t_signature))  // align
      pos=(pos/sizeof(t_signature) + 1) * sizeof(t_signature);
  }
  if (tb_write_var(cdp, tables[tb], rec, attr, pos, sizeof(t_signature), &sig))
    return TRUE;
  return FALSE;
}

BOOL write_signature_ex(cdp_t cdp, table_descr * keytab_descr, ttablenum tb, trecnum rec, tattrib attr, t_enc_hash * enc_hash,
                        trecnum keyrec, uns32 timestamp)
{// fill the signature contents:
  t_signature sig;
  sig.dtm=timestamp;
  if (tb_read_atr(cdp, keytab_descr, keyrec, KEY_ATR_UUID, (tptr)sig.key_uuid)) return TRUE;
  //if (tb_read_atr(cdp, usertab_descr, cdp->prvs.luser(), USER_ATR_INFO, (tptr)&sig.user_ident)) return TRUE;  // must read from the same sourceas in prepare_signing_ex
  if (tb_read_atr(cdp, keytab_descr, keyrec, KEY_ATR_IDENTIF, (tptr)&sig.user_ident)) return TRUE;
  memcpy(sig.enc_hash, enc_hash, sizeof(t_enc_hash));
  sig.state=sigstate_valid;
 // write the signature:
  if (tb_write_var(cdp, tables[tb], rec, attr, 0, sizeof(t_signature), &sig)) return TRUE;
  return FALSE;
}

void trace_sig(cdp_t cdp, t_user_ident * usid, char * dest, unsigned level)
{ char buf[sizeof(t_user_ident)+5+3]="";
  strcpy(buf, usid->name1);
  if (*usid->name2)
  { if (*buf) strcat(buf, " ");
    strcat(buf, usid->name2);
  }
  if (*usid->name3)
  { if (*buf) strcat(buf, " ");
    strcat(buf, usid->name3);
  }
  if (*usid->identif)
  { strcat(buf, " (");
    strcat(buf, usid->identif);
    strcat(buf, ")");
  }
  if (!*buf) strcpy(buf, "??");
  size_t len=strlen(buf);
  if (level)
    { buf[len++]=' ';  buf[len++]='#';  buf[len++]='0'+level-1;  buf[len]=0; }
  if (dest) strcpy(dest, buf);
  else
  { strcat(buf, "\r\n");
    tptr p=cdp->get_ans_ptr((int)strlen(buf));
    if (p) strcpy(p, buf);
  }
}

#if WBVERS<=810
static BOOL is_ca_key(cdp_t cdp, trecnum keyrec)  // for old keys only!
{ wbbool is_ca;
  return !tb_read_atr(cdp, KEY_TABLENUM, keyrec, KEY_ATR_CERTIF, (tptr)&is_ca) && is_ca;
}
#endif

t_sigstate check_signature_state(cdp_t cdp, ttablenum tb, trecnum rec, tattrib attr, BOOL trace)
{ 
 // read the signature:
  t_sigstate newstate=sigstate_notsigned;  t_signature sig;
 // read the signature:
  if (tb_read_var(cdp, tables[tb], rec, attr, 0, sizeof(t_signature), (tptr)&sig))
    { newstate=sigstate_error;  goto ex; }
  if (cdp->tb_read_var_result < sizeof(t_signature))
    { newstate=sigstate_notsigned;  goto wr; }
 // there is a signature
  if (trace) trace_sig(cdp, &sig.user_ident, NULL, trace);
  if (sig.state==sigstate_changed) goto ex; // do not change this
 // find the key who has signed it:
  tobjnum keyobj;
  keyobj=find_object_by_id(cdp, CATEG_KEY, sig.key_uuid);
  if (keyobj==NOOBJECT) 
    { newstate=sigstate_unknown;  goto wr; }
  uns32 keystatelen;
  tb_read_atr_len(cdp, tables[KEY_TABLENUM], keyobj, KEY_ATR_CERTIFS, &keystatelen);
  if (keystatelen==1)  // new certificate
  { t_hash_ex hsh;
    if (!calc_signature_ex(cdp, tb, rec, attr, &sig.user_ident, sig.dtm, hsh))
      { newstate=sigstate_error;  goto wr; }
   // get the certificate:
    uns32 cert_str_len;  tptr cert_str;
    cert_str = ker_load_blob(cdp, tables[KEY_TABLENUM], keyobj, KEY_ATR_PUBKEYVAL, &cert_str_len);
    if (!cert_str) { newstate=sigstate_error;  goto wr; }
    t_Certificate * cert;
    cert = make_certificate((unsigned char *)cert_str, cert_str_len);
    corefree(cert_str);
    if (!cert) { newstate=sigstate_error;  goto wr; }
    if (!verify_signature(cert, sig.enc_hash, sizeof(sig.enc_hash), hsh))
      newstate=sigstate_invalid;
    else  // verify the certificate
      newstate = verify_certificate(cdp, cert, sig.dtm, trace ? trace+1 : 0);
    DeleteCert(cert);
  }
  else
#if WBVERS<=810
  { t_hash hsh;
    if (!calc_signature(cdp, tb, rec, attr, &sig.user_ident, sig.dtm, hsh))
      { newstate=sigstate_error;  goto ex; }
    else
    {// decrypt the hash from the signature: sig.enc_hash
      CPubKey key;
      tb_read_var(cdp, tables[KEY_TABLENUM], keyobj, KEY_ATR_PUBKEYVAL, 0, 2*RSA_KEY_SIZE, (tptr)&key);
      t_hash decrypted_hsh;
      if (EDPublicDecrypt(decrypted_hsh, sig.enc_hash, &key) < 0)
        newstate=sigstate_error;
      else if (memcmp(hsh, decrypted_hsh, MD5_SIZE))
        newstate=sigstate_invalid;
      else if (tb==KEY_TABLENUM && is_top_ca_key(cdp, keyobj))
        newstate=sigstate_valid; // checked before time & CA flag
      else if (!in_validity_period(cdp, sig.dtm, keyobj))
        newstate=sigstate_time;
      else if (tb==KEY_TABLENUM && !is_ca_key(cdp, keyobj))
        newstate=sigstate_notca;
      else  // check the signature
        if (tb==KEY_TABLENUM && rec==(trecnum)keyobj) // cycle, but not top CA
          newstate=sigstate_notca;
        else newstate=GetKeyState(cdp, keyobj, trace ? trace+1 : 0) ?
               sigstate_valid : sigstate_uncertif;
    }
  }
#else
  newstate=sigstate_error;
#endif
 wr:
 // write the changed state:
  if (newstate!=sig.state && tb!=KEY_TABLENUM)
    tb_write_var(cdp, tables[tb], rec, attr, 0, 1, &newstate);
 ex:
  return newstate;
}

//////////////////////////////////////////////////////////////////////////////
//////////////// Comparing tables and cursors ////////////////////////////////

// Result 0 = no diff, 1 = recnum diff, 2 = attrnum diff, 3 = attrdef diff,
//        4 = data diff.

BOOL compare_tables(cdp_t cdp, tcursnum tb1, tcursnum tb2,
                    trecnum * diffrec, int * diffattr, int * result)
{ trecnum rec, reccnt1, reccnt2, reccnt;  int atr;  BOOL res;
  BOOL is_c1, is_c2;  int attrcnt;
  cur_descr *cd1, *cd2;  table_descr *tbdf1, *tbdf2;
  *diffrec = *diffattr = -1;

  is_c1 = IS_CURSOR_NUM(tb1);  is_c2 = IS_CURSOR_NUM(tb2);
 // analyse source 1:
  if (is_c1)
  { cd1 = get_cursor(cdp, tb1);
    if (!cd1) return FALSE; 
    make_complete(cd1);
    attrcnt = cd1->d_curs->attrcnt;
    reccnt1 = cd1->recnum;
  }
  else
  { tbdf1   = tables[tb1];
    attrcnt = tbdf1->attrcnt;
    reccnt1 = tbdf1->Recnum();
  }

 // analyse source 2:
  if (is_c2)
  { cd2 = get_cursor(cdp, tb2);
    if (!cd2) return FALSE; 
    make_complete(cd2);
    if (attrcnt != cd2->d_curs->attrcnt) { *result=2; return TRUE; }
    reccnt2 = cd2->recnum;
  }
  else
  { tbdf2   = tables[tb2];
    if (attrcnt != tbdf2->attrcnt) { *result=2; return TRUE; }
    reccnt2 = tbdf2->Recnum();
  }
  
  reccnt = reccnt1>reccnt2 ? reccnt1 : reccnt2;

 // compare attribute descriptions:
  for (atr=1;  atr<attrcnt;  atr++)
  { d_attr *att1, *att2;
    if (is_c1) att1 = &cd1->d_curs->attribs[atr];
    else       att1 = (d_attr*)(tbdf1->attrs+atr);
    if (is_c2) att2 = &cd2->d_curs->attribs[atr];
    else       att2 = (d_attr*)(tbdf2->attrs+atr);
    if (strcmp(att1->name, att2->name) ||
        att1->type!=att2->type || att1->multi!=att2->multi ||
        SPECIFLEN(att1->type) && att1->specif.length!=att2->specif.length)
      { *diffattr=atr;  *result=3;  return TRUE; }
  }

 // compare data:
  for (rec=0;  rec<reccnt;  rec++)
  { trecnum recnum1, recnum2;  const char * dt1, * dt2;  tfcbnum fcbn1, fcbn2;
    tattrib attr1, attr2;  elem_descr *attracc1, *attracc2;
    t_value exprval1, exprval2;
   /* seek and check to see if deleted: */
    uns8 dltd1, dltd2;
    if (is_c1)
    { cd1->curs_seek(rec);
      dltd1=cd1->cursor_record_state(rec);
    }
    else
    { dltd1=table_record_state(cdp, tbdf1, rec);
      recnum1 = rec;
      attracc1 = NULL;
    }
    if (is_c2)
    { cd2->curs_seek(rec);
      dltd2=cd2->cursor_record_state(rec);
    }
    else
    { dltd2=table_record_state(cdp, tbdf2, rec);
      recnum2 = rec;
      attracc2 = NULL;
    }
    if ((dltd1!=0) != (dltd2!=0))
      if (rec>=reccnt1 || rec>=reccnt2)
        { *diffrec=rec;  *diffattr=0;  *result=1;  return TRUE; }
      else
        { *diffrec=rec;  *diffattr=0;  *result=4;  return TRUE; }
    if (dltd1) continue;

   /* checking the other attributes */
    for (atr=1;  atr<attrcnt;  atr++)
    {// access the attribute 1:
      if (!is_c1) attr1=atr;
      else
      { attr_access(&cd1->qe->kont, atr, tb1, attr1, recnum1, attracc1);
        tbdf1=tables[tb1];
      }
      BOOL mult;  int  sz, tp;
      if (attracc1==NULL)
      { attribdef * att=tbdf1->attrs+attr1;
        mult = att->attrmult != 1;
        tp   = att->attrtype;
        sz   = typesize(att);
      }
      else
      { exprval1.set_null();
        { t_temporary_current_cursor tcc(cdp, cd1);  // sets the temporary current_cursor in the cdp until destructed
          expr_evaluate(cdp, attracc1->expr.eval, &exprval1);
        }
        tp=attracc1->type;          // fixed type supposed!
        mult=FALSE;  // FALSE supposed!
        sz = SPECIFLEN(tp)  ? attracc1->specif.length : tpsize[tp];
        if (exprval1.is_null()) qlset_null(&exprval1, tp, sz);
        dt1=exprval1.valptr();
      }

     // access the attribute 2:
      if (!is_c2) attr2=atr;
      else
      { attr_access(&cd2->qe->kont, atr, tb2, attr2, recnum2, attracc2);
        tbdf2=tables[tb2];
      }
      if (attracc2!=NULL)
      { exprval2.set_null();
        { t_temporary_current_cursor tcc(cdp, cd2);  // sets the temporary current_cursor in the cdp until destructed
          expr_evaluate(cdp, attracc2->expr.eval, &exprval2);
        }
        if (exprval2.is_null()) qlset_null(&exprval2, tp, sz);
        dt2=exprval2.valptr();
      }

     // compare the values:
      if (mult)  /* write the counter */
      { uns16 count1, count2;
        if (fast_table_read(cdp, tbdf1, recnum1, attr1, &count1)) count1=0;
        if (fast_table_read(cdp, tbdf2, recnum2, attr2, &count2)) count2=0;
        if (count1!=count2)
          { *diffrec=rec;  *diffattr=atr;  *result=4;  return TRUE; }
        for (int index=0;  index<count1;  index++)
        { dt1=fix_attr_ind_read(cdp, tbdf1, recnum1, attr1, index, &fcbn1);
          dt2=fix_attr_ind_read(cdp, tbdf2, recnum2, attr2, index, &fcbn2);
          if (dt1==NULL || dt2==NULL) return FALSE;
          if (IS_HEAP_TYPE(tp))
            res=*(const uns32*)(dt1+sizeof(tpiece)) != *(const uns32*)(dt2+sizeof(tpiece));
          else if (IS_STRING(tp))
            res=strncmp(dt1, dt2, sz);
          else
            res=memcmp(dt1, dt2, sz);
          unfix_page(cdp, fcbn1);  unfix_page(cdp, fcbn2);
          if (res)
            { *diffrec=rec;  *diffattr=atr;  *result=4;  return TRUE; }
        }
      }
      else
      { if (attracc1==NULL) dt1=fix_attr_read(cdp, tbdf1, recnum1, attr1, &fcbn1);
        if (attracc2==NULL) dt2=fix_attr_read(cdp, tbdf2, recnum2, attr2, &fcbn2);
        if (dt1==NULL || dt2==NULL) return FALSE;
        if (IS_HEAP_TYPE(tp))
        { int len1, len2;
          len1 = attracc1==NULL ? *(const uns32*)(dt1+sizeof(tpiece)) : exprval1.length;
          len2 = attracc2==NULL ? *(const uns32*)(dt2+sizeof(tpiece)) : exprval2.length;
          res=len1 != len2;
        }
        else if (IS_STRING(tp))
          res=strncmp(dt1, dt2, sz);
        else if (!IS_EXTLOB_TYPE(tp))
          res=memcmp(dt1, dt2, sz);
        if (attracc1==NULL) unfix_page(cdp, fcbn1);
        if (attracc2==NULL) unfix_page(cdp, fcbn2);
        if (res)
          { *diffrec=rec;  *diffattr=atr;  *result=4;  return TRUE; }
      }
    }
  }
  *result = 0;  // compared OK
  return TRUE;
}

