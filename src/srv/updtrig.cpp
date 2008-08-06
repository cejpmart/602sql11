////////////////////////////////////// trigger access encapsulation /////////////////////////////////////

t_rscope * create_trigger_rscope(cdp_t cdp, table_descr * tbdf, t_atrmap_flex * usage_map)
{// find the (shared) trigger scope:
  t_shared_scope * scope = get_trigger_scope(cdp, tbdf);  // increases the reference counter in scope
  if (!scope) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
 // allocate the rscope:
  t_rscope * rscope = new(scope->extent) t_rscope(scope);  // shared!
  if (rscope==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  scope->Release();  return NULL; }
  rscope_init(rscope);  
 // create rscope usage map:
  usage_map->init(tbdf->attrcnt);
  return rscope;
}

t_rscope * t_tab_trigger::prep_rscope(cdp_t cdp, table_descr * tbdf, int trigger_type, t_atrmap_flex * usage_map)
// Creates usage_map which is common for all triggers of the given type, independent of [column_map].
{ t_rscope * rscope = create_trigger_rscope(cdp, tbdf, usage_map);
  usage_map->clear();
  for (int i=0;  i<tbdf->triggers.count();  i++)
  { t_trigger_type_and_num * trigger_type_and_num = tbdf->triggers.acc0(i);
    if (trigger_type_and_num->trigger_type & trigger_type)
    { t_trigger * trig = obtain_trigger(cdp, trigger_type_and_num->trigger_objnum);
      if (!trig) continue;
      usage_map->union_(&trig->rscope_usage_map);
      psm_cache.put(trig);
    }
  }
  return rscope;
}

t_rscope * t_tab_trigger::prep_fictive_rscope(cdp_t cdp, table_descr * tbdf, t_atrmap_flex * usage_map, bool * is_fictive)
// Creates empty rscope usable for debugging only.
{ t_rscope * rscope = create_trigger_rscope(cdp, tbdf, usage_map);
  // allocating space just to be sure that will not be overwritten somewhere
  *is_fictive=true;
  return rscope;
}

BOOL t_tab_trigger::prepare_rscopes(cdp_t cdp, table_descr * tbdf, t_trigger_events eventIn)
// Creates memory scopes for triggers on table [tbdf] called on [eventIn].
// Clears [column_map] - prepares it for calling add_column_to_map().
{ event=eventIn;
  tbdf->prepare_trigger_info(cdp);  // I need the trigger_map
  trigger_map=tbdf->trigger_map;
  if (event==TGA_INSERT)
  { if (trigger_map & TRG_BEF_INS_NEW) // allocate rscope for NEW values in BEFORE trigger
      bef_rscope = prep_rscope(cdp, tbdf, TRG_BEF_INS_ROW, &bef_rscope_usage_map);
    else
      bef_rscope = prep_fictive_rscope(cdp, tbdf, &bef_rscope_usage_map, &bef_rscope_is_fictive);
    if (!bef_rscope) return FALSE;
    aft_rscope = prep_fictive_rscope(cdp, tbdf, &aft_rscope_usage_map, &aft_rscope_is_fictive);
    if (!aft_rscope) return FALSE;
  }
  else if (event==TGA_UPDATE)
  { if (trigger_map & TRG_BEF_UPD_NEW) // allocate rscope for NEW values in BEFORE trigger
      bef_rscope = prep_rscope(cdp, tbdf, TRG_BEF_UPD_ROW, &bef_rscope_usage_map);
    else
      bef_rscope = prep_fictive_rscope(cdp, tbdf, &bef_rscope_usage_map, &bef_rscope_is_fictive);
    if (!bef_rscope) return FALSE;
    if (trigger_map & TRG_AFT_UPD_OLD) // allocate rscope for OLD values in AFTER trigger
      aft_rscope = prep_rscope(cdp, tbdf, TRG_AFT_UPD_ROW, &aft_rscope_usage_map);
    else
      aft_rscope = prep_fictive_rscope(cdp, tbdf, &aft_rscope_usage_map, &aft_rscope_is_fictive);
    if (!aft_rscope) return FALSE;
   // if updating, prepare the column map for registering updated columns:
    //if (trigger_map & (TRG_BEF_UPD_STM | TRG_AFT_UPD_STM | TRG_BEF_UPD_ROW | TRG_AFT_UPD_ROW))  -- map myst be init anyways because columns are added into it!
    { column_map.init(tbdf->attrcnt);
      column_map.clear();
    }
  }
  else  // TGA_DELETE
  { bef_rscope = prep_fictive_rscope(cdp, tbdf, &bef_rscope_usage_map, &bef_rscope_is_fictive);
    if (!bef_rscope) return FALSE;
    if (trigger_map & TRG_AFT_DEL_OLD) // allocate rscope for OLD values in AFTER trigger
      aft_rscope = prep_rscope(cdp, tbdf, TRG_AFT_DEL_ROW, &aft_rscope_usage_map);
    else
      aft_rscope = prep_fictive_rscope(cdp, tbdf, &aft_rscope_usage_map, &aft_rscope_is_fictive);
    if (!aft_rscope) return FALSE;
  }
  return TRUE;
}

