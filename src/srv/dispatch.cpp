/****************************************************************************/
/* Dispatch.cpp - the request dispatcher                                    */
/****************************************************************************/
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
#include "worm.h"
#include "profiler.h"
#include "nlmtexts.h"
#include "opstr.h"
#include <stddef.h>  // offsetof
#ifdef WINS
#include <process.h>  
#include <time.h>
#else
#ifdef UNIX
#include <errno.h>
#include <semaphore.h>
#include <syslog.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/vfs.h>
#include <unistd.h>   // getpid
#include <sys/time.h>
#include <netdb.h>
#else
#include <nwmisc.h>	// GetCurrentTicks
#include <stddef.h>
#include <fcntl.h>
#include <nwsync.h>
#include <errno.h>
#include <process.h>
#endif
#endif
#include "netapi.h"
#include <flstr.h>

#include "regstr.cpp"
#include "wbsecur.h"
#include "enumsrv.h"
#if defined(WINS) && defined(_DEBUG) && !defined(X64ARCH) // handle for stackwalk
#include "stackwalk.h"
#endif
/***************************************** list of tasks ***********************************
Every task is represented by a thread and a cd structure, which is pointed by the cdp.
Pointers to all tasks (cdp) are in the cds[] list.
The access to cds is protected by cs_client_list critical section.
Task pointera are never moved to another place in cds.
Index of the cdp in cds is stored in the applnum_perm member of cd and identifies the task.
*/

cd_t  my_cd(PT_SERVER);
cdp_t my_cdp = &my_cd;
cd_t * cds[MAX_MAX_TASKS];  // Protected by [cs_client_list].
static int login_locked=0;     // > 0 iff server is locked. Protected by [cs_client_list].
int max_task_index = 0;  // max used index in cds
unsigned server_process_id = 0;
static uns32 reg_date=0;  // IK creation date + 30
static bool trial_fulltext_added = false, trial_lics_added = false;
int server_tzd;  // local time zone displacement in seconds, UTC = local - server_tzd
t_specif sys_spec =   // copied from the header and converted 0->1, 255->0
#ifdef WINS
  t_specif(0, CHARSET_NUM_UTF8, 0, 0);  // this value if used when converting server messages before openning the database file
#else
  t_specif(0, CHARSET_NUM_UTF8, 0, 0);
#endif
int http_tunnel_port_num = 80, http_web_port_num = 80;
time_t server_start_time;
bool is_dependent_server = false;
unsigned client_logout_timeout = 0;  // global parameter of the shutdown process, must be set before dir_kernel_close is called
bool is_service = false;

static int AddTask(cdp_t cdp) 
// Adds the cdp task into the cds task list, opens its FIL access.
// Returns TRUE iff cds[] is full or if open_fil_access fails. 
{ int new_ind;
  ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
  if (cdp->in_use==PT_SLAVE || cdp->in_use==PT_MAPSLAVE || cdp->in_use==PT_WORKER)
    if (login_locked)  // this test should be inside [cs_client_list], otherwise the server may be locked when client is being connected
      return KSE_SERVER_CLOSED;
  if (cdp->in_use == PT_SERVER) new_ind = 0;
  else
  { new_ind=1;  // find space if the cds:
    while (cds[new_ind] != NULL)
      if (++new_ind >= MAX_MAX_TASKS) return KSE_NOTASK; /* no free cell in the cds[] */
  }
  cdp->tzd        = server_tzd;
  cdp->conditional_attachment = cdp->in_use==PT_SLAVE || cdp->in_use==PT_KERDIR;
  cdp->last_request_time=stamp_now();  // prevents immediate disconnecting in check_for_dead_clients()
  cdp->appl_state = WAITING_FOR_REQ;
  cdp->threadID   = GetClientId();
#if defined(WINS) && defined(_DEBUG) && !defined(X64ARCH) // handle for stackwalk
  if (new_ind)
	  DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &cdp->thread_handle, 0, false, DUPLICATE_SAME_ACCESS);
#endif
  cdp->applnum_perm = cdp->applnum  = (t_client_number)new_ind;
  cds[new_ind]    = cdp;
  if (new_ind > max_task_index) max_task_index=new_ind;

  int retval=ffa.open_fil_access(cdp, server_name);
  if (retval!=KSE_OK) { if (new_ind) cds[new_ind]=NULL;  return retval; }
  return KSE_OK;
}

static void RemoveTask(cdp_t cdp)
// Removes the cdp task from the cds task list, closes its FIL access.
{ ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
 // remove forgotten kernel locks:
  login_locked -= cdp->my_server_locks;  cdp->my_server_locks=0;
  ffa.close_fil_access(cdp);
 // change dependent logins into one independent and other dependent on him:
  if (cdp->in_use==PT_KERDIR || cdp->in_use==PT_SLAVE)
    client_licences.return_access_licence(cdp);
  cds[cdp->applnum_perm]=NULL;
#if defined(WINS) && defined(_DEBUG) && !defined(X64ARCH) // handle for stackwalk
	CloseHandle(cdp->thread_handle);  cdp->thread_handle=0;
#endif
  cdp->applnum=cdp->applnum_perm=NOAPPLNUM;  // must be after return_access_licence
}

int get_external_client_connections(void)
// Ignoring workers, detached a weak clients connected
{ int count = 0;
  ProtectedByCriticalSection cs(&cs_client_list);
  for (int i=1;  i<=max_task_index;  i++)   /* system process 0 not counted */
    if (cds[i])
    { cdp_t acdp = cds[i];
      if (acdp->in_use==PT_SLAVE || acdp->in_use==PT_KERDIR)  // ignoring system workers and detached thread
        if (!acdp->weak_link)  // ignoring weak linked clients
          count++;
    }
  return count;  
}

CRITICAL_SECTION cs_short_term, cs_medium_term, cs_profile, cs_gcr;
CRITICAL_SECTION cs_trace_logs;  // protects [pending_log_request_list], [trace_log_list] and every log (its cache and file access)
LocEvent hLockOpenEvent = 0;
LocEvent Zipper_stopped = 0;

bool running_as_service = false;
BOOL back_end_operation_on = FALSE;
t_down_reason down_reason = down_system;
/************************ client search and information *********************/
// Variables defined when starting the server
bool xml_allowed = false;      // used in v. 11.0
#if WBVERS<900
bool fulltext_allowed = true, 
     network_server_licence = true;                // always allowed before 9.0
#else
bool fulltext_allowed = true,  // always allowed 9.0-10.0
     network_server_licence = false;               // may be enabled by licence or in the trial mode
#endif
t_client_licences client_licences;
char installation_key[MAX_LICENCE_LENGTH+1];
char ServerLicenceNumber[MAX_LICENCE_LENGTH+1];
bool my_private_server = false;

BOOL t_client_licences::get_access_licence(cdp_t cdp)
{ //if (cdp->in_use==PT_KERDIR && direct_clients) 
  //  return TRUE;  // local clients sharing the licence
  if (!server_shared_info.get_licence(installation_key)) return FALSE;      // no licence available
  licences_consumed++;
  return TRUE;
}

void t_client_licences::return_access_licence(cdp_t cdp)
// Celled iside [cs_client_list].
{ if (cdp->conditional_attachment) return;  // does not have the licence
 // check for co-owning the licence, update the group description:
  BOOL co_owning = cdp->dep_login_of != 0;
  int new_depend = 0;
  for (int j=1;  j<=max_task_index;  j++) if (cds[j])
    if (cds[j]->dep_login_of==cdp->applnum_perm)
    { co_owning = TRUE; 
      if (!new_depend)
        {  cds[j]->dep_login_of=0;  new_depend=j; }
      else cds[j]->dep_login_of=new_depend;
    }
 // return the licence unless co-owned:
  if (!co_owning && !cdp->limited_login)
  { //if (cdp->in_use!=PT_KERDIR || direct_clients==1)
    { server_shared_info.return_licence(installation_key);
      licences_consumed--;
    }
  }
 // switch to the state without the licence:
  if      (cdp->in_use==PT_SLAVE)  network_clients--;
  else if (cdp->in_use==PT_KERDIR) direct_clients--;
  if (cdp->www_client)
    www_clients--;
  cdp->conditional_attachment = cdp->in_use==PT_SLAVE || cdp->in_use==PT_KERDIR;
}

void t_client_licences::close(void)
// Called when server terminates just before closing access to the pool of licences. 
// Returns the licences not returned by clients to the common pool.
{ while (licences_consumed)
  { server_shared_info.return_licence(installation_key);
    licences_consumed--;
  }
}

void client_count_string(char * str)  // returns standartised info about connected users as string
{ sprintf(str, "%u(%u)", client_licences.licences_consumed, 
                         client_licences.network_clients+client_licences.direct_clients);
  if (client_licences.www_clients) sprintf(str+strlen(str), "+%u", client_licences.www_clients);
}

void client_start_error(cdp_t cdp, const char * message_start, int error_code)
// Logs the error returned by interf_init on thread start
{ char message[150];
  strcpy(message, message_start);
  strcat(message, kse_errors[error_code]);
  if (cdp && error_code==KSE_IP_FILTER)  // append the parameter
    sprintf(message+strlen(message), " (%s)", cdp->errmsg);
  dbg_err(message);
}

#if WBVERS<900
#ifdef WINS
void PrintAttachedUsers(void)
{ char szLine[81], *p;  BOOL any_user=FALSE;
  ProtectedByCriticalSection cs(&cs_client_list);
  for (int i = 1; i <= max_task_index; ++i)
    if (cds[i] != NULL)
    { cdp_t cdp = cds[i];
      memset(szLine, ' ', sizeof(szLine));
      p=szLine;
     // flags:
      if      (cdp->in_use == PT_KERDIR) *p='D';
      else if (cdp->in_use == PT_SLAVE)  *p='N';
      else if (cdp->in_use == PT_REMOTE || cdp->in_use == PT_WORKER) continue;
      else                               *p=' ';
      p++;
      *(p++) = cdp->appl_state == REQ_RUNS ? '+' : '-';
      int2str(cdp->wait_type, p);  p+=strlen(p);
      //*(p++) = cdp->wait_type < 10 ? ('0'+cdp->wait_type) : '#';
      *p=' ';  p++;  // space
     /* user name */
      memcpy(p, cdp->prvs.luser_name(), strlen(cdp->prvs.luser_name()));
      p+=OBJ_NAME_LEN+1;  *p=0;
     /* station address */
      if (cdp->in_use == PT_MAPSLAVE)
        form_message(p, 30, localClient);
      else  /* station address */
        if (cdp->pRemAddr) cdp->pRemAddr->GetAddressString(p);
      display_server_info(szLine);  // writes the line to the console
      any_user=TRUE;
    }
  if (!any_user) display_server_info(form_message(szLine, sizeof(szLine), noAttached));
}
#endif
#endif

#ifdef WINS
THREAD_HANDLE hThreadMakerThread = 0;  // not used on UNIX
HANDLE NoIPCEvent = 0;

PSECURITY_DESCRIPTOR pSD;
SECURITY_ATTRIBUTES  SA;

/* Initialize a security descriptor. */
static void make_security_descr(void)
{ if (pSD!=NULL) return;
  SA.nLength=sizeof(SECURITY_ATTRIBUTES);
  SA.lpSecurityDescriptor=pSD;
  SA.bInheritHandle=FALSE;
  pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);   /* defined in WINNT.H */
  SA.nLength=sizeof(SECURITY_ATTRIBUTES);
  SA.lpSecurityDescriptor=pSD;
  SA.bInheritHandle=FALSE;
  if (pSD != NULL)
  { if (InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))  /* defined in WINNT.H */
    /* Add a NULL disc. ACL to the security descriptor. */
 	  if (SetSecurityDescriptorDacl(pSD, TRUE,     /* specifying a disc. ACL  */
          (PACL) NULL, FALSE))  /* not a default disc. ACL */
      /* Add the security descriptor to the file. */
 	    /*if (SetFileSecurity(lpszTestFile, DACL_SECURITY_INFORMATION, pSD)) */
        return; 	/* OK */
    LocalFree((HLOCAL) pSD);  pSD=NULL;     
  }
}

static void free_security_descr(void)
{ if (pSD!=NULL) { LocalFree((HLOCAL) pSD);  pSD=NULL; } }

#include "dircomm.cpp"

// these variables are protected by the mutex on the CLIENT side
static HANDLE hMakeDoneEvent  = NULL;
static DWORD *mapped_clientId = NULL;
static bool mapped_long = false;
static bool try_global = true;  // define by ThreadMakerThreadProc, used by ThreadMakerThreadProc and DirectSlaveThread

THREAD_PROC(DirectSlaveThread)
{ unsigned id = *(DWORD*)data;  // thread parameter
  HANDLE LocalProcessHandle = 0;  bool client_supports_multipart = false;
#if WBVERS>=900
  if (mapped_long)  // create a local handle of the client process and enable the detecion of the termination of the client
  { DWORD ClientProcessId = ((DWORD*)data)[1];
    if (ClientProcessId)
      LocalProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, ClientProcessId);
  }
#endif
  int res=KSE_OK; 	
  cddir_t my_cd;
  if (my_cd.dcm.OpenClientSlaveComm(id, server_name, &SA, try_global))
  { cdp_t cdp=&my_cd;
    res = interf_init(cdp);
	  if (res == KSE_OK)
	  { my_cd.LinkIdNumber = id;     /* store LinkId generated by client */
//#if WBVERS>=900  -- added for 8.1 too
      if (mapped_long)  // return own process id to the client
        ((DWORD*)data)[1] = GetCurrentProcessId();
//#endif
      my_cd.hSlaveSem=my_cd.dcm.DataEvent();  /* used when killing the user */
      *mapped_clientId=KSE_OK;     /* no error supposed */
      SetEvent(hMakeDoneEvent);    /* server-side ready */

      { char buf[81];
        trace_msg_general(cdp, TRACE_LOGIN, form_message(buf, sizeof(buf), msg_connect, cdp->applnum_perm), NOOBJECT);
      }
      my_cd.dcm.PrepareSynchro(LocalProcessHandle);  // sets smart_wait when LocalProcessHandle!=0
      do  /* request-answer cycle */
      { my_cd.wait_type=WAIT_FOR_REQUEST;
       // get the next request:
        char * request_copy = NULL;  unsigned request_copy_len=0;  uns32 rest_length, transferred;
        do // itrerate on request parts:
        { if (!my_cd.dcm.WaitForRequest2())
            goto termin;  // client process terminated
         // new multipart request processing:   
          if (my_cd.dcm.IsMultipartReq())
          { client_supports_multipart = true;
            if (!request_copy)  // 1st part, allocate full length
            { rest_length = *(uns32*)&my_cd.dcm.rqst->req;
              request_copy = (char*)corealloc(rest_length, 1);
              if (!request_copy) 
                goto termin;  // client will hang up
              transferred = my_cd.dcm.request_part_size-sizeof(uns32); 
              memcpy(request_copy, (char*)&my_cd.dcm.rqst->req + sizeof(uns32), transferred);
              rest_length -= transferred;
            }
            else
            { uns32 part_length = rest_length < my_cd.dcm.request_part_size ? rest_length : my_cd.dcm.request_part_size;
              memcpy(request_copy+transferred, &my_cd.dcm.rqst->req, part_length);
              transferred += part_length;
              rest_length -= part_length;
              if (!rest_length) break;  // the last part copied
            }
          }
         // old multipart request:
          else if (my_cd.dcm.IsPartialReq() || request_copy)
          { request_copy = (char*)corerealloc(request_copy, request_copy_len+my_cd.dcm.request_part_size);
            if (!request_copy) 
              goto termin;  // client will hang up
            memcpy(request_copy+request_copy_len, &my_cd.dcm.rqst->req, my_cd.dcm.request_part_size);
            request_copy_len+=my_cd.dcm.request_part_size; 
            if (!my_cd.dcm.IsPartialReq())
              break;  //  the last part copied
          }
          else  // normal single-part request
            break;
          my_cd.dcm.RequestCompleted();  // ready for the next part of the request
        } while (true);
        if (my_cd.appl_state == SLAVESHUTDOWN) break; // used if client disconnected during waiting for its new request
        request_frame * the_request = request_copy ? (request_frame*)request_copy : &my_cd.dcm.rqst->req;
        if (the_request->areq[0] == OP_ACCCLOSE) break;
        my_cd.wait_type=WAIT_NO;
       // process the request:
        new_request(cdp, the_request);
        corefree(request_copy);
       // return the answer (get_ans() and get_anssize() include the prefix):
        const char * pans = (const char*)cdp->answer.get_ans();  // points to the prefix and data
        memcpy(my_cd.dcm.dir_answer, pans, ANSWER_PREFIX_SIZE); // copy the prefix of the answer 
        if (cdp->answer.get_anssize() > ANSWER_PREFIX_SIZE+my_cd.dcm.answer_part_size && client_supports_multipart)
        { pans += ANSWER_PREFIX_SIZE;
          my_cd.dcm.SetMultipartAns(true);
          int step = my_cd.dcm.answer_part_size - sizeof(uns32);  // data in the 1st part
          *(uns32*)my_cd.dcm.dir_answer->return_data = cdp->answer.get_anssize();
          memcpy(my_cd.dcm.dir_answer->return_data+sizeof(uns32), pans, step);
          my_cd.dcm.RequestCompleted();  // request completed - but the answer may be partial
          uns32 rest_size = cdp->answer.get_anssize() - ANSWER_PREFIX_SIZE - step;
          do
          { pans+=step;
           // wait for client to precess the previous part:
            if (!my_cd.dcm.WaitForRequest2())
              goto termin;  // client process terminated
           // send the next part:
            step = rest_size < my_cd.dcm.answer_part_size ? rest_size : my_cd.dcm.answer_part_size;
            memcpy(my_cd.dcm.dir_answer->return_data, pans, step);
            my_cd.dcm.RequestCompleted();  // signal for the client that the next part of the answer is anailable
            rest_size-=step;
          } while (rest_size);
        }
        else  // short answer or "partial" answer style
        { bool partial;  int sent=ANSWER_PREFIX_SIZE;
          do
          { int step = cdp->answer.get_anssize() - sent;
            if (step>my_cd.dcm.answer_part_size) 
              { step=my_cd.dcm.answer_part_size;  partial=true; }
            else partial=false;
            memcpy(my_cd.dcm.dir_answer->return_data, pans+sent, step); // copy the part of the answer
            my_cd.dcm.SetPartialAns(partial);
            my_cd.dcm.RequestCompleted();  // request completed - but the answer may be partial
            if (!partial) break;
            sent+=step;
           // wait for client to process the part of the answer:
            if (!my_cd.dcm.WaitForRequest2())
              goto termin;  // client process terminated
          } while (true);
        }
        cdp->answer.free_answer();                                        // ... and release it
      } while (my_cd.appl_state != SLAVESHUTDOWN); // used if client disconnected during its operation on the server
     termin:
      my_cd.wait_type=WAIT_TERMINATING;
      { char buf[81];
        trace_msg_general(cdp, TRACE_LOGIN, form_message(buf, sizeof(buf), msg_disconnect, cdp->applnum_perm), NOOBJECT);
      }
      kernel_cdp_free(cdp);
      cd_interf_close(cdp);
      my_cd.dcm.SetServerDown();
      my_cd.dcm.RequestCompleted();  /* OP_ACCCLOSE request completed */
    }
    else /* slave start error */
    { *mapped_clientId = res;  // give the error code to the client
	    SetEvent(hMakeDoneEvent);   /* slave creation completed */
    }
    my_cd.dcm.CloseClientSlaveComm();
  } // handles created
  else
  { *mapped_clientId = KSE_SYNCHRO_OBJ;  // give the error code to the client
	  SetEvent(hMakeDoneEvent);   /* slave creation completed */
  }
 // report the error, if any:
  if (res!=KSE_OK) client_start_error(&my_cd, clientThreadStart, res);
  if (LocalProcessHandle) CloseHandle(LocalProcessHandle);
  client_terminating();
  THREAD_RETURN;
}

THREAD_PROC(ThreadMakerThreadProc)
/* This thread listens for direct clients and creates the PT_MAPSLAVE threads. */
{ int res=KSE_OK;
 // do not use global shared objects on Vista unless
  { OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx(&osv);
    if (osv.dwMajorVersion >= 6)
      if (!is_service)
        try_global=false;
  }
  HANDLE hMakerEvent, hMemFile;
  if (CreateIPComm0(server_name, &SA, &hMakerEvent, &hMakeDoneEvent, &hMemFile, &mapped_clientId, mapped_long, try_global))
	{ HANDLE hnds[2] = { hMakerEvent, hKernelShutdownEvent };   
    //dbg_err("Thread maker started");
    while (TRUE)
	  { if (WaitForMultipleObjects(2, hnds, FALSE, INFINITE) != WAIT_OBJECT_0)
        break;  // kernel shut down
		  void * param=(void*)mapped_clientId;  unsigned id;
		  HANDLE hSlaveThread=(HANDLE)_beginthreadex(NULL, 10000, (LPTHREAD_START_ROUTINE)DirectSlaveThread, param, 0, &id);
      if (!hSlaveThread)
      { *mapped_clientId=KSE_START_THREAD;  /* slave creation error */
        SetEvent(hMakeDoneEvent);     /* slave creation completed */
      }
      else CloseHandle(hSlaveThread);  // is necessary!
	  }
    CloseIPComm0(hMakerEvent, hMakeDoneEvent, hMemFile, mapped_clientId);
  }
  else 
  { //dbg_err("Thread maker not started");
    res=KSE_SYNCHRO_OBJ;
  }
  THREAD_RETURN;
}

#else

// UNIX direct client not implemented so far

#endif  // !WINS

//////////////////////////////// Server initialization & shutdown //////////////////////////////////////////
tobjname server_name; // accessed directly and via the_sp, too
t_server_profile the_sp;
volatile wbbool kernel_is_init = wbfalse;  // server is started
LocEvent hKernelShutdownReq;
BOOL bCancelCopying = FALSE;

#ifdef WINS
BOOL fShutDown = FALSE, fLocking = FALSE;
HANDLE         hKernelShutdownEvent = 0;
HANDLE         hKernelLockEvent = 0;
#endif

#ifdef UNIX
inline condition_var::condition_var()
{
	pthread_cond_init(&cond, NULL);
	pthread_mutex_init(&mutex, NULL);
}

condition_var  hKernelShutdownEvent,             hKernelLockEvent;
bool_condition fShutDown(&hKernelShutdownEvent), fLocking(&hKernelLockEvent);

#endif
#ifdef NETWARE        
BOOL fShutDown = FALSE;
LONG           hKernelShutdownEvent = 0; 
#endif

t_temp_account::t_temp_account(cdp_t cdpIn, tobjnum new_usernum, tcateg category) : cdp(cdpIn)
{ 
  orig_usernum = cdp->prvs.luser();
  cdp->prvs.set_user(new_usernum, category, TRUE);  // hierarchy is necessary, may use ADMIN role in applications
};

t_temp_account::~t_temp_account(void)
{ 
  cdp->prvs.set_user(orig_usernum, CATEG_USER, TRUE);  // return to original privils
}

static void run_proc_in_every_schema(cdp_t cdp, const char * proc_name)
{ char query[200];  cur_descr * cd;
  sprintf(query, "SELECT * FROM OBJTAB WHERE category=chr(%d) AND OBJ_NAME=\'%s\'", CATEG_PROC, proc_name);
  tcursnum curs = open_working_cursor(cdp, query, &cd);
  if (curs!=NOCURSOR)
  { if (cd->recnum>0)
    { commit(cdp);  // prevents closing the cursor when a proc raises an error
      { t_temp_account tacc(cdp, CONF_ADM_GROUP, CATEG_GROUP);  // execute with CONF_ADMIN privils 
        for (trecnum r=0;  r<cd->recnum;  r++)
        { trecnum tr;  WBUUID schema_UUID;  tobjname schema_name;
          cd->curs_seek(r, &tr);
          tb_read_atr(cdp, objtab_descr, tr, APPL_ID_ATR, (tptr)schema_UUID);
          if (!ker_apl_id2name(cdp, schema_UUID, schema_name, NULL))
          { cdp->request_init();
            sprintf(query, "CALL `%s`.%s()", schema_name, proc_name);
            exec_direct(cdp, query);
            commit(cdp);
            cdp->request_init();
          }
        }
      } // return to original privils
    }
    close_working_cursor(cdp, curs);
  }
}

void stop_working_threads(cdp_t cdp)
{ 
  run_proc_in_every_schema(cdp, "_ON_WORKER_STOP");
  fLocking=true;
#ifdef WINS
    SetEvent(hKernelLockEvent);
#else // UNIX
    hKernelLockEvent.broadcast();
#endif
 // cancel waiting for events:                      
  { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
    for (int i=1;  i <= max_task_index;  i++) 
      if (cds[i])
        cds[i]->cevs.shutdown();
  }
}

