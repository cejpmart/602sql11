/****************************************************************************/
/* The S-Pascal compiler - table definition part - comptab.cpp              */
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
#include "xsa.h"
#include "profiler.h"
#include "flstr.h"
#include "opstr.h"
#include "netapi.h"
#include "nlmtexts.h"
#include <stddef.h>  // offsetof
#ifndef WINS
#include <unistd.h>
#include <ctype.h>
#ifndef UNIX
#include <process.h>
#include <advanced.h>
#endif
#undef putc
#endif

#define NEW_EXIT_HANDLING   // processing continues after the end of the block with the EXIT handler

/* __RELTAB rules
Referential integrity: 
  - je-li mezi dvojici tabulek vic vztahu ref. integrity, v tabulce __reltab je pouze jeden zaznam
  - zaznam v tabulce __reltab existuje tehdy, kdyz existuje porizena tabulka se vztahem ref. int., nezavisi na existenci nadrizene tabulky
Triggers:
  - see register_unregister_trigger
Domains:
  - a record in __RELTAB exists for every table referencing a domain, independent of the existence of the domain
Fulltext systems:
  - a record in __RELTAB exists for every table referenced in the fulltext system, independent of the existence of the table
  - only one such record exists even if the same table is referenced multiple times.
*/
static t_locdecl * get_noncont_condition_handler(cdp_t cdp, const t_scope * scope);
static t_locdecl * get_continue_condition_handler(cdp_t cdp, t_rscope *& start_rscope, t_rscope *& handler_rscope, tobjnum & handler_objnum);
static void execute_handler(cdp_t cdp, t_locdecl * locdecl, t_rscope * handler_rscope, tobjnum handler_objnum);

int WINAPI partial_compile_to_ta(cdp_t cdp, ttablenum tabnum, table_all * ta)
// Loads and compiles table definition to ta, except for constrains. 
// Returns error, does not call request_error for compilation errors.
{ tptr defin=ker_load_objdef(cdp, tabtab_descr, tabnum);
  if (!defin) return cdp->get_return_code();
  WBUUID table_schema_uuid;
  tb_read_atr(cdp, tabtab_descr, tabnum, APPL_ID_ATR, (tptr)table_schema_uuid);
 // t_ultralight_schema_context used instead t_short_term_schema_context because executing the initialization of global variables may reference the same table
  t_ultralight_schema_context stsc(cdp, table_schema_uuid);  // used when looking for domains
  int res=compile_table_to_all(cdp, defin, ta);
  corefree(defin);
  return res;
}

#include "tablesql.cpp"

void table_all::mark_core(void)
{ mark(this);
  int  i;
 // columns:
  attrs.mark_core();
  for (i=0;  i<attrs.count();  i++)
  { atr_all * att=attrs.acc0(i);
    if (att->alt_value) mark(att->alt_value);
    if (att->defval)    mark(att->defval);
    if (att->comment)   mark(att->comment);
  }
  names.mark_core();
 // mark constrains:
  indxs.mark_core();
  for (i=0;  i<indxs.count();   i++)
  { ind_descr * indx = indxs.acc0(i);
    indx->mark_core();
  }
  checks.mark_core();
  for (i=0;  i<checks.count();  i++)
  { check_constr * chk = checks.acc0(i);
    chk->mark_core();
  }
  refers.mark_core();
  for (i=0;  i<refers.count();  i++)
  { forkey_constr * ref = refers.acc0(i);
    ref->mark_core();
  }
}

void analyse_variable_type(CIp_t CI, int & tp, t_specif & specif, t_express ** pdefval)
// If pdefval==NULL then the default clause should not be compiled and should be ignored.
{ analyse_type(CI, tp, specif, NULL, 0);
 // for varibles and CAST, replace EXT tyoe by internal types:
  if      (tp==ATT_EXTCLOB) tp=ATT_TEXT;
  else if (tp==ATT_EXTBLOB) tp=ATT_NOSPEC;
  if (tp==ATT_SIGNAT || tp>=ATT_FIRSTSPEC && tp<=ATT_LASTSPEC || tp==ATT_PTR || tp==ATT_BIPTR) // || tp==ATT_EXTCLOB || tp==ATT_EXTBLOB)
    c_error(THIS_TYPE_NOT_ENABLED);
  if (tp==ATT_DOMAIN)  // translate the domain (type is not pointer, does not affect namecnt)
  { t_type_specif_info tsi;
    if (!compile_domain_to_type(CI->cdp, specif.domain_num, tsi))
      c_error(CI->cdp->get_return_code());
    tp=tsi.tp;  specif=tsi.specif;
    if (tsi.defval!=NULL && pdefval!=NULL)
    { compil_info xCI(CI->cdp, tsi.defval, default_value_link);
      t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  // must be defined, even with empty kontext
      attribdef att;
      xCI.univ_ptr=&att;  att.attrtype=tp;  att.attrspecif=specif;  att.defvalexpr=NULL;
      int res=compile_masked(&xCI);
      if (res) 
      { delete att.defvalexpr;  att.defvalexpr=NULL;
        c_error(res);
      }
      *pdefval=att.defvalexpr;  att.defvalexpr=NULL;
    }
  }
}

void get_table_schema_name(cdp_t cdp, ttablenum tabnum, char * schemaname)
// Determines the name of the schema the table tabnum belongs to and stores it to schemaname.
{ WBUUID uuid;
  if (!tb_read_atr(cdp, tabtab_descr, tabnum, APPL_ID_ATR, (tptr)uuid))
    if (null_uuid(uuid)) *schemaname=0;
    else ker_apl_id2name(cdp, uuid, schemaname, NULL);
  else *schemaname=0;  // default for errors
}

int WINAPI compile_table_to_all(cdp_t cdp, const char * defin, table_all * pta)
// Does not call request_error on compilation errors!
{ compil_info xCI(cdp, defin, table_def_comp_link);
  xCI.univ_ptr=pta;
  return compile_masked(&xCI);
}

static BOOL name_used(table_all * ta, tobjname impl_name)
{ int  j;
  for (j=0;  j<ta->indxs.count();  j++)
    if (!stricmp(ta->indxs .acc0(j)->constr_name, impl_name))
      return TRUE;
  for (j=0;  j<ta->checks.count();  j++)
    if (!stricmp(ta->checks.acc0(j)->constr_name, impl_name))
      return TRUE;
  for (j=0;  j<ta->refers.count();  j++)
    if (!stricmp(ta->refers.acc0(j)->constr_name, impl_name))
      return TRUE;
  return FALSE;
}

void add_implicit_constraint_names(table_all * ta)
// Assigns implicit names to all unnamed constrains in the table definition
{ int i;  unsigned curr_num;  tobjname impl_name;
  curr_num = 1;
  for (i=0;  i<ta->indxs.count();  i++)
  { ind_descr * indx = ta->indxs.acc0(i);
    if (!*indx->constr_name)  // add implicit name
    { do // cycling until an unique [impl_name] is found
      { sprintf(impl_name, "INDEX%u", curr_num++);  // next alter changes small letters to uppercase
        if (!name_used(ta, impl_name)) break;
      } while (TRUE);
      strcpy(indx->constr_name, impl_name);
    }
  }
  curr_num = 1;
  for (i=0;  i<ta->checks.count();  i++)
  { check_constr * chk = ta->checks.acc0(i);
    if (!*chk->constr_name)  // add implicit name
    { do // cycling until an unique [impl_name] is found
      { sprintf(impl_name, "CHECK%u", curr_num++);
        if (!name_used(ta, impl_name)) break;
      } while (TRUE);
      strcpy(chk->constr_name, impl_name);
    }
  }
  curr_num = 1;
  for (i=0;  i<ta->refers.count();  i++)
  { forkey_constr * ref = ta->refers.acc0(i);
    if (!*ref->constr_name)  // add implicit name
    { do // cycling until an unique [impl_name] is found
      { sprintf(impl_name, "FOREIGN_KEY%u", curr_num++);
        if (!name_used(ta, impl_name)) break;
      } while (TRUE);
      strcpy(ref->constr_name, impl_name);
    }
  }
}
/******************************* table descr ********************************/
/****************************** updating references *************************/
const t_colval reltab_coldescr[] = {
{  1, NULL, (void*)offsetof(t_reltab_vector0, name1),    NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(t_reltab_vector0, uuid1),    NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(t_reltab_vector0, name2),    NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(t_reltab_vector0, uuid2),    NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(t_reltab_vector0, classnum), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(t_reltab_vector0, info),     NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static BOOL update_reftab_list(cdp_t cdp, const tobjname childtabname, const tobjname partabname, 
                               const WBUUID child_schema_id, const char * parent_schema_name, BOOL adding)
// Adds/removes a foreign reference from the child table to the parent table
{// get the schema uuid of the parent table: either the schema name is specified or use same schema as for the child table
  WBUUID parent_schema_id;
  if (!parent_schema_name || !*parent_schema_name)
    memcpy(parent_schema_id, child_schema_id, UUID_SIZE);
  else
    if (ker_apl_name2id(cdp, parent_schema_name, parent_schema_id, NULL)) 
      { SET_ERR_MSG(cdp, parent_schema_name);  request_error(cdp, OBJECT_NOT_FOUND);  return FALSE; }
 // look for the relation:
  t_reltab_primary_key start(REL_CLASS_RI, partabname, parent_schema_id, childtabname, child_schema_id),
                       stop (REL_CLASS_RI, partabname, parent_schema_id, childtabname, child_schema_id);
  t_index_interval_itertor iii(cdp);
  if (!iii.open(REL_TABLENUM, REL_TAB_PRIM_KEY, &start, &stop)) 
    return FALSE;
 // the index is unique, no need to cycle on values:
  trecnum rec=iii.next();
  if (rec==NORECNUM) 
  { if (adding)
    { t_reltab_vector rtv(REL_CLASS_RI, partabname, parent_schema_id, childtabname, child_schema_id, 0);
      t_vector_descr vector(reltab_coldescr, &rtv);
      tb_new(cdp, iii.td, FALSE, &vector);
    }
    // else no action: inserting existing record
  }
  else  // record found
    if (!adding)
      tb_del(cdp, iii.td, rec);
    // else no action: deleting not existing record
  iii.close();
 // update the core information, if exists:
  ttablenum partabnum = find2_object(cdp, CATEG_TABLE, parent_schema_id, partabname);
  if (partabnum!=NOOBJECT)
  { ProtectedByCriticalSection cs(&cs_tables, cdp, WAIT_CS_TABLES);
    if (tables[partabnum])
      tables[partabnum]->referencing_tables_listed = FALSE;
  }
  return TRUE;
}

static BOOL find_referencing_tables(cdp_t cdp, const tobjname partabname, const WBUUID parent_appl, recnum_dynar * table_list)
{// look for the relation:
  t_reltab_primary_key start(REL_CLASS_RI, partabname, parent_appl, NULLSTRING, parent_appl),
                       stop (REL_CLASS_RI, partabname, parent_appl, "\xff",     parent_appl);
  t_index_interval_itertor iii(cdp);
  if (!iii.open(REL_TABLENUM, REL_TAB_PRIM_KEY, &start, &stop)) 
    return FALSE;
  BOOL res=TRUE;
  do
  { trecnum rec=iii.next();
    if (rec==NORECNUM) break;
   // find the referencing table:
    WBUUID child_schema_uuid;  tobjname child_name;
    fast_table_read(cdp, iii.td, rec, REL_CLD_UUID_COL, child_schema_uuid);
    fast_table_read(cdp, iii.td, rec, REL_CLD_NAME_COL, child_name);
    ttablenum childtabnum = find2_object(cdp, CATEG_TABLE, child_schema_uuid, child_name);
   // add the dependent table to the list, if found:
    if (childtabnum!=NOOBJECT)
    { trecnum * prec = table_list->next();
      if (!prec) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  res=FALSE;  break; }
      *prec=childtabnum;
    }
  } while (TRUE);
  iii.close();
  return res;
}

BOOL prepare_list_of_referencing_tables(cdp_t cdp, table_descr * td)
// Creates list of referencing tables distributed by indicies.
// Should not be called during the installation of td, but later!
// Otherwise desttabnum and par_index_num may not be defined.
{ ProtectedByCriticalSection cs(&td->lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
  if (!SYSTEM_TABLE(td->tbnum))
  { recnum_dynar table_list;  int i;
    if (!find_referencing_tables(cdp, td->selfname, td->schema_uuid, &table_list))
      return FALSE;
   // drop old lists:
    for (i=0;  i<td->indx_cnt;  i++)
      if (td->indxs[i].reftabnums)
        { corefree(td->indxs[i].reftabnums);  td->indxs[i].reftabnums=NULL; }
   // create new lists:
    for (i=0;  i<table_list.count();  i++)
    { table_descr_auto ch_tbdf(cdp, *table_list.acc0(i));
      if (ch_tbdf->me())
      { for (int j=0;  j<ch_tbdf->forkey_cnt;  j++)  // multipre references from the same child table may refer to various indices
          if (ch_tbdf->forkeys[j].desttabnum==td->tbnum)  // this RI references me
          { int indnum = ch_tbdf->forkeys[j].par_index_num;
            if (indnum >= 0)
            { dd_index * ddi = &td->indxs[indnum];
             // count the tables in the existing list:
              int cnt=0; 
              if (ddi->reftabnums)
                while (ddi->reftabnums[cnt]) cnt++;
             // allocate a bigger list:
              ttablenum * new_list = (ttablenum*)corealloc((cnt+2)*sizeof(ttablenum), 49);  // space for existing tables, new table and terminator
              if (!new_list) return FALSE;  // error
              new_list[cnt] = ch_tbdf->tbnum;  // new table added
              new_list[cnt+1]=0;  // terminator
             // copy and deallocate the original list, if exists:
              if (ddi->reftabnums)
              { memcpy(new_list, ddi->reftabnums, cnt*sizeof(ttablenum));
                corefree(ddi->reftabnums);
              }
             // store the new list:
              ddi->reftabnums=new_list;
            } // my referenced index num found
          } // reference to me found
      } // child table installed OK
    } // cycle on referencing tables
  } // not a system table
  td->referencing_tables_listed=TRUE;
  return TRUE;
}

int compile_index(cdp_t cdp, const char * key, const table_all * ta, const char * name, int index_type, bool has_nulls, dd_index * result)
// Compiles an index key and creates dd_index. Parameters are usually from an ind_descr.
{ 
  strcpy(result->constr_name, name);
  result->ccateg=index_type;
  result->has_nulls=has_nulls;
  result->atrmap.init(ta->attrs.count());
  result->atrmap.clear();
  compil_info xCI(cdp, key, index_expr_comp2);
  t_sql_kont sql_kont;  sql_kont.kont_ta=ta;  xCI.sql_kont=&sql_kont;
  xCI.univ_ptr=result;
  return compile_masked(&xCI);
}

int find_index_for_key(cdp_t cdp, const char * key, const table_all * ta, const ind_descr * index_info, dd_index_sep * ext_indx)
// Returns number of index with the key specified by [key].
// Returns -3 if key cannot be compiled in the kontext of ta.
// Returns -2 if key is empty and ta does not have a primary key.
// Returns -1 if index has not been found.
// If index_info!=NULL searches only indices with the same chracterstics.
{ int i;
  if (!key || !*key)  // empty key definition means the primary key
  { for (i=0;  i<ta->indxs.count();  i++)
    { ind_descr * indx = (ind_descr *)ta->indxs.acc0(i);
      if (indx->state!=INDEX_DEL && indx->index_type==INDEX_PRIMARY)
        return i;
    }
   // primary key not found
    request_error(cdp, NO_PARENT_INDEX);  
    return -2;
  }
 // compile key to ext_index or aux_indx:
  dd_index_sep aux_indx;  // atrmap.init() is in compile_index()
  if (!ext_indx) ext_indx=&aux_indx;
  if (compile_index(cdp, key, ta, NULLSTRING, INDEX_UNIQUE, false, ext_indx)) // type of the index is not important
    return -3;  // compilation error
 // pass indicies and compare to them:
  for (i=0;  i<ta->indxs.count(); i++)
  { ind_descr * indx = (ind_descr *)ta->indxs.acc0(i);
    if (indx->state==INDEX_DEL) continue;
    if (index_info!=NULL)
      if (indx->has_nulls!=index_info->has_nulls || indx->index_type!=index_info->index_type)
        continue;
   // compile the index:
    dd_index_sep aindx;  // atrmap.init() is in compile_index()
    if (compile_index(cdp, indx->text, ta, indx->constr_name, indx->index_type, indx->has_nulls, &aindx)) continue;
   // compare index parts:
    if (aindx.same(*ext_indx, true))
      return i;
  }
  return -1;
}

static BOOL check_index_for_referencing_tables(cdp_t cdp, const table_all * ta, const WBUUID schema_uuid)
// Check if there still is an index for every foreign key used by any referencing tables.
{ recnum_dynar table_list;
  if (!find_referencing_tables(cdp, ta->selfname, schema_uuid, &table_list))
    return FALSE;
  for (int i=0;  i<table_list.count();  i++)
  { ttablenum childnum=*table_list.acc0(i);
    table_all ch_ta;
    if (!partial_compile_to_ta(cdp, childnum, &ch_ta))
    { for (int j=0;  j<ch_ta.refers.count();  j++)
      { forkey_constr * ref = ch_ta.refers.acc0(j);
        if (!strcmp(ref->desttab_name, ta->selfname))
        {// get schema id of the parent table in the RI and compare it to own schema_uuid:
          WBUUID par_schema_uuid;
          if (*ref->desttab_schema)  // schema specified
          { if (ker_apl_name2id(cdp, ref->desttab_schema, par_schema_uuid, NULL)) continue;
          }
          else
            fast_table_read(cdp, tabtab_descr, childnum, APPL_ID_ATR, par_schema_uuid); // same as child
          if (!memcmp(par_schema_uuid, schema_uuid, UUID_SIZE))
          {// search foreign key among indicies:
            int par_indnum = find_index_for_key(cdp, ref->par_text, ta);
            if (par_indnum < 0)
            { SET_ERR_MSG(cdp, ch_ta.selfname);
              request_error(cdp, REFERENCED_BY_OTHER_OBJECT);  
              return FALSE;  // foreing key not found!
            }
           // if the referencing table is installed, update the parent index number stored in it:
            if (tables[childnum])
            { table_descr_auto ch_tbdf(cdp, childnum);
              ch_tbdf->forkeys[j].par_index_num = par_indnum;
            }
          }
        }
      }
    }
  }
  return TRUE;
}

static BOOL drop_ri_to_parent(cdp_t cdp, const tobjname childtabname, const WBUUID child_appl)
// Removes all foreign references from a child table to parent tables.
{ trecnum rec;
 // cannot cycle on index values and delete in the same time! Must re-open the index:
  do 
  {// find and delete one relation record: 
    t_reltab_index2 start(REL_CLASS_RI, childtabname, child_appl),
                    stop (REL_CLASS_RI, childtabname, child_appl);
    t_index_interval_itertor iii(cdp);
    if (!iii.open(REL_TABLENUM, REL_TAB_INDEX2, &start, &stop)) 
      return FALSE;
    rec=iii.next();
    if (rec!=NORECNUM) 
    {// read the parent table:
      tobjname parent_name;  WBUUID parent_schema_id;
      if (fast_table_read(cdp, iii.td, rec, REL_PAR_NAME_COL, parent_name) ||
          fast_table_read(cdp, iii.td, rec, REL_PAR_UUID_COL, (tptr)parent_schema_id)) 
        break;
     // delete the record:
      if (tb_del(cdp, iii.td, rec)) break;
     // find the parent table and update core structures for it:
      ttablenum partabnum = find2_object(cdp, CATEG_TABLE, parent_schema_id, parent_name);
      if (partabnum!=NOOBJECT)
      { ProtectedByCriticalSection cs(&cs_tables, cdp, WAIT_CS_TABLES);
        if (tables[partabnum])
          tables[partabnum]->referencing_tables_listed = FALSE;
      }
    }
    iii.close();
   // continue if found:
  } while (rec!=NORECNUM);
  return TRUE;
}

/////////////////////////////////// registering domain usage ///////////////////////////
static BOOL drop_table(cdp_t cdp, ttablenum objnum, bool cascade);

void unregister_domains_in_table(cdp_t cdp, const char * tabname, const WBUUID appl_id)
{ char sqlcmd[200];
  sprintf(sqlcmd, "DELETE FROM __RELTAB WHERE CLASSNUM=%u AND NAME1=\'%s\' AND APL_UUID1=X\'", REL_CLASS_DOM, tabname);
  size_t len=strlen(sqlcmd);
  bin2hex(sqlcmd+len, appl_id, UUID_SIZE);  len+=2*UUID_SIZE;
  sqlcmd[len++]='\'';  sqlcmd[len]=0;
  cdp->prvs.is_data_admin++; // prevents checking privils to __RELTAB
  sql_exec_direct(cdp, sqlcmd);
  cdp->prvs.is_data_admin--;
}

inline bool domain_from_local_schema(const table_all * ta, const double_name * dnm)
{ return !*dnm->schema_name || !sys_stricmp(dnm->schema_name, ta->schemaname); }

void register_domains_in_table(cdp_t cdp, const table_all * ta, const WBUUID appl_id)
// Insertes into __reltab information about all domains used in the table columns.
// Supposes ta->schemaname is defined.
{ for (int i=1;  i<ta->attrs.count();  i++)
    if (ta->attrs.acc0(i)->type==ATT_DOMAIN)
    { const double_name * dnm = ta->names.acc0(i);
     // find the previous occurence of the same domain: 
      bool is_registered = false;
      for (int j=1;  j<i;  j++)
        if (ta->attrs.acc0(j)->type==ATT_DOMAIN)
        { const double_name * dnm2 = ta->names.acc0(j);
          if (!sys_stricmp(dnm->name, dnm2->name))
            if (domain_from_local_schema(ta, dnm) ? domain_from_local_schema(ta, dnm2) :
                                                    !sys_stricmp(dnm->schema_name, dnm2->schema_name))
              is_registered=true;
        }
      if (is_registered) continue;
     // this domain did not occured in the table so far, register it:
      table_descr_auto td(cdp, REL_TABLENUM);
      if (td->me())
      {// find dom_schema_uuid: 
        WBUUID dom_schema_uuid;
        if (domain_from_local_schema(ta, dnm))  // domain schema specified and different from table schema
          memcpy(dom_schema_uuid, appl_id, UUID_SIZE);
        else 
          ker_apl_name2id(cdp, dnm->schema_name, dom_schema_uuid, NULL);
       // write the relation:
        t_reltab_vector rtv(REL_CLASS_DOM, ta->selfname, appl_id, dnm->name, dom_schema_uuid, 0);
        t_vector_descr vector(reltab_coldescr, &rtv);
        tb_new(cdp, td->me(), FALSE, &vector);
      }
    }
}

bool check_for_or_delete_dependent_tables(cdp_t cdp, const char * domain_name, tobjnum objnum, bool cascade)
// Returns true if cannot delete the tables
{ WBUUID dom_appl_id;  
  tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)dom_appl_id);
 // cannot cycle on index values and delete in the same time! Must re-open the index:
  do
  {// find dependent tables:
    t_reltab_index2 start(REL_CLASS_DOM, domain_name, dom_appl_id),
                    stop (REL_CLASS_DOM, domain_name, dom_appl_id);
    t_index_interval_itertor iii(cdp);
    if (!iii.open(REL_TABLENUM, REL_TAB_INDEX2, &start, &stop)) 
      return false;
    trecnum rec=iii.next();
    if (rec==NORECNUM) return false;  // no (more) dependent tables
    tobjname table_name;  WBUUID table_schema_id;
    if (fast_table_read(cdp, iii.td, rec, REL_PAR_NAME_COL, table_name) ||
        fast_table_read(cdp, iii.td, rec, REL_PAR_UUID_COL, (tptr)table_schema_id)) 
      return true;
   // search for he table:
    ttablenum tablenum = find2_object(cdp, CATEG_TABLE, table_schema_id, table_name);
    if (tablenum!=NOOBJECT)
    { iii.close();
      if (!cascade)  // a table depends on the domain, fail because CASCADE is not specified
        { SET_ERR_MSG(cdp, table_name);  request_error(cdp, REFERENCED_BY_OTHER_OBJECT);  return true; }
     // delete the table (privils not checked because I have the privilege to delete the domain): 
        if (drop_table(cdp, tablenum, true)) 
          return true; // error deleting the table
    }
    else  // table registered, but it does not exist, must delete the registration, otherwise would cycle infinitely
    { tb_del(cdp, iii.td, rec);
      iii.close();
    }
  } while (true);
}

static BOOL save_table_definition(cdp_t cdp, ttablenum tabnum, tptr defin)
// Saves the table SQL definition to TABTAB. Then releases it.
// Requires ta->schemaname to be specified, even though it is not stored.
{ if (!defin) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
 // encrypt the source, if necessary:
  int len = (int)strlen(defin);
  uns16 obj_flags;
  tb_read_atr(cdp, tabtab_descr, tabnum, OBJ_FLAGS_ATR, (tptr)&obj_flags);
  if (obj_flags & CO_FLAG_ENCRYPTED) enc_buffer_total(cdp, defin, len, (tobjnum)tabnum);
 // save the new definition:
  uns32 flags = cdp->ker_flags;  cdp->ker_flags |= KFL_NO_JOUR;
  BOOL res  = tb_write_var    (cdp, tabtab_descr, tabnum, OBJ_DEF_ATR, 0, len, defin) |
              tb_write_atr_len(cdp, tabtab_descr, tabnum, OBJ_DEF_ATR, len);
 // write the last modification date:
  uns32 stamp = stamp_now();
  tb_write_atr(cdp, tabtab_descr, tabnum, OBJ_MODIF_ATR, &stamp);
  cdp->ker_flags = flags;
  corefree(defin);
  return res;
}
/***************************** data transfer *******************************/
void WINAPI sql_expression_converted_entry(CIp_t CI)
{ const atr_all * att = (atr_all*)CI->univ_ptr;  // only type, specif and multi defined in att
  sql_expression(CI, (t_express**)&CI->stmt_ptr); 
  t_express * ex = (t_express*)CI->stmt_ptr;
  BOOL smult = (ex->type & 0x80) && ex->sqe==SQE_ATTR;
  BOOL dmult = att->multi > 1;
  if (smult) 
    if (dmult)
    { if ((ex->type & 0x7f) != att->type)
        c_error(INCOMPATIBLE_TYPES);
      CI->stmt_ptr=NULL;  delete ex;
    }
    else
    { t_expr_attr * atex = (t_expr_attr*)ex;  sig32 zero=0;
      atex->indexexpr=new t_expr_sconst(ATT_INT32, 0, &zero, sizeof(sig32));  if (!atex->indexexpr) c_error(OUT_OF_KERNEL_MEMORY);
      ex->type &= 0x7f;
      convert_value(CI, ex, att->type, att->specif);
    }
  else 
    if (dmult)
      convert_value(CI, ex, att->type, att->specif);
    else // normal:
      convert_value(CI, ex, att->type, att->specif);
}

static BOOL move_table_data(cdp_t cdp, table_all * ta_old, table_all * ta_new, table_descr * old_tabdescr, table_descr * new_tabdescr)
/* ker_flags contains NO_JOUR | ALLOW_TRACK_WRITE | KFL_NO_TRACE.
   Moves data between tables. Returns FALSE on error. */
{ int err=0;  BOOL any_multiassign=FALSE;  t_atrmap_flex multimap(ta_new->attrs.count());
#ifdef STOP
 // prepare compilation kontexts:
  db_kontext src;
  src.fill_from_td(cdp, old_tabdescr);
  src.next=NULL;
#endif  
 // prepare the initialisation vector for inserting records:
  t_colval * colvals = new t_colval[ta_new->attrs.count()];  t_vector_descr vector(colvals);
  multimap.clear();
  unsigned colind = 0;
  for (int i=1;  i<ta_new->attrs.count();  i++)
  { atr_all * att=ta_new->attrs.acc0(i);
    atr_all att_translated;
    att_translated.type=att->type;  att_translated.specif=att->specif;  att_translated.multi=att->multi;
    if (att_translated.type==ATT_DOMAIN)
    { t_type_specif_info tsi;
      if (compile_domain_to_type(cdp, att->specif.domain_num, tsi))
        { att_translated.type=tsi.tp;  att_translated.specif=tsi.specif; }
    }
    colvals[colind].attr =i;  
    colvals[colind].index=NULL;
    colvals[colind].expr =NULL;
    colvals[colind].table_number=0;
    colvals[colind].lengthptr=NULL;  // used when copying multiattributes
    if (att->alt_value==NULL || *att->alt_value)  /* unchanged attribute or VALUE specified */
    { char decorated_name[1+ATTRNAMELEN+1+1];
      sprintf(decorated_name, "`%s`", att->name);
      compil_info xCI(cdp, att->alt_value ? att->alt_value : decorated_name, sql_expression_converted_entry);
      t_sql_kont sql_kont;
#ifdef STOP      
      sql_kont.active_kontext_list=&src;
#else
      sql_kont.kont_ta=ta_old;
#endif      
      xCI.sql_kont=&sql_kont;  
      xCI.stmt_ptr=NULL;  xCI.univ_ptr=&att_translated;
      err=compile(&xCI);
      t_express * sex;
      sex = (t_express*)xCI.stmt_ptr;
      if (err) 
      { if (sex) delete sex;  
        make_exception_from_error(cdp, cdp->get_return_code());  // creates a normal error from compilation error
        break; 
      }
      colvals[colind].expr = sex;
      if (!sex) 
      { any_multiassign=TRUE;
        multimap.add(i);
        colvals[colind].dataptr=DATAPTR_DEFAULT;
      }
      else if (att->multi > 1)
      { sig32 zero=0;
        colvals[colind].index=new t_expr_sconst(ATT_INT32, 0, &zero, sizeof(sig32));
      }
    }
   // creating a new column:
    else if (!strcmp(att->name, "_W5_UNIKEY") || !strcmp(att->name, "_W5_ZCR"))
      continue;  // processed separetely in tb_write_vector, must not write normal type-dependent default
    else  // use the default value
      colvals[colind].dataptr=DATAPTR_DEFAULT;
    colind++;
  }
  colvals[colind].attr=NOATTRIB; // the delimiter
 // prepare index movement map:
  
 // prepare inserting deleted records:
  t_vector_descr default_deleted_vector(&default_colval);
  default_deleted_vector.delete_record=true;
 // move data (FULLTABLE locks prevent editing data in the old table and make writing into the new table faster):
  disksp.disable_flush();
  if (cdp->is_an_error()) err=TRUE;
  if (!err)
    if (!wait_record_lock_error(cdp, old_tabdescr, FULLTABLE, RLOCK))  // lock must not be TMP because commit is used
    { if (!wait_record_lock_error(cdp, new_tabdescr, FULLTABLE, WLOCK))  // lock must not be TMP because commit is used
      { commit(cdp);  // initial commit makes the status of previous statements in the transaction independent of the amount of data in the table!
        trecnum rec1, rec2, limit;
        enable_index(cdp, new_tabdescr, -1, FALSE);  // may be slower or faster, seems to be slower
        limit=old_tabdescr->Recnum();
        for (rec1=0;  rec1<limit && !err;  rec1++)
        { report_step(cdp, rec1, limit);
          if (!(rec1 % 32)) /* better than 64 */
          { commit(cdp); /* 1st commit at the start */
            if (cdp->is_an_error()) err=TRUE;
          }
          uns8 del=table_record_state(cdp, old_tabdescr, rec1);
          if (del)
          { int saved_flags = cdp->ker_flags;
            cdp->ker_flags |= KFL_DISABLE_INTEGRITY | KFL_DISABLE_REFCHECKS | KFL_NO_NEXT_SEQ_VAL | KFL_DISABLE_DEFAULT_VALUES | KSE_FORCE_DEFERRED_CHECK;  // disable immediate checks for the deleted record
            rec2=tb_new(cdp, new_tabdescr, FALSE, &default_deleted_vector);
            cdp->ker_flags = saved_flags;
            if (rec2==NORECNUM) break;
            //tb_del(cdp, new_tabdescr, rec2); -- done by default_deleted_vector, prevents conflict on unique indicies
          }
          else
          { cdp->check_rights=false;
            cdp->except_type=EXC_NO;  cdp->except_name[0]=0;  cdp->processed_except_name[0]=0;
#ifdef STOP
            src.position=rec1;  
#else
            cdp->kont_tbdf=old_tabdescr;  // this must be restored after aech commit!
            cdp->kont_recnum=rec1;
#endif            
            rec2=tb_new(cdp, new_tabdescr, FALSE, &vector);
            cdp->check_rights=true; 
            if (rec2==NORECNUM) break;
            if (cdp->except_type!=EXC_NO) 
              { err=TRUE;  break; }
            if (any_multiassign)
            { for (int i=1;  i<ta_new->attrs.count();  i++)
                if (multimap.has(i))
                { atr_all * att=ta_new->attrs.acc0(i);
                  for (int srcattr=1;  srcattr<old_tabdescr->attrcnt;  srcattr++)
                    if (!strcmp(att->alt_value ? att->alt_value : att->name, old_tabdescr->attrs[srcattr].name))
                    { copy_var_len(cdp, new_tabdescr, rec2, i, NOINDEX, old_tabdescr, rec1, srcattr, NOINDEX, NULL);
                      break;
                    }
                }
            }
          } // move data for the record pair
          if (cdp->break_request)
            { request_error(cdp, REQUEST_BREAKED);  err=TRUE;  break; }
        } // cycle on records
        report_total(cdp, rec1);
        if (cdp->is_an_error()) err=TRUE;
        //commit(cdp);   /* commit may have been called above */
        //enable_index(cdp, new_tabdescr, -1, TRUE);  -- do not re-enable indexes here, transform_indexes() will be called
        commit(cdp);   
        if (cdp->is_an_error()) err=TRUE;
        record_unlock(cdp, new_tabdescr, FULLTABLE, WLOCK);
      }
      else err=TRUE;
      record_unlock(cdp, old_tabdescr, FULLTABLE, RLOCK);
    }
    else err=TRUE;
  cdp->kont_tbdf=NULL;  
  disksp.enable_flush();
 // release colvals expressions (t_colval does not have the destructor):
  for (colind=0;  colvals[colind].attr!=NOATTRIB;  colind++)
  { if (colvals[colind].expr ) delete colvals[colind].expr;
    if (colvals[colind].index) delete colvals[colind].index;
  }
  delete [] colvals;
  return !err;
}

const char * explic_schema_name(const char * tb_schema_name, const char * ft_schema_name) 
// ft_schema_name must be defined.
{ return *tb_schema_name ? tb_schema_name : ft_schema_name; }

void register_tables_in_ftx(cdp_t cdp, t_fulltext * ftdef, WBUUID ftx_schema_uuid)
// Insertes into __RELTAB information about all tables referenced in the fulltext system.
{ 
  if (!*ftdef->schema)
    ker_apl_id2name(cdp, ftx_schema_uuid, ftdef->schema, NULL);  // explic_schema_name needs it
  for (int i=0;  i<ftdef->items.count();  i++)
  { const t_autoindex * ai = ftdef->items.acc0(i);
    if (ai->tag==ATR_DEL) continue;  // must be ignored in ALTER statement
   // find the previous occurence of the same table: 
    bool is_registered = false;
    for (int j=0;  j<i;  j++)
    { const t_autoindex * ai2 = ftdef->items.acc0(j);
      if (!sys_stricmp(ai->doctable_name, ai2->doctable_name) &&
          !sys_stricmp(explic_schema_name(ai->doctable_schema, ftdef->schema), 
                       explic_schema_name(ai2->doctable_schema, ftdef->schema)))
        is_registered=true;
    }
    if (is_registered) continue;
   // this table did not occured in the ftx so far, register it:
    table_descr_auto td(cdp, REL_TABLENUM);
    if (td->me())
    {// find table_schema_uuid: 
      WBUUID table_schema_uuid;
      if (*ai->doctable_schema)  // domain schema specified and different from table schema
        ker_apl_name2id(cdp, ai->doctable_schema, table_schema_uuid, NULL);
      else 
        memcpy(table_schema_uuid, ftx_schema_uuid, UUID_SIZE);
     // write the relation:
      t_reltab_vector rtv(REL_CLASS_FTX, ftdef->name, ftx_schema_uuid, ai->doctable_name, table_schema_uuid, 0);
      t_vector_descr vector(reltab_coldescr, &rtv);
      tb_new(cdp, td->me(), FALSE, &vector);
    }
  }
}

