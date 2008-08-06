// management.cpp
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
#include "controlpanel.h" 
#include "datagrid.h"
#include "wprez9.h"

///////////// dcl
const char sqlserver_owner[] = "@SQLSERVER";

enum PropertyItemType { PROP_TP_BOOL, PROP_TP_STRING, PROP_TP_NUM, PROP_TP_COMBO };

class PropertyItem
// Does not have the virtual destructor - does not need it at the momment.
{public:
  bool changed;
  wxString loaded_value;
  const wxChar * name;
  const char * propname;
  PropertyItemType type;
  bool secur_admin;
  PropertyItem(const wxChar * nameIn, const char * propnameIn, PropertyItemType typeIn, bool secur_adminIn) : 
    name(nameIn), propname(propnameIn), type(typeIn), secur_admin(secur_adminIn) 
      { changed=false; }
};

class PropertyItemBool : public PropertyItem
{public: 
  PropertyItemBool(const wxChar * nameIn, const char * propnameIn, bool secur_adminIn) : 
    PropertyItem(nameIn, propnameIn, PROP_TP_BOOL, secur_adminIn) { }
};

class PropertyItemString : public PropertyItem
{public: 
  int max_length;
  PropertyItemString(const wxChar * nameIn, const char * propnameIn, bool secur_adminIn, int max_lengthIn) : 
    PropertyItem(nameIn, propnameIn, PROP_TP_STRING, secur_adminIn), max_length(max_lengthIn) { }
};

class PropertyItemNum : public PropertyItem
{public: 
  int maxval, minval;
  PropertyItemNum(const wxChar * nameIn, const char * propnameIn, bool secur_adminIn, int minvalIn=-1, int maxvalIn=-1) : 
    PropertyItem(nameIn, propnameIn, PROP_TP_NUM, secur_adminIn), maxval(maxvalIn), minval(minvalIn) { }
};

class PropertyItemCombo : public PropertyItem
{public: 
  wxString choice;
  PropertyItemCombo(const wxChar * nameIn, const char * propnameIn, bool secur_adminIn, wxString choiceIn) : 
    PropertyItem(nameIn, propnameIn, PROP_TP_COMBO, secur_adminIn), choice(choiceIn) { }
};

WX_DEFINE_ARRAY_PTR(PropertyItem*, ArrayPropertyItems);

class PropertyGrid;

class PropertyGridTable : public wxGridTableBase
{ PropertyGrid * grid;
 public:
    int GetNumberRows();
 private:
    int GetNumberCols();
    bool IsEmptyCell( int row, int col );
    void SetValue( int row, int col, const wxString& value );
    wxString GetColLabelValue( int col );
    bool CanGetValueAs( int row, int col, const wxString& typeName );
 protected: 
    wxString GetValue( int row, int col );
 public:
  PropertyGridTable(PropertyGrid * gridIn)
    { grid=gridIn; }

};

class PropertyGrid
{public:
  cdp_t cdp;
  wxGrid * grid;
  ArrayPropertyItems items;
  virtual void create(wxGrid * gridIn, PropertyGridTable * grid_table = NULL);
  virtual void load_and_show(void);
  virtual bool save(void);
  PropertyGrid(cdp_t cdpIn) : cdp(cdpIn) { }
  ~PropertyGrid(void);
};

class InternPropertyGridTable : public PropertyGridTable
{ virtual wxString GetColLabelValue(int col);
 public:
  InternPropertyGridTable(PropertyGrid * gridIn) : PropertyGridTable(gridIn) { }
};

class InternProperties : public PropertyGrid
{public:
  InternProperties(cdp_t cdpIn);
  void create(wxGrid * gridIn, PropertyGridTable * grid_table);
  void load_and_show(void);
};

class PropertyGridWin : public wxGrid
{
 public:
  InternProperties * props;
  PropertyGridWin(InternProperties * propsIn, wxWindow* parent, wxWindowID id) 
    : wxGrid(parent, id, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE)  // wxFULL_REPAINT_ON_RESIZE needed for FL panes since WX 2.6.0
    { props=propsIn; }
  ~PropertyGridWin(void)
    { delete props; }
  void OnCellRightClick(wxGridEvent & event);
  void OnLabelRightClick(wxGridEvent & event);
  void OnxCommand( wxCommandEvent& event );
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(PropertyGridWin, wxGrid)
  EVT_GRID_CELL_RIGHT_CLICK(PropertyGridWin::OnCellRightClick)
  EVT_GRID_LABEL_RIGHT_CLICK(PropertyGridWin::OnLabelRightClick)
  EVT_MENU(-1, PropertyGridWin::OnxCommand)
END_EVENT_TABLE()



/////////////////////////////////// monitor /////////////////////////////////////////////////////

#define MONITOR_PAGE_MAIN    0
#define MONITOR_PAGE_CLIENTS 1
#define MONITOR_PAGE_LOCKS   2
#define MONITOR_PAGE_TRACE   3
#define MONITOR_PAGE_INTERN  4

BEGIN_EVENT_TABLE(MonitorNotebook, wxNotebook)
    EVT_TIMER(REFRESH_TIMER_ID, MonitorNotebook::OnTimer)
    EVT_MENU(CPM_REFRPANEL, MonitorNotebook::OnRefresh)
    EVT_SET_FOCUS(MonitorNotebook::OnFocus)
END_EVENT_TABLE()

void MonitorNotebook::OnFocus(wxFocusEvent& event)
{ 
// Without this the toolbar buttons cannot return focus to the proper page
#ifdef WINS
  if (GetSelection()!=-1) GetPage(GetSelection())->SetFocus();
#else
  // Direct SetFocus makes the navigation among pages by clicking arrows by mouse (usually) impossible on GTK!
  if (GetSelection()!=-1) wxGetApp().frame->SetFocusDelayed(GetPage(GetSelection()));
#endif  
  event.Skip();
}

#include "xrc/MonitorLogDlg.h"

void MonitorNotebook::OnTimer(wxTimerEvent& event)
{ refresh_all(); }

void MonitorNotebook::OnRefresh(wxCommandEvent & event)
{ refresh_all(); }

#include "xrc/MonitorMainDlg.h"

void monitored_server_selected(const char * name)
{ t_avail_server * as = find_server_connection_info(name);
  MonitorNotebook * monit = wxGetApp().frame->monitor_window;
  if (!as || !as->cdp)
  { monit->Stop_monitoring();  // used when -None- selected in the combo
    monit->reset_main_info();  // clears info on the main page - must be after Stop_monitoring() which sets cdp to NULL
  }
  else
    monit->AttachToServer(as->cdp);
}

void set_monitor_refresh(int refr_secs)
{ if (!refr_secs || refr_secs==(int)0x80000000)  // 0x80000000 obtained when the text control becomes empty
    wxGetApp().frame->monitor_window->m_timer.Stop();
  else
    wxGetApp().frame->monitor_window->m_timer.Start(1000*refr_secs, false);
}

#include "xrc/ClientsThreadsDlg.cpp"
#include "xrc/MonitorMainDlg.cpp"
#include "xrc/MonitorLogDlg.cpp"
#include "xrc/NewLogDlg.cpp"

void MonitorNotebook::Stop_monitoring(void)
{ if (!cdp) return;
 // close grids, if monitoring the connected server:
  m_timer.Stop();
  MonitorMainDlg * main = (MonitorMainDlg *)GetPage(MONITOR_PAGE_MAIN);
  main->mServer->SetSelection(0);
  main->mUser->SetValue(wxEmptyString);
  while (GetPageCount()>1) DeletePage(1);
  cdp=NULL;
}

void MonitorNotebook::Stop_monitoring_server(cdp_t a_cdp)
{ if (cdp && a_cdp==cdp) 
  { Stop_monitoring();
    reset_main_info();  // must be after Stop_monitoring() which sets cdp to NULL
  }
}    

void MonitorNotebook::Disconnecting(cdp_t dis_cdp)
{ 
  MonitorMainDlg * main = (MonitorMainDlg *)GetPage(MONITOR_PAGE_MAIN);
 // remove the server name from the combo
  int sel = main->mServer->FindString(wxString(dis_cdp->locid_server_name, *wxConvCurrent));
  if (sel!=-1) main->mServer->Delete(sel);
 // stop monitoring it, if monitoring: 
  Stop_monitoring_server(dis_cdp);
}

void MonitorNotebook::Connecting(cdp_t cdp)
{
  MonitorMainDlg * main = (MonitorMainDlg *)GetPage(MONITOR_PAGE_MAIN);
  main->mServer->Append(wxString(cdp->locid_server_name, *wxConvCurrent));
}

void MonitorNotebook::relist_logs(tcursnum curs)
{// delete log pages:
  while (GetPageCount()>monitor_fixed_pages) DeletePage(monitor_fixed_pages);
 // find logs:
  trecnum cnt, r;
  cd_Rec_cnt(cdp, curs, &cnt);
  bool basic_added = false;
  for (r=0;  r<cnt;  r++)
  { tobjname log_name;  char pathname[MAX_PATH];  t_varcol_size rd;
    if (!cd_Read(cdp, curs, r, 1, NULL, log_name))
    {
        if (cd_Read_var(cdp, curs, r, 2, NOINDEX, 0, sizeof(pathname)-1, pathname, &rd)) rd=0;
        pathname[rd]=0;
        MonitorLogDlg * dlg = new MonitorLogDlg(cdp, log_name);
        dlg->Create(this);
        if (dlg->mFile) dlg->mFile->SetLabel(wxString(pathname, *wxConvCurrent));
        dlg->refresh_log();
        InsertPage(*log_name && basic_added ? monitor_fixed_pages+1 : monitor_fixed_pages, dlg, *log_name ? wxString(log_name, *cdp->sys_conv) : wxString(_("Basic log")), false, 5);
        if (!*log_name) basic_added=true;
    }
  }
}

//////////////////////////////////// user list ///////////////////////////////////
class UserDataGrid : public DataGrid
{ int row_of_click;
 public:
  UserDataGrid(xcdp_t xcdpIn) : DataGrid(xcdpIn) {}
  void OnCellRightClick(wxGridEvent & event);
  void OnLabelRightClick(wxGridEvent & event);
  void OnxCommand(wxCommandEvent& event);
  DECLARE_EVENT_TABLE()
};

void UserDataGrid::OnCellRightClick(wxGridEvent & event)
{ row_of_click = event.GetRow();
  ClearSelection();
  SetGridCursor(event.GetRow(), event.GetCol());
  BOOL adm=cd_Am_I_config_admin(cdp);
  wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_REFRPANEL,    _("Refresh the list"));
  popup_menu->AppendSeparator();
  popup_menu->Append(CPM_CLIENT_BREAK, _("Interrupt the client operation"));
  popup_menu->Append(CPM_CLIENT_KILL,  _("Disconnect the client from the SQL server"));
  popup_menu->Enable(CPM_CLIENT_BREAK, adm==TRUE);
  popup_menu->Enable(CPM_CLIENT_KILL,  adm==TRUE);
  PopupMenu(popup_menu, event.GetPosition());
  delete popup_menu;
}

