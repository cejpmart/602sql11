/////////////////////////////////////////////////////////////////////////////
// Name:        DomainDesignerDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/18/04 09:30:18
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "DomainDesignerDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "DomainDesignerDlg.h"
/* The logic:
On start, dom.tp is translated to type index and specif.length to characters. Reverse translation is not performed, make_source is avare of it.
Current data are in the controls. 
  The exceptions are dom.tp and dom.specif.precision which get the current value on every change in the combo,
  because show_type_specif() needs the actual value to be in the variables.
*/
////@begin XPM images
////@end XPM images

/*!
 * DomainDesignerDlg type definition
 */

IMPLEMENT_CLASS( DomainDesignerDlg, wxDialog )

/*!
 * DomainDesignerDlg event table definition
 */

BEGIN_EVENT_TABLE( DomainDesignerDlg, wxDialog )

////@begin DomainDesignerDlg event table entries
    EVT_COMBOBOX( CD_DOM_TYPE, DomainDesignerDlg::OnCdDomTypeSelected )

    EVT_CHECKBOX( CD_DOM_NULL, DomainDesignerDlg::OnCdDomNullClick )

    EVT_COMBOBOX( CD_DOM_BYTELENGTH, DomainDesignerDlg::OnCdDomBytelengthSelected )

    EVT_SPINCTRL( CD_DOM_LENGTH, DomainDesignerDlg::OnCdDomLengthUpdated )
    EVT_TEXT( CD_DOM_LENGTH, DomainDesignerDlg::OnCdDomLengthTextUpdated )

    EVT_CHECKBOX( CD_DOM_UNICODE, DomainDesignerDlg::OnCdDomUnicodeClick )

    EVT_CHECKBOX( CD_DOM_IGNORE, DomainDesignerDlg::OnCdDomIgnoreClick )

    EVT_COMBOBOX( CD_DOM_COLL, DomainDesignerDlg::OnCdDomCollSelected )

    EVT_TEXT( CD_DOM_DEFAULT, DomainDesignerDlg::OnCdDomDefaultUpdated )

    EVT_TEXT( CD_DOM_CHECK, DomainDesignerDlg::OnCdDomCheckUpdated )

    EVT_COMBOBOX( CD_DOM_DEFER, DomainDesignerDlg::OnCdDomDeferSelected )

    EVT_BUTTON( wxID_OK, DomainDesignerDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, DomainDesignerDlg::OnCancelClick )

    EVT_BUTTON( CD_DOM_VALIDATE, DomainDesignerDlg::OnCdDomValidateClick )

    EVT_BUTTON( CD_DOM_SQL, DomainDesignerDlg::OnCdDomSqlClick )

////@end DomainDesignerDlg event table entries
    EVT_ACTIVATE(DomainDesignerDlg::OnActivate)
    EVT_CLOSE(DomainDesignerDlg::OnCloseWindow)
    EVT_KEY_DOWN(DomainDesignerDlg::OnKeyDown)
#if WBVERS>=1000
    EVT_MENU(-1, DomainDesignerDlg::OnxCommand)
#endif
END_EVENT_TABLE()

/*!
 * DomainDesignerDlg constructors
 */

DomainDesignerDlg::DomainDesignerDlg(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn) :
  any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, CATEG_DOMAIN, _domain_xpm, &domdsng_coords)
   { feedback_disabled=false; }

DomainDesignerDlg::~DomainDesignerDlg(void)
{ if (global_style==ST_POPUP) 
    remove_tools();
}

void DomainDesignerDlg::set_designer_caption(void)
{ wxString caption;
  if (!modifying()) caption = _("Design a New Domain");
  else caption.Printf(_("Edit Design of Domain %s"), wxString(dom.domname, *cdp->sys_conv).c_str());
  SetTitle(caption);
}

