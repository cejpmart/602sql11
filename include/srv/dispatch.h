/****************************************************************************/
/* Dispatch.h - the request dispatcher header file                          */
/****************************************************************************/
#ifndef DISPATCH_DEFINED
#define DISPATCH_DEFINED

extern int max_task_index;
extern volatile wbbool kernel_is_init;  // server is running
extern tobjname server_name;
extern bool is_service;
extern bool fulltext_allowed, network_server_licence;
extern char installation_key[MAX_LICENCE_LENGTH+1];  
extern char ServerLicenceNumber[MAX_LICENCE_LENGTH+1];
extern BOOL back_end_operation_on;
extern BOOL comm_log_replay; // replay the communications log and stop
extern bool running_as_service, minimal_start;
extern int backup_status;
enum t_server_init { SERVER_INIT_NO, SERVER_INIT_OBJECTS, SERVER_INIT_TRACE_LOG,
  SERVER_INIT_SECURDESCR, SERVER_INIT_DOWN_EVENT, SERVER_INIT_TASK, 
  SERVER_INIT_CDP, SERVER_INIT_FFA, SERVER_INIT_FRAME, SERVER_INIT_PIECES, SERVER_INIT_GLOBAL,
  SERVER_INIT_IP_PROTOCOL, SERVER_INIT_REQ_SYNCHRO, SERVER_INIT_LICENCES, SERVER_INIT_STARTPROC, SERVER_INIT_IDENT_VERIF,
  SERVER_INIT_NETWORK, SERVER_INIT_ADVERTISING, SERVER_INIT_DIRMAKER,
  SERVER_INIT_REPLIC,
  SERVER_INIT_COMPLETE };
extern t_server_init init_counter;  // server initialisation progress
extern bool network_server, is_dependent_server;
extern unsigned client_logout_timeout;
extern cdp_t my_cdp;
#ifdef ANY_SERVER_GUI
extern UINT uBreakMessage, uRestartMessage;
extern HWND hwMain;
#endif
extern int http_tunnel_port_num, http_web_port_num;
bool get_own_host_name(char * name, int buffer_size);
int get_external_client_connections(void);
void read_addon_licences(void);
void dump_thread_overview(cdp_t cdp);
void dump_frames(cdp_t cdp);
void CALLBACK direct_to_log(const char * msg);

struct request_frame;

#define SYS_JOUR    1
#define SYS_COMMIT  2
#define SYS_BISTOP  4

#ifdef FIX_CHECKS
#define MAX_APPL_FIXES 18
typedef struct
{ tfcbnum fcbn;
  uns8 fixes;
} t_fix_info;
#endif

extern CRITICAL_SECTION commit_sem, frames_sem, crs_sem, privil_sem, cs_sequences, 
                        cs_client_list, cs_short_term, cs_medium_term, cs_trace_logs, cs_profile;
extern LocEvent hLockOpenEvent;
extern LocEvent Zipper_stopped;
#ifdef WINS
// CS which guards msgsupp.dll library loading (e-mail parser/converter utility for fulltext)
// msgsupp.dll is for Win32 only
extern CRITICAL_SECTION cs_ftx_msgsupp_load;
#endif

#ifdef WINS
#define dispatch_kernel_control(cdp)
#else  /* !WINS */
#ifdef UNIX
#define dispatch_kernel_control(cdp)
#else // NLM
#define dispatch_kernel_control(cdp) ThreadSwitchWithDelay()
#endif
#endif /* !WINS */

#ifdef WINS
CFNC void send_break_req(DWORD who);
#endif
bool break_the_user(cdp_t cdp, int linknum);
bool kill_the_user(cdp_t cdp, int linknum);
void var_conv_pass(char *p, uns8 size, uns16 key);

#ifndef WINS
extern WORD dispatchPenalty;

int WINAPI GetPrivateProfileString(LPCSTR lpszSection,  LPCSTR lpszEntry,  LPCSTR lpszDefault,
            LPSTR lpszReturnBuffer, int cbReturnBuffer, LPCSTR lpszFilename);
UINT WINAPI GetPrivateProfileInt(LPCSTR lpszSection,  LPCSTR lpszEntry,  int defaultval, LPCSTR lpszFilename);
#endif

