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

#ifdef LINUX
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <wordexp.h>
#include <assert.h>
#endif

bool processing_config_file, processing_command_line;
// global variables :
bool interactive;    // stdin from terminal?
bool interactive_output;
bool verbose;        // verbose mode
bool disable_config_files = false;
// Default values of login parameters
tobjname ServerName = "";  // the default
tobjname LoginName = "";   // the default 
char Password[MAX_PASSWORD_LEN+1] = "";
tobjname SchemaName = "";
// display options:
bool disp_aligned = true;
std::string field_separator = "|",
            record_separator = "\n",
            disp_null = "",
            date_format = "", time_format = "", timestamp_format = "", real_format = "";
bool tuples_only = false;            
bool expanded = false;
bool pager = true;
int disp_border = 1;
// redirection options:
#include <string>
std::string command_from_args="", input_filename="", output_filename="";
bool command_from_args_specified = false;

void process_config_files(void)
// [processing_config_file] is true.
{ char fname[MAX_PATH];
#ifdef WINS
  HKEY hKey;  
 // system config file: 
  *fname=0;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ, &hKey)==ERROR_SUCCESS)
  { DWORD key_len=sizeof(fname);
    if (RegQueryValueExA(hKey, "Common AppData", NULL, NULL, (BYTE*)fname, &key_len)==ERROR_SUCCESS)
      strcat(fname, "\\Software602\\602SQL");
    RegCloseKey(hKey);
  }
  strcat(fname, "\\602ccli");
  process_command_file(fname, false);
 // user config file:
  *fname=0;
  if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ, &hKey)==ERROR_SUCCESS)
  { DWORD key_len=sizeof(fname);
    if (RegQueryValueExA(hKey, "AppData", NULL, NULL, (BYTE*)fname, &key_len)==ERROR_SUCCESS)
      strcat(fname, "\\Software602\\602SQL");
    RegCloseKey(hKey);
  }
  strcat(fname, "\\602ccli");
  process_command_file(fname, false);
#else
 // system config file: 
  process_command_file("/etc/602ccli", false);
 // user config file:
  const char * home=getenv("HOME");
  sprintf(fname, "%s/.602ccli", home);
  process_command_file(fname, false);
#endif
}

void output_string(const char * str)
{ char term;  const char * p;
  while (*str)
  {// find the string of normal characters:
    p=str;
    while (*(unsigned char *)p >= ' ') p++;
   // print the normal characters: 
    term = *p;
    *(char*)p = 0;
    if (*str) output_message(str, true, false);
    *(char*)p = term;
   // stop, if the end reached:
    if (!term) break;
   // print the special character: 
    if      (term=='\n') output_message("<newline>", true, false);
    else if (term=='\t') output_message("<tab>", true, false);
    else if (term=='\r') output_message("<CR>", true, false);
    else                 output_message("<?>", true, false);
   // go to the next part:
    str=p+1;
  } 
}

