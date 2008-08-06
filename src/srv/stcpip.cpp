//
//  tcpip.cpp - protocol dependent calls for sockets & TCP/IP from Windows 95
//  ===================================================================
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
#include <process.h>
#include <winsock.h>
#include "netapi.h"
#include "tcpip.h"
#include "regstr.h"
#include "nlmtexts.h"
#include "cnetapi.h"

#define USE_GETHOSTBYNAME
#define IP_ADDR_MAX  100
#define SET_SOCKADDR(sin,port,addr) \
{ (sin).sin_family = AF_INET;\
  (sin).sin_port	 = htons(port);\
  (sin).sin_addr.s_addr = addr; }\

enum t_conn_types { CONNTP_TCP, CONNTP_TUNNEL, CONNTP_WEB };  // values used for indexing the array of address dynars, IP_SETS_CNT

class cTcpIpAddress : public cAddress
/*************************************/
{ public:
  cTcpIpAddress (SOCKADDR_IN *callAddr);
  cTcpIpAddress (bool is_httpIn=false); // J.D.
  virtual ~cTcpIpAddress();
  BOOL Send(t_fragmented_buffer * frbu, BYTE bDataType, unsigned MsgDataTotal, unsigned &rDataSent);
  BOOL Unlink (cdp_t cdp, BOOL fTerminateConnection);
  cAddress * Copy ()
    { return new cTcpIpAddress (&acCallName); };
  BOOL AddrEqual (cAddress *pAddr)
    { return !memcmp ( &((cTcpIpAddress *)pAddr)->acCallName.sin_addr, &acCallName.sin_addr, sizeof (acCallName.sin_addr)) &&
             ((cTcpIpAddress *)pAddr)->acCallName.sin_port==acCallName.sin_port;
		}
  void RegisterReceiver (cdp_t cdp);
  void UnregisterReceiver ();
  BOOL ReceiveWork(cdp_t cdp, BOOL signal_it);
  void GetAddressString(char * addr); // Return the address of the other peer
  int  ValidatePeerAddress(cdp_t cdp); // check if the address of the client is allowed

  void store_rem_ident(void);
  public:
		SOCKADDR_IN acCallName;
		SOCKET  sock;
		BYTE *   send_buffer;
    BYTE *   tcpip_buffer;
    HANDLE   tcpip_event;
    uns32 firewall_adr;
    OVERLAPPED ovl;
    t_conn_types connection_type;  // defined in addresses created for slaves
#ifdef LINUX
  virtual SOCKET get_socket(){return sock;}
#endif
  virtual void mark_core(void) 
  { if (send_buffer ) mark_array(send_buffer );
    if (tcpip_buffer) mark_array(tcpip_buffer);
  }
};


int AdvertiseService(SOCKET SocketHandles[], int SocketCount);
BOOL receive_thread_running=FALSE;

long threadCount;  // count of running special server or client threads, Interlocked
HANDLE hShutDown;  // event "Last special thread terminated" (used on server side only)
HANDLE keepAlive;  // event "KeepAlive watch received client response"

BOOL TcpIpPresent(void)
{
	return TRUE;
}

cTcpIpAddress * waiting_address = NULL, * waiting_address80 = NULL;  // used only for memory lead testing

int cTcpIpAddress :: ValidatePeerAddress(cdp_t cdp) // check if the IP address of the client is allowed
{ return ::ValidatePeerAddress(cdp, (unsigned char *)&acCallName.sin_addr, (int)connection_type); }
                                                    
BYTE TcpIpProtocolStart(void)
{ threadCount = 0;
	hShutDown = CreateEvent(&SA, FALSE, FALSE, NULL);   // synchro objekt pri ukoncovani app
#ifdef MY_KEEPALIVE
  keepAlive = CreateEvent(&SA, FALSE, FALSE, NULL);
#endif
	return KSE_OK;
}

SOCKET listen_sock=0, sqp_sock=0,
       * p1_listen80_sock = NULL, * p2_listen80_sock = NULL;

