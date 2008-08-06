// datagrid.cpp
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
#include "wprez9.h"
#include "datagrid.h"
#include "querydef.h"
#include "controlpanel.h"
#include "compil.h"
#include <wx/clipbrd.h>

#include "bmps/delete_row.xpm"
#include "bmps/filter_add.xpm"
#include "bmps/filter_del.xpm"
#include "bmps/rows_del.xpm"
#include "xrc/ShowColumnDlg.cpp"
#include "fodlg.cpp"
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif 

wxDataFormat * DF_602SQLDataRow = NULL;

#define FSED_CLOB_EDITOR  // editing CLOBs in external editors makes it possible not to work with big CLOBs in the grid

#ifdef FSED_CLOB_EDITOR
class wxGridCellFSEDCLOBEditor : public wxGridCellEditor
{
public:
    wxGridCellFSEDCLOBEditor();
    virtual void Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler);
    virtual void SetSize(const wxRect& rect);
    wxString GetValue(void) const
      { return wxEmptyString; }
    virtual void PaintBackground(const wxRect& rectCell, wxGridCellAttr *attr);
    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);
    virtual void Reset();
    virtual void StartingKey(wxKeyEvent& event);
    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellFSEDCLOBEditor; }
protected:
//    wxTextCtrl *Text() const { return (wxTextCtrl *)m_control; }
    // parts of our virtual functions reused by the derived classes
    void DoBeginEdit(const wxString& startValue);
    void DoReset(const wxString& startValue);

private:
    DECLARE_NO_COPY_CLASS(wxGridCellFSEDCLOBEditor)
};
#endif

//////////////////////////////////// grid text editor not accepting Return ///////////////////////////////////////
// the editor for string/text data not being started by the return key
class wxGridCellTextNoReturnEditor : public wxGridCellTextEditor
{
 public:
  wxGridCellTextNoReturnEditor() : wxGridCellTextEditor()
    { }
  virtual bool IsAcceptedKey(wxKeyEvent& event)
    { return wxGridCellTextEditor::IsAcceptedKey(event) && event.GetKeyCode()!=WXK_RETURN && event.GetKeyCode()!=WXK_NUMPAD_ENTER; }
  virtual wxGridCellEditor *Clone() const
    { return new wxGridCellTextNoReturnEditor; }
  DECLARE_NO_COPY_CLASS(wxGridCellTextNoReturnEditor)
};
////////////////////////////////// writing the changes to the database ////////////////////////////

#include "xrc/ErrorSavingDataDlg.cpp"

bool DataGridTable::write_changes(t_on_write_error owe)
{ trecnum irec=inst->dt->edt_rec;
  bool on_fictive_record = !(inst->stat->vwHdFt & NO_VIEW_FIC) && irec!=NORECNUM && irec+1==inst->dt->fi_recnum;
  if (cache_synchro(inst))
  {// success:
    set_edited_row(NORECNUM);  // used when closing window with open cell editor
   // add a row to the grid: 
   /* write successfull, unlocked, update upper levels: */
    if (on_fictive_record)
    { if (!inst->cdp)
      { //if (!conn->SetPos_support)  -- with this condition writing to the finctive record in Oracle does not add a new row
        odbc_reset_cursor(inst);
        remap_reset(inst);
        //inst->sel_rec unchanged
       /* new record may be inserted anywhere between existing record */
       /* invalidate the new recs: new contents of the old fictive rec & new fictive rec */
      }
      else  
      { 
        dg->InsertRows(irec, 1);
      }
      inst->last_sel_rec=(trecnum)-2;
      inst->VdRecordEnter(TRUE);
    }
    return true;
  }
  else
  { if (owe!=OWE_FAIL)
    { ErrorSavingDataDlg dlg(inst->xcdp);   
      dlg.Create(dg);
      if (dlg.ShowModal()==wxID_OK)  // remove changes
      { if (owe==OWE_CONT_OR_RELOAD) 
        { cache_roll_back(inst);
          set_edited_row(NORECNUM);
          dg->Refresh();  // shows the old values
          return false;  // stops the following actions anyway (e.g. staring the external editor, setting a filter,...)
        }  
        else
        { set_edited_row(NORECNUM);
          return true;   // ignoring the error allows to continue (closing the grid)
        }  
      }
    }
   // the grid remains in the unsynchonized state, editing continues:
    return false;
  }
}
////////////////////////////////// data grid table ///////////////////////////////////////////
bool DataGridTable::IsEmptyCell( int row, int col )
{ return false; }

bool DataGridTable::AppendRows(size_t numRows)
{ 
  if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}

bool DataGridTable::DeleteRows(size_t pos, size_t numRows)
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true; 
}

void DataGridTable::SetValueAsLong( int row, int col, long value )
// Not called for NO_COMBO :-(
{ sview_dyn * sinst = inst->subinst;
  t_ctrl * itm = sinst->sitems[col].itm;
  inst->sel_rec=row;
  const wxChar * enumdef = get_enum_def(inst, itm);    // translation missing here, but the function is not used
  if (!enumdef) return;
  const wxChar * p = enumdef;
  while (value>1)
  { if (!*p) return;
    p+=wcslen(p)+1+sizeof(sig32)/sizeof(*p);
    value--;
  }
  if (!*p) return;
  p+=wcslen(p)+1;
  if (!inst->VdPutNativeVal(itm->ctId, row, 0, 0, (const char *)p))
    return;
  set_edited_row(row);
#ifdef STOP  // writing to the fictive record inserts a row, this hide the edit control, this writes again, this inserts a row again!
  if (!(sinst->inst->stat->vwHdFt & REC_SYNCHRO))
    write_changes(OWE_CONT_OR_RELOAD);
#endif
}

bool DataGridTable::InsertRows(size_t pos, size_t numRows)
{ if (GetView())
  { wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, (int)pos, (int)numRows);
    GetView()->ProcessTableMessage( msg );
  }
  return true;
}

bool DataGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{ if (typeName == wxGRID_VALUE_STRING) return true;
  if (typeName == wxGRID_VALUE_NUMBER)
  { sview_dyn * sinst = inst->subinst;
    t_ctrl * itm = sinst->sitems[col].itm;
    if (itm->ctClassNum==NO_COMBO) return true;
  }
  return false;
}

long DataGridTable::GetValueAsLong( int row, int col)
{ sview_dyn * sinst = inst->subinst;
  check_cache_top(row);
  t_ctrl * itm = sinst->sitems[col].itm;
  int binary_size;  sig32 enumval;
  if (inst->VdGetNativeVal(itm->ctId, row, 0, &binary_size, (char*)&enumval) != 1) 
    return 0;
  const wxChar * enumdef = get_enum_def(inst, itm);  // no translation here, not using the strings, converting the numeric value to the index
  if (!enumdef) return 0;
  const wxChar * p = enumdef;  int index = 1;
  while (*p)
  { p+=wcslen(p)+1;
    bool found = false;
    switch(itm->type)
    { case ATT_INT8:  case ATT_CHAR:
        if ((uns8 )enumval == *(uns8 *)p) found=true;  break;
      case ATT_INT16:
        if ((uns16)enumval == *(uns16*)p) found=true;  break;
      case ATT_INT32:
        if ((uns32)enumval == *(uns32*)p) found=true;  break;
    }
    if (found) return index;
    p+=sizeof(sig32)/sizeof(*p); 
    index++;
  }
  return 0;  // value not found in the enum
}



//////////////////////////////////////// data grid /////////////////////////////////////////

wxString DataGridTable::GetColLabelValue( int col )
{// find the header control: find PANE_PAGESTART control with the same X position 
  sview_dyn * sinst = inst->subinst;
  t_ctrl * itm = sinst->sitems[col].itm;
  char * tx  = NULL;
  t_ctrl *itm2;  int i;
  for (i = 0, itm2 = FIRST_CONTROL(inst->stat);  i < inst->stat->vwItemCount; i++, itm2 = NEXT_CONTROL(itm2))
    if (itm2->pane == PANE_PAGESTART && itm2->ctX == itm->ctX)
    { tx = get_var_value(&itm2->ctCodeSize, CT_TEXT);
      break;
    }
  if (!tx) return wxEmptyString;
  wxString capt = wxString(tx, GetConv(xcdp));
  if (dg->translate_captions)
    capt=wxGetTranslation(capt);
  return capt;
}

int DataGridTable::GetNumberRows()
{ 
  return inst->dt->fi_recnum;
}

int DataGridTable::GetNumberCols()
{ return inst->subinst->ItemCount; }

///////////////////////////////////////////////////////////////
BEGIN_EVENT_TABLE( DataGrid, wxGrid)
  EVT_SIZE(DataGrid::OnSize)
  EVT_KEY_DOWN(DataGrid::OnKeyDown)
  EVT_GRID_SELECT_CELL(DataGrid::OnSelectCell)
  EVT_GRID_RANGE_SELECT(DataGrid::OnRangeSelect)
  //EVT_MOUSE_EVENTS(DataGrid::OnMouseEvent)  -- must not use this because it disables the default processin of mouse wheel 
  EVT_MOUSEWHEEL(DataGrid::OnMouseWheel)
  EVT_GRID_LABEL_LEFT_DCLICK(DataGrid::OnLabelDblClick)
  EVT_GRID_EDITOR_HIDDEN(DataGrid::OnEditorHidden)
  EVT_MENU(-1, DataGrid::OnxCommand)
END_EVENT_TABLE()

#include "bmps/first.xpm"
#include "bmps/prevpage.xpm"
#include "bmps/prev.xpm"
#include "bmps/next.xpm"
#include "bmps/nextpage.xpm"
#include "bmps/last.xpm"

t_tool_info DataGrid::data_tool_info[DATA_TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_EXIT,    exit_xpm,     wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_FIRST,   first_xpm,    wxTRANSLATE("Go to the first record")),
  t_tool_info(TD_CPM_PREVPAGE,prevpage_xpm, wxTRANSLATE("Go to the previous page")),
  t_tool_info(TD_CPM_PREV,    prev_xpm,     wxTRANSLATE("Go to the previous record")),
  t_tool_info(TD_CPM_NEXT,    next_xpm,     wxTRANSLATE("Go to the next record")),
  t_tool_info(TD_CPM_NEXTPAGE,nextpage_xpm, wxTRANSLATE("Go to the next page")),
  t_tool_info(TD_CPM_LAST,    last_xpm,     wxTRANSLATE("Go to the last record")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_DELREC,  delete_row_xpm, wxTRANSLATE("Delete selected record(s)")),
  t_tool_info(TD_CPM_DELALL,  rows_del_xpm,   wxTRANSLATE("Delete all records")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_FILTORDER,    filter_add_xpm, wxTRANSLATE("Set filter and sort")),
  t_tool_info(TD_CPM_REMFILTORDER, filter_del_xpm, wxTRANSLATE("Remove filter and sort")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_REFRESH,      refrpanel_xpm,  wxTRANSLATE("Refresh data")),
  t_tool_info(TD_CPM_DATAPRIV,     privil_dat_xpm, wxTRANSLATE("Data privileges")),
  t_tool_info(0,  NULL, NULL)
};

wxMenu * DataGrid::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
  designer_menu = new wxMenu;
#ifdef LINUX
  AppendXpmItem(designer_menu, TD_CPM_EXIT,     _("Close\tCtrl+W"),         exit_xpm);
#else
  AppendXpmItem(designer_menu, TD_CPM_EXIT,     _("Close\tCtrl+F4"),        exit_xpm);
#endif
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_FIRST,    _("Go to the first record\tCtrl+Home"),   first_xpm);
  AppendXpmItem(designer_menu, TD_CPM_PREVPAGE, _("Go to the previous page\tPageUp"),     prevpage_xpm);
  AppendXpmItem(designer_menu, TD_CPM_PREV,     _("Go to the previous record"),           prev_xpm);
  AppendXpmItem(designer_menu, TD_CPM_NEXT,     _("Go to the next record"),               next_xpm);
  AppendXpmItem(designer_menu, TD_CPM_NEXTPAGE, _("Go to the next page\tPageDown"),       nextpage_xpm);
  AppendXpmItem(designer_menu, TD_CPM_LAST,     _("Go to the last record\tCtrl+End"),     last_xpm);
  AppendXpmItem(designer_menu, TD_CPM_SHOWCOL,  _("Go to column...\tCtrl+G"));
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_DELREC,   _("Delete selected record(s)"), delete_row_xpm);
  AppendXpmItem(designer_menu, TD_CPM_DELALL,   _("Delete all records"),      rows_del_xpm);
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_FILTORDER,_("Set filter and sort\tF3"),       filter_add_xpm);
  AppendXpmItem(designer_menu, TD_CPM_REMFILTORDER,_("Remove filter and sort\tShift+F3"), filter_del_xpm);
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_REFRESH,  _("Refresh data"),        refrpanel_xpm);
  AppendXpmItem(designer_menu, TD_CPM_DATAPRIV, _("Data privileges\tF11"), privil_dat_xpm);      
  if (!inst->cdp) designer_menu->Enable(TD_CPM_DATAPRIV, false);
  return designer_menu;
}

