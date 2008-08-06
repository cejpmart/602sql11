//#include "impexp.h"

static int compile_program9(cdp_t cdp, tptr source, int output_type,
          void (WINAPI *startnet)(CIp_t CI), BOOL report_success, BOOL debug_info,
          const char * srcname, tobjnum srcobj, tobjnum codeobj, void * univ_ptr)
/* compiles the program from the prepared source & puts result on the screen */
{ compil_info xCI(cdp, source, output_type, startnet);
  xCI.gener_dbg_info=(wbbool)debug_info;
  xCI.outname=srcname;
  strcpy(xCI.src_name, srcname ? srcname : NULLSTRING);
  xCI.src_objnum=srcobj;
  xCI.univ_ptr=univ_ptr;
  xCI.cb_object=codeobj;
  int res=compile(&xCI);
  if (res)
  { delete xCI.dbgi;  cdp->RV.dbgi=NULL;
    client_error(cdp, res);
  }
  else 
    cdp->RV.dbgi=xCI.dbgi;   /* move debug info */

  return res;
}

static BOOL timestamps_ok(cdp_t cdp, pgm_header_type * h)
{ 
#if WBVERS<900
  for (int i=0;  i<MAX_SOURCE_NUMS && h->sources[i]!=NOOBJECT;  i++)
  { uns32 stamp;
    if (Get_obj_modif_time(cdp, h->sources[i], CATEG_PGMSRC, &stamp))
      if (stamp > h->compil_timestamp)
        return FALSE;
  }
#endif  
  return TRUE;
}

#define MAX_RETRY 5 // nesmi byt prilis moc, pokud nemam pravo cist program, dlouho ceka

int synchronize9(cdp_t cdp, const char * srcname, tobjnum srcobj, tobjnum *codeobj, BOOL recompile, BOOL debug_info, BOOL report_success)
/* return error codes, comp. results are put into the info box */
// Doplnena castecna ochrana pred soubeznym prekladanim tehoz programu z ruznych klientu, deje se ve WBCGI, kdyz vice frames otevira stejny projekt
{ int res=0;  BOOL locked=FALSE;
 /* checking to see if compilation is necessary */
  BOOL comp_needed=TRUE;  int retry_counter=0;
 retry:
  if (!cd_Find_object(cdp, srcname, CATEG_PGMEXE, codeobj))
  {// must not try to lock nor read in cannot read 
    uns8 priv_val[PRIVIL_DESCR_SIZE];
    cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, OBJ_TABLENUM, *codeobj, OPER_GETEFF, priv_val);
    if (!HAS_READ_PRIVIL(priv_val, OBJ_DEF_ATR))
    { client_error(cdp, NO_RUN_RIGHT);  
      return NO_RUN_RIGHT;  
    }
    pgm_header_type h;  uns32 rdsize;
    if (!cd_Read_var(cdp, OBJ_TABLENUM, (trecnum)*codeobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(pgm_header_type), (tptr)&h, &rdsize))
      if (rdsize==sizeof(pgm_header_type))  /* non-empty code */
        if (h.version==EXX_CODE_VERSION && h.language==cdp->selected_lang && h.platform==CURRENT_PLATFORM)  /* compiler version number & language OK */
          if (timestamps_ok(cdp, &h))
            comp_needed=FALSE;
   // locking moved after checking the time in order to prevent NOT_LOCKED error messages when many clients simultaneously try to open a project.
    if (comp_needed)
      if (HAS_WRITE_PRIVIL(priv_val, OBJ_DEF_ATR))
       // try to lock and check if it has to be compiled    // cd_waiting(cdp, 70); -- does not work for Write_lock_...
        if (!cd_Write_lock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj)) locked=TRUE;
        else if (retry_counter++<MAX_RETRY) { Sleep(1000);  goto retry; }
  }
  else // create the code object
  { if (cd_Insert_object(cdp, srcname, CATEG_PGMEXE, codeobj))
      if (retry_counter++<MAX_RETRY) goto retry;
    if (!cd_Write_lock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj)) locked=TRUE;
    else if (retry_counter++<MAX_RETRY) goto retry;
    uns8 priv_val[PRIVIL_DESCR_SIZE];
    //*priv_val=RIGHT_DEL;
    //memset(priv_val+1, 0xff, PRIVIL_DESCR_SIZE-1); -- must not set privils which I do not have!
    cd_GetSet_privils(cdp, NOOBJECT,        CATEG_USER,  OBJ_TABLENUM, *codeobj, OPER_GET, priv_val);
    cd_GetSet_privils(cdp, EVERYBODY_GROUP, CATEG_GROUP, OBJ_TABLENUM, *codeobj, OPER_SET, priv_val);
  }
  if (srcobj==NOOBJECT)
    if (cd_Find_object(cdp, srcname, CATEG_PGMSRC, &srcobj))
      if (comp_needed)  /* no source, but code exists */
        { res=OBJECT_NOT_FOUND;  goto ex; }
      else
      { 
        goto ex;
      }

 // check the source run privilege:
  if (comp_needed || recompile || cdp->global_debug_mode != (cdp->RV.bpi!=NULL))   
  /* compilation necessary */
  { tptr src=cd_Load_objdef(cdp, srcobj, CATEG_PGMSRC);
    if (!src) { client_error(cdp, OUT_OF_MEMORY);  res=OUT_OF_MEMORY;  goto ex; }
    res=compile_program9(cdp, src, DB_OUT, program, report_success, debug_info, srcname, srcobj, *codeobj, NULL);
    corefree(src);
  }
 ex:
  if (locked) cd_Write_unlock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj);
  return res;
}

