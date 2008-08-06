/////////////////////////////////////////////////////////////////////////////
// Name:        NewSSDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     07/24/06 15:23:07
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "NewSSDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "NewSSDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * NewSSDlg type definition
 */

IMPLEMENT_CLASS( NewSSDlg, wxDialog )

/*!
 * NewSSDlg event table definition
 */

BEGIN_EVENT_TABLE( NewSSDlg, wxDialog )

////@begin NewSSDlg event table entries
    EVT_BUTTON( wxID_OK, NewSSDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, NewSSDlg::OnCancelClick )

////@end NewSSDlg event table entries

END_EVENT_TABLE()

/*!
 * NewSSDlg constructors
 */

NewSSDlg::NewSSDlg(cdp_t cdpIn)
{ cdp=cdpIn;
}

/*!
 * NewSSDlg creator
 */

bool NewSSDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NewSSDlg member initialisation
    mType = NULL;
    mName = NULL;
////@end NewSSDlg member initialisation

////@begin NewSSDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end NewSSDlg creation
    return TRUE;
}

/*!
 * Control creation for NewSSDlg
 */

void NewSSDlg::CreateControls()
{    
////@begin NewSSDlg content construction
    NewSSDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxString mTypeStrings[] = {
        _("&XML Style Sheet"),
        _("&Cascading Style Sheet")
    };
    mType = new wxRadioBox;
    mType->Create( itemDialog1, CD_NEWSS_TYPE, _("Select the type of the style sheet"), wxDefaultPosition, wxDefaultSize, 2, mTypeStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer2->Add(mType, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Object name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mName = new wxTextCtrl;
    mName->Create( itemDialog1, CD_NEWSS_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(mName, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton8 = new wxButton;
    itemButton8->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end NewSSDlg content construction
    mName->SetMaxLength(OBJ_NAME_LEN);
    mType->SetSelection(0);
}

/*!
 * Should we show tooltips?
 */

bool NewSSDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap NewSSDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NewSSDlg bitmap retrieval
    return wxNullBitmap;
////@end NewSSDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NewSSDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NewSSDlg icon retrieval
    return wxNullIcon;
////@end NewSSDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void NewSSDlg::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in NewSSDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in NewSSDlg. 
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void NewSSDlg::OnOkClick( wxCommandEvent& event )
{ tobjnum auxobj;
 // check the name:
  name = mName->GetValue();
  name.Trim(false);  name.Trim(true);
  if (name.IsEmpty())
    { unpos_box(_("The name cannot be empty."), this);  return; }
  if (!is_object_name(cdp, name, this)) return;  // the conversion may not the the proper one, but is it sufficient
 // try to find the object:
  if (!cd_Find_prefixed_object(cdp, cdp->sel_appl_name, name.mb_str(*cdp->sys_conv), CATEG_STSH, &auxobj)) 
    { unpos_box(OBJ_NAME_USED);  return; }
  event.Skip();
}