BOOL t_tab_trigger::pre_row(cdp_t cdp, table_descr * tbdf, trecnum position, const t_vector_descr * vector)
{ cdp->except_type=EXC_NO;  cdp->except_name[0]=0;  //cdp->processed_except_name[0]=0; -- should not clear this when an UPDATE statement is executed in a handler
 // load OLD values to bef_rscope for UPDATE, write default values for INSERT:
  if (event==TGA_INSERT)
  { if (bef_rscope && !bef_rscope_is_fictive) 
    { if (bef_rscope_contains_values) rscope_clear(bef_rscope);  else bef_rscope_contains_values=TRUE;
      // new values are going to be loaded immediately
      rscope_set_null(bef_rscope);
      attribdef * att;  int i;
      tptr dest = bef_rscope->data;
      for (i=1, att=tbdf->attrs+1;  i<tbdf->attrcnt;  i++, att++)
      { if (bef_rscope_usage_map.has(i) && att->defvalexpr!=NULL && !(cdp->ker_flags & KFL_DISABLE_DEFAULT_VALUES)) // specified DEFAULT 
        { if (IS_HEAP_TYPE(att->attrtype))
          { set_default_value(cdp, (t_value*)dest, att, tbdf);
            dest+=sizeof(t_value);
          }
          else
          { t_value val;  int size;
            set_default_value(cdp, &val, att, tbdf);
            size=TYPESIZE(att);  if (is_string(att->attrtype)) size++;
            memcpy(dest, val.valptr(), size);
            dest+=size;
          }
        }
        else // skipping
          if (IS_HEAP_TYPE(att->attrtype))
            dest+=sizeof(t_value);
          else { dest+=TYPESIZE(att);  if (is_string(att->attrtype)) dest++; }
      } // cycle on columns
    } // bef_rscope exists
  } // INSERT
  else // load OLD values into rscopes (UPDATE, DELETE):
    if (aft_rscope || bef_rscope)
    { if (bef_rscope)
        if (bef_rscope_contains_values && !bef_rscope_is_fictive) rscope_clear(bef_rscope);  
        else bef_rscope_contains_values=TRUE;
      if (aft_rscope)
        if (aft_rscope_contains_values && !aft_rscope_is_fictive) rscope_clear(aft_rscope);  
        else aft_rscope_contains_values=TRUE; 
      if (load_record_to_rscope(cdp, tbdf, position, aft_rscope_is_fictive ? NULL : aft_rscope, &aft_rscope_usage_map, 
                                                     bef_rscope_is_fictive ? NULL : bef_rscope, &bef_rscope_usage_map)) 
        return TRUE;
    }
 // write the NEW values to the BEF copy (bef_rscope does not exist for DELETE):
  if (bef_rscope && !bef_rscope_is_fictive) 
  { t_struct_type * st = bef_rscope->sscope->locdecls.acc0(0)->var.stype;
    t_value val;  
    for (unsigned pos=0;  vector->colvals[pos].attr!=NOATTRIB;  pos++) 
    { const t_colval * colval = &vector->colvals[pos];  
      if (colval->table_number!=vector->sel_table_number) continue;
      const char * src;  unsigned size, offset;  // size & offset used by var-len values only
      const attribdef * att = tbdf->attrs+colval->attr;
      int tp=att->attrtype;  unsigned sz=TYPESIZE(att);  
      size=sz;  // preset for ATT_BINARY
     // take or calculate the new value:
      vector->get_column_value(cdp, colval, tbdf, att, src, size, offset, val);
     // write the value to the bef_rscope
      tptr ptr = bef_rscope->data + st->elems.acc0(colval->attr-1)->offset;
      if (is_string(tp))
      { memcpy(ptr, src, att->attrspecif.length);
        ptr[att->attrspecif.length]=0;
        if (att->attrspecif.wide_char) ptr[att->attrspecif.length+1]=0;
      }
      else if (tp==ATT_BINARY)
        memcpy(ptr, src, att->attrspecif.length);
      else if (IS_HEAP_TYPE(tp))
      { t_value * val = (t_value*)ptr;
        if (!val->allocate(offset+size))
        { memcpy(val->valptr()+offset, src, size);  // src may be NULL when size==0
          val->valptr()[offset+size]=val->valptr()[offset+size+1]=0;
          val->length=offset+size;
        }
      }
      else memcpy(ptr, src, tpsize[tp]);

      val.set_null();  // must release the possible indirect value
    }
  }

 // run BEF ROW triggers:
  if (event==TGA_INSERT)
  { if (trigger_map & TRG_BEF_INS_ROW) 
      if (execute_triggers(cdp, tbdf, TRG_BEF_INS_ROW, bef_rscope, position             )) return TRUE; 
  }
  else if (event==TGA_UPDATE)
  { if (trigger_map & TRG_BEF_UPD_ROW) 
      if (execute_triggers(cdp, tbdf, TRG_BEF_UPD_ROW, bef_rscope, position, &column_map)) return TRUE; 
  }
  else  // TGA_DELETE
  { if (trigger_map & TRG_BEF_DEL_ROW) 
      if (execute_triggers(cdp, tbdf, TRG_BEF_DEL_ROW, bef_rscope, position             )) return TRUE; 
  }
  return FALSE;
}

