//
// Internal unzip declarations
//
#include "ZipFile.h"
#include "ZipInfo.h"

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

class CUnZipFind;
//
// Pseudo ZipEntryInfo for subfolder
//
class CSupDirEntry : public ZipEntryInfo
{
protected:

    char m_Rest[MAX_PATH];
};
//
// Indicates how much data shold be in input buffer after CUnZip602::SeekEntryHdr call
//
enum CUnZipSeek
{
    UZS_HEADER,     // ZipEntryHdr without file name
    UZS_EXTRA,      // ZipEntryHdr with file name and extra bytes
    UZS_DATA        // ZipEntryHdr with file name and extra bytes and at least one byte of data
};
//
// Decompressed zip archive
//
class CUnZip602 : public CZipFile
{
protected:

    int                  m_InBufSz;             // Input buffer size
    LPBYTE               m_InBuf;               // Input buffer pointer
    int                  m_OutBufSz;            // Output buffer size
    LPBYTE               m_OutBuf;              // Output buffer pointer
    int                  m_DirBufSz;            // Buffer for archive directory
    LPBYTE               m_DirBuf;              // Buffer for archive directory size
    int                  m_SupDirSz;            // Pseudo subfolders array size
    int                  m_SupDirCount;         // Pseudo subfolder entries count
    CSupDirEntry        *m_SupDir;              // Pseudo subfolders array
    bool                 m_AllRead;             // All input is read to input buffer
    bool                 m_MyOutBuf;            // false if output buffer is callers memory
    bool                 m_MyDirBuf;            // true if buffer for archive directory was separately allocated
    UQWORD               m_CurSeek;             // Offset in source file from which are data read in input buffer
    UQWORD               m_NextFileSeek;        // Offset in source file to header of extracted file
    UQWORD               m_DirSeek;             // Offset in source file to archive directory
    LPZIPFILEINFO        m_ZipInfo;             // Pointer to ZipFileInfo
    LPZIPENTRYINFO       m_ExpEntryInfo;        // Pointer to ZipEntryInfo of extracted file
    CZipFile             m_EntrFile;            // Output file instance
    DWORD                m_Keys[3];             // Encrypting keys
    const DWORD         *m_CRCTab;              // CRC table for decrypting data
    int                  m_ErrNO;               // Last system error code
    char                 m_ErrMsg[256];         // Last error message
    char                 m_Password[MAX_PATH];  // Password for decrypting compressed data
    CZip602Stream        m_InStream;            // Input stream
    const CZip602Stream *m_OutStream;           // Output stream

    UQWORD GetNextFileSeek();
    bool AddSupDir(LPZIPENTRYINFO pzei, int NameSz);
    bool ForceDirectories(char *FileName);
    void RemSupDir(LPZIPENTRYINFO pzei);
    int  Open();
    int  Inflate(LPZIPENTRYHDR pzh);
    int  FlushOutBuf(unsigned Size);
    int  UnZipImpl(const char *FileName);
    int  GetSupDirs();
    int  Read(void *Buf, int Size, int *RdSize = NULL);
    int  Write(void *Data, int Size);
    int  Seek(QWORD Offset, int Org = SEEK_SET, UQWORD *pSize = NULL);
    int  SeekEntryHdr(UQWORD Offset, CUnZipSeek Ext, LPZIPENTRYHDR *Hdr);
    LPZIPENTRYINFO GetExpEntryInfo();

public:

    int  Open(const char *ZipName);
    int  Open(const BYTE *Zip, int ZipSize);
    int  Open(const CZip602Stream &Stream);
    int  GetComment(wchar_t *Comment, int Size);
    int  FindFirst(const char *Folder, bool Recurse, INT_PTR *pFindHandle, INT_PTR *pEntryHandle);
    int  GetEntryInfo(LPZIPENTRYINFO Handle, ZIPENTRYINFO *Info);
    int  GetFileHdrExtraBytes(LPZIPENTRYINFO Entry, void *Buffer);
    int  UnZip(LPZIPENTRYINFO Entry, const char *FileName);
    int  UnZip(const char *FileName);
    int  UnZip(LPZIPENTRYINFO Entry, void *Buffer, int Size);
    int  UnZip(void *Buffer, int Size);
    int  UnZip(LPZIPENTRYINFO Entry, const CZip602Stream *Stream);
    int  UnZip(const CZip602Stream *Stream);
    int  UnZipAll(const char *Folder);
    int  ErrNO(){return m_ErrNO;}
    void GetInfo(ZIPINFO *Info);
    const char *ErrMsg();
    char *Password(){return m_Password;}

    CUnZip602()
    {
        m_InBufSz      = 0;
        m_InBuf        = NULL;
        m_OutBufSz     = 0;
        m_OutBuf       = NULL;
        m_DirBufSz     = 0;
        m_DirBuf       = NULL;
        m_SupDirSz     = 0;
        m_SupDir       = NULL;
        m_SupDirCount  = 0;
        m_AllRead      = false;
        m_MyOutBuf     = false;
        m_MyDirBuf     = false;
        m_NextFileSeek = 0;
        m_CRCTab       = NULL;
        m_Password[0]  = 0;    
        m_ErrNO        = 0;
        m_ErrMsg[0]    = 0;  
        m_CurSeek      = (UQWORD)-1;
        m_OutStream    = NULL;
    }
    ~CUnZip602();

    int                 InBufSz() {return m_InBufSz;}
    LPBYTE              InBuf() {return m_InBuf;}
    int                 DirSeek() {return (int)m_DirSeek;}

    friend class CUnZipFind;
};
//
// Zip archive directory search context
//
class CUnZipFind
{
protected:

    CUnZipFind     *m_Next;             // Link to next context
    CUnZip602      *m_UnZip;            // Search context owner
    bool            m_Recurse;          // Search subfolders too
    char            m_Folder[MAX_PATH]; // Searched folder name
    int             m_FolderSize;       // Searched folder name length
    LPZIPENTRYINFO  m_CurEntryInfo;     // Last entry info found
    CSupDirEntry   *m_CurSupDir;        // Last subfolder info found

    static CUnZipFind *Header;

    static void AddFindCtx(CUnZipFind *uzf);
    static void RemFindCtx(CUnZipFind *uzf);
    static void RemFindCtx(CUnZip602 *uz);

    int FindNextSupDir(INT_PTR *pEntryHandle);

public:

    static bool CheckFindCtx(CUnZipFind *uzf);
    int FindNext(INT_PTR *pEntryHandle);
    friend class CUnZip602;
};
