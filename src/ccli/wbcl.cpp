/************************************************************************
 * wbcl.cpp - command line client                                       *
 ************************************************************************/
#ifdef WINS
#include  <windows.h>
#else 
#include "winrepl.h"
#endif
#include <stdio.h>
#include <string.h>  // memset on Linux in wbkernel.h
// general client headers
#include "cdp.h"
#include "wbkernel.h"
// 602ccli headers
#include "options.h"
#include "wbcl.h"
#include "commands.h"
#pragma hdrstop
#include "wbapiex.h"

#ifdef WINS
typedef HWND WINAPI t_GSM(void);
#else
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <langinfo.h>
#include <wchar.h>
#include <iconv.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif

// Context variable of the client
#ifdef WINS
HANDLE hStdin, hStdout, hStderr;
#else
FILE * hStdin, * hStdout;
static char *history_file;
#endif
int max_lines = 0, lines_on_page = 0;
cdp_t cdp;
int sys_charset = CHARSET_NUM_UTF8;
bool connected = false;

#ifdef LINUX
// Processing of termination signals:
static void TermAbortProc(int sig)
{
  static volatile sig_atomic_t fatal_error_in_progress = 0;

  if (fatal_error_in_progress)
    raise (sig);

  fatal_error_in_progress=1;
  psignal(sig, "602ccli");
  cd_disconnect(cdp);
#ifdef LINUX
  if (libreadline_is_present)
    write_history(history_file);
#endif
  signal(sig, SIG_DFL);
  raise(sig);
}
#endif

cd_t my_cd; // Context variable of the client
const char * sys_charset_name = "iso8859-2";

void message_translation_callback(void * message)
// Translates the wide-char error message (in-place) to the national language.
{// convert the message to 8-bit (is in ASCII english):
  int len = (int)wcslen((const wchar_t*)message);
  char * ascii_message = (char*)corealloc(len+1, 59);
  if (!ascii_message) return;  // not translated on allocation error
  int i;
  for (i=0;  ((wchar_t*)message)[i];  i++)
    ascii_message[i] = (char)((wchar_t*)message)[i];
  ascii_message[i] = 0;
 // translate: 
  const char * trans = gettext(ascii_message);
 // convert the translation to wide:
#ifdef WINS
  superconv(ATT_STRING, ATT_STRING, trans, (char*)message, t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(0, 0, 0, 1), NULL);
#else
  superconv(ATT_STRING, ATT_STRING, trans, (char*)message, t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(0, 0, 0, 2), NULL);
#endif
  corefree(ascii_message);  // must deallocate this after using [trans] because ascii_message may be ==trans
}

static bool mask_pager = false;
bool cancel_long_output = false;

void output_message(const char * msg, int encoding, bool to_stderr)
// Outputs [msg] to stdout or to stderr. [msg] is translated. 
// If [encoding==1] it is in UTF-8, if [encoding==0] it is in the system encoding, if [encoding==2] the host encoding.
{ DWORD wr;
  if (pager && interactive_output && interactive && !to_stderr && !mask_pager)
    if (max_lines && lines_on_page>=max_lines)  // if window size defined and if it is full
    { if (cancel_long_output)
        return;
      mask_pager=true;
      output_message(_("Press <Enter> to continue the output or Q to cancel it..."), true, false);
      mask_pager=false;
#ifdef WINS
      SetConsoleMode(hStdin, 0);
      unsigned char c;  DWORD rd;
      do
      { ReadConsole(hStdin, &c, 1, &rd, NULL);
      } while (c!='q' && c!='Q' && c!='\n' && c!='\r');
      SetConsoleMode(hStdin, ENABLE_ECHO_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE);
      WriteConsoleW(hStdout, L"\n", 1, &wr, NULL);
#else
      unsigned char c;  DWORD rd;
      do
      { fread(&c, 1, 1, hStdin);
      } while (c!='q' && c!='Q' && c!='\n' && c!='\r');
      fwrite("\n", 1, 1, hStdout);
#endif
      if (c=='q' || c=='Q')
      { cancel_long_output=true;
        return;
      }
      else
        lines_on_page=0; 
    }
    else  // count lines
    { const char * p=msg;
      while (*p)
      { if (*p=='\n') lines_on_page++;
        p++;
      }
    }    
  cancel_long_output = false;  
  int len=(int)strlen(msg);
  char * buf = (char*)malloc((len+1)*sizeof(wchar_t));  // space for the translated message, may be wide
#ifdef WINS
  t_specif dest_spec;  
  if (interactive_output || to_stderr) 
    dest_spec = t_specif(0, 0, 0, 1);
  else
    dest_spec = t_specif(0, sys_charset, 0, 0);
  superconv(ATT_STRING, ATT_STRING, msg, buf, t_specif(0, encoding==1 ? CHARSET_NUM_UTF8 : encoding==0 ? sys_charset : GetHostCharSet(), 0, 0), dest_spec, NULL);
  if (to_stderr) 
    WriteConsoleW(hStderr, buf, (int)wcslen((wchar_t*)buf), &wr, NULL);
  else if (interactive_output)
    WriteConsoleW(hStdout, buf, (int)wcslen((wchar_t*)buf), &wr, NULL);
  else
    WriteFile(hStdout, buf, (int)strlen(buf), &wr, NULL);
#else
  iconv_t charset_sys_to_terminal = (iconv_t)-1;
  if (encoding!=2)
    charset_sys_to_terminal = iconv_open(nl_langinfo(CODESET), encoding==1 ? "UTF-8" : sys_charset_name);
  if (charset_sys_to_terminal != (iconv_t)-1)
  { size_t inleft=len+1, outleft=(len+1)*sizeof(wchar_t);
    const char * p1 = msg;  char * p2 = buf;
    size_t res = iconv(charset_sys_to_terminal, (char**)&p1, &inleft, &p2, &outleft);
    iconv_close(charset_sys_to_terminal);
  }
  else
    strcpy(buf, msg);
  fwrite(buf, strlen(buf), 1, to_stderr ? stderr : hStdout);
#endif  
  free(buf);
}

