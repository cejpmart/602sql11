//
//  tcpip.cpp - protocol dependent calls for sockets & TCP/IP from Windows 95
//  ===================================================================
//

#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include <process.h>
#include <winsock.h>
#include "netapi.h"
#include "tcpip.h"
#include "regstr.h"
#include "cnetapi.h"
#include "enumsrv.h"

#define USE_GETHOSTBYNAME
#define IP_ADDR_MAX  100
#define SET_SOCKADDR(sin,port,addr) \
{ (sin).sin_family = AF_INET;\
  (sin).sin_port	 = htons(port);\
  (sin).sin_addr.s_addr = addr; }\

class cTcpIpAddress : public cAddress
/*************************************/
{ public:
  cTcpIpAddress (SOCKADDR_IN *callAddr);
  cTcpIpAddress (void); // J.D.
  virtual ~cTcpIpAddress();
  BOOL Send (t_fragmented_buffer * frbu, BYTE bDataType, unsigned MsgDataTotal, unsigned &rDataSent);
  BYTE Link(cdp_t cdp);
  BOOL Unlink(cdp_t cdp, BOOL fTerminateConnection);
  cAddress * Copy ()
    { return new cTcpIpAddress (&acCallName); };
  BOOL AddrEqual (cAddress *pAddr)
    { return !memcmp ( &((cTcpIpAddress *)pAddr)->acCallName.sin_addr, &acCallName.sin_addr, sizeof (acCallName.sin_addr)) &&
             ((cTcpIpAddress *)pAddr)->acCallName.sin_port==acCallName.sin_port;
		}
	BOOL SendSqp(cd_t * cdp, int);
  BOOL SendBreak(cdp_t cdp)      // Sends client break to server
	  { return SendSqp(cdp, SQP_BREAK); }
	BOOL SendKeepAlive(cd_t * cdp) // Sends client response to the KeepAlive packet
  	{ return SendSqp(cdp, SQP_KEEP_ALIVE); }

  void store_rem_ident(void);
  bool send_iter(void * buf, unsigned len);
  public:
		SOCKADDR_IN acCallName;  // ip address and port defined
		SOCKET  sock;
		BYTE *   send_buffer;
    BYTE *   tcpip_buffer;
    HANDLE   tcpip_event;
    uns32 firewall_adr;
    OVERLAPPED ovl;
    HANDLE hReceiveThread;
};

long threadCount;      // count of running special server or client threads
HANDLE hShutDown = 0;  // event "Last special thread terminated", 0 iff TCP/IP not started
HANDLE keepAlive;      // event "KeepAlive watch received client response"
static short sip_listen_port = DEFAULT_IP_SOCKET+1;

BOOL TcpIpPresent(void)
{	return TRUE; }

BYTE TcpIpProtocolStart(void)
/* Can (and must) be called by each client, even if 16-bit. */
{ threadCount = 0;
	hShutDown = CreateEvent(NULL, FALSE, FALSE, NULL);   // synchro objekt pri ukoncovani app
#ifdef MY_KEEPALIVE
  keepAlive = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
	WORD vers = /*0x0002;*/ 0x0101;   // WINSOCK 1.1
	WSADATA wsa;
	int err = WSAStartup(vers, &wsa);
  if (err) return KSE_NO_WINSOCK;
 // check winsock by creating a socket:
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) return KSE_NO_WINSOCK;
  closesocket(sock);
	return KSE_OK;
}

SOCKET sipsock=0;

BYTE TcpIpProtocolStop(cdp_t cdp)
/* Can (and must) be called by each client, even if 16-bit. */
{ if (threadCount)						// pokud bezi nejake thready
	{	ResetEvent(hShutDown);   	// tak se priprav na cekani
		if (sipsock)          // asi neni treba
	    { closesocket(sipsock);     sipsock=0; }
	}
	else
		SetEvent(hShutDown);
#ifdef MY_KEEPALIVE
  SetEvent(keepAlive);  // releases K-A thread waiting for client response or reactivation
#endif
	WaitForSingleObject(hShutDown, INFINITE);   // ceka se na ukonceni komunikacnich threadu
	CloseHandle(hShutDown);
  hShutDown=0;
	WSACleanup();  // J.D. moved after waiting for other threads, crashes on NT 4.0  temp removed ###
#ifdef MY_KEEPALIVE
  CloseHandle(keepAlive);
#endif
	return KSE_OK;
}

//
// ************************************* cTcpIpAddress ***********************************
//
void cTcpIpAddress::store_rem_ident(void)
{ memcpy(abRemIdent,   &acCallName.sin_addr,   4);
  memcpy(abRemIdent+4, &acCallName.sin_family, 2);
}

cTcpIpAddress::cTcpIpAddress(SOCKADDR_IN *callAddr)
{ my_protocol=TCPIP;
	memcpy(&acCallName, callAddr, sizeof(*callAddr));
  store_rem_ident();
	send_buffer = new BYTE[TCPIP_BUFFSIZE];  // creates the default send buffer
  tcpip_buffer=NULL;
  firewall_adr=0;
  process_broken_link=FALSE;
  hReceiveThread=0;
  is_http=false;
}

cTcpIpAddress::cTcpIpAddress(void)
{ my_protocol=TCPIP;
	send_buffer = new BYTE[TCPIP_BUFFSIZE];  // creates the default send buffer
  tcpip_buffer=NULL;
  firewall_adr=0;
  process_broken_link=FALSE;
  hReceiveThread=0;
  is_http=false;
}

cTcpIpAddress::~cTcpIpAddress()
{	if (send_buffer) delete [] send_buffer; }

cAddress * make_fw_address(uns32 srv_ip, unsigned port, uns32 fw_ip)
{ SOCKADDR_IN sin;
  SET_SOCKADDR(sin,port+1,htonl(srv_ip));
  cTcpIpAddress * adr = new cTcpIpAddress(&sin);
  adr->firewall_adr=fw_ip;
  return adr;
}

