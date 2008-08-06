/////////////////////////////////////////////////////////////////////////////
// Name:        ObjectPrivilsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/27/04 07:42:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ObjectPrivilsDlg.h"
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

#include "ObjectPrivilsDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ObjectPrivilsDlg type definition
 */

IMPLEMENT_CLASS( ObjectPrivilsDlg, wxDialog )

/*!
 * ObjectPrivilsDlg event table definition
 */

BEGIN_EVENT_TABLE( ObjectPrivilsDlg, wxDialog )

////@begin ObjectPrivilsDlg event table entries
    EVT_RADIOBUTTON( CD_PRIV_USER, ObjectPrivilsDlg::OnCdPrivUserSelected )

    EVT_RADIOBUTTON( CD_PRIV_GROUP, ObjectPrivilsDlg::OnCdPrivGroupSelected )

    EVT_RADIOBUTTON( CD_PRIV_ROLE, ObjectPrivilsDlg::OnCdPrivRoleSelected )

    EVT_COMBOBOX( CD_PRIV_SUBJECT, ObjectPrivilsDlg::OnCdPrivSubjectSelected )

    EVT_BUTTON( CD_PRIV_MEMB1, ObjectPrivilsDlg::OnCdPrivMemb1Click )

    EVT_BUTTON( CD_PRIV_MEMB2, ObjectPrivilsDlg::OnCdPrivMemb2Click )

    EVT_LIST_ITEM_SELECTED( CD_PRIV_LIST, ObjectPrivilsDlg::OnCdPrivListSelected )
    EVT_LIST_ITEM_DESELECTED( CD_PRIV_LIST, ObjectPrivilsDlg::OnCdPrivListDeselected )

    EVT_BUTTON( CD_PRIV_GRANT, ObjectPrivilsDlg::OnCdPrivGrantClick )

    EVT_BUTTON( CD_PRIV_REVOKE, ObjectPrivilsDlg::OnCdPrivRevokeClick )

    EVT_BUTTON( CD_PRIV_GRANT_REC, ObjectPrivilsDlg::OnCdPrivGrantRecClick )

    EVT_BUTTON( CD_PRIV_REVOKE_REC, ObjectPrivilsDlg::OnCdPrivRevokeRecClick )

    EVT_BUTTON( CD_PRIV_SEL_READ, ObjectPrivilsDlg::OnCdPrivSelReadClick )

    EVT_BUTTON( CD_PRIV_SEL_WRITE, ObjectPrivilsDlg::OnCdPrivSelWriteClick )

    EVT_BUTTON( wxID_CANCEL, ObjectPrivilsDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, ObjectPrivilsDlg::OnHelpClick )

////@end ObjectPrivilsDlg event table entries

END_EVENT_TABLE()

/*!
 * ObjectPrivilsDlg constructors
 */

ObjectPrivilsDlg::ObjectPrivilsDlg(t_privil_info & piIn) : pi(piIn)
{  }

/*!
 * ObjectPrivilsDlg creator
 */

bool ObjectPrivilsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ObjectPrivilsDlg member initialisation
    mUser = NULL;
    mGroup = NULL;
    mRole = NULL;
    mSubject = NULL;
    mMemb1 = NULL;
    mMemb2 = NULL;
    mSizerContainingList = NULL;
    mList = NULL;
    mGrant = NULL;
    mRevoke = NULL;
    mGrantRec = NULL;
    mRevokeRec = NULL;
    mSelRead = NULL;
    mSelWrite = NULL;
////@end ObjectPrivilsDlg member initialisation

////@begin ObjectPrivilsDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ObjectPrivilsDlg creation
  if (pi.record_privs && !pi.sys_priv && !pi.obj_priv)
    ;//SetSize(630, 340);//mList->SetSizeHints(500, 100);
  else
    { mGrantRec->Hide();  mRevokeRec->Hide(); }
  if (pi.sys_priv || pi.obj_priv)
    { mSelRead->Hide();   mSelWrite->Hide(); }
  return TRUE;
}

