//
//  serverux - WinBase Server for UNIX, main and monitor functions
//  =================================================================
//

#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h>

#include <sys/stat.h>
#include "winrepl.h"
#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#include "netapi.h"
#include "tcpip.h"
#include "nlmtexts.h"
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <locale.h>
#include <assert.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "termios.h"
#include <iconv.h>
#include <langinfo.h>
#include <fcntl.h>

#include <argz.h>
#include "regstr.h"
#include "enumsrv.c"

const char * terminal_charset;  // name of the terminal charset
iconv_t msgtoterm; /* character conversion prepared for the terminal output */
static bool bad_terminal=false; /* true if terminal failed once */
struct passwd * server_account = NULL;

/* set to always use udp 5001 */
int alternate_sip=0;

unsigned char mac_address[6];

bool get_mac_address(void)
/* Get the hardware address to interface "ethX" (the ethernet adress of the first existing card), 
   and store it to mac_address. Return true on success. */
{ struct ifreq ifr;  int sockfd; int i;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return false;
  for (i=0;  i<8;  i++)
  { sprintf(ifr.ifr_name, "eth%u", i);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) != -1)
      break;
  }
  if (i==8)  // cannot find any MAC address, using NULL instead
    memset(ifr.ifr_hwaddr.sa_data, 0, 6); //{ close(sockfd);  return false; }
  close(sockfd);
  memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
  return true;
}

// command line paramaters:
static BOOL is_daemon = FALSE;  /* server runs as daemon */
tobjname servername = "";
static char * K_Passwd = NULL;  /* Kernel password from command line */
static BOOL sys_log = FALSE;    // copying the console output (basic log) to the syslog
static bool printing_pid=false; // Print pid after daemonizing if set

static pthread_t first_thread;  // main server thread, signals causing unload are redirected to it

///////////////////////////////////////////////////////////////////////
#if 0
/* Kodovani cestiny: provede se konverze pomoci iconv; jakmile jednou selze
 * (typicky protoze terminal je latin1), prejde se na fallbackovou metodu
 * prevodu do "bez hacku a carek" */

char* no_diacrit(char *str, const char *orig, size_t bufsize)
{
  char *ret=str;  // the pointer to the result to be returned
 // translation:
#ifdef ENGLISH  // messages, if translated, are in UTF-8
  orig=gettext(orig);
#endif
 // try conversion by iconv:
  if (!bad_terminal)
  { size_t inleft=1+strlen(orig), outleft=bufsize;
    size_t res=iconv(msgtoterm, (char**)&orig, &inleft, &str, &outleft);
    if ((size_t)-1 != res) return ret;  // converted OK
    bad_terminal=true; /* too bad... */
  }
 // remove diacritics from the (part of the) string not converted by iconv:
  static const uns8 bez_outcode[128]={
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','S',' ','S','T','Z','Z',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','s',' ','s','t','z','z',
    ' ',' ',' ','L',' ','A',' ',' ',' ',' ','S',' ',' ',' ',' ','Z',
    ' ',' ',' ','l',' ',' ',' ',' ',' ','a','s',' ','L',' ','l','z',
    'R','A','A','A','A','L','C','C','C','E','E','E','E','I','I','D',
    'D','N','N','O','O','O','O',' ','R','U','U','U','U','Y','T',' ',
    'r','a','a','a','a','l','c','c','c','e','e','e','e','i','i','d',
    'd','n','n','o','o','o','o',' ','r','u','u','u','u','y','t',' '};
  while (*orig && str<ret+bufsize)
  { if ((unsigned char)*orig>=128)
      *str=bez_outcode[(unsigned char)*orig-128];
    else *str=*orig;
    str++, orig++;
  }
  *str=0;
  return ret;
}
#endif

/********************  user interface routines  **********************
Rules for console acess:
- fatal errors detected on server start go to stderr,
- interactive communication between the operator and the server goes to stdout via convert_and_put,
- output to the basic log is echoed to stdout (and possibly syslog) via write_to_server_console.
*/
void WriteSlaveThreadCounter(void)
// Not used, this information may be obtained by tracing the logins
{ }

static void fputsToScrollRegion(const char *str)
// Outputs str to stdout in reentrant way. 
// [str] is supposed to be converted to the terminal charset.
{ static CRITICAL_SECTION hScreenSem=PTHREAD_MUTEX_INITIALIZER;
  EnterCriticalSection(&hScreenSem);
  fputs(str, stdout);
  fputs("\n", stdout);
  LeaveCriticalSection(&hScreenSem);
}

void remove_diacr(char * term_text, const char * text)
{ 
  while (*text)
  { *term_text = (unsigned char)*text >= 128 ? '?' : *text;
    text++;
    term_text++;
  }
  *term_text = 0;
}