////////////////////////////////////// direct copy from project.cpp ///////////////////////////////
void convert_type(typeobj *& tobj, objtable * id_tab);
CFNC void WINAPI link_forwards(objtable * id_tab);

void conv_type_inner(typeobj * tobj, objtable * id_tab)
// Converts type references used in the description of a type. Not called for T_SIMPLE.
{ switch (tobj->type.anyt.typecateg)
  { 
    case T_ARRAY:
      convert_type(tobj->type.array.elemtype, id_tab);  break;
    case T_PTR:
      convert_type(tobj->type.ptr.domaintype, id_tab);  break;
    case T_PTRFWD:
    { object * obd = search1(tobj->type.fwdptr.domainname, id_tab);
      if (obd==NULL || obd->any.categ!=O_TYPE)  // impossible, I hope
        tobj->type.fwdptr.domaintype=&att_int32;
      else
        tobj->type.fwdptr.domaintype=&obd->type;
      break;
    }
    case T_RECORD:
      link_forwards(tobj->type.record.items);  break;
    case T_RECCUR:
      if (tobj->type.record.items)
        link_forwards(tobj->type.record.items);  
      break;
  }
}

void convert_type(typeobj *& tobj, objtable * id_tab)
{ if (!tobj) return; // may ne the NULL ptr for retvaltype
  if ((size_t)tobj < ATT_AFTERLAST)
    tobj = simple_types[(size_t)tobj];
  else
    conv_type_inner(tobj, id_tab);
}

CFNC void WINAPI link_forwards(objtable * id_tab)
{ for (int i=0;  i<id_tab->objnum;  i++)
  { objdef * objd = &id_tab->objects[i];
    object * ob = objd->descr;
    switch (ob->any.categ)
    { case O_VAR:  case O_VALPAR:  case O_REFPAR:  case O_RECELEM:
        convert_type(ob->var.type, id_tab);
        break;
      case O_CONST:
        convert_type(ob->cons.type, id_tab);
        break;
      case O_TYPE:
      { typeobj * tobj = &ob->type;
        if (tobj->type.anyt.typecateg)
        { convert_type(tobj, id_tab);  
          ob=(object*)tobj;
        }
        else
          conv_type_inner(tobj, id_tab);
        break;
      }
      case O_FUNCT:  case O_PROC:
        convert_type(ob->subr.retvaltype, id_tab);
        link_forwards(ob->subr.locals);
        break;
      case O_CURSOR:
        convert_type(ob->curs.type, id_tab);
        break;
    }
  }
}
///////////////////////////////////////////////////////////

CFNC DllKernel objtable standard_table;

