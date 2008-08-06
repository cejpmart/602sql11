//#pragma warning( disable: 4786 )

#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

// imagehlp.h must be compiled with packing to eight-byte-boundaries,
// but does nothing to enforce that. I am grateful to Jeff Shanholtz
// <JShanholtz@premia.com> for finding this problem.
#pragma pack( push, before_imagehlp, 8 )
#include <imagehlp.h>
#pragma pack( pop, before_imagehlp )


#define gle (GetLastError())
#define lenof(a) (sizeof(a) / sizeof((a)[0]))
#define MAXNAMELEN 1024 // max name length for found symbols
#define IMGSYMLEN ( sizeof IMAGEHLP_SYMBOL )
#define TTBUFLEN 65536 // for a temp buffer



// SymCleanup()
typedef BOOL (__stdcall *tSC)( IN HANDLE hProcess );
tSC pSC = NULL;

// SymFunctionTableAccess()
typedef PVOID (__stdcall *tSFTA)( HANDLE hProcess, DWORD AddrBase );
tSFTA pSFTA = NULL;

// SymGetLineFromAddr()
typedef BOOL (__stdcall *tSGLFA)( IN HANDLE hProcess, IN DWORD dwAddr,
	OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE Line );
tSGLFA pSGLFA = NULL;

// SymGetModuleBase()
typedef DWORD (__stdcall *tSGMB)( IN HANDLE hProcess, IN DWORD dwAddr );
tSGMB pSGMB = NULL;

// SymGetModuleInfo()
typedef BOOL (__stdcall *tSGMI)( IN HANDLE hProcess, IN DWORD dwAddr, OUT PIMAGEHLP_MODULE ModuleInfo );
tSGMI pSGMI = NULL;

// SymGetOptions()
typedef DWORD (__stdcall *tSGO)( VOID );
tSGO pSGO = NULL;

// SymGetSymFromAddr()
typedef BOOL (__stdcall *tSGSFA)( IN HANDLE hProcess, IN DWORD dwAddr,
	OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_SYMBOL Symbol );
tSGSFA pSGSFA = NULL;

// SymInitialize()
typedef BOOL (__stdcall *tSI)( IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess );
tSI pSI = NULL;

// SymLoadModule()
typedef DWORD (__stdcall *tSLM)( IN HANDLE hProcess, IN HANDLE hFile,
	IN PSTR ImageName, IN PSTR ModuleName, IN DWORD BaseOfDll, IN DWORD SizeOfDll );
tSLM pSLM = NULL;

// SymSetOptions()
typedef DWORD (__stdcall *tSSO)( IN DWORD SymOptions );
tSSO pSSO = NULL;

// StackWalk()
typedef BOOL (__stdcall *tSW)( DWORD MachineType, HANDLE hProcess,
	HANDLE hThread, LPSTACKFRAME StackFrame, PVOID ContextRecord,
	PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
	PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
	PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
	PTRANSLATE_ADDRESS_ROUTINE TranslateAddress );
tSW pSW = NULL;

// UnDecorateSymbolName()
typedef DWORD (__stdcall WINAPI *tUDSN)( PCSTR DecoratedName, PSTR UnDecoratedName,
	DWORD UndecoratedLength, DWORD Flags );
tUDSN pUDSN = NULL;



struct ModuleEntry
{
	std::string imageName;
	std::string moduleName;
	DWORD baseAddress;
	DWORD size;
};
typedef std::vector< ModuleEntry > ModuleList;
typedef ModuleList::iterator ModuleListIter;

void proc_prep(void);
void write_frames(HANDLE hThread, CONTEXT& c, int clinum, int fixnum);
void close_process(void);
DWORD __stdcall TargetThread( void *arg );
void ThreadFunc1();
void ThreadFunc2();
DWORD Filter( EXCEPTION_POINTERS *ep );
void enumAndLoadModuleSymbols( HANDLE hProcess, DWORD pid );
bool fillModuleList( ModuleList& modules, DWORD pid, HANDLE hProcess );
bool fillModuleListTH32( ModuleList& modules, DWORD pid );
bool fillModuleListPSAPI( ModuleList& modules, DWORD pid, HANDLE hProcess );


