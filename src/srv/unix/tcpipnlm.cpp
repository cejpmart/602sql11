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
#include <sys/types.h>
#include <sys/socket.h>
#ifndef LINUX
#include <sys/filio.h>
#endif
#include <netinet/in.h>
#ifdef UNIX
#include <unistd.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#else
#include <process.h>
CFNC int _beginthread(void (*start_address)(void *), void *stack_bottom, unsigned stack_size, void *arglist);
#include <io.h>
#include <stropts.h>
#include <nwsemaph.h>
#endif
#include <errno.h>

#include "netapi.h"
#include "tcpip.h"
#include "nlmtexts.h"
#include "regstr.h"
#include "cnetapi.h"
#include "enumsrv.h"

enum t_conn_types { CONNTP_TCP, CONNTP_TUNNEL, CONNTP_WEB };  // values used for indexing the array of address dynars, IP_SETS_CNT

class cTcpIpAddress : public cAddress
/*************************************/
{ public:
  cTcpIpAddress(sockaddr_in *callAddr);
  cTcpIpAddress(bool is_httpIn=false); // J.D.
  virtual ~cTcpIpAddress();
  BOOL Send(t_fragmented_buffer * frbu, BYTE bDataType, unsigned MsgDataTotal, unsigned &rDataSent);
  BOOL Unlink(cdp_t cdp, BOOL fTerminateConnection);
  cAddress * Copy ()
    { return new cTcpIpAddress (&acCallName); };
  BOOL AddrEqual (cAddress *pAddr)
    { return !memcmp ( &((cTcpIpAddress *)pAddr)->acCallName.sin_addr, &acCallName.sin_addr, sizeof (acCallName.sin_addr)) &&
             ((cTcpIpAddress *)pAddr)->acCallName.sin_port==acCallName.sin_port;
		}
  void RegisterReceiver (cdp_t cdp);
  void UnregisterReceiver ();
  BOOL ReceiveWork(cdp_t cdp, BOOL signal_it);
  void SlaveReceiverStop(cdp_t cdp);
  void store_rem_ident(void);
  void GetAddressString(char * addr); // Return the address of the other peer
  int  ValidatePeerAddress(cdp_t cdp); // check if the address of the client is allowed
#ifdef LINUX
  virtual int get_sock(void) { return sock; }	     // redefined for TCP/IP
#endif
 public:
    sockaddr_in acCallName;
    int    sock;
		BYTE *   send_buffer;
    BYTE *   tcpip_buffer;
    t_conn_types connection_type;  // defined in addresses created for slaves
};
#define SOCKET_ERROR -1


BOOL TcpIpPresent(void)
{ return TRUE; }

cTcpIpAddress * waiting_address = NULL, * waiting_address80 = NULL;  // used only for memory lead testing

int cTcpIpAddress::ValidatePeerAddress(cdp_t cdp) // check if the IP address of the client is allowed
{ return ::ValidatePeerAddress(cdp, (unsigned char *)&acCallName.sin_addr, (int)connection_type); }

BYTE TcpIpProtocolStart(void)
{ return KSE_OK; }

BYTE TcpIpProtocolStop(cdp_t cdp)
{ return KSE_OK; }

/* close_thread closes given thread or by canceling it (Linux), or by closing its socket (Novell) */

static int closesock(int & sock) // safe closing a socket
{ int err=0;
  if (sock!=-1)
  { err=close(sock);
    sock=-1;
  }
  return err;
}

#if 0 // not using any more: causes the server to abort when the threads are being stopped
void listening_thread_proc_clean(void  *soc_v)
{
  int *soc=(int *)soc_v;
  closesock(*soc);
  *soc=-1;
}
#endif
//
// *************************** Advertise ********************************
//
int lsnsock=-1, lsn80sock1=-1, lsn80sock2=-1, sqpsock=-1; // global sockets

