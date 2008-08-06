/////////////////////////////////////////////////////////////////////////////
// Name:        CallRoutineDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/24/04 12:33:23
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "CallRoutineDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "CallRoutineDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * CallRoutineDlg type definition
 */

IMPLEMENT_CLASS( CallRoutineDlg, wxDialog )

/*!
 * CallRoutineDlg event table definition
 */

BEGIN_EVENT_TABLE( CallRoutineDlg, wxDialog )

////@begin CallRoutineDlg event table entries
    EVT_BUTTON( ID_CALL_CALL, CallRoutineDlg::OnCallCallClick )

    EVT_BUTTON( ID_CALL_CANCEL, CallRoutineDlg::OnCallCancelClick )

    EVT_BUTTON( wxID_HELP, CallRoutineDlg::OnHelpClick )

////@end CallRoutineDlg event table entries
    EVT_CLOSE(CallRoutineDlg::OnClose)
END_EVENT_TABLE()

/*!
 * CallRoutineDlg constructors
 */

CallRoutineDlg::CallRoutineDlg(cdp_t cdpIn, tobjnum objnumIn)
{ cdp=cdpIn;  objnum=objnumIn; 
  counter++;
  global_inst=this;
  pars=NULL;
  par_cnt=0;
  hvars=NULL;
}

/*!
 * CallRoutineDlg creator
 */

bool CallRoutineDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CallRoutineDlg member initialisation
    mSizerContainingGrid = NULL;
    mGrid = NULL;
    mDebug = NULL;
    mOK = NULL;
    mCancel = NULL;
////@end CallRoutineDlg member initialisation

////@begin CallRoutineDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS|wxDIALOG_EX_CONTEXTHELP);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CallRoutineDlg creation
    Raise();
    return TRUE;
}

/*!
 * Control creation for CallRoutineDlg
 */