void restart_working_threads(cdp_t cdp)
{ fLocking=false;
#ifdef WINS
    ResetEvent(hKernelLockEvent);
#else // UNIX
    hKernelLockEvent.broadcast();
#endif
  run_proc_in_every_schema(cdp, "_ON_WORKER_RESTART");
}

///////////////////////////////////////////////////// profile /////////////////////////////////////////////

BOOL WritePrivateProfileInt(const char * section, const char * key, int value, const char * pathname)
{ char buf[12];
  int2str(value, buf);
  return WritePrivateProfileString(section, key, buf, pathname);
}

unsigned get_last_separator_pos(const char * pathname)
// Returns the index of the last path separetor in the pathname or 0 iff none found.
{ unsigned i = (unsigned)strlen(pathname);
  while (i && pathname[--i] != PATH_SEPARATOR) ;
  return i;
}

t_property_descr * t_server_profile::find(const char * name_srch) const
{ for (int i=0;  i<SERVER_PROFILE_ITEM_COUNT;  i++)
  { t_property_descr * items = server_profile_items[i];
    if (items->is_named(name_srch)) return items;
  }
  return NULL;
}

const char sqlserver_property_name[] = "@SQLSERVER";

enum { DEFAULT_KERNEL_CACHE_SIZE = 3000, DEFAULT_PROFILE_BIGBLOCK_SIZE = 2048, DEFAULT_TOTAL_SORT_SPACE = 4096 };
enum { KILL_TIME_NOTLOGGED = (8*60*60) /* 8 hours */, KILL_TIME_LOGGED = (23*60*60) /* 23 hours */ };
enum { DEFAULT_SQL_OPTIONS = 
  SQLOPT_NULLEQNULL | SQLOPT_NULLCOMP | SQLOPT_RD_PRIVIL_VIOL |
  SQLOPT_MASK_NUM_RANGE | SQLOPT_MASK_INV_CHAR | SQLOPT_MASK_RIGHT_TRUNC | SQLOPT_EXPLIC_FREE |
  SQLOPT_OLD_ALTER_TABLE | SQLOPT_DUPLIC_COLNAMES | SQLOPT_USER_AS_SCHEMA| 
#if WBVERS<900
  SQLOPT_DISABLE_SCALED |
#endif
  SQLOPT_ERROR_STOPS_TRANS | SQLOPT_NO_REFINT_TRIGGERS | SQLOPT_USE_SYS_COLS | SQLOPT_CONSTRS_DEFERRED |
  SQLOPT_COL_LIST_EQUAL | SQLOPT_QUOTED_IDENT | SQLOPT_GLOBAL_REF_RIGHTS | SQLOPT_REPEATABLE_RETURN
};

t_property_descr_int t_server_profile::ipc_comm            ("IPCCommunication",   FALSE,  1); // not used on Linux
t_property_descr_int t_server_profile::net_comm            ("TCPIPCommunication", FALSE,  0);
t_property_descr_int t_server_profile::wb_tcpip_port       ("IPPort",             FALSE, DEFAULT_IP_SOCKET);
t_property_descr_int t_server_profile::http_tunel          ("HTTPTunnel",         FALSE,  0);
t_property_descr_int_ext t_server_profile::http_tunnel_port("HTTPTunnelPort",     FALSE, 80);
t_property_descr_int t_server_profile::web_emul            ("WebServerEmulation", FALSE,  0);
t_property_descr_int_ext t_server_profile::web_port        ("WebPort",            FALSE, 80);
t_property_descr_int t_server_profile::ext_web_server_port ("ExtWebServerPort",   FALSE, 80);
t_property_descr_int t_server_profile::using_emulation     ("UsingEmulation",     FALSE, TRUE);

