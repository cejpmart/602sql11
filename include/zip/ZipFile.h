//
// File utilities for 602zip
//
#include <sys/stat.h>
#include <time.h>
#include "ZipDefs.h"

#ifdef _WIN32 

#define PATH_DELIM    '\\'
#define PATH_FORDELIM '/'
#ifndef INVALID_SET_FILE_POINTER  // not defined in VC 6.0
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif
//
// File encapsulation, hides differencies between Windows and Linux file imlementation
//
class CZipFile
{
protected:

    HANDLE m_hFile;     // File handle

public:

    CZipFile(){m_hFile = INVALID_HANDLE_VALUE;}
   ~CZipFile()
    {
        if (m_hFile != INVALID_HANDLE_VALUE)
            CloseHandle(m_hFile);
    }
    // Creates file for write access
    bool Create(const char *FileName)
    {
        m_hFile = CreateFileA(FileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
        return m_hFile != INVALID_HANDLE_VALUE;
    }
    // Opens file for read access
    bool Open(const char *FileName)
    {
        if (m_hFile != INVALID_HANDLE_VALUE)
            CloseHandle(m_hFile);
        m_hFile = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        return m_hFile != INVALID_HANDLE_VALUE;
    }
    // Returns true if file is open
    bool Valid(){return m_hFile != INVALID_HANDLE_VALUE;}
    // Closes file
    void Close()
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
    // Writes data to file
    bool Write(const void *Buf, unsigned Size)
    {
        DWORD WrSize;
        return WriteFile(m_hFile, Buf, Size, &WrSize, NULL) != FALSE;
    }
    // Reads data from file
    int Read(void *Buf, unsigned Size)
    {
        DWORD RdSize;
        return ReadFile(m_hFile, Buf, Size, &RdSize, NULL) ? (int)RdSize : -1;
    }
    // Deletes file
    bool Delete(const char *FileName)
    {
        return DeleteFile(FileName) != 0;
    }
    // Returns last error code
    static int Error(){return GetLastError();}
    // Returns file size
    inline QWORD GetHugeSize(){DWORD Lower, Upper; Lower = GetFileSize(m_hFile, &Upper); return(((QWORD)Upper << 32) | Lower);}
    // Seeks to given position
    inline QWORD Seek(QWORD Pos, int Org = FILE_BEGIN)
    {
        LONG  HighPos  = (LONG)(Pos >> 32); 
        DWORD LoResult = SetFilePointer(m_hFile, (ULONG)(Pos % SIZE_4GB), &HighPos, Org);
        if (LoResult == INVALID_SET_FILE_POINTER && GetLastError())
            return (QWORD)-1;
        return (QWORD)LoResult  | ((QWORD)HighPos << 32);
    }
    friend class CFileDT;
};
//
// Folder structure search, hides differencies between Windows and Linux folder search imlementation
//
class CFindFile : public WIN32_FIND_DATAW
{
protected:

    HANDLE   m_hFD;                 // Find handle
    wchar_t  m_Folder[MAX_PATH];    // Current file path buffer
    wchar_t *m_fName;               // Pointer to file name in path buffer
    bool     m_SubDirs;             // Return subfolders too

public:

    CFindFile(){m_hFD = 0;}
    void FindClose()
    {
        if (m_hFD)
        {
            ::FindClose(m_hFD);
            m_hFD = 0;
        }
    }
    ~CFindFile(){FindClose();}


    bool  FindFirst(const wchar_t *fName, bool SubDirs);
    bool  FindNext();

    char *GetfType();