#ifdef WINS
#define GetClientId GetCurrentThreadId   /* WINS */
#else
#ifdef UNIX
#define GetClientId pthread_self         /* UNIX */
#else
#define GetClientId GetThreadID          /* NLM */
#endif
#endif

void client_count_string(char * str);  // returns standartised info about connected users as string
#if WBVERS<900
void PrintAttachedUsers(void);
#endif
int WINAPI kernel_init(cdp_t cdp, const char * path, bool networking);
void WINAPI dir_kernel_close(void);
enum t_down_reason { down_system, down_console, down_endsession, down_from_client, down_pwr, down_abort };
extern t_down_reason down_reason;
int  restore_database_file(const char * path);
bool dir_lock_kernel(cdp_t cdp, bool check_users);
void dir_unlock_kernel(cdp_t cdp);

// periodical actions:
void start_thread_for_periodical_actions(void);
extern ttablenum planner_tablenum;
void start_planned_actions(cdp_t cdp);
bool thread_is_running(const char * name);
void create_timer_table(cdp_t cdp);


#ifdef WINS
extern char WB_inst_key[];
extern char Database_str[];
extern char Installation_str[];
extern char Path_str[];
extern char ClientMem_str[];
extern char ServerMem_str[];
extern char SortSpace_str[];
extern char TotalSort_str[];
extern char FrameSpace_str[];
extern char ClientProt_str[];
extern char IP_str[];
extern char Broadcast_str[];
extern char Version_str[];

BOOL WINAPI ServerByPath(const char * path, char * server_name);
#endif
extern bool my_private_server;
//bool GetDatabaseString(const char * server_name, const char * value_name, void * dest, int bufsize, bool private_server);
//bool SetDatabaseString(const char * server_name, const char * value_name, const char * val);
//bool SetDatabaseInt(const char * server_name, const char * value_name, DWORD val);

// server shutdown signals:
extern LocEvent hKernelShutdownReq;
#ifdef WINS
extern BOOL fShutDown, fLocking;
extern HANDLE hKernelShutdownEvent, hKernelLockEvent;
extern THREAD_HANDLE hThreadMakerThread;
#endif
#ifdef UNIX

/* Class that is equivalent to BOOL, except that it broadcasts its value when
 * changed. 
 * When you do something that could wait Sleep_cond-ing function, be sure to
 * call update, and better close it in critical section.
 */
class condition_var {
	pthread_cond_t cond;
	pthread_mutex_t mutex;
public:
	condition_var();
	int broadcast() {return pthread_cond_broadcast(&cond);}
	int lock() {return pthread_mutex_lock(&mutex);}
	int unlock() {return pthread_mutex_unlock(&mutex);}
	int wait(timespec &ts){return pthread_cond_timedwait(&cond, &mutex,
							    &ts);}
	int wait(){return pthread_cond_wait(&cond, &mutex);}
};

extern condition_var hKernelShutdownEvent, hKernelLockEvent;
	
class bool_condition{
	bool val;
  condition_var * hEvent;
public:
	bool_condition(condition_var * hEventIn): val(false), hEvent(hEventIn) { }
	BOOL wait_for(timespec *ts); /* return TRUE if was timeouted, FALSE if val got true */
	BOOL operator=(bool newval){
		if (val==newval) return val;
		hEvent->lock();
		val=newval;
		hEvent->broadcast();
		hEvent->unlock();
		return val;
	}
	operator bool () const { return val; }
};

extern bool_condition fShutDown, fLocking;
void UnloadProc(int sig);
#endif
#ifdef NETWARE        
extern BOOL fShutDown;
extern LONG           hKernelShutdownEvent; 
#endif

void stop_the_server(void);
void stop_working_threads(cdp_t cdp);
void restart_working_threads(cdp_t cdp);

class t_temp_account
{ cdp_t cdp;
  tobjnum orig_usernum;
 public:
  t_temp_account(cdp_t cdpIn, tobjnum new_usernum, tcateg category);
  ~t_temp_account(void);
};

