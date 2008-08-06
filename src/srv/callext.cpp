/****************************************************************************/
/* Non-optimized part of calling functions from external libraries          */
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

//////////////////////////////// calling stored procedures ///////////////////////////////
typedef sig64    WINAPI pfsig64 (void);
typedef sig32    WINAPI pfsig32 (void);
typedef sig16    WINAPI pfsig16 (void);
typedef uns8     WINAPI pfuns8  (void);
typedef sig8     WINAPI pfsig8  (void);
typedef double   WINAPI pfdouble(void);
typedef char   * WINAPI pfstring(void);
typedef monstr * WINAPI pfmoney (void);

#ifdef LINUX
#include <setjmp.h>
#include <signal.h>
#include <dlfcn.h>

static pthread_key_t longjmp_data;
static pthread_once_t pthread_once_r=PTHREAD_ONCE_INIT;
void bad_signal(int sig)
{
    jmp_buf *jump=(jmp_buf *) pthread_getspecific(longjmp_data);
    if (jump==NULL) UnloadProc(sig);/* neni kam skocit */
    //void *retaddr;  J.D. removed
    char buff[50];
    sprintf(buff, "%s at %x",strsignal(sig), NULL);
    dbg_err(buff);
    longjmp(*jump, 1);
}

void key_alloc()
{
    pthread_key_create(&longjmp_data, NULL);
}
#endif




#ifdef X64ARCH

#define MAX_PARAMS 9
typedef uns64 WINAPI proc0(void);
typedef uns64 WINAPI proc1(uns64 p1);
typedef uns64 WINAPI proc2(uns64 p1, uns64 p2);
typedef uns64 WINAPI proc3(uns64 p1, uns64 p2, uns64 p3);
typedef uns64 WINAPI proc4(uns64 p1, uns64 p2, uns64 p3, uns64 p4);
typedef uns64 WINAPI proc5(uns64 p1, uns64 p2, uns64 p3, uns64 p4, uns64 p5);
typedef uns64 WINAPI proc6(uns64 p1, uns64 p2, uns64 p3, uns64 p4, uns64 p5, uns64 p6);
typedef uns64 WINAPI proc7(uns64 p1, uns64 p2, uns64 p3, uns64 p4, uns64 p5, uns64 p6, uns64 p7);
typedef uns64 WINAPI proc8(uns64 p1, uns64 p2, uns64 p3, uns64 p4, uns64 p5, uns64 p6, uns64 p7, uns64 p8);
typedef uns64 WINAPI proc9(uns64 p1, uns64 p2, uns64 p3, uns64 p4, uns64 p5, uns64 p6, uns64 p7, uns64 p8, uns64 p9);

