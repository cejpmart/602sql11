//
//  netapi.h - header of netapi.cpp
//  ===============================
//
#if defined(WINS) && (WBVERS<1000)
#define ALTERNATIVE_PROTOCOLS 1
#else
#define ALTERNATIVE_PROTOCOLS 0
#endif

extern BYTE  abKernelNetIdent[6];

#define WBSERVER_IPX_ID      0xc1e5 /* advertiser identification */

// Advertising the SQL server in the network:
extern tobjname acServerAdvertiseName;
int AdvertiseStart(const char * server_name, bool force_net_interface);
int AdvertiseStop(void);
void WarnClientsAndWait(unsigned timeout);

void IdentityVerificationStart(const char * server_name);
void IdentityVerificationStop(void);

/*********************        Waiting        *******************************/
BOOL WinWait (DWORD dwTickReference, DWORD dwTickTimeout);

#ifndef UNIX
/*************              Network Memory Allocator             ************/

#ifdef WINS

struct sNetMem
/************/
{ WORD     wSize;
  HGLOBAL  hMem;
  BYTE     *pMem;
};

void * AllocNetMem (sNetMem *psMem, WORD wSize);
void FreeNetMem (sNetMem *psMem);

#endif // WINS

/*****************              Buffer Pool            **********************/

class cBufferPool
/***************/
{ public:
    cBufferPool ();
    ~cBufferPool ();
    void *Alloc (WORD wSize, WORD wNum);
#ifdef WINS
		void Free ();
#else
    void Free ()
      { if (pMem) corefree(pMem);  pMem = 0;  wBuffOut = wBuffNum = 0; }
#endif
    void *GetBuffer (BOOL fWait);
    void PutBuffer (void *pvBuff);
    BOOL inline InUse ()
      { return (BOOL) wBuffOut; }
  private:
    WORD wBuffOut, wBuffNum;
    BYTE *pbRoot;
#ifdef WINS
		sNetMem sMem[10];
#else
    BYTE *pMem;
#endif
		CRITICAL_SECTION critSec;
};
#endif // !UNIX

/******************                 Address             *********************/
// Protocol independent adress.
// Virtual class whose redefinitions contains protocol dependent functions
// and address values.
#define REM_IDENT_SIZE 6

class cAddress
/************/
{ public:
  WORD my_protocol;
  bool is_http;

  virtual BOOL Unlink(cdp_t cdp, BOOL fTerminateConnection) = 0;
  virtual BOOL Send(t_fragmented_buffer * frbu, BYTE bDataType, unsigned MsgDataTotal, unsigned &rDataSent) = 0;
  virtual cAddress * Copy () = 0;
  virtual BOOL AddrEqual (cAddress *pAddr) = 0;
  virtual void RegisterReceiver (cdp_t cdp) = 0;
  virtual void UnregisterReceiver () = 0;
  virtual void GetStatus(void)               // debugging status info
    { }                                      // for all except TCP/IP
  virtual BOOL ReceiveWork(cdp_t cdp, BOOL signal_it) // explicit receiving activity
    { return TRUE; }                         // for all except TCP/IP
  virtual BOOL SendBreak(cdp_t cdp)          // client break
    { return ::Send(this, 0, DT_BREAK); }    // for all except TCP/IP
  virtual void SlaveReceiverStop(cdp_t cdp)  // terminating slave after connection failure
    { }                                      // for all except TCP/IP
	virtual BOOL SendKeepAlive(cdp_t cdp)      // Send client response to the KeepAlive packet
    { return TRUE; }                         // for all except TCP/IP
  virtual void GetAddressString(char * addr) // Return the address of the other peer
    { *addr=0; }                             // redefined for TCP/IP and IPX/SPX on server side
#ifdef LINUX
  virtual int get_sock(void)
    { return -1; }	                         // redefined for TCP/IP
#endif
  virtual int ValidatePeerAddress(cdp_t cdp)   // check if the address of the client is allowed
    { return KSE_OK; }                         // redefined for TCP/IP
  virtual ~cAddress(void) { }

  BYTE abRemIdent[REM_IDENT_SIZE];  // network node identification
  BYTE * GetRemIdent(void)
    { return abRemIdent; }
  virtual void mark_core(void) { }
};

/****************        wId to Cdp translation        *********************/


struct sId2Cdp
/************/
{ WORD  wId;
  cdp_t cdp;
};


class cId2Cdp
/***********/
{ public:
  cId2Cdp () { wNum = 0; };
  void Register (WORD wId, cdp_t cdp);
  void Unregister (WORD wId);
  cdp_t Translate (WORD wId);

  private:
  sId2Cdp asTranslate [MAX_MAX_TASKS];
  WORD wNum;
};

/********************             Functions             *********************/


BYTE NetworkStart(void);
BYTE NetworkStop (cdp_t cdp);

void StopSlaves (void);

int GetServerAddress(const char * psName, cAddress **ppAddr);

void Unlink (cdp_t cdp);
void RegisterReceiver (cdp_t cdp);

BOOL SendCloseConnection(cAddress *pRemAddr);
BOOL SendSlaveInitResult (cAddress *pRemAddr, int InitRes);

BOOL Receive(cdnet_t * cdp, BYTE *pBuff, unsigned wSize);
void StopReceiveProcess (cdp_t cdp);

extern int slaveThreadCounter, detachedThreadCounter;

/********************************* netx.asm ********************************/

CFNC BYTE GetConnectionNumber (void);
CFNC BYTE GetConnectionInformation (void *requestBuff, void *replyBuff);

/********************************* slave ***********************************/
THREAD_PROC(SlaveThread);
THREAD_PROC(SlaveThreadHTTP);
void StartReceive(cdp_t cdp);

extern uns16 wProtocol; // running protocols
#define IPXSPX                  0x0001
#define NETBIOS                 0x0002
#define TCPIP										0x0004
#define TCPIP_HTTP              0x0008  // http tunnel access running
#define TCPIP_WEB               0x0010  // web access running 
#define TCPIP_NET               0x0020  // net access running

cAddress * make_fw_address(uns32 srv_ip, uns32 fw_ip);
#define SLAVE_STACK_SIZE 200000
void MessageToClients(cdp_t cdp, const char * msg);
void StopTheSlave(cdp_t cdp);
void mark_ip_filter(void);
////////////////////////////////// impersonation /////////////////////////
#ifdef WINS
extern char local_computer_name[70];
enum tImpersonationStates { DISCONNECTED_STATE, CONNECTING_STATE, READING_STATE, WRITING_STATE };
struct tImpersonation
{ HANDLE hImpersonationPipe, hImpersonationEvent;
  OVERLAPPED hImpersonationOverlap;
  tImpersonationStates dwState; 
  BOOL fPendingIO;   
  char buf[1];
  DWORD cbBytes;
  tImpersonation(void)
    { hImpersonationPipe=INVALID_HANDLE_VALUE;  dwState=DISCONNECTED_STATE; }
  BOOL ConnectToNewClient(void); 
  void open(const char * server_name);
  void close(void);
  void reset(void);
};
extern tImpersonation Impersonation;
#endif

