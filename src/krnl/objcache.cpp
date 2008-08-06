#ifdef CLIENT_GUI
// object name cache and persistent object definitions cache

CFNC DllKernel void WINAPI deliver_messages(cdp_t cdp, BOOL waitcurs)
// For clients only
{ MSG msg;  HCURSOR oldcurs;
  if (task_switch_enabled<1) return;
  wsave_run(cdp);
  oldcurs=GetCursor();
  while (PeekMessage(&msg, 0, 0, WM_PAINT-1, PM_REMOVE) ||
         PeekMessage(&msg, 0, WM_PAINT+1, 0xffff, PM_REMOVE))
  { if ((msg.message!=WM_KEYDOWN) && (msg.message!=WM_KEYUP))
    { TranslateMessage(&msg);       /* Translates virtual key codes         */
      DispatchMessage(&msg);       /* Dispatches message to window         */
    }
  }
  SetCursor(oldcurs);
  wrestore_run(cdp);
}

//////////////////////////////// persistent cache ////////////////////////////////////////////////////
struct t_pcch_descr
{ BOOL  cached;
  BOOL  verified;
  uns32 mod_time;
  tobjname name;
  WBUUID   appl_uuid;
  tcateg   categ;
  uns8     nic;
  uns16    flags;
};

static void close_object_cache(cdp_t cdp)
{ if (!cdp->IStorageCache) return;
  cdp->IStorageCache->Release();
  cdp->IStorageCache=NULL;
}

static BOOL open_object_cache(cdp_t cdp)
{// check the caching flag:
  char key[160];  HKEY hKey;  DWORD key_len;  DWORD pcache_flag;  char path[MAX_PATH];
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, cdp->conn_server_name);
  pcache_flag=0;  *path=0;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)
  { key_len=sizeof(pcache_flag);
    if (RegQueryValueEx(hKey, Pcache_str, NULL, NULL, (BYTE*)&pcache_flag, &key_len)!=ERROR_SUCCESS)
      pcache_flag=0;
    key_len=sizeof(path);
    if (RegQueryValueEx(hKey, Pcchdir_str, NULL, NULL, (BYTE*)path, &key_len)!=ERROR_SUCCESS)
      *path=0;
    RegCloseKey(hKey);
  }
  if (!pcache_flag) return FALSE;
  if (!*path) GetTempPath(sizeof(path), path);
  int len = strlen(path);
  if (path[len-1]!='\\') path[len++]='\\';
  strcpy(path+len, cdp->conn_server_name);  
  for (int i=len;  path[i];  i++) 
    if (!(path[i]>='A' && path[i]<='Z' || path[i]>='a' && path[i]<='z' || path[i]>='0' && path[i]<='9' || path[i]=='-'))
      path[i]='_';
  strcat(path, ".CCH");
 // open the cache:
  HRESULT hres;  WCHAR wname[MAX_PATH];
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, path, strlen(path)+1, wname, MAX_PATH);
  hres=StgCreateDocfile(wname, 
    STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE/*DENY_NONE*/ | STGM_FAILIFTHERE, 0, &cdp->IStorageCache);
  if (hres==S_OK) // init the new pcache
  { IStream * objtab_stream;
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, "OBJTAB", 7, wname, MAX_PATH);
    hres=cdp->IStorageCache->CreateStream(wname, 
            STGM_CREATE | STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
            0, 0, &objtab_stream);
    if (hres==S_OK) objtab_stream->Release();
    else { close_object_cache(cdp);  return FALSE; }
  }
  else if (hres==STG_E_FILEALREADYEXISTS)
  { hres=StgOpenStorage(wname, NULL,
      STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE/*DENY_NONE*/, NULL, 0, &cdp->IStorageCache);
    if (hres==S_OK) // mark all objects as non-verified
    { IStream * objtab_stream;
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, "OBJTAB", 7, wname, MAX_PATH);
      hres=cdp->IStorageCache->OpenStream(wname, 0,
              STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
              0, &objtab_stream);
      if (hres!=S_OK) { close_object_cache(cdp);  return FALSE; }
     // get length: 
      STATSTG stat;  t_pcch_descr pcch_descr;
      if (objtab_stream->Stat(&stat, STATFLAG_NONAME) == S_OK)
        for (ULONGLONG offset=0;  offset<stat.cbSize.QuadPart;  offset+=sizeof(t_pcch_descr))
        { objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
          objtab_stream->Read(&pcch_descr, sizeof(t_pcch_descr), NULL);
          if (pcch_descr.cached && pcch_descr.verified) 
          { pcch_descr.verified=FALSE;
            objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
            objtab_stream->Write(&pcch_descr, sizeof(t_pcch_descr), NULL);
          }
        }
      objtab_stream->Release();
    }
  }
  if (hres!=S_OK) return FALSE;
  return TRUE;
}

