 // Replica.cpp - Replication threads manager

// t_replctx.m_KeySize x t_tablerepl_info.keysize
//
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#pragma hdrstop
#include "replic.h"
#include "nlmtexts.h"  // includes libintl.h in the proper way, defines gettext_noop()
#ifndef UNIX
#include <process.h>
#else
#include <unistd.h>
#define lstrcat strcat
#define lstrcpy strcpy
#define lstrcmp strcmp
#define lstrcmpi strcasecmp
#endif
#include "dbgprint.h"
#ifndef WINS
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>  // struct timeval for select
#include <sys/errno.h> // EWOULDBLOCK
#include <netinet/in.h>
#define closesocket(sock) close(sock)
#include <sys/ioctl.h>  // ioctl, FIONBIO
#include <stddef.h>
#include <netdb.h>
#ifdef UNIX
#include <stdlib.h>
#else
#include <direct.h>
#include <io.h>
#endif
#endif
#include "wbmail.h"
#include "enumsrv.h"

#ifdef WINS
#define SockErrno WSAGetLastError()
#define Errno     GetLastError()
#else
#define SockErrno errno
#define Errno     errno
#endif
//
// Atributy v _PACKETTAB
//
#define PT_ID       1
#define PT_ERASE    2
#define PT_PKTTYPE  3
#define PT_PKTNO    4
#define PT_DESTOBJ  5 
#define PT_DESTNAME 6
#define PT_ADDRESS  7
#define PT_FNAME    8

#define DIR_INPCK      0
#define DIR_OUTPCK     1
#define DIR_MAIL       2
#define DIR_DIRECTIP   3
#define DIR_ERROR      4

#define ERROR_RECOVERABLE 0x80000000        // Chyba jako napr. malo mista na disku, malo pameti 
                                            // v pristim kole se operace nejspis povede
#define WRN_REPLCNF_LOCAL  (short)0xFFFF    // Replikacni konflikt, prednost ma mistni hodnota
#define WRN_REPLCNF_INCOMM (short)0xFFFE    // Replikacni konflikt, prednost ma dosla hodnota

#ifndef ENGLISH
#include "replica_cs.h"
#else
#define RE0  gettext_noop("Reading packet from ")
#define RE1  gettext_noop("Creating packet for ")
#define RE2  gettext_noop(" type ")
#define RE3  gettext_noop("Replication")
#define RE4  gettext_noop("Replication request")
#define RE5  gettext_noop("Replication acknowledgement")
#define RE6  gettext_noop("State report request")
#define RE7  gettext_noop("State report")
#define RE8  gettext_noop("Server info request")
#define RE9  gettext_noop("Server info")
#define RE10 gettext_noop("Sharing request")
#define RE11 gettext_noop("Sharing acceptation")
#define RE12 gettext_noop("Sharing rejection")
#define RE13 gettext_noop("Sharing end")
#define RE14 gettext_noop("Unblocking replication")
#define RE15 gettext_noop(" number ")
#define RE16 gettext_noop("Sending file %s to address %d.%d.%d.%d:%d")
#define RE17 gettext_noop("IP connect failed (%d)")
#define RE18 gettext_noop("File access error (%d)")
#define RE19 gettext_noop("Error copying packet to dest server")
#define RE20 gettext_noop("Error copying packet received from IP")
#define RE21 gettext_noop("Replication conflict: local value prevails")
#define RE22 gettext_noop("Replication conflict: incoming value prevails")
#define RE23 gettext_noop("Error creating packet")
#define RE24 gettext_noop("...no changes to be replicated")
#define RE25 gettext_noop("...packet created")
#define RE26 gettext_noop("Dialing connection")
//#define RE27 
#define RE28 gettext_noop("Replication started.")
#define RE29 gettext_noop("Replication stopped.")
#define RE30 gettext_noop("Re-mailing the lost packet")
#define RE31 gettext_noop("...sharing request for an unknown application.")
#define RE32 gettext_noop("...cannot share, incompatible settings.")
#define RE33 gettext_noop("...cannot share, token manager not specified.")
#define RE34 gettext_noop("Application settings are not compatible.")
#define RE35 gettext_noop("Dialup connection established")
#define RE36 gettext_noop("Dialup connection terminated")
#define RE37 gettext_noop("Mail init failed (%d)")
#define RE38 gettext_noop("Mail packet sended for ")
#define RE39 gettext_noop("Error in mail packet sending for %s (%d)")
#define RE40 gettext_noop("IP protocol not started, cannot replicate via IP")
#define RE41 gettext_noop("Direct IP replication started")
#define RE42 gettext_noop("Cannot listen on the port nuber selected for the Direct IP replication")
#define RE43 gettext_noop("Loading mail box content")
#define RE44 gettext_noop("Loading mail box content failed (%d)")
#define RE45 gettext_noop("Dialup connection with mail server failed (%d)")
#define RE46 gettext_noop("Dialup connection for Direct IP replication failed (%d)")
#define RE47 gettext_noop("Direct IP connection is not available")
#define RE48 gettext_noop("No replication packet received")
#define RE49 gettext_noop("Direct IP packet transmitted for ")
#define RE50 gettext_noop("Error in Direct IP communication to %s (%d)")
#define RE51 gettext_noop("Wait, mail operation in progress")
#define RE52 gettext_noop("Replication packet no. %d could not be processed")
#define RE53 gettext_noop("Error writing socket (%d)")
#define RE54 gettext_noop("Error reading socket (%d)")
#define RE55 gettext_noop("Receiving file from address %d.%d.%d.%d")
#define RE56 gettext_noop("File has unknown format")
#define RE57 gettext_noop("Packet saved as %s")
#define RE58 gettext_noop("*** Difference ***")
#define RE59 gettext_noop("*** Difference in multiattribut value no. %d ***")
#define RE60 gettext_noop("Local")
#define RE61 gettext_noop("Incom.")
#define RE62 gettext_noop("Application %s, table %s, record %u")
#define RE63 gettext_noop("Server %s could not process packet no. %d")
#define RE64 gettext_noop("Error %d")
#define RE65 gettext_noop("Rejected replication")
#define RE66 gettext_noop("    In application %s, in table %s, in record no. %d occured error:")
#define RE67 gettext_noop("Confl.")
#define RE68 gettext_noop("Record")
#define RE69 gettext_noop("Replication request rejection")
#define RE70 gettext_noop("Incomming packet read error")
#define RE71 gettext_noop("    Incom.")
#define RE72 gettext_noop("Warning - universal key for replication has null value")
#define RE73 gettext_noop("    Application: %s  Table: %s  Record no.: %d")
#define RE74 gettext_noop("    Local")
#define RE75 gettext_noop("    In application %s, in table %s occured error:")
#define DelRecsBadFormat gettext_noop("Record deletion table _REPLDELRECS has an incorrect format")
#endif

char INTERNET[]   = "Internet";
char _MAIL602[]    = "_MAIL602";
char DIRECTIP[]   = "Direct IP";
char INPUTDIR[]   = "Input directory";
char SYSEXT[]     = "_SYSEXT";
char PACKETTAB[]  = "_PACKETTAB";


BOOL WINAPI Get_server_error_text(cdp_t cdp, int err, char * buf, unsigned buflen);

THREAD_PROC(watch_thread);
THREAD_PROC(work_thread);
THREAD_PROC(MailThread);
THREAD_PROC(transport_receiver);
THREAD_PROC(transport_sender);
static void start_transporter(cdp_t cdp);
static void monitor(cdp_t cdp, int direct, tptr server_name, t_packet_type packet_type, int numpar = 0);
static int  transact(cdp_t cdp);
static void replace_ip(cdp_t cdp, char * addr, unsigned own_ip);
static void UnPackAddrType(char *TypeAddr, char **Addr, char **Type);
static BOOL get_ip_and_port(char * addr, unsigned * ip, unsigned * port);
static BOOL GetPacketForDIP(cdp_t cdp, CIPSocket *Sock, unsigned prefip, char *fName, uns32 *PID);
static void GetAddress(cdp_t cdp, trecnum pRec, char *Address, char **Addr, char **Type);
static void CallOnSended(cdp_t cdp, WBUUID ServerID, tobjname ServerName, int PacketType, int PacketNO, int ErrNO, char *ErrText);
#ifdef WINS
static DWORD DIPDial(cdp_t cdp);
static void  ip_change_check(cdp_t cdp);
#endif

#include <stdarg.h>

void monitor_msg(cdp_t cdp, const char *  msg, ...)
{ char buf2[300];
 // translate and recode:
  char * trans_msg = trans_message(msg);  // not substituting parameters here
 // substitite: 
  va_list ap;
  va_start(ap, msg);
  vsnprintf(buf2, sizeof(buf2), trans_msg, ap);
  buf2[sizeof(buf2)-1]=0;  // on overflow the terminator is NOT written!
  va_end(ap);
  delete [] trans_msg;
 // output:
  trace_msg_general(cdp, TRACE_REPLICATION, buf2, NOOBJECT);
}

/////////////////////////////////////////////////////////////////////////////
//
// Zadost o replikaci
//
#pragma pack(1)
struct t_pull_req
{
    wbbool   m_Req;
    tobjname m_SpecTab;
    uns16    m_len16;
    char     m_SpecCond[1];
};
#pragma pack()
//
// Vlastnosti aplikace Spravce pesku, Otec pro detekci replikacnich konfliktu
// Pouziva se pri replikaci ven i dovnitr
//
struct t_apl_props
{
    trecnum m_ApplObj;
    BOOL IAmDP,     HeIsDP;
    BOOL IAmFather, HeIsFather;

    BOOL ApplPropsInit(cdp_t cdp, WBUUID appl_id, WBUUID other_server);
};
//
// Kontext aplikace pro replikaci ven
//
struct t_apl_item : public t_apl_props
{ 
    WBUUID     appl_id;
    trecnum    appl_rec;  // record number in REPL TABLE, NORECNUM if error
    t_zcr      gcr_start, gcr_stop;
    BOOL       token_request;
    tptr       special_cond;
    tobjname   special_tabname;

    void ApplInit(WBUUID appl_idIn, trecnum appl_recIn)
    // Store basic information about the application with relation to the other server
    { 
        memcpy(appl_id, appl_idIn, UUID_SIZE);
        appl_rec           = appl_recIn;
        token_request      = FALSE;
        special_cond       = NULL;
        special_tabname[0] = 0;
    }
    void SetTokenReq(t_pull_req *token_req);
    void free(void)
    { 
        if (special_cond)
            safefree((void**)&special_cond); 
    }
};

SPECIF_DYNAR(t_apl_list, t_apl_item, 1);

class t_repl_attrmap  // map of replicated columns, column number 255 is valid for all other columns
{
  uns8 map[256/8];
 public:
  bool has(tattrib a) const
    { if (a>255) a=255;
      return (map[a / 8] & (1 << (a % 8))) != 0;
    }
  void set(tattrib a)
    { if (a<=255)  
        map[a / 8] |= 1 << (a % 8);
    }   
  void unset(tattrib a)
    { if (a<=255)  
        map[a / 8] &= ~(1 << (a % 8));
    }    
  void clear(void)
    { memset(map, 0, sizeof(map)); } 
  int last_set(void) const  // returns the last set column number, or -1 if none is set
    {// find the byte:
      int i=31;
      while (i>=0 && !map[i]) i--;
      if (i<0) return -1;
     // find the bit: 
      i=8*i+7;
      while (!has(i)) i--;
      return i;
    }  
  char * ptr(void)
    { return (char*)map; }
};
//
// Kontext tabulky pro replikace ven i dovnitr
//
class t_tablerepl_info
{
public:
  
    table_descr *td;
    t_repl_attrmap attrmap;
    tattrib      keyattr, token_attr, zcr_attr;
    BOOL         unikey;
    int          keysize, indexnum;
    int          detect_offset, keyoffset, zcr_offset, luo_offset, token_offset;

    BOOL TableInit(table_descr *init_td);
    BOOL not_locked(cdp_t cdp, trecnum tr)
    { 
        if (record_lock(cdp, td, tr, WLOCK))
            return FALSE;
        record_unlock(cdp, td, tr, WLOCK);
        return TRUE;
    }
};
//
// Udalost "Paket odeslan" - ceka na ni klientske vlakno
//
// Instance se vytvori v klientskem vlaknu v repl_operation, pri vetsine operaci se preda
// v t_out_packet :: send_file pres seznam v t_replication do vlakna mailu nebo DirectIP, 
// t_replication :: SendDone nastavi udalost a uvolni referenci. Pri replikaci ven se
// v t_replctx :: replicate_appl preda pres t_server_work do work_thread a to
// t_out_packet :: send_data, je-li co replikovat, preda instanci mailu nebo DirectIP vlaknu,
// nebo nastavi udalost a uvolni referenci.
//
class CPcktSndd
{
protected:

    CPcktSndd *m_Next;
    LocEvent   m_Event;
    unsigned   m_ID;
    unsigned   m_Err;   
    unsigned   m_cRef;

public:

    static CPcktSndd *Create()
    { 
        CPcktSndd *ps = new CPcktSndd();
        if (!ps)
            return(NULL);
        if (!CreateLocalAutoEvent(FALSE, &ps->m_Event))
        {
            delete ps;
            return(NULL);
        }
        ps->m_Next = NULL;
        ps->m_Err  = NOT_ANSWERED;
        ps->m_cRef = 1;
        return(ps);
    }
    CPcktSndd *Next(){return(m_Next);}
    void SetNext(CPcktSndd *ps){m_Next = ps; m_cRef++;}
    static void Append(CPcktSndd **Hdr, CPcktSndd *List, unsigned ID)
    {
	    CPcktSndd **pps;
        for (pps = Hdr; *pps; pps = &((*pps)->m_Next));
        *pps = List;
        for (CPcktSndd *ps = List; ps; ps = ps->m_Next)
        {
            ps->m_ID = ID;
            ps->m_cRef++;
        }
    }
    void SetEvent(unsigned Err)
    {
        m_Err = Err;
        ::SetLocalAutoEvent(&m_Event);
    }
    static void SetEvents(CPcktSndd *Hdr, unsigned Err)
    {
        for (CPcktSndd *ps = Hdr; ps; ps = ps->m_Next)
            ps->SetEvent(Err);
    }
    static void SetEvents(CPcktSndd **Hdr, unsigned ID, unsigned Err)
    {
        for (CPcktSndd **pps = Hdr; *pps;)
        {
            CPcktSndd *ps = *pps;
            if (ps->m_ID == ID)
            {
                *pps = ps->m_Next;
                ps->SetEvent(Err);
                ps->Release();
            }
            else
            {
                 pps = &((*pps)->m_Next);
            }
        }
    }
    WORD Wait()
    {
        WaitLocalAutoEvent(&m_Event, 3000);
        return(m_Err);
    }
    static void Release(CPcktSndd *ps)
    {
        while (ps)
        {
            CPcktSndd *next = ps->m_Next;
            ps->Release();
            ps = next;
        }
    }
    void Release()
    {
        if (--m_cRef == 0)
        {
            CloseLocalAutoEvent(&m_Event);
            delete this;
        }
    }
};
//
// Kontext serveru 
//
typedef enum { ps_nothing=0, ps_prepared, ps_working } t_packet_state;

#define PCK_FNAME_LEN 20

struct t_server_work
{
    t_server_work   *next;
    WBUUID           server_id;
    char             packet_fname  [PCK_FNAME_LEN];    // replication packet file name without path
    char             statereq_fname[PCK_FNAME_LEN];    // state request packet file name without path
    t_packet_state   proc_state;
    BOOL             pull_req;
    unsigned         last_state_request_send_time;
    tobjnum          srvr_obj;
    tobjname         srvr_name;
    t_apl_list       apl_list;  // list of appls to be replicated out
    CPcktSndd       *PcktSndd;
    CRITICAL_SECTION cs;

    t_server_work(cdp_t cdp, WBUUID uuid, tobjnum srvr_obj);
    //~t_server_work(){DeleteCriticalSection(&cs);}
    void mark_core(void)
    { 
        apl_list.mark_core();
        for (int i = 0;  i < apl_list.count();  i++)
            if (apl_list.acc0(i)->special_cond) 
                mark(apl_list.acc0(i)->special_cond);
    }
    void Release()
    {
        for (int i = 0; i < apl_list.count(); i++)
        { 
            t_apl_item *apl_item = apl_list.acc(i);
            apl_item->free();
        }
        apl_list.~t_apl_list();
        pull_req = FALSE;
        CPcktSndd::Release(PcktSndd);
        PcktSndd = NULL;
    }
    //void Unlock()
    //{
    //    proc_state = ps_nothing;
    //    LeaveCriticalSection(&cs);
    //}
    //BOOL Lock();
    BOOL AddPacket(char *fName, BOOL Data);
    int  add_appl(cdp_t cdp, WBUUID appl_id, trecnum appl_rec, t_pull_req *pull_rq, CPcktSndd *ps);
};

// Server in inserted into this dynar when its 1st packet arrives or
// is generated. Server is never removed from dynar and its index never
// changes.
class t_server_dynar
{
protected:

    t_server_work *m_Server;
    t_server_work *GetServer(uns8 *uuid)
    { 
        for (t_server_work *server = m_Server; server; server = server->next)
        { 
            if (memcmp(uuid, server->server_id, sizeof(WBUUID)) == 0)
                return(server);
        }
        return(NULL);
    }
    t_server_work *Add(cdp_t cdp, WBUUID uuid, tobjnum srvr_obj);

public:

   ~t_server_dynar()
    {
        t_server_work *next;
        for (t_server_work *server = m_Server; server; server = next)
        {
            next = server->next;
            delete server;
        }
        m_Server = NULL;
    }
    t_server_work *get_server(cdp_t cdp, uns8 *uuid);
    void GetServerName(cdp_t cdp, WBUUID server_id, char *Buf);
    void GetServerID(cdp_t cdp, char *Name, WBUUID server_id);
    tobjnum server_id2obj(cdp_t cdp, WBUUID server_id);
    int  add_appl(cdp_t cdp, WBUUID ServerID, WBUUID ApplID, trecnum appl_rec, t_pull_req *pull_req, CPcktSndd *ps);
    void add_appls(cdp_t cdp, char *query);

    friend class t_replication;
};
//
// Popis chyby v dusledku duplicity klicu
//
class t_kdupl_info : public CTrcTbl
{
    CTrcTblRow *m_Hdr;
    CTrcTblRow *m_Rem;
    CTrcTblRow *m_Loc;
    CTrcTblRow *m_Cnf;
    BOOL        m_New;

public:

    t_kdupl_info(BOOL newrec) : CTrcTbl(newrec ? 3 : 4, 255)
    {
        m_New = newrec;
        m_Hdr = GetRow(0);
        m_Rem = GetRow(1);
        m_Hdr->AddVal(ATT_STRING, NULL, 0);

        if (newrec)
        {
            m_Cnf = GetRow(2);
        }
        else
        {
            m_Loc = GetRow(2);
            m_Cnf = GetRow(3);
            m_Loc->AddVal(ATT_STRING, RE74, sizeof(RE74) - 1);
        }
        m_Rem->AddVal(ATT_STRING, RE71, sizeof(RE71) - 1);
        m_Cnf->AddVal(ATT_STRING, RE67, sizeof(RE67) - 1);
    }
    char *GetNew(int i){return(m_Rem->GetVal(i));}
    void AddRec()
    {
        m_Hdr->AddVal(ATT_STRING, RE68, sizeof(RE68) - 1);
        m_Rem->AddVal(ATT_STRING, NULL, 0);
    }
    void AddNew(attribdef *att, const char *Val)
    {
        int Size;
        m_Hdr->AddVal(ATT_STRING, att->name, lstrlen(att->name));
        switch (att->attrtype)
        {
        case ATT_STRING:
            m_Rem->AddVal(att->attrtype, Val, lstrlen(Val));
            break;
        default:
            Size = typesize(att);
            m_Rem->AddVal(att->attrtype, Val, Size);
        }
    }
    void Add(char Type, const char *LocVal, const char *CnfVal, int Size)
    {
        switch (Type)
        {
        case ATT_STRING:
            if (!m_New)
                m_Loc->AddVal(Type, LocVal, lstrlen(LocVal));
            m_Cnf->AddVal(Type, CnfVal, lstrlen(CnfVal));
            break;
        default:
            if (!m_New)
                m_Loc->AddVal(Type, LocVal, Size);
            m_Cnf->AddVal(Type, CnfVal, Size);
        }
    }
    void AddVal(int Row, const char *Val, int Size)
    {
        CTrcTblRow *tRow = GetRow(Row);
        tRow->AddVal(ATT_STRING, Val, Size);
    }
    BOOL IsNew(){return(m_New);}
};
//
// Popis replikacniho konfliktu
//
class t_konfl_info : public CTrcTbl
{
    CTrcTblRow *m_Hdr;
    CTrcTblRow *m_Rem;
    CTrcTblRow *m_Loc;
    CTrcTblVal *m_UnqHdr;
    CTrcTblVal *m_UnqRem;
    CTrcTblVal *m_UnqLoc;

public:

    t_konfl_info() : CTrcTbl(3, 36)
    {
        m_Hdr    = GetRow(0);
        m_Rem    = GetRow(1);
        m_Loc    = GetRow(2);
        m_UnqHdr = m_Hdr->InsertVal(NULL, ATT_STRING, NULL, 0);
        m_UnqRem = m_Rem->InsertVal(NULL, ATT_STRING, RE61, sizeof(RE61) - 1);
        m_UnqLoc = m_Loc->InsertVal(NULL, ATT_STRING, RE60, sizeof(RE60) - 1);
    }
    void Add(char *Attr, char Type, const char *LocVal, char *RemVal, int Size, BOOL Uniq)
    {
        if (Uniq)
        {
            m_UnqHdr = m_Hdr->InsertVal(m_UnqHdr, ATT_STRING, Attr, lstrlen(Attr));
            switch (Type)
            {
            case ATT_STRING:
            {
                int ls = lstrlen(LocVal);
                int rs = lstrlen(RemVal);
                int sz = ls;
                if (sz > rs)
                    sz = rs;
                if (sz > 32)
                    sz = 32;
                if 
                (
                    (ls == rs && (ls == 0  || memcmp(LocVal, RemVal, ls) == 0)) ||
                     ls <= 32 ||  rs <= 32 ||
                    memcmp(LocVal, RemVal, sz)
                )
                {
                    m_UnqLoc = m_Loc->InsertVal(m_UnqLoc, Type, LocVal, ls);
                    m_UnqRem = m_Rem->InsertVal(m_UnqRem, Type, RemVal, rs);
                }
                else
                {
                    m_UnqLoc = m_Loc->InsertVal(m_UnqLoc, ATT_STRING, RE58, sizeof(RE58) - 1);
                    m_UnqRem = m_Rem->InsertVal(m_UnqRem, ATT_STRING, NULL, 0);
                }
                break;
            }
            case ATT_BINARY:
                if (memcmp(LocVal, RemVal, Size > 16 ? 16 : Size) || memcmp(LocVal, RemVal, Size) == 0)
                {
                    m_UnqLoc = m_Loc->InsertVal(m_UnqLoc, Type, LocVal, Size);
                    m_UnqRem = m_Rem->InsertVal(m_UnqRem, Type, RemVal, Size);
                }
                else
                {
                    m_UnqLoc = m_Loc->InsertVal(m_UnqLoc, ATT_STRING, RE58, sizeof(RE58) - 1);
                    m_UnqRem = m_Rem->InsertVal(m_UnqRem, ATT_STRING, NULL, 0);
                }
                break;
            default:
                m_UnqLoc = m_Loc->InsertVal(m_UnqLoc, Type, LocVal, Size);
                m_UnqRem = m_Rem->InsertVal(m_UnqRem, Type, RemVal, Size);
            }
        }
        else
        {
            m_Hdr->AddVal(ATT_STRING, Attr, lstrlen(Attr));
            switch (Type)
            {
            case ATT_STRING:
            {
                int ls = lstrlen(LocVal);
                int rs = lstrlen(RemVal);
                int sz = ls;
                if (sz > rs)
                    sz = rs;
                if (sz > 32)
                    sz = 32;
                if 
                (
                    (ls == rs && (ls == 0  || memcmp(LocVal, RemVal, ls) == 0)) ||
                     ls <= 32 ||  rs <= 32 ||
                    memcmp(LocVal, RemVal, sz)
                )
                {
                    m_Loc->AddVal(Type, LocVal, ls);
                    m_Rem->AddVal(Type, RemVal, rs);
                }
                else
                {
                    m_Loc->AddVal(ATT_STRING, RE58, sizeof(RE58) - 1);
                    m_Rem->AddVal(ATT_STRING, NULL, 0);
                }
                break;
            }
            case ATT_BINARY:
                if (memcmp(LocVal, RemVal, Size > 16 ? 16 : Size) || memcmp(LocVal, RemVal, Size) == 0)
                {
                    m_Loc->AddVal(Type, LocVal, Size);
                    m_Rem->AddVal(Type, RemVal, Size);
                }
                else
                {
                    m_Loc->AddVal(ATT_STRING, RE58, sizeof(RE58) - 1);
                    m_Rem->AddVal(ATT_STRING, NULL, 0);
                }
                break;
            default:
                m_Loc->AddVal(Type, LocVal, Size);
                m_Rem->AddVal(Type, RemVal, Size);
            }
        }
    }
    void Add(char *Attr, char Type, char *LocVal, int LocSz, char *RemVal, int RemSz)
    {
        m_Hdr->AddVal(ATT_STRING, Attr, lstrlen(Attr));
        if (Type == ATT_TEXT)
        {
            int sz = LocSz;
            if (sz > RemSz)
                sz = RemSz;
            if (sz > 32)
                sz = 32;
            if (!sz || LocSz <= 32 || RemSz <= 32 || memcmp(LocVal, RemVal, sz))
            {
                m_Loc->AddVal(Type, LocVal, LocSz);
                m_Rem->AddVal(Type, RemVal, RemSz);
            }
            else
            {
                m_Loc->AddVal(ATT_STRING, RE58, sizeof(RE58) - 1);
                m_Rem->AddVal(ATT_STRING, NULL, 0);
            }
        }
        else
        {
            m_Loc->AddVal(ATT_STRING, RE58, sizeof(RE58) - 1);
            m_Rem->AddVal(ATT_STRING, NULL, 0);
        }
    }
    void Add(char *Attr, int index)
    {
        char buf[sizeof(RE58) + 6];
        int sz = sprintf(buf, RE58, index);
        m_Hdr->AddVal(ATT_STRING, Attr, lstrlen(Attr));
        m_Loc->AddVal(ATT_STRING, buf,  sz);
        m_Rem->AddVal(ATT_STRING, NULL, 0);
    }
};
//
// Zahlavi paketu
//
#define CURR_PACKET_VERSION (5*256)

struct t_packet_header
{ 
    uns32         m_version;
    WBUUID        m_dest_server;
    WBUUID        m_source_server;
    t_packet_type m_packet_type;
};
//
// Serverove info
//
struct t_server_pack
{ 
    tobjname server_name;
    WBUUID   uuid;
    uns8     addr1[254 + 1];
    uns8     addr2[254 + 1];
};
//
// Paket typu "Zamitnuti replikace"
//
struct t_nack
{
    uns32    m_PacketNo;
    tobjname m_Appl;
    tobjname m_Table;
    trecnum  m_RecNum;
    int      m_Err;

    t_nack()
    {
        m_PacketNo = 0;
        m_Appl[0]  = 0;
        m_Table[0] = 0;
        m_RecNum   = NORECNUM;
        m_Err      = NOERROR;
    }
  
};

struct t_nackx : public t_nack
{
    uns32    m_KeyType;
    uns32    m_KeySize;
    char     m_KeyVal[MAX_KEY_LEN];

    t_nackx(){m_KeyType = 0;}
};
//
// Doplnkove informace o zamitnuti replikace
//
struct t_nackadi
{
    uns32    m_Type;
    uns32    m_Size;
    char     m_Val[1];
};
//
// Typ doplnkove informace
//
#define NAX_UNIKEY   0x0100     // _W5_UNIKEY
#define NAX_PRIMARY  0x0200     // Primarni klic
#define NAX_KDPLHDR  0x0300     // Hlavicka informace o duplicite klicu
#define NAX_KDPLREM  0x0400     // Informace o doslem zaznamu, ktery zpusobil duplicitu klicu
#define NAX_KDPLLOC  0x0500     // Informace o mistnim zaznamu, ktery zpusobil duplicitu klicu
#define NAX_KDPLCNF  0x0600     // Informace o zaznamu, se kterym je dosly v konfliktu
#define NAX_ERRSUPPL 0x0700     // Doplnkova informace o chybe 
#define NAX_APPLUUID 0x0800     // UUID Aplikace
//
// Spolecny kontext replikacniho paketu ven i dovnitr
//
class t_replctx : public t_packet_header, public t_tablerepl_info
{
protected:

    cdp_t    m_cdp;
    tobjname m_SrvrName;
    tobjnum  m_SrvrObj;
    WBUUID   m_ApplID;

    trecnum get_appl_replrec (WBUUID src_srv, WBUUID dest_srv, tptr tabname);
    trecnum find_appl_replrec(WBUUID src_srv, WBUUID dest_srv, tptr tabname);
    void    last_packet_acknowledged(WBUUID dest_srv);
    int     find_attribute(table_descr *td, const char *name);

public:

    t_replctx(){}
    t_replctx(cdp_t cdp)
    {
        m_cdp = cdp;
        memset(m_ApplID, 0, UUID_SIZE);
    }
    t_replctx(cdp_t cdp, char *Src)
    {
        m_cdp = cdp;
        memcpy(m_dest_server, Src,             UUID_SIZE);
        memcpy(m_ApplID,      Src + UUID_SIZE, UUID_SIZE);
    }
   ~t_replctx()
    {
        m_cdp->set_return_code(ANS_OK);
    }
    unsigned create_relation(WBUUID dest_srv, tptr relname, BOOL second_side);
    unsigned replicate_appl (WBUUID dest_srv, t_pull_req *pull_req = NULL, CPcktSndd *ps = NULL);
    unsigned skip_repl_appl();
    unsigned cancel_packet(tobjnum srvobj);
    void  reset_server_conn(tobjnum srvobj);
    void  remove_replic();
};
//
// Vstupni replikacni paket
//
struct t_in_packet : public t_replctx, public t_nackx, public t_apl_props
{
    FHANDLE       m_fhnd;
    tobjnum       m_TableObj;
    char          m_ErrMsg[128];
    timestamp     m_DT;
    trecnum       m_RecCnf;
    short         m_Warn;
    t_kdupl_info *m_DuplInfo;
    trecnum       m_ErrPos;
    BOOL          m_ConflResolution;
    t_repl_attrmap m_local_attrmap;
    dd_index     *m_indx;
    dd_index     *m_DelRecsIndex;
//    unsigned      m_rec_counter;

    static tobjnum   ReplErrHdr;
    static tobjnum   ReplErrRec;
    static tobjnum   ReplErrAttr;
    static timestamp ReplErrDT;

    t_in_packet(cdp_t cdp, FHANDLE fhnd) : t_replctx(cdp)
    {
        m_cdp          = cdp;
        m_fhnd         = fhnd;
        m_TableObj     = NOOBJECT;
        m_ErrMsg[0]    = 0;
        m_DT           = NONETIMESTAMP;
        m_RecCnf       = NORECNUM;
        m_Warn         = 0;
        m_DuplInfo     = NULL;
        m_ErrPos       = NORECNUM;
        m_DelRecsIndex = NULL;
    }
   ~t_in_packet()
    {
        if (m_DuplInfo)
            delete m_DuplInfo;
    }

    BOOL process_input_packet();
    BOOL process_incomming_repl();
    BOOL store_record(sig16 token_val, BOOL added);
    BOOL hp_rewrite(tpiece *pc, tattrib attr, uns16 index, BOOL *changed, BOOL do_copy);
    BOOL hp_difference(const tpiece *pc, const char *data, DWORD size, BOOL *diff);
    BOOL get_file_data(void * dest, DWORD size);
    void skip_file_data(DWORD size);
    BOOL get_file_hdata(tptr *pdata, uns32 * size);
    BOOL get_file_longdata(void *& data, uns32 & size);
    void konfl_info();
    void konfl_infodet();
    void dupl_infodet(BOOL newrec);

    tobjnum store_remote_server_description(/*BOOL answer*/);
    
    BOOL merge_servers(WBUUID dp_uuid, WBUUID os_uuid, WBUUID vs_uuid, BOOL * is_dp_server);

    void process_nack();
    void load_var_len_attribute(trecnum rec, tattrib attr);
    trecnum Unikey2RecNum(t_nackx *nack, t_nackadi *adi);
    t_kdupl_info *nack_dupl(int RowSize, trecnum rInc);
    void WriteReplErr(CTrcTbl *ki, t_nackx *nack, trecnum rInc, trecnum rLoc, trecnum rCnf);
    BOOL WriteReplErrHdr();
    BOOL GetReplErrTbls();
    BOOL token_in_appl(WBUUID appl_uuid);
    BOOL RelCmp(t_relatio *relo, t_relatio *reln, BOOL Acc);
    BOOL ProcessChanges();
    uns8 ProcessDelet();
    uns8 SkipChanges();
    bool SkipRestOfRecord(DWORD Off);
    void sharing_prepared(trecnum aplrec, t_replapl_states newstate);
    void CallOnReceived();
};
//
// Vystupni replikacni paket
//
class t_out_packet : public t_replctx
{ 
protected:

    DWORD       m_Err;
    FHANDLE     m_fhnd;
    uns32       m_PacketNo;
    BOOL        m_Empty;        // implicitne neni empty, pri generovani PKTTP_REPL_DATA se nejdriv empty schodi
                                // a po zapisu nahodi, destruktor podle empty paket smaze 
    t_apl_item *m_ApplItem;
    char        m_SrvrAddr[MAX_PATH];
    char        m_tempname[MAX_PATH];
    CPcktSndd  *m_PcktSndd;

protected:

    BOOL     start_packet(t_packet_type pkttype);
    BOOL     write_packet(const void *data, DWORD size);
    BOOL     complete_packet();
    BOOL     repl_appl_pack();
    BOOL     store_repl_record(trecnum tr, BOOL pass_token, BOOL deleted_only);
    BOOL     hp_packet_copy(const tpiece * pc, uns32 size);
    BOOL     write_own_server_info();
    bool     StoreDelRecsRecord(table_descr *tdr, trecnum tr);
    void     send_file();
    void     monitor_packet_err();
    tcursnum find_appl_replrecs(WBUUID appl_id, cur_descr **pcd);

public:

    t_out_packet(cdp_t cdp, CPcktSndd *PcktSndd = NULL)
    { 
        m_cdp         = cdp;
        m_PacketNo    = 0;
        m_Err         = NOERROR;
        m_fhnd        = INVALID_FHANDLE_VALUE;
        m_Empty       = FALSE;
        m_SrvrAddr[0] = 0;
        m_PcktSndd    = PcktSndd;
    }
    t_out_packet(cdp_t cdp, WBUUID DestSrvrID, tobjnum DestSrvrObj = NOOBJECT, WBUUID ApplID = NULL, CPcktSndd *PcktSndd = NULL)
    { 
        m_cdp         = cdp;
        m_PacketNo    = 0;
        m_Err         = NOERROR;
        m_fhnd        = INVALID_FHANDLE_VALUE;
        m_Empty       = FALSE;
        m_SrvrObj     = DestSrvrObj;
        m_SrvrAddr[0] = 0;
        m_PcktSndd    = PcktSndd;
        memcpy(m_dest_server, DestSrvrID, UUID_SIZE);
        if (ApplID)
            memcpy(m_ApplID, ApplID, UUID_SIZE);
    }
    t_out_packet(cdp_t cdp, char *Src, CPcktSndd *PcktSndd = NULL) : t_replctx(cdp, Src)
    { 
        m_PacketNo    = 0;
        m_Err         = NOERROR;
        m_fhnd        = INVALID_FHANDLE_VALUE;
        m_Empty       = FALSE;
        m_SrvrAddr[0] = 0;
        m_PcktSndd    = PcktSndd;
    }
    ~t_out_packet();
    DWORD send_server_info_request(char *eaddr, int opparsize);
    void  send_server_info();
    DWORD send_relation_request(t_relreq_pars *rrp);
    DWORD send_sharing_control(t_packet_type optype, char *relname, int errnum);
    DWORD send_pull_request(t_repl_param *pr);
    BOOL  send_state_request();
    DWORD resend(tobjnum srvobj);
    void  send_state_info(uns32 recvnum);
    void  send_data(t_server_work *server);    
    void  send_acknowledge(uns32 PacketNo);
    void  send_nack(t_nackx *nack, t_kdupl_info *di, WBUUID ApplID);
    void  send_nothtorepl(int err);
    void  resend_file(uns32 sendnum);
};
//
// Sprava replikaci 
//
#define MAX_THREADS  10
#define THREAD_STACK 40000
#define STATE_REQUEST_REPEAT_PERIOD 3600000 /* 1 hour */

