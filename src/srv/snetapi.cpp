//
//  netapi.cpp - protocol independent network API for WinBase
//  =========================================================
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
#include "nlmtexts.h"
#include "netapi.h"
#include "tcpip.h"
#include "regstr.h"
#define USE_GETHOSTBYNAME
#define WM_SZM_BASE          WM_USER+500
#include "cnetapi.h"

#ifdef WINS
#include <winnetwk.h>
#undef PASSWORD_EXPIRED  // conflicts
#include <lm.h>
#if ALTERNATIVE_PROTOCOLS
#include "ipx.h"
#include "netbios.h"
#endif
#else // !WINS
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define _strnicmp strncasecmp
#endif

#if WBVERS<900 
#include "flstr.h"
#include "../xmlext/dad.h"
#endif

/* Notifications and network problems
Symptoms: Windows client is connected to a Linux server and starts a lengthy procedure. The network connection probably
  fails during its execution. The slave thread on the server hangs up.
Probable explanation: Executing the procedure causes frequent sending of notifications to the client. When the network 
  connection is corrupted, sending every notification takes a long period to time-out. The procedure may not end in any 
  reasonable amount of time.
Partial solution: ODBC driver switches notifications off.
*/


//#define SMP          // symmetric multiprocessing allowed on NLM
#define RDSTEP                            512 // sizeof import/export buffer

uns16 wProtocol=0;  // started protocol (single on client, a set on server)

#ifdef WINS
/* Query Timer */
#define QT_ID                   1
#define QT_TIMEOUT              1000

/* Timeout of search for named server */
// Not bound with a timer, GetTickCount cycle
#define SEARCH_SERVER_TIMEOUT         2800
#define DIAL_SEARCH_SERVER_TIMEOUT   20000
#endif 

BYTE  abKernelNetIdent[6]; // network identification of the kernel

int slaveThreadCounter=0, detachedThreadCounter=0;

/*************              Network Memory Allocator             ************/
#ifdef WINS
// Memory used for network communication must be page locked. The correct way
// is a memory obtained by GlobalAlloc in dll with GMEM_FIXED. GlobalPageLock
// is necessary only in exe.
void * AllocNetMem (sNetMem *psMem, WORD wSize)
/*********************************************/
{ psMem->hMem = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, wSize);
  if (psMem->hMem)
  { psMem->wSize = wSize;
    psMem->pMem = (BYTE *) psMem->hMem;
    return psMem->pMem;
  }
  return NULL;
}

void FreeNetMem (sNetMem *psMem)
/******************************/
{ if (!psMem->hMem) return;
  GlobalFree(psMem->hMem);
  psMem->hMem = 0;
  psMem->pMem = NULL;
  psMem->wSize = 0;
}
#endif

#ifndef UNIX
/*****************              Buffer Pool            **********************/
// A pool of network buffers used by protocol modules for sending packets.
// Provides a standard way for obtaining and returning the buffer for
// control block and data

cBufferPool :: cBufferPool(void)
/***************************/
{ pbRoot = NULL;
  wBuffNum = 0;
  wBuffOut = 0;
  InitializeCriticalSection(&critSec);
}

cBufferPool::~cBufferPool(void)
{ if (wBuffNum && !wBuffOut) Free (); 
  // better to leave the memory page locked than to release a buffer being used by a network driver
  DeleteCriticalSection(&critSec);
}

#ifdef WINS
void cBufferPool :: Free ()
{
        for(int i=0; i<wBuffNum; i++)
          FreeNetMem(&sMem[i]);
        wBuffOut = wBuffNum = 0;
}

void * cBufferPool :: Alloc (WORD wSize, WORD wNum)
/*************************************************/
{ int i;
  BYTE *pbBuff;

  wBuffOut = wBuffNum = wNum;
  for (i = 0; i < wNum; ++i)
  {
    if (!AllocNetMem (&sMem[i], wSize))
    {
        wBuffNum = i;
                        Free();
        return 0;
                }
                pbBuff = sMem[i].pMem;
        PutBuffer (pbBuff);
  }

  return sMem[0].pMem;
}
#else  // !WINS
void * cBufferPool :: Alloc (WORD wSize, WORD wNum)
/*************************************************/
{ int i;
  BYTE *pbBuff;
  pMem = (tptr)corealloc(wSize * wNum, 55);
  pbBuff = pMem;
  wBuffOut = wBuffNum = wNum;
  for (i = 0; i < wNum; ++i)
  { PutBuffer (pbBuff);
    pbBuff += wSize;
  }
  return pMem;
}
#endif

void cBufferPool :: PutBuffer(void *pvBuff)
/******************************************/
{ EnterCriticalSection(&critSec);
  *(BYTE **) pvBuff = pbRoot;
  pbRoot = (BYTE *)pvBuff;
  --wBuffOut;
  LeaveCriticalSection(&critSec);
}


// Returns ptr to the buffer or 0.
// If all buffers are being used and fWait is set,
// GetBuffer keeps waiting for a buffer.
void * cBufferPool :: GetBuffer(BOOL fWait)
/******************************************/
{ BYTE *pbBuff;
  if (!wBuffNum) return 0;
  if (!pbRoot)
  { if (!fWait) return 0;
    while (!pbRoot) WinWait (0, 0);
  }
  EnterCriticalSection(&critSec);
  pbBuff = pbRoot;
  pbRoot = *(BYTE **)pbRoot;
  ++wBuffOut;
  LeaveCriticalSection(&critSec);
  return pbBuff;
}
#endif // !UNIX

/*****************        wId to Cdp translation        *********************/


// Translation of a connection ID to the appropriet cdp.
// After a connection is established, wConnId and cdp are put to the
// translation array. After a packet is received, only wConnId is known
// by the protocol module. Such translation is faster than searching through
// the cds.


void cId2Cdp :: Register (WORD wIdIn, cdp_t cdpIn)
/****************************************************/
{ asTranslate[wNum].wId = wIdIn;
  asTranslate[wNum].cdp = cdpIn;
  ++wNum;
}


void cId2Cdp :: Unregister (WORD wIdIn)
/*************************************/
{ WORD w;
  for (w = 0; w < wNum; ++w)
    if (asTranslate[w].wId == wIdIn) break;
  if (w == wNum) return;
  asTranslate[w] = asTranslate[--wNum];
}


cdp_t cId2Cdp :: Translate (WORD wIdIn)
/******************************************/
{ WORD w;
  for (w = 0; w < wNum; ++w)
    if (asTranslate[w].wId == wIdIn) break;
  if (w == wNum) return 0;
  return asTranslate[w].cdp;
}


void Unlink (cdp_t cdp)
/*********************/
{ if (cdp->pRemAddr)
  { cdp->pRemAddr->UnregisterReceiver ();
    cdp->pRemAddr->Unlink (cdp, TRUE);
    delete cdp->pRemAddr;  cdp->pRemAddr=NULL;
  }
}


