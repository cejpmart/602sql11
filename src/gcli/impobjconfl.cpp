/////////////////////////////////////////////////////////////////////////////
// Name:        impobjconfl.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/04/04 15:15:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////
#include "wx/wxprec.h"
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
#include "querydef.h"
#pragma hdrstop
#include "impobjconfl.h"

#ifdef __GNUG__
//#pragma implementation "impobjconfl.h"
#endif

////@begin XPM images
////@end XPM images

/*!
 * CImpObjConflDlg type definition
 */

IMPLEMENT_CLASS( CImpObjConflDlg, wxDialog )

/*!
 * CImpObjConflDlg event table definition
 */

BEGIN_EVENT_TABLE( CImpObjConflDlg, wxDialog )

////@begin CImpObjConflDlg event table entries
    EVT_RADIOBOX( ID_RADIOBOX, CImpObjConflDlg::OnRadioboxSelected )

    EVT_BUTTON( ID_Close, CImpObjConflDlg::OnCloseClick )

////@end CImpObjConflDlg event table entries
    EVT_CLOSE(CImpObjConflDlg::OnClose) 
END_EVENT_TABLE()

/*!
 * CImpObjConflDlg constructors
 */

CImpObjConflDlg::CImpObjConflDlg( )
{
}

CImpObjConflDlg::CImpObjConflDlg(cdp_t cdp, const wxString &Prompt, char *ObjName, tcateg Categ, bool Single, bool Application)
{
    m_cdp         = cdp;
    m_Prompt      = Prompt;
    m_ObjName     = ObjName;
    m_Categ       = Categ;
    m_Single      = Single;
    m_Application = Application;
    m_Value       = -1;
    Create(NULL);
}

CImpObjConflDlg::CImpObjConflDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CImpObjConflDlg creator
 */

bool CImpObjConflDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CImpObjConflDlg member initialisation
    m_Text = NULL;
    m_RadioBox = NULL;
    m_Label = NULL;
    m_Edit = NULL;
////@end CImpObjConflDlg member initialisation

////@begin CImpObjConflDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CImpObjConflDlg creation
    return TRUE;
}

/*!
 * Control creation for CImpObjConflDlg
 */

void CImpObjConflDlg::CreateControls()
{    

    //m_RadioBox = CreateRadioBox(itemDialog1);

////@begin CImpObjConflDlg content construction

    CImpObjConflDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    m_Text = new wxStaticText( itemDialog1, wxID_STATIC, _("Static text"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemBoxSizer2->Add(m_Text, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 15);

    wxString* m_RadioBoxStrings = NULL;
    m_RadioBox = CreateRadioBox(itemDialog1);
    itemBoxSizer2->Add(m_RadioBox, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxFlexGridSizer* itemFlexGridSizer5 = new wxFlexGridSizer(1, 2, 0, 0);
    itemFlexGridSizer5->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer5, 0, wxGROW|wxALL, 5);

    m_Label = new wxStaticText( itemDialog1, wxID_STATIC, _("New Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(m_Label, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    m_Edit = new wxTextCtrl( itemDialog1, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(m_Edit, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton8 = new wxButton( itemDialog1, ID_Close, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton8->SetDefault();
    itemBoxSizer2->Add(itemButton8, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 10);

////@end CImpObjConflDlg content construction

    m_Text->SetLabel(m_Prompt);
    tobjname ObjName;
    strcpy(ObjName, m_ObjName);
    GetFreeObjName(m_cdp, ObjName, m_Categ);
    m_Edit->SetValue(wxString(ObjName, *m_cdp->sys_conv));
    m_Edit->Enable(false);
    m_Edit->SetMaxLength(OBJ_NAME_LEN);
    m_Label->Enable(false);
}

wxRadioBox* CImpObjConflDlg::CreateRadioBox(wxWindow* Parent)
{
    wxString SingleStrings[] = 
    {
        _("&Replace existing object"),
        _("Create &new instance with another name"),
        _("&Cancel import")
    };
    wxString MultiStrings[] = 
    {
        _("&Replace existing object"),
        _("Replace &all existing object"),
        _("Create &new instance with another name"),
        _("&Skip import of this object and continue"),
        _("&Cancel import")
    };
    wxString ApplStrings[] = 
    {
        _("&Replace all objects in the current application"),
        _("Create &new application instance with another name"),
        _("&Cancel import")
    };
    wxString *itemStrings;
    int Count;
    if (m_Application)
    {
        itemStrings = ApplStrings;
        Count = 3;
    }
    else if (m_Single)
    {
        itemStrings = SingleStrings;
        Count = 3;
    }
    else
    {
        itemStrings = MultiStrings;
        Count = 5;
    }
    wxRadioBox *Item = new wxRadioBox(Parent, ID_RADIOBOX, _(" Do you want to "), wxDefaultPosition, wxDefaultSize, Count, itemStrings, 1, wxRA_SPECIFY_COLS );
    Item->SetSelection(Count - 1);
    return(Item);
}
/*!
 * Should we show tooltips?
 */

bool CImpObjConflDlg::ShowToolTips()
{
    return TRUE;
}

void CImpObjConflDlg::OnClose(wxCloseEvent & event)
{
    if (m_Value == -1)
        event.Veto();
}

int CImpObjConflDlg::GetSelValue()
{
    tiocresult SingleRes[] = {iocYes, iocNewName, iocCancel};
    tiocresult MultiRes[]  = {iocYes, iocYesToAll, iocNewName, iocNo, iocCancel};
    int Val = m_RadioBox->GetSelection();
    return(m_Single ? SingleRes[Val] : MultiRes[Val]);
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void CImpObjConflDlg::OnCloseClick( wxCommandEvent& event )
{
    // Insert custom code here
    int Val = GetSelValue();
    if (Val == iocNewName)
    {  
        wxString NewName = m_Edit->GetValue().Trim();
        if (ObjNameExists(m_cdp, NewName.mb_str(*m_cdp->sys_conv), m_Categ))
            return;
        strcpy(m_ObjName, NewName.mb_str(*m_cdp->sys_conv));
    }
    m_Value = Val;
    EndModal(m_Value);
    event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX
 */

void CImpObjConflDlg::OnRadioboxSelected( wxCommandEvent& event )
{
    // Insert custom code here
    bool Enb = GetSelValue() == iocNewName;
    m_Label->Enable(Enb);
    m_Edit->Enable(Enb);
    event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap CImpObjConflDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CImpObjConflDlg bitmap retrieval
    return wxNullBitmap;
////@end CImpObjConflDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CImpObjConflDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CImpObjConflDlg icon retrieval
    return wxNullIcon;
////@end CImpObjConflDlg icon retrieval
}
