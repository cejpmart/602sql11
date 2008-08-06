/****************************************************************************/
/* kerbase.cpp - 602krnlX.dll main module                                   */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#if WBVERS<1000  // moved to cint in 10.0
#include "dynar.cpp"
#include "wbmail.cpp"
#include "dwordptr.cpp"
#include "flstr.h"
#include "flstr.cpp"
#endif

HINSTANCE hKernelLibInst;

#ifdef _MSC_VER
CFNC BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
#else
#define _CRT_INIT(a,b,c)   1
#endif

CFNC BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
  switch (fdwReason)
  { case DLL_PROCESS_ATTACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      hKernelLibInst=hInstance;
      compiler_init();  /* the local compiler strucures initialized */
      break;
    case DLL_THREAD_ATTACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
    case DLL_PROCESS_DETACH:
      core_release();
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
    case DLL_THREAD_DETACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
  }
  return TRUE;
}

CFNC DllKernel int CALLBACK oexport WEP(int nExitType)
{ return 1; }  /* MSC requires this */

//////////////////////////////////////////////// ODMA interface /////////////////////////////////
#if WBVERS<1000
#include "odma.h"
#include "odmacom.h"

typedef HRESULT   WINAPI t_ODMQueryInterface(ODMHANDLE odmHandle, LPSTR lpszDocId, REFIID riid, LPVOID *ppvObj);
typedef ODMSTATUS WINAPI t_ODMRegisterApp(ODMHANDLE *pOdmHandle, WORD version, LPSTR lpszAppId, DWORD dwEnvData, LPVOID pReserved);
typedef void      WINAPI t_ODMUnRegisterApp(ODMHANDLE odmHandle);


CFNC DllKernel BOOL WINAPI ManageODMARepositories(HWND hParent)
{ HINSTANCE hODMAInst = LoadLibrary("ODMA32.DLL");
  if (!hODMAInst) return FALSE;
  t_ODMRegisterApp    * p_ODMRegisterApp    = (t_ODMRegisterApp   *)GetProcAddress(hODMAInst, "ODMRegisterApp");
  t_ODMQueryInterface * p_ODMQueryInterface = (t_ODMQueryInterface*)GetProcAddress(hODMAInst, "ODMQueryInterface");
  t_ODMUnRegisterApp  * p_ODMUnRegisterApp  = (t_ODMUnRegisterApp *)GetProcAddress(hODMAInst, "ODMUnRegisterApp");
  if (!p_ODMRegisterApp || !p_ODMQueryInterface || !p_ODMUnRegisterApp)
    goto err0;
  ODMHANDLE odmhandle;
  if ((*p_ODMRegisterApp)(&odmhandle, 150, "602SQLClient", (DWORD)hParent, NULL) == ODM_SUCCESS)
  { IODMWinBase * iODMWinBase;
    (*p_ODMQueryInterface)(odmhandle, "::ODMA\\602SQL\\TestRepository\\1", IID_IODMWinBase, (LPVOID*)&iODMWinBase);
    iODMWinBase->ManageRepositories();
    (*p_ODMUnRegisterApp)(odmhandle);
  }
  return TRUE;
 err0:
  FreeLibrary(hODMAInst);
  return FALSE;
}

#endif


