/* do_sql.cpp: Read one or more SQL command(s) from command line and execute. */
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
#include <envz.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define PROMPT "# "
#define PROMPT_CONT "+# "

bool error=false;
bool input_terminated=false;

void t_input_processor::process_metastatement(void)
{// clear whitespaces on the end: 
  while (!metastatement.empty() && (unsigned char)metastatement[metastatement.length()-1] <= ' ')
    metastatement.erase(metastatement.length()-1, 1);
  const char * stmt = metastatement.c_str();
 // find the name of the metastatement:
  size_t len=0;
  while ((unsigned char)stmt[len] > ' ') len++;
 // search the metastatement: 
  int fnd_index=-1, pref_count = 0;
  for (int i=0;  commands_list[i].name;  i++)
  { if (!memcmp(commands_list[i].name, stmt, len))
    { fnd_index=i;
      if (commands_list[i].name[len]==0)  // exact match
        { pref_count=0;  break; }  // ignoring prefix matches
      else
        pref_count++;  // prefix match
    }    
  }    
  if (fnd_index==-1 || pref_count>1)  // not found
  { if (verbose) 
    { output_message(_("Undefined metastatement: "), true, true);
      output_message(metastatement.c_str(), true, true);
      newline();
    }  
    error=true;
  }
  else
  { stmt+=len;
    while (*stmt==' ') stmt++;
    (this->*commands_list[fnd_index].action)((char*)stmt);
  }  
}

#define MAX_SQL_STATEMENTS 10

void t_input_processor::process_sql_statement(void)
{
  if (!processing_config_file)
	{ if (!connected)
	    output_message(_("Client is not connected to the SQL server.\n"), true, false);
	  else
	  {// remove leading CRLF from the processing of the previous statement
	    while (sql_statement.length()>0 && sql_statement[0]<' ')
	      sql_statement.erase(0, 1);
	   // execute:
	    uns32 results[MAX_SQL_STATEMENTS];
      memset(results, 0xff, sizeof(uns32)*MAX_SQL_STATEMENTS);
      results[0]=0;
	    if (cd_SQL_execute(cdp, sql_statement.c_str(), results))
	      write_error_info(cdp, "SQL", cd_Sz_error(cdp));
	    else
	    { //inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  // statement may by ALTER TABLE etc.
        bool any_specific=false;
        for (int i=0;  i<MAX_SQL_STATEMENTS;  i++)
          if (results[i]!=0xffffffff)
            if (IS_CURSOR_NUM(results[i]))
            { tcursnum curs = results[i];
              display_cursor_contents(cdp, curs);
              cd_Close_cursor(cdp, curs);
              if (interactive_output) newline();  // better appearance on the screen
              any_specific=true;
            }
            else // count-returning statements
            { char msg[200];
              sprintf(msg, _("Statement has been executed, number of processed records or object number is: %u.\n"), results[i]);
              output_message(msg, true, false);
              any_specific=true;
            }
        if (!any_specific)
          output_message(_("OK\n"), true, false);
	    }  
	  }
	}  
  sql_statement="";  // clear the buffer even on error
}

#define INBUF_SIZE 1024

void Do_SQL(void)
{ t_input_processor ip;
  do
  {
#ifdef LINUX
   // get next line:
    if (interactive && libreadline_is_present)
    { char * lineread=readline(ip.continuing_sql() ? PROMPT_CONT : PROMPT);
      lines_on_page=0;
      if (!lineread) 
        input_terminated=true;
      else
      { if (lineread[0]) add_history(lineread);
        ip.process_input(lineread);
        ip.process_input("\n");
        if (lineread) free(lineread);
      }  
    }
    else  
#endif    
    { char inbuf[INBUF_SIZE+1];  DWORD rd;
     // prompt:
       const char * pr = ip.continuing_sql() ? PROMPT_CONT : PROMPT;
#ifdef LINUX
      if (interactive && interactive_output)
        fwrite(pr, 1, strlen(pr), hStdout);
     // read the next part of input (must stop on LF because I may be reading interactive input when libreadline is not present):
      rd=0;
      while (rd<INBUF_SIZE)
      { int ard=fread(inbuf+rd, 1, 1, hStdin);
        rd+=ard;
        if (!ard || rd && inbuf[rd-1]=='\n') break;
      }
#else
      if (interactive && interactive_output)
        WriteFile(hStdout, pr, (DWORD)strlen(pr), &rd, NULL);
     // read the next part of input (must use ReadFile because the input may be redirected):
      if (!ReadFile(hStdin, inbuf, INBUF_SIZE, &rd, NULL)) rd=0;
#endif
      inbuf[rd]=0;
      lines_on_page=0;
      if (!rd) 
        input_terminated=true;
      else  
        ip.process_input(inbuf);
    }    
  } while (!input_terminated);
}

void t_input_processor::update_scanning_state(char c)
{
  if (quote && c==quote && !escaped) 
    quote=0;
  else if (!quote && (c=='"' || c=='\'') && !escaped)
    quote=c;
  else if (c=='\\' && !escaped) 
    escaped=true; 
  else
    escaped=false;
}    

void t_input_processor::process_input(const char * input)
// processes the next part of input read from the console or a file
{
  while (*input)
  { if (composing_metastatement)
    { while (*input && *input!='\n' && *input!='\r' && (*input!='\\' || escaped || quote))
      { metastatement=metastatement+*input;
        update_scanning_state(*input);
        input++;
      }  
      if (!*input) return;  // not terminated yet
      if (*input=='\\' && metastatement.empty())
        input++;  // this is the terminator of the metastatement
      else  
      { process_metastatement();
        while (*input=='\n' || *input=='\r')
          input++;  // skipping the terminator of the metastatement
        metastatement="";  escaped=false;  quote=0;
      }  
      composing_metastatement=false;  
    }
    else if (*input=='\\' && !escaped)
    { if (composing_metastatement && metastatement.length()==0)
        composing_metastatement=false;
      else  
        composing_metastatement=true;
      input++;
    }
    else  // adding to the SQL metastatement
    { while (*input && (*input!=';' || escaped || quote) && (*input!='\\' || quote))
      { sql_statement=sql_statement+*input;
        update_scanning_state(*input);
        input++;
      }  
      if (!*input) return;  // not terminated yet
      if (*input!='\\')     // not terminated yet
      { process_sql_statement();
        if (*input==';') input++;
        escaped=false;  quote=0;
      }
    }
  }
}

void process_command_file(char * fname, bool report_missing_file)
// Processes the contents of config files and command files.
{ FILE * f;
  f=fopen(fname, "r");
  if (!f) 
  { 
#ifdef LINUX
    if (errno==ENOENT) 
#else
    if (GetLastError()==ERROR_FILE_NOT_FOUND || GetLastError()==ERROR_PATH_NOT_FOUND)
#endif    
      if (!report_missing_file) return;  // silently skipping the file
    perror(fname);
  }
  else
  { char buf[256+1];  size_t rd;
    t_input_processor ip;
    do
    { rd = fread(buf, 1, 256, f);
      buf[rd]=0;
      ip.process_input(buf);
    } while (rd==256);
    ip.process_input("\n");  // terminates the last metastatement, if not terminated
    fclose(f); 
  }
}

