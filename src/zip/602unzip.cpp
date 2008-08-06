#ifdef   _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <stdlib.h>
#include <string.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include "UnZip.h"
#include "zlib.h"
#include "crypt.h"

#pragma warning( disable : 4996 )  // like #define _CRT_SECURE_NO_DEPRECATE
//
// Opens zip archive in file
// Input:   ZipName     - Name of zip file
//          pHandle     - Handle of open zip archive
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipOpenA(const char *ZipName, INT_PTR *pHandle)
{
    // Check parameters
    if (!IsValid(ZipName, 2) || !IsValid(pHandle, sizeof(INT_PTR)))
        return Z_INVALIDARG;
    *pHandle = 0;
    // Create unzip object
    CUnZip602 *UnZip = new CUnZip602();
    if (!UnZip)
        return Z_MEM_ERROR;
    // Open zip archive
    int Err = UnZip->Open(ZipName);
    if (Err)
    {
        delete UnZip;
        return Err;
    }
    *pHandle = (INT_PTR)UnZip;
    return Z_OK;
}
//
// Opens zip archive in file
// Input:   ZipName     - Name of zip file
//          pHandle     - Handle of open zip archive
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipOpen(const wchar_t *ZipName, INT_PTR *pHandle)
{
    // Check parameters
    if (!IsValid(ZipName, 2 * sizeof(wchar_t *)))
        return Z_INVALIDARG;
    char fName[MAX_PATH];
    // Convert file name to ASCII
    if (W2M(ZipName, fName, sizeof(fName)) <= 0)
        return Z_STRINGCONV;
    return UnZipOpenA(fName, pHandle);
}
//
// Opens zip archive in memory blok
// Input:   Zip         - Zip archive data
//          ZipSize     - Zip archive data size
//          pHandle     - Handle of open zip archive
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipOpenM(const BYTE *Zip, int ZipSize, INT_PTR *pHandle)
{
    // Check parameters
    if (!IsValid(Zip, ZipSize) || !IsValid(pHandle, sizeof(INT_PTR)))
        return Z_INVALIDARG;
    *pHandle = 0;
    // Create unzip object
    CUnZip602 *UnZip = new CUnZip602();
    if (!UnZip)
        return Z_MEM_ERROR;
    // Open zip archive
    int Err = UnZip->Open(Zip, ZipSize);
    if (Err)
        return Err;
    *pHandle = (INT_PTR)UnZip;
    return Z_OK;
}
//
// Opens zip archive in stream
// Input:   Stream      - Zip archive stream
//          pHandle     - Handle of open zip archive
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipOpenS(const CZip602Stream *Stream, INT_PTR *pHandle)
{
    // Check parameters
    if (!IsValid(Stream, sizeof(CZip602Stream)) || !IsValid((const void *)Stream->Read, 1) || !IsValid((const void *)Stream->Seek, 1) || !IsValid(pHandle, sizeof(INT_PTR)))
        return Z_INVALIDARG;
    *pHandle = 0;
    // Create unzip object
    CUnZip602 *UnZip = new CUnZip602();
    if (!UnZip)
        return Z_MEM_ERROR;
    // Open zip archive
    int Err = UnZip->Open(*Stream);
    if (Err)
        return Err;
    *pHandle = (INT_PTR)UnZip;
    return Z_OK;
}
//
// Closes zip archive
// Input:   Handle     - Handle of open zip archive
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipClose(INT_PTR Handle)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)))
        return Z_INVALIDARG;
    // Delete unzip object
    delete (CUnZip602 *)Handle;
    return Z_OK;
}
//
// Returns zip archive parameters
// Input:   Handle      - Handle of open zip archive
//          Info        - Pointer to structure for archive parameters
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetInfo(INT_PTR Handle, ZIPINFO *Info)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid(Info, sizeof(ZIPINFO)) || Info->Size < sizeof(ZIPINFO))
        return Z_INVALIDARG;
    // Read archive parameters
    ((CUnZip602 *)Handle)->GetInfo(Info);
    return Z_OK;
}
//
// Returns zip archive comment
// Input:   Handle      - Handle of open zip archive
//          Comment     - Buffer for comment
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetComment(INT_PTR Handle, wchar_t *Comment, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid(Comment, Size))
        return Z_INVALIDARG;
    // Read comment
    return ((CUnZip602 *)Handle)->GetComment(Comment, Size);
}
//
// Starts archive directory search, returns first file found in given folder
// Input:   Handle       - Handle of open zip archive
//          Folder       - Scanned folder name
//          Recurse      - If true, search subfolders too
//          pFindHandle  - Search handle for UnZipFindNext
//          pEntryHandle - Handle of found file
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipFindFirst(INT_PTR Handle, const wchar_t *Folder, bool Recurse, INT_PTR *pFindHandle, INT_PTR *pEntryHandle)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid(pFindHandle, sizeof(INT_PTR)) || !IsValid(pEntryHandle, sizeof(INT_PTR)))
        return Z_INVALIDARG;
    char folder[MAX_PATH];
    char *pf = NULL;
    // Convert folder name to ASCII
    if (Folder)
    {
        if (W2M(Folder, folder, sizeof(folder)) <= 0)
            return Z_STRINGCONV;
        pf = folder;
    }
    // Find first file
    return ((CUnZip602 *)Handle)->FindFirst(pf, Recurse, pFindHandle, pEntryHandle);
}
//
// Returns next file found in folder, specified in UnZipFindFirst
// Input:   Handle       - Handle of open zip archive
//          pEntryHandle - Handle of found file
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipFindNext(INT_PTR FindHandle, INT_PTR *pEntryHandle)
{
    // Check parameters
    if (!IsValid((void *)FindHandle, sizeof(CUnZipFind)) || !IsValid(pEntryHandle, sizeof(INT_PTR)) )
        return Z_INVALIDARG;
    if (!CUnZipFind::CheckFindCtx((CUnZipFind *)FindHandle))
        return Z_INVALIDARG;
    // Find next file
    return ((CUnZipFind *)FindHandle)->FindNext(pEntryHandle);
}
//
// Returns true if file reprezented by EntryHandle is folder
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
// Returns: true for folder, false for file, 602zip error code on error
//
inline int ZipEntryIsFolder(LPZIPENTRYINFO zei)
{ return (zei->fAttr & FILE_ATTRIBUTE_DIRECTORY) != 0 
      || zei->fNameSz && zei->fName[zei->fNameSz-1]=='/';  // FILE_ATTRIBUTE_DIRECTORY is not set in OOO zips
}

