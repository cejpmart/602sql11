// tablepanel.cpp
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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#include "tablepanel.h"
#include "controlpanel.h"

#define LINE_END_REGION 14
#define LINE_HIT_TOLERANCE 1
#define LNWD2 2
#define WD2 (WD/2)
#define TABLE_INSERT_STEP 130
#define TABLE_WIDTH 100
#define TABLE_GUTTER_WIDTH 14
#define line_start_cx  14

#include "bmps/key.xpm"
#include "bmps/multi.xpm"
#include "bmps/index.xpm"

table_panel::table_panel(cdp_t pcdpIn, const char * base_schema_nameIn) : pcdp(pcdpIn)
{ dragging=false;
  TrackerPen = new wxPen(*wxBLACK, WD, wxCROSSDIAG_HATCH);
  LinePen = new wxPen(*wxBLACK, 1, wxSOLID);
  key_bitmap = new wxBitmap(key_xpm);
  multi_bitmap = new wxBitmap(multi_xpm);
  index_bitmap = new wxBitmap(index_xpm);
  tables.DeleteContents(true);
  lines.DeleteContents(true);
  drag_key=NULL;
  { wxClientDC dc(wxGetApp().frame);
    dc.SetFont(system_gui_font);
    wxCoord cx, cy;
    dc.GetTextExtent(wxT("Mys"), &cx, &cy);
    table_column_height = cy+2;
    table_caption_height = table_column_height + 4;
  }   
  designer_opened=false;
  strcpy(base_schema_name, base_schema_nameIn);
}

table_panel::~table_panel(void)
{ delete TrackerPen;
  delete LinePen;
  delete key_bitmap;
  delete multi_bitmap;
  delete index_bitmap;
}

void write_shortened_text(wxDC & dc, wxString str, int x, int y, int xspace, int yspace)
{ wxCoord w, h;
  dc.GetTextExtent(str, &w, &h);
  if (w>xspace)
  { int len=(int)str.Length();
    while (len)
    { str=str.Left(--len)+wxT("...");
      dc.GetTextExtent(str, &w, &h);
      if (w<=xspace) break;
    }
  }
  dc.DrawText(str, x, h<yspace ? y+(yspace-h)/2 : y);
}

void table_in_diagram::draw(cdp_t cdp, wxDC & dc, table_panel * panel)
{ const d_table * td = cd_get_table_d(cdp, tbnum, CATEG_TABLE);
  if (td)
  {// main rectangle: 
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(wxBrush(*wxWHITE));
    dc.DrawRectangle(pos.x, pos.y, size.x, size.y);
   // caption rectangle:
    dc.SetBrush(wxBrush(*wxLIGHT_GREY));
    dc.SetPen(*wxLIGHT_GREY);
    dc.DrawRectangle(pos.x+1, pos.y+1, size.x-2, panel->table_caption_height-2);
   // caption text:
    dc.SetFont(system_gui_font);
    dc.SetBackgroundMode(wxTRANSPARENT);
    //dc.SetTextBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    write_shortened_text(dc, wxString(table_name, *cdp->sys_conv), pos.x+2, pos.y+2, size.x-4, panel->table_caption_height-4);
   // columns:
    DiagrIndexList::Node * nodeI;  index_in_table * indx;
    int ay = pos.y + panel->table_caption_height;
    for (int a=first_user_attr(td), posit=0;  a<td->attrcnt;  a++, posit++)
    { if (ay + panel->table_column_height > pos.y+size.y) break;
     // search among indexes:
      for (nodeI = indexes.GetFirst();  nodeI;  nodeI=nodeI->GetNext())
      { indx = nodeI->GetData();
        if (indx->pos_in_win==posit)
          break;
      }
      if (nodeI)
        dc.DrawBitmap(indx->index_type==3 ? *panel->index_bitmap : *panel->key_bitmap, pos.x+2, ay+4, false);
     // write text:
      char attrname[ATTRNAMELEN+1];
      strcpy(attrname, td->attribs[a].name);  wb_small_string(cdp, attrname, false);
      write_shortened_text(dc, wxString(attrname, *cdp->sys_conv), pos.x+TABLE_GUTTER_WIDTH, ay, size.x-TABLE_GUTTER_WIDTH-2, panel->table_column_height-4);
      ay += panel->table_column_height;
    }
    bool first_index=true;
    for (nodeI = indexes.GetFirst();  nodeI;  nodeI=nodeI->GetNext())
    { if (ay + panel->table_column_height > pos.y+size.y) break;
      indx = nodeI->GetData();
      if (indx->pos_in_win>=col_cnt)  // index which is not a column
      { if (first_index)
        { dc.SetPen(*wxBLACK_PEN);
          dc.DrawLine(pos.x, ay, pos.x+size.x, ay);
          first_index=false;
        }
        dc.DrawBitmap(indx->index_type==3 ? *panel->index_bitmap : *panel->key_bitmap, pos.x+2, ay+4, false);
        write_shortened_text(dc, wxString(indx->def, *cdp->sys_conv), pos.x+TABLE_GUTTER_WIDTH, ay, size.x-TABLE_GUTTER_WIDTH-2, panel->table_column_height-4);
        ay += panel->table_column_height;
      }
    }
    release_table_d(td);
  }
}

void table_in_diagram::write(cdp_t cdp, const char * base_schema_name, char * buf)
{ 
  sprintf(buf, "`%s` `%s` %d %d %d %d ", !wb_stricmp(cdp, base_schema_name, schema_name) ? "" : schema_name, 
    table_name, pos.x, pos.y, size.x, size.y);
}

bool read_int(const char *& pstr, int & val)
{ while (*pstr && *pstr<=' ') pstr++;
  bool negative=false;
  if (*pstr=='-') { negative=true;  pstr++; }
  if (*pstr<'0' || *pstr>'9') 
    { if (negative) pstr--;  return false; }
  val=0;
  do
  { val=10*val+(*pstr-'0');
    pstr++;
  } while (*pstr>='0' && *pstr<='9');
  if (negative) val=-val;
  return true;
}

bool read_ident(const char *& pstr, char * ident)
{ while (*pstr && *pstr<=' ') pstr++;
  int len=0;
  if (*pstr=='`')
  { pstr++;
    while (*pstr!='`')
    { if (len>=OBJ_NAME_LEN) return false;
      ident[len++]=*pstr;
      pstr++;
    } 
    pstr++;
  }
  else if (*pstr>='A' && *pstr<='Z')
  { while (*pstr>='A' && *pstr<='Z' || *pstr>='a' && *pstr<='z' || *pstr>='0' && *pstr<='9' || *pstr=='_')
    { if (len>=OBJ_NAME_LEN) return false;
      ident[len++]=*pstr;
      pstr++;
    } 
  }
  else return false;
  ident[len]=0;
  return true;
}

bool is_end(const char *& pstr)
{ while (*pstr && *pstr<=' ') pstr++;
  return !*pstr || *pstr=='}';
}

bool table_in_diagram::read(cdp_t cdp, const char * base_schema_name, const char *& pstr)
{ bool res=read_ident(pstr, schema_name) && read_ident(pstr, table_name) &&
         read_int(pstr, pos.x) && read_int(pstr, pos.y) && read_int(pstr, size.x) && read_int(pstr, size.y);
  if (!*schema_name) strcpy(schema_name, base_schema_name);
  return res;
}

WX_DEFINE_LIST(PointList);

void line_in_diagram::draw(wxDC & dc, wxBitmap * bitmap1, wxBitmap * bitmap2)
{ 
  PointList::Node * node = points.GetFirst();
  if (node)
  { wxPoint * point1 = node->GetData();
   // draw the beginning:
    bool rgt = endtable[0]->pos.x < point1->x;
    if (rgt)
    { dc.DrawLine(point1->x-LNWD2, point1->y-LNWD2, point1->x-LNWD2, point1->y+LNWD2);
      dc.DrawBitmap(*bitmap1, point1->x-line_start_cx+1, point1->y-LNWD2, false);
    }
    else
    { dc.DrawLine(point1->x+LNWD2, point1->y-LNWD2, point1->x+LNWD2, point1->y+LNWD2);
      dc.DrawBitmap(*bitmap1, point1->x+LNWD2+1, point1->y-LNWD2, false);
    }
    do
    { node=node->GetNext();
      if (!node) break;
      wxPoint * point2 = node->GetData();
      if (point1->x==point2->x)  // vertical line
        if (point1->y<point2->y)  // vertical line
        { dc.DrawLine(point1->x-LNWD2, point1->y-LNWD2, point2->x-LNWD2, point2->y+LNWD2);
          dc.DrawLine(point1->x+LNWD2, point1->y-LNWD2, point2->x+LNWD2, point2->y+LNWD2);
        }
        else
        { dc.DrawLine(point1->x-LNWD2, point1->y+LNWD2, point2->x-LNWD2, point2->y-LNWD2);
          dc.DrawLine(point1->x+LNWD2, point1->y+LNWD2, point2->x+LNWD2, point2->y-LNWD2);
        }
      else  // horizontal line
        if (point1->x<point2->x)
        { dc.DrawLine(point1->x-LNWD2, point1->y-LNWD2, point2->x+LNWD2, point2->y-LNWD2);
          dc.DrawLine(point1->x-LNWD2, point1->y+LNWD2, point2->x+LNWD2, point2->y+LNWD2);
        }
        else
        { dc.DrawLine(point1->x+LNWD2, point1->y-LNWD2, point2->x-LNWD2, point2->y-LNWD2);
          dc.DrawLine(point1->x+LNWD2, point1->y+LNWD2, point2->x-LNWD2, point2->y+LNWD2);
        }
      point1=point2;
    } while (true);
    rgt = endtable[1]->pos.x < point1->x;
    if (rgt)
    { dc.DrawLine(point1->x-LNWD2, point1->y-LNWD2, point1->x-LNWD2, point1->y+LNWD2);
      dc.DrawBitmap(*bitmap2, point1->x-line_start_cx+1, point1->y-LNWD2, false);
    }
    else
    { dc.DrawLine(point1->x+LNWD2, point1->y-LNWD2, point1->x+LNWD2, point1->y+LNWD2);
      dc.DrawBitmap(*bitmap2, point1->x+LNWD2+1, point1->y-LNWD2, false);
    }
  }
}

