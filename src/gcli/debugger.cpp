// debugger.cpp
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#endif                                           
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop
#include "regstr.h"
#include "fsed9.h"
#include "debugger.h"

#define ADD_WATCH wxT("") // _("<Add watch>")

#include "xrc/WatchListDlg.cpp"
#include "xrc/EvalDlg.cpp"
#include "xrc/CallStackDlg.cpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

/* Theory:
Debugged thread -------------------------------------------
Debugger may be activated in two situations:
- the connection is cloned, debugger is attached to it and a statement is executed in it,
- debugger is attached to an existing connection.

BP --------------------------------------------------------
Breakpoints may be created and removed in all object editors with suitable category independent of the fact 
  if any debugging is active.
When a BP is toggled and debugging is active, then the BP is created on the server too, in the context of the debugged thread.
When debugging is started all BPs existing in editors are created on the server. 
  Some of them may be deleted or moved to the next valid line.
When a new object editor is opened and debugging is active then BPs belonging to the object are read from the server to the editor.
*/

server_debugger * the_server_debugger = NULL;
WatchListDlg * persistent_watch_view = NULL;
CallStackDlg * persistent_call_stack = NULL;
#ifdef FL
wxDynamicToolBar * persistent_dbg_toolbar = NULL;
#else
wxToolBar * persistent_dbg_toolbar = NULL;
#endif
string_history persistent_eval_history(15);

void wxSrvDbgTimer::Notify(void)
{ if (the_server_debugger) the_server_debugger->OnTimerEvent();
}

wxThread::ExitCode t_debugged_thread::Entry(void)
{ err=cd_SQL_host_execute(cdp, stmt.mb_str(*cdp->sys_conv), results, hostvars, hostvars_count);
  terminated=true;
  return (void*)(size_t)err;
}

void server_debugger::start_scanning(void)
{ 
#ifdef _DEBUG
  wxLogDebug(wxT("Timer started"));
#endif  
  if (!debug_timer.IsRunning()) debug_timer.Start(700, wxTIMER_CONTINUOUS); 
}

bool server_debugger::start_debugging(cdp_t cdp2, wxString * stmt)
{// put cdp2 into the debugged mode:
  if (cd_Server_debug(cdp, DBGOP_INIT2, cd_Client_number(cdp2), 0, NULL, NULL, NULL)) 
    { cd_Signalize(cdp);  return false; }
 // move existing BPs to the server:
  realize_bps(cdp);
 // start scanning the state of cdp2:
  start_scanning();
 // create and run a thread in cdp2:
  memset(results, 0xff, sizeof(uns32)*MAX_SQL_STATEMENTS);
  if (dlg1)
    debugged_thread = new t_debugged_thread(wxTHREAD_JOINABLE, cdp2, results, dlg1->hvars, dlg1->par_cnt, stmt);
  else  // without host variables
    debugged_thread = new t_debugged_thread(wxTHREAD_JOINABLE, cdp2, results, NULL, 0, stmt);
  if (!debugged_thread) return false;
  if (debugged_thread->Create() != wxTHREAD_NO_ERROR) 
    { delete debugged_thread;  debugged_thread=NULL;  return false; }
  if (debugged_thread->Run() != wxTHREAD_NO_ERROR) 
    { delete debugged_thread;  debugged_thread=NULL;  return false; }
  if (dlg1) 
  { dlg1->clear_results();
    dlg1->EnableControls(false); /*dlg1->Disable();  dlg1->Hide();*/  // disabling the dialog makes it impossible to move it away
  }
  if (dlg2) dlg2->EnableControls(false);
  debug_edit_enable(false); 
  // Takes ownership of cdp.
  return true;
}

void attach_debugging(cdp_t cdp, int client_num)
// supposes the_server_debugger==NULL on entry.
{ the_server_debugger = new server_debugger(cdp);
  if (!the_server_debugger) return;
 // put the other client into the debugged mode:
  if (cd_Server_debug(cdp, DBGOP_INIT2, client_num, 0, NULL, NULL, NULL)) 
    { cd_Signalize(cdp);  return; }
 // move existing BPs to the server:
  realize_bps(cdp);
 // show the initial state of the debugged process:
  the_server_debugger->OnTimerEvent();
 // start scanning the state of cdp2:
  the_server_debugger->start_scanning();
  debug_edit_enable(false);
}

void server_debugger::OnTimerEvent(void)
{ int state;  tobjnum objnum;  int line;
#ifdef _DEBUG
  wxLogDebug(wxT("Timer event"));
#endif  
  if (!cd_Server_debug(cdp, DBGOP_GET_STATE, 0, 0, &state, &objnum, &line))
    process_new_debug_state(cdp, state, objnum, line);
}

