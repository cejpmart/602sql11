// debugger.h
#include <wx/thread.h>
#include <wx/listctrl.h>
#include <wx/dnd.h>

#include "xrc/CallRoutineDlg.h"
#include "xrc/SQLConsoleDlg.h"

// states of the debugged process:
enum t_debugger_states { DBGST_NOT_FOUND=0, DBGST_BREAKED=1, DBGST_EXECUTING=2, DBGST_NO_REQUEST=3, DBGST_UNDEFINED };

class wxSrvDbgTimer : public wxTimer
{ void Notify(void);
};

class CallRoutineDlg;
class SQLConsoleDlg;
class server_debugger;

class t_debugged_thread : public wxThread
{public:
  cdp_t cdp;
  wxString stmt;  // must have copy of the string
  uns32 * results;
  struct t_clivar * hostvars;
  unsigned hostvars_count;
  BOOL err;
  bool terminated;
  ExitCode Entry(void);  
  t_debugged_thread(wxThreadKind kind, cdp_t cdpIn, uns32 * resultsIn, 
                    struct t_clivar * hostvarsIn, unsigned hostvars_countIn, wxString * stmtIn) 
    : wxThread(kind), cdp(cdpIn), results(resultsIn), hostvars(hostvarsIn), hostvars_count(hostvars_countIn)
      { stmt=*stmtIn;  terminated=false; }
  ~t_debugged_thread(void)
    { } // cdp may or may not be owned, do not destroy here
};

class CallStackDlg;
class WatchListDlg;

#if 0
class t_WatchView : public wxListCtrl
{ 
 public:
  server_debugger * my_server_debugger;
  void recalc(int ind, wxString name);
  void recalc_all(void);
  void append(const wxString & data, int ind = -1);
  void OnKeyDown(wxListEvent & event);
  void OnEndLabelEdit(wxListEvent & event);
  void OnDropText(wxCoord x, wxCoord y, const wxString & data);
  t_WatchView(server_debugger * my_server_debuggerIn)
    { my_server_debugger=my_server_debuggerIn; }
  DECLARE_EVENT_TABLE()
};
#endif

class server_debugger
{ friend class WatchListDlg;
  friend class CallStackDlg;
  wxSrvDbgTimer debug_timer;
  //wxStaticText * state_text;
  wxTextCtrl * state_text;
  t_debugged_thread * debugged_thread;
  uns32 results[MAX_SQL_STATEMENTS];
  void update_toolbar(t_debugger_states oldstate, t_debugger_states newstate);
  void create_dbg_windows(void);
 public:
  void set_state_text(wxString str)
  { //state_text->SetLabel(str);
    state_text->SetValue(str);
  }
  t_debugger_states current_state;
  CallStackDlg * call_stack;
  CallRoutineDlg * dlg1; // dialogs waiting for the debugged thread to terminate - externally assigned
  SQLConsoleDlg  * dlg2; // dialogs waiting for the debugged thread to terminate - externally assigned
  cdp_t cdp;
  WatchListDlg * watch_view;
#ifdef FL
  wxDynamicToolBar * dbg_toolbar;
#else
  wxToolBar * dbg_toolbar;
#endif
  tobjnum stop_objnum;  
  int stop_line;
  void OnCommand(int act);
  void OnTimerEvent(void);
  void process_new_debug_state(cdp_t cdp, int state, tobjnum objnum, int line, bool init=false);
  server_debugger(cdp_t cdpIn);
  ~server_debugger(void);
  bool start_debugging(cdp_t cdp2, wxString * stmt);
  void ToggleBreakpoint(editor_type * edd, int line_n);
  void start_scanning(void);
};

extern server_debugger * the_server_debugger;
extern string_history persistent_eval_history;

wxWindow * create_persistent_watch_view(wxWindow * parent, int id);
wxWindow * create_persistent_call_stack(wxWindow * parent, int id);
#ifdef FL
wxDynamicToolBar * create_persistent_dbg_toolbar(wxWindow * parent);
#else
wxToolBar * create_persistent_dbg_toolbar(wxWindow * parent);
#endif
void destroy_persistent_debug_windows(void);
void attach_debugging(cdp_t cdp, int client_ind);