void UserDataGrid::OnLabelRightClick(wxGridEvent & event)
{ wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_REFRPANEL,    _("Refresh the list"));
  PopupMenu(popup_menu, event.GetPosition());
  delete popup_menu;
}

void UserDataGrid::OnxCommand( wxCommandEvent& event )
// Command on client list.
{ unsigned long usernum;
  switch (event.GetId())
  { case CPM_CLIENT_BREAK:
      if (!dgt->GetValue(row_of_click, 1).ToULong(&usernum)) break;  // the client number
      if (cd_Break_user(cdp, usernum)) cd_Signalize(cdp);
      refresh(RESET_CURSOR);
      break;
    case CPM_CLIENT_KILL:
      if (!dgt->GetValue(row_of_click, 1).ToULong(&usernum)) break;  // the client number
      if (cd_Kill_user(cdp, usernum)) cd_Signalize(cdp);
      refresh(RESET_CURSOR);
      break;
    case CPM_REFRPANEL:
      refresh(RESET_CURSOR);
      break;
    default:
      event.Skip();  break;
  }
}

BEGIN_EVENT_TABLE(UserDataGrid, DataGrid)
  EVT_GRID_CELL_RIGHT_CLICK(UserDataGrid::OnCellRightClick)
  EVT_GRID_LABEL_RIGHT_CLICK(UserDataGrid::OnLabelRightClick)
  //EVT_RIGHT_DOWN(UserDataGrid::OnOuterRightClick) -- message not delivered
  EVT_MENU(-1, UserDataGrid::OnxCommand)
END_EVENT_TABLE()

//////////////////////////////////// lock list ///////////////////////////////////

class LockDataGrid : public DataGrid
{ int row_of_click;
 public:
  LockDataGrid(xcdp_t xcdpIn) : DataGrid(xcdpIn) {}
  void OnCellRightClick(wxGridEvent & event);
  void OnLabelRightClick(wxGridEvent & event);
  void OnxCommand( wxCommandEvent& event );
  DECLARE_EVENT_TABLE()
};

void LockDataGrid::OnCellRightClick(wxGridEvent & event)
{ row_of_click = event.GetRow();
  ClearSelection();
  SetGridCursor(event.GetRow(), event.GetCol());
  BOOL adm=cd_Am_I_config_admin(cdp);
  wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_REFRPANEL,   _("Refresh the list"));
  popup_menu->AppendSeparator();
  popup_menu->Append(CPM_LOCK_REMOVE, _("Remove the lock"));
  popup_menu->Enable(CPM_LOCK_REMOVE,  adm==TRUE);
  PopupMenu(popup_menu, event.GetPosition());
  delete popup_menu;
}

void LockDataGrid::OnLabelRightClick(wxGridEvent & event)
{ wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_REFRPANEL,    _("Refresh the list"));
  PopupMenu(popup_menu, event.GetPosition());
  delete popup_menu;
}

void LockDataGrid::OnxCommand( wxCommandEvent& event )
// Command on client list.
{ unsigned long lval;
  switch (event.GetId())
  { case CPM_LOCK_REMOVE:
    { lockinfo itemdata;  tobjname table_name, schema_name, user_name;
     // get lock information:
      strcpy(schema_name, dgt->GetValue(row_of_click, 0).mb_str(*cdp->sys_conv));
      strcpy(table_name,  dgt->GetValue(row_of_click, 1).mb_str(*cdp->sys_conv));
      if (!*table_name) { wxBell();  break; } // page lock
      if (cd_Find_prefixed_object(cdp, schema_name, table_name, CATEG_TABLE, &itemdata.tabnum))
        { cd_Signalize(cdp);  break; }
      if (!dgt->GetValue(row_of_click, 2).ToULong(&lval)) break;  // the record number
      itemdata.recnum = lval;
      strcpy(user_name,  dgt->GetValue(row_of_click, 5).mb_str(*cdp->sys_conv));
      if (cd_Find2_object(cdp, user_name, NULL, CATEG_USER, &itemdata.usernum))
        { cd_Signalize(cdp);  break; }
      int locktype; // cannot use GetValue, translated
      cd_Read(cdp, inst->dt->cursnum, row_of_click, 4, NULL, (tptr)&locktype);
      itemdata.locktype=locktype;
      if (cd_Remove_lock(cdp, &itemdata)) cd_Signalize(cdp);
      refresh(RESET_CURSOR);
      break;
    }
    case CPM_REFRPANEL:
      refresh(RESET_CURSOR);
      break;
    default:
      event.Skip();  break;
  }
}

BEGIN_EVENT_TABLE(LockDataGrid, DataGrid)
  EVT_GRID_CELL_RIGHT_CLICK(LockDataGrid::OnCellRightClick)
  EVT_GRID_LABEL_RIGHT_CLICK(LockDataGrid::OnLabelRightClick)
  //EVT_RIGHT_DOWN(UserDataGrid::OnOuterRightClick) -- message not delivered
  EVT_MENU(-1, LockDataGrid::OnxCommand)
END_EVENT_TABLE()

//////////////////////////////////// trace list ///////////////////////////////////
#define TRACE_LIST_COL_LOGNAME   0
#define TRACE_LIST_COL_SITUATION 1
#define TRACE_LIST_COL_CONTEXT   2
#define TRACE_LIST_COL_USER      3
#define TRACE_LIST_COL_TABLE     4

class TraceDataGridTable : public DataGridTable
{ bool writing_value;
 public:
  void read_trace(int row, sig32 & situation, wxChar * user_name, char * table_name, wxChar * log_name, sig32 & context);
  TraceDataGridTable(DataGrid * dgIn, cdp_t cdpIn, view_dyn * instIn) : DataGridTable(dgIn, cdpIn, instIn)
    { writing_value=false;  }
  void SetValue(int row, int col, const wxString& value);
  wxString GetValue(int row, int col);
};

class TraceDataGrid : public DataGrid
{ int row_of_click;
  DataGridTable * create_grid_table(xcdp_t xcdp, view_dyn * inst)
    { return new TraceDataGridTable(this, xcdp->get_cdp(), inst); }
 public:
  TraceDataGrid(xcdp_t xcdpIn) : DataGrid(xcdpIn) {}
  virtual bool open(wxWindow* parent, t_global_style my_style); 
  void prep(void);
  void set_log_choice(tcursnum curs);
  void update_editability(void);
  void OnCellRightClick(wxGridEvent & event);
  void OnLabelRightClick(wxGridEvent & event);
  void OnxCommand( wxCommandEvent& event );
  virtual void delete_selected_records(void);
  DECLARE_EVENT_TABLE()
};

// Special handling of user_name and log_name: These names are in SQL server's charset but empty strings are replaced by
//  readable string in the LOCAL charset. This implies problems in encoding. Solution is to use the Unicode encoding.

void update_trace(cdp_t cdp, int situation, const wxChar * user_name, const char * table_name, const wxChar * log_name, int context)
{ char cmd[200];
  sprintf(cmd, "call _sqp_trace(%u,\'%s\',\'%s\',\'%s\',%d)", situation, 
    !wxString(_("All users")).Cmp(user_name) ? "" : (const char *)wxString(user_name).mb_str(*cdp->sys_conv), 
    table_name, 
    !wxString(_("Basic log")).Cmp(log_name)  ? "" : (const char *)wxString(log_name) .mb_str(*cdp->sys_conv), 
    context);
  if (cd_SQL_execute(cdp, cmd, NULL)) cd_Signalize(cdp);
}

void TraceDataGridTable::read_trace(int row, sig32 & situation, wxChar * user_name, char * table_name, wxChar * log_name, sig32 & context)
{ sview_dyn * sinst = inst->subinst;
  wcscpy(user_name,  GetValue(row, TRACE_LIST_COL_USER   ). c_str());
  wcscpy(log_name,   GetValue(row, TRACE_LIST_COL_LOGNAME). c_str());
  strcpy(table_name, GetValue(row, TRACE_LIST_COL_TABLE  ).mb_str(*xcdp->get_cdp()->sys_conv));
  int binary_size;  
  situation=0;
  inst->VdGetNativeVal(sinst->sitems[TRACE_LIST_COL_SITUATION].itm->ctId, row, 0, &binary_size, (char*)&situation);
  context=0;
  inst->VdGetNativeVal(sinst->sitems[TRACE_LIST_COL_CONTEXT  ].itm->ctId, row, 0, &binary_size, (char*)&context);
}

wxString TraceDataGridTable::GetValue(int row, int col)
{ wxString val = DataGridTable::GetValue(row, col);
  if (col==TRACE_LIST_COL_USER)
    if (val.IsEmpty()) return _("All users");
  if (col==TRACE_LIST_COL_LOGNAME)
    if (val.IsEmpty()) return _("Basic log");
  return val;
}