t_property_descr_int t_server_profile::DisableAnonymous    ("DisableAnonymous",   TRUE,  FALSE);
t_property_descr_int t_server_profile::DisableHTTPAnonymous("DisableHTTPAnonymous",TRUE, TRUE);
t_property_descr_int t_server_profile::MinPasswordLen      ("MinPasswordLen",     TRUE,  0);
t_property_descr_int t_server_profile::FullProtocol        ("FullProtocol",       TRUE,  FALSE);
t_property_descr_int t_server_profile::PasswordExpiration  ("PasswordExpiration", TRUE,  0);
t_property_descr_int t_server_profile::ConfAdmsOwnGroup    ("ConfAdmsOwnGroup",   TRUE,  TRUE);
t_property_descr_int t_server_profile::ConfAdmsUserGroups  ("ConfAdmsUserGroups", TRUE,  TRUE);
t_property_descr_str t_server_profile::http_user           ("HTTPUser",           TRUE,  "", sizeof(http_user_name), http_user_name);
t_property_descr_str t_server_profile::server_ip_addr      ("ServerIPAddr",       FALSE, "", sizeof(server_ip_addr_str), server_ip_addr_str);
t_property_descr_int t_server_profile::kill_time_notlogged ("KillTimeNotLogged",  FALSE, KILL_TIME_NOTLOGGED);
t_property_descr_int t_server_profile::kill_time_logged    ("KillTimeLogged",     FALSE, KILL_TIME_LOGGED);
t_property_descr_int t_server_profile::UnlimitedPasswords  ("UnlimitedPasswords", TRUE,  FALSE);
t_property_descr_int t_server_profile::trace_log_cache_size("TraceLogCacheSize",  FALSE, 4001);
t_property_descr_int t_server_profile::show_as_tray_icon   ("Tray Icon",          FALSE, TRUE);
//t_property_descr_int t_server_profile::ascii_console_output("use_ascii",          FALSE, FALSE); -- not using
t_property_descr_int t_server_profile::integr_check_interv ("IntegrCheckInterv",  FALSE, 0);
t_property_descr_int t_server_profile::integr_check_extent ("IntegrCheckExtent",  FALSE, INTEGR_CHECK_CONSISTENCY_NONEX_BLOCK | INTEGR_CHECK_CONSISTENCY_CROSS_LINK | INTEGR_CHECK_CONSISTENCY_DAMAGED_TABLE);
t_property_descr_int t_server_profile::IntegrTimeDay1       ("IntegrTimeDay1",       FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeHour1      ("IntegrTimeHour1",      FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeMin1       ("IntegrTimeMin1",       FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeDay2       ("IntegrTimeDay2",       FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeHour2      ("IntegrTimeHour2",      FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeMin2       ("IntegrTimeMin2",       FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeDay3       ("IntegrTimeDay3",       FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeHour3      ("IntegrTimeHour3",      FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeMin3       ("IntegrTimeMin3",       FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeDay4       ("IntegrTimeDay4",       FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeHour4      ("IntegrTimeHour4",      FALSE, 0);
t_property_descr_int t_server_profile::IntegrTimeMin4       ("IntegrTimeMin4",       FALSE, 0);
t_property_descr_int t_server_profile::integr_check_priority("IntegrCheckPriority",  FALSE, 0);
t_property_descr_int t_server_profile::default_SQL_options ("DefaultSQLOptions",  FALSE, DEFAULT_SQL_OPTIONS);
                                         
t_property_descr_int t_server_profile::kernel_cache_size    ("Frame Space",       FALSE, DEFAULT_KERNEL_CACHE_SIZE);
t_property_descr_int t_server_profile::core_alloc_size      ("Server Memory",     FALSE, 0);
t_property_descr_int t_server_profile::DisableFastLogin     ("DisableFastLogin",  TRUE,  TRUE);
t_property_descr_int t_server_profile::CloseFileWhenIdle    ("CloseFileWhenIdle", FALSE, FALSE);
t_property_descr_int t_server_profile::profile_bigblock_size("Sort Space",        FALSE, DEFAULT_PROFILE_BIGBLOCK_SIZE);
t_property_descr_int t_server_profile::total_sort_space     ("Total Sort Space",  FALSE, DEFAULT_TOTAL_SORT_SPACE);
t_property_descr_str_multi t_server_profile::dll_directory_list("Dir",            FALSE);
t_property_descr_str t_server_profile::ThrustedDomain       ("ThrustedDomain",    TRUE,  "", sizeof(thrusted_domain), thrusted_domain);
t_property_descr_int t_server_profile::CreateDomainUsers    ("CreateDomainUsers", TRUE,  FALSE);
t_property_descr_int t_server_profile::WriteJournal         ("WriteJournal",      FALSE, FALSE);
t_property_descr_int t_server_profile::SecureTransactions   ("SecureTransactions",FALSE, FALSE);
t_property_descr_int t_server_profile::FlushOnCommit        ("FlushOnCommit",     FALSE, FALSE);
t_property_descr_int t_server_profile::FlushPeriod          ("FlushPeriod",       FALSE, 5);
t_property_descr_int t_server_profile::down_conditions      ("DownConditions",    FALSE, REL_NONEX_BLOCK);

t_property_descr_str t_server_profile::BackupDirectory      ("BackupDirectory",      FALSE, "", sizeof(backup_directory), backup_directory);
t_property_descr_int t_server_profile::BackupFilesLimit     ("BackupFilesLimit",     FALSE, 0);
t_property_descr_int t_server_profile::BackupIntervalHours  ("BackupIntervalHours",  FALSE, 0);
t_property_descr_int t_server_profile::BackupIntervalMinutes("BackupIntervalMinutes",FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeDay1       ("BackupTimeDay1",       FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeHour1      ("BackupTimeHour1",      FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeMin1       ("BackupTimeMin1",       FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeDay2       ("BackupTimeDay2",       FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeHour2      ("BackupTimeHour2",      FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeMin2       ("BackupTimeMin2",       FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeDay3       ("BackupTimeDay3",       FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeHour3      ("BackupTimeHour3",      FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeMin3       ("BackupTimeMin3",       FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeDay4       ("BackupTimeDay4",       FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeHour4      ("BackupTimeHour4",      FALSE, 0);
t_property_descr_int t_server_profile::BackupTimeMin4       ("BackupTimeMin4",       FALSE, 0);
t_property_descr_int t_server_profile::BackupZip            ("BackupZip",            FALSE, FALSE);
t_property_descr_str t_server_profile::BackupZipMoveTo      ("BackupZipMoveTo",      FALSE, "", sizeof(backup_zip_move_to), backup_zip_move_to);
t_property_descr_int t_server_profile::NonblockingBackups   ("NonblockingBackups",   FALSE, TRUE); // default changed in 10.0.1.314
t_property_descr_str t_server_profile::BackupNetUsername    ("BackupNetUsername",    FALSE, "", sizeof(backup_net_username), backup_net_username);
t_property_descr_str_hdn t_server_profile::BackupNetPassword("BackupNetPassword",    FALSE, "", sizeof(backup_net_password), backup_net_password);

t_property_descr_filcrypt t_server_profile::FilEncrypt      ("FilEncrypt");
t_property_descr_str_ips  t_server_profile::IP_enabled_addr ("IP_enabled_addr",  FALSE, TRUE,  TRUE,  0);
t_property_descr_str_ips  t_server_profile::IP_enabled_mask ("IP_enabled_mask",  FALSE, TRUE,  FALSE, 0);
t_property_descr_str_ips  t_server_profile::IP_disabled_addr("IP_disabled_addr", FALSE, FALSE, TRUE,  0);
t_property_descr_str_ips  t_server_profile::IP_disabled_mask("IP_disabled_mask", FALSE, FALSE, FALSE, 0);
t_property_descr_str_ips  t_server_profile::IP1_enabled_addr ("IP_tunnel_enabled_addr",  FALSE, TRUE,  TRUE,  1);
t_property_descr_str_ips  t_server_profile::IP1_enabled_mask ("IP_tunnel_enabled_mask",  FALSE, TRUE,  FALSE, 1);
t_property_descr_str_ips  t_server_profile::IP1_disabled_addr("IP_tunnel_disabled_addr", FALSE, FALSE, TRUE,  1);
t_property_descr_str_ips  t_server_profile::IP1_disabled_mask("IP_tunnel_disabled_mask", FALSE, FALSE, FALSE, 1);
t_property_descr_str_ips  t_server_profile::IP2_enabled_addr ("IP_web_enabled_addr",  FALSE, TRUE,  TRUE,  2);
t_property_descr_str_ips  t_server_profile::IP2_enabled_mask ("IP_web_enabled_mask",  FALSE, TRUE,  FALSE, 2);
t_property_descr_str_ips  t_server_profile::IP2_disabled_addr("IP_web_disabled_addr", FALSE, FALSE, TRUE,  2);
t_property_descr_str_ips  t_server_profile::IP2_disabled_mask("IP_web_disabled_mask", FALSE, FALSE, FALSE, 2);
t_property_descr_srvkey   t_server_profile::SrvKey          ("ServerKey");
t_property_descr_srvuuid  t_server_profile::SrvUUID         ("SrvUUID");
t_property_descr_int      t_server_profile::required_comm_enc("ReqCommEnc",      TRUE, 0);
t_property_descr_str      t_server_profile::top_CA_uuid     ("TopCA",            TRUE, "", sizeof(top_CA_str), top_CA_str);
t_property_descr_str      t_server_profile::kerberos_name   ("kerberos name",    TRUE, "", sizeof(kerberos_name_str), kerberos_name_str);
t_property_descr_str      t_server_profile::kerberos_cache  ("kerberos cache",   TRUE, "", sizeof(kerberos_cache_str), kerberos_cache_str);
t_property_descr_licnum   t_server_profile::AddOnLicence    ("AddOnLicence");
t_property_descr_str      t_server_profile::decimal_separator("DecimalSeparator",FALSE, ".", sizeof(explic_decim_separ), explic_decim_separ);
t_property_descr_int      t_server_profile::DailyBasicLog   ("DailyBasicLog",    FALSE, FALSE);
t_property_descr_int      t_server_profile::DailyUserLogs   ("DailyUserLogs",    FALSE, FALSE);
t_property_descr_int      t_server_profile::LockWaitingTimeout("LockWaitingTimeout", FALSE, 150);
#if WBVERS<900
t_property_descr_int      t_server_profile::ReportLowLicences  ("ReportLowLicences",   FALSE, TRUE);
t_property_descr_gcr      t_server_profile::gcr                ("GCR");
t_property_descr_int      t_server_profile::EnableConsoleButton("EnableConsoleButton", FALSE, TRUE);
t_property_descr_str      t_server_profile::reqProtocolProp    ("Server Protocol",   FALSE, "", sizeof(reqProtocolStr), reqProtocolStr);
#endif
t_property_descr_str      t_server_profile::ExtFulltextDir  ("ExtFulltextDir",   FALSE,"", sizeof(ext_fulltext_dir), ext_fulltext_dir);
t_property_descr_str      t_server_profile::HostName        ("HostName",         FALSE,"", sizeof(host_name),        host_name);
t_property_descr_str      t_server_profile::HostPathPrefix  ("HostPathPrefix",   FALSE,"", sizeof(host_path_prefix), host_path_prefix);
t_property_descr_int      t_server_profile::report_slow     ("ReportSlowStatement", FALSE, 150);
t_property_descr_int      t_server_profile::max_routine_stack ("MaxRoutineStack", FALSE, 100);
t_property_descr_int      t_server_profile::max_client_cursors("MaxClientCursors", FALSE, 50);
t_property_descr_int      t_server_profile::DiskSpaceLimit  ("DiskSpaceLimit", FALSE, 10);
t_property_descr_str      t_server_profile::extlob_directory("ExtLobDir", FALSE, "", sizeof(extlob_directory_str), extlob_directory_str);

// properties not stored in the __PROPTAB:
t_property_descr_str_srvname t_server_profile::ServerName("ServerName", FALSE, "", sizeof(server_name), server_name);

t_property_descr * t_server_profile::server_profile_items[SERVER_PROFILE_ITEM_COUNT] =
{ 
  &ipc_comm,
  &net_comm,
  &wb_tcpip_port,
  &http_tunel,
  &http_tunnel_port,
  &web_emul,
  &web_port,
  &ext_web_server_port,
  &using_emulation,

  &DisableAnonymous,
  &DisableHTTPAnonymous,
  &MinPasswordLen,
  &FullProtocol,
  &PasswordExpiration,
  &ConfAdmsOwnGroup,
  &ConfAdmsUserGroups,
  &http_user,
  &server_ip_addr,
  &kill_time_notlogged,
  &kill_time_logged,
  &UnlimitedPasswords,
  &trace_log_cache_size,
  &show_as_tray_icon,
  //&ascii_console_output,
  &integr_check_interv,
  &integr_check_extent,
  &IntegrTimeDay1,
  &IntegrTimeHour1,
  &IntegrTimeMin1,
  &IntegrTimeDay2,
  &IntegrTimeHour2,
  &IntegrTimeMin2,
  &IntegrTimeDay3,
  &IntegrTimeHour3,
  &IntegrTimeMin3,
  &IntegrTimeDay4,
  &IntegrTimeHour4,
  &IntegrTimeMin4,
  &integr_check_priority,
  &default_SQL_options,

  &kernel_cache_size,
  &core_alloc_size,
  &DisableFastLogin,
  &CloseFileWhenIdle,
  &profile_bigblock_size,
  &total_sort_space,
  &dll_directory_list,
  &ThrustedDomain,
  &CreateDomainUsers,
  &WriteJournal,
  &SecureTransactions,
  &FlushOnCommit,
  &FlushPeriod,
  &down_conditions,

  &FilEncrypt,
  &IP_enabled_addr,
  &IP_enabled_mask,
  &IP_disabled_addr,
  &IP_disabled_mask,
  &IP1_enabled_addr,
  &IP1_enabled_mask,
  &IP1_disabled_addr,
  &IP1_disabled_mask,
  &IP2_enabled_addr,
  &IP2_enabled_mask,
  &IP2_disabled_addr,
  &IP2_disabled_mask,
  &SrvKey,
  &SrvUUID,
  &required_comm_enc,
  &top_CA_uuid,
  &AddOnLicence,
  &BackupDirectory,
  &BackupFilesLimit,
  &BackupIntervalHours,
  &BackupIntervalMinutes,
  &BackupTimeDay1,
  &BackupTimeHour1,
  &BackupTimeMin1,
  &BackupTimeDay2,
  &BackupTimeHour2,
  &BackupTimeMin2,
  &BackupTimeDay3,
  &BackupTimeHour3,
  &BackupTimeMin3,
  &BackupTimeDay4,
  &BackupTimeHour4,
  &BackupTimeMin4,
  &BackupZip,
  &BackupZipMoveTo,
  &NonblockingBackups,
  &BackupNetUsername,
  &BackupNetPassword,

  &trace_user_error,
  &trace_network_global,
  &trace_replication,
  &trace_direct_ip,
  &trace_user_mail,
  &trace_sql,
  &trace_login,
  &trace_cursor,
  &trace_impl_rollback,
  &trace_network_error,
  &trace_replic_mail,
  &trace_replic_copy,
  &trace_repl_conflict,
  &trace_bck_obj_error,
  &trace_procedure_call,
  &trace_trigger_exec,
  &trace_user_warning,
  &trace_server_info,
  &trace_server_failure,
  &trace_log_write,
  &trace_start_stop,
  &trace_lock_error,
  &trace_web_request,
  &trace_convertor_error,

  &kerberos_name,
  &kerberos_cache,
  &decimal_separator,
  &DailyBasicLog,
  &DailyUserLogs,
  &LockWaitingTimeout,
#if WBVERS<900
  &ReportLowLicences,
  &gcr,
  &EnableConsoleButton,
  &reqProtocolProp,
#endif
  &ExtFulltextDir,
  &HostName,
  &HostPathPrefix,
  &report_slow,
  &max_routine_stack,
  &max_client_cursors,
  &DiskSpaceLimit,
  &extlob_directory,

  &ServerName
};

#if WBVERS<900
char t_server_profile::reqProtocolStr[40];
#endif
char t_server_profile::thrusted_domain[MAX_LOGIN_SERVER+1];
char t_server_profile::top_CA_str[2*UUID_SIZE+1];
char t_server_profile::backup_directory[MAX_PATH+1];
char t_server_profile::backup_zip_move_to[MAX_PATH+1];
char t_server_profile::backup_net_username[100+1];
char t_server_profile::backup_net_password[100+1];
char t_server_profile::kerberos_name_str[50];
char t_server_profile::kerberos_cache_str[MAX_PATH+1];
char t_server_profile::server_ip_addr_str[24];
tobjname t_server_profile::http_user_name;
char t_server_profile::ext_fulltext_dir[254+1];
char t_server_profile::host_name[70+1];
char t_server_profile::host_path_prefix[70+1];
char t_server_profile::extlob_directory_str[100+1];
int backup_status = 0;

struct t_proptab_primary_key
{ char owner[32];  // without terminator space
  char name[36];   // without terminator space
  short num;
  trecnum rec;
  t_proptab_primary_key(const char * ownerIn, const char * nameIn, short numIn)
  { strmaxcpy(owner, ownerIn, sizeof(owner)+1);
    strmaxcpy(name,  nameIn,  sizeof(name )+1);
    num=numIn;
  }
};

void t_server_profile::set_all_to_defaut(void)
// Sets the default values for all server properties.
{ for (int i=0;  i<SERVER_PROFILE_ITEM_COUNT;  i++)
    server_profile_items[i]->set_default();
 // the default value for net_comm is TRUE for services (added when compulsory networking for services removed):
#ifdef WINS  
  net_comm.friendly_save(is_service ? TRUE : FALSE);
#else  
  net_comm.friendly_save(TRUE);
#endif
}

void t_server_profile::load_all(cdp_t cdp)
// Passes all stored @SQLSERVER properties and loads the known ones to memory.
// When a value of a property is an empty string, then next values of the same property are not loaded.
{// if the value of the IPPort property is specified in the /etc file (or registry), use it as the default value:
  int val;
  if (GetDatabaseString(server_name, IP_socket_str, (char*)&val, sizeof(val), my_private_server) && val)  // && patching error on console: 0 port was stored for the default port number
    wb_tcpip_port.friendly_save(val);
 // load values form the property table:
  t_index_interval_itertor iii(cdp);
  t_proptab_primary_key start(sqlserver_property_name, NULLSTRING, NONESHORT), 
                        stop (sqlserver_property_name, "ZZZZZZ", 0x7fff);
  if (!iii.open(PROP_TABLENUM, 0, &start, &stop)) 
    return;
  char name[36+1], stopname[36+1] = "";
  do
  { trecnum rec=iii.next();
    if (rec==NORECNUM) break;
   // take the name:
    tb_read_atr(cdp, iii.td, rec, PROP_NAME_ATR, name);
   // search the name:
    t_property_descr * it = the_sp.find(name);
    if (it && stricmp(name, stopname))
    { char buf[255+1];  sig16 num;
      tb_read_atr(cdp, iii.td, rec, PROP_NUM_ATR, (tptr)&num);
      tb_read_atr(cdp, iii.td, rec, PROP_VAL_ATR, buf);
      it->string_to_mem(buf, num);
      if (*buf) *stopname=0;
      else strcpy(stopname, name);
    }
  } while (TRUE);
  iii.close();
 // add the externally stored value of HTTP_tunnel_port and web_port
  if (!GetDatabaseString(server_name, http_tunnel_port.name, (char*)&val, sizeof(val), my_private_server)) val=80;
  http_tunnel_port.friendly_save(val);
  if (!GetDatabaseString(server_name, web_port.name, (char*)&val, sizeof(val), my_private_server)) val=80;
  web_port.friendly_save(val);
#ifdef WINS
 // add hidden values: 
  HKEY hNewKey;
  if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Software602\\602SQL\\Properties", 0, KEY_QUERY_VALUE, &hNewKey) == ERROR_SUCCESS) 
  { DWORD type, namesize, datasize;  tobjname valname;  char valdata[300];
    int ind=0;
    while (true)
    { namesize=sizeof(valname);  datasize=sizeof(valdata);
      if (RegEnumValue(hNewKey, ind++, valname, &namesize, NULL, &type, (BYTE*)valdata, &datasize) != ERROR_SUCCESS) 
        break;
      t_property_descr * it = the_sp.find(name);
      if (it)
        it->string_to_mem(valdata, 0);
    }    
    RegCloseKey(hNewKey);
  }  
#endif  
// adding the module directory to the list of dll directories:
#ifndef LINUX /* Na Linuxu jsou binarky v PREFIX/bin, knihovny jinde */
  char direc[MAX_PATH];
  GetModuleFileName(NULL, direc, sizeof(direc));
  direc[get_last_separator_pos(direc)]=0;
  dll_directory_list.add_to_list(direc);
#endif
 // converting protocol string to a bitmap:
#if WBVERS<900
  reqProtocol = 0;
  Upcase(reqProtocolStr);
  if (strstr(reqProtocolStr, "IPX"))     reqProtocol |= IPXSPX;
  if (strstr(reqProtocolStr, "NETBIOS")) reqProtocol |= NETBIOS;
  if (strstr(reqProtocolStr, "TCP/IP"))  reqProtocol |= TCPIP;
#else
  reqProtocol = TCPIP;
#endif
 // updating server configuration according to the changed properties:
  cdp->sqloptions = the_sp.default_SQL_options.val();
}

int hex2bin(char c)
{ if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='F') return c-'A'+10;
  if (c>='a' && c<='f') return c-'a'+10;
  return -1;
}

int hex2bin_str(const char * str, uns8 * bin, int maxsize)
// [maxsize] and returned size is in bytes.
{ int size=0;
  while (*str)
  { if (size++>=maxsize) return -1;
    int h=hex2bin(*str);  str++;
    if (h<0) return -1;
    *bin=16*h;
    h=hex2bin(*str);  str++;
    if (h<0) return -1;
    *bin+=h;  bin++;
  }
  return size;
}

void t_server_profile::get_top_CA_uuid(WBUUID uuid)
{ if (hex2bin_str(top_CA_str, uuid, UUID_SIZE) != UUID_SIZE)
    memset(uuid, 0, UUID_SIZE);
}

BOOL return_str(cdp_t cdp, unsigned buffer_length, const char * strval)
// Returning the string value, with a length limit
{ if (!buffer_length) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
  unsigned len=(unsigned)strlen(strval);
  if (len>=buffer_length) len=buffer_length-1;
  tptr pans = cdp->get_ans_ptr(sizeof(uns32)+len+1);
  if (!pans) return FALSE;
  *(uns32*)pans=len+1;  pans+=sizeof(uns32);
  memcpy(pans, strval, len);
  pans[len]=0;
  return TRUE;
}

BOOL t_property_descr::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
// Stores the value in the database
{ t_index_interval_itertor iii(cdp);
  t_proptab_primary_key start(owner, name, num), 
                        stop (owner, name, num);
  if (!iii.open(PROP_TABLENUM, 0, &start, &stop)) 
    return FALSE;
  trecnum rec=iii.next();
  iii.close_passing();
  BOOL res = TRUE;
  if (rec==NORECNUM) 
    rec=tb_new(cdp, iii.td, FALSE, NULL);
  if (rec==NORECNUM) res=FALSE;
  else
  { tb_write_atr(cdp, iii.td, rec, PROP_NAME_ATR,  name);
    tb_write_atr(cdp, iii.td, rec, PROP_OWNER_ATR, owner);
    tb_write_atr(cdp, iii.td, rec, PROP_NUM_ATR,   &num);
    tb_write_atr(cdp, iii.td, rec, PROP_VAL_ATR,   value);
  }
  return res;   
}

BOOL t_property_descr::load_to_memory(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buf)
// General searching and reading the property value
{ t_index_interval_itertor iii(cdp);
  t_proptab_primary_key start(owner, name, num), 
                        stop (owner, name, num);
  if (!iii.open(PROP_TABLENUM, 0, &start, &stop)) 
    return FALSE;
  trecnum rec=iii.next();
  if (rec!=NORECNUM) 
    tb_read_atr(cdp, iii.td, rec, PROP_VAL_ATR, buf);
  return rec!=NORECNUM;   
}

void t_property_descr::get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length)
{ char buf[256+1];
  if (!load_to_memory(cdp, sp, owner, num, buf))
    *buf=0;
  strmaxcpy(buffer, buf, buffer_length);
}

BOOL t_property_descr::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
// General searching, reading and returning the property value
{ char buf[256+1];
  if (!load_to_memory(cdp, sp, owner, num, buf))
    *buf=0;
  return return_str(cdp, buffer_length, buf); 
}

BOOL t_property_descr_int::string_to_mem(const char * strval, uns32 num)
{ sig32 auxval;
  if (!str2int(strval, &auxval)) return FALSE;
  ivalue = auxval==NONEINTEGER ? default_value : auxval;
  return TRUE;
}   

BOOL t_property_descr_int::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
// Stores the value in the database and in the memory
// Empty string is stored as empty, but the memory value becomes the default value.
{ sig32 ival;  
  if (*value==' ') value++;
  if (!*value) 
    ival=default_value;
  else
    if (!str2int(value, &ival)) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
 // store in the database
  if (!t_property_descr::set_and_save(cdp, sp, owner, 0, value, vallen))
    return FALSE;
 // store in memory:
  ivalue=ival;
 // reaction:
  change_notification(cdp);
  return TRUE;
}

BOOL t_property_descr_int_ext::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
// Stores the value in the registry and in the memory
{ sig32 ival;  
  if (*value==' ') value++;
  if (!*value) 
    ival=default_value;
  else
    if (!str2int(value, &ival)) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
 // store in the registry:
  if (!SetDatabaseNum(server_name, name, ival, my_private_server))
    return FALSE;
 // store in memory:
  ivalue=ival;
 // reaction:
  change_notification(cdp);
  return TRUE;
}

void t_property_descr_int::get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length)
{ if (buffer_length>=12) int2str(ivalue, buffer); }

BOOL t_property_descr_int::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
// Returns the value stored in memory
{ char numbuf[12];  
  get_and_copy(cdp, sp, owner, num, numbuf, sizeof(numbuf));
  return return_str(cdp, buffer_length, numbuf); 
}

BOOL t_property_descr_str::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
{ if (strlen(value) >= length)
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
 // store in the database
  if (!t_property_descr::set_and_save(cdp, sp, owner, 0, value, vallen))
    return FALSE;
 // store in memory:
  strcpy(strvalue, value);
 // reaction:
  change_notification(cdp);
  return TRUE;
}

void t_property_descr_str::get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length)
{ strmaxcpy(buffer, strvalue, buffer_length); }

BOOL t_property_descr_str::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
// Returns the value stored in memory
{ return return_str(cdp, buffer_length, strvalue); } 


BOOL t_property_descr_str_hdn::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
{ if (strlen(value) >= length)
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
 // store in the hidden place
#ifdef WINS
  DWORD Disposition;  HKEY hNewKey;
  if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Software602\\602SQL\\Properties", 0, "", REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS_EX, NULL, &hNewKey, &Disposition) != ERROR_SUCCESS) 
      { request_error(cdp, NO_CONF_RIGHT);  return FALSE; }
  LONG res=RegSetValueEx(hNewKey, name, 0, REG_SZ, (const BYTE*)value, (int)strlen(value)+1);
  RegCloseKey(hNewKey);
  if (res != ERROR_SUCCESS) 
    { request_error(cdp, NO_CONF_RIGHT);  return FALSE; }
#else
#endif 
 // store in memory:
  strcpy(strvalue, value);
 // reaction:
  change_notification(cdp);
  return TRUE;
}

void t_property_descr_str_hdn::get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length)
{ *buffer=0; }

BOOL t_property_descr_str_hdn::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
// Returns the value stored in memory
{ return return_str(cdp, buffer_length, ""); } 


#ifdef WINS
bool RegRenameKey(char * key, char * old_name, const char * new_name)
{ int len=(int)strlen(key);  HKEY hOldKey, hNewKey;
  strcat(key, old_name);
  if (RegOpenKeyEx(my_private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hOldKey) != ERROR_SUCCESS) return false;
  DWORD Disposition;  int ind;
  strcpy(key+len, new_name);
  if (RegCreateKeyEx(my_private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, "", REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS_EX, NULL, &hNewKey, &Disposition) != ERROR_SUCCESS) goto error1;
  ind=0;
  do
  { DWORD type, namesize, datasize;  char valname[50];  BYTE valdata[300];
    namesize=sizeof(valname);  datasize=sizeof(valdata);
    if (RegEnumValue(hOldKey, ind++, valname, &namesize, NULL, &type,
      valdata, &datasize) != ERROR_SUCCESS) break;
    if (RegSetValueEx(hNewKey, valname, 0, type, valdata, datasize))
      goto error2;
  } while (true);
  RegCloseKey(hNewKey);
  RegCloseKey(hOldKey);
  RegDeleteKey(my_private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key);
  return true;

 error2:
  RegCloseKey(hNewKey);
  RegDeleteKey(my_private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key);
 error1:
  RegCloseKey(hOldKey);
  return false;
}
#endif

BOOL t_property_descr_str_srvname::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
// The problem with renaming the server: Local clients see the updated name from the registry but the objects for 
//   the direct connection are not updated and the clients cannot connect directy until the server restarts.
{ if (strlen(value) >= length)
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
  //tobjname new_server_name;  strcpy(new_server_name, value);  Upcase(new_server_name);  -- preserving the case
 // store in the environment (registry, /etc):
 // check for the existence of the name:
  if (IsServerRegistered(value))
    { SET_ERR_MSG(cdp, "<SRVTAB>");  request_error(cdp, KEY_DUPLICITY);  return FALSE; }
#ifdef WINS
  char key[150];
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");
  if (!RegRenameKey(key, strvalue, value))
    { request_error(cdp, OS_FILE_ERROR);  return FALSE; }
#else
  if (!RenameSectionInPrivateProfile(strvalue, value, my_private_server ? locconffile : configfile))  // cannot write or something like this
    { request_error(cdp, OS_FILE_ERROR);  return FALSE; }
#endif
 // store in memory:
  strcpy(strvalue, value); // do not change it now, clients would freeze when connecting to this server directly with the old name
 // store in the FIL:
  { table_descr_auto srvtab_descr(cdp, SRV_TABLENUM);
    if (srvtab_descr->me()) 
      tb_write_atr(cdp, srvtab_descr->me(), 0, SRV_ATR_NAME, value); 
  }
  return TRUE;
}

BOOL t_property_descr_str_multi::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
// Save to the database. If saving empty string, reload all values to memory.
{// store in the database:
  if (!t_property_descr::set_and_save(cdp, sp, owner, num, value, vallen))
    return FALSE;
 // reload values:
  if (!*value)
  { set_default();
    int anum=1;  char buf[255+1];
    while (load_to_memory(cdp, sp, owner, anum++, buf) && *buf)
      add_to_list(buf);
  }
  return TRUE;
}

BOOL t_property_descr_str_multi::string_to_mem(const char * strval, uns32 num)
{ add_to_list(strval);  // check for empty string is inside
  return TRUE;
}

void t_property_descr_str_multi::add_to_list(const char * direc)
// Adds the directory to the value list. The final PATH_SEPARATOR is not added, if present.
// The case is preserved! (Originally it used to be converted to upper case on Windows)
{ unsigned len = list.count();
  unsigned ll=(unsigned)strlen(direc);
  if (!ll) return;  // bad directory, ignored
  if (direc[ll-1]==PATH_SEPARATOR) ll--;
  if (!ll) return;  // bad directory, ignored
  if (list.acc(len+ll) != NULL)  // unless memory allocation error
  { tptr dest = (tptr)list.acc0(len);
    memcpy(dest, direc, ll);  dest[ll]=0;
  }
}

/* Parameter: the last result of this function or NULL for the 1st string.
   Return value: pointer to the next string or NULL when no more atrings are available. */
const char * t_property_descr_str_multi::get_next_string(const char *old)
{
  if (!old) old=(const char *)list.acc0(0); // Must not return immediately: [old] may be NULL now
  else old+=strlen(old)+1;
  if (old < (const char*)list.acc0(list.count())) return old;  // existing string
  return NULL;
}

BOOL t_property_descr_str_multi::is_in_list(const char * str) const 
// Returns TRUE iff the [str] is in the dll_directory_list. [str] must not be terminated by PATH_SEPARATOR.
{ int i=0;
  while (i<list.count())
  { const char * adir = (tptr)list.acc0(i);
#ifdef WINS
    if (!stricmp(str, adir)) return TRUE;
#else
    if (!strcmp(str, adir)) return TRUE;
#endif
    i+=(int)strlen(adir)+1;
  }
  return FALSE;
}

void t_property_descr_str_multi::get_and_copy(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length)
{ const char * str = get_next_string(NULL);
  while (num--)
  { str = get_next_string(NULL);
    if (str==NULL) { *buffer=0;  return; }
  }
  strmaxcpy(buffer, str, buffer_length); 
}

void t_property_descr_filcrypt::get_and_copy(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length)
{ buffer[0]='0'+header.fil_crypt;  buffer[1]=0; }

BOOL t_property_descr_filcrypt::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
// Returns the current value 
{ char buf[2];  buf[0]='0'+header.fil_crypt;  buf[1]=0;
  return return_str(cdp, buffer_length, buf); 
}

BOOL t_property_descr_filcrypt::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
{ t_fil_crypt new_fil_crypt = (t_fil_crypt)(*value-'0');
  const char * new_password = value+1;
  if (new_fil_crypt > fil_crypt_slow)
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
  if (new_fil_crypt > fil_crypt_simple && !*new_password)  // password is required but it is empty
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
  if (dir_lock_kernel(cdp, true)) 
    return FALSE;
  if (new_fil_crypt!=acrypt.o.current_crypt_type() || new_fil_crypt>fil_crypt_simple)
  { ProtectedByCriticalSection cs(&ffa.remap_sem);
    header.fil_crypt = (uns8)new_fil_crypt;
    ffa.write_header();
    if (new_fil_crypt==fil_crypt_simple) new_password=def_password;
    acrypt.o.set_crypt(new_fil_crypt, new_password);
    crypt_changed(cdp);
    acrypt.i.set_crypt(new_fil_crypt, new_password);
  }
  dir_unlock_kernel(cdp);
  return TRUE;
}

BOOL t_property_descr_srvkey::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
// Returns the current value 
{ if (!buffer_length) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
  if (!cdp->prvs.is_secur_admin) { request_error(cdp, NO_RIGHT);  return FALSE; }
  unsigned len=header.srvkeylen;
  if (len>buffer_length) len=buffer_length;
  tptr pans = cdp->get_ans_ptr(sizeof(uns32)+len);
  if (!pans) return FALSE;
  *(uns32*)pans=len;  pans+=sizeof(uns32);
  return !hp_read(cdp, &header.srvkey, 1, 0, len, pans);
}

BOOL t_property_descr_srvuuid::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
// Returns the current value 
{ if (buffer_length<UUID_SIZE) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
  tptr pans = cdp->get_ans_ptr(sizeof(uns32)+UUID_SIZE);
  if (!pans) return FALSE;
  *(uns32*)pans=UUID_SIZE;  pans+=sizeof(uns32);
  memcpy(pans, header.server_uuid, UUID_SIZE);
  return TRUE;
}

t_Certificate * get_server_certificate(cdp_t cdp, trecnum recnum)
{ uns32 len;  t_Certificate * cert = NULL;
  table_descr_auto srvtab_descr(cdp, SRV_TABLENUM);
  if (srvtab_descr->me()) 
  { if (!tb_read_atr_len(cdp, srvtab_descr->me(), recnum, SRV_ATR_PUBKEY, &len) && len)  // certificate exists
    { unsigned char * buf=(unsigned char *)corealloc(len, 94);
      if (!buf) request_error(cdp,OUT_OF_KERNEL_MEMORY);  
      else 
      { if (!tb_read_var(cdp, srvtab_descr->me(), recnum, SRV_ATR_PUBKEY, 0, len, (tptr)buf))
        { len=cdp->tb_read_var_result;
          cert = make_certificate(buf, len);
        }
        corefree(buf);
      }
    }
  }
  return cert;
}

BOOL t_property_descr_srvuuid::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
{
  memcpy(header.server_uuid, value, UUID_SIZE);
  { table_descr_auto srvtab_descr(cdp, SRV_TABLENUM);
    if (!srvtab_descr->me()) return TRUE;
    tb_write_atr(cdp, srvtab_descr->me(), 0, SRV_ATR_UUID, header.server_uuid); // UUID
  }  
  ffa.write_header();
  commit(cdp);  // must not be rolled back!
  return TRUE;
}

BOOL t_property_descr_srvkey::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
{// check the consistency with the certificate
  t_Certificate * cert = get_server_certificate(cdp, 0);
  if (!cert) { request_error(cdp, INTERNAL_SIGNAL);  return FALSE; }
  CryptoPP::InvertibleRSAFunction * priv = make_privkey((const unsigned char *)value, vallen);
  if (!priv)
    { DeleteCert(cert);  request_error(cdp, INTERNAL_SIGNAL);  return FALSE; }
  BOOL comp=compare_keys(cert, priv);
  DeleteCert(cert);  DeletePrivKey(priv);
  if (!comp) { request_error(cdp, INTERNAL_SIGNAL);  return FALSE; }
  { ProtectedByCriticalSection cs(&ffa.remap_sem);
    if (hp_write(cdp, &header.srvkey, 1, 0, vallen, value, &cdp->tl)) return FALSE;
    header.srvkeylen=vallen;
    ffa.write_header();
  }
  commit(cdp);  // must not be rolled back!
  return TRUE;
}

BOOL t_property_descr_licnum::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
#if WBVERS>=900
{ if (buffer_length<MAX_LICENCE_LENGTH+1) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
  tptr pans = cdp->get_ans_ptr(sizeof(uns32)+MAX_LICENCE_LENGTH+1);
  if (!pans) return FALSE;
  *(uns32*)pans=MAX_LICENCE_LENGTH+1;                                          
  memcpy(pans+sizeof(uns32), ServerLicenceNumber, MAX_LICENCE_LENGTH+1);
  return TRUE;
}
#else  
{ request_error(cdp, ERROR_IN_FUNCTION_ARG);  
  return FALSE; 
}
#endif

BOOL t_property_descr_licnum::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
{ 
#if WBVERS<1100
  if (!check_lcs(value, installation_key))
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
#if WBVERS>=900
  if (!store_server_licence(value))
    { request_error(cdp, NO_RIGHT);  return FALSE; }
 // update the state of the server immediately:
  strcpy(ServerLicenceNumber, value);
  network_server_licence=true;
  trial_lics_added=false;  // importatnt for the Licence Information Dialog
#else
  if (valid_addon_prefix(value))  
  { if (find_licence_number(section_client_access, value)) 
      { SET_ERR_MSG(cdp, "<Licences>");  request_error(cdp, KEY_DUPLICITY);  return FALSE; }
    if (!store_licence_number(section_client_access, value))
      { request_error(cdp, NO_RIGHT);  return FALSE; }
    int max_clients;
    if (get_client_licences(installation_key, NULL, &max_clients))
      client_licences.update_total_lics(max_clients);
  }
  else if (!memcmp(value, "SQ1", 3))
  { if (insert_server_licence(value))
      strcpy(ServerLicenceNumber, value);
  }
  else
    { request_error(cdp, NO_RIGHT);  return FALSE; }
#endif
#endif
  return TRUE;
}

#if WBVERS<900
// just for updating the GCR by the client - not used any more
BOOL t_property_descr_gcr::set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen)
{ 
  if (memcmp(value, header.gcr, ZCR_SIZE) < 0)
    { request_error(cdp, INTERNAL_SIGNAL);  return FALSE; }
  update_gcr(cdp, value);
  return TRUE;
}

BOOL t_property_descr_gcr::get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length)
// Returns the current value 
{ if (buffer_length<ZCR_SIZE) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return FALSE; }
  tptr pans = cdp->get_ans_ptr(sizeof(uns32)+ZCR_SIZE);
  if (!pans) return FALSE;
  *(uns32*)pans=ZCR_SIZE;  
  memcpy(pans+sizeof(uns32), header.gcr, ZCR_SIZE);
  return TRUE;
}
#endif

void get_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, char * value, unsigned buffer_length)
{ t_property_descr * it;
  if (!stricmp(owner, sqlserver_property_name) && (it = the_sp.find(name)))
    it->get_and_copy(cdp, &the_sp, owner, num, value, buffer_length); // Returns the value stored in memory
  else  // Loads and returns the value
  { t_property_descr ait(name, FALSE);
    ait.get_and_copy(cdp, &the_sp, owner, num, value, buffer_length);
  }
}

void get_nad_return_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, unsigned buffer_length)
{ t_property_descr * it;
  if (!stricmp(owner, sqlserver_property_name) && (it = the_sp.find(name)))
    it->get_and_return(cdp, &the_sp, owner, num, buffer_length); // Returns the value stored in memory
  else  // Loads and returns the value
  { t_property_descr ait(name, FALSE);
    ait.get_and_return(cdp, &the_sp, owner, num, buffer_length);
  }
}

int set_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, const char * value, uns32 valsize)
// Returns 0 iff OK or the error code
{ t_property_descr * it;
  if (!stricmp(owner, sqlserver_property_name) && (it = the_sp.find(name)))
  { if (it->security_related)
    { if (!cdp->prvs.is_secur_admin) return NO_RIGHT;
    }
    else
    { if (!cdp->prvs.is_conf_admin) return NO_CONF_RIGHT;
    }
    it->set_and_save(cdp, &the_sp, owner, num, value, valsize);
  }
  else // just stores the value in the database
  { t_property_descr ait(name, FALSE);
    ait.set_and_save(cdp, &the_sp, owner, num, value, valsize);
  }
  return 0;
}
//////////////////////////////// trace /////////////////////////////////////////

class t_message_cache  // caches the recent messages in a log
{ enum { terminator=1 };    // is after the 0 of the last message in [buf]
  unsigned bufsize;         // the real [buf] size, may be 0 when not caching
  char * buf;               // contains messages separated by 0 byte, additional 0 byte after the last message in the buf
  int top, bottom;          // indicies to buf, top == start of the oldest message,
                            //                  bottom == start of the free space after the most recent message 
  int locked;               // enumerating messages in progress, must not add a message
  int last_message_ptr;     // points to the beginning of the most recent message unless empty (top==bottom)
 public:
  t_message_cache(void) 
  { top=bottom=0;  locked=0;  
    bufsize=the_sp.trace_log_cache_size.val();  buf=(char*)corealloc(bufsize, 82); 
    if (!buf) bufsize=0;
  }
  ~t_message_cache(void) 
    { corefree(buf); }
  bool reset_log_cache_size(void)
  { unsigned new_bufsize=the_sp.trace_log_cache_size.val();  
    if (new_bufsize<=bufsize) return true;  // making the buffer smaller is not programmed
    char * newbuf = (char*)corealloc(new_bufsize, 82); 
    if (!newbuf) return false;
    memcpy(newbuf, buf, bufsize);
    corefree(buf);  buf=newbuf;  bufsize=new_bufsize;
    return true;
  }
  const char * enum_messages(int & pos);
  const char * get_last_message(void) const;
  void add_message(const char * msg);
  inline void mark(void)
    { if (buf) ::mark(buf); }
};

struct t_trace_log;

static t_trace_log * trace_log_list = NULL;  // protected by [cs_trace_logs]

struct t_trace_log /////////////////////////////////////////////////////////////////////////////
{ enum { max_args=10 };
  tobjname log_name;
  FHANDLE hTraceLog;
  t_message_cache message_cache;
  char format[MAX_LOG_FORMAT_LENGTH+1];
  uns32 log_date; 
  char pathname[MAX_PATH];
  int date_added_at;  // index into pathname, -1 if not added
  uns32 last_flush;   // prevents too frequent flushing of the basic log
  unsigned repetition_count;
  t_trace_log * next;

  t_trace_log(const char * log_nameIn, const char * formatIn, const char * pathnameIn);
  BOOL open_log_file(void);
  void close_log_file(void);
  void get_error_envelope(cdp_t cdp, char * buf, unsigned bufsize, unsigned trace_type, const char * err_msg);
  void output(const char * xbuf);
  inline void mark(void)
    { ::mark(this);  message_cache.mark(); }
  void set_format(const char * formatIn);
#ifdef LINUX
private:
  int is_system; /* contains -1 if is not echoed to system log, log level otherwise */
#endif
};

/////////////////////////////////// creating server objects ///////////////////////////////////
BOOL exec_direct(cdp_t cdp, char * stmt, t_value * resptr)
{ sql_statement * so = sql_submitted_comp(cdp, stmt);
  if (so==NULL) return TRUE;
  if (so->statement_type==SQL_STAT_CALL)
    ((sql_stmt_call*)so)->result_ptr = resptr;
  sql_exec_top_list(cdp, so);
  delete so;
  return FALSE;
}

// Order of values must be the same as the initialisation order!
t_server_init init_counter = SERVER_INIT_NO;  // server initialisation progress
bool network_server = false;  // networking started in kernel_init()
bool minimal_start = false;   // when true, startup procedures are not performed
bool console_disabled = false;  // for short-term disabling the console output

/* Server startup and shutdown:
Server se startuje pomoci kernel_init, ukoncuje se pomoci dir_kernel_close. dir_kernel_close se vola taky pri neuspechu uvnitr kernel_init.
Promenna init_counter rika jak daleko postoupilo startovani serveru a tedy co vsechno se musi provest pri ukoncovani.
Postup startovani (postup ukoncovani je zhruba opacny):
- inicializace globalnich promennych
- nalezeni jmena serveru a databazoveho souboru
- natazeni parametru instalace a databaze
- vytvoreni zakladnich objektu, ktere se pri shutdown musi zrusit
- registrace cdp serveru v cds
- inicializace cdp serveru
- dalsi postupne inicializace struktur serveru
- trigger _on_server_start
- start sluzeb: protokoly a nabizeni sluzeb v siti, pripojovani lokalnich klientu, replikace
- zavreni filu pokud se ma zavirat bez klientu.
*/

int restore_database_file(const char * path)
{ ffa.preinit(path, server_name);
  if (!*server_name) return KSE_NO_FIL;   // server not registered
  return ffa.restore();
}

bool get_own_host_name(char * name, int buffer_size)
{ char hostname[150];
  if (!gethostname(hostname, sizeof(hostname)))
  { struct hostent * he = gethostbyname(hostname);
    if (he && he->h_aliases[0])
    { strmaxcpy(name, he->h_aliases[0], buffer_size);
      return true;
    }
  }
  return false;
}

#include "win32ux.cpp"
#if WBVERS<1100
#include "licence8.cpp"
#endif
static const char IK_WRITE_ERROR[] = "Error storing the Installation Key to the registry";
char default_log_format[MAX_LOG_FORMAT_LENGTH+1];
#define DEFAULT_LOG_FORMAT "%d %T %s %u %m"

int WINAPI kernel_init(cdp_t cdp, const char * path, bool networking)
// Routine starting the server. Core management is supposed to be opened outside.
// networking==true iff forcing the network support (e.g. when server started with the /n parameter)
// path is always non-empty
{ int res;  BOOL fil_closed_properly;  int i;
  char msg[150+OBJ_NAME_LEN+7];  // message buffer
  if (kernel_is_init) return KSE_OK;
  server_start_time = time(NULL);
  init_counter = SERVER_INIT_NO;
 // init variables:
  compiler_init();   // the kernel compiler initialization
  fShutDown = fLocking = FALSE;
#ifdef WINS
  server_process_id=GetCurrentProcessId();
  { TIME_ZONE_INFORMATION tzi;
    DWORD res = GetTimeZoneInformation(&tzi);
    server_tzd=-(60*(res==TIME_ZONE_ID_DAYLIGHT ? tzi.Bias+tzi.DaylightBias : tzi.Bias));
  }
#else
  server_process_id=getpid();
  { struct timeval tv;  struct timezone tz;
    gettimeofday(&tv, &tz);
    server_tzd=-(60*tz.tz_minuteswest);
  }
#endif
  wbt_construct();  // inits the static dynar, should not be necessary
  the_sp.set_all_to_defaut();  // set all the server properties to theirs default values
 // find database file paths and server name:
  ffa.preinit(path, server_name);
  if (!*server_name)        return KSE_NO_FIL;   // server not registered
 // check exclusive access to the database file, create it, if it does not exist:
  res=ffa.check_fil();
  if (res!=KSE_OK)          return res;          // KSE_DBASE_OPEN or KSE_CANNOT_CREATE_FIL
 // creating server objects:
  InitializeCriticalSection(&cs_tables);
  CreateLocalManualEvent(FALSE, &hTableDescriptorProgressEvent);
  CreateLocalManualEvent(FALSE, &hLockOpenEvent);
  CreateLocalManualEvent(TRUE,  &Zipper_stopped);
  InitializeCriticalSection(&cs_deadlock);
  InitializeCriticalSection(&privil_sem);
  InitializeCriticalSection(&commit_sem);
  InitializeCriticalSection(&crs_sem);
  InitializeCriticalSection(&cs_sequences);
  InitializeCriticalSection(&cs_client_list);
  InitializeCriticalSection(&cs_trace_logs);
  InitializeCriticalSection(&cs_short_term);
  InitializeCriticalSection(&cs_medium_term);
  InitializeCriticalSection(&cs_profile);
  InitializeCriticalSection(&cs_gcr);
#ifdef WINS
  InitializeCriticalSection(&cs_ftx_msgsupp_load);
#endif
  ffa.init();
#if defined (WINS) && defined(_DEBUG) && !defined(X64ARCH)
 // prepare crash information:
  stackwalk_prepare();  // must be called after initializing paths in ffa
#endif
  init_counter = SERVER_INIT_OBJECTS;
 // default trace log:
  cdp->prvs.is_conf_admin=TRUE;  // used below
  GetDatabaseString(server_name, "DefaultLogFormat", default_log_format, sizeof(default_log_format), my_private_server);
  if (!*default_log_format) strcpy(default_log_format, DEFAULT_LOG_FORMAT);
  define_trace_log(cdp, NULLSTRING, NULLSTRING, NULLSTRING);  // opens the default log, if log directory defined, uses server_name
  trace_def(cdp, TRACE_SERVER_FAILURE, NULLSTRING, NULLSTRING, NULLSTRING, 3+1000);  // defines the initial state of tracing
  trace_def(cdp, TRACE_LOG_WRITE,      NULLSTRING, NULLSTRING, NULLSTRING, 1+1000);  // defines the initial state of tracing
  trace_def(cdp, TRACE_SERVER_INFO,    NULLSTRING, NULLSTRING, NULLSTRING, 1+1000);  // defines the initial state of tracing
  trace_def(cdp, TRACE_START_STOP,     NULLSTRING, NULLSTRING, NULLSTRING, 1+1000);  // defines the initial state of tracing
 // write initial messages to the log:
  trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), serverStartDelimiter), NOOBJECT);
 // read the installation key (written to the log):                                
  *installation_key=0;
#if WBVERS<900
  GetDatabaseString(server_name, database_ik, installation_key, sizeof(installation_key), my_private_server);
  if (!stricmp(installation_key, "NEW"))  // used on Linux
  { create_ik(installation_key, ffa.first_fil_path());
    if (!SetDatabaseString(server_name, database_ik, installation_key, my_private_server))
      dbg_err(IK_WRITE_ERROR);
  }
  else
  { if (!verify_ik(installation_key, ffa.first_fil_path()))
      *installation_key=0;  // wrong IK
    if (memcmp(installation_key, "SA1", 3))  // IK not specified or invalid specification
      if (!get_any_valid_ik(installation_key, ffa.first_fil_path()))  // just for the older 8.0 databases
        *installation_key=0;
      else  // save the valid inst. key, used on Linux for the initial database and for databases created without inst. key
        if (!SetDatabaseString(server_name, database_ik, installation_key, my_private_server))
          dbg_err(IK_WRITE_ERROR);
  }
#endif  // WBVERS
  trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), running_as_service ? serverStartMsgService : serverStartMsg, server_name, VERS_STR), NOOBJECT);

  init_counter = SERVER_INIT_TRACE_LOG;
#ifdef WINS
  make_security_descr();  // must be before creating hKernelShutdownEvent
  init_counter = SERVER_INIT_SECURDESCR;
#endif
  CreateLocalAutoEvent(FALSE, &hKernelShutdownReq);
#ifdef WINS
  hKernelShutdownEvent=CreateEvent(&SA, TRUE, FALSE, NULL); // manual, not signalled
  hKernelLockEvent    =CreateEvent(&SA, TRUE, FALSE, NULL); // manual, not signalled
  if (!hKernelShutdownEvent || !hKernelLockEvent) { res=KSE_SYNCHRO_OBJ;  goto err; }
#endif
#ifdef UNIX
  //KernelShutdownEvent and hKernelLockEvent have a static initialization
#endif
#ifdef NETWARE
  hKernelShutdownEvent=OpenLocalSemaphore(0);  // error not possible according to the NW docs
#endif
  init_counter = SERVER_INIT_DOWN_EVENT;
 // registering server thread:
  res=AddTask(cdp);
  if (res!=KSE_OK)                                                            goto err;  // KSE_CANNOT_OPEN_FIL, KSE_CANNOT_OPEN_TRANS
  init_counter = SERVER_INIT_TASK;

  if (kernel_cdp_init(cdp)) // initializes the answer (necessary on errors) 
                                                        { res=KSE_NO_MEMORY;  goto err; }
  init_counter = SERVER_INIT_CDP;
  
  res=ffa.disc_open(cdp, fil_closed_properly);   // overwrites header.sezam_open
  if (res!=KSE_OK)                                                            goto err;
  init_counter = SERVER_INIT_FFA;
 // prepare and check system charset:
  init_charset_data();
  if      (header.sys_spec.charset==0)   sys_spec.charset = 1;  // MSW CS for the old database files
  else if (header.sys_spec.charset==255) sys_spec.charset = 0;  // english ASCII for 255 in the header
  else                                   sys_spec.charset = header.sys_spec.charset;  // charset from the header
  if ((sys_spec.charset & 0x7f) >= CHARSET_COUNT)
    { res=KSE_SYSTEM_CHARSET;  goto err; }
  if (!charset_available(sys_spec.charset))
    { res=KSE_SYSTEM_CHARSET;  goto err; }

  if (!frame_management_init(the_sp.kernel_cache_size.val())) { res=KSE_NO_MEMORY;  goto err; }
#ifdef IMAGE_WRITER
  the_image_writer.init();
#endif
  init_counter = SERVER_INIT_FRAME;
  if (jour_init())                                      { res=KSE_NO_MEMORY;  goto err; }
  /* jour_init must be before init_tables which may call commit(); */
  if (init_pieces(cdp, fil_closed_properly))            { res=KSE_NO_MEMORY;  goto err; }
  init_counter = SERVER_INIT_PIECES;
  if (init_tables_1(cdp))                               { res=KSE_DAMAGED;    goto err; }
  if (init_tables_2(cdp))                               { res=KSE_DAMAGED;    goto err; }

  if (!fil_closed_properly) restoring_actions(cdp);
  init_counter = SERVER_INIT_GLOBAL;
 // load server properties:
  the_sp.load_all(cdp);
 // upadte server state according to the loaded properties:
  for (i=0;  i<TRACE_PROPERTY_ASSOC_COUNT;  i++)  // restore the pertistent trace requests
    trace_def(cdp, the_sp.trace_property_assocs[i].situaton, NULLSTRING, NULLSTRING, NULLSTRING, 
                   the_sp.trace_property_assocs[i].prop->val() + 1000);
  cdp->trace_enabled=trace_map(NOOBJECT);
  core_init(the_sp.core_alloc_size.val());
  if (!frame_management_reset(the_sp.kernel_cache_size.val())) { res=KSE_NO_MEMORY;  goto err; }
  trace_log_list->message_cache.reset_log_cache_size();  // *trace_log_list is the basic log - the only log in the moment
  // write server start notification messages again, if they should be in a daily log:
  if (the_sp.DailyBasicLog)
  { console_disabled=true;  // the text has already been written to the console
    trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), serverStartDelimiter), NOOBJECT);
    trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), running_as_service ? serverStartMsgService : serverStartMsg, server_name, VERS_STR, installation_key), NOOBJECT);
    console_disabled=false;
  }