#ifdef LINUX
/**
 * make_socket_reusable - nastav socket jako znovupouzitelny
 * @socket: cislo socketu
 *
 * Pokud neni provedeno tohle, bind() nejakou dobu po spusteni odmita pripustit 
 * nove spojeni - na portu 5002 je stale jeste otevreno cosi ve stavu WAIT
 * Tohle nastaveni dovoli bind navzdory tomu.
 *
 * Vraci -1 pri chybe (a nastavi errno), 0 pokud OK.
 */
static int make_socket_reusable(int socket)
{
    int opt=1;
  return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
    (char *) &opt,
    sizeof(opt));
}
#endif

/*
 * ListenThreadProc - procedura naslouchajici TCP/IP
 *
 */

THREAD_PROC(ListenThreadProc)
{
  int cli_sock;  struct sockaddr_in sin;  int err;  int len;
  block_all_signals();

  Sleep(2000);
  lsnsock = socket(AF_INET, SOCK_STREAM, 0);
  if (lsnsock == -1){
    dbg_err("LSN: Socket failed");
    goto ex;
  }
  unsigned long ip;  ip = htonl(INADDR_ANY);
  if (*the_sp.server_ip_addr.val())
  { ip = inet_addr(the_sp.server_ip_addr.val());
    if (ip==INADDR_NONE) ip = htonl(INADDR_ANY);
  }
	sin.sin_family = AF_INET;										// af internet
	sin.sin_port	 = htons(the_sp.wb_tcpip_port+1);		// druhy naslouchaci port
	sin.sin_addr.s_addr = ip;

#ifdef LINUX
  if (-1==make_socket_reusable(lsnsock))
	 dbg_err("LSN: socket not reusabe");
  // Pri testu na RH 7.3 hlasi bind chybu pri spusteni dalsiho serveru 
  //  na stejnem portu bez ohledu na to, zda volam make_socket_reusable().
  // Hlaseni chyby je ale spravne! (Pise: "Adresa je uzivana")
#endif
	err = bind(lsnsock, (sockaddr*) &sin, sizeof(sin));
  if (err == -1)
  { dbg_err(strerror(errno));
    closesock(lsnsock);
    goto ex;
  }
	len = listen(lsnsock, 60);
  if (len == -1)
  { dbg_err(strerror(errno));
    closesock(lsnsock);
		goto ex;
	}

#if 0 
  pthread_cleanup_push(listening_thread_proc_clean, &lsnsock);
#endif
  wProtocol |= TCPIP_NET;  // information for the console 

  while (TRUE){
    cTcpIpAddress *pAddress = waiting_address = new cTcpIpAddress(false);  // vytvor adresu
    socklen_t ulen = sizeof(pAddress->acCallName);
		cli_sock = accept(lsnsock, (sockaddr*)&pAddress->acCallName, &ulen);   // cekej na pripojeni klienta
    waiting_address = NULL;
  	if (cli_sock == -1)
		{ delete pAddress;
      trace_info("LSN: Listen for connection ended");
			break;
		}
		BOOL b = TRUE;
    setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char*) &b, sizeof(BOOL));
    setsockopt(cli_sock, SOL_SOCKET, SO_KEEPALIVE, (char*) &b, sizeof(BOOL));
    pAddress->connection_type = CONNTP_TCP;
    pAddress->sock = cli_sock;	// prirad ji socket a spust slave thread
    pAddress->store_rem_ident();
#ifdef UNIX
    pthread_t slave_thread;
    if (pthread_create(&slave_thread, NULL, ::SlaveThread, (void*)pAddress))  // detached thread!
      SendSlaveInitResult(pAddress, KSE_START_THREAD);
    else pthread_detach(slave_thread);
#else
    if (_beginthread(SlaveThread, NULL, SLAVE_STACK_SIZE, (void*)pAddress)==-1) // zpusobuje, ze alokace pameti v CLIB neroste s kazdym spustenim a ukoncenim threadu
      SendSlaveInitResult(pAddress, KSE_START_THREAD);