extern "C" DllZip int WINAPI UnZipIsFolder(INT_PTR Handle, INT_PTR EntryHandle)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) )
        return Z_INVALIDARG;
    // return folder flag
    return ZipEntryIsFolder((LPZIPENTRYINFO)EntryHandle);
}
//
// Returns true if file reprezented by EntryHandle is encrypted
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
// Returns: true if file is encrypted, false if not, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipIsEncrypted(INT_PTR Handle, INT_PTR EntryHandle)
{
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) )
        return Z_INVALIDARG;
    return (((LPZIPENTRYINFO)EntryHandle)->Flags & 1) != 0;
}
//
// Returns properties of file reprezented by EntryHandle
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          Info        - Pointer to structure for file properties
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetFileInfo(INT_PTR Handle, INT_PTR EntryHandle, ZIPENTRYINFO *Info)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) || !IsValid(Info, sizeof(ZIPENTRYINFO)) || Info->Size < sizeof(ZIPENTRYINFO))
        return Z_INVALIDARG;
    // Get file properties
    return ((CUnZip602 *)Handle)->GetEntryInfo((LPZIPENTRYINFO)EntryHandle, Info);
}
//
// Returns name of file reprezented by EntryHandle
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          FileName    - Buffer for file name
//          Size        - Size of buffer
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetFileName(INT_PTR Handle, INT_PTR EntryHandle, wchar_t *FileName, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) || !IsValid(FileName, Size * sizeof(wchar_t)))
        return Z_INVALIDARG;
    if (Size <= ((LPZIPENTRYINFO)EntryHandle)->fNameSz)
        return Z_INSUFFBUFF;
    int sz = ((LPZIPENTRYINFO)EntryHandle)->fNameSz;
    if (sz != 0)
    {
        // IF file name ends with / or \, truncate it
        if (sz > 1 && (((LPZIPENTRYINFO)EntryHandle)->fName[sz - 1] == '/' || ((LPZIPENTRYINFO)EntryHandle)->fName[sz - 1] == '\\'))
            sz--;
        // Convert file name to UNICODE
        sz = D2W(((LPZIPENTRYINFO)EntryHandle)->fName, sz, FileName, Size);
        if (sz <= 0)
            return Z_STRINGCONV;
    }
    FileName[sz] = 0;
    return Z_OK;
}
//
// Returns original size of file reprezented by EntryHandle
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          Size        - Original file size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetFileSize(INT_PTR Handle, INT_PTR EntryHandle, QWORD *Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) || !IsValid(Size, sizeof(UQWORD)))
        return Z_INVALIDARG;
    // Get file size
    *Size = ((LPZIPENTRYINFO)EntryHandle)->GetFileSize();
    return Z_OK;
}
//
// Returns comment of file reprezented by EntryHandle
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          Comment     - Buffer for comment
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetFileComment(INT_PTR Handle, INT_PTR EntryHandle, wchar_t *Comment, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) || !IsValid(Comment, Size * sizeof(wchar_t)))
        return Z_INVALIDARG;
    if (Size <= ((LPZIPENTRYINFO)EntryHandle)->CommentSz)
        return Z_INSUFFBUFF;
    LPZIPENTRYINFO pzei = (LPZIPENTRYINFO)EntryHandle;
    int sz = 0;
    if (pzei->CommentSz != 0)
    {
        // Convert comment to UNICODE
        sz = M2W(pzei->fName + pzei->fNameSz + pzei->ExtraBytesSz, pzei->CommentSz, Comment, Size);
        if (sz <= 0)
            return Z_STRINGCONV;
    }
    Comment[sz] = 0;
    return Z_OK;
}
//
// Returns extra bytes from ZipEntryInfo of file reprezented by EntryHandle
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          Buffer      - Buffer for extra bytes
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetFileExtraBytes(INT_PTR Handle, INT_PTR EntryHandle, void *Buffer, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) || !IsValid(Buffer, Size))
        return Z_INVALIDARG;
    if (Size < ((LPZIPENTRYINFO)EntryHandle)->ExtraBytesSz)
        return Z_INSUFFBUFF;
    LPZIPENTRYINFO pzei = (LPZIPENTRYINFO)EntryHandle;
    // Get extra bytes
    if (pzei->ExtraBytesSz)
        memcpy(Buffer, pzei->fName + pzei->fNameSz, pzei->ExtraBytesSz);
    return Z_OK;
}
//
// Returns extra bytes from ZipEntryHdr of file reprezented by EntryHandle
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          Buffer      - Buffer for extra bytes
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetFileHdrExtraBytes(INT_PTR Handle, INT_PTR EntryHandle, void *Buffer, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) || !IsValid(Buffer, Size))
        return Z_INVALIDARG;
    if (Size < ((LPZIPENTRYINFO)EntryHandle)->ExtraBytesSz)
        return Z_INSUFFBUFF;
    // Get extra bytes
    return ((CUnZip602 *)Handle)->GetFileHdrExtraBytes((LPZIPENTRYINFO)EntryHandle, Buffer);
}
//
// Extracts from zip archive file reprezented by EntryHandle
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          FileName    - Destination file name or destination folder name
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipFile(INT_PTR Handle, INT_PTR EntryHandle, const wchar_t* FileName)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)))
        return Z_INVALIDARG;
    // Convert FileName to ASCII
    char fName[MAX_PATH];
    if (W2M(FileName, fName, sizeof(fName)) <= 0)
        return Z_STRINGCONV;
    // Extract file
    return ((CUnZip602 *)Handle)->UnZip((LPZIPENTRYINFO)EntryHandle, fName);
}
//
// Extracts next file from zip archive
// Input:   Handle      - Handle of open zip archive
//          FileName    - Destination file name or destination folder name
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipNextFileA(INT_PTR Handle, const char *FileName)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)))
        return Z_INVALIDARG;
    // Extract next file
    return ((CUnZip602 *)Handle)->UnZip(FileName);
}
//
// Extracts next file from zip archive
// Input:   Handle      - Handle of open zip archive
//          FileName    - Destination file name or destination folder name
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipNextFile(INT_PTR Handle, const wchar_t *FileName)
{
    // Check parameters
    char fName[MAX_PATH];
    // Convert FileName to ASCII
    if (W2M(FileName, fName, sizeof(fName)) <= 0)
        return Z_STRINGCONV;
    // Extract next file
    return UnZipNextFileA(Handle, fName);
}
//
// Extracts file reprezented by EntryHandle to memory
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          Buffer      - Output buffer
//          Size        - Output buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipFileM(INT_PTR Handle, INT_PTR EntryHandle, void *Buffer, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) || !IsValid(Buffer, Size))
        return Z_INVALIDARG;
    // Extract file
    return ((CUnZip602 *)Handle)->UnZip((LPZIPENTRYINFO)EntryHandle, Buffer, Size);
}
//
// Extracts next file from zip archive to memory
// Input:   Handle      - Handle of open zip archive
//          Buffer      - Output buffer
//          Size        - Output buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipNextFileM(INT_PTR Handle, void *Buffer, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid(Buffer, Size))
        return Z_INVALIDARG;
    // Extract next file
    return ((CUnZip602 *)Handle)->UnZip(Buffer, Size);
}
//
// Extracts file reprezented by EntryHandle to stream
// Input:   Handle      - Handle of open zip archive
//          EntryHandle - Handle of found file
//          Stream      - Output stream
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipFileS(INT_PTR Handle, INT_PTR EntryHandle, const CZip602Stream *Stream)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)EntryHandle, sizeof(ZipEntryInfo)) || !IsValid(Stream, sizeof(CZip602Stream)) || !IsValid((const void *)Stream->Write, 1))
        return Z_INVALIDARG;
    // Extract file
    return ((CUnZip602 *)Handle)->UnZip((LPZIPENTRYINFO)EntryHandle, Stream);
}
//
// Extracts next file from zip archive to stream
// Input:   Handle      - Handle of open zip archive
//          Stream      - Output stream
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipNextFileS(INT_PTR Handle, const CZip602Stream *Stream)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid(Stream, sizeof(CZip602Stream)) || !IsValid((const void *)Stream->Write, 1))
        return Z_INVALIDARG;
    // Extract file
    return ((CUnZip602 *)Handle)->UnZip(Stream);
}
//
// Extracts all files from zip archive
// Input:   Handle      - Handle of open zip archive
//          Folder      - Destination folder name
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipAll(INT_PTR Handle, const wchar_t *Folder)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)))
        return Z_INVALIDARG;
    // Convert FileName to ASCII
    char fName[MAX_PATH];
    if (W2M(Folder, fName, sizeof(fName)) <= 0)
        return Z_STRINGCONV;
    // Extract all files
    return ((CUnZip602 *)Handle)->UnZipAll(fName);
}
//
// Sets password for decrypt compressed data
// Input:   Handle      - Handle of open zip archive
//          Password    - Password
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipSetPassword(INT_PTR Handle, const wchar_t *Password)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || (Password && !IsValid(Password, 1)))
        return Z_INVALIDARG;

    CUnZip602 *UnZip602 = (CUnZip602 *)Handle;
    // Set password
    if (!Password)
        *UnZip602->Password() = 0;
    else if (W2M(Password, UnZip602->Password(), 256) < 0)
        return Z_STRINGCONV;
    return Z_OK;
}
//
// Returns last system error code when preceeding unzip operation failed with Z_ERRNO 
// Input:   Handle      - Handle of open zip archive 
// Returns: Last system error code
//
extern "C" DllZip int WINAPI UnZipGetErrNo(INT_PTR Handle)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)))
        return Z_INVALIDARG;
    // Return error code
    return ((CUnZip602 *)Handle)->ErrNO();
}
//
// Returns error message in ASCII corresponding preceeding unzip operation failure
// Input:   Handle      - Handle of open zip archive 
//          Msg         - Buffer for message
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetErrMsgA(INT_PTR Handle, char *Msg, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)Msg, Size))
        return Z_INVALIDARG;
    // Return error message 
    strncpy((char*)((CUnZip602 *)Handle)->ErrMsg(), Msg, Size - 1);
    Msg[Size - 1] = 0;
    return Z_OK;
}
//
// Returns error message in UNICODE corresponding preceeding unzip operation failure
// Input:   Handle      - Handle of open zip archive 
//          Msg         - Buffer for message
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI UnZipGetErrMsg(INT_PTR Handle, wchar_t *Msg, int Size)
{
    // Check parameters
    if (!IsValid((void *)Handle, sizeof(CUnZip602)) || !IsValid((void *)Msg, Size * sizeof(wchar_t)))
        return Z_INVALIDARG;
    // Return error message 
    M2W(((CUnZip602 *)Handle)->ErrMsg(), Msg, Size);
    return Z_OK;
}
//
// Destructor
//
CUnZip602::~CUnZip602()
{
    free(m_InBuf);
    if (m_MyOutBuf)
        free(m_OutBuf);
    if (m_MyDirBuf)
        free(m_DirBuf);
    if (m_SupDir)
        free(m_SupDir);
    CUnZipFind::RemFindCtx(this);
}
//
// Opens zip archive in file
// Input:   ZipName     - Name of zip file
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::Open(const char *ZipName)
{
    // Open input file
    if (!CZipFile::Open(ZipName))
    {
        m_ErrNO = Error();
        return Z_ERRNO;
    }
    // Open zip archive
    return Open();
}
//
// Opens zip archive in memory blok
// Input:   Zip         - Zip archive data
//          ZipSize     - Zip archive data size
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::Open(const BYTE *Zip, int ZipSize)
{
    // Reallocate input buffer
    m_InBuf = (LPBYTE)realloc(m_InBuf, ZipSize);
    if (!m_InBuf)
         return Z_MEM_ERROR;
    // Copy input data to buffer
    memcpy(m_InBuf, Zip, ZipSize);
    m_InBufSz = ZipSize;
    m_AllRead = true;
    m_CurSeek = 0;
    LPBYTE  ph;
    // Locate ZipFileInfo
    for (ph = m_InBuf + ZipSize - sizeof(ZipFileInfo) + 1; ph >= m_InBuf; ph--)
    {
        // IF found
        if (*(DWORD *)ph == PK56)
        {
            // Locate zip archive directory
            LPZIPFILEINFO pfi = (LPZIPFILEINFO)ph;
            if (pfi->DirSeek + pfi->DirSize > (DWORD)m_InBufSz)
                continue;
            m_DirSeek = pfi->DirSeek;
            m_DirBuf  = m_InBuf + pfi->DirSeek;
            m_ZipInfo = pfi;
            LPZIPENTRYINFO pei = (LPZIPENTRYINFO)m_DirBuf;
            if (pei->ID == PK12)
                return Z_OK;
        }
    }
    return Z_DATA_ERROR;
}
//
// Opens zip archive in stream
// Input:   Stream      - Zip archive stream
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::Open(const CZip602Stream &Stream)
{
    // Set input stream
    m_InStream.Param = Stream.Param;
    m_InStream.Read  = Stream.Read;
    m_InStream.Write = Stream.Write;
    m_InStream.Seek  = Stream.Seek;
    // Open archive
    return Open();
}
//
// Opens zip archive 
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::Open()
{
    UQWORD Size;
    // Get zip file size
    int Err = Seek(0, SEEK_END, &Size);
    if (Err)
        return Err;
    // Reallocate input buffer
    m_InBufSz = ReallocBuffer(Size, m_InBuf, m_InBufSz);
    if (m_InBufSz < 0)
        return Z_MEM_ERROR;
    int hSize;
    // IF zip file size < 1 MB
    if (Size < SIZE_1MB)
    {
        // Size to read is all data size
        hSize     = (int)Size;
        // All input is read
        m_AllRead = true;
        // Seek to first file
        m_CurSeek = 0;
        Err       = Seek(0);
        if (Err)
            return Err;
    }
    else
    {
        // Size to read is size of ZipFileInfo + something
        hSize = sizeof(ZipFileInfo) + SIZE_64KB - 1;
        Err   = Seek(Size - hSize, SEEK_SET, &m_CurSeek);
        if (Err)
            return Err;
    }
    // Read data
    Err = Read(m_InBuf, hSize);
    if (Err)
        return Err;
    LPBYTE  ph;
    // Locate ZipFileInfo
    for (ph = m_InBuf + hSize - sizeof(ZipFileInfo) + 1; ph >= m_InBuf; ph--)
    {
        // IF found
        if (*(DWORD *)ph == PK56)
        {
            LPZIPFILEINFO pfi = (LPZIPFILEINFO)ph;
            // IF all read
            if (m_AllRead)
            {
                if (pfi->DirSeek + pfi->DirSize > (DWORD)m_InBufSz)
                    continue;
                // Set archive directory offset
                m_DirSeek  = pfi->DirSeek;
                // Set archive directory pointer
                m_ZipInfo  = pfi;
                m_DirBuf   = m_InBuf + pfi->DirSeek;
            }
            else
            {
                // Get archive directory offset
                UQWORD Off = pfi->GetDirSeek();
                // Check if offset is OK
                //if (Off + pfi->DirSize > (UQWORD)Size)
                if (Off + pfi->DirSize+sizeof(ZipFileInfo)-1 > (UQWORD)Size)   // oprava J.D.
                {
                    // Error by compression of file > 4GB corrction
                    if (Off + pfi->DirSize+sizeof(ZipFileInfo)-1 - 2 * sizeof(Zip4GB34) > (UQWORD)Size)  // oprava J.D.
                        continue;
                    pfi->DirSeek -= 2 * sizeof(Zip4GB34);
                    Off -= 2 * sizeof(Zip4GB34);
                }
                // Seek to archive directory
                Err = Seek(Off);
                if (Err)
                    return Err;
                m_CurSeek  = Off;
                // Reallocate buffer for archive directory
                m_DirBufSz = ReallocBuffer(Size - Off, m_DirBuf, m_DirBufSz);
                if (m_DirBufSz < 0)
                    return Z_MEM_ERROR;
                m_MyDirBuf = true;
                // Read archive directory
                Err = Read(m_DirBuf, m_DirBufSz);
                if (Err)
                    return Err;
                // Set archive directory offset
                m_DirSeek = Off;
                // Set archive directory pointer
                m_ZipInfo = (LPZIPFILEINFO)(m_DirBuf + m_DirBufSz - (m_InBuf + hSize - (BYTE *)pfi));
                // Error by compression of file > 4GB corrction
                LPZIPENTRYINFO pei = (LPZIPENTRYINFO)m_DirBuf;
                if (pei->ID == PK12 && pei->ZipPos == sizeof(Zip4GB34))
                    pei->ZipPos = 0;
            }
            LPZIPENTRYINFO pei = (LPZIPENTRYINFO)m_DirBuf;
            if (pei->ID == PK12)
                return Z_OK;
        }
    }
    return Z_DATA_ERROR;
}
//
// Returns zip archive parameters
// Input:   Info        - Pointer to structure for archive parameters
//
void CUnZip602::GetInfo(ZIPINFO *Info)
{
    m_ErrNO            = 0;
    m_ErrMsg[0]        = 0;  
    Info->FileCount    = m_ZipInfo->FileCnt;
    Info->LocFileCount = m_ZipInfo->LocFileCnt;
    Info->DiskNO       = m_ZipInfo->DiskNO;      
    Info->DirDiskNO    = m_ZipInfo->DirDiskNO;   
    Info->CommentSize  = m_ZipInfo->MsgSz; 
}
//
// Returns zip archive comment
// Input:   Comment     - Buffer for comment
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::GetComment(wchar_t *Comment, int Size)
{
    m_ErrNO            = 0;
    m_ErrMsg[0]        = 0;  
    // Check buffer size
    if (Size < m_ZipInfo->MsgSz + 1)
        return Z_INSUFFBUFF;
    if (!m_ZipInfo->MsgSz)
        *Comment = 0;
    else
    {
        int sz = 0;
        if (m_ZipInfo->MsgSz != 0)
            // Convert comment to UNICODE
            sz = M2W(m_ZipInfo->Msg, m_ZipInfo->MsgSz, Comment, Size);
        Comment[sz] = 0;
    }
    return Z_OK;
}
//
// Zip archive directory search has a small problem, folders may but need not have own directory entry.
// Directory structure can be:
//
// aaa
// aaa/f1.typ
// aaa/bbb/f2.typ
// aaa/bbb
//
// or:
//
// aaa/f1.typ
// aaa/bbb/f2.typ
//
// To afford to library caller comfortable support and return full direectory structure, by first access is whole
// archive directory traversed, all file names are parsed and decription of all folders is writen as pseudo 
// ZipEntryInfo item to m_SupDir member. FindNext scans original directory in m_DirBuf, if folder entry is found, 
// it is removed from m_SupDir, if whole original direcory is passed, rest items in m_SupDir are traversed.
//
// FindFirst starts archive directory search, returns first file found in given folder
// Input:   Folder       - Scanned folder name
//          Recurse      - If true, search subfolders too
//          pFindHandle  - Pointer to CUnZipFind as search handle for UnZipFindNext
//                         CUnZipFind instance is created by FindFirst, destroyed by FindNextSupDir when no next
//                         file found, or in CUnZip602 descructor
//          pEntryHandle - Pointer to ZipEntryInfo as handle of found file
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::FindFirst(const char *Folder, bool Recurse, INT_PTR *pFindHandle, INT_PTR *pFileHandle)
{
   *pFindHandle     = NULL;
    m_ErrNO         = 0;
    m_ErrMsg[0]     = 0;  
    // Create new find context
    CUnZipFind *uzf = new CUnZipFind();
    if (!uzf)
        return Z_MEM_ERROR;
    uzf->m_Next         = NULL;
   *uzf->m_Folder       = 0;
    uzf->m_FolderSize   = 0;
    uzf->m_Recurse      = Recurse;
    uzf->m_UnZip        = this;
    uzf->m_CurEntryInfo = (LPZIPENTRYINFO)m_DirBuf;
    // IF folder specified, replace \ with /
    if (Folder)
    {
        const char *s;
        char *d;
        for (s = Folder, d = uzf->m_Folder; *s; s++, d++)
        {
            if (*s == '\\')
                *d = '/';
            else
                *d = *s;
        }
        // Truncate last /
        if (d[-1] == '/')
            *(--d) = 0;
        uzf->m_FolderSize = (int)(d - uzf->m_Folder);
    }
    // Get list of all subfolders
    int Err = GetSupDirs();
    if (Err)
        return Err;
    uzf->m_CurSupDir    = m_SupDir;
    // Find first entry
    Err = uzf->FindNext(pFileHandle);
    if (!Err)
    {
        // Add find context to list, for release
        CUnZipFind::AddFindCtx(uzf);
        // Return find context
        *pFindHandle = (INT_PTR)uzf;
    }
    return Err;
}
//
// Creates array of ZipEntryInfo structures with description of all subfolders in zip archive
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::GetSupDirs()
{
    // EXIT IF array allready exists
    char Entry[MAX_PATH];
    if (m_SupDir)
        return Z_OK;

    // FOR each ZipEntryInfo in zip archive directory
    for (LPZIPENTRYINFO pzei = (LPZIPENTRYINFO)m_DirBuf; pzei < (LPZIPENTRYINFO)m_ZipInfo; pzei = pzei->NextEntryInfo())
    {
        // Check correct data
        if (pzei->ID != PK12)
            return Z_DATA_ERROR;
        // Get file name
        memcpy(Entry, pzei->fName, pzei->fNameSz);
        Entry[pzei->fNameSz] = 0;
        char *p;
        // Replace \ with /
        for (p = Entry; (p = strchr(p + 1, '\\')) != NULL; )
            *p = '/';
        // Parse name, add each subfolder to array
        for (p = Entry; (p = strchr(p + 1, '/')) != NULL; )
        {
            if (!AddSupDir(pzei, (int)(p - Entry)))
                return Z_MEM_ERROR;
        }
    }
    return Z_OK;
}
//
// Adds subfolder description to m_SupDir array
// Input:   pzei    - ZipEntryInfo from original zip archive directory
//          NameSz  - Subfolder name length
// Returns: false on insufficient memory
//
bool CUnZip602::AddSupDir(LPZIPENTRYINFO pzei, int NameSz)
{
    // EXIT IF subfolder description allready exists
    for (int i = 0; i < m_SupDirCount; i++)
    {
        CSupDirEntry *psdi = &m_SupDir[i];
        if (psdi->fNameSz != NameSz)
            continue;
        if (strnicmp(pzei->fName, psdi->fName, NameSz) == 0)
            return true;
    }

    // Reallocate subfolder array
    int sz = ReallocBuffer((m_SupDirCount / 8 + 1) * 8 * sizeof(CSupDirEntry), *(LPBYTE *)&m_SupDir, m_SupDirSz);
    if (sz < 0)
        return false;
    // Fill ZipEntryInfo as subfolder description
    m_SupDirSz          = sz;
    CSupDirEntry *psdi  = &m_SupDir[m_SupDirCount++];
    psdi->ID            = 0;                // ID is 0 to indicate pseudo entry
    psdi->VerMadeBy     = 0;    
    psdi->VerNeedToExtr = 0;
    psdi->Flags         = 0;        
    psdi->Method        = 0;       
    psdi->fTime         = pzei->fTime;        
    psdi->fDate         = pzei->fDate;
    psdi->CRC           = 0;          
    psdi->ZipSize       = 0;      
    psdi->fLength       = 0;      
    psdi->fNameSz       = NameSz;      
    psdi->ExtraBytesSz  = 0;    
    psdi->CommentSz     = 0;    
    psdi->DiskNO        = 0;       
    psdi->iAttr         = 0;        
    psdi->fAttr         = FILE_ATTRIBUTE_DIRECTORY | S_IFDIR;
    psdi->ZipPos        = 0;       
    memcpy(psdi->fName, pzei->fName, NameSz);
    return true;
}
//
// Removes subfolder description from m_SupDir array
// Input:   pzei    - ZipEntryInfo from original zip archive directory
//
void CUnZip602::RemSupDir(LPZIPENTRYINFO pzei)
{
    // Search subfolder from pzei in m_SupDir array
    for (int i = 0; i < m_SupDirCount; i++)
    {
        CSupDirEntry *psdi = &m_SupDir[i];
        if (psdi->fNameSz != pzei->fNameSz && psdi->fNameSz != pzei->fNameSz - 1)
            continue;
        if (strnicmp(psdi->fName, pzei->fName, psdi->fNameSz) != 0)
            continue;
        // IF found, remove item from array
        if (psdi->fNameSz == pzei->fNameSz || pzei->fName[psdi->fNameSz] == '/' || pzei->fName[psdi->fNameSz] == '\\')
        {
            int cnt = m_SupDirCount - i - 1;
            if (cnt)
                memcpy(psdi, psdi + 1, cnt * sizeof(CSupDirEntry));
            m_SupDirCount--;
            return;
        }
    }
}
//
// Returns properties of file reprezented by Entry
// Input:   EntryHandle - ZipEntryInfo of found file
//          Info        - Pointer to structure for file properties
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::GetEntryInfo(LPZIPENTRYINFO Entry, ZIPENTRYINFO *Info)
{
    // Fill ZIPENTRYINFO
    m_ErrNO             = 0;
    m_ErrMsg[0]         = 0;  
    Info->CRC           = Entry->CRC;          
    Info->ZipSize       = Entry->GetZipSize();      
    Info->FileSize      = Entry->GetFileSize();     
    Info->fAttr         = Entry->fAttr;        
    Info->iAttr         = Entry->iAttr;   
    Info->FileNameSize  = Entry->fNameSz;
    Info->VerOS         = Entry->VerMadeBy >> 8;
    Info->VerMadeBy     = Entry->VerMadeBy & 0xFF;
    Info->VerNeedToExtr = Entry->VerNeedToExtr;
    Info->Flags         = Entry->Flags;        
    Info->Method        = Entry->Method;       
    Info->dExtraSize    = Entry->ExtraBytesSz;   
    Info->CommentSize   = Entry->CommentSz;  
    Info->DiskNO        = Entry->DiskNO;       
    
    CFileDT dt;
    Info->DateTime      = 0;
    if (dt.FromDosDT(Entry->fDate, Entry->fTime))
        Info->DateTime  = dt;

    Info->hExtraSize = 0;
    // IF not pseudo entry
    if (Entry->ID)
    {
        LPZIPENTRYHDR pzh;
        // Get file position
        UQWORD Pos = Entry->GetZipPos();
        if (Pos >= m_DirSeek)
            return Z_DATA_ERROR;
        // Seek to file header
        int Err = SeekEntryHdr(Pos, UZS_HEADER, &pzh);
        if (Err)
            return Err;
        if (pzh->ID != PK34)
            return Z_DATA_ERROR;
        // Write info from file header to ZIPENTRYINFO
        Info->hExtraSize = pzh->ExtraBytes;   
    }
    return Z_OK;
}
//
// Returns extra bytes from ZipEntryHdr of file reprezented by EntryHandle
// Input:   Entry       - ZipEntryInfo of found file
//          Buffer      - Buffer for extra bytes
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::GetFileHdrExtraBytes(LPZIPENTRYINFO Entry, void *Buffer)
{
    m_ErrNO     = 0;
    m_ErrMsg[0] = 0;  
    // IF pseudo entry, nothing
    if (Entry->ID == 0)
        return Z_OK;

    LPZIPENTRYHDR pzh;
    // Get file position
    UQWORD Pos = Entry->GetZipPos();
    if (Pos >= m_DirSeek)
        return Z_DATA_ERROR;
    // Seek to file header
    int Err = SeekEntryHdr(Pos, UZS_EXTRA, &pzh);
    if (Err)
        return Err;
    if (pzh->ID != PK34)
        return Z_DATA_ERROR;
    // IF any extra bytes exist, copy them to output buffer
    if (pzh->ExtraBytes)
        memcpy(pzh->fName + pzh->fNameSz, Buffer, pzh->ExtraBytes);
    return Z_OK;
}