bool process_assignment(char * name, const char * value, bool feedback)
// [value] may be NULL when called from a metastatement
// If [feedback] then report values and errors.
{
 // find the name:
  if (!strcmp(name, "format"))
  { if (!value) goto err;
    const char * xname = "Unaligned";
    if (!strnicmp(xname, value, strlen(value))) disp_aligned=false;
    else 
    { xname = "Aligned";
      if (!strnicmp(xname, value, strlen(value))) disp_aligned=true;
      else goto err;
    }  
    if (feedback) 
      output_message(disp_aligned ? _("Format is Aligned.\n") : _("Format is Unaligned.\n"), true, false);
  }
  else if (!strcmp(name, "border"))
  { if (!value) goto err;
    disp_border = atoi(value);
    if (feedback) 
    { char msg[50];
      sprintf(msg, _("Border is %d.\n"), disp_border);
      output_message(msg, true, false);
    }  
  }  
  else if (!strcmp(name, "expanded") || !strcmp(name, "x"))
  { if (!value || !*value) expanded=!expanded;
    else if (!strcmp(value, "0")) expanded=false;
    else if (!strcmp(value, "1")) expanded=true;
    else goto err;
    if (feedback) 
      output_message(expanded ? _("Expanded display is On.\n") : _("Expanded display is Off.\n"), true, false);
  }  
  else if (!strcmp(name, "pager"))
  { if (!value || !*value) pager=!pager;
    else if (!strcmp(value, "0")) pager=false;
    else if (!strcmp(value, "1")) pager=true;
    else goto err;
    if (feedback) 
      output_message(pager ? _("Pager is On.\n") : _("Pager is Off.\n"), true, false);
  }  
  else if (!strcmp(name, "null"))
  { if (value) disp_null.assign(value); else disp_null.clear();
    if (feedback) 
    { output_message(_("NULL representation is:"), true, false);
      output_message(disp_null.c_str(), true, false);
      newline();
    }
  }
  else if (!strcmp(name, "fieldsep"))
  { if (value) field_separator.assign(value); else field_separator.clear();
    if (feedback) 
    { output_message(_("Field separator is:"), true, false);
      output_string(field_separator.c_str());
      newline();
    }
  }
  else if (!strcmp(name, "recordsep"))
  { if (value) record_separator.assign(value); else record_separator.clear();
    if (feedback) 
    { output_message(_("Record separator is:"), true, false);
      output_string(record_separator.c_str());
      newline();
    }
  }
  else if (!strcmp(name, "tuples_only") || !strcmp(name, "t"))
  { if (!value || !*value) tuples_only=!tuples_only;
    else if (!strcmp(value, "0")) tuples_only=false;
    else if (!strcmp(value, "1")) tuples_only=true;
    else goto err;
    if (feedback) 
      output_message(tuples_only ? _("Tuples_only is On.\n") : _("Tuples_only is Off.\n"), true, false);
  }  
  else if (!strcmp(name, "date"))
  { if (value) date_format.assign(value); else date_format.clear();
    if (feedback) 
    { output_message(_("Date format is: "), true, false);
      output_message(date_format.c_str(), true, false);
      newline();
    }
  }
  else if (!strcmp(name, "time"))
  { if (value) time_format.assign(value); else time_format.clear();
    if (feedback) 
    { output_message(_("Time format is: "), true, false);
      output_message(time_format.c_str(), true, false);
      newline();
    }
  }
  else if (!strcmp(name, "timestamp"))
  { if (value) timestamp_format.assign(value); else timestamp_format.clear();
    if (feedback) 
    { output_message(_("Timestamp format is: "), true, false);
      output_message(timestamp_format.c_str(), true, false);
      newline();
    }
  }
  else if (!strcmp(name, "real"))
  { if (value) real_format.assign(value); else real_format.clear();
    if (feedback) 
    { output_message(_("Real value format is: "), true, false);
      output_message(real_format.c_str(), true, false);
      newline();
    }
  }
  else goto err;  // name not found
  return true;
 err:
  if (feedback) output_message(_("Error in parameter value.\n"), true, true);
  return false;
}

#ifdef WINS
const char * optarg = NULL;

struct option
{ const char *name;
  int has_arg;
  int *flag;
  int val;
};

static int curr_arg=1, curr_arg_pos=0;  // start analysing from here

