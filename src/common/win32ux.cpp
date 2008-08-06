// Emulation of Win32 API on UNIX API functions

#ifndef WINS
#include <fcntl.h>

#ifdef UNIX
/////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <ctype.h>  // toupper
#include <errno.h>  // ETIMEDOUT
#include <assert.h>

const pthread_mutex_t initial_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#ifndef SINGLE_THREAD_CLIENT
#include <pthread.h>

void SetCursor(int i) { }
/////////////////// Kriticke sekce //////////////////////////////

BOOL CheckCriticalSection(CRITICAL_SECTION * cs)
{
  if (pthread_mutex_trylock(cs)) return FALSE;
  pthread_mutex_unlock(cs);
  return TRUE;
}
#endif

//////////////////// Events ////////////////////////

/* Windows-like semafory jsou definovany pomoci promenne sem_count ve
 * strukture Semaph, chranene mechanismem pro condition variables. Duvodem je
 * (?) moznost cekat s casovym omezenim. Alternativnim nazvem pro Semaph je
 * LocEvent. */

/* Translate miliseconds to absolute time usable for timedwait */
void ms2abs(uns32 miliseconds, timespec&ts)
{
  timeval now;
  gettimeofday(&now,NULL);
  ts.tv_nsec=1000*(now.tv_usec+(miliseconds % 1000)*1000);
  ts.tv_sec=now.tv_sec+miliseconds / 1000;
  ts.tv_sec+=ts.tv_nsec / 1000000000;
  ts.tv_nsec%=1000000000;
}


#ifndef SINGLE_THREAD_CLIENT

CFNC BOOL WINAPI CreateSemaph(sig32 InitialCount, sig32 MaximumCount, Semaph * semaph)
{
  pthread_mutex_init(&semaph->mutex, NULL); /* these never fail */
  pthread_cond_init(&semaph->cond, NULL);
  semaph->sem_count=InitialCount;
  return TRUE;
}

CFNC BOOL  WINAPI CreateLocalManualEvent(BOOL bInitialState, LocEvent * event)
{ return CreateSemaph(bInitialState ? 1 : 0, 1, event); }

CFNC BOOL  WINAPI CloseLocalManualEvent(LocEvent * event)
{ return CloseSemaph(event); }

CFNC BOOL  WINAPI SetLocalManualEvent(LocEvent * event)
{ return ReleaseSemaph(event, 1); }

CFNC BOOL  WINAPI ResetLocalManualEvent(LocEvent * event)
{ return WaitSemaph(event, 0)!=WAIT_ABANDONED; }

CFNC DWORD WINAPI WaitLocalManualEvent(LocEvent * event, DWORD timeout)
{ DWORD res=WaitSemaph(event, timeout);
  if (res==WAIT_OBJECT_0) ReleaseSemaph(event, 1);  // make signalled again
  return res;
}

CFNC DWORD WINAPI WaitPulsedManualEvent(LocEvent * event, DWORD timeout)
{ int res;
  pthread_mutex_lock(&event->mutex);
  if (timeout==INFINITE) res=pthread_cond_wait(&event->cond, &event->mutex);
  else 
  { struct timespec abstime;
    ms2abs(timeout, abstime);
    res=pthread_cond_timedwait(&event->cond, &event->mutex, &abstime);
  }
  pthread_mutex_unlock(&event->mutex);  
  return res==0;
}

CFNC BOOL WINAPI CloseSemaph(Semaph * semaph)
{
    int res=pthread_mutex_destroy(&semaph->mutex);
    assert(res==0);
    res=pthread_cond_destroy(&semaph->cond);
    assert(res==0);
    return TRUE;
}

CFNC BOOL WINAPI ReleaseSemaph(Semaph * semaph, sig32 count)
{
    int res=pthread_mutex_lock(&semaph->mutex);
    assert(res==0);
    semaph->pulsed=0;
    semaph->sem_count+=count;
    pthread_cond_broadcast(&semaph->cond); 
    pthread_mutex_unlock(&semaph->mutex);
    return TRUE;
}

