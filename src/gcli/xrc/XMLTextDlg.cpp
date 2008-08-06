/////////////////////////////////////////////////////////////////////////////
// Name:        XMLTextDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/25/04 13:54:03
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "XMLTextDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "XMLTextDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLTextDlg type definition
 */

IMPLEMENT_CLASS( XMLTextDlg, wxDialog )

/*!
 * XMLTextDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLTextDlg, wxDialog )

////@begin XMLTextDlg event table entries
    EVT_RADIOBUTTON( ID_RADIOBUTTONb, XMLTextDlg::OnRadiobuttonbSelected )

    EVT_COMBOBOX( ID_COMBOBOXb, XMLTextDlg::OnComboboxbSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON1b, XMLTextDlg::OnRadiobutton1bSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON2b, XMLTextDlg::OnRadiobutton2bSelected )

    EVT_BUTTON( CD_XMLCOL_TRANS, XMLTextDlg::OnCdXmlcolTransClick )

    EVT_BUTTON( wxID_OK, XMLTextDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLTextDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, XMLTextDlg::OnHelpClick )

////@end XMLTextDlg event table entries

END_EVENT_TABLE()

/*!
 * XMLTextDlg constructors
 */

XMLTextDlg::XMLTextDlg(xml_designer * dadeIn, t_dad_col_name column_nameIn, dad_table_name table_nameIn, t_link_types link_typeIn, 
  wxString formatIn, wxString fixed_valIn,  tobjname inconv_fncIn, tobjname outconv_fncIn, bool updkeyIn)
{ dade=dadeIn;  cdp=dade->cdp;  format=formatIn;  fixed_val=fixed_valIn;
  strcpy(column_name, column_nameIn);  strcpy(table_name, table_nameIn);  link_type=link_typeIn;
  strcpy(inconv_fnc, inconv_fncIn);  strcpy(outconv_fnc, outconv_fncIn);  updkey=updkeyIn;
}

/*!
 * XMLTextDlg creator
 */

bool XMLTextDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLTextDlg member initialisation
    mAssocColumn = NULL;
    mTableC = NULL;
    mTable = NULL;
    mColumnC = NULL;
    mColumn = NULL;
    mUpdateKey = NULL;
    mAssocVariable = NULL;
    mVariableC = NULL;
    mVariable = NULL;
    mAssocConstant = NULL;
    mConstant = NULL;
    mFormatCapt = NULL;
    mFormat = NULL;
////@end XMLTextDlg member initialisation

////@begin XMLTextDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLTextDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLTextDlg
 */