int getopt_long(int argc, char **argv, const char * options, const struct option * longopts, int * longindex)
{ 
  if (curr_arg>=argc) return -1;  // no more parameters
  if (curr_arg_pos >= (int)strlen(argv[curr_arg]))
  { curr_arg++;  curr_arg_pos=0; 
    if (curr_arg>=argc) return -1;  // no more parameters
  }
  if (!curr_arg_pos)  // start of a new option
  { if (*argv[curr_arg] != '-' && *argv[curr_arg] != '/')
      { curr_arg++;  return '?'; }  // argument is not an option
    curr_arg_pos=1;  
  }  
  if (argv[curr_arg][curr_arg_pos]=='-')  // long option starts here
  { curr_arg_pos++;  // point to the beginning of the option's name
    size_t len=0;
    while (argv[curr_arg][curr_arg_pos+len] && argv[curr_arg][curr_arg_pos+len]!='=') len++;
   // search it (unique prefix is sufficient):
    int found_cnt=0, found_ind;
    for (int lo=0;  longopts[lo].name;  lo++)
      if (len<=strlen(longopts[lo].name) &&
          !memcmp(longopts[lo].name, argv[curr_arg]+curr_arg_pos, len))
        { found_ind=lo;  found_cnt++; }  
    if (found_cnt!=1)
      { curr_arg++;  return '?'; }  // unrecognized option name (skipping the rest of the argument)
    if (longindex) *longindex=found_ind;
   // prepare the return value:
    int retval;
    if  (longopts[found_ind].flag) 
    { *longopts[found_ind].flag = longopts[found_ind].val;
      retval=0;
    }
    else 
      retval=longopts[found_ind].val;
   // look for the option:
    curr_arg++;
    switch (longopts[found_ind].has_arg)
    { case 0:  // no argument, ignoring if = specified
        optarg=NULL;  break;
      case 1:  // required argument
        if (argv[curr_arg-1][curr_arg_pos+len]=='=')  // internal argument
          optarg=argv[curr_arg-1]+curr_arg_pos+len+1;
        else if (curr_arg>=argc || *argv[curr_arg]=='-' || *argv[curr_arg]=='/')  //  no argument
        { optarg=NULL;  curr_arg_pos=0;  return '?'; }  // missing
        else  // parameter is specified in the next argument
          optarg=argv[curr_arg++];
        break;  
      case 2:  // optional argument
        if (argv[curr_arg-1][curr_arg_pos+len]=='=')  // internal argument
          optarg=argv[curr_arg-1]+curr_arg_pos+len+1;  
        else if (curr_arg>=argc || *argv[curr_arg]=='-' || *argv[curr_arg]=='/')  //  no argument
          optarg=NULL;  
        else  // parameter is specified in the next argument
          optarg=argv[curr_arg++];
        break;  
    }
    curr_arg_pos=0;
    return retval;
  }
  else  // short option
  {// search the current option:
    const char * pos = strchr(options, argv[curr_arg][curr_arg_pos]);
    if (!pos)
      { curr_arg++;  return '?'; }  // unrecognized option letter (skipping the whole argument)
    curr_arg_pos++;
    if (pos[1]==':')  // processing parameters
      if (pos[2]==':')  // optional parameter
      { if (argv[curr_arg][curr_arg_pos])  // the parameter is specified
          optarg=argv[curr_arg]+curr_arg_pos;
        else
          optarg=NULL;
        curr_arg++;  curr_arg_pos=0;
      }
      else  // required parameter
      { curr_arg++;
        if (argv[curr_arg-1][curr_arg_pos])  // the parameter is specified locally
          optarg=argv[curr_arg-1]+curr_arg_pos;
        else if (curr_arg>=argc || *argv[curr_arg]=='-' || *argv[curr_arg]=='/')
        { optarg=NULL;  curr_arg_pos=0;  return '?'; }  // missing
        else  // parameter is specified in the next argument
          optarg=argv[curr_arg++];
        curr_arg_pos=0;
      }
    return *pos;
  }  
}
#endif

static char * options_help[] = {
  gettext_noop("Options are:"), 
  gettext_noop("-V --version             --- print the version and exit"), 
  gettext_noop("-l --list                --- print available servers and exit"), 
  gettext_noop("-q --quiet               --- work quietly"), 
  gettext_noop("-v --verbose             --- verbose mode"), 
  gettext_noop("-X --no-602ccli          --- do not process the config files"), 

  gettext_noop("-c --command <commands>  --- process the commands and exit"), 
  gettext_noop("-f --file <filename>     --- read commands from filename"), 
  gettext_noop("-o --output <filename>   --- put all output to filename"), 

  gettext_noop("-d --dbname <database>   --- connect to the database server"),
  gettext_noop("-U --username <name>     --- login as the specified user"),
  gettext_noop("-W --password <password> --- login with the password"),
  gettext_noop("-m --schema <name>       --- open the schema"),
  
  gettext_noop("-P --pset <name=value>   --- set the formatting option"),
  gettext_noop("-F --field-separator <|> --- set the field separator"),
  gettext_noop("-R --record-separator <> --- set the record separator"),
  gettext_noop("-t --tuples-only         --- turn off printing column names"),
  gettext_noop("-x --expanded            --- turn on the expanded formatting mode"),
  gettext_noop("-A --no-align            --- turn on the unaligned formatting"),
  gettext_noop("-? --help                --- show this help"),
  NULL
};

