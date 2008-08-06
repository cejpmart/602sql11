// ftdsgn.cpp
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
#include "compil.h"
#include "comp.h"
#include "tabledsgn.h"
#include "impexp.h"
#include "xrc/ShowTextDlg.h"
           
#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define ADD_NEW_ID_COLUMN  _("-- Add new ID column --")
extern char *delete_row_xpm[];
DECLARE_EVENT_TYPE(wxEVT_NEW_ID_COLUMN_EVENT, -1)
DEFINE_EVENT_TYPE(wxEVT_NEW_ID_COLUMN_EVENT)

#include "ftanal.cpp"
class fulltext_designer;
#include "xrc/FtxDsgn.h"

class fulltext_designer : public FtxDsgn, public any_designer
{
 public:
  fulltext_designer(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn) :
    any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, CATEG_INFO, _full_xpm, &fulltext_coords), 
    FtxDsgn(this)
      { changed=false; }
  ~fulltext_designer(void)
  { remove_tools(); 
  }
  bool open(wxWindow * parent, t_global_style my_style);
  t_fulltext ftdef;  // the edited definition
  t_autoindex * get_item_by_row(int row, int * ind = NULL);
  void prepare_ID_column_choice(int row, bool select1st);
  void prepare_text_column_choice(int row, bool select1st);
  void prepare_grid(wxGrid * mGrid);
  void action(int act);
  void delete_selected_rows(void);
  int orig_items;
  bool validate(void);
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
  wxMenu * get_designer_menu(void);
  void update_designer_menu(void);
  void _set_focus(void)
    { if (GetHandle()) SetFocus(); }
  void set_changed(void)
    { changed=true; }
  void table_altered_notif(ttablenum tbnum);
 // member variables:
  bool changed;

 // Callbacks:
  void OnKeyDown(wxKeyEvent & event);
  void OnCdFtdGridRangeSelect( wxGridRangeSelectEvent& event );
  void OnCdFtdReindexClick( wxCommandEvent& event );
  void OnCdFtdAddidcolClick( wxCommandEvent& event );
  void OnCdFtdLimitsUpdated1( wxCommandEvent& event );
  DECLARE_EVENT_TABLE()
};

table_all * create_ta_for_ftx(cdp_t cdp, ttablenum tabobj, const char * ftx_name);

#include "xrc/FtxDsgn.cpp"
#include "xrc/NewFtxIdCol.cpp"
//////////////////////////////////////////// grid /////////////////////////////////////////////////////
class FtxGridTable : public wxGridTableBase
{ fulltext_designer * ftd;
 public:
    virtual int GetNumberRows();
 private:
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void SetValue( int row, int col, const wxString& value );

    virtual wxString GetColLabelValue( int col );
    bool AppendRows(size_t numRows = 1);
    //bool InsertRows(size_t pos = 0, size_t numRows = 1);  // used only for initial filling
    bool DeleteRows(size_t pos = 0, size_t numRows = 1);
    bool CanGetValueAs( int row, int col, const wxString& typeName );
    //long GetValueAsLong( int row, int col);
    //void SetValueAsLong( int row, int col, long value );
 public:
  FtxGridTable(fulltext_designer * ftdIn)
    { ftd=ftdIn; }
};

// columns in the grid:
#define FTX_TABLE     0
#define FTX_ID        1
#define FTX_VALUE     2
#define FTX_INDIRECT  3
#define FTX_FORMAT    4     
#define FTX_COLUMN_COUNT 5

wxString FtxGridTable::GetColLabelValue( int col )
{ switch (col)
  { case FTX_TABLE:    return _("Document table");
    case FTX_ID:       return _("Document ID column");
	  case FTX_VALUE:    return _("Document column");
	  case FTX_INDIRECT: return _("Indirect");
    case FTX_FORMAT:  return  _("Format");
	  default: return wxEmptyString;
  }	
}

int FtxGridTable::GetNumberRows()
{ int cnt=0;
  for (int i=0;  i<ftd->ftdef.items.count();  i++)
    if (ftd->ftdef.items.acc0(i)->tag!=ATR_DEL)
      cnt++;
  return cnt;
}

int FtxGridTable::GetNumberCols()
{ return FTX_COLUMN_COUNT; }

bool FtxGridTable::IsEmptyCell( int row, int col )
{ return false; }

bool FtxGridTable::AppendRows(size_t numRows)
// only numRows==1 implemented
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}
bool FtxGridTable::DeleteRows(size_t pos, size_t numRows)
// only numRows==1 implemented
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

#if 0
bool FtxGridTable::InsertRows(size_t pos, size_t numRows)
// Used only for initial inserting
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, pos, numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}
#endif

