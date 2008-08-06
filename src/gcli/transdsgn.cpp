// transdsgn.cpp
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
#include "xmldsgn.h"
#include "dsparser.h"
#include "transdsgn.h"
#include <wx/dataobj.h>
#include <wx/dnd.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

void report_mapping_result(t_ie_run & x, wxWindow * parent)
{ wxString msg;
  if (x.source_type==IETYPE_TDT || x.dest_type==IETYPE_TDT) return;
  if (x.ignored_input_columns>0)
    msg.Printf(_("%d source columns mapped to target columns with the same names,\n%d source columns remained not mapped."), x.mapped_input_columns, x.ignored_input_columns);
  else 
  { if (!x.mapped_input_columns)  // not reporting if no columns found
      return;
    msg=_("All source columns mapped to target columns.");
  }  
  info_box(msg, parent);
}

t_type_list wb2_type_list[] = {
{ wxEmptyString, _("---------- Numbers: ----------"),       0,            0,      0 },
{ _("INTEGER; NUMERIC 8 bit"),  _("INTEGER; NUMERIC 8 bit"),  ATT_INT8,   2<<8,   0 },
{ _("INTEGER; NUMERIC 16 bit"), _("INTEGER; NUMERIC 16 bit"), ATT_INT16,  2<<8,   0 },
{ _("INTEGER; NUMERIC 32 bit"), _("INTEGER; NUMERIC 32 bit"), ATT_INT32,  2<<8,   0 },
{ _("INTEGER; NUMERIC 64 bit"), _("INTEGER; NUMERIC 64 bit"), ATT_INT64,  2<<8,   0 },
{ _("REAL"), _("REAL (double precision)"),                  ATT_FLOAT,    0,      0 },
{ wxEmptyString, _("---------- Characters: ----------"),    0,            0,      0 },
{ _("CHAR(n)"), _("CHAR(n) string with specified size"),    ATT_STRING,   12,     0 },
{ _("CLOB"), _("CLOB (variable size text)"),                ATT_TEXT,     0,      0 },
{ _("CHAR"), _("CHAR (single character)"),                  ATT_CHAR,     1,      0 },
{ wxEmptyString, _("---------- Date and Time: ----------"), 0,            0,      0 },
{ _("DATE"), _("DATE (day&month&year)"),                    ATT_DATE,     0,      0 },
{ _("TIME"), _("TIME (time within a day)"),                 ATT_TIME,     0,      0 },
{ _("TIMESTAMP"), _("TIMESTAMP (date and time)"),           ATT_TIMESTAMP,0,      0 },
{ wxEmptyString, _("---------- Binary: ----------"),        0,            0,      0 },
{ _("BINARY(n)"), _("BINARY(n) with fixed size"), ATT_BINARY,   12,     0 },
{ _("BLOB"), _("BLOB (variable size binary object)"),       ATT_NOSPEC,   0,      0 },
{ wxEmptyString, _("---------- Other types: ----------"),   0,            0,      0 },
{ _("BOOLEAN"), _("BOOLEAN (True or False)"),               ATT_BOOLEAN,  0,      0 },
{ NULL, NULL, 0 }
};


#include "xrc/DataTransDsgnDlg.h"

class trans_designer : public DataTransDsgnDlg
{ char * initial_design;
 public:
  trans_designer(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn, char * initial_designIn) :
    DataTransDsgnDlg(cdpIn, objnumIn, folder_nameIn, schema_nameIn), initial_design(initial_designIn)
     { }
  ~trans_designer(void)
    { remove_tools();  corefree(initial_design); }
  bool open(wxWindow * parent, t_global_style my_style);
  bool check_design(void);
  //void show_source_target_info(void);
 ////////// typical designer methods and members: ///////////////////////
  enum { TOOL_COUNT = 9 };
  void set_designer_caption(void);
  static t_tool_info tool_info[TOOL_COUNT+1];
 // virtual methods (commented in class any_designer):
  char * make_source(bool alter);
  bool IsChanged(void) const;  
  wxString SaveQuestion(void) const;  
  bool save_design(bool save_as);
  void OnCommand(wxCommandEvent & event);
  t_tool_info * get_tool_info(void) const
    { return tool_info; }
  void _set_focus(void) // checks on Windows if the designer has already been created - otherwise problems with passing the focus after closing the error message, e.g. data source not connected in ODBC transport designer
    { if (GetHandle()) SetFocus(); }
  wxMenu * get_designer_menu(void);
  void update_designer_menu(void);
 // member variables:
  tobjname object_name;
  void OnKeyDown(wxKeyEvent & event);
  DECLARE_EVENT_TABLE()
};

#include "xrc/DataTransDsgnDlg.cpp"

/////////////////////////// virtual methods (commented in class any_designer): //////////////////////////////////

char * trans_designer::make_source(bool alter)
{ return x.conv_ie_to_source(); }

