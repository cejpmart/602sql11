#ifdef WINS
#define USES_IID_IMessage
#define MIDL_PASS  // usefull for express VC with old SDK
#undef BOOKMARK  // workaround on 64-bit platforms
#define BOOKMARK MAPI_BOOKMARK
#include <mapix.h>
#include <mapiutil.h>
#undef BOOKMARK
#include <ras.h>
#include <process.h>
#ifdef MAIL602
#include "wm602m32.h"
#endif
#else
#include <netinet/in.h>
#define SOCKET int
#endif
//
// Message flags
//
#define WBL_READRCPT   0x0001               // Read receipt requested
#define WBL_DELAFTER   0x0002               // Delete from outbox after submit
#define WBL_PRILOW     0x0004               // Low priority
#define WBL_PRIHIGH    0x0008               // High priority
#define WBL_SENSPERS   0x0010               // Personal sensitivity
#define WBL_SENSPRIV   0x0020               // Private sensitivity
#define WBL_SENSCONF   0x0040               // Company confidential sensitivity
#define WBL_REMSENDNOW 0x0080               // Send message immediately (Remote/SMTP Mail602 client)
#define WBL_MSGINFILE  0x0100               // Msg parameter of LetterCreate is path to file containing message body
//
// Outging message flags
//
#define WBL_BCC        0x0200               // Carbon copy recipients are blind carbon copy recipients
#define WBL_DELFAFTER  0x0400               // Delete attachments after successful submit
//
// Incomming message flags
//
#define WBL_VIEWED     0x0200               // Message is read
#define WBL_FAX        0x0400               // Message is fax (Mail602 only)
#define WBL_REMOTE     0x0800               // Message is on remote post office (Mail602 only)
#define WBL_DELREQ     0x1000               // Message delete on remote post office requsted (Mail602 only)
//
// Internal message flags
//
#define WBL_UNICODESB  0x10000000           // Subject and body is in UNICODE
#define WBL_AUTH       0x20000000           // SMTP server requires authentication
#define WBL_602        0x40000000           // Mail602 address specified
#define WBL_MAPI       0x80000000           // MAPI address specified
//
// Mail type
//
#define WBMT_602       0x00000001           // Mail602
#define WBMT_MAPI      0x00000002           // MAPI
#define WBMT_SMTPOP3   0x00000004           // SMTP/POP3
#define WBMT_602REM    0x00010000           // Remote Mail602 client
#define WBMT_602SP3    0x00020000           // SMTP/POP3 client
#define WBMT_DIALUP    0x00040000           // DialUp connection requested
#define WBMT_DIRPOP3   0x00080000           // POP3 by direct message file access (Linux only)

#ifndef NOERROR
#define NOERROR 0
#endif

#ifdef UNIX
#define _MAX_PATH MAX_PATH
#endif

#ifndef WINS
#define SOCKET_ERROR   -1
#endif

typedef struct
{
	char MailDir[256];
	char GateDir[256];
	char CodStr[9];
	char UserName[31];
	char UserID[9];
	char OffName[9];
}
RMConfig;

#ifdef WINS
#ifdef MAIL602
//
// Mail602
//
typedef struct
{
    long Size;
    long Index;
    WORD Flag;
    char *UIDL;
    char *MsgId;
    char *Sender;
    char *Subject;
    long Date;
}
TPOP3MInfo, *LPTPOP3MINFO;

typedef short (WINAPI *LPJEEMI100)(char *Adresar);
typedef char  (WINAPI *LPMAIL602ISREADY)();
typedef short (WINAPI *LPINITMAIL602)(char *Mail602Dir, char *User, char *Password);
typedef short (WINAPI *LPINITMAIL602X)();
typedef short (WINAPI *LPINITMAIL602LOGGEDUSER)(char *Mail602Dir);
typedef short (WINAPI *LPINITMAIL602LOGGEDUSERX)(char *Mail602Dir);
typedef void  (WINAPI *LPDISPOSEMESSAGEFILELIST)(PMessageFileList *Files);
typedef int   (WINAPI *LPGETMAIL602USERNAMEFROMID)(char *Mail602Dir, char *User, char *ID);
typedef short (WINAPI *LPADDADDRESSTOADDRESSLIST)(PMail602Address Address, PMail602AddressList *AddressList);
typedef short (WINAPI *LPADDFILETOFILELIST)(char *FileName, char *LongName, PFileList *FileList);
typedef void  (WINAPI *LPDISPOSEADDRESSLIST)(PMail602AddressList *AddressList);
typedef void  (WINAPI *LPDISPOSEFILELIST)(PFileList *FileList);
typedef short (WINAPI *LPSENDMAIL602MESSAGE)(PMail602AddressList *Address, PMail602AddressList *CC,
                       char *Subject, char *Letter, PFileList *Files, long BeginDate, long EndDate,
                       unsigned char Certified, unsigned char NovellMessage, PTitlePage *TitlePage,
                       PSignature *Signature,  unsigned char EDI, unsigned char Executable, unsigned short Priorita);
typedef short (WINAPI *LPSENDMAIL602MESSAGEEX)(PMail602AddressList Address, PMail602AddressList CC,
                       char *Subject, char *Letter, void *LetterAsFileList, PFileList *Files, long BeginDate, long EndDate,
                       unsigned char Certified, unsigned char NovellMessage, PTitlePage TitlePage,
                       PSignature Signature,  unsigned char EDI, unsigned char Executable, unsigned short Priorita,
                       BOOL SendAsPOP3User, char *POP3SenderAddress, char *HTMLLetter,
                       long  SFlags,            // sfOverrideArchive=1 - ze plati nasledujici parametry
                       short HowArchiveLetter,  // 0/1/2...nic/evidovat/archivovat
                       short HowArchiveFiles,   // 0/1/2/3/4...nic/evidovat/archivovat/do 100K/do 1M
                       long *ID);               // ID vytvorene zasilky, pripadne prvni vytvorene
typedef void  (WINAPI *LPGETMAILBOXSTATUSINFO)(int *NoOfMessages, int *TotalSize, int *NoOfMessagesNotSeen, int *NoOfUnconfirmedMessages);
typedef short (WINAPI *LPGETMSGLISTINQUEUE)(BOOL Delivered, BOOL Own, PMessageInfoList *MessageInfoList);

typedef void (*PZobrFunc)(char *);
typedef BYTE  (WINAPI *LPG602MAIN)(char argc, char **argv);
typedef void  (WINAPI *LPEXITGATE)(void);  
typedef void  (WINAPI *LPSPEEDUP)(void);
typedef void  (WINAPI *LPFORCEREREAD)(void);
typedef void  (WINAPI *LPSETZOBRAZFUNC)(PZobrFunc ZobrazFunc);
typedef short (WINAPI *LPSENDGBATFROMRMMSGLIST)(WORD Opt, PMessageInfoList *ml, char *pw, DWORD db, DWORD de, WORD Pri);
typedef short (WINAPI *LPREADRMCONFIG)(char *path, RMConfig **cfg);
typedef void  (WINAPI *LPSETWBTIMEOUT)(unsigned short WBT);

typedef int   (WINAPI *LPSENDQUEUEDINTERNETMSGS)(LPSTR Mail602Dir, LPSTR Mail602ID, LPSTR MailServer, HWND Ahclient, HANDLE AhInst, UINT AWMMessage);

typedef short (WINAPI *LPGETMSGLIST)        (PMessageInfoList *MessageList);
typedef void  (WINAPI *LPFREEMSGLIST)       (PMessageInfoList *MessageList);
typedef short (WINAPI *LPSIZEOFMSG)         (TMessageInfo *pmsg, BYTE MsgType, /*Mail602Folder **/ LPVOID Folder, LPDWORD Size);
typedef short (WINAPI *LPREADMSG)           (char *MsgID, char *MsgPath, char *FileDir, LPWORD ExInfo);
typedef short (WINAPI *LPREADLETTERFROMMSG) (char *MsgID, char *MsgPath);
typedef short (WINAPI *LPGETFILELIST)       (char *MsgID, PMessageFileList *Files);
typedef void  (WINAPI *LPFREEFILELIST)      (PMessageFileList *Files);
typedef short (WINAPI *LPCOPYFILEFROMMSG)   (char *MessageID, LPCSTR FileName, char *NewFileName);
typedef short (WINAPI *LPDELETEMSG)         (char *MessageID);
typedef short (WINAPI *LPPOP3GETLISTOFMSGS) (TPOP3MInfo *oldlist, UINT OldCount, UINT *Count, ULONG *Size, LPSTR POP3Server, LPSTR POP3Username, LPSTR POP3Password, BOOL UseAPOP, TPOP3MInfo **list,
                                             UINT *FullCount, ULONG *FullSize, HWND Ahclient, HANDLE AhInst, UINT AWMMessage);
typedef void  (WINAPI *LPPOP3FREELISTOFMSGS)(TPOP3MInfo *list, UINT FullCount);
typedef int   (WINAPI *LPPOP3GETORERASEMSGS)(LPSTR Mail602Dir, LPSTR InQueue, TPOP3MInfo *oldlist, UINT OldFullCount, TPOP3MInfo **newlist, UINT *NewCount, ULONG *NewSize, UINT *NewFullCount,
                                             ULONG *NewFullSize, LPSTR Mail602ID, LPSTR POP3Server, LPSTR POP3Username, LPSTR POP3Password, BOOL UseAPOP, BOOL DeleteOnHost, HWND Ahclient, HANDLE AhInst, UINT AWMMessage);