#if WBVERS>=810
 // check space on the volumne containing the database file:
  uns32 space;
  get_server_info(cdp, OP_GI_DISK_SPACE, &space);  // space in kilobytes
  if (space/1024 < the_sp.DiskSpaceLimit.val())
    trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), lowSpaceOnDisc), NOOBJECT);
#endif

 // prepare TCP/IP (makes hostname accessible):
#ifdef WINS
	WORD vers;  vers = 0x0101;   // WINSOCK 1.1
	WSADATA wsa;
	i = WSAStartup(vers, &wsa);  // must be before TcpIpProtocolStart() in NetworkStart()
  if (i)                                                { res=KSE_NO_WINSOCK;  goto err; }
  init_counter = SERVER_INIT_IP_PROTOCOL;
	SOCKET sock;  sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)                           { res=KSE_NO_WINSOCK;  goto err; }
  closesocket(sock);
#endif
 // set the own host name for the HTTP access, when other name id not specified:
  if (!*the_sp.HostName.val())
  { char hostname[100];
    if (get_own_host_name(hostname, sizeof(hostname)))
      the_sp.HostName.friendly_save(hostname);
  }
 // prepare request synchonisation:
  integrity_check_planning.open();
  init_counter = SERVER_INIT_REQ_SYNCHRO;

 // licences:
#ifdef WINS
  server_shared_info.open(&SA);
#else
  server_shared_info.open(NULL);
#endif

  init_counter = SERVER_INIT_LICENCES;
#ifdef STOP  // no more licensing  
#if WBVERS<900
  int max_clients;           // limit on number of connected intranet clients
  if (!*installation_key) max_clients=0;  // licences are related to the IK, cannot use licences without IK
  else
  { if (!get_client_licences(installation_key, NULL, &max_clients))
      { res=KSE_INST_KEY;  goto err; }
    max_clients += get_client_lics_from_OPL_registration(installation_key);
    if (max_clients>0) 
    { fulltext_allowed=true;  // fulltext enabled iff the client licence added
      trace_msg_general(0, TRACE_START_STOP,
        form_message(msg, sizeof(msg), max_clients>=999 ? unlimitedLicences : normalLicences, max_clients), NOOBJECT);
    }
   // if trial licences are still valid, add them:
    int year, month, day;  
    if (get_server_registration(installation_key, ServerLicenceNumber, &year, &month, &day)) // registration has already been checked, now I need the date and type
    { if (!memcmp(ServerLicenceNumber, "SQY", 3))  // CD-database licence: no trial add-ons, but permanent fulltext
        fulltext_allowed=true;
      else  // trial licences: 30 days 5 add-ons, 90 days fulltext
      { reg_date = Make_date(day, month, year);
        reg_date=dt_plus(reg_date, 30);
        if (!max_clients)
        { int remaining_trial_days = dt_diff(reg_date, Today());
          if (remaining_trial_days>=0)
          { max_clients+=5;
            trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), trialLicences, remaining_trial_days), NOOBJECT);
            trial_lics_added=true;
          }
        }
       // fulltext licence:
        if (!fulltext_allowed)
        { 
#if WBVERS>=810
          int diff = dt_diff(reg_date, Today());
#else
          uns32 reg_date2 = dt_plus(reg_date, 60);  // 60 days added above 30
          int diff = dt_diff(reg_date2, Today());
#endif
          if (diff>=0)
          { fulltext_allowed=true;
            trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), trialFTLicence8, diff), NOOBJECT);
            trial_fulltext_added=true;
          }
        }
      }
    }
    max_clients+=1;  // +1 - base server licence
    server_shared_info.add_lics(installation_key, max_clients);
  }
#elif WBVERS<1100  // since 9.0
  if (lic_state==1)
  {// verify date against the creation date of the database file
    reg_date = Make_date(day, month, year);
    if (header.creation_date && header.creation_date<reg_date) reg_date=header.creation_date;
    reg_date=dt_plus(reg_date, 31);
    int remaining_trial_days = dt_diff(reg_date, Today());
    if (remaining_trial_days>=0)
    { trial_lics_added=true;
      network_server_licence=true;  // enables the network access
      trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), trial_valid, remaining_trial_days), NOOBJECT);
    }
    else
      trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), trial_expired), NOOBJECT);
  }
  else if (lic_state==2 || lic_state==3) // server has a valid licence
  { network_server_licence=true;    // enables the network access
    trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), server_licnum, ServerLicenceNumber), NOOBJECT);
    if (lic_state==3) 
      trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), developerLicence), NOOBJECT);
  }
  if (network_server_licence)
    server_shared_info.add_lics(installation_key, 1000);
  else
    server_shared_info.add_lics(installation_key, 1000);  // groupware wants it
#ifdef _DEBUG
  network_server_licence=true;    // enables the network access
#endif    
#else  // WBVERS>=1100
  network_server_licence=true;    // enables the network access
  server_shared_info.add_lics(installation_key, 1000);
  read_addon_licences();
#endif
#endif  // STOP licensing

 // open licensing:
  network_server_licence=true;    // enables the network access
  server_shared_info.add_lics(installation_key, 1000);
#if WBVERS>=1100
  read_addon_licences();
#else
  fulltext_allowed=true;
  xml_allowed=true;
#endif
 // consistency checks:
  ffa.fil_consistency();
  int cache_error, index_error;
  if (!minimal_start) check_all_indicies(cdp, true, INTEGR_CHECK_INDEXES|INTEGR_CHECK_CACHE, false, cache_error, index_error);  // checks indexes on systabs, reports errors to the system log

 // If registered under a name different from name stored in the database file, update the database file.
 // Historically registration name is displayed in the server window and used in the direct communication.
 // In other contexts the stored name is used. I do not want to have two different names.
  if (!minimal_start) 
  { table_descr_auto srvtab_descr(cdp, SRV_TABLENUM);
    if (srvtab_descr->me()) 
    { tobjname stored_server_name;
      tb_read_atr(cdp, srvtab_descr->me(), 0, SRV_ATR_NAME, stored_server_name);
      if (sys_stricmp(stored_server_name, server_name))
      { tb_write_atr(cdp, srvtab_descr->me(), 0, SRV_ATR_NAME, server_name); 
        commit(cdp);
      }
    }
  }
  
  if (!minimal_start) 
    complete_broken_ft_backup(cdp);
 // find auxiliary system objects:
  name_find2_obj(cdp, "_TIMERTAB", "_SYSEXT", CATEG_TABLE, &planner_tablenum);

  kernel_is_init=wbtrue;
 //////////////////////////// from now clients and services can work (when connected) ///////////////////////
#ifdef WINS
 // check for Unicode support:
  if (!CompareStringW(wbcharset_t(sys_spec.charset).lcid(), NORM_IGNORECASE, L"a", 1, L"b", 1))
    dbg_err(noUnicodeSupport);
#endif
 // _on_server_start:
  if (!minimal_start)
  { if (find_system_procedure(cdp, "_ON_SERVER_START"))
    { cdp->request_init();
      cdp->prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, TRUE);  // execute with CONF_ADMIN privils (hierarchy is necessary, I need to be the Admin os _sysext)
      //exec_direct(cdp, "CALL _SYSEXT._ON_SERVER_STOP()");  // for debug only, and when synchronout actions follow
      exec_direct(cdp, "CALL DETACHED _SYSEXT._ON_SERVER_START()");
      cdp->prvs.set_user(DB_ADMIN_GROUP, CATEG_GROUP, FALSE);  // return to DB_ADMIN privils
      cdp->request_init();
    }
    run_proc_in_every_schema(cdp, "_ON_SERVER_START_APPL");
  }
  init_counter = SERVER_INIT_STARTPROC;
 // trace free pieces loaded from the database file:
  trace_free_pieces(cdp, "loaded_free");

  IdentityVerificationStart(server_name);
  init_counter = SERVER_INIT_IDENT_VERIF;
  
 /* starting the network: */
  if (!the_sp.net_comm.val() && !the_sp.http_tunel.val() && !networking)
#ifndef UNIX
    if (!the_sp.ipc_comm.val())
#endif
    { networking=true;
      trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), default_nw_interf), NOOBJECT);
    }
  if (the_sp.net_comm.val() || the_sp.http_tunel.val() || the_sp.web_emul.val() || networking)
  { res = NetworkStart();
    if (res) goto err;
    init_counter = SERVER_INIT_NETWORK;
    network_server = true;  // information for dir_kernel_close only
    res = AdvertiseStart(server_name, networking);
    if (res) goto err;
    init_counter = SERVER_INIT_ADVERTISING;
  }
 // start direct slave threads maker:
#ifndef UNIX
  if (the_sp.ipc_comm.val())
  { if (!THREAD_START_JOINABLE(ThreadMakerThreadProc, 10000, NULL, &hThreadMakerThread))
      { res=KSE_START_THREAD;  goto err; }
  }
  else
  { char nameEvent[35+OBJ_NAME_LEN];
    make_name(nameEvent, "share%d_noipc%s", server_name, 0, true);
    NoIPCEvent = CreateEvent(&SA, FALSE, FALSE, nameEvent);
    if (!NoIPCEvent)
    { make_name(nameEvent, "share%d_noipc%s", server_name, 0, false);
      NoIPCEvent = CreateEvent(&SA, FALSE, FALSE, nameEvent);
    }
  }
  init_counter = SERVER_INIT_DIRMAKER;
#endif

  repl_start(cdp);
  init_counter = SERVER_INIT_REPLIC;
 /////////////////////////////////////// server init completed //////////////////////////////////
  if (the_sp.CloseFileWhenIdle.val())  // close the file access only on successfull startup
    ffa.close_fil_access(cdp); /* open_fil_access called in dir_kernel_close before RemoveTask */
  init_counter = SERVER_INIT_COMPLETE;
  return KSE_OK;

 err:
 // put error message to log:
  if (init_counter >= SERVER_INIT_TRACE_LOG) 
    trace_msg_general(0, TRACE_START_STOP, form_message(msg, sizeof(msg), serverStartFailed, server_name, kse_errors[res]), NOOBJECT);
  back_end_operation_on=FALSE;  // prevents waiting in dir_kernel_close()
  dir_kernel_close();
  return res;
}

void WINAPI dir_kernel_close(void)
/* called from a non-logged application (kernel is initialized) */
{ int i;  cdp_t cdp = cds[0];
  if (init_counter == SERVER_INIT_COMPLETE)
    if (the_sp.CloseFileWhenIdle.val())
      if (ffa.open_fil_access(cdp, server_name)) return;   /* cannot close server if cannot open files */
 // set the server closing event:
  fShutDown=fLocking=TRUE;
  if (init_counter >= SERVER_INIT_DOWN_EVENT)
#ifdef WINS
    SetEvent(hKernelLockEvent);
    SetEvent(hKernelShutdownEvent);
#endif
#ifdef UNIX
    hKernelLockEvent.broadcast();
    hKernelShutdownEvent.broadcast();
#endif
#ifdef NETWARE        
    for (i=0;  i<500;  i++) SignalLocalSemaphore(hKernelShutdownEvent);
#endif
  lock_opened();  // releases the threads waiting for any lock

 ////////////////// release slaves waiting on a breakpoint or being traced or waiting for event:////////////////
  if (init_counter == SERVER_INIT_COMPLETE)   // should be protected by a CS, debuged clients ending now
    for (i=1;  i <= max_task_index;  i++) 
      if (cds[i])
      { if (cds[i]->dbginfo) cds[i]->dbginfo->close();  
        cds[i]->cevs.shutdown();
      }
 ////////////////////////////////////// stopping client's slaves and services ////////////////////////
  if (init_counter >= SERVER_INIT_REPLIC)
    repl_stop();
 // stop direct access thread maker, should terminate almost immediately:
#ifdef WINS
  if (init_counter >= SERVER_INIT_DIRMAKER)
    if (hThreadMakerThread)  // may not have been started if ipc_comm was false on server start
    { WaitForSingleObject(hThreadMakerThread, 5000);
      CloseHandle(hThreadMakerThread);   hThreadMakerThread=0;
    }
    else
    { if (NoIPCEvent) CloseHandle(NoIPCEvent);  NoIPCEvent=0; }
#endif
#ifdef NETWARE
  if (init_counter >= SERVER_INIT_DIRMAKER)
    if (Create1SlaveSemaphore)  // thread maker really running
      SignalLocalSemaphore(Create1SlaveSemaphore);  // stops the thread maker
#endif

 // stop advertising and networking (if networking has been required)
  if (init_counter >= SERVER_INIT_ADVERTISING && network_server) 
    AdvertiseStop(); /* stops advertising and slaves */
  if (init_counter >= SERVER_INIT_ADVERTISING) 
  { if (client_logout_timeout) WarnClientsAndWait(client_logout_timeout);  // wait for disconnection of all clients
    StopSlaves();   // both network and direct slaves have to be stopped
  }
  if (init_counter >= SERVER_INIT_NETWORK     && network_server) NetworkStop(cdp);
  if (init_counter >= SERVER_INIT_IDENT_VERIF) IdentityVerificationStop();

 /////////////////////// _on_server_stop: executed when all clients have been disconnected /////////////////////////
  if (init_counter >= SERVER_INIT_STARTPROC)
  { run_proc_in_every_schema(cdp, "_ON_SERVER_STOP_APPL");
    if (find_system_procedure(cdp, "_ON_SERVER_STOP"))
    { cdp->request_init();
      cdp->prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, TRUE);  // execute with CONF_ADMIN privils (hierarchy is necessary, I need to be the Admin os _sysext)
      exec_direct(cdp, "CALL _SYSEXT._ON_SERVER_STOP()");
      cdp->prvs.set_user(DB_ADMIN_GROUP, CATEG_GROUP, FALSE);  // return to DB_ADMIN privils
      commit(cdp);
      cdp->request_init();
    }
  }
  bCancelCopying = TRUE;  // signal for backups
