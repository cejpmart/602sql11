// tabledsgn.cpp
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop
#include "wbprezex.h"
#include "tabledsgn.h"
#include "objlist9.h"
#include "compil.h"
#include "datagrid.h"
#include "impexp.h"

#ifdef _DEBUG
#define new DEBUG_NEW           
#endif 
/*
Exit Logic: I do not want to have too many exit dialogs. So there is only one dialog which appears when user closes the designer
  before saving changes. Choices are YES to save (CREATE, ALTER), NO to drop design/changes, CANCEL to remain in the designer.
  CANCEL does not appear when cannot veto closing the designer.
Index logic: Single column ascending indexes are shown in the column grid while all other indexes are in the index grid.
  Such indexes are marked with 0x80 bit in their index_type.
  When column is dropped, its single-column index MUST be dropped automatically because there is no way to drop it later manually.
Locking the definition:
  When modifying the existing table, its definition is locked. SaveAs unlocks the original definition and lock the new one.
TODO:
  - save as
  - designer stack
  - length limits when entering values in grids
  - restarting the designer after Save command
  - ODBC support

Moving old value logic:
  The designer provides a default conversion which is updated every time the user alters the column type or specif.
  The default conversion is derived from the old type, the new type and the new specif.
  If the user specifies an explicit conversion it takes precedence over the default conversion.
  The default conversion cannot be used if the column is renamed. Therefore the designer creates an explicit conversion on rename.
  Possible problem: When a column is renamed and the its type is changed, it has an invalid explicit conversion.
*/

#define DUPLICATE_INDEX_DEF 1 // Enables defining the same indexes in column description and index description.

#define FICTIVE_TABLE_NAME "X___QQ"  // used when checking the syntax before the name is specified

const int precisions[4] = { i8_precision, i16_precision, i32_precision, i64_precision };

#define CONSTRAINS_HAVE_ERROR _("Error in check constrains!")
#define DEFVALS_HAVE_ERROR _("Error in the default value!")

#define TP_FL_HAS_LENGTH         1
#define TP_FL_INDEXABLE          2
#define TP_FL_HAS_SCALE          4
#define TP_FL_HAS_NULLABLE_OPT   8
#define TP_FL_HAS_LANGUAGE     0x10
#define TP_FL_HAS_TZ_OPT       0x20
#define TP_FL_DOMAIN           0x40
#define TP_FL_CAN_TRANSL       0x80
#define TP_FL_STRING           0x100
#define TP_FL_OLE              0x200
#define TP_FL_GROUP            0x400
#define TP_FL_BYTES            0x800
#define TP_FL_UNDEFINED        0x1000  // just for displaying the tables from older versions

#include "xrc/ColumnBasicsDlg.cpp"
#include "xrc/ColumnHintsDlg.cpp"
#include "xrc/MoveColValueDlg.cpp"
#include "xrc/ShowTextDlg.cpp"
#include "xrc/TablePropertiesDlg.cpp"

//////////////////////////////// type grid editor ////////////////////////////////////

t_type_list wb_type_list[] = {
{ wxEmptyString, _("---------- Numbers: ----------"),       0,            0,      TP_FL_GROUP },
{ _("INTEGER"), _("INTEGER"),                               ATT_INT32,    2<<8,   TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT|TP_FL_BYTES|TP_FL_CAN_TRANSL },
{ _("NUMERIC"), _("NUMERIC (with decimal digits)"),         ATT_MONEY,    (3<<8)+2, TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT|TP_FL_BYTES|TP_FL_HAS_SCALE },
{ _("REAL"), _("REAL (double precision)"),                  ATT_FLOAT,    0,      TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT },
{ wxEmptyString, _("---------- Characters: ----------"),    0,            0,      TP_FL_GROUP },
{ _("CHAR(n)"), _("CHAR(n) string with specified size"),    ATT_STRING,   12,     TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT|TP_FL_HAS_LANGUAGE|TP_FL_HAS_LENGTH|TP_FL_STRING},
{ _("CLOB"), _("CLOB (variable size text)"),                ATT_TEXT,     0,                                             TP_FL_HAS_LANGUAGE },
{ _("EXTCLOB"), _("EXTCLOB (externally stored text)"),      ATT_EXTCLOB,  0,                                             TP_FL_HAS_LANGUAGE },
{ _("CHAR"), _("CHAR (single character)"),                  ATT_CHAR,     1,      TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT/*|TP_FL_HAS_LANGUAGE*/ }, // language is not currently supported fot the Char type
{ wxEmptyString, _("---------- Date and Time: ----------"), 0,            0,      TP_FL_GROUP },
{ _("DATE"), _("DATE (day&month&year)"),                    ATT_DATE,     0,      TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT },
{ _("TIME"), _("TIME (time within a day)"),                 ATT_TIME,     0,      TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT|TP_FL_HAS_TZ_OPT },
{ _("TIMESTAMP"), _("TIMESTAMP (date and time)"),           ATT_TIMESTAMP,0,      TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT|TP_FL_HAS_TZ_OPT },
{ wxEmptyString, _("---------- Binary: ----------"),        0,            0,      TP_FL_GROUP },
{ _("BINARY(n)"), _("BINARY(n) with fixed size"), ATT_BINARY,   12,     TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT|TP_FL_HAS_LENGTH },
{ _("BLOB"), _("BLOB (variable size binary object)"),       ATT_NOSPEC,   0,                                             TP_FL_OLE },
{ _("EXTBLOB"), _("EXTBLOB (externally stored binary object)"), ATT_EXTBLOB, 0,                                                    },
//{ _("PICTURE"), _("PICTURE (variable size picture)"),       ATT_RASTER,   0,                                             TP_FL_OLE },
{ wxEmptyString, _("---------- Other types: ----------"),   0,            0,      TP_FL_GROUP },
{ _("BOOLEAN"), _("BOOLEAN (True or False)"),               ATT_BOOLEAN,  0,      TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT },
//{ _("SIGNATURE"), _("SIGNATURE (digital signature)"),       ATT_SIGNAT,   0,                                             TP_FL_OLE },
{ _("Domain"), _("Domain (user-defined type)"),             ATT_DOMAIN,   0,      TP_FL_INDEXABLE|TP_FL_HAS_NULLABLE_OPT|TP_FL_DOMAIN },
// The last type must not have the TP_FL_GROUP flag!
{ _("Undefined"), _("Undefined"),                           -1,           0,      TP_FL_UNDEFINED },
{ NULL, NULL, 0, 0, 0 }
};

///////////////////////////////////////////////////////////////////////
// Column numbers in the main grid:
#define TD_MAIN_COL_NAME     0
#define TD_MAIN_COL_TYPE     1
#define TD_MAIN_COL_LENGTH   2
#define TD_MAIN_COL_DEFAULT  3
#define TD_MAIN_COL_INDEX    4
#define TD_MAIN_COL_NULLS    5
#define TD_MAIN_COL_COUNT    6

#define INDEX_CHOICE_PRIM       0
#define INDEX_CHOICE_PRIM_NN    1
#define INDEX_CHOICE_UNIQUE     2
#define INDEX_CHOICE_UNIQUE_NN  3
#define INDEX_CHOICE_NONUN      4
#define INDEX_CHOICE_NONUN_NN   5
#define INDEX_CHOICE_NO_INDEX   6  // ordinal value of "- No index -"

int wxTableDesignerGridTableBase::GetNumberRows()
{ int columns = 1;  // fictive one
  for (int i=0;  i<tde->edta.attrs.count();  i++)
  { atr_all * atr = tde->edta.attrs.acc0(i);
    if (atr->state!=ATR_DEL) columns++;
  }
  return columns;
}

int wxTableDesignerGridTableBase::GetNumberCols()
{ return TD_MAIN_COL_COUNT; }

bool wxTableDesignerGridTableBase::IsEmptyCell( int row, int col )
{ return row>=tde->edta.attrs.count(); }

atr_all * table_designer::find_column_descr(int colnum, char ** p_cur_name)
// Skips deleted columns and returns colnum-th non-deleted column from the design
// Returns NULL if it does not exist
{ for (int i=0;  i<edta.attrs.count();  i++)
  { atr_all * atr = edta.attrs.acc0(i);
    if (atr->state!=ATR_DEL)
      if (!colnum) 
      { if (p_cur_name) *p_cur_name=(char*)edta.names.acc(i);
        return atr;
      }
      else colnum--;
  }
  return NULL;
}

char * GetWindowTextAlloc(wxString txt, cdp_t cdp, wxWindow * parent)
{ 
  txt.Trim(false);  txt.Trim(true);
  if (!txt.IsEmpty())
    if (!convertible_name(cdp, parent, txt))
      return NULL;
  char * ptr = (char*)corealloc(txt.Length()+1, 84);
  if (ptr==NULL) 
    { no_memory();  return NULL; }
  strcpy(ptr, txt.mb_str(*cdp->sys_conv));
  return ptr;
}

#if 0
const char * table_designer::type_param_capt(int wbtype) const
{ const char * str = ti_(wbtype).strngs;
  if (!str) return NULL;
  str+=strlen(str)+1;  str+=strlen(str)+1;  str+=strlen(str)+1;  str+=strlen(str)+1; 
  return *str ? str : NULL;
}
  const xtype_info & ti_(int wbtype) const
  { const xtype_info * tit = ti;
    while (tit->wbtype!=wbtype && tit->strngs) tit++;
    return *tit;
  }
#endif

void table_designer::set_default(atr_all * atr)
{ atr->nullable=wbtrue;  atr->multi=1;
  atr->expl_val=false;
  atr->defval=NULL;
  atr->state=ATR_NEW;
  atr->comment=NULL;
  atr->alt_value=NULL;
  if (WB_table)  /* ODBC table: default type may need specif */
  { atr->type=1; // ATT_INT32, the 1st type
    atr->specif.set_num(0, 2); // 4 bytes
  }
  else
  { atr->type=0;
    atr->specif=0;
    atr->specif.length=(type_list[atr->type].flags & TP_FL_HAS_LENGTH) ? 12 : 0;
  }
}


bool table_designer::new_column_name_ok(const char * new_col_name, atr_all * for_atr)
// Check the consistency of the new column name.
// Compares the new column name to the names of existing columns.
// Reports errors.
{ const char * p = new_col_name;
  while (*p && *p==' ') p++;
  if (!*p) return false;
  if (!is_object_name(cdp, wxString(new_col_name, *cdp->sys_conv), this)) return false;  // checks length and occurrence of the ` character
  if (!memcmp(new_col_name, "_W5_", 4) || !memcmp(new_col_name, "_w5_", 4))
    { error_box(_("Column name cannot start with _W5_"), this);  return false; }
  for (int i=0;  i<edta.attrs.count();  i++)
  { atr_all * atr = edta.attrs.acc0(i);
    if (atr!=for_atr && atr->state!=ATR_DEL)
      if (!my_stricmp(atr->name, new_col_name))
        { error_box(_("There is a column with the same name in this table!"));  return false; }
  }
  return true;
}

#define TP_NUM 7
static int type_index(int tp)
/* called if WB_table only */
{ switch (tp)
  { case ATT_CHAR:  case ATT_STRING: case ATT_TEXT: case ATT_EXTCLOB:
      return 0;
    case ATT_INT16: case ATT_INT32:  case ATT_INT8:  case ATT_INT64:  case ATT_MONEY:  case ATT_FLOAT:
      return 1;
    case ATT_DATE:  case ATT_TIME:  case ATT_TIMESTAMP:  
      return 2;
    case ATT_DOMAIN:
      return 3;
    case ATT_BOOLEAN:
      return 4;
    case ATT_BINARY:
      return 5;
    case ATT_NOSPEC:  case ATT_EXTBLOB:
      return 6;
    default:        
      return -1;
  }
}

wxString table_designer::get_default_conversion(atr_all * cur_atr, const char * names)
// Requires designer's cur_atr->type but cur_atr->specif in bytes!
{ if (cur_atr->state==ATR_NEW) 
    return wxEmptyString;
  int tpind0=type_index(cur_atr->orig_type);  // orig type is not converted to type index
  int tpind1=type_index(type_list[cur_atr->type].wbtype);
  bool inconvertible = tpind0==-1 || tpind1==-1 || tpind0==1 && tpind1==2 || tpind0==2 && tpind1==1 ||
                       tpind0!=tpind1 && (tpind1==4 || tpind0==4);
  if (inconvertible)  // integer vs. time or any type is special
    return wxEmptyString;
  else
  { char dest_domain_qname[55+OBJ_NAME_LEN];
    char qname[2+ATTRNAMELEN+1];  ident_to_sql(cdp, qname, cur_atr->name);  
    if ((tpind0==1 || tpind0==2 || tpind0>=4) && tpind0==tpind1)  // int to int or string to string: use the default cast
      return wxString(qname, *cdp->sys_conv);
    else  // use explicit cast:
    { if (type_list[cur_atr->type].flags & TP_FL_DOMAIN)  // cast to domain name
        ident_to_sql(cdp, dest_domain_qname, names+sizeof(tobjname));
      else  // cast to the specified new type
        sql_type_name(type_list[cur_atr->type].wbtype, cur_atr->specif, true, dest_domain_qname);  
      wxString str;
      str.Printf(wxT("CAST(%s AS %s)"), wxString(qname, *cdp->sys_conv).c_str(), wxString(dest_domain_qname, *cdp->sys_conv).c_str());
      return str;
    }
  }
}

void table_designer::show_default_conversion(atr_all * cur_atr, const char * cur_name)
{ wxString str, conv=get_default_conversion(cur_atr, cur_name);
  if (conv.IsEmpty())  // inconvertible
    str=_("The default value for this column (NULL if not specified)");
  else
    str.Printf(_("The default conversion: %s"), conv.c_str());
  upd_page->mRadio->SetString(0, str);
 // must update the width of the radio box:
  //upd_page->mRadio->Fit(); -- does not work because RadioBox does not have wx-children
  wxSize sz = upd_page->mRadio->GetBestSize();
  //upd_page->mRadio->SetClientSize(sz); -- does not work, does not resize the platform-children
  upd_page->mRadio->SetSize(-1, -1, sz.GetWidth(), sz.GetHeight(), wxSIZE_USE_EXISTING);
}

void table_designer::is_modified(atr_all * cur_atr, const char * names, int row)
// May update the default conversion but never changes the explicit conversion not the choice default-explicit.
{/* propose the copy code: */
  if (cur_atr->state==ATR_OLD || cur_atr->state==ATR_MODIF)
  // for NEW columns must not update automatically: no sense. Renamed columns are NEW and have the original name as their default value
  { if (WB_table)
    { // if (!((wxRadioButton *)upd_page->FindWindow(CD_TBUP_SPEC))->GetValue()) -- no, propose the new default event if explicit conversion is active
      // checking the radio button because its value may not have been stored yet in cur_atr->expl_val
       // must put the new values into the dialog, otherwise overwrittent on the next save_...
      update_value_page(true, cur_atr);
      show_default_conversion(cur_atr, names);
    }
    cur_atr->state=ATR_MODIF;
    set_row_colour(row, false);
  }
  changed=true;
}

wxString wxTableDesignerGridTableBase::GetValue( int row, int col )
{ atr_all * cur_atr = tde->find_column_descr(row);
  if (cur_atr==NULL) return wxEmptyString;  // column does not exist
  switch (col)
  { case TD_MAIN_COL_NAME: 
      return wxString(cur_atr->name, *tde->cdp->sys_conv);
    case TD_MAIN_COL_LENGTH:  // length
      return wxEmptyString;  // used by types without length
    case TD_MAIN_COL_DEFAULT:  // default value
      return cur_atr->defval==NULL ? (wxString)wxEmptyString : wxString(cur_atr->defval, *tde->cdp->sys_conv);
    case TD_MAIN_COL_NULLS:  // NULL allowed
      if (!(tde->type_list[cur_atr->type].flags & TP_FL_HAS_NULLABLE_OPT))  // cell should be disabled
        return wxT("1");
      return cur_atr->nullable ? wxT("1") : wxT("0");
  }
  return wxEmptyString;
}

bool wxTableDesignerGridTableBase::AppendRows(size_t numRows)
// only numRows==1 implemented
{ atr_all * cur_atr = tde->edta.attrs.next();
  if (!cur_atr) return false;
  tde->set_default(cur_atr);
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}

bool wxTableDesignerGridTableBase::InsertRows(size_t pos, size_t numRows)
// used only for registering columns in an existing table
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

bool wxTableDesignerGridTableBase::CanGetValueAs( int row, int col, const wxString& typeName )
{// must check for the fictive record: otherwise trying to convert a non-existing type to its name 
  atr_all * cur_atr = tde->find_column_descr(row);
  if (!cur_atr) return false;
  if (typeName == wxGRID_VALUE_STRING) return true;
  if (typeName == wxGRID_VALUE_NUMBER)
    if (col==TD_MAIN_COL_TYPE || col==TD_MAIN_COL_INDEX || TD_MAIN_COL_LENGTH) return true;
  return false;
}

long wxTableDesignerGridTableBase::GetValueAsLong( int row, int col)
{ atr_all * cur_atr = tde->find_column_descr(row);
  if (!cur_atr) return col==TD_MAIN_COL_INDEX ? INDEX_CHOICE_NO_INDEX : -1;
  switch (col)
  { case TD_MAIN_COL_TYPE: 
      return cur_atr->type;
    case TD_MAIN_COL_LENGTH:
      if (tde->type_list[cur_atr->type].flags & TP_FL_BYTES)
        return cur_atr->specif.precision;
      if (tde->type_list[cur_atr->type].flags & TP_FL_HAS_LENGTH) 
        return cur_atr->specif.length;
      return 0;  // cell should be disabled 
    case TD_MAIN_COL_INDEX:
    { ind_descr * indx = get_index_by_column(tde->cdp, &tde->edta, cur_atr->name);
      if (!indx) return INDEX_CHOICE_NO_INDEX;
      int itp = indx->index_type;
#ifndef DUPLICATE_INDEX_DEF
      if (!(itp & 0x80)) return INDEX_CHOICE_NO_INDEX;  // signle column index added via index list should not appear here!
      itp &= 0x7f;
#endif
      return itp==INDEX_PRIMARY ? (indx->has_nulls ? INDEX_CHOICE_PRIM   : INDEX_CHOICE_PRIM_NN) :
             itp==INDEX_UNIQUE  ? (indx->has_nulls ? INDEX_CHOICE_UNIQUE : INDEX_CHOICE_UNIQUE_NN) :
                                  (indx->has_nulls ? INDEX_CHOICE_NONUN  : INDEX_CHOICE_NONUN_NN);
    }
  }
  return -1;
}

void table_designer::update_default_choice(int row, int wbtype)
// Create a cell editor with the choice typical of default values for the given type.
{ wxString * keys = new wxString[3];
  int cnt = 1;
  keys[0]=wxEmptyString;  // always allowed
  switch (wbtype)
  { case ATT_TIME:      keys[cnt++]=wxT("Current_time");       break;
    case ATT_TIMESTAMP: keys[cnt++]=wxT("Current_timestamp");  break;
    case ATT_DATE:      keys[cnt++]=wxT("Current_date");       break;
    case ATT_INT32:     keys[cnt++]=wxT("UNIQUE");             break;
    case ATT_BINARY:
    case ATT_STRING:    keys[cnt++]=wxT("USER");               break;
    case ATT_BOOLEAN:   keys[cnt++]=wxT("False");
                        keys[cnt++]=wxT("True");               break;
  }
  wxGridCellODChoiceEditor * default_choice_editor = new wxGridCellODChoiceEditor(cnt, keys, true);
  column_grid->SetCellEditor(row, TD_MAIN_COL_DEFAULT, default_choice_editor);
  delete [] keys;
}