void TraceDataGridTable::SetValue(int row, int col, const wxString& value)
{ if (writing_value) return; // prevents recursion: set_fcursir calls remap_reset calls DeleteRows which calls SetValue for the open cell editor -> infinite nesting
  writing_value=true;
  sview_dyn * sinst = inst->subinst;
  inst->sel_rec=row;
 // get current values:
  sig32 situation;  wxChar log_name[OBJ_NAME_LEN+1], user_name[OBJ_NAME_LEN+1];  
  char table_name[2*OBJ_NAME_LEN+6];  sig32 context;
  read_trace(row, situation, user_name, table_name, log_name, context);
  bool fictive = situation==(sig32)0x80000000;
  if (fictive) context=1; 
 // write changes:
  switch (col)
  { case TRACE_LIST_COL_CONTEXT:
    { t_ctrl * itm = sinst->sitems[col].itm;
      const wxChar * enumdef = get_enum_def(inst, itm);
      const void * val = dg->enum_reverse(enumdef, value);
      if (!val) goto ex;  // internal error
      context = *(int*)val;
      if (!fictive) update_trace(xcdp->get_cdp(), situation, user_name, table_name, log_name, context);
      break;
    }
    case TRACE_LIST_COL_SITUATION:
    { t_ctrl * itm = sinst->sitems[col].itm;
      const wxChar * enumdef = get_enum_def(inst, itm);
      const void * val = dg->enum_reverse(enumdef, value);
      if (!val) goto ex;  // internal error
      if (!fictive) update_trace(xcdp->get_cdp(), situation, user_name, table_name, log_name, 0);
      situation = *(int*)val;
      update_trace(xcdp->get_cdp(), situation, user_name, table_name, log_name, context);
      break;
    }
    case TRACE_LIST_COL_USER:
      if (!fictive) update_trace(xcdp->get_cdp(), situation, user_name, table_name, log_name, 0);
      update_trace(xcdp->get_cdp(), situation, value.c_str(), table_name, log_name, context);
      break;
    case TRACE_LIST_COL_TABLE:
      if (!fictive) update_trace(xcdp->get_cdp(), situation, user_name, table_name, log_name, 0);
      update_trace(xcdp->get_cdp(), situation, user_name, value.mb_str(*xcdp->get_cdp()->sys_conv), log_name, context);
      break;
    case TRACE_LIST_COL_LOGNAME:
      if (!fictive) update_trace(xcdp->get_cdp(), situation, user_name, table_name, log_name, 0);
      update_trace(xcdp->get_cdp(), situation, user_name, table_name, value.c_str(), context);
      break;

  }
 // reset
  ((TraceDataGrid*)dg)->refresh(RESET_CURSOR);
  ((TraceDataGrid*)dg)->update_editability();
//  cd_Rec_cnt(cdp, inst->dt->cursnum, &inst->dt->fi_recnum);
  if (fictive)
  { view_recnum(dg->inst);
    dg->SetGridCursor(dg->inst->dt->fi_recnum-2, dg->GetGridCursorCol());  
    dg->MakeCellVisible(dg->GetGridCursorRow(), dg->GetGridCursorCol());
  }
 ex:
  writing_value=false;
}

bool TraceDataGrid::open(wxWindow * parent, t_global_style my_style)
{
 // remove the NO_EDIT flags:
  create_column_description();
  for (int col = 0;  col < sinst->ItemCount;  col++)
  { t_ctrl * itm = sinst->sitems[col].itm;
    atr_info * catr = inst->dt->attrs+itm->column_number;  // may be invalid when column_number==-1
    catr->flags &= ~CA_NOEDIT;
  }
  return DataGrid::open(parent, my_style);
}

void TraceDataGrid::update_editability(void)
// Must be called after every change in the trace request list!
{
  for (int row=0;  row<inst->dt->rm_int_recnum;  row++)
  { int binary_size;  
    sig32 situation=0;
    inst->VdGetNativeVal(sinst->sitems[TRACE_LIST_COL_SITUATION].itm->ctId, row, 0, &binary_size, (char*)&situation);
    SetReadOnly(row, TRACE_LIST_COL_LOGNAME, false);
    SetReadOnly(row, TRACE_LIST_COL_TABLE,  !TRACE_SIT_TABLE_DEPENDENT(situation));
    SetReadOnly(row, TRACE_LIST_COL_USER,    TRACE_SIT_USER_INDEPENDENT(situation));
    SetReadOnly(row, TRACE_LIST_COL_CONTEXT, false);
  }
  SetReadOnly(inst->dt->fi_recnum-1, TRACE_LIST_COL_LOGNAME, true);
  SetReadOnly(inst->dt->fi_recnum-1, TRACE_LIST_COL_TABLE,   true);
  SetReadOnly(inst->dt->fi_recnum-1, TRACE_LIST_COL_USER,    true);
  SetReadOnly(inst->dt->fi_recnum-1, TRACE_LIST_COL_CONTEXT, true);
}

void TraceDataGrid::OnCellRightClick(wxGridEvent & event)
{ row_of_click = event.GetRow();
  ClearSelection();
  SetGridCursor(event.GetRow(), event.GetCol());
  BOOL adm=cd_Am_I_config_admin(xcdp->get_cdp());
  wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_REFRPANEL,    _("Refresh the list"));
  popup_menu->AppendSeparator();
  popup_menu->Append(CPM_TRACE_REMOVE, _("Remove the trace request"));
  popup_menu->Enable(CPM_TRACE_REMOVE, (adm || cd_Am_I_security_admin(xcdp->get_cdp())) && row_of_click<inst->dt->fi_recnum-1);
  popup_menu->Append(CPM_NEW_LOG,      _("Create a new temporary log"));
  popup_menu->Enable(CPM_NEW_LOG,      adm==TRUE);
  PopupMenu(popup_menu, event.GetPosition());
  delete popup_menu;
}

void TraceDataGrid::OnLabelRightClick(wxGridEvent & event)
{ wxMenu * popup_menu = new wxMenu;
  BOOL adm=cd_Am_I_config_admin(xcdp->get_cdp());
  popup_menu->Append(CPM_REFRPANEL,   _("Refresh the list"));
  popup_menu->AppendSeparator();
  popup_menu->Append(CPM_NEW_LOG,     _("Create a new temporary log"));
  popup_menu->Enable(CPM_NEW_LOG,     adm==TRUE);
  PopupMenu(popup_menu, event.GetPosition());
  delete popup_menu;
}

void TraceDataGrid::OnxCommand( wxCommandEvent& event )
// Command on client list.
{ 
  switch (event.GetId())
  { case CPM_TRACE_REMOVE:   // row_of_click is defined
    { sig32 situation;  char table_name[2*OBJ_NAME_LEN+6];  sig32 context;
      wxChar log_name[OBJ_NAME_LEN+1], user_name[OBJ_NAME_LEN+1];
      if (row_of_click==inst->dt->fi_recnum-1)
        { wxBell();  break; }
      ((TraceDataGridTable*)dgt)->read_trace(row_of_click, situation, user_name, table_name, log_name, context);
      update_trace(xcdp->get_cdp(), situation, user_name, table_name, log_name, 0);
      refresh(RESET_CURSOR);
      update_editability();
      break;
    }
    case CPM_REFRPANEL:
      refresh(RESET_CURSOR);
      update_editability();
      break;
    case CPM_NEW_LOG:
    { NewLogDlg dlg(xcdp->get_cdp());
      dlg.Create(this);
      if (dlg.ShowModal() == wxID_OK)  // update log list on pages and in the choice
      { MonitorNotebook * monit = wxGetApp().frame->monitor_window;
        tcursnum curs = monit->get_log_list();
        if (curs==NOCURSOR) cd_Signalize(xcdp->get_cdp());
        else
        { monit->relist_logs(curs);
          set_log_choice(curs);
          cd_Close_cursor(xcdp->get_cdp(), curs);
        }
      }
      break;
    }
    default:
      event.Skip();  break;
  }
}

void TraceDataGrid::set_log_choice(tcursnum curs)
{ trecnum cnt, r;  int count;
 // create list of logs:
  cd_Rec_cnt(xcdp->get_cdp(), curs, &cnt);
  wxString * keys = new wxString[cnt];  count = 0;
  for (r=0;  r<cnt;  r++)
  { tobjname log_name;  
    if (!cd_Read(xcdp->get_cdp(), curs, r, 1, NULL, log_name))
      keys[count++] = *log_name ? wxString(log_name, *xcdp->get_cdp()->sys_conv) : wxString(_("Basic log"));
  }
 // create the choice editor:
  wxGridCellAttr * col_attr = new wxGridCellAttr;
  wxGridCellODChoiceEditor * combo_editor = new wxGridCellODChoiceEditor(count, keys, false);
  col_attr->SetEditor(combo_editor);
  delete [] keys;
  SetColAttr(TRACE_LIST_COL_LOGNAME, col_attr);
}

void TraceDataGrid::prep(void)
{ char query[OBJ_NAME_LEN+100];  tcursnum curs;  trecnum cnt, r;
 // create list of users:
  sprintf(query, "select logname from USERTAB where category=chr(%u)", CATEG_USER);
  if (!cd_Open_cursor_direct(xcdp->get_cdp(), query, &curs)) 
  { cd_Rec_cnt(xcdp->get_cdp(), curs, &cnt);
    wxString * keys;  int count;   
    keys = new wxString[cnt+1];  count = 0;
    keys[count++] = _("All users");   // all users
    for (r=0;  r<cnt;  r++)
    { tobjname user_name;  
      if (!cd_Read(xcdp->get_cdp(), curs, r, 1, NULL, user_name))
        keys[count++] = wxString(user_name, *xcdp->get_cdp()->sys_conv);
    }
    cd_Close_cursor(xcdp->get_cdp(), curs);
   // set choice:
    wxGridCellAttr * col_attr = new wxGridCellAttr;
	wxGridCellODChoiceEditor * combo_editor = new wxGridCellODChoiceEditor(count, keys, false);
	col_attr->SetEditor(combo_editor);
	delete [] keys;
	SetColAttr(TRACE_LIST_COL_USER, col_attr);
  }
 // fictive record:
  inst->stat->vwHdFt &= ~NO_VIEW_FIC;
  inst->dt->fi_recnum++;
  AppendRows(1);
 // set initial editability:
  update_editability();
}

void TraceDataGrid::delete_selected_records(void)
{ wxArrayInt selrows = MyGetSelectedRows();
  if (selrows.GetCount()!=1)
    { error_box(_("A single trace request must be selected"));  return; }
 // deleting:
  row_of_click = selrows[0];
  wxCommandEvent event;  event.SetId(CPM_TRACE_REMOVE);
  OnxCommand(event);
}

BEGIN_EVENT_TABLE(TraceDataGrid, DataGrid)
  EVT_GRID_CELL_RIGHT_CLICK(TraceDataGrid::OnCellRightClick)
  EVT_GRID_LABEL_RIGHT_CLICK(TraceDataGrid::OnLabelRightClick)
  //EVT_RIGHT_UP(func)
  EVT_MENU(-1, TraceDataGrid::OnxCommand)
END_EVENT_TABLE()

