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
#include "enumsrv.c"
#include "regstr.cpp"

#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#endif

#ifdef WINS
typedef HANDLE FHANDLE;   // file handle
#define INVALID_FHANDLE_VALUE INVALID_HANDLE_VALUE
#define CloseFile(x) CloseHandle(x)
#else
typedef int FHANDLE;   // file handle
#define INVALID_FHANDLE_VALUE (-1)
#define CloseFile(x) close(x)
#endif

char * next_param(char * & p)
// Parses p for the next parameter. Advances p after the end of it, but not beyond the terminatong 0.
// Returns pointer to the beginning of the parameter, its value is 0-terminated.
// Parameters are separated by 1 or more spaces, may be enclosed in "" ot '' (the quotes are removed then).
// Escape sequences are translated in quoted parameters.
// On exit, p points after the parameter value or to the terminating 0.
{ char * parstart, * parval;
  while (*p==' ' || *p=='\t') p++;
  if (!*p) return NULL;  // no more parameters
  parstart=parval=p;
  if (*p=='"' || *p=='\'')  // compressing the value: removing the enclosing quotes and double quotes inside
  { char terminator = *p;
    p++;
    while (*p)
    { if (*p==terminator)
      { p++;
        if (*p!=terminator) break;  // p point just after the terminator
      }
      else if (*p=='\\')
      { p++;
        if (*p=='n') *p='\n'; else
        if (*p=='r') *p='\r'; else
        if (*p=='t') *p='\t'; 
      } // other escaped characters like \, ', " represent itself
      *parval = *p;
      parval++;  p++;
    }
    *parval=0;
  }
  else
  { while (*p && *p!=' ' && *p!='\t') p++;
    if (*p) { *p=0;  p++; }  // must not advance p when *p==0
  }
  return parstart;
}

#ifdef WINS
char * getpass(char * prompt)
{ DWORD wr;
  output_message(prompt, true, false);
 // read the password: 
  SetConsoleMode(hStdin, 0);
  int pos=0;
  do
  { unsigned char c;
    ReadConsole(hStdin, &c, 1, &wr, NULL);
    if (c>=' ')
    { if (pos<MAX_FIL_PASSWORD)
      { Password[pos++]=c;
        c='*';
        WriteConsole(hStdout, &c, 1, &wr, NULL);
      } // more characters: ignored 
    }
    else if (c=='\n' || c=='\r' || !c || c==0xff)
      break;
    else if (c=='\b')
    { if (pos)
      { pos--;  
        WriteConsole(hStdout, "\b \b", 3, &wr, NULL);
      }  
    }
  } while (true);
  Password[pos]=0;  
  WriteConsole(hStdout, "\n", 1, &wr, NULL);
 // restore the normal input mode: 
  SetConsoleMode(hStdin, ENABLE_ECHO_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE);
  return Password;
}

#endif

bool do_schema(char * schema_name)
// Opens the schema specified by [schema_name]. Returns false if failed.
{
  if (!connected)
  {	output_message(_("Client is not connected to the SQL server.\n"), true, true);
    return false;
  }  
	if (cd_Set_application(cdp, schema_name))
	{ write_error_info(cdp, "Open schema", cd_Sz_error(cdp));
	  return false;
	}  
	strmaxcpy(SchemaName, schema_name, sizeof(SchemaName));
  return true;
}

void t_input_processor::handle_schema(char * params)
{
  char * sch = next_param(params);
  if (!sch|| next_param(params))
	{ output_message(_("Error in command parameters.\n"), true, true);
	  return;
	}  
  do_schema(sch);
  if (verbose && !processing_config_file)
    if (!*SchemaName)
      output_message(_("No schema is opened.\n"), true, false);
    else
    {	char buf[30+OBJ_NAME_LEN];
      sprintf(buf, _("Open schema: %s\n"), SchemaName);
      output_message(buf, true, false);
    }  
}

void t_input_processor::handle_status(char *)
{ 
  show_status();
}

