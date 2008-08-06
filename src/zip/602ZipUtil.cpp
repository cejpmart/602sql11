//
// 602zip utilities
//
#ifdef   _WIN32
#include <windows.h>
#else
#include <langinfo.h>
#include <stdlib.h>
#include <utime.h>
#include <wchar.h>
#include <iconv.h>
#endif
#include <sys/stat.h>
#include "ZipFile.h"
//
// Reallocates buffer, maximum 1 MB
// Input:   NewSize     - Requested size
//          Buf         - Buffer reference
//          OldSize     - Current size
// Returns: New buffer size, -1 if out of memory
//
int ReallocBuffer(QWORD NewSize, LPBYTE &Buf, int OldSize)
{
    int Size = OldSize;
    if (NewSize > (QWORD)OldSize && OldSize < SIZE_1MB)
    {
        Size = SIZE_1MB;
        if (NewSize <= (QWORD)SIZE_1MB)
            Size = (int)NewSize;
        Buf = (LPBYTE)realloc(Buf, Size);
        if (!Buf)
            return -1;
    }
    return Size;
}
//
// Returns true if fName is existing folder name
//
bool ZipIsDirectory(const char *fName)
{
    struct stat statbuf;
    char Buf[MAX_PATH];
    size_t sz = strlen(fName) - 1;
    if (fName[sz] == PATH_DELIM)
    {
        memcpy(Buf, fName, sz);
        Buf[sz] = 0;
        fName = Buf;
    }
    if (stat(fName, &statbuf) == -1)
    {
#ifdef _DEBUG
#ifdef LINUX
        int Err = errno;
#else        
        int Err = GetLastError();
#endif        
#endif
        return false;
    }
    return (statbuf.st_mode & S_IFDIR) != 0;
}

