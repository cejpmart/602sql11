//
//  netapi.cpp - protocol independent network API for WinBase
//  =========================================================
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
#include "netapi.h"
#include "tcpip.h"
#include "regstr.h"
#include "enumsrv.h"
#if WBVERS<900
#include "kernelrc.h"
#ifndef ODBCDRV
#include "wprez.h"
#endif
#endif
#define USE_GETHOSTBYNAME

#ifdef WINS
#if ALTERNATIVE_PROTOCOLS
#include "ipx.h"
#include "netbios.h"
#endif
#include <process.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/poll.h>
#include <error.h>
#include <stdlib.h>
#endif

int wProtocol=0;  // started protocol (single on client, set on server)
//////////////////////// client protocol ////////////////////////
void client_log(char * text)
{
#ifdef LINUX
#ifdef STOP  // for debugging only
  FILE * f = fopen("/var/log/602sql8/clilog.txt", "a");
  if (f)
  { fputs(text, f);  fputc('\n', f);
    fclose(f);
  }
#endif
#endif
}

void client_log_i(int err)
{ char buf[40];
  sprintf(buf, "  error number %d", err);
  client_log(buf);
}
////////////////////////////////////////////////////////////////////////////////
#define MAX_HOSTNAME_LEN 256
bool cut_address(const char * psName, char * hostname, int * port)
// Separates the [psName] into host name or IP address and port number 
// [psName] may continue with space and server name.
{ const char *sep = strchr(psName, ':');
  if (sep)
  { char *eptr;
    size_t len=sep-psName;
    *port=strtol(sep+1, &eptr, 10);
    if (*port)  // is in the format host:port
    { if (len>MAX_HOSTNAME_LEN) return false;
      memcpy(hostname, psName, len);  hostname[len]=0; 
      return true;
    }
  }
 // does not contain ':' or ':' is not followed by a number - using the default port:
  const char * space = strchr(psName, ' ');
  size_t len = space ? space-psName : strlen(psName);
  if (len>MAX_HOSTNAME_LEN) return false;
  memcpy(hostname, psName, len);  hostname[len]=0;
  *port=DEFAULT_IP_SOCKET;
  return true;
}

/**********************         Server List      ****************************/
t_server_scan_callback * server_scan_callback = NULL;

CFNC DllKernel void WINAPI set_server_scan_callback(t_server_scan_callback * new_server_scan_callback)
{ server_scan_callback = new_server_scan_callback; }


#ifdef WINS

#if WBVERS<900
#include <commctrl.h>
static HWND hServerTreeWnd = 0;  // window containing a list of servers linked with a query for servers

void cServerAddress :: PostServerAddress(HWND hTree)
{ TV_ITEM it;
 // find server with the same name:
  HTREEITEM hItem=TreeView_GetRoot(hTree);
  while (hItem)
  { tobjname name;
    it.hItem=hItem;  it.pszText=name;  it.cchTextMax=sizeof(tobjname);
    it.mask=TVIF_TEXT | TVIF_IMAGE;
    if (TreeView_GetItem(hTree, &it))  // info obtained
      if (!my_stricmp(name, acName)) break;
    hItem=TreeView_GetNextSibling(hTree, hItem);
  }
  if (hItem)  // change icon
  { if (it.iImage==8 || it.iImage==4) // IND_SRVREMNR or IND_SRVRAS, must not change local server icon!
    { it.hItem=hItem;
      it.mask=TVIF_IMAGE | TVIF_SELECTEDIMAGE;
      it.iImage=it.iSelectedImage=10;  //IND_SRVREMR;
      TreeView_SetItem(hTree, &it);
    }
  }
  else  // insert a new server
  { TV_INSERTSTRUCT is;
		it.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
		it.pszText        = acName;
		it.iImage         = 10;  //IND_SRVREMR;
		it.iSelectedImage = it.iImage;
		it.lParam         = t_pckobjinfo(0, XCATEG_UNCONN).get_pck(); // NOOBJECT is a signal for action dialog
	  is.hParent      = TVI_ROOT;
		is.hInsertAfter = TVI_LAST;
		is.item         = it;
		HTREEITEM hServ = TreeView_InsertItem(hTree, &is);
   // falesna vetev, kvuli rozbaleni:
		is.hParent= hServ;
		TreeView_InsertItem(hTree, &is);
    InvalidateRect(hTree, NULL, TRUE);  // otherwise fictive branch is not visible
  }
}
#endif

// List of servers found in network.
// Protocol modules find a server and insert it to the list.
// Windows in the Serch Window List receive messages with server addresses.
// The list is cleared by cQuery :: Deinit


void cServerList :: Clear(void)
/**************************/
{ cServerAddress *pThis = pRoot, *pNext;
  while (pThis) { pNext = pThis->pNext;  delete pThis;  pThis = pNext; }
  pRoot = 0;
}


// Translates the server name to its address or NULL if not found in the list
cAddress * cServerList :: GetAddress(const char * psName)
/**********************************************/
{ cAddress *pAddr = 0;
  cServerAddress *pThis = pRoot;
  while (pThis)
  { if (!my_stricmp(pThis->acName, psName))
    { 
#if ALTERNATIVE_PROTOCOLS
      if (wProtocol == IPXSPX)  pAddr = pThis->pIpxAddress->Copy ();
      if (wProtocol == NETBIOS) pAddr = pThis->pNetBiosAddress->Copy ();
#endif
      if (wProtocol == TCPIP)   pAddr = pThis->pTcpIpAddress->Copy();
      break;
    }
    pThis = pThis->pNext;
  }
  return pAddr;
}

// Adds new server to the list.
// After adding a new server, the message is sent to all windows in the search
// window list.
// The function is called by a protocol module which knows only its protocol
// address, other address parameters must be zero.
void cServerList :: Bind
  (char *psName, cAddress *pIpxAddrIn, cAddress *pNetBiosAddrIn, cAddress *pTcpIpAddrIn, uns32 ip, uns16 port)