void register_tables_in_ftx(cdp_t cdp, tobjnum objnum)
{ WBUUID uuid;  t_fulltext ftdef_local;
  tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)uuid);
  char * defin = ker_load_objdef(cdp, objtab_descr, objnum);
  if (defin)
  { if (!ftdef_local.compile(cdp, defin))
      register_tables_in_ftx(cdp, &ftdef_local, uuid);
    corefree(defin);
  }
}

void unregister_tables_in_ftx(cdp_t cdp, const char * ftx_name, WBUUID schema_uuid)
// Removes from __RELTAB information about all tables referenced in the fulltext system.
{ char sqlcmd[100+OBJ_NAME_LEN+2*UUID_SIZE];
  sprintf(sqlcmd, "DELETE FROM __RELTAB WHERE CLASSNUM=%u AND NAME1=\'%s\' AND APL_UUID1=X\'", REL_CLASS_FTX, ftx_name);
  int len=(int)strlen(sqlcmd);
  bin2hex(sqlcmd+len, schema_uuid, UUID_SIZE);  len+=2*UUID_SIZE;
  sqlcmd[len++]='\'';  sqlcmd[len]=0;
  cdp->prvs.is_data_admin++; // prevents checking privils to __RELTAB
  unsigned saved_cnt = cdp->sql_result_cnt;
  sql_exec_direct(cdp, sqlcmd);
  cdp->sql_result_cnt = saved_cnt;
  cdp->prvs.is_data_admin--;
}

void index_doc_sources(cdp_t cdp, const t_fulltext * ftdef, bool altering)
// ftdef->schea is defined.
{ int i;
#ifdef STOP  // problem with multiple document columns sharing the same ID: dropping one drops all
  for (i=0;  i<ftdef->items.count();  i++)
  { const t_autoindex * ai = ftdef->items.acc0(i);
    char cmd[200+2*OBJ_NAME_LEN+2*ATTRNAMELEN+MAX_FT_FORMAT];
    if (ai->tag==ATR_DEL) 
    { sprintf(cmd, "for c as table `%s`.`%s` do call fulltext_remove_doc(\'%s.%s\',c.`%s`); end for;",
        *ai->doctable_schema ? ai->doctable_schema : ftdef->schema, ai->doctable_name, ftdef->schema, ftdef->name, 
        ai->id_column);
    }
    else if (ai->tag==ATR_NEW || !altering)
    { sprintf(cmd, "for c as table `%s`.`%s` do call fulltext_index_doc(\'%s.%s\',c.`%s`,c.`%s`,\'%s\',%d); end for;",
        *ai->doctable_schema ? ai->doctable_schema : ftdef->schema, ai->doctable_name, ftdef->schema, ftdef->name, 
        ai->id_column, ai->text_expr, ai->format, ai->mode);
    }
    else continue;
    cdp->prvs.is_data_admin++; // prevents checking privils 
    sql_exec_direct(cdp, cmd);
    cdp->prvs.is_data_admin--;
  }
#else // if any DROP present, reindex all
  bool any_dropping = false;
  for (i=0;  i<ftdef->items.count();  i++)
  { const t_autoindex * ai = ftdef->items.acc0(i);
    if (ai->tag==ATR_DEL) any_dropping = true;
  }
  bool rebuild_fulltext = any_dropping || !altering || ftdef->rebuild;
#if WBVERS>=950
  if (rebuild_fulltext && ftdef->separated)  // drop all
    refresh_index(cdp, ftdef);
  else // incremental rebuild
#endif
  { if (rebuild_fulltext)
      ft_purge(cdp, ftdef->name, ftdef->schema);
    for (i=0;  i<ftdef->items.count();  i++)
    { const t_autoindex * ai = ftdef->items.acc0(i);
      char cmd[200+2*OBJ_NAME_LEN+2*ATTRNAMELEN+MAX_FT_FORMAT];
      if (ai->tag==ATR_NEW || !altering || rebuild_fulltext && ai->tag==ATR_OLD)
      { sprintf(cmd, "for c as table `%s`.`%s` do call fulltext_index_doc(\'%s.%s\',c.`%s`,c.`%s`,\'%s\',%d); end for;",
          *ai->doctable_schema ? ai->doctable_schema : ftdef->schema, ai->doctable_name, ftdef->schema, ftdef->name, 
          ai->id_column, ai->text_expr, ai->format, ai->mode);
      }
      else continue;
      cdp->prvs.is_data_admin++; // prevents checking privils 
      sql_exec_direct(cdp, cmd);
      cdp->prvs.is_data_admin--;
    }
  }
#endif
}

int global_fulltext_info_generation = 0;
 
tattrib find_attrib(const table_descr * td, const char * name)
{ int i=1;
  while (i<td->attrcnt)
  { if (!strcmp(name, td->attrs[i].name)) return i;
    i++;  
  }
  return NOATTRIB;
}

/* Global list of fulltext systems: ref_cnt managed list of information about fulltext systems
   loaded from __RELTAB and FT description. */

static t_fulltext2 * global_ftxs = NULL;

void mark_fulltext_list(void)
{ ProtectedByCriticalSection cs(&cs_short_term);
  for (t_fulltext2 * ftx = global_ftxs;  ftx;  ftx=ftx->next)
    mark(ftx);
}

void drop_ft_ftom_list(tobjnum objnum)
{ ProtectedByCriticalSection cs(&cs_short_term);
  t_fulltext2 ** pftx = &global_ftxs;
  while (*pftx && (*pftx)->objnum!=objnum) pftx=&(*pftx)->next;
  if (*pftx) 
  { t_fulltext2 * ftx = *pftx;
    *pftx=ftx->next;
    ftx->Release();
  }
}

