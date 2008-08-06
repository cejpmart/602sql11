/////////////////////////////////////////////////////////////////////////////
// Name:        ProfilerDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/20/06 11:41:13
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ProfilerDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "ProfilerDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ProfilerDlg type definition
 */

IMPLEMENT_CLASS( ProfilerDlg, wxDialog )

/*!
 * ProfilerDlg event table definition
 */

BEGIN_EVENT_TABLE( ProfilerDlg, wxDialog )

////@begin ProfilerDlg event table entries
    EVT_CHECKBOX( CD_PROFILE_ALL, ProfilerDlg::OnCdProfileAllClick )

    EVT_BUTTON( CD_PROFILE_REFRESH, ProfilerDlg::OnCdProfileRefreshClick )

    EVT_GRID_CMD_CELL_CHANGE( CD_PROFILE_LIST, ProfilerDlg::OnCdProfileListCellChange )

    EVT_CHECKBOX( CD_PROFILE_LINES, ProfilerDlg::OnCdProfileLinesClick )

    EVT_BUTTON( CD_PROFILE_CLEAR, ProfilerDlg::OnCdProfileClearClick )

    EVT_BUTTON( CD_PROFILE_SHOW, ProfilerDlg::OnCdProfileShowClick )

    EVT_BUTTON( CD_PROFILE_SHOW_LINES, ProfilerDlg::OnCdProfileShowLinesClick )

////@end ProfilerDlg event table entries
    EVT_BUTTON( wxID_CANCEL, ProfilerDlg::OnCdProfileCancel )

END_EVENT_TABLE()

/*!
 * ProfilerDlg constructors
 */

ProfilerDlg::ProfilerDlg(cdp_t cdpIn)
{ cdp=cdpIn;  lock=0;
}

ProfilerDlg::~ProfilerDlg(void)
{ profiler_dlg=NULL; }

/*!
 * ProfilerDlg creator
 */

bool ProfilerDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ProfilerDlg member initialisation
    mAll = NULL;
    mSpecif = NULL;
    mRefresh = NULL;
    mGridSizer = NULL;
    mGrid = NULL;
    mLines = NULL;
    mClear = NULL;
    mShow = NULL;
    mShowLines = NULL;
////@end ProfilerDlg member initialisation

////@begin ProfilerDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ProfilerDlg creation
    return TRUE;
}

/*!
 * Control creation for ProfilerDlg
 */

void ProfilerDlg::CreateGrid(void)
{
  mGrid->BeginBatch();
  mGrid->SetColLabelSize(default_font_height);
  mGrid->SetDefaultRowSize(default_font_height);
  mGrid->SetRowLabelSize(0);
  mGrid->SetLabelFont(system_gui_font);
 // define columns:
  mGrid->DeleteCols(0, mGrid->GetNumberCols());
  mGrid->AppendCols(5);
  mGrid->SetColLabelValue(0, _("Profile"));
  mGrid->SetColLabelValue(1, _("Logged in user name"));
  mGrid->SetColLabelValue(2, _("Schema name"));
  mGrid->SetColLabelValue(3, _("Session number"));
  mGrid->SetColLabelValue(4, _("IP Address"));
  DefineColSize(mGrid, 0, 0);
  DefineColSize(mGrid, 1, 10);
  DefineColSize(mGrid, 2, 10);
  DefineColSize(mGrid, 3, 0);
  DefineColSize(mGrid, 4, 0);
 // read only:
  wxGridCellAttr * col_attr = new wxGridCellAttr;
  col_attr->SetReadOnly(true);
  mGrid->SetColAttr(1, col_attr);  // takes the ownership!! Must not reuse!!
  col_attr = new wxGridCellAttr;
  col_attr->SetReadOnly(true);
  mGrid->SetColAttr(2, col_attr);  // takes the ownership!! Must not reuse!!
  col_attr = new wxGridCellAttr;
  col_attr->SetReadOnly(true);
  mGrid->SetColAttr(3, col_attr);  // takes the ownership!! Must not reuse!!
  col_attr = new wxGridCellAttr;
  col_attr->SetReadOnly(true);
  mGrid->SetColAttr(4, col_attr);  // takes the ownership!! Must not reuse!!
 // check box:
  col_attr = new wxGridCellAttr;
  wxGridCellBoolEditor * bool_editor = new wxGridCellBoolEditor();
  col_attr->SetEditor(bool_editor);
  wxGridCellBoolRenderer * bool_renderer = new wxGridCellBoolRenderer();
  col_attr->SetRenderer(bool_renderer);  // takes the ownership
  mGrid->SetColAttr(0, col_attr);  // takes the ownership!! Must not reuse!!

  mGrid->EnableDragRowSize(false);
  mGrid->EndBatch();
}

