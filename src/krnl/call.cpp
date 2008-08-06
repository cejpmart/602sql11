// call.cpp - calling a C function from the IPL
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#ifdef UNIX
#include <dlfcn.h>
#endif

#ifdef WINS
BOOL WINAPI unload_library(cdp_t cdp, const char * libname)
// Sets the unloadable option or unloads a library.
{ if (!*libname)
  { cdp->RV.unloadable_libs=TRUE;
    return TRUE;
  }
  else
  { libinfo ** plib = &cdp->RV.dllinfo, * lib;
    while (*plib!=NULL)
    { lib=*plib;
      if (!stricmp(lib->libname, libname))
      { FreeLibrary(lib->hLibInst);
        *plib=lib->next_lib;
        corefree(lib);
        return TRUE;
      }
      else plib=&lib->next_lib;
    }
  }
  return FALSE;
}

#endif


#ifdef X64ARCH

void call_dll(cdp_t cdp, dllprocdef * def, scopevars * parptr)
{ }
void do_call(cdp_t cdp, unsigned procnum, scopevars * parptr)  /* calling a standard procedure */
{ }

#else

#ifdef __ia64__
typedef long any_fnc();
extern "C" long do_call_routine(int n, any_fnc fnc, long * args);
#endif

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
{ int len = (int)strlen(name);
  int lnm = (int)strlen(origname);
#ifdef WINS  // translate library name given without suffix
  if (len >= lnm && !stricmp(name+len-lnm, origname))
    { strcpy(name+len-lnm, newname);  return; }
#endif
 // translate library name followed by the proper suffix:
  if (len >= lnm+LIB_SUFFIX_LEN && !stricmp(name+len-LIB_SUFFIX_LEN, LIB_SUFFIX))
    if (!strnicmp(name+len-lnm-LIB_SUFFIX_LEN, origname, lnm))
      { strcpy(name+len-lnm-LIB_SUFFIX_LEN, newname);  strcat(name, LIB_SUFFIX);  return; }
}



#pragma optimize("gelot", off)
#pragma runtime_checks("s", off)

// creates bad stack frame when optimized!!

static basicval retval;

typedef sig64    WINAPI psig64 (void);
typedef uns32    WINAPI puns32 (void);
typedef uns16    WINAPI puns16 (void);
typedef uns8     WINAPI puns8  (void);
typedef sig8     WINAPI psig8  (void);
typedef double   WINAPI pdouble(void);
typedef char   * WINAPI pstring(void);
typedef monstr * WINAPI pmoney (void);

static monstr * retmoney;  /* if this variable was automatic, the optimization would change the standard stack */

#define FIRST_INERT_NUM   80
#define SIGNALIZE_NUM    195
#define INITMAIL_NUM     244
#define INITM602_NUM     245
#define CLOSEMAIL_NUM    246
#define CRELETTER_NUM    247
#define ADDADDR_NUM      248
#define ADDFILE_NUM      249
#define LETTERSEND_NUM   250
#define TAKEMAIL_NUM     251
#define CANCLETTER_NUM   252
#define MBOXOPEN_NUM     258
#define MBOXLOAD_NUM     259
#define MBOXGETMSG_NUM   260
#define MBOXGETFLI_NUM   261
#define MBOXSAVEFLAS_NUM 262
#define MBOXDELMSG_NUM   263
#define MBOXCLOSE_NUM    266
#define MBOXDIAL_NUM     267
#define MBOXHANGUP_NUM   268
#define MBOXGETMSGX_NUM  287
#define LETTERADDBLR_NUM 288
#define LETTERADDBLS_NUM 289
#define MBOXSAVEFDBR_NUM 290
#define MBOXSAVEFDBS_NUM 291

static short InterFcs[] =
{
    SIGNALIZE_NUM,
    INITMAIL_NUM,
    INITM602_NUM,
    CLOSEMAIL_NUM,
    LETTERSEND_NUM,
    TAKEMAIL_NUM,
    MBOXOPEN_NUM,
    MBOXLOAD_NUM,
    MBOXGETMSG_NUM,
    MBOXGETFLI_NUM,
    MBOXSAVEFLAS_NUM,
    MBOXDELMSG_NUM,
    MBOXCLOSE_NUM,
    MBOXDIAL_NUM,
    MBOXHANGUP_NUM,
    MBOXGETMSGX_NUM,
    MBOXSAVEFDBR_NUM,
    MBOXSAVEFDBS_NUM,
    0x7FFF
};

