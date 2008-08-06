/////////////////////////////////////////////////////////////////////////////
// Name:        expschemadlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/29/04 15:55:18
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

#include "xrc/DateSelectorDlg.h"
#include "expschemadlg.h"


////@begin XPM images
////@end XPM images

/*!
 * CExpSchemaDlg type definition
 */

IMPLEMENT_CLASS( CExpSchemaDlg, wxDialog )

/*!
 * CExpSchemaDlg event table definition
 */

BEGIN_EVENT_TABLE( CExpSchemaDlg, wxDialog )

////@begin CExpSchemaDlg event table entries
    EVT_RADIOBUTTON( ID_COMPLET, CExpSchemaDlg::OnRadioSelected )
    EVT_RADIOBUTTON( ID_CHANGEDSINCE, CExpSchemaDlg::OnRadioSelected )
    EVT_TEXT(ID_FOLDERED, CExpSchemaDlg::OnFolderChanged) 
    EVT_BUTTON( ID_SELECT, CExpSchemaDlg::OnSelectFolder )

    EVT_BUTTON( ID_BITMAPBUTTON, CExpSchemaDlg::OnDateTimeClick )

    EVT_BUTTON( wxID_OK, CExpSchemaDlg::OnOkClick )
    EVT_BUTTON( wxID_HELP, CExpSchemaDlg::OnHelp )

////@end CExpSchemaDlg event table entries

END_EVENT_TABLE()

/*!
 * CExpSchemaDlg constructors
 */

CExpSchemaDlg::CExpSchemaDlg( )
{
}

CExpSchemaDlg::CExpSchemaDlg(wxString Schema, wxString ApplName, bool Locked)
{
    Create(NULL);
    m_Encrypt  = Locked;
    m_Schema   = Schema;
    m_ApplName = ApplName;
}

CExpSchemaDlg::CExpSchemaDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CExpSchemaDlg creator
 */

bool CExpSchemaDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CExpSchemaDlg member initialisation
    m_Data = false;
    m_RolePrivils = false;
    m_UserPrivils = false;
    m_Encrypt = false;
    m_ChangedSince = NONETIMESTAMP;
    CompletRB = NULL;
    ChangedSinceRB = NULL;
    ChangedSinceED = NULL;
    ChangedSinceBut = NULL;
    DataCHB = NULL;
    RolePrivilsCHB = NULL;
    UserPrivilsCHB = NULL;
    EncryptCHB = NULL;
    FolderED   = NULL;
    OkBut      = NULL;
////@end CExpSchemaDlg member initialisation

////@begin CExpSchemaDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CExpSchemaDlg creation
    return TRUE;
}

/*!
 * Control creation for CExpSchemaDlg
 */