wxArrayInt DataGrid::MyGetSelectedRows(void) const
// Returs the list of selected rows independently of the selection method.
{ wxArrayInt selrows;
  wxArrayInt wxsel = GetSelectedRows();
  wxGridCellCoordsArray topleft = GetSelectionBlockTopLeft();
  wxGridCellCoordsArray botri   = GetSelectionBlockBottomRight();
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

void DataGrid::update_designer_menu(void)
{ 
  designer_menu->Enable(TD_CPM_DELREC, (inst->vwOptions & NO_VIEW_DELETE)==0 && inst->dt->cursnum!=NOOBJECT && MyGetSelectedRows().GetCount()>0);
  designer_menu->Enable(TD_CPM_DELALL, (inst->vwOptions & NO_VIEW_DELETE)==0 && inst->dt->cursnum!=NOOBJECT);
  designer_menu->Enable(TD_CPM_DATAPRIV, inst->cdp && (inst->info_flags & INFO_FLAG_NONUPDAT)==0);  // not exact
}

void DataGrid::update_toolbar(void) const
{ wxToolBarBase * tb;
#if WBVERS>=1000
  if (global_style==ST_MDI)
    tb=designer_toolbar;
  else
    tb = (wxToolBar *)GetParent()->FindWindow(OWN_TOOLBAR_ID);
#else
  tb=designer_toolbar;
#endif
  if (tb)
  { tb->EnableTool(TD_CPM_DELREC, (inst->vwOptions & NO_VIEW_DELETE)==0 && inst->dt->cursnum!=NOOBJECT && MyGetSelectedRows().GetCount()>0);
    tb->EnableTool(TD_CPM_DELALL, (inst->vwOptions & NO_VIEW_DELETE)==0 && inst->dt->cursnum!=NOOBJECT);
    tb->EnableTool(TD_CPM_DATAPRIV, inst->cdp && (inst->info_flags & INFO_FLAG_NONUPDAT)==0);
  }
}

DataGrid::~DataGrid(void)
{ 
  if (m_tooltip) delete m_tooltip;
  if (!child_grid) remove_tools();
  //if (inst->idadr) *inst->idadr=NULL; -- moved to OnCommand because destructor execution is delayed
  if (inst) delete inst;
  if (!child_grid && wxGetApp().frame->GetStatusBar()!=NULL) 
    wxGetApp().frame->SetStatusText(wxEmptyString, 1);  // unless termiating
}

void DataGrid::OnDisconnecting(ServerDisconnectingEvent & event)
// 1. Check if this grid should be closed 
// not so far: 2. If yes, tries to save the changes.
{ if (event.xcdp && event.xcdp!=xcdp) 
    event.ignored=true;
  //else if (!exit_request(wxGetApp().frame, true))
  //  event.cancelled=true;
}

void DataGrid::OnRecordUpdate(RecordUpdateEvent & event)
// Reload data if updated
{
  if (!event.closing_cursor)  // sent by the CLOB editor when data is saved.
    if (!in_closing_window)  // ignore RecordUpdateEvent emerged from my RecordUpdateEvent sent to the CLOB editor
      if (event.xcdp==inst->cdp && event.cursnum==inst->dt->cursnum)
      { trecnum intrec = exter2inter(inst, event.recnum);
        if (intrec < inst->dt->cache_top || intrec >= inst->dt->cache_top+inst->dt->cache_reccnt) return;
        cache_free(inst->dt,       intrec - inst->dt->cache_top, 1);
        cache_load(inst->dt, inst, intrec - inst->dt->cache_top, 1, inst->dt->cache_top);
        ForceRefresh();
      }
}

void DataGrid::Activated(void)
{
  add_tools(DATA_MENU_STRING);
}

void DataGrid::Deactivated(void)
{
}

bool close_dependent_editors(cdp_t cdp, tcursnum cursnum, bool unconditional)
// Returns false when cancelled.
{
  for (competing_frame * afrm = competing_frame_list;  afrm;  )
  { competing_frame * nx = afrm->next;  // afrm is destroyed if the editor is closed, muts copy the vale of [next]
    if (nx && nx->cf_style==ST_EDCONT)  // if the next is the editor container, it may be deleted when deleting afrm, I must skip it before this
      nx=nx->next;
    if (afrm->cf_style!=ST_EDCONT && afrm->cf_style!=ST_CHILD)
    { RecordUpdateEvent ruev(cdp, cursnum, unconditional);
      afrm->OnRecordUpdate(ruev);
      if (ruev.cancelled)
        return false;
    }
   // next and test for end:
    afrm=nx;
  }
  return true;
}

void DataGrid::OnCommand( wxCommandEvent& event )  
// This is required by competing_frame.
{
  switch (event.GetId())
  { case TD_CPM_EXIT:
    case TD_CPM_EXIT_FRAME:
     // close grid editor:
      if (IsCellEditControlEnabled()) DisableCellEditControl();
     // synchronize local data:
      if (dgt->edited_row!=NORECNUM)
        if (!dgt->write_changes(OWE_CONT_OR_IGNORE))
          break;
      goto exiting;
    case TD_CPM_EXIT_UNCOND:
     // close grid editor:
      if (IsCellEditControlEnabled()) DisableCellEditControl();
     // synchronize local data:
      if (dgt->edited_row!=NORECNUM)
        dgt->write_changes(OWE_FAIL);
     exiting:
     // close external editors:
      in_closing_window=true;
      if (!close_dependent_editors(cdp, inst->dt->cursnum, event.GetId() == TD_CPM_EXIT_UNCOND))
        { in_closing_window=false;  return; }
     // close the window:
      if (inst->idadr) *inst->idadr=NULL;
      destroy_designer(GetParent(), &datagrid_coords);
      break;
    case TD_CPM_FIRST:
      SetGridCursor(0, GetGridCursorCol());  // calls MakeCellVisible
      MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
      break;
    case TD_CPM_PREVPAGE:
      MovePageUp();
      MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
      break;
    case TD_CPM_PREV:
      MoveCursorUp(false);
      MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
      break;
    case TD_CPM_NEXT:
      MoveCursorDown(false);
      MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
      break;
    case TD_CPM_NEXTPAGE:
      MovePageDown();
      MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
      break;
    case TD_CPM_LAST:
      view_recnum(inst);
      SetGridCursor(inst->dt->fi_recnum-1, GetGridCursorCol());  // calls MakeCellVisible
      MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
      break;
    case TD_CPM_DATAPRIV:
      edit_grid_privils();
      break;
    case TD_CPM_DELREC:
      delete_selected_records();  break;
    case TD_CPM_DELALL:
      delete_all_records();  break;
    case TD_CPM_REFRESH:
      refresh(RESET_CURSOR);
      break;
    case TD_CPM_FILTORDER:
      OnFilterOrder();
      break;
    case TD_CPM_REMFILTORDER:
      OnRemFilterOrder();
      break;
    case TD_CPM_SHOWCOL:
    { ShowColumnDlg dlg(this);                                              
      dlg.Create(dialog_parent());
      if (dlg.ShowModal() == wxID_OK)
      { int sel = dlg.mShowcolName->GetSelection();
        if (sel!=-1)  // internal error
        { int column = (int)(size_t)dlg.mShowcolName->GetClientData(sel);
          SetGridCursor(GetGridCursorRow(), column);
          MakeCellVisible(GetGridCursorRow(), column);
        }
      }
      break;
    }
    default:
      event.Skip();  break;
  }
}

void DataGrid::OnxCommand( wxCommandEvent& event )
{ switch (event.GetId())
  { case TD_CPM_COPY:
    case TD_CPM_CUT:
    { wxString str = GetCellValue(GetGridCursorRow(), GetGridCursorCol());
      if (wxTheClipboard->Open())
      { wxTheClipboard->SetData(new wxTextDataObject(str));  // clipboard takes the ownership!
        wxTheClipboard->Close();
        if (event.GetId()==TD_CPM_CUT)
          SetCellValue(GetGridCursorRow(), GetGridCursorCol(), wxEmptyString);
      }
      else wxBell();
      return;
    }

    case TD_CPM_PASTE:
    { int selrows = (int)MyGetSelectedRows().GetCount();
      if (selrows>1) 
        wxBell();  // operations with multuiple rows not supported
      else // if (selrows<=1)
      { if (wxTheClipboard->Open())
        { if (wxTheClipboard->IsSupported(*DF_602SQLDataRow))
          { DataRowObject rdo;
            wxTheClipboard->GetData(rdo);
            copy_data_object_to_row(&rdo, selrows==1 ? MyGetSelectedRows()[0] : GetGridCursorRow());
            ForceRefresh();
          }  
          else if (selrows==0 && wxTheClipboard->IsSupported(wxDF_TEXT))
          { wxTextDataObject data;
            wxTheClipboard->GetData(data);
            SetCellValue(GetGridCursorRow(), GetGridCursorCol(), data.GetText());  // paste the text
          }
          else wxBell(); 
          wxTheClipboard->Close();
        }
        else wxBell();
      }
      return;
    } // paste
  }
  event.Skip();
}

void DataGrid::edit_grid_privils(void)
{ if (!inst->cdp) return;
  wxArrayInt selrows = MyGetSelectedRows();
  int fnd = 0;
  trecnum * recs = (trecnum*)sigalloc((2+(int)selrows.GetCount()) * (int)sizeof(trecnum), 77);  //  GetCount() may be 0
  if (!recs) return;
  for (int i=0;  i<selrows.GetCount();  i++)
  { trecnum selrow=selrows[i];
    trecnum irec = inter2exter(inst, selrow);
    if (irec != NORECNUM) recs[fnd++]=irec;
  }
  if (!selrows.GetCount())
  { trecnum irec = inter2exter(inst, GetGridCursorRow());
    if (irec != NORECNUM) recs[fnd++]=irec;
  }
  //error_box(_("Cannot edit privileges for the fictive record"));
  recs[fnd]=NORECNUM;
  Edit_privileges(inst->cdp, NULL, &inst->dt->cursnum, fnd ? recs : NULL, fnd>1 ? MULTIREC : 0);
  corefree(recs);
}

void DataGrid::delete_selected_records(void)
{ if ((inst->stat->vwHdFt & FORM_OPERATION)!=0 ||
      (inst->vwOptions & NO_VIEW_DELETE)!=0 ||
      inst->dt->cursnum==NOOBJECT ||
      inst->dt->co_owned_td && !(inst->dt->co_owned_td->updatable & QE_IS_DELETABLE))  // MS SQL
    { error_box(_("Cannot delete records in this form"));  return; }
  wxArrayInt selrows = MyGetSelectedRows();
  if (selrows.GetCount()==0)
    { error_box(_("No records selected"));  return; }
 // convert to external record numbers:
  int i, fnd = 0;
  trecnum * recs = (trecnum*)sigalloc((int)selrows.GetCount() * (int)sizeof(trecnum), 77);  //  GetCount() may be 0
  if (!recs) return;
  for (i=0;  i<selrows.GetCount();  i++)
  { trecnum selrow=selrows[i];
    trecnum irec = inter2exter(inst, selrow);
    if (irec != NORECNUM) recs[fnd++]=irec;
  }
  if (!fnd)
    { error_box(_("The record is fictitious"));  return; }
  if (inst->dt->conn && fnd>1)
    { error_box(_("Deleting multiple records from an ODBC data source is not supported."));  return; }
 // warning:
  wxString msg;
  if (fnd==1)
    msg.Printf(_("Do you really want to delete the selected record?"));
  else
    msg.Printf(_("Do you really want to delete %u records?"), fnd);
  if (!yesno_box(msg, dialog_parent()))
    return;
 // deleting:
  { int deleted = 0;
    for (i=0;  i<fnd;  i++)
    { if (inst->dt->conn!=NULL)
      { if (inst->dt->conn->SetPos_support)   // to muze byt problem, cisla se behem ruseni meni!
          { if (setpos_delete_record(cdp, inst->dt, recs[i])) break; }
        else
          { if (delete_odbc_record(inst, recs[i], TRUE)) break; }
      }
      else  /* WB record */
        if (cd_Delete(cdp, inst->dt->cursnum, recs[i]))
          { cd_Signalize(cdp);  break; }
      deleted++;
    }
    refresh(inst->cdp ? RESET_REMAP : RESET_CURSOR);
    ClearSelection();
  }
}

void DataGrid::delete_all_records(void)
{
  if (inst->vwOptions & (NO_VIEW_DELETE)) return;
  if (inst->dt->cursnum==NOOBJECT) return;
 // warning:
  if (!yesno_box(_("Do you want to delete ALL records?"), dialog_parent()))
    return;
  int res;
  if (inst->dt->conn!=NULL)  /* must delete only records selected by QBE */
  { { wxBusyCursor wait;
      res=odbc_delete_all9(inst->dt->conn, inst->dt->conn->hStmt, inst->dt->select_st);
    }  
    if (res==1) error_box(_("Cannot delete records in this form"));
  }
  else
  { { wxBusyCursor wait;
      res=cd_Delete_all_records(cdp, inst->dt->cursnum);
    }  
    if (res)
      { res=1;  cd_Signalize(cdp); }
    else res=2;
  }  
  if (res==2) /* deleted */
  { inst->sel_rec=inst->toprec=inst->dt->cache_top=0;
    refresh(inst->dt->conn!=NULL ?  RESET_CURSOR : RESET_REMAP);
  }
}

//void DataGrid::OnMouseEvent(wxMouseEvent & event)     // does not work :-(
//{
//  if (event.Dragging() && event.RightDown())
//    return;  // disable strange selection with right mouse button (start corner not selected)
//  event.Skip();
//}

void DataGrid::OnMouseWheel(wxMouseEvent & event)
// wxGrid does not have mouse wheel processing by default
{ int delta = event.GetWheelDelta();
  int start_x_su, start_y_su, lines;
  GetViewStart(&start_x_su, &start_y_su);
  cumulative_wheel_delta += event.GetWheelRotation();
  int lines_unit = /*event.IsPageScroll() ? win_ylen :*/ event.GetLinesPerAction();
  if (cumulative_wheel_delta>0)
  { lines = lines_unit * (cumulative_wheel_delta/delta);
    cumulative_wheel_delta %= delta;
    Scroll(-1, start_y_su>lines ? start_y_su-lines : 0);
  }
  else
  { lines = lines_unit * ((-cumulative_wheel_delta)/delta);
    cumulative_wheel_delta = -((-cumulative_wheel_delta) % delta);
    Scroll(-1, start_y_su+lines);
  }
}

void DataGrid::OnEditorHidden(wxGridEvent & event)
{
  event.Skip();
}  

void DataGrid::StartExternalEditor(int Row, int Col)
{
 // find column:
  if (Col==-1) return;  // left margin or corner double clicked
  t_ctrl *itm = sinst->sitems[Col].itm;  
  if (itm->type!=ATT_TEXT) // || (itm->access != ACCESS_CACHE && itm->access != ACCESS_DB))
    { wxBell();  return; }
 // synchronize:
  //if (IsCellEditControlEnabled())  -- this would close itself!
  //  DisableCellEditControl();
  if (!dgt->write_changes(OWE_CONT_OR_RELOAD))  
    return;
  synchronize_rec_count();
  dgt->set_edited_row(NORECNUM);
 // fail on fictive record:
  trecnum irec = Row;
  trecnum erec = inter2exter(inst, irec);
  if (erec == NORECNUM)
    { wxBell();  error_box(_("Cannot edit text in the fictive record."), dialog_parent());  return; }
 // find column number:
  int i=run_code(CT_GET_ACCESS, inst, itm, irec);
  if (i!=EXECUTED_OK) 
    { wxBell();  return; }
  curpos * wbacc = &cdp->RV.stt->wbacc;
 // start the editor:
  open_text_object_editor(cdp, inst->dt->cursnum, erec, wbacc->attr, (tcateg)-1, NULLSTRING, 0);  // not using the container is better
}

void DataGrid::cursor_changed(void)
{
  inst->sel_rec=inst->toprec=inst->dt->cache_top=0;
  refresh(RESET_REMAP);
}

void DataGrid::OnLabelDblClick(wxGridEvent & event)
{ if (inst->dt->cursnum==NOOBJECT)
    return;
  // check if the sorting is enabled and possible:
  if (inst->stat->vwHdFt & NO_FAST_SORT)
    return;
  if (inst->xcdp->connection_type==CONN_TP_602 && !inst->dt->remap_a)  // with deleted records: cannot sort, except for system tables
    if (!SYSTEM_TABLE(inst->dt->supercurs != NOOBJECT ? inst->dt->supercurs : inst->dt->cursnum))
      { wxBell();  return; }
  int Col = event.GetCol();
  if (Col==-1) return;  // corner double clicked
  t_ctrl *itm = sinst->sitems[Col].itm;  
  if (IS_HEAP_TYPE(itm->type)) // || (itm->access != ACCESS_CACHE && itm->access != ACCESS_DB))
    { wxGetApp().frame->SetStatusText(_("Cannot sort using a LOB type"));  wxBell();  return; }
  int i;
  if (inst->cdp)
  { for (i=0, itm=FIRST_CONTROL(inst->stat);  i<inst->stat->vwItemCount; i++, itm=NEXT_CONTROL(itm))
    if (itm->pane==PANE_PAGESTART && itm->ctX==sinst->sitems[Col].itm->ctX)
    { if (inst->VdIsEnabled(itm->ctId, inst->toprec) == 1)
        return;
      break;
    }
  }
  tptr itext = item_text(sinst->sitems[Col].itm);
  if (itext==NULL)
    return;
 // synchronize before sorting: 
  if (!dgt->write_changes(OWE_CONT_OR_RELOAD)) return;
  tptr Attr = itext;
  CBuffer Buf;
  if (*itext == '`')
  {
      int Len = (int)strlen(Attr);
      if (!Buf.Alloc(Len))
          return;
      Len -= 2;
      memcpy(Buf, Attr + 1, Len);
      Attr = Buf;
      Attr[Len] = 0;
  }
#ifdef STOP  // this prevents sorting by expressions eveluated on the client side but it disables sorting by columns which have deuplicate names (itext is prefixed)
  const d_table *Def = cd_get_table_d(cdp, inst->dt->cursnum, IS_CURSOR_NUM(inst->dt->cursnum) ? CATEG_DIRCUR : CATEG_TABLE);
  if (!Def)
      return;
  int Cnt = Def->attrcnt;
  for (i = 1; i < Cnt; i++)
  {
      if (wb_stricmp(cdp, Def->attribs[i].name, Attr) == 0)
          break;
  }
  release_table_d(Def);
  if (i >= Cnt)
      return;
#endif

  int order_stat = 0; // 0 - podle sloupce, 1 - podle sloupce DESC, 2 - nesetrideno
  if (inst->order)
  { COrderConds Conds;
    int Err = compile_order(cdp, inst->order, &Conds);
    if (!Err)
    { CQOrder *Cond = Conds.GetFirst();
      if (wb_stricmp(cdp, Cond->Expr, itext) == 0)
      { if (Cond->Desc)
          order_stat = 2;
        else
          order_stat = 1;
      }
    }
  }
  tptr order = NULL;
  if (order_stat == 0)
  { int Len = (int)strlen(itext) + 1;
    order = sigalloc(Len, 36);
    if (!order)
      return;
    memcpy(order, itext, Len);  // copies including the terminating 0
  }
  else if (order_stat == 1)
  { int Len = (int)strlen(itext);
    order = sigalloc(Len + 6, 36);
    if (!order)
      return;
    memcpy(order, itext, Len);
    memcpy(order + Len, " DESC", 6);  // copies including the terminating 0
  }
  if (!inst->accept_query(inst->filter, order))
    corefree(order);
  cursor_changed();  
  GetGridColLabelWindow()->Refresh(true);
}

void DataGrid::OnFilterOrder()
{
  if (IsCellEditControlEnabled()) DisableCellEditControl();
  if (!dgt->write_changes(OWE_CONT_OR_RELOAD)) return;
  CFODlg Dlg(dialog_parent(), inst);
  if (Dlg.ShowModal() == wxID_OK)
  { cursor_changed();  
    GetGridColLabelWindow()->Refresh(true);
  }  
}

void DataGrid::OnRemFilterOrder()
{
  if (IsCellEditControlEnabled()) DisableCellEditControl();
  if (!dgt->write_changes(OWE_CONT_OR_RELOAD)) return;
  inst->accept_query(NULL, NULL);
  cursor_changed();  
  GetGridColLabelWindow()->Refresh(true);
}

void DataGrid::OnKeyDown(wxKeyEvent & event)
{ if (event.AltDown())
    { event.Skip();  return; }
  switch (event.GetKeyCode())
  { case 'W':  case WXK_F4:
      if (event.ControlDown())
      { wxCommandEvent cmd;  cmd.SetId(TD_CPM_EXIT);
        OnCommand(cmd);
      }
      break;
    case 'G':
      if (event.ControlDown())
      { wxCommandEvent cmd;  cmd.SetId(TD_CPM_SHOWCOL);
        OnCommand(cmd);
        return;  // must not call event.Skip() - on GTK causes re-sending the event!
      }
      break;
    case 'C':
    case 'X':
      if (event.ControlDown())
      { int selrows = (int)MyGetSelectedRows().GetCount();
        if (selrows>1) 
          wxBell();  // operations with multuiple rows not supported
        else if (selrows==1)
        { if (event.GetKeyCode()=='X') { wxBell();  break; }
          DataRowObject * rdo = new DataRowObject;
          if (rdo) 
          { if (copy_row_to_data_object(MyGetSelectedRows()[0], rdo))
              if (wxTheClipboard->Open())
              { wxTheClipboard->SetData(rdo); // This data objects are held by the clipboard, so do not delete them in the app.
                wxTheClipboard->Close();
                return;
              }
            delete rdo;
          }
        }
        else
        { wxString str = GetCellValue(GetGridCursorRow(), GetGridCursorCol());
          if (wxTheClipboard->Open())
          { wxTheClipboard->SetData(new wxTextDataObject(str));  // clipboard takes the ownership!
            wxTheClipboard->Close();
            if (event.GetKeyCode()=='X')
              SetCellValue(GetGridCursorRow(), GetGridCursorCol(), wxEmptyString);
          }
        }
        return;
      }
      break;
    case 'V':
      if (event.ControlDown())
      { wxCommandEvent event;
        event.SetId(TD_CPM_PASTE);
        OnxCommand(event);
        return;
      }
      break;

    case WXK_F1:
      wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/grid.html"));
      return;
    case WXK_F3:    
      if (event.ShiftDown())
        OnRemFilterOrder();
      else
        OnFilterOrder();
      return;
    case WXK_F11:
      edit_grid_privils();
      return;
    case WXK_F12:  // Insert current date/time
    { int col=GetGridCursorCol(), row=GetGridCursorRow();
      t_ctrl * itm = sinst->sitems[col].itm;
      uns32 val;
      if      (itm->type==ATT_DATE)      val=Today();
      else if (itm->type==ATT_TIME)      
      { val=Now();
        if (itm->specif.with_time_zone)
          val=displace_time(val, -get_time_zone_info());  // ZULU -> UTC
      }
      else if (itm->type==ATT_TIMESTAMP) 
      { val=Current_timestamp();
        if (itm->specif.with_time_zone)
          val=displace_timestamp(val, -get_time_zone_info());  // ZULU -> UTC
      }
      else break;  // other type -> Skip
      inst->sel_rec=row;
      if (!inst->VdPutNativeVal(itm->ctId, row, 0, 0, (const char*)&val))
        return;
      dgt->set_edited_row(row);
#ifdef STOP  // writing to the fictive record inserts a row, this hide the edit control, this writes again, this inserts a row again!
      if (!(sinst->inst->stat->vwHdFt & REC_SYNCHRO))
      { dgt->write_changes(OWE_CONT_OR_RELOAD);
        synchronize_rec_count();
      }
#endif
      ForceRefresh();
      return;
    }
    case WXK_HOME:
      if (event.ControlDown())
        SetGridCursor(0, GetGridCursorCol());  // calls MakeCellVisible
      else
        SetGridCursor(GetGridCursorRow(), 0);  // calls MakeCellVisible
      MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
      return;
    case WXK_END:
      if (event.ControlDown())
      { view_recnum(inst);
        synchronize_rec_count();
        // wxGetApp().Yield();  does not work, the cells are not shown when more records added
        SetGridCursor(inst->dt->fi_recnum-1, GetGridCursorCol());  // calls MakeCellVisible
      }
      else
        SetGridCursor(GetGridCursorRow(), GetNumberCols()-1);  // calls MakeCellVisible
      MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
      return;
    case WXK_DELETE:
      if (inst->vwOptions & NO_VIEW_DELETE) return;  // no action, do not put X on the record header
      if (MyGetSelectedRows().GetCount()>0)
      { delete_selected_records();
        synchronize_rec_count();
      }
      else // if (selrows.GetCount()==0)
      { int col=GetGridCursorCol(), row=GetGridCursorRow();
        if (inter2exter(inst, row) == NORECNUM)
          { wxBell();  return; }
        t_ctrl * itm = sinst->sitems[col].itm;
        if (col>=0 && itm->access==ACCESS_NO)  // expression
          { wxBell();  return; }
        dgt->SetValue(row, col, wxEmptyString);
        Refresh();  // refreshing the current cell (and influenced) would be sufficient
      }
      return;  // no skip
    case WXK_RETURN:  // changing the defaut behaviour of the grid
    case WXK_NUMPAD_ENTER:
        if ( event.ControlDown() )
            event.Skip();  // to let the edit control have the return
        else
        { dgt->conversion_error=false;
          if (IsCellEditControlEnabled())
            DisableCellEditControl();
          if (dgt->conversion_error) return;  // no skip -> no move
          if ( GetGridCursorCol() < GetNumberCols()-1 )
          { MoveCursorRight( event.ShiftDown() );
            return;
          }
          else
          { int the_row=GetGridCursorRow();  // cache_synchro() may overwrite this when resetting the cursor for ODBC
            if (dgt->write_changes(OWE_CONT_OR_RELOAD))  // necessary on the fictive record (if there is no change in the fictive record, will not insert it and will not go down -:)
            { synchronize_rec_count();
              SetGridCursor(the_row, 0);  // goes to the 1st column, and via Skip goes to the next line
            }
          }
        }
        break;
    case WXK_INSERT:
      if (IsCellEditControlEnabled())
        DisableCellEditControl();
      if (dgt->write_changes(OWE_CONT_OR_RELOAD))  // necessary on the fictive record (if there is no change in the fictive record, will not insert it and will not go down -:)
      { synchronize_rec_count();
       // go to the fictive record:
        if (!(inst->stat->vwHdFt & NO_VIEW_FIC))
        { view_recnum(inst);
          SetGridCursor(inst->dt->fi_recnum-1, GetGridCursorCol());  // calls MakeCellVisible
          MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());
        }
        else
          { wxGetApp().frame->SetStatusText(_("This form contains non-editable data"));  wxBell(); }
      }
      return;
  }
  event.Skip();
}