#endif
	}

  wProtocol &= ~TCPIP_NET;  // information for the console 
#if 0 // not using any more: causes the server to abort when the threads are being stopped
  pthread_cleanup_pop(1);
#else
  closesock(lsnsock); // is safe, may already be closed
#endif
 ex:
  THREAD_RETURN;
}

void prepare_socket_on_port_80(const char * server_name)
// Called before UID changed from original (root) to the account defined for the server.
// Only root can open port 80. 
// When the socket is not necessary, it will be closed.
// Error is reported only if the socket is required by the server.
{ struct sockaddr_in sin;
  if (!GetDatabaseString(server_name, t_server_profile::http_tunnel_port.name, (char*)&http_tunnel_port_num, sizeof(http_tunnel_port_num), my_private_server)) 
    http_tunnel_port_num=80;
  if (!GetDatabaseString(server_name, t_server_profile::web_port.name,         (char*)&http_web_port_num,    sizeof(http_web_port_num), my_private_server)) 
    http_web_port_num   =80;
 // no action when not running as root: 
  if (getuid() != 0) return;
 // create the socket:
  lsn80sock1 = socket(AF_INET, SOCK_STREAM, 0);
  if (lsn80sock1 == -1) return;
 // bind it: 
  unsigned long ip;  ip = htonl(INADDR_ANY);
  // problem: The preferred IP address on the server is not known at the momment. Using the default one. 
  //if (*the_sp.server_ip_addr.val())
  //{ ip = inet_addr(the_sp.server_ip_addr.val());
  //  if (ip==INADDR_NONE) ip = htonl(INADDR_ANY);
  //}
  sin.sin_family = AF_INET;			// af internet
  sin.sin_addr.s_addr = ip;
  sin.sin_port	 = htons(http_tunnel_port_num);		// HTTP port
  if (bind(lsn80sock1, (sockaddr*) &sin, sizeof(sin)) == -1)
    closesock(lsn80sock1);  // sets lsn80sock1 back to -1
  if (http_web_port_num!=http_tunnel_port_num)
  { lsn80sock2 = socket(AF_INET, SOCK_STREAM, 0);
    if (lsn80sock2 == -1) return;
    sin.sin_port	 = htons(http_web_port_num);		// WWW port
    if (bind(lsn80sock2, (sockaddr*) &sin, sizeof(sin)) == -1)
      closesock(lsn80sock2);  // sets lsn80sock2 back to -1
  }
}

THREAD_PROC(Listen80ThreadProc)
// Started only when lsn80sock (passed by the argument) has been bound OK.
{ int port = (int)(size_t)data;
  int * p_listen80_sock = the_sp.http_tunel.val() && port==http_tunnel_port_num ? &lsn80sock1 : &lsn80sock2;
  int cli_sock;  int err;  int len;
  block_all_signals();

  Sleep(2000);

  len = listen(*p_listen80_sock, 60);
  if (len == -1)
  { dbg_err(strerror(errno));
    closesock(*p_listen80_sock);
    goto ex;
  }

#if 0 // not using any more: causes the server to abort when the threads are being stopped
  pthread_cleanup_push(listening_thread_proc_clean, p_listen80_sock);
#endif

  if (the_sp.http_tunel.val() && port==http_tunnel_port_num) wProtocol |= TCPIP_HTTP;  // enables the service & information for the console
  if (the_sp.web_emul  .val() && port==http_web_port_num   ) wProtocol |= TCPIP_WEB;   // enables the service & information for the console
  WriteSlaveThreadCounter();

  while (true)
  { cTcpIpAddress *pAddress = waiting_address80 = new cTcpIpAddress(true);  // vytvor adresu
    socklen_t ulen = sizeof(pAddress->acCallName);
		cli_sock = accept(*p_listen80_sock, (sockaddr*)&pAddress->acCallName, &ulen);   // cekej na pripojeni klienta
    waiting_address80 = NULL;
  	if (cli_sock == -1)
		{ delete pAddress;
      trace_info("LSN: Listen for connection on HTTP tunnel port ended");
			break;
		}
		BOOL b = TRUE;
    setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char*) &b, sizeof(BOOL));
    setsockopt(cli_sock, SOL_SOCKET, SO_KEEPALIVE, (char*) &b, sizeof(BOOL));
    pAddress->connection_type = port==http_tunnel_port_num ? CONNTP_TUNNEL : CONNTP_WEB;  // may be wrong when http_tunnel_port_num==http_web_port_num!
    pAddress->sock = cli_sock;	// prirad ji socket a spust slave thread
    pAddress->store_rem_ident();