CFNC int WINAPI set_project_up(cdp_t cdp, const char * progname, tobjnum srcobj,
                  BOOL recompile, BOOL report_success)
{ int res;  tobjnum codeobj;  pgm_header_type h;  run_state * RV = &cdp->RV;
  tptr prog_code, prog_vars;  uns8 bt;  char * prog_decl, * prog_decl2;
  uns32 rdsize;  
  if (!recompile && !strcmp(RV->run_project.project_name, progname) &&
      cdp->global_debug_mode == (cdp->RV.bpi!=NULL))
    return 0;  /* the project is already set-up */

  free_project(cdp, FALSE);
  res=synchronize9(cdp, progname, srcobj, &codeobj, recompile, cdp->global_debug_mode, report_success);
  /* 1st call to synchronize opens the info-window. It's placed below the windesk! */
  /* close if compilation OK and !recompile */
  if (res) return res;
 /* loading program header, code & decls: */
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(pgm_header_type), (tptr)&h, &rdsize))
    { client_error_param(cdp, cd_Sz_error(cdp));  return 1; }
  prog_code=(tptr)corealloc(h.code_size+255, 255); /* +255 prevents GPF when writing strings near the segment's end */
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, 0, h.code_size, prog_code, &rdsize))
    { client_error_param(cdp, cd_Sz_error(cdp));  corefree(prog_code);  return 1; }
  unsigned decl_space = h.decl_size > h.unpacked_decl_size ? h.decl_size : h.unpacked_decl_size; // unpacked_decl_size is usually bigger on 64-bit systems
  prog_decl =(tptr)corealloc(h.decl_size+1, 255); /* when relocating I will read 1 byte more */
  prog_decl2=(tptr)corealloc(h.unpacked_decl_size, 255);
  if (!prog_decl || !prog_decl2) { client_error_message(cdp, "no memory");  corefree(prog_code);  corefree(prog_decl);  corefree(prog_decl2);  return 1; }
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, h.code_size, h.decl_size, prog_decl, &rdsize))
    {client_error_param(cdp, cd_Sz_error(cdp));  corefree(prog_decl);  corefree(prog_decl2);  corefree(prog_code);  return 1; }
 /* create vars */
  prog_vars=(tptr)corealloc(h.proj_vars_size+sizeof(scopevars)+255, 255);  /* +255 prevents GPF when writing strings near the segment's end */
  if (!prog_vars)
    { client_error_message(cdp, "no memory");  corefree(prog_decl);  corefree(prog_decl2);  corefree(prog_code);  return 1; }
  memset(prog_vars, 0, h.proj_vars_size+sizeof(scopevars));
 /* relink the decls */
  uns32 src, dest, offset;
  src=0;  dest=0;
  while (src<h.decl_size)
  { bt=prog_decl[src++];
    if (bt==RELOC_MARK)
    { offset=*(uns32*)(prog_decl+src) & 0xffffffL;
      src+=3;
      if (offset!=0xffffffL)
      { *(tptr *)(prog_decl2+dest)=(tptr)(prog_decl2+offset);
        dest+=sizeof(tptr);
      }
      else prog_decl2[dest++]=bt;
    }
    else prog_decl2[dest++]=bt;
  }
  corefree(prog_decl);
 // update forward pointers:
  objtable * id_tab=(objtable*)(prog_decl2+h.pd_offset);
  id_tab->next_table=&standard_table;   // used when the project is set during the compilation of menu or view
  link_forwards(id_tab);
 /* set up the project info */
  RV->run_project.proj_decls = prog_decl2;
  RV->run_project.global_decls = id_tab;
  RV->run_project.common_proj_code=(pgm_header_type*)prog_code;
  strcpy(RV->run_project.project_name, progname);  
  Upcase(RV->run_project.project_name);
  RV->glob_vars=(scopevars*)prog_vars;  RV->glob_vars->next_scope=NULL;
  exec_constructors(cdp, TRUE);
 // new proj_decls & global_decls are valid, delete the previous:
  if (RV->run_project.prev_proj_decls) corefree(RV->run_project.prev_proj_decls);
  RV->run_project.prev_proj_decls=NULL;  RV->run_project.prev_global_decls=NULL;
  return 0;
}


CFNC DllKernel void WINAPI run_program(cdp_t cdp, const char * srcname, tobjnum srcobj, BOOL recompile)
/* on compilation error CompInfo remains on the screen */
{ int res;  run_state * RV = &cdp->RV;

  res=set_project_up(cdp, srcname, srcobj, recompile, FALSE);
  if (!res)
  { run_prep(cdp, (tptr)RV->run_project.common_proj_code, RV->run_project.common_proj_code->main_entry);
    res=run_pgm(cdp);
    if (res!=EXECUTED_OK)  /* run error */
      client_error(cdp, res);  
    /* foll. lines must not be called before reporting run error! */
    int clres=close_run(cdp);  /* closes files & cursors */
    if (!cd_Close_cursor(cdp, (tcursnum)-2)) clres|=2;
    if (res==EXECUTED_OK)  /* no error, display warnings: */
    { if (clres & 1) client_error_message(cdp, "FILES_NOT_CLOSED");
      if (clres & 2) client_error_message(cdp, "CURSORS_NOT_CLOSED");
    }
  }
}


/////////////////////////////////////////////////////////////////////////////////////////
#if 0  // the original project from verson 8
#ifdef NETWARE
#include <process.h>
#endif
#ifdef UNIX
#include <dlfcn.h>
#endif