/////////////////////////////// monitor management /////////////////////////////////////
void MonitorNotebook::reset_main_info(void)
{ MonitorMainDlg * main = (MonitorMainDlg *)GetPage(MONITOR_PAGE_MAIN);
  if (cdp) 
  { int sel = main->mServer->FindString(wxString(cdp->locid_server_name, *wxConvCurrent));
    if (sel!=-1 && sel!=main->mServer->GetSelection())
      main->mServer->SetSelection(sel);
    tobjname user_name;
    if (cd_Read(cdp, USER_TABLENUM, cdp->logged_as_user, OBJ_NAME_ATR, NULL, user_name))
      *user_name=0;  // important when server closed the connection
    main->mUser->SetValue(wxString(user_name, *cdp->sys_conv));
    bool conf_admin = cd_Am_I_config_admin(cdp)==TRUE;
    main->mConfAdm->SetLabel(conf_admin ? _("User is a configuration administrator") : _("User is NOT a configuration administrator"));
    main->mDataAdm->SetLabel(cd_Am_I_db_admin(cdp) ? _("User is a data administrator") : _("User is NOT a data administrator"));
    main->mSecurAdm->SetLabel(cd_Am_I_security_admin(cdp) ? _("User is a security administrator") : _("User is NOT a security administrator"));
    main->Layout(); // must recalc layout of children because its contents has changed, but must not call Fit() - size of the dialog is defined by the notebook
    main->mAccess->Enable(conf_admin);
    main->mBackground->Enable(conf_admin);
    uns32 server_locked, back_oper;
    if (cd_Get_server_info(cdp, OP_GI_LOGIN_LOCKS, &server_locked, sizeof(server_locked))) server_locked=0;
    main->mAccess->SetValue(server_locked!=0);
    if (cd_Get_server_info(cdp, OP_GI_BACKGROUD_OPER, &back_oper, sizeof(back_oper))) back_oper=0;
    main->mBackground->SetValue(back_oper!=0);
    main->mRefresh->Enable();
    main->mRefreshNow->Enable();
  }
  else 
  { main->mServer->SetSelection(0);
    main->mUser->SetValue(wxEmptyString);
    main->mConfAdm->SetLabel(wxEmptyString);
    main->mDataAdm->SetLabel(wxEmptyString);
    main->mSecurAdm->SetLabel(wxEmptyString);
    main->mAccess->Disable();
    main->mBackground->Disable();
    main->mRefresh->Disable();
    main->mRefreshNow->Disable();
  }
}

void MonitorNotebook::AttachToServer(cdp_t cdpIn)
{ if (cdpIn==cdp) return;  // is attached
  Freeze();
  if (cdp) Stop_monitoring();
 // open grids:
  cdp=cdpIn;
  reset_main_info();
 // create monitor pages in the numbering order: MONITOR_PAGE_MAIN, MONITOR_PAGE_CLIENTS, MONITOR_PAGE_LOCKS, MONITOR_PAGE_TRACE, MONITOR_PAGE_INTERN, etc
  ClientsThreadsDlg * ctd;
  ctd = new ClientsThreadsDlg();
  ctd->Create(this, MONITOR_PAGE_CLIENTS);
  if (ctd->open_monitor_page(cdp, new UserDataGrid(cdp), user_query, user_form))   
    AddPage(ctd, _("Clients and Threads"), false, 1);
  else ctd->Destroy();

  ctd = new ClientsThreadsDlg();
  ctd->Create(this, MONITOR_PAGE_LOCKS);
  if (ctd->open_monitor_page(cdp, new LockDataGrid(cdp), locks_query, locks_form))
    AddPage(ctd, _("Locks"), false, 2);
  else ctd->Destroy();

  ctd = new ClientsThreadsDlg();
  ctd->Create(this, MONITOR_PAGE_TRACE);
  TraceDataGrid * tdg = new TraceDataGrid(cdp);
  if (ctd->open_monitor_page(cdp, tdg, trace_query, trace_form))
    { AddPage(ctd, _("Trace"), false, 3);  tdg->prep(); }
  else 
    { ctd->Destroy();  tdg=NULL; /* deleted inside*/ }
 // "internals" page:
  InternProperties * int_prop = new InternProperties(cdp);  // will be owned PropertyGridWin
  PropertyGridWin * grid = new PropertyGridWin(int_prop, this, MONITOR_PAGE_INTERN);
  int_prop->create(grid, new InternPropertyGridTable(int_prop));
  int_prop->load_and_show();
  AddPage(grid, _("Internals"), false, 4);
  monitor_fixed_pages=(int)GetPageCount();
 // create the log list and use it twice:
  tcursnum curs = get_log_list();
  if (curs==NOCURSOR) cd_Signalize(cdp);
  else
  { relist_logs(curs);
    if (tdg) tdg->set_log_choice(curs);
    cd_Close_cursor(cdp, curs);
  }
  Thaw();
  GetPage(0)->Refresh();  // otherwise not drawn
 // start timer if auto-refresh is specified:
  MonitorMainDlg * main = (MonitorMainDlg *)GetPage(MONITOR_PAGE_MAIN);
  set_monitor_refresh(main->mRefresh->GetValue());
}

tcursnum MonitorNotebook::get_log_list(void)
{ tcursnum curs;
  if (cd_Open_cursor_direct(cdp, "select log_name, log_message from _iv_recent_log where log_name>=\'\'", &curs))
    curs=NOCURSOR;
  return curs;
}

void MonitorNotebook::refresh_all(void)
// Refreshes all except trace list.
{ ClientsThreadsDlg * ctd;
  if (!cdp) return;  // no action if not monitoring
  reset_main_info();
  ctd = (ClientsThreadsDlg *)GetPage(MONITOR_PAGE_CLIENTS);
  if (ctd)
  { UserDataGrid * grid = (UserDataGrid*)ctd->FindWindow(CD_THR_PLACEHOLDER);
    if (grid) grid->refresh(RESET_CURSOR);
  }
  ctd = (ClientsThreadsDlg *)GetPage(MONITOR_PAGE_LOCKS);
  if (ctd)
  { LockDataGrid * grid = (LockDataGrid*)ctd->FindWindow(CD_THR_PLACEHOLDER);
    if (grid) grid->refresh(RESET_CURSOR);
  }
  PropertyGridWin * grid = (PropertyGridWin *)GetPage(MONITOR_PAGE_INTERN);
  if (grid)
    grid->props->load_and_show();
 // refresh logs:
  for (int pg=monitor_fixed_pages;  pg<GetPageCount();  pg++) 
  { MonitorLogDlg * log = (MonitorLogDlg*)GetPage(pg);
    log->refresh_log();
  }
}

/////////////////////////////////// property grids ////////////////////////////////////////
PropertyGrid::~PropertyGrid(void)
{
  for (int i=0;  i<items.GetCount();  i++)
  { PropertyItem * item = items[i];
    delete item;
  }
}

wxString PropertyGridTable::GetColLabelValue( int col )
{ return !col ? _("Property") : _("Value"); }

int PropertyGridTable::GetNumberRows()
{ return (int)grid->items.GetCount(); }

int PropertyGridTable::GetNumberCols()
{ return 2; }

bool PropertyGridTable::IsEmptyCell( int row, int col )
{ return false; }
                                            
wxString PropertyGridTable::GetValue( int row, int col )
{ return col ? grid->items[row]->loaded_value : wxString(grid->items[row]->name, *wxConvCurrent); }

bool PropertyGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{ if (typeName == wxGRID_VALUE_STRING) return true;
  return false;
}

void PropertyGridTable::SetValue( int row, int col, const wxString& value )
{ if (grid->items[row]->loaded_value != value)
  { if (grid->items[row]->type==PROP_TP_NUM) // patching error in wxGridCellNumberEditor: is user clears the contents, decimal representation of NONEINTEGER is stored here
    { long lval;
      if (!value.ToLong(&lval)) grid->items[row]->loaded_value = wxEmptyString;
      else if (lval<((PropertyItemNum*)grid->items[row])->minval || lval>((PropertyItemNum*)grid->items[row])->maxval)
        grid->items[row]->loaded_value = wxEmptyString;
      else
        grid->items[row]->loaded_value = value;
    }
    else
      grid->items[row]->loaded_value = value;
    grid->items[row]->changed=true;
  }
}

BOOL cd_Get_property_num(cdp_t cdp, const char * owner, const char * name, sig32 * value)
{ char buf[13];
  if (cd_Get_property_value(cdp, owner, name, 0, buf, sizeof(buf)) || !*buf) return TRUE;
  return !str2int(buf, value);
}

BOOL cd_Set_property_num(cdp_t cdp, const char * owner, const char * name, sig32 value)
{ char buf[13];
  int2str(value, buf);
  return cd_Set_property_value(cdp, owner, name, 0, buf);
}


void PropertyGrid::create(wxGrid * gridIn, PropertyGridTable * grid_table)
{ int row;
  grid=gridIn;
 // creating the table:
  if (grid_table==NULL) grid_table = new PropertyGridTable(this);
  grid->SetTable(grid_table, TRUE);
  grid->BeginBatch();
  grid->EnableDragRowSize(false);
  grid->SetRowLabelSize(0);
  grid->SetColLabelSize(default_font_height);
  grid->SetDefaultRowSize(default_font_height);
  grid->SetLabelFont(system_gui_font);
  DefineColSize(grid, 0, 300);
  DefineColSize(grid, 1, 70);
  bool I_am_sec_adm  = cd_Am_I_security_admin(cdp)==TRUE;
  bool I_am_conf_adm = cd_Am_I_config_admin(cdp)  ==TRUE;
  for (row=0;  row<items.GetCount();  row++)
  { PropertyItem * item = items[row];
   // create item labels:
    //grid->SetCellValue(row, 0, item->name);
    grid->SetReadOnly(row, 0);
   // row attributes:
    if (item->secur_admin || !I_am_conf_adm) 
    { wxGridCellAttr * row_attr = new wxGridCellAttr;
      if (item->secur_admin)
        row_attr->SetBackgroundColour(wxColour(255, 0xc0, 0xc0));
      if (item->secur_admin ? !I_am_sec_adm : !I_am_conf_adm)
        row_attr->SetReadOnly();
      grid->SetRowAttr(row, row_attr);
    }
   // create item value cell:
    switch (item->type)
    { case PROP_TP_BOOL:
      { wxGridCellBoolEditor * bool_editor = new wxGridCellBoolEditor();
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>=8
        bool_editor->UseStringValues(wxT("1"), wxT("0"));
#endif
        grid->SetCellEditor(row, 1, bool_editor);  // takes the ownership
        wxGridCellBoolRenderer * bool_renderer = new wxGridCellBoolRenderer();
        grid->SetCellRenderer(row, 1, bool_renderer);  // takes the ownership
        break;
      }
      case PROP_TP_STRING:
        grid->SetCellEditor(row, 1, make_limited_string_editor(((PropertyItemString*)item)->max_length));  // takes the ownership
        break;
      case PROP_TP_NUM:
      { wxGridCellNumberEditor * num_editor = new wxGridCellNumberEditor(((PropertyItemNum*)item)->minval, ((PropertyItemNum*)item)->maxval);
        grid->SetCellEditor(row, 1, num_editor);  // takes the ownership
        break;
      }
      case PROP_TP_COMBO:
      { wxGridCellODChoiceEditor * ch_editor = new wxGridCellODChoiceEditor(0, NULL, false);
        ch_editor->SetParameters(((PropertyItemCombo*)item)->choice);
        grid->SetCellEditor(row, 1, ch_editor);
        break;
      }
    }
  }
  grid->EndBatch();
}