diagram_hit_type line_in_diagram::line_hit_test(int x, int y, int & itempart)
{ PointList::Node * node = points.GetFirst();
  if (node)
  { wxPoint * point1 = node->GetData();
    itempart=0;
    do
    { node=node->GetNext();
      if (!node) break;
      wxPoint * point2 = node->GetData();
      int xmin, xmax, ymin, ymax;
      bool horizontal = point1->y==point2->y;
      if (point1->x<point2->x) { xmin=point1->x;  xmax=point2->x; } else { xmin=point2->x;  xmax=point1->x; } 
      if (point1->y<point2->y) { ymin=point1->y;  ymax=point2->y; } else { ymin=point2->y;  ymax=point1->y; } 
      if (x>=xmin-LINE_HIT_TOLERANCE && x<=xmax+LINE_HIT_TOLERANCE && y>=ymin-LINE_HIT_TOLERANCE && y<=ymax+LINE_HIT_TOLERANCE)
      {// test for edges:
        if (node->GetNext())
        { wxPoint * point3 = node->GetNext()->GetData();
          if (x>=point2->x-LINE_END_REGION && x<=point2->x+LINE_END_REGION && y>=point2->y-LINE_END_REGION && y<=point2->y+LINE_END_REGION)
          { bool grow1 = horizontal ? 
              point1->x > point2->x : point1->y > point2->y;
            bool grow2 = point2->y==point3->y ? 
              point3->x > point2->x : point3->y > point2->y;
            return grow1==grow2 ? DG_HIT_LINEEND_NWSE : DG_HIT_LINEEND_NESW;
          }
        }
        return horizontal ? DG_HIT_LINE_NS : DG_HIT_LINE_WE;
      }
      point1=point2;  itempart++;
    } while (true);
  }
  return DG_HIT_NONE;
}

WX_DEFINE_LIST(DiagrTableList);
WX_DEFINE_LIST(DiagrLineList);


diagram_hit_type table_panel::HitTest(int x, int y, int & item, int & itempart)
{ int xx, yy;
  CalcUnscrolledPosition(x, y, &xx, &yy);  // device -> logical
 // search among tables (before lines):
  item=0;
  for (DiagrTableList::Node * nodeT = tables.GetFirst();  nodeT;  nodeT=nodeT->GetNext(), item++)
  { table_in_diagram * table = nodeT->GetData();
    if (xx>=table->pos.x && xx<table->pos.x+table->size.x &&
        yy>=table->pos.y && yy<=table->pos.y+table->size.y+LINE_HIT_TOLERANCE)
    { if (yy-table->pos.y < table_caption_height) return DG_HIT_TABLE;  // table caption
      if (yy-table->pos.y >= table->size.y-LINE_HIT_TOLERANCE) return DG_HIT_TABLE_END;  // table bottom line
      itempart = (yy-table->pos.y-table_caption_height) / table_column_height;
      return DG_HIT_COLUMN;
    }
  }
 // search among lines:
  item=0;
  for (DiagrLineList::Node * node = lines.GetFirst();  node;  node=node->GetNext(), item++)
  { line_in_diagram * line = node->GetData();
    diagram_hit_type lht = line->line_hit_test(xx, yy, itempart);
    if (lht!=DG_HIT_NONE) return lht;
  }
  return DG_HIT_NONE;
}

//void diagram_designer::OnErase(wxEraseEvent & event)
// No action: implemented to prevent flickering
//{ event.GetDC(); }

void table_panel::OnDraw(wxDC& dc)
{
//void table_panel::OnPaint(wxPaintEvent& event)
//{ wxPaintDC dc(this);
  for (DiagrLineList::Node * node = lines.GetFirst();  node;  node=node->GetNext())
  { line_in_diagram * line = node->GetData();
    dc.SetPen(*LinePen);
    line->draw(dc, line->ending_type[0]==ET_ONE ? key_bitmap : multi_bitmap, line->ending_type[1]==ET_ONE ? key_bitmap : multi_bitmap);
    dc.SetPen(wxNullPen);
  }
  for (DiagrTableList::Node * nodeT = tables.GetFirst();  nodeT;  nodeT=nodeT->GetNext())
  { table_in_diagram * table = nodeT->GetData();
    table->draw(pcdp, dc, this);
  }
  if (tables.GetCount()==0)  // empty design, write help text
  { dc.SetFont(system_gui_font);
    dc.DrawText(_("Drop or paste tables here"), 10, 4);
  }
}

class TablePanelDropTarget : public wxDropTarget
{ C602SQLDataObject * my_data_object;
  table_panel    *m_Owner;
  wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
  wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
public:
  TablePanelDropTarget(table_panel *Owner)
  { m_Owner  = Owner;
    my_data_object = new C602SQLDataObject();
    SetDataObject(my_data_object);
  }
};

wxDragResult TablePanelDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{ bool ok=false;
  C602SQLObjects * m_Objs = CCPDragSource::DraggedObjects();
  if (!my_stricmp(m_Objs->m_Server, m_Owner->pcdp->locid_server_name)) 
    for (unsigned i = 0; i < m_Objs->m_Count; i++)
      if (m_Objs->m_Obj[i].m_Categ == CATEG_TABLE) ok=true;
  return ok ? def : wxDragNone;
}

bool table_panel::insert_tables(int x, int y, C602SQLObjects * m_Objs, bool real_paste)
{ bool any_pasted = false;
  for (unsigned i = 0; i < m_Objs->m_Count; i++)
  { C602SQLObjName *on = &m_Objs->m_Obj[i];
    //if (!ol.Find(on->m_ObjName, on->m_Categ)) continue;
   // IF table
    if (on->m_Categ == CATEG_TABLE)
    { if (!contains_table(on->m_ObjNum))
      { if (real_paste)
        { table_in_diagram * tid = new table_in_diagram;
          if (!tid) continue;
          tid->tbnum=on->m_ObjNum;
          strcpy(tid->table_name, on->m_ObjName);
          tid->pos.x=x;  tid->pos.y=y;  x+=TABLE_INSERT_STEP;
          tid->size.x=TABLE_WIDTH;
          if (tid->get_table_info(pcdp, this))
          { tables.Append(tid);
            add_table_lines(tid);
          }
          else delete tid;
        }
        any_pasted=true;
      } // table is not in the diagram
    } // table in the clipboard
  } // cycle on objects in the clipboard
  return any_pasted;
}

wxDragResult TablePanelDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{ int xx, yy;
  m_Owner->CalcUnscrolledPosition(x, y, &xx, &yy);  // device -> logical
  GetData();
  C602SQLObjects * m_Objs = my_data_object->GetObjects();
  if (my_stricmp(m_Objs->m_Server, m_Owner->pcdp->locid_server_name))
    { wxBell();  return wxDragNone; }
  if (!m_Owner->insert_tables(xx, yy, m_Objs, true))
    { wxBell();  return wxDragNone; }
  m_Owner->Refresh();  
  m_Owner->set_changed(); 
  return wxDragCopy;
}

void table_panel::SetDragCursor(diagram_hit_type dght)
{ wxStockCursor new_current_cursor;
  new_current_cursor = dght==DG_HIT_LINE_NS      ? wxCURSOR_SIZENS   : 
                       dght==DG_HIT_LINE_WE      ? wxCURSOR_SIZEWE   :
                       dght==DG_HIT_LINEEND_NESW ? wxCURSOR_SIZENESW :
                       dght==DG_HIT_LINEEND_NWSE ? wxCURSOR_SIZENWSE : 
                       dght==DG_HIT_TABLE        ? wxCURSOR_SIZING   :
                       dght==DG_HIT_TABLE_END    ? wxCURSOR_SIZENS   : wxCURSOR_ARROW;
  if (new_current_cursor!=current_cursor)
  { current_cursor=new_current_cursor;
    SetCursor(wxCursor(new_current_cursor));
  }
}