#define DATA_DECRYPT 0x8000
//
// In case when compressed data are encrypted, decrypting is provided directly in input buffer,
// for event if user wants extract same file for second time, original data must be read from 
// input storage to get original state. DATA_DECRYPT flag in ZipEntryHdr::Version indicates, that
// data in input buffer were decrypted and it is neccessary to read them from input storage again.
// 
// SeekEntryHdr seeks to file position, returns pointer to ZipEntryHdr in input buffer
// Input:   Offset  - Position of file header in zip archive
//          Ext     - Indicates how much data do I need to have in input buffer
//          Hdr     - Pointer to ZipEntryHdr
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::SeekEntryHdr(UQWORD Offset, CUnZipSeek Ext, LPZIPENTRYHDR *Hdr)
{
    LPZIPENTRYHDR Result = NULL;
    // IF ZipEntryHdr is in input buffer
    if (Offset >= m_CurSeek && Offset + sizeof(ZipEntryHdr) - 1 <= m_CurSeek + m_InBufSz)
    {
        // Get ZipEntryHdr pointer
        Result = (LPZIPENTRYHDR)(m_InBuf + (int)(Offset - m_CurSeek));
        // IF header without file name is requested, done
        if (Ext == UZS_HEADER)
        {
            *Hdr = Result;
            return Z_OK;
        }
        // Requested size = file name length + extra bytes length
        int sz = Result->fNameSz + Result->ExtraBytes;
        // IF compressed data are requested
        if (Ext == UZS_DATA)
        {
            // At least one BYTE of data should be in buffer
            sz++;          
            // IF file is encrypted
            if (Result->Flags & 1)
            {
                // IF file data were decrypted, I must read them again from source storage
                if (Result->Version & DATA_DECRYPT)
                    Result = NULL;
                // ELSE I need next 12 bytes of crypto header
                else
                    sz += 12;
            }
        }
        // IF not enough data available, read whole header from source storage
        if (Result && Result->fName + sz > (char *)m_InBuf + m_InBufSz)
            Result = NULL;
    }
    // IF header is not in input buffer
    if (!Result)
    {
        // Seek to file position
        int Err = Seek(Offset, SEEK_SET, &m_CurSeek);
        if (Err)
            return Err;
        int sz;
        // Read input buffer
        Err = Read(m_InBuf, m_InBufSz, &sz);
        if (Err)
            return Err;
        m_AllRead = sz < m_InBufSz;
        // Get ZipEntryHdr pointer
        Result = (LPZIPENTRYHDR)m_InBuf;
    }
    *Hdr = Result;
    return Z_OK;
}
//
// Returns ZipEntryInfo of current file
//
LPZIPENTRYINFO CUnZip602::GetExpEntryInfo()
{
    // IF current ZipEntryInfo not specified, find it by m_NextFileSeek
    if (!m_ExpEntryInfo)
    {
        LPZIPENTRYINFO Result;
        for (Result = (LPZIPENTRYINFO)m_DirBuf; Result->GetZipPos() != m_NextFileSeek; Result = Result->NextEntryInfo())
            if (Result >= (LPZIPENTRYINFO)m_ZipInfo || Result->ID != PK12)
                return NULL;
        m_ExpEntryInfo = Result;
    }
    return m_ExpEntryInfo;
}
//
// Returns offset to next file
//
UQWORD CUnZip602::GetNextFileSeek()    
{
    // Get current ZipEntryInfo
    LPZIPENTRYINFO zei = GetExpEntryInfo();
    if (!zei)
        return (UQWORD)-1;
    // Get next ZipEntryInfo
    zei                = zei->NextEntryInfo();
    if ((LPBYTE)zei > (LPBYTE)m_ZipInfo)
        return (UQWORD)-1;
    // IF ZipFileInfo reached, return offset to first directory entry
    if ((LPBYTE)zei == (LPBYTE)m_ZipInfo)
        return m_DirSeek;
    // Return offset to next entry
    return zei->GetZipPos();
}
//
// Extracts file reprezented by Entry
// Input:   Entry       - ZipEntryInfo of found file
//          FileName    - Destination file name or destination folder name
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::UnZip(LPZIPENTRYINFO Entry, const char *FileName)
{
    m_NextFileSeek = Entry->GetZipPos();    // Set extracted file position
    m_ExpEntryInfo = Entry;                 // Set extracted file ZipEntryInfo
    return UnZipImpl(FileName);             // Extract file
}
//
// Extracts next file from zip archive
// Input:   FileName    - Destination file name or destination folder name
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::UnZip(const char *FileName)
{
    m_ExpEntryInfo = NULL;                  // Reset extracted file ZipEntryInfo
    return UnZipImpl(FileName);             // Extract file
}
//
// Extracts current archive entry to file 
// Input:   FileName    - Destination file name or destination folder name
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::UnZipImpl(const char *FileName)
{
    m_ErrNO     = 0;
    m_ErrMsg[0] = 0;  
    m_OutStream = NULL;
    // Check data correctness
    if (m_NextFileSeek > m_DirSeek)
        return Z_DATA_ERROR;
    // IF archive directory reached, all files extracted
    if (m_NextFileSeek == m_DirSeek)
        return Z_NOMOREFILES;
    // Seek to file header
    LPZIPENTRYHDR pzh;
    int Err = SeekEntryHdr(m_NextFileSeek, UZS_DATA, &pzh);
    if (Err)
        return Err;
    if (pzh->ID != PK34)
        return Z_DATA_ERROR;

    // IF FileName is existing folder name, append extracted file name
    char fName[MAX_PATH];
    size_t sz = strlen(FileName);
    memcpy(fName, FileName, sz + 1);
    if (ZipIsDirectory(fName))
    {
        wchar_t wn[MAX_PATH];
        char    an[MAX_PATH];
        int ws = D2W(pzh->fName, pzh->fNameSz, wn, sizeof(wn) / sizeof(wchar_t));
        if (!ws)
            return Z_STRINGCONV;
        wn[ws] = 0;
        if (!W2M(wn, an, sizeof(an)))
            return Z_STRINGCONV;
        char *s = an;
        if (*s == '/' || *s == '\\')
            s++;
        char *d = fName + sz;
        if (fName[sz - 1] != PATH_DELIM)
            *d++ = PATH_DELIM;
        for (; s < an + pzh->fNameSz && d < fName + MAX_PATH - 1; s++, d++)
        {
#ifdef _WIN32
            if (*s == PATH_FORDELIM)
                *d = PATH_DELIM;
            else
#endif            
                *d = *s;
        }
        *d = 0;
        FileName = fName;
    }

    // Create missing subfolders
    char *pf = strrchr(fName, PATH_DELIM);
    if (pf)
    {
        *pf = 0;
        if (!ForceDirectories(fName))
        {
            m_ErrNO = Error();
            return Z_ERRNO;
        }
        *pf = PATH_DELIM;
    }

    // While is still file header in buffer, get original file size
    QWORD qSize = pzh->GetFileSize();
    // Get position of next file
    UQWORD ns   = GetNextFileSeek();
    // IF original file size == 0
    if (qSize == 0)
    {
        // Get ZipEntryInfo
        LPZIPENTRYINFO pzei = GetExpEntryInfo();
        if (!pzei)
            return Z_DATA_ERROR;
        // IF folder, create folder
        if (ZipEntryIsFolder(pzei))
        {
#ifdef _WIN32
            if (mkdir(fName) != 0 && errno != EEXIST)
#else        
            if (mkdir(fName, S_IRWXU|S_IRWXG|S_IRWXO) != 0 && errno != EEXIST)
#endif
            {
                m_ErrNO = Error();
                return Z_ERRNO;
            }
            // Set position of next file
            m_NextFileSeek = ns;
            // Done
            return Z_OK;
        }
        else // J.D.: missing length in pzh replaced from pzei
        { qSize=pzei->fLength;    
          pzh->fLength=pzei->fLength;    
          pzh->ZipSize=pzei->ZipSize;    
          pzh->CRC=pzei->CRC;    
        }  
    }

    // Create destination file
    if (!m_EntrFile.Create(fName))
    {
        m_ErrNO = Error();
        return Z_ERRNO;
    }
    // Get date and time from header
    WORD  fd = pzh->fDate;
    WORD  ft = pzh->fTime;
    // IF file is not empty
    if (qSize != 0)
    {
        // Reallocate output buffer
        if (!m_MyOutBuf)
            m_OutBufSz = 0;
        m_OutBufSz = ReallocBuffer(pzh->GetFileSize(), m_OutBuf, m_OutBufSz);
        if (m_OutBufSz < 0)
            Err = Z_MEM_ERROR;
        else
        {
            // Inflate data
            m_MyOutBuf = true;
            Err        = Inflate(pzh);
        }
    }
    // Close destination file
    m_EntrFile.Close();
    // IF error AND not CRC disagreement, delete destination file 
    if (Err && Err != Z_CRCDISAGR)
#ifdef _WIN32
        DeleteFile(fName);
#else
        remove(fName);
#endif        
    else
    {
        // Set file date and time
        CFileDT dt;
        if (dt.FromDosDT(fd, ft))
            SetFileTime(fName, dt);
        // Set position of next file
        m_NextFileSeek = ns;
    }
    return Err;
}
//
// Creates missing folders for FileName
//
bool CUnZip602::ForceDirectories(char *FileName)
{
#ifdef _WIN32
    if (mkdir(FileName) == 0)
#else
    if (mkdir(FileName, S_IRWXU|S_IRWXG|S_IRWXO) == 0)
#endif
        return true;
    if (errno == EEXIST)
        return true;
    char *pf = strrchr(FileName, PATH_DELIM);
    if (!pf)
        return true;
    *pf = 0;
    if (!ForceDirectories(FileName))
        return false;
    *pf = PATH_DELIM;
#ifdef _WIN32
    if (mkdir(FileName) != 0)
#else
    if (mkdir(FileName, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
#endif
        return false;
    return true;
}
//
// Extracts file reprezented by ZipEntryInfo to memory
// Input:   Entry       - ZipEntryInfo of found file
//          Buffer      - Output buffer
//          Size        - Output buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::UnZip(LPZIPENTRYINFO Entry, void *Buffer, int Size)
{
    m_NextFileSeek = Entry->GetZipPos();    // Set extracted file position
    m_ExpEntryInfo = Entry;                 // Set extracted file ZipEntryInfo
    return UnZip(Buffer, Size);             // Extract file
}
//
// Extracts next file from zip archive to memory
// Input:   Buffer      - Output buffer
//          Size        - Output buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::UnZip(void *Buffer, int Size)
{
    m_ErrNO     = 0;
    m_ErrMsg[0] = 0;  
    m_OutStream = NULL;
    // Check data correctness
    if (m_NextFileSeek > m_DirSeek)
        return Z_DATA_ERROR;
    // IF archive directory reached, all files extracted
    if (m_NextFileSeek == m_DirSeek)
        return Z_NOMOREFILES;
    LPZIPENTRYHDR pzh;
    // Seek to file header
    int Err = SeekEntryHdr(m_NextFileSeek, UZS_DATA, &pzh);
    if (Err)
        return Err;
    if (pzh->ID != PK34)
        return Z_DATA_ERROR;
    // Get original file size
    QWORD qSize = pzh->GetFileSize();
    // IF file is empty, done
    if (qSize == 0)
        return Z_OK;
    // Check output buffer size
    if (qSize > Size)
        return Z_INSUFFBUFF;
    // Release internal output buffer
    if (m_MyOutBuf)
    {
        free(m_OutBuf);
        m_MyOutBuf = false;
    }
    // Set callers output buffer
    m_OutBuf   = (LPBYTE)Buffer;
    m_OutBufSz = Size;
    // Inflate data
    Err = Inflate(pzh);
    // IF not error, set next file position
    if (!Err || Err == Z_CRCDISAGR)
        m_NextFileSeek = GetNextFileSeek();
    m_OutBuf   = NULL;
    return Err;
}
//
// Extracts file reprezented by ZipEntryInfo to stream
// Input:   Entry       - ZipEntryInfo of found file
//          Stream      - Output stream
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::UnZip(LPZIPENTRYINFO Entry, const CZip602Stream *Stream)
{
    m_NextFileSeek = Entry->GetZipPos();    // Set extracted file position
    m_ExpEntryInfo = Entry;                 // Set extracted file ZipEntryInfo
    return UnZip(Stream);                   // Extract file
}
//
// Extracts next file from zip archive to stream
// Input:   Stream      - Output stream
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::UnZip(const CZip602Stream *Stream)
{
    m_ErrNO     = 0;
    m_ErrMsg[0] = 0;  
    m_OutStream = Stream;
    // Check data correctness
    if (m_NextFileSeek > m_DirSeek)
        return Z_DATA_ERROR;
    // IF archive directory reached, all files extracted
    if (m_NextFileSeek == m_DirSeek)
        return Z_NOMOREFILES;
    LPZIPENTRYHDR pzh;
    // Seek to file header
    int Err = SeekEntryHdr(m_NextFileSeek, UZS_DATA, &pzh);
    if (Err)
        return Err;
    if (pzh->ID != PK34)
        return Z_DATA_ERROR;
    // Get original file size
    QWORD qSize = pzh->GetFileSize();
    // IF file is empty, done
    if (qSize == 0)
        return Z_OK;
    // Reallocate output buffer
    if (!m_MyOutBuf)
        m_OutBufSz = 0;
    m_OutBufSz = ReallocBuffer(pzh->GetFileSize(), m_OutBuf, m_OutBufSz);
    if (m_OutBufSz < 0)
        return Z_MEM_ERROR;
    m_MyOutBuf = true;
    // Inflate data
    Err = Inflate(pzh);
    // IF not error, set next file position
    if (!Err || Err == Z_CRCDISAGR)
        m_NextFileSeek = GetNextFileSeek();
    return Err;
}
//
// Extracts all files from zip archive
// Input:   Folder      - Destination folder name
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::UnZipAll(const char *Folder)
{
    int Err;
    for (m_NextFileSeek = 0; (Err = UnZip(Folder)) == Z_OK;);
    if (Err == Z_NOMOREFILES)
        Err = Z_OK;
    return Err;
}
//
// Extracts data from archive
// Input:   pzh     - Pointer to ZipEntryHdr, I must get necessary info from header now, 
//                    after read next part of input, is pzh not valid
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZip602::Inflate(LPZIPENTRYHDR pzh)
{
    // Get pointer to compressed data
    BYTE *Data  = (BYTE *)pzh + pzh->GetHdrSize();
    // Get compressed file size
    QWORD qSize = pzh->GetZipSize();

    // IF password specified
    if (*m_Password)
    {
        // Create CRC table, initialize encryption keys
        if (!m_CRCTab)
            m_CRCTab = get_crc_table();
        init_keys(m_Password, m_Keys, m_CRCTab);
        // Set flag, that data in input buffer were decrypted
        pzh->Version |= DATA_DECRYPT;
        for (int i = 0; i < 12; i++)
            zdecode(m_Keys, m_CRCTab, Data[i]);
        // Increment data pointer, decrement data size
        Data  += 12;
        qSize -= 12;
    }
    // Get rest data size in input buffer
    int   Size  = (int)(m_InBuf + m_InBufSz - Data);
    if (qSize < (QWORD)Size)
        Size   = (int)qSize;

    // IF password specified, decrypt data
    if (*m_Password)
    {
        for(int i = 0; i < Size; i++)
            Data[i] = zdecode(m_Keys, m_CRCTab, Data[i]);
    }

    int   Err = Z_OK;
    DWORD pCRC = pzh->CRC;
    DWORD CRC  = 0;
    // IF data not compressed
    if (pzh->Method == 0)
    {
        BYTE *pOut = m_OutBuf;
        for (;;)
        {
            // Calculate CRC
            CRC = crc32(CRC, Data, Size);
            // IF output to callers memory, copy data to output buffer
            if (!m_MyOutBuf)
                memcpy(pOut, Data, Size);
            // ELSE write data to output storage
            else
            {
                if (!m_EntrFile.Write(Data, Size))
                {
                    m_ErrNO = Error();
                    return Z_ERRNO;
                }
            }
            qSize -= Size;
            // BREAK IF all read
            if (qSize == 0)
                break;
            // Read next part
            Size   = m_InBufSz;
            if (qSize < (QWORD)Size)
                Size   = (int)qSize;
            m_CurSeek += m_InBufSz;
            int sz;
            Err = Read(m_InBuf, m_InBufSz, &sz);
            if (Err)
                return Err;
            if (sz < m_InBufSz)
                m_AllRead = true;
            // IF password specified, decrypt data
            if (*m_Password)
            {
                for (int i = 0; i < Size; i++)
                    m_InBuf[i] = zdecode(m_Keys, m_CRCTab, m_InBuf[i]);
            }
            Data = m_InBuf;
        }
    }
    // ELSE data are compressed
    else
    {
        // Set inflate parametres
        z_stream zs;
        zs.zalloc = NULL;
        zs.zfree  = NULL;
        zs.opaque = NULL;
        // Initialize inflate
        Err = inflateInit2(&zs, -MAX_WBITS);
        if (Err)
        {
            if (zs.msg)
            {
                strncpy(m_ErrMsg, zs.msg, sizeof(m_ErrMsg) - 1);
                m_ErrMsg[sizeof(m_ErrMsg) - 1] = 0;
            }
            return Err;
        }
        zs.next_in   = Data;
        zs.avail_in  = Size;
        zs.next_out  = m_OutBuf;
        zs.avail_out = m_OutBufSz;
        int  Flush   = Z_NO_FLUSH;
        bool First   = true;

        do
        {
            // IF input buffer is empty AND not all read
            if (zs.avail_in == 0 && !m_AllRead)
            {
                // Read next part of input data
                qSize -= Size;
                Size   = m_InBufSz;
                if (qSize < (QWORD)Size)
                    Size   = (int)qSize;
                m_CurSeek += m_InBufSz;
                int sz;
                Err = Read(m_InBuf, m_InBufSz, &sz);
                if (Err)
                    return Err;
                if (sz < m_InBufSz)
                    m_AllRead = true;
                // IF password specified, decrypt data
                if (*m_Password)
                {
                    for(int i = 0; i < Size; i++)
                        m_InBuf[i] = zdecode(m_Keys, m_CRCTab, m_InBuf[i]);
                }
                zs.next_in  = m_InBuf;
                zs.avail_in = sz;
            }
            // IF output buffer is full
            if (zs.avail_out == 0)
            {
                // Calculate CRC
                CRC = crc32(CRC, m_OutBuf, m_OutBufSz);
                // IF not output to callers memory, flush output buffer to destination storage
                if (m_MyOutBuf)
                {
                    Err = FlushOutBuf(m_OutBufSz);
                    if (Err)
                        break;
                    zs.next_out  = m_OutBuf;
                    zs.avail_out = m_OutBufSz;
                }
            }
            // IF input buffer is empty AND all read, set finish flag
            if (zs.avail_in == 0 && m_AllRead)
                Flush = Z_FINISH;
            // Inflate data
            Err = inflate(&zs, Flush);
            if (Err)
            {
                if (zs.msg)
                {
                    strncpy(m_ErrMsg, zs.msg, sizeof(m_ErrMsg) - 1);
                    m_ErrMsg[sizeof(m_ErrMsg) - 1] = 0;
                }
                // IF data error AND file is encrypted, change error code to bad password
                if (Err == Z_DATA_ERROR && First && (pzh->Flags & 1))
                    Err = Z_BADPASSWORD;
                break;
            }
            First = false;
        }
        while (Flush != Z_FINISH);
     
        // IF no error
        if (Err >= 0)
        {
            Err   = Z_OK;
            // Calculate rest of CRC
            CRC = crc32(CRC, m_OutBuf, m_OutBufSz - zs.avail_out);
            // IF not output to callers memory, flush output buffer to destination storage
            if (m_MyOutBuf)
                Err = FlushOutBuf(m_OutBufSz - zs.avail_out);
        }
        inflateEnd(&zs);
    }
    if (Err == Z_OK && CRC != pCRC)
        Err = Z_CRCDISAGR;
    return Err;
}
//
// Seek in zip archive
// Input:   Offset  - New position
//          Org     - New position origin
//          pSize   - Real position from file begin, used to get archive size
// Returns: Z_OK on succes, 602zip error code on error
// 
int CUnZip602::Seek(QWORD Offset, int Org, UQWORD *pSize)
{
    QWORD Size;
    // IF input stream Seek specified, call stream Seek
    if (m_InStream.Seek)
    {
        Size = m_InStream.Seek(m_InStream.Param, Offset, Org);
        if (Size < 0)
            return Z_602STREAMERR;
    }
    // ELSE call file seek
    else
    {
        Size = CZipFile::Seek(Offset, Org);
        if (Size == (QWORD)-1)
        {
            m_ErrNO = Error();
            return Z_ERRNO;
        }
    }
    if (pSize)
        *pSize = Size;
    return Z_OK;
}
//
// Reads data from zip archive
// Input:   Buf     - Buffer
//          Size    - Buffer size
//          pSize   - Size of data realy read
// Returns: Z_OK on succes, 602zip error code on error
// 
int CUnZip602::Read(void *Buf, int Size, int *pSize)
{
    int RdSize;
    // IF input stream Read specified, call stream Read
    if (m_InStream.Read)
    {
        RdSize = m_InStream.Read(m_InStream.Param, Buf, Size);
        if (RdSize < 0)
            return Z_602STREAMERR;
    }
    // ELSE call file read
    else
    {
        RdSize = CZipFile::Read(Buf, Size);
        if (RdSize < 0)
        {
            m_ErrNO = Error();
            return Z_ERRNO;
        }
    }
    if (pSize)
        *pSize = RdSize;
    return Z_OK;
}
//
// Writes data to output storage
// Input:   Data    - Writen data
//          Size    - Data size
// Returns: Z_OK on succes, 602zip error code on error
// 
int CUnZip602::Write(void *Data, int Size)
{
    // IF output stream specified, write data to stream
    if (m_OutStream)
    {
        if (!m_OutStream->Write(m_OutStream->Param, Data, Size))
            return Z_602STREAMERR;
    }
    // ELSE write data to file
    else if (!m_EntrFile.Write(Data, Size))
    {
        m_ErrNO = Error();
        return Z_ERRNO;
    }
    return Z_OK;
}
//
// Flushes data from output buffer to output storage
// Input:   Size    - Data size
// Returns: Z_OK on succes, 602zip error code on error
// 
int CUnZip602::FlushOutBuf(unsigned Size)
{
    if (Size)
    {
        int Err = Write(m_OutBuf, Size);
        if (Err)
            return Err;
    }
    return Z_OK;
}
//
// Returns error message corresponding preceeding unzip operation failure
//
const char *CUnZip602::ErrMsg()
{
    if (!*m_ErrMsg)
    {
        if (m_ErrNO)
#ifdef _WIN32
            FormatMessage(0, NULL, m_ErrNO, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), m_ErrMsg, sizeof(m_ErrMsg), NULL);
#else
        {
            strncpy(m_ErrMsg, strerror(m_ErrNO), sizeof(m_ErrMsg) - 1);
            m_ErrMsg[sizeof(m_ErrMsg) - 1] = 0;
        }
#endif
    }
    return m_ErrMsg;
}

CUnZipFind *CUnZipFind::Header;     // Active find contexts list
//
// Check validity of find context
//
bool CUnZipFind::CheckFindCtx(CUnZipFind *uzf)
{
    for (CUnZipFind *auzf = Header; auzf; auzf = auzf->m_Next)
        if (auzf == uzf)
            return true;
    return false;
}
//
// Adds find context to active find contexts list
//
void CUnZipFind::AddFindCtx(CUnZipFind *uzf)
{
    uzf->m_Next        = CUnZipFind::Header;
    CUnZipFind::Header = uzf;
}
//
// Removes find context from active find contexts list
//
void CUnZipFind::RemFindCtx(CUnZipFind *uzf)
{
    for (CUnZipFind **puzf = &CUnZipFind::Header; *puzf; puzf = &(*puzf)->m_Next)
    {
        if (*puzf == uzf)
        {
            *puzf = uzf->m_Next;
            break;
        }
    }
}
//
// Removes all find contexts created by given CUnZip602 instance from active find contexts list
//
void CUnZipFind::RemFindCtx(CUnZip602 *uz)
{
    for (CUnZipFind **puzf = &CUnZipFind::Header; *puzf;)
    {
        if ((*puzf)->m_UnZip != uz)
             puzf = &(*puzf)->m_Next;
        else
        {
            CUnZipFind *pdel = *puzf;
            *puzf = (*puzf)->m_Next;
            delete pdel;
        }
    }
}
//
// Finds next file in archive directory
// Input:   pEntryHandle    - Resulting ZipEntryInfo
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZipFind::FindNext(INT_PTR *pEntryHandle)
{
    char Entry[MAX_PATH];
   *pEntryHandle         = 0;
    m_UnZip->m_ErrNO     = 0;
    m_UnZip->m_ErrMsg[0] = 0;  
    // Search archive directory
    for (;; m_CurEntryInfo = m_CurEntryInfo->NextEntryInfo())
    {
        // IF zip info reached, search subfolders array
        if (m_CurEntryInfo >= (LPZIPENTRYINFO)m_UnZip->m_ZipInfo)
            return FindNextSupDir(pEntryHandle);
        // Check data correctness
        if (m_CurEntryInfo->ID != PK12)
        {
            RemFindCtx(this);
            delete this;
            return Z_DATA_ERROR;
        }
        // IF sequential search of all, found
        if (!*m_Folder && m_Recurse)
            break;
        // Replace \ with /
        memcpy(Entry, m_CurEntryInfo->fName, m_CurEntryInfo->fNameSz);
        Entry[m_CurEntryInfo->fNameSz] = 0;
        char *p;
        for (p = Entry; (p = strchr(p + 1, '\\')) != NULL; )
            *p = '/';
        // IF search in root, Name = full name
        char *Name = Entry;
        // IF search in subfolder
        if (*m_Folder)
        {
            // IF current file not belongs to m_Folder, continue
            if (strnicmp(Entry, m_Folder, m_FolderSize) != 0)
                continue;
            if (Entry[m_FolderSize] != '/')
                continue;
            // IF current file is m_Folder, continue
            if (Entry[m_FolderSize + 1] == 0)
                continue;
            // Name = file name without m_Folder part
            Name = Entry + m_FolderSize + 1;
        }
        // IF sequential search of m_Folder, found
        if (m_Recurse)
            break;
        // IF current file is not in subfolder, found
        p = strchr(Name, '/');
        if (!p || p[1] == 0)
            break;
    }
    // IF found entry is subfolder, remove it from subfolders array
    m_UnZip->RemSupDir(m_CurEntryInfo);
    // Set result
    *pEntryHandle  = (INT_PTR)m_CurEntryInfo;
    // Set next ZipEntryInfo
    m_CurEntryInfo = m_CurEntryInfo->NextEntryInfo();
    return Z_OK;
}
//
// Finds next subfolder in subfolder array
// Input:   pEntryHandle    - Resulting ZipEntryInfo
// Returns: Z_OK on succes, 602zip error code on error
//
int CUnZipFind::FindNextSupDir(INT_PTR *pEntryHandle)
{
    char Entry[MAX_PATH];
    // Search subfolder array
    for (;; m_CurSupDir++)
    {
        // IF end of array reached
        if (m_CurSupDir >= m_UnZip->m_SupDir + m_UnZip->m_SupDirCount)
        {
            // Remove find context from active find contexts list
            RemFindCtx(this);
            // Delete find context
            delete this;
            // Return Z_NOMOREFILES
            return Z_NOMOREFILES;
        }
        // Replace \ with /
        memcpy(Entry, m_CurSupDir->fName, m_CurSupDir->fNameSz);
        Entry[m_CurSupDir->fNameSz] = 0;
        char *p;
        for (p = Entry; *p; p++)
            if (*p == '\\')
                *p = '/';
        // IF search in root, Name = full name
        char *Name = Entry;
        // IF search in subfolder
        if (*m_Folder)
        {
            // IF current subfolder not belongs to folder, continue
            if (strnicmp(Entry, m_Folder, m_FolderSize) != 0)
                continue;
            if (Entry[m_FolderSize] != '/')
                continue;
            // Name = current subfolder file name without m_Folder part
            Name = Entry + m_FolderSize + 1;
        }
        // IF sequntial serch, found
        if (m_Recurse)
            break;
        // IF current subfolder is not in subsubfolder, found
        p = strchr(Name, '/');
        if (!p)
            break;
    }
    // Set result
    *pEntryHandle = (INT_PTR)m_CurSupDir++;
    return Z_OK;
}