#pragma pack(1)
struct t_objtab_rec
{ uns8 zcr[ZCR_SIZE];
  tobjname objname;
  tcateg   categ;
  WBUUID   appl_uuid;
  uns16    flags;
  tobjname folder_name;
  uns32    last_modif;
};
#pragma pack()

static tptr get_space_op(cdp_t cdp, unsigned size, uns8 op);
static BOOL WINAPI cond_send(cdp_t cdp);

static void make_object_stream_name(tobjnum objnum, WCHAR * wname)
{ char name[30];  int2str(objnum, name);
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, strlen(name)+1, wname, 30);
}

enum t_rw_oper_type { RW_OPER_FIXED, RW_OPER_VAR, RW_OPER_LEN };

static BOOL object_db_to_cache(cdp_t cdp, trecnum objnum, IStream * objtab_stream)
// Loads all object info from db to the cache
{ t_objtab_rec objrec;  tptr buf;  enum { LD_STEP=0xc000 };
 // read basic info:
  if (cd_Read_record(cdp, OBJ_TABLENUM, objnum, &objrec, sizeof(objrec))) return FALSE;
  buf=(tptr)corealloc(LD_STEP, 55);  if (!buf) return FALSE;
  t_pcch_descr pcch_descr;
  strcpy(pcch_descr.name, objrec.objname);
  memcpy(pcch_descr.appl_uuid, objrec.appl_uuid, UUID_SIZE);
  pcch_descr.categ=objrec.categ;  pcch_descr.flags=objrec.flags;  
  pcch_descr.cached=TRUE;  pcch_descr.verified=TRUE;
 // open object stream (destroy if already exists):
  IStream * obj_stream;  WCHAR wname[30];  make_object_stream_name(objnum, wname);
  HRESULT hres=cdp->IStorageCache->CreateStream(wname, 
            STGM_CREATE | STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
            0, 0, &obj_stream);
  if (hres!=S_OK) { corefree(buf);  return FALSE; }
 // reading objdef:
  uns32 start=0;  t_varcol_size rd_size;  
  do
  { tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof(tattrib)+10, OP_READ);
    if (p==NULL) return TRUE;
    *(tcursnum*)p=OBJ_TABLENUM;   p+=sizeof(tcursnum);
    *(trecnum*)p=objnum;          p+=sizeof(trecnum);
    *(tattrib*)p=OBJ_DEF_ATR;     p+=sizeof(tattrib);
    *(p++)=MODINT;
    *(uns32*)p=start;       p+=sizeof(uns32);
    *(uns32*)p=LD_STEP;     p+=sizeof(uns32);
    *(p++)=MODSTOP;
    cdp->ans_array[cdp->ans_ind]=&rd_size;
    cdp->ans_type [cdp->ans_ind]=sizeof(rd_size);
    cdp->ans_ind++;
    cdp->ans_array[cdp->ans_ind]=buf;
    cdp->ans_type [cdp->ans_ind]=ANS_SIZE_PREVITEM;
    cdp->ans_ind++;
    if (cond_send(cdp)) goto err;
   // get stamp:
    if (!start)
      if (pcch_descr.categ==CATEG_APPL  || pcch_descr.categ==CATEG_PICT || 
          pcch_descr.categ==CATEG_ROLE  || pcch_descr.categ==CATEG_CONNECTION ||
          pcch_descr.categ==CATEG_GRAPH || pcch_descr.categ==CATEG_PGMEXE)
        pcch_descr.mod_time=(uns32)-1;
      else
        pcch_descr.mod_time=objrec.last_modif;
   // save to cache:
    obj_stream->Write(buf, rd_size, NULL);
    start+=rd_size;  
  } while (rd_size==LD_STEP);
  corefree(buf);
  DWORDLONG longsize;  longsize = start;
  obj_stream->SetSize(*(ULARGE_INTEGER*)&longsize);
  obj_stream->Release();
 // expand the objtab_stream, if necessary:
  LONGLONG offset;  
  STATSTG stat;  t_pcch_descr zero_descr;  memset(&zero_descr, 0, sizeof(zero_descr));
  objtab_stream->Stat(&stat, STATFLAG_NONAME);
  for (offset=stat.cbSize.QuadPart;  offset<objnum*sizeof(t_pcch_descr);  offset+=sizeof(t_pcch_descr))
  { objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
    objtab_stream->Write(&zero_descr, sizeof(t_pcch_descr), NULL);
  }
 // write the descriptor:
  offset = objnum * sizeof(t_pcch_descr);
  objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
  objtab_stream->Write(&pcch_descr, sizeof(t_pcch_descr), NULL);
  return TRUE;
 err:
  corefree(buf);
  obj_stream->Release();
  return FALSE;
}