wxGridCellTextEditor * make_limited_string_editor(int width)
{ wxGridCellTextEditor * string_editor = new wxGridCellTextEditor();
  wxString max_len_str;  
  max_len_str.Printf(wxT("%u"), width);
  string_editor->SetParameters(max_len_str);
  return string_editor;
}

void PropertyGrid::load_and_show(void)
{ int row;
  for (row=0;  row<items.GetCount();  row++)
  { PropertyItem * item = items[row];
    if (item->propname)
    { char buf[MAX_PATH+1];
      if (cd_Get_property_value(cdp, sqlserver_owner, item->propname, 0, buf, sizeof(buf))) *buf=0;
      switch (item->type)
      { case PROP_TP_STRING:
        case PROP_TP_COMBO:
          item->loaded_value = wxString(buf, *cdp->sys_conv);
          break;
        case PROP_TP_NUM:
          item->loaded_value = wxString(buf, *cdp->sys_conv);
          //str2int(buf, &((PropertyItemNum*)item)->loaded_value);  // NONEINTEGER if empty
          break;
        case PROP_TP_BOOL:
          item->loaded_value = wxString(buf, *cdp->sys_conv);
        //{ sig32 ival;
        //  if (!str2int(buf, &ival) || ival<0 || ival>1) 
        //    { ival=0;  strcpy(buf, "0"); }
        //  ((PropertyItemBool*)item)->loaded_value = ival!=0;
          break;
        //}
      } // switch
      grid->SetCellValue(row, 1, item->loaded_value);
      item->changed=false;
    } // linked with a property
  }
}

bool PropertyGrid::save(void)
{ int row;
  if (grid->IsCellEditControlEnabled()) grid->DisableCellEditControl();
  for (row=0;  row<items.GetCount();  row++)
  { PropertyItem * item = items[row];
    if (item->changed && item->propname)
    { wxString newval = grid->GetCellValue(row, 1);
      switch (item->type)
      { case PROP_TP_STRING:
          //if (((PropertyItemString*)item)->loaded_value == newval) continue;
          break;
        case PROP_TP_NUM:
        { //long newlval;
          //if (!newval.ToLong(&newlval)) continue;  // was empty and remained empty
          //if (((PropertyItemNum*)item)->loaded_value == newlval) continue;
          break;
        }
        case PROP_TP_BOOL:
        { //long newlval;
          //if (!newval.ToLong(&newlval)) continue;  // was empty and remained empty
          //if (((PropertyItemBool*)item)->loaded_value == (newlval!=0)) continue;
          if (newval.IsEmpty()) newval=wxT("0");  // sometimes returns empty string when unchecked
          break;
        }
        case PROP_TP_COMBO:
          break;
      }
      if (cd_Set_property_value(cdp, sqlserver_owner, item->propname, 0, newval.mb_str(*cdp->sys_conv)))
        { cd_Signalize(cdp);  return false; }
    }
  }
  return true;
}

class SecurProperties : public PropertyGrid
{public:
  SecurProperties(cdp_t cdpIn);
  void create(wxGrid * gridIn, PropertyGridTable * grid_table = NULL);
};

SecurProperties::SecurProperties(cdp_t cdpIn) : PropertyGrid(cdpIn)
{ 
 // insert items:
  items.Add(new PropertyItemBool  (_("Anonymous access is disabled"), "DisableAnonymous", true));  // SecurProperties::create uses the fact that this item is the 1st one
  items.Add(new PropertyItemBool  (_("Password never expires"), "UnlimitedPasswords", true));
  items.Add(new PropertyItemNum   (_("Minimum password length"), "MinPasswordLen", true, 0, 200));
  items.Add(new PropertyItemNum   (_("Passwords expire every (days, 0=never)"), "PasswordExpiration", true, 0, 200));
  items.Add(new PropertyItemString(_("Domain that validates logins"), "ThrustedDomain", true, 100));
  items.Add(new PropertyItemBool  (_("Create Domain users on the server"), "CreateDomainUsers", true));
  items.Add(new PropertyItemBool  (_("Encrypt network communication"), "ReqCommEnc", true));
  items.Add(new PropertyItemBool  (_("Configuration administrators can manage the CONFIG_ADMIN group"), "ConfAdmsOwnGroup", true));
  items.Add(new PropertyItemBool  (_("Configuration administrators can manage user groups"), "ConfAdmsUserGroups", true));
  //items.Add(new PropertyItemBool  (_("Enable HTTP access"), "HTTPTunnel", true));
  //items.Add(new PropertyItemNum   (_("Open HTTP access on port"), "HTTPTunnelPort", true, 1, 65534));
  wxString user_names = wxT(",");
  bool first=true;
  void * en = get_object_enumerator(cdp, CATEG_USER, "");
  tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
  while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
    if (objnum!=ANONYMOUS_USER)
    { if (first) first=false;  else user_names=user_names+wxT(",");
      user_names=user_names+wxString(name, *cdp->sys_conv);
    }  
  free_object_enumerator(en);

  items.Add(new PropertyItemCombo (_("Restrict HTTP access to the user account"), "HTTPUser", true, user_names));
  //items.Add(new PropertyItemBool  (_("Disable anonymous web access to the server"), "DisableHTTPAnonymous", true));
}

void SecurProperties::create(wxGrid * gridIn, PropertyGridTable * grid_table)
{ PropertyGrid::create(gridIn, grid_table);
 // when logged as anonymous, disable the checkbox of controlling the anonymous logging:
  if (cdp->logged_as_user==ANONYMOUS_USER)
    gridIn->SetReadOnly(0, 1);
}

class OperProperties : public PropertyGrid
{public:
  OperProperties(cdp_t cdpIn);
};

OperProperties::OperProperties(cdp_t cdpIn) : PropertyGrid(cdpIn)
{ 
  //items.Add(new PropertyItemNum   (_("TCP/IP port number (default 5001)"), "IPPort", false, 5001, 65530));
  items.Add(new PropertyItemNum   (_("Disconnect logged client when idle for (seconds)"), "KillTimeLogged", false, 0, 999999));
  items.Add(new PropertyItemNum   (_("Disconnect not logged client when idle for (seconds)"), "KillTimeNotLogged", false, 0, 999999));
  items.Add(new PropertyItemBool  (_("Use 2-phase commit"), "SecureTransactions", false));
  items.Add(new PropertyItemBool  (_("Flush changes to disk on every commit"), "FlushOnCommit", false));
  items.Add(new PropertyItemNum   (_("Flush changes to disk every X seconds (0=no flush)"), "FlushPeriod", false, 0, 999999));
  items.Add(new PropertyItemNum   (_("Wait for a lock (in tenths of a second, -1=infinite)"), "LockWaitingTimeout", false, -1, 999999));
  items.Add(new PropertyItemNum   (_("Database file cache (KB)"), "Frame Space", false, 150, 40000));
#if WBVERS<1100
  items.Add(new PropertyItemBool  (_("Minimize icon to the System tray"), "Tray Icon", false));
#endif  
  items.Add(new PropertyItemBool  (_("Close the database file when the server is idle"), "CloseFileWhenIdle", false));
  items.Add(new PropertyItemNum   (_("Log cache size (bytes)"), "TraceLogCacheSize", false, 500, 30000));
  items.Add(new PropertyItemBool  (_("Create a new file daily for the Basic log"), "DailyBasicLog", false));
  items.Add(new PropertyItemBool  (_("Create a new file daily for other logs"), "DailyUserLogs", false));
#ifdef EXTFT
  items.Add(new PropertyItemString(_("Directory to store external fulltext index"), "ExtFulltextDir", false, 254-(2*OBJ_NAME_LEN+1)-6));
#endif
  items.Add(new PropertyItemString(_("Directory to store external LOBs"), "ExtLobDir", false, 230));
}

class CompatPropertyGridTable : public PropertyGridTable
{ virtual int GetNumberCols();
  virtual wxString GetValue(int row, int col);
  virtual wxString GetColLabelValue(int col);
 public:
  CompatPropertyGridTable(PropertyGrid * gridIn) : PropertyGridTable(gridIn) { }
};

wxString CompatPropertyGridTable::GetColLabelValue(int col)
{ return !col ? _("Option") : col==1 ? _("Value") : _("Option identifier"); }

int CompatPropertyGridTable::GetNumberCols()
{ return 3; }

const wxChar * compat_ids[] = {
wxT("SQLOPT_NULLEQNULL"),
wxT("SQLOPT_NULLCOMP"),
wxT("SQLOPT_RD_PRIVIL_VIOL"),
wxT("SQLOPT_MASK_NUM_RANGE"),
wxT("SQLOPT_MASK_INV_CHAR"),
wxT("SQLOPT_MASK_RIGHT_TRUNC"),
wxT("SQLOPT_EXPLIC_FREE"),
wxT("SQLOPT_OLD_ALTER_TABLE"),
wxT("SQLOPT_DUPLIC_COLNAMES"),
wxT("SQLOPT_USER_AS_SCHEMA"),
wxT("SQLOPT_DISABLE_SCALED"),
wxT("SQLOPT_ERROR_STOPS_TRANS"),
wxT("SQLOPT_NO_REFINT_TRIGGERS"),
wxT("SQLOPT_USE_SYS_COLS"),
wxT("SQLOPT_CONSTRS_DEFERRED"),
wxT("SQLOPT_COL_LIST_EQUAL"),
wxT("SQLOPT_QUOTED_IDENT"),
wxT("SQLOPT_GLOBAL_REF_RIGHTS"),
wxT("SQLOPT_REPEATABLE_RETURN"),
wxT("SQLOPT_OLD_STRING_QUOTES")
};