void write_to_server_console(cdp_t cdp, const char * text)
// Writes [text] to console/syslog, unless the server is daemon. (called e.g. when writing to the basic log)
// [text] is in the system charset, translated.
// [charset_sys_to_terminal] should not persist because [sys_spec.charset] changes during the startup of the server.
{ if (sys_log) 
  { char * buf2 = (char*)corealloc(OBJ_NAME_LEN+3+strlen(text)+1, 77);
    if (buf2)
    { strcpy(buf2, servername);  strcat(buf2, ": ");  strcat(buf2, text);
      syslog(LOG_INFO, buf2);
      corefree(buf2);
    }
    else 
      syslog(LOG_INFO, text);
  }    
  if (is_daemon) return;
 // convert from the system charset to the terminal charset (remove diacr. if cannot):
  iconv_t charset_sys_to_terminal = iconv_open(terminal_charset, wbcharset_t(sys_spec.charset ? sys_spec.charset : CHARSET_NUM_UTF8));
  int len = strlen(text);
  size_t inleft=len+1, outleft=2*len+1;  // UTF-8 may need this additional space (outleft)!
  char * term_text = new char[outleft];
  if (charset_sys_to_terminal != (iconv_t)-1)
  { const char * p1 = text;  char * p2 = term_text;
    size_t res = iconv(charset_sys_to_terminal, (char**)&p1, &inleft, &p2, &outleft);
    iconv_close(charset_sys_to_terminal);
    if (res == (size_t)-1)
      remove_diacr(term_text, text);
  }
  else
    remove_diacr(term_text, text);
 // output:
  fputsToScrollRegion(term_text);
  delete [] term_text;
}

void write_to_stderr(const char * text)
// [text] is in the system charset, translated.
// [charset_sys_to_terminal] should not persist because [sys_spec.charset] changes during the startup of the server.
{ 
 // convert from the system charset to the terminal charset (remove diacr. if cannot):
  iconv_t charset_sys_to_terminal = iconv_open(terminal_charset, wbcharset_t(sys_spec.charset ? sys_spec.charset : CHARSET_NUM_UTF8));
  int len = strlen(text);
  size_t inleft=len+1, outleft=2*len+1;  // UTF-8 may need this additional space (outleft)!
  char * term_text = new char[outleft];
  if (charset_sys_to_terminal != (iconv_t)-1)
  { const char * p1 = text;  char * p2 = term_text;
    size_t res = iconv(charset_sys_to_terminal, (char**)&p1, &inleft, &p2, &outleft);
    iconv_close(charset_sys_to_terminal);
    if (res == (size_t)-1)
      remove_diacr(term_text, text);
  }
  else
    remove_diacr(term_text, text);
 // output:
  fputs(term_text, stderr);
  delete [] term_text;
}

static void convert_and_put(const char *str)
// Writes the messages without parameters to stdout.
{ char buf[241];  // max 120 characters, some may take double space in UTF-8
  if (is_daemon) return;
  form_message(buf, sizeof(buf), str);  // translates, converts to system charset
  write_to_server_console(NULL, buf);   // writes to stdout
}

static void convert_and_stderr(const char *str)
// Writes the messages without parameters to stderr.
{ char buf[241];  // max 120 characters, some may take double space in UTF-8
  form_message(buf, sizeof(buf), str);  // translates, converts to system charset
  write_to_stderr(buf);   // writes to stderr
}

/* SIGHUP is traditionaly used to force the server reload its settings. At the
 * moment, only few settings are reloaded. This function is also called when
 * the server is being initialized. */
static void load_wbkernel_settings(int sig)
{ signal(SIGHUP, load_wbkernel_settings); } /* is reset after call */

/**
 * Input type switch
 * @off: 0 to start noncanonical, other to start canonical
 *
 * Set noncanonical input (direct passing characters, no line editing) plus
 * turn off echoing. Should be called once more at the end of the program with
 * non-zero parametr so that to get the terminal back.
 */
void noncanonical_input(int off)
{
    	if (!isatty(0)) return;
        static tcflag_t oldvals=0;
	struct termios term;
	if (tcgetattr(0, &term))
	    perror("tcgetattr");
	if (off && oldvals==0) return;
	if (off) term.c_lflag=oldvals;
	else {
	       oldvals=term.c_lflag;
       	       term.c_lflag&=(~(ICANON|ECHO));
	}
	if (0!=tcsetattr(0, TCSANOW, &term))
	    perror("tcsetattr");
}

static void InitialMessage(int sig=0)
/*************************/
{ if (is_daemon) return;
  char buf[161];  // max 80 characters, some may take double space in UTF-8
  form_message(buf, sizeof(buf),  // translates, converts to system charset
#if WBVERS<900
    initial_msg8, servername, WB_VERSION_STRING_EX, VERS_4);
#else
    initial_msg9, servername, VERS_STR);
