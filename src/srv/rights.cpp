/****************************************************************************/
/* rights.cpp - users & privileges                                         */
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
#include <stddef.h>

#define CACHE_RECORD_PRIVILS  // used by objects!

/* Privil API:
Privils(tobjnum user_group_role, int subject_type,
        ttablenum table, trecnum record,
        int operation, uns8 * privils)

subject_type:
0 - user,
1 - group,
2 - role.

operation:
0 - set privils,
1 - get subject's own privils,
2 - get subject's effective privils.

record:
-1    - global table privils
other - specific record privils

privils:
[0] -      INS, DEL, NEW_REC_EVR_READ, NEW_REC_EVR_WRITE,
           IS_ALL_RD, IS_ALL_WR, GRANT
           (only DEL, GRANT for record!=-1), IS_ALL_ are computed from the other privileges
[1]-end - read/write atribute privils (2 bits per attribute)

Effective privils:
- an user inherits privileges from its groups (including the inherited ones) and privileges from its roles;
- a group inheris privileges from its roles;
- a record inherits privileges from the table.

Caches: drzet zvlast prava cele tabulky a zaznamu, z hlediska skupin pouze efektivni;

cache_start ukazuje na prvni zaznam v cyklickem bufferu, cache_stop za
posledni. Cache nemuze byt uplne plna, kdyz jsou si rovny, je prazdna.
Za platnym zaznamem na koncu bufferu je delka nula, leda ze tam ukazuje
cache_stop. Na delku nula nikdy neukazuje cache_start.

Pri vlastni zmene v pravech se vymaze cache pomoci reset_cache. Pokud jiny
proces zmeni prava nebo kdokoli zmeni hierarchii, nastavi se cache_invalid
a procesy pred pouzitim vlastni cache si vymazou jeji obsah prip. prebuduji
hierarchii -  jinak by mohlo dojit ke konfliktum.

Popis prav v objektu:
- 1 bajt nevyuzity (drive delka popisu prav (bez UUID));
- posloupnost zaznamu ve tvaru:
-- UUID subjektu;
-- popis prav (v 1 bajtu globalni prava, pak atributova), delka viz vyse.

Skupina DB_ADMIN:
- vlastní práva se jí nepøidìlují, ale má všechna;
- efektivní: kdo je èlenem , získá automaticky vše kromì RIGHT_NEW_...

Semafor chrani zapis vlastnich prav uzivatele a editaci nalezeni do skupin a roli.
Neni chraneno cteni var/len objektu behem jeho prepisu.

Zruseni subjektu prav: Subjekt se neodtranuje z popisu prav u objektu. Pokud jde
o uzivatele nebo skupinu, nevadi to, protoze maji jedinecne UUID. Pro roli plati, 
ze se nezrusi, pouze se vyradi z aplikace, takze nebude nikdy vice pouzita.

*/

tptr load_var_val(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib atr, unsigned * size)
// Returns NULL on error. Must return non-NULL for empty value loaded.
{ uns32 len;  uns8 dltd;  tptr buf;
  if (tb_read_atr(cdp, tbdf, recnum, DEL_ATTR_NUM, (tptr)&dltd) || dltd==RECORD_EMPTY) 
    goto error;
  if (tb_read_atr_len(cdp, tbdf, recnum, atr, &len)) goto error;
  buf=(tptr)corealloc(len+1, 94);
  if (!buf) 
  { if (!header.berle_used) request_error(cdp, OUT_OF_KERNEL_MEMORY);  
    goto error; 
  }
  if (tb_read_var(cdp, tbdf, recnum, atr, 0, len, buf))
    { corefree(buf);  goto error; }
  *size=cdp->tb_read_var_result;
  buf[*size]=0;   // not necessary
  return buf;
 error:
  if (!header.berle_used) return NULL;
  request_error(cdp, 0);   /* resets errors */
  buf=(tptr)corealloc(1, 94);
  *size=0;
  return buf;
}

static bool is_role_uuid(const WBUUID uuid, tobjnum role_num)
// Checks if [uuid] is the privil subject uuid of role [role_num].
{ if (*(tobjnum*)uuid != role_num) return false;
  for (int i=sizeof(tobjnum);  i<UUID_SIZE;  i++)
    if (uuid[i]!=0xff) return false;
  return true;
}

tobjnum role_from_uuid(const WBUUID p_uuid)
{ int j=sizeof(tobjnum);  
  while (j<UUID_SIZE)
    if (p_uuid[j++]!=0xff) return NOOBJECT;
  return *(tobjnum*)p_uuid;
}

//***************************** privils private
const WBUUID data_adm_uuid = { 1,1,1,1, 1,1,1,1, 1,1,1,0 };
const WBUUID anonymous_uuid= { 1,1,1,1, 1,1,1,1, 1,1,1,1 };
const WBUUID everybody_uuid= { 1,1,1,1, 1,1,1,1, 1,1,1,2 };
const WBUUID conf_adm_uuid = { 1,1,1,1, 1,1,1,1, 1,1,1,3 };
const WBUUID secur_adm_uuid= { 1,1,1,1, 1,1,1,1, 1,1,1,4 };
CRITICAL_SECTION privil_sem;  // currently not used

t_privil_version privil_version; // global privilege version counter

static void reset_privil_caches(BOOL reset_hierarchy)
{ ProtectedByCriticalSection cs(&cs_client_list);  
  for (int i=0;  i<=max_task_index;  i++)   /* system process 0 not reset */
    if (cds[i] != NULL)
      cds[i]->prvs.invalidate_cache(reset_hierarchy ? 2 : 1);  // just marks as invalid, does not rewrite the contets
}

int privils::find_uuid_pos(uns8 * priv_def, unsigned def_size, uns8 * uuid, int step)
{ if (!def_size) return -1;  // privileges not defined
  for (unsigned pos=1;  pos+step <= def_size;  pos+=step)
    if (!memcmp(uuid, priv_def+pos, UUID_SIZE)) // UUID found
      return pos;
  return -1;
}

BOOL privils::cache_next(unsigned &pos)
{ //if (pos=cache_stop) return FALSE;  -- is supposed!
  t_size_info sz=((privil_cache_entry*)(p_cache+pos))->entry_size;
  if (!sz) pos=0; else pos+=sz;
  return TRUE;
}