void trans_designer::set_designer_caption(void)
{ wxString caption;
  if (!modifying()) caption = _("Design a New Data Transport");
  else caption.Printf(_("Edit Design of Data Transport %s"), wxString(object_name, *cdp->sys_conv).c_str());
  set_caption(caption);
}

bool trans_designer::IsChanged(void) const
{ return changed; }

wxString trans_designer::SaveQuestion(void) const
{ return modifying() ?
    _("Your changes have not been saved. Do you want to save your changes?") :
    _("The data transport object has not been created yet. Do you want to save the data transport?");
}

bool trans_designer::save_design(bool save_as)
// Saves the domain, returns true on success.
{ if (!save_design_generic(save_as, object_name, CO_FLAG_TRANSPORT))  // using the generic save from any_designer
    return false;  
  changed=false;
  set_designer_caption();
  return true;
}

void trans_designer::OnCommand(wxCommandEvent & event)
{ 
  switch (event.GetId())
  { case TD_CPM_SAVE:
      if (mGrid) mGrid->DisableCellEditControl();
      if (!IsChanged()) break;  // no message if no changes
      if (!save_design(false)) break;  // error saving, break
      info_box(_("Your design has been saved."), dialog_parent());
      break;
    case TD_CPM_SAVE_AS:
      if (mGrid) mGrid->DisableCellEditControl();
      if (!save_design(true)) break;  // error saving, break
      info_box(_("Your design was saved under a new name."), dialog_parent());
      break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
    case TD_CPM_EXIT_FRAME:
      if (mGrid) mGrid->DisableCellEditControl();
      if (exit_request(dialog_parent(), true))
        destroy_designer(GetParent(), &transdsng_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
      exit_request(this, false);
      destroy_designer(GetParent(), &transdsng_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_CHECK:
    { //if (!store_selection(TRUE)) return FALSE;
      if (mGrid) mGrid->DisableCellEditControl();
      if (!check_design()) break;
      info_box(_("The design is correct"), dialog_parent());
      break;
    }
    case TD_CPM_RUN:
    { //if (!store_selection(TRUE)) return FALSE;
      if (mGrid) mGrid->DisableCellEditControl();
      if (!check_design()) break;
      if (x.creates_target_file())
        if (!can_overwrite_file(x.outpath, NULL))
          break;  // target file exists and overwrite not confirmed
      BOOL res;
      { wxBusyCursor wait;
        res=x.do_transport();
        x.run_clear();
      }
      if (res) 
      { info_box(_("Data transport completed."), dialog_parent());
        if (x.creates_target_table())
          Add_new_component(cdp, CATEG_TABLE, cdp->sel_appl_name, x.outpath, x.out_obj, 0, "", false);
      }
      else cd_Signalize2(cdp, dialog_parent());  // if table has been created, it will not be added to the control panel
      break;
    }
    case TD_CPM_TEXT_PROP:
      if (back_and_restart(3))
        changed=true;  // may have been changed
      break;
    case TD_CPM_CRE_IMPL:
      OnCdTrCreImplClick();
      break;
    case TD_CPM_HELP:
      wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/tran_descr.html"));  
      break;
  }
}

void trans_designer::OnKeyDown( wxKeyEvent& event )
{ 
  OnCommonKeyDown(event);  // event.Skip() is inside
}

BEGIN_EVENT_TABLE(trans_designer, DataTransDsgnDlg)
    EVT_KEY_DOWN(trans_designer::OnKeyDown)
END_EVENT_TABLE()

t_tool_info trans_designer::tool_info[TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,       File_save_xpm,        wxTRANSLATE("Save design")),
  t_tool_info(TD_CPM_EXIT,       exit_xpm,             wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CHECK,      validateSQL_xpm,      wxTRANSLATE("Validate")),
  t_tool_info(TD_CPM_TEXT_PROP,  tableProperties_xpm,  wxTRANSLATE("Text options")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_RUN,        run2_xpm,             wxTRANSLATE("Run")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_HELP,       help_xpm,             wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

void trans_delete_bitmaps(void)
{ delete_bitmaps(trans_designer::tool_info); }

////////////////////////////////////////// global actions /////////////////////////////////////////
bool trans_designer::check_design(void)
// Analyses all parametrs including details, reports errors
{ int i;  tdetail * det;  
  const wxChar * err = NULL; // error message pattern
  bool nonempty=false;

 // special cases:
  if (x.source_type==IETYPE_TDT)
    if (x.dest_type==IETYPE_WB) 
      if (x.dest_exists)
        return true;
      else { error_box(_("The target table does not exist."));  return false; }
    else { error_box(_("The native data format can only be used with data from the database."));  return false; }
  if (x.dest_type==IETYPE_TDT)
    if (x.source_type==IETYPE_WB || x.source_type==IETYPE_CURSOR) return true;
    else { error_box(_("The native data format can only be used with data from the database."));  return false; }

 // check moving to itself:
  if (x.dest_type==x.source_type)
    if (!my_stricmp(x.inpath, x.outpath))
      if (x.dest_type!=IETYPE_WB)
        if (!x.odbc_target() || !my_stricmp(x.inschema, x.outschema) && !my_stricmp(x.inserver, x.outserver))
          { error_box(_("Cannot copy data to itself."));  return false; }

 // prepare colmaps:
  if (!x.allocate_colmaps()) return false;

 // open the source DBF file in order to check the source column names:
  if (x.source_type==IETYPE_DBASE || x.source_type==IETYPE_FOX)
  { FHANDLE inhnd=CreateFileA(x.inpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (inhnd==INVALID_FHANDLE_VALUE)
      { error_box(_("Cannot open the input file."), this);  goto err; }
    t_dbf_io dbf_inp(inhnd, x.source_type==IETYPE_DBASE);
    if (!dbf_inp.rdinit(cdp, x.inpath, NULL))
      { CloseHandle(inhnd);  error_box(_("Cannot analyse the input file."), this);  goto err; }
    CloseHandle(inhnd);
     // convert column names to numbers:
    t_specif s_specif = code2specif(x.src_recode);
    i=x.convert_column_names_to_numbers(cdp, &dbf_inp, s_specif);
    if (i) { i--;  err=_("Source column not found on line %u.");  goto reperr; }
  }

 // find missing data:
  int target_record_size;
  target_record_size = x.dbf_target() ? 1 : 0;
  for (i=0;  i<x.dd.count();  i++)
  { det=x.dd.acc0(i);
    if (!*det->source && !*det->destin) continue;  // empty is ignored
    nonempty=true;
   // check source:
    if (x.source_type==IETYPE_CSV || x.source_type==IETYPE_COLS)
    { sig32 val;
      if (!str2int(det->source, &val) || val<=0 || val>=MAX_TABLE_COLUMNS)
        { err=_("Bad source column number on line %u.");  goto reperr; }
      if (x.src_colmap[val-1]!=NOATTRIB)
        { err=_("Duplicate column number on line %u.");  goto reperr; }
      x.src_colmap[val-1]=i;
    }
    else if (x.source_type==IETYPE_WB || x.source_type==IETYPE_CURSOR)
    { if (!*cutspaces(det->source))
        { err=_("Source column specification missing on line %u.");  goto reperr; }
    }
    else  // DBF  -- insufficied, should check the column name
    { if (!*cutspaces(det->source))
        { err=_("Source column specification missing on line %u.");  goto reperr; }
    }
   // check destination:
    if (x.creating_target)
    { if (x.dest_type==IETYPE_CSV || x.dest_type==IETYPE_COLS) // dest column number
      { sig32 val;
        cutspaces(det->destin); 
        if (*det->destin)// may be empty
        { if (!str2int(det->destin, &val) || val<=0 || val>=MAX_TABLE_COLUMNS)
            { err=_("Bad target column number on line %u.");  goto reperr; }
          if (x.colmap[val-1]!=NOATTRIB)
            { err=_("Duplicate column number on line %u.");  goto reperr; }
          x.colmap[val-1]=i;
        }
       // dest size:
        if (x.dest_type==IETYPE_COLS)
          if (!det->destlen1 || det->destlen1>10000)
            { err=_("Column width specification error on line %u.");  goto reperr; }
      }
      else // destination attribute name, type
      { cutspaces(det->destin);
        if (!*det->destin)
          { err=_("Data target missing on line %u.");  goto reperr; }
        if (x.dest_type==IETYPE_WB)  // allow empty destin
        { //if (*det->destin && !det->desttype)
          //  { err=IER_DET_NODESTTYPE;  goto reperr; }
        }
        else
        {// check type:
          //if (!det->desttype)
          //  { err=IER_DET_NODESTTYPE;  goto reperr; }
         // dest size:
          if (x.dest_type==IETYPE_DBASE || x.dest_type==IETYPE_FOX)
            if (det->desttype!=DBFTYPE_MEMO)
            { target_record_size += det->destlen1;
              if (!det->destlen1 || det->destlen1>255)   // single byte "size"
                { err=_("Data length specification error on line %u.");  goto reperr; }
            }
        }
      }
    }
    else  // check if the column name exists in the target
      if (x.dest_type==IETYPE_WB && x.out_obj!=NOOBJECT)
      { char uname[ATTRNAMELEN+1];  strmaxcpy(uname, det->destin, sizeof(uname));
        Upcase9(cdp, uname);
        const d_table * td = cd_get_table_d(cdp, x.out_obj, CATEG_TABLE);
        if (td!=NULL)
        { int i;  const d_attr * pattr;  bool found=false;
          for (i=0, pattr=FIRST_ATTR(td);  i<td->attrcnt;  i++, pattr=NEXT_ATTR(td,pattr))
          if (!strcmp(pattr->name, uname))
            { found=true;  break; }
          release_table_d(td);
          if (!found)
            { err=_("Bad target column name on line %u.");  goto reperr; }
        }
      }
  }
  if (x.dbf_target())
    if (target_record_size>0xffff)
      { error_box(_("DBF record cannot be larger than 64KB."), this);  goto err; }
  if (!nonempty) 
    { error_box(_("Transport design is empty."), this);  goto err; }
  return true;

 reperr:
  { wxString msg;
    msg.Printf(err, i+1);
    error_box(msg);
  }
 err:
  x.release_colmaps();
  return false;
}


//////////////////////////////////////////// grid table ///////////////////////////////////////////
#define TR_SOURCE_COLUMN 0
#define TR_TARGET_COLUMN 1
#define TR_TARGET_TYPE   2
#define TR_TARGET_LENGTH 3

class TransportGridTable : public wxGridTableBase
{public: 
  DataTransDsgnDlg * tr_dsgn;
 private:
  wxGrid * my_grid;
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
    bool DeleteRows(size_t pos, size_t numRows);
  TransportGridTable(DataTransDsgnDlg * tr_dsgnIn, wxGrid * my_gridIn)
    { tr_dsgn=tr_dsgnIn;  my_grid=my_gridIn; }

};

int TransportGridTable::GetNumberRows()
{ return tr_dsgn->x.dd.count(); }

int TransportGridTable::GetNumberCols()
{ return tr_dsgn->x.creating_target ? 4 : 2; }

wxString TransportGridTable::GetColLabelValue( int col )
{ switch (col)
  { case TR_SOURCE_COLUMN:
      return tr_dsgn->x.source_type==IETYPE_CSV || tr_dsgn->x.source_type==IETYPE_COLS ?
        _("Source column #") : _("Source value");
    case TR_TARGET_COLUMN:
      return tr_dsgn->x.dest_type==IETYPE_CSV || tr_dsgn->x.dest_type==IETYPE_COLS ?
        _("Target column #") : _("Target column");
    case TR_TARGET_TYPE:
      return _("Target column type");
    case TR_TARGET_LENGTH:
      return _("Target column length/scale");
  }
  return wxEmptyString;
}

bool TransportGridTable::IsEmptyCell( int row, int col )
{ return row+1==tr_dsgn->x.dd.count(); }

wxString TransportGridTable::GetValue( int row, int col )
{ tdetail * cur_col = tr_dsgn->x.dd.acc(row);  // not supposed to allocate
  switch (col)
  { case TR_SOURCE_COLUMN: 
      return wxString(cur_col->source, *tr_dsgn->cdp->sys_conv);
    case TR_TARGET_COLUMN: 
      return wxString(cur_col->destin, *tr_dsgn->cdp->sys_conv);
    case TR_TARGET_LENGTH:  // length
    { char buf[25];  
      if (tr_dsgn->x.dbf_target())
      { if (cur_col->desttype==DBFTYPE_MEMO) return wxEmptyString;  // length enabled except for MEMO
      }
      else if (tr_dsgn->x.db_target())
      { if (!SPECIFLEN(cur_col->desttype) && cur_col->desttype!=ATT_INT8 && cur_col->desttype!=ATT_INT16 && cur_col->desttype!=ATT_INT32 && cur_col->desttype!=ATT_INT64) 
          return wxEmptyString;  // length disabled
      }
      if (cur_col->destlen2)
        sprintf(buf, "%u,%u", cur_col->destlen1, cur_col->destlen2);
      else
        sprintf(buf, "%u", cur_col->destlen1);
      return wxString(buf, *tr_dsgn->cdp->sys_conv);
    }
  }
  return wxEmptyString;
}

bool TransportGridTable::AppendRows(size_t numRows)
// only numRows==1 implemented
{ tdetail * cur_col = tr_dsgn->x.dd.next();
  if (!cur_col) return false;
  //cur_par->set_default();
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}

bool TransportGridTable::InsertRows(size_t pos, size_t numRows)
// used only for registering columns in an existing table
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

bool TransportGridTable::DeleteRows(size_t pos, size_t numRows)
{ for (int i=0;  i<numRows;  i++)
    tr_dsgn->x.dd.delet((int)pos);
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

bool TransportGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{// must check for the fictive record: otherwise trying to convert a non-existing type to its name 
  if (typeName == wxGRID_VALUE_STRING) return true;
  if (typeName == wxGRID_VALUE_NUMBER)
    if (col==TR_TARGET_TYPE) return true;
  return false;
}

long TransportGridTable::GetValueAsLong( int row, int col)
{ tdetail * cur_col = tr_dsgn->x.dd.acc(row);  // not supposed to allocate
  switch (col)
  { case TR_TARGET_TYPE:
      if (tr_dsgn->x.text_target())
        return 0;  // the only choice
      else if (tr_dsgn->x.dbf_target())
      { if (row+1==tr_dsgn->x.dd.count()) return 6;  // after last, empty rendering
        return cur_col->desttype ? cur_col->desttype-1 : 0;  // must not return -1 
      }
      else  // db
      { long i = 0;
        if (row+1==tr_dsgn->x.dd.count())
        { while (wb2_type_list[i].long_name) i++;
          return i;
        }
        while (wb2_type_list[i].long_name)
          if (cur_col->desttype == wb2_type_list[i].wbtype) return i;
          else i++;
        return i;  // must not return -1
      }
  }
  return -1;
}

void TransportGridTable::SetValueAsLong( int row, int col, long value )
{ tdetail * cur_col = tr_dsgn->x.dd.acc(row);  // not supposed to allocate
  switch (col)
  { case TR_TARGET_TYPE:
      if (tr_dsgn->x.text_target())
        my_grid->SetReadOnly(row, TR_TARGET_LENGTH, false);  // length enabled
        // type not stored
      else if (tr_dsgn->x.dbf_target())
      { cur_col->desttype = value+1;  // ignored for CSV, COLS
        my_grid->SetReadOnly(row, TR_TARGET_LENGTH, cur_col->desttype==DBFTYPE_MEMO);  // length enabled except for MEMO
      }
      else // a table
      { cur_col->desttype = wb2_type_list[value].wbtype;
        my_grid->SetReadOnly(row, TR_TARGET_LENGTH, !SPECIFLEN(cur_col->desttype) && cur_col->desttype!=ATT_INT8 && cur_col->desttype!=ATT_INT16 && cur_col->desttype!=ATT_INT32 && cur_col->desttype!=ATT_INT64);  // length enabled
      }
      break;
    default:
      return;
  }
  tr_dsgn->changed=true;
}

void TransportGridTable::SetValue( int row, int col, const wxString& value )
{ tdetail * cur_col = tr_dsgn->x.dd.acc(row);  // not supposed to allocate
  switch (col)
  { case TR_SOURCE_COLUMN:
    { wxString v = value;  v.Trim(false);  v.Trim(true);
      strmaxcpy(cur_col->source, v.mb_str(*tr_dsgn->cdp->sys_conv), sizeof(cur_col->source));
     // add a new fictive row:
      if (row == tr_dsgn->x.dd.count()-1 && *cur_col->source)
      { my_grid->AppendRows(1);
        //my_grid->SetReadOnly(row+1, PARAM_LENGTH, true);  // length disabled
      }
      break;
    }
    case TR_TARGET_COLUMN:
    { wxString v = value;  v.Trim(false);  v.Trim(true);
      strmaxcpy(cur_col->destin, v.mb_str(*tr_dsgn->cdp->sys_conv), sizeof(cur_col->destin));
      break;
    }
    case TR_TARGET_LENGTH:
    { int i = 0;
      while (i<value.Length() && value[i]!=',') i++;
      long length1, length2 = 0;
      if (!value.Mid(0,i).ToLong(&length1)) { wxBell();  return; }
      if (value[i]==',' && !value.Mid(i+1).ToLong(&length2)) { wxBell();  return; }
      if (tr_dsgn->x.dbf_target())
      { if (cur_col->desttype==DBFTYPE_MEMO) { wxBell();  return; }  // length enabled except for MEMO
      }
      else if (tr_dsgn->x.db_target())
      { if (!SPECIFLEN(cur_col->desttype) && cur_col->desttype!=ATT_INT8 && cur_col->desttype!=ATT_INT16 && cur_col->desttype!=ATT_INT32 && cur_col->desttype!=ATT_INT64) 
          { wxBell();  return; }  // length disabled
      }
      cur_col->destlen1=length1;
      cur_col->destlen2=length2;
      break;
    }
  }
  tr_dsgn->changed=true;
}

void DelRowHandler::OnKeyDown(wxKeyEvent & event)
{
  if (event.GetKeyCode()==WXK_DELETE)
  { wxArrayInt ai = dlg->mGrid->GetSelectedRows();
    if (ai.GetCount() == 1)
    { int row = ai[0];
      TransportGridTable * tgt = (TransportGridTable *)dlg->mGrid->GetTable();
      tgt->DeleteRows(row, 1);
      tgt->tr_dsgn->changed=true;
      return;  // key consumed
    }
  }
  event.Skip();
}

void DelRowHandler::OnRangeSelect(wxGridRangeSelectEvent & event)
// Synchonizes the selection with the cursor
{
  if (event.Selecting())
    dlg->mGrid->SetGridCursor(event.GetTopRow(), event.GetLeftCol());
  event.Skip();
}

t_type_list DBF_type_list[] = {
{ _("CHAR"), _("C: Char"),       ATT_STRING,  0, 0 },
{ _("NUMERIC"), _("N: Numeric"), ATT_INT32,   0, 0 },
{ _("FLOAT"), _("F: Float"),     ATT_FLOAT,   0, 0 },
{ _("LOGICAL"), _("L: Logical"), ATT_BOOLEAN, 0, 0 },
{ _("DATE"), _("D: Date"),       ATT_DATE,    0, 0 },
{ _("MEMO"), _("M: Memo"),       ATT_TEXT,    0, 0 },
{ NULL, NULL,              0,           0, 0 } };

t_type_list string_type_list[] = {
{ _("String"), _("String"),      ATT_STRING,  0, 0 },
{ NULL, NULL,              0,           0, 0 } };

void DataTransDsgnDlg::CreateSpecificGrid(void)
{ if (x.source_type==IETYPE_TDT || x.dest_type==IETYPE_TDT)
  { if (mGrid) mGrid->Destroy();
    //mCreImpl->Disable();
    mGrid=NULL;
    return;
  }
  //mCreImpl->Enable();
 // create fictive parameter, if it does not exist:
  int dcnt = x.dd.count();
  if (dcnt==0 || x.dd.acc0(dcnt-1)->source[0]!=0)
    x.dd.next();
 // describe columns in the grid:
  mGrid->BeginBatch();
  TransportGridTable * transport_grid_table = new TransportGridTable(this, mGrid);
  mGrid->SetTable(transport_grid_table, TRUE);
  mGrid->SetColLabelSize(default_font_height);
  mGrid->SetDefaultRowSize(default_font_height);
  mGrid->SetRowLabelSize(22);  // for 2 digits
  mGrid->SetLabelFont(system_gui_font);
  mGrid->EnableDragRowSize(false);
  DefineColSize(mGrid, TR_SOURCE_COLUMN, 108);
  DefineColSize(mGrid, TR_TARGET_COLUMN, 86);
  if (x.creating_target)
  { DefineColSize(mGrid, TR_TARGET_TYPE, 108);
    DefineColSize(mGrid, TR_TARGET_LENGTH, 108);
  }
  wxString * keys = NULL;  int cnt=0;
 // source column:
  if (x.source_type==IETYPE_WB || x.source_type==IETYPE_CURSOR || x.source_type==IETYPE_ODBC)
  { if (x.in_obj!=NOOBJECT)
    { const d_table * td = cd_get_table_d(cdp, x.in_obj, x.source_type==IETYPE_CURSOR ? CATEG_CURSOR : CATEG_TABLE);
      if (td!=NULL)
      { int i;  const d_attr * pattr;  char qname[ATTRNAMELEN+3];
        keys = new wxString[td->attrcnt];
        for (i=first_user_attr(td), pattr=ATTR_N(td,i);        i<td->attrcnt;
             i++,                   pattr=NEXT_ATTR(td,pattr))
        { ident_to_sql(cdp, qname, pattr->name);  wb_small_string(cdp, qname, false);
          keys[cnt++]=wxString(qname, *cdp->sys_conv);
        }
        release_table_d(td);
      }
    }
  }
  else if (x.dbf_source())
  {// read xbase header
    FHANDLE hnd=CreateFileA(x.inpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hnd!=INVALID_FHANDLE_VALUE)
    { t_dbf_io * dbf_inp = new t_dbf_io(hnd, x.source_type==IETYPE_DBASE);
      if (dbf_inp->rdinit(cdp, x.inpath, NULL))
      { keys = new wxString[dbf_inp->xattrs];  
        for (int i=0;  i<dbf_inp->xattrs;  i++)
        { tobjname decoded_column_name;
          t_specif s_specif = code2specif(x.src_recode);
          superconv(ATT_STRING, ATT_STRING, dbf_inp->xdef[i].name, decoded_column_name, s_specif, t_specif(0, cdp->sys_charset, 0, 0), NULLSTRING);
          keys[cnt++]=wxString(decoded_column_name, *cdp->sys_conv);
        }
      }
      delete dbf_inp;
      CloseFile(hnd);
    }
  }
  //else // CSV, cols:
  //{ keys = new wxString[50];  char buf[10];
  //  for (int i=0;  i<50;  i++)
  //  { int2str(i+1, buf);
  //    keys[cnt++]=buf;
  //  }
  //}
  if (keys)
  { wxGridCellAttr * attr = new wxGridCellAttr;
    wxGridCellEditor * editor = new wxGridCellODChoiceEditor(cnt, keys, true);
    attr->SetEditor(editor);
    mGrid->SetColAttr(TR_SOURCE_COLUMN, attr);
    delete [] keys;
  }
  if (x.text_source())  // number editor for the source column in text formats
  { wxGridCellAttr * attr = new wxGridCellAttr;
    wxGridCellNumberEditor * editor = new wxGridCellNumberEditor(1, MAX_TABLE_COLUMNS-1);
    attr->SetEditor(editor);
    mGrid->SetColAttr(TR_SOURCE_COLUMN, attr);
  }
 // target column:
  keys=NULL;
  if (!x.creating_target)
  { if (x.out_obj!=NOOBJECT)
    { const d_table * td = cd_get_table_d(cdp, x.out_obj, CATEG_TABLE);
      if (td!=NULL)
      { int i;  const d_attr * pattr;  
        keys = new wxString[td->attrcnt];
        cnt = 0;
        for (i=first_user_attr(td), pattr=ATTR_N(td,i);        i<td->attrcnt;
             i++,                   pattr=NEXT_ATTR(td,pattr))
          keys[cnt++] = wxString(pattr->name, *cdp->sys_conv);  // not quoted, quotes added automatically when necessary
        release_table_d(td);
      }
    }
  }
  //else if (x.text_target())
  //{ keys = new wxString[50];  cnt=0;  char buf[10];
  //  for (int i=0;  i<50;  i++)
  //  { int2str(i+1, buf);
  //    keys[cnt++]=buf;
  //  }
  //}
  if (keys)
  { wxGridCellAttr * attr = new wxGridCellAttr;
    wxGridCellEditor * editor = new wxGridCellODChoiceEditor(cnt, keys, x.creating_target);
    attr->SetEditor(editor);
    mGrid->SetColAttr(TR_TARGET_COLUMN, attr);
    delete [] keys;
  }
  if (x.text_target())  // number editor for the source column in text formats
  { wxGridCellAttr * attr = new wxGridCellAttr;
    wxGridCellNumberEditor * editor = new wxGridCellNumberEditor(1, MAX_TABLE_COLUMNS-1);
    attr->SetEditor(editor);
    mGrid->SetColAttr(TR_TARGET_COLUMN, attr);
  }

  if (x.creating_target)
  {// fill types: 
    t_type_list * type_list;
    if (x.text_target())
      type_list=string_type_list;
    else if (x.db_target()) 
      type_list=wb2_type_list;
    else if (x.dbf_target())
      type_list=DBF_type_list;

    wxGridCellAttr * type_col_attr = new wxGridCellAttr;
    wxString type_choice;  int i;
    i=0;
    do
    { type_choice=type_choice+wxGetTranslation(type_list[i].long_name);
      i++;
      if (!type_list[i].long_name) break;
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
    type_choice=type_choice+wxT(", ");
    wxGridCellEnumRenderer * type_renderer = new wxGridCellEnumRenderer(type_choice);
    type_col_attr->SetRenderer(type_renderer);  // takes the ownership
    if (x.text_target())
      type_col_attr->SetReadOnly();  // single type - no editation
    mGrid->SetColAttr(TR_TARGET_TYPE, type_col_attr);
  }
  GetSizer()->SetItemMinSize(mGrid, get_grid_width(mGrid), get_grid_height(mGrid, 5));
  mGrid->EndBatch();
}

wxMenu * trans_designer::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items: 
  any_designer::get_designer_menu();  
 // add specific items:
  AppendXpmItem(designer_menu, TD_CPM_TEXT_PROP,   _("&Text Options"),  tableProperties_xpm);
  AppendXpmItem(designer_menu, TD_CPM_CRE_IMPL,    _("(Re)Create Default Design"));
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_RUN,         _("T&ransport Data"), run2_xpm);
  return designer_menu;
}

void trans_designer::update_designer_menu(void)
{ 
  designer_menu->Enable(TD_CPM_TEXT_PROP, x.text_source() || x.text_target());
  designer_menu->Enable(TD_CPM_CRE_IMPL, x.source_type!=IETYPE_TDT && x.dest_type!=IETYPE_TDT);
}

bool find_servers(t_ie_run * ier, wxWindow * parent)
{ wxString msg;
  ier->sxcdp = ier->txcdp = NULL;
 // find source server: 
  if (!*ier->inserver) ier->sxcdp = ier->cdp;
  else
  { t_avail_server * as = find_server_connection_info(ier->inserver);
    if (!as) { msg.Printf(_("Source server or DSN '%ls' not found"), wxString(ier->inserver, *wxConvCurrent).c_str());  goto err; }
    if (!as->cdp && !as->conn) { msg.Printf(_("Source server or DSN '%ls' not connected"), wxString(ier->inserver, *wxConvCurrent).c_str());  goto err; }
    ier->sxcdp = as->cdp ? (xcdp_t)as->cdp : (xcdp_t)as->conn;
  }
 // find source server: 
  if (!*ier->outserver) ier->txcdp = ier->cdp;
  else
  { t_avail_server * as = find_server_connection_info(ier->outserver);
    if (!as) { msg.Printf(_("Target server or DSN '%ls' not found"), wxString(ier->outserver, *wxConvCurrent).c_str());  goto err; }
    if (!as->cdp && !as->conn) { msg.Printf(_("Target server or DSN '%ls' not connected"), wxString(ier->outserver, *wxConvCurrent).c_str());  goto err; }
    ier->txcdp = as->cdp ? (xcdp_t)as->cdp : (xcdp_t)as->conn;
  }
  return true;
 err:
  error_box(msg, parent);
  return false;
}

bool trans_designer::open(wxWindow * parent, t_global_style my_style)
{ int res=0;
  if (modifying())
  { if (!x.load_design(objnum)) { /*cd_Signalize(cdp);*/  return false; }
    if (!find_servers(&x, parent)) return false;
    x.analyse_dsgn();  // data source may not have been found, ignoring here
    cd_Read(cdp, OBJ_TABLENUM, objnum, OBJ_NAME_ATR, NULL, object_name);
  }
  else  // new design
  { x.compile_design(initial_design);
    if (!find_servers(&x, parent)) return false;
    x.analyse_dsgn();  // data source may not have been found, ignoring here
    x.OnCreImpl();  
    *object_name=0;
    changed=true;
  }

 // open the window:
  if (!Create(parent, COMP_FRAME_ID))
    res=5;  // window not opened
  else
  { competing_frame::focus_receiver = this;
    show_source_target_info();
    if (mGrid) mGrid->SetFocus();  // must specify the initial focus to the control container, trans_designer passes the its focus to it
    return true;  // OK
  }
  return res==0;
}

bool start_transport_designer(cdp_t cdp, tobjnum objnum, const char * folder_name, char * initial_design)
{ trans_designer * dsgn;
  dsgn = new trans_designer(cdp, objnum, folder_name, cdp->sel_appl_name, initial_design);
  if (!open_standard_designer(dsgn))
    return false;
  if (!dsgn->modifying())
    if (dsgn->x.dest_exists)
      report_mapping_result(dsgn->x, dsgn->dialog_parent());
  return true;
}

#if 0
bool start_transport_designer(wxMDIParentFrame * frame, cdp_t cdp, tobjnum objnum, const char * folder_name, char * initial_design)
{ wxTopLevelWindow * dsgn_frame;  trans_designer * dsgn;
  if (global_style==ST_MDI)
  { DesignerFrameMDI * frm = new DesignerFrameMDI(wxGetApp().frame, -1, wxEmptyString);
    if (!frm) return false;
    dsgn_frame=frm;
    dsgn = new trans_designer(cdp, objnum, folder_name, cdp->sel_appl_name, dsgn_frame);
    if (!dsgn) goto err;
    frm->dsgn = dsgn;
  }
  else
  { DesignerFramePopup * frm = new DesignerFramePopup(wxGetApp().frame, -1, wxEmptyString, transdsng_coords.get_pos(), transdsng_coords.get_size());
    if (!frm) return false;
    dsgn_frame=frm;
    dsgn = new trans_designer(cdp, objnum, folder_name, cdp->sel_appl_name, dsgn_frame);
    if (!dsgn) goto err;
    frm->dsgn = dsgn;
  }
  wxGetApp().Yield();
  if (dsgn->modifying() && !dsgn->lock_the_object())
    cd_Signalize(cdp);   // must Destroy dsgn because it has not been opened as child of designer_ch
  else if (dsgn->modifying() && !check_not_encrypted_object(cdp, CATEG_PGMSRC, objnum)) 
    ;  // must Destroy dsgn because it has not been opened as child of designer_ch  // error signalised inside
  else
    if (dsgn->open(dsgn_frame, initial_design)==0) 
    { dsgn_frame->SetIcon(wxIcon(_transp_xpm));  dsgn_frame->Show();  
      if (!dsgn->modifying())
        if (dsgn->x.dest_exists)
          report_mapping_result(dsgn->x, dsgn->dialog_parent());
      return true; 
    } // designer opened OK
    else   
#if 0  // dsgn must be removed from the list of competing frames!
      dsgn=NULL;
#else    
      if (!dsgn->GetHandle()) { delete dsgn;  dsgn=NULL; }  // assert in Destroy fails if not created
#endif
 // error exit:
 err:
  dsgn->activate_other_competing_frame(); // with MDI style the tools of the previous competing frame are not restored because the frame will not receive the Activation signal. 
  // Must be before Destroy because dsgn is sent the deactivate event.
  if (dsgn) dsgn->Destroy();  // must Destroy dsgn because it has not been opened as child of designer_ch
  if (global_style==ST_MDI) ((DesignerFrameMDI *)dsgn_frame)->dsgn = NULL;  // prevents passing focus
  dsgn_frame->Destroy();
  return false;
}
#endif
