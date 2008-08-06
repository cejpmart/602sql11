#ifndef _WBFILE_H_
#define _WBFILE_H_

//#ifdef WINS
//#include <windows.h>
//#else
//#include "winrepl.h"
//#endif
#include <sys/stat.h>
#ifdef WINS
#include <sys/utime.h>
#else
#include <utime.h>
#endif
#include <time.h>
#include "exceptions.h"
#include "buffer.h"

#define WBF_INVALIDSIZE (unsigned)-1
#define gettext_noopL(string) L##string
//
// Predpoklada soubory < 4 GB, az bude potreba pro vetsi, doimplementujou se metoty Seek64 a Size64
//
class CWBFile
{
protected:

    FHANDLE m_Handle;

public:

    CWBFile(){m_Handle = INVALID_FHANDLE_VALUE;}
    ~CWBFile(){ Close(); }
    void Close()
    {
        if (m_Handle != INVALID_FHANDLE_VALUE)
        {
            CloseHandle(m_Handle);
            m_Handle = INVALID_FHANDLE_VALUE;
        }
    }
    bool Open(const char *fName, DWORD Mode = GENERIC_READ)
    {
        Close();
        m_Handle = CreateFileA(fName, Mode, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        return(m_Handle != INVALID_FHANDLE_VALUE); 
    }
    bool Create(const char *fName, bool Overwrite = false)
    {
        Close();
        m_Handle = CreateFileA(fName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, Overwrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        return(m_Handle != INVALID_FHANDLE_VALUE); 
    }
    unsigned Read(void *Buffer, unsigned Size)
    {
        DWORD rd;
        if (ReadFile(m_Handle, Buffer, Size, &rd, NULL))
            return(rd);
        else
            return(WBF_INVALIDSIZE);
    }
    unsigned Write(const void *Buffer, unsigned Size)
    {
        DWORD wr;
        if (WriteFile(m_Handle, Buffer, Size, &wr, NULL))
            return(wr);
        else
            return(WBF_INVALIDSIZE);
    }
    unsigned Write(const wchar_t *Buffer)
    {
        DWORD wr;
        CBuffer Buf;
        if (!Buf.Alloc((unsigned)wcslen(Buffer) + 1))
            return(WBF_INVALIDSIZE);
        FromUnicode(Buffer, Buf, Buf.GetSize());
        if (!WriteFile(m_Handle, Buf, (int)strlen(Buf), &wr, NULL))
            return(wr);
        else
            return(WBF_INVALIDSIZE);
    }
    unsigned Seek(long Distance, unsigned Method = FILE_BEGIN)
    {
        return(SetFilePointer(m_Handle, Distance, NULL, Method));
    }
    unsigned Size()
    {
        return(GetFileSize(m_Handle, NULL));
    }
    operator FHANDLE() const{return(m_Handle);}
};

class CWBFileX
{
protected:

    FHANDLE m_Handle;

public:

    CWBFileX(){m_Handle = INVALID_FHANDLE_VALUE;}
    ~CWBFileX(){Close();}
    void Close()
    {
        if (m_Handle != INVALID_FHANDLE_VALUE)
        {
            try
            {
                CloseHandle(m_Handle);
            }
            catch (...)
            {
            }
            m_Handle = INVALID_FHANDLE_VALUE;
        }
    }
    void Closed(){m_Handle = INVALID_FHANDLE_VALUE;}
    void Open(const char *fName, DWORD Mode = GENERIC_READ);
    void Create(const char *fName, bool Overwrite = false);
    unsigned Read(void *Buffer, unsigned Size)
    {
        DWORD rd;
        if (!ReadFile(m_Handle, Buffer, Size, &rd, NULL))
        {
            wchar_t Fmt[64];
            throw new CSysException(L"ReadFile", Get_transl(NULL, gettext_noopL("Cannot read file"), Fmt));
        }
        return(rd);
    }
    void Write(const void *Buffer, unsigned Size)
    {
        DWORD wr;
        if (!WriteFile(m_Handle, Buffer, Size, &wr, NULL))
        {
            wchar_t Fmt[64];
            throw new CSysException(L"WriteFile", Get_transl(NULL, gettext_noopL("Cannot write file"), Fmt));
        }
    }
    void Write(const wchar_t *Buffer)
    {
        DWORD wr;
        CBuffer Buf;
        Buf.AllocX((int)wcslen(Buffer) + 1);
        FromUnicode(Buffer, Buf, Buf.GetSize());
        if (!WriteFile(m_Handle, Buf, (int)strlen(Buf), &wr, NULL))
        {
            wchar_t Fmt[64];
            throw new CSysException(L"WriteFile", Get_transl(NULL, gettext_noopL("Cannot write file"), Fmt));
        }
    }
    void Write(const char *Buffer)
    {
        DWORD wr;
        if (!WriteFile(m_Handle, Buffer, (int)strlen(Buffer), &wr, NULL))
        {
            wchar_t Fmt[64];
            throw new CSysException(L"WriteFile", Get_transl(NULL, gettext_noopL("Cannot write file"), Fmt));
        }
    }
    unsigned Seek(long Distance, unsigned Method = FILE_BEGIN)
    {
        unsigned Result = SetFilePointer(m_Handle, Distance, NULL, Method);
        if (Result == WBF_INVALIDSIZE)
        {
            wchar_t Fmt[64];
            throw new CSysException(L"SetFilePointer", Get_transl(NULL, gettext_noopL("File seek failed"), Fmt));
        }
        return(Result);
    }
    unsigned Size()
    {
        unsigned Result = GetFileSize(m_Handle, NULL);
        if (Result == WBF_INVALIDSIZE)
        {
            wchar_t Fmt[64];
            throw new CSysException(L"GetFileSize", Get_transl(NULL, gettext_noopL("Cannot read file size"), Fmt));
        }
        return(Result);
    }
    operator FHANDLE() const{return(m_Handle);}
};

class CWBFileTime
{
protected:

    time_t m_Val;

public:

    CWBFileTime(){}
    CWBFileTime(time_t Val){m_Val = Val;}
    CWBFileTime(uns32 Stamp)
    {
        m_Val = time(NULL);
        if (Stamp != NONETIME)
        {
            struct tm tms;

            unsigned dt  = Stamp / (60 * 60 * 24);
            unsigned tm  = Stamp % (60 * 60 * 24);
            tms.tm_mday  = (dt % 31) + 1;  dt = dt / 31;
            tms.tm_mon   = (dt % 12);
            tms.tm_year  = (dt / 12) + 1900;
            tms.tm_hour  =  tm / (60 * 60);
            tms.tm_min   = (tm % (60 * 60)) / 60;
            tms.tm_sec   =  tm % 60;
            tms.tm_isdst = localtime(&m_Val)->tm_isdst;
            m_Val        = mktime(&tms);
        }
    }

    void Now(){m_Val = time(NULL);}
    bool operator <(const CWBFileTime &Other){return m_Val < Other.m_Val;}
    operator time_t(){return m_Val;} 
};

CWBFileTime GetModifTime(const char *filename);
CWBFileTime GetAccessTime(const char *filename);
void SetFileTimes(const char *filename, CWBFileTime AccTime, CWBFileTime ModTime);
bool FileExists(const char *filename);
bool IsDirectory(const char *fName);
void SysErrorMsg(char *Buf, int Size);

#endif // _WBFILE_H_