#ifdef UNIX
    pthread_t slave_thread;
    if (pthread_create(&slave_thread, NULL, ::SlaveThreadHTTP, (void*)pAddress))  // detached thread!
      SendSlaveInitResult(pAddress, KSE_START_THREAD);
    else pthread_detach(slave_thread);
#else
    if (_beginthread(SlaveThreadHTTP, NULL, SLAVE_STACK_SIZE, (void*)pAddress)==-1) // zpusobuje, ze alokace pameti v CLIB neroste s kazdym spustenim a ukoncenim threadu
      SendSlaveInitResult(pAddress, KSE_START_THREAD);
#endif
	}

  if (port==http_tunnel_port_num) wProtocol &= ~TCPIP_HTTP;  // information for the console
  if (port==http_web_port_num   ) wProtocol &= ~TCPIP_WEB;   // information for the console
#if 0 // not using any more: causes the server to abort when the threads are being stopped
  pthread_cleanup_pop(1);
#else
  closesock(*p_listen80_sock); // is safe, may already be closed
#endif
 ex:
  THREAD_RETURN;
}

static DWORD SqpSendSip(sockaddr_in srem)
// Send server identification packet to client to srem address
{
  int sock;	sockaddr_in sin;	int err;	
  if (!*acServerAdvertiseName) return KSE_OK;
  trace_info("SQP: SqpSendSip was called");
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1)
  { dbg_err("SQP: SqpSendSip failed to create socket");
    return KSE_WINSOCK_ERROR;
  }
  sin.sin_family = AF_INET;
  sin.sin_port	 = 0;	// libovolny port i addr
  sin.sin_addr.s_addr = 0;
  err = bind(sock, (sockaddr*)&sin, sizeof(sin));
  if (err == -1)
  { dbg_err("SQP: SqpSendSip failed to bind");
    close(sock);
    return KSE_WINSOCK_ERROR;
  }
  // trace SQP:
   char text[80];
   const char *ip=(const char *)&srem.sin_addr.s_addr;
  // send SIP:
  char buff[50];  buff[0] = SQP_SERVER_NAME_CURRENT;	 strcpy(buff+1, acServerAdvertiseName);
  if (the_sp.wb_tcpip_port.val()!=DEFAULT_IP_SOCKET) // append 1 and port number
    sprintf(buff+strlen(buff), "\1%u", the_sp.wb_tcpip_port.val());
  err = sendto(sock, buff, strlen(buff)+1, 0, (sockaddr*)&srem, sizeof(srem));
  if (err<=0)
    dbg_err("SQP: SqpSendSip failed to send socket");
  else {
    sprintf(text, "SQP: send to: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    trace_info(text);
  }
  close(sock);
  return err == -1 ? KSE_OK : KSE_WINSOCK_ERROR;
}

THREAD_PROC(SqpThreadProc)
{	sockaddr_in sin;	int err;	char buff[512]; 	
  block_all_signals();
  Sleep(2000);
  RenameThread(GetThreadID(), "WB SQP TCP/IP");
  sqpsock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sqpsock == -1)
  { dbg_err("SQP: Failed to create sqpsock");
    goto ex;
  }

	sin.sin_family = AF_INET;
 	if (alternate_sip) sin.sin_port=htons(5001);
 	else sin.sin_port=htons(the_sp.wb_tcpip_port.val());  // advertise port
	sin.sin_addr.s_addr = 0;

	BOOL bVal;  bVal = TRUE;
	err = setsockopt(sqpsock, SOL_SOCKET, SO_REUSEADDR, (char*) &bVal, sizeof(BOOL));

	err = bind(sqpsock, (sockaddr*) &sin, sizeof(sin));
	if (err == -1)
	{ err = errno;
    closesock(sqpsock);
    dbg_err("SQP: Failed to bind sqpsock");
		goto ex;
	}

