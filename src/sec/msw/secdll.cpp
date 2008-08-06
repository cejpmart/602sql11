// DLL entry point for the SEC dll
#ifdef WINS
#include <windows.h>
#else
#include "winreplsecur.h"
#include <winrepl.h>
#endif
#include <stdlib.h> // NULL
#include <stdio.h>
#include "cmem.h"
#include "pkcsasn.h"
#include "secur.h"
#pragma hdrstop
#include "wbvers.h"

extern "C" BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{ switch (fdwReason)
  { case DLL_PROCESS_ATTACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      hProgInst=hInstance;
      break;
    case DLL_THREAD_ATTACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
    case DLL_PROCESS_DETACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
    case DLL_THREAD_DETACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
  }
  return TRUE;
}
      