#endif  
  write_to_server_console(NULL, buf);
 // date time:
  char szTime[80];
  time_t time_v = time(NULL);
  strftime(szTime, sizeof(szTime), /*nl_langinfo(D_T_FMT)*/"%d.%m.%Y %H:%M:%S", localtime(&time_v));
  form_message(buf, sizeof(buf), loaded, szTime);
  if (server_account) 
  { int pos = strlen(buf);
    form_message(buf+pos, sizeof(buf)-pos, account, server_account->pw_name);
  }
  write_to_server_console(NULL, buf);
  convert_and_put(exitPrompt);
}

typedef enum {ET_QUIT, ET_ABORT} exit_type;

static void ScreenAndLogClose(exit_type iExitType)
/* Close screen and log. Closing message depends on the parameter passed. */
{ if (is_daemon) return;
  switch (iExitType)
  { case ET_QUIT:
      convert_and_put(winBaseQuit);      break;
    case ET_ABORT:
      convert_and_put(winBaseAbort);     break;
  }
  noncanonical_input(1);
}

BOOL YesNoPrompt(BOOL answer)
/***************************************/
{ 
#ifdef ENGLISH
  const char * cYes = _("Yes"), * cNo = _("No");
#else
  const char * cYes = "A", * cNo = "N";
#endif
  int ci = getchar();
  char cAns = toupper(ci);
  if (cAns==toupper(*cYes) || cAns=='A') return TRUE; // problem with slovak A'no - must explicitly allow A.
  if (cAns==toupper(*cNo )) return FALSE;
  return answer;
}

bool get_database_file_password(char * password)
/*******************************************/
{ if (K_Passwd != NULL)  // password specified on the command line
  { strncpy(password, K_Passwd, MAX_FIL_PASSWORD);
    return TRUE;
  }
  if (is_daemon) return FALSE;
 // enter the password interactively:
  convert_and_put(enterServerPassword);
  if (fgets( password, MAX_FIL_PASSWORD, stdin)==NULL) return FALSE;
  int pos=0;
  do  // replace \n => 0
    pos++;
  while (password[pos]!='\n' && pos<MAX_FIL_PASSWORD);
  password[pos]=0;
  return TRUE;
}

/* Vypise seznam prohlasenych uzivatelu. Deklarace v kurzor.h. */

void PrintAttachedUsers(void)
/****************************/
{ int i, printedRows;  char szLine[81];
  if (!slaveThreadCounter)
    convert_and_put(noAttached);
  else
  for (printedRows = 0, i = 1;  i <= max_task_index;  i++)
    if (cds[i] != NULL && cds[i]->in_use != PT_WORKER)
    {/* screen test */
      if (++printedRows % 23 == 0)
      { convert_and_put(pressKey);
        getchar();
      } 
     /* user name, max 25 chars, add spaces to 25 chars */
      logged_user_name(cds[i], cds[i]->prvs.luser(), szLine);
      int len=strlen(szLine);
      if (len<25) memset(szLine+len, ' ', 25-len); 
      szLine[25]=' ';
      int2str(cds[i]->wait_type, szLine+26);
      strcat(szLine, " ");
      if (cds[i]->in_use == PT_MAPSLAVE)
        strcat(szLine, "Local client");
      else
        if (cds[i]->pRemAddr) cds[i]->pRemAddr->GetAddressString(szLine+strlen(szLine));
      write_to_server_console(NULL, szLine);  // szLine is in system charset!
    }
}
//////////////////////////// test iconv ////////////////////////////////////
/********************************   main   **********************************/
void pid_file(bool create_it)
// Creates or deletes the pid-file
{ char path[MAX_PATH];
  strcpy(path, ffa.last_fil_path());
  append(path, "pid.txt");
  if (create_it)
  { int fh = open(path, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fh!=-1)
    { char buf[10];
      sprintf(buf, "%u", getpid());
      write(fh, buf, strlen(buf));
      close(fh);
    }
  }
  else
    unlink(path);  
}

static void SafeKernelClose(bool downing_server_by_operator)
/**********************************/
{
 // ask the operator about waiting for connected clients to disconnect:
  if (downing_server_by_operator && slaveThreadCounter)
  { convert_and_put(downWarning);
    if (YesNoPrompt(TRUE))
      client_logout_timeout = 60;
  }
 // delete the pid-file:
  pid_file(false);
  dir_kernel_close();
}

void stop_the_server(void)
// Stopping the server from another thread (e.g. client)
{ 
  pthread_kill(first_thread, SIGTERM);  // calls UnloadProc in the proper thread, doesn not block the calling thread
}

/* After initialization in main() set of all signals. Used by slave and
 * listening threads in order to block signals. Thus, signals are served by the
 * main thread only. */
sigset_t all_signals;
/* Je to nekonzistentni, pomocna vlakna volaji block_all_signals, coz blokuje signaly z mnoziny
   sigset, ktera neni nikde definovana. Vlakna slaves nyni nevolaji nic. */