BYTE TcpIpProtocolStop(cdp_t cdp)
/* Can (and must) be called by each client, even if 16-bit. */
{
	if (threadCount)						// various subsets of threads may have been started
	{	ResetEvent(hShutDown);   	// prepare for waiting
		if (listen_sock)
		  { closesocket(listen_sock); listen_sock=0; }
		if (p1_listen80_sock)
		  { closesocket(*p1_listen80_sock);  p1_listen80_sock=NULL; }
		if (p2_listen80_sock)
		  { closesocket(*p2_listen80_sock);  p2_listen80_sock=NULL; }
		if (sqp_sock)
	    { closesocket(sqp_sock);    sqp_sock=0; }
	}
	else
		SetEvent(hShutDown);
#ifdef MY_KEEPALIVE
  SetEvent(keepAlive);  // releases K-A thread waiting for client response or reactivation
#endif
	WaitForSingleObject(hShutDown, INFINITE);   // ceka se na ukonceni komunikacnich threadu
	CloseHandle(hShutDown);
#ifdef MY_KEEPALIVE
  CloseHandle(keepAlive);
#endif
	return KSE_OK;
}


//********* Advertise, Listen and other server functions *********************

#ifdef MY_KEEPALIVE
DWORD WINAPI KeepAliveThreadProc(LPVOID)
// Server procedure sending K-A requests and checking client responses
{	DWORD err;
	InterlockedIncrement(&threadCount);
	while (threadCount!=1)
	{ err = WaitForSingleObject(keepAlive, 60000);
		if (err!=WAIT_TIMEOUT) break;  // shutdown, event set in ProtocolStop
		for (int i=1;  i<=max_task_index;  i++)
			if (cds[i])
				if (cds[i]->in_use==PT_SLAVE)
				{ cTcpIpAddress * pAdr = (cTcpIpAddress*)cds[i]->pRemAddr;
					if (pAdr!=NULL)
						if (pAdr->my_protocol == TCPIP)
						{	ResetEvent(keepAlive);
							Send(pAdr, 0, DT_KEEPALIVE);
							err = WaitForSingleObject(keepAlive, 60000);   // KEEP_ALIVE timeout
							if (cds[i]==NULL || pAdr!=cds[i]->pRemAddr)
								continue;
							if (err != WAIT_OBJECT_0)
								closesocket(pAdr->sock);  // keep alive was timed out
						}
				}
	}

	if (!InterlockedDecrement(&threadCount))
		SetEvent(hShutDown);			// posledeni ukonceny thread
	return 0;
}
#endif

DWORD WINAPI ListenThreadProc(LPVOID)
// Creates listen socket and listens for the connection, starts slave threads for connected clients.
// Terminates when listen_socket is externally closed.
{	SOCKADDR_IN sin;
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) 
    { dbg_err("Server cannot create listen socket");  return 1; }
  unsigned long ip = htonl(INADDR_ANY);
  if (*the_sp.server_ip_addr.val()) 
  { ip = inet_addr(the_sp.server_ip_addr.val());
    if (ip==INADDR_NONE) ip = htonl(INADDR_ANY);
  }
  SET_SOCKADDR(sin, the_sp.wb_tcpip_port.val()+1, ip);
	if (bind  (listen_sock, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{	dbg_err("Server cannot bind to own IP address");
    closesocket(listen_sock);  listen_sock=0;  return 1; 
  }
	if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)   
	{	dbg_err("Server cannot listen on the socket (IP:port may be in use)");
    closesocket(listen_sock);  listen_sock=0;  return 1; 
  }

  /* Start Windows NT advertising */
  AdvertiseService(&listen_sock, 1);

	InterlockedIncrement(&threadCount);
  wProtocol |= TCPIP_NET;
  WriteSlaveThreadCounter();

	while (true)
	{ SOCKET cli_sock;	int err;
    cTcpIpAddress *pAddress = waiting_address = new cTcpIpAddress();  // vytvor adresu
    int len = sizeof(pAddress->acCallName);
		cli_sock = accept(listen_sock, (sockaddr*)&pAddress->acCallName, &len);   // cekej na pripojeni klienta
    waiting_address = NULL;
    // Pozor: zavreni socketu prepise pAddress->acCallName na 0, nesmi to byt automaticka promenna!!! 
		if (cli_sock == INVALID_SOCKET)
		{ 
#ifndef WINS  // other thread, frozen on MSW
      char msg[50];
      display_server_info(form_message(msg, sizeof(msg), terminatingListen)); // normal when terminating the server
#endif
      err = WSAGetLastError();
      delete pAddress;
			break;
		}
   // socket for client connection created, start slave thread:
		trace_info("accept client OK");
    BOOL b=TRUE;
		err = setsockopt(cli_sock, SOL_SOCKET, SO_KEEPALIVE, (char*) &b, sizeof(BOOL));
		err = setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char*) &b, sizeof(BOOL));
    pAddress->connection_type = CONNTP_TCP;
		pAddress->sock = cli_sock;   // prirad ji socket a spust slave thread
    pAddress->store_rem_ident();
    THREAD_START(SlaveThread, 0, pAddress, NULL);  // thread handle closed inside
	}

  wProtocol &= ~TCPIP_NET;
	if (listen_sock) { closesocket(listen_sock);  listen_sock=0; }
	if (!InterlockedDecrement(&threadCount))
		SetEvent(hShutDown);			// posledeni ukonceny thread
	return 0;
}

