/////////////////////////////////////////////////////////////////////////////
// Name:        XMLParamsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/31/04 11:06:44
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "XMLParamsDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "XMLParamsDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLParamsDlg type definition
 */

IMPLEMENT_CLASS( XMLParamsDlg, wxDialog )

/*!
 * XMLParamsDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLParamsDlg, wxDialog )

////@begin XMLParamsDlg event table entries
    EVT_BUTTON( wxID_OK, XMLParamsDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLParamsDlg::OnCancelClick )

////@end XMLParamsDlg event table entries
    EVT_CLOSE(XMLParamsDlg::OnClose) 

END_EVENT_TABLE()
//////////////////////////////////////////// grid table ///////////////////////////////////////////
#define PARAM_NAME   0
#define PARAM_TYPE   1
#define PARAM_LENGTH 2
#define PARAM_VALUE  3
#define PARAM_VAL_RT 1

#define TYPE_HAS_INT_LEN(tp) (wb_type_list[tp].wbtype==ATT_INT32  || wb_type_list[tp].wbtype==ATT_MONEY)
#define TYPE_HAS_STR_LEN(tp) (wb_type_list[tp].wbtype==ATT_STRING || wb_type_list[tp].wbtype==ATT_BINARY)

class ParamsGridTable : public wxGridTableBase
{ 
  wxGrid * my_grid;
    t_dad_param_dynar *params;
    wxCSConv * sys_conv;
    bool      *changed;
    bool designer_mode;
    virtual int GetNumberRows();
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void SetValue( int row, int col, const wxString& value );
    wxString GetColLabelValue( int col );

    bool AppendRows(size_t numRows = 1);
    bool InsertRows(size_t pos = 0, size_t numRows = 1);  // used only for registering columns in an existing table
    bool CanGetValueAs( int row, int col, const wxString& typeName );
    long GetValueAsLong( int row, int col);
    void SetValueAsLong( int row, int col, long value );
 public:
  ParamsGridTable(t_dad_param_dynar * paramsIn, wxCSConv * convIn, bool * changedIn, wxGrid * my_gridIn) :
    params(paramsIn), sys_conv(convIn), changed(changedIn), my_grid(my_gridIn)
      { designer_mode = changed!=NULL; }
  void set_length_editor(int row, int internal_type);
};

int ParamsGridTable::GetNumberRows()
{ return params->count();
}

int ParamsGridTable::GetNumberCols()
{ return designer_mode ? 4 : 2; }

wxString ParamsGridTable::GetColLabelValue( int col )
{ switch (col)
  { case PARAM_NAME:   return _("Name");
    case PARAM_TYPE:   return designer_mode ? _("Type") : _("Value");
    case PARAM_LENGTH: return _("Length");
    case PARAM_VALUE:  return _("Value in the designer");
	default: return wxEmptyString;
  }	
}

bool ParamsGridTable::IsEmptyCell( int row, int col )
{ return row+1==params->count(); }

wxString ParamsGridTable::GetValue( int row, int col )
{ t_dad_param * cur_par = params->acc(row);  // not supposed to allocate
  switch (col)
  { case PARAM_NAME: 
      return wxString(cur_par->name, *sys_conv);
    case PARAM_LENGTH:  // length
      return wxEmptyString;  // used by types without length
    case PARAM_VAL_RT:
    case PARAM_VALUE:  // default value
      return wxString(cur_par->val);
  }
  return wxEmptyString;
}

bool ParamsGridTable::AppendRows(size_t numRows)
// only numRows==1 implemented
{ t_dad_param * cur_par = params->next();
  if (!cur_par) return false;
  cur_par->set_default();
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}

bool ParamsGridTable::InsertRows(size_t pos, size_t numRows)
// used only for registering columns in an existing table
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

bool ParamsGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{// must check for the fictive record: otherwise trying to convert a non-existing type to its name 
  if (typeName == wxGRID_VALUE_STRING) return true;
  if (typeName == wxGRID_VALUE_NUMBER)
    if (designer_mode)
      if (col==PARAM_TYPE || col==PARAM_LENGTH) return true;
  return false;
}

long ParamsGridTable::GetValueAsLong( int row, int col)
{ t_dad_param * cur_par = params->acc(row);  // not supposed to allocate
  switch (col)
  { case PARAM_TYPE:        
     // type in the fictive row: 
      if (row+1==params->count())
      { long i = 0;
        while (wb_type_list[i].long_name) i++;
        return i;  // index of an empty string
      }
     // type in the normal row:
      return cur_par->type;  // must not return -1
    case PARAM_LENGTH:  // length
      if (TYPE_HAS_STR_LEN(cur_par->type))
        return cur_par->specif.length;
      else if (TYPE_HAS_INT_LEN(cur_par->type))
        return cur_par->specif.precision;
      return 0;  // cell should be disabled 
  }
  return -1;
}

void ParamsGridTable::set_length_editor(int row, int internal_type)
{ 
  if (!designer_mode) return;
  if (TYPE_HAS_INT_LEN(internal_type))
  { my_grid->SetCellRenderer(row, PARAM_LENGTH, new wxGridCellEnumRenderer(wxT("1 byte,2 bytes,4 bytes,8 bytes")));
    my_grid->SetCellEditor  (row, PARAM_LENGTH, new wxGridCellMyEnumEditor(wxT("1 byte,2 bytes,4 bytes,8 bytes")));
    my_grid->SetReadOnly    (row, PARAM_LENGTH, false);  // length enabled
  }
  else if (TYPE_HAS_STR_LEN(internal_type))
  { my_grid->SetCellRenderer(row, PARAM_LENGTH, new wxGridCellNumberRenderer);
    my_grid->SetCellEditor  (row, PARAM_LENGTH, new wxGridCellNumberEditor(1,MAX_FIXED_STRING_LENGTH));
    my_grid->SetReadOnly    (row, PARAM_LENGTH, false);  // length enabled
  }
  else
  { my_grid->SetCellRenderer(row, PARAM_LENGTH, new wxGridCellStringRenderer);  // will be empty
    my_grid->SetReadOnly    (row, PARAM_LENGTH);  // length disabled
  }
}

void ParamsGridTable::SetValueAsLong( int row, int col, long value )
{ t_dad_param * cur_par = params->acc(row);  // not supposed to allocate
  switch (col)
  { case PARAM_TYPE: 
      cur_par->type = value;
      set_length_editor(row, cur_par->type);
      break;
    case PARAM_LENGTH:
      if (TYPE_HAS_STR_LEN(cur_par->type))
        cur_par->specif.length=(uns16)value;
      else if (TYPE_HAS_INT_LEN(cur_par->type))
        cur_par->specif.precision=(uns8)value;
      else return;
      break;
    default:
      return;
  }
  if (changed) *changed=true;
}

void ParamsGridTable::SetValue( int row, int col, const wxString& value )
{ t_dad_param * cur_par = params->acc(row);  // not supposed to allocate
  switch (col)
  { case PARAM_NAME:
    { wxString v = value;  v.Trim(false);  v.Trim(true);
      strmaxcpy(cur_par->name, v.mb_str(*sys_conv), sizeof(cur_par->name));
     // add a new fictive row:
      if (row == params->count()-1 && *cur_par->name)
      { my_grid->AppendRows(1);
        my_grid->SetReadOnly(row+1, PARAM_LENGTH, true);  // length disabled
      }
      break;
    }
    case PARAM_VALUE:  // default value
    case PARAM_VAL_RT:
    { wxString v = value;  v.Trim(false);  v.Trim(true);
      wcscpy(cur_par->val, v.Mid(0, MAX_PARAM_VAL_LEN).c_str());
      break;
    }
  }
  if (changed) *changed=true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////

void XMLParamsDlg::convert_types_for_edit(void)
// convert parameter types for editing in the grid
{ 
  for (int i=0;  i<params->count();  i++)  // the last parameter is always fictive
  { t_dad_param * cur_par = params->acc0(i);
    if (cur_par->name[0])
      cur_par->type = convert_type_for_edit(wb_type_list, cur_par->type, cur_par->specif);
  }
}

/*!
 * XMLParamsDlg constructors
 */