void show_status(void)
{ char buf[100];
	if (!connected)
	  output_message(_("Client is not connected to the SQL server.\n"), true, false);
	else if (cd_is_dead_connection(cdp))
	  output_message(_("Connection the the SQL server has been lost.\n"), true, false);
  else
	{ uns32 v1=0, v2=0, v3=0, v4=0;
    cd_Get_server_info(cdp, OP_GI_VERSION_1, &v1, sizeof(v1));
    cd_Get_server_info(cdp, OP_GI_VERSION_2, &v2, sizeof(v2));
    cd_Get_server_info(cdp, OP_GI_VERSION_3, &v3, sizeof(v3));
    cd_Get_server_info(cdp, OP_GI_VERSION_4, &v4, sizeof(v4));
	  sprintf(buf, _("Connected to server: %s (version %u.%u.%u.%u, system charset %s).\n"), ServerName, v1, v2, v3, v4, sys_charset_name);
	  output_message(buf, true, false);
	  sprintf(buf, _("User is connected as %s.\n"), cd_Who_am_I(cdp));
      output_message(buf, true, false);
      if (!*SchemaName)
        output_message(_("No schema is opened.\n"), true, false);
      else
      {	sprintf(buf, _("Open schema: %s\n"), SchemaName);
        output_message(buf, true, false);
      }  
      if (cd_Am_I_security_admin(cdp)) 
        output_message(_("The user is the security administrator\n"), true, false);
      if (cd_Am_I_config_admin(cdp)) 
        output_message(_("The user is the configuration administrator.\n"), true, false);
      if (cd_Am_I_db_admin(cdp)) 
        output_message(_("The user is the data administrator.\n"), true, false);
      if (*SchemaName)
        if (cd_Am_I_appl_admin(cdp)) 
          output_message(_("The user is the administrator of the application.\n"), true, false);
	}
}

void t_input_processor::handle_connect(char * params)
{// parse the parameters:
  char * server, * user, * pass;
  server = next_param(params);
  user   = next_param(params);
  pass   = next_param(params);
  if (!server || next_param(params))
	{ output_message(_("Error in command parameters.\n"), true, true);
	  return;
	}
 // store the values:
  strmaxcpy(ServerName, server, sizeof(ServerName));
  strmaxcpy(LoginName,  user ? user : LoginName, sizeof(LoginName));   // default is ANONYMOUS, unless specified on the command line
  strmaxcpy(Password,   pass ? pass : Password,  sizeof(Password));    // default is empty password, unless specified on the command line
  { do_connect();
    *SchemaName=0;  // new connection clears the schema, but it must not be done in do_connect called in startup
    if (verbose && !processing_config_file) show_status();
  }
 // do not use the command-line username/password, nor the option specified in this statement:
  *LoginName = *Password = 0;
}

bool do_connect(void)
// Connect to the database as specified in ServerName, LoginName, Password. 
// Clear the values on connection error.
{ int res;
 // prompt for the password:
  if (!strcmp(Password, "?"))
    if (interactive)
    { char * pwd=getpass(_("Enter the password: "));
      if (!pwd)
      { output_message(_("Entering the password cancelled.\n"), true, true);
        goto failed;
      }
      strmaxcpy(Password, pwd, sizeof(Password));
    }
    else
    { output_message(_("Must not prompt for password in the batch mode.\n"), true, true);
      goto failed;
    }
 // connecting:     
  if (connected) 
    { cd_disconnect(cdp);  connected=false;  sys_charset = CHARSET_NUM_UTF8;  sys_charset_name = "UTF-8"; }
  res=cd_connect(cdp, ServerName, 1);
  if (res)
  { write_error_info(cdp, "connect", res);
    goto failed;
  }
  connected=true;  // used when reporting the Login errors
  if (cd_Login(cdp, LoginName, Password))
  { write_error_info(cdp, "Login", cd_Sz_error(cdp));
    cd_disconnect(cdp);  // connected==false;
    connected=false;
    goto failed;
  }
  cd_SQL_execute(cdp, "CALL _sqp_set_thread_name('602SQL Client Console')", NULL);
  cd_Set_progress_report_modulus(cdp, 0);  // disable progress reporting
 // read the system charset of the server:
  cd_Get_server_info(cdp, OP_GI_SYS_CHARSET, &sys_charset, sizeof(sys_charset));
  sys_charset_name =
    sys_charset==1   || sys_charset==2   ? "cp1250" :
    sys_charset>=3   && sys_charset<=5   ? "cp1252" :
    sys_charset==129 || sys_charset==130 ? "iso8859-2" :
    sys_charset>=131 && sys_charset<=133 ? "iso8859-1" : "UTF-8";
  return true;
 failed: 
  *ServerName=*LoginName=*Password=0;
  return false;
}