/* User usually sends TERM with kill.
 * System shutdown script has been modified so that to send PWR.
 * Former AbortProc() is also handled here.
 * Explicit termination by 'q' from keyboard is also handled here, with
 * sig=NSIG (which is maximum signal number+1)
 *
 * Assert:
 *	- daemon by nemel mit sanci chytit signal od klavesnice
 */
void __attribute__((noreturn)) UnloadProc(int sig)
{
  static volatile BOOL already_handling_signal=FALSE;
  if (pthread_self()!=first_thread){
	  pthread_kill(first_thread, sig);
	  for (;;) pause();
	  }
  if (sig==SIGTERM) Sleep(200);  // time for the issuing client to disconnect
  if (already_handling_signal) raise(sig);
  already_handling_signal=TRUE;
  alarm(80); /* If something hangs up, terminate by alarm anyway */
  signal(SIGALRM, SIG_DFL); /* default is end process - fine for us */
  if (sig==SIGPWR) down_reason = down_pwr;
  else if (sig==NSIG || sig==SIGINT || sig==SIGQUIT) down_reason=down_console;
  else if (sig==SIGABRT) down_reason=down_abort;
  SafeKernelClose (sig==NSIG);
  ScreenAndLogClose (sig==SIGABRT?ET_ABORT:ET_QUIT);
  core_disable(); // do not call core_release: exit will call the destructors of static classes (in crypto e.g. Integer::Zero) and they work with the memory (memset() in SecFree() in ~SecBlock())
  if (sig<NSIG){ /* re-raise signal */
	  _exit(0);
	  signal(sig, SIG_DFL);
	  kill(0,sig);
  }
  exit(0);
}

void bad_signal(int);

/* Check whether backup. Details in filbckup.cpp. */
static void TimerProc(int)
{ if (fShutDown) return;
  if (!fLocking) periodical_actions(my_cdp);
  alarm(50); /* check every 50 secs */
}

/* Prehled pouzivanych signalu:
   (ukoncovani)    TERM, PWR, INT, QUIT: shutdown serveru; detaily shutdownu zavisi na serveru
   (fatalni chyby) ABRT, SEGV, ILL, FPE, BUS, SYS: totez, pokud se vyskytnou v dynamicke knihovne zachyceny
   (periodicke)    ALRM: pouzivan pro zalohovani filu atd.; pri shutdownu k nasilnemu ukonceni
   (job kontrol)   CONT: po preruseni procesu.
 */
static void set_signals(void)
/**********************/
{
  signal(SIGTERM, UnloadProc);  // termination signal
  signal(SIGPWR,  UnloadProc);  // power failure
  signal(SIGINT, UnloadProc);   // interrupt from the keyboard
  signal(SIGQUIT, UnloadProc);  // termination from the keyboard
//  signal(SIGABRT, bad_signal);
//  signal(SIGURG, signal_oob);
//  siginterrupt(SIGURG, 1);
//  signal(SIGILL, bad_signal);
//  signal(SIGFPE, bad_signal);
//  signal(SIGBUS, bad_signal);
//  signal(SIGSYS, bad_signal);
//  signal(SIGSEGV, bad_signal);
  signal(SIGALRM, TimerProc); /* Check whether back up etc. periodically */
  // signal(SIGCONT, InitialMessage); // continuation after interrupt
  alarm(40); /* and start soon */
  signal(SIGPIPE, SIG_IGN); /* prisel signal pipe, neco se nepodarilo odeslat
			     *  do socketu nic s tim delat nebudu */
  /* Note that SIGHUP is also registered in load_wbkernel_settings() ... */
}

static void swap_trace(cdp_t cdp, int trace_type,
  t_property_descr_int &descr, const char *on_str, const char *off_str)
{ trace_def(cdp, trace_type, NULLSTRING, NULLSTRING, NULLSTRING, !descr.val());
  convert_and_put(descr.val() ? on_str : off_str);
}

/* Cte znaky z klavesnice a reaguje na ne. Nikdy se nevraci - pokud je zadano
 * 'q', ukonci program pomoci volani UnloadProc. */
