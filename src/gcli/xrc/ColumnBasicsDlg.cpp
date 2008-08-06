/////////////////////////////////////////////////////////////////////////////
// Name:        ColumnBasicsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/28/04 17:01:59
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "ColumnBasicsDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "ColumnBasicsDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ColumnBasicsDlg type definition
 */

IMPLEMENT_CLASS( ColumnBasicsDlg, wxScrolledWindow )

/*!
 * ColumnBasicsDlg event table definition
 */

BEGIN_EVENT_TABLE( ColumnBasicsDlg, wxScrolledWindow )

////@begin ColumnBasicsDlg event table entries
    EVT_CHECKBOX( CD_TDCB_UNICODE, ColumnBasicsDlg::OnCdTdcbUnicodeClick )

    EVT_CHECKBOX( CD_TDCB_IGNORE, ColumnBasicsDlg::OnCdTdcbIgnoreClick )

    EVT_COMBOBOX( CD_TDCB_LANG, ColumnBasicsDlg::OnCdTdcbLangSelected )
    EVT_TEXT( CD_TDCB_LANG, ColumnBasicsDlg::OnCdTdcbLangUpdated )

    EVT_SPINCTRL( CD_TDCB_SCALE, ColumnBasicsDlg::OnCdTdcbScaleUpdated )
    EVT_TEXT( CD_TDCB_SCALE, ColumnBasicsDlg::OnCdTdcbScaleTextUpdated )

    EVT_TEXT( CD_TDCB_COMMENT, ColumnBasicsDlg::OnCdTdcbCommentUpdated )

////@end ColumnBasicsDlg event table entries

END_EVENT_TABLE()

/*!
 * ColumnBasicsDlg constructors
 */

ColumnBasicsDlg::ColumnBasicsDlg( )
{
}

ColumnBasicsDlg::ColumnBasicsDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ColumnBasicsDlg creator
 */

bool ColumnBasicsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ColumnBasicsDlg member initialisation
    mUnicode = NULL;
    mIgnore = NULL;
    mLangCapt = NULL;
    mLang = NULL;
    mScaleCapt = NULL;
    mScale = NULL;
    mCommentCapt = NULL;
    mComment = NULL;
////@end ColumnBasicsDlg member initialisation
#ifdef WINS  // probably not necessary on GTK, and cannot be called before Create() 
   // change the backgroud colour so that it is the same as in dialogs:
    wxVisualAttributes va = wxDialog::GetClassDefaultAttributes();
    SetBackgroundColour(va.colBg);
#endif
////@begin ColumnBasicsDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxScrolledWindow::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ColumnBasicsDlg creation
   // start the functionality of scrollbars:
    FitInside();  
    SetScrollRate(8,8);
    return TRUE;
}

/*!
 * Control creation for ColumnBasicsDlg
 */

