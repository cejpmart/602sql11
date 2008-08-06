/////////////////////////////////////////////////////////////////////////////
// Name:        GridDataFormat.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/16/04 08:52:35
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "GridDataFormat.h"
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

#include "GridDataFormat.h"

////@begin XPM images
////@end XPM images

/*!
 * GridDataFormat type definition
 */

IMPLEMENT_CLASS( GridDataFormat, wxDialog )

/*!
 * GridDataFormat event table definition
 */

BEGIN_EVENT_TABLE( GridDataFormat, wxDialog )

////@begin GridDataFormat event table entries
    EVT_BUTTON( wxID_OK, GridDataFormat::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, GridDataFormat::OnCancelClick )

////@end GridDataFormat event table entries
END_EVENT_TABLE()

void set_real_format(signed char * precision, int decim, int form)
{ if (decim<0) get_real_format_info(*precision, &decim, NULL);
  if (form <0) get_real_format_info(*precision, NULL, &form);
  if (form==2) decim+=100;
  if (form==0 && decim==0) decim=127;
  *precision = form==0 ? -decim : decim;
}


/*!
 * GridDataFormat constructors
 */

GridDataFormat::GridDataFormat( )
{
}

GridDataFormat::GridDataFormat( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * GridDataFormat creator
 */

bool GridDataFormat::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin GridDataFormat member initialisation
    mDecSep = NULL;
    mReal = NULL;
    mDigits = NULL;
    mDate = NULL;
    mTime = NULL;
    mTimestamp = NULL;
////@end GridDataFormat member initialisation

////@begin GridDataFormat creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end GridDataFormat creation
    return TRUE;
}

/*!
 * Control creation for GridDataFormat
 */

void GridDataFormat::CreateControls()
{    
////@begin GridDataFormat content construction
    GridDataFormat* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(6, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, CD_FORMAT_C1, _("D&ecimal separator:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mDecSepStrings = NULL;
    mDecSep = new wxOwnerDrawnComboBox;
    mDecSep->Create( itemDialog1, CD_FORMAT_DECSEP, _T(""), wxDefaultPosition, wxSize(220, -1), 0, mDecSepStrings, wxCB_DROPDOWN );
    itemFlexGridSizer3->Add(mDecSep, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, CD_FORMAT_C2, _("&Real value format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mRealStrings = NULL;
    mReal = new wxOwnerDrawnComboBox;
    mReal->Create( itemDialog1, CD_FORMAT_REAL, _T(""), wxDefaultPosition, wxDefaultSize, 0, mRealStrings, wxCB_READONLY );
    if (ShowToolTips())
        mReal->SetToolTip(_("Decimal format example is 12345.67, scientfic format is 1.234567e4, separated format is 12 345.67"));
    itemFlexGridSizer3->Add(mReal, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemDialog1, CD_FORMAT_C3, _("Deci&mal digits in real values:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mDigits = new wxTextCtrl;
    mDigits->Create( itemDialog1, CD_FORMAT_DIGITS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mDigits, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText10 = new wxStaticText;
    itemStaticText10->Create( itemDialog1, CD_FORMAT_C4, _("&Date format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mDateStrings = NULL;
    mDate = new wxOwnerDrawnComboBox;
    mDate->Create( itemDialog1, CD_FORMAT_DATE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDateStrings, wxCB_DROPDOWN );
    itemFlexGridSizer3->Add(mDate, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText12 = new wxStaticText;
    itemStaticText12->Create( itemDialog1, CD_FORMAT_C5, _("&Time format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mTimeStrings = NULL;
    mTime = new wxOwnerDrawnComboBox;
    mTime->Create( itemDialog1, CD_FORMAT_TIME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTimeStrings, wxCB_DROPDOWN );
    itemFlexGridSizer3->Add(mTime, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText14 = new wxStaticText;
    itemStaticText14->Create( itemDialog1, CD_FORMAT_C6, _("Time&stamp format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText14, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mTimestampStrings = NULL;
    mTimestamp = new wxOwnerDrawnComboBox;
    mTimestamp->Create( itemDialog1, CD_FORMAT_TIMESTAMP, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTimestampStrings, wxCB_DROPDOWN );
    itemFlexGridSizer3->Add(mTimestamp, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer16, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton17 = new wxButton;
    itemButton17->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton17->SetDefault();
    itemBoxSizer16->Add(itemButton17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton18 = new wxButton;
    itemButton18->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemButton18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end GridDataFormat content construction
 // fill combo choices:
  mDecSep->Append(wxT("."));
  mDecSep->Append(wxT(","));
  mReal->Append(_("Decimal"));
  mReal->Append(_("Scientific (semi-logarithmic)"));
  mReal->Append(_("Separated digit groups"));
  mTime->Append(wxT("hh:mm"));
  mTime->Append(wxT("hh:mm:ss"));
  mTime->Append(wxT("hh:mm:ss.fff"));
  mDate->Append(wxT("D.M.CCYY"));
  mDate->Append(wxT("DD.MM.CCYY"));
  mDate->Append(wxT("DD.MM"));
  mDate->Append(wxT("MM/DD/CCYY"));
  mDate->Append(wxT("MM/DD"));
  mDate->Append(wxT("CCYY-MM-DD"));
  mTimestamp->Append(wxT("D.M.CCYY h:mm:ss"));
  mTimestamp->Append(wxT("DD.MM.CCYY hh:mm:ss"));
  mTimestamp->Append(wxT("CCYY-MM-DD hh:mm:ss"));
 // show data:
  if (mDecSep   ->FindString(format_decsep) == wxNOT_FOUND) mDecSep->Append(format_decsep);
  mDecSep   ->SetValue(format_decsep);
  if (mDate     ->FindString(format_date) == wxNOT_FOUND) mDate->Append(format_date);
  mDate     ->SetValue(format_date);
  if (mTime     ->FindString(format_time) == wxNOT_FOUND) mTime->Append(format_time);
  mTime     ->SetValue(format_time);
  if (mTimestamp->FindString(format_timestamp) == wxNOT_FOUND) mTimestamp->Append(format_timestamp);
  mTimestamp->SetValue(format_timestamp);
  int decim, form;  
  get_real_format_info(format_real, &decim, &form);
  SetIntValue(mDigits, decim);
  mReal->SetSelection(form);
  mDecSep->SetFocus();
}

/*!
 * Should we show tooltips?
 */

bool GridDataFormat::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void GridDataFormat::OnOkClick( wxCommandEvent& event )
{ unsigned long decval;
 // check data:
  if (!mDigits->GetValue().ToULong(&decval)) 
    { wxBell();  return; }
 // save data:
  format_decsep    = mDecSep   ->GetValue();
  format_date      = mDate     ->GetValue();
  format_time      = mTime     ->GetValue();
  format_timestamp = mTimestamp->GetValue();
  signed char precision;
  set_real_format(&precision, decval, mReal->GetSelection());
  format_real   = precision;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void GridDataFormat::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }



/*!
 * Get bitmap resources
 */

wxBitmap GridDataFormat::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin GridDataFormat bitmap retrieval
    return wxNullBitmap;
////@end GridDataFormat bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon GridDataFormat::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin GridDataFormat icon retrieval
    return wxNullIcon;
////@end GridDataFormat icon retrieval
}
