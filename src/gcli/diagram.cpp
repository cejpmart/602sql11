// diagram.cpp
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
#include <wx/listimpl.cpp>
#include <wx/dataobj.h>
#include <wx/dnd.h>
#include "wx/clipbrd.h"
#include <wx/printdlg.h>
           
#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#include "bmps/print_preview.xpm"
#include "bmps/print.xpm"
#include "bmps/print_preferences.xpm"
#include "bmps/page_preferences.xpm"
#include "tablepanel.h"

class diagram_designer : public table_panel, public any_designer
{
 public:
  diagram_designer(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn) :
    any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, CATEG_DRAWING, _draw_xpm, &diagram_coords),
    table_panel(cdpIn, schema_nameIn)
    { changed=false;  
    }
  ~diagram_designer(void)
  { remove_tools(); 
  }
  bool open(wxWindow * parent, t_global_style my_style);

 ////////// typical designer methods and members: ///////////////////////
#if WXWIN_COMPATIBILITY_2_4
  enum { TOOL_COUNT = 11 };
#else
  enum { TOOL_COUNT = 10 };
#endif
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
  void _set_focus(void)
    { if (GetHandle()) SetFocus(); }
  void set_changed(void)
    { changed=true; }
  void table_altered_notif(ttablenum tbnum);
 // member variables:
  bool changed;
  tobjname object_name;

 // Callbacks:
  void OnKeyDown(wxKeyEvent & event);
  DECLARE_EVENT_TABLE()
};

/////////////////////////// virtual methods (commented in class any_designer): //////////////////////////////////

char * diagram_designer::make_source(bool alter)
{ return table_panel::make_source(); }

void diagram_designer::set_designer_caption(void)
{ wxString caption;
  if (!modifying()) caption = _("Design a New Diagram");
  else caption.Printf(_("Editing Diagram %s"), wxString(object_name, *cdp->sys_conv).c_str());
  set_caption(caption);
}

bool diagram_designer::IsChanged(void) const
{ return changed; }

wxString diagram_designer::SaveQuestion(void) const
{ return modifying() ?
    _("Your changes have not been saved. Do you want to save your changes?") :
    _("The diagram object has not been saved. Do you want to save the diagram?");
}

bool diagram_designer::save_design(bool save_as)
// Saves the domain, returns true on success.
{ if (!save_design_generic(save_as, object_name, 0))  // using the generic save from any_designer
    return false;  
  changed=false;
  set_designer_caption();
  return true;
}