#ifdef STOP  // some strange problems....
 // wait for back-end operationd to be terminated
  while (back_end_operation_on)  // fShutDown is expected to be TRUE now
    Sleep(200);
#endif    
 // trace free pieces to be saved int the database file:
  trace_free_pieces(cdp, "saved_free");

 ////////////////////////////////////// from now clients cannot work properly ////////////////////////
  kernel_is_init=wbfalse;  /* must not be reset before ServerStop, clients may work 1 minute */
  if (init_counter >= SERVER_INIT_LICENCES)    
  { client_licences.close();     // returns licences owned by dead slaves
    server_shared_info.close();  // closes access to the pool
  }
  if (init_counter >= SERVER_INIT_REQ_SYNCHRO) integrity_check_planning.close();

#ifdef WINS
  if (init_counter >= SERVER_INIT_IP_PROTOCOL)
  	WSACleanup();  // must be after after waiting for other threads to stop in TcpIpProtocolStop(), crashes on NT 4.0
#endif
  if (init_counter >= SERVER_INIT_GLOBAL)
  {// must remove allocations of pages in triggers & cached stored procedures:
    uninst_tables(cdp);
    psm_cache.destroy();
    close_sequence_cache(cdp);
    wbt_close_all_semaphores();  // closing user semaphores not closed by clients
    commit(cdp);
  }

  if (init_counter >= SERVER_INIT_PIECES)      deinit_tables(cdp); // must be called even if init_tables_? fails!
  if (init_counter >= SERVER_INIT_PIECES) 
  { save_pieces(cdp);    // must be done before saving blocks! 
    commit(cdp);         // writes saved pieces
  }
  if (init_counter >= SERVER_INIT_FRAME)       
  { 
#ifdef IMAGE_WRITER
    the_image_writer.deinit();
#endif
    frame_management_close();
  }
  if (init_counter >= SERVER_INIT_FFA)         ffa.disc_close(cdp);
  if (init_counter >= SERVER_INIT_CDP)         kernel_cdp_free(cdp);  // must be: kernel_is_init==wbfalse !!!

  if (init_counter >= SERVER_INIT_CDP)         offline_threads.join_all();  // must be before closing trace logs

  if (init_counter >= SERVER_INIT_TASK)        RemoveTask(cdp);   // closes the database file
 // delete server objects (any running threads will crash!):
  if (init_counter >= SERVER_INIT_DOWN_EVENT)
  {
#ifdef WINS
    CloseHandle(hKernelShutdownEvent);          hKernelShutdownEvent=0; 
    CloseHandle(hKernelLockEvent);              hKernelLockEvent    =0; 
#endif
#ifdef UNIX
    // pthread_cond_destroy(&hKernelShutdownEvent);  pthread_cond_destroy(&hKernelLockEvent);
#endif
#ifdef NETWARE        
    CloseLocalSemaphore(hKernelShutdownEvent);  hKernelShutdownEvent=0;
#endif
    CloseLocalAutoEvent(&hKernelShutdownReq);
  }  
#ifdef WINS
  if (init_counter >= SERVER_INIT_SECURDESCR) free_security_descr();
#endif

  if (init_counter >= SERVER_INIT_TRACE_LOG) 
  { char buf[81], buf2[40];
    const char * reason = down_reason==down_system      ? DR_SYSTEM :
                          down_reason==down_console     ? DR_CONSOLE :
                          down_reason==down_endsession  ? DR_ENDSESSION :
                          down_reason==down_from_client ? DR_FROM_CLIENT :
                          down_reason==down_pwr         ? DR_POWER : DR_ABORT;
#ifdef ENGLISH
    reason = gettext(reason);
#endif    
    if (sys_spec.charset != 0)  // for charset 0 the message remains in UTF-8
      if (superconv(ATT_STRING, ATT_STRING, reason, buf2, t_specif(0, CHARSET_NUM_UTF8, 0, 0), sys_spec, NULL)>=0)
        reason=buf2;
    trace_msg_general(0, TRACE_START_STOP, form_message(buf, sizeof(buf), serverStopMsg, reason), NOOBJECT);
    close_trace_logs();
  }

  if (init_counter >= SERVER_INIT_OBJECTS)
  { ffa.deinit();
    CloseLocalManualEvent(&hTableDescriptorProgressEvent);
    CloseLocalManualEvent(&hLockOpenEvent);
    CloseLocalManualEvent(&Zipper_stopped);
#ifdef WINS
    DeleteCriticalSection(&cs_ftx_msgsupp_load);
#endif
    DeleteCriticalSection(&cs_tables);
    DeleteCriticalSection(&cs_deadlock);
    DeleteCriticalSection(&privil_sem);
    DeleteCriticalSection(&commit_sem);
    DeleteCriticalSection(&crs_sem);
    DeleteCriticalSection(&cs_sequences);
    DeleteCriticalSection(&cs_client_list);
    DeleteCriticalSection(&cs_trace_logs);
    DeleteCriticalSection(&cs_short_term);
    DeleteCriticalSection(&cs_medium_term);
    DeleteCriticalSection(&cs_profile);
    DeleteCriticalSection(&cs_gcr);
  }
  init_counter = SERVER_INIT_NO;
 // core_release(); -- must not call here, core used later to display messages
}

/****************************** locking kernel ******************************/
bool dir_lock_kernel(cdp_t cdp, bool check_users)
// Only CONF_ADMIN may call this funcion.
// if [check_users] then other users must not be connected, otherwise error is raised.
{ 
  if (!check_users)
    { login_locked++;  cdp->my_server_locks++; }
  else
  { int cnt=0;
    { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
      for (int i=1;  i<=max_task_index;  i++) if (cds[i])  
        if (cds[i]->in_use==PT_KERDIR || cds[i]->in_use==PT_SLAVE || cds[i]->in_use==PT_WORKER)
          cnt++;
      if (cnt<=1)  // locking it inside the CS  
        { login_locked++;  cdp->my_server_locks++; }
    }      
    if (cnt > 1)  // anybody else logged
      { request_error(cdp, CANNOT_LOCK_KERNEL);  return true; }  // error outside the CS!
  }
  return false;
}

void dir_unlock_kernel(cdp_t cdp)
{ 
  if (cdp->my_server_locks) 
    { login_locked--;  cdp->my_server_locks--; }
}

#ifdef WINS
CFNC void send_break_req(DWORD who)
/* break request from direct client received */
{ ProtectedByCriticalSection cs(&cs_client_list);
  for (int i=1;  i<=max_task_index;  i++)
		if (cds[i] && cds[i]->in_use==PT_MAPSLAVE)
//    if (cds[i]->LinkIdNumber==who)
		  { cds[i]->break_request=wbtrue;
//      break;
   		}
}
#endif

//////////////////////////////////////////////////////////////////////////////
CFNC cdp_t WINAPI GetCurrTaskPtr(void)
// I hope it is not used. cds not protected here.
{ int i = 0;  t_thread task;
  task = GetClientId ();
  do
  { if (cds[i])
#ifdef UNIX
        if (pthread_equal(cds[i]->threadID, task)) return cds[i];
#else
        if (cds[i]->threadID == task) return cds[i];
#endif
  } while (++i <= max_task_index);
  return (cdp_t)(cdp_t)NULL;
}

void cd_t::cd_init(void) // called in the constructor and by service in OnInit
{ core_init(0);  /* early error messages and initializing locDevNames need it */
  appl_state  = NOT_LOGGED;
  initialized = wbfalse;
  expl_trans  = TRANS_NO;
  ker_flags   = 0;
  waiting     = 0;
  stmts = sel_stmt = NULL;
  wait_type   = WAIT_NO;
  mask_token  = wbfalse;     
#ifndef UNIX
  hSlaveSem   = 0;
#endif
  commenc     = NULL;
  cond_login  = 0;
  memset(sel_appl_uuid, 0, UUID_SIZE);  memset(front_end_uuid, 0, UUID_SIZE);
  sel_appl_name[0]=0;
  current_schema_uuid=sel_appl_uuid;
  applnum = applnum_perm = NOAPPLNUM;
  admin_role=senior_role=junior_role=author_role=NOOBJECT;
  pRemAddr    = NULL;
  rscope = handler_rscope = NULL;
  processed_except_name[0]=except_name[0]=last_exception_raised[0]=0;
  except_type = EXC_NO;
  comp_kontext= NULL;
  mask_compile_error=0;
  isolation_level=READ_COMMITTED;
  trans_rw=TRUE;
  sqloptions = the_sp.default_SQL_options.val();
  stmt_counter=0;
  in_trigger=in_routine=in_admin_mode=open_cursors=0;
#ifdef STOP
  nextIndex=0;
#endif
  random_login_key=0;
  dep_login_of=0;  // not dependent
  dbginfo=NULL;
  report_modulus=256;
  seq_list=NULL; 
  last_active_trigger_tabkont=NULL;
  is_detached=false;
  detached_routine=NULL;
  conditional_attachment=true;  // does not have the licence, prevents returning it
  limited_login=false;  // must be false for detached threads, worker thread, system etc.
  www_client=false;
  MailCtx=NULL;
  execution_kontext=NULL;
  execution_kontext_locked=0;
  exkont_info=NULL;
  exkont_len=0;
  global_sscope=NULL;
  created_global_rscopes=global_rscope=NULL;
  //tzd=server_tzd; -- done in AddTask, server_tzd may not be defined yet here
  procedure_created_cursor=false;
  weak_link=false;
  in_active_ri=0;
  current_generic_error_message=NULL;
  subtrans=NULL;
  current_cursor=NULL;
  temp_param=NULL;
  waits_for=0;
  replication_change_detector=NULL;
  exclusively_accessed_table=NULL;
  my_server_locks=0;
  statement_counter=1;  date_time_statement_cnt_val=0;  // this makes the stored curr_date and curr_time invalid
  last_identity_value=NONEBIGINT;
  top_objnum=NOOBJECT;
  index_owner=NOOBJECT;
  profiling_thread=false;
  thread_name[0]=0;
  stk=NULL;
  last_linechngstop=LCHNGSTOP_ALWAYS;
  in_expr_eval=false;
  fixnum=0;
  cnt_requests=cnt_pagewr=cnt_pagerd=cnt_cursors=cnt_sqlstmts=0;
}

void cd_t::mark_core(void)
{ tl.mark_core();
  for (t_translog * subtl = subtrans;  subtl;  subtl=subtl->next)
    subtl->mark_core();    
  prvs.mark_core();
  if (stmts) stmts->mark_core();
  d_ins_curs.mark_core();
  answer.ans_mark_core();
  if (commenc) mark(commenc);
  if (comp_kontext) mark(comp_kontext);
  msg.mark();
  if (pRemAddr) { pRemAddr->mark_core();  mark(pRemAddr); }
  //if (pImpExpBuff) mark(pImpExpBuff);  -- the buffer is not owned, is not allocated!!!
  if (rscope) rscope->mark_core();
  savepoints.mark_core();
  if (seq_list) seq_list->mark_core();
  if (dbginfo) dbginfo->mark_core(); 
  worm_mark(&jourworm);
  if (exkont_info) mark(exkont_info);
  if (created_global_rscopes) created_global_rscopes->mark_core();
  cevs.mark_core();
  if (current_generic_error_message) mark(current_generic_error_message);
  for (t_stack_item * s = stk;  s;  s = s->next)
    mark(s);  
  if (is_detached && detached_routine) detached_routine->mark_core();  
}

////////////////////////////////////////// trace and logs /////////////////////////////////////////////////////
const char * t_message_cache::enum_messages(int & pos)
// Returns the next message in the buffer of NULL iff all messages passed.
// pos must be -1 in the initial call, then it is the index of the start of the next message to be returned.
// Must enumer all messages, not unlocked otherwise
{ if (pos==-1) { pos=top;  locked++; }
  if (pos==bottom) { locked--;  return NULL; }
  const char * msg = buf+pos;
  pos+=(int)strlen(msg)+1;
  if (buf[pos]==terminator && pos!=bottom) pos=0;  // last message in the buffer, go to the beginning
  return msg;
}

void t_message_cache::add_message(const char * msg)
{ if (locked || bufsize==0) return;
  unsigned len=(unsigned)strlen(msg)+1;
  if (len+1 >= bufsize) return;  // cannot store in the cache
  locked++;
  bool empty = top==bottom;
 // check the space from bottom to the end of buf:
  if (bottom+len+1 >= bufsize)  // drop the messages after [bottom] if any
  { buf[bottom]=terminator;  
    if (top>=bottom) top=0;
    bottom=0;  // move [bottom] to the start of the buffer
  }
 // NOW: can add the message at [bottom]
 // check the space to top, move top if necessary:
  if (bottom <= top && !empty)
    while (bottom+len >= top)  // must be >=, otherwise cannot distinguish empty and full
    { top+=(int)strlen(buf+top)+1;
      if (buf[top]==terminator) { top=0;  break; }
    }
 // copy the message:
  memcpy(buf+bottom, msg, len);
  last_message_ptr=bottom;
  bottom+=len; 
  locked--;
}

const char * t_message_cache::get_last_message(void) const
{
   if (top==bottom) return NULL;
   return buf+last_message_ptr;
}
////////////////////////////////////////////////////////////////////////////////
void t_trace_log::set_format(const char * formatIn)        
{ const char * p = formatIn;  
  while (*p==' ') p++;
  strmaxcpy(format, *p ? formatIn : default_log_format, sizeof(format));
}

t_trace_log::t_trace_log(const char * log_nameIn, const char * formatIn, const char * pathnameIn)
// on LINUX uses server_name
// Called inside [cs_trace_logs] which protects global [trace_log_list]
{ strcpy(log_name, log_nameIn);  sys_Upcase(log_name);
  set_format(formatIn);
  strmaxcpy(pathname, pathnameIn, sizeof(pathname)-6);  // space left for the date
  date_added_at=-1;  last_flush=0;  repetition_count=0;
  next=trace_log_list;  trace_log_list=this;
#ifdef LINUX
  is_system=-1;
  GetDatabaseString(server_name, "SYSLOGS", (char *)&is_system, sizeof(is_system), my_private_server);
#endif
}

static t_trace_log * find_trace_log(const tobjname log_name)
// Called inside [cs_trace_logs] which protects global [trace_log_list]  
{ for (t_trace_log * alog = trace_log_list;  alog;  alog=alog->next)
    if (!sys_stricmp(alog->log_name, log_name)) return alog;
  return NULL;
}

void dump_thread_overview(cdp_t cdp)
{ t_flstr fl;  char buf[500];  int i;
 // all must be protected, write_frames uses global variables
  ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);

#if defined(WINS) && defined(_DEBUG) && !defined(X64ARCH) // handle for stackwalk
// dump the other threads:	
  proc_prep();
	for (i = 1;  i<=max_task_index;  i++)
	{ cdp_t acdp = cds[i];
	  if (acdp)
	  { if (acdp->threadID != GetCurrentThreadId() && cdp!=acdp) // cdp may be NULL
	      SuspendThread(acdp->thread_handle);
      CONTEXT c;
    	memset( &c, '\0', sizeof c );
      c.ContextFlags = CONTEXT_FULL;
      GetThreadContext(acdp->thread_handle, &c);
      write_frames(acdp->thread_handle, c, acdp->applnum_perm, acdp->fixnum);
	    if (acdp->threadID != GetCurrentThreadId() && cdp!=acdp) // cdp may be NULL
	      ResumeThread(acdp->thread_handle);
    }  
	}  
	close_process();
#endif

  t_trace_log * baslog=find_trace_log(NULLSTRING);
  for (i=1;  i<=max_task_index;  i++)   // system process 0 not listed
  { bool insert_it = false;
    if (cds[i])
    { cdp_t cdx = cds[i];
      fl.put(cdx->prvs.luser_name());  fl.putc(',');
      fl.putint(cdx->applnum_perm);  fl.putc(',');
      fl.put(cdx->sel_appl_name);  fl.putc(',');
      fl.putint(cdx->wait_type);  fl.putc(',');
      fl.putint(cdx->in_use!=PT_SLAVE || !cdx->pRemAddr ? 0 : cdx->pRemAddr->is_http ? 4 :
        cdx->pRemAddr->my_protocol==TCPIP ? 1 : cdx->pRemAddr->my_protocol==IPXSPX ? 2 : cdx->pRemAddr->my_protocol==NETBIOS ? 3 : -1);  fl.putc(',');
      if (cdx->pRemAddr) 
        { cdx->pRemAddr->GetAddressString(buf);  fl.put(buf); }
      fl.putc(',');
      fl.putint(cdx->is_detached);  fl.putc(',');
      fl.putint(cdx->in_use==PT_WORKER);  fl.putc(',');
      fl.putint(cdx==cdp);  fl.putc(',');
      fl.putint(cdx->expl_trans!=TRANS_NO);  fl.putc(',');
      fl.putint(cdx->isolation_level);  fl.putc(',');
      fl.putint(cdx->sqloptions);  fl.putc(',');
      fl.putint(cdx->waiting);  fl.putc(',');
      fl.putint(cdx->commenc ? 1 : 0);  fl.putc(',');
      fl.put(cdx->thread_name);  fl.putc(',');
      fl.putint(cdx->session_number);  fl.putc(',');
      fl.putint(cdx->profiling_thread);  fl.putc(',');  
      fl.putint(cdx->wait_type==WAIT_FOR_REQUEST || cdx->wait_type==WAIT_SENDING_ANS || cdx->wait_type==WAIT_RECEIVING || cdx->wait_type==WAIT_TERMINATING 
                                 ? 0 : stamp_now() - cdx->last_request_time);  fl.putc(',');
      fl.putint(cdx->fixnum);  fl.putc(',');                           
     // dump the current context:
      t_exkont * ekt = lock_and_get_execution_kontext(cdx);
      while (ekt)
      { ekt->get_descr(cdx, buf, sizeof(buf));
        fl.put(buf);
        ekt=ekt->_next();
      }  
      unlock_execution_kontext(cdx);
      insert_it=true;
    }
   // database operation performed outside the CS:
    if (insert_it && baslog)
      baslog->output(fl.get());
    fl.reset();
  }
}

void CALLBACK direct_to_log(const char * msg)
{
  t_trace_log * baslog=find_trace_log(NULLSTRING);
  if (baslog) baslog->output(msg);
}

static const char trace_type_letters[] = "EFNRDMSLKVWCXYAQBrwidOPTI&GHJ";

void t_trace_log::get_error_envelope(cdp_t cdp, char * buf, unsigned bufsize, unsigned trace_type, const char * err_msg)  // cdp may be NULL during server startup and shutdown
// Works with translated messages in system charset.
{ char datebuf[11], timebuf[10], sitbuf;  tobjname userbuf;
  union {unsigned uns; const void *p;} args[max_args]; unsigned argnum=0;
  char xformat[MAX_LOG_FORMAT_LENGTH+1];
  unsigned i=0;
  for (;  format[i];  i++)
  { xformat[i]=format[i];                          
    if (format[i]=='%')
      if (argnum<max_args-1)
      { i++;
        while (format[i]=='-' || format[i]=='#' || format[i]>='0' && format[i]<='9')
           { xformat[i]=format[i];  i++; }
        switch (format[i])
        { case 'd':
            date2str(Today(), datebuf, 0);  args[argnum++].p= datebuf;  xformat[i]='s';  break;
          case 'D':
            date2str(Today(), datebuf, 1);  args[argnum++].p= datebuf;  xformat[i]='s';  break;
          case 't':
            time2str(Now(),   timebuf, 0);  args[argnum++].p= timebuf;  xformat[i]='s';  break;
          case 'T':
            time2str(Now(),   timebuf, 1);  args[argnum++].p= timebuf;  xformat[i]='s';  break;
          case 's':  case 'S':
          { unsigned trace_num=0;
            if (trace_type) // fuse
              while (!(trace_type & 1)) { trace_type >>= 1;  trace_num++; }
            sitbuf=trace_type_letters[trace_num];  
            args[argnum++].uns= sitbuf;  xformat[i]='c';  break;
          }
          case 'u':  case 'U':
            if      (!cdp || cdp==cds[0])    strcpy(userbuf, " Console");
            else if (cdp->is_detached)       strcpy(userbuf, " Detached");
            else if (cdp->in_use==PT_WORKER) strcpy(userbuf, " WorkerThread");
            else                             strcpy(userbuf, cdp->prvs.luser_name());
            args[argnum++].p = userbuf;  xformat[i]='s';  break;
          case 'm':  case 'M':
            args[argnum++].p = err_msg;  xformat[i]='s';  break;
          case 'c':  case 'C':
            args[argnum++].uns=cdp ? cdp->applnum_perm   : 0;   xformat[i]='u';  break;
          case 'e':  case 'E':
            args[argnum++].uns=cdp ? cdp->session_number : 0;   xformat[i]='d';  break;
          case 'a':  case 'A':
            args[argnum++].p = cdp ? cdp->sel_appl_name  : "";  xformat[i]='s';  break;
          default:
            args[argnum++].p= "(undef)";  xformat[i]='s';  break;
        }
      }
      else // too many %-s
        xformat[i]='#';
  }
  xformat[i]=0;
  snprintf(buf, bufsize, xformat, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
  buf[bufsize-1]=0;  // on overflow the terminator is NOT written!
}

void t_trace_log::output(const char * xbuf)
// Called inside [cs_trace_logs] which protects every t_trace_log structure
{// check for the date change: 
  if (!*log_name ? the_sp.DailyBasicLog : the_sp.DailyUserLogs)  // use daily log files
    if (Today() != log_date)  // log_date == 0 if log opened as non-daily
    { close_log_file();
      open_log_file();
    }
  if (!FILE_HANDLE_VALID(hTraceLog)) return;  // log may be closed when closing the server or when a bad pathname was specified
 // write to the log file:
  DWORD wr;
  WriteFile(hTraceLog, xbuf, (int)strlen(xbuf), &wr, NULL);
  WriteFile(hTraceLog, "\r\n", 2, &wr, NULL);
 // flush the basic log frequently 
  if (!*log_name) 
  { uns32 tm=clock_now();  // seconds from the start of the day
    if (last_flush!=tm)
    { last_flush=tm;
      FlushFileBuffers(hTraceLog);  // on Linux it writes data but does not update file update time -> faster
    }
  }  
#ifdef LINUX
  if (-1!=is_system) 
  { char * buf2 = (char*)corealloc(OBJ_NAME_LEN+3+strlen(xbuf)+1, 77);
    if (buf2)
    { strcpy(buf2, server_name);  strcat(buf2, ": ");  strcat(buf2, xbuf);
      syslog(is_system, buf2);
      corefree(buf2);
    }
    else 
      syslog(is_system, xbuf);
  }    
#endif
}

BOOL t_trace_log::open_log_file(void)
// Called inside [cs_trace_logs] which protects every t_trace_log structure
{ if (!*log_name ? the_sp.DailyBasicLog : the_sp.DailyUserLogs)  // use daily log files
  { log_date=Today();
   // make space for the date, if not made before: 
    if (date_added_at==-1)
    { date_added_at=(int)strlen(pathname);
      while (date_added_at && pathname[date_added_at]!='.' && pathname[date_added_at-1]!=PATH_SEPARATOR) date_added_at--;
      if (pathname[date_added_at]=='.')
        memmov(pathname+date_added_at+6, pathname+date_added_at, strlen(pathname)-date_added_at+1);
      else 
        { date_added_at=(int)strlen(pathname);  pathname[date_added_at+6]=0; }
    }
   // write date into the file name:
    pathname[date_added_at  ] = '0' + Year(log_date) % 100 / 10;
    pathname[date_added_at+1] = '0' + Year(log_date) % 10;
    pathname[date_added_at+2] = '0' + Month(log_date) / 10;
    pathname[date_added_at+3] = '0' + Month(log_date) % 10;
    pathname[date_added_at+4] = '0' + Day(log_date) / 10;
    pathname[date_added_at+5] = '0' + Day(log_date) % 10;
  }
  else log_date=0;  // causes reopenning the log when Daily property changes
  hTraceLog = CreateFile(pathname, GENERIC_WRITE, FILE_SHARE_READ, NULL,
		    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, (HANDLE) NULL);
  if (!FILE_HANDLE_VALID(hTraceLog)) return FALSE;
  SetFilePointer(hTraceLog, 0, NULL, FILE_END);
  return TRUE;
}

void t_trace_log::close_log_file(void)
// Called inside [cs_trace_logs] which protects every t_trace_log structure
{ CloseFile(hTraceLog);  hTraceLog=INVALID_FHANDLE_VALUE; }

BOOL define_trace_log(cdp_t cdp, tobjname log_name, const char * pathname, const char * format)
{ if (!cdp->prvs.is_conf_admin)
    { request_error(cdp, NO_CONF_RIGHT);  return FALSE; }
  { ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);  
    t_trace_log * alog = find_trace_log(log_name);
    if (alog)  // existing log => change the format only
    { alog->set_format(format);
      return TRUE;
    }
    char fname[MAX_PATH];
   // create the default pathname if not specified:
    if (!*pathname)
    { if (!GetDatabaseString(server_name, "LogPath", fname, sizeof(fname), my_private_server) || !*fname)
        strcpy(fname, ffa.last_fil_path()); // log in the FIL directory
      if (*log_name)
        { append(fname, log_name);  strcat(fname, ".txt"); }
      else
        append(fname, MAIN_LOG_NAME);
      pathname=fname;
    }
    alog = new t_trace_log(log_name, format, pathname);
    if (!alog) return FALSE;
    return alog->open_log_file();  // log exists even if the file was not open
  }
}