bool DomainDesignerDlg::open(wxWindow * parent, t_global_style my_style)
{ if (modifying())
  { tptr defin=cd_Load_objdef(cdp, objnum, CATEG_DOMAIN);
    if (!defin) { cd_Signalize(cdp);  return false; }
    if (!compile_domain(cdp, defin, &dom))
      { corefree(defin);  cd_Signalize(cdp);  return false; }
    corefree(defin);
    dom.tp = convert_type_for_edit(wb_type_list, dom.tp, dom.specif);
  }
  else
  { dom.domname[0]=dom.schema[0]=0;
    dom.tp=1;  // 1st type after group header
  }
  if (!Create(parent, SYMBOL_DOMAINDESIGNERDLG_IDNAME, SYMBOL_DOMAINDESIGNERDLG_TITLE, persistent_coords->get_pos())) return false;
  competing_frame::focus_receiver = this;
  //TransferDataToWindow();  -- for dialog it is not necessary to call it explicity, will be called soon in OnInitDialog
  return true;
}

void DomainDesignerDlg::Activated(void)
{ if (global_style==ST_POPUP)  // otherwise is modal
  { add_tools();
    write_status_message();
  }
}  

/*!
 * DomainDesignerDlg creator
 */

bool DomainDesignerDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DomainDesignerDlg member initialisation
    mTopSizer = NULL;
    mTB = NULL;
    mType = NULL;
    mNull = NULL;
    mByteLengthCapt = NULL;
    mByteLength = NULL;
    mLengthCapt = NULL;
    mLength = NULL;
    mUnicode = NULL;
    mIgnore = NULL;
    mCollCapt = NULL;
    mColl = NULL;
    mDefaultCapt = NULL;
    mDefault = NULL;
    mCheck = NULL;
    mDeferCapt = NULL;
    mDefer = NULL;
    mButtons = NULL;
////@end DomainDesignerDlg member initialisation

////@begin DomainDesignerDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end DomainDesignerDlg creation
    return TRUE;
}

/*!
 * Control creation for DomainDesignerDlg
 */