void * call_external_routine(cdp_t cdp, t_routine * croutine, FARPROC procptr, const t_scope * pscope, t_rscope * rscope, void * valbuf)
{ int i;
  uns64 parvals[MAX_PARAMS];  int parnum=0;
  for (i=0;  i<pscope->locdecls.count();  i++)
  { const t_locdecl * locdecl = pscope->locdecls.acc0(i);
    if      (locdecl->loccat==LC_INPAR || locdecl->loccat==LC_INPAR_)
    { if (parnum>=MAX_PARAMS) 
        return NULL;  // cannot call
      int thisparsize = simple_type_size(locdecl->var.type, locdecl->var.specif);
      parvals[parnum]=0;
      if (thisparsize<=8)  // can pass by value
        memcpy(parvals+parnum, rscope->data+locdecl->var.offset, thisparsize);
      parnum++;
    }
    else if (locdecl->loccat==LC_OUTPAR || locdecl->loccat==LC_OUTPAR_ || locdecl->loccat==LC_INOUTPAR || locdecl->loccat==LC_FLEX)
    { if (parnum>=MAX_PARAMS) 
        return NULL;  // cannot call
      if (IS_HEAP_TYPE(locdecl->var.type) && locdecl->loccat!=LC_FLEX) // fixed size
      { t_value * val = (t_value*)(rscope->data+locdecl->var.offset);
        parvals[parnum] = (uns64)val->valptr();
      }
      else  // reference, including flexible BLOB/CLOB
        parvals[parnum] = (uns64)(rscope->data+locdecl->var.offset);
      parnum++;
    }
  }
 // call it:
  uns64 resval;
  cdp->in_routine++;  
  switch (parnum)
  { case 0:
      resval=((proc0*)procptr)();  break;
    case 1:
      resval=((proc1*)procptr)(parvals[0]);  break;
    case 2:
      resval=((proc2*)procptr)(parvals[0], parvals[1]);  break;
    case 3:
      resval=((proc3*)procptr)(parvals[0], parvals[1], parvals[2]);  break;
    case 4:
      resval=((proc4*)procptr)(parvals[0], parvals[1], parvals[2], parvals[3]);  break;
    case 5:
      resval=((proc5*)procptr)(parvals[0], parvals[1], parvals[2], parvals[3], parvals[4]);  break;
    case 6:
      resval=((proc6*)procptr)(parvals[0], parvals[1], parvals[2], parvals[3], parvals[4], parvals[5]);  break;
    case 7:
      resval=((proc7*)procptr)(parvals[0], parvals[1], parvals[2], parvals[3], parvals[4], parvals[5], parvals[6]);  break;
    case 8:
      resval=((proc8*)procptr)(parvals[0], parvals[1], parvals[2], parvals[3], parvals[4], parvals[5], parvals[6], parvals[7]);  break;
    case 9:
      resval=((proc9*)procptr)(parvals[0], parvals[1], parvals[2], parvals[3], parvals[4], parvals[5], parvals[6], parvals[7], parvals[8]);  break;
  }
  cdp->in_routine--;  
  int ressize = simple_type_size(croutine->rettype, croutine->retspecif);
  if (ressize<=8) memcpy(valbuf, &resval, ressize);
  return valbuf;
}
#elif defined(LINUX) &&  defined(__i386__)

void * call_external_routine(cdp_t cdp, t_routine * croutine, FARPROC procptr, const t_scope * pscope, t_rscope * rscope, void * valbuf)
{ int i;
  int ext_offset;
  void * retptr = valbuf;
 // create parameters for external routine:
  char * extpars = (char*)corealloc(croutine->external_frame_size, 55);
 // set external param values:
  ext_offset=0;
  for (i=0;  i<pscope->locdecls.count();  i++)
  { t_locdecl * locdecl = pscope->locdecls.acc0(i);
    if      (locdecl->loccat==LC_INPAR || locdecl->loccat==LC_INPAR_)
    { int thisparsize = simple_type_size(locdecl->var.type, locdecl->var.specif);
      int thisparsizefull = ((thisparsize-1)/sizeof(int)+1)*sizeof(int);
      memset(extpars+ext_offset, 0, thisparsizefull);
      memcpy(extpars+ext_offset, rscope->data+locdecl->var.offset, thisparsize);
      ext_offset+=thisparsizefull;
    }

    else if (locdecl->loccat==LC_OUTPAR || locdecl->loccat==LC_OUTPAR_ || locdecl->loccat==LC_INOUTPAR || locdecl->loccat==LC_FLEX)
    { if (IS_HEAP_TYPE(locdecl->var.type) && locdecl->loccat!=LC_FLEX) // fixed size
      { t_value * val = (t_value*)(rscope->data+locdecl->var.offset);
        *(void**)(extpars+ext_offset) = val->valptr();
      }
      else  // reference, including flexible BLOB/CLOB
        *(void**)(extpars+ext_offset) = rscope->data+locdecl->var.offset;
      ext_offset+=sizeof(void*);
    }
  }
  cdp->in_routine++;
  char * epars, * origsp;
 // call it:
  jmp_buf jump;
  pthread_once(&pthread_once_r, key_alloc);
  pthread_setspecific(longjmp_data, &jump);
  if (setjmp(jump)==0)
  {
   // move params to the stack (no local variable must be declared after this!!):
    ext_offset = croutine->external_frame_size;
    asm("movl %%esp, %0" : "=m"(origsp));
    asm("subl %0, %%esp" : "=m"(ext_offset)) ;
    asm("movl %%esp, %0" : "=m"(epars));
    memcpy(epars, extpars, ext_offset);
    switch (croutine->rettype)
    { case ATT_BOOLEAN:  case ATT_CHAR:
        *(uns32*)valbuf = (uns32) ((pfuns8  *) (procptr))();  break;
      case ATT_INT8:
        *(sig32*)valbuf = (sig32) ((pfsig8  *) (procptr))();  break;
      case ATT_INT16:
        *(sig32*)valbuf = (sig32) ((pfsig16 *) (procptr))();  break;
      case ATT_INT32:   case ATT_DATE:  case ATT_TIME:  case ATT_TIMESTAMP:
        *(sig32*)valbuf =         ((pfsig32 *) (procptr))();  break;
      case ATT_INT64:
        *(sig64*)valbuf =         ((pfsig64 *) (procptr))();  break;
      case ATT_FLOAT:
        *(double*)valbuf=         ((pfdouble*) (procptr))();  break;
      case ATT_MONEY:
        retptr =       ((pfmoney *) (procptr))();  break;
      case ATT_STRING:  case ATT_BINARY:
        retptr =       ((pfstring*) (procptr))();  break;
      case 0:                       (*procptr)();  break;
    }
  }
  else {
	  char buff[100];
	  sprintf(buff, "Abnormal termination of dynamic library (%s)!", croutine->names);
	  dbg_err(buff);
	  retptr=NULL;
  }
 // destroy external parameters:
  asm("movl %0, %%esp" : "=m"(origsp));
  cdp->in_routine--;
  corefree(extpars);
  return retptr;
}