void close_trace_logs(void)
// Closes all trace logs except for the defaut one (which is closed by ffa)
// Called on shutdown only, trace_log_list is in invalid state after the call
{ for (t_trace_log * alog = trace_log_list;  alog;  alog=alog->next)
    alog->close_log_file();
  trace_log_list=NULL;
}

const char * enum_server_messages(int & pos)
// Used when the server window is opened. The [cs_trace_logs] may not be created yet.
{ t_trace_log * alog = find_trace_log("");
  if (!alog) return NULL;
  return alog->message_cache.enum_messages(pos); 
}

///////////////////////////////////////// recent log ///////////////////////////////////////////////
void fill_recent_log(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
// must use dynamic structores because of variable-size [log_message]
{ tobjname a_log_name;  sig32 a_seq_num;  uns32 a_length, a_length2;
  t_colval recent_log_coldescr[5];
  memset(recent_log_coldescr, 0, sizeof(recent_log_coldescr));
  recent_log_coldescr[0].attr=1;
  recent_log_coldescr[0].dataptr=a_log_name;
  recent_log_coldescr[1].attr=2;
  recent_log_coldescr[1].dataptr=&a_seq_num;
  recent_log_coldescr[2].attr=3;
  recent_log_coldescr[2].lengthptr=&a_length;
  recent_log_coldescr[2].offsetptr=NULL;
  recent_log_coldescr[3].attr=4;
  recent_log_coldescr[3].lengthptr=&a_length2;
  recent_log_coldescr[3].offsetptr=NULL;
  recent_log_coldescr[4].attr=NOATTRIB;
  t_vector_descr vector(recent_log_coldescr);
 // analyze the conditions:
  tobjname log_name;  BOOL found, loglist=FALSE;
  found=extract_string_condition(cdp, post_conds, 1, log_name, OBJ_NAME_LEN);
  for (int i=0;  i<post_conds->count();  i++)
  { t_express * ex = *post_conds->acc0(i);
    if (ex->sqe==SQE_BINARY)
    { t_expr_binary * bex = (t_expr_binary*)ex;
      if (bex->oper.pure_oper==PUOP_GE)
      { if (bex->op1->sqe==SQE_ATTR && bex->op2->sqe==SQE_LCONST)
          if (((t_expr_attr*)bex->op1)->elemnum==1) // log name
            if (!*((t_expr_lconst*)bex->op2)->lval) loglist=TRUE;  // condition: log_name>=""
      }
    }
  }
  if (found) sys_Upcase(log_name);
  if (!cdp->prvs.is_conf_admin)
    if (!loglist && (!found || *log_name))
      { request_error(cdp, NO_CONF_RIGHT);  return; }

 // pass the logs with short-term locking - must not be in [cs_trace_logs] when calling tb_new():
  unsigned counter=0, num;
  do
  { t_trace_log * alog;
    { ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);
     // get the data from counter-th log request:
      alog = trace_log_list;  num=0;
      while (alog && num<counter)
        { alog=alog->next;  num++; }
      if (!alog) break;  // end of the list 
     // have a log, copy data:
      strcpy(a_log_name, alog->log_name);
      if (loglist)
      { a_seq_num=-1;
        a_length=(uns32)strlen(alog->pathname);
        recent_log_coldescr[2].dataptr=alog->pathname;  // no log message
        a_length2=0;
        recent_log_coldescr[3].dataptr="";
      }
    }
    counter++;
    if (loglist) 
      tb_new(cdp, tbdf, FALSE, &vector);
    else if (!found || !strcmp(a_log_name, log_name)) 
    { a_seq_num=0;
      int pos = -1;
      do // alog is not protected any more but logs are never deleted
      { const char * msg = alog->message_cache.enum_messages(pos);  // starts the message cache protection
        if (!msg) break;
        a_length=(uns32)strlen(msg);
        recent_log_coldescr[2].dataptr=msg;
       // prepare the unicode version:
        recent_log_coldescr[3].dataptr=corealloc((a_length+1)*sizeof(wuchar), 85);
        conv1to2(msg, a_length, (wuchar*)recent_log_coldescr[3].dataptr, sys_spec.charset ? sys_spec.charset : CHARSET_NUM_UTF8, 0);
        a_length2 = 2*(uns32)wuclen((wuchar*)recent_log_coldescr[3].dataptr); 
        tb_new(cdp, tbdf, FALSE, &vector);
        corefree(recent_log_coldescr[3].dataptr); 
        a_seq_num++;
      } while (true);
    }
  } while (true);
}

void return_recent_log(cdp_t cdp, uns32 size_limit)
{ ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);
  cdp->answer.sized_answer_start();
  if (size_limit>=sizeof(wuchar)) // error
  { size_limit-=sizeof(wuchar);  // space for the final delimiter
    t_trace_log * alog = find_trace_log("");
    if (alog)
    { int pos = -1;
      while (size_limit>sizeof(wuchar)) 
      { const char * msg = alog->message_cache.enum_messages(pos);  // starts the message cache protection
        if (!msg) break;
        int length=(int)strlen(msg);
       // prepare the unicode version:
        wuchar * uptr = (wuchar*)corealloc((length+1)*sizeof(wuchar), 85);
        conv1to2(msg, length, uptr, sys_spec.charset ? sys_spec.charset : CHARSET_NUM_UTF8, 0);
        int length2 = 2*(int)wuclen(uptr); 
        if (length2+sizeof(wuchar) > size_limit)
          length2=size_limit-sizeof(wuchar);
        memcpy(cdp->get_ans_ptr(length2), uptr, length2);
        *(wuchar*)cdp->get_ans_ptr(sizeof(wuchar)) = 0;  // line terminator 
        size_limit -= length2+sizeof(wuchar);
        corefree(uptr); 
      } 
    }
    *(wuchar*)cdp->get_ans_ptr(sizeof(wuchar)) = 0;  // final terminator 
  }      
  cdp->answer.sized_answer_stop();
}
 
struct t_log_request; ////////////////////////////////////////////////////////////////////////////////////////////////
t_log_request * pending_log_request_list = NULL;  // protected by [cs_trace_logs]

struct t_log_request
{ uns32 situation;    // same as trace_type, nonzero
  tobjnum userobj;    // logged user or NOOBJECT iff logging is independent of user name
  int kontext_extent; // is never 0, log request removed when kontext_extent==0, 1=no kontext, 2=single kontext, 3=full kontext
  tobjnum objnum;     // logged object
  tobjname log_name;  // empty string for the default log
  t_log_request * next;
  t_log_request(uns32 situationIn, tobjnum usernumIn, int kontext_extentIn, tobjnum objnumIn, tobjname log_nameIn)
  // Called inside [cs_trace_logs] which protects [pending_log_request_list].
  { situation=situationIn;  userobj=usernumIn;  kontext_extent=kontext_extentIn;  objnum=objnumIn;
    strcpy(log_name, log_nameIn);
    next=NULL;
   // add to the end of the list (better behaviuor in the monitor):
    if (!pending_log_request_list) pending_log_request_list=this;
    else
    { t_log_request * rq = pending_log_request_list;  
      while (rq->next) rq=rq->next;
      rq->next=this;
    }
  }
  inline bool user_ok(tobjnum user_num)
    { return userobj==NOOBJECT || user_num==userobj; }
  inline bool object_ok(tobjnum object_num)
  { if (!TRACE_SIT_TABLE_DEPENDENT(situation)) return true;  // situation is not table-dependent
    if (object_num==objnum) return true;                     // tracing the actual table
    return objnum==NOOBJECT && !SYSTEM_TABLE(object_num);    // tracing all and the actual object is not a system table
  }
};

static unsigned global_trace_enabled = 0;

BOOL is_trace_enabled_global(unsigned trace_type)
{ return (global_trace_enabled & trace_type) != 0; }

#define MAX_TRACE_MSG_LEN 2000

static int write_message_to_a_log(cdp_t cdp, unsigned trace_type, const char * err_text, 
                            const char * log_name, int kontext_extent, tptr & xbuf)
// Called inside [cs_trace_logs].
// Returns 0 on normal output, >0 if the previuos message has been repeated, -1 when suspending the output of repeated message
{ int lenlimit = MAX_TRACE_MSG_LEN+(int)strlen(err_text);
  if (xbuf==NULL)
  { xbuf = (tptr)corealloc(lenlimit, 66);
    if (xbuf==NULL) return 0;
  }
  char buf[30+OBJ_NAME_LEN];  
 // find the log description:
  t_trace_log * alog = find_trace_log(log_name);
  if (!alog)
  { alog = find_trace_log("");
    if (!alog) // this may occur if the log file could not be opened, must define xbuf!
    { strcpy(xbuf, err_text);
      return 0;
    }
    sprintf(buf, TRACELOG_NOTFOUND, log_name); 
    err_text=buf;
  }
  alog->get_error_envelope(cdp, xbuf, lenlimit, trace_type, err_text);
  int len = (int)strlen(xbuf);
 // add kontext, if requered:
  if (cdp && kontext_extent>1)
  {// execution context list:
    t_exkont * ekt = lock_and_get_execution_kontext(cdp);
    while (ekt)
    { len = (int)strlen(xbuf);
      ekt->get_descr(cdp, xbuf+len, lenlimit-len);
      if (kontext_extent==2) break;
      ekt=ekt->_next();
    }
    unlock_execution_kontext(cdp);
  }
 // compare the nmessage with the previous one:
  const char * prev = alog->message_cache.get_last_message();
  if (prev && !strcmp(prev, xbuf))
    { alog->repetition_count++;  return -1; }
  int orig_rep_cnt = alog->repetition_count;  
  if (alog->repetition_count)
  { char buf[50];
    form_message(buf, sizeof(buf), msg_repetition, alog->repetition_count);
    alog->message_cache.add_message(buf);
    alog->output(buf);
    alog->repetition_count=0;
  }  
 // output to message cache and console:
  alog->message_cache.add_message(xbuf);
#ifdef LINUX
 /* We have already flushed the message to the cache, and only output
  * to the log file, system log and screen remain. Thus, it is safe to
  * change diacritics. */
  //no_diacrit(xbuf, lenlimit); - not converting here any more
#endif
 // output to file:
  alog->output(xbuf);
  return orig_rep_cnt;
}

void log_write_ex(cdp_t cdp, const char * err_text, const char * log_name, int kontext_extent)
{ char * xbuf = NULL;  // allocate when used for the 1st time
  { ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);  
    write_message_to_a_log(cdp, TRACE_LOG_WRITE, err_text, log_name, kontext_extent, xbuf);
  }
 // writing to the global log requires synchonisation with the thread owning the list box, must be outside CritSec
  if (!*log_name && xbuf)
    write_to_server_console(cdp, xbuf);
  if (xbuf) corefree(xbuf);
}

void WINAPI trace_msg_general(cdp_t cdp, unsigned trace_type, const char * err_text, tobjnum objnum)  // creates server trace message 
// [err_text] is in the system charset, translated.
{// prepare message buffer: 
  char * xbuf = NULL;  // allocate when used for the 1st time
 // write to non-basic logs:
  { ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);  // cdp may be NULL here!
   // write the message to log(s):
    for (t_log_request * req = pending_log_request_list;  req;  req=req->next)
      if (req->situation==trace_type && *req->log_name)
        if (!cdp || req->user_ok(cdp->prvs.luser()) || !cdp->applnum_perm) // log errors on server account
          if (req->object_ok(objnum))
            write_message_to_a_log(cdp, trace_type, err_text, req->log_name, req->kontext_extent, xbuf);
  }
 // write to the basic log (I need the massage to be formatted for the basic log after on the end):
  int to_global_log = -2;  // true when the basic log accepts the situation
  { ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);  // cdp may be NULL here!
   // write the message to log(s):
    for (t_log_request * req = pending_log_request_list;  req;  req=req->next)
      if (req->situation==trace_type && !*req->log_name)
        if (!cdp || req->user_ok(cdp->prvs.luser()) || !cdp->applnum_perm) // log errors on server account
          if (req->object_ok(objnum))
            to_global_log=write_message_to_a_log(cdp, trace_type, err_text, req->log_name, req->kontext_extent, xbuf);
  }
 // writing to the global log requires synchonisation with the thread owning the list box, must be outside CritSec
  if (to_global_log>=0 && xbuf && !console_disabled)
  { if (to_global_log>0)
    { char buf[50];
      form_message(buf, sizeof(buf), msg_repetition, to_global_log);
      write_to_server_console(cdp, buf);
    }  
    write_to_server_console(cdp, xbuf);
  }  
  if (xbuf) corefree(xbuf);
}

uns32 trace_map(tobjnum user_objnum)
{ uns32 map = 0;
  ProtectedByCriticalSection cs(&cs_trace_logs);
  for (t_log_request * req = pending_log_request_list;  req;  req=req->next)
    if (req->user_ok(user_objnum))
      map |= req->situation;
  return map;
}

BOOL trace_def(cdp_t cdp, uns32 situation, tobjname username, char * objname, tobjname log_name, int kontext_extent)
// Adds, updates or removes a trace request.
// If kontext_extent if offset by 1000 the request need not be persistently stored. Otherwise stores it in a property.
{ tobjnum user_objnum;
  if (cdp)
  { if (situation==TRACE_READ || situation==TRACE_WRITE)
    { if (!cdp->prvs.is_secur_admin)
        { request_error(cdp, NO_RIGHT);  return FALSE; }
    }
    else
      if (!cdp->prvs.is_conf_admin)
        { request_error(cdp, NO_CONF_RIGHT);  return FALSE; }
    
  }
  sys_Upcase(log_name);
  BOOL make_persistent = TRUE;
  if (kontext_extent >= 1000)
  { kontext_extent -= 1000;
    make_persistent = FALSE;
  }
 // find user object:
  if (*username && !TRACE_SIT_USER_INDEPENDENT(situation))
  { sys_Upcase(username);
    user_objnum=find_object(cdp, CATEG_USER, username);
    if (user_objnum==NOOBJECT) return FALSE;
  } else user_objnum=NOOBJECT;
 // analyse the object name (only for table-dependent situations):
  const char * p = objname;   tobjnum objnum;
  if (*objname && TRACE_SIT_TABLE_DEPENDENT(situation))
  { if (find_obj(cdp, objname, CATEG_TABLE, &objnum))  // analyses and searches the (prefixed) table name
      return FALSE;
  }
  else objnum=NOOBJECT;
 // look for the logging request in the list:
  { ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);
    t_log_request ** preq = &pending_log_request_list;
    while (*preq)
    { t_log_request * req = *preq;
      if (situation==req->situation && req->userobj==user_objnum && req->objnum==objnum && !stricmp(log_name, req->log_name))
        if (kontext_extent)
          { req->kontext_extent=kontext_extent;  goto update; }
        else                                    
          { *preq=req->next;  delete req;  goto update; }
      preq=&req->next;
    }
   // not found:
    if (kontext_extent)
      new t_log_request(situation, user_objnum, kontext_extent, objnum, log_name);  // must be in [cs_trace_logs]
  } // must not be in [cs_trace_logs] when etering [cs_client_list] or writing to the database!
 // update local bitmaps:
 update:
  { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
    for (int i=1;  i <= max_task_index;  i++) if (cds[i]) 
    { cdp_t acdp = cds[i];
      acdp->trace_enabled |=  trace_map(acdp->prvs.luser());
    }
  }
  global_trace_enabled = trace_map(NOOBJECT);
 // persistent storage of global trace flags:
  if (make_persistent && cdp && !*username && !*objname && !*log_name)
  { for (int i=0;  i<TRACE_PROPERTY_ASSOC_COUNT;  i++)
      if (the_sp.trace_property_assocs[i].situaton==situation)
      { char numbuf[12];  int2str(kontext_extent, numbuf);
        the_sp.trace_property_assocs[i].prop->set_and_save(cdp, &the_sp, sqlserver_property_name, 0, numbuf, 0); // vallen ignored
        break;
      }
  }
  return TRUE;
}

t_property_descr_int t_server_profile::trace_user_error    ("Monitor all errors",      FALSE, 0);
t_property_descr_int t_server_profile::trace_network_global("Monitor network",         FALSE, 0);
t_property_descr_int t_server_profile::trace_replication   ("Monitor replication",     FALSE, 0);
t_property_descr_int t_server_profile::trace_direct_ip     ("Monitor direct IP",       FALSE, 0);
t_property_descr_int t_server_profile::trace_user_mail     ("Monitor user mail",       FALSE, 0);
t_property_descr_int t_server_profile::trace_sql           ("Monitor SQL",             FALSE, 0);
t_property_descr_int t_server_profile::trace_login         ("Monitor login",           FALSE, 0);
t_property_descr_int t_server_profile::trace_cursor        ("Monitor cursor",          FALSE, 0);
t_property_descr_int t_server_profile::trace_impl_rollback ("Monitor impl. rollback",  FALSE, 0);
t_property_descr_int t_server_profile::trace_network_error ("Monitor network error",   FALSE, 0);
t_property_descr_int t_server_profile::trace_replic_mail   ("Monitor replic mail",     FALSE, 0);
t_property_descr_int t_server_profile::trace_replic_copy   ("Monitor replic copy",     FALSE, 0);
t_property_descr_int t_server_profile::trace_repl_conflict ("Monitor replic conflict", FALSE, 0);
t_property_descr_int t_server_profile::trace_bck_obj_error ("Monitor bck. obj. error", FALSE, 0);
t_property_descr_int t_server_profile::trace_procedure_call("Monitor procedure call" , FALSE, 0);
t_property_descr_int t_server_profile::trace_trigger_exec  ("Monitor trigger exec"   , FALSE, 0);
t_property_descr_int t_server_profile::trace_user_warning  ("Monitor warnings"       , FALSE, 0);
t_property_descr_int t_server_profile::trace_server_info   ("Monitor server info",     FALSE, 1);
t_property_descr_int t_server_profile::trace_server_failure("Monitor server failure",  FALSE, 1);
t_property_descr_int t_server_profile::trace_log_write     ("Monitor log write",       FALSE, 1);
t_property_descr_int t_server_profile::trace_start_stop    ("Monitor start stop",      FALSE, 1);
t_property_descr_int t_server_profile::trace_lock_error    ("Monitor lock error",      FALSE, 1);
t_property_descr_int t_server_profile::trace_web_request   ("Monitor web requests",    FALSE, 0);
t_property_descr_int t_server_profile::trace_convertor_error("Monitor convertor error",FALSE, 1);

// Association of persistent server properties and trace situations:
t_trace_property_assoc t_server_profile::trace_property_assocs[TRACE_PROPERTY_ASSOC_COUNT] =
{
 { TRACE_USER_ERROR,     &trace_user_error     },
 { TRACE_NETWORK_GLOBAL, &trace_network_global },
 { TRACE_REPLICATION,    &trace_replication    },
 { TRACE_DIRECT_IP,      &trace_direct_ip      },
 { TRACE_USER_MAIL,      &trace_user_mail      },
 { TRACE_SQL,            &trace_sql            },
 { TRACE_LOGIN,          &trace_login          },
 { TRACE_CURSOR,         &trace_cursor         },
 { TRACE_IMPL_ROLLBACK,  &trace_impl_rollback  },
 { TRACE_NETWORK_ERROR,  &trace_network_error  },
 { TRACE_REPLIC_MAIL,    &trace_replic_mail    },
 { TRACE_REPLIC_COPY,    &trace_replic_copy    },
 { TRACE_REPL_CONFLICT,  &trace_repl_conflict  },
 { TRACE_BCK_OBJ_ERROR,  &trace_bck_obj_error  },
 { TRACE_PROCEDURE_CALL, &trace_procedure_call },
 { TRACE_TRIGGER_EXEC,   &trace_trigger_exec   },
 { TRACE_USER_WARNING,   &trace_user_warning   },
 { TRACE_SERVER_INFO,    &trace_server_info    },
 { TRACE_SERVER_FAILURE, &trace_server_failure },
 { TRACE_LOG_WRITE,      &trace_log_write      },
 { TRACE_START_STOP,     &trace_start_stop     },
 { TRACE_LOCK_ERROR,     &trace_lock_error     },
 { TRACE_WEB_REQUEST,    &trace_web_request    },
 { TRACE_CONVERTOR_ERROR,&trace_convertor_error}
};

void mark_trace_logs(void)
{ ProtectedByCriticalSection cs(&cs_trace_logs, cds[0], WAIT_CS_TRACE_LOGS);
  for (t_trace_log * alog = trace_log_list;  alog;  alog=alog->next)
    alog->mark();  
  for (t_log_request * req = pending_log_request_list;  req;  req=req->next)
    mark(req);
}

#include "trctbl.cpp"

////////////////////////////////////// list of log requests ////////////////////////////////
struct result_log_reqs
{ tobjname log_name;
  int situation;
  tobjnum usernum;
  tobjnum objnum;
  int kontext;
};

static const t_colval log_reqs_coldescr[] = {
{  1, NULL, (void*)offsetof(result_log_reqs, log_name),  NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_log_reqs, situation), NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_log_reqs, usernum),   NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_log_reqs, objnum),    NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_log_reqs, kontext),   NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

void fill_log_reqs(cdp_t cdp, table_descr * tbdf)
{ result_log_reqs rlr; 
  t_vector_descr vector(log_reqs_coldescr, &rlr);
 // pass the log requests with short-term locking - must not be in [cs_trace_logs] when calling tb_new():
  unsigned counter=0, num;
  do
  {// get the data from counter-th log request:
    { ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);
      t_log_request * req = pending_log_request_list;  num=0;
      while (req && num<counter)
        { req=req->next;  num++; }
      if (!req) break;  // end of the list 
     // copy data from the log request to [rlr]:
      strcpy(rlr.log_name, req->log_name);
      rlr.situation=req->situation;
      rlr.usernum  =req->userobj;
      rlr.objnum   =req->objnum;
      rlr.kontext  =req->kontext_extent;
    }
    tb_new(cdp, tbdf, FALSE, &vector);
    counter++;
  } while (true);
}

/////////////////////////////////////////////// events ////////////////////////////////////////////
#define PARAM_VALUES_PER_EVENT_LIMIT  5000
uns32 event_handle_counter = 1;

bool is_local_event(const char * EventName) 
{ return *EventName=='@'; }

void event_management::register_event(cdp_t cdp, const char * EventName, const char * ParamStr, BOOL exact, uns32 * ev_handle)
// Can register multiple identical events.
{
  { ProtectedByCriticalSection cs2(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);  // may reallocate [reg_ev]
    if (!is_init)
      { CreateSemaph(1, 100, &event_semaphore);  cancel_requested=0;  is_init=TRUE; }
    tobjname upcase_event_name;  int space = -1;
    strmaxcpy(upcase_event_name, EventName, sizeof(upcase_event_name));  sys_Upcase(upcase_event_name);  
   // search the free space:
    for (int j=0;  j<reg_ev.count();  j++)
    { const char * name = reg_ev.acc0(j)->EventName;
      //if (!strcmp(name, upcase_event_name))
      //{ if (reg_ev.acc0(j)->ParamStr==NULL ? !*ParamStr : !strcmp(reg_ev.acc0(j)->ParamStr, ParamStr))
      //    goto err2;  // is already registered
      //}
      //else 
      if (!*name) space=j;
    }
   // create a new registered event:
    t_reg_ev * rev;
    if (space==-1)
    { rev = reg_ev.next();
      if (!rev) goto err;
    }
    else
      rev = reg_ev.acc0(space);
    if (*ParamStr)
    { rev->ParamStr=(char*)corealloc(strlen(ParamStr)+1, 49);
      if (!rev->ParamStr) goto err;  // rev is still free
      strcpy(rev->ParamStr, ParamStr);
    }
    else rev->ParamStr=NULL;
    strcpy(rev->EventName, upcase_event_name);
    rev->count=0;
    *ev_handle=rev->handle=event_handle_counter++;
    rev->exact=exact;
    rev->occ=NULL;
    return;
  }
 err:  // outside CS
  request_error(cdp, OUT_OF_KERNEL_MEMORY);  
//  return;
// err2:
//  SET_ERR_MSG(cdp, "-- events --");
//  request_error(cdp, KEY_DUPLICITY);  
}