class t_replication
{ 
 private:
  DWORD            work_interval;
  Semaph           work_waiting;
  int              running_threads;
  THREAD_HANDLE    threads[1+MAX_THREADS]; // all joinable
  LocEvent         hReplStopEvent;
  char             shared_dir[MAX_PATH];
  BOOL             RcvReq;
  LocEvent         hMailReqEvent;
#ifndef WINS
  LocEvent         hMailWorking;
#endif
  cd_t             replication_cd;
  BOOL             Remote;
  CPcktSndd       *PcktSndd;

 public:

  int              repl_thread_count;
  //
  // Inicializace
  //
  t_replication(void) : replication_cd(PT_WORKER)
    { }
  BOOL replication_start(cdp_t cdp);
  void replication_stop(void);
  void work_thread(cdp_t cdp);
  void watch_thread(void);
  void MailThread(cdp_t mcdp);
  //
  // Ovladani replikaci
  //
  void  SendDone(cdp_t cdp, uns32 PID, char *fName, int Dir, int Err);
  void  PackPackTab(cdp_t cdp);
  LocEvent *GetStopEvent(){return &hReplStopEvent;}
  void  ReleaseWorkThread(int cnt = 1){ReleaseSemaph(&work_waiting, cnt);}
  void  ReleaseWatchThread(){SetLocalAutoEvent(&hReplStopEvent);}
  void  StartDispatchThread(cdp_t cdp, BOOL DirectIP);

 private:

  void scan_replication_plans(void);
  void scan_input_queue(void);
  //
  // Mail
  //
  void mail_receive(void);
  UINT Receive     (cdp_t mcdp);
  void Send        (cdp_t mcdp, char *Addr, char *Type, char *fName, DWORD PID);
  BOOL GetPacketForMail(cdp_t cdp, char *Address, char **Addr, char **Type, char *fName, uns32 *PID);
  //
  // Utils
  //
  void CleanOutQueue();
  void GetReplType(tattrib Atrib);
  BOOL OnFlopy(char *Path);

 public:
  
  CPcktSndd **GetPcktSnddHdr(){return(&PcktSndd);}
  void mark_core(void);
};

// Replication variables made static, because they are used by the transporter which cannot be stopped
// when replication is deleted.
static char     input_queue[MAX_PATH]; // defined in replication_start
static char     output_queue[MAX_PATH];  // defined in replication_start
#ifndef LINUX
static BOOL     repl_shutdown;
#else
static bool_condition repl_shutdown(&hKernelShutdownEvent); /* manipulace chranena zamkem +
					       posle se sprava pokud zmeneno */
#endif
static int transporter_running=0;
static unsigned transporter_port;
static SOCKET   trans_lsn_sock;
static LocEvent hTransportEvent;      // wakes up the transport sender (when shuting down or when it shloud work)
static THREAD_HANDLE trans_sender, trans_receiver;
static unsigned preferred_send_ip;
static int      preferred_send_ip_state=0;  // 0-nothing, 1-received, 2-processing
static BOOL     OnSended;
static BOOL     OnSent;
static BOOL     OnReceived;
static BOOL     LogTables;
static BOOL     MailRepl;
#ifdef WINS
static BOOL     DialRepl;
static BOOL     DialReq;
static BOOL     MailDialed;
static BOOL     DIPTimeout;
static BOOL     DIPrWork;
static BOOL     DIPDialed;
static BOOL     DIPWSA;
static char     DIPDConn[64];
static char     DIPDUser[64];
static char     DIPDPword[64];
static HRASCONN DIPhDialConn;
#endif

static CRITICAL_SECTION cs_replica;
static CRITICAL_SECTION cs_replfls;
static CRITICAL_SECTION cs_mail;
static THREAD_HANDLE    hMailThread;
static t_replication   *the_replication;
static table_descr     *srvtab_descr;
static table_descr     *rpltab_descr;
static table_descr     *pcktab_descr;
static t_server_dynar   srvs;        // protected by cs_replica
static WBUUID           my_server_id;
static bool             ErrorCausesNack;

#ifdef NETWARE
#include <advanced.h>
static NETDBGETHOSTBYNAME lpNetDBgethostbyname;
#endif  // NETWARE
//
// Interface
//
// Spusteni replikaci pri startu serveru nebo z dialogu Replikace
//
void repl_start(cdp_t cdp)
{ 
    if (the_replication)
        return;
    srvtab_descr  = install_table(cdp, SRV_TABLENUM);
    if (srvtab_descr)
    { 
        wbbool repl_allowed;
        tb_read_atr(cdp, srvtab_descr, 0, SRV_ATR_ACKN, (tptr)&repl_allowed);
        if (repl_allowed) 
        { 
            the_replication = new t_replication();
            if (the_replication)
            { 
                if (the_replication->replication_start(cdp))
                    return;  // started OK
                repl_stop();
            }
        }
        unlock_tabdescr(srvtab_descr); // not started
    }
}
//
// Ukonceni replikaci pri ukonceni serveru nebo z dialogu Replikace
//
void repl_stop()
{ 
    if (the_replication)
    {
        unlock_tabdescr(srvtab_descr);
        the_replication->replication_stop();
        delete the_replication;
        the_replication = NULL;
    }
}
//
// Klientsky pozadavek na replikace
//
void repl_operation(cdp_t cdp, int optype, int opparsize, char *opparam)
// client request for replication operation specified by optype
{ 
    if (!the_replication) 
    { 
        request_error(cdp, REPLICATION_NOT_RUNNING);
        return; 
    }

#ifdef WINS
    // IF Vytacene spojeni, bude se vytacet
    if (DialRepl && optype >= PKTTP_REPL_DATA && optype <= PKTTP_REPL_STOP)
        DialReq = TRUE;
#endif

    CPcktSndd *PcktSndd = NULL;
    DWORD Err = NOERROR;
    switch (optype)
    { 
        case PKTTP_SERVER_REQ: // send request for server info
        {
            PcktSndd = CPcktSndd::Create();
            t_out_packet packet(cdp, PcktSndd);
            Err = packet.send_server_info_request(opparam, opparsize);
            break;
        }
        case PKTTP_REL_REQ:    // send request for a relation
        {
            PcktSndd = CPcktSndd::Create();
            t_out_packet packet(cdp, ((t_relreq_pars *)opparam)->srv_uuid, NOOBJECT, NULL, PcktSndd);
            Err = packet.send_relation_request((t_relreq_pars *)opparam);
            break;
        }
        case PKTTP_REL_ACK:    // accept the relation
        {
            PcktSndd = CPcktSndd::Create();
            t_out_packet packet(cdp, opparam, PcktSndd);
            Err = packet.create_relation((uns8 *)opparam, opparam + 2 * UUID_SIZE, TRUE);
            if (Err)
                break;
            Err = packet.send_sharing_control(PKTTP_REL_ACK, opparam + 2 * UUID_SIZE, 0);
            break;
        }
        case PKTTP_REL_NOACK:  // deny the relation
        {
            PcktSndd = CPcktSndd::Create();
            t_out_packet packet(cdp, opparam, PcktSndd);
            Err = packet.send_sharing_control(PKTTP_REL_NOACK, NULL, 0);
            break;
        }
        case PKTTP_REPL_REQ:   // send request for replication: params=server id, appl id
        {
            LPDWORD Srvr = (LPDWORD)opparam;
            // IF prazdne UUID serveru, stahnout dosle pakety
            if (Srvr[0] == 0 && Srvr[1] ==  0 && Srvr[2] == 0)
            { 
                the_replication->ReleaseWatchThread();
                break;
            }
            // Jinak odeslat zadost o replikaci
            else
            {
                PcktSndd = CPcktSndd::Create();
                t_out_packet packet(cdp, opparam, PcktSndd);
                Err = packet.send_pull_request((t_repl_param *)opparam);
            }
            break;
        }
        case PKTTP_REPL_DATA:  // replicate now: params=server id, appl id
        {
            PcktSndd = CPcktSndd::Create();
            t_replctx ctx(cdp, opparam);
            Err = ctx.replicate_appl((uns8 *)opparam, NULL, PcktSndd);
            break;
        }
        case OPER_SKIP_REPL:   // preskocit replikaci
        {
            t_replctx ctx(cdp, opparam);
            Err = ctx.skip_repl_appl();
            break;
        }
        case OPER_RESET_COMM:  // reset server "push" connection
        {
            t_replctx ctx(cdp);
            ctx.reset_server_conn(*(tobjnum *)opparam);
            break;
        }
        case OPER_RESEND_PACKET:  // opakovane odeslat ztraceny nebo odmitnuty paket
        {
            PcktSndd = CPcktSndd::Create();
            t_out_packet packet(cdp, PcktSndd);
            Err = packet.resend(*(tobjnum *)opparam);
            break;
        }
        case OPER_CANCEL_PACKET:  // zrusit odmitnuty paket
        {
            t_replctx ctx(cdp);
            Err = ctx.cancel_packet(*(tobjnum *)opparam);
            break;
        }
        case PKTTP_REPL_STOP:  // stop the relation
        {
            PcktSndd = CPcktSndd::Create();
            t_out_packet packet(cdp, opparam, PcktSndd);
            Err = packet.send_sharing_control(PKTTP_REPL_STOP, NULL, 0);
            // remove local replication info:
            packet.remove_replic();
            break;
        }
    }
    if (PcktSndd)
    {
        if (!Err)
            Err = PcktSndd->Wait();
        PcktSndd->Release();
    }
    if (Err)   
    {
        if (Err == NOT_ANSWERED || Err == NOTHING_TO_REPL || Err == WAITING_FOR_ACKN)
            cdp->set_return_code(Err);
        else
            request_error(cdp, Err);
    }
}
//
// Vola srvctrl.cpp pri zobrazovani stavu v okne serveru
//
static int ip_transport_counter=0, repl_in_counter=0, repl_out_counter=0;

void get_repl_state(int * ip_transport, int * repl_in, int * repl_out)
{ 
    *ip_transport = ip_transport_counter;
    *repl_in      = repl_in_counter;
    *repl_out     = repl_out_counter;
}
//
// Vola leak_test
// 
void mark_replication(void)
{ 
    if (the_replication)
    {
        mark(the_replication);
        the_replication->mark_core();
    }
}
//
// Vola OP_GI_REPLICATION - test replikace bezi / nebezi
//
uns32 GetReplInfo(void)
{    
    return(the_replication != NULL);
}
//
// t_replication
// 
// Spusteni
//
static char CreCmd[] = 
"CREATE TABLE _SYSEXT._PACKETTAB "
"("
    "ID           INTEGER DEFAULT UNIQUE, "
    "Erase        BIT, "
    "PacketType   INTEGER, "
    "PacketNo     INTEGER, "
    "DestServObj  INTEGER, "
    "DestServName CHAR(31), "
    "Address      CHAR(79), "
    "fName        CHAR(254), "
    "CONSTRAINT `I1` PRIMARY KEY (ID), "
    "CONSTRAINT `I2` INDEX (`DestServName`)"
")";

BOOL t_replication :: replication_start(cdp_t cdp)
{ 
    memset(threads, 0, sizeof(threads));
    repl_thread_count = 0;
    shared_dir[0]     = 0;
    RcvReq            = FALSE;
    repl_shutdown     = FALSE;
    PcktSndd          = NULL;
#ifndef UNIX
    hReplStopEvent    = 0;
    hMailReqEvent     = 0;
#endif

    ip_transport_counter = 0;
    repl_in_counter      = 0;
    repl_out_counter     = 0;
    
    InitializeCriticalSection(&cs_replica);
    InitializeCriticalSection(&cs_replfls);
    InitializeCriticalSection(&cs_mail);
    CreateSemaph(0, 100, &work_waiting);
    rpltab_descr  = install_table(cdp, REPL_TABLENUM);
    if (!rpltab_descr)
        return(FALSE);
    tobjnum   Apl;
    WBUUID    UUID;
    ttablenum PkTable;
    if (ker_apl_name2id(cdp, SYSEXT, (uns8 *)&UUID, &Apl))
    {
        sql_stmt_create_schema so;
        lstrcpy(so.schema_name, SYSEXT);
        if (so.exec(cdp))
            return(FALSE);
        commit(cdp);
        if (ker_apl_name2id(cdp, SYSEXT, (uns8 *)&UUID, &Apl))
            return(FALSE);
    }
    PkTable = find2_object(cdp, CATEG_TABLE, (const uns8 *)&UUID, PACKETTAB);
    if (PkTable == NOOBJECT)
    {
        if (sql_exec_direct(cdp, CreCmd))
            return(FALSE);
        commit(cdp);
        PkTable = find2_object(cdp, CATEG_TABLE, (const uns8 *)&UUID, PACKETTAB);
        if (PkTable == NOOBJECT)
            return(FALSE);
    }
    pcktab_descr = install_table(cdp, PkTable);
    if (!pcktab_descr)
        return(FALSE);
    OnReceived = find2_object(cdp, CATEG_PROC, UUID, "_ON_REPLPACKET_RECEIVED") != NOOBJECT;
    OnSent     = find2_object(cdp, CATEG_PROC, UUID, "_ON_REPLPACKET_SENT")     != NOOBJECT;
    OnSended   = find2_object(cdp, CATEG_PROC, UUID, "_ON_REPLPACKET_SENDED")   != NOOBJECT;

    if (!THREAD_START_JOINABLE(::watch_thread, THREAD_STACK, this, &threads[0]))
    { CloseLocalAutoEvent(&hReplStopEvent);  return FALSE; }
  // watch thread stops immediately if replication not allowed, but its
  // handle remains allocated until replication_stop() is called.
  return TRUE;
}
//
// Ukonceni
//
void t_replication :: replication_stop(void)
{ 
    if (repl_shutdown)
        return;
    repl_shutdown = TRUE;
    if (hMailThread)
    {
        monitor_msg(&replication_cd, RE51);
        SetLocalAutoEvent(&hMailReqEvent);
#ifdef WINS
        WaitForSingleObject(hMailThread, 180000);
        CloseHandle(hMailThread);
        hMailThread = NULL;
#else
        WaitLocalAutoEvent(&hMailWorking, 180000);
#endif
    }
    
    int i;
    ReleaseSemaph(&work_waiting, MAX_THREADS); // releases waiting working threads
#ifdef WINS
    for (i = 1; i <= MAX_THREADS; i++)  // waits for working threads to stop
        if (threads[i])
            WaitForSingleObject(threads[i], 20000);
#else
#ifdef UNIX
    for (i = 1; i <= MAX_THREADS; i++)  // waits for working threads to stop
        if (threads[i]) 
            pthread_join(threads[i], NULL);
#else // NLM
    for (i = 0; repl_thread_count > 1 && i < 40; i++) 
        delay(500);
#endif
    if (MailRepl)
        CloseLocalAutoEvent(&hMailWorking);
#endif
    if (transporter_running)
#ifdef WINS
    {
        closesocket(trans_lsn_sock);          // stops transport listener
        trans_lsn_sock = -1;
        SetLocalAutoEvent(&hTransportEvent);       // stops transport sender
        Sleep(500);                                // time for listener to stop
        WaitForSingleObject(trans_sender, 60000);   // waits for sender to stop
        //transporter_running=FALSE;
        CloseLocalAutoEvent(&hTransportEvent);
        if (DIPWSA)
        {
            WSACleanup();
            DIPWSA = FALSE;
        }
    }
#else
    {
	  
        pthread_cancel(trans_receiver);   // zavri thread, podle socketu nebo cisla threadu
#ifdef UNIX
        pthread_join(trans_sender, NULL);
        pthread_join(trans_receiver, NULL);
#else
        for (i = 0; transporter_running && i < 3 * 60 * 2; i++)
            delay(500);
#endif
    }
#endif
    SetLocalAutoEvent(&hReplStopEvent); // deletes watch cdp!!
#ifdef WINS
    WaitForSingleObject(threads[0], 5000);  // if exists on timeout, the watch thread may crash because the_replication does not exist
#else
#ifdef UNIX
    pthread_join(threads[0], NULL);
#else // NLM
    for (i = 0;  repl_thread_count && i < 10;  i++) 
        delay(500);
#endif
#endif
    CloseSemaph(&work_waiting);
    CloseLocalAutoEvent(&hReplStopEvent);
    DeleteCriticalSection(&cs_replica);
    DeleteCriticalSection(&cs_replfls);
    DeleteCriticalSection(&cs_mail);
    if (pcktab_descr)
      { unlock_tabdescr(pcktab_descr);  pcktab_descr=NULL; }
    if (rpltab_descr)
      { unlock_tabdescr(rpltab_descr);  rpltab_descr=NULL; }
    srvs.~t_server_dynar();
}
//
// watch_thread - prohlizi vstupni frontu a replikacni plany a zarazuje pozadavky do fronty work_threadu
//
THREAD_PROC(watch_thread)  // joinable
{
#ifdef NETWARE
  RenameThread(GetThreadID(), "Replication watch thread");
  delay(3000);
#endif
  t_replication * repl_info = (t_replication*)data;
  repl_info->repl_thread_count++;
  repl_info->watch_thread();
  repl_info->repl_thread_count--;
  THREAD_RETURN;
}

