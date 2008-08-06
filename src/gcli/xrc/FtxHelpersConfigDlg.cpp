/////////////////////////////////////////////////////////////////////////////
// Name:        FtxHelpersConfigDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     07/15/05 13:51:40
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "FtxHelpersConfigDlg.h"
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

#include "FtxHelpersConfigDlg.h"

// include names of regdb keys and values
#include <ftxhlpkeys.h>

////@begin XPM images
////@end XPM images

/*!
 * FtxHelpersConfigDlg type definition
 */

IMPLEMENT_DYNAMIC_CLASS( FtxHelpersConfigDlg, wxDialog )

/*!
 * FtxHelpersConfigDlg event table definition
 */

BEGIN_EVENT_TABLE( FtxHelpersConfigDlg, wxDialog )

////@begin FtxHelpersConfigDlg event table entries
    EVT_RADIOBUTTON( ID_OOO_STANDALONE_RADIOBUTTON, FtxHelpersConfigDlg::OnOooStandaloneRadiobuttonSelected )

    EVT_BUTTON( ID_OOO_BINARY_BROWSE_BUTTON, FtxHelpersConfigDlg::OnOooBinaryBrowseButtonClick )

    EVT_RADIOBUTTON( ID_OOO_SERVER_RADIOBUTTON, FtxHelpersConfigDlg::OnOooServerRadiobuttonSelected )

    EVT_BUTTON( ID_BUTTON, FtxHelpersConfigDlg::OnBrowseOOOProgramDirButtonClick )

    EVT_BUTTON( wxID_OK, FtxHelpersConfigDlg::OnOkClick )

    EVT_BUTTON( wxID_HELP, FtxHelpersConfigDlg::OnHelpClick )

////@end FtxHelpersConfigDlg event table entries

END_EVENT_TABLE()

/*!
 * FtxHelpersConfigDlg constructors
 */

FtxHelpersConfigDlg::FtxHelpersConfigDlg( )
{
}

FtxHelpersConfigDlg::FtxHelpersConfigDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * FtxHelpersConfigDlg creator
 */

bool FtxHelpersConfigDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin FtxHelpersConfigDlg member initialisation
    mDOC = NULL;
    mXLS = NULL;
    mPPT = NULL;
    mRTF = NULL;
    mOOOStandalone = NULL;
    mSoffice = NULL;
    mBrowseSoffice = NULL;
    mOOOServer = NULL;
    mHostname = NULL;
    mPort = NULL;
    mOOOProgramDir = NULL;
    mBrowseOOOProgramDir = NULL;
////@end FtxHelpersConfigDlg member initialisation

////@begin FtxHelpersConfigDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end FtxHelpersConfigDlg creation
    if( !LoadParams() )
    {
      wxWindow *tmp=this->FindWindow(wxID_OK);
      if( tmp!=NULL ) tmp->Disable();
    }
    return TRUE;
}

/*!
 * Control creation for FtxHelpersConfigDlg
 */