void privils::cache_insert(ttablenum tb, trecnum record, int descr_size, uns8 * priv_val)
{ int entry_size = offsetof(privil_cache_entry, priv_val)+descr_size;
 // wrap buffer end:
  if (cache_stop+entry_size > PRIVIL_CACHE_SIZE) // nevejde se na konec bufferu
  { if (entry_size > PRIVIL_CACHE_SIZE) return;  // cannot be in the cache!
    ((privil_cache_entry*)(p_cache+cache_stop))->entry_size=0;
    if (cache_start >= cache_stop) cache_start=0;
    cache_stop=0;
  }
 // will be copied on cache_stop, make space (move cache_start):
  while (cache_stop < cache_start &&
         cache_stop+entry_size >= cache_start) // uvolnovani mista
  { cache_start+=((privil_cache_entry*)(p_cache+cache_start))->entry_size;
    if (!((privil_cache_entry*)(p_cache+cache_start))->entry_size)
    { cache_start=0;
      break;
    }
  }
 // copy info:
  privil_cache_entry * pc = (privil_cache_entry *)(p_cache+cache_stop);
  pc->entry_size=(t_size_info)entry_size;
  pc->record=record;
  pc->tb=tb;
  memcpy(pc->priv_val, priv_val, descr_size);
  cache_stop+=entry_size;
}

bool privils::is_in_groupdef(const WBUUID uuid) const
{ const uns8 * an_id = groupdef;  int cnt = id_count;  
  while (cnt--)
  { if (!memcmp(an_id, uuid, UUID_SIZE)) 
      return true;
    an_id+=UUID_SIZE;
  }
  return temp_author!=NOOBJECT && is_role_uuid(uuid, temp_author);
}

void privils::sumarize_privils(unsigned descr_size, uns8 * priv_val, const uns8 * priv_def, unsigned def_size)
// Adds privileges from priv_def to priv_val according to groupdef
// Supposed priv_def!=NULL, def_size > 0
{ memset(priv_val, 0, descr_size);  
  if (!def_size) return;
  unsigned step=UUID_SIZE+descr_size;  
  priv_def++;  def_size--;  // skip the 1st byte, it used to contain the descr_size in previous versions
  while (def_size >= step)
  {// check UUID, add to [priv_val] if found in groupdef:
    if (is_in_groupdef(priv_def))
      for (int i=0;  i<descr_size;  i++) priv_val[i] |= priv_def[UUID_SIZE+i];
   // next privilege:
    priv_def+=step;  def_size-=step;
  }
}

BOOL privils::copy_up_uuids(uuid_dynar * uuids, tobjnum userobj)
{ unsigned pos, size;
  uns8 * user_ups=(uns8*)load_var_val(cdp, usertab_descr, userobj, USER_ATR_UPS, &size);
  if (user_ups!=NULL)
  { for (pos=0;  pos+UUID_SIZE <= size;  pos+=UUID_SIZE)
    { if      (!memcmp(data_adm_uuid,  user_ups+pos, UUID_SIZE))
        is_data_admin=TRUE;
      else if (!memcmp(conf_adm_uuid,  user_ups+pos, UUID_SIZE))
        is_conf_admin=TRUE;
      else if (!memcmp(secur_adm_uuid, user_ups+pos, UUID_SIZE))
        is_secur_admin=TRUE;
     // add the group/role UUID to the list. db_admin_uuid adding too, used by Get_membership(user, CATEG_USER, "db_admin", CATEG_GROUP, TRUE)
      uns8 * p_uuid=*uuids->next();
      if (p_uuid==NULL) return TRUE;
      memcpy(p_uuid, user_ups+pos, UUID_SIZE);
    }
    corefree(user_ups);
  }
  return FALSE;
}

//***************************** privils public ******************************

BOOL privils::set_user(tobjnum userobj, tcateg subject_categ, BOOL create_hierarchy)
{ uuid_dynar uuids;  uns8 * p_uuid;
  is_data_admin=is_conf_admin=is_secur_admin=FALSE;  
  set_temp_author(NOOBJECT);  // stops the temporary authoring mode
  *user_objname=0;
  if (tables==NULL || tabtab_descr==NULL || usertab_descr==NULL)
    { user_objnum=userobj;  return FALSE; }
  p_uuid=*uuids.acc(1);
  if (p_uuid==NULL) return TRUE;
 // start of the hierarchy: EVERYBODY group or nothing
  if (create_hierarchy && subject_categ==CATEG_USER && userobj!=ANONYMOUS_USER)
    memcpy(uuids.acc(0), everybody_uuid, UUID_SIZE);
  else memset(uuids.acc(0), 0xfe, UUID_SIZE);  // non-existing UUID
 // insert own uuid into the list, store own uuid and objnum:
  if (subject_categ != CATEG_ROLE)
  { user_objnum=userobj;
    tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_LOGNAME, user_objname);
    if (userobj==EVERYBODY_GROUP) memcpy(p_uuid, everybody_uuid, UUID_SIZE);  // used when creating standard tables
    else tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_UUID, (tptr)p_uuid);
    memcpy(user_uuid, p_uuid, UUID_SIZE);
    if      (user_objnum==DB_ADMIN_GROUP)  is_data_admin =is_conf_admin=TRUE; // used by system threads
    else if (user_objnum==CONF_ADM_GROUP)  is_conf_admin =TRUE;  // used by _on_server_start
    else if (user_objnum==SECUR_ADM_GROUP) is_secur_admin=TRUE;  // not used 
  }
  else
  { user_objnum=NOOBJECT;
    memset(p_uuid, 0xff, UUID_SIZE);  *(tobjnum*)p_uuid=userobj;
  }
 // add the hierarchy:
  if (create_hierarchy && subject_categ != CATEG_ROLE)
  { copy_up_uuids(&uuids, userobj);   // groups & roles of the user or roles the group plays
    if (subject_categ == CATEG_USER)  // add roles the added groups play
    { int last=uuids.count();  // end of list of direct groups & roles
      if (userobj!=ANONYMOUS_USER)
        copy_up_uuids(&uuids, EVERYBODY_GROUP); // roles the EVERYBODY plays
      for (int i=2;  i<last;  i++)            // adds roles of groups other than EVERYBODY
      { p_uuid=*uuids.acc(i);
        if (role_from_uuid(p_uuid)==NOOBJECT)  // expand the group!
        { user_key key;  trecnum recnum;
          dd_index * indx = &usertab_descr->indxs[1];
          memcpy(key.uuid, p_uuid, UUID_SIZE);  key.recnum=0;
          cdp->index_owner = usertab_descr->tbnum;
          if (find_key(cdp, indx->root, indx, (tptr)&key, &recnum, true))
            // group found!
            copy_up_uuids(&uuids, (tobjnum)recnum); // roles the group plays
        }
      }
    }
  }
 // move uuids contents to the groupdef list, set id_count:
  corefree(groupdef);
  groupdef=(uns8*)corealloc(UUID_SIZE*uuids.count(), 86);
  if (groupdef==NULL) { id_count=0;  return TRUE; }
  memcpy(groupdef, uuids.acc(0), UUID_SIZE*uuids.count()); 
  id_count=uuids.count();
 // reset cache:
  reset_cache();
  if (&cdp->prvs==this) cdp->trace_enabled=trace_map(user_objnum);
  return FALSE;
}