t_fulltext2 * get_fulltext_system(cdp_t cdp, tobjnum ftx_objnum, WBUUID * ftx_schema_id)
// Returns FT information from the global list, or creates it. Manages the ref_cnt. [schema] is defined in it.
// The caller must later call ftx->Release(); on the returned object.
{ t_fulltext2 * ftx; 
 // find existing ftx:
  { ProtectedByCriticalSection cs(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
    ftx = global_ftxs;
    while (ftx && ftx->objnum!=ftx_objnum) ftx=ftx->next;
    if (ftx) { ftx->AddRef();  return ftx; }
  }
 // load and compile the ftx description:
  t_fulltext2 * aftx = new t_fulltext2;
  if (!aftx) return NULL;
  char * defin = ker_load_objdef(cdp, objtab_descr, ftx_objnum);
  if (!defin) { delete aftx;  return NULL; }
  int res = aftx->compile(cdp, defin);
  corefree(defin);
  if (res) { delete aftx;  return NULL; } // compliation error
  aftx->objnum = ftx_objnum;  // will be searched by this value
 // add the schema name (never stored in the database):
  WBUUID schema_id;
  if (!ftx_schema_id)
  { tb_read_atr(cdp, objtab_descr, ftx_objnum, APPL_ID_ATR, (tptr)schema_id);
    ftx_schema_id = &schema_id;
  }
  ker_apl_id2name(cdp, *ftx_schema_id, aftx->schema, NULL);
  aftx->ftk = get_fulltext_kontext(cdp, NULLSTRING, aftx->name, aftx->schema);
  if (!aftx->ftk) { delete aftx;  return NULL; }  // FT system damaged
 // find existing ftx again:
  { ProtectedByCriticalSection cs(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
    ftx = global_ftxs;
    while (ftx && ftx->objnum!=ftx_objnum) ftx=ftx->next;
    if (ftx) { aftx->Release();  ftx->AddRef();  return ftx; }
   // add the new t_fulltext2 to the global list:
    aftx->next = global_ftxs;  global_ftxs = aftx;  aftx->AddRef();  // rec_cnt==2
  }
  return aftx;
}

t_ftx_info * get_ftx_info(cdp_t cdp, const table_descr * td)
// Creates the t_ftx_info for a table based on __RELTAB information
{ int space;  t_ftx_info * ftx_info;
  if (!*td->schemaname) get_table_schema_name(cdp, td->tbnum, (char *)td->schemaname);
 // 1st pass counts the references to the table columns, 2nd pass stores them in the allocated array: 
  for (int phase=0;  phase<2;  phase++)
  {// open terator on __RELTAB record: 
    t_reltab_index2 start(REL_CLASS_FTX, td->selfname, td->schema_uuid),
                    stop (REL_CLASS_FTX, td->selfname, td->schema_uuid);
    t_index_interval_itertor iii(cdp);
    if (!iii.open(REL_TABLENUM, REL_TAB_INDEX2, &start, &stop)) 
      return NULL;
   // pass the selected __RELTAB records:
    space=0;
    do
    { trecnum rec=iii.next();
      if (rec==NORECNUM) break;
      tobjname ftx_name;  WBUUID ftx_schema_id;
      if (fast_table_read(cdp, iii.td, rec, REL_PAR_NAME_COL, ftx_name) ||
          fast_table_read(cdp, iii.td, rec, REL_PAR_UUID_COL, (tptr)ftx_schema_id)) 
        break;
     // search for he table:
      tobjnum ftx_objnum = find2_object(cdp, CATEG_INFO, ftx_schema_id, ftx_name);
      if (ftx_objnum!=NOOBJECT)
      { t_fulltext2 * ftx = get_fulltext_system(cdp, ftx_objnum, &ftx_schema_id);
        if (ftx)
        { for (int i=0;  i<ftx->items.count();  i++)
          { const t_autoindex * ai = ftx->items.acc0(i);
            if (!strcmp(td->selfname, ai->doctable_name) &&
                !strcmp(td->schemaname, explic_schema_name(ai->doctable_schema, ftx->schema)))  // ftx->schema is defined by get_fulltext_system
            { tattrib text_atr = find_attrib(td, ai->text_expr);
              tattrib id_atr = find_attrib(td, ai->id_column);
              if (text_atr!=NOATTRIB && id_atr!=NOATTRIB)
              { if (phase==1) 
                { ftx_info[space].ftx_objnum = ftx_objnum;
                  ftx_info[space].id_col = id_atr;
                  ftx_info[space].text_col = text_atr;
                  ftx_info[space].item_index = i;
                }
                space++;  // must not count tables with changed columns
              }
            }
          }
          ftx->Release();
        }
      }
    } while (true);
    iii.close();
   // allocate the array if any valid data found:
    if (phase==0)
      if (!space) return NULL;
      else 
      { ftx_info=(t_ftx_info*)corealloc((space+1)*sizeof(t_ftx_info), 71);
        if (!ftx_info) return NULL;
      }
  }
  ftx_info[space].ftx_objnum=NOOBJECT;
  return ftx_info;
}

void table_descr::load_list_of_fulltexts(cdp_t cdp)
{ int i;
  if (fulltext_info_generation<global_fulltext_info_generation)
  { if (tbnum>0)
    { fulltext_info_list = get_ftx_info(cdp, this);
     // clear the column flags:
      for (i=1;  i<attrcnt;  i++)
        attrs[i].attrspec2 &= ~IS_IN_FULLTEXT;
     // set the column flags:
      if (fulltext_info_list)
        for (i=0;  fulltext_info_list[i].ftx_objnum!=NOOBJECT;  i++)
          attrs[fulltext_info_list[i].text_col].attrspec2 |= IS_IN_FULLTEXT;
    }
    fulltext_info_generation=global_fulltext_info_generation;
  }  
}

bool table_descr::get_next_table_ft(int & pos, const t_atrmap_flex & atrmap, t_ftx_info & info)
// Iterates on fulltext_info_list members whose [text_col] is in [atrmap].
{ ProtectedByCriticalSection cs(&lockbase.cs_locks);
 // check the passed positions (the list may have became shorter in between):
  if (!fulltext_info_list) return false;
  for (int mypos = 0;  mypos<=pos;  mypos++)
    if (fulltext_info_list[mypos].ftx_objnum==NOOBJECT) return false;  // end if the list
 // search:
  do
  { pos++;
    if (fulltext_info_list[pos].ftx_objnum==NOOBJECT) return false;  // end if the list
    if (atrmap.has(fulltext_info_list[pos].text_col))
    { info = fulltext_info_list[pos];
      return true;
    }
  } while (true);
}

bool table_descr::get_next_table_ft2(int & pos, const t_atrmap_flex & atrmap, t_ftx_info & info)
// Iterates on fulltext_info_list members which:
// - either have [text_col] in [atrmap],
// - or share [id_col] with a [text_col] which is in [atrmap].
{ ProtectedByCriticalSection cs(&lockbase.cs_locks);
 // check the passed positions (the list may have became shorter in between):
  if (!fulltext_info_list) return false;
  for (int mypos = 0;  mypos<=pos;  mypos++)
    if (fulltext_info_list[mypos].ftx_objnum==NOOBJECT) return false;  // end if the list
 // search:
  do
  { pos++;
    if (fulltext_info_list[pos].ftx_objnum==NOOBJECT) return false;
    if (atrmap.has(fulltext_info_list[pos].text_col))
    { info = fulltext_info_list[pos];
      return true;
    }
    else  // look for a text column containted in atrmap sharing id_col with the current text_col
    { tattrib id_col = fulltext_info_list[pos].id_col;
      for (int i = 0;  fulltext_info_list[i].ftx_objnum!=NOOBJECT;  i++)
        if (i!=pos && fulltext_info_list[i].id_col==id_col)  // text column sharing id column found
          if (atrmap.has(fulltext_info_list[i].text_col))  // .. and is in [atrmap]
          { info = fulltext_info_list[pos];
            return true;
          }
    }
  } while (true);
}


//////////////////////////// statement execution /////////////////////////////
// Leave-mode: except_type!=EXC_NO, leaving scopes (not entering exec's)
//  and looking for a label or for a handler (other than CONTINUE).

// exec returns TRUE iff CC or EC immediately occured but there is
// not a leave-mode yet (-> FALSE is returned in leave-mode).
// If except_type==EXC_NO then except_name contains unprocessed condition name.
// Otherwise except_name contains condition or label name for the leave-mode.
// except_type==EXC_NO on entry to each exec.

// In a statement list, when a statement returns TRUE, handler is searched.
// CONTINUE handlers are executed immediately (clearing the EC/CC).
// For other handlers or if handler not found, leave mode starts.

bool is_rollback_condition(cdp_t cdp)
{ int error_code;
  if (cdp->except_name[0]==SQLSTATE_EXCEPTION_MARK) 
    if (sqlstate_to_wb_error(cdp->except_name+1, &error_code))
    { t_condition_category ccat = condition_category(error_code);
      if (ccat==CONDITION_ROLLBACK) return true;
    }
  return false;
}

void process_rollback_conditions_by_cont_handler(cdp_t cdp)
// Called after statements.
{ if (cdp->except_type==EXC_EXC)
    if (!cdp->in_expr_eval)
      if (is_rollback_condition(cdp))
      { t_rscope * current_rscope;
        if (cdp->handler_rscope)  // I an in a handler, do not search a handler in the current rscope.
        { current_rscope=cdp->handler_rscope->next_rscope;
          if (!current_rscope) return;
        }
        else current_rscope = cdp->rscope;
        t_rscope * handler_rscope;  tobjnum handler_objnum;
        t_locdecl * hnddecl = get_continue_condition_handler(cdp, current_rscope, handler_rscope, handler_objnum);
        if (hnddecl)
        { execute_handler(cdp, hnddecl, handler_rscope, handler_objnum);  // execute the continue handler
          if (cdp->except_type==EXC_RESIGNAL) 
            cdp->except_type=EXC_EXC;
        }
      }
}

BOOL sql_exec_top_list(cdp_t cdp, sql_statement * so)
// Called for submitted & prepared SQL statements. Supposes so!=NULL.
// Returns TRUE on error. 
{ cdp->except_type=EXC_NO;  cdp->except_name[0]=0;  cdp->processed_except_name[0]=0;
  cdp->procedure_created_cursor = false;  // just to be sure
 // execute the statement list until exception:
  do  /* cycle on sql statements separated by semicolons */
  { cdp->statement_counter++;
    so->exec(cdp);
    process_rollback_conditions_by_cont_handler(cdp);
    so=so->next_statement;   /* to the next statement */
  } while (cdp->except_type==EXC_NO && so!=NULL);

 // if error, look for global EXIT handler
  if (cdp->except_type==EXC_EXC && cdp->global_sscope) // cdp->global_sscope may be NULL if no application still opened
  { t_locdecl * locdecl = get_noncont_condition_handler(cdp, cdp->global_sscope);
    if (locdecl!=NULL)  // execute the handler (it cannot be the continue handler)
    { execute_handler(cdp, locdecl, cdp->global_rscope, cdp->global_sscope->objnum);
      if (cdp->except_type==EXC_RESIGNAL) 
        cdp->except_type=EXC_EXC;   // do not continue searchig on this level
    }
  }
 // unhandled user-defined exception raised error in SIGNAL but the message has not been written into log:
  if (cdp->except_type==EXC_EXC && cdp->except_name[0]!=SQLSTATE_EXCEPTION_MARK && cdp->get_return_code()==SQ_UNHANDLED_USER_EXCEPT)
    if (!(cdp->ker_flags &  KFL_HIDE_ERROR))
      { SET_ERR_MSG(cdp, cdp->except_name);  err_msg(cdp, SQ_UNHANDLED_USER_EXCEPT); }
  cdp->procedure_created_cursor = false;  // must clear the flag, was not cleared in cursor created by a LOCAL procedure declared in a block.
  return cdp->except_type!=EXC_NO;
}

BOOL sql_exec_top_list_res2client(cdp_t cdp, sql_statement * so)
// Returns answer to the client. Adds uns32 result into answer for each statement generating result.
{ cdp->clear_sql_results();
  BOOL stmtres = sql_exec_top_list(cdp, so);
 // ODBC driver requires at least one result to be generated
  if (!cdp->sql_result_count()) cdp->add_sql_result(0xffffffff); 
 // copy all the results to the answer:
  unsigned result_space = cdp->sql_result_count()*sizeof(uns32);
  tptr pres = cdp->get_ans_ptr(sizeof(uns32)+result_space);
  if (pres==NULL) return TRUE;
#ifndef __ia64__  // alignment
   *(uns32*)pres = result_space;                                     // result size to the answer
#else
  memcpy(pres, &result_space, sizeof(uns32)); 
#endif
  memcpy(pres+sizeof(uns32), cdp->sql_results_ptr(), result_space); // result values to the answer
  return stmtres;
}

const char sqlexception_class_name[] = "SQLEXCEPTION",
           sqlwarning_class_name  [] = "SQLWARNING",
           not_found_class_name   [] = "NOT#FOUND";

static const char * get_condition_class_name(const char * except_name)
{ if (except_name[0]==SQLSTATE_EXCEPTION_MARK && except_name[1]=='0')
    if      (except_name[2]=='1') return sqlwarning_class_name;
    else if (except_name[2]=='2') return not_found_class_name;
  return sqlexception_class_name;
}

static t_locdecl * get_noncont_condition_handler(cdp_t cdp, const t_scope * scope)
// Looks for a non-continue condition handler declaration for the except_name in scope.
// Returns the handler declaration or NULL iff handler not found.
{ if (!cdp->except_name[0]) return NULL;  // looking for general handler needs it
  if (cdp->except_name[0]==SQLSTATE_EXCEPTION_MARK) 
    if (!may_have_handler(cdp->get_return_code(), cdp->in_trigger)) return NULL;  // prevents returning a general handler for unhandlable exceptions
 // find a specific handler:
  t_locdecl * locdecl = scope->find_name(cdp->except_name, LC_HANDLER);
  if (locdecl!=NULL && locdecl->handler.rtype!=HND_CONTINUE) return locdecl;
 // find a general handler:
  locdecl = scope->find_name(get_condition_class_name(cdp->except_name), LC_HANDLER);
  if (locdecl!=NULL)
  { if (locdecl->handler.rtype==HND_CONTINUE) return NULL;
    if (locdecl->handler.rtype==HND_EXIT) 
    { int error_code;
      if (sqlstate_to_wb_error(locdecl->exc.sqlstate, &error_code) && !may_have_handler(error_code, cdp->in_trigger))
        return NULL;
    }
  }
  return locdecl;
}

static void execute_handler(cdp_t cdp, t_locdecl * locdecl, t_rscope * handler_rscope, tobjnum handler_objnum)
// Executes handler declared in locdecl. Clears exception before.
// If handler_objnum==NOOBJECT then handler is defined in the object which is currently executed.
// Called only for exceptions that may have a handler.
{ t_idname outer_pen;  int rolled_back_error = 0;
 // save context:
  strcpy(outer_pen, cdp->processed_except_name);
  strcpy(cdp->processed_except_name, cdp->except_name);
  t_rscope * upper_handler_rscope = cdp->handler_rscope;
  t_rscope * current_rscope = cdp->rscope;
  cdp->rscope = cdp->handler_rscope = handler_rscope;
  if (cdp->except_name[0]==SQLSTATE_EXCEPTION_MARK) 
  { int error_code;
    if (sqlstate_to_wb_error(cdp->except_name+1, &error_code))
    { t_condition_category ccat = condition_category(error_code);
      if (ccat==CONDITION_ROLLBACK)
      { rolled_back_error = error_code;  // restore the error later
        if (locdecl->handler.rtype==HND_CONTINUE)
          request_error_no_context(cdp, error_code);  // not reported yet, report now, but without checking for handling
        if (cdp->in_expr_eval)
          cd_dbg_err(cdp, "WARNING: Rolling back in unsafe environment!");
        //if (!(cdp->ker_flags & KFL_DISABLE_ROLLBACK))
        if (cdp->current_cursor==NULL)  // probably not necessary
        { char buf[81];
          trace_msg_general(cdp, TRACE_IMPL_ROLLBACK, form_message(buf, sizeof(buf), msg_condition_rollback), NOOBJECT);
          roll_back(cdp);                                                                
        }
      }
    }
  }
  cdp->clear_exception();  cdp->roll_back_error=wbfalse;
#if 0
 // debugging support:
  tobjnum noscope_upper_objnum;  uns32 noscope_upper_line;  t_linechngstop noscope_upper_linechngstop;
  noscope_upper_objnum = cdp->top_objnum; // top_objnum defined by the last call of procedure or trigger
  if (handler_objnum!=NOOBJECT) cdp->top_objnum = handler_objnum;
  if (cdp->dbginfo)
  { noscope_upper_line         = cdp->dbginfo->curr_line;  // curr_line defined by the last call to check_break
    noscope_upper_linechngstop = cdp->dbginfo->linechngstop;
    if (cdp->dbginfo->linechngstop==LCHNGSTOP_SAME_LEVEL) cdp->dbginfo->linechngstop=LCHNGSTOP_NEVER;
  }
 // execute handler:
  locdecl->handler.stmt->exec(cdp);
 // restore debug info:
  if (cdp->dbginfo)
    if (cdp->dbginfo->linechngstop==LCHNGSTOP_NEVER) cdp->dbginfo->linechngstop = noscope_upper_linechngstop;
  cdp->top_objnum = noscope_upper_objnum;
#else
  if (handler_objnum!=NOOBJECT) cdp->top_objnum = handler_objnum;
  cdp->stk_add(cdp->top_objnum, NULL);  // the new top_objnum used here
 // execute handler:
  locdecl->handler.stmt->exec(cdp);
  cdp->stk_drop();
#endif
 // restore the error, if not handlable:
  if (rolled_back_error && cdp->except_type != EXC_RESIGNAL)
  { cdp->except_type = EXC_HANDLED_RB;
    strcpy(cdp->except_name, cdp->processed_except_name);
  }
 // restore context
  strcpy(cdp->processed_except_name, outer_pen);
  cdp->rscope = current_rscope;  // restores the rscope of the caller
  cdp->handler_rscope = upper_handler_rscope;
}

static t_locdecl * get_continue_condition_handler(cdp_t cdp, t_rscope *& start_rscope, t_rscope *& handler_rscope, tobjnum & handler_objnum)
// Returns nearest "continue" handler declaration for the except_name or NULL.
// When a "non-continue" handler is found nearer than a "continue" one, must return NULL.
// Called only for exceptions that may have a handler.
{ t_rscope * rscope;   t_locdecl * locdecl;
  if (!cdp->except_name[0]) return NULL;  // looking for general handler needs it
 // find a specific handler:
  for (rscope = start_rscope;  rscope!=NULL;  rscope=rscope->next_rscope)
    if (!rscope->sscope->params)
    { locdecl = rscope->sscope->find_name(cdp->except_name, LC_HANDLER);
      if (locdecl!=NULL) // stop searching when any type of handler found
      { handler_rscope = rscope;
        start_rscope = rscope->next_rscope;  // the rscope for the continued search if RESIGNALled
        handler_objnum=locdecl->handler.container_objnum;
        return locdecl->handler.rtype==HND_CONTINUE ? locdecl : NULL;
      }
    }
 // try the global declarations:
  if (cdp->global_sscope)
  { locdecl = cdp->global_sscope->find_name(cdp->except_name, LC_HANDLER);
    if (locdecl!=NULL) // stop searching when any type of handler found
    { handler_rscope = cdp->global_rscope;
      start_rscope = NULL;  // the rscope for the continued search if RESIGNALled
      handler_objnum=cdp->global_sscope->objnum;
      return locdecl->handler.rtype==HND_CONTINUE ? locdecl : NULL;
    }
  }

 // find a general handler:
  const char * class_name = get_condition_class_name(cdp->except_name);
  for (rscope = cdp->rscope;  rscope!=NULL;  rscope=rscope->next_rscope)
    if (!rscope->sscope->params)
    { locdecl = rscope->sscope->find_name(class_name, LC_HANDLER);
      if (locdecl!=NULL) // stop searching when any type of handler found
      { handler_rscope = rscope;
        start_rscope = rscope->next_rscope;  // the rscope for the continued search if RESIGNALled
        handler_objnum=locdecl->handler.container_objnum;
        return locdecl->handler.rtype==HND_CONTINUE ? locdecl : NULL;
      }
    }
 // try the global declarations:
  if (cdp->global_sscope)
  { locdecl = cdp->global_sscope->find_name(class_name, LC_HANDLER);
    if (locdecl!=NULL) // stop searching when any type of handler found
    { handler_rscope = cdp->global_rscope;
      start_rscope = NULL;  // the rscope for the continued search if RESIGNALled
      handler_objnum=cdp->global_sscope->objnum;
      return locdecl->handler.rtype==HND_CONTINUE ? locdecl : NULL;
    }
  }
  return NULL;  // start_rscope will be ignored in the calling function
}

BOOL try_handling(cdp_t cdp)  // except_type and except_name NOT defined on entry!
// Tries immediate handling the error or signal by CONTINUE handlers, returns TRUE iff handled.
{ t_rscope * current_rscope;
  bool snapshot_ready = false;
  if (cdp->handler_rscope)  // I an in a handler, do not search a handler in the current rscope.
  { current_rscope=cdp->handler_rscope->next_rscope;
    if (!current_rscope) return FALSE;
  }
  else current_rscope = cdp->rscope;
  do
  { t_rscope * handler_rscope;  tobjnum handler_objnum;
    t_locdecl * hnddecl = get_continue_condition_handler(cdp, current_rscope, handler_rscope, handler_objnum);
    if (hnddecl==NULL) return FALSE;  // continue handler not found, not handled
    if (!snapshot_ready) 
      { make_kontext_snapshot(cdp);  snapshot_ready=true; }
    execute_handler(cdp, hnddecl, handler_rscope, handler_objnum);  // execute the continue handler
    if (cdp->except_type==EXC_RESIGNAL) 
      { cdp->except_type=EXC_EXC;  return FALSE; }  // change from EXC_RESIGNAL, do not process on the same level
  } while (cdp->except_type==EXC_EXC);
  return TRUE;
}

struct t_sqlstate_conv
{ int error_code;
  char sqlstate[6];
};

static t_sqlstate_conv sqlstate_convtab[] =
{
  { NOT_FOUND_02000,           "02000" },

  { SQ_INVALID_CURSOR_STATE  , "24000" },
  { SQ_SAVEPOINT_INVAL_SPEC  , "3B001" },
  { SQ_SAVEPOINT_TOO_MANY    , "3B002" },
  { SQ_TRANS_STATE_ACTIVE    , "25001" },
  { SQ_INVAL_TRANS_TERM      , "2D000" },
  { SQ_TRANS_STATE_RDONLY    , "25006" },
  { SQ_NUM_VAL_OUT_OF_RANGE  , "22003" },
  { SQ_INV_CHAR_VAL_FOR_CAST , "22018" },
  { SQ_STRING_DATA_RIGHT_TRU , "22001" },
  { SQ_DIVISION_BY_ZERO      , "22012" },
  { SQ_CARDINALITY_VIOLATION , "21000" },
  { SQ_INVALID_ESCAPE_CHAR   , "22019" },
  { SQ_CASE_NOT_FOUND_STMT   , "20000" },
  { SQ_UNHANDLED_USER_EXCEPT , "45000" },
  { SQ_RESIGNAL_HND_NOT_ACT  , "0K000" },
  { SQ_EXT_ROUT_NOT_AVAIL    , "38001" },
  { SQ_NO_RETURN_IN_FNC      , "2F001" },
  { SQ_TRIGGERED_ACTION      , "09000" },
  { MUST_NOT_BE_NULL         , "40002" },
  { KEY_DUPLICITY            , "40004" },
  { CHECK_CONSTRAINT         , "40005" },
  { REFERENTIAL_CONSTRAINT   , "40006" },
  { SQ_INVALID_CURSOR_NAME   , "34000" },

  { 0, "" }
};

void wb_error_to_sqlstate(int error_code, char * except_name)
{ except_name[0]=SQLSTATE_EXCEPTION_MARK;
  for (int i=0;  sqlstate_convtab[i].error_code;  i++)
    if (sqlstate_convtab[i].error_code == error_code)
      { strcpy(except_name+1, sqlstate_convtab[i].sqlstate);  return; }
  sprintf(except_name+1, "W%04u", error_code);
}

BOOL sqlstate_to_wb_error(const char * sqlstate, int * error_code)
{ if (!*sqlstate) 
    { *error_code=0;  return TRUE; }
  for (int i=0;  sqlstate_convtab[i].error_code;  i++)
    if (!strcmp(sqlstate_convtab[i].sqlstate, sqlstate))
      { *error_code = sqlstate_convtab[i].error_code;  return TRUE; }
  sig32 conv;
  if (*sqlstate=='W' && str2int(sqlstate+1, &conv))
    { *error_code=conv;  return TRUE; }
  return FALSE;
}

void exec_stmt_list(cdp_t cdp, sql_statement * so)
// Execute list of statements with exception handling.
{ while (so!=NULL && cdp->except_type==EXC_NO)
  { cdp->statement_counter++;
    if (cdp->break_request)
      { request_error(cdp, REQUEST_BREAKED);  break; }
    so->exec(cdp);
    process_rollback_conditions_by_cont_handler(cdp);
    so=so->next_statement;   /* to the next statement */
  }
}

class t_table_access_for_altering
// The class is owning tbdf, if not NULL.
{ table_descr * tbdf;
 public:
  inline t_table_access_for_altering(void)
    { tbdf=NULL; }
  table_descr * open(cdp_t cdp, const tobjname schema_name, const tobjname table_name, WBUUID schema_uuid);
  inline void drop_ownership(void)
    { tbdf=NULL; }
  inline ~t_table_access_for_altering(void)
    { if (tbdf) unlock_tabdescr(tbdf); }
};

table_descr * t_table_access_for_altering::open(cdp_t cdp, const tobjname schema_name, const tobjname table_name, WBUUID schema_uuid)
// Finds schema_uuid and tablenum, checks privileges, installs and locks the tables, returns table_descr.
// Returns NULL on error, but the error may have been handled.
{ ttablenum tablenum;
 // get schema uuid of the table:
  if (!*schema_name) memcpy(schema_uuid, cdp->current_schema_uuid, UUID_SIZE);
  else if (ker_apl_name2id(cdp, schema_name, schema_uuid, NULL)) 
    { SET_ERR_MSG(cdp, schema_name);  request_error(cdp, OBJECT_DOES_NOT_EXIST);  return NULL; }  // may be handled
 // find the table and check privils:
  tablenum = find2_object(cdp, CATEG_TABLE, schema_uuid, table_name);
  if (tablenum==NOOBJECT)
    { SET_ERR_MSG(cdp, table_name);  request_error(cdp, OBJECT_DOES_NOT_EXIST);  return NULL; } // may be handled
  if (SYSTEM_TABLE(tablenum) || !can_write_objdef(cdp, tabtab_descr, tablenum))
    { request_error(cdp, NO_RIGHT);  return NULL; }
 // install the table:
  tbdf = install_table(cdp, tablenum);
  if (tbdf) tbdf->unique_value_cache.close_cache(cdp);  // must be called before free_descr which deletes cs_locks
  return tbdf;
}

BOOL sql_stmt_grant_revoke::single_oper(cdp_t cdp, table_descr * tbdf, trecnum recnum, privils & prvs)
{ t_privils_flex xpriv_val(objtab_descr->attrcnt);  int j;
  prvs.get_own_privils(tbdf, recnum, xpriv_val);
  if (statement_type==SQL_STAT_GRANT)
  { if (!cdp->prvs.can_grant_privils(cdp, &prvs, tbdf, recnum, priv_val))
      { request_error(cdp, NO_RIGHT);  return FALSE; }
    for (j=0;  j<xpriv_val.colbytes()+1;  j++) xpriv_val.the_map()[j] |=  priv_val.the_map()[j];
  }
  else  // REVOKE
    for (j=0;  j<xpriv_val.colbytes()+1;  j++) xpriv_val.the_map()[j] &= ~priv_val.the_map()[j];
  if (prvs.set_own_privils(tbdf, recnum, xpriv_val)) return FALSE;
  return TRUE;
}


BOOL sql_stmt_grant_revoke::exec(cdp_t cdp)
{ BOOL err=FALSE;  int i;  WBUUID routine_uuid;
  PROFILE_AND_DEBUG(cdp, source_line);
 // find object:
  tobjnum objnum;
  if (name_find2_obj(cdp, objname, schemaname, is_routine ? CATEG_PROC : CATEG_TABLE, &objnum))
    { SET_ERR_MSG(cdp, objname);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
  table_descr * tbdf;
  if (!is_routine) 
  { tbdf=install_table(cdp, objnum);
    if (!tbdf) return TRUE; 
  }
  else 
  { tbdf=NULL;  // init. to avoid a warning
    fast_table_read(cdp, objtab_descr, objnum, APPL_ID_ATR, routine_uuid);
  }
 // prepare:
  tcursnum cursnum;  cur_descr * cd;  trecnum reccnt;
  if (current_cursor_name[0]) // POSITIONED GRANT/REVOKE
  { if (find_named_cursor(cdp, current_cursor_name, cd) == NOCURSOR)
      { err=request_error(cdp, SQ_INVALID_CURSOR_NAME);   goto ex; }  // may be handled
    if ((int)cd->position < 0 || cd->position >= (int)cd->recnum)
      { err=request_error(cdp, SQ_INVALID_CURSOR_STATE);  goto ex; }  // may be handled
    if (cd->underlying_table==NULL || cd->underlying_table->tabnum!=objnum)
      { request_error(cdp, CANNOT_DELETE_IN_THIS_VIEW);  err=TRUE;  goto ex; }
  }
  else if (qe != NULL)   // operation on all cursor records in qe:
  { cursnum=create_cursor(cdp, qe, NULL, opt, &cd);
    if (cursnum==NOCURSOR) { err=TRUE;  goto ex; }
    unregister_cursor_creation(cdp, cd);
    make_complete(cd);
    if (cd->underlying_table==NULL || cd->underlying_table->tabnum!=objnum)
      { request_error(cdp, CANNOT_DELETE_IN_THIS_VIEW);  free_cursor(cdp, cursnum);  err=TRUE;  goto ex; }
  }

 // cycle on subjects:
  for (i=0;  i<user_names.count();  i++)
  { t_uname * subj = user_names.acc0(i);
    privils prvs(cdp);
    if (!*subj->subject_name)  // inserted by PUBLIC clause
      prvs.set_user(EVERYBODY_GROUP, CATEG_GROUP, FALSE);
    else  // find the subject
    { tobjnum userobj = NOOBJECT;
      if (subj->category==CATEG_USER || !subj->category)
        userobj = find_object(cdp, CATEG_USER, subj->subject_name);
      if (userobj!=NOOBJECT) subj->category=CATEG_USER; // may have not been specified
      else
      { if (subj->category==CATEG_GROUP || !subj->category)
          userobj = find_object(cdp, CATEG_GROUP, subj->subject_name);
        if (userobj!=NOOBJECT) subj->category=CATEG_GROUP; // may have not been specified
        else
        { if (subj->category==CATEG_ROLE)
            name_find2_obj(cdp, subj->subject_name, subj->subject_schema_name, CATEG_ROLE, &userobj);
        }
      }
      if (userobj!=NOOBJECT)
        prvs.set_user(userobj, subj->category, FALSE);
      else
        { SET_ERR_MSG(cdp, subj->subject_name);  request_error(cdp, OBJECT_NOT_FOUND);  err=TRUE;  goto ex1; }
    }

    if (current_cursor_name[0]) // POSITIONED GRANT/REVOKE
    { if (cd->curs_eseek(cd->position)) goto ex1;
      if (!single_oper(cdp, tbdf, cd->underlying_table->kont.position, prvs)) break;
      reccnt=1;
    }

    else if (qe != NULL)   // operation on all cursor records in qe:
    { for (trecnum r=0;  r<cd->recnum;  r++)
      { cd->curs_seek(r);
        if (!single_oper(cdp, tbdf, cd->underlying_table->kont.position, prvs)) 
          { free_cursor(cdp, cursnum);  break; }
        report_step(cdp, r);
      }
      reccnt=cd->recnum;
    }
    else if (is_routine)
      { t_privils_flex xpriv_val(objtab_descr->attrcnt);  int j;
        prvs.get_own_privils(objtab_descr, objnum, xpriv_val);
        if (statement_type==SQL_STAT_GRANT)
        { if (!cdp->has_full_access(routine_uuid))  // in can_grant_privils the full_access is not tested for the proper schema uuid
            if (!cdp->prvs.can_grant_privils(cdp, &prvs, objtab_descr, objnum, priv_val))
              { request_error(cdp, NO_RIGHT);  err=TRUE;  goto ex1; }
          for (j=0;  j<xpriv_val.colbytes()+1;  j++) xpriv_val.the_map()[j] |=  priv_val.the_map()[j];
        }
        else  // REVOKE
          for (j=0;  j<xpriv_val.colbytes()+1;  j++) xpriv_val.the_map()[j] &= ~priv_val.the_map()[j];
        if (prvs.set_own_privils(objtab_descr, objnum, xpriv_val)) { err=TRUE;  goto ex1; }
      }
    else // global table privils
    { if (!single_oper(cdp, tbdf, NORECNUM, prvs)) break;
      reccnt=1;
    }
  } // user cycle
 ex1:
  if (qe!=NULL)
  { free_cursor(cdp, cursnum);
    report_total(cdp, reccnt);
  }
 ex:
  if (!is_routine) unlock_tabdescr(tbdf);
  return err;
}

static BOOL drop_table(cdp_t cdp, ttablenum objnum, bool cascade)
// Drops a table without checking the privileges.
{ if (SYSTEM_TABLE(objnum))  { request_error(cdp, NO_RIGHT);  return TRUE; }
 // check the category of the table:
  tcateg categ;
  fast_table_read(cdp, tabtab_descr, objnum, OBJ_CATEG_ATR, &categ);
  if (categ & IS_LINK)   /* is not a real table, only a link */
  { tb_del(cdp, tabtab_descr, objnum);  
    psm_cache.invalidate(NOOBJECT, CATEG_PROC);  // destroys procedures and triggers
  } /* delete link only */
  else
  {// Deleting normal tables may be cascaded. Init the list of deleted tables.
    recnum_dynar table_list;
    trecnum * prec=table_list.next();
    if (!prec) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
    *prec=objnum;
    for (int bufpos=0;  bufpos<table_list.count();  bufpos++)
    { objnum=*table_list.acc0(bufpos);
      if (table_record_state(cdp, tabtab_descr, objnum)!=NOT_DELETED) // some tables may have been deleted by cascading!
        continue;
      if (/*wait_*/record_lock_error(cdp, tabtab_descr, objnum, WLOCK)) return TRUE;  // cannot lock
      // there is no good reason for waiting and the client has problems when minimized during the waiting
     // get schema uuid of the deleted table:
      WBUUID schema_uuid;  tobjname tabname;
      fast_table_read(cdp, tabtab_descr, objnum, OBJ_NAME_ATR, tabname);
      fast_table_read(cdp, tabtab_descr, objnum, APPL_ID_ATR,  schema_uuid);
     // drop references to parent tables:
      drop_ri_to_parent(cdp, tabname, schema_uuid);
     // delete references to domains:
      unregister_domains_in_table(cdp, tabname, schema_uuid);
     // find the dependent child tables to be deleted as well:
      if (!find_referencing_tables(cdp, tabname, schema_uuid, &table_list)) 
      { record_unlock(cdp, tabtab_descr, objnum, WLOCK);
        return TRUE;
      }
     // check for CASCADE if there is any child table to be deleted as well:
      if (bufpos==0 && !cascade && table_list.count()>1)
      { record_unlock(cdp, tabtab_descr, objnum, WLOCK);
        tobjname reftabname;
        fast_table_read(cdp, tabtab_descr, *table_list.acc0(1), OBJ_NAME_ATR, reftabname);
        SET_ERR_MSG(cdp, reftabname);
        request_error(cdp, REFERENCED_BY_OTHER_OBJECT);  
        return TRUE; 
      }
     /* deleting: */
      cdp->tl.unregister_changes_on_table(objnum);  // the table is permanent and is in the main log
      cdp->tl.log.table_deleted(objnum);  // little harm if DROP TABLE is rolled back
      table_rel(cdp, objnum);   /* release table data */
      tb_del(cdp, tabtab_descr, objnum);
      record_unlock(cdp, tabtab_descr, objnum, WLOCK);
      psm_cache.invalidate(NOOBJECT, CATEG_PROC);  // destroys procedures and triggers
      unregister_table_creation(cdp, objnum);
    } // cascading
  }
  return FALSE;
}


void drop_fulltext(cdp_t cdp, const char * name, const char * schema, WBUUID schema_uuid, tobjnum objnum)
// Removes the fulltext object and dependent objects (FTX_...). Unregisters tables defined in the fulltext.
// Removes fulltext kontext and information from global lists.
// Used by DROP fulltext and DROP SCHEMA.
{ unregister_tables_in_ftx(cdp, name, schema_uuid);
 // delete the FT objects: 
  ft_destroy(cdp, NULLSTRING, name, schema);  // removes the fulltext kontext from the global list, too
  global_fulltext_info_generation++;
 // remove from the list:  
  drop_ft_ftom_list(objnum);
}

BOOL sql_stmt_drop::exec(cdp_t cdp)
{ tobjnum objnum;
  PROFILE_AND_DEBUG(cdp, source_line);
  switch (categ)
  { case CATEG_APPL:  // I must not drop the schema if any its object is locked (in use)
    {// verify the consistency of indexes on main system tables:
      for (ttablenum tb = 0; tb<=OBJ_TABLENUM;  tb++)
      { table_descr_auto tbdf(cdp, tb);
        if (tbdf->me())
          for (int i = 0;  i<tbdf->indx_cnt;  i++)
          { int res = check_index(cdp, tbdf->me(), i);
            if (res!=1) 
            { SET_ERR_MSG(cdp, tbdf->selfname);  request_error(cdp, INDEX_DAMAGED);
              return TRUE;
            }
          }
      }
      //table_descr_auto reltab_tbdf(cdp, REL_TABLENUM);
      //if (!wait_record_lock_error(cdp, reltab_tbdf->me(), FULLTABLE, TMPWLOCK))  // locking order: reltab first, then tabtab
      { objnum=find_object(cdp, CATEG_APPL, schema);
        if (objnum==NOOBJECT)
          if (conditional) break;
          else { SET_ERR_MSG(cdp, schema);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); } // may be handled
        WBUUID apl_uuid;
        tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)apl_uuid);
       // privileges: AUTHOR and ADMINISTRATOR can drop the schema:
        if (!cdp->prvs.am_I_appl_admin(cdp, apl_uuid) && !cdp->prvs.am_I_appl_author(cdp, apl_uuid))
          if (!cdp->prvs.is_secur_admin && !cdp->prvs.is_data_admin)  // back door (db_admin can drop all objects)
            { request_error(cdp, NO_RIGHT);  return TRUE; }
       // check for other clients in this schema:
        bool found_other_client = false;
        { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
          for (int c=1;  c <= max_task_index;  c++)
          if (cds[c] && cds[c]!=cdp)
          { cdp_t acdp = cds[c];
            if (!memcmp(acdp->sel_appl_uuid, apl_uuid, UUID_SIZE))  // comparing back-end UUIDs
              found_other_client=true;
          }
        }
        if (found_other_client)
            return request_error(cdp, SCHEMA_IS_OPEN);  // may be handled
       // prepare cursors for schema objects:
        char query[200];  cur_descr * cd_tab, * cd_obj;  tcursnum curs_tab, curs_obj;  trecnum r, trec;
        sprintf(query, "SELECT * FROM TABTAB WHERE apl_uuid=(SELECT apl_uuid FROM OBJTAB WHERE obj_name=\'%s\' and category=chr(%d))", schema, CATEG_APPL);
        curs_tab = open_working_cursor(cdp, query, &cd_tab);
        if (curs_tab==NOCURSOR) return TRUE;
        make_complete(cd_tab);
        sprintf(query, "SELECT * FROM OBJTAB WHERE apl_uuid=(SELECT apl_uuid FROM OBJTAB WHERE obj_name=\'%s\' and category=chr(%d))", schema, CATEG_APPL);
        curs_obj = open_working_cursor(cdp, query, &cd_obj);
        if (curs_obj==NOCURSOR) { close_working_cursor(cdp, curs_tab);  return TRUE; }
        make_complete(cd_obj);
       // lock all objects:
        bool locked_all = true;
        for (r=0;  r<cd_tab->recnum;  r++)
        { cd_tab->curs_seek(r, &trec);
          if (record_lock_error(cdp, tabtab_descr, trec, WLOCK))
            { locked_all=false;  break; }
        }
        if (locked_all) 
          for (r=0;  r<cd_obj->recnum;  r++)
          { cd_obj->curs_seek(r, &trec);
            if (record_lock_error(cdp, objtab_descr, trec, WLOCK))
              { locked_all=false;  break; }
          }
        if (locked_all) 
        {// drop tables (must be before dropping domains):
          for (r=0;  r<cd_tab->recnum;  r++)
          { cd_tab->curs_seek(r, &trec);
            if (table_record_state(cdp, tabtab_descr, trec)==NOT_DELETED) // some tables may have been deleted by cascading!
              drop_table(cdp, trec, true);
          }
         // remove users & groups from application roles (must be done before deleting the roles):
          remove_app_roles(cdp, objnum);
         // delete objects from OBJTAB:
          for (r=0;  r<cd_obj->recnum;  r++)
          { cd_obj->curs_seek(r, &trec);
            if (table_record_state(cdp, objtab_descr, trec)!=NOT_DELETED) // FTX sequexnces may have been deleted by deleting the fulltext system
              continue;
            tcateg categ;
            fast_table_read(cdp, objtab_descr, trec, OBJ_CATEG_ATR, &categ);
            if (categ==CATEG_APPL) continue;
            if (categ==CATEG_INFO) 
            { tobjname name;
              fast_table_read(cdp, objtab_descr, trec, OBJ_NAME_ATR, name);
              drop_fulltext(cdp, name, schema, apl_uuid, trec);   // tb_del is inside
            }
            else  
            { if (categ==CATEG_SEQ) reset_sequence(cdp, trec);
              else if (categ==CATEG_TRIGGER) register_unregister_trigger(cdp, trec, FALSE);
              tb_del(cdp, objtab_descr, trec);
              if (categ==CATEG_PROC || categ==CATEG_TRIGGER)
                psm_cache.invalidate(trec, categ);  // removes from the cache unless executing
            }
          }
         // delete the schema object (must be the last, otherwise cannot properly delete fulltext systems - searching objects prefixed by schema name does not work)
          tb_del(cdp, objtab_descr, objnum);
        }
       // unlock:
        for (r=0;  r<cd_tab->recnum;  r++)
        { cd_tab->curs_seek(r, &trec);
          record_unlock(cdp, tabtab_descr, trec, WLOCK);
        }
        for (r=0;  r<cd_obj->recnum;  r++)
        { cd_obj->curs_seek(r, &trec);
          record_unlock(cdp, objtab_descr, trec, WLOCK);
        }
        close_working_cursor(cdp, curs_tab);
        close_working_cursor(cdp, curs_obj);
        if (!locked_all) return TRUE;  // nothing deleted
        if (commit(cdp))  // free_deleted can easilly bring an error but this is not a reasong to undo the DROP SCHEMA
          return TRUE;  // deletion rolled back
        uns32 saved_flags = cdp->ker_flags;  // do not report error when somebody works with system tables
        cdp->ker_flags |= KFL_HIDE_ERROR;
        free_deleted(cdp, tabtab_descr);
        free_deleted(cdp, objtab_descr);
        cdp->ker_flags = saved_flags;
        commit(cdp);  // free_deleted can easilly bring an error but this is not a reasong to undo the DROP SCHEMA
        request_error(cdp, 0);  // mask any error in free_deleted
       // delete objects from REPLTAB:
        table_descr * td = install_table(cdp, REPL_TABLENUM);
        if (td)
        { cur_descr * cd;
          strcpy(query, "SELECT * FROM REPLTAB WHERE appl_uuid=X'");
          int plen=(int)strlen(query);
          bin2hex(query+plen, apl_uuid, UUID_SIZE);  plen+=2*UUID_SIZE;
          query[plen++]='\'';  query[plen]=0;
          tcursnum curs = open_working_cursor(cdp, query, &cd);
          if (curs==NOCURSOR) return TRUE;
          make_complete(cd);
          for (r=0;  r<cd->recnum;  r++)
          { cd->curs_seek(r, &trec);
            tb_del(cdp, td, trec);
          }
          close_working_cursor(cdp, curs);
          free_deleted(cdp, td);
          unlock_tabdescr(td);
        }
       // if the deleted schema was the open schema, close it:
        if (!memcmp(apl_uuid, cdp->top_appl_uuid, UUID_SIZE) || !memcmp(apl_uuid, cdp->sel_appl_uuid, UUID_SIZE) || 
            !memcmp(apl_uuid, cdp->front_end_uuid, UUID_SIZE))
          ker_set_application(cdp, NULLSTRING, objnum);
      }
      break;
    }
    case CATEG_TABLE:
    { //table_descr_auto reltab_tbdf(cdp, REL_TABLENUM);
      //if (!wait_record_lock_error(cdp, reltab_tbdf->me(), FULLTABLE, TMPWLOCK))  // locking order: reltab first, then tabtab
      { if (name_find2_obj(cdp, main, schema, CATEG_TABLE|IS_LINK, &objnum))
          if (conditional) break;
          else { SET_ERR_MSG(cdp, main);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
       // check privileges:
        if (SYSTEM_TABLE(objnum) || !can_delete_record(cdp, tabtab_descr, objnum))
          { request_error(cdp, NO_RIGHT);  return TRUE; }
        drop_table(cdp, objnum, cascade);
      }
      break;
    } // DROP TABLE

    case CATEG_CURSOR:
     // find the object:
      if (name_find2_obj(cdp, main, schema, CATEG_CURSOR|IS_LINK, &objnum))
        if (conditional) break;
        else { SET_ERR_MSG(cdp, main);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
     // check the DELETE privilege:
      if (!can_delete_record(cdp, objtab_descr, objnum)) { request_error(cdp, NO_RIGHT);  return TRUE; }
     // delete it:
      tb_del(cdp, objtab_descr, objnum);
      break;

    case CATEG_SEQ:
     // find the object:
      if (name_find2_obj(cdp, main, schema, CATEG_SEQ|IS_LINK, &objnum))
        if (conditional) break;
        else { SET_ERR_MSG(cdp, main);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
     // check the DELETE privilege:
      if (!can_delete_record(cdp, objtab_descr, objnum)) { request_error(cdp, NO_RIGHT);  return TRUE; }
     // remove from the seq_cache:
      reset_sequence(cdp, objnum);
      tb_del(cdp, objtab_descr, objnum);
      psm_cache.invalidate(objnum, CATEG_PROC);  // removes from the cache unless executing (procs may refer the sequence)
      break;

    case CATEG_PROC:
     // find the routine:
      if (name_find2_obj(cdp, main, schema, CATEG_PROC|IS_LINK, &objnum))
        if (conditional) break;
        else { SET_ERR_MSG(cdp, main);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
     // check the DELETE privilege:
      if (!can_delete_record(cdp, objtab_descr, objnum)) { request_error(cdp, NO_RIGHT);  return TRUE; }
     // delete it:
      tb_del(cdp, objtab_descr, objnum);
      psm_cache.invalidate(objnum, CATEG_PROC);  // removes from the cache unless executing
      break;

    case CATEG_TRIGGER:
     // find the trigger: 
      if (name_find2_obj(cdp, main, schema, CATEG_TRIGGER|IS_LINK, &objnum))
        if (conditional) break;
        else { SET_ERR_MSG(cdp, main);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
     // check the DELETE privilege:
      if (!can_delete_record(cdp, objtab_descr, objnum)) { request_error(cdp, NO_RIGHT);  return TRUE; }
     // unregister the trigger:
      register_unregister_trigger(cdp, objnum, FALSE);
     // delete it:
      tb_del(cdp, objtab_descr, objnum);
      psm_cache.invalidate(objnum, CATEG_TRIGGER);  // removes from the cache unless executing
      break; // DROP TRIGGER

    case CATEG_DOMAIN:
     // find the domain: 
      if (name_find2_obj(cdp, main, schema, CATEG_DOMAIN|IS_LINK, &objnum))
        if (conditional) break;
        else { SET_ERR_MSG(cdp, main);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
     // check the DELETE privilege:
      if (!can_delete_record(cdp, objtab_descr, objnum)) { request_error(cdp, NO_RIGHT);  return TRUE; }
     // check if not used:
      if (check_for_or_delete_dependent_tables(cdp, main, objnum, cascade)) break;
     // delete it:
      tb_del(cdp, objtab_descr, objnum);
      break;

    case CATEG_INFO:
    {// find the object:
      if (name_find2_obj(cdp, main, schema, CATEG_INFO|IS_LINK, &objnum))
        if (conditional) break;
        else { SET_ERR_MSG(cdp, main);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
     // check the DELETE privilege:
      if (!can_delete_record(cdp, objtab_descr, objnum)) { request_error(cdp, NO_RIGHT);  return TRUE; }
     // unregister an delete:
      WBUUID uuid;  
      tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)uuid);
      if (!*schema) ker_apl_id2name(cdp, uuid, schema, NULL);  // may be different than cdp->sel_appl_name when executed in a remote procedure
      drop_fulltext(cdp, main, schema, uuid, objnum);
      break;
    }
    default:  // case CATEG_PGMSRC:  case CATEG_XMLFORM: case CATEG_STSH:
     // find the object:
      if (name_find2_obj(cdp, main, schema, categ|IS_LINK, &objnum))
        if (conditional) break;
        else { SET_ERR_MSG(cdp, main);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }  // may be handled
     // check the DELETE privilege:
      if (!can_delete_record(cdp, objtab_descr, objnum)) { request_error(cdp, NO_RIGHT);  return TRUE; }
     // delete it:
      tb_del(cdp, objtab_descr, objnum);
      break;
  }
  return FALSE;
}

cur_descr * get_cursor(cdp_t cdp, tcursnum cursnum)
{ cur_descr * cd;
  cursnum = GET_CURSOR_NUM(cursnum);
  if (cursnum >= crs.count())
    { request_error(cdp, CURSOR_MISUSE);  return NULL; }
  { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
    cd = *crs.acc0(cursnum);
  }
  if (cd==NULL || cd->owner != cdp->applnum_perm)
    { request_error(cdp, CURSOR_MISUSE);  return NULL; }
  return cd;
}

tcursnum find_named_cursor(const cdp_t cdp, const char * name, cur_descr *& rcd)
// Searches cursor by name, returns its number and cd.
// Returns the cursor with the biggest generation number.
{ tcursnum r_crnm = NOCURSOR;  rcd=NULL;  
  int found_generation = -1;
  { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
    for (tcursnum crnm=0;  crnm<crs.count();  crnm++)  
    { cur_descr * cd = *crs.acc0(crnm);
      if (cd!=NULL)
        if (cd->owner == cdp->applnum_perm)
          if (!strcmp(cd->cursor_name, name))
            if (cd->generation > found_generation)
              { rcd=cd;  r_crnm=crnm;  found_generation=cd->generation; }
    }
  }
  return r_crnm;
}

void assign_name_to_cursor(const cdp_t cdp, const char * new_cursor_name, cur_descr * cd)
// Stores the name of the cursor. If there is a cursor with the same name and owner, defines the generation number.
{ cur_descr * prev_cd;
  if (find_named_cursor(cdp, new_cursor_name, prev_cd) != NOCURSOR)
    cd->generation = prev_cd->generation+1;
  strcpy(cd->cursor_name, new_cursor_name);
}

BOOL sql_stmt_delete::cached_global_privils_ok(cdp_t cdp, table_descr * tbdf, BOOL & check_rec_privils)
// Checks the global DELETE privilege, uses the cache contents and stores the result into the cache.
// Returns FALSE if global privil is not granted and record privils are not defined.
{ check_rec_privils=FALSE;
  if (!cdp->has_full_access(tbdf->schema_uuid))
    if (!privil_test_result_cache.has_valid_positive_test_result(cdp)) // unless checked OK
      if (!privil_test_result_cache.has_valid_negative_test_result(cdp)) // unless checked FAIL
      { if (can_delete_record(cdp, tbdf, NORECNUM))  // has global privilege
          privil_test_result_cache.store_positive_test_result(cdp); // privils OK, put the result into the cache
        else // global privils failed
        { privil_test_result_cache.store_negative_test_result(cdp); // privils FAIL, put the result into the cache
          if (tbdf->rec_privils_atr==NOATTRIB)
            return FALSE;
          else check_rec_privils=TRUE;
        }
      }
      else // global privils failed
        if (tbdf->rec_privils_atr==NOATTRIB)
          return FALSE; 
        else check_rec_privils=TRUE;
  return TRUE;
}

BOOL sql_stmt_delete::record_privils_ok(cdp_t cdp, table_descr * tbdf, trecnum tr)
{ return can_delete_record(cdp, tbdf, tr); }

BOOL sql_stmt_delete::exec(cdp_t cdp)
// qe has been checked: it allows delete operation
{ ttablenum tbnum;  tcursnum cursnum;  cur_descr * cd;  
  trecnum reccnt;  // records deleted
  BOOL check_rec_privils;  
  PROFILE_AND_DEBUG(cdp, source_line);
  if (!cdp->trans_rw) return request_error(cdp, SQ_TRANS_STATE_RDONLY);  // may be handled
  cdp->exec_start();
  if (current_cursor_name[0]) // POSITIONED DELETE
  { if (find_named_cursor(cdp, current_cursor_name, cd) == NOCURSOR)
      return request_error(cdp, SQ_INVALID_CURSOR_NAME);  // may be handled
    if ((int)cd->position < 0 || cd->position >= (int)cd->recnum)
      return request_error(cdp, SQ_INVALID_CURSOR_STATE);  // may be handled
    if (cd->underlying_table==NULL)
      { request_error(cdp, CANNOT_DELETE_IN_THIS_VIEW);  goto err; }
    if (cd->pmat==NULL) // cursor is materialised, using in DELETE... CURRENT OF should be disabled, I think
      { request_error(cdp, CANNOT_APPEND);  goto err; }
   // position the record:
    if (cd->curs_eseek(cd->position)) goto err;
    tbnum = cd->underlying_table->tabnum;
    table_descr * tbdf = tables[tbnum]; // cursor opened -> tabdescr locked
    trecnum tr = cd->underlying_table->kont.position;
    if (!ttr_prepared)
      if (ttr.prepare_rscopes(cdp, tbdf, TGA_DELETE))
        ttr_prepared=TRUE;
      else { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
   // check privils:
    if (tbnum==TAB_TABLENUM)
      { request_error(cdp, NO_RIGHT);  goto err; }
    if (!cached_global_privils_ok(cdp, tbdf, check_rec_privils)) 
      { request_error(cdp, NO_RIGHT);  goto err; }
    if (check_rec_privils && !record_privils_ok(cdp, tbdf, tr)) 
      { request_error(cdp, NO_RIGHT);  goto err; }
   // BEF triggers:
    if (ttr.pre_stmt(cdp, tbdf)) goto err;
    if (ttr.pre_row(cdp, tbdf, tr, NULL)) goto err;
   // delete the record:
    if (tb_del(cdp, tbdf, tr)) goto err;
   // remove the record from cursor:
    if (PTRS_CURSOR(cd)) cd->pmat->curs_del(cdp, cd->position);
   // AFT triggers (deallocates rscope):
    if (ttr.post_row(cdp, tbdf, tr)) goto err;
    if (ttr.post_stmt(cdp, tbdf)) goto err;
    reccnt=1;
  }

  else if (qe != NULL)   // operation on all cursor records in qe:
  { trecnum r;
    cursnum=create_cursor(cdp, qe, NULL, opt, &cd);
    if (cursnum==NOCURSOR) goto err;
    unregister_cursor_creation(cdp, cd);
    make_complete(cd);
    if (cd->underlying_table==NULL)
      { request_error(cdp, CANNOT_DELETE_IN_THIS_VIEW);  goto errexc; }
    tbnum=cd->underlying_table->tabnum;
    table_descr * tbdf;  tbdf = tables[tbnum];  // cursor opened -> tabdescr locked
   // check global privils:
    if (tbnum==TAB_TABLENUM)
      { request_error(cdp, NO_RIGHT);  goto errexc; }
    if (!cached_global_privils_ok(cdp, tbdf, check_rec_privils)) 
      { request_error(cdp, NO_RIGHT);  goto errexc; }
   // prepare & BEF triggers:
    if (!ttr_prepared)
      if (ttr.prepare_rscopes(cdp, tbdf, TGA_DELETE))
        ttr_prepared=TRUE;
      else { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errexc; }
    else tbdf->prepare_trigger_info(cdp);  // the trigger_map may have been dropped since prepare_rscopes
    if (ttr.pre_stmt(cdp, tbdf)) goto errexc;
   // passing the cursor records:
    if (cd->recnum>1000) record_lock(cdp, tbdf, NORECNUM, TMPWLOCK);
    /* no error if cannot lock, will be slower */
    for (r=0;  r<cd->recnum;  r++)
    { cd->curs_seek(r);
      trecnum tr=cd->underlying_table->kont.position;
      if (check_rec_privils && !record_privils_ok(cdp, tbdf, tr)) 
        { request_error(cdp, NO_RIGHT);  goto errexc; }
     // BEF ROW triggers:
      if (ttr.pre_row(cdp, tbdf, tr, NULL)) goto errexc;
     // delete record:
      if (tb_del(cdp, tbdf, tr) || cdp->except_type!=EXC_NO) goto errexc;
     // AFT ROW triggers:
      if (ttr.post_row(cdp, tbdf, tr)) goto errexc;
      report_step(cdp, r);
    }
   // AFT STM triggers:
    if (ttr.post_stmt(cdp, tbdf)) goto errexc;
   // finishing:
    reccnt=cd->recnum;
    free_cursor(cdp, cursnum);
    report_total(cdp, reccnt);
    goto ok;
   errexc:
    free_cursor(cdp, cursnum);
    goto err;
  }

  else // operation on full table so->objnum
  { trecnum r;
    if (locdecl_offset!=-1) // local table, objnum not defined!
      tbnum = *(ttablenum*)variable_ptr(cdp, locdecl_level, locdecl_offset);
    else  // permanent table
      tbnum = objnum;  
    if (tbnum==TAB_TABLENUM)
      { request_error(cdp, NO_RIGHT);  goto err; }
    table_descr_auto tbdf(cdp, tbnum);
    if (tbdf->me()==NULL) goto err;
   // check global privils:
    if (!cached_global_privils_ok(cdp, tbdf->me(), check_rec_privils)) 
      { request_error(cdp, NO_RIGHT);  goto errex; }
   // prepare & BEF triggers:
    if (!ttr_prepared)
      if (ttr.prepare_rscopes(cdp, tbdf->me(), TGA_DELETE))
        ttr_prepared=TRUE;
      else { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errex; }
    else tbdf->prepare_trigger_info(cdp);  // the trigger_map may have been dropped since prepare_rscopes
    if (ttr.pre_stmt(cdp, tbdf->me())) goto errex;
   // passing the table records:
    record_lock(cdp, tbdf->me(), NORECNUM, TMPWLOCK);
    /* no error if cannot lock, will be slower */
    reccnt=0;
    for (r=0;  r<tbdf->Recnum();  r++)
    { if (table_record_state(cdp, tbdf->me(), r)) continue;
      if (check_rec_privils && !record_privils_ok(cdp, tbdf->me(), r)) 
        { request_error(cdp, NO_RIGHT);  goto errex; }
     // BEF ROW triggers:
      if (ttr.pre_row(cdp, tbdf->me(), r, NULL)) goto errex;
     // delete record:
      if (tb_del(cdp, tbdf->me(), r) || cdp->except_type!=EXC_NO) goto errex;
     // AFT ROW triggers:
      if (ttr.post_row(cdp, tbdf->me(), r)) goto errex;
      report_step(cdp, r);
      reccnt++;
    }
   // AFT STM triggers:
    if (ttr.post_stmt(cdp, tbdf->me())) goto errex;
    report_total(cdp, reccnt);
    goto ok;
   errex:
    goto err;
  }
 ok:
  cdp->__rowcount=reccnt;
  cdp->add_sql_result(reccnt);
  return FALSE;
 err:
  return TRUE;
}
////////////////////////// system variables ////////////////////////////////
void get_sys_var_value(cdp_t cdp, int sysvar_ident, t_value * val)
{ val->set_simple_not_null();  // used in most cases, but can be changed
  switch (sysvar_ident)
  { case SYSVAR_ID_ROWCOUNT: // @@ROWCOUNT
      val->intval=0;
      if (IS_CURSOR_NUM(cdp->__rowcount))
      { tcursnum cursnum = GET_CURSOR_NUM(cdp->__rowcount);
        if (cursnum < crs.count())
        { cur_descr * cd;
          { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
            cd = *crs.acc0(cursnum);
          }
          if (cd && cd->owner == cdp->applnum_perm)
            { make_complete(cd);  val->intval=cd->recnum; }
        }
      }
      else
        val->intval=cdp->__rowcount;
      break;
    case SYSVAR_ID_FULLTEXT_WEIGHT: // @@FULLTEXT_WEIGHT
      val->intval=cdp->__fulltext_weight;  break;
    case SYSVAR_ID_FULLTEXT_POSITION:
      val->intval=cdp->__fulltext_position;  break;
    case SYSVAR_ID_PLATFORM: // @@PLATFORM
      val->intval=CURRENT_PLATFORM;  break;
    case SYSVAR_ID_WAITING:
      val->intval=cdp->waiting;  break;
    case SYSVAR_ID_SQLOPTIONS:
      val->intval=cdp->sqloptions;  break;
    case SYSVAR_ID_SQLCODE:
    { int error_code;
      if (cdp->processed_except_name[0]==SQLSTATE_EXCEPTION_MARK &&
          sqlstate_to_wb_error(cdp->processed_except_name+1, &error_code))
        val->intval=error_code;
      else
        val->set_null();
      break;
    }
    case SYSVAR_ID_SQLSTATE:
      if (cdp->processed_except_name[0]==SQLSTATE_EXCEPTION_MARK)
        strcpy(val->strval, cdp->processed_except_name+1);
      else val->strval[0]=0;
      val->length=(int)strlen(val->strval);
      break;
    case SYSVAR_ID_LAST_EXC:
      strcpy(val->strval, cdp->last_exception_raised);
      val->length=(int)strlen(val->strval);
      break;
    case SYSVAR_ID_ROLLED_BACK:
    { if (cdp->processed_except_name[0]==SQLSTATE_EXCEPTION_MARK)
      { int error_code;
        val->intval=sqlstate_to_wb_error(cdp->processed_except_name+1, &error_code) &&
                    condition_category(error_code)==CONDITION_ROLLBACK;
      }
      else
        val->set_null();
      break;
    }
    case SYSVAR_ID_ACTIVE_RI:
      val->intval=cdp->in_active_ri != 0;
      break;
    case SYSVAR_ID_ERROR_MESSAGE:
    { int error_code;
      if (cdp->processed_except_name[0]!=SQLSTATE_EXCEPTION_MARK ||
          !sqlstate_to_wb_error(cdp->processed_except_name+1, &error_code))
        { val->set_null();  break; }
        val->intval=error_code;
      int space = 1024;
      if (val->allocate(space)) val->allocate(space=MAX_DIRECT_VAL);
      Get_server_error_text(cdp, error_code, val->valptr(), space);
      val->length=(int)strlen(val->valptr());
      break;
    }
    case SYSVAR_ID_IDENTITY:
      val->i64val=cdp->last_identity_value;  break;
    default:
      val->set_null();  break;
  }
}

BOOL set_sys_var_value(cdp_t cdp, int sysvar_ident, t_value * val)
{ switch (sysvar_ident)
  { case SYSVAR_ID_WAITING:
      cdp->waiting=val->intval;  break;
    case SYSVAR_ID_SQLOPTIONS:
      cdp->sqloptions=val->intval;  break;
    default:
      return FALSE; 
  }
  return TRUE;
}

////////////////////////// assignment and UPDATE ////////////////////////////
// Optimization:
// - checking global privils not in exec, but in preparing statement
// - evaluating attr_access in preparing, impossible for updatable UNION, may prepare default as normal expression

BOOL write_value_to_attribute(cdp_t cdp, t_value * res, table_descr * dest_tbdf, trecnum recnum, int attrnum, t_express * indexexpr, int stype, int dtype)
// Does not support loaded values of var-len types (results of operations).
// [dtype] is supposed to be the real type of the [attrnum] column, not tested.
// Write privileges are supposed to be tested by the caller.
// Triggers not called here, this function shoud not be used.
{ uns16 indxval;  BOOL result;
  tptr ptr = res->valptr();  // NULL value is supposed to be represented
  if (indexexpr!=NULL)
  { t_value ires;
    expr_evaluate(cdp, indexexpr, &ires);
    if (ires.is_null() || ires.intval<0 || ires.intval>0xffff) 
      { request_error(cdp, INDEX_OUT_OF_RANGE);  return TRUE; }
    indxval=(uns16)ires.intval;
    stype &= 0x7f;
  }
  else indxval=NOINDEX; // used below

  if (attrnum==DEL_ATTR_NUM) // maybe not necessary
    { request_error(cdp, NO_RIGHT);  return TRUE; }
  if (dtype & 0x80 || stype & 0x80 ||  IS_HEAP_TYPE(dtype & 0x7f) && IS_HEAP_TYPE(stype & 0x7f) && res->vmode==mode_access)
  { table_descr_auto td(cdp, res->dbacc.tb);
    if (td->me())
      copy_var_len(cdp, dest_tbdf, recnum, attrnum, indxval,
        td->me(), res->dbacc.rec, res->dbacc.attr, res->dbacc.index, NULL);
    return FALSE;
  }

  if (IS_HEAP_TYPE(dtype & 0x7f) || IS_EXTLOB_TYPE(dtype & 0x7f))  // simple type to var len
  { int bytesize = res->is_null() ? 0 :
               SPECIFLEN(stype) || IS_HEAP_TYPE(stype) || IS_EXTLOB_TYPE(stype) || stype==ATT_CHAR ? res->length :
               tpsize[stype & 0x7f];  // must not use this for ATT_CHAR, may be wide
    if (indexexpr!=NULL)
    { result=tb_write_ind_var(cdp, dest_tbdf, recnum, attrnum, indxval, 0, bytesize, ptr) |
             tb_write_ind_len(cdp, dest_tbdf, recnum, attrnum, indxval,    bytesize);
    }
    else
    { result =tb_write_var    (cdp, dest_tbdf, recnum, attrnum, 0, bytesize, ptr) |
              tb_write_atr_len(cdp, dest_tbdf, recnum, attrnum,    bytesize);
    }
  }
  else // simple type write
  { if (dtype==ATT_BINARY && res->length<dest_tbdf->attrs[attrnum].attrspecif.length)  // must expand the value by 0s
    { int ln=dest_tbdf->attrs[attrnum].attrspecif.length;
      if (res->reallocate(ln)) request_error(cdp, OUT_OF_KERNEL_MEMORY);
      else memset(res->valptr()+res->length, 0, ln-res->length);
    }
    if (indexexpr!=NULL)
      result=tb_write_ind(cdp, dest_tbdf, recnum, attrnum, indxval, ptr);
    else
      result=tb_write_atr(cdp, dest_tbdf, recnum, attrnum, ptr);
  }
  return result;
}

void set_default_value(cdp_t cdp, t_value * res, const attribdef * att, table_descr * tbdf)
// res->vmode not set properly. Does not work with longer values.
{ 
  if (att->defvalexpr==NULL)
  { res->set_null();
    qlset_null(res, att->attrtype, att->attrspecif.length);
  }  
  else if (att->defvalexpr==COUNTER_DEFVAL)
    if (tbdf)  // unique generation enabled
    { res->set_simple_not_null();
      res->i64val=0;  // result may be stored then into a 64-bit integer
      res->unsval=tbdf->unique_value_cache.get_next_available_value(cdp);
    }
    else 
    { res->set_null();
      qlset_null(res, att->attrtype, att->attrspecif.length);
    }  
  else 
    expr_evaluate(cdp, att->defvalexpr, res);
}

BOOL execute_assignment(cdp_t cdp, const t_express * dest, t_express * source, BOOL check_wr_right, BOOL check_rd_right, BOOL protected_execution)
// Assigns [source] value to [dest].
{ t_value res, ires; 
 // evaluate source:
  if (source!=NULL)
  { cdp->check_rights = (wbbool)check_rd_right;
    if (protected_execution) expr_evaluate(cdp, source, &res);
    else                stmt_expr_evaluate(cdp, source, &res);
    cdp->check_rights = true;
    if (cdp->except_name[0]) return TRUE;  // error when evaluating the expression
    if (res.is_null())
      if (!IS_HEAP_TYPE(source->type) && !(source->type & 0x80)) 
        qlset_null(&res, dest->type, dest->specif.length);  // value is converted, use dest->
  }

 // analyse the destination atribute:
  const t_expr_attr * atex;  trecnum recnum;  
  ttablenum tabnum;  tattrib attrnum;  elem_descr * attracc;
  if      (dest->sqe==SQE_ATTR)
    atex=(const t_expr_attr*)dest;
  else if (dest->sqe==SQE_POINTED_ATTR)
    atex=((const t_expr_pointed_attr*)dest)->pointer_attr;
  else if (dest->sqe==SQE_VAR_LEN)
    atex=((const t_expr_var_len*)dest)->attrex;
  else if (dest->sqe==SQE_MULT_COUNT)
    atex=((const t_expr_mult_count*)dest)->attrex;
  else if (dest->sqe==SQE_DBLIND)
    atex=((const t_expr_dblind*)dest)->attrex;
  else if (dest->sqe==SQE_VAR)
  { const t_expr_var * varex = (const t_expr_var*)dest;
    if (varex->level==SYSTEM_VAR_LEVEL) set_sys_var_value(cdp, varex->offset, &res);
    else  // writing to a declared variable
    { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
      if (IS_HEAP_TYPE(dest->type) || IS_EXTLOB_TYPE(dest->type))
      { t_value * val = (t_value*)varbl;
        val->set_null(); 
        if (source!=NULL && !res.is_null()) *val=res; // operator=
        val->load(cdp);  // must not remain indirect, deflock is not locked later
      }
      else
      { if (source==NULL)  // DEFAULT for variable is NULL (used when initializing non-specified input parameter)
          qset_null(varbl, dest->type, dest->specif.length);
        else 
        { if (IS_STRING(dest->type) || dest->type==ATT_CHAR)
          { int copylen;
            if (res.is_null()) copylen=0;
            else if (res.length > dest->specif.length)  // not checked in conversion (for both 8 and 16 bit chars)
            { copylen=dest->specif.length;
              if (!(cdp->sqloptions & SQLOPT_MASK_RIGHT_TRUNC)) 
                sql_exception(cdp, SQ_STRING_DATA_RIGHT_TRU);  // may be handled
            }
            else copylen = res.length;
            memcpy(varbl, res.valptr(), copylen);
            varbl[copylen]=0;
            if (dest->specif.wide_char) varbl[copylen+1]=0;
          }
          else if (dest->type==ATT_BINARY)
          { if (res.length<dest->specif.length)
            { memcpy(varbl, res.valptr(), res.length);
              memset(varbl+res.length, 0, dest->specif.length-res.length);
            }
            else memcpy(varbl, res.valptr(), dest->specif.length);
          }
          else memcpy(varbl, res.valptr(), simple_type_size(dest->type, dest->specif));
        }
      }
    }
    return FALSE;
  }
  else if (dest->sqe==SQE_DYNPAR)
  { if (source!=NULL)
    { if (res.is_null()) qlset_null(&res, dest->type, dest->specif.length);
      res.load(cdp);
      set_dp_value(cdp, ((const t_expr_dynpar*)dest)->num, &res, source->type);
    }
    return FALSE;
  }
  else if (dest->sqe==SQE_EMBED)
  { if (res.is_null()) qlset_null(&res, dest->type, dest->specif.length);
    t_emparam * empar = cdp->sel_stmt->find_eparam_by_name(((const t_expr_embed*)dest)->name);
    if (empar!=NULL)
    { if (IS_HEAP_TYPE(empar->type) || IS_EXTLOB_TYPE(empar->type))
      { if (res.length > empar->length)
        { empar->val=(tptr)corerealloc(empar->val, res.length);
          if (!empar->val) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
        }
        res.load(cdp);
        memcpy(empar->val, res.valptr(), res.length);
        empar->length=res.length;
      }
      else 
      { if (IS_STRING(dest->type)) 
          if (dest->specif.wide_char)
          { int datalen = res.length;
            if (datalen>dest->specif.length) datalen=dest->specif.length;
            memcpy(empar->val, res.valptr(), datalen);
            empar->val[datalen]=empar->val[datalen+1]=0;
          }
          else strmaxcpy(empar->val, res.valptr(), dest->specif.length+1);
        else if (dest->type==ATT_BINARY && res.length<dest->specif.length)  // padd by 0s
        { memcpy(empar->val, res.valptr(), res.length);
          memset(empar->val+res.length, 0, dest->specif.length-res.length);
        }
        else  // fixed size, copy all
          memcpy(empar->val, res.valptr(), simple_type_size(dest->type, dest->specif));
      }
    }
    return FALSE;
  }
  else { request_error(cdp, ASSERTION_FAILED);  return TRUE; }

 // write to the attribute (value, length, count, part of value, pointed attr):
  const db_kontext * akont = atex->kont;  int elemnum = atex->elemnum;
  attr_access_light(akont, elemnum, recnum);
  attracc=akont->elems.acc0(elemnum);
  if (attracc->link_type==ELK_NO)
  { tabnum=attracc->dirtab.tabnum;
    attrnum=elemnum;
    if (tabnum==NOTBNUM) { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  return TRUE; }
  }
  else { request_error(cdp, ASSERTION_FAILED);  return TRUE; }
  table_descr_auto dest_tbdf(cdp, tabnum);
  if (!dest_tbdf->me()) return TRUE;
  if (!cdp->has_full_access(dest_tbdf->schema_uuid) && check_wr_right && dest->sqe!=SQE_POINTED_ATTR)
    if (akont->privil_cache && !(akont->privil_cache->can_write_column(cdp, attrnum, dest_tbdf->me(), recnum)))
      { request_error(cdp, NO_RIGHT);  goto err; }

  if (source==NULL)  // prepare DEFAULT value for destination
    set_default_value(cdp, &res, dest_tbdf->attrs+attrnum, dest_tbdf->me());

  if (dest->sqe==SQE_ATTR)
  { if (write_value_to_attribute(cdp, &res, dest_tbdf->me(), recnum, attrnum, atex->indexexpr,
        source==NULL ? dest->type : source->type, dest->type))
      goto err;
  }
  else if (dest->sqe==SQE_POINTED_ATTR)
  { t_value auxval;
    const t_expr_pointed_attr * poatr = (const t_expr_pointed_attr*)dest;
   // read the pointer value:
    if (akont->privil_cache && !(akont->privil_cache->can_read_column(cdp, attrnum, dest_tbdf->me(), recnum)))
      { request_error(cdp, NO_RIGHT);  goto err; }
    tb_attr_value(cdp, dest_tbdf->me(), recnum, attrnum, atex->origtype, atex->origspecif, atex->indexexpr, &auxval);
   // check write privils (not cached):
    { table_descr_auto dest_tbdf2(cdp, poatr->desttb);
      if (!dest_tbdf2->me()) goto err;
      if (check_wr_right && !cdp->has_full_access(dest_tbdf2->schema_uuid))
      { t_privils_flex priv_val;
        cdp->prvs.get_effective_privils(dest_tbdf2->me(), auxval.intval, priv_val);
        if (!priv_val.has_write(poatr->elemnum))
          { request_error(cdp, NO_RIGHT);  goto err; }
      }
     // write to the pointed column:
      if (write_value_to_attribute(cdp, &res, dest_tbdf2->me(), auxval.intval, poatr->elemnum, poatr->indexexpr,
        source==NULL ? dest->type : source->type, dest->type))
          goto err;
    }
  }
  else if (dest->sqe==SQE_VAR_LEN)
  { if (res.intval<0) goto err;
    uns32 len = res.is_null() ? 0 : res.unsval;
    if (atex->indexexpr!=NULL)
    { if (dest_tbdf->attrs[attrnum].attrmult == 1) { request_error(cdp, BAD_MODIF);  return TRUE; }
      expr_evaluate(cdp, atex->indexexpr, &ires);
      if (ires.is_null() || ires.intval<0 || ires.intval>0xffff) 
        { request_error(cdp, INDEX_OUT_OF_RANGE);  goto err; }
      tb_write_ind_len(cdp, dest_tbdf->me(), recnum, attrnum, (uns16)ires.intval, len);
    }
    else
    { if (dest_tbdf->attrs[attrnum].attrmult != 1) { request_error(cdp, BAD_MODIF);  return TRUE; }
      tb_write_atr_len(cdp, dest_tbdf->me(), recnum, attrnum, len);
    }
  }
  else if (dest->sqe==SQE_MULT_COUNT)
  { if (dest_tbdf->attrs[attrnum].attrmult == 1) { request_error(cdp, BAD_MODIF);  return TRUE; }
    unsigned len = res.is_null() ? 0 : res.unsval;
    if (len>0xffff) { request_error(cdp, INDEX_OUT_OF_RANGE);  goto err; }
    tb_write_ind_num(cdp, dest_tbdf->me(), recnum, attrnum, (t_mult_size)len);
  }
  else if (dest->sqe==SQE_DBLIND)
  { expr_evaluate(cdp, ((t_expr_dblind*)dest)->start, &ires);
    if (ires.is_null() || ires.intval < 0) { request_error(cdp, INDEX_OUT_OF_RANGE);  goto err; }
    int start=ires.intval;
    expr_evaluate(cdp, ((t_expr_dblind*)dest)->size,  &ires);
    if (ires.is_null() || ires.intval < 0 || ires.intval > 0xffff) { request_error(cdp, INDEX_OUT_OF_RANGE);  goto err; }
    int size =ires.intval;
    if (atex->indexexpr!=NULL)
    { if (dest_tbdf->attrs[attrnum].attrmult == 1) { request_error(cdp, BAD_MODIF);  return TRUE; }
      expr_evaluate(cdp, atex->indexexpr, &ires);
      if (ires.is_null() || ires.intval<0 || ires.intval>0xffff) 
        { request_error(cdp, INDEX_OUT_OF_RANGE);  goto err; }
      tb_write_ind_var(cdp, dest_tbdf->me(), recnum, attrnum, (uns16)ires.intval, start, size, res.valptr());
    }
    else
    { if (dest_tbdf->attrs[attrnum].attrmult != 1) { request_error(cdp, BAD_MODIF);  return TRUE; }
      tb_write_var    (cdp, dest_tbdf->me(), recnum, attrnum, start, size, res.valptr());
    }
  }
  return FALSE;
 err:
  return TRUE;
}

BOOL sql_stmt_update::exec(cdp_t cdp)
{ ttablenum tbnum;  trecnum reccnt;  // records updated
  PROFILE_AND_DEBUG(cdp, source_line);
  if (!cdp->trans_rw) return request_error(cdp, SQ_TRANS_STATE_RDONLY);  // may be handled
  cdp->exec_start();

  if (current_cursor_name[0])  // positioned UPDATE ... WHERE CURRENT OF
  { cur_descr * cd;
    if (find_named_cursor(cdp, current_cursor_name, cd) == NOCURSOR)
      return request_error(cdp, SQ_INVALID_CURSOR_NAME);  // may be handled
    if ((int)cd->position < 0 || (int)cd->position >= (int)cd->recnum)
      return request_error(cdp, SQ_INVALID_CURSOR_STATE);  // may be handled
    if (!iuts.init_ttrs(cdp, NULL, cd, FALSE))
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
   // position the record:
    if (cd->curs_eseek(cd->position)) return TRUE;
    int priv_state = iuts.is_globally_updatable_in_cursor(cdp, cd);
    if (priv_state==0 || priv_state==2 && !iuts.is_record_updatable_in_cursor(cdp, cd, cd->position)) 
      { request_error(cdp, NO_RIGHT);  return TRUE; }
    if (iuts.pre_stmt_cursor(cdp, cd)) return TRUE;  // BEF UPD STM triggers
   // update the record:
    if (aux_qe!=NULL)  // SETs compiled in this kontext, replace
      aux_qe->kont.replacement=&cd->qe->kont;
    if (iuts.execute_write_cursor(cdp, cd, cd->position)) return TRUE;
    if (iuts.post_stmt_cursor(cdp, cd)) return TRUE;  // AFT UPD STM triggers
    reccnt=1;
  }

  else if (qe != NULL)   // operation on all cursor records in qe:
  { cur_descr * cd;
    tcursnum cursnum=create_cursor(cdp, qe, NULL, opt, &cd);
    if (cursnum==NOCURSOR) return TRUE;
    unregister_cursor_creation(cdp, cd);
    make_complete(cd);
    if (!iuts.init_ttrs(cdp, NULL, cd, FALSE))
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errexc; }
    if (cd->recnum)
    { int priv_state = iuts.is_globally_updatable_in_cursor(cdp, cd);
      if (priv_state==0) { request_error(cdp, NO_RIGHT);  return TRUE; }
      if (iuts.pre_stmt_cursor(cdp, cd)) goto errexc;  // BEF UPD STM triggers
     // passing the cursor records:
      if (cd->recnum>5000)
      { int limit=1000;
        cd->qe->locking(cdp, NULL, OP_SUBMIT, limit); // OP_SUBMIT causes setting TMPWLOCK, without error checking
      } /* no error if cannot lock, will be slower */
      for (trecnum r=0;  r<cd->recnum;  r++)
      { cd->curs_seek(r);
        if (priv_state==2 && !iuts.is_record_updatable_in_cursor(cdp, cd, r))
          { request_error(cdp, NO_RIGHT);  goto errexc; }
        cdp->statement_counter++;  // NEXT VALUE FOR seq should return a new value
        if (iuts.execute_write_cursor(cdp, cd, r)) goto errexc;
        report_step(cdp, r);
      }
      if (iuts.post_stmt_cursor(cdp, cd)) goto errexc;  // AFT UPD STM triggers
    }
    reccnt=cd->recnum;
    free_cursor(cdp, cursnum);
    report_total(cdp, reccnt);
    goto ok;
   errexc:
    free_cursor(cdp, cursnum);
    return TRUE;
  }

  else // operation on full table so->objnum
  { trecnum r;
    if (locdecl_offset!=-1) // local table, objnum not defined!
      tbnum = *(ttablenum*)variable_ptr(cdp, locdecl_level, locdecl_offset);
    else  // permanent table
      tbnum = objnum;  
    if (tbnum==TAB_TABLENUM)
      { request_error(cdp, NO_RIGHT);  return TRUE; }
    table_descr_auto tbdf(cdp, tbnum);
    if (tbdf->me()==NULL) return TRUE;
    if (!iuts.init_ttrs(cdp, tbdf->me(), NULL, FALSE))
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
    reccnt=0;
    if (tbdf->Recnum())
    { int priv_state = iuts.is_globally_updatable_in_table(cdp, tbdf->me(), &tabkont);
      if (priv_state==0) { request_error(cdp, NO_RIGHT);  return TRUE; }
      if (iuts.pre_stmt_table(cdp, tbdf->me())) return TRUE;  // BEF UPD STM triggers
     // passing the table records:
      record_lock(cdp, tbdf->me(), NORECNUM, TMPWLOCK);
      if (cdp->isolation_level >= REPEATABLE_READ)  
        record_lock(cdp, tbdf->me(), NORECNUM, TMPRLOCK);  // it is probable that the records will be read during the update, reducing the space necessary for READ locks
      /* no error if cannot lock, will be slower */
      for (r=0;  r<tbdf->Recnum();  r++)
      { if (table_record_state(cdp, tbdf->me(), r)) continue;
        tabkont.position=r;
        if (priv_state==2 && !iuts.is_record_updatable_in_table(cdp, tbdf->me(), &tabkont, r))
          { request_error(cdp, NO_RIGHT);  return TRUE; }
        cdp->statement_counter++;  // NEXT VALUE FOR seq should return a new value
        if (iuts.execute_write_table(cdp, tbdf->me(), r)) return TRUE;
        report_step(cdp, r);
        reccnt++; // counting total updated record
      }
      if (iuts.post_stmt_table(cdp, tbdf->me())) return TRUE;  // AFT UPD STM triggers
    }
    report_total(cdp, reccnt);
  }
 ok:
  cdp->__rowcount=reccnt;
  cdp->add_sql_result(reccnt);
  return FALSE;
}

trecnum new_record_in_cursor(cdp_t cdp, trecnum trec, cur_descr * cd, tcursnum crnm)
// Inserts the record into a cursor and its supercursors, returns the record number related to the 1st cursor.
{ make_complete(cd);
  trecnum rec=cd->recnum;
  do /* inserting into cursor & supercursors */
  { if (!PTRS_CURSOR(cd))
      { request_error(cdp, CANNOT_APPEND);  return NORECNUM; }
    if (!cd->pmat->put_recnums(cdp, &trec))
      { request_error(cdp, CANNOT_APPEND);  return NORECNUM; }
    cd->recnum++;
   // registering the cursor insertion:
    ins_curs * ic = cdp->d_ins_curs.next();  
    if (ic!=NULL) { ic->cursnum=crnm;  ic->recnum=cd->recnum-1; }
   // going to the supercursor, if any:
    if (!cd->subcurs) break;
    if (!IS_CURSOR_NUM(cd->super)) break;
    crnm=GET_CURSOR_NUM(cd->super);
    { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
      cd=*crs.acc(crnm);
    }
  } while (cd!=NULL);
  return rec; 
}


BOOL write_insert_rec_ex(cdp_t cdp, BOOL inserting, cur_descr * cd, tcursnum crnm, table_descr * tbdf, trecnum recnum, int col_count, const tattrib * cols, const char * databuf)
// Input recnum used only when !insering. Append the new record iff inserting==2.
{// check INSERT privil if inserting, load privils if single table used
  if (tbdf && tbdf->tbnum==TAB_TABLENUM) { request_error(cdp, NO_RIGHT);  return TRUE; }
  t_privils_flex priv_val;
  if (inserting)
  { if (cd)
    { if (cd->underlying_table==NULL)
        { request_error(cdp, CANNOT_APPEND);  return TRUE; }
      if (!(cd->qe->qe_oper & QE_IS_INSERTABLE))
        { request_error(cdp, CANNOT_INSERT_INTO_THIS_VIEW);  return TRUE; }
      tbdf=tables[cd->underlying_table->tabnum];  // tabdescr locked in the cursor
    }
    if (tbdf->tbnum==TAB_TABLENUM || tbdf->tbnum==OBJ_TABLENUM)  // this is not allowed, must use CREATE statement or cd_Isert_object
      { request_error(cdp, NO_RIGHT);  return TRUE; }
    cdp->prvs.get_effective_privils(tbdf, NORECNUM, priv_val);
    if (!(*priv_val.the_map() & RIGHT_APPEND))
    { t_exkont_table ekt(cdp, tbdf->tbnum, NORECNUM, NOATTRIB, NOINDEX, OP_INSERT, NULL, 0);
      request_error(cdp, NO_RIGHT);  return TRUE; 
    }
  }
  else 
    if (cd) // Write
    { if (!(cd->qe->qe_oper & QE_IS_UPDATABLE))
        { request_error(cdp, CANNOT_UPDATE_IN_THIS_VIEW);  return TRUE; }
    }
    else
      cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);

 // prepare column description for vector operation:
  t_ins_upd_trig_supp iuts;  BOOL privil_error=FALSE;  
  if (!iuts.start_columns(col_count))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
  unsigned data_offset = 0;
  for (int i=0;  i<col_count;  i++)
  { if (!iuts.register_column(cols[i], cd ? &cd->qe->kont : NULL, NULL))
      { SET_ERR_MSG(cdp, cd ? cd->qe->kont.elems.acc(cols[i])->name : tbdf->attrs[cols[i]].name);  request_error(cdp, COLUMN_NOT_EDITABLE);  return TRUE; } 
   // check priviledes, find table column:
    tattrib attr;  attribdef * att;  
    if (!cd)
    { attr=cols[i];
      att=tbdf->attrs+attr;
      if (!inserting && !priv_val.has_write(attr)) privil_error=TRUE;
    }
    else // cursor record write
    { elem_descr * attracc;  ttablenum tbnum;  trecnum tabrec;
      attr_access(&cd->qe->kont, cols[i], tbnum, attr, tabrec, attracc);
      if (attracc!=NULL || tbnum==TAB_TABLENUM) 
        { privil_error=TRUE;  break; }
      tbdf=tables[tbnum];  att=tbdf->attrs+attr;
      if (!inserting && !(cd->d_curs->attribs[cols[i]].a_flags & RI_UPDATE_FLAG))
        if (tbdf->tabdef_flags & TFL_REC_PRIVILS)
        { cdp->prvs.get_effective_privils(tbdf, tabrec, priv_val);
          if (!priv_val.has_write(attr)) privil_error=TRUE;
        }
        else privil_error=TRUE;
    }
   // add data pointer:
    if (IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype)) 
    { uns32 * lenptr = (uns32*)(databuf + data_offset);
      data_offset+=sizeof(uns32);
      iuts.add_ptrs((char*)databuf + data_offset, lenptr, NULL);
      data_offset+=*lenptr;
    }
    else
    { iuts.add_ptrs((char*)databuf + data_offset, NULL, NULL);
      int len = TYPESIZE(att);
      if (IS_STRING(att->attrtype)) len++;
      data_offset+=len;
    }
  }
  iuts.stop_columns();

  if (privil_error) 
  { if (tbdf)
    { t_exkont_table ekt(cdp, tbdf->tbnum, inserting ? NORECNUM : recnum, NOATTRIB, NOINDEX, inserting ? OP_INSERT : OP_WRITE, NULL, 0);
      request_error(cdp, NO_RIGHT);  
    }
    else request_error(cdp, NO_RIGHT);  
    return TRUE; 
  }
  if (!iuts.init_ttrs(cdp, tbdf, cd, inserting))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
 // BEF STM triggers:
  if (cd)
    { if (iuts.pre_stmt_cursor(cdp, cd))  return TRUE; }
  else
    { if (iuts.pre_stmt_table(cdp, tbdf)) return TRUE; }
 // write data:
  if (inserting)
  { if (cd) make_complete(cd);  // must be done before inserting the record into the table, otherwise it can (but need not) add the new record into the cursor
    trecnum result_recnum = iuts.execute_insert(cdp, tbdf, inserting==2);
    if (cd && result_recnum!=NORECNUM)
      result_recnum=new_record_in_cursor(cdp, result_recnum, cd, crnm);
    *(trecnum*)cdp->get_ans_ptr(sizeof(trecnum)) = result_recnum;
  }
  else  // updating
    if (cd)
      iuts.execute_write_cursor(cdp, cd, recnum);
    else
      iuts.execute_write_table(cdp, tbdf, recnum);
 // AFT STM triggers:
  if (cd)
    { if (iuts.post_stmt_cursor(cdp, cd))  return TRUE; } 
  else
    { if (iuts.post_stmt_table(cdp, tbdf)) return TRUE; } 
  return FALSE;
}

BOOL write_one_column(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib colnum, 
                      const char * val, uns32 offset, uns32 size)
// Used for API - Write, writes column value without shrinking the var-len values.
// offset==(uns32)-1 says that offset is 0 and the total length should be set.
{ t_colval colvals[2];
  colvals[0].attr=colnum;
  colvals[0].table_number=0;
  colvals[0].index=NULL;
  colvals[0].dataptr=val;
  colvals[0].lengthptr=&size;
  colvals[0].offsetptr=&offset;
  colvals[0].expr=NULL;
  colvals[1].attr=NOATTRIB;
  t_vector_descr vector(colvals); 
  if (offset==(uns32)-1) 
    offset=0;  // vector.setsize remains true
  else
    vector.setsize=false;
  t_tab_trigger ttr;
  if (!ttr.prepare_rscopes(cdp, tbdf, TGA_UPDATE)) return TRUE;
  ttr.add_column_to_map(colnum);
  if (ttr.pre_stmt(cdp, tbdf)) return TRUE;
  if (tb_write_vector(cdp, tbdf, recnum, &vector, FALSE, &ttr)) return TRUE;
  if (ttr.post_stmt(cdp, tbdf)) return TRUE;
  return FALSE;
}

////////////////////////////////// support for external record insert/update /////////////////////////////////
t_ins_upd_ext_supp::~t_ins_upd_ext_supp(void)
{ if (tbdf) unlock_tabdescr(tbdf);
  if (databuf) corefree(databuf);
  if (stmt) cdp->drop_statement(stmt->hstmt);
  if (column_list) corefree(column_list);
}

t_ins_upd_ext_supp * eius_prepare_orig(cdp_t cdp, ttablenum tbnum)
{ return eius_prepare(cdp, tbnum, NULL); }

t_ins_upd_ext_supp * eius_prepare(cdp_t cdp, ttablenum tbnum, tattrib * attrs)
// Registering ALL columns: easier access to the columns[] array.
// [table_num] says if the column value was specified and should be written.
{ t_ins_upd_ext_supp * eius = new t_ins_upd_ext_supp(cdp);
  if (!eius) return NULL;
  eius->tbdf=install_table(cdp, tbnum);
  if (!eius->tbdf) { delete eius;  return NULL; }
  if (!eius->iuts_insert.start_columns(eius->tbdf->attrcnt) || !eius->iuts_update.start_columns(eius->tbdf->attrcnt))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  delete eius;  return NULL; }
 // calculate the size of databuf - contains space for values of ALL columns with fixed types and lengths of var-len:
  int i, databuf_size = 0, ind;
  for (i=0;  i<eius->tbdf->attrcnt;  i++)
  { attribdef * att=eius->tbdf->attrs+i;
    if (att->attrmult==1)
      if (IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype)) 
        databuf_size+=sizeof(uns32);
      else
      { databuf_size+=TYPESIZE(att);
        if (IS_STRING(att->attrtype)) 
        { databuf_size++;  
          if (att->attrspecif.wide_char) databuf_size++;  
        }
      }
  }
 // allocate buffer for the row data:
  eius->databuf=(char*)corealloc(databuf_size, 51);
  if (!eius->databuf) { delete eius;  return NULL; }
  memset(eius->databuf, 0, databuf_size);  // initially cleared
 // register columns used in insert/update:
  int data_offset = 0;
  for (i=0;  i<eius->tbdf->attrcnt;  i++)
  { attribdef * att = eius->tbdf->attrs+i;
    bool is_key_column = false;
    if (attrs!=NULL)
    { ind = 0;
      while (attrs[ind] && attrs[ind]!=i) ind++;
      if (attrs[ind]) is_key_column = true;
    }
    eius->iuts_insert.register_column(i, NULL, NULL);
#if SEP_EIUS
    if (!is_key_column) 
#endif
    eius->iuts_update.register_column(i, NULL, NULL);
    if (att->attrmult!=1)
    { eius->iuts_insert.add_ptrs((char*)DATAPTR_DEFAULT, NULL, NULL);
#if SEP_EIUS
      if (!is_key_column) 
#endif
      eius->iuts_update.add_ptrs((char*)DATAPTR_DEFAULT, NULL, NULL);
    }
    else if (IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype)) 
    { uns32 * lenptr = (uns32*)(eius->databuf + data_offset);
      eius->iuts_insert.add_ptrs(NULL, lenptr, NULL);
#if SEP_EIUS
      if (!is_key_column) 
#endif
      eius->iuts_update.add_ptrs(NULL, lenptr, NULL);
      data_offset+=sizeof(uns32);
    }
    else
    { eius->iuts_insert.add_ptrs(eius->databuf + data_offset, NULL, NULL);
#if SEP_EIUS
      if (!is_key_column) 
#endif
      eius->iuts_update.add_ptrs(eius->databuf + data_offset, NULL, NULL);
      int len = TYPESIZE(att);
      if (IS_STRING(att->attrtype)) len++;
      data_offset+=len;
    }
  }
  eius->iuts_insert.stop_columns();
  eius->iuts_update.stop_columns();
  if (!eius->iuts_insert.init_ttrs(cdp, eius->tbdf, NULL, true))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  delete eius;  return NULL; }
  if (!eius->iuts_update.init_ttrs(cdp, eius->tbdf, NULL, false))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  delete eius;  return NULL; }
  if (attrs) eius->make_search_statement(cdp, attrs);
  return eius;     
}

