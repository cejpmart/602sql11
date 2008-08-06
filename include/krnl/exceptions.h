#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#include <stdio.h>
#include "wbcdp.h"

#ifdef LINUX
#define _vsnwprintf vswprintf 
#include <stdarg.h>
#include <wchar.h>
#include <errno.h>
#endif

#ifdef gettext_noop
#undef  gettext_noop
#endif
#define gettext_noopL(string) L##string

class CWBException
{
protected:

    int      m_Err;
    wchar_t *m_Msg;

    wchar_t *Format(const wchar_t *format, va_list argptr)
    {
        wchar_t *Msg = NULL;
        for (unsigned sz = 256; ; sz *= 2)
        {
            Msg = (wchar_t *)corerealloc(Msg, sz * (int)sizeof(wchar_t));
            if (!Msg)
                break;
            size_t len = _vsnwprintf(Msg, sz, format, argptr);
            if (len >= 0 && len <= sz)
                break;
        }
        return Msg;
    }

public:

    CWBException(){m_Msg = NULL; m_Err = 0;}
    CWBException(const wchar_t *Msg, ...)
    {
        va_list va;
        va_start(va, Msg);
        m_Msg = Format(Msg, va);
        m_Err = 0;
    }
    virtual ~CWBException()
    {   
        if (m_Msg)
            corefree(m_Msg);
    }
    virtual const wchar_t *GetMsg(){return m_Msg;}
    int GetError(){return m_Err;} 
};

#define FuncCallFailed gettext_noop(L"Function %ls call failed\n\n%ls")

class C602SQLException : public CWBException
{
protected:

    cdp_t   m_cdp;
    wchar_t m_Func[32];

    void Init(cdp_t cdp, const wchar_t *Func = NULL, int Err = 0)
    {
        m_cdp  = cdp;
       *m_Func = 0;
        if (Err)
            m_Err = Err;
        else
            m_Err  = cd_Sz_error(cdp);
        wchar_t Msg[256];
        Get_error_num_textW(m_cdp, m_Err, Msg, sizeof(Msg) / sizeof(wchar_t));
        if (Func && *Func)
        {
            //wchar_t fmt[64];
            wcsncpy(m_Func, Func, sizeof(m_Func) / sizeof(wchar_t) - 1);  // padding with 0s!!
            m_Func[sizeof(m_Func) / sizeof(wchar_t) - 1] = 0;
        //    const wchar_t *Arg[2];
        //    Arg[0] = Func;
        //    Arg[1] = Msg;
        //    m_Msg = Format(Get_transl(cdp, FuncCallFailed, fmt), (va_list)Arg);
        }
        //else
        {
            m_Msg = (wchar_t *)corealloc(256 * sizeof(wchar_t), 69);
            if (m_Msg)
                wcscpy(m_Msg, Msg);
        }
    }

public:

    C602SQLException(cdp_t cdp){Init(cdp);}
    C602SQLException(cdp_t cdp, const wchar_t *Func){Init(cdp, Func);}
    C602SQLException(cdp_t cdp, const wchar_t *Func, int Err){Init(cdp, Func, Err);}

    virtual ~C602SQLException(){}
};

class CSysException : public CWBException
{
protected:

    wchar_t m_Func[32];

#ifdef WINS
    int  GetErr(){return (int)GetLastError();}
    void GetErrMsg(int Err, wchar_t *Buf){FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), Buf, 256, NULL);}
#else
    int  GetErr(){return errno;}
    void GetErrMsg(int Err, wchar_t *Buf){superconv(ATT_STRING, ATT_STRING, strerror(Err), (char *)Buf, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 2), NULL);}
#endif

    void Init(const wchar_t *Func = NULL, const wchar_t *Msg = NULL, int Err = 0)
    {
       *m_Func = 0;
        wchar_t Buf[256];
    
        m_Err = Err ? Err : GetErr();
        if (!Msg)
        {
            GetErrMsg(m_Err, Buf);
            Msg = Buf;
        }
        if (Func && *Func)
        {
            //wchar_t fmt[64];
            wcsncpy(m_Func, Func, sizeof(m_Func) / sizeof(wchar_t) - 1);
            m_Func[sizeof(m_Func) / sizeof(wchar_t) - 1] = 0;
        //    const wchar_t *Arg[2];
        //    Arg[0] = Func;
        //    Arg[1] = Msg;
        //    m_Msg = Format(Get_transl(GetCurrTaskPtr(), FuncCallFailed, fmt), (va_list)Arg);
        }
        //else
        {
            m_Msg = (wchar_t *)corealloc(256 * sizeof(wchar_t), 69);
            if (m_Msg)
                wcscpy(m_Msg, Msg);
        }
    }

public:

    CSysException(const wchar_t *Msg){Init(NULL, Msg);}
    CSysException(const wchar_t *Func, int Err){Init(Func, NULL, Err);}
    CSysException(const wchar_t *Func, const wchar_t *Msg, ...)
    {
        va_list va;
        va_start(va, Msg);
        wchar_t *msg = Format(Msg, va);
        Init(Func, msg);
        if (msg)
            corefree(msg);
    }
    CSysException(const wchar_t *Func, unsigned long Err, const wchar_t *Msg, ...)
    {
        va_list va;
        va_start(va, Msg);
        wchar_t *msg = Format(Msg, va);
        Init(Func, msg, Err);
        if (msg)
            corefree(msg);
    }

    virtual ~CSysException(){}
};

class CNoMemoryException : public CWBException
{
public:

    CNoMemoryException()
    {
#ifdef WINS
        m_Err = E_OUTOFMEMORY;
#else
        m_Err = ENOMEM;
#endif
    }
    virtual const wchar_t *GetMsg()
    {
        wchar_t Msg[64];
        return Get_transl(GetCurrTaskPtr(), gettext_noopL("Not enough memory"), Msg);
    }
};

#endif  // _EXCEPTIONS_H_

