//
//  netapi.h - header of netapi.cpp
//  ===============================
//
#if defined(WINS) && (WBVERS<1000)
#define ALTERNATIVE_PROTOCOLS 1
#else
#define ALTERNATIVE_PROTOCOLS 0
#endif

#define WB_SERVER_NAME_LEN   OBJ_NAME_LEN
#define WBSERVER_IPX_ID      0xc1e5 /* advertiser identification */

/*********************        Waiting        *******************************/
BOOL WinWait (DWORD dwTickReference, DWORD dwTickTimeout);

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
  BOOL process_broken_link;  // used by TCP/IP client, is set to TRUE between successfull Link and Unlink
  virtual BYTE Link(cdp_t cdp) = 0;
  virtual BOOL Unlink(cdp_t cdp, BOOL fTerminateConnection) = 0;
  virtual BOOL Send(t_fragmented_buffer * frbu, BYTE bDataType, unsigned MsgDataTotal, unsigned &rDataSent) = 0;
  virtual cAddress * Copy () = 0;
  virtual BOOL AddrEqual (cAddress *pAddr) = 0;
  virtual void GetStatus(void)               // debugging status info
    { }                                      // for all except TCP/IP
  virtual BOOL SendBreak(cdp_t cdp)          // client break
    { return ::Send(this, 0, DT_BREAK); }    // for all except TCP/IP
	virtual BOOL SendKeepAlive(cd_t * cdp)     // Send client response to the KeepAlive packet
    { return TRUE; }                         // for all except TCP/IP
  virtual ~cAddress(void) { }
#ifdef USE_KERBEROS
  virtual bool krb5_sendauth(const char *)
    { return false; }                        // for TCP/IP only
#endif

  BYTE abRemIdent[REM_IDENT_SIZE];  // network node identification
  BYTE * GetRemIdent(void)
    { return abRemIdent; }
#ifdef SINGLE_THREAD_CLIENT
  virtual BOOL ReceiveWork(cdp_t cdp, int timeout)        // explicit receiving activity (if not handled by another thread)
    { return TRUE; } // for all except TCP/IP
#endif
};

// TCP/IP only:
#ifdef SINGLE_THREAD_CLIENT
BOOL ReceivePrepare(cdp_t cdp);
void ReceiveUnprepare(cdp_t cdp);
#endif

class cServerAddress
{ friend class cServerList;
 public:
  cServerAddress (char *psName)
    { strcpy (acName, psName);
      pIpxAddress = pNetBiosAddress = pTcpIpAddress = 0;
      pNext = 0;
    };
  ~cServerAddress ()
    { delete pIpxAddress; delete pNetBiosAddress; delete pTcpIpAddress; };
  void PostServerAddress(HWND hTree);

 private:
  char acName[WB_SERVER_NAME_LEN + 1];
  cAddress *pIpxAddress;
  cAddress *pNetBiosAddress;
  cAddress *pTcpIpAddress;
  class cServerAddress *pNext;
};


class cServerList
{public:
  void Clear(void);
  cServerList(void)  { pRoot = 0;  };
  ~cServerList(void) { }; // Clear(); }; there is only one static object of the class, when it is destructed, memory management is already closed and allocated info items are overwritten
  cAddress * GetAddress(const char * psName);
  void Bind(char *psName, cAddress *pIpxAddrIn, cAddress *pNetBiosAddrIn, cAddress *pTcpIpAddrIn, uns32 ip, uns16 port);
  void PostServers(HWND hWnd);

 private:
  cServerAddress *pRoot;
};

extern cServerList oSrchSrvrList;

/********************             Functions             *********************/


CFNC DllExport int NetworkStart(int reqProtocol);
CFNC DllExport int NetworkStop (cdp_t cdp);

BYTE ServerStart(char *psName, HWND hWnd);
BYTE ServerStop (void);
void StopSlaves (void);

int GetServerAddress(const char * psName, cAddress **ppAddr);

BYTE Link (cdp_t cdp);
void Unlink (cdp_t cdp);

BOOL SendRequest(cAddress *pRemAddr, t_fragmented_buffer * frbu);
BOOL SendExportAcknowledge(cAddress *pRemAddr, void *pBuff, uns32 Size);
BOOL SendImportAcknowledge(cAddress *pRemAddr, void *pBuff, uns32 Size);
BOOL SendVersion(cAddress *pRemAddr, LONG lVersion);
BOOL SendCloseConnection(cAddress *pRemAddr);

BOOL Receive (cd_t * cdp, BYTE *pBuff, unsigned Size);
void StopReceiveProcess (cd_t *cdp);

/********************************* netx.asm ********************************/

CFNC BYTE GetConnectionNumber (void);
CFNC BYTE GetConnectionInformation (void *requestBuff, void *replyBuff);

/********************************* slave ***********************************/
DWORD WINAPI SlaveThreadProc(cAddress *);
void StartReceive(cd_t*);

extern int wProtocol;
#define IPXSPX                  0x0001
#define NETBIOS                 0x0002
#define TCPIP										0x0004

cAddress * make_fw_address(uns32 srv_ip, unsigned port, uns32 fw_ip);
void get_ip_address(const char * server_name, char * ip_addr, char * conn_name,
                    char * fw_addr, BOOL * via_fw, int * port, BOOL * via_tunnel);
#define SLAVE_STACK_SIZE 45000

void client_log(char * text);
void client_log_i(int err);

BOOL send_http(cAddress *pRemAddr, t_fragmented_buffer * frbu, int message_type);
#define RDSTEP                            512 // sizeof import/export buffer
void ReceiveHTTP(cdp_t cdp, char * body, int bodysize);
bool can_broadcast(void);