DWORD WINAPI Listen80ThreadProc(void * arg)
// Creates listen socket and listens for the connection, starts slave threads for connected clients.
// Terminates when listen80_socket is externally closed.
{	SOCKADDR_IN sin;
  int port = (int)(size_t)arg;
	SOCKET listen80_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen80_sock == INVALID_SOCKET) 
    { dbg_err("Server cannot create HTTP listen socket");  return 1; }
  unsigned long ip = htonl(INADDR_ANY);
  if (*the_sp.server_ip_addr.val()) 
  { ip = inet_addr(the_sp.server_ip_addr.val());
    if (ip==INADDR_NONE) ip = htonl(INADDR_ANY);
  }
  SET_SOCKADDR(sin, port, ip);
	if (bind  (listen80_sock, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{	dbg_err("Server cannot bind to HTTP tunnel port");
    closesocket(listen80_sock);  return 1; 
  }
	if (listen(listen80_sock, SOMAXCONN) == SOCKET_ERROR)   
	{	dbg_err("Server cannot listen on the HTTP port");
    closesocket(listen80_sock);  return 1; 
  }

	InterlockedIncrement(&threadCount);
  if (the_sp.http_tunel.val() && port==http_tunnel_port_num) wProtocol |= TCPIP_HTTP;  // enables the service & information for the console
  if (the_sp.web_emul  .val() && port==http_web_port_num   ) wProtocol |= TCPIP_WEB;   // enables the service & information for the console
  if (!p1_listen80_sock) p1_listen80_sock=&listen80_sock;  else p2_listen80_sock=&listen80_sock;  // ptrs for external closing the socket
  WriteSlaveThreadCounter();

	while (true)
	{ SOCKET cli_sock;	int err;
    cTcpIpAddress *pAddress = waiting_address80 = new cTcpIpAddress(true);  // vytvor adresu
    int len = sizeof(pAddress->acCallName);
		cli_sock = accept(listen80_sock, (sockaddr*)&pAddress->acCallName, &len);   // cekej na pripojeni klienta
    waiting_address80 = NULL;
    // Pozor: zavreni socketu prepise pAddress->acCallName na 0, nesmi to byt automaticka promenna!!! 
		if (cli_sock == INVALID_SOCKET)
		{ 
#ifndef WINS  // other thread, frozen on MSW
      char msg[50];
      display_server_info(form_message(msg, sizeof(msg), terminatingListen)); // normal when terminating the server
#endif
      err = WSAGetLastError();
      delete pAddress;
			break;
		}
   // socket for client connection created, start slave thread:
		trace_info("accept HTTP client OK");
    BOOL b=TRUE;
		err = setsockopt(cli_sock, SOL_SOCKET, SO_KEEPALIVE, (char*) &b, sizeof(BOOL));
		err = setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char*) &b, sizeof(BOOL));
    pAddress->connection_type = port==http_tunnel_port_num ? CONNTP_TUNNEL : CONNTP_WEB;  // may be wrong when http_tunnel_port_num==http_web_port_num!
		pAddress->sock = cli_sock;   // prirad ji socket a spust slave thread
    pAddress->store_rem_ident();
    THREAD_START(SlaveThreadHTTP, 0, pAddress, NULL);  // thread handle closed inside
	}

  if (port==http_tunnel_port_num) wProtocol &= ~TCPIP_HTTP;  // information for the console
  if (port==http_web_port_num   ) wProtocol &= ~TCPIP_WEB;   // information for the console
	if (p1_listen80_sock==&listen80_sock) { closesocket(listen80_sock);  p1_listen80_sock=NULL; }  // ext ptr is NULL iff socket externally closed
	if (p2_listen80_sock==&listen80_sock) { closesocket(listen80_sock);  p2_listen80_sock=NULL; }
	if (!InterlockedDecrement(&threadCount))
		SetEvent(hShutDown);			// posledeni ukonceny thread
	return 0;
}
static DWORD SqpSendSip(SOCKADDR_IN srem)
// Send server identification packet to client to srem address
{	SOCKET sock;  SOCKADDR_IN sin;	int err;

  if (!*acServerAdvertiseName) return KSE_OK;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) return KSE_WINSOCK_ERROR;
  SET_SOCKADDR(sin,0,htonl(INADDR_ANY));
  if (bind(sock, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{	err = WSAGetLastError();
		closesocket(sock);
		return KSE_WINSOCK_ERROR;
	}

	char buf[50];  buf[0] = SQP_SERVER_NAME_CURRENT;	strcpy(buf+1, acServerAdvertiseName);
  if (the_sp.wb_tcpip_port.val()!=DEFAULT_IP_SOCKET) // append 1 and port number
    sprintf(buf+strlen(buf), "\1%u", the_sp.wb_tcpip_port.val());
	err = sendto(sock, buf, (int)strlen(buf)+1, 0, (sockaddr*)&srem, sizeof(srem));

	closesocket(sock);
	return err != SOCKET_ERROR ? KSE_OK : KSE_WINSOCK_ERROR;
}

DWORD WINAPI SqpThreadProc(LPVOID)
// Server thread, listens for asynchronous client requests
//  (BREAK, SQP_FOR_SERVER, KEEP_ALIVE answer)
{	SOCKADDR_IN sin;	int err;

	sqp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sqp_sock == INVALID_SOCKET)	return 1;
  SET_SOCKADDR(sin,the_sp.wb_tcpip_port.val(),htonl(INADDR_ANY));
	BOOL bVal = TRUE;
	err = setsockopt(sqp_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&bVal, sizeof(BOOL));
	if (bind(sqp_sock, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{	err = WSAGetLastError();
		closesocket(sqp_sock);  sqp_sock=0;  return 1;
	}

	InterlockedIncrement(&threadCount);
	while (TRUE)
	{ char buff[512];
		// cekej na pozadavek od klienta
    int len=sizeof(sin);
		err = recvfrom(sqp_sock, buff, sizeof(buff), 0, (sockaddr*)&sin, &len);
		if (err == SOCKET_ERROR || err==0)
      { err = WSAGetLastError();	break; }
		int i;  sig32 ipadr;
		switch (buff[0])
		{
			case SQP_BREAK:
				ipadr=0xffffffff;  str2int(buff+1, &ipadr);
				for (i=1;  i<=max_task_index;  i++) if (cds[i])
				 if (cds[i]->in_use==PT_SLAVE)
         { cAddress * pAdr = cds[i]->pRemAddr;
           if (pAdr!=NULL)
             if (ipadr==*(LONG*)(pAdr->GetRemIdent()/*+4*/))
               { cds[i]->break_request = wbtrue;
								 break;
							 }
         }
				 break;
#ifdef MY_KEEPALIVE
			case SQP_KEEP_ALIVE:
			 	ipadr=0xffffffff;  str2int(buff+1, &ipadr);
				for (i=1;  i<=max_task_index;  i++) if (cds[i])
					if (cds[i]->in_use==PT_SLAVE)
					{	cTcpIpAddress * pAdr = (cTcpIpAddress*)cds[i]->pRemAddr;
						if (pAdr!=NULL && pAdr->my_protocol == TCPIP)
							if (ipadr==*(LONG*)(pAdr->GetRemIdent()))
							{	SetEvent(keepAlive);
								break;
							}
					}
				break;
#endif
			case SQP_FOR_SERVER :
				if (ValidatePeerAddress(NULL, &sin.sin_addr.S_un.S_un_b.s_b1, -1)==KSE_OK) 
        { if (str2int(buff+1, &ipadr))  // specified only if not default
            sin.sin_port = htons(ipadr);
          else
        	  sin.sin_port = htons(DEFAULT_IP_SOCKET+1);
          SqpSendSip(sin);  // replay by own name
        }
				break;
		}
	}

	if (sqp_sock) { closesocket(sqp_sock);  sqp_sock=0; }
	if (!InterlockedDecrement(&threadCount))
		SetEvent(hShutDown);             // je-li to posledni thread
	return 0;
}

BYTE TcpIpAdvertiseStart(bool force_net_interface)
{	unsigned id;
  if (the_sp.net_comm.val() || force_net_interface)  // networking may have been started explicitly or because there is no interface enabled
	{ _beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE) ListenThreadProc, 0,           0, &id);
    _beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE) SqpThreadProc,    0,           0, &id);