bool add_refint(cdp_t cdp, table_in_diagram * table1, table_in_diagram * table2, const char * index_text1, const char * index_text2)
{ char cmd[6*OBJ_NAME_LEN+200];  bool res=true;

 // check if it already exists (prevents creating duplicate integrity rules):
  tcursnum cursnum;  trecnum cnt, rec;  uns32 len;
  bool found=false;
  sprintf(cmd, "select * from _iv_foreign_keys where schema_name=\'%s\' and table_name=\'%s\' and for_schema_name=\'%s\' and for_table_name=\'%s\'", 
     table1->schema_name, table1->table_name, table2->schema_name, table2->table_name);
  if (!cd_Open_cursor_direct(cdp, cmd, &cursnum))
  { cd_Rec_cnt(cdp, cursnum, &cnt);
    for (rec=0;  rec<cnt;  rec++)
    { char * def1=load_blob(cdp, cursnum, rec, 4, NOINDEX, &len);
      char * def2=load_blob(cdp, cursnum, rec, 7, NOINDEX, &len);
      cutspaces(def1);  cutspaces(def2);
      if (def1 && def2) 
        if (!my_stricmp(index_text1, def1) && !my_stricmp(index_text2, def2))
          found=true;
      corefree(def1);  corefree(def2);
    }
    cd_Close_cursor(cdp, cursnum);
  }
  if (found)
    { error_box(_("The same foreign key already exists."));  return false; }

 // add the constrain:
  sprintf(cmd, "ALTER TABLE `%s`.`%s` PRESERVE_FLAGS ADD FOREIGN KEY (%s) REFERENCES `%s`.`%s`(%s)", 
    table1->schema_name, table1->table_name, index_text1,
    table2->schema_name, table2->table_name, index_text2);
  uns32 optval; 
  cd_Get_sql_option(cdp, &optval);
  cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, 0);  // new ALTER TABLE selected
  if (cd_SQL_execute(cdp, cmd, NULL))
    { cd_Signalize(cdp);  res=false; }
  cd_Set_sql_option(cdp, SQLOPT_OLD_ALTER_TABLE, optval);  // option restored
  return res;
}

void table_panel::OnMouseEvent(wxMouseEvent& event)
{ int item, itempart;
    int x = (int)event.GetX(),
        y = (int)event.GetY();
    // with wxSP_LIVE_UPDATE style the splitter windows are always resized
    // following the mouse movement while it drags the sash, without it we only
    // draw the sash at the new position but only resize the windows when the
    // dragging is finished
  bool isLive = false; //(GetWindowStyleFlag() & wxSP_LIVE_UPDATE) != 0;
  if (event.LeftDown())
  { diagram_hit_type dght = HitTest(x, y, item, itempart);
    if (dght==DG_HIT_LINE_NS || dght==DG_HIT_LINE_WE || dght==DG_HIT_LINEEND_NESW || dght==DG_HIT_LINEEND_NWSE)
    { dragging=true;  drag_item=item;  drag_itempart=itempart;  drag_dght=dght;
      CaptureMouse();
      SetDragCursor(dght);
      if ( !isLive )
      { // remember the initial sash position and draw the initial shadow sash
               //m_sashPositionCurrent = m_sashPosition;
        DrawLineTracker(x, y);
      }
      m_oldX = x;  m_oldY = y;
    }
    else if (dght==DG_HIT_TABLE)
    { dragging=true;  drag_item=item;  drag_dght=dght;
      CaptureMouse();
      int xx, yy;
      CalcUnscrolledPosition(x, y, &xx, &yy);  // device -> logical
      table_in_diagram * table = tables.Item(item)->GetData();
      m_diffx = xx-table->pos.x;  m_diffy = yy-table->pos.y;
      DrawTableTracker(x, y);
      m_oldX = x;  m_oldY = y;
    }
    else if (dght==DG_HIT_TABLE_END)
    { dragging=true;  drag_item=item;  drag_dght=dght;
      CaptureMouse();
      int xx, yy;
      CalcUnscrolledPosition(x, y, &xx, &yy);  // device -> logical
      table_in_diagram * table = tables.Item(item)->GetData();
      m_diffx = xx-table->pos.x;  m_diffy = yy-table->pos.y;
      DrawTableBottomTracker(x, y);
      m_oldX = x;  m_oldY = y;
    }
    else if (dght==DG_HIT_COLUMN)
    { table_in_diagram * table = tables.Item(item)->GetData();
      const d_table * td = cd_get_table_d(pcdp, table->tbnum, CATEG_TABLE);
      if (!td) return;
      const char * text = itempart<table->col_cnt ? td->attribs[first_user_attr(td)+itempart].name : table->index_text(itempart);
      drag_key = new wxDragImage(wxString(text, *pcdp->sys_conv), /**wxSTANDARD_CURSOR*/wxNullCursor);
      release_table_d(td);
      if (!drag_key) return;
      dragging=true;  drag_item=item;  drag_itempart=itempart;  drag_dght=dght;
      drag_key->BeginDrag(wxPoint(16,0), this, this);
      drag_key->Show();
    }
  }
  else if (event.Moving() && !event.Dragging())
  { SetDragCursor(HitTest(x, y, item, itempart));
  }
  else if (event.LeftUp() && dragging)
  { dragging=false;
    if (drag_dght==DG_HIT_COLUMN && drag_key)
    { diagram_hit_type dght = HitTest(x, y, item, itempart);
      drag_key->EndDrag();
      delete drag_key;
      drag_key=NULL;
      current_cursor=wxCURSOR_ARROW;  SetCursor(wxCursor(current_cursor));  // restore the cursor
      if (dght==DG_HIT_COLUMN)
      { table_in_diagram * table1 = tables.Item(drag_item)->GetData();
        table_in_diagram * table2 = tables.Item(item)->GetData();
        if (table1!=table2)
        { const d_table * td1 = cd_get_table_d(pcdp, table1->tbnum, CATEG_TABLE);
          const d_table * td2 = cd_get_table_d(pcdp, table2->tbnum, CATEG_TABLE);
          if (!td1 || !td2) return;
          int itp1 = table1->is_index(drag_itempart);
          int itp2 = table2->is_index(itempart);
          wxString msg;
          wxWindow * parent;  // like dialog_parent but this method is not accessible here
          if (global_style!=ST_POPUP) parent = wxGetApp().frame;
          else parent = GetParent();
          if (!itp1 || !itp2) 
          { msg.Printf(_("Selected column %s in table %s is not indexed"), 
              wxString(itp1==0 ? td1->attribs[first_user_attr(td1)+drag_itempart].name : td2->attribs[first_user_attr(td2)+itempart].name, *pcdp->sys_conv).c_str(),
              wxString(itp1==0 ? table1->table_name : table2->table_name, *pcdp->sys_conv).c_str());
            error_box(msg, parent);
          }
          else if (itp1==3 && itp2==3) 
            error_box(_("Both indexes are non-unique."), parent);
          else if (itp1<3)  // table1 is master
          { msg.Printf(_("Add the foreign key %s\nreferencing %s in table %s\nto table %s?"), 
              wxString(table2->index_text(itempart), *pcdp->sys_conv).c_str(), 
              wxString(table1->index_text(drag_itempart), *pcdp->sys_conv).c_str(), 
              wxString(table1->table_name, *pcdp->sys_conv).c_str(), 
              wxString(table2->table_name, *pcdp->sys_conv).c_str());
            wx602MessageDialog dlg(parent, msg, _("Adding referential integrity"), wxYES_NO);
            if (dlg.ShowModal()==wxID_YES)
              if (add_refint(pcdp, table2, table1, table2->index_text(itempart), table1->index_text(drag_itempart)))
                { Refresh_tables_and_lines();  Refresh(); }

          }
          else if (itp2<3)  // table2 is master
          { msg.Printf(_("Add the foreign key %s\nreferencing %s in table %s\nto table %s?"), 
              wxString(table1->index_text(drag_itempart), *pcdp->sys_conv).c_str(), 
              wxString(table2->index_text(itempart), *pcdp->sys_conv).c_str(), 
              wxString(table2->table_name, *pcdp->sys_conv).c_str(), 
              wxString(table1->table_name, *pcdp->sys_conv).c_str());
            wx602MessageDialog dlg(parent, msg, _("Adding referential integrity"), wxYES_NO);
            if (dlg.ShowModal()==wxID_YES)
              if (add_refint(pcdp, table1, table2, table1->index_text(drag_itempart), table2->index_text(itempart)))
                { Refresh_tables_and_lines();  Refresh(); }
          }
          release_table_d(td1);
          release_table_d(td2);
        }
      }
      else
        Refresh();  // otherwise the drag cursor remains visible in the drop position on GTK
    }
    else  // dragging implemented locally
    {// stop dragging 
      ReleaseMouse();
      SetDragCursor(DG_HIT_NONE);
     // prevent draging outside the client area:
      if ( (short)x < 0 ) x = 0;
      if ( (short)y < 0 ) y = 0;
      int width, height;
      GetClientSize(&width, &height);
      if (x>width ) x=width;
      if (y>height) y=height;
     // Erase old tracker and move the object:
      if (drag_dght==DG_HIT_TABLE)
      { DrawTableTracker(m_oldX, m_oldY);
        MoveDraggedTable(x, y);
      }
      else if (drag_dght==DG_HIT_TABLE_END)
      { DrawTableBottomTracker(m_oldX, m_oldY);
        SizeDraggedTable(x, y);
      }
      else // dragging a line
      { if ( !isLive )
          DrawLineTracker(m_oldX, m_oldY);
        MoveDraggedLine(x, y);
      }
    }
  }  // left up && dragging
  else if (event.Dragging() && dragging)
  { if (drag_dght==DG_HIT_COLUMN && drag_key)
    { drag_key->Move(wxPoint(x,y));
      diagram_hit_type dght = HitTest(x, y, item, itempart);
      if (dght==DG_HIT_COLUMN && item!=drag_item)
      { table_in_diagram * table1 = tables.Item(drag_item)->GetData();
        table_in_diagram * table2 = tables.Item(item)->GetData();
        if (table1->is_index(drag_itempart) && table2->is_index(itempart))
             current_cursor=wxCURSOR_ARROW;
        else current_cursor=wxCURSOR_NO_ENTRY;
      }
      else current_cursor=wxCURSOR_NO_ENTRY;
      SetCursor(wxCursor(current_cursor));
    }
    else
    { bool diff = drag_dght==DG_HIT_LINE_WE ? x!=m_oldX : drag_dght==DG_HIT_LINE_WE ? y!=m_oldY : x!=m_oldX || y!=m_oldY;
      if (!diff) // nothing to do, mouse didn't really move far enough
        return;
     // Erase old tracker
      if (drag_dght==DG_HIT_TABLE)
        DrawTableTracker(m_oldX, m_oldY);
      else if (drag_dght==DG_HIT_TABLE_END)
        DrawTableBottomTracker(m_oldX, m_oldY);
      else if ( !isLive )
        DrawLineTracker(m_oldX, m_oldY);
     // Remember old positions
      m_oldX = x;
      m_oldY = y;
  #ifdef __WXMSW__
          // As we captured the mouse, we may get the mouse events from outside
          // our window - for example, negative values in x, y. This has a weird
          // consequence under MSW where we use unsigned values sometimes and
          // signed ones other times: the coordinates turn as big positive
          // numbers and so the sash is drawn on the *right* side of the window
          // instead of the left (or bottom instead of top). Correct this.
          if ( (short)m_oldX < 0 )
              m_oldX = 0;
          if ( (short)m_oldY < 0 )
              m_oldY = 0;
  #endif // __WXMSW__
     // prevent draging outside the client area - same as above:
      int width, height;
      GetClientSize(&width, &height);
      if (m_oldX>width ) m_oldX=width;
      if (m_oldY>height) m_oldY=height;
     // Draw new one 
      if (drag_dght==DG_HIT_TABLE)
        DrawTableTracker(m_oldX, m_oldY);
      else if (drag_dght==DG_HIT_TABLE_END)
        DrawTableBottomTracker(m_oldX, m_oldY);
      else if ( !isLive )
        DrawLineTracker(m_oldX, m_oldY);
    }
  }
  else if ( event.LeftDClick())
  {
  }
  else if (event.RightDown())
  { wxMenu popup_menu;
    diagram_hit_type dght = HitTest(x, y, item, itempart);
    if (dght==DG_HIT_NONE)
    { AppendXpmItem(&popup_menu, TD_CPM_PASTE_TABLES, _("&Paste Table(s)"), Vlozit_xpm);
      AppendXpmItem(&popup_menu, TD_CPM_REFRESH, _("&Refresh"), refrpanel_xpm);
      popup_menu.Enable(TD_CPM_PASTE_TABLES, PasteTables(0, 0, false));
      PopupMenu(&popup_menu, ScreenToClient(wxGetMousePosition()));
    }
    else if (dght==DG_HIT_TABLE || dght==DG_HIT_COLUMN)
    { AppendXpmItem(&popup_menu, TD_CPM_ALTER_TABLE,  _("&Edit Table"),  modify_xpm);
      AppendXpmItem(&popup_menu, TD_CPM_REMOVE_TABLE, _("&Remove Table from Diagram"), delete_xpm);
      drag_item=item;  drag_dght=dght;
      PopupMenu(&popup_menu, ScreenToClient(wxGetMousePosition()));
    }
    else if (dght==DG_HIT_LINE_NS || dght==DG_HIT_LINE_WE || dght==DG_HIT_LINEEND_NESW || dght==DG_HIT_LINEEND_NWSE)
    { AppendXpmItem(&popup_menu, TD_CPM_REMOVE_CONSTR, _("&Remove Constrain"), delete_xpm);
      drag_item=item;  drag_dght=dght;
      PopupMenu(&popup_menu, ScreenToClient(wxGetMousePosition()));
    }
    // no action on table bottom
  }
}

