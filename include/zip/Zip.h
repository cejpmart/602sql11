//
// Internal zip declarations
//
#include "zlib.h"
#include "ZipFile.h"
#include "ZipInfo.h"
//
// Created zip archive, common ancestor for CZip602F, CZip602M, CZip602S
//
class CZip602
{
protected:

    DWORD                m_InfoSz;              // Zip archive directory size
    LPBYTE               m_ZipInfo;             // Buffer for archive directory 
    int                  m_InBufSz;             // Input buffer size
    LPBYTE               m_InBuf;               // Input buffer
    int                  m_OutBufSz;            // Output buffer size
    LPBYTE               m_OutBuf;              // Output buffer
    LPZIPENTRYINFO       m_CurInfo;             // Pointer to current ZipEntryInfo
    QWORD                m_ZipPos;              // Current compressed file position
    QWORD                m_ZipSz;               // Current file compressed size
    DWORD                m_CRC;                 // Current file CRC
    WORD                 m_fCnt;                // Number files in archive
    WORD                 m_Level;               // Compression level - not used
    WORD                 m_Method;              // Compression method - Z_DEFLATED or nothing
    int                  m_ErrNO;               // Last system error code
    char                 m_ErrMsg[256];         // Last error message
    char                 m_Password[MAX_PATH];  // Password for encrypting compressed data
    char                 m_fName[MAX_PATH];     // Current file name
    char                 m_RootDir[MAX_PATH];   // Root folder name
    int                  m_RootDirSz;           // Root folder name length
    CZipFile             m_EntrFile;            // Input file instance
    QWORD                m_EntrFileSz;          // Input file size
    DWORD                m_Keys[3];             // Encrypting keys
    const DWORD         *m_CRCTab;              // CRC table for encrypting data
    BYTE                 m_CryptHdr[12];        // Encrypting header
    bool                 m_AllRead;             // All input data read
    bool                 m_NoFolders;           // Store file names without folder path
    bool                 m_ToMemory;            // Archive to memory buffer
    const CZip602Stream *m_InStream;            // Input stream

    bool SetRootDir(const wchar_t *RootDir);
    int  Deflate();
    int  Copy();
    int  WriteHeader(bool SubDir);
    int  GetCRC();
    int  EntrRead(void *Data, int Size, int *pSize);
    bool RegulateFileName(const wchar_t *Src, char *Dst);
    bool FileNameForHdr(const wchar_t *fName);

    virtual int  Seek(QWORD Offset, int Org) = 0;
    virtual int  Write(const void *Data, int Size) = 0;
    virtual int  FlushOutBuf(unsigned Size) = 0;
    virtual void Remove() = 0;

public:

    CZip602()
    {
        m_InfoSz      = 0;
        m_ZipInfo     = NULL;
        m_CurInfo     = NULL;
        m_InBufSz     = 0;
        m_InBuf       = NULL;
        m_OutBufSz    = 0;
        m_OutBuf      = NULL;
        m_Level       = 9;
        m_Method      = Z_DEFLATED;
        m_Password[0] = 0;
        m_CRCTab      = NULL;
        m_fCnt        = 0;
        m_ZipPos      = 0;
        m_NoFolders   = false;
    }
    virtual ~CZip602()
    {
        if (m_ZipInfo)
            free(m_ZipInfo);
        if (m_InBuf)
            free(m_InBuf);
        if (m_OutBuf && !m_ToMemory)
            free(m_OutBuf);
    }
    int Add(const void *Data, int Size, const wchar_t *fName, bool Compress);
    int Add(const CZip602Stream *Stream, const wchar_t *fName, bool Compress);
    int Add(const wchar_t *fName, bool Compress);
    int AddMask(const wchar_t *Template, bool Compress);
    int Close();
    int ErrNO(){return m_ErrNO;}
    const char *ErrMsg();
    char *Password(){return m_Password;}
    QWORD Size();
    bool ToMemory(){return m_ToMemory;}
};
//
// Zip archive written to file
//
class CZip602F : public CZip602, public CZipFile
{
protected:

    char            m_ZipName[MAX_PATH];    // Zip file name

    virtual int Seek(QWORD Offset, int Org)
    {
        if (CZipFile::Seek(Offset, Org) >= 0)
            return Z_OK;
        m_ErrNO = Error();
        return Z_ERRNO;
    }
    virtual int Write(const void *Data, int Size)
    {
        if (CZipFile::Write(Data, Size))
            return Z_OK;
        m_ErrNO = Error();
        return Z_ERRNO;
    }
    virtual void Remove()
    {
        CZipFile::Close();
        Delete(m_ZipName);
    }
    virtual int FlushOutBuf(unsigned Size);

public:

    virtual ~CZip602F(){CZipFile::Close();}
    int Create(const wchar_t *ZipName, const wchar_t *RootDir);
};
//
// Zip archive written to memory
//
class CZip602M : public CZip602
{
protected:

    int             m_ZipBufSz;     // Destination buffer size
    LPBYTE          m_ZipBuf;       // Destination buffer

    virtual int Seek(QWORD Offset, int Org);
    virtual int Write(const void *Data, int Size);
    virtual void Remove(){}
    virtual int FlushOutBuf(unsigned Size);
    bool Realloc(unsigned Size);

public:

    CZip602M()
    {
        m_ZipBufSz    = 0;
        m_ZipBuf      = NULL;
    }
    int Create(const wchar_t *RootDir);
    int Close(void *Zip, unsigned Size);

    friend class CZip602;
};
//
// Zip archive written to stream
//
class CZip602S : public CZip602
{
protected:

    CZip602Stream m_Stream;     // Destination stream

    virtual int Seek(QWORD Offset, int Org);
    virtual int Write(const void *Data, int Size);
    virtual void Remove(){}
    virtual int FlushOutBuf(unsigned Size);

public:

    CZip602S(const CZip602Stream &Stream)
    {
        m_Stream.Param   = Stream.Param;
        m_Stream.Read    = Stream.Read;
        m_Stream.Write   = Stream.Write;
        m_Stream.Seek    = Stream.Seek;
    }
    int Create(const wchar_t *RootDir);

    friend class CZip602;
};

