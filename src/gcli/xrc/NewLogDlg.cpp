/////////////////////////////////////////////////////////////////////////////
// Name:        NewLogDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/01/04 14:32:15
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "NewLogDlg.h"
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

#include "NewLogDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * NewLogDlg type definition
 */

IMPLEMENT_CLASS( NewLogDlg, wxDialog )

/*!
 * NewLogDlg event table definition
 */

BEGIN_EVENT_TABLE( NewLogDlg, wxDialog )

////@begin NewLogDlg event table entries
    EVT_BUTTON( CD_NEWLOG_SELFILE, NewLogDlg::OnCdNewlogSelfileClick )

    EVT_BUTTON( wxID_OK, NewLogDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, NewLogDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, NewLogDlg::OnHelpClick )

////@end NewLogDlg event table entries

END_EVENT_TABLE()

/*!
 * NewLogDlg constructors
 */

NewLogDlg::NewLogDlg(cdp_t cdpIn)
{ cdp=cdpIn; }

/*!
 * NewLogDlg creator
 */

bool NewLogDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NewLogDlg member initialisation
    mLogName = NULL;
    mLogFile = NULL;
    mLogFormat = NULL;
////@end NewLogDlg member initialisation

////@begin NewLogDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end NewLogDlg creation
    return TRUE;
}

/*!
 * Control creation for NewLogDlg
 */

void NewLogDlg::CreateControls()
{    
////@begin NewLogDlg content construction
    NewLogDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(3, 3, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Log &name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLogName = new wxTextCtrl;
    mLogName->Create( itemDialog1, CD_NEWLOG_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mLogName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("&Log file:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mLogFile = new wxTextCtrl;
    mLogFile->Create( itemDialog1, CD_NEWLOG_FILE, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    itemFlexGridSizer3->Add(mLogFile, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, CD_NEWLOG_SELFILE, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer3->Add(itemButton9, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText10 = new wxStaticText;
    itemStaticText10->Create( itemDialog1, wxID_STATIC, _("&Format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mLogFormat = new wxTextCtrl;
    mLogFormat->Create( itemDialog1, CD_NEWLOG_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mLogFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer13, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton14 = new wxButton;
    itemButton14->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton15 = new wxButton;
    itemButton15->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton16 = new wxButton;
    itemButton16->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end NewLogDlg content construction
  mLogName->SetMaxLength(OBJ_NAME_LEN);
}

/*!
 * Should we show tooltips?
 */

bool NewLogDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_NEWLOG_SELFILE
 */

void NewLogDlg::OnCdNewlogSelfileClick( wxCommandEvent& event )
{
  wxString str = GetExportFileName(mLogFile->GetValue().c_str(), _("Text files (*.txt)|*.txt|"), this);
  if (!str.IsEmpty())
    mLogFile->SetValue(str);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void NewLogDlg::OnOkClick( wxCommandEvent& event )
{ wxString name, file, format;
  name  = mLogName  ->GetValue();  name.Trim(false);  name.Trim(true);
  file  = mLogFile  ->GetValue();
  format= mLogFormat->GetValue();
  if (name.IsEmpty()) { wxBell();  return; }
  wxString cmd;
  cmd.Printf(wxT("call _sqp_define_log(\'%s\',\'%s\',\'%s\')"), name.c_str(), file.c_str(), format.c_str());
  if (cd_SQL_execute(cdp, cmd.mb_str(*cdp->sys_conv), NULL)) { cd_Signalize(cdp);  return; }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void NewLogDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void NewLogDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/tracelog.html"));
}



/*!
 * Get bitmap resources
 */

wxBitmap NewLogDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NewLogDlg bitmap retrieval
    return wxNullBitmap;
////@end NewLogDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NewLogDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NewLogDlg icon retrieval
    return wxNullIcon;
////@end NewLogDlg icon retrieval
}
