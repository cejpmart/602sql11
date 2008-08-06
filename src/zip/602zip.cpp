// 
// 602Zip - Zip compression / decompression library
// ================================================
//
// 1.0.0.1 14.11.05 Opravena chyba pri vstupni i vystupni konverzi data a casu komprimovaneho souboru
//                  Release
// 1.0.0.2  7.12.05 Opraven problem s priznakem S_IFDIR v ZipEntryInfo.fAttr
//                  Opravena chyba v UnZipAll - if (Err = Z_NOMOREFILES)
//                  Opravena chyba v UnZipImpl - vetev pro adresar neposouvala m_NextFileSeek
//                  Release
// 1.0.0.3 12. 1.06 Opravena chyba pri konverzi FILETIME na time_t 
//                  Opravena chyba pri dekomprimaci archivu vetsiho nez 1 MB - neposouval se m_CurSeek
//         24. 1.06 Opravena chyba IsDirectory nefungovala pokud bylo na konci jmena souboru lomitko
// 1.0.0.4 10. 2.06 Doplneny funkce pro kompresi a dekompresi do streamu
//                  Release
// 1.0.0.5 25. 5.06 Odladeno na linuxu
//                  Doplnena kontrola CRC pri dekomprimaci
//         13. 6.06 Opravena chyba pri rozbalovani nekomprimovaneho souboru, ktery je pres hranici bufferu
//                  Release   
// 1.0.0.6 20. 7.06 Opravene chyba pri rozbalovani prazdneho adresare
//                  Release
// 1.0.0.7  6. 9.06 Kvuli chybe, kdy se ve starsich verzich pri pakovani spatne pocital CRC,
//                  UnZip pri zjisteni nesouhlasu CRC vrati Z_CRCDISAGR, ale nesmaze vystupni soubor
//         18. 9.06 Doplnena funkce UnZipGetFileSize
//                  Release
// 1.0.0.8 25. 9.06 UnzipOpenM nepouziva, buffer, ktery dostane jako argument, ale kopii.
//         26.10.06 CZipFile::Open pouziva pri volani CreateFile FILE_SHARE_READ
//         12.01.07 Doplneno testovani konzistence adresare pri UnZipFindFirst
//                  Release
//
#ifdef   _WIN32
#include <windows.h>
#else
#include <wchar.h>
#include <string.h>
#endif
#include <stdlib.h>
#include <time.h>
#include "crypt.h"
#include "Zip.h"
#include "ZipDefs.h"