void ColumnBasicsDlg::CreateControls()
{    
////@begin ColumnBasicsDlg content construction
    ColumnBasicsDlg* itemScrolledWindow1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemScrolledWindow1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    mUnicode = new wxCheckBox;
    mUnicode->Create( itemScrolledWindow1, CD_TDCB_UNICODE, _("Unicode"), wxDefaultPosition, wxSize(200, -1), wxCHK_2STATE );
    mUnicode->SetValue(FALSE);
    itemBoxSizer3->Add(mUnicode, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mIgnore = new wxCheckBox;
    mIgnore->Create( itemScrolledWindow1, CD_TDCB_IGNORE, _("Ignore case"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mIgnore->SetValue(FALSE);
    itemBoxSizer3->Add(mIgnore, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer6 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer6->AddGrowableRow(2);
    itemFlexGridSizer6->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer6, 1, wxGROW|wxALL, 0);

    mLangCapt = new wxStaticText;
    mLangCapt->Create( itemScrolledWindow1, CD_TDCB_LANG_CAPT, _("Collation and charset:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer6->Add(mLangCapt, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mLangStrings = NULL;
    mLang = new wxOwnerDrawnComboBox;
    mLang->Create( itemScrolledWindow1, CD_TDCB_LANG, _T(""), wxDefaultPosition, wxSize(200, -1), 0, mLangStrings, wxCB_DROPDOWN );
    itemFlexGridSizer6->Add(mLang, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mScaleCapt = new wxStaticText;
    mScaleCapt->Create( itemScrolledWindow1, CD_TDCB_SCALE_CAPT, _("Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer6->Add(mScaleCapt, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mScale = new wxSpinCtrl;
    mScale->Create( itemScrolledWindow1, CD_TDCB_SCALE, _("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 100, 0 );
    itemFlexGridSizer6->Add(mScale, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mCommentCapt = new wxStaticText;
    mCommentCapt->Create( itemScrolledWindow1, wxID_STATIC, _("Comment:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer6->Add(mCommentCapt, 0, wxALIGN_LEFT|wxALIGN_TOP|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mComment = new wxTextCtrl;
    mComment->Create( itemScrolledWindow1, CD_TDCB_COMMENT, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    itemFlexGridSizer6->Add(mComment, 1, wxGROW|wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end ColumnBasicsDlg content construction
#ifdef WINS  // repairing error in spin control in WX 2.5.2
    HWND hEdit  = (HWND)::SendMessage((HWND)mScale->GetHandle(), WM_USER+106, 0, 0);
    ::SendMessage(hEdit, WM_SETFONT, (WPARAM)system_gui_font.GetResourceHandle(), MAKELPARAM(TRUE, 0));
#endif
}

/*!
 * Should we show tooltips?
 */

bool ColumnBasicsDlg::ShowToolTips()
{
    return TRUE;
}

void ColumnBasicsDlg::changed(bool major_change)
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  if (tde->feedback_disabled) return;  // e.g. wxTextStrl::SetValue() called
  char * cur_name;
  atr_all * cur_atr = tde->find_column_descr(tde->current_row_selected, &cur_name);
  if (cur_atr) 
    tde->is_modified(cur_atr, cur_name, tde->current_row_selected);
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TDCB_NULL_ALLOWED
 */

//void ColumnBasicsDlg::OnCdTdcbNullAllowedClick( wxCommandEvent& event )
//{ changed(true); }


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TDCB_UNICODE
 */

void ColumnBasicsDlg::OnCdTdcbUnicodeClick( wxCommandEvent& event )
{ changed(true); }

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TDCB_IGNORE
 */

void ColumnBasicsDlg::OnCdTdcbIgnoreClick( wxCommandEvent& event )
{ changed(true); }

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDCB_LANG
 */

void ColumnBasicsDlg::OnCdTdcbLangUpdated( wxCommandEvent& event )
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  if (tde->feedback_disabled) return;  // e.g. wxTextStrl::SetValue() called
 // first save the new domain name otherwise the new conversion cannot be generated:
  char * cur_name;
  atr_all * cur_atr = tde->find_column_descr(tde->current_row_selected, &cur_name);
  if (tde->WB_table && (tde->type_list[cur_atr->type].flags & TP_FL_DOMAIN)!=0)
  { wxString str = mLang->GetValue();
    str.Trim(true);  str.Trim(false);
    strmaxcpy(cur_name+sizeof(tobjname), str.mb_str(*tde->cdp->sys_conv), sizeof(tobjname));
    cur_atr->specif.domain_num=NOOBJECT;
  }
  changed(true); 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDCB_SCALE
 */

void ColumnBasicsDlg::OnCdTdcbScaleUpdated( wxSpinEvent& event )
{ changed(true); }

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDCB_COMMENT
 */

void ColumnBasicsDlg::OnCdTdcbCommentUpdated( wxCommandEvent& event )
{ changed(false); }


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDCB_SCALE
 */

void ColumnBasicsDlg::OnCdTdcbScaleTextUpdated( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap ColumnBasicsDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ColumnBasicsDlg bitmap retrieval
    return wxNullBitmap;
////@end ColumnBasicsDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ColumnBasicsDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ColumnBasicsDlg icon retrieval
    return wxNullIcon;
////@end ColumnBasicsDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TDCB_LANG
 */

void ColumnBasicsDlg::OnCdTdcbLangSelected( wxCommandEvent& event )
{
  OnCdTdcbLangUpdated(event);  // same handling
  event.Skip();
}