bool FtxGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{ if (typeName == wxGRID_VALUE_STRING) return true;
//  if (typeName == wxGRID_VALUE_NUMBER)
//    if (col==PAR_MODE) return true;
  return false;
}

wxString FtxGridTable::GetValue( int row, int col )
{ t_autoindex * ai = ftd->get_item_by_row(row);
  switch (col)
  { case FTX_TABLE: 
    { wxString fullname = wxString(ai->doctable_name, *ftd->cdp->sys_conv);
      if (*ai->doctable_schema) fullname = wxString(wxT("`")) + wxString(ai->doctable_schema, *ftd->cdp->sys_conv) + wxT("`.`") + fullname + wxT("`");
      return fullname;
    }
    case FTX_ID:
      return wxString(ai->id_column, *ftd->cdp->sys_conv);
    case FTX_VALUE:
      return wxString(ai->text_expr, *ftd->cdp->sys_conv);
    case FTX_INDIRECT:
      return ai->mode ? wxT("1") : wxT("0");
    case FTX_FORMAT:
      return *ai->format ? wxString(ai->format, *ftd->cdp->sys_conv) : *ai->doctable_name ? wxString(_("AUTO")) : wxString();
  }
  return wxEmptyString;
}

static int global_row;

void FtxGridTable::SetValue( int row, int col, const wxString& value )
{ int ind;
  t_autoindex * ai = ftd->get_item_by_row(row, &ind);
  if (ind+1==ftd->ftdef.items.count())
    AppendRows(1);
  switch (col)
  { case FTX_TABLE:                          
      strmaxcpy(ai->doctable_name, value.mb_str(*ftd->cdp->sys_conv), sizeof(ai->doctable_name));
     // update column choices for the table:
      *ai->id_column=0;  // drop the ID column - it may not exist in the new table
      *ai->text_expr=0;  // drop the text column - it may not exist in the new table
      ftd->prepare_ID_column_choice(row, true);
      ftd->prepare_text_column_choice(row, true);
      ftd->Refresh();  // because of updated ID column contents
      break;
    case FTX_ID:
      if (value==ADD_NEW_ID_COLUMN)
      { tobjnum objnum;
        if (cd_Find_prefixed_object(ftd->cdp, ai->doctable_schema, ai->doctable_name, CATEG_TABLE, &objnum))
          { cd_Signalize2(ftd->cdp, ftd->dialog_parent());  break; }  // improbable, table recently existed
#ifdef STOP  // dialog must be open off-line, otherwise crashes since 2.6.1
        NewFtxIdCol dlg(ftd->cdp, objnum, ftd->ftdef.name);
        dlg.Create(ftd->dialog_parent());
        if (dlg.ShowModal() == wxID_OK)  // table modified
        { for (trecnum r=0;  r<GetNumberRows();  r++)
          { t_autoindex * ai = ftd->get_item_by_row(r);
            if (ai->tag == ATR_NEW)
              ftd->prepare_ID_column_choice(r, false);
          }
          strmaxcpy(ai->id_column, dlg.newcolname.mb_str(*ftd->cdp->sys_conv), sizeof(ai->id_column));
        }
#else
        global_row = row;
        wxCommandEvent event( wxEVT_NEW_ID_COLUMN_EVENT, 0 );
        ftd->AddPendingEvent(event);
#endif
      }
      else
        strmaxcpy(ai->id_column, value.mb_str(*ftd->cdp->sys_conv), sizeof(ai->id_column));
      break;
    case FTX_VALUE:
      strmaxcpy(ai->text_expr, value.mb_str(*ftd->cdp->sys_conv), sizeof(ai->text_expr));
      break;
    case FTX_INDIRECT:
      ai->mode = value.c_str()[0]=='1' ? 1 : 0;  break;
    case FTX_FORMAT:
      if (value==_("AUTO")) *ai->format=0;
      else strmaxcpy(ai->format, value.mb_str(*ftd->cdp->sys_conv), sizeof(ai->format));
      break;
  }
  ftd->changed=true;
}

t_autoindex * fulltext_designer::get_item_by_row(int row, int * ind)
// Returns row-th not deleted item from ftdef, adds new items when necessary.
{ int i=0;  t_autoindex * ai;
  do
  { if (i<ftdef.items.count())
      ai = ftdef.items.acc(i);
    else
    { ai = ftdef.items.next();
      ai->tag=ATR_NEW;
    }
    if (ai->tag!=ATR_DEL)
      if (row) row--;
      else 
      { if (ind) *ind = i;
        return ai;
      }
    i++;
  } while (true);
}

