/////////////////////////////////////////////////////////////////////////////
// Name:        DuplFilProg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/26/04 15:17:22
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "impexpprog.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
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
#pragma hdrstop

#include "wx/listctrl.h"

#include "objlist9.h"
#include "DuplFilProg.h"

////@begin XPM images
////@end XPM images

/*!
 * CDuplFilProgDlg type definition
 */

IMPLEMENT_CLASS( CDuplFilProgDlg, wxDialog )

/*!
 * CDuplFilProgDlg event table definition
 */

BEGIN_EVENT_TABLE( CDuplFilProgDlg, wxDialog )

////@begin CDuplFilProgDlg event table entries
////@end CDuplFilProgDlg event table entries
    EVT_SIZE(CDuplFilProgDlg::OnSize)

END_EVENT_TABLE()

/*!
 * CDuplFilProgDlg constructors
 */

CDuplFilProgDlg::CDuplFilProgDlg( )
{
}

CDuplFilProgDlg::CDuplFilProgDlg(cdp_t cdp, wxWindow* parent)
{
    m_cdp       = cdp;
    ProgressBar = NULL;
    Create(parent);
}

/*!
 * CDuplFilProgDlg creator
 */

bool CDuplFilProgDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CDuplFilProgDlg member initialisation
    CategED = NULL;
    NameED = NULL;
    List = NULL;
    ProgressBar = NULL;
    OKBut = NULL;
////@end CDuplFilProgDlg member initialisation

////@begin CDuplFilProgDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CDuplFilProgDlg creation
    return TRUE;
}

/*!
 * Control creation for CDuplFilProgDlg
 */

