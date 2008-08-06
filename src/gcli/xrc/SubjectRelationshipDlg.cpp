/////////////////////////////////////////////////////////////////////////////
// Name:        SubjectRelationshipDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/16/04 13:04:04
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "SubjectRelationshipDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "SubjectRelationshipDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * SubjectRelationshipDlg type definition
 */

IMPLEMENT_CLASS( SubjectRelationshipDlg, wxDialog )

/*!
 * SubjectRelationshipDlg event table definition
 */

BEGIN_EVENT_TABLE( SubjectRelationshipDlg, wxDialog )

////@begin SubjectRelationshipDlg event table entries
////@end SubjectRelationshipDlg event table entries

END_EVENT_TABLE()

/*!
 * SubjectRelationshipDlg creator
 */

bool SubjectRelationshipDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SubjectRelationshipDlg member initialisation
////@end SubjectRelationshipDlg member initialisation

////@begin SubjectRelationshipDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SubjectRelationshipDlg creation
    return TRUE;
}

/*!
 * Control creation for SubjectRelationshipDlg
 */

void SubjectRelationshipDlg::CreateControls()
{    
////@begin SubjectRelationshipDlg content construction

    SubjectRelationshipDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxStaticText* item3 = new wxStaticText;
    item3->Create( item1, CD_SUBJ_REL_CAPT, _T(""), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
    item2->Add(item3, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxString* item4Strings = NULL;
    wxCheckListBox* item4 = new wxCheckListBox;
    item4->Create( item1, CD_SUBJ_REL_LIST, wxDefaultPosition, wxSize(340, 200), 0, item4Strings, 0 );
    item2->Add(item4, 1, wxGROW|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* item5 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item5, 0, wxGROW|wxALL, 0);

    item5->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item7 = new wxButton;
    item7->Create( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item7->SetDefault();
    item5->Add(item7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    item5->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item9 = new wxButton;
    item9->Create( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->Add(item9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    item5->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);
////@end SubjectRelationshipDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool SubjectRelationshipDlg::ShowToolTips()
{
    return TRUE;
}

bool SubjectRelationshipDlg::TransferDataToWindow(void)
{
 // create the title text:
  tobjname name;
  if (cd_Read(cdp, subject1==CATEG_ROLE ? OBJ_TABLENUM : USER_TABLENUM, subjnum, OBJ_NAME_ATR, NULL, name))
    { cd_Signalize(cdp);  strcpy(name, "?"); }
  const wxChar * patt;
  switch (subject1)
  { case CATEG_USER:
      patt = subject2==CATEG_GROUP ? _("Groups containing user %s:") : _("Roles containing user %s:");
      rel1in2=true;
      break;
    case CATEG_GROUP:
      if (subject2==CATEG_USER)
        { patt=_("Users in group %s:");  rel1in2=false; }
      else
        { patt=_("Roles containing group %s:");  rel1in2=true;  }
      break;
    case CATEG_ROLE:
      patt = subject2==CATEG_USER  ? _("Users in role %s:")  : _("Groups in role %s:");
      rel1in2=false;
      break;
    default:
      patt=wxEmptyString;  break;
  }
  wxString title;
  title.Printf(patt, wxString(name, *cdp->sys_conv).c_str());
  FindWindow(CD_SUBJ_REL_CAPT)->SetLabel(title);
 // fill subjects:
  wxCheckListBox * clb = (wxCheckListBox *)FindWindow(CD_SUBJ_REL_LIST);
  int ind = 0;  // counter 
  wxBeginBusyCursor();
  void * en = get_object_enumerator(cdp, subject2, NULL);
  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
  while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
  { if (subject2==CATEG_ROLE && *name>='0' && *name<='9') continue;  // deleted role
    //if (objnum==DB_ADMIN_GROUP && subject1==CATEG_ROLE) continue; // groups in role: do not list DB_ADMIN -- no more, it confuses users and J.S.
    //    small_string(src, TRUE);
    uns32 relation;
    if (rel1in2)
      cd_GetSet_group_role(cdp, subjnum, objnum, subject2, OPER_GET, &relation);
    else
      cd_GetSet_group_role(cdp, objnum, subjnum, subject1, OPER_GET, &relation);
    //clb->Append(name, (void*)relation);  // original state stored in the client data -- not allowed!
    clb->Append(wxString(name, *cdp->sys_conv));
    clb->Check(ind++, relation!=0);
  }
  free_object_enumerator(en);
  wxEndBusyCursor();
  return true;
}

bool SubjectRelationshipDlg::TransferDataFromWindow(void)
{ tobjname name;  uns32 orig_state, relation;
  wxCheckListBox * clb = (wxCheckListBox *)FindWindow(CD_SUBJ_REL_LIST);
  int index, count = clb->GetCount();
  BOOL res=FALSE;
  cd_Start_transaction(cdp);
 // adding:
  relation = TRUE;
  for (index=0;  index<count;  index++)
  { strcpy(name, clb->GetString(index).mb_str(*cdp->sys_conv));
    if (clb->IsChecked(index))
    { clb->SetSelection(index);
      tobjnum the_subject, objnum, super_subject;  tcateg super_categ;
      if (cd_Find2_object(cdp, name, NULL, subject2, &objnum))
        { cd_Signalize(cdp);  cd_Roll_back(cdp);  return false; }
      if (rel1in2)
        { the_subject=subjnum;  super_subject=objnum;   super_categ=subject2; }
      else
        { the_subject=objnum;   super_subject=subjnum;  super_categ=subject1; }
      if (!cd_GetSet_group_role(cdp, the_subject, super_subject, super_categ, OPER_GET, &orig_state) &&
          orig_state!=relation)
        if (cd_GetSet_group_role(cdp, the_subject, super_subject, super_categ, OPER_SET, &relation))
          { cd_Signalize(cdp);  cd_Roll_back(cdp);  return false; }
    }
  }
 // removing (must be after adding, may remove itself from adding privil)
  relation = FALSE;
  for (index=0;  index<count;  index++)
  { strcpy(name, clb->GetString(index).mb_str(*cdp->sys_conv));
    if (!clb->IsChecked(index))
    { clb->SetSelection(index);
      tobjnum the_subject, objnum, super_subject;  tcateg super_categ;
      if (cd_Find2_object(cdp, name, NULL, subject2, &objnum))
        { cd_Signalize(cdp);  cd_Roll_back(cdp);  return false; }
      if (rel1in2)
        { the_subject=subjnum;  super_subject=objnum;   super_categ=subject2; }
      else
        { the_subject=objnum;   super_subject=subjnum;  super_categ=subject1; }
      if (!cd_GetSet_group_role(cdp, the_subject, super_subject, super_categ, OPER_GET, &orig_state) &&
          orig_state!=relation)
      { if (super_categ==CATEG_GROUP)
        { if (super_subject==CONF_ADM_GROUP)
            if (wxMessageDialog(wxGetApp().frame, 
              _("You are removing a user from the CONFIG_ADMIN group.\nAre you sure that there is at least one other configuration administrator?"),
              STR_WARNING_CAPTION, wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION).ShowModal() != wxID_YES) continue;
          if (super_subject==SECUR_ADM_GROUP)
            if (wxMessageDialog(wxGetApp().frame, 
              _("You are removing a user from the SECURITY_ADMIN group.\nAre you sure that there is at least one other security administrator?"),
              STR_WARNING_CAPTION, wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION).ShowModal() != wxID_YES) continue;
        }
        if (cd_GetSet_group_role(cdp, the_subject, super_subject, super_categ, OPER_SET, &relation))
          { cd_Signalize(cdp);  cd_Roll_back(cdp);  return false; }
      }
    }
  }
  cd_Commit(cdp);
  return true;
}