void diagram_designer::OnCommand(wxCommandEvent & event)
{ 
  switch (event.GetId())
  { case TD_CPM_SAVE:
      if (!IsChanged()) break;  // no message if no changes
      if (!save_design(false)) break;  // error saving, break
      info_box(_("Your design has been saved."), dialog_parent());
      break;
    case TD_CPM_SAVE_AS:
      save_design(true);
      info_box(_("Your design was saved under a new name."), dialog_parent());
      break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
    case TD_CPM_EXIT_FRAME:
      if (exit_request(this, true))
        destroy_designer(GetParent(), &diagram_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
      exit_request(this, false);
      destroy_designer(GetParent(), &diagram_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_PRINT:
    { wxString title;
      title = wxString(_("Diagram ")) + wxString(object_name, *cdp->sys_conv);
      DiagramPrintout printout(title, this);
      wxPrintDialogData printDialogData(* g_printData);
      wxPrinter printer(&printDialogData);
      if (!printer.Print(dialog_parent(), &printout, true /*prompt*/))
      {   if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
              wxMessageBox(_("There was a problem printing.\nPlease check your printer settings."), _("Printing"), wxOK);
          else
              wxMessageBox(_("Printing was cancelled"), _("Printing"), wxOK);
      }
      else
          (*g_printData) = printer.GetPrintDialogData().GetPrintData();
      break;
    }
    case TD_CPM_PRINT_PREVIEW:
    { wxString title;
      title = wxString(_("Diagram ")) + wxString(object_name, *cdp->sys_conv);
      wxPrintDialogData printDialogData(* g_printData);
      DiagramPrintout * printout_for_preview = new DiagramPrintout(title, this);
      wxPrintPreview *preview = new wxPrintPreview(printout_for_preview, new DiagramPrintout(title, this), &printDialogData);
      // DiagramPrintout objects will be deleted in the destructor
      if (!preview->Ok())
      { delete preview;
        wxMessageBox(_("There was a problem generating the preview.\nPlease check your printer settings."), _("Preview"), wxOK);
        return;
      }
      printout_for_preview->my_preview=preview;
      wxPreviewFrame *frame = new wxPreviewFrame(preview, dialog_parent(), _("Diagram Print Preview"), wxPoint(100, 100), wxSize(600, 650));
      frame->Centre(wxBOTH);
      frame->Initialize();
      frame->Show();
      break;
    }
#if WXWIN_COMPATIBILITY_2_4
    case TD_CPM_PRINT_SETUP:  // setup dialog is not supported any more in the new WX versions
    { wxPrintDialogData printDialogData(* g_printData);
      wxPrintDialog printerDialog(dialog_parent(), & printDialogData);
      printerDialog.GetPrintDialogData().SetSetupDialog(true /*show print setup dialog*/);
      printerDialog.ShowModal();
      (*g_printData) = printerDialog.GetPrintDialogData().GetPrintData();
      break;
    }
#endif
    case TD_CPM_PAGE_SETUP:
    { (*g_pageSetupData) = * g_printData;
      wxPageSetupDialog pageSetupDialog(dialog_parent(), g_pageSetupData);
      pageSetupDialog.ShowModal();
      (*g_printData) = pageSetupDialog.GetPageSetupData().GetPrintData();
      (*g_pageSetupData) = pageSetupDialog.GetPageSetupData();
      break;
    }
    case TD_CPM_HELP:
      wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_diagram.html"));  
      break;
   // items from popup menus:
    case TD_CPM_PASTE_TABLES:
    { wxPoint pos= ScreenToClient(wxGetMousePosition());
      PasteTables(pos.x, pos.y, true);
      break;
    }
    case TD_CPM_PASTE:
      PasteTables(10, 10, true);
      break;
    case TD_CPM_REMOVE_TABLE:
      RemoveTable(drag_item);  break;
    case TD_CPM_ALTER_TABLE:
    { table_in_diagram * table = tables.Item(drag_item)->GetData();
      open_table_designer(cdp, table->tbnum, "", table->schema_name);  // folder_name used in SaveAs
      break;
    }
    case TD_CPM_REMOVE_CONSTR:
    { line_in_diagram * line = lines.Item(drag_item)->GetData();
      char query[4*OBJ_NAME_LEN+200];  tobjname constr_name;  tcursnum cursnum;  trecnum cnt;
      table_in_diagram * master, * slave;
      if (line->ending_type[0]==ET_ONE) { master=line->endtable[0];  slave=line->endtable[1]; }
      else { master=line->endtable[1];  slave=line->endtable[0]; }
      sprintf(query, "select * from _iv_foreign_keys where schema_name=\'%s\' and table_name=\'%s\' and for_schema_name=\'%s\' and for_table_name=\'%s\'", slave->schema_name, slave->table_name, master->schema_name, master->table_name);
      if (!cd_Open_cursor_direct(cdp, query, &cursnum))
      { cd_Rec_cnt(cdp, cursnum, &cnt);
        if (!cnt) error_box(_("Constrain not found."), dialog_parent());
        else if (cnt>1) error_box(_("Ambiguous constrains found."), dialog_parent());
        else cd_Read(cdp, cursnum, 0, 3, NULL, constr_name);
        cd_Close_cursor(cdp, cursnum);
        if (cnt==1)
        { sprintf(query, "ALTER TABLE `%s`.`%s` PRESERVE_FLAGS DROP CONSTRAINT `%s`", slave->schema_name, slave->table_name, constr_name);
          uns32 optval; 
          cd_Get_sql_option(cdp, &optval);
          cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, 0);  // new ALTER TABLE selected
          if (cd_SQL_execute(cdp, query, NULL))
            cd_Signalize2(cdp, dialog_parent());
          cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, optval);  // option restored
        }
      }
      Refresh_tables_and_lines();
      Refresh();
      break;
    }
    case TD_CPM_REFRESH:
      Refresh_tables_and_lines();
      Refresh();
      break;
  }
}

void diagram_designer::OnKeyDown( wxKeyEvent& event )
{ 
  OnCommonKeyDown(event);  // event.Skip() is inside
}

BEGIN_EVENT_TABLE( diagram_designer, wxScrolledWindow)
  //EVT_ERASE_BACKGROUND(diagram_designer::OnErase)
  //EVT_PAINT(diagram_designer::OnPaint)
  EVT_MOUSE_EVENTS(table_panel ::OnMouseEvent)
  EVT_KEY_DOWN(diagram_designer::OnKeyDown)
END_EVENT_TABLE()

t_tool_info diagram_designer::tool_info[TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,          File_save_xpm,        wxTRANSLATE("Save Design")),
  t_tool_info(TD_CPM_EXIT,          exit_xpm,             wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_PASTE,         Vlozit_xpm,           wxTRANSLATE("Paste Table(s)")),
  t_tool_info(TD_CPM_REFRESH,       refrpanel_xpm,        wxTRANSLATE("Refresh Diagram")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_PRINT,         print_xpm,            wxTRANSLATE("Print Diagram")),
  t_tool_info(TD_CPM_PRINT_PREVIEW, print_preview_xpm,    wxTRANSLATE("Print Preview")),
#if WXWIN_COMPATIBILITY_2_4
  t_tool_info(TD_CPM_PRINT_SETUP,   print_preferences_xpm,wxTRANSLATE("Print Setup")),