struct t_pack_addr
{ u_short sin_port;
  in_addr sin_addr;
  tobjname server_name;
  int is_private;  // 0 system, 1 private, 2 unregisterd
  bool running;
  bool local;
};

#define MAX_SRV_COUNT 100

struct t_addr_list
{ int count;
  t_pack_addr addr[MAX_SRV_COUNT];
  t_addr_list(void)
    { count=0; }
};  

static int add_running_server(const char *name, struct sockaddr_in* addr, void * data)
// Add servers from the network based on their addresses
{
 // test the uniqueness:
  t_addr_list * al = (t_addr_list*)data;
  for (int i =0;  i<al->count;  i++)
    if (!memcmp(&addr->sin_addr, &al->addr[i].sin_addr, sizeof(addr->sin_addr)) && 
        addr->sin_port==al->addr[i].sin_port)
    { al->addr[i].running = true;   
      strcpy(al->addr[i].server_name, name);  // may overwrite the local name
      return 1; 
    }  
 // add to the list:      
  if (al->count < MAX_SRV_COUNT)
  { t_pack_addr * a = al->addr+al->count;
    memcpy(&a->sin_addr, &addr->sin_addr, sizeof(addr->sin_addr));
    a->sin_port = addr->sin_port;
    a->is_private=2;
    a->running=true;
    a->local=false;
    strcpy(a->server_name, name);
    al->count++;
  }
  return 1;
}

void show_servers(char *)  
{ t_addr_list al;
  if (verbose)
    output_message(_("Available servers:\n"), true, false);
 // insert registered servers:
  tobjname a_server_name;  
  t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
  while (es.next(a_server_name))
  { char buf[100+1];  unsigned long val;  char a_path[MAX_PATH];
    es.GetPrimaryPath(a_server_name, a_path);
    if (!es.GetDatabaseString(a_server_name, IP_str, buf, sizeof(buf))) *buf=0;
    if (!es.GetDatabaseString(a_server_name, IP_socket_str, &val, sizeof(val))) val=DEFAULT_IP_SOCKET;
   // adding the server:
    if (al.count < MAX_SRV_COUNT)
    { t_pack_addr * a = al.addr+al.count;
      strcpy(a->server_name, a_server_name);
      a->local = *a_path != 0;
      a->is_private = es.is_private_server() ? 1 : 0;
      if (a->local)
      { int state = srv_Get_state_local(a_server_name);
        a->running = state==1 || state==2;
      }
      else a->running=false;  // may be changed later
      *(uns32*)&a->sin_addr = inet_addr(buf);
      a->sin_port = htons((u_short)val);
      al.count++;
    }
  }
 // insert network servers:
  enum_servers(200, add_running_server, &al);
 // print the list:
  char buf[60+OBJ_NAME_LEN];  const char *dotsname;
  if (!tuples_only)
  { sprintf(buf, "%15s:%-7s %-7s %-11s %-14s %s\n", _("IP Address"), _("port"), _("Local?"), _("Running?"), _("Registration"), _("Server name"));
    output_message(buf, true, false);
  }
  for (int i=0;  i<al.count;  i++)
  { t_pack_addr * a = al.addr+i;
    dotsname=
#ifdef WINS
      a->sin_addr.S_un.S_addr
#else
      a->sin_addr.s_addr!=INADDR_NONE
#endif
        ? inet_ntoa(a->sin_addr) : "localhost";
    sprintf(buf, "%15s:%-7d %-7s %-11s %-14s %s\n", dotsname, ntohs(a->sin_port), 
      a->local ? "Local" : "Network",  // may be Local event if IP address is different from "localhost"
      a->running ? "Running" : "Not running",
      a->is_private==0 ? "System" : a->is_private==1 ? "User" : "Not registered",
      a->server_name);
    output_message(buf, true, false);
  }  
}

void t_input_processor::handle_servers(char *cmd)
{
  show_servers(cmd);
}
  
void t_input_processor::handle_help(char *topic)
{
  if (*topic=='\\') topic++;  // metastatement name entered with the leading '\', drop it
 // show help for metastatements with the given prefix: 
  for (struct command_t *cmd=commands_list; cmd->name;  cmd++)
	  if (cmd->help)
	    if (!*topic || !strnicmp(cmd->name, topic, strlen(topic)))
	    { output_message(gettext(cmd->help), true, false);
	      newline();
      }
}