static BOOL make_cache_valid(cdp_t cdp, trecnum objnum, t_pcch_descr * pcch_descr, IStream * objtab_stream, t_rw_oper_type read_type);

static BOOL cache_verified(cdp_t cdp, t_pcch_descr * pcch_descr, trecnum objnum, IStream * objtab_stream)
{ if (pcch_descr->categ==CATEG_APPL  || pcch_descr->categ==CATEG_PICT || 
      pcch_descr->categ==CATEG_ROLE  || pcch_descr->categ==CATEG_CONNECTION)
    return FALSE;
  if (pcch_descr->categ==CATEG_GRAPH || pcch_descr->categ==CATEG_PGMEXE)
  { tobjnum master_obj;  
    if (cd_Find_object(cdp, pcch_descr->name, pcch_descr->categ==CATEG_PGMEXE ? CATEG_PGMSRC : CATEG_VIEW, &master_obj))
      return FALSE;  // master object not found -> not verified
   // validating master object puts verified copy of dependent object into the cache:
    t_pcch_descr master_pcch_descr;  
    return make_cache_valid(cdp, master_obj, &master_pcch_descr, objtab_stream, RW_OPER_LEN);
  }
  else
  { // read the name and the timestamp:
    tobjname name;  uns32 stamp;
    tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof(tattrib)+1+ 
                             1+sizeof(tcursnum)+sizeof(trecnum)+sizeof(tattrib)+1, OP_READ);
    if (p==NULL) return TRUE;
    *(tcursnum*)p=OBJ_TABLENUM;   p+=sizeof(tcursnum);
    *(trecnum*)p=objnum;          p+=sizeof(trecnum);
    *(tattrib*)p=OBJ_NAME_ATR;    p+=sizeof(tattrib);
    *(p++)=MODSTOP;
    *(p++)=OP_READ;
    *(tcursnum*)p=OBJ_TABLENUM;   p+=sizeof(tcursnum);
    *(trecnum*)p=objnum;          p+=sizeof(trecnum);
    *(tattrib*)p=OBJ_MODIF_ATR;   p+=sizeof(tattrib);
    *(p++)=MODSTOP;
    cdp->ans_array[cdp->ans_ind]=name;
    cdp->ans_type [cdp->ans_ind]=ANS_SIZE_VARIABLE;
    cdp->ans_ind++;
    cdp->ans_array[cdp->ans_ind]=&stamp;
    cdp->ans_type [cdp->ans_ind]=ANS_SIZE_VARIABLE;
    cdp->ans_ind++;
   // check to see if object has been changed:
    if (!cond_send(cdp)) 
      if (stamp==pcch_descr->mod_time && !strcmp(name, pcch_descr->name))
          return TRUE;
  }
  return FALSE;
}

static BOOL make_cache_valid(cdp_t cdp, trecnum objnum, t_pcch_descr * pcch_descr, IStream * objtab_stream, t_rw_oper_type read_type)
{ memset(pcch_descr, 0, sizeof(t_pcch_descr));  // clears "cached"
  LONGLONG offset = sizeof(t_pcch_descr) * objnum;
  objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
  objtab_stream->Read(pcch_descr, sizeof(t_pcch_descr), NULL);
  if (!pcch_descr->cached || !pcch_descr->verified) 
  { LONGLONG offset = sizeof(t_pcch_descr) * objnum;
    if (pcch_descr->cached && cache_verified(cdp, pcch_descr, objnum, objtab_stream))  // save the verified state to the cache
    { pcch_descr->verified=TRUE;
      objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
      objtab_stream->Write(pcch_descr, sizeof(t_pcch_descr), NULL);
    }
    else                 // refresh the cache contents
    { if (!object_db_to_cache(cdp, objnum, objtab_stream)) return FALSE; 
     // refresh the dependent object, if any:
      if (pcch_descr->categ==CATEG_VIEW || pcch_descr->categ==CATEG_PGMSRC)
      { tobjnum dep_obj;
        if (!cd_Find_object(cdp, pcch_descr->name, pcch_descr->categ==CATEG_PGMSRC ? CATEG_PGMEXE : CATEG_GRAPH, &dep_obj))
          object_db_to_cache(cdp, dep_obj, objtab_stream);
      }
      if (read_type==RW_OPER_FIXED)  // refresh the pcch_descr
      { objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
        objtab_stream->Read(pcch_descr, sizeof(t_pcch_descr), NULL);
      }
    }
  }
  return TRUE;
}

