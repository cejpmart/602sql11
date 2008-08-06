/////////////////////////////////////////////////////////////////////////////
// Name:        NewRoutineDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/16/04 17:11:00
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "NewRoutineDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "NewRoutineDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * NewRoutineDlg type definition
 */

IMPLEMENT_CLASS( NewRoutineDlg, wxDialog )

/*!
 * NewRoutineDlg event table definition
 */

BEGIN_EVENT_TABLE( NewRoutineDlg, wxDialog )

////@begin NewRoutineDlg event table entries
    EVT_RADIOBOX( CD_CREPROC_TYPE, NewRoutineDlg::OnCdCreprocTypeSelected )

////@end NewRoutineDlg event table entries

END_EVENT_TABLE()

/*!
 * NewRoutineDlg constructors
 */

NewRoutineDlg::NewRoutineDlg(cdp_t cdpIn )
{ cdp=cdpIn;
}

NewRoutineDlg::NewRoutineDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{ 
  Create(parent, id, caption, pos, size, style);
}

/*!
 * NewRoutineDlg creator
 */

bool NewRoutineDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NewRoutineDlg member initialisation
    mType = NULL;
    mNameCapt = NULL;
    mName = NULL;
    mOK = NULL;
////@end NewRoutineDlg member initialisation

////@begin NewRoutineDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end NewRoutineDlg creation
    return TRUE;
}

/*!
 * Control creation for NewRoutineDlg
 */

void NewRoutineDlg::CreateControls()
{    
////@begin NewRoutineDlg content construction

    NewRoutineDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxString item3Strings[] = {
        _("Create a module containing global declarations"),
        _("Create a stored procedure on the SQL Server"),
        _("Create a stored function on the SQL Server")
    };
    wxRadioBox* item3 = new wxRadioBox;
    item3->Create( item1, CD_CREPROC_TYPE, _("Select module type"), wxDefaultPosition, wxDefaultSize, 3, item3Strings, 1, wxRA_SPECIFY_COLS );
    mType = item3;
    item2->Add(item3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* item4 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item4, 0, wxGROW|wxALL, 0);

    item4->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* item6 = new wxStaticText;
    item6->Create( item1, CD_CREPROC_CAPT, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    mNameCapt = item6;
    item4->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item7 = new wxTextCtrl;
    item7->Create( item1, CD_CREPROC_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mName = item7;
    item4->Add(item7, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* item8 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item8, 0, wxGROW|wxALL, 0);

    item8->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item10 = new wxButton;
    item10->Create( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    mOK = item10;
    item10->SetDefault();
    item8->Add(item10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    item8->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item12 = new wxButton;
    item12->Create( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item8->Add(item12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    item8->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end NewRoutineDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool NewRoutineDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_CREPROC_TYPE
 */

void NewRoutineDlg::OnCdCreprocTypeSelected( wxCommandEvent& event )
{
  if (mType->GetSelection())
  { mName->Enable();
    mNameCapt->Enable();
    mName->SetFocus();
  }
  else
  { mName->Disable();
    mNameCapt->Disable();
    mOK->SetFocus();
  }
  event.Skip();
}

bool NewRoutineDlg::TransferDataToWindow(void)
// Inits the dialog: PROCEDURE is selected by default.
{ mName->SetMaxLength(OBJ_NAME_LEN);
 // if MODULE_GLOBALS exists then disable the option to create it:
  tobjnum objnum;
  if (!cd_Find2_object(cdp, MODULE_GLOBAL_DECLS, NULL, CATEG_PROC, &objnum))
    mType->Enable(0, false);
 // initial selection:
  mType->SetSelection(1);
  mName->SetFocus();  // without this the 1st selecting of CD_CRE_GLOBDECL does not generate an event!!
  return true; 
}

bool NewRoutineDlg::TransferDataFromWindow(void)
{ return true; }

bool NewRoutineDlg::Validate(void)
// does TransferDataFromWindow too.
{ 
  objtype = mType->GetSelection();
  if (objtype==0)
    strcpy(objname, MODULE_GLOBAL_DECLS);
  else
  { wxString name = mName->GetValue();
    name.Trim(false);  name.Trim(true);
    if (name.IsEmpty())
      { wxBell();  mName->SetFocus();  return false; }
    if (!convertible_name(cdp, this, name))
      return false;
    strcpy(objname, name.mb_str(*cdp->sys_conv));
    tobjnum objnum;
    if (!cd_Find2_object(cdp, objname, NULL, CATEG_PROC, &objnum))
      { error_box(_("An object with the same name and category already exists."), this);  return false; }
  }
  return true;
}



