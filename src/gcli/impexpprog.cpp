/////////////////////////////////////////////////////////////////////////////
// Name:        impexpprog.cpp
// Purpose:     Schema import and export progress dialog
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
#include "impexpprog.h"

////@begin XPM images
////@end XPM images

/*!
 * CImpExpProgDlg type definition
 */

IMPLEMENT_CLASS( CImpExpProgDlg, wxDialog )

/*!
 * CImpExpProgDlg event table definition
 */

BEGIN_EVENT_TABLE( CImpExpProgDlg, wxDialog )

////@begin CImpExpProgDlg event table entries
////@end CImpExpProgDlg event table entries
    EVT_SIZE(CImpExpProgDlg::OnSize)

END_EVENT_TABLE()

/*!
 * CImpExpProgDlg constructors
 */

CImpExpProgDlg::CImpExpProgDlg( )
{
}

CImpExpProgDlg::CImpExpProgDlg(cdp_t cdp, wxWindow* parent, wxString captionIn, bool dupldbIn)
{
    m_cdp    = cdp;
    caption = captionIn;
    dupldb = dupldbIn;
    ProgBar  = NULL;
    Create(parent);
}

/*!
 * CImpExpProgDlg creator
 */

bool CImpExpProgDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CImpExpProgDlg member initialisation
    CategED = NULL;
    NameED = NULL;
    List = NULL;
    ProgBar = NULL;
    OKBut = NULL;
////@end CImpExpProgDlg member initialisation

////@begin CImpExpProgDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CImpExpProgDlg creation
    return TRUE;
}

/*!
 * Control creation for CImpExpProgDlg
 */