bool DataRowObject::prepare_buffer(size_t extent)
{
  data=corealloc(extent, 41);
  data_size = extent;
  return data!=NULL;
}

void DataRowObject::reset_ptr(void)
{ internal_offset=0; }

void * DataRowObject::get_ptr(void)
{ return (char*)data + internal_offset; }

void DataRowObject::advance_ptr(size_t offset)
{ internal_offset += offset; }

bool DataGrid::copy_row_to_data_object(trecnum rec, DataRowObject * rdo)
{ int col, ext;  size_t extent;
 // calc data size:
  extent=sizeof(uns32);  // space for the delimiter
  for (col=0;  col<inst->subinst->ItemCount;  col++)
  { ext = dgt->GetNativeData(rec, col, NULL);
    if (ext!=-1)
      extent += sizeof(uns32) + ext;
  }
 // allocate the data buffer
  if (!rdo->prepare_buffer(extent)) return false;
  rdo->reset_ptr();
 // write data:
  for (col=0;  col<inst->subinst->ItemCount;  col++)
  { uns32 * ptr = (uns32*)rdo->get_ptr();
    ext = dgt->GetNativeData(rec, col, ptr+1);
    if (ext!=-1)
    { *ptr = ext;
      rdo->advance_ptr(sizeof(uns32) + ext);
    }
  }
 // add terminator:
  *(uns32*)rdo->get_ptr() = (uns32)-1;
  rdo->advance_ptr(sizeof(uns32));
  return true;
}

bool DataGrid::copy_data_object_to_row(DataRowObject * rdo, trecnum rec)
// More data items than columns: ignored
// Less data items than columns: last columns not written
{ int col;
  rdo->reset_ptr();
  for (col=0;  col<inst->subinst->ItemCount;  col++)
  { int ext = *(uns32*)rdo->get_ptr();
    rdo->advance_ptr(sizeof(uns32));
    if (ext==-1) break;
    if (!dgt->PutNativeData(rec, col, rdo->get_ptr(), ext))
      return false;
    rdo->advance_ptr(ext);
  }
  dgt->set_edited_row(rec);
  return true;
}

void DataGrid::OnSize(wxSizeEvent & event)
{ if (focus_receiver)  // unless creating
  { int row_height = GetNumberRows() ? GetRowSize(0) : GetDefaultRowSize();
    int visible_rows = GetClientSize().GetHeight() / row_height + 1;
    cache_resize(inst, visible_rows);
    UpdateDimensions();  // without this the grid does not recognize its size changes
    // adding wxGetApp().Yield();
    // AdjustScrollbars();  does not calculate the proper scroll page size because size changes on the target window are deferred (DoSetSize vs. DoGetClientSize)
    synchronize_rec_count();  // must add rows to the grid if records added
  }
  event.Skip();
}