void RegisterReceiver (cdp_t cdp)
/*******************************/
{ cdp->pRemAddr->RegisterReceiver(cdp); }

//////////////////////////////////////////////// HTTP request processing ///////////////////////////////
bool http_authentificate(cdp_t cdp, char * auth_string)
{
  if (!auth_string || _strnicmp(auth_string, "Basic ", 6)) 
    return !the_sp.DisableHTTPAnonymous.val();
 // decode the credentials:
  auth_string+=6;
  while (*auth_string==' ') auth_string++; 
  t_base64 base64;   base64.reset();
  int len = base64.decode((unsigned char*)auth_string);
  if (len<=0) return false;
  auth_string[len]=0;
  int pass = 0;
  while (auth_string[pass] && auth_string[pass]!=':') pass++;
  if (!pass || pass==len) return false;  // empty username or missing ':'
  auth_string[pass]=0;
  return web_login(cdp, auth_string, auth_string+pass+1);
}

char * ok_response = "HTTP/1.1 200 OK\r\n";
char * get_body = "<html><head><title>Bad connection</title></head><body bgcolor=\"#FFFFFF\" text=\"#000000\">This is the HTTP interface of the 602SQL server, not a web server.\n</body></html>";
char * error_response = "HTTP/1.1 405 Method not allowed\r\n\r\n";
char * error_400 = "HTTP/1.1 450 Bad Request\r\n";  // 400 does not display the error page in IE, 401 does but may have other problems, both work fine in FireFox. 450 is an undefined status code and the pagu should be displayed!!
char * error_403 = "HTTP/1.1 403 Forbidden (from your IP addresss)\r\n";   // Problem: does not display the error page in IE, OK in FF and XMLFiller
char * auth_req = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"%s\"\r\n\r\n";
char * no_wxi_resp = "Web XML interface is not running on the SQl server.";

bool process_request(cdhttp_t * cdp, int method, char * body, int bodysize, char ** options)
{ bool ret = true;
  if (method==HTTP_METHOD_NONE) 
    ret = false;  // connection closed
  else if (method==HTTP_METHOD_GET) 
  { bool analysed = false;  
    if (!(wProtocol & TCPIP_WEB))  // if (!the_sp.web_emul.val()) -- must check if the service is running, the property may have been just changed
    { send_response_text(cdp->pRemAddr, ok_response, no_wxi_resp, (int)strlen(no_wxi_resp), (char*)content_type_html);
      ret=false;
    }
    else
    { if (options && options[HTTP_REQOPT_PATH]) // analyse the path, it starts by '/':
      { char * resp = NULL;  const char * content_type;  tobjname realm;  int resplen;
        int status = get_xml_response(cdp, HTTP_METHOD_GET, options, NULL, 0, &resp, &content_type, realm, resplen);
        if (status==401)
        { char buf[100+OBJ_NAME_LEN];
          sprintf(buf, auth_req, realm);
          send_response(cdp->pRemAddr, buf, NULL, 0, 0);
          analysed=true;
        }
        else if (resp)
        { if (status==200)
            send_response_text(cdp->pRemAddr, ok_response, resp, resplen, (char*)content_type);
          else
            send_response_text(cdp->pRemAddr, error_400, resp, resplen, (char*)content_type);
          analysed=true;
          corefree(resp);
        }
      }
      if (!analysed) send_response_text(cdp->pRemAddr, ok_response, get_body, (int)strlen(get_body), content_type_html);
      ret = false;  // close the connection 
    }
  }
  else if (method==HTTP_METHOD_POST && body)
  {// tunnelling supposed when the first byte is the request type, process the SQL request:
    cdp->wait_type=WAIT_NO;
    void * pData = body+1;  bodysize--;
    ret = true;
    if (TUNELLED_REQUEST(*body))
      switch (*body)
      { case DT_EXPORTACK:
          if (cdp->ImpExpDataSize == 0x80000000) break; // value of 0x80000000 signalizes write error: it must stop the cycle in client_write!
          if (*(uns32*)pData==0x80000000) cdp->ImpExpDataSize = 0x80000000;
          else cdp->ImpExpDataSize += *(uns32*)pData; 
          break;
        case DT_IMPORTACK:
          memcpy(cdp->pImpExpBuff+cdp->ImpExpDataSize, pData, bodysize);
          cdp->ImpExpDataSize += bodysize;
          //if (cdp->ImpExpDataSize >= pWbHeader->wMsgDataTotal)
          cdp->data_import_completed=true;  // signal: message completed -- in HTTP data is send in complete chunks
          break;
        case DT_VERSION:
          cdp->clientVersion = *(LONG *)pData;
          break;
        case DT_BREAK :
          cdp->break_request = wbtrue;
          break;
        case DT_REQUEST:
          if (!(wProtocol & TCPIP_HTTP))  // if (!the_sp.http_tunel.val())  -- must check if the service is running, the property may have been just changed
            { ret=false;  break; }
          if (cdp->commenc)  // decrypt:
          {   //fprintf(stderr, "decrypting 1\n");
              cdp->commenc->decrypt((unsigned char*)pData, bodysize);
              //fprintf(stderr, "decrypted\n");
          }
          new_request(cdp, (request_frame *)pData);
          if (cdp->commenc)  // encrypt
          { //fprintf(stderr, "encrypting\n");
              cdp->commenc->encrypt((unsigned char*)cdp->answer.get_ans(), cdp->answer.get_anssize());
            //fprintf(stderr, "encrypted\n");
          }
         // send the answer as the response and release it:
          cdp->wait_type=WAIT_SENDING_ANS;
          send_response(cdp->pRemAddr, ok_response, (char*)cdp->answer.get_ans(), cdp->answer.get_anssize(), DT_ANSWER);
          cdp->answer.free_answer(); // releasing the sended answer
          break;
      }
    else  // DAD import supposed
    { bool analysed = false;  
      if (!(wProtocol & TCPIP_WEB))  // if (!the_sp.web_emul.val()) -- must check if the service is running, the property may have been just changed
        ret=false;
      else
      { if (options && options[HTTP_REQOPT_PATH]) // analyse the path, it starts by '/':
        { char * resp = NULL;  const char * content_type;  tobjname realm;  int resplen;
          int status = get_xml_response(cdp, HTTP_METHOD_POST, options, body, bodysize, &resp, &content_type, realm, resplen);
          if (status==401)
          { char buf[100+OBJ_NAME_LEN];
            sprintf(buf, auth_req, realm);
            send_response(cdp->pRemAddr, buf, NULL, 0, 0);
            analysed=true;
          }
          else if (status==200)
            send_response_text(cdp->pRemAddr, ok_response, resp, resplen, (char*)content_type);
          else
            send_response_text(cdp->pRemAddr, error_400, resp, resplen, (char*)content_type);
          analysed=true;
          corefree(resp);
        }
        if (!analysed) send_response_text(cdp->pRemAddr, ok_response, get_body, (int)strlen(get_body), content_type_html);
        ret = false;  // close the connection 
      }
    }
  }
  else  // other methods return the error response
  { send_response(cdp->pRemAddr, error_response, NULL, 0, 0);
    ret = false;  // close the connection 
  }
  corefree(body);
  if (options) free_options(options);
  return ret;
}