#endif
  t_tool_info(TD_CPM_PAGE_SETUP,    page_preferences_xpm, wxTRANSLATE("Page Setup")),
  t_tool_info(TD_CPM_HELP,          help_xpm,             wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

void diagram_delete_bitmaps(void)
{ delete_bitmaps(diagram_designer::tool_info); 
 // delete semi-persistent print data objects
  delete table_panel::g_printData;
  delete table_panel::g_pageSetupData;
}

wxMenu * diagram_designer::get_designer_menu(void)
{
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items:
  any_designer::get_designer_menu();
 // remove "Validate":
  designer_menu->Destroy(TD_CPM_CHECK);
 // add specific items:
  AppendXpmItem(designer_menu, TD_CPM_PASTE,         _("P&aste Table(s)"),  Vlozit_xpm);
  AppendXpmItem(designer_menu, TD_CPM_REFRESH,       _("&Refresh Diagram"), refrpanel_xpm);
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_PRINT,         _("&Print Diagram"), print_xpm);
  AppendXpmItem(designer_menu, TD_CPM_PRINT_PREVIEW, _("Print Preview"),  print_preview_xpm);
#if WXWIN_COMPATIBILITY_2_4
  AppendXpmItem(designer_menu, TD_CPM_PRINT_SETUP,   _("Print Setup..."), print_preferences_xpm);
#endif
  AppendXpmItem(designer_menu, TD_CPM_PAGE_SETUP,    _("Page Setup..."),  page_preferences_xpm);
  // page setup allows to the the margins but the implementation of margins is up to the application
  return designer_menu;
}

bool diagram_designer::open(wxWindow * parent, t_global_style my_style)
{ int res=0;
  if (!g_printData) g_printData = new wxPrintData;
  if (!g_pageSetupData) 
  { g_pageSetupData = new wxPageSetupDialogData;
    g_pageSetupData->SetMarginTopLeft(wxPoint(10,10));
    g_pageSetupData->SetMarginBottomRight(wxPoint(10,10));
  }

  if (modifying())
  { char * def = cd_Load_objdef(cdp, objnum, CATEG_DRAWING, NULL);
    if (!def) return false;
    cd_Read(cdp, OBJ_TABLENUM, objnum, OBJ_NAME_ATR, NULL, object_name);
    bool res=compile(def);
    corefree(def);
    if (!res) return false;  // Create() not called, "this" is not a child of [parent], must delete
  }
  else  // new design
    *object_name=0;
 // open the window:
  if (!Create(parent, COMP_FRAME_ID, wxDefaultPosition, wxDefaultSize))
    res=5;  // window not opened, "this" is not a child of [parent], must delete
  else
  { competing_frame::focus_receiver = this;
    init();
    designer_opened=true;
    set_virtual_size_by_contents();
    SetScrollRate(20,20);
    SetBackgroundColour(wxColour(255,255,255));
    return true;  // OK
  }
  return res==0;
}

void diagram_designer::table_altered_notif(ttablenum tbnum)
{ bool has_the_table = false;
 // find the table:
  for (DiagrTableList::Node * nodeT =  tables.GetFirst();  nodeT;  nodeT=nodeT->GetNext())
  { table_in_diagram * table = nodeT->GetData();
    if (table->tbnum==tbnum) has_the_table = true;
  }
  if (!has_the_table) return;
 // refresh:
  Refresh_tables_and_lines();
  Refresh();
}

bool start_diagram_designer(cdp_t cdp, tobjnum objnum, const char * folder_name)
{ 
  return open_standard_designer(new diagram_designer(cdp, objnum, folder_name, cdp->sel_appl_name));
}



#if 0
bool start_diagram_designer(wxMDIParentFrame * frame, cdp_t cdp, tobjnum objnum, const char * folder_name)
{ wxTopLevelWindow * dsgn_frame;  diagram_designer * dsgn;
  if (global_style==ST_MDI)
  { DesignerFrameMDI * frm = new DesignerFrameMDI(wxGetApp().frame, -1, wxEmptyString);
    if (!frm) return false;
    dsgn_frame=frm;
    dsgn = new diagram_designer(cdp, objnum, folder_name, cdp->sel_appl_name, dsgn_frame);
    if (!dsgn) goto err;
    frm->dsgn = dsgn;
  }
  else
  { DesignerFramePopup * frm = new DesignerFramePopup(wxGetApp().frame, -1, wxEmptyString, diagram_coords.get_pos(), diagram_coords.get_size(), wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | /*wxFRAME_NO_TASKBAR |*/ wxWANTS_CHARS);
    if (!frm) return false;
    dsgn_frame=frm;
    dsgn = new diagram_designer(cdp, objnum, folder_name, cdp->sel_appl_name, dsgn_frame);
    if (!dsgn) goto err;
    frm->dsgn = dsgn;
  }
  if (dsgn->modifying() && !dsgn->lock_the_object())
    cd_Signalize(cdp);  // must Destroy dsgn because it has not been opened as child of designer_ch
  else if (dsgn->modifying() && !check_not_encrypted_object(cdp, CATEG_DRAWING, objnum)) 
    ;  // must Destroy dsgn because it has not been opened as child of designer_ch  // error signalised inside
  else
    if (dsgn->open(dsgn_frame)==0) 
      { dsgn_frame->SetIcon(wxIcon(_draw_xpm));  dsgn_frame->Show();  return true; } // designer opened OK
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