#ifdef _WIN32
//
// Sets file date and time, returns true on success
//
bool SetFileTime(const char *fName, CFileDT DT)
{
    HANDLE hFile = CreateFileA(fName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    FILETIME ft = DT;
    BOOL OK = SetFileTime(hFile, &ft, &ft, &ft);
    CloseHandle(hFile);
    return OK != 0;
}
//
// Converts text from UNICODE to ASCII
//
int W2M(const wchar_t *Src, char *Dst, int DstSz)
{
    return(WideCharToMultiByte(CP_ACP, NULL, Src, -1, Dst, DstSz, NULL, NULL));
}
//
// Converts text from ASCII to UNICODE
//
int M2W(const char *Src, wchar_t *Dst, int DstSz)
{
    return(MultiByteToWideChar(CP_ACP, NULL, Src, -1, Dst, DstSz));
}
//
// Converts text from ASCII to UNICODE
//
int M2W(const char *Src, int SrcSz, wchar_t *Dst, int DstSz)
{
    return(MultiByteToWideChar(CP_ACP, NULL, Src, SrcSz, Dst, DstSz));
}
//
// Converts text from UNICODE to DOS codepage
//
int W2D(const wchar_t *Src, char *Dst, int DstSz)
{
    return(WideCharToMultiByte(CP_OEMCP, NULL, Src, -1, Dst, DstSz, NULL, NULL));
}
//
// Converts text from DOS codepage to UNICODE
//
int D2W(const char *Src, int SrcSz, wchar_t *Dst, int DstSz)
{
    return(MultiByteToWideChar(CP_OEMCP, NULL, Src, SrcSz, Dst, DstSz));
}
//
// Finds first file corresponding given template
// Input:   fName   - File name template, can contain wildcard characters 
//          SubDirs - true return folders too, false return ordinary files only
// Returns: true if found
//
bool CFindFile::FindFirst(const wchar_t *fName, bool SubDirs)
{
    m_SubDirs   = SubDirs;
   *m_Folder    = 0;
    m_fName     = m_Folder;
    // Get last \ separator
    const wchar_t *ls = wcsrchr(fName, '\\');
    // IF found, copy folder part to m_Folder, set m_fName pointer
    if (ls)
    {
        int sz = (int)(ls - fName + 1);
        lstrcpynW(m_Folder, fName, sz + 1);
        m_fName += sz;
    }
    // Reset old search
    FindClose();

    // Find first file
    m_hFD  = FindFirstFileW(fName, this);
    if (m_hFD == INVALID_HANDLE_VALUE)
        return false;
    // IF acceptable, done
    if (Acceptable())
        return true;
    // ELSE find next
    return FindNext();
}
//
// Finds next file corresponding template specified in FindFirst
// Returns: true if found
//
bool CFindFile :: FindNext()
{
    // Search until not found
    do
    {
        if (!FindNextFileW(m_hFD, this))
            return false;
    }
    while (!Acceptable());
    return true;
}

extern "C" DllZip void WINAPI InitServerEnv()
{
}

#else   // LINUX
//
// Sets file date and time, returns true on success
//
bool SetFileTime(const char *fName, CFileDT DT)
{
    struct utimbuf tbuf;
    tbuf.actime  = DT;
    tbuf.modtime = DT;
    return utime(fName, &tbuf) == 0;
}
//
// Converts text from one charset to an other
//
static int LinuxConv(const char *Src, size_t SrcSz, char *Dst, size_t DstSz, const char *FromCharSet, const char *ToCharSet)
{
    *Dst = 0;
    iconv_t conv = iconv_open(ToCharSet, FromCharSet);
    if (conv == (iconv_t)-1)
        return 0;
    char *Result = Dst;
    while (SrcSz > 0)
    { 
        int nconv = iconv(conv, (char **)&Src, &SrcSz, &Result, &DstSz);
        if (nconv == (size_t)-1)
            break;
    }
    iconv_close(conv);
    return(Result - Dst);
}

const char *WCharSet = "UTF-32";    // Default Linux charset for UNICODE is UTF-32
//
// Converts text from UNICODE to ASCII
//
int W2M(const wchar_t *Src, char *Dst, int DstSz)
{
    return LinuxConv((const char *)Src, (wcslen(Src) + 1) * sizeof(wchar_t), Dst, DstSz, WCharSet, nl_langinfo(CODESET));
}
//
// Converts text from ASCII to UNICODE
//
int M2W(const char *Src, wchar_t *Dst, int DstSz)
{
    return LinuxConv(Src, strlen(Src) + 1, (char *)Dst, DstSz, nl_langinfo(CODESET), WCharSet) / sizeof(wchar_t);
}
//
// Converts text from ASCII to UNICODE
//
int M2W(const char *Src, int SrcSz, wchar_t *Dst, int DstSz)
{
    return LinuxConv(Src, SrcSz, (char *)Dst, DstSz * sizeof(wchar_t), nl_langinfo(CODESET), WCharSet) / sizeof(wchar_t);
}
//
// Converts text from UNICODE to DOS codepage
//
int W2D(const wchar_t *Src, char *Dst, int DstSz)
{
    int Result = LinuxConv((const char *)Src, (wcslen(Src) + 1) * sizeof(wchar_t), Dst, DstSz, WCharSet, "852");
    if (Result)
        return Result;
    Result = LinuxConv((const char *)Src, (wcslen(Src) + 1) * sizeof(wchar_t), Dst, DstSz, WCharSet, nl_langinfo(CODESET));
    return Result;
}
//
// Converts text from DOS codepage to UNICODE
//
int D2W(const char *Src, int SrcSz, wchar_t *Dst, int DstSz)
{
    int Result = LinuxConv(Src, SrcSz, (char *)Dst, DstSz * sizeof(wchar_t), "852", WCharSet);
    if (Result)
        return Result / sizeof(wchar_t);
    return LinuxConv(Src, SrcSz, (char *)Dst, DstSz * sizeof(wchar_t), nl_langinfo(CODESET), WCharSet) / sizeof(wchar_t);
}
//
// Finds first file corresponding given template
// Input:   fName   - File name template, can contain wildcard characters 
//          SubDirs - true return folders too, false return ordinary files only
// Returns: true if found
//
bool CFindFile::FindFirst(const wchar_t *fName, bool SubDirs)
{
    FindClose();

    m_Empty     = false;
   *m_Folder    = 0;
    m_fName     = m_Folder;
   *m_aFolder   = 0;
    m_afName    = m_aFolder;
    const wchar_t *ls = wcsrchr(fName, '\\');
    if (ls)
    {
        int sz = (int)(ls - fName + 1);
        wcsncpy(m_Folder, fName, sz + 1);
        m_fName += sz;
        W2M(m_Folder, m_aFolder, sizeof(m_aFolder));
        m_afName += strlen(m_aFolder);
    }

    char afName[MAX_PATH];
    W2M(fName, afName, sizeof(afName));
    m_Dir = opendir(afName);
    if (!m_Dir)
        return false;
    bool Found = FindNext();
    if (!Found)
        m_Empty = errno == 0;
    return Found;
}
//
// Finds next file corresponding template specified in FindFirst
// Returns: true if found
//
bool CFindFile::FindNext()
{
    do
    {
        m_de = readdir(m_Dir);
        if (!m_de)
            return false;
    }
    while (!Acceptable());
    M2W(m_de->d_name, m_fName, sizeof(m_Folder) / sizeof(wchar_t) - (m_fName - m_Folder));
    return true;
}

extern "C" DllZip void WINAPI InitServerEnv()
{
    WCharSet = "UCS-2";     // 602sql server charset for UNICODE is UCS-2
}

#endif // _WIN32