/***********************            Send            *************************/
void cdhttp_t::send_status_nums(trecnum rec1, trecnum rec2)
{ trecnum buff[2];
  buff[0] = rec1;  buff[1] = rec2;
  send_response(pRemAddr, ok_response, (char*)buff, sizeof(buff), DT_NUMS);
}

void cdnet_t::send_status_nums(trecnum rec1, trecnum rec2)
{ trecnum buff[2];
  buff[0] = rec1;  buff[1] = rec2;
  t_message_fragment mfg = { sizeof(buff), (char*)buff };
  t_fragmented_buffer frbu(&mfg, 1);
  Send(pRemAddr, &frbu, DT_NUMS);
}

void cdhttp_t::SendServerDownWarning(void)
{ send_response(pRemAddr, ok_response, "", 1, DT_SERVERDOWN); }

void cdnet_t::SendServerDownWarning(void)
{ Send(pRemAddr, NULL, DT_SERVERDOWN); }

void cdhttp_t::SendClientMessage(const char * msg)
{ send_response(pRemAddr, ok_response, (char*)msg, (int)strlen(msg)+1, DT_MESSAGE); }

void cdnet_t::SendClientMessage(const char * msg)
{ t_message_fragment mfg = { (int)strlen(msg)+1, (char*)msg };
  t_fragmented_buffer frbu(&mfg, 1);
  Send(pRemAddr, &frbu, DT_MESSAGE);
}

bool cdnet_t::write_to_client(const void * buf, unsigned size)
{ ImpExpDataSize = 0;  // init counter of exported data
  t_message_fragment mfg = { size, (char*)buf };
  t_fragmented_buffer frbu(&mfg, 1);
  if (Send(pRemAddr, &frbu, DT_EXPORTRQ))
    while (ImpExpDataSize < size && appl_state != SLAVESHUTDOWN)
    { dispatch_kernel_control(this);
      pRemAddr->ReceiveWork(this, FALSE); /* makes the work of the receiving thread if it does not exist */
    }
  /* on error ImpExpDataSize is 0x80000000 */
  return ImpExpDataSize != size;
}

bool cdhttp_t::write_to_client(const void * buf, unsigned size)
{ ImpExpDataSize = 0;  // init counter of exported data
  send_response(pRemAddr, ok_response, (char*)buf, size, DT_EXPORTRQ);
  t_buffered_socket_reader bsr(this);  char * body;  int bodysize;  int method;
  while (ImpExpDataSize < size && appl_state != SLAVESHUTDOWN)
  { method = http_receive(this, bsr, body, bodysize);       // may allocate the [body]
    if (appl_state == SLAVESHUTDOWN)
      { corefree(body);  break; }
    if (!process_request(this, method, body, bodysize, NULL)) break;  // releases the [body]
  }
  /* on error ImpExpDataSize is 0x80000000 */
  return ImpExpDataSize != size;
}

sig32 cdnet_t::read_from_client(char * buf, uns32 size)
{ data_import_completed = false;  // set to true when complete data arrives
  pImpExpBuff = buf;
  ImpExpDataSize = 0;
  t_message_fragment mfg = { sizeof(size), (char*)&size};
  t_fragmented_buffer frbu(&mfg, 1);
  if (!Send(pRemAddr, &frbu, DT_IMPORTRQ)) return -1;
  while (!data_import_completed && appl_state != SLAVESHUTDOWN)
  { dispatch_kernel_control(this);
    pRemAddr->ReceiveWork(this, FALSE); /* makes the work of the receiving thread if it does not exist */
  }
  return ImpExpDataSize;
}

sig32 cdhttp_t::read_from_client(char * buf, uns32 size)
{ data_import_completed = false;  // set to true when complete data arrives
  pImpExpBuff = buf;
  ImpExpDataSize = 0;
  send_response(pRemAddr, ok_response, (char*)&size, sizeof(size), DT_IMPORTRQ);
  t_buffered_socket_reader bsr(this);  char * body;  int bodysize;  int method;
  while (!data_import_completed && appl_state != SLAVESHUTDOWN)
  { method = http_receive(this, bsr, body, bodysize);       // may allocate the [body]
    if (appl_state == SLAVESHUTDOWN)
      { corefree(body);  break; }
    if (!process_request(this, method, body, bodysize, NULL)) break;  // releases the [body]
  }
  return ImpExpDataSize;
}

BOOL SendCloseConnection(cAddress *pRemAddr)
{ if (pRemAddr->is_http) return FALSE;
  return Send(pRemAddr, NULL, DT_CLOSE_CON); 
}

BOOL SendSlaveInitResult(cAddress *pRemAddr, int InitRes)
// Sends 1 byte of the client connection result.
{ if (pRemAddr->is_http)
  { send_response(pRemAddr, ok_response, (char*)&InitRes, 1, DT_CONNRESULT);
    return TRUE;
  }
  else
  { t_message_fragment mfg = { sizeof(BYTE), (char*)&InitRes};
    t_fragmented_buffer frbu(&mfg, 1);
    return Send(pRemAddr, &frbu, DT_CONNRESULT);
  }
}

/****************                  Receive                 ******************/
// Copies data to cdp->msg, returns FALSE if no memory for buffer allocation.
BOOL CopyData (cdp_t cdp, BYTE *pData, unsigned DataSize, sWbHeader *pWbHeader)
/*******************************************************************************/
{ MsgInfo *pMsg = &cdp->msg;
  if (!pMsg->has_buffer())
    if (!pMsg->create_buffer(pWbHeader->wMsgDataTotal))
    { dbg_err(no_receive_memory);
      return FALSE;
    }
  pMsg->add_data(pData, DataSize);
  return TRUE;
}