void table_panel::MoveDraggedLine(int x, int y)
// x, y is in device coordinates
{ int orig_y;  // used only when breaking the line
  int xmin, xmax, ymin, ymax, xx, yy;
  CalcUnscrolledPosition(x, y, &xx, &yy);  // device -> logical
  xmin=xmax=xx;  ymin=ymax=yy;

  line_in_diagram * line = lines.Item(drag_item)->GetData();
  if (line->points.GetCount()<=2) return;  // cannot drag
  wxPoint * point1 = line->points.Item(drag_itempart)->GetData();
  wxPoint * point2 = line->points.Item(drag_itempart+1)->GetData();
  if (xmin>point1->x) xmin=point1->x; else if (xmax<point1->x) xmax=point1->x;
  if (ymin>point1->y) ymin=point1->y; else if (ymax<point1->y) ymax=point1->y;
  if (xmin>point2->x) xmin=point2->x; else if (xmax<point2->x) xmax=point2->x;
  if (ymin>point2->y) ymin=point2->y; else if (ymax<point2->y) ymax=point2->y;
  wxPoint * point3 = NULL;
  if (drag_dght==DG_HIT_LINEEND_NESW || drag_dght==DG_HIT_LINEEND_NWSE)
  { point3 = line->points.Item(drag_itempart+2)->GetData();
    if (xmin>point3->x) xmin=point3->x; else if (xmax<point3->x) xmax=point3->x;
    if (ymin>point3->y) ymin=point3->y; else if (ymax<point3->y) ymax=point3->y;
  }
  bool horiz1 = point1->y==point2->y;
  bool horiz2 = point3 && point2->y==point3->y;
  bool breaking_the_line=true;
  if      (          drag_itempart==0                        ) orig_y = point1->y; // dragging the beginning part (only or with the next part)
  else if (          drag_itempart==line->points.GetCount()-2) orig_y = point2->y; // dragging the end part (only)
  else if (point3 && drag_itempart==line->points.GetCount()-3) orig_y = point3->y; // dragging the end part with the prevoius part
  else breaking_the_line=false;
 // move point1:
  if (horiz1) // horizontal line
    point1->y=yy;
  else
    point1->x=xx;
 // move point2:
  if (!point3)
    if (horiz1) // horizontal line
      point2->y=yy;
    else
      point2->x=xx;
  else
  { point2->y=yy;
    point2->x=xx;
    if (horiz2) // horizontal line
      point3->y=yy;
    else
      point3->x=xx;
  }
 // insert additional point if breaking the line:
  if (breaking_the_line)
  { wxPoint * pta, * ptb;
    if (drag_itempart==0)  // breaking the beginning
    { int div = (point1->x+point2->x)/2;
      pta = new wxPoint(point1->x, orig_y);
      ptb = new wxPoint(div, orig_y);
      point1->x = div;
      line->points.Insert(ptb);
      line->points.Insert(pta);
    }
    else  // breaking the end
    {  if (point3)
      { int div = (point2->x+point3->x)/2;
        pta = new wxPoint(point3->x, orig_y);
        ptb = new wxPoint(div, orig_y);
        point3->x = div;
      }
      else
      { int div = (point1->x+point2->x)/2;
        pta = new wxPoint(point2->x, orig_y);
        ptb = new wxPoint(div, orig_y);
        point2->x = div;
      }
      line->points.Append(ptb);
      line->points.Append(pta);
    } 
  }
  else if (!point3)  // check for changing sides
  { 
    if (drag_itempart==1)
    { wxPoint * point0 = line->points.Item(0)->GetData();
      if (point1->x>line->endtable[0]->pos.x+line->endtable[0]->size.x+line_start_cx && point0->x<line->endtable[0]->pos.x)
        point0->x=line->endtable[0]->pos.x+line->endtable[0]->size.x+line_start_cx;
      else if (point0->x>=line->endtable[0]->pos.x && point1->x<line->endtable[0]->pos.x-line_start_cx)
        point0->x=line->endtable[0]->pos.x-line_start_cx;
    }
    if (drag_itempart==line->points.GetCount()-3)
    { wxPoint * point0 = line->points.Item(line->points.GetCount()-1)->GetData();
      if (point2->x>line->endtable[1]->pos.x+line->endtable[1]->size.x+line_start_cx && point0->x<line->endtable[1]->pos.x)
        point0->x=line->endtable[1]->pos.x+line->endtable[1]->size.x+line_start_cx;
      else if (point0->x>=line->endtable[1]->pos.x && point2->x<line->endtable[1]->pos.x-line_start_cx)
        point0->x=line->endtable[1]->pos.x-line_start_cx;
    }
  }
 // remove 0-length line parts:
  for (int i=1;  i<line->points.GetCount()-2;  i++)
  { if (line->points.GetCount() <= 4) break;  // every line must have at least 3 parts (axiom)
    wxPoint * pt1, * pt2;
    pt1 = line->points.Item(i  )->GetData();
    pt2 = line->points.Item(i+1)->GetData();
    if (pt1->x==pt2->x && pt1->y==pt2->y)
      { line->points.DeleteNode(line->points.Item(i));  line->points.DeleteNode(line->points.Item(i)); }
  }
 // RefreshRect needs device coords:
  CalcScrolledPosition(xmin, ymin, &xmin, &ymin);  // logical -> device
  CalcScrolledPosition(xmax, ymax, &xmax, &ymax);  // logical -> device
  wxRect rect(xmin-WD2, ymin-WD2, xmax-xmin+WD+1, ymax-ymin+WD+1);
  RefreshRect(rect);
  set_changed();
}

