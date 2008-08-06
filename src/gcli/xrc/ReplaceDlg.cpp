/////////////////////////////////////////////////////////////////////////////
// Name:        ReplaceDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/19/04 08:55:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "ReplaceDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "ReplaceDlg.h"
ReplaceDlg * pending_replace_dialog = NULL;
int last_search_flags = wxFR_DOWN;

////@begin XPM images
////@end XPM images

/*!
 * ReplaceDlg type definition
 */

IMPLEMENT_CLASS( ReplaceDlg, wxDialog )

/*!
 * ReplaceDlg event table definition
 */

BEGIN_EVENT_TABLE( ReplaceDlg, wxDialog )

////@begin ReplaceDlg event table entries
    EVT_RADIOBOX( CD_REPL_SEL, ReplaceDlg::OnCdReplSelSelected )

    EVT_BUTTON( CD_REPL_FIND, ReplaceDlg::OnCdReplFindClick )

    EVT_BUTTON( CD_REPL_REPL, ReplaceDlg::OnCdReplReplClick )

    EVT_BUTTON( CD_REPL_REPLALL, ReplaceDlg::OnCdReplReplallClick )

    EVT_BUTTON( wxID_CANCEL, ReplaceDlg::OnCancelClick )

////@end ReplaceDlg event table entries
    EVT_BUTTON( wxID_CANCEL, ReplaceDlg::OnCancelClick )
    EVT_CLOSE( ReplaceDlg::OnClose )

END_EVENT_TABLE()

/*!
 * ReplaceDlg constructors
 */

ReplaceDlg::ReplaceDlg(editor_type * edIn, wxString & ini_strIn, bool any_selIn, bool multiline_selIn)
{ ed=edIn;  ini_str=ini_strIn;
  any_sel=any_selIn;  multiline_sel=multiline_selIn;
}

ReplaceDlg::~ReplaceDlg(void)
{ pending_replace_dialog=NULL; }
/*!
 * ReplaceDlg creator
 */

bool ReplaceDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ReplaceDlg member initialisation
    m_textFind = NULL;
    m_textRepl = NULL;
    m_chkWord = NULL;
    m_chkCase = NULL;
    m_chkBack = NULL;
    m_radioExtent = NULL;
    m_button_find = NULL;
    m_button_replace = NULL;
////@end ReplaceDlg member initialisation

////@begin ReplaceDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ReplaceDlg creation
    return TRUE;
}

/*!
 * Control creation for ReplaceDlg
 */