// !! CopyData error handling
BOOL Receive(cdnet_t * cdp, BYTE *pBuff, unsigned Size)
/***************************************************/
{ if (!cdp) return TRUE;
  sWbHeader *pWbHeader = (sWbHeader *) pBuff;
  BYTE *pData = pBuff + sizeof (sWbHeader);
  unsigned DataSize = Size - sizeof (sWbHeader);
  MsgInfo *pMsg = &cdp->msg;

  switch (pWbHeader->bDataType)
  {/* slave */
    case DT_REQUEST:
      CopyData(cdp, pData, DataSize, pWbHeader);
      if (pMsg->has_all())
#ifdef WINS
        SetEvent(cdp->hSlaveSem);
#else
        ReleaseSemaph(&cdp->hSlaveSem, 1);
#endif
      break;
    case DT_VERSION:
      cdp->clientVersion = *(LONG *)pData;
      break;
    case DT_BREAK :
      cdp->break_request = wbtrue;
      break;
    case DT_EXPORTACK:
      if (cdp->ImpExpDataSize == 0x80000000) break; // value of 0x80000000 signalizes write error: it must stop the cycle in client_write!
      if (*(uns32*)pData==0x80000000) cdp->ImpExpDataSize = 0x80000000;
      else cdp->ImpExpDataSize += *(uns32*)pData; 
      break;
    case DT_IMPORTACK:
      memcpy (cdp->pImpExpBuff+cdp->ImpExpDataSize, pData, DataSize);
      cdp->ImpExpDataSize += DataSize;
      if (cdp->ImpExpDataSize >= pWbHeader->wMsgDataTotal)
        cdp->data_import_completed=true;  // signal: message completed
      break;
    case DT_CLOSE_CON:  // close connection
			return FALSE; 
  }
  return TRUE;
}

void StopTheSlave(cdp_t cdp)
/* can be called even for direct client - breaks it */
{ trace_info("Stopping a slave");
  cdp->appl_state = SLAVESHUTDOWN;
  cdp->break_request = wbtrue;
  cdp->cevs.cancel_event_if_waits(cdp);
#ifdef WINS
    SetEvent(cdp->hSlaveSem);
#else  /* !WINS */
    ReleaseSemaph(&cdp->hSlaveSem, 1);
    if (cdp->pRemAddr) // unless local client
      cdp->pRemAddr->SlaveReceiverStop(cdp);    // stop the slave recv. for TCP/IP
#endif
}

void StopReceiveProcess(cdp_t cdp)
/* Called when connection failed or has been closed by the other side. */
/*************************************/
{ if (!cdp) return;
  if (cdp->in_use == PT_SLAVE) /* closing slave connection and stopping it: */
  { cdp->pRemAddr->UnregisterReceiver();
    cdp->pRemAddr->Unlink(cdp, FALSE);
    // SpxTerminateConnection caused cycling in Unlink(TRUE) on NLM
    // if Unlink(FALSE) was not called.
    StopTheSlave(cdp);
  }
}


/////////////////////////////////////////////////////////////////////////////
//
//                              Start / Stop
//
/////////////////////////////////////////////////////////////////////////////

/****************                Set protocol               *****************/

BYTE ProtocolStart (void)
/***********************/
/* Returns KSE_OK if any protocol started. */
{ BYTE bCCode=KSE_OK, acode;
  wProtocol = 0;
#if ALTERNATIVE_PROTOCOLS
  if (the_sp.reqProtocol & IPXSPX)
  { acode=IpxProtocolStart();
    if (acode != KSE_OK) bCCode=acode;  else wProtocol |= IPXSPX;
  }
  if (the_sp.reqProtocol & NETBIOS)
  { acode=NetBiosProtocolStart();
    if (acode != KSE_OK) bCCode=acode;  else wProtocol |= NETBIOS;
  }
  if (the_sp.reqProtocol & TCPIP)
#endif
  { acode=TcpIpProtocolStart();
    if (acode != KSE_OK) bCCode=acode;  else wProtocol |= TCPIP;
  }
  return wProtocol ? KSE_OK : bCCode;
}


BYTE ProtocolStop (cdp_t cdp)
/**********************/
{ BYTE bRetCode=KSE_OK, acode;
#if ALTERNATIVE_PROTOCOLS
  if (wProtocol & IPXSPX)
  { acode = IpxProtocolStop (cdp);
    if (acode != KSE_OK) bRetCode=acode;
  }
  if (wProtocol & NETBIOS)
  { acode = NetBiosProtocolStop (cdp);
    if (acode != KSE_OK) bRetCode=acode;
  }
#endif
  if (wProtocol & TCPIP)
  { acode = TcpIpProtocolStop (cdp);
    if (acode != KSE_OK) bRetCode=acode;
  }
  return bRetCode;
}

#ifdef WINS
char local_computer_name[70];
typedef NET_API_STATUS __stdcall t_NetWkstaGetInfo(LPWSTR servername, DWORD level, LPBYTE *bufptr);
tImpersonation Impersonation;

BOOL tImpersonation::ConnectToNewClient(void) 
{ fPendingIO = FALSE;
  if (ConnectNamedPipe(hImpersonationPipe, &hImpersonationOverlap)) 
    return FALSE;  // error
  switch (GetLastError()) 
  { case ERROR_IO_PENDING: // The overlapped connection in progress. 
      fPendingIO = TRUE; 
      break; 
    case ERROR_PIPE_CONNECTED: // Client is already connected, so signal an event. 
      SetEvent(hImpersonationEvent); 
      break; 
    default:  // error
      return FALSE;
   } 
   return TRUE; 
} 

void tImpersonation::reset(void)
{ if (dwState==READING_STATE || dwState==WRITING_STATE) 
    DisconnectNamedPipe(hImpersonationPipe);
  dwState=DISCONNECTED_STATE;
} 

void tImpersonation::open(const char * server_name)
{ char pipe_name[40+OBJ_NAME_LEN];
  strcpy(pipe_name, "\\\\.\\pipe\\WinBaseImpersonationPipe_");  
  int i=(int)strlen(pipe_name);
  strcat(pipe_name, server_name);
  while (pipe_name[i])
  { if (pipe_name[i]=='\\' || pipe_name[i]=='/' || pipe_name[i]==':' || pipe_name[i]=='"' || pipe_name[i]=='|' || pipe_name[i]=='<' || pipe_name[i]=='>')
      pipe_name[i]='_';
    i++;
  }
  hImpersonationPipe = CreateNamedPipe(pipe_name, 
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,       
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 
      1, 100, 100, 7000, &SA);
  if (hImpersonationPipe != INVALID_HANDLE_VALUE)
  { hImpersonationEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    hImpersonationOverlap.hEvent=hImpersonationEvent;
  }
}

void tImpersonation::close(void)
{ if (hImpersonationPipe != INVALID_HANDLE_VALUE)
  { CloseHandle(hImpersonationPipe);  hImpersonationPipe=INVALID_HANDLE_VALUE; 
    CloseHandle(hImpersonationEvent);
  }
}

#endif

