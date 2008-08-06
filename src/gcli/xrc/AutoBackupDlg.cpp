/////////////////////////////////////////////////////////////////////////////
// Name:        AutoBackupDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/03/04 08:55:00
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "AutoBackupDlg.h"
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

#include "AutoBackupDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * AutoBackupDlg type definition
 */

IMPLEMENT_CLASS( AutoBackupDlg, wxDialog )

/*!
 * AutoBackupDlg event table definition
 */

BEGIN_EVENT_TABLE( AutoBackupDlg, wxDialog )

////@begin AutoBackupDlg event table entries
    EVT_BUTTON( CD_BCK_SELECT, AutoBackupDlg::OnCdBckSelectClick )

    EVT_BUTTON( CD_BCK_UNC, AutoBackupDlg::OnCdBckUncClick )

    EVT_RADIOBUTTON( CD_BCK_NO, AutoBackupDlg::OnCdBckNoSelected )

    EVT_RADIOBUTTON( CD_BCK_PERIOD, AutoBackupDlg::OnCdBckPeriodSelected )

    EVT_RADIOBUTTON( CD_BCK_TIME, AutoBackupDlg::OnCdBckTimeSelected )

    EVT_CHECKBOX( CD_BCK_IS_LIMIT, AutoBackupDlg::OnCdBckIsLimitClick )

    EVT_CHECKBOX( CD_BCK_ZIP, AutoBackupDlg::OnCdBckZipClick )

    EVT_BUTTON( CD_BCK_ZIPMOVE_SEL, AutoBackupDlg::OnCdBckZipmoveSelClick )

    EVT_BUTTON( wxID_OK, AutoBackupDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, AutoBackupDlg::OnCancelClick )

    EVT_BUTTON( CD_BCK_NOW, AutoBackupDlg::OnCdBckNowClick )

////@end AutoBackupDlg event table entries

END_EVENT_TABLE()

/*!
 * AutoBackupDlg constructors
 */

AutoBackupDlg::AutoBackupDlg(cdp_t cdpIn)
{ cdp=cdpIn; }

/*!
 * AutoBackupDlg creator
 */

bool AutoBackupDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin AutoBackupDlg member initialisation
    mSizerContainingGrid = NULL;
    mDir = NULL;
    mSelect = NULL;
    mUNC = NULL;
    mNo = NULL;
    mPeriod = NULL;
    mTime = NULL;
    mHours = NULL;
    mMinutes = NULL;
    mGrid = NULL;
    mNonBlocking = NULL;
    mIsLimit = NULL;
    mLimit = NULL;
    mZip = NULL;
    mZipMove = NULL;
    mZipMoveSel = NULL;
    mOK = NULL;
    mBackupNow = NULL;
////@end AutoBackupDlg member initialisation

////@begin AutoBackupDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end AutoBackupDlg creation
    SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // otherwise Enter and ESC in the grid cell editors close the dialog
    return TRUE;
}

/*!
 * Control creation for AutoBackupDlg
 */
#define backup_days _("- Never -,- Every day -,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday,Sunday,- Work days (Mon-Fri) -,- Weekend (Sat-Sun) -")

bool read_and_display_unsigned(cdp_t cdp, wxTextCtrl * ctrl, const char * valname)
{ sig32 val = 0;
  if (!cd_Get_property_num(cdp, sqlserver_owner, valname, &val))
  { SetIntValue(ctrl, val);
    return val!=0;
  }
  else ctrl->SetValue(wxEmptyString);
  return false;
}

bool read_and_display_grid(cdp_t cdp, wxGrid * grid, int row, int col, const char * valname)
{ sig32 val = 0;
  if (!cd_Get_property_num(cdp, sqlserver_owner, valname, &val))
  { wxString str;  str.Printf(wxT("%u"), val);
    grid->SetCellValue(row, col, str);
    return val!=0;
  }
  else grid->SetCellValue(row, col, wxEmptyString);
  return false;
}