void CExpSchemaDlg::CreateControls()
{   

////@begin CExpSchemaDlg content construction

    CExpSchemaDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);
    wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);
    wxGridSizer* item4 = new wxGridSizer(1, 2, 0, 0);
    item3->Add(item4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxStaticBox* item5Static = new wxStaticBox(item1, wxID_ANY, _(" Export what: "));
    wxStaticBoxSizer* item5 = new wxStaticBoxSizer(item5Static, wxVERTICAL);
    item4->Add(item5, 0, wxGROW|wxGROW|wxALL, 5);
    wxRadioButton* item6 = new wxRadioButton( item1, ID_COMPLET, _("Co&mplete application"), wxDefaultPosition, wxDefaultSize, 0 );
    CompletRB = item6;
    item6->SetValue(FALSE);
    item5->Add(item6, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 7);
    wxRadioButton* item7 = new wxRadioButton( item1, ID_CHANGEDSINCE, _("&Objects changed since:"), wxDefaultPosition, wxDefaultSize, 0 );
    ChangedSinceRB = item7;
    item7->SetValue(FALSE);
    item5->Add(item7, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 7);
    wxBoxSizer* item8 = new wxBoxSizer(wxHORIZONTAL);
    item5->Add(item8, 0, wxGROW|wxALL, 5);
    wxTextCtrl* item9 = new wxTextCtrl( item1, ID_CHANGEDSINCEED, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    ChangedSinceED = item9;
    item8->Add(item9, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5);
    wxBitmap item10Bitmap(wxNullBitmap);
    wxBitmapButton* item10 = new wxBitmapButton( item1, ID_BITMAPBUTTON, item10Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    ChangedSinceBut = item10;
    item8->Add(item10, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 5);
    wxStaticBox* item11Static = new wxStaticBox(item1, wxID_ANY, _(" Options: "));
    wxStaticBoxSizer* item11 = new wxStaticBoxSizer(item11Static, wxVERTICAL);
    item4->Add(item11, 0, wxGROW|wxGROW|wxALL, 5);
    wxCheckBox* item12 = new wxCheckBox( item1, ID_CHECKBOX, _("Include table &data"), wxDefaultPosition, wxDefaultSize, 0 );
    DataCHB = item12;
    item12->SetValue(FALSE);
    item11->Add(item12, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);
    wxCheckBox* item13 = new wxCheckBox( item1, ID_CHECKBOX1, _("Include privileges of &roles"), wxDefaultPosition, wxDefaultSize, 0 );
    RolePrivilsCHB = item13;
    item13->SetValue(m_Encrypt);
    item13->Enable(!m_Encrypt);
    item11->Add(item13, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);
    wxCheckBox* item14 = new wxCheckBox( item1, ID_CHECKBOX2, _("Include privileges of &users and groups"), wxDefaultPosition, wxDefaultSize, 0 );
    UserPrivilsCHB = item14;
    item14->SetValue(FALSE);
    item11->Add(item14, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);
    wxCheckBox* item15 = new wxCheckBox( item1, ID_CHECKBOX3, _("&Encrypt"), wxDefaultPosition, wxDefaultSize, 0 );
    EncryptCHB = item15;
    item15->SetValue(FALSE);
    item11->Add(item15, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);
    
    wxStaticBox* item20Static = new wxStaticBox(item1, wxID_ANY, _(" Target path: "));
    wxStaticBoxSizer* item20 = new wxStaticBoxSizer(item20Static, wxHORIZONTAL);
    item2->Add(item20, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

    FolderED = new wxTextCtrl( item1, ID_FOLDERED, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    item20->Add(FolderED, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    wxButton* item22 = new wxButton( item1, ID_SELECT, _("&Select"), wxDefaultPosition, wxDefaultSize, 0 );
    item20->Add(item22, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* item16 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item16, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 7);
    OkBut = new wxButton( item1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    OkBut->SetDefault();
    OkBut->Enable(false);
    item16->Add(OkBut, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10);
    wxButton* item18 = new wxButton( item1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item16->Add(item18, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10);
    wxButton* item19 = new wxButton( item1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item16->Add(item19, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10);

////@end CExpSchemaDlg content construction

    wxBitmap bmp(openselector_xpm);
    ChangedSinceBut->SetBitmapLabel(bmp);

    ChangedSinceED->Disable();
    ChangedSinceBut->Disable();
}

/*!
 * Should we show tooltips?
 */

bool CExpSchemaDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON1
 */

void CExpSchemaDlg::OnRadioSelected( wxCommandEvent& event )
{
    // Insert custom code here
    ChangedSinceED->Enable(event.GetId()  == ID_CHANGEDSINCE);
    ChangedSinceBut->Enable(event.GetId() == ID_CHANGEDSINCE);
    event.Skip();
}

void CExpSchemaDlg::OnFolderChanged( wxCommandEvent& event )
{
    wxString fName = FolderED->GetValue();
    bool Enb = false;
    if (!fName.IsEmpty())
    {
        Enb = wxDirExists(fName);
        if (!Enb)
        {
            wxFileName dName(fName);
            Enb = wxDirExists(dName.GetPath());
        }
    }
    OkBut->Enable(Enb);
}

void CExpSchemaDlg::OnSelectFolder( wxCommandEvent& event )
{
    wxString fName = GetExportFileName(m_ApplName, GetFileTypeFilter(CATEG_APPL), this);
    if (!fName.IsEmpty())
        FolderED->SetValue(fName);
}

wxString CExpSchemaDlg::GetPath()
{
    wxString fName = FolderED->GetValue();
    wxFileName dName;
    if (wxDirExists(fName))
    {
        dName.SetPath(fName);
        dName.SetFullName(m_ApplName);
    }
    else
    {
        dName.Assign(fName);
        if (dName.GetExt().IsEmpty())
            dName.SetExt(wxString(L"apl"));
    }
    return dName.GetFullPath();
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void CExpSchemaDlg::OnOkClick( wxCommandEvent& event )
{
    // Insert custom code here
    wxString fName = GetPath();
    if (wxFileExists(fName))
    {
        wxFileName dName(fName);
        if (!yesno_boxp(_("The folder %s already contains schema %s files.\n\nOverwrite?"), this, dName.GetPath(wxPATH_GET_VOLUME ).c_str(), m_Schema.c_str()))
            return;
    }
    if (ChangedSinceRB->GetValue())
    {
        if (superconv(ATT_STRING, ATT_TIMESTAMP, ChangedSinceED->GetValue().mb_str(*wxConvCurrent), (char*)&m_ChangedSince, t_specif(), t_specif(), format_timestamp.mb_str(*wxConvCurrent)) != 0)
        {
            error_box(_("Date and time value is not valid"));
            ChangedSinceED->SetFocus();
            return;
        }
    }
    else
    {
        m_ChangedSince = 0;
    }
    m_Data        = DataCHB->GetValue();
    m_RolePrivils = RolePrivilsCHB->GetValue();
    m_UserPrivils = UserPrivilsCHB->GetValue();
    m_Encrypt     = EncryptCHB->GetValue();
    event.Skip();
}

void CExpSchemaDlg::OnHelp( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/expapl.html"));
}


/*!
 * Get bitmap resources
 */

wxBitmap CExpSchemaDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CExpSchemaDlg bitmap retrieval
    return wxNullBitmap;
////@end CExpSchemaDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CExpSchemaDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CExpSchemaDlg icon retrieval
    return wxNullIcon;
////@end CExpSchemaDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON
 */

void CExpSchemaDlg::OnDateTimeClick( wxCommandEvent& event )
{
    // Insert custom code here
    uns32 ts, date, time;
    // get the specified date or today's date:
    if (superconv(ATT_STRING, ATT_TIMESTAMP, ChangedSinceED->GetValue().mb_str(*wxConvCurrent), (char *)&ts, t_specif(), t_specif(), format_timestamp.mb_str(*wxConvCurrent)) == 0 && ts != NONETIMESTAMP)
    { 
        date = timestamp2date(ts);
        time = timestamp2time(ts);
    }
    else //if (!str2date(srch_time->GetValue().mb_str(wxConvLocal), &date) || date==NONEINTEGER)
    {
        date = Today();
        time = 0;
    }
    // show the calendar:
    wxDateTime dtm(Day(date), (wxDateTime::Month)(Month(date)-1), Year(date), 0, 0, 0, 0);
    DateSelectorDlg cal(&dtm);
    int x, y, w, h;
    ChangedSinceBut->GetPosition(&x, &y); 
    wxPoint pt = ClientToScreen(wxPoint(x+20, y+20));
    cal.Create(this, -1, SYMBOL_DATESELECTORDLG_TITLE, pt, wxSize(170,180));
    cal.GetPosition(&x, &y); 
    cal.GetSize(&w, &h);
    if (x + w > wxSystemSettings::GetMetric(wxSYS_SCREEN_X))
        x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X) - w;
    if (y + h > wxSystemSettings::GetMetric(wxSYS_SCREEN_Y))
        y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) - h;
    cal.Move(x,y);
    if (cal.ShowModal() == 1)
    {
        date = Make_date(dtm.GetDay(), dtm.GetMonth() + 1, dtm.GetYear());
        char buf[50];
        uns32 ts = datetime2timestamp(date, time);
        superconv(ATT_TIMESTAMP, ATT_STRING, &ts, buf, t_specif(), t_specif(), format_timestamp.mb_str(*wxConvCurrent));
        ChangedSinceED->SetValue(wxString(buf, *wxConvCurrent));
    }
    event.Skip();
}