bool cTcpIpAddress::send_iter(void * buf, unsigned len)
{
	int sended = 0;
	do
	{ int bRetCode = send(sock, (char*)buf+sended, len-sended, 0);
		if (bRetCode<0) return false;
    sended = sended + bRetCode;
	} while (sended!=len);
  return true;
}

#pragma pack(1)
struct t_full_hdr
{ uns32 totlen;
  sWbHeader pWbHeader;
}; 
#pragma pack()

BOOL cTcpIpAddress::Send(t_fragmented_buffer * frbu, BYTE bDataType, unsigned MsgDataTotal, unsigned &rDataSent)
{ BOOL res=TRUE;  
  unsigned DataTotal = sizeof (sWbHeader) + sizeof(uns32) + MsgDataTotal;
  if (sock == -1) 
    res = FALSE;
  else if (DataTotal<=TCPIP_BUFFSIZE)  // use the preallocated [send_buffer]
  {// prepare the complete message:
	  *(uns32*)send_buffer = DataTotal;   // the message begins with its length
    sWbHeader *pWbHeader = (sWbHeader *)(send_buffer + sizeof(uns32));
    pWbHeader->bDataType     = bDataType;
    pWbHeader->wMsgDataTotal = MsgDataTotal;
    frbu->copy((char*)send_buffer + sizeof (sWbHeader) + sizeof(uns32), MsgDataTotal);
   // send it:
    if (!send_iter(send_buffer, DataTotal))
      res = FALSE;
  }
  else  // sending in parts if long
  { t_full_hdr hdr;
    hdr.totlen=DataTotal;
    hdr.pWbHeader.bDataType=bDataType;
    hdr.pWbHeader.wMsgDataTotal=MsgDataTotal;
    if (!send_iter((char*)&hdr, sizeof(hdr)))
      res = FALSE;
    else if (frbu->fragments) 
    { do // iterates on fragments
      { uns32 step=frbu->fragments[frbu->current_fragment].length - frbu->offset_in_current;  // - offset_in_current added 18.1.2006
        if (!send_iter(frbu->fragments[frbu->current_fragment].contents+frbu->offset_in_current, step))
          { res = FALSE;  break; }
        frbu->offset_in_current=0;
      } while (++frbu->current_fragment < frbu->count_fragments);
    }
  }
  rDataSent = MsgDataTotal;
  return res;
}

#pragma pack(1)
typedef struct socks_connect
{ uns8 version_number;
  uns8 command_code;
  uns16 dest_port;
  uns32 dest_ip;
  uns8 null_term;
  socks_connect(uns16 port, uns32 ip)
  { version_number=4;  command_code=1;  null_term=0;
    dest_port=port;  dest_ip=ip;
  }
} socks_connect;

typedef struct socks_connect_ans
{ uns8 version_number;
  uns8 result;
  uns16 dest_port;
  uns32 dest_ip;
} socks_connect_ans;
#pragma pack()    

#ifndef SINGLE_THREAD_CLIENT
DWORD WINAPI ReceiveThreadProc(cdp_t cdp);
#endif

BYTE cTcpIpAddress::Link(cdp_t cdp)
{	int err;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)	return KSE_CONNECTION;
  if (firewall_adr)
  {// find sockc port:
    struct servent * se;
    se=getservbyname("socks", NULL); 
   // send command to socks:
    socks_connect sc(acCallName.sin_port, acCallName.sin_addr.s_addr);
    acCallName.sin_addr.s_addr=htonl(firewall_adr);
    acCallName.sin_port = se==NULL ? htons(1080) /* default SOCKS port */  : se->s_port;
	  if (connect(sock, (sockaddr*)&acCallName, sizeof(acCallName)) == SOCKET_ERROR)
	  {	err = WSAGetLastError();     // spojeni se nezdarilo
		  closesocket(sock);
		  return KSE_FWNOTFOUND;
	  }
    if (send(sock, (tptr)&sc, sizeof(socks_connect), 0) != sizeof(socks_connect))
	  {	err = WSAGetLastError();     // spojeni se nezdarilo
		  closesocket(sock);
		  return KSE_FWCOMM;
	  }
    socks_connect_ans sca;
    if (recv(sock, (tptr)&sca, sizeof(socks_connect_ans), 0) != sizeof(socks_connect_ans))
	  {	err = WSAGetLastError();     // spojeni se nezdarilo
		  closesocket(sock);
		  return KSE_FWCOMM;
	  }
    if (sca.result!=90)
		{ closesocket(sock);
		  return KSE_FWDENIED;
	  }
  }
  else
	{ err = connect(sock, (sockaddr*)&acCallName, sizeof(acCallName));
	  if (err == SOCKET_ERROR)
	  {	err = WSAGetLastError();     // spojeni se nezdarilo
		  closesocket(sock);  sock=-1;
		  return KSE_CONNECTION;
	  }
  }
	BOOL b=TRUE;  //int len;
	err = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*) &b, sizeof(BOOL));
//err = getsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*) &b, &len);
	err = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*) &b, sizeof(BOOL));
#ifdef SINGLE_THREAD_CLIENT
  ReceivePrepare(cdp);
#else
 // start private receiver:
  unsigned id;  
  hReceiveThread=(HANDLE)_beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE)ReceiveThreadProc, (void*) cdp, 0, &id);
  SetThreadPriority(hReceiveThread, THREAD_PRIORITY_HIGHEST);
#endif
	return KSE_OK;
}

BOOL cTcpIpAddress::Unlink(cdp_t cdp, BOOL fTerminateConnection)
// cdp->pRemAddr is not NULL, cdp->in_use==PT_REMOTE
{ SendCloseConnection(this);  // this is necessary for Unix client
  closesocket(sock);      // this works on the Win32 client
  sock=-1;
 // wait for the receive thread to stop (Crashes in WSACleanup on NT 4.0 if doesn't wait)  
#ifdef SINGLE_THREAD_CLIENT
  ReceiveUnprepare(cdp);
#else
  if (hReceiveThread)
  { WaitForSingleObject(hReceiveThread, INFINITE);
    CloseHandle(hReceiveThread);  hReceiveThread=0;
  }
#endif
  return TRUE;
}