table_all * create_ta_for_ftx(cdp_t cdp, ttablenum tabobj, const char * ftx_name)
{ int a;  char seq_val1[OBJ_NAME_LEN+10+1], seq_val2[OBJ_NAME_LEN+10+1];
  strcpy(seq_val1, "FTX_DOCID");   strcat(seq_val1, ftx_name);  strcat(seq_val1, ".nextval");
  strcpy(seq_val2, "`FTX_DOCID");  strcat(seq_val2, ftx_name);  strcat(seq_val2, "`.nextval");
 // load and compile table definition:
  const char * defin=cd_Load_objdef(cdp, tabobj, CATEG_TABLE);
  if (!defin) return NULL;
  table_all * ta = new table_all;
  int err=compile_table_to_all(cdp, defin, ta);
  corefree(defin);
  if (err) { delete ta;  return NULL; }
 // for columns with the proper type check the default value and index:
  for (a=1;  a<ta->attrs.count();  a++)
  { atr_all * atr = ta->attrs.acc0(a);
    if (atr->type==ATT_INT32 || atr->type==ATT_INT64)
    { atr->orig_type = 0;
     // find index:
      ind_descr * indx = get_index_by_column(cdp, ta, atr->name);
      if (indx && indx->index_type!=INDEX_NONUNIQUE)  // PRIMARY KEY or UNIQUE
        atr->orig_type |= 1;
     // check the default value:
      if (atr->defval!=NULL)
        if (!wb_stricmp(cdp, atr->defval, seq_val1) || !wb_stricmp(cdp, atr->defval, seq_val2))
          atr->orig_type |= 2;
        else
          atr->orig_type |= 4;
    }
  }
  return ta;
}

void fulltext_designer::prepare_ID_column_choice(int row, bool select1st)
{ tobjnum objnum;
  t_autoindex * ai = get_item_by_row(row);
  wxString id_choice;  bool first=true;
  if (!cd_Find_prefixed_object(cdp, ai->doctable_schema, ai->doctable_name, CATEG_TABLE, &objnum))
  { 
    table_all * ta = create_ta_for_ftx(cdp, objnum, ftdef.name);
    if (ta)
    { for (int a=1;  a<ta->attrs.count();  a++)
      { atr_all * att = ta->attrs.acc0(a);
        if (att->type==ATT_INT32 || att->type==ATT_INT64)
          if (att->orig_type == 3 || att->orig_type == 5)
          { if (first) 
            { first=false;  
              if (select1st) 
              { strcpy(ai->id_column, att->name);
                if (att->orig_type == 5) strcat(ai->id_column, " (?)");
              }
            }
            else id_choice=id_choice+wxT(",");
            id_choice=id_choice+wxString(att->name, *cdp->sys_conv);
            if (att->orig_type == 5)
              id_choice=id_choice+wxT(" (?)");
          }
      }
      delete ta;
     // add the items for adding a new ID column (only if the table exists):
      if (!first) id_choice=id_choice+wxT(",");
      id_choice=id_choice+ADD_NEW_ID_COLUMN;
    }
  }
  wxGridCellODChoiceEditor * editor = new wxGridCellODChoiceEditor(0, NULL, false);
  editor->SetParameters(id_choice);
  mGrid->SetCellEditor(row, FTX_ID, editor);
}

void fulltext_designer::prepare_text_column_choice(int row, bool select1st)
{ tobjnum objnum;
  t_autoindex * ai = get_item_by_row(row);
  wxString tx_choice;  bool first=true;
  if (!cd_Find_prefixed_object(cdp, ai->doctable_schema, ai->doctable_name, CATEG_TABLE, &objnum))
  { const d_table * td = cd_get_table_d(cdp, objnum, CATEG_TABLE);
    if (td)
    { for (int i=1;  i<td->attrcnt;  i++)
      { const d_attr * att = &td->attribs[i];
        if (att->type==ATT_TEXT || att->type==ATT_STRING || att->type==ATT_NOSPEC)
        { if (first) 
          { first=false;  
            if (select1st) strcpy(ai->text_expr, att->name);
          }
          else tx_choice=tx_choice+wxT(",");
          tx_choice=tx_choice+wxString(att->name, *cdp->sys_conv);
        }
      }
      release_table_d(td);
    }
  }
  wxGridCellODChoiceEditor * tx_editor = new wxGridCellODChoiceEditor(0, NULL, false);
  tx_editor->SetParameters(tx_choice);
  mGrid->SetCellEditor(row, FTX_VALUE, tx_editor);
}

