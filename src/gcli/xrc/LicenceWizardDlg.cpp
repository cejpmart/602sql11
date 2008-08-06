/////////////////////////////////////////////////////////////////////////////
// Name:        LicenceWizardDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     10/04/04 15:29:27
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "LicenceWizardDlg.h"
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

#include "LicenceWizardDlg.h"
#if WBVERS<1100
#include "licence8.cpp"
#endif
////@begin XPM images
////@end XPM images

/*!
 * LicenceWizardDlg type definition
 */

IMPLEMENT_CLASS( LicenceWizardDlg, wxWizard )

/*!
 * LicenceWizardDlg event table definition
 */

BEGIN_EVENT_TABLE( LicenceWizardDlg, wxWizard )

////@begin LicenceWizardDlg event table entries
    EVT_WIZARD_HELP( ID_WIZARD, LicenceWizardDlg::OnWizardHelp )

////@end LicenceWizardDlg event table entries

END_EVENT_TABLE()

/*!
 * LicenceWizardDlg constructors
 */

LicenceWizardDlg::LicenceWizardDlg(cdp_t cdpIn)
{ cdp=cdpIn;
  if (cd_Get_server_info(cdp, OP_GI_INST_KEY, ik, sizeof(ik)))
    *ik=0;
}

/*!
 * LicenceWizardDlg creator
 */

bool LicenceWizardDlg::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos )
{
////@begin LicenceWizardDlg member initialisation
    mPage1 = NULL;
    mPage2 = NULL;
////@end LicenceWizardDlg member initialisation

////@begin LicenceWizardDlg creation
    SetExtraStyle(GetExtraStyle()|wxWIZARD_EX_HELPBUTTON);
    wxBitmap wizardBitmap(GetBitmapResource(wxT("trigwiz")));
    wxWizard::Create( parent, id, _("Licence Wizard"), wizardBitmap, pos );

    CreateControls();
////@end LicenceWizardDlg creation
    return TRUE;
}

/*!
 * Control creation for LicenceWizardDlg
 */

void LicenceWizardDlg::CreateControls()
{    
////@begin LicenceWizardDlg content construction
    wxWizard* itemWizard1 = this;

    mPage1 = new LicWizardPage1;
    mPage1->Create( itemWizard1 );

    itemWizard1->FitToPage(mPage1);
    mPage2 = new LicWizardPage2;
    mPage2->Create( itemWizard1 );

    itemWizard1->FitToPage(mPage2);
    wxWizardPageSimple* lastPage = NULL;
    if (lastPage)
        wxWizardPageSimple::Chain(lastPage, mPage1);
    lastPage = mPage1;
    if (lastPage)
        wxWizardPageSimple::Chain(lastPage, mPage2);
    lastPage = mPage2;
////@end LicenceWizardDlg content construction
    // must use wxWizardPageSimple* lastPage = NULL; above
}

/*!
 * Runs the wizard.
 */

bool LicenceWizardDlg::Run()
{
    if (GetChildren().GetCount() > 0)
    {
        wxWizardPage* startPage = wxDynamicCast(GetChildren().GetFirst()->GetData(), wxWizardPage);
        if (startPage) return RunWizard(startPage);
    }
    return FALSE;
}

/*!
 * Should we show tooltips?
 */

bool LicenceWizardDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * LicWizardPage1 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( LicWizardPage1, wxWizardPageSimple )

/*!
 * LicWizardPage1 event table definition
 */

BEGIN_EVENT_TABLE( LicWizardPage1, wxWizardPageSimple )

////@begin LicWizardPage1 event table entries
    EVT_BUTTON( CD_LICWIZ_BUY_DN, LicWizardPage1::OnCdLicwizBuyDnClick )

    EVT_BUTTON( CD_LICWIZ_DEVEL, LicWizardPage1::OnCdLicwizDevelClick )

////@end LicWizardPage1 event table entries

END_EVENT_TABLE()

/*!
 * LicWizardPage1 constructors
 */

LicWizardPage1::LicWizardPage1( )
{
}

