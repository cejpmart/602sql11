/*
** DLL.C - This is the ODBC driver code for LIBMAIN processing.
zmena zmena*/

#include "wbdrv.h"
#pragma hdrstop
                            
HINSTANCE s_hModule;		  // Saved module handle.
HINSTANCE hKernelLibInst; // support for client modules

#ifdef _MSC_VER
CFNC BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
#else
#define _CRT_INIT(a,b,c)   1
#endif

CFNC int __stdcall DllEntryPoint(HINSTANCE hInst,DWORD fdwReason,LPVOID lpvReserved)
{ switch (fdwReason)
	{ case DLL_PROCESS_ATTACH:	// case of libentry call in win 3.x
      if (!_CRT_INIT(hInst, fdwReason, lpvReserved)) return FALSE;
     	hKernelLibInst = s_hModule = hInst;
			break;
		case DLL_THREAD_ATTACH:
      if (!_CRT_INIT(hInst, fdwReason, lpvReserved)) return FALSE;
			break;
  	case DLL_PROCESS_DETACH:	// case of wep call in win 3.x
      if (!_CRT_INIT(hInst, fdwReason, lpvReserved)) return FALSE;
			break;
  	case DLL_THREAD_DETACH:
      if (!_CRT_INIT(hInst, fdwReason, lpvReserved)) return FALSE;
			break;
		default:
			break;
	} /* switch */
  return TRUE;
} 

#if WBVERS<900
ltable::~ltable(void) { }

CFNC int WINAPI check_atr_name(tptr name)
{ return 0; }

#else

#if WBVERS<1000  // moved to cint in 10.0
#include "dynar.cpp"
#include "wbmail.cpp"
#include "flstr.h"
#include "flstr.cpp"
#endif

#endif