void table_panel::MoveDraggedTable(int x, int y)
// x, y is in device coordinates
{ table_in_diagram * table = tables.Item(drag_item)->GetData();
  int xmin, xmax, ymin, ymax, xx, yy;
  xmin=xmax=table->pos.x;  ymin=ymax=table->pos.y;
 // get the new position:
  CalcUnscrolledPosition(x-m_diffx, y-m_diffy, &xx, &yy);  // device -> logical
 // change the position of the table:
  if (xx<xmin) xmin=xx; else if (xx>xmax) xmax=xx;
  if (yy<ymin) ymin=yy; else if (yy>ymax) ymax=yy;
  int origx = table->pos.x, origy = table->pos.y;
  table->pos.x=xx;  table->pos.y=yy;
 // update all lines connected to the table:
  bool any_line = false;
  for (DiagrLineList::Node * node = lines.GetFirst();  node;  node = node->GetNext())
  { line_in_diagram * line = node->GetData();
    wxPoint * pt1, * pt2;
    int cnt = (int)line->points.GetCount();
   // find points which will be updated:
    if (line->endtable[0]==table)
    { pt1 = line->points.Item(0)->GetData();
      pt2 = line->points.Item(1)->GetData();
    }
    else if (line->endtable[1]==table)
    { pt1 = line->points.Item(cnt-1)->GetData();
      pt2 = line->points.Item(cnt-2)->GetData();
    }
    else continue;
   // update the points:
    if (cnt>2 && pt2->x > xx+table->size.x+line_start_cx)
    { pt1->x=xx+table->size.x+line_start_cx;
      pt1->y += (yy-origy);
      pt2->y += (yy-origy);
    }
    else if (cnt>2 && pt2->x < xx-line_start_cx)
    { pt1->x=xx-line_start_cx;
      pt1->y += (yy-origy);
      pt2->y += (yy-origy);
    }
    else // delete and re-create the line
    { node->SetData(NULL); // inconsistent, but used by
      line_in_diagram * new_line = create_connecting_line(line->endtable[0], line->pos1, line->endtable[1], line->pos2, line->ending_type[0], line->ending_type[1]);
      node->SetData(new_line);
      delete line;
    }
    any_line=true;
  }
  update_virtual_size(table);
 // RefreshRect needs device coords:
  if (any_line) Refresh();
  else
  { CalcScrolledPosition(xmin, ymin, &xmin, &ymin);  // logical -> device
    CalcScrolledPosition(xmax, ymax, &xmax, &ymax);  // logical -> device
    if (xmin<0) xmin=0;  if (ymin<0) ymin=0;  // wrong update when coordinates are negative
    wxRect rect(xmin, ymin, xmax+table->size.x, ymax+table->size.y);
    RefreshRect(rect);
  }
  set_changed();
}

void table_panel::SizeDraggedTable(int x, int y)
// x, y is in device coordinates
{ table_in_diagram * table = tables.Item(drag_item)->GetData();
  int xx, yy;
 // get the moved position topleft corner to xx, yy :
  CalcUnscrolledPosition(x-m_diffx, y-m_diffy, &xx, &yy);  // device -> logical
 // change the size of the table:
  int origysize = table->size.y;
  int diffsize = yy - table->pos.y;
  table->size.y += diffsize;
  if (table->size.y < table_caption_height+table_column_height) table->size.y = table_caption_height+table_column_height;
  if (table->size.y > table->maxsizey) table->size.y = table->maxsizey;
 // update all lines connected to the table:
  bool any_line = false;
  for (DiagrLineList::Node * node = lines.GetFirst();  node;  node = node->GetNext())
  { line_in_diagram * line = node->GetData();
    wxPoint * pt1, * pt2;
    int cnt = (int)line->points.GetCount();
   // find points which will be updated:
    if (line->endtable[0]==table)
    { pt1 = line->points.Item(0)->GetData();
      pt2 = line->points.Item(1)->GetData();
    }
    else if (line->endtable[1]==table)
    { pt1 = line->points.Item(cnt-1)->GetData();
      pt2 = line->points.Item(cnt-2)->GetData();
    }
    else continue;
   // update the points:
    if (cnt>2 && pt1->y > table->pos.y+table->size.y)
    { pt1->y =  table->pos.y+table->size.y;
      pt2->y =  table->pos.y+table->size.y;
      line->displaced=true;
    }
    else if (line->displaced && pt1->y == table->pos.y+origysize && pt1->y < table->pos.y+table->size.y)// delete and re-create the line
    { node->SetData(NULL); // inconsistent, but used by
      line_in_diagram * new_line = create_connecting_line(line->endtable[0], line->pos1, line->endtable[1], line->pos2, line->ending_type[0], line->ending_type[1]);
      node->SetData(new_line);
      delete line;
    }
    any_line=true;
  }
  update_virtual_size(table);
 // RefreshRect needs device coords:
  if (any_line) Refresh();
  else
  { int maxy = table->size.y > origysize ? table->size.y : origysize;
    wxRect rect(table->pos.x, table->pos.y, table->pos.x+table->size.x, table->pos.y+maxy);
    RefreshRect(rect);
  }
  set_changed();
}

void table_panel::DrawLineTracker0(wxDC & screenDC, int x1, int y1, int x2, int y2, int x, int y, bool two)
// When [two]==true then (x,y) updates (x2, y2) in both coords.
{ if (x1==x2) 
  { x1=x2=x;
    if (two) y2=y;
  }
  else        
  { y1=y2=y;
    if (two) x2=x;
  }
  ClientToScreen(&x1, &y1);
  ClientToScreen(&x2, &y2);
  screenDC.DrawLine(x1, y1, x2, y2);
}
 
void table_panel::DrawLineTracker(int x, int y)
// x, y is in device coordinates
{
  int x1, y1, x2, y2, x3, y3;
  line_in_diagram * line = lines.Item(drag_item)->GetData();
  wxPoint * point1 = line->points.Item(drag_itempart)->GetData();
  wxPoint * point2 = line->points.Item(drag_itempart+1)->GetData();
  CalcScrolledPosition(point1->x, point1->y, &x1, &y1);  // logical -> device
  CalcScrolledPosition(point2->x, point2->y, &x2, &y2);  // logical -> device
  wxPoint * point3 = NULL;
  if (drag_dght==DG_HIT_LINEEND_NESW || drag_dght==DG_HIT_LINEEND_NWSE)
  { point3 = line->points.Item(drag_itempart+2)->GetData();
    CalcScrolledPosition(point3->x, point3->y, &x3, &y3);  // logical -> device
  }
  wxScreenDC screenDC;
  screenDC.SetLogicalFunction(wxINVERT);
  screenDC.SetPen(*TrackerPen);
  screenDC.SetBrush(*wxTRANSPARENT_BRUSH);
  DrawLineTracker0(screenDC, x1, y1, x2, y2, x, y, point3!=NULL);
  if (point3) DrawLineTracker0(screenDC, x3, y3, x2, y2, x, y, true);
  screenDC.SetLogicalFunction(wxCOPY);
}

void table_panel::DrawTableTracker(int x, int y)
// x, y is in device coordinates
{ table_in_diagram * table = tables.Item(drag_item)->GetData();
 // CalcScrolledPosition(table->pos.x, table->pos.y, &tx, &ty);  // logical -> device
  int tx = x-m_diffx,  ty = y-m_diffy;
  wxScreenDC screenDC;
  screenDC.SetLogicalFunction(wxINVERT);
  screenDC.SetPen(*TrackerPen);
  screenDC.SetBrush(*wxTRANSPARENT_BRUSH);
  ClientToScreen(&tx, &ty);
  screenDC.DrawLine(tx,               ty,               tx+table->size.x, ty);
  screenDC.DrawLine(tx+table->size.x, ty,               tx+table->size.x, ty+table->size.y);
  screenDC.DrawLine(tx+table->size.x, ty+table->size.y, tx,               ty+table->size.y);
  screenDC.DrawLine(tx,               ty+table->size.y, tx,               ty);
  screenDC.SetLogicalFunction(wxCOPY);
}