void DataGrid::OnRangeSelect(wxGridRangeSelectEvent & event)
{ 
  if (event.Selecting())
  { DisableCellEditControl();
   // if leaving a changed row, save or veto:
    if (dgt->edited_row!=NORECNUM)
    { if (!dgt->write_changes(OWE_CONT_OR_RELOAD))
        { event.Veto();  return; }
      dgt->set_edited_row(NORECNUM);  // changes saved
    }
   // if selecting all, find all
    if (inst->dt->remap_a && !inst->dt->rm_completed && event.GetTopRow()==0 && event.GetBottomRow()+1 == inst->dt->fi_recnum)
    { view_recnum(inst);
      synchronize_rec_count();
      SelectAll();  // recursive
      return;
    }
   // make the cell selection consistent:
    int row, col;
    col = GetGridCursorCol();
    row = GetGridCursorRow();
    if (event.GetBottomRow()!=row && event.GetTopRow()!=row)
    { if (event.GetTopRow() < inst->toprec)
        row=event.GetBottomRow();
      else
        row=event.GetTopRow();
      SetGridCursor(row, col);
      MakeCellVisible(row, col);
    }
  }
  update_toolbar();
  event.Skip();
}

void DataGrid::write_current_pos(trecnum row)
{ wxString msg;
  if (child_grid) return;
  if (inst->dt->fi_recnum!=0)  // empty info for empty grid
  { row++;
    if (row >= inst->dt->fi_recnum && !(inst->stat->vwHdFt & NO_VIEW_FIC))
      msg.Printf(_("On fictitious record (%u valid records)"), inst->dt->fi_recnum-1);
    else if (inst->dt->rm_completed)
      msg.Printf(_("On record %u of %u"), row, inst->dt->remap_a ? inst->dt->rm_int_recnum : inst->dt->rm_ext_recnum);
    else
      msg.Printf(_("On record %u of unknown"), row);
  }
  wxGetApp().frame->SetStatusText(msg, 1);
}

void DataGrid::OnSelectCell(wxGridEvent & event)
{
 // if leaving a changed row, save or veto:
  if (dgt->edited_row!=NORECNUM && dgt->edited_row!=event.GetRow())
  { if (!dgt->write_changes(OWE_CONT_OR_RELOAD))
      { event.Veto();  return; }
    dgt->set_edited_row(NORECNUM);  // changes saved
  }
 // row number info:
  if (inst->sel_rec != event.GetRow())
  {// if the fictive record entered, try to expand
    if (event.GetRow()+1 >= inst->dt->fi_recnum && !inst->dt->rm_completed && inst->dt->remap_a)
      remap_expand(inst, 512);  // adds rows to the grid
    write_current_pos(event.GetRow());
    inst->sel_rec = event.GetRow();
  }
  synchronize_rec_count();  // must add rows to the grid if records added
 // column info:
  if (event.GetCol() != current_column_selected && event.GetCol() != -1 && event.GetCol() < GetNumberCols())  // sometimes GetCol() gives non-existing column!
  { 
    char * tx=get_var_value(&sinst->sitems[event.GetCol()].itm->ctCodeSize, CT_STATUSTEXT);
    if (!child_grid) 
      if (tx && *tx!='^')
        wxGetApp().frame->SetStatusText(wxString(tx, *cdp->sys_conv));
      else
        wxGetApp().frame->SetStatusText(wxEmptyString);
    current_column_selected = event.GetCol();
  }                                                       
  SetFocus();  // important when a label is clicked
  event.Skip();
}

void DataGridTable::set_edited_row(trecnum row)
{ if ((row==NORECNUM) != (edited_row==NORECNUM))
  { edited_row=row;
    dg->ForceRefresh();
  }
  else edited_row=row;
}

wxString DataGridTable::GetRowLabelValue(int row)
{ trecnum erec=inter2exter(inst, row);
  return erec==NORECNUM ? wxT("*") : edited_row==row ? wxT("X") : wxEmptyString;
}

void DataGridTable::check_cache_top(int row)
// Synchronizes the cache contents with the scroll offset, current row number and the window size.
{ trecnum orig_fi_recnum = inst->dt->fi_recnum;  // safe in order to compare later
 // get the number of the top record (may by only partially visible):
  int start_x, start_y, xUnit, yUnit;
  dg->GetViewStart(&start_x, &start_y);
  dg->GetScrollPixelsPerUnit(&xUnit, &yUnit);
  trecnum toprec=start_y*yUnit/dg->GetDefaultRowSize();
  if (toprec>row) toprec=row;  // should be impossible
 // update the cache:
  set_top_rec(inst, toprec);
#if 0  // may cause double inserting of the fictive row: InsertRows->DisableCellEdit->Save->GetValue->check_cache_top->AppendRows
  dg->synchronize_rec_count();
#endif
 // the record counting may have been completed, must update the status information:
  if (orig_fi_recnum < inst->dt->fi_recnum)
    dg->write_current_pos(dg->GetGridCursorRow());
#if 0
  if (inst->dt->fi_recnum > dg->last_recnum)
  { dg->AppendRows(inst->dt->fi_recnum - dg->last_recnum);
    dg->last_recnum = inst->dt->fi_recnum;
  }
#endif
}

void DataGrid::synchronize_rec_count(void)
// Adds rows to the grid so that it contains all existing and fictive records in [dt].
{ if (inst->dt->fi_recnum > GetNumberRows())
    dgt->AppendRows(inst->dt->fi_recnum - GetNumberRows());
}

t_specif wx_string_spec(0, 0, 0,
#if wxUSE_UNICODE
#ifdef LINUX
  2);
#else  // MSW
  1);
#endif
#else  // 8-bit
  0);
#endif

wxString get_grid_default_string_format(int tp)
{ wxString string_format;
  switch (tp)
  { case ATT_INT8:  case ATT_INT16:  case ATT_INT32:   case ATT_INT64:
    case ATT_PTR:   case ATT_BIPTR:  case ATT_VARLEN:  case ATT_INDNUM:
    case ATT_MONEY:
      string_format=format_decsep;  break;
    case ATT_FLOAT:
    { int decim, form;
      get_real_format_info(format_real, &decim, &form);
      if (form==2)
        string_format.Printf(wxT("%c %uF"), format_decsep.GetChar(0), decim);  
      else
        string_format.Printf(wxT("%c%u%c"), format_decsep.GetChar(0), decim, form==0 ? 'F' : 'E');  
      break;
    }
    case ATT_DATE:
      string_format=format_date;  break;
    case ATT_TIME:
      string_format=format_time;  break;
    case ATT_TIMESTAMP:  case ATT_DATIM:
      string_format=format_timestamp;  break;
    case ATT_BOOLEAN:
      string_format=wxT("false,true");  break;   // used when passing parametres to procedure, DAD, query
  }  // other types have string_format=="" - the default format
  return string_format;
}

wxString get_grid_default_string_format_checkbool(int tp)
{ 
  return tp==ATT_BOOLEAN ? wxT("0,1") : get_grid_default_string_format(tp);  // used by the grid check box
}

void log_conv_error(int err)
{ const wxChar * msg = NULL;
  switch (err)
  { case SC_INCONVERTIBLE_WIDE_CHARACTER:
      msg=_("Unconvertible wide character");  break;
    case SC_INCONVERTIBLE_INPUT_STRING:
      msg=_("Unconvertible input string");  break;
    case SC_INPUT_STRING_TOO_LONG:
      msg=_("Input string is too long");  break;
    case SC_VALUE_OUT_OF_RANGE:
      msg=_("Value out of range");  break;
    case SC_BAD_TYPE:
      msg=_("Bad type");  break;
    case SC_BINARY_VALUE_TOO_LONG:
      msg=_("Binary value too long");  break;
  }
  if (msg) 
  { log_client_error((wxString(_("Conversion error: "))+wxString(msg)).mb_str(*wxConvCurrent));
    wxGetApp().frame->SetStatusText(msg, 0);
    wxBell();
  }  
}

wxString DataGridTable::GetValue( int row, int col )
{ sview_dyn * sinst = inst->subinst;
  check_cache_top(row);
  t_ctrl * itm = sinst->sitems[col].itm;
  const wxChar * enumdef = get_enum_def(inst, itm);
  if (itm->ctClassNum==NO_COMBO && enumdef)  // called of BeginEdit by wxGridCellChoiceEditor, written on ESC
  { long index = GetValueAsLong(row, col);
    if (!index)  // value not found
      return wxEmptyString;
    const wxChar *p = enumdef;
    while (*p && index>1)
      { p+=wcslen(p)+1+sizeof(sig32)/sizeof(*p);  index--; }
    return dg->translate_captions ? wxGetTranslation(p) : p;
  }
  if (itm->ctClassNum==NO_CHECK)
  { int binary_size;  wbbool boolval;
    if (inst->VdGetNativeVal(itm->ctId, row, 0, &binary_size, (char*)&boolval) != 1) 
      return wxEmptyString;
    return boolval==wbtrue ? wxT("1") : boolval==wbfalse ? wxT("0") : wxEmptyString;
  }
  if (itm->type==0) return wxT("<NULL>");
  if (itm->type==ATT_RASTER) return row >= inst->dt->rm_int_recnum ? wxEmptyString : wxT("<Picture>");
  if (itm->type==ATT_NOSPEC) return row >= inst->dt->rm_int_recnum ? wxEmptyString : wxT("<BLOB>");
  if (itm->type==ATT_SIGNAT) return row >= inst->dt->rm_int_recnum ? wxEmptyString : wxT("<Signature>");
  if (itm->type==ATT_HIST)   return row >= inst->dt->rm_int_recnum ? wxEmptyString : wxT("<History>");  // in tables from version < 9.0
 // read and convert the database value:
  wxString res;
  int err=0, length;  char native_buf[MAX_FIXED_STRING_LENGTH+2];  
  if (itm->type==ATT_TEXT || itm->type==ATT_STRING || itm->type==ATT_CHAR)  // the editor needs the full length event for text
  { char * pnative = native_buf;  bool allocated = false;
    if (itm->type==ATT_TEXT)
    { if (inst->VdGetNativeVal(itm->ctId, row, 0, &length, NULL) != TRUE) return wxEmptyString;
      if (length==0) return wxEmptyString;
#ifdef FSED_CLOB_EDITOR
      if (length>MAX_FIXED_STRING_LENGTH)
        if (inst->xcdp->connection_type==CONN_TP_602)
          length=MAX_FIXED_STRING_LENGTH;
#endif
      if (length>MAX_FIXED_STRING_LENGTH)
      { pnative=new char[length+2];
        if (!pnative) return wxEmptyString;
        allocated = true;
      }
    }
   // read the native string value:
    if (inst->VdGetNativeVal(itm->ctId, row, 0, &length, pnative) != TRUE) return wxEmptyString;
    if (itm->type==ATT_TEXT) // not terminated in the native form
    { if (itm->specif.wide_char) length &= 0x7ffffffe;  // sometimes the length is odd, but the terminating zero must be on an even address
      pnative[length]=pnative[length+1]=0;
#ifdef FSED_CLOB_EDITOR
      if (inst->xcdp->connection_type==CONN_TP_602)
      { int i;  // replace CR and LF by space, multiline display in the grid is bad 
        if (itm->specif.wide_char)
        { wuchar * wnat = (wuchar*)pnative;
          for (i=0;  i<length/2;  i++)
            if (wnat[i]=='\r' || wnat[i]=='\n') wnat[i]=' ';
        }
        else
          for (i=0;  i<length;  i++)
            if (pnative[i]=='\r' || pnative[i]=='\n') pnative[i]=' ';
      }
#endif
    }  
   // compute the output length:
    if (itm->specif.wide_char) length/=2;
#if wxUSE_UNICODE
    length*=sizeof(wxChar);
#endif
   // convert:
    //wxLogDebug(wxString(pnative, *wxConvCurrent));
    res.GetWriteBuf(length+1);  // length must not be 0!
    err=superconv(itm->type, ATT_STRING, pnative, (char*)res.c_str(), itm->specif, wx_string_spec, NULL);
    res.UngetWriteBuf();
    //wxLogDebug(res);
    if (allocated) delete [] pnative;
  }
  else 
  { if (inst->VdGetNativeVal(itm->ctId, row, 0, &length, native_buf) != TRUE) return wxEmptyString;
    wxString string_format = get_grid_default_string_format_checkbool(itm->type);
    if (itm->type==ATT_BINARY) length=2*length;  else length=MAX_FIXED_STRING_LENGTH; // too much
    res.GetWriteBuf(length);  // length must not be 0!
    err=superconv(itm->type, ATT_STRING, native_buf, (char*)res.c_str(), itm->specif, wx_string_spec, string_format.mb_str(*wxConvCurrent));
    res.UngetWriteBuf();
  }
  if (err<0) 
  { log_conv_error(err);
    return wxT("<Err>"); 
  }
  return res;
}

int DataGridTable::GetNativeData(int row, int col, void * buf)
// Copies data in the native format to buf, unless buf==NULL.
// Returns the data size, -1 on error.
{ sview_dyn * sinst = inst->subinst;
  check_cache_top(row);
  t_ctrl * itm = sinst->sitems[col].itm;
 // find the length (reading the LOB value needs it): 
  int length;
  if (!IS_HEAP_TYPE(itm->type))
  { length = type_specif_size(itm->type, itm->specif);
    if (itm->type==ATT_STRING) length+=2;  // wide terminator
  }
  else
    if (inst->VdGetNativeVal(itm->ctId, row, 0, &length, NULL) != TRUE) return -1;
 // get the value:
  if (buf)
    if (inst->VdGetNativeVal(itm->ctId, row, 0, &length, (char*)buf) != TRUE) return -1;
  return length;
}

bool DataGridTable::PutNativeData(int row, int col, void * buf, int length)
// Copies data in the native format from buf
// Returns false on error.
{ sview_dyn * sinst = inst->subinst;
  check_cache_top(row);
  t_ctrl * itm = sinst->sitems[col].itm;
  return inst->VdPutNativeVal(itm->ctId, row, 0, length, (const char *)buf)!=0;
}