void fulltext_designer::prepare_grid(wxGrid * mGrid)
{ 
 // for the fictive row:
  orig_items = ftdef.items.count();
  ftdef.items.next()->tag = ATR_NEW;
 // grid:
  mGrid->BeginBatch();
  FtxGridTable * ftx_grid_table = new FtxGridTable(this);
  mGrid->SetTable(ftx_grid_table, TRUE);
  mGrid->SetColLabelSize(default_font_height);
  mGrid->SetDefaultRowSize(default_font_height);
  mGrid->SetRowLabelSize(24);  // just for selecting and numbering
  mGrid->SetLabelFont(system_gui_font);
  DefineColSize(mGrid, FTX_TABLE,   120);
  DefineColSize(mGrid, FTX_ID,      140);
  DefineColSize(mGrid, FTX_VALUE,   140);
  DefineColSize(mGrid, FTX_INDIRECT, 60);
  DefineColSize(mGrid, FTX_FORMAT,   80);
 // prepare table name choice:
  wxString table_choice;  bool first=true;
  void * en = get_object_enumerator(cdp, CATEG_TABLE, cdp->sel_appl_name);
  tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
  while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
    if (!IS_ODBC_TABLE(objnum))
      if (memcmp(name, "FTX_WORDTAB", 11) && memcmp(name, "FTX_REFTAB", 10) && memcmp(name, "FTX_DOCPART", 11))
      { if (first) first=false;  else table_choice=table_choice+wxT(",");
        table_choice=table_choice+wxString(name, *cdp->sys_conv);
      }  
  free_object_enumerator(en);
  wxGridCellAttr * tabname_attr = new wxGridCellAttr;
  wxGridCellODChoiceEditor * tabname_editor = new wxGridCellODChoiceEditor(0, NULL, true);
  tabname_editor->SetParameters(table_choice);
  tabname_attr->SetEditor(tabname_editor);
  mGrid->SetColAttr(FTX_TABLE, tabname_attr);
 // mode column:
  wxGridCellAttr * bool_col_attr = new wxGridCellAttr;
  wxGridCellBoolEditor * bool_editor = new wxGridCellBoolEditor();
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>=8
  bool_editor->UseStringValues(wxT("1"), wxT("0"));
#endif
  bool_col_attr->SetEditor(bool_editor);
  wxGridCellBoolRenderer * bool_renderer = new wxGridCellBoolRenderer();
  bool_col_attr->SetRenderer(bool_renderer);  // takes the ownership
  mGrid->SetColAttr(FTX_INDIRECT, bool_col_attr);
 // format column:
  wxGridCellAttr * format_attr = new wxGridCellAttr;
  wxGridCellODChoiceEditor * format_editor = new wxGridCellODChoiceEditor(0, NULL, true);
  format_editor->SetParameters(wxT("AUTO,plain,HTML,XML,DOC,XLS,PPT,PDF,OOO,FO,ZFO"));
  format_attr->SetEditor(format_editor);
  mGrid->SetColAttr(FTX_FORMAT, format_attr);

  mGrid->EnableDragRowSize(false);
 // disable value edit for original rows:
  for (trecnum r=0;  r<orig_items;  r++)
  { 
    ftdef.items.acc0(r)->tag = ATR_OLD;
    prepare_ID_column_choice(r, false);
    prepare_text_column_choice(r, false);
    wxGridCellAttr * old_attr = new wxGridCellAttr;
    old_attr->SetReadOnly();
    mGrid->SetRowAttr(r, old_attr);
  }
  mGrid->EndBatch();
}

wxArrayInt UnivGetSelectedRows(wxGrid * grid)
// Returs the list of selected rows independently of the selection method.
{ wxArrayInt selrows;
  wxArrayInt wxsel = grid->GetSelectedRows();
  wxGridCellCoordsArray topleft = grid->GetSelectionBlockTopLeft();
  wxGridCellCoordsArray botri   = grid->GetSelectionBlockBottomRight();
 // when there is no block selection, return the row selection:
  if (topleft.GetCount()==0)
    return wxsel;
 // merge both selections:
  int i, j;
  for (i=0;  i<wxsel.GetCount();  i++)
    selrows.Add(wxsel[i]);
  for (i=0;  i<topleft.GetCount();  i++)
    for (j=topleft[i].GetRow();  j<=botri[i].GetRow();  j++)
      selrows.Add(j);
  return selrows;
}