/**************************************************************/
{ cServerAddress *pSrvr;
 // update CP:
  if (server_scan_callback && pTcpIpAddrIn)
    (*server_scan_callback)(psName, ip, port);
 /* check if the address is already in the list */
  pSrvr = pRoot;
  while (pSrvr != NULL)
  { 
#if ALTERNATIVE_PROTOCOLS
    if (pIpxAddrIn)
      if (pSrvr->pIpxAddress     && pSrvr->pIpxAddress->    AddrEqual(pIpxAddrIn))
        { delete pIpxAddrIn;      return; }
    if (pNetBiosAddrIn)
      if (pSrvr->pNetBiosAddress && pSrvr->pNetBiosAddress->AddrEqual(pNetBiosAddrIn))
        { delete pNetBiosAddrIn;  return; }
#endif
    if (pTcpIpAddrIn)
      if (pSrvr->pTcpIpAddress   && pSrvr->pTcpIpAddress->  AddrEqual(pTcpIpAddrIn))
        { delete pTcpIpAddrIn;    return; }
    pSrvr = pSrvr->pNext;
  }
  
 /* finds the Srvr with equal name or adds new one and sets the address */
  pSrvr = pRoot;
  while (pSrvr)
  { if (!strcmp(pSrvr->acName, psName)) break;
    pSrvr = pSrvr->pNext;
  }
  BOOL adding=FALSE;
  if (pSrvr==NULL)
  { pSrvr = new cServerAddress(psName);
    adding=TRUE;
  }
#if ALTERNATIVE_PROTOCOLS
  if (pIpxAddrIn)     
  { if (pSrvr->pIpxAddress)     delete pSrvr->pIpxAddress;
    pSrvr->pIpxAddress     = pIpxAddrIn;
  }
  if (pNetBiosAddrIn) 
  { if (pSrvr->pNetBiosAddress) delete pSrvr->pNetBiosAddress;
    pSrvr->pNetBiosAddress = pNetBiosAddrIn;
  }
#endif
  if (pTcpIpAddrIn)
  { if (pSrvr->pTcpIpAddress)   delete pSrvr->pTcpIpAddress;
    pSrvr->pTcpIpAddress   = pTcpIpAddrIn;
  }
  if (adding)
  { pSrvr->pNext = pRoot;  pRoot = pSrvr;
#if WBVERS<900
    if (hServerTreeWnd) pSrvr->PostServerAddress(hServerTreeWnd);
#endif
  }
}


#if WBVERS<900
// Posts all servers in the list to the specified window.
void cServerList :: PostServers(HWND hWnd)
/*****************************************/
{ cServerAddress *pSrvr = pRoot;
  while (pSrvr!=NULL)
  { 
    pSrvr->PostServerAddress(hWnd);
    pSrvr = pSrvr->pNext;
  }
}
#endif

cServerList oSrchSrvrList;

#endif // WINS

#ifdef STOP  // not used any more

#include "wchar.h"
#undef PASSWORD_EXPIRED
#include "lm.h"
#include "lmuse.h"

struct TUseInfo2
{ wchar_t * ui2_local, * ui2_remote, * ui2_password;
  DWORD ui2_status, ui2_asg_type, ui2_refcount, ui2_usecount;
  wchar_t * ui2_username, * ui2_domainname;
};

typedef NET_API_STATUS WINAPI t_NetUseEnum(LPWSTR UncServerName, DWORD Level, LPBYTE *BufPtr, DWORD PreferedMaximumSize, LPDWORD EntriesRead,
  LPDWORD TotalEntries, LPDWORD ResumeHandle);
typedef NET_API_STATUS WINAPI t_NetApiBufferFree(LPVOID Buffer);
  
BOOL WhoAmI(char * login_server, char * userName, int len)
/************************************/
{ DWORD bufsize = len;
  if (WNetGetUser(login_server, userName, &bufsize) == NO_ERROR)
    return TRUE;
  bufsize = len;
  if (!stricmp(login_server, "NULL"))
    if (WNetGetUser(NULL, userName, &bufsize) == NO_ERROR)
      return TRUE;
  if (!login_server || !*login_server) return FALSE;
  int namelen=strlen(login_server);
  for (int i = 'C';  i<='Z';  i++)
  { char locname[3];  *locname=i;  locname[1]=':';  locname[2]=0;
    char remname[100];
    bufsize=sizeof(remname);
    if (WNetGetConnection(locname, remname, &bufsize) == NO_ERROR)
      if (!stricmp(remname, login_server) ||
          !strnicmp(remname+2, login_server, namelen) &&
            remname[0]=='\\' && remname[1]=='\\' && remname[2+namelen]=='\\')
    { bufsize = len;
      if (WNetGetUser(locname, userName, &bufsize) == NO_ERROR) return TRUE;
    }
  }
 // tip od Jindry Grubhofera:
  *userName=0;
  HINSTANCE hLibInst = LoadLibrary("NETAPI32.DLL");
  if (hLibInst)
  { t_NetUseEnum * pNetUseEnum = (t_NetUseEnum*)GetProcAddress(hLibInst, "NetUseEnum");
    t_NetApiBufferFree * pNetApiBufferFree = (t_NetApiBufferFree*)GetProcAddress(hLibInst, "NetApiBufferFree");
    if (pNetUseEnum && pNetApiBufferFree)
    { TUseInfo2 *BufPtr;
      DWORD EntriesRead=0, TotalEntries=0, Resume_Handle=0;
      if (!(*pNetUseEnum)(NULL,2,(unsigned char**)&BufPtr,MAX_PREFERRED_LENGTH, &EntriesRead,&TotalEntries,&Resume_Handle))
        if (BufPtr)
        { for (i=0;  i<EntriesRead;  i++)
          { char buf[100];
            WideCharToMultiByte(CP_ACP, 0, BufPtr[i].ui2_remote, -1, buf, sizeof(buf), NULL, NULL);
            if (!stricmp(buf, login_server) ||
                !strnicmp(buf+2, login_server, namelen) && buf[0]=='\\' && buf[1]=='\\' && buf[2+namelen]=='\\')
            { WideCharToMultiByte(CP_ACP, 0, BufPtr[i].ui2_username, -1, buf, sizeof(buf), NULL, NULL);
              if (strlen(buf) <= OBJ_NAME_LEN)
              { strcpy(userName, buf);  Upcase(userName);
                break;
              }
            }
          }
          (*pNetApiBufferFree)(BufPtr);
        }
    }
    FreeLibrary(hLibInst);
  }
  return *userName!=0;
}
#endif  // STOP

/************************           Query          **************************/
//#ifndef UNIX
//#ifndef SRVR

// cQuery is a virtual class allowing protocol independent calls of functions in protocol modules.

class cQuery
/**********/
{ public:
  cQuery ()
    { wInstCounter = 0; }
  void Init ();
  void Deinit ();
  void Send ();

  protected:
  virtual void ProtocolInit () = 0;
  virtual void ProtocolDeinit () = 0;
  virtual void ProtocolSend () = 0;

  private:
  WORD wInstCounter;
};

#ifdef WINS
HANDLE hTimerThread = NULL;
HANDLE hTimerStop;
DWORD WINAPI TimerThreadProc(int *);
#define QT_TIMEOUT              1000
static const int qt_timeout = QT_TIMEOUT;
#endif

