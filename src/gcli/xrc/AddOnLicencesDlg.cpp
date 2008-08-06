/////////////////////////////////////////////////////////////////////////////
// Name:        AddOnLicencesDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/27/07 16:48:58
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "AddOnLicencesDlg.h"
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

#include "AddOnLicencesDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * AddOnLicencesDlg type definition
 */

IMPLEMENT_CLASS( AddOnLicencesDlg, wxDialog )

/*!
 * AddOnLicencesDlg event table definition
 */

BEGIN_EVENT_TABLE( AddOnLicencesDlg, wxDialog )

////@begin AddOnLicencesDlg event table entries
    EVT_BUTTON( CD_LIC_BUY, AddOnLicencesDlg::OnCdLicBuyClick )

    EVT_BUTTON( CD_LIC_SAVE, AddOnLicencesDlg::OnCdLicSaveClick )

    EVT_BUTTON( CD_LIC_PREP, AddOnLicencesDlg::OnCdLicPrepClick )

    EVT_BUTTON( CD_LIC_PROC, AddOnLicencesDlg::OnCdLicProcClick )

    EVT_BUTTON( wxID_HELP, AddOnLicencesDlg::OnHelpClick )

    EVT_BUTTON( wxID_CLOSE, AddOnLicencesDlg::OnCloseClick )

////@end AddOnLicencesDlg event table entries

END_EVENT_TABLE()

/*!
 * AddOnLicencesDlg constructors
 */

AddOnLicencesDlg::AddOnLicencesDlg(cdp_t cdpIn)
{ cdp=cdpIn;
}

AddOnLicencesDlg::AddOnLicencesDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * AddOnLicencesDlg creator
 */

bool AddOnLicencesDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin AddOnLicencesDlg member initialisation
    mExtCapt = NULL;
    mList = NULL;
    mBuy = NULL;
    mLic = NULL;
    mSave = NULL;
    mPrep = NULL;
    mProc = NULL;
////@end AddOnLicencesDlg member initialisation

////@begin AddOnLicencesDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end AddOnLicencesDlg creation
    return TRUE;
}

/*!
 * Control creation for AddOnLicencesDlg
 */

