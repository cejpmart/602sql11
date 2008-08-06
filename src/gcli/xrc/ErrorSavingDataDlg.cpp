/////////////////////////////////////////////////////////////////////////////
// Name:        ErrorSavingDataDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/19/04 15:18:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "ErrorSavingDataDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "ErrorSavingDataDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ErrorSavingDataDlg type definition
 */

IMPLEMENT_CLASS( ErrorSavingDataDlg, wxDialog )

/*!
 * ErrorSavingDataDlg event table definition
 */

BEGIN_EVENT_TABLE( ErrorSavingDataDlg, wxDialog )

////@begin ErrorSavingDataDlg event table entries
    EVT_BUTTON( wxID_OK, ErrorSavingDataDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, ErrorSavingDataDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, ErrorSavingDataDlg::OnHelpClick )

////@end ErrorSavingDataDlg event table entries

END_EVENT_TABLE()

/*!
 * ErrorSavingDataDlg constructors
 */

ErrorSavingDataDlg::ErrorSavingDataDlg(xcdp_t xcdpIn)
{ xcdp=xcdpIn; }

/*!
 * ErrorSavingDataDlg creator
 */

bool ErrorSavingDataDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ErrorSavingDataDlg member initialisation
    mMsg = NULL;
    mHelp = NULL;
////@end ErrorSavingDataDlg member initialisation

////@begin ErrorSavingDataDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ErrorSavingDataDlg creation
    FindWindow(wxID_CANCEL)->SetFocus();
    return TRUE;
}

/*!
 * Control creation for ErrorSavingDataDlg
 */

void ErrorSavingDataDlg::CreateControls()
{    
////@begin ErrorSavingDataDlg content construction
    ErrorSavingDataDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mMsg = new wxTextCtrl;
    mMsg->Create( itemDialog1, CD_DATAERR_MSG, _T(""), wxDefaultPosition, wxSize(300, 50), wxTE_MULTILINE|wxTE_READONLY|wxTE_LINEWRAP );
    itemBoxSizer2->Add(mMsg, 1, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("You can either undo the changes or continue editing\nand correct the problem yourself."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText4, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton6 = new wxButton;
    itemButton6->Create( itemDialog1, wxID_OK, _("Undo"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton7 = new wxButton;
    itemButton7->Create( itemDialog1, wxID_CANCEL, _("Continue"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mHelp = new wxButton;
    mHelp->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(mHelp, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ErrorSavingDataDlg content construction
  int err;
  if (xcdp->get_cdp())
  { err = topic = cd_Sz_error(xcdp->get_cdp());
    if (!find_help_topic_for_error(topic)) mHelp->Disable();
  }
  else  
  { err = GENERR_ODBC_DRIVER;
    mHelp->Disable();
  }  
  mMsg->SetValue(Get_error_num_textWX(xcdp, err));
}

/*!
 * Should we show tooltips?
 */

bool ErrorSavingDataDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void ErrorSavingDataDlg::OnOkClick( wxCommandEvent& event )
{ event.Skip(); }

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void ErrorSavingDataDlg::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void ErrorSavingDataDlg::OnHelpClick( wxCommandEvent& event )
{ wxString pathname;
  pathname.Printf(wxT("xml/html/err%u.html"), topic);
  wxGetApp().frame->help_ctrl.DisplaySection(pathname); 
}



/*!
 * Get bitmap resources
 */

wxBitmap ErrorSavingDataDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ErrorSavingDataDlg bitmap retrieval
    return wxNullBitmap;
////@end ErrorSavingDataDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ErrorSavingDataDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ErrorSavingDataDlg icon retrieval
    return wxNullIcon;
////@end ErrorSavingDataDlg icon retrieval
}