void cQuery :: Init ()
/********************/
{ if (wInstCounter++) return;
  ProtocolInit ();
#ifdef WINS  
  unsigned id;
  hTimerStop   = CreateEvent(NULL, FALSE, FALSE, NULL);
  hTimerThread = (HANDLE)_beginthreadex(NULL, 10000, (LPTHREAD_START_ROUTINE)TimerThreadProc, (LPVOID*)&qt_timeout, 0, &id);
#endif  
}


void cQuery :: Deinit ()
/**********************/
{ if (--wInstCounter) return;
  ProtocolDeinit ();
#ifdef WINS  
  oSrchSrvrList.Clear ();
  SetEvent(hTimerStop);  // hTimerStop will be closed in TimerThread
  WaitForSingleObject(hTimerThread, INFINITE); // must not delete cQuery object before TimerThread stops!
  CloseHandle(hTimerThread);  hTimerThread=0;
#endif
}

void cQuery :: Send ()
/********************/
{ if (!wInstCounter) return;
  ProtocolSend ();
}

#ifdef WINS
#if ALTERNATIVE_PROTOCOLS
class cIpxQuery : public cQuery
/*****************************/
{ public:
  void ProtocolInit ()
    { IpxQueryInit (); }
  void ProtocolDeinit ()
    { IpxQueryDeinit (); }
  void ProtocolSend ()
    { IpxQueryForServer (); }
};

class cNetBiosQuery : public cQuery
/*********************************/
{ public:
  void ProtocolInit ()
    { NetBiosQueryInit (); }
  void ProtocolDeinit ()
    { NetBiosQueryDeinit (); }
  void ProtocolSend ()
    { NetBiosQueryForServer (); }
};
#endif
#endif

class cTcpIpQuery : public cQuery
/********************************/
{ public:
  void ProtocolInit()
    { TcpIpQueryInit(); }
  void ProtocolDeinit()
    { TcpIpQueryDeinit(); }
  void ProtocolSend()
    { TcpIpQueryForServer(); }
};

cQuery * pQuery;
uns8 Query_count = 0;

//#endif
//#endif // !UNIX

static char localhost[]="127.0.0.1";

/****************              Search For Server             ****************/

#ifndef UNIX

#ifdef WINS
DWORD WINAPI TimerThreadProc(int * time)
/* Client thread sending initial requests to servers */
{ 
  while (TRUE)
  {
    if (pQuery!=NULL) pQuery->Send();  /* Test pridal J.D. */
    if (WaitForSingleObject(hTimerStop, *time) != WAIT_TIMEOUT)
      break;
  }
  CloseHandle(hTimerStop);
  return 0;
}

/********************************** RAS ***************************************/
#include <ras.h>
#include <raserror.h>
// private client RAS variables:
static wbbool ras_called = wbfalse;  // can be set by GetServerAddress, cleared by NetworkStop
static HRASCONN hRasConn;
static HINSTANCE hRasLib;
typedef DWORD WINAPI t_RasDial(LPRASDIALEXTENSIONS lpRasDialExtensions,
         LPTSTR lpszPhonebook, LPRASDIALPARAMS lpRasDialParams,
         DWORD dwNotifierType, LPVOID lpvNotifier, LPHRASCONN lphRasConn);
typedef DWORD WINAPI t_RasHangUp(HRASCONN lphRasConn);
typedef DWORD WINAPI t_RasGetConnectStatus(HRASCONN lphRasConn, LPRASCONNSTATUS lprasconnstatus);
t_RasDial             * pRasDial;
t_RasHangUp           * pRasHangUp;
t_RasGetConnectStatus * pRasGetConnectStatus;

void ras_unload(void)
{ FreeLibrary(hRasLib);  hRasLib=0; }

BOOL ras_load(void)
{ hRasLib=LoadLibrary("RASAPI32.DLL");
  if (!hRasLib) return FALSE;
  pRasDial            =(t_RasDial             *)GetProcAddress(hRasLib, "RasDialA");
  pRasHangUp          =(t_RasHangUp           *)GetProcAddress(hRasLib, "RasHangUpA");
  pRasGetConnectStatus=(t_RasGetConnectStatus *)GetProcAddress(hRasLib, "RasGetConnectStatusA");
  if (*pRasDial && *pRasHangUp && *pRasGetConnectStatus) return TRUE;
  ras_unload();
  return FALSE;
}

BOOL ras_dial(const char * conn_name)
{ if (ras_called) { ras_called++;  return TRUE; }
  if (!conn_name || !*conn_name || strlen(conn_name) > RAS_MaxEntryName)
    return FALSE;
  if (!ras_load()) return FALSE;
#ifdef CLIENT_GUI     // only in WBVERS<900
  char txbuf[80];
  LoadString(hKernelLibInst, STBR_DIALING, txbuf, sizeof(txbuf));
  strcat(txbuf, conn_name);  strcat(txbuf, "...");
  Set_status_text(txbuf);
#endif
  RASDIALPARAMS rdp;
  memset(&rdp, 0, sizeof(RASDIALPARAMS));
  rdp.dwSize=sizeof(RASDIALPARAMS);
  strcpy(rdp.szEntryName, conn_name);
  hRasConn=0;  // !!
  DWORD res=(*pRasDial)(NULL, NULL, &rdp, 0, NULL, &hRasConn);
  if (res)  /* dialing error */
  { 
#ifdef CLIENT_GUI  // only in WBVERS<900
    LoadString(hKernelLibInst, STBR_DIALING_ERROR, txbuf, sizeof(txbuf));
    int2str(res, txbuf+strlen(txbuf));
    Set_status_text(txbuf);
#endif
    if (hRasConn)  /* ... but must hang up! */
    { if (!(*pRasHangUp)(hRasConn))
      { RASCONNSTATUS rcs;
        rcs.dwSize=sizeof(RASCONNSTATUS);
        while ((*pRasGetConnectStatus)(hRasConn, &rcs) != ERROR_INVALID_HANDLE)
          Sleep(0);
      }
      hRasConn=0;
    }
  }
  else  /* dialed OK */
  { RASCONNSTATUS rcs;
    rcs.dwSize=sizeof(RASCONNSTATUS);
    (*pRasGetConnectStatus)(hRasConn, &rcs);
#ifdef CLIENT_GUI
    Set_status_text(NULLSTRING);
#endif
    ras_called=wbtrue;
  }
  ras_unload();
  return (BOOL)ras_called;
}

void ras_hangup(void)
{ if (!ras_called)  return;
  if (ras_called>1) return;
  if (!ras_load()) return;
  if (!(*pRasHangUp)(hRasConn))
  { RASCONNSTATUS rcs;
    rcs.dwSize=sizeof(RASCONNSTATUS);
    while ((*pRasGetConnectStatus)(hRasConn, &rcs) != ERROR_INVALID_HANDLE)
      Sleep(0);
    hRasConn=0;
    ras_called=wbfalse;
  }
  ras_unload();
}