static BOOL read_pcached(cdp_t cdp, trecnum objnum, tattrib attr, void * buffer, t_io_oper_size start, t_io_oper_size size, t_io_oper_size * read, t_rw_oper_type read_type)
// Read hook for OBJTAB and attr!=DEL_ATTR_NUM
{ IStream * objtab_stream;
 // open the OBJTAB stream:
  WCHAR wname[10];
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, "OBJTAB", 7, wname, 10);
  HRESULT hres=cdp->IStorageCache->OpenStream(wname, 0,
          STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &objtab_stream);
  if (hres!=S_OK) return FALSE; 
 // get object descr: 
  t_pcch_descr pcch_descr;  
  if (!make_cache_valid(cdp, objnum, &pcch_descr, objtab_stream, read_type))
    { objtab_stream->Release();  return FALSE; }
 // read data from the cache or pcch_descr:
  if (read_type==RW_OPER_FIXED)
  { switch (attr)
    { case OBJ_NAME_ATR:
        strcpy((tptr)buffer, pcch_descr.name);  break;
      case OBJ_CATEG_ATR:
        *(tcateg*)buffer=pcch_descr.categ;  break;
      case APPL_ID_ATR:
        memcpy(buffer, pcch_descr.appl_uuid, UUID_SIZE);  break;
      case OBJ_FLAGS_ATR:
        *(uns16*)buffer=pcch_descr.flags;  break;
    }
  }
  else
  { IStream * obj_stream;  WCHAR wname[30];  make_object_stream_name(objnum, wname);
    hres=cdp->IStorageCache->OpenStream(wname, 0,
            STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
            0, &obj_stream);
    if (hres!=S_OK) { objtab_stream->Release();  return FALSE; }
    if (read_type==RW_OPER_LEN) // get length: 
    { STATSTG stat;  
      obj_stream->Stat(&stat, STATFLAG_NONAME);
      *(uns32*)buffer=(uns32)stat.cbSize.QuadPart;
    }
    else // read objdef (or its part):
    { LONGLONG def_offset = start;
      obj_stream->Seek(*(LARGE_INTEGER*)&def_offset, STREAM_SEEK_SET, NULL);
      obj_stream->Read(buffer, size, read);
    }
    obj_stream->Release();
  }
  objtab_stream->Release();
  return TRUE;
}

static void write_pcache(cdp_t cdp, trecnum objnum, tattrib attr, const void * buffer, uns32 start, uns32 size, t_rw_oper_type read_type)
// Read hook for OBJTAB and attr!=DEL_ATTR_NUM
{ IStream * objtab_stream;
 // open the OBJTAB stream:
  WCHAR wname[10];
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, "OBJTAB", 7, wname, 10);
  HRESULT hres=cdp->IStorageCache->OpenStream(wname, 0,
          STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
          0, &objtab_stream);
  if (hres!=S_OK) return; 
 // get object descr: 
  t_pcch_descr pcch_descr;  memset(&pcch_descr, 0, sizeof(pcch_descr));  // clears "cached"
  LONGLONG offset = sizeof(t_pcch_descr) * objnum;
  objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
  objtab_stream->Read(&pcch_descr, sizeof(t_pcch_descr), NULL);
  if (!pcch_descr.cached || !pcch_descr.verified) 
    { objtab_stream->Release();  return; }
 // write the new value to the cache:
  if (read_type==RW_OPER_FIXED)
  { switch (attr)
    { case OBJ_NAME_ATR:
        strcpy(pcch_descr.name, (tptr)buffer);  break;
      case OBJ_CATEG_ATR:
        pcch_descr.categ=*(tcateg*)buffer;  break;
      case APPL_ID_ATR:
        memcpy(pcch_descr.appl_uuid, buffer, UUID_SIZE);  break;
      case OBJ_FLAGS_ATR:
        pcch_descr.flags=*(uns16*)buffer;  break;
      case OBJ_DEF_ATR:  // called when writing lenght
        objtab_stream->Release();  return;
      case OBJ_MODIF_ATR:   
        pcch_descr.mod_time=*(uns32*)buffer;  break;
    }
    objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
    objtab_stream->Write(&pcch_descr, sizeof(t_pcch_descr), NULL);
  }
  else
  { IStream * obj_stream;  WCHAR wname[30];  make_object_stream_name(objnum, wname);
    hres=cdp->IStorageCache->OpenStream(wname, 0,
            STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
            0, &obj_stream);
    if (hres!=S_OK) { objtab_stream->Release();  return; }
    if (read_type==RW_OPER_LEN) // get length: 
    { DWORDLONG xsize = *(uns32*)buffer;
      obj_stream->SetSize(*(ULARGE_INTEGER*)&xsize);
    }
    else // write objdef (or its part):
    { LONGLONG def_offset = start;
      obj_stream->Seek(*(LARGE_INTEGER*)&def_offset, STREAM_SEEK_SET, NULL);
      obj_stream->Write(buffer, size, NULL);
    }
    obj_stream->Release();
  }
  objtab_stream->Release();
  return;
}