void privils::mark_core(void)
{ if (groupdef) mark(groupdef); }

bool privils::get_effective_privils(table_descr * tbdf, trecnum rec, t_privils_flex & priv_val)
{ priv_val.init(tbdf->attrcnt);
// tb descr is supposed to be locked in this function!
  unsigned cpos;  privil_cache_entry * pc;  bool found;
  unsigned privil_descr_size = tbdf->privil_descr_size();
 // reset the cache contents if invalidation flag is set: 
  if (cache_invalid)
  { reset_cache();
    if (cache_invalid==2) reset_hierarchy();
    cache_invalid=0;
  }
 // search global privileges (RIGHT_NEW_... for admin):
  cpos=cache_start;  found=false;
  while (cpos!=cache_stop)
  { pc=(privil_cache_entry*)(p_cache+cpos);
    if (pc->tb==tbdf->tbnum && pc->record==-1) { found=true;  break; }
    cache_next(cpos);
  }
 // take or calculate the global privileges:
  if (found)  // copy privileges from the cache
    memcpy(priv_val.the_map(), pc->priv_val, privil_descr_size);
  else if (tbdf->tbnum<0)
  { priv_val.set_all_rw();
    *priv_val.the_map()=RIGHT_READ|RIGHT_WRITE|RIGHT_INSERT|RIGHT_DEL;
  }
  else        // calcucate privileges, store them in the cache, too
  { unsigned def_size;  uns8 * priv_def;
    priv_def=(uns8*)load_var_val(cdp, tabtab_descr, tbdf->tbnum, TAB_DRIGHT_ATR, &def_size);
    if (priv_def!=NULL)
    { sumarize_privils(privil_descr_size, priv_val.the_map(), priv_def, def_size);
      cache_insert(tbdf->tbnum, NORECNUM, privil_descr_size, priv_val.the_map());  // unless error or empty priv_def
      corefree(priv_def);
    }
  }

 // admin: add admin's privileges and return - no need to search record privileges:
  if (is_data_admin)
  { priv_val.set_all_rw();
    *priv_val.the_map() |= RIGHT_INSERT | RIGHT_DEL | RIGHT_GRANT | RIGHT_READ | RIGHT_WRITE; // RIGHT_NEW_ retreived above
    return false;
  }

 // search record privileges (if required & exist), add them if found:
  if (tbdf->rec_privils_atr!=NOATTRIB && rec!=NORECNUM && rec!=OUTER_JOIN_NULL_POSITION)
  { uns8 * priv_val0;  t_privils_flex rec_privils(tbdf->attrcnt);
#ifdef CACHE_RECORD_PRIVILS
    cpos=cache_start;  found=false;
    while (cpos!=cache_stop)
    { pc=(privil_cache_entry*)(p_cache+cpos);
      if (pc->tb==tbdf->tbnum && pc->record==rec) { found=true;  break; }
      cache_next(cpos);
    }
   // take or calculate the record privileges to priv_val0:
    if (found)  // copy privileges from the cache
      priv_val0=pc->priv_val;
    else        // calcucate privileges, store them in the cache, too
#endif
    { priv_val0=rec_privils.the_map();
      unsigned def_size;
      uns8 * priv_def=(uns8*)load_var_val(cdp, tbdf, rec, tbdf->rec_privils_atr, &def_size);
      if (priv_def!=NULL)
      { sumarize_privils(privil_descr_size, priv_val0, priv_def, def_size);
#ifdef CACHE_RECORD_PRIVILS
        cache_insert(tbdf->tbnum, rec, privil_descr_size, priv_val0);
#endif
        corefree(priv_def);
      }
      else return false;
    }
   // Add the record privileges:
    for (unsigned i=0;  i<privil_descr_size;  i++) priv_val.the_map()[i] |= priv_val0[i];
  }
  return false;
} // get_effective_privils

bool privils::get_own_privils(table_descr * tbdf, trecnum rec, t_privils_flex & priv_val)
{ 
  priv_val.init(tbdf ? tbdf->attrcnt : t_privils_flex::static_map_col_cnt);
  if (user_objnum==DB_ADMIN_GROUP)  // mostly not used
  { priv_val.set_all_rw();
    *priv_val.the_map()=RIGHT_INSERT | RIGHT_DEL | RIGHT_GRANT | RIGHT_READ | RIGHT_WRITE;
    return false;
  }
  priv_val.clear();
  table_descr * dest_tbdf;  tattrib atr;
 // find the location of the privileges:
  if (rec==NORECNUM)  // reading global privileges, stored in TABTAB
  { rec=tbdf->tbnum;
    dest_tbdf=tabtab_descr;
    atr=TAB_DRIGHT_ATR;
  }
  else  // reading record privileges
  { dest_tbdf=tbdf;
    atr=tbdf->rec_privils_atr;
    if (atr==NOATTRIB)
      { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return true; }
  }
 // load the privileges, check if initialized, find UUID in them:
  unsigned def_size;  uns8 * priv_def = (uns8*)load_var_val(cdp, dest_tbdf, rec, atr, &def_size);
  if (priv_def==NULL || !def_size)  // privileges not initialied yet
    { corefree(priv_def);  return false; }
  int rd_pos=find_uuid_pos(priv_def, def_size, groupdef+UUID_SIZE, UUID_SIZE+tbdf->privil_descr_size());
  if (rd_pos!=-1)  // when owner UUID is not found, result is null
    memcpy(priv_val.the_map(), priv_def+rd_pos+UUID_SIZE, tbdf->privil_descr_size());
  corefree(priv_def);
  return false;
} // get_own_privils