void IdentityVerificationStart(const char * server_name)
{
#ifdef WINS
 // open the pipe for impersonation
  Impersonation.hImpersonationPipe = INVALID_HANDLE_VALUE;
  *local_computer_name=0;
  HINSTANCE hNetApi = LoadLibrary("netapi32.dll");
  if (hNetApi)
  { t_NetWkstaGetInfo * p_NetWkstaGetInfo = (t_NetWkstaGetInfo*)GetProcAddress(hNetApi, "NetWkstaGetInfo");
    if (p_NetWkstaGetInfo)
    { WKSTA_INFO_100 wksta_info;  WKSTA_INFO_100 * ptr = &wksta_info;
      if ((*p_NetWkstaGetInfo)(NULL, 100, (LPBYTE*)&ptr) == NERR_Success)
        if (!WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ptr->wki100_computername, -1, local_computer_name, sizeof(local_computer_name), NULL, NULL))
          *local_computer_name=0;
      if (*local_computer_name) Impersonation.open(server_name);
    }
    FreeLibrary(hNetApi);
  }
#endif
}

void IdentityVerificationStop(void)
{
#ifdef WINS
  Impersonation.close();
#endif
}

tobjname acServerAdvertiseName; // name advertised by the server

int AdvertiseStart(const char * server_name, bool force_net_interface)
// Starts advertising the network access to the server
{ BYTE bRetCode;
  strmaxcpy(acServerAdvertiseName, server_name, sizeof(acServerAdvertiseName));
#if ALTERNATIVE_PROTOCOLS
  if (wProtocol & IPXSPX)
    if ((bRetCode = IpxAdvertiseStart()) != KSE_OK) return bRetCode;
  if (wProtocol & NETBIOS)
    if ((bRetCode = NetBiosAdvertiseStart()) != KSE_OK) return bRetCode;
#endif
  if (wProtocol & TCPIP)
    if ((bRetCode = TcpIpAdvertiseStart(force_net_interface)) != KSE_OK) return bRetCode;
  return KSE_OK;
}


int AdvertiseStop(void)
// Stops advertising the network access to the server
{ int bRetCode = KSE_OK;
#if ALTERNATIVE_PROTOCOLS
  if (wProtocol & IPXSPX)
    bRetCode = IpxAdvertiseStop    ();
  if (wProtocol & NETBIOS)
    bRetCode = NetBiosAdvertiseStop();
#endif
  if (wProtocol & TCPIP)
    bRetCode = TcpIpAdvertiseStop  ();
  *acServerAdvertiseName=0; // prevents advertising (adversing thread may have not been stopped yet)
  return bRetCode;
}

void ReceiveStart (void)
/**********************/
{ 
#if ALTERNATIVE_PROTOCOLS
  if (wProtocol & IPXSPX)  IpxReceiveStart ();
  if (wProtocol & NETBIOS) NetBiosReceiveStart ();
#endif
  if (wProtocol & TCPIP)   TcpIpReceiveStart ();
}


void ReceiveStop (void)
/*********************/
{ 
#if ALTERNATIVE_PROTOCOLS
  if (wProtocol & IPXSPX)  IpxReceiveStop ();
  if (wProtocol & NETBIOS) NetBiosReceiveStop ();
#endif
  if (wProtocol & TCPIP)   TcpIpReceiveStop ();
}

#if ALTERNATIVE_PROTOCOLS
static void SetProtocol(void)
/* Determines the protocol if not specified in the registry or INI. */
/********************/
{
#ifdef WINS
  if (!the_sp.reqProtocol)
  { if      (TcpIpPresent())   the_sp.reqProtocol = TCPIP;
    else if (IpxPresent())     the_sp.reqProtocol = IPXSPX;
    else if (NetBiosPresent()) the_sp.reqProtocol = NETBIOS;
  }
#else   // !WINS
  the_sp.reqProtocol = TCPIP;
#endif
}
#endif


/********************             Interface             *********************/
bool break_the_user(cdp_t cdp, int linknum)
// On errors calls request_error() and return true.
{ int errnum = 0;
  { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
    if (linknum > max_task_index) 
      errnum = ERROR_IN_FUNCTION_ARG;
    else if (!cds[linknum]) 
      return false;  // just unlinked
    else if (!cdp->prvs.is_conf_admin)  // only admin can kill
      errnum = NO_CONF_RIGHT;
    else if (linknum == cdp->applnum_perm ||  // cannot unlink myself 
             cds[linknum]->in_use==PT_SERVER || // main server thread
             cds[linknum]->in_use==PT_WORKER && !cds[linknum]->is_detached) // replication and other system worker thread
      errnum = NO_RIGHT;
    else 
    { cds[linknum]->break_request=wbtrue;
      cds[linknum]->cevs.cancel_event_if_waits(cds[linknum]);
    }
  }
  if (errnum)  // outside the CS!
    { request_error(cdp, errnum);  return true; }
  return false;
}

bool kill_the_user(cdp_t cdp, int linknum)
// On errors calls request_error() and return true.
{ int errnum = 0;
  { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
    if (linknum > max_task_index) 
      errnum = ERROR_IN_FUNCTION_ARG;
    else if (!cds[linknum]) 
      return false;  // just unlinked
    else if (!cdp->prvs.is_conf_admin)  // only admin can kill
      errnum = NO_CONF_RIGHT;
    else if (linknum == cdp->applnum_perm ||  // cannot unlink myself 
              cds[linknum]->in_use!=PT_SLAVE && cds[linknum]->in_use!=PT_KERDIR) // worked or detached thread
      errnum = NO_RIGHT;
    else 
      StopTheSlave(cds[linknum]);
  }
  if (errnum)  // outside the CS!
    { request_error(cdp, errnum);  return true; }
  for (int i=0;  i<30;  i++) 
  { if (!cds[linknum]) break;  // no need to protect this [cds] access
    Sleep(100);  // if does not wait, it can crash when slave terminates while refreshing the user list
  }
  return false;
}

void StopSlaves(void)
/********************/
{ int index;  cdp_t acdp;  char msg[50];
 // check for running slaves: 
  bool any_slave_running = false;
  { ProtectedByCriticalSection cs(&cs_client_list);
    for (index = 1;  index <= max_task_index;  index++) 
    { acdp=cds[index];
      if (acdp)
        if (acdp->in_use==PT_SLAVE || acdp->in_use==PT_KERDIR || acdp->in_use==PT_WORKER && acdp->is_detached)
          any_slave_running = true;
    }
  }
  if (!any_slave_running) return;
  
  display_server_info(form_message(msg, sizeof(msg), killingSlaves));

 // set break to clients and stop all slaves: 
  { ProtectedByCriticalSection cs(&cs_client_list);
    for (index = 1;  index <= max_task_index;  index++) 
    { acdp=cds[index];
      if (acdp)
        if (acdp->in_use==PT_SLAVE || acdp->in_use==PT_KERDIR || acdp->in_use==PT_WORKER && acdp->is_detached)
          StopTheSlave(acdp);
    }
  }
 // waiting until all slaves have terminated (max. 70 seconds):
  DWORD dwTickStart = GetTickCount();
  while (slaveThreadCounter || detachedThreadCounter)
    if (!WinWait(dwTickStart, 70000)) break;
  if (slaveThreadCounter) dump_thread_overview(NULL);
#ifndef WINS
  if (slaveThreadCounter)
    dbg_err("Problem disconnecting clients, trying to stop.");
 // delete connections and local semaphores of the unkilled slaves:
  for (index = 1;  index <= max_task_index;  index++) 
  { acdp=cds[index];
    if (acdp && acdp->in_use==PT_SLAVE)
    { CloseSemaph(&acdp->hSlaveSem);  
      kernel_cdp_free(acdp);
      cd_interf_close(acdp);
    }
  }
#endif
}


