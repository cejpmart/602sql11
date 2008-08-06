/////////////////////////////////////////////////////////////////////////////
// Name:        qdprops.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/17/04 13:08:16
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "qdprops.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop

#include "tlview.h"
////@begin includes
////@end includes

#include "qdprops.h"

////@begin XPM images
////@end XPM images

/*!
 * CQDPropsDlg type definition
 */

IMPLEMENT_CLASS( CQDPropsDlg, wxDialog )

/*!
 * CQDPropsDlg event table definition
 */

BEGIN_EVENT_TABLE( CQDPropsDlg, wxDialog )

////@begin CQDPropsDlg event table entries
    EVT_NOTEBOOK_PAGE_CHANGING(ID_NOTEBOOK, CQDPropsDlg::OnPageChanging)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, CQDPropsDlg::OnPageChanged)
    EVT_RADIOBUTTON( ID_RADIOBUTTON0, CQDPropsDlg::OnRadioSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON1, CQDPropsDlg::OnRadioSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON2, CQDPropsDlg::OnRadioSelected )

    EVT_CHECKBOX( ID_CHECKBOX1, CQDPropsDlg::OnWrapItemsClick )
    EVT_CHECKBOX( ID_CHECKBOX4, CQDPropsDlg::OnWrapAfterClick )

    EVT_TEXT( ID_SPINCTRL1, CQDPropsDlg::OnIndentChanged )

    EVT_TEXT( ID_SPINCTRL2, CQDPropsDlg::OnRowLengthChanged )

    EVT_CHECKBOX( ID_CHECKBOX, CQDPropsDlg::OnConvertClick )

    EVT_CHECKBOX( ID_CHECKBOX2, CQDPropsDlg::OnPrefixColumnsClick )

    EVT_CHECKBOX( ID_CHECKBOX3, CQDPropsDlg::OnPrefixTablesClick )

    EVT_BUTTON( wxID_OK, CQDPropsDlg::OnOkClick )

////@end CQDPropsDlg event table entries

END_EVENT_TABLE()

/*!
 * CQDPropsDlg constructors
 */

CQDPropsDlg::CQDPropsDlg( )
{
}

CQDPropsDlg::CQDPropsDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CQDPropsDlg creator
 */

const char qdSection[] = "QueryDesigner";

bool CQDPropsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CQDPropsDlg member initialisation
    m_InPageChange = false;
    m_DontWrapRB = NULL;
    m_WrapAllwaysRB = NULL;
    m_WrapLongRB = NULL;
    m_WrapItemsCB = NULL;
    m_WrapAfterClauseCB = NULL;
    m_BlockIndentUD = NULL;
    m_RowLengthUD = NULL;
    m_Sample1 = NULL;
    m_ConvertJoinCB = NULL;
    m_PrefixColumnsCB = NULL;
    m_PrefixTablesCB = NULL;
    m_Sample2 = NULL;
////@end CQDPropsDlg member initialisation

////@begin CQDPropsDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style | wxRESIZE_BORDER);

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    UpdateSize(m_Sample1);
    UpdateSize(m_Sample2);
#ifdef WINS
    wxFont Font = m_Sample1->GetFont();
    HWND hEdit  = (HWND)::SendMessage((HWND)m_BlockIndentUD->GetHandle(), WM_USER+106, 0, 0);
    ::SendMessage(hEdit, WM_SETFONT, (WPARAM)Font.GetResourceHandle(), MAKELPARAM(TRUE, 0));
    hEdit  = (HWND)::SendMessage((HWND)m_RowLengthUD->GetHandle(), WM_USER+106, 0, 0);
    ::SendMessage(hEdit, WM_SETFONT, (WPARAM)Font.GetResourceHandle(), MAKELPARAM(TRUE, 0));
#endif
    m_Sample1->SetFont(*text_editor_font);
    m_Sample2->SetFont(*text_editor_font);