void FtxHelpersConfigDlg::CreateControls()
{    
////@begin FtxHelpersConfigDlg content construction
    FtxHelpersConfigDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Fulltext plug-in for document types"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText4 = new wxStaticText( itemDialog1, wxID_STATIC, _("Decide which fulltext plug-in must be used for a particular document type.\nFulltext plug-ins are external applications that are used to extract words from documents."), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxFlexGridSizer* itemFlexGridSizer5 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer5->AddGrowableCol(1);
    itemStaticBoxSizer3->Add(itemFlexGridSizer5, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText( itemDialog1, wxID_STATIC, _("Microsoft Word (DOC):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString mDOCStrings[] = {
        _("catdoc"),
        _("antiword"),
        _("OpenOffice.org")
    };
    mDOC = new wxOwnerDrawnComboBox( itemDialog1, ID_CHOICE4, _("catdoc"), wxDefaultPosition, wxDefaultSize, 3, mDOCStrings, wxCB_READONLY );
    mDOC->SetStringSelection(_("catdoc"));
    itemFlexGridSizer5->Add(mDOC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText8 = new wxStaticText( itemDialog1, wxID_STATIC, _("Microsoft Excel (XLS):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString mXLSStrings[] = {
        _("xls2csv"),
        _("OpenOffice.org")
    };
    mXLS = new wxOwnerDrawnComboBox( itemDialog1, ID_CHOICE5, _("xls2csv"), wxDefaultPosition, wxDefaultSize, 2, mXLSStrings, wxCB_READONLY );
    mXLS->SetStringSelection(_("xls2csv"));
    itemFlexGridSizer5->Add(mXLS, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText10 = new wxStaticText( itemDialog1, wxID_STATIC, _("Microsoft PowerPoint (PPT):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString mPPTStrings[] = {
        _("catppt"),
        _("OpenOffice.org")
    };
    mPPT = new wxOwnerDrawnComboBox( itemDialog1, ID_CHOICE6, _("catppt"), wxDefaultPosition, wxDefaultSize, 2, mPPTStrings, wxCB_READONLY );
    mPPT->SetStringSelection(_("catppt"));
    itemFlexGridSizer5->Add(mPPT, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText12 = new wxStaticText( itemDialog1, wxID_STATIC, _("RTF documents:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString mRTFStrings[] = {
        _("rtf2html"),
        _("OpenOffice.org")
    };
    mRTF = new wxOwnerDrawnComboBox( itemDialog1, ID_CHOICE7, _("rtf2html"), wxDefaultPosition, wxDefaultSize, 2, mRTFStrings, wxCB_READONLY );
    mRTF->SetStringSelection(_("rtf2html"));
    itemFlexGridSizer5->Add(mRTF, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer14Static = new wxStaticBox(itemDialog1, wxID_ANY, _("OpenOffice.org usage"));
    wxStaticBoxSizer* itemStaticBoxSizer14 = new wxStaticBoxSizer(itemStaticBoxSizer14Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer14, 0, wxGROW|wxALL, 5);

    mOOOStandalone = new wxRadioButton( itemDialog1, ID_OOO_STANDALONE_RADIOBUTTON, _("Use standalone OpenOffice.org"), wxDefaultPosition, wxDefaultSize, 0 );
    mOOOStandalone->SetValue(FALSE);
    itemStaticBoxSizer14->Add(mOOOStandalone, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer14->Add(itemBoxSizer16, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemBoxSizer16->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer16->Add(itemBoxSizer18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText19 = new wxStaticText( itemDialog1, wxID_STATIC, _("Location of OOo executable:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mSoffice = new wxTextCtrl( itemDialog1, ID_OOO_BINARY_TEXTCTRL, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    itemBoxSizer18->Add(mSoffice, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mBrowseSoffice = new wxButton( itemDialog1, ID_OOO_BINARY_BROWSE_BUTTON, _("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(mBrowseSoffice, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mOOOServer = new wxRadioButton( itemDialog1, ID_OOO_SERVER_RADIOBUTTON, _("Use client-server OpenOffice.org"), wxDefaultPosition, wxDefaultSize, 0 );
    mOOOServer->SetValue(FALSE);
    itemStaticBoxSizer14->Add(mOOOServer, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 5);

    wxBoxSizer* itemBoxSizer23 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer14->Add(itemBoxSizer23, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemBoxSizer23->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer25 = new wxFlexGridSizer(2, 3, 0, 0);
    itemBoxSizer23->Add(itemFlexGridSizer25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText26 = new wxStaticText( itemDialog1, wxID_STATIC, _("Hostname:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer25->Add(itemStaticText26, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mHostname = new wxTextCtrl( itemDialog1, ID_OOO_HOSTNAME_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer25->Add(mHostname, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer25->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText29 = new wxStaticText( itemDialog1, wxID_STATIC, _("Port number:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer25->Add(itemStaticText29, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPort = new wxTextCtrl( itemDialog1, ID_OOO_PORT_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer25->Add(mPort, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer25->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText32 = new wxStaticText( itemDialog1, wxID_STATIC, _("OpenOffice.org 'program' directory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer25->Add(itemStaticText32, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mOOOProgramDir = new wxTextCtrl( itemDialog1, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxSize(150, -1), 0 );
    itemFlexGridSizer25->Add(mOOOProgramDir, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mBrowseOOOProgramDir = new wxButton( itemDialog1, ID_BUTTON, _("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer25->Add(mBrowseOOOProgramDir, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer2->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer36 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer36, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* itemButton37 = new wxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton37->SetDefault();
    itemBoxSizer36->Add(itemButton37, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton38 = new wxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer36->Add(itemButton38, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton39 = new wxButton( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer36->Add(itemButton39, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end FtxHelpersConfigDlg content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void FtxHelpersConfigDlg::OnOooBinaryBrowseButtonClick( wxCommandEvent& event )
{
  // browse for soffice executable
  wxString str=wxFileSelector(_("Select OpenOffice.org executable (soffice)."),mSoffice->GetValue(),
#ifdef WINS
    wxT("soffice.exe")
#else
    wxT("soffice")
#endif
    ,wxT(""),
#ifdef WINS
    _("Executable files (*.exe)|*.exe|All files (*.*)|*.*")
#else
    wxT("*.*")
#endif
    ,wxOPEN|wxFILE_MUST_EXIST,this);
  if( !str.empty() )
  {
    mSoffice->SetValue(str);
  }
  event.Skip();
}

/*!
 * Should we show tooltips?
 */

bool FtxHelpersConfigDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap FtxHelpersConfigDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin FtxHelpersConfigDlg bitmap retrieval
    return wxNullBitmap;
////@end FtxHelpersConfigDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon FtxHelpersConfigDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin FtxHelpersConfigDlg icon retrieval
    return wxNullIcon;
////@end FtxHelpersConfigDlg icon retrieval
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void FtxHelpersConfigDlg::OnOkClick( wxCommandEvent& event )
{
  if( Checkftx602macro() ) if( SaveParams() ) event.Skip();
}

#ifdef WINS
bool FtxHelpersConfigDlg::LoadwxTextCtrlValueFromRegDB(HKEY hKey,wxTextCtrl *tc,const char *regdb_valname)
{
  bool result=false;
  DWORD valuesize;
  if( RegQueryValueExA(hKey,regdb_valname,NULL,NULL,NULL,&valuesize)==ERROR_SUCCESS )
  {
    char *tmp=new char[valuesize+1];
    if( tmp!=NULL )
    {
      if( RegQueryValueExA(hKey,regdb_valname,NULL,NULL,(LPBYTE)tmp,&valuesize)==ERROR_SUCCESS )
      { // all succeeded
        tmp[valuesize]='\0';
        tc->SetValue(wxString(tmp,*wxConvCurrent));
        result=true;
      }
      delete[] tmp;
    }
  }
  return result;
}
#endif

bool FtxHelpersConfigDlg::LoadParams()
{
  // set default values
  mOOOStandalone->SetValue(true);
  mOOOServer->SetValue(false);
  mSoffice->Clear();
  mHostname->Clear();
  mPort->Clear();
  mDOC->SetStringSelection(_("catdoc"));
  mXLS->SetStringSelection(_("xls2csv"));
  mPPT->SetStringSelection(_("catppt"));
  mRTF->SetStringSelection(_("rtf2html"));
#ifdef WINS
  char subkey[300];
  strcpy(subkey,WB_inst_key);
  strcat(subkey,"\\"); strcat(subkey,FTXHLP_KEY);
  HKEY hKey;
  if( RegOpenKeyExA(HKEY_LOCAL_MACHINE,subkey,0,KEY_READ_EX,&hKey)==ERROR_SUCCESS )
  {
    DWORD value=0,valuesize=sizeof(DWORD);
    if( RegQueryValueExA(hKey,FTXHLP_VALNAME_USEOOO,NULL,NULL,(LPBYTE)&value,&valuesize)==ERROR_SUCCESS )
    {
      if( value!=0 )
      { // use OOo
        LoadwxTextCtrlValueFromRegDB(hKey,mSoffice,FTXHLP_VALNAME_OOOBINARY);
        LoadwxTextCtrlValueFromRegDB(hKey,mHostname,FTXHLP_VALNAME_OOOHOSTNAME);
        LoadwxTextCtrlValueFromRegDB(hKey,mPort,FTXHLP_VALNAME_OOOPORT);
        LoadwxTextCtrlValueFromRegDB(hKey,mOOOProgramDir,FTXHLP_VALNAME_OOOPROGRAMDIR);
      }
    }
    mOOOStandalone->SetValue(value!=2);
    mOOOServer->SetValue(value==2);
    char buffer[100];
    valuesize=sizeof(buffer);
    if( RegQueryValueExA(hKey,FTXHLP_VALNAME_DOC,NULL,NULL,(LPBYTE)buffer,&valuesize)==ERROR_SUCCESS )
    {
      mDOC->SetStringSelection(wxString(buffer,*wxConvCurrent));
    }
    valuesize=sizeof(buffer);
    if( RegQueryValueExA(hKey,FTXHLP_VALNAME_XLS,NULL,NULL,(LPBYTE)buffer,&valuesize)==ERROR_SUCCESS )
    {
      mXLS->SetStringSelection(wxString(buffer,*wxConvCurrent));
    }
    valuesize=sizeof(buffer);
    if( RegQueryValueExA(hKey,FTXHLP_VALNAME_PPT,NULL,NULL,(LPBYTE)buffer,&valuesize)==ERROR_SUCCESS )
    {
      mPPT->SetStringSelection(wxString(buffer,*wxConvCurrent));
    }
    valuesize=sizeof(buffer);
    if( RegQueryValueExA(hKey,FTXHLP_VALNAME_RTF,NULL,NULL,(LPBYTE)buffer,&valuesize)==ERROR_SUCCESS )
    {
      mRTF->SetStringSelection(wxString(buffer,*wxConvCurrent));
    }
    RegCloseKey(hKey);
  }
#else
  // Linux: read from /etc/602sql
  const char cfgfile[]="/etc/602sql";
  const char invalidvalue[]=":";
  char buffer[300];
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_USEOOO,invalidvalue,buffer,sizeof(buffer),cfgfile);
  mOOOStandalone->SetValue(strcmp(buffer,"2")!=0);
  mOOOServer->SetValue(strcmp(buffer,"2")==0);
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_OOOBINARY,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    mSoffice->SetValue(wxString(buffer,*wxConvCurrent));
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_OOOHOSTNAME,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    mHostname->SetValue(wxString(buffer,*wxConvCurrent));
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_OOOPORT,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    mPort->SetValue(wxString(buffer,*wxConvCurrent));
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_OOOPROGRAMDIR,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    mOOOProgramDir->SetValue(wxString(buffer,*wxConvCurrent));
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_DOC,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    mDOC->SetStringSelection(wxString(buffer,*wxConvCurrent));
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_XLS,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    mXLS->SetStringSelection(wxString(buffer,*wxConvCurrent));
  } 
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_PPT,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    mPPT->SetStringSelection(wxString(buffer,*wxConvCurrent));
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_RTF,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    mRTF->SetStringSelection(wxString(buffer,*wxConvCurrent));
  }
#endif
  // enable/disable controls
  bool value=mOOOStandalone->GetValue();
  mSoffice->Enable(value);
  mBrowseSoffice->Enable(value);
  value=!value;
  mHostname->Enable(value);
  mPort->Enable(value);
  mOOOProgramDir->Enable(value);
  mBrowseOOOProgramDir->Enable(value);
  return true;
}

bool FtxHelpersConfigDlg::SaveParams()
{
  // write parameters to config file
#ifdef WINS
  /* save to registry on Windows */
  char subkey[300];
  strcpy(subkey,WB_inst_key);
  strcat(subkey,"\\"); strcat(subkey,FTXHLP_KEY);
  HKEY hKey;
  DWORD dwDisposition;
  int counter=0;
  if( RegCreateKeyExA(HKEY_LOCAL_MACHINE,subkey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE_EX,NULL,&hKey,&dwDisposition)==ERROR_SUCCESS )
  {
    DWORD valuesize;
    counter++;
    valuesize=sizeof(DWORD);
    DWORD use_ooo=(mOOOStandalone->GetValue())?1:(mOOOServer->GetValue())?2:0;
    if( RegSetValueExA(hKey,FTXHLP_VALNAME_USEOOO,0,REG_DWORD,(LPBYTE)&use_ooo,valuesize)==ERROR_SUCCESS )
    {
      counter++;
      switch( use_ooo )
      {
        case 0: counter+=3; break;
        case 1:
          { // use standalone OOo
            char *tmp=new char[(valuesize=(DWORD)mSoffice->GetValue().Length())+1];
            if( tmp==NULL ) { error_box(_("Out of memory."),this); return false; }
            strcpy(tmp,mSoffice->GetValue().mb_str(*wxConvCurrent));
            if( RegSetValueExA(hKey,FTXHLP_VALNAME_OOOBINARY,0,REG_SZ,(LPBYTE)tmp,valuesize)==ERROR_SUCCESS )
            {
              counter+=3;
            }
            delete[] tmp;
          }
          break;
        case 2:
          { // use client-server OOo
            char *tmp=new char[(valuesize=(DWORD)mHostname->GetValue().Length())+1];
            if( tmp==NULL ) { error_box(_("Out of memory."),this); return false; }
            strcpy(tmp,mHostname->GetValue().mb_str(*wxConvCurrent));
            if( RegSetValueExA(hKey,FTXHLP_VALNAME_OOOHOSTNAME,0,REG_SZ,(LPBYTE)tmp,valuesize)==ERROR_SUCCESS )
            {
              counter++;
            }
            delete[] tmp;
            tmp=new char[(valuesize=(DWORD)mPort->GetValue().Length())+1];
            if( tmp==NULL ) { error_box(_("Out of memory."),this); return false; }
            strcpy(tmp,mPort->GetValue().mb_str(*wxConvCurrent));
            if( RegSetValueExA(hKey,FTXHLP_VALNAME_OOOPORT,0,REG_SZ,(LPBYTE)tmp,valuesize)==ERROR_SUCCESS )
            {
              counter++;
            }
            delete[] tmp;
            tmp=new char[(valuesize=(DWORD)mOOOProgramDir->GetValue().Length())+1];
            if( tmp==NULL ) { error_box(_("Out of memory."),this); return false; }
            strcpy(tmp,mOOOProgramDir->GetValue().mb_str(*wxConvCurrent));
            if( RegSetValueExA(hKey,FTXHLP_VALNAME_OOOPROGRAMDIR,0,REG_SZ,(LPBYTE)tmp,valuesize)==ERROR_SUCCESS )
            {
              counter++;
            }
            delete[] tmp;
          }
          break;
      }
    }
    char buffer[100];
    strcpy(buffer,mDOC->GetStringSelection().mb_str(*wxConvCurrent));
    valuesize=(DWORD)strlen(buffer);
    if( RegSetValueExA(hKey,FTXHLP_VALNAME_DOC,0,REG_SZ,(LPBYTE)buffer,valuesize)==ERROR_SUCCESS )
    {
      counter++;
    }
    strcpy(buffer,mXLS->GetStringSelection().mb_str(*wxConvCurrent));
    valuesize=(DWORD)strlen(buffer);
    if( RegSetValueExA(hKey,FTXHLP_VALNAME_XLS,0,REG_SZ,(LPBYTE)buffer,valuesize)==ERROR_SUCCESS )
    {
      counter++;
    }
    strcpy(buffer,mPPT->GetStringSelection().mb_str(*wxConvCurrent));
    valuesize=(DWORD)strlen(buffer);
    if( RegSetValueExA(hKey,FTXHLP_VALNAME_PPT,0,REG_SZ,(LPBYTE)buffer,valuesize)==ERROR_SUCCESS )
    {
      counter++;
    }
    strcpy(buffer,mRTF->GetStringSelection().mb_str(*wxConvCurrent));
    valuesize=(DWORD)strlen(buffer);
    if( RegSetValueExA(hKey,FTXHLP_VALNAME_RTF,0,REG_SZ,(LPBYTE)buffer,valuesize)==ERROR_SUCCESS )
    {
      counter++;
    }
    RegCloseKey(hKey);
  }
  if( counter<9 )
  {
    error_box(_("Cannot write configuration to registration database."),this);
    return false;
  }
#else
  /* save to /etc/602sql on Linux */
  const char cfgfile[]="/etc/602sql";
  const char invalidvalue[]=":";
  char buffer[300];
  bool failed=true;
  if( mOOOStandalone->GetValue() ) strcpy(buffer,"1"); else if( mOOOServer->GetValue() ) strcpy(buffer,"2"); else strcpy(buffer,"0");
  if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_USEOOO,buffer,cfgfile) )
  {
    strcpy(buffer,mSoffice->GetValue().mb_str(*wxConvCurrent));
    if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_OOOBINARY,buffer,cfgfile) )
    {
      strcpy(buffer,mHostname->GetValue().mb_str(*wxConvCurrent));
      if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_OOOHOSTNAME,buffer,cfgfile) )
      {
        strcpy(buffer,mPort->GetValue().mb_str(*wxConvCurrent));
        if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_OOOPORT,buffer,cfgfile) )
        {
          strcpy(buffer,mOOOProgramDir->GetValue().mb_str(*wxConvCurrent));
          if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_OOOPROGRAMDIR,buffer,cfgfile) )
          {
            strcpy(buffer,mDOC->GetStringSelection().mb_str(*wxConvCurrent));
            if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_DOC,buffer,cfgfile) )
            {
              strcpy(buffer,mXLS->GetStringSelection().mb_str(*wxConvCurrent));
              if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_XLS,buffer,cfgfile) )
              {
                strcpy(buffer,mPPT->GetStringSelection().mb_str(*wxConvCurrent));
                if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_PPT,buffer,cfgfile) )
                {
                  strcpy(buffer,mRTF->GetStringSelection().mb_str(*wxConvCurrent));
                  if( WritePrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_RTF,buffer,cfgfile) )
                  {
                    failed=false;
                  }
                }
              }
            } 
          }
        }
      }
    }
  }
  if( failed )
  {
    error_box(_("Cannot write configuration to /etc/602sql file."),this);
    return false;
  }
#endif
  return true;
}

bool FtxHelpersConfigDlg::IsOOoSelected()
{
  wxString ooo_item(_("OpenOffice.org"));
  return mDOC->GetStringSelection()==ooo_item || mXLS->GetStringSelection()==ooo_item || mPPT->GetStringSelection()==ooo_item || mRTF->GetStringSelection()==ooo_item;
}

bool FtxHelpersConfigDlg::Checkftx602macro()
{
  if( mOOOStandalone->GetValue() && IsOOoSelected() )
  {
    wxFileName fn(mSoffice->GetValue()); // ooo_home/program/soffice
    if( !fn.FileExists() )
    {
      wxString msg;
      msg.Printf(_("OpenOffice.org executable \"%s\" doesn't exist."), fn.GetFullPath().c_str());
      error_box(msg,this);
      return false;
    }
    fn.SetFullName(wxString("ftx602.xba",*wxConvCurrent)); // change filename.ext
    fn.RemoveLastDir(); // remove /program
    fn.AppendDir(wxString("share",*wxConvCurrent));
    fn.AppendDir(wxString("basic",*wxConvCurrent));
    fn.AppendDir(wxString("Tools",*wxConvCurrent)); // append share/basic/Tools
    if( !fn.IsOk() )
    {
      return false;
    }
    if( !fn.FileExists(fn.GetFullPath()) )
    {
      error_box(_("OpenOffice.org macro file ftx602.xba is missing\nin \"")+fn.GetPath()+_("\" directory.\n\nInstall this macro file first (see help for instructions)."),this);
      return false;
    }
    fn.SetFullName(wxString("script.xlb",*wxConvCurrent)); // change filename.ext
    if( !fn.FileExists(fn.GetFullPath()) )
    {
      error_box(_("OpenOffice.org macro library file script.xlb is missing\nin \"")+fn.GetPath()+_("\" directory.\n\nInstall macro file ftx602.xba first (see help for instructions)."),this);
      return false;
    }
    return Checkscriptxlbfile(fn.GetFullPath());
  }
  return true;
}

bool FtxHelpersConfigDlg::Checkscriptxlbfile(const wxString &filename)
{
  bool fileop_error=false;
  bool tag_found=false;
  FILE *f=fopen(filename.mb_str(*wxConvCurrent),"rb");
  if( f==NULL ) fileop_error=true;
  else
  {
    const size_t buflen=1000;
    char buf[buflen+1],*bufptr=buf;
    while( !tag_found && !feof(f) )
    {
      size_t readed=fread(bufptr,sizeof(char),buflen-(bufptr-buf),f);
      if( ferror(f) ) { fileop_error=true; break; }
      if( readed>0 )
      {
        size_t lastcharidx=readed+(bufptr-buf);
        buf[lastcharidx]='\0';
        if( strstr(buf,"\"ftx602\"")!=NULL ) { fileop_error=false; tag_found=true; break; } // found
        // move last 7 chars to the start of buf
        // in case that end of buf contains start of \"ftx602\" string
        if( lastcharidx>7 )
        {
          memcpy(buf,buf+lastcharidx-7,sizeof(char)*7);
          bufptr=buf+7; // we will read next chars there
        }
        else bufptr=buf;
      }
    }
    fclose(f);
  }
  if( fileop_error )
  {
    error_box(_("Cannot read file \"")+filename+_("\".\n Please review this file and make sure that it contains the XML tag\n <library:element library:name=\"ftx602\"/>\nand consult the help for instructions on how to install the ftx602.xba OpenOffice.org macro."),this);
    return false;
  }
  if( !tag_found )
  {
    error_box(_("File \"")+filename+_("\"\n does not contain the XML tag <library:element library:name=\"ftx602\"/>\n\nCorrect this problem first, then view the help file for instructions on how to install the ftx602.xba OpenOffice.org macro."),this);
    return false;
  }
  return true;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void FtxHelpersConfigDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/fulltext_conf.html"));
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void FtxHelpersConfigDlg::OnBrowseOOOProgramDirButtonClick( wxCommandEvent& event )
{
  // browse for program subdirectory of OOo installation
  wxString str=wxDirSelector(_("Select the program directory of the OpenOffice.org installation."),mOOOProgramDir->GetValue(),0,wxDefaultPosition,this);
  if( !str.empty() )
  {
    mOOOProgramDir->SetValue(str);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_OOO_STANDALONE_RADIOBUTTON
 */

void FtxHelpersConfigDlg::OnOooStandaloneRadiobuttonSelected( wxCommandEvent& event )
{
  bool value=mOOOStandalone->GetValue();
  mSoffice->Enable(value);
  mBrowseSoffice->Enable(value);
  value=!value;
  mHostname->Enable(value);
  mPort->Enable(value);
  mOOOProgramDir->Enable(value);
  mBrowseOOOProgramDir->Enable(value);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_OOO_SERVER_RADIOBUTTON
 */

void FtxHelpersConfigDlg::OnOooServerRadiobuttonSelected( wxCommandEvent& event )
{
  bool value=mOOOServer->GetValue();
  mHostname->Enable(value);
  mPort->Enable(value);
  mOOOProgramDir->Enable(value);
  mBrowseOOOProgramDir->Enable(value);
  value=!value;
  mSoffice->Enable(value);
  mBrowseSoffice->Enable(value);
  event.Skip();
}


