//
// Common internal zip and unzip declarations
//
#ifndef _ZIPDEFS_H_
#define _ZIPDEFS_H_

#define ZVER_FAT 0x0014
#define ZVER_4GB 0x0040 

#define PK      ('P' + ('K' << 8))
#define PK12    ('P' + ('K' << 8) + (1 << 16) + (2 << 24))
#define PK34    ('P' + ('K' << 8) + (3 << 16) + (4 << 24))
#define PK56    ('P' + ('K' << 8) + (5 << 16) + (6 << 24))
#define PK78    ('P' + ('K' << 8) + (7 << 16) + (8 << 24))

#define P4GB    ('>' + ('4' << 8) + ('G' << 16) + ('B' << 24))

#define HIDWORD(Huge) (*(((LPDWORD)&Huge) + 1))
#define LODWORD(Huge) (*(LPDWORD)&Huge)

#define Z_NOMOREFILES    100
#define Z_INVALIDARG   (-100)
#define Z_STRINGCONV   (-101)
#define Z_BADPASSWORD  (-102)
#define Z_INSUFFBUFF   (-103)
#define Z_602STREAMERR (-104)
#define Z_CRCDISAGR    (-105)

#define SIZE_64KB 0x10000
#define SIZE_1MB  0x100000

#ifdef _WIN32

#define SIZE_4GB  0x100000000i64

#ifdef ZIP_DLL
#define DllZip __declspec(dllexport)
#else
#define DllZip
#endif

typedef __int64 QWORD, *PQWORD;
typedef unsigned __int64 UQWORD;

inline bool IsValid(void *Ptr, int Size){return !IsBadWritePtr(Ptr, Size);}
inline bool IsValid(const void *Ptr, int Size){return !IsBadReadPtr(Ptr, Size);}

#else   // LINUX

#define DllZip
#define WINAPI

#define SIZE_4GB  0x100000000ll

#include <stdint.h>

typedef int64_t QWORD, *PQWORD;
typedef void *INT_PTR;
#ifndef WINDOWS_REPLAC
typedef /*unsigned*/ uint64_t UQWORD;  // J.D. updated unsigned int64_t -> uint64_t
typedef unsigned char BYTE, *LPBYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD, *LPDWORD ;  

#define strnicmp strncasecmp
#endif // WINDOWS_REPLAC
 

#ifndef MAX_PATH
#define MAX_PATH 260

#define FILE_ATTRIBUTE_READONLY  0x00000001  
#define FILE_ATTRIBUTE_HIDDEN    0x00000002  
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010  

#endif // MAX_PATH

#define IsValid(Ptr, Size) (Ptr != NULL)
#endif // _WIN32

typedef int   (WINAPI *LPZIP602READ)(INT_PTR Param, void *Buffer, int Size);
typedef bool  (WINAPI *LPZIP602WRITE)(INT_PTR Param, const void *Buffer, int Size);
typedef QWORD (WINAPI *LPZIP602SEEK)(INT_PTR Param, QWORD Offset, int Origin);

struct CZip602Stream
{
    INT_PTR       Param;
    LPZIP602READ  Read;  
    LPZIP602WRITE Write;
    LPZIP602SEEK  Seek;
    
    CZip602Stream()
    {
        Param = NULL;
        Read  = NULL; 
        Write = NULL;
        Seek  = NULL;
    }
};  
    
int ReallocBuffer(QWORD NewSize, LPBYTE &Buf, int OldSize);
int W2M(const wchar_t *Src, char *Dst, int DstSz);
int M2W(const char *Src, wchar_t *Dst, int DstSz);
int M2W(const char *Src, int SrcSz, wchar_t *Dst, int DstSz);
int W2D(const wchar_t *Src, char *Dst, int DstSz);
int D2W(const char *Src, int SrcSz, wchar_t *Dst, int DstSz);

#endif // _ZIPDEFS_H_