static void delete_pcache(cdp_t cdp, trecnum objnum)
// Delete on OBJTAB hook.
{ IStream * objtab_stream;
 // open the OBJTAB stream:
  WCHAR wname[30];
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, "OBJTAB", 7, wname, 10);
  HRESULT hres=cdp->IStorageCache->OpenStream(wname, 0,
          STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
          0, &objtab_stream);
  if (hres!=S_OK) return; 
 // clear the record in objtab_stream, if exists:
  ULONGLONG offset = sizeof(t_pcch_descr) * objnum;  STATSTG stat;  
  objtab_stream->Stat(&stat, STATFLAG_NONAME);
  if (offset<stat.cbSize.QuadPart)
  { t_pcch_descr zero_descr;  memset(&zero_descr, 0, sizeof(zero_descr));
    objtab_stream->Seek(*(LARGE_INTEGER*)&offset, STREAM_SEEK_SET, NULL);
    objtab_stream->Write(&zero_descr, sizeof(t_pcch_descr), NULL);
  }
 // delete object stream:
  make_object_stream_name(objnum, wname);
  cdp->IStorageCache-> DestroyElement(wname);
  objtab_stream->Release();
}
#endif

/************************** object caches & flags ***************************/
CFNC DllKernel BOOL WINAPI comp_dynar_search(t_dynar * d_comp, ctptr name, unsigned * pos)
// If found, returns TRUE and pos, otherwise returns FALSE and insert pos.
// name is supposed to be upper case.
{ unsigned start, stop, midd;  int res;
  start=0;  stop=d_comp->count();
  if (!stop) { *pos=0;  return FALSE; }
  while (start+1 < stop)
  { midd=(start+stop)/2;
    res=strcmp(((t_comp*)d_comp->acc(midd))->name, name);
    if (!res) { *pos=midd;  return TRUE; }
    if (res < 0) start=midd; else stop=midd;
  }
  res=strcmp(((t_comp*)d_comp->acc(start))->name, name);
  if (!res) { *pos=start;  return TRUE; }
  *pos=res < 0 ? start+1 : start;
  return FALSE;
}

CFNC DllKernel t_dynar * WINAPI get_object_cache(cdp_t cdp, tcateg category)
{ if (cdp->object_cache_disabled) return NULL;
  switch (category & CATEG_MASK)
  { case CATEG_TABLE:
      return &cdp->odbc.apl_tables;
    case CATEG_VIEW:
      return &cdp->odbc.apl_views;
    case CATEG_MENU:
      return &cdp->odbc.apl_menus;
    case CATEG_CURSOR:
      return &cdp->odbc.apl_queries;
    case CATEG_PGMSRC:
      return &cdp->odbc.apl_programs;
    case CATEG_PICT:
      return &cdp->odbc.apl_pictures;
    case CATEG_DRAWING:
      return &cdp->odbc.apl_drawings;
    case CATEG_REPLREL:
      return &cdp->odbc.apl_replrels;
    case CATEG_WWW:
      return &cdp->odbc.apl_wwws;
    case CATEG_RELATION:
      return &cdp->odbc.apl_relations;
    case CATEG_PROC:
      return &cdp->odbc.apl_procs;
    case CATEG_TRIGGER:
      return &cdp->odbc.apl_triggers;
    case CATEG_SEQ:
      return &cdp->odbc.apl_seqs;
    case CATEG_FOLDER:
      return &cdp->odbc.apl_folders;
    case CATEG_DOMAIN:
      return &cdp->odbc.apl_domains;
  }
  return NULL;
}