BOOL cTcpIpAddress::SendSqp(cdp_t cdp, int type)
// Sends datagram to connected address, primary port, with type and own IP address. Returns TRUE if sended OK. Returns error for HTTP tunnel.
{	BOOL res=FALSE;
  if (!is_http)
  {// prepare datagram containing type and own IP address:
    char buff[30];  
	  { SOCKADDR_IN my_addr;
      int len = sizeof(my_addr);
      if (getsockname(sock, (struct sockaddr *)&my_addr, &len) == SOCKET_ERROR)
        { client_error_message(cdp, "SQP err0 %u", WSAGetLastError());  return FALSE; }
      sprintf(buff, " %lu", my_addr.sin_addr.s_addr);
	    buff[0] = type;
    }
   // send it:
    SOCKADDR_IN sin;
	  SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	  if (sock == INVALID_SOCKET)	
      { client_error_message(cdp, "SQP err1");  return FALSE; }
    SET_SOCKADDR(sin,0,htonl(INADDR_ANY));
	  if (bind(sock, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	    client_error_message(cdp, "SQP err2 %u", WSAGetLastError());  
    else
    { unsigned primary_port = ntohs(acCallName.sin_port)-1;
      SET_SOCKADDR(sin, primary_port, acCallName.sin_addr.S_un.S_addr);
	    if (sendto(sock, buff, (int)strlen(buff)+1, 0, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	      client_error_message(cdp, "Network error when sending a datagram %u %x %u", WSAGetLastError(), acCallName.sin_addr.S_un.S_addr, primary_port);
	    else res=TRUE;
    }
    closesocket(sock);	
  }
  return res;
}


BOOL CreateTcpIpAddress(char * ip_addr, unsigned port, cAddress **ppAddr, bool is_http)
// When server is not registered, [is_http] is always false.
{ SOCKADDR_IN sin;  
  sin.sin_addr.s_addr = inet_addr(ip_addr);
  if (sin.sin_addr.s_addr!=INADDR_NONE) // converted
  //hostent *	hp = gethostbyname(ip_addr);
  //if (hp) 
	{ cTcpIpAddress * pAddr = new cTcpIpAddress();
	  pAddr->acCallName.sin_family=AF_INET;
    pAddr->is_http=is_http;
    if (is_http)
      pAddr->acCallName.sin_port=htons(port); 
    else
      pAddr->acCallName.sin_port=htons(port+1);
	  //bcopy(hp->h_addr, &pAddr->acCallName.sin_addr, hp->h_length);
    memcpy(&pAddr->acCallName.sin_addr, &sin.sin_addr, sizeof(sin.sin_addr));
	  pAddr->store_rem_ident();
    *ppAddr=pAddr;
    return TRUE;
  }
  else return FALSE;
}

//
// *********************   Query ***********************
//
static void SendToKnownServers(cdp_t cdp, SOCKET sock, char *msg)    // 32 bits
{ tobjname server_name;  
  t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
  while (es.next(server_name))
  { char ip_addr[IP_ADDR_MAX+1];  unsigned ip_port;
    if (!es.GetDatabaseString(server_name, IP_str, ip_addr, sizeof(ip_addr))) *ip_addr=0;
    if (!es.GetDatabaseString(server_name, IP_socket_str, &ip_port, sizeof(ip_port))) ip_port=DEFAULT_IP_SOCKET;
   /* send server request: */
    if (*ip_addr)  /* address specified */
	  { SOCKADDR_IN sin;  
	    sin.sin_addr.s_addr = inet_addr(ip_addr);
#ifdef USE_GETHOSTBYNAME
      if (sin.sin_addr.s_addr==INADDR_NONE)  /* not converted */
      { struct hostent * hep = gethostbyname(ip_addr);
        if (hep==NULL) continue;
        if (hep->h_addr_list==NULL || hep->h_addr_list[0]==NULL) continue;
        sin.sin_addr.s_addr=*(uns32*)hep->h_addr_list[0];
      }
#endif // lze vytacet->vytaci. Nelze vytacet nebo cancel vytaceni: vrati NULL
      if (sin.sin_addr.s_addr==INADDR_NONE) continue; /* not converted */
	    sin.sin_family			= AF_INET;
	    sin.sin_port				= htons(ip_port);
	    if (sendto(sock, msg, (int)strlen(msg)+1, 0, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) 
        client_error_message(cdp, "Network error when browsing for servers %u %x %u", WSAGetLastError(), sin.sin_addr.S_un.S_addr, ip_port);
    }
  }
}

SOCKET create_sipsock(void)
// Defines sip_listen_port. 
{ SOCKET asipsock = socket(AF_INET, SOCK_DGRAM, 0);
  if (asipsock != INVALID_SOCKET) 
  { BOOL bVal = TRUE;  int err;
    err = setsockopt(asipsock, SOL_SOCKET, SO_REUSEADDR, (char*)&bVal, sizeof(BOOL));
   // bind it and define [sip_listen_port], which will be used by TcpIpQueryForServer():
    struct sockaddr_in sin;
    SET_SOCKADDR(sin,DEFAULT_IP_SOCKET+1,htonl(INADDR_ANY));
    if (!bind(asipsock, (struct sockaddr*)&sin, sizeof(sin)))
      sip_listen_port=DEFAULT_IP_SOCKET+1;
    else  // port number in use, try another
    { SET_SOCKADDR(sin,0,htonl(INADDR_ANY)); // any port
      if (!bind(asipsock, (struct sockaddr*)&sin, sizeof(sin)))
      { socklen_t len = sizeof(sin);
        getsockname(asipsock, (struct sockaddr*)&sin, &len); // retrieve the port numbers assigned by the system
        sip_listen_port=ntohs(sin.sin_port);
      }
      else // not bound
      { closesocket(asipsock);
        asipsock = INVALID_SOCKET;  sip_listen_port=0;
      }
    }
  }   
  return asipsock;
} 


#include <time.h>

// Listens on given socket for given time, returns servers via [cbck].
inline static int read_replies(SOCKET sipsock, int timeout, server_callback cbck, void * data)
{ char buff[512];
#if 0
  struct WSAPOLLFD sip={sipsock, POLLIN, 0};
  struct sockaddr_in cli_addr;
  socklen_t len = sizeof(cli_addr);
  if (!cbck) cbck=print_server;
  while (TRUE)
  { int err;
    err=WSAPoll(&sip, 1, timeout);
    if (0==err) 
        break; /* time out */
    if (-1==err)
	    break;
    if (!(sip.revents&POLLIN))
	    continue;
    err = recvfrom(sipsock, buff, sizeof(buff), 0, (struct sockaddr*) &cli_addr, &len);
    if (err == -1 || err == 0) 
        break;
    if (buff[0] == SQP_SERVER_NAME_CURRENT)
    {
	    sig32 port;
	    char * p=buff+1;  while (*p && *p!=1) p++;
	    if (*p==1) {
		    *p=0;  port=atol(p+1);
	    }
	    else port=DEFAULT_IP_SOCKET;
	    cli_addr.sin_port=htons(port+1);  // server port
          //client_error_message(NULL, buff+1);
	    if (!cbck(buff+1, &cli_addr, data)) return 0;
    }
  }
#else
  struct sockaddr_in cli_addr;
  socklen_t len = sizeof(cli_addr);
  time_t start_time = time(NULL);  
  timeval tv;
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sipsock, &fds); 
  while (select(0, &fds, NULL, NULL, &tv))
  {
    int err = recvfrom(sipsock, buff, sizeof(buff), 0, (struct sockaddr*) &cli_addr, &len);
    if (err == -1 || err == 0) 
        break;
    if (buff[0] == SQP_SERVER_NAME_CURRENT)
    {
	    sig32 port;
	    char * p=buff+1;  while (*p && *p!=1) p++;
	    if (*p==1) {
		    *p=0;  port=atol(p+1);
	    }
	    else port=DEFAULT_IP_SOCKET;
	    cli_addr.sin_port=htons(port+1);  // server port
	    if (!cbck(buff+1, &cli_addr, data)) return 0;
	  }  
	 // check the total time: 
    time_t el_time = time(NULL) - start_time;  
    if (el_time > timeout / 1000) break;
    tv.tv_sec = (long)(timeout / 1000 - el_time);
    tv.tv_usec = (timeout % 1000) * 1000;
  }
#endif
  return 0;
}

DllKernel int enum_servers(int timeout, server_callback cbck, void * data)
// Synchronous enumerating servers. 
//  Opens SIP port. Sends SQP and waits for [timeout] millisends for the responses from servres.
//  Calls the callback [cbck] for every response received.
{  
  NetworkStart(TCPIP);  // no problem if it has already been started
   // open the reuseable socket: 
    SOCKET my_sipsock = create_sipsock();
    if (sipsock == INVALID_SOCKET)
      return -1;
   // send SQP and read answers: 
    TcpIpQueryForServer();
    read_replies(my_sipsock, timeout, cbck, data);
    closesocket(my_sipsock);
  NetworkStop(NULL);  // no problem if it has already been started before
  return 0;
}

DWORD WINAPI SipThreadProc(LPVOID)
// Client thread receiving server names
{	sipsock = create_sipsock();
	if (sipsock == INVALID_SOCKET)
    client_error_message(NULL, "SIP socket creation error %u", WSAGetLastError());
  else  
  { if (sip_listen_port)
	  { InterlockedIncrement(&threadCount);  /* J.D. moved from TcpIpQueryInit */
	    while (TRUE)
	    { SOCKADDR_IN cli_addr;  int len = sizeof(cli_addr);  int err;	char buff[512]; 	
		    err = recvfrom(sipsock, buff, sizeof(buff), 0, (sockaddr*)&cli_addr, &len);
		    if (err == SOCKET_ERROR || err == 0) break;
		    if (buff[0] == SQP_SERVER_NAME_CURRENT)
		    {	sig32 port;
          tptr p=buff+1;  while (*p && *p!=1) p++;
          if (*p==1) { *p=0;  str2int(p+1, &port); }
          else port=DEFAULT_IP_SOCKET;
          cli_addr.sin_port=htons(port+1);  // server port
          cAddress *pAddr = new cTcpIpAddress(&cli_addr);
			    oSrchSrvrList.Bind ((tptr)(buff+1), 0, 0, pAddr, cli_addr.sin_addr.S_un.S_addr, (uns16)port);			// zarad jej do seznamu
		    }
	    }
	    if (!InterlockedDecrement(&threadCount))
		    SetEvent(hShutDown);
    }
	  if (sipsock)		// pokud nebyl sipsock zavren z hlavniho threadu, tak to udelej
	    { closesocket(sipsock);  sipsock = 0;	}
  }
	return 0;
}

CFNC DllExport BOOL TcpIpQueryInit(void)
{ unsigned id;
	HANDLE hnd = (HANDLE)_beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE)SipThreadProc, 0, 0, &id);
  if (!hnd) return KSE_SYNCHRO_OBJ;
  SetThreadPriority(hnd, THREAD_PRIORITY_HIGHEST);
  CloseHandle(hnd);
	return KSE_OK;
}

BOOL TcpIpQueryDeinit(void)
{	if (sipsock)          // zrus cekani na odezvy od serveru
	{	SOCKET sips = sipsock;
		sipsock = 0;  // must first set 0 and then closesocket(), otherwise SipThreadProc calls closesocket(), too
    closesocket(sips);
	}
	return KSE_OK;
}

int querried=0;

CFNC DllExport int TcpIpQueryForServer(void)
{	SOCKET sock;	SOCKADDR_IN sin;	int err, result = KSE_WINSOCK_ERROR;
 // prepare the packet:
	char buff[16]; 
  buff[0] = SQP_FOR_SERVER;
  if (sip_listen_port==DEFAULT_IP_SOCKET+1) strcpy(buff+1, "QueryForServer");
  else int2str(sip_listen_port, buff+1);
  cdp_t cdp = NULL;
	if (querried)	return TRUE;  // zbytecne hromadeni dotazu, proto ven.
	querried = 1;

// create dial-up connection as a side-effect of connect:
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
    client_error_message(cdp, "Query err1 %u", WSAGetLastError());  
  else
  { SET_SOCKADDR(sin,0,htonl(INADDR_ANY));
	  if (bind(sock, (sockaddr*) &sin, sizeof(sin)) == SOCKET_ERROR)
      client_error_message(cdp, "Query err2 %u", WSAGetLastError());  
    else
	  { SendToKnownServers(cdp, sock, buff);
	    if (can_broadcast())
	    { BOOL b = TRUE;
  	    err = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*) &b, sizeof(BOOL)); // povol BROADCAST
        SET_SOCKADDR(sin, DEFAULT_IP_SOCKET, htonl(INADDR_BROADCAST));
  	    if (sendto(sock, buff, (int)strlen(buff)+1, 0, (sockaddr*) &sin, sizeof(sin)) == SOCKET_ERROR)
          client_error_message(cdp, "Network error %u when broadcasting to SQL servers.", WSAGetLastError());  
        else result=KSE_OK;
	    }
      else result=KSE_OK;
    }
	  closesocket(sock);
  }
	querried = 0;
	return result;
}