void AutoBackupDlg::CreateControls()
{    
////@begin AutoBackupDlg content construction
    AutoBackupDlg* itemDialog1 = this;

    mSizerContainingGrid = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mSizerContainingGrid);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    mSizerContainingGrid->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Backup directory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mDir = new wxTextCtrl;
    mDir->Create( itemDialog1, CD_BCK_DIR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mDir, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSelect = new wxButton;
    mSelect->Create( itemDialog1, CD_BCK_SELECT, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemBoxSizer3->Add(mSelect, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    mUNC = new wxButton;
    mUNC->Create( itemDialog1, CD_BCK_UNC, _("UNC"), wxDefaultPosition, wxSize(40, -1), 0 );
    itemBoxSizer3->Add(mUNC, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    mSizerContainingGrid->Add(itemBoxSizer8, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer8->Add(itemBoxSizer9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    mNo = new wxRadioButton;
    mNo->Create( itemDialog1, CD_BCK_NO, _("Do not backup"), wxDefaultPosition, wxDefaultSize, 0 );
    mNo->SetValue(FALSE);
    itemBoxSizer9->Add(mNo, 0, wxALIGN_LEFT|wxALL, 5);

    mPeriod = new wxRadioButton;
    mPeriod->Create( itemDialog1, CD_BCK_PERIOD, _("Backup at specified intervals:"), wxDefaultPosition, wxDefaultSize, 0 );
    mPeriod->SetValue(FALSE);
    itemBoxSizer9->Add(mPeriod, 0, wxALIGN_LEFT|wxALL, 5);

    mTime = new wxRadioButton;
    mTime->Create( itemDialog1, CD_BCK_TIME, _("Backup at specified time(s):"), wxDefaultPosition, wxDefaultSize, 0 );
    mTime->SetValue(FALSE);
    itemBoxSizer9->Add(mTime, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer8->Add(itemBoxSizer13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer13->Add(itemBoxSizer14, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxStaticText* itemStaticText15 = new wxStaticText;
    itemStaticText15->Create( itemDialog1, wxID_STATIC, _("every"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText15, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mHours = new wxTextCtrl;
    mHours->Create( itemDialog1, CD_BCK_HOURS, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer14->Add(mHours, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxStaticText* itemStaticText17 = new wxStaticText;
    itemStaticText17->Create( itemDialog1, wxID_STATIC, _("hours and"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText17, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mMinutes = new wxTextCtrl;
    mMinutes->Create( itemDialog1, CD_BCK_MINUTES, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer14->Add(mMinutes, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxStaticText* itemStaticText19 = new wxStaticText;
    itemStaticText19->Create( itemDialog1, wxID_STATIC, _("minutes."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mGrid = new wxGrid( itemDialog1, CD_BCK_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxWANTS_CHARS|wxHSCROLL|wxVSCROLL );
    mGrid->SetDefaultColSize(70);
    mGrid->SetDefaultRowSize(20);
    mGrid->SetColLabelSize(20);
    mGrid->SetRowLabelSize(20);
    mGrid->CreateGrid(4, 3, wxGrid::wxGridSelectCells);
    mSizerContainingGrid->Add(mGrid, 1, wxGROW|wxLEFT|wxRIGHT, 30);

    mSizerContainingGrid->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mNonBlocking = new wxCheckBox;
    mNonBlocking->Create( itemDialog1, CD_BCK_NONBLOCKING, _("Do not block the server operation during backup"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mNonBlocking->SetValue(FALSE);
    if (ShowToolTips())
        mNonBlocking->SetToolTip(_("If checked, clients can work on the SQL server during the backup. If not checked, most client operations are blocked and the backup is faster."));
    mSizerContainingGrid->Add(mNonBlocking, 0, wxALIGN_LEFT|wxALL, 5);

    mIsLimit = new wxCheckBox;
    mIsLimit->Create( itemDialog1, CD_BCK_IS_LIMIT, _("Rotate backup files by removing the oldest"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mIsLimit->SetValue(FALSE);
    mSizerContainingGrid->Add(mIsLimit, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    mSizerContainingGrid->Add(itemBoxSizer24, 0, wxALIGN_LEFT|wxALL, 0);

    wxStaticText* itemStaticText25 = new wxStaticText;
    itemStaticText25->Create( itemDialog1, wxID_STATIC, _("Keep the"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText25, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLimit = new wxTextCtrl;
    mLimit->Create( itemDialog1, CD_BCK_LIMIT, _T(""), wxDefaultPosition, wxSize(40, -1), 0 );
    itemBoxSizer24->Add(mLimit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxStaticText* itemStaticText27 = new wxStaticText;
    itemStaticText27->Create( itemDialog1, wxID_STATIC, _("most recent backup files"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText27, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mZip = new wxCheckBox;
    mZip->Create( itemDialog1, CD_BCK_ZIP, _("Compress (ZIP) each backup file and copy to the folder below:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mZip->SetValue(FALSE);
    mSizerContainingGrid->Add(mZip, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer29 = new wxFlexGridSizer(2, 3, 0, 0);
    itemFlexGridSizer29->AddGrowableCol(1);
    mSizerContainingGrid->Add(itemFlexGridSizer29, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText30 = new wxStaticText;
    itemStaticText30->Create( itemDialog1, wxID_STATIC, _("Compressed backup directory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText30, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mZipMove = new wxTextCtrl;
    mZipMove->Create( itemDialog1, CD_BCK_ZIPMOVE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(mZipMove, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mZipMoveSel = new wxButton;
    mZipMoveSel->Create( itemDialog1, CD_BCK_ZIPMOVE_SEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer29->Add(mZipMoveSel, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText33 = new wxStaticText;
    itemStaticText33->Create( itemDialog1, wxID_STATIC, _("Backup files are named using the date and time of the backup."), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrid->Add(itemStaticText33, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer34 = new wxBoxSizer(wxHORIZONTAL);
    mSizerContainingGrid->Add(itemBoxSizer34, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mOK = new wxButton;
    mOK->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    mOK->SetDefault();
    itemBoxSizer34->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton36 = new wxButton;
    itemButton36->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer34->Add(itemButton36, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mBackupNow = new wxButton;
    mBackupNow->Create( itemDialog1, CD_BCK_NOW, _("Backup Now"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        mBackupNow->SetToolTip(_("Starts the immediate backup of the database file, independent of the regular backups"));
    itemBoxSizer34->Add(mBackupNow, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end AutoBackupDlg content construction
    // must not remove grid methods from above!
#ifndef WINS    
    mUNC->Disable();
#endif    
    SetEscapeId(wxID_NONE);  // prevents the ESC in the grid from closing the dialog
    if (!cd_Am_I_config_admin(cdp))
      mOK->Disable();                                      
    mGrid->SetColLabelValue(0, _("Day"));
    mGrid->SetColLabelValue(1, _("Hour"));
    mGrid->SetColLabelValue(2, _("Minute"));
    mGrid->SetColLabelSize(default_font_height);
    mGrid->SetLabelFont(system_gui_font);
    DefineColSize(mGrid, 0, 128);
    DefineColSize(mGrid, 1, 40);
    DefineColSize(mGrid, 2, 40);
    mGrid->SetDefaultRowSize(default_font_height);
    wxGridCellAttr * col_attr = new wxGridCellAttr;
    wxGridCellMyEnumEditor * editor = new wxGridCellMyEnumEditor(backup_days);
    col_attr->SetEditor(editor);
    wxGridCellEnum2Renderer * renderer = new wxGridCellEnum2Renderer(wxString(backup_days)+wxT(", "));
    col_attr->SetRenderer(renderer);  // takes the ownership
    mGrid->SetColAttr(0, col_attr);
    mSizerContainingGrid->SetItemMinSize(mGrid, 20+128+40+40+7+wxSystemSettings::GetMetric(wxSYS_VSCROLL_X), 
                                                5*default_font_height+7+wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y));
   // show contents:
    char direc[MAX_PATH];
    if (cd_Get_property_value(cdp, sqlserver_owner, "BackupDirectory", 0, direc, sizeof(direc)))
      *direc=0;
    mDir->SetValue(wxString(direc, *wxConvCurrent));
    mIsLimit->SetValue(read_and_display_unsigned(cdp, mLimit, "BackupFilesLimit"));
    bool is_peri, is_time;
    is_peri =read_and_display_unsigned(cdp, mHours,   "BackupIntervalHours");
    is_peri|=read_and_display_unsigned(cdp, mMinutes, "BackupIntervalMinutes");
    read_and_display_grid(cdp, mGrid, 0, 1, "BackupTimeHour1");
    read_and_display_grid(cdp, mGrid, 0, 2, "BackupTimeMin1");
    is_time =read_and_display_grid   (cdp, mGrid, 0, 0, "BackupTimeDay1");
    read_and_display_grid(cdp, mGrid, 1, 1, "BackupTimeHour2");
    read_and_display_grid(cdp, mGrid, 1, 2, "BackupTimeMin2");
    is_time|=read_and_display_grid   (cdp, mGrid, 1, 0, "BackupTimeDay2");
    read_and_display_grid(cdp, mGrid, 2, 1, "BackupTimeHour3");
    read_and_display_grid(cdp, mGrid, 2, 2, "BackupTimeMin3");
    is_time|=read_and_display_grid   (cdp, mGrid, 2, 0, "BackupTimeDay3");
    read_and_display_grid(cdp, mGrid, 3, 1, "BackupTimeHour4");
    read_and_display_grid(cdp, mGrid, 3, 2, "BackupTimeMin4");
    is_time|=read_and_display_grid   (cdp, mGrid, 3, 0, "BackupTimeDay4");
    if (is_peri) mPeriod->SetValue(true);
    else if (is_time) mTime->SetValue(true);
    else mNo->SetValue(true);
    sig32 val = 0;
    cd_Get_property_num(cdp, sqlserver_owner, "BackupZip", &val);
    mZip->SetValue(val!=0);
    if (cd_Get_property_value(cdp, sqlserver_owner, "BackupZipMoveTo", 0, direc, sizeof(direc)))
      *direc=0;
    mZipMove->SetValue(wxString(direc, *wxConvCurrent));
    cd_Get_property_num(cdp, sqlserver_owner, "NonblockingBackups", &val);
    orig_nonblocking = val!=0;
    mNonBlocking->SetValue(orig_nonblocking);
    backup_enabling();
}

void AutoBackupDlg::backup_enabling(void)
{ bool ena = mPeriod->GetValue();
  mHours->Enable(ena);
  mMinutes->Enable(ena);
  ena = mTime->GetValue();
  mGrid->Enable(ena);
  mGrid->Refresh();  // otherwise changing the state is not immediately visible
  ena = mZip->GetValue();
  mZipMove->Enable(ena);
  mZipMoveSel->Enable(ena);
  mLimit->Enable(mIsLimit->GetValue());
}


/*!
 * Should we show tooltips?
 */

bool AutoBackupDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_BCK_NO
 */

void AutoBackupDlg::OnCdBckNoSelected( wxCommandEvent& event )
{
  backup_enabling();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_BCK_PERIOD
 */

void AutoBackupDlg::OnCdBckPeriodSelected( wxCommandEvent& event )
{
  backup_enabling();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_BCK_TIME
 */

void AutoBackupDlg::OnCdBckTimeSelected( wxCommandEvent& event )
{
  backup_enabling();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_BCK_SELECT
 */

void AutoBackupDlg::OnCdBckSelectClick( wxCommandEvent& event )
{ 
  if (!remote_operation_confirmed(cdp, this)) 
    return;
  wxString newdir = wxDirSelector(_("Choose a directory"), mDir->GetValue(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!newdir.IsEmpty())
    mDir->SetValue(newdir);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_BCK_ZIPMOVE_SEL
 */

void AutoBackupDlg::OnCdBckZipmoveSelClick( wxCommandEvent& event )
{
  if (!remote_operation_confirmed(cdp, this)) 
    return;
  wxString newdir = wxDirSelector(_("Choose a directory"), mZipMove->GetValue(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!newdir.IsEmpty())
    mZipMove->SetValue(newdir);
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_BCK_ZIP
 */

void AutoBackupDlg::OnCdBckZipClick( wxCommandEvent& event )
{
  backup_enabling();
  event.Skip();
}

/* Property value interpretation:
If [BackupIntervalHours] or [BackupIntervalMinutes] is specified and not zero, periodic backup is used
Else if any [BackupTimeDayX] is specified and not zero the the backup at specified time(s) is used
Else no automatic backup is performed.
*/
bool save_backup_time(cdp_t cdp, wxGrid * grid, int row)
{ unsigned long day;
  char labelh[] = "BackupTimeHour1";
  char labelm[] = "BackupTimeMin1";
  char labeld[] = "BackupTimeDay1";
  labelh[14]='1'+row;
  labelm[13]='1'+row;
  labeld[13]='1'+row;
  if (grid->GetCellValue(row, 0).ToULong(&day) && day>0) // unless "Never" selected
  { unsigned long minutes, hours;
    if (!grid->GetCellValue(row, 1).ToULong(&hours) || hours<0 || hours>=24) 
      return false;
    if (!grid->GetCellValue(row, 2).ToULong(&minutes) || minutes<0 || minutes>=60) 
      return false;
    cd_Set_property_num(cdp, sqlserver_owner, labelh, hours);
    cd_Set_property_num(cdp, sqlserver_owner, labelm, minutes);
    cd_Set_property_num(cdp, sqlserver_owner, labeld, day);
  }
  else 
  { cd_Set_property_value(cdp, sqlserver_owner, labelh, 0, "");
    cd_Set_property_value(cdp, sqlserver_owner, labelm, 0, "");
    cd_Set_property_value(cdp, sqlserver_owner, labeld, 0, "");
  }
  return true;
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void AutoBackupDlg::OnOkClick( wxCommandEvent& event )
// Process first the fields which need validating.
{ wxString dir;
  dir=mDir->GetValue();  dir.Trim(false);  dir.Trim(true);
  if (dir.IsEmpty() && (mPeriod->GetValue() || mTime->GetValue()))
    { error_box(_("Backup directory is not specified!"), this);  return; }
  if (mGrid->IsCellEditControlEnabled()) mGrid->DisableCellEditControl(); // commit cell edit
 // period parameters: 
  if (mPeriod->GetValue())
  { unsigned long hours, minutes;  
    if (!mHours->GetValue().ToULong(&hours) || hours<0 || hours>=24)
      { mHours->SetFocus();  wxBell();  return; }
    if (!mMinutes->GetValue().ToULong(&minutes) || minutes<0 || minutes>=60)
      { mMinutes->SetFocus();  wxBell();  return; }
    cd_Set_property_num(cdp, sqlserver_owner, "BackupIntervalHours",   hours);
    cd_Set_property_num(cdp, sqlserver_owner, "BackupIntervalMinutes", minutes);
  }
  else
  { cd_Set_property_value(cdp, sqlserver_owner, "BackupIntervalHours",   0, "");
    cd_Set_property_value(cdp, sqlserver_owner, "BackupIntervalMinutes", 0, "");
  }
 // time parameters
  if (mTime->GetValue())
  { for (int i=0;  i<4;  i++)
      if (!save_backup_time(cdp, mGrid, i))
        { mGrid->SetFocus();  wxBell();  return; }
  }
  else
  { cd_Set_property_value(cdp, sqlserver_owner, "BackupTimeDay1", 0, "");
    cd_Set_property_value(cdp, sqlserver_owner, "BackupTimeDay2", 0, "");
    cd_Set_property_value(cdp, sqlserver_owner, "BackupTimeDay3", 0, "");
    cd_Set_property_value(cdp, sqlserver_owner, "BackupTimeDay4", 0, "");
  }
 // save directory: 
  cd_Set_property_value(cdp, sqlserver_owner, "BackupDirectory", 0, dir.mb_str(*wxConvCurrent));
 // save max number of files (0 or empty: deletes the property and removes the limit):
  unsigned long maxfiles;
  if (mIsLimit->GetValue() && mLimit->GetValue().ToULong(&maxfiles) && maxfiles>=0)
    cd_Set_property_num(cdp, sqlserver_owner, "BackupFilesLimit", maxfiles);
  else cd_Set_property_value(cdp, sqlserver_owner, "BackupFilesLimit", 0, "");
 // zip:
  cd_Set_property_value(cdp, sqlserver_owner, "BackupZip", 0, mZip->GetValue() ? "1" : "0");
  dir=mZipMove->GetValue();  dir.Trim(false);  dir.Trim(true);
  cd_Set_property_value(cdp, sqlserver_owner, "BackupZipMoveTo", 0, dir.mb_str(*wxConvCurrent));
 // non-blocking (not saving if not changed, the default value differs in various server versions):
  bool nonblocking = mNonBlocking->GetValue();
  if (nonblocking!=orig_nonblocking)
  { cd_Set_property_value(cdp, sqlserver_owner, "NonblockingBackups", 0, nonblocking ? "1" : "0");
    orig_nonblocking=nonblocking;  // do not save it again, if called from OnCdBckNowClick()
  }
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void AutoBackupDlg::OnCancelClick( wxCommandEvent& event )
{
    EndModal(0);
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_BCK_IS_LIMIT
 */

void AutoBackupDlg::OnCdBckIsLimitClick( wxCommandEvent& event )
{
  backup_enabling();
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap AutoBackupDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin AutoBackupDlg bitmap retrieval
    return wxNullBitmap;
////@end AutoBackupDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon AutoBackupDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin AutoBackupDlg icon retrieval
    return wxNullIcon;
////@end AutoBackupDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_BCK_NOW
 */

void AutoBackupDlg::OnCdBckNowClick( wxCommandEvent& event )
{ wxString dir;
 // check the directory:
  dir=mDir->GetValue();  dir.Trim(false);  dir.Trim(true);
  if (dir.IsEmpty())
    { error_box(_("Backup directory is not specified!"), this);  return; }
 // save changes:
  OnOkClick(event);
 // backup now:
  BOOL res;
  { wxBusyCursor wait;
    res=cd_Backup_database_file(cdp, "");
  }
  if (res)
    cd_Signalize2(cdp, this);
  else 
    EndModal(2);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_BCK_UNC
 */

void AutoBackupDlg::OnCdBckUncClick( wxCommandEvent& event )
{
  UNCBackupDlg dlg(cdp);
  dlg.Create(this);
  dlg.ShowModal();
}