static BOOL Interruptable(short procnum)
{ int i;
    if (procnum < FIRST_INERT_NUM)
        return(TRUE);
    for (i = 0; procnum > InterFcs[i]; i++);
    return(procnum == InterFcs[i]);
}

#ifdef EXTERNAL_FNCS_DONT_WORK
#include <stdcall.c>
#else
void do_call(cdp_t cdp, unsigned procnum, scopevars * parptr)  /* calling a standard procedure */
{ unsigned localsize=parptr->localsize+sizeof(cdp_t);  char * origsp;
#ifndef __ia64__
  void * pars;
#endif
  BOOL Inter = Interruptable(procnum);
  if (Inter)
    if (wsave_run(cdp)) { corefree(parptr);  r_error(OUT_OF_R_MEMORY); }
#ifndef LINUX
  _asm {
         mov origsp,esp
         sub esp,localsize
         mov pars,esp
       }
#elif !defined(__ia64__)
  asm("movl %%esp, %0" : "=m"(origsp));
  asm("subl %0, %%esp" : "=m"(localsize)) ;
  asm("movl %%esp, %0" : "=m"(pars));
  //register char * esp __asm__("esp");
  //origsp=esp;
  //esp-=localsize;
  //pars=esp;
#else  // __ia64__
  int cnt = localsize/sizeof(long)+1;
  long pars_table[cnt];
  void *pars=pars_table;
#endif
  if (has_cdp_param[procnum])
  { *(cdp_t*)pars = cdp;  pars = (char*)pars + sizeof(cdp_t);
  }
  memcpy(pars, &parptr->vars, parptr->localsize);
#ifndef __ia64__
  switch (sfnc_table[procnum])
  { case ATT_BOOLEAN:  case ATT_CHAR:
      retval.int32=(sig32) ((puns8  *) (sproc_table[procnum]))(); break;
    case ATT_INT8:
      retval.int32=(sig32) ((psig8  *) (sproc_table[procnum]))(); break;
    case ATT_INT16:
      retval.int32=(sig32) ((puns16 *) (sproc_table[procnum]))(); break;
    case ATT_MONEY:
      retmoney=((pmoney *) (sproc_table[procnum]))();   /* cannot join with the next stat. */
      memcpy(&retval.moneys, retmoney, MONEY_SIZE);                break;
    case ATT_INT32:    case ATT_DATE:    case ATT_TIME:
    case ATT_NIL:      case ATT_TIMESTAMP:
      retval.int32=        ((puns32 *) (sproc_table[procnum]))(); break;
    case ATT_INT64:
      retval.int64=        ((psig64 *) (sproc_table[procnum]))(); break;
    case ATT_FLOAT:
      retval.real =        ((pdouble*) (sproc_table[procnum]))(); break;
    case ATT_STRING:
      retval.ptr  =        ((pstring*) (sproc_table[procnum]))(); break;
    case 0:                           (*sproc_table[procnum])();  break;
  }
#else  // __ia64__
  retval.int64=do_call_routine(cnt, (any_fnc*)(sproc_table[procnum]), pars_table);
#endif
  corefree(parptr);  // if optimised on GNU C++, removing parameters from the stack
  // ...is moved after the restoring the esp -> crash!
#ifndef LINUX
  _asm mov esp,origsp
#elif !defined(__ia64__)
  //esp=origsp;
  asm("movl %0, %%esp" : "=m"(origsp));
#endif
  if (Inter)
    wrestore_run(cdp);
  if (sfnc_table[procnum])  { cdp->RV.stt++;  *cdp->RV.stt=retval; }
}
#endif