static HINSTANCE hImagehlpDll = 0;
static FILE * logfile = NULL;
static bool prepared = false;

void stackwalk_unprepare( void )
{ 
  if (prepared)
  { SetUnhandledExceptionFilter(NULL);
    prepared=false;
  }  
  if (hImagehlpDll) { FreeLibrary( hImagehlpDll );  hImagehlpDll=0; }
  if (logfile) { fclose(logfile);  logfile=0; }
}

LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS * ep)
{	HANDLE hThread;  int i;
  fprintf(logfile, "\n\n\n");
  proc_prep();
	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(),
		GetCurrentProcess(), &hThread, 0, false, DUPLICATE_SAME_ACCESS );
	cdp_t acdp;	
	for (i = 1;  i<=max_task_index;  i++)
	{ acdp = cds[i];
	  if (acdp)
	    if (acdp->threadID == GetCurrentThreadId())
	      break;
  }
  write_frames(hThread, *(ep->ContextRecord), i<=max_task_index ? acdp->applnum : -1, i<=max_task_index ? acdp->fixnum: -1 );
// dump the other threads:	
	for (i = 1;  i<=max_task_index;  i++)
	{ acdp = cds[i];
	  if (acdp)
	    if (acdp->threadID != GetCurrentThreadId())
	    { CONTEXT c;
      	memset( &c, '\0', sizeof c );
	      c.ContextFlags = CONTEXT_FULL;
	      GetThreadContext(acdp->thread_handle, &c);
        write_frames(acdp->thread_handle, c, acdp->applnum, acdp->fixnum);
      }  
	}
	close_process();
	ResumeThread( hThread );
	CloseHandle( hThread );
  stackwalk_unprepare();  // otherwise the enf of the log file contents is missing
  return EXCEPTION_CONTINUE_SEARCH;
}

bool stackwalk_prepare(void)
{// prepare the file name:
  char fname[MAX_PATH];
  strcpy(fname, ffa.last_fil_path()); // log in the FIL directory
  append(fname, "602sqlCrashReport.txt");
 // open the log file:
  logfile = fopen(fname, "a");
  if (!logfile) return false;
	// we load imagehlp.dll dynamically because the NT4-version does not
	// offer all the functions that are in the NT5 lib
	hImagehlpDll = LoadLibrary( "imagehlp.dll" );
	if ( hImagehlpDll == NULL )
	  fprintf(logfile, "LoadLibrary( \"imagehlp.dll\" ): gle = %lu\n", gle );
  else
	{ pSC = (tSC) GetProcAddress( hImagehlpDll, "SymCleanup" );
	  pSFTA = (tSFTA) GetProcAddress( hImagehlpDll, "SymFunctionTableAccess" );
	  pSGLFA = (tSGLFA) GetProcAddress( hImagehlpDll, "SymGetLineFromAddr" );
	  pSGMB = (tSGMB) GetProcAddress( hImagehlpDll, "SymGetModuleBase" );
	  pSGMI = (tSGMI) GetProcAddress( hImagehlpDll, "SymGetModuleInfo" );
	  pSGO = (tSGO) GetProcAddress( hImagehlpDll, "SymGetOptions" );
	  pSGSFA = (tSGSFA) GetProcAddress( hImagehlpDll, "SymGetSymFromAddr" );
	  pSI = (tSI) GetProcAddress( hImagehlpDll, "SymInitialize" );
	  pSSO = (tSSO) GetProcAddress( hImagehlpDll, "SymSetOptions" );
	  pSW = (tSW) GetProcAddress( hImagehlpDll, "StackWalk" );
	  pUDSN = (tUDSN) GetProcAddress( hImagehlpDll, "UnDecorateSymbolName" );
	  pSLM = (tSLM) GetProcAddress( hImagehlpDll, "SymLoadModule" );

	  if ( pSC == NULL || pSFTA == NULL || pSGMB == NULL || pSGMI == NULL ||
         pSGO == NULL || pSGSFA == NULL || pSI == NULL || pSSO == NULL ||
		     pSW == NULL || pUDSN == NULL || pSLM == NULL )
		  fprintf(logfile, "GetProcAddress(): some required function not found." );
    else
    { SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
      prepared=true;
      return true;  // prepared OK
    }  
    FreeLibrary( hImagehlpDll );
  }
  fclose(logfile);
  return false;
}