#ifdef MY_KEEPALIVE
    Sleep(200); // threads must start, otherwise KeepAlive would immediately end
	  _beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE) KeepAliveThreadProc, 0,        0, &id);
#endif
  }
  if (the_sp.http_tunel.val() || the_sp.web_emul.val())
    _beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE) Listen80ThreadProc, 
       (void*)(size_t)(the_sp.http_tunel.val() ? http_tunnel_port_num : http_web_port_num), 0, &id);
  if (the_sp.http_tunel.val() && the_sp.web_emul.val() && http_tunnel_port_num!=http_web_port_num)
    _beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE) Listen80ThreadProc, 
       (void*)(size_t)http_web_port_num,                                                    0, &id);
	return KSE_OK;
}

BYTE TcpIpAdvertiseStop(void)
{	return KSE_OK; }

//
// ************************************* cTcpIpAddress ***********************************
//
void cTcpIpAddress::store_rem_ident(void)
{ memcpy(abRemIdent,   &acCallName.sin_addr,   4);
  memcpy(abRemIdent+4, &acCallName.sin_family, 2);
	send_buffer = new BYTE[TCPIP_BUFFSIZE];  // creates the default send buffer (reports lost memory if created in (void) constructor
}

cTcpIpAddress::cTcpIpAddress(SOCKADDR_IN *callAddr)
{ my_protocol=TCPIP;  is_http=false;
	memcpy(&acCallName, callAddr, sizeof(*callAddr));
  tcpip_buffer=NULL;
  firewall_adr=0;
  store_rem_ident();
}