void get_ip_address(const char * server_name, char * ip_addr, char * conn_name, 
                    char * fw_addr, BOOL * via_fw, int * port, BOOL * via_tunnel)
{ char key[160];  HKEY hKey;  DWORD key_len;
  *ip_addr=*conn_name=*fw_addr=0;  *via_fw=FALSE;  *port=DEFAULT_IP_SOCKET;
  if (via_tunnel) *via_tunnel=FALSE;
 // find address info from the registration of the server:
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)
  { key_len=IP_ADDR_MAX+1;
    if (RegQueryValueEx(hKey, IP_str,   NULL, NULL, (BYTE*)ip_addr, &key_len)!=ERROR_SUCCESS)
      *ip_addr=0;
    key_len=sizeof(int);
    if (RegQueryValueEx(hKey, IP_socket_str, NULL, NULL, (BYTE*)port, &key_len)!=ERROR_SUCCESS)
      *port=DEFAULT_IP_SOCKET;
    key_len=RAS_MaxEntryName+1;
    if (RegQueryValueEx(hKey, Conn_str, NULL, NULL, (BYTE*)conn_name, &key_len)!=ERROR_SUCCESS)
      *conn_name=0;
    if (via_tunnel)
    { key_len=sizeof(BOOL);
      if (RegQueryValueEx(hKey, useTunnel_str, NULL, NULL, (BYTE*)via_tunnel, &key_len)!=ERROR_SUCCESS)
        *via_tunnel=FALSE;
    }
    key_len=sizeof(BOOL);
    if (RegQueryValueEx(hKey, viaFW_str,NULL, NULL, (BYTE*)via_fw, &key_len)!=ERROR_SUCCESS)
      *via_fw=FALSE;
    if (via_fw)
    { key_len=IP_ADDR_MAX+1;
      if (RegQueryValueEx(hKey, FWIP_str, NULL, NULL, (BYTE*)fw_addr, &key_len)!=ERROR_SUCCESS)
        *fw_addr=0;
    }
    RegCloseKey(hKey);
#if WBVERS>=1000  // prevent the network search for server which if registered without the IP addres
    if (!*ip_addr)
      strcpy(ip_addr, "#");
#endif
  }
 // find global FW address if connecting via FW and local FW address not specified:
  if (via_fw && !*fw_addr)
  { strcpy(key, WB_inst_key);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)
    { key_len=IP_ADDR_MAX+1;
      if (RegQueryValueEx(hKey, FWIP_str, NULL, NULL, (BYTE*)fw_addr, &key_len)!=ERROR_SUCCESS)
        *fw_addr=0;
      RegCloseKey(hKey);
    }
  }
}

uns32 ip_str2num(char * ip_addr)
{	DWORD ip = inet_addr(ip_addr);
#ifdef USE_GETHOSTBYNAME
  if (ip==INADDR_NONE)  /* not converted */
  { struct hostent * hep = gethostbyname(ip_addr);
    if (hep==NULL) WSAGetLastError();
	else
      if (hep->h_addr_list!=NULL && hep->h_addr_list[0]!=NULL)
        ip=*(uns32*)hep->h_addr_list[0];
  }
#endif // lze vytacet->vytaci. Nelze vytacet nebo cancel vytaceni: vrati NULL
  return ntohl(ip);
//  unsigned a1, a2, a3, a4;
//  if (sscanf(ip_addr, " %u.%u.%u.%u", &a1, &a2, &a3, &a4) != 4) return 0;
//  return a4+256*(a3+256*(a2+256*a1));
}
#endif  // WINS


#define SEARCH_SERVER_TIMEOUT         4000  // Timeout of search for named server (Not bound with a timer, GetTickCount cycle) -- made longer because the client may have started the server
#define DIAL_SEARCH_SERVER_TIMEOUT   20000

// Server address is defined only if KSE_OK returned.
// Query is periodical (QT_TIMEOUT), timer is started by cQuery :: Init.
// If psName defined, search for server with this name is performed.
// Otherwise the dlgbox is opened and its listbox filled with the names of
// all servers found in the network.
int GetServerAddress(const char * psName, cAddress **ppAddr)
/***********************************************************************************/
{ DWORD timeout = SEARCH_SERVER_TIMEOUT;

  char ip_addr[IP_ADDR_MAX+1], fw_addr[IP_ADDR_MAX+1], conn_name[RAS_MaxEntryName+1];  BOOL via_fw, via_tunnel;  
  int port;  char hostname[MAX_HOSTNAME_LEN+1];  bool can_cut;
  get_ip_address(psName, ip_addr, conn_name, fw_addr, &via_fw, &port, &via_tunnel);
 // dial if dial-up connection
  if (*conn_name)
  { timeout = DIAL_SEARCH_SERVER_TIMEOUT;
    ras_dial(conn_name);
  }
 // check fw:
  if (wProtocol == TCPIP && via_fw)
  { uns32 srv_ip = ip_str2num(ip_addr);
    uns32 fw_ip  = ip_str2num(fw_addr);
	  if (fw_ip==INADDR_NONE) return KSE_FWNOTFOUND;
    if (srv_ip!=INADDR_NONE)
    { if (ppAddr!=NULL) *ppAddr = make_fw_address(srv_ip, port, fw_ip);
      return KSE_OK;
    }
  }
 // direct access if IP address is known:
  if (wProtocol == TCPIP)
  { 
#if WBVERS>=1000  // prevent the network search for server which if registered without the IP addres
    if (!strcmp(ip_addr, "#"))   // must link to localhost!
      return CreateTcpIpAddress(localhost, port, ppAddr, false)  ? KSE_OK : KSE_NOSERVER; 
#endif
    if (*ip_addr)  // if the IP address is specified, it must be valid, not looking elsewhere
      return CreateTcpIpAddress(ip_addr, port, ppAddr, via_tunnel!=0)  ? KSE_OK : KSE_NOSERVER; 
   // check if psName is the host address:
    int port;
    can_cut = cut_address(psName, hostname, &port);
    if (can_cut)
      if (inet_addr(hostname)!=INADDR_NONE)  // avoiding DNS search at this point
        return CreateTcpIpAddress(hostname, port, ppAddr, false) ? KSE_OK : KSE_NOSERVER;
  }
 // start searching servers:
  if (!Query_count)
  { if      (wProtocol == TCPIP)   pQuery = new cTcpIpQuery;
#if ALTERNATIVE_PROTOCOLS
    else if (wProtocol == NETBIOS) pQuery = new cNetBiosQuery;
    else if (wProtocol == IPXSPX)  pQuery = new cIpxQuery;
#endif
    else return KSE_NETWORK_INIT;
    Query_count=1;
    pQuery->Init();
  }
  else Query_count++;

 // scan server responeses:
  DWORD dwTickStart = GetTickCount();
  bool found_by_broadcast = false;
  do
  {// check the list of servers:
    cAddress * pAddr = oSrchSrvrList.GetAddress(psName);
    if (pAddr!=NULL)
    { if (ppAddr) *ppAddr=pAddr;
      found_by_broadcast=true;
      break;
    }
   // check time, sleep:
    if (GetTickDifference(GetTickCount(), dwTickStart) > timeout) break;
    Sleep(200);
  } while (TRUE);

 // close query:
  if (!--Query_count)
    { pQuery->Deinit();  delete pQuery;  pQuery=NULL; }
  if (found_by_broadcast) return KSE_OK;

 // trying the DNS translation of psName (without possible port number on its end):   -- disabled
  if (can_cut && CreateTcpIpAddress(hostname, port, ppAddr, false)) return KSE_OK;
 // SQL server not found in any way:
  return KSE_NOSERVER;

}