void table_designer::enable_by_type(int row, const atr_all * cur_atr)
{ column_grid->SetReadOnly(row, TD_MAIN_COL_NAME, (type_list[cur_atr->type].flags & TP_FL_UNDEFINED)!=0);  
  column_grid->SetReadOnly(row, TD_MAIN_COL_TYPE, (type_list[cur_atr->type].flags & TP_FL_UNDEFINED)!=0);  
  column_grid->SetReadOnly(row, TD_MAIN_COL_LENGTH, 
    !(type_list[cur_atr->type].flags & (TP_FL_HAS_LENGTH|TP_FL_BYTES)));  // length enabled
  if (type_list[cur_atr->type].flags & TP_FL_BYTES)
  { column_grid->SetCellRenderer(row, TD_MAIN_COL_LENGTH, new wxGridCellEnumRenderer(wxT("1 byte,2 bytes,4 bytes,8 bytes")));
    column_grid->SetCellEditor  (row, TD_MAIN_COL_LENGTH, new wxGridCellMyEnumEditor(wxT("1 byte,2 bytes,4 bytes,8 bytes")));
  }
  else if (type_list[cur_atr->type].flags & TP_FL_HAS_LENGTH)
  { column_grid->SetCellRenderer(row, TD_MAIN_COL_LENGTH, new wxGridCellNumberRenderer);
    column_grid->SetCellEditor  (row, TD_MAIN_COL_LENGTH, new wxGridCellNumberEditor(1,MAX_FIXED_STRING_LENGTH));
  }
  else
    column_grid->SetCellRenderer(row, TD_MAIN_COL_LENGTH, new wxGridCellStringRenderer);  // will be empty
  column_grid->SetReadOnly(row, TD_MAIN_COL_DEFAULT, (type_list[cur_atr->type].flags & TP_FL_UNDEFINED)!=0);  
  column_grid->SetReadOnly(row, TD_MAIN_COL_INDEX,  !(type_list[cur_atr->type].flags & TP_FL_INDEXABLE));   // index enabled
  column_grid->SetReadOnly(row, TD_MAIN_COL_NULLS,  !(type_list[cur_atr->type].flags & TP_FL_HAS_NULLABLE_OPT));  // index enabled
  if (WB_table) update_default_choice(row, type_list[cur_atr->type].wbtype);
}

void table_designer::set_row_colour(int row, bool inserting_new)
{ wxGridCellAttr * row_attr = new wxGridCellAttr;
  row_attr->SetTextColour(inserting_new ? wxColour(0, 128, 0) : wxColour(0, 0, 192));
  column_grid->SetRowAttr(row, row_attr);
  if (!inserting_new) column_grid->ForceRefresh();
}

void wxTableDesignerGridTableBase::SetValueAsLong( int row, int col, long value )
{ char * cur_name;
  atr_all * cur_atr = tde->find_column_descr(row, &cur_name);
  if (!cur_atr) return;
  switch (col)
  { case TD_MAIN_COL_TYPE: 
      if (value==-1) break;  // editor does not know the value
      if (cur_atr->type == value) break;
      if (tde->type_list[value].flags & TP_FL_GROUP) value++;
      cur_atr->type = value;
      cur_atr->specif=t_specif(tde->type_list[cur_atr->type].default_specif);
      if (tde->type_list[cur_atr->type].flags & TP_FL_HAS_LANGUAGE)
        cur_atr->specif.charset = tde->cdp->sys_charset;  // the default charset is the system charset
      tde->is_modified(cur_atr, cur_name, row);
      tde->enable_by_type(row, cur_atr);
      if (row==tde->column_grid->GetGridCursorRow()) tde->show_column_parameters(cur_atr, cur_name); // no more -- GetGridCursorRow returns the original row even if changing the selection
      // changes in column parametrs are lost by show_column_parameters but the apperance of param pages is updated
      break;
    case TD_MAIN_COL_LENGTH:
      if (tde->type_list[cur_atr->type].flags & TP_FL_BYTES)
        cur_atr->specif.precision=(uns8)value;
      else if (tde->type_list[cur_atr->type].flags & TP_FL_HAS_LENGTH)
      { cur_atr->specif.length=(uns16)value;
        if (cur_atr->specif.wide_char && value>MAX_FIXED_STRING_LENGTH/2) // must not be more, the test may not be exact because wide_char may be obsolete
          cur_atr->specif.length=MAX_FIXED_STRING_LENGTH/2;
      }
      else
        return; // cell should be disabled 
      tde->is_modified(cur_atr, cur_name, row);
      break;
    case TD_MAIN_COL_INDEX:
    { int itp = value==INDEX_CHOICE_PRIM   || value==INDEX_CHOICE_PRIM_NN   ? INDEX_PRIMARY :
                value==INDEX_CHOICE_UNIQUE || value==INDEX_CHOICE_UNIQUE_NN ? INDEX_UNIQUE : INDEX_NONUNIQUE;
      bool has_nulls = value==INDEX_CHOICE_PRIM || value==INDEX_CHOICE_UNIQUE || value==INDEX_CHOICE_NONUN;
     // find existing index:
      ind_descr * indx = get_index_by_column(tde->cdp, &tde->edta, cur_atr->name);
      if (!indx)  // creating a new index
      { if (value==INDEX_CHOICE_NO_INDEX) return;  // none->none
#ifdef DUPLICATE_INDEX_DEF
       // add a row to the index grid:
        if (!tde->index_list_grid->AppendRows(1))
          return;
        indx=tde->edta.indxs.acc0(tde->edta.indxs.count()-1);
        indx->index_type=itp;

#else
        indx=tde->edta.indxs.next();
        if (!indx) return;
        *indx->constr_name=0;  
        indx->state=ATR_NEW;  
        indx->index_type=0x80 | itp;
#endif
        indx->text=(char*)corealloc(2+ATTRNAMELEN+1, 77);
        if (indx->text) sprintf(indx->text, "`%s`", cur_atr->name);
        indx->has_nulls = /*value+1==INDEX_NONUNIQUE*/ true; // changed because indexes without null values are not very usable
        indx->has_nulls = has_nulls;
#ifdef DUPLICATE_INDEX_DEF
        tde->index_list_grid->Refresh();
#endif
      }
      else if (value==INDEX_CHOICE_NO_INDEX) // deleting an existing index
      { 
        int offs = indx-tde->edta.indxs.acc0(0);
#ifdef DUPLICATE_INDEX_DEF
        tde->index_list_grid->DeleteRows(offs, 1);  // this is wrong if ATR_DEL may be used in the [state]
#else
        tde->edta.indxs.delet(offs);  
#endif
      }
      else // modifying the type of an existing index
      { indx->has_nulls = has_nulls;
#ifdef DUPLICATE_INDEX_DEF
        indx->index_type=itp;
        tde->index_list_grid->Refresh();
#else
        indx->index_type=0x80 | itp;
#endif
      }
      tde->changed=true;
      tde->update_local_key_choice();
      break;
    }
  }
}

void table_designer::disable_cells_in_fictive_record(int row)
// For the fictive [row] describing non-existing table column disable all cells except for column name.
{ column_grid->SetReadOnly(row, TD_MAIN_COL_TYPE);     column_grid->SetReadOnly(row, TD_MAIN_COL_LENGTH);  
  column_grid->SetReadOnly(row, TD_MAIN_COL_DEFAULT);  column_grid->SetReadOnly(row, TD_MAIN_COL_INDEX);
  column_grid->SetReadOnly(row, TD_MAIN_COL_NULLS);
}

void wxTableDesignerGridTableBase::SetValue( int row, int col, const wxString& value )
{ char * cur_name;
  if (col==TD_MAIN_COL_NAME || col==TD_MAIN_COL_DEFAULT)
    if (!value.IsEmpty() && !convertible_name(tde->cdp, tde->dialog_parent(), value))
      return;
  atr_all * cur_atr = tde->find_column_descr(row, &cur_name);
  bool fictive = cur_atr==NULL;
  if (fictive) // starting new column
  { if (col!=TD_MAIN_COL_NAME) return;  // disabled cell
    if (!tde->new_column_name_ok(value.mb_str(*tde->cdp->sys_conv), NULL)) return;
   // add a new row into the design
    if (!tde->column_grid->AppendRows(1)) return;
    cur_atr = tde->find_column_descr(row, &cur_name);
   // enable the current column:
    tde->enable_by_type(row, cur_atr);
    tde->set_row_colour(row, true);
   // disable cells for the fictive column:
    tde->disable_cells_in_fictive_record(row+1);
  }
  switch (col)
  { case TD_MAIN_COL_NAME:
      if (!fictive && !tde->new_column_name_ok(value.mb_str(*tde->cdp->sys_conv), cur_atr)) return;
      if (!fictive && *cur_atr->name)  // overwriting and existing column name
      {// replace the name in constrains:
        tde->replace_column_name(cur_atr->name, value.mb_str(*tde->cdp->sys_conv));
        if (tde->modifying() && cur_atr->state!=ATR_NEW) // rename existing column
        {// insert the original column as "deleted":
          atr_all * del_atr = tde->edta.attrs.next();
          if (!del_atr) return;
          cur_atr = tde->find_column_descr(row, &cur_name);  // ATTN: must update cur_atr, dynar may have been reallocated!!!
          *del_atr=*cur_atr;
          del_atr->defval=NULL;  del_atr->comment=NULL;  del_atr->alt_value=NULL;
          del_atr->state=ATR_DEL;  
          cur_atr->state=ATR_NEW;
          tde->set_row_colour(row, true);
         // moving the value of the original column (must use the explicit conversion because the dafault cnversion referes to the new column name):
          //corefree(cur_atr->alt_value);
          //cur_atr->alt_value=(tptr)corealloc(ATTRNAMELEN+2+1, 62);
          //if (cur_atr->alt_value) ident_to_sql(cdp, cur_atr->alt_value, cur_atr->name);
          // must update values in the dialog, saved later:
          cur_atr->expl_val=true;
          tde->update_value_page(true, cur_atr);
#ifdef OLD_MOVING
          ((wxTextCtrl *)tde->upd_page->FindWindow(CD_TBUP_VAL))->SetValue(cur_atr->name); 
#else
          char qname[2+ATTRNAMELEN+1];  ident_to_sql(tde->cdp, qname, cur_atr->name);  
          tde->upd_page->mVal->SetValue(wxString(qname, *tde->cdp->sys_conv));
#endif
        }
      }
      strmaxcpy(cur_atr->name, value.mb_str(*tde->cdp->sys_conv), sizeof(cur_atr->name));
      break;
    case TD_MAIN_COL_DEFAULT:  // default value
    { safefree((void**)&cur_atr->defval);
      cur_atr->defval=GetWindowTextAlloc(value, tde->cdp, tde->dialog_parent());
      break;
    }
    case TD_MAIN_COL_NULLS:  // NULL allowed
      if (!(tde->type_list[cur_atr->type].flags & TP_FL_HAS_NULLABLE_OPT)) return;  // cell should be disabled
      cur_atr->nullable = value.c_str()[0]=='1';
      break;
  }
 // column info changed:
  tde->is_modified(cur_atr, cur_name, row);
  if (fictive) 
    tde->show_column_parameters(cur_atr, cur_name);  // must be called when name and state is defined
}

///////////////////////////// SQL server interface /////////////////////////
CFNC void stat_error(wxString msg)
{ wxGetApp().frame->SetStatusText(msg, 0);
  if (!msg.IsEmpty()) wxBell();
}

CFNC bool stat_signalize(cdp_t cdp)
{ wxString buf;
  int err=cd_Sz_error(cdp);
  if (err == INTERNAL_SIGNAL) return false;
  buf=Get_error_num_textWX(cdp, err);
  if (buf.IsEmpty()) return false;
  stat_error(buf);
  return true;
}

///////////////////////////// checks ///////////////////////////////////////
trecnum table_designer::create_new_check(void)
{ if (!edta.checks.next()) return NORECNUM;
#if 0
  if (hWndChk)
  { view_dyn * inst = (view_dyn*)GetWindowLong(hWndChk, 0);
    inst->dt->fi_recnum++;  inst->dt->rm_ext_recnum++;  
    inst->sel_rec=edta.checks.count()-1;
  }
#endif
  changed=true;
  return edta.checks.count()-1;
}


//////////////////////// charset and collation /////////////////////////////
static void fill_charset_collation_names(wxOwnerDrawnComboBox * combo)
{ combo->Clear();
  combo->Append(_("English (ASCII)"),   (void*)0);
  /*if (charset_available(1))     */combo->Append(_("Czech+Slovak Windows"), (void*)1);
  /*if (charset_available(128+1)) */combo->Append(_("Czech+Slovak ISO"),     (void*)(128+1));
  /*if (charset_available(2))     */combo->Append(_("Polish Windows"),       (void*)2);
  /*if (charset_available(128+2)) */combo->Append(_("Polish ISO"),           (void*)(128+2));
  /*if (charset_available(3))     */combo->Append(_("French Windows"),       (void*)3);
  /*if (charset_available(128+3)) */combo->Append(_("French ISO"),           (void*)(128+3));
  /*if (charset_available(4))     */combo->Append(_("German Windows"),       (void*)4);
  /*if (charset_available(128+4)) */combo->Append(_("German ISO"),           (void*)(128+4));
  /*if (charset_available(5))     */combo->Append(_("Italian Windows"),      (void*)5);
  /*if (charset_available(128+5)) */combo->Append(_("Italian ISO"),          (void*)(128+5));
}

static void show_charset_collation(wxOwnerDrawnComboBox * combo, int spec)
{ int count = combo->GetCount();
  for (int i=0;  i<count;  i++)
  { int data = (int)(size_t)combo->GetClientData(i);
    if (data==spec)
      { combo->SetSelection(i);  return; }
  }
  combo->SetSelection(0);  // unknown replaced by ASCII
}

static int get_charset_collation(wxOwnerDrawnComboBox * combo)
{ int ind = combo->GetSelection();
  if (ind==-1) return 0;
  return (int)(size_t)combo->GetClientData(ind);
}

/////////////////////////////// column parameters //////////////////////////////////
void table_designer::show_column_parameters(atr_all * cur_atr, char * cur_name)
// Transfers all properties of the selected column into the dialog controls.
// cur_atr is NULL for the fictive column.
{ bool fict = cur_atr==NULL;
  feedback_disabled=true;
  const wxChar * unicode_wtz_caption = NULL,
               * collation_domain_caption = NULL;
 // comment:
  cbd_page->mComment->Enable(!fict);
  cbd_page->mComment->SetValue(wxString(fict ? "" : skip_hints_in_comment(cur_atr->comment), *cdp->sys_conv));
 // scale:
  if (!fict && (type_list[cur_atr->type].flags & TP_FL_HAS_SCALE)!=0)
  { cbd_page->mScale->Show();  cbd_page->mScaleCapt->Show();
    cbd_page->mScale->SetRange(0, precisions[cur_atr->specif.precision]);
    cbd_page->mScale->SetValue(cur_atr->specif.scale);
  }
  else
    { cbd_page->mScale->Hide();  cbd_page->mScale->SetValue(0x80000000);  cbd_page->mScaleCapt->Hide(); }
 // character string specif:
  if (!fict && WB_table && (type_list[cur_atr->type].flags & TP_FL_HAS_LANGUAGE)!=0)
  { cbd_page->mIgnore->Show();
    cbd_page->mUnicode->SetValue(cur_atr->specif.wide_char!=0);
    cbd_page->mIgnore ->SetValue(cur_atr->specif.ignore_case);
    fill_charset_collation_names(cbd_page->mLang);
    show_charset_collation(cbd_page->mLang, cur_atr->specif.charset);
    unicode_wtz_caption = _("Unicode");
    collation_domain_caption = _("Collation and charset:");
  }
  else
    cbd_page->mIgnore->Hide();
 // time:
  if (!fict && WB_table && (type_list[cur_atr->type].flags & TP_FL_HAS_TZ_OPT)!=0)
  { cbd_page->mUnicode->SetValue(cur_atr->specif.with_time_zone);
    unicode_wtz_caption = _("Time with time zone");
  }
 // domains:
  if (!fict && WB_table && (type_list[cur_atr->type].flags & TP_FL_DOMAIN)!=0)
  { collation_domain_caption=_("Domain name:");
    cbd_page->mLang->Clear();
    move_object_names_to_combo(cbd_page->mLang, cdp, CATEG_DOMAIN);  // alternates with collation names
    char name2[2*OBJ_NAME_LEN+2];
    //if (*cur_name) { strcpy(name2, cur_name);  strcat(name2, "."); } else  -- not supported now
    *name2=0;
    strcat(name2, cur_name+(OBJ_NAME_LEN+1));
    cbd_page->mLang->SetValue(wxString(name2, *cdp->sys_conv));
  }
 // unicode/WTZ caption:
  cbd_page->mUnicode->Show(unicode_wtz_caption!=NULL);
  if (unicode_wtz_caption) cbd_page->mUnicode->SetLabel(unicode_wtz_caption);
 // collation/domain caption value and combo visibility:
  cbd_page->mLang->Show(collation_domain_caption!=NULL);
  cbd_page->mLangCapt->Show(collation_domain_caption!=NULL);
  if (collation_domain_caption) cbd_page->mLangCapt->SetLabel(collation_domain_caption);
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>=6
  cbd_page->Layout();  // since wx 2.6.0 must be called explicitly because control->Hide() call it, but control->Show() does not
#endif

#ifdef STOP
 // multiattribute:
  BOOL can_be_multi = WB_table && !fict && cur_atr->type!=ATT_HIST && cur_atr->type!=ATT_DATIM && cur_atr->type!=ATT_AUTOR && cur_atr->type!=ATT_SIGNAT;
  EnableWindow(GetDlgItem(hPage0, CD_TDCB_MULTI), can_be_multi);
  CheckDlgButton(hPage0, CD_TDCB_MULTI, !fict && cur_atr->multi>1);
  EnableWindow(GetDlgItem(hPage0, CD_TDCB_MULTI_RES_CAPT), !fict && can_be_multi && cur_atr->multi>1);
  EnableWindow(GetDlgItem(hPage0, CD_TDCB_MULTI_RES),      !fict && can_be_multi && cur_atr->multi>1);
  EnableWindow(GetDlgItem(hPage0, CD_TDCB_MULTI_EXT),      !fict && can_be_multi && cur_atr->multi>1);
  if (!fict) SetDlgItemInt(hPage0, CD_TDCB_MULTI_RES, (cur_atr->multi & 0x7f), FALSE);
  CheckDlgButton(hPage0, CD_TDCB_MULTI_EXT, !fict && (cur_atr->multi & 0x80)!=0);
#endif

 // hint information:
  if (hint_page)
  { bool combo, trans;
    if (fict) combo=trans=false;
    else 
    { combo = (type_list[cur_atr->type].flags & (TP_FL_CAN_TRANSL | TP_FL_STRING | TP_FL_DOMAIN)) != 0;
      trans = (type_list[cur_atr->type].flags & (TP_FL_CAN_TRANSL | TP_FL_DOMAIN)) != 0;
    }
    hint_page->FindWindow(CD_TDHI_CAPTION)      ->Enable(!fict);
    hint_page->FindWindow(CD_TDHI_CAPTION_C)    ->Enable(!fict);
    hint_page->FindWindow(CD_TDHI_HELPTEXT)     ->Enable(!fict);
    hint_page->FindWindow(CD_TDHI_HELPTEXT_C)   ->Enable(!fict);
    hint_page->FindWindow(CD_TDHI_BUBBLE)       ->Enable(!fict);
    //hint_page->FindWindow(CD_TDHI_COMBOGROUP)   ->Enable(combo);
    hint_page->FindWindow(CD_TDHI_COMBO_QUERY)  ->Enable(combo);
    hint_page->FindWindow(CD_TDHI_COMBO_QUERY_C)->Enable(combo);
    hint_page->FindWindow(CD_TDHI_COMBO_TEXT)   ->Enable(combo);
    hint_page->FindWindow(CD_TDHI_COMBO_TEXT_C) ->Enable(combo);
    hint_page->FindWindow(CD_TDHI_COMBO_VAL)    ->Enable(trans);
    hint_page->FindWindow(CD_TDHI_COMBO_VAL_C)  ->Enable(trans);
    if (!fict)
    { char buf[HINT_MAXLEN+1];
      get_hint_from_comment(cur_atr->comment, HINT_CAPTION, buf, sizeof(buf));
      ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_CAPTION))->SetValue(wxString(buf, *cdp->sys_conv));
      get_hint_from_comment(cur_atr->comment, HINT_HELPTEXT, buf, sizeof(buf));
      ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_HELPTEXT))->SetValue(wxString(*buf=='^' ? buf+1 : buf, *cdp->sys_conv));
      ((wxCheckBox *)hint_page->FindWindow(CD_TDHI_BUBBLE))->SetValue(*buf=='^');
     // combo:
      ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_VAL))->SetValue(wxEmptyString);
      if (combo)
      { get_hint_from_comment(cur_atr->comment, HINT_CODEBOOK, buf, sizeof(buf));
        ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_QUERY))->SetValue(wxString(buf, *cdp->sys_conv));
        get_hint_from_comment(cur_atr->comment, HINT_CODETEXT, buf, sizeof(buf));
        ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_TEXT))->SetValue(wxString(buf, *cdp->sys_conv));
        if (trans)
        { get_hint_from_comment(cur_atr->comment, HINT_CODEVAL, buf, sizeof(buf));
          ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_VAL))->SetValue(wxString(buf, *cdp->sys_conv));
        }
      }
      else
      { ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_QUERY))->SetValue(wxEmptyString);
        ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_TEXT))->SetValue(wxEmptyString);
      }
    }
  }
 // value update information:
  if (upd_page)  // only when altering a WB_table
  { bool moved_col = cur_atr!=NULL && *cur_atr->name && (cur_atr->state==ATR_NEW || cur_atr->state==ATR_MODIF);  // not for OLD
    update_value_page(moved_col, cur_atr);
    if (moved_col)
      show_default_conversion(cur_atr, cur_name);
#ifdef OLDMOVE
    ((wxTextCtrl *)upd_page->FindWindow(CD_TBUP_VAL))->SetValue(moved_col && cur_atr->expl_val && cur_atr->alt_value ? 
      cur_atr->alt_value : "");
#else
    upd_page->mVal->SetValue(wxString(moved_col && cur_atr->expl_val && cur_atr->alt_value ? cur_atr->alt_value : "", *cdp->sys_conv));
#endif
  }
  feedback_disabled=false;
}