#include "httpsupp.c"

//
// ********************* Receive ****************************
//
// Prijimaci funkce pro CLIENTA 
#define BASIC_BUFSIZE 1024

DWORD WINAPI ReceiveThreadProc(cdp_t cdp)
{	char * buffer=NULL;			// buffer pro zpravu

	if (!cdp->pRemAddr)	return 1;
  cTcpIpAddress * adr = (cTcpIpAddress*)cdp->pRemAddr;
  if (adr->is_http)
  { t_buffered_socket_reader bsr(cdp);  
    do
    { char * body;  int bodysize;  int method;
      method = http_receive(cdp, bsr, body, bodysize);
      if (method==HTTP_METHOD_RESP)
        if (body)
          ReceiveHTTP(cdp, body, bodysize);
      corefree(body);
    } while (!bsr.terminated);
  }  
  else
  {
    buffer = new char[BASIC_BUFSIZE];  // alokace bufferu pro zpravu
  	int err;          // result of the recv functon
	  uns32 size;				  // delka zpravy
	  int received;	    // pocet prijatych bajtu zpravy
	  while (true)
	  {// receiving packet size info:
      received = 0;
		  do
		  { if (!cdp->pRemAddr) { err = SOCKET_ERROR;  break; }
        err = recv(adr->sock, (char*)&size+received, sizeof(uns32)-received, 0);
        if (err == 0)  // to nastava, kdy Novell server rozpoji spojeni (vyskytne se take po neuspesnem pripojeni klienta na Unix nebo Novell server)
          { err = SOCKET_ERROR;  break; }
        else if (err == SOCKET_ERROR) // no nstava, kdyz klient prvede Unlink nebo kdyz Win32 server rozpoji spojeni
          break;
			  received += err;
		  }	while (received < (int)sizeof(uns32));

		  if (err == SOCKET_ERROR)
      { WSAGetLastError();
        goto unlink_broken;
      }

     // reallocate packet buffer for long packets:
		  if (size > BASIC_BUFSIZE)
  		  {	delete [] buffer;  buffer = new char[size]; }

     // receiving packet data:
		  received = 0;  
		  do
		  { if (!cdp->pRemAddr) { err = SOCKET_ERROR;  break; }
        err = recv(adr->sock, buffer+received, size-received-sizeof(int), 0);
        if (err==0)
          { err = SOCKET_ERROR;  break; }
        else if (err == SOCKET_ERROR) 
          break;
		    received += err;  // received err bytes, no error
		  } while (received < size-(int)sizeof(int));

		  if (err == SOCKET_ERROR)
      { WSAGetLastError();
        goto unlink_broken;
      }

     // processing packet and releasing buffer:
		  if (!Receive(cdp, (unsigned char*)buffer, received)) // to nastava, kdyz je klient odpojen Unix serverem
      { SendCloseConnection(cdp->pRemAddr);
        Sleep(500);
        goto unlink_broken;
      }
		  if (size > BASIC_BUFSIZE)
  		  {	delete [] buffer;  buffer = new char[BASIC_BUFSIZE]; }
	  }
  }
 // connection has been closed by the Unlink or the link is broken (by the server or by the network):
 unlink_broken:
  if (adr->process_broken_link)  // unless normal Unlink issued by the client or Link in progress
  { StopReceiveProcess(cdp);  // reports the termination
    closesocket(adr->sock);  adr->sock=-1;      // like in TCP/IP Unlink
    delete cdp->pRemAddr;  cdp->pRemAddr=NULL;  // like in common Unlink
  }
	if (buffer) delete [] buffer;
	return 0;
}