void CImpExpProgDlg::CreateControls()
{    

////@begin CImpExpProgDlg content construction

    CImpExpProgDlg* item1 = this;

    wxFlexGridSizer* item2 = new wxFlexGridSizer(4, 1, 0, 0);
    item2->AddGrowableRow(1);
    item2->AddGrowableCol(0);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);
    wxFlexGridSizer* item3 = new wxFlexGridSizer(3, 2, 0, 0);
    item3->AddGrowableCol(1);
    item2->Add(item3, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 10);
    wxStaticText* item4 = new wxStaticText( item1, wxID_STATIC, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 4);
    wxTextCtrl* item5 = new wxTextCtrl( item1, ID_CATEGED, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    CategED = item5;
    item3->Add(item5, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5);
    wxStaticText* item6 = new wxStaticText( item1, wxID_STATIC, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 4);
    wxTextCtrl* item7 = new wxTextCtrl( item1, ID_NAMEED, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    NameED = item7;
    item3->Add(item7, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5);
    wxStaticText* item8 = new wxStaticText( item1, wxID_STATIC, _("File name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);
    wxListCtrl* item9 = new wxListCtrl( item1, ID_LISTCTRL, wxDefaultPosition, wxSize(100, 100), wxLC_REPORT|wxLC_NO_HEADER );
    List = item9;
    item2->Add(item9, 1, wxGROW|wxLEFT|wxRIGHT, 10);
    wxGauge* item10 = new wxGauge( item1, ID_GAUGE, 100, wxDefaultPosition, wxSize(-1, 22), wxGA_HORIZONTAL );
    ProgBar = item10;
    item10->SetValue(0);
    item2->Add(item10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 10);
    wxButton* item11 = new wxButton( item1, wxID_CANCEL, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    OKBut = item11;
    item11->SetDefault();
    item2->Add(item11, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 10);
////@end CImpExpProgDlg content construction

    SetTitle(caption);
    item4->SetLabel(_("Processing:"));
    CategED->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    List->InsertColumn(0, wxEmptyString);
    OKBut->Enable(false);
}

/*!
 * Should we show tooltips?
 */

bool CImpExpProgDlg::ShowToolTips()
{
    return TRUE;
}
//
// Resize event hanler, sets list column size to whole list client size
//
void CImpExpProgDlg::OnSize(wxSizeEvent &event)
{
    int Width, Height;
    List->GetClientSize(&Width, &Height);
    List->SetColumnWidth(0, Width);
    event.Skip();
}
//
// t_export_param.callback implementation (static method)
//
void WINAPI CImpExpProgDlg::Progress(int Categ, const void *Value, void *Param)
{
    ((CImpExpProgDlg *)Param)->Progress(Categ, (const char *)Value);
}
//
// Shows current progress
//
void CImpExpProgDlg::Progress(int Categ, const char *Value)
{
    int Width, Height;
    const wxChar *Text;

    switch (Categ)
    {
    case IMPEXP_PROGMAX:
        ProgBar->SetRange((int)(size_t)Value);          // Reset ProgressBar
        SetSize(250, 400);
        Show();
        List->GetClientSize(&Width, &Height);           // Reset list colum width
        List->SetColumnWidth(0, Width);
        Update();
        counter=0;
        break;
    case IMPEXP_PROGSTEP:
        ProgBar->SetValue(ProgBar->GetValue() + 1);     // Set next ProgressBar step
        break;
    case IMPEXP_PROGPOS:
        ProgBar->SetValue((int)(size_t)Value);          // Set new ProgressBar value
        break;
    case IMPEXP_MSG_UNICODE:
        List->EnsureVisible(List->InsertItem(List->GetItemCount(), (const wxChar*)Value));
        List->wxWindow::Update();
        break;
    case IMPEXP_FILENAME:
        Append(Value);                                  // Show object filename
        break;
    case IMPEXP_ERROR:
        if (IsShown())                                  // IF dialog shown, show message in list box
            Append(Value);
        else                                            // ELSE show message in MessageBox
            error_box(wxString(Value, *wxConvCurrent));
        break;
    case IMPEXP_STATUS:
        Append(Value);
        if (!dupldb)                                    // this status information does not stop the operation during duplication
          if (IsShown())                                // IF Dialog shown, show operation result
          { OKBut->Enable(true);
            ShowModal();
          }
        break;  
    case IMPEXP_FINAL:
        if (IsShown())                                  // IF Dialog shown, show operation result
        {   if (*Value) Append(Value);
            OKBut->Enable(true);
            ShowModal();
        }
        break;
    default:
        wchar_t wc[64];                                 // Show object category
        if (Categ == IMPEXP_DATA)
            { Text = _("Data");  counter=0; }
        else if (Categ == IMPEXP_DATAPRIV)
            Text = _("Privileges");
        else if (Categ == IMPEXP_ICON)
            Text = _("Icon");
        else
            Text = CCategList::CaptFirst(m_cdp, Categ, wc);
        CategED->SetValue(Text);
        CategED->wxWindow::Update();
        NameED->SetValue(wxString(Value, *wxConvCurrent));  // Show object name
        NameED->wxWindow::Update();
    }
#ifdef WINS
    wxApp *App = &wxGetApp();                           // Dispatch system events to redrraw current dialog content
    while (App->Pending())
        App->Dispatch();
#else
    if (!(counter++ % 10))
      wxSafeYield(this);  // very slow, but necessary for updating the windows
#endif    
}
//
// Converts ASCII string to UNICODE wxString
//
wxString ToUnicode(const char *Src, int Size)
{
    CBuffer Buf;
    if (Src[Size])
    {
        if (!Buf.Alloc(Size + 1))
            return wxEmptyString;
        memcpy(Buf, Src, Size);
        Buf[Size] = 0;
        Src = Buf;
    }
    wxString Result;
    wxChar *Dst = Result.GetWriteBuf((Size + 1) * sizeof(wxChar));
    if (!Dst)
        return wxEmptyString;
    superconv(ATT_STRING, ATT_STRING, Src, (char *)Dst, t_specif(0, GetHostCharSet(), false, false), wx_string_spec, NULL);
    Result.UngetWriteBuf();
    return Result;
}
//
// Appends separate lines of Value to listbox content
//
void CImpExpProgDlg::Append(const char *Value)
{
    long Item = List->GetItemCount();
    const char *p, *n;
    for (p = Value; ; p = n)
    {
        n = strchr(p, '\r');
        if (!n)
        {
            n = strchr(p, '\n');
            if (!n)
                break;
        }
        List->InsertItem(Item++, ToUnicode(p, n - p));
        if (*n == '\r')
            n++;
        if (*n == '\n')
            n++;
    }

    List->InsertItem(Item, ToUnicode(p, (int)strlen(p)));
    List->EnsureVisible(Item);
    List->wxWindow::Update();
}

/*!
 * Get bitmap resources
 */

wxBitmap CImpExpProgDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CImpExpProgDlg bitmap retrieval
    return wxNullBitmap;
////@end CImpExpProgDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CImpExpProgDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CImpExpProgDlg icon retrieval
    return wxNullIcon;
////@end CImpExpProgDlg icon retrieval
}