void table_designer::update_value_page(bool moved_col, atr_all * cur_atr)
{ 
#ifdef OLDMOVE
  upd_page->FindWindow(CD_TBUP_DEF)      ->Enable(moved_col);
  upd_page->FindWindow(CD_TBUP_SPEC)     ->Enable(moved_col);
  upd_page->FindWindow(CD_TBUP_VAL)      ->Enable(moved_col && cur_atr->expl_val);
  upd_page->FindWindow(CD_TBUP_LIST)     ->Enable(moved_col);
  upd_page->FindWindow(CD_TBUP_LIST_CAPT)->Enable(moved_col);
  ((wxStaticText *)upd_page->FindWindow(CD_TBUP_CAPT))->SetLabel(cur_atr==NULL || !*cur_atr->name ? "" : 
      moved_col ? "Value of the column in the edited table:" : "Column value will not be changed");
  if (moved_col)
  { ((wxRadioButton *)upd_page->FindWindow(CD_TBUP_DEF)) ->SetValue(cur_atr->expl_val==0);
    ((wxRadioButton *)upd_page->FindWindow(CD_TBUP_SPEC))->SetValue(cur_atr->expl_val!=0);
  }
#else
  upd_page->mRadio   ->Enable(moved_col);
  upd_page->mVal     ->Enable(moved_col && cur_atr->expl_val);
  upd_page->mValCapt ->Enable(moved_col && cur_atr->expl_val);
  upd_page->mList    ->Enable(moved_col);
  upd_page->mListCapt->Enable(moved_col);
  upd_page->mCapt->SetLabel(cur_atr==NULL || !*cur_atr->name ? wxEmptyString : 
    moved_col ? (cur_atr->state==ATR_NEW ?  _("This column is new") : _("The column has been changed") ): 
    _("Column value will not be changed"));
  if (moved_col)
    upd_page->mRadio->SetSelection(cur_atr->expl_val ? 1 : 0);
#endif
}

bool _ToLong(wxString & str, long *val, int base=10) 
{
    const wxChar *start = str.c_str();
    wxChar *end;
    *val = wxStrtol(start, &end, base);

    // return TRUE only if scan was stopped by the terminating NUL and if the
    // string was not empty to start with
    return !*end && (end != start);
}

bool table_designer::save_column_parameters(atr_all * cur_atr, char * cur_name) // returns false iff error in parameters
{ if (cur_atr==NULL) return true;  // nothig to be saved
  bool post_modif = false;
 // scale:
  if ((type_list[cur_atr->type].flags & TP_FL_HAS_SCALE)!=0)
  { int ival = cbd_page->mScale->GetValue();
    int scale_limit = precisions[cur_atr->specif.precision];
    if (ival==0x80000000) ival=0;  // when changing type from string to Int and iimmediately going away
    else if (ival>scale_limit || ival<0) 
      { wxBell();  cbd_page->mScale->SetFocus();  return false; }
    cur_atr->specif.scale=ival;
  }
 // character string specif:
  if (WB_table && (type_list[cur_atr->type].flags & TP_FL_HAS_LANGUAGE)!=0)
  { cur_atr->specif.wide_char   = cbd_page->mUnicode->GetValue();
    cur_atr->specif.ignore_case = cbd_page->mIgnore->GetValue();
    cur_atr->specif.charset     =(uns8)get_charset_collation(cbd_page->mLang);
  }
 // time:
  if (WB_table && (type_list[cur_atr->type].flags & TP_FL_HAS_TZ_OPT)!=0)
    cur_atr->specif.with_time_zone = cbd_page->mUnicode->GetValue();
 // domain:
  if (WB_table && (type_list[cur_atr->type].flags & TP_FL_DOMAIN)!=0)
  { wxString str = cbd_page->mLang->GetValue();
    str.Trim(true);  str.Trim(false);
    if (str.IsEmpty()) { wxBell();  cbd_page->mLang->SetFocus();  return false; }
    strmaxcpy(cur_name+sizeof(tobjname), str.mb_str(*cdp->sys_conv), sizeof(tobjname));
    cur_atr->specif.domain_num=NOOBJECT;
  }

#ifdef STOP
 // multiattribute:
  cur_atr->multi=(uns8)GetDlgItemInt(hPage0, CD_TDCB_MULTI_RES, NULL, FALSE);
  if (IsDlgButtonChecked(hPage0, CD_TDCB_MULTI_EXT)) cur_atr->multi |= 0x80;
#endif
 // comment & hints:
  corefree(cur_atr->comment);
  cur_atr->comment=NULL;
  { t_flstr com;  
    if (hint_page)
    { wxString str;
      str = ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_CAPTION))->GetValue();
      if (*str.c_str())
        { com.putc('%');  com.put(HINT_CAPTION);  com.putc(' ');  com.putproc(str.mb_str(*cdp->sys_conv)); }
      str = ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_HELPTEXT))->GetValue();
      if (*str.c_str())
      { com.putc('%');  com.put(HINT_HELPTEXT);  com.putc(' ');  
        if (((wxCheckBox *)hint_page->FindWindow(CD_TDHI_BUBBLE))->GetValue()) com.putc('^');  
        com.putproc(str.mb_str(*cdp->sys_conv)); 
      }
      str = ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_QUERY))->GetValue();
      if (*str.c_str())
        { com.putc('%');  com.put(HINT_CODEBOOK);  com.putc(' ');  com.putproc(str.mb_str(*cdp->sys_conv)); }
      str = ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_TEXT))->GetValue();
      if (*str.c_str())
        { com.putc('%');  com.put(HINT_CODETEXT);  com.putc(' ');  com.putproc(str.mb_str(*cdp->sys_conv)); }
      str = ((wxTextCtrl *)hint_page->FindWindow(CD_TDHI_COMBO_VAL))->GetValue();
      if (*str.c_str())
        { com.putc('%');  com.put(HINT_CODEVAL);  com.putc(' ');  com.putproc(str.mb_str(*cdp->sys_conv)); }
    }
   // append the comment:
    wxString comm = cbd_page->mComment->GetValue();
    if (*comm.c_str())
    { if (com.getlen()) com.put("% ");
      com.put(comm.mb_str(*cdp->sys_conv));
    }
    cur_atr->comment=com.unbind();
  }

 // value update information:
  if (upd_page)  // only when altering a WB_table
  { if (cur_atr->state==ATR_NEW || cur_atr->state==ATR_MODIF)
    if (upd_page->mRadio->GetSelection()==0)
      cur_atr->expl_val=false;
      //safefree((void**)&cur_atr->alt_value);
    else
    { cur_atr->expl_val=true;
      corefree(cur_atr->alt_value);
      cur_atr->alt_value = GetWindowTextAlloc(upd_page->mVal->GetValue(), cdp, dialog_parent());
    }
  }
  return true;
}
///////////////////////////////// index list //////////////////////////////////////////
#include "xrc/IndexListDlg.cpp"

ind_descr * get_index_by_column(cdp_t cdp, table_all * ta, const char * unq_colname)
{ char quouted_name[2+ATTRNAMELEN+1];
  sprintf(quouted_name, "`%s`", unq_colname);
  for (int i=0;  i<ta->indxs.count();  i++)
  { ind_descr * indx = ta->indxs.acc0(i);
    if (indx->state!=ATR_DEL && indx->text!=NULL)
      if (!wb_stricmp(cdp, unq_colname, indx->text) || !wb_stricmp(cdp, quouted_name, indx->text))  /* found */
        return indx;
  }
  return NULL;
}

ind_descr * table_designer::get_index(int row)
// Returns the index descriptor for a given row ni the index list grid. Returns NULL for the fictive index.
{ for (int i=0;  i<edta.indxs.count();  i++)
  { ind_descr * indx = edta.indxs.acc0(i);
    if (indx->state!=ATR_DEL)
#ifndef DUPLICATE_INDEX_DEF
      if (!(indx->index_type & 0x80)) 
#endif
        if (!row) return indx;
        else row--;
  }
  return NULL;
}

wxString wxIndexListGridTable::GetColLabelValue( int col )
{ switch (col)
  { case 0:  return _("Index name");
    case 1:  return _("Index type");
	case 2:  return _("Index definition (column1, column2,...)");
	case 3:  return _("NULL?"); 
	default: return wxEmptyString;
  }	
}

int wxIndexListGridTable::GetNumberRows()
{ int columns = 1;  // fictive one
  for (int i=0;  i<tde->edta.indxs.count();  i++)
  { ind_descr * indx = tde->edta.indxs.acc0(i);
    if (indx->state!=ATR_DEL) 
#ifndef DUPLICATE_INDEX_DEF
      if (!(indx->index_type & 0x80)) 
#endif
        columns++;
  }
  return columns; 
}

int wxIndexListGridTable::GetNumberCols()
{ return 4; }

bool wxIndexListGridTable::IsEmptyCell( int row, int col )
{ return false; }

wxString wxIndexListGridTable::GetValue( int row, int col )
{ ind_descr * indx = tde->get_index(row);
  if (indx==NULL) return wxEmptyString;  // index does not exist
  switch (col)
  { case 0: 
      return wxString(indx->constr_name, *tde->cdp->sys_conv);
    case 2:  // definition
      return wxString(indx->text ? indx->text : "", *tde->cdp->sys_conv);
    case 3:  // has NULLs
      return indx->has_nulls ? wxT("1") : wxT("0");
  }
  return wxEmptyString;
}

bool wxIndexListGridTable::AppendRows(size_t numRows)
// only numRows==1 implemented
{ ind_descr * indx = tde->edta.indxs.next();
  if (!indx) return false;
  *indx->constr_name=0;  indx->index_type=INDEX_NONUNIQUE;  indx->text=NULL;  indx->has_nulls = true;
  indx->state=ATR_NEW;
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}

bool wxIndexListGridTable::InsertRows(size_t pos, size_t numRows)
// Used only for registering constrains in an existing table
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

bool wxIndexListGridTable::DeleteRows(size_t pos, size_t numRows)
// only numRows==1 implemented
{ ind_descr * indx = tde->get_index((int)pos);
  if (!indx) return false;
  int offs = indx-tde->edta.indxs.acc0(0);
  tde->edta.indxs.delet(offs);
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}


bool wxIndexListGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{
  if (typeName == wxGRID_VALUE_STRING) return true;
  if (typeName == wxGRID_VALUE_NUMBER)
    if (col==1) return true;
  return false;
}

long wxIndexListGridTable::GetValueAsLong( int row, int col)
{ ind_descr * indx = tde->get_index(row);
  if (!indx) return 3;  // displays empty field
  switch (col)
  { case 1: return indx->index_type-1;  break;
  }
  return -1;
}

void wxIndexListGridTable::SetValueAsLong( int row, int col, long value )
{ ind_descr * indx = tde->get_index(row);
  if (!indx) // starting a new constrain
  {// add a new row into the design
    if (!GetView()->AppendRows(1)) return;
    indx = tde->get_index(row);
    if (!indx) return;  // internal error
  }
  switch (col)
  { case 1: 
      if (value==-1) value=3;  // nothing selected in the combo
      indx->index_type=value+1;  break;
      break;
  }
#ifdef DUPLICATE_INDEX_DEF
  tde->column_grid->Refresh();  // index information in the column description may have been changed
#endif
  tde->changed=true;
}

void wxIndexListGridTable::SetValue( int row, int col, const wxString& value )
{ ind_descr * indx = tde->get_index(row);
  if (!indx) // starting a new constrain
  {// add a new row into the design
    if (!GetView()->AppendRows(1)) return;
    indx = tde->get_index(row);
    if (!indx) return;  // internal error
  }
  switch (col)
  { case 0:
      strmaxcpy(indx->constr_name, value.mb_str(*tde->cdp->sys_conv), sizeof(indx->constr_name));
      break;
    case 1:  // index type: not used any more (once called only when writing in fictive row)
      indx->index_type = value.c_str()[0]-'0'+1;
      tde->update_local_key_choice();
      break;
    case 2:  // definition
      safefree((void**)&indx->text);
      indx->text=GetWindowTextAlloc(value, tde->cdp, tde->dialog_parent());
      tde->update_local_key_choice();
      break;
    case 3:
      indx->has_nulls = value.c_str()[0]=='1';  break;
      break;
  }
#ifdef DUPLICATE_INDEX_DEF
  tde->column_grid->Refresh();  // index information in the column description may have been changed
#endif
  tde->changed=true;
}

void IndexListDlg::OnCdTdccDelClick( wxCommandEvent& event )
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  int row = tde->index_list_grid->GetGridCursorRow();
  if (row<0) return;
  ind_descr * indx = tde->get_index(row);
  if (!indx) { wxBell();  return; }  // the fictive row
  tde->index_list_grid->DeleteRows(row);
#ifdef DUPLICATE_INDEX_DEF
  tde->column_grid->Refresh();  // index information in the column description may have been changed
#endif
  tde->changed=true;
}

void IndexListDlg::OnHelpButton( wxCommandEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_table_pages.html#page_2")); }

void IndexListDlg::OnRangeSelect(wxGridRangeSelectEvent & event)
// Synchonizes selected cell with the selected range. 
// DROP constrain depends on the selected cell, but user may have selected a row in the design,...
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  if (event.Selecting())
    tde->index_list_grid->SetGridCursor(event.GetTopRow(), tde->index_list_grid->GetGridCursorCol());
  event.Skip();
}

wxGrid * table_designer::create_index_list_grid(wxWindow * parent)
{ index_list_grid = new wxGrid(parent, CD_TDCC_GRID, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER );
  index_list_grid->BeginBatch();
  index_list_grid_table = new wxIndexListGridTable(this);
  index_list_grid->SetTable(index_list_grid_table, TRUE);
  index_list_grid->SetColLabelSize(default_font_height);
  index_list_grid->SetDefaultRowSize(default_font_height);
  index_list_grid->SetRowLabelSize(22);  // for 2 digits
  index_list_grid->SetLabelFont(system_gui_font);
  DefineColSize(index_list_grid, 0, 100);
  DefineColSize(index_list_grid, 1, 80);
  DefineColSize(index_list_grid, 2, 240);
  DefineColSize(index_list_grid, 3, 10);  // orig 78, minimized
 // limit the length of the constrain name:
  wxGridCellAttr * name_attr = new wxGridCellAttr;
  name_attr->SetEditor(make_limited_string_editor(OBJ_NAME_LEN));
  index_list_grid->SetColAttr(0, name_attr);
 // boolean column:
  wxGridCellAttr * bool_col_attr = new wxGridCellAttr;
  wxGridCellBoolEditor * bool_editor = new wxGridCellBoolEditor();
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>=8
  bool_editor->UseStringValues(wxT("1"), wxT("0"));
#endif
  bool_col_attr->SetEditor(bool_editor);
  wxGridCellBoolRenderer * bool_renderer = new wxGridCellBoolRenderer();
  bool_col_attr->SetRenderer(bool_renderer);  // takes the ownership
  index_list_grid->SetColAttr(3, bool_col_attr);
 // prepare index type choice:
  wxGridCellAttr * indx_col_attr = new wxGridCellAttr;
  wxGridCellMyEnumEditor * indx_editor = new wxGridCellMyEnumEditor(_("Primary key,Unique,Non-unique"));
  indx_col_attr->SetEditor(indx_editor);
  wxGridCellEnumRenderer * indx_renderer = new wxGridCellEnumRenderer(wxString(_("Primary key,Unique,Non-unique, "))+wxT(" "));
  indx_col_attr->SetRenderer(indx_renderer);  // takes the ownership
  index_list_grid->SetColAttr(1, indx_col_attr);

  index_list_grid->EnableDragRowSize(false);
  index_list_grid->EndBatch();
  return index_list_grid;
}

///////////////////////////////// check list //////////////////////////////////////////
#include "xrc/CheckListDlg.cpp"

check_constr * table_designer::get_check(int row)
// Returns the index descriptor for a given row in the check list grid. Returns NULL for the fictive check.
{ for (int i=0;  i<edta.checks.count();  i++)
  { check_constr * check = edta.checks.acc0(i);
    if (check->state!=ATR_DEL) 
      if (!row) return check;
      else row--;
  }
  return NULL;
}

wxString wxCheckListGridTable::GetColLabelValue( int col )
{ switch (col)
  { case 0:  return _("Check name");
    case 1:  return _("Check condition");
    case 2:  return _("Deferred?");
    default: return wxEmptyString;
  }	
}

int wxCheckListGridTable::GetNumberRows()
{ int rows = 1;  // +fictive one
  for (int i=0;  i<tde->edta.checks.count();  i++)
  { check_constr * check = tde->edta.checks.acc0(i);
    if (check->state!=ATR_DEL) rows++;
  }
  return rows;
}

int wxCheckListGridTable::GetNumberCols()
{ return 3; }

bool wxCheckListGridTable::IsEmptyCell( int row, int col )
{ return false; }

wxString wxCheckListGridTable::GetValue( int row, int col )
{ check_constr * check = tde->get_check(row);
  if (check==NULL) return wxEmptyString;  // check does not exist
  switch (col)
  { case 0: 
      return wxString(check->constr_name, *tde->cdp->sys_conv);
    case 1:  // definition
      return wxString(check->text ? check->text : "", *tde->cdp->sys_conv);
    case 2:  // deferred
      return check->co_cha==COCHA_DEF ? wxT("1") : check->co_cha==COCHA_IMM_NONDEF ? wxT("0") : wxEmptyString;
  }
  return wxEmptyString;
}

