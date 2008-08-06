/////////////////////////////////////////////////////////////////////////////
// Name:        XMLAttributeDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/25/04 11:30:02
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "XMLAttributeDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "XMLAttributeDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLAttributeDlg type definition
 */

IMPLEMENT_CLASS( XMLAttributeDlg, wxDialog )

/*!
 * XMLAttributeDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLAttributeDlg, wxDialog )

////@begin XMLAttributeDlg event table entries
    EVT_RADIOBUTTON( ID_RADIOBUTTON, XMLAttributeDlg::OnRadiobuttonSelected )

    EVT_COMBOBOX( ID_COMBOBOXa, XMLAttributeDlg::OnComboboxaSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON1, XMLAttributeDlg::OnRadiobutton1Selected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON2, XMLAttributeDlg::OnRadiobutton2Selected )

    EVT_BUTTON( CD_XMLCOL_TRANS, XMLAttributeDlg::OnCdXmlcolTransClick )

    EVT_BUTTON( wxID_OK, XMLAttributeDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLAttributeDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, XMLAttributeDlg::OnHelpClick )

////@end XMLAttributeDlg event table entries

END_EVENT_TABLE()

/*!
 * XMLAttributeDlg constructors
 */

XMLAttributeDlg::XMLAttributeDlg(xml_designer * dadeIn, t_dad_attribute_name attr_nameIn, wxString attrcondIn, wxString formatIn, wxString fixed_valIn,
                    t_dad_col_name column_nameIn, dad_table_name table_nameIn, t_link_types link_typeIn, tobjname inconv_fncIn, tobjname outconv_fncIn, bool updkeyIn)
{ dade=dadeIn;  cdp=dade->cdp;  
  strcpy(attr_name, attr_nameIn);  attrcond=attrcondIn;  format=formatIn;  fixed_val=fixed_valIn;
  strcpy(column_name, column_nameIn);  strcpy(table_name, table_nameIn);  link_type=link_typeIn;
  strcpy(inconv_fnc, inconv_fncIn);  strcpy(outconv_fnc, outconv_fncIn);  updkey=updkeyIn;
}

/*!
 * XMLAttributeDlg creator
 */

bool XMLAttributeDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLAttributeDlg member initialisation
    mAttrName = NULL;
    mAttrCond = NULL;
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
////@end XMLAttributeDlg member initialisation

////@begin XMLAttributeDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLAttributeDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLAttributeDlg
 */