#endif // !UNIX

#ifdef WINS
#if WBVERS<900
#ifdef CLIENT_GUI

static BOOL Browsing_servers = FALSE;

CFNC DllKernel BOOL WINAPI BrowseStart(HWND hServTree)
{ if (Query_count || Browsing_servers) return FALSE;
  //return FALSE;  // temp.!
  NetworkStart(get_client_protocol(NULL));
  if      (wProtocol == IPXSPX)  pQuery = new cIpxQuery;
  else if (wProtocol == NETBIOS) pQuery = new cNetBiosQuery;
  else if (wProtocol == TCPIP)   pQuery = new cTcpIpQuery;
  else { NetworkStop(cds[0]);  status_text(BROWSING_NO);  return FALSE; }
  Browsing_servers=TRUE;  Query_count=1;
  pQuery->Init();
  oSrchSrvrList.PostServers(hServTree);  // servers already in the list will be posted to the hWnd
  hServerTreeWnd = hServTree;       // new servers will be posted to the window
  if      (wProtocol == IPXSPX)  status_text(BROWSING_IPXSPX);
  else if (wProtocol == NETBIOS) status_text(BROWSING_NETBIOS);
  else if (wProtocol == TCPIP)   status_text(BROWSING_TCPIP);
  return TRUE;
}

CFNC DllKernel void WINAPI BrowseStop(HWND hServTree)
{ if (!Browsing_servers) return;
  pQuery->Deinit();  delete pQuery;  pQuery=NULL;  Query_count=0;
  hServerTreeWnd=0;
  NetworkStop(cds[0]);
  Browsing_servers=FALSE;
  //status_text("Browsing stopped.");
}

#endif // CLIENT_GUI
#else  // version >= 9.0

static bool Browsing_servers = false;

CFNC DllKernel BOOL WINAPI BrowseStart(HWND hServTree)
// Network is supposed to be started.
{ if (Browsing_servers) return FALSE;
  if (!Query_count)
  { pQuery = new cTcpIpQuery;
    Query_count=1;
    pQuery->Init();
  }
  Browsing_servers=true;  
  return TRUE;
}

CFNC DllKernel void WINAPI BrowseStop(HWND hServTree)
{ if (!Browsing_servers || !Query_count) return;
  if (!--Query_count)
    { pQuery->Deinit();  delete pQuery;  pQuery=NULL; }
  Browsing_servers=false;
}

#endif // version >= 9.0
#else  // !WINS

CFNC DllKernel BOOL WINAPI BrowseStart(void * hServTree) { return TRUE; }
CFNC DllKernel void WINAPI BrowseStop(void * hServTree)  { }
#endif // !WINS

#ifdef UNIX
struct name_and_pointer {
	const char *name;
	cAddress ** addr;
};

static int callback_createTcpIp(const char *name, struct sockaddr_in* addr, void *v_data)
{
    struct name_and_pointer *data=(struct name_and_pointer*) v_data;
    if (strcasecmp(name, data->name)) return 1; /* není to ono */
    if (!CreateTcpIpAddress(inet_ntoa(addr->sin_addr), ntohs(addr->sin_port)-1, data->addr, false)) 
      *(data->addr)=NULL;
    return 0; /* done */
}

/**
 * Finding the SQL server:
 * 1. NULL, "" - localserver:DEFAULT
 * 2. IPPort from /etc/602sql, IP Address or hostname too (or localhost if not specified)
 * 3. IP adresa nebo IP adresa:port nebo port
 * 4. TCP/IP broadcast (muze chvilku trvat)
 * 5. hostname nebo hostname:port (muze trvat - dns resolving)
 */
int GetServerAddress(const char * psName, cAddress **ppAddr)
// Creates the IP address of SQL server.
{// empty name refers to the local server and default port:
  if (!psName || !*psName)  /* prazdne jmeno odkazuje na lokalni server a default port */
    return CreateTcpIpAddress(localhost, DEFAULT_IP_SOCKET, ppAddr, false) ? KSE_OK : KSE_NOSERVER;
 // Searching the registration of the server in /etc/602sql:
  char hostname[MAX_HOSTNAME_LEN+1];
  int port   =GetPrivateProfileInt   (psName, "IPPort",     -1,                                      configfile);
  int is_http=GetPrivateProfileInt   (psName, useTunnel_str, 0,                                      configfile);
              GetPrivateProfileString(psName, "IP Address",  NULLSTRING, hostname, sizeof(hostname), configfile);
  bool is_registered;
  if (port!=-1 || *hostname)  // server is registered
    is_registered=true;
  else
  {	char path[MAX_PATH];
    *path=0;
    GetPrivateProfileString(psName, "PATH", NULLSTRING, path, sizeof(path), configfile);
    if (!*path)
	  GetPrivateProfileString(psName, "PATH1", NULLSTRING, path, sizeof(path), configfile);
    is_registered = *path!=0;
  }
 // if the server is registered, return the address:port 
  if (is_registered)  // server is registered
	  return CreateTcpIpAddress(*hostname ? hostname : localhost, port!=-1 ? port : DEFAULT_IP_SOCKET, ppAddr, is_http) 
      ? KSE_OK : KSE_NOSERVER;
 // is psName just IP or IP:port? 
  bool can_cut = cut_address(psName, hostname, &port);
  struct in_addr adr;
  if (can_cut && 0!=inet_aton(hostname, &adr))  // avoiding DNS search at this point
    return CreateTcpIpAddress(hostname, port, ppAddr, false) ?  KSE_OK : KSE_NOSERVER;
 // broadcasting:
  name_and_pointer data={psName, ppAddr};
  *ppAddr=NULL;
  enum_servers(10, callback_createTcpIp, &data);
  if (ppAddr) return KSE_OK;
 // trying the DNS translation of psName (without possible port number on its end):
  if (can_cut && CreateTcpIpAddress(hostname, port, ppAddr, false)) return KSE_OK;
 // SQL server not found in any way:
  return KSE_NOSERVER;
}
#endif
 /////////////////////////////////////////////////////////////////////////////