///////////////// planning and synchonising the integrity checks of the database file ///////////////
/* Integrity check can be started by a client (strong==true) or by a periodical actions planner (strong==false).
Resuming repliaction                                                            waits form completing the check.
Resuming the thread waiting on semaphore or in Sleep cancels the weak check and waits form completing the check.
New client request                                   cancels the weak check and waits form completing the check.
  (cancelling the weak check may be disabled by a property)
  
*/
#define INTEGR_CHECK_CONSISTENCY_LOST_BLOCK    1
#define INTEGR_CHECK_CONSISTENCY_LOST_PIECE    2
#define INTEGR_CHECK_CONSISTENCY_NONEX_BLOCK   4
#define INTEGR_CHECK_CONSISTENCY_CROSS_LINK    8
#define INTEGR_CHECK_CONSISTENCY_DAMAGED_TABLE 0x10
#define INTEGR_CHECK_CONSISTENCY (INTEGR_CHECK_CONSISTENCY_LOST_BLOCK | INTEGR_CHECK_CONSISTENCY_LOST_PIECE | INTEGR_CHECK_CONSISTENCY_NONEX_BLOCK | INTEGR_CHECK_CONSISTENCY_CROSS_LINK | INTEGR_CHECK_CONSISTENCY_DAMAGED_TABLE)
#define INTEGR_CHECK_INDEXES                   0x20
#define INTEGR_CHECK_CACHE                     0x40

class t_integrity_check_planning
{public: 
  uns32 last_check_or_server_startup;
  long  running_threads_counter;  // Interlocked
 private:
  LocEvent hCheckNotRunning;  // manual event, threads wait for the termination of the test
  Semaph hCheckSemaphore;
  long  cancel_request_counter;   // Interlocked, >0 when a thread requests cancelling the integrity check, ignored by a strong check
  BOOL is_strong_check;  // write protected by hCheckSemaphore
 public:
  bool last_check_cancelled;  // valid after try_starting_check returning TRUE
  t_integrity_check_planning(void)
   { SetLocEventInvalid(hCheckNotRunning);  SetSemaphoreInvalid(hCheckSemaphore); }
  void open(void);
  void close(void);

  inline BOOL starting_client_request(void)
  { InterlockedIncrement(&running_threads_counter);  // prevents new checks to be started
    if (!the_sp.integr_check_priority.val()) InterlockedIncrement(&cancel_request_counter);
    DWORD res=WaitLocalManualEvent(&hCheckNotRunning, /*is_strong_check ? 5 :*/ INFINITE);
    // strong integrity check blocks all client requests because clients may not react properly to the REJECTED_BY_KERNEL error
    // weak   integrity check blocks all client requests until it is effectively cancelled
    if (!the_sp.integr_check_priority.val()) InterlockedDecrement(&cancel_request_counter);
    if (res==WAIT_OBJECT_0)
      return TRUE;
    InterlockedDecrement(&running_threads_counter);
    return FALSE;
  }
  inline void continuing_detached_operation(void)
  { InterlockedIncrement(&running_threads_counter);  // prevents new checks to be started
    if (!the_sp.integr_check_priority.val()) InterlockedIncrement(&cancel_request_counter);
    WaitLocalManualEvent(&hCheckNotRunning, INFINITE);
    if (!the_sp.integr_check_priority.val()) InterlockedDecrement(&cancel_request_counter);
  }
  inline void continuing_replication_operation(void)
  { InterlockedIncrement(&running_threads_counter);  // prevents new checks to be started
    WaitLocalManualEvent(&hCheckNotRunning, INFINITE);
  }
  inline void thread_operation_stopped(void)
    { InterlockedDecrement(&running_threads_counter); }
  inline void system_worker_thread_start(void)
    { InterlockedIncrement(&running_threads_counter); }

  BOOL try_starting_check(bool strong, bool from_client);
  bool uncheckable_state(void);
  void check_completed(void);
  BOOL timer_event(void);
  inline bool cancel_request_pending(void)
  { if (!is_strong_check && cancel_request_counter || fShutDown) // must not check [fLocking] here, integrity is often checked when fLocking is on.
    { last_check_cancelled=true;
      return true;
    }
    return false;
  }
};

extern t_integrity_check_planning integrity_check_planning;

///////////////////////////// sharing countable licences among servers ///////////////////////////////
class t_shared_info
{ HANDLE hFileMapping;
 protected:
  void * shared_info;
 public:
  t_shared_info(void)
    { hFileMapping=0;  shared_info=NULL; }
  BOOL open(SECURITY_ATTRIBUTES * pSA);
  void close(void);
  inline void * ptr(void) const
    { return shared_info; }
};

