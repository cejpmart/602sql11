/////////////////////////////////////////////////////////////////////////////
// Name:        ExtEditorsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     07/25/06 15:34:27
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ExtEditorsDlg.h"
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

#include "ExtEditorsDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ExtEditorsDlg type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ExtEditorsDlg, wxDialog )

/*!
 * ExtEditorsDlg event table definition
 */

BEGIN_EVENT_TABLE( ExtEditorsDlg, wxDialog )

////@begin ExtEditorsDlg event table entries
    EVT_BUTTON( CD_EE_STSH_BROWSE, ExtEditorsDlg::OnCdEeStshBrowseClick )

    EVT_BUTTON( CD_EE_CSS_BROWSE, ExtEditorsDlg::OnCdEeCssBrowseClick )

    EVT_BUTTON( wxID_OK, ExtEditorsDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, ExtEditorsDlg::OnCancelClick )

////@end ExtEditorsDlg event table entries

END_EVENT_TABLE()

/*!
 * ExtEditorsDlg constructors
 */

ExtEditorsDlg::ExtEditorsDlg( )
{
}

ExtEditorsDlg::ExtEditorsDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ExtEditorsDlg creator
 */

bool ExtEditorsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ExtEditorsDlg member initialisation
    mStsh = NULL;
    mCss = NULL;
////@end ExtEditorsDlg member initialisation

////@begin ExtEditorsDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ExtEditorsDlg creation
    return TRUE;
}

/*!
 * Control creation for ExtEditorsDlg
 */

void ExtEditorsDlg::CreateControls()
{    
////@begin ExtEditorsDlg content construction
    ExtEditorsDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 3, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("XML Style Sheet Editor:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mStsh = new wxTextCtrl;
    mStsh->Create( itemDialog1, CD_EE_STSH, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mStsh, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    wxButton* itemButton6 = new wxButton;
    itemButton6->Create( itemDialog1, CD_EE_STSH_BROWSE, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemButton6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("Cascading Style Sheet Editor:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText7, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mCss = new wxTextCtrl;
    mCss->Create( itemDialog1, CD_EE_CSS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mCss, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, CD_EE_CSS_BROWSE, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemButton9, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton11 = new wxButton;
    itemButton11->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ExtEditorsDlg content construction
    mStsh->SetMaxLength(256);
    mCss ->SetMaxLength(256);
   // show values:
    mStsh->SetValue(get_stsh_editor_path(false));
    mCss ->SetValue(get_stsh_editor_path(true ));
}

/*!
 * Should we show tooltips?
 */

bool ExtEditorsDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ExtEditorsDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ExtEditorsDlg bitmap retrieval
    return wxNullBitmap;
////@end ExtEditorsDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ExtEditorsDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ExtEditorsDlg icon retrieval
    return wxNullIcon;
////@end ExtEditorsDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EE_CSS_BROWSE
 */

void ExtEditorsDlg::OnCdEeCssBrowseClick( wxCommandEvent& event )
{
  wxString path, cmd = mCss->GetValue();
  cmd = SelectFileName(path, cmd, wxString(FilterEXE)+FilterAll, false, this);
  if (!cmd.IsEmpty())
    mCss->SetValue(cmd);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EE_STSH_BROWSE
 */

void ExtEditorsDlg::OnCdEeStshBrowseClick( wxCommandEvent& event )
{
  wxString path, cmd = mStsh->GetValue();
  cmd = SelectFileName(path, cmd, wxString(FilterEXE)+FilterAll, false, this);
  if (!cmd.IsEmpty())
    mStsh->SetValue(cmd);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void ExtEditorsDlg::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in ExtEditorsDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in ExtEditorsDlg. 
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void ExtEditorsDlg::OnOkClick( wxCommandEvent& event )
{ wxString cmd;
 // save values:
  cmd = mStsh->GetValue();  cmd.Trim(false);  cmd.Trim(true);
  write_profile_string("Editors", "StyleSheet", cmd.mb_str(*wxConvCurrent));
  cmd = mCss ->GetValue();  cmd.Trim(false);  cmd.Trim(true);
  write_profile_string("Editors", "CSS",        cmd.mb_str(*wxConvCurrent));
  event.Skip();
}


