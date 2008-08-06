/////////////////////////////////////////////////////////////////////////////
// Name:        LicenceInformationDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     10/04/04 15:16:59
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "LicenceInformationDlg.h"
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

#include "LicenceInformationDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * LicenceInformationDlg type definition
 */

IMPLEMENT_CLASS( LicenceInformationDlg, wxDialog )

/*!
 * LicenceInformationDlg event table definition
 */

BEGIN_EVENT_TABLE( LicenceInformationDlg, wxDialog )

////@begin LicenceInformationDlg event table entries
    EVT_BUTTON( CD_LIC_START, LicenceInformationDlg::OnCdLicStartClick )

    EVT_BUTTON( CD_LIC_SHOW, LicenceInformationDlg::OnCdLicShowClick )

////@end LicenceInformationDlg event table entries

END_EVENT_TABLE()

/*!
 * LicenceInformationDlg constructors
 */

LicenceInformationDlg::LicenceInformationDlg(cdp_t cdpIn)
{ cdp=cdpIn; }

/*!
 * LicenceInformationDlg creator
 */

bool LicenceInformationDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin LicenceInformationDlg member initialisation
    mIK = NULL;
    mLC = NULL;
    mType = NULL;
    mInfo = NULL;
    mStart = NULL;
////@end LicenceInformationDlg member initialisation

////@begin LicenceInformationDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end LicenceInformationDlg creation
    return TRUE;
}

/*!
 * Control creation for LicenceInformationDlg
 */

void LicenceInformationDlg::CreateControls()
{    
////@begin LicenceInformationDlg content construction

    LicenceInformationDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxFlexGridSizer* item3 = new wxFlexGridSizer(2, 2, 0, 0);
    item3->AddGrowableCol(1);
    item2->Add(item3, 0, wxGROW|wxALL, 0);

    wxStaticText* item4 = new wxStaticText;
    item4->Create( item1, wxID_STATIC, _("Product ID:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item5 = new wxTextCtrl;
    item5->Create( item1, CD_LIC_IK, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    mIK = item5;
    item3->Add(item5, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    wxStaticText* item6 = new wxStaticText;
    item6->Create( item1, wxID_STATIC, _("Licence number:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item7 = new wxTextCtrl;
    item7->Create( item1, CD_LIC_LC, _T(""), wxDefaultPosition, wxSize(240, -1), wxTE_READONLY );
    mLC = item7;
    item3->Add(item7, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    wxStaticText* item8 = new wxStaticText;
    item8->Create( item1, wxID_STATIC, _("Licence type:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item9 = new wxTextCtrl;
    item9->Create( item1, CD_LIC_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    mType = item9;
    item3->Add(item9, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    wxTextCtrl* item10 = new wxTextCtrl;
    item10->Create( item1, CD_LIC_INFO, _T(""), wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE|wxTE_READONLY );
    mInfo = item10;
    item2->Add(item10, 1, wxGROW|wxALL, 5);

    wxBoxSizer* item11 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* item12 = new wxButton;
    item12->Create( item1, CD_LIC_START, _("Start License Wizard"), wxDefaultPosition, wxDefaultSize, 0 );
    mStart = item12;
    item12->SetDefault();
    item11->Add(item12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxButton* item13 = new wxButton;
    item13->Create( item1, CD_LIC_SHOW, _("Show Licence Agreement"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add(item13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item14 = new wxButton;
    item14->Create( item1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add(item14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end LicenceInformationDlg content construction
  show_info();
  if (mStart->IsEnabled()) mStart->SetFocus();
}

/*!
 * Should we show tooltips?
 */

bool LicenceInformationDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_START
 */

void LicenceInformationDlg::OnCdLicStartClick( wxCommandEvent& event )
{
  LicenceWizardDlg dlg(cdp);
  dlg.Create(this);
  dlg.RunWizard(dlg.mPage1);
  show_info();
}

void LicenceInformationDlg::show_info(void)
{
  int network_server;  // has a valid licence, not trial
  if (cd_Get_server_info(cdp, OP_GI_LICS_SERVER, &network_server, sizeof(network_server)))
    network_server=0;
  int platform;
  cd_Get_server_info(cdp, OP_GI_SERVER_PLATFORM, &platform, sizeof(platform));
  wxString msg;
  msg.Printf(network_server ? _("602SQL Server Licence (%s)") : _("602SQL Licence (%s)"), 
             platform==PLATFORM_WINDOWS || platform==PLATFORM_WINDOWS64 ? _("Windows") : _("Linux"));
  mType->SetValue(msg);
 // LCS:
  char lc[MAX_LICENCE_LENGTH+1];
  if (cd_Get_property_value(cdp, sqlserver_owner, "AddOnLicence", 0, lc, sizeof(lc))) *lc=0;
  bool trial = /*!*lc ||*/ !stricmp(lc, "TRIAL"); // normally in the trial mode the ls from server contains word "TRIAL"
  // !*ls says that only the local version is installed
  network_access_effective = network_server || trial;
  if (network_access_effective)  // when TRIAL mode expires, network_server is false but trial is true
  {// IK: 
    char ik[MAX_LICENCE_LENGTH+1];
    if (cd_Get_server_info(cdp, OP_GI_INST_KEY, ik, sizeof(ik)))
      *ik=0;
    mIK->SetValue(wxString(ik, *wxConvCurrent));
    if (!trial)
    { mLC->SetValue(wxString(lc, *wxConvCurrent));  
      mInfo->SetValue(
#if WBVERS<1100
        DEVELOPER_LICENCE(lc) ? _("The SQL server has a DEVELOPER licence.") : 
#endif        
                      _("The SQL Server has an UNLIMITED USER license."));
    }
    else
    { mLC->SetValue(wxString(_("TRIAL")));  
      int trial_days;
      cd_Get_server_info(cdp, OP_GI_TRIAL_REM_DAYS, &trial_days, sizeof(trial_days));
      if (trial_days>0)
      { msg.Printf(_("The SQL Server is in TRIAL mode. It will expire in %d day(s). Use the License Wizard to obtain a license."), trial_days);
        mInfo->SetValue(msg);
      }
      else
        mInfo->SetValue(_("Your TRIAL has expired. Use the License Wizard to obtain a license."));
    }
    mStart->Enable();
  }
  else 
  { mIK->SetValue(_("N/A"));  // no IK because TRIAL must not start too soon!
    mLC->SetValue(_("N/A"));
    mInfo->SetValue(_("The SQL Server is licensed for local use only."));
    mStart->Disable();
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_SHOW
 */

#include "ShowTextDlg.h"

void LicenceInformationDlg::OnCdLicShowClick( wxCommandEvent& event )
{
  ShowTextDlg dlg(this, -1, _("Licence agreement"));
  dlg.mText->SetValue(get_licence_text(network_access_effective));
  dlg.ShowModal();
}