#ifdef CLIENT_GUI
#if WBVERS<900
CFNC DllExport int WINAPI view_flags(view_stat * vdef)
{ BOOL is_report = (vdef->vwStyle3 & VIEW_IS_REPORT) != 0;
  UINT codesize = vdef->vwCodeSize;
  var_start * ptr = (var_start*)vdef->vwCode;
  while (codesize>0)
  { if (ptr->type>=VW_GROUPA && ptr->type<=VW_GROUPE)
      is_report = TRUE;
	  codesize-=ptr->size + 3;
    ptr=(var_start*)((tptr)ptr + ptr->size + 3);
  }
  int i;  t_ctrl * itm;
  for (i=0, itm=FIRST_CONTROL(vdef);  i<vdef->vwItemCount;
       i++, itm=NEXT_CONTROL(itm))
    if (itm->pane!=PANE_DATA && itm->pane!=PANE_PAGESTART && itm->pane!=PANE_PAGEEND)
      is_report = TRUE;
  if (vdef->vwIsLabel)               return CO_FLAG_LABEL_VW;  // must be before is_report checking!
  if (is_report)                     return CO_FLAG_REPORT_VW;
  if (vdef->vwHdFt  & VIEW_IS_GRAPH) return CO_FLAG_GRAPH_VW;
  if (vdef->vwStyle & SIMPLE_VIEW)   return CO_FLAG_SIMPLE_VW;
  return 0;
}
#endif
#endif

static unsigned get_obj_flags(cdp_t cdp, tcateg category, ctptr name)
{ unsigned flags;  tobjnum objnum, lobjnum;  tcateg cat;
  if (cd_Find_object(cdp, name, (tcateg)(category|IS_LINK), &lobjnum)) return 0;
  if (cd_Read(cdp, CATEG2SYSTAB(category), lobjnum, OBJ_CATEG_ATR, NULL, &cat)) return 0;
  if (cat & IS_LINK)
  { if (category==CATEG_TABLE)
    { uns8 link_type;
      cd_Read_var(cdp, TAB_TABLENUM, lobjnum, OBJ_DEF_ATR, NOINDEX, 0, 1, &link_type, NULL);
      if (link_type==LINKTYPE_ODBC) return CO_FLAG_LINK|CO_FLAG_ODBC;
      else return CO_FLAG_LINK;
    }
    else
    { flags=CO_FLAG_LINK;
      if (cd_Find_object(cdp, name, category, &objnum)) return 0;
    }
  }
  else
    { flags=0;  objnum=lobjnum; }
  switch (category)
  { case CATEG_TABLE:
      if (!(flags & CO_FLAG_LINK))
      { const d_table * td = cd_get_table_d(cdp, objnum, CATEG_TABLE);
        if (td!=NULL)
        { if (td->tabdef_flags & TFL_ZCR) flags = CO_FLAG_REPLTAB;
          release_table_d(td);
        }
      }
      break;
    case CATEG_PGMSRC:
    { tptr src=cd_Load_objdef(cdp, objnum, CATEG_PGMSRC);
      if (src==NULL) break;
      flags|=program_flags(cdp, src);
      corefree(src);
      break;
    }
#ifdef CLIENT_GUI
#if WBVERS<900
    case CATEG_VIEW:
    { tptr src=cd_Load_objdef(cdp, objnum, CATEG_VIEW);
      if (src==NULL) break;
      compil_info xCI(cdp, src, MEMORY_OUT, view_struct_comp);
      int res=compile(&xCI);
      corefree(src);
      if (res) break;
      flags|=view_flags((view_stat*)xCI.univ_code);
      corefree(xCI.univ_code);
      break;
    }
    case CATEG_MENU:
    { tptr p, src=cd_Load_objdef(cdp, objnum, CATEG_PGMSRC);
      if (src==NULL) break;
      p=src;
      while (*p==' ' || *p=='\r' || *p=='\n') p++;
      if (!strnicmp(p, "POPUP", 5)) flags |= CO_FLAG_POPUP_MN;
      corefree(src);
      break;
    }
    case CATEG_WWW:
      flags|=lnk_get_www_object_type(cdp, objnum);
      break;
#endif
#endif
  }
  return flags;
}

CFNC DllKernel unsigned WINAPI get_object_flags(cdp_t cdp, tcateg category, ctptr name)
// categ is without IS_LINK!
{ if (cdp->odbc.mapping_on && category==CATEG_TABLE) return CO_FLAG_ODBC;
  char objname[OBJ_NAME_LEN+1];  strcpy(objname, name);  Upcase9(cdp, objname);
 // flags from the object cache:
  t_dynar * d_comp = get_object_cache(cdp, category);  unsigned pos;
  if (d_comp!=NULL)
  { if (comp_dynar_search(d_comp, objname, &pos))
      return ((t_comp*)d_comp->acc(pos))->flags;
   // object not in the cache: just added (imported, created)
    return get_obj_flags(cdp, category, objname);
  }
#ifdef CLIENT_ODBC
  else if (category==CATEG_CONNECTION)
  { t_connection * conn = find_data_source(cdp, objname);
    return conn && conn->hStmt ? 0 : CO_FLAG_POPUP_MN;
  }
#endif  
 // flags for non-cached objects:
  return 0;
}

