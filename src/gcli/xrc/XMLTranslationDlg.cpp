/////////////////////////////////////////////////////////////////////////////
// Name:        XMLTranslationDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/25/05 10:14:57
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "XMLTranslationDlg.h"
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

#include "XMLTranslationDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLTranslationDlg type definition
 */

IMPLEMENT_CLASS( XMLTranslationDlg, wxDialog )

/*!
 * XMLTranslationDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLTranslationDlg, wxDialog )

////@begin XMLTranslationDlg event table entries
    EVT_BUTTON( CD_XMLTRANS_IMP_NEW, XMLTranslationDlg::OnCdXmltransImpNewClick )

    EVT_BUTTON( CD_XMLTRANS_EXP_NEW, XMLTranslationDlg::OnCdXmltransExpNewClick )

    EVT_BUTTON( wxID_OK, XMLTranslationDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLTranslationDlg::OnCancelClick )

////@end XMLTranslationDlg event table entries

END_EVENT_TABLE()

/*!
 * XMLTranslationDlg constructors
 */

XMLTranslationDlg::XMLTranslationDlg(cdp_t cdpIn, char * inconv_fncIn, char * outconv_fncIn)
{ cdp=cdpIn; 
  inconv_fnc=inconv_fncIn;  outconv_fnc=outconv_fncIn;
}

/*!
 * XMLTranslationDlg creator
 */

bool XMLTranslationDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLTranslationDlg member initialisation
    mImpFnc = NULL;
    mImpNew = NULL;
    mExpFnc = NULL;
    mExpNew = NULL;
////@end XMLTranslationDlg member initialisation

////@begin XMLTranslationDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLTranslationDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLTranslationDlg
 */

bool SetStringSelection2(wxOwnerDrawnComboBox * ctrl, wxString str)
{ int ind = ctrl->FindString(str);
  if (ind==wxNOT_FOUND) return false;
  ctrl->Select(ind);
  ctrl->SetStringSelection(str);
  return true;
}