void ProfilerDlg::RefreshGrid(void)
{ tcursnum curs;  trecnum cnt, rec;
  mGrid->DeleteRows(0, mGrid->GetNumberRows());
  if (cd_SQL_execute(cdp, "select profiled_thread,login_name,sel_schema_name,session_number,net_address from _iv_logged_users where not worker_thread", (uns32*)&curs))
    cd_Signalize(cdp);
  else
  { cd_Rec_cnt(cdp, curs, &cnt);
    mGrid->AppendRows(cnt);
    for (rec=0;  rec<cnt;  rec++)
    { wbbool prof;  tobjname logname, schemaname;  uns32 session;  char address[30+1];
      cd_Read(cdp, curs, rec, 1, NULL, &prof);
      cd_Read(cdp, curs, rec, 2, NULL, logname);
      cd_Read(cdp, curs, rec, 3, NULL, schemaname);
      cd_Read(cdp, curs, rec, 4, NULL, &session);
      cd_Read(cdp, curs, rec, 5, NULL, address);  // IP address
      mGrid->SetCellValue(rec, 0, prof ? wxT("1") : wxT(""));  // the default FALSE value for the BoolCellEditor is ""!
      mGrid->SetCellValue(rec, 1, wxString(logname, *cdp->sys_conv));
      mGrid->SetCellValue(rec, 2, wxString(schemaname, *cdp->sys_conv));
      wxString sess;
      sess.Printf(wxT("%d"), session);
      mGrid->SetCellValue(rec, 3, sess);
      mGrid->SetCellValue(rec, 4, wxString(address, *wxConvCurrent));
    }
    cd_Close_cursor(cdp, curs);
  }
}