//
//                              Connection
//
/////////////////////////////////////////////////////////////////////////////

/************************         Link/Unlink        ************************/

// Link causes server to start the slave process, which must call initialisation
// functions. After that the slave sends back the initialisation result
// (SendSlaveInitResult). Client is waiting for this information (including
// the login key).
// LOWORD (cdp->auxinfo) ... flag of slave init reslut received
// HIWORD (cdp->auxinfo) ... result of slave inititalisation
BYTE Link(cdp_t cdp)
{ cdp->auxinfo = 0L;
#ifdef LINUX
  signal(SIGPIPE, SIG_IGN); // kdyz prisel signal pipe, neco se nepodarilo odeslat do socketu nic s tim delat nebudu (deje se u SendCloseConnection po rozpadu nebo nenavazani spojeni)
#endif
  int bRetCode = cdp->pRemAddr->Link(cdp);
  if (bRetCode!=KSE_OK)
  { delete cdp->pRemAddr;  cdp->pRemAddr=NULL;
    return bRetCode;
  }
  cdp->pRemAddr->GetStatus(); /* added J.D. for debugging */
 // get slave initialization result from the server:
  if (!cdp->pRemAddr->is_http)
  { DWORD dwTickStart = GetTickCount();
#ifdef SINGLE_THREAD_CLIENT
    while (!cdp->auxinfo)
      if (!cdp->pRemAddr->ReceiveWork(cdp, 50000)) break;
#else
    while (!cdp->auxinfo)
      if (!WinWait (dwTickStart, 50000)) break;
#endif
    if (!cdp->auxinfo) // timeout
      { Unlink(cdp);  client_log("Slave status not received");  return KSE_CONNECTION; }
    bRetCode = LOBYTE(HIWORD(cdp->auxinfo));
    if (bRetCode != KSE_OK)
      { Unlink(cdp);  return bRetCode; }
  }
 // send client version to server:
  cdp->pRemAddr->process_broken_link=TRUE;  // allows normal processing of connection errors 
  if (!SendVersion(cdp->pRemAddr, MAKELONG(WB_VERSION_MINOR,WB_VERSION_MAJOR)))
    client_log("Client version not sended in Link");
  return KSE_OK;
}

void Unlink(cdp_t cdp)
/*********************/
{ if (cdp->pRemAddr)
  { cdp->pRemAddr->process_broken_link=FALSE;  // disables the feedback of breaking the connection 
    cdp->pRemAddr->Unlink(cdp, TRUE);
    delete cdp->pRemAddr;  cdp->pRemAddr=NULL;
  }
}

/***********************            Send            *************************/
BOOL SendRequest(cAddress *pRemAddr, t_fragmented_buffer * frbu)
{ if (pRemAddr->is_http)
    return send_http(pRemAddr, frbu, DT_REQUEST);
  else
    return Send(pRemAddr, frbu, DT_REQUEST); 
}

BOOL SendExportAcknowledge(cAddress *pRemAddr, void *pBuff, uns32 Size)
{ t_message_fragment mfg = { Size, (char*)pBuff };
  t_fragmented_buffer frbu(&mfg, 1);
  if (pRemAddr->is_http)
    return send_http(pRemAddr, &frbu, DT_EXPORTACK);
  else
    return Send(pRemAddr, &frbu, DT_EXPORTACK);
}

BOOL SendImportAcknowledge(cAddress *pRemAddr, void *pBuff, uns32 Size)
{ t_message_fragment mfg = { Size, (char*)pBuff };
  t_fragmented_buffer frbu(&mfg, 1);
  if (pRemAddr->is_http)
    return send_http(pRemAddr, &frbu, DT_IMPORTACK);
  else
    return Send(pRemAddr, &frbu, DT_IMPORTACK);
}

BOOL SendVersion(cAddress *pRemAddr, LONG lVersion)
{ t_message_fragment mfg = { sizeof(uns32), (char*)&lVersion };
  t_fragmented_buffer frbu(&mfg, 1);
  if (pRemAddr->is_http)
    return send_http(pRemAddr, &frbu, DT_VERSION);
  else
    return Send(pRemAddr, &frbu, DT_VERSION);
}

BOOL SendCloseConnection(cAddress *pRemAddr)
{ return Send(pRemAddr, NULL, DT_CLOSE_CON); }

/****************                  Receive                 ******************/
// Copies data to cdp->msg, returns FALSE if no memory for buffer allocation.
static BOOL CopyData(cdp_t cdp, BYTE *pData, unsigned DataSize, sWbHeader *pWbHeader)
/*******************************************************************************/
{ MsgInfo *pMsg = &cdp->msg;
  if (!pMsg->has_buffer())
    if (!pMsg->create_buffer(pWbHeader->wMsgDataTotal))
      return FALSE;
  pMsg->add_data(pData, DataSize);
  return TRUE;
}

void ReceiveHTTP(cdp_t cdp, char * body, int bodysize)
{ BYTE * pData = (BYTE *)body+1;  bodysize--;
  switch (*body)                                            
  { case DT_ANSWER:
      if (cdp->commenc)  // decrypt!
          cdp->commenc->decrypt(pData, bodysize);
      write_ans_proc(cdp, (answer_cb *)pData);
      break;
    case DT_CONNRESULT:
      cdp->auxinfo=MAKELONG(0xffff, *pData);
      break;
#if WBVERS>=900
    case DT_NUMS :
      if (notification_callback)
      { cdp->notification_parameter=corealloc(2*sizeof(trecnum), 81);
        if (cdp->notification_parameter)
        { memcpy(cdp->notification_parameter, pData, 2*sizeof(trecnum));
          cdp->notification_type=NOTIF_PROGRESS;
          cdp->RV.holding |= 4;
        }
      }
      break;
    case DT_SERVERDOWN:
      if (notification_callback)
      { cdp->notification_type=NOTIF_SERVERDOWN;
        cdp->RV.holding |= 4;
      }
      break;
    case DT_MESSAGE:
      if (notification_callback)
      { cdp->notification_parameter=corealloc(strlen((const char*)pData)+1, 81);
        if (cdp->notification_parameter)
        { strcpy((char*)cdp->notification_parameter, (const char*)pData);
          cdp->notification_type=NOTIF_MESSAGE;
          cdp->RV.holding |= 4;
        }
      }
      break;
#endif
    case DT_EXPORTRQ:
    { uns32 wr=((t_container *)cdp->auxptr)->write((tptr)pData, bodysize);
      if (wr!=bodysize) wr=0x80000000;
      SendExportAcknowledge(cdp->pRemAddr, &wr, sizeof(uns32));
      break;
    }
    case DT_IMPORTRQ:
    { char abImpBuff[RDSTEP];
      uns32 rdres=((t_container *)cdp->auxptr)->read(abImpBuff, *(uns32*)pData);
      if (!rdres || rdres==(uns32)HFILE_ERROR)
        { abImpBuff[0]=(uns8)0xff;  rdres=1; }
      SendImportAcknowledge(cdp->pRemAddr, abImpBuff, rdres);
      break;
    }
  } // switch
}