void table_panel::DrawTableBottomTracker(int x, int y)
// x, y is in device coordinates
{ table_in_diagram * table = tables.Item(drag_item)->GetData();
 // CalcScrolledPosition(table->pos.x, table->pos.y, &tx, &ty);  // logical -> device
  int tx = x-m_diffx,  ty = y-m_diffy;
  wxScreenDC screenDC;
  screenDC.SetLogicalFunction(wxINVERT);
  screenDC.SetPen(*TrackerPen);
  screenDC.SetBrush(*wxTRANSPARENT_BRUSH);
  ClientToScreen(&tx, &ty);
  screenDC.DrawLine(tx+table->size.x, ty+table->size.y, tx, ty+table->size.y);
  screenDC.SetLogicalFunction(wxCOPY);
}

bool table_panel::contains_table(ttablenum tbnum, int * index)
{ int item=0;
  for (DiagrTableList::Node * nodeT = tables.GetFirst();  nodeT;  nodeT=nodeT->GetNext(), item++)
  { table_in_diagram * table = nodeT->GetData();
    if (table->tbnum==tbnum)
    { if (index!=NULL) *index=item;
      return true;  
    }
  }
  return false;
}

int table_panel::find_free_ypos(table_in_diagram * table, int pos, bool right_side)
{ int xpos = right_side ? table->pos.x+table->size.x+line_start_cx : table->pos.x-line_start_cx;
  int ypos = pos < 0 ? table->pos.y + 2 : table->pos.y + table_caption_height + table_column_height*pos + 2;
  if (ypos>table->pos.y+table->size.y-1) ypos=table->pos.y+table->size.y-1;
  do
  { bool conflict = false;  wxPoint * pt;
    for (DiagrLineList::Node * node = lines.GetFirst();  node;  node=node->GetNext())
    { line_in_diagram * line = node->GetData();
      if (!line) continue;  // temporary state when updating lines after a table has been moved
      pt = line->points.Item(0)->GetData();
      if (pt->x==xpos && pt->y>=ypos-2 && pt->y<=ypos+2)
        { conflict=true;  break; }
      pt = line->points.Item(line->points.GetCount()-1)->GetData();
      if (pt->x==xpos && pt->y>=ypos-2 && pt->y<=ypos+2)
        { conflict=true;  break; }
    }
    if (!conflict) return ypos;
    if (ypos+2*WD <= table->pos.y+table->size.y)
      ypos+=2*WD;
    else return ypos+4;
  } while (true);
}

line_in_diagram * table_panel::make_z_line(table_in_diagram * table1, int pos1, table_in_diagram * table2, int pos2, int x1, int x2)
{ int y1 = find_free_ypos(table1, pos1, true);
  int y2 = find_free_ypos(table2, pos2, false);
  line_in_diagram * line = new line_in_diagram;
 // start point:
  wxPoint * pt = new wxPoint(x1, y1);
  line->points.Append(pt);
  if (y2!=y1)
  { int xx = x1+(x2-x1)/2;
    if (xx!=x1)
    { pt = new wxPoint(xx, y1);
      line->points.Append(pt);
    }
    pt = new wxPoint(xx, y2);
    line->points.Append(pt);
    if (x2!=xx)
    { pt = new wxPoint(x2, y2);
      line->points.Append(pt);
    }
  }
  else
  { pt = new wxPoint(x2, y1);
    line->points.Append(pt);
  }
  line->endtable[0]=table1;  line->endtable[1]=table2;
  line->pos1=pos1;           line->pos2=pos2;
  return line;
}

line_in_diagram * table_panel::make_c_line(table_in_diagram * table1, int pos1, table_in_diagram * table2, int pos2, int x1, int x2)
{ int y1 = find_free_ypos(table1, pos1, true);
  int y2 = find_free_ypos(table2, pos2, true);
  line_in_diagram * line = new line_in_diagram;
  int xx = x1 > x2 ? x1 : x2;
  xx+=20;
  wxPoint * pt = new wxPoint(x1, y1);
  line->points.Append(pt);
  pt = new wxPoint(xx, y1);
  line->points.Append(pt);
  pt = new wxPoint(xx, y2);
  line->points.Append(pt);
  pt = new wxPoint(x2, y2);
  line->points.Append(pt);
  line->endtable[0]=table1;  line->endtable[1]=table2;
  line->pos1=pos1;           line->pos2=pos2;
  return line;
}

line_in_diagram * table_panel::create_connecting_line(table_in_diagram * table1, int pos1, table_in_diagram * table2, int pos2, t_ending_type et1, t_ending_type et2)
{ int x1a, x1b, x2a, x2b;
  x1a=table1->pos.x-line_start_cx;  x1b=table1->pos.x+table1->size.x+line_start_cx;
  x2a=table2->pos.x-line_start_cx;  x2b=table2->pos.x+table2->size.x+line_start_cx;
  line_in_diagram * line;
  if (x1b<=x2a)      
  { line=make_z_line(table1, pos1, table2, pos2, x1b, x2a);
    line->ending_type[0]=et1;  line->ending_type[1]=et2;
  }
  else if (x2b<=x1a) 
  { line=make_z_line(table2, pos2, table1, pos1, x2b, x1a);
    line->ending_type[0]=et2;  line->ending_type[1]=et1;
  }
  else               
  { line=make_c_line(table1, pos1, table2, pos2, x1b, x2b);
    line->ending_type[0]=et1;  line->ending_type[1]=et2;
  }
  return line;
}

int table_in_diagram::find_key(const char * key)
// Returns the line in the table rectangle corresponding to the given key, -1 if not found
{ DiagrIndexList::Node * nodeI;  index_in_table * indx;
  for (nodeI = indexes.GetFirst();  nodeI;  nodeI=nodeI->GetNext())
  { indx = nodeI->GetData();
    if (!my_stricmp(indx->def, key))
      return indx->pos_in_win;
  }
  return -1;
}

void table_panel::connect_tables(ttablenum tbnum1, const char * key1, ttablenum tbnum2, const char * key2, t_ending_type et1, t_ending_type et2)
{ int index1, index2;
  if (!contains_table(tbnum1, &index1) || !contains_table(tbnum2, &index2)) return;
  table_in_diagram * table1 = tables.Item(index1)->GetData();
  table_in_diagram * table2 = tables.Item(index2)->GetData();
  line_in_diagram * line = create_connecting_line(table1, table1->find_key(key1), table2, table2->find_key(key2), et1, et2);
  lines.Append(line);
}

void table_panel::add_table_lines(table_in_diagram * tid)
{ char query[2*OBJ_NAME_LEN+200];  tcursnum cursnum;  trecnum cnt, rec;  
  tobjname table_name, schema_name;  ttablenum tbnum;  uns32 len;
 // add references from this table:
  sprintf(query, "select * from _iv_foreign_keys where schema_name=\'%s\' and table_name=\'%s\'", tid->schema_name, tid->table_name);
  if (!cd_Open_cursor_direct(pcdp, query, &cursnum))
  { cd_Rec_cnt(pcdp, cursnum, &cnt);
    for (rec=0;  rec<cnt;  rec++)
    { cd_Read(pcdp, cursnum, rec, 6, NULL, table_name);
      cd_Read(pcdp, cursnum, rec, 5, NULL, schema_name);
      if (!cd_Find_prefixed_object(pcdp, schema_name, table_name, CATEG_TABLE, &tbnum))
        if (contains_table(tbnum))
        { char * def1=load_blob(pcdp, cursnum, rec, 4, NOINDEX, &len);
          char * def2=load_blob(pcdp, cursnum, rec, 7, NOINDEX, &len);
          cutspaces(def1);  cutspaces(def2);
          if (def1 && def2) connect_tables(tid->tbnum, def1, tbnum, def2, ET_MANY, ET_ONE);
          corefree(def1);  corefree(def2);
        }
    }
    cd_Close_cursor(pcdp, cursnum);
  }
 // add references to this table:
  sprintf(query, "select * from _iv_foreign_keys where for_schema_name=\'%s\' and for_table_name=\'%s\'", tid->schema_name, tid->table_name);
  if (!cd_Open_cursor_direct(pcdp, query, &cursnum))
  { cd_Rec_cnt(pcdp, cursnum, &cnt);
    for (rec=0;  rec<cnt;  rec++)
    { cd_Read(pcdp, cursnum, rec, 2, NULL, table_name);
      cd_Read(pcdp, cursnum, rec, 1, NULL, schema_name);
      if (!cd_Find_prefixed_object(pcdp, schema_name, table_name, CATEG_TABLE, &tbnum))
        if (contains_table(tbnum))
        { char * def1=load_blob(pcdp, cursnum, rec, 4, NOINDEX, &len);
          char * def2=load_blob(pcdp, cursnum, rec, 7, NOINDEX, &len);
          connect_tables(tid->tbnum, def2, tbnum, def1, ET_ONE, ET_MANY);
          corefree(def1);  corefree(def2);
        }
    }
    cd_Close_cursor(pcdp, cursnum);
  }
}

WX_DEFINE_LIST(DiagrIndexList);