const char * t_tab_trigger::new_value(int attr)
{ t_struct_type * st = bef_rscope->sscope->locdecls.acc0(0)->var.stype;
  tptr ptr = bef_rscope->data + st->elems.acc0(attr-1)->offset;
  return ptr;
}

BOOL t_tab_trigger::post_row(cdp_t cdp, table_descr * tbdf, trecnum position)
{ if (event==TGA_INSERT)
  { if (trigger_map & TRG_AFT_INS_ROW)  // AFT INS ROW triggers:
      if (execute_triggers(cdp, tbdf, TRG_AFT_INS_ROW, aft_rscope, position             )) return TRUE;
  }
  else if (event==TGA_UPDATE)
  { if (trigger_map & TRG_AFT_UPD_ROW)  // AFT UPD ROW triggers:
      if (execute_triggers(cdp, tbdf, TRG_AFT_UPD_ROW, aft_rscope, position, &column_map)) return TRUE;
  }
  else  // TGA_DELETE
  { if (trigger_map & TRG_AFT_DEL_ROW) 
      if (execute_triggers(cdp, tbdf, TRG_AFT_DEL_ROW, aft_rscope, position             )) return TRUE; 
  }
  return FALSE;
}

//////////////////////////////// t_ins_upd_trig_supp ////////////////////////////////////////////////
BOOL t_ins_upd_trig_supp::start_columns(unsigned column_count)
{ column_space=column_count;
  colvals=(t_colval*)corealloc(sizeof(t_colval)*(column_space+1), 43);
  if (!colvals) return FALSE;
  colval_pos=0;
  colvals[colval_pos].attr=NOATTRIB;  // the destructor needs the colvals array always to be terminated!
  return TRUE;
}