BYTE NetworkStart(void)
/***********************************/
{ 
#if ALTERNATIVE_PROTOCOLS
  SetProtocol();
#endif
#ifdef WINS
  ProtocolStart();  // server should work even if no protocol installed and /n from InstallShield used
#else
  BYTE bCCode = ProtocolStart();
  if (bCCode != KSE_OK) return bCCode;
#endif
  ReceiveStart();  // empty action for TCP/IP
  return KSE_OK;
}

BYTE NetworkStop(cdp_t cdp)
/**********************************/
{ 
#ifndef WINS
  char msg[50];
  display_server_info(form_message(msg, sizeof(msg), unloadingServer));
#endif
  fShutDown = fLocking = TRUE;  // added T.Z.
  ReceiveStop();  // empty action for TCP/IP
  ProtocolStop(cdp);
  return KSE_OK;
}

void WarnClientsAndWait(unsigned timeout)
/*********************/
{ 
  if (slaveThreadCounter)
  { int i;  char msg[50];
    display_server_info(form_message(msg, sizeof(msg), warningClients));
#ifdef LINUX
    alarm(timeout+10);
#endif
   // send warning to all clients:
    { ProtectedByCriticalSection cs(&cs_client_list);
      for (i = 1;  i <= max_task_index;  i++) if (cds[i])
        if (!cds[i]->dep_login_of)  // do not send to dependent logins
          cds[i]->SendServerDownWarning();  // no action for PT_WORKER, PT_SERVER
    }
   // wait timeout seconds for clients to disconnect:
#ifdef LINUX
    for (i = 0; i < timeout; ++i) /* waiting, then killing the remaining clients */
      if (slaveThreadCounter) Sleep(1000);
      else break;
#else
    DWORD dwTickStart = GetTickCount();
    while (slaveThreadCounter)
      if (!WinWait(dwTickStart, timeout*1000)) break;
#endif
  }
}

void MessageToClients(cdp_t cdp, const char * msg)
{ ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
  for (int i=1;  i<=max_task_index;  i++) if (cds[i]) 
    if (cds[i]!=cdp && !cds[i]->dep_login_of)  // do not send to itself nor to dependent logins
      cds[i]->SendClientMessage(msg);  // no action for PT_WORKER, PT_SERVER
}

/***********************             Slave        ***************************/

BOOL SlaveRequestHandler(cdp_t cdp)
// Processes client request delivered to the server, sends answer.
{ cdp->wait_type=WAIT_NO;  
  if (!cdp->msg.has_buffer())  // buffer for the request not allocated
    cdp->answer.ans_error(OUT_OF_KERNEL_MEMORY);
  else // request received OK
  {// decrypt:
    if (cdp->commenc)
    { //fprintf(stderr, "decrypting\n");
      cdp->commenc->decrypt(cdp->msg._set(), cdp->msg._total());
      //fprintf(stderr, "decrypted\n");
    }
   // write the request into the communication log 
    if (ffa.logging_communication())
      ffa.log_client_request(cdp);
   // process the request
    new_request(cdp, (request_frame *)cdp->msg._set());
  }
 // releasing the request buffer:
  cdp->msg.release();
 // transmit & free the asnwer:
  cdp->wait_type=WAIT_SENDING_ANS;
  t_message_fragment mfg = { cdp->answer.get_anssize(), (char*)cdp->answer.get_ans() };
  if (cdp->commenc)  // encrypt
  { //fprintf(stderr, "encrypting\n");
    cdp->commenc->encrypt((unsigned char *)mfg.contents, mfg.length);
    //fprintf(stderr, "encrypted\n");
  }
  t_fragmented_buffer frbu(&mfg, 1);
  BOOL fRet = Send(cdp->pRemAddr, &frbu, DT_ANSWER);
  cdp->answer.free_answer(); // releasing the sended answer
  return fRet;
}

#ifdef WINS

THREAD_PROC(SlaveThread)
{ int InitRes;
  cdnet_t cd;  cdp_t cdp = &cd;

  cd.pRemAddr = (cAddress*)data;
  trace_info("Slave started");
  InitRes = cd.pRemAddr->ValidatePeerAddress(cdp);
  if (InitRes==KSE_OK)
    InitRes = interf_init(cdp);
  if (InitRes != KSE_OK)
  { client_start_error(cdp, clientThreadStart, InitRes);
    SendSlaveInitResult(cd.pRemAddr, InitRes);
    kernel_cdp_free(cdp);
    Unlink(cdp);
    return InitRes;
  }
  if (!SendSlaveInitResult(cd.pRemAddr, KSE_OK))
  { dbg_err(Client_connection_failed);
    kernel_cdp_free(cdp);
    cd_interf_close(cdp);
    return KSE_OK+1;
  }
  { char buf[50+30], addr[30+1];
    cdp->pRemAddr->GetAddressString(addr);
    trace_msg_general(cdp, TRACE_LOGIN, form_message(buf, sizeof(buf), msg_connect_net, cdp->applnum_perm, addr), NOOBJECT);
  }
  cd.hSlaveSem = CreateEvent(&SA, FALSE, FALSE, NULL);
  DWORD wait_time = cd.pRemAddr->my_protocol==TCPIP ? 0 : INFINITE;
  while (cd.appl_state != SLAVESHUTDOWN)
  { cd.wait_type=WAIT_FOR_REQUEST;
    cd.pRemAddr->ReceiveWork(cdp, TRUE); 
    DWORD ww = WaitForSingleObject(cd.hSlaveSem, wait_time);
    ResetEvent(cd.hSlaveSem);
		if (cd.appl_state == SLAVESHUTDOWN) break;
    if (ww==WAIT_OBJECT_0 && !SlaveRequestHandler(cdp))
      { cd.appl_state = SLAVESHUTDOWN;  trace_info("slave down 1"); }
  }
  cd.wait_type=WAIT_TERMINATING;
  { char buf[81];
    trace_msg_general(cdp, TRACE_LOGIN, form_message(buf, sizeof(buf), msg_disconnect, cdp->applnum_perm), NOOBJECT);
  }
  if (ffa.logging_communication())
    ffa.log_client_termination(cdp);
  CloseHandle(cd.hSlaveSem);  cd.hSlaveSem = 0;
  kernel_cdp_free(cdp);
  cd_interf_close (cdp);
  trace_info("Slave terminating");
  client_terminating();
  THREAD_RETURN;
}

