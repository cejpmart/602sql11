/////////////////////////////////////////////////////////////////////////////
// Name:        NewLocalServer.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/12/04 16:14:16
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "NewLocalServer.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "NewLocalServer.h"

////@begin XPM images
////@end XPM images

/*!
 * NewLocalServer type definition
 */

IMPLEMENT_CLASS( NewLocalServer, wxDialog )

/*!
 * NewLocalServer event table definition
 */

BEGIN_EVENT_TABLE( NewLocalServer, wxDialog )

////@begin NewLocalServer event table entries
    EVT_BUTTON( CD_REGLOC_SELPATH, NewLocalServer::OnCdReglocSelpathClick )

    EVT_RADIOBUTTON( CD_REGLOC_DEFPORT, NewLocalServer::OnCdReglocDefportSelected )

    EVT_RADIOBUTTON( CD_REGLOC_SPECPORT, NewLocalServer::OnCdReglocSpecportSelected )

    EVT_BUTTON( wxID_OK, NewLocalServer::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, NewLocalServer::OnCancelClick )

    EVT_BUTTON( wxID_HELP, NewLocalServer::OnHelpClick )

////@end NewLocalServer event table entries

END_EVENT_TABLE()

/*!
 * NewLocalServer constructors
 */

NewLocalServer::NewLocalServer( )
{
}

NewLocalServer::NewLocalServer( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * NewLocalServer creator
 */

bool NewLocalServer::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NewLocalServer member initialisation
    mServerName = NULL;
    mPath = NULL;
    mCharset = NULL;
    mBig = NULL;
    mDefPort = NULL;
    mSpecPort = NULL;
    mPort = NULL;
////@end NewLocalServer member initialisation

////@begin NewLocalServer creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end NewLocalServer creation
    return TRUE;
}

/*!
 * Control creation for NewLocalServer
 */

//"Allows the size data in the table (without LOBs and indexes) to be bigger than 4GB."