bool t_ins_upd_ext_supp::make_search_statement(cdp_t cdp, tattrib * attrs)
// attrs is an array of column numbers terminated by 0.
{ int ind;  t_sql_kont sql_kont;  
  wbstmt * upper_sel_stmt = cdp->sel_stmt;  // sel_stmt may describe emb. vars in the SQL statement passed to the server
  if (!*attrs) return true;  // empty list -> no statement
 // copy the column list:
  ind=0;
  while (attrs[ind]) ind++;
  column_list = (tattrib*)corealloc(sizeof(tattrib) * (ind+1), 66);
  if (!column_list) return false;
  memcpy(column_list, attrs, sizeof(tattrib) * (ind+1));
 // create a new statement with an unique handle:
  uns32 handle = cdp->get_new_stmt_handle();
  stmt = new wbstmt(cdp, handle);
  if (!stmt) return false;
 // create the query and variables:
  t_flstr query;
  query.put("SELECT * FROM ");
  query.putq(tbdf->selfname);
  query.put(" WHERE ");
  for (ind = 0;  attrs[ind];  ind++)
  { attribdef * att = &tbdf->attrs[attrs[ind]];
   // add the condition:
    if (ind) query.put(" AND ");
    query.putq(att->name);
    query.put("=:P");
    query.putint2(ind);
   // describe the variable:
    t_emparam * ep = stmt->eparams.acc(ind);
    sprintf(ep->name, "P%u", ind);
    ep->type=att->attrtype;
    ep->specif=att->attrspecif;
    ep->mode=MODE_IN;
    ep->val=NULL;  // will be defined before the execution of the statement
    ep->val_is_indep=true;
  }
  if (query.error()) goto err1;
 // compile the query:
  //stmt->so = sql_submitted_comp(cdp, query.get(), NULL);
  //if (!stmt->so) goto err1;
  sql_stmt_select * so;
  stmt->so = so = new sql_stmt_select();
  if (!so) goto err1;
  cdp->sel_stmt=stmt;
  if (compile_query(cdp, query.get(), &sql_kont, &so->qe, &so->opt)) goto err1;
  cdp->sel_stmt=upper_sel_stmt;

  return true;
 err1:
  cdp->drop_statement(handle);
  stmt=NULL;  // prevents using the searching
  cdp->sel_stmt=upper_sel_stmt;
  return false;
}

trecnum t_ins_upd_ext_supp::find_record_by_key_columns(cdp_t cdp)
{ int ind;  trecnum recnum = NORECNUM;
  wbstmt * upper_sel_stmt = cdp->sel_stmt;  // sel_stmt may describe emb. vars in the SQL statement passed to the server
 // link embvars to values in the data buffer:
  for (ind = 0;  column_list[ind];  ind++)
  { attribdef * att = &tbdf->attrs[column_list[ind]];
    t_emparam * ep = stmt->eparams.acc(ind);
    ep->val = iuts_insert.get_dataptr(column_list[ind]);   // and length??
    if (!IS_HEAP_TYPE(att->attrtype) && !IS_EXTLOB_TYPE(att->attrtype))
      ep->length = TYPESIZE(att);
  }
  sql_stmt_select * so = (sql_stmt_select *)stmt->so;
  cdp->sel_stmt=stmt;
  so->opt->reset(cdp);
  if (so->opt->get_next_record(cdp))
  { 
#if SQL3
    int ordnum = 0;
    so->qe->get_position_indep(&recnum, ordnum);
#else
    so->qe->get_position(&recnum);
#endif
  }
  so->opt->close_data(cdp);
  cdp->sel_stmt=upper_sel_stmt;
  return recnum;
}

void eius_drop(t_ins_upd_ext_supp * eius)
{ delete eius; }

void eius_clear(t_ins_upd_ext_supp * eius)
{ for (int i=0;  i<eius->tbdf->attrcnt;  i++)
  { attribdef * att = eius->tbdf->attrs+i;
    if (att->attrmult==1)
      eius->iuts_insert.release_data(i, IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype));
    eius->iuts_insert.disable_pos(i);
    eius->iuts_update.disable_pos(i);
  }
}

bool eius_store_data(t_ins_upd_ext_supp * eius, int pos, char * dataptr, unsigned size, bool appending)  // appends data to a var-len buffer
{ eius->iuts_insert.enable_pos(pos);
  eius->iuts_update.enable_pos(pos);
  return eius->iuts_insert.store_data(pos, dataptr, size, appending); 
}

char * eius_get_dataptr(t_ins_upd_ext_supp * eius, int pos)
{ eius->iuts_insert.enable_pos(pos);
  eius->iuts_update.enable_pos(pos);
  return eius->iuts_insert.get_dataptr(pos); 
}

trecnum eius_insert(cdp_t cdp, t_ins_upd_ext_supp * eius)
{ trecnum result_recnum;
 // search the record, if key columns specified:
  if (eius->stmt)
  { result_recnum=eius->find_record_by_key_columns(cdp);
    if (result_recnum!=NORECNUM)
    {// borrow [colvals] from iuts_insert:
      t_colval * upd_colvals = eius->iuts_update.colvals;
      eius->iuts_update.colvals = eius->iuts_insert.colvals;
      if (eius->iuts_update.pre_stmt_table(cdp, eius->tbdf)) 
        result_recnum=NORECNUM; 
      else  // write data:
      { eius->iuts_update.execute_write_table(cdp, eius->tbdf, result_recnum);
        if (eius->iuts_update.post_stmt_table(cdp, eius->tbdf)) 
          result_recnum=NORECNUM; 
      }
      eius->iuts_update.colvals = upd_colvals;
      return result_recnum;
    }
  }
 // not searching or not found: insert a new record:
  if (eius->iuts_insert.pre_stmt_table(cdp, eius->tbdf)) return NORECNUM; 
  result_recnum = eius->iuts_insert.execute_insert(cdp, eius->tbdf, false);
  if (eius->iuts_insert.post_stmt_table(cdp, eius->tbdf)) return NORECNUM;
  return result_recnum;
}

bool eius_read_column_value(cdp_t cdp, t_ins_upd_ext_supp * eius, trecnum recnum, int pos)
{ char * ptr = eius->iuts_insert.get_dataptr(pos);
  return !tb_read_atr(cdp, eius->tbdf, recnum, pos, ptr);
}
///////////////////////////////////////////// INSERT /////////////////////////////////////////////////////
#ifdef STOP
BOOL materialize_vector(cdp_t cdp, t_vector_descr * vector, table_descr * tbdf)
{ BOOL res=FALSE;  t_value val;
  const t_colval * colval = vector->colvals;  attribdef * att = tbdf->attrs+1;
  while (colval->attr!=NOATTRIB)
  { if (colval->expr)
    { t_expr_var varex(att->attrtype, att->attrspecif, 0, (char*)colval->dataptr-cdp->rscope->data);
      res |= execute_assignment(cdp, &varex, colval->expr, FALSE, TRUE);
    }
    else // write the default value
    { set_default_value(cdp, &val, att, tbdf);
      if (!IS_HEAP_TYPE(att->attrtype) && att->attrmult==1)
        memcpy(colval->dataptr, val.valptr(), simple_type_size(att->attrtype, att->attrspecif));
      val.set_null();
    }
    colval++;  att++;
  }
  return res;
}
#endif

