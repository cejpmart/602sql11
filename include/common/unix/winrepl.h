#ifndef WINDOWS_REPLAC
#define WINDOWS_REPLAC

#include <stdint.h>
typedef uint32_t               uns32;
typedef int32_t                sig32;

#ifdef __WATCOMC__
#pragma off (unreferenced)
#endif

#ifdef UNIX
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#endif

#define FAR
#define EXPORT
#define EXPORTED
#define APIENTRY
#define NEAR
#ifdef BOOL // this is defined sometimes
#undef BOOL
#endif
typedef int BOOL;
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
typedef void * HANDLE; // non-file handle, convertible to any pointer
typedef int    FHANDLE;    // file handle, not convertible to any pointer
typedef void * HINSTANCE;
typedef unsigned HMETAFILE;
typedef void * HGLOBAL;
#ifdef UNIX
#define CALLBACK 
#define WINAPI   
#else
#define CALLBACK _stdcall
#define WINAPI   _stdcall
#endif
typedef void (WINAPI * FARPROC)(void); // used for procedures from externl DLLs/NLMs

#ifdef STOP  // conflicts with sqlucode.h
typedef char TCHAR;
typedef TCHAR * LPTSTR;
typedef unsigned short WCHAR;
typedef WCHAR * LPWSTR;
#endif

#include <setjmp.h>
typedef jmp_buf CATCHBUF;
#define Catch(x) setjmp(x)
#define MyThrow(x,y) longjmp(x,y)

#define hmemcpy(hpvDest, hpvSource, cbCopy) memcpy(hpvDest, hpvSource, cbCopy)
#define wsprintf 			    sprintf

#define __huge
#define _fastcall
#define __fastcall

#ifndef WORD
typedef unsigned short WORD;
#endif
#ifndef BYTE
typedef unsigned char BYTE, * LPBYTE;
#endif
#ifndef byte
typedef unsigned char byte;
#endif
typedef WORD * LPWORD;
typedef void * LPVOID;
typedef const void * LPCVOID;
#ifndef LONG
typedef long LONG;
#endif
typedef long * LPLONG;
typedef unsigned UINT;
typedef long unsigned int DWORD;  // compatibility with unixODBC/sqltypes.h
#ifndef UCHAR
typedef unsigned char UCHAR;
#endif
typedef DWORD * LPDWORD;
typedef void VOID;
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define LOWORD(l)           ((WORD)(DWORD)(l))
#define HIWORD(l)           ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))
#define LOBYTE(l)           ((BYTE)(DWORD)(l))
#define HIBYTE(l)           ((BYTE)((((DWORD)(l)) >> 8) & 0xFF))
#define SELECTOROF(lp)      HIWORD(lp)
#define OFFSETOF(lp)        LOWORD(lp)
#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))

typedef void * HWND;
typedef void * HTASK;
typedef void * HINST;
typedef void * HICON;
typedef void * HFONT;
typedef void * HBRUSH;
typedef void * HPALETTE;
typedef void * HDC;
typedef void * HCURSOR;
typedef void * HMENU;
typedef void * HBITMAP;
typedef unsigned WPARAM;
typedef unsigned long LPARAM;
typedef unsigned long LRESULT;
typedef char * LPSTR;
typedef const char * LPCSTR;
typedef const char * LPCTSTR;
typedef UINT                ATOM;
typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void SECURITY_ATTRIBUTES;
typedef unsigned int LCID;
typedef void * LPNMHDR;

#define OF_READ 	    O_RDONLY
#define OF_WRITE	    O_WRONLY
#define OF_READWRITE	    O_RDWR
#define OF_SHARE_EXCLUSIVE  0x0000
#define OF_SHARE_DENY_NONE  0x0000
#define OF_SHARE_DENY_WRITE 0x0000
#define READ 	          O_RDONLY
#define WRITE     	    O_WRONLY
#define READ_WRITE	    O_RDWR

#define HFILE_ERROR  -1
typedef int HFILE;