// TCP/IP does not have common receivers. 
// Each thread receives input by calling Receive Work instead.
BOOL TcpIpReceiveStart(void)  {	return KSE_OK; }
BOOL TcpIpReceiveStop(void)   { return KSE_OK; }


//////////////////////////////////////// ping /////////////////////////////////////
//#define WIN32_LEAN_AND_MEAN 
//#include <winsock.h> 
 
#define ICMP_ECHO 8 
#define ICMP_ECHOREPLY 0 
 
#define ICMP_MIN 8 // minimum 8 byte icmp packet (just header) 
 
/* The IP header */ 
typedef struct iphdr  
{ unsigned int h_len:4;          // length of the header 
  unsigned int version:4;        // Version of IP 
  unsigned char tos;             // Type of service 
  unsigned short total_len;      // total length of the packet 
  unsigned short ident;          // unique identifier 
  unsigned short frag_and_flags; // flags 
  unsigned char  ttl;  
  unsigned char proto;           // protocol (TCP, UDP etc) 
  unsigned short checksum;       // IP checksum 
 
  unsigned int sourceIP; 
  unsigned int destIP; 
} IpHeader; 
 
// 
// ICMP header 
// 
typedef struct _ihdr 
{ BYTE i_type; 
  BYTE i_code; /* type sub code */ 
  USHORT i_cksum; 
  USHORT i_id; 
  USHORT i_seq; 
  /* This is not the std header, but we reserve space for time */ 
  ULONG timestamp; 
} IcmpHeader; 
 