wxString CompatPropertyGridTable::GetValue(int row, int col)
{ if (col==2)
    return wxString(compat_ids[row]);
  else return PropertyGridTable::GetValue(row, col); 
}


class CompatProperties : public PropertyGrid
{public:
  CompatProperties(cdp_t cdpIn);
  void load_and_show(void);
  bool save(void);
};

CompatProperties::CompatProperties(cdp_t cdpIn) : PropertyGrid(cdpIn)
{ 
  items.Add(new PropertyItemBool  (_("Can evaluate NULL=NULL"), NULL, false));
  items.Add(new PropertyItemBool  (_("Can compare NULL to other values"), NULL, false));
  items.Add(new PropertyItemBool  (_("Secret value returns NULL"), NULL, false));
  items.Add(new PropertyItemBool  (_("On numeric range overflow the result is NULL"), NULL, false));
  items.Add(new PropertyItemBool  (_("On string conversion error the result is NULL"), NULL, false));
  items.Add(new PropertyItemBool  (_("Character strings are right truncated if too long"), NULL, false));
  items.Add(new PropertyItemBool  (_("Deleted records are not released immediately and can be undeleted"), NULL, false));
  items.Add(new PropertyItemBool  (_("ALTER TABLE uses the old syntax"), NULL, false));
  items.Add(new PropertyItemBool  (_("Cursor can have multiple columns with the same name"), NULL, false));
  items.Add(new PropertyItemBool  (_("Prefix of an object name is never considered to be the owner name"), NULL, false));
  items.Add(new PropertyItemBool  (_("Scaled columns in the result of a query are converted to the Real type"), NULL, false));
  items.Add(new PropertyItemBool  (_("Errors terminate transactions immediately"), NULL, false));
  items.Add(new PropertyItemBool  (_("Active referential integrity will not fire triggers"), NULL, false));
  items.Add(new PropertyItemBool  (_("SELECT * will return system columns"), NULL, false));
  items.Add(new PropertyItemBool  (_("The default evaluation time of integrity constrains is DEFERRED"), NULL, false));
  items.Add(new PropertyItemBool  (_("An UPDATE trigger with a column list is executed only when the same set of columns are changed"), NULL, false));
  items.Add(new PropertyItemBool  (_("Text between double quotes are considered a character string (literal)"), NULL, false));
  items.Add(new PropertyItemBool  (_("Global select rights are required for columns used in query conditions"), NULL, false));
  items.Add(new PropertyItemBool  (_("The RETURN statement does not terminate the execution of a function"), NULL, false));
  items.Add(new PropertyItemBool  (_("Both ' and \" in character strings must be doubled"), NULL, false));
}

void CompatProperties::load_and_show(void)
{ sig32 val;
  if (cd_Get_property_num(cdp, sqlserver_owner, "DefaultSQLOptions", &val)) val=0;
  grid->SetCellValue(0, 1, (val & SQLOPT_NULLEQNULL) ? wxT("1") : wxT("0"));
  grid->SetCellValue(1, 1, (val & SQLOPT_NULLCOMP) ? wxT("1") : wxT("0"));
  grid->SetCellValue(2, 1, (val & SQLOPT_RD_PRIVIL_VIOL) ? wxT("1") : wxT("0"));
  grid->SetCellValue(3, 1, (val & SQLOPT_MASK_NUM_RANGE) ? wxT("1") : wxT("0"));
  grid->SetCellValue(4, 1, (val & SQLOPT_MASK_INV_CHAR) ? wxT("1") : wxT("0"));
  grid->SetCellValue(5, 1, (val & SQLOPT_MASK_RIGHT_TRUNC) ? wxT("1") : wxT("0"));
  grid->SetCellValue(6, 1, (val & SQLOPT_EXPLIC_FREE) ? wxT("1") : wxT("0"));
  grid->SetCellValue(7, 1, (val & SQLOPT_OLD_ALTER_TABLE) ? wxT("1") : wxT("0"));
  grid->SetCellValue(8, 1, (val & SQLOPT_DUPLIC_COLNAMES) ? wxT("1") : wxT("0"));
  grid->SetCellValue(9, 1, (val & SQLOPT_USER_AS_SCHEMA) ? wxT("1") : wxT("0"));
  grid->SetCellValue(10, 1, (val & SQLOPT_DISABLE_SCALED) ? wxT("1") : wxT("0"));
  grid->SetCellValue(11, 1, (val & SQLOPT_ERROR_STOPS_TRANS) ? wxT("1") : wxT("0"));
  grid->SetCellValue(12, 1, (val & SQLOPT_NO_REFINT_TRIGGERS) ? wxT("1") : wxT("0"));
  grid->SetCellValue(13, 1, (val & SQLOPT_USE_SYS_COLS) ? wxT("1") : wxT("0"));
  grid->SetCellValue(14, 1, (val & SQLOPT_CONSTRS_DEFERRED) ? wxT("1") : wxT("0"));
  grid->SetCellValue(15, 1, (val & SQLOPT_COL_LIST_EQUAL) ? wxT("1") : wxT("0"));
  grid->SetCellValue(16, 1, (val & SQLOPT_QUOTED_IDENT) ? wxT("1") : wxT("0"));
  grid->SetCellValue(17, 1, (val & SQLOPT_GLOBAL_REF_RIGHTS) ? wxT("1") : wxT("0"));
  grid->SetCellValue(18, 1, (val & SQLOPT_REPEATABLE_RETURN) ? wxT("1") : wxT("0"));
  grid->SetCellValue(19, 1, (val & SQLOPT_OLD_STRING_QUOTES) ? wxT("1") : wxT("0"));
 // clear the "changed" flag:
  for (int row=0;  row<items.GetCount();  row++)
    items[row]->changed=false;
}

bool CompatProperties::save(void)
{ bool any_changed = false;
  if (grid->IsCellEditControlEnabled()) grid->DisableCellEditControl();
  for (int row=0;  row<items.GetCount();  row++)
    if (items[row]->changed) any_changed=true;
  if (any_changed)
  { sig32 val = 0;
    if (!_tcscmp(grid->GetCellValue(0, 1).c_str(), wxT("1"))) val |= SQLOPT_NULLEQNULL;
    if (!_tcscmp(grid->GetCellValue(1, 1).c_str(), wxT("1"))) val |= SQLOPT_NULLCOMP;
    if (!_tcscmp(grid->GetCellValue(2, 1).c_str(), wxT("1"))) val |= SQLOPT_RD_PRIVIL_VIOL;
    if (!_tcscmp(grid->GetCellValue(3, 1).c_str(), wxT("1"))) val |= SQLOPT_MASK_NUM_RANGE;
    if (!_tcscmp(grid->GetCellValue(4, 1).c_str(), wxT("1"))) val |= SQLOPT_MASK_INV_CHAR;
    if (!_tcscmp(grid->GetCellValue(5, 1).c_str(), wxT("1"))) val |= SQLOPT_MASK_RIGHT_TRUNC;
    if (!_tcscmp(grid->GetCellValue(6, 1).c_str(), wxT("1"))) val |= SQLOPT_EXPLIC_FREE;
    if (!_tcscmp(grid->GetCellValue(7, 1).c_str(), wxT("1"))) val |= SQLOPT_OLD_ALTER_TABLE;
    if (!_tcscmp(grid->GetCellValue(8, 1).c_str(), wxT("1"))) val |= SQLOPT_DUPLIC_COLNAMES;
    if (!_tcscmp(grid->GetCellValue(9, 1).c_str(), wxT("1"))) val |= SQLOPT_USER_AS_SCHEMA;
    if (!_tcscmp(grid->GetCellValue(10, 1).c_str(), wxT("1"))) val |= SQLOPT_DISABLE_SCALED;
    if (!_tcscmp(grid->GetCellValue(11, 1).c_str(), wxT("1"))) val |= SQLOPT_ERROR_STOPS_TRANS;
    if (!_tcscmp(grid->GetCellValue(12, 1).c_str(), wxT("1"))) val |= SQLOPT_NO_REFINT_TRIGGERS;
    if (!_tcscmp(grid->GetCellValue(13, 1).c_str(), wxT("1"))) val |= SQLOPT_USE_SYS_COLS;
    if (!_tcscmp(grid->GetCellValue(14, 1).c_str(), wxT("1"))) val |= SQLOPT_CONSTRS_DEFERRED;
    if (!_tcscmp(grid->GetCellValue(15, 1).c_str(), wxT("1"))) val |= SQLOPT_COL_LIST_EQUAL;
    if (!_tcscmp(grid->GetCellValue(16, 1).c_str(), wxT("1"))) val |= SQLOPT_QUOTED_IDENT;
    if (!_tcscmp(grid->GetCellValue(17, 1).c_str(), wxT("1"))) val |= SQLOPT_GLOBAL_REF_RIGHTS;
    if (!_tcscmp(grid->GetCellValue(18, 1).c_str(), wxT("1"))) val |= SQLOPT_REPEATABLE_RETURN;
    if (!_tcscmp(grid->GetCellValue(19, 1).c_str(), wxT("1"))) val |= SQLOPT_OLD_STRING_QUOTES;
    if (cd_Set_property_num(cdp, sqlserver_owner, "DefaultSQLOptions", val)) 
      { cd_Signalize(cdp);  return false; }
  }
  return true;
}

//////////////////////////////////////////////// internals ///////////////////////////////////////////////

wxString InternPropertyGridTable::GetColLabelValue(int col)
{ return !col ? _("Server Information") : _("Value"); }

InternProperties::InternProperties(cdp_t cdpIn) : PropertyGrid(cdpIn)
{ 
  items.Add(new PropertyItemNum   (_("Total number of opened cursors"), "", false, 0, 1000000));
  items.Add(new PropertyItemNum   (_("Number of opened cursors by the current user"),   "", false, 0, 1000000));
  items.Add(new PropertyItemNum   (_("Number of temporary tables"),   "", false, 0, 1000000));
  items.Add(new PropertyItemNum   (_("Memory used by the server"), "", false, 0, 1000000000));
  items.Add(new PropertyItemNum   (_("Fixed frames"), "", false, 0, 1000000000));
  items.Add(new PropertyItemNum   (_("Fixes on frames"), "", false, 0, 1000000000));
}