const void * DataGrid::enum_reverse(const wxChar * enumdef, const wxChar * str)
{ 
  const wxChar *p = enumdef;
  if (translate_captions) 
    while (*p && wxString(str).Cmp(wxGetTranslation(wxString(p))))
      p+=wcslen(p)+1+sizeof(sig32)/sizeof(*p);
  else
  { //wxCharBuffer buf8 = wxString(str).mb_str(*cdp->sys_conv);
    while (*p && wxString(str).Cmp(wxString(p)))
    //while (*p && strcmp(p, buf8))
      p+=wcslen(p)+1+sizeof(sig32)/sizeof(*p);
  }
  if (!*p) return NULL;
  return p+wcslen(p)+1;
}

void DataGridTable::SetValue( int row, int col, const wxString& value )
{ if (dg->inst->dt->co_owned_td && !(inst->dt->co_owned_td->updatable & QE_IS_UPDATABLE))
    return;  // can be called by the read-only multiline text grid editor, cut causes access violation in MySQl ODBC driver in SQLSetPos
  sview_dyn * sinst = inst->subinst;
  t_ctrl * itm = sinst->sitems[col].itm;
  inst->sel_rec=row;
  const wxChar * enumdef = get_enum_def(inst, itm);
  if (itm->ctClassNum==NO_COMBO && enumdef)
  { const void * val = dg->enum_reverse(enumdef, value);
    if (!val) return;
    if (!inst->VdPutNativeVal(itm->ctId, row, 0, 0, (const char*)val))
      return;
  }
  else if (itm->type==ATT_TEXT || itm->type==ATT_STRING || itm->type==ATT_CHAR)
  { int len=(int)value.Length();
    if (itm->specif.wide_char)
      len*=2;
    char native_buf[MAX_FIXED_STRING_LENGTH+2];
    char * pnative = native_buf;  bool allocated = false;
    if (len>MAX_FIXED_STRING_LENGTH)
    { pnative=new char[len+2];
      if (!pnative) return;
      allocated = true;
    }
    if (itm->type==ATT_TEXT) itm->specif.length=0;  // prevents truncating in the superconv
    int err=superconv(ATT_STRING, itm->type, (char*)value.c_str(), pnative, wx_string_spec, itm->specif, NULL);
    if (err>=0) 
    { inst->VdPutNativeVal(itm->ctId, row, 0, len, pnative);
      if (itm->type==ATT_TEXT)
	    inst->VdPutNativeVal(itm->ctId, row, 0, len, NULL); // setting the length
    }
    if (allocated) delete [] pnative;
  }
  else
  { char native_buf[MAX_FIXED_STRING_LENGTH+1];
#if 0
    if (itm->ctClassNum==NO_CHECK)
      strcpy(native_buf, value[0]=='1' ? "+" : value[0]=='0' ? "-" : "");
#endif
    { int err;
      wxString string_format = get_grid_default_string_format_checkbool(itm->type);
      err=superconv(ATT_STRING, itm->type, value.c_str(), native_buf, wx_string_spec, itm->specif, string_format.mb_str(*wxConvCurrent));
      if (err<0) 
        { log_conv_error(err);  conversion_error=true;  return; }
      if (itm->type==ATT_BINARY) // add nulls
        for (int i=err;  i<itm->specif.length;  i++)
          native_buf[i]=0;
      if (!inst->VdPutNativeVal(itm->ctId, row, 0, 0, native_buf))
        return;
    }
  }
  set_edited_row(row);
#ifdef STOP  // writing to the fictive record inserts a row, this hide the edit control, this writes again, this inserts a row again!
  if (!(sinst->inst->stat->vwHdFt & REC_SYNCHRO))
  { write_changes(OWE_CONT_OR_RELOAD);
    dg->synchronize_rec_count();
  }
#endif
}


int DataGrid::create_column_description()
{ sview_item * sitems;  int i, cnt;  t_ctrl * itm;
  if (inst->subinst) return 0;  // used by TraceDataGrid
  sinst=(sview_dyn *)sigalloc(sizeof(sview_dyn), 54);
  if (!sinst) return -1;  /* cannot open */
  sinst->inst=inst;  inst->subinst=sinst;
  sinst->sel_col=sinst->first_col=0;
  sinst->editmode=wbfalse;  sinst->editbuf=NULL;
  sinst->selection_on=TRUE;
  sinst->ItemCount=0;
  if (inst->stat->vwItemCount)
  { sitems=sinst->sitems=(sview_item *)sigalloc(
      sizeof(sview_item)*inst->stat->vwItemCount, 54);
    if (!sitems) return -1; /* sinst deallocated by  delete inst outside */
  }
  else sitems=sinst->sitems=NULL;  /* error later */
#if 0
  if (inst->hParent && IsSView(inst->hParent) &&
      (inst->query_mode != NO_QUERY)) /* copy the column design */
  { sview_dyn * par_sinst = (sview_dyn *)((view_dyn *)
                              GetWindowLong(inst->hParent, 0))->subinst;
    cnt=par_sinst->ItemCount;
   /* inherit the column order: */
    memcpy(sitems, par_sinst->sitems, sizeof(sview_item) * cnt);
    for (i=0; i<cnt; i++)
      sitems[i].itm=get_control_def(inst, par_sinst->sitems[i].itm->ctId);
  }
  else
#endif  
  { cnt=0;
    for (i=0, itm=FIRST_CONTROL(inst->stat);  i<inst->stat->vwItemCount;
         i++, itm=NEXT_CONTROL(itm))  /* copying controls */
     if (itm->pane==PANE_DATA)
      if (itm->ctClassNum==NO_EDI || itm->ctClassNum==NO_STR ||
          itm->ctClassNum==NO_VAL || itm->ctClassNum==NO_TEXT ||
          is_check(itm) && ((itm->type==ATT_BOOLEAN) || !itm->type) || /* zero type in the designer simple view */
          itm->ctClassNum==NO_BUTT ||
          itm->ctClassNum==NO_COMBO|| itm->ctClassNum==NO_EC ||
          itm->ctClassNum==NO_OLE  || itm->ctClassNum==NO_RASTER)
      { sitems[cnt].col_start=
           cnt ? sitems[cnt-1].col_start+sitems[cnt-1].col_size : 0;
        sitems[cnt].col_size=itm->ctCX;
        sitems[cnt].itm=itm;
        cnt++;
      }
  }
  sinst->ItemCount=cnt;
  if (!cnt) return -1;
 /* all data have been calculated */
  inst->last_sel_rec=(trecnum)-2;
  inst->VdRecordEnter(FALSE);
  return 0;
}

class MyGridCellEnumRenderer : public wxGridCellEnumRenderer
{public:
  void add_string(wxString str)
    { m_choices.Add(str); }
  MyGridCellEnumRenderer(void)
    { }
};

static int font_space(wxWindow * win, wxFont font)
{ wxClientDC dc(win);
  dc.SetFont(font);
  wxCoord w, h, descent, externalLeading;
  dc.GetTextExtent(wxT("pQgY"), &w, &h, &descent, &externalLeading);
#ifdef WINS  
  return (h+descent+externalLeading) * 5 / 4;
#else  
  return (h+descent+externalLeading) + 3;
#endif  
}

#define COLUMN_LABEL_MIN_MARGIN 5

int DataGrid::get_column_size(int col, t_ctrl * itm)
// Find the column size defined as the size specified for the column, but not smaller than the static column label (if any)
// This is used by grids displaying data from system inforaton views, like the list of threads.
{ 
  sview_dyn * sinst = inst->subinst;
  int size = sinst->sitems[col].col_size;
  t_ctrl *itm2;  int i;
  for (i = 0, itm2 = FIRST_CONTROL(inst->stat);  i < inst->stat->vwItemCount;  i++, itm2 = NEXT_CONTROL(itm2))
    if (itm2->pane == PANE_PAGESTART)
      if (itm2->ctX == itm->ctX)
      { const char * tx = get_var_value(&itm2->ctCodeSize, CT_TEXT);
        if (tx)
        { 
          wxString capt = wxString(tx, GetConv(cdp));
          if (translate_captions)
            capt=wxGetTranslation(capt);
          wxClientDC dc(this);  wxCoord w, h;
          dc.SetFont(*grid_label_font);
          dc.GetTextExtent(capt, &w, &h);
          w+=2*COLUMN_LABEL_MIN_MARGIN;  // margin
          if (w>size) size=w;
        }
        break;
      }
  return size;
}

//bool DataGrid::Create(wxWindow * parent, wxWindowID id, xcdp_t xcdpIn, view_dyn * instIn, bool in_pane)
//{ inst=NULL;  // used by TraceDataGrid in OnSize
//  return true;
//}

bool DataGrid::open(wxWindow * parent, t_global_style my_style)
{
  if (my_style==ST_MDI)
  { parent->SetBackgroundColour(WindowColour);
    parent->ClearBackground();
  }
  wxGrid::Create(parent, COMP_FRAME_ID, wxDefaultPosition, wxDefaultSize, 
    child_grid ? wxSUNKEN_BORDER | wxVSCROLL | wxFULL_REPAINT_ON_RESIZE : wxSUNKEN_BORDER | wxVSCROLL);
  competing_frame::focus_receiver = this;
  current_column_selected=-1;
  if (inst->dt->co_owned_td && !(inst->dt->co_owned_td->updatable & QE_IS_DELETABLE))
    inst->stat->vwHdFt |= NO_VIEW_FIC;
 // toolbar:
  create_column_description();
  dgt = create_grid_table(xcdp, inst);
  dgt->prepare();
  SetTable(dgt, TRUE);
  last_recnum=inst->dt->fi_recnum;
  SetLabelFont(*grid_label_font);
  SetDefaultCellFont(*grid_cell_font);
  inst->hWnd=this;
 // define layout:
  EnableDragRowSize(false);
  SetColLabelSize(font_space(this, *grid_label_font));
  SetDefaultRowSize(font_space(this, *grid_cell_font));
  SetRowLabelSize(12);  
  SetSelectionMode(wxGrid::wxGridSelectRows);
  sview_dyn * sinst = inst->subinst;
  bool data_source_read_only = inst->dt->co_owned_td && (!inst->dt->co_owned_td->updatable & QE_IS_UPDATABLE);
  for (int col = 0;  col < sinst->ItemCount;  col++)
  { t_ctrl * itm = sinst->sitems[col].itm;
    atr_info * catr = inst->dt->attrs+itm->column_number;  // may be invalid when column_number==-1
    SetColSize(col, get_column_size(col, itm));
    wxGridCellAttr * col_attr = new wxGridCellAttr;
    if (itm->ctClassNum==NO_VAL || data_source_read_only || itm->column_number==-1 || (catr->flags & CA_NOEDIT))
      if (itm->type!=ATT_TEXT)  // for text implemented inside the control
        col_attr->SetReadOnly();
    const wxChar * enumdef = get_enum_def(inst, itm);
    if (enumdef && (itm->ctClassNum==NO_COMBO || itm->ctClassNum==NO_EC))  // define the editor
    { int count = 0;  const wxChar * p;
      for (p=enumdef;  *p;  p+=wcslen(p)+1+sizeof(sig32)/sizeof(*p)) count++;
      wxString * keys = new wxString[count];
      count=0;
      for (p=enumdef;  *p;  p+=wcslen(p)+1+sizeof(sig32)/sizeof(*p))
        keys[count++] = translate_captions ? wxGetTranslation(p) : p;
      wxGridCellEditor * combo_editor = new wxGridCellChoiceEditorNoKeys(count, keys, itm->ctClassNum==NO_EC);
      col_attr->SetEditor(combo_editor);
      delete [] keys;
     // define the renderer
      if (itm->ctClassNum==NO_COMBO)  
      { MyGridCellEnumRenderer * combo_renderer = new MyGridCellEnumRenderer();
        combo_renderer->add_string(wxEmptyString);
        for (p=enumdef;  *p;  p+=wcslen(p)+1+sizeof(sig32)/sizeof(*p))
          combo_renderer->add_string(translate_captions ? wxGetTranslation(p) : p);
        col_attr->SetRenderer(combo_renderer);  // takes the ownership
      }
    }
    else if (itm->ctClassNum==NO_CHECK)
    { wxGridCellBool3Editor * bool_editor = new wxGridCellBool3Editor();
      col_attr->SetEditor(bool_editor);
      wxGridCellBool3Renderer * bool_renderer = new wxGridCellBool3Renderer();
      col_attr->SetRenderer(bool_renderer);  // takes the ownership
    }
    else if (itm->type==ATT_RASTER || itm->type==ATT_NOSPEC || itm->type==ATT_SIGNAT || itm->type==ATT_HIST)
      col_attr->SetReadOnly();
    else if (itm->type==ATT_TEXT)
    { wxGridCellEditor * mltext_editor;
#ifdef FSED_CLOB_EDITOR
      if (inst->xcdp->connection_type==CONN_TP_602)
        mltext_editor = new wxGridCellFSEDCLOBEditor();
      else
#endif
        mltext_editor = new wxGridCellMLTextEditor(data_source_read_only);
      col_attr->SetEditor(mltext_editor);
    }
    else if (itm->type==ATT_STRING || itm->type==ATT_BINARY || itm->type==ATT_CHAR)  // limited size
    { wxGridCellTextNoReturnEditor * text_editor = new wxGridCellTextNoReturnEditor();
      wxString maxlen;
      maxlen.Printf(wxT("%d"), itm->type==ATT_STRING ? itm->specif.length : itm->type==ATT_CHAR ? 1 : 2*itm->specif.length);
      text_editor->SetParameters(maxlen);
      col_attr->SetEditor(text_editor);
    }
    else // can use special editors for numbers
    { wxGridCellTextNoReturnEditor * text_editor = new wxGridCellTextNoReturnEditor();
      col_attr->SetEditor(text_editor);
    }
   // temporary: used by system table views:
    if (get_var_value(&itm->ctCodeSize, CT_ENABLED))  // .. and used for disabling editation of the not-editable texts
      col_attr->SetReadOnly();
    SetColAttr(col, col_attr);
  }
  ColLabel.m_Grid  = this;
  ColLabel.m_Label = GetGridColLabelWindow();
  ColLabel.m_Label->PushEventHandler(&ColLabel);
  set_top_rec(inst, 0);
  wxGridEvent event(0, wxEVT_GRID_SELECT_CELL, NULL, 0, 0);
  OnSelectCell(event);
  if (!child_grid) add_tools(DATA_MENU_STRING);  // necessary for the MDI style
  return true;
}