void t_reg_ev::clear(void)
{// release owned data:
  if (ParamStr) { corefree(ParamStr);  ParamStr=NULL; }
  while (occ)
  { t_event_occurrence * aocc = occ;
    occ = aocc->next;
    delete aocc;
  }
  *EventName=0;    // marked as unused
  handle=0;  // prevents finding it when it is unused
  count=0;   // prevents action in wait_for_event
}

void event_management::unregister_event(uns32 ev_handle)
{ if (!is_init) return;  // nothing registered
  { ProtectedByCriticalSection cs(&cs_short_term);
   // search the event:
    for (int j=0;  j<reg_ev.count();  j++)
      if (reg_ev.acc0(j)->handle==ev_handle)
      { reg_ev.acc0(j)->clear();
        break; 
      }
  }
}

int event_management::wait_for_event(cdp_t cdp, int timeout, uns32* EventHandle, uns32 * event_count, char * ParamStr, sig32 param_size)
{ *ParamStr=0;  *EventHandle=0;  // returned if result is not WAIT_EVENT_OK
  do  // must cycle because multiple events may have been fetched in a signle wait
  { if (timeout)
    { cdp->wait_type=WAIT_EVENT;
      integrity_check_planning.thread_operation_stopped();
//#ifdef WINS
//      HANDLE arr[2] = { event_semaphore, hKernelLockEvent };
//      DWORD res=WaitForMultipleObjects(2, arr, FALSE, tm);
//#else
      DWORD res=WaitSemaph(&event_semaphore, timeout);
//#endif
      cdp->wait_type=WAIT_NO;
      integrity_check_planning.continuing_replication_operation();  // this restart does not cancel integrity checks
      if (res == WAIT_TIMEOUT) return WAIT_EVENT_TIMEOUT;
//      if (res == WAIT_OBJECT_0+1) return WAIT_EVENT_SHUTDOWN;
    }
    if (cancel_requested) 
    { int retval = cancel_requested;
      cancel_requested = 0;
      return retval;  // WAIT_EVENT_CANCELLED or WAIT_EVENT_SHUTDOWN
    }
   // find the signalled event:
    { ProtectedByCriticalSection cs2(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
      for (int j=0;  j<reg_ev.count();  j++)
        if (reg_ev.acc0(j)->count)
        { t_reg_ev * rev = reg_ev.acc0(j);
          if (!*rev->EventName) continue;  // fuse, event may be unregistered
         // copy the parameter:
          { const char * param = rev->exact ? rev->ParamStr : rev->occ->param;
            if (param)
            { if (strlen(param) >= param_size) return WAIT_EVENT_ERROR_BUFFER_SIZE;  // buffer size error
              strcpy(ParamStr, param);
            }
            else 
            { if (param_size<=0) return WAIT_EVENT_ERROR_BUFFER_SIZE;  // buffer size error
              *ParamStr=0;
            }
            if (rev->exact)
            { *event_count=rev->count;
              rev->count=0;
            }
            else
            { t_event_occurrence * occ = rev->occ;
              *event_count = occ->count;
              rev->count -= occ->count;
              rev->occ = occ->next;
              delete occ;
            }
          }
          *EventHandle=rev->handle;
          return WAIT_EVENT_OK;
        }
    }
    if (!timeout) return WAIT_EVENT_NOT_SIG;
  } while (true);  // unregistered before Wait awaked
}

void cancel_event_wait(cdp_t cdp, uns32 ev_handle)
{ bool found = false;
  ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
  ProtectedByCriticalSection cs2(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
  for (int c=1;  c <= max_task_index;  c++)
    if (cds[c])
    { cdp_t acdp = cds[c];
      if (acdp->cevs.is_init)
      { for (int j=0;  j<acdp->cevs.reg_ev.count();  j++)
          if (acdp->cevs.reg_ev.acc0(j)->handle==ev_handle)
            { found=true;  break; }
        if (found)
        { acdp->cevs.cancel_requested=WAIT_EVENT_CANCELLED;
          ReleaseSemaph(&acdp->cevs.event_semaphore, 1);
          break;
        }
      }
    }
}

void event_management::cancel_event_if_waits(cdp_t cdp)
{ if (cdp->wait_type==WAIT_EVENT)
  { cdp->cevs.cancel_requested=WAIT_EVENT_CANCELLED;
    ReleaseSemaph(&cdp->cevs.event_semaphore, 1);
  }
}

void event_management::shutdown(void)
{ if (is_init)
  { cancel_requested=WAIT_EVENT_SHUTDOWN;
    ReleaseSemaph(&event_semaphore, 1);
  }
}

void event_management::mark_core(void)
{ for (int i=0;  i<invoked_ev.count();  i++)
    if (invoked_ev.acc0(i)->ParamStr)
      mark(invoked_ev.acc0(i)->ParamStr);
  for (int j=0;  j<reg_ev.count();  j++)
  { t_reg_ev * rev = reg_ev.acc0(j);
    if (rev->ParamStr) mark(rev->ParamStr);
    t_event_occurrence * occ = rev->occ;
    while (occ)
    { mark(occ);
      occ = occ->next;
    }
  }
  invoked_ev.mark_core();
  reg_ev.mark_core();
}

void event_management::close(void)
{ cancel_events();  // independent of [is_init]
 // unregister registered events:
  for (int j=0;  j<reg_ev.count();  j++)
    reg_ev.acc0(j)->clear();
  if (is_init) 
    { CloseSemaph(&event_semaphore);  is_init=FALSE; }
}

void * t_event_occurrence::operator new(size_t size, unsigned data_size)
  { return corealloc(size+data_size, 56); }
#ifdef WINS
void  t_event_occurrence::operator delete(void * ptr, unsigned data_size)
  { corefree(ptr); }
#endif
#if !defined(WINS) || defined(X64ARCH)
void  t_event_occurrence::operator delete(void * ptr)
  { corefree(ptr); }
#endif
t_event_occurrence::t_event_occurrence(const char * ParamStr)
{ strcpy(param, ParamStr ? ParamStr : NULLSTRING);
  count=0;
}

void event_management::invoke_event(cdp_t cdp, const char * EventName, const char * ParamStr)
// Stores the event among invoked_ev. Called only by owner of cdp, no CS necessary
// ParamStr is never NULL, may be "".
{ 
  t_invoked_ev * ev;  tobjname upcase_event_name;  
  strmaxcpy(upcase_event_name, EventName, sizeof(upcase_event_name));  sys_Upcase(upcase_event_name);
  for (int i=0;  i<invoked_ev.count();  i++)
  { ev = invoked_ev.acc0(i);
    if (!strcmp(upcase_event_name, ev->EventName))
      if (ev->ParamStr==NULL ? !*ParamStr : !strcmp(ParamStr, ev->ParamStr))
        { ev->count++;  return; }
  }
  ev = invoked_ev.next();
  if (!ev) goto err;
  strcpy(ev->EventName, upcase_event_name);
  if (*ParamStr)
  { ev->ParamStr=(char*)corealloc(strlen(ParamStr)+1, 49);
    if (!ev->ParamStr) goto err;  // will not be committed
    strcpy(ev->ParamStr, ParamStr);
  }
  else ev->ParamStr=NULL;
  ev->count=1;
  return;
 err:  // outside CS
  request_error(cdp, OUT_OF_KERNEL_MEMORY);  
}

void event_management::cancel_events(void)
{ for (int i=0;  i<invoked_ev.count();  i++)
    safefree((void**)&invoked_ev.acc0(i)->ParamStr);
  invoked_ev.clear(); 
}

void event_management::commit_events(cdp_t cdp)
{ ProtectedByCriticalSection cs(&cs_client_list);
  ProtectedByCriticalSection cs2(&cs_short_term);
  for (int c=1;  c <= max_task_index;  c++)
    if (cds[c])
    { cdp_t acdp = cds[c];
      if (acdp->cevs.is_init)
        for (int i=0;  i<invoked_ev.count();  i++)
          for (int j=0;  j<acdp->cevs.reg_ev.count();  j++)
            if (!is_local_event(invoked_ev.acc0(i)->EventName) || !memcmp(cdp->sel_appl_uuid, acdp->sel_appl_uuid, UUID_SIZE))
              if (!strcmp(invoked_ev.acc0(i)->EventName, acdp->cevs.reg_ev.acc0(j)->EventName))
              { const char * inv_par = invoked_ev.acc0(i)->ParamStr;
                const char * reg_par = acdp->cevs.reg_ev.acc0(j)->ParamStr;
                if (acdp->cevs.reg_ev.acc0(j)->exact)
                { if (reg_par==NULL ? 
                       inv_par==NULL :
                       inv_par!=NULL && !strcmp(reg_par, inv_par))
                  { acdp->cevs.reg_ev.acc0(j)->count += invoked_ev.acc0(i)->count;
                    ReleaseSemaph(&acdp->cevs.event_semaphore, 1);
                  }
                }
                else // !exact
                  if (reg_par==NULL || inv_par!=NULL && !memcmp(reg_par, inv_par, strlen(reg_par)))
                  {// find the exact value of parameter
                    t_event_occurrence * occ = acdp->cevs.reg_ev.acc0(j)->occ;
                    unsigned occ_cnt = 0;
                    while (occ)
                    { if (inv_par==NULL ? !*occ->param : !strcmp(inv_par, occ->param))
                        break;
                      occ = occ->next;  occ_cnt++;
                    }
                    if (!occ)  // create if it does not exist
                    { if (occ_cnt>PARAM_VALUES_PER_EVENT_LIMIT) continue;
                      int len = inv_par ? (int)strlen(inv_par)+1 : 1;
                      occ=new(len) t_event_occurrence(inv_par);
                      if (!occ) continue;
                      occ->next = acdp->cevs.reg_ev.acc0(j)->occ;  acdp->cevs.reg_ev.acc0(j)->occ = occ;
                    }
                    occ->count += invoked_ev.acc0(i)->count;
                    acdp->cevs.reg_ev.acc0(j)->count += invoked_ev.acc0(i)->count; // total, used in wait_for_event
                    ReleaseSemaph(&acdp->cevs.event_semaphore, 1);
                  }
              }
    }
  cancel_events();
}

/**************************** interf init and close *************************/
CFNC int WINAPI interf_init(cdp_t cdp)
{ cdp->trace_enabled=trace_map(NOOBJECT);
  int retval=AddTask(cdp);
  if (retval!=KSE_OK) return retval;
  if (kernel_cdp_init(cdp))  // Must be called after AddTask!
    { RemoveTask(cdp);  return KSE_NO_MEMORY; }
  switch (cdp->in_use)
  { case PT_SLAVE:
#ifdef WINS  
      if (cdp->pRemAddr) // the status of this test is unclear
#endif
        RegisterReceiver(cdp);
      /* cont. */
    case PT_MAPSLAVE:
      cdp->waiting=the_sp.LockWaitingTimeout; // medium waiting
      slaveThreadCounter++;
      WriteSlaveThreadCounter();
      break;
    case PT_WORKER:
      cdp->prvs.set_user(DB_ADMIN_GROUP, CATEG_GROUP, TRUE);
      cdp->waiting=-1; // infinite waiting
      break;
  }
  char gcr[ZCR_SIZE];  write_gcr(cdp, gcr);
  cdp->session_number=*(uns32*)(gcr+ZCR_SIZE-sizeof(uns32));
  return KSE_OK;
}

CFNC void WINAPI cd_interf_close(cdp_t cdp)
// kernel_cdp_close called before this function
{ if (cdp->in_use==PT_SLAVE || cdp->in_use==PT_MAPSLAVE)
    slaveThreadCounter--;
  if (cdp->in_use==PT_SLAVE)
#ifdef WINS  
    if (cdp->pRemAddr) // the status of this test is unclear
#endif
      Unlink(cdp);
  RemoveTask(cdp);
  WriteSlaveThreadCounter(); // must be after RemoveTask because writes info affected by this function
}

/////////////////////////////////// WB semaphores //////////////////////////////////////////
/* WinBase semaphores are identified by its names. This is the native approach on Windows, 
     and the Windows handle of the semaphore is the WinBase handle of the semaphore.
   On UNIX the list of all semaphores has to be maintained. The list contains the name, the reference count,
     system-dependent semaphore data and integer value which represents the WinBase handle of the 
     semaphore.
   List of semaphores is protected by CS crs_sem. Invalid items have its reference count 0.
*/

#ifndef WINS
struct t_WB_sem
{ tobjname sem_name; // name identifying the semaphore
  unsigned ref_cnt;  // 0 iff the item is not used
  int _handle;       // unique handle, = position in the list + 1
 // system-dependent semaphore:
#ifdef UNIX
  sem_t semaphore;
#else // Netware
  LONG  semaphore;
#endif
  HANDLE handle(void) const  // returns handle to the semaphore 
    { return (HANDLE)(size_t)_handle; }
  int init(int init_value); /* 0 if OK */
  int close(void);
};

/* Platform dependent parts follow */
#ifdef UNIX
int t_WB_sem::init(int init_value)
{ return sem_init(&semaphore, 0, init_value); }

inline int t_WB_sem::close()
{ return sem_destroy(&semaphore); }   
#else
int init(int init_value)
{ semaphore=OpenLocalSemaphore(initial_value);
  return 0;
}

static inline int t_WB_sem::close()
{ return CloseLocalSemaphore(semaphore);
  return 0;
}   
#endif
    
SPECIF_DYNAR(t_WB_sem_dynar, t_WB_sem, 2);
// A cell of the dynar is in use iff ref_cnt>0.

t_WB_sem_dynar WB_sem_dynar;

#endif // !WINDOWS

void wbt_construct(void)
{
#ifndef WINS
  WB_sem_dynar.init();
#endif
}

HANDLE wbt_create_semaphore(const char * name, int initial_value) // returns 0 on error
{ 
#ifdef WINS
  char sem_name[14+OBJ_NAME_LEN+1];
  int2str(server_process_id, sem_name);  strcat(sem_name, "#");
  strmaxcpy(sem_name+strlen(sem_name), name, sizeof(tobjname));  sys_Upcase(sem_name);  
  return CreateSemaphore(NULL, initial_value, MAX_MAX_TASKS, sem_name);
#else
  tobjname sem_name;  strmaxcpy(sem_name, name, sizeof(tobjname));  sys_Upcase(sem_name);  
  t_WB_sem * WB_sem, * free_cell = NULL;  unsigned i;
  ProtectedByCriticalSection cs(&crs_sem);
 // allocate WB_sem:
  for (i=0;  i<WB_sem_dynar.count();  i++)
  { WB_sem = WB_sem_dynar.acc0(i);
    if (!WB_sem->ref_cnt) free_cell=WB_sem;
    else if (!strcmp(sem_name, WB_sem->sem_name))  // found, return it
    { WB_sem->ref_cnt++;
      return WB_sem->handle();
    }
  }
 // the semaphore was not found, create a new one
  if (free_cell) WB_sem=free_cell;
  else
  { WB_sem = WB_sem_dynar.next();
    if (WB_sem==NULL) return 0;  // no memory error
    WB_sem->_handle = WB_sem_dynar.count();  // generating the unique handle
  }
  strcpy(WB_sem->sem_name, sem_name);
 // create system semaphore (not necesaary to release WB_sem if fails):
  WB_sem->init(initial_value);
  WB_sem->ref_cnt=1;
  return WB_sem->handle();
#endif // !WINS
}

#ifndef WINS

/* Najdi t_WB_sem podle semaforu. Funkce neni chranena kritickou sekci, protoze
 * pokud bude s navracenou hodnotou volajici funkce pracovat, bude chtit
 * rozsirit ochranu na sirsi oblast. */

static t_WB_sem * find_semaphore(HANDLE hSemaphore)
{
  for (int i=0;  i<WB_sem_dynar.count();  i++){
	  t_WB_sem * WB_sem=WB_sem_dynar.acc0(i);
	  if (!WB_sem->ref_cnt) continue;
	  if (WB_sem->handle()==hSemaphore) return WB_sem;
  }
  dbg_err("Pokus o pristup k neexistujicimu semaforu");
  return NULL;
}  
#endif

/* Zavre semafor s danym handlem, pokud uz neni pouzivan, smaze jej. */
void wbt_close_semaphore(HANDLE hSemaphore) 
{
#ifdef WINS
  CloseHandle(hSemaphore);
#else
  ProtectedByCriticalSection cs(&crs_sem);
  t_WB_sem * WB_sem = find_semaphore(hSemaphore);
  if (WB_sem && !--WB_sem->ref_cnt)  // closed the last access, release the system semaphore
	  WB_sem->close();
#endif // !WINS
}

/* Zavre vsechny semafory */
void wbt_close_all_semaphores(void)
{ 
#ifndef WINS
  ProtectedByCriticalSection cs(&crs_sem);
  for (unsigned i=0;  i<WB_sem_dynar.count();  i++)
  { t_WB_sem * WB_sem = WB_sem_dynar.acc0(i);
    if (WB_sem->ref_cnt)  // semafor existuje a je pouzivan
    { WB_sem->ref_cnt=0;
      WB_sem->close();
    }
  }
#endif // !WINS
}

void wbt_release_semaphore(HANDLE hSemaphore) 
{
#ifdef WINS
  ReleaseSemaphore(hSemaphore, 1, NULL);
#else
  t_WB_sem *sem;
  { ProtectedByCriticalSection cs(&crs_sem);
    sem=find_semaphore(hSemaphore);
  } // teoreticky by tady mohlo dojit k preempci, jiny proces zrusit semafor atd.
  if (sem)
#ifdef UNIX
	  { hKernelLockEvent.lock();
		  sem_post(&sem->semaphore);
		  hKernelLockEvent.broadcast();
		  hKernelLockEvent.unlock();
	  }
#else  // Netware
	  SignalLocalSemaphore(sem->semaphore);
#endif
#endif // !WINS
}

#define WAIT_STEP 100

int wbt_wait_semaphore(HANDLE hSemaphore, int timeout)  // returns 0 iff OK, -1 when timeout, -2 on shutdown, -3 when bad semaphore (non-WIN)
{ int res;
  //cdp->wait_type=WAIT_SEMAPHORE;  // is outside
#ifdef WINS
  HANDLE arr[2];  arr[0]=hSemaphore;  arr[1]=hKernelLockEvent;
  res=WaitForMultipleObjects(2, arr, FALSE, timeout);
  if      (res==WAIT_OBJECT_0)   res= 0;  // obtained
  else if (res==WAIT_OBJECT_0+1) res=-2;  // shutdown or locking
  else if (res==WAIT_TIMEOUT)    res=-1;  // timeout
  else                           res=-2;  // terminating (error)
#else
  t_WB_sem *sem;
  { ProtectedByCriticalSection cs(&crs_sem);
    sem=find_semaphore(hSemaphore);
  } // teoreticky by tady mohlo dojit k preempci, jiny proces zrusit semafor atd.
  if (!sem) return -3;
#ifdef UNIX
  timespec ts;
  assert(timeout>=-1);
  if (timeout!=-1){
	  ms2abs(timeout, ts);
  }
  /* Cekaci smycka: pokud nemuzeme mit hned to, co chceme, cekame na jednu ze
   * tri veci, ktere mohou primet wait() skoncit: timeout, shutdown, nebo
   * uvolneni *libovolneho* semaforu. */
  hKernelLockEvent.lock();
  for (;;){
	  int rc;
	  if (!sem_trywait(&sem->semaphore)) {res=0; break;}
	  if (fShutDown || fLocking) { res=-2; break;} 
	  if (timeout==-1) rc=hKernelLockEvent.wait();
	  else rc=hKernelLockEvent.wait(ts);
	  if (rc && (errno==ETIMEDOUT||errno==EAGAIN)){ res=-1; break;}
  }
  hKernelLockEvent.unlock();
#else  // Netware
#ifdef STOP  // simple, but cannot recognize the shutdown
  if (timeout==-1)
  { WaitOnLocalSemaphore(sem->semaphore);
    res=0;
  }
  else
    res = !TimedWaitOnLocalSemaphore(sem->semaphore, timeout) ? 0 : -1;
#else
  int small_timeout;
  res=-1;  // timeout
  do
  { small_timeout = timeout==-1 || timeout >= WAIT_STEP ? WAIT_STEP : timeout;
    if (!TimedWaitOnLocalSemaphore(sem->semaphore, small_timeout)) // obtained
      { res=0;  break; }
    if (fShutDown || fLocking) { res=-2;  break; } // shutdown
    if (timeout!=-1) timeout-=small_timeout;
  } while (timeout);
#endif
#endif
#endif // !WINS
  //cdp->wait_type=WAIT_NO;  outside
  return res;
}

////////////////////////////////// odpojovani klientu /////////////////////////////////
void check_for_dead_clients(cdp_t cdp)
// ATTN: The database file may be closed!
{ unsigned stopped=0;
 // find and disconnect the dead clients:
  { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
    uns32 curr_tm  = stamp_now();
    for (int i=1;  i<=max_task_index;  i++)   /* system process 0 not counted */
      if (cds[i])
      { cdp_t acdp = cds[i];
        if (acdp->in_use==PT_SLAVE || acdp->in_use==PT_KERDIR)
          if (acdp->wait_type==WAIT_FOR_REQUEST)
          { uns32 last_req = acdp->last_request_time;  // prevents the change in last_request_time between two tests
            unsigned time_limit = acdp->conditional_attachment ? the_sp.kill_time_notlogged.val() : the_sp.kill_time_logged.val();
            if (time_limit &&                     // a limit is specified
                last_req < curr_tm &&             // unless a new request came after calling stamp_now() here
                curr_tm - last_req > time_limit)  // limit crossed
            {// check for passing the short month end:
              TIMESTAMP_STRUCT ts1, ts2;
              datim2TIMESTAMP(last_req, &ts1);
              datim2TIMESTAMP(curr_tm, &ts2);
              if (ts2.day==1 && ts1.month!=ts2.month)
                continue;
              StopTheSlave(acdp);
              stopped++;
            }
          }  
      }
  }
 // report dead clients outside the CS:
  while (stopped--)
  { char buf[81];
    display_server_info(form_message(buf, sizeof(buf), killingDeadUser));
  }
}
///////////////////////////// sharing countable licences among servers ///////////////////////////////
#ifdef WINS
BOOL t_shared_info::open(SECURITY_ATTRIBUTES * pSA)
{ BOOL new_file_mapping;
  hFileMapping = OpenFileMapping(FILE_MAP_WRITE, FALSE, "602SQLServerLicencePool");
  if (hFileMapping)
    new_file_mapping=FALSE;
  else
  { new_file_mapping=TRUE;
    hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, pSA, PAGE_READWRITE, 0, 1000, "602SQLServerLicencePool");
    if (!hFileMapping) return FALSE;
  }
  shared_info = MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
  if (shared_info==NULL) { CloseHandle(hFileMapping);  hFileMapping=0;  return FALSE; }
 // init the shared data, if created:
  if (new_file_mapping) *(char*)shared_info = 0;
  return TRUE;
}

void t_shared_info::close(void)
{ if (shared_info) { UnmapViewOfFile(shared_info);  shared_info=NULL; }
  if (hFileMapping) { CloseHandle(hFileMapping);  hFileMapping=0; }
}
#else
BOOL t_shared_info::open(SECURITY_ATTRIBUTES * pSA)
{
    static const key_t wbkey=0xaaacb603;
    int id=shmget(wbkey, 1000, 0666);
    if (-1==id) id=shmget(wbkey, 1000, IPC_CREAT + 0666);
    if (-1==id){
	    perror("shmget"); // problems on RH 7.1
	    shared_info=NULL;
	    return FALSE;
    }
    shared_info=shmat(id, NULL, 0);
    if (shared_info==(void *)-1){
	    perror("shmat");
	    shared_info=NULL;
	    return FALSE;
    }
    shmid_ds shmid;
    shmctl(id, IPC_STAT, &shmid);
    if (shmid.shm_nattch==1) *(char*)shared_info = 0;
    return TRUE;
}

void t_shared_info::close(void)
{ if (shared_info) { shmdt(shared_info);  shared_info=NULL; }
}
#endif

void t_shared_info_lic::add_lics(const char * ik, unsigned initial_licences)
{ char * lics = (char*)ptr();
#if WBVERS<900
  for (;  *lics;  lics+=MAX_LICENCE_LENGTH+2*sizeof(unsigned))
    if (!memcmp(ik, lics, MAX_LICENCE_LENGTH)) return;  // already stored
  memcpy(lics, ik, MAX_LICENCE_LENGTH);
#endif
  lics+=MAX_LICENCE_LENGTH;
  *(unsigned*)lics=initial_licences;  lics+=sizeof(unsigned);  // current licences
  *(unsigned*)lics=initial_licences;  lics+=sizeof(unsigned);  // original licences
  *lics=0;  // terminator
}

void t_shared_info_lic::update_total_lics(const char * ik, unsigned licences)
{ 
#if WBVERS<900
  for (char * lics = (char*)ptr();  *lics;  lics+=MAX_LICENCE_LENGTH+2*sizeof(unsigned))
    if (!memcmp(ik, lics, MAX_LICENCE_LENGTH))
#else
  char * lics = (char*)ptr();
#endif
    { unsigned * num = (unsigned*)(lics+MAX_LICENCE_LENGTH);
      if (licences>num[1]) num[0]+=licences-num[1];
      num[1]=licences;  // original licences
#if WBVERS<900
      break;
#endif
    }
}

bool t_shared_info_lic::get_licence(const char * ik)
{ char * lics = (char*)ptr();
#if WBVERS<900
  while (*lics)
  { if (!memcmp(ik, lics, MAX_LICENCE_LENGTH))
    { lics+=MAX_LICENCE_LENGTH;
      if (!*(unsigned*)lics) return false;
      (*(unsigned*)lics)--;
      return true;
    }
    lics+=MAX_LICENCE_LENGTH+2*sizeof(unsigned);
  }
#else
      lics+=MAX_LICENCE_LENGTH;
      if (!*(unsigned*)lics) return false;
      (*(unsigned*)lics)--;
      return true;
#endif
  return false;
}

void t_shared_info_lic::get_info(const char * ik, unsigned * total, unsigned * used)
{ char * lics = (char*)ptr();
#if WBVERS<900
  if (lics) while (*lics)
  { if (!memcmp(ik, lics, MAX_LICENCE_LENGTH)) 
    { lics+=MAX_LICENCE_LENGTH;
      *used  = *(unsigned*)lics;  lics+=sizeof(unsigned);  // current licences
      *total = *(unsigned*)lics;                           // original licences
      *used = *total - *used;
      return; 
    }
    lics+=MAX_LICENCE_LENGTH+2*sizeof(unsigned);
  }
  *total=*used=0;
#else
      lics+=MAX_LICENCE_LENGTH;
      *used  = *(unsigned*)lics;  lics+=sizeof(unsigned);  // current licences
      *total = *(unsigned*)lics;                           // original licences
      *used = *total - *used;
#endif
}

void t_shared_info_lic::return_licence(const char * ik)
{ char * lics = (char*)ptr();
#if WBVERS<900
  while (*lics)
  { if (!memcmp(ik, lics, MAX_LICENCE_LENGTH)) 
    { lics+=MAX_LICENCE_LENGTH;
      (*(unsigned*)lics)++;
      break; 
    }
    lics+=MAX_LICENCE_LENGTH+2*sizeof(unsigned);
  }
#else
      lics+=MAX_LICENCE_LENGTH;
      (*(unsigned*)lics)++;
#endif
}

t_shared_info_lic server_shared_info;
/////////////////////////////////////// server info ////////////////////////////////////////////
#if WBVERS>=1100
#include <wcs.h>
#include <wcs.cpp>

void read_addon_licences(void)
{ 
#if (BLD_FLAGS & 1)
  fulltext_allowed=xml_allowed=true;
#else
  void * licence_handle;
  fulltext_allowed=xml_allowed=false;
  char licdir[MAX_PATH];  
  get_path_prefix(licdir);
#ifdef LINUX
  strcat(licdir, "/lib/" WB_LINUX_SUBDIR_NAME); 
#endif
  int res=licence_init(&licence_handle, SOFT_ID, licdir, "en-US", 1, NULL, NULL);
  if (licence_handle)
  { trial_lics_added=false;
   // read/write the installation date:
    const char *installDate; 
    res=getInstallDate(licence_handle, &installDate);
    if (res==LICENCE_OK || res==PROB_NOT_ACTIVATED)
      if (installDate)
        if (!memcmp(installDate, "1900-", 5))
        { markInstallation(SOFT_ID, licdir);
          reg_date=dt_plus(Today(), 30);
        }  
        else
          if (sql92_str2date(installDate, &reg_date))
            reg_date=dt_plus(reg_date, 30);
   // read licence information:
    char * licence;
    res=getLicenceSummedAK(licence_handle, &licence, 1);
    if (res!=LICENCE_OK)
    { if (res!=PROB_NOT_ACTIVATED && res!=BAD_PARAMETER)  // initial or interminnent state
        dbg_err(WCGetLastErrorA(res, licence_handle, "en-US"));       
    }    
    else
    { if (strstr(licence, "XMLExtension"))
        xml_allowed=true;
      if (strstr(licence, "FulltextSystem"))
        fulltext_allowed=true;
    }
    licence_close(licence_handle);
   // if there is no valid licence, check for TRIAL period:
    if (!xml_allowed && !fulltext_allowed)
      if (reg_date)  // installation date or the date of the first run found
      { int remaining_trial_days = dt_diff(reg_date, Today());
        if (remaining_trial_days>=0)
        { xml_allowed=fulltext_allowed=true;
          trial_lics_added=true;
        }  
      }  
  }  
  else if (res!=DLL_RELOAD_FAIL)  // do not report anything if the wc librry is missing
    dbg_err("The wc library has not been initialized.");
#endif  
}
#endif

unsigned count_cursors(t_client_number applnum)
{ int crnm;  unsigned count=0;
  ProtectedByCriticalSection cs(&crs_sem);
  for (crnm=0;  crnm<crs.count();  crnm++)
  { cur_descr * cd = *crs.acc0(crnm);
    if (cd!=NULL)
      if (applnum == (t_client_number)-1 || cd->owner == applnum) 
        count++;
  }
  return count;
}

BOOL get_server_info(cdp_t cdp, int info_type, uns32 * result)
{ switch (info_type)
  { case OP_GI_SERVER_PLATFORM:
      *result=CURRENT_PLATFORM;  break;
    case OP_GI_REPLICATION:
      *result=GetReplInfo();     break;
    case OP_GI_LICS_CLIENT:
    { unsigned total, used;
      client_licences.get_info(&total, &used);
      *result=total;             break;
    }
    case OP_GI_LICS_WWW:
      *result=TRUE;              break;
    case OP_GI_LICS_FULLTEXT:            // enables the XML extension too, in 8.1
      *result=fulltext_allowed;  break;  // always allowed 9.0-10.0
#if WBVERS>=900
    case OP_GI_LICS_XML:
      *result=xml_allowed;       break;  // since version 11
#endif      
    case OP_GI_LICS_SERVER:
#if WBVERS<900
      *result=TRUE;
#else
      *result=network_server_licence && !trial_lics_added;  
#endif
      break;  // always true before 9.0
    case OP_GI_TRIAL_ADD_ON:  // used since v. 11, too
      *result=0;
      if (reg_date && trial_lics_added) *result = dt_diff(reg_date, Today()) >= 0;
      break;
    case OP_GI_TRIAL_FULLTEXT:
      *result=0;
      if (reg_date && trial_fulltext_added)
      { uns32 reg_date2 = dt_plus(reg_date, 60);  // 60 days added above 30
        *result = dt_diff(reg_date2, Today()) >= 0;
      }
      break;
    case OP_GI_TRIAL_REM_DAYS:
      *result=0;
      if (reg_date)
      { int remaining_trial_days = dt_diff(reg_date, Today());
        if (remaining_trial_days>=0)
          *result=remaining_trial_days;
      }
      break;

    case OP_GI_VERSION_1:
      *result=VERS_1;            break;
    case OP_GI_VERSION_2:
      *result=VERS_2;            break;
    case OP_GI_VERSION_3:
      *result=VERS_3;            break;
    case OP_GI_VERSION_4:
      *result=VERS_4;            break;
    case OP_GI_PID:
      *result=server_process_id; break;
    case OP_GI_LICS_USING:
      *result=client_licences.licences_consumed;  break;
    case OP_GI_CLUSTER_SIZE:
      *result=BLOCKSIZE;  break;
    case OP_GI_DISK_SPACE:
#ifdef WINS
    { char lpszRootPathName[4] = "A:\\";
      lpszRootPathName[0]=ffa.last_fil_path()[0];
      DWORD SectorsPerCluster, BytesPerSector, FreeClusters, Clusters;
      if (!GetDiskFreeSpace(lpszRootPathName, &SectorsPerCluster, &BytesPerSector, &FreeClusters, &Clusters))
        *result=0xffffffffU;  // error when last_fil_path() is relative
      else  
      { unsigned _int64 space = (unsigned _int64)FreeClusters*SectorsPerCluster*BytesPerSector/1024;
        if (space > 0xffffffffU) space = 0xffffffffU;
        *result=(uns32)space;
      }  
    }
#endif  // WINS
#ifdef UNIX
    { struct statfs st;
      *result = !statfs(ffa.last_fil_path(), &st) ? (uns32)((uns64)st.f_bavail * st.f_bsize / 1024) : 0xffffffffU;;
    }
#endif
      break;
    case OP_GI_OWNED_CURSORS:
      *result = count_cursors(cdp->applnum_perm);  break;
    case OP_GI_FIXED_PAGES:
    { unsigned pages, fixes, frames;
      get_fixes(&pages, &fixes, &frames);
      *result=pages;
      break;
    }
    case OP_GI_FIXES_ON_PAGES:
    { unsigned pages, fixes, frames;
      get_fixes(&pages, &fixes, &frames);
      *result=fixes;
      break;
    }
    case OP_GI_FRAMES:
    { unsigned pages, fixes, frames;
      get_fixes(&pages, &fixes, &frames);
      *result=frames;
      break;
    }
    case OP_GI_FREE_CLUSTERS:
      *result=disksp.free_blocks_count(cdp);  break;
    case OP_GI_USED_MEMORY:
      *result=total_used_memory();  break;
    case OP_GI_INSTALLED_TABLES:
    { uns16 installed_tables, locked_tables, sum_table_locks, temp_tables;
      tables_stat(&installed_tables, &locked_tables, &sum_table_locks, &temp_tables);
      *result=installed_tables;  break;
    }
    case OP_GI_LOCKED_TABLES:
    { uns16 installed_tables, locked_tables, sum_table_locks, temp_tables;
      tables_stat(&installed_tables, &locked_tables, &sum_table_locks, &temp_tables);
      *result=locked_tables;  break;
    }
    case OP_GI_TABLE_LOCKS:
    { uns16 installed_tables, locked_tables, sum_table_locks, temp_tables;
      tables_stat(&installed_tables, &locked_tables, &sum_table_locks, &temp_tables);
      *result=sum_table_locks;  break;
    }
    case OP_GI_TEMP_TABLES:
    { uns16 installed_tables, locked_tables, sum_table_locks, temp_tables;
      tables_stat(&installed_tables, &locked_tables, &sum_table_locks, &temp_tables);
      *result=temp_tables;  break;
    }
    case OP_GI_OPEN_CURSORS:
      *result = count_cursors((t_client_number)-1);  break;
    case OP_GI_PAGE_LOCKS:
      *result=page_locks_count();  break;
    case OP_GI_LOGIN_LOCKS:
      *result=login_locked;  break;
    case OP_GI_CLIENT_NUMBER:
      *result=cdp->applnum_perm;  break;
    case OP_GI_BACKGROUD_OPER:
      *result = fLocking ? 0 : 1;  break;
    case OP_GI_SYS_CHARSET:
      *result = sys_spec.charset;  break;
    case OP_GI_MINIMAL_CLIENT_VERSION:
#if WBVERS>=950
      *result = /*WBVERS;*/  810;
#else
      *result = 500;
#endif
      break;
    case OP_GI_SERVER_UPTIME:
      *result = (uns32)(time(NULL) - server_start_time);  break;  // since MSW .NET 2005 time_t is uns64
    case OP_GI_PROFILING_ALL:
      *result = profiling_all;  break;
    case OP_GI_PROFILING_LINES:
      *result = profiling_lines;  break;
    case OP_GI_HTTP_RUNNING:
      *result = (wProtocol & TCPIP_HTTP) != 0;  break;
    case OP_GI_WEB_RUNNING:
      *result = (wProtocol & TCPIP_WEB) != 0;  break;
    case OP_GI_NET_RUNNING:
      *result = (wProtocol & TCPIP_NET) != 0;  break;
    case OP_GI_IPC_RUNNING:
#ifdef LINUX
      *result = 0;  break;
#else
      *result = hThreadMakerThread!=0;  break;
#endif
    case OP_GI_BACKING_UP:
      *result = backup_status;  break;
    case OP_GI_CLIENT_COUNT:  
      *result = get_external_client_connections();  break;
    case OP_GI_DAEMON:
      *result = is_service;  break;
    case OP_GI_LEAKED_MEMORY:
      *result=16*leak_test();  break;
    default:
      return FALSE;
  }
  return TRUE;
}
////////////////////////////////// offline thread list ////////////////////////////////////
t_offline_threads offline_threads;

bool t_offline_threads::add(THREAD_HANDLE thread_handle)
{ ProtectedByCriticalSection cs(&cs_short_term);
  for (int i = 0;  i<thread_handle_dynar.count();  i++)
    if (*thread_handle_dynar.acc0(i) == INVALID_THANDLE_VALUE)
      { *thread_handle_dynar.acc0(i) = thread_handle;  return true; }
  THREAD_HANDLE * p_thread_handle = thread_handle_dynar.next();
  if (!p_thread_handle) return false;
  *p_thread_handle = thread_handle;
  return true;
}

void t_offline_threads::remove(THREAD_HANDLE thread_handle)
{ if (joining_on_exit) return;  // must not close handles because the main thread wait for them to be signalled
  ProtectedByCriticalSection cs(&cs_short_term);
  for (int i = 0;  i<thread_handle_dynar.count();  i++)
    if (*thread_handle_dynar.acc0(i) == thread_handle)
    { THREAD_DETACH(*thread_handle_dynar.acc0(i));  // makes the thread detached, can exit without any lost of resources
      *thread_handle_dynar.acc0(i) = INVALID_THANDLE_VALUE;
    }
}

void t_offline_threads::join_all(void)
{ ProtectedByCriticalSection cs(&cs_short_term);
  bool wait_reported_to_log = false;
  joining_on_exit=true;
  for (int i = 0;  i<thread_handle_dynar.count();  i++)
    if (*thread_handle_dynar.acc0(i) != INVALID_THANDLE_VALUE)
    { LeaveCriticalSection(&cs_short_term);
      if (!wait_reported_to_log)
      { char buf[81];
        trace_msg_general(0, TRACE_START_STOP, form_message(buf, sizeof(buf), waitingForOfflineThread), NOOBJECT);
        wait_reported_to_log=true;
      }
      THREAD_JOIN(*thread_handle_dynar.acc0(i));  // must not be in CS, otherwise the thread stops in remove() and cannot terminate and join
      EnterCriticalSection(&cs_short_term);
      *thread_handle_dynar.acc0(i) = INVALID_THANDLE_VALUE;
    }
}

/////////////////////////////////// periodical server operations //////////////////////////////
class integr_check_plan
{ enum { READ_PARAM_INTERVAL=300, INTEGR_TIME_COUNT=4 };
  unsigned backup_interval;       // in seconds
  unsigned backup_time[INTEGR_TIME_COUNT]; // seconds since midnight
  unsigned backup_day [INTEGR_TIME_COUNT]; // 0-no backup, 1-daily, 2-Monday etc.

 public:
  BOOL     backup_done[INTEGR_TIME_COUNT];
  void read_backup_params(void);
  integr_check_plan(void)
  {// if the current time is in the time window for a check, disable it (otherwise restarting the server winthin the window would re-start the check)
    time_t long_time;  struct tm * tmbuf;
    long_time=time(NULL);  tmbuf=localtime(&long_time);
    if (!tmbuf) return; // date > 2040
    uns32 clk = 60*(60*tmbuf->tm_hour+tmbuf->tm_min)+tmbuf->tm_sec; // seconds since midnight
    unsigned int day=tmbuf->tm_wday;  if (!day) day=7;  day++;  // 2..8
    for (int i=0;  i<INTEGR_TIME_COUNT;  i++)
      if (backup_day[i])
      { backup_done[i] = clk >= backup_time[i] && clk < backup_time[i]+300 && 
          (backup_day[i]==1 || day==backup_day[i] || backup_day[i]==9 && day>=2 && day<=6 || backup_day[i]==10 && day>=7 && day<=8);
  //       denne               prave dnes            pracovni dny                            weekend
      }
      else backup_done[i]=FALSE;
  }
  bool check_for_check(cdp_t cdp, int & index);
};

void integr_check_plan::read_backup_params(void)
// Precte parametry. ATTN: The database file may be closed!
{// interval: 
  backup_interval=the_sp.integr_check_interv.val();
 // backup time:
  backup_day[1]=the_sp.IntegrTimeDay1.val();
  if (backup_day[1]) backup_time[1] = 60*(60*the_sp.IntegrTimeHour1.val()+the_sp.IntegrTimeMin1.val());
  backup_day[2]=the_sp.IntegrTimeDay2.val();
  if (backup_day[2]) backup_time[2] = 60*(60*the_sp.IntegrTimeHour2.val()+the_sp.IntegrTimeMin2.val());
  backup_day[3]=the_sp.IntegrTimeDay3.val();
  if (backup_day[3]) backup_time[3] = 60*(60*the_sp.IntegrTimeHour3.val()+the_sp.IntegrTimeMin3.val());
  backup_day[4]=the_sp.IntegrTimeDay4.val();
  if (backup_day[4]) backup_time[4] = 60*(60*the_sp.IntegrTimeHour4.val()+the_sp.IntegrTimeMin4.val());
}

bool integr_check_plan::check_for_check(cdp_t cdp, int & index)
{// prevent check while replaying journal (both change the header.closetime)
  if (replaying) return false;
 // periodical re-read of backup parameters:
  read_backup_params();
 // check the backup interval:
  timestamp stamp = stamp_now();
  if (backup_interval && integrity_check_planning.last_check_or_server_startup < stamp-backup_interval+25) // testing every 50 seconds, can do it 25 seconds before
    if (!integrity_check_planning.running_threads_counter) 
      if (integrity_check_planning.try_starting_check(false, false)) // try to start the weak check:
      { index=-1;
        return true;
      }
 // check for the backup time:
  time_t long_time;  struct tm * tmbuf;
  long_time=time(NULL);  tmbuf=localtime(&long_time);
  if (!tmbuf) return false; // date > 2040
  uns32 clk = 60*(60*tmbuf->tm_hour+tmbuf->tm_min)+tmbuf->tm_sec; // seconds since midnight
  unsigned int day=tmbuf->tm_wday;  if (!day) day=7;  day++;  // 2..8
  for (int i=0;  i<INTEGR_TIME_COUNT;  i++)
    if (backup_day[i])
    { BOOL active = clk >= backup_time[i] && clk < backup_time[i]+15*60 && // 15 minutes time window
        (backup_day[i]==1 || day==backup_day[i] || backup_day[i]==9 && day>=2 && day<=6 || backup_day[i]==10 && day>=7 && day<=8);
//       denne               prave dnes            pracovni dny                            weekend
      if (!active) backup_done[i]=FALSE;
      else if (!backup_done[i])
        if (!integrity_check_planning.running_threads_counter) 
          if (integrity_check_planning.try_starting_check(false, false)) // try to start the weak check:
          { index=i;  // backup_done[i]=TRUE; -- will be done if not cancelled
            return true;
          }
    }
  return false;
}

integr_check_plan the_integr_check_plan;

void periodical_actions(cdp_t cdp)
{// skip the action if the server is locked (consistency repair, compacting, explicit locking):
  if (login_locked) return;
 // do it:
  back_end_operation_on=TRUE;  
  bool breaked=false;
  start_planned_actions(cdp);
  check_for_fil_backup(my_cdp); 
  check_for_dead_clients(my_cdp);
 // integrity checking:
  int index;
  if (the_integr_check_plan.check_for_check(my_cdp, index))
  //if (integrity_check_planning.timer_event())
  { ffa.open_fil_access(my_cdp, server_name);  // important when (the_sp.CloseFileWhenIdle.val()!=0)
   // call the vetoing trigger:
    t_value res;  
    if (find_system_procedure(cdp, "_ON_SYSTEM_EVENT"))
    { tobjnum orig_user = cdp->prvs.luser();
      cdp->prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, TRUE);  // execute with CONF_ADMIN privils (hierarchy is necessary, I need to be the Admin os _sysext)
      if (!exec_direct(cdp, "CALL _SYSEXT._ON_SYSTEM_EVENT(2,0,\'\')", &res)) commit(cdp);
      cdp->prvs.set_user(orig_user, CATEG_USER, TRUE);  // return to original privils (may be DB_ADMIN)
      commit(cdp);
      cdp->request_init();
    }
    else res.intval = 0;
    if (res.intval!=-1)  // unless Veto
    { int result = 0;
      if (the_sp.integr_check_extent.val() & INTEGR_CHECK_CONSISTENCY)
      { uns32 counts[5];  di_params di;
        di.lost_blocks   = counts+0;
        di.lost_dheap    = counts+1;
        di.nonex_blocks  = counts+2;
        di.cross_link    = counts+3;
        di.damaged_tabdef= counts+4;
        if (db_integrity(my_cdp, FALSE, &di)) breaked=true;
        else
        { if (*di.lost_blocks)    result |= INTEGR_CHECK_CONSISTENCY_LOST_BLOCK;
          if (*di.lost_dheap)     result |= INTEGR_CHECK_CONSISTENCY_LOST_PIECE;
          if (*di.nonex_blocks)   result |= INTEGR_CHECK_CONSISTENCY_NONEX_BLOCK;
          if (*di.cross_link)     result |= INTEGR_CHECK_CONSISTENCY_CROSS_LINK;
          if (*di.damaged_tabdef) result |= INTEGR_CHECK_CONSISTENCY_DAMAGED_TABLE;
        }
        result &= the_sp.integr_check_extent.val();
      }
      integrity_check_planning.check_completed(); // opens access for other clients
      if (!integrity_check_planning.last_check_cancelled)
      { if (the_sp.integr_check_extent.val() & INTEGR_CHECK_INDEXES)
        { int cache_error, index_error;
          check_all_indicies(cdp, false, INTEGR_CHECK_INDEXES|INTEGR_CHECK_CACHE, false, cache_error, index_error);
          if (index_error) result |= INTEGR_CHECK_INDEXES;
          if (cache_error) result |= INTEGR_CHECK_CACHE;
        }
        if (index>=0)  // check AT given time
          the_integr_check_plan.backup_done[index]=TRUE;
      }
     // start the trigger or write a message to the log (on errors):
      if (!breaked && !integrity_check_planning.last_check_cancelled)
        if (find_system_procedure(cdp, "_ON_CONSISTENCY_ERROR"))
        { char buf[40+MAX_PATH];
          sprintf(buf, "CALL _SYSEXT._ON_CONSISTENCY_ERROR(%d)", result);
          tobjnum orig_user = cdp->prvs.luser();
          cdp->prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, TRUE);  // execute with CONF_ADMIN privils (hierarchy is necessary, I need to be the Admin os _sysext)
          exec_direct(cdp, buf);
          cdp->prvs.set_user(orig_user, CATEG_USER, TRUE);  // return to original privils (may be DB_ADMIN)
          commit(cdp);
          cdp->request_init();
        }
        else // trigger does not exist
          if (result)
          { if (result & INTEGR_CHECK_CONSISTENCY) dbg_err(msg_integrity);
            if (result & INTEGR_CHECK_INDEXES)     dbg_err(msg_index_error);
          }
    }
    else  // check vetoed
    { integrity_check_planning.check_completed(); // opens access for other clients
      if (index>=0)  // check AT given time
        the_integr_check_plan.backup_done[index]=TRUE;  // disable immediate restaring
    }
    ffa.close_fil_access(my_cdp);
  }
  back_end_operation_on=FALSE; 
}

//////////////////////////////// backup timer /////////////////////////////////////////
#ifdef WINS
THREAD_PROC(PeriodicalActionsThread)
{
  while (!fShutDown)
  { if (WaitForSingleObject(hKernelShutdownEvent, 50000) != WAIT_TIMEOUT)
      break;
    if (!fLocking) periodical_actions(my_cdp);
  }
  THREAD_RETURN;
}

static THREAD_HANDLE hPeriodicalActionsThread = (THREAD_HANDLE)NULL;

void start_thread_for_periodical_actions(void)
{
  THREAD_START(PeriodicalActionsThread, 40000, NULL, &hPeriodicalActionsThread);
}
#endif

#include "filbckup.cpp"
#include "creplay.cpp"