void newline(void)
{ output_message("\n", true, false); }

void CALLBACK log_client_error(const char * text)
{
  output_message(text, false, true);
  newline();
}

int main(int argc, char *argv[])
{
	cdp=&my_cd;

#ifdef LINUX
	signal(SIGTERM, TermAbortProc);
	signal(SIGABRT, TermAbortProc);
	signal(SIGINT, TermAbortProc);
  setlocale(LC_ALL, ""); // without this the locale-dependent strtod does not work, problems with conversion of strings to real numbers
  hStdin  = stdin;
  hStdout = stdout; 
  interactive = isatty(fileno(hStdin));
  interactive_output = isatty(fileno(hStdout));
  prepare_charset_conversions();
#else
  hStdin  = GetStdHandle(STD_INPUT_HANDLE); 
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
  hStderr = GetStdHandle(STD_ERROR_HANDLE); 
#if 0
  HINSTANCE hInst = LoadLibrary("kernel32.dll");
  t_GSM * p_GSM = (t_GSM*)GetProcAddress(hInst, "GetConsoleWindow");
  if (p_GSM)
    interactive = (*p_GSM)() != 0;
  else  
    interactive = true;  // default
  FreeLibrary(hInst);
#endif
  DWORD Mode;
  interactive        = GetConsoleMode(hStdin,  &Mode) != 0;
  interactive_output = GetConsoleMode(hStdout, &Mode) != 0;
 // get screen buffer size for paging:
  if (interactive_output)
  { CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
    if (GetConsoleScreenBufferInfo(hStdout, &ConsoleScreenBufferInfo))
    { max_lines=ConsoleScreenBufferInfo.dwSize.Y;
      if (max_lines>3) max_lines-=2;
    }  
  }
#endif

 // start the internationalization:
  { char path[256];  int i;
#ifdef LINUX
    strcpy(path, argv[0]);
    i=(int)strlen(path);
    while (i && path[i-1]!='/') i--;
    strcpy(path+i, "../share/locale");  // .. removes the bin directory from the [path]
#else
    GetModuleFileName(NULL, path, sizeof(path));
    i=(int)strlen(path);
    while (i && path[i-1]!='\\') i--;
    path[i]=0;
#endif    
    bindtextdomain(GETTEXT_DOMAIN, path);
    bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");  // will be recoded to host charset
    textdomain(GETTEXT_DOMAIN);
    char * trans = gettext("Continue?");  // test
  }
 // start error messages translation in the international version
  Set_translation_callback(NULL, &message_translation_callback);
 // client error callback:
  set_client_error_callback(log_client_error);

 // process these command-line options (some are used during the config-file processing):
  verbose=interactive;  // predefined, may be changed in command-line parameters
  processing_command_line=true;
  process_options_from_argv(argc, argv);
  if (terminate_after_processing_options) 
    return 0;
 // connect to the SQL server, if its name has been specified among the arguments:
  if (*ServerName && !do_connect())
    return 1;
 // set application, if its name has been specified:
  if (*SchemaName && !do_schema(SchemaName))  // returns error if not connected
    return 2; 
  processing_command_line=false;

 // process the config file(s) - connection and schema information are stored only:
  if (!disable_config_files)
  { processing_config_file=true;
    process_config_files();
    processing_config_file=false;
  }
#ifdef LINUX
  if (libreadline_is_present)
  { const char *home=getenv("HOME");
    asprintf(&history_file, "%s/.602ccli_history", home);
    read_history(history_file);
  }
#endif

 // batch processing:
  if (command_from_args_specified)
  { command_from_args=command_from_args+";\n"; // add the terminators for SQL/meta
    t_input_processor ip;
    ip.process_input(command_from_args.c_str());
  }
  else if (!input_filename.empty())  // specified on the command line
    process_command_file((char*)input_filename.c_str(), true);
  else  // interactive processing
  {// initial message:
    if (verbose)
    { output_message(_("602sql console client\n"), true, false);
      show_status();
      output_message(_("Enter an SQL statement terminated by a semicolon or a metastatement (enter \\? for the list)\n"), true, false);
    }  
   // process the input: 
    Do_SQL();  
    if (verbose) 
      output_message(_("\n602client terminated.\n"), true, false);
  }
 // termination:
  if (connected) 
  { cd_disconnect(cdp);
    connected=false;
  }
#ifdef LINUX
  if (libreadline_is_present)
    write_history(history_file);
#endif
  return 0;
}

void write_error_info(cdp_t cdp, const char * caller, int err)
{
  char buffer[500];
  strcpy(buffer, caller);  strcat(buffer, ": ");
  int pos = (int)strlen(buffer);
  Get_error_num_text(connected ? cdp : NULL, err, buffer+pos, sizeof(buffer)-pos-1);
  strcat(buffer, "\n");
  output_message(buffer, false, true);
}

void strmaxcpy(char * dest, const char * src, size_t size)
{ size_t len = strlen(src);
  if (len>=size)
  { if (!size) return;
    len=size-1;
  }
  memcpy(dest, src, len);
  dest[len]=0;
}