void fulltext_designer::action(int act)
{ int i, done;
  wxArrayInt selrows = UnivGetSelectedRows(mGrid);
  int cnt = (int)selrows.GetCount();
  switch (act)
  { case 0:
      if (cnt==1)
      { int pos;
        get_item_by_row(selrows[0], &pos);
        if (pos+1==ftdef.items.count())  // fictive row selected
          cnt--;
      }
      //mAddIdCol->Enable(cnt==1);
      //mDelete->Enable(cnt>=1);
      mReindex->Enable(cnt>=1);
      break;
    case 1:  // add column:
    { ttablenum objnum;
      t_autoindex * ai = get_item_by_row(global_row);
      mGrid->DisableCellEditControl();
      if (cd_Find_prefixed_object(cdp, ai->doctable_schema, ai->doctable_name, CATEG_TABLE, &objnum))
        { cd_Signalize2(cdp, dialog_parent());  break; }
      NewFtxIdCol dlg(cdp, objnum, ftdef.name);
      dlg.Create(dialog_parent());
      if (dlg.ShowModal() == wxID_OK)  // table modified
      { for (trecnum r=0;  r<mGrid->GetNumberRows();  r++)
        { t_autoindex * ai2 = get_item_by_row(r);
          if (ai2->tag == ATR_NEW)
            prepare_ID_column_choice(r, false);
        }
        strmaxcpy(ai->id_column, dlg.newcolname.mb_str(*cdp->sys_conv), sizeof(ai->id_column));
      }
      mGrid->Refresh();
      break;
    }
    case 2:  // reindex:
      done=0;
      for (i=0;  i<cnt;  i++)
      { wxBusyCursor wait;
        t_autoindex * ai = get_item_by_row(selrows[i]);
        if (ai->tag==ATR_DEL || !*ai->doctable_name || !*ai->id_column || !*ai->text_expr) continue;
        char cmd[200+2*OBJ_NAME_LEN+2*ATTRNAMELEN+MAX_FT_FORMAT];
        char basename[ATTRNAMELEN+1];
        base_column_name(ai->id_column, basename);
#ifdef STOP  // this is wrong, removes other indexed columns when adding the new one
        strcpy(cmd, "FOR c AS table `");
        if (*ai->doctable_schema)
          { strcat(cmd, ai->doctable_schema);  strcat(cmd, "`.`"); }
        sprintf(cmd+strlen(cmd), "%s` DO CALL Fulltext_index_doc(\'%s.%s\',c.`%s`,c.`%s`,\'%s\',%d); END FOR;",
          ai->doctable_name, ftdef.schema, ftdef.name, basename, ai->text_expr, ai->format, ai->mode);
#else
        strcpy(cmd, "UPDATE `");
        if (*ai->doctable_schema)
          { strcat(cmd, ai->doctable_schema);  strcat(cmd, "`.`"); }
        sprintf(cmd+strlen(cmd), "%s` SET `%s`=`%s`", ai->doctable_name, ai->text_expr, ai->text_expr);
#endif
        if (cd_SQL_execute(cdp, cmd, NULL))
          { cd_Signalize2(cdp, dialog_parent());  break; }
        done++;
      }
      if (done)
      { wxString msg;
        msg.Printf(_("Reindexed %d document source(s)."), done);
        info_box(msg, dialog_parent());
      }
      break;
  }
}

/////////////////////////// virtual methods (commented in class any_designer): //////////////////////////////////

char * fulltext_designer::make_source(bool alter)
{// save "limits", other values are saved: 
  corefree(ftdef.limits);  // used in ALTER FULLTEXT
  ftdef.limits=(tptr)corealloc(strlen(mLimits->GetValue().mb_str(*wxConvCurrent))+1, 99);
  if (!ftdef.limits) { wxBell();  return NULL; }
  strcpy(ftdef.limits, mLimits->GetValue().mb_str(*wxConvCurrent));
 // create source:
  strcpy(ftdef.schema, schema_name);
  return ftdef.to_source(cdp, alter); 
}

void fulltext_designer::set_designer_caption(void)
{ wxString caption;
  if (!modifying()) caption = _("Design a New Fulltext System");
  else caption.Printf(_("Design of Fulltext System %s"), wxString(ftdef.name, *cdp->sys_conv).c_str());
  set_caption(caption);
}

bool fulltext_designer::IsChanged(void) const
{ return changed; }

wxString fulltext_designer::SaveQuestion(void) const
{ return modifying() ?
    _("Your changes have not been saved. Do you want to save your changes?") :
    _("The fulltext system has not been created yet. Do you want to save the fulltext system?");
}