void ProfilerDlg::CreateControls()
{    
////@begin ProfilerDlg content construction
    ProfilerDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mAll = new wxCheckBox;
    mAll->Create( itemDialog1, CD_PROFILE_ALL, _("Profile all threads"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mAll->SetValue(FALSE);
    itemBoxSizer2->Add(mAll, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    mSpecif = new wxStaticText;
    mSpecif->Create( itemDialog1, CD_PROFILE_SPECIF, _("Profile specified threrads:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(mSpecif, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    itemBoxSizer4->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mRefresh = new wxButton;
    mRefresh->Create( itemDialog1, CD_PROFILE_REFRESH, _("Refresh list of threads"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(mRefresh, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mGridSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(mGridSizer, 1, wxGROW|wxALL, 0);

    mGrid = new wxGrid( itemDialog1, CD_PROFILE_LIST, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mGrid->SetDefaultColSize(50);
    mGrid->SetDefaultRowSize(25);
    mGrid->SetColLabelSize(25);
    mGrid->SetRowLabelSize(50);
    mGrid->CreateGrid(5, 5, wxGrid::wxGridSelectCells);
    mGridSizer->Add(mGrid, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mLines = new wxCheckBox;
    mLines->Create( itemDialog1, CD_PROFILE_LINES, _("Profile source lines in routines"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mLines->SetValue(FALSE);
    itemBoxSizer2->Add(mLines, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mClear = new wxButton;
    mClear->Create( itemDialog1, CD_PROFILE_CLEAR, _("Clear Profile"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(mClear, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mShow = new wxButton;
    mShow->Create( itemDialog1, CD_PROFILE_SHOW, _("Show Global Profile"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(mShow, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mShowLines = new wxButton;
    mShowLines->Create( itemDialog1, CD_PROFILE_SHOW_LINES, _("Show Profile of Lines"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(mShowLines, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ProfilerDlg content construction
    SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // otherwise Enter and ESC in the grid cell editors close the dialog
    CreateGrid();
    mGridSizer->SetItemMinSize(mGrid, get_grid_width(mGrid), get_grid_height(mGrid, 3));
    RefreshGrid();
    uns32 state;
    cd_Get_server_info(cdp, OP_GI_PROFILING_ALL, &state, sizeof(state));
    mAll->SetValue(state!=0);
    if (state) { mGrid->Disable();  mSpecif->Disable();  mRefresh->Disable(); }
    cd_Get_server_info(cdp, OP_GI_PROFILING_LINES, &state, sizeof(state));
    mLines->SetValue(state!=0);
}

/*!
 * Should we show tooltips?
 */

bool ProfilerDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ProfilerDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ProfilerDlg bitmap retrieval
    return wxNullBitmap;
////@end ProfilerDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ProfilerDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ProfilerDlg icon retrieval
    return wxNullIcon;
////@end ProfilerDlg icon retrieval
}

void ProfilerDlg::OnCdProfileCancel( wxCommandEvent& event )
{ 
  Destroy();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_PROFILE_ALL
 */

void ProfilerDlg::OnCdProfileAllClick( wxCommandEvent& event )
{ bool on = mAll->GetValue();
  if (cd_SQL_execute(cdp, on ? "call _sqp_profile_all(true)" : "call _sqp_profile_all(false)", NULL))
    cd_Signalize(cdp);
  else
  { mGrid->Enable(!on);  mSpecif->Enable(!on);  mRefresh->Enable(!on);
    event.Skip();
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PROFILE_REFRESH
 */

void ProfilerDlg::OnCdProfileRefreshClick( wxCommandEvent& event )
{
  RefreshGrid();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PROFILE_CLEAR
 */

void ProfilerDlg::OnCdProfileClearClick( wxCommandEvent& event )
{
   if (cd_SQL_execute(cdp, "call _sqp_profile_reset()", NULL))
     cd_Signalize(cdp);
   else
     RefreshGrid();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PROFILE_SHOW_LINES
 */

#include "fsed9.h"

void ProfilerDlg::OnCdProfileShowLinesClick( wxCommandEvent& event )
{
 // get the list of routines:
  tcursnum cursnum;  trecnum cnt, r;  
  if (cd_Open_cursor_direct(cdp, "select object_number from _iv_profile where category_name='Source line' group by object_number", &cursnum))
    cd_Signalize(cdp);
  else
  { cd_Rec_cnt(cdp, cursnum, &cnt);
    if (cnt)
    { tobjnum objnum;
      for (r=0;  r<cnt;  r++)
      { cd_Read(cdp, cursnum, r, 1, NULL, &objnum);
        text_object_editor_type * edd = (text_object_editor_type*)find_editor_by_objnum9(cdp, OBJ_TABLENUM, objnum, OBJ_DEF_ATR, CATEG_PROC, true);
        if (edd)  // unless internal error
          edd->refresh_profile();
      }
    }
    else
      info_box(_("No line number profiling information found"), this);
    cd_Close_cursor(cdp, cursnum);
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PROFILE_SHOW
 */

void ProfilerDlg::OnCdProfileShowClick( wxCommandEvent& event )
{ tcursnum cursnum;
  if (global_profile_grid) 
  { wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND)
    cmd.SetId(TD_CPM_EXIT_UNCOND);
    global_profile_grid->OnCommand(cmd);  // must overwrite global_profile_grid SYNCHRONOUSLY, not later!
  }
  if (cd_SQL_execute(cdp, "select * from _iv_profile", (uns32*)&cursnum))
    cd_Signalize2(cdp, this);
  else
  { wxWindow * frame = NULL;
    DataGrid * grid = new DataGrid(cdp);
    if (!grid || !grid->open_data_grid(NULL, cdp, CATEG_DIRCUR, cursnum, "", false, NULL))  // deletes the grid on error
      { cd_Close_cursor(cdp, cursnum);  return; }
    global_profile_grid = grid;
    grid->inst->idadr = (wxWindow**)&global_profile_grid;
  }
}


/*!
 * wxEVT_GRID_CMD_CELL_CHANGE event handler for CD_PROFILE_LIST
 */

void ProfilerDlg::OnCdProfileListCellChange( wxGridEvent& event )
{ //if (lock) return;
  int row = event.GetRow();
  wxString str = mGrid->GetCellValue(row,0);
  bool prof = !str.IsEmpty() && str[0]!='0';
  unsigned long sess;
  mGrid->GetCellValue(row,3).ToULong(&sess);
  char cmd[100];
  sprintf(cmd, "call _sqp_profile_thread(%d,%s)", sess, prof ? "true" : "false");  // must use %d because it will be assigned into a INT32 formal parameter
  if (cd_SQL_execute(cdp, cmd, NULL))
    { cd_Signalize(cdp);  return; }
  //lock++;
  //RefreshGrid();
  //lock--;
}


void ProfilerDlg::disconnecting(cdp_t dis_cdp)
{ if (dis_cdp==cdp)
    Destroy();
}
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_PROFILE_LINES
 */

void ProfilerDlg::OnCdProfileLinesClick( wxCommandEvent& event )
{ bool on = mLines->GetValue();
  if (cd_SQL_execute(cdp, on ? "call _sqp_profile_lines(true)" : "call _sqp_profile_lines(false)", NULL))
    cd_Signalize(cdp);
  else
  { mShowLines->Enable(on);
    event.Skip();
  }
}