CFNC DllPrezen void WINAPI free_project(cdp_t cdp, BOOL total)
// total: delete previous symbol info as well.
{ global_project_type * project = &cdp->RV.run_project;
#ifdef WINS
  Set_timer(cdp, 0, 0);
 // close all views, because the may use the program code from the project
  if (cdp->RV.hClient && project->project_name[0] && !cdp->non_mdi_client)
    DestroyMdiChildren(cdp->RV.hClient, FALSE);
#endif
 // destruct objects:
  exec_constructors(cdp, FALSE);
 // delete debug structures:
  if (cdp->RV.dbgi) { delete cdp->RV.dbgi;  cdp->RV.dbgi=NULL; }
  safefree((void**)&cdp->RV.bpi);
 /* free decls, vars, code: */
#ifdef WINS
  dynmem_stop();
#endif
  if (project->proj_decls)
  { if (project->prev_proj_decls) corefree(project->prev_proj_decls);
    project->prev_proj_decls=project->proj_decls;
    project->prev_global_decls=project->global_decls;
  }
  project->proj_decls=NULL;  project->global_decls=NULL;
  if (total)
  { if (project->prev_proj_decls) corefree(project->prev_proj_decls);
    project->prev_proj_decls=NULL;  project->prev_global_decls=NULL;
  }
  free_run_vars(cdp);
  if (cdp->RV.glob_vars)
    { corefree(cdp->RV.glob_vars);  cdp->RV.glob_vars=NULL; }
  cdp->RV.auto_vars=NULL;  /* very important: was freed by the 2 previous statements */
  corefree(project->common_proj_code);  project->common_proj_code=NULL;
  project->project_name[0]=0;
#ifdef WINS
  if (cdp->RV.hClient)
    SendMessage(GetFrame(cdp->RV.hClient), SZM_NEWWINNAME, 0, 0);  /* remove project name from the window title */
#endif
 /* free external DLLs */
  libinfo * blib, * alib=cdp->RV.dllinfo;
  while (alib!=NULL)    /* searching installed libraries */
  { FreeLibrary(alib->hLibInst);
    blib=alib->next_lib;  corefree(alib);  alib=blib;
  }
  cdp->RV.dllinfo=NULL;
}

#ifdef WINS
void comp_progress(int local_lines, int total_lines, const char * name)
{ if (!hCompInfo) return;
  char buf[20];  sprintf(buf, "%u/%u", local_lines, total_lines);
  SetDlgItemText(hCompInfo, CD_LINENUM, buf);
  if (name) SetDlgItemText(hCompInfo, CD_SRCNAME, name);
}
#else
#define comp_progress NULL  // no compilation progress indicator when compiling in NLM
#endif

static int compile_program(cdp_t cdp, tptr source, int output_type,
          void (WINAPI *startnet)(CIp_t CI), BOOL report_success, BOOL debug_info,
          const char * srcname, tobjnum srcobj, tobjnum codeobj, void * univ_ptr)
