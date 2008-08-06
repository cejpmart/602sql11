#ifndef _BUFFER_H_
#define _BUFFER_H_

//#ifdef WINS
//#define vsnprintf _vsnprintf
//#endif

class CBuffer
{
protected:

    void    *m_Ptr;
    unsigned m_Size;

public:

    CBuffer(){m_Ptr = NULL; m_Size = 0;}
    ~CBuffer(){if (m_Ptr) corefree(m_Ptr);}
    bool Alloc(unsigned Size)
    {
        if (Size > m_Size)
        {
            m_Ptr = corerealloc(m_Ptr, Size);
            if (!m_Ptr)
                return(false);
            m_Size = Size;
        }
        return(true);
    }
    void AllocX(unsigned Size)
    {
        if (Size > m_Size)
        {
            m_Ptr = corerealloc(m_Ptr, Size);
            if (!m_Ptr)
                throw new CNoMemoryException();
            m_Size = Size;
        }
    }

    const char *FormatV(const char *format, va_list argptr)
    {
        wchar_t *Msg = NULL;
        for (unsigned sz = 256; ; sz *= 2)
        {
            if (!Alloc(sz))
                break;
            int len = vsnprintf ((char *)m_Ptr, sz, format, argptr);
            if (len >= 0)
                break;
        }
        return (char *)m_Ptr;
    }
    const char *Format(const char *format, ...)
    {
        va_list va;
        va_start(va, format);
        return FormatV(format, va);
    }
    //void SetChar(int off, char Val)
    //{
    //    ((char *)m_Ptr)[off] = Val;
    //}
    unsigned GetSize(){return(m_Size);}
    operator void *(){return(m_Ptr);}
    operator uns8 *(){return((uns8 *)m_Ptr);}
    operator char *(){return((char *)m_Ptr);}
    operator const char *(){return((const char *)m_Ptr);}
    char& operator [](int i){return(*(((char *)m_Ptr) + i));}
};

class CWBuffer : public CBuffer
{
public:

    const wchar_t *Format(const wchar_t *format, ...)
    {
        wchar_t *Msg = NULL;
        for (unsigned sz = 256; ; sz *= 2)
        {
            if (!Alloc(sz * (unsigned)sizeof(wchar_t)))
                break;
            va_list va;
            va_start(va, format);
            size_t len = _vsnwprintf((wchar_t *)m_Ptr, sz, format, va);
            if (len >= 0 && len <= sz)
                break;
        }
        return (wchar_t *)m_Ptr;
    }

    operator wchar_t *(){return((wchar_t *)m_Ptr);}
    operator const wchar_t *(){return((const wchar_t *)m_Ptr);}
    wchar_t& operator [](size_t i){return(*(((wchar_t *)m_Ptr) + i));}
};
#endif  // _BUFFER_H_