void XMLTextDlg::CreateControls()
{    
////@begin XMLTextDlg content construction
    XMLTextDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mAssocColumn = new wxRadioButton;
    mAssocColumn->Create( itemDialog1, ID_RADIOBUTTONb, _("Associated with the database column:"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
    mAssocColumn->SetValue(FALSE);
    itemBoxSizer2->Add(mAssocColumn, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    itemBoxSizer4->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer7 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer7->AddGrowableCol(1);
    itemBoxSizer6->Add(itemFlexGridSizer7, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mTableC = new wxStaticText;
    mTableC->Create( itemDialog1, ID_STATICTEXT, _("Table:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(mTableC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mTableStrings = NULL;
    mTable = new wxOwnerDrawnComboBox;
    mTable->Create( itemDialog1, ID_COMBOBOXb, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTableStrings, wxCB_READONLY );
    itemFlexGridSizer7->Add(mTable, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mColumnC = new wxStaticText;
    mColumnC->Create( itemDialog1, ID_STATICTEXT1, _("Column or expression:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(mColumnC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mColumnStrings = NULL;
    mColumn = new wxOwnerDrawnComboBox;
    mColumn->Create( itemDialog1, ID_COMBOBOX1b, _T(""), wxDefaultPosition, wxDefaultSize, 0, mColumnStrings, wxCB_READONLY );
    itemFlexGridSizer7->Add(mColumn, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mUpdateKey = new wxCheckBox;
    mUpdateKey->Create( itemDialog1, CD_XMLCOL_UPDATE_KEY, _("Is a key of the record to be updated"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mUpdateKey->SetValue(FALSE);
    itemBoxSizer6->Add(mUpdateKey, 0, wxALIGN_LEFT|wxALL, 0);

    mAssocVariable = new wxRadioButton;
    mAssocVariable->Create( itemDialog1, ID_RADIOBUTTON1b, _("Associated with the variable:"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
    mAssocVariable->SetValue(FALSE);
    itemBoxSizer2->Add(mAssocVariable, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer14, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    itemBoxSizer14->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mVariableC = new wxStaticText;
    mVariableC->Create( itemDialog1, wxID_STATICb, _("Variable name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(mVariableC, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mVariable = new wxTextCtrl;
    mVariable->Create( itemDialog1, ID_TEXTCTRL4b, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(mVariable, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mAssocConstant = new wxRadioButton;
    mAssocConstant->Create( itemDialog1, ID_RADIOBUTTON2b, _("Constant value:"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
    mAssocConstant->SetValue(FALSE);
    itemBoxSizer2->Add(mAssocConstant, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer19, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    itemBoxSizer19->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mConstant = new wxTextCtrl;
    mConstant->Create( itemDialog1, ID_TEXTCTRL5b, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(mConstant, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer22 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer22, 0, wxGROW|wxALL, 0);

    mFormatCapt = new wxStaticText;
    mFormatCapt->Create( itemDialog1, CD_XMLCOL_FORMAT_CAPT, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer22->Add(mFormatCapt, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mFormat = new wxTextCtrl;
    mFormat->Create( itemDialog1, CD_XMLCOL_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer22->Add(mFormat, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton25 = new wxButton;
    itemButton25->Create( itemDialog1, CD_XMLCOL_TRANS, _("Translation"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer22->Add(itemButton25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer26, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton27 = new wxButton;
    itemButton27->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton27->SetDefault();
    itemBoxSizer26->Add(itemButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton28 = new wxButton;
    itemButton28->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemButton28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton29 = new wxButton;
    itemButton29->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemButton29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLTextDlg content construction
 // link:
  mVariable->SetMaxLength(sizeof(t_dad_col_name)-1);
  fill_my_table_names(dade, mTable, NULL, true);
  if      (link_type==LT_COLUMN) { mAssocColumn->SetValue(true);    mAssocColumn->SetFocus();   }
  else if (link_type==LT_VAR)    { mAssocVariable->SetValue(true);  mAssocVariable->SetFocus(); }
  else                           { mAssocConstant->SetValue(true);  mAssocConstant->SetFocus(); }
  enabling();
  link_show();
  mUpdateKey->SetValue(updkey);
}

void XMLTextDlg::enabling(void)
{ bool en = link_type==LT_COLUMN;
  mTableC ->Enable(en);  mTable ->Enable(en);
  mColumnC->Enable(en);  mColumn->Enable(en);
  en = link_type==LT_VAR;
  mVariableC->Enable(en);  mVariable->Enable(en);
  mConstant->Enable(link_type==LT_CONST);
  mFormat->Enable(link_type!=LT_CONST);
  mFormatCapt->Enable(link_type!=LT_CONST);
}

void XMLTextDlg::link_show(void)
{ mVariable->SetValue(wxEmptyString);
  mConstant->SetValue(wxEmptyString);
  mTable->SetSelection(-1);
  mColumn->SetSelection(-1);
  if (link_type==LT_COLUMN)
  { if (*table_name)
    { int ind = mTable->FindString(wxString(table_name, *cdp->sys_conv));
      if (ind!=-1)
      { mTable->SetSelection(ind);
        fill_column_names(dade, mTable, NULL, NULL, mColumn, column_name);
      }
    }
    else  // table not specified yet
    { if (mTable->GetCount() == 1)  // only one table is in the list
      { mTable->SetSelection(0);  // select it
        fill_column_names(dade, mTable, NULL, NULL, mColumn, column_name);
      }
    }
    int ind = mColumn->FindString(wxString(column_name, *cdp->sys_conv));
    if (ind!=-1) mColumn->SetSelection(ind);
    mAssocVariable->SetValue(false);    
    mAssocConstant->SetValue(false);
  }
  else if (link_type==LT_VAR)
  { mVariable->SetValue(wxString(column_name, *cdp->sys_conv));
    mAssocColumn->SetValue(false);    
    mAssocConstant->SetValue(false);
  }
  else // link_type==LT_CONST
  { mConstant->SetValue(fixed_val);
    mAssocVariable->SetValue(false);    
    mAssocColumn->SetValue(false);
  }
  mFormat->SetValue(format);
}

void XMLTextDlg::link_store(void)
{ *table_name=0;
  if (link_type==LT_COLUMN)
  { int ind = mTable->GetSelection();
    if (ind!=-1) strcpy(table_name, mTable->GetString(ind).mb_str(*cdp->sys_conv));
    ind = mColumn->GetSelection();
    if (ind!=-1) strcpy(column_name, mColumn->GetString(ind).mb_str(*cdp->sys_conv));
  }
  else if (link_type==LT_VAR)
    strcpy(column_name, mVariable->GetValue().mb_str(*cdp->sys_conv));
  else // link_type==LT_CONST
    fixed_val=mConstant->GetValue();
  format=mFormat->GetValue();
}
/*!
 * Should we show tooltips?
 */

bool XMLTextDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON
 */

void XMLTextDlg::OnRadiobuttonbSelected( wxCommandEvent& event )
{ link_type=LT_COLUMN;  enabling();  link_show();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON1
 */

void XMLTextDlg::OnRadiobutton1bSelected( wxCommandEvent& event )
{ link_type=LT_VAR;  enabling();  link_show();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON2
 */

void XMLTextDlg::OnRadiobutton2bSelected( wxCommandEvent& event )
{ link_type=LT_CONST;  enabling();  link_show();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLTextDlg::OnOkClick( wxCommandEvent& event )
{ link_store();
  updkey=mUpdateKey->GetValue();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLTextDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void XMLTextDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/xmllinks.html"));
}


/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOXb
 */

void XMLTextDlg::OnComboboxbSelected( wxCommandEvent& event )
{
  fill_column_names(dade, mTable, NULL, NULL, mColumn, column_name);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLCOL_TRANS
 */

void XMLTextDlg::OnCdXmlcolTransClick( wxCommandEvent& event )
{
  XMLTranslationDlg dlg(cdp, inconv_fnc, outconv_fnc);
  dlg.Create(this);
  dlg.ShowModal();
}



/*!
 * Get bitmap resources
 */

wxBitmap XMLTextDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLTextDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLTextDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLTextDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLTextDlg icon retrieval
    return wxNullIcon;
////@end XMLTextDlg icon retrieval
}