bool table_in_diagram::get_table_info(cdp_t cdp, table_panel * panel)
{// column info: 
  const d_table * td = cd_get_table_d(cdp, tbnum, CATEG_TABLE);
  if (!td) return false;
  col_cnt=td->attrcnt-first_user_attr(td);
  strcpy(schema_name, td->schemaname);
 // index info:
  char query[2*OBJ_NAME_LEN+200];  tcursnum cursnum;  trecnum cnt, rec;  
  int indnum = col_cnt;
  sprintf(query, "select * from _iv_indicies where schema_name=\'%s\' and table_name=\'%s\'", schema_name, table_name);
  if (!cd_Open_cursor_direct(cdp, query, &cursnum))
  { cd_Rec_cnt(cdp, cursnum, &cnt);
    for (rec=0;  rec<cnt;  rec++)
    { uns16 index_type;  uns32 len;
      cd_Read(cdp, cursnum, rec, 4, NULL, &index_type);
      char * def=load_blob(cdp, cursnum, rec, 5, NOINDEX, &len);
      if (def)
      { index_in_table * iit = new index_in_table;
        if (iit)
        { iit->index_type=index_type;
          iit->def=def;
         // find the index key among columns:
          wxString key = wxString(iit->def, *cdp->sys_conv);
          key.Trim(false);  key.Trim(true);
          if (key[0u]=='`') key=key.Right(key.Length()-1);
          if (key.Last()=='`') key=key.Left(key.Length()-1);
          int i;
          for (i=1;  i<td->attrcnt;  i++)
            if (!my_stricmp(key.mb_str(*cdp->sys_conv), td->attribs[i].name))
              break;
          if (i<td->attrcnt)  // found
            iit->pos_in_win=i-first_user_attr(td);
          else
            iit->pos_in_win=indnum++;
          indexes.Append(iit);
        }
      }
    }
    cd_Close_cursor(cdp, cursnum);
  }
  maxsizey = size.y = panel->table_caption_height + panel->table_column_height*indnum;
  panel->update_virtual_size(this);
  release_table_d(td);
  return true;
}

void table_panel::update_virtual_size(table_in_diagram * tid)
{ int width, height;
  if (!designer_opened) return;  // must not set virtual size before the window is created (when openning the designer)
  GetVirtualSize(&width, &height);
  int maxw, maxh;
  maxw = tid->pos.x+tid->size.x+400;
  maxh = tid->pos.y+tid->size.y+400;
  bool update = false;
  if (maxw>width)  { width =maxw;  update=true; }
  if (maxh>height) { height=maxh;  update=true; }
  if (update)
    SetVirtualSize(width, height);
}

void table_panel::set_virtual_size_by_contents(void)
{ int width, height;
  if (tables.GetFirst() == NULL)  // for empty diagrams create a lot of free space
  { width=2000;  height=2000; 
  }
  else  // create virtual size containing all tables and 400 more
  { width=height=0;
    for (DiagrTableList::Node * nodeT = tables.GetFirst();  nodeT;  nodeT=nodeT->GetNext())
    { table_in_diagram * table = nodeT->GetData();
      int maxw, maxh;
      maxw = table->pos.x+table->size.x+400;
      maxh = table->pos.y+table->size.y+400;
      if (maxw>width)  width =maxw;
      if (maxh>height) height=maxh;
    }
  }
  SetVirtualSize(width, height);
}

const char * table_in_diagram::index_text(int posit)
{ DiagrIndexList::Node * nodeI;  index_in_table * indx;
  for (nodeI = indexes.GetFirst();  nodeI;  nodeI=nodeI->GetNext())
  { indx = nodeI->GetData();
    if (indx->pos_in_win==posit) return indx->def;
  }
  return "";
}

int table_in_diagram::is_index(int posit)
{ DiagrIndexList::Node * nodeI;  index_in_table * indx;
  for (nodeI = indexes.GetFirst();  nodeI;  nodeI=nodeI->GetNext())
  { indx = nodeI->GetData();
    if (indx->pos_in_win==posit) return indx->index_type;
  }
  return 0;
}
                    
bool table_panel::PasteTables(int x, int y, bool real_paste)
// Inserts tables from the clipborad into th diagram, avoiding duplicities.
{ bool any_pasted = false;
  if (wxTheClipboard->Open())
  { C602SQLDataObject Data;
    if (wxTheClipboard->GetData(Data))
    { size_t Size = Data.GetDataSize(*DF_602SQLObject);
      if (Size)
      { C602SQLObjects * m_Objs = C602SQLObjects::Allocs((int)Size);
        if (!m_Objs) no_memory();
        else
        { if (Data.GetDataHere(*DF_602SQLObject, m_Objs))
          { // IF jiny server
            if (!stricmp(m_Objs->m_Server, pcdp->locid_server_name)) 
            // if (!stricmp(m_Objs->m_Schema, schema_name))
              any_pasted=insert_tables(x, y, m_Objs, real_paste);
          } // data from clipboard obtained
          m_Objs->Free();
        } // buffer allocated
      } // object size obtained, non-zero
    } // GetData successfull
    wxTheClipboard->Close();
  } // clipboard opened
  if (real_paste) 
    if (any_pasted) 
    { Refresh();  set_changed(); }
    else wxBell();
  return any_pasted;
}

void table_panel::RemoveTable(int item)
// Removed the table specified by [item] from the table_panel and removes all lines connected to the table.
{ table_in_diagram * tid = tables.Item(item)->GetData();
 // remove all lines connected to the table:
  for (DiagrLineList::Node * node = lines.GetFirst();  node;  )
  { line_in_diagram * line = node->GetData();
    DiagrLineList::Node * node2 = node->GetNext();
    if (line->endtable[0]==tid || line->endtable[1]==tid)
      lines.DeleteNode(node);
    node=node2;
  }
 // delete the table node:
  tables.DeleteNode(tables.Item(item));
  Refresh();
  set_changed();
}

void table_panel::clear_all(void)
{ tables.Clear();
  lines.Clear();
}

void table_panel::Refresh_tables_and_lines(void)
// Updates the table_panel so that it reflects changes made it table definitions and referential constrains.
// Does not have any impact on [changed] flag.
{
  char * src = make_source();
  if (!src) { no_memory();  return; }
  clear_all();
  compile(src);
  corefree(src);
}

char * table_panel::make_source(void)
{ t_flstr src;
  src.put("{ ");
 // tables:
  for (DiagrTableList::Node * nodeT = tables.GetFirst();  nodeT;  nodeT=nodeT->GetNext())
  { table_in_diagram * table = nodeT->GetData();
    src.put("T ");
    char buf[2*OBJ_NAME_LEN+50];
    table->write(pcdp, base_schema_name, buf);
    src.put(buf);
    src.put("\r\n");
  }
 // lines:
  for (DiagrLineList::Node * node = lines.GetFirst();  node;  node=node->GetNext())
  { line_in_diagram * line = node->GetData();
    if (line->points.GetCount()>=3)  // strait lines not stored
    { src.put("L ");
      //src.put(line->write().c_str());
      if (!wb_stricmp(pcdp, base_schema_name, line->endtable[0]->schema_name)) src.put("``");
      else src.putq(line->endtable[0]->schema_name);  
      src.putc(' ');
      src.putq(line->endtable[0]->table_name);   src.putc(' ');
      if (!wb_stricmp(pcdp, base_schema_name, line->endtable[1]->schema_name)) src.put("``");
      else src.putq(line->endtable[1]->schema_name);  
      src.putc(' ');
      src.putq(line->endtable[1]->table_name);   src.putc(' ');
      src.putint(line->ending_type[0]);  src.putc(' ');
      src.putint(line->ending_type[1]);  src.putc(' ');
      for (PointList::Node * nodeP = line->points.GetFirst();  nodeP;  nodeP=nodeP->GetNext())
      { wxPoint * point = nodeP->GetData();
        src.putint(point->x);  src.putc(' ');
        src.putint(point->y);  src.putc(' ');
      }
      src.put("\r\n");
    }
  }
  src.put("}\r\n");
  return src.unbind();
}