XMLParamsDlg::XMLParamsDlg(xml_designer * xmldsgnIn)
{ 
  params   = &xmldsgnIn->dad_tree->params;
  sys_conv = xmldsgnIn->cdp->sys_conv;
  changed  = &xmldsgnIn->changed;
  convert_types_for_edit();
}

void XMLParamsDlg::closing(void)  // for OK and Cancel and close button
// convert types from editing in the grid:
{ int i;
  for (i=0;  i<params->count();  i++)  // the last parameter is always fictive
  { t_dad_param * cur_par = params->acc0(i);
    if (cur_par->name[0])
      cur_par->type = convert_type_for_sql_2(wb_type_list, cur_par->type, cur_par->specif);
  }
 // remove cleared parameters:
  for (i=0;  i<params->count();  )  // the last parameter is always fictive
    if (!*params->acc0(i)->name)
      params->delet(i);
    else i++;
}

void XMLParamsDlg::OnClose(wxCloseEvent & event)  // for OK and Cancel
{
  closing();
  Destroy();
}

XMLParamsDlg::~XMLParamsDlg(void)
{ }

XMLParamsDlg::XMLParamsDlg(CQueryDesigner * querydsgnIn)
{ 
  params   = &querydsgnIn->m_Params;
  sys_conv = querydsgnIn->cdp->sys_conv;
  changed  = &querydsgnIn->changed;
  convert_types_for_edit();
}

XMLParamsDlg::XMLParamsDlg(t_dad_param_dynar * paramsIn, wxCSConv * convIn, bool * changedIn)
{ params = paramsIn; 
  sys_conv = convIn;
  changed  = changedIn;
  convert_types_for_edit();  // types are not used, but the destructor will convert it back
}

/*!
 * XMLParamsDlg creator
 */

