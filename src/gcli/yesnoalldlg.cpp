/////////////////////////////////////////////////////////////////////////////
// Name:        yesnoalldlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/09/04 13:20:33
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
////@begin includes
#include "wx/wx.h"
////@end includes
#endif
#ifndef WINS
#include "winrepl.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop

#ifdef __GNUG__
//#pragma implementation "yesnoalldlg.h"
#endif

#include "yesnoalldlg.h"
#include "bmps/question.xpm"

////@begin XPM images
////@end XPM images

/*!
 * CYesNoAllDlg type definition
 */

IMPLEMENT_CLASS( CYesNoAllDlg, wxDialog )

/*!
 * CYesNoAllDlg event table definition
 */

BEGIN_EVENT_TABLE( CYesNoAllDlg, wxDialog )

////@begin CYesNoAllDlg event table entries
    EVT_BUTTON( wxID_YES, CYesNoAllDlg::OnClick )

    EVT_BUTTON( wxID_NO, CYesNoAllDlg::OnClick )

    EVT_BUTTON( wxID_YESTOALL, CYesNoAllDlg::OnClick )

////@end CYesNoAllDlg event table entries
    EVT_CLOSE(CYesNoAllDlg::OnClose) 

END_EVENT_TABLE()

/*!
 * CYesNoAllDlg constructors
 */

CYesNoAllDlg::CYesNoAllDlg( )
{
}

CYesNoAllDlg::CYesNoAllDlg(wxWindow* parent, wxString Text, wxString AllBut)
{
    m_Text   = Text;
    m_AllBut = AllBut;
    Create(parent);
}

CYesNoAllDlg::CYesNoAllDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CYesNoAllDlg creator
 */

bool CYesNoAllDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CYesNoAllDlg member initialisation
////@end CYesNoAllDlg member initialisation

////@begin CYesNoAllDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CYesNoAllDlg creation
    return TRUE;
}

/*!
 * Control creation for CYesNoAllDlg
 */

void CYesNoAllDlg::CreateControls()
{    
    //wxBitmap item4Bitmap(question_xpm);

////@begin CYesNoAllDlg content construction

    CYesNoAllDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxFlexGridSizer* item3 = new wxFlexGridSizer(1, 2, 0, 0);
    item3->AddGrowableCol(1);
    item2->Add(item3, 0, wxGROW|wxALL, 10);

    wxBitmap item4Bitmap(question_xpm);
    wxStaticBitmap* item4 = new wxStaticBitmap( item1, wxID_STATIC, item4Bitmap, wxDefaultPosition, wxSize(32, 32), 0 );
    item3->Add(item4, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* item5 = new wxStaticText( item1, wxID_STATIC, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    m_Label = item5;
    item3->Add(item5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* item6 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item6, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* item7 = new wxButton( item1, wxID_YES, _("&Yes"), wxDefaultPosition, wxDefaultSize, 0 );
    item6->Add(item7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item8 = new wxButton( item1, wxID_NO, _("&No"), wxDefaultPosition, wxDefaultSize, 0 );
    item8->SetDefault();
    item6->Add(item8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item9 = new wxButton( item1, wxID_YESTOALL, _("Yes to &All"), wxDefaultPosition, wxDefaultSize, 0 );
    item6->Add(item9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end CYesNoAllDlg content construction

    m_Label->SetLabel(m_Text);
    if (!m_AllBut.IsEmpty())
        item9->SetLabel(m_AllBut);
}

/*!
 * Should we show tooltips?
 */

bool CYesNoAllDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_YES
 */

void CYesNoAllDlg::OnClick( wxCommandEvent& event )
{
    // Insert custom code here
    EndModal(event.GetId());
    event.Skip();
}

void CYesNoAllDlg::OnClose(wxCloseEvent & event)
{
    event.Veto();
}


