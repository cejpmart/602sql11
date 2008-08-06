/////////////////////////////////////////////////////////////////////////////
// Name:        XMLXSDDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/09/05 14:33:08
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "XMLXSDDlg.h"
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

#include "XMLXSDDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLXSDDlg type definition
 */

IMPLEMENT_CLASS( XMLXSDDlg, wxDialog )

/*!
 * XMLXSDDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLXSDDlg, wxDialog )

////@begin XMLXSDDlg event table entries
    EVT_BUTTON( wxID_OK, XMLXSDDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLXSDDlg::OnCancelClick )

////@end XMLXSDDlg event table entries

END_EVENT_TABLE()

/*!
 * XMLXSDDlg constructors
 */

XMLXSDDlg::XMLXSDDlg(t_xsd_link * xsdIn)
{ xsd=xsdIn; }

/*!
 * XMLXSDDlg creator
 */

bool XMLXSDDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLXSDDlg member initialisation
    mPrefix = NULL;
    mLength = NULL;
    mUnicode = NULL;
    mIDType = NULL;
////@end XMLXSDDlg member initialisation

////@begin XMLXSDDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLXSDDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLXSDDlg
 */

void XMLXSDDlg::CreateControls()
{    
////@begin XMLXSDDlg content construction
    XMLXSDDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Table name prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPrefix = new wxTextCtrl;
    mPrefix->Create( itemDialog1, CD_XMLXSD_PREFIX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mPrefix, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, wxID_STATIC, _("Default string length:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLength = new wxSpinCtrl;
    mLength->Create( itemDialog1, CD_XMLXSD_LENGTH, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
    itemFlexGridSizer3->Add(mLength, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mUnicode = new wxCheckBox;
    mUnicode->Create( itemDialog1, CD_XMLXSD_UNICODE, _("Use Unicode strings"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mUnicode->SetValue(FALSE);
    itemFlexGridSizer3->Add(mUnicode, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText10 = new wxStaticText;
    itemStaticText10->Create( itemDialog1, wxID_STATIC, _("ID column type:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mIDTypeStrings = NULL;
    mIDType = new wxOwnerDrawnComboBox;
    mIDType->Create( itemDialog1, CD_XMLXSD_IDTYPE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mIDTypeStrings, wxCB_READONLY );
    itemFlexGridSizer3->Add(mIDType, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton13->SetDefault();
    itemBoxSizer12->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton14 = new wxButton;
    itemButton14->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLXSDDlg content construction
   // set restrictions:
    mPrefix->SetMaxLength(20);
    mLength->SetRange(2, MAX_FIXED_STRING_LENGTH);
    mIDType->Append(_("Integer (32-bit)"));
    mIDType->Append(_("BigInt (64-bit)"));
   // show values:
    mPrefix->SetValue(wxString(xsd->tabname_prefix, *wxConvCurrent));
    mIDType->SetSelection(xsd->id_type==ATT_INT32 ? 0 : 1);
    mLength->SetValue(int2wxstr(xsd->string_length));
    mUnicode->SetValue(xsd->unicode_strings);
}

/*!
 * Should we show tooltips?
 */

bool XMLXSDDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLXSDDlg::OnOkClick( wxCommandEvent& event )
{ wxString pref;
  pref = mPrefix->GetValue();  pref.Trim(false);  pref.Trim(true);
  strcpy(xsd->tabname_prefix, pref.mb_str(*wxConvCurrent));
  xsd->id_type = mIDType->GetSelection() ? ATT_INT64 : ATT_INT32;
  xsd->string_length=mLength->GetValue();
  xsd->unicode_strings = mUnicode->GetValue();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLXSDDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap XMLXSDDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLXSDDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLXSDDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLXSDDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLXSDDlg icon retrieval
    return wxNullIcon;
////@end XMLXSDDlg icon retrieval
}