#define STATUS_FAILED 0xFFFF 
#define DEF_PACKET_SIZE 32 
#define MAX_PACKET 1024 
 
#include <ras.h>
BOOL ras_dial(const char * conn_name);

// This function pings remote machine using undocumented ICMP.DLL
// functions. Result is 0 if it succeeds, error code otherwise.
//#include "ipexport.h" // definice struktur
//#include "icmpapi.h" // deklarace funkci

struct ip_option_information {
    unsigned char      Ttl;             // Time To Live
    unsigned char      Tos;             // Type Of Service
    unsigned char      Flags;           // IP header flags
    unsigned char      OptionsSize;     // Size in bytes of options data
    unsigned char FAR *OptionsData;     // Pointer to options data
}; /* ip_option_information */
typedef unsigned long   IPAddr;     // An IP address.
//
// The icmp_echo_reply structure describes the data returned in response
// to an echo request.
//
struct icmp_echo_reply {
    IPAddr                         Address;         // Replying address
    unsigned long                  Status;          // Reply IP_STATUS
    unsigned long                  RoundTripTime;   // RTT in milliseconds
    unsigned short                 DataSize;        // Reply data size in bytes
    unsigned short                 Reserved;        // Reserved for system use
    void                          *Data;            // Pointer to the reply data
    ip_option_information          Options;         // Reply options
}; /* icmp_echo_reply */
typedef HANDLE (WINAPI *cft)(VOID);
typedef BOOL (WINAPI *cht)(HANDLE  IcmpHandle);
typedef DWORD (WINAPI *set) (HANDLE IcmpHandle, IPAddr DestinationAddress, LPVOID RequestData, WORD RequestSize,
    ip_option_information * RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);

#define IP_STATUS_BASE              11000
#define IP_SUCCESS                  0
#define IP_REQ_TIMED_OUT            (IP_STATUS_BASE + 10)

inline static int tcpip_echo_icmp(HINSTANCE hinstLib, unsigned ip_addr)
{
	// Najdi adresy potrebnych fci v knihovne
	cft IcmpCreateFile=(cft)GetProcAddress(hinstLib, "IcmpCreateFile");
	cht IcmpCloseHandle=(cht)GetProcAddress(hinstLib, "IcmpCloseHandle");
	set IcmpSendEcho=(set)GetProcAddress(hinstLib, "IcmpSendEcho");

	HANDLE hIcmp = IcmpCreateFile();
    if (!hIcmp) {
		FreeLibrary(hinstLib);
		return 4;
	}
	const int size = sizeof(icmp_echo_reply) + 8;
	char buff[size]; // Do bufferu bude vracena odpoved
	icmp_echo_reply* reply=(icmp_echo_reply *)buff;
	DWORD res = IcmpSendEcho(hIcmp, ip_addr, 0, 0, NULL, reply, size, 1500);
	IcmpCloseHandle(hIcmp);
	FreeLibrary(hinstLib);
	if (res==0) return 20; // no reply
	switch(reply->Status){
	case IP_SUCCESS: return 0; // OK, path found
	case IP_REQ_TIMED_OUT: return 9; // timed out
	default: // Pokud by mely byt zajimave ostatni chyby, jsou v includu.
			return 50; 
	}
	// konec ICMP knihovny
}	


CFNC DllKernel int WINAPI tcpip_echo(char * server_name)
{ char ip_addr[IP_ADDR_MAX+1], fw_addr[IP_ADDR_MAX+1], conn_name[RAS_MaxEntryName+1];  BOOL via_fw;  int port;
  get_ip_address(server_name, ip_addr, conn_name, fw_addr, &via_fw, &port, NULL);
  if (via_fw) return 200;
  if (!*ip_addr) return 201;
 // dial if dial-up connection
  if (*conn_name) ras_dial(conn_name);
  return tcpip_echo_addr(ip_addr);
}