bool wxCheckListGridTable::AppendRows(size_t numRows)
// only numRows==1 implemented
{ check_constr * check = tde->edta.checks.next();
  if (!check) return false;
  *check->constr_name=0;  check->text=NULL;  check->state=ATR_NEW;
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}

bool wxCheckListGridTable::InsertRows(size_t pos, size_t numRows)
// Used only for registering constrains in an existing table
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

bool wxCheckListGridTable::DeleteRows(size_t pos, size_t numRows)
// only numRows==1 implemented
{ check_constr * check = tde->get_check((int)pos);
  if (!check) return false;
  int offs = check-tde->edta.checks.acc0(0);
  tde->edta.checks.delet(offs);  // or change the state??
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}


bool wxCheckListGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{// must check for the fictive record: otherwise trying to convert a non-existing type to its name 
  check_constr * check = tde->get_check(row);
  if (!check) return false;
  if (typeName == wxGRID_VALUE_STRING) return true;
  return false;
}

long wxCheckListGridTable::GetValueAsLong( int row, int col)
{ check_constr * check = tde->get_check(row);
  if (!check) return -1;
  return -1;
}

void wxCheckListGridTable::SetValueAsLong( int row, int col, long value )
{ check_constr * check = tde->get_check(row);
  if (!check) return;
}

void wxCheckListGridTable::SetValue( int row, int col, const wxString& value )
{ check_constr * check = tde->get_check(row);
  bool fictive = check==NULL;
  if (!check) // starting a new constrain
  {// add a new row into the design
    if (!GetView()->AppendRows(1)) return;
    check = tde->get_check(row);
    if (!check) return;  // internal error
  }
  switch (col)
  { case 0:
      strmaxcpy(check->constr_name, value.mb_str(*tde->cdp->sys_conv), sizeof(check->constr_name));
      break;
    case 1:  // definition
      safefree((void**)&check->text);
      check->text=GetWindowTextAlloc(value, tde->cdp, tde->dialog_parent());
      break;
    case 2:
      check->co_cha = value.c_str()[0]=='1' ? COCHA_DEF : value.c_str()[0]=='0' ? COCHA_IMM_NONDEF : COCHA_UNSPECIF;  break;
      break;
  }
  tde->changed=true;
}

void CheckListDlg::OnCdChklDropClick( wxCommandEvent& event )
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  int row = tde->check_list_grid->GetGridCursorRow();
  if (row<0) return;
  check_constr * check = tde->get_check(row);
  if (!check) { wxBell();  return; }  // the fictive row
  tde->check_list_grid->DeleteRows(row);
  tde->changed=true;
}

void CheckListDlg::OnHelpButton( wxCommandEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_table_pages.html#page_3")); }
                                                          
void CheckListDlg::OnRangeSelect(wxGridRangeSelectEvent & event)
// Synchonizes selected cell with the selected range. 
// DROP constrain depends on the selected cell, but user may have selected a row in the design,...
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  if (event.Selecting())
    tde->check_list_grid->SetGridCursor(event.GetTopRow(), tde->check_list_grid->GetGridCursorCol());
  event.Skip();
}

wxGrid * table_designer::create_check_list_grid(wxWindow * parent)
{ check_list_grid = new wxGrid(parent, CD_CHKL_GRID, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER );
  check_list_grid->BeginBatch();
  check_list_grid_table = new wxCheckListGridTable(this);
  check_list_grid->SetTable(check_list_grid_table, TRUE);
  check_list_grid->SetColLabelSize(default_font_height);
  check_list_grid->SetDefaultRowSize(default_font_height);
  check_list_grid->SetRowLabelSize(22);  // for 2 digits
  check_list_grid->SetLabelFont(system_gui_font);
  DefineColSize(check_list_grid, 0, 100);
  DefineColSize(check_list_grid, 1, 240);
  DefineColSize(check_list_grid, 2, 10); // orig 78, minimized
 // limit the length of the constrain name:
  wxGridCellAttr * name_attr = new wxGridCellAttr;
  name_attr->SetEditor(make_limited_string_editor(OBJ_NAME_LEN));
  check_list_grid->SetColAttr(0, name_attr);
 // boolean column:
  wxGridCellAttr * bool_col_attr = new wxGridCellAttr;
  wxGridCellBool3Editor * bool_editor = new wxGridCellBool3Editor();
  bool_col_attr->SetEditor(bool_editor);
  wxGridCellBool3Renderer * bool_renderer = new wxGridCellBool3Renderer();
  bool_col_attr->SetRenderer(bool_renderer);  // takes the ownership
  check_list_grid->SetColAttr(2, bool_col_attr);

  check_list_grid->EnableDragRowSize(false);
  check_list_grid->EndBatch();
  return check_list_grid;
}

///////////////////////////////// ref list ////////////////////////////////////////////
#include "xrc/RefListDlg.cpp"

forkey_constr * table_designer::get_forkey(int row)
// Returns the forkey descriptor for a given row in the forkey list grid. Returns NULL for the fictive forkey.
{ for (int i=0;  i<edta.refers.count();  i++)
  { forkey_constr * forkey = edta.refers.acc0(i);
    if (forkey->state!=ATR_DEL) 
      if (!row) return forkey;
      else row--;
  }
  return NULL;
}

wxString wxRefListGridTable::GetColLabelValue( int col )
{ switch (col)
  { case 0:  return _("Name");
    case 1:  return _("Local column(s)");
	case 2:  return _("Remote table");
    case 3:  return _("Remote column(s)");
	case 4:  return _("ON UPDATE");
    case 5:  return _("ON DELETE");
	case 6:  return _("Deferred?");
	default: return wxEmptyString;
  }	
}

int wxRefListGridTable::GetNumberRows()
{ int rows = 1;  // +fictive one
  for (int i=0;  i<tde->edta.refers.count();  i++)
  { forkey_constr * forkey = tde->edta.refers.acc0(i);
    if (forkey->state!=ATR_DEL) rows++;
  }
  return rows;
}

int wxRefListGridTable::GetNumberCols()
{ return 7; }

bool wxRefListGridTable::IsEmptyCell( int row, int col )
{ return false; }

bool wxRefListGridTable::AppendRows(size_t numRows)
// only numRows==1 implemented
{ forkey_constr * forkey = tde->edta.refers.next();
  if (!forkey) return false;
  *forkey->constr_name=0;  forkey->par_text=forkey->text=NULL;  forkey->state=ATR_NEW;
  *forkey->desttab_name=*forkey->desttab_schema=0;
  forkey->delete_rule=forkey->update_rule=REFRULE_NO_ACTION;
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}

bool wxRefListGridTable::InsertRows(size_t pos, size_t numRows)
// Used only for registering constrains in an existing table
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

bool wxRefListGridTable::DeleteRows(size_t pos, size_t numRows)
// only numRows==1 implemented
{ forkey_constr * forkey = tde->get_forkey((int)pos);
  if (!forkey) return false;
  int offs = forkey-tde->edta.refers.acc0(0);
  tde->edta.refers.delet(offs);  // or change the state??
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}


bool wxRefListGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{// must check for the fictive record: otherwise trying to convert a non-existing type to its name 
  forkey_constr * forkey = tde->get_forkey(row);
  if (!forkey) return false;
  if (typeName == wxGRID_VALUE_STRING) return true;
  if (typeName == wxGRID_VALUE_NUMBER)
    if (col==4 || col==5) return true;
  return false;
}

long wxRefListGridTable::GetValueAsLong( int row, int col)
{ forkey_constr * forkey = tde->get_forkey(row);
  if (!forkey) return 4; // maps to an empty string
  switch (col)
  { case 4: return forkey->update_rule-1;
    case 5: return forkey->delete_rule-1;
  }
  return -1;
}

void wxRefListGridTable::SetValueAsLong( int row, int col, long value )
{ forkey_constr * forkey = tde->get_forkey(row);
  if (!forkey) return;
  switch (col)
  { case 4: forkey->update_rule=value+1;  break;
    case 5: forkey->delete_rule=value+1;  break;
  }
  tde->changed=true;
}

wxString wxRefListGridTable::GetValue( int row, int col )
{ forkey_constr * forkey = tde->get_forkey(row);
  if (forkey==NULL) return wxEmptyString;  // forkey does not exist
  switch (col)
  { case 0: 
      return wxString(forkey->constr_name, *tde->cdp->sys_conv);
    case 1:  // columns
      return wxString(forkey->text ? forkey->text : "", *tde->cdp->sys_conv);
    case 2:  // remote table
      return wxString(forkey->desttab_name, *tde->cdp->sys_conv);
    case 3:  // remote columns
      return wxString(forkey->par_text ? forkey->par_text : "", *tde->cdp->sys_conv);
    case 6:  // deferred
      return forkey->co_cha==COCHA_DEF ? wxT("1") : forkey->co_cha==COCHA_IMM_NONDEF ? wxT("0") : wxEmptyString;
  }
  return wxEmptyString;
}

void wxRefListGridTable::SetValue( int row, int col, const wxString& value )
{ forkey_constr * forkey = tde->get_forkey(row);
  bool fictive = forkey==NULL;
  if (!forkey) // starting a new constrain
  {// add a new row into the design
    if (!GetView()->AppendRows(1)) return;
    forkey = tde->get_forkey(row);
    if (!forkey) return;  // internal error
  }
  switch (col)
  { case 0:
      strmaxcpy(forkey->constr_name, value.mb_str(*tde->cdp->sys_conv), sizeof(forkey->constr_name));
      break;
    case 1:  // colums(s)
      safefree((void**)&forkey->text);
      forkey->text=GetWindowTextAlloc(value, tde->cdp, tde->dialog_parent());
      break;
    case 2:  // remote table
      strmaxcpy(forkey->desttab_name, value.mb_str(*tde->cdp->sys_conv), sizeof(forkey->desttab_name));  
      tde->update_remote_key_choice(forkey->desttab_name, row);
      break;
    case 3:  // remote colums(s)
      safefree((void**)&forkey->par_text);
      forkey->par_text=GetWindowTextAlloc(value, tde->cdp, tde->dialog_parent());
      break;
    case 6:
      forkey->co_cha = value.c_str()[0]=='1' ? COCHA_DEF : value.c_str()[0]=='0' ? COCHA_IMM_NONDEF : COCHA_UNSPECIF;  break;
      break;
  }
  tde->changed=true;
}

void RefListDlg::OnCdReflDropClick( wxCommandEvent& event )
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  int row = tde->ref_list_grid->GetGridCursorRow();
  if (row<0) return;
  forkey_constr * forkey = tde->get_forkey(row);
  if (!forkey) { wxBell();  return; }  // the fictive row
  tde->ref_list_grid->DeleteRows(row);
  tde->changed=true;
}