void DomainDesignerDlg::CreateControls()
{    
////@begin DomainDesignerDlg content construction
    DomainDesignerDlg* itemDialog1 = this;

    mTopSizer = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mTopSizer);

    mTB = new wxToolBar;
    mTB->Create( itemDialog1, CD_DOM_TB, wxDefaultPosition, wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxTB_NODIVIDER|wxNO_BORDER );
    mTB->SetToolSeparation(3);
    mTB->Realize();
    mTopSizer->Add(mTB, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticLine* itemStaticLine4 = new wxStaticLine;
    itemStaticLine4->Create( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    mTopSizer->Add(itemStaticLine4, 0, wxGROW|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer5 = new wxFlexGridSizer(10, 2, 0, 0);
    itemFlexGridSizer5->AddGrowableCol(1);
    mTopSizer->Add(itemFlexGridSizer5, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, wxID_STATIC, _("&Type:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText6, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mTypeStrings = NULL;
    mType = new wxOwnerDrawnComboBox;
    mType->Create( itemDialog1, CD_DOM_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTypeStrings, wxCB_READONLY );
    itemFlexGridSizer5->Add(mType, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemDialog1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mNull = new wxCheckBox;
    mNull->Create( itemDialog1, CD_DOM_NULL, _("&NULL value enabled"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mNull->SetValue(FALSE);
    itemFlexGridSizer5->Add(mNull, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mByteLengthCapt = new wxStaticText;
    mByteLengthCapt->Create( itemDialog1, wxID_STATIC, _("&Extent:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(mByteLengthCapt, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString mByteLengthStrings[] = {
        _("1 byte"),
        _("2 bytes"),
        _("4 bytes"),
        _("8 bytes")
    };
    mByteLength = new wxOwnerDrawnComboBox;
    mByteLength->Create( itemDialog1, CD_DOM_BYTELENGTH, _T(""), wxDefaultPosition, wxDefaultSize, 4, mByteLengthStrings, wxCB_READONLY );
    itemFlexGridSizer5->Add(mByteLength, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mLengthCapt = new wxStaticText;
    mLengthCapt->Create( itemDialog1, CD_DOM_LENGTH_CAPT, _("&Length:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(mLengthCapt, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLength = new wxSpinCtrl;
    mLength->Create( itemDialog1, CD_DOM_LENGTH, _("0"), wxDefaultPosition, wxSize(86, -1), wxSP_ARROW_KEYS, 0, 100, 0 );
    itemFlexGridSizer5->Add(mLength, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText14 = new wxStaticText;
    itemStaticText14->Create( itemDialog1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText14, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer15 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer5->Add(itemBoxSizer15, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    mUnicode = new wxCheckBox;
    mUnicode->Create( itemDialog1, CD_DOM_UNICODE, _("&Unicode"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mUnicode->SetValue(FALSE);
    itemBoxSizer15->Add(mUnicode, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mIgnore = new wxCheckBox;
    mIgnore->Create( itemDialog1, CD_DOM_IGNORE, _("&Ignore case"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mIgnore->SetValue(FALSE);
    itemBoxSizer15->Add(mIgnore, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mCollCapt = new wxStaticText;
    mCollCapt->Create( itemDialog1, CD_DOM_COLL_CAPT, _("&Collation and charset:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(mCollCapt, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mCollStrings = NULL;
    mColl = new wxOwnerDrawnComboBox;
    mColl->Create( itemDialog1, CD_DOM_COLL, _T(""), wxDefaultPosition, wxDefaultSize, 0, mCollStrings, wxCB_READONLY );
    itemFlexGridSizer5->Add(mColl, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDefaultCapt = new wxStaticText;
    mDefaultCapt->Create( itemDialog1, CD_DOM_DEFAULT_CAPT, _("&Default value:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(mDefaultCapt, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mDefault = new wxTextCtrl;
    mDefault->Create( itemDialog1, CD_DOM_DEFAULT, _T(""), wxDefaultPosition, wxSize(180, -1), 0 );
    itemFlexGridSizer5->Add(mDefault, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText22 = new wxStaticText;
    itemStaticText22->Create( itemDialog1, wxID_STATIC, _("C&heck condition:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText22, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mCheck = new wxTextCtrl;
    mCheck->Create( itemDialog1, CD_DOM_CHECK, _T(""), wxDefaultPosition, wxSize(180, -1), 0 );
    itemFlexGridSizer5->Add(mCheck, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDeferCapt = new wxStaticText;
    mDeferCapt->Create( itemDialog1, CD_DOM_DEFER_CAPT, _("Check mode:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(mDeferCapt, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString mDeferStrings[] = {
        _("Not specified"),
        _("Deferred"),
        _("Immediate")
    };
    mDefer = new wxOwnerDrawnComboBox;
    mDefer->Create( itemDialog1, CD_DOM_DEFER, _T(""), wxDefaultPosition, wxDefaultSize, 3, mDeferStrings, wxCB_READONLY );
    itemFlexGridSizer5->Add(mDefer, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mButtons = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(mButtons, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton27 = new wxButton;
    itemButton27->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(itemButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton28 = new wxButton;
    itemButton28->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(itemButton28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton29 = new wxButton;
    itemButton29->Create( itemDialog1, CD_DOM_VALIDATE, _("Validate"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(itemButton29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton30 = new wxButton;
    itemButton30->Create( itemDialog1, CD_DOM_SQL, _("Show SQL"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(itemButton30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end DomainDesignerDlg content construction
    mByteLength->SetSelection(2);
//#ifdef WINS  // repairing error in spin control in WX 2.5.2
//    HWND hEdit  = (HWND)::SendMessage((HWND)mLength->GetHandle(), WM_USER+106, 0, 0);
//    ::SendMessage(hEdit, WM_SETFONT, (WPARAM)system_gui_font.GetResourceHandle(), MAKELPARAM(TRUE, 0));
//#endif
#if WBVERS<1000
    mTopSizer->Hide(mTB);
#else
    create_tools(mTB, get_tool_info());
#endif
    GetSizer()->Show(mButtons, global_style!=ST_POPUP);
    GetSizer()->Layout();
}

/*!
 * Should we show tooltips?
 */

bool DomainDesignerDlg::ShowToolTips()
{
    return TRUE;
}

void DomainDesignerDlg::show_type_specif(void)
// Uses the value of dom.tp and dom.specif.precision.
{ const wxChar * unicode_wtz_caption  = NULL,
               * length_scale_caption = NULL;
  mNull          ->Enable((wb_type_list[dom.tp].flags & TP_FL_HAS_NULLABLE_OPT)!=0);
  mByteLength    ->Enable((wb_type_list[dom.tp].flags & TP_FL_BYTES)!=0);
  mByteLengthCapt->Enable((wb_type_list[dom.tp].flags & TP_FL_BYTES)!=0);
 // length:
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_LENGTH)
  { length_scale_caption = _("Length:");
    mLength->SetRange(0, MAX_FIXED_STRING_LENGTH);
  }
 // scale:
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_SCALE)
  { length_scale_caption = _("Scale:");
    mLength->SetRange(0, precisions[dom.specif.precision]);
  }
 // length/scale caption:
  mLength    ->Enable(length_scale_caption!=NULL);
  mLengthCapt->Enable(length_scale_caption!=NULL);
  if (length_scale_caption) mLengthCapt->SetLabel(length_scale_caption);
 // character string specif:
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_LANGUAGE)
  { mIgnore->Show();  mColl->Enable();   mCollCapt->Enable();
    unicode_wtz_caption = _("Unicode");
  }
  else
  { mIgnore->Hide();  mColl->Disable();  mCollCapt->Disable(); }
 // time:
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_TZ_OPT)
    unicode_wtz_caption = _("Time with time zone");
 // unicode/WTZ caption:
  mUnicode->Show(unicode_wtz_caption!=NULL);
  if (unicode_wtz_caption) mUnicode->SetLabel(unicode_wtz_caption);
}

bool DomainDesignerDlg::TransferDataToWindow(void)
// Called on entry:
{ feedback_disabled=true;
 // init panel:
  mType->Clear();
  for (int i = 0;  wb_type_list[i].long_name;  i++)
    if (!(wb_type_list[i].flags & TP_FL_DOMAIN))
      mType->Append(wxGetTranslation(wb_type_list[i].long_name));
  fill_charset_collation_names(mColl);
 // update controls:
  show_type_specif();
 // show values:
  mType->SetSelection(dom.tp);
  mNull->SetValue(dom.nullable);
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_LENGTH)
    mLength->SetValue(dom.specif.length);
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_SCALE)
    mLength->SetValue(dom.specif.scale);
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_LANGUAGE)
  { mIgnore->Show();
    mUnicode->SetValue(dom.specif.wide_char!=0);
    mIgnore->SetValue(dom.specif.ignore_case);
    show_charset_collation(mColl, dom.specif.charset);
  }
 // time:
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_TZ_OPT)
    mUnicode->SetValue(dom.specif.with_time_zone);
 // bytes:
  if (wb_type_list[dom.tp].flags & TP_FL_BYTES)
    mByteLength->SetSelection(dom.specif.precision);
 // other:
  mDefault->SetValue(wxString(dom.defval, *cdp->sys_conv));
  mCheck->SetValue(dom.check ? wxString(dom.check, *cdp->sys_conv) : wxString(wxEmptyString));
  mDefer->SetSelection(dom.co_cha==COCHA_DEF ? 1 : dom.co_cha==COCHA_IMM_NONDEF ? 2 : 0);
  mDefer->Enable(dom.check && *dom.check!=0);
  mDeferCapt->Enable(dom.check && *dom.check!=0);
  feedback_disabled=false;
  changed = !modifying();  // the new design is valid and can be saved
  return true;
}

bool DomainDesignerDlg::TransferDataFromWindow(void)
// Validates
{// type index is in dom.tp
  if (wb_type_list[dom.tp].wbtype==ATT_MONEY) dom.specif.scale=2;
  if (wb_type_list[dom.tp].wbtype==ATT_CHAR)  dom.specif.length=1;
 // bytes:
  if (wb_type_list[dom.tp].flags & TP_FL_BYTES)
    dom.specif.precision = mByteLength->GetSelection();
 // nullable:
  dom.nullable=mNull->GetValue();
 // length & scale:
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_LENGTH)
  { int val;
    val = mLength->GetValue();
    if (val<=0 || val>MAX_FIXED_STRING_LENGTH)
      { client_error(cdp, INT_OUT_OF_BOUND);  cd_Signalize(cdp);  mLength->SetFocus();  return false; }
    dom.specif.length=(uns16)val;
  }
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_SCALE)
  { int val;
    val = mLength->GetValue();
    if (val<0 || val>precisions[dom.specif.precision])
      { client_error(cdp, INT_OUT_OF_BOUND);  cd_Signalize(cdp);  mLength->SetFocus();  return false; }
    dom.specif.scale=(sig8)val;
  }
 // string:
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_LANGUAGE)
  { dom.specif.wide_char  =mUnicode->GetValue();
    dom.specif.ignore_case=mIgnore->GetValue();
    dom.specif.charset    =(uns8)get_charset_collation(mColl);
  }
 // time:
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_TZ_OPT)
    dom.specif.with_time_zone = mUnicode->GetValue();
 // defval:
  corefree(dom.defval);  
  dom.defval = GetWindowTextAlloc(mDefault->GetValue(), cdp, dialog_parent());
  if (!dom.defval) return false;  // error reported
 // check:
  corefree(dom.check);  
  dom.check = GetWindowTextAlloc(mCheck->GetValue(), cdp, dialog_parent());
  if (!dom.check) return false;  // error reported
  int state = mDefer->GetSelection();
  dom.co_cha = state==1 ? COCHA_DEF : state==2 ? COCHA_IMM_NONDEF : COCHA_UNSPECIF;
  return true;
}

bool DomainDesignerDlg::Validate(void)
{
  return true;
}

/////////////////////////// virtual methods (commented in class any_designer): //////////////////////////////////
char * DomainDesignerDlg::make_source(bool alter)
{ t_flstr src(500,500);
  src.putc(' ');  // space form "testing" flag
  src.put(alter ? "ALTER" : "CREATE");  src.put(" DOMAIN `");
  if (*dom.schema) { src.put(dom.schema);  src.put("`.`"); }
  src.put(dom.domname);  src.put("`\r\n");
 // type:
  char type_name[55+OBJ_NAME_LEN];  
  t_specif wbspecif = dom.specif;
  convert_type_for_sql_1(wb_type_list, dom.tp, wbspecif);
  int wbtype = convert_type_for_sql_2(wb_type_list, dom.tp, wbspecif);
  sql_type_name(wbtype, wbspecif, true, type_name);
  src.put(type_name);
  src.put("\r\n");
 // default value:
  if (dom.defval && *dom.defval)
    { src.put("DEFAULT ");  src.put(dom.defval);  src.put("\r\n"); }
 // nullable: (NOT NULL must be before CHECK in the source, otherwise LL(1) conflict with NOT DEFERRABLE)
  if (!dom.nullable)
    src.put("NOT NULL\r\n");
 // check:
  if (dom.check && *dom.check)
  { src.put("CHECK (");  src.put(dom.check);  src.putc(')');
    write_constraint_characteristics(dom.co_cha, &src);  // ignored when condition not specified
    src.put("\r\n");
  }
  if (src.error()) return NULL; 
  return src.unbind();
}

bool DomainDesignerDlg::IsChanged(void) const
{ return changed; }

wxString DomainDesignerDlg::SaveQuestion(void) const
{ return modifying() ?
    _("Your changes have not been saved.\nDo you want to save your changes to the domain?") :
    _("The domain has not been created yet.\nDo you want to save the changes to the domain?");
}

bool DomainDesignerDlg::save_design(bool save_as)
// Saves the domain, returns true on success.
{ if (!save_design_generic(save_as, dom.domname, 0)) return false;  // using the generic save from any_designer (calls TransferDataFromWindow)
  changed=false;
  set_designer_caption();
  inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  /* tables modified */
  inval_table_d(cdp, NOOBJECT, CATEG_CURSOR); /* cursors may depend on tables */
 // must refresh table object cache because table object numbers changed:
  cd_Relist_objects_ex(cdp, TRUE);
  return true;
}

void DomainDesignerDlg::OnCommand(wxCommandEvent & event)
{ OnxCommand(event); }

void DomainDesignerDlg::destroy_designer(void)
{ 
  persistent_coords->pos=GetPosition();  
  persistent_coords->coords_updated=true;  // valid and should be saved
  if (global_style!=ST_POPUP) EndModal(1);  else Destroy();  // must not call Close, Close is directed here
}

void DomainDesignerDlg::OnxCommand(wxCommandEvent & event)
{
  switch (event.GetId())
  { case TD_CPM_SAVE:
      if (!IsChanged()) break;  // no message if no changes
      if (!save_design(false)) break;  // error saving, break
     // exit or continue:
      destroy_designer();
      break;
    case TD_CPM_SAVE_AS:
      if (!save_design(true)) break;  // error saving, break
     // exit or continue:
      destroy_designer();
      break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
      if (exit_request(this, true))
        destroy_designer();
      break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
      exit_request(this, false);
      destroy_designer();
      break;

    case TD_CPM_SQL:
    case TD_CPM_CHECK:  
    { if (!TransferDataFromWindow()) break;
     // add a fake name if name has not been specified yet:
      bool faked=false;
      if (!*dom.domname) 
        { strcpy(dom.domname, "<name>");  faked=true; } // fake name for a new domain which does not have a name externally specified
      char * src=make_source(modifying());
      if (src)
      { if (event.GetId()==TD_CPM_SQL)
        { ShowTextDlg dlg(this);  
          ((wxTextCtrl*)dlg.FindWindow(CD_TD_SQL))->SetValue(wxString(src+1, *cdp->sys_conv));  // skipping the space on the start
          dlg.ShowModal();
        }
        else
        { src[0]=(char)QUERY_TEST_MARK;   // source starts with a space
          BOOL err=cd_SQL_execute(cdp, src, NULL);
          if (err)  /* report error: */
            cd_Signalize(cdp);  // for a domain (unlike tables) problems with constrains or default value are reported as errors
          else info_box(_("The design is correct."));
        }  
        corefree(src); 
      }
      if (faked) *dom.domname=0;
      break;
    }
    case TD_CPM_HELP:
      wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_domain.html"));
      break;
  }
}

void DomainDesignerDlg::OnKeyDown( wxKeyEvent& event )
{ 
  OnCommonKeyDown(event);  // event.Skip() is inside
}

t_tool_info DomainDesignerDlg::dom_tool_info[DOM_TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,    File_save_xpm,   wxTRANSLATE("Save design")),
  t_tool_info(TD_CPM_EXIT,    exit_xpm,        wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CHECK,   validateSQL_xpm, wxTRANSLATE("Validate")),
  t_tool_info(TD_CPM_SQL,     showSQLtext_xpm, wxTRANSLATE("Show SQL")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_HELP,    help_xpm,        wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

wxMenu * DomainDesignerDlg::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items: 
  any_designer::get_designer_menu();
 // add specific items:
  AppendXpmItem(designer_menu, TD_CPM_SQL, _("Show S&QL"), showSQLtext_xpm);
  return designer_menu;
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for CD_DOM_TYPE
 */

void DomainDesignerDlg::OnCdDomTypeSelected( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  dom.tp=mType->GetSelection();  // must head here, show_type_specif uses it.
  if (wb_type_list[dom.tp].flags & TP_FL_GROUP) 
    { dom.tp++;  mType->SetSelection(dom.tp); }
 // make it consistent:
  if (wb_type_list[dom.tp].flags & TP_FL_BYTES)
    if (dom.specif.precision>3) dom.specif.precision=2;
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_LENGTH)
    if (mLength->GetValue() == 0x80000000)
      mLength->SetValue(12);
  if (wb_type_list[dom.tp].flags & TP_FL_HAS_SCALE)
    if (mLength->GetValue() == 0x80000000)
      mLength->SetValue(0);
  show_type_specif();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DOM_NULL
 */

void DomainDesignerDlg::OnCdDomNullClick( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DOM_LENGTH
 */

void DomainDesignerDlg::OnCdDomLengthUpdated( wxSpinEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DOM_UNICODE
 */

void DomainDesignerDlg::OnCdDomUnicodeClick( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DOM_IGNORE
 */

void DomainDesignerDlg::OnCdDomIgnoreClick( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for CD_DOM_COLL
 */

void DomainDesignerDlg::OnCdDomCollSelected( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DOM_DEFAULT
 */

void DomainDesignerDlg::OnCdDomDefaultUpdated( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DOM_CHECK
 */

void DomainDesignerDlg::OnCdDomCheckUpdated( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  wxString val = mCheck->GetValue();
  val.Trim(false);  val.Trim(true);
  mDefer->Enable(!val.IsEmpty());
  mDeferCapt->Enable(!val.IsEmpty());
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for CD_DOM_DEFER
 */

void DomainDesignerDlg::OnCdDomDeferSelected( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}

void dom_delete_bitmaps(void)
{ delete_bitmaps(DomainDesignerDlg::dom_tool_info); }

void DomainDesignerDlg::OnActivate(wxActivateEvent & event)
{ if (event.GetActive())
    Activated();
  else
    Deactivated();
  event.Skip();
}

void DomainDesignerDlg::OnCloseWindow(wxCloseEvent& event)
{ wxCommandEvent cmd; 
  cmd.SetId(event.CanVeto() ? TD_CPM_EXIT : TD_CPM_EXIT_UNCOND);
  OnxCommand(cmd);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DomainDesignerDlg::OnOkClick( wxCommandEvent& event )
{ if (IsChanged())
    if (!save_design(false)) return;  // error saving, break
  destroy_designer();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DomainDesignerDlg::OnCancelClick( wxCommandEvent& event )
{ 
  destroy_designer();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DOM_VALIDATE
 */

void DomainDesignerDlg::OnCdDomValidateClick( wxCommandEvent& event )
{ wxCommandEvent cmd;
  cmd.SetId(TD_CPM_CHECK);
  OnxCommand(cmd);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DOM_SQL
 */

void DomainDesignerDlg::OnCdDomSqlClick( wxCommandEvent& event )
{ wxCommandEvent cmd;
  cmd.SetId(TD_CPM_SQL);
  OnxCommand(cmd);
}


/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for CD_DOM_BYTELENGTH
 */

void DomainDesignerDlg::OnCdDomBytelengthSelected( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
 // must move mByteLength to the variable, show_type_specif uses it:
  if (wb_type_list[dom.tp].flags & TP_FL_BYTES)
    dom.specif.precision = mByteLength->GetSelection();
  show_type_specif();  // changes range for scale
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DOM_LENGTH
 */

void DomainDesignerDlg::OnCdDomLengthTextUpdated( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap DomainDesignerDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DomainDesignerDlg bitmap retrieval
    return wxNullBitmap;
////@end DomainDesignerDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DomainDesignerDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DomainDesignerDlg icon retrieval
    return wxNullIcon;
////@end DomainDesignerDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_DOM_BYTELENGTH
 */