static void __attribute__((noreturn)) KeyboardScanner(void)
  /************************/
{ char buf[128];
  noncanonical_input(0);  // must be called after entering the password, but before normal interactions
  for (;;) {
    int c=getchar();
    if (c==EOF) UnloadProc(NSIG);  // console closed
    switch (tolower(c)){
    case 'i':
      form_message(buf, sizeof(buf), memorySize, total_used_memory());
      write_to_server_console(NULL, buf);
      *buf=0;
      if (wProtocol & TCPIP_NET) 
        sprintf(buf,             "TCP/IP port: %u  ", the_sp.wb_tcpip_port.val());  // protocol always in use
      if (wProtocol & TCPIP_HTTP) 
        sprintf(buf+strlen(buf), "HTTP: %u  ",        http_tunnel_port_num);
      if (wProtocol & TCPIP_WEB)
        sprintf(buf+strlen(buf), "WWW: %u",           http_web_port_num);
      write_to_server_console(NULL, buf);
#if WBVERS<1100
      unsigned total, used;
      client_licences.get_info(&total, &used);
      form_message(buf, sizeof(buf), licenseInfo, used, total, client_licences.licences_consumed);
      write_to_server_console(NULL, buf);
#endif
      break;
    case 'm':
      PrintHeapState(display_server_info);
      break;
    case 'u':
      PrintAttachedUsers();
      break;
    case 'q': {
      convert_and_put(sureToExit);
      wbbool escape = YesNoPrompt(FALSE);
      if (!escape) convert_and_put(notUnloaded);
      else UnloadProc(NSIG);
    } break;
    case 'l':
      swap_trace(my_cdp, TRACE_USER_ERROR, the_sp.trace_user_error, user_log_on, user_log_off);
      commit(my_cdp);
      break;
    case 't':
      swap_trace(my_cdp, TRACE_NETWORK_GLOBAL, the_sp.trace_network_global, trace_is_on, trace_is_off);
      commit(my_cdp);
      break;
    case 'r':
      swap_trace(my_cdp, TRACE_REPLICATION, the_sp.trace_replication, replic_log_on, replic_log_off);
      trace_def(my_cdp, TRACE_REPL_CONFLICT, NULLSTRING, NULLSTRING, NULLSTRING,  !the_sp.trace_replication.val());
      commit(my_cdp);
      break;
    case 'h': case '?':
      convert_and_put(stmt_info1);
      convert_and_put(stmt_info2);
      break;
    case 0: // extended key
      getchar();
      break; // skip its code
    case 'v':
      form_message(buf, sizeof(buf), linux_version_info, SERVER_FILE_NAME, VERS_STR, __DATE__, __TIME__, COMPILED_WITH);
      write_to_server_console(NULL, buf);
      break;
    };
  };
}

/* If we run as root, let us change the persona */
inline static void custom_setuid(void)
{
  if (getuid()!=0) return;
  char username[100];
  int res=GetDatabaseString(servername, "USER", username, sizeof(username), my_private_server);
  if (res)
  { if ((server_account=getpwnam(username))!=NULL)
    { if(setuid(server_account->pw_uid)) 
      perror("setuid");
    }
  }  	  
}

/* Set root directory */
inline static void custom_chroot(const char *kernelpath)
{
    if (getuid()!=0) return; /* Only root can chroot */
    char chrootdir[100];
    int res=GetDatabaseString(servername, "CHROOT", chrootdir, sizeof(chrootdir), my_private_server);
    if (!res) return;
    if(chroot((strcasecmp(chrootdir, "PWD"))? chrootdir: kernelpath))
	    perror("chroot");
}

/* Vr��server s daty v uveden� adres�i, p�adn�jediny existuj��server
 * (pokud je inode_t=-1)
 */
static bool server_from_inode(ino_t inode, dev_t device)
{ char buffer[4096];
  int len=GetPrivateProfileString(NULL, NULL, "", buffer, sizeof(buffer), configfile);
  char *entry=NULL;
  *servername=0;
  while (entry=argz_next(buffer, len, entry))
  { if (strlen(entry) >= sizeof(tobjname)) continue;
    char path[256];  struct stat info;
    if (!GetDatabaseString(entry, "Path",  path, sizeof(path), my_private_server) &&
        !GetDatabaseString(entry, "PATH1", path, sizeof(path), my_private_server)) continue;
    if (-1==stat(path, &info)) continue;
    if (-1==inode)  // searching for the only database
      if (*servername) return false;  // there are multiple databases
      else strcpy(servername, entry);
    else if (info.st_dev==device && info.st_ino==inode)
      { strcpy(servername, entry);  break; }
  }
  return *servername!=0;
}
/*
 * Vr��jm�o serveru v dan� adres�i nebo NULL
 * Nekotroluje se jm�o jako takov� ale zda-li uveden�cesta a cesta zadan� * v PATH odpov�aj��sekce jsou stejn adres� (take se nenech�zm�t
 * symlinky, relativn�i cestami, pebyte�mi / apod.
 */
static bool servername_from_path(const char *searched_path)
{
  struct stat info;
  dev_t device;
  ino_t inode;
  if (-1==stat(searched_path,&info)){
    perror(searched_path);
    return false;
  }
  if (!S_ISDIR(info.st_mode))
  { char buf[200];
    form_message(buf, sizeof(buf), isNotDir, searched_path);  // translates, converts to system charset
    write_to_stderr(buf);   // writes to stderr
    return false;
  }
  return server_from_inode(info.st_ino, info.st_dev);
}

