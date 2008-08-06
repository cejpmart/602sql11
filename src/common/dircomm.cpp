// dircomm.cpp - direct client-server communication support

void make_name(char * name, const char * pattern, const char * server_name, DWORD id, bool global_name)
{ if (global_name)
    { strcpy(name, "Global\\");  name+=7; }
#if WBVERS<950
  sprintf(name, pattern, 8, server_name, id);  
#else
  sprintf(name, pattern, 95, server_name, id);  
#endif
  Upcase(name);
}

bool CreateIPComm0(const char * server_name, SECURITY_ATTRIBUTES * pSA, HANDLE * hMakerEvent, HANDLE * hMakeDoneEvent, HANDLE * hMemFile, DWORD ** comm_data, bool & mapped_long, bool try_global)
// Creates a link between client and server processes used for starting slave threads on the server and 
//  recognizing the termination of the other process.

{ char nameEvent[35+OBJ_NAME_LEN];  bool global_name;
  global_name = try_global;  mapped_long=false;
  make_name(nameEvent, "share%d_wbmakerevent%s", server_name, 0, global_name);
  *hMakerEvent = CreateEvent(pSA, FALSE, FALSE, nameEvent);
  if (!*hMakerEvent && global_name)    // used on W95 and similiar, where no global names are available
  { global_name=false;
    make_name(nameEvent, "share%d_wbmakerevent%s", server_name, 0, global_name);
    *hMakerEvent = CreateEvent(pSA, FALSE, FALSE, nameEvent);
  }
  if (*hMakerEvent)
	{ make_name(nameEvent, "share%d_wbmakedoneevent%s", server_name, 0, global_name);
    *hMakeDoneEvent = CreateEvent(pSA, FALSE, FALSE, nameEvent);
    if (*hMakeDoneEvent)
	  { make_name(nameEvent, "share%d_wbmmf%s", server_name, 0, global_name);
      *hMemFile = CreateFileMapping(INVALID_HANDLE_VALUE, pSA, PAGE_READWRITE, 0, 2*sizeof(DWORD), nameEvent);
      if (*hMemFile) 
      { *comm_data = (DWORD*)MapViewOfFile(*hMemFile, FILE_MAP_WRITE, 0, 0, 2*sizeof(DWORD));
        if (*comm_data)
          mapped_long=true;
        else 
          *comm_data = (DWORD*)MapViewOfFile(*hMemFile, FILE_MAP_WRITE, 0, 0, sizeof(DWORD));
      }
      else
      { GetLastError();
        *hMemFile = CreateFileMapping(INVALID_HANDLE_VALUE, pSA, PAGE_READWRITE, 0, sizeof(DWORD), nameEvent);
        if (*hMemFile) 
          *comm_data = (DWORD*)MapViewOfFile(*hMemFile, FILE_MAP_WRITE, 0, 0, sizeof(DWORD));
      }
      if (*hMemFile && *comm_data)
        return true; // success
      if (*hMemFile) CloseHandle(*hMemFile);
      CloseHandle(*hMakeDoneEvent);
    }
    CloseHandle(*hMakerEvent);
  }
	return false; // failed
}

void CloseIPComm0(HANDLE hMakerEvent, HANDLE hMakeDoneEvent, HANDLE hMemFile, DWORD * comm_data)
{ UnmapViewOfFile(comm_data);
  CloseHandle(hMemFile);
  CloseHandle(hMakeDoneEvent);
  CloseHandle(hMakerEvent);
}

bool t_dircomm::OpenClientSlaveComm(DWORD id, const char * server_name, SECURITY_ATTRIBUTES * pSA, bool try_global)
// Create a link between client and the slave thread on the server.
// Alocates 2*client_server_mapping_size bytes of memory in the page file (using SEC_RESERVE does not change this)
//   Big [client_server_mapping_size] may cause problems with page file size when there are many direct clients.
//   The server calls CreateFileMapping first, clients waits for hMakeDoneEvent and calls it too. So the memory mapping size is determined by the server.
{ char nameEvent[35+OBJ_NAME_LEN];  bool global_name;
 // find the mapping size (the value must be the same for client and server:
  DWORD mapping_extent = CLIENT_SERVER_FILE_MAPPING_SIZE; // the default 
  char key[MAX_PATH];  HKEY hKey;
  strcpy(key, WB_inst_key);  
  strcat(key, Database_str);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)
  { DWORD key_len=sizeof(mapping_extent);
    RegQueryValueEx(hKey, "MappingExtent", NULL, NULL, (BYTE*)&mapping_extent, &key_len);
    RegCloseKey(hKey);
  }
  request_part_size = mapping_extent-1;
  answer_part_size  = mapping_extent-ANSWER_PREFIX_SIZE;
  global_name = try_global;
	make_name(nameEvent, "share%d_wbdata%sq%lx", server_name, id, global_name);
	hDataEvent = CreateEvent(pSA, FALSE, FALSE, nameEvent);
  if (!hDataEvent && try_global)  // used on W95 and similiar, where no global names are available
  { global_name = false;
	  make_name(nameEvent, "share%d_wbdata%sq%lx", server_name, id, global_name);
	  hDataEvent = CreateEvent(pSA, FALSE, FALSE, nameEvent);
  }
  if (hDataEvent)
	{ ResetEvent(hDataEvent);  // the event may have existed if the server was stopped and started immediatelly
	  make_name(nameEvent, "share%d_wbdone%sq%lx", server_name, id, global_name);
	  hDoneEvent = CreateEvent(pSA, FALSE, FALSE, nameEvent);
    if (hDoneEvent)
	  { ResetEvent(hDoneEvent);  // the event may have existed if the server was stopped and started immediatelly
	    make_name(nameEvent, "share%d_wbreq%sq%lx", server_name, id, global_name);
	    hReqMap = CreateFileMapping(INVALID_HANDLE_VALUE, pSA, PAGE_READWRITE, 0, mapping_extent, nameEvent);
      if (hReqMap)
	    { make_name(nameEvent, "share%d_wbans%sq%lx", server_name, id, global_name);
	      hAnsMap = CreateFileMapping(INVALID_HANDLE_VALUE, pSA, PAGE_READWRITE, 0, mapping_extent, nameEvent);
        if (hAnsMap) 
        { rqst = (t_request_struct *)MapViewOfFile(hReqMap, FILE_MAP_WRITE, 0, 0, 0);
          if (rqst)
	        { dir_answer = (answer_cb *)MapViewOfFile(hAnsMap, FILE_MAP_WRITE, 0, 0, 0);
            if (dir_answer)
              return true;
            UnmapViewOfFile(rqst);  
          }
          CloseHandle(hAnsMap);
        }
        CloseHandle(hReqMap);
      }
      CloseHandle(hDoneEvent);
    }
    CloseHandle(hDataEvent);
  }
	return false;
}