CFNC DllKernel t_folder_index WINAPI search_or_add_folder(cdp_t cdp, const char * fldname, tobjnum objnum)
{ t_folder_index folder_indx;
  if (!*fldname || *fldname=='.' && !fldname[1]) return 0; // root folder
  tobjname ufldname; strcpy(ufldname, fldname);  Upcase9(cdp, ufldname);
  for (folder_indx = 1;  folder_indx < cdp->odbc.apl_folders.count();  folder_indx++)
    if (!strcmp(cdp->odbc.apl_folders.acc0(folder_indx)->name, ufldname)) break;
  if (folder_indx >= cdp->odbc.apl_folders.count())
  { t_folder * fld;
    if (!cdp->odbc.apl_folders.count())
    { fld = cdp->odbc.apl_folders.next();
      if (fld)
        { strcpy(fld->name, ".");  fld->objnum=0;  fld->flags=0; }
    }
    fld = cdp->odbc.apl_folders.next();
    if (!fld) return 0;  // 0 as default on error
    strcpy(fld->name, ufldname);
    fld->objnum=objnum;  fld->flags=0; 
  }
  else if (cdp->odbc.apl_folders.acc0(folder_indx)->objnum == NOOBJECT)
    cdp->odbc.apl_folders.acc0(folder_indx)->objnum = objnum;
  return folder_indx;
}

CFNC DllKernel tobjnum WINAPI object_to_cache(cdp_t cdp, t_dynar * d_comp,
   tcateg cat, ctptr name, tobjnum objnum, uns16 flags, unsigned * cache_index)