void print_existing_databases(bool autostart_only)
// Print the list of local databases
{ tobjname a_server_name;  char a_path[MAX_PATH];  char autostart_opt[10];
  t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
  while (es.next(a_server_name))
    if (!autostart_only || 
        es.GetDatabaseString(a_server_name, "AUTOSTART",  autostart_opt, sizeof(autostart_opt)) 
        && strcmp(autostart_opt, "0"))     
      if (es.GetPrimaryPath(a_server_name, a_path) && *a_path)
        fprintf(stdout, "%s\n", a_server_name);
}

/* Read options from the command line arguments. */
static void parse_options(int argc, char **argv, BOOL & traceon_usererr, BOOL & traceon_replic, BOOL & traceon_trace)
{ traceon_usererr=traceon_replic=traceon_trace=FALSE;
    static const option long_options[]={
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {"dbpath", required_argument, NULL, 'f'},
    {"dbname", required_argument, NULL, 'n'},
    {NULL, 0, NULL, '\0'}
  };
  int optnm;  char buf[1000];
  while(optnm=getopt_long(argc, argv, "AaPn:s:qf:detrlp:hv", long_options, NULL), optnm!=-1)
  {
    switch (optnm)
    {
      case 'A':
        alternate_sip=1;
        break;
      case 'a': // used when starting a runlevel
        print_existing_databases(true);
        exit(EXIT_SUCCESS); 
      case 'q':
        disable_system_triggers=TRUE;  minimal_start=true;
        break;
      case '?':
        convert_and_stderr(syntaxError);
        form_message(buf, sizeof(buf), serverCommLine, SERVER_FILE_NAME);  // translates, converts to system charset, substitutes parameters
        write_to_stderr(buf);   // writes to stderr
        exit(EXIT_FAILURE);
        break;
      case 'f':
        if (!servername_from_path(optarg))
        { form_message(buf, sizeof(buf), noDbInDir, optarg);  // translates, converts to system charset, substitutes parameters
          write_to_stderr(buf);   // writes to stderr
          exit(EXIT_FAILURE);
        }
        break;
      case 'n':
      case 's':  // silent alternative
        strmaxcpy(servername, optarg, sizeof(tobjname));
        break;
      case 'd':
        is_daemon = TRUE;  is_service=true;
        break;
      case 'e':
        traceon_usererr = TRUE;
        break;
      case 't':
        traceon_trace = TRUE;
        break;
      case 'r':
        traceon_replic = TRUE;
        break;
      case 'l':
        sys_log = TRUE;
        break;
      case 'p':
        K_Passwd =optarg;
        break;
      case 'h':
        form_message(buf, sizeof(buf), serverCommLine, SERVER_FILE_NAME);  // translates, converts to system charset, substitutes parameters
        write_to_stderr(buf);   // writes to stderr
        exit(0);
        break;
      case 'v':
        form_message(buf, sizeof(buf), linux_version_info, SERVER_FILE_NAME, VERS_STR, __DATE__, __TIME__, COMPILED_WITH);  // translates, converts to system charset, substitutes parameters
        write_to_stderr(buf);   // writes to stderr
        exit(0);
        break;
      case 'P':
        printing_pid=true;
        break;
    }
  }
 // check for non-options, none are allowed:
  if (optind<argc)
  { form_message(buf, sizeof(buf), nonOptionFound, argv[optind]);  // translates, converts to system charset, substitutes parameters
    write_to_stderr(buf);   // writes to stderr
    form_message(buf, sizeof(buf), serverCommLine, SERVER_FILE_NAME);  // translates, converts to system charset, substitutes parameters
    write_to_stderr(buf);   // writes to stderr
    exit(EXIT_FAILURE);
  }
}

/* Print error text describing error number ccode */
static void print_kse_error(int ccode)
{ 
  convert_and_stderr(kernelInitError);
  convert_and_stderr(kse_errors[ccode]);
  fputs("\n", stderr);
}

/* Fork. Have the parent wait for status confirmation from the child and exit.
 * The child then redirrects stdin, stdout and stderr to the
 * /dev/null (strictly speaking, nothing should ever be sent there/read from
 * there, but to be sure) and creates its own session group. (which implies
 * disconnecting from the terminal)
 *
 * The child and the father use pipe for communication, following variable
 * keeps the file descriptor from child to father.
 */
static int fathers_ear;  // child's end of the pipe to its father