// !! CopyData error handling
BOOL Receive(cd_t * cdp, BYTE *pBuff, unsigned Size)
/***************************************************/
{ if (!cdp) return TRUE;
  sWbHeader *pWbHeader = (sWbHeader *) pBuff;
  BYTE *pData = pBuff + sizeof (sWbHeader);
  unsigned DataSize = Size - sizeof(sWbHeader);

  switch (pWbHeader->bDataType)
  { case DT_ANSWER:
    { MsgInfo *pMsg = &cdp->msg;
      CopyData(cdp, pData, DataSize, pWbHeader);
      if (pMsg->has_all())
      { if (cdp->commenc)  // decrypt!
          cdp->commenc->decrypt(pMsg->_set(), pMsg->_total());
        write_ans_proc (cdp, (answer_cb *)pMsg->_set());
        pMsg->release();
      }
      break;
    }
    case DT_CONNRESULT:
      cdp->auxinfo=MAKELONG(0xffff, *pData);  // 1 byte sent, auxinfo must be !=0 whatever received
      break;
#if WBVERS<900
#ifdef CLIENT_GUI
    case DT_NUMS :
      if (cdp->RV.hClient)
      { cdp->RV.holding |= 4;
        cdp->server_progress_num1=((trecnum*)pData)[0];
        cdp->server_progress_num2=((trecnum*)pData)[1];
        SetLocalAutoEvent(&cdp->hSlaveSem);
      }
      break;
    case DT_SERVERDOWN:
      if (cdp->RV.hClient)
      { char str[80], caption[30];
        BringWindowToTop(GetParent(cdp->RV.hClient));
        SetFocus(GetParent(cdp->RV.hClient));
        LoadString(hKernelLibInst, SERVER_EXIT,         str,     sizeof(str));
        LoadString(hKernelLibInst, SERVER_MESSAGE_CAPT, caption, sizeof(caption));
        MessageBox(GetParent(cdp->RV.hClient), str, caption, MB_OK | MB_ICONSTOP | MB_APPLMODAL);
      }
      break;
    case DT_MESSAGE:
      if (cdp->RV.hClient)
      { BringWindowToTop(GetParent(cdp->RV.hClient));
        SetFocus(GetParent(cdp->RV.hClient));
        char caption[30];
        LoadString(hKernelLibInst, SERVER_MESSAGE_CAPT, caption, sizeof(caption));
        MessageBox(GetParent(cdp->RV.hClient), (tptr)pData, caption, MB_OK | MB_ICONSTOP | MB_APPLMODAL);
      }
      break;
#endif
#else // since version 9.0
    case DT_NUMS :
      if (notification_callback && !cdp->notification_parameter)
      { cdp->notification_parameter=corealloc(2*sizeof(trecnum), 81);
        if (cdp->notification_parameter)
        { memcpy(cdp->notification_parameter, pData, 2*sizeof(trecnum));
          cdp->notification_type=NOTIF_PROGRESS;
          cdp->RV.holding |= 4;
#ifndef PEEKING_DURING_WAIT
#ifndef SINGLE_THREAD_CLIENT
          SetLocalAutoEvent(&cdp->hSlaveSem);
#endif
#endif
        }
      }
      break;
    case DT_SERVERDOWN:
      if (notification_callback && !cdp->notification_parameter)
      { cdp->notification_type=NOTIF_SERVERDOWN;
        cdp->RV.holding |= 4;
#ifndef PEEKING_DURING_WAIT
#ifndef SINGLE_THREAD_CLIENT
        SetLocalAutoEvent(&cdp->hSlaveSem);
#endif
#endif
      }
      break;
    case DT_MESSAGE:
      if (notification_callback && !cdp->notification_parameter)
      { cdp->notification_parameter=corealloc(strlen((const char*)pData)+1, 81);
        if (cdp->notification_parameter)
        { strcpy((char*)cdp->notification_parameter, (const char*)pData);
          cdp->notification_type=NOTIF_MESSAGE;
          cdp->RV.holding |= 4;
#ifndef PEEKING_DURING_WAIT
#ifndef SINGLE_THREAD_CLIENT
          SetLocalAutoEvent(&cdp->hSlaveSem);
#endif
#endif
        }
      }
      break;
#endif
    case DT_EXPORTRQ:
    { uns32 wr=((t_container *)cdp->auxptr)->write((tptr)pData, DataSize);
      if (wr!=DataSize) wr=0x80000000;
      SendExportAcknowledge(cdp->pRemAddr, &wr, sizeof(uns32));
      break;
    }
    case DT_IMPORTRQ:
    { char abImpBuff[RDSTEP];
      uns32 rdres=((t_container *)cdp->auxptr)->read(abImpBuff, *(uns32*)pData);
      if (!rdres || rdres==(uns32)HFILE_ERROR)
        { abImpBuff[0]=(uns8)0xff;  rdres=1; }
      SendImportAcknowledge(cdp->pRemAddr, abImpBuff, rdres);
      break;
    }
    case DT_KEEPALIVE:  // reply with the keep alive datagram
		{	cAddress *pAdr = cdp->pRemAddr;
			if (pAdr)	pAdr->SendKeepAlive(cdp);
      break;
		}
    case DT_CLOSE_CON:  // close connection
			return FALSE; 
  }
  return TRUE;
}