bool fulltext_designer::validate(void)
{ int i,j;
  mGrid->DisableCellEditControl();
 // validate individual items:
  for (i=orig_items;  i<ftdef.items.count();  i++)
  { t_autoindex * ai = ftdef.items.acc0(i);
    if (!*ai->doctable_name && !*ai->id_column && !*ai->text_expr) continue;  // ignore empty lines
    mGrid->SetGridCursor(i, 1);
    if (!*ai->doctable_name || !*ai->id_column || !*ai->text_expr) 
      { error_box(_("Incomplete specification."), dialog_parent());  return false; }
    tobjnum objnum;  d_attr info;
    if (cd_Find_prefixed_object(cdp, ai->doctable_schema, ai->doctable_name, CATEG_TABLE, &objnum))
      { cd_Signalize2(cdp, dialog_parent());  return false; }
    char basename[ATTRNAMELEN+1];
    base_column_name(ai->id_column, basename);
    if (!find_attr(cdp, objnum, CATEG_TABLE, basename, NULL, NULL, &info))
      { error_box(_("ID column not found."), dialog_parent());  return false; }
    if (!find_attr(cdp, objnum, CATEG_TABLE, ai->text_expr, NULL, NULL, &info))
      { error_box(_("Text column not found in the table."), dialog_parent());  return false; }
  }
 // find the use of duplicate IDs: 
  for (i=0;  i<ftdef.items.count()-1;  i++)
  { t_autoindex * ai1 = ftdef.items.acc0(i);
    if (!*ai1->doctable_name) continue;  // ignore empty lines
    for (j=i+1;  j<ftdef.items.count();  j++)
    { t_autoindex * ai2 = ftdef.items.acc0(j);
      if (!strcmp(ai1->id_column, ai2->id_column))
        if (!strcmp(ai1->doctable_name, ai2->doctable_name))
          if (!strcmp(ai1->doctable_schema, ai2->doctable_schema))
          { mGrid->SetGridCursor(j, 1);
            error_box(_("The same ID column used more than once."), dialog_parent());  return false; 
          }
    }
  }  
  return true;
}

bool fulltext_designer::save_design(bool save_as)
// Saves the fulltext system design, returns true on success.
{ BOOL res;  char * src;
  mGrid->DisableCellEditControl();
  if (!validate()) return false;  // prevents saving an invalid design
  if (!IsChanged() && !save_as) return true;  // empty action
 // creating a new fulltext:
  if (!modifying() || save_as)
  { if (save_as || !*ftdef.name) // save as or name specified outside
    { *ftdef.name = 0;
      if (!get_name_for_new_object(cdp, dialog_parent(), schema_name, category, _("Enter the name of the new fulltext system:"), ftdef.name))
        return false;  // entering the name cancelled
    }
   // storing the definition:
    src=make_source(false);
    if (!src) return false;
    { { t_temp_appl_context tac(cdp, schema_name);
        res=cd_SQL_execute(cdp, src, NULL);
        if (res) cd_Signalize(cdp); 
      }
      corefree(src);
      if (res) return false; 
      unlock_the_object();  // important for SaveAs of an existing object
      cd_Find_prefixed_object(cdp, schema_name, ftdef.name, CATEG_INFO, &objnum);
      lock_the_object();
      Set_object_folder_name(cdp, objnum, category, folder_name);
      Add_new_component(cdp, category, schema_name, ftdef.name, objnum, ftdef.separated ? CO_FLAG_FT_SEP : 0, folder_name, true);
      cd_Relist_objects_ex(cdp, TRUE);
    }
    set_designer_caption();
  }
  else  // modifying an existing fulltext
  { src=make_source(true);
    if (!src) return false;
    { t_temp_appl_context tac(cdp, schema_name);
      wxBeginBusyCursor();
      res=cd_SQL_execute(cdp, src, NULL);
      wxEndBusyCursor();
      corefree(src);
      if (res) cd_Signalize(cdp);
      if (res) return false;
      Changed_component(cdp, category, schema_name, ftdef.name, objnum, 0);
    }
  }
  changed=false;
  return true;
}