void RefListDlg::OnHelpClick( wxCommandEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_table_pages.html#page_4")); }
                              
void RefListDlg::OnRangeSelect(wxGridRangeSelectEvent & event)
// Synchonizes selected cell with the selected range. 
// DROP constrain depends on the selected cell, but user may have selected a row in the design,...
{ table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
  if (event.Selecting())
    tde->ref_list_grid->SetGridCursor(event.GetTopRow(), tde->ref_list_grid->GetGridCursorCol());
  event.Skip();
}

void table_designer::update_local_key_choice(void)
{// if editor is opened, close it:
  int col = ref_list_grid->GetGridCursorCol();
  int row = ref_list_grid->GetGridCursorRow();
  if (col==1 && row!=-1)
    ref_list_grid->DisableCellEditControl();  // otherwise crashes when returning to the control
 // create the new editor:
  wxString * keys = new wxString[edta.indxs.count()];
  int i, cnt = 0;
  for (i=0;  i<edta.indxs.count();  i++)
  { ind_descr * indx = edta.indxs.acc0(i);
    if (indx->state!=ATR_DEL)
      if (indx->text && *indx->text)
        keys[cnt++]=wxString(indx->text, *cdp->sys_conv);
  }
  wxGridCellAttr * loc_key_attr = new wxGridCellAttr;
  wxGridCellODChoiceEditor * loc_key_editor = new wxGridCellODChoiceEditor(cnt, keys, true);
  loc_key_attr->SetEditor(loc_key_editor);
  ref_list_grid->SetColAttr(1, loc_key_attr);
  delete [] keys;
}

void table_designer::update_remote_key_choice(const char * tabname, int row)
{ if (!ref_list_grid) return;  // ref list is not open
  int cnt = 0;  wxString * keys = NULL;
  ttablenum tabobj;  table_all loc_ta;
  if (!cd_Find_prefixed_object(cdp, schema_name, tabname, CATEG_TABLE, &tabobj))
  { tptr defin=cd_Load_objdef(cdp, tabobj, CATEG_TABLE);
    if (defin)
    { int err=compile_table_to_all(cdp, defin, &loc_ta);
      corefree(defin);
      keys = new wxString[loc_ta.indxs.count()];
      if (!err)
        for (int i=0;  i<loc_ta.indxs.count();  i++)
        { ind_descr * indx = loc_ta.indxs.acc0(i);
          if ((indx->index_type==INDEX_PRIMARY) || (indx->index_type==INDEX_UNIQUE))
            if (indx->text && *indx->text)
              keys[cnt++]=wxString(indx->text, *cdp->sys_conv);
        }
    }
  }
  //wxGridCellAttr * rem_key_attr = new wxGridCellAttr;
  wxGridCellODChoiceEditor * rem_key_editor = new wxGridCellODChoiceEditor(cnt, keys, true);
  //rem_key_attr->SetEditor(rem_key_editor);
  //ref_list_grid->SetColAttr(3, rem_key_attr);
  ref_list_grid->SetCellEditor(row, 3, rem_key_editor);
  delete [] keys;
}

wxGrid * table_designer::create_ref_list_grid(wxWindow * parent)
{ ref_list_grid = new wxGrid(parent, CD_REFL_GRID, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER );
  ref_list_grid->BeginBatch();
  ref_list_grid_table = new wxRefListGridTable(this);
  ref_list_grid->SetTable(ref_list_grid_table, TRUE);
  ref_list_grid->SetColLabelSize(default_font_height);
  ref_list_grid->SetDefaultRowSize(default_font_height);
  ref_list_grid->SetRowLabelSize(22);  // for 2 digits
  ref_list_grid->SetLabelFont(system_gui_font);
  DefineColSize(ref_list_grid, 0, 100);
  DefineColSize(ref_list_grid, 1, 120);
  DefineColSize(ref_list_grid, 2, 100);
  DefineColSize(ref_list_grid, 3, 120);
  DefineColSize(ref_list_grid, 4, 80);
  DefineColSize(ref_list_grid, 5, 80);
  DefineColSize(ref_list_grid, 6, 10); // orig 68, minimized
 // limit the length of the constrain name:
  wxGridCellAttr * name_attr = new wxGridCellAttr;
  name_attr->SetEditor(make_limited_string_editor(OBJ_NAME_LEN));
  ref_list_grid->SetColAttr(0, name_attr);
 // prepare table choice:
  wxString table_choice;  bool first=true;
  void * en = get_object_enumerator(cdp, CATEG_TABLE, cdp->sel_appl_name);
  tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
  while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
    if (!IS_ODBC_TABLE(objnum))
    { if (first) first=false;  else table_choice=table_choice+wxT(",");
      table_choice=table_choice+wxString(name, *cdp->sys_conv);
    }  
  free_object_enumerator(en);
  wxGridCellAttr * tabname_attr = new wxGridCellAttr;
  wxGridCellODChoiceEditor * tabname_editor = new wxGridCellODChoiceEditor(0, NULL, true);
  tabname_editor->SetParameters(table_choice);
  tabname_attr->SetEditor(tabname_editor);
  ref_list_grid->SetColAttr(2, tabname_attr);
  //tabname_attr->DecRef(); -- causes error (memory reuse)!!!
 // prepare ON action choice:
  wxGridCellAttr * on_action_attr = new wxGridCellAttr;
  wxGridCellMyEnumEditor * on_editor = new wxGridCellMyEnumEditor(_("Rollback,Set NULL,Set Default,Cascade update"));
  on_action_attr->SetEditor(on_editor);
  wxGridCellEnumRenderer * on_renderer = new wxGridCellEnumRenderer(wxString(_("Rollback,Set NULL,Set Default,Cascade update,"))+wxT(" "));
  on_action_attr->SetRenderer(on_renderer);  // takes the ownership
  ref_list_grid->SetColAttr(4, on_action_attr);  // takes the ownership!! Must not reuse!!
  on_action_attr = new wxGridCellAttr;
  on_editor = new wxGridCellMyEnumEditor(_("Rollback,Set NULL,Set Default,Cascade delete,"));
  on_action_attr->SetEditor(on_editor);
  on_renderer = new wxGridCellEnumRenderer(wxString(_("Rollback,Set NULL,Set Default,Cascade delete,"))+wxT(" "));
  on_action_attr->SetRenderer(on_renderer);  // takes the ownership
  ref_list_grid->SetColAttr(5, on_action_attr);
 // boolean column:
  wxGridCellAttr * bool_col_attr = new wxGridCellAttr;
  wxGridCellBool3Editor * bool_editor = new wxGridCellBool3Editor();
  bool_col_attr->SetEditor(bool_editor);
  wxGridCellBool3Renderer * bool_renderer = new wxGridCellBool3Renderer();
  bool_col_attr->SetRenderer(bool_renderer);  // takes the ownership
  ref_list_grid->SetColAttr(6, bool_col_attr);

  ref_list_grid->EnableDragRowSize(false);
  ref_list_grid->EndBatch();
  return ref_list_grid;
}

///////////////////////////////// main grid ///////////////////////////////////////////
wxString wxTableDesignerGridTableBase::GetColLabelValue( int col )
{ switch (col)
  { case TD_MAIN_COL_NAME:    return _("Column name");
    case TD_MAIN_COL_TYPE:    return _("Column type");
    case TD_MAIN_COL_LENGTH:  return _("Length");
    case TD_MAIN_COL_DEFAULT: return _("Default value");
    case TD_MAIN_COL_INDEX:   return _("Index");
    case TD_MAIN_COL_NULLS:   return _("NULLable");
	default: return wxEmptyString;
  }	
}

#define COLUMN_LABEL_MIN_MARGIN 5

void DefineColSize(wxGrid * grid, int col, int minsize)
{ wxString label = grid->GetColLabelValue(col);
  wxClientDC dc(grid);
  dc.SetFont(system_gui_font);
  wxCoord w, h;
  dc.GetTextExtent(label, &w, &h);
  w+=2*COLUMN_LABEL_MIN_MARGIN;  // margin
  if (w<minsize) w=minsize;
  grid->SetColSize(col, w);
}

wxGrid * table_designer::create_grid(wxWindow * parent)
{ wxGrid * grid = new wxGrid(parent, 1, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER);  // wxSIMPLE_BORDER makes the sash visible
  grid->BeginBatch();
  grid_table = new wxTableDesignerGridTableBase(this);
  grid->SetTable(grid_table, TRUE);
  grid->SetRowLabelSize(24);
  grid->SetColLabelSize(default_font_height);
  grid->SetDefaultRowSize(default_font_height);
  grid->SetLabelFont(system_gui_font);
  DefineColSize(grid, TD_MAIN_COL_NAME, 90);
  DefineColSize(grid, TD_MAIN_COL_TYPE, 190);
  DefineColSize(grid, TD_MAIN_COL_LENGTH, LENGTH_SPIN_COL_WIDTH);  // must have space for 4 digits and the spin control because spin cannot scroll
  DefineColSize(grid, TD_MAIN_COL_DEFAULT, 96);
  DefineColSize(grid, TD_MAIN_COL_INDEX, 130);
  DefineColSize(grid, TD_MAIN_COL_NULLS, 10);  // originally 66, but minimized
 // limit the length of the table column name:
  wxGridCellAttr * tabcol_attr = new wxGridCellAttr;
  tabcol_attr->SetEditor(make_limited_string_editor(ATTRNAMELEN));
  grid->SetColAttr(TD_MAIN_COL_NAME, tabcol_attr);
 // prepare type choice:
  wxGridCellAttr * type_col_attr = new wxGridCellAttr;
  wxString type_choice;  int i;
  i=0;
  do
  { if (!(type_list[i].flags & TP_FL_UNDEFINED))
      type_choice=type_choice+wxGetTranslation(type_list[i].long_name);
    i++;
    if (!type_list[i].long_name) break;
    if (!(type_list[i].flags & TP_FL_UNDEFINED))
      type_choice=type_choice+wxT(",");
  } while (true);  
  wxGridCellMyEnumEditor * type_editor = new wxGridCellMyEnumEditor(type_choice);
  type_col_attr->SetEditor(type_editor);
  i=0;  type_choice=wxEmptyString;
  do
  { type_choice=type_choice+wxGetTranslation(type_list[i].short_name);
    i++;
    if (!type_list[i].short_name) break;
    type_choice=type_choice+wxT(",");
  } while (true);  
  wxGridCellEnumRenderer * type_renderer = new wxGridCellEnumRenderer(type_choice);
  type_col_attr->SetRenderer(type_renderer);  // takes the ownership
  grid->SetColAttr(TD_MAIN_COL_TYPE, type_col_attr);

 // prepare index type choice:
  wxGridCellAttr * indx_col_attr = new wxGridCellAttr;
  wxGridCellMyEnumEditor * indx_editor = new wxGridCellMyEnumEditor(_("Primary key,Primary key (NOT NULL),Unique,Unique (NOT NULL),Non-unique,Non-unique (NOT NULL), - no index -"));
  indx_col_attr->SetEditor(indx_editor);
  wxGridCellEnumRenderer * indx_renderer = new wxGridCellEnumRenderer(wxString(_("Primary key,Primary key (NOT NULL),Unique,Unique (NOT NULL),Non-unique,Non-unique (NOT NULL),"))+wxT(" "));
  indx_col_attr->SetRenderer(indx_renderer);  // takes the ownership
  grid->SetColAttr(TD_MAIN_COL_INDEX, indx_col_attr);

 // boolean column:
  wxGridCellAttr * bool_col_attr = new wxGridCellAttr;
  wxGridCellBoolEditor * bool_editor = new wxGridCellBoolEditor();
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>=8
  bool_editor->UseStringValues(wxT("1"), wxT("0"));
#endif
  bool_col_attr->SetEditor(bool_editor);
  wxGridCellBoolRenderer * bool_renderer = new wxGridCellBoolRenderer();
  bool_col_attr->SetRenderer(bool_renderer);  // takes the ownership
  grid->SetColAttr(TD_MAIN_COL_NULLS, bool_col_attr);

  grid->EnableDragRowSize(false);
  grid->EndBatch();
  return grid;
}

void table_designer::init_new_table(const char * table_name)
{ edta.tabdef_flags=0;
  if (table_name && *table_name) strcpy(edta.selfname, table_name);  // name for the created object specified
  else strcpy(edta.selfname, FICTIVE_TABLE_NAME);
  edta.schemaname[0]=0;
#ifdef STOP
 /* find actual type info: */
  tbdf_conn = cdp->odbc.conns;
  while (tbdf_conn != NULL && !tbdf_conn->selected)
    tbdf_conn=tbdf_conn->next_conn;
  if (tbdf_conn != NULL)
       { ti=tbdf_conn->ti;  if (ti==NULL) return FALSE;  WB_table=false; }
  else
#endif
  { type_list=wb_type_list;                              WB_table=true; }
}

bool table_designer::compile_WB_table_structure(void)
// Table definition is in the [ce_buffer] of in the [objnum] object.
// Compiles the table definition into [edta], defines [WB_table], [ti].
{ WB_table=true;  type_list=wb_type_list;  tptr defin;
  if (ce_buffer) defin=ce_buffer;  // take the definition from [ce_buffer]
  else  // load the table definition from the database:
  { defin=cd_Load_objdef(cdp, objnum, CATEG_TABLE);
    if (!defin) { cd_Signalize(cdp);  return false; }
  }
 // compile the table definition:
  int comp_err = compile_table_to_all(cdp, defin, &edta);
  if (!ce_buffer) corefree(defin);  // release the buffer iff allocated here
  return comp_err==0;
}

/* Representation of type information:
Topic                           In the designer             In SQL
type number                     index to the type_list      ATT_ type number
char string length              # of characters             # of bytes
scalability of integer          Integer or Numeric type     specif.scale zero on nonzero
# of bytes for integer          specif.precision 0..3       ATT_ type
*/

int convert_type_for_edit(const t_type_list * type_list, int type, t_specif & specif)
{
 // convert integer subtypes:
  if      (type==ATT_INT8)
    { type=specif.scale ? ATT_MONEY : ATT_INT32;  specif.precision=0; }
  else if (type==ATT_INT16)
    { type=specif.scale ? ATT_MONEY : ATT_INT32;  specif.precision=1; }
  else if (type==ATT_INT32)
    { type=specif.scale ? ATT_MONEY : ATT_INT32;  specif.precision=2; }
  else if (type==ATT_INT64)
    { type=specif.scale ? ATT_MONEY : ATT_INT32;  specif.precision=3; }
  else if (type==ATT_MONEY)
    { specif.precision=3; }
 // convert type to type index:
  if (type==ATT_RASTER)  // hide the obsolete 8.1 type
    { type=ATT_NOSPEC;  specif.length=SPECIF_VARLEN-1; }
  if (type==ATT_SIGNAT)  // hide the obsolete 8.1 type
    { type=ATT_NOSPEC;  specif.length=SPECIF_VARLEN-2; }
  int j=0;
  while (type_list[j].long_name)
    if (type_list[j].wbtype==type) break;
    else j++;
  if (type_list[j].long_name) // found
    type=j;
  else
    type=j-1; // not found, use the last type == "Undefined"
  if (type_list[type].flags & TP_FL_HAS_LANGUAGE)
    if (specif.wide_char)
      specif.length /= 2;  // conveert bytes to charactes
  return type;
}

void table_designer::convert_for_edit(void)
{
  for (int i=0;  i<edta.attrs.count();  i++)
  { atr_all * cur_atr = edta.attrs.acc0(i);
    if (cur_atr->state==ATR_DEL) continue;
    cur_atr->type = convert_type_for_edit(type_list, cur_atr->type, cur_atr->specif);
   // release implicit default value:
    if (cur_atr->state==ATR_NEW || cur_atr->state==ATR_MODIF)
      if (!cur_atr->expl_val)
        { corefree(cur_atr->alt_value);  cur_atr->alt_value=NULL; }
  }
}

void convert_type_for_sql_1(const t_type_list * type_list, int type, t_specif & specif)
{
  if (type_list[type].flags & TP_FL_HAS_LANGUAGE)
    if (specif.wide_char)
    { specif.length *= 2;  // conveert charactes to bytes
      if (specif.length > MAX_FIXED_STRING_LENGTH) specif.length = MAX_FIXED_STRING_LENGTH;
    }
}

int convert_type_for_sql_2(const t_type_list * type_list, int type, t_specif & specif)
{ 
  type=type_list[type].wbtype;
  if (type==ATT_NOSPEC && specif.length==SPECIF_VARLEN-1)
    type=ATT_RASTER;  // restore the obsolete 8.1 type
  if (type==ATT_NOSPEC && specif.length==SPECIF_VARLEN-2)
    type=ATT_SIGNAT;  // restore the obsolete 8.1 type
  if (type==ATT_INT32 || type==ATT_MONEY)
  {// check the scale: 
    if (type==ATT_INT32)
      specif.scale=0;
    else  // convert to a reasonable value: (used by the domain designer)
    { int scale_limit = precisions[specif.precision];
      if (specif.scale<0) specif.scale=0;
      else if (specif.scale>scale_limit) specif.scale=scale_limit;
    }
   // convert the type representation:
    if      (specif.precision==0) type=ATT_INT8;
    else if (specif.precision==1) type=ATT_INT16;
    else if (specif.precision==2) type=ATT_INT32;
    else if (specif.precision==3) type=ATT_INT64;
  }
  return type;
}

void table_designer::convert_for_sql(void)
{
  for (int i=0;  i<edta.attrs.count();  i++)
  { atr_all * cur_atr = edta.attrs.acc0(i);
    if (cur_atr->state==ATR_DEL) continue;
    convert_type_for_sql_1(type_list, cur_atr->type, cur_atr->specif);
   // set implicit default value (must be after converting specif but before convering the type):
    if (cur_atr->state==ATR_NEW || cur_atr->state==ATR_MODIF)
      if (!cur_atr->expl_val)
      { wxString conv = get_default_conversion(cur_atr, (char*)edta.names.acc0(i));
        if (!conv.IsEmpty())
        { corefree(cur_atr->alt_value);
          cur_atr->alt_value=(tptr)sigalloc((int)conv.Length()+1, 62);
          if (cur_atr->alt_value)
            strcpy(cur_atr->alt_value, conv.mb_str(*cdp->sys_conv));
        }
      }
   // convert the type:
    cur_atr->type = convert_type_for_sql_2(type_list, cur_atr->type, cur_atr->specif);
  }
}

void table_designer::prepare_table_def_editing(void)
// Prepares the table definition if edta for editing
{ int i;
  inval_table_d(cdp, objnum, CATEG_TABLE);
  inval_table_d(cdp, NOOBJECT, CATEG_CURSOR); // cursors may depend on the table
 // delete system columns:
  edta.attrs.delet(0);  edta.names.delet(0);
  while (!memcmp(edta.attrs.acc(0)->name, "_W5_", 4))
    { edta.attrs.delet(0);   edta.names.delet(0); }
 // delete system indicies:
  while (edta.indxs.count() && !memcmp(edta.indxs.acc(0)->constr_name, "_W5_", 4))
      edta.indxs.delet(0);
 // mark existing columns as "old" and store their current types:
  for (i=0;  i<edta.attrs.count();  i++)
  { atr_all * cur_atr = edta.attrs.acc0(i);
    cur_atr->state=ATR_OLD;  cur_atr->alt_value=NULL;
    cur_atr->orig_type=cur_atr->type;
    cur_atr->expl_val=false;
  }
 // list of column names in the update page:
  if (upd_page)
    upd_page->mList->Clear();
 // convert data:
  convert_for_edit();
 // find single-column indexes and create the list of original columns:
  for (i=0;  i<edta.attrs.count();  i++)
  { atr_all * cur_atr = edta.attrs.acc0(i);
#ifndef DUPLICATE_INDEX_DEF
    ind_descr * indx = get_index_by_column(cdp, &edta, cur_atr->name);
    if (indx) indx->index_type |= 0x80;
#endif
    if (upd_page) upd_page->mList->Append(wxString(cur_atr->name, *cdp->sys_conv));
  }
 // insert existing data into grids:
  column_grid->InsertRows(0, edta.attrs.count());
  if (index_list_grid)
    index_list_grid->InsertRows(0, index_list_grid_table->GetNumberRows()-1);  // inserting the existing rows
  if (check_list_grid)
    check_list_grid->InsertRows(0, check_list_grid_table->GetNumberRows()-1);  // inserting the existing rows
  if (ref_list_grid)
    ref_list_grid  ->InsertRows(0,   ref_list_grid_table->GetNumberRows()-1);  // inserting the existing rows
 // enable parts of main grid and prepare chices based of types (types are converted now):
  for (i=0;  i<edta.attrs.count();  i++)
  { atr_all * cur_atr = edta.attrs.acc0(i);
    enable_by_type(i, cur_atr);
  }
}

void competing_frame::set_caption(wxString caption)
{ 
  if (dsgn_mng_wnd) // popup or MDI
    dsgn_mng_wnd->SetTitle(caption);
  else // child or aui_nb
    if (global_style==ST_AUINB)
    { int page = aui_nb->GetPageIndex(focus_receiver->GetParent());
      if (page != wxNOT_FOUND)  // unless a child
       aui_nb->SetPageText(page, caption);
    }
}

void table_designer::set_designer_caption(void)
{ wxString caption;
  if (!modifying()) caption=_("Design a New Table");
  else caption.Printf(_("Editing Design of Table %s"), wxString(edta.selfname, *cdp->sys_conv).c_str());
  set_caption(caption);
}

bool table_designer::open(wxWindow * parent, t_global_style my_style)
// Opens the table designer windows a children of the parent window.
{ 
  int client_width, client_height;
  parent->GetClientSize(&client_width, &client_height);
 // init:
  if (ce_buffer) objnum = *ce_buffer==0 ? NOOBJECT : 0;  // objnum defined because modified() must work
  WB_table=true;  // used when creating pages
  type_list=wb_type_list;  // used when creating the grid
  upd_page = NULL;
  hint_page = NULL;
  column_grid=index_list_grid=check_list_grid=ref_list_grid=NULL;
 // create the splitter:
  if (!Create(parent, COMP_FRAME_ID, wxPoint(0, 0), wxSize(client_width, client_height), wxSP_BORDER, wxT("TableDesignerMainSplitter")))
    return false;
  competing_frame::focus_receiver = this;
 // create split windows:
  column_grid=create_grid(this);
  property_notebook = new wxNotebook();
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>=6
  property_notebook->Create(this, 2, wxDefaultPosition, wxDefaultSize, wxNB_TOP);  // wxSIMPLE_BORDER causes bad painting of the page contents: black lines at the right and bottom border (error in the notebook sizer perhaps)
#else
  property_notebook->Create(this, 2, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER);  // wxSIMPLE_BORDER makes the sash visible
#endif
 // create page contens:
  cbd_page = new ColumnBasicsDlg(property_notebook);
 // create pages:
  property_notebook->AddPage(cbd_page, _("Properties of the column"));
  if (!modifying() || WB_table)
  { property_notebook->AddPage(new IndexListDlg(property_notebook), _("Indexes"));
    property_notebook->AddPage(new CheckListDlg(property_notebook), _("Checks"));
    if (WB_table)
    { property_notebook->AddPage(new RefListDlg(property_notebook), _("Referential integrity"));
      hint_page = new ColumnHintsDlg(property_notebook);
      property_notebook->AddPage(hint_page, _("Grid"));
      if (modifying())
      { upd_page = new MoveColValueDlg(property_notebook);
        property_notebook->AddPage(upd_page, _("Move column values"));
      }
    }
  }
  SplitHorizontally(column_grid, property_notebook);
 // Set this to prevent unsplitting:
  SetMinimumPaneSize(20);

  changed=false;

  current_row_selected=-1;  // must be before inserting rows into the grid, otherwise saves data into an undefined location!
  if (modifying()) // modifying an existing table
  { //if (!ce_buffer && IS_ODBC_TABLE(objnum)) // ODBC table
    //{ if (!tde->get_ODBC_table_structure()) goto error_on_start; }
    //else // WinBase table
      if (!compile_WB_table_structure()) return false;
    prepare_table_def_editing();
  }
  else
    init_new_table(NULL);
 // from now, if modifying(), then the definiton is locked
  disable_cells_in_fictive_record(column_grid->GetNumberRows()-1);
  update_local_key_choice();
  for (int row = 0;  row<ref_list_grid_table->GetNumberRows()-1;  row++)
  { forkey_constr * forkey = get_forkey(row);
    update_remote_key_choice(forkey->desttab_name, row);
  }
 // initial selection:
  wxGridEvent event(0, wxEVT_GRID_SELECT_CELL, column_grid, 0, 0);
  OnSelectCell(event); // not done by SetFocus() nor by the creation of the grid!
  column_grid->SetFocus();  // defines initial focus in the control container, tabledsgn->SetFocus() will restore it
  return true;
}

void table_designer::OnRangeSelect(wxGridRangeSelectEvent & event)
// Synchonizes selected cell with the selected range. 
// DROP table column depends on the selected cell, but user may have selected a row in the design,...
{ if (event.Selecting())
    column_grid->SetGridCursor(event.GetTopRow(), column_grid->GetGridCursorCol());
  event.Skip();
}

void table_designer::OnSelectCell( wxGridEvent & event)
{ if (event.GetRow() != current_row_selected)
  { char * cur_name;  atr_all * cur_atr;
    if (current_row_selected!=-1)
    { cur_atr = find_column_descr(current_row_selected, &cur_name);
      if (!save_column_parameters(cur_atr, cur_name))
        { event.Veto();  return; }
    }
    cur_atr = find_column_descr(event.GetRow(), &cur_name);
    show_column_parameters(cur_atr, cur_name);
    current_row_selected = event.GetRow();
  }
  event.Skip();
}

static char * contains_name(cdp_t cdp, char * text,  const char * name)
// Returns pointer to the 1st case-insensitive occurence of [name] in [text], or NULL if none found.
{ do
  {// go to start of a identifier:
    bool quoted = false;
    while (*text)
    { if (*text=='`') { text++;  quoted=true;  break; }
      if (is_AlphanumA(*text, cdp->sys_charset) && (unsigned char)*text>'9') break;
      text++;
    }
    if (!*text) break;
   // copy the identifier (or part of it) and count the full length:
    char nm[ATTRNAMELEN+1];  int i=0, j=0;  bool over=false;
    if (quoted)
    { while (text[i] && text[i]!='`')
      { if (i<ATTRNAMELEN) nm[j++]=text[i];  else over=true;
        i++;
      }
      if (text[i]=='`') i++;
    }
    else
      while (is_AlphanumA(text[i], cdp->sys_charset))
      { if (i<ATTRNAMELEN) nm[j++]=text[i];  else over=true;
        i++;
      }
   // compare:
    if (!over)
    { nm[j]=0;
      if (!my_stricmp(nm, name)) return text;
    }
   // goto to next:
    text+=i;
  } while (true);
  return NULL;
}

static bool replace_name(cdp_t cdp, char ** text, const char * oldname, const char * newname)
{ if (*text==NULL) return false;
  bool replaced = false;
  int ol=(int)strlen(oldname), nl=(int)strlen(newname);  
  tptr start=*text;
  do
  { char * occ = contains_name(cdp, start, oldname);
    if (occ==NULL) break;
    int offset=occ-*text;
   // replace:
    if (nl>ol)
    { tptr x=(tptr)corerealloc(*text, (int)strlen(*text)+1+nl-ol);
      if (!x) break; // error not reported, occurence not replaced
      *text=x;
    }
    memmov(*text+offset+nl, *text+offset+ol, strlen(*text)-offset-ol+1);
    memcpy(*text+offset, newname, nl);
    start=*text+offset+nl;
    replaced=true;
  } while (true);
  return replaced;
}

bool table_designer::search_column_name(const char * name, int mode)
// Searches all constrains for the column name. 
// If [mode==2] then deletes all constrains which contain it.
// If [mode==1] then deletes only single-column indexes which contain it.
// If [mode==0] then reports, if any contrain contains it.
{ int i;  bool reset;
  reset=false;
  for (i=0;  i<edta.indxs.count();  i++)
  { ind_descr * cur_c=edta.indxs.acc0(i);
    if (cur_c->state!=ATR_DEL && contains_name(cdp, cur_c->text, name))
      if (mode==2
#ifndef DUPLICATE_INDEX_DEF
          || mode==1 && (cur_c->index_type & 0x80)
#endif
         ) { edta.indxs.delet(i);  i--;  reset=true; }
      else return true;
  }
  if (reset && index_list_grid) index_list_grid->ForceRefresh();
  reset=false;
  for (i=0;  i<edta.checks.count();  i++)
  { check_constr * cur_c=edta.checks.acc0(i);
    if (cur_c->state!=ATR_DEL && contains_name(cdp, cur_c->text, name))
      if (mode==2) { edta.checks.delet(i);  i--;  reset=true; }
      else return true;
  }
  if (reset && check_list_grid) check_list_grid->ForceRefresh();
  reset=false;
  for (i=0;  i<edta.refers.count();  i++)
  { forkey_constr * cur_c=edta.refers.acc0(i);
    if (cur_c->state!=ATR_DEL && contains_name(cdp, cur_c->text, name))
      if (mode==2) { edta.refers.delet(i);  i--;  reset=true; }
      else return true;
  }
  if (reset && ref_list_grid) ref_list_grid->ForceRefresh();
  return false;  // not found, if searching, ignored if deleting
}

void table_designer::replace_column_name(const char * oldname, const char * newname)
{ int i;  
  for (i=0;  i<edta.indxs.count();  i++)
  { ind_descr * cur_c=edta.indxs.acc0(i);
    if (cur_c->state!=ATR_DEL)
      if (replace_name(cdp, &cur_c->text, oldname, newname) && index_list_grid) 
        index_list_grid->ForceRefresh();
  }
  for (i=0;  i<edta.checks.count();  i++)
  { check_constr * cur_c=edta.checks.acc0(i);
    if (cur_c->state!=ATR_DEL)
      if (replace_name(cdp, &cur_c->text, oldname, newname) && check_list_grid) 
        check_list_grid->ForceRefresh();
  }
  for (i=0;  i<edta.refers.count();  i++)
  { forkey_constr * cur_c=edta.refers.acc0(i);
    if (cur_c->state!=ATR_DEL)
      if (replace_name(cdp, &cur_c->text, oldname, newname) && ref_list_grid) 
        ref_list_grid->ForceRefresh();
  }
}

char * table_designer::make_source(bool alter)
// Returns the SQL statement for creating/modifying the table.
// Caller must corefree() the returned pointer.
{ convert_for_sql();
#if 0
  char * src=odbc_table_to_source(&edta, alter, tbdf_conn);
#else
  char * src=table_all_to_source(&edta, alter);
#endif
  convert_for_edit();
  return src;
}

int table_designer::check_consistency(bool explic)
// Check the consistency of the table definition
// Returns: 0-error (reported here), 1-OK (caller may report it), 2-cannot check.
{ BOOL err;
  if (!explic && !IsChanged()) return 1;  // explic test added, no reaction is confusing
#if 0
  if (!WB_table)
  { char dbms_name[20];
    SQLGetInfo(tbdf_conn->hDbc, SQL_DBMS_NAME, dbms_name, sizeof(dbms_name), NULL);
    if (!strnicmp(dbms_name, "Oracle", 6)) return 2;  /* SQLPrepare creates the table! */
  }
#endif  
  char * src=make_source(modifying());
  if (src==NULL) return 2;  // error reported
  if (WB_table)
  { uns32 optval;
   // source starts with a space:
    src[0]=(char)QUERY_TEST_MARK;
    if (modifying())
    { cd_Get_sql_option(cdp, &optval);
      cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, SQLOPT_OLD_ALTER_TABLE);  // old ALTER TABLE selected
    }
    { t_temp_appl_context tac(cdp, schema_name);
      err=cd_SQL_execute(cdp, src, NULL);
    }
    corefree(src);
    if (err)  /* report error: */
      { if (explic) cd_Signalize(cdp);  else stat_signalize(cdp); }
    else if (cd_Sz_warning(cdp)==ERROR_IN_CONSTRS)
      { if (explic) error_box(CONSTRAINS_HAVE_ERROR);  else stat_error(CONSTRAINS_HAVE_ERROR);  err=TRUE; }
    else if (cd_Sz_warning(cdp)==ERROR_IN_DEFVAL)
      { if (explic) error_box(DEFVALS_HAVE_ERROR);  else stat_error(DEFVALS_HAVE_ERROR);  err=TRUE; }
    if (modifying()) cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, optval);  // must not be before checking the error from ALTER TABLE
    if (err) return 0;
  }
