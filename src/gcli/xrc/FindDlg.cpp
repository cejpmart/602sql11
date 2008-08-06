/////////////////////////////////////////////////////////////////////////////
// Name:        FindDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/19/04 14:31:34
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "FindDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "FindDlg.h"
FindDlg * pending_find_dialog = NULL;

////@begin XPM images
////@end XPM images

/*!
 * FindDlg type definition
 */

IMPLEMENT_CLASS( FindDlg, wxDialog )

/*!
 * FindDlg event table definition
 */

BEGIN_EVENT_TABLE( FindDlg, wxDialog )

////@begin FindDlg event table entries
    EVT_BUTTON( CD_FIND_FIND, FindDlg::OnCdFindFindClick )

    EVT_BUTTON( wxID_CANCEL, FindDlg::OnCancelClick )

////@end FindDlg event table entries
    EVT_BUTTON( wxID_CANCEL, FindDlg::OnCancelClick )
    EVT_CLOSE( FindDlg::OnClose)

END_EVENT_TABLE()

/*!
 * FindDlg constructors
 */

FindDlg::FindDlg(editor_type * edIn, wxString & ini_strIn)
{ ed=edIn;  ini_str=ini_strIn;
}

FindDlg::~FindDlg(void)
{ pending_find_dialog=NULL; }
/*!
 * FindDlg creator
 */

bool FindDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin FindDlg member initialisation
    m_textFind = NULL;
    m_chkWord = NULL;
    m_chkCase = NULL;
    m_chkBack = NULL;
    m_button_find = NULL;
////@end FindDlg member initialisation

////@begin FindDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end FindDlg creation
    return TRUE;
}

/*!
 * Control creation for FindDlg
 */

void FindDlg::CreateControls()
{    
////@begin FindDlg content construction
    FindDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxALIGN_TOP|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(1, 2, 0, 0);
    itemFlexGridSizer4->AddGrowableCol(1);
    itemBoxSizer3->Add(itemFlexGridSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Fi&nd what:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* m_textFindStrings = NULL;
    m_textFind = new wxOwnerDrawnComboBox;
    m_textFind->Create( itemDialog1, CD_FIND_FINDSTR, _T(""), wxDefaultPosition, wxSize(180, -1), 0, m_textFindStrings, wxCB_DROPDOWN );
    itemFlexGridSizer4->Add(m_textFind, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer7, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer7->Add(itemBoxSizer8, 1, wxALIGN_TOP|wxALL, 0);

    m_chkWord = new wxCheckBox;
    m_chkWord->Create( itemDialog1, CD_FIND_WORD, _("Match &whole word only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_chkWord->SetValue(FALSE);
    itemBoxSizer8->Add(m_chkWord, 0, wxALIGN_LEFT|wxALL, 5);

    m_chkCase = new wxCheckBox;
    m_chkCase->Create( itemDialog1, CD_FIND_CASE, _("Match &case"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_chkCase->SetValue(FALSE);
    itemBoxSizer8->Add(m_chkCase, 0, wxALIGN_LEFT|wxALL, 5);

    m_chkBack = new wxCheckBox;
    m_chkBack->Create( itemDialog1, CD_FIND_BACK, _("Search back"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_chkBack->SetValue(FALSE);
    itemBoxSizer8->Add(m_chkBack, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer12, 0, wxALIGN_TOP|wxALL, 0);

    m_button_find = new wxButton;
    m_button_find->Create( itemDialog1, CD_FIND_FIND, _("&Find next"), wxDefaultPosition, wxDefaultSize, 0 );
    m_button_find->SetDefault();
    itemBoxSizer12->Add(m_button_find, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* itemButton14 = new wxButton;
    itemButton14->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemButton14, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

////@end FindDlg content construction
    find_history.fill(m_textFind);
    m_textFind->SetValue(ini_str);
    if (!(last_search_flags & wxFR_DOWN))   m_chkBack->SetValue(true);
    if (last_search_flags & wxFR_MATCHCASE) m_chkCase->SetValue(true);
    if (last_search_flags & wxFR_WHOLEWORD) m_chkWord->SetValue(true);
}

/*!
 * Should we show tooltips?
 */

bool FindDlg::ShowToolTips()
{
    return TRUE;
}

void FindDlg::SendEvent(const wxEventType& evtType)
{
    wxFindDialogEvent event(evtType, GetId());
    event.SetEventObject(this);
    event.SetFindString(m_textFind->GetValue());
    int flags = 0;
    if ( m_chkCase->GetValue() )
        flags |= wxFR_MATCHCASE;
    if ( m_chkWord->GetValue() )
        flags |= wxFR_WHOLEWORD;
    if (!m_chkBack->GetValue() )
        flags |= wxFR_DOWN;
    event.SetFlags(flags);
   // must not send to the parent because parent of a modeless dialog must be the main frame!
    ed->GetEventHandler()->ProcessEvent(event);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REPL_FIND
 */

void FindDlg::OnCdFindFindClick( wxCommandEvent& event )
{ wxString str = m_textFind->GetValue();
  find_history.add_to_history(str);
  find_history.fill(m_textFind);  // update the history now, because the dialog is not closed by the action
  m_textFind->SetValue(str);  // return the string cleared by fill_history();
  SendEvent(wxEVT_COMMAND_FIND_NEXT); 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void FindDlg::OnCancelClick( wxCommandEvent& event )
{ SendEvent(wxEVT_COMMAND_FIND_CLOSE); 
  Destroy();
}

void FindDlg::OnClose( wxCloseEvent& event )
{ SendEvent(wxEVT_COMMAND_FIND_CLOSE); 
  Destroy();
}


/*!
 * Get bitmap resources
 */

wxBitmap FindDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin FindDlg bitmap retrieval
    return wxNullBitmap;
////@end FindDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon FindDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin FindDlg icon retrieval
    return wxNullIcon;
////@end FindDlg icon retrieval
}