/* compiles the program from the prepared source & puts result on the screen */
{ compil_info xCI(cdp, source, output_type, startnet);
  xCI.gener_dbg_info=(wbbool)debug_info;
  xCI.comp_callback=comp_progress;
  xCI.outname=srcname;
  strcpy(xCI.src_name, srcname ? srcname : NULLSTRING);
  xCI.src_objnum=srcobj;
  xCI.univ_ptr=univ_ptr;
  xCI.cb_object=codeobj;
#ifdef WINS
  HCURSOR oldcurs;
  if (cdp->RV.hClient)
  { hCompInfo=CreateDialogParam(hPrezInst, MAKEINTRESOURCE(DLG_COMP_INFO),
                                GetParent(cdp->RV.hClient), ModelessCompInfoDlgProc, 0);
    SendDlgItemMessage(hCompInfo, 199, ACM_OPEN, 0, (LPARAM)MAKEINTRESOURCE(AVI_COMPILE));
    SendDlgItemMessage(hCompInfo, 199, ACM_PLAY, -1, MAKELONG(0,-1));
    oldcurs=SetCursor(LoadCursor(NULL, IDC_WAIT));
  }
#endif
  int res=compile(&xCI);
#ifdef WINS
  if (cdp->RV.hClient)
  { SetCursor(oldcurs);
    SendDlgItemMessage(hCompInfo, 199, ACM_STOP, 0, 0);   // not sufficient
  }
#endif
  if (res)
  { delete xCI.dbgi;  cdp->RV.dbgi=NULL;
#ifdef WINS
  if (cdp->RV.hClient)  /* set cursor on the error position if the right editor window open */
    show_pgm_line(cdp, xCI.src_objnum, 0, xCI.src_line, xCI.sc_col, startnet == event_handler ? PST_AXEHCOMP : 0);
#endif
  }
  else 
    cdp->RV.dbgi=xCI.dbgi;   /* move debug info */

#ifdef WINS
  if (cdp->RV.hClient) if (res || report_success)
  { char compmsgbuf[120];
    if (res)
    { client_error(cdp, res);
      Get_error_num_text(cdp, res, compmsgbuf, sizeof(compmsgbuf));
    }
    else LoadString(hPrezInst, IS_COMPILED_OK, compmsgbuf, sizeof(compmsgbuf));
   /* new: */
    SetDlgItemText(hCompInfo, CD_COMP_RESULT, compmsgbuf);
    SetActiveWindow(hCompInfo);  /* openning the new editor window moves the activity */
    EnableWindow(GetDlgItem(hCompInfo, CD_COMP_OK), TRUE);
    SetFocus(GetDlgItem(hCompInfo, CD_COMP_OK));
    EnableWindow(GetParent(cdp->RV.hClient), FALSE);
    SendDlgItemMessage(hCompInfo, 199, ACM_STOP, 0, 0);   // not sufficient
    while (hCompInfo)
    { MSG msg;
      if (!GetMessage(&msg, 0, 0, 0)) break;
      if (!IsDialogMessage(hCompInfo, &msg))
      { TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      if (hCompInfo) SendDlgItemMessage(hCompInfo, 199, ACM_STOP, 0, 0); // !!
    }
    EnableWindow(GetParent(cdp->RV.hClient), TRUE);
  }
  if (hCompInfo) { DestroyWindow(hCompInfo);  hCompInfo=0; }
#endif
  return res;
}

static BOOL timestamps_ok(cdp_t cdp, pgm_header_type * h)
{ for (int i=0;  i<MAX_SOURCE_NUMS && h->sources[i]!=NOOBJECT;  i++)
  { uns32 stamp;
    if (Get_object_modif_time(cdp, h->sources[i], CATEG_PGMSRC, &stamp))
      if (stamp > h->compil_timestamp)
        return FALSE;
  }
  return TRUE;
}

#define MAX_RETRY 5 // nesmi byt prilis moc, pokud nemam pravo cist program, dlouho ceka

CFNC DllPrezen int WINAPI synchronize(cdp_t cdp, const char * srcname, tobjnum srcobj, tobjnum *codeobj, BOOL recompile, BOOL debug_info, BOOL report_success)
/* return error codes, comp. results are put into the info box */
// Doplnena castecna ochrana pred soubeznym prekladanim tehoz programu z ruznych klientu, deje se ve WBCGI, kdyz vice frames otevira stejny projekt
{ int res=0;  BOOL locked=FALSE;
#ifdef WINS
  if (cdp->RV.global_state==PROG_RUNNING)
    { errbox(MAKEINTRESOURCE(PROG_IS_RUNNING));  return NO_RUN_RIGHT; }
#endif
 /* checking to see if compilation is necessary */
  BOOL comp_needed=TRUE;  int retry_counter=0;
 retry:
  if (!cd_Find_object(cdp, srcname, CATEG_PGMEXE, codeobj))
  {// must not try to lock nor read in cannot read 
    uns8 priv_val[PRIVIL_DESCR_SIZE];
    cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, OBJ_TABLENUM, *codeobj, OPER_GETEFF, priv_val);
    if (!HAS_READ_PRIVIL(priv_val, OBJ_DEF_ATR))
    { client_error(cdp, NO_RUN_RIGHT);  
      return NO_RUN_RIGHT;  
    }
    pgm_header_type h;  uns32 rdsize;
    if (!cd_Read_var(cdp, OBJ_TABLENUM, (trecnum)*codeobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(pgm_header_type), (tptr)&h, &rdsize))
      if (rdsize==sizeof(pgm_header_type))  /* non-empty code */
        if (h.version==EXX_CODE_VERSION && h.language==cdp->selected_lang && h.platform==CURRENT_PLATFORM)  /* compiler version number & language OK */
          if (timestamps_ok(cdp, &h))
            comp_needed=FALSE;
   // locking moved after checking the time in order to prevent NOT_LOCKED error messages when many clients simultaneously try to open a project.
    if (comp_needed)
      if (HAS_WRITE_PRIVIL(priv_val, OBJ_DEF_ATR))
       // try to lock and check if it has to be compiled    // cd_waiting(cdp, 70); -- does not work for Write_lock_...
        if (!cd_Write_lock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj)) locked=TRUE;
        else if (retry_counter++<MAX_RETRY) { Sleep(1000);  goto retry; }
  }
  else // create the code object
  { if (cd_Insert_object(cdp, srcname, CATEG_PGMEXE, codeobj))
      if (retry_counter++<MAX_RETRY) goto retry;
    if (!cd_Write_lock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj)) locked=TRUE;
    else if (retry_counter++<MAX_RETRY) goto retry;
    uns8 priv_val[PRIVIL_DESCR_SIZE];
    //*priv_val=RIGHT_DEL;
    //memset(priv_val+1, 0xff, PRIVIL_DESCR_SIZE-1); -- must not set privils which I do not have!
    cd_GetSet_privils(cdp, NOOBJECT,        CATEG_USER,  OBJ_TABLENUM, *codeobj, OPER_GET, priv_val);
    cd_GetSet_privils(cdp, EVERYBODY_GROUP, CATEG_GROUP, OBJ_TABLENUM, *codeobj, OPER_SET, priv_val);
  }
  if (srcobj==NOOBJECT)
    if (cd_Find_object(cdp, srcname, CATEG_PGMSRC, &srcobj))
      if (comp_needed)  /* no source, but code exists */
        { res=OBJECT_NOT_FOUND;  goto ex; }
      else
      { 
#ifdef WINS
        if (!check_object_privils(cdp, CATEG_PGMEXE, *codeobj, FALSE))
        { client_error(cdp, NO_RUN_RIGHT);  Signalize();
          res=NO_RUN_RIGHT;  
        }
#endif
        goto ex;
      }

 // check the source run privilege:
#ifdef WINS
  if (!check_object_privils(cdp, CATEG_PGMSRC, srcobj, FALSE))
  { client_error(cdp, NO_RUN_RIGHT);  Signalize();
    res=NO_RUN_RIGHT;  goto ex;
  }
#endif
  if (comp_needed || recompile || cdp->global_debug_mode != (cdp->RV.bpi!=NULL))   
  /* compilation necessary */
  { tptr src=cd_Load_objdef(cdp, srcobj, CATEG_PGMSRC);
    if (!src) { client_error(cdp, OUT_OF_MEMORY);  Signalize();  res=OUT_OF_MEMORY;  goto ex; }
    res=compile_program(cdp, src, DB_OUT, program, report_success, debug_info, srcname, srcobj, *codeobj, NULL);
    corefree(src);
  }
 ex:
  if (locked) cd_Write_unlock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj);
  return res;
}

void convert_type(typeobj *& tobj, objtable * id_tab);

void conv_type_inner(typeobj * tobj, objtable * id_tab)
// Converts type references used in the description of a type. Not called for T_SIMPLE.
{ switch (tobj->type.anyt.typecateg)
  { 
    case T_ARRAY:
      convert_type(tobj->type.array.elemtype, id_tab);  break;
    case T_PTR:
      convert_type(tobj->type.ptr.domaintype, id_tab);  break;
    case T_PTRFWD:
    { object * obd = search1(tobj->type.fwdptr.domainname, id_tab);
      if (obd==NULL || obd->any.categ!=O_TYPE)  // impossible, I hope
        tobj->type.fwdptr.domaintype=&att_int32;
      else
        tobj->type.fwdptr.domaintype=&obd->type;
      break;
    }
    case T_RECORD:
      link_forwards(tobj->type.record.items);  break;
    case T_RECCUR:
      if (tobj->type.record.items)
        link_forwards(tobj->type.record.items);  
      break;
  }
}

void convert_type(typeobj *& tobj, objtable * id_tab)
{ if (!tobj) return; // may ne the NULL ptr for retvaltype
  if ((long)tobj < ATT_AFTERLAST)
    tobj = simple_types[(int)tobj];
  else
    conv_type_inner(tobj, id_tab);
}


CFNC DllPrezen void WINAPI link_forwards(objtable * id_tab)
{ for (int i=0;  i<id_tab->objnum;  i++)
  { objdef * objd = &id_tab->objects[i];
    object * ob = objd->descr;
    switch (ob->any.categ)
    { case O_VAR:  case O_VALPAR:  case O_REFPAR:  case O_RECELEM:
        convert_type(ob->var.type, id_tab);
        break;
      case O_CONST:
        convert_type(ob->cons.type, id_tab);
        break;
      case O_TYPE:
      { typeobj * tobj = &ob->type;
        if (tobj->type.anyt.typecateg)
        { convert_type(tobj, id_tab);  
          ob=(object*)tobj;
        }
        else
          conv_type_inner(tobj, id_tab);
        break;
      }
      case O_FUNCT:  case O_PROC:
        convert_type(ob->subr.retvaltype, id_tab);
        link_forwards(ob->subr.locals);
        break;
      case O_CURSOR:
        convert_type(ob->curs.type, id_tab);
        break;
    }
  }
}

CFNC DllKernel objtable standard_table;

CFNC DllPrezen int WINAPI set_project_up(cdp_t cdp, const char * progname, tobjnum srcobj,
                  BOOL recompile, BOOL report_success)
{ int res;  tobjnum codeobj;  pgm_header_type h;  run_state * RV = &cdp->RV;
  tptr prog_code, prog_vars;  uns8 bt;  char * prog_decl, * prog_decl2;
  uns32 rdsize;  
#ifdef WINS
  if (RV->global_state==PROG_RUNNING)
    { errbox(MAKEINTRESOURCE(PROG_IS_RUNNING));  return NO_RUN_RIGHT; }
  if (cdp->RV.hClient && !cdp->non_mdi_client)
    DestroyMdiChildren(cdp->RV.hClient, FALSE);  /* must be done before free_project() */
  if (my_stricmp(RV->run_project.project_name, progname))
    *eval_history=1; // clear the eval history dependent on the current project
#endif
  if (!recompile && !strcmp(RV->run_project.project_name, progname) &&
      cdp->global_debug_mode == (cdp->RV.bpi!=NULL))
    return 0;  /* the project is already set-up */

  free_project(cdp, FALSE);
  res=synchronize(cdp, progname, srcobj, &codeobj, recompile, cdp->global_debug_mode, report_success);
  /* 1st call to synchronize opens the info-window. It's placed below the windesk! */
  /* close if compilation OK and !recompile */
  if (res) return res;
 /* loading program header, code & decls: */
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(pgm_header_type), (tptr)&h, &rdsize))
    { Signalize();  return 1; }
  prog_code=(tptr)corealloc(h.code_size+255, 255); /* +255 prevents GPF when writing strings near the segment's end */