void t_input_processor::handle_quit(char * params)
{
  if (next_param(params))
	  output_message(_("Error in command parameters.\n"), true, true);
	else  
    input_terminated=true;
}

void t_input_processor::handle_echo(char * params)
{ const char * param;  bool append_newline = true;
  if (!memcmp(params, "-n ", 3))
  { params+=3;
    append_newline=false;
  }
  do
  { param = next_param(params);
    if (!param) break;
    output_message(param, true, false);
  } while (true);
  if (append_newline) newline();
}

void set_output_file(char * fname)
{ 
#ifdef WINS
  DWORD Mode;
 // close the current output file, unless it is the std output
  if (hStdout != GetStdHandle(STD_OUTPUT_HANDLE))
    CloseHandle(hStdout);
 // open the new file:
  if (!fname || !*fname)
  { hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    interactive_output = GetConsoleMode(hStdout, &Mode) != 0;
  }  
  else
  { HANDLE hF = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hF==INVALID_HANDLE_VALUE)
    { perror(fname);
      hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
      interactive_output = GetConsoleMode(hStdout, &Mode) != 0;
    }  
    else
    { hStdout = hF;
      interactive_output=false;
    }  
  }
#else
 // close the current output file, unless it is the std output
  if (hStdout != stdout)
    fclose(hStdout);
 // open the new file:
  if (!fname || !*fname)
  { perror(fname);
    hStdout = stdout;
    interactive_output = isatty(fileno(hStdout));
  }  
  else
  { FILE * hF = fopen(fname, "w");
    if (!hF)
    { hStdout = stdout;
      interactive_output = isatty(fileno(hStdout));
    }  
    else
    { hStdout = hF;
      interactive_output=false;
    }  
  }
#endif
}

void t_input_processor::handle_command_file(char * params)
{ 
  char * fname = next_param(params);
  if (!fname || next_param(params))
	{ output_message(_("Error in command parameters.\n"), true, true);
	  return;
	}  
  process_command_file(fname, true);
}

void t_input_processor::handle_output_file(char * params)
{ 
  char * fname = next_param(params);
  if (next_param(params))
	{ output_message(_("Error in command parameters.\n"), true, true);
	  return;
	}  
  set_output_file(fname);
}

void t_input_processor::handle_pset(char * params)
{
  char * name = next_param(params);
  char * value = next_param(params);
  if (!name || next_param(params))
	{ output_message(_("Error in command parameters.\n"), true, true);
	  return;
	}  
	process_assignment(name, value, verbose);
}

void t_input_processor::handle_aligned(char * params)
{ 
  if (next_param(params))
	  output_message(_("Error in command parameters.\n"), true, true);
  else  
    process_assignment("format", disp_aligned ? "Unaligned" : "Aligned", verbose);
}

void t_input_processor::handle_fieldsep(char * sep)
{ 
  char * convsep = next_param(sep);  // convsep may be NULL (set the default), empty string or non-empty string
  process_assignment("fieldsep", convsep, verbose);
}

void t_input_processor::handle_recordsep(char * sep)
{ 
  char * convsep = next_param(sep);  // convsep may be NULL (set the default), empty string or non-empty string
  process_assignment("recordsep", convsep, verbose);
}

void t_input_processor::handle_expanded(char * params)
{
  if (next_param(params))
	  output_message(_("Error in command parameters.\n"), true, true);
  else
    process_assignment("expanded", NULL, verbose);
}

void t_input_processor::handle_tuples(char * params)
{
  if (next_param(params))
	  output_message(_("Error in command parameters.\n"), true, true);
  else
    process_assignment("tuples_only", NULL, verbose);
}

void t_input_processor::handle_print(char * params)
{
  if (next_param(params))
	  output_message(_("Error in command parameters.\n"), true, true);
	else if (!continuing_sql())
	  output_message(_("The SQL statement buffer is empty.\n"), true, true);
  else  
  { output_message(sql_statement.c_str(), true, false);
    newline();
  }  
}

void t_input_processor::handle_reset(char * params)
{
  if (next_param(params))
	  output_message(_("Error in command parameters.\n"), true, true);
	else if (!continuing_sql())
	  output_message(_("The SQL statement buffer is empty.\n"), true, true);
  else  
  { sql_statement="";
    if (verbose)
  	  output_message(_("The SQL statement buffer cleared.\n"), true, false);
  }	  
}