BOOL WINAPI PulseEvent(LocEvent & semaph)
{
    int res=pthread_mutex_lock(&semaph.mutex);
    assert(res==0);
    semaph.pulsed=1;
    pthread_cond_broadcast(&semaph.cond);
    pthread_mutex_unlock(&semaph.mutex);
    return TRUE;
}


CFNC DWORD WINAPI WaitSemaph(Semaph * semaph, DWORD timeout)
{ 
    int res=pthread_mutex_lock(&semaph->mutex);
    assert(res==0);
  do {
    if (semaph->sem_count>0) { /* Na semaforu je zelena */
	    semaph->sem_count--;
	    pthread_mutex_unlock(&semaph->mutex);
	    return WAIT_OBJECT_0;
    }
    if (timeout == INFINITE) res=pthread_cond_wait(&semaph->cond, &semaph->mutex);
    else {
	    struct timespec abstime;
	    ms2abs(timeout, abstime);
	    res=pthread_cond_timedwait(&semaph->cond, &semaph->mutex, &abstime);
    }
    if (semaph->pulsed) {
    pthread_mutex_unlock(&semaph->mutex);  
      return WAIT_OBJECT_0;
    }
  } while (res==0||res==EINTR);
  pthread_mutex_unlock(&semaph->mutex);  
  return WAIT_TIMEOUT;
}
#endif // ! SINGLE_THREAD_CLIENT

#ifdef SRVR
CFNC LONG WINAPI InterlockedIncrement(LONG * count)
{ ProtectedByCriticalSection cs(&cs_short_term);   // multiprocessor machines need it
  LONG res = ++(*count);
  return res;
}

CFNC LONG WINAPI InterlockedDecrement(LONG * count)
{ ProtectedByCriticalSection cs(&cs_short_term);   // multiprocessor machines need it
  LONG res = --(*count);
  return res;
}
#endif

#else  //////////////////////////// NLM ///////////////////////////////////////////
#include <process.h>
#include <nwsemaph.h>
#include <io.h>
#include <nwfile.h>
#include <nwmisc.h>
typedef DIR dirent;
//////////////////////////// Critical sections ///////////////////////////////
void InitializeCriticalSection(CRITICAL_SECTION * cs)
{
#ifndef SINGLE_THREAD_CLIENT
  cs->thread_id=cs->count=0;
  cs->hSemaph=OpenLocalSemaphore(1);
#endif
}

void DeleteCriticalSection(CRITICAL_SECTION * cs)
{
#ifndef SINGLE_THREAD_CLIENT
  if (cs->hSemaph) 
    { CloseLocalSemaphore(cs->hSemaph);  cs->hSemaph=0; } 
#endif
}

void EnterCriticalSection(CRITICAL_SECTION * cs)
{
#ifndef SINGLE_THREAD_CLIENT
  int ti=GetThreadID();
  if (ti==cs->thread_id) cs->count++;
  else
  { WaitOnLocalSemaphore(cs->hSemaph);  // cs->count is 0 now
    cs->thread_id=ti;
  }
#endif
}

void LeaveCriticalSection(CRITICAL_SECTION * cs)
{
#ifndef SINGLE_THREAD_CLIENT
  if (cs->count) cs->count--;
  else
  { cs->thread_id=0;
    SignalLocalSemaphore(cs->hSemaph);
  }
#endif
}

BOOL CheckCriticalSection(CRITICAL_SECTION * cs)
{
#ifndef SINGLE_THREAD_CLIENT
return cs->thread_id==0;
#else
return FALSE
#endif
}

////////////////////////////// Events //////////////////////////////////////////////
CFNC BOOL  WINAPI CreateLocalAutoEvent(BOOL bInitialState, LocEvent * event)
{ *event=OpenLocalSemaphore(bInitialState ? 1 : 0); 
  return *event!=0;
}

CFNC BOOL  WINAPI CloseLocalAutoEvent(LocEvent * event)
{ if (!*event) return FALSE;
  CloseLocalSemaphore(*event); 
  *event=0;
  return TRUE;
}

CFNC BOOL  WINAPI SetLocalAutoEvent(LocEvent * event)
{ if (!*event) return FALSE;
  SignalLocalSemaphore(*event);
  return TRUE;
}