void CDuplFilProgDlg::CreateControls()
{    

////@begin CDuplFilProgDlg content construction

    CDuplFilProgDlg* item1 = this;

    wxFlexGridSizer* item2 = new wxFlexGridSizer(4, 1, 0, 0);
    item2->AddGrowableRow(1);
    item2->AddGrowableCol(0);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);
    wxFlexGridSizer* item3 = new wxFlexGridSizer(5, 2, 0, 0);
    item3->AddGrowableCol(1);
    item2->Add(item3, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 10);
    wxStaticText* item4 = new wxStaticText( item1, wxID_STATIC, _("Exporting:"), wxDefaultPosition, wxDefaultSize, 0 );
    CategLab = item4;
    item3->Add(item4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 4);
    wxTextCtrl* item5 = new wxTextCtrl( item1, ID_CATEGED, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    CategED = item5;
    item3->Add(item5, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5);
    wxStaticText* item6 = new wxStaticText( item1, wxID_STATIC, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 4);
    wxTextCtrl* item7 = new wxTextCtrl( item1, ID_NAMEED, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    NameED = item7;
    item3->Add(item7, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5);

    wxStaticText* item16 = new wxStaticText( item1, wxID_STATIC, _("From schema:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item16, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 4);
    wxTextCtrl* item17 = new wxTextCtrl( item1, ID_SCHEMAED, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    ApplED = item17;
    item3->Add(item17, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5);

    wxStaticText* item8 = new wxStaticText( item1, wxID_STATIC, _("File name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);
    wxListCtrl* item9 = new wxListCtrl( item1, ID_LISTCTRL, wxDefaultPosition, wxSize(100, 100), wxLC_REPORT|wxLC_NO_HEADER );
    List = item9;
    item2->Add(item9, 0, wxGROW|wxGROW|wxLEFT|wxRIGHT, 10);
    wxGauge* item10 = new wxGauge( item1, ID_GAUGE, 100, wxDefaultPosition, wxSize(-1, 22), wxGA_HORIZONTAL );
    ProgressBar = item10;
    item10->SetValue(0);
    item2->Add(item10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 10);
    wxButton* item11 = new wxButton( item1, wxID_CANCEL, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    OKBut = item11;
    item11->SetDefault();
    item2->Add(item11, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 10);
////@end CDuplFilProgDlg content construction

    CategED->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    List->InsertColumn(0, wxEmptyString);
    SetTitle(_("Regenerate database file"));
    OKBut->Enable(false);
}

/*!
 * Should we show tooltips?
 */

bool CDuplFilProgDlg::ShowToolTips()
{
    return TRUE;
}
//
// Resize event hanler, sets list column size to whole list client size
//
void CDuplFilProgDlg::OnSize(wxSizeEvent &event)
{
    int Width, Height;
    List->GetClientSize(&Width, &Height);
    List->SetColumnWidth(0, Width);
    event.Skip();
}
//
// t_export_param.callback implementation
//
void WINAPI CDuplFilProgDlg::Progress(int Categ, const void *Value, void *Param)
{
    if (Categ == IMPEXP_PROGMAX || Categ == IMPEXP_PROGSTEP || Categ == IMPEXP_PROGPOS)
        ((CDuplFilProgDlg *)Param)->Progress(Categ, (const wxChar *)Value);
    else
        ((CDuplFilProgDlg *)Param)->Progress(Categ, wxString((const char *)Value, *wxConvCurrent));
}
//
// Shows current progress
//
void CDuplFilProgDlg::Progress(int Categ, const wxChar *Value)
{
    int Width, Height;
    const wxChar *Text;

    switch (Categ)
    {
    case IMPEXP_PROGMAX:
        ProgressBar->SetRange((int)(size_t)Value);              // Reset ProgressBar
        ProgressBar->SetValue(0);
        List->GetClientSize(&Width, &Height);                   // Reset list colum width
        List->SetColumnWidth(0, Width);
        Update();
        break;
    case IMPEXP_PROGSTEP:
        ProgressBar->SetValue(ProgressBar->GetValue() + 1);     // Set next ProgressBar step
        break;
    case IMPEXP_PROGPOS:
        ProgressBar->SetValue((int)(size_t)Value);              // Set new ProgressBar value
        break;
    case IMPEXP_FILENAME:
        ShowItem(Value);                                        // Show file name
        break;
    case IMPEXP_ERROR:
        if (IsShown())                                          // IF Dialog shown, show message in list box
            ShowItem(Value);
        else                                                    // ELSE show message in MessageBox
            error_box(Value);
        break;
    case IMPEXP_STATUS:
        if (IsShown())                                          // IF Dialog shown, show operation result
        {
            ShowItem(Value);
        }
        break;
    default:
        wchar_t wc[64];                                         // Show object category
        if (Categ == IMPEXP_DATA)
            Text = _("Data");
        else if (Categ == IMPEXP_DATAPRIV)
            Text = _("Privileges");
        else if (Categ == IMPEXP_ICON)
            Text = _("Icon");
        else if (Categ == CATEG_KEY)
            Text = _("Keys");
        else
            Text = CCategList::CaptFirst(m_cdp, Categ, wc);
        CategED->SetValue(Text);
        CategED->wxWindow::Update();
        NameED->SetValue(Value);                                // Show object name
        NameED->wxWindow::Update();
    }
#ifdef WINS
    wxApp *App = &wxGetApp();                                   // Dispatch system events to redrraw current dialog content
    while (App->Pending())
        App->Dispatch();
#else
    wxSafeYield(this);            
#endif    
}
//
// Shows separate lines of Value to listbox
//
void CDuplFilProgDlg::ShowItem(const wxChar *Value)
{
    long Item = List->GetItemCount();
    const wxChar *p, *n;
    for (p = Value; ; p = n)
    {
        n = _tcschr(p, '\r');
        if (!n)
        {
            n = _tcschr(p, '\n');
            if (!n)
                break;
        }
        List->InsertItem(Item++, wxString(p, n - p));
        if (*n == '\r')
            n++;
        if (*n == '\n')
            n++;
    }
    
    List->InsertItem(Item, p);
    List->EnsureVisible(Item);
    List->wxWindow::Update();
}

/*!
 * Get bitmap resources
 */

wxBitmap CDuplFilProgDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CDuplFilProgDlg bitmap retrieval
    return wxNullBitmap;
////@end CDuplFilProgDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CDuplFilProgDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CDuplFilProgDlg icon retrieval
    return wxNullIcon;
////@end CDuplFilProgDlg icon retrieval
}