void server_debugger::ToggleBreakpoint(editor_type * edd, int line_n)
{ int pos;  bool adding;
 // try to store BP on the server:
  if (!cd_Server_debug(edd->cdp, DBGOP_REMOVE_BP, edd->objnum, line_n+1, NULL, NULL, &pos))
    adding=false;
  else if (!cd_Server_debug(edd->cdp, DBGOP_ADD_BP, edd->objnum, line_n+1, NULL, NULL, &pos))
    adding=true;
  else { wxBell();  return; }
 // store BP locally:
  wxChar * bpline = edd->line_num2ptr(pos-1);
  if (adding) edd->breakpts.add(bpline);
  else edd->breakpts.del(bpline);
  if (edd->cur_line_n+1==pos) edd->refresh_current_line();  else edd->Refresh2();
}

void server_debugger::OnCommand(int act)
{ editor_type * edd = NULL;
  int state;  tobjnum objnum;  int line;
  if (act!=DBG_BREAK && act!=DBG_EXIT && act!=DBG_KILL)
  { if (current_state!=DBGST_BREAKED) return;
    edd=find_editor_by_objnum9(cdp, OBJ_TABLENUM, stop_objnum, OBJ_DEF_ATR, NONECATEG, false);
  }
  switch (act)
  {
    case DBG_STEP_OVER:
      stop_line=-1;  
      if (edd) edd->Refresh2();  // removes the colour of the active line
      if (cd_Server_debug(cdp, DBGOP_STEP, 0, 0, &state, &objnum, &line)) break;
      process_new_debug_state(cdp, state, objnum, line);
      break;
    case DBG_STEP_INTO:
      stop_line=-1;  
      if (edd) edd->Refresh2();  // removes the colour of the active line
      if (cd_Server_debug(cdp, DBGOP_TRACE, 0, 0, &state, &objnum, &line)) break;
      process_new_debug_state(cdp, state, objnum, line);
      break;
    case DBG_RETURN:
      stop_line=-1;  
      if (edd) edd->Refresh2();  // removes the colour of the active line
      if (cd_Server_debug(cdp, DBGOP_OUTER, 0, 0, &state, &objnum, &line)) break;
      process_new_debug_state(cdp, state, objnum, line);
      break;
    case DBG_GOTO:
      stop_line=-1;  
      if (edd) edd->Refresh2();  // removes the colour of the active line
      if (edd==NULL) break;  // not in object editor
      if (cd_Server_debug(cdp, DBGOP_GOTO, edd->objnum, edd->cur_line_n+1, &state, &objnum, &line)) break;
      process_new_debug_state(cdp, state, objnum, line);
      break;
    case DBG_RUN:
      stop_line=-1;  
      if (edd) edd->Refresh2();  // removes the colour of the active line
      if (cd_Server_debug(cdp, DBGOP_GO, 0, 0, &state, &objnum, &line)) break;
      process_new_debug_state(cdp, state, objnum, line);
      break;
    case DBG_EVAL:
    { EvalDlg dlg(cdp, wxEmptyString);  
      dlg.Create(wxGetApp().frame);  dlg.ShowModal();
      break;
    }
    case DBG_TOGGLE:
      if (edd==NULL) break;  // not in object editor
      ToggleBreakpoint(edd, edd->cur_line_n);
      break;
    case DBG_EXIT: // allow the debugged thread to run without any further intervention, close the debugger
    case DBG_KILL:
      debug_timer.Stop();
      set_state_text(_("running"));  // shows this state until run terminates
      cd_Server_debug(cdp, act==DBG_EXIT ? DBGOP_CLOSE : DBGOP_KILL, 0, 0, NULL, NULL, NULL);
     // join the thread, but do not show the results (otherwise the thread would not disconnect from the server):
     // must wait before destroying the dialog because the thread writes to its host vars!
      if (debugged_thread)
      { debugged_thread->Wait();
        if (dlg1) dlg1->Destroy();
        if (dlg2) 
          dlg2->display_results(results, debugged_thread->cdp);
        wx_disconnect(debugged_thread->cdp);  delete debugged_thread->cdp;
#if wxMAJOR_VERSION==2 && wxMINOR_VERSION<6  // reports error in 2.6.0
        debugged_thread->Delete();  // is necessary?
#endif
        delete debugged_thread;  // must delete explicitly because the thread is joinable
        debugged_thread=NULL;
      }
     // destroy the debugger:
      the_server_debugger=NULL;
      debug_edit_enable(true);  // should be called when the_server_debugger is NULL (enables Compile button)
      stop_line=-1;
      edd=find_editor_by_objnum9(cdp, OBJ_TABLENUM, stop_objnum, OBJ_DEF_ATR, NONECATEG, false);
      if (edd) edd->Refresh2();  // hides the active line
      delete this;
      break;
    case DBG_BREAK:
      cd_Server_debug(cdp, DBGOP_BREAK, 0, 0, NULL, NULL, NULL);
      break;
  }
}