#if 0
  else
  { RETCODE retcode=SQLPrepare(tbdf_conn->hStmt, (UCHAR*)src, SQL_NTS);
    corefree(src);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
      { odbc_stmt_error(cdp, tbdf_conn->hStmt);  return 0; }
  }
#endif
  if (explic) info_box(_("The design is correct."));
  else stat_error(wxEmptyString);
  return 1;
}

/////////////////////////// virtual methods (commented in class any_designer): //////////////////////////////////
//IMPLEMENT_DYNAMIC_CLASS2(table_designer, wxSplitterWindow, any_designer)

bool table_designer::save_design(bool save_as)
// Returns true on success, false when user cancelled entring the name or error occurred.
{ char * src;  BOOL res;
  if (!IsChanged() && !save_as) return true;  // empty action
  uns16 flags = edta.tabdef_flags & TFL_ZCR ? CO_FLAG_REPLTAB : 0;
 // creating a new table:
  if (!modifying() || save_as)
  { tobjname new_name;  
    if (save_as || !*edta.selfname || !strcmp(edta.selfname, FICTIVE_TABLE_NAME)) // save as or name specified outside
    { *new_name = 0;
      if (!get_name_for_new_object(cdp, dialog_parent(), schema_name, category, _("Enter the name of the new table:"), new_name))
        return false;  // entering the name cancelled
    }
    else strcpy(new_name, edta.selfname);  // original name of an existing table or proposed name for a new table

   // storing the definition or creating the table:
    tobjname saved_orig_name;  strcpy(saved_orig_name, edta.selfname);  strcpy(edta.selfname, new_name);
    src=make_source(false);
    if (!src) goto error_creating;
    if (ce_buffer!=NULL) // saving to buffer
    { if (strlen(src) >= ce_bufsize) { no_memory();  corefree(src);  goto error_creating; }
      strcpy(ce_buffer, src);
      corefree(src);
    }
    else if (WB_table)
    { trecnum tablerec;
      { t_temp_appl_context tac(cdp, schema_name);
        res=cd_SQL_execute(cdp, src, &tablerec);
        if (res) cd_Signalize(cdp); 
        else if (cd_Sz_warning(cdp)==ERROR_IN_CONSTRS)
          info_box(CONSTRAINS_HAVE_ERROR);
        else if (cd_Sz_warning(cdp)==ERROR_IN_DEFVAL)
          info_box(DEFVALS_HAVE_ERROR);
      }
      corefree(src);
      if (res) goto error_creating; 
      unlock_the_object();  // important for SaveAs of an existing object
      objnum=tablerec;
      lock_the_object();
      Set_object_folder_name(cdp, objnum, category, folder_name);
      Set_object_flags(cdp, objnum, category, flags);
      Add_new_component(cdp, category, schema_name, edta.selfname, objnum, flags, folder_name, (ce_flags & DSGN_FLAG_DEVENV) !=0);
    }
#if 0
    else
    { RETCODE retcode=SQLExecDirect(tbdf_conn->hStmt, (UCHAR*)src, SQL_NTS);
      corefree(src);
      if (retcode!=SQL_SUCCESS) odbc_stmt_error(cdp, tbdf_conn->hStmt);
      if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) goto error_creating;
     /* create odbc_link and link it: */
      t_odbc_link odbc_link;
      odbc_link.link_type=LINKTYPE_ODBC;
      strcpy(odbc_link.dsn, tbdf_conn->dsn);
      strcpy(odbc_link.tabname, edta.selfname);
      *odbc_link.owner=*odbc_link.qualifier=0;
      tobjnum objnum;  // link object
      if (cd_Insert_object(cdp, edta.selfname, CATEG_TABLE|IS_LINK, &objnum))
        cd_Signalize(cdp);
      else
      { cd_Write_var(cdp, TAB_TABLENUM, objnum, OBJ_DEF_ATR, NOINDEX, 0, sizeof(t_odbc_link), &odbc_link);
        Set_object_folder_name(cdp, objnum, CATEG_TABLE, folder_name);
        objnum=Add_component(cdp, CATEG_TABLE, edta.selfname, objnum, TRUE);
      }
    }
#endif
    set_designer_caption();
    goto ex;  // the new name remains in edta.selfname
   error_creating:
    strcpy(edta.selfname, saved_orig_name);  // undo the efect of entring the name
    return false;
  }
  else  // modifying an existing table
  { src=make_source(true);
    if (!src) return false;
    if (ce_buffer!=NULL) // saving to buffer
    { if (strlen(src) >= ce_bufsize) { no_memory();  corefree(src);  return false; }
      strcpy(ce_buffer, src);
      corefree(src);
    }
    else if (WB_table)
    { uns32 optval;
      cd_Get_sql_option(cdp, &optval);
      cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, SQLOPT_OLD_ALTER_TABLE);  // old ALTER TABLE selected
      { t_temp_appl_context tac(cdp, schema_name);
        wxBeginBusyCursor();
        res=cd_SQL_execute(cdp, src, NULL);
        wxEndBusyCursor();
        corefree(src);
        if (res) cd_Signalize(cdp);
        else if (cd_Sz_warning(cdp) == ERROR_IN_CONSTRS)
          info_box(CONSTRAINS_HAVE_ERROR);
        else if (cd_Sz_warning(cdp) == ERROR_IN_DEFVAL)
          info_box(DEFVALS_HAVE_ERROR);
      }
      cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, optval);  // must not be before reporting the error from ALTER TABLE
      if (res) return false;
      Set_object_flags(cdp, objnum, category, flags);
      Changed_component(cdp, category, schema_name, edta.selfname, objnum, flags);
    }
#if 0
    else
    { wxBeginBusyCursor();
      RETCODE retcode=SQLExecDirect(tbdf_conn->hStmt, (UCHAR*)src, SQL_NTS);
      wxEndBusyCursor();
      corefree(src);
      if (retcode!=SQL_SUCCESS) odbc_stmt_error(cdp, tbdf_conn->hStmt);
      if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) return false;
    }
#endif
  }
 ex:
  inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  /* table created or modified */
  inval_table_d(cdp, NOOBJECT, CATEG_CURSOR); /* cursors may depend on he table */
  return true;
}

bool table_designer::IsChanged(void) const
{ return changed; }

wxString table_designer::SaveQuestion(void) const
{ return !modifying() ? 
    _("The table has not been created yet. Do you want to save the table?") : 
    _("Your changes have not been saved.\nDo you want to save the changes to the table?");
}

void table_designer::OnCommand( wxCommandEvent& event )
// Command processing
{
// Must save pending changes in the current column before the action (not necessary for TD_CPM_HELP and TD_CPM_OPTIONS): 
  if (column_grid    ->IsCellEditControlEnabled()) column_grid    ->DisableCellEditControl(); // commit cell edit
  if (index_list_grid->IsCellEditControlEnabled()) index_list_grid->DisableCellEditControl(); // commit cell edit
  if (check_list_grid->IsCellEditControlEnabled()) check_list_grid->DisableCellEditControl(); // commit cell edit
  if (  ref_list_grid->IsCellEditControlEnabled())   ref_list_grid->DisableCellEditControl(); // commit cell edit
  char * cur_name;  atr_all * cur_atr;
  if (current_row_selected!=-1)
  { cur_atr = find_column_descr(current_row_selected, &cur_name);
    save_column_parameters(cur_atr, cur_name);  // do not disable commands on save errors
  }
 // execute the command:
  switch (event.GetId())
  { case TD_CPM_SAVE:
    { if (!IsChanged()) break;  // no message if no changes
      if (!save_design(false)) break;  // error saving, break
     // notifying:
      for (competing_frame * curr_dsgn = competing_frame_list;  curr_dsgn;  curr_dsgn=curr_dsgn->next)
        curr_dsgn->table_altered_notif(objnum);
     // exit or reload ###
      destroy_designer(GetParent(), &tabledsng_coords);
      break;
    }
    case TD_CPM_SAVE_AS:
      if (!save_design(true)) break;  // error saving, break
     // exit or continue:
      destroy_designer(GetParent(), &tabledsng_coords);
      break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
    case TD_CPM_EXIT_FRAME:
      if (exit_request(this, true))
        destroy_designer(GetParent(), &tabledsng_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
      exit_request(this, false);
      destroy_designer(GetParent(), &tabledsng_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_INSCOL:
    { int row = column_grid->GetGridCursorRow();
      if (row<0 || row>=column_grid->GetNumberRows()-1) // nothing or fictive record selected
        { wxBell();  return; }
      atr_all * cur_atr = find_column_descr(row, &cur_name);
      if (!cur_atr) return;  // internal error
      int offset = cur_atr-edta.attrs.acc(0);
      cur_atr = edta.attrs.insert(offset); //attrs may have been reallocated
      edta.names.insert(offset);
      set_default(cur_atr);
      column_grid->InsertRows(row, 1);  // redraws only
      enable_by_type(row, cur_atr);
      set_row_colour(row, true);
      show_column_parameters(cur_atr, (char*)edta.names.acc0(offset));
      break;
    }
    case TD_CPM_DELCOL:
    { int row = column_grid->GetGridCursorRow();
      if (row<0 || row>=column_grid->GetNumberRows()-1) // nothing or fictive record selected
        { wxBell();  return; }
      atr_all * cur_atr = find_column_descr(row, &cur_name);
      if (!cur_atr) return;  // internal error
      int offset = cur_atr-edta.attrs.acc(0);
     // check to see if attribute is used in constrains
      if (*cur_atr->name)
        if (search_column_name(cur_atr->name, 0))
        { wx602MessageDialog md(dialog_parent(), 
            _("This column is referenced in one or more contrains.\nDo you want to delete these contrains as well?"), 
            STR_WARNING_CAPTION, wxYES_NO | wxCANCEL);
          int decision = md.ShowModal();
          if (decision==wxID_CANCEL) break; // dropping vetoed
#ifdef DUPLICATE_INDEX_DEF
          if (decision==wxID_YES)
            search_column_name(cur_atr->name, 2);
#else
          search_column_name(cur_atr->name, decision==wxID_YES ? 2 : 1);
#endif
        }
     // deleting:
      if (cur_atr->state==ATR_NEW)   /* removing a new attribute */
        { edta.attrs.delet(offset);  edta.names.delet(offset); }
      else   /* mark the attribute as deleted only */
        cur_atr->state=ATR_DEL;
      changed=true;
     // update the view:
      wxGridTableMessage msg(grid_table, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1);
      column_grid->ProcessTableMessage(msg);
      break;
    }
    case TD_CPM_SQL:
    { ShowTextDlg dlg(this);
      if (!IsChanged()) 
      { dlg.mText->SetValue(modifying() ? _("Design has not changed.") : _("Design has not been created."));
        dlg.ShowModal();
      }
      else
      { char * src=make_source(modifying());
        if (src) 
        { dlg.mText->SetValue(wxString(src, *cdp->sys_conv));
          dlg.ShowModal();
          corefree(src); 
        }
      }
      break;
    }
    case TD_CPM_CHECK:
      check_consistency(true);
      break;
    case TD_CPM_OPTIONS:
    { TablePropertiesDlg dlg(this);
      dlg.SetPrivils((edta.tabdef_flags & TFL_REC_PRIVILS)!=0);
#ifdef STOP  // hidden
      dlg.SetJournal((edta.tabdef_flags & TFL_NO_JOUR)==0);
#endif
      dlg.SetZcr((edta.tabdef_flags & TFL_ZCR)!=0);
      dlg.SetUnikey((edta.tabdef_flags & TFL_UNIKEY)!=0);
      dlg.SetLuo((edta.tabdef_flags & TFL_LUO)!=0);
      dlg.SetDetect((edta.tabdef_flags & TFL_DETECT)!=0);
      ((wxCheckBox*)dlg.FindWindow(CD_TFL_UNIKEY))->Enable(dlg.GetZcr());
      ((wxCheckBox*)dlg.FindWindow(CD_TFL_LUO))->Enable(dlg.GetZcr());
      ((wxCheckBox*)dlg.FindWindow(CD_TFL_DETECT))->Enable(dlg.GetZcr());
      if (dlg.ShowModal() == wxID_OK)
      { int flags=0;
        if (dlg.GetPrivils()) flags |= TFL_REC_PRIVILS;
#ifdef STOP  // hidden
        if (!dlg.GetJournal()) flags |= TFL_NO_JOUR;
#endif
        if (dlg.GetZcr()) 
        { flags |= TFL_ZCR;
          if (dlg.GetUnikey()) flags |= TFL_UNIKEY;
          if (dlg.GetLuo()) 
          { flags |= TFL_LUO;
            if (dlg.GetDetect()) flags |= TFL_DETECT;
          }
        }
        if (edta.tabdef_flags!=flags)
          { edta.tabdef_flags=flags;  changed=true; }
      }
      break;
    }
    case TD_CPM_HELP:
      wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_table.html"));  
      break;
  }
}

void table_designer::OnKeyDown( wxKeyEvent& event )
{ switch (event.GetKeyCode())
  { case 'W':  case WXK_F4:
      if (event.ControlDown())
      { wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, TD_CPM_EXIT)
        cmd.SetId( TD_CPM_EXIT);
        OnCommand(cmd);
        return;                                                                                                            }
      break;
    case WXK_RETURN:  // changing the defaut behaviour of the grid
    case WXK_NUMPAD_ENTER:
      if ( event.ControlDown() )
          event.Skip();  // to let the edit control have the return
      else
      { if ( column_grid->GetGridCursorCol() < column_grid->GetNumberCols()-1 )
        { column_grid->MoveCursorRight( event.ShiftDown() );
          return;
        }
      }
      break;
  }
  OnCommonKeyDown(event);  // event.Skip() is inside
}

void table_designer::OnSize(wxSizeEvent & event)
{ int width, height, pos;
  GetClientSize(&width, &height);
  pos=GetSashPosition();
  if (pos<height/5 || pos>height/5*4-10)
    SetSashPosition(height / 2);
  event.Skip();
}

#if 0  // not necessary, replaced by the standard focusing in the control container, but must give focus to a child when created
void table_designer::OnFocus(wxFocusEvent& event) 
{ 
  if (column_grid)
    column_grid->SetFocus();
  event.Skip();
}
#endif

table_designer::~table_designer(void)
{ remove_tools();
}

BEGIN_EVENT_TABLE( table_designer, wxSplitterWindow )
    EVT_GRID_SELECT_CELL( table_designer::OnSelectCell )
    EVT_GRID_RANGE_SELECT( table_designer::OnRangeSelect )
    EVT_KEY_DOWN(table_designer::OnKeyDown)
    EVT_SIZE(table_designer::OnSize)
    //EVT_SET_FOCUS(table_designer::OnFocus) 
END_EVENT_TABLE()

t_tool_info table_designer::td_tool_info[TD_TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,    File_save_xpm, wxTRANSLATE("Save Design")),
  t_tool_info(TD_CPM_EXIT,    exit_xpm,      wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_INSCOL,  InsertCol_xpm, wxTRANSLATE("Insert column")),
  t_tool_info(TD_CPM_DELCOL,  DeleteCol_xpm, wxTRANSLATE("Delete column")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CHECK,   validateSQL_xpm,     wxTRANSLATE("Validate")),
  t_tool_info(TD_CPM_OPTIONS, tableProperties_xpm, wxTRANSLATE("Table properties")),
  t_tool_info(TD_CPM_SQL,     showSQLtext_xpm,     wxTRANSLATE("Show SQL")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_HELP,    help_xpm,            wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

wxMenu * table_designer::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items: 
  any_designer::get_designer_menu();  
 // add specific items:
  AppendXpmItem(designer_menu, TD_CPM_OPTIONS, _("&Table Properties"), tableProperties_xpm);
  AppendXpmItem(designer_menu, TD_CPM_SQL,     _("Show S&QL"),         showSQLtext_xpm);
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_INSCOL,  _("&Insert Column"),    InsertCol_xpm);
  AppendXpmItem(designer_menu, TD_CPM_DELCOL,  _("&Delete Column"),    DeleteCol_xpm);
  return designer_menu;
}