void t_input_processor::handle_go(char * params)
{
  if (next_param(params))
	  output_message(_("Error in command parameters.\n"), true, true);
	else if (!continuing_sql())
	  output_message(_("The SQL statement buffer is empty.\n"), true, true);\
  else  
    process_sql_statement();
}

void t_input_processor::handle_edit(char * params)
{ char filename[MAX_PATH];  bool temp_file = false;
  char * fname = next_param(params);  
  if (next_param(params))
	  output_message(_("Error in command parameters.\n"), true, true);
  else  
  {// create a temporary file, if the <filename> parameter has not been specified:
    if (!fname)
    { GetTempPath(MAX_PATH, filename);
      GetTempFileName(filename, "sql", 0, filename);
      fname=filename;
      temp_file=true;
     // save the current sql buffer to the file: 
      if (sql_statement.length() > 0)
      { DWORD wr;
        FHANDLE hFile = CreateFile(fname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile==INVALID_FHANDLE_VALUE)
          { perror(fname);  return; }
        WriteFile(hFile, sql_statement.c_str(), (DWORD)sql_statement.length(), &wr, NULL);  
        CloseFile(hFile);
      }  
    }
   // run the editor:
    const char * editor_name=getenv("SQL_EDITOR");
    if (!editor_name) editor_name=getenv("EDITOR");
#ifdef WINS
    if (!editor_name) editor_name="notepad.exe";
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof STARTUPINFO);
    memset(&pi, 0, sizeof PROCESS_INFORMATION);
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags |= STARTF_USESHOWWINDOW;  si.wShowWindow = SW_SHOWNORMAL; 
    char cmd[2*MAX_PATH+10];
    sprintf(cmd, "%s %s", editor_name, fname);
    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_PROCESS_GROUP|NORMAL_PRIORITY_CLASS,
                      NULL, NULL, &si, &pi)) 
      { perror("create process");  return; }                
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
#else
    if (!editor_name) editor_name="vi";
    int pid=fork();
    if (pid==-1)
      output_message(_("fork error"), true, true);
    else if (pid==0)  // the child
      execlp(editor_name, editor_name, fname, NULL);
    else   // the parent
      waitpid(pid, NULL, 0);
#endif  
   // open the file and copy it to the buffer:
    DWORD rd;  char buf[128+1];
    FHANDLE hFile = CreateFile(fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile==INVALID_FHANDLE_VALUE)
      { perror(fname);  return; }
    long length = SetFilePointer(hFile, 0, NULL, FILE_END);
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    sql_statement="";
    do
    { ReadFile(hFile, buf, length<128 ? length : 128, &rd, NULL);
      buf[rd]=0;
      sql_statement=sql_statement+buf;
    } while (rd);  
    CloseFile(hFile);
   // process the buffer (processing everything if ; is contained anywhere): 
    bool has_semicolon = false;
    for (size_t i = 0;  i<sql_statement.length();  i++)
      if (sql_statement[i]==';')
        { has_semicolon=true;  break; }
    if (has_semicolon)    
      process_sql_statement();
   // delete the temporary file:
    if (temp_file)
      DeleteFile(fname);
  }
}  

void displ_objects(tcateg categ)
{ char * buffer = NULL;
  if (cd_List_objects(cdp, categ, NULL, &buffer))
    write_error_info(cdp, "List objects", cd_Sz_error(cdp));
  else
  { for (tptr src=buffer;  *src;  src+=strlen(src)+1+sizeof(tobjnum)+sizeof(uns16))
    { output_message(src, false, false);  
      newline();
    }  
    corefree(buffer);
  }  
}

void t_input_processor::handle_displ_tables(char * params)
{ 
  if (!connected)
   	output_message(_("Client is not connected to the SQL server.\n"), true, true);
  else
  { output_message(_("List of tables:\n"), true, false);
    displ_objects(CATEG_TABLE); 
  }
}

void t_input_processor::handle_displ_queries(char * params)
{ 
  if (!connected)
   	output_message(_("Client is not connected to the SQL server.\n"), true, true);
  else
  { output_message(_("List of stored views:\n"), true, false);
    displ_objects(CATEG_CURSOR); 
  }
}

