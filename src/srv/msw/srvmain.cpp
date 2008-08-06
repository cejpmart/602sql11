// srvmain.cpp - the main source for the SQL server running as task
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
#include "server.h"
#include "netapi.h"
#include <windowsx.h>
#include "nlmtexts.h"

static BOOL networking; // networking request at the server start, not the actual state

int main(int argc, char * argv[])
{
  HANDLE hStdin  = GetStdHandle(STD_INPUT_HANDLE); 
  HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
 // preare the gettext domain:
  { char path[MAX_PATH];  int i;
    GetModuleFileName(NULL, path, sizeof(path));
    i=strlen(path);
    while (i && path[i-1]!=PATH_SEPARATOR) i--;
    path[i]=0;
    if (*path) bindtextdomain(GETTEXT_DOMAIN, path);
    bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
    textdomain(GETTEXT_DOMAIN);
    char * trans = gettext("Continue?");
  }
  cd_t my_cd;  my_cdp=&my_cd;
 // analyse the command line:
  BOOL restore_db_file = FALSE;
  kernelpath=NULL;  networking=FALSE;
  is_dependent_server=false;
 // parse the command line ("s are removed):
  for (int i=1;  i<argc;  i++)
  { char p = * argv[i];
    if (*p=='-' || *p=='/' || *p=='&')  // parameter start
    { if (*p!='&') p++;
      switch (*p)
      { case 'N':
          if (p[1]<=' ') networking=TRUE;  break;
        case 'Q':  case 'q':
          if (p[1]<=' ') { disable_system_triggers=TRUE;  minimal_start=true; }  break;
        case '!':
          if (p[1]<=' ') is_dependent_server=true;  break;
        default:
          if (!_memicmp(p+1, "Berle", 5)) restore_db_file=TRUE;  break;
        case 's':  case 'n':  case '&':
        { p++;
          strmaxcpy(loc_server_name, p);
         // read the server name:
            while (*p>' ')
            { if (i<OBJ_NAME_LEN) loc_server_name[i++]=*p;
              p++;
            }
          loc_server_name[i]=0;
          kernelpath=loc_server_name;
          p--;  // preare to skip
          break;    
        }
      }
    }
    
   if (restore_db_file)
   { err=restore_database_file(kernelpath);
     return err;
   }  
   else  // start server
   { 
     if (!GetDatabaseString(loc_server_name, t_server_profile::http_tunnel_port.name, (char*)&http_tunnel_port_num, sizeof(http_tunnel_port_num), my_private_server)) 
       http_tunnel_port_num=80;
     if (!GetDatabaseString(loc_server_name, t_server_profile::web_port        .name, (char*)&http_web_port_num,    sizeof(http_web_port_num),    my_private_server)) 
       http_web_port_num=80;
     err=kernel_init(&my_cd, kernelpath, networking!=0);
     if (err!=KSE_OK)
     { if (is_dependent_server) report_error_to_client(kernelpath, err); // kernelpath is defined from the beginning
      // show error in the console
        char msg[100];
        form_terminal_message(msg, sizeof(msg)-1, kse_errors[0]);
        strcat(msg, "\n");
        WriteConsole(hStdout, msg, strlen(msg), &wr, NULL);
        form_terminal_message(msg, sizeof(msg)-1, kse_errors[err]);
        strcat(msg, "\n");
        WriteConsole(hStdout, msg, strlen(msg), &wr, NULL);
     } 
   }

 onerr:
  if (err)
  { if (is_dependent_server) report_error_to_client(kernelpath, err); // kernelpath is defined from the beginning
    if (err!=KSE_NO_SERVER_LICENCE)
      if (!is_dependent_server /*|| err==KSE_BAD_PASSWORD*/) // display error box - unless it will be displayed by the client
        Kernel_error_box(hwMain, err);
    if (hwMain) DestroyWindow(hwMain);
    return 1;
  }

  { ffa.log_client_restart(); // write restart info to the client communication log
    start_thread_for_periodical_actions();
   // message cycle:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
      if (!hInfoWnd || !IsDialogMessage(hInfoWnd, &msg))
      { TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
   // stop backup timer:
    
    
}