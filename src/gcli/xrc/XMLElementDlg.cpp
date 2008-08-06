/////////////////////////////////////////////////////////////////////////////
// Name:        XMLElementDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/23/04 13:47:11
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "XMLElementDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "XMLElementDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLElementDlg type definition
 */

IMPLEMENT_CLASS( XMLElementDlg, wxDialog )

/*!
 * XMLElementDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLElementDlg, wxDialog )

////@begin XMLElementDlg event table entries
    EVT_CHECKBOX( CD_XML_RELATION, XMLElementDlg::OnCdXmlRelationClick )

    EVT_CHECKBOX( CD_XML_RECUR, XMLElementDlg::OnCdXmlRecurClick )

    EVT_TEXT( CD_XML_TABLENAME, XMLElementDlg::OnCdXmlTablenameUpdated )

    EVT_TEXT( CD_XML_ALIAS, XMLElementDlg::OnCdXmlAliasUpdated )

    EVT_TEXT( CD_XML_UTABLENAME, XMLElementDlg::OnCdXmlUtablenameUpdated )

    EVT_CHECKBOX( CD_XML_INFOCOL, XMLElementDlg::OnCdXmlInfocolClick )

    EVT_TEXT( CD_XML_ITABLENAME, XMLElementDlg::OnCdXmlItablenameUpdated )

    EVT_BUTTON( wxID_OK, XMLElementDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLElementDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, XMLElementDlg::OnHelpClick )

////@end XMLElementDlg event table entries

END_EVENT_TABLE()

/*!
 * XMLElementDlg constructors
 */

XMLElementDlg::XMLElementDlg(xml_designer * dadeIn, dad_element_node * elIn)
{ dade=dadeIn;  el=elIn;  cdp=dade->cdp; }

/*!
 * XMLElementDlg creator
 */

bool XMLElementDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLElementDlg member initialisation
    mName = NULL;
    mRelation = NULL;
    mRelationGroup = NULL;
    mRecur = NULL;
    mTablenameC = NULL;
    mTablename = NULL;
    mAliasC = NULL;
    mAlias = NULL;
    mColumnnameC = NULL;
    mColumnname = NULL;
    mUtablenameC = NULL;
    mUtablename = NULL;
    mUcolumnnameC = NULL;
    mUcolumnname = NULL;
    mOrderbyC = NULL;
    mOrderby = NULL;
    mCondC = NULL;
    mCond = NULL;
    mMultiocc = NULL;
    mInfocol = NULL;
    iInfoGroup = NULL;
    mItablenameC = NULL;
    mItablename = NULL;
    mIcolumnnameC = NULL;
    mIcolumnname = NULL;
    mElemcondC = NULL;
    mElemcond = NULL;
////@end XMLElementDlg member initialisation

////@begin XMLElementDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLElementDlg creation
    return TRUE;
}

void XMLElementDlg::enable_rel(bool enable, bool not_recur)
// Enables controls describing JOIN of tables (or the top table in the top table node).
// For explicit SQL statement [enable] == false.
{ bool is_top=true;
  if (enable)  // unless explicit SQL
  {// find upper node with a table reference
    wxTreeItemId anc = dade->GetItemParent(dade->hItem);
    while (anc.IsOk())
    { if (anc==dade->GetRootItem()) break;  // must not call get_item_node() for the hidden root
      dad_element_node * anc_el = (dad_element_node*)get_item_node(dade, anc);
      if (anc_el->type()==DAD_NODE_TOP) break;
      if (*anc_el->lower_tab) 
      { is_top=false;
        break;
      }
      anc = dade->GetItemParent(anc);
    } 
  }
  mTablenameC->Enable(enable);               mTablename->Enable(enable);
  mUtablenameC->Enable(enable && !is_top);   mUtablename->Enable(enable && !is_top);
  mColumnnameC->Enable(enable && !is_top);   mColumnname->Enable(enable && !is_top);
  mUcolumnnameC->Enable(enable && !is_top);  mUcolumnname->Enable(enable && !is_top);
  mMultiocc->Enable(!enable);
  mElemcondC->Enable(!enable);               mElemcond->Enable(!enable);
  mRelationGroup->Enable(enable);
  mRecur->Enable(enable && !is_top);
  table_dependent_enabling(enable);
}