BOOL sql_stmt_insert::insert_record(cdp_t cdp, table_descr * tbdf, tcursnum dcursnum)
{// insert the record to the table: 
  tabkont.position=iuts.execute_insert(cdp, tbdf, FALSE);
  if (tabkont.position==NORECNUM) return TRUE;
 // insert the new record to the cursor as well:
  if (qe!=NULL)
  { if (!dcd->pmat->put_recnums(cdp, &tabkont.position))
      { request_error(cdp, CANNOT_APPEND);  return TRUE; }
    trecnum crec = dcd->recnum++;
    dcd->curs_seek(crec);
   // registering the cursor insertion:
    ins_curs * ic = cdp->d_ins_curs.next();  
    if (ic!=NULL) { ic->cursnum=dcursnum;  ic->recnum=crec; }
  }
  return FALSE;
}

t_locdecl * find_name_in_run_scopes(cdp_t cdp, const char * name, t_loc_categ loccat)
// Finds the name in all active scopes.
{ for (t_rscope * rscope = cdp->rscope;  rscope;  rscope=rscope->next_rscope)
  { const t_scope * scope = rscope->sscope;
    t_locdecl * locdecl = scope->find_name(name, loccat);
    if (locdecl!=NULL)  // name found, return
      return locdecl;
  }
  return NULL;
}

BOOL sql_stmt_insert::exec(cdp_t cdp)
{ cur_descr *scd = NULL;  table_descr * tbdf = NULL;
  tcursnum dcursnum=NOCURSOR, scursnum=NOCURSOR;
  BOOL res=TRUE;  trecnum reccnt;
  dcd = NULL;
  PROFILE_AND_DEBUG(cdp, source_line);
  if (!cdp->trans_rw) return request_error(cdp, SQ_TRANS_STATE_RDONLY);  // may be handled
  cdp->exec_start();

 // find the destination table;
  if (qe!=NULL) // inserting into a cursor
  { dcursnum=create_cursor(cdp, qe, NULL, opt, &dcd);
    if (dcursnum==NOCURSOR) goto errex;
    unregister_cursor_creation(cdp, dcd);
    if (dcd->underlying_table==NULL || !PTRS_CURSOR(dcd))
      { request_error(cdp, CANNOT_APPEND);  goto errex; }
    if (!iuts.init_ttrs(cdp, NULL, dcd, TRUE))
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errex; }
    tbdf=tables[objnum];
    if (!tabkont.elems.count())  // used by BEFORE triggers
      tabkont.fill_from_td(cdp, tbdf);  // cursor opened -> tabdescr locked
  }
  else // inserting into a table
  { if (locdecl_offset!=-1) // local table, objnum may vary!
      objnum = *(ttablenum*)variable_ptr(cdp, locdecl_level, locdecl_offset);
    tbdf=install_table(cdp, objnum);
    if (tbdf==NULL) return TRUE;
    if (!iuts.init_ttrs(cdp, tbdf, NULL, TRUE))
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  unlock_tabdescr(tbdf);  goto errex; }
  }
  if (objnum==TAB_TABLENUM || objnum==OBJ_TABLENUM)  // this is not allowed, must use CREATE statement or cd_Isert_object
    { request_error(cdp, NO_RIGHT);  goto errex; }

 // check insertion privils:
  if (!cdp->has_full_access(tbdf->schema_uuid)) // unless admin mode
    if (!privil_test_result_cache.has_valid_positive_test_result(cdp)) // unless checked OK
      if (!privil_test_result_cache.has_valid_negative_test_result(cdp)) // unless checked FAIL
      { if (!can_insert_record(cdp, tbdf)) 
        { privil_test_result_cache.store_negative_test_result(cdp); // privils FAIL, put the result into the cache
          request_error(cdp, NO_RIGHT);  goto errex; 
        }
        else
          privil_test_result_cache.store_positive_test_result(cdp); // privils OK, put the result into the cache
      }
      else
        { request_error(cdp, NO_RIGHT);  goto errex; }

  if (iuts.pre_stmt_table(cdp, tbdf)) goto errex;  // BEF INS STM triggers
  if (src_qe!=NULL) // INSERT ... SELECT
  { scursnum=create_cursor(cdp, src_qe, NULL, src_opt, &scd);
    if (scursnum==NOCURSOR) goto errex;
    unregister_cursor_creation(cdp, scd);
    make_complete(scd);
   // try to lock (no error if cannot lock, will be slower):
   // disabling index is not a good idea, other users may be fatally slowed down!
    if (scd->recnum>1000) record_lock(cdp, tbdf, NORECNUM, TMPWLOCK);
   // preallocating space for records if insertinf big amount:
    if (scd->recnum>500)
    { trecnum count;
      tbdf->free_rec_cache.list_records(cdp, tbdf, NULL, &count);
      if (scd->recnum > count+100)
        tbdf->expand_table_blocks(cdp, tbdf->Recnum()+(scd->recnum - count));
    }  
   // passing the cursor records:
    for (trecnum r=0;  r<scd->recnum;  r++)
    { scd->curs_seek(r);
      cdp->statement_counter++;  // NEXT VALUE FOR seq should return a new value
      if (insert_record(cdp, tbdf, dcursnum) || cdp->except_type!=EXC_NO) goto errex;
      report_step(cdp, r);
    }
    reccnt=scd->recnum;
    report_total(cdp, reccnt);
  }
  else // insert single record: VALUES (...) or DEFAULT VALUES
  { if (insert_record(cdp, tbdf, dcursnum) || cdp->except_type!=EXC_NO) goto errex;
    reccnt=1;
  }
 // AFT STM triggers:
  if (iuts.post_stmt_table(cdp, tbdf)) goto errex;  // AFT INS STM triggers
  res=FALSE;  // OK result
  cdp->__rowcount=reccnt;
  cdp->add_sql_result(reccnt);

 errex:
  if (qe!=NULL)
  { if (dcursnum!=NOCURSOR) free_cursor(cdp, dcursnum);
  }
  else unlock_tabdescr(tbdf);
  if (src_qe!=NULL)
  { if (scursnum!=NOCURSOR) free_cursor(cdp, scursnum);
  }
  return res;
}
////////////////////////////////// BEGIN ... END block //////////////////////////////////
static table_descr *  make_temp_table_from_ta(cdp_t cdp, table_all * ta, const WBUUID schema_uuid, t_translog * translog)
// translog must be NULL when independent allocation of alpha data is needed!
{ table_descr * temp_table_descr = make_td_from_ta(cdp, ta, -1);
  if (!temp_table_descr) return NULL;
  temp_table_descr->deflock_counter=0;
  temp_table_descr->translog=translog;
  if (!create_temporary_table(cdp, temp_table_descr, translog)) 
    { temp_table_descr->free_descr(cdp);  return NULL; }
 // now, temp_table_descr is owned by tables[].
  memcpy(temp_table_descr->schema_uuid, schema_uuid, UUID_SIZE);  // null uuid for the TABTAB, used when compiling constrains
  if (compile_constrains(cdp, ta, temp_table_descr)) // constrains checked during transfer
    { destroy_temporary_table(cdp, temp_table_descr->tbnum);  return NULL; }
  return temp_table_descr;
}

BOOL sql_stmt_block::exec(cdp_t cdp)
{ int i;  BOOL res = FALSE;
  int sapo_num, sapo_ind;  // allocated savepoint number and index
 // create variables (must create scope even if no objects declared inside):
  t_rscope * rscope = new(scope.extent) t_rscope(&scope);
  if (rscope==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
  memset(rscope->data, 0, scope.extent);  // CLOB and BLOB vars must be init. (done by rscope_init() below)
  rscope->next_rscope=cdp->rscope;  cdp->rscope=rscope;
 // create savepoint if atomic:
  if (atomic)
  { sapo_num=0;  // allocate a new sapo number
    if (cdp->savepoints.create(cdp, NULLSTRING, sapo_num, TRUE)) 
      { res=TRUE;  goto ex; }
    sapo_ind=cdp->savepoints.find_sapo(NULLSTRING, sapo_num);
  }
 redo:
 // initialize local cursors as closed and LOBs as empty:
  rscope_init(rscope);
  rscope->level=scope_level;  // must be after rscope_init which sets level to 0
 // create local tables:
  bool initial_error;
  initial_error = false;
  for (i=0;  i<scope.locdecls.count();  i++)
  { t_locdecl * locdecl = scope.locdecls.acc0(i);
    if (locdecl->loccat==LC_TABLE)
    { t_translog_loctab * translog = new t_translog_loctab(cdp);
      if (!translog) goto ex;
      table_descr * temp_table_descr = make_temp_table_from_ta(cdp, locdecl->table.ta, cdp->current_schema_uuid, translog);
      if (!temp_table_descr) initial_error=true;
      else 
      { *(ttablenum*)(rscope->data+locdecl->table.offset) = temp_table_descr->tbnum;  // store the temp. table number in the rscope
        translog->tbdf=temp_table_descr;
      }
    }
  }
 // execute statements:
  if (!initial_error)
  { PROFILE_AND_DEBUG(cdp, source_line);
    exec_stmt_list(cdp, body);
    cdp->stop_leaving_for_name(label_name);
  }
 // close local cursors, destroy tables, release LOBs:
  for (i=0;  i<scope.locdecls.count();  i++)
  { t_locdecl * locdecl = scope.locdecls.acc0(i);
    if (locdecl->loccat==LC_CURSOR) // initialized in rscope_init
    { tcursnum * pcursnum = (tcursnum*)(rscope->data+locdecl->cursor.offset);
      if (*pcursnum != NOCURSOR) 
      { cur_descr * cd;
        { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
          cd=*crs.acc0(GET_CURSOR_NUM(*pcursnum));
        }
        t_temporary_current_cursor tcc(cdp, cd);
        free_cursor(cdp, *pcursnum);  *pcursnum=NOCURSOR;  locdecl->cursor.opt->close_data(cdp); 
      }
    }
    else if (locdecl->loccat==LC_VAR || locdecl->loccat==LC_CONST)
    { if (IS_HEAP_TYPE(locdecl->var.type))
      { t_value * val = (t_value*)(rscope->data+locdecl->var.offset);
        val->set_null();
      }
    }
    else if (locdecl->loccat==LC_TABLE)
    { ttablenum * ptabnum = (ttablenum*)(rscope->data+locdecl->table.offset);
      if (*ptabnum!=NOTBNUM)  // table created (tbnum is init to NOTBNUM in rscope_init)
      { destroy_temporary_table(cdp, *ptabnum);  // destroys the subtransaction, too
        *ptabnum=NOTBNUM;  // NOTBNUM says that the table does not exist any more 
      }
    }
  }
 //////// process exceptions: ///////////////////////////////
  if (cdp->except_type==EXC_EXC)
  {// look for handler in the current scope:
    t_locdecl * locdecl = get_noncont_condition_handler(cdp, cdp->rscope->sscope);
    if (locdecl!=NULL)  // execute the handler (it cannot be the continue handler)
    { if (locdecl->handler.rtype==HND_UNDO || locdecl->handler.rtype==HND_REDO)
      // roll back to the BEGIN, atomic valid (because handler found on the local scope)
        cdp->savepoints.rollback(cdp, sapo_ind);
      execute_handler(cdp, locdecl, cdp->rscope, NOOBJECT);  // another exception - processed on upper level!
      if (cdp->except_type==EXC_RESIGNAL) // on RESIGNAL going to the outer level
        cdp->except_type=EXC_EXC;  // do not continue searchig on this level
      else if (locdecl->handler.rtype==HND_REDO) goto redo;
#ifdef NEW_EXIT_HANDLING
      else if (cdp->except_type == EXC_HANDLED_RB)
        cdp->clear_exception();  // rollback stops here
#endif
    }
    // stays in the leaving mode if handler not found or RESIGNALled
  }
  else if (cdp->except_type == EXC_HANDLED_RB && atomic)
    cdp->clear_exception();  // rollback stops here, no redo because not handled here

 // BP check: variables still exist:
  PROFILE_AND_DEBUG(cdp, source_line2);
 // destroy the savepoint and all savepoints created inside iff atomic:
  if (atomic)
    cdp->savepoints.destroy(cdp, sapo_ind, TRUE);
 // destroy variables (must be done even on error!):
 ex:
  cdp->rscope=rscope->next_rscope;  delete rscope;
  return FALSE;
}

BOOL sql_stmt_if::exec(cdp_t cdp)
{ t_value res;
  PROFILE_AND_DEBUG(cdp, source_line);
  cdp->exec_start();  cdp->statement_counter++;
  stmt_expr_evaluate(cdp, condition, &res);
  if (cdp->except_name[0]) return TRUE;
  if (res.is_true())
    exec_stmt_list(cdp, then_stmt);
  else if (else_stmt!=NULL)
    exec_stmt_list(cdp, else_stmt);
  return FALSE;
}

BOOL sql_stmt_case::exec(cdp_t cdp)
{ t_value res;  sql_stmt_case * so;
  cdp->exec_start();
 // find the valid branch or go to no_branch:
  if (searched)  // searched CASE statement
  { so = this;
    while (so->cond!=NULL)
    { PROFILE_AND_DEBUG(cdp, source_line);
      cdp->statement_counter++;
      stmt_expr_evaluate(cdp, so->cond, &res);
      if (cdp->except_name[0]) return TRUE;
      if (res.is_true()) break;
      so=so->contin;
      if (so==NULL) goto no_branch;
    }
  }
  else  // simple case statement
  { t_value patt_res;
    PROFILE_AND_DEBUG(cdp, source_line);
    cdp->statement_counter++;
    stmt_expr_evaluate(cdp, cond, &patt_res);
    if (cdp->except_name[0]) return TRUE;
    so = contin;
    while (so->cond!=NULL)
    { stmt_expr_evaluate(cdp, so->cond, &res);
      if (cdp->except_name[0]) return TRUE;
      binary_oper(cdp, &res, &patt_res, eq_oper); // eq_oper is in the top node
      if (res.is_true()) break;
      so=so->contin;
      if (so==NULL) goto no_branch;
    }
  }
 // valid branch found (may be ELSE branch):
  exec_stmt_list(cdp, so->branch);
  return FALSE;

 no_branch:  // no branch is valid, completion condition
  return request_error(cdp, SQ_CASE_NOT_FOUND_STMT);  // may be handled
}

BOOL sql_stmt_loop::exec(cdp_t cdp)
// stops once on the line without cond and every time on the line with cond
{ bool eval=precond;
  if (!precond) PROFILE_AND_DEBUG(cdp, source_line);
  do
  { if (cdp->break_request)
      { request_error(cdp, REQUEST_BREAKED);  return TRUE; }
    if (eval)
    { PROFILE_AND_DEBUG(cdp, precond ? source_line : source_line2);
      cdp->exec_start();  cdp->statement_counter++;
      t_value res;
      stmt_expr_evaluate(cdp, condition, &res);
      if (cdp->except_name[0]) return TRUE;
      if (res.is_true() != precond) break;
    }
    exec_stmt_list(cdp, stmt);
    if (condition!=NULL) eval=true;
  } while (cdp->except_type==EXC_NO);
  cdp->stop_leaving_for_name(label_name);
  if (precond) PROFILE_AND_DEBUG(cdp, source_line2);
  return FALSE;
}

BOOL sql_stmt_leave::exec(cdp_t cdp)
// LEAVE extension to ISO: The label may be from a calling procedure. The label is searched dynamically. Exiting all scopes if the label is not found.
{ PROFILE_AND_DEBUG(cdp, source_line);
 // start "leaving" mode:
  cdp->except_type=EXC_LEAVE;  strcpy(cdp->except_name, label_name);
  return FALSE;
}

bool load_cursor_record_to_rscope(cdp_t cdp, cur_descr * cd, t_rscope * rscope)
{ cd->curs_seek(cd->position);
  t_locdecl * locdecl;  t_struct_type * row;
  locdecl = rscope->sscope->locdecls.acc0(0);  // the FOR variable
  row = locdecl->var.stype;
  for (int i=1;  i<cd->qe->kont.elems.count();  i++)
  { t_row_elem * rel = row->elems.acc0(i-1);
    t_expr_var  forvar(rel->type, rel->specif, rscope->level, locdecl->var.offset+rel->offset);
    t_expr_attr attr(i, &cd->qe->kont);
   // assign the value:
    if (execute_assignment(cdp, &forvar, &attr, FALSE, TRUE, TRUE)) return true;
  }
  return false;
}

void release_cursor_rscope(cdp_t cdp, cur_descr * cd, t_rscope * rscope)
{ t_locdecl * locdecl;  t_struct_type * row;
  locdecl = rscope->sscope->locdecls.acc0(0);  // the FOR variable
  row = locdecl->var.stype;
  for (int i=1;  i<cd->qe->kont.elems.count();  i++)
  { t_row_elem * rel = row->elems.acc0(i-1);
    if (IS_HEAP_TYPE(rel->type) || IS_EXTLOB_TYPE(rel->type))
    { t_value * val = (t_value*)variable_ptr(cdp, rscope->level, locdecl->var.offset+rel->offset);
      val->set_null();  
    }
  }
}

t_rscope * create_and_activate_rscope(cdp_t cdp, t_scope & scope, int scope_level)
{ t_rscope * rscope = new(scope.extent) t_rscope(&scope);
  if (rscope==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  memset(rscope->data, 0, scope.extent);  // LOBs must be init! (done by rscope_init() below)
  rscope_init(rscope);
  rscope->level=scope_level;  // must be after rscope_init which sets level to 0
  rscope->next_rscope=cdp->rscope;  cdp->rscope=rscope;
  return rscope;
}

void deactivate_and_drop_rscope(cdp_t cdp)
{ t_rscope * rscope = cdp->rscope;
  cdp->rscope=rscope->next_rscope;  
  delete rscope;
}

BOOL sql_stmt_for::exec(cdp_t cdp)
{ BOOL res=FALSE;  cur_descr * cd;  tcursnum cursnum;  
  PROFILE_AND_DEBUG(cdp, source_line);
  cdp->exec_start();
 // create the FOR variable:
  t_rscope * rscope = create_and_activate_rscope(cdp, scope, scope_level);
  if (!rscope) return TRUE;  // error reported
 // open cursor:
  cursnum=create_cursor(cdp, qe, NULL, opt, &cd);
  if (cursnum==NOCURSOR) { res=TRUE;  goto err1; }
  unregister_cursor_creation(cdp, cd);
  if (cursnum_offset)  // named cursor, store cursnum for CURRENT OF use
    *(tcursnum*)(rscope->data+cursnum_offset) = cursnum;
 // position it before the 1st row:
  if (*cursor_name) assign_name_to_cursor(cdp, cursor_name, cd);

  make_complete(cd);
 // cycle on records:
  for (cd->position=0;  cd->position<cd->recnum;  cd->position++)
  { if (cdp->break_request)
      { request_error(cdp, REQUEST_BREAKED);  res=TRUE;  goto err2; }
   // fetch and assign next row:
    if (load_cursor_record_to_rscope(cdp, cd, rscope)) { res=TRUE;  goto err2; }
   // execute statements:
    exec_stmt_list(cdp, body);
    PROFILE_AND_DEBUG(cdp, source_line2);  // is inside cycle for better debugging of sigle-line cycles
    if (cdp->except_type!=EXC_NO) break;
  }
  cdp->stop_leaving_for_name(label_name);

 err2:  
 // release LOB data in the FOR variable
  release_cursor_rscope(cdp, cd, rscope);
 // close cursor:
  { t_temporary_current_cursor tcc(cdp, cd);
    opt->close_data(cdp); // releases the temporary tables in the cursor
  }
  free_cursor(cdp, cursnum);

 err1:  // destroy the FOR variable scope (must be done even on error!):
  deactivate_and_drop_rscope(cdp);
  //if (cdp->dbginfo) cdp->dbginfo->check_break(source_line2);
  return res;
}

BOOL sql_stmt_signal::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  strcpy(cdp->except_name, name);
  cdp->except_type=EXC_EXC;
  strcpy(cdp->last_exception_raised, name);
  if (try_handling(cdp)) return FALSE;  // signal may be handled immediately
 // exception not handled by a continue handler:
  uns32 saved_flags = cdp->ker_flags;  cdp->ker_flags |= KFL_HIDE_ERROR;  // no reason to report, may be handled by EXIT handler 
  SET_ERR_MSG(cdp, cdp->except_name);  // exception may have been changed in the handler, do not use [name]!
  request_error_no_context(cdp, SQ_UNHANDLED_USER_EXCEPT);
  cdp->ker_flags = saved_flags;
  return TRUE;
}

BOOL sql_stmt_resignal::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  if (!cdp->processed_except_name[0])
    return request_error(cdp, SQ_RESIGNAL_HND_NOT_ACT);  // may be handled
  strcpy(cdp->except_name, name[0] ? name : cdp->processed_except_name);
  cdp->except_type=EXC_RESIGNAL;
  strcpy(cdp->last_exception_raised, cdp->except_name);
  if (try_handling(cdp)) return FALSE;  // signal may be handled immediately
 // exception not handled by a continue handler:
  uns32 saved_flags = cdp->ker_flags;  cdp->ker_flags |= KFL_HIDE_ERROR;  // no reason to report, may be handled by EXIT handler 
  if (cdp->except_name[0]!=SQLSTATE_EXCEPTION_MARK)
  { SET_ERR_MSG(cdp, cdp->except_name);
    request_error_no_context(cdp, SQ_UNHANDLED_USER_EXCEPT);
  }
  else
  { int error_code;
    if (sqlstate_to_wb_error(cdp->except_name+1, &error_code))
      request_error_no_context(cdp, error_code);
    else
      request_error_no_context(cdp, SQ_UNHANDLED_USER_EXCEPT);
  }
  cdp->ker_flags = saved_flags;
  return TRUE;
}

sql_stmt_set::sql_stmt_set(t_express * defval) : sql_statement(SQL_STAT_SET)
{ assgn.dest=NULL;
  assgn.source=defval;
  owning = defval==NULL;
}

BOOL sql_stmt_set::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  cdp->exec_start();
  if (execute_assignment(cdp, assgn.dest, assgn.source, TRUE, TRUE, FALSE)) return TRUE;
  return FALSE;
}

sql_stmt_return::sql_stmt_return(void) : sql_stmt_set(NULL)
{ 
}

BOOL sql_stmt_return::exec(cdp_t cdp)
{ //if (cdp->dbginfo) cdp->dbginfo->check_break(source_line); -- will be tested in sql_stmt_set::exec
  if (sql_stmt_set::exec(cdp)) return TRUE;
 // set the flag that RETURN has been executed
  t_expr_var * varex = (t_expr_var*)assgn.dest;  // return dest is a var
  t_rscope * rscope = get_rscope(cdp, varex->level);
  rscope->data[0]=wbtrue;
#if WBVERS>=900  // feature must be disabled in older versions, the default state would change the default behaviour of the server
 // stop the execution, if allowed by the options:
  if (!(cdp->sqloptions & SQLOPT_REPEATABLE_RETURN))
    { cdp->except_type=EXC_LEAVE;  strcpy(cdp->except_name, function_name); }
#endif
  return FALSE;
}

int simple_type_size(int type, t_specif specif)
// Space for the local variable of the specified type
{ return type==ATT_BINARY   ? specif.length :
         IS_STRING(type)    ? (specif.wide_char ? specif.length+2 : specif.length+1) :
         type==ATT_CHAR     ? (specif.wide_char ? 4 : 2) : // must have space for the terminator
         IS_HEAP_TYPE(type) || IS_EXTLOB_TYPE(type) ? sizeof(t_value) : tpsize[type];
}

//////////////////////////////////// detached routines ///////////////////////////////
/* The detached routine may be internal or external, but must not be standard and must not
   be internal and local at the same time.
*/

struct t_detached_param // parameters passed to the detached thread
{ t_routine * croutine;  // owning iff not local
  t_rscope * rscope;
  tobjnum user_objnum;
  BOOL admin_mode_routine;
};

static THREAD_PROC(detached_proc_thread)
{
#ifdef NETWARE
  RenameThread(GetThreadID(), "Detached procedure thread");
#endif
 // copy *dp to mydp and release it, ownership passed to mydp:
  t_detached_param mydp;
  t_detached_param * dp = (t_detached_param*)data;
  mydp=*dp;
  corefree(dp);
 // attach the thread to the server:
  cd_t cd(PT_WORKER);  cdp_t cdp = &cd;
  int code = interf_init(cdp);
  if (code == KSE_OK)
  { if (!mydp.croutine->is_local())  // unless local routine
      cdp->detached_routine = mydp.croutine;
    cdp->is_detached=true;
    detachedThreadCounter++;
   // define user name 
    cdp->prvs.set_user(mydp.user_objnum, CATEG_USER, TRUE); 
    cdp->random_login_key=GetTickCount();
   // set application kontext:
    tobjname schema_name;  tobjnum aplobj;
    ker_apl_id2name(cdp, mydp.croutine->schema_uuid, schema_name, NULL);  
    ker_set_application(cdp, schema_name, aplobj);
   // execute the routine:
    mydp.rscope->level=0;
    cdp->rscope=mydp.rscope;  // parameters
    cdp->in_routine++;
    if (mydp.admin_mode_routine) cdp->in_admin_mode++;
    mydp.croutine->stmt->exec(cdp);
    process_rollback_conditions_by_cont_handler(cdp);
    if (mydp.admin_mode_routine) cdp->in_admin_mode--;
    cdp->in_routine--;

    commit(cdp); // commits the changes made by the detached procedure
   // deleting the routine should be done before interf_close, if possible:
    delete mydp.rscope;  
    if (!mydp.croutine->is_local())  // unless local routine
    { if (cdp->procedure_created_cursor)
      { dbg_err("Detached procedure opened a cursor");
        delete mydp.croutine;
      }
      else psm_cache.put(mydp.croutine);
    }
    kernel_cdp_free(cdp);
    cd_interf_close(cdp);
    detachedThreadCounter--;
  }
  else // slightly unsafe, because GetCurrTaskPtr() returns NULL
  { client_start_error(cdp, detachedThreadStart, code);
    delete mydp.rscope;  
    if (!mydp.croutine->is_local())  // unless local routine
      psm_cache.put(mydp.croutine);
  }
  integrity_check_planning.thread_operation_stopped();
  THREAD_RETURN;
}

static BOOL run_detached(cdp_t cdp, t_routine * croutine, t_rscope * rscope, BOOL admin_mode_routine)  
// The croutine is not local and internal at the same time.
// If the routine is not local, croutine is an owning pointer
// If returns TRUE then the ownership of rscope and croutine (unless local) is taken by the new thread (or relased here)
// If returns FALSE then rscope and croutine (unless local) have to be released outside.
{ BOOL ok;
  t_detached_param * dp = (t_detached_param*)corealloc(sizeof(t_detached_param), 89);
  if (dp==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  ok=FALSE; }
  else
  { dp->user_objnum=cdp->prvs.luser();
    dp->rscope=rscope;  dp->croutine=croutine;  dp->admin_mode_routine=admin_mode_routine;
    if (integrity_check_planning.starting_client_request())  // registers the new running request, cannot return FALSE because the calling request is running
    { if (THREAD_START(detached_proc_thread, 0, dp, NULL)) // thread handle closed inside
        return TRUE; // started OK, releasing will be done in detached_proc_thread
     // thread not started, must release its objects:
      if (request_error(cdp, THREAD_START_ERROR))
        ok=FALSE;
      else // error handled, so must release the objects that would be released by the routine if it would be started
      { delete dp->rscope;  
        if (!dp->croutine->is_local())  // unless local routine
          psm_cache.put(dp->croutine);
        ok=TRUE;  // error handled
      }
      integrity_check_planning.thread_operation_stopped();
    }
    else ok=FALSE;
    corefree(dp);  
  }
  return ok; 
}

t_routine * sql_stmt_call::get_called_routine(cdp_t cdp, t_zcr * required_version, BOOL complete)
{ if (calling_local_routine())  // local or extension routine
    return called_local_routine;
  else  // shared routine, take it from the cache
    return executed_global_routine=get_stored_routine(cdp, objnum, required_version, complete);
}

void sql_stmt_call::release_called_routine(t_routine * rout)
{ if (!calling_local_routine()) 
  { psm_cache.put(rout);
    executed_global_routine=NULL;
  }  
}

void write_result(void * destptr, void * retptr, int retvalsize, t_routine * routine)
// __try must be in a separate function
{
#ifdef WINS
  __try
  {
#endif
    memcpy(destptr, retptr, retvalsize);  // retptr is NULL on external exception
#ifdef WINS
  }
  __except ((GetExceptionCode() != 0) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
  { char buf[100];
    sprintf(buf, ">> Exception %X occured in %s.", GetExceptionCode(), routine->names);
    dbg_err(buf);
  }
#endif
}

BOOL sql_stmt_call::exec(cdp_t cdp)
{ BOOL res=TRUE;  int i;
  prof_time start, saved_lower_level_time, orig_lock_waiting_time;
  t_value * resptr = result_ptr;  result_ptr=NULL;  // result_ptr must be set NULL before the next call (which may not define it)
  PROFILE_AND_DEBUG(cdp, source_line);
  if (cdp->in_routine > the_sp.max_routine_stack.val())
  { tobjname err_param;
    sprintf(err_param, "MaxRoutineStack=%d", the_sp.max_routine_stack.val());
    SET_ERR_MSG(cdp, err_param);
    return request_error(cdp, LIMIT_EXCEEDED);
  }
  cdp->exec_start();
  if (std_routine!=NULL)  
  { t_value resval;
    expr_evaluate(cdp, std_routine, &resval);
    if (cdp->is_an_error()) return TRUE;
    return FALSE;
  }
 // get the called routine:
  t_routine * routine = get_called_routine(cdp, &callee_version, TRUE);
  if (routine==NULL) return TRUE;
  bool must_drop_the_procedure_code = false;
 // trace:
  if (cdp->trace_enabled & TRACE_PROCEDURE_CALL)
  { char buf[100+OBJ_NAME_LEN]; 
    if (routine->is_internal())
      form_message(buf, sizeof(buf), msg_trace_proc_call, routine->name);
    else if (routine->names)
      form_message(buf, sizeof(buf), msg_trace_dll_call, routine->names, routine->names+strlen(routine->names)+1);
	else strcpy(buf, "<extension>");
    trace_msg_general(cdp, TRACE_PROCEDURE_CALL, buf, NOOBJECT);
  }
#ifdef LINUX
  if (!routine->is_internal())
    if (geteuid()==0) // If we run as root, no functions allowed
      { res=request_error(cdp, WONT_RUN_AS_ROOT);  goto errex0; }
#endif
 // check EXECUTE privilege:
  if (!calling_local_routine())
  { if (!cdp->has_full_access(routine->schema_uuid) && routine->is_internal()) // unless calling external or local routine 
      if (!privil_test_result_cache.has_valid_positive_test_result(cdp)) // unless checked
      { if (!can_read_objdef(cdp, objtab_descr, objnum))
          { request_error(cdp, NO_RIGHT);  goto errex0; }
        privil_test_result_cache.store_positive_test_result(cdp); // privils OK, put the result into the cache
      }
    saved_lower_level_time = cdp->lower_level_time;  cdp->lower_level_time=0;
    orig_lock_waiting_time = cdp->lock_waiting_time;
    start = get_prof_time();  // important when the procedure starts profiling!
  }
 // create parameters:
  const t_scope * pscope;  pscope = routine->param_scope();
  t_rscope * rscope;  rscope = new(pscope->extent) t_rscope(pscope);
  if (rscope==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errex0; }
  memset(rscope->data, 0, pscope->extent);  // historically all parameter values are 0
  rscope_init(rscope);  // CLOB and BLOB vars must be init
 // prepare returning a value:
  char valbuf[8];          // result space for external functions (enough for sig64 and double)
  void * retptr;           // external function returns its value here
  t_locdecl * retlocdecl;  // space for return value of internal function
  if (routine->rettype)
    rscope->data[0]=wbfalse;  // return_not_performed_yet flag
 // activate the parameter scope, for parameter assignment only:
  rscope->level=-1;  // must be after rscope_init which sets level to 0
  rscope->next_rscope=cdp->rscope;  cdp->rscope=rscope;
 // assign input parameter values or default values:
  for (i=0;  i<pscope->locdecls.count();  i++)
  { t_locdecl * locdecl = pscope->locdecls.acc0(i);
    if (routine->extension_routine)
      if (!i)
      { *(cdp_t*)rscope->data=cdp;
        continue;
      }
    if (locdecl->loccat==LC_INPAR || locdecl->loccat==LC_INPAR_ || locdecl->loccat==LC_INOUTPAR || locdecl->loccat==LC_FLEX)
    { t_express * act;
      if (i<actpars.count()) act = *actpars.acc0(i);
      else act=NULL;
      if (act==NULL) // actual parameter not specified, assign the default value
        act=locdecl->var.defval;
      t_expr_var parvar(locdecl->var.type, locdecl->var.specif, -1, locdecl->var.offset);
      if (execute_assignment(cdp, &parvar, act, FALSE, TRUE, FALSE)) goto errex;
    }
    else if (locdecl->loccat==LC_OUTPAR || locdecl->loccat==LC_OUTPAR_)
    { if (IS_HEAP_TYPE(locdecl->var.type))
      { t_value * val = (t_value*)(rscope->data+locdecl->var.offset);
        val->set_null();
      }
    }
    else if (locdecl->loccat==LC_VAR) // return value will be here (internal only)
      retlocdecl=locdecl;
  }
 // complete the profile of the CALL line before entering the new scope (before changing top_objnum):
  CHANGING_SCOPE(cdp);
 // execute statements:
  res=FALSE;
  if (detached) // execute detached
  { cdp->rscope=rscope->next_rscope;  rscope->next_rscope=NULL;  // detach the rscope
    if (run_detached(cdp, routine, rscope, !calling_local_routine() && is_in_admin_mode(cdp, objnum, routine->schema_uuid))) 
    { executed_global_routine=NULL;
      return FALSE;  // OK, rscope and (possibly) routine will be released by the detached thread
    }  
   // error staring the detached routine:
    res=TRUE;
    delete rscope;
    goto errex;   // detached routine not started, must release it (ATTN: it is already detached)
  }

 // procedure not detached:
  if (routine->is_internal())
  {// update debugger info:
    rscope->level=routine->scope_level;
    cdp->in_routine++;
    if (!calling_local_routine()) // unless calling local routine with the same objnum
      cdp->top_objnum = objnum;
    if (cdp->stk) cdp->stk->line = source_line;  // may not have been set in PROFILE_AND_DEBUG if debugging is off
    cdp->stk_add(cdp->top_objnum, routine->name);
   // prepare checking for cursors opened by the procedure:
    bool upper_procedure_created_cursor = cdp->procedure_created_cursor;
    cdp->procedure_created_cursor = false;
    t_exkont_proc ekt(cdp, rscope, routine);
   // when calling routine from a remote schema, must temporarily clear the external in_admin_mode:
    int orig_admin_mode = cdp->in_admin_mode;
    if (memcmp(routine->schema_uuid, cdp->current_schema_uuid, UUID_SIZE))
      cdp->in_admin_mode=0;
    { t_short_term_schema_context stsc(cdp, routine->schema_uuid);
     // execute:
      BOOL admin_mode_routine = calling_local_routine() ? FALSE : is_in_admin_mode(cdp, objnum, routine->schema_uuid);
      if (admin_mode_routine) cdp->in_admin_mode++;
      res=routine->stmt->exec(cdp);
      process_rollback_conditions_by_cont_handler(cdp);
      if (admin_mode_routine) cdp->in_admin_mode--;
    }
    cdp->in_admin_mode = orig_admin_mode; // restoring
   // check if procedure created a cursor:
    if (cdp->procedure_created_cursor)
      if (!calling_local_routine())
      { must_drop_the_procedure_code=true;
        cdp->procedure_created_cursor = upper_procedure_created_cursor;
      }
      else
        cdp->procedure_created_cursor |= upper_procedure_created_cursor;  // must drop the non-local procedure
   // complete the profile of the RETURN line before changing scopes (before changing top_objnum):
    CHANGING_SCOPE(cdp);
   // update debugger info:
    cdp->stk_drop();
    cdp->in_routine--;
    if (res) goto errex; 
    rscope->level=-1;
    cdp->stop_leaving_for_name(routine->name);
  }
  else // external
  { 
    FARPROC procptr=routine->proc_ptr(cdp);  
    if (procptr==(FARPROC)1)
      { res=TRUE;  goto errex; }  // returns TRUE, error not handled
    if (procptr==NULL)
      { res=FALSE;  goto errex; }  // returns FALSE, error handled
    cdp->rscope=rscope->next_rscope;  // temp. removing the param scope: opens access for local variables from DADs
    retptr=call_external_routine(cdp, routine, procptr, pscope, rscope, valbuf);
    cdp->rscope=rscope;               // reinstalling the param scope for removing the parameters
    routine->free_proc_ptr();  // necessary on Novell Netware
  }

 // assign output parameter values:
  for (i=0;  i<pscope->locdecls.count();  i++)
  { t_locdecl * locdecl = pscope->locdecls.acc0(i);
    if (locdecl->loccat==LC_OUTPAR || locdecl->loccat==LC_OUTPAR_ || locdecl->loccat==LC_INOUTPAR || locdecl->loccat==LC_FLEX)
    { t_express * act;
      if (i<actpars.count()) act = *actpars.acc0(i);
      else act=NULL;
      if (act==NULL) continue; // actual parameter not specified
      t_expr_var parvar(locdecl->var.type, locdecl->var.specif, -1, locdecl->var.offset);
      if (execute_assignment(cdp, act, &parvar, TRUE, FALSE, FALSE)) goto errex;
    }
  }
 // assign return value (if function && result space provided):
  if (routine->rettype)  // function, not a procedure
  { if (resptr!=NULL)  // unless function called as a procedure
    { if (routine->is_internal())
      { if (!rscope->data[0])  // return_not_performed_yet flag
        { resptr->set_null();  
          if (!cdp->is_an_error())  // do not overwite other errors by this error
            if (request_error(cdp, SQ_NO_RETURN_IN_FNC))
              { res=TRUE;  goto errex; }
        }
        else  // return value assigned
        { t_expr_var retvar(retlocdecl->var.type, retlocdecl->var.specif, -1, retlocdecl->var.offset);
          stmt_expr_evaluate(cdp, &retvar, resptr);  // copies the return value
          if (IS_HEAP_TYPE(retlocdecl->var.type))
          { t_value * val = (t_value*)(rscope->data+retlocdecl->var.offset);
            val->set_null();
          }
        }
      }
      else // external, return type not heap
      { int type = routine->rettype;
        int retvalsize = simple_type_size(type, routine->retspecif);
        resptr->set_simple_not_null();
        if (resptr->allocate(retvalsize)) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errex; }
        if (retptr) 
        { tptr destptr = resptr->valptr();
          write_result(destptr, retptr, retvalsize, routine);
         // classification (like after loading database or variable value):
          Fil2Mem_convert(resptr, destptr, type, routine->retspecif);
        }  
      }
    }
  }
 errex:
 // release long values in parameters and return value (must be after assigning the return value):
  for (i=0;  i<pscope->locdecls.count();  i++)
  { t_locdecl * locdecl = pscope->locdecls.acc0(i);
    if (locdecl->loccat>LC_VAR || locdecl->loccat<=LC_OUTPAR_)
    { if (IS_HEAP_TYPE(locdecl->var.type))
      { t_value * val = (t_value*)(rscope->data+locdecl->var.offset);
        val->set_null();
      }
    }
  }
 // destroy variables (must be done even on error!):
  if (cdp->rscope==rscope) cdp->rscope=rscope->next_rscope;  // nothing to be done for detached routine start error
  delete rscope;
 errex0:
  if (!calling_local_routine())
  { prof_time interv = get_prof_time()-start;
    if (PROFILING_ON(cdp)) add_hit_and_time2(interv, cdp->lower_level_time, cdp->lock_waiting_time-orig_lock_waiting_time, PROFCLASS_ROUTINE, objnum, 0, NULL);
    cdp->lower_level_time = saved_lower_level_time + interv;
  }
 // release or destroy the called routine:
  executed_global_routine=NULL;
  if (must_drop_the_procedure_code) delete routine;
  else release_called_routine(routine);
  return res;
}