#if 0 // not using any more: causes the server to abort when the threads are being stopped
  pthread_cleanup_push(listening_thread_proc_clean, &sqpsock);
#endif

	while (TRUE)
	{// cekej na pozadavek od noveho klienta
    socklen_t len = sizeof(sin);
		err = recvfrom(sqpsock, buff, sizeof(buff), 0, (sockaddr*) &sin, &len);
		if (err == -1 || err==0)
    { trace_info("SQP: SQP ended, closing sqpsock");
			break;
    }
		int i;
		sig32 ipadr;
		switch (buff[0])
		{ case SQP_BREAK:
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
			case SQP_FOR_SERVER :
				if (ValidatePeerAddress(NULL, (unsigned char *)&sin.sin_addr, -1)==KSE_OK)
        { if (str2int(buff+1, &ipadr))  // specified only if not default
            sin.sin_port = htons(ipadr);
          else
        	  sin.sin_port = htons(DEFAULT_IP_SOCKET+1);
          SqpSendSip(sin);  // replay by own name
        }
				break;
		} // switch
	} // while

#if 0 // not using any more: causes the server to abort when the threads are being stopped
  pthread_cleanup_pop(1);
#else
  closesock(sqpsock); // is safe, may already be closed
#endif
ex:
  THREAD_RETURN;
}

static THREAD_HANDLE listen_thread=0, listen80_thread1=0, listen80_thread2=0, sqp_thread=0;  // global in order to be joined

BYTE TcpIpAdvertiseStart(bool force_net_interface)
{
  if (the_sp.net_comm.val() || force_net_interface)  // networking may have been started explicitly or because there is no interface enabled
  { if (!THREAD_START_JOINABLE(::ListenThreadProc,    15000, NULL, &listen_thread))
      { listen_thread=0;  dbg_err("LSN: Thread not started"); }
    if (!THREAD_START_JOINABLE(::SqpThreadProc,       15000, NULL, &sqp_thread))
      { sqp_thread=0;  dbg_err("SQP: Thread not started"); }
  }
 // HTTP listen thread 1:
  if (the_sp.http_tunel.val())
  { //if (getuid() != 0) -- cannot check getuid now, it may hev ben changed since port 80 was being bound
    //  dbg_err("Server will not listen on HTTP tunnel port because it is not running as root.");
    //else 
    if (lsn80sock1==-1)
      dbg_err("Server cannot bind to HTTP tunnel port (server not started as root or port may be occupied by a web server).");
    else  
      if (!THREAD_START_JOINABLE(::Listen80ThreadProc,15000, (void*)http_tunnel_port_num, &listen80_thread1))
        { listen80_thread1=0;  dbg_err("LSN80: Thread 1 not started"); }
  }
  else  // port 80 is not necessary
    closesock(lsn80sock1);    // closes port 80 if it has been opened
 // HTTP listen thread 2:
  if (the_sp.web_emul.val() && (!the_sp.http_tunel.val() || http_web_port_num!=http_tunnel_port_num))
  { if (lsn80sock2==-1)
      dbg_err("Server cannot bind to web port (server not started as root or port may be occupied by a web server).");
    else  
      if (!THREAD_START_JOINABLE(::Listen80ThreadProc,15000, (void*)http_web_port_num, &listen80_thread2))
        { listen80_thread2=0;  dbg_err("LSN80: Thread 2 not started"); }
  }
  else  // port 80 is not necessary
    closesock(lsn80sock2);    // closes port 80 if it has been opened
  return KSE_OK;
}

