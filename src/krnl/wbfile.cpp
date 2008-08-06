void CWBFileX::Open(const char *fName, DWORD Mode)
{
    Close();
    m_Handle = CreateFileA(fName, Mode, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_Handle == INVALID_FHANDLE_VALUE) 
    {
        wchar_t Fmt[64];
        wchar_t wName[MAX_PATH];
        throw new CSysException(L"CreateFile", Get_transl(NULL, gettext_noopL("Cannot open file %ls"), Fmt), ToUnicode(fName, wName, sizeof(wName) / sizeof(wchar_t)));
    }
}

void CWBFileX::Create(const char *fName, bool Overwrite)
{
    Close();
    m_Handle = CreateFileA(fName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, Overwrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_Handle == INVALID_FHANDLE_VALUE) 
    {
        wchar_t Fmt[64];
        wchar_t wName[MAX_PATH];
        throw new CSysException(L"CreateFile", Get_transl(NULL, gettext_noopL("Cannot create file %ls"), Fmt), ToUnicode(fName, wName, sizeof(wName) / sizeof(wchar_t)));
    }
}

CWBFileTime GetModifTime(const char *filename)
{
    struct stat statbuf;
    return CWBFileTime((time_t)(stat(filename, &statbuf) == 0 ? statbuf.st_mtime : 0));
}

CWBFileTime GetAccessTime(const char *filename)
{
    struct stat statbuf;
    return CWBFileTime((time_t)(stat(filename, &statbuf) == 0 ? statbuf.st_atime : 0));
}

void SetFileTimes(const char *filename, CWBFileTime AccTime, CWBFileTime ModTime)
{
    utimbuf utb;
    utb.actime  = AccTime;
    utb.modtime = ModTime;
    utime(filename, &utb);
}

bool FileExists(const char *filename)
{
#ifdef WINS

    return GetFileAttributesA(filename) != (DWORD)-1;

#else

    struct stat statbuf;
    return stat(filename, &statbuf) == 0;

#endif
}

inline bool IsDirectory(const char *fName)
{
#ifdef WINS

    DWORD Attrs = GetFileAttributesA(fName);
    return (Attrs != 0xffffffff) && (Attrs & FILE_ATTRIBUTE_DIRECTORY);

#else

    struct stat statbuf;
    return (stat(fName, &statbuf) == 0) && S_ISDIR(statbuf.st_mode);

#endif
}

inline void SysErrorMsg(char *Buf, int Size)
{
#ifdef WINS

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), Buf, Size, NULL);

#else

    strncpy(Buf, strerror(errno), Size);
    Buf[Size - 1] = 0;

#endif
}
