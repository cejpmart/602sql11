// communication log replay
/*
Pri praci serveru se vytvari log pozadavku klientu prichazejicich na server, pokud je pro databazi specifikovano jmeno adresare pro log
v udaji CommJournalPath. Log se zapisuje do tohoto souboru zpristupneneho pres handle ffa.c_handle, pokud handle neni INVALID_HANDLE_VALUE.
Kazdy pozadavek v logu zacina hlavickou t_reqhdr. 

Po kazdem spusteni severu se vytvori hlavicka pozadavku s nulovou delkou s client_number==0xffff, coz pri prehravani znamena 
ukonceni vsech klientu.

Pri prehravani se sekvencne ctou pozadavky z logu a predavaji se paralelne bezicim vlaknum. Beh vlaken se synchronizuje dvojici udalosti.
Pozadavek se predava v p_request, pozadavek NULL znamena signal k ukonceni vlakna. Pozadavek je dealokovan vlaknem po provedeni.

Pole slaves pri prehravani je indexovano cislem klienta, ktery zapsal pozadavek do logu. Kazdemu prehravocimu slave
*/

#include <stdlib.h>

BOOL comm_log_replay = FALSE; // replay the communications log and stop

struct t_reqhdr // header of every communication log item
{ uns16 client_number;  // cislo slave, ktery dostal pozadavek, nebo 0xffff indikujici restart serveru
  int request_length;   // number of bytes following this header, may be 0 for slave termination
};

/////////////////////////////////////////// creating the log ///////////////////////////////////
// Only this functions write to the log. None of them is called when replaying the log.

void t_ffa::log_client_restart(void) // log the server start, initialize the log if empty
{ DWORD wr, pos=SetFilePointer(c_handle, 0, NULL, FILE_END);  // position on the end of the journal
  if (!pos)
    WriteFile(c_handle, "WBCOMMLOG\0\x1\0", 12, &wr, NULL);
  t_reqhdr reqhdr;
  reqhdr.client_number  = 0xffff;
  reqhdr.request_length = 0;
  WriteFile(c_handle, &reqhdr, sizeof(t_reqhdr),           &wr, NULL);
}

void t_ffa::log_client_request(cdp_t cdp) // log the request
{ DWORD wr;  t_reqhdr reqhdr;
  ProtectedByCriticalSection cs(&c_log_cs);
  reqhdr.client_number  = cdp->applnum_perm;
  reqhdr.request_length = cdp->msg._total();
  WriteFile(c_handle, &reqhdr, sizeof(t_reqhdr),           &wr, NULL);
  WriteFile(c_handle, cdp->msg._set(), reqhdr.request_length, &wr, NULL);
}

void t_ffa::log_client_termination(cdp_t cdp) // log client termination
{ DWORD wr;  t_reqhdr reqhdr;
  ProtectedByCriticalSection cs(&c_log_cs);
  reqhdr.client_number  = cdp->applnum_perm;
  reqhdr.request_length = 0;
  WriteFile(c_handle, &reqhdr, sizeof(t_reqhdr),           &wr, NULL);
}

BOOL t_ffa::c_log_read(void * buf, DWORD req_length)
{ DWORD rd;
  return ReadFile(c_handle, buf, req_length, &rd, NULL) && rd==req_length;
}

BOOL t_ffa::c_log_prepare(void)
{ char buf[12];  DWORD rd;
  SetFilePointer(c_handle, 0, NULL, FILE_BEGIN);
  return ReadFile(c_handle, buf, 12, &rd, NULL) && rd==12 && !memcmp(buf, "WBCOMMLOG", 10);
}
////////////////////////////////////////// replaying the log ///////////////////////////////////

static BOOL replaying_error;  // global error indicator

struct t_slave
{ THREAD_HANDLE thr;    // slave thread handle or INVALID_HANDLE_VALUE if not running
  LocEvent new_req;       // event starting the request processing
  LocEvent req_completed; // event indicating the request processing completion, slave waiting for the new request
  tptr p_request;         // request pointer; valid on new_req, released on req_completed
  t_slave(void)
  { thr=INVALID_THANDLE_VALUE;  
    SetLocEventInvalid(new_req);  SetLocEventInvalid(req_completed);  
    p_request=NULL; 
  }
  BOOL start(void);  // returns TRUE if started OK
  void terminate_slave(void); // terminates the specified slave and releases all its resources
  void new_slave_request(int req_length);
};

#define MAX_SLAVES 100  // maximal number of conurrent slaves


