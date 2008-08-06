#define NETDB_USE_INTERNET

#ifdef SRVR
#include "opstr.h"
#else
#include <time.h>
#endif
#ifdef WINS
#include <initguid.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>  // struct timeval for select
#include <sys/ioctl.h>  // ioctl, FIONBIO
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#define lstrcpyn strncpy
#define lstrcmp strcmp
#define lstrcmpi strcasecmp
#ifdef  NETWARE
#include <direct.h>
#include <io.h>
#include <stream.h>
#include <advanced.h>
#else
#include <unistd.h>
#endif
#include "winrepl.h"
#endif
#include "dbgprint.h"
#include "wbmail.h"
#include "enumsrv.h"

#ifdef  WINS
#define KEY602SQL_MAIL "SOFTWARE\\Software602\\602SQL\\Mail"
#else
#define CONFIG_FILE "/etc/602sql"
#endif

#ifdef WINS
//
// MAPI
//
static LPMAPIALLOCATEBUFFER       lpMAPIAllocateBuffer;
static LPMAPIFREEBUFFER           lpMAPIFreeBuffer;
static LPOPENSTREAMONFILE         lpOpenStreamOnFile;
static LPMAPIUNINITIALIZE         lpMAPIUninitialize;

#ifdef MAIL602
//
// Mail602
//
static LPGETMAIL602USERNAMEFROMID lpGetUserNameFromID;
static LPADDADDRESSTOADDRESSLIST  lpAddAddr;
static LPADDFILETOFILELIST        lpAddFile;
static LPDISPOSEADDRESSLIST       lpFreeAList;
static LPDISPOSEFILELIST          lpFreeFList;
static LPSENDMAIL602MESSAGE       lpSend;
static LPSENDMAIL602MESSAGEEX     lpSendEx;
static LPGETMAILBOXSTATUSINFO     lpGetMailBoxStatusInfo;
static LPSENDQUEUEDINTERNETMSGS   lpSendQueuedInternetMsgs;

static LPGETLOCALUSERID           lpGetLocalUserID;
static LPGETMYUSERINFO            lpGetMyUserInfo;
static LPGETEXTERNALUSERLIST      lpGetExternalUserList;
static LPGETPRIVATELIST           lpGetPrivateList;
static LPGETPRIVATEUSERLIST       lpGetPrivateUserList;
static LPDISPOSELISTSLISTS        lpFreeListList;
static LPREADADRFILEX             lpReadADRFileX;
//static LPGETMSGLISTINQUEUE        lpGetMsgListInQueue;
//
// Mail602 Remote
//
static LPSPEEDUP                  lpSpeedUp;
static LPEXITGATE                 lpExitGate;
static LPFORCEREREAD              lpForceReread;
static LPSETWBTIMEOUT             lpSetWBTimeout;
//
// Mail602 prijem
//
static LPGETMSGLIST               lpGetMsgList;
static LPFREEMSGLIST              lpFreeMsgList;
static LPSIZEOFMSG                lpSizeOfMsg;
//static LPREADMSG                  lpReadMsg;
static LPREADLETTERFROMMSG        lpReadLetterFromMsg;
static LPGETFILELIST              lpGetFileList;
static LPFREEFILELIST             lpFreeFileList;
static LPCOPYFILEFROMMSG          lpCopyFileFromMsg;
static LPDELETEMSG                lpDeleteMsg;
static LPPOP3GETLISTOFMSGS        lpPOP3GetListOfMsgs;
static LPPOP3FREELISTOFMSGS       lpPOP3FreeListOfMsgs;
static LPPOP3GETORERASEMSGS       lpPOP3GetOrEraseMsgs;
static LPSENDGBATFROMRMMSGLIST    lpSendGBATFromRMMsgList;
#endif // MAIL602
#endif // WINS

#ifdef NETWARE
static NWGETSERVBYNAME    lpgetservbyname; 
static NETDBGETHOSTBYNAME lpNetDBgethostbyname;
static GETHOSTNAME        lpgethostname;
//NETDB_DEFINE_CONTEXT
#endif  // NETWARE

#ifdef WINS
CDialCtx   DialCtx;         // Kontext DialUp spojeni pro SMTP/POP3 a DirectIP
#ifdef MAIL602
CRemoteCtx RemoteCtx;       // Kontext spojeni pro SMTP/POP3 a Remote klienta Mail602
#endif
#endif

#ifdef SRVR
CWBMailSect WBMailSect;
#endif // SRVR

// this should be rewritten: use functions from baselib.cpp!
#ifdef WINS

int FromUnicode(const wuchar *Src, int SrcSz, char *Dst, int DstSz, wbcharset_t DstCharSet)
{
    return WideCharToMultiByte(DstCharSet.cp(), NULL, Src, SrcSz, Dst, DstSz, NULL, NULL);
}

int LangToLang(const char *Src, int SrcSz, char *Dst, int DstSz, wbcharset_t SrcCharSet, wbcharset_t DstCharSet)
{
    if (SrcSz == (size_t)-1)
        SrcSz = lstrlen(Src) + 1;
    CBuf Buf;
    if (!Buf.Alloc(SrcSz * sizeof(WCHAR)))
        return 0;
    int sz = MultiByteToWideChar(SrcCharSet.cp(), NULL, Src, SrcSz, Buf, SrcSz);
    sz     = WideCharToMultiByte(DstCharSet.cp(), NULL, Buf, sz,    Dst, DstSz, NULL, NULL);
    return sz;
}

int ToUnicode(const char *Src, int SrcSz, wuchar *Dst, int DstSz, wbcharset_t SrcCharSet)
{
    return MultiByteToWideChar(SrcCharSet.cp(), NULL, Src, SrcSz, Dst, DstSz);
}

#else   // LINUX

#include "iconv.h"

int LinuxConv(const char *Src, size_t SrcSz, char *Dst, size_t DstSz, const char *FromCharSet, const char *ToCharSet)
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
    return Result - Dst;
}

int FromUnicode(const wuchar *Src, int SrcSz, char *Dst, int DstSz, wbcharset_t DstCharSet)
{
    if (SrcSz == (size_t)-1)
        SrcSz = wuclen(Src) + 1;
    return LinuxConv((const char *)Src, SrcSz * sizeof(wuchar), Dst, DstSz, "UCS-2", DstCharSet);
}

int LangToLang(const char *Src, int SrcSz, char *Dst, int DstSz, wbcharset_t SrcCharSet, wbcharset_t DstCharSet)
{
    if (SrcSz == (size_t)-1)
        SrcSz = strlen(Src) + 1;
    char *Aux = NULL;
    if (Src == Dst)
    {
        Aux = (char *)malloc(DstSz);
        if (!Aux)
            return 0;
        Dst = Aux;
    }
    int Result = LinuxConv(Src, SrcSz, Dst, DstSz, SrcCharSet, DstCharSet);
    if (Result && Aux)
    {
        memcpy((void *)Src, Aux, Result);
        free(Aux);
    }
    return Result;
}

int ToUnicode(const char *Src, int SrcSz, wuchar *Dst, int DstSz, wbcharset_t SrcCharSet)
{
    if (SrcSz == -1)
        SrcSz = strlen(Src) + 1;
    return LinuxConv(Src, SrcSz, (char *)Dst, DstSz * sizeof(wuchar), SrcCharSet, "UCS-2");
}

#endif  // WINS
//
// DllExport Interface
//
// Obecna inicializacni funkce
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI InitWBMail(char *Profile, char *PassWord)
{
    return cd_InitWBMailEx(GetCurrTaskPtr(), Profile, PassWord, PassWord);
}

CFNC DllExport DWORD WINAPI InitWBMailEx(char *Profile, char *RecvPassWord, char *SendPassWord)
{
    return cd_InitWBMailEx(GetCurrTaskPtr(), Profile, RecvPassWord, SendPassWord);
}

#endif

DWORD cd_InitWBMail(cdp_t cdp, const char *Profile, const char *PassWord)
{
    return cd_InitWBMailEx(cdp, Profile, PassWord, PassWord);
}

DWORD cd_InitWBMailEx(cdp_t cdp, const char *Profile, const char *RecvPassWord, const char *SendPassWord)
{
    if (!Profile)
        Profile = "";
    //DbgLogFile("\r\n\r\nInitWBMail %s, %.20s", Profile ? Profile : "NULL", PassWord ? "*****" : "NULL");
#ifdef SRVR
    WBMailSect.Init();
    WBMailSect.Enter();
#endif
    DWORD Err;
    if (cdp->MailCtx)
    {
        CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
        if (MailCtx->IsOpen())
            Err = MAIL_ALREADY_INIT;
        else
            Err = MailCtx->Open(Profile, RecvPassWord, SendPassWord);
    }
    else
    {
        CWBMailCtx *MailCtx = new CWBMailCtx(cdp);
        if (!MailCtx)
        {
            Err = OUT_OF_APPL_MEMORY;
        }
        else
        {
            Err = MailCtx->Open(Profile, RecvPassWord, SendPassWord);
            if (Err)
                delete MailCtx;
            else
                cdp->MailCtx = MailCtx;
        }
    }
#ifdef SRVR
    WBMailSect.Leave();
    //DbgLogFile("InitWBMail %d", Err);
#else
    SetErr(cdp, Err);
    //DbgLogFile("InitWBMail Cli  %d", Err);
#endif
    return Err;
}

bool IsAscii(char *Src)
{
    if (!Src)
        return true;
    for (; *Src; Src++)
        if ((unsigned char)*Src >= 0x80)
            return false;
    return true;
}

#ifdef WINS

static HINSTANCE hInst;

char SMTPWndClass[] = "SMTPWndClass";
//
// Inicializace Mail602 Net, Lan nebo Remote
// (Kvuli kompatibilite se starsimi verzemi, nahrazena InitWBMail)
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI InitWBMail602(char *EmiPath, char *UserID, char *PassWord)
{
    return cd_InitWBMail602(GetCurrTaskPtr(), EmiPath, UserID, PassWord);
}

#endif

DWORD cd_InitWBMail602(cdp_t cdp, char *EmiPath, char *UserID, char *PassWord)
{
#ifdef MAIL602
    //DbgLogFile("\r\n\r\nInitWBMail602 %s, %s, %s", EmiPath ? EmiPath : "NULL", UserID ? UserID : "NULL", PassWord ? "*****" : "NULL");
#ifdef SRVR
    WBMailSect.Init();
    WBMailSect.Enter();
#endif
    DWORD Err;
    if (cdp->MailCtx)
    {
        Err = MAIL_ALREADY_INIT;
    }
    else
    {
        CWBMailCtx *MailCtx = new CWBMailCtx(cdp);
        if (!MailCtx)
        {
            Err = OUT_OF_APPL_MEMORY;
        }
        else
        {
            Err = MailCtx->Open602(EmiPath, UserID, PassWord);
            if (Err)
                delete MailCtx;
            else
                cdp->MailCtx = MailCtx;
        }
    }
#ifdef SRVR
    WBMailSect.Leave();
#else
    SetErr(cdp, Err);
#endif
    //DbgLogFile("InitWBMail602 %d, %d", Err);
    return Err;
#else // !defined(MAIL602)
    return MAIL_TYPE_INVALID;
#endif
}
//
// Inicializace Mail602 Net, Remote nebo SMTP/POP3
// (nejde pouzit pro jineho nez aktualniho uzivatele)
// (Kvuli kompatibilite se starsimi verzemi, nahrazena InitWBMail)
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI InitWBMail602x(char *Profile)
{
    return cd_InitWBMail602x(GetCurrTaskPtr(), Profile);
}

#endif

DWORD cd_InitWBMail602x(cdp_t cdp, char *Profile)
{
#ifdef MAIL602
    //DbgLogFile("\r\n\r\nInitWBMail602x %s", Profile ? Profile : "NULL");
#ifdef SRVR
    WBMailSect.Init();
    WBMailSect.Enter();
#endif
    DWORD Err;
    if (cdp->MailCtx)
    {
        Err = MAIL_ALREADY_INIT;
    }
    else
    {
        CWBMailCtx *MailCtx = new CWBMailCtx(cdp);
        if (!MailCtx)
        {
            Err = OUT_OF_APPL_MEMORY;
        }
        else
        {
            Err = MailCtx->Open602(NULL, NULL, NULL, Profile);
            if (Err)
                delete MailCtx;
            else
                cdp->MailCtx = MailCtx;
        }
    }
#ifdef SRVR
    WBMailSect.Leave();
#else
    SetErr(cdp, Err);
#endif
    //DbgLogFile("InitWBMail602x %d, %d", Err);
    return Err;
#else // !defined(MAIL602)
    return MAIL_TYPE_INVALID;
#endif
}

#endif // WINS
//
// Uvolneni prostredku posty
//
#ifndef SRVR

CFNC DllExport void WINAPI CloseWBMail()
{
    cd_CloseWBMail(GetCurrTaskPtr());
}

#endif

void cd_CloseWBMail(cdp_t cdp)
{
    //DbgLogFile("CloseWBMail %d");
#ifdef SRVR
    WBMailSect.Enter();
#endif

    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
        delete MailCtx;
#ifdef SRVR
    WBMailSect.Leave();
    WBMailSect.Delete();
#endif
}
//
// Vytvoreni zasilky
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI LetterCreate(char *Subj, char *Msg, UINT Flags, HANDLE *lpLetter)
{
    return cd_LetterCreate(GetCurrTaskPtr(), Subj, Msg, Flags, lpLetter);
}

CFNC DllExport DWORD WINAPI LetterCreateW(const wuchar *Subj, const wuchar *Msg, UINT Flags, HANDLE *lpLetter)
{
    return cd_LetterCreateW(GetCurrTaskPtr(), Subj, Msg, Flags, lpLetter);
}

#endif

DWORD cd_LetterCreate(cdp_t cdp, char *Subj, char *Msg, UINT Flags, HANDLE *lpLetter)
{
    DWORD Err;
    //DbgLogFile("LetterCreate");
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (!MailCtx)
    {
        Err = MAIL_NOT_INITIALIZED;
        goto exit;
    }
    //DbgLogFile("\r\nLetterCreate %s, %s, %04X", Subj ? Subj : "NULL", Msg ? "Msg" : "NULL", Flags);
    if (IsBadWptr((LPVOID)lpLetter, sizeof(LPDWORD)))
    {
        Err = ERROR_IN_FUNCTION_ARG;
        goto exit;
    }
    // IF SMTP Posta a neni zadan SMTP server, chyba
    if ((MailCtx->m_Type & (WBMT_SMTPOP3 | WBMT_602SP3))  && !*MailCtx->m_SMTPServer)
    {
        Err = MAIL_PROFSTR_NOTFND;
        goto exit;
    }
    *lpLetter = 0;

    CWBLetter *pLttr;
    pLttr = new CWBLetter(MailCtx);
    if (!pLttr)
    {
        Err = OUT_OF_APPL_MEMORY;
        goto exit;
    }

    Err = pLttr->Create(Subj, Msg, Flags);
    if (Err)
        delete pLttr;
    else
        *lpLetter = (HANDLE)pLttr;

exit:

    SetErr(cdp, Err);
    //DbgLogFile("LetterCreate %d", Err);
    return Err;
}

DWORD cd_LetterCreateW(cdp_t cdp, const wuchar *Subj, const wuchar *Msg, UINT Flags, HANDLE *lpLetter)
{
    return cd_LetterCreate(cdp, (char *)Subj, (char *)Msg, Flags | WBL_UNICODESB, lpLetter);
}
//
// Pridani adresy
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI LetterAddAddr(HANDLE Letter, char *Addr, char *Type, BOOL CC)
{
    return cd_LetterAddAddr(GetCurrTaskPtr(), Letter, Addr, Type, CC);
}

CFNC DllExport DWORD WINAPI LetterAddAddrW(HANDLE Letter, const wuchar *Addr, const wuchar *Type, BOOL CC)
{
    return cd_LetterAddAddrW(GetCurrTaskPtr(), Letter, Addr, Type, CC);
}

#endif

DWORD cd_LetterAddAddr(cdp_t cdp, HANDLE Letter, char *Addr, char *Type, BOOL CC)
{
    //DbgLogFile("LetterAddAddr %s, %s, %s", Addr ? Addr : "NULL", Type ? Type : "NULL", CC ? "CC" : "");
    //DbgLogFile("LetterAddAddr");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if 
        (
            MailCtx->IsValid((CWBLetter *)Letter) &&
            !IsBadRptr((LPVOID)Addr,   64) && *Addr &&
            (Type == NULL || !IsBadRptr((LPVOID)Type,   4))
        )
            Err = ((CWBLetter *)(Letter))->AddAddr(Addr, Type, CC);
    }
    SetErr(cdp, Err);
    //DbgLogFile("LetterAddAddr %d", Err);
    return Err;
}

DWORD cd_LetterAddAddrW(cdp_t cdp, HANDLE Letter, const wuchar *wAddr, const wuchar *wType, BOOL CC)
{
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if 
        (
            MailCtx->IsValid((CWBLetter *)Letter) &&
            !IsBadRptr((LPVOID)wAddr,   64) && *wAddr &&
            (wType == NULL || !IsBadRptr((LPVOID)wType,   4))
        )
        {
            char Addr[80];
            char Type[32];
            char *pType = NULL;
            int sz = FromUnicode(wAddr, -1, Addr, sizeof(Addr), wbcharset_t(0));
            if (sz)
            {
                if (wType && *wType)
                {
                    sz    = FromUnicode(wType, -1, Type, sizeof(Type), wbcharset_t(0));
                    pType = Type;
                }
            }
            if (sz)
                Err = ((CWBLetter *)(Letter))->AddAddr(Addr, pType, CC);
        }
    }
    SetErr(cdp, Err);
    return Err;
}
//
// Pridani souboru z disku
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI LetterAddFile(HANDLE Letter, char *fName)
{
    return cd_LetterAddFile(GetCurrTaskPtr(), Letter, fName);
}

CFNC DllExport DWORD WINAPI LetterAddFileW(HANDLE Letter, const wuchar *fName)
{
    return cd_LetterAddFileW(GetCurrTaskPtr(), Letter, fName);
}

#endif

DWORD cd_LetterAddFile(cdp_t cdp, HANDLE Letter, char *fName)
{
    //DbgLogFile("AddFile %s", fName ? fName : "NULL");
    //DbgLogFile("LetterAddFile");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if (MailCtx->IsValid((CWBLetter *)Letter) && !IsBadRptr((LPVOID)fName, 12))
            Err = ((CWBLetter *)Letter)->AddFile(fName, false);
    }
    SetErr(cdp, Err);
    //DbgLogFile("LetterAddFile %d", Err);
    return Err;
}

DWORD cd_LetterAddFileW(cdp_t cdp, HANDLE Letter, const wuchar *fName)
{
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if (MailCtx->IsValid((CWBLetter *)Letter) && !IsBadRptr((LPVOID)fName, 12))
            Err = ((CWBLetter *)Letter)->AddFile((const char *)fName, true);
    }
    SetErr(cdp, Err);
    return Err;
}
//
// Pridani souboru z databaze
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI LetterAddBLOBs(HANDLE Letter, char *fName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond)
{
    return cd_LetterAddBLOBs(GetCurrTaskPtr(), Letter, fName, Table, Attr, Cond);
}

CFNC DllExport DWORD WINAPI LetterAddBLOBsW(HANDLE Letter, const wuchar *fName, const wuchar *Table, const wuchar *Attr, const wuchar *Cond)
{
    return cd_LetterAddBLOBsW(GetCurrTaskPtr(), Letter, fName, Table, Attr, Cond);
}

#endif

DWORD cd_LetterAddBLOBs(cdp_t cdp, HANDLE Letter, char *fName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond)
{
    //DbgLogFile("AddFile %s", fName ? fName : "NULL");
    //DbgLogFile("AddBLOBs");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if (MailCtx->IsValid((CWBLetter *)Letter) && !IsBadRptr((LPVOID)fName, 12) && !IsBadRptr((LPVOID)Table, 4) && !IsBadRptr((LPVOID)Attr, 4) && !IsBadRptr((LPVOID)Cond, 12))
            Err = ((CWBLetter *)Letter)->AddBLOBs(Table, Attr, Cond, fName, false);
    }
    SetErr(cdp, Err);
    //DbgLogFile("AddBLOBs %d", Err);
    return Err;
}

DWORD cd_LetterAddBLOBsW(cdp_t cdp, HANDLE Letter, const wuchar *fName, const wuchar *Table, const wuchar *Attr, const wuchar *Cond)
{
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if (MailCtx->IsValid((CWBLetter *)Letter) && !IsBadRptr((LPVOID)fName, 12) && !IsBadRptr((LPVOID)Table, 4) && !IsBadRptr((LPVOID)Attr, 4) && !IsBadRptr((LPVOID)Cond, 12))
        {
            CBuf aTable;
            CBuf aAttr;
            CBuf aCond;
            int tLen = (int)wuclen(Table) + 1;
            int aLen = (int)wuclen(Attr) + 1;
            int cLen = (int)wuclen(Cond) + 1;
            if (!aTable.Alloc(tLen) || !aAttr.Alloc(aLen) || !aCond.Alloc(cLen))
                Err = OUT_OF_APPL_MEMORY;
            else
            {
#ifdef SRVR
                wbcharset_t charset = sys_spec.charset;
#else
                wbcharset_t charset = cdp->sys_charset;
#endif
                tLen = FromUnicode(Table, tLen, aTable, tLen, charset);
                aLen = FromUnicode(Attr,  aLen, aAttr,  aLen, charset);
                cLen = FromUnicode(Cond,  cLen, aCond,  cLen, charset);
                if (tLen && aLen && cLen)
                    Err = ((CWBLetter *)Letter)->AddBLOBs(aTable, aAttr, aCond, fName, true);
            }
        }
    }
    SetErr(cdp, Err);
    //DbgLogFile("AddBLOBs %d", Err);
    return Err;
}
//
// Pridani souboru z databaze
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI LetterAddBLOBr(HANDLE Letter, char *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index)
{
    return cd_LetterAddBLOBr(GetCurrTaskPtr(), Letter, fName, Table, Pos, Attr, Index);
}

CFNC DllExport DWORD WINAPI LetterAddBLOBrW(HANDLE Letter, const wuchar *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index)
{
    return cd_LetterAddBLOBrW(GetCurrTaskPtr(), Letter, fName, Table, Pos, Attr, Index);
}

#endif

DWORD cd_LetterAddBLOBr(cdp_t cdp, HANDLE Letter, char *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index)
{
    //DbgLogFile("AddFile %s", fName ? fName : "NULL");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if (MailCtx->IsValid((CWBLetter *)Letter) && !IsBadRptr((LPVOID)fName, 12))
            Err = ((CWBLetter *)Letter)->AddBLOBr(Table, Pos, Attr, Index, fName, false);
    }
    SetErr(cdp, Err);
    //DbgLogFile("AddBLOBr %d", Err);
    return Err;
}

DWORD cd_LetterAddBLOBrW(cdp_t cdp, HANDLE Letter, const wuchar *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index)
{
    //DbgLogFile("AddFile %s", fName ? fName : "NULL");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if (MailCtx->IsValid((CWBLetter *)Letter) && !IsBadRptr((LPVOID)fName, 12))
            Err = ((CWBLetter *)Letter)->AddBLOBr(Table, Pos, Attr, Index, fName, true);
    }
    SetErr(cdp, Err);
    //DbgLogFile("AddBLOBr %d", Err);
    return Err;
}
//
// Odeslani zasilky
// 

#ifndef SRVR

CFNC DllExport DWORD WINAPI LetterSend(HANDLE Letter)
{
    return cd_LetterSend(GetCurrTaskPtr(), Letter);
}

#endif

DWORD cd_LetterSend(cdp_t cdp, HANDLE Letter)
{
    //DbgLogFile("LetterSend");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        Err = ERROR_IN_FUNCTION_ARG; 
        if (MailCtx->IsValid((CWBLetter *)Letter))
            Err = ((CWBLetter *)Letter)->Send();
    }
    SetErr(cdp, Err);
    //DbgLogFile("LetterSend %d", Err);
    return Err;
}
//
// Odeslani zasilky z lokalniho na vzdaleny urad 
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI TakeMailToRemOffice()
{
    return cd_TakeMailToRemOffice(GetCurrTaskPtr());
}

#endif

DWORD cd_TakeMailToRemOffice(cdp_t cdp)
{
#ifdef MAIL602
#ifdef WINS

    //DbgLogFile("TakeMailToRemOffice");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!(MailCtx->m_Type & (WBMT_602REM | WBMT_602SP3)))
            Err = MAIL_NOT_REMOTE;
        else if (*MailCtx->m_SMTPServer)
            Err = MailCtx->SendSMTP602();
        else
            Err = MailCtx->SendRemote();
    }
    SetErr(cdp, Err);
    //DbgLogFile("TakeMailToRemOffice %d", Err);
    return Err;

#else

    return NOERROR;

#endif
#else
    return MAIL_TYPE_INVALID;
#endif
}
//
// Zruseni zasilky
//
#ifndef SRVR

CFNC DllExport void WINAPI LetterCancel(HANDLE Letter)
{
    cd_LetterCancel(GetCurrTaskPtr(), Letter);
}

#endif

void cd_LetterCancel(cdp_t cdp, HANDLE Letter)
{
    //DbgLogFile("LetterCancel");
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (MailCtx->IsValid((CWBLetter *)Letter))
            delete (CWBLetter *)Letter;
    }
}
//
// Otevreni postovni schranky
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailOpenInBox(HANDLE *lpMailBox)
{
    return cd_MailOpenInBox(GetCurrTaskPtr(), lpMailBox);
}

#endif

DWORD cd_MailOpenInBox(cdp_t cdp, HANDLE *lpMailBox)
{
    //DbgLogFile("\r\nMailOpenInBox");
    DWORD Err = NOERROR;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (!MailCtx)
    {
        Err = MAIL_NOT_INITIALIZED;
        goto exit;
    }
    if (IsBadWptr((LPVOID)lpMailBox, sizeof(DWORD)))
    {
        Err = ERROR_IN_FUNCTION_ARG;
        goto exit;
    }
    *lpMailBox = 0;
    if (MailCtx->m_MailBox)
    {
        Err = MAIL_BOX_LOCKED;
        goto exit;
    }

#ifdef WINS
#ifdef MAIL602    
    if (MailCtx->m_hMail602)
        MailCtx->m_MailBox = new CWBMailBox602(MailCtx);
#endif        
    else if (MailCtx->m_lpSession)
        MailCtx->m_MailBox = new CWBMailBoxMAPI(MailCtx);
    else if (*MailCtx->m_POP3Server)
        MailCtx->m_MailBox = new CWBMailBoxPOP3s(MailCtx);

#else

    if (*MailCtx->m_POP3Server)
    {
        WIN32_FIND_DATA fd;
        HANDLE ff = FindFirstFile(MailCtx->m_POP3Server, &fd);
        if (ff != INVALID_HANDLE_VALUE)
        {
            MailCtx->m_MailBox = new CWBMailBoxPOP3f(MailCtx);
            FindClose(ff);
            MailCtx->m_Type |= WBMT_DIRPOP3;
        }
        else
        {
            MailCtx->m_MailBox = new CWBMailBoxPOP3s(MailCtx);
        }
    }
    
#endif // WINS
        
    else if (*MailCtx->m_SMTPServer)
        Err = MAIL_PROFSTR_NOTFND;
    else
        Err = MAIL_NOT_INITIALIZED;

    if (Err)
        goto exit;
    if (!MailCtx->m_MailBox)
    {
        Err = OUT_OF_APPL_MEMORY;
        goto exit;
    }
    Err = MailCtx->m_MailBox->OpenInBox(cdp);
    if (Err)
        delete MailCtx->m_MailBox;

    *lpMailBox = MailCtx->m_MailBox;

exit:

    SetErr(cdp, Err);
    //DbgLogFile("MailOpenInBox %d", Err);
    return Err;
}
//
// Nacteni seznamu zasilek
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailBoxLoad(HANDLE MailBox, UINT Flags)
{
    return cd_MailBoxLoad(GetCurrTaskPtr(), MailBox, Flags);
}

#endif

DWORD cd_MailBoxLoad(cdp_t cdp, HANDLE MailBox, UINT Flags)
{
    //DbgLogFile("MailBoxLoad(%04X)", Flags);
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!MailBox || MailBox != MailCtx->m_MailBox)
            Err = ERROR_IN_FUNCTION_ARG;
        else
            Err = MailCtx->m_MailBox->LoadList(Flags);
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailBoxLoad = %d", Err);
    return Err;
}
//
// Nacteni textu dopisu
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailBoxGetMsg(HANDLE MailBox, DWORD MsgID)
{
    return cd_MailBoxGetMsg(GetCurrTaskPtr(), MailBox, MsgID, 0);
}

CFNC DllExport DWORD WINAPI MailBoxGetMsgEx(HANDLE MailBox, DWORD MsgID, DWORD Flags)
{
    return cd_MailBoxGetMsg(GetCurrTaskPtr(), MailBox, MsgID, Flags);
}

#endif

DWORD cd_MailBoxGetMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD Flags)
{
    //DbgLogFile("MailBoxGetMsg %d", MsgID);
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!MailBox || MailBox != (HANDLE)MailCtx->m_MailBox)
            Err = ERROR_IN_FUNCTION_ARG;
        else
            Err = MailCtx->m_MailBox->GetMsg(MsgID, Flags & (MBL_BODY | MBL_HDRONLY | MBL_MSGONLY));
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailBoxGetMsg %d", Err);
    return Err;
}
//
// Nacteni seznamu pripojenych souboru
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailBoxGetFilInfo(HANDLE MailBox, DWORD MsgID)
{
    return cd_MailBoxGetFilInfo(GetCurrTaskPtr(), MailBox, MsgID);
}

#endif

DWORD cd_MailBoxGetFilInfo(cdp_t cdp, HANDLE MailBox, DWORD MsgID)
{
    //DbgLogFile("MailBoxGetFilInfo %d", MsgID);
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!MailBox || MailBox != (HANDLE)MailCtx->m_MailBox)
            Err = ERROR_IN_FUNCTION_ARG;
        else
            Err = MailCtx->m_MailBox->GetFilesInfo(MsgID);
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailBoxGetFilInfo %d", Err);
    return Err;
}
//
// Ulozeni pripojeneho souboru na disk
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailBoxSaveFileAs(HANDLE MailBox, DWORD MsgID, DWORD FilIdx, char *FilName, char *DstPath)
{
    return cd_MailBoxSaveFileAs(GetCurrTaskPtr(), MailBox, MsgID, FilIdx, FilName, DstPath);
}

#endif

DWORD cd_MailBoxSaveFileAs(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, char *DstPath)
{
    //DbgLogFile("MailBoxSaveFileAs %d, %d, %s, %s", MsgID, FilIdx, FilName ? FilName : "NULL", DstPath ? DstPath : "NULL");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!MailBox || MailBox != (HANDLE)MailCtx->m_MailBox)
            Err = ERROR_IN_FUNCTION_ARG;
        else
            Err = MailCtx->m_MailBox->SaveFileAs(MsgID, FilIdx, FilName, DstPath);
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailBoxSaveFileAs %d", Err);
    return Err;
}
//
// Ulozeni pripojeneho souboru do databaze
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailBoxSaveFileDBs(HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond)
{
    return cd_MailBoxSaveFileDBs(GetCurrTaskPtr(), MailBox, MsgID, FilIdx, FilName, Table, Attr, Cond);
}

#endif

DWORD cd_MailBoxSaveFileDBs(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond)
{
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!MailBox || MailBox != (HANDLE)MailCtx->m_MailBox || IsBadRptr((LPVOID)Table, 4) || IsBadRptr((LPVOID)Attr, 4) || IsBadRptr((LPVOID)Cond, 12))
            Err = ERROR_IN_FUNCTION_ARG;
        else
            Err = MailCtx->m_MailBox->SaveFileToDBs(MsgID, FilIdx, FilName, Table, Attr, Cond);
    }
    SetErr(cdp, Err);
    return Err;
}
//
// Ulozeni pripojeneho souboru do databaze
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailBoxSaveFileDBr(HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index)
{
    return cd_MailBoxSaveFileDBr(GetCurrTaskPtr(), MailBox, MsgID, FilIdx, FilName, Table, Pos, Attr, Index);
}

#endif

DWORD cd_MailBoxSaveFileDBr(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index)
{
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!MailBox || MailBox != (HANDLE)MailCtx->m_MailBox)
            Err = ERROR_IN_FUNCTION_ARG;
        else
            Err = MailCtx->m_MailBox->SaveFileToDBr(MsgID, FilIdx, FilName, Table, Pos, Attr, Index);
    }
    SetErr(cdp, Err);
    return Err;
}
//
// Zruseni zasilky
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailBoxDeleteMsg(HANDLE MailBox, DWORD MsgID, BOOL RecToo)
{
    return cd_MailBoxDeleteMsg(GetCurrTaskPtr(), MailBox, MsgID, RecToo);
}

#endif

DWORD cd_MailBoxDeleteMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID, BOOL RecToo)
{
    //DbgLogFile("MailBoxDeleteMsg %d %s", MsgID, RecToo ? "RecToo" : "");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!MailBox || MailBox != (HANDLE)MailCtx->m_MailBox)
            Err = ERROR_IN_FUNCTION_ARG;
        else
            Err = MailCtx->m_MailBox->DeleteMsg(MsgID, RecToo);
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailBoxDeleteMsg %d", Err);
    return Err;
}
//
// Vraci info o tabulkach pro doslou postu
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailGetInBoxInfo(HANDLE MailBox, char *mTblName, ttablenum *mTblNum, char *fTblName, ttablenum *fTblNum)
{
    return cd_MailGetInBoxInfo(GetCurrTaskPtr(), MailBox, mTblName, mTblNum, fTblName, fTblNum);
}

#endif

DWORD cd_MailGetInBoxInfo(cdp_t cdp, HANDLE MailBox, char *mTblName, ttablenum *mTblNum, char *fTblName, ttablenum *fTblNum)
{
    //DbgLogFile("MailGetInBoxInfo");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!MailBox || MailBox != (HANDLE)MailCtx->m_MailBox)
            Err = ERROR_IN_FUNCTION_ARG;
        else
#ifdef WINS
            if 
            (
                (mTblName && IsBadWritePtr(mTblName, 64)) ||
                (mTblNum  && IsBadWritePtr(mTblNum,   2)) ||
                (fTblName && IsBadWritePtr(fTblName, 64)) ||
                (fTblNum  && IsBadWritePtr(fTblNum,   2))
            )
                Err = ERROR_IN_FUNCTION_ARG;
            else
#endif
            MailCtx->m_MailBox->GetInfo(mTblName, mTblNum, fTblName, fTblNum);
    }
    SetErr(cdp, Err);
    return Err;
}
//
// Vytvoreni tabulek pro postovni schranku
//
DWORD cd_MailCreInBoxTables(cdp_t cdp, char *Profile)
{
    tobjname Appl, InBoxTbl, InBoxTbf;
    *Appl     = 0;
    *InBoxTbl = 0;
    *InBoxTbf = 0;

    if (Profile && *Profile)
    {
#ifdef WINS
        HKEY hKey;
        char Key[128] = KEY602SQL_MAIL;  
        lstrcat(Key, "\\");
        lstrcat(Key, Profile);
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0, KEY_READ_EX, &hKey) == NOERROR)
        {
            DWORD Size = sizeof(Appl);
            RegQueryValueEx(hKey, "InBoxAppl",     NULL, NULL, (LPBYTE)Appl,     &Size);
            Size = sizeof(InBoxTbl);
            RegQueryValueEx(hKey, "InBoxMessages", NULL, NULL, (LPBYTE)InBoxTbl, &Size);
            Size = sizeof(InBoxTbf);
            RegQueryValueEx(hKey, "InBoxFiles",    NULL, NULL, (LPBYTE)InBoxTbf, &Size);
            RegCloseKey(hKey);
        }
#else

        extern const char *configfile;
        char ProfSect[70];

        strcpy(ProfSect, ".MAIL_");  
        strcat(ProfSect, Profile);

        GetPrivateProfileString(ProfSect, "InBoxAppl",     "", Appl,     sizeof(Appl),     configfile);
        GetPrivateProfileString(ProfSect, "InBoxMessages", "", InBoxTbl, sizeof(InBoxTbl), configfile);
        GetPrivateProfileString(ProfSect, "InBoxFiles",    "", InBoxTbf, sizeof(InBoxTbf), configfile);

#endif // WINS

    }

    ttablenum InBoxL, InBoxF;

#ifdef SRVR

    table_descr *InBoxLDescr, *InBoxFDescr;
    return CWBMailBox::GetTables(cdp, Appl, InBoxTbl, &InBoxL, &InBoxLDescr, InBoxTbf, &InBoxF, &InBoxFDescr);

#else

    return CWBMailBox::GetTables(cdp, Appl, InBoxTbl, &InBoxL, InBoxTbf, &InBoxF);

#endif

}
//
// Uvolneni prostredku postovni schranky
//
#ifndef SRVR

CFNC DllExport void WINAPI MailCloseInBox(HANDLE MailBox)
{
    cd_MailCloseInBox(GetCurrTaskPtr(), MailBox);
}

#endif

void cd_MailCloseInBox(cdp_t cdp, HANDLE MailBox)
{
    //DbgLogFile("MailCloseInBox");
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (MailBox && MailBox == (HANDLE)MailCtx->m_MailBox)
        {
            delete MailCtx->m_MailBox;
            MailCtx->m_MailBox = NULL;
        }
    }
}

//
// Navazani Dial komunikace 
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailDial(char *PassWord)
{
    return cd_MailDial(GetCurrTaskPtr(), PassWord);
}

#endif

DWORD cd_MailDial(cdp_t cdp, char *PassWord)
{
#ifdef WINS

    //DbgLogFile("MailDial %s", PassWord ? "*****" : "NULL");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!(MailCtx->m_Type & (WBMT_602REM | WBMT_602SP3 | WBMT_SMTPOP3)))
            Err = MAIL_NOT_REMOTE;
        else if (*MailCtx->m_SMTPServer)
            Err = MailCtx->DialSMTP(PassWord);
#ifdef MAIL602
        else
            Err = RemoteCtx.Dial(MailCtx->m_EMI, MailCtx->m_hMail602, 15);
#endif
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailDial %d", Err);
    return Err;

#else
  
    return NOERROR;

#endif
}
//
// Ukonceni Dial komunikace
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailHangUp()
{
    return cd_MailHangUp(GetCurrTaskPtr());
}

#endif

DWORD cd_MailHangUp(cdp_t cdp)
{
#ifdef WINS

    //DbgLogFile("MailHangUp");
    DWORD Err = MAIL_NOT_INITIALIZED;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
    {
        if (!(MailCtx->m_Type & (WBMT_602REM | WBMT_602SP3 | WBMT_SMTPOP3)))
            Err = MAIL_NOT_REMOTE;
        else if (*MailCtx->m_DialConn)
            Err = MailCtx->HangUpSMTP();
#ifdef MAIL602
        else
            RemoteCtx.HangUp();
#endif
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailHangUp %d", Err);
    return Err;

#else

    return NOERROR;

#endif
}
//
// Vraci typ posty
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailGetType()
{
    return cd_MailGetType(GetCurrTaskPtr());
}

#endif

DWORD cd_MailGetType(cdp_t cdp)
{
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx)
        return MailCtx->m_Type;
    else
        return 0;
}
//
// Sprava profilu
//
// Umoznuje programove vytvaret, menit a rusit postovni profily. Docasny profil je pro jedno pouziti, 
// nikam se persistentne neuklada, property se zapisuji primo do kontextu, zanikne po zavolani CloseWBMail.
//
// Vytvoreni profilu
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailCreateProfile(const char *ProfileName, BOOL Temp)
{
    return cd_MailCreateProfile(GetCurrTaskPtr(), ProfileName, Temp);
}

#endif

DWORD cd_MailCreateProfile(cdp_t cdp, const char *ProfileName, BOOL Temp)
{
    //DbgLogFile("MailCreateProfile");
    if (!ProfileName || lstrlen(ProfileName) > 64)
        return ERROR_IN_FUNCTION_ARG;

    DWORD Err = NOERROR;
    if (!Temp)
    {
        CWBMailProfile mp(cdp, FALSE);
        Err = mp.Create(ProfileName);
    }
    else
    {
        // IF neexistuje permanentni profil
        Err = CWBMailProfile::Exists(ProfileName);
        if (!Err)
        {
            // IF existuje neotevreny MailCtx, vycistit parametry 
            CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
            if (MailCtx)
            {
            
                if (MailCtx->IsOpen())
                    Err = MAIL_ALREADY_INIT;
                else
                    MailCtx->CWBMailProfile::Reset();
            }
            // ELSE vytvorit novy MailCtx
            else
            {
                MailCtx = new CWBMailCtx(cdp, true);
                if (!MailCtx)
                    Err = OUT_OF_APPL_MEMORY;
            }
            if (!Err)
            {
                lstrcpy(MailCtx->m_Profile, ProfileName);
                if (!MailCtx->FilePath())
                {
                    Err = OUT_OF_APPL_MEMORY;
                    delete MailCtx;
                    MailCtx = NULL;
                }
                cdp->MailCtx = MailCtx;
            }
        }
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailCreateProfile %d", Err);
    return Err;
}
//
// Zruseni profilu
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailDeleteProfile(const char *ProfileName)
{
    return cd_MailDeleteProfile(GetCurrTaskPtr(), ProfileName);
}

#endif

DWORD cd_MailDeleteProfile(cdp_t cdp, const char *ProfileName)
{
    DWORD Err = NOERROR;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    // IF temporary profile
    if (MailCtx && MailCtx->IsTemp(ProfileName))
    {
        // IF context is open, error
        if (MailCtx->IsOpen())
        {
            Err = MAIL_ALREADY_INIT;
        }
        // ELSE delete context
        else
        {
            delete MailCtx;
            cdp->MailCtx = NULL;
        }
    }
    // ELSE delete profile in registry/inifile
    else
    {
        CWBMailProfile mp(cdp, FALSE);
        Err = mp.Delete(ProfileName);
    }
    SetErr(cdp, Err);
    return Err;
}
//
// Zapis property
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailSetProfileProp(const char *ProfileName, const char *PropName, const char *PropValue)
{
    return cd_MailSetProfileProp(GetCurrTaskPtr(), ProfileName, PropName, PropValue);
}

#endif

DWORD cd_MailSetProfileProp(cdp_t cdp, const char *ProfileName, const char *PropName, const char *PropValue)
{
    DWORD Err = NOERROR;
    //DbgLogFile("MailSetProfileProp");
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx && MailCtx->IsTemp(ProfileName))
    {
        if (MailCtx->IsOpen())
            Err = MAIL_ALREADY_INIT;
        else
            Err = MailCtx->SetTempProp(PropName, PropValue);
    }
    else
    {
        CWBMailProfile mp(cdp, FALSE);
        Err = mp.SetProp(ProfileName, PropName, PropValue);
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailSetProfileProp %s=%s", PropName, PropValue);
    return Err;
}
//
// Cteni property
//
#ifndef SRVR

CFNC DllExport DWORD WINAPI MailGetProfileProp(const char *ProfileName, const char *PropName, char *PropValue, int ValSize)
{
    return cd_MailGetProfileProp(GetCurrTaskPtr(), ProfileName, PropName, PropValue, ValSize);
}

#endif

DWORD cd_MailGetProfileProp(cdp_t cdp, const char *ProfileName, const char *PropName, char *PropValue, int ValSize)
{
    DWORD Err = NOERROR;
    CWBMailCtx *MailCtx = (CWBMailCtx *)cdp->MailCtx;
    if (MailCtx && MailCtx->IsTemp(ProfileName))
    {
        Err = MailCtx->GetTempProp(PropName, PropValue, ValSize);
    }
    else
    {
        CWBMailProfile mp(cdp, FALSE);
        Err = mp.GetProp(ProfileName, PropName, PropValue, ValSize);
    }
    SetErr(cdp, Err);
    //DbgLogFile("MailGetProfileProp %d", Err);
    return Err;
}
//
// Kontext posty - predpoklada se, ze kazde vlakno vlakno muze postovat v jednom okamziku pouze v jednom kontextu
// t.j. muze existovat maximalne jeden kontext.
//
CWBMailCtx::CWBMailCtx(cdp_t cdp, bool TempProf) : CWBMailProfile(cdp, TempProf), m_charset(0)
{
    memset((char *)this + sizeof(CWBMailProfile), 0, sizeof(CWBMailCtx) - sizeof(CWBMailProfile));
}

CWBMailCtx::~CWBMailCtx()
{
#ifdef WINS

    if (m_lpSession)
        CloseMAPI();
#ifdef MAIL602
    if (m_hMail602)
        Close602();
#endif
#endif

    Close();
    m_cdp->MailCtx = NULL;
}
//
// Inicializace posty
//
// Pokud existuje v profilu klic ProfileMAPI, imicializuje MAPI.
// Pokud existuje klic Profile602 nebo EmiPath, inicializuje Mail602.
// Jinak nacte parametry SMTP/POP3.
// Na Novelu a Unixu nacte parametry SMTP/POP3.
//
DWORD CWBMailCtx::Open(const char *WBProfile, const char *RecvPassWord, const char *SendPassWord)
{
    if (lstrlen(WBProfile) > 64 || (RecvPassWord && lstrlen(RecvPassWord) > 31) || (SendPassWord && lstrlen(SendPassWord) > 31))
        return ERROR_IN_FUNCTION_ARG;

    DWORD Err = OpenProfile(WBProfile);
    if (Err)
        goto  exit;
    //DbgLogFile("OpenProfile = %d", Err);

#ifdef WINS

    char Profile[64];
    char EmiPath[MAX_PATH];

    hInst = GetModuleHandle(NULL);  // hInst potrebuji nektere funkce Mail602
    // Nacist hesla z profilu
    char pw[96];
    memset(pw, 0, sizeof(pw));
    if (CWBMailProfile::IsTemp())
        memcpy(pw, m_PassWord, sizeof(pw));
    else
    {
        Err = GetProfileString("Options", pw, sizeof(pw));
        if (Err && Err != MAIL_PROFSTR_NOTFND)
            goto exit;
        if (!Err)
        {
            pw[0] ^= pw[17];    // Odsifrovat delku hesla k mailu
            if (pw[0] > 30)     // Ochrana proti preteceni
            {
                pw[0]  = 30;
                pw[31] = 0;
            }
            pw[32] ^= pw[43];   // Odsifrovat delku hesla k DialUp
            if (pw[32] > 30)
            {
                pw[32] = 30;
                pw[63] = 0;
            }
            pw[64] ^= pw[88];   // Odsifrovat delku hesla k SMTP
            if (pw[64] > 30)
            {
                pw[64] = 30;
                pw[95] = 0;
            }
        }
        else
        {
            pw[ 0] = 0;
            pw[32] = 0;
            pw[64] = 0;
        }
    }
    // IF existuje ProfileMAPI, inicializovat MAPI
    Err = GetProfileString("ProfileMAPI", Profile, sizeof(Profile));
    if (!Err)
    {
        // IF nezadano heslo, odsifrovat to z profilu
        if (!RecvPassWord || !*RecvPassWord)
        {
            DeMix(pw, pw);
            pw[pw[0] + 1] = 0;
            RecvPassWord = pw + 1;
        }
        Err = OpenMAPI(Profile, RecvPassWord);
        if (Err)
            goto exit;
    }
    // ELSE Mail602 nebo SMTP/POP3
    else if (Err == MAIL_PROFSTR_NOTFND)
    {
        EmiPath[0] = 0;
        Err = GetProfileString("UserName", m_POP3Name, sizeof(m_POP3Name));
        if (Err && Err != MAIL_PROFSTR_NOTFND)
            goto exit;
        // IF neexistuje Profil602, nacist parametry SMTP/POP3 
        Err = GetProfileString("Profile602", Profile, sizeof(Profile));
        if (Err == MAIL_PROFSTR_NOTFND)
        {
            GetProfileString("SMTPServer",        m_SMTPServer, sizeof(m_SMTPServer));
            if (Err && Err != MAIL_PROFSTR_NOTFND)
                goto exit;
            GetProfileString("MYAddress",         m_MyAddress,  sizeof(m_MyAddress));
            if (Err && Err != MAIL_PROFSTR_NOTFND)
                goto exit;
            Err = GetProfileString("POP3Server",  m_POP3Server, sizeof(m_POP3Server));
            if (Err && Err != MAIL_PROFSTR_NOTFND)
                goto exit;
            Err = GetProfileString("SMTPUserName",  m_SMTPName, sizeof(m_SMTPName));
            if (Err && Err != MAIL_PROFSTR_NOTFND)
                goto exit;
            Err = GetProfileString("DialConn",    m_DialConn,   sizeof(m_DialConn));
            if (Err && Err != MAIL_PROFSTR_NOTFND)
                goto exit;
            Err = GetProfileString("DialUserName",m_DialName,   sizeof(m_DialName));
            if (Err && Err != MAIL_PROFSTR_NOTFND)
                goto exit;
            // Kvuli kompatibilite
            Err = GetProfileString("EmiPath",     EmiPath,      sizeof(EmiPath));
            if (Err && Err != MAIL_PROFSTR_NOTFND)
                goto exit;
            memcpy(m_DialPassWord, pw + 32, pw[32] + 1);
        }
        // IF Profile602 nebo EmiPath, inicializovat Mail602
#ifdef MAIL602
        if (*Profile || *EmiPath)
        {
            char User[64];
            lstrcpy(User, m_POP3Name);
            // IF nezadano heslo, odsifrovat to z profilu
            if (!RecvPassWord || !*RecvPassWord)
            {
                DeMix(pw, pw);
                pw[pw[0] + 1] = 0;
                RecvPassWord = pw + 1;
            }
            Err = Open602(EmiPath, User, RecvPassWord, Profile);
            if (Err)
                goto exit;
        }
        else
#endif
        {
            // IF neni ani SMTP ani POP3, chyba
            if (!*m_SMTPServer && !*m_POP3Server)
            {
                Err = MAIL_PROFSTR_NOTFND;
                goto exit;
            }
            // IF neni adresa odesilatele pro SMTP nebo jmeno uzivatele pro POP3, chyba
            if ((*m_SMTPServer && !*m_MyAddress) || (*m_POP3Server && !*m_POP3Name))
            {
                Err = MAIL_PROFSTR_NOTFND;
                goto exit;
            }
            // IF POP3
            if (*m_POP3Server)
            {
                // IF zadano heslo POP3, zasifrovat
                if (RecvPassWord && *RecvPassWord)
                {
                    lstrcpy(m_PassWord + 1, RecvPassWord);
                    *m_PassWord = lstrlen(RecvPassWord);
                    Mix(m_PassWord);
                }
                // ELSEIF neni docasny profil, schovat heslo z profilu
                else if (!CWBMailProfile::IsTemp())
                {
                    memcpy(m_PassWord, pw, pw[0] + 2);
                }
            }
            // IF SMTP
            if (*m_SMTPServer)
            {
                // IF zadano heslo SMTP, zasifrovat
                if (SendPassWord && *SendPassWord)
                {
                    lstrcpy(m_SMTPPassWord, SendPassWord);
                    *m_SMTPPassWord = lstrlen(SendPassWord);
                    Mix(m_SMTPPassWord);
                }
                // ELSEIF neni docasny profil, schovat heslo z profilu
                else if (!CWBMailProfile::IsTemp())
                {
                    memcpy(m_SMTPPassWord, pw + 64, pw[64] + 2);
                }
                // IF stary profil, uzivatel SMTP = uzivatel POP3, heslo SMTP = heslo POP3
                if (!m_SMTPUserSet)
                {
                    strcpy(m_SMTPName, m_POP3Name);
                    memcpy(m_SMTPPassWord, m_PassWord, m_PassWord[0] + 2);
                }
            }
	        WSADATA wsa;
	        Err = WSAStartup(0x0101, &wsa); // WINSOCK 1.1
            //DbgLogWrite("Mail  WSAStartup()");

            if (Err)
            {
                //DbgLogFile("MAIL_ERROR WSAStartup");
                Err = MAIL_ERROR;
                goto exit;
            }
            m_WSA  = TRUE;
            m_Type = WBMT_SMTPOP3;
        }
    }
    else
        goto exit;

    // IF Dial, naloadovat RASAPI32.DLL
    if (*m_DialConn)
    {
        Err = DialCtx.RasLoad();
        if (Err)
        {
            //DbgLogFile("rasapi32.dll NOT Found");
            goto exit;
        }
        //DbgLogFile("Ras Load OK");
        m_Type |= WBMT_DIALUP;
    }
    
#else

#ifdef NETWARE

    // Na Novelu prilinkovat funkce pro inicializaci sockets
    Err = MAIL_FUNC_NOT_FOUND;
    unsigned hNLM        = GetNLMHandle();
    lpgetservbyname      = (NWGETSERVBYNAME)ImportSymbol(hNLM,    "NWgetservbyname");
    if (!lpgetservbyname)
        goto exit;
    lpNetDBgethostbyname = (NETDBGETHOSTBYNAME)ImportSymbol(hNLM, "NetDBgethostbyname");
    if (!lpNetDBgethostbyname)
        goto exit;
    lpgethostname        = (GETHOSTNAME)ImportSymbol(hNLM,        "gethostname");
    if (!lpgethostname)
        goto exit;
    //DbgLogFile("lpgetservbyname OK");

#endif // NETWARE

    // Na Linuxu nacist parametry SMTP/POP3
    Err = GetProfileString("SMTPServer", m_SMTPServer, sizeof(m_SMTPServer));
    if (Err && Err != MAIL_PROFSTR_NOTFND)
        goto exit;
    Err = GetProfileString("MYAddress",  m_MyAddress,  sizeof(m_MyAddress));
    if (Err && Err != MAIL_PROFSTR_NOTFND)
        goto exit;
    Err = GetProfileString("POP3Server", m_POP3Server, sizeof(m_POP3Server));
    if (Err && Err != MAIL_PROFSTR_NOTFND)
        goto exit;
    Err = GetProfileString("UserName",   m_POP3Name,   sizeof(m_POP3Name));
    if (Err && Err != MAIL_PROFSTR_NOTFND)
        goto exit;
    Err = GetProfileString("SMTPUserName",   m_SMTPName,   sizeof(m_SMTPName));
    if (Err && Err != MAIL_PROFSTR_NOTFND)
        goto exit;

    if ((*m_SMTPServer && !*m_MyAddress) || (*m_POP3Server && !(m_Type & WBMT_DIRPOP3) && !*m_POP3Name))
    {
        Err = MAIL_PROFSTR_NOTFND;
        goto exit;
    }
    if (*m_POP3Server)
    {
        // IF nezadano heslo POP3, nacist heslo z profilu
        if (RecvPassWord && *RecvPassWord)
        {
            lstrcpy(m_PassWord + 1, RecvPassWord);
            // Zasifrovat
            *m_PassWord = lstrlen(m_PassWord + 1);
            Mix(m_PassWord);
        }
        else if (!CWBMailProfile::IsTemp())
        {
            Err = GetProfileString("Password", m_PassWord + 1, 31);
            if (!Err)
            {
                // Zasifrovat
                *m_PassWord = lstrlen(m_PassWord + 1);
                Mix(m_PassWord);
            }
            else if (Err != MAIL_PROFSTR_NOTFND)
                goto exit;
        }
    }
    if (*m_SMTPServer)
    {
        // IF nezadano heslo SMTP, nacist heslo z profilu
        if (SendPassWord && *SendPassWord)
        {
            lstrcpy(m_SMTPPassWord + 1, SendPassWord);
            // Zasifrovat
            *m_SMTPPassWord = lstrlen(m_SMTPPassWord);
            Mix(m_PassWord);
        }
        else if (!CWBMailProfile::IsTemp())
        {
            Err = GetProfileString("SMTPPassword", m_SMTPPassWord + 1, 31);
            if (!Err)
            {
                // Zasifrovat
                *m_SMTPPassWord = lstrlen(m_SMTPPassWord + 1);
                Mix(m_SMTPPassWord);
            }
            else if (Err != MAIL_PROFSTR_NOTFND)
                goto exit;
        }
        // IF stary profil, uzivatel SMTP = uzivatel POP3, heslo SMTP = heslo POP3
        if (!m_SMTPUserSet && !*m_SMTPName && !*m_SMTPPassWord)
        {
            strcpy(m_SMTPName, m_POP3Name);
            memcpy(m_SMTPPassWord, m_PassWord, m_PassWord[0] + 2);
        }
    }
    m_Type = WBMT_SMTPOP3;

#endif // WINS

    // Nacist seznam adresaru, ze kterych lze odesilat soubory
    Err = GetFilePath();
    if (Err == MAIL_PROFSTR_NOTFND)
        Err = NOERROR;
    Err = GetProfileString("InBoxAppl",     m_Appl,     sizeof(m_Appl));
    if (Err && Err != MAIL_PROFSTR_NOTFND)
        goto exit;
    Err = GetProfileString("InBoxMessages", m_InBoxTbl, sizeof(m_InBoxTbl));
    if (Err && Err != MAIL_PROFSTR_NOTFND)
        goto exit;
    Err = GetProfileString("InBoxFiles",    m_InBoxTbf, sizeof(m_InBoxTbf));
    if (Err && Err != MAIL_PROFSTR_NOTFND)
        goto exit;
#ifdef SRVR
    m_charset = sys_spec.charset;
#else
    m_charset = wbcharset_t::get_host_charset();
#endif
    Err = NOERROR;

exit:

    CloseProfile();
    m_Open = !Err;
    return Err;
}
//
// Uvolneni prostredku posty
//
void CWBMailCtx::Close()
{
#ifdef WINS
    if (m_hDialConn)
        DialCtx.HangUp(m_hDialConn);
    DialCtx.RasFree();
#endif

    while (m_Letters)
        delete m_Letters;

    if (m_MailBox)
        delete m_MailBox;

#ifdef WINS
    if (m_WSA)
        WSACleanup();
#endif

    if (m_FilePath)
        delete []m_FilePath;

#ifdef NETWARE

    unsigned hNLM = GetNLMHandle();
    if (lpgetservbyname)
    {
        UnimportSymbol(hNLM, "NWgetservbyname");
        lpgetservbyname = NULL;
    }
    if (lpNetDBgethostbyname)
    {
        UnimportSymbol(hNLM, "NetDBgethostbyname");
        lpNetDBgethostbyname = NULL;
    }
    if (lpgethostname)
    {
        UnimportSymbol(hNLM, "gethostname");
        lpgethostname = NULL;
    }

#endif

    //DbgLogFile("Clear");
}

#ifdef WINS
//
// Inicializace MAPI
//
// Naloaduje MAPI32.DLL, inicializuje MAPI a prihlasi se, otevre slozku Posta k odeslani,
// otevre seznam adresatu a nacte seznam typu adres.
// Starsi verze MAPI neumoznuje odeseilani posty z NT servicu.
//
DWORD CWBMailCtx::OpenMAPI(const char *Profile, const char *PassWord)
{
    if (!Profile || !*Profile)
        return ERROR_IN_FUNCTION_ARG;
    //DbgLogFile("Session %08X", m_lpSession);
    if (m_lpSession)
        return NOERROR;

    m_hMapi = LoadLibrary("mapi32.dll");
    if (!m_hMapi)
        return MAIL_DLL_NOT_FOUND;
    //DbgLogFile("Load OK");
    HRESULT hrErr;

    hrErr = MAIL_FUNC_NOT_FOUND;

    MAPIINIT_0 MapInit = {MAPI_INIT_VERSION, MAPI_MULTITHREAD_NOTIFICATIONS};
    LPMAPILOGONEX    lpMAPILogonEx;
    LPMAPIINITIALIZE lpMAPIInitialize = (LPMAPIINITIALIZE)GetProcAddress(m_hMapi, "MAPIInitialize");
    if (!lpMAPIInitialize)
        goto error;
    lpMAPIUninitialize = (LPMAPIUNINITIALIZE)GetProcAddress(m_hMapi, "MAPIUninitialize");
    if (!lpMAPIUninitialize)
        goto error;
    lpMAPILogonEx = (LPMAPILOGONEX)GetProcAddress(m_hMapi, "MAPILogonEx");
    if (!lpMAPILogonEx)
        goto error;
    lpMAPIAllocateBuffer = (LPMAPIALLOCATEBUFFER)GetProcAddress(m_hMapi, "MAPIAllocateBuffer");
    if (!lpMAPIAllocateBuffer)
        goto error;
    lpMAPIFreeBuffer = (LPMAPIFREEBUFFER)GetProcAddress(m_hMapi, "MAPIFreeBuffer");
    if (!lpMAPIFreeBuffer)
        goto error;
    lpOpenStreamOnFile = (LPOPENSTREAMONFILE)GetProcAddress(m_hMapi, "OpenStreamOnFile");
    if (!lpOpenStreamOnFile)
        goto error;

    hrErr = lpMAPIInitialize(&MapInit);
    if (hrErr)
    {
        //DbgLogFile("MAPIInitialize = %08X", hrErr);
        goto error;
    }

    hrErr = lpMAPILogonEx(0, (char*)Profile, (char*)PassWord, MAPI_NEW_SESSION, &m_lpSession);
    //DbgLogFile("MAPILogonEx %08X", hrErr);
    if (hrErr)
    {
		if (hrErr == MAPI_E_LOGON_FAILED)
		{
			char  buf[64];
			DWORD sz = 64;
			WNetGetUser(NULL, buf, &sz);
			if (lstrcmp(buf, "SYSTEM") == 0)
				hrErr = ERROR_INVALID_SERVICE_ACCOUNT;
		}
        //DbgLogFile("MAPILogonEx(%s, %s) = %08X", Profile, PassWord ? *PassWord ? PassWord : "0" : "NULL", hrErr);
        goto error;
    }

    // Otevrit schranku pro postu k odeslani
    hrErr = GetOutBox();
    if (hrErr)
    {
        //DbgLogFile("GetOutBox = %08X", hrErr);
        goto error;
    }

    hrErr = m_lpSession->OpenAddressBook(NULL, NULL, 0, &m_lpAddrBook);
    if (FAILED(hrErr))
        goto error;

    // Nacist seznam podporovanych typu adres
    hrErr = m_lpSession->EnumAdrTypes(0, &m_cTypes, &m_aTypes);
    if (hrErr)
        goto error;

#ifdef  NEVER
    m_MailBox = &MailBoxMAPI;
#endif
    m_Type = WBMT_MAPI;
    return NOERROR;

error:

    //DbgLogFile("Mapi Error = %08X", hrErr);
    CloseMAPI();
    return hrErr == MAIL_FUNC_NOT_FOUND ? hrErr : hrErrToszErr(hrErr);
}
//
// Vraci interface chranky pro doslou postu a postu k odeslani.
//  
HRESULT CWBMailCtx::GetBox(LPMAPIFOLDER *lpBox, BOOL OutBox)
{
    LPMAPITABLE  lpTable    = NULL;
    LPSRowSet    lpMSSet    = NULL;
    LPSPropValue lpOutBoxID = NULL;
    LPENTRYID    lpInBoxID  = NULL;
    LPENTRYID    lpBoxID;
    HRESULT      hrErr;
    ULONG        Size;

    if (!m_lpMsgStore)
    {
        hrErr = m_lpSession->GetMsgStoresTable(0, &lpTable);
        if (hrErr != NOERROR)
        {
            //DbgLogFile("GetMsgStoresTable %08X", hrErr);
            goto exit;
        }

        // Z atributu Stores me zajima jen PR_DEFAULT_STORE a PR_ENTRYID
        char ar[sizeof(SPropTagArray) + sizeof(ULONG)];
        SPropTagArray *MSDesc;
        MSDesc = (SPropTagArray *)ar;
        MSDesc->cValues = 2;
        MSDesc->aulPropTag[0] = PR_DEFAULT_STORE;
        MSDesc->aulPropTag[1] = PR_ENTRYID;
        hrErr = lpTable->SetColumns(MSDesc, 0);
        if (hrErr != NOERROR)
        {
            //DbgLogFile("SetColumns %08X", hrErr);
            goto exit;
        }
        // Nacist seznam Stores
        hrErr = lpTable->GetRowCount(0, &Size);
        if (hrErr != NOERROR)
        {
            //DbgLogFile("GetRowCount %08X", hrErr);
            goto exit;
        }
        hrErr = lpTable->QueryRows(Size, 0, &lpMSSet);
        if (hrErr != NOERROR)
        {
            //DbgLogFile("QueryRows %08X", hrErr);
            goto exit;
        }
        // V seznamu najit PR_DEFAULT_STORE 
        UINT i;
        for (i = 0; i < lpMSSet->cRows; i++)
        {
            if (lpMSSet->aRow[i].lpProps[0].Value.b)
            {
                // Otevrit store a zjistit PR_IPM_OUTBOX_ENTRYID
                hrErr = m_lpSession->OpenMsgStore(NULL, lpMSSet->aRow[i].lpProps[1].Value.bin.cb, (LPENTRYID)lpMSSet->aRow[i].lpProps[1].Value.bin.lpb, NULL, MDB_NO_DIALOG | MDB_WRITE, &m_lpMsgStore);
                if (hrErr != NOERROR)
                {
                    //DbgLogFile("OpenMsgStore %08X", hrErr);
                    goto exit;
                }
                break;
            }
            lpMAPIFreeBuffer(&lpMSSet->aRow[i].lpProps[0]);
            lpMAPIFreeBuffer(&lpMSSet->aRow[i].lpProps[1]);
        }
    }
    // Zjistit ENTRYID pozadovane schranky
    if (OutBox)
    {
        SPropTagArray OutBoxPropID;
        OutBoxPropID.cValues       = 1;
        OutBoxPropID.aulPropTag[0] = PR_IPM_OUTBOX_ENTRYID;
        hrErr = m_lpMsgStore->GetProps(&OutBoxPropID, 0, &Size, &lpOutBoxID);
        if (hrErr != NOERROR)
        {
            //DbgLogFile("GetProps %08X", hrErr);
            goto exit;
        }
        Size    = lpOutBoxID->Value.bin.cb;
        lpBoxID = (LPENTRYID)lpOutBoxID->Value.bin.lpb;

        // Zjistit ENTRYID "Odeslana posta"
        DWORD sSize;
        OutBoxPropID.cValues       = 1;
        OutBoxPropID.aulPropTag[0] = PR_IPM_SENTMAIL_ENTRYID;
        m_lpMsgStore->GetProps(&OutBoxPropID, 0, &sSize, &m_lpSentMailID);
    }
    else
    {
        hrErr = m_lpMsgStore->GetReceiveFolder(NULL, 0, &Size, &lpInBoxID, NULL);
        if (hrErr != NOERROR)
            goto exit;
        lpBoxID = lpInBoxID;
    }
    
    ULONG ObjType;
    // Otevrit Box
    hrErr = m_lpSession->OpenEntry(Size, lpBoxID, NULL, /*MDB_NO_DIALOG | */MAPI_BEST_ACCESS, &ObjType, (LPUNKNOWN *)lpBox);
    if (hrErr != NOERROR)
    {
        //DbgLogFile("OpenEntry %08X", hrErr);
        goto exit;
    }

exit:    

    if (lpOutBoxID)
        lpMAPIFreeBuffer(lpOutBoxID);
    if (lpInBoxID)
        lpMAPIFreeBuffer(lpInBoxID);
    if (lpMSSet)
        lpMAPIFreeBuffer(lpMSSet);
    if (lpTable)
        lpTable->Release();
    return hrErr;
}
//
// Uvolneni prosredku alokovanych pro MAPI
//
void CWBMailCtx::CloseMAPI()
{
    if (m_lpSentMailID)
        lpMAPIFreeBuffer(m_lpSentMailID);
    if (m_aTypes)
        lpMAPIFreeBuffer(m_aTypes);
    if (m_lpAddrBook)
    {
        DWORD i = m_lpAddrBook->Release();
        //DbgLogFile("AddrBook  %d", i);
    }
    if (m_lpOutBox)
    {
        DWORD i = m_lpOutBox->Release();
        //DbgLogFile("OutBox  %d", i);
    }
    if (m_lpMsgStore)
        m_lpMsgStore->Release();
    if (m_lpSession)
    {
       m_lpSession->Logoff(0, 0, 0);
       DWORD i = m_lpSession->Release();
       m_lpSession = NULL;
       //DbgLogFile("Session  %d", i);
       lpMAPIUninitialize();
    }
    if (m_hMapi)
       FreeLibrary(m_hMapi);
    //CoUninitialize();
}

#ifdef MAIL602
//
// Vraci cestu k MAIL602.INI
//
char MultiUser[] = "Software602\\Mail602.Klient\\MultiUser"; 
char POP3[]      = "POP3";

void GetMail602INI(char *Ini)
{
    DWORD Sz;
    HKEY  hKey;
    *Ini = 0;
    // Nejdriv zkusim registracni databazi
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, MultiUser, 0, KEY_QUERY_VALUE_EX, &hKey) == ERROR_SUCCESS)
    {
        DWORD Multion;
        Sz       = sizeof(Multion);
        LONG Err = RegQueryValueEx(hKey, "multion", NULL, NULL, (LPBYTE)&Multion, &Sz);
        RegCloseKey(hKey);
        if (Err == ERROR_SUCCESS && Multion)
        {
            if (RegOpenKeyEx(HKEY_CURRENT_USER, MultiUser, 0, KEY_QUERY_VALUE_EX, &hKey) == ERROR_SUCCESS)
            {
                Sz  = MAX_PATH;
                RegQueryValueEx(hKey, "inipath", NULL, NULL, (LPBYTE)Ini, &Sz);
                RegCloseKey(hKey);
            }
        }
    }
    // Jinak adresar windows
    if (!*Ini)
        GetWindowsDirectory(Ini, MAX_PATH);
    Sz = lstrlen(Ini);
    if (Ini[Sz - 1] == '\\')
        Sz--;
    lstrcpy(Ini + Sz, "\\MAIL602.INI");
}
//
// Inicializace Mail602
//
// Pokud neni zadana cesta k EMI, najde profilu prislusejici INI soubor a nej vytahne vsechny
// potrebne parametry.
//
DWORD CWBMailCtx::Open602(char *EmiPath, char *UserID, const char *PassWord, const char *Profile)
{
    LPJEEMI100               lpJeEMI100;
    LPMAIL602ISREADY         lpMail602IsReady;
    LPINITMAIL602LOGGEDUSER  lpInitLogged;
    LPINITMAIL602LOGGEDUSERX lpInitLoggedX;
    LPDISPOSEMESSAGEFILELIST lpFreeFileList;
    LPINITMAIL602            lpInitMail602;
    LPINITMAIL602X           lpInitMail602X;
    WIN32_FIND_DATA          ff;
    DWORD                    Err = 0;
    char Ini[MAX_PATH];
    char Mail602INI[MAX_PATH];
    char pEmiPath[MAX_PATH];  
    char EMI[MAX_PATH];

    //DbgLogFile("Open602 %s %s %s %s", EmiPath  ? *EmiPath  ? EmiPath  : "0" : "NULL",
    //                                  UserID   ? *UserID   ? UserID   : "0" : "NULL",
    //                                  PassWord ? *PassWord ? PassWord : "0" : "NULL",
    //                                  Profile  ? *Profile  ? Profile  : "0" : "NULL");
    if (!EmiPath || !*EmiPath)
    {
        EmiPath = EMI;
        DWORD Sz;
        
        // Mail602INI = cesta k MAIL602.INI 
        GetMail602INI(Mail602INI);
        // Pokud profil neni zadan a v systemu je pouze jediny profil, pouzije se tento jediny
        if (!Profile || !*Profile)
        {
            Sz = GetPrivateProfileString("PROFILES", NULL, NULL, Ini, sizeof(Ini), Mail602INI);
            if (!Sz || Sz >= sizeof(Ini) - 2)
                return MAIL_BAD_PROFILE;
            char *p = Ini + lstrlen(Ini) + 1;
            if (*p)
                return MAIL_BAD_PROFILE;
            Profile = Ini;
            //DbgLogFile("SingleProfile: %s", Ini);
        }
        // Profilu prislusici INI
        Sz = GetPrivateProfileString("PROFILES", Profile, NULL, Ini, sizeof(Ini), Mail602INI);
        if (!Sz)
            return MAIL_BAD_PROFILE;
        //DbgLogFile("Ini: %s", Ini);
        // Cesta k EMI
        Sz = GetPrivateProfileString("SETTINGS", "MAIL602DIR", NULL, EMI, sizeof(EMI), Ini);
        if (!Sz)
            return MAIL_BAD_PROFILE;
        //DbgLogFile("Emi: %s", EMI);
        // Pokud profil pouziva POP3, POP3Server, UserName, SMTPServer, SenderAddress
        if (GetPrivateProfileInt(POP3, POP3, 0, Ini))
        {
            //DbgLogFile("POP3");
            GetPrivateProfileString(POP3, "Server",   NULL, m_POP3Server, sizeof(m_POP3Server), Ini);
            //DbgLogFile("POP3Server: %s", m_POP3Server);
            GetPrivateProfileString(POP3, "UserName", NULL, m_POP3Name,   sizeof(m_POP3Name),   Ini);
            //DbgLogFile("UserName: %s", m_POP3Name);
            if (GetPrivateProfileInt(POP3, "UsePOP3ForSending", 0, Ini))
            {
                GetPrivateProfileString(POP3, "SMTPServer",      NULL, m_SMTPServer, sizeof(m_SMTPServer), Ini);
                //DbgLogFile("SMTPServer: %s", m_SMTPServer);
                GetPrivateProfileString(POP3, "InternetAddress", NULL, m_MyAddress,  sizeof(m_MyAddress),  Ini);
                //DbgLogFile("MyAddress: %s", m_MyAddress);
                ctopas(m_MyAddress, m_MyAddress);
            }
            if (GetPrivateProfileInt(POP3, "DIAL", 0, Ini))
            {
                GetPrivateProfileString(POP3, "DIALUPCONN",     NULL, m_DialConn, sizeof(m_DialConn), Ini);
                //DbgLogFile("DialConn: %s", m_DialConn);
                GetPrivateProfileString(POP3, "DIALUPUSERNAME", NULL, m_DialName, sizeof(m_DialName), Ini);
                //DbgLogFile("DialName: %s", m_DialName);
            }

            if (*m_POP3Server || *m_DialConn)
            {
                char pwf[MAX_PATH];
                GetWindowsDirectory(pwf, sizeof(pwf));
                //DbgLogFile("WinDir: %s", pwf);
                char *p = pwf + lstrlen(pwf);
                *p++ = '\\';
                //DbgLogFile(Ini);
                Sz = lstrlen(Ini) - 3;
                lstrcpyn(p, Ini, Sz + 1);
                p += Sz;
                if (*m_POP3Server)
                {
                    lstrcpy(p, "PW3");
                    //DbgLogFile("PWF: %s", pwf);
                    FHANDLE hFile = CreateFile(pwf, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                    if (!FILE_HANDLE_VALID(hFile))
                        return MAIL_FILE_NOT_FOUND;
                    if (!ReadFile(hFile, m_PassWord, sizeof(m_PassWord), &Sz, NULL))
                        Err = OS_FILE_ERROR;
                    CloseFile(hFile);
                    if (Err)
                        return OS_FILE_ERROR;
                }
                if (*m_DialConn)
                {
                    lstrcpy(p, "PWD");
                    //DbgLogFile("PWD: %s", pwf);
                    FHANDLE hFile = CreateFile(pwf, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                    if (!FILE_HANDLE_VALID(hFile))
                        return MAIL_FILE_NOT_FOUND;
                    if (!ReadFile(hFile, m_DialPassWord, sizeof(m_DialPassWord), &Sz, NULL))
                        Err = OS_FILE_ERROR;
                    CloseFile(hFile);
                    if (Err)
                        return OS_FILE_ERROR;
                }
            }
        }
    }
    if (!UserID)
        UserID = "";
    // Dalsi prihlaseni s jinym EMI nebo uzivatelem, Chyba
    if (*m_EMI)
    {
        //DbgLogFile("m_EMI: %s", m_EMI);
        //DbgLogFile("EMI:   %s", EmiPath);
        //DbgLogFile("m_UserID: %s", m_UserID);
        //DbgLogFile("UserID:   %s", UserID);
        if (lstrcmpi(EmiPath, m_EMI) || (lstrcmpi(UserID, m_UserID) && *m_SMTPServer == 0 && *m_POP3Server == 0))
            return MAIL_ALREADY_INIT;
        else
            return NOERROR;
    }
    else
    {
        lstrcpy(m_EMI,    EmiPath);
        lstrcpy(m_UserID, UserID);
    }

    // IF EMI neexistuje, chyba
    lstrcpy(pEmiPath, m_EMI);
    char *p = pEmiPath + lstrlen(pEmiPath);
    if (*(p - 1) == '\\')
        p--;
    lstrcpy(p, "\\m602.emi");
    HANDLE hff = FindFirstFile(pEmiPath, &ff);
    if (hff == INVALID_HANDLE_VALUE)
    {
        //DbgLogFile("EMI not found %08X", GetLastError());
        Err = MAIL_BAD_PROFILE;
        goto error;
    }
    FindClose(hff);
    
    // Naloadovat WM32M32.DLL
    char bb[256];
    GetModuleFileName(NULL, bb, sizeof(bb));
    DbgLogWrite(bb);
    m_hMail602 = LoadLibrary((LPSTR)"wm602m32.dll");
    if (!m_hMail602)
    {
        //DbgLogFile("WM602M32.DLL not found");
        Err = MAIL_DLL_NOT_FOUND;
        goto error;
    }
    Err = MAIL_FUNC_NOT_FOUND;
    lpFreeFileList = (LPDISPOSEMESSAGEFILELIST)GetProcAddress(m_hMail602, "DisposeMessageFileList");
    if (!lpFreeFileList) 
        goto error;
    lpJeEMI100 = (LPJEEMI100)GetProcAddress(m_hMail602, "JeEMI100");
    if (!lpJeEMI100)
        goto error;
    lpMail602IsReady = (LPMAIL602ISREADY)GetProcAddress(m_hMail602, "Mail602IsReady");
    if (!lpMail602IsReady)
        goto error;
    lpInitLogged = (LPINITMAIL602LOGGEDUSER)GetProcAddress(m_hMail602, "InitMail602LoggedUser");
    if (!lpInitLogged)
        goto error;
    lpGetUserNameFromID = (LPGETMAIL602USERNAMEFROMID)GetProcAddress(m_hMail602, "cGetMail602UserNameFromID");
    if (!lpGetUserNameFromID)
        goto error;
    lpInitMail602 = (LPINITMAIL602)GetProcAddress(m_hMail602, "InitMail602");
    if (!lpInitMail602)
        goto error;
    lpInitMail602X = (LPINITMAIL602X)GetProcAddress(m_hMail602, "InitMail602X");
    if (!lpInitMail602X)
        goto error;
    lpAddAddr     = (LPADDADDRESSTOADDRESSLIST)GetProcAddress(m_hMail602, "AddAddressToAddressListX"); // udela kopii adresy!
    if (!lpAddAddr)
        goto error;
    lpAddFile     = (LPADDFILETOFILELIST)GetProcAddress(m_hMail602, "AddFileToFileList");
    if (!lpAddFile)
        goto error;
    lpFreeAList   = (LPDISPOSEADDRESSLIST)GetProcAddress(m_hMail602, "DisposeAddressList"); // uvolni kopii adresy
    if (!lpFreeAList)
        goto error;
    lpFreeFList   = (LPDISPOSEFILELIST)GetProcAddress(m_hMail602, "DisposeFileList");
    if (!lpFreeFList)
        goto error;
    lpGetLocalUserID      = (LPGETLOCALUSERID)GetProcAddress(m_hMail602, "GetLocalUserID");
    if (!lpGetLocalUserID)
        goto error;
    lpGetMyUserInfo       = (LPGETMYUSERINFO)GetProcAddress(m_hMail602, "GetMyUserInfo");
    if (!lpGetMyUserInfo)
        goto error;
    lpGetExternalUserList = (LPGETEXTERNALUSERLIST)GetProcAddress(m_hMail602, "GetExternalUserList");
    if (!lpGetExternalUserList)
        goto error;
    lpGetPrivateList      = (LPGETPRIVATELIST)GetProcAddress(m_hMail602, "GetPrivateList");
    if (!lpGetPrivateList)
        goto error;
    lpGetPrivateUserList  = (LPGETPRIVATEUSERLIST)GetProcAddress(m_hMail602, "GetPrivateUserList");
    if (!lpGetPrivateUserList)
        goto error;
    lpFreeListList        = (LPDISPOSELISTSLISTS)GetProcAddress(m_hMail602, "DisposeListsLists");
    if (!lpFreeListList)
        goto error;
    lpReadADRFileX        = (LPREADADRFILEX)GetProcAddress(m_hMail602, "ReadADRFileX");
    if (!lpReadADRFileX)
        goto error;
    lpInitLoggedX = (LPINITMAIL602LOGGEDUSERX)GetProcAddress(m_hMail602, "InitMail602LoggedUserX");

    lpSendEx      = (LPSENDMAIL602MESSAGEEX)GetProcAddress(m_hMail602, "SendMail602MessageEx");
    //DbgLogFile("SendEx: %08X", lpSendEx);
    if (!lpSendEx)
    {
        lpSend    = (LPSENDMAIL602MESSAGE)GetProcAddress(m_hMail602, "SendMail602Message");
        if (!lpSend)
            goto error;
    }
    m_Type |= WBMT_602;
    // IF SMTP, naloadovat M602INET.DLL
    if (*m_SMTPServer || *m_POP3Server)
    {
        Err = MAIL_DLL_NOT_FOUND;
        m_hM602Inet = LoadLibrary((LPSTR)"m602inet.dll");
        if (!m_hM602Inet)
        {
            //DbgLogFile("M602INET.DLL NOT Found");
            goto error;
        }
        Err = MAIL_FUNC_NOT_FOUND;
        if (!lpSendEx)
            goto error;
        lpSendQueuedInternetMsgs = (LPSENDQUEUEDINTERNETMSGS)GetProcAddress(m_hM602Inet, "M602InetSendQueuedDirectInternetMessagesForUser");
        if (!lpSendQueuedInternetMsgs)
            goto error;
        //DbgLogFile("M602INET.DLL OK");
        m_Type |= WBMT_602SP3;
    }

    // emt = typ Mail602 klienta
    ctopas(m_EMI, pEmiPath);
    short emt;
    emt = lpJeEMI100(pEmiPath);
    //DbgLogFile("JeEMI100: %d", emt);
    // IF Remote nebo SMTP/POP3
    if (emt == 6 || emt == 7)
    {
        if (!*m_SMTPServer && !*m_POP3Server)
            m_Type |= WBMT_602REM | WBMT_DIALUP;
        LPREADRMCONFIG lpReadRMConfig = (LPREADRMCONFIG)GetProcAddress(m_hMail602,"ReadRMConfiguration");
        if (lpReadRMConfig)
        {
            RMConfig *cfg;
            lpReadRMConfig(pEmiPath, &cfg);
            cfg->UserID[9] = 0;
            if (lstrcmp(cfg->UserID + 1, "EEEEEEEE"))
            {
                memcpy(m_RemBox, "Schr\0xA0nka : ", 11);
                memcpy(m_RemBox + 11, cfg->UserID + 1, 8);
            }
            else
            {
                memcpy(m_UserID, "00000001", 9);
            }
        }
    }

    // IF WM32M32.DLL uz inicializovana, jen InitMail602X
    if (lpMail602IsReady())
    {
        //DbgLogFile("Mail602 Is Ready");
        lpInitMail602X();
        m_NoLogout = TRUE;
        goto OK;
    }

    // IF Remote nebo nezadan uzivatel, InitLogged
    if ((m_Type & WBMT_602REM) || !UserID || !*UserID)
    { 
        if (emt == 4 && lpInitLoggedX) // LAN
            Err = lpInitLoggedX(pEmiPath);
        else
            Err = lpInitLogged(pEmiPath);
        //DbgLogFile("InitLogged: %d", Err);
        if (!Err)
            goto OK;
        if (Err == 2)
            goto errorc;
    }

    char UserName[32];
    *UserName = 0;
    char *pe;
    strtol(UserID, &pe, 16);
    // IF zadano ID, Zkonvertovat UserName
    if (pe == UserID + 8)
    {
        Err = lpGetUserNameFromID(m_EMI, UserName, UserID);
        //DbgLogFile("GetUserNameFromID: %d", Err);
        if (Err)
            goto errorc;
        UserID = UserName;
    }

    // Inicializovat pro schranku uzivatele UserName
    char pPassWord[32];
    ctopas(UserID,   UserName);
    ctopas((char*)PassWord, pPassWord);
    Err = lpInitMail602(pEmiPath, UserName, pPassWord);
    //DbgLogFile("InitMail602: %d", Err);
    if (Err)
        goto errorc;

OK:

    return NOERROR;

errorc:

    Err = Err602ToszErr(Err);

error:

    m_Type = 0;
    Close602();
    return Err;
}
//
// Uvolneni prostredku alokovanych pro Mail602
//
void CWBMailCtx::Close602()
{
    if (RemoteCtx.m_hGateThread)
        RemoteCtx.HangUp();
    if (m_hM602Inet)
        FreeLibrary(m_hM602Inet);
    if (m_hMail602 && !m_NoLogout)
        FreeLibrary(m_hMail602);

    lpGetMsgList = NULL;
}
#endif // MAIL602

//
// Vraci priznak zda je zadany typ adresy podporovan MAPI
//
BOOL CWBMailCtx::IsMAPIAddr(char *Type)
{
    if (m_lpSession)
    {
        if (lstrcmpi(Type, "Internet") == 0)
            return TRUE;
        if (lstrcmpi(Type, "SMTP") == 0)
            return TRUE;
        for (ULONG i = 0; i < m_cTypes; i++)
        {
            if (!lstrcmpi(Type, m_aTypes[i]))
                return TRUE;
        }
    }
    return FALSE;
}
//
// Vraci zda je zadany typ adresy podporovan Mail602
//
BOOL CWBMailCtx::Is602Addr(char *Type)
{
#ifdef MAIL602
    return m_hMail602 && (!lstrcmpi(Type, "Internet") || !lstrcmpi(Type, "SMTP") || !lstrcmpi(Type, "MAIL602"));
#else
    return FALSE;
#endif        
}
//
// WndProc stavoveho okna M602INET.DLL
//
LRESULT CALLBACK SMTPWndProc(HWND hWnd, UINT Msg, WPARAM wPar, LPARAM lPar)
{
    return DefWindowProc(hWnd, Msg, wPar, lPar);
}
//
// Vytvoreni stavoveho okna M602INET.DLL
//
HWND CWBMailCtx::CreateLogWnd()
{
    static ATOM Registred;
    WNDCLASS WndClass;
    if (!Registred)
    {
        WndClass.style         = 0;
        WndClass.lpfnWndProc   = SMTPWndProc; 
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = 0;
        WndClass.hInstance     = 0; 
        WndClass.hIcon         = NULL;
        WndClass.hCursor       = NULL;
        WndClass.hbrBackground = NULL; 
        WndClass.lpszMenuName  = NULL;
        WndClass.lpszClassName = SMTPWndClass;
        Registred = RegisterClass(&WndClass);
        if (!Registred)
            return NULL;
    }

    return CreateWindow(SMTPWndClass, "WBSMTP", WS_POPUP, 0, 0, 1, 1, NULL, NULL, hInst, 0);
}

#ifdef MAIL602
//
// Odeslani zasilky pomoci M602INET.DLL
//
DWORD CWBMailCtx::SendSMTP602()
{
    DWORD Err;
    if (*m_DialConn)
    {
        //DbgLogFile("    DialSMTP");
        Err = DialSMTP();
        //DbgLogFile("    DialSMTP %d", Err);
        if (Err)
            return Err;
    }

    HWND hLogWnd = CreateLogWnd();
    if (!hLogWnd)
    {
        //DbgLogFile("MAIL_ERROR CreateLogWnd");
        return MAIL_ERROR;
    }
    char dir[MAX_PATH];
    GetCurrentDirectory(sizeof(dir), dir);
    Err = lpSendQueuedInternetMsgs(m_EMI, m_UserID, m_SMTPServer, hLogWnd, hInst, 12345);
    //DbgLogFile("    SendQueuedInternetMsgs = %d", Err);

    if (hLogWnd)
        DestroyWindow(hLogWnd);
    if (m_hDialConn)
    {
        //DbgLogFile("    HangUpSMTP");
        HangUpSMTP();
        //DbgLogFile("    HangUpSMTP OK");
    }

    if (Err)
        return Err <= 15 ? MAIL_CONNECT_FAILED : MAIL_SOCK_IO_ERROR;
    return NOERROR;
}
//
// Vytvoreni dial TCP/IP spojeni
//
//
// Odeslani zasilky prostrednictvim remote klienta
//
// V pripade Remote klienta zabezpecuje komunikaci s materskym uradem FaxMailServer.
// Ten bezi v samostatnem vlakne, vzdy po stanovenem intervalu prohledne vystupni schranku,
// pokud v ni jsou nejake zasilky, navaze dialup spojeni a prenese je na matesky urad, je-li
// mezi zasilkami zadost o seznam zasilek nebo stazeni zasilky, prevezme pozadovane zasilky.  
// Po te co preda a prevezme vsechny zasilky, zavesi. Funkce SetWBTimeout zabezpeci to, ze 
// se pred zavesenim jeste okamzik pocka, aby byl cas zpracovat dosle zasilky a odeslat ve 
// stejne relaci odpoved nebo pozadat o dalsi zasilky. Aktivitu FaxMailServeru mimo interval
// vyvola funkce SpeedUp. Znovu prohledani vystupni fronty zpusobi funkce ForceReread.
//
DWORD CWBMailCtx::SendRemote()
{
    //DbgLogFile("DialRemote");
    DWORD Err = RemoteCtx.Dial(m_EMI, m_hMail602, 1);
    //DbgLogFile("DialRemote %d", Err);
    if (Err)
        return Err;
    DbgPrint("Wait IN");
    WaitForSingleObject(RemoteCtx.m_GateON, INFINITE);
    DbgPrint("Wait OUT");
    ResetEvent(RemoteCtx.m_GateON);
    //DbgLogFile("SpeedUp");
    lpSpeedUp();
    //DbgLogFile("SpeedUp OK");
    RemoteCtx.HangUp();
    return NOERROR;
}

#endif  // MAIL602

DWORD CWBMailCtx::DialSMTP(char *PassWord)
{   
    if (!*m_DialConn)
        return MAIL_PROFSTR_NOTFND;
    char pw[256];
    if (!PassWord || !*PassWord)
    {
        DeMix(pw, m_DialPassWord);
        pw[*pw + 1] = 0;
        PassWord = pw + 1;
    }
    return DialCtx.Dial(m_DialConn, m_DialName, PassWord, &m_hDialConn);
}
//
// Zruseni dial TCP/IP spojeni
//
DWORD CWBMailCtx::HangUpSMTP()
{ 
    // IF neni dialup posta, chyba
    if (!*m_DialConn)
        return MAIL_PROFSTR_NOTFND;
    DialCtx.HangUp(m_hDialConn);
    m_hDialConn = DialCtx.m_hDialConn;
    return NOERROR;
}
#endif

//
// Vraci TRUE pokud se zadany soubor nachazi v povolene ceste
// 
BOOL CWBMailCtx::IsValidPath(const char *fName)
{
    if (!m_FilePath)
        return TRUE;

    char FileName[MAX_PATH];
    const char *ps = fName;
    const char *pe = fName + MAX_PATH;
    char       *pd = FileName;

    if (*fName == '.')
        pd += GetCurrentDir(FileName, sizeof(FileName));

    for (; *ps; ps++, pd++)
    {
        if (ps >= pe)
            return FALSE;
        if (*ps == '.' && *(ps + 1) == PATH_SEPARATOR)
        {
            ps++;
        }
        else if (*ps == '.' && *(ps + 1) == '.' && *(ps + 2) == PATH_SEPARATOR)
        {
            pd = strrchr(FileName, '\\');
            if (!pd)
                return FALSE;
            ps += 2;
        }
        else if (*ps == PATH_SEPARATOR)
        {
            *pd = '\\';
        }
        else
        {
            *pd = *ps;
        }
    }

    int Len;

    for (char *pt = m_FilePath; *pt; pt += Len + 1) 
    {
        Len = lstrlen(pt);
        if (strncmpDir(FileName, pt, Len))
            return TRUE;
    }
    return FALSE;
}
//
// To stejne pro UNICODE
//
BOOL CWBMailCtx::IsValidPathW(const wuchar *fName)
{
    if (!m_FilePath)
        return TRUE;

    wuchar FileName[MAX_PATH];
    const wuchar *ps = fName;
    wuchar       *pd = FileName;

    if (*fName == '.')
        pd += GetCurrentDirW(FileName, sizeof(FileName) / sizeof(wuchar));

    for (; *ps; ps++, pd++)
    {
        if (*ps == '.' && *(ps + 1) == PATH_SEPARATOR)
        {
            ps++;
        }
        else if (*ps == '.' && *(ps + 1) == '.' && *(ps + 2) == PATH_SEPARATOR)
        {
            pd = wusrchr(FileName, '\\');
            if (!pd)
                return FALSE;
            ps += 2;
        }
        else if (*ps == PATH_SEPARATOR)
        {
            *pd = '\\';
        }
        else
        {
            *pd = *ps;
        }
    }

    int Len;
    wuchar wPath[MAX_PATH];

    for (char *pt = m_FilePath; *pt; pt += Len + 1) 
    {
        Len = lstrlen(pt);
        ToUnicode(pt, Len + 1, wPath, sizeof(wPath) / sizeof(wuchar), wbcharset_t::get_host_charset());
        if (strncmpDirW(FileName, wPath, Len))
            return TRUE;
    }
    return FALSE;
}
//
// Test platnosti ukazatele na zasilku
//                 
// Rozpracovane zasilky jsou ve spojovem seznamu, ktery zacina na m_Letters.
//
BOOL CWBMailCtx::IsValid(CWBLetter *Letter)
{
    for (CWBLetter *lt = m_Letters; lt; lt = lt->m_Next)
    {
        if (lt == Letter)
            return TRUE;
    }
    return FALSE;
}

#if defined(SRVR)
//
// Kriticka sekce pro synchronizaci na serveru
// - Synchronizuje zapis ukazatele na CWBMailCtx do cdp - nejspis neni potreba
// - Synchronizuje odesilani zasilky
// - Synchronizuje vytaceni a zavesovani DialUp spojeni
//
void CWBMailSect::Init()
{
    if (!m_cRef)
    {
        //DbgLogFile("InitializeCriticalSection");
        InitializeCriticalSection(&m_cSect);
    }
    m_cRef++;
}

void CWBMailSect::Enter()
{
    //DbgLogFile("EnterCriticalSection IN");
    if (m_cRef)
        EnterCriticalSection(&m_cSect);
    //DbgLogFile("EnterCriticalSection OUT");
}

void CWBMailSect::Leave()
{
    if (m_cRef)
        LeaveCriticalSection(&m_cSect);
    //DbgLogFile("LeaveCriticalSection OUT");
}

void CWBMailSect::Delete()
{
    if (!m_cRef)
        return;
    m_cRef--;
    if (m_cRef == 0)
    {
        //DbgLogFile("DeleteCriticalSection");
        DeleteCriticalSection(&m_cSect);
    }
}

#endif // SRVR
//
// Sprava postovnich profilu
//
// Vytvoreni
//
DWORD CWBMailProfile::Create(const char *ProfileName)
{
    DWORD Err = NOERROR;
    if (!AmIDBAdmin())
        return NO_RIGHT;

#ifdef WINS

    char Key[128] = KEY602SQL_MAIL;
    lstrcat(Key, "\\");
    lstrcat(Key, ProfileName);

    DWORD Dispo;
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS_EX, NULL, &m_hKey, &Dispo) != NOERROR)
    {
        //DbgLogFile("MAIL_ERROR RegCreateKeyEx %s", Key);
        return MAIL_ERROR;
    }
    if (Dispo == REG_OPENED_EXISTING_KEY)
        Err = MAIL_PROFILE_EXISTS;
    return Err;

#else

    return Exists(ProfileName);

#endif

}
//
// Testuje, zda existuje permanentni profil ProfilName
// Vraci: NOERROR             - profil neexistuje
//        MAIL_PROFILE_EXISTS - profil existuje
//        MAIL_ERROR          - jina chyba
//
#ifdef WINS

DWORD CWBMailProfile::Exists(const char *ProfileName)
{
    char Key[128] = KEY602SQL_MAIL;
    lstrcat(Key, "\\");
    lstrcat(Key, ProfileName);

    HKEY hKey;
    DWORD Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0, KEY_READ_EX, &hKey);
    if (Err == NOERROR)
    {
        RegCloseKey(hKey);
        return MAIL_PROFILE_EXISTS;
    }
    return Err == ERROR_FILE_NOT_FOUND ? NOERROR : MAIL_ERROR;
}

#else

DWORD CWBMailProfile::Exists(const char *ProfileName)
{
    int  Size = 512;
    char *Prof = (char *)corealloc(Size, 0x88);
    while (Prof)
    {
        if (GetPrivateProfileString(NULL, NULL, "", Prof, Size, CONFIG_FILE) < Size - 2)
            break;
        Size += 512;
        Prof = (char *)corerealloc(Prof, Size);
    }
    if (!Prof)
        return OUT_OF_APPL_MEMORY;

    DWORD Err = NOERROR;
    char *p;
    for (p = Prof; *p; p += lstrlen(p) + 1)
    {
        if (strnicmp(p, ".MAIL_", 6) == 0 && stricmp(p + 6, ProfileName) == 0)
        {
            Err = MAIL_PROFILE_EXISTS;
            break;
        }
    }
    corefree(Prof);
    return Err;
}

#endif 
//
// Zruseni profilu
//
DWORD CWBMailProfile::Delete(const char *ProfileName)
{
    if (lstrlen(ProfileName) > 64)
        return ERROR_IN_FUNCTION_ARG;
    if (!AmIDBAdmin())
        return NO_RIGHT;

#ifdef WINS

    char Key[128] = KEY602SQL_MAIL;
    lstrcat(Key, "\\");
    lstrcat(Key, ProfileName);
    if (RegDeleteKey(HKEY_LOCAL_MACHINE, Key) != NOERROR)
        return MAIL_BAD_PROFILE;

#else

    OpenProfile(ProfileName);
    if (!WritePrivateProfileString(m_ProfSect, NULL, NULL, CONFIG_FILE))
        return OS_FILE_ERROR;

#endif

    return NOERROR;
}
//
// Nacteni polozky
//
DWORD CWBMailProfile::GetProp(const char *ProfileName, const char *PropName, char *PropValue, int ValSize)
{
#ifdef WINS

    if (lstrcmpi(PropName, "OPTIONS") == 0)
        return MAIL_PROFSTR_NOTFND;
    DWORD Err = OpenProfile(ProfileName);
    if (Err)
        return Err;
    DWORD sz = ValSize;
    Err = RegQueryValueEx(m_hKey, PropName, NULL, NULL, (LPBYTE)PropValue, &sz); 
    if (Err)
        return Err == ERROR_MORE_DATA ? ERROR_IN_FUNCTION_ARG : MAIL_PROFSTR_NOTFND;

#else   // UNIX

    if (lstrcmpi(PropName, "Password") == 0 || lstrcmpi(PropName, "SMTPPassword") == 0)
        return MAIL_PROFSTR_NOTFND;
    DWORD Err = OpenProfile(ProfileName);
    if (Err)
        return Err;
    DWORD sz = GetPrivateProfileString(m_ProfSect, PropName, "", PropValue, ValSize, CONFIG_FILE);
    if (sz >= ValSize - 1)
        return ERROR_IN_FUNCTION_ARG;
    if (sz == 0)
        return MAIL_PROFSTR_NOTFND;
        
#endif

    return NOERROR;
}
//
// Zapis polozky
//
DWORD CWBMailProfile::SetProp(const char *ProfileName, const char *PropName, const char *PropValue)
{
    if (!AmIDBAdmin())
        return NO_RIGHT;

#ifdef WINS

    if (lstrcmpi(PropName, "OPTIONS") == 0)
        return ERROR_IN_FUNCTION_ARG;
    DWORD Err = OpenProfile(ProfileName);
    if (Err)
        return Err;
    // IF heslo, zasifrovat a ulozit do klice Options
    bool mpw = lstrcmpi(PropName, "Password") == 0;
    bool dpw = lstrcmpi(PropName, "DialPassword") == 0;
    bool spw = lstrcmpi(PropName, "SMTPPassword") == 0;
    if (mpw || dpw || spw)
    {
        int len = lstrlen(PropValue);
        if (len > 31)
            return ERROR_IN_FUNCTION_ARG;
        char pw[92];
        int  poff;
        int  xoff;
        if (mpw)
        {
            poff = 0;
            xoff = 17;
        }
        else if (dpw)
        {
            poff = 32;
            xoff = 43;
        }
        else 
        {
            poff = 64;
            xoff = 88;
        }
        pw[ 0] = 0;
        pw[ 1] = 0;
        pw[32] = 0;
        pw[33] = 0;
        pw[64] = 0;
        pw[65] = 0;
        DWORD Size = sizeof(pw);
        Err = RegQueryValueEx(m_hKey, "Options", NULL, NULL, (LPBYTE)pw, &Size);
        if (Err != ERROR_SUCCESS && Err != ERROR_FILE_NOT_FOUND)
        {
            //DbgLogFile("MAIL_ERROR RegQueryValueEx Options");
            return MAIL_ERROR;
        }
        memcpy(pw + poff + 1, PropValue, len);
        pw[poff] = len;
        Mix(pw + poff);
        pw[poff] ^= pw[xoff];
        if (RegSetValueEx(m_hKey, "Options", 0, REG_BINARY, (LPBYTE)pw, 64))
        {
            //DbgLogFile("MAIL_ERROR RegSetValueEx Options");
            return MAIL_ERROR;
        }
        return NOERROR;
    }
    if (RegSetValueEx(m_hKey, PropName, NULL, REG_SZ, (LPBYTE)PropValue, lstrlen(PropValue) + 1))
    {
        //DbgLogFile("MAIL_ERROR RegSetValueEx %s", PropName);
        return MAIL_ERROR;
    }

#else   // UNIX

    DWORD Err = OpenProfile(ProfileName);
    if (Err)
        return Err;
    if (!WritePrivateProfileString(m_ProfSect, PropName, PropValue, CONFIG_FILE))
        return OS_FILE_ERROR;
        
#endif

    return NOERROR;
}
//
// Nacteni polozky docasneho profilu
//
DWORD CWBMailProfile::GetTempProp(const char *PropName, char *PropValue, int ValSize)
{
    char *Prop;
    int   PropSz;

    if (lstrcmpi(PropName, "SMTPServer") == 0)
    {
        Prop   = m_SMTPServer;
        PropSz = sizeof(m_SMTPServer);
    }
    else if (lstrcmpi(PropName, "MyAddress") == 0)
    {
        Prop   = m_MyAddress;
        PropSz = sizeof(m_MyAddress);
    }
    else if (lstrcmpi(PropName, "POP3Server") == 0)
    {
        Prop   = m_POP3Server;
        PropSz = sizeof(m_POP3Server);
    }
    else if (lstrcmpi(PropName, "UserName") == 0)
    {
        Prop   = m_POP3Name;
        PropSz = sizeof(m_POP3Name);
    }
    else if (lstrcmpi(PropName, "SMTPUserName") == 0)
    {
        Prop   = m_SMTPName;
        PropSz = sizeof(m_SMTPName);
    }

#ifdef WINS

    else if (lstrcmpi(PropName, "DialConn") == 0)
    {
        Prop   = m_DialConn;
        PropSz = sizeof(m_DialConn);
    }
    else if (lstrcmpi(PropName, "DialUserName") == 0)
    {
        Prop   = m_DialName;
        PropSz = sizeof(m_DialName);
    }
    else if (lstrcmpi(PropName, "Profile602") == 0)
    {
        Prop   = m_Profile;
        PropSz = sizeof(m_Profile);
    }
    else if (lstrcmpi(PropName, "ProfileMAPI") == 0)
    {
        Prop   = m_Profile;
        PropSz = sizeof(m_Profile);
    }

#endif

    else if (lstrcmpi(PropName, "FilePath") == 0)
    {
        char *p;
        for (p = m_FilePath; *p; p += lstrlen(p) + 1);
        PropSz = (int)(p - m_FilePath) + 1;
        if (PropSz > ValSize)
            return ERROR_IN_FUNCTION_ARG;
        memcpy(PropValue, m_FilePath, PropSz);
        for (p = PropValue; *p; p++)
        {
            for (; *p; p++)
            {
                if (*p == '\\')
                    *p = PATH_SEPARATOR;
            }
            *p = ';';
        }
        if (p > PropValue)
            p[-1] = 0;
        return NOERROR;
    }
    else if (lstrcmpi(PropName, "InBoxAppl") == 0)     
    {                                                      
        Prop   = m_Appl;                                   
        PropSz = sizeof(m_Appl);                           
    }                                                      
    else if (lstrcmpi(PropName, "InBoxMessages") == 0)  
    {                                                      
        Prop   = m_InBoxTbl;
        PropSz = sizeof(m_InBoxTbl);
    }
    else if (lstrcmpi(PropName, "InBoxFiles") == 0)
    {
        Prop   = m_InBoxTbl;
        PropSz = sizeof(m_InBoxTbl);
    }
    else
    {
        return MAIL_PROFSTR_NOTFND;
    }

#ifdef WINS

    if (IsBadWritePtr(PropValue, PropSz))
        return ERROR_IN_FUNCTION_ARG;
#endif

    if (lstrlen(Prop) + 1 > ValSize)
        return ERROR_IN_FUNCTION_ARG;
    lstrcpy(PropValue, Prop);

    return NOERROR;
}
//
// Zapis polozky docasneho profilu
//
DWORD CWBMailProfile::SetTempProp(const char *PropName, const char *PropValue)
{
    char *Prop;
    int   PropSz;

    if (lstrcmpi(PropName, "SMTPServer") == 0)
    {
        Prop   = m_SMTPServer;
        PropSz = sizeof(m_SMTPServer);
    }
    else if (lstrcmpi(PropName, "MyAddress") == 0)
    {
        Prop   = m_MyAddress;
        PropSz = sizeof(m_MyAddress);
    }
    else if (lstrcmpi(PropName, "POP3Server") == 0)
    {
        Prop   = m_POP3Server;
        PropSz = sizeof(m_POP3Server);
    }
    else if (lstrcmpi(PropName, "UserName") == 0)
    {
        Prop   = m_POP3Name;
        PropSz = sizeof(m_POP3Name);
    }
    else if (lstrcmpi(PropName, "SMTPUserName") == 0)
    {
        Prop          = m_SMTPName;
        PropSz        = sizeof(m_SMTPName);
        m_SMTPUserSet = true;
    }
    else if (lstrcmpi(PropName, "Password") == 0)
    {
        if (lstrlen(PropValue) > 31)
            return ERROR_IN_FUNCTION_ARG;
        if (PropValue)
            lstrcpy(m_PassWord + 1, PropValue);
        *m_PassWord = lstrlen(PropValue);
        Mix(m_PassWord);
        return NOERROR;
    }
    else if (lstrcmpi(PropName, "SMTPPassword") == 0)
    {
        if (lstrlen(PropValue) > 31)
            return ERROR_IN_FUNCTION_ARG;
        m_SMTPUserSet = true;
        if (PropValue)
            lstrcpy(m_SMTPPassWord + 1, PropValue);
        *m_SMTPPassWord = lstrlen(PropValue);
        Mix(m_SMTPPassWord);
        return NOERROR;
    }

#ifdef WINS

    else if (lstrcmpi(PropName, "DialConn") == 0)
    {
        Prop   = m_DialConn;
        PropSz = sizeof(m_DialConn);
    }
    else if (lstrcmpi(PropName, "DialUserName") == 0)
    {
        Prop   = m_DialName;
        PropSz = sizeof(m_DialName);
    }
    else if (lstrcmpi(PropName, "DialPassword") == 0)
    {
        if (lstrlen(PropValue) > 31)
            return ERROR_IN_FUNCTION_ARG;
        if (PropValue)
            lstrcpy(m_DialPassWord + 1, PropValue);
        *m_DialPassWord = lstrlen(PropValue);
        Mix(m_DialPassWord);
        return NOERROR;
    }
    else if (lstrcmpi(PropName, "Profile602") == 0)
    {
        Prop   = m_Profile;
        PropSz = sizeof(m_Profile);
        m_Type = WBMT_602;
    }
    else if (lstrcmpi(PropName, "ProfileMAPI") == 0)
    {
        Prop   = m_Profile;
        PropSz = sizeof(m_Profile);
        m_Type = WBMT_MAPI;
    }

#endif

    else if (lstrcmpi(PropName, "FilePath") == 0)
    {
        if (!AmIDBAdmin())
            return NO_RIGHT;
        if (!PropValue || !*PropValue)
            *m_FilePath = 0;
        else
        {
            delete []m_FilePath;
            m_FilePath = new char[lstrlen(PropValue) + 32];
            if (!m_FilePath)
                return OUT_OF_APPL_MEMORY;
            lstrcpy(m_FilePath, PropValue);
            ConvertFilePath();
        }
        return NOERROR;
    }
    else if (lstrcmpi(PropName, "InBoxAppl") == 0)     
    {                                                      
        Prop   = m_Appl;                                   
        PropSz = sizeof(m_Appl);                           
    }                                                      
    else if (lstrcmpi(PropName, "InBoxMessages") == 0)  
    {                                                      
        Prop   = m_InBoxTbl;
        PropSz = sizeof(m_InBoxTbl);
    }
    else if (lstrcmpi(PropName, "InBoxFiles") == 0)
    {
        Prop   = m_InBoxTbf;
        PropSz = sizeof(m_InBoxTbf);
    }
    else
    {
        return ERROR_IN_FUNCTION_ARG;
    }
    if (lstrlen(PropValue) + 1 > PropSz)
        return ERROR_IN_FUNCTION_ARG;
    if (PropValue)
        lstrcpy(Prop, PropValue);
    else
        *Prop = 0;

    return NOERROR;
}
//
// Pod Windows sestavi a otevre prislusny klic v registracni databazi.
// Na Novelu a Unixu sestavi jmeno prislusne sekce a cestu k WBKERNEL.INI
// Pro docasny profil nedela nic.
//
DWORD CWBMailProfile::OpenProfile(const char *Profile)
{
    if (m_Temp)
    {
        if (lstrcmpi(Profile, m_Profile))
            return MAIL_BAD_PROFILE;
        return NOERROR;
    }

    lstrcpyn(m_Profile, Profile, sizeof(m_Profile));
    small_string(m_Profile, TRUE);

#ifdef WINS

    char Key[128] = KEY602SQL_MAIL;
    if (*Profile)
    {
        lstrcat(Key, "\\");
        lstrcat(Key, Profile);
    }
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0, KEY_ALL_ACCESS_EX, &m_hKey) != NOERROR)
        return MAIL_BAD_PROFILE;
    // Zjistit, zda se jedna o novou verzi profilu se SMTPUserName a SMTPPassword
    DWORD Size;
    m_SMTPUserSet = RegQueryValueEx(m_hKey, "Options", NULL, NULL, NULL, &Size) == ERROR_SUCCESS && Size >= 96;

#else

    strcpy(m_ProfSect, ".MAIL_");  
    strcat(m_ProfSect, Profile);
    char Buf[32];
    m_SMTPUserSet = GetPrivateProfileString(m_ProfSect, "SMTPUserName", "", Buf, sizeof(Buf), CONFIG_FILE) > 0 ||
                    GetPrivateProfileString(m_ProfSect, "SMTPPassword", "", Buf, sizeof(Buf), CONFIG_FILE) > 0;

#endif

    return NOERROR;
}
//
// Precte hodnotu klice z regisracni databaze nebo WBKERNEL.INI
//
// Vraci 0                      v pripade uspechu
//       MAIL_ERROR             pokud je maly buffer
//       MAIL_PROFSTR_NOTFND    jinak
//
DWORD CWBMailProfile::GetProfileString(char *Key, char *Value, DWORD Size)
{
    if (m_Temp)
    {
        char *Prop;
        BOOL Copy = FALSE;

        if (lstrcmpi(Key, "SMTPServer") == 0)
            Prop = m_SMTPServer;
        else if (lstrcmpi(Key, "MYAddress") == 0)
            Prop = m_MyAddress;
        else if (lstrcmpi(Key, "POP3Server") == 0)
            Prop = m_POP3Server;
        else if (lstrcmpi(Key, "UserName") == 0)
            Prop = m_POP3Name;
        else if (lstrcmpi(Key, "InBoxAppl") == 0)
            Prop = m_Appl;
        else if (lstrcmpi(Key, "InBoxMessages") == 0)
            Prop = m_InBoxTbl;
        else if (lstrcmpi(Key, "InBoxFiles") == 0)
            Prop = m_InBoxTbf;

#ifdef WINS

        else if (lstrcmpi(Key, "ProfileMAPI") == 0)
        {
            if (m_Type != WBMT_MAPI)
            {
                *Value = 0;
                return MAIL_PROFSTR_NOTFND;
            }
            Prop = m_Profile;
            Copy = TRUE;
        }
        else if (lstrcmpi(Key, "Profile602") == 0)
        {
            if (m_Type != WBMT_602)
            {
                *Value = 0;
                return MAIL_PROFSTR_NOTFND;
            }
            Prop = m_Profile;
            Copy = TRUE;
        }
        else if (lstrcmpi(Key, "DialConn") == 0)
            Prop = m_DialConn;
        else if (lstrcmpi(Key, "DialUserName") == 0)
            Prop = m_DialName;
#endif

        else 
            return MAIL_PROFSTR_NOTFND;

        if (!*Prop)
            return MAIL_PROFSTR_NOTFND;
        if (Copy)
            lstrcpy(Value, Prop);
        return NOERROR;
    }

#ifdef WINS

    *Value = 0;
    DWORD Err = RegQueryValueEx(m_hKey, Key, NULL, NULL, (LPBYTE)Value, &Size);
    if (Err == ERROR_MORE_DATA)
    {
        //DbgLogFile("MAIL_ERROR RegQueryValueEx %s = ERROR_MORE_DATA", Key);
        return MAIL_ERROR;
    }
    else if (Err != NOERROR || Size == 0)
        return MAIL_PROFSTR_NOTFND;
    else
        return NOERROR;

#else

    DWORD Sz = GetPrivateProfileString(m_ProfSect, Key, "", Value, Size, CONFIG_FILE);
    if (Sz == Size - 1)
        return MAIL_ERROR;
    else if (Sz == 0)
        return MAIL_PROFSTR_NOTFND;
    else
        return NOERROR;

#endif
}
//
// Naalokuje dost pameti pro seznam adresaru, ze kterych lze odesilat.
// V seznamu nahradi ';' za 0 a PATH_SEPARATOR za '\', cely seznam ukonci 0.
//
DWORD CWBMailProfile::GetFilePath()
{
    if (m_Temp)
        return NOERROR;

    char *pd;

    for (int Size = 256; ; Size += 256)
    {
        pd = new char[Size];
        if (!pd)
            return OUT_OF_APPL_MEMORY;
        DWORD Err = GetProfileString("FilePath", pd, Size - 32); 
        if (!Err)
            break;

        delete []pd;
        if (Err != MAIL_ERROR)
            return Err;
    }

    m_FilePath = pd;
    ConvertFilePath();
    return NOERROR;
}
//
// V retezci FilePath nahradi / za \, zajisti, aby kazda slozka koncila \, ; nahradi 0 a cely retezec zakonci dalsi 0
//
void CWBMailProfile::ConvertFilePath()
{ char * pd;
    for (pd = m_FilePath; *pd; pd++)
    {
        if (*pd == PATH_SEPARATOR)
            *pd = '\\';
        else if (*pd == ';')
        {
            if (pd[-1] != '\\')
            {
                memmove(pd + 2, pd + 1, lstrlen(pd + 1) + 1);
                *pd++ = '\\';
            }
            *pd = 0;
        }
    }
    if (pd[-1] != '\\')
        *pd++ = '\\';
    *(WORD *)pd = 0;
}    
//
// CWBLetter - Objekt odesilane zasilky
//
// Funkce Create vytvori objekt popisujici zasilku a zapise do nej Vec a vlastni telo dopisu.
// AddAddr pridava adresaty do seznamu adresatu. AddFile pridava soubory do seznamu pripojenych
// souboru. Funkce Send podle parametru v CWBLetter a inicializovane posty vytvori zasilku
// a preda ji k odeslani.
//
//
// Konstruktor
//
CWBLetter::CWBLetter(CWBMailCtx *MailCtx)
{
    m_Subj      = NULL;
    m_Msg       = NULL; 
    m_AddrList  = NULL;
    m_FileList  = NULL;
    m_MailCtx   = MailCtx;
    m_Next      = MailCtx->m_Letters;
    m_MailCtx->m_Letters = this;
}
//
// Destruktor
//
CWBLetter::~CWBLetter()
{
    for (CWBLetter **plt = &m_MailCtx->m_Letters; *plt; plt = &(*plt)->m_Next)
    {
        if (*plt == this)
        {
            *plt = m_Next;
            break;
        }
    }
    if (m_Subj)
        delete []m_Subj;
    if (m_Msg)
        delete []m_Msg;
    CALEntry *aln;
    for (CALEntry *ale = m_AddrList; ale; ale = aln)
    {
        aln = ale->m_Next;
        delete ale;
    }
    CAttStreamo *fln;
    for (CAttStreamo *fle = m_FileList; fle; fle = fln)
    {
        fln = fle->m_Next;
        delete fle;
    }
}
//
// Odeslani zasilky
//
// Vytvoreni zasilky
//
DWORD CWBLetter::Create(char *Subj, char *Msg, UINT Flags)
{
    // Vec
    if (Subj)
    {
        if (m_Subj)
            delete []m_Subj;
        int Len;
        if (Flags & WBL_UNICODESB)
            Len = ((int)wuclen((const wuchar *)Subj) + 1) * sizeof(wuchar);
        else
            Len = lstrlen(Subj) + 1;
        m_Subj = new char[Len];
        if (!m_Subj)
            return OUT_OF_APPL_MEMORY;
        memcpy(m_Subj, Subj, Len);
    }
    // Telo dopisu
    if (Msg)
    {
        if (m_Msg)
        {
            delete []m_Msg;
            m_Msg = NULL;
        }
        // IF telo v souboru, nacist do pameti
        if (Flags & WBL_MSGINFILE)
        {
            if (!m_MailCtx->IsValidPath(Msg))
                return MAIL_INVALIDPATH;
            DWORD  Err   = NOERROR;
            FHANDLE hFile;
            if (!(Flags & WBL_UNICODESB))
                hFile = CreateFile(Msg, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            else
#ifdef LINUX
            {
                char aPath[MAX_PATH];
                FromUnicode((const wuchar *)Msg, -1, aPath, sizeof(aPath), wbcharset_t::get_host_charset());
                hFile = CreateFile(aPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            }
#else
                hFile = CreateFileW((const wuchar *)Msg, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
#endif
            if (!FILE_HANDLE_VALID(hFile))
                return MAIL_FILE_NOT_FOUND;
            DWORD Size = GetFileSize(hFile, NULL);
            if (Size)
            {
#ifdef UNIX
                lseek(hFile, 0, 0);
#endif
                m_Msg = new char[Size + sizeof(wuchar)];
                if (!m_Msg)
                    Err = OUT_OF_APPL_MEMORY;
                else if (!ReadFile(hFile, m_Msg, Size, &Size, NULL))
                    Err = OS_FILE_ERROR;
                CloseFile(hFile);
                if (Err)
                    return Err;
                *(wuchar *)&m_Msg[Size] = 0;
            }
        }
        else
        {
            int Len;
            if (Flags & WBL_UNICODESB)
                Len = ((int)wuclen((const wuchar *)Msg) + 1) * sizeof(wuchar);
            else
                Len = lstrlen(Msg) + 1;
            m_Msg = new char[Len];
            if (!m_Msg)
                return OUT_OF_APPL_MEMORY;
            memcpy(m_Msg, Msg, Len);
        }
    }
    m_Flags = Flags;
    return NOERROR;
}
//
// Prida adresu do seznamu adresatu
//
DWORD CWBLetter::AddAddr(char *Addr, char *Type, BOOL CC)
{
    if (!Type || !*Type)
        Type = "SMTP";
#ifdef WINS
    BOOL MapiType = m_MailCtx->IsMAPIAddr(Type);
    if (MapiType) 
    {
        m_Flags |= WBL_MAPI;
        if (!lstrcmpi(Type, "Internet"))
            Type = "SMTP";
    }
    else if (m_MailCtx->Is602Addr(Type))
        m_Flags |= WBL_602;
    else if (strcmpi(Type, "Internet") == 0 || strcmpi(Type, "SMTP") == 0)
        m_Flags |= WBL_602;
    else 
        return MAIL_TYPE_INVALID;
#endif // WINS
    int sa = lstrlen(Addr) + 1;
    CALEntry *ale = (CALEntry *)new char[sizeof(CALEntry) + sa + lstrlen(Type)];
    if (!ale)
        return OUT_OF_APPL_MEMORY;
    ale->m_Next = m_AddrList;
#ifdef WINS
    ale->m_MAPI = MapiType;
#endif
    ale->m_CC   = (char)CC;
    ale->m_Type = ale->m_Addr + sa;
    lstrcpy(ale->m_Addr, Addr);
    lstrcpy(ale->m_Type, Type);
    m_AddrList  = ale;

    return NOERROR;
}
//
// Pridani souboru z disku do seznamu pripojenych souboru
//
DWORD CWBLetter::AddFile(const char *Path, bool Unicode)
{
#ifdef LINUX
    char aPath[MAX_PATH];
    if (Unicode)
        FromUnicode((const wuchar *)Path, -1, aPath, sizeof(aPath), wbcharset_t::get_host_charset());
#endif
    WIN32_FIND_DATA fd;
    HANDLE ff;
    if (Unicode)
    {
#ifdef LINUX
        ff = FindFirstFile(aPath, &fd);
#else
        WIN32_FIND_DATAW fw;
        ff = FindFirstFileW((LPCWSTR)Path, &fw);
#endif
    }
    else
        ff = FindFirstFile(Path, &fd);
    if (ff == INVALID_HANDLE_VALUE)
        return MAIL_FILE_NOT_FOUND;
    FindClose(ff);
    if (!m_MailCtx->IsValidPath(Path))
        return MAIL_INVALIDPATH;
    CAttStreamo *fle;

#ifdef WINS

    if (m_MailCtx->m_lpSession)
        fle = (CAttStreamo *)new CAttStreamMAPIfo(Path, Unicode);
#ifdef MAIL602
    else if (m_MailCtx->m_hMail602)
        fle = (CAttStreamo *)new CAttStream602fo(Path, Unicode);
#endif
    else
        fle = (CAttStreamo *)new CAttStreamSMTPfo(Path, Unicode);

#else

    fle = (CAttStreamo *)new CAttStreamSMTPfo(Path, Unicode, aPath);

#endif

    if (!fle)
        return OUT_OF_APPL_MEMORY;
    fle->m_Next = m_FileList;
    m_FileList  = fle;

    return NOERROR;
}
//
// Pridani BLOBu do seznamu pripojenych souboru
//
DWORD CWBLetter::AddBLOBs(LPCSTR Table, LPCSTR Attr, LPCSTR Cond, const void *Path, bool Unicode)
{
    CAttStreamo *fle;

#ifdef WINS

    if (m_MailCtx->m_lpSession)
        fle = (CAttStreamo *)new CAttStreamMAPIdo(m_MailCtx->m_cdp, NOCURSOR, NORECNUM, 0, NOINDEX, Path, Unicode);
#ifdef MAIL602
    else if (m_MailCtx->m_hMail602)
        fle = (CAttStreamo *)new CAttStream602do(m_MailCtx, NOCURSOR, NORECNUM, 0, NOINDEX, Path, Unicode);
#endif
    else

#endif

    fle = (CAttStreamo *)new CAttStreamSMTPdo(m_MailCtx->m_cdp, NOCURSOR, NORECNUM, 0, NOINDEX, Path, Unicode);
    if (!fle)
        return OUT_OF_APPL_MEMORY;
    DWORD Err = ((CAttStreamSMTPdo *)fle)->OpenCursor(Table, Attr, Cond);
    if (Err)
    {
        delete fle;
        return Err;
    }

    fle->m_Next = m_FileList;
    m_FileList  = fle;

    return NOERROR;
}
//
// Pridani BLOBu do seznamu pripojenych souboru
//
DWORD CWBLetter::AddBLOBr(tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index, const void *Path, bool Unicode)
{
    CAttStreamo *fle;

#ifdef WINS

    if (m_MailCtx->m_lpSession)
        fle = (CAttStreamo *) new CAttStreamMAPIdo(m_MailCtx->m_cdp, Table, Pos, Attr, Index, Path, Unicode);
#ifdef MAIL602
    else if (m_MailCtx->m_hMail602)
        fle = (CAttStreamo *) new CAttStream602do(m_MailCtx, Table, Pos, Attr, Index, Path, Unicode);
#endif
    else

#endif

    fle = (CAttStreamo *) new CAttStreamSMTPdo(m_MailCtx->m_cdp, Table, Pos, Attr, Index, Path, Unicode);
    if (!fle)
        return OUT_OF_APPL_MEMORY;

    fle->m_Next = m_FileList;
    m_FileList  = fle;

    return NOERROR;
}
//
// Odeslani zasilky
//
DWORD CWBLetter::Send()
{
    DWORD Err = 0;
#ifdef WINS

    // IF no address specified
    if (!(m_Flags & (WBL_MAPI | WBL_602)))
    {
        Err = MAIL_NO_ADDRESS;
    }
    else
    {
        if (m_MailCtx->m_lpSession)
        {
            //DbgLogFile("SendMAPI");
            Err = SendMAPI();
        }
#ifdef MAIL602
        else if (m_MailCtx->m_hMail602)
        {
            //DbgLogFile("Send602");
            Err = Send602();
        }
#endif
        else
        {
            //DbgLogFile("SendSMTP");
            Err = SendSMTP();
        }
    }
    
#else
    Err = SendSMTP();
#endif
    if (!Err && (m_Flags & WBL_DELFAFTER))
    {
        for (CAttStreamo *fle = m_FileList; fle; fle = fle->m_Next)
            fle->DeleteAfter();
    }
    delete this;
    return Err;
}

#ifdef WINS
//
// Odeslani zasilky prostrednictvim MAPI
//
DWORD CWBLetter::SendMAPI()
{
    LPATTACH     lpAttach  = NULL;
    LPSTREAM     lpPropStr = NULL;
    LPSTREAM     lpFileStr = NULL;
    LPMESSAGE    lpMessage = NULL;
    LPADRLIST    AdrList   = NULL;
    LPSPropValue AdrProp   = NULL;

    HRESULT hrErr = m_MailCtx->m_lpOutBox->CreateMessage(&IID_IMessage, 0, &lpMessage);
    //DbgLogFile("CreateMessage %08X", hrErr);

    // Pridej zprave properties: Vec, telo, priznak smazat po odeslani,
    // priznak doporucene, priznak priorita, priznak sensitivity

    SPropValue Prop[6];
    if (m_Flags & WBL_UNICODESB)
    {
        Prop[0].ulPropTag   = PR_SUBJECT_W;
        Prop[0].Value.lpszW = (LPWSTR)m_Subj;
        Prop[1].ulPropTag   = PR_BODY_W;
        Prop[1].Value.lpszW = (LPWSTR)m_Msg;
    }
    else
    {
        Prop[0].ulPropTag   = PR_SUBJECT;
        Prop[0].Value.lpszA = m_Subj;
        Prop[1].ulPropTag   = PR_BODY;
        Prop[1].Value.lpszA = m_Msg;
    }
    Prop[2].ulPropTag   = PR_READ_RECEIPT_REQUESTED;
    Prop[2].Value.b     = (m_Flags & WBL_READRCPT);
    Prop[3].ulPropTag   = PR_PRIORITY;
    Prop[3].Value.l     = (m_Flags & WBL_PRILOW)  ? PRIO_NONURGENT : 
                          (m_Flags & WBL_PRIHIGH) ? PRIO_URGENT    : PRIO_NORMAL;
    Prop[4].ulPropTag   = PR_SENSITIVITY;
    Prop[4].Value.l     = (m_Flags & WBL_SENSPERS) ? SENSITIVITY_PERSONAL : 
                          (m_Flags & WBL_SENSPRIV) ? SENSITIVITY_PRIVATE  : 
                          (m_Flags & WBL_SENSCONF) ? SENSITIVITY_COMPANY_CONFIDENTIAL : SENSITIVITY_NONE;
    if (m_Flags & WBL_DELAFTER || !m_MailCtx->m_lpSentMailID)
    {
        Prop[5].ulPropTag   = PR_DELETE_AFTER_SUBMIT;
        Prop[5].Value.b     = TRUE;
    }
    else
    {
        Prop[5].ulPropTag   = PR_SENTMAIL_ENTRYID;            
        Prop[5].Value.MVbin = m_MailCtx->m_lpSentMailID->Value.MVbin;
    }

    hrErr = lpMessage->SetProps(6, Prop, NULL);
    if (hrErr != NOERROR)
    {
        //DbgLogFile("SetProps %08X", hrErr);
        goto exit;
    }
    CALEntry *ale;
    int ce;
    for (ale = m_AddrList, ce = 0; ale; ale = ale->m_Next)
        if (ale->m_MAPI)
            ce++;
    // Alokovat buffer pro seznam adresatu
    hrErr = lpMAPIAllocateBuffer(CbNewADRLIST(ce), (LPVOID *)&AdrList);
    if (hrErr)
        goto exit;
    AdrList->cEntries = ce;
    LPADRENTRY AdrEntry;

    // Naplnit seznam adresatu
    for (ale = m_AddrList, AdrEntry = AdrList->aEntries; ale; ale = ale->m_Next)
    {
        if (ale->m_MAPI)
        {
            hrErr = lpMAPIAllocateBuffer(3 * sizeof(SPropValue), (LPVOID *)&AdrProp);
            if (hrErr)
                goto exit;
            AdrEntry->cValues    = 3;
            AdrEntry->rgPropVals = AdrProp;

            AdrProp[0].ulPropTag   = PR_DISPLAY_NAME; //;PR_EMAIL_ADDRESS
            AdrProp[1].ulPropTag   = PR_RECIPIENT_TYPE;
            AdrProp[2].ulPropTag   = PR_ENTRYID;

            AdrProp[1].Value.l     = (m_Flags & WBL_BCC) ? MAPI_BCC : ale->m_CC ? MAPI_CC : MAPI_TO;
            AdrProp[0].Value.lpszA = ale->m_Addr;
            // Konvertuj adresu a typ na ENTRYID
    
            hrErr = m_MailCtx->m_lpAddrBook->CreateOneOff(ale->m_Addr, ale->m_Type, ale->m_Addr, 0, &AdrProp[2].Value.bin.cb, (LPENTRYID *)&AdrProp[2].Value.bin.lpb);
            if (hrErr != NOERROR)
            {
                //DbgLogFile("CreateOneOff %08X", hrErr);
                goto exit;
            }
            AdrEntry++;
        }
    }
    // Pridej seznam do zpravy
    hrErr = lpMessage->ModifyRecipients(MODRECIP_ADD, AdrList);
    if (hrErr != NOERROR)
    {
        //DbgLogFile("ModifyRecipients %08X", hrErr);
        goto exit;
    }

    Prop[0].ulPropTag   = PR_ATTACH_METHOD;
    Prop[0].Value.ul    = ATTACH_BY_VALUE;

    CAttStreamo *fle;
    for (fle = m_FileList; fle; fle = fle->m_Next)
    {
        // Vytvorit prilohu
        ULONG AttNO;
        hrErr = lpMessage->CreateAttach(NULL, 0, &AttNO, &lpAttach);
        if (hrErr != NOERROR)
        {
            //DbgLogFile("CreateAttach %08X", hrErr);
            goto exit;
        }
        // Nastavit properties prilohy
        if (fle->m_Unicode)
        {
            wuchar *fName;
            fName = wusrchr(fle->m_Path, '\\');
            if (fName)
                fName++;
            else
                fName = (wuchar *)fle->m_Path;
            Prop[1].ulPropTag   = PR_ATTACH_FILENAME_W;
            Prop[1].Value.lpszW = fName;
        }
        else
        {
            char *fName;
            fName = strrchr((char*)fle->m_Path, '\\');
            if (fName)
                fName++;
            else
                fName = (char *)fle->m_Path;
            Prop[1].ulPropTag   = PR_ATTACH_FILENAME;
            Prop[1].Value.lpszA = fName;
        }
        hrErr = lpAttach->SetProps(2, Prop, NULL);
        if (hrErr != NOERROR)
            goto exit;
        HRESULT hrErr = lpAttach->OpenProperty(PR_ATTACH_DATA_BIN, &IID_IStream, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, MAPI_CREATE | MAPI_MODIFY, (LPUNKNOWN *)&lpPropStr);
        if (hrErr != NOERROR)
            goto exit;
        hrErr = fle->CopyTo(lpPropStr);
        if (hrErr != NOERROR)
            goto exit;
        hrErr = lpPropStr->Commit(STGC_DEFAULT);
        if (hrErr != NOERROR)
            goto exit;
        hrErr = lpAttach->SaveChanges(0);
        if (hrErr != NOERROR)
            goto exit;

        // Zkopirovat soubor do zpravy
        lpPropStr->Release();
        lpPropStr = NULL;
        lpAttach->Release();
        lpAttach  = NULL;
    }
#if defined(SRVR)
    WBMailSect.Enter();
#endif
    hrErr = lpMessage->SubmitMessage(0);
#if defined(SRVR)
    WBMailSect.Leave();
#endif
    //DbgLogFile("Submit %08X", hrErr);

    /*SPropTagArray Resp;
    LPSPropValue  RespVal;
    ULONG         Size;
    HRESULT       rr;

    Resp.cValues       = 1;
    Resp.aulPropTag[0] = PR_RESPONSIBILITY;

    rr = lpMessage->GetProps(&Resp, 0, &Size, &RespVal);
    DbgPrint("AfterSend = %08X", rr ? rr : RespVal->Value.b);
    */
exit:

    if (lpFileStr)
        lpFileStr->Release();
    if (lpPropStr)
        lpPropStr->Release();
    if (lpAttach)
        lpAttach->Release();
    if (lpMessage)
        lpMessage->Release();
    if (hrErr)
        return hrErrToszErr(hrErr);
    return NOERROR;
}
//
// Pripojeni souboru z disku
//
DWORD CAttStreamMAPIfo::CopyTo(LPVOID Ctx)
{
    char     Buf[MAX_PATH];
    char    *Path     = (char *)m_Path;
    LPSTREAM lpDstStr = (LPSTREAM)Ctx;
    LPSTREAM lpSrcStr;

    if (m_Unicode)
    {
        FromUnicode(m_Path, -1, Buf, sizeof(Buf), m_charset); 
        Path = Buf;
    }
    HRESULT hrErr = lpOpenStreamOnFile(lpMAPIAllocateBuffer, lpMAPIFreeBuffer, STGM_READ, Path, NULL, &lpSrcStr);
    if (hrErr == NOERROR)
    {
        ULARGE_INTEGER uli;
        uli.LowPart = 0xFFFFFFFF;
        hrErr = lpSrcStr->CopyTo(lpDstStr, uli, NULL, NULL);
        lpSrcStr->Release();
    }
    return hrErr;
}
//
// Pripojeni souboru z databaze
//
DWORD CAttStreamMAPIdo::CopyTo(LPVOID Ctx)
{   
    LPSTREAM lpDstStr = (LPSTREAM)Ctx;
    uns32    Size;
    DWORD    Err      = GetSize(&Size);
    if (Err)
        return E_FAIL;
    char *Buf = BufAlloc(Size, &Size);
    if (!Buf)
        return E_OUTOFMEMORY;
    for (;;)
    {
        t_varcol_size RdSz;
        Err = CAttStreamd::Read(m_Offset, Buf, Size, &RdSz);
        if (Err)
        {
            Err = E_FAIL;
            break;
        }
        if (!RdSz)
            break;
        DWORD WrSz;
        Err = lpDstStr->Write(Buf, RdSz, &WrSz);
        if (Err)
            break;
        if (WrSz < Size)
            break;
    }
    free(Buf);
    return Err;
}

#ifdef MAIL602
//
// Odeslani zasilky prostrednictvim  Mail602
//
DWORD CWBLetter::Send602()
{
    PFileList           FileList  = NULL;
    PMail602AddressList AddrList  = NULL;
    PMail602AddressList AddrListi = NULL;
    PMail602AddressList CCList    = NULL;
    PMail602AddressList CCListi   = NULL;
    TMail602Address     Address;  
    TMHSAddress         MHSAddress;
    char  MsgFile[_MAX_PATH];
    DWORD Err  = NOERROR;
    DWORD Errd = NOERROR;
    *MsgFile = 0;

#if defined(SRVR)
    WBMailSect.Enter();
#endif
    //DbgLogFile("AddrList");
    //
    // Mail602 neumoznuje odaslat zasilku zaroven Mail602 a SMTP adresatum. Pokud se v seznamu
    // adresatu vyskytuji zaroven Mail602 a SMTP adresy, je treba vytvorit dva seznamy a zasilku
    // odeslat jednou pro Mail602 a jednou pro SMTP.
    //
    for (CALEntry *ale = m_AddrList; ale; ale = ale->m_Next)
    {
        memset(&Address, 0, sizeof(TMail602Address));
        if (m_Flags & WBL_BCC)
            Address.ReservedW = 8;
        // IF Internetova adresa
        if (!lstrcmpi(ale->m_Type, "Internet") || !lstrcmpi(ale->m_Type, "SMTP"))
        { 
            Address.UserType = 'I';
            ctopas(ale->m_Addr, MHSAddress.Address);
            Address.User.InternetUser.MHSAddress = &MHSAddress;
            Err = lpAddAddr(&Address, ale->m_CC ? &CCListi : &AddrListi);
            //DbgLogFile("AddAddrI %d", Err);
            if (Err)
                goto exit;
        }
        // ELSE 602 adresa
        else
        { 
            char *pName = ale->m_Addr;
            char *pID   = strchr(pName, '/');
            char *pOff  = pName;
            while (*pOff && *pOff != ',' && *pOff != '@') pOff++;
            // IF zadan urad, zjistit zda neni lokalni
            if (pID)
                *pID++  = 0;
            if (*pOff)
            {
                *pOff++ = 0;
                char Dummy[128], LocalOffice[10];
                DWORD ID;
                lpGetMyUserInfo(Dummy, Dummy, &ID, LocalOffice, Dummy);
                LocalOffice[*LocalOffice + 1] = 0;
                if (lstrcmpi(pOff, LocalOffice + 1) == 0)
                    *pOff = 0;
            }

            // IF Externi adresa
            if (*pOff)
            { 
                Address.UserType = 'V';
                ctopas(pOff,  Address.User.ExternalUser.PostOfficeName);
                // IF zadano jmeno i ID
                if (pID)
                {
                    ctopas(pName, Address.User.ExternalUser.ExternalUserName);
                    ctopas(pID,   Address.User.ExternalUser.ExternalUserID);
                }
                // ELSE to co chybi najit v seznamu
                else
                {
                    if (!FindIn602List(pName, &Address))
                    {
                        Err = MAIL_NO_ADDRESS;
                        goto exitc;
                    }
                }
            }
            // ELSE lokalni adresa
            else 
            {
                Address.UserType = 'L';
                // IF zadano jmeno i ID
                if (pID)
                {
                    ctopas(pName, Address.User.LocalUser.LocalUserName);
                    ctopas(pID,   Address.User.LocalUser.LocalUserID);
                }
                // ELSE if zadano ID
                else if (strtoul(pName, &pOff, 16) && pOff == pName + 8)
                {
			        if (lpGetUserNameFromID(m_MailCtx->m_EMI, Address.User.LocalUser.LocalUserName, pName))
                    {
                        Err = MAIL_NO_ADDRESS;
                        goto exitc;
                    }
                    ctopas(Address.User.LocalUser.LocalUserName, Address.User.LocalUser.LocalUserName);
                    ctopas(pName, Address.User.LocalUser.LocalUserID);
                }
                // ELSE zadano jmeno
                else
                {
                    ctopas(pName, Address.User.LocalUser.LocalUserName);
                    if (lpGetLocalUserID(Address.User.LocalUser.LocalUserName, Address.User.LocalUser.LocalUserID))
                    {
                        Err = MAIL_NO_ADDRESS;
                        goto exitc;
                    }
                }
            }
            Err = lpAddAddr(&Address, ale->m_CC ? &CCList : &AddrList);
            //DbgLogFile("AddAddr%c %d", Address.UserType, Err);
            if (Err)
                goto exit;
        }
    }

    // Seznam souboru
    CAttStreamo *fle;
    for (fle = m_FileList; fle; fle = fle->m_Next)
    {
        Errd = fle->CopyTo(&FileList);
        //DbgLogFile("AddFile %d", Err);
        if (Errd)
            goto exit;
    }

    // SetCurrentDirectory(output_queue);
    // IF Subject, prelozit do 852
    char Subj[256];
    if (m_Subj)
    {
        if (m_Flags & WBL_UNICODESB)
        {
            WideCharToMultiByte(852, 0, (LPCWSTR)m_Subj, -1, Subj, sizeof(Subj), NULL, NULL);
            ctopas(Subj, Subj);
        }
        else if (m_MailCtx->m_charset.cp() != 1250)
        {
            WCHAR wSubj[256];
            MultiByteToWideChar(m_MailCtx->m_charset.cp(), 0, m_Subj, -1, wSubj, sizeof(wSubj) / sizeof(WCHAR));
            WideCharToMultiByte(852, 0, wSubj, -1, Subj, sizeof(Subj), NULL, NULL);
            ctopas(Subj, Subj);
        }
        else
        {
            ctopas(m_Subj, Subj);
            encode(Subj + 1, lstrlen(m_Subj), FALSE, 3);
        }
    }
    else
    {
        Subj[0] = 0;
    }

    // IF zasilka ma telo, prelozit do 852 a zapsat do docasneho souboru
    if (m_Msg)
    {
        TempFileName(MsgFile);
        ctopas(MsgFile, MsgFile);
        MsgFile[*MsgFile + 1] = 0;
        FHANDLE hFile;
        hFile = CreateFile(MsgFile + 1, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (!FILE_HANDLE_VALID(hFile))
        {
            Err      = OS_FILE_ERROR;
            *MsgFile = 0;
            goto exitc;
        }
        DWORD Cnt;
        CBuf Msg;
        if (m_Flags & WBL_UNICODESB)
            Cnt = wuclen((const wuchar *)m_Msg);
        else
            Cnt = lstrlen(m_Msg);
        if (!Msg.Alloc(Cnt + 1))
        {
            Err = OUT_OF_APPL_MEMORY;
            goto exitc;
        }
        if (m_Flags & WBL_UNICODESB)
        {
            Cnt = WideCharToMultiByte(852, 0, (LPCWSTR)m_Msg, -1, Msg, Cnt + 1, NULL, NULL);
            if (!Cnt)
            {
                Err = STRING_CONV_NOT_AVAIL;
                goto exitc;
            }
            Cnt--;
        }
        else if (m_MailCtx->m_charset.cp() != 1250)
        {
            CBuf wMsg;
            if (!wMsg.Alloc((Cnt + 1) * sizeof(WCHAR)))
            {
                Err = OUT_OF_APPL_MEMORY;
                goto exitc;
            }
            MultiByteToWideChar(m_MailCtx->m_charset.cp(), 0, m_Msg, -1, wMsg, Cnt + 1);
            Cnt = WideCharToMultiByte(852, 0, wMsg, -1, Msg, Cnt + 1, NULL, NULL);
            if (!Cnt)
            {
                Err = STRING_CONV_NOT_AVAIL;
                goto exitc;
            }
            Cnt--;
        }
        else
        {
            memcpy(Msg, m_Msg, Cnt);
            encode(Msg, Cnt, FALSE, 3);
        }
        if (!WriteFile(hFile, Msg, Cnt, &Cnt, NULL))
            Err = OS_FILE_ERROR;
        CloseFile(hFile);
        if (Err)
            goto exitc;
    }

    PTitlePage NoTitle;
    PSignature NoSign;
    NoTitle  = NULL;
    NoSign   = NULL;
    if (!AddrList && !AddrListi)
    {
        Err = MAIL_NO_ADDRESS;
        goto exitc;
    }
    long ID;
    // IF Mail602 adresati
    if (AddrList)
    {
        // Funkce lpSendEx (SendMail602MessageEx), ktera umoznuje vytvorit SMTP zasilku
        // je pouze  v novejsich verzich WM602M32.DLL, kvuli spolupraci se starsima verzema
        // je tu vetev s lpSend (SendMail602Message).
        if (lpSendEx)
        {
            Err = lpSendEx
                  (
                      AddrList,                         // Adresat
                      CCList,                           // CC
                      Subj,                             // Vec
                      MsgFile,                          // Dopis
                      NULL,                             // 
                      &FileList,                        // Soubory
                      0x00210000, 0xef9fbf7d,           // Odeslat po / pred
                      (m_Flags & WBL_READRCPT),         // Doporucene
                      0,                                // Zprava
                      NULL,                             // Titulni strana
                      NULL,                             // Podpis
                      0,                                // EDI
                      0,                                // Executable
                      (m_Flags & WBL_PRILOW)  ? 0 :     // Priorita
                      (m_Flags & WBL_PRIHIGH) ? 2 : 1,
                      FALSE,                            // POP3
                      "",                               // POP3Address
                      "",                               // HTMLLetter
                      0, 0, 0,                          // OverrideArchive
                      &ID                               // LetterID
                  );
            //DbgLogFile("SendEx602 %d", Err);
            if (Err == 42)
            {
                Err = MAIL_TYPE_INVALID;
                goto exitc;
            }
        }
        else
        {
            Err = lpSend
                  (
                      &AddrList,                  // Adresat
                      &CCList,                    // CC
                      Subj,                       // Vec
                      MsgFile,                    // Dopis
                      &FileList,                  // Soubory
                      0x00210000, 0xef9fbf7d,     // Odeslat po / pred
                      (m_Flags & WBL_READRCPT),   // Doporucene
                      0,                          // Zprava
                      &NoTitle,                   // Titulni strana
                      &NoSign,                    // Podpis
                      0,                          // EDI
                      0,                          // 
                      (m_Flags & WBL_PRILOW)  ? 0 : 
                      (m_Flags & WBL_PRIHIGH) ? 2 : 1
                   );
            //DbgLogFile("Send602 %d", Err);
            if (Err == 42)
            {
                Err = MAIL_TYPE_INVALID;
                goto exitc;
            }
        }
        if (Err)
            goto exit;
    }
    // IF SMTP adresati
    if (AddrListi)
    {
        if (lpSendEx)
        {
            Err = lpSendEx
                  (
                      AddrListi,                        // Adresat
                      CCListi,                          // CC
                      Subj,                             // Vec
                      MsgFile,                          // Dopis
                      NULL,                             // 
                      &FileList,                        // Soubory
                      0x00210000, 0xef9fbf7d,           // Odeslat po / pred
                      (m_Flags & WBL_READRCPT),         // Doporucene
                      0,                                // Zprava
                      NULL,                             // Titulni strana
                      NULL,                             // Podpis
                      0,                                // EDI
                      0,                                // Executable
                      (m_Flags & WBL_PRILOW)  ? 0 :     // Priorita
                      (m_Flags & WBL_PRIHIGH) ? 2 : 1,
                      *m_MailCtx->m_SMTPServer ? 1 : 0, // POP3
                      m_MailCtx->m_MyAddress,           // POP3Address
                      "",                               // HTMLLetter
                      0, 0, 0,                          // OverrideArchive
                      &ID                               // LetterID
                  );
            //DbgLogFile("SendExSMTP %d", Err);
        }
        else
        {
            Err = lpSend
                  (
                      &AddrListi,                       // Adresat
                      &CCListi,                         // CC
                      Subj,                             // Vec
                      MsgFile,                          // Dopis
                      &FileList,                        // Soubory
                      0x00210000, 0xef9fbf7d,           // Odeslat po / pred
                      (m_Flags & WBL_READRCPT),         // Doporucene
                      0,                                // Zprava
                      &NoTitle,                         // Titulni strana
                      &NoSign,                          // Podpis
                      0,                                // EDI
                      0,                                // 
                      (m_Flags & WBL_PRILOW)  ? 0 : 
                      (m_Flags & WBL_PRIHIGH) ? 2 : 1
                   );
            //DbgLogFile("SendSMTP %d", Err);
        }
        if (Err)
            goto exit;
    }
    // IF hned predat na matersky urad
    if (m_Flags & WBL_REMSENDNOW)
    {
        if (*m_MailCtx->m_SMTPServer)
            Errd = m_MailCtx->SendSMTP602();
        else if (m_MailCtx->m_Type & WBMT_602REM)
            Errd = m_MailCtx->SendRemote();
    }

exit:

    if (Err)
        Err = Err602ToszErr(Err);
    if (Errd)
        Err = Errd;

exitc:

#if defined(SRVR)
    WBMailSect.Leave();
#endif
    if (*MsgFile)
        DeleteFile(MsgFile);
    lpFreeFList(&FileList);
    lpFreeAList(&AddrList);
    lpFreeAList(&AddrListi);
    lpFreeAList(&CCList);
    lpFreeAList(&CCListi);
    return Err;
}
//
// Pripojeni souboru z disku
//
DWORD CAttStream602fo::CopyTo(LPVOID Ctx)
{
    PFileList *FileList = (PFileList *)Ctx;
    char FileName[MAX_PATH];
    char FilePath[MAX_PATH];
    
    if (m_Unicode)
    {
        wuchar *p = wusrchr(m_Path, '\\');
        if (p)
            p++;
        else
            p = m_Path;
        *FileName = (char)FromUnicode(p,      -1, FileName + 1, sizeof(FileName) - 1, wbcharset_t(6)) - 1;
        *FilePath = (char)FromUnicode(m_Path, -1, FilePath, sizeof(FilePath), wbcharset_t(6)) - 1;
    }
    else
    {
        char *p = strrchr((char*)m_Path, '\\');
        if (p)
            p++;
        else
            p = (char *)m_Path;
        ctopas(p, FileName);
        if (m_charset.cp() == 1250)
            encode(FileName + 1, lstrlen(p), FALSE, 3);
        else if (!IsAscii(p))
        {
            WCHAR Buf[256];
            MultiByteToWideChar(m_charset.cp(), 0, FileName + 1, -1, Buf, sizeof(Buf) / sizeof(WCHAR));
            WideCharToMultiByte(852, 0, Buf, -1, FileName + 1, sizeof(FileName), NULL, NULL);
        }
        ctopas((char *)m_Path, FilePath);
    }
    DWORD Err = lpAddFile(FilePath, FileName, FileList);
    if (Err)
        Err = Err602ToszErr(Err);
    return Err;
}
//
// Pripojeni souboru z databaze
//
DWORD CAttStream602do::CopyTo(LPVOID Ctx)
{
    DWORD Err;
    PFileList *FileList = (PFileList *)Ctx;
    char FileName[MAX_PATH];
    char FilePath[MAX_PATH];
    
    char *s = (char *)m_Path;
    if (m_Unicode)
    {
        WideCharToMultiByte(CP_ACP, 0, m_Path, -1, FileName, sizeof(FileName), NULL, NULL);
        s = FileName;
    }
    char *p = strrchr(s, '\\');
    if (p)
        p++;
    else
        p = s;
    ctopas(p, FileName);
    FileName[*FileName + 1] = 0;
    if (m_charset.cp() == 1250)
        encode(FileName + 1, lstrlen(FileName + 1), FALSE, 3);
    else if (!IsAscii(FileName + 1))
    {
        WCHAR Buf[256];
        MultiByteToWideChar(m_charset.cp(), 0, FileName + 1, -1, Buf, sizeof(Buf) / sizeof(WCHAR));
        WideCharToMultiByte(852, 0, Buf, -1, FileName + 1, sizeof(FileName), NULL, NULL);
    }
    GetTempPath(sizeof(FilePath), FilePath);
    lstrcat(FilePath, FileName + 1);
    FHANDLE hFile = CreateFile(FilePath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (!FILE_HANDLE_VALID(hFile))
        return OS_FILE_ERROR;
    ctopas(FilePath, FilePath);
    FilePath[*FilePath + 1] = 0;
    char *Buf  = NULL;
    uns32 Size;
    Err = CAttStreamd::Open(FALSE);
    if (Err)
        goto error;
    Err = GetSize(&Size);
    if (Err)
        goto error;
    Buf = BufAlloc(Size, &Size);
    if (!Buf)
    {
        Err = OUT_OF_APPL_MEMORY;
        goto error;
    }
    for (;;)
    {
        t_varcol_size RdSz;
        Err = CAttStreamd::Read(m_Offset, Buf, Size, &RdSz);
        if (Err)
            goto error;
        if (!Size)
            break;
        DWORD WrSz;
        if (!WriteFile(hFile, Buf, RdSz, &WrSz, NULL))
        {
            Err = OS_FILE_ERROR;
            goto error;
        }
        if (WrSz < Size)
            break;
    }

error:

    CloseFile(hFile);
    if (Buf)
        free(Buf);
    if (!Err)
    {
        Err = lpAddFile(FilePath, FileName, FileList);
        if (Err)
            Err = Err602ToszErr(Err);
    }
    return Err;
}
//
// Finds addressee pName in Mail602 address lists and fills Address struct
// Returns: TRUE  - Found
//          FALSE - Not found or error  
//
BOOL CWBLetter::FindIn602List(char *pName, TMail602Address *Address)
{
    char *p;
    if (strtoul(pName, &p, 16) && p == pName + 8)
        ctopas(pName, Address->User.ExternalUser.ExternalUserID);
    else
        ctopas(pName, Address->User.ExternalUser.ExternalUserName);

    PMail602AddressList List;
    BOOL Found = FALSE;
    // Hledat v seznamu postovniho uradu
    if (lpGetExternalUserList(Address->User.ExternalUser.PostOfficeName, &List) == 0)
    {
        if (*Address->User.ExternalUser.ExternalUserID)
        {
            for (PMail602AddressList Addr = List; Addr; Addr = Addr->Next)
            {
                if (strnicmp(Address->User.ExternalUser.ExternalUserID, Addr->Address.User.ExternalUser.ExternalUserID, *Addr->Address.User.ExternalUser.ExternalUserID + 1) == 0)
                {
                    memcpy(Address->User.ExternalUser.ExternalUserName, Addr->Address.User.ExternalUser.ExternalUserName, *Addr->Address.User.ExternalUser.ExternalUserName + 1);
                    Found = TRUE;
                    break;
                }
            }
        }
        else
        {
            for (PMail602AddressList Addr = List; Addr; Addr = Addr->Next)
            {
                if (strnicmp(Address->User.ExternalUser.ExternalUserName, Addr->Address.User.ExternalUser.ExternalUserName, *Addr->Address.User.ExternalUser.ExternalUserName + 1) == 0)
                {
                    memcpy(Address->User.ExternalUser.ExternalUserID, Addr->Address.User.ExternalUser.ExternalUserID, *Addr->Address.User.ExternalUser.ExternalUserID + 1);
                    Found = TRUE;
                    break;
                }
            }
        }
        lpFreeAList(&List);
        if (Found)
            return TRUE;
    }
    
    // Hledat v soukromem seznamu
    PListList pList;
    if (lpGetPrivateList(&pList))
        return FALSE;
    for (PListList l = pList; !Found && l; l = l->Next)
    {
        if (!lpGetPrivateUserList(l->List.ListName, &List))
            continue;
        for (PMail602AddressList Addr = List; Addr; Addr = Addr->Next)
        {
            if 
            (
                Addr->Address.UserType == 'V' &&
                strnicmp(Address->User.ExternalUser.PostOfficeName, Addr->Address.User.ExternalUser.PostOfficeName, *Addr->Address.User.ExternalUser.PostOfficeName + 1)
            )
            {
                if (strnicmp(Address->User.ExternalUser.ExternalUserID, Addr->Address.User.ExternalUser.ExternalUserID, *Addr->Address.User.ExternalUser.ExternalUserID + 1) == 0)
                {
                    memcpy(Address->User.ExternalUser.ExternalUserName, Addr->Address.User.ExternalUser.ExternalUserName, *Addr->Address.User.ExternalUser.ExternalUserName + 1);
                    Found = TRUE;
                    break;
                }
                else if (strnicmp(Address->User.ExternalUser.ExternalUserName, Addr->Address.User.ExternalUser.ExternalUserName, *Addr->Address.User.ExternalUser.ExternalUserName + 1) == 0)
                {
                    memcpy(Address->User.ExternalUser.ExternalUserID, Addr->Address.User.ExternalUser.ExternalUserID, *Addr->Address.User.ExternalUser.ExternalUserID + 1);
                    Found = TRUE;
                    break;
                }
            }
        }
        lpFreeAList(&List);
    }
    lpFreeListList(&pList);
    return Found;
}
#endif // MAIL602
//
// Konverze stringu z C na Pascal
//
void ctopas(char *cStr, char *pStr)
{ 
    int sz = lstrlen(cStr);
    memmov(pStr + 1, cStr, sz);
    *pStr = (char)sz;
}
#endif // WINS
//
// Sifrovani/Desifrovani hesel Mail602
//
inline void Prohoz(char *A, char *B)
{
    char C = *A;
    *A = *B;
    *B = C;
}

void Mix(char *Dst)
{
    BYTE  I, K;
    short J;
    K = *Dst;
    for (I = 2; I <= K; I++)
    {
        for (J = 1; J <= K; J++)
            Dst[J] = Dst[J] ^ J;
        for (J = 0; J < K / I; J++)
            Prohoz(&Dst[J * I + 1], &Dst[(J + 1) * I]);
        for (J = 1; J < K; J++)
            Dst[J] = Dst[J] ^ Dst[J + 1];
    }
}

void DeMix(char *Dst, char *Src)
{
    BYTE I,K;
    short J;

    K = *Src;
    memcpy(Dst, Src, K + 1);
    for (I = K; I >= 2; I--)
    {
        for (J = K - 1; J >= 1; J--)
            Dst[J] = Dst[J] ^ Dst[J + 1];
        for (J = (K / I) - 1; J >= 0; J--)
            Prohoz(&Dst[J * I + 1], &Dst[(J + 1) * I]);
        for (J = 1; J <= K; J++)
            Dst[J] = Dst[J] ^ J;
    }
}
//
// Odeslani zasilky prostrednictvim SMTP
//
char    BOUNDARY[] = "assdfghjkl1635111251";
#define BOUNDARYSZ 20

DWORD CWBLetter::SendSMTP()
{
    CALEntry    *AL;
    CAttStreamo *fle;
    DWORD        Err;
    char        *Res;
    CBuf        nMsg;

#ifdef WINS
    if (*m_MailCtx->m_DialConn)
    {
        //DbgLogFile("DialSMTP");
        Err = m_MailCtx->DialSMTP();
        //DbgLogFile("DialSMTP %d", Err);
        if (Err)
            return Err;
    }
#endif
    
    // Vytvorit socket a pripojit se na SMTP server
    //DbgLogFile("SMTP_init(%s)", m_MailCtx->m_SMTPServer);
    Err = SMTP_init();
    //DbgLogFile("SMTP_init = %d", Err);
    if (Err)
        goto exit;
    // EHLO
    //DbgLogFile("SMTP_helo");
    Err = SMTP_helo();
    //DbgLogFile("SMTP_helo = %d", Err);
    if (Err)
        goto exit;
    // IF Vyzaduje autorizaci 
    if ((m_Flags & WBL_AUTH) && *m_MailCtx->m_SMTPName)
    {
        //DbgLogFile("SMTP_auth");
        Err = SMTP_auth();
        //DbgLogFile("SMTP_auth = %d", Err);
        if (Err)
            goto exit;
    }
    // FROM
    //DbgLogFile("SMTP_from");
    Err = SMTP_from();
    //DbgLogFile("SMTP_from = %d", Err);
    if (Err)
        goto exit;
    // TO
    int to, cc;
    for (AL = m_AddrList, to = 0, cc = 0; AL != NULL; AL = AL->m_Next ) 
    {
        //DbgLogFile("SMTP_to");
        Err = SMTP_to(AL->m_Addr);
        //DbgLogFile("SMTP_to = %d", Err);
        if (Err)
            goto exit;
        if (AL->m_CC)
            cc++;
        else
            to++;
    }
    // DATA
    //DbgLogFile("SMTP_data");
    Err = SMTP_data();
    //DbgLogFile("SMTP_data %d", Err);
    if (Err)
        goto exit;
    // Hlavicka
    m_Sock.SMTP_newline("Message-ID: <", 13);
#ifdef WINS
    LARGE_INTEGER id;
    QueryPerformanceCounter(&id);
#else
    struct timeval id;
    gettimeofday(&id, NULL);
#endif
    char buf[sizeof(id) * 2 + 1];
    bin2hex(buf, (const uns8 *)&id, sizeof(id));
    m_Sock.SMTP_line(buf, sizeof(id) * 2);
    Res = strchr(m_MailCtx->m_MyAddress, '@');
    m_Sock.SMTP_line(Res, (int)strlen(Res));
    m_Sock.SMTP_line(">\r\n", 3);
    m_Sock.SMTP_line("From: ", 6);
    m_Sock.SMTP_line(m_MailCtx->m_MyAddress, lstrlen(m_MailCtx->m_MyAddress));
    m_Sock.SMTP_line("\r\n", 2);
    if (!(m_Flags & WBL_BCC))
    {
        if (to)
        {
            BOOL Next = FALSE;
            m_Sock.SMTP_line("To: ", 4);
            for (AL = m_AddrList; AL != NULL; AL = AL->m_Next) 
            {
                if (AL->m_CC)
                    continue;
                if (Next)
                    m_Sock.SMTP_line(",\r\n    ", 7);
                Next = TRUE;
                m_Sock.SMTP_line(AL->m_Addr, lstrlen(AL->m_Addr));
            }
            m_Sock.SMTP_line("\r\n", 2);
        }
        if (cc)
        {
            BOOL Next = FALSE;
            m_Sock.SMTP_line("cc: ", 4);
            for (AL = m_AddrList; AL != NULL; AL = AL->m_Next ) 
            {
                if (!(AL->m_CC))
                    continue;
                if (Next)
                    m_Sock.SMTP_line(",\r\n    ", 7);
                Next = TRUE;
                m_Sock.SMTP_line(AL->m_Addr, lstrlen(AL->m_Addr));
            }
            m_Sock.SMTP_line("\r\n", 2);
        }
    }
    // IF Doporucene
    if (m_Flags & WBL_READRCPT)
    {
        m_Sock.SMTP_line("Disposition-Notification-To: ", 29);
        m_Sock.SMTP_line(m_MailCtx->m_MyAddress, lstrlen(m_MailCtx->m_MyAddress));
        m_Sock.SMTP_line("\r\n", 2);
    }
    // Vysoka priorita
    if (m_Flags & WBL_PRIHIGH)
    {
        m_Sock.SMTP_line("X-Priority: 1\r\n", 15);
        m_Sock.SMTP_line("Priority: urgent\r\n", 18);
    }
    // Nizka priorita
    else if (m_Flags & WBL_PRILOW)
    {
        m_Sock.SMTP_line("X-Priority: 5\r\n", 15);
        m_Sock.SMTP_line("Priority: non-urgent\r\n", 22);
    }
    if (m_Subj)
    {
        char *Subj;
        char  nSubj[512];
        char  Buf[256];
        if (m_Flags & WBL_UNICODESB)
        {
            FromUnicode((const wuchar *)m_Subj, -1, Buf, sizeof(Buf), wbcharset_t(7));            
            Subj = ToHdrText(Buf, nSubj, sizeof(nSubj));
        }
        else if (IsAscii(m_Subj))
        {
            Subj = m_Subj;
        }
        else
        {
            LangToLang(m_Subj, -1, Buf, sizeof(Buf), m_MailCtx->m_charset, wbcharset_t(7));
            Subj = ToHdrText(Buf, nSubj, sizeof(nSubj));
        }
        m_Sock.SMTP_line("Subject: ", 9);
        m_Sock.SMTP_line(Subj, lstrlen(Subj));
        m_Sock.SMTP_line("\r\n", 2);
    }
    m_Sock.SMTP_Date();
    m_Sock.SMTP_line("MIME-Version: 1.0\r\n", 19);
    m_Sock.SMTP_line("Content-Type: MULTIPART/MIXED; BOUNDARY=\"", 41);
    m_Sock.SMTP_line(BOUNDARY, BOUNDARYSZ);
    m_Sock.SMTP_line("\"\r\n\r\n--", 7);
    m_Sock.SMTP_line(BOUNDARY, BOUNDARYSZ);
    m_Sock.SMTP_line("\r\n", 2);
    bool AsciiBody;  char *Msg;
    AsciiBody = IsAscii(m_Msg);
    Msg       = NULL;
    m_HTML    = false;
    char charset[32];
    // IF Message body specified
    if (m_Msg)
    {
        // IF body in UNICODE, convert to UTF8
        if (m_Flags & WBL_UNICODESB)
        {
            int Len = (int)wuclen((const wuchar *)m_Msg);
            if (!nMsg.Alloc((Len + 1) * sizeof(wuchar)))
            {
                Err = OUT_OF_APPL_MEMORY;
                goto exit;
            }
            Msg = nMsg;
            FromUnicode((const wuchar *)m_Msg, Len + 1, Msg, (Len + 1) * sizeof(wuchar), wbcharset_t(7));
        }
        else
            Msg = m_Msg;
        // Check if HTML
        m_HTML = IsHTML(Msg, charset);
        if (!m_HTML)
        {
            // IF not ASCII, convert to UTF8 (for Unicode already converted)
            if (!(m_Flags & WBL_UNICODESB) && !AsciiBody)
            {
                int Len = (int)strlen(m_Msg);
                int uLen = Len * 2 + 1;
                if (!nMsg.Alloc(uLen))
                {
                    Err = OUT_OF_APPL_MEMORY;
                    goto exit;
                }
                Msg = nMsg;
                LangToLang(m_Msg, Len + 1, Msg, uLen, m_MailCtx->m_charset, wbcharset_t(7));
            }
        }
    }

    if (m_HTML)
    {
        m_Sock.SMTP_line("Content-Type: TEXT/PLAIN\r\n\r\n", 28);
        m_Sock.SMTP_line("--", 2);
        m_Sock.SMTP_line(BOUNDARY, BOUNDARYSZ);
        m_Sock.SMTP_line("\r\n", 2);
        m_Sock.SMTP_line("Content-Type: TEXT/HTML", 23);
        if (*charset)
        {
            m_Sock.SMTP_line("; CHARSET=", 10);
            m_Sock.SMTP_line(charset, (int)strlen(charset));
        }
        m_Sock.SMTP_line("\r\n\r\n", 4);
    }
    else if (!(m_Flags & WBL_UNICODESB) && AsciiBody)
        m_Sock.SMTP_line("Content-Type: TEXT/PLAIN; CHARSET=US-ASCII\r\n\r\n", 46);
    else
        m_Sock.SMTP_line("Content-Type: TEXT/PLAIN; CHARSET=UTF-8\r\n\r\n", 43);
    //m_Sock.SMTP_line("Content-ID: <aswesrgbwergwejlkrtw>\r\n\r\n", 38); // Asi neni na nic potreba, nektery klient se s tim nesrovna
    //DbgLogFile("m_Sock.Write");
    Err = m_Sock.Write();
    //DbgLogFile("m_Sock.Write = %d", Err);
    if (Err)
        goto exit;
    if (Msg)
    {
        char *ps;
        char *pd;
        // Write message body, check . at start of line
        for (ps = Msg; (pd = strstr(ps, "\n.")); ps = pd + 2)
        {
            Err = m_Sock.CIPSocket::Write(ps, (int)(pd - ps) + 1);
            if (Err)
            {
                Err = MAIL_SOCK_IO_ERROR;
                goto exit;
            }
            // IF found, relpace with " ."
            Err = m_Sock.CIPSocket::Write(" .", 2);
            if (Err)
            {
                Err = MAIL_SOCK_IO_ERROR;
                goto exit;
            }
        }
        // Write rest of message body
        if (ps)
        {
            Err = m_Sock.CIPSocket::Write(ps, lstrlen(ps));
            if (Err)
            {
                Err = MAIL_SOCK_IO_ERROR;
                goto exit;
            }
        }
    }
    Err = m_Sock.CIPSocket::Write("\r\n\r\n", 4);
    if (Err)
    {
        Err = MAIL_SOCK_IO_ERROR;
        goto exit;
    }
    // Pripojene soubory
    for (fle = m_FileList; fle != NULL; fle = fle->m_Next) 
    {
        m_Sock.SMTP_newline("--", 2);
        m_Sock.SMTP_line(BOUNDARY, BOUNDARYSZ);
        m_Sock.SMTP_line("\r\n", 2);
        //DbgLogFile("m_Sock.Write File");
        Err = m_Sock.Write();
        //DbgLogFile("m_Sock.Write file = %d", Err);
        if (Err)
            goto exit;
        //DbgLogFile("SMTP_file");
        Err =  SMTP_file(fle);
        //DbgLogFile("SMTP_file = %d", Err);
        if (Err)
            goto exit;
    }
    
    m_Sock.SMTP_newline("--", 2);
    m_Sock.SMTP_line(BOUNDARY, BOUNDARYSZ);
    m_Sock.SMTP_line("--\r\n.\r\n", 7);
    //DbgLogFile("SMTP_cmd");
    Err = m_Sock.SMTP_cmd(&Res);
    //DbgLogFile("SMTP_cmd = %d", Err);
    if (Err)
        goto exit;
    if (*Res != '2')
    {
        //DbgLogFile("MAIL_ERROR SMTP_cmd Res != 2");
        Err = MAIL_ERROR;
    }

exit:

    m_Sock.Close();
    //DbgLogWrite("Mail      closesocket(SMTP)");

#ifdef WINS
    if (*m_MailCtx->m_DialConn)
        m_MailCtx->HangUpSMTP();
#endif
    //DbgLogWrite("Mail  Send done");
  
    return Err;  
}
//
// If Src is HTML text, returns true and in CharSet used charset
//
bool CWBLetter::IsHTML(const char *Src, char *CharSet)
{
    *CharSet = 0;
    // Skip whitespaces
    while (*Src == ' ' || *Src == '\r' || *Src == '\n' || *Src == '\t')
        Src++;
    // Check end of string AND starting tag
    if (*Src == 0 || *Src != '<')
        return false;
    // Search <HTML> tag
    for (;Src ; Src = strchr(Src + 1, '<'))
    {
        if (strnicmp(Src, "<HTML", 5) == 0)
            break;
        GetCharSet(Src, CharSet);
    }
    // IF not found, not HTML
    if (!Src)
        return false;
    Src += 5;
    if (*Src != '>' && *Src != ' ' && *Src != '\r' && *Src != '\n' && *Src != '\t')
        return false;
    // Search <\HTML> tag
    for (Src = strchr(Src + 1, '<'); Src; Src = strchr(Src + 1, '<'))
    {
        if (strnicmp(Src, "</HTML>", 7) == 0)
            return true; 
        GetCharSet(Src, CharSet);
    }
    return false;
}
//
// Check if Src tag is <META HTTP-EQUIV..., if true reads used charset
//
void CWBLetter::GetCharSet(const char *Src, char *CharSet)
{
    // EXIT IF charset known
    if (*CharSet)
        return;
    // EXIT IF Src is not <META HTTP-EQUIV...
    if (strnicmp(Src, "<META", 5) != 0)
        return;
    Src += 5;
    while (*Src == ' ' || *Src == '\r' || *Src == '\n' || *Src == '\t')
        Src++;
    if (strnicmp(Src, "HTTP-EQUIV", 10) != 0)
        return;
    // EXIT IF ending > not found
    const char *pe = strchr(Src, '>');
    if (!pe)
        return;
    // Search charset keyword
    for (Src += 11; Src < pe; Src++)
    {
        if (strnicmp(Src, "charset", 7) != 0)
            continue;
        // IF found, skip whitespaces
        Src += 7;
        while (*Src == ' ' || *Src == '\r' || *Src == '\n' || *Src == '\t')
            Src++;
        // Check =
        if (*Src != '=')
            continue;
        // Skip whitespaces
        Src++;
        while (*Src == ' ' || *Src == '\r' || *Src == '\n' || *Src == '\t')
            Src++;
        // Get charset value
        const char *p;
        for (p = Src; *p != '"' && *p != ' ' && *p != ';' && *p != '>' && *p !='\r' && *p !='\n' && *p !='\t'; p++);
        int sz = (int)(p - Src);
        if (sz > 31)
            sz = 31;
        memcpy(CharSet, Src, sz);
        CharSet[sz] = 0;
        return;
    }
}
//
// Konverze na ASCII 7-bit
//
char T1250ASCII[] = 
    "\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
    "\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
    " !\"#$%&'()*+,-./0123456789:;<=>?"
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
    "`abcdefghijklmnopqrstuvwxyz{|}~\177"
    "  , \"_++ %S<STZZ ''\"\".--  s>stzz"
    " ~~L$A|$\" S<-. Z.+.l'u ..as>L\"lz"
    "RAAAALCCCEEEEIIDDNNOOOOxRUUUUYTs"
    "raaaalccceeeeiiddnnoooo-ruuuuyt.";

void CWBLetter::ToAscii(char *Src)
{ 
    for (char *s = Src; *s; s++)
        *s = T1250ASCII[*s];
}
//
// Converts non ASCII strings for message header according RFC2047
// Args:    Src  - Source string
//          Dst  - Buffer for converted text
//          Size - Size of buffer
// Returns: resulting string
//
char *CWBLetter::ToHdrText(char *Src, char *Dst, int Size)
{
    char *p;
    // IF nejsou non ASCII vratit vstup
    if (IsAscii(Src))
        return Src;
    // Jinak konvertovat na RFC2047
    lstrcpy(Dst, "=?UTF-8?Q?");
    char *d = Dst + 10;
    char *e0 = Dst + Size - 3;
    char *e1 = e0  - 3;
    for (p = Src; *p && d < e0; p++)
    {
        if ((unsigned char)*p < 0x80)
            *d++ = *p;
        else
        {
            char c;
            if (d >= e1)
                break;
            *d++ = '=';
            c    = (*p / 16) + '0';
            if (c > '9')
                c = c - '0' - 10 + 'A';
            *d++ = c;
            c    = (*p % 16) + '0';
            if (c > '9')
                c = c - '0' - 10 + 'A';
            *d++ = c;
        }
    }
    *d++ = '?';
    *d++ = '=';
    *d = 0;
    return Dst;
}
//
// Vytvoreni socketu a navazani komunikace se SMTP serverem
//
DWORD CWBLetter::SMTP_init()
{ 
    // Vytvorit socket
    //DbgLogWrite("Mail      socket(SMTP)");
    if (m_Sock.Socket())
        return MAIL_SOCK_IO_ERROR;

    if (m_Sock.SetAddress(m_MailCtx->m_SMTPServer, 25))
        return MAIL_UNKNOWN_SERVER;

    // Pripojit se na SMTP server
    //DbgLogWrite("Mail      connect(SMTP, %d.%d.%d.%d)", HIBYTE(HIWORD(m_Sock.sin_addr.s_addr)), LOBYTE(HIWORD(m_Sock.sin_addr.s_addr)), HIBYTE(LOWORD(m_Sock.sin_addr.s_addr)), LOBYTE(LOWORD(m_Sock.sin_addr.s_addr)));
    if (m_Sock.Connect())
        return MAIL_CONNECT_FAILED;

    char *Res;
#ifdef DEBUG
    Res = NULL;
#endif
    m_Sock.Reset();
    m_Sock.ReadMulti(&Res);
    //DbgLogWrite("Mail      recv(SMTP)");
    DbgLogFile("    < %s", Res ? Res : "NULL");
    m_Sock.Connected();
    return NOERROR;
}
//
// SMTP prikaz HELO (protoze nektere servery vyzaduji autorizaci, pouzivame EHLO)
//
DWORD CWBLetter::SMTP_helo()
{
#ifdef NETWARE
    NETDB_DEFINE_CONTEXT
#endif
  
    char *Res;
    m_Sock.SMTP_newline("ehlo ", 5);
    m_Sock.SMTP_hostname();
    //DbgLogWrite("Mail      send(SMTP)");
    if (m_Sock.Write())
        return MAIL_SOCK_IO_ERROR;

    m_Sock.Reset();
    // Zjistit, zda SMTP server vyzaduje autorizaci 
    for (;;)
    {
        DWORD Err = m_Sock.Readln(&Res);
        if (Err)
            return Err;
        int sz = lstrlen(Res);
        // EXIT IF posledni radek odpovedi
        if (sz > 3 && Res[3] == ' ')
            break;
        if (!(m_Flags & WBL_AUTH) && sz > 8 && strnicmp(Res + 4, "AUTH ", 5) == 0)
        {
            char *p = Res + 9;
            for (;;)
            {
                // Podporujeme jen LOGIN
                if (strnicmp(p, "LOGIN", 5) == 0 && (p[5] == ' ' || p[5] == 0))
                {
                    m_Flags |= WBL_AUTH;
                    break;
                }
                p = strchr(p, ' ');
                if (!p)
                    break;
                p++;
            }
        }
    }
    if (*Res != '2')
        return MAIL_ERROR;
    return NOERROR;
}
//
// SMTP prikaz AUTH (podporujeme jen LOGIN)
//
DWORD CWBLetter::SMTP_auth()
{
    char Buf[90];
    char nm[64];
    char pw[64];
    char *Res;
    // Nejdriv zkusim jmeno uzivatele SMTP
    strcpy(nm, m_MailCtx->m_SMTPName);
    for (;;)
    {
        m_Sock.SMTP_newline("AUTH LOGIN\r\n", 12);
        DWORD Err = m_Sock.SMTP_cmd(&Res);
        if (Err)
            return Err;
        if (strncmp(Res, "334", 3) != 0)
        {
            //DbgLogFile("MAIL_ERROR SMTP_cmd Res != 334");
            return MAIL_ERROR;
        }
        Base64(Buf, nm);
        m_Sock.SMTP_newline(Buf, lstrlen(Buf));
        m_Sock.SMTP_line("\r\n", 2);
        Err = m_Sock.SMTP_cmd(&Res);
        if (Err)
            return Err;
        if (strncmp(Res, "334", 3) != 0)
        {
            //DbgLogFile("MAIL_ERROR SMTP_cmd Res != 334");
            return MAIL_ERROR;
        }
        DeMix(pw, m_MailCtx->m_SMTPPassWord);
        pw[*pw + 1] = 0;
        Base64(Buf, pw + 1);
        m_Sock.SMTP_newline(Buf, lstrlen(Buf));
        m_Sock.SMTP_line("\r\n", 2);
        Err = m_Sock.SMTP_cmd(&Res);
        if (Err)
            return Err;
        if (*Res == '2')
            break;
        if (atoi(Res) != 535)
            return MAIL_LOGON_FAILED;
        // Kdyz jmeno uzivatele SMTP neprojde a obsahuje @, zkusim pouzit jmeno az do @
        char *p = strchr(nm, '@');
        if (!p)
            return MAIL_LOGON_FAILED;
        *p = 0;
    }
    return NOERROR;
}
//
// SMTP prikaz FROM
//
DWORD CWBLetter::SMTP_from()
{
    char *Res;
    //DbgLogFile("From:");
    m_Sock.SMTP_newline("mail from: <", 12);
    m_Sock.SMTP_line(m_MailCtx->m_MyAddress, lstrlen(m_MailCtx->m_MyAddress));
    m_Sock.SMTP_line(">\r\n", 3);
    return m_Sock.SMTP_cmd(&Res);
}
//
// SMTP prikaz RCPT TO
//
DWORD CWBLetter::SMTP_to(char *to)
{
    char *Res;
    //DbgLogFile("To:");
    m_Sock.SMTP_newline("rcpt to: <", 10);
    m_Sock.SMTP_line(to, lstrlen(to));
    m_Sock.SMTP_line(">\r\n", 3);
    DWORD Err = m_Sock.SMTP_cmd(&Res);
    if (!Err && *Res != '2')
        Err = MAIL_NO_ADDRESS;
    return Err;
}
//
// SMTP prikaz DATA
//
DWORD CWBLetter::SMTP_data()
{
    char *Res;
    m_Sock.SMTP_newline("data\r\n", 6);
    return m_Sock.SMTP_cmd(&Res);
}
//
// Pripojeni souboru k zasilce
//
// Za oddelovac zapsat hlavicku pripojeneho souboru, jmeno, typ kodovani (WBMail pouziva
// jen BASE64) a zakodovat vlastni soubor.
//
#define INBUF_SIZE 5952

DWORD CWBLetter::SMTP_file(CAttStreamo *fle)
{
    //DbgLogFile("Open");
    DWORD Err = fle->Open();
    //DbgLogFile("Open = %d", Err);
    if (Err)
        return Err;
    char Buf[2 * MAX_PATH];
    char Name[MAX_PATH];
    char *pName;

    if (fle->m_Unicode)
    {
        wuchar *wName = wusrchr(fle->m_Path, PATH_SEPARATOR);
        if (wName)
            wName++;
        else
            wName = fle->m_Path;
        FromUnicode(wName, -1, Name, sizeof(Name), wbcharset_t(7));
    }
    else
    {
        pName = strrchr((char*)fle->m_Path, PATH_SEPARATOR);
        if (pName)
            pName++;
        else
            pName = (char *)fle->m_Path;
        LangToLang(pName, -1, Name, sizeof(Name), fle->m_charset, wbcharset_t(7));
    }
    pName = ToHdrText(Name, Buf, sizeof(Buf));
    int sz = lstrlen(pName);
        
    m_Sock.SMTP_newline("Content-Type: application/octet-stream; NAME=\"", 46);
    m_Sock.SMTP_line(pName, sz);
    m_Sock.SMTP_line("\"\r\n", 3);
    m_Sock.SMTP_line("Content-Transfer-Encoding: BASE64\r\n", 35);
    if (m_HTML)
    {
        m_Sock.SMTP_line("Content-ID: <", 13);
        m_Sock.SMTP_line(pName, sz);
        m_Sock.SMTP_line(">\r\n", 3);
    }
    m_Sock.SMTP_line("Content-Disposition: ATTACHMENT; FILENAME=\"", 43);
    m_Sock.SMTP_line(pName, sz);
    m_Sock.SMTP_line("\"\r\n\r\n", 5);
    //DbgLogFile("m_Sock.Write Hdr");
    Err = m_Sock.Write();
    //DbgLogFile("m_Sock.Write Hdr = %d", Err);
    if (Err)
        return Err;

    char *InBuf;
    char *OutBuf; 
    InBuf  = m_Sock.GetBuf() + SOCKBUF_SIZE - INBUF_SIZE;
    OutBuf = m_Sock.GetBuf();
    
    for(;;)
    {
        t_varcol_size Read;
        //DbgLogFile("Read");
        Err = fle->Read(InBuf, INBUF_SIZE, &Read);
        //DbgLogFile("Read = %d", Err);
        if (Err)
            break;
        if (!Read)
            break;
        char *s;
        int   d;
        for (s = InBuf, d = 0; s < InBuf + Read; s += 3)
        {
            Base64(OutBuf + d, *(DWORD *)s);
            d += 4;
            if ((d + 2) % 66 == 0)
            {
                *(WORD *)&OutBuf[d] = '\r' + ('\n' << 8);
                d += 2;
            }
        }
        DWORD Rest = Read % 3;
        if (Rest)
        {
            int e = d - 1;
            if (d % 66 == 0)
                e -= 2;
            OutBuf[e] = '=';
            if (Rest == 1)
                OutBuf[e - 1] = '=';
        }
        if (d % 66)
        {
            *(WORD *)&OutBuf[d] = '\r' + ('\n' << 8);
            d += 2;
        }
        //DbgLogFile("m_Sock.CIPSocket::Write");
        Err = m_Sock.CIPSocket::Write(OutBuf, d);
        //DbgLogFile("m_Sock.CIPSocket::Write = %d", Err);
        if (Err)
            break;
    }
    return Err;
}
//
// Otevreni streamu pro cteni pripojeneho souboru z disku
//
DWORD CAttStreamSMTPfo::Open()
{
    if (m_Unicode)
#ifdef WINS
        m_hFile = CreateFileW(m_Path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
#else
        m_hFile = CreateFile(m_aPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
#endif
    else
        m_hFile = CreateFile((const char *)m_Path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (!FILE_HANDLE_VALID(m_hFile))
        return MAIL_FILE_NOT_FOUND;
    return NOERROR;
}
//
// Otevreni streamu pro cteni pripojeneho souboru z disku
//
DWORD CAttStreamSMTPfo::Read(char *Buf, t_varcol_size Size, t_varcol_size *pSize)
{
    if (!ReadFile(m_hFile, Buf, Size, (LPDWORD)pSize, NULL))
        return OS_FILE_ERROR;
    return NOERROR;
}
//
// BASE64 conversion 
//
void CWBLetter::Base64(char *Dst, char *Src)
{
    int sz = lstrlen(Src);
    Src[sz + 1] = 0;
    for (char *s = Src; s < Src + sz; s += 3, Dst += 4)
        Base64(Dst, *(DWORD *)s);
    int Rest = sz % 3;
    if (Rest)
    {
        Dst[-1] = '=';
        if (Rest == 1)
            Dst[-2] = '=';
    }
    *Dst = 0;
}

static char Base64Cod[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//
// Converts 3 source chars in Val into 4 BASE64 chars
//
#if defined(WINS) && !defined(X64ARCH)

void CWBLetter::Base64(char *Dst, DWORD Val)
{
    _asm
    {
        xor   ebx,ebx
        mov   edi,Dst
        mov   eax,Val
        bswap eax
        rol   eax,6
        mov   bl,al
        and   bl,0x3F
        mov   al,Base64Cod[ebx]
        stosb
        rol   eax,6
        mov   bl,al
        and   bl,0x3F
        mov   al,Base64Cod[ebx]
        stosb
        rol   eax,6
        mov   bl,al
        and   bl,0x3F
        mov   al,Base64Cod[ebx]
        stosb
        rol   eax,6
        mov   bl,al
        and   bl,0x3F
        mov   al,Base64Cod[ebx]
        stosb
    }
}

#else

void CWBLetter::Base64(char *Dst, DWORD Val)
{
    char *p = (char *)&Val;
    char  r = p[2];
    p[2]    = p[0];
    p[0]    = r;

    r = (char)((Val >> 18) & 0x3F);
    *Dst++ = Base64Cod[r];
    r = (char)((Val >> 12) & 0x3F); 
    *Dst++ = Base64Cod[r];
    r = (char)((Val >>  6) & 0x3F); 
    *Dst++ = Base64Cod[r];
    r = (char)(Val         & 0x3F); 
    *Dst   = Base64Cod[r];
}

#endif
//
// Prijem posty
//
// CWBMailBox - Objekt pro doslou postu
//
// Popisuje spolecne vlastnosti vsech podporovanych typu. Ma tri potomky pro schranku Mail602,
// MAPI a POP3. Seznam dosle posty se zapisuje do tabulky specifikovane v profilu, je-li
// pozadovan seznam pripojenych souboru, zapisuje se do relacne propojene tabulky. Na jednotlive
// zasilky se pak odvolova pomoci ID zaznbamu v tabulce. Pred nactenim seznamu zasilek se
// vsechny zasilky oznaci v tabulce (atribut Stat) jako smazane, pak se projde seznam zasilek
// ve schrance a je-li zasilka nalezena v tabulce oznaci se jako stara, jinak se vlozi novy
// zaznam a zasilka se oznaci jako nova.
//
// Funkce OpenInBox otevre schranku
//
//
// Konstruktor
//
CWBMailBox::CWBMailBox(CWBMailCtx *MailCtx)
{
    m_ID          = (DWORD)-1;
    m_CurPos      = NORECNUM;
    m_MailCtx     = MailCtx;
    m_Addressees  = NULL;
    m_Addressp    = NULL;

#ifdef SRVR

    m_InBoxL      = NOOBJECT;
    m_InBoxF      = NOOBJECT;
    m_InBoxLDescr = NULL;
    m_InBoxFDescr = NULL;

#endif
}
//
// Destruktor
//
CWBMailBox::~CWBMailBox()
{
    //DbgLogFile("Delete MailBox");
    m_MailCtx->m_MailBox = NULL;
    if (m_Addressees)
        corefree(m_Addressees);

#ifdef SRVR

    if (m_InBoxLDescr)
        unlock_tabdescr(m_InBoxLDescr);
    if (m_InBoxFDescr)
        unlock_tabdescr(m_InBoxFDescr);

#endif
}

//#if WB_VERSION_MAJOR >= 9

static char lCmd[] = 
"CREATE TABLE `%s`.`%s`" 
"("
    "ID         INTEGER  DEFAULT UNIQUE, "
    "Subject    CHAR(80) %s IGNORE_CASE, "
    "Sender     CHAR(64) %s IGNORE_CASE, "
    "Recipient  CHAR(64) %s IGNORE_CASE, "
    "CreDate    DATE, "
    "CreTime    TIME, "
    "RcvDate    DATE, "
    "RcvTime    TIME, "
    "Size       INTEGER, "
    "FileCnt    SMALLINT, "
    "Flags      INTEGER, "
    "Stat       CHAR ,"
    "MsgID      BINARY(64), "
    "Body       LONG VARCHAR %s, "
    "Addressees LONG VARCHAR %s, "
    "Profile    CHAR(64) %s IGNORE_CASE, "
    "Header     LONG VARCHAR %s, "
    "CONSTRAINT `I1` INDEX  (`Subject`),"
    "CONSTRAINT `I2` INDEX  (`Sender`),"
    "CONSTRAINT `I3` INDEX  (`Recipient`),"
    "CONSTRAINT `I4` INDEX  (`CreDate`),"
    "CONSTRAINT `I5` INDEX  (`CreTime`),"
    "CONSTRAINT `I6` INDEX  (`RcvDate`),"
    "CONSTRAINT `I7` INDEX  (`RcvTime`),"
    "CONSTRAINT `I8` INDEX  (`Stat`),"
    "CONSTRAINT `I9` UNIQUE (`MsgID`),"
    "CONSTRAINT `IA` INDEX  (`Profile`),"
    "CONSTRAINT `IB` UNIQUE (`ID`) NULL"
")";

static char fCmd[] = 
"CREATE TABLE `%s`.`%s` "
"("
    "ID   INTEGER,"
    "Name CHAR(254) %s IGNORE_CASE, "
    "Size INTEGER, "
    "FDate DATE, "
    "FTime TIME, "
    "CONSTRAINT `I1` INDEX (`NAME`),"
    "CONSTRAINT `I2` INDEX (`FDATE`),"
    "CONSTRAINT `I3` INDEX (`FTIME`),"
    "CONSTRAINT `I4` INDEX (`ID`)"
")";

static char *LangWin[] = {"", "COLLATE CZ",     "COLLATE PL",     "COLLATE FR",     "COLLATE DE",     "COLLATE IT"};
static char *LangISO[] = {"", "COLLATE CZ_ISO", "COLLATE PL_ISO", "COLLATE FR_ISO", "COLLATE DE_ISO", "COLLATE IT_ISO"};

char *GetLang(uns8 charset)
{
    bool ISO  = (charset & 0x80) != 0;
    uns8 Lang = charset & 0x7F;
    if (Lang >= sizeof(LangWin) / sizeof(char *))
        return "";
    return ISO ? LangISO[Lang] : LangWin[Lang];
}

#ifdef NEVER
static char lCmd[] = 
"CREATE TABLE `%s`.`%s`" 
"("
    "ID         INTEGER  DEFAULT UNIQUE, "
    "Subject    CHAR(80) COLLATE %s, "
    "Sender     CHAR(64) COLLATE %s, "
    "Recipient  CHAR(64) COLLATE %s, "
    "CreDate    DATE, "
    "CreTime    TIME, "
    "RcvDate    DATE, "
    "RcvTime    TIME, "
    "Size       INTEGER, "
    "FileCnt    SMALLINT, "
    "Flags      INTEGER, "
    "Stat       CHAR ,"
    "MsgID      BINARY(64), "
    "Body       LONG VARCHAR, "
    "Addressees LONG VARCHAR, "
    "Profile    CHAR(64) COLLATE %s, "
    "Header     LONG VARCHAR, "
    "CONSTRAINT `I1` INDEX  (`Subject`),"
    "CONSTRAINT `I2` INDEX  (`Sender`),"
    "CONSTRAINT `I3` INDEX  (`Recipient`),"
    "CONSTRAINT `I4` INDEX  (`CreDate`),"
    "CONSTRAINT `I5` INDEX  (`CreTime`),"
    "CONSTRAINT `I6` INDEX  (`RcvDate`),"
    "CONSTRAINT `I7` INDEX  (`RcvTime`),"
    "CONSTRAINT `I8` INDEX  (`Stat`),"
    "CONSTRAINT `I9` UNIQUE (`MsgID`),"
    "CONSTRAINT `IA` INDEX  (`Profile`)"
")";

static char fCmd[] = 
"CREATE TABLE `%s`.`%s` "
"("
    "ID   INTEGER,"
    "Name CHAR(254) COLLATE CSISTRING, "
    "Size INTEGER, "
    "FDate DATE, "
    "FTime TIME, "
    "CONSTRAINT `I1` INDEX (`NAME`),"
    "CONSTRAINT `I2` INDEX (`FDATE`),"
    "CONSTRAINT `I3` INDEX (`FTIME`)"
")";

#endif //NEVER

#if WB_VERSION_MAJOR < 9

static char alCmd[] = 
"ALTER TABLE `%s`.`%s`" 
    "ADD COLUMN Addressees LONG VARCHAR %s, "
    "ADD COLUMN Profile    CHAR(64) %s, "
    "ADD COLUMN Header     LONG VARCHAR %s, "
    "CONSTRAINT `I1` INDEX  (`Subject`),"
    "CONSTRAINT `I2` INDEX  (`Sender`),"
    "CONSTRAINT `I3` INDEX  (`Recipient`),"
    "CONSTRAINT `I4` INDEX  (`CreDate`),"
    "CONSTRAINT `I5` INDEX  (`CreTime`),"
    "CONSTRAINT `I6` INDEX  (`RcvDate`),"
    "CONSTRAINT `I7` INDEX  (`RcvTime`),"
    "CONSTRAINT `I8` INDEX  (`Stat`),"
    "CONSTRAINT `I9` UNIQUE (`MsgID`),"
    "CONSTRAINT `IA` INDEX  (`Profile`)";

#endif // WB_VERSION_MAJOR >= 9
//
// Vytvoreni tabulek pro postovni schranku
//
#ifdef SRVR

DWORD CWBMailBox::GetTables
(
    cdp_t cdp, char *Appl, 
    char *InBoxTbl, ttablenum *InBoxL, table_descr **InBoxLDescr,
    char *InBoxTbf, ttablenum *InBoxF, table_descr **InBoxFDescr
)
{
    tobjnum Apl;
    WBUUID  UUID;

    //printf("GetTables %s %s\n", Appl, InBoxTbl);
    // IF v profilu neni aplikace
    if (!*Appl)
    {
        // IF neexistuje _SYSEXT, vytvorit
        lstrcpy(Appl, "_SYSEXT");
        if (ker_apl_name2id(cdp, Appl, (uns8 *)&UUID, &Apl))
        {
            sql_stmt_create_schema so;
            lstrcpy(so.schema_name, Appl);
            if (so.exec(cdp))
                goto error;
            commit(cdp);
        }
    }
    // IF aplikace neexistuje, chyba
    if (ker_apl_name2id(cdp, Appl, (uns8 *)&UUID, &Apl))
        goto error;
    
    // IF v profilu neni jmeno tabulky, nastavit default
    if (!*InBoxTbl)
        lstrcpy(InBoxTbl, "_INBOXMSGS");
    if (!*InBoxTbf)
        lstrcpy(InBoxTbf, "_INBOXFILES");

    // IF tabulka neexistuje, vytvorit
    *InBoxL = find2_object(cdp, CATEG_TABLE, (const uns8 *)&UUID, InBoxTbl);
    if (*InBoxL == NOOBJECT)
    {
        char Cmd[1024];

//#if WB_VERSION_MAJOR >= 9

        char *Lang = GetLang(sys_spec.charset);
        //printf("charset = %02X\n", sys_spec.charset);
        sprintf(Cmd, lCmd, Appl, InBoxTbl, Lang, Lang, Lang, Lang, Lang, Lang, Lang);

//#else

//        sprintf(Cmd, lCmd, Appl, InBoxTbl);

//#endif

        if (sql_exec_direct(cdp, Cmd))
            goto error;
        commit(cdp);
        *InBoxL = find2_object(cdp, CATEG_TABLE, (const uns8 *)&UUID, InBoxTbl);
    }

#if WB_VERSION_MAJOR < 9

    else
    {
        // IF v tabulce neni atribut ADDRESSEES, modifikovat
        table_descr_auto tda(cdp, *InBoxL);
        table_descr *td = tda->me();
        int a;
        for (a = 1; a < td->attrcnt; a++)
        { 
            if (lstrcmp(td->attrs[a].name, "ADDRESSEES") == 0)
                break;
        }
        if (a >= td->attrcnt)
        {
            char Cmd[540];            
            DWORD oldopt = cdp->sqloptions;
            cdp->sqloptions |= SQLOPT_OLD_ALTER_TABLE;
            sprintf(Cmd, alCmd, Appl, InBoxTbl);
            BOOL Err = sql_exec_direct(cdp, Cmd);
            cdp->sqloptions = oldopt;
            if (Err)
                goto error;
        }
    }

#endif // WB_VERSION_MAJOR < 9

    *InBoxLDescr = install_table(cdp, *InBoxL);
    if (!*InBoxLDescr)
        goto error;

    *InBoxF = find2_object(cdp, CATEG_TABLE, (const uns8 *)&UUID, InBoxTbf);
    if (*InBoxF == NOOBJECT)
    {
        char Cmd[300];

//#if WB_VERSION_MAJOR >= 9

        char *Lang = GetLang(sys_spec.charset);
        sprintf(Cmd, fCmd, Appl, InBoxTbf, Lang);

//#else

//        sprintf(Cmd, fCmd, Appl, InBoxTbf);

//#endif

        if (sql_exec_direct(cdp, Cmd))
            goto error;
        commit(cdp);
        *InBoxF = find2_object(cdp, CATEG_TABLE, (const uns8 *)&UUID, InBoxTbf);
    }
    *InBoxFDescr = install_table(cdp, *InBoxF);
    if (!*InBoxFDescr)
        goto error;
    return NOERROR;

error:

    return cdp->get_return_code();
}

#else   // !SRVR

DWORD CWBMailBox::GetTables
(
    cdp_t cdp, char *Appl, 
    char *InBoxTbl, ttablenum *InBoxL,
    char *InBoxTbf, ttablenum *InBoxF
)
{
    tobjnum Apl;
    WBUUID  UUID;
    DWORD   Err;

    // IF v profilu neni aplikace
    //printf("GetTables %s %s", Appl, InBoxTbl);
    if (!*Appl)
    {
        // IF neexistuje _SYSEXT, vytvorit
        lstrcpy(Appl, "_SYSEXT");
        if (cd_Apl_name2id(cdp, Appl, (uns8 *)&UUID))
        {
            if (cd_Insert_object(cdp, Appl, CATEG_APPL, &Apl))
                goto error;
        }
    }
    // ELSE IF aplikace neexistuje, chyba
    else if (cd_Apl_name2id(cdp, Appl, (uns8 *)&UUID))
        goto error;
    
    // IF v profilu neni jmeno tabulky, nastavit default
    if (!*InBoxTbl)
        lstrcpy(InBoxTbl, "_INBOXMSGS");
    if (!*InBoxTbf)
        lstrcpy(InBoxTbf, "_INBOXFILES");

    // IF tabulka neexistuje, vytvorit
    if (cd_Find2_object(cdp, InBoxTbl, (const uns8 *)&UUID, CATEG_TABLE, InBoxL))
    {
        Err = cd_Sz_error(cdp);
        if (Err != OBJECT_NOT_FOUND)
            goto exit;
        char Cmd[1024];

//#if WB_VERSION_MAJOR >= 9

        char *Lang = GetLang(cdp->sys_charset);
        sprintf(Cmd, lCmd, Appl, InBoxTbl, Lang, Lang, Lang, Lang, Lang, Lang, Lang);

//#else

//        sprintf(Cmd, lCmd, Appl, InBoxTbl);

//#endif

        uns32 Res;
        if (cd_SQL_execute(cdp, Cmd, &Res))
            goto error;
        *InBoxL = (ttablenum)Res;
    }

#if WB_VERSION_MAJOR < 9

    else
    {
        tattrib AttrNO;
        uns8    AttrTP;
        t_specif AttrSP;
        if (!cd_Attribute_info_ex(cdp, *InBoxL, "ADDRESSEES", &AttrNO, &AttrTP, &AttrTP, &AttrSP))
        {
            char Cmd[512];
            char *Lang = GetLang(cdp->sys_charset);
            sprintf(Cmd, alCmd, Appl, InBoxTbl, Lang, Lang, Lang);
            uns32 Res;
            cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, SQLOPT_OLD_ALTER_TABLE);
            if (cd_SQL_execute(cdp, Cmd, &Res))
                goto error;
        }
    }

#endif

    if (cd_Find2_object(cdp, InBoxTbf, (const uns8 *)&UUID, CATEG_TABLE, InBoxF))
    {
        Err = cd_Sz_error(cdp);
        if (Err != OBJECT_NOT_FOUND)
            goto exit;
        char Cmd[300];

//#if WB_VERSION_MAJOR >= 9

        char *Lang = GetLang(cdp->sys_charset);
        sprintf(Cmd, fCmd, Appl, InBoxTbf, Lang);

//#else

//        sprintf(Cmd, fCmd, Appl, InBoxTbf);

//#endif

        uns32 Res;
        if (cd_SQL_execute(cdp, Cmd, &Res))
            goto error;
        *InBoxF = (ttablenum)Res;
    }

error:

    Err = cd_Sz_error(cdp);

exit:

    return Err;
}

#endif  // SRVR
//
// Otevreni postovni schranky
//
// Pokud neexistuji tabulky pro doslou postu a pro pripojene soubory, vytvori je.
// Jinak jen zavola funkci Open potomka.
//
DWORD CWBMailBox::OpenInBox(cdp_t cdp)
{
    DWORD   Err;

    m_cdp = cdp;

#ifdef SRVR

    Err = GetTables(cdp, m_MailCtx->m_Appl, m_MailCtx->m_InBoxTbl, &m_InBoxL, &m_InBoxLDescr, m_MailCtx->m_InBoxTbf, &m_InBoxF, &m_InBoxFDescr);

#else

    Err = GetTables(cdp, m_MailCtx->m_Appl, m_MailCtx->m_InBoxTbl, &m_InBoxL, m_MailCtx->m_InBoxTbf, &m_InBoxF);

#endif

    if (Err)
        return Err;

    sprintf(m_InBoxTbl, "%s.%s", m_MailCtx->m_Appl, m_MailCtx->m_InBoxTbl); 
    sprintf(m_InBoxTbf, "%s.%s", m_MailCtx->m_Appl, m_MailCtx->m_InBoxTbf); 

    static time_t FreeTime;
    time_t t;
    t = time(NULL);
    if (t - FreeTime >= 24 * 60 * 60)
    {
        FreeDeleted();
        FreeTime = t;
    }

    return Open();
}
//
// Nacteni seznamu dosle posty do tabulky
//
// V cele tabulce oznaci vsechny zasilky jako smazane a pomoci funkce potomka nacte do tabulky seznam
// zasilek.
//
DWORD CWBMailBox::LoadList(UINT Flags)
{
    char Cmd[200];

//#ifdef SRVR
//    m_cdp->set_return_code(ANS_OK);
//#endif
    sprintf(Cmd, "UPDATE %s SET Stat=Chr(2) WHERE Stat <> Chr(2) AND Profile=\"%s\"", m_InBoxTbl, m_MailCtx->m_Profile);
    if (SQLExecute(Cmd))
        return WBError();
    Commit();
    return Load(Flags);
}
//
// Nacte telo dopisu
//
// Zkonvertuje ID zaznamu na ID zasilky a do atributu BODY nacte pomoci funkce potomka 
// vlastni telo dopisu.
//
DWORD CWBMailBox::GetMsg(DWORD ID, DWORD Flags)
{
//#ifdef SRVR
//    m_cdp->set_return_code(ANS_OK);
//#endif
    DWORD Err = GetMsgID(ID);
    if (!Err)
        Err = GetMsg(Flags);
    return Err;
}
//
// Nacte informaci o pripojenchg souborech
//
// Zkonvertuje ID zaznamu na ID zasilky a do atributu tabulky zapise pomoci funkce potomka 
// informaci o pripojenych souborech.
//
DWORD CWBMailBox::GetFilesInfo(DWORD ID)
{
//#ifdef SRVR
//    m_cdp->set_return_code(ANS_OK);
//#endif
    DWORD Err = GetMsgID(ID);
    if (Err)
        return Err;

    return GetFilesInfo();
}
//
// Ulozeni pripojeneho souboru do databaze
//
// Zkonvertuje ID zaznamu na ID zasilky a pomoci funkce potomka ulozi zadany soubor na 
// zadane misto.
//
DWORD CWBMailBox::SaveFileToDBs(DWORD ID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR AttrName, LPCSTR Cond)
{
    DWORD Err = GetMsgID(ID);
    if (Err)
        return Err;
    Err = CreateDBStream(NOCURSOR, NORECNUM, 0, NOINDEX);
    if (Err)
        return Err;

    Err = ((CAttStreamSMTPdi *)m_AttStream)->OpenCursor(Table, AttrName, Cond);
    if (!Err)
        Err = SaveFile(FilIdx, FilName);
    delete m_AttStream;
    return Err;
}
//
// Ulozeni pripojeneho souboru do databaze
//
// Zkonvertuje ID zaznamu na ID zasilky a pomoci funkce potomka ulozi zadany soubor na 
// zadane misto.
//
DWORD CWBMailBox::SaveFileToDBr(DWORD ID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index)
{
    DWORD Err = GetMsgID(ID);
    if (Err)
        return Err;
    Err = CreateDBStream(Table, Pos, Attr, Index);
    if (Err)
        return Err;
    Err = SaveFile(FilIdx, FilName);
    delete m_AttStream;
    return Err;
}
//
// Ulozeni pripojeneho souboru na disk
//
// Zkonvertuje ID zaznamu na ID zasilky a pomoci funkce potomka ulozi zadany soubor na 
// zadane misto.
//
DWORD CWBMailBox::SaveFileAs(DWORD ID, int FilIdx, LPCSTR FilName, char *DstPath)
{
    DWORD Err = GetMsgID(ID);
    if (Err)
        return Err;
    Err = CreateFileStream(DstPath);
    if (Err)
        return Err;
    Err = SaveFile(FilIdx, FilName);
    delete m_AttStream;
    return Err;
}
//
// Smazani zasilky
//
// Zkonvertuje ID zaznamu na ID zasilky, pomoci funkce potomka smaze zasilku ve schrance
// a je-li zadano smaze prislusny zaznam v tabulce.
//
DWORD CWBMailBox::DeleteMsg(DWORD ID, BOOL RecToo)
{
//#ifdef SRVR
//    m_cdp->set_return_code(ANS_OK);
//#endif
    DWORD Err = GetMsgID(ID);
    if (Err && Err != MAIL_MSG_NOT_FOUND)
        return Err;
    if (Err != MAIL_MSG_NOT_FOUND)
    {
        Err = DeleteMsg();
        if (Err)
            return Err;
    }
    if (RecToo)
    {
        Err = NOERROR;
        char  Cmd[80];
        sprintf(Cmd, "DELETE FROM %s WHERE ID=%d", m_InBoxTbf, ID);
        if (SQLExecute(Cmd))
            goto error;
        if (DeleteRec())
            goto error;
    }
    else
    {
        BYTE Stat = MBS_DELETED;
        if (WriteIndL(ATML_STAT, &Stat, 1))
            goto error;
    }
    return Err;

error:

    return WBError();
}
//
// Konverze ID zasilky na cislo zaznamu v tabulce
//
// Pouziva se pri hledani zasilky ze seznamu doslych zasilek v tabulce.
//
// Vraci MAIL_MSG_NOT_FOUND, pokud zasilka nebyla nalezena
// 
DWORD CWBMailBox::MsgID2Pos()
{
    char     Query[240];
    DWORD    Err = NOERROR;

    m_BodySize = NONEINTEGER;
    m_FileCnt  = NONESHORT;
    // Najit zasilku v tabulce
    DWORD  Len = sprintf(Query, "SELECT * FROM %s WHERE MsgID=X'", m_InBoxTbl);
    bin2hex(Query + Len, m_MsgID, MSGID_SIZE);
    *(WORD *)&Query[Len + 2 * MSGID_SIZE] = '\'';
    tcursnum Curs;
    if (!FindMsg(Query, &Curs))
        goto error;
    // IF nenalezena
    if (Curs == NOCURSOR)
    {
        m_CurPos = NORECNUM;
        Err = MAIL_MSG_NOT_FOUND;
        goto exit;
    }
    // Nacist delku tela a pocet soboru, aby se uz jednou stazena zasilka nestahovala znovu
    if (ReadLenC(Curs, ATML_BODY, &m_BodySize))
        goto error;
    if (ReadIndC(Curs, ATML_FILECNT, &m_FileCnt))
        goto error;
    m_CurPos = Translate(Curs);
    if (m_CurPos == NORECNUM)
        goto exit;

error:

    Err = WBError();

exit:

    if (Curs != NOCURSOR)
        CloseCursor(Curs);

    return Err;
}
//
// Konverze z ID zaznamu na ID zasilky
//
// Vraci MAIL_MSG_NOT_FOUND, pokud zasilka nebyla nalezena
// 
DWORD CWBMailBox::GetMsgID(uns32 ID)
{
    if (ID == m_ID)
        return NOERROR;

    DWORD    Err  = NOERROR;
    char     Query[80];

    sprintf(Query, "SELECT * FROM %s WHERE ID=%d", m_InBoxTbl, ID);
    tcursnum Curs;
    if (!FindMsg(Query, &Curs))
        goto error;

    if (Curs == NOCURSOR)
    {
        Err = MAIL_MSG_NOT_FOUND;
        goto exit;
    }

    m_CurPos = Translate(Curs);
    if (m_CurPos == NORECNUM)
        goto error;

    if (ReadIndC(Curs, ATML_MSGID, m_MsgID))
        goto error;

    m_ID = ID;
    goto exit;

error:

    Err = WBError();

exit:

    if (Curs != NOCURSOR)
        CloseCursor(Curs);

    return Err;
}
//
// Konvertuje zakodovane polozky ze zahlavi zasilky do kodovani serveru
//
// Vraci ukazatel ve vystupnim bufferu, kam se bude ukladat zkonverovany text z pripadneho dalsiho radku
//
char *CWBMailBoxPOP3::Convert(char **pSrc, char *Dst, DWORD Size)
{
    DWORD sz;
    char *Src    = *pSrc;
    int  SrcLang = 0;
    bool Conv    = false;
    UINT Flags   = 0;
    char q       = 0;

    if (*Src == '"' || *Src == '\'')
    {
        q     = *Src++;
        *pSrc = Src;
    }
    if (*(WORD *)Src == ('=' + ('?' << 8)))
    {
        Src    += 2;
        char *p = Src;
        SrcLang = GetSrcLang(&Src);
        Conv    = Src > p;
        if (Conv)
        {
            if (*Src == '?')
                Src++;
            else
                Conv = false;
        }
        if (Conv)
        {
            if (*Src == 'Q')
                Flags = P3CF_Q;
            else if (*Src == 'B')
                Flags = P3CF_BASE64;
            else
                Conv = false;
        }
        if (!Conv)
        {
            *Dst++ = *Src++;
            Size--;
        }
    }

    if (Conv)
    {
        // Najit konec zakodovaneho textu
        Src += 2;
        char *pe = strstr(Src, "?=");
        if (!pe)
            pe = Src + strlen(Src);
        sz = Convert(Src, (int)(pe - Src), Flags);
        while (Src[sz - 1] == 0)
            sz--;
        if (sz > Size)
            sz = Size;
        sz = CWBMailBox::Convert(Src, Dst, sz, SrcLang);
        Dst  += sz;
        Size -= sz;
        Src   = pe;
        if (*Src)
            Src += 2;
    }
    // ELSE jen zkopirovat
    else
    {
        Src = *pSrc;
    }
    while (Size-- && *Src != q && *(WORD *)Src != ('=' + ('?' << 8)))
        *Dst++ = *Src++;
    if (q)
        Src++;
    *Dst  = 0;
    *pSrc = Src;
    return Dst;
}

/*
unsigned char T885921250[] = 
{
//    00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0xA0, 0xA5, 0xA2, 0xA3, 0xA4, 0xBC, 0x8C, 0xA7, 0xA8, 0x8A, 0xAA, 0x8D, 0x8F, 0xB7, 0x8E, 0xAF,
    0xB0, 0xB9, 0xB2, 0xB3, 0xB4, 0xBE, 0x9C, 0xA1, 0xB8, 0x9A, 0xBA, 0x9D, 0x9F, 0x94, 0x9E, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};
*/
//
// Konvertuje data v bufferu z Quted-printable, BASE64 apod (prepisuje original)
//
// Vraci delku zkonvertovanych dat
//
DWORD CWBMailBoxPOP3::Convert(char *Ptr, DWORD Size, UINT Flags)
{
    char *pb = Ptr;
    // IF preskocit hlavicku, preskocit vsechno az do prazdneho radku
    if (Flags & P3CF_SKIPHDR)
    {
        pb = strstr(Ptr, "\r\n\r\n");
        if (pb)
            pb += 4;
        else
        {
            pb = strstr(Ptr, "\n\n");
            if (pb)
                pb += 2;
        }
        if (!pb)
            return 0;
    }   
    // IF Base64
    if (Flags & P3CF_BASE64)
    {
        char *s, *d; 
        for (s = pb, d = pb; s < Ptr + Size; s += 4, d += 3)
        {
            if (*s == '\r')
                s++;
            if (*s == '\n')
                s++;
            uns32 v =  Base64(*(DWORD *)s);
            if ((sig32)v < 0)
                break;
            *(DWORD *)d = v;
        }
        Size = (int)(d - Ptr);
    }
    // IF quoted-printable 
    else if (Flags & P3CF_QUOTED)
    {
        char *s, *d; 
        for (s = pb, d = pb; s < Ptr + Size; s++, d++)
        {
            // IF '='
            if (*s == '=')
            {
                // IF konec radku, preskocit
                s++;
                if (*s == '\r' && s[1] == '\n')
                {
                    s++;
                    d--;
                    continue;
                }
                else if (*s == '\n')
                {
                    d--;
                    continue;
                }
                // Jinak zkonvertovat
                char c = s[2];
                s[2] = 0;
                char n = (char)strtol(s, NULL, 16);
                //if (Flags & P3CF_ISO8859)
                //    *d = T885921250[n];
                //else
                //{
                    *d = n;
                //}
                s[2] = c;
                s++;
            }
            // IF "_" AND kodovani Q, zkonvertovat na mezeru
            else if ((Flags & P3CF_UNDRSCR) && *s == '_')
            {
                *d = ' ';
            }
            else
            {
                *d = *s;
            }
        }
        Size = (int)(d - Ptr);
    }

#ifdef UNIX

    else if (Flags & P3CF_DIRECT)
    {
        char *s, *e; 
        DWORD sz = Size;
        for (s = pb, e = Ptr + sz; (s = strstr(s, "\n>From ")) && s < e; s += 7)
        {
            memcpy(s + 1, s + 2, e - s - 2);
            sz--;
        }
        Size = sz;
    }

#endif

    return Size;
}
//
// Zkonvertuje radek Base64, maximalne 400 znaku t.j. 300 na vystupu
// Vraci delku vystupu
// 
DWORD CWBMailBox::Base64Line(char *Src, char *Dst, char **Next)
{ 
    // Preskocit mezery
    while (*Src == ' ' || *Src == '\t')
        Src++;
    
    // Zkontrolovat = na zacatku radku
    if (*Src == '=')
    {
        Src++;
        int sz = 1;
        if (*Src == '=')
        {
            Src++;
            sz++;
        }
        if (*Src == '\r' || *Src == '\n')
        {
            *Next = Src;
            return sz;
        }
        return (DWORD)-1;
    }
    
    // Zkontrolovat jestli do konce radku zbyva mene nez 4 znaky
    int  sz = 400;
    char *pr = strchr(Src, '\r');
    char *pn = strchr(Src, '\n');
    if (pr && (pr - Src) < sz)
        sz = (int)(pr - Src);
    if (pn && (pn - Src) < sz)
        sz = (int)(pn - Src);
    // IF zbyva mene nez 4 znaky
    if (sz < 4)
    {
        // Zkopirovat 4 znaky do Rest
        char Rest[4];
        pr = Rest;
        do
            *pr++ = *Src++;
        while (--sz);
        
        do
        {
            if (*Src == '\r')
                Src++;
            if (*Src == '\n')
                Src++;
            while (*Src == ' ' || *Src == '\t')
                Src++;
            if (*Src == '\r' || *Src == '\n')
                return (DWORD)-1;
            *pr++ = *Src++;
        }
        while (pr < Rest + 4);

        // Zkonverovat Rest
        uns32 d =  Base64(*(DWORD *)Rest);
        if ((sig32)d < 0)
            return (DWORD)-1;
        *(DWORD *)Dst = d;
        *Next = Src;
        return 3;
    }

    // Zaokrouhlit zbytek radku na 4 znaky
    sz = sz / 4 * 4;
    char *pMax = Src + sz;

    char *Ptr;
    for (Ptr = Dst; *Src != '=' && Src < pMax; Ptr += 3, Src += 4)
    {
        uns32 d =  Base64(*(DWORD *)Src);
        if ((sig32)d < 0)
            return (DWORD)-1;
        *(DWORD *)Ptr = d;
    }
    *Next = Src;
    return (DWORD)(Ptr - Dst);
}
        
signed char Base64Dec[] = 
{
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1, 0,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};
//
// Zkonvertuje 4 byty Base64 na 3 byty textu, v nejvyssim bytu vraci pocet
//

#if defined(WINS) && !defined(X64ARCH)

DWORD CWBMailBox::Base64(DWORD c)
{
    _asm
    {
        mov   eax,0xFFFF0000
        xor   ebx,ebx
        xor   cl,cl
        mov   edx,c
        mov   bl,dl
        mov   bl,Base64Dec[ebx]
        or    cl,bl
        or    ah,bl
        shl   eax,6
        shr   edx,8
        mov   bl,dl
        mov   bl,Base64Dec[ebx]
        or    cl,bl
        or    ah,bl
        shl   eax,6
        shr   edx,8
        mov   bl,dl
        mov   bl,Base64Dec[ebx]
        or    cl,bl
        or    ah,bl
        shl   eax,6
        shr   edx,8
        mov   bl,dl
        mov   bl,Base64Dec[ebx]
        or    cl,bl
        js    err
        or    ah,bl
        bswap eax
err:
        mov   c,eax
    }
    return c;
}

#else

DWORD CWBMailBox::Base64(DWORD c)
{
    DWORD Dst = 0;
    signed char  Err = 0;

    Err |= Base64Dec[c & 0xFF];
    Dst |= Base64Dec[c & 0xFF];
    Dst <<= 6;
    c   >>= 8;
    Err |= Base64Dec[c & 0xFF];
    Dst |= Base64Dec[c & 0xFF];
    Dst <<= 6;
    c   >>= 8;
    Err |= Base64Dec[c & 0xFF];
    Dst |= Base64Dec[c & 0xFF];
    Dst <<= 6;
    c   >>= 8;
    Err |= Base64Dec[c & 0xFF];
    if (Err < 0)
        return (DWORD)-1;
    Dst |= Base64Dec[c & 0xFF];
    char *pDst = (char *)&Dst;
    char t  = pDst[0];
    pDst[0] = pDst[2];
    pDst[2] = t;
    return Dst;
}

#endif
//
// Konvertuje text do ciloveho jazyka
//
// Vraci delku zkonvertovanych dat
//
DWORD CWBMailBox::Convert(char *Src, char *Dst, DWORD Size, int SrcLang)
{
    wbcharset_t SrcCharSet(SrcLang);
#ifdef SRVR
    wbcharset_t DstCharSet(sys_spec.charset);
#else
    wbcharset_t DstCharSet(m_cdp->sys_charset);
#endif

    DWORD sz = (DWORD)strlen(Src) + 1;
    if (sz < Size)
        Size = sz;

#ifdef WINS
    if (DstCharSet.cp() == SrcCharSet.cp())
#else
    if (strcmp(DstCharSet, SrcCharSet) == 0)
#endif
    {
        if (Src != Dst)
            memcpy(Dst, Src, Size);
        return Size;
    }

#ifdef WINS

    if (SrcCharSet.cp() == 852 && DstCharSet.cp() == 1250)
    {
        if (Src != Dst)
            memcpy(Dst, Src, Size);
        encode(Dst, Size, true, 3);
        return Size;
    }

#endif

    sz = LangToLang(Src, Size, Dst, Size, SrcCharSet, DstCharSet);
    if (!sz) 
    {
        if (Src != Dst)
            memcpy(Dst, Src, Size);
        sz = Size;
    }
    return sz;
}
//
// Reallocates memory for adressees list
//
BOOL CWBMailBox::ReallocAddrs(UINT Size)
{
    // IF neni naalokovan seznam, naalokovat
    if (!m_Addressees)
    {
        m_Addressees = (char *)corealloc(256, 0xEF);
        if (!m_Addressees)
            return FALSE;
        m_Addressp = m_Addressees;
        m_Addressz = 256;
    }
    // ELSE IF malo mista v seznamu, realokovat
    else 
    {
        UINT Off = (UINT)(m_Addressp - m_Addressees);
        if ((int)(m_Addressz - Off) < (int)Size)
        {
            if (Size < 256)
                Size = 256;
            char *NewList = (char *)corerealloc(m_Addressees, m_Addressz + Size);
            if (!NewList)
                return FALSE;
            m_Addressz  += Size;
            m_Addressees = NewList;
            m_Addressp   = NewList + Off;
        }
    }
    return TRUE;
}

#ifdef WINS
#ifdef MAIL602
//
// CWBMailBox602 - Postovni schranka Mail602
//
// Speky pri praci se vzdalenym nebo POP3 klientem
//
// POP3     Vzdalena zasilka Referencni datum je TPOP3MInfo.Date
//          Lokalni zasilka                      PMessageInfo.DatumCasVzniku
//
// Remote   Vzdalena zasilka Referencni datum je PMessageInfo.DatumCasOdeslani           
//                           Odesilatel 602      PMessageInfo.Odesilatel.JmenoUcastnika = User@Office
//                                               PMessageInfo.Odesilatel.JmenoPosty     =                                                  
//                           Odesilatel SMTP     PMessageInfo.Odesilatel.JmenoUcastnika = User@Domain@INTERNET
//                                               PMessageInfo.Odesilatel.JmenoPosty     =            
//          Lokalni zasilka  Referencni datum je PMessageInfo.DatumCasOdeslani                                
//                           Odesilatel 602      PMessageInfo.Odesilatel.JmenoUcastnika = User         
//                                               PMessageInfo.Odesilatel.JmenoPosty     = Office                    
//                           Odesilatel SMTP     PMessageInfo.Odesilatel.JmenoUcastnika = User@Domain
//                                               PMessageInfo.Odesilatel.JmenoPosty     = INTERNET
//
// Konstruktor
//
CWBMailBox602::CWBMailBox602(CWBMailCtx *MailCtx) : CWBMailBox(MailCtx)
{
    m_FileList   = NULL;
    m_OldList    = NULL;
    m_RemMsgList = NULL;
    m_OldCount   = 0;
}
//
// Destruktor
//
CWBMailBox602::~CWBMailBox602()
{
    if (m_FileList)
        lpFreeFileList(&m_FileList);
    if (m_OldList)
        lpPOP3FreeListOfMsgs(m_OldList, m_OldCount);
    if (m_RemMsgList)
        lpFreeMsgList(&m_RemMsgList);
}
//
// Inicializacce schranky
//
DWORD CWBMailBox602::Open()
{
    if (lpGetMsgList)
        return NOERROR;

    if (!m_MailCtx->m_hMail602)
        return MAIL_NOT_INITIALIZED;

    if (*m_MailCtx->m_POP3Server)
    {
        //DbgLogWrite("M602InetPOP3GetListOfMessages");
        lpPOP3GetListOfMsgs  = (LPPOP3GETLISTOFMSGS) GetProcAddress(m_MailCtx->m_hM602Inet, "M602InetPOP3GetListOfMessages");
        if (!lpPOP3GetListOfMsgs)
            goto error;
        //DbgLogWrite("M602InetPOP3FreeListOfMessages");
        lpPOP3FreeListOfMsgs = (LPPOP3FREELISTOFMSGS)GetProcAddress(m_MailCtx->m_hM602Inet, "M602InetPOP3FreeListOfMessages");
        if (!lpPOP3FreeListOfMsgs)
            goto error;
        //DbgLogWrite("M602InetPOP3GetOrEraseMessages");
        lpPOP3GetOrEraseMsgs = (LPPOP3GETORERASEMSGS)GetProcAddress(m_MailCtx->m_hM602Inet, "M602InetPOP3GetOrEraseMessages");
        if (!lpPOP3GetOrEraseMsgs)
            goto error;
    }    
    else if (m_MailCtx->m_Type & WBMT_602SP3)
        return MAIL_PROFSTR_NOTFND;

        //DbgLogWrite("DisposeMessageList");
    lpFreeMsgList = (LPFREEMSGLIST)            GetProcAddress(m_MailCtx->m_hMail602, "DisposeMessageList");
    if (!lpFreeMsgList)                         
        goto error;                            
        //DbgLogWrite("SizeOfLetterX");
    lpSizeOfMsg   = (LPSIZEOFMSG)              GetProcAddress(m_MailCtx->m_hMail602, "SizeOfLetterX");
    if (!lpSizeOfMsg)                          
        goto error;                            
        //DbgLogWrite("ReadLetterFromMessage");
    lpReadLetterFromMsg = (LPREADLETTERFROMMSG)GetProcAddress(m_MailCtx->m_hMail602, "ReadLetterFromMessage");
    if (!lpReadLetterFromMsg)                        
        goto error;                        
        //DbgLogWrite("GetMessageFileList");
    lpGetFileList = (LPGETFILELIST)            GetProcAddress(m_MailCtx->m_hMail602, "GetMessageFileList");
    if (!lpGetFileList)                       
        goto error;                           
        //DbgLogWrite("DisposeMessageFileList");
    lpFreeFileList = (LPFREEFILELIST)          GetProcAddress(m_MailCtx->m_hMail602, "DisposeMessageFileList");
    if (!lpFreeFileList)                      
        goto error;                           
        //DbgLogWrite("CopyFileFromMessage");
    lpCopyFileFromMsg = (LPCOPYFILEFROMMSG)    GetProcAddress(m_MailCtx->m_hMail602, "CopyFileFromMessage");
    if (!lpCopyFileFromMsg)                    
        goto error;                            
        //DbgLogWrite("DeleteMessage");
    lpDeleteMsg = (LPDELETEMSG)                GetProcAddress(m_MailCtx->m_hMail602, "DeleteMessage");
    if (!lpDeleteMsg)                          
        goto error;                            
        //DbgLogWrite("GetMessageList");
    lpGetMsgList = (LPGETMSGLIST)              GetProcAddress(m_MailCtx->m_hMail602, "GetMessageList");
    if (!lpGetMsgList)
        goto error;
        //DbgLogWrite("GetMailBoxStatusInfo");
    lpGetMailBoxStatusInfo = (LPGETMAILBOXSTATUSINFO)GetProcAddress(m_MailCtx->m_hMail602, "GetMailBoxStatusInfo");
    if (!lpGetMailBoxStatusInfo)
        goto error;
    //lpGetMsgListInQueue = (LPGETMSGLISTINQUEUE)GetProcAddress(m_hMail602, "GetMessageListInQueue");
    //if (!lpGetMsgListInQueue)
    //    goto error;

        //DbgLogWrite("CWBMailBox602::Open OK");
    return 0;

error:

    return MAIL_FUNC_NOT_FOUND;
}
//
// Nacteni seznamu zasilek
//
DWORD CWBMailBox602::Load(UINT Param)
{
    PMessageInfoList MsgList = NULL, Msg;
    DWORD Date, Time, Size, Flags;
    DWORD Err;
    BOOL  Trans = FALSE;
    BOOL  Found;
    char  Subject[81];
    char  Sender[65];
    BYTE  Stat;

    // IF POP3, stahnout obsah POP3 schranky
    if (*m_MailCtx->m_POP3Server)
    {
        Err = LoadPOP3(Param);
        if (Err)
            goto exit;
    }
    // IF bezi FaxMailServer, stahnout seznam z materskeho uradu
    else if (RemoteCtx.m_hGateThread)
    {
        Err = LoadRem(Param);
        if (Err)
            goto exit;
    }

    // Pres vsechny zasilky
    Err = lpGetMsgList(&MsgList);
    if (Err)
    {
        //DbgLogFile("GetMsgList = %d", Err);
        goto errorm;
    }
    for (Msg = MsgList; Msg; Msg = Msg->Next)
    {
        PMessageInfo Msgi = &Msg->MessageInfo;

        // Zasilku "Seznam zasilek" muzeme zahodit
        if ((m_MailCtx->m_Type & WBMT_602REM) && memcmp(Msgi->Subject + 1, m_MailCtx->m_RemBox, 19) == 0)
            continue;

        // Normalni Mail602 zasilky maji jako ID 8-mistne hexadecimalni cislo,
        // remote zasilky na materskem urade maji ID = 00000000
        // IF zasilka nema ID, sestavit ID z data odeslani, subjetu a odesilatele
        if (Msgi->IDZasilky)
        {
            Size = *Msg->MessageID + 1;
            memcpy(m_MsgID, Msg->MessageID, Size);
            memset(m_MsgID + Size, 0, MSGID_SIZE - Size);
        }
        else
        {
            BYTE *p = m_MsgID;
            *(DWORD *)p = Msgi->DatumCasOdeslani;
            p += 4;
            Size = *Msgi->Subject + 1;
            if (Size > MSGID_SIZE - 4)
                Size = MSGID_SIZE - 4;
            memcpy(p, Msgi->Subject, Size);
            p += Size;
            Size = *Msgi->Odesilatel.JmenoUcastnika + 1;
            if (Size > m_MsgID + MSGID_SIZE - p)
                Size = m_MsgID + MSGID_SIZE - p;
            if (Size)
            {
                memcpy(p, Msgi->Odesilatel.JmenoUcastnika, Size);
                p += Size;
                Size = m_MsgID + MSGID_SIZE - p;
                if (Size)
                    memset(p, 0, Size);
            }
        }
        Size = Convert(Msgi->Subject + 1, Subject, *Msgi->Subject, 6);
        Subject[Size] = 0;
        // Srovnat adresu odesilatele, aby byla stejna u remote i lokalni zasilky
        //DbgLogFile("Msg: %s", Subject);
        Size = *Msgi->Odesilatel.JmenoUcastnika + 1;
        if (memcmp(Msgi->Odesilatel.JmenoUcastnika + Size - 9, "@INTERNET", 9) == 0)
            Size -= 9;
        lstrcpyn(Sender, Msgi->Odesilatel.JmenoUcastnika + 1, Size);
        if (*Msgi->Odesilatel.JmenoPosty && memcmp(Msgi->Odesilatel.JmenoPosty + 1, "INTERNET", 8))
        {
            Sender[Size - 1] = '@';
            lstrcpyn(Sender + Size, Msgi->Odesilatel.JmenoPosty + 1, *Msgi->Odesilatel.JmenoPosty + 1);
            Size += *Msgi->Odesilatel.JmenoPosty + 1;
        }
        Size = Convert(Sender, Sender, Size, 6);
        Sender[Size] = 0;
        m_MsgType = Msgi->Odesilatel.DruhUcastnika; 
        if (m_MsgType != 'F') // Fax
        {
            m_Addressp = m_Addressees;
            Err = GetAddressees(Msgi);
            if (Err)
                goto errorm;
        }
        DDTM2WB(*m_MailCtx->m_POP3Server ? Msgi->DatumCasVzniku : Msgi->DatumCasOdeslani, &Date, &Time);

        if (StartTransaction())
            goto error;

        Trans = TRUE;
        Found = FALSE;
        Stat  = MBS_OLD;
        // IF lokalni zasilka nalezena jako remote, prepsat MsgID
        if (m_MailCtx->m_Type & (WBMT_602REM | WBMT_602SP3))
        {
            Err = LocMsgID2Pos(Subject, Sender, Date, Time);
            if (!Err)
            {
                Found = TRUE;
                if (m_Addressp > m_Addressees)
                {
                    if (WriteVarL(ATML_ADDRESSEES, m_Addressees, m_Addressp - m_Addressees))
                        goto error;
                    if (WriteLenL(ATML_ADDRESSEES, m_Addressp - m_Addressees))
                        goto error;
                }
            }
            else if (Err != MAIL_MSG_NOT_FOUND)
                goto exit;
        }
        if (!Found)
        {
            // IF zasilka nenalezena v databazi, vlozit novy zaznam
            Err = MsgID2Pos();
            if (Err)
            {
                if (Err != MAIL_MSG_NOT_FOUND)
                    goto exit;

                m_CurPos = InsertL();
                if (m_CurPos == NORECNUM)
                    goto error;
                if (WriteIndL(ATML_SUBJECT, Subject, 80))
                    goto error;
                if (WriteIndL(ATML_SENDER,  Sender,  64))
                    goto error;
                if (WriteIndL(ATML_CREDATE, &Date,   4))
                    goto error;
                if (WriteIndL(ATML_CRETIME, &Time,   4))
                    goto error;
                DDTM2WB(Msgi->DatumCas,     &Date,   &Time); 
                if (WriteIndL(ATML_RCVDATE, &Date,   4))
                    goto error;
                if (WriteIndL(ATML_RCVTIME, &Time,   4))
                    goto error;
                lpSizeOfMsg(Msgi, 0, NULL,  &Size);
                if (WriteIndL(ATML_SIZE,    &Size,   4))
                    goto error;
                if (m_Addressp > m_Addressees)
                {
                    if (WriteVarL(ATML_ADDRESSEES, m_Addressees, m_Addressp - m_Addressees))
                        goto error;
                    if (WriteLenL(ATML_ADDRESSEES, m_Addressp - m_Addressees))
                        goto error;
                }

                Stat = MBS_NEW;
            }
        }
        // Prepsat ID zasilky, adresu odesilatele, priznaky
        if (WriteIndL(ATML_MSGID, m_MsgID, MSGID_SIZE))
            goto error;
        Size = Convert(Msgi->Adresat.JmenoUcastnika + 1, Sender, *Msgi->Adresat.JmenoUcastnika, 6);
        Sender[Size] = 0;
        if (WriteIndL(ATML_RECIPIENT, Sender, 64))
            goto error;
        Flags = 0;  
        if (Msgi->Priznaky & poConfirm)
            Flags |= WBL_READRCPT;
        if (Msgi->Priznaky & poViewed)
            Flags |= WBL_VIEWED;
        if (Msgi->Priznaky & poFAXMessage)
            Flags |= WBL_FAX;
        if (Msgi->EXPriznaky & expoNizkaPriorita)
            Flags |= WBL_PRILOW;
        if (Msgi->EXPriznaky & expoVysokaPriorita)
            Flags |= WBL_PRIHIGH;
        if ((m_MailCtx->m_Type & WBMT_602REM) && !(Msgi->EXPriznaky & expoMistni))
            Flags |= WBL_REMOTE;
        if (Msgi->EXPriznaky & expoZazadanVymaz)
            Flags |= WBL_DELREQ;
        if (WriteIndL(ATML_FLAGS, &Flags, 4))
            goto error;
        if (WriteIndL(ATML_STAT,  &Stat,  1))
            goto error;
        // IF take telo dopisu a jeste nebylo nacteno,
        // ulozit telo dopisu z lokalni zasilky do tabulky
        // (remote zasilky byly stazeny v LoadRem nebo LoadPOP3)
        if (Param & (MBL_BODY | MBL_HDRONLY | MBL_MSGONLY))
        {
            Err = GetMsgLoc(Param & (MBL_BODY | MBL_HDRONLY | MBL_MSGONLY));
            if (Err)
                goto exit;
        }
        // IF take seznam pripojenych souboru a jeste nebyl nacten
        // ulozit seznam pripojenych souboru do tabulky
        if ((Param & MBL_FILES) && (Msgi->PocetSouboru > 0))
        {
            Err = GetFilesInfoLoc();
            if (Err)
                goto exit;

            lpFreeFileList(&m_FileList);
             m_FileList = NULL;
        }
        if (WriteIndL(ATML_PROFILE, m_MailCtx->m_Profile, 64))
            goto error;

        Trans = FALSE;
        if (Commit())
            goto error;
    }
    Err = NOERROR;
    goto exit;

error:

    Err = WBError();
    goto exit;

errorm:

    Err = Err602ToszErr(Err);

exit:

    if (Trans)
        RollBack();
    if (MsgList)
    {
        //DbgLogFile("FreeMsgList");
        lpFreeMsgList(&MsgList);
        //DbgLogFile("FreeMsgList OUT");
    }
    return Err;
}
//
// Reads addressees list from message
//
DWORD CWBMailBox602::GetAddressees(PMessageInfo Msgi)
{
    TMail602AddressList *AddrList;
    WORD Type = 0xF18;   // TO:
    DWORD To = ADDR_TO;
    for (int i = 2; i; i--)
    {
        DWORD Err = lpReadADRFileX(Msgi, 0, NULL, &AddrList, Type);
        if (Err)
            return Err;
        if (!AddrList && i == 2)
        {
            if(!ReallocAddrs(*Msgi->Adresat.JmenoUcastnika + *Msgi->Adresat.JmenoPosty + 8))
                return 5;
            *(DWORD *)m_Addressp = ADDR_TO;
            m_Addressp          += 4;
            memcpy(m_Addressp, Msgi->Adresat.JmenoUcastnika + 1, *Msgi->Adresat.JmenoUcastnika);
            m_Addressp += *Msgi->Adresat.JmenoUcastnika;
           *m_Addressp++ = '@';
            memcpy(m_Addressp, Msgi->Adresat.JmenoPosty + 1, *Msgi->Adresat.JmenoPosty);
            m_Addressp += *Msgi->Adresat.JmenoPosty;
            *(WORD *)m_Addressp = '\r' + ('\n' << 8);
            m_Addressp         += 2;
        }
        for (TMail602AddressList *le = AddrList; le; le = le->Next)
        {
            int sz = *le->Address.LongName;
            if(!ReallocAddrs(sz + 9))
                return 5;
            *(DWORD *)m_Addressp = To;
            m_Addressp          += 4;
            if (le->Address.UserType == 'I')
            {
                char *pb = strchr(le->Address.LongName + 1, '<');
                if (!pb || pb - le->Address.LongName - 1 > sz)
                    *m_Addressp++ = '<';
            }
            sz = Convert(le->Address.LongName + 1, m_Addressp, sz, 6);
            m_Addressp[sz] = 0;
            if (m_Addressp[-1] == '<')
            {
                m_Addressp[sz] = '>';
                sz++;
            }
            else
            {
                char c = 0;
                if (*m_Addressp == '"')
                    c = '"';
                else if (*m_Addressp == '\'')
                    c = '\'';
                if (c)
                {
                    memcpy(m_Addressp, m_Addressp + 1, sz - 1);
                    sz--;
                    char *p = strchr(m_Addressp, c);
                    if (p)
                    {
                        memcpy(p, p + 1, sz - (p - m_Addressp) - 1);
                        sz--;
                    }
                }
            }
            m_Addressp += sz;
            *(WORD *)m_Addressp = '\r' + ('\n' << 8);
            m_Addressp         += 2;
        }
        *m_Addressp = 0;
        Type        = 0xF0C;     // CC
        To          = ADDR_CC;

        lpFreeAList(&AddrList);
    }
    return NOERROR;
}
//
// Ulozeni tela dopisu do databaze
//
// Je-li treba, stahne zasilku z materskeho uradu a ulozi telo dopisu do tabulky
//
DWORD CWBMailBox602::GetMsg(DWORD Flags)
{
    if (m_MailCtx->m_Type & (WBMT_602REM | WBMT_602SP3))
    {
        DWORD Err = LoadRemMsg();
        if (Err)
            return Err;
    }
    else if (Flags & (MBL_HDRONLY | MBL_MSGONLY))
    {
        PMessageInfoList MsgList = NULL, Msg;

        DWORD Err = lpGetMsgList(&MsgList);
        if (Err)
            return Err602ToszErr(Err);
        m_MsgType = 0;
        for (Msg = MsgList; Msg; Msg = Msg->Next)
        {
            if (memcmp(m_MsgID, Msg->MessageID, *Msg->MessageID + 1) == 0)
            {
                m_MsgType = Msg->MessageInfo.Odesilatel.DruhUcastnika;
                break;
            }
        }
        if (MsgList)
            lpFreeMsgList(&MsgList);
    }
    return GetMsgLoc(Flags);
}
//
// Ulozeni tela dopisu z lokalni zasilky do databaze
//
DWORD CWBMailBox602::GetMsgLoc(DWORD Flags)
{
    DWORD Err;
    DWORD Size;
    char  Buf[4096 + 1];
    char  *TmpPath = Buf;
    char  TmpFile[_MAX_PATH];

    if (m_MsgType == 'F') // FAX
    {
        WriteLenL(ATML_BODY, 0);
        return NOERROR;
    }

    TempFileName(TmpFile);
    ctopas(TmpFile, TmpPath);
    Err = lpReadLetterFromMsg((LPSTR)m_MsgID, TmpPath);
    if (Err)
        return Err602ToszErr(Err);
    FHANDLE hMsg = CreateFile(TmpFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (!FILE_HANDLE_VALID(hMsg))
    {
        Err = MAIL_FILE_NOT_FOUND;
        goto exit;
    }

    if (Flags & MBL_BODY)
        Flags = 0;

    char *pBody;
    char *pBuf;
    DWORD hOffset, bOffset;
    DWORD bSize;
    pBody   = NULL;
    hOffset = 0;
    bOffset = 0;
    for (;;)
    { 
        if (!ReadFile(hMsg, Buf, 4096, &Size, NULL))
        { 
            Err = OS_FILE_ERROR;
            goto exit;
        }
        if (!Size) 
            break;
        Buf[Size] = 0;
        pBuf = Buf;
        if (!pBody && (Flags & (MBL_HDRONLY | MBL_MSGONLY)))
        {
            // Konec posledniho radku
            char *pe = strrchr(Buf, '\n');
            if (pe)
            {
                pe++;
                int Rest = Size - (pe - Buf);
                // Dalsi kus se bude cist od zacatku radku
                if (Rest > 0)
                    SetFilePointer(hMsg, -Rest, NULL, FILE_CURRENT);
                Size -= Rest;
                Buf[Size] = 0;
            }
            // IF internetova zasilka
            if (m_MsgType == 'I')
            {
                // IF buf zacina prazdnym radkem
                if (*(WORD *)Buf == '\r' + ('\n' << 8))
                    pBody = Buf;
                // ELSE Hledat prazdny radek
                else
                    pBody = strstr(Buf, "\r\n\r\n");
            }
            // ELSE hledat oddelovac
            else
            {
                pBody = strstr(Buf, "----------------");
            }
            // IF oddelovac nalezen
            if (pBody)
            {
                if (Flags & MBL_MSGONLY)
                {
                    // preskocit prazdne radky
                    pBody = strchr(pBody, '\n') + 1;
                    while (*pBody == '\r' || *pBody == '\n')
                        pBody++;
                    bSize = Size - (pBody - Buf);
                }
                // IF jen hlavicka
                if (Flags & MBL_HDRONLY)
                {
                    Size = pBody - Buf;
                }
            }
            if (Flags & MBL_HDRONLY)
            {
                Size = Convert(pBuf, pBuf, Size, 6);
                if (WriteVarL(ATML_HEADER, pBuf, hOffset, Size))
                    goto error;
                hOffset += Size;
                Size     = 0;
            }
            if (pBody)
            {
                if (WriteLenL(ATML_HEADER, hOffset))
                    goto exit;
                if (Flags == MBL_HDRONLY)
                    break;
                pBuf = pBody;
                Size = bSize;
            }
        }           
        if (Size)
        {
            Size = Convert(pBuf, pBuf, Size, 6);
            if (WriteVarL(ATML_BODY, pBuf, bOffset, Size))
                goto error;
            bOffset += Size;
        }
    } 
    if (!WriteLenL(ATML_BODY, bOffset))
        goto exit;

error:

    Err = WBError();
    
exit:

    if (FILE_HANDLE_VALID(hMsg))
    {
        CloseFile(hMsg);
        DeleteFile(TmpFile);
    }
    return Err;
}
//
// Ulozeni seznamu pripojenych souboru do databaze
//
// Je-li treba, stahne zasilku z materskeho uradu a ulozi seznam pripojenych souboru do tabulky
//
DWORD CWBMailBox602::GetFilesInfo()
{
    DWORD Err = LoadRemMsg();
    if (Err)
        return Err;
    return GetFilesInfoLoc();
}
//
// Ulozeni seznamu pripojenych souboru z lokalni zasilky do databaze
//
DWORD CWBMailBox602::GetFilesInfoLoc()
{
    PMessageFileList File;
    DWORD Date, Time;
    DWORD ID;
    DWORD Err;
    char  Buf[256];

    if (!m_FileList)
    {
        Err = lpGetFileList((LPSTR)m_MsgID, &m_FileList);
        if (Err)
        {
            //DbgLogFile("GetFileList = %d", Err);
            return Err602ToszErr(Err);
        }
    }
    if (StartTransaction())
        goto exit;
    if (ReadIndL(ATML_ID, &ID))
        goto exit;
    sprintf(Buf, "DELETE FROM %s WHERE ID=%d", m_InBoxTbf, ID);
    if (SQLExecute(Buf))
        goto exit;

    for (File = m_FileList; File; File = File->Next)
    {
        trecnum PosF = InsertF();
        if (PosF == NORECNUM)
            break;
        if (WriteIndF(PosF, ATMF_ID, &ID, 4))
            break;
        Convert(File->LongName + 1, Buf, *File->LongName, 6);
        //DbgLogFile("FileName: %.80s", File->LongName + 1);
        if (WriteIndF(PosF, ATMF_NAME, Buf, 254))
            break;
        DDTM2WB(File->Time, &Date, &Time);
        if (WriteIndF(PosF, ATMF_DATE, &Date, 4))
            break;
        if (WriteIndF(PosF, ATMF_TIME, &Time, 4))
            break;
        if (WriteIndF(PosF, ATMF_SIZE, &File->Size, 4))
            break;
    }

exit:

    Err = WBError();
    if (Err)
        RollBack();
    else
        Commit();

    return Err;
}
//
// Ulozeni pripojeneho souboru
// 
DWORD CWBMailBox602::SaveFile(int FilIdx, LPCSTR FilName)
{
    DWORD Err;

    //DbgLogFile("SaveFile602 %d", FilIdx);
    // Je-li treba, stahnout zasilku z materskeho uradu
    Err = LoadRemMsg();
    if (Err)
        return Err;

    if (!m_FileList)
    {
        Err = lpGetFileList((LPSTR)m_MsgID, &m_FileList);
        if (Err)
        {
            //DbgLogFile("GetFileList = %d", Err);
            Err = Err602ToszErr(Err);
            goto exit;
        }
    }
    
    PMessageFileList File;
    // IF zadano jmeno souboru, hledat podle jmena, jinak podle indexu
    if (FilName && *FilName)
    {
        char Buf[MAX_PATH];
        for (File = m_FileList; File; File = File->Next)
        {
            DWORD sz = Convert(File->LongName + 1, Buf, *File->LongName, 6);
            if (strnicmp(FilName, Buf, *File->LongName) == 0 && FilName[sz] == 0)
                break;
        }
    }
    else
    {
        for (File = m_FileList; File; File = File->Next)
        {
            if (--FilIdx < 0)
                break;
        }
    }
    if (!File)
    {
        Err = MAIL_NO_MORE_FILES;
        goto exit;
    }

    Err = m_AttStream->CopyFrom(File->FileName, 0);

exit:

    return Err;
}
//
// Vytvoreni streamu pro zapis pripojeneho souboru do databaze
//
DWORD CWBMailBox602::CreateDBStream(tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index)
{
    m_AttStream = (CAttStream *) new CAttStream602di(m_cdp, Curs, Pos, Attr, Index, m_MsgID);
    if (!m_AttStream)
        return OUT_OF_APPL_MEMORY;
    return NOERROR;
}
//
// Vytvoreni streamu pro zapis pripojeneho souboru na disk
//
DWORD CWBMailBox602::CreateFileStream(LPCSTR DstPath)
{
    m_AttStream = new CAttStream602fi(DstPath, m_MsgID);
    if (!m_AttStream)
        return OUT_OF_APPL_MEMORY;
    return NOERROR;
}
//
// Zruseni zasilky
//
DWORD CWBMailBox602::DeleteMsg()
{
    DWORD Err;
    if (m_MailCtx->m_Type & (WBMT_602REM | WBMT_602SP3))
    {
        // IF zasilka je i na materskem urade, smazat na materskem urade
        if (*m_MailCtx->m_POP3Server)
        {
            TPOP3MInfo *MsgInfo;
            Err = GetMsgInfoPOP3(&MsgInfo);
            if (Err)
                return Err;
            MsgInfo->Flag = P3RQ_DELMSG;
            RequestPOP3();
        }
        else
        {
            TMessageInfoList MsgInfo;
            MsgInfo.MessageInfo.EXPriznaky = 0;
            DWORD Flags;
            if (ReadIndL(ATML_FLAGS, &Flags))
                return WBError();
            if (Flags & WBL_REMOTE)
            {
                Err = GetMsgInfoRem(&MsgInfo);
                if (Err)
                    return Err;
                if (MsgInfo.MessageInfo.EXPriznaky & expoNaSiti)
                {
                    BOOL Send = RemoteCtx.m_hGateThread && WaitForSingleObject(RemoteCtx.m_GateON, 0) == WAIT_TIMEOUT;
                    //DbgLogFile("Delete GateON OFF");
                    RequestRem(RMRQ_DELMSG, &MsgInfo, Send);
                }
            }
        }
    }
    // Smazat zasilku v lokalni schrance
    Err = lpDeleteMsg((LPSTR)m_MsgID);
    if (Err)
    {
        //DbgLogFile("DeleteMsg = %d", Err);
        return Err602ToszErr(Err);
    }
    return NOERROR;
}
//
// Konverze z ID zaznamu na ID zasilky
//
// Meni kontext aktualni zasilky t.j. zrusi seznam souboru predesle zasilky
//
DWORD CWBMailBox602::GetMsgID(uns32 ID)
{
    if (ID == m_ID)
        return NOERROR;

    if (m_FileList)
    {
        lpFreeFileList(&m_FileList);
         m_FileList = NULL;
    }
    return CWBMailBox::GetMsgID(ID);
}
//
// Vyhledani stazene zasilky v databazi
//
// Protoze remote a lokalni zasilky maji ruzne ID, neni mozne hledat v databazi podle ID zasilky,
// ale je treba hledat podle Subjektu adresy odesilatele a casu odeslani. Ovsem je treba pocitat
// s tim, ze adresa odesilatele se trochu lisi u remote a lokalni zasilky a pozice pouzitelneho
// casu odeslani se lisi u POP3 a remote zasilky
//
// Vraci MAIL_MSG_NOT_FOUND, pokud zasilka nebyla nalezena
//
DWORD CWBMailBox602::LocMsgID2Pos(char *Subj, char *Sender, DWORD Date, DWORD Time)
{
    char     Query[240];
    char     Subject[160];
    DWORD    Err = NOERROR;

    m_BodySize = NONEINTEGER;
    m_FileCnt  = NONESHORT;

    char *ps, *pd;
    for (ps = Subj, pd = Subject; *ps; ps++, pd++)
    {
        *pd = *ps;
        if (*ps == '\'' || *ps == '"')
            *++pd = *ps;
    }
    DWORD Len = sprintf(Query, "SELECT * FROM %s WHERE Subject=\"%s\" AND Sender=\"%s\" AND CreDate=", m_InBoxTbl, Subject, Sender);
    char *q = Query + Len;
    date2str(Date, q, 1);
    if (!*q)
        lstrcpy(q, "NONEDATE");
    q += lstrlen(q);
    lstrcpy(q, " AND CreTime=");
    q += 13;
    time2str(Time, q, 1);
    if (!*q)
        lstrcpy(q, "NONETIME");
    tcursnum Curs;
    if (!FindMsg(Query, &Curs))
        goto error;

    if (Curs == NOCURSOR)
    {
        m_CurPos = NORECNUM;
        Err = MAIL_MSG_NOT_FOUND;
        goto exit;
    }
    if (ReadLenC(Curs, ATML_BODY, &m_BodySize))
        goto error;
    if (ReadIndC(Curs, ATML_FILECNT, &m_FileCnt))
        goto error;
    m_CurPos = Translate(Curs);
    if (m_CurPos != NORECNUM)
        goto exit;

error:

    Err = WBError();

exit:

    if (Curs != NOCURSOR)
        CloseCursor(Curs);

    return Err;
}
//
// Stazeni zasilky z materskeho uradu
//
DWORD CWBMailBox602::LoadRemMsg()
{
    PMessageInfoList MsgList = NULL, Msg;
    PMessageInfo     Msgi;
    DWORD Date, Time, Size, Flags;
    DWORD Err;
    BOOL  Trans = FALSE;
    char  Buf[65];
    char  Subject[sizeof(Msgi->Subject)];
    char  Sender[sizeof(Msgi->Odesilatel.JmenoUcastnika)];
    DWORD DT;

    // IF posta neni POP3 ani Remote, neni treba nic stahovat
    if (!(m_MailCtx->m_Type & (WBMT_602REM | WBMT_602SP3)))
        return NOERROR;

    // IF zasilka neni na materskem urade, neni treba nic stahovat
    if (ReadIndL(ATML_FLAGS, &Flags))
        return WBError();
    if (!(Flags & WBL_REMOTE))
        return NOERROR;

    // IF POP3, predat pozadavek POP3 serveru, jinak FaxMailServeru
    if (*m_MailCtx->m_POP3Server)
    {
        TPOP3MInfo *MsgInfo;
        Err = GetMsgInfoPOP3(&MsgInfo);
        if (Err)
            return Err;

        lstrcpy(Subject, MsgInfo->Subject);
        lstrcpy(Sender,  MsgInfo->Sender);
        DT = MsgInfo->Date;

        MsgInfo->Flag = P3RQ_GETMSG;
        Err = RequestPOP3();
        if (Err)
            return Err;
    }
    else
    {
        TMessageInfoList MsgInfo;
        Err = GetMsgInfoRem(&MsgInfo);
        if (Err)
            return Err;
        lstrcpyn(Subject, MsgInfo.MessageInfo.Subject + 1,                   *MsgInfo.MessageInfo.Subject + 1);
        lstrcpyn(Sender,  MsgInfo.MessageInfo.Odesilatel.JmenoUcastnika + 1, *MsgInfo.MessageInfo.Odesilatel.JmenoUcastnika + 1);
        DT  = MsgInfo.MessageInfo.DatumCasOdeslani;
        Err = RequestRem(RMRQ_GETMSG, &MsgInfo);
        if (Err)
            return Err;
        // ? Smazat na rem
    }

    // Vyhledat zasilku v seznamu doslych zasilek
    Err = lpGetMsgList(&MsgList);
    if (Err)
        goto errorm;
    for (Msg = MsgList; Msg; Msg = Msg->Next)
    {
        Msgi = &Msg->MessageInfo;
        if 
        (
            strnicmp(Subject, Msgi->Subject + 1, *Msgi->Subject) == 0 &&
            strnicmp(Sender,  Msgi->Odesilatel.JmenoUcastnika + 1, *Msgi->Odesilatel.JmenoUcastnika) == 0 &&
            DT == (*m_MailCtx->m_POP3Server ? Msgi->DatumCasVzniku : Msgi->DatumCasOdeslani)
        )
        {
            break;
        }
    }
    if (!Msg)
    {
        //DbgLogFile("MAIL_ERROR !Msg");
        Err = MAIL_ERROR;
        goto exit;
    }

    // Prepsat v databazi parametry zasilky
    if (StartTransaction())
        goto error;
    Trans = TRUE;

    Size = *Msg->MessageID + 1;
    memcpy(m_MsgID, Msg->MessageID, Size);
    memset(m_MsgID + Size, 0, MSGID_SIZE - Size);
    Size = Convert(Msgi->Adresat.JmenoUcastnika + 1, Buf, *Msgi->Adresat.JmenoUcastnika, 6);
    Buf[Size] = 0;
    if (WriteIndL(ATML_RECIPIENT, Buf, 64))
        goto error;
    DDTM2WB(Msgi->DatumCas,     &Date, &Time); 
    if (WriteIndL(ATML_RCVDATE, &Date, 4))
        goto error;
    if (WriteIndL(ATML_RCVTIME, &Time, 4))
        goto error;
    lpSizeOfMsg(Msgi, 0, NULL, &Size);
    if (WriteIndL(ATML_SIZE, &Size, 4))
        goto error;
    if (WriteIndL(ATML_FILECNT, &Msgi->PocetSouboru, 2))
        goto error;
    if (WriteIndL(ATML_MSGID, m_MsgID, MSGID_SIZE))
        goto error;
    Flags = 0;  
    if (Msgi->Priznaky & poConfirm)
        Flags |= WBL_READRCPT;
    if (Msgi->Priznaky & poViewed)
        Flags |= WBL_VIEWED;
    if (Msgi->Priznaky & poFAXMessage)
        Flags |= WBL_FAX;
    if (Msgi->EXPriznaky & expoNizkaPriorita)
        Flags |= WBL_PRILOW;
    if (Msgi->EXPriznaky & expoVysokaPriorita)
        Flags |= WBL_PRIHIGH;
    if ((m_MailCtx->m_Type & WBMT_602REM) && !(Msgi->EXPriznaky & expoMistni))
        Flags |= WBL_REMOTE;
    if (Msgi->EXPriznaky & expoZazadanVymaz)
        Flags |= WBL_DELREQ;
    if (WriteIndL(ATML_FLAGS, &Flags, 4))
        goto error;
    m_MsgType = Msgi->Odesilatel.DruhUcastnika;
    Trans = FALSE;
    if (Commit())
        goto error;
    Err = NOERROR;
    goto exit;

error:

    Err = WBError();
    goto exit;

errorm:

    Err = Err602ToszErr(Err);

exit:

    if (Trans)
        RollBack();
    if (MsgList)
        lpFreeMsgList(&MsgList);
    return NOERROR;
}
//
// Stazeni seznamu zasilek z POP3 schranky
//
// Seznam zasilek v POP3 schrance se nepromitne do seznamu zasilek v lokalni schrance (lpGetMsgList),
// proto je potreba informaci o zasilkach zapsat do databaze zde.
//
DWORD CWBMailBox602::LoadPOP3(UINT Param)
{
    TPOP3MInfo *NewList;
    UINT        NewCount, FullCount;
    ULONG       NewSize,  FullSize;
    DWORD       Date, Time;
    DWORD       Err;
    DWORD       Flags = WBL_REMOTE;
    char        PassWord[256];
    char        Subject[81];
    char        Sender[65];
    BOOL        Trans = FALSE;
    BYTE        Stat;

    HWND hLogWnd = m_MailCtx->CreateLogWnd();
    if (!hLogWnd)
    {
        //DbgLogFile("MAIL_ERROR m_MailCtx->CreateLogWnd");
        return MAIL_ERROR;
    }
    DeMix(PassWord, m_MailCtx->m_PassWord);
    PassWord[*PassWord + 1] = 0;

    Err = lpPOP3GetListOfMsgs
          (
              m_OldList,
              m_OldCount,
              &NewCount,
              &NewSize, 
              m_MailCtx->m_POP3Server,
              m_MailCtx->m_POP3Name,
              PassWord + 1,
              m_MailCtx->m_Apop,
              &NewList,
              &FullCount,
              &FullSize,
              hLogWnd,
              hInst,
              12345
          );
    //DbgLogFile("POP3GetListOfMessages = %d", Err);
    if (Err)
        goto errorm;

    //DWORD Res;
    //char  Cmd[128];
    //sprintf(Cmd, "DELETE FROM %s WHERE ((Flags DIV 2048) MOD 2) = 1", m_InBoxTbl);
    //if (SQL_execute(Cmd, &Res);
    //    goto error;

    TPOP3MInfo *Msg;
    for (Msg = NewList; Msg < NewList + NewCount; Msg++)
    {
        DWORD Size = lstrlen(Msg->MsgId);
        lstrcpy((char *)m_MsgID, Msg->MsgId);
        memset(m_MsgID + Size, 0, MSGID_SIZE - Size);
        DDTM2WB(Msg->Date, &Date, &Time);
        Convert(Msg->Subject, Subject, 80, 6);
        Subject[80] = 0;
        Convert(Msg->Sender, Sender, 64, 6);
        Sender[64] = 0;
        //DbgLogFile("MsgPOP3: %s", Subject);

        if (StartTransaction())
            goto error;
        Trans = TRUE;
        Stat  = MBS_OLD;

        // IF zaznam nenalezen mezi lokalnimi zasilkami
        Err   = LocMsgID2Pos(Msg->Subject, Msg->Sender, Date, Time);
        if (Err)
        {
            if (Err != MAIL_MSG_NOT_FOUND)
                goto exit;
            // IF zaznam nenalezen mezi remote zasilkami
            Err   = MsgID2Pos();
            if (Err)
            {
                if (Err != MAIL_MSG_NOT_FOUND)
                    goto exit;
                m_CurPos = InsertL();
                if (m_CurPos == NORECNUM)
                    goto error;
                if (WriteIndL(ATML_SUBJECT, Subject, 80))
                    goto error;
                if (WriteIndL(ATML_SENDER,  Sender, 64))
                    goto error;
                if (WriteIndL(ATML_CREDATE, &Date, 4))
                    goto error;
                if (WriteIndL(ATML_CRETIME, &Time, 4))
                    goto error;
                //DDTM2WB(Msgi->DatumCas,       &Date, &Time); 
                if (WriteIndL(ATML_RCVDATE, &Date, 4))
                    goto error;
                if (WriteIndL(ATML_RCVTIME, &Time, 4))
                    goto error;
                //lpSizeOfMsg(Msgi, 0, NULL, &Size);
                if (WriteIndL(ATML_SIZE, &Msg->Size, 4))
                    goto error;
                //if (WriteIndL(ATML_FILECNT, &Msgi->PocetSouboru, 2))
                //    goto error;
                if (WriteIndL(ATML_MSGID, m_MsgID, MSGID_SIZE))
                    goto error;
                Stat = MBS_NEW;
            }
        }
        if (WriteIndL(ATML_MSGID, m_MsgID, MSGID_SIZE))
            goto error;
        if (WriteIndL(ATML_FLAGS, &Flags, 4))
            goto error;
        if (WriteIndL(ATML_STAT, &Stat, 1))
            goto error;
        if (WriteIndL(ATML_PROFILE, m_MailCtx->m_Profile, 64))
            goto error;
    
        Trans = FALSE;
        if (Commit())
            goto error;
    }
    
    SetOldList(NewList, FullCount);

    // IF take tela nebo seznam souboru, stahnout vsechny zasilky
    if (Param)
    {
        for (Msg = NewList; Msg < NewList + NewCount; Msg++)
            Msg->Flag = P3RQ_GETMSG;
        Err = RequestPOP3();
        if (Err)
            goto exit;
    }

    Err = NOERROR;
    goto exit;

error:

    Err = WBError();
    goto exit;

errorm:

    Err = (Err <= 15) ? MAIL_CONNECT_FAILED : MAIL_SOCK_IO_ERROR;

exit:

    if (Trans)
        RollBack();
    return Err;
}
//
// Vyhledani zasilky v seznamu zasilek v POP3 schrance
//
DWORD CWBMailBox602::GetMsgInfoPOP3(TPOP3MInfo **MsgInfo)
{
    DWORD Err;
    // IF nemam seznam zasilek, pozadat o jeho stazeni
    if (!m_OldList)
    {
        Err = LoadPOP3(0);
        if (Err)
            return Err;
    }
    // Vyhledat podle ID zasilky
    for (TPOP3MInfo *pInfo = m_OldList; pInfo < m_OldList + m_OldCount; pInfo++)
    {
        if (strcmp((char *)m_MsgID, pInfo->MsgId) == 0)
        {
            *MsgInfo = pInfo;
            return NOERROR;
        }
    }
    return MAIL_MSG_NOT_FOUND;    
}
//
// Zrusi stary seznam zasilek v POP3 schrance a nahradi ho aktualnim
//
void CWBMailBox602::SetOldList(TPOP3MInfo *NewList, UINT FullCount)
{
    if (m_OldList)
    {
        lpPOP3FreeListOfMsgs(m_OldList, m_OldCount);
        m_OldList  = NULL;
        //m_NextPOP3 = NULL;
        m_OldCount = 0;
    }
    if (FullCount)
    {
        m_OldList  = NewList;
        m_OldCount = FullCount;
    }
}
//
// Pozadavek na POP3 server
//
DWORD CWBMailBox602::RequestPOP3()
{
    TPOP3MInfo *NewList;
    UINT        NewCount, FullCount;
    ULONG       NewSize,  FullSize;
    DWORD       Err;
    char        PassWord[256];

    HWND hLogWnd = m_MailCtx->CreateLogWnd();
    if (!hLogWnd)
    {
        //DbgLogFile("MAIL_ERROR m_MailCtx->CreateLogWnd");
        return MAIL_ERROR;
    }
    DeMix(PassWord, m_MailCtx->m_PassWord);
    PassWord[*PassWord + 1] = 0;

    Err = lpPOP3GetOrEraseMsgs
          (
              m_MailCtx->m_EMI, 
              NULL,             // may be NULL; ma smysl pro sitoveho klienta
              m_OldList,        // seznam, podle ktereho
              m_OldCount,       // se bude mazat / stahovat
              &NewList,
              &NewCount,        // nove hodnoty, pokud se zjisti
              &NewSize,         // nove
              &FullCount,       // celkem
              &FullSize,        // celkem
              m_MailCtx->m_UserID,
              m_MailCtx->m_POP3Server,
              m_MailCtx->m_POP3Name,
              PassWord + 1,
              m_MailCtx->m_Apop,
              FALSE,
              hLogWnd,
              hInst,
              12345
          );
    //DbgLogFile("POP3GetOrEraseMsgs = %d", Err);
    DestroyWindow(hLogWnd);
    if (Err)
        return Err <= 15 ? MAIL_CONNECT_FAILED : MAIL_SOCK_IO_ERROR;
    SetOldList(NewList, FullCount);
    return NOERROR;
}
// 
// Stazeni seznamu zasilek ze vzdaleneho materskeho uradu
//
// Seznam zasilek na materskem uradu se promitne do seznamu zasilek v lokalni schrance (lpGetMsgList),
// proto muzu informaci o zasilkach zapsat do databaze ve funkci Load.
//
DWORD CWBMailBox602::LoadRem(UINT Param)
{
    if (!RemoteCtx.m_hGateThread)
        return NOERROR;

    // Pozadat o seznam zasilek v remote schrance
    PMessageInfoList   MsgList = NULL;
    DWORD Err = RequestRem(Param ? RMRQ_GETNEWLIST : RMRQ_GETLIST, MsgList);
    return Err;
/*
    DWORD Err = RequestRem(RMRQ_GETLIST, MsgList);
    if (Err)
        return Err;
    if (!Param)
        return NOERROR;
#ifdef DEBUG
    int i = 0;
    for (PMessageInfoList Msg = m_RemMsgList; Msg; Msg = Msg->Next)
        i++;
    DbgPrint("New Msgs %d", i);
#endif
    // IF tela nebo seznam souboru, stahnout vsechny zasilky
    return RequestRem(RMRQ_GETMSG, m_RemMsgList);
    */

}
//
// Vyhledani zasilky v seznamu zasilek na vzdalenem materskem uradu
//
DWORD CWBMailBox602::GetMsgInfoRem(PMessageInfoList MsgInfo)
{
    DWORD Err;
    // IF nemam seznam zasilek, stahnout
    if (!m_RemMsgList)
    {
        Err = RequestRem(RMRQ_GETLIST, NULL);
        if (Err)
            return Err;
    }
    PMessageInfoList Msg;
    // Vyhledat zasiku porovnanim m_MsgID s casem odeslani, subjektem a adresou odesilatele
    for (Msg = m_RemMsgList; Msg; Msg = Msg->Next)
    {
        PMessageInfo Msgi = &Msg->MessageInfo;
        if (*(DWORD *)m_MsgID == Msgi->DatumCasOdeslani)
        {
            BYTE *p    = m_MsgID + 4;
            DWORD Rest = MSGID_SIZE - 4;
            DWORD Size = *p + 1;
            if (Size > Rest)
                Size = Rest;
            if (memcmp(p, Msgi->Subject, Size) == 0)
            {
                p    += Size;
                Rest -= Size;
                if (Rest = 0)
                    break;
                Size = *p + 1;
                if (Size > Rest)
                    Size = Rest;
                if (memcmp(p, Msgi->Odesilatel.JmenoUcastnika, Size) == 0)
                    break;
            }
        }
    }
    if (!Msg)
        return MAIL_MSG_NOT_FOUND;
    
    memcpy(MsgInfo, Msg, sizeof(TMessageInfoList));
    MsgInfo->Next = NULL;
    return NOERROR;
}
//
// Pozadavek na FaxMailServer
//
DWORD CWBMailBox602::RequestRem(WORD Req, PMessageInfoList MsgList, BOOL Send)
{
    DbgPrint("Wait IN");
    WaitForSingleObject(RemoteCtx.m_GateON, INFINITE);
    DbgPrint("Wait OUT");

    // Zaradit pozadavek do vystupni fronty
    //DbgLogFile("SendGBAT");
    DWORD Err = lpSendGBATFromRMMsgList(Req, &MsgList, "", 0, (DWORD)-1, 100);
    //DbgLogFile("SendGBAT %d", Err);
    if (Err)
        return Err602ToszErr(Err);

    // pokud nebyla pomoci Dial inicializovana gate
    if (!RemoteCtx.m_hGateThread)
        return MAIL_REQ_QUEUED;
    // IF uz je zaveseno, nechame to na priste
    if (!Send)
        return MAIL_REQ_QUEUED;

    int i, j, k;
    // k = pocet zasilek v schrance
    lpGetMailBoxStatusInfo(&k, &j, &j, &j);
    ResetEvent(RemoteCtx.m_GateON);
    ResetEvent(RemoteCtx.m_GateRec);
    // Znovu prohlednout vystupni frontu
    //DbgLogFile("ForceReread IN");
    lpForceReread();
    //DbgLogFile("ForceReread OUT");
    // Pockat az prijde zasilka, nebo FaxMailServer skonci
    UINT Event = WaitForMultipleObjects(2, &RemoteCtx.m_GateON, FALSE, INFINITE);
    if (Event == WAIT_OBJECT_0 + 1)
    {
        // Potom co FaxMailServer oznami ze zasilka dosla je treba jeste chvili
        // pockat nez se objevi v obsahu schranky
        do 
        {
            if (WaitForSingleObject(RemoteCtx.m_GateON, 500) == WAIT_OBJECT_0)
            {
                //DbgLogFile("WaitForBox GateON OFF");
                break;
            }
            //DbgLogFile("WaitForBox");
            lpGetMailBoxStatusInfo(&i, &j, &j, &j);
            if (k > i)
                k = i;
        }
        while (i <= k); 

        if (m_RemMsgList)
        {
            //DbgLogFile("FreeMsgList IN");
            lpFreeMsgList(&m_RemMsgList);
            //DbgLogFile("FreeMsgList OUT");
        }

        PMessageInfoList *pRemNext;
        PMessageInfoList *pNext;

        //DbgLogFile("GetMsgList IN");
        Err = lpGetMsgList(&MsgList);
        //DbgLogFile("GetMsgList OUT %d", Err);
        if (Err)
            return Err602ToszErr(Err);
    
        // Ze seznamu zasilek ve schrance, oddelit seznam remote zasilek
        m_RemMsgList = NULL;      
        pRemNext     = &m_RemMsgList; 
        pNext        = &MsgList;

        for (PMessageInfoList Msg = MsgList; Msg; Msg = Msg->Next)
        {
            if (!(Msg->MessageInfo.EXPriznaky & expoMistni) || !(Msg->MessageInfo.Priznaky & poViewed))
            {
                *pNext    =  Msg->Next;
                *pRemNext =  Msg;
                 pRemNext = &Msg->Next;
            }
            else
            {
                 pNext     = &Msg->Next;
            }            
        }
        *pRemNext = NULL;
        *pNext    = NULL;
        // Seznam lokalnich zasilek muzeme zahodit
        if (MsgList)
        {
            //DbgLogFile("FreeMsgList IN");
            lpFreeMsgList(&MsgList);
            //DbgLogFile("FreeMsgList OUT");
        }
    }
    else if (Event == WAIT_OBJECT_0)
    {
        //DbgLogFile("RemoteReq GateON");
        SetEvent(RemoteCtx.m_GateON);
        Err = MAIL_CONNECT_FAILED;
    }
    return Err;
}
//
// Ulozeni pripojeneho souboru na disk
//
DWORD CAttStream602fi::CopyFrom(LPVOID SrcFile, int Flags)
{
    char DstPath[MAX_PATH];
    ctopas((char *)m_DstPath, DstPath);
    DWORD Err = lpCopyFileFromMsg((LPSTR)m_MsgID, (LPCSTR)SrcFile, DstPath);
    if (Err)
        Err = Err602ToszErr(Err);
    return Err;
}
//
// Ulozeni pripojeneho souboru do databaze
//
DWORD CAttStream602di::CopyFrom(LPVOID SrcFile, int Flags)
{
    char DstPath[MAX_PATH];
    TempFileName(DstPath);
    ctopas(DstPath, DstPath);
    DstPath[*DstPath + 1] = 0;
    DWORD Err = lpCopyFileFromMsg((LPSTR)m_MsgID, (LPCSTR)SrcFile, DstPath);
    DbgLogWrite("CopyFileFromMsg = %d", Err);
    if (Err)
        return Err602ToszErr(Err);
    FHANDLE hFile = CreateFile(DstPath + 1, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    DbgLogWrite("CreateFile = %08X", hFile);
    if (!FILE_HANDLE_VALID(hFile))
        return OS_FILE_ERROR;
    uns32 Size;
    Err = OUT_OF_APPL_MEMORY;
    char *Buf = BufAlloc(GetFileSize(hFile, NULL), &Size);
    DbgLogWrite("BufAlloc = %08X", Buf);
#ifdef UNIX
    lseek(hFile, 0, 0);
#endif
    if (Buf)
    {
        Err = CAttStreamd::Open(TRUE);
        DbgLogWrite("CAttStreamd::Open = %d", Err);
        if (Err)
            return Err;
        for (;;)
        { DWORD sz;
            if (!ReadFile(hFile, Buf, Size, &sz, NULL))
            {
                Err = OS_FILE_ERROR;
                DbgLogWrite("ReadFile = %d", Err);
                break;
            }
            if (!sz)
            {
                Err = NOERROR;
                break;
            }
            t_varcol_size vsz;
            Err = CAttStreamd::Write(m_Offset, Buf, sz, &vsz);
            DbgLogWrite("CAttStreamd::Write = %d", Err);
            if (Err)
                break;
        }
        free(Buf);
    }
    CloseFile(hFile);
    DeleteFile(DstPath + 1);
    return Err;
}
#endif // MAIL602

//
// CWBMailBoxMAPI - Postovni schranka MAPI
//
//
// Konstruktor
//
CWBMailBoxMAPI::CWBMailBoxMAPI(CWBMailCtx *MailCtx) : CWBMailBox(MailCtx) 
{
    m_lpMessage  = NULL;
    m_lpFilesSet = NULL;
}
//
// Destruktor
//
CWBMailBoxMAPI::~CWBMailBoxMAPI()
{
    if (m_lpInBox)
        m_lpInBox->Release();
    ReleaseLastMsg();
}
//
// Uvolneni kontextu minule zasilky
//
void CWBMailBoxMAPI::ReleaseLastMsg()
{
    if (m_lpMessage)
    {
        m_lpMessage->Release();
        m_lpMessage = NULL;
    }
    if (m_lpFilesSet)
    {
        lpMAPIFreeBuffer(m_lpFilesSet);
        m_lpFilesSet = NULL;
    }
}
//
// Inicializace schranky
//
DWORD CWBMailBoxMAPI::Open()
{
    UINT cp = GetACP();
    if (cp == 1250)
        m_SrcLang = 1;
    else if (cp == 1252)
        m_SrcLang = 3;
    else
        m_SrcLang = 0;
    return m_MailCtx->GetBox(&m_lpInBox, FALSE);
}
//
// Nacteni seznamu zasilek
//
DWORD CWBMailBoxMAPI::Load(UINT Param)
{
    LPMAPITABLE lpMsgTable = NULL;
    LPSRowSet   lpMsgSet   = NULL;
    HRESULT     hrErr;
    DWORD       Date, Time, Flags;
    DWORD       Size;
    DWORD       Err;
    BOOL        Trans = FALSE;
    BYTE        Stat;

    // Otevrit schranku 
    hrErr = m_lpInBox->GetContentsTable(0, &lpMsgTable);
    if (hrErr != NOERROR)
        goto errorm;

    // Atributy zprav
    BYTE ColDescA[sizeof(SPropTagArray) + (PRID_MSIZE - 1) * sizeof(ULONG)]; 
    LPSPropTagArray ColDesc;
    ColDesc = (LPSPropTagArray)ColDescA;
    ColDesc->cValues  = PRID_MSIZE;
    ColDesc->aulPropTag[PRID_ENTRYID              ] = PR_ENTRYID;
    ColDesc->aulPropTag[PRID_SUBJECT              ] = PR_SUBJECT;
    ColDesc->aulPropTag[PRID_SENDER_NAME          ] = PR_SENDER_NAME;
    ColDesc->aulPropTag[PRID_SENDER_EMAIL_ADDRESS ] = PR_SENDER_EMAIL_ADDRESS;
    ColDesc->aulPropTag[PRID_DISPLAY_TO           ] = PR_DISPLAY_TO;
    ColDesc->aulPropTag[PRID_CREATION_TIME        ] = PR_CREATION_TIME;
    ColDesc->aulPropTag[PRID_MESSAGE_DELIVERY_TIME] = PR_MESSAGE_DELIVERY_TIME;
    ColDesc->aulPropTag[PRID_MESSAGE_FLAGS        ] = PR_MESSAGE_FLAGS;
    ColDesc->aulPropTag[PRID_PRIORITY             ] = PR_PRIORITY;
    ColDesc->aulPropTag[PRID_SENSITIVITY          ] = PR_SENSITIVITY;
    ColDesc->aulPropTag[PRID_READ_RECEIPT         ] = PR_READ_RECEIPT_REQUESTED;
    ColDesc->aulPropTag[PRID_MESSAGE_SIZE         ] = PR_MESSAGE_SIZE;
    ColDesc->aulPropTag[PRID_BODY                 ] = PR_BODY;
    ColDesc->aulPropTag[PRID_HASATTACH            ] = PR_HASATTACH;
    hrErr = lpMsgTable->SetColumns(ColDesc, 0);
    if (hrErr != NOERROR)
        goto errorm;

    // Nacist seznam zasilek
    DWORD cMsgs;
    hrErr = lpMsgTable->GetRowCount(0, &cMsgs);
    if (hrErr != NOERROR || cMsgs == 0)
        goto errorm;

    hrErr = lpMsgTable->QueryRows(cMsgs, 0, &lpMsgSet);
    if (hrErr != NOERROR)
        goto exit;

    int i;
    for (i = 0; i < cMsgs; i++)
    {
        LPSPropValue Prop = lpMsgSet->aRow[i].lpProps;
        Size = Prop[PRID_ENTRYID].Value.bin.cb;
        *(DWORD *)m_MsgID = Size;
        memcpy(m_MsgID + 4,    Prop[PRID_ENTRYID].Value.bin.lpb, Size);
        memset(m_MsgID + Size + 4, 0, MSGID_SIZE - Size - 4);
#ifdef DEBUG
        DbgLogFile("CWBMailBoxMAPI::Load(%d)", i);
        if (Prop[PRID_SUBJECT].ulPropTag == PR_SUBJECT)
            DbgLogFile("Msg: %.240s", Prop[PRID_SUBJECT].Value.lpszA);
        else
            DbgLogFile("Msg: %08X, %08X", Prop[PRID_SUBJECT].ulPropTag, Prop[PRID_SUBJECT].Value.lpszA);
#endif
        if (StartTransaction())
            goto error;
        Trans = TRUE;
        
        // IF zasilka neni v databazi, vlozit novy zaznam
        Err = MsgID2Pos();
        DbgLogFile("   MsgID2Pos = %d", Err);
        if (Err)
        {
            char *ps;
            char  Buf[80];
            if (Err != MAIL_MSG_NOT_FOUND)
                goto exit;
            m_CurPos = InsertL();
            DbgLogFile("   InsertL = %d", m_CurPos);
            if (m_CurPos == NORECNUM)
                goto error;
            if (Prop[PRID_SUBJECT].ulPropTag == PR_SUBJECT)
            {
                ps = Prop[PRID_SUBJECT].Value.lpszA;
                Convert(ps, Buf, sizeof(Buf) - 1, m_SrcLang);
                DbgLogFile("   WriteIndL(ATML_SUBJECT)");
                if (WriteIndL(ATML_SUBJECT, Buf, sizeof(Buf)))
                    goto error;
            }
            if (Prop[PRID_SENDER_EMAIL_ADDRESS].ulPropTag == PR_SENDER_EMAIL_ADDRESS)
            {
                if (WriteIndL(ATML_SENDER, Prop[PRID_SENDER_EMAIL_ADDRESS].Value.lpszA, 64))
                    goto error;
            }
            else
            {
                ps = Prop[PRID_SENDER_NAME].Value.lpszA;
                Convert(ps, Buf, 64, m_SrcLang);
                if (WriteIndL(ATML_SENDER, Buf, 64))
                    goto error;
            }
            hrErr = WriteAddressees();
            DbgLogFile("   WriteAddressees = %08X", hrErr);
            if (hrErr)
                goto errorm;
            if (m_Addressp > m_Addressees)
            {
                if (WriteVarL(ATML_ADDRESSEES, m_Addressees, (int)(m_Addressp - m_Addressees)))
                    goto error;
                DbgLogFile("   WriteVarL(ATML_ADDRESSEES)");
                if (WriteLenL(ATML_ADDRESSEES, (int)(m_Addressp - m_Addressees)))
                    goto error;
            }
            char *pb;
            if (m_Addressp > m_Addressees && (pb = strchr(m_Addressees, '<')))
            {
                pb++;
                char *pe = strchr(pb, '>');
                *pe = 0;
                DbgLogFile("   WriteVarL(ATML_RECIPIENT)");
                if (WriteIndL(ATML_RECIPIENT, pb, 64))
                    goto error;
                *pe = '>';
            }
            else if (Prop[PRID_DISPLAY_TO].ulPropTag == PR_DISPLAY_TO)
            {
                ps = strchr(Prop[PRID_DISPLAY_TO].Value.lpszA, ';');
                if (ps)
                    *ps = 0;
                ps = Prop[PRID_DISPLAY_TO].Value.lpszA;
                Convert(ps, Buf, 64, m_SrcLang);
                if (WriteIndL(ATML_RECIPIENT, Buf, 64))
                    goto error;
            }
            FDTM2WB(*(LONGLONG *)&Prop[PRID_CREATION_TIME].Value.ft, &Date, &Time);
            if (WriteIndL(ATML_CREDATE, &Date, 4))
                goto error;
            if (WriteIndL(ATML_CRETIME, &Time, 4))
                goto error;
            if (Prop[PRID_MESSAGE_DELIVERY_TIME].ulPropTag == PR_MESSAGE_DELIVERY_TIME)
            {
                FDTM2WB(*(LONGLONG *)&Prop[PRID_MESSAGE_DELIVERY_TIME].Value.ft, &Date, &Time); 
                if (WriteIndL(ATML_RCVDATE, &Date, 4))
                    goto error;
                if (WriteIndL(ATML_RCVTIME, &Time, 4))
                    goto error;
            }
            if (WriteIndL(ATML_MSGID, m_MsgID, MSGID_SIZE))
                goto error;
            Stat = MBS_NEW;
        }
        else
        {
            Stat = MBS_OLD;
        }
        // Prepsat priznaky
        Flags = 0;  
        if (Prop[PRID_READ_RECEIPT].ulPropTag == PR_READ_RECEIPT_REQUESTED && Prop[PRID_READ_RECEIPT].Value.b)
            Flags |= WBL_READRCPT;
        if (Prop[PRID_MESSAGE_FLAGS].Value.l & MSGFLAG_READ)
            Flags |= WBL_VIEWED;
        if (Prop[PRID_PRIORITY].ulPropTag == PR_PRIORITY)
        {
            if (Prop[PRID_PRIORITY].Value.l == PRIO_NONURGENT)
                Flags |= WBL_PRILOW;
            if (Prop[PRID_PRIORITY].Value.l == PRIO_URGENT)
                Flags |= WBL_PRIHIGH;
        }
        if (Prop[PRID_SENSITIVITY].ulPropTag == PR_SENSITIVITY)
        {
            if (Prop[PRID_SENSITIVITY].Value.l == SENSITIVITY_PERSONAL)
                Flags |= WBL_SENSPERS;
            if (Prop[PRID_SENSITIVITY].Value.l == SENSITIVITY_PRIVATE)
                Flags |= WBL_SENSPRIV;
            if (Prop[PRID_SENSITIVITY].Value.l == SENSITIVITY_COMPANY_CONFIDENTIAL)
                Flags |= WBL_SENSCONF;
        }
        if (WriteIndL(ATML_FLAGS, &Flags, 4))
            goto error;
        if (WriteIndL(ATML_STAT,  &Stat,  1))
            goto error;
        // IF zasilka ma telo a jeste nebylo zpracovano
        if (m_BodySize == NONEINTEGER)
        {
            // IF taky telo, ulozit telo do tabulky
            if (Param & (MBL_BODY | MBL_MSGONLY))
            {
                if (Prop[PRID_BODY].ulPropTag == PR_BODY)
                {
                    Size = lstrlen(Prop[PRID_BODY].Value.lpszA);
                    DbgLogFile("   WriteVarL(ATML_BODY)");
                    if (WriteVarL(ATML_BODY, Prop[PRID_BODY].Value.lpszA, Size))
                        goto error;
                    if (WriteLenL(ATML_BODY, Size))
                        goto error;
                    if (WriteIndL(ATML_SIZE, &Size, 4))
                        goto error;
                }
                else
                {
                    Err = GetMsg(Param);
                    DbgLogFile("   GetMsg() = %d", Err);
                    if (Err)
                        goto exit;
                }
            }
            // ELSE IF telo je string, zapsat delku do tabulky
            else
            {
                DWORD Size;
                if (Prop[PRID_BODY].ulPropTag == PR_BODY)
                {
                    Size = lstrlen(Prop[PRID_BODY].Value.lpszA);
                    if (WriteIndL(ATML_SIZE, &Size, 4))
                        goto error;
                }
            }
        }
        // IF seznam pripojenych souboru a jeste nebyl zpracovan
        if (m_FileCnt == (WORD)NONESHORT)
        {
            // IF zasilka ma pripojene soubory
            if (Prop[PRID_HASATTACH].Value.b)
            {
                // IF take seznam souboru
                if (Param & MBL_FILES)
                {
                    Err = GetFilesInfo();
                    DbgLogFile("   GetFilesInfo() = %d", Err);
                    if (Err)
                        goto exit;
                }
            }
            else
            {
                WORD Cnt = 0;
                if (WriteIndL(ATML_FILECNT, &Cnt, 2))
                    goto error;
            }
        }
        if (WriteIndL(ATML_PROFILE, m_MailCtx->m_Profile, 64))
            goto error;
       
        Trans = FALSE;
        DbgLogFile("   Commit()");
        if (Commit())
            goto error;
        DbgLogFile("   ReleaseLastMsg()");
        ReleaseLastMsg();
        DbgLogFile("CWBMailBoxMAPI::Load(%d) = OK", i);
    }
    Err = NOERROR;
    goto exit;

error:

    Err = WBError();
    goto exit;

errorm:

    Err = hrErrToszErr(hrErr);

exit:

    if (Trans)
        RollBack();
    if (lpMsgSet)
        lpMAPIFreeBuffer(lpMsgSet);
    if (lpMsgTable)
        lpMsgTable->Release();
    return Err;
}
//
// Writes Addressees from message into m_Addressees list
//
HRESULT CWBMailBoxMAPI::WriteAddressees()
{
    LPMAPITABLE lpRecipTable = NULL;
    LPSRowSet   lpRecipSet   = NULL;
    ULONG       ObjType;

    m_Addressp = m_Addressees;
    if (m_lpMessage)
        ReleaseLastMsg();
    HRESULT hrErr = m_lpInBox->OpenEntry(*(DWORD *)m_MsgID, (LPENTRYID)(m_MsgID + 4), NULL, 0, &ObjType, (LPUNKNOWN *)&m_lpMessage);
    if (hrErr != NOERROR)
        return hrErr;

    hrErr = m_lpMessage->GetRecipientTable(0, &lpRecipTable);
    if (hrErr != NOERROR)
        goto error;

    BYTE ColDescA[sizeof(SPropTagArray) + (PRID_RSIZE - 1) * sizeof(ULONG)]; 
    LPSPropTagArray ColDesc;
    ColDesc = (LPSPropTagArray)ColDescA;
    ColDesc->cValues  = PRID_RSIZE;
    ColDesc->aulPropTag[PRID_RECIPIENT_TYPE] = PR_RECIPIENT_TYPE;
    ColDesc->aulPropTag[PRID_DISPLAY_NAME  ] = PR_DISPLAY_NAME;
    ColDesc->aulPropTag[PRID_EMAIL_ADDRESS ] = PR_EMAIL_ADDRESS;
    hrErr = lpRecipTable->SetColumns(ColDesc, 0);
    if (hrErr != NOERROR)
        goto error;
    DWORD cMsgs;
    hrErr = lpRecipTable->GetRowCount(0, &cMsgs);
    if (hrErr != NOERROR || cMsgs == 0)
        goto error;

    hrErr = lpRecipTable->QueryRows(cMsgs, 0, &lpRecipSet);
    if (hrErr != NOERROR)
        goto error;

    int i;
    for (i = 0; i < cMsgs; i++)
    {
        LPSPropValue Prop = lpRecipSet->aRow[i].lpProps;
        DWORD sz = 0;
        BOOL Name = Prop[PRID_DISPLAY_NAME].ulPropTag == PR_DISPLAY_NAME;
        if (Name)
            sz += lstrlen(Prop[PRID_DISPLAY_NAME].Value.lpszA);
        if (Prop[PRID_EMAIL_ADDRESS].ulPropTag == PR_EMAIL_ADDRESS)
        {
            sz += lstrlen(Prop[PRID_EMAIL_ADDRESS].Value.lpszA);
            if (Name && lstrcmp(Prop[PRID_DISPLAY_NAME].Value.lpszA, Prop[PRID_EMAIL_ADDRESS].Value.lpszA) == 0)
                Name = FALSE;
        }
        if (!ReallocAddrs(sz + 8))
        {
            hrErr = E_OUTOFMEMORY;
            goto error;
        }
        if (Prop[PRID_RECIPIENT_TYPE].Value.l == MAPI_TO)
            *(DWORD *)m_Addressp = ADDR_TO;
        else
            *(DWORD *)m_Addressp = ADDR_CC;
        m_Addressp += 4;
        if (Name)
        {
            char *ps   = Prop[PRID_DISPLAY_NAME].Value.lpszA;
            sz = Convert(ps, m_Addressp, lstrlen(ps), m_SrcLang);
            m_Addressp += sz;
            *m_Addressp++ = ' ';
        }
        if (Prop[PRID_EMAIL_ADDRESS].ulPropTag == PR_EMAIL_ADDRESS)
        {
            *m_Addressp++ = '<';
            lstrcpy(m_Addressp, Prop[PRID_EMAIL_ADDRESS].Value.lpszA);
            m_Addressp += lstrlen(m_Addressp);
            *m_Addressp++ = '>';
        }
        *(WORD *)m_Addressp = '\r' + ('\n' << 8);
        m_Addressp += 2;
    }
    *m_Addressp = 0;

error:

    if (lpRecipSet)
        lpMAPIFreeBuffer(lpRecipSet);
    if (lpRecipTable)
        lpRecipTable->Release();
    return hrErr != NOERROR;
}
//
// Ulozeni tela dopisu do databaze
//
DWORD CWBMailBoxMAPI::GetMsg(DWORD Flags)
{
    LPSPropValue Body = NULL;
    CBuf         Buf;
    HRESULT      hrErr;
    ULONG        ObjType;
    DWORD        Err;

    if ((Flags & ~MBL_FILES) == MBL_HDRONLY)
    {
        if (WriteLenL(ATML_BODY, 0))
            goto error;
        return NOERROR;
    }
    // IF zasilka neni otevrena, otevrit
    if (!m_lpMessage)
    {
        HRESULT hrErr = m_lpInBox->OpenEntry(*(DWORD *)m_MsgID, (LPENTRYID)(m_MsgID + 4), NULL, 0, &ObjType, (LPUNKNOWN *)&m_lpMessage);
        if (hrErr != NOERROR)
            goto errorm;
    }
    ULONG Size, Sz;
    SPropTagArray BodyTag;
    BodyTag.cValues       = 1;
    BodyTag.aulPropTag[0] = PR_BODY;
    hrErr = m_lpMessage->GetProps(&BodyTag, 0, &Size, &Body);
    if (HR_FAILED(hrErr))
        goto errorm;
    Sz = Size = 0;
    if (Body->ulPropTag == PR_BODY)
    {
        char *ptr = Body->Value.lpszA;
        Size = lstrlen(ptr);
        if ((Flags & ~MBL_FILES) != MBL_HDRONLY)
        {
            Sz = Size;
#ifdef SRVR
            wbcharset_t charset = sys_spec.charset;
#else
            wbcharset_t charset = m_cdp->sys_charset;
#endif
            if (wbcharset_t(m_SrcLang).cp() != charset.cp())
            {
                if (!Buf.Alloc(Size + 1))
                {
                    Err = OUT_OF_APPL_MEMORY;
                    goto exit;
                }
                Sz  = Convert(Body->Value.lpszA, Buf, Size, m_SrcLang);
                ptr = Buf;
            }
            if (WriteVarL(ATML_BODY, ptr, Sz))
                goto error;
        }
        if (WriteLenL(ATML_BODY, Sz))
            goto error;
    }
    if (WriteIndL(ATML_SIZE, &Size, 4))
        goto error;
    Err = NOERROR;

error:

    Err = WBError();
    goto exit;

errorm:

    Err = hrErrToszErr(hrErr);

exit:

    if (Body)
        lpMAPIFreeBuffer(Body);
    return Err;
}
//
// Ulozani seznamu pripojenych souboru do databaze
//
DWORD CWBMailBoxMAPI::GetFilesInfo()
{
    LPSTREAM  lpFileStr  = NULL;
    HRESULT   hrErr;
    DWORD     Date, Time;
    DWORD     ID, cFils;
    BOOL      Trans = FALSE;
    DWORD     Err = GetFilesSet(&cFils);
    if (Err)
        goto exit;

    if (ReadIndL(ATML_ID, &ID))
        goto error;

    if (StartTransaction())
        goto error;
    Trans = TRUE;

    char Buf[128];
    sprintf(Buf, "DELETE FROM %s WHERE ID=%d", m_InBoxTbf, ID);
    if (SQLExecute(Buf))
        goto error;

    int i;
    for (i = 0; i < cFils; i++)
    {
        trecnum PosF = InsertF();
        if (PosF == NORECNUM)
            goto error;
        if (WriteIndF(PosF, ATMF_ID, &ID, 4))
            goto error;
       
        LPSPropValue Prop = m_lpFilesSet->aRow[i].lpProps;
        char Path[_MAX_PATH];
        char *p = Path;
        *p = 0;
        if (Prop[PRID_ATTACH_LONG_FILENAME].ulPropTag == PR_ATTACH_LONG_FILENAME)
        {
            if 
            (
                 Prop[PRID_ATTACH_LONG_PATHNAME].ulPropTag == PR_ATTACH_LONG_PATHNAME &&
                *Prop[PRID_ATTACH_LONG_PATHNAME].Value.lpszA
            )
            {
                lstrcpy(p, Prop[PRID_ATTACH_LONG_PATHNAME].Value.lpszA);
                p += lstrlen(p);
                if (p[-1] != '\\')
                    *p++ = '\\';
            }
            if (Prop[PRID_ATTACH_LONG_FILENAME].ulPropTag == PR_ATTACH_LONG_FILENAME)
                lstrcpy(p, Prop[PRID_ATTACH_LONG_FILENAME].Value.lpszA);
            if (*p && WriteIndF(PosF, ATMF_NAME, Path, 254))
                goto error;
            //DbgLogFile("File: %s", Path);
        }

        Err = GetAttachStream(Prop[PRID_ATTACH_NUM].Value.l, Prop[PRID_ATTACH_METHOD].Value.l, &lpFileStr);
        if (Err && Err != MAIL_FILE_DELETED)
            goto exit;
        STATSTG sStat;
        if (!Err)
        {
            if (Prop[PRID_ATTACH_METHOD].Value.l == ATTACH_OLE)
            {
                hrErr = ((LPSTORAGE)lpFileStr)->Stat(&sStat, STATFLAG_DEFAULT); //STATFLAG_NONAME); 
                if (hrErr)
                    goto errorm;
                //LPENUMSTATSTG  lpEnum;
                //if (((LPSTORAGE)lpFileStr)->EnumElements(0, NULL, 0,  &lpEnum) == NOERROR)
                //{
                //    LPSTREAM lps;
                //    STATSTG st;
                //    while (lpEnum->Next(1, &st, NULL) == NOERROR)
                //    {
                //        if (st.type == STGTY_STREAM)
                //        {
                //            char *Buf = new char[st.cbSize.LowPart];
                //            ((LPSTORAGE)lpFileStr)->OpenStream(st.pwcsName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &lps);
                //            lps->Read(Buf, st.cbSize.LowPart, NULL);
                //            delete []Buf;
                //        }
                //    }
                //}
            }
            else
            {
                hrErr = lpFileStr->Stat(&sStat, STATFLAG_DEFAULT); 
                if (hrErr)
                    goto errorm;
            }
            lpFileStr->Release();
            lpFileStr = NULL;
        }
        else // MAIL_FILE_DELETED
        {
            sStat.cbSize.QuadPart = 0;
            Err = NOERROR;
        }
        if (WriteIndF(PosF, ATMF_SIZE, &sStat.cbSize.QuadPart, 4))
            goto error;
        if (Prop[PRID_LAST_MODIFICATION_TIME].ulPropTag == PR_LAST_MODIFICATION_TIME)
        {
            FDTM2WB(*(LONGLONG *)&Prop[PRID_LAST_MODIFICATION_TIME].Value.ft, &Date, &Time);
            if (WriteIndF(PosF, ATMF_DATE, &Date, 4))
                goto error;
            if (WriteIndF(PosF, ATMF_TIME, &Time, 4))
                goto error;
        }
    }
    if (WriteIndL(ATML_FILECNT, &cFils, 2))
        goto error;
    Trans = FALSE;
    Commit();
    goto exit;

error: 

    Err = WBError();
    goto exit;

errorm:

    Err = hrErrToszErr(hrErr);

exit:

    if (Trans)
        RollBack();

    if (lpFileStr)
        lpFileStr->Release();
    return Err;
}
//
// Ulozeni pripojeneho souboru
//
DWORD CWBMailBoxMAPI::SaveFile(int FilIdx, LPCSTR FilName)
{
    //DbgLogFile("SaveFileMAPI %d", FilIdx);
    LPSTREAM  lpFileStr  = NULL;
    HRESULT   hrErr;
    DWORD     cFils;
    DWORD     Err = GetFilesSet(&cFils);
    if (Err)
        goto exit;

    DWORD i;
    if (FilName && *FilName)
    {
        for (i = 0; i < cFils; i++)
        {
            if (lstrcmpi(FilName, m_lpFilesSet->aRow[i].lpProps[PRID_ATTACH_LONG_FILENAME].Value.lpszA) == 0)
                break;
        }
    }
    else
    {
        for (i = 0; i < cFils; i++)
        {
            if (--FilIdx < 0)
                break;
        }
    }
    if (i >= cFils)
    {
        Err = MAIL_NO_MORE_FILES;
        goto exit;
    }
    Err = GetAttachStream(m_lpFilesSet->aRow[i].lpProps[PRID_ATTACH_NUM].Value.l, m_lpFilesSet->aRow[i].lpProps[PRID_ATTACH_METHOD].Value.l, &lpFileStr);
    if (Err)
        goto exit;
    hrErr = m_AttStream->CopyFrom(lpFileStr, m_lpFilesSet->aRow[i].lpProps[PRID_ATTACH_METHOD].Value.l);
    if (hrErr != NOERROR)
        Err = hrErrToszErr(hrErr);

exit:

    if (lpFileStr)
        lpFileStr->Release();
    return Err;
}
//
// Vytvoreni streamu pro zapis pripojeneho souboru do databaze
//
DWORD CWBMailBoxMAPI::CreateDBStream(tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index)
{
    m_AttStream = (CAttStream *) new CAttStreamMAPIdi(m_cdp, Curs, Pos, Attr, Index);
    if (!m_AttStream)
        return OUT_OF_APPL_MEMORY;
    return NOERROR;
}
//
// Vytvoreni streamu pro zapis pripojeneho souboru na disk
//
DWORD CWBMailBoxMAPI::CreateFileStream(LPCSTR DstPath)
{
    m_AttStream = new CAttStreamMAPIfi(DstPath);
    if (!m_AttStream)
        return OUT_OF_APPL_MEMORY;
    return NOERROR;
}
//
// Zruseni zasilky
//
DWORD CWBMailBoxMAPI::DeleteMsg()
{
    SBinary MsgID = {*(DWORD *)m_MsgID, m_MsgID + 4};
    ENTRYLIST DelMsg = {1, &MsgID};
    HRESULT hrErr = m_lpInBox->DeleteMessages(&DelMsg, 0, NULL, 0);
    if (hrErr)
        return hrErrToszErr(hrErr);
    return NOERROR;
}
//
// Konverze z ID zaznamu na ID zasilky
//
// Meni kontext aktualni zasilky
//
DWORD CWBMailBoxMAPI::GetMsgID(uns32 ID)
{
    if (ID == m_ID)
        return NOERROR;

    ReleaseLastMsg();
    return CWBMailBox::GetMsgID(ID);
}
//
// Nacteni seznamu pripojenych souboru
//
DWORD CWBMailBoxMAPI::GetFilesSet(DWORD *cFils)
{
    if (m_lpFilesSet)
        return NOERROR;

    LPMAPITABLE lpFilesTable = NULL;
    LPMESSAGE   lpMessage    = NULL;
    HRESULT     hrErr;
    ULONG       ObjType;

    if (!m_lpMessage)
    {
        hrErr = m_lpInBox->OpenEntry(*(DWORD *)m_MsgID, (LPENTRYID)(m_MsgID + 4), NULL, 0, &ObjType, (LPUNKNOWN *)&m_lpMessage);
        if (hrErr != NOERROR)
            goto errorm;
    }
    hrErr = m_lpMessage->GetAttachmentTable(0, &lpFilesTable);
    if (hrErr != NOERROR)
        goto errorm;

    // Z atributu priloh me zajima jen ATTACH_NUM, jmeno souboru, velikost a datum
    char ar[sizeof(SPropTagArray) + (PRID_FSIZE - 1) * sizeof(ULONG)];
    SPropTagArray *FilDesc;
    FilDesc = (SPropTagArray *)ar;
    FilDesc->cValues = PRID_FSIZE;
    FilDesc->aulPropTag[PRID_ATTACH_NUM            ] = PR_ATTACH_NUM;
    FilDesc->aulPropTag[PRID_ATTACH_LONG_FILENAME  ] = PR_ATTACH_LONG_FILENAME;
    FilDesc->aulPropTag[PRID_ATTACH_LONG_PATHNAME  ] = PR_ATTACH_LONG_PATHNAME;
    FilDesc->aulPropTag[PRID_ATTACH_SIZE           ] = PR_ATTACH_SIZE;
    FilDesc->aulPropTag[PRID_LAST_MODIFICATION_TIME] = PR_LAST_MODIFICATION_TIME;
    FilDesc->aulPropTag[PRID_ATTACH_METHOD         ] = PR_ATTACH_METHOD;
    FilDesc->aulPropTag[PRID_ATTACH_FILENAME       ] = PR_ATTACH_FILENAME;
    FilDesc->aulPropTag[PRID_ATTACH_PATHNAME       ] = PR_ATTACH_PATHNAME;
    FilDesc->aulPropTag[PRID_ATTACH_TRANSPORT_NAME ] = PR_ATTACH_TRANSPORT_NAME;
    hrErr = lpFilesTable->SetColumns(FilDesc, 0);
    if (hrErr != NOERROR)
        goto errorm;

    // Nacist seznam pripojenych souboru
    hrErr = lpFilesTable->GetRowCount(0, cFils);
    if (hrErr != NOERROR)
        goto errorm;
    hrErr = lpFilesTable->QueryRows(*cFils, 0, &m_lpFilesSet);

errorm:
    
    if (lpFilesTable)
        lpFilesTable->Release();
    if (hrErr)
        return hrErrToszErr(hrErr);
    return NOERROR;
}
//
// Vraci ukazatel na stream pripojeneho souboru
//
DWORD CWBMailBoxMAPI::GetAttachStream(DWORD Idx, DWORD Meth, LPSTREAM *lpFileStr)
{
    LPATTACH  lpAttach = NULL;
    ULONG     ObjType;

    HRESULT   hrErr;
    if (!m_lpMessage)
    {
        hrErr = m_lpInBox->OpenEntry(*(DWORD *)m_MsgID, (LPENTRYID)(m_MsgID + 4), NULL, 0, &ObjType, (LPUNKNOWN *)&m_lpMessage);
        if (hrErr != NOERROR)
            goto error;
    }
    hrErr    = m_lpMessage->OpenAttach(Idx, NULL, MAPI_BEST_ACCESS, &lpAttach);
    if (hrErr != NOERROR)
        goto error;
    if (Meth == ATTACH_OLE)
        hrErr = lpAttach->OpenProperty(PR_ATTACH_DATA_OBJ, &IID_IStorage, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, (LPUNKNOWN *)lpFileStr);
    else
        hrErr = lpAttach->OpenProperty(PR_ATTACH_DATA_BIN, &IID_IStream,  STGM_READ | STGM_SHARE_EXCLUSIVE, 0, (LPUNKNOWN *)lpFileStr);
    //    hrErr = lpAttach->OpenProperty(PR_ATTACH_DATA_OBJ, &IID_IStream,  STGM_READ | STGM_SHARE_EXCLUSIVE, 0, (LPUNKNOWN *)lpFileStr);
    
error:

    if (lpAttach)
        lpAttach->Release();
    if (hrErr)
    {
        if (hrErr == MAPI_E_NOT_FOUND)
            return MAIL_FILE_DELETED;
        else
            return hrErrToszErr(hrErr);
    }
    return NOERROR;
}
//
// Saving attachment into file
//
#define CREATE_MODE STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE

DWORD CAttStreamMAPIfi::CopyFrom(LPVOID SrcFile, int Flag)
{
    HRESULT hrErr;
    if (Flag == ATTACH_OLE)
    {
        LPSTORAGE lpSrcStg = (LPSTORAGE)SrcFile;
        LPSTORAGE lpDestStg;
        WCHAR wPath[_MAX_PATH];
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_DstPath, -1, wPath, MAX_PATH);
        hrErr = StgCreateDocfile(wPath, CREATE_MODE , 0, (LPSTORAGE *)&lpDestStg);
        if (hrErr)
            return hrErr;
        hrErr = lpSrcStg->CopyTo(0, NULL, NULL, lpDestStg);
        if (!hrErr)
            hrErr = lpDestStg->Commit(STGC_DEFAULT);
        lpDestStg->Release();
    }
    else
    {
        LPSTREAM lpSrcStm = (LPSTREAM)SrcFile;
        LPSTREAM lpDestStm;
        hrErr = lpOpenStreamOnFile(lpMAPIAllocateBuffer, lpMAPIFreeBuffer, CREATE_MODE, (char *)m_DstPath, NULL, (LPSTREAM *)&lpDestStm);
        if (hrErr)
            return hrErr;
        ULARGE_INTEGER uli;
        uli.LowPart  = 0xFFFFFFFF;
        uli.HighPart = 0xFFFFFFFF;
        hrErr = lpSrcStm->CopyTo((LPSTREAM)lpDestStm, uli, NULL, NULL);
        if (!hrErr)
            hrErr = lpDestStm->Commit(STGC_DEFAULT);
        lpDestStm->Release();
    }
    return hrErr;
}
//
// Saving attachment into database
//
DWORD CAttStreamMAPIdi::CopyFrom(LPVOID SrcFile, int Flags)
{
    HRESULT hrErr;

    if (Flags == ATTACH_OLE)
    {
        LPSTORAGE lpSrcStg = (LPSTORAGE)SrcFile;
        LPSTORAGE lpDestStg; 
        hrErr = ::StgCreateDocfileOnILockBytes(this, CREATE_MODE, 0, &lpDestStg);
        if (hrErr)
            return hrErr;
        hrErr = lpSrcStg->CopyTo(0, NULL, NULL, lpDestStg);
        if (!hrErr)
            hrErr = lpDestStg->Commit(STGC_DEFAULT);
        lpDestStg->Release();
    }
    else
    {
        LPSTREAM lpSrcStm = (LPSTREAM)SrcFile;
        STATSTG ss;
        hrErr = lpSrcStm->Stat(&ss, STATFLAG_NONAME);
        if (hrErr)
            return hrErr;
        uns32 Size;
        char *Buf = BufAlloc(ss.cbSize.LowPart, &Size);
        if (!Buf)
            return E_OUTOFMEMORY;
        if (CAttStreamd::Open(TRUE))
            return E_FAIL;
        for (;;)
        { DWORD sz;
            hrErr = lpSrcStm->Read(Buf, Size, &sz);
            if (hrErr)
                break;
            if (!sz)
                break;
            t_varcol_size vsz;
            CAttStreamd::Write(m_Offset, Buf, sz, &vsz);
        }
        free(Buf);
    }
    return hrErr;
}
//
// Implementace ILockBytes
//
HRESULT CAttStreamMAPIdi::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if (riid == IID_ILockBytes || riid == IID_IUnknown)
    {
        m_cRef++;
		*ppvObj = this;
        return NOERROR;
	}
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

ULONG   CAttStreamMAPIdi::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG   CAttStreamMAPIdi::Release()
{
    m_cRef--;
    return m_cRef;
}

HRESULT CAttStreamMAPIdi::ReadAt(ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead)
{
    return NOERROR;
}

HRESULT CAttStreamMAPIdi::WriteAt(ULARGE_INTEGER ulOffset, void const *pv, ULONG cb,  ULONG *pcbWritten)
{ t_varcol_size sz;
    if (CAttStreamd::Write(ulOffset.LowPart, (LPCSTR)pv, cb, &sz))
        return E_FAIL;
    *pcbWritten=sz;
    return NOERROR;
}

HRESULT CAttStreamMAPIdi::Flush()
{
#ifdef SRVR
    if (commit(m_cdp))
#else
    if (cd_Commit(m_cdp))
#endif
        return E_FAIL;
    return NOERROR;
}

HRESULT CAttStreamMAPIdi::SetSize(ULARGE_INTEGER libNewSize)
{
    return NOERROR;
}

HRESULT CAttStreamMAPIdi::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return NOERROR;
}

HRESULT CAttStreamMAPIdi::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return NOERROR;
}

HRESULT CAttStreamMAPIdi::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    pstatstg->pwcsName             = NULL;
    pstatstg->type                 = STGTY_LOCKBYTES;
    pstatstg->cbSize.HighPart      = 0;
    pstatstg->cbSize.LowPart       = m_Offset;
    pstatstg->mtime.dwLowDateTime  = 0;
    pstatstg->mtime.dwHighDateTime = 0;
    pstatstg->ctime.dwLowDateTime  = 0;
    pstatstg->ctime.dwHighDateTime = 0;
    pstatstg->atime.dwLowDateTime  = 0;
    pstatstg->atime.dwHighDateTime = 0;
    pstatstg->grfMode              = 0;
    pstatstg->grfLocksSupported    = 0;
    pstatstg->clsid                = CLSID_NULL;
    pstatstg->grfStateBits         = 0;
    pstatstg->reserved             = 0;

    return NOERROR;
}

/*
LONGLONG WB2FDTM(DWORD WBDate, DWORD WBTime)
{
    DWORD    DDTM = WB2DDTM(WBDate, WBTime);
    FILETIME FDTM;
    if  (!CoDosDateTimeToFileTime(HIWORD(DDTM), LOWORD(DDTM), &FDTM))
        return 0;
    LocalFileTimeToFileTime((LPFILETIME)&FDTM, (LPFILETIME)&FDTM);
    return *(LONGLONG *)&FDTM;
}
*/
//
// Konverze FILETIME na DATE a TIME
//
void FDTM2WB(LONGLONG FDTM, LPDWORD WBDate, LPDWORD WBTime)
{
    DWORD DDTM;
    FileTimeToLocalFileTime((LPFILETIME)&FDTM, (LPFILETIME)&FDTM);
    if (!CoFileTimeToDosDateTime((LPFILETIME)&FDTM, ((LPWORD)&DDTM) + 1, (LPWORD)&DDTM))
    {
        *WBDate = 0;
        *WBTime = 0;
    }
    else
    {
        DDTM2WB(DDTM, WBDate, WBTime);
    }
}

#endif // WINS
//
// CWBMailBoxPOP3 - Postovni schranka POP3
//
//
// Konstruktor
//
CWBMailBoxPOP3::CWBMailBoxPOP3(CWBMailCtx *MailCtx) : CWBMailBox(MailCtx)
{
    m_hFile      = INVALID_FHANDLE_VALUE;
    m_cMsgs      = 0;
    m_IDTbl      = NULL;
    m_MsgBuf     = NULL;
}
//
// Nacteni seznamu zasilek
//
DWORD CWBMailBoxPOP3::Load(UINT Param)
{
    char  From[65];
    char  To[65];
    char  For[65];
    char  Subject[81];
    DWORD CreDate, CreTime;
    DWORD RcvDate, RcvTime;
    DWORD Flags;
    DWORD Err;
    bool  Files;
    bool  RetPath;
    bool  Trans = false;
    BYTE  Stat;
    char *Line;

    // m_cMsgs = pocet zasilek ve schrance
    Err = Init();
    DbgLogFile("Load Init = %d", Err);
    if (Err)
        return Err;

    for (m_Msg = 1; m_Msg <= m_cMsgs; m_Msg++)
    {
        DbgLogFile("");
#ifdef UNIX
#ifdef _DEBUG
//        printf("\nGetHdr(%d)", m_Msg);
#endif
#endif
        Err = GetHdr((Param & (MBL_BODY | MBL_HDRONLY | MBL_MSGONLY)) == MBL_HDRONLY);
#ifdef UNIX
#ifdef _DEBUG
//        printf("\nGetHdr = %d", Err);
#endif
#endif
        DbgLogFile("Load GetHdr(%d) = %d", m_Msg, Err);
        if (Err)
            return Err;

        *From     = 0;
        *To       = 0;
        *For      = 0;
        *Subject  = 0;
        *m_MsgID  = 0;
        Flags     = 0;
        CreDate   = NONEDATE;
        CreTime   = NONETIME;
        RcvDate   = NONEDATE;
        RcvTime   = NONETIME;
        Files     = false;
        RetPath   = false;

        *m_IDTbl[m_Msg - 1].m_Bound = 0;
        char *pSubj = NULL;
        char *pFrom = NULL;
        char *pTo   = NULL;
        bool  inRec = false;
        m_Addressp  = m_Addressees;

        //DbgLogFile("TOP");
        while ((Err = Readln(&Line)) == NOERROR)
        {
            if (*(WORD *)Line == '.')
                break;
            char *Val;

            if (RcvDate == NONEDATE)
            {
                //if (GetValC(buf, "from ", 5, &Val)) 
                //{
                //    P3DTM2WB(Val, &RcvDate, &RcvTime);
                //}
                if (GetValC(Line, "Return-Path:", 12, &Val) || GetValC(Line, "Received:", 9, &Val))
                {
                    char *p = strchr(Val, ';');
                    if (p)
                        P3DTM2WB(p + 1, &RcvDate, &RcvTime);
                    else
                        RetPath = true;
                }
                else if (RetPath)
                {
                    char *p = strchr(Val, ';');
                    if (p)
                        P3DTM2WB(p + 1, &RcvDate, &RcvTime);
                    else if (strchr(Line, ':'))
                        RetPath =  false;
                }
            }
            // IF Subject na nekolik radku, nacist dalsi radek
            if (pSubj)
            {
                if (*Line == ' ' || *Line == '\t')
                {
                    pSubj = ConvertSubj(Line, pSubj, (int)sizeof(Subject) - 1 - (int)(pSubj - Subject));
                    continue;
                }
                else
                    pSubj = NULL;
            }
            // IF From na nekolik radku, nacist dalsi radek
            else if (pFrom)
            {
                if (*Line == ' ' || *Line == '\t')
                {
                    pFrom = ConvertAddr(Line, pFrom, (int)sizeof(From) - 1 - (int)(pFrom - From));
                    continue;
                }
                else
                    pFrom = NULL;
            }
            // IF To na nekolik radku, nacist dalsi radek
            else if (pTo)
            {
                if (*Line == ' ' || *Line == '\t')
                {
                    pTo = ConvertAddr(Line, pTo, (int)sizeof(To) - 1 - (int)(pTo - To));
                    continue;
                }
                else
                    pTo = NULL;
            }
            // IF Received na nekolik radku
            else if (inRec)
            {
                if (*Line == ' ' || *Line == '\t')
                {
                    char *p = strstr(Line, "for ");
                    if (p && (p[-1] == ' ' || p[-1] == '\t'))
                    {
                        m_Addressp = m_Addressees;
                        m_Addresst = ADDRS_TO | ADDRS_SIGN | ADDRS_FIRST;
                        ConvertAddr(p + 4, For, sizeof(For));
                        inRec = false;
                    }
                }
                else
                    inRec = false;
            }

            // From
            if (GetValC(Line, "from:", 5, &Val)) 
            {
                m_Addresst = ADDRS_NAME;
                pFrom = ConvertAddr(Val, From, sizeof(From));
            }
            else if (GetValC(Line, "Return-Path:", 12, &Val))
            {
                ValCpy(Val, From, 64);
            }
            // To
            else if (GetValC(Line, "Received:", 9, &Val)) 
            {
                if (!*For)
                {
                    char *p = strstr(Val, " for ");
                    if (p)
                    {
                        m_Addressp = m_Addressees;
                        m_Addresst = ADDRS_TO | ADDRS_SIGN | ADDRS_FIRST;
                        ConvertAddr(p + 5, For, sizeof(For));
                    }
                    else
                        inRec = true;
                }
            }
            else if (GetValC(Line, "to:", 3, &Val)) 
            {
                m_Addressp = m_Addressees;
                m_Addresst = ADDRS_TO | ADDRS_SIGN | ADDRS_FIRST;
                pTo = ConvertAddr(Val, To, sizeof(To));
            }
            else if (GetValC(Line, "cc:", 3, &Val)) 
            {
                m_Addresst = ADDRS_CC | ADDRS_SIGN;
                pTo = ConvertAddr(Val, To, 0);
            }
            // Subject
            else if (GetValC(Line, "subject:", 8, &Val))
                pSubj = ConvertSubj(Val, Subject, 80);
            // Message-ID
            else if (GetValC(Line, "message-id:", 11, &Val))
                ValCpy(Val, (char *)m_MsgID, MSGID_SIZE - 1);
            // Date
            else if (GetValC(Line, "date:", 5, &Val)) 
                P3DTM2WB(Val, &CreDate, &CreTime);
            // Content-type
            else if (GetValC(Line, "content-type:", 13, &Val))
            {
                // IF multipart/mixed, zapsat boundary do tabulky
                if (strnicmp(Val, "multipart/", 10) == 0) 
                {                    
                    if (strnicmp(Val + 10, "mixed;", 6) == 0)
                        Val += 16;
                    else if (strnicmp(Val + 10, "alternative;", 12) == 0)
                        Val += 22;
                    else if (strnicmp(Val + 10, "report;", 7) == 0)
                        Val += 17;
                    if (GetValE(Val, "boundary", 8, &Val))
                    {
                        Files = true;
                    }
                    else
                    {
                        Err = Readln(&Line);
                        if (Err)
                            break;
                        if (GetValE(Line, "boundary", 8, &Val))
                            Files = true;
                    }
                    if (Files)
                    {
                        *(WORD *)m_IDTbl[m_Msg - 1].m_Bound = ('-' << 8) + '-';
                        ValCpy(Val, m_IDTbl[m_Msg - 1].m_Bound + 2, 64);
                    }
                }
            }
            // Doporucene
            else if (GetValC(Line, "Disposition-Notification-To:", 28, &Val))
            {
                Flags |= WBL_READRCPT;
            }
            // Priorita
            else if (GetValC(Line, "Priority:", 9, &Val))
            {
                if (strnicmp(Val, "non-urgent", 10) == 0) 
                    Flags |= WBL_PRILOW;
                else if (strnicmp(Val, "urgent", 6) == 0) 
                    Flags |= WBL_PRIHIGH;
            }
        }
#ifdef UNIX
#ifdef _DEBUG
        //printf("\nGetHdr Konec = %d    Subj = %s", Err, Subject);
        //if (strlen(From) > 64)
        //{
        //    printf("\nChyba strlen");
        //    return 1;
        //}
#endif
#endif
        if (Err)
            return Err;
        // Ridici zasilky muzeme zahodit
        //DbgLogFile("Msg: %s", Subject);
        if (*m_MsgID == 0)
        {
            BYTE *p = m_MsgID;
            *(DWORD *)p = RcvDate;
            p += 4;
            *(DWORD *)p = RcvTime;
            p += 4;
            int Size = lstrlen(Subject);
            if (Size > MSGID_SIZE - 8)
                Size = MSGID_SIZE - 8;
            memcpy(p, Subject, Size);
            p += Size;
            Size = lstrlen(From);
            if (Size > (int)(m_MsgID + MSGID_SIZE - p))
                Size = (int)(m_MsgID + MSGID_SIZE - p);
            if (Size)
            {
                memcpy(p, From, Size);
                p += Size;
            }
            Size = (int)(m_MsgID + MSGID_SIZE - p);
            if (Size)
                memset(p, 0, Size);
            //DbgLogFile("MsgID: %08X%08X%.42s", *(int *)m_MsgID, *(int *)(m_MsgID + 4), m_MsgID + 8);
        }
        else
        {
            DWORD Size = lstrlen((char *)m_MsgID);
            if (Size < MSGID_SIZE)
                memset(m_MsgID + Size, 0, MSGID_SIZE - Size);
            //DbgLogFile("MsgID: %.32s", m_MsgID);
        }
        memcpy(m_IDTbl[m_Msg - 1].m_MsgID, m_MsgID, MSGID_SIZE);

#ifdef _DEBUG
        if (strlen(Subject) > 80 || strlen(From) > 64)
        {
            DbgLogWrite("Chyba(%d) strlen", m_Msg);
            DbgLogWrite("Subject: %s", Subject);
            DbgLogWrite("From:    %s", From);
            return 1;
        }
#endif
        
        if (StartTransaction())
            goto error;

        Trans = true;
        
        Err = MsgID2Pos();
        if (Err)
        {
            if (Err != MAIL_MSG_NOT_FOUND)
                goto exit;
            m_CurPos = InsertL();
            DbgLogWrite("Inserted record %d", m_CurPos);
            if (m_CurPos == NORECNUM)
                goto error;
            if (RcvDate == NONEDATE)
            {
                RcvDate = Today();
                RcvTime = Now();
            }
            pTo = For;
            if (!*pTo)
                pTo = To;
            if (WriteIndL(ATML_SUBJECT,   Subject, 80))
                goto error;
            if (WriteIndL(ATML_SENDER,    From,    64))
                goto error;
            if (WriteIndL(ATML_RECIPIENT, pTo,     64))
                goto error;
            if (WriteIndL(ATML_CREDATE,   &CreDate, 4))
                goto error;
            if (WriteIndL(ATML_CRETIME,   &CreTime, 4))
                goto error;
            if (WriteIndL(ATML_RCVDATE,   &RcvDate, 4))
                goto error;
            if (WriteIndL(ATML_RCVTIME,   &RcvTime, 4))
                goto error;
            if (WriteIndL(ATML_MSGID,     m_MsgID,  MSGID_SIZE))
                goto error;
            if (WriteIndL(ATML_FLAGS,     &Flags,   4))
                goto error;
            if (m_Addressp > m_Addressees)
            {
                int sz = lstrlen(m_Addressees);
                if (WriteVarL(ATML_ADDRESSEES, m_Addressees, sz))
                    goto error;
                if (WriteLenL(ATML_ADDRESSEES, sz))
                    goto error;
            }
            if ((Param & (MBL_BODY | MBL_HDRONLY | MBL_MSGONLY)) == MBL_HDRONLY)
            {
                int sz = (int)(m_HdrPtr - m_HdrBuf);
                if (WriteVarL(ATML_HEADER, m_HdrBuf, sz))
                    goto error;
                if (WriteLenL(ATML_HEADER, sz))
                    goto error;
            }
            Stat = MBS_NEW;
        }
        else
        {
            Stat = MBS_OLD;
        }
        if (WriteIndL(ATML_STAT, &Stat, 1))
            goto error;

        // IF take body a jeste jsme ho nenacetli, nacist
        if (Param & (MBL_BODY | MBL_MSGONLY))
        {
            //DbgLogFile("Load GetMsg");
            Err = GetMsg(Param & (MBL_BODY | MBL_HDRONLY | MBL_MSGONLY));
            //DbgLogFile("Load GetMsg = %d", Err);
            if (Err)
                goto exit;
        }

        // IF jeste jsme nenacetli FilesInfo
        //DbgLogFile("    GetFilesInfo(%02X) %d %08X", Param, Files, m_FileCnt);
        if (m_FileCnt == (WORD)NONESHORT)
        {
            if (Files)
            {
                if (Param & MBL_FILES)
                {
#ifdef UNIX
#ifdef _DEBUG
//        printf("\nGetFilesInfo", Err);
#endif
#endif
                    Err = GetFilesInfo();
#ifdef UNIX
#ifdef _DEBUG
//        printf("\nGetFilesInfo = %d", Err);
#endif
#endif
                    //DbgLogFile("Load GetFilesInfo = %d", Err);
                    if (Err)
                        goto exit;
                }
            }
            else
            {
                Flags = 0;
                if (WriteIndL(ATML_FILECNT,  &Flags, 2))
                    goto error;
            }
        }
        if (WriteIndL(ATML_PROFILE,   m_MailCtx->m_Profile, 64))
            goto error;
        Trans = false;
        if (Commit())
            goto error;
        ReleaseLastMsg();
#ifdef UNIX
        //printf("\nZasilka %d Ok", m_Msg);
#endif
    }
    return NOERROR;

error:

    Err = WBError();

exit:

    if (Trans)
        RollBack();
    return Err;
}

//
// ULozeni tela dopisu do databaze
// 
DWORD CWBMailBoxPOP3::GetMsg(DWORD Flags)
{
    // Nacist zasilku do docasneho souboru
    DWORD Err = LoadMsg();
    if (Err)
        return Err;

    DWORD Size;
    DWORD Offset  = 0;
    UINT  ConvF   = 0;
    int   SrcLang = 0;
    bool  Header  = true;
    char *Val;
    char *Ptr;
    char *pl;
    char *pRec    = NULL;

    if (Flags & MBL_BODY)
        Flags = 0;

    // Nacist prvni cast zasilky (az do boundery nebo dokonce)
    while ((Err = GetNextPart(0, &Ptr, &Size)) == NOERROR && Size > 0)
    { 
        if (Header)
        {
            for (pl = Ptr; *pl != '\r' && *pl != '\n' && pl - Ptr < Size;)
            {
                // Nektery postovni server nedava pred Received: CRLF a predchozi radek je pak moc dlouhy
                if (GetValC(pl, "Return-Path:", 12, &Val))
                {
                    pRec = strstr(pl + 12, "Received:");
                    if (pRec)
                        pRec = strstr(pl + 12, "received:");
                }
                // IF Content-type, najit charset
                else 
                {
                    ConvF |= GetConvertFlag(pl, &SrcLang);
                }
                char *pl2 = strchr(pl, '\r');
                if (pl2)
                    pl = pl2 + 2;
                else
                    pl = strchr(pl, '\n') + 1;
            }
            // IF dopis kodovan v base64 vynechat radek za hlavickou
            if (Flags == 0 && (ConvF & P3CF_BASE64) && !m_IDTbl[m_Msg - 1].m_Bound[0])
            {
                if (*pl == '\r')
                    pl++;
                if (*pl == '\n')
                    pl++;
            }
            if (Flags != MBL_MSGONLY)
            {
                Size = (int)(pl - Ptr);
                tattrib attr = (Flags & MBL_HDRONLY) ? ATML_HEADER : ATML_BODY;
                if (pRec)
                {
                    DWORD Sz = (DWORD)(pRec - Ptr);
                    if (WriteVarL(attr, Ptr, Offset, Sz))
                        goto error;
                    Offset += Sz;
                    if (WriteVarL(attr, "\r\n", Offset, 2))
                        goto error;
                    Offset += 2;
                    Ptr    += Sz;
                    Size   -= Sz;
                }
                if (WriteVarL(attr, Ptr, Offset, Size))
                    goto error;
                Offset += Size;
                if (Flags & MBL_HDRONLY)
                {
                    if (WriteLenL(attr, Offset))
                        goto error;
                    Offset = 0;
                    if (Flags == MBL_HDRONLY)
                        break;
                }
            }
            if (Flags & MBL_MSGONLY)
            {
                while (*pl == '\r' || *pl == '\n')
                    pl++;
            }
            Size = (int)(m_EndOfPart - pl);
            if ((int)Size < 0)
                Size = 0;
            if (m_IDTbl[m_Msg - 1].m_Bound[0])  // telo je az za boundery
                Size = 0;
#ifdef UNIX
            if (m_MailCtx->m_Type & WBMT_DIRPOP3)
            {
                ConvF |= P3CF_DIRECT;
            }
#endif

            Ptr    = pl;
            Header = false;
        }
        if (Size)
        {
            // IF neni multipart (telo je hned za hlavickou), zkonvertovat
            // Zkonvertovat z Quoted-printable, BASE64 apod
            Size = Convert(Ptr, Size, ConvF);
            // Zkonvertovat do jazyka serveru
            //printf("\n    Body Singlepart %d %02X", m_Msg, SrcLang);
            //printf("\n    %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", Ptr[0], Ptr[1], Ptr[2], Ptr[3], Ptr[4], Ptr[5], Ptr[6], Ptr[7], Ptr[8], Ptr[9], Ptr[10], Ptr[11]);
            Size = CWBMailBox::Convert(Ptr, Ptr, Size, SrcLang);
            //printf("\n    %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", Ptr[0], Ptr[1], Ptr[2], Ptr[3], Ptr[4], Ptr[5], Ptr[6], Ptr[7], Ptr[8], Ptr[9], Ptr[10], Ptr[11]);
            if (WriteVarL(ATML_BODY, Ptr, Offset, Size))
                goto error;
            Offset += Size;
        }
    } 
    BOOL SubBreak;
    SubBreak = FALSE;
    char SubBound[68];
    *SubBound  = 0;
    m_PartOpen = FALSE;
    ConvF      = P3CF_SKIPHDR;
    // IF multipart, telo je ze boundery
    if (m_IDTbl[m_Msg - 1].m_Bound[0] && (Flags == 0 || (Flags & MBL_MSGONLY)))
    {
        while ((Err = GetNextPart(1, &Ptr, &Size)) == NOERROR && Size > 0)
        {
            // IF poprve, zjistit konverzi
            if (ConvF & P3CF_SKIPHDR)
            {
                for (pl = Ptr; *pl != '\r' && *pl != '\n' && pl - Ptr < Size;)
                {
                    ConvF |= GetConvertFlag(pl, &SrcLang);
                    char *pb;
                    if (GetValE(pl, "boundary", 8, &pb))
                    {
                        *(WORD *)SubBound = ('-' << 8) + '-';
                        ValCpy(pb, SubBound + 2, sizeof(SubBound) - 4);
                    }
                    char *pl2 = strchr(pl, '\r');
                    if (pl2)
                        pl = pl2 + 2;
                    else
                        pl = strchr(pl, '\n') + 1;
                }
                // IF pouze dopis, preskocit prazdne radky
                if (Flags & MBL_MSGONLY)
                {
                    if (*SubBound)
                    {
                        char *pl2 = strstr(pl, SubBound);
                        if (pl2)
                            pl = pl2 + lstrlen(SubBound);
                        while (pl - Ptr < Size)
                        {
                            pl2 = strchr(pl, '\r');
                            if (pl2)
                                pl = pl2 + 2;
                            else
                                pl = strchr(pl, '\n') + 1;
                            if (*pl == '\r' || *pl == '\n')
                                break;
                            ConvF |= GetConvertFlag(pl, &SrcLang);
                        }
                    }
                    while (*pl == '\r' || *pl == '\n')
                        pl++;
                    Ptr    = pl;
                    Size   = (int)(m_EndOfPart - Ptr);
                    ConvF &= ~P3CF_SKIPHDR;
                }
            }
            if ((Flags & MBL_MSGONLY) && (*SubBound))
            {
                char *p = strstr(Ptr, SubBound);
                if (p)
                {
                    Size = (int)(p - Ptr);
                    SubBreak = TRUE;
                }
            }
            // Zkonvertovat z Quoted-printable, BASE64 apod
            Size = Convert(Ptr, Size, ConvF);
            // Zkonvertovat do jazyka serveru
            //printf("\nBody Multipart %08X", SrcLang);
            Size = CWBMailBox::Convert(Ptr, Ptr, Size, SrcLang);
            if (WriteVarL(ATML_BODY, Ptr, Offset, Size))
                goto error;
            Offset += Size;
            ConvF &= ~P3CF_SKIPHDR;
            if (SubBreak)
                break;
        } 
        m_PartOpen = FALSE;
    }
    if (Err == MAIL_NO_MORE_FILES)
        Err = NOERROR;
    if (WriteLenL(ATML_BODY, Offset))
        goto error;
    if (WriteIndL(ATML_SIZE, &Offset, 4))
        goto error;
    return Err;

error:

    m_PartOpen = FALSE;
    return WBError();
};
//
// Converts charset indicator from message header into 602SQL charset flag
//
int CWBMailBox::GetSrcLang(char **Val)
{
    if (strnicmp(*Val, "ISO-8859-2", 10) == 0)
    {
        *Val += 10;
        return 0x81;
    }
    if (strnicmp(*Val, "windows-1250", 12) == 0)
    {
        *Val += 12;
        return 0x01;
    }
    if (strnicmp(*Val, "windows-1252", 12) == 0)
    {
        *Val += 12;
        return 0x03;
    }
    if (strnicmp(*Val, "ISO-8859-1", 10) == 0)
    {
        *Val += 10;
        return 0x83;
    }
    if (strnicmp(*Val, "UTF-8", 5) == 0)
    {
        *Val += 5;
        return 0x07;
    }
    if (strnicmp(*Val, "US-ASCII", 8) == 0)
    {
        *Val += 8;
        return 0x00;
    }
    return 0;
}
//
// Checks message encoding type
// If the line starts with "content-type:" and "charset" keyword follows, writes used charset into SrcLang
// If the line starts with "Content-transfer-encoding:", returns encoding type
//
UINT CWBMailBoxPOP3::GetConvertFlag(char *Line, int *SrcLang)
{
    char *Val;

    if (GetValC(Line, "content-type:", 13, &Val))
    {
        do
        {
            if (GetValE(Val, "charset", 7, &Val))
            {
                if (*Val == '"')
                    Val++;
                *SrcLang = GetSrcLang(&Val);
                return 0;
            }
            if (Val[0] == '\r' && Val[1] == '\n' && (Val[2] == ' ' || Val[2] == '\t'))
                Val += 3;
            else if (Val[0] == '\n' && (Val[1] == ' ' || Val[1] == '\t'))
                Val += 2;
        }
        while (*Val != '\r' && *Val != '\n');
    }
    // IF Content-transfer-encoding == quoted-printable, bude se konvertovat
    else if (GetValC(Line, "Content-transfer-encoding:", 26, &Val))
    {
        if (strnicmp(Val, "quoted-printable", 16) == 0)
            return P3CF_QUOTED;
        else if (strnicmp(Val, "base64", 6) == 0)
            return P3CF_BASE64;
    }
    return 0;
}
//
// Ulozeni seznamu pripojenych souboru do databaze
//
DWORD CWBMailBoxPOP3::GetFilesInfo()
{
    // Nacist zasilku do docasneho souboru
    DWORD Err = LoadMsg();
    //DbgLogFile("GetFilesInfo LoadMsg = %d", Err);
    if (Err)
        return Err;

    DWORD Size;
    DWORD FileLen;
    DWORD ID;
    DWORD CreDate;
    DWORD CreTime;
    WORD  FileNO;
    UINT  Flags;
    char  FileName[MAX_PATH];

    if (StartTransaction())
        goto error;
    if (ReadIndL(ATML_ID, &ID))
        goto error;
    if (ReadIndL(ATML_CREDATE, &CreDate))
        goto error;
    if (ReadIndL(ATML_CRETIME, &CreTime))
        goto error;
    sprintf(FileName, "DELETE FROM %s WHERE ID=%d", m_InBoxTbf, ID);
    if (SQLExecute(FileName))
        goto error;
    //
    // Pres vsechny attachmenty
    //
    for (FileNO = 2; ; FileNO++)
    {
        char *Line, *Val;
        *FileName = 0;

        // Nacist hlavicku
        //DbgLogFile("        GetFileInfo(%d)", FileNO);
        Err = GetFileInfo(FileNO, &Flags, FileName, &Line);
        //DbgLogFile("    GetFilesInfo GetFileInfo = %d  %.220s", Err, FileName);
        if (Err)
        {
            if (Err == MAIL_NO_MORE_FILES)
                break;
            goto exit;
        }
        // IF Base64, spocitat delku souboru z poctu radku
        if (Flags & P3CF_BASE64)
        {
            Val = Line;
            // FileLen = Delka radku
            while (*Val != '\r' && *Val != '\n' && *Val)
                Val++;
            FileLen = (int)(Val - Line);
            // lc = pocet radku
            DWORD lc;
            for (lc = 0; ; lc++)
            {
                Err = NextLine(Val, &Line);
                if (Err)
                {
                    if (Err != MAIL_UNK_MSG_FMT)
                        goto exit;
                    break;
                }
                if (*Line == '\r' || *Line == '\n')
                {
                    Line = Val;
                    break;
                }
                Val = Line;
            }
            // Posledni nebyva plny, nebo neni ukoncen <CR><LF> ale ==--boundery
            FileLen *= lc;
            Val = Line;
            while (*Val != '\r' && *Val != '\n' && *Val != '-' && *Val)
                Val++;
            Val = Line + ((Val - Line) & 0xFFFFFFFC);
            FileLen += (int)(Val - Line);
            FileLen = FileLen / 4 * 3;
            Val--;
            if (*Val == '=')
            {
                FileLen--;
                Val--;
                if (*Val == '=')
                    FileLen--;
            }
        }
        // ELSE quoted-printable, prepocitat delku souboru
        else if (Flags & P3CF_QUOTED) // && (Flags & (P3CF_ISO8859 | P3CF_WIN1250)))
        {
            for (FileLen = (int)(m_EndOfPart - Line); ;FileLen += Size) 
            {
                Err = GetNextPart(FileNO, &Line, &Size);
                if (Err)
                    goto exit;
                if (!Size)
                    break;
                while ((Line = strchr(Line, '=')))
                {
                    Line++;
                    if (*Line == '\r')
                        Size -= 1;
                    else
                        Size -= 2;
                }
            }
        }
        // ELSE Plain-text, delka souboru je delka casti
        else
        {
            for (FileLen = (int)(m_EndOfPart - Line); ;FileLen += Size) 
            {
                Err = GetNextPart(FileNO, &Line, &Size);
                if (Err)
                    goto exit;
                if (!Size)
                    break;
            }
        }

        m_PartOpen = FALSE;
        //DbgLogFile("    Attachment %d: %s", FileNO - 1, FileName);

        trecnum PosF = InsertF();
        if (PosF == NORECNUM)
            goto error;
        if (WriteIndF(PosF, ATMF_ID, &ID, 4))
            goto error;
        //encode(Buf, *File->LongName, TRUE, 3);
        if (WriteIndF(PosF, ATMF_NAME, FileName, 254))
            goto error;
        if (WriteIndF(PosF, ATMF_DATE, &CreDate, 4))
            goto error;
        if (WriteIndF(PosF, ATMF_TIME, &CreTime, 4))
            goto error;
        if (WriteIndF(PosF, ATMF_SIZE, &FileLen, 4))
            goto error;
    }
    FileNO -= 2;
    if (WriteIndL(ATML_FILECNT, &FileNO, 2))
        goto error;

    Err = NOERROR;
    goto exit;

error:

    Err = WBError();

exit:

    m_PartOpen = FALSE;
    if (Err)
        RollBack();
    else
        Commit();

    return Err;
}
//
// Ulozeni pripojeneho souboru
//
DWORD CWBMailBoxPOP3::SaveFile(int FilIdx, LPCSTR FilName)
{
    //DbgLogFile("SaveFilePOP3 %d", FilIdx);
    char  Name[MAX_PATH];
    char *Line;
    UINT  Flags;
    DWORD Size;
    
    // Nacist zasilku do docasneho souboru
    DWORD Err = LoadMsg();
    //DbgLogFile("    LoadMsg %d", Err);
    if (Err)
        return Err;

    // IF zadano jmeno souboru, najit soubor podle jmena
    if (FilName && *FilName)
    {
        for (int i = 2; ; i++)
        {
            Err = GetFileInfo(i, &Flags, Name, &Line);
            //DbgLogFile("    GetFileInfo %d  %08X  %s", Err, Flags, Name);
            if (Err)
                return Err;
            if (stricmp(Name, FilName) == 0)
                break;
            m_PartOpen = FALSE;
        }
    }
    // ELSE najit soubor podle indexu
    else
    {
        Err = GetFileInfo(FilIdx + 2, &Flags, Name, &Line);
        //DbgLogFile("    GetFileInfo %d  %08X  %s", Err, Flags, Name);
        if (Err)
            return Err;
    }

    Err = m_AttStream->Open();
    //DbgLogFile("    AttStream->Open %d", Err);
    if (Err)
        return Err;
    // IF Base64, dekodovat
    if (Flags & P3CF_BASE64)
    {
        char Buf[300];
        while (*Line != '\r' && *Line != '\n')
        {
            Size = Base64Line(Line, Buf, &Line);
            if (Size == (DWORD)-1)
            {
                Err = MAIL_UNK_MSG_FMT;
                goto exit;
            }
            if (Line[-1] == '=')
            {
                Size--;
                if (Line[-2] == '=')
                    Size--;
                while (*Line == '=')
                    Line++;
            }
            Err = m_AttStream->Write(Buf, Size);
            if (Err)
                goto exit;
            if (*Line == '\r')
                Line++;
            if (*Line == '\n')
                Line++;
            // IF zbyva mene nez 4 byty do konce part, nacist dalsi
            if (Line >= m_EndOfPart - 4)
            {
                Err = GetNextPart(m_FileNO, &Line, &Size);
                if (Err)
                    goto exit;
                if (Size == 0)
                    break;
            }
        }
        //DbgLogFile("    Base64 OK");
    }
    // ELSE IF quoted-printable, dekodovat
    else if (Flags & P3CF_QUOTED) // && (Flags & (P3CF_ISO8859 | P3CF_WIN1250)))
    {
        Size = (int)(m_EndOfPart - Line);
        do
        {
            Size = Convert(Line, Size, Flags);
            Err = m_AttStream->Write(Line, Size);
            if (Err)
                goto exit;
            Err = GetNextPart(m_FileNO, &Line, &Size);
            if (Err)
                goto exit;
        }
        while (Size);
        //DbgLogFile("    quoted-printable OK");
    }
    // ELSE plain-text
    else
    {
        Size = (int)(m_EndOfPart - Line);

        do
        {

#ifdef UNIX

            if (m_MailCtx->m_Type & WBMT_DIRPOP3)
                Size = Convert(Line, Size, P3CF_DIRECT);
            
#endif

            Err = m_AttStream->Write(Line, Size);
            if (Err)
                goto exit;
            Err = GetNextPart(m_FileNO, &Line, &Size);
            //DbgLogFile("    GetNextPart %d", Err);
            if (Err)
                goto exit;
        }
        //while (m_EndOfPart - Line);
        while (Size);
        //DbgLogFile("    plain-text OK");
    }

exit:

    m_PartOpen = FALSE;
    //DbgLogFile("SaveFilePOP3 = %d", Err);
    return Err;
}
//
// Vytvoreni streamu pro zapis pripojeneho souboru do databaze
//
DWORD CWBMailBoxPOP3::CreateDBStream(tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index)
{
    m_AttStream = (CAttStream *) new CAttStreamSMTPdi(m_cdp, Curs, Pos, Attr, Index);
    if (!m_AttStream)
        return OUT_OF_APPL_MEMORY;
    return NOERROR;
}
//
// Vytvoreni streamu pro zapis pripojeneho souboru na disk
//
DWORD CWBMailBoxPOP3::CreateFileStream(LPCSTR DstPath)
{
    m_AttStream = new CAttStreamSMTPfi(DstPath);
    if (!m_AttStream)
        return OUT_OF_APPL_MEMORY;
    return NOERROR;
}
//
// Konverze z ID zaznamu na ID zasilky
//
// Meni kontext aktualni zasilky
//
DWORD CWBMailBoxPOP3::GetMsgID(uns32 ID)
{
    DWORD Err;

    if (ID == m_ID)
        return NOERROR;

    ReleaseLastMsg();
    if (!m_IDTbl)
    {
        Err = Load(0);
        if (Err)
            return Err;
    }
    
    Err = CWBMailBox::GetMsgID(ID);
    if (Err)
        return Err;

    for (m_Msg = 1; m_Msg <= m_cMsgs; m_Msg++)
        if (memcmp((char *)m_MsgID, m_IDTbl[m_Msg - 1].m_MsgID, MSGID_SIZE) == 0)
            return NOERROR;
    return MAIL_MSG_NOT_FOUND;
}
//
// Konverze subjektu do jazyka serveru
//
// Size = misto v cilovem bufferu
//
// Vraci ukazatel ve vystupnim bufferu, kam se bude ukladat zkonverovany text z pripadneho dalsiho radku
//
char *CWBMailBoxPOP3::ConvertSubj(char *Src, char *Dst, DWORD Size)
{
    char *pt = Dst;
    while (*Src == ' ' || *Src == '\t')
        Src++;

    while (*Src && Size)
    {
        DWORD sz = Size;
        if (*Src != '"' && *Src != '=')
        {
            char *pq = strchr(Src, '"');
            char *ps = strstr(Src, "=?");
            if (!pq || (ps && ps < pq))
                pq = ps;
            if (pq)
            {
                sz = (int)(pq - Src);
                if (sz > Size)
                    sz = Size;
            }
        }
        char *pd = Convert(&Src, Dst, sz);
        Size -= (int)(pd - Dst);
        Dst   = pd;
    }
    if (Dst > pt)
    {
        do
            Dst--;
        while (Dst >= pt && (*Dst == ' ' || *Dst == '\t'));
        Dst++;
        *Dst = 0;
    }
    return Dst;
}
//
// Konverze adresy
//
// Adresu ve formatu "jmeno" <xyz@email.cz> resp. =?ISO-8859-2?Q?jmeno?= <xyz@email.cz> parsuje na 
// jmeno a vlastni adresu. Prvni adresu ze seznamu v zahlavi zasilky ulozi do Dst, vsechny adresy 
// prijemcu uklada do seznamu adresatu ve formatu: TO: jmeno <xyz@email.cz>\r\n
//
// Zdrojovy seznam ::= Addr {, Addr} | Adresa;
// Addr            ::= Adresa | Jmeno <Adresa>
// Jmeno           ::= QTopic{QTopic}
// QTopic          ::= QText | =?ISO-xxxx?x?QText?=
// QText           ::= "Text" | Text
// Adresa          ::= Topic{.Topic}@Topic{.Topic}
// Topic           ::= Text | =?ISO-xxxx?x?Text?=
//
// Prechod na novy radek byva mezi Addr, Adresa, Jmeno a QTopicy
// dalsi radek zacina mezerou nebo tabulatorem
// 
// Vraci TRUE pokud adresa pokracuje na dalsim radku
//
char *CWBMailBoxPOP3::ConvertAddr(char *Src, char *Dst, DWORD Size)
{
    char *Next;
    char  Sep;

    do
    {
        while (*Src == ' ' || *Src == '\t')
            Src++;
        if (!*Src)
            break;
        // IF zacatek
        if (m_Addresst & ADDRS_SIGN)
        {
            // Realokovat seznam adresatu
            int sz = lstrlen(Src);
            if (!ReallocAddrs(sz + 7))           // 7 = "TO: " + '<' + '>' + 0
                return NULL;
            // Zapsat TO: / CC:
            *(DWORD *)m_Addressp = m_Addresst & ADDRS_CC ? ADDR_CC : ADDR_TO;
            m_Addressp += 4;
            m_Addresst ^= ADDRS_SIGN | ADDRS_NAME;
        }
        // IF Jmeno
        if (m_Addresst & ADDRS_NAME)
        {
            Sep = GetNameTopic(Src, &Next);
            // Jmeno = neco mezi Src a Next AND neobsahuje @
            if (Next > Src && Sep != '@')
            {
                if (m_Addresst & (ADDRS_TO | ADDRS_CC))
                {
                    char *pe;
                    for (pe = Next; pe[-1] == ' ' || pe[-1] == '\t'; pe--);
                    //if (pe[-1] == '"')
                    //    pe--;
                    char c = *pe;
                    *pe = 0;
                    //if (*Src == '"')
                    //    Src++;
                    char *pq = ConvertSubj(Src, m_Addressp, (int)(pe - Src));
                    *pe = c;
                    // IF " zahodit "
                    if ((*m_Addressp == '"' && pq[-1] == '"') || (*m_Addressp == '\'' && pq[-1] == '\''))
                    {
                        lstrcpy(m_Addressp, m_Addressp + 1);
                        pq -= 2;
                    }
                    m_Addressp = pq;
                }
            }
            if (Sep)
                m_Addresst ^= ADDRS_NAME | ADDRS_ADDR;
            if (Sep != '@')
                Src = Next;
        }
        // IF Adresa
        if (m_Addresst & ADDRS_ADDR)
        {
            Sep = GetAddrTopic(Src, &Next);
            if (*Src == '<')
            {
                Src++;
                if (m_Addresst & (ADDRS_TO | ADDRS_CC))
                    *m_Addressp++ = ' ';
            }
            char *pe;
            for (pe = Next; pe[-1] == ' ' || pe[-1] == '\t'; pe--);
            char c = *pe;
            *pe = 0;
            if (m_Addresst & (ADDRS_TO | ADDRS_CC))
            {
               *m_Addressp++ = '<';
                char *pd   = m_Addressp;
                m_Addressp = ConvertSubj(Src, m_Addressp, (int)(pe - Src));
                if (m_Addresst & ADDRS_FIRST)
                {
                    lstrcpy(Dst, pd);
                    Dst += lstrlen(Dst);
                }
                *(DWORD *)m_Addressp = '>' + ('\r' << 8) + ('\n' << 16);
                m_Addressp += 3;
                m_Addresst ^=  ADDRS_SIGN | ADDRS_ADDR;
                m_Addresst &= ~ADDRS_FIRST;
            }
            else
            {
                DWORD sz = (DWORD)(pe - Src);
                if (sz > Size)
                    sz = Size;
                Dst = ConvertSubj(Src, Dst, sz);
            }
            *pe = c;
            Src = Next;
            if (*Src == '>')
                Src++;
            if (*Src == ',')
                Src++;
            if (*Src == ';')
                break;
        }
    }
    while (*Src);
    return Dst;
}
//
// Hleda oddelovac v seznamu adres
//
// Vraci oddelovac a v Next ukazatel na oddelovac
//
char CWBMailBoxPOP3::GetNameTopic(char *Src, char **Next)
{
    while (*Src && *Src != '@' && *Src != '<')
    {
        if (*Src == '"')
            do Src++; while (*Src != '"');
        else if (*(WORD *)Src == ('=' + ('?' << 8)))
        {
            do Src++; while (*(WORD *)Src != ('?' + ('=' << 8)));
            Src++;
        }
        Src++;
    }
    *Next = Src;
    return *Src;
}

char CWBMailBoxPOP3::GetAddrTopic(char *Src, char **Next)
{
    if (*Src == '<')
    {
        while (*Src && *Src != '>')
            Src++;
    }
    else
    {
        while (*Src && *Src != ',' && *Src != ';' && *Src != ' ' && *Src != '\t')
        {
            if (*Src == '"')
            {
                do
                    Src++;
                while (*Src && *Src != '"');
                if (!*Src)
                    break;
            }
            Src++;
        }
    }
    *Next = Src;
    return *Src;
}
//
// Vraci parametry pripojeneho souboru
//
// Flags    - Content-Type, Content-Transfer-Encoding
// FileName - Jmeno souboru
// Next     - Ukazatel na zacatek vlastniho souboru
//
DWORD CWBMailBoxPOP3::GetFileInfo(int FileNO, UINT *Flags, char *FileName, char **Next)
{
    DWORD Size;
    char *Val, *Line;
    UINT  Cont = 3;

    DWORD Err = GetNextPart(FileNO, &Line, &Size);
    if (Err)
        goto exit;
    if (Size == 0)
        return MAIL_NO_MORE_FILES;

    *Flags = 0;
    // Najit Content-Type, jmeno a Content-Transfer-Encoding
    do 
    {   
        if (GetValC(Line, "Content-Type:", 13, &Val))
        {
            do 
            {
                char *Var;
                if (GetValE(Val, "name", 4, &Var))
                {
                    ValCpy(Var, FileName, MAX_PATH - 1);
                    //DbgLogFile("            name = %s", FileName);
                    Cont &= ~1;
                }
                //else if (GetValE(Val, "charset", 7, &Var))
                //{
                //    if (strnicmp(Var, "ISO-8859-2", 10) == 0)
                //        *Flags |= P3CF_ISO8859;
                //    else if (strnicmp(Val, "windows-1250", 12) == 0)
                //        *Flags |= P3CF_WIN1250;
                //}
                if (Var[-1] == ';')
                {
                    while (*Var == '\r' || *Var == '\n')
                        Var++;
                }
                Val = Var;
            }
            while (*Val != '\r' && *Val != '\n');
        }
        else if (GetValC(Line, "Content-Transfer-Encoding:", 26, &Val))
        {
            if (strnicmp(Val, "quoted-printable", 16) == 0)
                *Flags |= P3CF_QUOTED;
            else if (strnicmp(Val, "base64", 6) == 0)
                *Flags |= P3CF_BASE64;
            Cont &= ~2;
        }
        else if (GetValC(Line, "Content-Disposition:", 20, &Val))
        {
            do 
            {
                char *Var;
                if (GetValE(Val, "filename", 8, &Var))
                {
                    ValCpy(Var, FileName, MAX_PATH - 1);
                    //DbgLogFile("            filename = %s", FileName);
                    Cont &= ~1;
                    break;
                }
                if (Var[-1] == ';')
                {
                    while (*Var == '\r' || *Var == '\n')
                        Var++;
                }
                Val = Var;
            }
            while (*Val != '\r' && *Val != '\n');
        }
        Err = NextLine(Val, &Line);
        if (Err)
            goto exit;
    }
    while (Cont && (*Line != '\r' && *Line != '\n'));

    ConvertSubj(FileName, FileName, MAX_PATH);
    //DbgLogFile("            Convert = %s", FileName);
    // vyhazet CR LF
    char *s, *d;
    for (s = FileName, d = FileName; *s; s++, d++)
    {
        if (*s == '\r' || *s == '\n')
        {
            while (*s == '\r' || *s == '\n') 
                s++;
            while (*s == ' ') 
                s++;
        }
        *d = *s;
    }
    *d = 0;
    // Preskocit ostatni property
    while (*Line != '\r' && *Line != '\n')
    {
        Err = NextLine(Line, &Line);
        if (Err)
            goto exit;
    }        
    // Preskocit prazdne radky
    while (*Line == '\r' || *Line == '\n')
    {
        Err = NextLine(Line, &Line);
        if (Err)
        {
            if (Err != MAIL_UNK_MSG_FMT)
                goto exit;
            Err = NOERROR;
            break;
        }
    }        
    *Next = Line;

exit:

    return Err;
}
//
// Nacteni casti zasilky
//
DWORD CWBMailBoxPOP3::GetNextPart(int Part, LPSTR *pPtr, DWORD *pSize)
{ 
    DWORD Err;
    char *MsgPtr;

    if (Part && !*m_IDTbl[m_Msg - 1].m_Bound)
    {
        *pSize = 0;
        return NOERROR;
    }

    // IF jina part
    if (!m_PartOpen)
    {
        // IF v bufferu je az dalsi part, seek na zacatek
        if (Part < m_StartNO || (Part == m_StartNO && !m_StartOfStart))
        {
            m_Eof = FALSE;
            if (!SeekToTop())
                return OS_FILE_ERROR;
            Err = ReadBuf(TRUE);
            if (Err)
                return Err;
            m_FileNO       = 0;
            m_StartOfPart  = m_MsgBuf;
            m_EndOfPart    = NULL;
        }

        // IF pozadovana je aktualni a je v bufferu
        if (Part == m_FileNO && m_StartOfPart)
        {
            MsgPtr = m_StartOfPart;
        }
        else
        {
            // IF pozadovana part je za aktualni, hledam od konce aktualni, jinak od zacatku bufferu
            if (Part > m_FileNO && m_EndOfPart)
            {
                MsgPtr = m_EndOfPart;
            }
            else
            {
                m_FileNO = m_StartNO;
                MsgPtr   = m_StartOfStart;
                if (!MsgPtr)
                {
                    m_FileNO--;
                    MsgPtr = m_MsgBuf;
                }
            }
        
            // Dokud nenajdu pozadovanou part
            while (m_FileNO < Part)
            {
                // Hledej v bufferu boundery
                MsgPtr = strstr(MsgPtr, m_IDTbl[m_Msg - 1].m_Bound);
                // IF nenalezena, nacist dalsi kus dokud nenajdu zacatek dalsi part
                if (!MsgPtr)
                {
                    do
                    {
                        if (m_Eof)
                            return MAIL_NO_MORE_FILES;
                        Err = ReadBuf(FALSE);
                        if (Err)
                            return Err;
                    }
                    while (!m_StartOfStart);
                    MsgPtr = m_StartOfStart;
                }
                else
                {
                    MsgPtr += m_bLen;
                    if (*(WORD *)MsgPtr == ('-' << 8) + '-')
                        return MAIL_NO_MORE_FILES;
                    if (*MsgPtr != '\r' && *MsgPtr != '\n')
                        continue;
                }
                m_FileNO++;
            }
        
            // Preskocit prazdne radky
            while (*MsgPtr == '\r' || *MsgPtr == '\n')
            {
                MsgPtr++;
                if (MsgPtr >= m_NextLine)
                {
                    Err = ReadBuf(FALSE);
                    if (Err)
                        return Err;
                    MsgPtr = m_MsgBuf;
                }
            }
            m_StartOfPart = MsgPtr;
        }
        m_PartOpen = TRUE;
    }
    else
    {
        // IF Pos na konci part, Size = 0
        if (m_EndOfPart < m_NextLine || m_Eof)
        {
            *pSize   = 0;
            return NOERROR;
        }
        Err = ReadBuf(FALSE);
        if (Err)
            return Err;
        MsgPtr = m_MsgBuf;
    }
    // Najit konec part nebo konec bufferu
    m_EndOfPart     = NULL;
    if (m_bLen)
    {   char * p;
        for (p = MsgPtr; ; p += m_bLen)
        {
            p = strstr(p, m_IDTbl[m_Msg - 1].m_Bound);
            if (!p)
                break;
            if (*(WORD *)&p[m_bLen] == '-' + ('-' << 8))
                break;
            if (p[m_bLen] == '\r' || p[m_bLen] == '\n')
                break;
        }
        m_EndOfPart = p;
    }
    // IF nenalezena boundery
    if (!m_EndOfPart)
    {
        char *p;
        // Najit posledni neprazdny radek
        for (p = m_NextLine - 1; p > MsgPtr && (*p == '\r' || *p == '\n'); p--);
        // IF . na zacatku radku (konec zasilky)
        if (*p == '.' && (p == MsgPtr || p[-1] == '\n'))
        {
            for (;p > MsgPtr && (p[-1] == '\r' || p[-1] == '\n'); p--);
            if (p > MsgPtr)
            {
                if (*p == '\r')
                    p++;
                if (*p == '\n')
                    p++;
            }
            m_EndOfPart = p;
        }
        else
        {
            m_EndOfPart = m_NextLine;
        }
    }
    *pPtr    = MsgPtr;
    *pSize   = (DWORD)(m_EndOfPart - MsgPtr);

    return NOERROR;
}
//
// Cteni casti zasilky do bufferu
//
DWORD CWBMailBoxPOP3::ReadBuf(BOOL Start)
{
    m_StartOfStart = NULL;
    m_StartOfPart  = NULL;
    if (m_Eof)
        return NOERROR;
    DWORD Size;
    DWORD BufSize = GetBufSize();
    // Posunout zbytek na zacatek bufferu
    DWORD Rest = lstrlen(m_NextLine);
    if (Rest)
        memcpy(m_MsgBuf, m_NextLine, Rest);
    // Nacist ze souboru
    if (!ReadFile(m_hFile, m_MsgBuf + Rest, BufSize - Rest, &Size, NULL))
        return OS_FILE_ERROR;
    Size += Rest;
    m_MsgBuf[Size] = 0;
    m_MsgPtr       = m_MsgBuf;
    m_NextLine     = m_MsgBuf + Size;
    // IF buffer plny, najit konec posledniho celeho radku
    if (Size >= BufSize)
    {
        while (m_NextLine[-1] != '\n')
            m_NextLine--;
    }
    else    // ELSE Cely soubor nacten = TRUE
    {
        m_Eof = TRUE;
    }
    // IF zacatek zasilky
    if (Start)
    {
        m_StartNO      = 0;                                             // Cast na zacatku bufferu = 0
        m_StartOfStart = m_MsgBuf;                                      // Ukazatel na zacatek casti = m_MsgBuf
    }
    else if (m_bLen)                                                    // IF Multipart
    {
        char *p;                                                        // Ukazatel na zacatek casti = 1. neprazdny
        for (p = m_MsgBuf; ; )                                          // radek po boundary
        {
            p = strstr(p, m_IDTbl[m_Msg - 1].m_Bound);
            if (!p)
            {
                m_StartNO = m_FileNO;
                break;
            }
            p += m_bLen;
            if (*p == '\r' || *p == '\n')
            {
                while (*p == '\r' || *p == '\n')
                    p++;
                m_StartNO      = m_FileNO + 1;
                m_StartOfStart = p;
                break;
            }
        }
    }
    return NOERROR;
}
//
// Vraci hodnotu property Var:
// Pokud Var najde, vrati TRUE a ukazatel na hodnotu ve Val
// Pokud Var nenajde, vrati FALSE
//
BOOL CWBMailBoxPOP3::GetValC(char *Ptr, char *Var, int Len, char **Val)
{
    BOOL OK = FALSE;
    if (strnicmp(Ptr, Var, Len) == 0)
    {
        Ptr += Len;
        while (*Ptr == ' ' || *Ptr == '\t')
            Ptr++;
        OK = TRUE;
    }
    *Val = Ptr;
    return OK;
}
//
// Vraci hodnotu property Var=
// Pokud Var najde, vrati TRUE a ukazatel na hodnotu ve Val
// Pokud Var nenajde, vrati FALSE a ve Val ukazatel na dalsi property
//
BOOL CWBMailBoxPOP3::GetValE(char *Ptr, char *Var, int Len, char **Val)
{
    while (*Ptr == ' ' || *Ptr == '\t')
        Ptr++;
    if (strnicmp(Ptr, Var, Len) == 0)
    {
        Ptr += Len;
        while (*Ptr == ' ' || *Ptr == '\t')
            Ptr++;
        if (*Ptr == '=')
        {
            Ptr++;
            while (*Ptr == ' ' || *Ptr == '\t')
                Ptr++;
            *Val = Ptr;
            return TRUE;
        }
    }
    while (*Ptr != ';' && *Ptr != '\r' && *Ptr != '\n')
        Ptr++;
    if (*Ptr == ';')
    {
        Ptr++;
        while (*Ptr == ' ' || *Ptr == '\t')
            Ptr++;
    }
    *Val = Ptr;
    return FALSE;
}
//
// Kopiruje hodnotu property
//
void CWBMailBoxPOP3::ValCpy(char *Src, char *Dst, int Len)
{
    // IF zacina uvozovkami, az do dalsich uvozovek
    if (*Src == '"')
    {
        Src++;
        while (*Src != '"' && Len--)
            *Dst++ = *Src++;
    }
    // ELSE az do stredniku, konce radku nebo plneho bufferu
    else
    {
        while (*Src != ';' && *Src != '\r' && *Src != '\n' && Len--)
            *Dst++ = *Src++;
    }
    *Dst = 0;
}
//
// Cteni dalsiho radku zasilky
//
DWORD CWBMailBoxPOP3::NextLine(char *Ptr, char **Line)
{
    // Najit konec aktualniho radku
    while (*Ptr != '\n')
    {
        if (*Ptr == 0)
            return MAIL_UNK_MSG_FMT;
        Ptr++;
    }
    Ptr++;
    // IF na konci bufferu, nacist dalsi kus
    if (Ptr >= m_EndOfPart)
    {
        DWORD Size;
        DWORD Err = GetNextPart(m_FileNO, &Ptr, &Size);
        if (Err)
            return Err;
        if (Size == 0)
            return MAIL_UNK_MSG_FMT;
    }
    *Line = Ptr;
    return NOERROR;
}
//
// CWBMailBoxPOP3s 
//
// Destruktor
//
CWBMailBoxPOP3s::~CWBMailBoxPOP3s()
{
    m_Sock.Close();
    //DbgLogWrite("Mail      closesocket(POP3)");
    //DbgLogWrite("Mail  Receive Done");
    ReleaseLastMsg();
    if (m_IDTbl)
        delete []m_IDTbl;
    if (m_HdrBuf)
        free(m_HdrBuf);
}
//
// Inicializace schranky
//
DWORD CWBMailBoxPOP3s::Open()
{
    // vytvori a otevre socket, prihlasi se k host na port 110
    // jako user + passwd

    if (!*m_MailCtx->m_POP3Name)
        return MAIL_PROFSTR_NOTFND;

    DWORD  Err;
    char   buf[80];
    char  *Res;

    //DbgLogWrite("Mail  Receive");
    //DbgLogWrite("Mail      socket(POP3)");
    if (m_Sock.Socket())
        return MAIL_SOCK_IO_ERROR;
    //DbgLogFile("\n\rOpen = %d", m_Sock);
    if (m_Sock.SetAddress(m_MailCtx->m_POP3Server, 110))
        return MAIL_UNKNOWN_SERVER;
    //DbgLogWrite("Mail      connect(POP3, %d.%d.%d.%d)", HIBYTE(HIWORD(m_Sock.sin_addr.s_addr)), LOBYTE(HIWORD(m_Sock.sin_addr.s_addr)), HIBYTE(LOWORD(m_Sock.sin_addr.s_addr)), LOBYTE(LOWORD(m_Sock.sin_addr.s_addr)));
    if (m_Sock.Connect())
        return MAIL_CONNECT_FAILED;
    //DbgLogWrite("Mail      recv(POP3)");
    Err = m_Sock.Readln(&Res);
    //DbgLogFile("    < %s\r\n", Res);
    if (Err || *Res != '+')
        return MAIL_CONNECT_FAILED;
    m_Sock.Connected();
    // User
    int Sz = sprintf(buf, "USER %s\r\n", m_MailCtx->m_POP3Name);
    //DbgLogWrite("Mail      send(POP3)");
    //DbgLogFile("    > USER %s", m_MailCtx->m_POP3Name);
    Err = m_Sock.POP3_cmd(buf, Sz, &Res);
    //DbgLogFile("    < %s\r\n", Res);
    if (Err)
        return Err;
    if (*Res != '+')
        return MAIL_LOGON_FAILED;
    // Pass
    lstrcpy(buf, "PASS");
    DeMix(buf + 4, m_MailCtx->m_PassWord);
    char *pe = buf + 4 + buf[4] + 1;
    *(DWORD *)pe = '\r' + ('\n' << 8);
    buf[4] = ' ';
    //DbgLogFile("    > PASS %s", *buf ? "***" : "");
    Err = m_Sock.POP3_cmd(buf, (int)(pe + 2 - buf), &Res);
    //DbgLogFile("    < %s", Res);
    if (Err)
        return Err;
    if (*Res != '+')
        return MAIL_LOGON_FAILED;
    return NOERROR;
}
//
// Zruseni zasilky
//
DWORD CWBMailBoxPOP3s::DeleteMsg()
{
    char buf[20];
    char *Res;
    DWORD Err;
    char *MsgID = m_IDTbl[m_Msg - 1].m_MsgID;
    for (UINT i = 0; i < m_cMsgs; i++)
    {
        if (strcmp(MsgID, m_IDTbl[i].m_MsgID))
            continue;
        int Sz   = sprintf(buf, "DELE %i\r\n", i + 1);
        Err      = m_Sock.POP3_cmd(buf, Sz, &Res);
        if (Err)
            return Err;
    }
    return NOERROR;
}
//
// Seek na zacatek zasilky
//
BOOL CWBMailBoxPOP3s::SeekToTop()
{
    return SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) != (DWORD)-1;
}
//
// Uvolneni kontextu minule zasilky
//
void CWBMailBoxPOP3s::ReleaseLastMsg()
{
    if (FILE_HANDLE_VALID(m_hFile))
    {
        CloseFile(m_hFile);
        DeleteFile(m_FileName);
        m_hFile = INVALID_FHANDLE_VALUE;
    }
    if (m_MsgBuf)
    {
        free(m_MsgBuf);
        m_MsgBuf = NULL;
    }
}
//
// Zjisti pocet zasilek ve schrance a alokuje konverzni tabulku
//
DWORD CWBMailBoxPOP3s::Init()
{
    char *Line;
    m_Sock.Reset();
    //DbgLogFile("STAT");
    DWORD Err = m_Sock.POP3_cmd("STAT\r\n", 6, &Line);
    if (Err)
        return Err;
    if (*Line != '+')
    {
        DbgLogFile("MAIL_SOCK_IO_ERROR  Line != '+' 1");
        return MAIL_SOCK_IO_ERROR;
    }
    m_cMsgs = atoi(Line + 4);
    // Alokovat konverzni tabulku
    if (m_cMsgs)
    {
        m_IDTbl = new CPOP3MsgInfo[m_cMsgs];
        if (!m_IDTbl)
            return OUT_OF_APPL_MEMORY;
    }
    return NOERROR;
}
//
// Nacteni hlavicky zasilky
//
DWORD CWBMailBoxPOP3s::GetHdr(bool Save)
{
    char *Line;
    char  buf[64];
    //DbgLogFile("TOP");
    int   Sz  = sprintf(buf, "TOP %i 0\r\n", m_Msg);
    DWORD Err = m_Sock.POP3_cmd(buf, Sz, &Line);
    if (Err)
        return Err;
    if (*Line != '+')
    {
        //DbgLogFile("MAIL_SOCK_IO_ERROR  Line != '+' 2");
        return MAIL_SOCK_IO_ERROR;
    }
    m_HdrPtr = NULL;
    if (Save)
    {
        if (!m_HdrBuf)
        {
            m_HdrBuf = (char *)malloc(4096);
            if (!m_HdrBuf)
                return OUT_OF_APPL_MEMORY;
            m_HdrSize = 4096;
        }
        m_HdrPtr = m_HdrBuf;
    }

    return NOERROR;
}
//
// Nacte zasilku do docasneho souboru
//
DWORD CWBMailBoxPOP3s::LoadMsg()
{
    DWORD Err;
    if (FILE_HANDLE_VALID(m_hFile))
        return NOERROR;

    TempFileName(m_FileName);
    m_hFile = CreateFile(m_FileName, GENERIC_READ + GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    if (!FILE_HANDLE_VALID(m_hFile))
        return OS_FILE_ERROR;

    char *buf;
    char Retr[20];
    //DbgLogFile("RETR");
    int Sz = sprintf(Retr, "RETR %i\r\n", m_Msg);
    Err = m_Sock.POP3_cmd(Retr, Sz, &buf);
    if (Err)
        return Err;
    if (*buf != '+')
    {
        DbgLogFile("MAIL_SOCK_IO_ERROR  buf != '+'");
        return MAIL_SOCK_IO_ERROR;
    }
    DWORD ErrF = NOERROR;
    DWORD Size = m_Sock.GetRest(&buf);
    if (Size)
    {
        if (!WriteFile(m_hFile, buf, Size, &Size, NULL))
            ErrF = OS_FILE_ERROR;
    }
    while ((Err = m_Sock.Recv(&buf, &Size)) == NOERROR && Size > 0)
    {
        // Kdyz dojde pri zapisu do souboru k chybe, stejne se to musi docist do konce !!!
        if (!ErrF)
            if (!WriteFile(m_hFile, buf, Size, &Size, NULL))
                ErrF = OS_FILE_ERROR;
    }
    if (Err)
    {
        DbgLogFile("    m_Sock.Recv = %d", Err);
        return MAIL_SOCK_IO_ERROR;
    }
    if (ErrF)
    {
        ReleaseLastMsg();
        return ErrF;
    }
    m_MsgBuf = BufAlloc(GetFileSize(m_hFile, NULL) + 1, &m_MsgSize);
    if (!m_MsgBuf)
        return OUT_OF_APPL_MEMORY;
    m_MsgSize--;
    m_FileNO   = (WORD)-1;
    m_StartNO  = (WORD)-1;
    m_NextLine = m_MsgBuf;
   *m_NextLine = 0;
    m_PartOpen = FALSE;
    m_bLen     = lstrlen(m_IDTbl[m_Msg - 1].m_Bound);
    return NOERROR;
}
//
// Nacte dalsi radek zasilky
//
DWORD CWBMailBoxPOP3s::Readln(char **Line)
{
    DWORD Err = m_Sock.Readln(Line);
    if (Err == NOERROR && m_HdrPtr && **Line != 0 && **Line != '.')
    {
        int Len = lstrlen(*Line);
        int Off = (int)(m_HdrPtr - m_HdrBuf);
        if (Off + Len + 4 > m_HdrSize)
        {
            m_HdrSize *= 2;
            m_HdrBuf = (char *)realloc(m_HdrBuf, m_HdrSize);
            if (!m_HdrBuf)
                return OUT_OF_APPL_MEMORY;
            m_HdrPtr = m_HdrBuf + Off;
        }
        memcpy(m_HdrPtr, *Line, Len);
        m_HdrPtr += Len;
        *(DWORD *)m_HdrPtr = '\r' + ('\n' << 8);
        m_HdrPtr += 2;
    }
    return Err;
}
//
// Vraci MIN(velikost bufferu, velikost zasilky)
//
DWORD CWBMailBoxPOP3s::GetBufSize()
{
    return m_MsgSize;
}
#ifdef UNIX
//
// CWBMailBoxPOP3f
//
// Destruktor
//
CWBMailBoxPOP3f::~CWBMailBoxPOP3f()
{
    if (FILE_HANDLE_VALID(m_hFile))
        CloseFile(m_hFile);
    ReleaseLastMsg();
    if (m_MsgBuf)
        free(m_MsgBuf);
    if (m_IDTbl)
        delete []m_IDTbl;
}
//
// Inicializace schranky
//
DWORD CWBMailBoxPOP3f::Open()
{
    // otevre soubor schranky

    m_hFile = CreateFile(m_MailCtx->m_POP3Server, GENERIC_READ + GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    if (!FILE_HANDLE_VALID(m_hFile))
        return OS_FILE_ERROR;
    return NOERROR;
}
//
// Zruseni zasilky
//
DWORD CWBMailBoxPOP3f::DeleteMsg()
{
    DWORD Err   = NOERROR;
    DWORD fSize = SetFilePointer(m_hFile, 0, NULL, FILE_END);
    do
    {
        DWORD Size;
        DWORD Offset = m_IDTbl[m_Msg].m_Offset;
        DWORD Diff   = Offset - m_IDTbl[m_Msg - 1].m_Offset;
        for (;;)
        {
            SetFilePointer(m_hFile, Offset, NULL, FILE_BEGIN);
            if (!ReadFile(m_hFile, m_MsgBuf, m_MsgSize, &Size, NULL))
                return OS_FILE_ERROR;
            if (!Size)
                break;
            SetFilePointer(m_hFile, Offset - Diff, NULL, FILE_BEGIN);
            if (!WriteFile(m_hFile, m_MsgBuf, Size, &Size, NULL))
                return OS_FILE_ERROR;
            Offset += Size;
        }
        if (m_Msg < m_cMsgs)
            memcpy(&m_IDTbl[m_Msg - 1], &m_IDTbl[m_Msg], (m_cMsgs - m_Msg) * sizeof(CPOP3MsgInfo));
        m_cMsgs--;
        for (int i = m_Msg - 1; i < m_cMsgs; i++)
	        m_IDTbl[i].m_Offset = m_IDTbl[i].m_Offset - Diff;
        fSize   -= Diff;
        DWORD ID = m_ID;
        m_ID     = (DWORD)-1;
        Err      = GetMsgID(ID);
        m_ID     = ID;
    }
    while (Err == NOERROR);
    if (Err == MAIL_MSG_NOT_FOUND)
        Err = NOERROR;
    ftruncate(m_hFile, fSize);
    return Err;
}
//
// Seek na zacatek zasilky
//
BOOL CWBMailBoxPOP3f::SeekToTop()
{
    return SetFilePointer(m_hFile, m_IDTbl[m_Msg - 1].m_Offset, NULL, FILE_BEGIN) != (DWORD)-1;
}
//
// Uvolneni kontextu minule zasilky
//
void CWBMailBoxPOP3f::ReleaseLastMsg()
{
}
//
// Alokuje konverzni tabulku
//
inline BOOL CWBMailBoxPOP3f::AllocMsgInfo(int items)
{
    CPOP3MsgInfo *nt = new CPOP3MsgInfo[m_IDTblSz + items];
    if (!nt) return FALSE;
    if (m_IDTbl)
    {
	    for (int i = 0; i < m_IDTblSz; i++)
		    nt[i].m_Offset = m_IDTbl[i].m_Offset;
	    delete []m_IDTbl;
    }
    m_IDTbl    = nt;
    m_IDTblSz += 32;
    return TRUE;
}

/* Format souboru:
 zasilka=[\n|^]<FROM line><non-FROM lines><empty line>
 <from line>=From .*\n

 Nekdo pouziva demanglovani >>>From na >>From
 Nekdo ma Content-legth, nekdo Lines:
 Ale standard je zrejme spolehat jen na FROM radky
 */

//
// Zjisti pocet zasilek ve schrance a vytvori konverzni tabulku
//

DWORD CWBMailBoxPOP3f::Init()
{
    DWORD Size;
    DWORD Offset; /* pozice v postovnim souboru */

    m_cMsgs   = 0;
    m_MsgBuf  = BufAlloc(GetFileSize(m_hFile, NULL) + 1, &m_MsgSize);
    if (!m_IDTbl && !AllocMsgInfo(32)) 
        return OUT_OF_APPL_MEMORY;
    m_MsgSize--;

    if (!m_IDTblSz) AllocMsgInfo(32);
    for (Offset = 0, m_cMsgs = 0; ; Offset += Size)
    {
        SetFilePointer(m_hFile, Offset, NULL, FILE_BEGIN);
        if (!ReadFile(m_hFile, m_MsgBuf, m_MsgSize, &Size, NULL))
            return OS_FILE_ERROR;
        m_MsgBuf[Size] = 0;
        char *p = strrchr(m_MsgBuf, '\n');
        if (!p)
            continue; /* v bloku neni \n, tudiz ani nova zprava */
        p--; /* chceme, aby \n bylo na zacatku pristiho bloku */
        Size = p + 1 - m_MsgBuf; /* velikost vcetne NUL */
        if (!Size)
            break;
        m_MsgBuf[Size] = 0;
        p = m_MsgBuf;
        for(;;) 
        {
            if (m_cMsgs + 2 > m_IDTblSz)
                AllocMsgInfo(32);
            m_IDTbl[m_cMsgs].m_Offset = Offset + p - m_MsgBuf;
            m_cMsgs++;
    	    p = strstr(p, "\nFrom ");
	        if (!p++)
                break;
        }
    }
    m_IDTbl[m_cMsgs].m_Offset = Offset+2;
    return NOERROR;
}
//
// Nacteni hlavicky zasilky
//
DWORD CWBMailBoxPOP3f::GetHdr(bool Save)
{
    DWORD Size;

    SetFilePointer(m_hFile, m_IDTbl[m_Msg - 1].m_Offset, NULL, FILE_BEGIN);
    if (!ReadFile(m_hFile, m_MsgBuf, 8 * 1024, &Size, NULL))
        return OS_FILE_ERROR;
    m_MsgBuf[Size] = 0;
    char *p = strstr(m_MsgBuf, "\n\n");
    if (!p)
        return MAIL_UNK_MSG_FMT;
    *(DWORD *)p = '\n' + ('.' << 8) + ('\n' << 16);
    m_NextLine = m_MsgBuf;
    m_HdrBuf   = m_MsgBuf;
    m_HdrPtr   = p;
    return NOERROR;
}
//
// 
//
DWORD CWBMailBoxPOP3f::LoadMsg()
{
    DWORD Err;
    m_FileNO   = (WORD)-1;
    m_StartNO  = (WORD)-1;
    m_NextLine = m_MsgBuf;
   *m_NextLine = 0;
    m_PartOpen = FALSE;
    m_bLen     = lstrlen(m_IDTbl[m_Msg - 1].m_Bound);
    return NOERROR;
}
//
// Nacte dalsi radek zasilky
//
DWORD CWBMailBoxPOP3f::Readln(char **Line)
{
    char *p = m_NextLine;
    char *n = strchr(m_NextLine, '\n');
    if (!n)
        return MAIL_UNK_MSG_FMT;
   *n = 0;
    m_NextLine = n + 1;
   *Line = p;
    return NOERROR;
}
//
// Vraci MIN(velikost bufferu, velikost zasilky)
//
DWORD CWBMailBoxPOP3f::GetBufSize()
{
    DWORD Size = m_IDTbl[m_Msg].m_Offset - m_IDTbl[m_Msg - 1].m_Offset;
    if (Size > m_MsgSize)
        Size = m_MsgSize;
    return Size;
}
#endif
//
// Inicializace streamu pro zapis do souboru
//
DWORD CAttStreamSMTPfi::Open()
{
    m_hFile = CreateFile(m_DstPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    if (!FILE_HANDLE_VALID(m_hFile))
        return OS_FILE_ERROR;
    return NOERROR;
}
//
// Zapis pripojeneho souboru na disk
//
DWORD CAttStreamSMTPfi::Write(LPCSTR Buf, DWORD Size)
{
    if (!WriteFile(m_hFile, Buf, Size, &Size, NULL))
        return OS_FILE_ERROR;
    return NOERROR;
}
//
// Zapis pripojeneho souboru do databaze
//
DWORD CAttStreamSMTPdi::Write(LPCSTR Buf, DWORD Size)
{ t_varcol_size sz;
    return CAttStreamd::Write(m_Offset, Buf, Size, &sz);
}


#ifdef SRVR
//
// Zapise delku attributu, odemkne zaznam, uvolni kurzor nebo tabulku
//
CAttStreamd::~CAttStreamd()
{
    if (m_wLock)
    {
        if (m_Index != NOINDEX)
            tb_write_ind_len(m_cdp, m_tbdf, m_Pos, m_Attr, m_Index, m_Offset);
        else
            tb_write_atr_len(m_cdp, m_tbdf, m_Pos, m_Attr, m_Offset);
        tb_unwlock(m_cdp, m_tbdf, m_Pos);
    }
    if (m_rLock)
        tb_unrlock(m_cdp, m_tbdf, m_Pos);
    if (m_cd)
        free_cursor(m_cdp, m_Curs);
    else if (m_Curs != NOCURSOR)
        unlock_tabdescr(m_tbdf);
    if (m_cdp->roll_back_error && (m_cdp->sqloptions & SQLOPT_ERROR_STOPS_TRANS))
        roll_back(m_cdp);
    else
        commit(m_cdp);
}
//
// Otevre dotaz, zjisti, zda obsahuje pouze jeden zaznam, zjisti cislo atributu
//
DWORD CAttStreamd::OpenCursor(LPCSTR Table, LPCSTR AttrName, LPCSTR Cond)
{
    UINT Sz = lstrlen(Table) + lstrlen(AttrName) + lstrlen(Cond) + 32;
    char *Query = (char *)corealloc(Sz, 0xCD);
    if (!Query)
        return OUT_OF_APPL_MEMORY;
    sprintf(Query, "SELECT * FROM %s WHERE %s", Table, Cond);

    m_Curs = open_working_cursor(m_cdp, Query, &m_cd);
    if (m_Curs == NOCURSOR)
        goto error;
    if (m_cd->recnum != 1)
    {
        request_error(m_cdp, SQ_CARDINALITY_VIOLATION);
        goto error;
    }
    d_attr *Att;
    for (Att = m_cd->d_curs->attribs; Att < m_cd->d_curs->attribs + m_cd->d_curs->attrcnt; Att++)
        if (sys_stricmp(Att->name, AttrName) == 0)
            break;
    m_Attr = (tattrib)(Att - m_cd->d_curs->attribs);
    if (m_Attr >= m_cd->d_curs->attrcnt)
    {
        request_error(m_cdp, BAD_ELEM_NUM);
        goto error;
    }
    m_Pos = 0;

error:

    corefree(Query);
    return m_cdp->get_return_code();
}
//
// Instaluje tabulku resp. prevede zaznam a atribut v kurzoru na zaznam a atribut v tabulce, zkontroluje
// pristupova prava a zamkne zaznam
//
DWORD CAttStreamd::Open(BOOL WriteAcc)
{
    tattrib tattr;
    t_privils_flex priv_val;

    if (!m_cd)
    { 
        m_tbdf = install_table(m_cdp, m_Curs);
        if (!m_tbdf) 
            goto error;  // error inside
        if (m_Pos >= m_tbdf->Recnum())
        { 
            request_error(m_cdp, OUT_OF_TABLE);
            goto error; 
        }
        if (m_Attr >= m_tbdf->attrcnt)
        {
            request_error(m_cdp, BAD_ELEM_NUM);
            goto error; 
        }
    }
    else  /* cursor: all tables already installed */
    { 
        elem_descr *attracc;
        ttablenum   tbnum;
        if (m_cd->curs_eseek(m_Pos))
            goto error;
        if (!attr_access(&m_cd->qe->kont, m_Attr, tbnum, tattr, m_Pos, attracc))
        {
            request_error(m_cdp, BAD_ELEM_NUM);
            goto error; 
        }
        m_tbdf = tables[tbnum];  // tabdescr is locked in the cursor
    }
    if (WriteAcc)
    {
        if (!m_cd)  // table rights
        { 
            m_cdp->prvs.get_effective_privils(m_tbdf, m_Pos, priv_val);
            if (m_tbdf->tbnum <= KEY_TABLENUM || !priv_val.has_write(m_Attr))
            {
                t_exkont_table ekt(m_cdp, m_tbdf->tbnum, m_Pos, m_Attr, NOINDEX, OP_WRITE, NULL, 0);
                request_error(m_cdp, NO_RIGHT);
                goto error; 
            }
        }
        else      // cursor access rights:
        { 
            if (!(m_cd->qe->qe_oper & QE_IS_UPDATABLE))
            { 
                request_error(m_cdp, CANNOT_UPDATE_IN_THIS_VIEW);
                goto error;
            }
            if (!(m_cd->d_curs->attribs[m_Attr].a_flags & RI_UPDATE_FLAG))
            {
                if (!(m_cd->d_curs->tabdef_flags & TFL_REC_PRIVILS))
                { 
                    request_error(m_cdp, NO_RIGHT);
                    goto error;
                }
                else
                {
                    m_cdp->prvs.get_effective_privils(m_tbdf, m_Pos, priv_val);
                    if (m_tbdf->tbnum <= KEY_TABLENUM || !priv_val.has_write(tattr))
                    {
                        t_exkont_table ekt(m_cdp, m_tbdf->tbnum, m_Pos, tattr, NOINDEX, OP_WRITE, NULL, 0);
                        request_error(m_cdp, NO_RIGHT);
                        goto error; 
                    }
                }
            }
            m_Attr = tattr;
        }
        tb_wlock(m_cdp, m_tbdf, m_Pos);
        m_wLock = TRUE;
    }
    else
    {
        if (!m_cd)  // table rights
        {
            m_cdp->prvs.get_effective_privils(m_tbdf, m_Pos, priv_val);
            if (m_tbdf->tbnum <= KEY_TABLENUM || !priv_val.has_read(m_Attr))
            {
                t_exkont_table ekt(m_cdp, m_tbdf->tbnum, m_Pos, m_Attr, NOINDEX, OP_READ, NULL, 0);
                request_error(m_cdp, NO_RIGHT);
                goto error;
            }
        }
        else
        {
            // check cursor access rights, unless checked when filling own table
            if (!m_cd->insensitive && !(m_cd->d_curs->attribs[m_Attr].a_flags & RI_SELECT_FLAG))
            {
                if (!(m_cd->d_curs->tabdef_flags & TFL_REC_PRIVILS))
                { 
                    request_error(m_cdp, NO_RIGHT);
                    goto error;
                }
                else
                {
                    m_cdp->prvs.get_effective_privils(m_tbdf, m_Pos, priv_val); // tbdf defined only is attracc==NULL
                    if (m_tbdf->tbnum <= KEY_TABLENUM || !priv_val.has_read(tattr))
                    {
                        t_exkont_table ekt(m_cdp, m_tbdf->tbnum, m_Pos, tattr, NOINDEX, OP_READ, NULL, 0);
                        request_error(m_cdp, NO_RIGHT);
                        goto error; 
                    }
                }
            }
        }
        tb_rlock(m_cdp, m_tbdf, m_Pos);
        m_rLock = TRUE;
    }

error:

    return m_cdp->get_return_code();
}
//
// Nacte delku atributu
//
DWORD CAttStreamd::GetSize(uns32 * pSize)
{
    BOOL Err;
    if (m_Index != NOINDEX)
        Err = tb_read_ind_len(m_cdp, m_tbdf, m_Pos, m_Attr, m_Index, pSize);
    else
        Err = tb_read_atr_len(m_cdp, m_tbdf, m_Pos, m_Attr, pSize);
    return Err ? m_cdp->get_return_code() : NOERROR;
}
//
// Zapise obsah bufferu do atributu
//
DWORD CAttStreamd::Write(t_varcol_size Offset, LPCSTR Buf, t_varcol_size Size, t_varcol_size * pSize)
{
    *pSize = 0;
    if (m_Index != NOINDEX)
    {
        if (tb_write_ind_var(m_cdp, m_tbdf, m_Pos, m_Attr, m_Index, Offset, Size, Buf))
            goto error;
    }
    else
    {   write_one_column(m_cdp, m_tbdf, m_Pos, m_Attr, Buf, Offset, Size);
#ifdef STOP
        t_tab_atr_upd_trig trg(m_tbdf, m_Attr);
        m_tbdf->prepare_trigger_info(m_cdp);
        BOOL AnyTrg = trg.any_trigger();
        if (AnyTrg)
        { 
            if (trg.pre(m_cdp, m_Pos, (char *)Buf, Size))
                goto error;
            if (trg.updated_val)
            { 
                t_value *val = (t_value *)trg.updated_val;
                Buf  = val->indir.val;
                Size = val->length;
            }
        }
        if (tb_write_var(m_cdp, m_tbdf, m_Pos, m_Attr, Offset, Size, Buf))
            goto error;
        if (AnyTrg && trg.post(m_cdp, m_Pos))
            goto error;
#endif
    }
    Offset += Size;
    if (Offset > m_Offset)
        m_Offset = Offset;
    *pSize = Size;
    return NOERROR;

error:

    return m_cdp->get_return_code();
}
//
// Precte obsah atributu do bufferu
//
DWORD CAttStreamd::Read(t_varcol_size Offset, char *Buf, t_varcol_size Size, t_varcol_size * pSize)
{
    *pSize = 0;
    if (m_Index != NOINDEX)
    {
        if (tb_read_ind_var(m_cdp, m_tbdf, m_Pos, m_Attr, m_Index, Offset, Size, Buf))
            goto error;
    }
    else
    {
        if (tb_read_var(m_cdp, m_tbdf, m_Pos, m_Attr, Offset, Size, Buf))
            goto error;
    }
    Offset += m_cdp->tb_read_var_result;
    if (Offset > m_Offset)
        m_Offset = Offset;
    *pSize = m_cdp->tb_read_var_result;
    return NOERROR;

error:

    return m_cdp->get_return_code();
}
//
// Smaze obsah BLOBU
//
void CAttStreamd::Delete()
{
    if (m_Index != NOINDEX)
        tb_write_ind_len(m_cdp, m_tbdf, m_Pos, m_Attr, m_Index, 0);
    else
        tb_write_atr_len(m_cdp, m_tbdf, m_Pos, m_Attr, 0);
}
    
#else // !SRVR
//
// Zapise delku attributu, odemkne zaznam, uvolni kurzor nebo tabulku
//
CAttStreamd::~CAttStreamd()
{
    if (m_wLock)
    {
        cd_Write_len(m_cdp, m_Curs, m_Pos, m_Attr, m_Index, m_Offset);
        cd_Write_unlock_record(m_cdp, m_Curs, m_Pos);
    }
    if (m_rLock)
        cd_Read_unlock_record(m_cdp, m_Curs, m_Pos);
    if (m_Curs != NOCURSOR && IS_CURSOR_NUM(m_Curs))
        cd_Close_cursor(m_cdp, m_Curs);
}
//
// Otevre dotaz, zjisti, zda obsahuje pouze jeden zaznam, zjisti cislo atributu
//
DWORD CAttStreamd::OpenCursor(LPCSTR Table, LPCSTR AttrName, LPCSTR Cond)
{
    DWORD Err = NOERROR;
    UINT Sz = lstrlen(Table) + lstrlen(AttrName) + lstrlen(Cond) + 32;
    char *Query = (char *)corealloc(Sz, 0xCD);
    if (!Query)
        return OUT_OF_APPL_MEMORY;
    sprintf(Query, "SELECT * FROM %s WHERE %s", Table, Cond);

    if (cd_Open_cursor_direct(m_cdp, Query, &m_Curs))
        goto error;
    trecnum rCnt;
    if (cd_Rec_cnt(m_cdp, m_Curs, &rCnt))
        goto error;
    if (rCnt != 1)
    {
        Err = SQ_CARDINALITY_VIOLATION;
        goto err;
    }
    uns8  tp;
    if (!cd_Attribute_info(m_cdp, m_Curs, AttrName, &m_Attr, &tp, &tp, NULL))
        Err = BAD_ELEM_NUM;
    m_Pos = 0;

err:

    corefree(Query);
    return Err;

error:

    corefree(Query);
    return cd_Sz_error(m_cdp);
}
//
// Zamkne zaznam
//
DWORD CAttStreamd::Open(BOOL WriteAcc)
{
    if (WriteAcc)
    {
        if (!cd_Write_lock_record(m_cdp, m_Curs, m_Pos))
        {        
            m_wLock = TRUE;
            return NOERROR;
        }
    }
    else
    {
        if (!cd_Read_lock_record(m_cdp, m_Curs, m_Pos))
        {
            m_rLock = TRUE;
            return NOERROR;
        }
    }
    return cd_Sz_error(m_cdp);
}
//
// Nacte delku atributu
//
DWORD CAttStreamd::GetSize(t_varcol_size * pSize)
{
    if (cd_Read_len(m_cdp, m_Curs, m_Pos, m_Attr, m_Index, pSize))
        return cd_Sz_error(m_cdp);
    return NOERROR;
}
//
// Zapise obsah bufferu do atributu
//
DWORD CAttStreamd::Write(t_varcol_size Offset, LPCSTR Buf, t_varcol_size Size, t_varcol_size * pSize)
{
    *pSize = 0;
    if (cd_Write_var(m_cdp, m_Curs, m_Pos, m_Attr, m_Index, Offset, Size, Buf))
        return cd_Sz_error(m_cdp);
    Offset += Size;
    if (Offset > m_Offset)
        m_Offset = Offset;
    *pSize = Size;
    return NOERROR;
}
//
// Precte obsah atributu do bufferu
//
DWORD CAttStreamd::Read(t_varcol_size Offset, char *Buf, t_varcol_size Size, t_varcol_size * pSize)
{
    *pSize = 0;
    if (cd_Read_var(m_cdp, m_Curs, m_Pos, m_Attr, m_Index, Offset, Size, Buf, pSize))
        return cd_Sz_error(m_cdp);
    Offset += *pSize;
    if (Offset > m_Offset)
        m_Offset = Offset;
    return NOERROR;
}
//
// Smaze obsah BLOBU
//
void CAttStreamd::Delete()
{
    cd_Write_len(m_cdp, m_Curs, m_Pos, m_Attr, m_Index, 0);
}

#endif // SRVR

//
// CIPSocket
//
// SetAddress nastavi IP adresu, je-li zadano jmeno serveru, necha ho prelozit
//
int CIPSocket::SetAddress(char *Addr, unsigned short DefPort)
{
#ifdef NETWARE
    NETDB_DEFINE_CONTEXT
#endif
    memset((char *)(sockaddr_in *)this, 0, sizeof(sockaddr_in));

    // IF Adresa ve formatu a1.a2.a3.a4
    unsigned a1, a2, a3, a4, port, ip;
    int flds = sscanf(Addr, " %u . %u . %u . %u : %u", &a1, &a2, &a3, &a4, &port);
    if (flds >= 4)
    {
        ip = (a1 << 24) | (a2 << 16) | (a3 << 8) | a4;
        sin_family = AF_INET;	
        sin_addr.s_addr = htonl(ip);
        if (flds > 4)
            sin_port = htons(port);
    }
    else // zeptat se DNS
    {
        char *p = strchr(Addr, ':');
        if (p)
        {
            sin_port = htons(atoi(p+1));
            *p = 0;
        }
        struct hostent *hp = gethostbyname(Addr);
        //DbgLogWrite("          gethostbyname(%s)", Addr);
        if (p)
            *p = ':';
        if (!hp)
            return Errno();
        memcpy((char *)&sin_addr, hp->h_addr, hp->h_length);
        sin_family= hp->h_addrtype;
    }
    
    // IF nezadan port SMTP, zjistit, implicitne 25
    if (!sin_port)
        sin_port = htons(DefPort);
    //DbgLogFile("    SetAddr(%s) = %3d.%3d.%3d.%3d:%d", Addr, sin_addr.s_addr & 0xFF, (sin_addr.s_addr >> 8) & 0xFF, (sin_addr.s_addr >> 16) & 0xFF, (sin_addr.s_addr >> 24) & 0xFF, ntohs(sin_port));
    return 0;
}
//
// Provede connect a je-li socket blokovan, pocka chvili na odblokovani
//
int CIPSocket::Connect()
{
    unsigned long param = 1;
#ifdef WINS
    ioctlsocket(m_Sock, FIONBIO, &param);
    //if (ioctlsocket(m_Sock, FIONBIO, &param) == SOCKET_ERROR)
#else
    ioctl(m_Sock, FIONBIO, &param);
    //if (ioctl(m_Sock, FIONBIO, &param) == SOCKET_ERROR)
#endif
        DbgLogFile("ioctl(%d) = %d", m_Sock, Errno());

    int Err = ::connect(m_Sock, (sockaddr *)this, sizeof(sockaddr_in));
    if (Err != SOCKET_ERROR)
        return 0;
#ifdef DEBUG 
    m_Err = 
#endif
    Err = Errno();
    DbgLogFile("    connect(%d) = %d", m_Sock, Err);
    if (IsBlocked(Err))
    { 
        //DbgLogFile("    connect(%d.%d.%d.%d:%d) = WSAEWOULDBLOCK", HIBYTE(HIWORD(ip)),LOBYTE(HIWORD(ip)),HIBYTE(LOWORD(ip)),LOBYTE(LOWORD(ip)), port);
        fd_set  writeset; 
        timeval timeout = {10, 0};
        FD_ZERO(&writeset);
        FD_SET(m_Sock, &writeset);
#ifdef UNIX
        Err = select(m_Sock + 1, NULL, &writeset, NULL, &timeout);
        //DbgLogFile("    select(%d) = %d", m_Sock, Err);
#else
        select(1, NULL, &writeset, NULL, &timeout);
        DbgLogFile("    select(%d) = %d", m_Sock, Errno());
#endif
        if (FD_ISSET(m_Sock, &writeset))
        {
            DbgLogFile("    connect OK");
            return 0;
        }
        else
        {
            DbgLogFile("    connect Timeout");
            return SOCKET_ERROR;
        }
    }
    return Err;
}
//
// Odesila data pres socket (nemusi najednou odeslat vsech Sz butu)
//
int CIPSocket::Send(char *Buf, int Sz, int *Wrtn)
{
    int Err = 0;
    fd_set  writeset;
    timeval timeout = {30, 0};
    FD_ZERO(&writeset);  
    FD_SET(m_Sock, &writeset);
#ifdef UNIX
    select(m_Sock + 1, NULL, &writeset, NULL, &timeout);
#else
    select(1, NULL, &writeset, NULL, &timeout);
#ifdef DEBUG
    if (Errno())
        //DbgLogFile("    selects(%d) = %d", m_Sock, Errno());
#endif
#endif
    if (!FD_ISSET(m_Sock, &writeset))
    {
        *Wrtn = 0;
        //DbgLogFile("    Send Timeout");
        return SOCKET_ERROR;
    }
    Sz = ::send(m_Sock, Buf, Sz, 0);
    if (Sz == SOCKET_ERROR)
    {
        Sz = 0;
#ifdef DEBUG
        m_Err = 
#endif
        Err = Errno();
        //DbgLogFile("    Send %d", Err);
        if (IsBlocked(Err))
            Err = 0;
    }
        
    *Wrtn = Sz;
    return Err;
}
//
// Odesle len bytu
//
int CIPSocket::Write(char *buf, int len)
{
    int index = 0;
    int wr;
    DbgLogFile("write %.123s\n\r", buf + index);

    do
    {
        int Err = Send(buf + index, len - index, &wr);
        if (Err)
            return Err;
        index += wr;
    }
    while (index < len);
    return NOERROR;
}
//
// Cteni dat ze socketu
//
int CIPSocket::Recv()
{
    int Err = 0;
    DWORD toRead;
    m_Read = 0;
    for (int Sec = 0; ;Sec++)
    {
#ifdef WINS
        ioctlsocket(m_Sock, FIONREAD, &toRead);
        //DbgLogFile("    ioctlsocket(%d) = %d", m_Sock, toRead); 
        //if (ioctlsocket(m_Sock, FIONREAD, &toRead) == SOCKET_ERROR)
        //    DbgLogFile("ioctlsocket = %d", Errno()); 
#else
        ioctl(m_Sock, FIONREAD, &toRead);
#endif
        //DbgLogFile("toRead = %d", toRead);
        if (toRead)
            break;
        if (Sec)
            return SOCKET_ERROR;
        fd_set  readset;
        timeval timeout = {60, 0};
        FD_ZERO(&readset);  
        FD_SET(m_Sock, &readset);
#ifdef UNIX
        select(m_Sock + 1, &readset, NULL, NULL, &timeout);
#else
#ifdef DEBUG
        DWORD tc = GetTickCount();
#endif
        int sel = select(1, &readset, NULL, NULL, &timeout);
#ifdef DEBUG
        //DbgLogFile("    select = %d (%d) %d", sel, Errno(), GetTickCount() - tc);
#endif
#endif
        if (!FD_ISSET(m_Sock, &readset))
        {
            //DbgLogFile("    Recv Timeout");
            return SOCKET_ERROR;
        }
    }
    m_BufRest = SOCKBUF_SIZE - (int)(m_BufPtr - m_Buf);
    if (toRead > m_BufRest)
        toRead = m_BufRest;
    //DbgLogFile("    recv(%d, %d)", m_Sock, toRead);
    m_Read = ::recv(m_Sock, m_BufPtr, toRead, 0);
    if (m_Read == SOCKET_ERROR)
    {
#ifdef DEBUG
        m_Err =
#endif
        Err = Errno();
        //DbgLogFile("    recv %d", Err);
        m_Read = 0;
        if (IsBlocked(Err))
        {
            Sleep(50);
            Err = NOERROR;
            //DbgLogFile("    recv2(%d, %d)", m_Sock, toRead);
            m_Read = ::recv(m_Sock, m_BufPtr, toRead, 0);
            if (m_Read == SOCKET_ERROR)
            {
                Err = Errno();
                //DbgLogFile("    recv2 %d", Err);
                m_Read = 0;
            }
        }
    }
    //DbgLogFile("    recv = %d", m_Read);
    return Err;
}
//
// Socket pro replikace
//
// Poskytuje dva zpusoby cteni. Pro nacteni hlavicky cte pokud neni v bufferu pozadovane
// mnozstvi bytu, pro cteni paketu, cte tolik, kolik prislo. Konec se pozna podle delky
// v hlavicce.
//
// Accept - Ceka na zahajeni komunikace z druhe strany
//
int CReplSock::Accept(SOCKET Socket)
{
#ifdef UNIX
    unsigned sz = sizeof(sockaddr_in);
#else
    int sz = sizeof(sockaddr_in);
#endif
    m_Sock = accept(Socket, (sockaddr *)(sockaddr_in *)this, &sz);
    if (m_Sock == SOCKET_ERROR)
    {
        int Err;
#ifdef DEBUG
        m_Err = 
#endif
        Err = Errno();
        //DbgLogFile("Accept(%d) %d", Socket, Err);
        return Err;
    }
    unsigned long param = 1;
#ifdef WINS
    ioctlsocket(m_Sock, FIONBIO, &param);
#else
    ioctl(m_Sock, FIONBIO, &param);
#endif
    setsockopt(m_Sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&param, sizeof(param));
    m_BufRest = SOCKBUF_SIZE;
    m_BufPtr  = m_Buf;
    m_PktPtr  = m_Buf;
    return NOERROR;
}
//
// Cte sz bytu hlavicky paketu
//
DWORD CReplSock::Read(char **buf, int sz)
{
    for (int rd = (int)(m_BufPtr - m_PktPtr); rd < sz; rd += m_Read)
    {
        DWORD Err = CIPSocket::Recv();
        if (Err)
            return Err;
        if (!m_PktPtr)
            m_PktPtr = m_Buf;
        rd        += m_Read;
        m_BufPtr  += m_Read;
        m_BufRest -= m_Read;
    }
    *buf = m_PktPtr;
    m_PktPtr += sz;
    return NOERROR;
}
//
// Vraci zbytek dat nactenych do bufferu
//
int CReplSock::GetRest()
{
    int Sz    = (int)(m_BufPtr - m_PktPtr);
    m_BufPtr  = m_Buf;
    m_BufRest = SOCKBUF_SIZE;
    return Sz;
}
//
// Cte ze socketu tolik, kolik prislo
//
int CReplSock::Recv(char **Buf, int *rd)
{
    int Err = CIPSocket::Recv();
    if (!Err)
    {
        *Buf = m_Buf;
        *rd  = m_Read;
    }
    return Err;
}
//
// Socket pro postu
//
// Poskytuje dva zpusoby cteni. Pro cteni odpovedi na prikazy a pro cteni hlavicky (TOP), 
// cte pokud neni v bufferu cely radek do CRLF, pro cteni cele zasilky, cte tolik, kolik
// zrovna prislo. Konec se pozna podle samotne tecky na radku.
//
//
// Write - Zapise string do socketu
//
DWORD CMailSock::Write()
{
    return CIPSocket::Write(m_Buf, (int)(m_CmdPtr - m_Buf)) ? MAIL_SOCK_IO_ERROR : NOERROR;
}
//
// Precte ze socketu radek (do LF)  
// Jednoradkove odpovedi na SMTP/POP3 prikazy, viceradkova odpved na TOP
//
DWORD CMailSock::Readln(char **buf)
{
    DWORD Err;
    char *p;
    m_LinePtr = m_LineNext;
    for (;;)
    {
        // IF je v bufferu cely radek
        //DbgLogFile("    Readln  %08X %08X %08X %08X", m_Buf, m_BufPtr, m_LinePtr, m_LineNext);
        if ((p = strchr(m_LinePtr, '\r')) && p[1] == '\n')
        {
            m_LineNext = p + 2;
            break;
        }
        else if ((p = strchr(m_LinePtr, '\n')))
        {            
            m_LineNext = p + 1;
            break;
        }
        // IF aktualni radek neni na zacatku bufferu
        if (m_LinePtr > m_Buf)
        {
            // IF v bufferu neco zbyva, presunout na zacatek
            int Rest = (int)(m_BufPtr - m_LinePtr);
            if (Rest > 0)
                memcpy(m_Buf, m_LinePtr, Rest);
            m_BufPtr  = m_Buf + Rest;
            m_LinePtr = m_Buf;
        }
        //DbgLogFile("    RecvB   %08X %08X %08X %08X", m_Buf, m_BufPtr, m_LinePtr, m_LineNext);
        Err = CIPSocket::Recv();
        //DbgLogFile("    RecvA   %08X", m_Read);
        if (Err)
            return Err;
        m_BufPtr += m_Read;
       *m_BufPtr  = 0;
    }
    *p   = 0;
    *buf = m_LinePtr;
    //DbgLogFile("    Readln = %.240s", m_LinePtr);
    return NOERROR;
}
//
// Cteni viceradkove odpovedi ze SMTP socketu
//
// Posledni radek odpovedi ma tvar: kod xxxxxxxx
// jinak                            kod-xxxxxxxx
//
DWORD CMailSock::ReadMulti(char **buf)
{
#ifdef _DEBUG
    char *pb = m_BufPtr;
#endif
    do
    {
        DWORD Err = Readln(buf);
        if (Err)
            return Err;
    }
    while (lstrlen(*buf) > 3 && (*buf)[3] == '-');
#ifdef _DEBUG
    DbgLogFile("Read  %.128s\r\n", pb);
#endif
    return NOERROR;
}
//
// Cte data ze socketu
// Konec zasilky se pozna podle samotne tecky na zacatku radku
//
#define EOF2 ('\n' + ('.' << 8))
#define EOF3 ('\n' + ('.' << 8) + ('\n' << 16))
#define EOF4 ('\n' + ('.' << 8) + ('\r' << 16) + ('\n' << 24))

int CMailSock::Recv(char **Buf, DWORD *rd)
{
    char *pEof = m_BufPtr + m_Read;
   *pEof  = 0;
    pEof -= 4;
    if (pEof < m_Buf)
        pEof = m_Buf;
    // IF na konci nactenych dat neni LF. ani aspon LF, nemuze byt konec zasilky
    if (*(WORD *)pEof != EOF2 && *(WORD *)pEof != '\n')
    {
        pEof++;
        if (*(WORD *)pEof != EOF2 && *(WORD *)pEof != '\n')
        {
            pEof++;
            if (*(WORD *)pEof != EOF2 && *(WORD *)pEof != '\n')
            {
                pEof++;
                if (*pEof != '\n')
                    pEof = NULL;
            }
        }
    }
    // IF muze byt konec zasilky
    if (pEof)
    {
        // IF konec zasiky se vesel do bufferu, konec
        if (*(DWORD *)pEof == EOF4 || *(DWORD *)pEof == EOF3)
        {
            *rd = 0;
            //DbgLogFile("    CMailSock::Recv = EOF");
            return NOERROR;
        }
        // Soupnout zacatek konce na zacatek bufferu
        for (m_BufPtr = m_Buf; *pEof; pEof++, m_BufPtr++)
            *m_BufPtr = *pEof;
        m_BufRest = SOCKBUF_SIZE - (int)(m_BufPtr - m_Buf);
        pEof = NULL;
    }
    else
    {
        m_BufPtr  = m_Buf;
        m_BufRest = SOCKBUF_SIZE;
    }
    int Err = CIPSocket::Recv();
    if (Err)
    {
        //DbgLogFile("    CIPSocket::Recv = %d", Err);
        return MAIL_SOCK_IO_ERROR;
    }
    *Buf = m_BufPtr;
    *rd  = m_Read;
#ifdef DEBUG
    m_BufPtr[m_Read] = 0;
#endif
    //DbgLogFile("    CMailSock::Recv = %.200s", m_BufPtr);
    return NOERROR;
}
//
// Plneni bufferu
//
void CMailSock::SMTP_newline(char *Src, int Sz)
{
    memcpy(m_Buf, Src, Sz);
    m_CmdPtr = m_Buf + Sz;
}

void CMailSock::SMTP_line(char *Src, int Sz)
{
    memcpy(m_CmdPtr, Src, Sz);
    m_CmdPtr += Sz;
}
//
// Writes current date and time into message header
//
static const char *monname[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *dayname[7]  = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void CMailSock::SMTP_Date()
{
    time_t tm;
    struct tm *ltm;

    time(&tm);
    ltm = localtime(&tm);
#ifdef NEVER
    /* Use strftime to build a customized time string. */
#ifdef LINUX
	char *OldLC = getenv("LC_ALL");
	setlocale(LC_ALL, "en_US");
	//printf(OldLC);
    int Sz = strftime(m_CmdPtr, 80, "Date: %a, %d %b %Y %H:%M:%S %z\r\n", ltm);
	//printf(m_CmdPtr);
	setlocale(LC_ALL, OldLC);
	//printf(OldLC);
#else
    int Sz  = strftime(m_CmdPtr, 80, "Date: %a, %d %b %Y %H:%M:%S ", ltm);
    m_CmdPtr += Sz;
    sprintf(m_CmdPtr, "%+03d%02d\r\n", - _timezone / 3600, _timezone % 3600 / 60);
    Sz = 7;
#endif
#endif
    int my_timezone;
#ifdef WINS
    { TIME_ZONE_INFORMATION tzi;
      DWORD res = GetTimeZoneInformation(&tzi);
      my_timezone=-(60*(res==TIME_ZONE_ID_DAYLIGHT ? tzi.Bias+tzi.DaylightBias : tzi.Bias));
    }
#else
    { struct timeval tv;  struct timezone tz;
      gettimeofday(&tv, &tz);
      my_timezone=-(60*tz.tz_minuteswest);
    }
#endif
    sprintf(m_CmdPtr, "Date: %s, %d %s %d %02d:%02d:%02d %+03d%02d\r\n", dayname[ltm->tm_wday], ltm->tm_mday, monname[ltm->tm_mon], 1900 + ltm->tm_year, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, my_timezone / 3600, my_timezone % 3600 / 60);
    int Sz = (int)strlen(m_CmdPtr);
    m_CmdPtr += Sz;
   // NOTE: The "timezone" variable does not indicate the summer time on Windows 
}
//
// Writes hostname into SMTP command
//
void CMailSock::SMTP_hostname()
{
    gethostname(m_CmdPtr, 90);
    m_CmdPtr += lstrlen(m_CmdPtr);
    *(WORD *)m_CmdPtr = '\r' + ('\n' << 8);
    m_CmdPtr += 2;
}
//
// Odeslani SMTP prikazu a nacteni odpovedi
//
DWORD CMailSock::SMTP_cmd(char **buf)
{
#ifdef DEBUG
    m_CmdPtr[-2] = 0;
    //DbgLogFile("    > %s", m_Buf);
    m_CmdPtr[-2] = '\r';
#endif
    if (CIPSocket::Write(m_Buf, (int)(m_CmdPtr - m_Buf)))
        return MAIL_SOCK_IO_ERROR;

    Reset();
    DWORD Err = ReadMulti(buf);
    //DbgLogFile("    < %.80s\r\n", *buf);
    return Err;
}
//
// Odeslani POP3 prikazu a nacteni odpovedi
//
DWORD CMailSock::POP3_cmd(char *line, int Sz, char **Res)
{ 
    //DbgLogFile("SND: %s", line);
    if (CIPSocket::Write(line, Sz))
    {
        //DbgLogFile("CIPSocket::Write");
        return MAIL_SOCK_IO_ERROR;
    }
    Reset();
    DWORD Err = Readln(Res);
    //DbgLogFile("Rcv: %s", *Res);
    return Err;
}

void CMailSock::Close(void)
{
    if (m_Sock != SOCKET_ERROR)
    {
        if (m_Connected)
        {
            char *Res;
            POP3_cmd("QUIT\r\n", 6, &Res);
        }
        CIPSocket :: Close();
    }
}

//
// Konverze data z hlavicky POP3 zasilky
//
void P3DTM2WB(char *dt, LPDWORD WBDate, LPDWORD WBTime)
{
    char *Months   = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec";
    char  Mon[4];
    char  Dummy[64];
    int   Day, Year, Hour, Min, Sec;

    *WBDate = NONEDATE;
    *WBTime = NONETIME;
    if 
    (
        sscanf(dt, " %63s %d %3s %d %d:%d:%d", Dummy, &Day, Mon, &Year, &Hour, &Min, &Sec) == 7 ||
        sscanf(dt,    "   %d %3s %d %d:%d:%d",        &Day, Mon, &Year, &Hour, &Min, &Sec) == 6
    )
    {
        DWORD *m = (DWORD *)Months;
        int i;
        for (i = 0; i < 12; i++, m++)
            if (*(DWORD *)&Mon == *m)
                break;
        if (i < 12)
        {
            *WBDate = Make_date(Day, i + 1, Year);
            *WBTime = Make_time(Hour, Min, Sec, 0);
        }
    }
}

/*
DWORD WB2DDTM(DWORD WBDate, DWORD WBTime)
{
    DWORD DDTM = 0;
    if (WBDate != 0 && WBDate != NONEINTEGER)
        *((LPWORD)&DDTM + 1) = Day(WBDate) | (Month(WBDate) << 5) | ((Year(WBDate) - 1980) << 9);
    if (WBTime != 0 && WBTime != NONEINTEGER)
        *(LPWORD)&DDTM = (Hours(WBTime) << 11) | (Minutes(WBTime) << 5) | (Seconds(WBTime) / 2);
    return DDTM;
}
*/
//
// Konverze DOS data
//
void DDTM2WB(DWORD DDTM, LPDWORD WBDate, LPDWORD WBTime)
{
    if (!DDTM)
    {
        *WBDate = NONEINTEGER;
        *WBTime = NONEINTEGER;
    }
    else
    {
        *WBDate = Make_date((DDTM >> 16) & 0x1F, (DDTM >> 21) & 0xF, (DDTM >> 25) + 1980);
        *WBTime = Make_time((DDTM >> 11) & 0x1F, (DDTM >> 5) & 0x3F, (DDTM & 0x1F) * 2, 0);
    }
}
//
// Alokace bufferu
//
// Alokuje buffer pro cteni ze souboru maximalne 1 MB minimalne 8 KB
//
char *BufAlloc(uns32 Size, uns32 * pSize)
{
    if (Size > 0x100000)
        Size = 0x100000;
    char *Buf;
    while ((Buf = (char *)malloc(Size)) == NULL)
    {
        Size /= 2;
        if (Size < 8 * 1024)
            return NULL;
    }
    if (pSize)
        *pSize = Size;
    return Buf;
}
//
// Vraci cestu k docasnemu souboru
//
void TempFileName(char *Buf)
{
    char TempDir[MAX_PATH];
#ifdef WINS
    GetTempPath(sizeof(TempDir), TempDir);
#else
    *(WORD *)TempDir = '.';
#endif
    GetTempFileName(TempDir, "", 0, Buf);
}

#ifdef WINS

//
// RAS API
//
LPRASENUMCONNECTIONS  lpRasEnumConnections;
LPRASDIAL             lpRasDial;
LPRASHANGUP           lpRasHangUp;
LPRASGETCONNECTSTATUS lpRasGetConnectStatus;
//
// Loads DialUp dll and necessary API functions
//
DWORD CDialCtx::RasLoad()
{
    if (m_hInst)
        return NOERROR;
    DWORD Err = MAIL_DLL_NOT_FOUND;
    m_hInst = LoadLibrary((LPSTR)"rasapi32.dll");
    if (!m_hInst)
        return MAIL_DLL_NOT_FOUND;
    
    lpRasEnumConnections  = (LPRASENUMCONNECTIONS)GetProcAddress(m_hInst, "RasEnumConnectionsA");
    if (!lpRasEnumConnections)
        goto exit;
    lpRasDial             = (LPRASDIAL)GetProcAddress(m_hInst, "RasDialA");
    if (!lpRasDial)
        goto exit;
    lpRasHangUp           = (LPRASHANGUP)GetProcAddress(m_hInst, "RasHangUpA");
    if (!lpRasHangUp)
        goto exit;
    lpRasGetConnectStatus = (LPRASGETCONNECTSTATUS)GetProcAddress(m_hInst, "RasGetConnectStatusA");
    if (!lpRasGetConnectStatus)
        goto exit;

    return NOERROR;

exit:

    return MAIL_FUNC_NOT_FOUND;
}
//
// Dials DialUp connection
//
DWORD CDialCtx::Dial(char *DialConn, char *User, char *PassWord, LPHRASCONN lpConn)
{   
#ifdef SRVR
    WBMailSect.Init();
    WBMailSect.Enter();
#endif

    RASCONN RasConn[4];
    DWORD   Sz = sizeof(RasConn);
    DWORD   Cnt;
    RasConn->dwSize = sizeof(RASCONN);
    DWORD   Err = lpRasEnumConnections(RasConn, &Sz, &Cnt);
    //DbgLogFile("RasEnumConnections = %08X  %d", Err, Cnt);
    if (Err)
    {
        //DbgLogFile("MAIL_ERROR lpRasEnumConnections");
        Err = Err == ERROR_NOT_ENOUGH_MEMORY ? OUT_OF_APPL_MEMORY : MAIL_ERROR;
        goto exit;
    }
    // IF uz existuje spojeni
    if (Cnt)        
    {
        for (DWORD i = 0; i < Cnt; i++)
        {
            //DbgLogFile("   %s", RasConn[i].szEntryName);
            if (lstrcmpi(RasConn[i].szEntryName, DialConn) == 0)
            {
                // IF vytocil jsem ho ja, citac++, jinak ho vytocil nekdo jiny a nebudu ani zavesovat
                if (m_cRef)
                    m_cRef++;
                goto exit;
            }
        }
        Err = ERROR_IN_FUNCTION_ARG;
        goto exit;
    }
    else
    {
        m_cRef = 0;
    }
    RASDIALPARAMS rdp;
    ZeroMemory(&rdp, sizeof(RASDIALPARAMS));
    rdp.dwSize=sizeof(RASDIALPARAMS);
    lstrcpy(rdp.szEntryName, DialConn);
    lstrcpy(rdp.szUserName,  User);
    lstrcpy(rdp.szPassword,  PassWord);

    m_hDialConn = 0;  
    Err = lpRasDial(NULL, NULL, &rdp, 0, NULL, &m_hDialConn);
    //DbgLogWrite("RasDial");
    if (Err)
    { 
        if (m_hDialConn)
            HangUp(m_hDialConn);
        Err = MAIL_DIAL_ERROR;
        goto exit;
    }
    m_cRef++;

exit:

    *lpConn = Err ? NULL : m_hDialConn;

#ifdef SRVR
    WBMailSect.Leave();
#endif
    return Err;
}
//
// Closes DialUp connection
//
DWORD CDialCtx::HangUp(HRASCONN hDialConn)
{ 
    // IF neni spojeni
    if (!m_hDialConn)
        return NOERROR;
    // IF jine spojeni
    if (hDialConn != m_hDialConn)
        return ERROR_IN_FUNCTION_ARG;
    // IF jeste nekdo telefonuje
    if (m_cRef && --m_cRef)
        return NOERROR;
    DWORD Err;
#ifdef SRVR
    WBMailSect.Enter();
#endif
    //DbgLogWrite("RasHangUp");
    if (lpRasHangUp(m_hDialConn))
    {
        Err = MAIL_DIAL_ERROR;
        goto exit;
    }
    RASCONNSTATUS rcs;
    rcs.dwSize = sizeof(RASCONNSTATUS);
    while (lpRasGetConnectStatus(m_hDialConn, &rcs) != ERROR_INVALID_HANDLE)
        Sleep(0);
    //DbgLogFile("Zaveseno");
exit:
    m_hDialConn = 0;
#ifdef SRVR
    WBMailSect.Leave();
    WBMailSect.Delete();
#endif
    return NOERROR;
}

#ifdef MAIL602
//
// Callback funkce FaxMailServeru
//
// Tato callback funkce je jedina me znama moznost jak sledovat stav komunikace. 
// Kdyz prijde zprava "Schrankovani ukonceno.", znamena to, ze dosel seznam zasilek
// nebo pozadovana zasilka, ovsem je potreba jeste chvili pockat nez ji klient zpracuje.
// Kdyz prijde zprava "KONEC -> PostUrad", znamena to, ze vystupni schranka je prazdna
// a FaxMailServer zavesuje.
//
void GateZobraz(char *Msg)
{
#ifdef DEBUG

    //char buf[256];
    //sprintf(buf, "GATE %s", Msg);
    //OutputDebugString(buf);
    char *d = strchr(Msg, '\r');
    if (!d)
        d = strchr(Msg, '\n');
    *d = 0;
    DbgPrint(Msg);
    //DbgLogFile(Msg);

#endif

    char *p = Msg;
    p += Msg[7] == '>' ? 9 : 10;
    if (memcmp(p, "Schr\0xE1nkov\0xE1n\0xED ukon\0xE8eno.", 22) == 0)
        SetEvent(RemoteCtx.m_GateRec);
    else if (memcmp(p, "KONEC   -> ", 11) == 0 && memcmp(p + 11, "\0xE8ten\0xED fronty z\0xE1silek", 20) != 0)
    {
        SetEvent(RemoteCtx.m_GateON);
        //DbgLogFile("Konec GateON ON");
    }
}

//
// Vlakno FaxMailServeru
//
THREAD_PROC(GateThread)
{ 
    LPG602MAIN lpG602Main = (LPG602MAIN)GetProcAddress(RemoteCtx.m_hGate602, "g602main");
    if (!lpG602Main)
        return 0;

    char par[MAX_PATH];
    char *ppar[2];
    ppar[0] = NULL;
    ppar[1] = par;
    lstrcpy(par,    "-emi:");
    lstrcpy(par + 5, RemoteCtx.m_Emi);
	SetCurrentDirectory(RemoteCtx.m_Emi);
    //DbgLogFile("G602Main IN");
    DWORD Res = lpG602Main(1, ppar);
	RemoteCtx.m_hGateThread = NULL;
    SetEvent(RemoteCtx.m_GateON);
    //DbgLogFile("G602Main Exit GateON ON");
    RemoteCtx.m_Emi[0] = 0;
    return Res;
}
//
// Navazani dial spojeni s materskym postovnim uradem
//
DWORD CRemoteCtx::Dial(char *Emi, HINSTANCE hMail602, UINT TimeOut)
{
    DWORD Err;

    //DbgLogFile("RemoteDial %s", Emi);
    if (*m_Emi && lstrcmpi(m_Emi, Emi))
        return MAIL_ALREADY_INIT;
    if (m_hGateThread)
    {
        m_cRef++;
        return NOERROR;
    }

    lstrcpy(m_Emi, Emi);
    if (!m_hGate602)
    {
        // Naloudovat FaxMailServer
        m_hGate602 = LoadLibrary("G602RM32.DLL");
        if (!m_hGate602)
            return MAIL_DLL_NOT_FOUND;
        // Ukazatele na opakovane volane funkce
        Err = MAIL_FUNC_NOT_FOUND;
        lpSpeedUp = (LPSPEEDUP)GetProcAddress(m_hGate602, "SpeedUp");
        if (!lpSpeedUp)
            goto error;
        lpExitGate = (LPEXITGATE)GetProcAddress(m_hGate602, "ExitGate");
        if (!lpExitGate)
            goto error;
		lpForceReread = (LPFORCEREREAD)GetProcAddress(m_hGate602, "ForceReread"); 
        if (!lpForceReread)
            goto error;
        lpSetWBTimeout  = (LPSETWBTIMEOUT)GetProcAddress(m_hGate602, "SetWBTimeout");
        if (!lpSetWBTimeout)
            goto error;
        lpSendGBATFromRMMsgList = (LPSENDGBATFROMRMMSGLIST)GetProcAddress(hMail602, "SendGBATFromRMMessageList");
        if (!lpSendGBATFromRMMsgList)
            goto error;

		LPSETZOBRAZFUNC lpSetZobraz = (LPSETZOBRAZFUNC)GetProcAddress(m_hGate602, "SetZobrazFunction");
		if (lpSetZobraz)
			lpSetZobraz(GateZobraz);
        
        // Thread pro FaxMailServer
        //DbgLogFile("ThreadStart");
        THREAD_START_JOINABLE(::GateThread, 8 * 1024, this, &m_hGateThread);
        // Shodi se pri zacatku komunikace, nahodi se po zaveseni nebo pri chybe
        //DbgLogFile("CreateEvent ON");
        m_GateON  = CreateEvent(NULL, TRUE, TRUE, "WBMailGateOnEvent");
        // Shodi se pri zarazeni pozadavku, nahodi se kdyz dojde nejaka zasilka
        //DbgLogFile("CreateEvent REC");
        m_GateRec = CreateEvent(NULL, TRUE, TRUE, "WBMailGateRecEvent");
        //DbgLogFile("CreateEvent OUT");
        if (!m_hGateThread || !m_GateON || !m_GateRec)
        {
            //DbgLogFile("MAIL_ERROR CreateEvent");
            Err = MAIL_ERROR;
            goto error;
        }
        //DbgLogFile("SetWBTimeout IN");
        //lpSetWBTimeout(TimeOut);
        //DbgLogFile("SetWBTimeout OUT");
        m_cRef++;
        return NOERROR;
    }

error:

    FreeLibrary(m_hGate602);
    m_hGate602 = NULL;
    return Err;
}
//
// Ukonceni spojeni s materskym uradem
//
void CRemoteCtx::HangUp()
{
    // IF neni remote, tak nic
    if (!m_hGate602)
        return;
    // IF FaxMailServer bezi 
    if (m_hGateThread)
    {
        if (--m_cRef)
            return;
        //DbgLogFile("WaitForGate");
        WaitForSingleObject(m_GateON, INFINITE);
        //DbgLogFile("ExitGate IN");
        lpExitGate();
        //DbgLogFile("ExitGate %08X", m_hGateThread);
        WaitForSingleObject(m_hGateThread, INFINITE);
        //DbgLogFile("WaitFor GateThread");
        CloseHandle(m_GateON);
        m_GateON = NULL;
    }
    // IF nebezi, muzeme odloadovat dllku
    if (!m_hGateThread)
    {
        FreeLibrary(m_hGate602);
        m_hGate602 = NULL;
        m_cRef     = 0;
        m_Emi[0]   = 0;
    }
}
#endif // MAIL602
//
// Konverze MAPI chyby
//
DWORD hrErrToszErr(HRESULT hrErr)
{
    if (hrErr == E_OUTOFMEMORY || hrErr == MAPI_E_NOT_ENOUGH_RESOURCES)
        return OUT_OF_APPL_MEMORY;
    if (hrErr == MAPI_E_NOT_INITIALIZED)
        return MAIL_NOT_INITIALIZED;
    if (hrErr == MAPI_E_VERSION)
        return INCOMPATIBLE_VERSION;
    if (hrErr == E_ACCESSDENIED)
        return NO_RIGHT;
    if (hrErr == MAPI_E_PASSWORD_CHANGE_REQUIRED || hrErr == MAPI_E_PASSWORD_EXPIRED)
        return PASSWORD_EXPIRED;
    if (hrErr == MAPI_E_NOT_ENOUGH_DISK)
        return OUT_OF_BLOCKS;
    if (hrErr == MAPI_E_DISK_ERROR)
        return OS_FILE_ERROR;
    if (hrErr == MAPI_E_LOGON_FAILED)
        return MAIL_LOGON_FAILED;
    if (hrErr == ERROR_INVALID_SERVICE_ACCOUNT)
        return MAIL_SYSTEM_ACCOUNT;
    if (hrErr == MAPI_E_NO_SUPPORT)
        return MAIL_NO_SUPPORT;
    //DbgLogFile("MAIL_ERROR hrErrToszErr %08X", hrErr);
    return MAIL_ERROR;
}

#ifdef MAIL602
//
// Konverze Mail602 chyby
//
DWORD Err602ToszErr(DWORD Err)
{
    if (Err == 2)
        return MAIL_BAD_PROFILE;
    if (Err == 3)
        return MAIL_BAD_USERID;
    if (Err == 4)
        return BAD_PASSWORD;
    if (Err == 5)
        return OUT_OF_APPL_MEMORY;
    if (Err == 19)                      // Service nema pristup na soubory Mail602
        return MAIL_NO_SUPPORT;
    if (Err == 42)
        return MAIL_BAD_USERID;
    if (Err == 46)
        return MAIL_NO_ADDRESS;
    if (Err == 167)
        return NO_RIGHT;

    //DbgLogFile("MAIL_ERROR Err602ToszErr %d", Err);
    return MAIL_ERROR;
}
#endif // MAIL602
#endif // WINS