bool table_panel::compile(const char * src)
{ src++; // skip '{'
  tobjname name;
 // tables:
  do 
  { if (is_end(src)) return true;
    if (!read_ident(src, name)) return false; 
    if (strcmp(name, "T")) break;
    table_in_diagram * tid = new table_in_diagram;
    if (tid->read(pcdp, base_schema_name, src))
      if (!cd_Find_prefixed_object(pcdp, tid->schema_name, tid->table_name, CATEG_TABLE, &tid->tbnum))
        if (tid->get_table_info(pcdp, this))
        { tables.Append(tid);
          add_table_lines(tid);
        }
        else delete tid;
      else delete tid;
    else { delete tid;  return false; }
  } while (true);
 // lines:
  while (!strcmp(name, "L")) 
  {// read basic info:
    tobjname schema_name1, schema_name2, table_name1, table_name2;  int e1, e2;
    if (!(read_ident(src, schema_name1) && read_ident(src, table_name1) &&
          read_ident(src, schema_name2) && read_ident(src, table_name2) &&
          read_int(src, e1) && read_int(src, e2))) return false;
    if (!*schema_name1) strcpy(schema_name1, base_schema_name);
    if (!*schema_name2) strcpy(schema_name2, base_schema_name);
   // find endtables:
    table_in_diagram * t1=NULL, * t2 = NULL;
    for (DiagrTableList::Node * nodeT = tables.GetFirst();  nodeT;  nodeT=nodeT->GetNext())
    { table_in_diagram * table = nodeT->GetData();
      if (!my_stricmp(table_name1, table->table_name) && !my_stricmp(schema_name1, table->schema_name))
        t1=table;
      else if (!my_stricmp(table_name2, table->table_name) && !my_stricmp(schema_name2, table->schema_name))
        t2=table;
    }
    line_in_diagram * line = NULL;
    int starty, stopy;
    if (t1 && t2)
    {// find the line:
      for (DiagrLineList::Node * nodeL = lines.GetFirst();  nodeL;  nodeL=nodeL->GetNext())
      { line_in_diagram * aline = nodeL->GetData();
        if (aline->endtable[0]==t1 && aline->endtable[1]==t2)
        { starty = aline->points.Item(0                         )->GetData()->y;
          stopy  = aline->points.Item(aline->points.GetCount()-1)->GetData()->y;
          aline->points.Clear();
          line=aline;
          break;
        }
      }
#if 0
      if (!line) 
      { line = new line_in_diagram;
        line->ending_type[0]=(t_ending_type)e1;  line->ending_type[1]=(t_ending_type)e2;
      }
      line->endtable[0]=t1;  line->endtable[1]=t2;
#endif
    }
   // read points: 
    wxPoint pt;
    while (read_int(src, pt.x) && read_int(src, pt.y))
      if (line) line->points.Append(new wxPoint(pt));
    if (line && line->points.GetCount()>=3)
    { line->points.Item(0                        )->GetData()->y = starty;
      line->points.Item(1                        )->GetData()->y = starty;
      line->points.Item(line->points.GetCount()-2)->GetData()->y = stopy;
      line->points.Item(line->points.GetCount()-1)->GetData()->y = stopy;
    }
    if (is_end(src)) return true;
    if (!read_ident(src, name)) return false; 
  }
  return true;
}



void table_panel::init(void)
{ 
  SetDropTarget(new TablePanelDropTarget(this));
}  

/////////////////////////////// printing ////////////////////////////////////////////
wxPrintData           * table_panel::g_printData     = NULL;
wxPageSetupDialogData * table_panel::g_pageSetupData = NULL;

void DiagramPrintout::OnPreparePrinting(void)
{ int xmax=0, ymax=0;
 // calculate the diagram size:
  for (DiagrLineList::Node * node = panel->lines.GetFirst();  node;  node=node->GetNext())
  { line_in_diagram * line = node->GetData();
    for (PointList::Node * pnode = line->points.GetFirst();  pnode;  pnode=pnode->GetNext())
    { wxPoint * point = pnode->GetData();
      if (xmax<point->x+WD) xmax=point->x+WD;
      if (ymax<point->y+WD) ymax=point->y+WD;
    }
  }
  for (DiagrTableList::Node * nodeT =  panel->tables.GetFirst();  nodeT;  nodeT=nodeT->GetNext())
  { table_in_diagram * table = nodeT->GetData();
    int maxw, maxh;
    maxw = table->pos.x+table->size.x+2;
    maxh = table->pos.y+table->size.y+2;
    if (xmax<maxw) xmax=maxw;
    if (ymax<maxh) ymax=maxh;
  }
 // get sizing koeficients:
  GetPageSizePixels(&pagepxx, &pagepxy);
  GetPPIPrinter(&prippix, &prippiy);
  GetPPIScreen(&scrppix, &scrppiy);
  margin_l = table_panel::g_pageSetupData->GetMarginTopLeft().x;
  margin_t = table_panel::g_pageSetupData->GetMarginTopLeft().y;
  margin_r = table_panel::g_pageSetupData->GetMarginBottomRight().x;
  margin_b = table_panel::g_pageSetupData->GetMarginBottomRight().y;
  pagepxx -= (int)((margin_l+margin_r) * prippix / 25.6);
  pagepxy -= (int)((margin_t+margin_b) * prippiy / 25.6);
  width_pages  = xmax * prippix / scrppix / pagepxx + 1;
  height_pages = ymax * prippiy / scrppiy / pagepxy + 1;
#ifdef STOP
 // change scaling the the preview: m_previewScale
  HDC dc = ::GetDC(NULL);
  int screenYRes = ::GetDeviceCaps(dc, VERTRES);
  ::ReleaseDC(NULL, dc);
 // Get a device context for the currently selected printer
  wxPrinterDC printerDC(my_preview->m_printDialogData.GetPrintData());
  int printerYRes;
  dc = ((HDC)(printerDC).GetHDC());
  if ( dc )
  { printerYRes = ::GetDeviceCaps(dc, VERTRES);
    my_preview->m_previewScale = (float)((float)screenYRes/(float)printerYRes);
  }
#endif
}

void DiagramPrintout::GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo)
{ *minPage =1;  *maxPage=width_pages * height_pages;
  *pageFrom=1;  *pageTo =width_pages * height_pages;
}

bool DiagramPrintout::OnPrintPage(int pageNum)
{
  pageNum--;  // 1-base to 0-based
  int xpageoff = pageNum % width_pages;
  int ypageoff = pageNum / width_pages;
  wxDC * hDC = GetDC();
  if (!IsPreview())
  { hDC->SetUserScale((double)prippix / scrppix, (double)prippiy / scrppiy);
    hDC->SetDeviceOrigin( (int)(margin_l * prippix / 25.6) - xpageoff * pagepxx, 
                          (int)(margin_t * prippix / 25.6) - ypageoff * pagepxy);
    hDC->DestroyClippingRegion();
    //hDC->SetClippingRegion(xpageoff * (pagepxx), ypageoff * (pagepxy), 
    //                                  (pagepxx),            (pagepxy));
    hDC->SetClippingRegion(xpageoff * (pagepxx*scrppix/prippix), ypageoff * (pagepxy*scrppiy/prippiy), 
                                      (pagepxx*scrppix/prippix),            (pagepxy*scrppiy/prippiy));
  }
  else if (my_preview)
  { 
#ifdef WINS  
    hDC->SetUserScale((double)my_preview->GetZoom() / 100, (double)my_preview->GetZoom() / 100);
#else    
    hDC->SetUserScale((double)my_preview->GetZoom() / 100 * (double)prippix / scrppix, (double)my_preview->GetZoom() / 100 * (double)prippiy / scrppiy);
#endif    
    int w, h;
    hDC->GetSize(&w, &h);
    int prippix, prippiy, scrppix, scrppiy;
    GetPPIPrinter(&prippix, &prippiy);
    GetPPIScreen(&scrppix, &scrppiy);
    // int pagepxx_, pagepxy_;  my_preview->GetPrintoutForPrinting()->GetPageSizePixels(&pagepxx_, &pagepxy_); -- is zero!
    // create margins in the preview:
    int xmarg=0, ymarg=0;
#ifdef WINS    
    xmarg = margin_l * scrppix / 25.6;
    ymarg = margin_t * scrppiy / 25.6;
    if (!xmarg && !ymarg)
    // When margins are 0, the printer creates own margins, but the preview does not - centering the preview
    { xmarg = ((w * 100 / my_preview->GetZoom()) - (pagepxx*scrppix/prippix)) /2;
      ymarg = ((h * 100 / my_preview->GetZoom()) - (pagepxy*scrppiy/prippiy)) /2;
      if (xmarg<0) xmarg=0;  if (ymarg<0) ymarg=0;
    }
#else
    xmarg = (int)(margin_l * prippix / 25.6);
    ymarg = (int)(margin_t * prippix / 25.6);
#endif
    hDC->DestroyClippingRegion();
#ifdef LINUX 
    // this transformation is exact for a postscript printer on linux:
    //hDC->SetDeviceOrigin(-xpageoff * (w*scrppix/prippix), -ypageoff * (h*scrppiy/prippiy));
    hDC->SetDeviceOrigin((int)((xmarg -xpageoff * (pagepxx)) * (double)my_preview->GetZoom() / 100), 
                         (int)((ymarg -ypageoff * (pagepxy)) * (double)my_preview->GetZoom() / 100));  // works on MSW!!!
    hDC->SetClippingRegion(xpageoff * (pagepxx*scrppix/prippix), ypageoff * (pagepxy*scrppiy/prippiy), 
                                      (pagepxx*scrppix/prippix),            (pagepxy*scrppiy/prippiy));
#else
    hDC->SetDeviceOrigin((xmarg -xpageoff * (pagepxx*scrppix/prippix)) * (double)my_preview->GetZoom() / 100, 
                         (ymarg -ypageoff * (pagepxy*scrppiy/prippiy)) * (double)my_preview->GetZoom() / 100);  // works on MSW!!!
    hDC->SetClippingRegion(xpageoff * (pagepxx*scrppix/prippix), ypageoff * (pagepxy*scrppiy/prippiy), 
                                      (pagepxx*scrppix/prippix),            (pagepxy*scrppiy/prippiy));
#endif
  }
  panel->OnDraw(*hDC);
  return true;
}