void t_exkont_proc::get_descr(cdp_t cdp, char * buf, int buflen)
{ char mybuf[2*OBJ_NAME_LEN+60];  
  strcpy(mybuf, " [Procedure ");
  //if (*routine->schema) { strcat(mybuf, routine->schema);  strcat(mybuf, "."); } -- schema name not stored any more
  strcat(mybuf, routine->name);  
  if (cdp->top_objnum == routine->objnum && cdp->last_line_num!=(uns32)-1)
    sprintf(mybuf+strlen(mybuf), " line %u", cdp->last_line_num);
  strcat(mybuf, "]");
  if (buflen>0) strmaxcpy(buf, mybuf, buflen);
}

void t_exkont_proc::get_info(cdp_t cdp, t_exkont_info * info)
{ t_exkont::get_info(cdp, info);  
  info->par1 = routine->objnum;  
  info->par2 = cdp->top_objnum == routine->objnum ? cdp->last_line_num : (uns32)-1;
} 

BOOL disable_system_triggers = FALSE;

BOOL find_system_procedure(cdp_t cdp, const char * name)
{ tobjnum proc_obj;  tobjname aplname;
  if (disable_system_triggers) return FALSE;
  strcpy(aplname, "_SYSEXT");  // error in Upcase() if string constant passed to name_find2_obj on Linux
  return !name_find2_obj(cdp, name, aplname, CATEG_PROC, &proc_obj);
}

///////////////////////////////////////////////////////////////////////////////////////////
BOOL sql_stmt_select::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  cdp->exec_start();
  { t_fictive_current_cursor fcc(cdp);
    opt->close_data(cdp);  // cursor may have been still open from the previous execution of the statement
  }
  cur_descr * cd;
  tcursnum cursnum=create_cursor(cdp, qe, source, opt, &cd);
  if (cursnum==NOCURSOR) return TRUE;  // cursor openning error
  BOOL res=FALSE;
  if (is_into)  // says: qe->qe_type==QE_SPECIF && ((t_qe_specif*)qe)->into.count();
  { make_complete(cd);
    // cdp->sql_result = cdp->__rowcount = cd->recnum; -- neni k tomu duvod
    if (cd->recnum > 1)
      res=request_error(cdp, SQ_CARDINALITY_VIOLATION);
    else   // if (!res)  -- changed, originally the value assigned in the CONTINUE handler was overwritten by the 1st value from the cursor
    { t_temporary_current_cursor tcc(cdp, cd);  // sets the temporary current_cursor in the cdp until destructed
      if (!cd->curs_eseek(0))
        ((t_qe_specif*)qe)->assign_into(cdp);  // must not refer to cd->qe, which may been QE_IDENT
      else // NO DATA CC: no assignment
        res=request_error(cdp, SQ_CARDINALITY_VIOLATION);
    }
    { t_temporary_current_cursor tcc(cdp, cd);
      opt->close_data(cdp); // releases the temporary tables in the cursor
    }
    free_cursor(cdp, cursnum);  cursnum=NOCURSOR;
  }
  else // zde nenastavuji @@ROWCOUNT, asi neni duvod
  { cdp->add_sql_result(0xffff0000LU | cursnum);
    if (embvar_referenced && (!cdp->sel_stmt || cdp->sel_stmt->hstmt==0) ||  
    // when query contains embedded variables, complete only if it is not a prepared statement
        //dynpar_referenced ||  // contains dynamic parameters which will be freed on exit -- not necessary, makes it slow
        cdp->in_routine)      // may reference routine variables or parameters
      make_complete(cd);
    cdp->procedure_created_cursor = true;  // this causes droping the procedure code instead of returning it into the cache
  }
  return res;
}

/////////////////////////// object creation /////////////////////////////////
BOOL schema_uuid_for_new_object(cdp_t cdp, const char * specified_schema_name, WBUUID uuid)
// Finds UUID of the (explicit or implicit) schema the created object will belong to
// Error may be handled.
{ if (specified_schema_name && *specified_schema_name) // explicit schema name specified
  { if (ker_apl_name2id(cdp, specified_schema_name, uuid, NULL)) 
      { SET_ERR_MSG(cdp, specified_schema_name);  request_error(cdp, OBJECT_DOES_NOT_EXIST);  return FALSE; }
  }
  else // name of the created object is not prefixed: use the current (short-term) schema
  { if (null_uuid(cdp->current_schema_uuid) && cdp!=cds[0]) // current schema not defined
      // ... error unless called by the system thread
      { SET_ERR_MSG(cdp, "<schema name>");  request_error(cdp, OBJECT_DOES_NOT_EXIST);  return FALSE; }
    memcpy(uuid, cdp->current_schema_uuid, UUID_SIZE);  // copying the current schema uuid
  }
  return TRUE;
}

static void set_role_privils(cdp_t cdp, trecnum trec, tcateg categ, const WBUUID schema_uuid, BOOL limited)
// Assigns role privileges for a new object
{ privils prvs(cdp);  tobjnum roleobj;
  BOOL in_current_schema = !memcmp(schema_uuid, cdp->top_appl_uuid, UUID_SIZE);
  table_descr * object_table_descr = (categ & CATEG_MASK) == CATEG_TABLE ? tabtab_descr : objtab_descr;
  t_privils_flex priv_val(object_table_descr->attrcnt), priv_val_tab;
  table_descr * tbdf;
  if (categ==CATEG_TABLE) // used when importing data during the import of application
  { tbdf = install_table(cdp, trec);
    priv_val_tab.init(tbdf->attrcnt);
  }
 // author: all privileges to the definition and to table data
  if (in_current_schema) roleobj=cdp->author_role;
  else roleobj=find2_object(cdp, CATEG_ROLE, schema_uuid, "AUTHOR");
  if (roleobj != NOOBJECT)
  { prvs.set_user(roleobj, CATEG_ROLE, FALSE);
    *priv_val.the_map()=RIGHT_DEL | RIGHT_GRANT | RIGHT_READ | RIGHT_WRITE;
    priv_val.set_all_rw();
    prvs.set_own_privils(object_table_descr, trec, priv_val);
    if (categ==CATEG_TABLE) // privileges used when importing data during the import of application
    { *priv_val_tab.the_map() = RIGHT_DEL | RIGHT_INSERT | RIGHT_GRANT | RIGHT_READ | RIGHT_WRITE;
      priv_val_tab.set_all_rw();
      prvs.set_own_privils(tbdf, NORECNUM, priv_val_tab);
    }
  }
  if (limited) // remove privils of the creator
  { priv_val.clear();
    cdp->prvs.set_own_privils(object_table_descr, trec, priv_val);
  }
 // define privils of the other standard roles:
 // for "limited" they probable will be overwriten, bud the import may not define role privileges!
  {// admin: usage privilege to the definition and all to table data
    if (in_current_schema) roleobj=cdp->admin_role;
    else roleobj=find2_object(cdp, CATEG_ROLE, schema_uuid, "ADMINISTRATOR");
    if (roleobj != NOOBJECT)
    { prvs.set_user(roleobj, CATEG_ROLE, FALSE);
      *priv_val.the_map()=RIGHT_GRANT | RIGHT_READ;
      priv_val.set_all_r();
      prvs.set_own_privils(object_table_descr, trec, priv_val);
      if (categ==CATEG_TABLE)
      { *priv_val_tab.the_map() = RIGHT_DEL | RIGHT_INSERT | RIGHT_GRANT | RIGHT_READ | RIGHT_WRITE;
        priv_val_tab.set_all_rw();
        prvs.set_own_privils(tbdf, NORECNUM, priv_val_tab);
      }
    }
   // senior user: read privilege to the definition, all to data
    if (in_current_schema) roleobj=cdp->senior_role;
    else roleobj=find2_object(cdp, CATEG_ROLE, schema_uuid, "SENIOR_USER");
    if (roleobj != NOOBJECT)
    { prvs.set_user(roleobj, CATEG_ROLE, FALSE);
      *priv_val.the_map()=RIGHT_READ;
      priv_val.set_all_r();
      prvs.set_own_privils(object_table_descr, trec, priv_val);
      if (categ==CATEG_TABLE)
      { *priv_val_tab.the_map()=RIGHT_DEL | RIGHT_INSERT | RIGHT_READ | RIGHT_WRITE;
        priv_val_tab.set_all_rw();
        prvs.set_own_privils(tbdf, NORECNUM, priv_val_tab);
      }
    }
   // junior user: read privilege to the definition and to data
    if (in_current_schema) roleobj=cdp->junior_role;
    else roleobj=find2_object(cdp, CATEG_ROLE, schema_uuid, "JUNIOR_USER");
    if (roleobj != NOOBJECT)
    { prvs.set_user(roleobj, CATEG_ROLE, FALSE);
      *priv_val.the_map()=RIGHT_READ;
      priv_val.set_all_r();
      prvs.set_own_privils(object_table_descr, trec, priv_val);
      if (categ==CATEG_TABLE)
      { *priv_val_tab.the_map()=RIGHT_READ;
        priv_val_tab.set_all_r();
        prvs.set_own_privils(tbdf, NORECNUM, priv_val_tab);
      }
    }
  }
  if (categ==CATEG_TABLE) 
    unlock_tabdescr(tbdf);
}

trecnum create_object(cdp_t cdp, const tobjname name, const WBUUID schema_uuid, const char * source, tcateg categ, BOOL limited)
{ table_descr * ins_tabdescr = (categ & CATEG_MASK)==CATEG_TABLE ? tabtab_descr : objtab_descr;
 // check insert privils:
  if (!can_insert_record(cdp, ins_tabdescr))
    { request_error(cdp, NO_RIGHT);  return NORECNUM; }
 // disable creating a trigger in locked application:
  if (categ==CATEG_TRIGGER ||  // test for Insert_object and CREATE TRIGGER, for Write and ALTER TRIGGER protected by privileges
      !memcmp(schema_uuid, sysext_schema_uuid, UUID_SIZE) ||  // test for creating object in the _sysext schema
      categ==CATEG_PROC && !strcmp(name, MODULE_GLOBAL_DECLS))  // test for creating the MODULE_GLOBALS
  { //tobjnum aplobj = find_object_by_id(cdp, CATEG_APPL, schema_uuid);
    //apx_header apx;  memset(&apx, 0, sizeof(apx));
    //tb_read_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, 0, sizeof(apx_header), (tptr)&apx);
    //if (apx.appl_locked)
    if (!cdp->prvs.am_I_appl_author(cdp, schema_uuid))
      { request_error(cdp, NO_RIGHT);  return NORECNUM; }
  }
 // vectorised insert:
  uns32 len;
  uns32 stamp = stamp_now();
  t_colval colvals[5+1] = {
   { OBJ_NAME_ATR,  NULL, name,        NULL, NULL, 0, NULL },
   { OBJ_CATEG_ATR, NULL, &categ,      NULL, NULL, 0, NULL },
   { APPL_ID_ATR,   NULL, schema_uuid, NULL, NULL, 0, NULL },
   { OBJ_DEF_ATR,   NULL, source,      &len, NULL, 0, NULL },
   { OBJ_MODIF_ATR, NULL, &stamp,      NULL, NULL, 0, NULL },
   { NOATTRIB,      NULL, NULL,        NULL, NULL, 0, NULL } };
  t_vector_descr vector(colvals, NULL);
  len=source ? (int)strlen(source) : 0;  // empty definition for a role, application etc.
  trecnum trec=tb_new(cdp, ins_tabdescr, FALSE, &vector);
  if (trec!=NORECNUM) 
    if (categ!=CATEG_TABLE && categ!=CATEG_ROLE && categ!=CATEG_APPL) // table not completely created yet
      set_role_privils(cdp, trec, categ, schema_uuid, limited);
  return trec;
}

bool create_schema(cdp_t cdp, const char * name, BOOL limited, tobjnum * objnum, bool conditional)
{// must explicitly check for schema name duplicity, unique index will not ensure this
  if (find2_object(cdp, CATEG_APPL, NULL, name)!=NOOBJECT)
    if (conditional) return false;
    else { SET_ERR_MSG(cdp, "OBJTAB");  request_error(cdp, KEY_DUPLICITY);  return true; }
 // create UUID and schema object:
  WBUUID uuid;  safe_create_uuid(uuid);
  *objnum=create_object(cdp, name, uuid, NULL, CATEG_APPL, limited);
  if (*objnum==NORECNUM) return true;
 // create standard roles:
  tobjnum admin_role, junior_role;
  admin_role =create_object(cdp, "ADMINISTRATOR", uuid, NULL, CATEG_ROLE, limited);
              create_object(cdp, "SENIOR_USER",   uuid, NULL, CATEG_ROLE, limited);
  junior_role=create_object(cdp, "JUNIOR_USER",   uuid, NULL, CATEG_ROLE, limited);
 // creator is assigned the admin role:
  uns8 state;  state=wbtrue;
  if (admin_role!=NORECNUM) 
  { user_and_group(cdp, cdp->prvs.luser(), admin_role, CATEG_ROLE, OPER_SET_ON_CREATE, &state);
    cdp->prvs.set_user(cdp->prvs.luser(), CATEG_USER, TRUE);  // re-create the hierarchy: information about the admin role is added into it
  }
 // EVERYBODY group is assigned the junior role:
  if (junior_role!=NORECNUM) // now I am the Administrator and I can use the OPER_SET
    user_and_group(cdp, EVERYBODY_GROUP, junior_role, CATEG_ROLE, OPER_SET, &state);
 // the author role is created and creator is assigned into it:
  if (!limited)
  { tobjnum author_role = create_object(cdp, "AUTHOR", uuid, NULL, CATEG_ROLE, limited);
    if (author_role!=NORECNUM)
      user_and_group(cdp, cdp->prvs.luser(), author_role, CATEG_ROLE, OPER_SET, &state);
  }
  return false;
}


BOOL create_application_object(cdp_t cdp, const char * name, tcateg categ, BOOL limited, tobjnum * objnum)
// Used when creating object through API
{ tobjname upname;
  strcpy(upname, name);  sys_Upcase(upname);
  if (categ==CATEG_USER || categ==CATEG_GROUP)  // user or group
  { WBUUID uuid;  safe_create_uuid(uuid);
    *objnum = new_user_or_group(cdp, upname, categ, uuid);  // privileges checked inside
    if (*objnum==NOOBJECT) return TRUE;
    set_new_password2(cdp, *objnum, NULLSTRING, TRUE);  // expired empty password
    return FALSE;
  }
  else if (categ==CATEG_APPL)    // create a new UUID
    return create_schema(cdp, upname, limited, objnum, false);
  else // creating a normal object
  { tcateg cat = categ & CATEG_MASK;
   // find the uuid for the object:
    uns8 * pid = cat==CATEG_ROLE ? cdp->top_appl_uuid :
      cat==CATEG_TABLE || cat==CATEG_CURSOR || cat==CATEG_TRIGGER || cat==CATEG_PROC || cat==CATEG_SEQ || cat==CATEG_REPLREL ?
      cdp->sel_appl_uuid : cdp->front_end_uuid;
    *objnum=create_object(cdp, upname, pid, NULL, categ, limited);
    return *objnum==NORECNUM;
  }
  return FALSE;
}

static bool alter_object(cdp_t cdp, table_descr * tbdf, tobjnum objnum, tptr source, tcateg categ)
// Stores a new definition [source] of the [objnum] object in [tbdf].
// Returns false on error.
{// check privileges:
  if (!can_write_objdef(cdp, tbdf, objnum)) 
    if (categ==CATEG_SEQ && cdp->in_admin_mode)
    { WBUUID schema_uuid;
      tb_read_atr(cdp, tbdf, objnum, APPL_ID_ATR, (tptr)schema_uuid);
      if (memcmp(schema_uuid, cdp->current_schema_uuid, UUID_SIZE))
        { request_error(cdp, NO_RIGHT);  return false; }
    }
    else
    { request_error(cdp, NO_RIGHT);  return false; }
 // encrypt the source, if necessary:
  unsigned len=(unsigned)strlen(source);
  uns16 flags;
  tb_read_atr(cdp, tbdf, objnum, OBJ_FLAGS_ATR, (tptr)&flags);
  if (flags & CO_FLAG_ENCRYPTED) enc_buffer_total(cdp, source, len, (tobjnum)objnum);
 // save the new definition:
  if (tb_write_var(cdp, tbdf, objnum, OBJ_DEF_ATR, 0, len, source) ||
      tb_write_atr_len(cdp, tbdf, objnum, OBJ_DEF_ATR, len)) return false;
 // write the last modification date:
  uns32 stamp = stamp_now();
  tb_write_atr(cdp, tbdf, objnum, OBJ_MODIF_ATR, &stamp);
  cdp->add_sql_result(objnum);  // asi se nevyuziva
  return true;
}