void XMLAttributeDlg::CreateControls()
{    
////@begin XMLAttributeDlg content construction
    XMLAttributeDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Attribute name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mAttrName = new wxTextCtrl;
    mAttrName->Create( itemDialog1, ID_TEXTCTRLa, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mAttrName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, wxID_STATIC, _("Output condition:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mAttrCond = new wxTextCtrl;
    mAttrCond->Create( itemDialog1, ID_TEXTCTRL1a, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mAttrCond, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer8Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Value of the attribute"));
    wxStaticBoxSizer* itemStaticBoxSizer8 = new wxStaticBoxSizer(itemStaticBoxSizer8Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer8, 0, wxGROW|wxALL, 5);

    mAssocColumn = new wxRadioButton;
    mAssocColumn->Create( itemDialog1, ID_RADIOBUTTON, _("Associated with the database column:"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
    mAssocColumn->SetValue(FALSE);
    itemStaticBoxSizer8->Add(mAssocColumn, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer8->Add(itemBoxSizer10, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    itemBoxSizer10->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer10->Add(itemBoxSizer12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer13 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer13->AddGrowableCol(1);
    itemBoxSizer12->Add(itemFlexGridSizer13, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mTableC = new wxStaticText;
    mTableC->Create( itemDialog1, wxID_STATIC, _("Table:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer13->Add(mTableC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mTableStrings = NULL;
    mTable = new wxOwnerDrawnComboBox;
    mTable->Create( itemDialog1, ID_COMBOBOXa, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTableStrings, wxCB_READONLY );
    itemFlexGridSizer13->Add(mTable, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mColumnC = new wxStaticText;
    mColumnC->Create( itemDialog1, wxID_STATIC, _("Column or expression:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer13->Add(mColumnC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mColumnStrings = NULL;
    mColumn = new wxOwnerDrawnComboBox;
    mColumn->Create( itemDialog1, ID_COMBOBOX1a, _T(""), wxDefaultPosition, wxDefaultSize, 0, mColumnStrings, wxCB_READONLY );
    itemFlexGridSizer13->Add(mColumn, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mUpdateKey = new wxCheckBox;
    mUpdateKey->Create( itemDialog1, CD_XMLCOL_UPDATE_KEY, _("Is a key of the record to be updated"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mUpdateKey->SetValue(FALSE);
    itemBoxSizer12->Add(mUpdateKey, 0, wxALIGN_LEFT|wxALL, 0);

    mAssocVariable = new wxRadioButton;
    mAssocVariable->Create( itemDialog1, ID_RADIOBUTTON1, _("Associated with the variable:"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
    mAssocVariable->SetValue(FALSE);
    itemStaticBoxSizer8->Add(mAssocVariable, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer8->Add(itemBoxSizer20, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    itemBoxSizer20->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mVariableC = new wxStaticText;
    mVariableC->Create( itemDialog1, wxID_STATIC, _("Variable name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(mVariableC, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mVariable = new wxTextCtrl;
    mVariable->Create( itemDialog1, ID_TEXTCTRL4, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(mVariable, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mAssocConstant = new wxRadioButton;
    mAssocConstant->Create( itemDialog1, ID_RADIOBUTTON2, _("Constant value:"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
    mAssocConstant->SetValue(FALSE);
    itemStaticBoxSizer8->Add(mAssocConstant, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer8->Add(itemBoxSizer25, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    itemBoxSizer25->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mConstant = new wxTextCtrl;
    mConstant->Create( itemDialog1, ID_TEXTCTRL5, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(mConstant, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer28 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer28, 0, wxGROW|wxALL, 0);

    mFormatCapt = new wxStaticText;
    mFormatCapt->Create( itemDialog1, CD_XMLCOL_FORMAT_CAPT, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(mFormatCapt, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mFormat = new wxTextCtrl;
    mFormat->Create( itemDialog1, CD_XMLCOL_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(mFormat, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton31 = new wxButton;
    itemButton31->Create( itemDialog1, CD_XMLCOL_TRANS, _("Translation"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(itemButton31, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer32, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton33 = new wxButton;
    itemButton33->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton33->SetDefault();
    itemBoxSizer32->Add(itemButton33, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton34 = new wxButton;
    itemButton34->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemButton34, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton35 = new wxButton;
    itemButton35->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemButton35, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLAttributeDlg content construction
 // attribute:
  mAttrName->SetMaxLength(sizeof(attr_name)-1);
  mAttrName->SetValue(wxString(attr_name, *cdp->sys_conv));
  mAttrCond->SetValue(attrcond);
 // link:
  mVariable->SetMaxLength(sizeof(t_dad_col_name)-1);
  fill_my_table_names(dade, mTable, NULL, true);
  if      (link_type==LT_COLUMN) { mAssocColumn->SetValue(true);    mAssocColumn->SetFocus();   }
  else if (link_type==LT_VAR)    { mAssocVariable->SetValue(true);  mAssocVariable->SetFocus(); }
  else                           { mAssocConstant->SetValue(true);  mAssocConstant->SetFocus(); }
  enabling();
  link_show();
  mUpdateKey->SetValue(updkey);
  /*if (!*attr_name)*/ mAttrName->SetFocus();  // otherwise a radiobutton is focused
  mAttrName->SetInsertionPoint(0);  // names starting with @ need it, must be after focusing
  mAttrName->SetSelection(0,0);
}

void XMLAttributeDlg::enabling(void)
{ bool en = link_type==LT_COLUMN;
  mTableC ->Enable(en);  mTable ->Enable(en);
  mColumnC->Enable(en);  mColumn->Enable(en);
  en = link_type==LT_VAR;
  mVariableC->Enable(en);  mVariable->Enable(en);
  mConstant->Enable(link_type==LT_CONST);
  mFormat->Enable(link_type!=LT_CONST);
  mFormatCapt->Enable(link_type!=LT_CONST);
}

void XMLAttributeDlg::link_show(void)
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

void XMLAttributeDlg::link_store(void)
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

bool XMLAttributeDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON
 */

void XMLAttributeDlg::OnRadiobuttonSelected( wxCommandEvent& event )
{ link_type=LT_COLUMN;  enabling();  link_show();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON1
 */

void XMLAttributeDlg::OnRadiobutton1Selected( wxCommandEvent& event )
{ link_type=LT_VAR;  enabling();  link_show();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON2
 */

void XMLAttributeDlg::OnRadiobutton2Selected( wxCommandEvent& event )
{ link_type=LT_CONST;  enabling();  link_show();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLAttributeDlg::OnOkClick( wxCommandEvent& event )
{ link_store();
  strcpy(attr_name, mAttrName->GetValue().mb_str(*cdp->sys_conv));
  attrcond = mAttrCond->GetValue();
  updkey=mUpdateKey->GetValue();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLAttributeDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void XMLAttributeDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/xmllinks.html"));
}


/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOXa
 */

void XMLAttributeDlg::OnComboboxaSelected( wxCommandEvent& event )
{
  fill_column_names(dade, mTable, NULL, NULL, mColumn, column_name);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLCOL_TRANS
 */

void XMLAttributeDlg::OnCdXmlcolTransClick( wxCommandEvent& event )
{
  XMLTranslationDlg dlg(cdp, inconv_fnc, outconv_fnc);
  dlg.Create(this);
  dlg.ShowModal();
}



/*!
 * Get bitmap resources
 */

wxBitmap XMLAttributeDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLAttributeDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLAttributeDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLAttributeDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLAttributeDlg icon retrieval
    return wxNullIcon;
////@end XMLAttributeDlg icon retrieval
}