#else // !WINS

static void dec_stc(void *v_cdp)
{
  //fprintf(stderr, "dec_stc\n");
  cdp_t const cdp=(cdp_t)v_cdp;
  CloseSemaph(&(cdp->hSlaveSem));
  kernel_cdp_free(cdp);
  cd_interf_close(cdp);
}

THREAD_PROC(SlaveThread)
{ int InitRes;
  cdnet_t cd;  cdp_t cdp = &cd;

  cd.pRemAddr = (cAddress*)data;
  CreateSemaph(0, 1, &cd.hSlaveSem); /* errors when after SendSlaveInitResult */
  InitRes = cd.pRemAddr->ValidatePeerAddress(cdp);
  if (InitRes==KSE_OK)
    InitRes = interf_init(cdp);
  if (!SendSlaveInitResult(cd.pRemAddr, InitRes))
  { CloseSemaph(&cd.hSlaveSem);  
    dbg_err(Client_connection_failed);
    kernel_cdp_free(cdp);
    cd_interf_close (cdp);
    goto ex;
  }
  if (InitRes != KSE_OK)
  { client_start_error(cdp, clientThreadStart, InitRes);
    CloseSemaph(&cd.hSlaveSem);  
    kernel_cdp_free(cdp);
    Unlink(cdp);
    goto ex;
  }
  trace_info("Slave initialization OK.");
  { char buf[50+30], addr[30+1];
    cdp->pRemAddr->GetAddressString(addr);
    trace_msg_general(cdp, TRACE_LOGIN, form_message(buf, sizeof(buf), msg_connect_net, cdp->applnum_perm, addr), NOOBJECT);
  }
  pthread_cleanup_push(dec_stc, cdp); /* Bude volano pri chybnem ukonceni */
  while (/*!fShutDown &&*/ cd.appl_state != SLAVESHUTDOWN)  // fShutDown is activated on the beginning of dir_kernel_close but clients may be allowed to work for some 60 seconds
  {
	  BOOL request_arrived;
	  cd.wait_type=WAIT_FOR_REQUEST;
	  if (!(cd.pRemAddr->ReceiveWork(cdp, TRUE))) 
	  { cd.appl_state = SLAVESHUTDOWN;
		  break;
	  }
	  if (/*fShutDown ||*/ cd.appl_state == SLAVESHUTDOWN) break;
	  request_arrived = WaitSemaph(&cd.hSlaveSem, 0)==WAIT_OBJECT_0;
	  if (/*fShutDown ||*/ cd.appl_state == SLAVESHUTDOWN) break;
	  if (request_arrived && !SlaveRequestHandler (cdp))
		  cd.appl_state = SLAVESHUTDOWN;
  }

  cd.wait_type=WAIT_TERMINATING;
  { char buf[81];
    trace_msg_general(cdp, TRACE_LOGIN, form_message(buf, sizeof(buf), msg_disconnect, cdp->applnum_perm), NOOBJECT);
  }
  if (ffa.logging_communication())
    ffa.log_client_termination(cdp);
  pthread_cleanup_pop(1);
 ex:
  THREAD_RETURN;
}

#endif // !WINS

THREAD_PROC(SlaveThreadHTTP)
{ int InitRes;
  cdhttp_t cd;  cdp_t cdp = &cd;

  cd.pRemAddr = (cAddress*)data;
  trace_info("Slave started");
#ifdef LINUX  
  CreateSemaph(0, 1, &cd.hSlaveSem); /* errors when after SendSlaveInitResult */
#endif  
  InitRes = cd.pRemAddr->ValidatePeerAddress(cdp);
  if (InitRes==KSE_OK)
    InitRes = interf_init(cdp);

#ifdef STOP  // removed in order to make it more flexible
  if (InitRes != KSE_OK)
  { client_start_error(cdp, clientThreadStart, InitRes);
#ifdef LINUX  
    CloseSemaph(&cd.hSlaveSem);  
#endif  
    SendSlaveInitResult(cd.pRemAddr, InitRes);
    kernel_cdp_free(cdp);
    Unlink(cdp);
    THREAD_RETURN;
  }
  if (/*false &&*/ !SendSlaveInitResult(cd.pRemAddr, KSE_OK)) // ### temp. disables http tunnel!!!
  { 
#ifdef LINUX  
    CloseSemaph(&cd.hSlaveSem);  
#endif  
    dbg_err(Client_connection_failed);
    kernel_cdp_free(cdp);
    cd_interf_close(cdp);
    THREAD_RETURN;
  }
#else   // special processing of init error:
    if (InitRes != KSE_OK)
    { t_buffered_socket_reader bsr(cdp);  char * body;  int bodysize;  int method;
      char * options[HTTP_REQOPT_COUNT];
      memset(options, 0, sizeof(options));
      method = http_receive(&cd, bsr, body, bodysize, options);       // may allocate the [body] and [path]
      if (method==HTTP_METHOD_POST && body && TUNELLED_REQUEST(*body))
      { answer_cb err_ans;
        err_ans.flags=0;  err_ans.return_code=500+InitRes;
        send_response(cd.pRemAddr, ok_response, (char*)&err_ans, ANSWER_PREFIX_SIZE, DT_ANSWER);
      }
      else // web XML interface request
      { const char * resp = kse_errors[InitRes];
        send_response_text(cd.pRemAddr, InitRes==KSE_IP_FILTER ? error_403 : error_400, (char*)resp, (int)strlen(resp), (char*)content_type_html);
      }
      client_start_error(cdp, clientThreadStart, InitRes);
      free_options(options);  corefree(body);
      kernel_cdp_free(cdp);
      Unlink(cdp);
      THREAD_RETURN;
    }
#endif
  { char buf[50+30], addr[30+1];
    cdp->pRemAddr->GetAddressString(addr);
    trace_msg_general(cdp, TRACE_LOGIN, form_message(buf, sizeof(buf), msg_connect_net, cdp->applnum_perm, addr), NOOBJECT);
  }

#ifdef WINS  
  cd.hSlaveSem = CreateEvent(&SA, FALSE, FALSE, NULL);
#else  
  pthread_cleanup_push(dec_stc, cdp); /* Bude volano pri chybnem ukonceni */
#endif  
  t_buffered_socket_reader bsr(cdp);  char * body;  int bodysize;  int method;
  char * options[HTTP_REQOPT_COUNT];
  do 
  { cdp->wait_type=WAIT_FOR_REQUEST;  
    memset(options, 0, sizeof(options));
    method = http_receive(&cd, bsr, body, bodysize, options);       // may allocate the [body] and [path]
    if (cd.appl_state == SLAVESHUTDOWN)
      { free_options(options);  corefree(body);  break; }
  } while (process_request(&cd, method, body, bodysize, options));  // releases the [body] and [path]
  cd.wait_type=WAIT_TERMINATING;
  { char buf[81];
    trace_msg_general(cdp, TRACE_LOGIN, form_message(buf, sizeof(buf), msg_disconnect, cdp->applnum_perm), NOOBJECT);
  }
  if (ffa.logging_communication())
    ffa.log_client_termination(cdp);
#ifdef WINS  
  CloseHandle(cd.hSlaveSem);  cd.hSlaveSem = 0;
  kernel_cdp_free(cdp);
  cd_interf_close (cdp);
#else  
  pthread_cleanup_pop(1);
#endif  
  trace_info("Slave terminating");
#ifdef WINS  
  client_terminating();
#endif
  THREAD_RETURN;
}