void ReplaceDlg::CreateControls()
{    
////@begin ReplaceDlg content construction
    ReplaceDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxALIGN_TOP|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer4->AddGrowableCol(1);
    itemBoxSizer3->Add(itemFlexGridSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Fi&nd what:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* m_textFindStrings = NULL;
    m_textFind = new wxOwnerDrawnComboBox;
    m_textFind->Create( itemDialog1, CD_REPL_FINDSTR, _T(""), wxDefaultPosition, wxSize(166, -1), 0, m_textFindStrings, wxCB_DROPDOWN );
    itemFlexGridSizer4->Add(m_textFind, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("Re&place with:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* m_textReplStrings = NULL;
    m_textRepl = new wxOwnerDrawnComboBox;
    m_textRepl->Create( itemDialog1, CD_REPL_REPLSTR, _T(""), wxDefaultPosition, wxSize(166, -1), 0, m_textReplStrings, wxCB_DROPDOWN );
    itemFlexGridSizer4->Add(m_textRepl, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer9, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer9->Add(itemBoxSizer10, 1, wxALIGN_TOP|wxALL, 0);

    m_chkWord = new wxCheckBox;
    m_chkWord->Create( itemDialog1, CD_REPL_WORD, _("Match &whole word only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_chkWord->SetValue(FALSE);
    itemBoxSizer10->Add(m_chkWord, 0, wxALIGN_LEFT|wxALL, 5);

    m_chkCase = new wxCheckBox;
    m_chkCase->Create( itemDialog1, CD_REPL_CASE, _("Match &case"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_chkCase->SetValue(FALSE);
    itemBoxSizer10->Add(m_chkCase, 0, wxALIGN_LEFT|wxALL, 5);

    m_chkBack = new wxCheckBox;
    m_chkBack->Create( itemDialog1, CD_REPL_BACK, _("Search back"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_chkBack->SetValue(FALSE);
    itemBoxSizer10->Add(m_chkBack, 0, wxALIGN_LEFT|wxALL, 5);

    wxString m_radioExtentStrings[] = {
        _("&Selection"),
        _("Wh&ole text")
    };
    m_radioExtent = new wxRadioBox;
    m_radioExtent->Create( itemDialog1, CD_REPL_SEL, _("Replace in"), wxDefaultPosition, wxDefaultSize, 2, m_radioExtentStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer9->Add(m_radioExtent, 0, wxALIGN_TOP|wxALL, 5);

    wxBoxSizer* itemBoxSizer15 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer15, 0, wxALIGN_TOP|wxALL, 0);

    m_button_find = new wxButton;
    m_button_find->Create( itemDialog1, CD_REPL_FIND, _("&Find next"), wxDefaultPosition, wxDefaultSize, 0 );
    m_button_find->SetDefault();
    itemBoxSizer15->Add(m_button_find, 0, wxGROW|wxALL, 5);

    m_button_replace = new wxButton;
    m_button_replace->Create( itemDialog1, CD_REPL_REPL, _("&Replace"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(m_button_replace, 0, wxGROW|wxALL, 5);

    wxButton* itemButton18 = new wxButton;
    itemButton18->Create( itemDialog1, CD_REPL_REPLALL, _("Replace &All"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(itemButton18, 0, wxGROW|wxALL, 5);

    wxButton* itemButton19 = new wxButton;
    itemButton19->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(itemButton19, 0, wxGROW|wxALL, 5);

////@end ReplaceDlg content construction
    find_history.fill(m_textFind);
    replace_history.fill(m_textRepl);
    m_textFind->SetValue(ini_str);
    if (!any_sel) 
    { m_radioExtent->SetSelection(1);
      m_radioExtent->Disable();
    }
    else
    { m_radioExtent->SetSelection(multiline_sel ? 0 : 1);
      if (multiline_sel)
      { m_button_find->Disable();
        m_button_replace->Disable();
      }
    }
    if (!(last_search_flags & wxFR_DOWN))   m_chkBack->SetValue(true);
    if (last_search_flags & wxFR_MATCHCASE) m_chkCase->SetValue(true);
    if (last_search_flags & wxFR_WHOLEWORD) m_chkWord->SetValue(true);
}

/*!
 * Should we show tooltips?
 */

bool ReplaceDlg::ShowToolTips()
{
    return TRUE;
}

// ----------------------------------------------------------------------------
// send the notification event
// ----------------------------------------------------------------------------

void ReplaceDlg::SendEvent(const wxEventType& evtType)
{
    wxFindDialogEvent event(evtType, GetId());
    event.SetEventObject(this);
    event.SetFindString(m_textFind->GetValue());
    event.SetReplaceString(m_textRepl->GetValue());
    int flags = 0;
    if ( m_chkCase->GetValue() )
        flags |= wxFR_MATCHCASE;
    if ( m_chkWord->GetValue() )
        flags |= wxFR_WHOLEWORD;
    if (m_radioExtent->GetSelection() == 0 )
        flags |= FR_INSELECTION;
    if (!m_chkBack->GetValue() )
        flags |= wxFR_DOWN;
    event.SetFlags(flags);
   // must not send to the parent because parent of a modeless dialog must be the main frame!
    ed->GetEventHandler()->ProcessEvent(event);
}



/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REPL_FIND
 */

void ReplaceDlg::OnCdReplFindClick( wxCommandEvent& event )
{ wxString str = m_textFind->GetValue();
  find_history.add_to_history(str);
  find_history.fill(m_textFind);  // update the history now, because the dialog is not closed by the action
  m_textFind->SetValue(str);  // return the string cleared by fill_history();
  SendEvent(wxEVT_COMMAND_FIND_NEXT); 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REPL_REPL
 */

void ReplaceDlg::OnCdReplReplClick( wxCommandEvent& event )
{ wxString str = m_textFind->GetValue();
  find_history.add_to_history(str);
  find_history.fill(m_textFind);  // update the history now, because the dialog is not closed by the action
  m_textFind->SetValue(str);  // return the string cleared by fill_history();
  str = m_textRepl->GetValue();
  replace_history.add_to_history(str);
  replace_history.fill(m_textRepl);  // update the history now, because the dialog is not closed by the action
  m_textRepl->SetValue(str);  // return the string cleared by fill_history();
  SendEvent(wxEVT_COMMAND_FIND_REPLACE); 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REPL_REPLALL
 */

void ReplaceDlg::OnCdReplReplallClick( wxCommandEvent& event )
{ wxString str = m_textFind->GetValue();
  find_history.add_to_history(str);
  find_history.fill(m_textFind);  // update the history now, because the dialog is not closed by the action
  m_textFind->SetValue(str);  // return the string cleared by fill_history();
  str = m_textRepl->GetValue();
  replace_history.add_to_history(str);
  replace_history.fill(m_textRepl);  // update the history now, because the dialog is not closed by the action
  m_textRepl->SetValue(str);  // return the string cleared by fill_history();
  SendEvent(wxEVT_COMMAND_FIND_REPLACE_ALL); 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void ReplaceDlg::OnCancelClick( wxCommandEvent& event )
{ SendEvent(wxEVT_COMMAND_FIND_CLOSE); 
  Destroy();
}

void ReplaceDlg::OnClose( wxCloseEvent& event )
{ SendEvent(wxEVT_COMMAND_FIND_CLOSE); 
  Destroy();
}


/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_REPL_SEL
 */

void ReplaceDlg::OnCdReplSelSelected( wxCommandEvent& event )
{
  if (m_radioExtent->GetSelection())
  { m_button_find->Enable();
    m_button_replace->Enable();
  }
  else
  { m_button_find->Disable();
    m_button_replace->Disable();
  }
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap ReplaceDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ReplaceDlg bitmap retrieval
    return wxNullBitmap;
////@end ReplaceDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ReplaceDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ReplaceDlg icon retrieval
    return wxNullIcon;
////@end ReplaceDlg icon retrieval
}