/*
int threadAbortFlag = 0;
HANDLE hTapTapTap = NULL;

	HANDLE hThread;
	DWORD idThread;
	hTapTapTap = CreateEvent( NULL, false, false, NULL );

	hThread = CreateThread( NULL, 5*524288, TargetThread, NULL, 0, &idThread );
	// let the thread complete the exception-handler part
	// and yes, I know I should check the return code
	WaitForSingleObject( hTapTapTap, INFINITE );
	CloseHandle( hTapTapTap );

	// time to stop the victim
	if ( (DWORD) (-1L) == SuspendThread( hThread ) )
	{
		printf( "SuspendThread(): gle = %lu\n", gle );
		goto cleanup;
	}

	CONTEXT c;
	memset( &c, '\0', sizeof c );
	c.ContextFlags = CONTEXT_FULL;

	// init CONTEXT record so we know where to start the stackwalk
	if ( ! GetThreadContext( hThread, &c ) )
	{
		printf( "GetThreadContext(): gle = %lu\n", gle );
		goto cleanup;
	}

	ShowStack( hThread, c );

cleanup:
	++ threadAbortFlag;
	Sleep( 100 ); // to let the thread commit suicide
	FreeLibrary( hImagehlpDll );
	return 0;
}



// if you use C++ exception handling: install a translator function
// with set_se_translator(). In the context of that function (but *not*
// afterwards), you can either do your stack dump, or save the CONTEXT
// record as a local copy. Note that you must do the stack sump at the
// earliest opportunity, to avoid the interesting stackframes being gone
// by the time you do the dump.
DWORD Filter( EXCEPTION_POINTERS *ep )
{
	HANDLE hThread;

	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(),
		GetCurrentProcess(), &hThread, 0, false, DUPLICATE_SAME_ACCESS );
	ShowStack( hThread, *(ep->ContextRecord) );
	CloseHandle( hThread );

	return EXCEPTION_EXECUTE_HANDLER;
}



void ThreadFunc2()
{
	// first, we cause an exception and dump our own stack
	__try
	{
		char *p = NULL;
		*p = 'A'; // BANG!
	}
	__except ( Filter( GetExceptionInformation() ) )
	{
		printf( "Handled exception.\n" );
	}

	// phase 1 done, tell main thread we can be autopsied from the outside
	SetEvent( hTapTapTap );

	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_IDLE );
	for ( ; ! threadAbortFlag; )
	{
		Sleep( 10 );
	}
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
}



void ThreadFunc1()
{
	ThreadFunc2();
}



DWORD __stdcall TargetThread( void *arg )
{
	(void) arg;
	ThreadFunc1();
	return 0;
}

*/

