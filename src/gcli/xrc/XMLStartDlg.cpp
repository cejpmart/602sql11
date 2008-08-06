/////////////////////////////////////////////////////////////////////////////
// Name:        XMLStartDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/24/04 18:01:22
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "XMLStartDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "XMLStartDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLStartDlg type definition
 */

IMPLEMENT_CLASS( XMLStartDlg, wxDialog )

/*!
 * XMLStartDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLStartDlg, wxDialog )

////@begin XMLStartDlg event table entries
    EVT_RADIOBUTTON( CD_XMLSTART_TABLE, XMLStartDlg::OnCdXmlstartTableSelected )

    EVT_RADIOBUTTON( CD_XMLSTART_QUERY, XMLStartDlg::OnCdXmlstartQuerySelected )

    EVT_RADIOBUTTON( CD_XMLSTART_EMPTY, XMLStartDlg::OnCdXmlstartEmptySelected )

    EVT_BUTTON( wxID_OK, XMLStartDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLStartDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, XMLStartDlg::OnHelpClick )

////@end XMLStartDlg event table entries

END_EVENT_TABLE()

/*!
 * XMLStartDlg constructors
 */

XMLStartDlg::XMLStartDlg(cdp_t cdpIn)
{ cdp=cdpIn;  new_td=NULL; }

/*!
 * XMLStartDlg creator
 */

bool XMLStartDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLStartDlg member initialisation
    mByTable = NULL;
    mTableNameC = NULL;
    mTableName = NULL;
    mTableAliasC = NULL;
    mTableAlias = NULL;
    mByQuery = NULL;
    mQuery = NULL;
    mByEmpty = NULL;
////@end XMLStartDlg member initialisation

////@begin XMLStartDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLStartDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLStartDlg
 */

void XMLStartDlg::CreateControls()
{    
////@begin XMLStartDlg content construction
    XMLStartDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mByTable = new wxRadioButton;
    mByTable->Create( itemDialog1, CD_XMLSTART_TABLE, _("Start by mapping columns of the top-level table:"), wxDefaultPosition, wxDefaultSize, 0 );
    mByTable->SetValue(FALSE);
    itemBoxSizer2->Add(mByTable, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    itemBoxSizer4->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer6 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer6->AddGrowableCol(1);
    itemBoxSizer4->Add(itemFlexGridSizer6, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    mTableNameC = new wxStaticText;
    mTableNameC->Create( itemDialog1, wxID_STATIC, _("Table name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer6->Add(mTableNameC, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxString* mTableNameStrings = NULL;
    mTableName = new wxOwnerDrawnComboBox;
    mTableName->Create( itemDialog1, ID_COMBOBOX, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTableNameStrings, wxCB_DROPDOWN );
    itemFlexGridSizer6->Add(mTableName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    mTableAliasC = new wxStaticText;
    mTableAliasC->Create( itemDialog1, wxID_STATIC, _("Alias:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer6->Add(mTableAliasC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mTableAlias = new wxTextCtrl;
    mTableAlias->Create( itemDialog1, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer6->Add(mTableAlias, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mByQuery = new wxRadioButton;
    mByQuery->Create( itemDialog1, CD_XMLSTART_QUERY, _("Start by mapping columns of the query:"), wxDefaultPosition, wxDefaultSize, 0 );
    mByQuery->SetValue(FALSE);
    itemBoxSizer2->Add(mByQuery, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer12, 1, wxGROW|wxALL, 0);

    itemBoxSizer12->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mQuery = new wxTextCtrl;
    mQuery->Create( itemDialog1, ID_TEXTCTRL1, _T(""), wxDefaultPosition, wxSize(-1, 50), wxTE_MULTILINE );
    itemBoxSizer12->Add(mQuery, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mByEmpty = new wxRadioButton;
    mByEmpty->Create( itemDialog1, CD_XMLSTART_EMPTY, _("Start from an empty design"), wxDefaultPosition, wxDefaultSize, 0 );
    mByEmpty->SetValue(FALSE);
    itemBoxSizer2->Add(mByEmpty, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer16, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP, 5);

    wxButton* itemButton17 = new wxButton;
    itemButton17->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemButton17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton18 = new wxButton;
    itemButton18->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemButton18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton19 = new wxButton;
    itemButton19->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemButton19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLStartDlg content construction
    mTableAlias->SetMaxLength(OBJ_NAME_LEN);
    fill_table_names(cdp, mTableName, false, "");
    mByTable->SetValue(true);
    enabling();
}

void XMLStartDlg::enabling(void)
{ bool bytable = mByTable->GetValue();
  mTableNameC ->Enable(bytable);  mTableName ->Enable(bytable);
  mTableAliasC->Enable(bytable);  mTableAlias->Enable(bytable);
  mQuery->Enable(mByQuery->GetValue());
}

/*!
 * Should we show tooltips?
 */

bool XMLStartDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLStartDlg::OnOkClick( wxCommandEvent& event )
{ if (mByTable->GetValue())
  { int ind = mTableName->GetSelection();
    if (ind==-1) { wxBell();  return; }
    strcpy(table_name, mTableName->GetString(ind).c_str());
    wxString alias = mTableAlias->GetValue();
    alias.Trim(true);  alias.Trim(false);
    strcpy(table_alias, alias.c_str());
    EndModal(1);
  }
  else if (mByQuery->GetValue())
  { query=mQuery->GetValue();
    new_td = get_expl_descriptor(cdp, query.c_str(), NULL); 
    if (!new_td) return;
    EndModal(2);
  }
  else
    EndModal(3);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLStartDlg::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void XMLStartDlg::OnHelpClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_TABLE
 */

void XMLStartDlg::OnCdXmlstartTableSelected( wxCommandEvent& event )
{ enabling();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_QUERY
 */

void XMLStartDlg::OnCdXmlstartQuerySelected( wxCommandEvent& event )
{ enabling();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_EMPTY
 */

void XMLStartDlg::OnCdXmlstartEmptySelected( wxCommandEvent& event )
{ enabling();
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap XMLStartDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLStartDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLStartDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLStartDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLStartDlg icon retrieval
    return wxNullIcon;
////@end XMLStartDlg icon retrieval
}