bool privils::get_cursor_privils(cur_descr * cd, trecnum rec, t_privils_flex & priv_val, bool effective)
// Inits and defines [priv_val]. Returns TRUE on error.
{ 
  priv_val.init(cd->d_curs->attrcnt);
  priv_val.clear();
  if (cd->insensitive)
  { *priv_val.the_map() = RIGHT_READ | RIGHT_DEL;
    priv_val.set_all_r();  // can read everythink
  }
  else
  { trecnum recnums[MAX_CURS_TABLES];
    if (cd->curs_seek(rec, recnums)) return TRUE;
   // load record privils in tables:
    t_privils_flex * trp = new t_privils_flex[cd->tabcnt];
    if (trp==NULL) return TRUE;
    for (unsigned i=0;  i<cd->tabcnt;  i++)
    { BOOL res;
      ttablenum tb = cd->tabnums[i];  // tabdescr is locked
      if (tb>=0 && tb!=(ttablenum)0x8000 && tb!=(ttablenum)0x8001 && tb!=(ttablenum)0x8002)
        if (effective)
          res=get_effective_privils(tables[tb], recnums[i], trp[i]);
        else
          res=get_own_privils      (tables[tb], recnums[i], trp[i]);
      else
      { trp[i].set_all_rw();  *trp[i].the_map()=0xff;
        res=FALSE;
      }
      if (res) { delete [] trp;  return TRUE; }
    }
   // define cursor column privils:
    bool can_read_all = true, can_write_all = true;  int ca;
    for (ca=1;  ca<cd->d_curs->attrcnt;  ca++)
    { elem_descr * attracc;  ttablenum tb;  unsigned tbindex;  tattrib attr;  trecnum recnum;
      attr_access(&cd->qe->kont, ca, tb, attr, recnum, attracc);
      if (attracc)
      { priv_val.add_read(ca);  // supposed, the user can try to read it
        can_write_all=false;
      }
      else
      { for (tbindex=0;  tbindex<cd->tabcnt;  tbindex++)
          if (cd->tabnums[tbindex]==tb) break; 
        if (tbindex<cd->tabcnt)
        { if (trp[tbindex].has_read(attr)) priv_val.add_read(ca);
          else can_read_all=false;
          if (trp[tbindex].has_write(attr)) priv_val.add_write(ca);
          else can_write_all=false;
        }
      }
    }
    if (can_read_all ) *priv_val.the_map() |= RIGHT_READ;
    if (can_write_all) *priv_val.the_map() |= RIGHT_WRITE;
    if (cd->tabcnt==1) 
    { if (*trp[0].the_map() & RIGHT_DEL)    *priv_val.the_map() |= RIGHT_DEL;
      if (*trp[0].the_map() & RIGHT_INSERT) *priv_val.the_map() |= RIGHT_INSERT;
    }
    delete [] trp;
  }
  return FALSE;
}

bool privils::set_own_privils(table_descr * tbdf, trecnum rec, t_privils_flex & priv_val)
// grantor's privileges are supposed to be checked
{ 
  if (user_objnum==DB_ADMIN_GROUP) return false; // ignored but no error, used when creating standard tables
  if (tbdf==NULL) return true;
  unsigned def_size;  table_descr * dest_tbdf;  tattrib atr;
  bool res;  int wr_pos;  uns8 * priv_def;
  if (user_objnum==DB_ADMIN_GROUP)  return false; // ignored but no error, used when creating standard tables
 // find the location of the privileges (tb locked now):
  if (rec==NORECNUM)
  { rec=tbdf->tbnum;
    dest_tbdf=tabtab_descr;
    atr=TAB_DRIGHT_ATR;
  }
  else
  { dest_tbdf=tbdf;
    atr=tbdf->rec_privils_atr;
    if (atr==NOATTRIB)
      { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return true; }
    *priv_val.the_map() &= RIGHT_GRANT | RIGHT_DEL | RIGHT_READ | RIGHT_WRITE; // clear the global privileges, must be ignored
  }
 // calculate the derived privileges:
  bool all_read, all_write;  int i;
  all_read=all_write=true;
  *priv_val.the_map() &= ~(RIGHT_READ | RIGHT_WRITE);
  for (i=1;  i<tbdf->attrcnt;  i++)
  { if (IS_SYSTEM_COLUMN(tbdf->attrs[i].name)) continue;
    if (!priv_val.has_read (i)) all_read =false;
    if (!priv_val.has_write(i)) all_write=false;
  }
  if (all_read ) *priv_val.the_map() |= RIGHT_READ;
  if (all_write) *priv_val.the_map() |= RIGHT_WRITE;

 // load the privileges, check if initialized, find UUID in them:
  unsigned privil_descr_size = tbdf->privil_descr_size();
  if (wait_record_lock_error(cdp, dest_tbdf, rec, TMPWLOCK)) return TRUE;
  priv_def=(uns8*)load_var_val(cdp, dest_tbdf, rec, atr, &def_size);
  if (priv_def==NULL || !def_size)  // privileges not initialied yet or error
  { if (tb_write_var(cdp, dest_tbdf, rec, atr, 0, 1, &privil_descr_size))  // not used any more
      { corefree(priv_def);  return true; }
    wr_pos=1;
  }
  else  // find UUID in privileges:
  { wr_pos=find_uuid_pos(priv_def, def_size, groupdef+UUID_SIZE, UUID_SIZE+privil_descr_size);
    if (wr_pos==-1) wr_pos=def_size; // not found
  }
  corefree(priv_def);
 // write the privileges value:
  res=tb_write_var(cdp, dest_tbdf, rec, atr, wr_pos,           UUID_SIZE,         groupdef+UUID_SIZE)!=0;
  res=tb_write_var(cdp, dest_tbdf, rec, atr, wr_pos+UUID_SIZE, privil_descr_size, priv_val.the_map())!=0;
  reset_privil_caches(FALSE);  privil_version.new_privil_version();
  return res;
} // set_own_privils

