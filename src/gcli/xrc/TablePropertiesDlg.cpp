/////////////////////////////////////////////////////////////////////////////
// Name:        TablePropertiesDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/08/04 07:35:41
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "TablePropertiesDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "TablePropertiesDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * TablePropertiesDlg type definition
 */

IMPLEMENT_CLASS( TablePropertiesDlg, wxDialog )

/*!
 * TablePropertiesDlg event table definition
 */

BEGIN_EVENT_TABLE( TablePropertiesDlg, wxDialog )

////@begin TablePropertiesDlg event table entries
    EVT_CHECKBOX( CD_TFL_ZCR, TablePropertiesDlg::OnCdTflZcrClick )

    EVT_CHECKBOX( CD_TFL_LUO, TablePropertiesDlg::OnCdTflLuoClick )

    EVT_CHECKBOX( CD_TFL_DETECT, TablePropertiesDlg::OnCdTflDetectClick )

    EVT_BUTTON( wxID_HELP, TablePropertiesDlg::OnHelpClick )

////@end TablePropertiesDlg event table entries

END_EVENT_TABLE()

/*!
 * TablePropertiesDlg constructors
 */

TablePropertiesDlg::TablePropertiesDlg( )
{
}

TablePropertiesDlg::TablePropertiesDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * TablePropertiesDlg creator
 */

bool TablePropertiesDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin TablePropertiesDlg member initialisation
    mTopSizer = NULL;
    mReplSizer = NULL;
    mRepl = NULL;
    mReplGroup = NULL;
////@end TablePropertiesDlg member initialisation

////@begin TablePropertiesDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end TablePropertiesDlg creation
    return TRUE;
}

/*!
 * Control creation for TablePropertiesDlg
 */

void TablePropertiesDlg::CreateControls()
{    
////@begin TablePropertiesDlg content construction

    TablePropertiesDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    mTopSizer = item2;
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxCheckBox* item3 = new wxCheckBox;
    item3->Create( item1, CD_TFL_REC_PRIVILS, _("Enable record-level privileges"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->SetValue(FALSE);
    item2->Add(item3, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* item4 = new wxBoxSizer(wxVERTICAL);
    mReplSizer = item4;
    item2->Add(item4, 0, wxGROW|wxALL, 0);

    wxCheckBox* item5 = new wxCheckBox;
    item5->Create( item1, CD_TFL_ZCR, _("Enable replication"), wxDefaultPosition, wxDefaultSize, 0 );
    mRepl = item5;
    item5->SetValue(FALSE);
    item4->Add(item5, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticBox* item6Static = new wxStaticBox(item1, wxID_ANY, _("Replication properties"));
    wxStaticBoxSizer* item6 = new wxStaticBoxSizer(item6Static, wxVERTICAL);
    mReplGroup = item6Static;
    item4->Add(item6, 0, wxALIGN_LEFT|wxALL, 5);

    wxCheckBox* item7 = new wxCheckBox;
    item7->Create( item1, CD_TFL_UNIKEY, _("Create universal replication key"), wxDefaultPosition, wxDefaultSize, 0 );
    item7->SetValue(FALSE);
    item6->Add(item7, 0, wxALIGN_LEFT|wxALL, 5);

    wxCheckBox* item8 = new wxCheckBox;
    item8->Create( item1, CD_TFL_LUO, _("Prevent echos"), wxDefaultPosition, wxDefaultSize, 0 );
    item8->SetValue(FALSE);
    item6->Add(item8, 0, wxALIGN_LEFT|wxALL, 5);

    wxCheckBox* item9 = new wxCheckBox;
    item9->Create( item1, CD_TFL_DETECT, _("Detect conflicts"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->SetValue(FALSE);
    item6->Add(item9, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* item10 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item10, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* item11 = new wxButton;
    item11->Create( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->SetDefault();
    item10->Add(item11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item12 = new wxButton;
    item12->Create( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item10->Add(item12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item13 = new wxButton;
    item13->Create( item1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item10->Add(item13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);


    // Set validators
    item3->SetValidator( wxGenericValidator(& rec_privils) );
    item5->SetValidator( wxGenericValidator(& zcr) );
    item7->SetValidator( wxGenericValidator(& unikey) );
    item8->SetValidator( wxGenericValidator(& luo) );
    item9->SetValidator( wxGenericValidator(& detect) );
////@end TablePropertiesDlg content construction
  mTopSizer->Show(mReplSizer, false);
}

/*!
 * Should we show tooltips?
 */

bool TablePropertiesDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TFL_ZCR
 */

void TablePropertiesDlg::OnCdTflZcrClick( wxCommandEvent& event )
{ if (((wxCheckBox*)event.GetEventObject())->GetValue())
  { ((wxCheckBox*)FindWindow(CD_TFL_UNIKEY))->Enable();
    ((wxCheckBox*)FindWindow(CD_TFL_LUO))->Enable();
    ((wxCheckBox*)FindWindow(CD_TFL_DETECT))->Enable();
  }
  else
  { ((wxCheckBox*)FindWindow(CD_TFL_UNIKEY))->SetValue(false);
    ((wxCheckBox*)FindWindow(CD_TFL_LUO))->SetValue(false);
    ((wxCheckBox*)FindWindow(CD_TFL_DETECT))->SetValue(false);
    ((wxCheckBox*)FindWindow(CD_TFL_UNIKEY))->Disable();
    ((wxCheckBox*)FindWindow(CD_TFL_LUO))->Disable();
    ((wxCheckBox*)FindWindow(CD_TFL_DETECT))->Disable();
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TFL_DETECT
 */

void TablePropertiesDlg::OnCdTflDetectClick( wxCommandEvent& event )
{ if (((wxCheckBox*)event.GetEventObject())->GetValue())
    ((wxCheckBox*)FindWindow(CD_TFL_LUO))->SetValue(true);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TFL_LUO
 */

void TablePropertiesDlg::OnCdTflLuoClick( wxCommandEvent& event )
{ if (!((wxCheckBox*)event.GetEventObject())->GetValue())
    ((wxCheckBox*)FindWindow(CD_TFL_DETECT))->SetValue(false);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void TablePropertiesDlg::OnHelpClick( wxCommandEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/rightrecdat.html")); }