/*!
 * Control creation for ObjectPrivilsDlg
 */
int create_list(wxListCtrl * mList, t_privil_info & pi);

void ObjectPrivilsDlg::CreateControls()
{    
////@begin ObjectPrivilsDlg content construction
    ObjectPrivilsDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Subject of the privileges"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer3->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    mUser = new wxRadioButton;
    mUser->Create( itemDialog1, CD_PRIV_USER, _("User"), wxDefaultPosition, wxDefaultSize, 0 );
    mUser->SetValue(FALSE);
    itemBoxSizer5->Add(mUser, 0, wxALIGN_LEFT|wxALL, 5);

    mGroup = new wxRadioButton;
    mGroup->Create( itemDialog1, CD_PRIV_GROUP, _("Group"), wxDefaultPosition, wxDefaultSize, 0 );
    mGroup->SetValue(FALSE);
    itemBoxSizer5->Add(mGroup, 0, wxALIGN_LEFT|wxALL, 5);

    mRole = new wxRadioButton;
    mRole->Create( itemDialog1, CD_PRIV_ROLE, _("Role"), wxDefaultPosition, wxDefaultSize, 0 );
    mRole->SetValue(FALSE);
    itemBoxSizer5->Add(mRole, 0, wxALIGN_LEFT|wxALL, 5);

    wxString* mSubjectStrings = NULL;
    mSubject = new wxOwnerDrawnComboBox;
    mSubject->Create( itemDialog1, CD_PRIV_SUBJECT, _T(""), wxDefaultPosition, wxSize(180, -1), 0, mSubjectStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer4->Add(mSubject, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer3->Add(itemBoxSizer10, 0, wxGROW|wxALL, 0);

    mMemb1 = new wxButton;
    mMemb1->Create( itemDialog1, CD_PRIV_MEMB1, _("Groups containing the user"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(mMemb1, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    mMemb2 = new wxButton;
    mMemb2->Create( itemDialog1, CD_PRIV_MEMB2, _("Groups containing the user"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(mMemb2, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    mSizerContainingList = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(mSizerContainingList, 1, wxGROW|wxALL, 5);

    mList = new wxListCtrl;
    mList->Create( itemDialog1, CD_PRIV_LIST, wxDefaultPosition, wxSize(100, 100), wxLC_REPORT|wxLC_HRULES|wxLC_VRULES );
    mSizerContainingList->Add(mList, 1, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer15 = new wxBoxSizer(wxVERTICAL);
    mSizerContainingList->Add(itemBoxSizer15, 0, wxGROW|wxALL, 0);

    mGrant = new wxButton;
    mGrant->Create( itemDialog1, CD_PRIV_GRANT, _("Grant"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(mGrant, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    mRevoke = new wxButton;
    mRevoke->Create( itemDialog1, CD_PRIV_REVOKE, _("Revoke"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(mRevoke, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    mGrantRec = new wxButton;
    mGrantRec->Create( itemDialog1, CD_PRIV_GRANT_REC, _("Grant for record"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(mGrantRec, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    mRevokeRec = new wxButton;
    mRevokeRec->Create( itemDialog1, CD_PRIV_REVOKE_REC, _("Revoke for record"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(mRevokeRec, 0, wxGROW|wxLEFT, 5);

    itemBoxSizer15->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 2);

    mSelRead = new wxButton;
    mSelRead->Create( itemDialog1, CD_PRIV_SEL_READ, _("Select all read"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(mSelRead, 0, wxGROW|wxLEFT|wxTOP|wxBOTTOM, 5);

    mSelWrite = new wxButton;
    mSelWrite->Create( itemDialog1, CD_PRIV_SEL_WRITE, _("Select all write"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(mSelWrite, 0, wxGROW|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer23 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer23, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton24 = new wxButton;
    itemButton24->Create( itemDialog1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer23->Add(itemButton24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton25 = new wxButton;
    itemButton25->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer23->Add(itemButton25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ObjectPrivilsDlg content construction
 // caption:
  if (pi.obj_priv)
    if (!pi.multioper)
    { tobjname name;  wxString capt;
      cd_Read(pi.cdp, pi.tbs[0], *pi.recnums, OBJ_NAME_ATR, NULL, name);
      wb_small_string(pi.cdp, name, true);
      capt.Printf(pi.tbs[0]==TAB_TABLENUM ? _("Privileges to Table %s") : _("Privileges of Object %s"), wxString(name, *pi.cdp->sys_conv).c_str());
      SetTitle(capt);
    }
    else
      SetTitle(_("Privileges of the Selected Group of Objects"));
  else if (pi.sys_priv)
    SetTitle(pi.tbs[0]==TAB_TABLENUM  ? _("Privilege to Create New Tables") :
             pi.tbs[0]==OBJ_TABLENUM  ? _("Privilege to Create New Objects") :
             pi.tbs[0]==USER_TABLENUM ? _("Privilege to Create New Users and Groups") :
             _("System Privileges"));
  else   // data
    if (pi.multioper!=MULTITAB)
    { tobjname name;  wxString capt;
      cd_Read(pi.cdp, TAB_TABLENUM, pi.tbs[0], OBJ_NAME_ATR, NULL, name);
      wb_small_string(pi.cdp, name, true);
      capt.Printf(_("Data Privileges on Table %s"), wxString(name, *pi.cdp->sys_conv).c_str());
#if 0  // useless
      if (pi.record_privs && pi.multioper!=MULTIREC)
      { wxString recnum;
        recnum.Printf(_(" (record %u)"), pi.recnums[0]);
        capt = capt + recnum;
      }
#endif
      SetTitle(capt);
    }
    else 
      SetTitle(_("Global Data Privileges on a Group of Tables"));
 // privilege list:
  int w = create_list(mList, pi);
  mSizerContainingList->SetItemMinSize(mList, w, -1);

 // disable role subjects for system tables:
  if (pi.sys_priv)
  { mRole->Disable();
    mMemb2->Disable();
  }
  mGrant->Disable();
  mRevoke->Disable();
  mGrantRec->Disable();
  mRevokeRec->Disable();
 // preselect subject:
  mUser->SetValue(true);
  pi.set_subject_category(CATEG_USER, mSubject, mMemb1, mMemb2, mList);
 // preselect the connected user:
  tobjname user_name;
  cd_Read(pi.cdp, USER_TABLENUM, pi.cdp->logged_as_user, OBJ_NAME_ATR, NULL, user_name);
  int pos = mSubject->FindString(wxString(user_name, *pi.cdp->sys_conv));
  if (pos!=-1) mSubject->SetSelection(pos);
  pi.load_state(mSubject);
  pi.show_state(mList);
}

#define CAPT_MARG 16

int create_list(wxListCtrl * mList, t_privil_info & pi)
{ int row, h, w, totw;
 // columns in the list:
  wxListItem itemCol;
  itemCol.m_mask   = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT | wxLIST_MASK_WIDTH;
  itemCol.m_format = wxLIST_FORMAT_LEFT;
  itemCol.m_text   = _("Privilege");
  mList->GetTextExtent(itemCol.m_text, &w, &h);
  itemCol.m_width  = 3*w;
  totw=0;   // will be added later
  mList->InsertColumn(0, itemCol);
  if (!pi.record_privs || pi.sys_priv || pi.obj_priv)
  { itemCol.m_text   = _("Granted");
    mList->GetTextExtent(itemCol.m_text, &w, &h);
    itemCol.m_width = w+CAPT_MARG;
    totw+=itemCol.m_width;
    mList->InsertColumn(1, itemCol);
    itemCol.m_text   = _("Effective");
    mList->GetTextExtent(itemCol.m_text, &w, &h);
    itemCol.m_width = w+CAPT_MARG;
    totw+=itemCol.m_width;
    mList->InsertColumn(2, itemCol);
  }
  else
  { itemCol.m_text   = _("Table granted");
    mList->GetTextExtent(itemCol.m_text, &w, &h);
    itemCol.m_width = w+CAPT_MARG;
    totw+=itemCol.m_width;
    mList->InsertColumn(1, itemCol);
    itemCol.m_text   = _("Table effective");
    mList->GetTextExtent(itemCol.m_text, &w, &h);
    itemCol.m_width = w+CAPT_MARG;
    totw+=itemCol.m_width;
    mList->InsertColumn(2, itemCol);
    itemCol.m_text   = _("Record granted");
    mList->GetTextExtent(itemCol.m_text, &w, &h);
    itemCol.m_width = w+CAPT_MARG;
    totw+=itemCol.m_width;
    mList->InsertColumn(3, itemCol);
    itemCol.m_text   = _("Record effective");
    mList->GetTextExtent(itemCol.m_text, &w, &h);
    itemCol.m_width = w+CAPT_MARG;
    totw+=itemCol.m_width;
    mList->InsertColumn(4, itemCol);
  }
 // rows in the list:
  wxString tx;
  if (pi.sys_priv)
    mList->InsertItem(0, _("Create new objects"));
  else if (pi.obj_priv)
  { mList->InsertItem(0, _("Use the object"));
    mList->InsertItem(1, _("Edit the design"));
    mList->InsertItem(2, _("Delete the object"));
    mList->InsertItem(3, _("Grant privileges"));
  }
  else  // data privileges
  { mList->InsertItem(0, _("Insert new records"));
    mList->InsertItem(1, _("Delete records"));
    mList->InsertItem(2, _("Grant privileges"));
    row=3;
    if (pi.multioper!=MULTITAB)
    { const d_table * td= cd_get_table_d(pi.cdp, pi.tbs[0], CATEG_TABLE);
      if (td!=NULL)
      { for (int i=pi.atrbase;  i<td->attrcnt;  i++)
        { char name[ATTRNAMELEN+1];
          strcpy(name, td->attribs[i].name);  wb_small_string(pi.cdp, name, false);
          tx = wxString(_("Read column " ))+wxString(name, *pi.cdp->sys_conv);
          mList->InsertItem(row++, tx);
          tx = wxString(_("Update column "))+wxString(name, *pi.cdp->sys_conv);
          mList->InsertItem(row++, tx);
        }
        release_table_d(td);
      }
    }
    pi.new_priv_start=row;
    if (pi.new_privs)
    { mList->InsertItem(row++, _("Common reading of new records"));
      mList->InsertItem(row++, _("Common updating of new records"));
      mList->InsertItem(row++, _("Common deleting of new records"));
    }
  }
  mList->SetColumnWidth(0, wxLIST_AUTOSIZE);
  totw += mList->GetColumnWidth(0); 
  return totw+wxSystemSettings::GetMetric(wxSYS_VSCROLL_X)+4;
}

/*!
 * Should we show tooltips?
 */

bool ObjectPrivilsDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_PRIV_USER
 */

void ObjectPrivilsDlg::OnCdPrivUserSelected( wxCommandEvent& event )
{ 
  pi.set_subject_category(CATEG_USER, mSubject, mMemb1, mMemb2, mList);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_PRIV_GROUP
 */

void ObjectPrivilsDlg::OnCdPrivGroupSelected( wxCommandEvent& event )
{ 
  pi.set_subject_category(CATEG_GROUP, mSubject, mMemb1, mMemb2, mList);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_PRIV_ROLE
 */

void ObjectPrivilsDlg::OnCdPrivRoleSelected( wxCommandEvent& event )
{ 
  pi.set_subject_category(CATEG_ROLE, mSubject, mMemb1, mMemb2, mList);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_MEMB1
 */

void ObjectPrivilsDlg::OnCdPrivMemb1Click( wxCommandEvent& event )
{
  pi.set_relation(false);
  if (pi.load_state(mSubject)) cd_Signalize(pi.cdp);
  pi.show_state(mList);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_MEMB2
 */

void ObjectPrivilsDlg::OnCdPrivMemb2Click( wxCommandEvent& event )
{
  pi.set_relation(true);
  if (pi.load_state(mSubject)) cd_Signalize(pi.cdp);
  pi.show_state(mList);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_GRANT
 */

void ObjectPrivilsDlg::OnCdPrivGrantClick( wxCommandEvent& event )
{
  pi.update_privils(mList, true, false);
  if (pi.load_state(mSubject)) cd_Signalize(pi.cdp);
  pi.show_state(mList);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_REVOKE
 */

void ObjectPrivilsDlg::OnCdPrivRevokeClick( wxCommandEvent& event )
{
  pi.update_privils(mList, false, false);
  if (pi.load_state(mSubject)) cd_Signalize(pi.cdp);
  pi.show_state(mList);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_GRANT_REC
 */

void ObjectPrivilsDlg::OnCdPrivGrantRecClick( wxCommandEvent& event )
{
  pi.update_privils(mList, true, true);
  if (pi.load_state(mSubject)) cd_Signalize(pi.cdp);
  pi.show_state(mList);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_REVOKE_REC
 */

void ObjectPrivilsDlg::OnCdPrivRevokeRecClick( wxCommandEvent& event )
{
  pi.update_privils(mList, false, true);
  if (pi.load_state(mSubject)) cd_Signalize(pi.cdp);
  pi.show_state(mList);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void ObjectPrivilsDlg::OnCancelClick( wxCommandEvent& event )
{
  EndModal(wxID_CLOSE);
}



/*!
 * wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for CD_PRIV_LIST
 */

void ObjectPrivilsDlg::OnCdPrivListSelected( wxListEvent& event )
{
  bool enable = mList->GetSelectedItemCount() > 0;
  mGrant->Enable(enable);
  mRevoke->Enable(enable);
  mGrantRec->Enable(enable);
  mRevokeRec->Enable(enable);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_LIST_ITEM_DESELECTED event handler for CD_PRIV_LIST
 */

void ObjectPrivilsDlg::OnCdPrivListDeselected( wxListEvent& event )
{
  bool enable = mList->GetSelectedItemCount() > 0;
  mGrant->Enable(enable);
  mRevoke->Enable(enable);
  mGrantRec->Enable(enable);
  mRevokeRec->Enable(enable);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void ObjectPrivilsDlg::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(
    pi.sys_priv ? wxT("xml/html/rightsystab.html") : 
    pi.obj_priv ? wxT("xml/html/rightobj.html") : // must be tested before record_privs
    pi.record_privs ? wxT("xml/html/rightrecdat.html") : 
    wxT("xml/html/rightdat.html") ); 
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_SEL_READ
 */

void ObjectPrivilsDlg::OnCdPrivSelReadClick( wxCommandEvent& event )
{
   int cnt = mList->GetItemCount();
   if (pi.new_privs) cnt-=3;
   for (int row=3;  row<cnt;  row+=2)
     mList->SetItemState(row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
   mList->SetFocus();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_SEL_WRITE
 */

void ObjectPrivilsDlg::OnCdPrivSelWriteClick( wxCommandEvent& event )
{
   int cnt = mList->GetItemCount();
   if (pi.new_privs) cnt-=3;
   for (int row=4;  row<cnt;  row+=2)
     mList->SetItemState(row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
   mList->SetFocus();
}



/*!
 * Get bitmap resources
 */

wxBitmap ObjectPrivilsDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ObjectPrivilsDlg bitmap retrieval
    return wxNullBitmap;
////@end ObjectPrivilsDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ObjectPrivilsDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ObjectPrivilsDlg icon retrieval
    return wxNullIcon;
////@end ObjectPrivilsDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_PRIV_SUBJECT
 */

void ObjectPrivilsDlg::OnCdPrivSubjectSelected( wxCommandEvent& event )
{
  pi.load_state(mSubject);
  pi.show_state(mList);
  event.Skip();
}