CFNC DWORD WINAPI WaitLocalAutoEvent(LocEvent * event, DWORD timeout)
{ if (!*event) return FALSE;
  if (timeout == INFINITE) 
    WaitOnLocalSemaphore(*event);
  else
    if (TimedWaitOnLocalSemaphore(*event, timeout)) return WAIT_TIMEOUT;
  return WAIT_OBJECT_0;
}

CFNC BOOL WINAPI CreateSemaph(sig32 lInitialCount, long lMaximumCount, Semaph * semaph)
{ *semaph=OpenLocalSemaphore(lInitialCount); 
  return *semaph!=0;
}

CFNC BOOL WINAPI CloseSemaph(Semaph * semaph)
{ if (!*semaph) return FALSE;
  CloseLocalSemaphore(*semaph); 
  *semaph=0;
  return TRUE;
}

CFNC BOOL WINAPI ReleaseSemaph(Semaph * semaph, sig32 count)
{ if (!*semaph) return FALSE;
  while (count--) SignalLocalSemaphore(*semaph); // no error repored, abend on bad semaphore
  return TRUE;
}

CFNC DWORD WINAPI WaitSemaph(Semaph * semaph, DWORD timeout)
{ if (!*semaph) return WAIT_ABANDONED;
  if (timeout == INFINITE) 
    WaitOnLocalSemaphore(*semaph);
  else
    if (TimedWaitOnLocalSemaphore(*semaph, timeout)) return WAIT_TIMEOUT;
  return WAIT_OBJECT_0;
}

#endif //////////////// end of NLM synchro /////////////////////////

//////////////// file I/O /////////////////
CFNC FHANDLE WINAPI CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, 
  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
  int hnd;
  int access_mode;
  if (dwDesiredAccess & GENERIC_WRITE) 
    if (dwDesiredAccess & GENERIC_READ) access_mode = O_BINARY | O_RDWR;
    else                                access_mode = O_BINARY | O_WRONLY;
  else                                  access_mode = O_BINARY | O_RDONLY;
  switch (dwCreationDisposition)
  { case CREATE_ALWAYS: 
      hnd=creat(lpFileName, 0666);  break; 
    case CREATE_NEW:  // must fail if the file exists  
      hnd=open(lpFileName, access_mode|O_CREAT|O_EXCL, 0666);  break; 
    case OPEN_EXISTING:                     // pouze pro unix !!!!
      hnd=open(lpFileName, access_mode);  break;
    case OPEN_ALWAYS:
      hnd=open(lpFileName, access_mode|O_CREAT, 0666);  break;  // less problems (log file etc)
    default:
      hnd=-1;  break;
  }
  if (hnd==-1){
	 //perror(lpFileName);
	 return INVALID_FHANDLE_VALUE;
  }
  return hnd;
}

CFNC BOOL WINAPI CloseHandle(FHANDLE hObject)
{ //if (!hObject && error_call_back) { (*error_call_back)("NULL file handle Close!", MON_GLOBAL);  return FALSE; }
  if (hObject==INVALID_FHANDLE_VALUE) return FALSE;
  return close(hObject) != -1; 
}