cTcpIpAddress::cTcpIpAddress(bool is_httpIn)
{ my_protocol=TCPIP;  is_http=is_httpIn;
  tcpip_buffer=NULL;  send_buffer = NULL;
  firewall_adr=0;
}

cTcpIpAddress::~cTcpIpAddress()
{	if (send_buffer) delete [] send_buffer; }

cAddress * make_fw_address(uns32 srv_ip, uns32 fw_ip)
{ SOCKADDR_IN sin;
  SET_SOCKADDR(sin,0,htonl(srv_ip));
  cTcpIpAddress * adr = new cTcpIpAddress(&sin);
  adr->firewall_adr=fw_ip;
  return adr;
}

BOOL cTcpIpAddress::Send(t_fragmented_buffer * frbu, BYTE bDataType, unsigned MsgDataTotal, unsigned &rDataSent)
{ unsigned DataTotal = sizeof (sWbHeader) + sizeof(uns32) + MsgDataTotal;
  BYTE * pBuffSend;
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
  int bRetCode;  
  if (sock == -1) bRetCode = SOCKET_ERROR;
  else  // iterative sending
	{ unsigned sended = 0;
	  do
	  { bRetCode = send(sock, (char*)pBuffSend+sended, DataTotal-sended, 0);
		  if (bRetCode>0) sended = sended + bRetCode;
      else break;
	  } while (sended!=DataTotal);
  }
	rDataSent = MsgDataTotal;
	if (DataTotal>TCPIP_BUFFSIZE) delete [] pBuffSend; // delete the extended send buffer
  return bRetCode == SOCKET_ERROR ? FALSE : TRUE;
}

