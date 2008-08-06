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
#include <sys/types.h>
#include <sys/socket.h>
#ifndef LINUX
#include <sys/filio.h>
#endif
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#ifdef UNIX
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <error.h>
#include <stdlib.h>
#include <cnetapi.h>

#endif
#include <fcntl.h>

#include "netapi.h"
#include "tcpip.h"
#include "regstr.h"
#include "enumsrv.h"

typedef int SOCKET;
#define INVALID_SOCKET   -1
#define SOCKET_ERROR     -1
#define closesocket(s) close(s)

#define USE_GETHOSTBYNAME
#define IP_ADDR_MAX  100
#define SET_SOCKADDR(sin,port,addr) \
{ (sin).sin_family = AF_INET;\
  (sin).sin_port	 = htons(port);\
  (sin).sin_addr.s_addr = addr; }\

class cTcpIpAddress : public cAddress
/*************************************/
{ public:
  cTcpIpAddress (sockaddr_in *callAddr);
  cTcpIpAddress (void);
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
	BOOL SendSqp(cdp_t cdp, int type);
  BOOL SendBreak(cdp_t cdp)      // Sends client break to server
	  { return SendSqp(cdp, SQP_BREAK); }
	BOOL SendKeepAlive(cd_t * cdp) // Sends client response to the KeepAlive packet
  	{ return SendSqp(cdp, SQP_KEEP_ALIVE); }
  virtual BOOL ReceiveWork(cdp_t cdp, int timeout);  // explicit receiving activity (if not handled by another thread)
#ifdef USE_KERBEROS
  virtual bool krb5_sendauth(const char *servername);         // for TCP/IP
#endif

  void store_rem_ident(void);
  public:
		sockaddr_in acCallName;  // ip address and port defined
		SOCKET  sock;
		BYTE * send_buffer;
    BYTE * tcpip_buffer;
    uns32 firewall_adr;
    char * receive_buffer;	// buffer pro prijimanou zpravu
  t_buffered_socket_reader bsr;  // used by HTTP tunelling only
};


BOOL receive_thread_running=FALSE;
short sip_listen_port = DEFAULT_IP_SOCKET+1;
    
HANDLE keepAlive;  // event "KeepAlive watch received client response"

BOOL TcpIpPresent(void)
{	return TRUE; }

BYTE TcpIpProtocolStart(void)
{ SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) return KSE_NO_WINSOCK; 
  closesocket(sock);
	return KSE_OK;
}

SOCKET sipsock=0;

BYTE TcpIpProtocolStop(cdp_t cdp)
{ if (sipsock)
	  { closesocket(sipsock);     sipsock=0; }
	return KSE_OK;
}

//
// ************************************* cTcpIpAddress ***********************************
//
void cTcpIpAddress::store_rem_ident(void)
{ memcpy(abRemIdent,   &acCallName.sin_addr,   4);
  memcpy(abRemIdent+4, &acCallName.sin_family, 2);
}

cTcpIpAddress::cTcpIpAddress(sockaddr_in *callAddr) : bsr(this)
{ my_protocol=TCPIP;
	memcpy(&acCallName, callAddr, sizeof(*callAddr));
  store_rem_ident();
	send_buffer = new BYTE[TCPIP_BUFFSIZE];  // creates the default send buffer
  tcpip_buffer=NULL;
  firewall_adr=0;
  receive_buffer=NULL;
  is_http=false;
}

cTcpIpAddress::cTcpIpAddress(void) : bsr(this)
{ my_protocol=TCPIP;
  // send buffer init
	send_buffer = new BYTE[TCPIP_BUFFSIZE];  // creates the default send buffer
  tcpip_buffer=NULL;
  firewall_adr=0;
  receive_buffer=NULL;
  is_http=false;
}

cTcpIpAddress::~cTcpIpAddress()
{	if (send_buffer) delete [] send_buffer; }