CFNC BOOL WINAPI ReadFile(FHANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                          LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{ //if (!hFile && error_call_back) { (*error_call_back)("NULL file handle Read!", MON_GLOBAL);  return FALSE; }
  if (hFile==INVALID_FHANDLE_VALUE) return FALSE;
  LONG rd=read(hFile, (char*)lpBuffer, nNumberOfBytesToRead);
  if (rd==-1) { *lpNumberOfBytesRead=0;  return FALSE; }
  *lpNumberOfBytesRead=rd;
  return TRUE;
}

CFNC BOOL WINAPI WriteFile(FHANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
             LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{ //if (!hFile && error_call_back) { (*error_call_back)("NULL file handle Write!", MON_GLOBAL);  return FALSE; }
  if (hFile==INVALID_FHANDLE_VALUE) return FALSE;
  if (!nNumberOfBytesToWrite) return TRUE;  // null operation in Win32, does not truncate!
  LONG wr=write(hFile, (char*)lpBuffer, nNumberOfBytesToWrite);
  if (wr==-1) { *lpNumberOfBytesWritten=0;  return FALSE; }
  *lpNumberOfBytesWritten=wr;
  if ((DWORD)wr!=nNumberOfBytesToWrite) return FALSE;
  return TRUE;
}

long SetFilePointer(FHANDLE hFile, DWORD pos, DWORD * hi, int start)
{ return lseek(hFile, pos, start); }

long SetFilePointer64(FHANDLE hFile, uint64_t pos, int start)
{ return lseek(hFile, pos, start); }

uint64_t GetFileSize(FHANDLE hFile, DWORD * hi)
// [hi] ignored here, full size returned
{ 
  uint64_t orig = lseek(hFile, 0, FILE_CURRENT);
  uint64_t len  = lseek(hFile, 0, FILE_END);
  lseek(hFile, orig, FILE_BEGIN);
  return len;
}

#ifdef UNIX
static BOOL copy_file2file(int hFrom, int hTo, BOOL * cancel) // TRUE on error
{ 
#define COPY_BUFSIZE (1024*1024)
  BOOL res = FALSE;
  char * buf = new char[COPY_BUFSIZE];
  if (!buf) return TRUE;
  do
  { LONG rd=read(hFrom, buf, COPY_BUFSIZE);
    if (rd==-1) { res=TRUE;  break; }
    if (!rd) break;
    if (write(hTo, buf, rd) != rd) { res=TRUE;  break; }
  } while (!cancel || !*cancel);
  delete [] buf;
  return res;
}
#endif

CFNC BOOL WINAPI MoveFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName)
// Options MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING and used by default.
{ 
  if (-1!=rename(lpExistingFileName, lpNewFileName))
    return TRUE;
  if (errno!=EXDEV) return FALSE;
 // moving to another filesystem replaced by copying and deleting: 
  int hFrom, hTo;  BOOL ok=TRUE;  LONG wr;
  hFrom=open(lpExistingFileName, O_RDONLY|O_BINARY);
  if (hFrom==-1) ok=FALSE;
  else
  { hTo=creat(lpNewFileName, 0600);
    if (hTo==-1) ok=FALSE;
    else
    { if (copy_file2file(hFrom, hTo, NULL)) ok=FALSE;
      else unlink(lpExistingFileName);
      close(hTo);
    }
    close(hFrom);
  }
  return ok;
}

#if WBVERS>=900
BOOL CopyFileEx(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName,
  PROGRESS_ROUTINE * lpProgressRoutine, LPVOID lpData, BOOL * pbCancel, DWORD dwCopyFlags)
// Cancelling implemented, progress not implemented.  
{ int hFrom, hTo;  BOOL ok=TRUE;  
  if (dwCopyFlags & COPY_FILE_FAIL_IF_EXISTS)
  { hTo=open(lpNewFileName, O_RDONLY|O_BINARY);
    if (hTo!=-1) 
      { close(hTo);  return FALSE; } // exists -> fail
  }
  hFrom=open(lpExistingFileName, O_RDONLY|O_BINARY);
  if (hFrom==-1) ok=FALSE;
  else
  { hTo=creat(lpNewFileName, 0600);
    if (hTo==-1) ok=FALSE;
    else
    { if (copy_file2file(hFrom, hTo, pbCancel)) ok=FALSE;
      close(hTo);
      if (pbCancel && *pbCancel)
        { unlink(lpNewFileName);  ok=FALSE; }
    }
    close(hFrom);
  }
  return ok;
}
#endif

CFNC BOOL WINAPI CopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{ return CopyFileEx(lpExistingFileName, lpNewFileName, NULL, NULL, NULL, bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0); }

CFNC BOOL WINAPI DeleteFile(LPCTSTR lpFileName)
{ return unlink(lpFileName)==0; }

CFNC BOOL WINAPI CreateDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
#ifdef UNIX
{ return mkdir(lpPathName, S_IRWXU | S_IRWXG | S_IRWXO)==0; }
#else
{ return mkdir(lpPathName)==0; }
#endif

DWORD GetTempPath(DWORD BuffLength, LPSTR Buffer) {
const char TempAddr[]="/tmp";
	strncpy(Buffer, TempAddr, BuffLength);
	if (sizeof(TempAddr)>BuffLength) return sizeof(TempAddr);
	return sizeof(TempAddr)-1;
}

CFNC UINT WINAPI GetTempFileName(LPCTSTR lpPathName, LPCTSTR lpPrefixString, UINT uUnique, LPTSTR lpTempFileName)
// Implemented for uUnique==0 only!
{ int len, pos, cycle=0, hnd;
  strcpy(lpTempFileName, lpPathName);
  pos=strlen(lpTempFileName);  
  if (pos && lpTempFileName[pos-1]!=PATH_SEPARATOR) lpTempFileName[pos++]=PATH_SEPARATOR;
  len=strlen(lpPrefixString);  if (len>3) len=3;
  memcpy(lpTempFileName+pos, lpPrefixString, len);  pos+=len;
#ifdef UNIX
  LONG tm=0;
    timeval  tv;  
    gettimeofday(&tv, NULL);   
    tm=tv.tv_usec+tv.tv_sec;
#else
  LONG tm=GetCurrentTicks();
#endif
  do
  { char buf[9];
    if (cycle++ > 200) return 0;
    sprintf(buf, "%08lX", tm++);
    for (int i=0;  i<8-len;  i++) lpTempFileName[pos+i]=buf[7-i];
    strcpy(lpTempFileName+pos+8-len, ".TMP");
    hnd=open(lpTempFileName, O_RDONLY, 0600);
    if (hnd!=-1) { close(hnd);  continue; }
    hnd=creat(lpTempFileName, 0600);
  } while (hnd==-1);
  close(hnd);
  return tm-1;
}

HANDLE FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
#ifdef UNIX
{ char FilePath[MAX_PATH];
  strcpy(FilePath, lpFileName);
  int FNL = strlen(FilePath);
  int i=0;
  do  /* Copy pathname to FilePath */
  {
    if (FilePath[FNL] == PATH_SEPARATOR) 
    {
      strcpy( lpFindFileData->Pat, FilePath+FNL+1);
      FilePath[FNL]=0;
      FNL=1;
    }
    FNL--;
  }
  while (FNL>0);
  lpFindFileData->PatL = strlen( lpFindFileData->Pat);
  do
  {
    if (lpFindFileData->Pat[i] == '*') lpFindFileData->PatL = i;
    i++;
  }
  while ( i < lpFindFileData->PatL ); 
  DIR * dirStructP = opendir(FilePath);
  if (dirStructP==NULL) return INVALID_HANDLE_VALUE;
  dirent * direntp;
  do
  {
    direntp = readdir(dirStructP);
    if (direntp==NULL) { closedir(dirStructP);  return INVALID_HANDLE_VALUE; }
  }
  while (strncmp(direntp->d_name, lpFindFileData->Pat, lpFindFileData->PatL));
  strcpy(lpFindFileData->cFileName, direntp->d_name);
  return (HANDLE)dirStructP;
}

#else

{ DIR * dirStructP = opendir(lpFileName);
  if (dirStructP==NULL) return INVALID_HANDLE_VALUE;
  dirent * direntp = readdir(dirStructP);
  if (direntp==NULL) { closedir(dirStructP);  return INVALID_HANDLE_VALUE; }
  strcpy(lpFindFileData->cFileName, direntp->d_name);
  return (HANDLE)dirStructP;
}
#endif


BOOL FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
#ifdef UNIX

{
  HANDLE dirStructP = hFindFile;
  dirent * direntp;
  do
  {
    direntp = readdir((DIR *)dirStructP);
    if (direntp==NULL)  return FALSE;
  }
  while (strncmp(direntp->d_name, lpFindFileData->Pat, lpFindFileData->PatL));
  strcpy(lpFindFileData->cFileName, direntp->d_name);
  return TRUE;
}

#else

{ if (!hFindFile || hFindFile==INVALID_HANDLE_VALUE) return FALSE;
  DIR * dirStructP = (DIR*)hFindFile;
  dirent * direntp = readdir(dirStructP);
  if (direntp==NULL) return FALSE;
  strcpy(lpFindFileData->cFileName, direntp->d_name);
  return TRUE;
}

#endif

BOOL FindClose(HANDLE hFindFile)
{ if (!hFindFile || hFindFile==INVALID_HANDLE_VALUE) return FALSE;
  DIR * dirStructP = (DIR*)hFindFile;
  closedir(dirStructP);
  return TRUE;
}

#ifdef SRVR
BOOL SetFileSize(FHANDLE hnd, tblocknum block_cnt) // returns TRUE iff OK
{
#ifdef UNIX
  return !ftruncate(hnd, (uns64)block_cnt * BLOCKSIZE);
#else
  return !chsize(hnd, block_cnt * BLOCKSIZE);
#endif
}
#endif
///////////////////////////////// other auxiliary functions /////////////////////////
#ifdef UNIX
int memicmp(const void *buf1, const void *buf2, unsigned int count)
{ const uns8 * p1 = (const uns8 *)buf1,  * p2 = (const uns8 *)buf2;
  while (count--)
  { uns8 c1 = mcUPCASE(*p1), c2 = mcUPCASE(*p2);
    if (c1<c2) return -1;
    if (c1>c2) return  1;
    p1++;  p2++;
  }
  return 0;
}

/* Sleep specified time. */
void Sleep(uns32 miliseconds)
{
    timespec ts;
    ts.tv_sec=miliseconds/1000;
    ts.tv_nsec=(miliseconds%1000)*1000000;
    nanosleep(&ts, NULL);
}

#endif

#else // WINS

#ifdef SRVR
BOOL SetFileSize(FHANDLE hnd, tblocknum block_cnt) // return TRUE iff OK
{ unsigned _int64 off64 = (unsigned _int64)block_cnt*BLOCKSIZE;
  SetFilePointer(hnd, (DWORD)off64, ((long*)&off64)+1, FILE_BEGIN);
  return SetEndOfFile(hnd);
}

#endif // !WINS
#endif
////////////////////////////// threading and waiting //////////////////////////////////////
#ifdef WINS
#include <process.h>

BOOL THREAD_START_JOINABLE(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle)
{ unsigned id;  THREAD_HANDLE hnd;
  hnd=(THREAD_HANDLE)_beginthreadex(NULL, stack_size, name, param, 0, &id); // returns 0 on error
  if (handle) *handle=hnd;
  return hnd!=INVALID_THANDLE_VALUE;
}

BOOL THREAD_START(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle)
{ unsigned id;  THREAD_HANDLE hnd;
  hnd=(THREAD_HANDLE)_beginthreadex(NULL, stack_size, name, param, 0, &id); // returns 0 on error
  if (handle) *handle=hnd;
  if (hnd!=0)
  { CloseHandle(hnd);
    return TRUE;
  }
  return FALSE;
}

void THREAD_JOIN(THREAD_HANDLE thread_handle)
{ WaitForSingleObject(thread_handle, INFINITE);
  CloseHandle(thread_handle);
}

void THREAD_DETACH(THREAD_HANDLE thread_handle)
{ CloseHandle(thread_handle); }

#ifdef SRVR
BOOL Sleep_cond(uns32 miliseconds) // wait until the time elapses or until the server shutdown
// return FALSE on server shutdown, TRUE if waited
{ return WaitForSingleObject(hKernelLockEvent, miliseconds) == WAIT_TIMEOUT; }
#endif  // SRVR

#else
#ifdef UNIX ///////////////////

BOOL THREAD_START(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle)
{ THREAD_HANDLE hnd;  
  if (pthread_create(&hnd, NULL, name, param))  // returns 0 on success
    { perror("pthread_create");  hnd=0; }
  else pthread_detach(hnd);  // set the detached state
  if (handle) *handle=hnd;
  return hnd!=0;
}

BOOL THREAD_START_JOINABLE(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle)
{ THREAD_HANDLE hnd;
  if (pthread_create(&hnd, NULL, name, param))  // returns 0 on success
    { perror("pthread_create");  hnd=INVALID_THANDLE_VALUE; }
  if (handle) *handle=hnd;
  return hnd!=INVALID_THANDLE_VALUE;
}

void THREAD_JOIN(THREAD_HANDLE thread_handle)
{ pthread_join(thread_handle, NULL); }

void THREAD_DETACH(THREAD_HANDLE thread_handle)
{ pthread_detach(thread_handle); }

#ifdef SRVR
/* wait until the time elapses or until a shutdown specified arrives.
 * return FALSE on condition, TRUE if waited. The change of the shutdown
 * generally should be protected by a lock. */
BOOL Sleep_cond(uns32 miliseconds, const bool_condition &cond)
{
  int res=0;
  timespec ts;
  ms2abs(miliseconds, ts);
  hKernelLockEvent.lock();
  while (!cond && res!=ETIMEDOUT)
	  res=hKernelLockEvent.wait(ts);
  hKernelLockEvent.unlock();
  return (res==ETIMEDOUT);
}
#endif // SRVR


#else // NLM /////////////////

CFNC int _beginthread(void (*start_address)(void *), void *stack_bottom, unsigned stack_size, void *arglist);
CFNC void _endthread(void);
#undef OUT_OF_MEMORY
#include <niterror.h>

BOOL THREAD_START(THREAD_PROC_TYPE * name, unsigned stack_size, void * param, THREAD_HANDLE * handle)
{ THREAD_HANDLE hnd;
  hnd=(THREAD_HANDLE)_beginthread(name, NULL, stack_size, param);  // returns -1 on error
  if (hnd==-1) hnd=0; // error
  if (handle) *handle=hnd;
  return hnd!=0;
}

#ifdef SRVR
BOOL Sleep_cond(uns32 miliseconds) // wait until the time elapses or until the server shutdown
// return FALSE on server shutdown, TRUE if waited
{ int res = TimedWaitOnLocalSemaphore(hKernelShutdownEvent, miliseconds);
  if (!res) SignalLocalSemaphore(hKernelShutdownEvent); // re-open the semaphore
  return res == ERR_TIMEOUT_FAILURE;
}
#endif //SRVR

#endif
///////////////////////////// Unicode ////////////////////////////
CFNC DllKernel size_t wuclen(const wuchar * wstr)
{ size_t len = 0;
  if (wstr!=NULL)
    while (*wstr)
      { wstr++;  len++; }
  return len;
}

CFNC DllKernel wuchar * wuccpy(wuchar * wdest, const wuchar * wsrc)
{ wuchar * origdest = wdest;
  if (wdest!=NULL && wsrc!=NULL)
  { while (*wsrc)
      *(wdest++) = *(wsrc++);
    *wdest=0;
  }
  return origdest;
}

int wucnicmp(const wuchar * s1, const wuchar * s2, int maxlength)
{ while (maxlength>0)
  { wuchar c1 = __toupper_l(*s1, wbcharset_t(0).locales16());
    wuchar c2 = __toupper_l(*s2, wbcharset_t(0).locales16());
    if (c1==c2)
    { if (!c1) break;  // both strings end here
      s1++;  s2++;  maxlength--;  // continue comparing
    }
    else // different (one may end here)
      return (c1>c2) ? 1 : -1;
  }
  return 0;
}

int wucncmp(const wuchar * s1, const wuchar * s2, int maxlength)
{ while (maxlength>0)
  { if (*s1==*s2)
    { if (!*s1) break;  // both strings end here
      s1++;  s2++;  maxlength--;  // continue comparing
    }
    else // different (one may end here)
      return (*s1>*s2) ? 1 : -1;
  }
  return 0;
}

wuchar *wusrchr(const wuchar *s1, int c)
{ wuchar *res = NULL;
  for (const wuchar *p = s1; *s1; p++)
  	if ((int)*p == c)
	  res = (wuchar *)p;
  return res;
}

#include <errno.h>
DWORD GetLastError(void)
{ return errno; }

#endif // !WINS ///////////////////////////////////////////////////////////////////