void server_debugger::process_new_debug_state(cdp_t cdp, int state, tobjnum objnum, int line, bool init)
{ editor_type * edd;  
  if (debugged_thread && debugged_thread->terminated) 
  { state=DBGST_NOT_FOUND;
#ifdef _DEBUG
    wxLogDebug(wxT("debug thread is terminated"));
#endif  
  }
  switch (state)
  { case DBGST_NOT_FOUND:  // proces skoncil, skonci ladeni
#ifdef _DEBUG
      wxLogDebug(wxT("Debug State not found"));
#endif  
      debug_timer.Stop();
     // join the thread:
      if (debugged_thread)
      { 
#ifdef _DEBUG
      wxLogDebug(wxT("Debug wait started"));
#endif  
        debugged_thread->Wait();
#ifdef _DEBUG
      wxLogDebug(wxT("Debug wait ended"));
#endif  
        if (dlg2) // show results or errors in the output window
          dlg2->display_results(results, debugged_thread->cdp);
        else // show errors in modal dialog
          if (debugged_thread->err) cd_Signalize2(debugged_thread->cdp, wxGetApp().frame);
        if (dlg1) 
        { if (!debugged_thread->err) dlg1->display_results();
          dlg1->EnableControls(true);
        }
        check_open_transaction(debugged_thread->cdp, wxGetApp().frame);  
#ifdef _DEBUG
      wxLogDebug(wxT("Debug results shown"));
#endif  
        wx_disconnect(debugged_thread->cdp);  delete debugged_thread->cdp;
#if wxMAJOR_VERSION==2 && wxMINOR_VERSION<6  // reports error in 2.6.0
        debugged_thread->Delete();  // is necessary?
#endif
        delete debugged_thread;  // must delete explicitly because the thread is joinable
        debugged_thread=NULL;
      }
#ifdef _DEBUG
      wxLogDebug(wxT("Debug thread deleted"));
#endif  
     // destroy the debugger:
      the_server_debugger=NULL;
      debug_edit_enable(true);  // should be called when the_server_debugger is NULL (enables Compile button)
      stop_line=-1;
      edd=find_editor_by_objnum9(cdp, OBJ_TABLENUM, stop_objnum, OBJ_DEF_ATR, NONECATEG, false);
      if (edd) edd->Refresh2();  // hides the active line
      if (dlg1) dlg1->Raise();  // must be after find_editor_by_objnum9()!
      delete this;
      return;
    case DBGST_BREAKED:    // show the position
    { if (state!=current_state) set_state_text(_("break"));
#ifdef _DEBUG
      wxLogDebug(wxT("Debug State breaked"));
#endif  
      debug_timer.Stop();
      watch_view->recalc_all();
      watch_view->Enable();
      call_stack->update(true);
      call_stack->Enable();
      stop_objnum=objnum;  stop_line=line;   // stop_line is 1-based
      if (objnum!=NOOBJECT)
      { edd=find_editor_by_objnum9(cdp, OBJ_TABLENUM, stop_objnum, OBJ_DEF_ATR, NONECATEG, true);
        if (edd)
        { edd->position_on_line_column(line, 0);
          edd->Refresh2();  // show the active line
        }
      }
      break;
    }
    case DBGST_EXECUTING:  // debugged process runs, debug commands disabled
#ifdef _DEBUG
      wxLogDebug(wxT("Debug State running"));
#endif  
      if (state!=current_state) set_state_text(_("running"));
      watch_view->Disable();
      call_stack->update(false);
      call_stack->Disable();
      start_scanning();
      break;
    case DBGST_NO_REQUEST:
#ifdef _DEBUG
      wxLogDebug(wxT("Debug State waiting"));
#endif  
      if (state!=current_state) set_state_text(_("waiting"));
      watch_view->Disable();
      call_stack->update(false);
      call_stack->Disable();
      start_scanning();
      break;
  }
  update_toolbar(current_state, (t_debugger_states)state);
  current_state=(t_debugger_states)state;
}

#include "bmps/traceInto.xpm"
#include "bmps/stepOver.xpm"
#include "bmps/runToCursor.xpm"
#include "bmps/runUntilReturn.xpm"
#include "bmps/evaluate.xpm"
#include "bmps/break.xpm"
#include "bmps/runu.xpm"
#include "bmps/debugRun.xpm"
#include "bmps/debugKill.xpm"