BYTE TcpIpAdvertiseStop(void)
{ int err;
  if (sqp_thread)
  { trace_info("SQP: Service shutdown");
    err = pthread_cancel(sqp_thread);
    if (err!=0) dbg_err("SQP: Close failed");
  }
  if (listen_thread)
  { trace_info("LSN: Service shutdown");
    err = pthread_cancel(listen_thread);
    //char buf[20]; sprintf(buf, "err=%d", err);  dbg_err(buf);
    if (err!=0) dbg_err("LSN: Close failed");
  }
  if (listen80_thread1)
  { err = pthread_cancel(listen80_thread1);
    if (err!=0) dbg_err("LSN80: Close 1 failed");
  }
  if (listen80_thread2)
  { err = pthread_cancel(listen80_thread2);
    if (err!=0) dbg_err("LSN80: Close 2 failed");
  }
 // waiting for Listen and SQP threads to end:
  if (listen_thread)
    pthread_join(listen_thread, NULL); 
  if (sqp_thread)
    pthread_join(sqp_thread,    NULL); 
  if (listen80_thread1)
    pthread_join(listen80_thread1, NULL); 
  if (listen80_thread2)
    pthread_join(listen80_thread2, NULL); 
 // close sockets left open by the cancelled threads
  closesock(lsnsock);
  closesock(lsn80sock1);
  closesock(lsn80sock2);
  closesock(sqpsock);
  return KSE_OK;
}

//
// ************************************* cTcpIpAddress ***********************************
//
#include "httpsupp.c"

void cTcpIpAddress::store_rem_ident(void)
{ memcpy(abRemIdent,   &acCallName.sin_addr,   4);
  memcpy(abRemIdent+4, &acCallName.sin_family, 2);
}

cTcpIpAddress::cTcpIpAddress(sockaddr_in *callAddr)
{ my_protocol=TCPIP;  is_http=false;
	memcpy(&acCallName, callAddr, sizeof(*callAddr));
  store_rem_ident();
	send_buffer = new BYTE[TCPIP_BUFFSIZE];  // creates the default send buffer
}

cTcpIpAddress::cTcpIpAddress(bool is_httpIn)
{ my_protocol=TCPIP;  is_http=is_httpIn;
	send_buffer = new BYTE[TCPIP_BUFFSIZE];  // creates the default send buffer
}

cTcpIpAddress::~cTcpIpAddress()
{	if (send_buffer) delete [] send_buffer; }

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
		  if (bRetCode>=0) sended = sended + bRetCode;
      else
      { //char buf[60];  sprintf(buf, "send errror %d on socket %d", errno, sock);  dbg_err(buf);
        // after the client disconnects, 1 send is called and error 9 occurs
        break;
      }
	  } while (sended!=DataTotal);
  }
	rDataSent = MsgDataTotal;
	if (DataTotal>TCPIP_BUFFSIZE) delete [] pBuffSend; // delete the extended send buffer
  return bRetCode == SOCKET_ERROR ? FALSE : TRUE;
}

#ifdef NEVER
BYTE cTcpIpAddress::Link()
{	int err;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) return KSE_CONNECTION;
	acCallName.sin_port = htons(WB_TCPIP_PORT+1);				// komunikacni port
	err = connect(sock, (sockaddr*) &acCallName, sizeof(acCallName));
	if (err == -1)
	{	err = errno;     // spojeni se nezdarilo
    close(sock);
		return KSE_CONNECTION;
	}
	BOOL b=TRUE;
	err = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*) &b, sizeof(BOOL));
	return KSE_OK;
}
#endif

BOOL cTcpIpAddress::Unlink(cdp_t cdp, BOOL )
{	if (cdp->in_use==PT_SLAVE)
  /* slave thread deinitialization, called from interf_close */
  { SendCloseConnection(this);
		delete [] tcpip_buffer;  tcpip_buffer=NULL; 
	}
  return closesock(sock) != -1;
}