void write_frames(HANDLE hThread, CONTEXT& c, int clinum, int fixnum)
{
	fprintf(logfile, "\nClient %d, has %d fixes.\n", clinum, fixnum);
  if (!prepared) return;
 	HANDLE hProcess = GetCurrentProcess(); // hProcess normally comes from outside
	DWORD imageType = IMAGE_FILE_MACHINE_I386;
	IMAGEHLP_SYMBOL *pSym = (IMAGEHLP_SYMBOL *) malloc( IMGSYMLEN + MAXNAMELEN );
	IMAGEHLP_LINE Line;
	IMAGEHLP_MODULE Module;
	int frameNum; // counts walked frames
	DWORD offsetFromSymbol; // tells us how far from the symbol we were
	char undName[MAXNAMELEN]; // undecorated name
	char undFullName[MAXNAMELEN]; // undecorated name with all shenanigans

	STACKFRAME s; // in/out stackframe
	memset( &s, '\0', sizeof s );

	// init STACKFRAME for first call
	// Notes: AddrModeFlat is just an assumption. I hate VDM debugging.
	// Notes: will have to be #ifdef-ed for Alphas; MIPSes are dead anyway,
	// and good riddance.
	s.AddrPC.Offset = c.Eip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Ebp;
	s.AddrFrame.Mode = AddrModeFlat;

	memset( pSym, '\0', IMGSYMLEN + MAXNAMELEN );
	pSym->SizeOfStruct = IMGSYMLEN;
	pSym->MaxNameLength = MAXNAMELEN;

	memset( &Line, '\0', sizeof Line );
	Line.SizeOfStruct = sizeof Line;

	memset( &Module, '\0', sizeof Module );
	Module.SizeOfStruct = sizeof Module;

	offsetFromSymbol = 0;

	fprintf(logfile, "\n--# FV EIP----- RetAddr- FramePtr StackPtr Symbol\n" );
	for ( frameNum = 0; ; ++ frameNum )
	{
		// get next stack frame (StackWalk(), SymFunctionTableAccess(), SymGetModuleBase())
		// if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
		// assume that either you are done, or that the stack is so hosed that the next
		// deeper frame could not be found.
		if ( ! pSW( imageType, hProcess, hThread, &s, &c, NULL,
			pSFTA, pSGMB, NULL ) )
			break;

		// display its contents
		fprintf(logfile, "\n%3d %c%c %08lx %08lx %08lx %08lx ",
			frameNum, s.Far? 'F': '.', s.Virtual? 'V': '.',
			s.AddrPC.Offset, s.AddrReturn.Offset,
			s.AddrFrame.Offset, s.AddrStack.Offset );

		if ( s.AddrPC.Offset == 0 )
		{
			fprintf(logfile, "(-nosymbols- PC == 0)\n" );
		}
		else
		{ // we seem to have a valid PC
			// show procedure info (SymGetSymFromAddr())
			if ( ! pSGSFA( hProcess, s.AddrPC.Offset, &offsetFromSymbol, pSym ) )
			{
				if ( gle != 487 )
					fprintf(logfile, "SymGetSymFromAddr(): gle = %lu\n", gle );
			}
			else
			{
				// UnDecorateSymbolName()
				pUDSN( pSym->Name, undName, MAXNAMELEN, UNDNAME_NAME_ONLY );
				pUDSN( pSym->Name, undFullName, MAXNAMELEN, UNDNAME_COMPLETE );
				fprintf(logfile, "%s", undName );
				if ( offsetFromSymbol != 0 )
					fprintf(logfile, " %+ld bytes", (long) offsetFromSymbol );
				fprintf(logfile, "    Sig:  %s\n", pSym->Name );
				fprintf(logfile, "    Decl: %s\n", undFullName );
			}

			// show line number info, NT5.0-method (SymGetLineFromAddr())
			if ( pSGLFA != NULL )
			{ // yes, we have SymGetLineFromAddr()
				if ( ! pSGLFA( hProcess, s.AddrPC.Offset, &offsetFromSymbol, &Line ) )
				{
					if ( gle != 487 )
						fprintf(logfile, "SymGetLineFromAddr(): gle = %lu\n", gle );
				}
				else
				{
					fprintf(logfile, "    Line: %s(%lu) %+ld bytes\n",
						Line.FileName, Line.LineNumber, offsetFromSymbol );
				}
			} // yes, we have SymGetLineFromAddr()

			// show module info (SymGetModuleInfo())
			if ( ! pSGMI( hProcess, s.AddrPC.Offset, &Module ) )
			{
				fprintf(logfile, "SymGetModuleInfo): gle = %lu\n", gle );
			}
			else
			{ // got module info OK
				char ty[80];
				switch ( Module.SymType )
				{
				case SymNone:
					strcpy( ty, "-nosymbols-" );
					break;
				case SymCoff:
					strcpy( ty, "COFF" );
					break;
				case SymCv:
					strcpy( ty, "CV" );
					break;
				case SymPdb:
					strcpy( ty, "PDB" );
					break;
				case SymExport:
					strcpy( ty, "-exported-" );
					break;
				case SymDeferred:
					strcpy( ty, "-deferred-" );
					break;
				case SymSym:
					strcpy( ty, "SYM" );
					break;
				default:
					_snprintf( ty, sizeof ty, "symtype=%ld", (long) Module.SymType );
					break;
				}

				fprintf(logfile, "    Mod:  %s[%s], base: %08lxh\n",
					Module.ModuleName, Module.ImageName, Module.BaseOfImage );
				fprintf(logfile, "    Sym:  type: %s, file: %s\n",
					ty, Module.LoadedImageName );
			} // got module info OK
		} // we seem to have a valid PC

		// no return address means no deeper stackframe
		if ( s.AddrReturn.Offset == 0 )
		{
			// avoid misunderstandings in the printf() following the loop
			SetLastError( 0 );
			break;
		}

	} // for ( frameNum )

	if ( gle != 0 )
		fprintf(logfile, "\nStackWalk(): gle = %lu\n", gle );
	free( pSym );
}