BOOL privils::can_grant_privils(cdp_t cdp, privils * prvs, table_descr * tbdf, trecnum rec, t_privils_flex & a_priv_val)
// Checks the difference between rights owned by the user given in [prvs] and right specified for him in [a_priv_val].
// If there is any difference, the same bit must be contained in grantor's rights and the grantor must have the RIGHT_GRANT.
// Otherwise FALSE is returned.
{ t_privils_flex my_priv_val(tbdf->attrcnt), his_priv_val(tbdf->attrcnt);
  if (cdp->has_full_access(tbdf->schema_uuid)) return TRUE;
  get_effective_privils(tbdf, rec,  my_priv_val);
  prvs->get_own_privils(tbdf, rec, his_priv_val);
  bool must_grant=false;
  for (int i=0;  i<1+my_priv_val.colbytes();  i++)
    if (a_priv_val.the_map()[i] ^ his_priv_val.the_map()[i]) // a privilege changed
    { must_grant=true;  // there is a difference
      if ((a_priv_val.the_map()[i] ^ his_priv_val.the_map()[i]) & ~my_priv_val.the_map()[i])
      { unsigned diff=(a_priv_val.the_map()[i] ^ his_priv_val.the_map()[i]) & ~my_priv_val.the_map()[i];
        if (i==0)
        { if (diff & ~(RIGHT_NEW_READ|RIGHT_NEW_WRITE|RIGHT_NEW_DEL|RIGHT_READ|RIGHT_WRITE)) return FALSE;  // no excuse here
          continue;
        }
        int attr=1+4*(i-1);  // offending attribute
        if (!(diff & 0x3))
        { attr++;
          if (!(diff & 0xc))
          { attr++;
            if (!(diff & 0x30)) attr++;
          }
        }
        //ttablenum tb2 = (ttablenum)(rec==-1 ? TAB_TABLENUM : tb);
        if (attr < tbdf->attrcnt) return FALSE;
        else break;  // no need to check the others
      }
    }
  if (must_grant)
    if (!(*my_priv_val.the_map() & RIGHT_GRANT)) return FALSE;
  return TRUE;
}

BOOL privils::any_eff_privil(table_descr * tbdf, trecnum rec, BOOL write_privil)
{ t_privils_flex priv_val;
  get_effective_privils(tbdf, rec, priv_val);
  if (tbdf->tbnum==TAB_TABLENUM || tbdf->tbnum==OBJ_TABLENUM)  // for locking the object definition checking the rights to the definition
    return write_privil ? priv_val.has_write(OBJ_DEF_ATR) : priv_val.has_read(OBJ_DEF_ATR);
  if (write_privil)
  { if (*priv_val.the_map() & RIGHT_WRITE) return TRUE;
    for (int a=1;  a<tbdf->attrcnt;  a++)
      if (priv_val.has_write(a)) return TRUE;
  }
  else
  { if (*priv_val.the_map() & RIGHT_READ ) return TRUE;
    for (int a=1;  a<tbdf->attrcnt;  a++)
      if (priv_val.has_read(a)) return TRUE;
  }
  return FALSE;
}

BOOL privils::convert_list(table_descr * tbdf, trecnum rec, tattrib atr, tattrib * map, int old_atr_count, int new_atr_count, bool global)
// Converts a list of privils according to the map.
// For new columns assings full privils to the user performing the ALTER and appopriate privils to the standard roles
{ unsigned def_size;  uns8 * old_priv_def, * new_priv_def;
  t_privils_flex old_priv_val(old_atr_count), new_priv_val(new_atr_count);  
 // load the old list of privils:
  old_priv_def=(uns8*)load_var_val(cdp, tbdf, rec, atr, &def_size);
  if (old_priv_def==NULL || !def_size) 
    { corefree(old_priv_def);  return FALSE; } // privileges not initialied yet or error
 // calculate the list segment sizes:
  int old_descr_size=SIZE_OF_PRIVIL_DESCR(old_atr_count);
  int new_descr_size=SIZE_OF_PRIVIL_DESCR(new_atr_count);
  unsigned old_step=UUID_SIZE+old_descr_size;
  unsigned new_step=UUID_SIZE+new_descr_size;
 // create and initialize the new list:
  new_priv_def=(uns8*)corealloc(1+((def_size-1)/old_step+1)*new_step, 89); // +1 for posible added own descriptor
  if (!new_priv_def) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
  *new_priv_def=(uns8)new_descr_size;  // not used in new servers
 // pass the elements & convert them:
  unsigned pos, wr_pos;  uns8 * old_segment;
  bool owner_stored;  
  owner_stored=false; 
  for (pos=wr_pos=1, old_segment=old_priv_def+1;  pos+old_step <= def_size;  pos+=old_step, wr_pos+=new_step, old_segment+=old_step)
  { memcpy(old_priv_val.the_map(), old_segment+UUID_SIZE, old_descr_size);
    bool own_privils=!memcmp(old_segment, user_uuid, UUID_SIZE); // modifying user own privils
    if (own_privils) owner_stored=true;
    new_priv_val.clear();
    *new_priv_val.the_map() = *old_priv_val.the_map();  // global privils
    for (int na=1;  na<new_atr_count;  na++)
    { tattrib oa=map[na];
      if (oa!=NOATTRIB) // copy the existing privils for the existing column
      { if (old_priv_val.has_read (oa)) new_priv_val.add_read (na);
        if (old_priv_val.has_write(oa)) new_priv_val.add_write(na);
      }
      else if (global)  // new column, no record privils, default global privils
      { if (own_privils) // defining privils of the user which is altering the table
        { new_priv_val.add_read (na);
          new_priv_val.add_write(na);
        }
        else if (is_role_uuid(old_segment, cdp->junior_role))
          new_priv_val.add_read(na);
        else if (is_role_uuid(old_segment, cdp->senior_role) || is_role_uuid(old_segment, cdp->admin_role) || is_role_uuid(old_segment, cdp->author_role))
        { new_priv_val.add_read(na);
          new_priv_val.add_write(na);
        }
      }
    }
    memcpy(new_priv_def+wr_pos,           old_segment,            UUID_SIZE);
    memcpy(new_priv_def+wr_pos+UUID_SIZE, new_priv_val.the_map(), new_descr_size);
  }
 // when privils for the user altering the table have not been specified, add them:
  if (global && !owner_stored)
  { memcpy(new_priv_def+wr_pos,           user_uuid, UUID_SIZE);
    memset(new_priv_def+wr_pos+UUID_SIZE, 0xff,      new_descr_size);
    new_priv_def[wr_pos+UUID_SIZE] = RIGHT_INSERT | RIGHT_DEL | RIGHT_GRANT | RIGHT_READ | RIGHT_WRITE;
    wr_pos += new_step;
  }
 // write the new list:
  if (tb_write_var(cdp, tbdf, rec, atr, 0, wr_pos, new_priv_def)) goto err;
  corefree(old_priv_def);  corefree(new_priv_def);  
  return FALSE;
 err:
  corefree(old_priv_def);  corefree(new_priv_def);  
  return TRUE;
}