#if WBVERS<900
class t_shared_info_lic : public t_shared_info
{
#ifdef LINUX  // resolution of problems on RH 7.1
  char replac[300];
#endif
 public:
  void add_lics(const char * ik, unsigned initial_licences);
  bool get_licence(const char * ik);
  void return_licence(const char * ik);
  void get_info(const char * ik, unsigned * total, unsigned * used);
  void update_total_lics(const char * ik, unsigned licences);
#ifdef LINUX  // resolution of problems on RH 7.1
  inline char * ptr(void) const
    { return shared_info ? (char*)shared_info : (char*)replac; }
  t_shared_info_lic(void)
    { *replac=0; }  
#endif
};
#else
class t_shared_info_lic
{ char data[MAX_LICENCE_LENGTH+2*sizeof(unsigned)];
 public:
  void add_lics(const char * ik, unsigned initial_licences);
  bool get_licence(const char * ik);
  void return_licence(const char * ik);
  void get_info(const char * ik, unsigned * total, unsigned * used);
  void update_total_lics(const char * ik, unsigned licences);
  inline char * ptr(void) const
    { return (char*)data; }
  BOOL open(SECURITY_ATTRIBUTES * pSA)
    { return TRUE; }
  void close(void)
    { }
};
#endif

extern t_shared_info_lic server_shared_info;

struct t_client_licences
{ int licences_consumed, 
      network_clients, direct_clients,  // client is counted iff !conditional_attachment (has own license or shares one)
      www_clients;                      // counted in network_clients or direct_clients too
  BOOL get_access_licence(cdp_t cdp);
  void return_access_licence(cdp_t cdp);
  t_client_licences(void)
    { licences_consumed=network_clients=direct_clients=www_clients=0; }
  inline void get_info(unsigned * total, unsigned * used)
    { server_shared_info.get_info(installation_key, total, used); }
  inline void update_total_lics(unsigned licences)
    { server_shared_info.update_total_lics(installation_key, licences); }
  void close(void);
};

extern t_client_licences client_licences;

BOOL get_server_info(cdp_t cdp, int info_type, uns32 * result);
void periodical_actions(cdp_t cdp);

///////////////////////
#ifdef LINUX
void ts2abs(uns32 miliseconds, timespec &ts); /* Convert time in milisecs to absolute */ 
BOOL Sleep_cond(uns32 miliseconds, const bool_condition &cond=fLocking); // wait until the time elapses or until the server shutdown
#else
BOOL Sleep_cond(uns32 miliseconds); // wait until the time elapses or until the server shutdown/locking
#endif
BOOL find_system_procedure(cdp_t cdp, const char * name);
extern BOOL disable_system_triggers;
BOOL SetFileSize(FHANDLE hnd, tblocknum block_cnt); // returns TRUE iff OK

void wbt_construct(void);
HANDLE wbt_create_semaphore(const char * name, int initial_value);
void   wbt_close_semaphore(HANDLE hSemaphore);
void   wbt_close_all_semaphores(void);
void   wbt_release_semaphore(HANDLE hSemaphore); 
int    wbt_wait_semaphore(HANDLE hSemaphore, int timeout);

void check_for_dead_clients(cdp_t cdp);

class CTrcTblVal
{
protected:

    CTrcTblVal *m_Next;
    char        m_Val[1];

public:

    void SetVal(int Type, LPCVOID Val, int Size, int MaxSize);
    void Tiny2a(sig8 Val){int8tostr(Val, m_Val);}
    void Short2a(sig16 Val){int16tostr(Val, m_Val);}
    void Int2a(int Val){int2str(Val, m_Val);}
    void Uns2a(unsigned Val){sprintf(m_Val, "%u", Val);}
    void Bigint2a(sig64 Val){int64tostr(Val, m_Val);}
    void Mon2a(monstr *Val){money2str(Val, m_Val, SQL_LITERAL_PREZ);}
    void Real2a(double *Val){real2str(*Val, m_Val, *Val > 1000000.0 || *Val < 0.00001 ? 10 : -10);}
    void Date2a(DWORD Val){date2str(Val, m_Val, 1);}
    void Time2a(DWORD Val){time2str(Val, m_Val, 1);}
    void Tstmp2a(DWORD Val){datim2str(Val, m_Val, 5);}
    void Bool2a(wbbool Val)
    {
        char *Txt;
        if (Val == 0)
            Txt = "FALSE";
        else if ((unsigned char)Val == NONEBOOLEAN)
            Txt = "NONE";
        else
            Txt = "TRUE";
        strcpy(m_Val, Txt);
    }
    void Char2a(char Val)
    {
        if (Val < 32)
        {
            m_Val[0] = '#';
            itoa(Val, m_Val + 1, 10);
        }
        else
        {
            m_Val[0] = Val;
            m_Val[1] = 0;
        }
    }
    void Str2a(char *Val, int Size, int MaxSize)
    {
        if (!Size)
            *m_Val = 0;
        else
        {
            char *v, *b;
            for (v = Val, b = m_Val; v < Val + MaxSize - 4 && *v; v++, b++)
            {
                if (*v < 32)
                    *b = '.';
                else
                    *b = *v;
            }
            if (*v)
                strcpy(b, "...");
            else
                *b = 0;
        }
    }
    void Bin2a(char *Val, int Size, int MaxSize)
    {
        int sz = (MaxSize - 4) / 2;
        if (sz > Size)
            sz = Size;
        bin2hex(m_Val, (const uns8 *)Val, sz);
        if (sz < Size)
            strcpy(m_Val + 2 * sz, "...");
        else
            m_Val[2 * sz] = 0;
    }
    CTrcTblVal *GetNext(){return(m_Next);}
    char *GetVal(){return(m_Val);}

    friend class CTrcTbl;
    friend class CTrcTblRow;
};

class CTrcTblRow
{
protected:

    int          m_MaxValSz;
    CTrcTblVal  *m_Hdr;
    CTrcTblVal **m_Tail;

public:

    CTrcTblVal *AddVal(int Type, LPCVOID Val, int Size = 0);
    CTrcTblVal *InsertVal(CTrcTblVal *After, int Type, LPCVOID Val, int Size = 0);
    char *GetVal(int Pos)
    {
        CTrcTblVal *Val;
        int i;
        for (Val = m_Hdr, i = 0; i < Pos; Val = Val->m_Next, i++);
        return(Val->m_Val);
    }
    CTrcTblVal  *GetHdr(){return(m_Hdr);}

    friend class CTrcTbl;
};

class CTrcTbl
{
protected:

    int        m_RowCnt;
    int        m_ColCnt;
    int       *m_FPSize;
#ifdef ANY_SERVER_GUI
    int       *m_VPSize;
#endif
    CTrcTblRow m_Row[4];

    int BuildFPRow(char *MsgHdr, int Rw, int Col0);
#ifdef ANY_SERVER_GUI
    void BuildVPRow(char *MsgHdr, int Rw, int Col0, int ColMax);
#endif

public:

    CTrcTbl(int RowCnt, int MaxValSz)
    {
        m_RowCnt = RowCnt;
        m_ColCnt = 0;
        m_FPSize = NULL;
#ifdef ANY_SERVER_GUI
        m_VPSize = NULL;
#endif
        for (CTrcTblRow *Row = m_Row; Row < m_Row + RowCnt; Row++)
        {
            Row->m_MaxValSz = MaxValSz;
            Row->m_Hdr      = NULL;
            Row->m_Tail     = &Row->m_Hdr;
        }
    }
   ~CTrcTbl()
    {
        for (CTrcTblRow *Row = m_Row; Row < m_Row + m_RowCnt; Row++)
        {
            CTrcTblVal *Tmp;
            for (CTrcTblVal *Val = Row->m_Hdr; Val; Val = Tmp)
            {
                Tmp = Val->m_Next;
                corefree(Val);
            }
        }
        if (m_FPSize)
            corefree(m_FPSize);
#ifdef ANY_SERVER_GUI
        if (m_VPSize)
            corefree(m_VPSize);
#endif
    }
    CTrcTblRow *GetRow(int i){return(m_Row + i);}
    int GetRowCnt(){return(m_RowCnt);}
    int GetColCnt()
    {
        if (!m_ColCnt)
            for (CTrcTblVal *Item = m_Row->m_Hdr; Item; Item = Item->m_Next, m_ColCnt++);
        return(m_ColCnt);
    }
    void Trace(cdp_t cdp, unsigned trace_type);
};

#endif  /* DISPATCH_DEFINED */
