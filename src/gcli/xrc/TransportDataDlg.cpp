/////////////////////////////////////////////////////////////////////////////
// Name:        TransportDataDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/06/04 16:12:15
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "TransportDataDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "TransportDataDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * TransportDataDlg type definition
 */

IMPLEMENT_CLASS( TransportDataDlg, wxDialog )

/*!
 * TransportDataDlg event table definition
 */

BEGIN_EVENT_TABLE( TransportDataDlg, wxDialog )

////@begin TransportDataDlg event table entries
    EVT_BUTTON( CD_TR_SOURCE_NAME_SEL, TransportDataDlg::OnCdTrSourceNameSelClick )

    EVT_BUTTON( CD_TR_DEST_NAME_SEL, TransportDataDlg::OnCdTrDestNameSelClick )

    EVT_BUTTON( wxID_OK, TransportDataDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, TransportDataDlg::OnCancelClick )

////@end TransportDataDlg event table entries

END_EVENT_TABLE()

/*!
 * TransportDataDlg constructors
 */

TransportDataDlg::TransportDataDlg(t_ie_run * dsgnIn)
{ dsgn=dsgnIn; }

/*!
 * TransportDataDlg creator
 */

bool TransportDataDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin TransportDataDlg member initialisation
    mSourceName = NULL;
    mSourceNameSel = NULL;
    mDestName = NULL;
    mDestNameSel = NULL;
////@end TransportDataDlg member initialisation

////@begin TransportDataDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end TransportDataDlg creation
    return TRUE;
}

/*!
 * Control creation for TransportDataDlg
 */

void TransportDataDlg::CreateControls()
{    
////@begin TransportDataDlg content construction
    TransportDataDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 3, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Source:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mSourceNameStrings = NULL;
    mSourceName = new wxOwnerDrawnComboBox;
    mSourceName->Create( itemDialog1, CD_TR_SOURCE_NAME, _T(""), wxDefaultPosition, wxSize(200, -1), 0, mSourceNameStrings, wxCB_DROPDOWN );
    itemFlexGridSizer3->Add(mSourceName, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSourceNameSel = new wxButton;
    mSourceNameSel->Create( itemDialog1, CD_TR_SOURCE_NAME_SEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer3->Add(mSourceNameSel, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("Target:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText7, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mDestNameStrings = NULL;
    mDestName = new wxOwnerDrawnComboBox;
    mDestName->Create( itemDialog1, CD_TR_DEST_NAME, _T(""), wxDefaultPosition, wxSize(200, -1), 0, mDestNameStrings, wxCB_DROPDOWN );
    itemFlexGridSizer3->Add(mDestName, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDestNameSel = new wxButton;
    mDestNameSel->Create( itemDialog1, CD_TR_DEST_NAME_SEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer3->Add(mDestNameSel, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton11 = new wxButton;
    itemButton11->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end TransportDataDlg content construction
 // source objects:
  if (dsgn->db_source())
  { tcateg cat = dsgn->source_type==IETYPE_WB ? CATEG_TABLE : CATEG_CURSOR;
    void * en = get_object_enumerator(dsgn->sxcdp->get_cdp(), cat, dsgn->inschema);
    tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      if (cat==CATEG_TABLE ? !(flags & (CO_FLAG_ODBC | CO_FLAG_LINK)) : !(flags & CO_FLAG_LINK))
        mSourceName->Append(wxString(name, *dsgn->sxcdp->get_cdp()->sys_conv));
    free_object_enumerator(en);
    mSourceName->SetValue(wxString(dsgn->inpath, *dsgn->sxcdp->get_cdp()->sys_conv));  // object name
  }
  else
  { mSourceName->Append(wxString(dsgn->inpath, *wxConvCurrent));
    mSourceName->SetValue(wxString(dsgn->inpath, *wxConvCurrent));        // file ODBC table name
  }
  mSourceNameSel->Enable(dsgn->text_source() || dsgn->dbf_source());
 // target objects:
  if (dsgn->db_target())
  { void * en = get_object_enumerator(dsgn->txcdp->get_cdp(), CATEG_TABLE, dsgn->outschema);
    tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      if (!(flags & (CO_FLAG_ODBC | CO_FLAG_LINK)))
        mDestName->Append(wxString(name, *dsgn->txcdp->get_cdp()->sys_conv));
    free_object_enumerator(en);
    mDestName->SetValue(wxString(dsgn->outpath, *dsgn->txcdp->get_cdp()->sys_conv));  // object name
  }
  else
  { mDestName->Append(wxString(dsgn->outpath, *wxConvCurrent));
    mDestName->SetValue(wxString(dsgn->outpath, *wxConvCurrent));        // file or ODBC table name
  }
  mDestNameSel->Enable(dsgn->text_target() || dsgn->dbf_target());
}

/*!
 * Should we show tooltips?
 */

bool TransportDataDlg::ShowToolTips()
{
    return TRUE;
}

wxString get_filter2(int sel)
{ wxString filter;
  switch (sel)
  { case IETYPE_TDT:   filter = _("602SQL Table data files (*.tdt)|*.tdt|");  break;
    case IETYPE_CSV:   filter = _("CSV files (*.csv)|*.csv|");  break;
    case IETYPE_COLS:  filter = _("Files with data in columns (*.txt)|*.txt|");  break;
    case IETYPE_DBASE: filter = _("dBase files (*.dbf)|*.dbf|");  break;
    case IETYPE_FOX:   filter = _("FoxPro files (*.dbf)|*.dbf|");  break;
  }
#ifdef WINS
  return filter+_("All files|*.*");
#else
  return filter+_("All files|*");
#endif
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_SOURCE_NAME_SEL
 */

void TransportDataDlg::OnCdTrSourceNameSelClick( wxCommandEvent& event )
{
  wxString filter = get_filter2(dsgn->source_type);  // must not be only the parameter of the next function!
  wxString str = GetExportFileName(mSourceName->GetValue().c_str(), filter.c_str(), this);
  if (!str.IsEmpty())
    mSourceName->SetValue(str);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_DEST_NAME_SEL
 */

void TransportDataDlg::OnCdTrDestNameSelClick( wxCommandEvent& event )
{
  wxString filter = get_filter2(dsgn->dest_type);  // must not be only the parameter of the next function!
  wxString str = GetExportFileName(mDestName->GetValue().c_str(), filter.c_str(), this);
  if (!str.IsEmpty())
    mDestName->SetValue(str);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void TransportDataDlg::OnOkClick( wxCommandEvent& event )
{
  if (dsgn->db_source())
    strmaxcpy(dsgn->inpath,  mSourceName->GetValue().mb_str(*dsgn->cdp->sys_conv), sizeof(dsgn->inpath ));
  else
    strmaxcpy(dsgn->inpath,  mSourceName->GetValue().mb_str(*wxConvCurrent),       sizeof(dsgn->inpath ));
  if (dsgn->db_target())
    strmaxcpy(dsgn->outpath, mDestName  ->GetValue().mb_str(*dsgn->cdp->sys_conv), sizeof(dsgn->outpath));
  else
    strmaxcpy(dsgn->outpath, mDestName  ->GetValue().mb_str(*wxConvCurrent),       sizeof(dsgn->outpath));
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void TransportDataDlg::OnCancelClick( wxCommandEvent& event )
{
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap TransportDataDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin TransportDataDlg bitmap retrieval
    return wxNullBitmap;
////@end TransportDataDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon TransportDataDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin TransportDataDlg icon retrieval
    return wxNullIcon;
////@end TransportDataDlg icon retrieval
}