void XMLElementDlg::enable_infocol(bool enable)
{ mItablenameC->Enable(enable);   mItablename->Enable(enable);
  mIcolumnnameC->Enable(enable);  mIcolumnname->Enable(enable);
  iInfoGroup->Enable(enable);
}

void XMLElementDlg::table_dependent_enabling(bool joining)
{ bool enable = false;
  if (joining)
  { int ind = GetNewComboSelection(mTablename);  //mTablename->GetSelection();
    if (ind!=-1)
      enable = 0 != mTablename->GetString(ind).CmpNoCase(base_cursor_name);
  }
  mAliasC->Enable(enable);    mAlias->Enable(enable);
  mOrderbyC->Enable(enable);  mOrderby->Enable(enable);
  mCondC->Enable(enable);     mCond->Enable(enable); 
}

/*!
 * Control creation for XMLElementDlg
 */

void XMLElementDlg::CreateControls()
{    
////@begin XMLElementDlg content construction
    XMLElementDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Element name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mName = new wxTextCtrl;
    mName->Create( itemDialog1, CD_XML_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mName, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mRelation = new wxCheckBox;
    mRelation->Create( itemDialog1, CD_XML_RELATION, _("Join a new table here"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mRelation->SetValue(FALSE);
    itemBoxSizer2->Add(mRelation, 0, wxALIGN_LEFT|wxALL, 5);

    mRelationGroup = new wxStaticBox(itemDialog1, wxID_ANY, _("Joining the table"));
    wxStaticBoxSizer* itemStaticBoxSizer7 = new wxStaticBoxSizer(mRelationGroup, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer7, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mRecur = new wxCheckBox;
    mRecur->Create( itemDialog1, CD_XML_RECUR, _("Recursive occurrence with the same content model"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mRecur->SetValue(FALSE);
    itemStaticBoxSizer7->Add(mRecur, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer9 = new wxFlexGridSizer(7, 2, 0, 0);
    itemFlexGridSizer9->AddGrowableCol(1);
    itemStaticBoxSizer7->Add(itemFlexGridSizer9, 0, wxGROW|wxALL, 0);

    mTablenameC = new wxStaticText;
    mTablenameC->Create( itemDialog1, CD_XML_TABLENAME_C, _("Joined table:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mTablenameC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mTablenameStrings = NULL;
    mTablename = new wxComboBox;
    mTablename->Create( itemDialog1, CD_XML_TABLENAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTablenameStrings, wxCB_READONLY );
    itemFlexGridSizer9->Add(mTablename, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mAliasC = new wxStaticText;
    mAliasC->Create( itemDialog1, CD_XML_ALIAS_C, _("Alias:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mAliasC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mAlias = new wxTextCtrl;
    mAlias->Create( itemDialog1, CD_XML_ALIAS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mAlias, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mColumnnameC = new wxStaticText;
    mColumnnameC->Create( itemDialog1, CD_XML_COLUMNNAME_C, _("Column of joined table:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mColumnnameC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mColumnnameStrings = NULL;
    mColumnname = new wxComboBox;
    mColumnname->Create( itemDialog1, CD_XML_COLUMNNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mColumnnameStrings, wxCB_DROPDOWN );
    itemFlexGridSizer9->Add(mColumnname, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mUtablenameC = new wxStaticText;
    mUtablenameC->Create( itemDialog1, CD_XML_UTABLENAME_C, _("Upper table:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mUtablenameC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mUtablenameStrings = NULL;
    mUtablename = new wxComboBox;
    mUtablename->Create( itemDialog1, CD_XML_UTABLENAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mUtablenameStrings, wxCB_READONLY );
    itemFlexGridSizer9->Add(mUtablename, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mUcolumnnameC = new wxStaticText;
    mUcolumnnameC->Create( itemDialog1, CD_XML_UCOLUMNNAME_C, _("Column of parent table:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mUcolumnnameC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mUcolumnnameStrings = NULL;
    mUcolumnname = new wxComboBox;
    mUcolumnname->Create( itemDialog1, CD_XML_UCOLUMNNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mUcolumnnameStrings, wxCB_DROPDOWN );
    itemFlexGridSizer9->Add(mUcolumnname, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mOrderbyC = new wxStaticText;
    mOrderbyC->Create( itemDialog1, CD_XML_ORDERBY_C, _("Order by:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mOrderbyC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mOrderby = new wxTextCtrl;
    mOrderby->Create( itemDialog1, CD_XML_ORDERBY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mOrderby, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mCondC = new wxStaticText;
    mCondC->Create( itemDialog1, CD_XML_COND_C, _("Condition:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mCondC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mCond = new wxTextCtrl;
    mCond->Create( itemDialog1, CD_XML_COND, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer9->Add(mCond, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mMultiocc = new wxCheckBox;
    mMultiocc->Create( itemDialog1, CD_XML_MULTIOCC, _("Repeatable element"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mMultiocc->SetValue(FALSE);
    itemBoxSizer2->Add(mMultiocc, 0, wxALIGN_LEFT|wxALL, 5);

    mInfocol = new wxCheckBox;
    mInfocol->Create( itemDialog1, CD_XML_INFOCOL, _("Repeat on change of"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mInfocol->SetValue(FALSE);
    itemBoxSizer2->Add(mInfocol, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);

    iInfoGroup = new wxStaticBox(itemDialog1, wxID_ANY, _T(""));
    wxStaticBoxSizer* itemStaticBoxSizer26 = new wxStaticBoxSizer(iInfoGroup, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer26, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxFlexGridSizer* itemFlexGridSizer27 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer27->AddGrowableCol(1);
    itemStaticBoxSizer26->Add(itemFlexGridSizer27, 0, wxGROW|wxALL, 0);

    mItablenameC = new wxStaticText;
    mItablenameC->Create( itemDialog1, CD_XML_ITABLENAME_C, _("Table:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer27->Add(mItablenameC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mItablenameStrings = NULL;
    mItablename = new wxComboBox;
    mItablename->Create( itemDialog1, CD_XML_ITABLENAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mItablenameStrings, wxCB_READONLY );
    itemFlexGridSizer27->Add(mItablename, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mIcolumnnameC = new wxStaticText;
    mIcolumnnameC->Create( itemDialog1, CD_XML_ICOLUMNNAME_C, _("Column:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer27->Add(mIcolumnnameC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mIcolumnnameStrings = NULL;
    mIcolumnname = new wxComboBox;
    mIcolumnname->Create( itemDialog1, CD_XML_ICOLUMNNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mIcolumnnameStrings, wxCB_DROPDOWN );
    itemFlexGridSizer27->Add(mIcolumnname, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer32, 0, wxGROW|wxALL, 0);

    mElemcondC = new wxStaticText;
    mElemcondC->Create( itemDialog1, CD_XML_ELEMCOND_C, _("Element output condition:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(mElemcondC, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mElemcond = new wxTextCtrl;
    mElemcond->Create( itemDialog1, CD_XML_ELEMCOND, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(mElemcond, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer35, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton36 = new wxButton;
    itemButton36->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton36->SetDefault();
    itemBoxSizer35->Add(itemButton36, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton37 = new wxButton;
    itemButton37->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer35->Add(itemButton37, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton38 = new wxButton;
    itemButton38->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer35->Add(itemButton38, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLElementDlg content construction
    mName->SetMaxLength(sizeof(el->el_name)-1);
    mAlias->SetMaxLength(sizeof(el->lower_alias)-1);
    //mColumnname->SetMaxLength(sizeof(el->lower_col)-1);
    //mUcolumnname->SetMaxLength(sizeof(el->upper_col)-1);
    //mIcolumnname->SetMaxLength(sizeof(el->info_column.column_name)-1);
#if WBVERS<950
    mRecur->Hide();
#endif
   // set values:
    mName->SetValue(wxString(el->el_name, *cdp->sys_conv));
    mRecur->SetValue(el->recursive);
  //  mRelation->Enable(!el->recursive);
  //  mInfocol->Enable(!el->recursive);
    fill_table_names(dade->cdp, mTablename, dade->dad_tree->explicit_sql_statement && is_top_table_element(dade), el->lower_tab);
    mAlias->SetValue(wxString(el->lower_alias, *cdp->sys_conv));
    fill_column_names(dade, mTablename, NULL, NULL, mColumnname, NULL);
    mColumnname->SetValue(wxString(el->lower_col, *cdp->sys_conv));
    fill_my_table_names(dade, mUtablename, el->upper_tab, false);
    fill_column_names(dade, mUtablename, NULL, NULL, mUcolumnname, NULL);
    mUcolumnname->SetValue(wxString(el->upper_col, *cdp->sys_conv));
    if (*el->lower_tab || *el->lower_col || /**el->upper_tab ||*/ *el->upper_col)
      { mRelation->SetValue(true);   enable_rel(true, !el->recursive); }
    else
      { mRelation->SetValue(false);  enable_rel(false, !el->recursive); }

    fill_my_table_names(dade, mItablename, el->info_column.table_name, true);
    fill_column_names(dade, mItablename, NULL, NULL, mIcolumnname, el->info_column.column_name);
    if (el->has_info_column() || *el->info_column.table_name)
      { mInfocol->SetValue(true);  enable_infocol(true); }
    else
      { mInfocol->SetValue(false);  enable_infocol(false); }
    mMultiocc->SetValue(el->multi);
    mCond    ->SetValue(el->condition ? wxString(el->condition, *cdp->sys_conv) : wxGetEmptyString());
    mOrderby ->SetValue(el->order_by  ? wxString(el->order_by,  *cdp->sys_conv) : wxGetEmptyString());
    mElemcond->SetValue(el->elemcond  ? wxString(el->elemcond,  *cdp->sys_conv) : wxGetEmptyString());
   // disable most controls in the root element:
    if (el==dade->dad_tree->root_element)
      { mMultiocc->Disable();  mInfocol->Disable();  mCond->Disable();  mElemcond->Disable(); }

}

/*!
 * Should we show tooltips?
 */

bool XMLElementDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLElementDlg::OnOkClick( wxCommandEvent& event )
{// check values before overwriting anythig:
  if (mRelation->GetValue())
  { if (mColumnname->GetValue().Length()  >= sizeof(el->lower_col))
      { error_box(_("Text is too long!"));  mColumnname->SetFocus();   return; }
    if (mUcolumnname->GetValue().Length() >= sizeof(el->upper_col))
      { error_box(_("Text is too long!"));  mUcolumnname->SetFocus();  return; }
  }
  if (mInfocol->GetValue())
    if (mIcolumnname->GetValue().Length() >= sizeof(el->info_column.column_name))
      { error_box(_("Text is too long!"));  mIcolumnname->SetFocus();  return; }
 // data verified OK:
  strcpy(el->el_name, mName->GetValue().mb_str(*cdp->sys_conv));
  el->recursive = mRecur->GetValue();
 // RELATIONAL LINK: 
  *el->lower_tab=*el->lower_col=*el->upper_tab=*el->upper_col=0;
  int ind;  wxString str;
  if (mRelation->GetValue())
  { ind=mTablename->GetSelection();
    if (ind!=-1)
      strcpy(el->lower_tab, mTablename->GetString(ind).mb_str(*cdp->sys_conv));
    strcpy(el->lower_alias, mAlias->GetValue().mb_str(*cdp->sys_conv));
    strcpy(el->lower_col, mColumnname->GetValue().mb_str(*cdp->sys_conv));
    ind=mUtablename->GetSelection();
    if (ind!=-1)
      strcpy(el->upper_tab, mUtablename->GetString(ind).mb_str(*cdp->sys_conv));
    strcpy(el->upper_col, mUcolumnname->GetValue().mb_str(*cdp->sys_conv));
  }
 // info column:
  *el->info_column.table_name=*el->info_column.column_name=0;
  if (mInfocol->GetValue())
  { ind=mItablename->GetSelection();
    if (ind!=-1)
      strcpy(el->info_column.table_name, mItablename->GetString(ind).mb_str(*cdp->sys_conv));
    strcpy(el->info_column.column_name, mIcolumnname->GetValue().mb_str(*cdp->sys_conv));
  }
 // other data:
  el->multi = mMultiocc->GetValue();
 // column condition:
  if (el->condition) delete_chars(el->condition);
  el->condition=GetStringAlloc(cdp, mCond->GetValue());
 // element condition:
  if (el->elemcond) delete_chars(el->elemcond);
  el->elemcond=GetStringAlloc(cdp, mElemcond->GetValue());
 // order by:
  if (el->order_by) delete_chars(el->order_by);
  el->order_by=GetStringAlloc(cdp, mOrderby->GetValue());
 // OK, end the dialog:
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLElementDlg::OnCancelClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void XMLElementDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/xml_elementdlg.html"));
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_XML_TABLENAME
 */

void XMLElementDlg::OnCdXmlTablenameUpdated( wxCommandEvent& event )
{ wxString col = mColumnname->GetValue();  // save...
  fill_column_names(dade, mTablename, NULL, NULL, mColumnname, NULL);
  mColumnname->SetValue(col); // ...and restore the column name(s)
  table_dependent_enabling(mRelation->GetValue()/* && !mRecur->GetValue()*/);
  OnCdXmlAliasUpdated(event);  // new table list
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_XML_ALIAS
 */

void XMLElementDlg::OnCdXmlAliasUpdated( wxCommandEvent& event )
{ fill_my_table_names(dade, mItablename, el->info_column.table_name, false);
 // new local table must add by hand, it is not stored yet:
  int ind = GetNewComboSelection(mTablename); // mTablename->GetSelection();
  if (ind!=-1)
  { wxString table_name = mAlias->GetValue();
    table_name.Trim(true);  table_name.Trim(false);
    if (table_name.IsEmpty())
      table_name = mTablename->GetValue();
    mItablename->Append(table_name);
  }
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_XML_UTABLENAME
 */

void XMLElementDlg::OnCdXmlUtablenameUpdated( wxCommandEvent& event )
{ wxString col = mUcolumnname->GetValue();  // save...
  fill_column_names(dade, mUtablename, NULL, NULL, mUcolumnname, NULL);
  mUcolumnname->SetValue(col); // ...and restore the column name(s)
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_XML_ITABLENAME
 */

void XMLElementDlg::OnCdXmlItablenameUpdated( wxCommandEvent& event )
{ fill_column_names(dade, mItablename, NULL, NULL, mIcolumnname, el->info_column.column_name); }


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_XML_RELATION
 */

void XMLElementDlg::OnCdXmlRelationClick( wxCommandEvent& event )
{ enable_rel(mRelation->GetValue(), !mRecur->GetValue()); 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_XML_INFOCOL
 */

void XMLElementDlg::OnCdXmlInfocolClick( wxCommandEvent& event )
{ enable_infocol(mInfocol->GetValue()/* && !mRecur->GetValue()*/); }


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_XML_RECUR
 */

void XMLElementDlg::OnCdXmlRecurClick( wxCommandEvent& event )
{
//  enable_infocol(mInfocol->GetValue() && !mRecur->GetValue());
//  enable_rel(mRelation->GetValue(), !mRecur->GetValue()); 
//  mRelation->Enable(!mRecur->GetValue());
//  mInfocol->Enable(!mRecur->GetValue());
}



/*!
 * Get bitmap resources
 */

wxBitmap XMLElementDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLElementDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLElementDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLElementDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLElementDlg icon retrieval
    return wxNullIcon;
////@end XMLElementDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XML_KEYGEN
 */

void XMLElementDlg::OnCdXmlKeygenClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XML_KEYGEN in XMLElementDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XML_KEYGEN in XMLElementDlg. 
}