void t_replication::watch_thread(void)
// uses class cdp
{
 // init replication structures (interf_init must be called by this thread!)
  int input_threads;  char my_server_name[OBJ_NAME_LEN+1];
  int res = interf_init(&replication_cd);
  if (res != KSE_OK)
  {
      client_start_error(&replication_cd, workerThreadStart, res);
      return;
  }
  replication_cd.mask_token=wbtrue;
  integrity_check_planning.system_worker_thread_start();

  tb_read_atr(&replication_cd, srvtab_descr, 0, SRV_ATR_NAME, my_server_name);
  tb_read_atr(&replication_cd, srvtab_descr, 0, SRV_ATR_UUID, (tptr)my_server_id);
 // read server replication parameters:
  t_my_server_info lsrv;  memset(&lsrv, 0, sizeof(t_my_server_info));
  tb_read_var(&replication_cd, srvtab_descr, 0, SRV_ATR_APPLS, 0, sizeof(t_my_server_info), (tptr)&lsrv);
  lstrcpy(input_queue,  lsrv.input_dir);
  lstrcpy(output_queue, lsrv.output_dir);
  input_threads   = lsrv.input_threads;
  work_interval   = lsrv.scanning_period * 60000L;
  LogTables       = lsrv.log_tables;
  ErrorCausesNack = lsrv.err_causes_nack != 0;
  if (!lsrv.repl_triggers)
  {
      OnSent     = FALSE;
      OnSended   = FALSE;
      OnReceived = FALSE;
  }
#ifdef WINS
  DialReq      = FALSE;
  MailDialed   = FALSE;
  DIPrWork     = FALSE;
  DIPDialed    = FALSE;
  DIPWSA       = FALSE;
  DIPhDialConn = NULL;      
  lstrcpy(DIPDConn,  lsrv.dipdconn);
  lstrcpy(DIPDUser,  lsrv.dipduser);
  lstrcpy(DIPDPword, lsrv.dipdpword);
  DialRepl = (BOOL)*lsrv.dipdconn;
  if (!DialRepl && *lsrv.profile)
  {
    if (cd_InitWBMail(&replication_cd, lsrv.profile, lsrv.mailpword) == NOERROR)
    {
      DialRepl = cd_MailGetType(&replication_cd) & WBMT_DIALUP;
      cd_CloseWBMail(&replication_cd);
    }
  }
#endif

  wbbool replication_alowed;
  tb_read_atr(&replication_cd, srvtab_descr, 0, SRV_ATR_ACKN, (tptr)&replication_alowed);
  if (input_threads > MAX_THREADS) input_threads=MAX_THREADS;
  if (input_threads < 0) input_threads=0;
 // find own transporter port:
  transporter_port = 0;
  MailRepl = FALSE;
  GetReplType(SRV_ATR_ADDR1);
  GetReplType(SRV_ATR_ADDR2);
  CleanOutQueue();              // Pri spusteni vymete smeti z vystupni fronty
#ifndef LINUX
  integrity_check_planning.thread_operation_stopped();
  Sleep(2000);  // time for service to open the window (messages unblock the check box)
  integrity_check_planning.continuing_replication_operation();
#endif

  if (replication_alowed)
  {
    if (!OnFlopy(input_queue))
        CreateDirectory(input_queue, NULL);
    if (!OnFlopy(output_queue))
        CreateDirectory(output_queue, NULL);

   // start working threads:
    CreateLocalAutoEvent(FALSE, &hReplStopEvent);
    CreateLocalAutoEvent(FALSE, &hMailReqEvent);
#ifndef WINS
    if (MailRepl)
      CreateLocalAutoEvent(TRUE, &hMailWorking);
#endif
    for (int i=0;  i<input_threads;  i++)
      THREAD_START_JOINABLE(::work_thread, THREAD_STACK, this, &threads[1+i]);
#ifdef WINS
    if (transporter_port && !*DIPDConn) 
#else
    if (transporter_port) 
#endif
      start_transporter(&replication_cd);
    monitor_msg(&replication_cd, RE28);
   // do watch:
    DWORD last_scan_time=0, start_time;
    DWORD res = WAIT_TIMEOUT;
    do
    { replication_cd.set_return_code(ANS_OK);
      replication_cd.wait_type=WAIT_NO;
      scan_input_queue();
      if (repl_shutdown) break;
      start_time =GetTickCount();
      if (start_time > last_scan_time + work_interval)
      { scan_replication_plans();
        last_scan_time=start_time;
      }
      if (repl_shutdown) break;
      integrity_check_planning.thread_operation_stopped();
      replication_cd.wait_type=WAIT_SLEEP;
      res=WaitLocalAutoEvent(&hReplStopEvent, work_interval);
      integrity_check_planning.continuing_replication_operation();
    } while (!repl_shutdown);
    replication_cd.wait_type=WAIT_TERMINATING;
    monitor_msg(&replication_cd, RE29);
  }
  kernel_cdp_free(&replication_cd);
  integrity_check_planning.thread_operation_stopped();
  cd_interf_close(&replication_cd);
}
//
// Vraci typ pouzite replikace Mail / DirectIP / Sdilene adresare
//
void t_replication::GetReplType(tattrib Atrib)
{
    char AddrType[MAX_PATH];
    char *Addr, *Type;
    tb_read_atr(&replication_cd, srvtab_descr, 0, Atrib, AddrType);
    if (!*AddrType)
        return;
    UnPackAddrType(AddrType, &Addr, &Type);
    if (stricmp(Type, DIRECTIP) == 0)
    {
        unsigned a1,a2,a3,a4,port;
        int flds = sscanf(Addr, " %u . %u . %u . %u : %u ", &a1, &a2, &a3, &a4, &port);
        if (flds == 4)
            transporter_port = the_sp.wb_tcpip_port.val() + 2;  // base port +2
        else if (flds == 5)
            transporter_port = port;
        else
        { 
            char *p = strchr(Addr, ':');
            if (p)
                transporter_port = atoi(p+1);
            else
                transporter_port = the_sp.wb_tcpip_port.val() + 2;
        }
    }
    else if (stricmp(Type, INPUTDIR) == 0)
        lstrcpy(shared_dir, Addr);
    else
        MailRepl = TRUE;
}
//
// Smaze zbytecne soubory ve vystupni fronte
//
void t_replication::CleanOutQueue()
{
    tobjnum    DestServ   = NOOBJECT;
    int        PacketType = 0;
    char       fName[MAX_PATH];
    trecnum    r;
    trecnum    tr;
    cur_descr *cd;
    WIN32_FIND_DATA fd;
    tcursnum   Curs = open_working_cursor(&replication_cd, "SELECT * FROM _sysext._packettab ORDER BY DestServObj, PacketType, PacketNO DESC", &cd);
    if (Curs == NOCURSOR) 
        return;
    for (r = 0; r < cd->recnum; r++)
    {
        WORD    ds;
        int     pt;
        BOOL    Del = FALSE;
        cd->curs_seek(r, &tr);
        if (tb_read_atr(&replication_cd, pcktab_descr, tr, PT_DESTOBJ, (char *)&ds))
            break;
        if (tb_read_atr(&replication_cd, pcktab_descr, tr, PT_PKTTYPE, (char *)&pt))
            break;
        if (tb_read_atr(&replication_cd, pcktab_descr, tr, PT_FNAME, fName))
            break;
        if (ds != DestServ)
        {
            DestServ   = ds;
            PacketType = 0;
        }
        // Od kazdeho typu paketu nechat pouze posledni verzi, ostatni a vsechny dotazy na stav muzeme smazat
        if (pt != PacketType && pt != PKTTP_STATE_REQ)
        {
            PacketType = pt;
            // IF soubor neexistuje, muzeme smazat zaznam
            HANDLE hFF = FindFirstFile(fName, &fd);
            if (hFF == INVALID_HANDLE_VALUE)
                Del = TRUE;
            else
                FindClose(hFF);
        }
        else
        {
            DeleteFile(fName);
            Del = TRUE;
        }
        if (Del)
        {
            if (!tb_del(&replication_cd, pcktab_descr, tr))
                commit(&replication_cd);
        }
    }
    commit(&replication_cd);

    // Prohledat vystupni adresar a smazat vsechny soubory, ktere nejsou v _PACKETTAB a nejsou to nepotvrzene
    // replikacni pakety
    if (*output_queue == 0)
        goto exit;
    lstrcpy(fName, output_queue);
    lstrcat(fName, PATH_SEPARATOR_STR "*.*");
    HANDLE hFF;  hFF = FindFirstFile(fName, &fd);
    if (hFF == INVALID_HANDLE_VALUE)
        goto exit;
    do
    {
        if (*fd.cFileName == '.')
            continue;
        for (r = 0; r < cd->recnum; r++)
        {
            cd->curs_seek(r, &tr);
            if (tb_read_atr(&replication_cd, pcktab_descr, tr, PT_FNAME, fName))
                break;
            char *p = strrchr(fName, PATH_SEPARATOR);
            if (!p)
                continue;
            if (stricmp(p + 1, fd.cFileName) == 0)
                break;
        }
        if (r >= cd->recnum)
        {
            lstrcpy(fName, output_queue);
            char *p = fName + lstrlen(fName);
            *p++ = PATH_SEPARATOR;
            lstrcpy(p, fd.cFileName);
            FHANDLE hFile = CreateFile(fName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
            if (!FILE_HANDLE_VALID(hFile))
                continue;

            t_packet_header Packet;
            tobjnum         DestServ;
            DWORD           Size;
            DWORD           PacketNO;
            BOOL            Del = FALSE;
            if (ReadFile(hFile, &Packet, sizeof(t_packet_header), &Size, NULL) && Size == sizeof(t_packet_header))
            {
                // Cizi soubory, pripadne pakety starsi verze muzeme smazat
                if (Packet.m_version != CURR_PACKET_VERSION)
                    Del = TRUE;
                // Jine nez replikacni data muzeme smazat
                else if (Packet.m_packet_type != PKTTP_REPL_DATA)
                    Del = TRUE;
                // Poskozene datove pakety muzeme smazat
                else if (!ReadFile(hFile, &PacketNO, 4, &Size, NULL) || Size != 4)
                    Del = TRUE;
                else if ((DestServ = srvs.server_id2obj(&replication_cd, Packet.m_dest_server)) == NOOBJECT)
                    Del = TRUE;
                else
                {
                    DWORD SndNO;
                    bool  Ack;
                    if 
                    (
                        tb_read_atr(&replication_cd, srvtab_descr, DestServ, SRV_ATR_SENDNUM, (char *)&SndNO) ||
                        tb_read_atr(&replication_cd, srvtab_descr, DestServ, SRV_ATR_ACKN,    (char *)&Ack)
                    )
                        continue;
                    // Pokud se nejedna o odeslany a nepotvrzeny, muzeme smazat
                    if (PacketNO != SndNO || Ack == wbtrue)
                        Del = TRUE;
                }
            }
            CloseFile(hFile);
            if (Del)
                DeleteFile(fName);
        }
    }
    while (FindNextFile(hFF, &fd));

exit:

    free_cursor(&replication_cd, Curs);
    free_deleted(&replication_cd, pcktab_descr);
    commit(&replication_cd);
}
//
// Testuje, zda je vstupni a vystupni fronta na diskete
//
BOOL t_replication::OnFlopy(char *Path)
{
#ifdef WINS
    if (Path[1] == ':')
    {
        char c  = Path[3];
        Path[3] = 0;
        UINT dt = GetDriveType(Path);
        Path[3] = c;
        return(dt == DRIVE_REMOVABLE);
    }
#endif
    return(FALSE);
}
//
// work_thread - zpracovava pozadavky na replikace ven a dovnitr
//
THREAD_PROC(work_thread)  // joinable
{ 
#ifdef NETWARE
  RenameThread(GetThreadID(), "Replication worker thread");
#endif
  t_replication * repl_info = (t_replication*)data;
  cd_t cd(PT_WORKER);  cdp_t cdp = &cd;
  repl_info->repl_thread_count++;
  int res = interf_init(cdp);
  if (res == KSE_OK)
  { integrity_check_planning.system_worker_thread_start();
    cdp->mask_token=wbtrue;
    cdp->ker_flags |= KFL_DISABLE_REFINT | KFL_DISABLE_INTEGRITY;   // nikde se neschazuje, ale asi to nevadi, protoze work_thread nikde 
    repl_info->work_thread(cdp);                                    // nepouziva referencni integritu ani integritni omezeni 
    kernel_cdp_free(cdp);
    integrity_check_planning.thread_operation_stopped();
    cd_interf_close(cdp);
  }
  else 
    client_start_error(cdp, workerThreadStart, res);
  repl_info->repl_thread_count--;
  THREAD_RETURN;
}


void t_replication::work_thread(cdp_t cdp)
// uses own cdp initialized by the thread
{ cdp->wait_type=WAIT_SEMAPHORE;
  for (;;)
  {
    integrity_check_planning.thread_operation_stopped();
    DWORD Res = WaitSemaph(&work_waiting, INFINITE);
    integrity_check_planning.continuing_replication_operation();
    if (Res == WAIT_ABANDONED)
      break; 
    if (repl_shutdown)
      break;
    cdp->set_return_code(ANS_OK);
    cdp->wait_type=WAIT_NO;
    char pathname[MAX_PATH];
    // find server with any work:
    bool any_work;
    for (;;) // cycle until no work
    { any_work=false;  t_server_work *server;
      // IF je prace pro nejaky server
      EnterCriticalSection(&cs_replica);
      for (server = srvs.m_Server; server; server = server->next)
      { if (server->proc_state == ps_prepared)
        { server->proc_state = ps_working;
          any_work = true;
          break;
        }
      }
      LeaveCriticalSection(&cs_replica);
      if (!any_work)
          break;
      // incoming replication packet:
      if (*server->packet_fname)
      { lstrcpy(pathname, input_queue);
        lstrcat(pathname, PATH_SEPARATOR_STR);
        lstrcat(pathname, server->packet_fname);
        FHANDLE fhnd=CreateFile(pathname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        BOOL proc=TRUE;
        if (FILE_HANDLE_VALID(fhnd))
        { t_in_packet packet(cdp, fhnd);  // all t_in_packet processing uses the cdp of the working thread
          DWORD rd=0;                   
          if (ReadFile(fhnd, (t_packet_header *)&packet, sizeof(t_packet_header), &rd, NULL) && rd==sizeof(t_packet_header))
            proc = packet.process_input_packet();
          CloseFile(fhnd);
          if (proc) 
            DeleteFile(pathname);  // file may not be deleted if it is being scanned, but it will be deleted later
        }
        *server->packet_fname=0;
      }

      // incoming state request:
      if (*server->statereq_fname)
      { lstrcpy(pathname, input_queue);  
        lstrcat(pathname, PATH_SEPARATOR_STR);
        lstrcat(pathname, server->statereq_fname);
        FHANDLE fhnd=CreateFile(pathname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        BOOL proc=TRUE;
        if (FILE_HANDLE_VALID(fhnd))
        { t_in_packet packet(cdp, fhnd);  // all t_in_packet processing uses the cdp of the working thread
          DWORD rd=0;
          if (ReadFile(fhnd, (t_packet_header *)&packet, sizeof(t_packet_header), &rd, NULL) && rd==sizeof(t_packet_header))
            proc = packet.process_input_packet();
          CloseFile(fhnd);
          if (proc)
            DeleteFile(pathname);  // file may not be deleted if it is being scanned, but it will be deleted later
        }
        *server->statereq_fname=0;
      }
      // outgoing replication:
      if (server->apl_list.count())
      {
          t_out_packet packet(cdp, server->server_id, server->srvr_obj);
          packet.send_data(server);
      }
      server->proc_state = ps_nothing;
    }
    cdp->wait_type=WAIT_SEMAPHORE;
  } // new waiting for a work
  cdp->wait_type=WAIT_TERMINATING;
}
//
// Replikace ven
//
void t_replication::scan_replication_plans(void)
// Finds servers and appls for which replication packet should be created.
// Checks if they are not blocked by GCR_LIMIT=6.0xff.
// Inserts them into srvs and sets proc_state to ps_prepared.
{ char query[200];  int len;
  lstrcpy(query, "SELECT * FROM REPLTAB WHERE GCR_LIMIT<>X'ffffffffffff' AND TABNAME='' AND SOURCE_UUID=X'");
  len = 88;
  bin2hex(query+len, my_server_id, UUID_SIZE);                     len += 2 * UUID_SIZE;
  lstrcpy(query+len, "' AND CAST(NEXT_REPL_TIME AS TIMESTAMP)<="); len += 41;
  datim2str(stamp_now(), query + len, 5);                          len += lstrlen(query + len);
  lstrcpy(query+len, " ORDER BY DEST_UUID");
  srvs.add_appls(&replication_cd, query);
}
//
// Stores request for replicating the application sent by client or other server.
//
// pull_req = 1 - z protejsiho serveru / = 0 - z klienta
//
unsigned t_replctx :: replicate_appl(WBUUID dest_server_id, t_pull_req *pull_req, CPcktSndd *PacketSended)
{   
    int err = 0;
    // check communication state to the server:
    m_SrvrObj = srvs.server_id2obj(m_cdp, dest_server_id);
    if (m_SrvrObj == NOOBJECT)
    { 
        *m_cdp->errmsg = 0;
        err = OBJECT_NOT_FOUND;
        goto exit;
    }
    wbbool ackn;
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_ACKN, (tptr)&ackn);
    if (ackn != wbtrue)  // last packet not acknowledged, cannot create a new one
    { 
        t_server_work *server = srvs.get_server(m_cdp, dest_server_id);
        if (server->last_state_request_send_time == 0 ||
            GetTickDifference(GetTickCount(), server->last_state_request_send_time) > STATE_REQUEST_REPEAT_PERIOD || pull_req) // state not requested recently
        {   char msg[64];
            Get_server_error_text(m_cdp, WAITING_FOR_ACKN, msg, sizeof(msg));  // is translated
            trace_msg_general(m_cdp, TRACE_REPLICATION, msg, NOOBJECT);
            t_out_packet packet(m_cdp, dest_server_id, m_SrvrObj);
            if (packet.send_state_request() && !pull_req)
                server->last_state_request_send_time = GetTickCount();
        }
        err = WAITING_FOR_ACKN;
        goto exit;
    }
    // find replication info for the application:
    BOOL new_prepared;
    new_prepared = FALSE;
    trecnum trec;
    trec = find_appl_replrec(my_server_id, dest_server_id, NULL);
    if (trec != NORECNUM)
    { 
        t_zcr gcr_val;
        tb_read_atr(m_cdp, rpltab_descr, trec, REPL_ATR_GCRLIMIT, (tptr)gcr_val);
        int i;
        for (i = 0; i < ZCR_SIZE; i++) if (gcr_val[i] != 0xff) break;
        if (i == ZCR_SIZE) // all 0xff, blocked
        {
            err = REPL_BLOCKED;
            goto exit;
        }
        new_prepared = srvs.add_appl(m_cdp, dest_server_id, m_ApplID, trec, pull_req, PacketSended);
        if (new_prepared < 0)
        {
            err = OUT_OF_KERNEL_MEMORY;
            new_prepared = 0;
        }
    }
    if (new_prepared)
    {
        the_replication->ReleaseWorkThread();
    }
    else
    {
#ifdef WINS
        DialReq = FALSE;
#endif
        err = NOTHING_TO_REPL;
    }

exit:
    // IF pozadavek z protejsiho serveru a nebyl sestaven paket, odeslat PKTTP_NOTH_TOREPL
    if (pull_req && err)
    {
        t_out_packet packet(m_cdp, dest_server_id, m_SrvrObj);
        packet.send_nothtorepl(err);
    }
    return(err);
}
//
// Replikace dovnitr
//
void t_replication :: scan_input_queue(void) // called by watch_thread only
// Finds input packets for the current server and inserts them into srvs
// unless any input processing for the source server is in progress.
{ 
#ifdef WINS
    if (!DialRepl || DialReq)
    { 
        if (MailRepl)
            mail_receive();
        if (transporter_port && *DIPDConn)
        { 
            DIPTimeout = 6;
            start_transporter(&replication_cd);
        }
    }
#else
    if (MailRepl)
        mail_receive();
#endif
    int   work_waiting_count = 0;
    char  pathname[MAX_PATH];
    WIN32_FIND_DATA wfd;
    HANDLE fh;
    if (*shared_dir && lstrcmpi(shared_dir, input_queue) != 0)
    {
        char Shared[MAX_PATH];
        lstrcpy(Shared, shared_dir);  
        char *s = Shared + lstrlen(Shared) + 1;
        lstrcat(Shared, PATH_SEPARATOR_STR "WRP*.*");
        HANDLE fh = FindFirstFile(Shared, &wfd);
        if (fh != INVALID_HANDLE_VALUE)
        {
            lstrcpy(pathname, input_queue);
            lstrcat(pathname, PATH_SEPARATOR_STR);
            char *d = pathname + lstrlen(pathname);
            do
            {
                lstrcpy(s, wfd.cFileName);
                lstrcpy(d, wfd.cFileName);
                if (!MoveFile(Shared, pathname))
                {
                    if (CopyFile(s, d, FALSE))
                        DeleteFile(Shared);
                    else
                        monitor_msg(&replication_cd, RE18, Errno);
                }
            }
            while (FindNextFile(fh, &wfd));
            FindClose(fh);
        }
    }
    lstrcpy(pathname, input_queue);  
    lstrcat(pathname, PATH_SEPARATOR_STR "WRP*.*");
    fh = FindFirstFile(pathname, &wfd);
    if (fh == INVALID_HANDLE_VALUE)
        return;
    do  // cycle on files in the input queue
    { 
        lstrcpy(pathname + lstrlen(input_queue) + 1, wfd.cFileName);
        FHANDLE fhnd = CreateFile(pathname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (!FILE_HANDLE_VALID(fhnd))
            continue;
        t_in_packet packet(&replication_cd, fhnd);
        DWORD rd = 0;
        ReadFile(fhnd, (t_packet_header *)&packet, sizeof(t_packet_header), &rd, NULL);
        // IF aktualni verze, pro me nebo zadost o serverove info
        if 
        (
            rd == sizeof(t_packet_header) && packet.m_version == CURR_PACKET_VERSION && 
            (!memcmp(packet.m_dest_server, my_server_id, sizeof(WBUUID)) || packet.m_packet_type == PKTTP_SERVER_REQ)
        )
        {
            // IF od znameho serveru nebo serverove info
            t_server_work *server = srvs.get_server(&replication_cd, packet.m_source_server);
            if (server || packet.m_packet_type == PKTTP_SERVER_REQ || packet.m_packet_type == PKTTP_SERVER_INFO)
            {
                // IF data nebo dotaz na stav, predat ke zpracovani work threadu
                if (packet.m_packet_type == PKTTP_REPL_DATA || packet.m_packet_type == PKTTP_STATE_REQ)
                {
                    CloseFile(fhnd);
                    if (server->AddPacket(wfd.cFileName, packet.m_packet_type == PKTTP_REPL_DATA))
                        work_waiting_count++;
                }
                else // Jinak zpracovat hned
                {
                    BOOL proc = packet.process_input_packet();
                    CloseFile(fhnd);
                    if (proc)
                        DeleteFile(pathname);
                }
                continue;
            }
        }
        CloseFile(fhnd);        // close unless closed before
        DeleteFile(pathname);     // poskozene, neplatne, nebo pakety pro jiny server muzeme smazat
    } 
    while (FindNextFile(fh, &wfd));
    FindClose(fh);
    ReleaseSemaph(&work_waiting, work_waiting_count);
}
//
// Spusteni DirectIP nebo Mail vlakna v dusledku pozadavku na odeslani paketu
//
void t_replication :: StartDispatchThread(cdp_t cdp, BOOL DirectIP)
{
    if (DirectIP) // has a IP address
    { 
#ifdef WINS
        if (*DIPDConn && DialReq)
        {
            DIPTimeout = 2;
            start_transporter(cdp);
        }
#endif
        SetLocalAutoEvent(&hTransportEvent); 
    }
    else
    { 
#ifdef WINS
        if (!DialRepl || DialReq)
#endif
        {
            EnterCriticalSection(&cs_mail);
            SetLocalAutoEvent(&hMailReqEvent);
            //DbgLogFile("hMailReqEvent mail_send IN %08X", hMailThread);
            if (!hMailThread)
                THREAD_START(::MailThread, THREAD_STACK, this, &hMailThread);
            //DbgLogFile("mail_send OUT");
            LeaveCriticalSection(&cs_mail);
        }
    }
}
//
// Spusteni Mail vlakna kvuli prijmu posty
//
void t_replication::mail_receive(void)
{ replication_cd.wait_type=WAIT_RECEIVING;
  //DbgLogFile("Recievemail");
  if (RcvReq)
    return;
  EnterCriticalSection(&cs_mail);
  RcvReq = TRUE;
  SetLocalAutoEvent(&hMailReqEvent);
  //DbgLogFile("hMailReqEvent mail_receive IN %08X", hMailThread);
  if (!hMailThread)
      THREAD_START(::MailThread, THREAD_STACK, this, &hMailThread);  
  //DbgLogFile("mail_receive OUT");
  LeaveCriticalSection(&cs_mail);
  replication_cd.wait_type=WAIT_NO;
}
//
// MailThread - Prijem a odesilani posty
//
THREAD_PROC(MailThread)
{ cd_t mail_cd(PT_WORKER);
#ifdef NETWARE
  RenameThread(GetThreadID(), "Mail replication thread");
#endif
  t_replication * repl_info = (t_replication*)data;
  repl_info->repl_thread_count++;
  int res = interf_init(&mail_cd);
  if (res == KSE_OK)
  { integrity_check_planning.system_worker_thread_start();
    repl_info->MailThread(&mail_cd);
    kernel_cdp_free(&mail_cd);
    integrity_check_planning.thread_operation_stopped();
    cd_interf_close(&mail_cd);
  }
  else 
    client_start_error(&mail_cd, workerThreadStart, res);
  repl_info->repl_thread_count--;
  THREAD_RETURN;
}

char WB602RP[] = "WB602 Replication Packet";

void t_replication::MailThread(cdp_t cdp)
{ int   tmoRcv;
  BOOL  timeout = TRUE;
  tcursnum Curs = NOCURSOR;
  DWORD Err = NOERROR;
  // Nacist server_info
  t_my_server_info lsrv;  
#ifndef WINS
  integrity_check_planning.thread_operation_stopped();
  WaitLocalAutoEvent(&hMailWorking, INFINITE);
  integrity_check_planning.continuing_replication_operation();
#endif
  //DbgLogWrite("MailStart");
  integrity_check_planning.thread_operation_stopped();
  WaitLocalAutoEvent(&hMailReqEvent, 0);
  integrity_check_planning.continuing_replication_operation();
  memset(&lsrv, 0, sizeof(t_my_server_info));
  if (tb_read_var(cdp, srvtab_descr, 0, SRV_ATR_APPLS, 0, sizeof(t_my_server_info), (tptr)&lsrv))
  {
    goto exit;
  }

  char *Msg;
  Msg = RE37;
#ifdef WINS
  BOOL DialUp;
  DialUp = FALSE;
  // Inicializovat postu
  //DbgLogFile("Repl Init IN %08X", mcdp);
  if (*lsrv.emi_path)
  {
    Err = cd_InitWBMail602(cdp, lsrv.emi_path, lsrv.mail_id, lsrv.password);
  }
  else
  {
    Err = cd_InitWBMail(cdp, lsrv.profile, lsrv.mailpword);
  }
  //DbgLogFile("Repl Init OUT");
  if (!Err)
  {
    // IF dialup posta, vytocit spojeni
    DWORD Type = cd_MailGetType(cdp);
    DialUp = Type & WBMT_DIALUP;
    Remote = Type & WBMT_602REM;
    if (DialUp)
    { // IF jeste nevytocila ani posta ani DirectIP
      DialReq  = FALSE;
      if (!MailDialed && !DIPDialed)
        monitor_msg(cdp, RE26);
      Msg = RE45;
      Err = cd_MailDial(cdp, lsrv.dialpword);
      if (!Err)
      { MailDialed = TRUE;  
        // IF uz volame, stahnout taky doslou postu
        RcvReq = TRUE; 
        if (!DIPDialed)
          monitor_msg(cdp, RE35);
      }
    }
  }
#else
  Err = cd_InitWBMail(cdp, lsrv.profile, lsrv.mailpword);
#endif

  if (Err)
  { monitor_msg(cdp, Msg, Err);
    goto exit;
  }

  tmoRcv  = GetTickCount() + 60 * 60000;

  do
  {
    // WHILE neco ve fronte nebo uz dlouho se nekontrolovala dosla posta
    cdp->set_return_code(ANS_OK);
    if (!RcvReq)
        RcvReq = GetTickCount() > tmoRcv;

    char Address[MAX_PATH];
    char fName[MAX_PATH];
    char *Addr, *Type;
    uns32 PID;
    for (PID = 0; RcvReq || GetPacketForMail(cdp, Address, &Addr, &Type, fName, &PID);)
    {
      // IF pozadavek na prijem
      if (RcvReq)
      {
        // IF neco prislo, precist, pustit watch_thred, chvili pockat na dalsi pozadavek
        //DbgLogWrite("Mail Prijiam");
        if (Receive(cdp))
        { 
          Sleep(0);
          timeout = FALSE;
        }
        //DbgLogWrite("Mail Doprijimano");
        // Dalsi prijem za 5 minut
        tmoRcv  = GetTickCount() + 5 * 60000;
        RcvReq  = FALSE;
      }
      else
      {
        // Odeslat zasilku a pockat chvilku na odpovd
        //DbgLogWrite("Mail Vysilam");
        Send(cdp, Addr, Type, fName, PID);
        timeout = FALSE;
        PID++;
      }
      if (repl_shutdown)
        break;
    }
    // EXIT IF Prazdna fronta and Timeout
    if (timeout)
      break;
    timeout = TRUE;
    //DbgLogFile("hMailReqEvent Wait");
    integrity_check_planning.thread_operation_stopped();
    WaitLocalAutoEvent(&hMailReqEvent, 20000);
    integrity_check_planning.continuing_replication_operation();
  }
  while (!repl_shutdown);

  //DbgLogWrite("Mail Dovysilano");
#ifdef WINS
  if (DialUp)
  {
    cd_MailHangUp(cdp);
    MailDialed = FALSE;
    if (!DIPDialed)
       monitor_msg(cdp, RE36);
  }
#endif
  cd_CloseWBMail(cdp);

exit:

  PackPackTab(cdp);
  //DbgLogFile("hMailThread = NULL");
  hMailThread = (THREAD_HANDLE)NULL;
#ifndef WINS
  SetLocalAutoEvent(&hMailWorking);
  //DbgLogWrite("MailStop");
#endif
  RcvReq      = FALSE;
}
//
// Prijem posty
//
char MBQuery[] = "SELECT * FROM %s WHERE Stat<>Chr(2) AND Subject=\"WB602 Replication Packet\" ORDER BY RcvDate, RcvTime";

UINT t_replication::Receive(cdp_t cdp)  // called by mail thread only
{
    UINT     cMsgs = 0;
    tcursnum Curs  = NOCURSOR;
    // Otevrit MailBox
#ifdef WINS
    if (MailDialed || DIPDialed)
        monitor_msg(cdp, RE43);
#endif
    HANDLE MailBox = 0;
    DWORD Err = cd_MailOpenInBox(cdp, &MailBox);
    if (Err)
        goto exit;
    Err = cd_MailBoxLoad(cdp, MailBox, Remote ? MBL_BODY | MBL_FILES : 0);
    if (Err)                                    
        goto exit;
    
    // Vybrat replikacni pakety
    char Query[180];
    char mTblName[64];
    ttablenum mTblNum;
    
    cd_MailGetInBoxInfo(cdp, MailBox, mTblName, &mTblNum, NULL, NULL);
    sprintf(Query, MBQuery, mTblName);
    cur_descr *cd;
    Curs  = open_working_cursor(cdp, Query, &cd);
    cMsgs = cd->recnum;
    //DbgLogFile("cMsgs = %d", cMsgs);
    // Pres vsechny replikacni pakety
    UINT r;
    for (r=0; r < cMsgs; r++)
    { 
        // Zjistit ID zasilky
        DWORD ID;
        trecnum tr;
        cd->curs_seek(r, &tr);
        if (tb_read_atr(cdp, mTblNum, tr, ATML_ID, (char *)&ID))
            goto exit;
        // Vybrat vsechny soubory
        char destfile[MAX_PATH];
        GetTempFileName(input_queue, "WRM", 0, destfile);
        //DbgLogFile("Mail %s", destfile);
        Err = cd_MailBoxSaveFileAs(cdp, MailBox, ID, 0, NULL, destfile);
        if (Err)
        { 
            monitor_msg(cdp, RE44, Err);
            Err = 0;
            continue;
        }
        //DbgLogFile("Mail Loaded");
        // IF prislo serverove info, nastavit priznak v postovni adrese serveru
        FHANDLE hFile;
        hFile = CreateFile(destfile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (FILE_HANDLE_VALID(hFile))
        {
            char buf[sizeof(t_packet_header) + sizeof(t_server_pack)];
            DWORD rd;
            ReadFile(hFile, buf, sizeof(buf), &rd, NULL);
            //DbgLogFile("Packet type %d", ((t_packet_header*)buf)->m_packet_type);
            if (((t_packet_header*)buf)->m_packet_type==PKTTP_SERVER_REQ || ((t_packet_header*)buf)->m_packet_type==PKTTP_SERVER_INFO)
            {
                char *ip = (char *)((t_server_pack *)(buf+sizeof(t_packet_header)))->addr1;
                if (strncmp(ip + 1, DIRECTIP, 9) == 0 || strncmp(ip + 1, INPUTDIR, 15) == 0)
                    ip   = (char *)((t_server_pack *)(buf+sizeof(t_packet_header)))->addr2;
                *(uns32 *)(ip + lstrlen(ip) + 1) = 0xCCCCCCCC; // Packet prisel postou
                SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                WriteFile(hFile, buf, sizeof(buf), &rd, NULL);
            }
            CloseFile(hFile);
        }
        char packfile[MAX_PATH];
        GetTempFileName(input_queue, "WRP", 0, packfile);        
        if (!MoveFile(destfile, packfile)) // must copy, name conflict:
        { if (!CopyFile(destfile, packfile, FALSE))
            monitor_msg(cdp, RE20);
          DeleteFile(destfile);
        }
        SetLocalAutoEvent(&hReplStopEvent);
        // Smazat zasilku i se zaznamem v tabulce
        cd_MailBoxDeleteMsg(cdp, MailBox, ID, TRUE);
    }
    
exit:

  if (Curs != NOCURSOR)
      free_cursor(cdp, Curs);
  if (MailBox)
      cd_MailCloseInBox(cdp, MailBox);
  if (Err && Err != MAIL_BOX_LOCKED && Err != MAIL_ALREADY_INIT)
    monitor_msg(cdp, RE44, Err);
#ifdef WINS
  else if ((MailDialed || DIPDialed) && !cMsgs)
    monitor_msg(cdp, RE48);
#endif
  return(cMsgs);
}
//
// Odeslani paketu
//
void t_replication::Send(cdp_t cdp, char *Addr, char *Type, char *fName, DWORD PID)  // called by mail thread only
{
    HANDLE Letter;
    DWORD Err = cd_LetterCreate(cdp, WB602RP, NULL, WBL_REMSENDNOW | WBL_DELAFTER, &Letter);
    if (Err)
        goto exit;
    Err = cd_LetterAddAddr(cdp, Letter, Addr, Type, FALSE);
    if (Err)
    {
        cd_LetterCancel(cdp, Letter);
        goto exit;
    }
    Err = cd_LetterAddFile(cdp, Letter, fName);
    if (Err)
    {
        cd_LetterCancel(cdp, Letter);
        goto exit;
    }
    Err = cd_LetterSend(cdp, Letter);

exit:

    SendDone(cdp, PID, fName, DIR_MAIL, Err);
}
//
// Vraci informace o paketu k odeslani postou
//
static char SelAdr[]  = "SELECT * FROM _SYSEXT._PACKETTAB WHERE ID>=%d ORDER BY ID";

BOOL t_replication::GetPacketForMail(cdp_t cdp, char *Address, char **Addr, char **Type, char *fName, uns32 *PID)
{
    EnterCriticalSection(&cs_mail);
    cur_descr *cd;
    BOOL Found = FALSE;
    char Sel[70];
    sprintf(Sel, SelAdr, *PID);
    tcursnum Curs = open_working_cursor(cdp, Sel, &cd);
    if (Curs != NOCURSOR)
    {
        trecnum r, tr;
        for (r = 0; r < cd->recnum; r++)
        {
            cd->curs_seek(r, &tr);
            GetAddress(cdp, tr, Address, Addr, Type);
            if (lstrcmp(*Type, DIRECTIP) == 0)
                continue;
            tb_read_atr(cdp, pcktab_descr, tr, PT_FNAME, fName);
            tb_read_atr(cdp, pcktab_descr, tr, PT_ID,(tptr)PID);
            Found = TRUE;
            break;
        }
        free_cursor(cdp, Curs);
    }
    LeaveCriticalSection(&cs_mail);
    return(Found);
}
//
// DirectIP
//
// Spusti vlakna DirectIP prenosu
//
static void start_transporter(cdp_t cdp)
{ if (!transporter_running)
  { trans_lsn_sock= socket(AF_INET, SOCK_STREAM, 0);
#ifdef WINS
    if (trans_lsn_sock == -1) 
    { if (SockErrno == WSANOTINITIALISED)
      { WSADATA wsa;
        if (WSAStartup(0x0101, &wsa) == 0) // WINSOCK 1.1
        { DIPWSA = TRUE;
          trans_lsn_sock= socket(AF_INET, SOCK_STREAM, 0);
        }
      }
    }
#endif
	if (trans_lsn_sock == -1) 
      monitor_msg(cdp, RE40); 
    else
    {
#ifdef WINS
      // IF vytacene DirectIP
      if (*DIPDConn)
      { // IF jeste nevytacelo DirerectIP ani posta
        if (!DIPDialed && !MailDialed)
        { if (DIPDial(cdp))
            return;
          DIPDialed = TRUE;
        }
      }
#endif
      THREAD_START_JOINABLE(::transport_receiver, 10000, 0, &trans_receiver);
      THREAD_START_JOINABLE(::transport_sender,   10000, 0, &trans_sender);
#ifdef WINS
      CreateLocalAutoEvent(FALSE, &hTransportEvent);
#endif
      //transporter_running=TRUE;
      monitor_msg(cdp, RE41); 
    }
  }
}

#ifdef WINS
//
// Vytaceni kvuli directIP
//
extern CDialCtx DialCtx;

DWORD DIPDial(cdp_t cdp)
{
    DialReq = FALSE;
    if (DIPhDialConn)
        return(NOERROR);
    if (!DIPDialed && !MailDialed)
        monitor_msg(cdp, RE26);
    DWORD Err = DialCtx.RasLoad();
    if (Err)
        return(Err);
    Err = DialCtx.Dial(DIPDConn, DIPDUser, DIPDPword, &DIPhDialConn);
    if (Err)
        goto error;
    if (!DIPDialed && !MailDialed)
    {
        monitor_msg(cdp, RE35);
        DIPDialed = TRUE;
    }
    return(NOERROR);

error:
    monitor_msg(cdp, RE46, Err);
    return(Err);
}

DWORD DIPHangUp(cdp_t cdp)
{ 
    DWORD Err = NOERROR;
    // IF moje spojeni
    if (DIPhDialConn)
    {
        DWORD Err = DialCtx.HangUp(DIPhDialConn);
        DIPhDialConn = NULL;
    }
    DIPDialed = FALSE;
    if (!MailDialed)
        monitor_msg(cdp, RE36);
    DialCtx.RasFree();
    return(Err);
}

#endif // WINS
//
// DirectIP Vysilani
//
#define TRANSPORT_HEADER_MAGIC 0x3ef04d36

THREAD_PROC(transport_sender)
{ BOOL anythig_sended;
#ifdef NETWARE
  RenameThread(GetThreadID(), "IP Transport sender");
#endif
  cd_t cd(PT_WORKER);  cdp_t cdp = &cd;
  int res = interf_init(cdp);
  if (res != KSE_OK)
  {
    client_start_error(cdp, workerThreadStart, res);
    THREAD_RETURN;
  }
  integrity_check_planning.system_worker_thread_start();
  CIPSocket *sock = new CIPSocket();
  if (!sock)
    goto exit;

  DWORD FirstFail; FirstFail = 0;
  transporter_running|=1;
  DWORD fixed_addr; fixed_addr = 0;
  do
  { cdp->set_return_code(ANS_OK);
    anythig_sended=FALSE;  
#ifdef WINS
    ip_change_check(cdp);
#endif
    if (preferred_send_ip_state==1) preferred_send_ip_state=2;
    char pathname[MAX_PATH];
    uns32 PID;
    for (PID = 0; !repl_shutdown && GetPacketForDIP(cdp, sock, preferred_send_ip_state==2 ? preferred_send_ip : 0, pathname, &PID); PID++)
    { // cycle on files in the input queue
      //DbgLogFile("    IP State CCL %d  Packet %d.%d.%d.%d:%d", preferred_send_ip_state, HIBYTE(HIWORD(ip)),LOBYTE(HIWORD(ip)),HIBYTE(LOWORD(ip)),LOBYTE(LOWORD(ip)), port);
      if (preferred_send_ip_state==1) break;  // stop normal operation
      // connect:
      int err; BOOL connected=FALSE;
      unsigned own_connection_ip=0;
      ip_transport_counter++;  
#ifdef WINS
      WriteSlaveThreadCounter();
#endif
      err = sock->Socket();
      //DbgLogFile("socket(%d) = %d", sock->GetSock(), err);
      if (!err)	
      { if (is_trace_enabled_global(TRACE_DIRECT_IP))
        { char text[300];
          form_message(text, sizeof(text), RE16, pathname, sock->GetIP() & 0xFF, (sock->GetIP() >> 8) & 0xFF,(sock->GetIP() >> 16) & 0xFF,(sock->GetIP() >> 24) & 0xFF, ntohs(sock->GetPort()));
          trace_msg_general(cdp, TRACE_DIRECT_IP, text, NOOBJECT);
        }
        //DbgLogWrite("DIP Vysilam");
	    err = sock->Connect();
        if (err)
        { if (is_trace_enabled_global(TRACE_DIRECT_IP))
          { char text[50];
            form_message(text, sizeof(text), RE17, err);
            trace_msg_general(cdp, TRACE_DIRECT_IP, text, NOOBJECT);
          }
          //DbgLogFile("    connect(%d.%d.%d.%d:%d) = %d", sock->GetIP() & 0xFF, (sock->GetIP() >> 8) & 0xFF,(sock->GetIP() >> 16) & 0xFF,(sock->GetIP() >> 24) & 0xFF, ntohs(sock->GetPort()), err);
          DWORD ct = GetTickCount();
          if (!FirstFail)
            FirstFail = ct;
          else if (ct > FirstFail + 60 * 60 * 1000)
          { monitor_msg(cdp, RE47);
            FirstFail = ct;
          }
#ifdef DEBUG
          DWORD opttcp;
          DWORD oprsock;
          int sz = 4;
          getsockopt(*sock, IPPROTO_TCP, SO_ERROR, (char *)&opttcp, &sz);
          getsockopt(*sock, SOL_SOCKET , SO_ERROR, (char *)&oprsock, &sz);
          //DbgLogFile("    timeout %d %d", opttcp, oprsock);
#endif
        }
	    else
        { connected=TRUE;
          if (!GetDatabaseString(server_name, "FIXED_IP_ADDR", (char*)&fixed_addr, sizeof(fixed_addr), my_private_server) || !fixed_addr)
          { sockaddr_in mysockAddr;  socklen_t adrsize = sizeof(mysockAddr);
            err = getsockname(*sock, (sockaddr*)&mysockAddr, &adrsize);
            if (!err) own_connection_ip=ntohl(mysockAddr.sin_addr.s_addr);
            else monitor_msg(cdp, "getsockname error %u.", err);
          }
        }
      }
      if (connected)
      { BOOL b;  b=TRUE;  FirstFail = 0;
        char *msg;
        FHANDLE fhnd;
        fhnd=CreateFile(pathname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        //DbgLogFile("    CreateFile = %d", fhnd);
        if (!FILE_HANDLE_VALID(fhnd))
        { err = Errno;
          msg = RE18;
        }
        else 
        { int size;  size=TRANSPORT_HEADER_MAGIC;  BOOL firstpart=TRUE;
#ifdef DEBUG
          DWORD tm = GetTickCount();
#endif
          err = sock->Send((char*)&size, sizeof(int), &size);
          if (err)
          {
            //DbgLogFile("    Send(TRANSPORT_HEADER_MAGIC) = %d", err);
            msg = RE54;
            goto stop;
          }
          size=GetFileSize(fhnd, NULL); 
#ifdef UNIX
          SetFilePointer(fhnd, 0, NULL, 0);
#endif
          err =sock->Send((char*)&size, sizeof(int), &size); 
          if (err)
          {
            //DbgLogFile("    Send(size) = %d", err);
            msg = RE54;
            goto stop;
          }
          DWORD rd;  
          do
          { char *buf = sock->GetBuf();
            if (!ReadFile(fhnd, buf, SOCKBUF_SIZE, &rd, NULL)) 
            {
              err = Errno;
              msg = RE18;
              //DbgLogFile("    Read = %d", err);
              goto stop;
            }
            if (firstpart && own_connection_ip && !fixed_addr)
            { firstpart=FALSE;
              if (((t_packet_header*)buf)->m_packet_type==PKTTP_SERVER_REQ || ((t_packet_header*)buf)->m_packet_type==PKTTP_SERVER_INFO)
              { 
                tptr ip = (tptr)((t_server_pack *)(buf+sizeof(t_packet_header)))->addr1;
                if (strncmp(ip + 1, DIRECTIP, 9))
                    ip  = (tptr)((t_server_pack *)(buf+sizeof(t_packet_header)))->addr2;
                replace_ip(cdp, ip, own_connection_ip);
              }
            }
            // send the buffer:
            err = sock->Write(buf, rd);
            if (err)
            {
              //DbgLogFile("    Send(data) = %d", err);
              msg = RE54;
              goto stop;
            }
          } while (rd==SOCKBUF_SIZE);
          anythig_sended=TRUE;
#ifdef DEBUG
          //DbgLogWrite("ReplSend = %d", GetTickCount() - tm);
#endif
          err=0;
#ifdef WINS
          DIPTimeout = 3;                   // Pockame jen chvilku na odpoved
#endif
          //DbgLogFile("    file_sended");
       stop:
          CloseFile(fhnd);
        }
        if (err && is_trace_enabled_global(TRACE_DIRECT_IP))
        { char text[50];
          form_message(text, sizeof(text), msg, err);
          trace_msg_general(cdp, TRACE_DIRECT_IP, text, NOOBJECT);
        }
        the_replication->SendDone(cdp, PID, pathname, DIR_DIRECTIP, err);

      } // connected
      sock->Close();
      //DbgLogFile("    closesocket %d", sock);
      ip_transport_counter--;  
#ifdef WINS
      WriteSlaveThreadCounter();
#endif
    }
    if (preferred_send_ip_state==2) preferred_send_ip_state=0;
#ifdef WINS 
    if (!anythig_sended)                    // IF v tomto kole nic neodeslo
    { if (*DIPDConn)                        // IF vytacene DirectIP
      { 
        //DbgLogWrite("DIPsTmo = %d", DIPTimeout);
        if (!DIPTimeout)                    // IF uz dlouho nic
        { 
          //DbgLogWrite("DIP Dovysilano");
          if (!DIPrWork)                    // IF DirectIP neprijima
          { closesocket(trans_lsn_sock);    // Koncim s prijmem
            trans_lsn_sock=-1;
            break;                          // Konec DirectIP
          }
        }
        else
          DIPTimeout--;                     // Dasli kolo
      }
      if (!repl_shutdown)
      {
        integrity_check_planning.thread_operation_stopped();
        WaitLocalAutoEvent(&hTransportEvent, 20000);
        integrity_check_planning.continuing_replication_operation();
      }
    }
#elif defined LINUX
    if (!anythig_sended)
    {
        integrity_check_planning.thread_operation_stopped();
        BOOL Res = Sleep_cond(20000, repl_shutdown);
        integrity_check_planning.continuing_replication_operation();
	    if (!Res)
            break;
    }
#endif
    else
       the_replication->PackPackTab(cdp);
  } while (!repl_shutdown);
#ifdef WINS
  if (DIPDialed)                            // IF dialed DirectIP, zavesit
    DIPHangUp(cdp);
#endif
  transporter_running&=~1;
  delete sock;
exit:
  //DbgLogFile("DIPSender Exit");
  kernel_cdp_free(cdp);
  integrity_check_planning.thread_operation_stopped();
  cd_interf_close(cdp);
  THREAD_RETURN;
}

#ifdef WINS
void ip_change_check(cdp_t cdp)
// Checks to see if own IP address changed: sends it to all remote servers if new address detected
{ unsigned dhcp_ip_addr, dhcp_ip_mask;  char buf[100];  uns32 fixed_addr;
  //DbgLogFile("Server:        %s", server_name);
  if (!GetDatabaseString(server_name, "FIXED_IP_ADDR", (char*)&fixed_addr, sizeof(fixed_addr), my_private_server)) fixed_addr=0;
  //DbgLogFile("FIXED_IP_ADDR: %d", fixed_addr);
  if (fixed_addr) return;
  if (!GetDatabaseString(server_name, "DHCP_IP_ADDR", buf, sizeof(buf), my_private_server)) *buf=0;
  //DbgLogFile("DHCP_IP_ADDR:  %s", buf);
  unsigned a1,a2,a3,a4;
  int flds = sscanf(buf, " %u . %u . %u . %u", &a1, &a2, &a3, &a4);
  dhcp_ip_addr = flds==4 ? (a1<<24) | (a2<<16) | (a3<<8) | a4 : 0;
  if (!GetDatabaseString(server_name, "DHCP_IP_MASK", buf, sizeof(buf), my_private_server)) *buf=0;
  //DbgLogFile("DHCP_IP_MASK:  %s", buf);
  flds = sscanf(buf, " %u . %u . %u . %u", &a1, &a2, &a3, &a4);
  dhcp_ip_mask = flds==4 ? (a1<<24) | (a2<<16) | (a3<<8) | a4 : 0;
 // get current IP address:
  gethostname(buf, sizeof(buf));
  hostent * he = gethostbyname(buf);
  if (he && he->h_addr_list) 
  { int i=0;  unsigned ip=0;
    while (he->h_addr_list[i]) 
    { unsigned an_ip = ntohl(*(uns32*)he->h_addr_list[i]);
      //DbgLogFile("Adresa%d:       %d.%d.%d.%d", i, (an_ip >> 24) & 0xFF, (an_ip >> 16) & 0xFF, (an_ip >> 8) & 0xFF, an_ip & 0xFF);
      if (an_ip && an_ip!=0x7f000001 && (an_ip & dhcp_ip_mask)==dhcp_ip_addr)
        ip=an_ip;
      i++;
    }
    if (ip) // have a real IP address, DHCP address preferred
    { char addr[MAX_PATH];  unsigned stored_ip, stored_transporter_port;  BOOL addr1=FALSE, addr2=FALSE;
      // read the stored address:
      tb_read_atr(cdp, srvtab_descr, 0, SRV_ATR_ADDR1, addr);
      if (get_ip_and_port(addr, &stored_ip, &stored_transporter_port))
        addr1=TRUE;
      else
      { tb_read_atr(cdp, srvtab_descr, 0, SRV_ATR_ADDR2, addr);
        if (get_ip_and_port(addr, &stored_ip, &stored_transporter_port))
          addr2=TRUE;
      }
      if (addr1 || addr2)
      { //DbgLogFile("stored_ip:     %d.%d.%d.%d", (stored_ip >> 24) & 0xFF, (stored_ip >> 16) & 0xFF, (stored_ip >> 8) & 0xFF, stored_ip & 0xFF);
        if (stored_ip!=ip)
        { struct in_addr ina;  ina.S_un.S_addr=htonl(ip);
          monitor_msg(cdp, "Globalne nahrazuji vlastni Direct IP adresu: %s:%u", inet_ntoa(ina), stored_transporter_port);
         // store the new address:
          sprintf(addr, "{Direct IP}%s:%u", inet_ntoa(ina), stored_transporter_port);
          tb_write_atr(cdp, srvtab_descr, 0, addr1 ? SRV_ATR_ADDR1 : SRV_ATR_ADDR2, addr);
          commit(cdp);
         // send info to replicating servers:
          trecnum r, cnt = srvtab_descr->Recnum();
          if (the_replication)
            for (r=1;  r<cnt;  r++)
              if (!table_record_state(cdp, srvtab_descr, r)) // not deleted
              {
                  WBUUID DestID;
                  tb_read_atr(cdp, srvtab_descr, r, SRV_ATR_UUID, (tptr)DestID);
                  t_out_packet packet(cdp, DestID, (tobjnum)r);
                  packet.send_server_info();
              }
        }
      } // own IP address stored
    } // have IP
  } // own IP determined
}
#endif

void replace_ip(cdp_t cdp, char * addr, unsigned own_ip)
{ char adrcpy[254+1];  unsigned ip, port;
  lstrcpy(adrcpy, addr);
  if (get_ip_and_port(adrcpy, &ip, &port) && ip!=own_ip)
  { sprintf(addr, "{Direct IP}%u.%u.%u.%u:%u", own_ip>>24, (own_ip>>16) & 0xff, (own_ip>>8) & 0xff, own_ip & 0xff, port);
    monitor_msg(cdp, "Lokalne nahrazuji Direct IP adresu: %u.%u.%u.%u", own_ip>>24, (own_ip>>16) & 0xff, (own_ip>>8) & 0xff, own_ip & 0xff);
  }
}
//
// Konvertuje DirectIP adresu ze stringu na cislo
//
static BOOL get_ip_and_port(char * addr, unsigned * ip, unsigned * port)
{ char *Addr=addr, *Type;

  if (*addr == '{')
  { UnPackAddrType(addr, &Addr, &Type);
    if (stricmp(Type, DIRECTIP)) return FALSE;
  }
  unsigned a1,a2,a3,a4;
  int flds = sscanf(Addr, " %u . %u . %u . %u : %u ", &a1, &a2, &a3, &a4, port);
  if (flds>=4)
  { if (flds==4) *port=the_sp.wb_tcpip_port.val()+2;  // base port +2
    *ip = (a1<<24) | (a2<<16) | (a3<<8) | a4;
  }
  else if (flds==0)
  { char *p = strchr(Addr, ':');
    if (p)
    { *port = atoi(p+1);
      *p = 0;
    }
    else
      *port=the_sp.wb_tcpip_port.val()+2;  // base port +2

#ifdef NETWARE
    NETDB_DEFINE_CONTEXT

    if (!lpNetDBgethostbyname)
    { unsigned hNLM        = GetNLMHandle();
      lpNetDBgethostbyname = (NETDBGETHOSTBYNAME)ImportSymbol(hNLM, "NetDBgethostbyname");
      if (!lpNetDBgethostbyname)
        return(FALSE);
    }
#endif
    struct hostent *hp = gethostbyname(Addr);
    if (p)
      *p = ':';
    if (!hp)
      return(FALSE);
    *ip = ntohl(*(uns32 *)hp->h_addr);
  }
  return TRUE;
}

#ifdef LINUX
/* Tohle se bude volat na Linuxu v pripade zruseni prijimaciho
 * replikacniho vlakna funkci pthread_cancel. */
static void close_listener_socket(void *cdp_v)
{
  close(trans_lsn_sock);
  const cdp_t cdp = (cdp_t)cdp_v;
  transporter_running&=~2;
  //DbgLogFile("DIPReceiver Exit");
  kernel_cdp_free(cdp);
  integrity_check_planning.thread_operation_stopped();
  cd_interf_close(cdp);
}
#endif   

THREAD_PROC(transport_receiver)
{ struct sockaddr_in sin;  int len, err;
  unsigned long param = 1;
  cd_t cd(PT_WORKER);  cdp_t cdp = &cd;
  int res = interf_init(cdp);
  if (res != KSE_OK)
  {
    client_start_error(cdp, workerThreadStart, res);
    THREAD_RETURN;
  }
  integrity_check_planning.system_worker_thread_start();

#ifdef NETWARE
  RenameThread(GetThreadID(), "IP Transport receiver");
#endif
#ifndef LINUX
  CReplSock *tra_sock = new CReplSock();
#else
  CReplSock __repl_sock; /* auto -> no cleanup needed */
  CReplSock *const tra_sock = &__repl_sock;
#endif
  
  if (!tra_sock)
    goto ex;
  sin.sin_family = AF_INET;		        // af internet
  sin.sin_port	 = htons(transporter_port);         // naslouchaci port
  sin.sin_addr.s_addr = htonl(INADDR_ANY);		      // cokoli
#ifdef LINUX
/*
 * Pokud neni nastaveno tohle, bind nejakou dobu po spusteni odmita pripustit 
 * nove spojeni - na portu je stale jeste otevreno cosi ve stavu WAIT
 * Tohle nastaveni dovoli bind navzdory tomu.
 */
  {
    int opt=1;
    setsockopt(trans_lsn_sock, SOL_SOCKET, SO_REUSEADDR,
		(char *) &opt, sizeof(opt));
  }
#endif
  len = bind(trans_lsn_sock, (sockaddr*)&sin, sizeof(sin));
  //DbgLogFile("bind(%d) = %d", trans_lsn_sock, len);
  if (len == -1 || listen(trans_lsn_sock, 5) == -1)
  { monitor_msg(cdp, RE42);
    //DbgLogWrite("Listen failed %d", WSAGetLastError());
    goto ex;
  }
 
  transporter_running|=2;
 {  // { is needed by implementations initializinz a variable in pthread_cleanup_push
#ifdef LINUX
  pthread_cleanup_push(close_listener_socket, cdp);
#endif
  while (TRUE)
  { 
    cdp->set_return_code(ANS_OK);
    integrity_check_planning.thread_operation_stopped();
    err = tra_sock->Accept(trans_lsn_sock);   // cekej na pripojeni klienta
    integrity_check_planning.continuing_replication_operation();
  	if (err)
    {
      break;
    }
   // packets for the IP address will be preferred (remote operation optimization)
    preferred_send_ip=tra_sock->GetIP();
    //DbgLogWrite("DIP Accept %d:%d:%d:%d",LOBYTE(LOWORD(preferred_send_ip)), HIBYTE(LOWORD(preferred_send_ip)), LOBYTE(HIWORD(preferred_send_ip)), HIBYTE(HIWORD(preferred_send_ip)));
    if (is_trace_enabled_global(TRACE_DIRECT_IP))
    { char text[50];
      form_message(text, sizeof(text), RE55, LOBYTE(LOWORD(preferred_send_ip)), HIBYTE(LOWORD(preferred_send_ip)), LOBYTE(HIWORD(preferred_send_ip)), HIBYTE(HIWORD(preferred_send_ip)));
      trace_msg_general(cdp, TRACE_DIRECT_IP, text, NOOBJECT);
    }
    preferred_send_ip_state=1;
    ip_transport_counter++;  
#ifdef WINS
    SetLocalAutoEvent(&hTransportEvent);
    WriteSlaveThreadCounter();
    DIPrWork = TRUE;
    //DbgLogWrite("DIP Prijimam");
#endif
//  	setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char*) &b, sizeof(BOOL));
    int size;
#ifdef DEBUG
    DWORD tm = GetTickCount();
#endif
    char *buf, *hdr, *msg;
    err = tra_sock->Read(&buf, sizeof(int));
    if (err)
    {
      //DbgLogFile("    recv(TRANSPORT_HEADER_MAGIC) = %d", err);
      msg = RE53;
      goto stop;
    }
    if (*(int *)buf!=TRANSPORT_HEADER_MAGIC)
    { err = -1;
      msg = RE55;
      goto stop;
    }
    err = tra_sock->Read(&buf, sizeof(int));
    if (err)
    {
      //DbgLogFile("    recv(size) = %d", err);
      msg = RE53;
      goto stop;
    }
    size = *(int *)buf;
    //DbgLogWrite("DIP Size %d", size);
    if (!size) 
    { err = -1;
      msg = RE55;
      goto stop;
    }
    // create the file
    char fname[MAX_PATH];  FHANDLE fhnd;
    GetTempFileName(input_queue, "WRM", 0, fname);
    fhnd=CreateFile(fname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (!FILE_HANDLE_VALID(fhnd))
    { err = Errno;
      msg = RE18;
      goto stop;
    }
    //DbgLogWrite("DIP %s", fname);
    // receive the file:
    err = tra_sock->Read(&hdr, sizeof(t_packet_header));
    if (err) 
    {
      msg = RE53;
      goto stop; 
    }
    // IF prislo serverove info nastavit priznak v DirectIP adrese serveru
    int len;
    len   = sizeof(t_packet_header);
    if (((t_packet_header*)hdr)->m_packet_type==PKTTP_SERVER_REQ || ((t_packet_header*)hdr)->m_packet_type==PKTTP_SERVER_INFO)
    { err = tra_sock->Read(&buf, sizeof(t_server_pack));
      if (err) 
      { msg = RE53;
        goto stop; 
      }
      tptr ip = (tptr)((t_server_pack *)buf)->addr1;
      if (strncmp(ip + 1, DIRECTIP, 9))
        ip  = (tptr)((t_server_pack *)buf)->addr2;
      *(uns32 *)(ip + lstrlen(ip) + 1) = 0xCCCCCCCC;
      len  += sizeof(t_server_pack);
    }
    DWORD wr;  
    len += tra_sock->GetRest();
    //DbgLogWrite("DIP len %d", len);
    if (!WriteFile(fhnd, hdr, len, &wr, NULL) || wr!=len)
    { err = Errno;
      msg = RE18;
      goto stop;
    }
    while ((size -= len) > 0)
    { //char buf[1024];
      err = tra_sock->Recv(&buf, &len);
      if (err)
      { msg = RE53;
        goto stop;
      }
      //DbgLogFile("    WriteFile %d %d", len, size);
      if (!WriteFile(fhnd, buf, len, &wr, NULL) || wr!=len)
      { err = Errno;
        msg = RE18;
        goto stop;
      }
    }
#ifdef DEBUG
    //DbgLogWrite("Received = %d", GetTickCount() - tm);
#endif
   stop:
    CloseFile(fhnd);  
    tra_sock->Close();
    //DbgLogFile("    closesocket(%d)", tra_sock);
   // received, rename the file:
    if (err)
    { DeleteFile(fname);
      if (is_trace_enabled_global(TRACE_DIRECT_IP))
      { char text[50];
        form_message(text, sizeof(text), msg, err);
        trace_msg_general(cdp, TRACE_DIRECT_IP, text, NOOBJECT);
      }
    }
    else
    { char fname2[MAX_PATH];
      GetTempFileName(input_queue, "WRP", 0, fname2);        
      //DbgLogFile("    Rename %s", fname2);
      if (!MoveFile(fname, fname2)) // must copy, name conflict:
      { if (!CopyFile(fname, fname2, FALSE))
        { monitor_msg(cdp, RE20);
          err = Errno;
        }
        DeleteFile(fname);
        if (!err && is_trace_enabled_global(TRACE_DIRECT_IP))
        { char text[300];
          form_message(text, sizeof(text), RE57, fname2);
          trace_msg_general(cdp, TRACE_DIRECT_IP, text, NOOBJECT);
        }
      }
#ifdef WINS
      DIPTimeout = 2;               // Cas na odpoved odesilateli
#endif
      SetLocalAutoEvent(the_replication->GetStopEvent());
    }
    ip_transport_counter--;  
#ifdef WINS
    WriteSlaveThreadCounter();
    DIPrWork = FALSE;
    //DbgLogWrite("DIP Doprijimano");
#endif
  } // cycle on connections
#ifdef LINUX
  pthread_cleanup_pop(1);
#endif
 }
ex:
  if (trans_lsn_sock!=-1) closesocket(trans_lsn_sock); // is safe, may already be closed
  transporter_running&=~2;

#ifndef LINUX
  if (tra_sock)
    delete tra_sock;
#endif
  //DbgLogFile("DIPReceiver Exit");
  kernel_cdp_free(cdp);
  integrity_check_planning.thread_operation_stopped();
  cd_interf_close(cdp);
  THREAD_RETURN;
}
//
// Vraci informace o paketu k odeslani po DirectIP
//
BOOL GetPacketForDIP(cdp_t cdp, CIPSocket *Sock, unsigned prefip, char *fName, uns32 *PID)
{
    EnterCriticalSection(&cs_mail);
    cur_descr *cd;
    char Address[MAX_PATH];
    char *addr, *type;
    BOOL Found = FALSE;
    char Sel[70];
    sprintf(Sel, SelAdr, *PID);
    tcursnum Curs = open_working_cursor(cdp, Sel, &cd);
    if (Curs != NOCURSOR)
    {
        trecnum r, tr;
        for (r = 0; r < cd->recnum; r++)
        {
            cd->curs_seek(r, &tr);
            GetAddress(cdp, tr, Address, &addr, &type);
            if (lstrcmp(type, DIRECTIP))
                continue;
            Sock->SetAddress(addr, DEFAULT_IP_SOCKET + 2);
            if (prefip && Sock->GetIP()!=prefip)
                continue;
            tb_read_atr(cdp, pcktab_descr, tr, PT_FNAME, fName);
            tb_read_atr(cdp, pcktab_descr, tr, PT_ID,(tptr)PID);
            Found = TRUE;
            break;
        }
        free_cursor(cdp, Curs);
    }
#ifdef DEBUG
    //else
        //DbgLogWrite("Chyba �en�_PACKETTAB %d", cdp->get_return_code());
#endif
    LeaveCriticalSection(&cs_mail);
    return(Found);
}
//
// Vraci adresu ciloveho serveru
//
void GetAddress(cdp_t cdp, trecnum pRec, char *Address, char **Addr, char **Type)
{
    tobjnum sObj;
    *Type = 0;
    tb_read_atr(cdp, pcktab_descr, pRec, PT_DESTOBJ, (tptr)&sObj);
    if (sObj == NOOBJECT)
        tb_read_atr(cdp, pcktab_descr, pRec, PT_ADDRESS, Address);
    else
    { 
        bool usealt; 
        tb_read_atr(cdp, srvtab_descr, sObj, SRV_ATR_USEALT, (tptr)&usealt);
        tb_read_atr(cdp, srvtab_descr, sObj, usealt == wbtrue ? SRV_ATR_ADDR2 : SRV_ATR_ADDR1, Address);
    }
    UnPackAddrType(Address, Addr, Type);
}
//
// Pakovany zapis adresy serveru rozdeli na vlastni adresu a typ adresy 
//
void UnPackAddrType(char *TypeAddr, char **Addr, char **Type)
{
    char *p = TypeAddr;
    if (!*TypeAddr)
    {
        *Type = *Addr = TypeAddr;
    }
    else if (*TypeAddr == '>')
    {
        *Type = INPUTDIR;
        while (*++p == ' ');
        *Addr = p;
    }
    else if (*TypeAddr == '{')
    {
        *Type = ++p;
        while (*p && *p!='}') p++;
        *p++  = 0;
        *Addr = p;
    }
    else 
    {
        *Addr = TypeAddr;
        while (*p && *p != ',' && *p != '@') 
            p++;
        if (*p == '@')  // internet address
            *Type = INTERNET;
        else 
            *Type = _MAIL602;
    }
}
//
// Odeslani paketu dokonceno
//
static char SelDone[54]  = "SELECT * FROM _SYSEXT._PACKETTAB WHERE ID=";

#ifndef WINS  // for LINUX
#define ERROR_FILE_NOT_FOUND ENOENT
#endif

void t_replication::SendDone(cdp_t cdp, uns32 PID, char *fName, int Dir, int Err)
{
    tobjname DestServ;
    WBUUID   DestServID;
    t_packet_type PktType = (t_packet_type)0;
    int  PktNo = 0;
    bool Erase;
    *(int *)DestServ = '?' + ('?' << 8);
    EnterCriticalSection(&cs_mail);
    trecnum tr;
    cur_descr *cd;
    int2str(PID, SelDone + 42);
    tcursnum Curs = open_working_cursor(cdp, SelDone, &cd);
    if (Curs != NOCURSOR)
    {
        DWORD ID;
        if (!cd->curs_seek(0, &tr))
        {
            tb_read_atr(cdp, pcktab_descr, tr, PT_ID,      (tptr)&ID);
            tb_read_atr(cdp, pcktab_descr, tr, PT_DESTNAME, DestServ);
            tb_read_atr(cdp, pcktab_descr, tr, PT_ERASE,   (tptr)&Erase);
            tb_read_atr(cdp, pcktab_descr, tr, PT_PKTTYPE, (tptr)&PktType);
            tb_read_atr(cdp, pcktab_descr, tr, PT_PKTNO,   (tptr)&PktNo);
            if (Err == 0 || Err == MAIL_FILE_NOT_FOUND || Err == ERROR_FILE_NOT_FOUND)
            {
                tb_del(cdp, pcktab_descr, tr);
                if (Erase)
                    DeleteFile(fName);
            }
            CPcktSndd::SetEvents(&PcktSndd, ID, Err);
        }
        free_cursor(cdp, Curs);
        commit(cdp);
    }
    LeaveCriticalSection(&cs_mail);
    char Buf[90];
    if (Err)
        monitor_msg(cdp, Dir == DIR_DIRECTIP ? RE50 : RE39, DestServ, Err);
    else
    {
        *Buf = 0;
        monitor(cdp, Dir, DestServ, PktType, PktNo);
    }
    srvs.GetServerID(cdp, DestServ, DestServID);
    CallOnSended(cdp, DestServID, DestServ, PktType, PktNo, Err, Buf);
}
//
// Uvolneni zrusenych zaznamu v tabulce _PACKETTAB
//
void t_replication::PackPackTab(cdp_t cdp)
{
    EnterCriticalSection(&cs_mail);
    free_deleted(cdp, pcktab_descr);
    commit(cdp);
    LeaveCriticalSection(&cs_mail);
}
//
// Utility
//
void t_replication::mark_core(void)
{ 
    for (t_server_work *server = srvs.m_Server; server;  server = server->next)
        server->mark_core();
    replication_cd.mark_core();
}
//
// t_out_packet - odesilani replikacniho paketu
//
//
// Odesle zadost o serverove info
//
DWORD t_out_packet :: send_server_info_request(char *eaddr, int opparsize)
{ 
    lstrcpy(m_SrvrAddr, eaddr);
    int sz = lstrlen(eaddr) + 1;
    if (opparsize >= sz + sizeof(WBUUID) + sizeof(tobjnum))
    {
        memcpy(m_dest_server, eaddr + sz, sizeof(WBUUID));
        m_SrvrObj = *(tobjnum *)(eaddr + sz + sizeof(WBUUID));
    }
    else
    {
        memset(m_dest_server, 0, UUID_SIZE);
        m_SrvrObj = NOOBJECT;
    }
    if (start_packet(PKTTP_SERVER_REQ))
        if (write_own_server_info())
            complete_packet();
    if (m_Err)
        monitor_packet_err();
    return(m_Err);
}
//
// Odesle paket typu "Serverove info"
//
void t_out_packet :: send_server_info()
{ 
    uns32 shApps  = 0;
    if (start_packet(PKTTP_SERVER_INFO))
        if (write_own_server_info())
            // Tady se puvodne zapisoval do paketu seznam sdilenych aplikaci, ale ten se na nic nepouziva
            if (write_packet(&shApps, sizeof(uns32)))
                complete_packet();
    if (m_Err)
        monitor_packet_err();
}
//
// Do paketu zapise informace o lokalnim serveru
//
BOOL t_out_packet :: write_own_server_info()
{ 
    t_server_pack sp;
    tb_read_atr(m_cdp, srvtab_descr, 0, SRV_ATR_NAME, (tptr)sp.server_name);
    tb_read_atr(m_cdp, srvtab_descr, 0, SRV_ATR_UUID, (tptr)sp.uuid);
    tb_read_atr(m_cdp, srvtab_descr, 0, SRV_ATR_ADDR1,(tptr)sp.addr1);
    tb_read_atr(m_cdp, srvtab_descr, 0, SRV_ATR_ADDR2,(tptr)sp.addr2);
    *(uns32 *)(sp.addr1 + lstrlen((tptr)sp.addr1) + 1) = 0;
    *(uns32 *)(sp.addr2 + lstrlen((tptr)sp.addr2) + 1) = 0;
    if (!write_packet(&sp, sizeof(t_server_pack)))
        return(FALSE);
    tfcbnum fcbn;
    const heapacc *tmphp = (const heapacc*)fix_attr_read(m_cdp, srvtab_descr, 0, SRV_ATR_PUBKEY, &fcbn);
    if (!tmphp)
    {
        m_Err = m_cdp->get_return_code();
        return(FALSE);
    }
    if (!write_packet(&tmphp->len, sizeof(uns32)))
    {
        if (tmphp->len)
        { 
            tptr ptr = (tptr)corealloc(tmphp->len, 33);
            if (!ptr)
            {
                m_Err = OUT_OF_KERNEL_MEMORY;
            }
            else
            {
                hp_read(m_cdp, &tmphp->pc, 1, 0, tmphp->len, ptr);
                write_packet(ptr, tmphp->len);
                corefree(ptr);
            }
        }
    }
    unfix_page(m_cdp, fcbn);
    return(m_Err == NOERROR);
}
//
// Odesle zadost o navazani vztahu
//
DWORD t_out_packet :: send_relation_request(t_relreq_pars *rrp)
{ 
    if (!start_packet(PKTTP_REL_REQ))
        goto error;
    // write parameters into the packet (other server uuid is duplicated here):
    if (!write_packet(rrp, sizeof(t_relreq_pars)))
        goto error;
//#ifdef NEWREL
    tobjnum relobj;
    relobj = find2_object(m_cdp, CATEG_REPLREL, rrp->apl_uuid, rrp->relname);
    if (relobj == NOOBJECT)
    {
        m_Err = m_cdp->get_return_code();
        goto error;
    }
    char *rel;
    rel = ker_load_objdef(m_cdp, objtab_descr, relobj);
    if (!rel)
    {
        m_Err = m_cdp->get_return_code();
        goto error;
    }
    DWORD Size;
    Size = lstrlen(rel);
    if (!write_packet((char *)&Size, sizeof(Size)))
        goto error;
    if (!write_packet(rel, Size))
        goto error;
//#endif
    complete_packet();

error:

    if (m_Err)
        monitor_packet_err();
    return(m_Err);
}
//
// Odesle potvrzeni / zamitnuti replikace apod.
//
DWORD t_out_packet :: send_sharing_control(t_packet_type optype, char * relname, int errnum)
{ 
    if (!start_packet(optype))
        goto error;
    // write parameters into the packet:
    if (!write_packet(m_ApplID, UUID_SIZE))
        goto error;
    if (optype == PKTTP_REL_ACK)  // add server selection
    { 
        apx_header ap;  
        memset(&ap, 0, sizeof(ap));
        trecnum aplobj;
        if (ker_apl_id2name(m_cdp, m_ApplID, NULL, &aplobj))
        {
            m_Err = m_cdp->get_return_code();
            goto error;
        }
        tb_read_var(m_cdp, objtab_descr, aplobj, OBJ_DEF_ATR, 0, sizeof(apx_header), (tptr)&ap);
        if (!write_packet(ap.dp_uuid, UUID_SIZE))
            goto error;
        if (!write_packet(ap.os_uuid, UUID_SIZE))
            goto error;
        if (!write_packet(ap.vs_uuid, UUID_SIZE))
            goto error;
        if (!write_packet(relname, OBJ_NAME_LEN + 1))
            goto error;
    }
    else
    {
        if (!write_packet(&errnum, sizeof(int)))
            goto error;
    }
    // complete packet: cs ensures that no other thread can create pathname after the file is deleted
    complete_packet();

error:

    if (m_Err)
        monitor_packet_err();
    return(m_Err);
}
//
// Odesle dotaz na stav
//
BOOL t_out_packet :: send_state_request()
{ 
    tobjnum dso;
    int     pt;
    wbbool  del;
    // Pokud ve vystupni fronte uz dotaz na stav je, nevytvaret
    for (trecnum r = 0; r < pcktab_descr->Recnum(); r++)
    {
        if (tb_read_atr(m_cdp, pcktab_descr, r, DEL_ATTR_NUM, (char *)&del))
            return(FALSE);
        if (del)
            continue;
        if (tb_read_atr(m_cdp, pcktab_descr, r, PT_DESTOBJ, (char *)&dso))
            return(FALSE);
        if (dso == m_SrvrObj)
        {
            if (tb_read_atr(m_cdp, pcktab_descr, r, PT_PKTTYPE, (char *)&pt))
                return(FALSE);
            if (pt == PKTTP_STATE_REQ)
                return(FALSE);
        }
    }
    if (start_packet(PKTTP_STATE_REQ))
        complete_packet();
    if (m_Err)
    {
        monitor_packet_err();
        return(FALSE);
    }
    return(TRUE);
}
//
// Odesle paket typu "Stavove info"
//
void t_out_packet :: send_state_info(uns32 recvnum)
// Sends state info: the number of the last received packet
{ 
    if (start_packet(PKTTP_STATE_INFO))
        // write parameters into the packet:
        if (write_packet(&recvnum, sizeof(uns32)))
            complete_packet();
    if (m_Err)
        monitor_packet_err();
}
//
// Odesle zadost o replikaci
//
DWORD t_out_packet :: send_pull_request(t_repl_param *rp)
{ 
    if (!start_packet(PKTTP_REPL_REQ))
        goto error;
    if (!write_packet(rp->appl_id, sizeof(WBUUID)))
        goto error;
    if (!write_packet(&rp->token_req, 1))
        goto error;
    if (!write_packet(rp->spectab, OBJ_NAME_LEN + 1))
        goto error;
    if (rp->len16)
        rp->len16 = lstrlen((char *)rp + sizeof(rp)) + 1;
    if (!write_packet(&rp->len16, sizeof(uns16)))
        goto error;
    if (rp->len16)
        if (!write_packet((char *)rp + sizeof(rp), rp->len16))
            goto error;
    complete_packet();

error:

    if (m_Err)
        monitor_packet_err();
    return(m_Err);
}
//
// Odesle potvrzeni replikacniho paketu
//
void t_out_packet :: send_acknowledge(uns32 PacketNo)
{ 
    m_PacketNo = PacketNo;
    if (start_packet(PKTTP_REPL_ACK))
        if (write_packet(&m_PacketNo, sizeof(uns32)))
            complete_packet();
    if (m_Err)
        monitor_packet_err();
}
//
// Odesle zamitnuti replikacniho paketu
//
void t_out_packet :: send_nack(t_nackx *nack, t_kdupl_info *di, WBUUID ApplID)
{ 
    if (!start_packet(PKTTP_REPL_NOACK))
        goto error;
    if (!write_packet(nack, sizeof(t_nack)))
        goto error;
    if (!write_packet(&nack->m_KeyType, nack->m_KeySize + 2 * sizeof(uns32)))
        goto error;
    char Buf[OBJ_NAME_LEN + sizeof(t_nackadi)];
    t_nackadi *adi;  adi = (t_nackadi *)Buf;
    adi->m_Type = NAX_APPLUUID;
    adi->m_Size = sizeof(WBUUID);
    memcpy(adi->m_Val, ApplID, sizeof(WBUUID));
    if (!write_packet(adi, adi->m_Size + 2 * sizeof(uns32)))
        goto error;
    
    if (*m_cdp->errmsg)
    {
        adi->m_Type = NAX_ERRSUPPL;
        adi->m_Size = OBJ_NAME_LEN + 1;
        lstrcpy(adi->m_Val, m_cdp->errmsg);
        if (!write_packet(adi, adi->m_Size + 2 * sizeof(uns32)))
            goto error;
    }
    if (di)
    {
        UINT i;
        CTrcTblVal *ttVal;
        for (ttVal = di->GetRow(0)->GetHdr()->GetNext(), i = 0; ttVal; ttVal = ttVal->GetNext(), i++);
        char *Buf  = (char *)corealloc(i * 255, 1);
        if (Buf)
        {
            t_nackadi *adi = (t_nackadi *)Buf;
            char *p;
            for (ttVal = di->GetRow(0)->GetHdr()->GetNext(), p = adi->m_Val; ttVal; ttVal = ttVal->GetNext(), p += lstrlen(p) + 1)
                lstrcpy(p, ttVal->GetVal());
            adi->m_Type = NAX_KDPLHDR;
            adi->m_Size = (uns32)(p - adi->m_Val);
            if (!write_packet(adi, adi->m_Size + 2 * sizeof(uns32)))
                goto error;
            i = 2;
            if (!di->IsNew())
            {
                for (ttVal = di->GetRow(2)->GetHdr()->GetNext(), p = adi->m_Val; ttVal; ttVal = ttVal->GetNext(), p += lstrlen(p) + 1)
                    lstrcpy(p, ttVal->GetVal());
                adi->m_Type = NAX_KDPLLOC;
                adi->m_Size = (uns32)(p - adi->m_Val);
                if (!write_packet(adi, adi->m_Size + 2 * sizeof(uns32)))
                    goto error;
                i = 3;
            }
            for (ttVal = di->GetRow(1)->GetHdr()->GetNext(), p = adi->m_Val; ttVal; ttVal = ttVal->GetNext(), p += lstrlen(p) + 1)
                lstrcpy(p, ttVal->GetVal());
            adi->m_Type = NAX_KDPLREM;
            adi->m_Size = (uns32)(p - adi->m_Val);
            if (!write_packet(adi, adi->m_Size + 2 * sizeof(uns32)))
                goto error;
            for (ttVal = di->GetRow(i)->GetHdr()->GetNext(), p = adi->m_Val; ttVal; ttVal = ttVal->GetNext(), p += lstrlen(p) + 1)
                lstrcpy(p, ttVal->GetVal());
            adi->m_Type = NAX_KDPLCNF;
            adi->m_Size = (uns32)(p - adi->m_Val);
            if (!write_packet(adi, adi->m_Size + 2 * sizeof(uns32)))
                goto error;
            corefree(Buf);
        }
    }
    // complete packet: cs ensures that no other thread can create pathname after the file is deleted
    complete_packet();

error:

    if (m_Err)
        monitor_packet_err();
}
//
// Odesle paket "Neni co replikovat", jako odpoved na zadost o replikaci
//
void t_out_packet :: send_nothtorepl(int err)
{ 
    if (start_packet(PKTTP_NOTH_TOREPL))
        if (write_packet(&err, sizeof(err)))
            complete_packet();
    if (m_Err)
        monitor_packet_err();
}
//
// Opakovane odesle ztraceny nebo zamitnuty paket
//
DWORD t_out_packet :: resend(tobjnum srvobj)
{
    m_SrvrObj = srvobj;

    wbbool ackn;
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_ACKN, (tptr)&ackn);
    // IF posledni paket potvrzen, neni co delat
    if (ackn == wbtrue)
        return(INTERNAL_SIGNAL);
    if (tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_UUID, (tptr)m_dest_server))
        return(m_cdp->get_return_code());
    // IF neprislo potvrzeni ani zamitnuti, odeslat zadost o stav
    if (ackn == wbfalse)
    {
        send_state_request();
        return(m_Err);
    }
    // Jinak znovu odeslat zamitnuty paket
    if (tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDNUM, (tptr)&m_PacketNo))
        return(m_cdp->get_return_code());
    resend_file(m_PacketNo);
    return(NOERROR);
}
//
// Creates a data replication packet. Uses own cdp initialized by the thread.
//
void t_out_packet :: send_data(t_server_work *server)
{ 
    int i;
    // check the transation state for the server:
    wbbool ackn;  
    m_PcktSndd = server->PcktSndd;
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_ACKN, (tptr)&ackn);
    if (ackn != wbtrue)  // last packet not acknowledged, cannot create a new one
    { 
        if 
        (
            server->last_state_request_send_time == 0 || 
            GetTickDifference(GetTickCount(), server->last_state_request_send_time) > STATE_REQUEST_REPEAT_PERIOD
        ) // state not requested recently
        { 
            char msg[64];
            Get_server_error_text(m_cdp, WAITING_FOR_ACKN, msg, sizeof(msg));  // translated
            trace_msg_general(m_cdp, TRACE_REPLICATION, msg, NOOBJECT);
            if (send_state_request())
                server->last_state_request_send_time = GetTickCount();
        }
        // otherwise do nothing: still waiting for acknowledge
        goto exit;
    }
    // last request acknowledged OK, can create a new packet
    // get packet number:
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDNUM, (tptr)&m_PacketNo); // number of the last sended packet
    m_PacketNo++;  // number for the packet
    if (!start_packet(PKTTP_REPL_DATA))
        goto exit;

    BOOL  connection_reset;
    m_Empty = TRUE;
    connection_reset = FALSE;
    repl_out_counter++;  
#ifdef WINS
    WriteSlaveThreadCounter();
#endif
    if (m_PacketNo == 0)
    { 
        m_PacketNo       = 1;      // the connection has been reset
        connection_reset = TRUE;
    }
    //DbgLogFile("\r\nPacket %d", m_PacketNo);
    if (!write_packet(&m_PacketNo, sizeof(m_PacketNo)))
        goto exit0;
    // write replication info per application:
    for (i = 0; i < server->apl_list.count(); i++)
    { 
        m_ApplItem = server->apl_list.acc(i);
        tb_read_atr(m_cdp, rpltab_descr, m_ApplItem->appl_rec, REPL_ATR_GCRLIMIT, (tptr)m_ApplItem->gcr_start);
        if (connection_reset) // initial send
            memset(m_ApplItem->gcr_start, 0, ZCR_SIZE);
        memcpy(m_ApplItem->gcr_stop, header.gcr, ZCR_SIZE);
        //DbgLogFile("Appl %d  <%d : %d>", m_ApplItem->appl_rec, htonl(*(long *)m_ApplItem->gcr_start), htonl(*(long *)m_ApplItem->gcr_stop));
        if (!repl_appl_pack())
            goto exit0;
        transact(m_cdp);  // store changes in tokens
    }
    if (m_Empty)
    {
        monitor_msg(m_cdp, RE24);
        if (server->pull_req)
        {
            CloseFile(m_fhnd);  
            DeleteFile(m_tempname);
            m_fhnd        = INVALID_FHANDLE_VALUE;
            m_tempname[0] = 0;
            m_Empty       = FALSE;
            send_nothtorepl(NOTHING_TO_REPL);
        }
    }
    else
    {
        // write empty appl id as delimiter:
        WBUUID null_uuid;
        memset(null_uuid, 0, UUID_SIZE);
        if (!write_packet(null_uuid, UUID_SIZE))
            goto exit0;
        // complete packet: cs ensures that no other thread can create pathname after the file is deleted
        if (!complete_packet())
            goto exit0;
        // store GCR limit for each application:
        for (i = 0; i < server->apl_list.count(); i++)
        { 
            m_ApplItem = server->apl_list.acc(i);
            //tb_write_atr(m_cdp, rpltab_descr, m_ApplItem->appl_rec, REPL_ATR_GCRLIMIT, &p, STD_OPT);
            // GCR se ulozi zatim sem, do REPL_ATR_GCRLIMIT se zapise az po prijeti potvrzeni
            tb_write_var(m_cdp, rpltab_descr, m_ApplItem->appl_rec, REPL_ATR_REPLCOND, sizeof(t_replapl), ZCR_SIZE, m_ApplItem->gcr_stop);
        }
        // store sended packet number & not ackn state for server:
        ackn   = wbfalse;
        tb_write_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_ACKN,      &ackn);
        tb_write_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDNUM,   &m_PacketNo);
        tptr p = strrchr(m_tempname, PATH_SEPARATOR) + 1;
        tb_write_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDFNAME, p);
        // prevent early sending state requests:
        server->last_state_request_send_time = GetTickCount();
        monitor_msg(m_cdp, RE25);
    }