void DataGrid::set_designer_caption(void)
{ 
  const char * capt = get_var_value(&inst->stat->vwCodeSize, VW_CAPTION);
  wxString caption(capt, GetConv(xcdp));
  if (translate_captions)
    caption = wxGetTranslation(caption);
  set_caption(caption);
}

wxWindow * DataGrid::open_form(wxWindow * parent, xcdp_t xcdpIn, const char * source, tcursnum curs, int flags)
// Deletes the grid on error. Closes [curs] on error iff (flags & AUTO_CURSOR).
// Error in WX: cannot delete a grid without creating it before!!
{ wxWindow * ret;
  inst = new basic_vd(xcdp);
  if (!inst) { Create(wxGetApp().frame, wxID_ANY);  delete this;  return NULL; }
  wxBusyCursor wait;
  if (!inst->open_on_desk(source, curs, flags, 0, NULL, 0, NULL, NULL))
    { Create(wxGetApp().frame, wxID_ANY);  delete this;  return NULL; }  // inst is owned by this
  if (inst->dt->cdp && SYSTEM_TABLE(inst->dt->cursnum))
    translate_captions=true;
  if (parent && (flags & CHILD_VIEW)!=0)
  { child_grid=true;  cf_style=ST_CHILD;
    if (!open(parent, ST_CHILD))
    { if (focus_receiver) Destroy();  delete this;  // if created, use Destroy(), delete otherwise
      if ((flags & AUTO_CURSOR)!=0 && curs!=NOOBJECT) 
        cd_Close_cursor(xcdp->get_cdp(), curs);
      return NULL;
    }
    ret=this;
  }
  else
  { child_grid=false;
    ret=open_standard_frame(this);
    if (!ret) return NULL;
  }
  AdjustScrollbars();  // this calculates the proper scroll page size for popup window, but does not help for MDI or client grid
 // from now grid owns inst and ret owns grid or ret==grid
  m_tooltip = new DataGridToolTip(this, GetGridWindow()); 
  return ret;
}


#if 0
wxWindow * DataGrid::open_form(wxWindow * parent, xcdp_t xcdp, const char * source, tcursnum curs, int flags)
{ basic_vd * inst;  wxWindow * ret;
  inst = new basic_vd(xcdp);
  if (!inst) return NULL;
  wxBusyCursor wait;
  if (!inst->open_on_desk(source, curs, flags, 0, NULL, 0, NULL, NULL))
    { delete inst;  return NULL; }
  if (inst->dt->cdp && SYSTEM_TABLE(inst->dt->cursnum))
    translate_captions=true;
  if (parent && (flags & CHILD_VIEW)!=0)
  { child_grid=true;
    Create(parent, 1, xcdp, inst, NULL, true);
    ret=this;
  }
  else
  { child_grid=false;
    const char * capt = get_var_value(&inst->stat->vwCodeSize, VW_CAPTION);
    wxString scapt(capt, GetConv(xcdp));
    if (translate_captions)
      scapt = wxGetTranslation(scapt);
    if (global_style==ST_MDI && !parent)
    { DataFrameMDI * frm = new DataFrameMDI(wxGetApp().frame, -1, scapt);
      if (!frm) { delete inst;  return NULL; }
      frm->SetBackgroundColour(WindowColour);
      frm->ClearBackground();
      Create(frm, 1, xcdp, inst, frm, false);
      frm->grid=this;
      frm->SetIcon(wxIcon(_table_xpm));
      frm->Show();
      SetFocus();  // must not focus child grids
      ret = frm;
    }
    else
    { if (!parent) parent=wxGetApp().frame;
      DataFramePopup * frm = new DataFramePopup(parent, -1, scapt, datagrid_coords.get_pos(), datagrid_coords.get_size());
      if (!frm) { delete inst;  return NULL; }
      Create(frm, 1, xcdp, inst, frm, false);
      frm->grid=this;
      frm->SetIcon(wxIcon(_table_xpm));
      //frm->PostCreation();
      frm->Show();
      SetFocus();
      ret = frm;
    }
    put_designer_into_frame(ret, this);
  }
  AdjustScrollbars();  // this calculates the proper scroll page size for popup window, but does not help for MDI or client grid
 // from now grid owns inst and ret owns grid or ret==grid
  m_tooltip = new DataGridToolTip(this, GetGridWindow()); 
  return ret;
}
#endif

wxString DataGridToolTip::GetText(const wxPoint& pos, wxRect& boundary)
{ boundary = wxRect(pos.x - 4, pos.y - 4, 8, 8);
  return grid->GetTooltipText(); 
}

wxString DataGrid::GetTooltipText(void)
{ int xpos, ypos;
 // test focus: without this crashes when tooltip opens at the same time when error window opens
  wxWindow * focus_window = wxWindow::FindFocus();
  if (!focus_window || focus_window != GetGridWindow() && focus_window->GetParent() != this) // any grid component may have focus
    return wxEmptyString;
 // find column of the grid below mouse cursor:
  wxPoint pt = GetGridWindow()->ScreenToClient(wxGetMousePosition());
  xpos = pt.x;
  ypos = pt.y;
  CalcUnscrolledPosition(xpos, ypos, &xpos, &ypos);
  int col = XToCol(xpos);
  trecnum irec = ypos / GetDefaultRowSize();
  if (col==wxNOT_FOUND || irec>=inst->dt->fi_recnum) return wxEmptyString;
 // get text defined for the column:
  char * tx=get_var_value(&sinst->sitems[col].itm->ctCodeSize, CT_STATUSTEXT);
  if (tx) 
    if (*tx=='^') return wxString(tx+1, *cdp->sys_conv);
  return wxEmptyString;
}

DataGridToolTip::DataGridToolTip(DataGrid * gridIn, wxWindow * w) : wxAdvToolTipText(w), grid(gridIn) 
{}


wxWindow * DataGrid::open_data_grid(wxWindow * parent, xcdp_t xcdp, tcateg categ, tcursnum curs, char * objname, bool delrecs, const char * odbc_prefix)
// Deletes the grid on error.
{ delrecs = delrecs && xcdp->connection_type!=CONN_TP_ODBC;
  int flags = delrecs ? NO_INSERT|DEL_RECS|COUNT_RECS : 0;
  if (parent) flags |= CHILD_VIEW;
  char * src;
  if (categ==CATEG_DIRCUR)
  { src=(char*)corealloc(50, 78);  sprintf(src, "DEFAULT %u CURSORVIEW", curs);
    flags |= AUTO_CURSOR;
  }
  else
    src=initial_view(xcdp, 0, 0, delrecs ? VIEWTYPE_SIMPLE | VIEWTYPE_DEL : VIEWTYPE_SIMPLE, objname, categ, curs, odbc_prefix);
  if (!src) 
    { delete this;  return NULL; }
  wxWindow * ret = open_form(parent, xcdp, src, NOOBJECT, flags);  // deletes the grid on error
  corefree(src);
  return ret;             
}

wxWindow * open_data_grid(wxWindow * parent, xcdp_t xcdp, tcateg categ, tcursnum curs, char * objname, bool delrecs, const char * odbc_prefix)
{ DataGrid * grid = new DataGrid(xcdp);
  return grid->open_data_grid(parent, xcdp, categ, curs, objname, delrecs, odbc_prefix);  // deletes the grid on error
}

wxWindow * open_form(wxWindow * parent, cdp_t cdp, const char * source, tcursnum curs, int flags)
{ DataGrid * grid = new DataGrid(cdp);
  return grid->open_form(parent, cdp, source, curs, flags);  // deletes the grid on error
}

void DataGridTable :: prepare()
// adds initial number of records
{ 
  //if (inst->dt->remap_a) -- must be called for ODBC tables, too
  remap_expand(inst, 100);
}

void DataGrid::refresh(t_refresh_extent extent)
{ 
  if (inst->cdp && inst->cdp->RV.holding) return;
 // store the position:
  int row, col;
  col = GetGridCursorCol();
  if (col>=GetNumberCols()) col=GetNumberCols()-1;  // happens
  row = GetGridCursorRow();

  if (extent == RESET_CURSOR)
  { if (!dgt->write_changes(OWE_CONT_OR_RELOAD)) return;
    if (inst->xcdp->connection_type==CONN_TP_ODBC)
      odbc_reset_cursor(inst);
    else if (IS_CURSOR_NUM(inst->dt->cursnum)) /* remake WB cursors */
    { close_dependent_editors(inst->cdp, inst->dt->cursnum, true);
      tcursnum newcurs;
      if (cd_Open_cursor(inst->cdp, inst->dt->cursnum, &newcurs))
        { cd_Signalize(inst->cdp);  inst->dt->cursnum=NOOBJECT;  return; }
      inst->dt->cursnum=newcurs;
    }
  }
  
  if (extent >= RESET_REMAP)
    remap_reset(inst);  
 // always reset the cache contents:
  cache_reset(inst);
  dgt->set_edited_row(NORECNUM);
  
  if (extent >= RESET_REMAP)  // originally this was called always
  { if (inst->dt->remap_a) remap_expand(inst, row>100 ? row : 100);
    DeleteRows(0, GetNumberRows());
    if (inst->dt->fi_recnum > GetNumberRows())  // some added in OnSize
      AppendRows(inst->dt->fi_recnum-GetNumberRows());
    if (row>=inst->dt->fi_recnum) row=inst->dt->fi_recnum-1;
    if (col!=-1 && row!=-1)
    { SetGridCursor(row, col);
      MakeCellVisible(row, col);
    }
  }
}

void DataGrid::make_cursor_persistent(void)
{ cd_Make_persistent(cdp, inst->dt->cursnum); }

void grid_delete_bitmaps(void)
{ delete_bitmaps(DataGrid::data_tool_info); }

#if 0
////////////////////////////////// dada grid frame ////////////////////////////////////
BEGIN_EVENT_TABLE( DataFramePopup, wxFrame )
  //EVT_WINDOW_CREATE(DataFramePopup::OnCreate)
  EVT_MENU(-1, DataFramePopup::OnCommand)
  EVT_ACTIVATE(DataFramePopup::OnActivate)
  EVT_CLOSE(DataFramePopup::OnCloseWindow)
  EVT_SET_FOCUS(DataFramePopup::OnFocus) 
  EVT_CUSTOM(disconnect_event, 0, DataFramePopup::OnDisconnecting)
  EVT_CUSTOM(record_update, 0, DataFramePopup::OnRecordUpdate)
END_EVENT_TABLE()

void DataFramePopup::OnInternalIdle()
{ wxFrame::OnInternalIdle();  // without this does not resize!!
}

void DataFramePopup::OnCommand(wxCommandEvent & event)
{ grid->OnCommand(event); }

void DataFramePopup::OnFocus(wxFocusEvent & event) 
{ 
#ifdef _DEBUG
  wxLogDebug(wxT("DataFramePopup::OnFocus")); 
#endif
  if (grid) grid->SetFocus();
  event.Skip();
}

void DataFramePopup::OnCloseWindow(wxCloseEvent & event)
{ wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND)
  cmd.SetId(event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND);
  if (grid) grid->OnCommand(cmd);
  else event.Skip();
}

void DataFramePopup::OnDisconnecting(wxEvent & event)
// Destroys itself if the underlying grid should be closed.
{ ServerDisconnectingEvent * disev = (ServerDisconnectingEvent*)&event;
  disev->ignored=false;
  if (grid)
    grid->OnDisconnecting(*disev);
  if (!disev->ignored)
    Destroy();
}

void DataFramePopup::OnRecordUpdate(wxEvent & event)
{ RecordUpdateEvent * ruev = (RecordUpdateEvent*)&event;
  if (grid) grid->OnRecordUpdate(*ruev); 
}


void DataFramePopup::OnActivate(wxActivateEvent & event)
{ if (event.GetActive())
    grid->Activated();
  else
    grid->Deactivated();
  event.Skip();
}

BEGIN_EVENT_TABLE( DataFrameMDI, wxMDIChildFrame )
  EVT_MENU(-1, DataFrameMDI::OnCommand)
  EVT_ACTIVATE(DataFrameMDI::OnActivate)
  EVT_CLOSE(DataFrameMDI::OnCloseWindow)
  EVT_SET_FOCUS(DataFrameMDI::OnFocus) 
  EVT_CUSTOM(disconnect_event, 0, DataFrameMDI::OnDisconnecting)
  EVT_CUSTOM(record_update, 0, DataFrameMDI::OnRecordUpdate)
END_EVENT_TABLE()

void DataFrameMDI::OnCommand(wxCommandEvent & event)
{ grid->OnCommand(event); }

void DataFrameMDI::OnFocus(wxFocusEvent & event) 
{ if (grid) grid->SetFocus();
  event.Skip();
}

void DataFrameMDI::OnCloseWindow(wxCloseEvent & event)
{ wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND)
  cmd.SetId(event.CanVeto() ? TD_CPM_EXIT_FRAME : TD_CPM_EXIT_UNCOND);
  if (grid) grid->OnCommand(cmd);
  //else 
  event.Skip();
}

void DataFrameMDI::OnDisconnecting(wxEvent & event)
// Destroys itself if the underlying grid should be closed.
{ ServerDisconnectingEvent * disev = (ServerDisconnectingEvent*)&event;
  disev->ignored=false;
  if (grid)
    grid->OnDisconnecting(*disev);
  if (!disev->ignored)
    Destroy();
}

void DataFrameMDI::OnRecordUpdate(wxEvent & event)
{ RecordUpdateEvent * ruev = (RecordUpdateEvent*)&event;
  if (grid) grid->OnRecordUpdate(*ruev); // the designer will ignore or save or veto
}

void DataFrameMDI::OnActivate(wxActivateEvent & event)
{ if (event.GetActive())
    grid->Activated();
  else
    grid->Deactivated();
  event.Skip();
}
#endif
////////////////////////////////////////// multiline text editor //////////////////////////////////////
wxGridCellMLTextEditor::wxGridCellMLTextEditor(bool read_onlyIn)
{ read_only = read_onlyIn;
}