void t_input_processor::handle_displ_schemas(char * params)
{ 
  if (!connected)
   	output_message(_("Client is not connected to the SQL server.\n"), true, true);
  else
  { output_message(_("List of schemas:\n"), true, false);
    displ_objects(CATEG_APPL); 
  }
}

void t_input_processor::handle_displ_groups(char * params)
{ 
  if (!connected)
   	output_message(_("Client is not connected to the SQL server.\n"), true, true);
  else
  { output_message(_("List of groups:\n"), true, false);
    displ_objects(CATEG_GROUP); 
  }
}

void t_input_processor::handle_displ_users(char * params)
{ 
  if (!connected)
   	output_message(_("Client is not connected to the SQL server.\n"), true, true);
  else
  { output_message(_("List of users:\n"), true, false);
    displ_objects(CATEG_USER); 
  }
}


void WINAPI Progress(int Categ, const void *Value, void *Param)
{
  switch (Categ)
  {
    case IMPEXP_FILENAME:
      if (!verbose) break;
    case IMPEXP_ERROR:
    case IMPEXP_STATUS:
    case IMPEXP_FINAL:
      output_message((char*)Value, 2, false); 
      newline();
      break;
  }      
}

void t_input_processor::handle_import(char * params)
{ 
  if (!connected)
   	output_message(_("Client is not connected to the SQL server.\n"), true, true);
  else
  { const char * fname = next_param(params);
    if (!fname)
	    output_message(_("Error in command parameters.\n"), true, true);
    else
    {// write the file name, errors in \\ are common:
      if (verbose)
      { char msg[MAX_PATH+100];
        snprintf(msg, sizeof(msg), _("Importing application from file: %s.\n"), fname);  msg[sizeof(msg)-1]=0;
        output_message(msg, true, false);
      }  
     // import:
      t_import_param ip;
      memset(&ip, 0, sizeof(t_import_param));
      ip.cbSize = sizeof(t_import_param);
      ip.file_name = fname;
      ip.report_progress=true;
      ip.callback=Progress;
      ip.param=NULL;
      ip.flags=0;
      do
      { const char * aparam = next_param(params);
        if (!aparam) break;
        if (!stricmp(aparam, "REPLACE")) ip.flags = IMPAPPL_REPLACE;
        else 
        { ip.alternate_name = aparam;
          ip.flags = IMPAPPL_NEW_INST|IMPAPPL_USE_ALT_NAME;
        }  
      } while (true);
      Import_appl_param(cdp, &ip);
     // find the current application name:
      WBUUID appl_uuid;  tobjname server_name;
      cd_Get_appl_info(cdp, appl_uuid, SchemaName, server_name);
    }
  }
}

void t_input_processor::handle_export(char * params)
{ 
  if (!connected)
   	output_message(_("Client is not connected to the SQL server.\n"), true, true);
  else if (!*SchemaName)
   	output_message(_("No schema is open.\n"), true, true);
  else
  { const char * fname = next_param(params);
    if (!fname)
	    output_message(_("Error in command parameters.\n"), true, true);
    else
    {// write the file name, errors in \\ are common:
      if (verbose)
      { char msg[MAX_PATH+100];
        snprintf(msg, sizeof(msg), _("Creating application file: %s.\n"), fname);  msg[sizeof(msg)-1]=0;
        output_message(msg, true, false);
      }  
     // export:
      t_export_param ip;
      memset(&ip, 0, sizeof(t_export_param));
      ip.cbSize = sizeof(t_export_param);
      ip.file_name = fname;
      ip.report_progress=true;
      ip.long_names=true;
      ip.overwrite=true;
      ip.callback=Progress;
      ip.param=NULL;
      do
      { const char * aparam = next_param(params);
        if (!aparam) break;
        if (!strnicmp(aparam, "DATE_LIMIT=", 11))
        { if (superconv(ATT_STRING, ATT_TIMESTAMP, aparam+11, (char*)&ip.date_limit, t_specif(), t_specif(), NULL) != 0)
     	      { output_message(_("Error in DATE_LIMIT parameter.\n"), true, true);  return; }
        }
        else if (!stricmp(aparam, "LOCKED"))            ip.exp_locked=true;
        else if (!stricmp(aparam, "WITH_DATA"))         ip.with_data=true;
        else if (!stricmp(aparam, "WITH_ROLE_PRIVILS")) ip.with_role_privils=true;
        else if (!stricmp(aparam, "WITH_USERGRP"))      ip.with_usergrp=true;
        else if (!stricmp(aparam, "BACK_END"))          ip.back_end=true;
        else 
   	      { output_message(_("Error in command parameters.\n"), true, true);  return; }
      } while (true);
      Export_appl_param(cdp, &ip);
    }
  }
}