void AddOnLicencesDlg::CreateControls()
{    
////@begin AddOnLicencesDlg content construction
    AddOnLicencesDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mExtCapt = new wxStaticText;
    mExtCapt->Create( itemDialog1, wxID_STATIC, _("Activated extensions:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(mExtCapt, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mListStrings = NULL;
    mList = new wxListBox;
    mList->Create( itemDialog1, CD_LIC_LIST, wxDefaultPosition, wxDefaultSize, 0, mListStrings, wxLB_SINGLE );
    itemBoxSizer2->Add(mList, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Activating a new extension:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer6, 0, wxALIGN_LEFT|wxALL, 0);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("1. Buy the extension in the e-shop"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mBuy = new wxButton;
    mBuy->Create( itemDialog1, CD_LIC_BUY, _("Buy"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mBuy, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemDialog1, wxID_STATIC, _("2. Enter the activation key here:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText9, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxGROW|wxALL, 5);

    itemBoxSizer10->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mLic = new wxTextCtrl;
    mLic->Create( itemDialog1, CD_LIC_LIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(mLic, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxStaticText* itemStaticText13 = new wxStaticText;
    itemStaticText13->Create( itemDialog1, wxID_STATIC, _("3. Activate the extension either on-line or off-line:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText13, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer14, 0, wxGROW|wxALL, 0);

    itemBoxSizer14->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer16Static = new wxStaticBox(itemDialog1, wxID_ANY, _("On-line activation:"));
    wxStaticBoxSizer* itemStaticBoxSizer16 = new wxStaticBoxSizer(itemStaticBoxSizer16Static, wxVERTICAL);
    itemBoxSizer14->Add(itemStaticBoxSizer16, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText17 = new wxStaticText;
    itemStaticText17->Create( itemDialog1, wxID_STATIC, _("Press the key:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer16->Add(itemStaticText17, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mSave = new wxButton;
    mSave->Create( itemDialog1, CD_LIC_SAVE, _("Activate the extension"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer16->Add(mSave, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText19 = new wxStaticText;
    itemStaticText19->Create( itemDialog1, wxID_STATIC, _("(Internet connection required)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer16->Add(itemStaticText19, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxStaticBox* itemStaticBoxSizer20Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Off-line activation:"));
    wxStaticBoxSizer* itemStaticBoxSizer20 = new wxStaticBoxSizer(itemStaticBoxSizer20Static, wxVERTICAL);
    itemBoxSizer14->Add(itemStaticBoxSizer20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer21 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer21->AddGrowableRow(2);
    itemFlexGridSizer21->AddGrowableCol(3);
    itemStaticBoxSizer20->Add(itemFlexGridSizer21, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* itemStaticText22 = new wxStaticText;
    itemStaticText22->Create( itemDialog1, wxID_STATIC, _("3.1."), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer21->Add(itemStaticText22, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPrep = new wxButton;
    mPrep->Create( itemDialog1, CD_LIC_PREP, _("Save the activation request to a file"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer21->Add(mPrep, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxStaticText* itemStaticText24 = new wxStaticText;
    itemStaticText24->Create( itemDialog1, wxID_STATIC, _("3.2."), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer21->Add(itemStaticText24, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText25 = new wxStaticText;
    itemStaticText25->Create( itemDialog1, wxID_STATIC, _("Process the request on http://602xmlfs.602.cz/response"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer21->Add(itemStaticText25, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText26 = new wxStaticText;
    itemStaticText26->Create( itemDialog1, wxID_STATIC, _("3.3."), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer21->Add(itemStaticText26, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mProc = new wxButton;
    mProc->Create( itemDialog1, CD_LIC_PROC, _("Process the activation response file"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer21->Add(mProc, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer28 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer28, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton29 = new wxButton;
    itemButton29->Create( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(itemButton29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton30 = new wxButton;
    itemButton30->Create( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(itemButton30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end AddOnLicencesDlg content construction
    mLic->SetMaxLength(5*5+4);  // 5 groups of 5 characters, 4 separators 
    mList->SetInitialSize(wxSize(140,80));  // this prevents creating a very wide manager window when a local SQL server with a long text in its log is running
    show_lics();
 //   mPrep->Disable();  mProc->Disable();  -- no more
}

/*!
 * Should we show tooltips?
 */

bool AddOnLicencesDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap AddOnLicencesDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin AddOnLicencesDlg bitmap retrieval
    return wxNullBitmap;
////@end AddOnLicencesDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon AddOnLicencesDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin AddOnLicencesDlg icon retrieval
    return wxNullIcon;
////@end AddOnLicencesDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_BUY
 */

void AddOnLicencesDlg::show_lics(void)
// Must read from the server:
{
  mList->Clear();
 // list the licences:
  uns32 state;
  if (!cd_Get_server_info(cdp, OP_GI_LICS_XML, &state, sizeof(state)) && state)
    mList->AppendString(_("XML Extension"));
  if (!cd_Get_server_info(cdp, OP_GI_LICS_FULLTEXT, &state, sizeof(state)) && state)
    mList->AppendString(_("Fulltext Extension"));
 // information about TRIAL licences:   
  if (!cd_Get_server_info(cdp, OP_GI_TRIAL_ADD_ON, &state, sizeof(state)))
  { if (!state)
      mExtCapt->SetLabel(_("Activated extensions:"));
    else  
      if (!cd_Get_server_info(cdp, OP_GI_TRIAL_REM_DAYS, &state, sizeof(state)))
      { wxString msg;
        msg.Printf(_("The trial extensions licence is valid for another %u days"), state);
        mExtCapt->SetLabel(msg);
      }
      Fit();  // resize the dialog
  }    
}

void AddOnLicencesDlg::OnCdLicBuyClick( wxCommandEvent& event )
// No more "http://www.602.cz/cz/nakup"
{
  stViewHTMLFile(_("http://reg.602.cz/phelp.php?software-id=602SQL&vendor=&lang=en-US&command=getlicence&feature=eshop"));
}

#include <wcs.h>
#include <wcs.cpp>

bool AddOnLicencesDlg::activation(char * lic, char * company, char * computer)
{ void * licence_handle;  bool success=false;
  const char * mylocale;  int lang;  char * request, * response;
  if (!read_profile_int("Locale", "Language", &lang)) lang=wxLANGUAGE_DEFAULT;
  mylocale = lang==wxLANGUAGE_CZECH || lang==wxLANGUAGE_SLOVAK ? "cs-CZ" : "en-US";
  char licdir[MAX_PATH];  
  get_path_prefix(licdir);
#ifdef LINUX
  strcat(licdir, "/lib/" WB_LINUX_SUBDIR_NAME); 
#endif
  int res=licence_init(&licence_handle, SOFT_ID, licdir, mylocale, 1, NULL, NULL);
  if (licence_handle)
  { if (cd_Store_lic_num(cdp, lic, company, computer, &request))
      cd_Signalize2(cdp, this);
    else
    { res=sendActivationRequest(licence_handle, request, &response);
      corefree(request);
      if (res!=LICENCE_OK)
      { 
#ifdef WINS
        wxString msg(WCGetLastErrorW(res, licence_handle, mylocale));
#else  // on Linux WCGetLastErrorW returs bad byte order
        wxString msg(WCGetLastErrorA(res, licence_handle, mylocale), wxConvUTF8);
#endif
        licence_close(licence_handle);
        error_box(msg, this);
      }
      else  
      {// make copy of the response before closing the system:
        char * response_copy = (char*)corealloc(strlen(response)+1, 55);
        strcpy(response_copy, response);
        licence_close(licence_handle);  // the licence system must be closed before server stores the licence!
        if (cd_Store_activation(cdp, response_copy))
          cd_Signalize2(cdp, this);
        else 
        { success=true;
         // local updating of wc library: 
          int res=licence_init(&licence_handle, SOFT_ID, licdir, mylocale, 1, NULL, NULL);
          if (licence_handle)
          { saveProp(licence_handle, WCP_BETA, "");  // remove the activation made by product with embedded 602SQL
            acceptActivationResponse(&licence_handle, response_copy);  // can change the licence_handle!
            licence_close(licence_handle);
          }
        }
        corefree(response_copy);
      }
    }
  }
  else
  { 
#ifdef WINS
    wxString msg(WCGetLastErrorW(res, licence_handle, mylocale));
#else  // on Linux WCGetLastErrorW returs bad byte order
    wxString msg(WCGetLastErrorA(res, licence_handle, mylocale), wxConvUTF8);
#endif
    if (msg.IsEmpty()) msg=_("Licence library not loaded.");
    error_box(msg, this);
  }
  return success;
}

bool AddOnLicencesDlg::proc_lic_num(char lic8[25+1])
{ bool bad_char = false;
  wxString lic = mLic->GetValue(), dense;
  for (int i=0;  i<lic.Length();  i++)
  { wxChar c = lic[i];
    if (c==' ' || c=='-') continue;  // ignoring separators
    else if (c>='0' && c<='9' || c>='a' && c<='z' || c>='A' && c<='Z')
      dense=dense+c;
    else bad_char=true;
  }
  if (bad_char)
    { error_box(_("The licence number contains an invalid character."), this);  return false; }
  else if (dense.Length() != 25)  
    { error_box(_("The licence number has wrong length."), this);  return false; }
  strcpy(lic8, dense.mb_str(*wxConvCurrent));  // convert the lic.num.
  return true;
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_SAVE
 */

void AddOnLicencesDlg::OnCdLicSaveClick( wxCommandEvent& event )
{ char lic8[25+1];
  if (proc_lic_num(lic8))
    if (activation(lic8, "N/A", ""))  // the company name must not be empty!!
    { show_lics();
      mLic->SetValue(wxEmptyString);
      info_box(_("Licence activated."), this);
    }  
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_PROC
 */

void AddOnLicencesDlg::OnCdLicProcClick( wxCommandEvent& event )
{ wxFileDialog fd(this, _("Enter the file name of the activation response"), wxEmptyString,
                      wxT("actresp.txt"), wxT("*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (fd.ShowModal()==wxID_OK)
  { wxString fname = fd.GetPath();
    wxFile freq(fname, wxFile::read);
    if (!freq.IsOpened())
      error_box(_("Cannot open the file"), this);
    else
    { int len = freq.Length();
      char * resp = (char*)corealloc(len+1, 89);
      freq.Read(resp, len);
      resp[len]=0;
      freq.Close();
      if (cd_Store_activation(cdp, resp))
        cd_Signalize2(cdp, this);
      else 
      {// local updating of wc library: 
        void * licence_handle;  const char * mylocale;  int lang;
        if (!read_profile_int("Locale", "Language", &lang)) lang=wxLANGUAGE_DEFAULT;
        mylocale = lang==wxLANGUAGE_CZECH || lang==wxLANGUAGE_SLOVAK ? "cs-CZ" : "en-US";
        char licdir[MAX_PATH];  
        get_path_prefix(licdir);
#ifdef LINUX
        strcat(licdir, "/lib/" WB_LINUX_SUBDIR_NAME); 
#endif
        int res=licence_init(&licence_handle, SOFT_ID, licdir, mylocale, 1, NULL, NULL);
        if (licence_handle)
        { saveProp(licence_handle, WCP_BETA, "");  // remove the activation made by product with embedded 602SQL
          acceptActivationResponse(&licence_handle, resp);  // can change the licence_handle!
          licence_close(licence_handle);
        }
       // report success 
        show_lics();
        mLic->SetValue(wxEmptyString);
        info_box(_("Licence activated."), this);
      }
      corefree(resp);
    }  
  }  
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_PREP
 */

void AddOnLicencesDlg::OnCdLicPrepClick( wxCommandEvent& event )
{ char lic8[25+1];  char * request;
  if (proc_lic_num(lic8))  // a valid number entered
    if (cd_Store_lic_num(cdp, lic8, "N/A", "", &request))
      cd_Signalize2(cdp, this);
    else
    { wxFileDialog fd(this, _("Enter the file name for the activation request"), wxEmptyString,
                      wxT("actreq.txt"), wxT("*.*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      if (fd.ShowModal()==wxID_OK)
      { wxString fname = fd.GetPath();
        wxFile freq(fname, wxFile::write);
        if (!freq.IsOpened())
          error_box(_("Cannot create the file"), this);
        else
        { freq.Write(request, strlen(request));
          freq.Close();
          wxString msg;
          msg.Printf(_("Activation request stored in %s."), fname.c_str());
          info_box(msg, this);
        }
      }
      corefree(request);
    }  
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void AddOnLicencesDlg::OnCloseClick( wxCommandEvent& event )
{
  Destroy();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void AddOnLicencesDlg::OnHelpClick( wxCommandEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/extensions.html")); }