exit0:

    repl_out_counter--;  
#ifdef WINS
    WriteSlaveThreadCounter();
#endif

exit:

    transact(m_cdp);  // remove errors
    // store next time (even if error or nothing replicated):
    timestamp refdtm = stamp_now();
    for (i = 0; i < server->apl_list.count(); i++)
    { 
        m_ApplItem = server->apl_list.acc(i);
        timestamp datim;  uns8 aux_map[256/8];
        tb_read_atr(m_cdp, rpltab_descr, m_ApplItem->appl_rec, REPL_ATR_ATTRMAP, (tptr)aux_map);
        if (!aux_map[1] && !aux_map[2])
            datim = 0xffffffff;
        else 
            datim = refdtm + 3600 * aux_map[1] + 60 * aux_map[2];
        tb_write_atr(m_cdp, rpltab_descr, m_ApplItem->appl_rec, REPL_ATR_NEXTTIME, &datim);
    }
    transact(m_cdp);  // remove errors
    if (m_Err || m_Empty)
        CPcktSndd::SetEvents(server->PcktSndd, m_Err ? m_Err : NOTHING_TO_REPL);
    server->Release();

    if (m_Err) 
        monitor_packet_err();
}
//
// Stores replication info for given application into the repl. packet
//
BOOL t_out_packet :: repl_appl_pack()
{ 
    int  len;
    char query[630];
    // open query for description of tables replicated in the application for given server:
    cur_descr *cd;
    tcursnum cursnum1 = NOCURSOR, cursnum2 = NOCURSOR;
    tcursnum cursnum3 = NOCURSOR;
    tcursnum cursnum = find_appl_replrecs(m_ApplItem->appl_id, &cd);
    if (cursnum == NOCURSOR) 
    {
        *m_cdp->errmsg = 0;
        m_Err = OBJECT_NOT_FOUND;
        //DbgLogFile("find_appl_replrecs");
        return(FALSE);
    }
    // save application ID to the packet and init data size:
    DWORD file_pos = 0;
    if (cd->recnum) // save application ID to the packet:
    { 
        if (!write_packet(m_ApplItem->appl_id, UUID_SIZE))
            goto exit;
        file_pos = SetFilePointer(m_fhnd, 0, NULL, FILE_CURRENT);
        if (!write_packet(&file_pos, sizeof(uns32)))
            goto exit;
    }
    memcpy(m_cdp->dest_srvr_id,  m_dest_server, UUID_SIZE);
    {
        t_short_term_schema_context stsc(m_cdp, m_ApplItem->appl_id);
        // pass tables in the application:
        trecnum rec;
        for (rec = 0; rec < cd->recnum; rec++)
        { 
            char tabname[OBJ_NAME_LEN + 1];  uns32 replcond_len;
            // read info for replicating a table:
            trecnum tr;
            cd->curs_seek(rec, &tr);
            tb_read_atr(m_cdp, rpltab_descr, tr, REPL_ATR_TAB, tabname);
            if (!*tabname)
                continue;  // application record, ignore it
            ttablenum tabobj = find_object(m_cdp, CATEG_TABLE,  tabname);
            if (tabobj == NOOBJECT) 
            {
                //DbgLogFile("%s nenalezena", tabname);
                continue; // ignore deleted tables
            }
            tb_read_atr(m_cdp, rpltab_descr, tr, REPL_ATR_ATTRMAP, attrmap.ptr());
            int i = attrmap.last_set();  
            if (i < 0) 
                continue;  // map empty, ignore table
            bool repl_del  = attrmap.has(0);
            bool repl_data = i > 0;
            // create query for updated records:
            len = sprintf(query, "SELECT * FROM %s WHERE (_W5_ZCR>X'", tabname);
            bin2hex (query + len, m_ApplItem->gcr_start, ZCR_SIZE); len += 2 * ZCR_SIZE;
            lstrcpy (query + len, "' AND _W5_ZCR<=X'");             len += 17;
            bin2hex (query + len, m_ApplItem->gcr_stop,  ZCR_SIZE); len += 2 * ZCR_SIZE;
            lstrcpy (query + len, "')");                            len += 2;

            tb_read_atr_len(m_cdp, rpltab_descr, tr, REPL_ATR_REPLCOND, &replcond_len);
            if (replcond_len > 1)
            { 
                tptr replcond = (tptr)corealloc(replcond_len + 1, 44);
                if (!replcond) 
                {
                    m_Err = OUT_OF_KERNEL_MEMORY;
                    break; 
                }
                if (tb_read_var(m_cdp, rpltab_descr, tr, REPL_ATR_REPLCOND, 0, replcond_len, replcond))
                    replcond_len = 0;
                replcond[replcond_len] = 0;
                cutspaces(replcond);
                if (*replcond)  // condition may be empty!
                    len += sprintf(query + len, " AND (%s)", replcond);
                corefree(replcond);
            }
            // install table, add condition for returned tokens
            table_descr_auto tda(m_cdp, tabobj);
            if (!tda->me()) 
            { 
                //DbgLogFile("%s tda->me", tabname);
                m_Err = m_cdp->get_return_code();
                break; 
            }
            if (tda->tabdef_flags & TFL_TOKEN && m_ApplItem->HeIsDP)
                lstrcpy(query + len, " OR _W5_TOKEN BETWEEN -32768 AND -16385");
            // open cursor for standard replication:
            cur_descr *cd1; trecnum r, trr;
            cursnum1 = open_working_cursor(m_cdp, query, &cd1);
            if (cursnum1 == NOCURSOR) 
            { 
                //DbgLogFile("%s open_working_cursor", tabname);
                m_Err = m_cdp->get_return_code();
                break; 
            }
            // open cursor for special replication:
            cur_descr *cd2;  tcursnum cursnum2 = NOCURSOR;
            if (repl_data && m_ApplItem->special_cond && !sys_stricmp(tabname, m_ApplItem->special_tabname))
            { 
                sprintf(query, "SELECT * FROM %s WHERE %s", tabname, m_ApplItem->special_cond);
                cursnum2 = open_working_cursor(m_cdp, query, &cd2);
                //DbgLogFile("cd2 = %d", cd2 ? cd2->recnum : -1);
            }
            
            // deleted records
            table_descr *DelRecs;
            cur_descr *cd3;
            if (repl_del)
            {
                DelRecs = GetReplDelRecsTable(m_cdp);
                if (DelRecs)
                {
                    lstrcpy(query,       "SELECT * FROM _SYSEXT._REPLDELRECS WHERE SCHEMAUUID=X'"); len  = 54;
                    bin2hex(query + len, m_ApplItem->appl_id,   UUID_SIZE);                  len += 2 * UUID_SIZE;
                    lstrcpy(query + len, "' AND TABLENAME='");                               len += 17;
                    lstrcpy(query + len, tabname);                                           len += lstrlen(tabname);
                    lstrcpy(query + len, "' AND ZCR>X'");                                    len += 12;
                    bin2hex(query + len, m_ApplItem->gcr_start, ZCR_SIZE);                   len += 2 * ZCR_SIZE;
                    lstrcpy(query + len, "' AND ZCR<=X'");                                   len += 13;
                    bin2hex(query + len, m_ApplItem->gcr_stop,  ZCR_SIZE);                   len += 2 * ZCR_SIZE;
                    lstrcpy(query + len, "'");                                               len += 2;
                    cursnum3 = open_working_cursor(m_cdp, query, &cd3);
                    if (cursnum3 == NOCURSOR || cd3->recnum == 0)
                        repl_del = false;
                }
            }
            // write records to the file:
            //DbgLogFile("%s Cd = %d", tabname, cd1->recnum);
            if (cd1->recnum || cursnum2 != NOCURSOR && cd2->recnum || repl_del) // any records to be posted in the table
            { 
                if (!TableInit(tda->me()))  // returns FALSE when index not found
                {
                    m_Err = NO_REPL_UNIKEY;
                    lstrcpy(m_cdp->errmsg, tabname);
                    break;
                }
                //DbgLogFile("    %s  %d", tabname, cd1->recnum);
                // save table name and attribute map to the packet:
                if (!write_packet(tabname, OBJ_NAME_LEN + 1))
                    break;
                if (!write_packet(attrmap.ptr(), 256/8))
                    break;
                // pass deleted records:
                if (repl_del)
                {
                    //DbgLogFile("    Deleted");
                    if (cursnum3 != NOCURSOR)
                    {
                        for (r = 0; r < cd3->recnum; r++)
                            if (!cd3->curs_seek(r, &trr))
                                if (!StoreDelRecsRecord(DelRecs, trr))
                                    break;
                    }
                    else
                    {
                        for (r = 0; r < tda->Recnum();  r++)
                        { 
                            if (!store_repl_record(r, FALSE, TRUE))
                            {
                                //DbgLogFile("repl_del store_repl_record");    
                                break;
                            }
                        }
                    }
                }
                if (repl_data)
                {
                    // pass updated/added records, save changes:
                    for (r = 0; r < cd1->recnum; r++)
                        if (!cd1->curs_seek(r, &trr))
                        { 
                            if (!store_repl_record(trr, FALSE, FALSE))
                            {
                                //DbgLogFile("cd store_repl_record");    
                                break;
                            }
                        }
                    // pass requested records:
                    if (cursnum2 != NOCURSOR)
                    {
                        //DbgLogFile("    SpecTable  %d", cd2->recnum);
                        for (r=0;  r < cd2->recnum;  r++)
                        {
                            if (!cd2->curs_seek(r, &trr))
                            { 
                                if (!store_repl_record(trr, m_ApplItem->token_request, FALSE))
                                {
                                    //DbgLogFile("cd2 store_repl_record");    
                                    break;
                                }
                            }
                        }
                        free_cursor(m_cdp, cursnum2);
                        cursnum2 = NOCURSOR;
                    }
                }
                // save table terminator:
                uns8 term = 0xff;            
                if (!write_packet(&term, 1)) 
                {
                    //DbgLogFile("write_term");
                    break;
                }
            }
            free_cursor(m_cdp, cursnum1);
            cursnum1 = NOCURSOR;
            if (cursnum3 != NOCURSOR)
            {
                free_cursor(m_cdp, cursnum3);
                cursnum3 = NOCURSOR;
            }
        } // cycle on replicated tables

        // store table list terminator: null table name (unless no table replicable)
        if (!m_Err && cd->recnum)
        {
            memset(query, 0, OBJ_NAME_LEN + 1);
            write_packet(&query, OBJ_NAME_LEN + 1);
            // store the application data size:
            DWORD data_size = SetFilePointer(m_fhnd, 0, NULL, FILE_CURRENT) - file_pos - sizeof(uns32);
            SetFilePointer(m_fhnd, file_pos,  NULL, FILE_BEGIN);
            write_packet(&data_size, sizeof(uns32));
            SetFilePointer(m_fhnd, data_size, NULL, FILE_CURRENT);
        }
    } // application context
exit:

    if (cursnum3 != NOCURSOR)
        free_cursor(m_cdp, cursnum3);
    if (cursnum2 != NOCURSOR)
        free_cursor(m_cdp, cursnum2);
    if (cursnum1 != NOCURSOR)
        free_cursor(m_cdp, cursnum1);
    free_cursor(m_cdp, cursnum);
    return(m_Err == NOERROR);
}
//
// Z REPLTAB vybere zazamy pro zadany src_srv, dest_srv a appl_id
//
tcursnum t_out_packet :: find_appl_replrecs(WBUUID appl_id, cur_descr **pcd)
{ 
    int len;
    char query[260];
    lstrcpy(query, "SELECT * FROM REPLTAB WHERE SOURCE_UUID=X'"); len  = 42;
    bin2hex(query + len, my_server_id, UUID_SIZE);                len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND DEST_UUID=X'");                   len += 18;
    bin2hex(query + len, m_dest_server, UUID_SIZE);               len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND APPL_UUID=X'");                   len += 18;
    bin2hex(query + len, appl_id,  UUID_SIZE);                    len += 2 * UUID_SIZE;
    lstrcpy(query + len, "'");
    return(open_working_cursor(m_cdp, query, pcd));
}
//
// Writes replicated info from database to packet.
//
BOOL t_out_packet :: store_repl_record(trecnum tr, BOOL pass_token, BOOL deleted_only)
{ 
    tfcbnum fcbn, fcbna;  const char *dt, *dta;  uns8 dltd;  int a;
    BOOL post_token;  // TRUE iff must pass the token even if luo fails
    sig16 token_val;  uns8 detect_val[ZCR_SIZE];
    
    dt = fix_attr_read(m_cdp, td, tr, DEL_ATTR_NUM, &fcbn);
    if (!dt)
    { 
        m_Err = m_cdp->get_return_code();
        //DbgLogFile("        fix_attr_read(%d) = %04X", tr, m_Err);
        return(FALSE);
    }

    // for deleted_only replication check the status and ZCR scope:
    dltd = *dt;
#ifdef DEBUG
    BOOL dl = FALSE;
    BOOL ec = FALSE;
#endif
    if (deleted_only)
    { 
#ifdef DEBUG
        dl = TRUE;
#endif
        if (dltd != 1)
            goto ex;
        if (memcmp(dt + zcr_offset, m_ApplItem->gcr_start, ZCR_SIZE) <= 0 || 
            memcmp(dt + zcr_offset, m_ApplItem->gcr_stop,  ZCR_SIZE) >  0)
            goto ex; // out of ZCR scope
#ifdef DEBUG
        dl = 2;
#endif
    }
    // prepare record header:
    if (detect_offset)
    { 
        if (m_ApplItem->IAmFather)
            memcpy(detect_val, dt + zcr_offset, ZCR_SIZE);                  // _W5_ZCR
        else
            memcpy(detect_val, dt + detect_offset + ZCR_SIZE, ZCR_SIZE);    // _W5_DETECT2
    }
    // passed token value:
    post_token = FALSE; // TRUE iff must replicate even to the last update origin
    if (token_offset)
    { 
        token_val = *(sig16*)(dt + token_offset);
        if (m_ApplItem->HeIsDP)                                         // must pass the counter value even if not returning
        { 
            if (pass_token && !(token_val & 0x8000))                    // token request from DP: no replication if I do not have it
                goto ex;
            if ((token_val & 0xc000)==0x8000 ||                         // return free token to DP
                 pass_token && not_locked(m_cdp, tr))                   // return requested token
            { 
                tptr dta = fix_attr_write(m_cdp, td, tr, DEL_ATTR_NUM, &fcbna);
                if (dta)
                { t_simple_update_context sic(m_cdp, td, token_attr);
                  if (sic.lock_roots())
                    if (sic.inditem_remove(tr, NULL))    //  remove from token index 
                    { *(sig16*)(dta + token_offset) = 0;                  // I do not have it any more
                      sic.inditem_add(tr);             // add into token index
                    }
                    unfix_page(m_cdp, fcbna);
                    post_token = TRUE;
                }
                else
                    token_val &= 0x1FFF;                                // cannot return
            }
            else
                token_val &= 0x1FFF;                                    // I do not have or I am holding
        }
        else if (m_ApplItem->IAmDP)
        { 
            // Posted token value: passing the token iff 0x8000 set, 0x1fff bits hold the counter then.
            // Otherwise request denied iff 0x4000 set, 0x2000 says "I do not have it"
            if (pass_token)                                             // this token is requested
            { 
                post_token = TRUE;
                if ((token_val & 0xc000) == 0x8000)                     // I have it and do not hold it
                {
                    token_val = (token_val +1) & 0x1fff;                // updated counter value
                    tptr dta  = fix_attr_write(m_cdp, td, tr, DEL_ATTR_NUM, &fcbna);
                    if (dta != NULL)
                    { t_simple_update_context sic(m_cdp, td, token_attr);
                      if (sic.lock_roots())
                        if (sic.inditem_remove(tr, NULL))    //  remove from token index 
                        { *(sig16*)(dta + token_offset) = token_val;                  // I do not have it any more
                          sic.inditem_add(tr);             // add into token index
                        }
                        unfix_page(m_cdp, fcbna);
                    }
                    token_val |= 0xC000;                                // granting it, will be holded
                }
                else if ((token_val & 0xc000) == 0xc000)                // cannot grant, holding it
                    token_val = 0x4000;
                else if (!(token_val & 0x8000))                         // cannot grant, I do not have it
                    token_val=0x6000;
            }
            else
                token_val = 0;                                          // token not requested
        }
        else
            token_val = 0;                                              // nobody is DP
    }

    // check LUO:
    if (luo_offset)
        if (!memcmp(dt + luo_offset, m_dest_server, UUID_SIZE))         // this is echo
            if (!post_token)                                            // do not need to pass the token
            {
#ifdef DEBUG
                ec = TRUE;
#endif
                goto ex;
            }

    // check for empty unikey: (this is not enabled, but causes more severe errors):
    if (unikey)
    { 
        const DWORD *p = (DWORD *)(dt + keyoffset);  int cnt = keysize / sizeof(DWORD);
        while (!*p && cnt)
        { cnt--;  p++; }
        if (!cnt)               // empty uniey, must not replicate
        {   tobjname appl;
            appl[0] = 0;
            tb_read_atr(m_cdp, objtab_descr, m_ApplItem->m_ApplObj, OBJ_NAME_ATR, appl);
            monitor_msg(m_cdp, RE72);
            monitor_msg(m_cdp, RE73, appl, td->selfname, tr);
            goto ex;
        }
    }
    // writing record header:
    if (!write_packet(&dltd, 1))
        goto ex;
    if (token_offset)
        if (!write_packet(&token_val, sizeof(sig16)))
            goto ex;
    char keyval[MAX_KEY_LEN]; // patri do vetve !unikey
    if (unikey)
    { 
        if (!write_packet(dt + keyoffset, keysize))
            goto ex;
    }
    else  // compute the primary key
    { 
        dd_index *cc = td->indxs + indexnum;
        if (!compute_key(m_cdp, td, tr, cc, keyval, false))
        { 
            m_Err = LAST_DB_ERROR + 1;
            goto ex; 
        }
        //        *(trecnum*)cdp->RV.index_ptr=tr;  -- no need, not stored to file
        if (!write_packet(keyval, keysize))
            goto ex;
    }
    if (deleted_only)
    {
        m_Empty = FALSE;
        goto ex;
    }
    if (detect_offset)
        if (!write_packet(detect_val, ZCR_SIZE))
            goto ex;

    // store attribute values:
    for (a=1;  a < td->attrcnt;  a++)
    {
        if (attrmap.has(a))  // replicated attribute
        { 
            attribdef *att = &td->attrs[a];
            if (att->attrmult>1)
            {
                uns16 count;
                if (fast_table_read(m_cdp, td, tr, a, &count))
                    count = 0;
                if (!write_packet(&count, CNTRSZ))
                    goto ex;
                for (int index = 0; index < count;  index++)
                { 
                    dta = fix_attr_ind_read(m_cdp, td, tr, a, index, &fcbna);
                    if (dta)
                    { 
                        if (IS_HEAP_TYPE(att->attrtype))
                            hp_packet_copy((tpiece*)dta, *(uns32*)(dta+sizeof(tpiece)));
                        else 
                            write_packet(dta, typesize(att));
                        unfix_page(m_cdp, fcbna);
                    }
                    else
                    {
                        m_Err = m_cdp->get_return_code();
                        break;
                    }
                }
            }
            else  // not a multiattribute
            { 
                if (!att->attrpage) // take from the fixed page
                    dta = dt + att->attroffset;
                else
                { 
                    dta = fix_attr_read(m_cdp, td, tr, a, &fcbna);
                    if (!dta)
                    { 
                        m_Err = m_cdp->get_return_code();
                        break;
                    }
                }
                if (IS_HEAP_TYPE(att->attrtype))
                    hp_packet_copy((tpiece*)dta, *(uns32*)(dta+sizeof(tpiece)));
                else 
                    write_packet(dta, typesize(att));
                if (att->attrpage)
                    unfix_page(m_cdp, fcbna);
            }
        }
        if (m_Err)
            goto ex;
    }
    m_Empty = FALSE;

ex:

    unfix_page(m_cdp, fcbn);
    return(m_Err == NOERROR);
}