cAddress * make_fw_address(uns32 srv_ip, unsigned port, uns32 fw_ip)
{ sockaddr_in sin;
  SET_SOCKADDR(sin,port+1,htonl(srv_ip));
  cTcpIpAddress * adr = new cTcpIpAddress(&sin);
  adr->firewall_adr=fw_ip;
  return adr;
}

BOOL cTcpIpAddress::Send(t_fragmented_buffer * frbu, BYTE bDataType, unsigned MsgDataTotal, unsigned &rDataSent)
{ unsigned DataTotal = sizeof (sWbHeader) + sizeof(uns32) + MsgDataTotal;
  BYTE * pBuffSend;  BOOL result = TRUE;
	if (DataTotal>TCPIP_BUFFSIZE)
       pBuffSend = new BYTE[DataTotal];  // creates the extended send buffer
  else pBuffSend = send_buffer;
 // prepare the complete message:
  sWbHeader *pWbHeader = (sWbHeader *) ( (char*) pBuffSend + sizeof(uns32));
  pWbHeader->bDataType     = bDataType;
  pWbHeader->wMsgDataTotal = MsgDataTotal;
  frbu->copy((char*)pBuffSend + sizeof (sWbHeader) + sizeof(uns32), MsgDataTotal);
	*(uns32*)pBuffSend = DataTotal;   // nastav delku zpravy do zpravy
 // send it:
  if (sock == -1)
    { client_log("send without socket");  result=FALSE; }
  else  // iterative sending
	{ unsigned sended = 0;
	  do
	  { int bRetCode = send(sock, (char*)pBuffSend+sended, DataTotal-sended, 0);
		  if (bRetCode>=0) sended = sended + bRetCode;
		  else { client_log("send error");  client_log_i(errno);  result=FALSE;  break; }
	  } while (sended!=DataTotal);
  }
	rDataSent = MsgDataTotal;
	if (DataTotal>TCPIP_BUFFSIZE) delete [] pBuffSend; // delete the extended send buffer
  return result;
}

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

#ifndef SINGLE_THREAD_CLIENT
void * ReceiveThreadProc(cdp_t cdp);
#endif

BYTE cTcpIpAddress::Link(cdp_t cdp)
{	int err;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	  { client_log("Client socket not created");  client_log_i(errno);  return KSE_CONNECTION; }
#ifdef STOP  // SOCKS disabled for the momment
  if (firewall_adr)
  {// find sockc port:
    struct servent * se;
    se=getservbyname("socks", NULL);
   // send command to socks:
    socks_connect sc(acCallName.sin_port, acCallName.sin_addr.s_addr);
    acCallName.sin_addr.s_addr=htonl(firewall_adr);
    acCallName.sin_port = se==NULL ? htons(1080) /* default SOCKS port */  : se->s_port;
	  if (connect(sock, (sockaddr*)&acCallName, sizeof(acCallName)) == SOCKET_ERROR)
	  {	closesocket(sock);
		  return KSE_FWNOTFOUND;
	  }
    if (send(sock, (tptr)&sc, sizeof(socks_connect), 0) != sizeof(socks_connect))
	  {	closesocket(sock);
		  return KSE_FWCOMM;
	  }
    socks_connect_ans sca;
    if (recv(sock, (tptr)&sca, sizeof(socks_connect_ans), 0) != sizeof(socks_connect_ans))
	  {	closesocket(sock);
		  return KSE_FWCOMM;
	  }
    if (sca.result!=90)
		{ closesocket(sock);
		  return KSE_FWDENIED;
	  }
  }
  else
#endif
	{ 
      bool nonblocking, connected=false;
     // set the non-blocking mode, if possible:
      int oldFlag = fcntl(sock, F_GETFL, 0);
      if (fcntl(sock, F_SETFL, oldFlag | O_NONBLOCK) == -1)
        nonblocking=false;
      else
        nonblocking=true;
      err = connect(sock, (sockaddr*)&acCallName, sizeof(acCallName));
	  if (err == SOCKET_ERROR)
	  { if (nonblocking && errno == EINPROGRESS)  // wait for establishing the connection using select
        { fd_set set;  timeval tv;
          FD_ZERO(&set);  FD_SET(sock, &set);
          tv.tv_sec = 5;  tv.tv_usec = 0;
          if (select(sock + 1, NULL, &set, NULL, &tv) > 0)
          { connected=true;
            fcntl(sock, F_SETFL, oldFlag);  // restore the blocking mode
          }
        }
        if (!connected)
        { closesocket(sock);
		  client_log("connect error");  client_log_i(errno);  return KSE_CONNECTION;
        }  
	  }
  }
	BOOL b=TRUE;
	err = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*) &b, sizeof(BOOL));
	err = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*) &b, sizeof(BOOL));