/* Table of built-in commands */
struct command_t commands_list[]= {
	{"help",    &t_input_processor::handle_help,            gettext_noop("\\help [<command>] --- show help") },
	{"?",       &t_input_processor::handle_help,            NULL },
	{"connect", &t_input_processor::handle_connect,         gettext_noop("\\connect <server> [<username> [<password>]] --- connect and login to the server") },
	{"list",    &t_input_processor::handle_servers,         gettext_noop("\\list --- list available servers") },
	{"status",  &t_input_processor::handle_status,          gettext_noop("\\status --- show connection status and schema") },
	{"schema",  &t_input_processor::handle_schema,          gettext_noop("\\schema <schema_name> --- open the schema") },
	{"q",    &t_input_processor::handle_quit,               gettext_noop("\\q --- stop the client") },
	{"echo", &t_input_processor::handle_echo,               gettext_noop("\\echo text --- print text to the output") },
	{"i",    &t_input_processor::handle_command_file,       gettext_noop("\\i <filename> --- process command file") },
	{"o",    &t_input_processor::handle_output_file,        gettext_noop("\\o [<filename>] --- use output file") },
	
	{"pset",  &t_input_processor::handle_pset,              gettext_noop("\\pset <parameter> [<value>] --- set a formatting option\n"
	                                                        "  \\pset format Aligned|Unaligned --  selects aligned or CSV output\n"
	                                                        "  \\pset border 0|1|2 --- defines border of the aligned output\n"
	                                                        "  \\pset expanded --- toggels the expanded formatting mode\n"
	                                                        "  \\pset pager --- toggels the pager\n"
	                                                        "  \\pset null <text> --- sets the representation of the NULL value\n"
	                                                        "  \\pset fieldsep <...> --- defines the field separator for the unaligned output\n"
	                                                        "  \\pset recordsep <...> --- defines the record separator for the unaligned output\n"
	                                                        "  \\pset tuples_only -- toggles the output of data without column headers\n"
	                                                        "  \\pset {time|date|timestamp|real} <pattern> -- defines the display format") },
	{"a",     &t_input_processor::handle_aligned,           gettext_noop("\\a --- toggle aligned output format") },
	{"f",     &t_input_processor::handle_fieldsep,          gettext_noop("\\f --- set the field separator for unaligned output") },
	{"t",     &t_input_processor::handle_tuples,            gettext_noop("\\t --- toggle the display of column names and record count") },
	{"x",     &t_input_processor::handle_expanded,          gettext_noop("\\x --- toggle the expanded formatting mode") },

  {"r",     &t_input_processor::handle_reset,             gettext_noop("\\r --- reset the query buffer") },
  {"p",     &t_input_processor::handle_print,             gettext_noop("\\p --- print the query buffer") },
  {"g",     &t_input_processor::handle_go,                gettext_noop("\\g --- send the query to the server") },
	
  {"edit",  &t_input_processor::handle_edit,              gettext_noop("\\edit [<filename>] --- open external editor and edit an sql statement") },
  {"e",     &t_input_processor::handle_edit,              NULL },

  {"dt",    &t_input_processor::handle_displ_tables,      gettext_noop("\\dt --- list tables in the open schema") },
  {"dq",    &t_input_processor::handle_displ_queries,     gettext_noop("\\dq --- list queries in the open schema") },
  {"ds",    &t_input_processor::handle_displ_schemas,     gettext_noop("\\ds --- list schemata") },
  {"du",    &t_input_processor::handle_displ_users,       gettext_noop("\\du --- list users") },
  {"dg",    &t_input_processor::handle_displ_groups,      gettext_noop("\\dg --- list user groups") },

  {"import",&t_input_processor::handle_import,            gettext_noop("\\import <file> [REPLACE | <alternative_name>] --- import application") },
  {"export",&t_input_processor::handle_export,            gettext_noop("\\export <file> [LOCKED] [WITH_DATA] [WITH_USERGRP] [WITH_ROLE_PRIVILS] [DATE_LIMIT=<timestamp>] --- export application") },
	{NULL, NULL, NULL}
};