const char * short_opts_descr = "VlqvXc:f:o:d:U:W:m:s:AF:R:txP:i:I:";

// long options:
const option long_opts_descr[] = {
  { "help",        0, NULL, '?' },
  { "command",     1, NULL, 'c' },
  { "file",        1, NULL, 'f' },
  { "output",      1, NULL, 'o' },
  { "version",     0, NULL, 'V' },
  { "list",        0, NULL, 'l' },
  { "quiet",       0, NULL, 'q' },
  { "verbose",     0, NULL, 'v' },
  { "no-602ccli",  0, NULL, 'X' },
  
  { "dbname",      1, NULL, 'd' },
  { "username",    1, NULL, 'U' },  // default is ANONYMOUS
  { "password",    1, NULL, 'W' },  // default is empty password
  { "schema",      1, NULL, 'm' },

  { "no-align",         0, NULL, 'A' },
  { "field-separator",  1, NULL, 'F' },
  { "record-separator", 1, NULL, 'R' },
  { "tuples_only",      0, NULL, 't' }, 
  { "expanded",         0, NULL, 'x' },
  { "pset",             1, NULL, 'P' },
  
  { "", 1, NULL, 'c' },
  { NULL, 0, NULL, 0 }
};

bool terminate_after_processing_options = false;

void process_options_from_argv(int argc, char **argv)
// Process options whicg may overturn the options set in the config files.
{ int opt;
  while ((opt=getopt_long(argc, argv, short_opts_descr, long_opts_descr, NULL))!=-1)
  switch (opt)
  {
   // main operational options terminating the operation:
    case 'V':  // print version and exit
    { char version_msg[30];
      sprintf(version_msg, "%u.%u.%u.%u", VERS_1, VERS_2, VERS_3, VERS_4);
      output_message(version_msg, true, false);
      if (interactive_output) newline();  // Linux consoles need this, otherwise the shell prompt follows the version number
      terminate_after_processing_options=true;
      break;
    } 
    case 'l':  // list servers and exit
      show_servers(NULL);
      terminate_after_processing_options=true;
      break;

    case '?': 
    { for (int i=0;  options_help[i];  i++)
      { output_message(gettext(options_help[i]), true, false);
        newline();
      }  
      terminate_after_processing_options = true;
      return;  // stop processing options immediately after error
    }

   // redirection options: 
    case 'c':
      command_from_args=optarg;  command_from_args_specified=true;
      break;
    case 'f':
      input_filename=optarg;
      break;
    case 'o':
      set_output_file((char*)optarg);  // immediately change the output file, used by 'V' and 'l'
      break;
   // general options:
    case 'X':
      disable_config_files=true;
      break;
    case 'q':
      verbose=false;
      break;
    case 'v':
      verbose=true;
      break;
   // database options:
    case 'd':
      strmaxcpy(ServerName, optarg, sizeof(ServerName));
      break;
    case 'U':
      strmaxcpy(LoginName,  optarg, sizeof(LoginName));   // default is ANONYMOUS
      break;
    case 'W':
      strmaxcpy(Password,   optarg, sizeof(Password));    // default is empty password
      break;
    case 'm':
      strmaxcpy(SchemaName, optarg, sizeof(SchemaName));
      break;
   // display options:
    case 'A':
      disp_aligned=false;  break;
    case 'F':  
      field_separator = optarg;  break;
    case 'R':
      record_separator = optarg;  break;
    case 't':
      tuples_only = true;  break;
    case 'x':
      expanded = true;  break;
    case 'P':
    {// divide the name and the value:
      char * p = (char*)optarg;
      while (*p && *p!='=') p++;
      if (!*p) p=NULL;  // no value
      else { *p=0;  p++; }  // separate the name and the value
     // store: 
      process_assignment((char*)optarg, p, false);
      break;
    }  
  }
}