bool t_out_packet :: StoreDelRecsRecord(table_descr *tdr, trecnum tr)
{
    bool  True = 1;
    sig16 Token = 0;
    if (!write_packet(&True, 1))
        goto ex;
    if (token_offset)
        if (!write_packet(&Token, sizeof(sig16)))
            goto ex;

    tfcbnum fcbn;
    const char *dt;
    tattrib Attr;  Attr = (unikey && RDR_W5_UNIKEY != (tattrib)-1) ? RDR_W5_UNIKEY : RDR_UNIKEY;
    dt = fix_attr_read(m_cdp, tdr, tr, Attr, &fcbn);
    if (!dt)
    { 
        m_Err = m_cdp->get_return_code();
        goto ex;
    }
    if (!write_packet(dt, keysize))
        goto ex;
    m_Empty = FALSE;

ex:

    unfix_page(m_cdp, fcbn);
    return(m_Err == NOERROR);
}
//
// Vytvori paket a zapise hlavicku
//
BOOL t_out_packet :: start_packet(t_packet_type pkttype)
{ 
    t_server_work *server = srvs.get_server(m_cdp, m_dest_server);
    if (server)
    {
        m_SrvrObj = server->srvr_obj;
        lstrcpy(m_SrvrName, server->srvr_name);
    }
    else
    {
        if (pkttype != PKTTP_SERVER_REQ)
        {
            *m_cdp->errmsg = 0;
            m_Err = OBJECT_NOT_FOUND;
        }
        *(DWORD *)m_SrvrName = '?' + ('?' << 8);
    }
    monitor(m_cdp, DIR_OUTPCK, m_SrvrName, pkttype, m_PacketNo);
    if (m_Err)
        return(FALSE);
    if (!GetTempFileName(output_queue, "WRX", 0, m_tempname))
        goto error;
    m_fhnd = CreateFile(m_tempname, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (!FILE_HANDLE_VALID(m_fhnd))
        goto error;
    // write packet header:
    m_version     = CURR_PACKET_VERSION;
    m_packet_type = pkttype;
    memcpy(m_source_server, my_server_id, sizeof(WBUUID));
    write_packet((t_packet_header *)this, sizeof(t_packet_header));
    return(m_Err == NOERROR);

error:
      
    m_Err = OS_FILE_ERROR;
    return(FALSE);
}
//
// Writes dat to packet. May implement caching and error tracking.
//
BOOL t_out_packet :: write_packet(const void *data, DWORD size)
{ 
    DWORD wr;
    if (WriteFile(m_fhnd, data, size, &wr, NULL) && wr == size)
        return(TRUE);
    m_Err = OS_FILE_ERROR;
    return(FALSE);
}
//
// Stores var-len attribute to the packet.
// Returns FALSE on database errors, ignores packet file errors.
//
BOOL t_out_packet :: hp_packet_copy(const tpiece *pc, uns32 size)
{ 
    if (!write_packet(&size, HPCNTRSZ))
        return(FALSE);
    if (size && pc->len32 && pc->dadr)
    {
        t_dworm_access dwa(m_cdp, pc, 1, FALSE, NULL);
        if (!dwa.is_open())
        {
            m_Err = m_cdp->get_return_code();
            return(FALSE);
        }
        unsigned local;
        for (unsigned pos = 0; size; pos += local, size -= local)
        {
            tfcbnum fcbn;
            const char *dt = dwa.pointer_rd(m_cdp, pos, &fcbn, &local);
	        if (!dt)
            {
                m_Err = m_cdp->get_return_code();
                return(FALSE);
            }
	        if (local > size)
                local = size;
            write_packet(dt, local);
            if (fcbn != NOFCBNUM)
                unfix_page(m_cdp, fcbn);
        }
    }
    return(TRUE);
}
//
// Prejmenuje paket a odesle ho protejsimu serveru
//
BOOL t_out_packet :: complete_packet()
{ 
    CloseFile(m_fhnd);
    char pathname[MAX_PATH];
    EnterCriticalSection(&cs_replfls);
    GetTempFileName(output_queue, "WRP", 0, pathname);
    DeleteFile(pathname);
    if (!MoveFile(m_tempname, pathname))
        m_Err = OS_FILE_ERROR;
    if (!m_Err)
    {
        lstrcpy(m_tempname, pathname);  // actual name stored, used when sending repl. data
        // send to the server's or specified address:
        send_file();
    }
    LeaveCriticalSection(&cs_replfls);
    return(m_Err == NOERROR);
}
//
// Odesle paket
//
void t_out_packet :: send_file()
{
    // find the address of the destination server:
    if (m_SrvrObj != NOOBJECT && !m_SrvrAddr[0])
    { 
        uns8 usealt = wbfalse;
        tb_read_atr(m_cdp, SRV_TABLENUM, m_SrvrObj, SRV_ATR_USEALT, (tptr)&usealt);
        tb_read_atr(m_cdp, SRV_TABLENUM, m_SrvrObj, usealt == wbtrue ? SRV_ATR_ADDR2 : SRV_ATR_ADDR1, m_SrvrAddr);
        if (!m_SrvrAddr[0])
        {
            m_Err = MAIL_NO_ADDRESS;
            return;
        }
    }
    int len; 
    for (len = lstrlen(m_tempname); len && m_tempname[len - 1] != PATH_SEPARATOR; len--);
    char *Addr;
    for (Addr = m_SrvrAddr; *Addr == ' '; Addr++);
    // IF sdilene adresare, zkopirovat 
    if (*Addr == '>')
    {
        char destname[MAX_PATH];
        do
            Addr++;
        while (*Addr==' ');
        lstrcpy(destname, Addr);
        lstrcat(destname, m_tempname + len - 1);  // file name including preceeding \ char
        if ((m_packet_type == PKTTP_REPL_DATA) || !MoveFile(m_tempname, destname))
        {// must copy, erasing disabled or (legal) name conflict:
            GetTempFileName(Addr, "WRP", 0, destname);
            if (!CopyFile(m_tempname, destname, FALSE))
            {
                monitor_msg(m_cdp, RE19);
                m_Err = OS_FILE_ERROR;
            }
        }
        if (m_PcktSndd)
            CPcktSndd::SetEvents(m_PcktSndd, m_Err);
        if (!m_Err)
            CallOnSended(m_cdp, m_dest_server, m_SrvrName, m_packet_type, m_PacketNo, m_Err, NULL);
        return;
    }
    else  // Zapsat do PacketTab a spustit DirectIP nebo Mail vlakno
    { 
        EnterCriticalSection(&cs_mail);
        trecnum r = tb_new(m_cdp, pcktab_descr, FALSE, NULL);
        if (r == NORECNUM)
        {
            LeaveCriticalSection(&cs_mail);
            return;
        }
        if (m_SrvrObj == NOOBJECT)  // PKTTP_SERVER_REQ
          tb_write_atr(m_cdp, pcktab_descr, r, PT_ADDRESS,  Addr);
        wbbool erase = m_packet_type != PKTTP_REPL_DATA;
        tb_write_atr(m_cdp, pcktab_descr, r, PT_ERASE,    &erase);
        tb_write_atr(m_cdp, pcktab_descr, r, PT_PKTTYPE,  &m_packet_type);
        tb_write_atr(m_cdp, pcktab_descr, r, PT_PKTNO,    &m_PacketNo);
        tb_write_atr(m_cdp, pcktab_descr, r, PT_DESTOBJ,  &m_SrvrObj);
        tb_write_atr(m_cdp, pcktab_descr, r, PT_DESTNAME, m_SrvrName);
        tb_write_atr(m_cdp, pcktab_descr, r, PT_FNAME,    m_tempname);
        if (m_PcktSndd)
        {
            DWORD ID;
            if (!tb_read_atr(m_cdp, pcktab_descr, r, PT_ID, (tptr)&ID))
                CPcktSndd::Append(the_replication->GetPcktSnddHdr(), m_PcktSndd, ID);
        }
        commit(m_cdp);
        LeaveCriticalSection(&cs_mail);
        the_replication->StartDispatchThread(m_cdp, strncmp(Addr, "{Direct IP}", 11) == 0);
    }
}
//
// Znovu odesle paket
//
void t_out_packet :: resend_file(uns32 sendnum)
{
    t_server_work *server = srvs.get_server(m_cdp, m_dest_server);
    lstrcpy(m_SrvrName, server->srvr_name);
    m_packet_type = PKTTP_REPL_DATA;
    m_PacketNo    = sendnum;
    lstrcpy(m_tempname, output_queue);  
    lstrcat(m_tempname, PATH_SEPARATOR_STR);
    char *pf = m_tempname + lstrlen(m_tempname);
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDFNAME, pf);
    if (*pf)
    { 
        send_file();
        monitor_msg(m_cdp, RE30);
    }
    server->last_state_request_send_time = GetTickCount();
}
//
// Vypise chybu pri odesilani paketu do logu
//
void t_out_packet :: monitor_packet_err()
{
    char msg[120];
    monitor(m_cdp, DIR_ERROR, m_SrvrName, m_packet_type, m_PacketNo);
    *(DWORD *)msg = ' ' + (' ' << 8) + (' ' << 16) + (' ' << 24);
    // tady taky
    Get_server_error_text(m_cdp, m_Err, msg + 4, sizeof(msg) - 4);  // translated
    trace_msg_general(m_cdp, TRACE_REPLICATION, msg, NOOBJECT);
    CallOnSended(m_cdp, m_dest_server, m_SrvrName, m_packet_type, m_PacketNo, m_Err, msg + 4);
}
//
// Destruktor
//
t_out_packet :: ~t_out_packet()
{
    if ((m_Err || m_Empty) && FILE_HANDLE_VALID(m_fhnd))
    {
        EnterCriticalSection(&cs_replfls);
        CloseFile(m_fhnd);  
        DeleteFile(m_tempname);
        LeaveCriticalSection(&cs_replfls);
    }
}
//
// t_in_packet - replikace dovnitr
//
// Spojovani zadosti o pesky: Pokud zpracuji vic doslych zadosti o pesky ze
// stejne tabulky, spojim podminky. Kriticka sekce v prochazeni doslych paketu
// posunuta tak, aby se zadost o replikaci nezacala zpracovavat driv, nez
// prectu dalsi zadost.
// Kdyz prijde dalsi zadost v dobe, kdy cekam na potvrzeni predchozi replikace,
// nezpracuji tento paket a vratim se k nemu pri pristim zpracovani.
//
BOOL t_in_packet :: process_input_packet()
// Processing of incomming packet by the working thread
// Returs TRUE if file processed and should be deleted, FALSE if postponed
{ 
    repl_in_counter++;  
#ifdef WINS
    WriteSlaveThreadCounter();
#endif
    m_cdp->except_type              = EXC_NO;
    m_cdp->except_name[0]           = 0;
    m_cdp->processed_except_name[0] = 0;

    srvs.GetServerName(m_cdp, m_source_server, m_SrvrName);
    if 
    (
        m_packet_type != PKTTP_REPL_DATA  && m_packet_type != PKTTP_REPL_ACK &&
        m_packet_type != PKTTP_SERVER_REQ && m_packet_type != PKTTP_SERVER_INFO
    )
        monitor(m_cdp, DIR_INPCK, m_SrvrName, m_packet_type);
    switch (m_packet_type)
    { 
    case PKTTP_REPL_DATA:
    { 
        uns32 recvnum;
        if (!get_file_data(&m_PacketNo, sizeof(uns32))) // ignore incomplete packet
            break;
        monitor(m_cdp, DIR_INPCK, m_SrvrName, m_packet_type, m_PacketNo);
        m_SrvrObj = srvs.server_id2obj(m_cdp, m_source_server);
        if (m_SrvrObj == NOOBJECT) 
            break; // server deleted
        if (tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_RECVNUM, (tptr)&recvnum))
            break;
        if (m_PacketNo == recvnum + 1 || m_PacketNo == 1) // the expected packet
        {
            if (process_incomming_repl())
            {
                tb_write_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_RECVNUM, &m_PacketNo);
                t_out_packet packet(m_cdp, m_source_server, m_SrvrObj);
                packet.send_acknowledge(m_PacketNo);
            }
            else
            {
                for (t_exkont_info * eki = m_cdp->exkont_info; eki && eki < m_cdp->exkont_info + m_cdp->exkont_len; eki++)
                {
                    if (eki->type == EKT_TABLE)
                    {
                        if (eki->par1 != m_TableObj)
                            tb_read_atr(m_cdp, tabtab_descr, eki->par1, OBJ_NAME_ATR, (tptr)m_Table);
                        m_RecNum = eki->par2;
                    }
                }

                monitor_msg(m_cdp, RE52, m_PacketNo);
                monitor_msg(m_cdp, RE66, m_Appl, m_Table, m_RecNum);
                *(DWORD *)m_ErrMsg = ' ' + (' ' << 8) + (' ' << 16) + (' ' << 24);
                Get_server_error_text(m_cdp, m_Err & ~ERROR_RECOVERABLE, m_ErrMsg + 4, sizeof(m_ErrMsg) - 4);
                if (m_ErrMsg[4])  // translated
                  trace_msg_general(m_cdp, TRACE_REPLICATION, m_ErrMsg, NOOBJECT);
                if (m_DuplInfo)
                    m_DuplInfo->Trace(m_cdp, TRACE_REPL_CONFLICT);
                if (!(m_Err & ERROR_RECOVERABLE))
                {
                    t_out_packet packet(m_cdp, m_source_server, m_SrvrObj);
                    packet.send_nack(this, m_DuplInfo, m_ApplID);
                }
                WriteReplErr(m_DuplInfo, this, NORECNUM, m_RecNum, m_RecCnf);
            }
        }
        else if (m_PacketNo == recvnum) // last packet sended againg
        {
            t_out_packet packet(m_cdp, m_source_server, m_SrvrObj);
            packet.send_acknowledge(m_PacketNo);
        }
        // ignore older packets, acknowledge must have been receive
        break;
    }
    case PKTTP_REPL_REQ:  // pull request, param: appl id, token_req, spectab, len, speccond
    { 
        t_pull_req *pull_req = (t_pull_req *)corealloc(GetFileSize(m_fhnd, NULL) - sizeof(t_packet_header) - UUID_SIZE, 0xEF);
#ifdef UNIX
        SetFilePointer(m_fhnd, sizeof(t_packet_header), NULL, 0);
#endif
        if (!pull_req)
        {
            m_Err = OUT_OF_KERNEL_MEMORY | ERROR_RECOVERABLE;
            break;
        }
        if (get_file_data(m_ApplID, UUID_SIZE) && get_file_data(pull_req, offsetof(t_pull_req, m_SpecCond))) // ignore incomplete packet
        {
            if (pull_req->m_len16)
                if (!get_file_data(pull_req->m_SpecCond, pull_req->m_len16))
                    break;
            replicate_appl(m_source_server, pull_req);
        }
        corefree(pull_req);
        break;
    }

    case PKTTP_REPL_ACK:
    { 
        m_SrvrObj = srvs.server_id2obj(m_cdp, m_source_server);
        if (m_SrvrObj == NOOBJECT)
            break; // server deleted, ignore packet
        if (get_file_data(&m_PacketNo, sizeof(uns32))) // ignore incomplete packet
        {
            monitor(m_cdp, DIR_INPCK, m_SrvrName, m_packet_type, m_PacketNo);
            uns32 sendnum;
            if (!tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDNUM, (tptr)&sendnum)) // sendnum==number of the last sended packet
            { 
                if (m_PacketNo == sendnum) // the last packet sended has been received OK, store the positive acknowledge
                { 
                    last_packet_acknowledged(m_source_server);
                    transact(m_cdp);
                }
            }
        }
        break;
    }

    case PKTTP_REPL_NOACK:
    { 
        process_nack();
        break;
    }

    case PKTTP_NOTH_TOREPL:
    { 
        int err;
        if (get_file_data(&err, sizeof(err)))
        { 
            char buf[160];
            if (Get_server_error_text(m_cdp, err, buf, sizeof(buf)))  // translated
              trace_msg_general(m_cdp, TRACE_REPLICATION, buf, NOOBJECT);    
            else
              monitor_msg(m_cdp, RE64, err);
        }
        break;
    }      

    case PKTTP_STATE_REQ:  // send receive-state info unless processing incomming packet
    { 
        m_SrvrObj = srvs.server_id2obj(m_cdp, m_source_server);
        if (m_SrvrObj == NOOBJECT)
            break; // server deleted
//      t_server_work * server = find_server(phd->source_server, NULL);
//      if (server!=NULL && server->proc_state!=ps_nothing)
//        break;  // acknowledge will be send when the packet will be processed
//  --  does not work because it sees own prcessing
        uns32 recvnum;
        if (!tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_RECVNUM, (tptr)&recvnum))
        {
            t_out_packet packet(m_cdp, m_source_server, m_SrvrObj);
            packet.send_state_info(recvnum);
        }
        break;
    }

    case PKTTP_STATE_INFO:
    { 
        m_SrvrObj = srvs.server_id2obj(m_cdp, m_source_server);
        if (m_SrvrObj == NOOBJECT)
            break; // server deleted, ignore packet
        if (!get_file_data(&m_PacketNo, sizeof(uns32))) // ignore incomplete packet
            break;
        uns32 sendnum;
        if (!tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDNUM, (tptr)&sendnum)) // last received packet number
        {
            if (m_PacketNo == sendnum) // the last packet sended has been received OK, store the positive acknowledge
            {
                last_packet_acknowledged(m_source_server);
                transact(m_cdp);
            }
            if (m_PacketNo == sendnum - 1) // last packet lost, re-send it
            {
                t_out_packet packet(m_cdp, m_source_server, m_SrvrObj);
                packet.resend_file(sendnum);
            }
            // ignore other (old) packets
        }
        break;
    }
    case PKTTP_SERVER_REQ:
    {
        tobjnum srv_obj = store_remote_server_description(/*FALSE*/);
        if (srv_obj != NOOBJECT)
        {
            t_out_packet packet(m_cdp, m_source_server, srv_obj);
            packet.send_server_info();
        }
        break;
    }
    case PKTTP_SERVER_INFO:
        store_remote_server_description(/*TRUE*/);
        break;

    case PKTTP_REL_REQ:    // request for a relation
    { 
        char *pRel = NULL;
        t_relreq_pars rrp;
        if (!get_file_data(&rrp, sizeof(t_relreq_pars))) // ignore incomplete packet
            break;
        int err=0;  BOOL is_dp_server;
        memcpy(m_ApplID, rrp.apl_uuid, UUID_SIZE);
        if (ker_apl_id2name(m_cdp, m_ApplID, NULL, &m_ApplObj))
        {
            err = 1;
            monitor_msg(m_cdp, RE31);
        }
        // check specific servers
        else if (!merge_servers(rrp.dp_uuid, rrp.os_uuid, rrp.vs_uuid, &is_dp_server))
        {
            err = 2;
            monitor_msg(m_cdp, RE32);
        }
        // check token state
        else if (!is_dp_server && token_in_appl(m_ApplID))
        { 
            err = 4;
            monitor_msg(m_cdp, RE33);
        }
        // store "request" state, block pushing
        else
        {
            trecnum rec = get_appl_replrec(m_dest_server, m_source_server, NULL);
            if (rec == NORECNUM)
                err = 3;
            else
            { 
                t_replapl ra(REPLAPL_REQRECV, rrp.relname, rrp.get_def);
                if (tb_write_var(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, 0, sizeof(t_replapl), &ra))
                {
                    err = 3;
                    goto exit;
                }
//#ifdef NEWREL
                uns32 Size;
                bool  NewRel;
                NewRel = false;
                if (get_file_hdata(&pRel, &Size))
                {
                    tobjnum relobj = find2_object(m_cdp, CATEG_REPLREL, rrp.apl_uuid, rrp.relname);
                    if (relobj != NOOBJECT)
                    {
                        pRel[Size] = 0;
                        tptr def = ker_load_objdef(m_cdp, objtab_descr, relobj);
                        t_replrel relo(m_cdp, &relobj);
                        compil_info xCIo(m_cdp, def, replrel_comp);
	                    xCIo.univ_ptr = &relo;
                        compile_masked(&xCIo);
                        corefree(def);
                        t_replrel reln(m_cdp, NULL);
                        compil_info xCIn(m_cdp, pRel, replrel_comp); 
	                    xCIn.univ_ptr = &reln;
	                    compile_masked(&xCIn);
                        if ((relo.konflres != reln.konflres) || !RelCmp(&relo.r[0], &reln.r[0], FALSE) || !RelCmp(&relo.r[1], &reln.r[1], TRUE))
                        {
                            if (tb_write_var(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, sizeof(t_replapl), sizeof(Size), &Size))
                            {
                                err = 3;
                                goto exit;
                            }
                            if (tb_write_var(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, sizeof(t_replapl) + sizeof(Size), Size, pRel))
                            {
                                err = 3;
                                goto exit;
                            }                            
                            //DbgLogFile("Server zapisuji do %d", rec);
                            NewRel = true;
                        }
                    }
                }
                if (!NewRel)
                {
                    Size = 0;
                    if (tb_write_var(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, sizeof(t_replapl), sizeof(Size), &Size))
                    {
                        err = 3;
                        goto exit;
                    }
                }
                if (tb_write_atr_len(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, sizeof(t_replapl) + sizeof(Size) + Size))
                {
                    err = 3;
                    goto exit;
                }
                m_Err = NOERROR;
//#endif
                t_zcr gcr_val;
                memset(gcr_val, 0xff, ZCR_SIZE);
                tb_write_atr(m_cdp, rpltab_descr, rec, REPL_ATR_GCRLIMIT, gcr_val);
            }
        }
exit:
        if (err) // negative acknowledgement if application not found or internal error
        {
            t_out_packet packet(m_cdp, m_source_server);
            packet.send_sharing_control(PKTTP_REL_NOACK, NULL, err);
        }
        if (pRel)
            free(pRel);
        break;
    }
    case PKTTP_REL_ACK: // relation accepted, perforam all settings and unblock
    { 
        WBUUID dp_uuid, os_uuid, vs_uuid;
        char relname[OBJ_NAME_LEN + 1];
        if (get_file_data(m_ApplID, UUID_SIZE) && // ignore incomplete packet
            get_file_data(dp_uuid,  UUID_SIZE) &&
            get_file_data(os_uuid,  UUID_SIZE) &&
            get_file_data(vs_uuid,  UUID_SIZE) &&
            get_file_data(relname,  OBJ_NAME_LEN + 1))
        {
            trecnum aplrec = find_appl_replrec(m_dest_server, m_source_server, NULL);
            if (aplrec != NORECNUM && !ker_apl_id2name(m_cdp, m_ApplID, NULL, &m_ApplObj)) // ignore if sharing cancelled or application deleted
            {
                BOOL is_dp_server;
                if (!merge_servers(dp_uuid, os_uuid, vs_uuid, &is_dp_server))
                    monitor_msg(m_cdp, RE34);
                // create the relation:
                create_relation(m_source_server, relname, FALSE);
                sharing_prepared(aplrec, REPLAPL_IS);  // store new sharing state
                // unblock pushing from the other side:
                t_out_packet packet(m_cdp, m_source_server, NOOBJECT, m_ApplID);
                packet.send_sharing_control(PKTTP_REPL_UNBLOCK, NULL, 0);
                replicate_appl(m_source_server);
            }
        }
        break;
    }
    case PKTTP_REL_NOACK: // relation not accepted, store the state
    { 
        int errnum;
        if (get_file_data(&m_ApplID, UUID_SIZE) && get_file_data(&errnum, sizeof(int)))
        {
            trecnum aplrec = find_appl_replrec(m_dest_server, m_source_server, NULL);
            if (aplrec != NORECNUM) // ignore if sharing cancelled
                sharing_prepared(aplrec,   // store new sharing state
                                 errnum == 3 ? REPLAPL_INTERR   : errnum == 1 ? REPLAPL_DELETED :
                                 errnum == 2 ? REPLAPL_CONFLICT : REPLAPL_REQREJ);
        }
        break;
    }
    case PKTTP_REPL_UNBLOCK:  // unblock pushing
    { 
        if (get_file_data(&m_ApplID, UUID_SIZE)) // ignore incomplete packet
        {
            trecnum aplrec = find_appl_replrec(m_dest_server, m_source_server, NULL);
            if (aplrec != NORECNUM) // ignore if sharing cancelled
            {
                sharing_prepared(aplrec, REPLAPL_IS);  // store new sharing state
                replicate_appl(m_source_server);
            }
        }
        break;
    }

    case PKTTP_REPL_STOP:  // relation deleted by the other side, clear information
    { 
        if (get_file_data(&m_ApplID, UUID_SIZE)) // ignore incomplete packet
        {// erase all sharing data:
            remove_replic();
        }
        break;
    }
    }
    if (m_Err && m_packet_type != PKTTP_REPL_DATA)
    {
        monitor_msg(m_cdp, RE70);
        *(DWORD *)m_ErrMsg = ' ' + (' ' << 8) + (' ' << 16) + (' ' << 24);
        Get_server_error_text(m_cdp, m_Err & ~ERROR_RECOVERABLE, m_ErrMsg + 4, sizeof(m_ErrMsg) - 4);
        if (m_ErrMsg[4])  // translated
          trace_msg_general(m_cdp, TRACE_REPLICATION, m_ErrMsg, NOOBJECT);
        WriteReplErr(NULL, this, NORECNUM, NORECNUM, NORECNUM);
    }
    transact(m_cdp);
    // CALL _ON_REPLPACKET_RECEIVED
    repl_in_counter--;  