void server_debugger::update_toolbar(t_debugger_states oldstate, t_debugger_states newstate)
{ bool was_breaked = oldstate==DBGST_BREAKED;
  bool  is_breaked = newstate==DBGST_BREAKED;
  if (was_breaked!=is_breaked || oldstate==DBGST_UNDEFINED)
  { dbg_toolbar->EnableTool(DBG_RUN, is_breaked);
    dbg_toolbar->EnableTool(DBG_STEP_INTO, is_breaked);
    dbg_toolbar->EnableTool(DBG_STEP_OVER, is_breaked);
    dbg_toolbar->EnableTool(DBG_GOTO, is_breaked);
    dbg_toolbar->EnableTool(DBG_RETURN, is_breaked);
    dbg_toolbar->EnableTool(DBG_EVAL, is_breaked);
  }
  if ((oldstate==DBGST_EXECUTING) != (newstate==DBGST_EXECUTING) || oldstate==DBGST_UNDEFINED)
    dbg_toolbar->EnableTool(DBG_BREAK, newstate==DBGST_EXECUTING);
}

void server_debugger::create_dbg_windows(void)
{ 
 // toolbar:
  if (persistent_dbg_toolbar)
  { dbg_toolbar=persistent_dbg_toolbar;
    //state_text=(wxStaticText*)dbg_toolbar->FindWindow(DBG_STATE);
    state_text=(wxTextCtrl*)dbg_toolbar->FindWindow(DBG_STATE);
  }
 // call stack:
  if (persistent_call_stack)
  { call_stack=persistent_call_stack;
    call_stack->my_server_debugger=this;
  }
 // watch list:
  if (persistent_watch_view)
  { watch_view=persistent_watch_view;
    watch_view->my_server_debugger=this;
  }
 // switch layouts:
  switch_layouts(true);
}

server_debugger::server_debugger(cdp_t cdpIn)
{ cdp=cdpIn;
  dlg1=NULL;  dlg2=NULL;
  current_state=DBGST_UNDEFINED;
  create_dbg_windows();
  debugged_thread=NULL;
}

server_debugger::~server_debugger(void)
{ 
  the_server_debugger = NULL;
  if (call_stack)
    call_stack->my_server_debugger=NULL;  // not necessary
  if (watch_view)
    watch_view->my_server_debugger=NULL;  // not necessary
 // switch layouts:
  switch_layouts(false);
}

/////////////////////////////////////// watch support ////////////////////////////////

// Mouse selecting of items does not work.
// Keys not delivered before any item is selected by up ro down arrow - at least 2 items must be in the watch list.
// Adding images, removing rules, removing event table, writing text 
//   to the fictive item and writing space to the value column do not help.


wxWindow * create_persistent_watch_view(wxWindow * parent, int id)
{ persistent_watch_view = new WatchListDlg(NULL);  // no link to the debugger at the moment (will be established when debugging starts)
  if (!persistent_watch_view) return NULL;
  persistent_watch_view->Create(parent, id);//, wxDefaultPosition, wxDefaultSize, wxLC_EDIT_LABELS | wxLC_REPORT | wxLC_HRULES | wxLC_VRULES);
  //persistent_watch_view->Hide();
  return persistent_watch_view;
}

wxWindow * create_persistent_call_stack(wxWindow * parent, int id)
{ persistent_call_stack = new CallStackDlg(NULL);  // no link to the debugger at the moment (will be established when debugging starts)
  if (!persistent_call_stack) return NULL;
  persistent_call_stack->Create(parent, id);
  return persistent_call_stack;
}