BOOL sql_stmt_create_alter_front_object::exec(cdp_t cdp)
{
  PROFILE_AND_DEBUG(cdp, source_line);
  if (creating)
  { WBUUID uuid;  // using front-end uuid, different from schema_uuid_for_new_object()
    if (*schema) // explicit schema name specified
    { if (ker_apl_name2id(cdp, schema, uuid, NULL)) 
        { SET_ERR_MSG(cdp, schema);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
    }
    else // name of the created object is not prefixed: use the current (short-term) schema
    { if (null_uuid(cdp->front_end_uuid)) // current schema not defined
        { SET_ERR_MSG(cdp, "<schema name>");  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
      memcpy(uuid, cdp->front_end_uuid, UUID_SIZE);  // copying the current schema uuid
    }
    if (conditional && find2_object(cdp, categ, uuid, name)!=NOOBJECT) return FALSE;
    trecnum rec=create_object(cdp, name, uuid, definition, categ, FALSE);
    if (rec!=NORECNUM)
    {// analyse subcategory:
      uns16 obj_flags = 0;
      while (*definition==' ') definition++;
      if (categ==CATEG_PGMSRC)
      { if (*definition=='*')
          obj_flags=CO_FLAG_TRANSPORT;
        else if (!memcmp(definition, "<?xml", 5))  
          obj_flags=CO_FLAG_XML;
      }
      if (obj_flags != 0)
        tb_write_atr(cdp, objtab_descr, rec, OBJ_FLAGS_ATR, &obj_flags);
    }
    return rec==NORECNUM; 
  }
  else  // altering
  { tobjnum objnum;
    if (name_find2_obj(cdp, name, schema, categ, &objnum))
      { SET_ERR_MSG(cdp, name);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
    if (!alter_object(cdp, objtab_descr, objnum, definition, categ)) return TRUE;
    return FALSE;
  }  
}

BOOL sql_stmt_create_table::exec(cdp_t cdp)
{ trecnum trec;  int i;  tobjnum tabobjnum;
  PROFILE_AND_DEBUG(cdp, source_line);
 // locking order: reltab first, then tabtab:
  //table_descr_auto reltab_tbdf(cdp, REL_TABLENUM<tables_last ? REL_TABLENUM : NOTBNUM);
  //if (reltab_tbdf->me() && wait_record_lock_error(cdp, reltab_tbdf->me(), FULLTABLE, TMPWLOCK)) return TRUE;
 // get schema uuid for the table:
  WBUUID schema_uuid;
  if (!schema_uuid_for_new_object(cdp, ta->schemaname, schema_uuid)) return TRUE;
 // check for table name duplicity:
  tabobjnum = find2_object(cdp, CATEG_TABLE, schema_uuid, ta->selfname);
  if (tabobjnum != NOOBJECT)
    if (!SYSTEM_TABLE(tabobjnum) || cdp->in_use!=PT_SERVER) // unless initial creation of system tables
      if (conditional) return FALSE;
      else { SET_ERR_MSG(cdp, "TABTAB");  request_error(cdp, KEY_DUPLICITY);  return TRUE; }
 // store the definition of the table:
  if (!*ta->schemaname && objtab_descr!=NULL)  // must add explicit schema name to ta, required by table_all_to_source and when registering domains usage
    ker_apl_id2name(cdp, schema_uuid, ta->schemaname, NULL);  // schema uuid of the table converted to the schema name
  tptr defin=table_all_to_source(ta, FALSE);
  if (!defin) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
  cdp->ker_flags |= KFL_NO_JOUR;  
  trec=create_object(cdp, ta->selfname, schema_uuid, defin, CATEG_TABLE, FALSE);
  corefree(defin);
  if (trec==NORECNUM) { cdp->ker_flags &= ~KFL_NO_JOUR;  return TRUE; }
  cdp->add_sql_result(trec);  // used when creating FT system an by the table designer
 // table flags:
  uns16 flags;
  flags = ta->tabdef_flags & TFL_ZCR ? CO_FLAG_REPLTAB : 0;
  tb_write_atr(cdp, tabtab_descr, trec, OBJ_FLAGS_ATR, &flags);
 // creation (non-transactional):
  tblocknum basblockn=create_table_data(cdp, ta->indxs.count(), &cdp->tl);
  if (!basblockn) goto err;
 // store basblock number into the table record in TABTAB: 
  tb_write_atr(cdp, tabtab_descr, trec, TAB_BASBL_ATR, &basblockn);
 // register the reference integrity rules:
  for (i=0;  i<ta->refers.count();  i++)
  { forkey_constr * ref = ta->refers.acc0(i);
    update_reftab_list(cdp, ta->selfname, ref->desttab_name, schema_uuid, ref->desttab_schema, TRUE);
  }
 // register the domain usage:
  register_domains_in_table(cdp, ta, schema_uuid);
 // assign global privileges to all records of the new table (Autor: read & write all, insert, delete, grant):
  { table_descr_auto td(cdp, trec);
    if (td->me())
    { if (trec > USER_TABLENUM) 
      { t_privils_flex priv_val(td->attrcnt);
        priv_val.set_all_rw();
        *priv_val.the_map()=RIGHT_DEL | RIGHT_INSERT | RIGHT_GRANT; // global read & write will be added
        cdp->prvs.set_own_privils(td->me(), NORECNUM, priv_val);
      }
      set_role_privils(cdp, trec, CATEG_TABLE, schema_uuid, FALSE);
     // to transaction register:
      register_table_creation(cdp, (ttablenum)trec);
      register_basic_allocation(cdp, basblockn, td->multpage, td->rectopage, td->indx_cnt);
    }
  }
  cdp->ker_flags &= ~KFL_NO_JOUR;
  return FALSE;
 err:
  tb_del(cdp, tabtab_descr, trec);
  cdp->ker_flags &= ~KFL_NO_JOUR;
  return TRUE;
}

BOOL sql_stmt_create_view::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  WBUUID uuid;
  if (!schema_uuid_for_new_object(cdp, schema, uuid)) return TRUE;
  if (conditional && find2_object(cdp, CATEG_CURSOR, uuid, name)!=NOOBJECT) return FALSE;
  return create_object(cdp, name, uuid, source, CATEG_CURSOR, FALSE)==NORECNUM; 
}

BOOL sql_stmt_create_schema::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  tobjnum aplobj;
  if (create_schema(cdp, schema_name, FALSE, &aplobj, conditional)) return TRUE;
 // open the new application:
  ker_set_application(cdp, schema_name, aplobj);
  return FALSE;
}

trecnum new_user_or_group(cdp_t cdp, const tobjname name, tcateg categ, const WBUUID uuid, bool no_privil_check)
// Does not define the password.
{// check privils: CONF and SECUR admins are always OK, other subjects may be granted the privilege (DB_ADMIN has it by default)
  if (!no_privil_check && !cdp->prvs.is_secur_admin && !cdp->prvs.is_conf_admin)
    if (!can_insert_record(cdp, usertab_descr))
      { request_error(cdp, NO_CONF_RIGHT);  return TRUE; }
 // insert the record:
  wbbool zero8 = 0;
  t_colval colvals[4+1] = {
   { USER_ATR_LOGNAME, NULL, name,   NULL, NULL, 0, NULL },
   { USER_ATR_CATEG,   NULL, &categ, NULL, NULL, 0, NULL },
   { USER_ATR_UUID,    NULL, uuid,   NULL, NULL, 0, NULL },
   { USER_ATR_STOPPED, NULL, &zero8, NULL, NULL, 0, NULL },
   { NOATTRIB,         NULL, NULL,   NULL, NULL, 0, NULL } };
  t_vector_descr vector(colvals, NULL);
  trecnum trec = tb_new(cdp, usertab_descr, FALSE, &vector);
  if (trec!=NORECNUM && categ==CATEG_USER && trec!=ANONYMOUS_USER)
  {// set self-privileges:
    t_privils_flex priv_descr(usertab_descr->attrcnt);
    priv_descr.clear();
    priv_descr.the_map()[0]=RIGHT_DEL|RIGHT_GRANT;
    priv_descr.add_write(USER_ATR_LOGNAME);
    priv_descr.add_write(USER_ATR_INFO);
    priv_descr.add_write(USER_ATR_CERTIF);
    priv_descr.add_write(USER_ATR_SERVER);
    privils prvs(cdp);
    prvs.set_user(trec, CATEG_USER, FALSE);
    prvs.set_own_privils(usertab_descr, trec, priv_descr);
    // categ, UUID cannot be overwriten.
    // ups, certif_koef, pubkey, certificates are writen by server only.
   // remove creator's privileges (usually creator is ANONYMOUS_USER):
    priv_descr.clear();
    cdp->prvs.set_own_privils(usertab_descr, trec, priv_descr);
  }
  return trec;
}

BOOL sql_stmt_create_drop_subject::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  if (creating)
  { if (categ==CATEG_ROLE)
    {// get schema uuid for the role:
      WBUUID schema_uuid;
      if (!schema_uuid_for_new_object(cdp, subject_schema, schema_uuid)) return TRUE;
     // conditionally create:
      if (conditional && find2_object(cdp, CATEG_ROLE, schema_uuid, subject_name)!=NOOBJECT) return FALSE;
      return create_object(cdp, subject_name, schema_uuid, NULL, CATEG_ROLE, FALSE)==NORECNUM;
    }
    else  // user or group
    { if (conditional && find2_object(cdp, categ, NULL, subject_name) != NOOBJECT)
        return FALSE;
      WBUUID subj_uuid;  safe_create_uuid(subj_uuid);
      trecnum trec=new_user_or_group(cdp, subject_name, categ, subj_uuid);  // privileges checked inside
      if (trec==NORECNUM) return TRUE;
      set_new_password2(cdp, trec, password ? password : NULLSTRING, !password || subject_name[0]=='_');  // expired empty password, if not specified
      return FALSE;
    }
  }
  else // dropping
  { tobjnum objnum;  
    if (categ==CATEG_ROLE)
    { if (name_find2_obj(cdp, subject_name, subject_schema, categ, &objnum))
        if (conditional) return FALSE;
        else { SET_ERR_MSG(cdp, subject_name);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
      if (!can_delete_record(cdp, objtab_descr, objnum))
        { request_error(cdp, NO_RIGHT);  return TRUE; }
      return tb_del(cdp, objtab_descr, objnum);
    }
    else  // user, group: may be deleted by conf. or security admin or by itself
    { objnum=find2_object(cdp, categ, NULL, subject_name);
      if (objnum==NOOBJECT)
        if (conditional) return FALSE;
        else { SET_ERR_MSG(cdp, subject_name);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
      if (objnum<FIXED_USER_TABLE_OBJECTS) { request_error(cdp, NO_RIGHT);  return TRUE; }
      if (!cdp->prvs.is_secur_admin && !cdp->prvs.is_conf_admin && objnum!=cdp->prvs.luser())
        if (!can_delete_record(cdp, usertab_descr, objnum))
          { request_error(cdp, NO_CONF_RIGHT);  return TRUE; }
      return tb_del(cdp, usertab_descr, objnum);
    }
  }
}

BOOL sql_stmt_create_alter_seq::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
 // find the current value:
  if (creating) seq.currval=seq.startval;
  else // ALTERING
    if (seq.restart_specified) seq.currval=seq.restartval;
    else if (!seq.anything_specified) seq.currval=seq.startval;
    else // preserving current value: check if the currval is between the new min and max values
    { if (seq.currval<seq.minval) seq.currval=seq.minval;
      if (seq.currval>seq.maxval) seq.currval=seq.maxval;
    }
 // create the definition and save it:
  tptr source = sequence_to_source(cdp, &seq, FALSE); // creates CREATE SEQUENCE statement anyways, without schema name
  if (!source) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
  if (creating)
  { WBUUID uuid;
    if (!schema_uuid_for_new_object(cdp, seq.schema, uuid)) goto err;
    if (conditional && find2_object(cdp, CATEG_SEQ, uuid, seq.name)!=NOOBJECT) goto ok_exit;
    trecnum trec = create_object(cdp, seq.name, uuid, source, CATEG_SEQ, FALSE);
    if (trec==NORECNUM) goto err;
    cdp->add_sql_result(trec);  // used by the dialog for creating the sequence
  }
  else // altering exising sequence
  { tobjnum seq_obj;
    if (name_find2_obj(cdp, seq.name, seq.schema, CATEG_SEQ, &seq_obj))
      { SET_ERR_MSG(cdp, seq.name);  request_error(cdp, OBJECT_DOES_NOT_EXIST);  goto err; }
    save_sequence(cdp, seq_obj); // stores the cached values
    if (!alter_object(cdp, objtab_descr, seq_obj, source, CATEG_SEQ)) goto err;
   // reset the sequence cache:
    reset_sequence(cdp, seq_obj);
  }
 ok_exit:
  corefree(source);
  commit(cdp);  // otherwise seq.next_val cannot be called because it uses another transaction stream
  return FALSE;
 err:
  corefree(source);
  return TRUE;
}

BOOL sql_stmt_create_alter_fulltext::exec(cdp_t cdp)
// NOTE: The empty ALTER FULLTEXT statement means "rebuild index".
{ bool ok;  WBUUID uuid;
  PROFILE_AND_DEBUG(cdp, source_line);
  if (creating)
  { if (!schema_uuid_for_new_object(cdp, ftdef.schema, uuid)) return TRUE;
    if (conditional && find2_object(cdp, CATEG_INFO, uuid, ftdef.name)!=NOOBJECT) return FALSE;
   // create the new source without the possible SCHEMA prefix:
    char * create_source = ftdef.to_source(cdp, false);
    if (!create_source) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
    if (!*ftdef.schema)
      ker_apl_id2name(cdp, uuid, ftdef.schema, NULL);  // explic_schema_name needs it
    ok=create_fulltext_system(cdp, ftdef.name, create_source, ftdef.name, ftdef.schema, ftdef.language, ftdef.with_substructures, ftdef.separated, ftdef.bigint_id);  // creating v9 fulltext system
    corefree(create_source);
    if (!ok) return TRUE;
   // registering the fulltext system at the tables:
    register_tables_in_ftx(cdp, &ftdef, uuid);
   // index new document sources:
    index_doc_sources(cdp, &ftdef, false);
  }
  else // altering existing fulltext
  { t_fulltext ftdef_local;
   // recompile the alter statement against the current state of the fulltext system:
    ftdef_local.modifying = true;
    int err = ftdef_local.compile(cdp, source);
    if (err) { request_error(cdp, err);  return TRUE; }  // compliation error, including OBJECT_NOT_FOUND
   // create the altered source:
    char * create_source = ftdef_local.to_source(cdp, false);
    if (!create_source) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
    ok = alter_object(cdp, objtab_descr, ftdef_local.objnum, create_source, CATEG_INFO);
    corefree(create_source);
    if (!ok) return TRUE;
   // re-registering the fulltext system at the tables:
    tb_read_atr(cdp, objtab_descr, ftdef_local.objnum, APPL_ID_ATR, (tptr)uuid);
    unregister_tables_in_ftx(cdp, ftdef.name, uuid);
    register_tables_in_ftx(cdp, &ftdef_local, uuid);
   // remove the FT system description from the global list:
    drop_ft_ftom_list(ftdef_local.objnum);
   // index new document sources and un-index the dropped sources:
    index_doc_sources(cdp, &ftdef_local, true);
   // update contetxt:
    update_ft_kontext(cdp, &ftdef_local); 
  }
  commit(cdp);  
  global_fulltext_info_generation++;
  return FALSE;
}

BOOL sql_stmt_create_alter_rout::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  if (creating)
  { WBUUID uuid;
    if (!schema_uuid_for_new_object(cdp, schema, uuid)) return TRUE;
    if (conditional && find2_object(cdp, CATEG_PROC, uuid, name)!=NOOBJECT) return FALSE;
    return create_object(cdp, name, uuid, source, CATEG_PROC, FALSE)==NORECNUM; 
  }
  else // altering exising routine
  { tobjnum objnum;
    if (name_find2_obj(cdp, name, schema, CATEG_PROC, &objnum))
      { SET_ERR_MSG(cdp, name);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
    if (!alter_object(cdp, objtab_descr, objnum, source, CATEG_PROC)) return TRUE;
    psm_cache.invalidate(/*objnum*/NOOBJECT, CATEG_PROC);
    return FALSE;
  }
}

BOOL sql_stmt_create_alter_trig::exec(cdp_t cdp)
// Triggers can be created/modified only by database administrators or by administrators of the schema
//  the trigger belongs to.
{ tobjnum objnum;
  PROFILE_AND_DEBUG(cdp, source_line);
  WBUUID uuid;
  if (!schema_uuid_for_new_object(cdp, schema, uuid)) return TRUE;
 // create or alter the trigger:
  if (creating)
  { if (conditional && find2_object(cdp, CATEG_TRIGGER, uuid, name)!=NOOBJECT) return FALSE;
    objnum = create_object(cdp, name, uuid, source, CATEG_TRIGGER, FALSE); 
  }
  else // altering exising routine
  {// unregister the original trigger:
    if (name_find2_obj(cdp, name, schema, CATEG_TRIGGER, &objnum))
      { SET_ERR_MSG(cdp, name);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
    register_unregister_trigger(cdp, objnum, FALSE);
   // altering (the trigger will be searched again, no problem):
    if (!alter_object(cdp, objtab_descr, objnum, source, CATEG_TRIGGER)) return TRUE;
    psm_cache.invalidate(objnum, CATEG_TRIGGER);
  }
  if (objnum==NOOBJECT) return TRUE;
 // register the trigger:
  register_unregister_trigger(cdp, objnum, TRUE);
  return FALSE;
}

struct t_altered_table
{ tobjname table_name;
  WBUUID appl_uuid;
  ttablenum orig_tabnum;
  table_descr * orig_td;
  table_descr * new_td;
  ttablenum new_tabnum;
};

SPECIF_DYNAR(t_alt_tab_dynar, t_altered_table, 4);

BOOL sql_stmt_create_alter_dom::exec(cdp_t cdp)
{ tobjnum objnum;
  t_alt_tab_dynar alt_tab_dynar;
  PROFILE_AND_DEBUG(cdp, source_line);
  WBUUID uuid;
  if (!schema_uuid_for_new_object(cdp, schema, uuid)) return TRUE;
  int orig_flags = cdp->ker_flags;
 // create or alter the domain:
  if (creating)
  { if (conditional && find2_object(cdp, CATEG_DOMAIN, uuid, name)!=NOOBJECT) return FALSE;
    objnum = create_object(cdp, name, uuid, source, CATEG_DOMAIN, FALSE); 
  }
  else // altering existing domain
  { if (name_find2_obj(cdp, name, schema, CATEG_DOMAIN, &objnum))
      { SET_ERR_MSG(cdp, name);  request_error(cdp, OBJECT_DOES_NOT_EXIST);  goto alter_exit; }
    cdp->ker_flags |= KFL_DISABLE_REFINT;
   // create list of dependent tables:
    t_reltab_index2 start(REL_CLASS_DOM, name, uuid),
                    stop (REL_CLASS_DOM, name, uuid);
    t_index_interval_itertor iii(cdp);
    if (!iii.open(REL_TABLENUM, REL_TAB_INDEX2, &start, &stop)) 
      return TRUE;
    do
    { trecnum rec=iii.next();
      if (rec==NORECNUM) break;  // no (more) dependent tables
      tobjname table_name;  WBUUID table_schema_id;
      if (fast_table_read(cdp, iii.td, rec, REL_PAR_NAME_COL, table_name) ||
          fast_table_read(cdp, iii.td, rec, REL_PAR_UUID_COL, (tptr)table_schema_id)) 
        return TRUE;
      ttablenum tabnum = find2_object(cdp, CATEG_TABLE, table_schema_id, table_name);
      if (tabnum!=NOOBJECT)   // table exists, otherwise ignored
      { t_altered_table * at = alt_tab_dynar.next();
        if (at==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
        at->orig_tabnum=tabnum;  at->new_tabnum=NOOBJECT;  at->orig_td=NULL;  at->new_td=NULL;
        strcpy(at->table_name, table_name);  memcpy(at->appl_uuid, table_schema_id, UUID_SIZE);
      }
    } while (true);
    iii.close();
   // lock original tabdescrs and write-lock definitions of tables
    int i;
    for (i=0;  i<alt_tab_dynar.count();  i++)
    { t_altered_table * at = alt_tab_dynar.acc0(i);
      at->orig_td=install_table(cdp, at->orig_tabnum);
      if (at->orig_td==NULL) goto alter_exit;
     // just check if not read locked
      if (record_lock_error(cdp, tabtab_descr, at->orig_tabnum, WLOCK)) goto alter_exit;
      record_unlock(cdp, tabtab_descr, at->orig_tabnum, WLOCK);
      if (record_lock_error(cdp, tabtab_descr, at->orig_tabnum, TMPWLOCK)) goto alter_exit;
    }
   // altering (RB on error):
    if (!alter_object(cdp, objtab_descr, objnum, source, CATEG_DOMAIN)) goto alter_exit;
   // rename the old tables (RB on error):
    for (i=0;  i<alt_tab_dynar.count();  i++)
    { t_altered_table * at = alt_tab_dynar.acc0(i);
      tobjname uniq_name;
      sprintf(uniq_name, "\001%u", at->orig_tabnum);
      tb_write_atr(cdp, tabtab_descr, at->orig_tabnum, OBJ_NAME_ATR, uniq_name);
    }
   // create new tables (RB on error):
    for (i=0;  i<alt_tab_dynar.count();  i++)
    { t_altered_table * at = alt_tab_dynar.acc0(i);
      tptr defin=ker_load_objdef(cdp, tabtab_descr, at->orig_tabnum);
      at->new_tabnum=create_object(cdp, at->table_name, at->appl_uuid, defin, CATEG_TABLE, FALSE);
      if (at->new_tabnum==NORECNUM) { corefree(defin);  goto alter_exit; }
     // copy table flags and folder:
      uns16 flags;  tobjname folder_name;
      tb_read_atr(cdp, tabtab_descr, at->orig_tabnum, OBJ_FLAGS_ATR,  (char*)&flags);  // export & encryption flags, replication
      tb_read_atr(cdp, tabtab_descr, at->orig_tabnum, OBJ_FOLDER_ATR, folder_name);
      tb_write_atr(cdp, tabtab_descr, at->new_tabnum, OBJ_FLAGS_ATR,  &flags);
      tb_write_atr(cdp, tabtab_descr, at->new_tabnum, OBJ_FOLDER_ATR, folder_name);
     // re-save the encrypted definition, if the oiginal was encypted:
      if (flags & CO_FLAG_ENCRYPTED)
        save_table_definition(cdp, at->new_tabnum, defin);   // releases [defin]
      else
        corefree(defin);
     // creation (transactional):
      tblocknum basblockn=create_table_data(cdp, at->orig_td->indx_cnt, &cdp->tl);
      if (!basblockn) goto alter_exit;
     // store basblock number into the table record in TABTAB: 
      tb_write_atr(cdp, tabtab_descr, at->new_tabnum, TAB_BASBL_ATR, &basblockn);
      at->new_td=install_table(cdp, at->new_tabnum);
      if (at->new_td==NULL) goto alter_exit;
    }
   // move data:
    for (i=0;  i<alt_tab_dynar.count();  i++)
    { t_altered_table * at = alt_tab_dynar.acc0(i);
      bool any_multiassign=false;  t_atrmap_flex multimap(at->new_td->attrcnt);  multimap.clear();
     // prepare source kontext:
      db_kontext src;  src.fill_from_td(cdp, at->orig_td);  src.next=NULL;
     // prepare the initialisation vector for inserting records:
      t_colval colvals[255+1];  t_vector_descr vector(colvals);
      unsigned colind = 0;
      for (int i=1;  i<at->new_td->attrcnt;  i++)
      { attribdef * new_att = &at->new_td->attrs[i];
        t_express * ex;
        colvals[colind].attr =i;  
        colvals[colind].index=NULL;
        colvals[colind].table_number=0;
        colvals[colind].lengthptr=NULL;  // used when copying multiattributes
        if (new_att->attrmult!=1)
        { any_multiassign=true;
          multimap.add(i);
          colvals[colind].dataptr=DATAPTR_DEFAULT;
        }
        else
        { colvals[colind].expr = ex = new t_expr_attr(i, &src, new_att->name);
          if (ex==NULL) goto alter_exit;
         // add the explicit conversion, if possible:
          t_convert conv = get_type_cast(ex->type, new_att->attrtype, ex->specif, new_att->attrspecif, false);
          if (conv.conv==CONV_ERROR) { delete ex;  colvals[colind].expr = NULL;  continue; }
          ex->convert = conv;
          ex->type=new_att->attrtype;  ex->specif=new_att->attrspecif;
        }
        colind++;
      }
      colvals[colind].attr=NOATTRIB; // the delimiter
     // prepare inserting deleted records:
      t_vector_descr default_deleted_vector(&default_colval);
      default_deleted_vector.delete_record=true;
     // move data (FULLTABLE locks prevent editing data in the old table and make writing into the new table faster):
      bool err=false;
      if (!wait_record_lock_error(cdp, at->orig_td, FULLTABLE, RLOCK))  // lock must not be TMP because commit is used
      { if (!wait_record_lock_error(cdp, at->new_td, FULLTABLE, WLOCK))  // lock must not be TMP because commit is used
        { trecnum rec1, rec2, limit;
          limit=at->orig_td->Recnum();
          for (rec1=0;  rec1<limit && !err;  rec1++)
          { //if (!(rec1 % 32)) /* better than 64 */
            //{ commit(cdp); /* 1st commit at the start */
            //  if (cdp->is_an_error()) err=TRUE;
            //}
            uns8 del=table_record_state(cdp, at->orig_td, rec1);
            if (del)
            { int saved_flags = cdp->ker_flags;
              cdp->ker_flags |= KFL_DISABLE_INTEGRITY | KFL_DISABLE_REFCHECKS | KFL_NO_NEXT_SEQ_VAL | KFL_DISABLE_DEFAULT_VALUES | KSE_FORCE_DEFERRED_CHECK;  // disable immediate checks for the deleted record
              rec2=tb_new(cdp, at->new_td, FALSE, &default_deleted_vector);
              cdp->ker_flags = saved_flags;
              if (rec2==NORECNUM) break;
              //tb_del(cdp, new_tabdescr, rec2); -- done by default_deleted_vector, prevents conflict on unique indicies
            }
            else
            { cdp->check_rights=false;
              cdp->except_type=EXC_NO;  cdp->except_name[0]=0;  cdp->processed_except_name[0]=0;
              src.position=rec1;  
              rec2=tb_new(cdp, at->new_td, FALSE, &vector);
              cdp->check_rights=true; 
              if (rec2==NORECNUM) break;
              if (cdp->except_type!=EXC_NO) 
                { err=TRUE;  break; }
              if (any_multiassign)
              { for (int i=1;  i<at->new_td->attrcnt;  i++)
                  if (multimap.has(i))
                    copy_var_len(cdp, at->new_td, rec2, i, NOINDEX, at->orig_td, rec1, i, NOINDEX, NULL);
              }
            } // move data for the record pair
            if (cdp->break_request)
              { request_error(cdp, REQUEST_BREAKED);  err=TRUE;  break; }
          } // cycle on records
          if (cdp->is_an_error()) err=TRUE;
          //commit(cdp);   /* commit may have been called above */
          if (cdp->is_an_error()) err=TRUE;
          record_unlock(cdp, at->new_td, FULLTABLE, WLOCK);
        }
        else err=TRUE;
        record_unlock(cdp, at->orig_td, FULLTABLE, RLOCK);
      }
      else err=TRUE;
     // release colvals expressions:
      for (colind=0;  colvals[colind].attr!=NOATTRIB;  colind++)
      { if (colvals[colind].expr ) delete colvals[colind].expr;
        if (colvals[colind].index) delete colvals[colind].index;
      }

    }
   // commit all:
    if (commit(cdp)) goto alter_exit;
   // copy old privileges to the new tables and drop the old tables:
    for (i=0;  i<alt_tab_dynar.count();  i++)
    { t_altered_table * at = alt_tab_dynar.acc0(i);
     // copy privils:
      uns32 len;  tptr buf;
      buf = ker_load_blob(cdp, tabtab_descr, at->orig_tabnum, tabtab_descr->rec_privils_atr,   &len);
      tb_write_var       (cdp, tabtab_descr, at->new_tabnum,  tabtab_descr->rec_privils_atr, 0, len, buf);
      tb_write_atr_len   (cdp, tabtab_descr, at->new_tabnum,  tabtab_descr->rec_privils_atr,    len);
      corefree(buf);
      buf = ker_load_blob(cdp, tabtab_descr, at->orig_tabnum, TAB_DRIGHT_ATR,   &len);
      tb_write_var       (cdp, tabtab_descr, at->new_tabnum,  TAB_DRIGHT_ATR, 0, len, buf);
      tb_write_atr_len   (cdp, tabtab_descr, at->new_tabnum,  TAB_DRIGHT_ATR,    len);
      corefree(buf);
     // drop table:
      table_rel(cdp, at->orig_tabnum);   /* release table data */
      tb_del(cdp, tabtab_descr, at->orig_tabnum);
      unlock_tabdescr(at->orig_td); 
      unlock_tabdescr(at->new_td);
    }
    cdp->ker_flags = orig_flags;
    psm_cache.invalidate(NOOBJECT, CATEG_PROC);  // destroys procedures and triggers
  }
  if (objnum==NOOBJECT) return TRUE;
  return FALSE;

 alter_exit:
  for (int i=0;  i<alt_tab_dynar.count();  i++)
  { t_altered_table * at = alt_tab_dynar.acc0(i);
    //record_unlock(cdp, tabtab_descr, at->orig_tabnum, TMPWLOCK);
    if (at->orig_td!=NULL) unlock_tabdescr(at->orig_td);
    if (at->new_td !=NULL) unlock_tabdescr(at->new_td);
  }
  cdp->ker_flags = orig_flags;
  return TRUE;
}

void WINAPI create_index_comp_link(CIp_t CI)
{ bool conditional=false;
  while (CI->cursym && CI->cursym!=S_INDEX && CI->cursym!=S_UNIQUE) 
  { if (CI->cursym==S_EXISTS) conditional=true;
    next_sym(CI);
  }
  table_def_comp(CI, (table_all *)CI->univ_ptr, (table_all *)CI->stmt_ptr, conditional ? CREATE_INDEX_COND : CREATE_INDEX);
}

void WINAPI drop_index_comp_link(CIp_t CI)
{ bool conditional=false;
  while (CI->cursym && CI->cursym!=S_INDEX) 
  { if (CI->cursym==S_EXISTS) conditional=true;
    next_sym(CI);
  }
  table_def_comp(CI, (table_all *)CI->univ_ptr, (table_all *)CI->stmt_ptr, conditional ? DROP_INDEX_COND : DROP_INDEX);
}


void destroy_temptab_and_restore_error(cdp_t cdp, ttablenum tbnum, int saved_ker_flags)
{ int err = cdp->get_return_code();
  roll_back(cdp);  // to be sure
  request_error(cdp, 0);  // remove the error in order to be able to destroy the temp. table
  destroy_temporary_table(cdp, tbnum);  
  commit(cdp);
  cdp->ker_flags = saved_ker_flags;  
  request_error(cdp, err);  // restore the error
}      

BOOL sql_stmt_alter_table::exec(cdp_t cdp)
{ int i, j;  table_all ta_old, ta_new;
  PROFILE_AND_DEBUG(cdp, source_line); 
 // compile the ALTER TABLE statement:
  { compil_info xCI(cdp, source, statement_type==SQL_STAT_CREATE_INDEX ? create_index_comp_link : 
                                 statement_type==SQL_STAT_DROP_INDEX   ? drop_index_comp_link   : table_def_comp_link);
    xCI.univ_ptr=&ta_new;  xCI.stmt_ptr=&ta_old;
    if (compile(&xCI)) return TRUE;
  }
  WBUUID schema_uuid;  
  t_table_access_for_altering tafa;
  table_descr * orig_tabdescr = tafa.open(cdp, ta_new.schemaname, ta_new.selfname, schema_uuid);
  if (!orig_tabdescr) return TRUE;  // the error may have been handled
  if (commit(cdp)) return TRUE;  // error pending. Moved AFTER checking the privils for alter, otherwise missing privils do nor rollback previous actions
  ttablenum tabnum = orig_tabdescr->tbnum;
  int saved_ker_flags = cdp->ker_flags;
 // locking order: reltab first, then tabtab:
  //table_descr_auto reltab_tbdf(cdp, REL_TABLENUM); -- timeouts!
  //if (wait_record_lock_error(cdp, reltab_tbdf->me(), FULLTABLE, TMPWLOCK)) return TRUE;
  if (!*ta_new.schemaname)  // must add explicit schema name to ta, required by save_table_definition etc.
    ker_apl_id2name(cdp, schema_uuid, ta_new.schemaname, NULL);  // schema uuid of the table converted to the schema name
  add_implicit_constraint_names(&ta_new);

#if WBVERS>=950 // ALTER TABLE has stricter consistency rules than using the existing table: 
// 1. Consistency of remote keys if the parent table exists
// 2. Consistency of remote keys with local keys
  for (i=0;  i<ta_new.refers.count();  i++)
  { const forkey_constr * ref = ta_new.refers.acc0(i);
    if (ref->state!=ATR_DEL)  // important when !old_alter_table_syntax
    { ttablenum desttabnum;
      if (*ref->desttab_schema)
      { if (name_find2_obj(cdp, ref->desttab_name, ref->desttab_schema, CATEG_TABLE, &desttabnum))
          desttabnum=NOOBJECT;
      }
      else 
        desttabnum=find2_object(cdp, CATEG_TABLE, schema_uuid, ref->desttab_name);
      if (desttabnum!=NOOBJECT)
      { table_all par_ta;
        int res=partial_compile_to_ta(cdp, desttabnum, &par_ta);
        if (res) 
          { request_error(cdp, res);  return TRUE; }
        dd_index_sep par_ind_descr, loc_ind_descr;  // must be destructed and constructed for every [ref]!
        if (find_index_for_key(cdp, ref->par_text, &par_ta, NULL, &par_ind_descr) < 0)
          { request_error(cdp, NO_PARENT_INDEX); return TRUE; }
        if (find_index_for_key(cdp, ref->text, &ta_new, NULL, &loc_ind_descr) < 0)  // missing local index
          { request_error(cdp, NO_LOCAL_INDEX); return TRUE; }
        if (!loc_ind_descr.same(par_ind_descr, false))  // compating types only
          { request_error(cdp, NO_PARENT_INDEX); return TRUE; }
      }
    }
  }

#endif

  cdp->ker_flags |= KFL_ALLOW_TRACK_WRITE | KFL_NO_JOUR | KFL_NO_TRACE | KFL_DISABLE_REFINT;
 // update the list of valid referential integrity rules:
  if (old_alter_table_syntax) // old ALTER
  {/* removed ref. integrity: update parent reference list: */
    for (i=0;  i<ta_old.refers.count();  i++)   /* old foreign key */
    { forkey_constr * oref = ta_old.refers.acc0(i);
      BOOL found = FALSE;
      for (j=0;  j<ta_new.refers.count();  j++)  /* new foreign keys */
      { forkey_constr * nref = ta_new.refers.acc0(j);
        if (!strcmp(oref->desttab_name, nref->desttab_name) && !strcmp(oref->desttab_schema, nref->desttab_schema))
          { found=TRUE;  break; }
      }
      if (!found)  /* a foreign key removed */
        if (!update_reftab_list(cdp, ta_new.selfname, oref->desttab_name, schema_uuid, oref->desttab_schema, FALSE))
          { cdp->ker_flags = saved_ker_flags;  return TRUE; }
    }
    /* added ref. integrity: update parent reference list: */
    for (i=0;  i<ta_new.refers.count();  i++)   /* new foreign key */
    { forkey_constr * nref = ta_new.refers.acc0(i);
      BOOL found = FALSE;
      for (j=0;  j<ta_old.refers.count();  j++)  /* old foreign keys */
      { forkey_constr * oref = ta_old.refers.acc0(j);
        if (!strcmp(oref->desttab_name, nref->desttab_name) && !strcmp(oref->desttab_schema, nref->desttab_schema))
          { found=TRUE;  break; }
      }
      if (!found)  /* a foreign key removed */
        if (!update_reftab_list(cdp, ta_new.selfname, nref->desttab_name, schema_uuid, nref->desttab_schema, TRUE))
          { cdp->ker_flags = saved_ker_flags;  return TRUE; }
    }
  }
  else // updating constrains: new ALTER
  {// store the changes in references to the parent tables:
    for (i=0;  i<ta_new.refers.count();  i++)   /* new foreign key */
    { forkey_constr * ref = ta_new.refers.acc0(i);  BOOL error=FALSE;
      switch (ref->state)
      { case ATR_DEL: // must not remove the record in __reltab if there is another RI to the same table
        { int cnt = 0;
          for (j=0;  j<ta_old.refers.count();  j++)  /* old foreign keys */
          { forkey_constr * oref = ta_old.refers.acc0(j);
            if (!strcmp(oref->desttab_name, ref->desttab_name) && !strcmp(oref->desttab_schema, ref->desttab_schema))
              cnt++;
          }
          if (cnt==1)  // remove if there is only one reference to the upper table
            error=!update_reftab_list(cdp, ta_new.selfname, ref->desttab_name, schema_uuid, ref->desttab_schema, FALSE);
          break;
        }
        case ATR_NEW: // if the relation already exists, new record in __RELTAB will not be created
          error=!update_reftab_list(cdp, ta_new.selfname, ref->desttab_name, schema_uuid, ref->desttab_schema, TRUE);
          break;
      } // no action for unchanged references
      if (error) { cdp->ker_flags = saved_ker_flags;  return TRUE; }
    }
  }

 // update the domain usage:
  unregister_domains_in_table(cdp, ta_new.selfname, schema_uuid);
  register_domains_in_table(cdp, &ta_new, schema_uuid);

 // check if the index used in an RI in not deleted, update its number in the descriptor of the depending table:
  if (!check_index_for_referencing_tables(cdp, &ta_new, schema_uuid))
    { cdp->ker_flags = saved_ker_flags;  return TRUE; }

 // move ownership of tbdf from tafa to eta:
  { t_exclusive_table_access eta(cdp, true);
    if (!eta.open_excl_access(orig_tabdescr)) 
      { cdp->ker_flags = saved_ker_flags;  return TRUE; }
    tafa.drop_ownership();  eta.drop_descr=true;

    if (ta_new.attr_changed)    /* table redefinition via temporary table */
    {// create the temporary table:  
      tptr defin;  defin = table_all_to_source(&ta_new, FALSE);  // must create defin before translating domains (may add defval & check)
      table_descr * temp_table_descr = make_temp_table_from_ta(cdp, &ta_new, schema_uuid, NULL);
      if (!temp_table_descr) 
        { corefree(defin);  cdp->ker_flags = saved_ker_flags;  return TRUE; }
     /* prepare conversion program */
      if (!move_table_data(cdp, &ta_old, &ta_new, orig_tabdescr, temp_table_descr)) /* removes 2nd table on error */
      { corefree(defin);  
        destroy_temptab_and_restore_error(cdp, temp_table_descr->tbnum, saved_ker_flags);
        return TRUE; 
      }
      if (save_table_definition(cdp, tabnum, defin))   // not committed, defin released inside
      { destroy_temptab_and_restore_error(cdp, temp_table_descr->tbnum, saved_ker_flags);
        return TRUE; 
      }
       // { temp_table_descr->free_descr(cdp);  cdp->ker_flags = saved_ker_flags;  return TRUE; }  
      if (!transform_indexes(cdp, orig_tabdescr, temp_table_descr, &ta_old, &ta_new)) 
      { destroy_temptab_and_restore_error(cdp, temp_table_descr->tbnum, saved_ker_flags);
        return TRUE; 
      }
//        { temp_table_descr->free_descr(cdp);  cdp->ker_flags = saved_ker_flags;  return TRUE; } 
      destroy_table_data(cdp, orig_tabdescr);  // destroying data of the original table
     // write basblock number and index roots from the temp. table to the new table:
      tb_write_atr(cdp, tabtab_descr, tabnum, TAB_BASBL_ATR, &temp_table_descr->basblockn);
     // delete the temporary table:
      tables[temp_table_descr->tbnum]=NULL;
      temp_table_descr->free_descr(cdp);
    }
    else  /* constrain redefinition only, no temp. table used */
    { if (old_alter_table_syntax) // old ALTER
      {// searching new indicies among the existing ones (works for automatic indicies too):
        for (i=0;  i<ta_new.indxs.count();  i++)   /* new indicies */
        { ind_descr * nind = ta_new.indxs.acc0(i);
          int indxnum = find_index_for_key(cdp, nind->text, &ta_old, nind);
          nind->state = indxnum>=0 ? indxnum : INDEX_NEW;
        }
       // an old index may be referenced multiple times in [state] but change_indicies() will cope with this
      }
     /* change the table definition: */
      tptr defin;  defin = table_all_to_source(&ta_new, FALSE);
      if (save_table_definition(cdp, tabnum, defin)) { cdp->ker_flags = saved_ker_flags;  return TRUE; } // defin released inside
      change_indicies(cdp, orig_tabdescr, &ta_new, ta_new.indxs.count(), ta_new.indxs.acc0(0));  // must not use orig_tabdscr after this
    }

   // store table flags:
    uns16 flags;
    tb_read_atr(cdp, tabtab_descr, tabnum, OBJ_FLAGS_ATR, (tptr)&flags);
    if (ta_new.tabdef_flags & TFL_ZCR) flags |= CO_FLAG_REPLTAB;
    else                               flags &= ~CO_FLAG_REPLTAB;
    tb_write_atr(cdp, tabtab_descr, tabnum, OBJ_FLAGS_ATR, &flags);
    cdp->ker_flags = saved_ker_flags;
    psm_cache.invalidate(NOOBJECT, CATEG_PROC);  // destroys procedures and triggers
    commit(cdp);  // saving the new definition must not be rolled back because the table is restructured!
  } // this closes the exclusive access and make it possible to install the table. New definition must be committed before
 // update column privileges:
  { table_descr_auto td(cdp, tabnum);
    if (td->me()) cdp->prvs.convert_privils(td->me(), &ta_old, &ta_new);
  }
  return FALSE;
}
///////////////////////////////////////////// renaming objects /////////////////////////////////////////////////
static void WINAPI object_name_entry(CIp_t CI)
// On exit, CI->univ_ptr points to the object name start, CI->compil_ptr-1 is after it.
{ 
  if (CI->cursym==SQ_DECLARE || CI->cursym==S_CREATE) next_sym(CI); // CREATE: for wider compatibility
  if (CI->cursym!=SQ_TRIGGER && CI->cursym!=SQ_DOMAIN && CI->cursym!=SQ_PROCEDURE && CI->cursym!=SQ_FUNCTION && 
      CI->cursym!=SQ_SEQUENCE) 
    c_error(GENERAL_SQL_SYNTAX);
  next_sym(CI);
  if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
  CI->univ_ptr=(void*)CI->prepos;
  const char * nameend = CI->compil_ptr;
  if (next_sym(CI)=='.')  // the previous name was a schema name
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    CI->univ_ptr=(void*)CI->prepos;
  }
  else  // restore CI->compil_ptr
    CI->compil_ptr = nameend;
}

static void WINAPI trigger_minimal_entry(CIp_t CI)
// On exit, CI->univ_ptr points to the table name start, CI->compil_ptr-1 is after it.
{ 
  if (CI->cursym==SQ_DECLARE || CI->cursym==S_CREATE) next_sym(CI); // CREATE: for wider compatibility
  if (CI->cursym!=SQ_TRIGGER) c_error(TRIGGER_EXPECTED);
  tobjname name, schema;  get_schema_and_name(CI, name, schema);
 // trigger time:
  if (CI->cursym!=SQ_BEFORE && CI->cursym!=SQ_AFTER) c_error(BEFORE_OR_AFTER_EXPECTED);
 // trigger event:
  next_sym(CI);
  CIpos pos(CI);     // save compil. position
  if      (CI->cursym==S_DELETE) next_sym(CI);
  else if (CI->cursym==S_INSERT) next_sym(CI);
  else if (CI->cursym==S_UPDATE) 
  { if (next_sym(CI)==SQ_OF)
    { next_sym(CI);
      if (CI->cursym=='=' || CI->cursym==SQ_ANY) next_sym(CI);
      do // 1st pass -> skipping column list, cursym must be a column name
      { test_and_next(CI, S_IDENT, IDENT_EXPECTED);
        if (CI->cursym!=',') break;
        next_sym(CI);
      } while (true);
    }
  }
  else c_error(TRIGGER_EVENT_EXPECTED);
 // ON table name (schema name not allowed here):
  test_and_next(CI, S_ON, ON_EXPECTED);
  if (CI->cursym!=S_IDENT) c_error(TABLE_NAME_EXPECTED);
  CI->univ_ptr=(void*)CI->prepos;
}

char * replace_name(char * src, const char * objname_start, const char * objname_end, const char * new_name)
// Replaces object name between [objname_start] and [objname_end] in [src] by [new_name].
// Frees [src] and returns allocated definition.
{ int name_len = (int)(objname_end-objname_start);
  char * new_def = (char*)corealloc(strlen(src)+1 + strlen(new_name) - name_len + 2, 81);
  if (!new_def) return NULL;
  int pos = (int)(objname_start-src);
  memcpy(new_def, src, pos);
  new_def[pos++]='`';
  strcpy(new_def+pos, new_name);  pos+=(int)strlen(new_name);
  new_def[pos++]='`';
  strcpy(new_def+pos, objname_end);
  corefree(src);  
  return new_def;
}

bool rename_table(cdp_t cdp, ttablenum tbnum, tobjname new_name)
// Updates definitions of referencing tables
// Update definitions of triggers
// Update __RELTAB (refs, domain use, triggers)
// Update the definition of the renamed table and its descriptor
// -- All in a single transaction
// If committed:
// update descriptors of referencing table
// update its descriptor.
{ table_all ta;  int i, j;  tptr defin;  
  sys_Upcase(new_name);
  unsigned res_cnt; 
  res_cnt = cdp->sql_result_count(); 
 // get td, ta:
  table_descr_auto tbdf(cdp, tbnum);
  if (!tbdf->me()) return false;
  int res = partial_compile_to_ta(cdp, tbnum, &ta);
  if (res)
    { request_error(cdp, res);  return false; }
 // find triggers:
  tbdf->prepare_trigger_info(cdp);
 // find referencing tables:
  recnum_dynar table_list;
  if (!find_referencing_tables(cdp, tbdf->selfname,  tbdf->schema_uuid, &table_list))
    return FALSE;
 // Update definitions of referencing tables:
  for (i=0;  i<table_list.count();  i++)
  { table_all ch_ta;  
    ttablenum ch_tabnum = *table_list.acc0(i);
    res = partial_compile_to_ta(cdp, ch_tabnum, &ch_ta);
    if (res)
      { request_error(cdp, res);  goto err; }
   // update ch_ta:
    WBUUID ch_uuid;
    fast_table_read(cdp, tabtab_descr, ch_tabnum, APPL_ID_ATR, ch_uuid);
    for (j=0;  j<ch_ta.refers.count();  j++)
    { forkey_constr * ref = ch_ta.refers.acc0(i);
      if (!strcmp(ref->desttab_name, tbdf->selfname) && 
          (!strcmp(ref->desttab_schema, tbdf->schemaname) || !*ref->desttab_schema && !memcmp(tbdf->schema_uuid, ch_uuid, UUID_SIZE)))
        strcpy(ref->desttab_name, new_name);
    }
   // store modified tabdef:
    defin = table_all_to_source(&ch_ta, FALSE);
    if (save_table_definition(cdp, ch_tabnum, defin)) goto err;  
    // defin released inside
  }
 // update trigger definitions:
  for (i=0;  i<tbdf->triggers.count();  i++)
  { tobjnum objnum = tbdf->triggers.acc0(i)->trigger_objnum;
   // load the definition: 
    tptr src=ker_load_objdef(cdp, objtab_descr, objnum);
    if (src==NULL) 
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
   // update table name in the def.
    compil_info xCI(cdp, src, trigger_minimal_entry);
    int err=compile_masked(&xCI);  // client error not set when error in trigger encountered
    if (!err)
    { const char * tabname_start = (const char *)xCI.univ_ptr, * tabname_end = xCI.compil_ptr-xCI.charsize;
      src = replace_name(src, tabname_start, tabname_end, new_name);
      if (!src) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
      if (!alter_object(cdp, objtab_descr, objnum, src, CATEG_TRIGGER)) { corefree(src);  goto err; }
    }
    corefree(src);
  }
 // update __RELTAB:
  char sqlcmd[200];  
  cdp->prvs.is_data_admin++; // prevents checking privils to __RELTAB
  sprintf(sqlcmd, "UPDATE __RELTAB SET NAME1=\'%s\' WHERE NAME1=\'%s\' AND APL_UUID1=X\'", new_name, tbdf->selfname);
  i=(int)strlen(sqlcmd);
  bin2hex(sqlcmd+i, tbdf->schema_uuid, UUID_SIZE);  i+=2*UUID_SIZE;
  sqlcmd[i++]='\'';  sqlcmd[i]=0;
  if (sql_exec_direct(cdp, sqlcmd))
    { cdp->prvs.is_data_admin--;  goto err; }
  sprintf(sqlcmd, "UPDATE __RELTAB SET NAME2=\'%s\' WHERE NAME2=\'%s\' AND CLASSNUM=%u AND APL_UUID2=X\'", new_name, tbdf->selfname, REL_CLASS_RI);
  i=(int)strlen(sqlcmd);
  bin2hex(sqlcmd+i, tbdf->schema_uuid, UUID_SIZE);  i+=2*UUID_SIZE;
  sqlcmd[i++]='\'';  sqlcmd[i]=0;
  if (sql_exec_direct(cdp, sqlcmd))
    { cdp->prvs.is_data_admin--;  goto err; }
  cdp->prvs.is_data_admin--;
 // update own definition:
  strcpy(ta.selfname, new_name);
  defin = table_all_to_source(&ta, FALSE);
  if (save_table_definition(cdp, tbnum, defin)) goto err;  
  tb_write_atr(cdp, tabtab_descr, tbnum, OBJ_NAME_ATR, new_name);

  cdp->clear_sql_results(res_cnt);
  if (commit(cdp)) return false;
  cdp->add_sql_result(tbnum);

#ifdef STOP  // nothing to do, table number unchanged
 // update references in 
  for (i=0;  i<table_list.count();  i++)
  { ttablenum ch_tabnum = *table_list.acc0(i);
    table_descr_cond ch_tbdf(cdp, ch_tabnum);
    if (ch_tbdf->tbdf())
      for (j=0;  j<ch_tbdf->forkey_cnt;  j++)
      { dd_forkey * ch_cc = ch_tbdf->forkeys[j];
        ch_cc-> 
      }

  }
#endif
  strcpy(tbdf->selfname, new_name);
  return true;
 err:
  roll_back(cdp);
  return false;
}

BOOL sql_stmt_rename_object::exec(cdp_t cdp)
// For OBJTAB objects the modify privileges will be checked in alter_object.
{ tobjnum objnum;
  if (name_find2_obj(cdp, orig_name, schema, categ, &objnum))
    { SET_ERR_MSG(cdp, orig_name);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
  switch (categ)
  { case CATEG_TABLE:
      if (!can_write_objdef(cdp, tabtab_descr, objnum)) 
        return request_error(cdp, NO_RIGHT);
      return !rename_table(cdp, objnum, new_name);
    case CATEG_USER:  case CATEG_GROUP:
      //if (!can_write_objdef(cdp, usertab_descr, objnum))   ????
      //  return request_error(cdp, NO_RIGHT);
      return tb_write_atr(cdp, usertab_descr, objnum, USER_ATR_LOGNAME, new_name);  // names not stored in the membership/rights, UUIDS/objnums used instead
    case CATEG_DOMAIN:
    { WBUUID schema_uuid;
      fast_table_read(cdp, objtab_descr, objnum, APPL_ID_ATR, schema_uuid); 
     // update definitions of tables using the domain (no change in table descriptors):
      t_reltab_index2 start(REL_CLASS_DOM, orig_name, schema_uuid),
                      stop (REL_CLASS_DOM, orig_name, schema_uuid);
      t_index_interval_itertor iii(cdp);
      if (!iii.open(REL_TABLENUM, REL_TAB_INDEX2, &start, &stop)) 
        return TRUE;
      do
      { trecnum rec=iii.next();
        if (rec==NORECNUM) break;
       // read the table name:
        tobjname parent_name;  WBUUID parent_schema_id;
        if (fast_table_read(cdp, iii.td, rec, REL_PAR_NAME_COL, parent_name) ||
            fast_table_read(cdp, iii.td, rec, REL_PAR_UUID_COL, (tptr)parent_schema_id)) 
          break;
       // find the table:
        ttablenum tabnum = find2_object(cdp, CATEG_TABLE, parent_schema_id, parent_name);
        if (tabnum!=NOOBJECT)
        { table_all ta;  
          int res = partial_compile_to_ta(cdp, tabnum, &ta);
          if (res)
            { request_error(cdp, res);  break; }
         // update ch_ta:
          for (int i=1;  i<ta.attrs.count();  i++)
          { atr_all * att = ta.attrs.acc0(i);
            if (att->type==ATT_DOMAIN)
            { double_name * dnm = ta.names.acc(i);
              if (!*schema || !*dnm->schema_name || !strcmp(schema, dnm->schema_name))
                if (!strcmp(orig_name, dnm->name))
                  strcpy(dnm->name, new_name);
            }
          }
         // store modified tabdef:
          char * defin = table_all_to_source(&ta, FALSE);
          if (save_table_definition(cdp, tabnum, defin)) break;  // defin released inside
        }
      } while (true);
      iii.close();
     // cont. 
    }
    case CATEG_TRIGGER:  // update references in __RELTAB
    { char sqlcmd[200];  WBUUID schema_uuid;
      fast_table_read(cdp, objtab_descr, objnum, APPL_ID_ATR, schema_uuid); 
      sprintf(sqlcmd, "UPDATE __RELTAB SET NAME2=\'%s\' WHERE NAME2=\'%s\' AND CLASSNUM=%u AND APL_UUID2=X\'", new_name, orig_name, 
               categ==CATEG_DOMAIN ? REL_CLASS_DOM : REL_CLASS_TRIG);
      int i=(int)strlen(sqlcmd);
      bin2hex(sqlcmd+i, schema_uuid, UUID_SIZE);  i+=2*UUID_SIZE;
      sqlcmd[i++]='\'';  sqlcmd[i]=0;
      cdp->prvs.is_data_admin++; // prevents checking privils to __RELTAB
      if (sql_exec_direct(cdp, sqlcmd))
        { cdp->prvs.is_data_admin--;  return TRUE; }
      cdp->prvs.is_data_admin--;
      break;
    }
    // no special actions for CATEG_APPL, CATEG_ROLE, etc.
  }
 // only objects from OBJTAB come here
  if (categ==CATEG_PROC || categ==CATEG_TRIGGER || categ==CATEG_DOMAIN || categ==CATEG_SEQ)
  {// update object name in the def.
    char * src = ker_load_objdef(cdp, objtab_descr, objnum);
    if (!src) return TRUE;
    compil_info xCI(cdp, src, object_name_entry);
    int err=compile_masked(&xCI);  // client error not set when error in trigger encountered
    if (!err)
    { const char * tabname_start = (const char *)xCI.univ_ptr, * tabname_end = xCI.compil_ptr-xCI.charsize;
      src = replace_name(src, tabname_start, tabname_end, new_name);
      if (!src) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
      if (!alter_object(cdp, objtab_descr, objnum, src, categ)) { corefree(src);  return TRUE; }
    }
    corefree(src);
  }
 // store the new name in OBJTAB:
  tb_write_atr(cdp, objtab_descr, objnum, OBJ_NAME_ATR, new_name);
  return FALSE;
}
/////////////////////////// cursor operations /////////////////////////////
tcursnum open_stored_cursor(cdp_t cdp, tobjnum cursobj, cur_descr ** pcd, bool direct_stmt)
// Opens fixed cursor. Returns cursor or NOCURSOR on error.
// Defines the cursor name.
{ *pcd=NULL;
 // check privils & load cursor:
  WBUUID query_schema_uuid;
  fast_table_read(cdp, objtab_descr, cursobj, APPL_ID_ATR, query_schema_uuid); // same as child
  if (!cdp->has_full_access(query_schema_uuid))
    if (!can_read_objdef(cdp, objtab_descr, cursobj))
      { request_error(cdp, NO_RIGHT);  return NOCURSOR; }
  tptr buf=ker_load_objdef(cdp, objtab_descr, cursobj);
  if (buf==NULL) return NOCURSOR;  /* request_error called inside */
  BOOL admin_mode_query = is_in_admin_mode(cdp, cursobj, query_schema_uuid);
 // open the cursor:
  tcursnum cursnum;
  { t_exkont_sqlstmt ekt(cdp, cdp->sel_stmt, buf);
    t_short_term_schema_context stsc(cdp, query_schema_uuid);  // admin mode needs this
    if (admin_mode_query) cdp->in_admin_mode++;
    sql_open_cursor(cdp, buf, cursnum, pcd, direct_stmt);  // sets cursnum=NOCURSOR on error
    if (admin_mode_query) 
    { if (cursnum!=NOCURSOR) make_complete(*pcd);
      cdp->in_admin_mode--;
    }
  }

  corefree(buf);
 // write cursor name to cd->cursor_name:
  if (cursnum!=NOCURSOR)
  { tobjname cursor_name;
    tb_read_atr(cdp, objtab_descr, cursobj, OBJ_NAME_ATR, cursor_name);
    assign_name_to_cursor(cdp, cursor_name, *pcd);
  }
  return cursnum;
}

BOOL sql_stmt_open::exec(cdp_t cdp)
{ tcursnum cursnum;  cur_descr * cd;
  PROFILE_AND_DEBUG(cdp, source_line);
  if (offset==-1) // global cursor
  { if (find_named_cursor(cdp, cursor_name, cd) != NOCURSOR)
      return request_error(cdp, SQ_INVALID_CURSOR_NAME);  // may be handled
    tobjnum objnum = find_object(cdp, CATEG_CURSOR, cursor_name);
    if (objnum==NOOBJECT)
      { SET_ERR_MSG(cdp, cursor_name);  return request_error(cdp, OBJECT_DOES_NOT_EXIST); }
    cursnum=open_stored_cursor(cdp, objnum, &cd, false);
  }
  else // declared local cursor
  { tcursnum * varcurs = (tcursnum*)variable_ptr(cdp, level, offset);
    if (*varcurs != NOCURSOR)
      return request_error(cdp, SQ_INVALID_CURSOR_STATE);  // may be handled
    cursnum=create_cursor(cdp, loc_qe, NULL, loc_opt, &cd);
    *varcurs = cursnum;
  }
  if (cursnum==NOCURSOR) return TRUE;
 // position it before the 1st row:
  assign_name_to_cursor(cdp, cursor_name, cd);
  cd->position=(trecnum)-1;
  return FALSE;
}

BOOL sql_stmt_close::exec(cdp_t cdp)
{ tcursnum cursnum;  cur_descr * cd;
  PROFILE_AND_DEBUG(cdp, source_line);
  if (offset==-1) // global cursor
  { cursnum=find_named_cursor(cdp, cursor_name, cd);
    if (cursnum == NOCURSOR)
      return request_error(cdp, SQ_INVALID_CURSOR_NAME);  // may be handled
  }
  else
  { tcursnum * varcurs = (tcursnum*)variable_ptr(cdp, level, offset);
    cursnum = *varcurs;
    if (cursnum == NOCURSOR)
      return request_error(cdp, SQ_INVALID_CURSOR_STATE);  // may be handled
    *varcurs = NOCURSOR;
    { cur_descr * cd;
      { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
        cd = *crs.acc0(GET_CURSOR_NUM(cursnum));
      }
      t_temporary_current_cursor tcc(cdp, cd);
      loc_opt->close_data(cdp);
    }
  }
  free_cursor(cdp, cursnum);
  return FALSE;
}

BOOL sql_stmt_fetch::exec(cdp_t cdp)
{ tcursnum cursnum;  cur_descr * cd;
  PROFILE_AND_DEBUG(cdp, source_line);
  if (offset==-1) // global cursor
  { cursnum=find_named_cursor(cdp, cursor_name, cd);
    if (cursnum == NOCURSOR)
      return request_error(cdp, SQ_INVALID_CURSOR_NAME);  // may be handled
  }
  else
  { tcursnum * varcurs = (tcursnum*)variable_ptr(cdp, level, offset);
    cursnum = *varcurs;
    if (cursnum == NOCURSOR)
      return request_error(cdp, SQ_INVALID_CURSOR_STATE);  // may be handled
    { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
      cd = *crs.acc(GET_CURSOR_NUM(cursnum));
    }
  }
 // cursor in cd, find record number and position on it:
  int pos = cd->position;
  switch (fetch_type)
  { case FT_NEXT:  pos++;  break;
    case FT_PRIOR: pos--;  break;
    case FT_FIRST: pos=0;  break;
    case FT_LAST:  make_complete(cd);  pos=cd->recnum-1;  break;
    case FT_ABS:   // ABS is 1-based!
    { t_value res;  expr_evaluate(cdp, step, &res);
      pos=res.intval-1;  break;
    }
    case FT_REL:
    { t_value res;  expr_evaluate(cdp, step, &res);
      pos+=res.intval;  break;
    }
  }
  if (pos >= (int)cd->recnum)  // (int) necessary, otherwise position would be converted to unsigned!
  { cd->enlarge_cursor(pos+1);
    if (pos >= (int)cd->recnum)
    { cd->position=cd->recnum; // positioned after the last row
      return request_error(cdp, NOT_FOUND_02000);
    }
  }
  else if (pos < 0)
  { cd->position=NORECNUM;         // positioned before the 1st row
    return request_error(cdp, NOT_FOUND_02000);
  }
  cd->position=pos;
  cd->curs_seek(pos);
 // fetch values:
  for (int i=0;  i<assgns.count();  i++)
  { t_assgn * assgn = assgns.acc0(i);
   // must not use assgn->source because execute_assignment kurzor kontext used in it may be deleted in between
    if (i+1>=cd->qe->kont.elems.count()) // too many INTO items
      assgns.delet(i);
    else  // assign the value:
    { t_expr_attr source(i+1, &cd->qe->kont);
      t_convert conv = get_type_cast(source.origtype, assgn->dest->type, source.origspecif, assgn->dest->specif, TRUE);
      if (conv==CONV_ERROR)
      { request_error(cdp, CANNOT_CONVERT_DATA);  return TRUE;
      }
      else
      { source.convert=conv;  source.type=assgn->dest->type;  source.specif=assgn->dest->specif;
        if (execute_assignment(cdp, assgn->dest, &source, TRUE, TRUE, TRUE)) return TRUE;
      }
    }
  }
  return FALSE;
}

//////////////////////////// transactions ///////////////////////////////////
static int get_sapo_num(cdp_t cdp, t_express * sapo_target)
// get savepoint number identification from sapo_target:
{ if (sapo_target!=NULL)
  { t_value res;  expr_evaluate(cdp, sapo_target, &res);
    if (res.is_null()) return -1; // will not be found
    return res.intval;
  }
  return 0; // 0 must be copied into the created named savepoint
}

static void set_sapo_num(cdp_t cdp, t_express * sapo_target, int sapo_num)
// stores savepoint number identification, sapo_target!=NULL.
{ t_expr_sconst val(ATT_INT32, 0, &sapo_num, sizeof(sig32));
  execute_assignment(cdp, sapo_target, &val, TRUE, FALSE, TRUE);
}

BOOL sql_stmt_start_tra::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  if (cdp->expl_trans!=TRANS_NO) //warn(cdp, WAS_IN_TRANS);
    return request_error(cdp, SQ_TRANS_STATE_ACTIVE);  // may be handled
  cdp->expl_trans=TRANS_EXPL;
  cdp->isolation_level=isolation;
  cdp->trans_rw=rw;
  return FALSE;
}

BOOL sql_stmt_commit::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  if (cdp->savepoints.find_atomic_index() != -1 || cdp->in_trigger)
    return request_error(cdp, SQ_INVAL_TRANS_TERM);  // may be handled
  if (cdp->in_expr_eval)  // should be error, but I only treport it in order to remain compatible for some time
  { cd_dbg_err(cdp, "COMMIT inside an SQL expression is not allowed");
    return FALSE;
  }
  if (commit(cdp)) return TRUE;
  if (chain) cdp->expl_trans=TRANS_EXPL;
  return FALSE;
}

BOOL sql_stmt_rollback::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  //if (cdp->ker_flags & KFL_DISABLE_ROLLBACK) 
  if (cdp->current_cursor!=NULL)  // probably not necessary
    { request_error(cdp, ROLLBACK_IN_CURSOR_CREATION);  return TRUE; }
  if (*sapo_name || sapo_target!=NULL)  // ROLLBACK TO SAVEPOINT
  { int sapo_num = get_sapo_num(cdp, sapo_target);
   // check to see if the savepoint already exists:
    int ind = cdp->savepoints.find_sapo(sapo_name, sapo_num);
    if (ind==-1) // savepoint not found
      return request_error(cdp, SQ_SAVEPOINT_INVAL_SPEC);  // may be handled
   // check to see if not going outside atomic block:
    int at_ind = cdp->savepoints.find_atomic_index();
    if (at_ind != -1 && at_ind > ind)
      return request_error(cdp, SQ_SAVEPOINT_INVAL_SPEC);  // may be handled
   // rollback the savepoint:
    cdp->savepoints.rollback(cdp, ind);
   // destroy the later savepoints:
    cdp->savepoints.destroy(cdp, ind+1, TRUE);
    return FALSE;
  }

  else  // global ROLLBACK
  { if (cdp->savepoints.find_atomic_index() != -1 || cdp->in_trigger)
      return request_error(cdp, SQ_INVAL_TRANS_TERM);  // may be handled
    if (cdp->in_expr_eval)  // should be error, but I only treport it in order to remain compatible for some time
    { cd_dbg_err(cdp, "ROLLBACK inside an SQL expression is not allowed");
      return FALSE;
    }
    roll_back(cdp);
    if (chain) cdp->expl_trans=TRANS_EXPL;
  }
  return FALSE;
}