#ifdef WINS
    WriteSlaveThreadCounter();
#endif
    CallOnReceived();
    return(m_Err == NOERROR || !(m_Err & ERROR_RECOVERABLE));
}

BOOL t_in_packet :: RelCmp(t_relatio *relo, t_relatio *reln, BOOL Acc)
{
    if (Acc && (relo->hours != reln->hours || relo->minutes != reln->minutes || relo->tabs.count() != reln->tabs.count()))
        return(FALSE);
    for (int i = 0; i < relo->tabs.count(); i++)
    {
        t_reltab *rto = (t_reltab *)relo->tabs.acc0(i);
        t_reltab *rtn = (t_reltab *)reln->tabs.acc0(i);
        if (rto->can_del != rtn->can_del)
            return(FALSE);
        if (rto->atrlist.count() != rtn->atrlist.count())
            return(FALSE);
        if (lstrcmpi(rto->tabname, rtn->tabname))
            return(FALSE);
        if (lstrcmpi(rto->condition, rtn->condition))
            return(FALSE);
        for (int a = 0; a < rto->atrlist.count(); a++)
        {
            if (lstrcmpi((char *)rto->atrlist.acc0(a), (char *)rtn->atrlist.acc0(a)))
                return(FALSE);
        }
    }
    return(TRUE);
}
//
// Volani systemoveho triggeru _ON_REPLPACKET_RECEIVED
//
void t_in_packet :: CallOnReceived()
{
    if (!OnReceived)
        return;
    char cmd[300];
    int  len;
    lstrcpy(cmd, "CALL _SYSEXT._ON_REPLPACKET_RECEIVED(X'"); len  = 39;
    bin2hex(cmd + len, m_source_server, UUID_SIZE);          len += 2 * UUID_SIZE;
    char *p = m_ErrMsg;
    if (*(DWORD *)m_ErrMsg ==  ' ' + (' ' << 8) + (' ' << 16) + (' ' << 24))
        p += 4;
    len += sprintf(cmd + len, "', \"%s\", %d, %d, %d, \"%s\", %d, ", m_SrvrName, m_packet_type, m_PacketNo, m_Err ? m_Err : m_Warn, p, m_ErrPos);
    timestamp2str(m_DT, cmd + len, 5);
    lstrcat(cmd + len, ")");
    if (!exec_direct(m_cdp, cmd))
      transact(m_cdp);
}
//
// Zapise informaci o protejsim serveru do SRVTAB
//
tobjnum t_in_packet :: store_remote_server_description(/*BOOL answer*/)
{ 
    t_server_pack sp;

    if (!get_file_data(&sp, sizeof(t_server_pack)))
        return(NOOBJECT);  // ignore incomplete packet
    // write server info:
    monitor(m_cdp, DIR_INPCK, sp.server_name, m_packet_type);

    if (memcmp(sp.uuid, my_server_id, UUID_SIZE) == 0)
        return(NOOBJECT);

    tobjnum srvr_obj = find_object_by_id(m_cdp, CATEG_SERVER, sp.uuid);
    if (srvr_obj == NOOBJECT)
    {
        srvr_obj = tb_new(m_cdp, srvtab_descr, FALSE, NULL);
        if (srvr_obj == NORECNUM)
        {
            m_Err = m_cdp->get_return_code();
            if (!ErrorCausesNack)
                m_Err |= ERROR_RECOVERABLE;
            return(NOOBJECT);
        }
        uns32 zero = 0;
        tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_RECVNUM,   &zero);
        tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_SENDNUM,   &zero);
        wbbool ackn = wbtrue;  
        tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_ACKN,      &ackn);
        tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_SENDFNAME, NULLSTRING);
        tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_UUID,      sp.uuid);
    }
    t_server_work *server = srvs.get_server(m_cdp, sp.uuid);
    if (!server)
    {
        commit(m_cdp);
        return(NOOBJECT);
    }
    wbbool usealt = NONEBOOLEAN;
    tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_NAME,  sp.server_name);
    // IF info prislo z primarni adresy, odpovidat na primarni adresu
    if (*(uns32 *)(sp.addr1 + lstrlen((char*)sp.addr1) + 1) == 0xCCCCCCCC)
        usealt = FALSE;
    tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_ADDR1, sp.addr1);
    // IF info prislo ze sekundarni adresy, odpovidat na sekundarni adresu
    if (*(uns32 *)(sp.addr2 + lstrlen((char*)sp.addr2) + 1) == 0xCCCCCCCC)
        usealt = TRUE;
    tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_ADDR2, sp.addr2);
    if (usealt != NONEBOOLEAN)
      tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_USEALT, &usealt);
    load_var_len_attribute(srvr_obj, SRV_ATR_PUBKEY);
    lstrcpy(server->srvr_name, sp.server_name);
    // list shareable applications:
    // na nic se nepouziva
    //if (answer)
    //  load_var_len_attribute(m_cdp, fhnd, SRV_TABLENUM, rec, SRV_ATR_APPLS);
    commit(m_cdp);
    return((tobjnum)srvr_obj);
}
//
// Zapise do atribut promenne delky z paketu do databaze 
//
void t_in_packet :: load_var_len_attribute(trecnum rec, tattrib attr)
{ 
    tptr  data;
    uns32 len;
    if (get_file_hdata(&data, &len))
    {
        if (len)
        {
            tb_write_var(m_cdp, srvtab_descr, rec, attr, 0, len, data);
            free(data);
        }
        tb_write_atr_len(m_cdp, srvtab_descr, rec, attr, len);
    }
}
//
// Merges specifis server uuids, returns FALSE if cannot
//
BOOL null_uuid(const WBUUID uuid)
{ for (int i=0;  i<UUID_SIZE;  i++) if (uuid[i]) return FALSE;
  return TRUE;
}

BOOL t_in_packet :: merge_servers(WBUUID dp_uuid, WBUUID os_uuid, WBUUID vs_uuid, BOOL *is_dp_server)
{ 
    apx_header ap;
    *is_dp_server = TRUE;
    memset(&ap, 0, sizeof(ap));
    if (tb_read_var(m_cdp, objtab_descr, m_ApplObj, OBJ_DEF_ATR, 0, sizeof(apx_header), (tptr)&ap))
        return(FALSE);
    if (!null_uuid(dp_uuid))
    {
        if (null_uuid(ap.dp_uuid))
            memcpy(ap.dp_uuid, dp_uuid, UUID_SIZE);
        else if (memcmp(ap.dp_uuid, dp_uuid, UUID_SIZE))
            return(FALSE);
    }
    else if (null_uuid(ap.dp_uuid))
        *is_dp_server = FALSE; // both are null
    if (!null_uuid(os_uuid))
    {
        if (null_uuid(ap.os_uuid))
            memcpy(ap.os_uuid, os_uuid, UUID_SIZE);
        else if (memcmp(ap.os_uuid, os_uuid, UUID_SIZE))
            return(FALSE);
    }
    //if (!null_uuid(vs_uuid))
    //  if (null_uuid(ap.vs_uuid)) memcpy(ap.vs_uuid, vs_uuid, UUID_SIZE);
    //  else if (memcmp(ap.vs_uuid, vs_uuid, UUID_SIZE)) return FALSE;
    return(!tb_write_var(m_cdp, objtab_descr, m_ApplObj, OBJ_DEF_ATR, 0, sizeof(apx_header), &ap));
}
//
// Testuje, zda ma aplikace peska
//
BOOL t_in_packet :: token_in_appl(WBUUID appl_uuid)
{ 
    char query[60 + OBJ_NAME_LEN + 2 * UUID_SIZE];
    make_apl_query(query, "TABTAB", appl_uuid);
    cur_descr *cd;
    tcursnum cursnum = open_working_cursor(m_cdp, query, &cd);
    if (cursnum == NOCURSOR)
        return(FALSE);
    trecnum r, trec;  BOOL token=FALSE;
    for (r = 0; !token && r < cd->recnum; r++)
    {
        tcateg categ;
        cd->curs_seek(r, &trec);
        tb_read_atr(m_cdp, tabtab_descr, trec, OBJ_CATEG_ATR, (tptr)&categ);
        if (categ == CATEG_TABLE)  // not a link
        {
            table_descr_auto td(m_cdp, trec);
            if (td->me() && td->token_attr != NOATTRIB)
                token=TRUE;
        }
    }
    free_cursor(m_cdp, cursnum);
    return(token);
}
//
// store the new state of application sharing, initialize gcr:
//
void t_in_packet :: sharing_prepared(trecnum aplrec, t_replapl_states newstate)
{
    t_replapl ra;
    tb_read_var(m_cdp, rpltab_descr, aplrec, REPL_ATR_REPLCOND, 0, sizeof(t_replapl), (tptr)&ra);
    ra.state = newstate;
    tb_write_var(m_cdp, rpltab_descr, aplrec, REPL_ATR_REPLCOND, 0, sizeof(t_replapl), &ra);
    // unblock my pushing:
    if (newstate == REPLAPL_IS)
    {
        t_zcr gcr_val;  
        if (ra.want_def & 2)
            memcpy(gcr_val, header.gcr, ZCR_SIZE);
        else
            memset(gcr_val, 0,          ZCR_SIZE);  
        tb_write_atr(m_cdp, rpltab_descr, aplrec, REPL_ATR_GCRLIMIT, gcr_val);
    }
}
//
// Osetri chybu vracenou v paketu typu PKTTP_REPL_NOACK
//
void t_in_packet :: process_nack()
{
    t_nackx nack;
    
    if (!get_file_data(&nack, sizeof(t_nack)))
        return;
    tobjnum srvr_obj = find_object_by_id(m_cdp, CATEG_SERVER, m_source_server);
    if (srvr_obj != NOOBJECT)
    {
        wbbool ackn = NONEBOOLEAN;
        tb_write_atr(m_cdp, srvtab_descr, srvr_obj, SRV_ATR_ACKN, &ackn);
        transact(m_cdp);
    }
    char buf[160];
    monitor_msg(m_cdp, RE63, m_SrvrName, nack.m_PacketNo);
    if (nack.m_RecNum == NORECNUM)
        monitor_msg(m_cdp, RE75, nack.m_Appl, nack.m_Table);
    else
        monitor_msg(m_cdp, RE66, nack.m_Appl, nack.m_Table, nack.m_RecNum);
    if (nack.m_Err == KEY_DUPLICITY || nack.m_Err == NO_REPL_UNIKEY || nack.m_Err == OBJECT_NOT_FOUND)
        lstrcpy(m_cdp->errmsg, nack.m_Table);
    *(DWORD *)m_ErrMsg = ' ' + (' ' << 8) + (' ' << 16) + (' ' << 24);
    t_nackadi adi;
    DWORD CurPos = SetFilePointer(m_fhnd, 0, NULL, FILE_CURRENT);
    DWORD rd;
    BOOL  OK;
    bool  HaveApplID = false;
    while ((OK = ReadFile(m_fhnd, &adi, 2 * sizeof(uns32), &rd, NULL)) && rd == 2 * sizeof(uns32))
    {
        if (adi.m_Type == NAX_APPLUUID && ReadFile(m_fhnd, m_ApplID, sizeof(WBUUID), &rd, NULL) && rd == sizeof(WBUUID))
            HaveApplID = true;
        else if (adi.m_Type == NAX_ERRSUPPL)
            ReadFile(m_fhnd, m_cdp->errmsg, adi.m_Size, &rd, NULL);
        else if ((char)adi.m_Type == '(')
            break;
        else
            SetFilePointer(m_fhnd, adi.m_Size, NULL, FILE_CURRENT);
    }           
    SetFilePointer(m_fhnd, CurPos, NULL, FILE_BEGIN);
    if (Get_server_error_text(m_cdp, nack.m_Err, m_ErrMsg + 4, sizeof(m_ErrMsg) - 4)) // translated
      trace_msg_general(m_cdp, TRACE_REPLICATION, m_ErrMsg, NOOBJECT);
    else  
      monitor_msg(m_cdp, RE64, nack.m_Err);
    if (!HaveApplID)
        ker_apl_name2id(m_cdp, nack.m_Appl, m_ApplID, NULL);
    m_TableObj = find2_object(m_cdp, CATEG_TABLE, m_ApplID, nack.m_Table);
    t_kdupl_info *di = NULL;
    trecnum RecLoc = NORECNUM, RecCnf = NORECNUM;
    while ((OK = ReadFile(m_fhnd, &adi, 2 * sizeof(uns32), &rd, NULL)) && rd == 2 * sizeof(uns32))
    {
        if (adi.m_Type == NAX_UNIKEY || adi.m_Type == NAX_PRIMARY)
        {
            RecLoc = Unikey2RecNum(&nack, &adi);
            if (RecLoc == NORECNUM)
                return;
        }
        else if (adi.m_Type == NAX_KDPLHDR)
        {
            di = nack_dupl(adi.m_Size, RecLoc);
            if (!di)
                return;
        }
        // Kvuli kompatibilite s predchozimi verzemi
        else if ((char)adi.m_Type == '(')
        {
            DWORD rd;
            memcpy(buf + 4, &adi, 2 * sizeof(uns32));
            if (ReadFile(m_fhnd, buf + 4 + 2 * sizeof(uns32), sizeof(buf) - 4 - 2 * sizeof(uns32), &rd, NULL) && rd > 0)
                monitor_msg(m_cdp, buf);
        }
        else
        {
            SetFilePointer(m_fhnd, adi.m_Size, NULL, FILE_CURRENT);
        }
    }
    if (OK || rd == 0)
    {
        WriteReplErr(di, &nack, RecLoc, nack.m_RecNum, m_RecCnf);
        transact(m_cdp);
    }
    else
        m_Err = OS_FILE_ERROR;
    if (di)
        delete di;
}
//
// Konvertuje unikey na cislo zaznamu
//
trecnum t_in_packet :: Unikey2RecNum(t_nackx *nack, t_nackadi *adi)
{
    if (!get_file_data(nack->m_KeyVal, adi->m_Size))
        return(NORECNUM);
    nack->m_KeyType = adi->m_Type;
    nack->m_KeySize = adi->m_Size;
    table_descr_auto td(m_cdp, m_TableObj);
    if (!td->me())
        return(NORECNUM);
    tattrib unikey = 0;
    for (int a = 1; a < td->attrcnt; a++)
    { 
        attribdef *att = &td->attrs[a];
        if (strncmp(att->name, "_W5_", 4))
            break;
        if (!lstrcmp(att->name + 4, "UNIKEY"))
        { 
            unikey  = a;
            break;
        }
    }
    dd_index *cc;
    int ind;
    for (ind = 0, cc = td->indxs; ind < td->indx_cnt;  ind++, cc++)
    {
            if (unikey)
            { 
                if (cc->atrmap.has(unikey))
                    break;
            }
            else if (cc->ccateg == INDEX_PRIMARY)
            {
                break;
            }
    }
    if (ind >= td->indx_cnt)
        return(NORECNUM);
    dd_index *indx = &td->indxs[ind];
    *(trecnum *)(nack->m_KeyVal + nack->m_KeySize) = 0;
    trecnum RecNum = NORECNUM;
    m_cdp->index_owner = td->tbnum;
    find_key(m_cdp, indx->root, indx, nack->m_KeyVal, &RecNum, true);
    return(RecNum);
}
//
// Zobrazi informaci o neprijeti paketu v dusledku duplicity klicu
//
t_kdupl_info *t_in_packet :: nack_dupl(int RowSize, trecnum rInc)
{
    t_nackadi adi;
    t_kdupl_info *di = NULL;
    char *data = NULL;
    BOOL OK = FALSE;

    data = (char *)corealloc(RowSize, 2);
    if (!data)
        return(NULL);
    // Nacist hlavicky
    if (!get_file_data(data, RowSize))
        goto exit;
    // Precist zahlavi dalsiho radku
    if (!get_file_data(&adi, 2 * sizeof(uns32)))
        goto exit;
    // Smaze se v process_nack
    di = new t_kdupl_info(adi.m_Type == NAX_KDPLREM);
    if (!di)
        goto exit;
    char *p, *pe;
    int Size;
    // Zapsat hlavicky do tabulky
    for (p = data, pe = data + RowSize; p < pe; p += Size + 1)
    {
        Size = lstrlen(p);
        di->AddVal(0, p, Size);
    }
    int Cnf;
    Cnf = 2;
    // IF duplicita pri prepisu existujiciho zaznamu, zapsat do tabulky hodnoty existujiciho zaznamu
    if (adi.m_Type == NAX_KDPLLOC)
    {
        corefree(data);
        data = (char *)corealloc(adi.m_Size, 2);
        if (!data)
            goto exit;
        if (!get_file_data(data, adi.m_Size))
            goto exit;
        for (p = data, pe = data + adi.m_Size; p < pe; p += Size + 1)
        {
            Size = lstrlen(p);
            di->AddVal(2, p, Size);
        }
        if (!get_file_data(&adi, 2 * sizeof(uns32)))
            goto exit;
        Cnf = 3;
    }
    // Zapsat do tabulky data zdrojoveho zaznamu
    corefree(data);
    data = (char *)corealloc(adi.m_Size, 2);
    if (!data)
        goto exit;
    if (!get_file_data(data, adi.m_Size))
        goto exit;
    di->GetRow(1)->AddVal(ATT_INT32, &rInc, 10);
    for (p = data + 1, pe = data + adi.m_Size; p < pe; p += Size + 1)
    {
        Size = lstrlen(p);
        di->AddVal(1, p, Size);
    }
    // Zapsat do tabulky data konfliktniho zaznamu
    if (!get_file_data(&adi, 2 * sizeof(uns32)))
        goto exit;
    corefree(data);
    data = (char *)corealloc(adi.m_Size, 2);
    if (!data)
        goto exit;
    if (!get_file_data(data, adi.m_Size))
        goto exit;
    for (p = data, pe = data + adi.m_Size; p < pe; p += Size + 1)
    {
        Size = lstrlen(p);
        di->AddVal(Cnf, p, Size);
    }
    di->Trace(m_cdp, TRACE_REPL_CONFLICT);
    const char *r;
    r = di->GetRow(Cnf)->GetHdr()->GetNext()->GetVal();
    str2uns(&r, &m_RecCnf);
    OK = TRUE;

exit:

    if (!OK && di)
    {
        delete di;
        di = NULL;
    }
    if (data)
        corefree(data);
    return(di);
}
//
// processes incomming replication packet
//
BOOL t_in_packet :: process_incomming_repl()
{   
    memcpy(m_cdp->luo, m_source_server, UUID_SIZE);  // source server uuid as luo
    // cycle on applications                         
    do 
    {
        // read and check appl uuid:
        if (!get_file_data(m_ApplID, UUID_SIZE))
            break;
        if (null_uuid(m_ApplID)) // last application processed, exit
            break;
        // find application, skip if deleted:
        DWORD appl_data_size, appl_start_pos;
        if (!get_file_data(&appl_data_size, sizeof(DWORD)))
            break;
        appl_start_pos = SetFilePointer(m_fhnd, 0, NULL, FILE_CURRENT);
        trecnum aplrec = find_appl_replrec(m_dest_server, m_source_server, NULL);
        if (aplrec == NORECNUM || !ApplPropsInit(m_cdp, m_ApplID, m_source_server))  // application deleted, skip its data
        {
            SetFilePointer(m_fhnd, appl_start_pos + appl_data_size, NULL, FILE_BEGIN);
            goto next_app;
        }
        // processing the application data:
        {
            t_short_term_schema_context stsc(m_cdp, m_ApplID);
            ker_apl_id2name(m_cdp, m_ApplID, m_Appl, NULL);
            uns8 aux_attrmap[256/8];
            tb_read_atr(m_cdp, rpltab_descr, aplrec, REPL_ATR_ATTRMAP, (tptr)aux_attrmap);
            m_ConflResolution = aux_attrmap[3];
            // cycle on tables in the application
            do
            { 
                if (!get_file_data(m_Table, OBJ_NAME_LEN + 1))
                    goto ex;
                if (!m_Table[0]) // table list delimiter, normal end of tables
                    break;
                // prepare table:
                m_TableObj = find_object(m_cdp, CATEG_TABLE, m_Table);
                if (m_TableObj == NOOBJECT)  // skip the rest of application data
                { 
                    lstrcpy(m_cdp->errmsg, m_Table);
                    m_Err = OBJECT_NOT_FOUND;
                    goto ex;
                }
                table_descr_auto tda(m_cdp, m_TableObj);
                if (!tda->me())
                { 
                    m_Err = m_cdp->get_return_code();
                    if (!ErrorCausesNack)
                        m_Err |= ERROR_RECOVERABLE;
                    goto ex;
                }
                // get remote and local attr maps:
                if (!get_file_data(attrmap.ptr(), sizeof(attrmap)))
                    goto ex;
                m_local_attrmap.clear();
                trecnum trec = find_appl_replrec(m_source_server, m_dest_server, m_Table);
                if (trec != NORECNUM)
                    tb_read_atr(m_cdp, rpltab_descr, trec, REPL_ATR_ATTRMAP, (tptr)m_local_attrmap.ptr());
                if (!TableInit(tda->me()))  // returns FALSE when index not found
                {
                    m_Err = NO_REPL_UNIKEY;
                    lstrcpy(m_cdp->errmsg, m_Table);
                    goto ex;
                }
                if (keyattr)
                  if (attrmap.has(keyattr))
                    m_local_attrmap.set(keyattr);
                m_indx    = &td->indxs[indexnum];
                m_KeyType = unikey ? NAX_UNIKEY : NAX_PRIMARY;
                m_KeySize = keysize;
//                m_rec_counter = 0;
        
                uns8  dltd;
                if (!get_file_data(&dltd, 1))
                    goto ex; 

                if (!dltd)              // IF tabulka zacina zmenami
                {
                    DWORD NextOff;
                    DWORD ChngOff = SetFilePointer(m_fhnd, 0, NULL, FILE_CURRENT);
                    dltd = SkipChanges();                                           // Preskocit zmeny
                    if (!dltd)              
                        goto ex;
                    if (dltd != 0xFF && ProcessDelet() == 1)                        // Zpracovat smazane
                        goto ex;
                    NextOff = SetFilePointer(m_fhnd, 0, NULL, FILE_CURRENT);
                    SetFilePointer(m_fhnd, ChngOff, NULL, FILE_BEGIN);              // Vratit se na zmeny                    
                    if (!ProcessChanges())                                          // Zpracovat zmeny
                        goto ex;
                    SetFilePointer(m_fhnd, NextOff, NULL, FILE_BEGIN);              // Skocit na konec tabulky
                }
                else if (dltd != 0xFF)  // ELSE IF tabulka zacina smazanymi
                {
                    dltd = ProcessDelet();                                          // Zpracovat smazane
                    if (dltd == 1)
                        goto ex;
                    if (dltd == 0 && !ProcessChanges())                             // Zpracovat zmeny
                        goto ex;
                }
            } 
            while (TRUE); // cycle on tables in the application
        } // short-term application context

next_app:

        m_Err = transact(m_cdp);
        if (m_Err)
            goto ex;
    } 
    while (TRUE); // cycle on applications

ex:

    transact(m_cdp);
    tb_read_atr(m_cdp, srvtab_descr, 0, SRV_ATR_UUID, (tptr)m_cdp->luo); // back the local uuid as luo
    return((sig16)m_Err <= 0);
}
//
// Zpracovani smazanych zaznamu
//
// Vraci: 0    - Nasleduji zmeny
//        1    - Chyba
//        0xFF - Konec tabulky
//
uns8 t_in_packet :: ProcessDelet()
{
    sig16 token_val;
    uns8  dltd;

    do
    {
        if (token_offset)
            if (!get_file_data(&token_val, sizeof(sig16)))
                return(1);
        // read key value:
        if (!get_file_data(m_KeyVal, m_KeySize))
            return(1);
        if (m_local_attrmap.has(0))  // accept incoming deletion
        {
            // find or insert the record
            m_cdp->index_owner = td->tbnum;
            *(trecnum*)(m_KeyVal + m_KeySize) = 0;
            if (find_key(m_cdp, m_indx->root, m_indx, m_KeyVal, &m_RecNum, true))
                tb_del(m_cdp, td->me(), m_RecNum);
        }
        // transacct, if transaction is very big:
        if (m_cdp->tl.log.total_records() > 15000 /* || ++m_rec_counter > 20 */) 
        {
//            m_rec_counter = 0;
            m_Err = transact(m_cdp);
            if (m_Err)
                return(1);
        }
        if (!get_file_data(&dltd, sizeof(dltd)))
            return(1);
    }
    while (dltd != 0 && dltd != 0xFF);
    return(dltd);
}
//
// Zpracovani zmenenych zaznamu
//
BOOL t_in_packet :: ProcessChanges()
{ 
    sig16 token_val;
    uns8  dltd;
    do  // cycle on records in the packet
    {
        if (token_offset)
            if (!get_file_data(&token_val, sizeof(sig16)))
                return FALSE;
        // read key value:
        if (!get_file_data(m_KeyVal, m_KeySize))
            return FALSE;
        // find or insert the record
        m_cdp->index_owner = td->tbnum;
        *(trecnum*)(m_KeyVal + m_KeySize) = 0;
        if (find_key(m_cdp, m_indx->root, m_indx, m_KeyVal, &m_RecNum, true))
        { 
            if (!store_record(token_val, FALSE))
                return(FALSE);
        }
        else // replicating to a non-existing record
        { 
            wbbool Del = false;
            table_descr *tdr = GetReplDelRecsTable(m_cdp);
            if (tdr)
            { 
                char query[1200];
                int len;
                lstrcpy(query, "SELECT * FROM _SYSEXT._REPLDELRECS WHERE SCHEMAUUID=X'"); len  = 54;
                bin2hex(query + len, m_ApplID,   UUID_SIZE);                  len += 2 * UUID_SIZE;
                lstrcpy(query + len, "' AND TABLENAME='");                    len += 17;
                lstrcpy(query + len, m_Table);                                len += lstrlen(m_Table);
                if (m_KeyType == NAX_UNIKEY && RDR_W5_UNIKEY != (tattrib)-1)
                {
                    lstrcpy(query + len, "' AND W5_UNIKEY=X'");               len += 18;
                    bin2hex(query + len, (const uns8 *)m_KeyVal, m_KeySize);  len += 2 * m_KeySize;
                    lstrcpy(query + len, "'");
                }
                else
                {
                    uns8 tp = tdr->attrs[RDR_UNIKEY].attrtype;
                    lstrcpy(query + len, "' AND UNIKEY=");                    len += 13;
                    if (tp == ATT_STRING)
                    {
                        lstrcpy(query + len, "'");                            len++;
                    }
                    else if (tp == ATT_BINARY)
                    {
                        lstrcpy(query + len, "X'");                           len += 2;
                    }
                    superconv(tp, ATT_STRING, m_KeyVal, query + len, tdr->attrs[RDR_UNIKEY].attrspecif, t_specif(0, 0, 1, 0), NULL);
                    len += (int)strlen(query + len);
                    if (tp == ATT_STRING || tp == ATT_BINARY)
                        lstrcpy(query + len, "'");
                }
                cur_descr *cd;
                tcursnum curs = open_working_cursor(m_cdp, query, &cd);
                Del = curs != NOCURSOR && cd->recnum != 0;
                free_cursor(m_cdp, curs);
            }
            if (Del)
                SkipRestOfRecord(0);
            else
            {
                m_cdp->ker_flags |= KFL_DISABLE_DEFAULT_VALUES;     // Kvuli default hodnotam unikatnich klicu
                m_RecNum = tb_new(m_cdp, td->me(), FALSE, NULL);
                m_cdp->ker_flags &= ~KFL_DISABLE_DEFAULT_VALUES;
                if (m_RecNum==NORECNUM)
                {
                    m_Err = m_cdp->get_return_code();
                    if (!ErrorCausesNack)
                        m_Err |= ERROR_RECOVERABLE;
                    return(FALSE);
                }
                //m_cdp->set_return_code(ANS_OK);
                if (!store_record(token_val, TRUE))
                    return(FALSE);
            }
        }
        // transacct, if transaction is very big:
        if (m_cdp->tl.log.total_records() > 15000) 
        {
            m_Err = transact(m_cdp);
            if (m_Err)
                return FALSE;
        }
        if (!get_file_data(&dltd, sizeof(dltd)))
            return FALSE;
    } 
    while (dltd == 0);
    return TRUE;
}

//
// Preskoci zaznamy o zmenach dokud nenajde zaznam o smazani nebo konec tabulky
//                
// Vraci:   0     - Chyba
//          0xFF  - Konec tabulky
//          jinak - Nasleduji zaznamy o zmenach
//
uns8 t_in_packet :: SkipChanges()
{ 
    uns8    dltd;
    DWORD   Off;
    
    do
    {
        Off = m_KeySize;                // Prskocit UNIKEY
        if (token_offset)               // Preskocit peska
            Off += sizeof(sig16);
        if (!SkipRestOfRecord(Off))
            return(0);
        if (!get_file_data(&dltd, sizeof(dltd)))
            return(0);
    }
    while (dltd == 0);
    return(dltd);
}