void XMLTranslationDlg::CreateControls()
{    
////@begin XMLTranslationDlg content construction
    XMLTranslationDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("SQL function to translate XML string to database values:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 1, wxGROW|wxALL, 0);

    wxString* mImpFncStrings = NULL;
    mImpFnc = new wxOwnerDrawnComboBox;
    mImpFnc->Create( itemDialog1, CD_XMLTRANS_IMP, _T(""), wxDefaultPosition, wxDefaultSize, 0, mImpFncStrings, wxCB_READONLY );
    itemBoxSizer4->Add(mImpFnc, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mImpNew = new wxButton;
    mImpNew->Create( itemDialog1, CD_XMLTRANS_IMP_NEW, _("Create New"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(mImpNew, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("SQL function to translate database values to XML string:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText7, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer8, 1, wxGROW|wxALL, 0);

    wxString* mExpFncStrings = NULL;
    mExpFnc = new wxOwnerDrawnComboBox;
    mExpFnc->Create( itemDialog1, CD_XMLTRANS_EXP, _T(""), wxDefaultPosition, wxDefaultSize, 0, mExpFncStrings, wxCB_READONLY );
    itemBoxSizer8->Add(mExpFnc, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mExpNew = new wxButton;
    mExpNew->Create( itemDialog1, CD_XMLTRANS_EXP_NEW, _("Create New"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(mExpNew, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton12->SetDefault();
    itemBoxSizer11->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLTranslationDlg content construction
  fill();
  //##if (!*inconv_fnc)  mImpFnc->SetSelection(0);  else SetStringSelection2(mImpFnc, wxString(inconv_fnc,  *cdp->sys_conv));
  //##if (!*outconv_fnc) mExpFnc->SetSelection(0);  else SetStringSelection2(mExpFnc, wxString(outconv_fnc, *cdp->sys_conv));
  mImpFnc->SetValue(wxString(inconv_fnc,  *cdp->sys_conv));
  mExpFnc->SetValue(wxString(outconv_fnc, *cdp->sys_conv));
}

void XMLTranslationDlg::fill(void)
{
 // save the original selection:
  //##wxString imp_sel, exp_sel;
  //##if (mImpFnc->GetSelection()!=-1)  // GetStringSelection fails on GTK when there is no selection!
  //##  imp_sel = mImpFnc->GetStringSelection();
  //##if (mExpFnc->GetSelection()!=-1)
  //##  exp_sel = mExpFnc->GetStringSelection();
 // refill the lists:
  mImpFnc->Clear();
  mExpFnc->Clear();
  mImpFnc->Append(wxEmptyString);
  mExpFnc->Append(wxEmptyString);
  char query[200+OBJ_NAME_LEN];  tcursnum cursnum;  trecnum cnt, rec;
  sprintf(query, "SELECT * FROM _IV_PROCEDURE_PARAMETERS WHERE SCHEMA_NAME='%s' ORDER BY PROCEDURE_NAME, ORDINAL_POSITION", cdp->sel_appl_name);
  if (!cd_Open_cursor_direct(cdp, query, &cursnum))
  { cd_Rec_cnt(cdp, cursnum, &cnt);
    tobjname procname, procname1;  int ok_params = 0, type, specif;  *procname=0;
    for (rec=0;  rec<cnt;  rec++)
    { cd_Read(cdp, cursnum, rec, 2, NULL, procname1);
      if (!strcmp(procname, procname1))  // continuing parameters if the same routine
      { if (ok_params>=2) continue;  // has too many parameters or had a wrong parameter
      }
      else  // a new routine
      { if (ok_params==2)  // previous routine has 2 parameters, both are ok
        { mImpFnc->Append(wxString(procname, *cdp->sys_conv));
          mExpFnc->Append(wxString(procname, *cdp->sys_conv));
        }
        ok_params=0;
        strcpy(procname, procname1);
      }
     // analysing the type of the return value or parameter:
      cd_Read(cdp, cursnum, rec, 5, NULL, (char*)&type);
      cd_Read(cdp, cursnum, rec, 9, NULL, (char*)&specif);
      if (type==ATT_STRING && t_specif(specif).wide_char==1)
        ok_params++;  // another OK parameter
      else 
        ok_params=3;  // has a wrong parameter
    }
    if (ok_params==2)  // previous routine has 2 parameters, both are ok
    { mImpFnc->Append(wxString(procname, *cdp->sys_conv));
      mExpFnc->Append(wxString(procname, *cdp->sys_conv));
    }
    cd_Close_cursor(cdp, cursnum);
  }
 // reselect the original values:
  //##if (imp_sel.IsEmpty()) mImpFnc->SetSelection(0);  else SetStringSelection2(mImpFnc, imp_sel);
  //##if (exp_sel.IsEmpty()) mExpFnc->SetSelection(0);  else SetStringSelection2(mExpFnc, exp_sel);
}

wxString XMLTranslationDlg::create_new_function(void)
// Ask for the the name, creates a template function, adds it to the control panel, refills both combols.
// Retuns the function name or empty string, if cancelled.
{ tobjname procname;  tobjnum objnum;
  *procname = 0;
  if (!get_name_for_new_object(cdp, this, cdp->sel_appl_name, CATEG_PROC, _("Name of the new function"), procname, false))
    return wxEmptyString;
  Upcase9(cdp, procname);
 // create the pattern:
  wxString initial_def;
  initial_def.Printf(wxT("FUNCTION `%s`(IN InputValue NCHAR(2045)) RETURNS NCHAR(2045);\r\nBEGIN\r\n  { <statement> }\r\n  RETURN InputValue;\r\nEND\r\n\r\n"), wxString(procname, *cdp->sys_conv).c_str());
 // insert the new object:
  if (cd_Insert_object(cdp, procname, CATEG_PROC, &objnum))
    return wxEmptyString;
  cd_Write_var(cdp, OBJ_TABLENUM, objnum, OBJ_DEF_ATR, NOINDEX, 0, (int)strlen(initial_def.mb_str(*cdp->sys_conv)), initial_def.mb_str(*cdp->sys_conv));
  uns32 stamp = stamp_now();
  cd_Write(cdp, OBJ_TABLENUM, objnum, OBJ_MODIF_ATR,  NULL, &stamp, sizeof(uns32));
#ifndef LINUX  // on GTK the modal dialog blocks the editor
  open_text_object_editor(cdp, OBJ_TABLENUM, objnum, OBJ_DEF_ATR, CATEG_PROC, NULLSTRING, OPEN_IN_EDCONT);
#endif
 // add to CP:
  Add_new_component(cdp, CATEG_PROC, cdp->sel_appl_name, procname, objnum, 0, "", false);
  //##fill();
  return wxString(procname, *cdp->sys_conv);
}

/*!
 * Should we show tooltips?
 */

bool XMLTranslationDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLTRANS_IMP_NEW
 */

void XMLTranslationDlg::OnCdXmltransImpNewClick( wxCommandEvent& event )
{
  wxString fnc = create_new_function();
  if (!fnc.IsEmpty())  // select the new function, if created
  { mImpFnc->Append(fnc);
    //##SetStringSelection2(mImpFnc, fnc);
    mImpFnc->SetValue(fnc);
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLTRANS_EXP_NEW
 */

void XMLTranslationDlg::OnCdXmltransExpNewClick( wxCommandEvent& event )
{
  wxString fnc = create_new_function();
  if (!fnc.IsEmpty())  // select the new function, if created
  { mExpFnc->Append(fnc);
    //##SetStringSelection2(mExpFnc, fnc);
    mExpFnc->SetValue(fnc);
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLTranslationDlg::OnOkClick( wxCommandEvent& event )
{
 // save selected values (GetStringSelection fails on GTK when there is no selection!):
  //##strcpy(inconv_fnc,  mImpFnc->GetSelection()==-1 ? "" : mImpFnc->GetStringSelection().mb_str(*cdp->sys_conv));
  //##strcpy(outconv_fnc, mExpFnc->GetSelection()==-1 ? "" : mExpFnc->GetStringSelection().mb_str(*cdp->sys_conv));
  strcpy(inconv_fnc,  mImpFnc->GetValue().mb_str(*cdp->sys_conv));
  strcpy(outconv_fnc, mExpFnc->GetValue().mb_str(*cdp->sys_conv));
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLTranslationDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap XMLTranslationDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLTranslationDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLTranslationDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLTranslationDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLTranslationDlg icon retrieval
    return wxNullIcon;
////@end XMLTranslationDlg icon retrieval
}