LicWizardPage1::LicWizardPage1( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage creator
 */

bool LicWizardPage1::Create( wxWizard* parent )
{
////@begin LicWizardPage1 member initialisation
    mBuy = NULL;
    mDevel = NULL;
////@end LicWizardPage1 member initialisation

////@begin LicWizardPage1 creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end LicWizardPage1 creation
    return TRUE;
}

/*!
 * Control creation for WizardPage
 */

void LicWizardPage1::CreateControls()
{    
////@begin LicWizardPage1 content construction
    LicWizardPage1* itemWizardPageSimple2 = this;

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemWizardPageSimple2->SetSizer(itemBoxSizer3);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemWizardPageSimple2, wxID_STATIC, _("The first step is to obtain a DISTRIBUTION number\nfor the 602SQL server."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mBuy = new wxButton;
    mBuy->Create( itemWizardPageSimple2, CD_LICWIZ_BUY_DN, _("Purchase a Registration Key from the online store."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mBuy, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemWizardPageSimple2, wxID_STATIC, _("If you are a developer, you may obtain a special \nRegistration Key for developing database applications."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    mDevel = new wxButton;
    mDevel->Create( itemWizardPageSimple2, CD_LICWIZ_DEVEL, _("Ask for a developer Registration Key"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mDevel, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemWizardPageSimple2, wxID_STATIC, _("Click Next if you have a Registration Key."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

////@end LicWizardPage1 content construction
}

/*!
 * Should we show tooltips?
 */

bool LicWizardPage1::ShowToolTips()
{
    return TRUE;
}

/*!
 * LicWizardPage2 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( LicWizardPage2, wxWizardPageSimple )

/*!
 * LicWizardPage2 event table definition
 */

BEGIN_EVENT_TABLE( LicWizardPage2, wxWizardPageSimple )

////@begin LicWizardPage11 event table entries
    EVT_WIZARD_PAGE_CHANGING( -1, LicWizardPage2::OnWizardpage1PageChanging )

    EVT_BUTTON( CD_LICWIZ_REGISTER, LicWizardPage2::OnCdLicwizRegisterClick )

    EVT_TEXT( CD_LICWIZ_LN, LicWizardPage2::OnCdLicwizLnUpdated )

////@end LicWizardPage11 event table entries

END_EVENT_TABLE()

/*!
 * LicWizardPage2 constructors
 */

LicWizardPage2::LicWizardPage2( )
{
}

LicWizardPage2::LicWizardPage2( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage1 creator
 */

bool LicWizardPage2::Create( wxWizard* parent )
{
////@begin LicWizardPage11 member initialisation
    mRegister = NULL;
    mLicNum = NULL;
////@end LicWizardPage11 member initialisation

////@begin LicWizardPage11 creation
    wxBitmap wizardBitmap;
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end LicWizardPage11 creation
    return TRUE;
}

/*!
 * Control creation for WizardPage1
 */

void LicWizardPage2::CreateControls()
{    
////@begin LicWizardPage11 content construction

    LicWizardPage2* item9 = this;

    wxBoxSizer* item10 = new wxBoxSizer(wxVERTICAL);
    item9->SetSizer(item10);
    item9->SetAutoLayout(TRUE);

    wxStaticText* item11 = new wxStaticText;
    item11->Create( item9, wxID_STATIC, _("The second step is to register your product."), wxDefaultPosition, wxDefaultSize, 0 );
    item10->Add(item11, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxButton* item12 = new wxButton;
    item12->Create( item9, CD_LICWIZ_REGISTER, _("Register"), wxDefaultPosition, wxDefaultSize, 0 );
    mRegister = item12;
    item10->Add(item12, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* item13 = new wxStaticText;
    item13->Create( item9, wxID_STATIC, _("The License ID will be sent by e-mail to you immediately\nafter registration."), wxDefaultPosition, wxDefaultSize, 0 );
    item10->Add(item13, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* item14 = new wxStaticText;
    item14->Create( item9, wxID_STATIC, _("Enter your Licenes ID here and click Finish:"), wxDefaultPosition, wxDefaultSize, 0 );
    item10->Add(item14, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item15 = new wxTextCtrl;
    item15->Create( item9, CD_LICWIZ_LN, _T(""), wxDefaultPosition, wxSize(250, -1), 0 );
    mLicNum = item15;
    item10->Add(item15, 0, wxALIGN_LEFT|wxALL, 5);

////@end LicWizardPage11 content construction
}

/*!
 * Should we show tooltips?
 */

bool LicWizardPage2::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LICWIZ_BUY_DN
 */

void LicWizardPage1::OnCdLicwizBuyDnClick( wxCommandEvent& event )
{
#if WBVERS<950
  stViewHTMLFile(_("http://www.602.cz/produkty/kosik/kos_602sql.htm"));
#else
  stViewHTMLFile(_("http://www.602.cz/cz/nakup"));
#endif
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LICWIZ_DEVEL
 */

void LicWizardPage1::OnCdLicwizDevelClick( wxCommandEvent& event )
{ 
#if WBVERS<950
  stViewHTMLFile(_("http://www.602.cz/produkty/602sql/registrace.php"));
#else
  stViewHTMLFile(_("http://www.602.cz/produkty/602sql/registrace.php"));
#endif
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LICWIZ_REGISTER
 */

void LicWizardPage2::OnCdLicwizRegisterClick( wxCommandEvent& event )
{ wxString url;
  url.Printf(_("http://eshop.software602.cz/cgi-bin/wbcgi/eshop/registrace/licence2.htw?prefix=SQ2&ik=%s"),
             wxString(((LicenceWizardDlg*)GetParent())->ik, *wxConvCurrent).c_str());
  stViewHTMLFile(url);
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_LICWIZ_LN
 */

void LicWizardPage2::OnCdLicwizLnUpdated( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
}


/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE1
 */

void LicWizardPage2::OnWizardpage1PageChanging( wxWizardEvent& event )
{ char lc[MAX_LICENCE_LENGTH+1];
  LicenceWizardDlg * lw = (LicenceWizardDlg*)GetParent();
  if (event.GetDirection())
  { wxString licnum = mLicNum->GetValue();
    licnum.Trim(false);  licnum.Trim(true);
   // verify the licence number:
    if (licnum.Length() != MAX_LICENCE_LENGTH)
      { error_box(_("The length of the License ID is incorrect."), lw);  event.Veto();  return; }
    strcpy(lc, licnum.mb_str(*wxConvCurrent));
    if (memcmp(lc, "SQ2", 3))
      { error_box(_("This is not a License ID."), lw);  event.Veto();  return; }
   // save it on the server (must not verify locally because cannot verify the IK):
    if (cd_Set_property_value(lw->cdp, sqlserver_owner, "AddOnLicence", 0, lc))
    { if (cd_Sz_error(lw->cdp)==ERROR_IN_FUNCTION_ARG) error_box(_("The License ID is invalid."), lw);  // better message
      else cd_Signalize2(lw->cdp, lw);  
      event.Veto();  
      return; 
    }
  }
  event.Skip();
}


/*!
 * Get bitmap resources
 */

wxBitmap LicenceWizardDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin LicenceWizardDlg bitmap retrieval
    if (name == wxT("trigwiz"))
    {
        wxBitmap bitmap(trigwiz_xpm);
        return bitmap;
    }
    return wxNullBitmap;
////@end LicenceWizardDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon LicenceWizardDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin LicenceWizardDlg icon retrieval
    return wxNullIcon;
////@end LicenceWizardDlg icon retrieval
}
/*!
 * wxEVT_WIZARD_HELP event handler for ID_WIZARD
 */

void LicenceWizardDlg::OnWizardHelp( wxWizardEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/licence.html"));
}



/*!
 * Get bitmap resources
 */

wxBitmap LicWizardPage1::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin LicWizardPage1 bitmap retrieval
    return wxNullBitmap;
////@end LicWizardPage1 bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon LicWizardPage1::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin LicWizardPage1 icon retrieval
    return wxNullIcon;
////@end LicWizardPage1 icon retrieval
}