#ifdef WINS
  HCURSOR old_cursor;
  if (cdp->RV.hClient)
  { old_cursor=SetCursor(LoadCursor(NULL, IDC_WAIT));
    Set_status_text(MAKEINTRESOURCE(STBR_LOADING_PGM));
  }
#endif
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, 0, h.code_size, prog_code, &rdsize))
    { Signalize();  corefree(prog_code);  return 1; }
  unsigned decl_space = h.decl_size > h.unpacked_decl_size ? h.decl_size : h.unpacked_decl_size; // unpacked_decl_size is usually bigger on 64-bit systems
  prog_decl =(tptr)corealloc(h.decl_size+1, 255); /* when relocating I will read 1 byte more */
  prog_decl2=(tptr)corealloc(h.unpacked_decl_size, 255);
  if (!prog_decl || !prog_decl2) { no_memory();  corefree(prog_code);  corefree(prog_decl);  corefree(prog_decl2);  return 1; }
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, h.code_size, h.decl_size, prog_decl, &rdsize))
    { Signalize();  corefree(prog_decl);  corefree(prog_decl2);  corefree(prog_code);  return 1; }
 /* create vars */
  prog_vars=(tptr)corealloc(h.proj_vars_size+sizeof(scopevars)+255, 255);  /* +255 prevents GPF when writing strings near the segment's end */
  if (!prog_vars)
    { no_memory();  corefree(prog_decl);  corefree(prog_decl2);  corefree(prog_code);  return 1; }
  memset(prog_vars, 0, h.proj_vars_size+sizeof(scopevars));
 /* relink the decls */
  uns32 src, dest, offset;
  src=0;  dest=0;
  while (src<h.decl_size)
  { bt=prog_decl[src++];
    if (bt==RELOC_MARK)
    { offset=*(uns32*)(prog_decl+src) & 0xffffffL;
      src+=3;
      if (offset!=0xffffffL)
      { *(tptr *)(prog_decl2+dest)=(tptr)(prog_decl2+offset);
        dest+=sizeof(tptr);
      }
      else prog_decl2[dest++]=bt;
    }
    else prog_decl2[dest++]=bt;
  }
  corefree(prog_decl);
 // update forward pointers:
  objtable * id_tab=(objtable*)(prog_decl2+h.pd_offset);
  id_tab->next_table=&standard_table;   // used when the project is set during the compilation of menu or view
  link_forwards(id_tab);
 /* set up the project info */
  RV->run_project.proj_decls = prog_decl2;
  RV->run_project.global_decls = id_tab;
  RV->run_project.common_proj_code=(pgm_header_type*)prog_code;
  strcpy(RV->run_project.project_name, progname);  
  Upcase(RV->run_project.project_name);
  RV->glob_vars=(scopevars*)prog_vars;  RV->glob_vars->next_scope=NULL;
  exec_constructors(cdp, TRUE);
 // new proj_decls & global_decls are valid, delete the previous:
  if (RV->run_project.prev_proj_decls) corefree(RV->run_project.prev_proj_decls);
  RV->run_project.prev_proj_decls=NULL;  RV->run_project.prev_global_decls=NULL;
#ifdef WINS
  if (cdp->RV.hClient)
  { SendMessage(GetParent(cdp->RV.hClient), SZM_NEWWINNAME, 0, 0);  /* project name to the main window title */
    Set_status_text(NULLSTRING);
    SetCursor(old_cursor);
  }
  if (cdp->global_debug_mode)
  {// prepare data structures for breakpoints:
    RV->bpi=(breakpoints*)sigalloc(sizeof(breakpoints), 77);
    if (!RV->bpi) { delete RV->dbgi;  RV->dbgi=NULL;  cdp->global_debug_mode=wbfalse;  return 0; }
    RV->bpi->bp_count=0;  RV->bpi->bp_space=1;
    RV->bpi->realized=FALSE;
    RV->bpi->code_addr=(uns8*)RV->run_project.common_proj_code;
   // set all persistent breakpoints from all source files of the program:
    for (unsigned i=0;  i<cdp->RV.dbgi->filecnt;  i++)  // search all modules
    { HWND hWndChild;  editor_type * edx;
      edx = find_editor_by_objnum(cdp->RV.hClient, cdp->RV.dbgi->srcobjs(i), OBJ_TABLENUM, &hWndChild, NULL, FALSE, 0);
      if (edx!=NULL) edx->breakpts.set_bps(edx);
    }
  // prepare the "Modules" submenu in the debug menu:
    HMENU hSubMenu=GetSubMenu(debug_menu, 3);
    while (GetMenuItemCount(hSubMenu) > 0)    /* remove old items */
      DeleteMenu(hSubMenu, 0, MF_BYPOSITION);
    for (i=0;  i<RV->dbgi->filecnt;  i++)
    { tobjname buf;
      if (!cd_Read(cdp, OBJ_TABLENUM, (trecnum)RV->dbgi->srcobjs(i), OBJ_NAME_ATR, NULL, buf))
      { small_string(buf, TRUE);
        AppendMenu(hSubMenu, i && !(i % 16) ?
          MF_STRING|MF_ENABLED|MF_MENUBARBREAK : MF_STRING|MF_ENABLED,
          MI_DBG_MODULES+i, buf);
      }
    }
  }   