////@end CQDPropsDlg creation

    m_Wrap            = read_profile_int(qdSection,  "WrapStatements",      2);
    m_Indent          = read_profile_int(qdSection,  "BlockIndent",         4);
    m_RowLength       = read_profile_int(qdSection,  "LineLength",        100);
    m_WrapItems       = read_profile_bool(qdSection, "WrapItems",       false);
    m_WrapAfterClause = read_profile_bool(qdSection, "WrapAfterClause", false);
    m_ConvertJoin     = read_profile_bool(qdSection, "ConvertJoin",     false);
    m_PrefixColumns   = read_profile_bool(qdSection, "PrefixColumns",   false);
    m_PrefixTables    = read_profile_bool(qdSection, "PrefixTables",    false);

    m_DontWrapRB->SetValue(m_Wrap == 0);
    m_WrapAllwaysRB->SetValue(m_Wrap == 1);
    m_WrapLongRB->SetValue(m_Wrap == 2);
    m_BlockIndentUD->SetValue(m_Indent);
    m_RowLengthUD->SetValue(m_RowLength);
    m_WrapItemsCB->SetValue(m_WrapItems);
    m_WrapAfterClauseCB->SetValue(m_WrapAfterClause);
    m_ConvertJoinCB->SetValue(m_ConvertJoin);
    m_PrefixColumnsCB->SetValue(m_PrefixColumns);
    m_PrefixTablesCB->SetValue(m_PrefixTables); 

    m_WrapItemsCB->Enable(m_Wrap != 0);
    m_WrapAfterClauseCB->Enable(m_Wrap != 0 && !m_WrapItems);
    m_BlockIndentUD->Enable(m_Wrap != 0 && m_WrapAfterClause);
    m_RowLengthUD->Enable(m_Wrap != 0);

#ifdef LINUX    
    ShowSample1();
#endif    
    ShowSample2();
    return TRUE;
}

/*!
 * Control creation for CQDPropsDlg
 */