static void daemonize(void)
{ static const char nulldevice[]="/dev/null";
  int filedes[2];
  if (-1==pipe(filedes)) perror("pipe");
  if (fork())
  {// father waits for the child initialization results:
    close(filedes[1]); /* asi neni nutne, stejne za chvili exiti */
    int ccode=KSE_SYNCHRO_OBJ; // init prevets error when read fails because child crashed
    if (-1==read(filedes[0], &ccode, sizeof(int))) { perror("read");  ccode=KSE_SYNCHRO_OBJ; }
    if (ccode==KSE_OK) // child started OK, father can exit
    { int pid;
      if (-1==read(filedes[0], &pid, sizeof(int))) perror("read");
      if (printing_pid) printf("%i ", pid);  // father prints pid of the child (\n replaced by space - autostart management needs this, but looks bad on the console)
      exit(0);
    }
    else  // error starting the child, write message and exit
    { print_kse_error(ccode);
      close(filedes[0]);
      exit(1);
    }
  }
  else
  { fathers_ear=filedes[1];
    close(filedes[0]);
#ifdef ORIG_TZ
    freopen(nulldevice, "r", stdin);  // crashed on RH 8.0
    freopen(nulldevice, "w", stdout);
#ifdef DEBUG
    freopen("/tmp/err", "w", stderr);
#else
    freopen(nulldevice, "w", stderr);
#endif
    setsid();
#else  // for RH8.0
    setsid();
    int sd = open(nulldevice, O_RDWR, 0);
    if (sd!=-1)
    { if (sd!=0) dup2(sd, 0);
      if (sd!=1) dup2(sd, 1);
      if (sd!=2) dup2(sd, 2);
      if (sd>2) close(sd);
    }
#endif
  }
}

/* The main function. It calls all the necessary initializations, starts the
 * kernel, and  - depending on whether daemon or not - or sleeps, or runs
 * keyboard checking loop.
 */

/* Database name vs. path: If path is specified, then it is converted to servername.
   Path if searched for the name and chdir is called on it.
   Path is obtained by getcwd and passed to kernel_init
   Path is converted to server_name in ffa.preinit. Horrible!!!
*/
extern "C" int  _nl_msg_cat_cntr;