CFNC DllKernel int WINAPI tcpip_echo_addr(const char * ip_addr)
{ //WSADATA wsaData; 
  SOCKET sockRaw; 
  struct sockaddr_in dest,from; 
  struct hostent * hp; 
  int bread, datasize, fromlen, timeout; 
  char *icmp_data; 
  char *recvbuf; 

  int res=0;
  BOOL protocol_started_here;
  if (!*ip_addr) return 201;

  if (hShutDown) protocol_started_here=FALSE;
  else
  { if (TcpIpProtocolStart() != KSE_OK) return 202;
    protocol_started_here=TRUE;
  }
  //if (WSAStartup(MAKEWORD(2,1),&wsaData) != 0) return 100;

  memset(&dest,0,sizeof(dest)); 
  hp = gethostbyname(ip_addr); 
  if (!hp) 
	{ unsigned addr = inet_addr(ip_addr); 
      if (addr == INADDR_NONE) { res=4;  goto err1; }
      dest.sin_addr.s_addr = addr; 
      dest.sin_family = AF_INET; 
	} 
  else
    { memcpy(&(dest.sin_addr), hp->h_addr, hp->h_length); 
      dest.sin_family = hp->h_addrtype; 
    }
	HINSTANCE hinstLib;
	if ((hinstLib=LoadLibrary("ICMP.DLL"))!=NULL)
		return	tcpip_echo_icmp(hinstLib, dest.sin_addr.s_addr);	
  sockRaw = socket(AF_INET, SOCK_STREAM, 0); 
  if (sockRaw == INVALID_SOCKET) res=1;
  else
  { timeout = 1000; 
    bread = setsockopt(sockRaw,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)); 
    if (bread == SOCKET_ERROR) { res=2;  goto err1; }
    timeout = 1000; 
    bread = setsockopt(sockRaw,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout, sizeof(timeout)); 
    if (bread == SOCKET_ERROR) { res=3;  goto err1; }

 
    datasize = DEF_PACKET_SIZE + sizeof(IcmpHeader);   
    icmp_data = (char*)corealloc(MAX_PACKET, 55); 
    recvbuf   = (char*)corealloc(MAX_PACKET, 55); 
    if (!icmp_data || !recvbuf) res=5;
    else
    {// prepare the header:
      memset(icmp_data, 0, MAX_PACKET); 
      IcmpHeader * icmp_hdr = (IcmpHeader*)icmp_data; 
      icmp_hdr->i_type = ICMP_ECHO; 
      icmp_hdr->i_code = 0; 
      icmp_hdr->i_id = (USHORT)GetCurrentProcessId(); 
      icmp_hdr->i_cksum = 0; 
      icmp_hdr->i_seq = 0; 
      memset(icmp_data + sizeof(IcmpHeader), 'E', datasize - sizeof(IcmpHeader)); 
      DWORD start;
      //while(1) 
      { icmp_hdr->i_cksum = 0; 
        icmp_hdr->timestamp = start =GetTickCount(); 
        icmp_hdr->i_seq = 0; 
       // checksum:
        USHORT *buffer = (USHORT*)icmp_data;
        int size = datasize;
        unsigned cksum=0; 
        while(size >1) 
        { cksum+=*buffer++; 
          size -=sizeof(USHORT); 
        } 
        if(size) cksum += *(UCHAR*)buffer; 
        cksum = (cksum >> 16) + (cksum & 0xffff); 
        cksum += (cksum >>16); 
        icmp_hdr->i_cksum = (USHORT)(~cksum); 
       // send:
        int bwrote = sendto(sockRaw, icmp_data, datasize, 0, (struct sockaddr*)&dest, sizeof(dest)); 
        if (bwrote == SOCKET_ERROR) 
          { res=WSAGetLastError() == WSAETIMEDOUT ? 6 : 7;  goto err2; }
        if (bwrote < datasize ) { res=8;  goto err2; }
       rerecv:
        if (GetTickCount()-start > 5000) { res=11;  goto err2; } // timeout
        fromlen = sizeof(from); 
        bread = recvfrom(sockRaw, recvbuf, MAX_PACKET, 0, (struct sockaddr*)&from, &fromlen); 
        if (bread == SOCKET_ERROR) 
          { res=WSAGetLastError() == WSAETIMEDOUT ? 9 : 10;  goto err2; }

       // The response is an IP packet. We must decode the IP header to locate the ICMP data  
        IpHeader * iphdr = (IpHeader *)recvbuf; 
        unsigned short iphdrlen = iphdr->h_len * 4 ; // number of 32-bit words *4 = bytes 
        if (bread  < iphdrlen + ICMP_MIN) { res=11;  goto err2;} // to few bytes
        IcmpHeader * icmphdr = (IcmpHeader*)(recvbuf + iphdrlen); 
        if (icmphdr->i_type != ICMP_ECHOREPLY) goto rerecv; // non-echo received
        if (icmphdr->i_id != (USHORT)GetCurrentProcessId()) goto rerecv; // someone else's packet!
      } 
     err2:
      corefree(icmp_data);
      corefree(recvbuf);
    }
   err1:
    closesocket(sockRaw);
  }
  if (protocol_started_here) TcpIpProtocolStop(cds[0]);
  return res; 
} 

#ifdef STOP
CFNC DllKernel int WINAPI tcp_connect_test(char * server_name)
{ //WSADATA wsaData; 
  SOCKET sockRaw; 
  struct sockaddr_in dest,from; 
  struct hostent * hp; 
  int bread, datasize, fromlen, timeout; 
  char *icmp_data; 
  char *recvbuf; 

  int res=0;
  BOOL protocol_started_here;
  char ip_addr[IP_ADDR_MAX+1], fw_addr[IP_ADDR_MAX+1], conn_name[RAS_MaxEntryName+1];  BOOL via_fw;  unsigned port;
  get_ip_address(server_name, ip_addr, conn_name, fw_addr, &via_fw, &port);
  if (via_fw) return 200;
  if (!*ip_addr) return 201;
 // dial if dial-up connection
  if (*conn_name) ras_dial(conn_name);

  if (hShutDown) protocol_started_here=FALSE;
  else
  { if (TcpIpProtocolStart() != KSE_OK) return 202;
    protocol_started_here=TRUE;
  }
  //if (WSAStartup(MAKEWORD(2,1),&wsaData) != 0) return 100;

  memset(&dest, 0, sizeof(dest)); 
  hp = gethostbyname(ip_addr); 
  if (!hp) 
	{ unsigned addr = inet_addr(ip_addr); 
    if (addr == INADDR_NONE) { res=4;  goto err1; }
    dest.sin_addr.s_addr = addr; 
    dest.sin_family = AF_INET; 
	} 
  else
  { memcpy(&(dest.sin_addr), hp->h_addr, hp->h_length); 
    dest.sin_family = hp->h_addrtype; 
  }


 	int err;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)	return KSE_CONNECTION;
  if (via_fw)
  {// find sockc port:
    struct servent * se;
    se=getservbyname("socks", NULL);
   // send command to socks:
    socks_connect sc(acCallName.sin_port, acCallName.sin_addr.s_addr);
    acCallName.sin_addr.s_addr=htonl(fw_addr);
    acCallName.sin_port = se==NULL ? htons(1080) /* default SOCKS port */  : se->s_port;
    socks_connect_ans sca;
	  if (connect(sock, (sockaddr*)&acCallName, sizeof(acCallName)) == SOCKET_ERROR)
	   	res = WSAGetLastError();     // spojeni se nezdarilo
    else if (send(sock, (tptr)&sc, sizeof(socks_connect), 0) != sizeof(socks_connect))
	   	res = WSAGetLastError();     // spojeni se nezdarilo
    else if (recv(sock, (tptr)&sca, sizeof(socks_connect_ans), 0) != sizeof(socks_connect_ans))
	   	res = WSAGetLastError();     // spojeni se nezdarilo
    else if (sca.result!=90)
		  res=KSE_FWDENIED;
  }
  else
	{ if (connect(sock, (sockaddr*)&dest, sizeof(dest)) == SOCKET_ERROR)
	   	res = WSAGetLastError();     // spojeni se nezdarilo
  }
  if (res)
    closesocket(sock);  
  else
  {
  }
  if (protocol_started_here) TcpIpProtocolStop(cds[0]);
  return res; 
} 
#endif
 
