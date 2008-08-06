#define UNIX
#define LINUX
#include <winrepl.h>
#undef sys_read
#undef sys_write
#undef sys_close

#include <malloc.h>
#include <stdio.h>

#define VirtualFree(a,b,c) free(a)
//#define VirtualAlloc(a,b,c,d) ((a==NULL)?malloc(b):realloc(a,b))
#define VirtualAlloc(a,b,c,d) malloc(b)
#define IDYES 122345
#define MessageBox(a,b,c,d) IDYES
extern "C" WINAPI int CloseHandle(FHANDLE h);
#define INVALID_FHANDLE_VALUE -1

#include <assert.h>

typedef char OPENFILENAME[12];

inline void itoa(int num, char *target, int base)
{
	assert(base==10);
	sprintf(target, "%d", num);
}

inline void ltoa(int num, char *target, int base)
{
	assert(base==10);
	sprintf(target, "%d", num);
}

// inline functions do not conflict in the static sec library with the same functions in the krnl library
inline bool OpenPrivateKeyFile(HWND hParent, char * pathname, const char * title, char * password)
{ return false; }

inline long SetFilePointer(FHANDLE hFile, DWORD pos, DWORD * hi, int start)
{ return lseek(hFile, pos, start); }

inline uint64_t GetFileSize(FHANDLE hFile, DWORD * hi)
{ uint64_t orig = lseek(hFile, 0, FILE_CURRENT);
  uint64_t len  = lseek(hFile, 0, FILE_END);
  lseek(hFile, orig, FILE_BEGIN);
  return len;
}

struct SYSTEMTIME
{
	WORD wDay, wMilliseconds; 
	WORD wMonth, wYear, wHour, wMinute, wSecond;
	int bias;
	struct SYSTEMTIME &operator=(struct tm& unixtm){
		wYear =1900+unixtm.tm_year;
    wMonth=1+unixtm.tm_mon;
    wDay  =unixtm.tm_mday;
    wHour  =unixtm.tm_hour;
    wMinute=unixtm.tm_min;
    wSecond=unixtm.tm_sec;
    wMilliseconds=0;
		bias=unixtm.tm_gmtoff;
    return *this;
	}
};

inline void GetLocalTime(struct SYSTEMTIME *st)
{
	struct tm unix_tm;
	time_t now=time(NULL);
	if (&unix_tm!=localtime_r(&now, &unix_tm)) return;
	*st=unix_tm;
}

