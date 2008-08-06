#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop

#ifdef LINUX
#if WBVERS<1000  // moved to cint.cpp since 10.0
#include "dynar.cpp"
#include "flstr.h"
#include "flstr.cpp"  // in kerbase.cpp in WINS
#endif
//const char *configfile="/etc/602sql";  -- moved to enumsrv.c
#include <dlfcn.h>
#endif


CFNC DllExport void Get_object_folder_name(cdp_t cdp, tobjnum objnum, tcateg categ, char * folder_name)
{ if (cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_FOLDER_ATR, NULL, folder_name))
    *folder_name=0;
}

CFNC DllExport void WINAPI no_memory()
{  }

CFNC DllExport tptr WINAPI sigalloc(unsigned size, uns8 owner)
{ tptr p;
  p=(tptr)corealloc(size,owner);
  if (!p) no_memory();
  return p;
}

CFNC DllExport BOOL WINAPI cd_Signalize(cdp_t cdp)
{
  int err=cd_Sz_error(cdp);
  return err && err != INTERNAL_SIGNAL;
}

CFNC DllExport void WINAPI cd_Help_file(cdp_t cdp, const char * filename)
{ }

CFNC DllExport void WINAPI free_project(cdp_t cdp, BOOL total)
// total: delete previous symbol info as well.
{ global_project_type * project = &cdp->RV.run_project;
 // destruct objects:
  exec_constructors(cdp, FALSE);
 // delete debug structures:
  if (cdp->RV.dbgi) { delete cdp->RV.dbgi;  cdp->RV.dbgi=NULL; }
  safefree((void**)&cdp->RV.bpi);
 /* free decls, vars, code: */
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
 /* free external DLLs */
  libinfo * blib, * alib=cdp->RV.dllinfo;
  while (alib!=NULL)    /* searching installed libraries */
  { FreeLibrary(alib->hLibInst);
    blib=alib->next_lib;  corefree(alib);  alib=blib;
  }
  cdp->RV.dllinfo=NULL;
}

CFNC DllExport void WINAPI Set_status_text(const char * text)
{ }

CFNC DllExport void WINAPI Set_client_status_nums(cdp_t cdp, trecnum num0, trecnum num1)
{ }

CFNC DllExport BOOL WINAPI RemDBContainer(void)
{ return TRUE; }