#ifdef NEVER  // old version of echo
CFNC DllKernel int WINAPI tcpip_echo(char * server_name)
{ //WSADATA wsaData; 
  SOCKET sockRaw; 
  struct sockaddr_in dest,from; 
  struct hostent * hp; 
  int bread, datasize, fromlen, timeout; 
  char *icmp_data; 
  char *recvbuf; 

  int res=0;
  BOOL protocol_started_here;
  char ip_addr[IP_ADDR_MAX+1], fw_addr[IP_ADDR_MAX+1], conn_name[RAS_MaxEntryName+1];  BOOL via_fw;  unsigned port;
  get_ip_address(server_name, ip_addr, conn_name, fw_addr, &via_fw, &port);
  if (via_fw) return 200;
  if (!*ip_addr) return 201;
 // dial if dial-up connection
  if (*conn_name) ras_dial(conn_name);

  if (hShutDown) protocol_started_here=FALSE;
  else
  { if (TcpIpProtocolStart() != KSE_OK) return 202;
    protocol_started_here=TRUE;
  }
  //if (WSAStartup(MAKEWORD(2,1),&wsaData) != 0) return 100;
  sockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); 
  if (sockRaw == INVALID_SOCKET) res=1;
  else
  { timeout = 1000; 
    bread = setsockopt(sockRaw,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)); 
    if (bread == SOCKET_ERROR) { res=2;  goto err1; }
    timeout = 1000; 
    bread = setsockopt(sockRaw,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout, sizeof(timeout)); 
    if (bread == SOCKET_ERROR) { res=3;  goto err1; }

    memset(&dest,0,sizeof(dest)); 
    hp = gethostbyname(ip_addr); 
    if (!hp) 
    { unsigned addr = inet_addr(ip_addr); 
      if (addr == INADDR_NONE) { res=4;  goto err1; }
      dest.sin_addr.s_addr = addr; 
      dest.sin_family = AF_INET; 
    } 
    else
    { memcpy(&(dest.sin_addr), hp->h_addr, hp->h_length); 
      dest.sin_family = hp->h_addrtype; 
    }
 
    datasize = DEF_PACKET_SIZE + sizeof(IcmpHeader);   
    icmp_data = (char*)corealloc(MAX_PACKET, 55); 
    recvbuf   = (char*)corealloc(MAX_PACKET, 55); 
    if (!icmp_data || !recvbuf) res=5;
    else
    {// prepare the header:
      memset(icmp_data, 0, MAX_PACKET); 
      IcmpHeader * icmp_hdr = (IcmpHeader*)icmp_data; 
      icmp_hdr->i_type = ICMP_ECHO; 
      icmp_hdr->i_code = 0; 
      icmp_hdr->i_id = (USHORT)GetCurrentProcessId(); 
      icmp_hdr->i_cksum = 0; 
      icmp_hdr->i_seq = 0; 
      memset(icmp_data + sizeof(IcmpHeader), 'E', datasize - sizeof(IcmpHeader)); 
      DWORD start;
      //while(1) 
      { icmp_hdr->i_cksum = 0; 
        icmp_hdr->timestamp = start =GetTickCount(); 
        icmp_hdr->i_seq = 0; 
       // checksum:
        USHORT *buffer = (USHORT*)icmp_data;
        int size = datasize;
        unsigned cksum=0; 
        while(size >1) 
        { cksum+=*buffer++; 
          size -=sizeof(USHORT); 
        } 
        if(size) cksum += *(UCHAR*)buffer; 
        cksum = (cksum >> 16) + (cksum & 0xffff); 
        cksum += (cksum >>16); 
        icmp_hdr->i_cksum = (USHORT)(~cksum); 
       // send:
        int bwrote = sendto(sockRaw, icmp_data, datasize, 0, (struct sockaddr*)&dest, sizeof(dest)); 
        if (bwrote == SOCKET_ERROR) 
          { res=WSAGetLastError() == WSAETIMEDOUT ? 6 : 7;  goto err2; }
        if (bwrote < datasize ) { res=8;  goto err2; }
       rerecv:
        if (GetTickCount()-start > 5000) { res=11;  goto err2; } // timeout
        fromlen = sizeof(from); 
        bread = recvfrom(sockRaw, recvbuf, MAX_PACKET, 0, (struct sockaddr*)&from, &fromlen); 
        if (bread == SOCKET_ERROR) 
          { res=WSAGetLastError() == WSAETIMEDOUT ? 9 : 10;  goto err2; }

       // The response is an IP packet. We must decode the IP header to locate the ICMP data  
        IpHeader * iphdr = (IpHeader *)recvbuf; 
        unsigned short iphdrlen = iphdr->h_len * 4 ; // number of 32-bit words *4 = bytes 
        if (bread  < iphdrlen + ICMP_MIN) { res=11;  goto err2;} // to few bytes
        IcmpHeader * icmphdr = (IcmpHeader*)(recvbuf + iphdrlen); 
        if (icmphdr->i_type != ICMP_ECHOREPLY) goto rerecv; // non-echo received
        if (icmphdr->i_id != (USHORT)GetCurrentProcessId()) goto rerecv; // someone else's packet!
      } 
     err2:
      corefree(icmp_data);
      corefree(recvbuf);
    }
   err1:
    closesocket(sockRaw);
  }
  if (protocol_started_here) TcpIpProtocolStop(cds[0]);
  return res; 
} 
#endif
 