BOOL cTcpIpAddress::Unlink(cdp_t cdp, BOOL )
{
	if (cdp->in_use==PT_SLAVE)
  /* slave thread deinitialization, called from interf_close */
  { delete [] tcpip_buffer;    tcpip_buffer=NULL;
    CloseHandle(tcpip_event);  tcpip_event=0;
  }
	if (cdp->in_use==PT_REMOTE)
  { closesocket(sock);
    while (receive_thread_running)   // Crashes in WSACleanup on NT 4.0 if doesn't wait
      WinWait(0, 0);
    return TRUE;
  }
	return closesocket(sock) != SOCKET_ERROR;
}

void cTcpIpAddress::UnregisterReceiver()
{ }

void cTcpIpAddress::RegisterReceiver(cdp_t cdp)
{	if (cdp->in_use==PT_SLAVE)
  /* initialize slave thread (specific for protocol) */
  { tcpip_buffer = new BYTE[TCPIP_BUFFSIZE];
    tcpip_event = CreateEvent(&SA, TRUE, FALSE, NULL);
  }
}

BOOL cTcpIpAddress::ReceiveWork(cdp_t cdp, BOOL signal_it)
{ OVERLAPPED * povl = &((cTcpIpAddress*)cdp->pRemAddr)->ovl;

	{ BYTE *pbuff;						// pozice v bufferu
	  int size = 0;  DWORD len;  BOOL resok;
	  pbuff = tcpip_buffer;
	  HANDLE events[2];
    events[0] = cdp->hSlaveSem;  events[1] = tcpip_event;
	  int received = 0, who;
	  BOOL ret=TRUE;

	  memset(povl, 0, sizeof(OVERLAPPED));	povl->hEvent = events[1];
    SOCKET sock = ((cTcpIpAddress*)cdp->pRemAddr)->sock;
	  do
	  { ResetEvent(povl->hEvent);
		  if (!ReadFile((FHANDLE)sock, (char*)&size, sizeof(int)-received, &len, povl) && GetLastError()!=ERROR_IO_PENDING)
		  { who = WAIT_OBJECT_0;  //trace_info("slave down 2");
			  cdp->appl_state = SLAVESHUTDOWN;		// proved SLAVESHUTDOWN
			  return FALSE;
		  }
		  who = WaitForMultipleObjects(2, events, FALSE, INFINITE);
		  if (who == WAIT_OBJECT_0+1)     // doslo k udalosti prijmu znaku
		  { resok = GetOverlappedResult((FHANDLE)sock, povl, &len, FALSE);
			  received += len;
        cdp->wait_type=WAIT_RECEIVING;
			  if (!resok || len == 0) // on W95 sometimes returns len==0
			  { cdp->appl_state = SLAVESHUTDOWN;  //trace_info(!resok ? "slave down 2 err" : "slave down 2 len");
				  who = WAIT_OBJECT_0;
				  return FALSE;
			  }
		  }
      else // error or hSlaveSem used to stop the slave
      { if (who==WAIT_FAILED) GetLastError();
        cdp->appl_state = SLAVESHUTDOWN;  //trace_info("slave down 3");
        break;
      }
	  } while (received < sizeof(int));

	  if (who == WAIT_OBJECT_0+1)  // udalost kvuli prichozim znakum
	  {/* reallocate buffer: */
		  if (size > TCPIP_BUFFSIZE)
		  { delete [] tcpip_buffer;
			  tcpip_buffer = new BYTE[size];
		  }
     /* do receive */
		  pbuff = tcpip_buffer;  received = 0;
		  do
		  { ResetEvent(povl->hEvent);
			  if (!ReadFile((FHANDLE)sock, pbuff, size-received-sizeof(int),  &len, povl) && GetLastError()!=ERROR_IO_PENDING)
			  { who = WAIT_OBJECT_0;  //trace_info("slave down 4");
				  cdp->appl_state = SLAVESHUTDOWN;		// proved SLAVESHUTDOWN
				  break;
			  }
			  who = WaitForMultipleObjects(2, events, FALSE, INFINITE);
			  if (who == WAIT_OBJECT_0+1)     // doslo k udalosti prijmu znaku
			  {	resok = GetOverlappedResult((FHANDLE)sock, povl, &len, FALSE);
				  if (!resok || len == 0)
				  { who = WAIT_OBJECT_0;  //trace_info("slave down 5");
					  cdp->appl_state = SLAVESHUTDOWN;		// proved SLAVESHUTDOWN
					  break;
				  }
				  pbuff += len;
				  received += len;
			  }
        else 
			  { if (who == WAIT_FAILED)	WSAGetLastError();
          break;
        }
		  } while (size-sizeof(int) > received);
     /* use receive info: */
	    if (who != WAIT_OBJECT_0)
			  if (!Receive((cdnet_t*)cdp, tcpip_buffer, received))
        { SendCloseConnection(this);
          cdp->appl_state = SLAVESHUTDOWN;		// proved SLAVESHUTDOWN
          //trace_info("slave down 6");
          ret=FALSE;
        }


     /* reallocate buffer back: */
		  if (size > TCPIP_BUFFSIZE)
		  {	delete [] tcpip_buffer;
			  tcpip_buffer = new BYTE[TCPIP_BUFFSIZE];
		  }
	  }
    return ret;
  }
}