int main (int argc, char *argv[])
/********************************/
{ BOOL traceon_usererr, traceon_replic, traceon_trace;  char buf[128];

  setlocale(LC_ALL, ""); /* monetary symbols used according to user environment setting */
#ifdef ENGLISH  // start gettext() in the international version
  //putenv("LANGUAGE=sk_SK");  // this works
  { char path[MAX_PATH];
    get_path_prefix(path);
    strcat(path, "/share/locale");
    bindtextdomain(GETTEXT_DOMAIN, path);
    bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");  // system charset woud be better but it changes
    textdomain(GETTEXT_DOMAIN);
  }
#endif
 // allow creating files with all privileges - used when creating wb8.fil, transact.fil, log files,...
  umask(0);
 // prepare the output string conversion to the terminal:
  terminal_charset = nl_langinfo(CODESET);
#if 0
#ifdef ENGLISH  // messages, if translated, are in UTF-8
  msgtoterm = iconv_open(terminal_charset, "utf-8");
#else // national language messages are in cd 1250
  msgtoterm = iconv_open(terminal_charset, "cp1250");
#endif
    if ((iconv_t)-1==msgtoterm)
      { bad_terminal=true;  perror("iconv_open"); }
#endif

  prep_regdb_access();
  parse_options(argc, argv, traceon_usererr, traceon_replic, traceon_trace);
  if (!*servername) // database not specified in options
    if (!server_from_inode((ino_t)-1, 0))
    { form_message(buf, sizeof(buf), dbNotSpecif, *argv);  // translates, converts to system charset, substitutes parameters
      write_to_stderr(buf);   // writes to stderr
      print_existing_databases(false);
      exit(EXIT_FAILURE); 
    }

 // the database is known now, find the language for its messages:
#ifdef ENGLISH  // messages, if translated, are in UTF-8
  { char lang[40], buf[50];
    if (GetDatabaseString(servername, "LanguageOfMessages", lang, sizeof(lang), my_private_server))
    { sprintf(buf, "LANGUAGE=%s", lang);
      if (putenv(buf)==-1)
	    fputs("Error setting the language environment.\n", stderr);
      sprintf(buf, "LANG=%s", lang);
      if (putenv(buf)==-1)
	    fputs("Error setting the language environment.\n", stderr);
      ++_nl_msg_cat_cntr;
      textdomain(GETTEXT_DOMAIN);
      char * trans = gettext("Continue?");
      trans=trans;
    }
  }
#endif
 // find MAC address:
  if (!get_mac_address())
    { convert_and_stderr(socketsNotActive);  exit(EXIT_FAILURE); }
#if 0
 // check server registration:
  if (!check_server_registration())  // Always true since 9.0
  { char ikbuf[MAX_LICENCE_LENGTH+1];  char url[200];
    convert_and_stderr(noRunLicence);
    if (!get_any_valid_ik(ikbuf, NULL)) exit(EXIT_FAILURE);  // error reported inside
    form_message(url, sizeof(url), serverLicenceURL, ikbuf);
    write_to_stderr(url);
    convert_and_stderr(enterRunLicence);
    exit(EXIT_FAILURE);
  }
#endif
 // find the database file path:
  { char kernelpath[255];
    if (!GetPrimaryPath(servername, false, kernelpath) &&
        !GetPrimaryPath(servername, true,  kernelpath))
      { form_message(buf, sizeof(buf), pathNotDefined, servername);  // translates, converts to system charset, substitutes parameters
        write_to_stderr(buf);   // writes to stderr
        exit(EXIT_FAILURE); 
      }

    if (-1==chdir(kernelpath))
      { form_message(buf, sizeof(buf), isNotDir, kernelpath);  // translates, converts to system charset, substitutes parameters
        write_to_stderr(buf);   // writes to stderr
        exit(EXIT_FAILURE); 
      }
    if (is_daemon) daemonize();  // father exits inside, child continues
    custom_chroot(kernelpath);
  }
  
  prepare_socket_on_port_80(servername);  // must open port 80 before changig the UID
  custom_setuid(); // changes the account of the server from root to defined USER // moved after kernel_init in order to make it possible to listen on port 80 -- no, problems with termnating threads

  set_signals();
  load_wbkernel_settings(0);  // no action, it is a placeholder only
  InitialMessage();
  if (!wbcharset_t::prepare_conversions())
    convert_and_put(noConversion);

 // init server with signals temporarily blocked:
  int ccode;
  { //char cwd[256]; getcwd(cwd, sizeof(cwd));
    sigemptyset(&all_signals); /* Nastavi all_signals na seznam nami pouzivanych signalu. */
    sigaddset(&all_signals, SIGHUP);
    sigaddset(&all_signals, SIGALRM);
    sigaddset(&all_signals, SIGPWR);
    sigaddset(&all_signals, SIGTERM);
    sigaddset(&all_signals, SIGINT);
    sigaddset(&all_signals, SIGPIPE);
    sigaddset(&all_signals, SIGCONT);
    pthread_sigmask(SIG_BLOCK, &all_signals, NULL);
    first_thread=pthread_self();
    ccode = kernel_init(my_cdp, servername, false);  // network interface will be switched ON when no other interface is active
    pthread_sigmask(SIG_UNBLOCK, &all_signals, NULL);
  }

  if (is_daemon) // send init result and pid to the father
  { int pid=getpid();
    write(fathers_ear, &ccode, sizeof(int));
    write(fathers_ear, &pid, sizeof(int));
    close(fathers_ear);
  }
 // exit on initialization error:
  if (ccode != KSE_OK)
  { if (!is_daemon) /* pokud jsme daemon, neni co psat, sent to father */
    { print_kse_error(ccode);  
	  sleep(3);  // user has the opportunity to read the error mesasage before the window closes.
	}
    return -1; 
  }  

 // write the pid-file:
  pid_file(true);
 // server initialised OK:
  if (traceon_usererr)
    trace_def(my_cdp, TRACE_USER_ERROR,     NULLSTRING, NULLSTRING, NULLSTRING, TRUE+1000);
  if (traceon_replic)
  { trace_def(my_cdp, TRACE_REPLICATION,    NULLSTRING, NULLSTRING, NULLSTRING, TRUE+1000);
    trace_def(my_cdp, TRACE_REPL_CONFLICT,  NULLSTRING, NULLSTRING, NULLSTRING, TRUE+1000);
  }
  if (traceon_trace)
    trace_def(my_cdp, TRACE_NETWORK_GLOBAL, NULLSTRING, NULLSTRING, NULLSTRING, TRUE+1000);

  if (is_daemon) for (;;) pause();
  else
  { if (printing_pid) printf("PID=%i\n", getpid());
    KeyboardScanner();
  }
  assert(0); /* this never returns */
}

/**
 * create_uuid - vygeneruj unikatni identifikator
 * @appl_id: pointer na misto pro nove uuid
 *
 * Generate unique user id. It consists of ethernet address, date of creation
 * and pid of creating process.
 */
CFNC void WINAPI create_uuid(WBUUID appl_id)
	/*******************************************/
{
#pragma pack(1)
	struct internal_t {
		uns8 lan[6];
		uns32 dt;
		uns16 pid;
	} * internal=(internal_t *)appl_id;
#pragma pack()
	assert (sizeof(WBUUID)==sizeof(struct internal_t));
	struct timeval TP;
	if (gettimeofday(&TP, NULL)) perror("GetTimeOfDay");

	memcpy(internal->lan, mac_address, 6);
	uns32 dt=Today();
	int year=Year(dt), month=Month(dt), day=Day(dt);
	internal->dt=Now()/1000 + 86400L*((day-1)+31L*((month-1)+12*(year-1996)));
	internal->pid=getpid()+(uns16) TP.tv_usec;
}

//#include "dwordptr.cpp"

extern "C" void set_err_msg(cdp_t cdp, const char* err)
{
  SET_ERR_MSG(cdp, err);
}

#if WBVERS>=1100
#elif WBVERS>=900
#include "/usr/local/src/wb8/comm/glibc23replac.c"
#endif

/* vim: set sw=2: */