void cTcpIpAddress::UnregisterReceiver(void)
{ }

void cTcpIpAddress::RegisterReceiver(cdp_t cdp)
{
	if (cdp->in_use==PT_SLAVE)
  /* initialize slave thread (specific for protocol) */
    tcpip_buffer = new BYTE[TCPIP_BUFFSIZE];
}

BOOL cTcpIpAddress::ReceiveWork(cdp_t cdp, BOOL signal_it)
{ char *pbuff;						// pozice v bufferu
	int err;
  int size = 0, received = 0;
	BOOL ret=TRUE;
 // receive int header to the "size" variable:
	do
	{ err=recv(sock, (char*)&size+received, sizeof(int)-received, 0);
		if (err<=0)
		{ cdp->appl_state = SLAVESHUTDOWN;
			trace_info("SLAVE: Disconnecting 1.");
			//char buf[40];  sprintf(buf, "recv errror 1 %d on socket %d", errno, sock);
			//trace_msg_general(cdp, TRACE_SERVER_FAILURE, buf, NOOBJECT);
			ret=FALSE;
   		return ret;
		}
    cdp->wait_type=WAIT_RECEIVING;
  	received += err;
	} while (received < (int)sizeof(int));
 // prepare receive buffer:
 	if (size > TCPIP_BUFFSIZE)
	{ delete [] tcpip_buffer;
		tcpip_buffer = new BYTE[size];
	}
 // receive the massage body:
	pbuff = (char*)tcpip_buffer;  received = 0;
	do
  { err = recv(sock, pbuff, size-sizeof(int)-received,  0);
		if (err <= 0)
		{ cdp->appl_state = SLAVESHUTDOWN;
      trace_info("SLAVE: Disconnecting 2.");
      //char buf[40];  sprintf(buf, "recv errror 2 %d on socket %d", errno, sock);
      //trace_msg_general(cdp, TRACE_SERVER_FAILURE, buf, NOOBJECT);
			break;
		}
		pbuff += err;  received += err;
	} while (size-(int)sizeof(int) > received);
 // processing the receive message:
  if (err>0)
 		if (!Receive((cdnet_t*)cdp, tcpip_buffer, received)) // FALSE for DT_CLOSE_CONN
		{
//    SendCloseConnection(this);  -- problemy s posilanim na UNIXu, protoze Windows klient uz zavrel socket, zpusobuje BROKEN PIPE
      closesock(sock);  // alternativni reakce na odpojeni ze strany klienta
 			ret=FALSE;
		}
 // return to the small buffer size:
	if (size > TCPIP_BUFFSIZE)
	{	delete [] tcpip_buffer;
	  tcpip_buffer = new BYTE[TCPIP_BUFFSIZE];
	}
	return ret;
}

void cTcpIpAddress::SlaveReceiverStop(cdp_t cdp)
// Interrupts receiving input by the slave
{ 
#ifdef UNIX
  SendCloseConnection(this);
  pthread_cancel(cdp->threadID);
#else
  closesock(sock);  //
  ThreadSwitchWithDelay();
#endif
}

// TCP/IP does not have specific receiving threads. Each thread
// receives input by calling Receive Work instead.

BOOL TcpIpReceiveStart(void)
{	return KSE_OK; }

BOOL TcpIpReceiveStop(void)
{	return KSE_OK; }

void cTcpIpAddress :: GetAddressString(char * addr) // Return the address of the other peer
{ int j;  char szAddr[20];	char szPort[7];
  for (j = 0; j < 4; ++j)
  { sprintf(szAddr + 4*j, "%03i", ((BYTE *)&acCallName.sin_addr)[j]);
    szAddr[4*j+3] = j<3 ? '.' : 0;
	}
	sprintf(szPort,"%i", ntohs(acCallName.sin_port));
	strcpy(addr, szAddr);  strcat(addr, " : ");  strcat(addr, szPort);
}