#ifdef SINGLE_THREAD_CLIENT
  ReceivePrepare(cdp);
#else
 // start private receiver:
  pthread_t id;
  pthread_create(&id, NULL, (void *(*)(void *))::ReceiveThreadProc, cdp);
#endif
  return KSE_OK;
}

BOOL cTcpIpAddress::Unlink(cdp_t cdp, BOOL )
{ if (cdp->in_use==PT_REMOTE)
  { if (!SendCloseConnection(this)) client_log("not sended in Unlink");
//	 closesocket(sock);
#ifdef SINGLE_THREAD_CLIENT
    ReceiveUnprepare(cdp);
#else
    while (receive_thread_running)
      WinWait(0, 0);
#endif
    return TRUE;
  }
	return closesocket(sock) != SOCKET_ERROR;
}

BOOL cTcpIpAddress::SendSqp(cd_t * cdp, int type)
// Sends datagram to connected address, primary port, with type and own IP address
{	char buff[30];
 // prepare datagram:
	{ sockaddr_in my_addr;
    socklen_t len = sizeof(my_addr);
    if (getsockname(sock, (struct sockaddr *)&my_addr, &len) == SOCKET_ERROR)
      return FALSE;
    sprintf(buff, " %lu", my_addr.sin_addr.s_addr);
	  buff[0] = type;
  }
 // send it:
  sockaddr_in sin;  int err;
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)	return FALSE;
  SET_SOCKADDR(sin,0,htonl(INADDR_ANY));
	if (bind(sock, (sockaddr*) &sin, sizeof(sin)) == SOCKET_ERROR)
	{	closesocket(sock);  return FALSE;
	}
  unsigned primary_port = ntohs(acCallName.sin_port)-1;

  SET_SOCKADDR(sin, primary_port, acCallName.sin_addr.s_addr);
	if (sendto(sock, buff, strlen(buff)+1, 0, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{	closesocket(sock);	return FALSE;
	}
	closesocket(sock);
	return TRUE;
}


#ifdef UNIX
BOOL CreateTcpIpAddress(char * ip_addr, unsigned port, cAddress **ppAddr, bool is_http)
// When server is not registered, [is_http] is always false.
{ hostent *	hp = gethostbyname(ip_addr);
  if (!hp) return FALSE;
	cTcpIpAddress * pAddr = new cTcpIpAddress();
	// a ted se to musi jeste naplnit
	pAddr->acCallName.sin_family=AF_INET;
  pAddr->is_http=is_http;
  if (is_http)
    pAddr->acCallName.sin_port=htons(port); 
  else
    pAddr->acCallName.sin_port=htons(port+1);
	bcopy(hp->h_addr, &pAddr->acCallName.sin_addr, hp->h_length);
	pAddr->store_rem_ident();
  *ppAddr=pAddr;
  return TRUE;
};

BOOL GetDatabaseString(const char * server_name, const char * value_name, void * dest, DWORD key_len)
{
  BOOL result=TRUE;
  if (key_len>sizeof(DWORD))
  { char *buf=(char *)alloca(key_len);
    int len = GetPrivateProfileString(server_name, value_name, "\1", buf, key_len, configfile);
    if (*buf==1) result=FALSE;
    else memcpy(dest, buf, len+1);  // must not use strcpy, when value_name==NULL then buf may contain 0s
  }
  else  // reading a number
  { int val = GetPrivateProfileInt(server_name, value_name, -12345, configfile);
    if (val==-12345) result=FALSE;
    else if (key_len==sizeof(int))   *(int  *)dest=val;
    else if (key_len==sizeof(short)) *(short*)dest=(short)val;
    else if (key_len==sizeof(char))  *(char *)dest=(char)val;
  }
  return result;
}

static void SendToKnownServers(cdp_t cdp, int sock, char *msg)    // 32 bits
{ tobjname server_name;
  t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
  while (es.next(server_name))
  { char ip_addr[IP_ADDR_MAX+1];  unsigned ip_port;
    if (!es.GetDatabaseString(server_name, IP_str, ip_addr, sizeof(ip_addr))) *ip_addr=0;
    if (!es.GetDatabaseString(server_name, IP_socket_str, &ip_port, sizeof(ip_port))) ip_port=DEFAULT_IP_SOCKET;
   /* send server request: */
    if (*ip_addr)  /* address specified */
    { struct sockaddr_in sin;  
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
      int err = sendto(sock, msg, strlen(msg)+1, 0, (struct sockaddr*)&sin, sizeof(sin));
      if (err == -1) 
        client_error_message(cdp, "Network error when browsing for servers %u %x %u", err, sin.sin_addr.s_addr, ip_port);
    }
  }
}

/*
 Poli �ost o serverov�info vem serverm na lok�n� segmentu.
 V okamiku vol��by u m� bt aktivn�naslouchaj��socket, jinak meme o
 n�o pij�.*/
int TcpIpQueryForServer(void)
{ int err;
  int sock;  struct sockaddr_in s_in;
  BOOL b = TRUE;
 // prepare the packet:
	char buff[16];
  buff[0] = SQP_FOR_SERVER;
  if (sip_listen_port==DEFAULT_IP_SOCKET+1) strcpy(buff+1, "QueryForServer");
  else int2str(sip_listen_port, buff+1);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET){
	    perror("socket");
	    return KSE_WINSOCK_ERROR;
    }

    SET_SOCKADDR(s_in,0,htonl(INADDR_ANY));
    err = bind(sock, (struct sockaddr*) &s_in, sizeof(s_in));
    if (err == -1)
    {	closesocket(sock);
	    perror("bind");
	    return KSE_WINSOCK_ERROR;
    }
    SendToKnownServers(NULL, sock, buff);
    if (can_broadcast())
	  { err = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*) &b, sizeof(BOOL)); // povol BROADCAST
	    SET_SOCKADDR(s_in,DEFAULT_IP_SOCKET,htonl(INADDR_BROADCAST));
	    err = sendto(sock, buff, strlen(buff)+1, 0, (struct sockaddr*) &s_in, sizeof(s_in));  // posli dotaz
	    if (err == -1)
      { client_error_message(NULL, "Network error %u when broadcasting to SQL servers.", errno);  
	      closesocket(sock);
		    return KSE_WINSOCK_ERROR;
	    }
    }
    closesocket(sock);
    return KSE_OK;
}