BOOL t_ins_upd_trig_supp::register_column(int colnum, db_kontext * kont, t_express * indexexpr)
{ if (colval_pos >= column_space) // not probable, more SET columns than kontext columns (is possible!)
  { column_space=colval_pos+3;
    colvals=(t_colval*)corerealloc(colvals, sizeof(t_colval)*(column_space+1));  // cannot return NULL because allocating same or less size!
    if (!colvals) return FALSE;
  }
  colvals[colval_pos+1].attr=NOATTRIB;  // the destructor needs the colvals array always to be terminated!
  colvals[colval_pos].index    =indexexpr;   // must be be defined, may be destructed at any time
  colvals[colval_pos].dataptr  =NULL;
  colvals[colval_pos].lengthptr=NULL;
  colvals[colval_pos].offsetptr=NULL;
  colvals[colval_pos].expr     =NULL;
  if (!kont) 
  { colvals[colval_pos].attr=colnum;
    colvals[colval_pos].table_number=0;
  }
  else
  { db_kontext * _kont;  int _elemnum;
    if (!resolve_access(kont, colnum, _kont, _elemnum))
      return FALSE;
    colvals[colval_pos].attr=_elemnum;
    colvals[colval_pos].table_number=_kont->ordnum;
  }
  return TRUE;
}

void t_ins_upd_trig_supp::stop_columns(void)
{ if (colval_pos+1 < column_space)
    colvals=(t_colval*)corerealloc(colvals, sizeof(t_colval)*(colval_pos+1));  // cannot return NULL because allocating same or less size!
  colvals[colval_pos].attr=NOATTRIB;  // reallocation may have removed the terminator
}

// I suppose that index of a table in cd->tabnums[] is the same as its ordnum.
BOOL t_ins_upd_trig_supp::init_ttrs(cdp_t cdp, table_descr * tbdf, cur_descr * cd, BOOL inserting)
{ if (!is_init)
  { if (!cd) table_count=1;
    else table_count=cd->tabcnt;  //cd->qe->mat_extent;
    ttrs=new t_ins_upd_tab[table_count];  // must run constructors!  // (t_ins_upd_tab*)corealloc(sizeof(t_ins_upd_tab)*table_count, 43);
    if (!ttrs) return FALSE;
   // prepare rscopes:
    if (cd)
    { for (int i=0;  i<table_count;  i++)
      { ttablenum tb = cd->tabnums[i];
        if (tb>=0 && tb!=(ttablenum)0x8000 && tb!=(ttablenum)0x8001 && tb!=(ttablenum)0x8002)
          if (!ttrs[i].ttr.prepare_rscopes(cdp, tables[tb], inserting ? TGA_INSERT : TGA_UPDATE))
            return FALSE;
      }
    }
    else // single-table operation
      if (!ttrs[0].ttr.prepare_rscopes(cdp, tbdf, inserting ? TGA_INSERT : TGA_UPDATE))
        return FALSE;
   // add columns to the column_maps of its tables:
    if (!inserting)
    { t_colval * colval = colvals;
      while (colval->attr!=NOATTRIB)
      { ttrs[colval->table_number].ttr.add_column_to_map(colval->attr);
        colval++;
      }
    }
   // finding table contexts for updatable tables (tabkont for non-updatable tables remains NULL):
    if (cd)
    { db_kontext * _kont;  int _elemnum;
      for (int colnum=1;  colnum<cd->d_curs->attrcnt;  colnum++)
        if (resolve_access(&cd->qe->kont, colnum, _kont, _elemnum))
          ttrs[_kont->ordnum].tabkont=_kont;
    }
    is_init=TRUE;
  }
  else  // ATTN: when is_init is true, trigger info may have been destroyed after after ttrs was init
    if (tbdf)
      tbdf->prepare_trigger_info(cdp);  // I need the trigger_map
    else
      for (int i=0;  i<table_count;  i++)
      { ttablenum tb = cd->tabnums[i];
        if (tb>=0 && tb!=(ttablenum)0x8000 && tb!=(ttablenum)0x8001 && tb!=(ttablenum)0x8002)
          tables[tb]->prepare_trigger_info(cdp);  // I need the trigger_map
      }
  return TRUE;
}