////////////////////////////////////// common for all designers ////////////////////////////
competing_frame::competing_frame(void)
{ 
  toolbar_is_active=false;  
  designer_menu = NULL;
  focus_receiver = NULL;
  persistent_coords = NULL;
  dsgn_mng_wnd = NULL;
 // add to the competing_frame_list:
  next=competing_frame_list;
  competing_frame_list=this;
}

competing_frame::~competing_frame(void)
{ // if (active_tool_owner==this) remove_tools(); -- remove_tools must be called in the destructor of the derived class because it uses the pure virtual function get_tool_info()
 // remove from the competing_frame_list
  competing_frame ** pfrm = &competing_frame_list; 
  while (*pfrm && *pfrm!=this)
    pfrm = &(*pfrm)->next;
  if (*pfrm==this)
    *pfrm=next;
#ifndef RECREATE_MENUS
  if (designer_menu) delete designer_menu;
#endif
};

competing_frame * competing_frame_list = NULL;
competing_frame * competing_frame::active_tool_owner = NULL;
#ifdef FL
wxDynamicToolBar * competing_frame::designer_toolbar = NULL;
#else
wxToolBar * competing_frame::designer_toolbar = NULL;
#endif

IMPLEMENT_ABSTRACT_CLASS(competing_frame, wxObject)
IMPLEMENT_ABSTRACT_CLASS(any_designer, competing_frame)
extern const wxChar * bar_name[BAR_COUNT];

void competing_frame::add_tools(const wxChar * menu_name)
// Creates tools for the [competing_frame]: the Design menu and (for MDI) the common designer toolbar.
// Defines [active_tool_owner] for receiving the commands.
{ if (toolbar_is_active) return;
  if (active_tool_owner)
    { active_tool_owner->remove_tools();  active_tool_owner=NULL; }
 // put may tools on the toolbar:
#ifdef FL
  wxFrameLayout * fl = wxGetApp().frame->mpLayout;
  if (!fl) return;  // closing all
#endif
#if WBVERS>=1000
  if (global_style==ST_MDI)
#endif
  { if (!designer_toolbar) return;
    for (t_tool_info * ti = get_tool_info();  ti->command;  ti++)
#ifdef FL
      if (ti->command==-1)
        designer_toolbar->AddSeparator(new wxMySeparatorLine(designer_toolbar, -1));
      else
      { if (!ti->bitmap_created)
          { ti->bmp=new wxBitmap(ti->xpm);  ti->bitmap_created=true; }
        designer_toolbar->AddTool(ti->command, *ti->bmp, wxNullBitmap, false, -1, -1, NULL, wxGetTranslation(ti->tooltip));
        //designer_toolbar->wxToolBarBase::AddTool(ti->command, wxString(""), *ti->bmp, wxString(ti->tooltip), ti->kind);
      }
    designer_toolbar->SetLayout(designer_toolbar->CreateDefaultLayout());
#else
      if (ti->command==-1)
        designer_toolbar->AddSeparator();
      else
      { if (!ti->bitmap_created)
          { ti->bmp=new wxBitmap(ti->xpm);  ti->bitmap_created=true; }
        designer_toolbar->AddTool(ti->command, wxEmptyString, *ti->bmp, wxNullBitmap, ti->kind, wxGetTranslation(ti->tooltip));
      }
    designer_toolbar->Realize();
    //wxGetApp().frame->m_mgr.GetPane(bar_name[BAR_ID_DESIGNER_TOOLBAR]).BestSize(-1,-1).FloatingSize(-1,-1);
    wxGetApp().frame->m_mgr.DetachPane(designer_toolbar);
    wxGetApp().frame->m_mgr.AddPane(wxGetApp().frame->bars[BAR_ID_DESIGNER_TOOLBAR], wxAuiPaneInfo().Name(bar_name[BAR_ID_DESIGNER_TOOLBAR]).Caption(wxGetTranslation(bar_name[BAR_ID_DESIGNER_TOOLBAR])).ToolbarPane().Top().LeftDockable(false).RightDockable(false));
    wxGetApp().frame->m_mgr.Update();  // "commit" all changes made to wxAuiManager
#endif
    update_toolbar();
  }
 // add my menu:
  MyFrame * frame = wxGetApp().frame;
  wxMenuBar * MenuBar = frame->GetMenuBar();
  if (menu_name!=NULL) saved_menu_name=menu_name;
  int index  = MenuBar->FindMenu(DESIGNER_MENU_STRING);
  if (index==wxNOT_FOUND) index = MenuBar->FindMenu(DATA_MENU_STRING);
  if (index!=wxNOT_FOUND)
  {
#if 0 // def WINS  // index depends on the maximization of MDI children  -- only in 2.4.2
    int is_max; // MSW BOOL
    if (SendMessage((HWND)frame->mpClientWnd->GetHandle(), WM_MDIGETACTIVE, 0, (LPARAM)&is_max) && is_max)
      index++;
#endif
    wxMenu * old_menu = NULL;
    wxMenu * own_menu = get_designer_menu();  // creates or re-creates or returns created
    if (own_menu)  // frame with own menu
      old_menu = MenuBar->Replace(index, own_menu, saved_menu_name);
    else
    { if (!MenuBar->GetMenu(index)->FindItem(CPM_EMPTY_DESIGNER)) // not an empty designer menu
        old_menu = MenuBar->Replace(index, get_empty_designer_menu(), DESIGNER_MENU_STRING);
    }
#ifdef RECREATE_MENUS
    if (old_menu) delete old_menu;
#endif  
  }
  //focus_command_receiver=dsgn_mng_wnd;
 // note the active state:
  toolbar_is_active=true;
  active_tool_owner=this;
}

void competing_frame::remove_tools(void)
{ if (!toolbar_is_active) return;
  MyFrame * frame = wxGetApp().frame;
 // remove may tools from the toolbar:
#ifdef FL
  wxFrameLayout * fl = frame->mpLayout;
#if WBVERS>=1000
  if (global_mdi_style)
#endif
    if (fl && designer_toolbar)  // unless closing all
      for (t_tool_info * ti = get_tool_info();  ti->command;  ti++)
        designer_toolbar->RemveTool(ti->command);
#else
#if WBVERS>=1000
  if (global_style==ST_MDI)
#endif
    if (!frame->closing_all && designer_toolbar)  // unless closing all
      for (t_tool_info * ti = get_tool_info();  ti->command;  ti++)
        designer_toolbar->DeleteToolByPos(0);
#endif
 // remove my menu (must be done even if closing all and fl==NULL):
  wxMenuBar * MenuBar = frame->GetMenuBar();
  int index = MenuBar->FindMenu(DESIGNER_MENU_STRING);
  if (index==wxNOT_FOUND) index = MenuBar->FindMenu(DATA_MENU_STRING);
  if (index!=wxNOT_FOUND)
  {
#if 0 // def WINS  // index depends on the maximization of MDI children  -- only in 2.4.2
    int is_max; // MSW BOOL
    if (SendMessage((HWND)frame->mpClientWnd->GetHandle(), WM_MDIGETACTIVE, 0, (LPARAM)&is_max) && is_max)
      index++;
#endif
    wxMenu * old_menu = NULL;
    if (!MenuBar->GetMenu(index)->FindItem(CPM_EMPTY_DESIGNER)) // not an empty designer menu
      MenuBar->Replace(index, get_empty_designer_menu(), DESIGNER_MENU_STRING);
#ifdef RECREATE_MENUS
    if (old_menu) delete old_menu;
#endif  
  }
  //if (focus_command_receiver==dsgn_mng_wnd)
  //  focus_command_receiver=NULL;
 // note the inactive state:
  if (frame->GetStatusBar()) frame->SetStatusText(wxEmptyString);  // status bar may not exist when closing the APP
  toolbar_is_active=false;
  if (active_tool_owner==this) active_tool_owner=NULL;
}

void competing_frame::activate_other_competing_frame(void)
{ 
  if (global_style!=ST_MDI) return;  // otherwise cannot cast to wxMDIChildFrame!
  for (competing_frame * an_dsgn = competing_frame_list;  an_dsgn;  an_dsgn=an_dsgn->next)
    if (an_dsgn!=this && an_dsgn->cf_style!=ST_CHILD && an_dsgn->cf_style!=ST_EDCONT && an_dsgn->dsgn_mng_wnd)
    { ((wxMDIChildFrame*)an_dsgn->dsgn_mng_wnd)->Activate();  // changing tools before closing the window because the notification are sometimes not delivered
      break;
    }
}

void competing_frame::destroy_designer(wxWindow * dsgnmain, t_designer_window_coords * coords)
// In fact, this function is used by all competing frames, not only designers
{ if (coords && global_style==ST_POPUP)
  { wxTopLevelWindow * parent_frame = wxDynamicCast(dsgnmain, wxTopLevelWindow);
    if (parent_frame && !parent_frame->IsIconized() && !parent_frame->IsMaximized())
    { coords->pos=dsgnmain->GetPosition();
      coords->size=dsgnmain->GetSize();
      coords->coords_updated=true;  // valid and should be saved
    }
  }
  if (global_style==ST_MDI)
    activate_other_competing_frame();
  if (global_style==ST_AUINB)
  { wxAuiNotebookEvent evt(wxEVT_COMMAND_AUINOTEBOOK_BUTTON, 0);
    evt.SetEventObject(aui_nb->GetActiveTabCtrl());
    evt.SetInt(wxAUI_BUTTON_CLOSE);
    aui_nb->OnTabButton(evt);
    //aui_nb->DeletePage(aui_nb->GetPageIndex(dsgnmain));  -- after closing the last tab the AUI_NB does not paint its background
    //aui_nb->normalize_offset();                          -- after closing the last tab the AUI_NB does not paint its background
  }
  else
    dsgnmain->Destroy();
}

void competing_frame::Activated(void)
{ }

void competing_frame::Deactivated(void)
{ }

any_designer::~any_designer(void)
{
  unlock_the_object();  // Unlocks the underlying object, if locked
#ifdef OWN_TOOLBARS
#ifdef FL
  wxFrameLayout * fl = wxGetApp().frame->mpLayout;
  if (!fl) return;  // application closing
  if (designer_toolbar)  // remove it from the layout and destroy
  { if (toolbar_is_active)
    { cbBarInfo * barinfo = fl->FindBarByWindow(designer_toolbar);
      if (barinfo) fl->RemoveBar(barinfo);
    }
    designer_toolbar->Destroy();
    fl->RefreshNow(true);  // removes the bar's handle etc from the row
  }
#endif
#else
  //remove_tools(); -- calls the pure virtual function, must be in destructors of specific designers
#endif
}

bool any_designer::lock_the_object(void)  // Returns false iff cannot lock
{ if (objnum==NOOBJECT) return false;  // just a fuse
  if (!read_only_mode && !object_locked) // object may have been locked before (fsed does so)
  { if (cd_Write_lock_record(cdp, cursnum, objnum)) return false;
    object_locked=true;
  }
  return true;
}

void any_designer::unlock_the_object(void)
{ if (modifying() && object_locked) 
    cd_Write_unlock_record(cdp, cursnum, objnum);
  object_locked=false;
}

void any_designer::Activated(void)
{ 
#ifdef OWN_TOOLBARS
#ifdef FL
  wxFrameLayout * fl = wxGetApp().frame->mpLayout;
  if (!fl) return;  // application closing
  if (designer_toolbar && !toolbar_is_active)  // show
  { cbBarInfo * barinfo = fl->FindBarByWindow(designer_toolbar);
    if (barinfo)
		{ int newState = 0;
		  if ( barinfo->mAlignment == -1 )
			{	barinfo->mAlignment = 0;       // just remove "-1" marking
			 newState = wxCBAR_FLOATING;
			}
			else
			if ( barinfo->mAlignment == FL_ALIGN_TOP || barinfo->mAlignment == FL_ALIGN_BOTTOM )
			  newState = wxCBAR_DOCKED_HORIZONTALLY;
			else
			  newState = wxCBAR_DOCKED_VERTICALLY;
      fl->SetBarState( barinfo, newState, TRUE );
		  if ( newState == wxCBAR_FLOATING ) fl->RepositionFloatedBar( barinfo ); 
      fl->RefreshNow(true);  // removes the bar's handle etc from the row
    }
    toolbar_is_active=true;
  }
#endif
#else
  add_tools();
  write_status_message();
#endif
}

void any_designer::Deactivated(void)
{ 
#ifdef OWN_TOOLBARS
#ifdef FL
  wxFrameLayout * fl = wxGetApp().frame->mpLayout;
  if (!fl) return;  // application closing
  if (designer_toolbar && toolbar_is_active)  // hide
  { cbBarInfo * barinfo = fl->FindBarByWindow(designer_toolbar);
    if (barinfo)
    { if ( barinfo->mState == wxCBAR_FLOATING ) barinfo->mAlignment = -1;
      fl->SetBarState( barinfo, wxCBAR_HIDDEN, TRUE );
      fl->RefreshNow(true);  // removes the bar's handle etc from the row
    }
    toolbar_is_active=false;
  }
#endif
#else
//  remove_tools();  -- no more
#endif
}

void any_designer::write_status_message(void)
{ wxString msg, catname;
  wchar_t wc[64];
  if (!cdp) return;  // used by the editor_container
  catname = CCategList::Lower(cdp, category, wc);
/*  switch (category)
  { case CATEG_TABLE: catname="table";  break;
    case CATEG_DOMAIN: catname="domain";  break;
    case CATEG_SEQ: catname="sequence";  break;
    case CATEG_CURSOR: catname="query";  break;
    case CATEG_TRIGGER: catname="trigger";  break;
    case CATEG_PROC: catname="procedure";  break;
    case CATEG_PGMSRC: catname="transport";  break;
    case CATEG_DRAWING: catname="diagram";  break;
    default: catname="object";  break;
  } */
  if (!modifying())
    msg.Printf(_("Design a new %s in %s.%s"), catname.c_str(), (const wxChar*)wxString(cdp->locid_server_name, *cdp->sys_conv), (const wxChar*)wxString(schema_name, *cdp->sys_conv));
  else
    msg.Printf(_("Design of the %s in %s.%s"), catname.c_str(), (const wxChar*)wxString(cdp->locid_server_name, *cdp->sys_conv), (const wxChar*)wxString(schema_name, *cdp->sys_conv));
  if (wxGetApp().frame->GetStatusBar()) wxGetApp().frame->SetStatusText(msg);
}

wxMenu * any_designer::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // Creates the designer-specific menu and adds the common items
  designer_menu = new wxMenu;
  AppendXpmItem(designer_menu, TD_CPM_SAVE,        _("&Save Design\tCtrl+S"), File_save_xpm);
  AppendXpmItem(designer_menu, TD_CPM_SAVE_AS,     _("Save Design &As\tCtrl+Shift+S"));
  designer_menu->AppendSeparator();
#ifdef LINUX
  AppendXpmItem(designer_menu, TD_CPM_EXIT,        _("Close"),        exit_xpm);  // \tCtrl+W no shortcut, because usually does not work
#else
  AppendXpmItem(designer_menu, TD_CPM_EXIT, global_style==ST_POPUP ? _("Close\tAlt+F4") : _("Close\tCtrl+F4"), exit_xpm);  // shortcuts are not reliable but these system shortcuts work
#endif
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_CHECK,       _("&Validate"),            validateSQL_xpm);
  return designer_menu;
}

#define SQL_MANIPULATED_CATEGORY(category) ((category)==CATEG_SEQ || (category)==CATEG_DOMAIN || (category)==CATEG_INFO)

void component_change_notif(cdp_t cdp, tcateg category, const char * object_name, const char * schema_name, int flags, tobjnum objnum)
{ if (!SQL_MANIPULATED_CATEGORY(category))
  { uns32 stamp = stamp_now();
    cd_Write(cdp, OBJ_TABLENUM, objnum, OBJ_MODIF_ATR,  NULL, &stamp, sizeof(uns32));
  }
  Changed_component(cdp, category, schema_name, object_name, objnum, flags);
}

bool any_designer::save_design_generic(bool save_as, char * object_name, int flags)
// Generic save: Caller must provide the pointer to the object_name. If true returned, caller must clear the "changed" flag.
{ bool adding_new_object = false;  BOOL err;  tobjnum save_objnum;
  if (!IsChanged() && !save_as) return true;  // no action
  if (!TransferDataFromWindow()) return false;  // used e.g. by the domain designer
  if (!modifying() || save_as)
  { const wxChar * prompt =
      category==CATEG_DOMAIN ? _("Enter the name of the new domain:") :
      category==CATEG_SEQ    ? _("Enter the name of the new sequence:") :
      category==CATEG_CURSOR ? _("Enter the name of the new query:") :
      category==CATEG_PGMSRC ? _("Enter the name of the new program:") :
      category==CATEG_DRAWING? _("Enter the name of the new diagram:") :
                               _("Enter the name of the new object:");
    if (SQL_MANIPULATED_CATEGORY(category))
    { if (!get_name_for_new_object(cdp, dialog_parent(), schema_name, category, prompt, object_name))
        return false;  // entering the name cancelled
    }
    else
    { if (!edit_name_and_create_object(cdp, dialog_parent(), schema_name, category, prompt, &save_objnum, object_name)) 
        return false;  // cancelled
    }
    adding_new_object=true;
  }
  else save_objnum=objnum;
  char * src=make_source(modifying() && !save_as);
  if (!src) { no_memory(); return false; }
  { t_temp_appl_context tac(cdp, schema_name);
    if (SQL_MANIPULATED_CATEGORY(category))
    { wxBeginBusyCursor();
      err=cd_SQL_execute(cdp, src, NULL);
      wxEndBusyCursor();
    }
    else
      err=cd_Write_var(cdp, OBJ_TABLENUM, save_objnum, OBJ_DEF_ATR, NOINDEX, 0, (int)strlen(src), src) ||
          cd_Write_len(cdp, OBJ_TABLENUM, save_objnum, OBJ_DEF_ATR, NOINDEX,    (int)strlen(src));
  }
  corefree(src);
  if (err) { cd_Signalize(cdp);  return false; }
  if (adding_new_object)
  { unlock_the_object();  // important for SaveAs of an existing object
    if (SQL_MANIPULATED_CATEGORY(category))
    { if (cd_Find_prefixed_object(cdp, schema_name, object_name, category, &objnum))
        { cd_Signalize(cdp);  return false; }  // internal error
    }
    else
      objnum=save_objnum;
    lock_the_object();
    Set_object_folder_name(cdp, objnum, category, folder_name);
    if (flags) Set_object_flags(cdp, objnum, category, flags);  // storing the initial flags
    Add_new_component(cdp, category, schema_name, object_name, objnum, flags, folder_name, true);
  }
  else
    component_change_notif(cdp, category, object_name, schema_name, flags, objnum);
  return true;
}

bool any_designer::exit_request(wxWindow * parent, bool can_veto)
// If there are unsaved changes, asks user if they should be saved (and possibly saves).
// Returns true if exit confirmed.
{ if (!IsChanged()) return true;
  wx602MessageDialog md(dialog_parent(), SaveQuestion(), STR_WARNING_CAPTION, can_veto ? wxYES_NO | wxCANCEL : wxYES_NO);
  int decision = md.ShowModal();
  if (decision==wxID_CANCEL) return false; // exit vetoed
  if (decision==wxID_NO) return true;  // exit confirmed, without saving
 // save:
  if (save_design(false)) return true;  // saved OK
  return !can_veto;  // saving not successfull, if can veto then prevent exiting
}

void any_designer::OnDisconnecting(ServerDisconnectingEvent & event)
// 1. Check if this designer should be closed 
// 2. If yes, tries to save the changes.
{ if (event.xcdp && event.xcdp->get_cdp()!=cdp) 
    event.ignored=true;
  else if (!exit_request(wxGetApp().frame, true))  // cannot call OnCommand because wxCommandEvent does not say if it was vetoed
    event.cancelled=true;
}