static int print_server(const char *name, struct sockaddr_in* addr, void * data)
{
    const char *dotsname;
    dotsname=inet_ntoa(addr->sin_addr);
    printf("%15s:%-7d%s\n", dotsname, ntohs(addr->sin_port), name);
    return 1;
}

// Listens on given socket for given time, returns servers via [cbck].
inline static int read_replies(int sipsock, int timeout, server_callback cbck, void * data)
{ char buff[512];
  struct pollfd sip={sipsock, POLLIN, 0};
  struct sockaddr_in cli_addr;
  socklen_t len = sizeof(cli_addr);
  if (!cbck) cbck=print_server;
  while (TRUE)
  { int err;
    err=poll(&sip, 1, timeout);
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
  return 0;
}
/* }}} */

int create_sipsock(void)
// Defines sip_listen_port. 
{ int asipsock = socket(AF_INET, SOCK_DGRAM, 0);
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

DllKernel int enum_servers(int timeout, server_callback cbck, void * data)
// Synchronous enumerating servers. 
//  Opens SIP port. Sends SQP and waits for [timeout] millisends for the responses from servres.
//  Calls the callback [cbck] for every response received.
{   int err;
   // open the reuseable socket: 
    int my_sipsock = create_sipsock();
    if (sipsock == INVALID_SOCKET)
      return -1;
   // send SQP and read answers: 
    TcpIpQueryForServer();
    read_replies(my_sipsock, timeout, cbck, data);
    close(my_sipsock);
    return 0;
}

#endif

extern t_server_scan_callback * server_scan_callback;

void thread_proc_clean(void  *soc_v)
{
	int *soc=(int *)soc_v;
  closesocket(*soc);
  *soc=-1;
}

THREAD_PROC(SipThreadProc)
// Client thread receiving server names
{	sipsock = create_sipsock();
	if (sipsock == INVALID_SOCKET)
    client_error_message(NULL, "SIP socket creation error %u", errno);
  else  
  { if (sip_listen_port)
	  {
#ifdef LINUX /* We may abort by cancellation, thus should register cleanup */
      pthread_cleanup_push(thread_proc_clean, &sipsock);
#endif
      while (TRUE)
	    { struct sockaddr_in cli_addr;  socklen_t len = sizeof(cli_addr);  int err;	char buff[512];
		    err = recvfrom(sipsock, buff, sizeof(buff), 0, (sockaddr*)&cli_addr, &len);
		    if (err == SOCKET_ERROR || err == 0) break;
		    if (buff[0] == SQP_SERVER_NAME_CURRENT)
		    {	sig32 port;
          tptr p=buff+1;  while (*p && *p!=1) p++;
          if (*p==1) { *p=0;  str2int(p+1, &port); }
          else port=DEFAULT_IP_SOCKET;
          cli_addr.sin_port=htons(port+1);  // server port
          //cAddress *pAddr = new cTcpIpAddress(&cli_addr);
			    //oSrchSrvrList.Bind ((tptr)(buff+1), 0, 0, pAddr);			// zarad jej do seznamu
          if (server_scan_callback)
            (*server_scan_callback)((tptr)(buff+1),
                             cli_addr.sin_addr.s_addr, cli_addr.sin_port);

		    }
	    }
#ifdef LINUX
      pthread_cleanup_pop(1);
#endif
    }
	  if (sipsock)		// pokud nebyl sipsock zavren z hlavniho threadu, tak to udelej
	    { closesocket(sipsock);  sipsock = 0;	}
  }
  THREAD_RETURN;
}

THREAD_HANDLE sip_thread = 0;

BOOL TcpIpQueryInit(void)  // create a joinable thread
{
  if (pthread_create(&sip_thread, NULL, SipThreadProc, NULL))  // returns 0 on success
    { perror("pthread_create");  sip_thread=0;  return KSE_WINSOCK_ERROR; }
  return KSE_OK;
}

BOOL TcpIpQueryDeinit(void)
{ if (!sip_thread) return FALSE;
  pthread_cancel(sip_thread);
  pthread_join(sip_thread, NULL);
  sip_thread=0;
  return TRUE;
}

//
// ********************* Receive ****************************
// Prijimaci funkce pro CLIENTA 32 bitu
#include "httpsupp.c"

#define BASIC_BUFSIZE 1024

BOOL ReceivePrepare(cdp_t cdp)
{	if (!cdp->pRemAddr)	return FALSE;
  if (cdp->pRemAddr->my_protocol==TCPIP)
  { cTcpIpAddress * adr = (cTcpIpAddress*)cdp->pRemAddr;
    adr->receive_buffer = new char[BASIC_BUFSIZE];  // alokace bufferu pro zpravu
    return adr->receive_buffer!=NULL;
  }
  return TRUE;
}

/* Cekej na zpravu (pak lze recv) ci timeout (pak se vrat s chybou) */\
#ifdef SINGLE_THREAD_CLIENT
#define WAIT_FOR_DATA \
	    err=poll(&ufds, 1, timeout);\
	    if (err==0 || err==-1 || !(ufds.revents&POLLIN)) return FALSE;
#else
#define WAIT_FOR_DATA /* nic */
#endif

/* Explicit receiving activity. Returns FALSE iff link broken.
   Timeout in miliseconds (as in poll()) */
BOOL cTcpIpAddress::ReceiveWork(cdp_t cdp, int timeout)
{ 
  if (is_http)
  { char * body;  int bodysize;  int method;
    method = http_receive(cdp, bsr, body, bodysize);
    if (body && method==HTTP_METHOD_RESP)
      ReceiveHTTP(cdp, body, bodysize);
    corefree(body);
    return !bsr.terminated;
  }
  else
  { int err;
    int size;	  // delka zpravy
    int received;    // pocet prijatych bajtu zpravy
#ifdef SINGLE_THREAD_CLIENT
    struct pollfd ufds={sock, POLLIN, 0};
#endif
    // receiving packet size info:
    received = 0;
    do {
	    if (!cdp->pRemAddr) { err = SOCKET_ERROR;  break; }
	    WAIT_FOR_DATA
	    err = recv(sock, (char*)&size+received, sizeof(int)-received, 0);
	    if (err == 0||  // to nastava, kdy Novell server rozpoji spojeni (vyskytne se take po neuspesnem pripojeni klienta na Unix nebo Novell server)
		err == SOCKET_ERROR) // to nastava, kdyz klient provede Unlink nebo kdyz Win32 server rozpoji spojeni
		  { client_log("receive error 1");  client_log_i(errno);  return FALSE; }
	    received += err;
    }	while (received < (int)sizeof(int));

    // reallocate packet buffer for long packets:
    if (size > BASIC_BUFSIZE)
    {	delete [] receive_buffer;  receive_buffer = new char[size]; }

    // receiving packet data:
    received = 0;
    do
    { if (!cdp->pRemAddr) return FALSE; /* Muze k tomu vubec dojit? */
	    WAIT_FOR_DATA
	    err = recv(sock, receive_buffer+received, size-received-sizeof(int), 0);
	    if (err==0 || err == SOCKET_ERROR) 
	     { client_log("receive error 2");  client_log_i(errno);  return FALSE; }
	    received += err;  // received err bytes, no error
    } while (received < size-(int)sizeof(int));

    // processing packet and releasing receive_buffer:
    if (!Receive(cdp, (unsigned char*)receive_buffer, received)) // to nastava, kdyz je klient odpojen Unix serverem
    { SendCloseConnection(cdp->pRemAddr);
	    Sleep(500);
	    return FALSE;
    }
    if (size > BASIC_BUFSIZE)
    {	delete [] receive_buffer;  receive_buffer = new char[BASIC_BUFSIZE]; }
    return TRUE;
  }
}

void ReceiveUnprepare(cdp_t cdp)
{	if (!cdp->pRemAddr)	return;
  if (cdp->pRemAddr->my_protocol==TCPIP)
  { cTcpIpAddress * adr = (cTcpIpAddress*)cdp->pRemAddr;
    if (adr->process_broken_link)  // unless normal Unlink issued by the client or Link in progress
    { StopReceiveProcess(cdp);  // reports the termination
      closesocket(adr->sock);  adr->sock=-1;      // like in TCP/IP Unlink
      delete cdp->pRemAddr;  cdp->pRemAddr=NULL;  // like in common Unlink
    }
	  delete [] adr->receive_buffer;  adr->receive_buffer=NULL;
  }
}

#ifndef SINGLE_THREAD_CLIENT
void * ReceiveThreadProc(cdp_t cdp)
{ if (!ReceivePrepare(cdp)) return (void*)1;
  cTcpIpAddress * adr = (cTcpIpAddress*)cdp->pRemAddr;
	receive_thread_running=TRUE;
  while (adr->ReceiveWork(cdp, -1)) ;  // returns FALSE iff link broken
  ReceiveUnprepare(cdp);
  receive_thread_running=FALSE;
	return NULL;
}
#endif

// TCP/IP does not have common receivers. 
// Each thread receives input by calling Receive Work instead.
BOOL TcpIpReceiveStart(void) { return KSE_OK; }
BOOL TcpIpReceiveStop(void)  { return KSE_OK; }

//////////////////////////////////////// ping /////////////////////////////////////
//#define WIN32_LEAN_AND_MEAN 
 
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
 
#ifdef STOP
CFNC DllKernel int WINAPI tcpip_echo(char * server_name)
{ char ip_addr[IP_ADDR_MAX+1], fw_addr[IP_ADDR_MAX+1], conn_name[RAS_MaxEntryName+1];  BOOL via_fw;  int port;
  get_ip_address(server_name, ip_addr, conn_name, fw_addr, &via_fw, &port);
  if (via_fw) return 200;
  if (!*ip_addr) return 201;
  return tcpip_echo_addr(ip_addr);
}
#endif

CFNC DllKernel int WINAPI tcpip_echo_addr(const char * ip_addr)
{ int res=0;
  if (!*ip_addr) return 201;

  if (getuid() != 0)
  // implementation using the system ping utility - works for the non-root user too
  { char cmd[100];
    sprintf(cmd, "ping -c 1 -w 5 %s >nul", ip_addr);
    res=system(cmd);
    if (res<0) res=21;
    else if (res) res=11;  // timeout
    // res==0 -> OK
  }
  else  // root can ping using system API
  { struct sockaddr_in dest, from; 
    int bread, datasize, timeout; 
    char *icmp_data; 
    char *recvbuf; 

    SOCKET sockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); // on Linux works for the root only!
    if (sockRaw == INVALID_SOCKET) res=1;
    else
    { 
#ifndef LINUX  // cannot change timeouts on Linux
        timeout = 1000; 
        bread = setsockopt(sockRaw,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)); 
        if (bread == SOCKET_ERROR) { res=2;  goto err1; }
        timeout = 1000; 
        bread = setsockopt(sockRaw,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout, sizeof(timeout)); 
        if (bread == SOCKET_ERROR) { res=3;  goto err1; }
#endif
       // prepare the sending address:
        memset(&dest,0,sizeof(dest)); 
        unsigned addr = inet_addr(ip_addr); 
        if (addr != INADDR_NONE) 
        { dest.sin_addr.s_addr = addr; 
          dest.sin_family = AF_INET; 
        } 
        else
        { struct hostent * hp = gethostbyname(ip_addr); 
          if (!hp) { res=4;  goto err1; }
          memcpy(&dest.sin_addr, hp->h_addr, hp->h_length); 
          dest.sin_family = hp->h_addrtype; 
        }
        dest.sin_port=0;
       // prepare the packet:
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
        int pid = getpid();
        icmp_hdr->i_id = pid; 
        icmp_hdr->i_cksum = 0; 
        icmp_hdr->i_seq = 0; 
        memset(icmp_data + sizeof(IcmpHeader), 'E', datasize - sizeof(IcmpHeader)); 
        DWORD start;
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
            { res=7;  goto err2; }
            if (bwrote < datasize ) { res=8;  goto err2; }
           // receiving:
#if 0  // problems with the long timeout on Linux
        rerecv:
            if (GetTickCount()-start > 5000) { res=11;  goto err2; } // timeout
            socklen_t fromlen;
            fromlen = sizeof(from); 
            bread = recvfrom(sockRaw, recvbuf, MAX_PACKET, 0, (struct sockaddr*)&from, &fromlen); 
            if (bread == SOCKET_ERROR) 
            { res=10;  goto err2; }
    
        // The response is an IP packet. We must decode the IP header to locate the ICMP data  
            IpHeader * iphdr = (IpHeader *)recvbuf;
            unsigned short iphdrlen = iphdr->h_len * 4 ; // number of 32-bit words *4 = bytes 
            if (bread  < iphdrlen + ICMP_MIN) { res=11;  goto err2;} // to few bytes
            IcmpHeader * icmphdr = (IcmpHeader*)(recvbuf + iphdrlen); 
            if (icmphdr->i_type != ICMP_ECHOREPLY) goto rerecv; // non-echo received
            if (icmphdr->i_id != pid) goto rerecv; // someone else's packet!
#else  // receiving via select, 5 seconds
            fd_set mySet;  timeval tv;  int iphdrlen;  IcmpHeader * icmphdr;
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            do
            {
              FD_ZERO(&mySet);
              FD_SET(sockRaw, &mySet);
              if (select(sockRaw + 1, &mySet, NULL, NULL, &tv) < 0)
                { res=10;  goto err2; }
              if (FD_ISSET(sockRaw, &mySet))
              { socklen_t fromlen;
                fromlen = sizeof(from); 
                bread = recvfrom(sockRaw, recvbuf, MAX_PACKET, 0, (struct sockaddr*)&from, &fromlen); 
                if (bread == SOCKET_ERROR) 
                  { res=10;  goto err2; }
              }
              else 
                { res=11;  goto err2; } // timeout
             // The response is an IP packet. We must decode the IP header to locate the ICMP data  
              IpHeader * iphdr = (IpHeader *)recvbuf;
              iphdrlen = iphdr->h_len * 4 ; // number of 32-bit words *4 = bytes 
              icmphdr = (IcmpHeader*)(recvbuf + iphdrlen); 
            } while (bread < iphdrlen + ICMP_MIN || icmphdr->i_type != ICMP_ECHOREPLY || icmphdr->i_id != pid);  // non-echo received or someone else's packet!
#endif
        } 
        err2:
        corefree(icmp_data);
        corefree(recvbuf);
        }
    err1:
        closesocket(sockRaw);
    }
  }
  return res; 
} 
 
