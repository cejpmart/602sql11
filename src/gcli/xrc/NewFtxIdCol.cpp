/////////////////////////////////////////////////////////////////////////////
// Name:        NewFtxIdCol.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/15/05 11:50:05
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "NewFtxIdCol.h"
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

#include "NewFtxIdCol.h"

////@begin XPM images
////@end XPM images

/*!
 * NewFtxIdCol type definition
 */

IMPLEMENT_CLASS( NewFtxIdCol, wxDialog )

/*!
 * NewFtxIdCol event table definition
 */

BEGIN_EVENT_TABLE( NewFtxIdCol, wxDialog )

////@begin NewFtxIdCol event table entries
    EVT_BUTTON( wxID_OK, NewFtxIdCol::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, NewFtxIdCol::OnCancelClick )

////@end NewFtxIdCol event table entries

END_EVENT_TABLE()

/*!
 * NewFtxIdCol constructors
 */

NewFtxIdCol::NewFtxIdCol(cdp_t cdpIn, ttablenum tbnumIn, const char * ftx_nameIn)
{ cdp=cdpIn;  tbnum=tbnumIn;
  strcpy(ftx_name, ftx_nameIn);
  ta=NULL;
}

NewFtxIdCol::~NewFtxIdCol(void)
{ if (ta) delete ta; }

/*!
 * NewFtxIdCol creator
 */

bool NewFtxIdCol::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NewFtxIdCol member initialisation
    mCombo = NULL;
    mOK = NULL;
////@end NewFtxIdCol member initialisation

////@begin NewFtxIdCol creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end NewFtxIdCol creation
    return TRUE;
}

/*!
 * Control creation for NewFtxIdCol
 */

void NewFtxIdCol::CreateControls()
{    
////@begin NewFtxIdCol content construction
    NewFtxIdCol* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("Enter the new column name or select an existing column."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Column name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mComboStrings = NULL;
    mCombo = new wxOwnerDrawnComboBox;
    mCombo->Create( itemDialog1, CD_FTXCOL_COMBO, _T(""), wxDefaultPosition, wxDefaultSize, 0, mComboStrings, wxCB_DROPDOWN );
    itemBoxSizer4->Add(mCombo, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mOK = new wxButton;
    mOK->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    mOK->SetDefault();
    itemBoxSizer7->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end NewFtxIdCol content construction
    ta=create_ta_for_ftx(cdp, tbnum, ftx_name);
    if (ta)
    { for (int a=1;  a<ta->attrs.count();  a++)
      { atr_all * att = ta->attrs.acc0(a);
        if (att->type==ATT_INT32 || att->type==ATT_INT64)
          if (att->orig_type != 3)
            mCombo->Append(wxString(att->name, *cdp->sys_conv), (void*)(size_t)a);
      }
    }
    mCombo->SetFocus();
}

/*!
 * Should we show tooltips?
 */

bool NewFtxIdCol::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void NewFtxIdCol::OnOkClick( wxCommandEvent& event )
{ char colname[ATTRNAMELEN+1];  tobjname seq_name;
  sprintf(seq_name, "FTX_DOCID%s", ftx_name);
  if (!ta) return;  // internal error
  newcolname = mCombo->GetValue();
  newcolname.Trim(false);  newcolname.Trim(true);  newcolname.MakeUpper();  // column name cannot be found if not converted to upper case
  strmaxcpy(colname, newcolname.mb_str(*cdp->sys_conv), sizeof(colname));
  if (!*colname) { wxBell();  return; }
  int sel = GetNewComboSelection(mCombo);  //mCombo->GetSelection(); -- is wrong!
  int data;
  if (sel!=wxNOT_FOUND)
    data = (int)(size_t)mCombo->GetClientData(sel);
  char comm[200+4*OBJ_NAME_LEN];
  strcpy(comm, "ALTER TABLE ");
  { t_flstr fullname;
    make_full_table_name(cdp, fullname, ta->selfname, ta->schemaname);
    strcat(comm, fullname.get());
  }
  if (sel==wxNOT_FOUND)
    sprintf(comm+strlen(comm), " ADD `%s` INTEGER DEFAULT %s.nextval UNIQUE", colname, seq_name);
  else
  { 
#if 0  
    sprintf(comm+strlen(comm), " ALTER `%s` INTEGER", ta->attrs.acc(data)->name);
    //if (!(ta->attrs.acc(data)->orig_type & 2))  -- must re-specify
      sprintf(comm+strlen(comm), " DEFAULT `%s`.nextval", seq_name);
    if (!(ta->attrs.acc(data)->orig_type & 1))
      strcat(comm, " UNIQUE");
#else  // new syntax preserving the values in the ID column
    bool flag=false;
    if (!(ta->attrs.acc(data)->orig_type & 6))  // preserving any default value
    { sprintf(comm+strlen(comm), " ALTER `%s` SET DEFAULT ", ta->attrs.acc(data)->name);
      sprintf(comm+strlen(comm), "`%s`.nextval", seq_name);
      flag=true;
    }  
    if (!(ta->attrs.acc(data)->orig_type & 1))
    { if (flag) strcat(comm, ",");
      sprintf(comm+strlen(comm), " ADD UNIQUE (`%s`) ", ta->attrs.acc(data)->name);
    }  
#endif      
  }
  uns32 optval;
  cd_Get_sql_option(cdp, &optval);
  cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, 0);  // new ALTER TABLE selected
  BOOL res = cd_SQL_execute(cdp, comm, NULL);
  if (res)
    cd_Signalize2(cdp, this);  
  cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, optval);  // restored
  inval_table_d(cdp, tbnum, CATEG_TABLE);
  if (res) return;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void NewFtxIdCol::OnCancelClick( wxCommandEvent& event )
{
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap NewFtxIdCol::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NewFtxIdCol bitmap retrieval
    return wxNullBitmap;
////@end NewFtxIdCol bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NewFtxIdCol::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NewFtxIdCol icon retrieval
    return wxNullIcon;
////@end NewFtxIdCol icon retrieval
}