typedef short (WINAPI *LPGETLOCALUSERID)(char *User, char *ID);
typedef short (WINAPI *LPGETMYUSERINFO)(char *uName, char *uFullName, LPDWORD ID, char *oName, char *oFullName);
typedef short (WINAPI *LPGETEXTERNALUSERLIST)(char *Office, PMail602AddressList *List);
typedef short (WINAPI *LPGETPRIVATELIST)(PListList *List);
typedef short (WINAPI *LPGETPRIVATEUSERLIST)(char *ListName, PMail602AddressList *List);
typedef void  (WINAPI *LPDISPOSELISTSLISTS)(PListList *List);
typedef short (WINAPI *LPREADADRFILEX)(TMessageInfo *pmsg, char MsgType, LPVOID Folder, PMail602AddressList *AddressList, WORD Flag);
#endif // MAIL602

#else   // LINUX
int ToUnicode(const char *Src, int SrcSz, wuchar *Dst, int DstSz, wbcharset_t SrcCharSet);
#endif  // WINS

#ifndef SRVR
extern "C"
{
DllExport DWORD WINAPI InitWBMail(char *Profile, char *PassWord);
DllExport DWORD WINAPI InitWBMailEx(char *Profile, char *RecvPassWord, char *SendPassWord);
DllExport DWORD WINAPI InitWBMail602(char *EmiPath, char *UserID, char *PassWord);
DllExport DWORD WINAPI InitWBMail602x(char *Profile);
DllExport void  WINAPI CloseWBMail();

DllExport DWORD WINAPI LetterCreate(char *Subj, char *Msg, UINT Flags, HANDLE *lpLetter);
DllExport DWORD WINAPI LetterCreateW(const wuchar *Subj, const wuchar *Msg, UINT Flags, HANDLE *lpLetter);
DllExport DWORD WINAPI LetterAddAddr(HANDLE Letter, char *Addr, char *Type, BOOL CC);
DllExport DWORD WINAPI LetterAddAddrW(HANDLE Letter, const wuchar *Addr, const wuchar *Type, BOOL CC);
DllExport DWORD WINAPI LetterAddFile(HANDLE Letter, char *fName);
DllExport DWORD WINAPI LetterAddFileW(HANDLE Letter, const wuchar *fName);
DllExport DWORD WINAPI LetterAddBLOBs(HANDLE Letter, char *fName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond);
DllExport DWORD WINAPI LetterAddBLOBsW(HANDLE Letter, const wuchar *fName, const wuchar *Table, const wuchar *Attr, const wuchar *Cond);
DllExport DWORD WINAPI LetterAddBLOBr(HANDLE Letter, char *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DllExport DWORD WINAPI LetterAddBLOBrW(HANDLE Letter, const wuchar *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DllExport DWORD WINAPI LetterSend(HANDLE Letter);
DllExport DWORD WINAPI TakeMailToRemOffice();
DllExport void  WINAPI LetterCancel(HANDLE Letter);

DllExport DWORD WINAPI MailOpenInBox(HANDLE *lpMailBox);
DllExport DWORD WINAPI MailBoxLoad(HANDLE MailBox, UINT Flags);
DllExport DWORD WINAPI MailBoxGetMsg(HANDLE MailBox, DWORD MsgID);
DllExport DWORD WINAPI MailBoxGetFilInfo(HANDLE MailBox, DWORD MsgID);
DllExport DWORD WINAPI MailBoxSaveFileAs(HANDLE MailBox, DWORD MsgID, DWORD FilIdx, char *FilName, char *DstPath);
DllExport DWORD WINAPI MailBoxSaveFileDBs(HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond);
DllExport DWORD WINAPI MailBoxSaveFileDBr(HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DllExport DWORD WINAPI MailBoxDeleteMsg(HANDLE MailBox, DWORD MsgID, BOOL RecToo);
DllExport DWORD WINAPI MailGetInBoxInfo(HANDLE MailBox, char *mTblName, ttablenum *mTblNum, char *fTblName, ttablenum *fTblNum);
DllExport void  WINAPI MailCloseInBox(HANDLE MailBox);
DllExport DWORD WINAPI MailGetType();
DllExport DWORD WINAPI MailDial(char *PassWord);
DllExport DWORD WINAPI MailHangUp();

DllExport DWORD WINAPI MailCreateProfile(const char *ProfileName, BOOL Temp);
DllExport DWORD WINAPI MailDeleteProfile(const char *ProfileName);
DllExport DWORD WINAPI MailSetProfileProp(const char *ProfileName, const char *PropName, const char *PropValue);
DllExport DWORD WINAPI MailGetProfileProp(const char *ProfileName, const char *PropName, char *PropValue, int ValSize);
}
#endif

DWORD cd_InitWBMail(cdp_t cdp, const char *Profile, char *RecvPassWord, char *SendPassWord);
DWORD cd_InitWBMailEx(cdp_t cdp, const char *Profile, const char *RecvPassWord, const char *SendPassWord);
DWORD cd_InitWBMail602(cdp_t cdp, char *EmiPath, char *UserID, char *PassWord);
DWORD cd_InitWBMail602x(cdp_t cdp, char *Profile);
void  cd_CloseWBMail(cdp_t cdp);

DWORD cd_LetterCreate(cdp_t cdp, char *Subj, char *Msg, UINT Flags, HANDLE *lpLetter);
DWORD cd_LetterCreateW(cdp_t cdp, const wuchar *Subj, const wuchar *Msg, UINT Flags, HANDLE *lpLetter);
DWORD cd_LetterAddAddr(cdp_t cdp, HANDLE Letter, char *Addr, char *Type, BOOL CC);
DWORD cd_LetterAddAddrW(cdp_t cdp, HANDLE Letter, const wuchar *Addr, const wuchar *Type, BOOL CC);
DWORD cd_LetterAddFile(cdp_t cdp, HANDLE Letter, char *fName);
DWORD cd_LetterAddFileW(cdp_t cdp, HANDLE Letter, const wuchar *fName);
DWORD cd_LetterAddBLOBs(cdp_t cdp, HANDLE Letter, char *fName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond);
DWORD cd_LetterAddBLOBsW(cdp_t cdp, HANDLE Letter, const wuchar *fName, const wuchar *Table, const wuchar *Attr, const wuchar *Cond);
DWORD cd_LetterAddBLOBr(cdp_t cdp, HANDLE Letter, char *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DWORD cd_LetterAddBLOBrW(cdp_t cdp, HANDLE Letter, const wuchar *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DWORD cd_LetterSend(cdp_t cdp, HANDLE Letter);
DWORD cd_TakeMailToRemOffice(cdp_t cdp);
void  cd_LetterCancel(cdp_t cdp, HANDLE Letter);

DWORD cd_MailOpenInBox(cdp_t cdp, HANDLE *lpMailBox);
DWORD cd_MailBoxLoad(cdp_t cdp, HANDLE MailBox, UINT Flags);
DWORD cd_MailBoxGetMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD Flags);
DWORD cd_MailBoxGetFilInfo(cdp_t cdp, HANDLE MailBox, DWORD MsgID);
DWORD cd_MailBoxSaveFileAs(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, char *DstPath);
DWORD cd_MailBoxSaveFileDBs(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond);
DWORD cd_MailBoxSaveFileDBr(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DWORD cd_MailBoxDeleteMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID, BOOL RecToo);
DWORD cd_MailGetInBoxInfo(cdp_t cdp, HANDLE MailBox, char *mTblName, ttablenum *mTblNum, char *fTblName, ttablenum *fTblNum);
void  cd_MailCloseInBox(cdp_t cdp, HANDLE MailBox);
DWORD cd_MailGetType(cdp_t cdp);
DWORD cd_MailDial(cdp_t cdp, char *PassWord);
DWORD cd_MailHangUp(cdp_t cdp);

DWORD cd_MailCreateProfile(cdp_t cdp, const char *ProfileName, BOOL Temp);
DWORD cd_MailDeleteProfile(cdp_t cdp, const char *ProfileName);
DWORD cd_MailSetProfileProp(cdp_t cdp, const char *ProfileName, const char *PropName, const char *PropValue);
DWORD cd_MailGetProfileProp(cdp_t cdp, const char *ProfileName, const char *PropName, char *PropValue, int ValSize);
//
// _INBOXMSGS column numbers
//
#define ATML_ID         1
#define ATML_SUBJECT    2
#define ATML_SENDER     3
#define ATML_RECIPIENT  4
#define ATML_CREDATE    5
#define ATML_CRETIME    6
#define ATML_RCVDATE    7
#define ATML_RCVTIME    8
#define ATML_SIZE       9 
#define ATML_FILECNT    10
#define ATML_FLAGS      11
#define ATML_STAT       12
#define ATML_MSGID      13
#define ATML_BODY       14
#define ATML_ADDRESSEES 15
#define ATML_PROFILE    16
#define ATML_HEADER     17
//
// _INBOXFILES column numbers
//
#define ATMF_ID         1
#define ATMF_NAME       2
#define ATMF_SIZE       3
#define ATMF_DATE       4
#define ATMF_TIME       5
//
// Inbox load flags
//
#define MBL_BODY        1       // Load header and body into Body column
#define MBL_FILES       2       // Load attachment info into _INBOXFILES table
#define MBL_HDRONLY     4       // Load header into Header column
#define MBL_MSGONLY     8       // Load body into Body column 
//
// Incomming message status
//
#define MBS_NEW         0       // Newly incomming message
#define MBS_OLD         1       // Old message
#define MBS_DELETED     2       // Message was deleted on mail server
                        
#define MSGID_SIZE      64
                        
#define RMRQ_GETNEWLIST 0x0061  // Vyzvednout aktualni seznam a nevidene
#define RMRQ_GETLIST    0x0021  // Vyzvednout aktualni seznam
#define RMRQ_GETMSG     0x0004  // Vyzvednout vybrane
#define RMRQ_DELMSG     0x0200  // Smazat vybrane

#define P3RQ_DELMSG    0x0002
#define P3RQ_GETMSG    0x0004

class CWBLetter;
class CWBMailBox;

#ifdef SRVR
extern cdp_t GetReplCdp();
#endif

#define SOCKBUF_SIZE 8192

class CIPSocket : public sockaddr_in
{
protected:

    SOCKET m_Sock;
    int   m_Read;                   // Pocet naposledy nactenych bytu
    DWORD m_BufRest;                // Zbyvajici misto v bufferu m_BufSz - (m_BufPtr - m_Buf)
    char *m_BufPtr;                 // Ukazatel na pozici, na kterou se bude cist
    char  m_Buf[SOCKBUF_SIZE + 1];  // Buffer
#ifdef DEBUG
    int m_Err;
#endif

public:

    CIPSocket()
    {
        //memset(this, 0, sizeof(CIPSocket));
        m_Sock    = SOCKET_ERROR;
        m_BufPtr  = m_Buf;
        m_BufRest = SOCKBUF_SIZE;   
    }
   ~CIPSocket()
    {
        if (m_Sock != SOCKET_ERROR)
#ifdef WINS
            ::closesocket(m_Sock);
#else
            ::close(m_Sock);
#endif
    }
    char *GetBuf(){return(m_Buf);}
    void Close()
    {
#ifdef WINS
        ::closesocket(m_Sock);
#else
        ::close(m_Sock);
#endif
        m_Sock = SOCKET_ERROR;
    }
    int  Errno()
#ifdef WINS
    {return(WSAGetLastError());}
#else
    {return(errno);}
#endif

    BOOL IsBlocked(int Err)
#if defined(WINS)
    {return(Err == WSAEWOULDBLOCK);}
#elif defined(NETWARE)
    {return(Err == EWOULDBLOCK || Err == EINPROGRESS || Err == 1);}  // returns 1 on netware, why?
#elif defined(LINUX)
    {return(Err == EINPROGRESS);}
#else
    {return(Err == EWOULDBLOCK);}
#endif
    SOCKET GetSock(){return(m_Sock);}

    int Socket()
    {
        m_Sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_Sock != SOCKET_ERROR)
        {
            //DbgLogFile("    Socket(%d) OK", m_Sock);
            return(0);
        }
        int Err;
#ifdef DEBUG
        m_Err = 
#endif
        Err = Errno();
        //DbgLogFile("    Socket = %d", Err);
        return(Err);
    }
    int Connect();
    int SetAddress(char *Addr, unsigned short DefPort);
    int Send(char *Buf, int Sz, int *Wrtn);
    int Write(char *buf, int len);

    int Recv();

    uns32 GetIP(){return(sin_addr.s_addr);}
    uns16 GetPort(){return(sin_port);}
    operator SOCKET(){return(m_Sock);}
};

class CMailSock : public CIPSocket
{
protected:

    char *m_LinePtr;
    char *m_LineNext;
    char *m_CmdPtr;
    BOOL  m_Connected;

public:

    CMailSock()
    {
        Reset();
        m_Connected = FALSE;
    }
    void Reset()
    {
        m_BufRest  = SOCKBUF_SIZE;
        m_BufPtr   = m_Buf;
        m_LineNext = m_Buf;
       *m_LineNext = 0;
    }
    void Connected(){m_Connected = TRUE;}
    inline void  SMTP_newline(char *Src, int Sz);
    inline void  SMTP_line(char *Src, int Sz);
    inline void  SMTP_Date();
    inline void  SMTP_hostname();
    inline DWORD SMTP_cmd(char **buf);
    inline int   Recv(char **Buf, DWORD *Rd);
    inline DWORD Write();
    inline DWORD POP3_cmd(char *line, int Sz, char **Res); 
    DWORD  Readln(char **buf);
    DWORD  ReadMulti(char **buf);
    DWORD  GetRest(char**buf)
    {
        *buf   = m_LineNext;
        m_Read = 0;
        return (DWORD)(size_t)(m_BufPtr - m_LineNext);
    }

    void Close(void);
};

class CReplSock : public CIPSocket
{
protected:

    char *m_PktPtr;

public:

    int Accept(SOCKET Socket);
    DWORD Read(char **buf, int sz);
    int   GetRest();
    int   Recv(char **Buf, int *rd);
};

class CWBMailProfile
{
protected:

    cdp_t    m_cdp;
    bool     m_Temp;                // Temorary profile flag
    bool     m_SMTPUserSet;         // New profile with separate user name and password for SMTP
    DWORD    m_Type;                // Mail type
    char    *m_FilePath;            // Property Path
    char     m_Profile[64];         // Property Profile (Mail602/MAPI)
    char     m_SMTPServer[64];      // Property SMTPServer
    char     m_POP3Server[64];      // Property POP3Server
    char     m_POP3Name[64];        // Property UserName
    char     m_SMTPName[64];        // Property SMTPUserName 
    char     m_PassWord[256];       // Property PassWord (on MAPI can password be 256 chars long)
    char     m_MyAddress[64];       // Property MyAddress
    char    *m_SMTPPassWord;        // Property SMTPassword
    tobjname m_Appl;                // Property InBoxAppl
    tobjname m_InBoxTbl;            // Property InBoxMessages
    tobjname m_InBoxTbf;            // Property InBoxFiles

#ifdef WINS

    HKEY     m_hKey;

    char     m_DialConn[256];       // Property DialConn
    char     m_DialName[256];       // Property DialUserName
    char     m_DialPassWord[256];   // Property DialPassword

#else   // !WINS

    char     m_ProfSect[64 + 6];

#endif

    DWORD OpenProfile(const char *Profile);
    DWORD GetProfileString(char *Value, char *Buf, DWORD Size);
    DWORD GetFilePath();
    void  ConvertFilePath();

#ifdef WINS

    void  CloseProfile()
    {
        if (m_hKey)
        {
            RegCloseKey(m_hKey);
            m_hKey = NULL;
        }
    }

#else   // !WINS

    void  CloseProfile(){}

#endif  // WINS

#ifdef SRVR

    BOOL AmIDBAdmin(){return(m_cdp->prvs.is_conf_admin || m_cdp->in_admin_mode);}

#else

    BOOL AmIDBAdmin(){return(cd_Am_I_config_admin(m_cdp));}

#endif

public:

    CWBMailProfile(cdp_t cdp, bool TempProf)
    {
        memset(this, 0, sizeof(CWBMailProfile)); 
        m_cdp  = cdp;
        m_Temp = TempProf;
        if (m_Temp)
        {
            m_FilePath = new char;
            if (m_FilePath)
                *m_FilePath = 0;
        }
        m_SMTPPassWord = m_PassWord + 64;
    }
   ~CWBMailProfile()
    {
       CloseProfile();
    }
    void  Reset()
    {
        cdp_t cdp = m_cdp;
        if (m_FilePath)
            delete []m_FilePath;
        memset(this, 0, sizeof(CWBMailProfile));
        m_cdp  = cdp;
        m_Temp = TRUE;
        m_FilePath = new char;
        if (m_FilePath)
            *m_FilePath = 0;
    }
    BOOL  IsTemp(){return(m_Temp);}
    BOOL  IsTemp(const char *ProfileName){return(m_Temp && stricmp(m_Profile, ProfileName) == 0);}
    DWORD Create(const char *ProfileName);
    DWORD Delete(const char *ProfileName);
    DWORD GetProp(const char *ProfileName, const char *PropName, char *PropValue, int ValSize);
    DWORD SetProp(const char *ProfileName, const char *PropName, const char *PropValue);
    DWORD GetTempProp(const char *PropName, char *PropValue, int ValSize);
    DWORD SetTempProp(const char *PropName, const char *PropValue);
    char *FilePath(){return(m_FilePath);}
    static DWORD Exists(const char *ProfileName);

    friend DWORD cd_MailCreateProfile(cdp_t cdp, const char *ProfileName, BOOL Temp);
};

class CWBMailCtx : public CWBMailProfile
{
protected:

#ifdef WINS

    // MAPI
    HINSTANCE     m_hMapi;          // Handle mapi32.dll
    LPMAPISESSION m_lpSession;      // MAPI session
    LPMDB         m_lpMsgStore;     // Uloziste zasilek
    LPMAPIFOLDER  m_lpOutBox;       // Schranka pro postu k odeslani
    LPADRBOOK     m_lpAddrBook;     // Adresar
    LPSPropValue  m_lpSentMailID;   // ID slozky Odeslana posta 
    LPSTR        *m_aTypes;         // Seznam podporovanych typu adres
    ULONG         m_cTypes;         // Pocet podporovanych typu adres
#ifdef MAIL602
    // Mail602
    HINSTANCE     m_hMail602;       // Handle wm602m32.dll
    HINSTANCE     m_hM602Inet;      // Handle m602inet.dll
    BOOL          m_NoLogout;       // Priznak wm602m32.dll naloadoval nekdo jiny, na konci neodloadovavat
    BOOL          m_Apop;
    char          m_EMI[_MAX_PATH]; // Cesta k souboru s parametry Mail602
    char          m_RemBox[20];     // Identifikator zpravy se seznamem zasilek
    char          m_UserID[32];     // ID uzivatele
#endif
    // SMTP/POP3
    BOOL          m_WSA;            // Priznak sockets nastartovany
    HRASCONN      m_hDialConn;      // Handle vytoceneho spojeni

#endif  // WINS

    BOOL          m_Open;           // Priznak kontext je aktivni
    CWBLetter    *m_Letters;        // Seznam rozpracovanych zasilek k odeslani
    CWBMailBox   *m_MailBox;        // Schranka pro doslou postu
    wbcharset_t   m_charset;        // Na serveru charset serveru, na klientu charset hostitelskeho pocitace

#ifdef WINS

    DWORD DialSMTP(char *PassWord = NULL);
    DWORD HangUpSMTP();
#ifdef MAIL602
    DWORD SendSMTP602();
#endif
    HWND  CreateLogWnd();
    DWORD GetCurrentDir(char *Buf, int Size){return(GetCurrentDirectory(Size, Buf));}
    DWORD GetCurrentDirW(wuchar *Buf, int Size){return(GetCurrentDirectoryW(Size, Buf));}
    BOOL  strncmpDir(char *Dir1, char *Dir2, int Size){return(strnicmp(Dir1, Dir2, Size) == 0);}
    BOOL  strncmpDirW(const wuchar *Dir1, const wuchar *Dir2, int Size){return(_wcsnicmp(Dir1, Dir2, Size) == 0);}

#else   // !WINS

    DWORD GetCurrentDir(char *Buf, int Size){getcwd(Buf, Size); return(strlen(Buf));}
    DWORD GetCurrentDirW(wuchar *Buf, int Size)
    {
        char aBuf[MAX_PATH];
        getcwd(aBuf, sizeof(aBuf));
        return(ToUnicode(aBuf, -1, Buf, Size, wbcharset_t::get_host_charset()));
    }
    BOOL  strncmpDir(char *Dir1, char *Dir2, int Size){return(strncmp(Dir1, Dir2, Size) == 0);}
    BOOL  strncmpDirW(const wuchar *Dir1, const wuchar *Dir2, int Size)
	{return(memcmp(Dir1, Dir2, Size * sizeof(wuchar)) == 0);}

    friend class CWBMailBoxPOP3f;

#endif  // WINS

    BOOL  IsValidPath(const char *fName);
    BOOL  IsValidPathW(const wuchar *fName);

public:

    CWBMailCtx(cdp_t cdp, bool TempProf = false);
   ~CWBMailCtx();
    BOOL  IsValid(CWBLetter *Letter);
    BOOL  IsOpen(){return(m_Open);}
    
#ifdef WINS

    // MAPI
    DWORD   OpenMAPI(const char *Profile, const char *PassWord);
    void    CloseMAPI();
    HRESULT GetBox(LPMAPIFOLDER *lpBox, BOOL OutBox);
    HRESULT GetOutBox()
    {
        if (m_lpOutBox)
            return(NOERROR);
        return(GetBox(&m_lpOutBox, TRUE));
    }
    HRESULT IsService(char *Profile, char *PassWord);
    BOOL    IsMAPIAddr(char *Type);
#ifdef MAIL602
    // Mail602
    DWORD   Open602(char *EmiPath, char *UserID, const char *PassWord, const char *Profile = NULL);
    void    Close602();
    DWORD   SendRemote();
#endif    
    BOOL    Is602Addr(char *Type);

#endif

    DWORD   Open(const char *WBProfile, const char *RecvPassWord, const char *SendPassWord);
    void    Close();
    DWORD   LoadInBox(DWORD dAfter, DWORD tAfter, UINT Flags);
    DWORD   GetFilesInfo(DWORD ID);
    DWORD   SaveFileAs(DWORD MsgID, int FilIdx, LPCSTR FilName, char *DstPath);
    DWORD   DeleteMsg(DWORD MsgID, BOOL RecToo);

    friend class CWBMailBox;
    friend class CWBMailBox602;
    friend class CWBMailBoxMAPI;
    friend class CWBMailBoxPOP3;
    friend class CWBMailBoxPOP3s;
    friend class CAttStream602do;

    friend DWORD cd_InitWBMail(cdp_t cdp, const char *Profile, char *RecvPassWord, char *SendPassWord);
#ifdef WINS
    friend DWORD cd_InitWBMail602(cdp_t cdp, char *EmiPath, char *UserID, char *PassWord);
    friend DWORD cd_InitWBMail602x(cdp_t cdp, char *Profile);
    friend void  GateZobraz(char *Msg);
#endif
    friend void  cd_CloseWBMail(cdp_t cdp);
    friend DWORD cd_LetterCreate(cdp_t cdp, char *Subj, char *Msg, UINT Flags, HANDLE *lpLetter);
    friend DWORD cd_TakeMailToRemOffice(cdp_t cdp);
    friend class CWBLetter;

    friend DWORD cd_MailOpenInBox(cdp_t cdp, HANDLE *lpMailBox);
    friend DWORD cd_MailBoxLoad(cdp_t cdp, HANDLE MailBox, UINT Flags);
    friend DWORD cd_MailBoxGetMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD Flags);
    friend DWORD cd_MailBoxGetFilInfo(cdp_t cdp, HANDLE MailBox, DWORD MsgID);
    friend DWORD cd_MailBoxSaveFileAs(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, char *DstPath);
    friend DWORD cd_MailBoxSaveFileDBs(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond);
    friend DWORD cd_MailBoxSaveFileDBr(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
    friend DWORD cd_MailBoxDeleteMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID, BOOL RecToo);
    friend DWORD cd_MailGetInBoxInfo(cdp_t cdp, HANDLE MailBox, char *mTblName, ttablenum *mTblNum, char *fTblName, ttablenum *fTblNum);
    friend void  cd_MailCloseInBox(cdp_t cdp, HANDLE MailBox);
    friend DWORD cd_MailGetType(cdp_t cdp);
    friend DWORD cd_MailDial(cdp_t cdp, char *PassWord);
    friend DWORD cd_MailHangUp(cdp_t cdp);
};

#ifdef SRVR

class CWBMailSect
{
protected:

    CRITICAL_SECTION m_cSect;
    WORD             m_cRef;  

public:
    
    void Init();
    void Enter();
    void Leave();
    void Delete();
};

#define SetErr(cdp, Err)

#else

inline void SetErr(cdp_t cdp, DWORD Err)
{
    if (Err)
        client_error(cdp, Err);
}

#endif // SRVR

class CALEntry
{
protected:

    CALEntry *m_Next;
    char      m_MAPI;
    char      m_CC;
    char     *m_Type;
    char      m_Addr[1];

    friend class CWBLetter;
};

//class CFLEntry
//{
//protected:
//
//    CFLEntry *m_Next;
//    char      m_Path[1];
//
//    friend class CWBLetter;
//};

//
// Obecny stream pro praci s pripojenymi soubory
//
class CAttStream
{
public:

    virtual ~CAttStream(){};
    virtual DWORD Open(){return(MAIL_ERROR);}
    virtual DWORD Write(LPCSTR Buf, DWORD Size){return(MAIL_ERROR);}
    virtual DWORD CopyFrom(LPVOID SrcFile, int Flags){return(MAIL_ERROR);}
};
//
// Obecny stream pro praci s odesilanymi pripojenymi soubory
//
class CAttStreamo : public CAttStream
{
protected:

    CAttStreamo *m_Next;
    bool         m_Unicode;
    wbcharset_t  m_charset;
    wuchar       m_Path[MAX_PATH];
#ifdef LINUX
    char         m_aPath[MAX_PATH];
#endif

    friend class CWBLetter;

public:

    CAttStreamo(LPCVOID Path, bool Unicode)
    {
        if (Unicode)
            wuccpy(m_Path, (wuchar *)Path);
        else
            lstrcpy((char *)m_Path, (char *)Path);
        m_Unicode = Unicode;
    }
    virtual DWORD CopyTo(LPVOID Ctx){return(MAIL_ERROR);}
    virtual DWORD Read(char *Buf, t_varcol_size Size, t_varcol_size *pSize){return(MAIL_ERROR);}
    virtual void DeleteAfter()
    {
        if (m_Unicode)
#ifdef LINUX
            DeleteFile(m_aPath);
#else
            DeleteFileW(m_Path);
#endif
        else
            DeleteFile((const char *)m_Path);
    }
};
//
// Pridavne informace pro cteni a zapis pripojenych souboru do databaze
//
class CAttStreamd
{
protected:

    cdp_t    m_cdp;
    tcurstab m_Curs;
    trecnum  m_Pos;
    tattrib  m_Attr;
    uns16    m_Index;
    DWORD    m_Offset;
    BOOL     m_wLock;
    BOOL     m_rLock;

#ifdef SRVR

    cur_descr   *m_cd;
    table_descr *m_tbdf;

#endif

    DWORD Open(BOOL WriteAcc);
    DWORD Write(t_varcol_size Offset, LPCSTR Buf, t_varcol_size Size, t_varcol_size * pSize);
    DWORD Read(t_varcol_size Offset, char *Buf, t_varcol_size Size, t_varcol_size * pSize);
    DWORD GetSize(t_varcol_size * Size);
    void  Delete();

public:

    CAttStreamd(cdp_t cdp, tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index)
    {
        m_cdp    = cdp;
        m_Curs   = Curs;
        m_Pos    = Pos;
        m_Attr   = Attr;
        m_Index  = Index;
        m_Offset = 0;
        m_wLock  = FALSE;
        m_rLock  = FALSE;

#ifdef SRVR
        m_cd     = NULL;
        m_tbdf   = NULL;
#endif
    }

    DWORD OpenCursor(LPCSTR Table, LPCSTR AttrName, LPCSTR Cond);

    ~CAttStreamd();
};
//
// Stream pro ukladani pripojenych souboru na disk
//
class CAttStreamfi : public CAttStream
{
protected:

    LPCSTR m_DstPath;

public:

    CAttStreamfi(LPCSTR DstPath){m_DstPath = DstPath;}
};

#ifdef WINS

//
// Stream pro ukladani pripojenych souboru Mail602 na disk
//
class CAttStream602fi : public CAttStreamfi
{
    LPBYTE m_MsgID;

public:

    CAttStream602fi(LPCSTR DstPath, LPBYTE MsgID) : CAttStreamfi(DstPath){m_MsgID = MsgID;}

    virtual DWORD CopyFrom(LPVOID SrcFile, int Flags);
};
//
// Stream pro cteni pripojenych souboru Mail602 z disku
//
class CAttStream602fo : public CAttStreamo
{
public:

    CAttStream602fo(LPCVOID DstPath, bool Unicode) : CAttStreamo(DstPath, Unicode){m_charset = wbcharset_t::get_host_charset();}
    virtual DWORD CopyTo(LPVOID Ctx);
};
//
// Stream pro ukladani pripojenych souboru Mail602 do databaze
//
class CAttStream602di : public CAttStream, public CAttStreamd
{
    LPBYTE m_MsgID;

public:

    CAttStream602di(cdp_t cdp, tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index, LPBYTE MsgID)
    : CAttStreamd(cdp, Curs, Pos, Attr, Index)
    {
        m_MsgID = MsgID;
    }

    virtual DWORD Open(){return(CAttStreamd::Open(TRUE));}
    virtual DWORD CopyFrom(LPVOID SrcFile, int Flags);
};
//
// Stream pro cteni pripojenych souboru Mail602 z databaze
//
class CAttStream602do : public CAttStreamo, public CAttStreamd
{
public:

    CAttStream602do(CWBMailCtx *Ctx, tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index, LPCVOID FileName, bool Unicode)
    : CAttStreamo(FileName, Unicode),  CAttStreamd(Ctx->m_cdp, Curs, Pos, Attr, Index)
    {m_charset = Ctx->m_charset;}

    virtual DWORD Open(){return(CAttStreamd::Open(FALSE));}
    virtual DWORD CopyTo(LPVOID Ctx);
    virtual void  DeleteAfter(){Delete();}
};
//
// Stream pro ukladani pripojenych souboru MAPI na disk
//
class CAttStreamMAPIfi : public CAttStreamfi
{
public:

    CAttStreamMAPIfi(LPCSTR DstPath) : CAttStreamfi(DstPath){}
    virtual DWORD CopyFrom(LPVOID SrcFile, int Flags);
};
//
// Stream pro cteni pripojenych souboru MAPI z disku
//
class CAttStreamMAPIfo : public CAttStreamo
{
public:

    CAttStreamMAPIfo(LPCSTR DstPath, bool Unicode) : CAttStreamo(DstPath, Unicode){m_charset = wbcharset_t::get_host_charset();}
    virtual DWORD CopyTo(LPVOID Ctx);
};
//
// Stream pro ukladani pripojenych souboru MAPI do databaze
//
class CAttStreamMAPIdi : public CAttStream, public CAttStreamd, public ILockBytes
{
    UINT    m_cRef;

public:

    CAttStreamMAPIdi(cdp_t cdp, tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index)
    : CAttStreamd(cdp, Curs, Pos, Attr, Index)
    {
        m_cRef = 1;
    }
    virtual DWORD Open(){return(CAttStreamd::Open(TRUE));}
    virtual DWORD CopyFrom(LPVOID SrcFile, int Flags);

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(ReadAt)(ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(WriteAt)(ULARGE_INTEGER ulOffset, void const *pv, ULONG cb,  ULONG *pcbWritten);
    STDMETHOD(Flush)();
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
};
//
// Stream pro cteni pripojenych souboru MAPI z databaze
//
class CAttStreamMAPIdo : public CAttStreamo, public CAttStreamd
{
public:

    CAttStreamMAPIdo(cdp_t cdp, tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index, LPCVOID FileName, bool Unicode)
    : CAttStreamo(FileName, Unicode), CAttStreamd(cdp, Curs, Pos, Attr, Index)
    {m_charset = wbcharset_t::get_host_charset();}

    virtual DWORD Open(){return(CAttStreamd::Open(FALSE));}
    virtual DWORD CopyTo(LPVOID Ctx);
    virtual void DeleteAfter(){Delete();}
};

#endif // WINS

//
// Stream pro ukladani pripojenych souboru SMTP na disk
//
class CAttStreamSMTPfi : public CAttStreamfi
{
protected:

    FHANDLE m_hFile;

public:

    CAttStreamSMTPfi(LPCSTR DstPath) : CAttStreamfi(DstPath){m_hFile = INVALID_FHANDLE_VALUE;}

    virtual ~CAttStreamSMTPfi(){CloseFile(m_hFile);}
    virtual DWORD Open();
    virtual DWORD Write(LPCSTR Buf, DWORD Size);
};
//
// Stream pro cteni pripojenych souboru SMTP z disku
//
class CAttStreamSMTPfo : public CAttStreamo
{
protected:

    FHANDLE m_hFile;

public:

#ifdef LINUX
    CAttStreamSMTPfo(LPCVOID DstPath, bool Unicode, LPCSTR aPath) : CAttStreamo(DstPath, Unicode){m_hFile = INVALID_FHANDLE_VALUE; strcpy(m_aPath, aPath); m_charset = wbcharset_t::get_host_charset();}
#else
    CAttStreamSMTPfo(LPCVOID DstPath, bool Unicode) : CAttStreamo(DstPath, Unicode){m_hFile = INVALID_FHANDLE_VALUE;m_charset = wbcharset_t::get_host_charset();}
#endif
    virtual ~CAttStreamSMTPfo(){if (m_hFile != INVALID_FHANDLE_VALUE) CloseFile(m_hFile);}
    virtual DWORD Open();
    virtual DWORD Read(char *Buf, t_varcol_size Size, t_varcol_size *pSize);
    virtual void DeleteAfter()
    {
        if (m_hFile != INVALID_FHANDLE_VALUE)
        {
            CloseFile(m_hFile);
            m_hFile = INVALID_FHANDLE_VALUE;
        }
        CAttStreamo::DeleteAfter();
    }
};
//
// Stream pro ukladani pripojenych souboru SMTP do databaze
//
class CAttStreamSMTPdi : public CAttStream, public CAttStreamd
{
public:

    CAttStreamSMTPdi(cdp_t cdp, tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index) : 
    CAttStreamd(cdp, Curs, Pos, Attr, Index){}
    virtual DWORD Open(){return(CAttStreamd::Open(TRUE));}
    virtual DWORD Write(LPCSTR Buf, DWORD Size);
};
//
// Stream pro cteni pripojenych souboru SMTP z databaze
//
class CAttStreamSMTPdo : public CAttStreamo, public CAttStreamd
{
public:

    CAttStreamSMTPdo(cdp_t cdp, tcurstab Curs, trecnum Pos, tattrib Attr, uns16 Index, LPCVOID FileName, bool Unicode)
    : CAttStreamo(FileName, Unicode), CAttStreamd(cdp, Curs, Pos, Attr, Index){}
    //virtual DWORD Write(LPCSTR Buf, DWORD Size);
    virtual DWORD Open(){return(CAttStreamd::Open(FALSE));}
    virtual DWORD Read(char *Buf, t_varcol_size Size, t_varcol_size *pSize){return(CAttStreamd::Read(m_Offset, Buf, Size, pSize));}
    virtual void DeleteAfter(){Delete();}
};

class CWBLetter
{
protected:

    LPSTR        m_Subj;            // Subject
    LPSTR        m_Msg;             // Message body
    UINT         m_Flags;           // Message flags WBL_...
    CALEntry    *m_AddrList;        // Addressees list
    CAttStreamo *m_FileList;        // Attachment list
    CWBLetter   *m_Next;            // Link in processed messages list
    CMailSock    m_Sock;            // SMTP socket
    CWBMailCtx  *m_MailCtx;         // Owning mail context
    bool         m_HTML;            // HTML letter, supported only for SMTP

    inline DWORD SMTP_init();
    inline DWORD SMTP_helo();
    inline DWORD SMTP_auth();
    inline DWORD SMTP_from();
    inline DWORD SMTP_to(char *to);
    inline DWORD SMTP_data();
    inline void  Base64(char *Dst, DWORD Val);
    inline void  Base64(char *Dst, char *Src);
    DWORD  SMTP_file(CAttStreamo *fle);
#ifdef MAIL602
    BOOL   FindIn602List(char *pName, TMail602Address *Address);
#endif
    bool IsHTML(const char *Src, char *CharSet);
    void GetCharSet(const char *Src, char *CharSet);

public:

    CWBLetter(CWBMailCtx *MailCtx);
   ~CWBLetter();

    DWORD Create(char *Subj, char *Msg, UINT Flags);
    DWORD AddAddr(char *Addr, char *Type, BOOL CC);
    DWORD AddFile(const char *fName, bool Unicode);
    DWORD AddBLOBs(LPCSTR Table, LPCSTR Attr, LPCSTR Cond, const void *Path, bool Unicode);
    DWORD AddBLOBr(tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index, const void *Path, bool Unicode);
    DWORD Send();
    DWORD SendSMTP();
#ifdef WINS
    DWORD SendMAPI();
    DWORD Send602();
#endif
    void  ToAscii(char *Src);
    char *ToHdrText(char *Src, char *Dst, int Size);

    friend class CWBMailCtx;
};

void  Mix(char *Dst);
void  DeMix(char *Dst, char *Src);
char *BufAlloc(uns32 Size, uns32 * pSize);
void TempFileName(char *Buf);

#ifdef WINS

void  ctopas(char *cStr, char *pStr);
DWORD hrErrToszErr(HRESULT hrErr);
DWORD Err602ToszErr(DWORD Err);
DWORD RasLoad(HINSTANCE *lpinst);

THREAD_PROC(GateThread);

#endif

#define ADDR_CC ('C' + ('C' << 8) + (':' << 16) + (' ' << 24))
#define ADDR_TO ('T' + ('O' << 8) + (':' << 16) + (' ' << 24))

class CWBMailBox
{
protected:

    DWORD       m_ID;                               // ID in _INBOXMSGS of open message
    trecnum     m_CurPos;                           // Record number in _INBOXMSGS of open message
    BYTE        m_MsgID[MSGID_SIZE];                // Message ID in message header               
    cdp_t       m_cdp;                              // cdp
    char        m_InBoxTbl[2 * OBJ_NAME_LEN + 2];   // Name of _INBOXMSGS
    char        m_InBoxTbf[2 * OBJ_NAME_LEN + 2];   // Name of _INBOXFILES
    ttablenum   m_InBoxL;                           // Table number of _INBOXMSGS
    ttablenum   m_InBoxF;                           // Table number of _INBOXFILES
    uns32       m_BodySize;                         // Size of message body
    WORD        m_FileCnt;                          // Attachment count
    CWBMailCtx *m_MailCtx;                          // Owning CWBMailCtx
    char       *m_Addressees;                       // Ukazatel na seznam ostatnich adresatu
    char       *m_Addressp;                         // Ukazatel v seznamu ostatnich adresatu
    DWORD       m_Addressz;                         // Velikost seznamu ostatnich adresatu
    DWORD       m_Addresst;                         // Stav zapisu do seznamu adresatu
    CAttStream *m_AttStream;                        // Open attachment stream

    DWORD MsgID2Pos();
    BOOL  ReallocAddrs(UINT Size);
    DWORD Base64Line(char *Src, char *Dst, char **Next);
    DWORD Base64(DWORD c);
    DWORD Convert(char *Src, char *Dst, DWORD Size, int SrcLang);
    int   GetSrcLang(char **Val);

#ifndef SRVR

    BOOL  ReadIndL(tattrib Attr, void  *pData){return(cd_Read_ind(m_cdp, m_InBoxL, m_CurPos, Attr, NOINDEX, pData));}
    BOOL  WriteIndL(tattrib Attr, void  *pData, WORD Size){return(cd_Write_ind(m_cdp, m_InBoxL, m_CurPos, Attr, NOINDEX, pData, Size));}
    BOOL  WriteVarL(tattrib Attr, const void  *pData, DWORD Offset, DWORD Size){return(cd_Write_var(m_cdp, m_InBoxL, m_CurPos, Attr, NOINDEX, Offset, Size, pData));}
    BOOL  WriteLenL(tattrib Attr, DWORD Len){return(cd_Write_len(m_cdp, m_InBoxL, m_CurPos, Attr, NOINDEX, Len));}
    BOOL  WriteVarL(tattrib Attr, void  *pData, DWORD Size)
    {
        if (cd_Write_var(m_cdp, m_InBoxL, m_CurPos, Attr, NOINDEX, 0, Size, pData))
            return(TRUE);
        return(cd_Write_len(m_cdp, m_InBoxL, m_CurPos, Attr, NOINDEX, Size));
    }
    BOOL  ReadIndF(trecnum Pos, tattrib Attr, void  *pData){return(cd_Read_ind(m_cdp, m_InBoxF, Pos, Attr, NOINDEX, pData));}
    BOOL  WriteIndF(trecnum Pos, tattrib Attr, void  *pData, WORD Size){return(cd_Write_ind(m_cdp, m_InBoxF, Pos, Attr, NOINDEX, pData, Size));}
    BOOL  ReadLenC(tcursnum Curs, tattrib Attr, t_varcol_size *pLen){return(cd_Read_len(m_cdp, Curs, 0, Attr, NOINDEX, pLen));}
    BOOL  ReadIndC(tcursnum Curs, tattrib Attr, void  *Buf){return(cd_Read_ind(m_cdp, Curs, 0, Attr, NOINDEX, Buf));}

    BOOL  SQLExecute(char *Cmd){uns32 Res; return(cd_SQL_execute(m_cdp, Cmd, &Res));}
    BOOL  CloseCursor(tcursnum Curs){return(cd_Close_cursor(m_cdp, Curs));}
    BOOL  StartTransaction(){return(cd_Start_transaction(m_cdp));}
    BOOL  Commit(){return(cd_Commit(m_cdp));}
    void  RollBack(){cd_Roll_back(m_cdp);}
    BOOL  FindMsg(char *Cmd, tcursnum *Curs)
    {
        *Curs = NOCURSOR; 
        if (cd_Open_cursor_direct(m_cdp, Cmd, Curs))
            return(FALSE);
        trecnum rCnt;
        BOOL Err = cd_Rec_cnt(m_cdp, *Curs, &rCnt);
        if (Err || rCnt == 0)
        {
            cd_Close_cursor(m_cdp, *Curs);
            *Curs = NOCURSOR;
        }
        return(!Err);
    }
    BOOL  DeleteRec(){return(cd_Delete(m_cdp, m_InBoxL, m_CurPos));}
    trecnum Translate(tcursnum Curs){trecnum recnum = NORECNUM; cd_Translate(m_cdp, Curs, 0, 0, &recnum); return(recnum);}
    trecnum InsertL(){return(cd_Insert(m_cdp, m_InBoxL));}
    trecnum InsertF(){return(cd_Insert(m_cdp, m_InBoxF));}
    DWORD   WBError(){return(cd_Sz_error(m_cdp));}
    void    FreeDeleted()
    {
        tcursnum Curs;
        trecnum  rCnt;
        if (!cd_Open_cursor_direct(m_cdp, "SELECT * FROM _IV_LOCKS WHERE TABLE_NAME=\"TABTAB\" AND OBJECT_NAME IN (\"_INBOXMSGS\", \"_INBOXFILES\")", &Curs))
        {
            if (!cd_Rec_cnt(m_cdp, Curs, &rCnt) && rCnt == 0)
            {
                cd_Free_deleted(m_cdp, m_InBoxL);
                cd_Free_deleted(m_cdp, m_InBoxF);
            }
            cd_Close_cursor(m_cdp, Curs);
        }
    }

#else   // !SRVR

    table_descr *m_InBoxLDescr;
    table_descr *m_InBoxFDescr;

    trecnum Translate(tcursnum Curs)
    {
        trecnum recnum;
        cur_descr *cd = *crs.acc0(GET_CURSOR_NUM(Curs));
        cd->curs_seek(0, &recnum);
        return(recnum);
    }
    trecnum InsertL(){return(tb_new(m_cdp, tables[m_InBoxL], FALSE, NULL));} 
    trecnum InsertF(){return(tb_new(m_cdp, tables[m_InBoxF], FALSE, NULL));} 
    BOOL  ReadIndL(tattrib Attr, void  *pData){return(tb_read_atr(m_cdp, m_InBoxL, m_CurPos, Attr, (char *)pData));}
    BOOL  WriteIndL(tattrib Attr, void  *pData, WORD Size){return(tb_write_atr(m_cdp, tables[m_InBoxL], m_CurPos, Attr, pData));}
    BOOL  WriteVarL(tattrib Attr, const void  *pData, DWORD Offset, DWORD Size){return(tb_write_var(m_cdp, tables[m_InBoxL], m_CurPos, Attr, Offset, Size, pData));}
    BOOL  WriteLenL(tattrib Attr, DWORD Len){return(tb_write_atr_len(m_cdp, tables[m_InBoxL], m_CurPos, Attr, Len));}
    BOOL  WriteVarL(tattrib Attr, const void  *pData, DWORD Size)
    {
        if (tb_write_var(m_cdp, tables[m_InBoxL], m_CurPos, Attr, 0, Size, pData))
            return(TRUE);
        return(tb_write_atr_len(m_cdp, tables[m_InBoxL], m_CurPos, Attr, Size));
    }
    BOOL  ReadIndF(trecnum Pos, tattrib Attr, void  *pData){return(tb_read_atr(m_cdp, m_InBoxF, Pos, Attr, (char *)pData));}
    BOOL  WriteIndF(trecnum Pos, tattrib Attr, void  *pData, WORD Size){return(tb_write_atr(m_cdp, tables[m_InBoxF], Pos, Attr, pData));}
    BOOL  ReadLenC(tcursnum Curs, tattrib Attr, t_varcol_size *pLen){return(tb_read_atr_len(m_cdp, tables[m_InBoxL], Translate(Curs), Attr, pLen));}
    BOOL  ReadIndC(tcursnum Curs, tattrib Attr, void  *Buf){return(tb_read_atr(m_cdp, tables[m_InBoxL], Translate(Curs), Attr, (char *)Buf));}

    BOOL  SQLExecute(char *Cmd){return(sql_exec_direct(m_cdp, Cmd));}
    BOOL  CloseCursor(tcursnum Curs){free_cursor(m_cdp, Curs); return(FALSE);}
    BOOL  StartTransaction()
    {
        if (m_cdp->expl_trans==TRANS_NO)
        { 
            commit(m_cdp);
            m_cdp->expl_trans = TRANS_EXPL;
            m_cdp->trans_rw   = TRUE;
        }
        return(FALSE);
    }
    BOOL  Commit(){return(commit(m_cdp));}
    void  RollBack(){roll_back(m_cdp);}
    BOOL  FindMsg(char *Cmd, tcursnum *Curs)
    {
        cur_descr *cd;
        *Curs = open_working_cursor(m_cdp, Cmd, &cd);
        if (*Curs == NOCURSOR)
            return(FALSE);
        if (cd->recnum == 0)
        {
            free_cursor(m_cdp, *Curs);
            *Curs = NOCURSOR;
        }
        return(TRUE);
    }
    BOOL DeleteRec()
    {
        BOOL Err = tb_del(m_cdp, tables[m_InBoxL], m_CurPos);
        if (!Err)
            Err = commit(m_cdp);
        return(Err);
    }
    DWORD WBError(){return(m_cdp->get_return_code());}
    void  FreeDeleted()
    {
        uns16 of = m_cdp->ker_flags;
        m_cdp->ker_flags |= KFL_HIDE_ERROR;
        free_deleted(m_cdp, m_InBoxLDescr);
        free_deleted(m_cdp, m_InBoxFDescr);
        m_cdp->ker_flags = of;
    }       

#endif // SRVR

public:

    CWBMailBox(CWBMailCtx *MailCtx);
    virtual ~CWBMailBox();

    DWORD OpenInBox(cdp_t cdp);
    DWORD LoadList(UINT Param);
    DWORD GetMsg(DWORD ID, DWORD Flags);
    DWORD GetFilesInfo(DWORD ID);
    DWORD SaveFileAs(DWORD ID, int FilIdx, LPCSTR FilName, char *DstPath);
    DWORD SaveFileToDBs(DWORD MsgID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR AttrName, LPCSTR Cond);
    DWORD SaveFileToDBr(DWORD MsgID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
    DWORD DeleteMsg(DWORD ID, BOOL RecToo);
    LPCSTR GetInBoxMName(){return(m_InBoxTbl);}
    LPCSTR GetInBoxFName(){return(m_InBoxTbf);}
    ttablenum GetInBoxM(){return(m_InBoxL);}
    ttablenum GetInBoxF(){return(m_InBoxF);}
    void GetInfo(char *mTblName, ttablenum *mTblNum, char *fTblName, ttablenum *fTblNum)
    {
//#ifdef SRVR
//        m_cdp->set_return_code(ANS_OK);
//#endif
        if (mTblName)
            strcpy(mTblName, m_InBoxTbl);
        if (mTblNum)
            *mTblNum = m_InBoxL;
        if (fTblName)
            strcpy(fTblName, m_InBoxTbf);
        if (fTblNum)
            *fTblNum = m_InBoxF;
    }

#ifndef SRVR
    static DWORD GetTables(cdp_t cdp, char *Appl, char *InBoxTbl, ttablenum *InBoxL, char *InBoxTbf, ttablenum *InBoxF);
#else
    static DWORD GetTables(cdp_t cdp, char *Appl, char *InBoxTbl, ttablenum *InBoxL, table_descr **InBoxLDescr, char *InBoxTbf, ttablenum *InBoxF, table_descr **InBoxFDescr);
#endif
    virtual DWORD Open() = 0;
    virtual DWORD Load(UINT Param) = 0;
    virtual DWORD GetMsg(DWORD Flags) = 0;
    virtual DWORD GetFilesInfo() = 0;
    virtual DWORD SaveFile(int FilIdx, LPCSTR FilName) = 0;
    virtual DWORD DeleteMsg() = 0;
    virtual DWORD CreateDBStream(tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index) = 0;
    virtual DWORD CreateFileStream(LPCSTR DstPath) = 0;

    virtual DWORD GetMsgID(uns32 ID);
};

typedef CWBMailBox *LPWBMAILBOX;

#ifdef WINS
#ifdef MAIL602
//
// Mail602
//
#define poConfirm          0x0002
#define poViewed           0x0008
#define poFAXMessage       0x0010
#define poCommandForG602   0x1000

#define expoMistni         0x0010
#define expoNaSiti         0x0020
#define expoZazadanVymaz   0x0080
#define expoNizkaPriorita  0x4000
#define expoVysokaPriorita 0x8000

class CWBMailBox602 : public CWBMailBox
{
protected:

    TPOP3MInfo      *m_OldList;     // List of messages allready loaded from mother post office
    DWORD            m_OldCount;    // Count of messages in the list
    PMessageInfoList m_RemMsgList;  // List of remote messages
    PMessageFileList m_FileList;    // Attachment list
    char             m_MsgType;     // Message type Local/Internet/Fax

    DWORD LocMsgID2Pos(char *Subj, char *Sender, DWORD Date, DWORD Time);
    DWORD LoadRemMsg();
    DWORD GetMsgLoc(DWORD Flags);
    DWORD GetFilesInfoLoc();
    DWORD LoadPOP3(UINT Param);
    DWORD GetMsgPOP3();
    void  SetOldList(TPOP3MInfo *NewList, UINT FullCount);
    DWORD RequestPOP3();
    DWORD LoadRem(UINT Param);
    DWORD RequestRem(WORD Req, PMessageInfoList MsgList, BOOL Send = TRUE);
    DWORD GetMsgInfoPOP3(TPOP3MInfo **MsgInfo);
    DWORD GetMsgInfoRem(PMessageInfoList MsgList);
    DWORD GetAddressees(PMessageInfo Msgi);

public:

    CWBMailBox602(CWBMailCtx *MailCtx);
    virtual ~CWBMailBox602();

    virtual DWORD Open();
    virtual DWORD Load(UINT Param);
    virtual DWORD GetMsg(DWORD Flags);
    virtual DWORD GetFilesInfo();
    virtual DWORD SaveFile(int FilIdx, LPCSTR FilName);
    virtual DWORD DeleteMsg();
    virtual DWORD CreateDBStream(tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
    virtual DWORD CreateFileStream(LPCSTR DstPath);

    virtual DWORD GetMsgID(uns32 ID);
};

#endif // MAIL602

//DWORD WB2DDTM(DWORD WBDate, DWORD WBTime);
void  DDTM2WB(DWORD DDTM, LPDWORD WBDate, LPDWORD WBTime);

//
// MAPI
//
#define PRID_ENTRYID                0
#define PRID_SUBJECT                1
#define PRID_SENDER_NAME            2
#define PRID_SENDER_EMAIL_ADDRESS   3   // Neni u faxu a zprav na vyvesce
#define PRID_DISPLAY_TO             4   // Neni u faxu a zprav na vyvesce
#define PRID_CREATION_TIME          5   
#define PRID_MESSAGE_DELIVERY_TIME  6   // Neni u nekterych zasilek
#define PRID_MESSAGE_FLAGS          7
#define PRID_PRIORITY               8   // Neni u faxu a zprav na vyvesce
#define PRID_SENSITIVITY            9   // Neni u faxu a zprav na vyvesce
#define PRID_READ_RECEIPT          10   // Neni u faxu a zprav na vyvesce
#define PRID_MESSAGE_SIZE          11   // To je asi velikost zpravy i s attachmenty
#define PRID_BODY                  12
#define PRID_HASATTACH             13
#define PRID_MSIZE                 14

#define PRID_RECIPIENT_TYPE         0
#define PRID_DISPLAY_NAME           1
#define PRID_EMAIL_ADDRESS          2
#define PRID_RSIZE                  3

#define PRID_ATTACH_NUM             0           
#define PRID_ATTACH_LONG_FILENAME   1   // Neni u faxu a zprav na vyvesce
#define PRID_ATTACH_LONG_PATHNAME   2   // Neni u faxu a zprav na vyvesce
#define PRID_ATTACH_SIZE            3   // Tam byvaj taky nesmysly
#define PRID_LAST_MODIFICATION_TIME 4   // Neni u faxu a zprav na vyvesce
#define PRID_ATTACH_METHOD          5
#define PRID_ATTACH_FILENAME        6   // Neni u faxu a zprav na vyvesce
#define PRID_ATTACH_PATHNAME        7   // Neni u faxu a zprav na vyvesce
#define PRID_ATTACH_TRANSPORT_NAME  8
#define PRID_FSIZE                  9

class CWBMailBoxMAPI : public CWBMailBox
{
protected:

    LPMAPIFOLDER m_lpInBox;             // InBox interface
    LPSRowSet    m_lpFilesSet;          // Attachmet rowset
    LPMESSAGE    m_lpMessage;           // Open message interface
    int          m_SrcLang;             // Hosts charset

    void  ReleaseLastMsg();
    DWORD GetFilesSet(DWORD *cFils);
    DWORD GetAttachStream(DWORD Idx, DWORD Meth, LPSTREAM *lpFileStr);
    HRESULT WriteAddressees();

public:

    CWBMailBoxMAPI(CWBMailCtx *MailCtx);
    virtual ~CWBMailBoxMAPI();

    virtual DWORD Open();
    virtual DWORD Load(UINT Param);
    virtual DWORD GetMsg(DWORD Flags);
    virtual DWORD GetFilesInfo();
    virtual DWORD SaveFile(int FilIdx, LPCSTR FilName);
    virtual DWORD DeleteMsg();
    virtual DWORD CreateDBStream(tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
    virtual DWORD CreateFileStream(LPCSTR DstPath);

    virtual DWORD GetMsgID(uns32 ID);
};  

//LONGLONG WB2FDTM(DWORD WBDate, DWORD WBTime);
void     FDTM2WB(LONGLONG FDTM, LPDWORD WBDate, LPDWORD WBTime);

#endif // WINS    
//
// POP3
//
#define P3CF_SKIPHDR 0x01           // Skip message header in single part message
#define P3CF_UNDRSCR 0x02           // Convert '_' to ' '
#define P3CF_QUOTED  0x04           // Message is by quoted-printable method encoded
#define P3CF_Q       0x06           // Message header entry is =?Q encoded = P3CF_UNDRSCR + P3CF_QUOTED
#define P3CF_BASE64  0x08           // Message or header entry is base64 encoded
//#define P3CF_WIN1250 0x10
//#define P3CF_ISO8859 0x20
#define P3CF_DIRECT  0x40           // Direct incomming messages file access used

#define ADDRS_SIGN    0x00000001
#define ADDRS_NAME    0x00000002
#define ADDRS_ADDR    0x00000004

#define ADDRS_TO      0x20000000
#define ADDRS_CC      0x40000000
#define ADDRS_FIRST   0x80000000

#define MAX_MSGSIZE   0xFFFFF

struct CPOP3MsgInfo
{
    char m_MsgID[MSGID_SIZE];           // Message ID
    char m_Bound[68];                   // Message parts boundery
#ifdef UNIX
    DWORD m_Offset;                     // Offset of message in incomming messages file
#endif
};

class CWBMailBoxPOP3 : public CWBMailBox
{
protected:

    UINT          m_cMsgs;              // Message count on POP3 server
    UINT          m_Msg;                // Loaded message number
    CPOP3MsgInfo *m_IDTbl;              // Message info table
    FHANDLE       m_hFile;              // Loaded message file handle
    char         *m_MsgBuf;             // Buffer for message max 1 MB
    uns32         m_MsgSize;            // Velikost bufferu
    DWORD         m_bLen;               // Delka boundery
    WORD          m_StartNO;            // Cislo souboru na zacatku bufferu
    WORD          m_FileNO;             // Cislo aktualniho souboru, 0 = Hdr, 1 = dopis, 2 = 1. soubor
    char         *m_StartOfStart;       // Ukazatel na zacatek casti na zacatku bufferu (NULL, kdyz uz v bufferu neni)
    char         *m_StartOfPart;        // Ukazatel na zacatek aktualni casti (NULL, kdyz uz v bufferu neni)
    char         *m_MsgPtr;             // Ukazatel v aktualni casti
    char         *m_EndOfPart;          // Ukazatel na konec aktualni casti (Ukazuje na boundery nebo na posledni radek bufferu)
    char         *m_NextLine;           // Ukazatel na zacatek posledniho radku v bufferu
    char         *m_HdrBuf;             // Ukazatel na zacatek hlavicky
    char         *m_HdrPtr;             // Ukazatel na konec hlavicky
    BOOL          m_Eof;                // End of message read
    BOOL          m_PartOpen;           // Requested message part is in buffer

    DWORD NextLine(char *Ptr, char **Line);
    DWORD GetFileInfo(int FileNO, UINT *Flags, char *FileName, char **Line);
    DWORD GetNextPart(int Part, LPSTR *pPtr, DWORD *pSize);
    DWORD ReadBuf(BOOL Start);
    BOOL  GetValC(char *Ptr, char *Var, int Len, char **Val);
    BOOL  GetValE(char *Ptr, char *Var, int Len, char **Val);
    void  ValCpy(char *Src, char *Dst, int Len);
    char *ConvertSubj(char *Src, char *Dst, DWORD Size);
    char *ConvertAddr(char *Src, char *Dst, DWORD Size);
    char  GetNameTopic(char *pSrc, char **Next);
    char  GetAddrTopic(char *pSrc, char **Next);
    UINT  GetConvertFlag(char *Line, int *SrcLang);
    char *Convert(char **Src, char *Dst, DWORD Size);
    DWORD Convert(char *Ptr, DWORD Size, UINT Flags);

    virtual DWORD Init()              = 0;
    virtual DWORD GetHdr(bool Save)   = 0;
    virtual DWORD GetBufSize()        = 0;
    virtual DWORD Readln(char **Line) = 0;
    virtual DWORD LoadMsg()           = 0;
    virtual BOOL  SeekToTop()         = 0;
    virtual void  ReleaseLastMsg()    = 0;

public:

    CWBMailBoxPOP3(CWBMailCtx *MailCtx);

    //virtual DWORD Open();
    virtual DWORD Load(UINT Param);
    virtual DWORD GetMsg(DWORD Flags);
    virtual DWORD GetFilesInfo();
    virtual DWORD SaveFile(int FilIdx, LPCSTR FilName);
    virtual DWORD GetMsgID(uns32 ID);
    virtual DWORD CreateDBStream(tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
    virtual DWORD CreateFileStream(LPCSTR DstPath);
};  

void     P3DTM2WB(char *dt, LPDWORD WBDate, LPDWORD WBTime);

class CWBMailBoxPOP3s : public CWBMailBoxPOP3
{
protected:

    CMailSock     m_Sock;               // Socket for POP3 dialog
    char          m_FileName[MAX_PATH]; // Message temp file name
    int           m_HdrSize;            // Message Header size

    virtual DWORD Init();
    virtual DWORD GetHdr(bool Save);
    virtual DWORD GetBufSize();
    virtual DWORD Readln(char **Line);
    virtual DWORD LoadMsg();
    virtual BOOL  SeekToTop();
    virtual void  ReleaseLastMsg();

public:

    CWBMailBoxPOP3s(CWBMailCtx *MailCtx) : CWBMailBoxPOP3(MailCtx)
    {
        *m_FileName = 0;
         m_HdrBuf   = NULL;
    }

    virtual ~CWBMailBoxPOP3s();
    virtual DWORD Open();
    virtual DWORD DeleteMsg();
};

#ifdef UNIX

class CWBMailBoxPOP3f : public CWBMailBoxPOP3
{
private:
    BOOL AllocMsgInfo(int size); /* TRUE if OK */
protected:

    UINT     m_IDTblSz;             // m_IDTbl size

    virtual DWORD Init();
    virtual DWORD GetHdr(bool Save);
    virtual DWORD GetBufSize();
    virtual DWORD Readln(char **Line);
    virtual DWORD LoadMsg();
    virtual BOOL  SeekToTop();
    virtual void  ReleaseLastMsg();

public:

    CWBMailBoxPOP3f(CWBMailCtx *MailCtx) : CWBMailBoxPOP3(MailCtx){m_IDTblSz = 0;}

    virtual ~CWBMailBoxPOP3f();
    virtual DWORD Open();
    virtual DWORD DeleteMsg();
};

#endif

class CBuf
{
protected:

    char *m_Buf;

public:

    CBuf(){m_Buf = NULL;}
   ~CBuf(){if (m_Buf) corefree(m_Buf);}
    bool Alloc(int Size){m_Buf = (char *)corealloc(Size, 192); return(m_Buf != NULL);}
    operator char *(){return(m_Buf);}
    operator void *(){return(m_Buf);}
    operator const wuchar *(){return((const wuchar *)m_Buf);}
    operator wuchar *(){return((wuchar *)m_Buf);}
};

#ifdef WINS
#ifdef MAIL602
//
// Kontext spojeni Remote nebo SMTP/POP3 Mail602 klienta s materskym uradem
//
class CRemoteCtx
{
protected:

    HINSTANCE m_hGate602;       // Handle G602RM32.DLL
    HANDLE    m_hGateThread;    // Handle vlakna, ve kterem bezi FaxMailServer
    HANDLE    m_GateON;         // Udalost - shodi se pri zacatku komunikace, nahodi se po zaveseni nebo pri chybe
    HANDLE    m_GateRec;        // Udalost - shodi se pri zacatku komunikace, nahodi kdyz prijde odpoved z materskeho uradu
                                // Neprehazovat posloupnost m_GateON, m_GateRec, vyuziva se ve WaitForMultipleObjects
    WORD      m_cRef;           // Citac referenci, Dial nahazuje, HangUp shazuje, posledni HangUp shodi vlakno FaxMailServeru
    char      m_Emi[_MAX_PATH]; // Cesta k souboru s parametry Mail602

public:

    CRemoteCtx()
    {
        m_hGate602    = NULL;
        m_hGateThread = NULL;
        m_Emi[0]      = 0;
        m_cRef        = 0;
    }
    DWORD Dial(char *Emi, HINSTANCE hMail602, UINT Timeout);
    void  HangUp();
    friend THREAD_PROC(GateThread);
    friend class CWBMailCtx;
    friend class CWBMailBox602;
    friend void GateZobraz(char *Msg);
};
#endif
//
// RAS API
//
typedef DWORD (APIENTRY *LPRASENUMCONNECTIONS)(LPRASCONNA, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *LPRASDIAL)(LPRASDIALEXTENSIONS, LPCSTR, LPRASDIALPARAMSA, DWORD, LPVOID, LPHRASCONN);
typedef DWORD (APIENTRY *LPRASHANGUP)(HRASCONN);
typedef DWORD (APIENTRY *LPRASGETCONNECTSTATUS)(HRASCONN, LPRASCONNSTATUS);
//
// Kontext DialUp spojeni pro SMTP/POP3 a DirectIP
//
class CDialCtx
{
protected:

    HINSTANCE m_hInst;      // Handle rasapi32.dll
    HRASCONN  m_hDialConn;  // Handle vytoceneho spojeni
    WORD      m_cRef;       // Citac referenci, Dial nahazuje, HangUp shazuje, prvni Dial opravdu vytaci, posledni HangUp opravdu zavesi

public:

    DWORD RasLoad();
    DWORD Dial(char *DialConn, char *User, char *PassWord, LPHRASCONN lpConn);
    DWORD HangUp(HRASCONN hDialConn);
    void  RasFree()
    {
        if (m_hInst && !m_cRef)
        {
            FreeLibrary(m_hInst);
            m_hInst = NULL;
        }
    }
    friend class CWBMailCtx;
};

#if 0 //#ifndef SRVR

BOOL THREAD_START_JOINABLE(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle)
{ unsigned id;  THREAD_HANDLE hnd;
  hnd=(THREAD_HANDLE)_beginthreadex(NULL, stack_size, name, param, 0, &id); // returns 0 on error
  if (handle) *handle=hnd;
  return hnd!=0;
}

BOOL THREAD_START(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle)
{ unsigned id;  THREAD_HANDLE hnd;
  hnd=(THREAD_HANDLE)_beginthreadex(NULL, stack_size, name, param, 0, &id); // returns 0 on error
  if (handle) *handle=hnd;
  if (hnd!=0) 
  { CloseHandle(hnd);
    return TRUE;
  }
  return FALSE;
}


#endif  // SRVR

#define IsBadRptr(ptr, size) (IsBadReadPtr(ptr, size))
#define IsBadWptr(ptr, size) (IsBadWritePtr(ptr, size))
#else   // UNIX
#define IsBadRptr(ptr, size) (ptr == NULL)
#define IsBadWptr(ptr, size) (ptr == NULL)
#endif  // WINS