bool t_in_packet :: SkipRestOfRecord(DWORD Off)
{
    if (detect_offset)              // Preskocit ZCR
        Off += ZCR_SIZE;

    for (int a = 1; a < td->attrcnt; a++)
    {
        if (!attrmap.has(a))  // replicated attribute
            continue;
        attribdef *att = &td->attrs[a];
        if (att->attrmult > 1)
        { 
            uns16 count;
            SetFilePointer(m_fhnd, Off, NULL, FILE_CURRENT);
            if (!get_file_data(&count, CNTRSZ))
                return false;
            if (IS_HEAP_TYPE(att->attrtype))
            { 
                for (int index = 0; index < count; index++)
                {
                    if (!get_file_data(&Off, sizeof(Off)))
                        return false;
                    SetFilePointer(m_fhnd, Off, NULL, FILE_CURRENT);
                }
                Off = 0;
            }
            else // fixed-len type
            {
                Off = typesize(att) * count;
            }
        }
        else  // not a multiattribute
        {
            if (IS_HEAP_TYPE(att->attrtype))
            {
                SetFilePointer(m_fhnd, Off, NULL, FILE_CURRENT);
                if (!get_file_data(&Off, sizeof(Off)))
                    return false;
            }
            else
            {
                Off += typesize(att);
            }
        }
    }
    if (Off)
        SetFilePointer(m_fhnd, Off, NULL, FILE_CURRENT);
    return true;
}
//
// From file to database.
// Can be optimized: if all vales are compared first, only some indicies
// may be affected. Moving file pointer back would be necessary then.
//
static BOOL null_zcr(const char * dt)
{ return !dt[0] && !dt[1] && !dt[2] && !dt[3] && !dt[4] && !dt[5]; }

BOOL t_in_packet :: store_record(sig16 token_val, BOOL added)
{ 
    tfcbnum fcbn, fcbna;
    tptr    dt, dta;
    int     a, size;
    BOOL    do_copy, changed;
    char    valbuf[MAX_FIXED_STRING_LENGTH];  
    uns8 detect_val[ZCR_SIZE];
    t_simple_update_context sic(m_cdp, td, 0);    
    if (!sic.lock_roots()) return FALSE;

    dt = fix_attr_write(m_cdp, td, m_RecNum, DEL_ATTR_NUM, &fcbn);  // this locks the record, must be before removing from indices
    if (!dt)
    {   
        m_Err = m_cdp->get_return_code();
        if (!ErrorCausesNack)
            m_Err |= ERROR_RECOVERABLE;
        return(FALSE);
    }
    if (!sic.inditem_remove(m_RecNum, NULL))  /* remove from ALL indicies */
    {   unfix_page(m_cdp, fcbn);
        m_Err = m_cdp->get_return_code();
        if (!ErrorCausesNack)
            m_Err |= ERROR_RECOVERABLE;
        return(FALSE);
    }
    m_cdp->ker_flags |= KFL_STOP_INDEX_UPDATE;  // must not return now!
    // token:
    do_copy = TRUE;
    if (token_offset)
    { 
        if (added)
            *(sig16 *)(dt + token_offset) = 0; // initial value
        if (*(sig16 *)(dt + token_offset) & 0x8000) // I have the token, no replication!
            do_copy = FALSE;
        else if (HeIsDP)
            *(sig16 *)(dt + token_offset) = token_val; // granted or not
        else if (IAmDP)
            if ((token_val & 0x1fff) == (*(sig16 *)(dt + token_offset) & 0x1fff))
            { 
                if (token_val & 0x8000) // returning token
                    *(sig16 *)(dt + token_offset) = token_val & 0x9fff; // token returned
            }
            else
                do_copy = FALSE;
    }
    // detecting conflict:
    if (detect_offset)
    {   if (HeIsFather)  // from the father to a son
        {
            if (!get_file_data(detect_val, ZCR_SIZE))
                goto ex;
            if (!added && memcmp(dt + zcr_offset, dt + detect_offset, ZCR_SIZE)) // conflict
                if (!null_zcr(dt + detect_offset))  // unless just added
                { 
                    if (!m_ConflResolution)
                    { 
                        do_copy = FALSE;
                        m_Warn  = WRN_REPLCNF_LOCAL;
                    }
                    else
                        m_Warn  = WRN_REPLCNF_INCOMM;
                    konfl_info();
                }
        }
        else // if (IAmFather) // from a son to the father
        { 
            if (!get_file_data(detect_val, ZCR_SIZE))
                goto ex;
            if (!added && memcmp(dt + zcr_offset, detect_val, ZCR_SIZE)) // conflict
                if (!null_zcr((tptr)detect_val))  // unless inserted by a son
                    if (!luo_offset || memcmp(dt + luo_offset, m_cdp->luo, UUID_SIZE))
                    {
                        if (!m_ConflResolution)
                        {
                            do_copy = FALSE;
                            m_Warn  = WRN_REPLCNF_LOCAL;
                        }
                        else
                            m_Warn  = WRN_REPLCNF_INCOMM;
                        konfl_info();
                    }
        }
    }
    // load values from the file and compare them to the old ones
    changed = FALSE;
    for (a = 1; a < td->attrcnt; a++)
    {
        if (!attrmap.has(a))  // replicated attribute
            continue;
        BOOL do_copy_atr = do_copy && m_local_attrmap.has(a);
        attribdef *att = &td->attrs[a];
        if (att->attrmult > 1)
        { 
            uns16 count;
            if (!get_file_data(&count, CNTRSZ))
                goto ex;
            if (tb_write_ind_num(m_cdp, td, m_RecNum, a, count))
            {
                m_Err = m_cdp->get_return_code();
                if (!ErrorCausesNack)
                    m_Err |= ERROR_RECOVERABLE;
                goto ex;
            }
            for (int index = 0; index < count; index++)
            {
                dta = fix_attr_ind_write(m_cdp, td, m_RecNum, a, index, &fcbna);
                if (!dta) 
                { 
                    m_Err = m_cdp->get_return_code();
                    if (!ErrorCausesNack)
                        m_Err |= ERROR_RECOVERABLE;
                    goto ex;
                }
                if (IS_HEAP_TYPE(att->attrtype))
                { 
                    if (!hp_rewrite((tpiece*)dta, a, index, &changed, do_copy_atr))
                    { 
                        unfix_page(m_cdp, fcbna);
                        goto ex;
                    }
                }
                else // fixed-len type
                {
                    size = typesize(att);
                    if (!changed)  // optimization
                    {
                        if (!get_file_data(valbuf, size))
                        {
                            unfix_page(m_cdp, fcbna);
                            goto ex; 
                        }
                        if (do_copy_atr && memcmp(dta, valbuf, size))
                        {
                            changed = TRUE;
                            memcpy(dta, valbuf, size);
                        }
                    }
                    else if (do_copy_atr)
                    {
                        if (!get_file_data(dta, size))
                        {
                            unfix_page(m_cdp, fcbna);
                            goto ex;
                        }
                    }
                    else if (!get_file_data(valbuf, size))
                    {
                        unfix_page(m_cdp, fcbna);
                        goto ex;
                    }
                }
                unfix_page(m_cdp, fcbna);
            }
        }
        else  // not a multiattribute
        {
            if (!att->attrpage) // take from the fixed page
                dta = dt + att->attroffset;
            else
            {
                dta = fix_attr_write(m_cdp, td, m_RecNum, a, &fcbna);
                if (!dta) 
                {
                    m_Err = m_cdp->get_return_code();
                    if (!ErrorCausesNack)
                        m_Err |= ERROR_RECOVERABLE;
                    //DbgLogWrite("fix_attr");
                    break; 
                }
            }
            if (IS_HEAP_TYPE(att->attrtype))
            {
                if (!hp_rewrite((tpiece*)dta, a, NOINDEX, &changed, do_copy_atr))
                {
                    if (att->attrpage)
                        unfix_page(m_cdp, fcbna);
                    goto ex;
                }
            }
            else
            {
                size = typesize(att);
                if (!changed)  // optimization
                {
                    if (!get_file_data(valbuf, size))
                    {
                        if (att->attrpage)
                            unfix_page(m_cdp, fcbna);
                        //DbgLogWrite("get_file_data !changed");
                        goto ex;
                    }
                    if (do_copy_atr && memcmp(dta, valbuf, size))
                    { 
                        changed = TRUE;
                        memcpy(dta, valbuf, size);
                    }
                }
                else if (do_copy_atr)
                {
                    if (!get_file_data(dta, size))
                    {
                        if (att->attrpage)
                            unfix_page(m_cdp, fcbna);
                        goto ex; 
                    }
                }
                else
                    if (!get_file_data(valbuf, size))
                    {
                        if (att->attrpage)
                            unfix_page(m_cdp, fcbna);
                        //DbgLogWrite("get_file_data read only");
                        goto ex;
                    }
                // update unique_counter if value of an unique attribute stored:
                if (a == keyattr)
                {
                    td->unique_value_cache.register_used_value(m_cdp, get_typed_unique_value(dta + UUID_SIZE, ATT_INT32));
                }
                else if (att->defvalexpr == COUNTER_DEFVAL && do_copy_atr)
                {
                    td->unique_value_cache.register_used_value(m_cdp, get_typed_unique_value(dta, att->attrtype));
                }
            }
            if (att->attrpage)
                unfix_page(m_cdp, fcbna);
        }
    }
    if (changed)
    {
        write_gcr(m_cdp, dt + zcr_offset);   // update ZCR
        if (luo_offset)
            memcpy(dt + luo_offset, m_cdp->luo, UUID_SIZE);
    }
    if (detect_offset && HeIsFather)  // from the father to a son
        if (do_copy)  // unless conflict disabled the replication
        {
            memcpy(dt + detect_offset, dt + zcr_offset, ZCR_SIZE);  // store own version number
            memcpy(dt + detect_offset + ZCR_SIZE, detect_val, ZCR_SIZE);  // store father's version
            // must be after updating own ZCR
        }
    if (m_cdp->get_return_code() & 0x80)
        m_Err = m_cdp->get_return_code();

ex:

    m_cdp->ker_flags &= ~KFL_STOP_INDEX_UPDATE;
    sic.inditem_add(m_RecNum);   /* add into ALL indicies */
    unfix_page(m_cdp, fcbn);
    if (!m_Err && (m_cdp->get_return_code() & 0x80))
        m_Err = m_cdp->get_return_code();
    if (m_Err == KEY_DUPLICITY)
        dupl_infodet(added);
    return(m_Err == NOERROR);
}

//
// Zapise atribut promenne delky z paketu do databaze
//
BOOL t_in_packet :: hp_rewrite(tpiece *pc, tattrib attr, uns16 index, BOOL *changed, BOOL do_copy)
{
    uns32 size, csize, *psize;
    psize = (uns32 *)(pc + 1);
    // load data from the file:
    tptr data;
    if (!get_file_hdata(&data, &size))
        return(FALSE);
    if (!do_copy)
        goto exit;
    // compare, if necessary:
    BOOL diff;
    if (!*changed)
    {
        diff = FALSE;
        if (!hp_difference(pc, data, size, &diff))
        {
            //DbgLogWrite("hp_difference");
		    goto exit;
        }
        *changed = diff;
    }
    else
        diff = TRUE;
    // store if may be different:
    if (diff)
    { 
        int  hpoff = 0;
        csize = size;
        char *dt = data;
        while (csize)
        { 
            unsigned loc = csize > 0x8000 ? 0x8000 : csize;
            if (index == NOINDEX)
            {
                if (tb_write_var(m_cdp, td, m_RecNum, attr, hpoff, (uns16)loc, dt))
                {
                    m_Err = m_cdp->get_return_code();
                    if (!ErrorCausesNack)
                        m_Err |= ERROR_RECOVERABLE;
                    goto exit;
                }
            }
            else
            { 
                if (tb_write_ind_var(m_cdp, td, m_RecNum, attr, index, hpoff, (uns16)loc, dt))
                {
                    m_Err = m_cdp->get_return_code();
                    if (!ErrorCausesNack)
                        m_Err |= ERROR_RECOVERABLE;
                    goto exit;
                }
            }
            dt    += loc;
            hpoff += loc;
            csize -= loc;
        }
        if (index == NOINDEX)
        {
            if (tb_write_atr_len(m_cdp, td, m_RecNum, attr, size))
            {
                m_Err = m_cdp->get_return_code();
                if (!ErrorCausesNack)
                    m_Err |= ERROR_RECOVERABLE;
                goto exit;
            }
        }
        else
        {
            if (tb_write_ind_len(m_cdp, td, m_RecNum, attr, index, size))
            {
                m_Err = m_cdp->get_return_code();
                if (!ErrorCausesNack)
                    m_Err |= ERROR_RECOVERABLE;
                goto exit;
            }
        }
    }

exit:

    if (data)
        free(data);
    return(m_Err == NOERROR);
}
//
// Najde v tabulce zaznam s nimz je vkladany zaznam kvuli duplicite klicu v konfliktu
// a vytvori tabulku unikatnich klicu
//
void t_in_packet :: dupl_infodet(BOOL newrec)
{
    // Vyhledat zaznam, s nimz je prichazejici zaznam v konfliktu
    const char *p;
    for (p = m_cdp->errmsg; *p; p++)
    {
        if (*p == '(')
        {
            p++;
            str2uns(&p, &m_RecCnf);
        }
    }
    if (m_RecCnf == NORECNUM)
        return;
    // Vyhledat unikatni klice
	dd_index *cc;
	uns8      Uniqs[32];
    uns8     *pu = Uniqs;
	for (cc = td->indxs; cc < td->indxs + td->indx_cnt; cc++)
	{
		if (cc->ccateg == INDEX_PRIMARY || cc->ccateg == INDEX_UNIQUE)
        {
            for (int p = 0; p < cc->partnum; p++)
            {
                t_expr_attr *ea = (t_expr_attr *)cc->parts[p].expr;
                // Krome _W5_UNIKEY
                if (ea->elemnum != keyattr)
                {
                    uns8 *pn;
                    for (pn = Uniqs; pn < pu && *pn != ea->elemnum; pn++);
                    if (*pn == ea->elemnum)
                        continue;
			        *pu++ = ea->elemnum;
                }
            }
        }
	}
    *pu = 0;

    attribdef *att;
    tfcbnum    fcbn = NOFCBNUM;
    tfcbnum    fcbc = NOFCBNUM;
    char       UNIKEYnew[16];
    DWORD      size;
    // Smaze se v destruktoru t_in_packet
    m_DuplInfo = new t_kdupl_info(newrec);
    if (!m_DuplInfo)
        return;
    const char *dta;
    const char *dt  = fix_attr_read(m_cdp, td, m_RecNum, DEL_ATTR_NUM, &fcbn);
    if (!dt)
        goto error;
    m_DuplInfo->AddRec();
    // Zapsat nove hodnoty unikatnich klicu
    for (pu = Uniqs; *pu; pu++)
    {
		att   = &td->attrs[*pu];
        dta = dt + att->attroffset;
        m_DuplInfo->AddNew(att, dta);
    }
    // IF existuje _W5_UNIKEY AND novy zaznam precist hodnotu
    if (keyattr && newrec)
    {
		att   = &td->attrs[keyattr];
        memcpy(UNIKEYnew, dt + att->attroffset, sizeof(UNIKEYnew));
    }
    unfix_page(m_cdp, fcbn);
    fcbn = NOFCBNUM;

    // Obnovit puvodni hodnoty
    transact(m_cdp);

    const char *dtac;
    if (!newrec)
    {
        dt  = fix_attr_read(m_cdp, td, m_RecNum, DEL_ATTR_NUM, &fcbn);
        if (!dt)
            goto error;
    }
    const char *dtc;
    dtc = fix_attr_read(m_cdp, td, m_RecCnf, DEL_ATTR_NUM, &fcbc);
    if (!dtc)
        goto error;
    m_DuplInfo->Add(ATT_INT32, (char *)&m_RecNum, (char *)&m_RecCnf, 4);
    for (pu = Uniqs; *pu; pu++)
	{
		att  = &td->attrs[*pu];
        size = typesize(att);

        if (!att->attrpage) // take from the fixed page
        {
            if (!newrec)
                dta  = dt  + att->attroffset;
            dtac = dtc + att->attroffset;
        }
        m_DuplInfo->Add(att->attrtype, dta, dtac, size);
    }
    // IF existuje _W5_UNIKEY, precist hodnoty 
    if (keyattr && newrec)
    {
		att  = &td->attrs[keyattr];
        dtac = dtc + att->attroffset;
        // IF _W5_UNIKEY v novem zaznamu != _W5_UNIKEY v konfliktnim zaznamu
        if (memcmp(UNIKEYnew, dtac, sizeof(UNIKEYnew)))
        {
            m_DuplInfo->AddNew(att, (LPCSTR)UNIKEYnew);
            m_DuplInfo->Add(att->attrtype, NULL, dtac, sizeof(UNIKEYnew));
        }
    }
    unfix_page(m_cdp, fcbn);
    unfix_page(m_cdp, fcbc);
    return;

error:

    if (fcbn != NOFCBNUM)
        unfix_page(m_cdp, fcbn);
    if (fcbc != NOFCBNUM)
        unfix_page(m_cdp, fcbc);
    delete m_DuplInfo;
    m_DuplInfo = NULL;
}

void t_in_packet :: konfl_info()          
{ uns16 saved_flags = m_cdp->ker_flags;
  m_cdp->ker_flags &= ~KFL_STOP_INDEX_UPDATE;
  monitor_msg(m_cdp, m_Warn == WRN_REPLCNF_LOCAL ? RE21 : RE22);
  if (is_trace_enabled_global(TRACE_REPL_CONFLICT))
    {
        char buf[80];
        form_message(buf, sizeof(buf), RE62, m_Appl, td->selfname, m_RecNum);
        trace_msg_general(m_cdp, TRACE_REPL_CONFLICT, buf, NOOBJECT);
        konfl_infodet();
    }
  m_cdp->ker_flags = saved_flags;
}
//
// Pro zaznam, v nemz doslo ke konfliktu, zobrazi unikatni klice a adributy, ve kterych je zmena
//
void t_in_packet :: konfl_infodet()
{
	// Vyhledat unikatni klice
	dd_index *cc;
	uns8      Uniqs[32];
    uns8     *pu = Uniqs;
	for (cc = td->indxs; cc < td->indxs + td->indx_cnt; cc++)
	{
		if (cc->ccateg == INDEX_PRIMARY || cc->ccateg == INDEX_UNIQUE)
        {
            for (int p = 0; p < cc->partnum; p++)
            {
                t_expr_attr *ea = (t_expr_attr *)cc->parts[p].expr;
                if (ea->elemnum != keyattr)
                {
                    uns8 *pn;
                    for (pn = Uniqs; pn < pu && *pn != ea->elemnum; pn++);
                    if (*pn == ea->elemnum)
                        continue;
			        *pu++ = ea->elemnum;
                }
            }
        }
	}
	*pu = 0;

	int   a;
    char  valbuf[256];
    const char *dta;
    BOOL  diff;
    BOOL  DiffFound = FALSE;
    tptr  data = NULL;
    uns32 size;
    tfcbnum fcbn;
    tfcbnum fcbna = NOFCBNUM;
    t_konfl_info ki;
    const char *dt  = fix_attr_read(m_cdp, td, m_RecNum, DEL_ATTR_NUM, &fcbn);
    if (!dt)
        return;
	DWORD FilePos = SetFilePointer(m_fhnd, 0, NULL, FILE_CURRENT);
	for (a = 1; a < td->attrcnt; a++)
	{
		BOOL ReplAttr  = attrmap.has(a);
    BOOL UniqKey   = NULL!=strchr((const char *)Uniqs, a);
		attribdef *att = &td->attrs[a];
        // IF replikovany atribut
		if (ReplAttr)
		{
			if (att->attrmult > 1)
			{ 
				uns16 count;
				if (!get_file_data(&count, CNTRSZ))
					goto exit;
				uns16 index;
				for (index = 0, diff = FALSE; index < count;  index++)
				{ 
					const char *dta = fix_attr_ind_read(m_cdp, td, m_RecNum, a, index, &fcbna);
					if (dta == NULL)
						goto exit;
                    if (IS_HEAP_TYPE(att->attrtype))
                    { 
                        if (!get_file_hdata(&data, &size))
                            goto exit;
                        // IF jeste neni zaznam, testovat zmenu, jinak docist zbyle hodnoty ze souboru
                        if (!diff)
                        {
                            tpiece *pc = (tpiece *)dta;
                            if (!hp_difference(pc, data, size, &diff))
                                goto exit;
                            // IF zmena, vytvorit zaznam
                            if (diff)
                            {
                                DWORD lsize = *(DWORD *)(pc + 1);
                                if (att->attrtype == ATT_TEXT && index == 0)
                                {
                                    DWORD sz = lsize;
                                    if (sz > 32)
                                        sz = 32;
                                    if (lsize && tb_read_ind_var(m_cdp, td, m_RecNum, a, index, 0, sz, valbuf))
                                        goto exit;
                                    data[size] = 0;
                                    valbuf[sz] = 0;
                                    ki.Add(att->name, att->attrtype, valbuf, lsize, data, size);
                                }
                                else
                                    ki.Add(att->name, index); 
                                DiffFound = TRUE;
                            }
                        }
                        if (data)
                            free(data);
                        data = NULL;
                    }
                    else // fixed-len type
                    { 
                        size = typesize(att);
                        if (!get_file_data(valbuf, size))
                            goto exit;
                        if (!diff && memcmp(dta, valbuf, size))
                        {
                            ki.Add(att->name, att->attrtype, dta, valbuf, size, FALSE);
                            diff      = TRUE;
                            DiffFound = TRUE;
                        }
                    }
                    unfix_page(m_cdp, fcbna);
                    fcbna = NOFCBNUM;
                }
            }
            else  // not a multiattribute
            { 
                if (!att->attrpage) // take from the fixed page
                    dta = dt + att->attroffset;
                else
                { 
                    dta = fix_attr_read(m_cdp, td, m_RecNum, a, &fcbna);
                    if (!dta)
                        goto exit;
                }
                if (IS_HEAP_TYPE(att->attrtype))
                {
                    if (!get_file_hdata(&data, &size))
                        goto exit;
                    const tpiece *pc;
                    pc = (const tpiece *)dta;
                    if (!hp_difference(pc, data, size, &diff))
                        goto exit;
                    if (diff)
                    {
                        DWORD lsize = *(DWORD *)(pc + 1);
                        if (att->attrtype == ATT_TEXT)
                        {
                            DWORD sz = lsize;
                            if (sz > 32)
                                sz = 32;
                            if (lsize && tb_read_var(m_cdp, td, m_RecNum, a, 0, sz, valbuf))
                                goto exit;
                            if (data)
                                data[size] = 0;
                            valbuf[sz] = 0;
                        }
                        ki.Add(att->name, att->attrtype, valbuf, lsize, data, size);
                    }
                    if (data)
                        free(data);
                    data = NULL;
                }
                else
                { 
                    size = typesize(att);
                    if (!get_file_data(valbuf, size))
                        goto exit;
                    if (UniqKey || memcmp(dta, valbuf, size))
                        ki.Add(att->name, att->attrtype, dta, valbuf, size, UniqKey);
                }
            }
            if (att->attrpage)
            { 
                unfix_page(m_cdp, fcbna);  
                fcbna = NOFCBNUM;
            }
        }
        // ELSE IF unikatni klic, vytvorit zaznam
        else if (UniqKey)
        {
            dta     = dt + att->attroffset;
            size    = typesize(att);
            *valbuf = 0;
            ki.Add(att->name, att->attrtype, dta, valbuf, size, TRUE);
        }
    }
    // IF nenalezen rozdil, zobrazit _W5_DETECT
    //if (!DiffFound)
    //{
    //}
    ki.Trace(m_cdp, TRACE_REPL_CONFLICT);
    WriteReplErr(&ki, this, NORECNUM, m_RecNum, NORECNUM);

exit:

    if (data)
        free(data);
    if (fcbna != NOFCBNUM)
        unfix_page(m_cdp, fcbna);
    unfix_page(m_cdp, fcbn);
	SetFilePointer(m_fhnd, FilePos, NULL, FILE_BEGIN);
}
//
// Porovnava obsah dvou BLOBu
//
BOOL t_in_packet :: hp_difference(const tpiece *pc, const char *data, DWORD size, BOOL *diff)
{ 
    BOOL differ = FALSE;
    if (size != *(uns32 *)(pc + 1))
        differ = TRUE;
    else if (size && pc->len32 && pc->dadr) // make data comparison:
    {
        t_dworm_access dwa(m_cdp, pc, 1, FALSE, NULL);
        if (!dwa.is_open())
            return FALSE;
        unsigned pos = 0;  
        while (size && !differ)
        {
            tfcbnum fcbn;
            unsigned local;
            const char *dt = dwa.pointer_rd(m_cdp, pos, &fcbn, &local);
	        if (!dt)
                return(FALSE);
	        if (local > size)
                local=size;
            differ = memcmp(dt, data, local);
            if (fcbn != NOFCBNUM)
                unfix_page(m_cdp, fcbn);
            pos  += local;
            data += local;
            size -= local;
        }
    }
    *diff = differ;
    return(TRUE);
}
//
// Cte data z paketu
// 
BOOL t_in_packet :: get_file_data(void * dest, DWORD size)
// Reads data from fhnd, return FALSE if cannot read (enough).
{ 
    DWORD rd;
    if (!ReadFile(m_fhnd, dest, size, &rd, NULL) || rd < size)
    {
        m_Err = OS_FILE_ERROR;
        return(FALSE);
    }
    return(TRUE);
}

void t_in_packet :: skip_file_data(DWORD size)
{ SetFilePointer(m_fhnd, size, NULL, FILE_CURRENT); }

//
// Cte z paketu blok dat promenne velikosti (buffer alokuje pomoci malloc, volajici ho musi uvolnit pomoci free)
// 
BOOL t_in_packet :: get_file_hdata(tptr *pdata, uns32 *size)
{
    if (!get_file_data(size, HPCNTRSZ))
        return(FALSE);
    if (!*size)
    {
        *pdata = NULL;
        return(TRUE);
    }
    *pdata = (tptr)malloc(*size + 1);
    if (!*pdata)
    {
        m_Err = OUT_OF_KERNEL_MEMORY | ERROR_RECOVERABLE;
        return FALSE;
    }
    if (get_file_data(*pdata, *size))
        return TRUE;
    free(*pdata);
    *pdata = NULL;
    return(FALSE);
}


//
// Cte z paketu blok dat promenne velikosti, alouje buffer
// 
BOOL t_in_packet :: get_file_longdata(void *& data, uns32 & size)
{
    if (!get_file_data(&size, HPCNTRSZ))
        return(FALSE);
    if (!size)
    {
        data = NULL;
        return(TRUE);
    }
    data = corealloc(size + 1, 71);
    if (!data)
    {
        m_Err = OUT_OF_KERNEL_MEMORY | ERROR_RECOVERABLE;
        return FALSE;
    }
    if (get_file_data(data, size))
        return TRUE;
    corefree(data);
    data = NULL;
    return(FALSE);
}
//
// Zapis do tabulek _REPLERR...
//
#define RERH_DT         1
#define RERH_SERVERID   2
#define RERH_SERVERNAME 3
#define RERH_TYPE       4
#define RERH_PACKETNO   5

#define RERR_DT         1
#define RERR_APPLID     2
#define RERR_APPLNAME   3
#define RERR_TABLENO    4
#define RERR_TABLENAME  5
#define RERR_W5_UNIKEY  6
#define RERR_RECREM     7
#define RERR_RECLOC     8
#define RERR_RECCNF     9
#define RERR_DETID      10
#define RERR_ERRNO      11
#define RERR_ERRTEXT    12
#define RERR_UNIKEY     13

#define RERA_DETID      1
#define RERA_ATTRNO     2
#define RERA_ATTRNAME   3
#define RERA_REMVAL     4
#define RERA_LOCVAL     5
#define RERA_CNFVAL     6

void t_in_packet :: WriteReplErr(CTrcTbl *ki, t_nackx *nack, trecnum rInc, trecnum rLoc, trecnum rCnf)
{
    if (!LogTables)
        return;
    if (!WriteReplErrHdr())
        return;
    table_descr_auto tdr(m_cdp, ReplErrRec);
    if (!tdr->me())
        return;
    trecnum rr = tb_new(m_cdp, tdr->me(), FALSE, NULL);
    if (rr == NORECNUM)
        return;
    tb_write_atr(m_cdp, tdr->me(), rr, RERH_DT, &m_DT); 
    tb_write_atr(m_cdp, tdr->me(), rr, RERR_APPLID, m_ApplID); 
    tb_write_atr(m_cdp, tdr->me(), rr, RERR_APPLNAME, nack->m_Appl); 
    //tobjnum TableNO = find2_object(m_cdp, CATEG_TABLE, ApplID, TableName);
    tb_write_atr(m_cdp, tdr->me(), rr, RERR_TABLENO, &m_TableObj); 
    tb_write_atr(m_cdp, tdr->me(), rr, RERR_TABLENAME, nack->m_Table); 
    if (nack->m_KeyType == NAX_UNIKEY)
        tb_write_atr(m_cdp, tdr->me(), rr, RERR_W5_UNIKEY, nack->m_KeyVal); 
    if (nack->m_KeyType == NAX_PRIMARY)
        tb_write_var(m_cdp, tdr->me(), rr, RERR_UNIKEY, 0, nack->m_KeySize, nack->m_KeyVal); 
    tb_write_atr(m_cdp, tdr->me(), rr, RERR_RECREM, &rInc); 
    tb_write_atr(m_cdp, tdr->me(), rr, RERR_RECLOC, &rLoc); 
    tb_write_atr(m_cdp, tdr->me(), rr, RERR_RECCNF, &rCnf);
    tptr p = nack->m_Err ? (char *)&nack->m_Err : (char *)&m_Warn;
    tb_write_atr(m_cdp, tdr->me(), rr, RERR_ERRNO, p); 
    p = m_ErrMsg;
    if (*(DWORD *)m_ErrMsg ==  ' ' + (' ' << 8) + (' ' << 16) + (' ' << 24))
        p += 4;
    tb_write_var(m_cdp, tdr->me(), rr, RERR_ERRTEXT, 0, lstrlen(p) + 1, p); 

    if (!ki)
        return;

    table_descr_auto tda(m_cdp, ReplErrAttr);
    if (!tda->me())
        return;
    table_descr_auto td(m_cdp, m_TableObj);
    if (!td->me())
        return;
    DWORD DetID;
    tb_read_atr(m_cdp, tdr->me(), rr, RERR_DETID, (tptr)&DetID);
    
    CTrcTblVal *hVal = ki->GetRow(0)->GetHdr()->GetNext(); 
    CTrcTblVal *rVal = ki->GetRow(1)->GetHdr()->GetNext(); 
    CTrcTblVal *lVal = NULL;
    CTrcTblVal *cVal = NULL;
    // Replikacni konflikt
    if (m_Err != KEY_DUPLICITY)
        lVal = ki->GetRow(2)->GetHdr()->GetNext();
    // Duplicita klicu
    if (m_Err == KEY_DUPLICITY)
    {
        hVal = hVal->GetNext();     // Preskocit cislo zaznamu v prvnim sloupci
        rVal = rVal->GetNext();
        int rc = ki->GetRowCnt() - 1;
        if (rc > 2)
            lVal = ki->GetRow(2)->GetHdr()->GetNext()->GetNext();
        cVal = ki->GetRow(rc)->GetHdr()->GetNext()->GetNext();
    }

    while (hVal)
    {
        trecnum ra = tb_new(m_cdp, tda->me(), FALSE, NULL);
        if (ra == NORECNUM)
            break;
        int Attr;
        for (Attr = 1; Attr < td->attrcnt; Attr++)
        { 
            if (lstrcmp(td->attrs[Attr].name, hVal->GetVal()) == 0)
                break;
        }
        if (Attr >= td->attrcnt)
            Attr = 0;
        tb_write_atr(m_cdp, tda->me(), ra, RERA_DETID,    &DetID);
        tb_write_atr(m_cdp, tda->me(), ra, RERA_ATTRNO,   &Attr);
        tb_write_atr(m_cdp, tda->me(), ra, RERA_ATTRNAME, hVal->GetVal());
        hVal = hVal->GetNext();
        tb_write_var(m_cdp, tda->me(), ra, RERA_REMVAL, 0, lstrlen(p) + 1, rVal->GetVal());
        rVal = rVal->GetNext();
        if (lVal)
        {
            tb_write_var(m_cdp, tda->me(), ra, RERA_LOCVAL, 0, lstrlen(p) + 1, lVal->GetVal());
            lVal = lVal->GetNext();
        }
        if (cVal)
        {
            tb_write_var(m_cdp, tda->me(), ra, RERA_CNFVAL, 0, lstrlen(p) + 1, cVal->GetVal());
            cVal = cVal->GetNext();
        }      
    }
}

timestamp t_in_packet :: ReplErrDT;

BOOL t_in_packet :: WriteReplErrHdr()
{
    if (m_ErrPos != NORECNUM)
        return(TRUE);
    if (!GetReplErrTbls())
        return(FALSE);
    // Zahlavi
    table_descr_auto tdh(m_cdp, ReplErrHdr);
    if (!tdh->me())
        return(FALSE);
    m_ErrPos = tb_new(m_cdp, tdh->me(), FALSE, NULL);
    if (m_ErrPos == NORECNUM)
        return(FALSE);
    m_DT = stamp_now();
    if (m_DT == ReplErrDT)
        m_DT++;
    ReplErrDT = m_DT;
    tb_write_atr(m_cdp, tdh->me(), m_ErrPos, RERH_DT, &m_DT); 
    tb_write_atr(m_cdp, tdh->me(), m_ErrPos, RERH_SERVERID, m_source_server);
    tb_write_atr(m_cdp, tdh->me(), m_ErrPos, RERH_SERVERNAME, m_SrvrName);
    bool Type = (m_packet_type != PKTTP_REPL_NOACK || m_Err != 0);
    tb_write_atr(m_cdp, tdh->me(), m_ErrPos, RERH_TYPE, &Type);
    tb_write_atr(m_cdp, tdh->me(), m_ErrPos, RERH_PACKETNO, &m_PacketNo);
    return(TRUE);
}

static char CreHdr[] = 
"CREATE TABLE _SYSEXT._REPLERRHDR "
"("
    "DT         TIMESTAMP, "
    "ServerID   BINARY(12), "
    "ServerName CHAR(31), "
    "Type       BOOLEAN, "
    "PacketNO   INTEGER, "
    "CONSTRAINT I1 INDEX (DT), "
    "CONSTRAINT I2 INDEX (ServerID), "
    "CONSTRAINT I3 INDEX (ServerName), "
    "CONSTRAINT I4 INDEX (Type) "
")";

static char CreRec[] = 
"CREATE TABLE _SYSEXT._REPLERRREC "
"("
    "DT         TIMESTAMP, "
    "ApplID     BINARY(12), "
    "ApplName   CHAR(31), "
    "TableNO    SMALLINT, "
    "TableName  CHAR(31), "
    "W5_UNIKEY  BINARY(16), "
    "RecRem     INTEGER, "
    "RecLoc     INTEGER, "
    "RecCnf     INTEGER, "
    "DetID      INTEGER DEFAULT UNIQUE, "
    "ErrNO      SMALLINT, "
    "ErrText    CLOB, "
    "UNIKEY     BLOB, "
    "CONSTRAINT I1 INDEX (DT) "
")";

static char CreAttr[] = 
"CREATE TABLE _SYSEXT._REPLERRATTR "
"("
    "DetID      INTEGER, "
    "AttrNO     TINYINT, "
    "AttrName   CHAR(31), "
    "RemVal     CLOB, "
    "LocVal     CLOB, "
    "CnfVal     CLOB, "
    "CONSTRAINT I1 INDEX (DetID) "
")";

tobjnum t_in_packet :: ReplErrHdr  = NOOBJECT; 
tobjnum t_in_packet :: ReplErrRec  = NOOBJECT;
tobjnum t_in_packet :: ReplErrAttr = NOOBJECT;