void InternProperties::create(wxGrid * gridIn, PropertyGridTable * grid_table)
{ int row;
  PropertyGrid::create(gridIn, grid_table);
 // set all values read-only:
  for (row=0;  row<items.GetCount();  row++)
  { PropertyItem * item = items[row];
    grid->SetReadOnly(row, 1);
  }
}

void InternProperties::load_and_show(void)
{ sig32 val;
  cd_Get_server_info(cdp, OP_GI_OPEN_CURSORS,     &val, sizeof(val));
  grid->SetCellValue(0, 1, int2wxstr(val));
  cd_Get_server_info(cdp, OP_GI_OWNED_CURSORS,    &val, sizeof(val));
  grid->SetCellValue(1, 1, int2wxstr(val));
  cd_Get_server_info(cdp, OP_GI_TEMP_TABLES,      &val, sizeof(val));
  grid->SetCellValue(2, 1, int2wxstr(val));
  cd_Get_server_info(cdp, OP_GI_USED_MEMORY,      &val, sizeof(val));
  grid->SetCellValue(3, 1, int2wxstr(val));
  cd_Get_server_info(cdp, OP_GI_FIXED_PAGES,      &val, sizeof(val));
  grid->SetCellValue(4, 1, int2wxstr(val));
  cd_Get_server_info(cdp, OP_GI_FIXES_ON_PAGES,   &val, sizeof(val));
  grid->SetCellValue(5, 1, int2wxstr(val));
}

void PropertyGridWin::OnCellRightClick(wxGridEvent & event)
{ wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_REFRPANEL,    _("Refresh the list"));
  PopupMenu(popup_menu, event.GetPosition());
  delete popup_menu;
}

void PropertyGridWin::OnLabelRightClick(wxGridEvent & event)
{ OnCellRightClick(event); }

void PropertyGridWin::OnxCommand( wxCommandEvent& event )
{ switch (event.GetId())
  { case CPM_REFRPANEL:
      props->load_and_show();
      break;
    default:
      event.Skip();  break;
  }
}

/////////////////////////////////////
class wxGridCellEnum2Renderer : public wxGridCellEnumRenderer
{public:
  wxGridCellEnum2Renderer( const wxString& choices = wxEmptyString ) : wxGridCellEnumRenderer(choices)
    { }
  virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
  wxString GetString(wxGrid& grid, int row, int col);
};

void wxGridCellEnum2Renderer::Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rectCell, int row, int col, bool isSelected)
// Same as in wxGridCellEnumRenderer, but calls local GetString (which is sadly not virtual)
{ wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);
  SetTextColoursAndFont(grid, attr, dc, isSelected);
 // draw the text right aligned by default
  int hAlign, vAlign;
  attr.GetAlignment(&hAlign, &vAlign);
  hAlign = wxRIGHT;
  wxRect rect = rectCell;
  rect.Inflate(-1);
  grid.DrawTextRectangle(dc, GetString(grid, row, col), rect, hAlign, vAlign);
}

wxString wxGridCellEnum2Renderer::GetString(wxGrid& grid, int row, int col)
{ wxGridTableBase *table = grid.GetTable();
  wxString text;
  if ( table->CanGetValueAs(row, col, wxGRID_VALUE_NUMBER) )
  {
      int choiceno = table->GetValueAsLong(row, col);
      text.Printf(_T("%s"), m_choices[ choiceno ].c_str() );
  }
  else
  { unsigned long lval;
    text = table->GetValue(row, col);
    if (text.ToULong(&lval))
      text.Printf(_T("%s"), m_choices[ lval ].c_str() );
  }
  //If we faild to parse string just show what we where given?
  return text;
}

bool runBrowser(const char * url)
{
#ifdef WINS
  HCURSOR hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT)); 
 // 1.vytvorit temporary file kvuli FindExecutable
  char szTmpPath[MAX_PATH];
  char szName[MAX_PATH];
  char szCmdLine[2*MAX_PATH+2];
  STARTUPINFOA        si;
  PROCESS_INFORMATION pi;
  char szBrowser[MAX_PATH];
  HINSTANCE hRet;
  FHANDLE iFileHandle;

  if (!GetTempPathA(sizeof(szTmpPath), szTmpPath))
    goto Failed;
  if (!GetTempFileNameA(szTmpPath, "NIC", 0, szName))
    goto Failed;
  DeleteFileA(szName);
  strcpy(szName+strlen(szName)-3, "HTM");  //koncovka htm
  iFileHandle = CreateFileA(szName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
  if (iFileHandle == INVALID_FHANDLE_VALUE)
    goto Failed;

 //2. najit default browser
  hRet = FindExecutableA(szName, NULL, (LPSTR) &szBrowser);
  CloseHandle(iFileHandle);
  DeleteFileA(szName);
  if ((size_t)hRet <= 32) //nastala chyba
    goto Failed;

 //3. spustit browser
  memset(&si, 0, sizeof STARTUPINFO);
  memset(&pi, 0, sizeof PROCESS_INFORMATION);
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags |= STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOWMAXIMIZED; 

  sprintf(szCmdLine, "%s %s", szBrowser, url);
  if (CreateProcessA(NULL, szCmdLine,
         NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE|
         CREATE_NEW_PROCESS_GROUP|NORMAL_PRIORITY_CLASS,
         NULL, NULL, &si, &pi)) {
    CloseHandle(pi.hThread);  //maji se uvolnit...
    CloseHandle(pi.hProcess);
  }
  SetCursor(hPrevCursor);
  return true;

Failed:
  SetCursor(hPrevCursor);
  return false;
#else
  info_box(wxString(url, *wxConvCurrent));
  return false;
#endif
}

#include <wx/mimetype.h>

void stViewHTMLFile(const wxString& url)
{
#ifdef WINS  // private solution
  runBrowser(url.mb_str(*wxConvCurrent));
#else
  if (wxLaunchDefaultBrowser(url, 0))
    return;
  const int STRLEN = 10000;
  char shell_cmd[STRLEN];
  pid_t childpid;
  FILE * pipein_fp;
  char * lin_browser[7] = {"mozilla", "konqueror", "firefox", "galeon", "firebird", "netscape", "opera"};
  char * brows_exec_path[STRLEN];

  int i;
  for (i = 0; i < 7; i++)
  {
    sprintf(shell_cmd, "which %s 2> /dev/null", lin_browser[i]);

    if ((pipein_fp = popen(shell_cmd, "r")) == NULL)
    {
      perror("popen");
      exit(1);
    }

    int len = fread(brows_exec_path, sizeof(unsigned char), sizeof(brows_exec_path), pipein_fp);
    pclose(pipein_fp);
    if (len > 0)
    {
      break;
    }
  }

  if ((childpid = fork()) == -1)
  {
//    perror("fork");
    return;
  }
  else if (childpid == 0)
  {
    return;
  }
  else
  {
    execlp(lin_browser[i], lin_browser[i], (const char *)url.mb_str(*wxConvCurrent), NULL);
    exit(0);
  }
#endif
}

/*
void stViewHTMLFile(const wxString& url)                                        
{      
#ifdef WINS  // private solution
  runBrowser(url.mb_str(*wxConvCurrent));
  return;
#endif
                                                                         
#ifdef __WXMAC__                                                                
    wxString url1(url);                                                         
    if (url1.Left(5) != wxT("file:"))                                           
        url1 = wxNativePathToURL(url1);                                         
                                                                                
    OSStatus err;                                                               
    ICInstance inst;                                                            
    SInt32 startSel;                                                            
    SInt32 endSel;                                                              
                                                                                
    err = ICStart(&inst, 'STKA'); // put your app creator code here             
    if (err == noErr) {                                                         
#if !TARGET_CARBON                                                              
        err = ICFindConfigFile(inst, 0, nil);                                   
#endif                                                                          
        if (err == noErr) {                                                     
            startSel = 0;                                                       
            endSel = wxStrlen(url1);                                            
            err = ICLaunchURL(inst, "\p", url1, endSel, &startSel, &endSel);    
        }                                                                       
        ICStop(inst);                                                           
    }                                                                           
    // return err;                                                              
#else                                                                           
    wxFileType *ft =                                                            
wxTheMimeTypesManager->GetFileTypeFromExtension(wxT("html"));                   
    if ( !ft )                                                                  
    {                                
        wxLogError(_T("Impossible to determine the file type for extension html. Please edit your MIME types."));                                          
        return ;                                                                
    }                                                                           
                                                                                
    wxString cmd;                                                               
    bool ok = ft->GetOpenCommand(&cmd,                                          
                                 wxFileType::MessageParameters(url, _T("")));   
    delete ft;                                                                  
                                                                                
    if (!ok)                                                                    
    {                                                                           
        // TODO: some kind of configuration dialog here.                        
        wxMessageBox(_("Could not determine the command for running the browser."),                                                                     
                   wxT("Browsing problem"), wxOK|wxICON_EXCLAMATION);           
        return ;                                                                
    }                                                                           
                                                                                
    if (cmd.Find(wxT("http://")) != -1)                                         
        cmd.Replace(wxT("file://"), wxT(""));                    
                                                                                
    ok = (wxExecute(cmd, FALSE) != 0);                                          
#endif                                                                          
}                                                                               
*/                                                                                
const wxString g_unixPathString(wxT("/"));                                      
const wxString g_nativePathString(wxFILE_SEP_PATH);                             
                                                                                
#if 0
// Returns the file URL for a native path                                       
wxString wxNativePathToURL( const wxString& path )                              
{                                                                               
#if wxCHECK_VERSION(2, 5, 0)                                                    
    return wxFileSystem::FileNameToURL(path);                                   
#else                                                                           
    wxString url = path ;                                                       
#ifdef __WXMSW__                                                                
        // unc notation                                                         
        if ( url.Find(wxT("\\\\")) == 0 )                                       
        {                                                                       
                url = url.Mid(2) ;                                              
        }                                                                       
        else                                                                    
#endif                                                                          
        url.Replace(g_nativePathString, g_unixPathString) ;                     
        url = wxT("file://") + url ;                                            
        return url ;                                                            
#endif                                                                          
}                                                                               
#endif

ProfilerDlg * profiler_dlg = NULL;       // the unique profiler dialog
DataGrid * global_profile_grid = NULL;   // the unique global profile grid opened from the profiler dialog