BOOL t_ins_upd_trig_supp::pre_stmt_cursor(cdp_t cdp, cur_descr * cd)
{ for (int i=0;  i<table_count;  i++)
  { ttablenum tb = cd->tabnums[i];
    if (tb>=0 && tb!=(ttablenum)0x8000 && tb!=(ttablenum)0x8001 && tb!=(ttablenum)0x8002)
      if (ttrs[i].ttr.pre_stmt(cdp, tables[tb])) return TRUE;
  }
  return FALSE;
}

BOOL t_ins_upd_trig_supp::post_stmt_cursor(cdp_t cdp, cur_descr * cd)
{ for (int i=0;  i<table_count;  i++)
  { ttablenum tb = cd->tabnums[i];
    if (tb>=0 && tb!=(ttablenum)0x8000 && tb!=(ttablenum)0x8001 && tb!=(ttablenum)0x8002)
      if (ttrs[i].ttr.post_stmt(cdp, tables[tb])) return TRUE;
  }
  return FALSE;
}

trecnum t_ins_upd_trig_supp::execute_insert(cdp_t cdp, table_descr * tbdf, BOOL append_it)
{ t_vector_descr vector(colvals); 
  return tb_new(cdp, tbdf, append_it, &vector, &ttrs->ttr); 
}
BOOL t_ins_upd_trig_supp::execute_write_table(cdp_t cdp, table_descr * tbdf, trecnum recnum) // does not check privils
{ t_vector_descr vector(colvals); 
  return tb_write_vector(cdp, tbdf, recnum, &vector, FALSE, &ttrs->ttr);
}
BOOL t_ins_upd_trig_supp::execute_write_cursor(cdp_t cdp, cur_descr * cd, trecnum recnum)
{ t_vector_descr vector(colvals); 
  trecnum recnums[MAX_CURS_TABLES];
  cd->curs_seek(recnum, recnums);
  { for (int i=0;  i<table_count;  i++)
    { ttablenum tb = cd->tabnums[i];
      if (tb!=(ttablenum)0x8000 && tb!=(ttablenum)0x8001 && tb!=(ttablenum)0x8002)  // must be done for local tables too!
        if (ttrs[i].tabkont!=NULL)  // the table is updatable (not UNION etc.)
        { vector.sel_table_number=i;
          if (recnums[i]!=OUTER_JOIN_NULL_POSITION)
            if (tb_write_vector(cdp, tables[tb], recnums[i], &vector, FALSE, &ttrs[i].ttr) || cdp->is_an_error() || cdp->except_type!=EXC_NO)
            // user exception in the trigger should stop processing becaise it cannot be handled any more and may be erase in the pre_row of the next table and then replaced by other error
              return TRUE;
        }
    }
  }
  return FALSE;
}

