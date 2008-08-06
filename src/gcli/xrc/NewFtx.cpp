/////////////////////////////////////////////////////////////////////////////
// Name:        NewFtx.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/07/05 16:22:02
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "NewFtx.h"
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

#include "NewFtx.h"

////@begin XPM images
////@end XPM images

/*!
 * NewFtx type definition
 */

IMPLEMENT_CLASS( NewFtx, wxDialog )

/*!
 * NewFtx event table definition
 */

BEGIN_EVENT_TABLE( NewFtx, wxDialog )

////@begin NewFtx event table entries
    EVT_BUTTON( wxID_OK, NewFtx::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, NewFtx::OnCancelClick )

////@end NewFtx event table entries

END_EVENT_TABLE()

/*!
 * NewFtx constructors
 */

NewFtx::NewFtx(cdp_t cdpIn, t_fulltext * ftdefIn)
{ cdp=cdpIn;  ftdef = ftdefIn; }

/*!
 * NewFtx creator
 */

bool NewFtx::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NewFtx member initialisation
    mName = NULL;
    mLanguage = NULL;
    mBasic = NULL;
    mSubstruct = NULL;
    mExternal = NULL;
    mBigIntId = NULL;
////@end NewFtx member initialisation

////@begin NewFtx creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end NewFtx creation
    return TRUE;
}

/*!
 * Control creation for NewFtx
 */

void NewFtx::CreateControls()
{    
////@begin NewFtx content construction
    NewFtx* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mName = new wxTextCtrl;
    mName->Create( itemDialog1, CD_FTD_NAME, _T(""), wxDefaultPosition, wxSize(180, -1), 0 );
    if (ShowToolTips())
        mName->SetToolTip(_("The name cannot have more than 20 characters. It will be used as a suffix to names of all auxiliary objects in the fulltext system."));
    itemFlexGridSizer3->Add(mName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, wxID_STATIC, _("Language:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mLanguageStrings = NULL;
    mLanguage = new wxOwnerDrawnComboBox;
    mLanguage->Create( itemDialog1, CD_FTD_LANG, _T(""), wxDefaultPosition, wxSize(180, -1), 0, mLanguageStrings, wxCB_READONLY );
    if (ShowToolTips())
        mLanguage->SetToolTip(_("Without specifying the language, the words could not be converted to their basic form and the Unicode text could not be translated to the proper code page."));
    itemFlexGridSizer3->Add(mLanguage, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mBasic = new wxCheckBox;
    mBasic->Create( itemDialog1, CD_FTD_BASIC, _("Convert words to basic form"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mBasic->SetValue(FALSE);
    if (ShowToolTips())
        mBasic->SetToolTip(_("When checked, the fulltext system does not differentiate among various forms of the word (e.g. the singular and plural form)."));
    itemBoxSizer2->Add(mBasic, 0, wxALIGN_LEFT|wxALL, 5);

    mSubstruct = new wxCheckBox;
    mSubstruct->Create( itemDialog1, CD_FTD_SUBSTRUCT, _("Index documents inside archives (e.g. ZIP)"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mSubstruct->SetValue(FALSE);
    if (ShowToolTips())
        mSubstruct->SetToolTip(_("When checked, indexing a compound document creates auxiliary information about its internal structure."));
    itemBoxSizer2->Add(mSubstruct, 0, wxALIGN_LEFT|wxALL, 5);

    mExternal = new wxCheckBox;
    mExternal->Create( itemDialog1, CD_FTD_EXTERN, _("Store the index in an external file"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mExternal->SetValue(FALSE);
    if (ShowToolTips())
        mExternal->SetToolTip(_("If not checked, the index will be stored in the database."));
    itemBoxSizer2->Add(mExternal, 0, wxGROW|wxALL, 5);

    mBigIntId = new wxCheckBox;
    mBigIntId->Create( itemDialog1, CD_FTD_BIGINT_ID, _("Document ID is BigInt (64-bit]"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mBigIntId->SetValue(FALSE);
    mBigIntId->SetHelpText(_("Document ID is Integer (32-bit) if not checked"));
    if (ShowToolTips())
        mBigIntId->SetToolTip(_("Document ID is Integer (32-bit) if not checked"));
    itemBoxSizer2->Add(mBigIntId, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton13->SetDefault();
    itemBoxSizer12->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton14 = new wxButton;
    itemButton14->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end NewFtx content construction
    mName->SetMaxLength(OBJ_NAME_LEN-(int)strlen("FTX_WORDTAB"));
    mBasic->SetValue(ftdef->basic_form);
    mSubstruct->SetValue(ftdef->with_substructures);
    mExternal->SetValue(ftdef->separated);
    mBigIntId->SetValue(ftdef->bigint_id);
    int i=0;
    while (ft_lang_name[i])
    { mLanguage->Append(wxGetTranslation(wxString(ft_lang_name[i], *wxConvCurrent)), (void*)(size_t)i);
      i++;
    }
    //int i;
    //for (i=0;  i<mLanguage->GetCount();  i++)
    //  if ((int)mLanguage->GetClientData(i) == ftdef->language) break;
    mLanguage->SetSelection(0);
#if WBVERS<950
    mSubstruct->Hide();
#endif
#ifndef EXTFT
    mExternal->Hide();
#endif
}

/*!
 * Should we show tooltips?
 */

bool NewFtx::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void NewFtx::OnOkClick( wxCommandEvent& event )
// Verifies the names and creates the system
{ tobjnum objnum;
 // move data from controls to the variables:
  strcpy(ftdef->name, mName->GetValue().Trim(false).Trim(true).mb_str(*cdp->sys_conv));
  if (!*ftdef->name)
    { error_box(_("Object name must be specified"), this);  mName->SetFocus();  return; }
  if (!cd_Find2_object(cdp, ftdef->name, NULL, CATEG_INFO, &objnum))
    { unpos_box(OBJ_NAME_USED, this);  mName->SetFocus();  return; }
  ftdef->basic_form = mBasic->GetValue();
  ftdef->with_substructures = mSubstruct->GetValue();
  ftdef->separated = mExternal->GetValue();
  ftdef->bigint_id = mBigIntId->GetValue();
  int sel_lang = mLanguage->GetSelection();
  ftdef->language = (int)(size_t)mLanguage->GetClientData(sel_lang);
  *ftdef->schema=0;
 // try creating the fulltext system:
  char * source = ftdef->to_source(cdp, false);
  if (!source) return;
  if (cd_SQL_execute(cdp, source, NULL))
    { cd_Signalize2(cdp, this);  return; }
  event.Skip();  // FT created
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void NewFtx::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap NewFtx::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NewFtx bitmap retrieval
    return wxNullBitmap;
////@end NewFtx bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NewFtx::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NewFtx icon retrieval
    return wxNullIcon;
////@end NewFtx icon retrieval
}
