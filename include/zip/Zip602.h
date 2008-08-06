//
// Public declarations for 602zip
//
#include <time.h>

#define Z_OK              0
#define Z_STREAM_END      1
#define Z_NEED_DICT       2
#define Z_ERRNO         (-1)
#define Z_STREAM_ERROR  (-2)
#define Z_DATA_ERROR    (-3)
#define Z_MEM_ERROR     (-4)
#define Z_BUF_ERROR     (-5)
#define Z_VERSION_ERROR (-6)

#define Z_NOMOREFILES     100
#define Z_INVALIDARG    (-100)
#define Z_STRINGCONV    (-101)
#define Z_BADPASSWORD   (-102)
#define Z_INSUFFBUFF    (-103)
#define Z_602STREAMERR  (-104)
#define Z_CRCDISAGR     (-105)

#ifdef _WIN32
#ifdef ZIP602_STATIC
#define DllZip
#else
#define DllZip __declspec(dllimport)
#endif
typedef __int64 QWORD;
#else   // LINUX
#define DllZip
#define WINAPI
typedef int64_t QWORD;
typedef void *INT_PTR;
typedef unsigned char BYTE;
typedef uns16 WORD;
//typedef uns32 DWORD;  
typedef long unsigned int DWORD;  // compatible with winrepl.h
#endif


#pragma pack(1)
//
// Zip archive parameters
//
struct ZIPINFO
{
    DWORD   Size;               // Size of ZIPINFO structure
    WORD    FileCount;          // Count of files in archive
    WORD    LocFileCount;       // Count of files on current flopy disc
    WORD    DiskNO;             // Current flopy disc index
    WORD    DirDiskNO;          // Index of flopy disc with archive directory 
    WORD    CommentSize;        // Archive comment size
};
//
// Zip entry properties
//
struct ZIPENTRYINFO
{
    DWORD   Size;               // Size of ZIPENTRYINFO structure 
    DWORD   CRC;                // CRC
    QWORD   ZipSize;            // Compressed size
    QWORD   FileSize;           // Original size
    time_t  DateTime;           // Date and time of last file chenge
    DWORD   fAttr;              // File flags, depends on VerOS
    WORD    iAttr;              // Internal file flags 
    WORD    FileNameSize;       // File name length   
    WORD    VerOS;              // Operating/file system
    WORD    VerMadeBy;          // Zip software version used to compress this file
    WORD    VerNeedToExtr;      // Zip software version needed to extract this file
    WORD    Flags;              // Compression and encryption flags
    WORD    Method;             // Compression method
    WORD    hExtraSize;         // Extra bytes in file header length
    WORD    dExtraSize;         // Extra bytes in directory info length
    WORD    CommentSize;        // File comment length
    WORD    DiskNO;             // Index of flopy disc on which this file begins
};

#pragma pack()

typedef int   (WINAPI *LPZIP602READ)(INT_PTR Param, void *Buffer, int Size);
typedef bool  (WINAPI *LPZIP602WRITE)(INT_PTR Param, const void *Buffer, int Size);
typedef QWORD (WINAPI *LPZIP602SEEK)(INT_PTR Param, QWORD Offset, int Origin);
//
// Stream enabling store zip input and output to custom staorage
//
struct CZip602Stream
{
    INT_PTR       Param;    // Custom parameter
    LPZIP602READ  Read;     // Read function
    LPZIP602WRITE Write;    // Write function
    LPZIP602SEEK  Seek;     // Seek function
};

extern "C" DllZip int WINAPI ZipCreate(const wchar_t *ZipName, const wchar_t *RootDir, INT_PTR *pHandle);
extern "C" DllZip int WINAPI ZipCreateS(const CZip602Stream *Stream, const wchar_t *RootDir, INT_PTR *pHandle);
extern "C" DllZip int WINAPI ZipAdd(INT_PTR Handle, const wchar_t *FileName, bool Compress = true);
extern "C" DllZip int WINAPI ZipAddM(INT_PTR Handle, const void *Data, int Size, const wchar_t *FileName, bool Compress = true);
extern "C" DllZip int WINAPI ZipAddS(INT_PTR Handle, const CZip602Stream *Stream, const wchar_t *FileName, bool Compress = true);
extern "C" DllZip int WINAPI ZipAddFolder(INT_PTR Handle, const wchar_t *FolderName, bool Compress = true);
extern "C" DllZip int WINAPI ZipAddMask(INT_PTR Handle, const wchar_t *Template, bool Compress = true);
extern "C" DllZip int WINAPI ZipClose(INT_PTR Handle);
extern "C" DllZip int WINAPI ZipCloseM(INT_PTR Handle, void *Zip, unsigned int Size);
extern "C" DllZip int WINAPI ZipGetErrNo(INT_PTR Handle);
extern "C" DllZip int WINAPI ZipSetPassword(INT_PTR Handle, const wchar_t *Password);
extern "C" DllZip int WINAPI ZipGetErrNo(INT_PTR Handle);
extern "C" DllZip int WINAPI ZipGetErrMsg(INT_PTR Handle, wchar_t *Msg, int Size);
extern "C" DllZip int WINAPI ZipGetErrMsgA(INT_PTR Handle, char *Msg, int Size);
extern "C" DllZip int WINAPI ZipGetSize(INT_PTR Handle, QWORD *Size);