int t_ins_upd_trig_supp::is_globally_updatable_in_table(cdp_t cdp, table_descr * tbdf, db_kontext * kont)
// 0 no and no records privils, 1 yes for all records, 2 depends on record privils
// The result must not be cached in the compiled statement because it depends on admin_mode.
{ bool all_global_ok = true;
  if (tbdf->tbnum < 0) return 1;  // temporary tables may not have kont->privil_cache
  if (!cdp->has_full_access(tbdf->schema_uuid)) // unless admin mode
    for (t_colval * colval = colvals;  colval->attr!=NOATTRIB;  colval++)
      if (!kont->privil_cache->t_privil_cache::can_write_column(cdp, colval->attr, tbdf, NORECNUM))
        { all_global_ok = false;  break; }
  return all_global_ok ? 1 : tbdf->rec_privils_atr!=NOATTRIB ? 2 : 0;
}

bool t_ins_upd_trig_supp::is_record_updatable_in_table(cdp_t cdp, table_descr * tbdf, db_kontext * kont, trecnum recnum)
{ for (t_colval * colval = colvals;  colval->attr!=NOATTRIB;  colval++)
    if (!kont->privil_cache->can_write_column(cdp, colval->attr, tbdf, recnum))
      return false;
  return true;
}

int t_ins_upd_trig_supp::is_globally_updatable_in_cursor(cdp_t cdp, cur_descr * cd)
// 0 no and no records privils, 1 yes for all records, 2 depends on record privils
// The result must not be cached in the compiled statement because it depends on admin_mode.
// Does not check the columns which are not updatable (and cannot be used in the update statement)
{ bool all_global_ok = true, all_non_global_can_have_local = true;
  for (t_colval * colval = colvals;  colval->attr!=NOATTRIB;  colval++)
  { db_kontext * kont = ttrs[colval->table_number].tabkont;
    if (kont)  // unless the column of the cursor is not updatable
    { ttablenum tb = cd->tabnums[colval->table_number];
      if (tb!=(ttablenum)0x8000 && tb!=(ttablenum)0x8001 && tb!=(ttablenum)0x8002)
      { table_descr * tbdf = tables[tb];
        if (tbdf->tbnum>=0 && !cdp->has_full_access(tbdf->schema_uuid)) // unless temp table or admin mode
          if (!kont->privil_cache->t_privil_cache::can_write_column(cdp, colval->attr, tbdf, NORECNUM))
          { all_global_ok = false; 
            if (tbdf->rec_privils_atr==NOATTRIB) all_non_global_can_have_local=false;
          }
      }
    }
  }
  return all_global_ok ? 1 : all_non_global_can_have_local ? 2 : 0;
}

bool t_ins_upd_trig_supp::is_record_updatable_in_cursor(cdp_t cdp, cur_descr * cd, trecnum recnum)
{ trecnum recnums[MAX_CURS_TABLES];
  cd->curs_seek(recnum, recnums);
  for (t_colval * colval = colvals;  colval->attr!=NOATTRIB;  colval++)
  { db_kontext * kont = ttrs[colval->table_number].tabkont;
    if (kont)  // unless the column of the cursor is not updatable
    { ttablenum tb = cd->tabnums[colval->table_number];
      if (tb!=(ttablenum)0x8000 && tb!=(ttablenum)0x8001 && tb!=(ttablenum)0x8002)
        if (!kont->privil_cache->can_write_column(cdp, colval->attr, tables[cd->tabnums[colval->table_number]], recnums[colval->table_number]))
          return false;
    }
  }
  return true;
}

bool t_ins_upd_trig_supp::store_data(int pos, char * dataptr, unsigned size, bool appending)  // appends data to a var-len buffer
{ t_colval * colval = &colvals[pos];
  int len = appending ? *colval->lengthptr : 0;
  char * newbuf = (char*)corealloc(size+len+2, 47);
  if (!newbuf) return false;
  if (len) memcpy(newbuf, colval->dataptr, len);
  memcpy(newbuf+len, dataptr, size);
  newbuf[len+size]=0;
  if (colval->dataptr) corefree(colval->dataptr);
  colval->dataptr=newbuf;
  *colval->lengthptr=len+size;
  return true;
}

char * t_ins_upd_trig_supp::get_dataptr(int pos)
{ 
  return (char*)colvals[pos].dataptr; 
}

