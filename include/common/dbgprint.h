#ifndef _DBGPRINT_H_
#define _DBGPRINT_H_

#ifdef DEBUG

#ifdef WINS

inline void DbgSprint(char *fmt, ...)
{
    char buf[256];
    char fmtlf[128];
    lstrcpy(fmtlf, fmt);
    lstrcat(fmtlf, "\n");
    va_list arg = (va_list)((LPBYTE)&fmt + sizeof(LPSTR));
    wvsprintf(buf, fmtlf, arg);
    OutputDebugString(buf);
}

#define DbgPrint DbgSprint

inline void DbgSBox(char *fmt, ...)
{
    char buf[256];
    va_list arg = (va_list)((LPBYTE)&fmt + sizeof(LPSTR));
    wvsprintf(buf, fmt, arg);
    MessageBox(NULL, buf, "DbgBox", MB_OK | MB_ICONEXCLAMATION);
}

#define DbgBox DbgSBox

CFNC DllKernel BOOL   WINAPI Log_write(const char * text);

inline void DbgSLogWrite(char *fmt, ...)
{
    char buf[256];
    va_list arg = (va_list)((LPBYTE)&fmt + sizeof(LPSTR));
    wvsprintf(buf, fmt, arg);
#ifdef SRVR
    dbg_err(buf);
#else
    Log_write(buf);
#endif
}

#define DbgLogWrite DbgSLogWrite

inline void DbgSLogFile(char *fmt, ...)
{
    char buf[256];
    DWORD wrtn;
    va_list arg = (va_list)((LPBYTE)&fmt + sizeof(LPSTR));
    char fmtlf[80];
    lstrcpy(fmtlf, fmt);
    lstrcat(fmtlf, "\r\n");
    wvsprintf(buf, fmtlf, arg);
    char fName[MAX_PATH];
    GetTempPath(MAX_PATH, fName);
    char *p = fName + lstrlen(fName);
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(p, "WB%02d%02d.LOG", st.wDay, st.wMonth);
    FHANDLE hFile = CreateFile(fName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!FILE_HANDLE_VALID(hFile))
    {
        sprintf(buf, "CreateFile %08X\r\n", GetLastError());
        OutputDebugString(buf);
        return;
    }
    SetFilePointer(hFile, 0, 0, FILE_END);
    WriteFile(hFile, buf, lstrlen(buf), &wrtn, NULL);
    CloseHandle(hFile);
}

#define DbgLogFile DbgSLogFile

inline LPCSTR SInterf2Str(REFGUID Guid)
{
    char Key[MAX_PATH];
    static char Result[64];
    WCHAR wGuid[40];
    lstrcpy(Key, "Interface\\");
    StringFromGUID2(Guid, wGuid, sizeof(wGuid) / 2);
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)&wGuid, -1, Key + 10, sizeof(Key) - 10, NULL, NULL);
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, Key, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD Size = sizeof(Result);
        DWORD Err  = RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)Result, &Size);
        RegCloseKey(hKey);
        if (!Err)
            return(Result);
    }
    sprintf(Result, "%08X", Guid.Data1);
    return(Result);
}

#define Interf2Str SInterf2Str

inline char * CurDT()
{
    static char buf[24];
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(buf, "%02d.%02d.%04d %02d:%02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    return(buf);
}

#else  // !WINS

inline void DbgSPrint(char *fmt, ...)
{
    va_list arg = (va_list)((LPBYTE)&fmt + sizeof(LPSTR));
    vfprintf(stderr, fmt, arg);
}


#define DbgPrint(fmt, args...)\
	 DbgSPrint(__FILE__ ":%d " fmt "\n", __LINE__ , ## args)

#define DbgBox DbgPrint
#define DbgLogWrite DbgPrint
#define DbgLogFile DbgPrint

#endif
#else  // !DEBUG


inline void DbgPrint(char *fmt, ...) { }
inline void DbgBox(char *fmt, ...) { }
inline void DbgLogWrite(char *fmt, ...) { } 
inline void DbgLogFile(char *fmt, ...) { }
inline char * CurDT() { return NULL; }
//inline LPCSTR Interf2Str(REFGUID Guid) { return NULL; }

#endif
#endif  // _DBGPRINT_H_