#ifdef WINS
#if WBVERS<900
void call_method(cdp_t cdp, view_dyn * inst, unsigned procnum, scopevars * parptr, int tp)
// Calling a method
{ unsigned localsize=parptr->localsize;  void * origsp, * pars;
  uns32 pom32;  uns16 pom16;
 // create the stack frame:
#ifdef WINS
  _asm {
     mov origsp,esp
     sub esp,localsize
     mov pars,esp
       }
#else
  //register int esp __asm__("esp");
  //origsp=esp;
  //esp-=localsize;
  //pars=esp;
  asm("movl %%esp, %0" : "=m"(origsp));
  asm("subl %0, %%esp" : "=m"(localsize)) ;
  asm("movl %%esp, %0" : "=m"(pars));
#endif
  memcpy(pars, &parptr->vars, localsize);
  corefree(parptr);
  if (wsave_run(cdp)) r_error(OUT_OF_R_MEMORY);
  switch (tp)  // other types never returned
  { case ATT_BOOLEAN:  case ATT_CHAR:
      retval.int32=(sig32) ((puns8  *) (vwmtd_table[procnum]))(); break;
    case ATT_INT8:
      retval.int32=(sig32) ((psig8  *) (vwmtd_table[procnum]))(); break;
    case ATT_INT16:
      retval.int32=(sig32) ((puns16 *) (vwmtd_table[procnum]))(); break;
    case ATT_MONEY:
      ((pmoney *) (vwmtd_table[procnum]))();   /* cannot join with the next stat. */
#ifndef LINUX
      _asm mov pom32,eax
      _asm mov pom16,dx
#else
      register int pom32 __asm__("eax");
      register sig16 pom16 __asm__("dx");
#endif
      retval.moneys.money_hi4=pom32;
      retval.moneys.money_lo2=pom16;
      //memcpy(&retval.moneys, retmoney, MONEY_SIZE);
      break;
    case ATT_INT32:    case ATT_DATE:    case ATT_TIME:  case ATT_CURSOR:
    case ATT_NIL:      case ATT_TIMESTAMP:
      retval.int32=        ((puns32 *) (vwmtd_table[procnum]))(); break;
    case ATT_INT64:
      retval.int64=        ((psig64 *) (vwmtd_table[procnum]))(); break;
    case ATT_FLOAT:
      retval.real =        ((pdouble*) (vwmtd_table[procnum]))(); break;
    case ATT_STRING:    case ATT_BINARY:
      retval.ptr  =        ((pstring*) (vwmtd_table[procnum]))(); break;
    case 0:                           (*vwmtd_table[procnum])();  break;
  }
#ifndef LINUX
  _asm mov esp,origsp
#else
  asm("movl %0, %%esp" : "=m"(origsp));
  //esp=origsp;
#endif
  if (tp) // preserve hyperbuf
    memcpy(cdp->RV.prev_run_state->hyperbuf, cdp->RV.hyperbuf, sizeof(cdp->RV.hyperbuf));
  wrestore_run(cdp);
  if (tp)  { cdp->RV.stt++;  *cdp->RV.stt=retval; }
}
#endif
#endif