BOOL privils::convert_privils(table_descr * tbdf, table_all * ta_old, table_all * ta_new)
{ 
 // create the attribute translation map: map[new column number] == old_column number, NOATTRIB for new columns.
  tattrib * map = (tattrib *)corealloc(sizeof(tattrib)*ta_new->attrs.count(), 56);
  if (!map) return TRUE;
  int i,j;
  for (i=1;  i<ta_new->attrs.count();  i++) // every new column...
  { atr_all * aln = ta_new->attrs.acc0(i);
    map[i]=NOATTRIB; // not mapped as default
    for (j=1;  j<ta_old->attrs.count();  j++) // ... search among old column names
    { atr_all * alo = ta_old->attrs.acc0(j);
      if (!strcmp(aln->name, alo->name))
        { map[i]=j;  break; }
    }
  }
  if (wait_record_lock_error(cdp, tbdf, FULLTABLE, TMPWLOCK)) 
    { corefree(map);  return TRUE; }
// convert global privils
  convert_list(tabtab_descr, tbdf->tbnum, TAB_DRIGHT_ATR, map, ta_old->attrs.count(), ta_new->attrs.count(), true);
 // convert record privils:
  if (tbdf->rec_privils_atr!=NOATTRIB)
  { for (trecnum rec = 0;  rec<tbdf->Recnum();  rec++)
      if (!table_record_state(cdp, tbdf, rec))
        convert_list(tbdf, rec, tbdf->rec_privils_atr, map, ta_old->attrs.count(), ta_new->attrs.count(), false);
  }
  reset_privil_caches(FALSE);  privil_version.new_privil_version();
  corefree(map);
  return FALSE;
}
/****************************************************************************/
static int first_user_attr(d_table * td)
{ for (int i=1;  i<td->attrcnt;  i++)
    if (!IS_SYSTEM_COLUMN(ATTR_N(td,i)->name)) return i;
  return 0;
}

BOOL check_all_right(cdp_t cdp, tcursnum curs, BOOL write_rg)
/* Supposes that table_descr of curs is locked. */
// Returs FALSE iff user has global privileges to all attributes
// Used when saving/loading a table/cursor.
{ if (IS_CURSOR_NUM(curs))  /* cannot be temporary table */
  { int i, mask = write_rg ? RI_UPDATE_FLAG : RI_SELECT_FLAG;
    d_table * tdd;
    { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
      tdd = (*crs.acc(GET_CURSOR_NUM(curs)))->d_curs;
    }
    for (i=first_user_attr(tdd);  i<tdd->attrcnt;  i++)
      if (!(tdd->attribs[i].a_flags & mask)) return TRUE;
    return FALSE;
  }
  else
  { t_privils_flex priv_val;
    cdp->prvs.get_effective_privils(tables[curs], NORECNUM, priv_val);
    if (*priv_val.the_map() & (write_rg ? RIGHT_WRITE : RIGHT_READ)) return FALSE;
    return TRUE;
  }
}

BOOL remove_app_roles(cdp_t cdp, tobjnum applobj)
{ WBUUID apl_uuid;  trecnum rec;
 // get appl uuid:
  if (tb_read_atr(cdp, objtab_descr, applobj, APPL_ID_ATR, (tptr)apl_uuid))
    return TRUE;
 // list roles of the application:
  uuid_dynar uuids;
  for (rec=0;  rec < objtab_descr->Recnum();  rec++)
  { tfcbnum fcbn;
    const ttrec * dt = (const ttrec*)fix_attr_read(cdp, objtab_descr, rec, DEL_ATTR_NUM, &fcbn);
    if (dt!=NULL)
    { if (!dt->del_mark && dt->categ==CATEG_ROLE &&
          !memcmp(dt->apluuid, apl_uuid, UUID_SIZE))
      { uns8 * p_uuid=*uuids.next();
        if (p_uuid!=NULL)
        { memset(p_uuid, 0xff, UUID_SIZE);
          *(tobjnum*)p_uuid=(tobjnum)rec;
        }
      }
      unfix_page(cdp, fcbn);
    } // fixed
  } // cycle

 // go through users & groups, remove role from them:
  for (rec=0;  rec < usertab_descr->Recnum();  rec++)
  { tfcbnum fcbn;
    const ttrec * dt = (const ttrec*)fix_attr_read(cdp, usertab_descr, rec, DEL_ATTR_NUM, &fcbn);
    if (dt!=NULL)
    { if (!dt->del_mark)
      { unsigned def_size;
        uns8 * user_ups=(uns8*)load_var_val(cdp, usertab_descr, rec, USER_ATR_UPS, &def_size);
        if (user_ups!=NULL)
        { def_size=def_size/UUID_SIZE*UUID_SIZE;  // alignment
          unsigned pos=0;  BOOL changed=FALSE;
          while (pos < def_size)  // def_size aligned, test is OK
          { bool found=false;
            if (user_ups[pos+sizeof(tobjnum)] == 0xff)  // pre-test
              for (int i=0;  i<uuids.count();  i++)
              { if (!memcmp(user_ups+pos, *uuids.acc(i), UUID_SIZE))
                  { found=true;  break; }
              }
            if (found)
            { def_size-=UUID_SIZE;
              memmov(user_ups+pos, user_ups+pos+UUID_SIZE, def_size-pos);
              changed=TRUE;
            }
            else pos+=UUID_SIZE;
          }
          if (changed)   // found
          { tb_write_var    (cdp, usertab_descr, rec, USER_ATR_UPS, 0, def_size, user_ups);
            tb_write_atr_len(cdp, usertab_descr, rec, USER_ATR_UPS,    def_size);
          }
          corefree(user_ups);
        } // ups loaded
      }
      unfix_page(cdp, fcbn);
    } // fixed
  } // cycle
  return FALSE;
}