void t_dircomm::CloseClientSlaveComm(void)	  
{ UnmapViewOfFile(rqst);    UnmapViewOfFile(dir_answer);
	CloseHandle(hReqMap);	    CloseHandle(hAnsMap);
  CloseHandle(hDataEvent);	CloseHandle(hDoneEvent);	
#ifndef SRVR  // close own handle of the server process
  if (hSrvWait[1])   // 0 before version 9.0.x
    CloseHandle(hSrvWait[1]);	
  // on server, the slave thread closes own handle of the client process by itself
#endif
}

///////////////////// server methods ////////////////////////////////////
void t_dircomm::send_status_nums(trecnum rec1, trecnum rec2)
{ dir_answer->flags=17;          // says: moving intra-request data
  dir_answer->return_data[0]=2;  // says: moving progress numbers
  *(trecnum*)(dir_answer->return_data+1)                =rec1;
  *(trecnum*)(dir_answer->return_data+1+sizeof(trecnum))=rec2;
  SetEvent(hDoneEvent);  /* request completed */
  WaitForRequest2();
}

void t_dircomm::send_notification(int notification_type, const void * notification_parameter, unsigned param_size)
{ dir_answer->flags=17;          // says: moving intra-request data
  dir_answer->return_data[0]=3;  // says: general notification
  dir_answer->return_data[1]=notification_type;
  if (notification_parameter)
    memcpy(dir_answer->return_data+2, notification_parameter, param_size);
  SetEvent(hDoneEvent);  /* request completed */
  WaitForRequest2();
}

bool t_dircomm::write_to_client(const void * buf, unsigned size)
{ dir_answer->flags=17;          // says: moving intra-request data
  dir_answer->return_data[0]=0;  // says: writing data
  *(sig32*)(dir_answer->return_data+1) = size;
  memcpy(dir_answer->return_data+1+sizeof(sig32), buf, size);
  sig32 * data_written = (sig32*)rqst->req.areq;
  sig32 saved = *data_written;
	SetEvent(hDoneEvent);  /* request completed */
  if (!WaitForRequest2()) 
    return true;  // client terminated
  bool error = *data_written!=size;
  *data_written = saved;
  return error;
}

sig32 t_dircomm::read_from_client(char * buf, uns32 size)
{ sig32 result;
  dir_answer->flags=17;          // says: moving intra-request data
  dir_answer->return_data[0]=1;  // says: reading data
  char pending_req[40];
  memcpy(pending_req, &rqst->req, sizeof(pending_req));   /* save original request */
  sig32 * insize  = (sig32*)(dir_answer->return_data+1);
  sig32 * outsize = (sig32*)rqst->req.areq;
  char  * outdata = rqst->req.areq+sizeof(sig32);
  *insize=size;
	SetEvent(hDoneEvent);  /* request completed */
  if (!WaitForRequest2()) 
    return -1;  // error, client terminated
  if (*outsize != -1) memcpy(buf, outdata, *outsize);
  result=*outsize;
  memcpy(&rqst->req, pending_req, sizeof(pending_req));   /* restore original request */
  return result;
}

//////////////////// client methods //////////////////////////////////

#ifndef SRVR
CFNC DllKernel BOOL WINAPI DetectLocalServer(const char * loc_server_name)
{ char nameEvent[35+OBJ_NAME_LEN];
	make_name(nameEvent, "share%d_wbmakerevent%s", loc_server_name, 0, true);
  HANDLE hMakerEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, nameEvent);
  if (!hMakerEvent) 
	{ make_name(nameEvent, "share%d_wbmakerevent%s", loc_server_name, 0, false);
    hMakerEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, nameEvent);
    if (!hMakerEvent) 
    { make_name(nameEvent, "share%d_noipc%s", loc_server_name, 0, true);
      hMakerEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, nameEvent);
      if (!hMakerEvent) 
	    { make_name(nameEvent, "share%d_noipc%s", loc_server_name, 0, false);
        hMakerEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, nameEvent);
        if (!hMakerEvent) 
          return FALSE;
      }
      CloseHandle(hMakerEvent);
      return 2;  // server runs, but without the IPC interface
    }
  }
  CloseHandle(hMakerEvent);
  return TRUE;
}

bool is_server_in_global_namespace(const char * server_name)
{ char nameEvent[35+OBJ_NAME_LEN];
	make_name(nameEvent, "share%d_wbmakerevent%s", server_name, 0, true);
  HANDLE hMakerEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, nameEvent);
  if (!hMakerEvent) return false;
  CloseHandle(hMakerEvent);
  return true;
}

#endif