char *tt = 0;

void proc_prep(void)
{	// normally, call ImageNtHeader() and use machine info from PE header
  if (!prepared) return;
	HANDLE hProcess = GetCurrentProcess(); // hProcess normally comes from outside
	DWORD symOptions; // symbol handler settings
	std::string symSearchPath;
	char *p;
	uns32 tm = Now(), dt=Today();
	fprintf(logfile, "\n\nReporting on %u.%u.%u at %u:%u:%u.\n", Day(dt), Month(dt), Year(dt), Hours(tm), Minutes(tm), Seconds(tm));

	// NOTE: normally, the exe directory and the current directory should be taken
	// from the target process. The current dir would be gotten through injection
	// of a remote thread; the exe fir through either ToolHelp32 or PSAPI.

	tt = new char[TTBUFLEN]; // this is a _sample_. you can do the error checking yourself.

	// build symbol search path from:
	symSearchPath = "";
	// current directory
	if ( GetCurrentDirectory( TTBUFLEN, tt ) )
		symSearchPath += tt + std::string( ";" );
	// dir with executable
	if ( GetModuleFileName( 0, tt, TTBUFLEN ) )
	{
		for ( p = tt + strlen( tt ) - 1; p >= tt; -- p )
		{
			// locate the rightmost path separator
			if ( *p == '\\' || *p == '/' || *p == ':' )
				break;
		}
		// if we found one, p is pointing at it; if not, tt only contains
		// an exe name (no path), and p points before its first byte
		if ( p != tt ) // path sep found?
		{
			if ( *p == ':' ) // we leave colons in place
				++ p;
			*p = '\0'; // eliminate the exe name and last path sep
			symSearchPath += tt + std::string( ";" );
		}
	}
	// environment variable _NT_SYMBOL_PATH
	if ( GetEnvironmentVariable( "_NT_SYMBOL_PATH", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );
	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if ( GetEnvironmentVariable( "_NT_ALTERNATE_SYMBOL_PATH", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );
	// environment variable SYSTEMROOT
	if ( GetEnvironmentVariable( "SYSTEMROOT", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );

	if ( symSearchPath.size() > 0 ) // if we added anything, we have a trailing semicolon
		symSearchPath = symSearchPath.substr( 0, symSearchPath.size() - 1 );

	fprintf(logfile, "symbols path: %s\n", symSearchPath.c_str() );

	// why oh why does SymInitialize() want a writeable string?
	strncpy( tt, symSearchPath.c_str(), TTBUFLEN );
	tt[TTBUFLEN - 1] = '\0'; // if strncpy() overruns, it doesn't add the null terminator

	// init symbol handler stuff (SymInitialize())
	if ( ! pSI( hProcess, tt, false ) )
	{
		fprintf(logfile, "SymInitialize(): gle = %lu\n", gle );
		return;
	}

	// SymGetOptions()
	symOptions = pSGO();
	symOptions |= SYMOPT_LOAD_LINES;
	symOptions &= ~SYMOPT_UNDNAME;
	pSSO( symOptions ); // SymSetOptions()

	// Enumerate modules and tell imagehlp.dll about them.
	// On NT, this is not necessary, but it won't hurt.
	enumAndLoadModuleSymbols( hProcess, GetCurrentProcessId() );
}

void close_process(void)
{ 
  if (!prepared) return;
  HANDLE hProcess = GetCurrentProcess();
  pSC( hProcess );
	delete [] tt;
}

void enumAndLoadModuleSymbols( HANDLE hProcess, DWORD pid )
{
	ModuleList modules;
	ModuleListIter it;
	char *img, *mod;

	// fill in module list
	fillModuleList( modules, pid, hProcess );

	for ( it = modules.begin(); it != modules.end(); ++ it )
	{
		// unfortunately, SymLoadModule() wants writeable strings
		img = new char[(*it).imageName.size() + 1];
		strcpy( img, (*it).imageName.c_str() );
		mod = new char[(*it).moduleName.size() + 1];
		strcpy( mod, (*it).moduleName.c_str() );

		if ( pSLM( hProcess, 0, img, mod, (*it).baseAddress, (*it).size ) == 0 )
			fprintf(logfile, "Error %lu loading symbols for \"%s\"\n",
				gle, (*it).moduleName.c_str() );
		else
			fprintf(logfile, "Symbols loaded: \"%s\"\n", (*it).moduleName.c_str() );

		delete [] img;
		delete [] mod;
	}
}



bool fillModuleList( ModuleList& modules, DWORD pid, HANDLE hProcess )
{
	// try toolhelp32 first
	if ( fillModuleListTH32( modules, pid ) )
		return true;
	// nope? try psapi, then
	return fillModuleListPSAPI( modules, pid, hProcess );
}



// miscellaneous toolhelp32 declarations; we cannot #include the header
// because not all systems may have it
#define MAX_MODULE_NAME32 255
#define TH32CS_SNAPMODULE   0x00000008
#pragma pack( push, 8 )
typedef struct tagMODULEENTRY32
{
    DWORD   dwSize;
    DWORD   th32ModuleID;       // This module
    DWORD   th32ProcessID;      // owning process
    DWORD   GlblcntUsage;       // Global usage count on the module
    DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
    BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
    DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
    HMODULE hModule;            // The hModule of this module in th32ProcessID's context
    char    szModule[MAX_MODULE_NAME32 + 1];
    char    szExePath[MAX_PATH];
} MODULEENTRY32;
typedef MODULEENTRY32 *  PMODULEENTRY32;
typedef MODULEENTRY32 *  LPMODULEENTRY32;
#pragma pack( pop )



bool fillModuleListTH32( ModuleList& modules, DWORD pid )
{
	// CreateToolhelp32Snapshot()
	typedef HANDLE (__stdcall *tCT32S)( DWORD dwFlags, DWORD th32ProcessID );
	// Module32First()
	typedef BOOL (__stdcall *tM32F)( HANDLE hSnapshot, LPMODULEENTRY32 lpme );
	// Module32Next()
	typedef BOOL (__stdcall *tM32N)( HANDLE hSnapshot, LPMODULEENTRY32 lpme );

	// I think the DLL is called tlhelp32.dll on Win9X, so we try both
	const char *dllname[] = { "kernel32.dll", "tlhelp32.dll" };
	HINSTANCE hToolhelp;
	tCT32S pCT32S;
	tM32F pM32F;
	tM32N pM32N;

	HANDLE hSnap;
	MODULEENTRY32 me = { sizeof me };
	bool keepGoing;
	ModuleEntry e;
	int i;

	for ( i = 0; i < lenof( dllname ); ++ i )
	{
		hToolhelp = LoadLibrary( dllname[i] );
		if ( hToolhelp == 0 )
			continue;
		pCT32S = (tCT32S) GetProcAddress( hToolhelp, "CreateToolhelp32Snapshot" );
		pM32F = (tM32F) GetProcAddress( hToolhelp, "Module32First" );
		pM32N = (tM32N) GetProcAddress( hToolhelp, "Module32Next" );
		if ( pCT32S != 0 && pM32F != 0 && pM32N != 0 )
			break; // found the functions!
		FreeLibrary( hToolhelp );
		hToolhelp = 0;
	}

	if ( hToolhelp == 0 ) // nothing found?
		return false;

	hSnap = pCT32S( TH32CS_SNAPMODULE, pid );
	if ( hSnap == (HANDLE) -1 )
		return false;

	keepGoing = !!pM32F( hSnap, &me );
	while ( keepGoing )
	{
		// here, we have a filled-in MODULEENTRY32
		fprintf(logfile, "%08lXh %6lu %-15.15s %s\n", (DWORD)(size_t)me.modBaseAddr, me.modBaseSize, me.szModule, me.szExePath );
		e.imageName = me.szExePath;
		e.moduleName = me.szModule;
		e.baseAddress = (DWORD) (size_t)me.modBaseAddr;
		e.size = me.modBaseSize;
		modules.push_back( e );
		keepGoing = !!pM32N( hSnap, &me );
	}

	CloseHandle( hSnap );

	FreeLibrary( hToolhelp );

	return modules.size() != 0;
}



// miscellaneous psapi declarations; we cannot #include the header
// because not all systems may have it
typedef struct _MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD SizeOfImage;
    LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;



bool fillModuleListPSAPI( ModuleList& modules, DWORD pid, HANDLE hProcess )
{
	// EnumProcessModules()
	typedef BOOL (__stdcall *tEPM)( HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded );
	// GetModuleFileNameEx()
	typedef DWORD (__stdcall *tGMFNE)( HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize );
	// GetModuleBaseName() -- redundant, as GMFNE() has the same prototype, but who cares?
	typedef DWORD (__stdcall *tGMBN)( HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize );
	// GetModuleInformation()
	typedef BOOL (__stdcall *tGMI)( HANDLE hProcess, HMODULE hModule, LPMODULEINFO pmi, DWORD nSize );

	HINSTANCE hPsapi;
	tEPM pEPM;
	tGMFNE pGMFNE;
	tGMBN pGMBN;
	tGMI pGMI;

	int i;
	ModuleEntry e;
	DWORD cbNeeded;
	MODULEINFO mi;
	HMODULE *hMods = 0;
	char *tt = 0;

	hPsapi = LoadLibrary( "psapi.dll" );
	if ( hPsapi == 0 )
		return false;

	modules.clear();

	pEPM = (tEPM) GetProcAddress( hPsapi, "EnumProcessModules" );
	pGMFNE = (tGMFNE) GetProcAddress( hPsapi, "GetModuleFileNameExA" );
	pGMBN = (tGMFNE) GetProcAddress( hPsapi, "GetModuleBaseNameA" );
	pGMI = (tGMI) GetProcAddress( hPsapi, "GetModuleInformation" );
	if ( pEPM == 0 || pGMFNE == 0 || pGMBN == 0 || pGMI == 0 )
	{
		// yuck. Some API is missing.
		FreeLibrary( hPsapi );
		return false;
	}

	hMods = new HMODULE[TTBUFLEN / sizeof HMODULE];
	tt = new char[TTBUFLEN];
	// not that this is a sample. Which means I can get away with
	// not checking for errors, but you cannot. :)

	if ( ! pEPM( hProcess, hMods, TTBUFLEN, &cbNeeded ) )
	{
		fprintf(logfile, "EPM failed, gle = %lu\n", gle );
		goto cleanup;
	}

	if ( cbNeeded > TTBUFLEN )
	{
		fprintf(logfile, "More than %lu module handles. Huh?\n", lenof( hMods ) );
		goto cleanup;
	}

	for ( i = 0; i < cbNeeded / sizeof hMods[0]; ++ i )
	{
		// for each module, get:
		// base address, size
		pGMI( hProcess, hMods[i], &mi, sizeof mi );
		e.baseAddress = (DWORD)(size_t) mi.lpBaseOfDll;
		e.size = mi.SizeOfImage;
		// image file name
		tt[0] = '\0';
		pGMFNE( hProcess, hMods[i], tt, TTBUFLEN );
		e.imageName = tt;
		// module name
		tt[0] = '\0';
		pGMBN( hProcess, hMods[i], tt, TTBUFLEN );
		e.moduleName = tt;
		fprintf(logfile, "%08lXh %6lu %-15.15s %s\n", e.baseAddress,
			e.size, e.moduleName.c_str(), e.imageName.c_str() );

		modules.push_back( e );
	}

cleanup:
	if ( hPsapi )
		FreeLibrary( hPsapi );
	delete [] tt;
	delete [] hMods;

	return modules.size() != 0;
}