void CQDPropsDlg::CreateControls()
{    
////@begin CQDPropsDlg content construction

    CQDPropsDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxNotebook* item3 = new wxNotebook( item1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize /*wxSize(200, 200)*/, wxNB_TOP );
    wxPanel* item4 = new wxPanel( item3, ID_PANEL, wxDefaultPosition, wxDefaultSize /*wxSize(100, 80) */, wxTAB_TRAVERSAL );
    wxBoxSizer* item5 = new wxBoxSizer(wxVERTICAL);
    item4->SetSizer(item5);
    item4->SetAutoLayout(TRUE);
    wxBoxSizer* item6 = new wxBoxSizer(wxHORIZONTAL);
    //wxFlexGridSizer* item6 = new wxFlexGridSizer(1, 2, 0, 0);
    item5->Add(item6, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 4);
    wxStaticBox* item7Static = new wxStaticBox(item4, wxID_ANY, _T(""));
    wxStaticBoxSizer* item7 = new wxStaticBoxSizer(item7Static, wxVERTICAL);
    item6->Add(item7, 1, wxGROW|wxALL, 4);
    wxRadioButton* item8 = new wxRadioButton( item4, ID_RADIOBUTTON0, _("Do &not wrap statemnt"), wxDefaultPosition, wxDefaultSize, 0 );
    m_DontWrapRB = item8;
    item8->SetValue(FALSE);
    item7->Add(item8, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 8);
    wxRadioButton* item9 = new wxRadioButton( item4, ID_RADIOBUTTON1, _("Wrap st&atement"), wxDefaultPosition, wxDefaultSize, 0 );
    m_WrapAllwaysRB = item9;
    item9->SetValue(FALSE);
    item7->Add(item9, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 8);
    wxRadioButton* item10 = new wxRadioButton( item4, ID_RADIOBUTTON2, _("&Wrap long statement"), wxDefaultPosition, wxDefaultSize, 0 );
    m_WrapLongRB = item10;
    item10->SetValue(FALSE);
    item7->Add(item10, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 8);
    wxStaticBox* item11Static = new wxStaticBox(item4, wxID_ANY, _T(""));
    wxStaticBoxSizer* item11 = new wxStaticBoxSizer(item11Static, wxHORIZONTAL);
    wxFlexGridSizer* item12 = new wxFlexGridSizer(4, 2, 0, 0);
    item11->Add(item12, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    item6->Add(item11, 1, wxGROW|wxALL, 4);
    wxStaticText* item13 = new wxStaticText( item4, wxID_STATIC, _("&Every item on new row:"), wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add(item13, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);
    wxCheckBox* item14 = new wxCheckBox( item4, ID_CHECKBOX1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    m_WrapItemsCB = item14;
    item14->SetValue(FALSE);
    item12->Add(item14, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 4);
    wxStaticText* item33 = new wxStaticText( item4, wxID_STATIC, _("Wrap after &clause:"), wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add(item33, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);
    m_WrapAfterClauseCB = new wxCheckBox( item4, ID_CHECKBOX4, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add(m_WrapAfterClauseCB, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 4);
    wxStaticText* item15 = new wxStaticText( item4, wxID_STATIC, _("&Block indent:"), wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add(item15, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);
    wxSpinCtrl* item16 = new wxSpinCtrl( item4, ID_SPINCTRL1, _T(""), wxDefaultPosition, wxSize(64, -1), wxSP_ARROW_KEYS, 1, 16, 0 );
    m_BlockIndentUD = item16;
    item12->Add(item16, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);
    wxStaticText* item17 = new wxStaticText( item4, wxID_STATIC, _("&Row length:"), wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add(item17, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 3);
    wxSpinCtrl* item18 = new wxSpinCtrl( item4, ID_SPINCTRL2, _T(""), wxDefaultPosition, wxSize(64, -1), wxSP_ARROW_KEYS, 20, 600, 0 );
    m_RowLengthUD = item18;
    item12->Add(item18, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 3);
    wxStaticBox* item19Static = new wxStaticBox(item4, wxID_ANY, _("Sample "));
    wxStaticBoxSizer* item19 = new wxStaticBoxSizer(item19Static, wxVERTICAL);
    item5->Add(item19, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 8);
    wxTextCtrl* item20 = new wxTextCtrl( item4, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxSize(-1, 128), wxTE_MULTILINE|wxTE_READONLY|wxTE_LEFT|wxHSCROLL );
    m_Sample1 = item20;
    item19->Add(item20, 1, wxGROW|wxALL, 6);
    item3->AddPage(item4, _("&Statement format"));
    wxPanel* item21 = new wxPanel( item3, ID_PANEL1, wxDefaultPosition, wxSize(100, 80), wxTAB_TRAVERSAL );
    wxBoxSizer* item22 = new wxBoxSizer(wxVERTICAL);
    item21->SetSizer(item22);
    item21->SetAutoLayout(TRUE);
    wxFlexGridSizer* item23 = new wxFlexGridSizer(2, 1, 0, 0);
    item23->AddGrowableRow(1);
    item23->AddGrowableCol(0);
    item23->AddGrowableCol(1);
    item21->SetSizer(item23);
    item21->SetAutoLayout(TRUE);
    wxStaticBox* item24Static = new wxStaticBox(item21, wxID_ANY, _T(""));
    wxStaticBoxSizer* item24 = new wxStaticBoxSizer(item24Static, wxVERTICAL);
    item23->Add(item24, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 8);
    wxCheckBox* item25 = new wxCheckBox( item21, ID_CHECKBOX, _("&Convert INNER JOIN to WHERE contition"), wxDefaultPosition, wxDefaultSize, 0 );
    m_ConvertJoinCB = item25;
    item25->SetValue(FALSE);
    item24->Add(item25, 0, wxALIGN_LEFT|wxALL, 8);
    wxCheckBox* item26 = new wxCheckBox( item21, ID_CHECKBOX2, _("Prefix &column names with table name"), wxDefaultPosition, wxDefaultSize, 0 );
    m_PrefixColumnsCB = item26;
    item26->SetValue(FALSE);
    item24->Add(item26, 0, wxALIGN_LEFT|wxALL, 8);
    wxCheckBox* item27 = new wxCheckBox( item21, ID_CHECKBOX3, _("Prefix &table names with the schema name"), wxDefaultPosition, wxDefaultSize, 0 );
    m_PrefixTablesCB = item27;
    item27->SetValue(FALSE);
    item24->Add(item27, 0, wxALIGN_LEFT|wxALL, 8);
    wxStaticBox* item28Static = new wxStaticBox(item21, wxID_ANY, _("Sample "));
    wxStaticBoxSizer* item28 = new wxStaticBoxSizer(item28Static, wxVERTICAL);
    item23->Add(item28, 0, wxGROW|wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 8);
    wxTextCtrl* item29 = new wxTextCtrl( item21, ID_TEXTCTRL1, _T(""), wxDefaultPosition, wxSize(-1, 128), wxTE_MULTILINE|wxTE_READONLY|wxTE_LEFT );
    m_Sample2 = item29;
    item28->Add(item29, 1, wxGROW|wxALL, 6);
    item3->AddPage(item21, _("&Options"));
    item2->Add(item3, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 4);

    wxBoxSizer* item30 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item30, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 8);

    wxButton* item31 = new wxButton( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item31->SetDefault();
    item30->Add(item31, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 8);

    wxButton* item32 = new wxButton( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item30->Add(item32, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 8);

////@end CQDPropsDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool CQDPropsDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL1
 */

void CQDPropsDlg::OnIndentChanged( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
    m_Indent = m_BlockIndentUD->GetValue();
    ShowSample1();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL2
 */

void CQDPropsDlg::OnRowLengthChanged( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
    m_RowLength = m_RowLengthUD->GetValue();
    ShowSample1();
}

void CQDPropsDlg::OnPageChanging(wxNotebookEvent &event)
{
    m_InPageChange = true;
    event.Skip();
}

void CQDPropsDlg::OnPageChanged(wxNotebookEvent &event)
{
    m_InPageChange = false;
    if (event.GetSelection() == 0)
    {
        m_DontWrapRB->SetValue(m_Wrap == 0);
        m_WrapAllwaysRB->SetValue(m_Wrap == 1);
        m_WrapLongRB->SetValue(m_Wrap == 2);
    }
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON0
 */

void CQDPropsDlg::OnRadioSelected( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
    if (m_InPageChange)
        return;
    m_Wrap = event.GetId();
    m_WrapItemsCB->Enable(m_Wrap != 0);
    m_WrapAfterClauseCB->Enable(m_Wrap != 0 && !m_WrapItems);
    m_BlockIndentUD->Enable(m_Wrap != 0 && m_WrapAfterClause);
    m_RowLengthUD->Enable(m_Wrap != 0);
    ShowSample1();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX1
 */

void CQDPropsDlg::OnWrapItemsClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
    m_WrapItems = m_WrapItemsCB->IsChecked();
    m_WrapAfterClauseCB->Enable(m_Wrap != 0 && !m_WrapItems);
    ShowSample1();
}


void CQDPropsDlg::OnWrapAfterClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
    m_WrapAfterClause = m_WrapAfterClauseCB->IsChecked();
    m_BlockIndentUD->Enable(m_Wrap != 0 && m_WrapAfterClause);
    ShowSample1();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX
 */

void CQDPropsDlg::OnConvertClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
    m_ConvertJoin = m_ConvertJoinCB->IsChecked();
    ShowSample2();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX2
 */

void CQDPropsDlg::OnPrefixColumnsClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
    m_PrefixColumns = m_PrefixColumnsCB->IsChecked();
    ShowSample2();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX3
 */

void CQDPropsDlg::OnPrefixTablesClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
    m_PrefixTables = m_PrefixTablesCB->IsChecked();
    ShowSample2();
}


static const wxChar *Sample1 = wxT("SELECT Name, SurName, Phone, E_mail, (SELECT CompName FROM Companies WHERE ID = CompID) FROM Directory");
static const wxChar *Sample2 = wxT("Name, SurName, Phone, E_mail, ");
static const wxChar *Sample3 = wxT("Name, SurName, Phone, ");
static const wxChar *Sample4 = wxT("Name, SurName, ");
static const wxChar *Sample5 = wxT("(SELECT CompName FROM Companies WHERE ID = CompID)");
static const wxChar *Sample6 = wxT("SELECT   Name, SurName, Phone, E_mail,");
static const wxChar *Sample7 = wxT("SELECT   Name, SurName, Phone,");
static const wxChar *Sample8 = wxT("SELECT   Name, SurName,");
static const wxChar *Sample9 = wxT("SELECT   Name,");

void CQDPropsDlg::ShowSample1()
{
    switch (m_Wrap)
    {
    case 0:
        m_Sample = Sample1;
        break;
    case 1:
        m_Sample.Empty();
        if (m_WrapItems)
        {
            PutLine(_T("SELECT"));
            PutLine(wxT("Name,"),       1);
            PutLine(wxT("SurName,"),    1);
            PutLine(wxT("Phone,"),      1);
            PutLine(wxT("E_mail,"),     1);
            PutLine(_T("("),          1);
            PutLine(_T("SELECT"),     2);
            PutLine(wxT("CompName"),    3);
            PutLine(_T("FROM"),       2);
            PutLine(wxT("Companies"),   3);
            PutLine(_T("WHERE"),      2);
            PutLine(wxT("ID = CompID"), 3);
            PutLine(_T(")"),          1);
            PutLine(_T("FROM"));
            PutLine(wxT("Directory"),   1);
        }
        else if (m_WrapAfterClause)
        {
            PutLine(_T("SELECT"));
            PutLine(wxT("Name, SurName, Phone, E_mail,"), 1);
            PutLine(_T("("),          1);
            PutLine(_T("SELECT"),     2);
            PutLine(wxT("CompName"),    3);
            PutLine(_T("FROM"),       2);
            PutLine(wxT("Companies"),   3);
            PutLine(_T("WHERE"),      2);
            PutLine(wxT("ID = CompID"), 3);
            PutLine(_T(")"),          1);
            PutLine(_T("FROM"));
            PutLine(wxT("Directory"),   1);
        }
        else
        {
            PutLine(_T("SELECT   Name, SurName, Phone, E_mail,"));
            PutLine(_T("("),                   1);
            PutLine(wxT("SELECT   CompName"),    2);
            PutLine(wxT("FROM     Companies"),   2);
            PutLine(wxT("WHERE    ID = CompID"), 2);
            PutLine(_T(")"),    1);
            PutLine(wxT("FROM     Directory"));
        }
        break;
    case 2:
        m_Sample.Empty();
        if (m_RowLength >= wxStrlen(Sample1))
            m_Sample = Sample1;
        else if (m_WrapItems)
        {
            PutLine(_T("SELECT"));
            PutLine(wxT("Name,"),       1);
            PutLine(wxT("SurName,"),    1);
            PutLine(wxT("Phone,"),      1);
            PutLine(wxT("E_mail,"),     1);
            PutLine(_T("("),          1);
            PutLine(_T("SELECT"),     2);
            PutLine(wxT("CompName"),    3);
            PutLine(_T("FROM"),       2);
            PutLine(wxT("Companies"),   3);
            PutLine(_T("WHERE"),      2);
            PutLine(wxT("ID = CompID"), 3);
            PutLine(_T(")"),          1);
            PutLine(_T("FROM"));
            PutLine(wxT("Directory"),   2);
        }
        else if (m_WrapAfterClause)
        {
            PutLine(_T("SELECT"));
            if (m_RowLength >= m_Indent + wxStrlen(Sample2))
                PutLine(Sample2, 1);
            else if (m_RowLength >= m_Indent + wxStrlen(Sample3))
            {
                PutLine(Sample3,          1);
                PutLine(wxT("E_mail, "),    1);
            }
            else if (m_RowLength >= m_Indent + wxStrlen(Sample4))
            {
                PutLine(Sample4,              1);
                PutLine(wxT("Phone, E_mail, "), 1);
            }
            else
            {
                PutLine(wxT("Name"),        1);
                PutLine(wxT("SurName,"),    1);
                PutLine(wxT("Phone,"),      1);
                PutLine(wxT("E_mail, "),    1);
            }
            if (m_RowLength >= m_Indent + wxStrlen(Sample5))
                PutLine(Sample5,          1);
            else 
            {
                PutLine(_T("("),          1);
                PutLine(_T("SELECT"),     2);
                PutLine(wxT("CompName"),    3);
                PutLine(_T("FROM"),       2);
                PutLine(wxT("Companies"),   3);
                PutLine(_T("WHERE"),      2);
                PutLine(wxT("ID = CompID"), 3);
                PutLine(_T(")"),          1);
            }
            PutLine(_T("FROM"));
            PutLine(wxT("Directory"), 1);
        }
        else
        {
            if (m_RowLength >= wxStrlen(Sample6))
                PutLine(Sample6);
            else if (m_RowLength >= wxStrlen(Sample7))
            {
                PutLine(Sample7);
                PutLine(wxT("E_mail,"), 1);
            }
            else if (m_RowLength >= wxStrlen(Sample8))
            {
                PutLine(Sample8);
                PutLine(wxT("Phone, E_mail,"), 1);
            }
            else
            {
                PutLine(Sample9);
                PutLine(wxT("SurName, Phone, E_mail,"), 1);
            }
            if (m_RowLength >= m_Indent + wxStrlen(Sample5))
                PutLine(Sample5, 1);
            else
            {
                PutLine(_T("("),                   1);
                PutLine(wxT("SELECT   CompName"),    2);
                PutLine(wxT("FROM     Companies"),   2);
                PutLine(wxT("WHERE    ID = CompID"), 2);
                PutLine(_T(")"),                   1);
            }
            PutLine(wxT("FROM     Directory"));
        }
    }
    m_Sample1->SetValue(m_Sample);
}

void CQDPropsDlg::PutLine(const wxChar *Val, int Indent)
{

    if (Indent > 0)
    {
        Indent *= m_WrapAfterClause ? m_Indent : 9;
        m_Sample += wxString(_T(' '), Indent);
    }
    m_Sample += Val;
    m_Sample += _T("\r\n");
}


void CQDPropsDlg::ShowSample2()
{
    const wxChar *Col1, *Col2, *Tbl1, *Tbl2;

    if (m_PrefixColumns)
    {
        Col1 = wxT("Persons.Name");
        Col2 = wxT("Companies.Address");
    }
    else
    {
        Col1 = wxT("Name");
        Col2 = wxT("Address");
    }
    if (m_PrefixTables)
    {
        Tbl1 = wxT("Directory.Persons");
        Tbl2 = wxT("Accounting.Companies");
    }
    else
    {
        Tbl1 = wxT("Persons");
        Tbl2 = wxT("Companies");
    };
    wxString Caption;
    if (m_ConvertJoin)
        Caption = wxString(wxT("SELECT\r\n    ")) + Col1 + wxT(", ") + Col2 + wxT("\r\nFROM\r\n    ") + Tbl1 + wxT(", ") + Tbl2 + wxT("\r\nWHERE\r\n    ") + Col1 + wxT(" = ") + Col2;
    else
        Caption = wxString(wxT("SELECT\r\n    ")) + Col1 + wxT(", ") + Col2 + wxT("\r\nFROM\r\n    ") + Tbl1 + wxT(" JOIN ") + Tbl2 + wxT(" ON ") + Col1 + wxT(" = ") + Col2;
#ifdef WINS
//    Caption = "\n\r\n\r" + Caption;
#endif
    m_Sample2->SetValue(Caption);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void CQDPropsDlg::OnOkClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

    write_profile_int(qdSection,  "WrapStatements",  m_Wrap);
    write_profile_int(qdSection,  "BlockIndent",     m_Indent);
    write_profile_int(qdSection,  "LineLength",      m_RowLength);
    write_profile_bool(qdSection, "WrapItems",       m_WrapItems);
    write_profile_bool(qdSection, "WrapAfterClause", m_WrapAfterClause);
    write_profile_bool(qdSection, "ConvertJoin",     m_ConvertJoin);
    write_profile_bool(qdSection, "PrefixColumns",   m_PrefixColumns);
    write_profile_bool(qdSection, "PrefixTables",    m_PrefixTables);
}