#include "regstr.h"
#include "xrc/ServerConfigurationDlg.cpp"
#include "xrc/DatabaseFileEncryptionDlg.cpp"
#include "xrc/UNCBackupDlg.cpp"
#include "xrc/AutoBackupDlg.cpp"
#include "xrc/ServerCertificateDlg.cpp"
#include "xrc/DatabaseConsistencyDlg.cpp"
#include "xrc/RenameServerDlg.cpp"
#include "xrc/ProtocolsPortsDlg.cpp"

#if WBVERS>=1100
#include "xrc/AddOnLicencesDlg.cpp"
#else
#include "xrc/LicenceWizardDlg.cpp"
#include "xrc/LicenceInformationDlg.cpp"
#endif
#include "xrc/ProfilerDlg.cpp"
#include "xrc/HTTPAuthentification.cpp"
#include "xrc/HTTPAccessDlg.cpp"

void console_action(tcpitemid cti, cdp_t cdp)
{ wxWindow * parent = wxGetApp().frame;
  switch (cti)
  { case IND_CONSISTENCY:
    { wxGetApp().frame->SetStatusText(wxEmptyString, 0);  // Must remove the old "Operation Completed" string
      if (!cd_Am_I_config_admin(cdp))
        { error_box(_("Only the configuration administrator can check the consistency."));  return; }
      DatabaseConsistencyDlg dlg(cdp);
      dlg.Create(parent);
      dlg.ShowModal();
      break;
    }
    case IND_LICENCES:
    {
#if WBVERS>=1100
      AddOnLicencesDlg dlg(cdp);
#else
      LicenceInformationDlg dlg(cdp);
#endif      
      dlg.Create(parent);
      dlg.ShowModal();
      break;
    }
    case IND_RUNPROP:
    { SecurProperties  * pg1 = new SecurProperties(cdp);
      OperProperties   * pg2 = new OperProperties(cdp);
      CompatProperties * pg3 = new CompatProperties(cdp);
      ServerConfigurationDlg dlg(cdp, pg1, pg2, pg3);
      dlg.Create(parent);
      dlg.ShowModal();
      delete pg1;  delete pg2;  delete pg3;
      break;
    }
    case IND_PROTOCOLS:
    { ProtocolsPortsDlg dlg(cdp);
      dlg.Create(parent);
      dlg.ShowModal();
      break;
    }
    case IND_FILENC:
    { DatabaseFileEncryptionDlg dlg(cdp);
      dlg.Create(parent);
      dlg.ShowModal();
      break;
    }
    case IND_MESSAGE:
    { if (!cd_Am_I_config_admin(cdp))
        { error_box(_("Only the configuration administrator can send messages to clients."));  return; }
      wxString msg = wxGetTextFromUser(_("Enter the message to be sent to all interactive non-local clients of the server:"),
        _("Message to Clients"), wxEmptyString, parent, -1, -1, false);
      if (!msg.IsEmpty())
        if (cd_Message_to_clients(cdp, msg.mb_str(wxConvUTF8))) 
          cd_Signalize(cdp);
      break;
    }
    case IND_BACKUP:
    { AutoBackupDlg dlg(cdp);
      dlg.Create(parent);
      dlg.ShowModal();
      break;
    }
    case IND_SRVCERT:
      if (cd_Am_I_security_admin(cdp))
      { ServerCertificateDlg dlg(cdp);
        dlg.Create(parent);
        dlg.ShowModal();
      }
      else error_box(_("These parameters are accessible to the security administrator only."));
      break;
    case IND_HTTP:
    { HTTPAccessDlg dlg(cdp);
      dlg.Create(parent);
      dlg.ShowModal();
      break;
    }
    case IND_PROFILER:
      if (!cd_Am_I_config_admin(cdp))
        { error_box(_("Only the configuration administrator can profile the server."));  return; }
      if (profiler_dlg)
      { profiler_dlg->Iconize(false);
        profiler_dlg->Raise();
      }
      else
      { profiler_dlg = new ProfilerDlg(cdp);
        profiler_dlg->Create(parent);
        profiler_dlg->Show(true);
      }
      break;
    case IND_EXP_USERS:
    { if (!cd_Am_I_config_admin(cdp))
        { error_box(_("Only the configuration administrator can export users."));  break; }
      wxString fname = GetExportFileName(wxString("usertab.tdt", *wxConvCurrent), _("602SQL Table data files (*.tdt)|*.tdt"), parent);
      if (fname.IsEmpty()) break;
      if (!can_overwrite_file(fname.mb_str(*wxConvCurrent), NULL)) break; /* file exists, overwrite not confirmed */
     // export usertab:
      FHANDLE hFile=CreateFileA(fname.mb_str(*wxConvCurrent), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      if (hFile==INVALID_FHANDLE_VALUE)
        { client_error(cdp, OS_FILE_ERROR);  cd_Signalize(cdp);  break; }
      BOOL res=cd_Export_user(cdp, NOOBJECT, hFile);
      CloseHandle(hFile);
      if (res) { cd_Signalize(cdp);  break; }
#ifdef STOP
     // export repltab:
      int i=strlen(fname);
      while (i && fname[i-1]!='\\') i--;
      strcpy(fname+i, "KEYTAB.TDT");
      hFile=CreateFileA(fname.mb_str(*wxConvCurrent), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      if (hFile==INVALID_FHANDLE_VALUE)
        { client_error(cdp, OS_FILE_ERROR);  cd_Signalize(cdp);  break; }
      res=cd_xSave_table(cdp, KEY_TABLENUM, hFile);
      CloseHandle(hFile);
      if (res) { cd_Signalize(cdp);  break; }
#endif
      info_box(_("Users exported"), parent);
      break;
    }

    case IND_IMP_USERS:
    { if (!cd_Am_I_config_admin(cdp))  // blocked in the server too
        { error_box(_("Only the configuration administrator can import users."));  return; }
      wxString fname = GetImportFileName(wxString("usertab.tdt", *wxConvCurrent), _("602SQL Table data files (*.tdt)|*.tdt"), parent);
      if (fname.IsEmpty()) break;
     // ask for clearing the passwords:
	    bool clear_pswds = 
       yesno_box(_("Reset the password on all users?\nThe original passwords are only valid if the server has the same UUID.\nClick Yes if the server UUID has not been restored."),
                 parent);
      FHANDLE hFile;  BOOL res;
#ifdef STOP
     // import REPLTAB first because after importing the USERTAB I may not have the rights to do it:
      int pos=strlen(fname);
      while (pos && fname[pos-1]!=PATH_SEPARATOR) pos--;
      strcpy(fname+pos, "KEYTAB.TDT");
      hFile=CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (hFile == INVALID_HANDLE_VALUE)
        { client_error(cdp, OS_FILE_ERROR);  cd_Signalize(cdp);  break; }
      BOOL res=cd_Restore_table(cdp, KEY_TABLENUM, hFile, FALSE);
      CloseHandle(hFile);
      if (res) { cd_Signalize(cdp);  break; }
     // import USRTAB:
      strcpy(fname+pos, "USERTAB.TDT");
#endif
      hFile=CreateFileA(fname.mb_str(*wxConvCurrent), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (hFile == INVALID_FHANDLE_VALUE)
        { client_error(cdp, OS_FILE_ERROR);  cd_Signalize(cdp);  break; }
      res=cd_Restore_table(cdp, TAB_TABLENUM, hFile, clear_pswds);  // TABTAB represents the whole USERTAB!
      CloseHandle(hFile);
      if (res) { cd_Signalize(cdp);  break; }
      info_box(_("Users imported"), parent);
      break;
    }
    case IND_SERV_RENAME:
    { 
      //int old = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | /*_CRTDBG_CHECK_CRT_DF | _CRTDBG_DELAY_FREE_MEM_DF | */ _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_EVERY_16_DF);
      RenameServerDlg dlg(cdp);
      dlg.Create(parent);
      dlg.ShowModal();
      wxLogDebug(wxT(""));
      wxLogDebug(wxT("RenameServer"));
      break;
    }
  }
}

#include "xrc/FontSelectorDlg.cpp"
#include "xrc/GridDataFormat.cpp"
#include "xrc/ClientLanguageDlg.cpp"
#include "xrc/FtxHelpersConfigDlg.cpp"
#include "xrc/ClientCommDlg.cpp"
#include "xrc/ExtEditorsDlg.cpp"
#include <odbcinst.h>
#include "cptree.h"

void client_action(wxCPSplitterWindow * control_panel, item_info & info)
{
  switch (info.cti)
  {
    case IND_FONTS:
    {
        FontSelectorDlg dlg(control_panel->frame, -1);
        dlg.ShowModal();
        break;
    }
    case IND_FORMATS:
    {
        GridDataFormat dlg(control_panel->frame, -1);
        dlg.ShowModal();
        break;
    }
    case IND_LOCALE:
    {   ClientLanguageDlg dlg(control_panel->frame, -1);
        dlg.ShowModal();
        break;
    }
    case IND_ODBCTAB:
#ifdef WINS
      //SQLManageDataSources((HWND)control_panel->frame->GetHandle()); -- interferes with the wxWigdets, last tested in 2.8.0
      wxExecute(wxT("odbcad32.exe"), wxEXEC_SYNC);
      control_panel->RefreshPanel();  // updates the list of data sources
#else
      system("ODBCConfig");
#endif
        break;
    case IND_MAIL:
        control_panel->tree_view->select_child(info);  // refills this list view
        break;
    case IND_MAILPROF:
    { 
        single_selection_iterator si(info);
        control_panel->OnModify(si, false);
        break;
    }
    case IND_FTXHELPERS:
    {
      FtxHelpersConfigDlg dlg(control_panel->frame, -1);
      dlg.ShowModal();
      break;
    }
    case IND_COMM:
    { ClientCommDlg dlg(control_panel->frame, -1);
      bool orig_browse_enabled = browse_enabled;
      if (dlg.ShowModal() == wxID_OK)
      {// set the new modulus into all existing connections:
        for (t_avail_server * as = available_servers;  as;  as=as->next)
          if (as->cdp)
            cd_Set_progress_report_modulus(as->cdp, notification_modulus);
       // browse:
        if (!connected_count)
          if (orig_browse_enabled!=browse_enabled)
            if (browse_enabled) BrowseStart(0);
            else                BrowseStop(0);
        Enable_server_search(broadcast_enabled);
      }
      break;
    }
    case IND_EDITORS:
    { ExtEditorsDlg dlg(control_panel->frame, -1);
      dlg.ShowModal();
      break;
    }
  }
}