THREAD_PROC(replaying_slave)  // joinable thread
{ t_slave * me = (t_slave*)data;
  cd_t my_cd(PT_MAPSLAVE);  cdp_t cdp=&my_cd;
 // connect to the server:
  int res = interf_init(cdp);
  if (res != KSE_OK)
  { client_start_error(cdp, workerThreadStart, res);
    replaying_error=TRUE;  THREAD_RETURN;
  }
 // processing the requests:
  do
  { WaitLocalAutoEvent(&me->new_req, INFINITE);
    if (!me->p_request) break;  // termination request
   // process the new request:
    new_request(cdp, (request_frame*)me->p_request);
    cdp->answer.free_answer(); // release the answer, ignored
    free(me->p_request);  me->p_request=NULL;
    SetLocalAutoEvent(&me->req_completed);
  } while (TRUE);
 // disconnect from the server:
  kernel_cdp_free(cdp);
  cd_interf_close(cdp);
  THREAD_RETURN;
}

BOOL t_slave::start(void)  // returns TRUE if started OK
{ CreateLocalAutoEvent(FALSE, &new_req);
  CreateLocalAutoEvent(TRUE,  &req_completed);
  p_request=NULL;
  return THREAD_START_JOINABLE(::replaying_slave, 60000, this, &thr); // will join
}

void t_slave::terminate_slave(void) // terminates the specified slave and releases all its resources
{ WaitLocalAutoEvent(&req_completed, 600000); // 10 minutes, wait for slave to complete the previous request
  p_request=NULL; // should not be necessary
  SetLocalAutoEvent(&new_req);  // NULL request terminates the slave
#ifdef WINS
  WaitForSingleObject(thr, 10000);  // 10 sec wait for termination
  CloseHandle(thr);
#else
#ifdef UNIX
  pthread_join(thr, NULL);
#else
  Sleep(2000);
#endif
#endif
  CloseLocalAutoEvent(&req_completed);
  CloseLocalAutoEvent(&new_req);
  thr=INVALID_THANDLE_VALUE;  
  SetLocEventInvalid(new_req);  SetLocEventInvalid(req_completed);  
}

void t_slave::new_slave_request(int req_length)
// axiom: req_length>0
{ tptr buf;
 // allocate the buffer:
  buf=(tptr)malloc(req_length);
  if (!buf)
  { dbg_err("Buffer allocation error");
    replaying_error=TRUE;  return;
  }
 // read the request:
  if (!ffa.c_log_read(buf, req_length))
    { dbg_err("Error reading the request");  replaying_error=TRUE;  return; }
 // start processing the request:
  WaitLocalAutoEvent(&req_completed, INFINITE);  // wait for slave to complete the previous request
  p_request=buf;
  SetLocalAutoEvent(&new_req);
}

#ifdef ANY_SERVER_GUI
static THREAD_PROC(communic_replay_2)  // detached thread
{ t_slave slaves[MAX_SLAVES];  int i;
  LocEvent * p_replay_completed = (LocEvent*)data;
  replaying_error=FALSE;
  {// cycle on requests from the file: 
    t_reqhdr reqhdr;  
    while (!replaying_error && ffa.c_log_read(&reqhdr, sizeof(t_reqhdr)))
    { if (reqhdr.client_number==0xffff) // server restarted, terminate all slaves
      { for (i=0;  i<MAX_SLAVES;  i++)
          if (slaves[i].thr!=INVALID_THANDLE_VALUE) slaves[i].terminate_slave();
      }
      else // normal requst for a slave defined by client_number
      { t_slave * the_slave = &slaves[reqhdr.client_number];
       // start the slave if not running:
        if (the_slave->thr==INVALID_THANDLE_VALUE) // must create the new slave
          if (!the_slave->start()) 
            { replaying_error=TRUE;  continue; }
       // process the request or terminate the slave
        if (reqhdr.request_length)
             the_slave->new_slave_request(reqhdr.request_length);
        else the_slave->terminate_slave();
      }
    }
   // terminate all slaves:
    for (i=0;  i<MAX_SLAVES;  i++)
      if (slaves[i].thr!=INVALID_THANDLE_VALUE) slaves[i].terminate_slave();
    Sleep(3000);
  }
#ifdef WINS
  //DestroyWindow(hwMain);  -- does not work, different thread
  SendMessage(hwMain, WM_CLOSE, 0, 0);
#else
  SetLocalAutoEvent(p_replay_completed);
#endif
  THREAD_RETURN;
}
#endif
  
void communic_replay(void)
{// check the log file:
#ifdef ANY_SERVER_GUI
  display_server_info("Replaying communications log file");
  if (!ffa.c_log_prepare())
    dbg_err("Error in log file header");
  else
  { LocEvent replay_completed;  
    CreateLocalAutoEvent(FALSE, &replay_completed);
    THREAD_START(::communic_replay_2, 10000, &replay_completed, NULL);  // thread handle closed inside
#ifdef WINS
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
      if (!hInfoWnd || !IsDialogMessage(hInfoWnd, &msg))
      { TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
#else
    WaitLocalAutoEvent(&replay_completed, INFINITE);
#endif
    CloseLocalAutoEvent(&replay_completed);
    display_server_info("Replaying completed");
  }
#endif // ANY_SERVER_GUI
}