#else
#ifdef __ia64__
extern "C" {
typedef long any_fnc();
long do_call_routine(int n, any_fnc fnc, long*args);
}
#endif

void * call_external_routine(cdp_t cdp, t_routine * croutine, FARPROC procptr, const t_scope * pscope, t_rscope * rscope, void * valbuf)
{ int i;
  int ext_offset;
  void * retptr = valbuf;
 // create parameters for external routine:
  ext_offset = croutine->external_frame_size;
#if defined ( __WATCOMC__)
  static DWORD epars, origsp;   // bad, but asm does not translate otherwise
#else
  char * epars, * origsp;
#endif
#ifndef LINUX
  _asm {
        mov origsp,esp
        sub esp,ext_offset
        mov epars,esp
  }
#elif defined __i386__ // Jiny asembler, jine konvence.
  //register char * esp __asm__("esp");
  //origsp=esp;
  //esp-=ext_offset;
  //epars=esp;
  asm("movl %%esp, %0" : "=m"(origsp));
  asm("subl %0, %%esp" : "=m"(ext_offset)) ;
  asm("movl %%esp, %0" : "=m"(epars));
#elif defined __ia64__
  /* Na itaniu je to jeste jinak */
#define MAXPARS 9 /* Vic parametru je spatne */
#define ext_offset (i*sizeof(long))
#define extpars (char *)param_longvalues
  register double fpar __asm__("f8"); /* v techto registrech se predavaji
					 vstupni float parametry. */
  register double fpar2 __asm__("f9");
  register double fpar3 __asm__("f10");
  register double fpar4 __asm__("f11");
  int fpars=0; /* pocet uzitych float parametru */
  long param_longvalues[MAXPARS];
#endif
 // set external param values:
#ifndef __ia64__
  tptr extpars = (tptr)epars;
  ext_offset=0;
#endif
  for (i=0;  i<pscope->locdecls.count();  i++)
  { t_locdecl * locdecl = pscope->locdecls.acc0(i);
    if      (locdecl->loccat==LC_INPAR || locdecl->loccat==LC_INPAR_)
    { int thisparsize = simple_type_size(locdecl->var.type, locdecl->var.specif);
#ifndef __ia64__
	    int thisparsizefull = ((thisparsize-1)/sizeof(int)+1)*sizeof(int);
	    memset(extpars+ext_offset, 0, thisparsizefull);
      memcpy(extpars+ext_offset, rscope->data+locdecl->var.offset, thisparsize);
      ext_offset+=thisparsizefull;
#else /* Je to itanium, jen to ulozime: */
      int slots=(thisparsize-1)/sizeof(long); /* pocet slotu minus 1*/
      assert(i+slots<MAXPARS);
      param_longvalues[i+slots]=0;
      memcpy(param_longvalues+i, rscope->data+locdecl->var.offset, thisparsize);
      i+=slots; /* ++ je v cyklu */
      if (ATT_FLOAT==locdecl->var.type){
	double val=*(double *)rscope->data+locdecl->var.offset;
	switch (fpars++){
	case 0:
	  fpar=val;
	  break;
	case 1:
	  fpar2=val;
	  break;
	case 2:
	  fpar3=val;
	  break;
	case 3:
	  fpar4=val;
	  break;
	default:
	  assert(0);
	  break;
	}
    }
#endif
    }

    else if (locdecl->loccat==LC_OUTPAR || locdecl->loccat==LC_OUTPAR_ || locdecl->loccat==LC_INOUTPAR || locdecl->loccat==LC_FLEX)
    { if (IS_HEAP_TYPE(locdecl->var.type) && locdecl->loccat!=LC_FLEX) // fixed size
      { t_value * val = (t_value*)(rscope->data+locdecl->var.offset);
        *(void**)(extpars+ext_offset) = val->valptr();
      }
      else  // reference, including flexible BLOB/CLOB
        *(void**)(extpars+ext_offset) = rscope->data+locdecl->var.offset;
#ifndef __ia64__
      ext_offset+=sizeof(void*);
#endif
    }
  }
 // call it:
  cdp->in_routine++;  
#ifdef WINS
  __try
#else
  jmp_buf jump;
  pthread_once(&pthread_once_r, key_alloc);
  pthread_setspecific(longjmp_data, &jump);
  if (setjmp(jump)==0)
#endif

#ifdef __ia64__
  {
    long res=do_call_routine(i, (any_fnc*)procptr, param_longvalues);
    if (croutine->rettype==ATT_FLOAT) res=(long)fpar;
    memcpy(valbuf, &res, 8);
  }
#else
  { switch (croutine->rettype)
    { case ATT_BOOLEAN:  case ATT_CHAR:
        *(uns32*)valbuf = (uns32) ((pfuns8  *) (procptr))();  break;
      case ATT_INT8:
        *(sig32*)valbuf = (sig32) ((pfsig8  *) (procptr))();  break;
      case ATT_INT16:
        *(sig32*)valbuf = (sig32) ((pfsig16 *) (procptr))();  break;
      case ATT_INT32:   case ATT_DATE:  case ATT_TIME:  case ATT_TIMESTAMP:
        *(sig32*)valbuf =         ((pfsig32 *) (procptr))();  break;
      case ATT_INT64:
        *(sig64*)valbuf =         ((pfsig64 *) (procptr))();  break;
      case ATT_FLOAT:
        *(double*)valbuf=         ((pfdouble*) (procptr))();  break;
      case ATT_MONEY:
        retptr =       ((pfmoney *) (procptr))();  break;
      case ATT_STRING:  case ATT_BINARY:
        retptr =       ((pfstring*) (procptr))();  break;
      case 0:                       (*procptr)();  break;
    }
  }
#endif
#ifdef WINS
  __except ((GetExceptionCode() != 0) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
  { char buf[100];
    sprintf(buf, ">> Exception %X occured in %s.", GetExceptionCode(), croutine->names);
    dbg_err(buf);
    retptr=NULL;  
  }
#else
  else {
	  char buff[100];
	  sprintf(buff, "Abnormal termination of dynamic library (%s)!", croutine->names);
	  dbg_err(buff);
	  retptr=NULL;
  }
#endif
  cdp->in_routine--;
 // destroy external parameters:
#ifndef LINUX
  _asm mov esp,origsp
#elif !defined __ia64__
  asm("movl %0, %%esp" : "=m"(origsp));
	  //esp=origsp;
#endif
  return retptr;
}

#endif  // !X64ARCH