#define sys_open(x,y)      open(x,y)
#define sys_creat(x)       creat(x,0)
#define sys_close(x)       close(x)
#define sys_read(x,y,z)    read(x,y,z)
#define sys_write(x,y,z)   write(x,y,z)
#define sys_lseek(x,y,z)   lseek(x,y,z)
#define _lopen(x,y)      open(x,y)
#define _lcreat(x,y)     creat(x,y)
#define _lclose(x)       ::close(x)
#define _lread(x,y,z)    read(x,y,z)
#define _lwrite(x,y,z)   write(x,y,z)
#define _llseek(x,y,z)   lseek(x,y,z)
#define lstrlen(x)	strlen(x)
#define lstrcpy	strcpy

#define LF_FACESIZE	    32
typedef struct tagLOGFONT
{
    int     lfHeight;
    int     lfWidth;
    int     lfEscapement;
    int     lfOrientation;
    int     lfWeight;
    BYTE    lfItalic;
    BYTE    lfUnderline;
    BYTE    lfStrikeOut;
    BYTE    lfCharSet;
    BYTE    lfOutPrecision;
    BYTE    lfClipPrecision;
    BYTE    lfQuality;
    BYTE    lfPitchAndFamily;
    char    lfFaceName[LF_FACESIZE];
} LOGFONT;

