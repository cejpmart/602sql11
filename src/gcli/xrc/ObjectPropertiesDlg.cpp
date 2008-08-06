/////////////////////////////////////////////////////////////////////////////
// Name:        ObjectPropertiesDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/20/04 09:07:54
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "ObjectPropertiesDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "ObjectPropertiesDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ObjectPropertiesDlg type definition
 */

IMPLEMENT_CLASS( ObjectPropertiesDlg, wxScrolledWindow )

/*!
 * ObjectPropertiesDlg event table definition
 */

BEGIN_EVENT_TABLE( ObjectPropertiesDlg, wxScrolledWindow )

////@begin ObjectPropertiesDlg event table entries
    EVT_CHECKBOX( CD_OBJPROP_EXPORT, ObjectPropertiesDlg::OnCdObjpropExportClick )

    EVT_CHECKBOX( CD_OBJPROP_EXPDATA, ObjectPropertiesDlg::OnCdObjpropExpdataClick )

    EVT_CHECKBOX( CD_OBJPROP_PROTECT, ObjectPropertiesDlg::OnCdObjpropProtectClick )

    EVT_BUTTON( wxID_HELP, ObjectPropertiesDlg::OnHelpClick )

////@end ObjectPropertiesDlg event table entries

END_EVENT_TABLE()

/*!
 * ObjectPropertiesDlg constructors
 */

ObjectPropertiesDlg::ObjectPropertiesDlg( )
{
}

/*!
 * ObjectPropertiesDlg creator
 */

bool ObjectPropertiesDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ObjectPropertiesDlg member initialisation
    mCapt = NULL;
    mExport = NULL;
    mExpData = NULL;
    mProtect = NULL;
////@end ObjectPropertiesDlg member initialisation
#ifdef WINS  // probably not necessary on GTK, and cannot be called before Create() 
   // change the backgroud colour so that it is the same as in dialogs:
    wxVisualAttributes va = wxDialog::GetClassDefaultAttributes();
    SetBackgroundColour(va.colBg);
#endif
////@begin ObjectPropertiesDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxScrolledWindow::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ObjectPropertiesDlg creation
   // start the functionality of scrollbars:
    FitInside();  
    SetScrollRate(8, 8);
    return TRUE;
}

/*!
 * Control creation for ObjectPropertiesDlg
 */

void ObjectPropertiesDlg::CreateControls()
{    
////@begin ObjectPropertiesDlg content construction

    ObjectPropertiesDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxStaticText* item3 = new wxStaticText;
    item3->Create( item1, CD_OBJPROP_CAPT, _("Export object with application"), wxDefaultPosition, wxDefaultSize, 0 );
    mCapt = item3;
    item2->Add(item3, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxCheckBox* item4 = new wxCheckBox;
    item4->Create( item1, CD_OBJPROP_EXPORT, _("Export object with application"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
    mExport = item4;
    item4->SetValue(FALSE);
    item2->Add(item4, 0, wxGROW|wxALL, 5);

    wxCheckBox* item5 = new wxCheckBox;
    item5->Create( item1, CD_OBJPROP_EXPDATA, _("Export data with application"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
    mExpData = item5;
    item5->SetValue(FALSE);
    item2->Add(item5, 0, wxGROW|wxALL, 5);

    wxCheckBox* item6 = new wxCheckBox;
    item6->Create( item1, CD_OBJPROP_PROTECT, _("Protected object"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
    mProtect = item6;
    item6->SetValue(FALSE);
    item2->Add(item6, 0, wxGROW|wxALL, 5);

    wxButton* item7 = new wxButton;
    item7->Create( item1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item2->Add(item7, 0, wxALIGN_LEFT|wxALL, 5);

////@end ObjectPropertiesDlg content construction
  mExport ->SetWindowStyle(wxCHK_3STATE);  // does not work, must set above!!
  mExpData->SetWindowStyle(wxCHK_3STATE);
  mProtect->SetWindowStyle(wxCHK_3STATE);
}

/*!
 * Should we show tooltips?
 */

bool ObjectPropertiesDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_OBJPROP_EXPORT
 */

void ObjectPropertiesDlg::OnCdObjpropExportClick( wxCommandEvent& event )
{
  bool checked = mExport->GetValue();  // user cannot set the undeterminate value
  MyFrame * frame = wxGetApp().frame;
  ControlPanelList * objlist = frame->control_panel->objlist;
  int index = -1;
  do
  { index=objlist->GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (index==-1) break;
    item_info info = *objlist->m_BranchInfo;
    objlist->add_list_item_info(index, info);
   // write the new value:
    Change_component_flag(info.cdp, info.category, info.schema_name, info.object_name, info.objnum, CO_FLAG_NOEXPORT, checked ? 0 : CO_FLAG_NOEXPORT);
  } while (true);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_OBJPROP_EXPDATA
 */

void ObjectPropertiesDlg::OnCdObjpropExpdataClick( wxCommandEvent& event )
{
  bool checked = mExpData->GetValue();  // user cannot set the undeterminate value
  MyFrame * frame = wxGetApp().frame;
  ControlPanelList * objlist = frame->control_panel->objlist;
  int index = -1;
  do
  { index=objlist->GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (index==-1) break;
    item_info info = *objlist->m_BranchInfo;
    objlist->add_list_item_info(index, info);
   // write the new value:
    if (info.category==CATEG_CURSOR || info.category==CATEG_PROC)
      cd_Admin_mode(info.cdp, info.objnum, checked);
    else 
      Change_component_flag(info.cdp, info.category, info.schema_name, info.object_name, info.objnum, CO_FLAG_NOEXPORTD, checked ? 0 : CO_FLAG_NOEXPORTD);
  } while (true);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_OBJPROP_PROTECT
 */

void ObjectPropertiesDlg::OnCdObjpropProtectClick( wxCommandEvent& event )
{
  bool checked = mProtect->GetValue();  // user cannot set the undeterminate value
  MyFrame * frame = wxGetApp().frame;
  ControlPanelList * objlist = frame->control_panel->objlist;
  int index = -1;
  do
  { index=objlist->GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (index==-1) break;
    item_info info = *objlist->m_BranchInfo;
    objlist->add_list_item_info(index, info);
   // write the new value:
    Change_component_flag(info.cdp, info.category, info.schema_name, info.object_name, info.objnum, CO_FLAG_PROTECT, checked ? CO_FLAG_PROTECT : 0);
  } while (true);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void ObjectPropertiesDlg::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/output.html"));
}