BOOL sql_stmt_set_trans::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  if (cdp->expl_trans==TRANS_EXPL) //warn(cdp, WAS_IN_TRANS);
    return request_error(cdp, SQ_TRANS_STATE_ACTIVE);  // may be handled
  cdp->isolation_level=isolation;
  cdp->trans_rw=rw;
  return FALSE;
}

BOOL sql_stmt_sapo::exec(cdp_t cdp)
// Creates or destroys and re-creates a savepoint
{ PROFILE_AND_DEBUG(cdp, source_line);
  int sapo_num = get_sapo_num(cdp, sapo_target);

 // check to see if the savepoint already exists:
  int ind = cdp->savepoints.find_sapo(sapo_name, sapo_num);
  if (ind!=-1) // savepoint found
    cdp->savepoints.destroy(cdp, ind, FALSE); // savepoint number will be preserved
  else // non-existing savepoint: check if the number is 0
    if (sapo_target!=NULL && sapo_num!=0)
      return request_error(cdp, SQ_SAVEPOINT_INVAL_SPEC);  // may be handled

 // create the savepoint:
  if (cdp->savepoints.create(cdp, sapo_name, sapo_num, FALSE)) return TRUE;
  if (sapo_target!=NULL)
    set_sapo_num(cdp, sapo_target, sapo_num); // sapo_num changed above
  return FALSE;
}

BOOL sql_stmt_rel_sapo::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  int sapo_num = get_sapo_num(cdp, sapo_target);
 // check to see if the savepoint already exists:
  int ind = cdp->savepoints.find_sapo(sapo_name, sapo_num);
  if (ind==-1) // savepoint not found
    return request_error(cdp, SQ_SAVEPOINT_INVAL_SPEC);  // may be handled

 // destroy the savepoint:
  cdp->savepoints.destroy(cdp, ind, TRUE);
  return FALSE;
}


BOOL sql_stmt_::exec(cdp_t cdp)
{ PROFILE_AND_DEBUG(cdp, source_line);
  return TRUE;
}

/////////////////////////////// extlibs /////////////////////////////////////
#if defined( WINS) || defined (LINUX)

#ifdef LINUX
#include <dlfcn.h>

/* Ensuring only libraries from valid paths are opened on Linux:
 * If the name of the library starts with '/', it is considered to be
 * absolute path, and the same mechanism as in Win can be used - however,
 * since SQL commands using that wont be portable, it should not be used and
 * it is disabled now as long as I find the reason why allow it.
 *
 * Otherwise, all allowed paths are prepended to it, and the opening is tried
 * with full path obtained that way. In this case, we do not need check for
 * valid directories later again.
 *
 * 20.12.2000 Possible final .dll is stripped from the library name.
 */
 
#ifndef RTLD_DEEPBIND  // not defined in older versions
#define RTLD_DEEPBIND 0
#endif 
 
HINSTANCE LoadLibraryInDirs(const char * name)
{
    int len=strlen(name);
    if      (len>4 && !strcmp(name+len-4, ".dll")) len-=4;
    else if (len>3 && !strcmp(name+len-3, ".so" )) len-=3;
    const char *str=NULL;
    while ((str=the_sp.dll_directory_list.get_next_string(str))){
	    char buffer[400];
	    snprintf(buffer, sizeof(buffer), "%s/%.*s.so", str, len, name);
        //fprintf(stderr, "Loading %s\n", buffer);
	    HINSTANCE h=dlopen(buffer, RTLD_LAZY|RTLD_DEEPBIND);
		if (h) return h;
        //fprintf(stderr, "%s\n", dlerror()); -- generates strange errors in the server console whenever aaa.bbb cannot be compiled
    }
    return NULL;
}

#endif // LINUX

// Loaded libraries must not be released because t_routines contain pointers to external procedures in them

class t_extlibs
{ libs_dynar libs;  // cell empty iff *name==0
  HINSTANCE get_lib(cdp_t cdp, char * name);  // 0-error handled, 1-error, other-handle
 public:
  void unload_all(void);
  FARPROC get_proc(cdp_t cdp, char * libname, const char * procname, int parsize=0);  // 0-error handled, 1-error, other-pointer

  ~t_extlibs(void) 
  { // unload_all(); -- must not be called for global class, dynar memory already dellocated and overwritten!!
  }
  void mark_core(void)
    { libs.mark_core(); }
};

void t_extlibs::unload_all(void)
{ for (int i=0;  i<libs.count();  i++)
  { t_library * lib = libs.acc0(i);
    if (lib->name[0])
      { FreeLibrary(lib->hLibInst);  lib->name[0]=0; }
  }
}

#if WBVERS<900
#ifdef WINS
#define LIB_SUFFIX      ".dll"
#define LIB_SUFFIX_LEN  4
#else
#define LIB_SUFFIX      ".so"
#define LIB_SUFFIX_LEN  3
#endif

static void translate(char * name, const char * origname, const char * newname)
// Replaces library name in [name]
// Supposes strlen(origname) >= strlen(newname) !!
{ int len=strlen(name);
  int lnm = strlen(origname);
  if (len >= lnm && !stricmp(name+len-lnm, origname))
    { strcpy(name+len-lnm, newname);  return; }
  if (len >= lnm+LIB_SUFFIX_LEN && !stricmp(name+len-LIB_SUFFIX_LEN, LIB_SUFFIX))
    if (!strnicmp(name+len-lnm-LIB_SUFFIX_LEN, origname, lnm))
      { strcpy(name+len-lnm-LIB_SUFFIX_LEN, newname);  strcat(name, LIB_SUFFIX);  return; }
}
#endif

HINSTANCE t_extlibs::get_lib(cdp_t cdp, char * name)
// Returns handle of the loaded library or loads it and registers (unless no memory).
// On Linux, the library name may be searched with a lower case name, when it is not found with the original case.
{ ProtectedByCriticalSection cs(&cs_medium_term);
 // look for the library, it may have already been loaded:
  int i, freecell=-1;  t_library * lib;
  for (i=0;  i<libs.count();  i++)
  { lib = libs.acc0(i);
#ifdef WINS
    if (!stricmp(name, lib->name)) 
#else
    if (!strcmp(name, lib->name)) 
#endif
      return lib->hLibInst;
    else if (!lib->name[0]) freecell=i;
  }
 // library is not loaded:
#if WBVERS<900
  translate(name, "wbkernel", "602krnl8");
  translate(name, "wbprezen", "602prez8");
  translate(name, "wbviewed", "602dvlp8");
#endif

#ifdef LINUX  // when library not found, try a lower-case name, but store the original names in [libs]
  HINSTANCE hInst = LoadLibraryInDirs(name);
  if (!hInst) 
  { char lcname[strlen(name)+1];
    strcpy(lcname, name);
    for (int i=0; lcname[i];  i++) lcname[i]=tolower(lcname[i]);
    if (strcmp(name, lcname)) // if the original [name] was not lower case
      hInst = LoadLibraryInDirs(lcname);  //  .. try again
  }
  if (!hInst) 
    { SET_ERR_MSG(cdp, name);  return (HINSTANCE)request_error(cdp, LIBRARY_NOT_FOUND); }
#else
  HINSTANCE hInst = LoadLibrary(name);
  if (!hInst) 
    { SET_ERR_MSG(cdp, name);  return (HINSTANCE)(size_t)request_error(cdp, LIBRARY_NOT_FOUND); }
 // on Windows, check the directory of the library (on Linux the library is searched in the allowed directories only)
  char pathname[MAX_PATH]; 
  GetModuleFileName(hInst, pathname, sizeof(pathname));
  pathname[get_last_separator_pos(pathname)]=0;
 // search in the directory list:
  if (!*pathname || !the_sp.dll_directory_list.is_in_list(pathname))  // the search ignores the case
    { FreeLibrary(hInst);  return (HINSTANCE)(size_t)request_error(cdp, LIBRARY_ACCESS_DISABLED); }
#endif
 // register the library:
  if (freecell==-1) lib = libs.next();
  else              lib = libs.acc0(freecell);
  if (lib!=NULL)
    { strmaxcpy(lib->name, name, sizeof(lib->name));  lib->hLibInst=hInst; }
  return hInst;
}

FARPROC t_extlibs::get_proc(cdp_t cdp, char * libname, const char * procname, int parsize)
{ HINSTANCE hLibInst = get_lib(cdp, libname);
  if (!hLibInst) return (FARPROC)0;               // error requested, handled
  if (hLibInst==(HINSTANCE)1) return (FARPROC)1;  // error requested, not handled
  FARPROC procadr=GetProcAddress(hLibInst, procname);
#ifdef WINS
  if (procadr==NULL)  /* try the alternative names, add A or _...@num */
  { int len=(int)strlen(procname);
    tptr alt_name=(tptr)corealloc(1+len+1+1+4+1, 53);
    if (alt_name!=NULL)
    { BOOL try_suffix = len && procname[len-1]!='A' && procname[len-1]!='W';
      if (try_suffix)
      { strcpy(alt_name, procname);  strcat(alt_name, "A");
        procadr=GetProcAddress(hLibInst, alt_name);
      }
      if (procadr==NULL)
      { *alt_name='_';  strcpy(alt_name+1, procname);  strcat(alt_name, "@");
        int2str(parsize, alt_name+len+2);
        procadr=GetProcAddress(hLibInst, alt_name);
        if (procadr==NULL && try_suffix)
        { strcat(alt_name, "A");
          procadr=GetProcAddress(hLibInst, alt_name);
        }
      }
      corefree(alt_name);
    }
  }
#endif
  if (procadr==NULL) return (FARPROC)(size_t)request_error(cdp, SQ_EXT_ROUT_NOT_AVAIL);  // may he handled
  return procadr;
}


t_extlibs extlibs;  // server global, used by triggers & procedure cache

void extlibs_mark_core(void)
{ extlibs.mark_core(); }

#endif
//////////////////////////////// t_routine //////////////////////////////////
t_routine::~t_routine(void)
{ delete stmt;  corefree(names); }

void t_routine::mark_core(void)  // marks itself, too
{ if (stmt) stmt->mark_core();
  if (names) mark(names);
  scope.mark_core();
  mark(this);
}

FARPROC t_routine::proc_ptr(cdp_t cdp)
// Returns procedure ptr or NULL iff library or proc not found.
{ 
#if defined( WINS )|| defined(UNIX)
  if (procptr==NULL || procptr==(FARPROC)1) // not bound yet
  { tptr libname = names+strlen(names)+1;
#ifdef WINS
    int parsize = external_frame_size;  //scope.extent;
    if (parsize % sizeof(int))  
      parsize = (parsize / sizeof(int) + 1) * sizeof(int);
    procptr=extlibs.get_proc(cdp, *libname ? libname : "USER32.DLL", names, parsize);
#else
    //for (char *ptr=libname;*ptr!='\0';ptr++) *ptr=tolower(*ptr); -- moved to get_lib(), trying the original case first
    procptr=extlibs.get_proc(cdp, libname, names); 
#endif
  }
#else // NLM
  procptr=(FARPROC)ImportSymbol(GetNLMHandle(), names);
  if (!procptr) request_error(cdp, SQ_EXT_ROUT_NOT_AVAIL);  
#endif
  return procptr;
}

void t_routine::free_proc_ptr(void)
{ 
#ifndef WINS
#ifndef UNIX
  UnimportSymbol(GetNLMHandle(), names); 
  procptr=NULL;
#endif
#endif
}

///////////////////////////// sequence cache //////////////////////////////
// AXIOM: the current value is the next value to be allocated. 
// It is a valid value between minval and maxval or the cache is empty (spaces)

// Load the new cached values has to be done on the cds[0] account so that it cannot be rolled back!!!

class t_seq_cache  // This class represents a compiled sequence with (possibly) some cached values.
{ t_sequence seq; // compiled sequence
  int first_ind, after_ind;
 public:
  t_seq_cache * next;
  BOOL load_values(cdp_t cdp);
  void save_values(cdp_t cdp);
  BOOL next_value(cdp_t cdp, t_seq_value * val);
  inline BOOL empty(void) { return first_ind==after_ind; }
  t_seq_cache(t_sequence *seqIn)
    { seq=*seqIn; first_ind=after_ind=0; }
  void * operator new(size_t size, unsigned cache_size);
#ifdef WINS
  void operator delete(void * ptr, unsigned cache_size);
#endif
#if !defined(WINS) || defined(X64ARCH)
  void operator delete(void * ptr);
#endif
  inline tobjnum seq_obj_num(void) { return seq.objnum; }
 private:
  t_seq_value values[1];  // array is bigger, must be on the end!
};

static t_seq_cache * seq_cache = NULL;  // list of compiled sequences with (possibly) cached values
CRITICAL_SECTION cs_sequences;          // protects access to [seq_cache]

void * t_seq_cache::operator new(size_t size, unsigned cache_size)
  { return corealloc(size+sizeof(t_seq_value)*(cache_size-1), 44); }

#ifdef WINS
void t_seq_cache::operator delete(void * ptr, unsigned cache_size)
  { corefree(ptr); }
#endif

#if !defined(WINS) || defined(X64ARCH)
void t_seq_cache::operator delete(void * ptr)
  { corefree(ptr); }
#endif

BOOL t_seq_cache::load_values(cdp_t cdp)
{ if (wait_record_lock_error(cdp, objtab_descr, seq.objnum, TMPWLOCK)) return TRUE;
 // read the beginning the the sequence definition:
  char buf[40];
  if (tb_read_var(cdp, objtab_descr, seq.objnum, OBJ_DEF_ATR, 0, sizeof(buf), buf)) return TRUE;
  tptr aux=buf, p;
  p=aux;
 // read the current value:
  tptr start;  BOOL minus=FALSE, digit=FALSE;  t_seq_value val=0;
  if (*p=='{')  // read the current value
  { p++;  start=p;
    while (*p==' ') p++;
    if (*p=='-') { p++;  minus=TRUE; }
    while (*p>='0' && *p<='9')
      { val=10*val+*p-'0';  p++;  digit=TRUE; }
    while (*p==' ') p++;
    if (minus) val=-val;
  }
  else 
    return FALSE;  // syntax error in the sequence, must prevent working with it (or correct it)
  if (!digit) // sequence position is not stored
    if (!memcmp(start, "EXH", 3))  // sequence has been exhausted in previous operations
      return FALSE;  // error will be generated outside
    else
      val=seq.startval; // sequence imported without the current value and shall be started from the start value
 // fill the cache:
  BOOL exh=FALSE;  int i;
  for (i=0;  i<seq.cache && !exh;  i++) 
  { values[i]=val;
    val+=seq.step;  
    if (seq.step>0)
    { if (val>seq.maxval) 
        if (seq.cycles) val=seq.minval;
        else exh=TRUE;  
    }
    else
    { if (val<seq.minval) 
        if (seq.cycles) val=seq.maxval;
        else exh=TRUE;  
    }
  }
  first_ind=0;  after_ind=i;
 // write the new current value to the definition:
  if (exh) strcpy(start, "EXH");
  else int64tostr(val, start);  
  start+=strlen(start); 
  memset(start, ' ', p-start);
  if (tb_write_var(cdp, objtab_descr, seq.objnum, OBJ_DEF_ATR, 0, (int)(p-aux), aux)) return TRUE;
 // if (empty()) error will be generated outside
  return FALSE;
}

void t_seq_cache::save_values(cdp_t cdp)
// Returns the unused sequence values from the cache to the saved position
{ if (empty()) return;
  if (wait_record_lock_error(cdp, objtab_descr, seq.objnum, TMPWLOCK)) return;
 // read the beginning the the sequence definition:
  char buf[40];
  if (tb_read_var(cdp, objtab_descr, seq.objnum, OBJ_DEF_ATR, 0, sizeof(buf), buf)) return;
  tptr aux=buf, p;
  p=aux;
 // find the value space:
  tptr start;  
  if (*p!='{') return;
  p++;  start=p;
  while (*p==' ') p++;
  if (*p=='-') p++;
  while (*p>='0' && *p<='9') p++;  
  while (*p==' ') p++;
 // write the first allocated unused value:
  int64tostr(values[first_ind], start);  start+=strlen(start); 
  if (p>start) memset(start, ' ', p-start);
  tb_write_var(cdp, objtab_descr, seq.objnum, OBJ_DEF_ATR, 0, (int)(p-aux), aux);
  first_ind=after_ind=0;
}

BOOL t_seq_cache::next_value(cdp_t cdp, t_seq_value * val)
/* New values are allocated in the cds[0] context in order to be able to terminate immediately
   the transaction overwriting the sequence definition in OBJTAB. Otherwise the following deadlock
   is possible:
   1st client allocated a value and has TMPWLOCK on OBJTAB record
   2nd client wants to allocate a value from the same sequence and waits inside [cs_sequences] for 
     the TMPWLOCK on OBJTAB record
   1st client wants to perform any sequence operation and waits for access into [cs_sequences].
*/
{ if (empty())
  { //if (seq.cache>1)
    { cds[0]->request_init();
      BOOL err = load_values(cds[0]);
      commit(cds[0]);
      if (cds[0]->is_an_error())
        request_error(cdp, cds[0]->get_return_code());
      cds[0]->request_init();
      if (err) return TRUE;
    }
    //else -- this easily causes a deadlock
    //  if (load_values(cdp)) return TRUE; // error reported
    if (empty()) { SET_ERR_MSG(cdp, seq.name);  request_error(cdp, SEQUENCE_EXHAUSTED);  return TRUE; }
  }
  *val=values[first_ind++];
  return FALSE;
}

static BOOL get_next_sequence_value(cdp_t cdp, tobjnum seq_obj, t_seq_value * val)
// If the sequence is not in the list of compiled sequences, inserts it there.
// Then gets the next sequence value.
{ ProtectedByCriticalSection cs(&cs_sequences, cdp, WAIT_CS_SEQUENCES);
 // find the sequence cache:
  t_seq_cache * sc = seq_cache;
  while (sc && sc->seq_obj_num()!=seq_obj) sc=sc->next;
 // create the sequence cache if it does not exist:
  if (!sc)
  {// check if the sequence object still exists: 
    tcateg categ;
    if (table_record_state(cdp, objtab_descr, seq_obj)!=NOT_DELETED ||
        tb_read_atr(cdp, objtab_descr, seq_obj, OBJ_CATEG_ATR, (tptr)&categ) ||
        categ!=CATEG_SEQ)
      { SET_ERR_MSG(cdp, "<sequence>");  request_error(cdp, OBJECT_NOT_FOUND);  return TRUE; }
   // loadand compile the sequence:
    t_sequence seq;
    tptr src=ker_load_objdef(cdp, objtab_descr, seq_obj);
    if (src==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
    seq.modifying=FALSE;  seq.recursive=FALSE;
    if (!compile_sequence(cdp, src, &seq)) { corefree(src);  return TRUE; }
    corefree(src);
   // create the new sequence cache and insert it into the list:
    seq.objnum=seq_obj;
    sc=new(seq.cache) t_seq_cache(&seq);
    if (sc==NULL) 
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  
      return TRUE; 
    }
    sc->next=seq_cache;
    seq_cache=sc;
  }
 // get the next value from the sequence cache:
  BOOL res=sc->next_value(cdp, val);
  cdp->last_identity_value = *val;
  return res;
}

void close_sequence_cache(cdp_t cdp)
// Saves cached values of all sequences in the cache and deletes the cache
{ ProtectedByCriticalSection cs(&cs_sequences, cdp, WAIT_CS_SEQUENCES);
  while (seq_cache)
  { t_seq_cache * sc = seq_cache;
    sc->save_values(cdp);
    seq_cache=sc->next;
    delete sc;
  }
}

void mark_sequence_cache(void)
{ ProtectedByCriticalSection cs(&cs_sequences, cds[0], WAIT_CS_SEQUENCES);
  for (t_seq_cache * sc = seq_cache;  sc;  sc=sc->next)
    mark(sc);
}

BOOL get_own_sequence_value(cdp_t cdp, tobjnum seq_obj, int nextval, t_seq_value * val)
// Returns current (nextval==0) or next (nextval==1) sequence value for the calling user.
// For [nextval]==2 returns the current or next value based on statement counter.
{ t_seq_list * sl = cdp->seq_list;
 // look for the user's cache for the last sequence value:
  while (sl && sl->seq_obj!=seq_obj) sl=sl->next;
 // get the last sequence value:
  if (!nextval || nextval==2 && sl && sl->statement_cnt_val==cdp->statement_counter)  
  { if (!sl)
    { tobjname name;  tb_read_atr(cdp, objtab_descr, seq_obj, OBJ_NAME_ATR, name);
      SET_ERR_MSG(cdp, name);  request_error(cdp, NO_CURRENT_VAL);  return TRUE; 
    }
    *val=sl->val;
  }
 // get the next sequence value:
  else // nextval==TRUE
  { if (cdp->ker_flags & KFL_NO_NEXT_SEQ_VAL) *val=0;
    else if (get_next_sequence_value(cdp, seq_obj, val)) return TRUE;
   // create the user's cache for the last sequence value:
    if (!sl)
    { sl=new t_seq_list(seq_obj);
      if (!sl) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
      sl->next=cdp->seq_list;
      cdp->seq_list=sl;
    }
   // store the last sequence value in the user's cache:
    sl->statement_cnt_val=cdp->statement_counter;
    sl->val=*val;
  }
  return FALSE;
}

static void remove_own_sequence_value(cdp_t cdp, tobjnum seq_obj)
{ t_seq_list ** psl = &cdp->seq_list;
  while (*psl!=NULL)
  { t_seq_list * sl = *psl;
    if (sl->seq_obj==seq_obj) 
    { *psl=sl->next;  // removing from the list
      sl->next=NULL;
      delete sl;
      break;
    }
    psl=&sl->next;
  }
}

t_seq_list::~t_seq_list(void)
{ if (next) delete next; }

void reset_sequence(cdp_t cdp, tobjnum seq_obj) 
// Removes the sequence from the cache, called on DROP and ALTER
// Deletes the "curval"
{ ProtectedByCriticalSection cs(&cs_sequences, cdp, WAIT_CS_SEQUENCES);
 // remove from the server's sequence cache:
  t_seq_cache ** psc = &seq_cache;
  while (*psc!=NULL)
  { t_seq_cache * sc = *psc;
    if (sc->seq_obj_num()==seq_obj)
    { *psc=sc->next;  sc->next=NULL;  // removes sc and prevents cascade deleting when deleting sc
      delete sc;
      break;
    }
    psc=&sc->next;
  }
 // remove from the client's cache:
  remove_own_sequence_value(cdp, seq_obj);
}    

void save_sequence(cdp_t cdp, tobjnum seq_obj) // stores the sequence values from the cache
{ ProtectedByCriticalSection cs(&cs_sequences, cdp, WAIT_CS_SEQUENCES);
  for (t_seq_cache * sc = seq_cache;  sc!=NULL;  sc=sc->next)
    if (sc->seq_obj_num()==seq_obj)
    { sc->save_values(cdp);
      break;
    }
}