extern "C" DllZip int WINAPI UnZipOpen(const wchar_t *ZipName, INT_PTR *pHandle);
extern "C" DllZip int WINAPI UnZipOpenA(const char *ZipName, INT_PTR *pHandle);
extern "C" DllZip int WINAPI UnZipOpenM(const BYTE *Zip, int ZipSize, INT_PTR *pHandle);
extern "C" DllZip int WINAPI UnZipOpenS(const CZip602Stream *Stream, INT_PTR *pHandle);
extern "C" DllZip int WINAPI UnZipClose(INT_PTR Handle);
extern "C" DllZip int WINAPI UnZipFile(INT_PTR Handle, INT_PTR EntryHandle, const wchar_t* FileName);
extern "C" DllZip int WINAPI UnZipNextFile(INT_PTR Handle, const wchar_t* FileName);
extern "C" DllZip int WINAPI UnZipNextFileA(INT_PTR Handle, const char *FileName);
extern "C" DllZip int WINAPI UnZipFileM(INT_PTR Handle, INT_PTR EntryHandle, void *Buffer, int Size);
extern "C" DllZip int WINAPI UnZipNextFileM(INT_PTR Handle, void *Buffer, int Size);
extern "C" DllZip int WINAPI UnZipFileS(INT_PTR Handle, INT_PTR EntryHandle, const CZip602Stream *Stream);
extern "C" DllZip int WINAPI UnZipNextFileS(INT_PTR Handle, const CZip602Stream *Stream);
extern "C" DllZip int WINAPI UnZipAll(INT_PTR Handle, const wchar_t *Folder);
extern "C" DllZip int WINAPI UnZipFindFirst(INT_PTR Handle, const wchar_t *Folder, bool Recurse, INT_PTR *pFindHandle, INT_PTR *pEntryHandle);
extern "C" DllZip int WINAPI UnZipFindNext(INT_PTR FindHandle, INT_PTR *pEntryHandle);
extern "C" DllZip int WINAPI UnZipIsFolder(INT_PTR Handle, INT_PTR EntryHandle);
extern "C" DllZip int WINAPI UnZipIsEncrypted(INT_PTR Handle, INT_PTR EntryHandle);
extern "C" DllZip int WINAPI UnZipGetInfo(INT_PTR Handle, ZIPINFO *Info);
extern "C" DllZip int WINAPI UnZipGetComment(INT_PTR Handle, wchar_t *Comment, int Size);
extern "C" DllZip int WINAPI UnZipGetFileInfo(INT_PTR Handle, INT_PTR EntryHandle, ZIPENTRYINFO *Info);
extern "C" DllZip int WINAPI UnZipGetFileName(INT_PTR Handle, INT_PTR EntryHandle, wchar_t *FileName, int Size);
extern "C" DllZip int WINAPI UnZipGetFileSize(INT_PTR Handle, INT_PTR EntryHandle, QWORD *Size);
extern "C" DllZip int WINAPI UnZipGetFileComment(INT_PTR Handle, INT_PTR EntryHandle, wchar_t *Comment, int Size);
extern "C" DllZip int WINAPI UnZipGetFileExtraBytes(INT_PTR Handle, INT_PTR EntryHandle, void *Buffer, int Size);
extern "C" DllZip int WINAPI UnZipGetFileHdrExtraBytes(INT_PTR Handle, INT_PTR EntryHandle, void *Buffer, int Size);
extern "C" DllZip int WINAPI UnZipSetPassword(INT_PTR Handle, const wchar_t *Password);
extern "C" DllZip int WINAPI UnZipGetErrNo(INT_PTR Handle);
extern "C" DllZip int WINAPI UnZipGetErrMsg(INT_PTR Handle, wchar_t *Msg, int Size);
extern "C" DllZip int WINAPI UnZipGetErrMsgA(INT_PTR Handle, char *Msg, int Size);