void StopReceiveProcess(cdp_t cdp)
/* Called when connection failed or has been closed by the other side. */
/*************************************/
{ if (!cdp) return;
  BYTE user = cdp->in_use;
  if (user == PT_REMOTE)  /* client side */
  { if (cdp->RV.hClient)  // unless called from interf_close() for remote client
    { 
#ifdef WINS 
#if WBVERS<900
      BringWindowToTop(GetParent(cdp->RV.hClient));
      SetFocus(GetParent(cdp->RV.hClient));
      char str[80];  LoadString(hKernelLibInst, SERVER_TERM, str,  sizeof(str));
      MessageBox(GetParent(cdp->RV.hClient), str, "", MB_OK | MB_ICONSTOP | MB_APPLMODAL);
#endif
#endif
    }
    if (cdp->RV.holding & 2) 
    { //client_error(cdp, CONNECTION_LOST); -- removed because this thread cannot write to the client log
      cdp->RV.holding=1; // answered, but messages still blocked
#ifndef PEEKING_DURING_WAIT
#ifndef SINGLE_THREAD_CLIENT
      SetLocalAutoEvent(&cdp->hSlaveSem);
#endif
#endif
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//                              Start / Stop
/////////////////////////////////////////////////////////////////////////////

static int ProtocolStart(int reqProtocol)  // Starts reqProtocol, writes it to wProtocol if OK
/* Returns KSE_OK if any protocol started. */
{ int bCCode=KSE_OK, acode;
  wProtocol = 0;
#if ALTERNATIVE_PROTOCOLS
#ifndef UNIX
  if (reqProtocol & IPXSPX)
  { acode=IpxProtocolStart ();
    if (acode != KSE_OK) bCCode=acode;  else wProtocol |= IPXSPX;
  }
#endif
#ifdef WINS
  if (reqProtocol & NETBIOS)
  { acode=NetBiosProtocolStart ();
    if (acode != KSE_OK) bCCode=acode;  else wProtocol |= NETBIOS;
  }
#endif
#endif
  if (reqProtocol & TCPIP)
  { acode=TcpIpProtocolStart ();
    if (acode != KSE_OK) bCCode=acode;  else wProtocol |= TCPIP;
  }
  return wProtocol ? KSE_OK : bCCode;
}

static int ProtocolStop(cdp_t cdp)  // Stops all running protocols
{ int bRetCode=KSE_OK, acode;
#if ALTERNATIVE_PROTOCOLS
#ifndef UNIX
  if (wProtocol & IPXSPX)
  { acode = IpxProtocolStop (cdp);
    if (acode != KSE_OK) bRetCode=acode;
  }
#endif
#ifdef WINS
  if (wProtocol & NETBIOS)
  { acode = NetBiosProtocolStop (cdp);
    if (acode != KSE_OK) bRetCode=acode;
  }
#endif
#endif
  if (wProtocol & TCPIP)
  { acode = TcpIpProtocolStop (cdp);
    if (acode != KSE_OK) bRetCode=acode;
  }
  return bRetCode;
}

void ReceiveStart (void)
/**********************/
{ 
#if ALTERNATIVE_PROTOCOLS
#ifndef UNIX
  if (wProtocol & IPXSPX)  IpxReceiveStart();
#endif
#ifdef WINS
  if (wProtocol & NETBIOS) NetBiosReceiveStart();
#endif
#endif
  if (wProtocol & TCPIP)   TcpIpReceiveStart();
}


void ReceiveStop (void)
/*********************/
{ 
#if ALTERNATIVE_PROTOCOLS
#ifndef UNIX
  if (wProtocol & IPXSPX)  IpxReceiveStop();
#endif
#ifdef WINS
  if (wProtocol & NETBIOS) NetBiosReceiveStop();
#endif
#endif
  if (wProtocol & TCPIP)   TcpIpReceiveStop();
}

/********************  Network Start - Stop *********************/
static int GetAvailableProtocol(void)
// Determines the protocol if not specified in the registry or INI.
// If server is specified by IP address or host name then the protocol is not found in the registry or /etc.
//  Then the default choice of the TCP/IP protocol is necessary!
{
#ifdef WINS
  if (TcpIpPresent())   return TCPIP;  // TCP/IP checked before netbios from 6.0, is the first from 8.0
#if ALTERNATIVE_PROTOCOLS
  if (IpxPresent())     return IPXSPX;
  if (NetBiosPresent()) return NETBIOS;
#endif
  return 0;
#endif
#ifdef UNIX
  return TCPIP;
#endif
}

static int iNetInstCounter=0;

CFNC DllExport int NetworkStart(int reqProtocol)
/***********************************/
{ int bCCode;
  if (iNetInstCounter) { ++iNetInstCounter;  return KSE_OK; }
  if (!reqProtocol) reqProtocol=GetAvailableProtocol();
  if ((bCCode = ProtocolStart(reqProtocol)) != KSE_OK) return bCCode;
  ReceiveStart();
  ++iNetInstCounter;
  return KSE_OK;
}

CFNC DllExport int NetworkStop(cdp_t cdp)
/**********************************/
{ if ( !iNetInstCounter) return KSE_OK; /* error, network has not been init. */
  if (--iNetInstCounter) return KSE_OK;
  ReceiveStop();
#ifdef WINS
  ras_hangup(); // temp. remove
#endif
  ProtocolStop(cdp);
  return KSE_OK;
}

////////////////////////////// WinWait //////////////////////////////////////
#ifdef WINS

BOOL WinWait (DWORD dwTickReference, DWORD dwTickTimeout)
/**************************************************************/
{
  if (dwTickReference)
    if (GetTickDifference (GetTickCount (), dwTickReference) > dwTickTimeout)
      return FALSE;
  Sleep(50);  // delay 100 ms
  return TRUE;
}

#else
#include <sys/time.h>

DWORD GetTickCount(void)  // returns 1000 ticks per second
// ATTN: The tick counter wraps, but GetTickDifference resolves this
{ //return clock() * (1000/CLOCKS_PER_SEC); // wrong, this is the processor time
  struct timeval tv;  struct timezone tz;
  if (gettimeofday(&tv, &tz)) return 0;
  return (DWORD)(1000*(uns64)tv.tv_sec + (uns64)(tv.tv_usec/1000));
}

BOOL WinWait(DWORD dwTickReference, DWORD dwTickTimeout)
/**************************************************************/
{ if (dwTickReference)
    if (GetTickDifference (GetTickCount(), dwTickReference) > dwTickTimeout)
      return FALSE;
#ifdef UNIX
  Sleep(100);
#else
  ThreadSwitchWithDelay();
#endif
  return TRUE;
}

#endif

#include "cnetapi.cpp"

///////////////////////////////////////////// support for searching servers //////////////////////////////
static bool broadcast_enabled = true;
static bool broadcast_register_read = false;  // set to true when [broadcast_enabled] is defined either fro the registry or by the client

bool can_broadcast(void)  /* returns true unless broadcasting disabled */
{
#ifdef WINS
  if (!broadcast_register_read)
  { HKEY hKey;  char key[160];  DWORD state=0, key_len=sizeof(state);
    strcpy(key, WB_inst_key);  strcat(key, Database_str);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey) == ERROR_SUCCESS)
    { RegQueryValueEx(hKey, Broadcast_str, NULL, NULL, (BYTE*)&state, &key_len);
      RegCloseKey(hKey);
    }
    broadcast_enabled = state==0;
    broadcast_register_read = true;
  }
#endif
  return broadcast_enabled;
}

CFNC DllKernel void WINAPI Enable_server_search(BOOL enable)
{ broadcast_enabled = enable!=FALSE;
  broadcast_register_read = true;
}