void wxGridCellMLTextEditor::Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler)
{
#if defined(__WXMSW__)
    long style = wxTE_MULTILINE | wxHSCROLL | wxTE_PROCESS_TAB | wxTE_AUTO_SCROLL;
#else
    long style = wxTE_MULTILINE | wxHSCROLL;
#endif
    if (read_only) style |= wxTE_READONLY;
    m_control = new wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, style);
    wxGridCellEditor::Create(parent, id, evtHandler);
    parent_grid = parent;
}

void wxGridCellMLTextEditor::PaintBackground(const wxRect& WXUNUSED(rectCell),
                                           wxGridCellAttr * WXUNUSED(attr))
{
    // as we fill the entire client area, don't do anything here to minimize
    // flicker
}

void wxGridCellMLTextEditor::SetSize(const wxRect& rectOrig)
{
    wxRect rect(rectOrig);
    rect.height=wxMax(rect.height, 300);  rect.width=wxMax(rect.width, 300);
   // limit the size so that the editor does not go beyon its parent
    int pwidth, pheight;
    parent_grid->GetClientSize(&pwidth, &pheight);
    if (rect.x+rect.width  > pwidth ) rect.width  = pwidth -rect.x;
    if (rect.y+rect.height > pheight) rect.height = pheight-rect.y;
#if 0

    // Make the edit control large enough to allow for internal
    // margins
    //
    // TODO: remove this if the text ctrl sizing is improved esp. for
    // unix
    //
#if defined(__WXGTK__)
    if (rect.x != 0)
    {
        rect.x += 1;
        rect.y += 1;
        rect.width -= 1;
        rect.height -= 1;
    }
#else // !GTK
    int extra_x = ( rect.x > 2 )? 2 : 1;

// MB: treat MSW separately here otherwise the caret doesn't show
// when the editor is in the first row.
#if defined(__WXMSW__)
    int extra_y = 2;
#else
    int extra_y = ( rect.y > 2 )? 2 : 1;
#endif // MSW

#if defined(__WXMOTIF__)
    extra_x *= 2;
    extra_y *= 2;
#endif
    rect.SetLeft( wxMax(0, rect.x - extra_x) );
    rect.SetTop( wxMax(0, rect.y - extra_y) );
    rect.SetRight( rect.GetRight() + 2*extra_x );
    rect.SetBottom( rect.GetBottom() + 2*extra_y );
#endif // GTK/!GTK
#endif

    wxGridCellEditor::SetSize(rect);
}

void wxGridCellMLTextEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));

    m_startValue = grid->GetTable()->GetValue(row, col);

    DoBeginEdit(m_startValue);
}

void wxGridCellMLTextEditor::DoBeginEdit(const wxString& startValue)
{
    Text()->SetValue(startValue);
    Text()->SetInsertionPointEnd();
    Text()->SetSelection(-1,-1);
    Text()->SetFocus();
}

bool wxGridCellMLTextEditor::EndEdit(int row, int col,
                                   wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));

    bool changed = FALSE;
    wxString value = Text()->GetValue();
    if (value != m_startValue)
        changed = TRUE;

    if (changed)
        grid->GetTable()->SetValue(row, col, value);

    m_startValue = wxEmptyString;
    // No point in setting the text of the hidden control
    //Text()->SetValue(m_startValue);

    return changed;
}


void wxGridCellMLTextEditor::Reset()
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));

    DoReset(m_startValue);
}

void wxGridCellMLTextEditor::DoReset(const wxString& startValue)
{
    Text()->SetValue(startValue);
    Text()->SetInsertionPointEnd();
}

bool wxGridCellMLTextEditor::IsAcceptedKey(wxKeyEvent& event)
{
    if ( wxGridCellEditor::IsAcceptedKey(event) )
    {
        int keycode = event.GetKeyCode();
        switch ( keycode )
        {
            case WXK_NUMPAD0:
            case WXK_NUMPAD1:
            case WXK_NUMPAD2:
            case WXK_NUMPAD3:
            case WXK_NUMPAD4:
            case WXK_NUMPAD5:
            case WXK_NUMPAD6:
            case WXK_NUMPAD7:
            case WXK_NUMPAD8:
            case WXK_NUMPAD9:
            case WXK_MULTIPLY:
            case WXK_NUMPAD_MULTIPLY:
            case WXK_ADD:
            case WXK_NUMPAD_ADD:
            case WXK_SUBTRACT:
            case WXK_NUMPAD_SUBTRACT:
            case WXK_DECIMAL:
            case WXK_NUMPAD_DECIMAL:
            case WXK_DIVIDE:
            case WXK_NUMPAD_DIVIDE:
                return TRUE;

            default:
                // accept Unicode character keys
                if (event.GetUnicodeKey() >= 128)
                    return TRUE;
                // accept 8 bit chars too if isprint() agrees
                if ( (keycode < 255) && (wxIsprint(keycode)) )
                    return TRUE;
        }
    }

    return FALSE;
}

void wxGridCellMLTextEditor::StartingKey(wxKeyEvent& event)
{
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>6 || wxRELEASE_NUMBER>=2
    wxTextCtrl* tc = Text();
    wxChar ch;
    long pos;

#if wxUSE_UNICODE
    ch = event.GetUnicodeKey();
    if (ch <= 127)
        ch = (wxChar)event.GetKeyCode();
#else
    ch = (wxChar)event.GetKeyCode();
#endif
    switch (ch)
    {
        case WXK_DELETE:
            // delete the character at the cursor
            pos = tc->GetInsertionPoint();
            if (pos < tc->GetLastPosition())
                tc->Remove(pos, pos+1);
            break;

        case WXK_BACK:
            // delete the character before the cursor
            pos = tc->GetInsertionPoint();
            if (pos > 0)
                tc->Remove(pos-1, pos);
            break;

        default:
            tc->WriteText(ch);
            break;
    }
#else  // old version
    if ( !Text()->EmulateKeyPress(event) )
    {
        event.Skip();
    }
#endif
}

void wxGridCellMLTextEditor::HandleReturn( wxKeyEvent& event )
{
#if defined(__WXMOTIF__) || defined(__WXGTK__)
    // wxMotif needs a little extra help...
    size_t pos = (size_t)( Text()->GetInsertionPoint() );
    wxString s( Text()->GetValue() );
    s = s.Left(pos) + wxT("\n") + s.Mid(pos);
    Text()->SetValue(s);
    Text()->SetInsertionPoint( pos+1 );
#else
    // the other ports can handle a Return key press
    //
    event.Skip();
#endif
}

// return the value in the text control
wxString wxGridCellMLTextEditor::GetValue() const
{
  return Text()->GetValue();
}


////////////////////////////////////////// CLOB editor ////////////////////////////////////////////////
#if CLOB_EDITOR
class wxGridCellCLOBEditor : public wxGridCellEditor
{
public:
    wxGridCellCLOBEditor();

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);
    virtual void SetSize(const wxRect& rect);

    virtual void PaintBackground(const wxRect& rectCell, wxGridCellAttr *attr);

    virtual bool IsAcceptedKey(wxKeyEvent& event);
    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();
    virtual void StartingKey(wxKeyEvent& event);
    virtual void HandleReturn(wxKeyEvent& event);

    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellCLOBEditor; }

protected:
    wxTextCtrl *Text() const { return (wxTextCtrl *)m_control; }

    // parts of our virtual functions reused by the derived classes
    void DoBeginEdit(const wxString& startValue);
    void DoReset(const wxString& startValue);

private:
    //wxString m_startValue;

    DECLARE_NO_COPY_CLASS(wxGridCellCLOBEditor)
};

wxGridCellCLOBEditor::wxGridCellCLOBEditor()
{
}

void wxGridCellCLOBEditor::Create(wxWindow* parent,
                                  wxWindowID id,
                                  wxEvtHandler* evtHandler)
{
    m_control = new wxTextCtrl(parent, id, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize
#if defined(__WXMSW__)
                               , wxTE_PROCESS_TAB | wxTE_AUTO_SCROLL
#endif
                              );

    wxGridCellEditor::Create(parent, id, evtHandler);
}

void wxGridCellCLOBEditor::PaintBackground(const wxRect& WXUNUSED(rectCell),
                                           wxGridCellAttr * WXUNUSED(attr))
{
    // as we fill the entire client area, don't do anything here to minimize
    // flicker
}

void wxGridCellCLOBEditor::SetSize(const wxRect& rectOrig)
{
    fsed->SetSize(rect.x, rect.y, 200, 200);
#if 0
    wxRect rect(rectOrig);

    // Make the edit control large enough to allow for internal
    // margins
    //
    // TODO: remove this if the text ctrl sizing is improved esp. for
    // unix
    //
#if defined(__WXGTK__)
    if (rect.x != 0)
    {
        rect.x += 1;
        rect.y += 1;
        rect.width -= 1;
        rect.height -= 1;
    }
#else // !GTK
    int extra_x = ( rect.x > 2 )? 2 : 1;

// MB: treat MSW separately here otherwise the caret doesn't show
// when the editor is in the first row.
#if defined(__WXMSW__)
    int extra_y = 2;
#else
    int extra_y = ( rect.y > 2 )? 2 : 1;
#endif // MSW

#if defined(__WXMOTIF__)
    extra_x *= 2;
    extra_y *= 2;
#endif
    rect.SetLeft( wxMax(0, rect.x - extra_x) );
    rect.SetTop( wxMax(0, rect.y - extra_y) );
    rect.SetRight( rect.GetRight() + 2*extra_x );
    rect.SetBottom( rect.GetBottom() + 2*extra_y );
#endif // GTK/!GTK

    wxGridCellEditor::SetSize(rect);
#endif
}

void wxGridCellCLOBEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));
#if 0
    m_startValue = grid->GetTable()->GetValue(row, col);

    DoBeginEdit(m_startValue);
#endif
}

void wxGridCellCLOBEditor::DoBeginEdit(const wxString& startValue)
{
    Text()->SetValue(startValue);
    Text()->SetInsertionPointEnd();
    Text()->SetSelection(-1,-1);
    Text()->SetFocus();
}

bool wxGridCellCLOBEditor::EndEdit(int row, int col,
                                   wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));
#if 0
    bool changed = FALSE;
    wxString value = Text()->GetValue();
    if (value != m_startValue)
        changed = TRUE;

    if (changed)
        grid->GetTable()->SetValue(row, col, value);

    m_startValue = wxEmptyString;
    // No point in setting the text of the hidden control
    //Text()->SetValue(m_startValue);
#endif
    return changed;
}


void wxGridCellCLOBEditor::Reset()
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));

    //DoReset(m_startValue);
}

void wxGridCellCLOBEditor::DoReset(const wxString& startValue)
{
    Text()->SetValue(startValue);
    Text()->SetInsertionPointEnd();
}

bool wxGridCellCLOBEditor::IsAcceptedKey(wxKeyEvent& event)
{
    if ( wxGridCellEditor::IsAcceptedKey(event) )
    {
        int keycode = event.GetKeyCode();
        switch ( keycode )
        {
            case WXK_NUMPAD0:
            case WXK_NUMPAD1:
            case WXK_NUMPAD2:
            case WXK_NUMPAD3:
            case WXK_NUMPAD4:
            case WXK_NUMPAD5:
            case WXK_NUMPAD6:
            case WXK_NUMPAD7:
            case WXK_NUMPAD8:
            case WXK_NUMPAD9:
            case WXK_MULTIPLY:
            case WXK_NUMPAD_MULTIPLY:
            case WXK_ADD:
            case WXK_NUMPAD_ADD:
            case WXK_SUBTRACT:
            case WXK_NUMPAD_SUBTRACT:
            case WXK_DECIMAL:
            case WXK_NUMPAD_DECIMAL:
            case WXK_DIVIDE:
            case WXK_NUMPAD_DIVIDE:
                return TRUE;

            default:
                // accept 8 bit chars too if isprint() agrees
                if ( (keycode < 255) && (wxIsprint(keycode)) )
                    return TRUE;
        }
    }

    return FALSE;
}

void wxGridCellCLOBEditor::StartingKey(wxKeyEvent& event)
{
    //if ( !Text()->EmulateKeyPress(event) )
    {
        event.Skip();
    }
}

void wxGridCellCLOBEditor::HandleReturn( wxKeyEvent&
                                         WXUNUSED_GTK(WXUNUSED_MOTIF(event)) )
{
#if defined(__WXMOTIF__) || defined(__WXGTK__)
    // wxMotif needs a little extra help...
    size_t pos = (size_t)( Text()->GetInsertionPoint() );
    wxString s( Text()->GetValue() );
    s = s.Left(pos) + wxT("\n") + s.Mid(pos);
    Text()->SetValue(s);
    Text()->SetInsertionPoint( pos );
#else
    // the other ports can handle a Return key press
    //
    event.Skip();
#endif
}

#endif // CLOB_EDITOR

/////////////////////////////////////// FSED CLOB editor //////////////////////////////////////////
/* Integration of external editors into the grid:
The cell rendered must not obtain long text via GetValue because it work very slowno on them.
If GetValue returns the shortened text, the editor must get its value in another way.
The external editor should not work with the cache of the grid, because the grid may be scrolled. 
  It works with the contents of the cursor of the grid.
The fullscreen editor needs own menu and toolbar -> It is opened as external "competing frame" and synchronized by events:
- when the contents of the external editor is saved, the event notifies other frames showing the same contents to reload.
- when the owned of the editor (and its cursor) is closed, editor are notified to close itself.
*/
#ifdef FSED_CLOB_EDITOR
wxGridCellFSEDCLOBEditor::wxGridCellFSEDCLOBEditor()
{
}