#ifdef FL
wxDynamicToolBar * create_persistent_dbg_toolbar(wxWindow * parent)
{
  persistent_dbg_toolbar = new wxDynamicToolBar();
  persistent_dbg_toolbar->Create(parent, -1);
  persistent_dbg_toolbar->AddTool(DBG_RUN, wxBitmap(debugRun_xpm), wxNullBitmap, false, -1, -1, NULL, _("Continue Execution"));
  persistent_dbg_toolbar->AddSeparator();
  persistent_dbg_toolbar->AddTool(DBG_STEP_INTO, wxBitmap(traceInto_xpm), wxNullBitmap, false, -1, -1, NULL, _("Trace into"));
  persistent_dbg_toolbar->AddTool(DBG_STEP_OVER, wxBitmap(stepOver_xpm), wxNullBitmap, false, -1, -1, NULL, _("Step over"));
  persistent_dbg_toolbar->AddTool(DBG_GOTO, wxBitmap(runToCursor_xpm), wxNullBitmap, false, -1, -1, NULL, _("Goto"));
  persistent_dbg_toolbar->AddTool(DBG_RETURN, wxBitmap(runUntilReturn_xpm), wxNullBitmap, false, -1, -1, NULL, _("Return from the current routine"));
  persistent_dbg_toolbar->AddTool(DBG_EVAL, wxBitmap(evaluate_xpm), wxNullBitmap, false, -1, -1, NULL, _("Evaluate"));
  persistent_dbg_toolbar->AddSeparator();
  persistent_dbg_toolbar->AddTool(DBG_BREAK, wxBitmap(break_xpm), wxNullBitmap, false, -1, -1, NULL, _("Break"));
  persistent_dbg_toolbar->AddTool(DBG_EXIT, wxBitmap(runu_xpm), wxNullBitmap, false, -1, -1, NULL, _("Continue without debugging"));
  persistent_dbg_toolbar->AddTool(DBG_KILL, wxBitmap(debugKill_xpm), wxNullBitmap, false, -1, -1, NULL, _("Kill the debug process"));
  persistent_dbg_toolbar->AddSeparator();
  //wxStaticText * state_text = new wxStaticText(persistent_dbg_toolbar, DBG_STATE, "?", wxDefaultPosition, wxSize(100,20), wxSUNKEN_BORDER);
  wxTextCtrl * state_text = new wxTextCtrl(persistent_dbg_toolbar, DBG_STATE, wxT("?"), wxDefaultPosition, wxSize(100,30), wxSUNKEN_BORDER | wxTE_READONLY);
  persistent_dbg_toolbar->AddTool(DBG_STATE, state_text, wxSize(120,30));
  return persistent_dbg_toolbar;
}
#else
wxToolBar * create_persistent_dbg_toolbar(wxWindow * parent)
{
  persistent_dbg_toolbar = new wxToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
  persistent_dbg_toolbar->SetToolBitmapSize(wxSize(16,16));
  persistent_dbg_toolbar->AddTool(DBG_RUN, wxEmptyString, wxBitmap(debugRun_xpm), _("Continue Execution"));
  persistent_dbg_toolbar->AddSeparator();
  persistent_dbg_toolbar->AddTool(DBG_STEP_INTO, wxEmptyString, wxBitmap(traceInto_xpm), _("Trace into"));
  persistent_dbg_toolbar->AddTool(DBG_STEP_OVER, wxEmptyString, wxBitmap(stepOver_xpm), _("Step over"));
  persistent_dbg_toolbar->AddTool(DBG_GOTO, wxEmptyString, wxBitmap(runToCursor_xpm), _("Goto"));
  persistent_dbg_toolbar->AddTool(DBG_RETURN, wxEmptyString, wxBitmap(runUntilReturn_xpm),  _("Return from the current routine"));
  persistent_dbg_toolbar->AddTool(DBG_EVAL, wxEmptyString, wxBitmap(evaluate_xpm), _("Evaluate"));
  persistent_dbg_toolbar->AddSeparator();
  persistent_dbg_toolbar->AddTool(DBG_BREAK, wxEmptyString, wxBitmap(break_xpm), _("Break"));
  persistent_dbg_toolbar->AddTool(DBG_EXIT, wxEmptyString, wxBitmap(runu_xpm), _("Continue without debugging"));
  persistent_dbg_toolbar->AddTool(DBG_KILL, wxEmptyString, wxBitmap(debugKill_xpm), _("Kill the debug process"));
  persistent_dbg_toolbar->AddSeparator();
  //wxStaticText * state_text = new wxStaticText(persistent_dbg_toolbar, DBG_STATE, "?", wxDefaultPosition, wxSize(100,20), wxSUNKEN_BORDER);
  wxTextCtrl * state_text = new wxTextCtrl(persistent_dbg_toolbar, DBG_STATE, wxT("?"), wxDefaultPosition, wxSize(120,30), wxSUNKEN_BORDER | wxTE_READONLY);
  persistent_dbg_toolbar->AddControl(state_text);
  persistent_dbg_toolbar->Realize();
  return persistent_dbg_toolbar;
}
#endif

void destroy_persistent_debug_windows(void)
{ if (persistent_watch_view)
  { persistent_watch_view->Destroy();
    persistent_watch_view=NULL;
  }
  if (persistent_call_stack)
  { persistent_call_stack->Destroy();
    persistent_call_stack=NULL;
  }
  if (persistent_dbg_toolbar)
  { persistent_dbg_toolbar->Destroy();
    persistent_dbg_toolbar=NULL;
  }
}