bool any_designer::OnCommonKeyDown(wxKeyEvent & event)
// Processing of common keys in designers, must be explicitly called in OnKeyDown() of the designer.
{ switch (event.GetKeyCode())
  { case WXK_F4:
      if (event.ControlDown())
      { wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, TD_CPM_EXIT)
        cmd.SetId(TD_CPM_EXIT);
        OnCommand(cmd);
        event.Skip(false);  // must block further processing because the window has been destroyed!
        return true;
      }
      break;
    case 'S':
      if (event.ControlDown())
      { wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, TD_CPM_EXIT)
        cmd.SetId(event.ShiftDown() ? TD_CPM_SAVE_AS : TD_CPM_SAVE);
        OnCommand(cmd);
        return true;
      }
      break;
  }
 // not processed:
  event.Skip();
  return false;
}

///////////////////////////// frame for a designer /////////////////////////////////////////////////
BEGIN_EVENT_TABLE( DesignerFrameMDI, wxMDIChildFrame )
    EVT_MENU(-1, DesignerFrameMDI::OnCommand)
    EVT_CLOSE(DesignerFrameMDI::OnCloseWindow)
    EVT_ACTIVATE(DesignerFrameMDI::OnActivate)
    EVT_SET_FOCUS(DesignerFrameMDI::OnFocus) 
    EVT_CUSTOM(disconnect_event, 0, DesignerFrameMDI::OnDisconnecting)
    EVT_CUSTOM(record_update, 0, DesignerFrameMDI::OnRecordUpdate)
    EVT_SIZE(DesignerFrameMDI::OnSize)
END_EVENT_TABLE()

void DesignerFrameMDI::OnCommand( wxCommandEvent& event )
{ 
  dsgn->OnCommand(event); 
}

void DesignerFrameMDI::OnFocus(wxFocusEvent& event) 
{ 
  if (dsgn /*&& dsgn->GetHandle()*/)  // constructed and creted
    dsgn->_set_focus();
  event.Skip();
}

void DesignerFrameMDI::OnCloseWindow(wxCloseEvent& event)
{ wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND)
  cmd.SetId(event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND);
  dsgn->OnCommand(cmd);
}

void DesignerFrameMDI::OnActivate(wxActivateEvent & event)
{ if (!dsgn)
    return;
  if (event.GetActive())
    dsgn->Activated();
  else
    dsgn->Deactivated();
  event.Skip();
}

void DesignerFrameMDI::OnDisconnecting(wxEvent & event)
{ ServerDisconnectingEvent * disev = (ServerDisconnectingEvent*)&event;
  if (dsgn) dsgn->OnDisconnecting(*disev); // the designer will ignore or save or veto
  if (!disev->cancelled && !disev->ignored) 
    Destroy();  
}

void DesignerFrameMDI::OnRecordUpdate(wxEvent & event)
{ RecordUpdateEvent * ruev = (RecordUpdateEvent*)&event;
  if (dsgn) dsgn->OnRecordUpdate(*ruev); // the designer will ignore or save or veto
}

void DesignerFrameMDI::OnSize(wxSizeEvent & event)
{ dsgn->OnResize(); event.Skip();}


#ifdef DSGN_IN_FRAME
BEGIN_EVENT_TABLE( DesignerFramePopup, wxFrame )
#else
BEGIN_EVENT_TABLE( DesignerFramePopup, wxDialog )
#endif    
    EVT_MENU(-1, DesignerFramePopup::OnCommand)
    EVT_CLOSE(DesignerFramePopup::OnCloseWindow)
    EVT_ACTIVATE(DesignerFramePopup::OnActivate)
    EVT_SET_FOCUS(DesignerFramePopup::OnFocus) 
    EVT_CUSTOM(disconnect_event, 0, DesignerFramePopup::OnDisconnecting)
    EVT_CUSTOM(record_update, 0, DesignerFramePopup::OnRecordUpdate)
    EVT_SIZE(DesignerFramePopup::OnSize)
END_EVENT_TABLE()

void DesignerFramePopup::OnCommand( wxCommandEvent& event )
{ dsgn->OnCommand(event); }

void DesignerFramePopup::OnFocus(wxFocusEvent& event) 
{ 
  if (dsgn)  // not tested if the window has been created - testing in _set_focus
    dsgn->_set_focus();
  event.Skip();
}

void DesignerFramePopup::OnCloseWindow(wxCloseEvent& event)
{ wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND)
  cmd.SetId(event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND);
  dsgn->OnCommand(cmd);
}

void DesignerFramePopup::OnActivate(wxActivateEvent & event)
{ if (event.GetActive())
    dsgn->Activated();
  else
    dsgn->Deactivated();
  event.Skip();
}

void DesignerFramePopup::OnDisconnecting(wxEvent & event)
{ ServerDisconnectingEvent * disev = (ServerDisconnectingEvent*)&event;
  if (dsgn) dsgn->OnDisconnecting(*disev); // the designer will ignore or save or veto
  if (!disev->cancelled && !disev->ignored) 
    Destroy();  
}

void DesignerFramePopup::OnRecordUpdate(wxEvent & event)
{ RecordUpdateEvent * ruev = (RecordUpdateEvent*)&event;
  if (dsgn) dsgn->OnRecordUpdate(*ruev); // the designer will ignore or save or veto
}

void DesignerFramePopup::OnSize(wxSizeEvent & event)
{ 
  dsgn->OnResize(); event.Skip();
}

void create_tools(wxToolBarBase * tb, t_tool_info * ti)
{ 
  if (!ti) return;  // empty editor container
  for (;  ti->command;  ti++)
    if (ti->command==-1)
      tb->AddSeparator();
    else
    { if (!ti->bitmap_created)
        { ti->bmp=new wxBitmap(ti->xpm);  ti->bitmap_created=true; }
      if (ti->kind==wxITEM_NORMAL)
        tb->AddTool     (ti->command, wxEmptyString, *ti->bmp,           wxGetTranslation(ti->tooltip));
      else
        tb->AddCheckTool(ti->command, wxEmptyString, *ti->bmp, *ti->bmp, wxGetTranslation(ti->tooltip)); 
    }
  tb->Realize();
  //tb->SetMinSize(wxSize(50,50));
}

void competing_frame::put_designer_into_frame(wxWindow * frame, wxWindow * dsgn)
// The designer is completely created. Join it with its frame and create the toolbar.
// AUI: The commands from the toolbar are directed to MyFrame and then to [active_tool_owner].
// Called only for the ST_AUINB and ST_POPUP styles.
{
#if WBVERS>=1000
  frame->SetSizer(new wxBoxSizer(wxVERTICAL));
  wxToolBar * tb = new wxToolBar(frame, OWN_TOOLBAR_ID, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxNO_BORDER | wxTB_NODIVIDER | wxTB_FLAT);
#ifdef LINUX // the editor container needs the height of the toolbarbefore any page is created
  tb->SetMinSize(wxSize(10,26));  // solves the Gnome problems with incomplete butons, too
#endif    
  tb->SetToolBitmapSize(wxSize(16,16));
  create_tools(tb, get_tool_info());
  //designer_toolbar->SetLayout(designer_toolbar->CreateDefaultLayout());
  update_toolbar();

  frame->GetSizer()->Add(tb, 0, wxGROW | wxBOTTOM | wxTOP | wxLEFT, 4);  // exblicit border on the bottom
  frame->GetSizer()->Add(dsgn, 1, wxGROW);
  frame->SetBackgroundColour(tb->GetBackgroundColour());  // colour of the border around the toolbar
  frame->Layout();
#endif
}

/* Common designer logic
Designer has:
- a copy of cdp,
- schema name for new object to be placed in (current schema may change during the operation),
- folder name for new object to be placed in (current folder may change during the operation),
- objnum if the object exists (NOOBJECT otherwise).
Designer may have:
- the object name for the new object.
Designer must create own toolbar on start and destroy it on exit.
On save the designer must create or update the component in caches and on the control panel.
*/
/////////////////////////////////// DOMAIN designer support ////////////////////////////////////////////////////////
#include "comp.h"

static void WINAPI domain_anal(CIp_t CI) 
// univ_ptr contains t_domain *, "modifying" defined in it.
{ t_type_specif_info * dom = (t_type_specif_info*)CI->univ_ptr;
  if (CI->cursym==S_CREATE) next_sym(CI);
  else if (CI->cursym==S_ALTER) next_sym(CI);
 // domain name: 
  next_and_test(CI, S_IDENT, IDENT_EXPECTED);
  strmaxcpy(dom->domname, CI->curr_name, sizeof(tobjname));
  if (next_sym(CI)=='.')
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    strcpy(dom->schema, dom->domname);
    strmaxcpy(dom->domname, CI->curr_name, sizeof(tobjname));
    next_sym(CI);
  }
  else dom->schema[0]=0;
  if (CI->cursym==S_AS) next_sym(CI);  // skip AS if specified
 // type:
  analyse_type(CI, dom->tp, dom->specif, NULL, 0);  // history and pointers not supported
  compile_domain_ext(CI, dom);
}
static BOOL compile_domain(cdp_t cdp, tptr defin, t_type_specif_info * dom)
{ compil_info xCI(cdp, defin, DUMMY_OUT, domain_anal);
  set_compiler_keywords(&xCI, NULL, 0, 1); /* SQL keywords selected */
  xCI.univ_ptr=dom;
  return !compile(&xCI);
}

// Designer entry points:
//////////////////////////////////////// domain design tool //////////////////////////////////////////////
#include "xrc/DomainDesignerDlg.cpp"

bool start_domain_designer(cdp_t cdp, tobjnum objnum, const char * folder_name)
{ 
  return open_dialog_designer(new DomainDesignerDlg(cdp, objnum, folder_name, cdp->sel_appl_name));
}


#if 0
bool start_domain_designer(cdp_t cdp, tobjnum objnum, const char * folder_name)
{
  DomainDesignerDlg * dlg = new DomainDesignerDlg(cdp, objnum, folder_name, cdp->sel_appl_name);
  if (!dlg) return false;
  if (dlg->modifying() && !dlg->lock_the_object())
    { cd_Signalize(cdp);  delete dlg; } 
  else if (!dlg->open(wxGetApp().frame, domdsng_coords.get_pos())) 
    delete dlg;
  else
  { dlg->SetIcon(wxIcon(_domain_xpm));  
    if (global_style!=ST_POPUP)
    { 
      dlg->ShowModal();
      delete dlg;
    }
    else
      dlg->Show();
    return true;  // designer opened OK
  }
 // error exit:
  return false;
}
#endif
//////////////////////////////////////// sequence design tool //////////////////////////////////////////////
#include "xrc/SequenceDesignerDlg.cpp"
#include "sequence.cpp"

bool start_sequence_designer(cdp_t cdp, tobjnum objnum, const char * folder_name)
{ 
  return open_dialog_designer(new SequenceDesignerDlg(cdp, objnum, folder_name, cdp->sel_appl_name));
}

#if 0
bool start_sequence_designer(cdp_t cdp, tobjnum objnum, const char * folder_name)
{
#ifdef STOP
  wxAcceleratorEntry entries[3];
  entries[0].Set(wxACCEL_CTRL,  (int) 'S', TD_CPM_SAVE);
  entries[1].Set(wxACCEL_CTRL|wxACCEL_SHIFT,  (int) 'S', TD_CPM_SAVE_AS);
  entries[2].Set(wxACCEL_CTRL,  (int) WXK_F4, TD_CPM_EXIT_FRAME);
  wxAcceleratorTable accel(3, entries);
  dsgn_frame->SetAcceleratorTable(accel);
#endif
  SequenceDesignerDlg * dlg = new SequenceDesignerDlg(cdp, objnum, folder_name, cdp->sel_appl_name);
  if (!dlg) return false;
  if (dlg->modifying() && !dlg->lock_the_object())
    { cd_Signalize(cdp);  delete dlg; }
  else if (!dlg->open(wxGetApp().frame, seqdsng_coords.get_pos()))
    delete dlg;
  else
  { dlg->SetIcon(wxIcon(_seq_xpm));
    if (global_style!=ST_POPUP)
    {
      dlg->ShowModal();
      delete dlg;
    }
    else
      dlg->Show();
    return true;  // designer opened OK
  }
 // error exit:
  return false;
}
#endif
////////////////////////////////////// table designer //////////////////////////////////////////////////
bool open_dialog_designer(any_designer * dsgn)
{
  if (!dsgn) return false;
 // check and lock the designed object:
  if (dsgn->modifying())
  { if (!dsgn->lock_the_object())
      { cd_Signalize(dsgn->cdp);  delete dsgn;  return false; }
    if (!check_not_encrypted_object(dsgn->cdp, dsgn->category, dsgn->objnum)) 
      { delete dsgn;  return false; }  // error signalised inside
  }
 // open the dialog:
  dsgn->cf_style = global_style;
  if (!dsgn->open(wxGetApp().frame, global_style))
  { if (dsgn->focus_receiver) dsgn->focus_receiver->Destroy();  else delete dsgn;  // if created, use Destroy(), delete otherwise
    return false;
  }
  if (global_style==ST_POPUP) 
    dsgn->add_tools();  // in MDI style the designer is a modal dialog  
  wxDialog * dlg = (wxDialog*)dsgn->focus_receiver;
  dsgn->dsgn_mng_wnd = dlg;
  dlg->SetIcon(wxIcon(dsgn->small_icon));  
  dsgn->set_designer_caption();  // must be after AddPage() for ST_AUINB or after defining dsgn_mng_wnd otherwise
  dsgn->write_status_message();
  if (global_style!=ST_POPUP)
  { dlg->ShowModal();
    delete dsgn;
  }
  else
  { dlg->Show();
    dlg->SetFocus();   // designer opened OK
  }
  return true;  // designer opened OK
}

bool open_standard_designer(any_designer * dsgn)
{
  if (!dsgn) return false;
 // check and lock the designed object:
  if (dsgn->modifying())
  { if (!dsgn->lock_the_object())
      { cd_Signalize(dsgn->cdp);  delete dsgn;  return false; }
    if (!check_not_encrypted_object(dsgn->cdp, dsgn->category, dsgn->objnum)) 
      { delete dsgn;  return false; }  // error signalised inside
  }
  wxWindow * frm = open_standard_frame(dsgn, global_style);
  if (!frm) return false;
  dsgn->write_status_message();
  dsgn->add_tools();
  return true;
}

wxWindow * open_standard_frame(competing_frame * dsgn, t_global_style my_style)
// Opens competing_frame.
// [dsgn] is supposed not to be Create(d) on entry. On error, [dsgn] is deleted.
{ wxWindow * ret;
  dsgn->cf_style = my_style;
 // create the "frame" for the designer:
  if (my_style==ST_AUINB)
  { wxPanel * frm = new wxPanel(aui_nb, -1);
    if (!frm) { delete dsgn;  return NULL; }
    if (!dsgn->open(frm, my_style)) 
    { if (dsgn->focus_receiver) dsgn->focus_receiver->Destroy();  else delete dsgn;  // if created, use Destroy(), delete otherwise
      frm->Destroy();
      return NULL; 
    }
    dsgn->put_designer_into_frame(frm, dsgn->focus_receiver);
    aui_nb->AddPage(frm, wxEmptyString, true, wxBitmap(dsgn->small_icon));
    if (aui_nb->has_right_button_visible())
      aui_nb->increase_offset();
    ret=frm;
  }
  else if (my_style==ST_MDI)
  { DesignerFrameMDI * frm = new DesignerFrameMDI(wxGetApp().frame, -1, wxEmptyString);
    if (!frm) { delete dsgn;  return NULL; }
    frm->dsgn = dsgn;
    if (!dsgn->open(frm, my_style)) 
    { dsgn->activate_other_competing_frame(); // with MDI style the tools of the previous competing frame are not restored because the frame will not receive the Activation signal. 
      // Must be before Destroy because dsgn is sent the deactivate event.
      if (dsgn->focus_receiver) dsgn->focus_receiver->Destroy();  else delete dsgn;  // if created, use Destroy(), delete otherwise
      frm->dsgn = NULL;  // prevents passing focus (in this designer must be before dsgn->Destroy(); because the windows may be created)
      frm->Destroy();
      return NULL; 
    }
    dsgn->dsgn_mng_wnd = frm;
    frm->SetIcon(wxIcon(dsgn->small_icon));  
    frm->Show();
    frm->Activate();  // sometimes neceaasry on Linux, for XML and query designers
    ret=frm;
  }
  else if (my_style==ST_POPUP)
  { DesignerFramePopup * frm = new DesignerFramePopup(wxGetApp().frame, -1, wxEmptyString, dsgn->persistent_coords->get_pos(), dsgn->persistent_coords->get_size());
    if (!frm) { delete dsgn;  return NULL; }
    frm->dsgn = dsgn;
    if (!dsgn->open(frm, my_style)) 
    { if (dsgn->focus_receiver) dsgn->focus_receiver->Destroy();  else delete dsgn;  // if created, use Destroy(), delete otherwise
      // Must be before Destroy because dsgn is sent the deactivate event.
      frm->dsgn = NULL;  // prevents passing focus (in this designer must be before dsgn->Destroy(); because the windows may be created)
      frm->Destroy();
      return NULL; 
    }
    dsgn->put_designer_into_frame(frm, dsgn->focus_receiver);
    dsgn->dsgn_mng_wnd = frm;
    frm->SetIcon(wxIcon(dsgn->small_icon));  
    frm->Show();
    ret=frm;
  }
  dsgn->set_designer_caption();  // must be after AddPage() for ST_AUINB or after defining dsgn_mng_wnd otherwise
  dsgn->focus_receiver->SetFocus();   // designer opened OK
  return ret;
}

bool open_table_designer(cdp_t cdp, tobjnum objnum, const char * folder_name, const char * schema_name)
// [schema_name] is used when the schema is not the current one (happens when called from the diagram)
{ 
  return open_standard_designer(new table_designer(cdp, objnum, folder_name, *schema_name ? schema_name : cdp->sel_appl_name));
}

void table_delete_bitmaps(void)
{ delete_bitmaps(table_designer::td_tool_info); }

wxPoint get_best_designer_pos(void)
{ wxPoint frpos = wxGetApp().frame->GetPosition();
  return wxPoint(frpos.x+36, frpos.y+90);
}

//////////////////////////////////////////////////////////////////////////////
BOOL index_dynar::delet(unsigned index)
{ if (index >= (unsigned)count()) return FALSE;
  acc(index)->ind_descr::~ind_descr();
  return t_dynar::delet(index);
}

index_dynar::~index_dynar(void)    /* t_dynar destructor called then */
{ for (int i=0;  i<count();  i++) acc(i)->ind_descr::~ind_descr(); }

BOOL check_dynar::delet(unsigned index)
{ if (index >= (unsigned)count()) return FALSE;
  acc(index)->check_constr::~check_constr();
  return t_dynar::delet(index);
}

check_dynar::~check_dynar(void)    /* t_dynar destructor called then */
{ for (int i=0;  i<count();  i++) acc(i)->check_constr::~check_constr(); }

BOOL refer_dynar::delet(unsigned index)
{ if (index >= (unsigned)count()) return FALSE;
  acc(index)->forkey_constr::~forkey_constr();
  return t_dynar::delet(index);
}

refer_dynar::~refer_dynar(void)    /* t_dynar destructor called then */
{ for (int i=0;  i<count();  i++) acc(i)->forkey_constr::~forkey_constr(); }

BOOL attr_dynar::delet(unsigned index)
{ if (index >= (unsigned)count()) return FALSE;
  acc(index)->atr_all::~atr_all();
  return t_dynar::delet(index);
}

attr_dynar::~attr_dynar(void)    /* t_dynar destructor called then */
{ for (int i=0;  i<count();  i++) acc(i)->atr_all::~atr_all(); }

ind_descr::~ind_descr(void)   /* destructor for reftables called automatically */
{ corefree(text);  text=NULL; }

check_constr::~check_constr(void)
{ corefree(text);  text=NULL; }