void call_dll(cdp_t cdp, dllprocdef * def, scopevars * parptr)
{ unsigned localsize=parptr->localsize;  char * origsp;  FARPROC procadr;
  if (!def->procadr)   /* function not called yet, must find its address */
  { HINSTANCE hDllInst=0;  char libname[MAX_LIBRARY_NAME+1];
   // perform the name translation:
    strmaxcpy(libname, def->names, sizeof(libname));
#ifdef LINUX  // replace ".DLL" by ".so" (must be before name translation):
    int ln=strlen(libname);
    //for (char *ptr=libname; *ptr!='\0'; ptr++) *ptr=tolower(*ptr);
    if (ln>4 && !strcasecmp(libname+ln-4, ".dll")) strcpy(libname+ln-4, ".so");
#endif
    translate(libname, "wbkernel", KRNL_LIB_NAME);
    translate(libname, "wbprezen", "602prez8");
    translate(libname, "wbviewed", "602dvlp8");
    translate(libname, "wbinet",   "602inet8");
    if (!strcmp("602inet8.so", libname)) strcpy(libname, "libcgi602.so.8"); 
   // search the library among the loaded libraries:
    libinfo * alib = cdp->RV.dllinfo;
    while (alib)    
    { if (!my_stricmp(alib->libname, libname))
        { hDllInst=alib->hLibInst;  break; }
      alib=alib->next_lib;
    }
    if (!hDllInst)   /* library not loaded yet */
    { hDllInst=LoadLibrary(libname);
      if (!hDllInst)
        { corefree(parptr);  SET_ERR_MSG(cdp, libname);  r_error(DLL_NOT_FOUND); }
     // add the library into the list of loaded libraries:
      alib=(libinfo*)corealloc(sizeof(libinfo), 53);
      if (!alib)
        { FreeLibrary(hDllInst);  corefree(parptr);  r_error(OUT_OF_R_MEMORY); }
      strcpy(alib->libname, libname);
      alib->hLibInst=hDllInst;
      alib->next_lib=cdp->RV.dllinfo;
      cdp->RV.dllinfo=alib;
    }
    tptr procname = def->names+strlen(def->names)+1;
#ifdef WINS
    def->procadr=GetProcAddress(hDllInst, procname);
    if (!def->procadr)  /* try the alternative names, add A or _...@num */
    { int len=(int)strlen(procname);
      tptr alt_name=(tptr)corealloc(1+len+1+1+4+1, 53);
      if (alt_name!=NULL)
      { BOOL try_suffix = len && procname[len-1]!='A' && procname[len-1]!='W';
        if (try_suffix)
        { strcpy(alt_name, procname);  strcat(alt_name, "A");
          def->procadr=GetProcAddress(hDllInst, alt_name);
        }
        if (!def->procadr)
        { *alt_name='_';  strcpy(alt_name+1, procname);  strcat(alt_name, "@");
          int2str(def->parsize, alt_name+len+2);
          def->procadr=GetProcAddress(hDllInst, alt_name);
        }
        if (!def->procadr && try_suffix)
        { strcat(alt_name, "A");
          def->procadr=GetProcAddress(hDllInst, alt_name);
        }
        corefree(alt_name);
      }
    }
#else
    def->procadr=(FARPROC) dlsym(hDllInst, procname);
#endif
    if (!def->procadr) { corefree(parptr);  SET_ERR_MSG(cdp, procname);  r_error(FUNCTION_NOT_FOUND); }
  }
  procadr=def->procadr;
 // allocate the stack frame and copy params into it:
  { void *  pars;
#ifndef LINUX
         _asm {
                        mov origsp,esp
                        sub esp,localsize
                        mov pars,esp
                                          }
#elif !defined __ia64__
	 //register char* esp __asm__("esp");
	 //origsp=esp;
	 //esp-=localsize;
	 //pars=esp;
  asm("movl %%esp, %0" : "=m"(origsp));
  asm("subl %0, %%esp" : "=m"(localsize)) ;
  asm("movl %%esp, %0" : "=m"(pars));
#endif
#ifndef __ia64__
 	 memcpy(pars, &parptr->vars, localsize);
#endif
  }
 // save the upper run:
  if (wsave_run(cdp)) 
  { corefree(parptr);  
    r_error(OUT_OF_R_MEMORY);
  }
 // call it:
#ifdef __ia64__
  retval.int64=do_call_routine(localsize/8, (any_fnc*)procadr, (long *)&parptr->vars);
#else
  switch (def->fnctype)
  { case ATT_BOOLEAN:  case ATT_CHAR:
      retval.int32=(sig32) ((puns8  *) procadr)();  break;
    case ATT_INT8:
      retval.int32=(sig32) ((psig8  *) procadr)();  break;
    case ATT_INT16:
      retval.int32=(sig32) ((puns16 *) procadr)();  break;
    case ATT_MONEY:
      retmoney    =        ((pmoney *) procadr)();   /* cannot join with the next stat. */
      memcpy(&retval.moneys, retmoney, MONEY_SIZE);                break;
    case ATT_INT32:    case ATT_DATE:    case ATT_TIME:
      retval.int32=        ((puns32 *) procadr)();  break;
    case ATT_INT64:
      retval.int64=        ((psig64 *) procadr)();  break;
    case ATT_FLOAT:
      retval.real =        ((pdouble*) procadr)();  break;
    case ATT_STRING:
      retval.ptr  =        ((pstring*) procadr)();  break;
    case 0:                          (*procadr)();  break;
  }
#endif
  wrestore_run(cdp);
#ifndef LINUX
  _asm mov esp,origsp  // destroy the stack frame
#elif !defined(__ia64__)
	 //register char * esp __asm__("esp");
	 //esp=origsp;
  asm("movl %0, %%esp" : "=m"(origsp));
#endif
  if (def->fnctype) { cdp->RV.stt++;  *cdp->RV.stt=retval; }
}

#pragma optimize("", on)
#pragma runtime_checks("s", restore)
#endif