#ifdef USE_KERBEROS
#include <krb5.h>
/* krb5_sendauth()
 *
 * servername: jm�o serveru podle Kerbera (to jest, mus�existovat principal
 * pro tento server)
 * 
 * Pokus�se autentifikovat serveru. Pou��standartn�cache, co by m�o bt
 * dosta�j�� Vrac�true v p�ad�sp�hu, false jinak.
 * V tuto chv�i ned��informace o chyb� viz odkomentovan fprintf �ek.
 *
 * TODO: context by zejm�mohl bt glob�n�.. ale u klienta to nebude hr�
 * a takovou roli...
 */
bool cTcpIpAddress::krb5_sendauth(const char *servername)
{
  krb5_context context;
  krb5_auth_context auth_context;
  krb5_principal server_principal;
  krb5_ap_req req;
  krb5_ticket *ticket;
  krb5_error_code res;

  /* Inicializace */
  res=krb5_init_context(&context);
  if (res) return false;
  res=krb5_auth_con_init(context, &auth_context);
  if (res) goto clean_context;
  res=krb5_parse_name(context, servername, &server_principal);
  if (res) goto clean_auth_context;

  /* Akce: */
  res=::krb5_sendauth(
    context,
    &auth_context,
    &sock, /* stdin */
    "test",
    NULL, /* client - default cache */
    server_principal,
    AP_OPTS_MUTUAL_REQUIRED, /* flags */
    NULL, /* data */
    NULL, /* creds */
    NULL, /* cache */
    NULL, /* kam sloit error */
    NULL,
    NULL);
  if (res) {
    fputs(error_message(res), stderr);
  }
    
  /* Uvol�v��zdroj */
  krb5_free_principal(context, server_principal);
clean_auth_context:
  krb5_auth_con_free(context, auth_context);
clean_context:
  krb5_free_context(context);
  return (res==0);
}

#endif