#include "httpsupp.c"

//
// ********************* Receive ****************************
// TCP/IP does not have specific receiving threads. Each thread
// receives input by calling Receive Work instead.

BOOL TcpIpReceiveStart(void)
{	return KSE_OK; }

BOOL TcpIpReceiveStop(void)
{	return KSE_OK; }

/************************** advertise service *******************************/
#include <nspapi.h>

typedef int WINAPI t_SetService(DWORD dwNameSpace, DWORD dwOperation,
  DWORD dwflags, LPSERVICE_INFO lpServiceInfo, LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
  LPDWORD lpdwStatusFlags);

GUID ServiceGuid = { 0x73bee800, 0xa843, 0x11cf, 0x8c, 0x5c,
                     0x00, 0x20, 0xaf, 0x54, 0xef, 0xe3 };
#define SERVICE_TYPE_NAME "602SQL 8.0 Server"

int AdvertiseService(SOCKET SocketHandles[], int SocketCount)
/*++
Routine Description:
    Advertises this service on all the default name spaces.
Arguments:
    SocketHandles - array of sockets that we have opened. For each socket,
        we do a getsockname() to discover the actual local address.
    SocketCount - number of sockets in SockHandles[]
Return Value:
    0 if success. !=0 otherwise.
--*/
{   SERVICE_INFO serviceInfo ;
    SERVICE_ADDRESSES *serviceAddrs;
    PSOCKADDR sockAddr;
    BYTE *addressBuffer;
    DWORD addressBufferSize ;
    int i, successCount, err;

    // Allocate some memory for the SERVICE_ADDRESSES structure
    serviceAddrs = (SERVICE_ADDRESSES *) malloc(
                        sizeof(SERVICE_ADDRESSES) +
                        (SocketCount - 1) * sizeof(SERVICE_ADDRESS)) ;
    if (!serviceAddrs) return 1;

    // Allocate some memory for the SOCKADDR addresses returned
    // by getsockname().
    addressBufferSize = SocketCount * sizeof(SOCKADDR);
    addressBuffer = (BYTE*)malloc(addressBufferSize);
    if (!addressBuffer) { free(serviceAddrs);  return 2; }

    // Setup the SERVICE_INFO structure. In this example, the only
    // interesting fields are the lpServiceType, lpServiceName and the
    // lpServiceAddress fields.
    serviceInfo.lpServiceType    = &ServiceGuid;
    serviceInfo.lpServiceName    = SERVICE_TYPE_NAME;
    serviceInfo.lpComment        = "Advertising 602SQL 8.0 Server Service";
    serviceInfo.lpLocale         = NULL;
    serviceInfo.lpMachineName    = NULL ;
    serviceInfo.dwVersion        = 1;
    serviceInfo.dwDisplayHint    = 0;
    serviceInfo.dwTime           = 0;
    serviceInfo.lpServiceAddress = serviceAddrs;
    serviceInfo.ServiceSpecificInfo.cbSize = 0 ;
    serviceInfo.ServiceSpecificInfo.pBlobData = NULL ;

    // For each socket, get its local association.
    // Adresy za sebe do addressBuffer, jejich popisy do pole serviceAddrs

    sockAddr = (PSOCKADDR)addressBuffer;

    for (i = successCount = 0;  i < SocketCount;  i++)
    {   // Call getsockname() to get the local association for the socket.
        int size = (int) addressBufferSize ;
        err = getsockname(SocketHandles[i], sockAddr, &size) ;
        if (err == SOCKET_ERROR) continue;

        // Now setup the Addressing information for this socket.
        // Only the dwAddressType, dwAddressLength and lpAddress
        // is of any interest in this example.
        serviceAddrs->Addresses[successCount].dwAddressType = sockAddr->sa_family;
        serviceAddrs->Addresses[successCount].dwAddressFlags = 0;
        serviceAddrs->Addresses[successCount].dwAddressLength = size ;
        serviceAddrs->Addresses[successCount].dwPrincipalLength = 0 ;
        serviceAddrs->Addresses[successCount].lpAddress = (LPBYTE) sockAddr;
        serviceAddrs->Addresses[successCount].lpPrincipal = NULL ;
        successCount++ ;

        // Advance pointer and adjust buffer size. Assumes that
        // the structures are aligned.
        addressBufferSize -= size ;
        sockAddr = (PSOCKADDR) ((BYTE*)sockAddr + size)  ;
    }
    serviceAddrs->dwAddressCount = successCount;

    // If we got at least one address, go ahead and advertise it.
    err = SOCKET_ERROR;
    DWORD statusFlags ;
    if (successCount)
    { HINSTANCE hWinsock=LoadLibrary("WSOCK32.dll");
      if (hWinsock)
      { t_SetService * p_SetService=(t_SetService *)GetProcAddress(hWinsock, "SetServiceA");
        if (p_SetService)  // NULL under W95
          err =  (*p_SetService)(
                     NS_DEFAULT,       // for all default name spaces
                     SERVICE_REGISTER, // we want to register (advertise)
                     0,                // no flags specified
                     &serviceInfo,     // SERVICE_INFO structure
                     NULL,             // no async support yet
                     &statusFlags) ;   // returns status flags
        FreeLibrary(hWinsock);  
      }
    }
    free (addressBuffer) ;
    free (serviceAddrs) ;
    return err;
}

void cTcpIpAddress :: GetAddressString(char * addr) // Return the address of the other peer
{ int j;  char szAddr[20];	char szPort[7];
  for (j = 0; j < 4; ++j)
  { sprintf(szAddr + 4*j, "%03i", ((BYTE *)&acCallName.sin_addr)[j]);
    szAddr[4*j+3] = j<3 ? '.' : 0;
	}
	sprintf(szPort,"%i", ntohs(acCallName.sin_port));
	strcpy(addr, szAddr);  strcat(addr, " : ");  strcat(addr, szPort);
}

