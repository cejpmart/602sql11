/////////////////////////////////////////////////////////////////////////////
// Name:        XMLKeyGenDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     11/03/06 10:49:20
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "XMLKeyGenDlg.h"
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

#include "XMLKeyGenDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLKeyGenDlg type definition
 */

IMPLEMENT_CLASS( XMLKeyGenDlg, wxDialog )

/*!
 * XMLKeyGenDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLKeyGenDlg, wxDialog )

////@begin XMLKeyGenDlg event table entries
    EVT_RADIOBOX( CD_KG_TYPE, XMLKeyGenDlg::OnCdKgTypeSelected )

    EVT_BUTTON( wxID_OK, XMLKeyGenDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLKeyGenDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, XMLKeyGenDlg::OnHelpClick )

////@end XMLKeyGenDlg event table entries

END_EVENT_TABLE()

/*!
 * XMLKeyGenDlg constructors
 */

XMLKeyGenDlg::XMLKeyGenDlg(XMLElementDlg * owning_dlgIn)
{ owning_dlg=owning_dlgIn; }

/*!
 * XMLKeyGenDlg creator
 */

bool XMLKeyGenDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLKeyGenDlg member initialisation
    mIdColumnCapt = NULL;
    mIdColumn = NULL;
    mExprCapt = NULL;
    mExprCapt2 = NULL;
    mExpr = NULL;
    mType = NULL;
////@end XMLKeyGenDlg member initialisation

////@begin XMLKeyGenDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLKeyGenDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLKeyGenDlg
 */

void XMLKeyGenDlg::CreateControls()
{    
////@begin XMLKeyGenDlg content construction
    XMLKeyGenDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mIdColumnCapt = new wxStaticText;
    mIdColumnCapt->Create( itemDialog1, CD_KG_ID_COLUMN_CAPT, _("Column for the key value:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(mIdColumnCapt, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mIdColumnStrings = NULL;
    mIdColumn = new wxOwnerDrawnComboBox;
    mIdColumn->Create( itemDialog1, CD_KG_ID_COLUMN, _T(""), wxDefaultPosition, wxDefaultSize, 0, mIdColumnStrings, wxCB_READONLY );
    itemBoxSizer2->Add(mIdColumn, 0, wxALIGN_LEFT|wxALL, 5);

    mExprCapt = new wxStaticText;
    mExprCapt->Create( itemDialog1, CD_KG_EXPR_CAPT, _("Expression returning the key value:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(mExprCapt, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    mExprCapt2 = new wxStaticText;
    mExprCapt2->Create( itemDialog1, CD_KG_EXPR_CAPT2, _("(in the first output parameter or in its first column)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(mExprCapt2, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    mExpr = new wxTextCtrl;
    mExpr->Create( itemDialog1, CD_KG_EXPR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(mExpr, 0, wxGROW|wxALL, 5);

    wxString mTypeStrings[] = {
        _("&Default or none"),
        _("&Evaluate the expression and use its value in the INSERT statement"),
        _("&INSERT the record and get the key value from the expression")
    };
    mType = new wxRadioBox;
    mType->Create( itemDialog1, CD_KG_TYPE, _("Key generation type"), wxDefaultPosition, wxDefaultSize, 3, mTypeStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer2->Add(mType, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer9, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton10 = new wxButton;
    itemButton10->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton11 = new wxButton;
    itemButton11->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLKeyGenDlg content construction
    mType->SetSelection(owning_dlg->key_generator==KG_NONE ? 0 : owning_dlg->key_generator==KG_PRE ? 1 : 2);
    enabling(owning_dlg->key_generator!=KG_NONE);
    fill_column_names(owning_dlg->dade, owning_dlg->mTablename, owning_dlg->mSchema, owning_dlg->mDataSource, mIdColumn, owning_dlg->id_column);
    mExpr->SetValue(owning_dlg->key_generator_expression);
}

void XMLKeyGenDlg::enabling(bool en)
{
  mIdColumnCapt->Enable(en);  mIdColumn->Enable(en);
  mExprCapt->Enable(en);  mExprCapt2->Enable(en);  mExpr->Enable(en);
}

/*!
 * Should we show tooltips?
 */

bool XMLKeyGenDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap XMLKeyGenDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLKeyGenDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLKeyGenDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLKeyGenDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLKeyGenDlg icon retrieval
    return wxNullIcon;
////@end XMLKeyGenDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_KG_TYPE
 */

void XMLKeyGenDlg::OnCdKgTypeSelected( wxCommandEvent& event )
{
  enabling(mType->GetSelection()!=0);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLKeyGenDlg::OnOkClick( wxCommandEvent& event )
{
  owning_dlg->key_generator = mType->GetSelection()==0 ? KG_NONE : mType->GetSelection()==1 ? KG_PRE : KG_POST;
  if (owning_dlg->key_generator!=KG_NONE)
  { int ind=mIdColumn->GetSelection();
    if (ind==-1)
      { wxBell();  mIdColumn->SetFocus();  return; }
    if (mExpr->GetValue().IsEmpty())
      { wxBell();  mExpr->SetFocus();  return; }
    strcpy(owning_dlg->id_column, mIdColumn->GetString(ind).mb_str(*owning_dlg->cdp->sys_conv));
    owning_dlg->key_generator_expression = mExpr->GetValue();
  }
  else
  { *owning_dlg->id_column=0;
    owning_dlg->key_generator_expression=wxEmptyString;
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLKeyGenDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void XMLKeyGenDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/xmlkeys.html"));
}


