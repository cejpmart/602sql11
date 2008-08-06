/////////////////////////////////////////////////////////////////////////////
// Name:        ColumnHintsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/30/04 10:50:28
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "ColumnHintsDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "ColumnHintsDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ColumnHintsDlg type definition
 */

IMPLEMENT_CLASS( ColumnHintsDlg, wxScrolledWindow )

/*!
 * ColumnHintsDlg event table definition
 */

BEGIN_EVENT_TABLE( ColumnHintsDlg, wxScrolledWindow )

////@begin ColumnHintsDlg event table entries
    EVT_TEXT( CD_TDHI_CAPTION, ColumnHintsDlg::OnCdTdhiCaptionUpdated )

    EVT_TEXT( CD_TDHI_HELPTEXT, ColumnHintsDlg::OnCdTdhiHelptextUpdated )

    EVT_CHECKBOX( CD_TDHI_BUBBLE, ColumnHintsDlg::OnCdTdhiBubbleClick )

    EVT_TEXT( CD_TDHI_COMBO_QUERY, ColumnHintsDlg::OnCdTdhiComboQueryUpdated )

    EVT_TEXT( CD_TDHI_COMBO_TEXT, ColumnHintsDlg::OnCdTdhiComboTextUpdated )

    EVT_TEXT( CD_TDHI_COMBO_VAL, ColumnHintsDlg::OnCdTdhiComboValUpdated )

////@end ColumnHintsDlg event table entries
END_EVENT_TABLE()

/*!
 * ColumnHintsDlg constructors
 */

ColumnHintsDlg::ColumnHintsDlg( )
{ }

ColumnHintsDlg::ColumnHintsDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{   
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ColumnHintsDlg creator
 */

bool ColumnHintsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ColumnHintsDlg member initialisation
////@end ColumnHintsDlg member initialisation
#ifdef WINS  // probably not necessary on GTK, and cannot be called before Create() 
   // change the backgroud colour so that it is the same as in dialogs:
    wxVisualAttributes va = wxDialog::GetClassDefaultAttributes();
    SetBackgroundColour(va.colBg);
#endif
////@begin ColumnHintsDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxScrolledWindow::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ColumnHintsDlg creation
   // start the functionality of scrollbars:
    FitInside();  
    SetScrollRate(8,8);
    return TRUE;
}

/*!
 * Control creation for ColumnHintsDlg
 */

void ColumnHintsDlg::CreateControls()
{    
////@begin ColumnHintsDlg content construction
    ColumnHintsDlg* itemScrolledWindow1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemScrolledWindow1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemScrolledWindow1, CD_TDHI_CAPTION_C, _("Caption:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl5 = new wxTextCtrl;
    itemTextCtrl5->Create( itemScrolledWindow1, CD_TDHI_CAPTION, _T(""), wxDefaultPosition, wxSize(150, -1), 0 );
    itemFlexGridSizer3->Add(itemTextCtrl5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemScrolledWindow1, CD_TDHI_HELPTEXT_C, _("Help text:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl7 = new wxTextCtrl;
    itemTextCtrl7->Create( itemScrolledWindow1, CD_TDHI_HELPTEXT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemTextCtrl7, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxCheckBox* itemCheckBox9 = new wxCheckBox;
    itemCheckBox9->Create( itemScrolledWindow1, CD_TDHI_BUBBLE, _("Show the help text in a tool tip"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox9->SetValue(FALSE);
    itemFlexGridSizer3->Add(itemCheckBox9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer10Static = new wxStaticBox(itemScrolledWindow1, wxID_ANY, _("Choice in a combobox"));
    wxStaticBoxSizer* itemStaticBoxSizer10 = new wxStaticBoxSizer(itemStaticBoxSizer10Static, wxHORIZONTAL);
    itemBoxSizer2->Add(itemStaticBoxSizer10, 0, wxGROW|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer11 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer11->AddGrowableCol(1);
    itemStaticBoxSizer10->Add(itemFlexGridSizer11, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxStaticText* itemStaticText12 = new wxStaticText;
    itemStaticText12->Create( itemScrolledWindow1, CD_TDHI_COMBO_QUERY_C, _("Query returning the choice:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl13 = new wxTextCtrl;
    itemTextCtrl13->Create( itemScrolledWindow1, CD_TDHI_COMBO_QUERY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl13, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText14 = new wxStaticText;
    itemStaticText14->Create( itemScrolledWindow1, CD_TDHI_COMBO_TEXT_C, _("Column with value:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText14, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl15 = new wxTextCtrl;
    itemTextCtrl15->Create( itemScrolledWindow1, CD_TDHI_COMBO_TEXT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl15, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText16 = new wxStaticText;
    itemStaticText16->Create( itemScrolledWindow1, CD_TDHI_COMBO_VAL_C, _("Column with value description:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText16, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl17 = new wxTextCtrl;
    itemTextCtrl17->Create( itemScrolledWindow1, CD_TDHI_COMBO_VAL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl17, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ColumnHintsDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool ColumnHintsDlg::ShowToolTips()
{
    return TRUE;
}

void ColumnHintsDlg::changed(bool major_change)
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  if (tde->feedback_disabled) return;  // e.g. wxTextStrl::SetValue() called
  char * cur_name;
  atr_all * cur_atr = tde->find_column_descr(tde->current_row_selected, &cur_name);
  if (cur_atr) 
    tde->is_modified(cur_atr, cur_name, tde->current_row_selected);
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_CAPTION
 */

void ColumnHintsDlg::OnCdTdhiCaptionUpdated( wxCommandEvent& event )
{ changed(false); }

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_HELPTEXT
 */

void ColumnHintsDlg::OnCdTdhiHelptextUpdated( wxCommandEvent& event )
{ changed(false); }

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TDHI_BUBBLE
 */

void ColumnHintsDlg::OnCdTdhiBubbleClick( wxCommandEvent& event )
{ changed(false); }

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_COMBO_QUERY
 */

void ColumnHintsDlg::OnCdTdhiComboQueryUpdated( wxCommandEvent& event )
{ changed(false); }

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_COMBO_TEXT
 */

void ColumnHintsDlg::OnCdTdhiComboTextUpdated( wxCommandEvent& event )
{ changed(false); }

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_COMBO_VAL
 */

void ColumnHintsDlg::OnCdTdhiComboValUpdated( wxCommandEvent& event )
{ changed(false); }



/*!
 * Get bitmap resources
 */

wxBitmap ColumnHintsDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ColumnHintsDlg bitmap retrieval
    return wxNullBitmap;
////@end ColumnHintsDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ColumnHintsDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ColumnHintsDlg icon retrieval
    return wxNullIcon;
////@end ColumnHintsDlg icon retrieval
}