#endif
  return 0;
}

CFNC DllPrezen BOOL WINAPI cd_Open_project(cdp_t cdp, const char * projname)
{ if (!cdp) return FALSE;
  if (!*cdp->sel_appl_name) return FALSE;  // otherwise creates code object outside schemas
  if (projname==NULL || !*projname)
    { free_project(cdp, TRUE);  return TRUE; }
  else return set_project_up(cdp, projname, NOOBJECT, FALSE, FALSE)==0;
}

CFNC DllPrezen BOOL WINAPI Open_project(const char * projname)
{ return cd_Open_project(GetCurrTaskPtr(), projname); }

CFNC DllPrezen int WINAPI Exec_statements(cdp_t cdp, const char * statements)
{ compil_info xCI(cdp, statements, MEMORY_OUT, statement_seq);
#ifdef WINS
  HCURSOR oldcurs=SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  int res=compile(&xCI);
  if (res) 
  {
#ifdef WINS
    SetCursor(oldcurs);  
#endif
    return res; 
  }
  run_prep(cdp, xCI.univ_code, 0);
  res=run_pgm(cdp);
  corefree(xCI.univ_code);
#ifdef WINS
  SetCursor(oldcurs);
#endif
  return res;
}

CFNC DllPrezen object * WINAPI find_symbol(run_state * RV, const char * name)
{ tname varname;
  strmaxcpy(varname, name, sizeof(tname));  Upcase(varname);
  if (RV->run_project.proj_decls && RV->glob_vars->vars)
  { objtable * id_tab = RV->run_project.global_decls;
    for (int i=0;  i<id_tab->objnum;  i++)
      if (!strncmp(varname, id_tab->objects[i].name, NAMELEN))  /* name found */
        return id_tab->objects[i].descr;
  }
  return NULL;
}

CFNC DllPrezen void * WINAPI Get_var_address(const char * name)
// Returns address of the "name" variable or NULL if not found.
{ run_state * RV = get_RV();
  object * obj = find_symbol(RV, name);
  if (obj && obj->any.categ==O_VAR)
    return RV->glob_vars->vars + obj->var.offset;
  return NULL;
}

CFNC DllPrezen void * WINAPI cd_Get_var_address(cdp_t cdp, const char * name)
// Returns address of the "name" variable or NULL if not found.
{ object * obj = find_symbol(&cdp->RV, name);
  if (obj && obj->any.categ==O_VAR)
    return cdp->RV.glob_vars->vars + obj->var.offset;
  return NULL;
}

CFNC DllPrezen int WINAPI Var_type_info(cdp_t cdp, const char * name, unsigned * valsize)
// Searches for variable name, returns its ATT_type & valsize.
// Returns 0 if name not found, name is not of a variable or type is
//  structured, but not string.
{ object * obj = find_symbol(&cdp->RV, name);
  if (obj && obj->any.categ==O_VAR)
  { int tp;  typeobj * tobj=obj->var.type;
		if (DIRECT(tobj))
    { tp=(uns8)ATT_TYPE(tobj);
      *valsize=tpsize[tp];
      return tp;
    }
    else if (tobj->type.anyt.typecateg==T_STR) 
      tp=ATT_STRING;
    else tp=0;  // other structured type
    *valsize=tobj->type.anyt.valsize;
    return tp;
  }
  *valsize=0;
  return 0;
}

CFNC DllPrezen BOOL WINAPI cd_Enum_variables(cdp_t cdp, enum_vars * callback)
{ if (cdp->RV.run_project.proj_decls==NULL || cdp->RV.glob_vars->vars==NULL) 
    return FALSE;
  objtable * id_tab = cdp->RV.run_project.global_decls;
  for (int i=0;  i<id_tab->objnum;  i++)
  { tname name;  
    strcpy(name, id_tab->objects[i].name);
    int tp, valsize;  typeobj * tobj = id_tab->objects[i].descr->var.type;
		if (DIRECT(tobj))
    { tp=(uns8)ATT_TYPE(tobj);
      valsize=tpsize[tp];
    }
    else 
    { valsize=tobj->type.anyt.valsize;
      if (tobj->type.anyt.typecateg==T_STR) 
        tp=ATT_STRING;
      else tp=0;  // other structured type
    }
    if (!(*callback)(name, tp, valsize)) break;
  }
  return TRUE;
}

#endif