    inline const wchar_t *GetFileName() const
    {
        lstrcpyW(m_fName, cFileName);
        return m_Folder;
    }
    inline bool Acceptable()
    {
        // IF subfolders too, check if file name is not . OR ..
        if (m_SubDirs)
            return (*(DWORD *)cFileName != *(DWORD *)L".") && ((*(DWORD *)cFileName != *(DWORD *)L"..") || cFileName[2] != 0);
        // ELSE ordinary files only, check if file is not folder
        else
            return !(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    bool IsFolder(){return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;}
    bool IsEmpty(){return GetLastError() == ERROR_NO_MORE_FILES;}
};
//
// Reterns file flags
//
inline unsigned GetFileAttr(const char *fName)
{ 
    unsigned Result = GetFileAttributesA(fName);
    if (Result & FILE_ATTRIBUTE_DIRECTORY)          // IF folder, supply libc style flag
         Result |= S_IFDIR;
    return Result;
}
//
// File date and time, hides differencies between Windows and Linux file imlementation
//
// FileTimeToLocalFileTime function returns for all file times in winter UTC + 1 in summer UTC + 2,
// therefore Windows presents for same file other time in winter then in summer
// 
class CFileDT
{
private:

    FILETIME m_tm;  // File date and time value

public:

    CFileDT(){}
    // Conversion from DOS date and time
    bool FromDosDT(WORD Date, WORD Time)
    {
        FILETIME   ft;
        return DosDateTimeToFileTime(Date, Time, &ft) != FALSE &&
               LocalFileTimeToFileTime(&ft, &m_tm) != FALSE;
    }
    // Conversion to DOS date and time
    bool ToDosDT(WORD *pDate, WORD *pTime)
    {
        FILETIME ft;
        return FileTimeToLocalFileTime(&m_tm, &ft) != FALSE &&
               FileTimeToDosDateTime(&ft, pDate, pTime) != FALSE;
    }
    // Conversion to time_t
    inline operator time_t() const
    {
        FILETIME ft;
        if (!FileTimeToLocalFileTime(&m_tm, &ft))
            return 0;
        SYSTEMTIME st;
        if (!FileTimeToSystemTime(&ft, &st))
            return 0;
        struct tm ltm;
        ltm.tm_year  = st.wYear  - 1900;
        ltm.tm_mon   = st.wMonth - 1;
        ltm.tm_mday  = st.wDay;
        ltm.tm_hour  = st.wHour;
        ltm.tm_min   = st.wMinute;
        ltm.tm_sec   = st.wSecond;
        ltm.tm_isdst = -1;
        return mktime(&ltm);
    }
    // Conversion to FILETIME
    inline operator FILETIME() const
    {
        return m_tm;
    }
    // Reads last file change date und time
    inline bool GetModifTime(CZipFile *fl)
    {
        return GetFileTime(fl->m_hFile, NULL, NULL, &m_tm) != FALSE;
    }
    // Reads current date and time
    inline void Now()
    {
        GetSystemTimeAsFileTime(&m_tm);
    }
};

#else   // LINUX

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#define PATH_DELIM '/'
//
// File encapsulation hides differencies between Windows and Linux file imlementation
//
class CZipFile
{
protected:

    int m_hFile;    // File handle

public:

    CZipFile(){m_hFile = -1;}
    // Creates file for write access
    bool Create(const char *FileName)
    {
        m_hFile = open(FileName, O_CREAT | O_WRONLY | O_TRUNC, 0755);
        return m_hFile != -1;
    }
    // Opens file for read access
    bool Open(const char *FileName)
    {
        if (m_hFile != -1)
            close(m_hFile);
        m_hFile = open(FileName, 0, 0);
        return m_hFile != -1;
    }
    // Returns true if file is open
    bool Valid(){return m_hFile != -1;}
    // Closes file
    void Close()
    {
        close(m_hFile);
        m_hFile = -1;
    }
    // Writes data to file
    bool Write(const void *Buf, unsigned Size)
    {
        return write(m_hFile, Buf, Size) != -1;
    }
    // Reads data from file
    int Read(void *Buf, unsigned Size)
    {
        return read(m_hFile, Buf, Size);
    }
    // Deletes file
    bool Delete(const char *FileName)
    {
        return remove(FileName) == 0;
    }
   ~CZipFile()
    {
        if (m_hFile != -1)
            close(m_hFile);
    }

    // Returns last error code
    static int Error(){return errno;}
    // Returns file size
    inline QWORD GetHugeSize(){return lseek(m_hFile, 0, SEEK_END);}
    // Seeks to given position
    inline QWORD Seek(QWORD Pos, int Org = SEEK_SET){return lseek(m_hFile, Pos, Org);}
    
    friend class CFileDT;
};
//
// Reterns file flags
//
inline unsigned GetFileAttr(const char *fName)
{
    struct stat statbuf;
    if (stat(fName, &statbuf) == -1)
        return((unsigned)-1);
    unsigned Result = 0;
    if (!(statbuf.st_mode & S_IWOTH))
        Result |= FILE_ATTRIBUTE_READONLY;
    if (S_ISDIR(statbuf.st_mode))
        Result |= FILE_ATTRIBUTE_DIRECTORY | S_IFDIR;
    const char *pn = strrchr(fName, '/');
    if (!pn)
        pn = fName;
    if (*pn == '.')
        Result |= FILE_ATTRIBUTE_HIDDEN;
    return Result;
}
//
// Enables folder structure search hides differencies between Windows and Linux folder search imlementation
//
class CFindFile
{
protected:

    DIR           *m_Dir;               // Directory handle
    bool           m_SubDirs;           // Return subfolders too
    struct dirent *m_de;                // Current directory entry
    wchar_t        m_Folder[MAX_PATH];  // Current file path buffer UNICODE
    wchar_t       *m_fName;             // Pointer to file name in path buffer
    char           m_aFolder[MAX_PATH]; // Current file path buffer ASCII
    char          *m_afName;            // Pointer to file name in path buffer
    bool           m_Empty;             // Empty flag

public:

    CFindFile(){m_Dir = NULL;}
    void FindClose()
    {
        if (m_Dir)
        {
            closedir(m_Dir);
            m_Dir = NULL;
        }
    }
    ~CFindFile(){FindClose();}

    bool FindFirst(const wchar_t *fName, bool SubDirs);
    bool FindNext();
    bool IsFolder()
    {
        strcpy(m_afName, m_de->d_name);
        struct stat statbuf;
        if (stat(m_aFolder, &statbuf) != 0)
            return false;
        return S_ISDIR(statbuf.st_mode);
    }
    bool Acceptable()
    {
        if (m_SubDirs)
            return (*(WORD *)m_de->d_name != *(WORD *)".") && ((*(WORD *)m_de->d_name != *(WORD *)"..") || m_de->d_name[2] != 0);
        else
            return !IsFolder();
    }
    inline const wchar_t *GetFileName() const{return m_Folder;}
    bool IsEmpty(){return m_Empty;}
};

class CFileDT
{
private:

    time_t m_tm;    // File date and time value

public:

    CFileDT(){}
    // Conversion from DOS date and time
    bool FromDosDT(WORD Date, WORD Time)
    {
        struct tm ldt;
        ldt.tm_sec   = (Time & 0x1F) * 2;
        ldt.tm_min   = (Time >> 5) & 0x3F;
        ldt.tm_hour  = (Time >> 11);
        ldt.tm_mday  = (Date & 0x1F);
        ldt.tm_mon   = ((Date >> 5) & 0x0F) - 1;
        ldt.tm_year  = (Date >> 9) + 80;
        ldt.tm_isdst = -1;
        m_tm         = mktime(&ldt);
        return m_tm != (time_t)-1;
    }
    // Conversion to DOS date and time
    bool ToDosDT(WORD *pDate, WORD *pTime)
    {
        struct tm *ldt = localtime(&m_tm);

        *pDate = ldt->tm_mday + ((ldt->tm_mon + 1) << 5) + ((ldt->tm_year - 80) << 9);
        *pTime = (ldt->tm_sec / 2) + (ldt->tm_min << 5) + (ldt->tm_hour << 11);
        return true;
    }
    // Conversion to time_t
    inline operator time_t() const
    {
        return(m_tm);
    }
    // Assignment operator
    inline time_t operator =(const time_t &Other)
    {
        m_tm = Other;
        return(m_tm);
    }
    // Reads last file change date und time
    inline bool GetModifTime(CZipFile *fl)
    {
        m_tm = 0;
        struct stat statbuf;
        if (fstat((int)fl->m_hFile, &statbuf) != 0)
            return false;
        m_tm = statbuf.st_mtime;
        return true;
    }
    // Reads current date and time
    inline void Now()
    {
        m_tm = time(NULL);
    }
};

#endif // _WIN32

bool ZipIsDirectory(const char *fName);
bool SetFileTime(const char *fName, CFileDT DT);