// Writes the given object into the client cache.
// Creates and returns ODBC table number for ODBC tables.
// Warning: does not define folder/timestamp!
{ 
  if (cat==CATEG_CONNECTION)
  { 
#ifdef CLIENT_ODBC
    t_conndef conndef;  uns32 rd;
    if (!cd_Read_var(cdp, OBJ_TABLENUM, objnum, OBJ_DEF_ATR, NOINDEX, 0,
                    sizeof(t_conndef), &conndef, &rd) || rd==sizeof(t_conndef))
    { t_connection * conn = find_data_source(cdp, conndef.dsname);
      if (conn==NULL)
      { conn = *conndef.conn_string ?
          connect_data_source(cdp, conndef.conn_string, TRUE,  FALSE) :
          connect_data_source(cdp, conndef.dsname,      FALSE, FALSE);
      }
      if (conn!=NULL)
      { if (conn->objnum==NOOBJECT) conn->objnum=objnum;
        if (!conn->dsn[0]) strcpy(conn->dsn, conndef.dsname);  // used if connection error
      }
    }
#endif  
  }
  else 
  if (*name != TEMP_TABLE_MARK)
  { char upname[OBJ_NAME_LEN+1];  strcpy(upname, name);  Upcase9(cdp, upname);
#ifdef CLIENT_ODBC
    if (cat==CATEG_TABLE && flags & CO_FLAG_ODBC)  // create ODBC table num
    { t_odbc_link * odbc_link = (t_odbc_link *)sigalloc(sizeof(t_odbc_link), 44);
      if (odbc_link==NULL) return NOOBJECT;
      uns32 rd;
      if (cd_Read_var(cdp, TAB_TABLENUM, objnum, OBJ_DEF_ATR, NOINDEX, 0, sizeof(t_odbc_link), odbc_link, &rd)
          || rd != sizeof(t_odbc_link))
        { corefree(odbc_link);  return NOOBJECT; }
     // find the connection:
      t_connection * conn;
      conn=conn = find_data_source(cdp, odbc_link->dsn);
      if (conn==NULL)  /* connection not created yet */
      { conn=connect_data_source(cdp, odbc_link->dsn, FALSE, FALSE);
        if (conn==NULL)
          { corefree(odbc_link);  return NOOBJECT; }
      }
      odbc_tabcurs * tab = conn->ltabs.next();
      if (tab==NULL)
        { client_error(cdp, OUT_OF_MEMORY);  corefree(odbc_link);  return NOOBJECT; }
      tab->is_table=TRUE;
      tab->mapped  =FALSE;
      tab->s.odbc_link=odbc_link;
      tab->odbc_num=objnum=get_new_odbc_tabnum(cdp);
      tab->conn    =conn;
      tab->ltab    =NULL;
    }
#endif
   // insert the object info the cache:
    unsigned pos;
    if (cat==CATEG_FOLDER)
      pos=search_or_add_folder(cdp, upname, objnum);
    else
    { if (!comp_dynar_search(d_comp, upname, &pos))
      { if (d_comp->next() == NULL) return NOOBJECT;
        /* space added at the end */
        if (d_comp->count() > (int)pos+1) // empty item create if not tested
          memmov(d_comp->acc(pos+1), d_comp->acc(pos), d_comp->el_size() *
                 (d_comp->count()-pos-1));
      }
      t_comp * acomp = (t_comp*)d_comp->acc(pos);
      strcpy(acomp->name, upname);
      acomp->objnum = objnum;  // updated for ODBC tables
      acomp->flags  = flags;
    }
    if (cache_index) *cache_index=pos;
#ifdef CLIENT_GUI
#if WBVERS<900
    if (cat==CATEG_RELATION)  // load its definition and analyses it into the cache
    { tptr def=cd_Load_objdef(cdp, objnum, CATEG_RELATION);
      t_relation * rel = (t_relation*)d_comp->acc(pos);
      rel->index1=rel->index2=0;
      rel->refint=0xff;
      if (def)
      { tptr p=def;
        Get_object_folder_name(cdp, objnum, CATEG_RELATION, rel->object_folder);
        strmaxcpy(rel->tab1name, p, sizeof(rel->tab1name));  p+=sizeof(tobjname);
        strmaxcpy(rel->tab2name, p, sizeof(rel->tab2name));  p+=sizeof(tobjname);
        const d_table * td1=NULL, * td2=NULL;  d_attr info1, info2;   int i;
        if (!cd_Find_object(cdp, rel->tab1name, CATEG_TABLE, &rel->tab1num))
          td1=cd_get_table_d(cdp, rel->tab1num, CATEG_TABLE);
        if (!cd_Find_object(cdp, rel->tab2name, CATEG_TABLE, &rel->tab2num))
          td2=cd_get_table_d(cdp, rel->tab2num, CATEG_TABLE);
        if (td1 && td2)
        { for (i=0;  i<MAX_REL_ATTRS && *p;  i++)
          { if (find_attr(cdp, rel->tab1num, CATEG_TABLE, p, NULL, NULL, &info1) &&
                find_attr(cdp, rel->tab2num, CATEG_TABLE, p+ATTRNAMELEN+1, NULL, NULL, &info2))
              { rel->atr1[i] = info1.name[0];  rel->atr2[i] = info2.name[0]; }
            else rel->atr1[i] = rel->atr2[i] = 0xff;
            p+=2*(ATTRNAMELEN+1);
          }
          if (i<MAX_REL_ATTRS) rel->atr1[i]=rel->atr2[i]=0xff; // delimiter
        }
        if (td1) release_table_d(td1);
        if (td2) release_table_d(td2);
        corefree(def);
      }
      else
      { //no_memory();  -- may be caused by the lack of privils, do not report it (called when openning the application)
        *rel->tab1name=*rel->tab2name=0;  rel->tab1num=rel->tab2num=NOOBJECT;
        rel->atr1[0]=rel->atr2[0]=0xff;
      }
    }
#endif
#endif
  }
  return objnum;
}

void scan_objects(cdp_t cdp, tcateg cat, BOOL extended)
// Loads all objects of the given category into the object cache
// Checks and deletes bad relations
{ tptr info;  t_dynar * d_comp = get_object_cache(cdp, cat);
  if (cd_List_objects(cdp, extended ? cat|IS_LINK : cat, NULL, &info))
  { 
#ifdef CLIENT_GUI
#if WBVERS<900
    if (cat!=CATEG_RELATION) cd_Signalize(cdp);  // called for relations even when server connection broken (from inval_table_d)
#endif
#endif
    return; 
  }
  for (tptr src=info;  *src;  )
  { tptr name, folder_name;  uns32 stamp;  tobjnum objnum;  uns16 flags;  unsigned cache_index;
    name   = src;             src+=strlen(src)+1;
    if (extended)
    { folder_name=src;        src+=strlen(src)+1;
      stamp=*(uns32*)src;     src+=sizeof(uns32);
    }
    objnum = *(tobjnum*)src;  src+=sizeof(tobjnum);
    flags  = *(uns16*)src;    src+=sizeof(uns16);
    if (object_to_cache(cdp, d_comp, cat, name, objnum, flags, &cache_index) != NOOBJECT) // object OK, added to cache
      if (extended && cat!=CATEG_CONNECTION)
      { t_folder_index fi = search_or_add_folder(cdp, folder_name, NOOBJECT); 
        t_comp * acomp    = (t_comp*)d_comp->acc0(cache_index);
        if (acomp)
        { acomp->folder = fi;
          acomp->modif_timestamp=stamp;
        }
      }
  }
  corefree(info);
}