BOOL t_in_packet :: GetReplErrTbls()
{
    if (ReplErrHdr != NOOBJECT && ReplErrRec != NOOBJECT && ReplErrAttr != NOOBJECT)
        return(TRUE);

    WBUUID sxUUID;
    if (ker_apl_name2id(m_cdp, SYSEXT, sxUUID, NULL))
        return(FALSE);

    ReplErrHdr = find2_object(m_cdp, CATEG_TABLE, sxUUID, "_REPLERRHDR");
    if (ReplErrHdr == NOOBJECT)
    {
        if (sql_exec_direct(m_cdp, CreHdr))
            return(FALSE);
        commit(m_cdp);
        ReplErrHdr = find2_object(m_cdp, CATEG_TABLE, sxUUID, "_REPLERRHDR");
        if (ReplErrHdr == NOOBJECT)
            return(FALSE);
    }
    ReplErrRec = find2_object(m_cdp, CATEG_TABLE, sxUUID, "_REPLERRREC");
    if (ReplErrRec == NOOBJECT)
    {
        if (sql_exec_direct(m_cdp, CreRec))
            return(FALSE);
        commit(m_cdp);
        ReplErrRec = find2_object(m_cdp, CATEG_TABLE, sxUUID, "_REPLERRREC");
        if (ReplErrRec == NOOBJECT)
            return(FALSE);
    }
    ReplErrAttr = find2_object(m_cdp, CATEG_TABLE, sxUUID, "_REPLERRATTR");
    if (ReplErrAttr == NOOBJECT)
    {
        if (sql_exec_direct(m_cdp, CreAttr))
            return(FALSE);
        commit(m_cdp);
        ReplErrAttr = find2_object(m_cdp, CATEG_TABLE, sxUUID, "_REPLERRATTR");
        if (ReplErrAttr == NOOBJECT)
            return(FALSE);
    }
    return(TRUE);
}
//
// Podle replikacniho vztahu vytvori replikacni pravidla
//
unsigned t_replctx :: create_relation(WBUUID other_server, tptr relname, BOOL second_side)
{ 
    int i;
    tptr def = NULL;
    trecnum rec = get_appl_replrec(my_server_id, other_server, NULL);
    // load & compile the relation:
//#ifdef NEWREL
    // IF na akceptujici strane AND zmena vztahu, nacist z REPL_ATR_REPLCOND, jinak normalne puvodni vztah z OBJTAB 
    if (rec != NORECNUM && second_side)
    {
        int sz;
        DbgLogWrite("Server ctu z %d", rec);
        if (!tb_read_var(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, sizeof(t_replapl), sizeof(sz), (char *)&sz) && m_cdp->tb_read_var_result > 0)
        {
            def = (char *)corealloc(sz, 0xAB);
            if (tb_read_var(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, sizeof(t_replapl) + sizeof(sz), sz, def))
                safefree((void**)&def);
        }
    }
    if (!def)
    {
//#endif
        tobjnum relobj = find2_object(m_cdp, CATEG_REPLREL, m_ApplID, relname);
        if (relobj == NOOBJECT)
            return(OBJECT_NOT_FOUND);
        def = ker_load_objdef(m_cdp, objtab_descr, relobj);
//#ifdef NEWREL
    }
//#endif
    t_replrel rel(m_cdp, NULL);
    compil_info xCI(m_cdp, def, replrel_comp);
	xCI.univ_ptr = &rel;
    DWORD Err = compile_masked(&xCI);
    corefree(def);
	if (Err)
        return(Err);

    // conflict resolution:
    char aux_attrmap[256/8];
    switch (rel.konflres)
    { 
    case 0: aux_attrmap[3] = 0;            break;
    case 1: aux_attrmap[3] = 1;            break;
    case 2: aux_attrmap[3] = second_side;  break;
    case 3: aux_attrmap[3] = !second_side; break;
    }

    // create application-level replication description:
    t_relatio *rr = &rel.r[second_side ? 1 : 0];
    if (rec != NORECNUM)
    {
        aux_attrmap[1] = rr->hours;
        aux_attrmap[2] = rr->minutes;
        tb_write_atr(m_cdp, rpltab_descr, rec, REPL_ATR_ATTRMAP, aux_attrmap);
    }

    // per-table rules for outgoing replication:
    t_repl_attrmap attrmap;
    for (i = 0;  i < rr->tabs.count();  i++)
    {
        t_reltab *rt = rr->tabs.acc(i);
        tobjnum tabobj = find2_object(m_cdp, CATEG_TABLE, m_ApplID, rt->tabname);
        if (tabobj != NOOBJECT)
        { 
            table_descr_auto td(m_cdp, tabobj);
            if (td->me()!=NULL)
            {
                if (td->tabdef_flags & TFL_ZCR)
                {
                    attrmap.clear();
                    if (rt->can_del)
                        attrmap.set(0);
                    for (int j = 0;  j < rt->atrlist.count();  j++)
                    {
                        int a = find_attribute(td->me(), *rt->atrlist.acc(j));
                        if (a != -1)
                            attrmap.set(a);
                    }
                    rec = get_appl_replrec(my_server_id, other_server, rt->tabname);
                    if (rec != NORECNUM)
                    {
                        tb_write_atr(m_cdp, rpltab_descr, rec, REPL_ATR_ATTRMAP, attrmap.ptr());
                        tb_write_var(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, 0, lstrlen(rt->condition)+1, rt->condition);
                    }
                } // table replicable
            } // table installed 
        } // table exists
    } // cycle on tables

    // per-table rules for incoming replication:
    rr = &rel.r[second_side ? 0 : 1];
    for (i = 0;  i < rr->tabs.count();  i++)
    {
        t_reltab * rt = rr->tabs.acc(i);
        tobjnum tabobj = find2_object(m_cdp, CATEG_TABLE, m_ApplID, rt->tabname);
        if (tabobj != NOOBJECT)
        {
            table_descr_auto td(m_cdp, tabobj);
            if (td->me() != NULL)
            {
                if (td->tabdef_flags & TFL_ZCR)
                {
                    attrmap.clear();
                    if (rt->can_del)
                        attrmap.set(0);
                    for (int j = 0;  j < rt->atrlist.count();  j++)
                    {
                        int a = find_attribute(td->me(), *rt->atrlist.acc(j));
                        if (a != -1)
                            attrmap.set(a);
                    }
                    rec = get_appl_replrec(other_server, my_server_id, rt->tabname);
                    if (rec != NORECNUM)
                    {
                        tb_write_atr(m_cdp, rpltab_descr, rec, REPL_ATR_ATTRMAP, attrmap.ptr());
                        tb_write_var(m_cdp, rpltab_descr, rec, REPL_ATR_REPLCOND, 0, lstrlen(rt->condition)+1, rt->condition);
                    }
                } // table replicable
            } // table installed 
        } // table exists
    } // cycle on tables
    transact(m_cdp);
  
    return(NOERROR);
}
//
// Looks for specified attribute and returns its number or -1 iff not found
// Name must be UPCASE.
//
int t_replctx :: find_attribute(table_descr *td, const char *name)
{ 
    attribdef *att;  int i;
    for (att = td->attrs, i = 0; ; att++, i++) 
    { 
        if (i >= td->attrcnt)
            return(-1);
        if (!lstrcmp(att->name, name)) 
        {
            if (att->attrtype >= ATT_FIRSTSPEC && att->attrtype <= ATT_LASTSPEC)
                return(-1);     // disables tracking attributes
            break;              // attribute found, return i
        }
    }
    return(i);
}
//
// Kompilace replikacniho vztahu
//
void WINAPI replrel_comp(CIp_t CI)
{ t_replrel * rel = (t_replrel *)CI->univ_ptr;
  rel->konflres=num_val(CI, 0, 3);
  for (int direc=0;  direc<2;  direc++)
  { t_relatio * rr = &rel->r[direc];
    rr->hours  =num_val(CI, 0, 1000);
    rr->minutes=num_val(CI, 0, 60);
    while (CI->cursym=='*')
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      t_reltab * rt = rr->get_reltab(CI->curr_name);
      next_and_test(CI, S_STRING, STRING_EXPECTED);
      strmaxcpy(rt->condition, CI->curr_string(), MAX_REPLCOND+1);
      next_sym(CI);
      rt->can_del=num_val(CI, 0, 2);
      while (CI->cursym==S_IDENT)
      { t_attrname * atr = rt->atrlist.next();
        if (atr==NULL) c_error(OUT_OF_MEMORY);
        strmaxcpy(*atr, CI->curr_name, ATTRNAMELEN+1);
        next_sym(CI);
      }
    }
  }
}
//
// Vraci pro zadany src_srv, dest_srv, appl_id a tabname cislo zaznamu v REPLTAB
// pokud zaznam neexistuje vytvori novy
//
trecnum t_replctx :: get_appl_replrec(WBUUID src_srv, WBUUID dest_srv, tptr tabname)
{ 
    trecnum rec = find_appl_replrec(src_srv, dest_srv, tabname);
    if (rec != NORECNUM)
        return(rec);
    rec = tb_new(m_cdp, rpltab_descr, FALSE, NULL);
    if (rec != NORECNUM)
    {
        tb_write_atr(m_cdp, rpltab_descr, rec, REPL_ATR_SOURCE,  src_srv);
        tb_write_atr(m_cdp, rpltab_descr, rec, REPL_ATR_DEST,    dest_srv);
        tb_write_atr(m_cdp, rpltab_descr, rec, REPL_ATR_APPL,    m_ApplID);
        if (tabname)
            tb_write_atr(m_cdp, rpltab_descr, rec, REPL_ATR_TAB, tabname);
    }
    return(rec);
}
//
// Najde pro zadany src_srv, dest_srv, appl_id a tabname zaznam v REPLTAB
//
trecnum t_replctx :: find_appl_replrec(WBUUID src_srv, WBUUID dest_srv, tptr tabname)
{ 
    int  len;
    char query[260];
    lstrcpy(query, "SELECT * FROM REPLTAB WHERE SOURCE_UUID=X'"); len  = 42;
    bin2hex(query + len, src_srv, UUID_SIZE);                     len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND DEST_UUID=X'");                   len += 18;
    bin2hex(query + len, dest_srv, UUID_SIZE);                    len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND APPL_UUID=X'");                   len += 18;
    bin2hex(query + len, m_ApplID, UUID_SIZE);                    len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND TABNAME");                        len += 13;
    if (tabname) 
    {
        lstrcpy(query + len, "='");                               len += 2;
        lstrcpy(query + len, tabname);                            len += lstrlen(tabname); 
        lstrcpy(query + len, "'");
    }
    else
        lstrcpy(query + len, " IS NULL");
    cur_descr *cd;
    tcursnum cursnum = open_working_cursor(m_cdp, query, &cd);
    if (cursnum == NOCURSOR)
        return(NORECNUM);
    trecnum trec = NORECNUM;
    if (cd->recnum)
        cd->curs_seek(0, &trec);
    free_cursor(m_cdp, cursnum);
    return(trec);
}
//
// Preskoceni replikace
//
unsigned t_replctx :: skip_repl_appl()
{ 
    m_SrvrObj = srvs.server_id2obj(m_cdp, m_dest_server);
    if (m_SrvrObj == NOOBJECT)
    {
        *m_cdp->errmsg = 0;
        return(OBJECT_NOT_FOUND); // server deleted
    }
    wbbool ackn;  
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_ACKN, (tptr)&ackn);
    if (ackn != wbtrue)     // last packet not acknowledged, cannot create a new one
        return(WAITING_FOR_ACKN);
    trecnum trec = find_appl_replrec(my_server_id, m_dest_server, NULL);
    if (trec == NORECNUM)
    {
        *m_cdp->errmsg = 0;
        return(OBJECT_NOT_FOUND);
    }
    t_zcr gcr_val;
    tb_read_atr(m_cdp, rpltab_descr, trec, REPL_ATR_GCRLIMIT, (tptr)gcr_val);
    int i;
    for (i = 0;  i < ZCR_SIZE;  i++)
        if (gcr_val[i] != 0xff)
            break;
    if (i == ZCR_SIZE) // all 0xff, blocked
        return(REPL_BLOCKED);
    memcpy(&gcr_val, header.gcr, ZCR_SIZE);
    tb_write_atr(m_cdp, rpltab_descr, trec, REPL_ATR_GCRLIMIT, &gcr_val);
    return(NOERROR);
}
//
// Resetuje komunikaci s protejsim serverem (Synchronizovat na ridicim panelu)
//
void t_replctx :: reset_server_conn(tobjnum srvobj)
{ 
    m_SrvrObj = srvobj;
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_UUID, (tptr)m_dest_server);
    
    last_packet_acknowledged(NULL); // erases last file, stores ackn.
    uns32 sendnum = (uns32)-1;
    tb_write_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDNUM, &sendnum); // new value in 6.0
    commit(m_cdp);

    char query[200];
    lstrcpy(query, "SELECT * FROM REPLTAB WHERE GCR_LIMIT<>X'ffffffffffff' AND TABNAME='' AND SOURCE_UUID=X'");
    int len = 88;
    bin2hex(query + len, my_server_id, UUID_SIZE);  len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND DEST_UUID=X'");     len += 18;
    bin2hex(query + len, m_dest_server, UUID_SIZE); len += 2 * UUID_SIZE;
    lstrcpy(query + len, "'");
    srvs.add_appls(m_cdp, query);
}
//
// Zrusi zamitnuty paket
//
unsigned t_replctx :: cancel_packet(tobjnum srvobj)
{
    m_SrvrObj = srvobj;

    wbbool ackn;
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_ACKN, (tptr)&ackn);
    // IF posledni paket potvrzen, neni co delat
    if (ackn != NONEBOOLEAN)
        return(INTERNAL_SIGNAL);
    uns32 sendnum;
    if (tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDNUM, (tptr)&sendnum))
        return(m_cdp->get_return_code());
    sendnum--;
    if (tb_write_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDNUM, &sendnum))
        return(m_cdp->get_return_code());
    last_packet_acknowledged(NULL);
    return(NOERROR);
}
//
// Zapise do databaze "Prislo potvrzeni posledniho paketu" a smaze paket z vystupni fronty
//
void t_replctx :: last_packet_acknowledged(WBUUID dest_srv)
// Stores the acknowledgement of the last packet sended and deletes its local copy.
{ 
    wbbool ackn = wbtrue;
    tb_write_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_ACKN, &ackn);
    // get last packet copy name:
    char last_packet_file[MAX_PATH];
    char fname[12 + 1];
    tb_read_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDFNAME, fname);
    if (*fname) 
    {
        lstrcpy(last_packet_file, output_queue);  
        lstrcat(last_packet_file, PATH_SEPARATOR_STR);
        lstrcat(last_packet_file, fname);
        DeleteFile(last_packet_file);
    }
    // clear the last packet copy name:
    tb_write_atr(m_cdp, srvtab_descr, m_SrvrObj, SRV_ATR_SENDFNAME, NULLSTRING);
    // IF potvrzeni, zapsat GCR poslenich odeslanych zmen do REPL_ATR_GCRLIMIT
    if (dest_srv)
    {
        int  len;
        char query[260];
        lstrcpy(query, "SELECT * FROM REPLTAB WHERE SOURCE_UUID=X'"); len  = 42;
        bin2hex(query + len, my_server_id, UUID_SIZE);                len += 2 * UUID_SIZE;
        lstrcpy(query + len, "' AND DEST_UUID=X'");                   len += 18;
        bin2hex(query + len, dest_srv, UUID_SIZE);                    len += 2 * UUID_SIZE;
        lstrcpy(query + len, "' AND TABNAME=''");
        cur_descr *cd;
        tcursnum cursnum = open_working_cursor(m_cdp, query, &cd);
        if (cursnum != NOCURSOR)
        {
            char zcr[ZCR_SIZE];
            trecnum rec, trec;
            for (rec = 0; rec < cd->recnum; rec++)
            {
                if (cd->curs_seek(rec, &trec))
                    continue;
                if (tb_read_var(m_cdp, rpltab_descr, trec, REPL_ATR_REPLCOND, sizeof(t_replapl), ZCR_SIZE, zcr) || m_cdp->tb_read_var_result < ZCR_SIZE)
                    continue;
                tb_write_atr(m_cdp, rpltab_descr, trec, REPL_ATR_GCRLIMIT, zcr);
            }
            free_cursor(m_cdp, cursnum);
            //table_descr *DelRecs = GetReplDelRecsTable(m_cdp);
            //if (DelRecs)
            //{
            //    cur_descr *cdd;
            //    tcursnum curs = open_working_cursor(m_cdp, "SELECT GCR_LIMIT FROM REPLTAB WHERE TABNAME='' ORDER BY GCR_LIMIT", &cdd);
            //    if (curs != NOCURSOR)
            //    {
            //        if (cdd->recnum > 0 && !cdd->curs_seek(0, &trec))
            //        {
            //            if (!tb_read_atr(m_cdp, rpltab_descr, trec, REPL_ATR_GCRLIMIT, zcr))
            //            {
            //                lstrcpy(query,       "DELETE FROM _SYSEXT._REPLDELRECS WHERE ZCR<=X'"); len  = 46;
            //                bin2hex(query + len, (const uns8 *)zcr, ZCR_SIZE);                                    len += 2 * ZCR_SIZE;
            //                lstrcpy(query + len, "'");
            //                sql_exec_direct(m_cdp, query);
            //            }
            //        }
            //        free_cursor(m_cdp, curs);
            //    }
            //}
            commit(m_cdp);
        }
    }
}
//
// Removes the replication of application appl_id to dest_srv from REPLTAB
//
void t_replctx :: remove_replic()
{ 
    int  len;
    char query[120 + 6 * UUID_SIZE + OBJ_NAME_LEN];
    lstrcpy(query, "DELETE FROM REPLTAB WHERE DEST_UUID=X'"); len  = 38;
    bin2hex(query + len, m_dest_server, UUID_SIZE);           len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND SOURCE_UUID=X'");             len += 20;
    bin2hex(query + len, m_source_server, UUID_SIZE);         len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND APPL_UUID=X'");               len += 18;
    bin2hex(query + len, m_ApplID,  UUID_SIZE);               len += 2 * UUID_SIZE;
    lstrcpy(query + len, "'");
    sql_exec_direct(m_cdp, query);

    lstrcpy(query, "DELETE FROM REPLTAB WHERE DEST_UUID=X'"); len  = 38;
    bin2hex(query + len, m_source_server, UUID_SIZE);         len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND SOURCE_UUID=X'");             len += 20;
    bin2hex(query + len, m_dest_server, UUID_SIZE);           len += 2 * UUID_SIZE;
    lstrcpy(query + len, "' AND APPL_UUID=X'");               len += 18;
    bin2hex(query + len, m_ApplID, UUID_SIZE);                len += 2 * UUID_SIZE;
    lstrcpy(query + len, "'");
    sql_exec_direct(m_cdp, query);
    transact(m_cdp);
}
//
// Kontext replikovane tabulky - inicializace
//
BOOL t_tablerepl_info :: TableInit(table_descr *init_td)
{ 
    td           = init_td;
    unikey       = FALSE;
    keyattr      = detect_offset = luo_offset = token_offset = 0;
    token_attr   = 0;
    // analyse attributes:
    for (int a = 1; a < td->attrcnt; a++)
    { 
        attribdef *att = &td->attrs[a];
        if (strncmp(att->name, "_W5_", 4))
            break;
        if (!lstrcmp(att->name, "_W5_UNIKEY"))
        { 
            keyattr         = a;
            unikey          = TRUE;
            keyoffset       = att->attroffset;
            keysize         = typesize(att);
            attrmap.set(a);
        }
        else if (!lstrcmp(att->name, "_W5_DETECT1"))
        { 
            detect_offset   = att->attroffset;
            attrmap.unset(a);
        }
        else if (!lstrcmp(att->name, "_W5_ZCR"))
        {   zcr_attr        = a;
            zcr_offset      = att->attroffset;
            attrmap.unset(a);
        }
        else if (!lstrcmp(att->name, "_W5_LUO"))
        {
            luo_offset      = att->attroffset;
            attrmap.unset(a);
        }
        else if (!lstrcmp(att->name, "_W5_TOKEN"))
        {
            token_attr      = a;
            token_offset    = att->attroffset;
            attrmap.unset(a);
        }
    }
    // find unikey index or primary key index:
    indexnum = -1;
    dd_index *cc;  int ind;
    for (ind = 0, cc = td->indxs;  ind < td->indx_cnt;  ind++, cc++)
    {
        if (unikey)
        {
            if (cc->atrmap.has(keyattr))
                indexnum = ind;  // ATTN: ZCR index has this column in the map, too!!
        }
        // unikey not found, look for primary key
        else if (cc->ccateg == INDEX_PRIMARY)
        {
            indexnum = ind;
            keysize  = cc->keysize - sizeof(trecnum);
            break;
        }
    }
    return indexnum != -1;
}
//
// Nacte vlastnosti aplikace
//
BOOL t_apl_props :: ApplPropsInit(cdp_t cdp, WBUUID appl_id, WBUUID other_server)
{
    // get global application info (DP):
    if (ker_apl_id2name(cdp, appl_id, NULL, &m_ApplObj)) 
        return(FALSE);
    apx_header ap;
    memset(&ap, 0, sizeof(ap));
    if (tb_read_var(cdp, objtab_descr, m_ApplObj, OBJ_DEF_ATR, 0, sizeof(apx_header), (tptr)&ap))
        return(FALSE);
    HeIsDP     = !memcmp(ap.dp_uuid, other_server, UUID_SIZE);
    IAmDP      = !memcmp(ap.dp_uuid, my_server_id, UUID_SIZE);
    HeIsFather = !memcmp(ap.os_uuid, other_server, UUID_SIZE);
    IAmFather  = !HeIsFather;
    //IAmFather = !memcmp(ap.os_uuid, my_server_id, UUID_SIZE); -- I can be a father even if I have a father!
    return(TRUE);
}
//
//
//
void t_apl_item :: SetTokenReq(t_pull_req *token_req)
{
    if (!token_req || !token_req->m_Req)
        return;
    token_request = TRUE;
    if (*token_req->m_SpecTab)
    { 
        if (!*special_tabname || lstrcmp(special_tabname, token_req->m_SpecTab))
        { 
            lstrcpy(special_tabname, token_req->m_SpecTab);
            if (special_cond)
                corefree(special_cond);
            special_cond = (tptr)corealloc(token_req->m_len16, 86);
            if (special_cond)
                lstrcpy(special_cond, token_req->m_SpecCond);
        }
        else // merge conditions
        { 
            tptr mcond = (tptr)corealloc(token_req->m_len16 + strlen(special_cond) + 10, 86);
            if (mcond)
            {
                *mcond = '(';
                lstrcpy(mcond + 1, token_req->m_SpecCond);  
                lstrcat(mcond, ")OR(");
                lstrcat(mcond, special_cond); 
                lstrcat(mcond, ")");
                corefree(special_cond);
                special_cond = mcond;
            }
        }
    }
}
//
// Konstruktor
//
t_server_work :: t_server_work(cdp_t cdp, WBUUID uuid, tobjnum srv_obj)
{
    proc_state        = ps_nothing;
    srvr_obj          = srv_obj;
    packet_fname[0]   = 0;
    statereq_fname[0] = 0;
    pull_req          = FALSE;
    PcktSndd          = NULL;
    last_state_request_send_time = 0;
    apl_list.init();
    memcpy(server_id, uuid, sizeof(WBUUID));
    tb_read_atr(cdp, srvtab_descr, srvr_obj, SRV_ATR_NAME, srvr_name);
    //InitializeCriticalSection(&cs);
}
//
// Vyhleda server v seznamu aktivnich serveru, pokud tam neni prida ho podle informaci ze SRVTAB
//
t_server_work *t_server_dynar :: get_server(cdp_t cdp, uns8 *uuid)
{
    t_server_work *server = GetServer(uuid);
    if (server)
        return(server);
    tobjnum srvr_obj = find_object_by_id(cdp, CATEG_SERVER, uuid);
    if (srvr_obj == NOOBJECT || srvr_obj == 0)
        return(NULL);
    server = Add(cdp, uuid, srvr_obj);
    return(server);
}
//
// Prida server do seznamu aktivnich serveru
//
t_server_work *t_server_dynar :: Add(cdp_t cdp, WBUUID uuid, tobjnum srvr_obj)
{
    t_server_work *server;
    EnterCriticalSection(&cs_replica);
    server = GetServer(uuid);
    if (!server)
    {
        server = new t_server_work(cdp, uuid, srvr_obj);
        if (server)
        {
            server->next = m_Server;
            m_Server     = server;
        }
    }
    LeaveCriticalSection(&cs_replica);
    return(server);
}
//
// Konvertuje UUID serveru na jmeno
//
void t_server_dynar :: GetServerName(cdp_t cdp, WBUUID server_id, char *Buf)
{
    t_server_work *server = get_server(cdp, server_id);
    if (server)
        lstrcpy(Buf, server->srvr_name);
    else
        *(DWORD *)Buf = '?' + ('?' << 8);
}
//
// Konvertuje jmeno serveru na UUID 
//
void t_server_dynar :: GetServerID(cdp_t cdp, char *Name, WBUUID server_id)
{
    t_server_work *server;
    for (server = m_Server; server; server = server->next)
    { 
        if (lstrcmp(server->srvr_name, Name) == 0)
            break;
    }
    if (!server)
    {
        tobjnum srvr_obj = find_object(cdp, CATEG_SERVER, Name);
        if (srvr_obj != NOOBJECT && srvr_obj != 0)
        {
            tb_read_atr(cdp, srvtab_descr, srvr_obj, SRV_ATR_UUID, (tptr)server_id);
            server = Add(cdp, server_id, srvr_obj);
        }
    }
    if (server)
        memcpy(server_id, server->server_id, UUID_SIZE);
    else
        memset(server_id, 0, UUID_SIZE);
}
//
// Konvertuje UUID serveru na cislo objektu v tabulce serveru
//
tobjnum t_server_dynar :: server_id2obj(cdp_t cdp, WBUUID server_id)
{
    t_server_work *server = get_server(cdp, server_id);
    return(server ? server->srvr_obj : NOOBJECT);
}
//
// Adds application into the apl_list, unless already contained.
// Returns TRUE if added new.
//
// Vola se z replicate_appl
//
int t_server_dynar :: add_appl(cdp_t cdp, WBUUID server_id, WBUUID appl_id, trecnum appl_rec, t_pull_req *pull_req, CPcktSndd *ps)
{    
    t_server_work *server = get_server(cdp, server_id);
    if (!server)
        return(-1);
    EnterCriticalSection(&cs_replica);
    int res = server->add_appl(cdp, appl_id, appl_rec, pull_req, ps);
    LeaveCriticalSection(&cs_replica);
    return(res);
}
//
// Prida serverum v seznamu aktivnich serveru aplikace vybrane v dotazu query
//
void t_server_dynar :: add_appls(cdp_t cdp, char *query)
{
    cur_descr *cd;
    tcursnum cursnum = open_working_cursor(cdp, query, &cd);
    if (cursnum == NOCURSOR)
        return;
    int new_prepared = 0;
    WBUUID server_id, /*server_cur, */ appl_id;
    t_server_work *server;
    trecnum        rec, trec;
    EnterCriticalSection(&cs_replica);
    for (rec = 0, server = NULL; rec < cd->recnum; rec++)
    {
        if (cd->curs_seek(rec, &trec))
            continue;
        if (tb_read_atr(cdp, rpltab_descr, trec, REPL_ATR_DEST, (tptr)server_id))
            continue;
        //if (memcmp(server_id, server_cur, UUID_SIZE))
        //{
        //    if (server)
        //    {
        //        LeaveCriticalSection(&server->cs);
        //    }
        //    server = get_server(cdp, server_id);
        //    if (!server)
        //        continue;
        //    memcpy(server_cur, server_id, UUID_SIZE);
        //    EnterCriticalSection(&server->cs);
        //}
        server = get_server(cdp, server_id);
        if (!server)
            continue;
        if (tb_read_atr(cdp, rpltab_descr, trec, REPL_ATR_APPL, (tptr)appl_id))
            continue;
        int nw = server->add_appl(cdp, appl_id, trec, NULL, NULL);
        if (nw > 0)
            new_prepared++;
    }
    LeaveCriticalSection(&cs_replica);
    //if (server)
    //{
    //    LeaveCriticalSection(&server->cs);
    //}
    free_cursor(cdp, cursnum);
    if (new_prepared)
    {
        the_replication->ReleaseWorkThread(new_prepared);
    }
}
//
// Zamceni kvuli zpracovani work threadem
//
//BOOL t_server_work :: Lock()
//{
//    EnterCriticalSection(&cs);
//    if (proc_state == ps_prepared)
//        return(TRUE);
//    LeaveCriticalSection(&cs);
//    return(FALSE);
//}
//
// Prida aplikaci do seznamu aplikaci, ktere se maji replikavat
// Musi probihat uvnitr kriticke sekce cs_replica
// Vraci: 1 = OK, seznam byl prazdny, spustit work_thred
//        0 = OK, seznam nebyl prazdny
//       -1 = chyba
// 
// Vola se z replicate_appl prostrednictvim t_server_dynar :: add_appl 
// nebo ze scan_replication_plans prostrednictvim add_appls
//
int t_server_work :: add_appl(cdp_t cdp, WBUUID appl_id, trecnum appl_rec, t_pull_req *pull_rq, CPcktSndd *ps)
{   int i; 
    if (proc_state == ps_working)
        return(0);
    t_apl_item *apl_item = NULL;
    for (i = 0; i < apl_list.count(); i++)
    {
        t_apl_item *item = apl_list.acc(i);
        if (!memcmp(appl_id, item->appl_id, UUID_SIZE))
        {
            apl_item = item;
            break;  // found
        }
    }
    if (!apl_item)
    {
        apl_item = apl_list.next();
        if (!apl_item)
            return(-1);
        apl_item->ApplInit(appl_id, appl_rec);
        //DbgPrint("Added %d", appl_rec);
        if (!apl_item->ApplPropsInit(cdp, appl_id, server_id))
        {
            apl_list.delet(i);
            return(-1);
        }
    }
    pull_req |= (NULL!=pull_rq);
    apl_item->SetTokenReq(pull_rq);
    if (ps)
    {
        ps->SetNext(PcktSndd);
        PcktSndd = ps;
    }
    proc_state = ps_prepared;
    return(1);
}

BOOL t_server_work :: AddPacket(char *fName, BOOL Data)
{
    BOOL Added = FALSE;
    EnterCriticalSection(&cs_replica);
    // Unless processing other or even the same file
    if (proc_state == ps_nothing)
    {
        if (Data)
        {
            if (!*packet_fname)
            {
                lstrcpy(packet_fname, fName);
                proc_state = ps_prepared;
                Added = TRUE;
            }
        }
        else
        {
            if (!*statereq_fname)
            {
                lstrcpy(statereq_fname, fName);
                proc_state = ps_prepared;
                Added = TRUE;
            }
            else 
            { 
                DeleteFile(fName); // delete duplicate state request
            }
        }
    }
    LeaveCriticalSection(&cs_replica);
    return(Added);
}
//
// Vypis hlaseni do logu
//
void monitor(cdp_t cdp, int direct, tptr server_name, t_packet_type packet_type, int PacketNo)
{ 
    char msg[100], *p, buf[20];  int len;
    // prepare the message:
    if (direct == DIR_INPCK) 
        form_message(msg, sizeof(msg), RE0);
    else if (direct == DIR_OUTPCK)
        form_message(msg, sizeof(msg), RE1);
    else if (direct == DIR_MAIL)
        form_message(msg, sizeof(msg), RE38);
    else if (direct == DIR_DIRECTIP)
        form_message(msg, sizeof(msg), RE49);
    else
        form_message(msg, sizeof(msg), RE23);
    lstrcat(msg, server_name);
    len=(int)strlen(msg);
    form_message(msg+len, sizeof(msg)-len, RE2);
    switch (packet_type)
    { 
        case PKTTP_REPL_DATA:    p=RE3;  break;
        case PKTTP_REPL_REQ:     p=RE4;  break;
        case PKTTP_REPL_ACK:     p=RE5;  break;
        case PKTTP_REPL_NOACK:   p=RE65; break;
        case PKTTP_STATE_REQ:    p=RE6;  break;
        case PKTTP_STATE_INFO:   p=RE7;  break;
        case PKTTP_SERVER_REQ:   p=RE8;  break;
        case PKTTP_SERVER_INFO:  p=RE9;  break;
        case PKTTP_REL_REQ:      p=RE10; break;
        case PKTTP_REL_ACK:      p=RE11; break;
        case PKTTP_REL_NOACK:    p=RE12; break;
        case PKTTP_REPL_STOP:    p=RE13; break;
        case PKTTP_REPL_UNBLOCK: p=RE14; break;
        case PKTTP_NOTH_TOREPL:  p=RE69; break;
        default:                 p=buf;  int2str((int)packet_type, buf);  break;
    }
    len=(int)strlen(msg);
    form_message(msg+len, sizeof(msg)-len, p);
    if (PacketNo)
    {
        int2str(PacketNo, buf);
        len=(int)strlen(msg);
        form_message(msg+len, sizeof(msg)-len, RE15);
        lstrcat(msg, buf);
    }
    // send it to the window:
    trace_msg_general(cdp, TRACE_REPLICATION, msg, NOOBJECT);
}
//
// Volani systemoveho triggeru _ON_REPLPACKET_SENDED
//
void CallOnSended(cdp_t cdp, WBUUID ServerID, tobjname ServerName, int PacketType, int PacketNO, int ErrNO, char *ErrText)
{
    if (!OnSended && !OnSent)
        return;
    char cmd[256];
    int  len;
    lstrcpy(cmd, OnSent ? "CALL _SYSEXT._ON_REPLPACKET_SENT(X'" : "CALL _SYSEXT._ON_REPLPACKET_SENDED(X'");  len  = OnSent ? 35 : 37;
    bin2hex(cmd + len, ServerID, UUID_SIZE);                len += 2 * UUID_SIZE;
    sprintf(cmd + len, "', \"%s\", %d, %d, %d, \"%s\")", ServerName, PacketType, PacketNO, ErrNO, ErrText ? ErrText : "");
    if (!exec_direct(cdp, cmd))
      transact(cdp);
}
//
// Ulozi / zrusi rozpracovanou transakci, vynuluje
//
int transact(cdp_t cdp)
{ 
    int Err;
    if (cdp->roll_back_error)
    {
        Err = cdp->get_return_code();
        roll_back(cdp);  /* must remove TMPlocks even if no data changes performed */
    }
    else
    {
        commit(cdp);
        Err = cdp->get_return_code();
        if (Err < BAD_MODIF)
            Err = ANS_OK;
    }
    cdp->set_return_code(ANS_OK);
    cdp->roll_back_error=wbfalse;
    return(Err);
}

static tobjnum ReplDelRecsNum = 0;
static table_descr *delrecs_descr = NULL;

tattrib RDR_W5_UNIKEY = (tattrib)-1;
tattrib RDR_UNIKEY    = (tattrib)-1;
tattrib RDR_ZCR       = 4;
tattrib RDR_DATETIME  = 5;

table_descr *GetReplDelRecsTable(cdp_t cdp)
{ 
    if (ReplDelRecsNum == 0)
    {
        WBUUID sxUUID;
        if (ker_apl_name2id(cdp, SYSEXT, sxUUID, NULL))
            ReplDelRecsNum = NOOBJECT;
        else
        {
            ReplDelRecsNum = find2_object(cdp, CATEG_TABLE, sxUUID, "_REPLDELRECS");
            if (ReplDelRecsNum != NOOBJECT)
            {
                tattrib r = 3;
                delrecs_descr  = install_table(cdp, ReplDelRecsNum);
                if (strcmp(delrecs_descr->attrs[3].name, "W5_UNIKEY") == 0)
                {
                    RDR_W5_UNIKEY = 3;
                    r             = 4;
                }
                if (strcmp(delrecs_descr->attrs[r].name, "UNIKEY") == 0)
                {
                    RDR_UNIKEY    = r;
                    r++;
                }
                if (RDR_W5_UNIKEY == (tattrib)-1 && RDR_UNIKEY == (tattrib)-1)
                {
                    monitor_msg(cdp, DelRecsBadFormat);
                    try_uninst_table(cdp, ReplDelRecsNum);
                    ReplDelRecsNum = NOOBJECT;
                    delrecs_descr  = NULL;
                }
                else
                {
                    RDR_ZCR       = r++;
                    RDR_DATETIME  = r;
                }
            }
        }
    }
    return delrecs_descr;
}

void ResetDelRecsTable()
{
    if (delrecs_descr)
        unlock_tabdescr(delrecs_descr);
    ReplDelRecsNum = 0;
    delrecs_descr  = NULL;
}