typedef struct tagBITMAPFILEHEADER
{
    UINT    bfType;
    DWORD   bfSize;
    UINT    bfReserved1;
    UINT    bfReserved2;
    DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD   biSize;
    LONG    biWidth;
    LONG    biHeight;
    WORD    biPlanes;
    WORD    biBitCount;
    DWORD   biCompression;
    DWORD   biSizeImage;
    LONG    biXPelsPerMeter;
    LONG    biYPelsPerMeter;
    DWORD   biClrUsed;
    DWORD   biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD
{
    BYTE    rgbBlue;
    BYTE    rgbGreen;
    BYTE    rgbRed;
    BYTE    rgbReserved;
} RGBQUAD;

typedef struct tagRECT {    /* rc */
   int left;
   int top;
   int right;
   int bottom;
} RECT, *LPRECT;


typedef struct tagDRAWITEMSTRUCT {  /* ditm */
    UINT  CtlType;
    UINT  CtlID;
    UINT  itemID;
    UINT  itemAction;
    UINT  itemState;
    HWND  hwndItem;
    HDC   hDC;
    RECT  rcItem;
    DWORD itemData;
} DRAWITEMSTRUCT;

#define COLORREF LONG

typedef struct tagPOINT {   /* pt */
   int x;
   int y;
} POINT;

typedef struct tagMSG {     /* msg */
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, * LPMSG;

DWORD GetTickCount(void);  // replacement for NLM
typedef unsigned long ULONG;
typedef void * LPDEVMODE;
//////////////////////////////// Win32 file I/O /////////////////////////////////////////////
typedef void * LPOVERLAPPED;
typedef void * LPSECURITY_ATTRIBUTES;
#define INVALID_HANDLE_VALUE ((HANDLE)-1)  // compatible with HANDLE, not with FHANDLE!
#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2
#define GENERIC_READ      (0x80000000L)
#define GENERIC_WRITE     (0x40000000L)
#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define FILE_ATTRIBUTE_NORMAL    0x00000080  
#define FILE_SHARE_READ         0 // not defined
#define FILE_SHARE_WRITE        0 // not defined
#define FILE_FLAG_RANDOM_ACCESS 0 // not defined
#define FILE_FLAG_SEQUENTIAL_SCAN 0 // not defined

struct WIN32_FIND_DATA
{ char cFileName[256];
#ifdef UNIX
  char Pat[13];
  int  PatL;
#endif
};

#define WIN32_FIND_DATAA WIN32_FIND_DATA

extern "C" {
FHANDLE WINAPI CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL   WINAPI CloseHandle(FHANDLE hObject);
BOOL   WINAPI ReadFile(FHANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                          LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
BOOL   WINAPI WriteFile(FHANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
             LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
long SetFilePointer(FHANDLE hFile, DWORD pos, DWORD * hi, int start);
long SetFilePointer64(FHANDLE hFile, uint64_t pos, int start);
uint64_t GetFileSize(FHANDLE hFile, DWORD * hi);
#define CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile) CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)
#define DeleteFileA(lpFileName) DeleteFile(lpFileName)
#define CreateDirectoryA(lpPathName, lpSecurityAttributes) CreateDirectory(lpPathName, lpSecurityAttributes)
#define FlushFileBuffers(fhandle) fdatasync(fhandle)

#ifdef LINUX
#define MoveFileEx(lpExistingFileName, lpNewFileName, options_ignored) MoveFile(lpExistingFileName, lpNewFileName)
#endif
BOOL WINAPI MoveFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);
BOOL WINAPI CopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists);
typedef int64_t LARGE_INTEGER;
typedef DWORD CALLBACK PROGRESS_ROUTINE(
  LARGE_INTEGER TotalFileSize,  LARGE_INTEGER TotalBytesTransferred,
  LARGE_INTEGER StreamSize,     LARGE_INTEGER StreamBytesTransferred,
  DWORD dwStreamNumber,         DWORD dwCallbackReason,
  HANDLE hSourceFile,           HANDLE hDestinationFile,
  LPVOID lpData);   // usage not implemented
#define COPY_FILE_FAIL_IF_EXISTS              0x00000001
BOOL CopyFileEx(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName,
  PROGRESS_ROUTINE * lpProgressRoutine, LPVOID lpData, BOOL * pbCancel, DWORD dwCopyFlags);
BOOL WINAPI DeleteFile(LPCTSTR lpFileName);
BOOL WINAPI CreateDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
UINT WINAPI GetTempFileName(LPCTSTR lpPathName, LPCTSTR lpPrefixString, UINT uUnique, LPSTR lpTempFileName);
DWORD GetTempPath(DWORD BuffLength, LPSTR Buffer);

typedef WIN32_FIND_DATA * LPWIN32_FIND_DATA;
HANDLE FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
BOOL FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
BOOL FindClose(HANDLE hFindFile);
#define  FindFirstFileA(lpFileName, lpFindFileData) FindFirstFile(lpFileName, lpFindFileData)
}
/////////////////////////////////// synchronization ///////////////////////////////////////////
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#define WAIT_OBJECT_0       0x00000000L
#define WAIT_ABANDONED      0x00000080L 
#define WAIT_TIMEOUT        0x00000102L

#ifdef UNIX
#include "threadux.h"
extern "C" {
#include "eventux.h"
}


#else // NLM

struct CRITICAL_SECTION
{ int thread_id; // ... of the thread inside, 0 otherwise
  int count;     // number of additional entries on the thread inside
  LONG hSemaph;
};
typedef LONG Semaph;
typedef LONG LocEvent;

void InitializeCriticalSection(CRITICAL_SECTION * cs);
void DeleteCriticalSection    (CRITICAL_SECTION * cs);
void EnterCriticalSection     (CRITICAL_SECTION * cs);
void LeaveCriticalSection     (CRITICAL_SECTION * cs);
BOOL CheckCriticalSection     (CRITICAL_SECTION * cs);
#endif


BOOL PulseEvent(LocEvent & event);

extern "C" {
BOOL  WINAPI CreateLocalAutoEvent(BOOL bInitialState, LocEvent * event);
BOOL  WINAPI CloseLocalAutoEvent (LocEvent * event);
BOOL  WINAPI SetLocalAutoEvent   (LocEvent * event);
DWORD WINAPI WaitLocalAutoEvent  (LocEvent * event, DWORD timeout);

BOOL  WINAPI CreateLocalManualEvent(BOOL bInitialState, LocEvent * event);
BOOL  WINAPI CloseLocalManualEvent (LocEvent * event);
BOOL  WINAPI SetLocalManualEvent   (LocEvent * event);
BOOL  WINAPI ResetLocalManualEvent (LocEvent * event);
DWORD WINAPI WaitLocalManualEvent  (LocEvent * event, DWORD timeout);
DWORD WINAPI WaitPulsedManualEvent (LocEvent * event, DWORD timeout);

BOOL  WINAPI CreateSemaph(sig32 lInitialCount, sig32 lMaximumCount, Semaph * semaph);
BOOL  WINAPI CloseSemaph(Semaph * semaph);
BOOL  WINAPI ReleaseSemaph(Semaph * semaph, sig32 count);
DWORD WINAPI WaitSemaph(Semaph * semaph, DWORD timeout);

LONG  WINAPI InterlockedIncrement(LONG * count);
LONG  WINAPI InterlockedDecrement(LONG * count);

}

#endif
////////////////////////////////////// timing ////////////////////////////////////////
#ifdef UNIX
void Sleep(uns32 miliseconds);
#else
#define Sleep(x) delay(x)
#endif
///////////////////////////////////// profile ////////////////////////////////////////
extern "C" {
int WINAPI GetPrivateProfileString(LPCSTR lpszSection,  LPCSTR lpszEntry,  LPCSTR lpszDefault,
            LPSTR lpszReturnBuffer, int cbReturnBuffer, LPCSTR lpszFilename);
UINT WINAPI GetPrivateProfileInt(LPCSTR lpszSection,  LPCSTR lpszEntry,  int defaultval, LPCSTR lpszFilename);
BOOL WINAPI WritePrivateProfileString(LPCSTR lpszSection,  LPCSTR lpszEntry, LPCSTR lpszString, LPCSTR lpszFilename);
BOOL WINAPI RenameSectionInPrivateProfile(const char * lpszSection,  const char * lpszNewSection, const char * lpszFilename);
}
////////////////////////////////////// UNIX specifics ///////////////////////////////////////////
#ifdef UNIX
typedef int Widget;
#define stricmp(a,b)      strcasecmp(a,b)
#define strnicmp(a,b,c)   strncasecmp(a,b,c)
int memicmp(const void *buf1, const void *buf2, unsigned int count);
#ifndef O_BINARY
#define O_BINARY 0  // not used in UNIX
#endif
#endif

#define MAKEINTRESOURCE(i) (LPSTR)((DWORD)((WORD)(i)))
void SetCursor(int i);  // collision with wx/generic/listctrl.h when #define used
#define LoadCursor(i,j) 0
#define GlobalAlloc(i, j) (HGLOBAL) malloc(j)
#define GlobalReAlloc(i, j, k) (HGLOBAL) realloc((void *)i, j)
#define GlobalLock(j) ((void *)(j))
#define GlobalUnlock(j)  NULL
#define GlobalHandle(j) ((void *)(j)) 
#define GlobalFree(j) ::free((void *)j)
#define GetCurrentProcessId getpid

#define GetPrivateProfileStringA(lpszSection, lpszEntry, lpszDefault, lpszReturnBuffer, cbReturnBuffer, lpszFilename) GetPrivateProfileString(lpszSection, lpszEntry, lpszDefault, lpszReturnBuffer, cbReturnBuffer, lpszFilename) 
#define WritePrivateProfileStringA(lpszSection, lpszEntry, lpszString, lpszFilename) WritePrivateProfileString(lpszSection, lpszEntry, lpszString, lpszFilename)

#define __forceinline inline
#define __declspec(a) /* */
#define lstrcpy strcpy
#define lstrcmpi strcasecmp

#if wxUSE_UNICODE==1
#define _tcscpy wcscpy
#define _tcscmp wcscmp
#define _tcslen wcslen
#define _tcschr wcschr
#define _tcsnicmp wcsncasecmp
#define _tcsicmp wcscasecmp
#else
#define _tcscpy strcpy
#define _tcscmp strcmp
#define _tcslen strlen
#define _tcschr strchr
#define _tcsnicmp strnicmp
#define _tcsicmp stricmp
#endif
struct SYSTEMTIME;