void remove_refs_to_all_roles(cdp_t cdp, tobjnum user_or_group)
{ unsigned def_size;
  uns8 * user_ups=(uns8*)load_var_val(cdp, usertab_descr, user_or_group, USER_ATR_UPS, &def_size);
  if (user_ups!=NULL)
  { def_size=def_size/UUID_SIZE*UUID_SIZE;  // alignment
    unsigned pos=0;  bool changed=false;
    while (pos < def_size)  // def_size aligned, test is OK
    { bool is_role=true;
      for (int i=sizeof(tobjnum);  i<UUID_SIZE;  i++)
        if (user_ups[pos+i]!=0xff) { is_role=false;  break; }
      if (is_role)
      { def_size-=UUID_SIZE;
        memmov(user_ups+pos, user_ups+pos+UUID_SIZE, def_size-pos);
        changed=true;
      }
      else
        pos+=UUID_SIZE;
    }
    if (changed)
    { tb_write_var    (cdp, usertab_descr, user_or_group, USER_ATR_UPS, 0, def_size, user_ups);
      tb_write_atr_len(cdp, usertab_descr, user_or_group, USER_ATR_UPS,    def_size);
    }
    corefree(user_ups);
  } // ups loaded
}

BOOL user_and_group(cdp_t cdp, tobjnum user_or_group, tobjnum group_or_role,
                    tcateg subject2, t_oper oper, uns8 * state)
{ uns8 s2_uuid[UUID_SIZE];  unsigned def_size;
  BOOL res = FALSE;
  if (subject2==CATEG_APPL) return remove_app_roles(cdp, group_or_role);
  if (user_or_group==NOOBJECT) user_or_group=cdp->prvs.luser();
 // process the relationship which are not stored:
  if (subject2==CATEG_GROUP && group_or_role==EVERYBODY_GROUP && user_or_group!=ANONYMOUS_USER)
    if (oper==OPER_SET)
      if (*state) return FALSE;  // no action
      else { request_error(cdp, NO_RIGHT);  return TRUE; } // impossible
    else  // OPER_GET
      { *state=TRUE;  return FALSE; }
 // process stored relationships:
  if (oper==OPER_SET) 
    if (wait_record_lock_error(cdp, usertab_descr, user_or_group, TMPWLOCK)) return TRUE;
  uns8 * user_ups=(uns8*)load_var_val(cdp, usertab_descr, user_or_group, USER_ATR_UPS, &def_size);
  if (user_ups==NULL) { res=TRUE;  goto ex; }
  def_size=def_size/UUID_SIZE*UUID_SIZE;  // alignment
  if (subject2==CATEG_GROUP)
  { if (tb_read_atr(cdp, usertab_descr, group_or_role, USER_ATR_UUID, (tptr)s2_uuid))
      { corefree(user_ups);  res=TRUE;  goto ex; }
  }
  else if (subject2==CATEG_ROLE)
  { memset(s2_uuid, 0xff, UUID_SIZE);
    *(tobjnum*)s2_uuid=group_or_role;
  }
  else { corefree(user_ups);  res=TRUE;  goto ex; }
 // search:
  unsigned pos;  pos=0;
  while (pos < def_size)  // def_size aligned, test is OK
  { if (!memcmp(user_ups+pos, s2_uuid, UUID_SIZE)) break;
    pos+=UUID_SIZE;
  }
  switch (oper)
  { case OPER_SET: // set state, check the privileges of the grantor:
      if (subject2==CATEG_GROUP)
        if (group_or_role<SPECIAL_USER_COUNT)  
        // Special groups can be administered by the security administrator only,
        // But CONF_ADM_GROUP may be allowed to be administered by conf admin.
        { if (!cdp->prvs.is_secur_admin) 
            if (group_or_role!=CONF_ADM_GROUP || !cdp->prvs.is_conf_admin || !the_sp.ConfAdmsOwnGroup.val())
              { request_error(cdp, NO_RIGHT);  res=TRUE;  break; }
        }
        else
        // General groups can be administered by the security administrator,
        //  may be allowed to be administered by conf admin.
        { if (!cdp->prvs.is_secur_admin) 
            if (!cdp->prvs.is_conf_admin || !the_sp.ConfAdmsUserGroups.val())
              { request_error(cdp, NO_RIGHT);  res=TRUE;  break; }
        }
      else  // administerting a role: allowed only for the administrator of the application! (+ sec admin + admin mode)
      { WBUUID role_apl_uuid;
        if (tb_read_atr(cdp, objtab_descr, group_or_role, APPL_ID_ATR, (tptr)role_apl_uuid))
          { res=TRUE;  break; }
        if (!cdp->prvs.is_secur_admin) 
          if (!cdp->prvs.am_I_appl_admin(cdp, role_apl_uuid))
            if (!cdp->in_admin_mode || memcmp(role_apl_uuid, cdp->current_schema_uuid, UUID_SIZE))
            // special feature: administerting a role allowed in admin mode for the current schema (eDock)
              { request_error(cdp, NO_RIGHT);  res=TRUE;  break; }
       // disable creatating an author of locked application:
        if (*state) // adding
        { tobjname role_name;
          tb_read_atr(cdp, objtab_descr, group_or_role, OBJ_NAME_ATR, role_name);
          if (!strcmp(role_name, "AUTHOR"))
          { tobjnum aplobj = find_object_by_id(cdp, CATEG_APPL, role_apl_uuid);
            apx_header apx;  memset(&apx, 0, sizeof(apx));
            tb_read_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, 0, sizeof(apx_header), (tptr)&apx);
            if (apx.appl_locked)
              { request_error(cdp, NO_RIGHT);  res=TRUE;  break; }
          }
        }
      }
      // cont.
    case OPER_SET_ON_CREATE:  // setting without checking privileges: used only when assigning the creator of the application into admin role
     // set:
      if (!*state) // remove
      { if (pos < def_size)   // found
        { def_size-=UUID_SIZE;
          memmov(user_ups+pos, user_ups+pos+UUID_SIZE, def_size-pos);
          res =tb_write_var    (cdp, usertab_descr, user_or_group, USER_ATR_UPS, 0, def_size, user_ups) ||
               tb_write_atr_len(cdp, usertab_descr, user_or_group, USER_ATR_UPS,    def_size);
          reset_privil_caches(TRUE);  privil_version.new_privil_version();
        }
      }
      else  // add
        if (pos == def_size)   // not found
        { res=tb_write_var(cdp, usertab_descr, user_or_group, USER_ATR_UPS, def_size, UUID_SIZE, s2_uuid);
          reset_privil_caches(TRUE);  privil_version.new_privil_version();
        }
      break;
    case OPER_GET: // get state
      *state=(uns8)(pos < def_size);
      break;
  }
  corefree(user_ups);
 ex:
  return res;
}