bool XMLParamsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLParamsDlg member initialisation
    mSizer = NULL;
    mGrid = NULL;
    mButtons = NULL;
    mOK = NULL;
    mCancel = NULL;
////@end XMLParamsDlg member initialisation

////@begin XMLParamsDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLParamsDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLParamsDlg
 */

void XMLParamsDlg::CreateControls()
{    
////@begin XMLParamsDlg content construction
    XMLParamsDlg* itemDialog1 = this;

    mSizer = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mSizer);

    mGrid = new wxGrid( itemDialog1, CD_XML_PARAM_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mSizer->Add(mGrid, 1, wxGROW|wxALL, 5);

    mButtons = new wxBoxSizer(wxHORIZONTAL);
    mSizer->Add(mButtons, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mOK = new wxButton;
    mOK->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    mOK->SetDefault();
    mButtons->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mCancel = new wxButton;
    mCancel->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(mCancel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLParamsDlg content construction
   // mGrid = new wxGrid( itemDialog1, CD_XML_PARAM_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
   // remove calling all grid methods
  SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // otherwise Enter (and ESC--no more) in the grid cell editors close the dialog
  SetEscapeId(wxID_NONE);  // prevents the ESC in the grid from closing the dialog

 // create fictive parameter, if it does not exist:
  bool designer_mode = changed!=NULL;
  if (designer_mode)
  { int cnt = params->count();
    if (cnt==0 || params->acc0(cnt-1)->name[0]!=0)
      params->next();
    mButtons->Show(mCancel, false);
    mOK->SetLabel(_("Close"));
  }
  else
    SetTitle(_("Parameters"));
 // grid
  mGrid->BeginBatch();
  ParamsGridTable * params_grid_table = new ParamsGridTable(params, sys_conv, changed, mGrid);
  mGrid->SetTable(params_grid_table, TRUE);
  mGrid->SetColLabelSize(default_font_height);
  mGrid->SetDefaultRowSize(default_font_height);
  mGrid->SetRowLabelSize(22);  // for 2 digits
  mGrid->SetLabelFont(system_gui_font);
  DefineColSize(mGrid, PARAM_NAME, 76);
  if (designer_mode)
  { DefineColSize(mGrid, PARAM_TYPE,   190);
    DefineColSize(mGrid, PARAM_LENGTH, LENGTH_SPIN_COL_WIDTH);
    DefineColSize(mGrid, PARAM_VALUE,  160);
  }
  else
    DefineColSize(mGrid, PARAM_VAL_RT, 160);
  mGrid->EnableDragRowSize(false);
 // limit the length of the constrain name:
  wxGridCellAttr * name_attr = new wxGridCellAttr;
  name_attr->SetEditor(make_limited_string_editor(OBJ_NAME_LEN));
  mGrid->SetColAttr(PARAM_NAME, name_attr);
 // prepare type choice:
  if (designer_mode)
  { wxGridCellAttr * type_col_attr = new wxGridCellAttr;
    wxString type_choice;  int i;
    i=0;
    do
    { type_choice=type_choice+wxGetTranslation(wb_type_list[i].long_name);
      i++;
      if (!wb_type_list[i].long_name) break;
      type_choice=type_choice+wxT(",");
    } while (true);  
    wxGridCellMyEnumEditor * type_editor = new wxGridCellMyEnumEditor(type_choice);
    type_col_attr->SetEditor(type_editor);
    i=0;  type_choice=wxEmptyString;
    do
    { type_choice=type_choice+wxGetTranslation(wb_type_list[i].short_name);
      i++;
      if (!wb_type_list[i].short_name) break;
      type_choice=type_choice+wxT(",");
    } while (true);  
    type_choice=type_choice+wxT(", ");  // for fictive row
    wxGridCellEnumRenderer * type_renderer = new wxGridCellEnumRenderer(type_choice);
    type_col_attr->SetRenderer(type_renderer);  // takes the ownership
    mGrid->SetColAttr(PARAM_TYPE, type_col_attr);
   // prepare length editors:
    for (i=0;  i<params->count();  i++)
      params_grid_table->set_length_editor(i, params->acc0(i)->type);
  }
  else  // make names read-only
  { wxGridCellAttr * col_attr = new wxGridCellAttr;
    col_attr->SetReadOnly();
    mGrid->SetColAttr(PARAM_NAME, col_attr);
  }
  mSizer->SetItemMinSize(mGrid, get_grid_width(mGrid), get_grid_height(mGrid, 4));
  mGrid->EndBatch();
}

/*!
 * Should we show tooltips?
 */

bool XMLParamsDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap XMLParamsDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLParamsDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLParamsDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLParamsDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLParamsDlg icon retrieval
    return wxNullIcon;
////@end XMLParamsDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLParamsDlg::OnOkClick( wxCommandEvent& event )
{ 
  if (mGrid->IsCellEditControlEnabled())  // commit cell edit
    mGrid->DisableCellEditControl();
  closing();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLParamsDlg::OnCancelClick( wxCommandEvent& event )
{
  closing();
  Destroy();  // otherwise the Cancel button does not work
  event.Skip();
}