#define INFOSIZE_INC 4096
#pragma warning( disable : 4996 )  // like #define _CRT_SECURE_NO_DEPRECATE
//
// Creates new zip archive whitch should be stored to file or to block of memory
// Input:   ZipName     - Name of new zip file, if NULL or empty string, archive will be stored to memory
//          RootDir     - Root folder of archived files, file names of stored files will be in zip noticed relative to this folder
//          pHandle     - Handle of created zip archive
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipCreate(const wchar_t *ZipName, const wchar_t *RootDir, INT_PTR *pHandle)
{
    // Check parameters
    if ((ZipName &&  !IsValid(ZipName, sizeof(wchar_t))) || !IsValid(pHandle, sizeof(INT_PTR)))
        return Z_INVALIDARG;
    *pHandle = 0;
    // IF zip file name specified
    if (ZipName && *ZipName)
    {
        // Create zip object for output to file
        CZip602F *Zip = new CZip602F();
        if (!Zip)
            return Z_MEM_ERROR;
        // Initialize zip object
        int Err = Zip->Create(ZipName, RootDir);
        if (Err)
        {
            delete Zip;
            return Err;
        }
        *pHandle = (INT_PTR)Zip;
    }
    else
    {
        // Create zip object for output to memory
        CZip602M *Zip = new CZip602M();
        if (!Zip)
            return Z_MEM_ERROR;
        // Initialize zip object
        int Err = Zip->Create(RootDir);
        if (Err)
        {
            delete Zip;
            return Err;
        }
        *pHandle = (INT_PTR)Zip;
    }
    return Z_OK;
}
//
// Creates new zip archive whitch shold be writen to stream
// Input:   Stream      - Stream to write zip archive
//          RootDir     - Root folder of archived files, file names of stored files will be in zip noticed relative to this folder
//          pHandle     - Handle of created zip archive
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipCreateS(const CZip602Stream *Stream, const wchar_t *RootDir, INT_PTR *pHandle)
{
    // Check parametres
    if (!IsValid(Stream, sizeof(CZip602Stream)) || !IsValid((const void *)Stream->Write, 1) || !IsValid((const void *)Stream->Seek, 1) || !IsValid(pHandle, sizeof(INT_PTR)))
        return Z_INVALIDARG;
    *pHandle = 0;
    // Create zip object for output to stream
    CZip602S *Zip = new CZip602S(*Stream);
    if (!Zip)
        return Z_MEM_ERROR;
    // Initialize zip object
    int Err = Zip->Create(RootDir);
    if (Err)
    {
        delete Zip;
        return Err;
    }
    *pHandle = (INT_PTR)Zip;
    return Z_OK;
}
//
// Adds file to zip archive
// Input:   Handle      - Handle of zip archive 
//          FileName    - Input file name
//          Compress    - If false, file will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipAdd(INT_PTR Handle, const wchar_t *FileName, bool Compress)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)))
        return Z_INVALIDARG;
    // Add file to archive
    int Err = ((CZip602 *)Handle)->Add(FileName, Compress);
    return Err;
}
//
// Adds block of data to zip archive
// Input:   Handle      - Handle of zip archive 
//          Data        - Pointer to data
//          Size        - Data size in bytes
//          FileName    - File name under whitch will be data stored
//          Compress    - If false, data will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipAddM(INT_PTR Handle, const void *Data, int Size, const wchar_t *FileName, bool Compress)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)))
        return Z_INVALIDARG;
    // Add data
    int Err = ((CZip602 *)Handle)->Add(Data, Size, FileName, Compress);
    return Err;
}
//
// Adds data from stream to zip archive
// Input:   Handle      - Handle of zip archive 
//          Stream      - Input stream
//          FileName    - File name under whitch will be data stored
//          Compress    - If false, data will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipAddS(INT_PTR Handle, const CZip602Stream *Stream, const wchar_t *FileName, bool Compress)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)) || !IsValid((const void *)Stream->Read, 1) || !IsValid((const void *)Stream->Seek, 1))
        return Z_INVALIDARG;
    // Add data
    int Err = ((CZip602 *)Handle)->Add(Stream, FileName, Compress);
    return Err;
}
//
// Adds all files and subfolders from given folder to zip archive
// Input:   Handle      - Handle of zip archive 
//          FolderName  - Source folder name
//          Compress    - If false, files will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipAddFolder(INT_PTR Handle, const wchar_t *FolderName, bool Compress)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)))
        return Z_INVALIDARG;
    // Create *.* mask
    wchar_t fn[MAX_PATH];
    wcscpy(fn, FolderName);
    size_t sz = wcslen(fn);
    if (fn[sz - 1] != PATH_DELIM)
        fn[sz++] = PATH_DELIM;
    wcscpy(fn + sz, L"*.*");
    // Add files to archive
    int Err = ((CZip602 *)Handle)->AddMask(fn, Compress);
    return Err;
}
//
// Adds all files corresponding given template to zip archive
// Input:   Handle      - Handle of zip archive 
//          Template    - Template for file names, can contain wildcard characters 
//          Compress    - If false, files will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipAddMask(INT_PTR Handle, const wchar_t *Template, bool Compress)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)))
        return Z_INVALIDARG;
    // Add files to archive
    int Err = ((CZip602 *)Handle)->AddMask(Template, Compress);
    return Err;
}
//
// Closes zip archive stored to file or to stream
// Input:   Handle      - Handle of zip archive 
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipClose(INT_PTR Handle)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)) || ((CZip602 *)Handle)->ToMemory())
        return Z_INVALIDARG;
    // Close zip archive
    int Err = ((CZip602 *)Handle)->Close();
    // Delete zip archive object
    delete (CZip602 *)Handle;
    return Err;
}
//
// Closes zip archive stored memory
// Input:   Handle      - Handle of zip archive 
//          Zip         - Pointer to otput buffer
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipCloseM(INT_PTR Handle, void *Zip, unsigned int Size)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)) || !IsValid(Zip, Size) || !((CZip602 *)Handle)->ToMemory())
        return Z_INVALIDARG;
    // Check buffer size
    if (((CZip602 *)Handle)->Size() > Size)
        return Z_INSUFFBUFF;
    // Write archive to buffer
    int Err = ((CZip602M *)Handle)->Close(Zip, Size);
    // Delete zip archive object
    delete (CZip602M *)Handle;
    return Err;
}
//
// Returns current zip archive size, usefull for storing archive to memory 
// Input:   Handle      - Handle of zip archive 
//          Size        - Resulting size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipGetSize(INT_PTR Handle, QWORD *Size)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)) || !IsValid(Size, sizeof(QWORD)))
        return Z_INVALIDARG;
    // Set archive size
    *Size = ((CZip602 *)Handle)->Size();
    return Z_OK;
}
//
// Sets password for encrypting archived files content
// Input:   Handle      - Handle of zip archive 
//          Password    - Password
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipSetPassword(INT_PTR Handle, const wchar_t *Password)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)) || (Password && !IsValid(Password, 1)))
        return Z_INVALIDARG;
    // Set password
    CZip602 *Zip602 = (CZip602 *)Handle;
    if (!Password)
        *Zip602->Password() = 0;
    else if (W2M(Password, Zip602->Password(), 256) < 0)
        return Z_STRINGCONV;
    return Z_OK;
}
//
// Returns last system error code when preceeding zip operation failed with Z_ERRNO 
// Input:   Handle      - Handle of zip archive 
// Returns: Last system error code
//
extern "C" DllZip int WINAPI ZipGetErrNo(INT_PTR Handle)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)))
        return Z_INVALIDARG;
    // Return error code
    return ((CZip602 *)Handle)->ErrNO();
}
//
// Returns error message in ASCII corresponding preceeding zip operation failure
// Input:   Handle      - Handle of zip archive 
//          Msg         - Buffer for message
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipGetErrMsgA(INT_PTR Handle, char *Msg, int Size)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)) || !IsValid((void *)Msg, Size * sizeof(wchar_t)))
        return Z_INVALIDARG;
    // Return error message
    strncpy((char*)((CZip602 *)Handle)->ErrMsg(), Msg, Size - 1);
    Msg[Size - 1] = 0;
    return Z_OK;
}
//
// Returns error message in UNICODE corresponding preceeding zip operation failure
// Input:   Handle      - Handle of zip archive 
//          Msg         - Buffer for message
//          Size        - Buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
extern "C" DllZip int WINAPI ZipGetErrMsg(INT_PTR Handle, wchar_t *Msg, int Size)
{
    // Check parametres
    if (!IsValid((void *)Handle, sizeof(CZip602)) || !IsValid((void *)Msg, Size * sizeof(wchar_t)))
        return Z_INVALIDARG;
    // Return error message
    M2W(((CZip602 *)Handle)->ErrMsg(), Msg, Size);
    return Z_OK;
}
//
// Initializes zip archive for output to file
// Input:   ZipName     - Name of new zip file
//          RootDir     - Root folder of archived files, file names of stored files will be in zip noticed relative to this folder
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602F::Create(const wchar_t *ZipName, const wchar_t *RootDir)
{
	m_ToMemory = false;
    // Convert file name to ASCII
    if (W2M(ZipName, m_ZipName, sizeof(m_ZipName)) <= 0)
        return Z_STRINGCONV;
    // Set root folder
	if (!SetRootDir(RootDir))
		return Z_STRINGCONV;
    // IF file type not specified, append .zip
    char *pn = strrchr(m_ZipName, PATH_DELIM);
    if (!pn)
        pn = m_ZipName;
    if (!strchr(pn, '.'))
        strcat(pn, ".zip");
    // Common archive initialization
    if (!CZipFile::Create(m_ZipName))
    {
        m_ErrNO = Error();
        return Z_ERRNO;
    }
    return Z_OK;
}
//
// Initializes zip archive for output to memory
// Input:   RootDir     - Root folder of archived files, file names of stored files will be in zip noticed relative to this folder
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602M::Create(const wchar_t *RootDir)
{
	m_ToMemory = true;
    // Set root folder
	if (!SetRootDir(RootDir))
		return Z_STRINGCONV;
    return Z_OK;
}
//
// Initializes zip archive for output to stream
// Input:   RootDir     - Root folder of archived files, file names of stored files will be in zip noticed relative to this folder
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602S::Create(const wchar_t *RootDir)
{
    m_ToMemory = false;
    // Set root folder
    if (!SetRootDir(RootDir))
        return Z_STRINGCONV;
    return Z_OK;
}
//
// Sets root folder, returns false if folder name cannot be converted to ASCII
//
bool CZip602::SetRootDir(const wchar_t *RootDir)
{
   *m_RootDir   = 0;
    m_RootDirSz = 0;
    if (RootDir && *RootDir)
    {
        // Convert name to ASCII, remove leading disc name, \\ or /, replace \ with /
        if (!RegulateFileName(RootDir, m_RootDir))
            return false;
        // IF root name is not terminated with /, append /
        m_RootDirSz = (int)strlen(m_RootDir);
        char *pb = m_RootDir + m_RootDirSz;
        if (*(pb - 1) != '/')
        {
            *(WORD *)pb = '/';
            m_RootDirSz++;
        }
    }
	return true;
}
//
// Adds file to zip archive
// Input:   fName       - Input file name
//          Compress    - If false, file will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602::Add(const wchar_t *fName, bool Compress)
{
    // Add file to archive
    int Err = Add(NULL, fName, Compress);
    // Close input file
    if (m_EntrFile.Valid())
        m_EntrFile.Close();
    return Err;
}
//
// Adds data to zip archive
// Input:   Stream      - Input stream, NULL if input from file
//          fName       - File name under whitch will be data stored
//          Compress    - If false, data will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602::Add(const CZip602Stream *Stream, const wchar_t *fName, bool Compress)
{
    m_ErrNO     = 0;
    m_ErrMsg[0] = 0;  
    m_AllRead   = false;
    m_InStream  = Stream;
    // Convert file name to ASCII
    if (W2M(fName, m_fName, sizeof(m_fName)) <= 0)
        return Z_STRINGCONV;

    // Get max size of ZipEntryInfo
    DWORD  iSz    = sizeof(ZipEntryInfo) + MAX_PATH;
    // Get current ZipEntryInfo offset
    DWORD  iOff   = (DWORD)((LPBYTE)m_CurInfo - m_ZipInfo);
    // Get new archive directory size
    DWORD  iNewSz = iOff + iSz + sizeof(ZipFileInfo) + sizeof(Zip4GB12);
    // Reallocate archive directory
    if (iNewSz > m_InfoSz)
    {
        m_InfoSz += INFOSIZE_INC;
        LPBYTE ZipInfo = (LPBYTE)realloc(m_ZipInfo, m_InfoSz);
        if (!ZipInfo)
            return Z_MEM_ERROR;
        m_ZipInfo = ZipInfo;
        m_CurInfo = (LPZIPENTRYINFO)(ZipInfo + iOff);
    }
    // Store file name to current ZipEntryInfo
    if (!FileNameForHdr(fName))
        return Z_STRINGCONV;
    
    bool  SubDir  = ZipIsDirectory(m_fName);
    int CrHdrSz   = 0;
    m_ZipSz       = 0;
    m_EntrFileSz  = 0;
    m_Method      = 0;
    int Err       = Z_ERRNO;
    // IF Input file is folder, write ZipEntryHdr and ZipEntryInfo
    if (SubDir)
    {
        m_CRC = 0;
        Err   = WriteHeader(true);
    }
    // ELSE ordinary file
    else
    {
        // IF input from stream
        if (Stream)
        {
            // Get input file size
            m_EntrFileSz = Stream->Seek(Stream->Param, 0, SEEK_END);
            if (m_EntrFileSz < 0 || Stream->Seek(Stream->Param, 0, SEEK_SET) < 0)
                return Z_602STREAMERR; 
        }
        // ELSE input form file
        else
        {
            // Open input file
            if (!m_EntrFile.Open(m_fName))
            {
                m_ErrNO = m_EntrFile.Error();
                return Z_ERRNO;
            }
            // Get input file size
            m_EntrFileSz = m_EntrFile.GetHugeSize();
            if (m_EntrFileSz < 0 || m_EntrFile.Seek(0) < 0)
            {
                m_ErrNO = m_EntrFile.Error();
                return Z_ERRNO;
            }
        }
        // IF Compression requested, set compression method
        if (Compress)
            m_Method = Z_DEFLATED;
        // Calculate CRC
        Err = GetCRC();
        if (Err)
            return Err;
        // Get ZipEntryHdr size
        QWORD HdrSz = sizeof(ZipEntryHdr) + m_CurInfo->fNameSz - 1;
        if (m_EntrFileSz > SIZE_4GB)
            HdrSz += sizeof(Zip4GB34);
        // IF password specified
        if (*m_Password)
        {
            // Get CRC  table
            if (!m_CRCTab)
                m_CRCTab = get_crc_table();
            // Create crypto header
            CrHdrSz  = crypthead(m_Password, m_CryptHdr, sizeof(m_CryptHdr), m_Keys, m_CRCTab, m_CRC);
            HdrSz   += CrHdrSz;
            m_ZipSz  = CrHdrSz;
        }
        // IF input file not empty
        if (m_EntrFileSz)
        {
            // Leave place for ZipEntryHdr and crypto header
            Err = Seek(HdrSz, SEEK_CUR);
            if (Err)
                return Err;
            // IF Compress requested, deflate data
            if (Compress)
            {
                Err = Deflate();
                if (Err)
                    return Err;
            }
            // ELSE copy data to archive
            else
            {
                Err = Copy();
                if (Err)
                    return Err;
                m_ZipSz += m_EntrFileSz;
            }
            // Seak back to ZipEntryHdr position
            Err = Seek(m_ZipPos, SEEK_SET);
            if (Err)
                return Err;
        }
        // Write ZipEntryHdr and ZipEntryInfo
        Err = WriteHeader(false);
        if (Err)
            return Err;
        // IF Password specified, write crypto heder
        if (*m_Password)
        {
            Err = Write(m_CryptHdr, CrHdrSz);
            if (Err)
                return Err;
        }
        // Seek to next file position
        Seek(m_ZipSz - CrHdrSz, SEEK_CUR);
        Err = Z_OK;
    }
    return Err;
}
//
// Adds all files corresponding given template to zip archive
// Input:   Template    - Template for file names, can contain wildcard characters 
//          Compress    - If false, files will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602::AddMask(const wchar_t *Template, bool Compress)
{
    const wchar_t *ls = wcsrchr(Template, PATH_DELIM);
    if (!ls)
        ls = Template;
    bool SubDirs = wcscmp(ls + 1, L"*.*") == 0;
    CFindFile ff;
    // Find first file corresponding template
    if (!ff.FindFirst(Template, SubDirs))
    {
        // IF no such file exists, return Z_NOMOREFILES
        if (ff.IsEmpty())
            return Z_NOMOREFILES;
        // IF error, return Z_ERRNO
        m_ErrNO = CZipFile::Error();
        return Z_ERRNO;
    }
    // FOR each corresponding file
    do
    {
        // IF subolder
        if (ff.IsFolder())
        {
            // All all files from fubfoder to archive
            wchar_t Folder[MAX_PATH];
            wcscpy(Folder, ff.GetFileName());
            size_t sz = wcslen(Folder);
            Folder[sz] = PATH_DELIM;
            wcscpy(Folder + sz + 1, L"*.*");
            int Err = AddMask(Folder, Compress);
            // IF subfolder is empty, add subfolder to archive
            if (Err == Z_NOMOREFILES)
                Err = Add(NULL, ff.GetFileName(), Compress);
            if (Err)
                return Err;
        }
        // ELSE ordinary file
        else
        {
            // Add file to archive
            int Err = Add(ff.GetFileName(), Compress);
            if (Err)
                return Err;
        }
    }
    while (ff.FindNext());
    return Z_OK;
}
//
// Adds block of data to zip archive
// Input:   Data        - Pointer to data
//          Size        - Data size in bytes
//          FileName    - File name under whitch will be data stored
//          Compress    - If false, data will be stored to archive without compression
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602::Add(const void *Data, int Size, const wchar_t *fName, bool Compress)
{
    m_ErrNO     = 0;
    m_ErrMsg[0] = 0;  
    m_AllRead   = false;
    m_InStream  = NULL;
    // Convert file name to ASCII
    if (W2M(fName, m_fName, sizeof(m_fName)) <= 0)
        return Z_STRINGCONV;

    // Get max size of ZipEntryInfo
    DWORD  iSz    = sizeof(ZipEntryInfo) + MAX_PATH;
    // Get current ZipEntryInfo offset
    DWORD  iOff   = (DWORD)((LPBYTE)m_CurInfo - m_ZipInfo);
    // Get new archive directory size
    DWORD  iNewSz = iOff + iSz + sizeof(ZipFileInfo) + sizeof(Zip4GB12);
    // Reallocate archive directory
    if (iNewSz > m_InfoSz)
    {
        m_InfoSz += INFOSIZE_INC;
        LPBYTE ZipInfo = (LPBYTE)realloc(m_ZipInfo, m_InfoSz);
        if (!ZipInfo)
            return Z_MEM_ERROR;
        m_ZipInfo = ZipInfo;
        m_CurInfo = (LPZIPENTRYINFO)(ZipInfo + iOff);
    }
    // Store file name to current ZipEntryInfo
    if (!FileNameForHdr(fName))
        return Z_STRINGCONV;

    int CrHdrSz   = 0;
    int Err       = Z_ERRNO;
    m_ZipSz       = 0;
    m_EntrFileSz  = (QWORD)Size;
    m_Method      = 0;
    // IF Compression requested, set compression method
    if (Compress)
        m_Method = Z_DEFLATED;
    // Calculate CRC
    m_CRC = crc32(0, (const Bytef *)Data, Size);

    QWORD HdrSz = sizeof(ZipEntryHdr) + m_CurInfo->fNameSz - 1;
    // IF password specified
    if (*m_Password)
    {
        // Get CRC  table
        if (!m_CRCTab)
            m_CRCTab = get_crc_table();
        // Create crypto header
        CrHdrSz  = crypthead(m_Password, m_CryptHdr, sizeof(m_CryptHdr), m_Keys, m_CRCTab, m_CRC);
        HdrSz   += CrHdrSz;
        m_ZipSz  = CrHdrSz;
    }
    // IF block is empty
    if (Size)
    {
        // Leave place for ZipEntryHdr and crypto header
        Err = Seek(HdrSz, SEEK_CUR);
        if (Err)
            return Err;
        // Set input buffer parameters
        if (m_InBuf)
            free(m_InBuf);
        m_InBuf   = (LPBYTE)Data;
        m_InBufSz = Size;
        m_AllRead = true;
        // IF compression requested, deflate data
        if (Compress)
        {
            Err = Deflate();
        }
        // ELSE copy data to archive
        else
        {
            Err = Copy();
            m_ZipSz += m_EntrFileSz;
        }
        m_InBuf   = NULL;
        m_InBufSz = 0;
        if (Err)
            return Err;
        // Seak back to ZipEntryHdr position
        Err = Seek(m_ZipPos, SEEK_SET);
        if (Err)
            return Err;
    }
    // Write ZipEntryHdr and ZipEntryInfo
    Err = WriteHeader(false);
    if (Err)
        return Err;
    // IF Password specified, write crypto heder
    if (*m_Password)
    {
        Err = Write(m_CryptHdr, CrHdrSz);
        if (Err)
            return Err;
    }
    // Seek to next file position
    Seek(m_ZipSz - CrHdrSz, SEEK_CUR);
    return Z_OK;
}
//
// Compresses data
//
int CZip602::Deflate()
{
    // IF not zip to memory, reallocate output buffer
    if (!m_ToMemory)
    {
        m_OutBufSz = ReallocBuffer(m_EntrFileSz, m_OutBuf, m_OutBufSz);
        if (m_OutBufSz < 0)
            return Z_MEM_ERROR;
    }
    z_stream zs;
    zs.zalloc = NULL;
    zs.zfree  = NULL;
    zs.opaque = NULL;
    // Initialize deflate
    int Err = deflateInit2(&zs, m_Level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (Err)
    {
        if (zs.msg)
        {
            strncpy(m_ErrMsg, zs.msg, sizeof(m_ErrMsg) - 1);
            m_ErrMsg[sizeof(m_ErrMsg) - 1] = 0;
        }
        return Err;
    }
    // Set deflate context
    zs.next_in   = m_InBuf;
    zs.avail_in  = m_AllRead ? (unsigned)m_EntrFileSz : 0;
    zs.next_out  = m_OutBuf;
    zs.avail_out = m_OutBufSz;
    int  Flush   = Z_NO_FLUSH;

    do
    {
        // IF input buffer empty AND not all input data read yet
        if (zs.avail_in == 0 && !m_AllRead)
        {
            int sz;
            // Read next part of input data
            Err = EntrRead(m_InBuf, m_InBufSz, &sz);
            if (Err)
                break;
            // IF read size < input buffer size, whole input read
            if (sz < m_InBufSz || sz==0)  // corrected by J.D.: for empty files m_InBufSz==0 and sz==0!
                m_AllRead = true;
            zs.next_in  = m_InBuf;
            zs.avail_in = sz;
        }
        // IF output buffer is full
        if (zs.avail_out == 0)
        {
            // Write buffer content to destination storage
            Err = FlushOutBuf(m_OutBufSz);
            if (Err)
                break;
            // IF zip to memory, reallocate output buffer
            if (m_ToMemory)
            {
                if (!((CZip602M *)this)->Realloc(m_InBufSz + 1024))
                {
                    Err = Z_MEM_ERROR;
                    break;
                }
            }
            zs.next_out  = m_OutBuf;
            zs.avail_out = m_OutBufSz;
        }
        // IF input buffer empty AND all input data read yet, set finish flag
        if (zs.avail_in == 0 && m_AllRead)
            Flush = Z_FINISH;
        // Deflate data
        Err = deflate(&zs, Flush);
        if (Err)
        {
            if (zs.msg)
            {
                strncpy(m_ErrMsg, zs.msg, sizeof(m_ErrMsg) - 1);
                m_ErrMsg[sizeof(m_ErrMsg) - 1] = 0;
            }
            break;
        }
    }
    while (Err != Z_STREAM_END);
 
    // IF compress succeded, write rest of output buffer to destination storage
    if (Err >= 0)
        Err = FlushOutBuf(m_OutBufSz - zs.avail_out);
    // Terminate deflate
    deflateEnd(&zs);
    return Err;
}
//
// Copies input data to archive
//
int CZip602::Copy()
{
    // Reallocate buffer
    if (m_ToMemory)
    {
        if (m_EntrFileSz >= SIZE_4GB)
            return Z_MEM_ERROR;
        if (!((CZip602M *)this)->Realloc((unsigned)m_EntrFileSz))
            return Z_MEM_ERROR;
    }
    else
    {
        m_OutBufSz = ReallocBuffer(m_EntrFileSz, m_OutBuf, m_OutBufSz);
        if (m_OutBufSz < 0)
            return Z_MEM_ERROR;
    }
    // sz = copied block size
    int sz = m_EntrFileSz < (QWORD)m_OutBufSz ? (int)m_EntrFileSz : m_OutBufSz;
    do
    {
        // IF not all input data read
        if (!m_AllRead)
        {
            // Read next part of input data
            int Err = EntrRead(m_OutBuf, m_OutBufSz, &sz);
            if (Err)
                return Err;
            // IF read size < buffer size, whole input read
            if (sz < m_OutBufSz || sz==0)  // corrected by J.D.: for empty files m_OutBufSz==0 and sz==0!
                m_AllRead = true;
        }
        // IF anything read, write it to destination storage
        if (sz)
        {
            int Err = FlushOutBuf(sz);
            if (Err)
                return Err;
        }
    }
    while (!m_AllRead);
    return Z_OK;
}
//
// Writes compressed file description to ZipEntryHdr and ZipEntryInfo
//
int CZip602::WriteHeader(bool SubDir)
{
    CFileDT fDT;
    WORD    dD, dT;
    // IF input file exist, get file modify date and time ELSE get current date and time
    if (m_EntrFile.Valid())
    {
        fDT.GetModifTime(&m_EntrFile);
    }
    else
    {
        fDT.Now();
    }
    // Convert date and time to dos format
    fDT.ToDosDT(&dD, &dT);
    // Get conversion flag
    WORD Flags = 0;
    if (m_Level >=8 )
        Flags |= 2;
    if (m_Level == 2)
        Flags |= 4;
    if (m_Level == 1)
        Flags |= 6;
    if (*m_Password)
        Flags |= 1;
    WORD Ver = m_EntrFileSz >= SIZE_4GB ? ZVER_4GB : ZVER_FAT;

    // Fill ZipEntryHdr
    int  hSz = sizeof(ZipEntryHdr) - 1;
    char Buf[sizeof(ZipEntryHdr) + sizeof(Zip4GB34) + MAX_PATH];
    LPZIPENTRYHDR pHdr = (LPZIPENTRYHDR)Buf;
    pHdr->ID           = PK34;
    pHdr->Version      = Ver;
    pHdr->Flags        = Flags;
    pHdr->Method       = m_Method;
    pHdr->fTime        = dT;
    pHdr->fDate        = dD;
    pHdr->CRC          = m_CRC;
    pHdr->ZipSize      = LODWORD(m_ZipSz);
    pHdr->fLength      = LODWORD(m_EntrFileSz);
    pHdr->fNameSz      = m_CurInfo->fNameSz;
    pHdr->ExtraBytes   = 0;
    memcpy(pHdr->fName, m_CurInfo->fName, m_CurInfo->fNameSz);
    hSz               += pHdr->fNameSz;
    // IF input file size >= 4 GB
    if (m_EntrFileSz >= SIZE_4GB)
    {
        // Set ExtraBytes size
        pHdr->ExtraBytes = sizeof(Zip4GB34);
        LPZIP4GB34 pgb   = (LPZIP4GB34)&pHdr->fName[pHdr->fNameSz];
        // Write extra info to ExtraBytes
        pgb->ID          = P4GB;
        pgb->_1          = 0;
        pgb->ZipSizeHigh = HIDWORD(m_ZipSz);
        pgb->fLengthHigh = HIDWORD(m_EntrFileSz);
        hSz             += sizeof(Zip4GB34);
    }
    // Write ZipEntryHdr to output storage
    int Err = Write(pHdr, hSz);
    if (Err)
        return Err;

    if (m_ZipPos >= SIZE_4GB)
        Ver = ZVER_4GB;
    // Fill ZipEntryInfo
    int iSz                  = sizeof(ZipEntryInfo) + pHdr->fNameSz - 1;
    m_CurInfo->ID            = PK12;
    m_CurInfo->VerMadeBy     = Ver;  
    m_CurInfo->VerNeedToExtr = Ver;  
    m_CurInfo->Flags         = Flags;
    m_CurInfo->Method        = m_Method;
    m_CurInfo->fTime         = dT;
    m_CurInfo->fDate         = dD;
    m_CurInfo->CRC           = m_CRC;
    m_CurInfo->ZipSize       = LODWORD(m_ZipSz);
    m_CurInfo->fLength       = LODWORD(m_EntrFileSz);
    m_CurInfo->ExtraBytesSz  = 0;
    m_CurInfo->CommentSz     = 0;
    m_CurInfo->DiskNO        = 0;
    m_CurInfo->iAttr         = 0;
    m_CurInfo->fAttr         = m_EntrFile.Valid() || SubDir ? GetFileAttr(m_fName) : 0;
    m_CurInfo->ZipPos        = LODWORD(m_ZipPos);
    // IF input file size >= 4 GB OR current zip size >= 4 GB
    if (m_EntrFileSz >= SIZE_4GB || m_ZipPos >= SIZE_4GB)
    {
        // Set CommentSz
        m_CurInfo->CommentSz = sizeof(Zip4GB12);
        LPZIP4GB12 pgb       = (LPZIP4GB12)&m_CurInfo->fName[m_CurInfo->fNameSz];
        // Write extra info to Comment
        pgb->ID              = P4GB;
        pgb->_1              = 0;
        pgb->ZipSizeHigh     = HIDWORD(m_ZipSz);
        pgb->fLengthHigh     = HIDWORD(m_EntrFileSz);
        pgb->ZipPosHigh      = HIDWORD(m_ZipPos);
        iSz                 += sizeof(Zip4GB12);
    }

    // Set offset of next file
    m_ZipPos += hSz + m_ZipSz;

    // Set pointer to next ZipEntryInfo
    m_CurInfo = (LPZIPENTRYINFO)((LPBYTE)m_CurInfo + iSz);
    m_fCnt++;
    return Z_OK;
}
//
// Calculates input file CRC
//
int CZip602::GetCRC()
{
    m_CRC = 0;
    // Reallocate input buffer
    m_InBufSz = ReallocBuffer(m_EntrFileSz, m_InBuf, m_InBufSz);
    if (m_InBufSz < 0)
        return Z_MEM_ERROR;

    bool AllRead = false;
    do
    {
        int sz; 
        // Read next part of input data
        int Err = EntrRead(m_InBuf, m_InBufSz, &sz);
        if (Err)
            return Err;
        // IF read size < buffer size, whole input read
        if (sz < m_InBufSz || sz==0)  // corrected by J.D.: for empty files m_InBufSz==0 and sz==0!
            AllRead = true;
        // IF anything read, calculate CRC
        if (sz)
            m_CRC = crc32(m_CRC, m_InBuf, sz);
    }
    while (!AllRead);
    // IF input file size <= input buffer size, all input data read
    if (m_EntrFileSz <= m_InBufSz)
        m_AllRead = true;
    // IF input from stream, seek input stream back to begin
    else if (m_InStream)
    {
        if (m_InStream->Seek(m_InStream->Param, 0, SEEK_SET) < 0)
            return Z_602STREAMERR;
    }
    // ELSE seek input file back to begin
    else
    {
        if (m_EntrFile.Seek(0) < 0)
        {
            m_ErrNO = m_EntrFile.Error();
            return Z_ERRNO;
        }
    }
    return Z_OK;
}
//
// Reads input data
// Input:   Data    - Buffer for data
//          Size    - Buffer size
//          pSize   - Size realy read
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602::EntrRead(void *Data, int Size, int *pSize)
{
    int sz;
    // IF input from stream, read from stream
    if (m_InStream)
    {
        sz = m_InStream->Read(m_InStream->Param, Data, Size);
        if (sz < 0)
            return Z_602STREAMERR; 
    }
    // ELSE read from file
    else
    {
        sz = m_EntrFile.Read(Data, Size);
        if (sz < 0)
        {
            m_ErrNO = m_EntrFile.Error();
            return Z_ERRNO;
        }
    }
    if (pSize)
        *pSize = sz;
    return Z_OK;
}
//
// Converts file name to ASCII, removes disc name, removes leading \\ or /, replaces \ with /
// Input:   Src     - Input file name
//          Dst     - Buffer for resulting name
// Returns: true on success
//
bool CZip602::RegulateFileName(const wchar_t *Src, char *Dst)
{
    char dName[MAX_PATH];
    if (W2D(Src, dName, sizeof(dName)) <= 0)
        return false;
    char *ps = strchr(dName, ':');
    if (ps)
    {
        ps++;
        if (*ps = '\\')
            ps++;
    }
    else if (*(WORD *)dName == '\\' + ('\\' << 8))
    {
        ps = strchr(dName + 2, '\\');
        if (!ps)
        {
            *Dst = 0;
            return true;
        }
        ps++;
    }
    else if (*dName == '/' || *dName == '\\')
        ps = (char *)dName + 1;
    else
        ps = (char *)dName;
    char *pd;
    for (pd = Dst; *ps; ps++, pd++)
        *pd = *ps == '\\' ? '/' : *ps;
    *pd = 0;
    return true;
}
//
// Writes file name to ZipEntryInfo
//
bool CZip602::FileNameForHdr(const wchar_t *fName)
{
    char Buf[MAX_PATH];
    // Convert name to ASCII, remove leading disc name, \\ or /, replace \ with /
    if (!RegulateFileName(fName, Buf))
        return false;
    char *ps = Buf;
    // IF do not store folder names 
    if (m_NoFolders)
    {
        // Get pointer to file name
        ps = strrchr(Buf, '/');
        if (!ps)
            ps = Buf;
    }
    // IF input file belongs to RootDir, remove RootDir part from file name
#ifdef _WIN32
    else if (strnicmp(Buf, m_RootDir, m_RootDirSz) == 0)
#else
    else if (strncmp(Buf, m_RootDir, m_RootDirSz) == 0)
#endif
        ps = Buf + m_RootDirSz;
    // Write file name to ZipEntryInfo
    strcpy(m_CurInfo->fName, ps);
    m_CurInfo->fNameSz = (WORD)strlen(m_CurInfo->fName);
    return true;
}
//
// Closes zip archive
//
int CZip602::Close()
{
    // IF no files added to zip archive, delete zip file
    if (!m_fCnt)
    {
        Remove();
        return Z_OK;
    }
    // Get archive directory size
    DWORD DirSize  = (DWORD)((LPBYTE)m_CurInfo - m_ZipInfo);
    // Get archive directory + ZipFileInfo size
    DWORD InfoSize = DirSize + sizeof(ZipFileInfo) - 1;
    // IF Archive size >= 4 GB, add extra info size
    if (m_ZipPos >= SIZE_4GB)
        InfoSize += sizeof(Zip4GB56);
    // Reallocate archive directory size
    if (InfoSize < m_InfoSz)
    {
        DWORD iOff = (DWORD)((LPBYTE)m_CurInfo - m_ZipInfo);
        m_ZipInfo  = (LPBYTE)realloc(m_ZipInfo, InfoSize);
        if (!m_ZipInfo)
            return Z_MEM_ERROR;
        m_CurInfo  = (LPZIPENTRYINFO)(m_ZipInfo + iOff);
    }
    // IF some file added to zip archive
    if (m_fCnt)
    {
        // Fill ZipFileInfo
        LPZIPFILEINFO pfi = (LPZIPFILEINFO)m_CurInfo;
        pfi->ID           = PK56;
        pfi->DiskNO       = 0;
        pfi->DirDiskNO    = 0;
        pfi->LocFileCnt   = m_fCnt;
        pfi->FileCnt      = m_fCnt;
        pfi->DirSize      = DirSize;
        pfi->DirSeek      = DWORD(m_ZipPos);
        pfi->MsgSz        = 0;
        // IF Archive size >= 4 GB, fill extra info
        if (m_ZipPos >= SIZE_4GB)
        {
            pfi->MsgSz     = sizeof(Zip4GB56);
            LPZIP4GB56 pgb = (LPZIP4GB56)pfi->Msg;
            pgb->ID        = P4GB;
            pgb->_1        = 0;
            pgb->SeekHigh  = HIDWORD(m_ZipPos);
        }
    }
    // Write archive directory to output storage
    return Write(m_ZipInfo, InfoSize);
}
//
// Returns current zip archive size
//
QWORD CZip602::Size()
{
    DWORD DirSize  = (DWORD)((LPBYTE)m_CurInfo - m_ZipInfo);    // Archive directory size
    DWORD InfoSize = DirSize + sizeof(ZipFileInfo) - 1;         // += size of ZipFileInfo
    if (m_ZipPos >= SIZE_4GB)                                   // IF current zip offset > 4 GB
        InfoSize += sizeof(Zip4GB56);                           // += size of extra info
    return m_ZipPos + InfoSize;                                 // current zip offset
}
//
// Closes zip archive stored to memory
// Input:   Zip     - Output buffer
//          Size    - Output buffer size
// Returns: Z_OK on succes, 602zip error code on error
//
int CZip602M::Close(void *Zip, unsigned Size)
{
    // Close archive
    int Err = CZip602::Close();
    // Write result to output buffer
    if (!Err)
        memcpy(Zip, m_ZipBuf, m_OutBuf - m_ZipBuf);
    return Err;
}
//
// Writes output data to output file
// Input:   Size    - Size of data
// Returns: Z_OK on succes, 602zip error code on error
// 
int CZip602F::FlushOutBuf(unsigned Size)
{
    if (Size)
    {
        // Increment compressed size
        m_ZipSz += Size;
        // IF password specified, encrypt data
        if (*m_Password)
        {
            int t;
            for (unsigned i = 0; i < Size; i++)
                m_OutBuf[i] = zencode(m_Keys, m_CRCTab, m_OutBuf[i], t);
        }
        // Write data to destination storage
        int Err = Write(m_OutBuf, Size);
        if (Err)
            return Err;
    }
    return Z_OK;
}
//
// Reallocates output buffer
// Input:   Size  - Requested buffer increment
// Returns: true on success
//
bool CZip602M::Realloc(unsigned Size)
{
    // Get actual output data size
    int Off     = (int)(m_OutBuf - m_ZipBuf);
    // Get new size
    int NewSize = Off + Size;
    // IF new size > current buffer size, reallocate
    if (NewSize <= m_ZipBufSz)
        return true;
    m_ZipBuf = (LPBYTE)realloc(m_ZipBuf, NewSize);
    if (!m_ZipBuf)
    {
        m_OutBuf = NULL;
        return false;
    }
    m_ZipBufSz = NewSize;
    m_OutBuf   = m_ZipBuf + Off;
    m_OutBufSz = NewSize  - Off;
    return true;
}
//
// Flushes output data to memory zip archive - only encryptes output data if requested
// Input:   Size    - Size of data
// Returns: Z_OK on succes, 602zip error code on error
// 
int CZip602M::FlushOutBuf(unsigned Size)
{
    if (Size)
    {
        // Increment compressed size
        m_ZipSz += Size;
        // IF password specified, encrypt data
        if (*m_Password)
        {
            int t;
            for (unsigned i = 0; i < Size; i++)
                m_OutBuf[i] = zencode(m_Keys, m_CRCTab, m_OutBuf[i], t);
        }
        m_OutBuf   += Size;
        m_OutBufSz -= Size;
    }
    return Z_OK;
}
//
// Seek in memory zip archive
// Input:   Offset  - New position
//          Org     - New position origin
// Returns: Z_OK on succes, 602zip error code on error
// 
int CZip602M::Seek(QWORD Offset, int Org)
{
    // Get new buffer pointer
    if (Org == SEEK_SET)
        m_OutBuf  = m_ZipBuf + Offset;
    else
        m_OutBuf += Offset;
    // IF new position is after buffer end, reallocate buffer
    int Dif = (int)(m_OutBuf - (m_ZipBuf + m_ZipBufSz));
    if (Dif > 0 && !Realloc(0))
        return Z_MEM_ERROR;
    m_OutBufSz = m_ZipBufSz - (int)(m_OutBuf - m_ZipBuf);
    return Z_OK;
}
//
// Writes data to memory zip archive
// Input:   Data  - Pointer to data
//          Size  - Data size
// Returns: Z_OK on succes, 602zip error code on error
// 
int CZip602M::Write(const void *Data, int Size)
{
    // Reallocate buffer
    if (!Realloc(Size))
        return Z_MEM_ERROR;
    // Write data to buffer
    memcpy(m_OutBuf, Data, Size);
    m_OutBuf   += Size;
    m_OutBufSz -= Size;
    return Z_OK;
}
//
// Seek in stream zip archive
// Input:   Offset  - New position
//          Org     - New position origin
// Returns: Z_OK on succes, 602zip error code on error
// 
int CZip602S::Seek(QWORD Offset, int Org)
{
    return m_Stream.Seek(m_Stream.Param, Offset, Org) >= 0 ? Z_OK : Z_602STREAMERR;
}
//
// Writes data to stream zip archive
// Input:   Data  - Pointer to data
//          Size  - Data size
// Returns: Z_OK on succes, 602zip error code on error
// 
int CZip602S::Write(const void *Data, int Size)
{
    return m_Stream.Write(m_Stream.Param, Data, Size) ? Z_OK : Z_602STREAMERR;
}
//
// Flushes output data to stream zip archive
// Input:   Size    - Size of data
// Returns: Z_OK on succes, 602zip error code on error
// 
int CZip602S::FlushOutBuf(unsigned Size)
{
    if (Size)
    {
        // Increment compressed size
        m_ZipSz += Size;
        // IF password specified, encrypt data
        if (*m_Password)
        {
            int t;
            for (unsigned i = 0; i < Size; i++)
                m_OutBuf[i] = zencode(m_Keys, m_CRCTab, m_OutBuf[i], t);
        }
        // Write data to destination storage
        int Err = Write(m_OutBuf, Size);
        if (Err)
            return Err;
    }
    return Z_OK;
}
//
// Returns error message corresponding preceeding zip operation failure
//
const char *CZip602::ErrMsg()
{
    if (!*m_ErrMsg)
    {
        if (m_ErrNO)
#ifdef _WIN32
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, m_ErrNO, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), m_ErrMsg, sizeof(m_ErrMsg), NULL);
#else
        {
            strncpy(m_ErrMsg, strerror(m_ErrNO), sizeof(m_ErrMsg) - 1);
            m_ErrMsg[sizeof(m_ErrMsg) - 1] = 0;
        }
#endif
    }
    return m_ErrMsg;
}