bool privils::am_I_effectively_in_role(tobjnum roleobj)
{ WBUUID uuid;
  memset(uuid, 0xff, UUID_SIZE);  *(tobjnum*)uuid=roleobj;
  return is_in_groupdef(uuid);
}

bool get_effective_membership(cdp_t cdp, tobjnum subject, tcateg subject_categ, tobjnum container, tcateg cont_categ)
{ privils prvs(cdp);
  if (prvs.set_user(subject, subject_categ, TRUE))
  { request_error(cdp, OUT_OF_KERNEL_MEMORY);
    return false;
  }
  else
  { WBUUID cont_uuid;
    if (cont_categ==CATEG_GROUP)
      tb_read_atr(cdp, usertab_descr, container, USER_ATR_UUID, (tptr)cont_uuid);
    else // cont_categ==CATEG_ROLE
    { memset(cont_uuid, 0xff, UUID_SIZE);
      *(tobjnum*)cont_uuid=container;
    }
    return prvs.is_in_groupdef(cont_uuid);
  }
}

BOOL privils::am_I_appl_admin(cdp_t cdp, WBUUID apl_uuid)
{ tobjnum admobj;
  if (!memcmp(apl_uuid, cdp->top_appl_uuid, UUID_SIZE)) 
       admobj=cdp->admin_role;  // faster version of finding the administrator's objnum
  else admobj=find2_object(cdp, CATEG_ROLE, apl_uuid, "ADMINISTRATOR");
  if (admobj==NOOBJECT) return FALSE;
  return am_I_effectively_in_role(admobj);
  //uns8 state;  -- this checks for direct membership only!
  //if (user_and_group(cdp, user_objnum, admobj, CATEG_ROLE, OPER_GET, &state)) return FALSE;
  //return state;
}

BOOL privils::am_I_appl_author(cdp_t cdp, const WBUUID apl_uuid)
{ tobjnum authobj;
 // find author objnum of schema [apl_uuid]: 
  if (!memcmp(apl_uuid, cdp->top_appl_uuid, UUID_SIZE)) 
       authobj=cdp->author_role;  // faster version of finding the administrator's objnum
  else authobj=find2_object(cdp, CATEG_ROLE, apl_uuid, "AUTHOR");
  if (authobj==NOOBJECT) return FALSE;
 // check for the temporary author mode:
  if (temp_author==authobj) return true;
 // check for permanent author:
  return am_I_effectively_in_role(authobj);
  //uns8 state;  -- this checks for direct membership only!
  //if (user_and_group(cdp, user_objnum, authobj, CATEG_ROLE, OPER_GET, &state)) return FALSE;
  //return state;
}

BOOL cd_t::has_full_access(const WBUUID schema_uuid) const
{ return prvs.is_data_admin || in_admin_mode>0 && !memcmp(schema_uuid, current_schema_uuid, UUID_SIZE); }

bool t_privil_cache::can_reference_columns(cdp_t cdp, t_atrmap_flex & map, table_descr * tbdf, t_atrmap_flex * problem_map)
// Checks if all bits referenced in map have the read privilege.
// If not, return false, and (if problem_map!=NULL) sets the problem bits problem_map.
// Fast version: Checks only parts containing any bit set. Stops passing a part when all set bit passed.
{ bool ok = true;
  if (problem_map) 
  { problem_map->init(map.get_bitcount());
    problem_map->clear();
  }
  if (priv_user!=cdp->prvs.luser() || !privil_version.check_privil_version(priv_version))
    load_global_privils(cdp, tbdf);
  t_atrmap_enum mapenum(&map);
  int column;
  do
  { column=mapenum.get_next_set();
    if (column==-1) break;
    if (!global_privils.has_read(column)) 
    { ok = false;
      if (problem_map) problem_map->add(column);
    }
  } while (true);
  return ok;
}

bool t_record_privil_cache::have_local_rights_to_columns(cdp_t cdp, table_descr * tbdf, trecnum recnum, t_atrmap_flex & map)
{ if (priv_user!=cdp->prvs.luser() || !privil_version.check_privil_version(priv_version) || positioned_on_record != recnum)
    load_record_privils(cdp, tbdf, recnum);
  t_atrmap_enum mapenum(&map);
  int column;
  do
  { column=mapenum.get_next_set();
    if (column==-1) break;
    if (!record_privils.has_read(column)) return false;
  } while (true);
  return true;
}
