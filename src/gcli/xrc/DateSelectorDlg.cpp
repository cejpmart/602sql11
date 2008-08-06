/////////////////////////////////////////////////////////////////////////////
// Name:        DateSelectorDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/24/04 09:42:19
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "DateSelectorDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes      
////@end includes

#include "DateSelectorDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * DateSelectorDlg type definition
 */

IMPLEMENT_CLASS( DateSelectorDlg, wxDialog )

/*!
 * DateSelectorDlg event table definition
 */

BEGIN_EVENT_TABLE( DateSelectorDlg, wxDialog )

////@begin DateSelectorDlg event table entries
    EVT_CALENDAR( ID_CALCTRL, DateSelectorDlg::OnCalctrlDoubleClicked )

    EVT_BUTTON( wxID_CANCEL, DateSelectorDlg::OnCancelClick )

////@end DateSelectorDlg event table entries
END_EVENT_TABLE()

/*!
 * DateSelectorDlg constructors
 */

DateSelectorDlg::DateSelectorDlg(wxDateTime * dtmIn)
{ p_dtm = dtmIn; }

/*!
 * DateSelectorDlg creator
 */

bool DateSelectorDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DateSelectorDlg member initialisation
    mSizer = NULL;
    mCal = NULL;
////@end DateSelectorDlg member initialisation

////@begin DateSelectorDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
////@end DateSelectorDlg creation
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    return TRUE;
}

/*!
 * Control creation for DateSelectorDlg
 */

void DateSelectorDlg::CreateControls()
{    
////@begin DateSelectorDlg content construction

    DateSelectorDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    mSizer = item2;
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxStaticText* item3 = new wxStaticText;
    item3->Create( item1, wxID_STATIC, _("Double-click to select the date:"), wxDefaultPosition, wxDefaultSize, 0 );
    item2->Add(item3, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxCalendarCtrl* item4 = new wxCalendarCtrl;
    item4->Create( item1, ID_CALCTRL, wxDateTime(), wxDefaultPosition, wxDefaultSize, wxCAL_MONDAY_FIRST|wxSUNKEN_BORDER );
    mCal = item4;
    item2->Add(item4, 1, wxGROW|wxALL, 5);

    wxBoxSizer* item5 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* item6 = new wxButton;
    item6->Create( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end DateSelectorDlg content construction
    mCal->SetDate(*p_dtm);
    mSizer->SetItemMinSize(mCal, 150, 156); 
    mCal->GetMonthControl()->SetSize(-1, -1, -1, 20, wxSIZE_USE_EXISTING); // because of error in wxCalendarCtrl (-1 height of month combo) the size month list is not added to the size of the combo - the list has zero height on MSW95 (not XP)
    mCal->SetBackgroundColour(wxColour(255,255,255));  // otherwise gray
#ifdef WINS  // repairing error in spin control in WX 2.5.2
    //HWND hEdit  = (HWND)::SendMessage((HWND)mCal->GetYearControl()->GetHandle(), WM_USER+106, 0, 0);
    //::SendMessage(hEdit, WM_SETFONT, (WPARAM)system_gui_font.GetResourceHandle(), MAKELPARAM(TRUE, 0));
#endif
}

/*!
 * Should we show tooltips?
 */

bool DateSelectorDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_CALENDAR_DOUBLECLICKED event handler for ID_CALCTRL
 */

void DateSelectorDlg::OnCalctrlDoubleClicked( wxCalendarEvent& event )
{ *p_dtm = event.GetDate();
  EndModal(1);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DateSelectorDlg::OnCancelClick( wxCommandEvent& event )
{
  event.Skip();
}