void wxGridCellFSEDCLOBEditor::Create(wxWindow* parent,
                                  wxWindowID id,
                                  wxEvtHandler* evtHandler)
{
 // a (fictive) control must be created because [m_control] is used in some oparations 
  m_control = new wxStaticText(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
  wxGridCellEditor::Create(parent, id, evtHandler);
#if 0  // no more, crash when row is synchronized during openning the editor
 // start the external editor:
  DataGrid * grid = (DataGrid*)parent->GetParent();
  grid->StartExternalEditor(grid->GetGridCursorRow(), grid->GetGridCursorCol());
#endif
}

void wxGridCellFSEDCLOBEditor::PaintBackground(const wxRect& WXUNUSED(rectCell),
                                           wxGridCellAttr * WXUNUSED(attr))
{
}

void wxGridCellFSEDCLOBEditor::SetSize(const wxRect& rectOrig)
{
}

void wxGridCellFSEDCLOBEditor::BeginEdit(int row, int col, wxGrid* grid)
{
#if 0  // no more, crash when row is synchronized during openning the editor
  grid->DisableCellEditControl();  // immediately closes the grid editor, but FSED remains open
  // otherwise [m_control] would be hidden instead of closed and the next editing would not call Create()
#else
  grid->DisableCellEditControl();  // immediately closes the grid editor
  // otherwise [m_control] would be hidden instead of closed and the next editing would not call Create()
  // sets focus to the grid, so must be before openning the FSED editor
 // start the external editor:
  ((DataGrid*)grid)->StartExternalEditor(grid->GetGridCursorRow(), grid->GetGridCursorCol());
 // if (m_control) m_control->Hide();  -- replaced by DisableCellEditControl
#endif
}

void wxGridCellFSEDCLOBEditor::DoBeginEdit(const wxString& startValue)
{
}

bool wxGridCellFSEDCLOBEditor::EndEdit(int row, int col,
                                   wxGrid* grid)
{
#if 0  // no more, crash when row is synchronized during openning the editor
 // removing the control causes that it will be re-created in the next edit operation
  if (m_control)
  { m_control->Destroy();
    m_control=NULL;
  }
#endif
  return false;
}


void wxGridCellFSEDCLOBEditor::Reset()
{
}

void wxGridCellFSEDCLOBEditor::DoReset(const wxString& startValue)
{
}

void wxGridCellFSEDCLOBEditor::StartingKey(wxKeyEvent& event)
{
  event.Skip();
}


#endif // FSED_CLOB_EDITOR

///////////////////////////////////////// 3-state Bool editor ///////////////////////////////////
// ----------------------------------------------------------------------------
// wxGridCellBool3Editor
// ----------------------------------------------------------------------------

void wxGridCellBool3Editor::Create(wxWindow* parent,
                                  wxWindowID id,
                                  wxEvtHandler* evtHandler)
{
    m_control = new wxCheckBox(parent, id, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxNO_BORDER | wxCHK_3STATE | wxCHK_ALLOW_3RD_STATE_FOR_USER );

    wxGridCellEditor::Create(parent, id, evtHandler);
}

void wxGridCellBool3Editor::SetSize(const wxRect& r)
{
    bool resize = FALSE;
    wxSize size = m_control->GetSize();
    wxCoord minSize = wxMin(r.width, r.height);

    // check if the checkbox is not too big/small for this cell
    wxSize sizeBest = m_control->GetBestSize();
    if ( !(size == sizeBest) )
    {
        // reset to default size if it had been made smaller
        size = sizeBest;

        resize = TRUE;
    }

    if ( size.x >= minSize || size.y >= minSize )
    {
        // leave 1 pixel margin
        size.x = size.y = minSize - 2;

        resize = TRUE;
    }

    if ( resize )
    {
        m_control->SetSize(size);
    }

    // position it in the centre of the rectangle (TODO: support alignment?)

#if defined(__WXGTK__) || defined (__WXMOTIF__)
    // the checkbox without label still has some space to the right in wxGTK,
    // so shift it to the right
    size.x -= 8;
#elif defined(__WXMSW__)
    // here too, but in other way
    size.x += 1;
    size.y -= 2;
#endif

    int hAlign = wxALIGN_CENTRE;
    int vAlign = wxALIGN_CENTRE;
    if (GetCellAttr())
        GetCellAttr()->GetAlignment(& hAlign, & vAlign);

    int x = 0, y = 0;
    if (hAlign == wxALIGN_LEFT)
    {
        x = r.x + 2;
#ifdef __WXMSW__
        x += 2;
#endif
        y = r.y + r.height/2 - size.y/2;
    }
    else if (hAlign == wxALIGN_RIGHT)
    {
        x = r.x + r.width - size.x - 2;
        y = r.y + r.height/2 - size.y/2;
    }
    else if (hAlign == wxALIGN_CENTRE)
    {
        x = r.x + r.width/2 - size.x/2;
        y = r.y + r.height/2 - size.y/2;
    }

    m_control->Move(x, y);
}

void wxGridCellBool3Editor::Show(bool show, wxGridCellAttr *attr)
{
    m_control->Show(show);

    if ( show )
    {
        wxColour colBg = attr ? attr->GetBackgroundColour() : *wxLIGHT_GREY;
        CBox()->SetBackgroundColour(colBg);
    }
}

void wxGridCellBool3Editor::BeginEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));

    wxString cellval( grid->GetTable()->GetValue(row, col) );
    if      (cellval == wxT("0")) m_startValue = wxCHK_UNCHECKED; 
    else if (cellval == wxT("1")) m_startValue = wxCHK_CHECKED; 
    else                          m_startValue = wxCHK_UNDETERMINED;
    CBox()->Set3StateValue(m_startValue);
    CBox()->SetFocus();
}

bool wxGridCellBool3Editor::EndEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));

    bool changed = false;
    wxCheckBoxState value = CBox()->Get3StateValue();
    if ( value != m_startValue )
        changed = true;
    if ( changed )
       grid->GetTable()->SetValue(row, col, value==wxCHK_CHECKED ? _T("1") : value==wxCHK_UNCHECKED ? _T("0") : wxEmptyString);
    return changed;
}

void wxGridCellBool3Editor::Reset()
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be Created first!"));

    CBox()->Set3StateValue(m_startValue);
}

void wxGridCellBool3Editor::StartingClick()
{
    CBox()->Set3StateValue(CBox()->Get3StateValue()==wxCHK_CHECKED ? wxCHK_UNCHECKED : wxCHK_CHECKED);
}

bool wxGridCellBool3Editor::IsAcceptedKey(wxKeyEvent& event)
{
    if ( wxGridCellEditor::IsAcceptedKey(event) )
    {
        int keycode = event.GetKeyCode();
        switch ( keycode )
        {
            case WXK_MULTIPLY:
            case WXK_NUMPAD_MULTIPLY:
            case WXK_ADD:
            case WXK_NUMPAD_ADD:
            case WXK_SUBTRACT:
            case WXK_NUMPAD_SUBTRACT:
            case WXK_SPACE:
            case '+':
            case '-':
                return TRUE;
        }
    }

    return FALSE;
}

// return the value as "1" for true and the empty string for false
wxString wxGridCellBool3Editor::GetValue() const
{
  wxCheckBoxState bSet = CBox()->Get3StateValue();
  return bSet==wxCHK_CHECKED ? _T("1") : bSet==wxCHK_UNCHECKED ? _T("0") : wxEmptyString;
}

// ----------------------------------------------------------------------------
// wxGridCellBool3Renderer
// ----------------------------------------------------------------------------

wxSize wxGridCellBool3Renderer::ms_sizeCheckMark;

// FIXME these checkbox size calculations are really ugly...

// between checkmark and box
static const wxCoord wxGRID_CHECKMARK_MARGIN = 2;

wxSize wxGridCellBool3Renderer::GetBestSize(wxGrid& grid,
                                           wxGridCellAttr& WXUNUSED(attr),
                                           wxDC& WXUNUSED(dc),
                                           int WXUNUSED(row),
                                           int WXUNUSED(col))
{
    // compute it only once (no locks for MT safeness in GUI thread...)
    if ( !ms_sizeCheckMark.x )
    {
        // get checkbox size
        wxCheckBox *checkbox = new wxCheckBox(&grid, -1, wxEmptyString);
        wxSize size = checkbox->GetBestSize();
        wxCoord checkSize = size.y + 2*wxGRID_CHECKMARK_MARGIN;

        // FIXME wxGTK::wxCheckBox::GetBestSize() gives "wrong" result
#if defined(__WXGTK__) || defined(__WXMOTIF__)
        checkSize -= size.y / 2;
#endif

        delete checkbox;

        ms_sizeCheckMark.x = ms_sizeCheckMark.y = checkSize;
    }

    return ms_sizeCheckMark;
}

void wxGridCellBool3Renderer::Draw(wxGrid& grid,
                                  wxGridCellAttr& attr,
                                  wxDC& dc,
                                  const wxRect& rect,
                                  int row, int col,
                                  bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);

    // draw a check mark in the centre (ignoring alignment - TODO)
    wxSize size = GetBestSize(grid, attr, dc, row, col);

    // don't draw outside the cell
    wxCoord minSize = wxMin(rect.width, rect.height);
    if ( size.x >= minSize || size.y >= minSize )
    {
        // and even leave (at least) 1 pixel margin
        size.x = size.y = minSize - 2;
    }

    // draw a border around checkmark
    int vAlign, hAlign;
    attr.GetAlignment(& hAlign, &vAlign);

    wxRect rectBorder;
    if (hAlign == wxALIGN_CENTRE)
    {
        rectBorder.x = rect.x + rect.width/2 - size.x/2;
        rectBorder.y = rect.y + rect.height/2 - size.y/2;
        rectBorder.width = size.x;
        rectBorder.height = size.y;
    }
    else if (hAlign == wxALIGN_LEFT)
    {
        rectBorder.x = rect.x + 2;
        rectBorder.y = rect.y + rect.height/2 - size.y/2;
        rectBorder.width = size.x;
        rectBorder.height = size.y;
    }
    else if (hAlign == wxALIGN_RIGHT)
    {
        rectBorder.x = rect.x + rect.width - size.x - 2;
        rectBorder.y = rect.y + rect.height/2 - size.y/2;
        rectBorder.width = size.x;
        rectBorder.height = size.y;
    }

    wxCheckBoxState value;
    wxString cellval( grid.GetTable()->GetValue(row, col) );
    if      (cellval == wxT("0")) value = wxCHK_UNCHECKED; 
    else if (cellval == wxT("1")) value = wxCHK_CHECKED; 
    else                          value = wxCHK_UNDETERMINED;

    if ( value==wxCHK_CHECKED )
    {
        wxRect rectMark = rectBorder;
#ifdef __WXMSW__
        // MSW DrawCheckMark() is weird (and should probably be changed...)
        rectMark.Inflate(-wxGRID_CHECKMARK_MARGIN/2);
        rectMark.x++;
        rectMark.y++;
#else // !MSW
        rectMark.Inflate(-wxGRID_CHECKMARK_MARGIN);
#endif // MSW/!MSW

        dc.SetTextForeground(attr.GetTextColour());
        dc.DrawCheckMark(rectMark);
    }
    if ( value==wxCHK_UNDETERMINED )
      dc.SetBrush(*wxGREY_BRUSH);
    else 
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(attr.GetTextColour(), 1, wxSOLID));
    dc.DrawRectangle(rectBorder);
}

BEGIN_EVENT_TABLE( CColLabel, wxEvtHandler )
    EVT_PAINT( CColLabel::OnPaint )
END_EVENT_TABLE()

void CColLabel::OnPaint( wxPaintEvent& event )
{ wxPaintDC dc(m_Label);
  m_Label->ProcessEvent(event);
  if (!m_Grid->inst->order)
    return;
  COrderConds Conds;
  if (compile_order(m_Grid->cdp, m_Grid->inst->order, &Conds))
    return;
  CQOrder *Cond = Conds.GetFirst();
  wxArrayInt cols = m_Grid->CalcColLabelsExposed(m_Label->GetUpdateRegion());
  for (int i = 0;  i < cols.GetCount();  i++ )
  { int Col = cols[i];
    tptr itext = item_text(m_Grid->sinst->sitems[Col].itm);
    if (itext==NULL)
      continue;
    if (wb_stricmp(m_Grid->cdp, Cond->Expr, itext) != 0)
      continue;
    { long lineWidth, lineHeight;
      wxString Text = m_Grid->GetColLabelValue(Col);

      dc.SetFont(m_Grid->GetLabelFont());
      dc.GetTextExtent(Text, &lineWidth, &lineHeight);
      int hAlign, vAlign;
      m_Grid->GetColLabelAlignment(&hAlign, &vAlign);

      int Rest;
      int width = m_Grid->GetColWidth(Col) - 4; 
      switch (hAlign)
      {
      case wxALIGN_RIGHT:
        Rest = 1;
        break;
      case wxALIGN_CENTRE:
        Rest = (width - lineWidth) / 2;
        break;
      default:
        Rest = width - lineWidth - 1;
        break;
      }
      Rest -= 2;
      if (Rest >= 8)
      { int x = m_Grid->GetColRight(Col) - 2 - Rest;
#ifdef LINUX
        // Grid na Linuxu nebere ohled na scrollovani
        int yy;
        m_Grid->CalcScrolledPosition(x, 0, &x, &yy);
#endif	  
        int y = (m_Grid->GetColLabelSize() - 8) / 2;
        dc.DrawBitmap(GetOrderBitmap(Cond->Desc != 0), x, y, true);
      }
    }
  }
  event.Skip(false);
}

#include "bmps/asc.xpm"
#include "bmps/desc.xpm"

static wxBitmap BmpAsc;
static wxBitmap BmpDesc;

static char ShadowCol[14] = "0 c #000000";
 
const wxBitmap &CColLabel::GetOrderBitmap(bool Desc)
{ if (asc_xpm[1] != ShadowCol)
  { 
#ifdef WINS
    wxColour Col = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
    sprintf(ShadowCol + 5, "%06X", Col.GetPixel() & 0xFFFFFF);
#else	
    wxColour Col = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    sprintf(ShadowCol + 5, "%02X%02X%02X", Col.Red() * 7 / 12, Col.Green() * 7 / 12, Col.Blue() * 7 / 12);
#endif	
    asc_xpm[1]  = ShadowCol;
    desc_xpm[1] = ShadowCol;
  }
  if (Desc)
  { if (!BmpDesc.Ok())  
      BmpDesc = wxBitmap(desc_xpm);
    return BmpDesc;
  }
  else
  { if (!BmpAsc.Ok())  
      BmpAsc = wxBitmap(asc_xpm);
    return BmpAsc;
  }
}

///////////////////////////////////// DataRowObject ////////////////////////////////////////////