////////////////////////////// WinWait //////////////////////////////////////
#ifndef WINS
DWORD GetTickCount(void)
{ return time(NULL) * 1000;}
#endif

BOOL WinWait (DWORD dwTickReference, DWORD dwTickTimeout)
/**************************************************************/
{ if (dwTickReference)
    if (GetTickDifference (GetTickCount(), dwTickReference) > dwTickTimeout)
      return FALSE;
  Sleep(50);
  return TRUE;
}

#include "cnetapi.cpp"

////////////////////////////////////////// IP address validation ///////////////////////////////
#define IP_ADDR_LEN 4
#define IP_SETS_CNT 3
SPECIF_DYNAR(t_ip_masked_dynar, t_ip_masked, 1);
t_ip_masked_dynar ip_enabled[IP_SETS_CNT], ip_disabled[IP_SETS_CNT];

class cTcpIpAddress;
extern cTcpIpAddress * waiting_address,  * waiting_address80;

void mark_ip_filter(void)
{ int i;
  for (i=0;  i<IP_SETS_CNT;  i++)
    { ip_enabled[i].mark_core();  ip_disabled[i].mark_core(); }
  if (waiting_address) mark(waiting_address);   // address space waiting in the accept
  if (waiting_address80) mark(waiting_address80);   // address space waiting in the accept
}

BOOL t_property_descr_str_ips::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
// Save to the database. If saving empty string, reload all values to memory.
{// store in the database:
  if (!t_property_descr::set_and_save(cdp, sp, owner, num, value, vallen))
    return FALSE;
 // reload values (both addesses and masks for my set and for either enabled or disabled list):
  if (!*value && !is_addr)  // reload when empty value is stored
  { int anum;  char buf[255+1];  // addresses and masks counted from 1, terminated by the empty value
    t_property_descr_str_ips * p_addr, * p_mask;
    if (set_num==0)
    { p_addr = is_ena ? &the_sp.IP_enabled_addr : &the_sp.IP_disabled_addr;
      p_mask = is_ena ? &the_sp.IP_enabled_mask : &the_sp.IP_disabled_mask;
    }
    else if (set_num==1)
    { p_addr = is_ena ? &the_sp.IP1_enabled_addr : &the_sp.IP1_disabled_addr;
      p_mask = is_ena ? &the_sp.IP1_enabled_mask : &the_sp.IP1_disabled_mask;
    }
    else
    { p_addr = is_ena ? &the_sp.IP2_enabled_addr : &the_sp.IP2_disabled_addr;
      p_mask = is_ena ? &the_sp.IP2_enabled_mask : &the_sp.IP2_disabled_mask;
    }
    if (is_ena)
      ip_enabled[set_num].clear();  
    else
      ip_disabled[set_num].clear();
    anum=1;
    while (p_addr->load_to_memory(cdp, sp, owner, anum, buf) && *buf)
        { p_addr->string_to_mem(buf, anum);  anum++; }
    anum=1;
    while (p_mask->load_to_memory(cdp, sp, owner, anum, buf) && *buf)
        { p_mask->string_to_mem(buf, anum);  anum++; }
  }
  return TRUE;
}

BOOL t_property_descr_str_ips::string_to_mem(const char * strval, uns32 num)
{ unsigned char addr[IP_ADDR_LEN];  unsigned a1, a2, a3, a4;
  if (!minimal_start)
   if (*strval && num>0)
    if (sscanf(strval, " %u.%u.%u.%u", &a1, &a2, &a3, &a4)==4)
    { t_ip_masked_dynar * dyn = is_ena ? &ip_enabled[set_num] : &ip_disabled[set_num];
      t_ip_masked * item;
      if (num-1 < dyn->count()) item = dyn->acc0(num-1);
      else
      { item = dyn->acc(num-1);
        if (!item) return FALSE;
        item->init_mask();
      }
      addr[0]=a1;  addr[1]=a2;  addr[2]=a3;  addr[3]=a4;
      if (is_addr) item->store_addr(addr);
      else         item->store_mask(addr);
    }
  return TRUE;
}

int ValidatePeerAddress(cdp_t cdp, const unsigned char * addr, int connection_type) // parameter in the network order
{ int i;
#if WBVERS<1000
  connection_type=0;  // not using the other sets of addresses
#endif
 // -1 is used for SQP - allow it when TCP or TUNNEL allowed
  if (connection_type==-1) 
  { i = ValidatePeerAddress(cdp, addr, 0);
    if (i!=KSE_OK)
      i = ValidatePeerAddress(cdp, addr, 1);
    return i;
  }
 // for non-network servers allow only local connection:
  if (!network_server_licence && ntohl(*(uns32*)addr) != 0x7f000001)
  { char buf[100];  bool found=false;
    gethostname(buf, sizeof(buf));
    hostent * he = gethostbyname(buf);
    if (he && he->h_addr_list) 
      for (int i=0;  he->h_addr_list[i];  i++) 
        if (!memcmp(he->h_addr_list[i], addr, he->h_length))
          { found=true;  break; }
    if (!found) return KSE_NO_NETWORK_LICENCE;
  }
  if (minimal_start)
    return KSE_OK;  // not verifying the address in this mode
 // check enabled IP addresses, if present (invalid empty address is posible when specified more masks than addresses):
  if (ip_enabled[connection_type].count() && !ip_enabled[connection_type].acc0(0)->is_empty_addr())
  { BOOL found=FALSE;
    for (i=0;  i<ip_enabled[connection_type].count();  i++)
      if (ip_enabled[connection_type].acc0(i)->is_masked_addr(addr)) 
        { found=TRUE;  break; }
     if (!found) goto filter_error;
  }
 // check disabled IP addresses:
  for (i=0;  i<ip_disabled[connection_type].count();  i++)
    if (ip_disabled[connection_type].acc0(i)->is_masked_addr(addr)) 
      goto filter_error;
  return KSE_OK;
 filter_error: 
  if (cdp)  // format the peer IP address for the error message in the server log
  { char ip_text[3+1+3+1+3+1+3+1];
    sprintf(ip_text, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
    SET_ERR_MSG(cdp, ip_text);
  }  
  return KSE_IP_FILTER;
}