void NewLocalServer::CreateControls()
{    
////@begin NewLocalServer content construction
    NewLocalServer* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(3, 3, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("&Name for the server:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mServerName = new wxTextCtrl;
    mServerName->Create( itemDialog1, CD_REGLOC_SERVER_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mServerName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("Database &file directory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPath = new wxTextCtrl;
    mPath->Create( itemDialog1, CD_REGLOC_PATH, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    itemFlexGridSizer3->Add(mPath, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, CD_REGLOC_SELPATH, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer3->Add(itemButton9, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText10 = new wxStaticText;
    itemStaticText10->Create( itemDialog1, wxID_STATIC, _("System charset:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mCharsetStrings = NULL;
    mCharset = new wxOwnerDrawnComboBox;
    mCharset->Create( itemDialog1, ID_REGLOC_CHARSET, _T(""), wxDefaultPosition, wxDefaultSize, 0, mCharsetStrings, wxCB_READONLY );
    itemFlexGridSizer3->Add(mCharset, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText13 = new wxStaticText;
    itemStaticText13->Create( itemDialog1, wxID_STATIC, _("NOTE: If the directory already contains a database file, the server will use this file."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText13, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    mBig = new wxCheckBox;
    mBig->Create( itemDialog1, CD_REGLOC_BIG, _("Allow tables bigger than 4GB"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mBig->SetValue(FALSE);
    itemBoxSizer2->Add(mBig, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer15Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Port for the SQL Server"));
    wxStaticBoxSizer* itemStaticBoxSizer15 = new wxStaticBoxSizer(itemStaticBoxSizer15Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer15, 0, wxGROW|wxALL, 5);

    mDefPort = new wxRadioButton;
    mDefPort->Create( itemDialog1, CD_REGLOC_DEFPORT, _("SQL server works on the &default port number (5001)"), wxDefaultPosition, wxDefaultSize, 0 );
    mDefPort->SetValue(FALSE);
    itemStaticBoxSizer15->Add(mDefPort, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer15->Add(itemBoxSizer17, 0, wxGROW|wxALL, 0);

    mSpecPort = new wxRadioButton;
    mSpecPort->Create( itemDialog1, CD_REGLOC_SPECPORT, _("SQL server works on &port:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSpecPort->SetValue(FALSE);
    itemBoxSizer17->Add(mSpecPort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mPort = new wxTextCtrl;
    mPort->Create( itemDialog1, CD_REGLOC_PORT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(mPort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText20 = new wxStaticText;
    itemStaticText20->Create( itemDialog1, wxID_STATIC, _("Only servers created/registered by the administrator are visible to all other users."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText20, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer21, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton22 = new wxButton;
    itemButton22->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton22->SetDefault();
    itemBoxSizer21->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton23 = new wxButton;
    itemButton23->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemButton23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton24 = new wxButton;
    itemButton24->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemButton24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end NewLocalServer content construction
  mServerName->SetMaxLength(OBJ_NAME_LEN);
  mPath->SetMaxLength(MAX_PATH);
  mDefPort->SetValue(true);
  mPort->Disable();
  mCharset->Append(_("English (ASCII)"), (void*)255);  // server will convert it to 0 on each start
  if (charset_available(1)) mCharset->Append(_("Czech+Slovak Windows"), (void*)1);
  if (charset_available(2)) mCharset->Append(_("Polish Windows"), (void*)2);
  if (charset_available(3)) mCharset->Append(_("French Windows"), (void*)3);
  if (charset_available(4)) mCharset->Append(_("German Windows"), (void*)4);
  if (charset_available(5)) mCharset->Append(_("Italian Windows"),(void*)5);
  if (charset_available(128+1)) mCharset->Append(_("Czech+Slovak ISO"), (void*)(128+1));
  if (charset_available(128+2)) mCharset->Append(_("Polish ISO"), (void*)(128+2));
  if (charset_available(128+3)) mCharset->Append(_("French ISO"), (void*)(128+3));
  if (charset_available(128+4)) mCharset->Append(_("German ISO"), (void*)(128+4));
  if (charset_available(128+5)) mCharset->Append(_("Italian ISO"),(void*)(128+5));
  mCharset->SetSelection(0);
}

/*!
 * Should we show tooltips?
 */

bool NewLocalServer::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void NewLocalServer::OnOkClick( wxCommandEvent& event )
// Copies data from controls to variables and validates them.
{// server name: 
  server_name=mServerName->GetValue();
  server_name.Trim(false);  server_name.Trim(true);
  if (server_name.IsEmpty())
    { unpos_box(_("The server name cannot be empty."));  return; }
  if (!is_object_name(NULL, server_name, this)) return;
  const wxChar * nm = server_name;
  while (*nm)
  { if (*nm>0x7f)  // must not contain non-ascii chars
      { unpos_box(_("Please, do not use the non-ascii characters in the server name."), this);  return; }
    nm++;
  }
  if (IsServerRegistered(server_name.mb_str(wxConvLocal)))
    { error_box(SERVER_NAME_DUPLICITY, this);  return; }
 // path:
  path=mPath->GetValue();
  path.Trim(false);  path.Trim(true);
  if (path.IsEmpty())
    { unpos_box(_("The database directory path must be specified."), this);  return; }
  if (!wxDirExists(path))
    if (!wxMkdir(path))
      { error_box(_("Cannot create the database directory."), this);  return; }
#ifdef LINUX
  chmod(path.mb_str(*wxConvCurrent), S_IRUSR|S_IWUSR|S_IXUSR| S_IRGRP|S_IWGRP|S_IXGRP| S_IROTH|S_IWOTH|S_IXOTH| S_ISVTX);
  // reduces the startup problems, admin may change it
#endif 
 // port:
  if (mDefPort->GetValue())
  { default_port=true;
    port_number=5001;
  }
  else
  { default_port=false;
    if (!mPort->GetValue().ToULong((unsigned long*)&port_number) || port_number>65535)
      { error_box(_("A valid port number must be specified."), this);  return; }
  }
  if (!is_port_unique(port_number, ""))
    if (!yesno_box(_("There is another local server with the same port number.\nThese servers will be inaccessible when running simultaneously.\nDo you want to continue anyway?"), this)) return;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void NewLocalServer::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void NewLocalServer::OnCdReglocSelpathClick( wxCommandEvent& event )
{ wxString str = wxDirSelector(_("Select Database Directory"), mPath->GetValue().c_str(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!str.IsEmpty()) mPath->SetValue(str);
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_REGLOC_DEFPORT
 */

void NewLocalServer::OnCdReglocDefportSelected( wxCommandEvent& event )
{ mPort->Disable();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_REGLOC_SPECPORT
 */

void NewLocalServer::OnCdReglocSpecportSelected( wxCommandEvent& event )
{ mPort->Enable();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void NewLocalServer::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/dlgcre_new_serv.html"));
}



/*!
 * Get bitmap resources
 */

wxBitmap NewLocalServer::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NewLocalServer bitmap retrieval
    return wxNullBitmap;
////@end NewLocalServer bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NewLocalServer::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NewLocalServer icon retrieval
    return wxNullIcon;
////@end NewLocalServer icon retrieval
}