void fulltext_designer::OnCommand(wxCommandEvent & event)
{ 
  switch (event.GetId())
  { case TD_CPM_SAVE:
      if (!IsChanged()) break;  // no message if no changes
      if (!save_design(false)) break;  // error saving, break
      destroy_designer(GetParent(), &fulltext_coords);
      //info_box(_("Your design has been saved."), dialog_parent());
      break;
    case TD_CPM_SAVE_AS:
      if (!save_design(true)) break;  // error saving, break
      //info_box(_("Your design has been saved under a new name."), dialog_parent());
      destroy_designer(GetParent(), &fulltext_coords);
      break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
    case TD_CPM_EXIT_FRAME:
      if (exit_request(this, true))
        destroy_designer(GetParent(), &fulltext_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
      exit_request(this, false);
      destroy_designer(GetParent(), &fulltext_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_HELP:
      wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_fulltext.html"));  
      break;
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
      if (validate())
        info_box(_("The design is valid."), dialog_parent());
      break;
    case TD_CPM_DELREC:
      delete_selected_rows();  
      break;
  }
}

void fulltext_designer::delete_selected_rows(void)
{ wxArrayInt selrows = UnivGetSelectedRows(mGrid);
  if (selrows.GetCount()==0)
    { error_box(_("No records selected"));  return; }
 // warning:
  wxString msg;
  if (selrows.GetCount()==1)
    msg.Printf(_("Do you really want to delete the selected row?"));
  else
    msg.Printf(_("Do you really want to delete %u rows?"), selrows.GetCount());
  if (!yesno_box(msg, dialog_parent()))
    return;
 // deleting
  int i,j;
  for (i=0;  i<selrows.GetCount();  i++)
  { trecnum selrow=NORECNUM;
   // get max row:
    for (j=0;  j<selrows.GetCount();  j++)
      if (selrows[j]!=NORECNUM)  // unless taken before
        if (selrow==NORECNUM || selrow<selrows[j])
          selrow=selrows[j];
   // remove the max row from selrows:
    for (j=0;  j<selrows.GetCount();  j++)
      if (selrow==selrows[j])
        selrows[j]=NORECNUM;
    int ind;
    t_autoindex * ai = get_item_by_row(selrow, &ind);
    if (ind+1==ftdef.items.count()) continue;
    if (ai->tag==ATR_OLD)
      ai->tag=ATR_DEL;
    else
      ftdef.items.delet(ind);
    mGrid->DeleteRows(selrow);
  }
  changed=true;
}

void fulltext_designer::OnCdFtdAddidcolClick( wxCommandEvent& event )
{
  action(1);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FTD_REINDEX
 */

void fulltext_designer::OnCdFtdReindexClick( wxCommandEvent& event )
{
  action(2);
}


/*!
 * wxEVT_GRID_RANGE_SELECT event handler for CD_FTD_GRID
 */

void fulltext_designer::OnCdFtdGridRangeSelect( wxGridRangeSelectEvent& event )
{
  action(0);
  event.Skip();
}


void fulltext_designer::OnKeyDown( wxKeyEvent& event )
{ 
  if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_NUMPAD_DELETE)
    delete_selected_rows();
  else
    OnCommonKeyDown(event);  // event.Skip() is inside
}

void fulltext_designer::OnCdFtdLimitsUpdated1( wxCommandEvent& event )
{
  changed=true;  
  event.Skip();
}

BEGIN_EVENT_TABLE( fulltext_designer, wxPanel)
  EVT_KEY_DOWN(fulltext_designer::OnKeyDown)
  EVT_GRID_RANGE_SELECT( fulltext_designer::OnCdFtdGridRangeSelect )
  EVT_BUTTON( CD_FTD_REINDEX, fulltext_designer::OnCdFtdReindexClick )
  EVT_TEXT( CD_FTD_LIMITS, fulltext_designer::OnCdFtdLimitsUpdated1 )
  EVT_COMMAND(-1, wxEVT_NEW_ID_COLUMN_EVENT, fulltext_designer::OnCdFtdAddidcolClick)
END_EVENT_TABLE()

t_tool_info fulltext_designer::tool_info[TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,      File_save_xpm,   wxTRANSLATE("Save Design")),
  t_tool_info(TD_CPM_EXIT,      exit_xpm,        wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CHECK,     validateSQL_xpm, wxTRANSLATE("Validate")),
  t_tool_info(TD_CPM_SQL,       showSQLtext_xpm, wxTRANSLATE("Show SQL")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_DELREC,    delete_row_xpm,  wxTRANSLATE("Delete selected row(s)")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_HELP,      help_xpm,        wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

void fulltext_delete_bitmaps(void)
{ delete_bitmaps(fulltext_designer::tool_info); 
}

wxMenu * fulltext_designer::get_designer_menu(void)
{
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items:
  any_designer::get_designer_menu();
 // add specific items:
  AppendXpmItem(designer_menu, TD_CPM_SQL,     _("Show S&QL"), showSQLtext_xpm);
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_DELREC,  _("Delete selected row(s)\tDelete"), delete_row_xpm);
  return designer_menu;
}

void fulltext_designer::update_designer_menu(void)
{ 
  designer_menu->Enable(TD_CPM_DELREC, UnivGetSelectedRows(mGrid).GetCount()>0);
}

bool fulltext_designer::open(wxWindow * parent, t_global_style my_style)
{ int res=0;
  if (modifying())
  { char * def = cd_Load_objdef(cdp, objnum, CATEG_DRAWING, NULL);
    if (!def) return false;
    ftdef.modifying=false;
    int res=ftdef.compile(cdp, def);
    corefree(def);
    if (res) 
    { error_box(_("Cannot edit the fulltext system definition from 602SQL 8"), wxGetApp().frame);
      return false;  // Create() not called, "this" is not a child of [parent], must delete
    }
  }
  else  // new design
  { *ftdef.name=0;
    ftdef.basic_form=true;
    ftdef.language=0;  // change it to the default language
    ftdef.bigint_id=false;  // default
    changed=true;
  }
 // open the window:
  if (!Create(parent, COMP_FRAME_ID))
    res=5;  // window not opened, "this" is not a child of [parent], must delete
  else
  { competing_frame::focus_receiver = this;
   // showing data:
    prepare_grid(mGrid);
    mLang->SetLabel(wxGetTranslation(wxString(ft_lang_name[ftdef.language], *wxConvCurrent)));
    mLemma->Show(ftdef.basic_form);
    mSubstruct->SetLabel(ftdef.with_substructures ? _("Recognizes the structure of files") : _("Does not recognize the structure of files"));
    mExternal->SetLabel(ftdef.separated ? _("[Stored externally]") : _("[Stored internally]"));
    mLimits->SetValue(wxString(ftdef.limits, *wxConvCurrent));
    changed=false;  // mLimits->SetValue() set true to [changed]
    SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);
    action(0);  // disables buttons
    //Layout();
    return true;  // OK
  }
  return res==0;
}

void fulltext_designer::table_altered_notif(ttablenum tbnum)
{ bool has_the_table = false;
  // ####
  Refresh();
}

bool start_fulltext_designer(cdp_t cdp, tobjnum objnum, const char * folder_name)
{ 
  return open_standard_designer(new fulltext_designer(cdp, objnum, folder_name, cdp->sel_appl_name));
}

#if 0
bool start_fulltext_designer(wxMDIParentFrame * frame, cdp_t cdp, tobjnum objnum, const char * folder_name)
{ wxTopLevelWindow * dsgn_frame;  fulltext_designer * dsgn;  DesignerFrameMDI * frmM;  DesignerFramePopup * frmP;
  if (global_style==ST_MDI)
  { frmM = new DesignerFrameMDI(wxGetApp().frame, -1, wxEmptyString);
    if (!frmM) return false;
    dsgn_frame=frmM;
    dsgn = new fulltext_designer(cdp, objnum, folder_name, cdp->sel_appl_name, dsgn_frame);
    if (!dsgn) goto err;
    frmM->dsgn = dsgn;
  }
  else
  { frmP = new DesignerFramePopup(wxGetApp().frame, -1, wxEmptyString, fulltext_coords.get_pos(), fulltext_coords.get_size(), wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | /*wxFRAME_NO_TASKBAR |*/ wxWANTS_CHARS);
    if (!frmP) return false;
    dsgn_frame=frmP;
    dsgn = new fulltext_designer(cdp, objnum, folder_name, cdp->sel_appl_name, dsgn_frame);
    if (!dsgn) goto err;
    frmP->dsgn = dsgn;
  }
  if (dsgn->modifying() && !dsgn->lock_the_object())
    cd_Signalize(cdp);  // must Destroy dsgn because it has not been opened as child of designer_ch
  else if (dsgn->modifying() && !check_not_encrypted_object(cdp, CATEG_INFO, objnum)) 
    ;  // must Destroy dsgn because it has not been opened as child of designer_ch  // error signalised inside
  else
    if (dsgn->open(dsgn_frame)==0) 
      { dsgn_frame->SetIcon(wxIcon(_full_xpm));  dsgn_frame->Show();  return true; } // designer opened OK
    else   
#if 0  // dsgn must be removed from the list of competing frames!
      dsgn=NULL;
#else    
      if (!dsgn->GetHandle()) { delete dsgn;  dsgn=NULL; }  // assert in Destroy fails if not created
#endif
 // error exit:
 err:
  if (!dsgn)  // must prevent sending Deactivate signal to deleted dsgn!
    if (global_style==ST_MDI) frmM->dsgn = NULL;
    else                  frmP->dsgn = NULL;
  dsgn->activate_other_competing_frame(); // with MDI style the tools of the previous competing frame are not restored because the frame will not receive the Activation signal. 
  // Must be before Destroy because dsgn is sent the deactivate event.
  if (dsgn) dsgn->Destroy();  // must Destroy dsgn because it has not been opened as child of designer_ch
  if (global_style==ST_MDI) frmM->dsgn = NULL;  // prevents passing focus
  else                  frmP->dsgn = NULL;
  dsgn_frame->Destroy();
  return false;
}
#endif