void CallRoutineDlg::CreateControls()
{    
////@begin CallRoutineDlg content construction
    CallRoutineDlg* itemDialog1 = this;

    mSizerContainingGrid = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mSizerContainingGrid);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("&Parameters and results:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrid->Add(itemStaticText3, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mGrid = new wxGrid( itemDialog1, ID_CALL_GRID, wxDefaultPosition, wxSize(200, 120), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mGrid->SetHelpText(_("Enter the input parameters before calling the routine."));
    if (ShowToolTips())
        mGrid->SetToolTip(_("Enter the input parameters before calling the routine."));
    mSizerContainingGrid->Add(mGrid, 1, wxGROW|wxALL, 5);

    mDebug = new wxCheckBox;
    mDebug->Create( itemDialog1, ID_CALL_DEBUG, _("&Run in debug mode"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mDebug->SetValue(FALSE);
    if (ShowToolTips())
        mDebug->SetToolTip(_("If checked, the execution of the routine will stop on the first statement and the debug process will start."));
    mSizerContainingGrid->Add(mDebug, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    mSizerContainingGrid->Add(itemBoxSizer6, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mOK = new wxButton;
    mOK->Create( itemDialog1, ID_CALL_CALL, _("Call"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mCancel = new wxButton;
    mCancel->Create( itemDialog1, ID_CALL_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mCancel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end CallRoutineDlg content construction
    //mGrid = new wxGrid( itemDialog1, ID_CALL_GRID, wxDefaultPosition, wxSize(200, 120), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    //mGrid->SetHelpText(_("Enter the input parameters before calling the routine."));
    //if (ShowToolTips())
    //    mGrid->SetToolTip(_("Enter the input parameters before calling the routine."));
    //mSizerContainingGrid->Add(mGrid, 1, wxGROW|wxALL, 5);
    // the calling of all grid methods above should removed (item4)
    prepare_grid();  // prepare the grid before Fit-ting
    int visible_rows = mGrid->GetNumberRows();
    if (visible_rows > 5) visible_rows=5;
    if (visible_rows < 2) visible_rows=2;
    mSizerContainingGrid->SetItemMinSize(mGrid, get_grid_width(mGrid), get_grid_height(mGrid, visible_rows));
    mDebug->SetValue(debug_mode);
}

/*!
 * Should we show tooltips?
 */

bool CallRoutineDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void CallRoutineDlg::OnCallCallClick( wxCommandEvent& event )
{ 
  if (mGrid->IsCellEditControlEnabled())  // commit cell edit
    mGrid->DisableCellEditControl();
  debug_mode=mDebug->GetValue();  // save this in a static variable
  call_sql_proc(debug_mode); 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void CallRoutineDlg::OnCallCancelClick( wxCommandEvent& event )
{ Destroy(); }

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void CallRoutineDlg::OnHelpClick( wxCommandEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_procedure.html")); }

void CallRoutineDlg::EnableControls(bool enable)
{ mGrid->Enable(enable);
  mGrid->Refresh();  // without this the graying is not added/removed
  mDebug->Enable(enable);
  mOK->Enable(enable);
  mCancel->Enable(enable);
}
////////////////////////// grid table ///////////////////////////////
class ParamGridTable : public wxGridTableBase
{ CallRoutineDlg * cr;
 public:
    virtual int GetNumberRows();
 private:
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void SetValue( int row, int col, const wxString& value );

    virtual wxString GetColLabelValue( int col );
    //bool AppendRows(size_t numRows = 1);
    //bool InsertRows(size_t pos = 0, size_t numRows = 1);  // used only for initial filling
    //bool DeleteRows(size_t pos = 0, size_t numRows = 1);
    bool CanGetValueAs( int row, int col, const wxString& typeName );
    long GetValueAsLong( int row, int col);
    //void SetValueAsLong( int row, int col, long value );
 public:
  ParamGridTable(CallRoutineDlg * crIn)
    { cr=crIn; }
};

// columns in the grid:
#define PAR_MODE  0
#define PAR_NAME  1
#define PAR_VALUE 2
#define PAR_TYPE  3

wxString ParamGridTable::GetColLabelValue( int col )
{ switch (col)
  { case PAR_MODE:  return _("Mode");
    case PAR_NAME:  return _("Name");
	case PAR_VALUE: return _("Value");
    case PAR_TYPE:  return _("Type");
	default: return wxEmptyString;
  }	
}

int ParamGridTable::GetNumberRows()
{ return cr->par_cnt; }  // function result included in parameters

int ParamGridTable::GetNumberCols()
{ return 4; }

bool ParamGridTable::IsEmptyCell( int row, int col )
{ return false; }

#if 0
bool ParamGridTable::AppendRows(size_t numRows)
// only numRows==1 implemented
{ forkey_constr * forkey = tde->edta.refers.next();
  if (!forkey) return false;
  *forkey->constr_name=0;  forkey->par_text=forkey->text=NULL;  forkey->state=ATR_NEW;
  *forkey->desttab_name=*forkey->desttab_schema=0;
  forkey->delete_rule=forkey->update_rule=REFRULE_NO_ACTION;
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}
bool ParamGridTable::DeleteRows(size_t pos, size_t numRows)
// only numRows==1 implemented
{ forkey_constr * forkey = tde->get_forkey(pos);
  if (!forkey) return false;
  int offs = forkey-tde->edta.refers.acc0(0);
  tde->edta.refers.delet(offs);  // or change the state??
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, pos, numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

void ParamGridTable::SetValueAsLong( int row, int col, long value )
{ forkey_constr * forkey = tde->get_forkey(row);
  if (!forkey) return;
  switch (col)
  { case 4: forkey->update_rule=value+1;  break;
    case 5: forkey->delete_rule=value+1;  break;
  }
  tde->indx_change=true;
}

bool ParamGridTable::InsertRows(size_t pos, size_t numRows)
// Used only for initial inserting
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, pos, numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

#endif

bool ParamGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{ if (typeName == wxGRID_VALUE_STRING) return true;
  if (typeName == wxGRID_VALUE_NUMBER)
    if (col==PAR_MODE) return true;
  return false;
}

long ParamGridTable::GetValueAsLong( int row, int col)
{ if (col!=PAR_MODE) return -1;
  int ind = cr->is_fnc ? row+1 : row;
  if (ind>=cr->par_cnt) return 3;  // result is <Ret>
  return cr->pars[ind].direction-1;
}

wxString ParamGridTable::GetValue( int row, int col )
{ int ind = cr->is_fnc ? row+1 : row;
  if (ind>=cr->par_cnt) // function result
    switch (col)
    { case PAR_NAME: 
        return _("<function value>");
      case PAR_VALUE:
        return cr->fncval;
      case PAR_TYPE:
      { char buf[55+OBJ_NAME_LEN];  // buffer for the type name
        sql_type_name(cr->pars[0].data_type, cr->pars[0].specif, true, buf);
        return wxString(buf, *wxConvCurrent);  // buf contents used in the construction of wxString, but not after this
      }
    }
  else
    switch (col)
    { case PAR_NAME: 
        return wxString(cr->pars[ind].parname, *cr->cdp->sys_conv);
      case PAR_VALUE:
        return cr->pars[ind].parval;
      case PAR_TYPE:
      { char buf[55+OBJ_NAME_LEN];  // buffer for the type name
        sql_type_name(cr->pars[ind].data_type, cr->pars[ind].specif, true, buf);
        return wxString(buf, *wxConvCurrent);  // buf contents used in the construction of wxString, but not after this
      }
    }
  return wxEmptyString;
}

void ParamGridTable::SetValue( int row, int col, const wxString& value )
{ int ind = cr->is_fnc ? row+1 : row;
  switch (col)
  { case PAR_VALUE:
      cr->pars[ind].parval=value;  break;
  }
}

//////////////////////////// grid /////////////////////////////////////
void CallRoutineDlg::prepare_grid(void)
{ 
  if (!init()) return;  // prepares data for the mGrid
  mGrid->BeginBatch();
  ParamGridTable * param_grid_table = new ParamGridTable(this);
  mGrid->SetTable(param_grid_table, TRUE);
  mGrid->SetColLabelSize(default_font_height);
  mGrid->SetDefaultRowSize(default_font_height);
  mGrid->SetRowLabelSize(22);  // for 2 digits
  mGrid->SetLabelFont(system_gui_font);
  DefineColSize(mGrid, PAR_MODE, 38);
  DefineColSize(mGrid, PAR_NAME, 86);
  DefineColSize(mGrid, PAR_VALUE, 160);
  DefineColSize(mGrid, PAR_TYPE, 100);
 // prepare mode choice:
  wxGridCellAttr * mode_attr = new wxGridCellAttr;
  wxGridCellEnumRenderer * mode_renderer = new wxGridCellEnumRenderer(wxT("In,Out,InOut,Ret"));
  mode_attr->SetRenderer(mode_renderer);  // takes the ownership
  mode_attr->SetReadOnly(true);
  mGrid->SetColAttr(PAR_MODE, mode_attr);  // takes the ownership!! Must not reuse!!
 // read only:
  wxGridCellAttr * ro_attr = new wxGridCellAttr;
  ro_attr->SetReadOnly(true);
  mGrid->SetColAttr(PAR_NAME, ro_attr);  // takes the ownership!! Must not reuse!!
  ro_attr = new wxGridCellAttr;
  ro_attr->SetReadOnly(true);
  mGrid->SetColAttr(PAR_TYPE, ro_attr);  // takes the ownership!! Must not reuse!!

  mGrid->EnableDragRowSize(false);
 // disable value cell for OUT parameters and the result:
  for (trecnum r=0;  r<par_cnt;  r++)
  { int row = r;
    if (is_fnc) if (!r) row=par_cnt-1;  else row=r-1;
    mGrid->SetReadOnly(row, PAR_VALUE, pars[r].direction==MODE_OUT || pars[r].direction==0);
  }
  mGrid->EndBatch();
}

#include "compil.h"

bool CallRoutineDlg::init(void)
{ WBUUID uuid;  
 // read the information about the routine:
  cd_Read(cdp, OBJ_TABLENUM, objnum, OBJ_NAME_ATR, NULL, name);
  cd_Read(cdp, OBJ_TABLENUM, objnum, APPL_ID_ATR,  NULL, uuid);
  if (cd_Apl_id2name(cdp, uuid, schema_name))
    { cd_Signalize(cdp);  return false; }
  if (!stricmp(name, MODULE_GLOBAL_DECLS))  // cannot run
    { wxBell();  return false; }
 // dialog title:
  wxString str;  
  str.Printf(_("Calling Routine %s"), wxString(name, *cdp->sys_conv).c_str());
  SetTitle(str);
 // get information about parametrs:
  wxString query;  tcursnum curs;  trecnum r;
  query.Printf(wxT("SELECT * FROM _iv_procedure_parameters WHERE schema_name='%s' AND procedure_name='%s' ORDER BY ordinal_position"), 
           (const wxChar*)wxString(schema_name, *cdp->sys_conv).c_str(), 
           (const wxChar*)wxString(name,        *cdp->sys_conv).c_str());
  if (cd_Open_cursor_direct(cdp, query.mb_str(*cdp->sys_conv), &curs)) { cd_Signalize(cdp);  return false; }
  cd_Rec_cnt(cdp, curs, &par_cnt);
  if (par_cnt)
  { pars= new t_run_sql_param[par_cnt];  // must call constructors for wxString
    hvars=(t_clivar*)sigalloc(sizeof(t_clivar)*par_cnt, 78);
    if (!pars || !hvars) { cd_Close_cursor(cdp, curs);  return false; }
    for (r=0;  r<par_cnt;  r++)
    {// read the description of a parameter:
      cd_Read(cdp, curs, r, 3, NULL, pars[r].parname);
      cd_Read(cdp, curs, r, 5, NULL, &pars[r].data_type);
      cd_Read(cdp, curs, r, 6, NULL, &pars[r].length);     // is in bytes for Unicode strings
      cd_Read(cdp, curs, r, 7, NULL, &pars[r].precision);
      cd_Read(cdp, curs, r, 8, NULL, &pars[r].direction);
      cd_Read(cdp, curs, r, 9, NULL, &pars[r].specif);
     // create the description of a host variable:
      strcpy(hvars[r].name, pars[r].parname);
      if (!*hvars[r].name) sprintf(hvars[r].name, "Param%u", r);
      hvars[r].wbtype=pars[r].data_type;
      hvars[r].specif=pars[r].specif.opqval;
      if (pars[r].direction==0) hvars[r].mode=MODE_OUT; // function result has MODE_OUT
      else hvars[r].mode=(t_parmode)pars[r].direction;
     // calculate space for the host variable and allocate it:
      hvars[r].buflen = IS_HEAP_TYPE(pars[r].data_type) ? MAX_FIXED_STRING_LENGTH+2 : SPECIFLEN(pars[r].data_type) ? pars[r].length+2 : WB_type_length(pars[r].data_type);
      hvars[r].buf=corealloc(hvars[r].buflen, 79);
      hvars[r].actlen = 0;  // used by OUT parameters when the called procedure has some problems
    }
    int ordr;
    cd_Read(cdp, curs, 0, 4, NULL, &ordr);
    is_fnc = ordr==0;
  }
  else  // no parameters nor a return value: must be a procedure
  { is_fnc = false;
    pars=NULL;
    hvars=NULL;
  }
  cd_Close_cursor(cdp, curs); 
  return true;
}

/* Conversion of parameter values:
   ATT_NOSPEC and ATT_RASTER values represented like ATT_BINARY, but specif must be set according to the actual length, limited by buffer size
   ATT_TEXT values may have to be truncated, too
*/

bool CallRoutineDlg::call_sql_proc(bool debugged)
{ if (debugged && the_server_debugger)
    { unpos_box(_("A debug session is already in progress."));  return false; }
 // move param values to host variables:
  int par;
  for (par=0;  par<par_cnt;  par++)
  { int parnum = is_fnc ? par-1 : par;
    if (parnum>=0 && pars[par].direction!=(int)MODE_OUT)  // IN or INOUT parameter
    { pars[par].parval.Trim(true);  pars[par].parval.Trim(false);
      const wxChar * valbuf = pars[par].parval.c_str();
     // determine the size of the native value (if necessary):
      int tp=pars[par].data_type;
      if (tp==ATT_STRING || tp==ATT_TEXT)
      { hvars[par].actlen=(int)_tcslen(valbuf);
        if (pars[par].specif.wide_char) hvars[par].actlen *= 2;
      }
      else if (tp==ATT_BINARY || tp==ATT_NOSPEC || tp==ATT_RASTER)
      { hvars[par].actlen=((int)_tcslen(valbuf)+1)/2;  // '0' added when the number of characters if odd
        if (tp==ATT_NOSPEC || tp==ATT_RASTER)
        { pars[par].specif.length = (uns16)hvars[par].actlen;  // prevents adding 0s on the end!
          tp=ATT_BINARY;  // convert_from_string cannot be used for ATT_NOSPEC or ATT_RASTER
        }
      }
     // convert string value to the native value:
      //if (!convert_from_string(tp, pars[par].parval.mb_str(*cdp->sys_conv), hvars[par].buf, pars[par].specif))
     // limit the input string size according to specif:
      int limit;
      if (tp==ATT_STRING || tp==ATT_TEXT)
        { limit = hvars[par].buflen-2;  if (pars[par].specif.wide_char) limit/=2; }
      else if (tp==ATT_BINARY || IS_HEAP_TYPE(tp))
        limit = (hvars[par].buflen-2)/2;
      else limit=100000;
      if (pars[par].parval.Length() > limit) 
        pars[par].parval=pars[par].parval.Mid(0, limit);
      wxString string_format = get_grid_default_string_format(tp);
      int err=superconv(ATT_STRING, tp, pars[par].parval.c_str(), (char*)hvars[par].buf, wx_string_spec, pars[par].specif, string_format.mb_str(*wxConvCurrent));
#if 0  // just for testing      
      char* xbuf = (char*)(hvars[par].buf = corealloc(5000001, 55));
      memset(hvars[par].buf, 'X', 5000000);
      xbuf[5000000]=0;
      hvars[par].buflen=hvars[par].actlen=strlen(xbuf);
#endif      
      if (err<0) 
        { wxBell();  error_box(_("Cannot convert this parameter value."), wxGetApp().frame);  return false; }
    }
  }
 // create the SQL statement:
  wxString stmt;
  stmt.Printf(is_fnc ? wxT("SET :>Param0=`%s`.`%s`(") : wxT("CALL `%s`.`%s`("),
           wxString(schema_name, *cdp->sys_conv).c_str(), 
           wxString(name,        *cdp->sys_conv).c_str());
  bool firstpar=true;
  for (par=0;  par<par_cnt;  par++)
  { int parnum = is_fnc ? par-1 : par;
    if (parnum>=0)
    { stmt = stmt + (firstpar ? wxT(":") : wxT(",:"));  firstpar=false;
      stmt = stmt + (pars[par].direction==(int)MODE_IN ? wxT("<") : pars[par].direction==(int)MODE_OUT ? wxT(">") : wxEmptyString);
      stmt = stmt + wxString(pars[par].parname, *cdp->sys_conv);
    }
  }
  stmt = stmt + wxT(")");
 // execute:
  if (debugged)
  { cdp_t cdp2 = clone_connection(cdp);
    if (!cdp2) return false;
   // start debugging:
    the_server_debugger = new server_debugger(cdp);
    if (the_server_debugger)
    { the_server_debugger->dlg1=this;
      if (the_server_debugger->start_debugging(cdp2, &stmt))
        return true;  // cdp2 is now owned by the_server_debugger
      delete the_server_debugger;
      the_server_debugger=NULL;
      unpos_box(_("A debug session is already in progress.")); 
    }
    wx_disconnect(cdp2);
    delete cdp2;
    return false;
  }
  else
  { clear_results();
    Disable();  wxGetApp().Yield();
    { wxBusyCursor wait;
      if (cd_SQL_host_execute(cdp, stmt.mb_str(*cdp->sys_conv), NULL, hvars, par_cnt))
        { cd_Signalize2(cdp, wxGetApp().frame);  Enable();  return false; }
      display_results();
    }  
    Enable();
    check_open_transaction(cdp, wxGetApp().frame);  
  }
  return true;
}

void CallRoutineDlg::clear_results(void)
{// Clear the contents of OUT parameters
  for (int par=0;  par<par_cnt;  par++)
  { int parnum = is_fnc ? par-1 : par;
    if (pars[par].direction!=(int)MODE_IN && pars[par].direction!=(int)MODE_INOUT)  // OUT or function result
      if (parnum==-1) fncval=wxEmptyString;  else pars[par].parval=wxEmptyString;
  }
  mGrid->ForceRefresh();  // shows the updated values
}

void CallRoutineDlg::display_results(void)
{// restore the dialog (may have been minimized by the user):
  if (IsIconized()) Iconize(false);
 // display the values of OUT parameters and the return value:
  for (int par=0;  par<par_cnt;  par++)
  { int parnum = is_fnc ? par-1 : par;
    if (pars[par].direction!=(int)MODE_IN)  // OUT, INOUT or function result
    { int tp=pars[par].data_type;
     // limit the result size:
      if (tp==ATT_TEXT) 
        if (pars[par].specif.wide_char)
        { if (hvars[par].actlen > 2*MAX_FIXED_STRING_LENGTH) hvars[par].actlen = 2*MAX_FIXED_STRING_LENGTH;
          ((wuchar*)hvars[par].buf)[hvars[par].actlen/2]=0;
        }
        else
        { if (hvars[par].actlen > MAX_FIXED_STRING_LENGTH) hvars[par].actlen = MAX_FIXED_STRING_LENGTH;
          ((char  *)hvars[par].buf)[hvars[par].actlen  ]=0;
        }
      else if (tp==ATT_NOSPEC || tp==ATT_RASTER)
      { if (hvars[par].actlen > MAX_FIXED_STRING_LENGTH/2) hvars[par].actlen = MAX_FIXED_STRING_LENGTH/2;
        pars[par].specif.length = (uns16)hvars[par].actlen;  // must be defined!
        //tp=ATT_BINARY;  // convert2string cannot be used for ATT_NOSPEC or ATT_RASTER
      }
      wxChar valbuf[MAX_FIXED_STRING_LENGTH+1];
      //convert2string(hvars[par].buf, tp, valbuf, SQL_LITERAL_PREZ, pars[par].specif);
      //if (parnum==-1) fncval=wxString(valbuf, *cdp->sys_conv);  else pars[par].parval=wxString(valbuf, *cdp->sys_conv);
      int err;
      wxString string_format = get_grid_default_string_format(tp);
      err=superconv(tp, ATT_STRING, hvars[par].buf, (char*)valbuf, pars[par].specif, wx_string_spec, string_format.mb_str(*wxConvCurrent));
      if (parnum==-1) fncval=wxString(valbuf);  else pars[par].parval=wxString(valbuf);
    }
  }
  mGrid->ForceRefresh();  // shows the updated values
  wxGetApp().frame->Raise();  // if the main window is not active, it will start blinking
  wxGetApp().Yield();  // this prevents the info_box() below to be obscured by a popup window (e.g. the editor), Raise() is not sufficient.
  info_box(_("Routine has been executed"), wxGetApp().frame);
 // change Cancel button to Close
  mCancel->SetLabel(_("Close"));
}

void CallRoutineDlg::OnClose(wxCloseEvent & event)
{ if (event.CanVeto() && the_server_debugger)
  { event.Veto();
    unpos_box(_("Please finish the debug session."));
  }
  else Destroy();
}

CallRoutineDlg::~CallRoutineDlg(void)
{ delete [] pars;
  for (trecnum r=0;  r<par_cnt;  r++)  corefree(hvars[r].buf);
  corefree(hvars);
  counter--;
  global_inst=NULL;
}

bool CallRoutineDlg::debug_mode = false;
int CallRoutineDlg::counter = 0;
CallRoutineDlg * CallRoutineDlg::global_inst = NULL;

// Do not use this dialog as parent for modal dialog - other window is activate when the modal dialog is closed on MSW!

/*!
 * Get bitmap resources
 */

wxBitmap CallRoutineDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CallRoutineDlg bitmap retrieval
    return wxNullBitmap;
////@end CallRoutineDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CallRoutineDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CallRoutineDlg icon retrieval
    return wxNullIcon;
////@end CallRoutineDlg icon retrieval
}
