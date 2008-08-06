// xmldsgn.cpp
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
#include "controlpanel.h"
#include "wbprezex.h"             
#include "compil.h"
#include "xmldsgn.h"
#include "dsparser.h"
#include "transdsgn.h"
#include "impexpui.h"
#include <wx/dataobj.h>
#include <wx/dnd.h>
#include "wx/treectrl.h"
#include <wx/splitter.h>

#include "bmps/elem_l.xpm"
#include "bmps/elem_r.xpm"
#include "bmps/xml_atribut.xpm"
#include "bmps/elem_text.xpm"
#include "bmps/xmlinfo.xpm"
#include "bmps/xml_import.xpm"
#include "bmps/xml_export.xpm"
#include "bmps/xml_test.xpm"
#include "bmps/xml_properties.xpm"
#include "bmps/dad_element.xpm"
#include "bmps/dad_attribute.xpm"
#include "bmps/dad_text.xpm"
#include "bmps/dad_edit.xpm"
#include "bmps/dad_delete.xpm"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#if WBVERS<950
#include "xrc/NewDataTransferDlg.h"
#else
#include "xrc/NewDataTransfer95Dlg.h"
#endif

class xml_designer : public wxTreeCtrl, public any_designer
{ WizardPage3 * wpg;
  t_xsd_gen_params * gen_params;
 public:
  t_dad_root * dad_tree;
  dad_node * anode;  // temporary data
  wxTreeItemId hItem;   // temporary data
  d_table * td;  // global td for explicit SQL, fully owned, !=NULL iff dad_tree->explicit_sql_statement
  ::t_ucd_cl * top_ucdp;  // data source for the explicit SQL statement
  tcursnum top_curs;   // cursor for the explicit SQL, fully owned, !=NOOBJECT iff dad_tree->explicit_sql_statement
 // dragging:
  dad_node * dragged_node;
  wxTreeItemId dragged_item;
  bool dragging_end_item;
 // params:
  void materialize_params(t_ucd_cl * cdcl);
  xml_designer(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn, WizardPage3 * wpgIn, t_xsd_gen_params * gen_paramsIn) :
    any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, CATEG_PGMSRC, _xml_xpm, &xmldsng_coords),
    wpg(wpgIn), gen_params(gen_paramsIn), dragged_item()
     { td=NULL;  top_ucdp=NULL;  top_curs=NOOBJECT;  dad_tree=NULL;  dragged_node=NULL;  dragging_end_item=false;  changed=false; fromdclick = false;}
  ~xml_designer(void)
  { Freeze();  // prevents long collapsing
    remove_tools();
    if (dad_tree) delete dad_tree;
    if (top_ucdp) 
    { if (td) top_ucdp->release_table_d(td);
      if (top_curs!=NOOBJECT) top_ucdp->close_cursor(top_curs);  // must be after release_table_d()
      link_delete_t_ucd_cl(top_ucdp);
    }
    link_close_platform_utils();
  }
  bool open(wxWindow * parent, t_global_style my_style);
  bool item_action(wxTreeItemId item, dad_node * node, int cmd);
  void insert_element(wxTreeItemId item, dad_node * node, dad_element_node * newel, bool from_clipboard, int command);
  void insert_attribute(wxTreeItemId item, dad_node * node, dad_attribute_node * newat, bool from_clipboard);
  void insert_text(wxTreeItemId item, dad_node * node, t_dad_column_node * newcol, bool from_clipboard);
  bool is_target_my_subnode(wxTreeItemId htiTarget);
  bool can_drop_endnode_here(wxTreeItemId htiTarget);
  void move_endnode_here(wxTreeItemId htiTarget);
  bool show_popup_menu(wxTreeItemId item, dad_node * node, wxPoint pt);
  bool select_node_by_num(wxTreeItemId hParItem, int & err_node_num);
  int load_dad_design(t_ucd_cl * cdcl);
  
 ////////// typical designer methods and members: ///////////////////////
  enum { TOOL_COUNT = 12 };
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
 // member variables:
  bool changed;
  bool fromdclick;
  tobjname object_name;
 // Callbacks:
  void OnActivated(wxTreeEvent & event);
  void OnExpanding(wxTreeEvent & event);
  void OnRightButtonDown(wxMouseEvent & event);
  void OnMouseMove(wxMouseEvent & event);
  void OnKeyDown(wxTreeEvent & event);
  void OnBeginDrag(wxTreeEvent & event);
  void OnEndDrag(wxTreeEvent & event);
  static void err_callback(const void *Owner, int ErrCode, DSPErrSource ErrSource, const wchar_t *ErrMsg);
#ifdef WINS
  void OnRightClick(wxTreeEvent & event);
#else
  void OnRightClick(wxMouseEvent & event);
#endif
  DECLARE_EVENT_TABLE()
};

dad_node * clipboard_node = NULL;  // the clipboard is shared among designs

BEGIN_EVENT_TABLE( xml_designer, wxTreeCtrl )
  //EVT_KEY_DOWN(OnKeyDown)
  EVT_TREE_KEY_DOWN(-1, xml_designer::OnKeyDown)
  EVT_TREE_ITEM_ACTIVATED(-1, xml_designer::OnActivated)
  EVT_TREE_ITEM_EXPANDING(-1, xml_designer::OnExpanding) 
  EVT_TREE_ITEM_COLLAPSING(-1, xml_designer::OnExpanding)
  EVT_RIGHT_DOWN(xml_designer::OnRightButtonDown)  // may be replaced by EVT_TREE_ITEM_RIGHT_CLICK
#ifdef LINUX
  EVT_RIGHT_UP(xml_designer::OnRightClick)  // enters selection mode on MSW
#else
  //EVT_COMMAND_RIGHT_CLICK(-1, xml_designer::OnRightClick)  // exists only on MSW, but emited only when EVT_TREE_ITEM_RIGHT_CLICK is not emitted
  EVT_TREE_ITEM_RIGHT_CLICK(-1, xml_designer::OnRightClick)  // on LINUX emitted on ButtonDown
#endif
  EVT_TREE_BEGIN_DRAG(-1, xml_designer::OnBeginDrag) 
  EVT_TREE_END_DRAG(-1, xml_designer::OnEndDrag) 
  EVT_MOTION(xml_designer::OnMouseMove) 
END_EVENT_TABLE()

// items from the local popup menu (not in the Designer menu nor toolbar):
#define XML_EDIT       1
#define XML_CUT        TD_CPM_CUT    // making it compatible with the potential Designer menu/toolbar commands
#define XML_PASTE      TD_CPM_PASTE  // making it compatible with the potential Designer menu/toolbar commands
#define XML_DELETE     4
#define XML_INS_ELEM   5
#define XML_INS_ATTR   6
#define XML_INS_TEXT   7
#define XML_INS_ELEM_BEFORE 8
#define XML_INS_ALL    9

class _t_ucd_cl
{ t_ucd_cl * pcdcl;
 public:
  _t_ucd_cl(cdp_t cdpIn)
    { pcdcl = link_new_t_ucd_cl_602(cdpIn); }
  ~_t_ucd_cl(void)
    { link_delete_t_ucd_cl(pcdcl); } 
  operator t_ucd_cl *(void)
    { return pcdcl; }
  operator t_ucd_cl &(void)
    { return *pcdcl; }
  t_ucd_cl * x(void)    
    { return pcdcl; }
};
    
char * xml_designer::make_source(bool alter)
{ wxBusyCursor wait;
  return ::link_make_source(dad_tree, alter, cdp); 
}

struct xml_item_data : public wxTreeItemData
{ dad_node * ptr;
  xml_item_data(dad_node * ptrIn) : ptr(ptrIn)
   { }
};

dad_node * get_item_node(wxTreeCtrl * tree, wxTreeItemId item)
{ return ((xml_item_data*)tree->GetItemData(item))->ptr; }

wxTreeItemId stop2start(wxTreeCtrl * tree, wxTreeItemId hStop)
// For stop tag returns start tag, for other items returns itself.
{ dad_node * stopnode = get_item_node(tree, hStop);
  wxTreeItemId hStart = tree->GetPrevSibling(hStop);
  if (hStart.IsOk())
  { dad_node * startnode = get_item_node(tree, hStart);
    if (startnode==stopnode) return hStart;
  }
  return hStop;
}

wxTreeItemId start2stop(wxTreeCtrl * tree, wxTreeItemId hStart)
// For start tag returns stop tag, for other items return itself.
{ dad_node * startnode = get_item_node(tree, hStart);
  wxTreeItemId hStop = tree->GetNextSibling(hStart);
  if (hStop.IsOk())
  { dad_node * stopnode = get_item_node(tree, hStop);
    if (startnode==stopnode) return hStop;
  }
  return hStart;
}

#if 0
class XML_node_data_object : public wxDataObjectSimple
{ dad_node * the_data;
 public:
  size_t GetDataSize(void) const
    { return sizeof(dad_node *); }
  bool GetDataHere(void *buf) const
    { return false; }

  bool SetData(size_t len, const void *buf)
    { the_data=(dad_node *)buf;
      return true;
    }
  XML_node_data_object(const wxDataFormat& format = wxFormatInvalid) : wxDataObjectSimple(format)
    { the_data=NULL; }

};

class XMLDropSource : public wxDropSource
{ xml_designer * xmldsgn;
 public:
  XMLDropSource(xml_designer * xmldsgnIn) : wxDropSource(xmldsgnIn) 
    { xmldsgn=xmldsgnIn; }
};
#endif

void xml_designer::OnBeginDrag(wxTreeEvent & event)
{// analyse the dragged item: 
  dragged_item      = stop2start(this, event.GetItem());
  if (dragged_item==GetRootItem() || dragged_item==GetItemParent(GetRootItem()))
    { event.Veto();  return; }
  dragging_end_item = dragged_item!=event.GetItem();
  dragged_node      = get_item_node(this, dragged_item);
  event.Allow();
#if 0
 // start dragging:
  XML_node_data_object drag_data(wxDataFormat("XML design node"));
  drag_data.SetData(sizeof(dragged_node), dragged_node);
  XMLDropSource dragSource(this);
  dragSource.SetData(drag_data);
    wxDragResult result = dragSource.DoDragDrop(wxDrag_DefaultMove);
  if (result==wxDragMove) ;
#endif
}

void xml_designer::OnEndDrag(wxTreeEvent & event)
{
  event.Allow();  // changes the drag cursor back
  if (!dragged_item.IsOk()) return;  // internal error
  wxTreeItemId target_item;
  target_item=event.GetItem();
  if (target_item.IsOk() && (GetItemParent(target_item)!=GetRootItem() || stop2start(this, target_item)!=target_item))
  { if (dragging_end_item)
      if (can_drop_endnode_here(target_item))
      { move_endnode_here(target_item);
        changed=true;
      }
      else
        wxBell();
    else // normal drag&drop
     // check for dropping into own subnodes:
      if (is_target_my_subnode(target_item))
        wxBell();
      else
      { dad_node * node = get_item_node(this, target_item);
        item_action(dragged_item, dragged_node, XML_CUT);
        item_action(target_item, node, XML_PASTE);
        changed=true;
      }
  }
  dragged_item=wxTreeItemId();  dragged_node=NULL;
  SetCursor(*wxSTANDARD_CURSOR);
}

void xml_designer::OnMouseMove(wxMouseEvent & event)
{ event.Skip();  // allows the further processing
  if (!dragged_item.IsOk() || !event.Dragging()) return;  // ignore if not dragging
  int flags;  bool drop_ok;  wxTreeItemIdValue cookie;
  wxTreeItemId hit = HitTest(event.GetPosition(), flags);
  if (hit.IsOk() && (flags & (wxTREE_HITTEST_ONITEMBUTTON | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL | wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT)))
    drop_ok = dragging_end_item ? can_drop_endnode_here(hit) : !is_target_my_subnode(hit);
  else
  { drop_ok = false;
    if (flags & wxTREE_HITTEST_ABOVE) 
    { wxTreeItemId hPrev, hFirst = GetFirstVisibleItem();
      hPrev=GetPrevSibling(hFirst);
      if (hPrev.IsOk()) // go to its last child
      { do
        { wxTreeItemId hChild=GetFirstChild(hPrev, cookie);
          if (!hChild.IsOk()) break;
          do
          { wxTreeItemId hNext=GetNextSibling(hChild);
            if (!hNext.IsOk()) break;
            hChild=hNext;
          } while (true);
          hPrev=hChild;
        } while (true);
      }
      else
        hPrev=GetItemParent(hFirst);
      hit=hPrev;
    }
    else if (flags & (wxTREE_HITTEST_BELOW | wxTREE_HITTEST_NOWHERE))
    { wxTreeItemId hNext, hLast = GetFirstVisibleItem();
      do // from first viible to the last visible
      { hNext=GetNextVisible(hLast);
        if (!hNext.IsOk()) break;
        hLast=hNext;
      } while (true);
      do // find sibling or parent's sibling
      { hNext=GetNextSibling(hLast);
        if (hNext.IsOk()) break;
        hLast=GetItemParent(hLast);
        if (!hLast.IsOk() || hLast==GetRootItem()) break;
      } while (true);
      if (hNext.IsOk()) do // find sibling's first child
      { wxTreeItemId hChild=GetFirstChild(hNext, cookie);
        if (!hChild.IsOk()) break;
        hNext=hChild;
      } while (true);
      hit=hNext;
    }
  }
  if (hit.IsOk() && hit!=GetRootItem()) 
    EnsureVisible(hit);
  if (drop_ok) SetCursor(*wxSTANDARD_CURSOR);  
  else SetCursor(wxCursor(wxCURSOR_NO_ENTRY));
}

void xml_designer::set_designer_caption(void)
{ wxString caption;
  if (!modifying()) caption = _("Design a New DAD");
  else caption.Printf(_("Editing DAD %s"), wxString(object_name, *cdp->sys_conv).c_str());
  set_caption(caption);
}

//////////////////////////////// materializing parameters ///////////////////////
#define PIECESIZE 6  // sizeof(tpiece)
uns8 _tpsize[ATT_AFTERLAST] =
 {0,                                   /* fictive, NULL */
  1,1,2,4,6,8,0,0,0,0,4,4,4,sizeof(trecnum),sizeof(trecnum), /* normal  */
  UUID_SIZE,4,4+PIECESIZE,        /* special */
  4+PIECESIZE,4+PIECESIZE,   /* raster, text */
  4+PIECESIZE,4+PIECESIZE,   /* nospec, signature */
#ifdef SRVR
  0,               // UNTYPED is not used on server
#else
  sizeof(untstr),  // UNTYPED
#endif
  0,0,0,0,    /* not used & ATT_NIL */
  1,sizeof(ttablenum),3*sizeof(tobjnum),8,0,3*sizeof(tcursnum), /* file, table, cursor, dbrec, view, statcurs */
  0,0,0, 0,0,0, 0,0,0, 0,0,
  1, 8};                       // int8, int64

void hostvars_from_params(t_dad_param * params, int parcnt, t_clivar *& hostvars, int & hc)
{ 
  hc=0;  hostvars=NULL;
  if (!parcnt) return;
  hostvars = (t_clivar*)corealloc(sizeof(t_clivar) * parcnt, 66);
  if (!hostvars) { no_memory();  return; }
  memset(hostvars, 0, sizeof(t_clivar) * parcnt);
  for (int i=0;  i<parcnt;  i++)
  { t_dad_param * param = params+i;
    if (!param->name[0]) continue;
    strmaxcpy(hostvars[hc].name, param->name, sizeof(hostvars[hc].name));
    hostvars[hc].wbtype=param->type;
    hostvars[hc].specif=param->specif.opqval;
    hostvars[hc].mode=MODE_IN;
    hostvars[hc].buflen=param->type==ATT_STRING || param->type==ATT_BINARY ? t_specif(hostvars[hc].specif).length+1 :
                       IS_HEAP_TYPE(param->type) ? MAX_PARAM_VAL_LEN+1 :
                       _tpsize[param->type];
    hostvars[hc].buf=(tptr)corealloc(hostvars[hc].buflen+1, 66);
    hostvars[hc].actlen=hostvars[hc].buflen;
    wxString string_format = get_grid_default_string_format(param->type);
    int err=superconv(ATT_STRING, param->type, param->val, (char*)hostvars[hc].buf, wx_string_spec, param->specif, string_format.mb_str(*wxConvCurrent));
    if (err<0)
      superconv(ATT_STRING, param->type, wxT(""), (char*)hostvars[hc].buf, wx_string_spec, param->specif, "");
    hc++;
  }
  if (!hc) 
    { corefree(hostvars);  hostvars=NULL; }
}

void xml_designer::materialize_params(t_ucd_cl * cdcl)
{ cdcl->dematerialize_params();   // to be safe
  t_clivar * hostvars;  int hc;
  hostvars_from_params(dad_tree->params.acc0(0), dad_tree->params.count(), hostvars, hc);
  cdcl->supply_temp_hostvars(hostvars, hc);
}

//////////////////////////////// Converting memory tree into the tree control ////////////////////////////////////
wxTreeItemId to_tree_ctrl(wxMBConv * conv, dad_attribute_node * node, wxTreeCtrl * tree, wxTreeItemId hParent, t_insert_mode mode = IM_LASTCHILD, wxTreeItemId hInsertAfter = wxTreeItemId())
{ char buf[sizeof(node->attr_name)+3+OBJ_NAME_LEN+1+ATTRNAMELEN+50+1];
  node->get_caption(buf, false, sizeof(buf));
  wxTreeItemId item;
  if (mode==IM_LASTCHILD || tree->GetChildrenCount(hParent, false)==0)
    item = tree->AppendItem(hParent,               wxString(buf, *conv), 1, 1, new xml_item_data(node));
  else if (mode==IM_FIRSTCHILD)  // this InsertItem requires GetChildrenCount()>0
    item = tree->InsertItem(hParent, 0,            wxString(buf, *conv), 1, 1, new xml_item_data(node));
  else
    item = tree->InsertItem(hParent, hInsertAfter, wxString(buf, *conv), 1, 1, new xml_item_data(node));
  tree->SetItemBold(item);
  tree->Expand(item);
  return item;
}

wxTreeItemId to_tree_ctrl(wxMBConv * conv, t_dad_column_node * node, wxTreeCtrl * tree, wxTreeItemId hParent, t_insert_mode mode = IM_LASTCHILD, wxTreeItemId hInsertAfter = wxTreeItemId())
{ char buf[1+OBJ_NAME_LEN+1+sizeof(t_dad_col_name)+1+1];
  node->get_caption(buf, false, sizeof(buf));
  wxTreeItemId item;
  if (mode==IM_LASTCHILD || tree->GetChildrenCount(hParent, false)==0)
    item = tree->AppendItem(hParent,               wxString(buf, *conv), 2, 2, new xml_item_data(node));
  else if (mode==IM_FIRSTCHILD)  // this InsertItem requires GetChildrenCount()>0
    item = tree->InsertItem(hParent, 0,            wxString(buf, *conv), 2, 2, new xml_item_data(node));
  else
    item = tree->InsertItem(hParent, hInsertAfter, wxString(buf, *conv), 2, 2, new xml_item_data(node));
  tree->Expand(item);
  return item;
}

wxTreeItemId to_tree_ctrl(wxMBConv * conv, dad_element_node * node, wxTreeCtrl * tree, wxTreeItemId hParent, t_insert_mode mode = IM_LASTCHILD, wxTreeItemId hInsertAfter = wxTreeItemId())
{// insert element node:
  char buf[100+2*OBJ_NAME_LEN+1+sizeof(t_dad_col_name)+1+1];
  node->get_caption(buf, false, sizeof(buf));
  wxTreeItemId elitem;
  if (mode==IM_LASTCHILD || tree->GetChildrenCount(hParent, false)==0)
    elitem = tree->AppendItem(hParent,               wxString(buf, *conv), 0, 0, new xml_item_data(node));
  else if (mode==IM_FIRSTCHILD)  // this InsertItem requires GetChildrenCount()>0
    elitem = tree->InsertItem(hParent, 0,            wxString(buf, *conv), 0, 0, new xml_item_data(node));
  else
    elitem = tree->InsertItem(hParent, hInsertAfter, wxString(buf, *conv), 0, 0, new xml_item_data(node));
  if (node->is_related_to_table())
    tree->SetItemBold(elitem);
 // insert attributes:
  for (dad_attribute_node * attr = node->attributes;  attr;  attr=attr->next)
    to_tree_ctrl(conv, attr, tree, elitem);  // appending
  if (node->is_linked_to_column())
    to_tree_ctrl(conv, &node->text_column, tree, elitem);  // appending
 // insert subelements
  for (dad_element_node * subel = node->sub_elements;  subel;  subel=subel->next)
    to_tree_ctrl(conv, subel, tree, elitem);  // appending
 // insert closing element node:
  node->get_caption(buf, true, sizeof(buf));
  wxTreeItemId elitem2 = tree->InsertItem(hParent, elitem, wxString(buf, *conv), 4, 4, new xml_item_data(node));
  if (node->is_related_to_table())
    tree->SetItemBold(elitem2);
  tree->Expand(elitem);
  return elitem;
}

wxTreeItemId dad2tree(cdp_t cdp, wxTreeCtrl * tree, t_dad_root * dad)
{ char buf[200];
  wxBusyCursor wait;
  tree->AddRoot(wxEmptyString);
  dad->get_caption(buf, false, sizeof(buf));
  wxTreeItemId iitem = tree->AppendItem(tree->GetRootItem(), wxString(buf, *cdp->sys_conv), 3, 3, new xml_item_data(dad));
  wxTreeItemId ritem = to_tree_ctrl(&GetConv(cdp), dad->root_element, tree, tree->GetRootItem());  // appending
  tree->Expand(ritem);
  tree->SelectItem(iitem);
  return iitem;
}

//////////////////////////////////////// dialog support ///////////////////////////////////////////
void fill_table_names(cdp_t cdp, wxOwnerDrawnComboBox * combo, bool add_base_cursor, const char * sel_table_name)
{ tobjname upcase_table_name;
  strcpy(upcase_table_name, sel_table_name);
  Upcase9(cdp, upcase_table_name);  // table with diacritics with a different case would not be found
  combo->Clear();
  if (add_base_cursor)
    combo->Append(base_cursor_name, (void*)0);
  void * en = get_object_enumerator(cdp, CATEG_TABLE, NULL);
  tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
  while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
    if (!(flags & (CO_FLAG_ODBC | CO_FLAG_LINK)))
      combo->Append(wxString(name, *cdp->sys_conv), (void*)(size_t)objnum);
  free_object_enumerator(en);
  int ind = combo->FindString(wxString(upcase_table_name, *cdp->sys_conv));
  if (ind!=wxNOT_FOUND) 
    combo->SetSelection(ind);
}

bool is_top_table_element(xml_designer * dade)
{ wxTreeItemId item = dade->GetItemParent(dade->hItem);
  while (item.IsOk())
  { if (item==dade->GetRootItem()) break;  // must not call get_item_node() for the hidden root
    dad_element_node * el = (dad_element_node*)get_item_node(dade, item);
    if (el->type() == DAD_NODE_ELEMENT)
      if (*el->lower_tab) return false;
    item = dade->GetItemParent(item);
  }
  return true;
}

void fill_my_table_names(xml_designer * dade, wxOwnerDrawnComboBox * combo, const char * sel_table_name, bool including_local)
// Fills names of tables (aliases) included into the design.
{ combo->Clear();
  for (wxTreeItemId item = dade->hItem;  item.IsOk();  item = dade->GetItemParent(item))
  { if (item==dade->GetRootItem()) break;  // must not call get_item_node() for the hidden root
    dad_element_node * el = (dad_element_node*)get_item_node(dade, item);
    if (el->type() == DAD_NODE_ELEMENT)
    { if (*el->lower_tab) 
        if (including_local || item != dade->hItem)
        { tgenobjname tabname;  ttablenum tablenum;
          strcpy(tabname, el->lower_table_name());
         // analyse the DSN:
          if (!stricmp(tabname, base_cursor_name8))
            tablenum=0;  // -base cursor-
          else
          { const t_avail_server * as = find_server_connection_info(!*el->dsn ? dade->cdp->locid_server_name : el->dsn);
            if (as)
            { if (as->odbc) tablenum=-2;
              else if (as->cdp)
              { wb_small_string(dade->cdp, tabname, TRUE);
                if (cd_Find_prefixed_object(as->cdp, dade->schema_name, el->lower_tab, CATEG_TABLE, &tablenum)) 
                  tablenum=0;  // -base cursor-
              }
              else continue;
            }
            else continue;
          }
          combo->Append(wxString(tabname, *dade->cdp->sys_conv), (void*)(size_t)tablenum);
        }
    }
  }
  if (sel_table_name)
    if (*sel_table_name)
    { int ind = combo->FindString(wxString(sel_table_name, *dade->cdp->sys_conv));
      if (ind!=-1) 
        combo->SetSelection(ind);
    }
    else  // table not specified yet
    { if (combo->GetCount() == 1)  // only one table is in the list
        combo->SetSelection(0);  // select it
    }
}

void fill_column_names(xml_designer * dade, wxOwnerDrawnComboBox * table, wxOwnerDrawnComboBox * c_schema, wxOwnerDrawnComboBox * c_dsn, wxOwnerDrawnComboBox * column, const char * column_name)
// If [c_schema]!=NULL and [c_dsn]!=NULL use schema and ds names from these combos. Otherwise use names stored in parent elements in the DAD tree.
// Fills columns from table selected in hTable; Fills columns from td when base_cursor selected
{ column->Clear();  bool local_td_created=false;
  const d_table * atd = dade->td;  ttablenum tablenum = 0;   const t_avail_server * as = NULL;  
  int ind = GetNewComboSelection(table);  //table->GetSelection();
  if (ind!=-1)
  { tablenum = (ttablenum)(size_t)table->GetClientData(ind);  // will be ignored for ODBC, but must be different from 0
    if (tablenum!=0)  // 0 value used for -base cursor-
#if WBVERS<1000
      atd = cd_get_table_d(dade->cdp, tablenum, CATEG_TABLE);
#else      
    { bool found=false;  wxString dsn, schema;  tgenobjname schemaname, tablename;
      if (c_schema!=NULL && c_dsn!=NULL)  // get data from the combos in the current dialog
      { int sel = GetNewComboSelection(c_dsn);
        if (sel>=0) dsn=c_dsn->GetString(sel); 
        as = find_server_connection_info(dsn.IsEmpty() ? dade->cdp->locid_server_name : dsn.mb_str(*wxConvCurrent));
        if (!as) return;  // fuse
       // get schema name:
        sel = GetNewComboSelection(c_schema);
        wxString schema;
        if (sel>=0) schema=c_schema->GetString(sel);
        strcpy(schemaname, schema.mb_str(*(as->cdp ? as->cdp->sys_conv : wxConvCurrent)));
        strcpy(tablename,  table->GetString(ind).mb_str(*(as->cdp ? as->cdp->sys_conv : wxConvCurrent)));
        found=true;
      }
      else  // use dsn/schema names from the parent element
      { wxString tablenm = table->GetString(ind);
       // search the upper nodes:
        wxTreeItemId item = dade->hItem;
        while (item.IsOk())
        { if (item==dade->GetRootItem()) break;  // must not call get_item_node() for the hidden root
          dad_element_node * el = (dad_element_node*)get_item_node(dade, item);
          if (el->type() == DAD_NODE_ELEMENT)
          { if (*el->lower_tab) 
            { as = find_server_connection_info(!*el->dsn ? dade->cdp->locid_server_name : el->dsn);
              if (!tablenm.CmpNoCase(wxString(el->lower_table_name(), *(as->cdp ? as->cdp->sys_conv : wxConvCurrent))))
              { strcpy(schemaname, el->lower_schema);  // used by the ODBC version only
                strcpy(tablename,  el->lower_tab);     // used by the ODBC version only
                found=true;
                break;
              }
            }
          }
          item = dade->GetItemParent(item);
        }
      }
      atd=NULL;
      if (found)
      { if (as->odbc)
        { if (as->conn)
            { atd = make_odbc_descriptor9(as->conn, tablename, schemaname, TRUE);  local_td_created=true; }
        }
        else
        { if (as->cdp)
            { atd = cd_get_table_d(as->cdp, tablenum, CATEG_TABLE);  local_td_created=true; }
        }
      }
    }
#endif    
  }
 // fill the list:
  wxMBConv * conv = as ? (as->cdp ? as->cdp->sys_conv : wxConvCurrent) : dade->cdp->sys_conv;
  if (atd) // unless error
  { for (int i=1;  i<atd->attrcnt;  i++)
      if (atd->attribs[i].multi==1)  // skip multiattributes
      { char name[ATTRNAMELEN+1];
        strcpy(name, atd->attribs[i].name);
        if (!as || !as->odbc) wb_small_string(as ? as->cdp : NULL, name, false);
        column->Append(wxString(name, *conv));
      }
    if (local_td_created)         // local d_table, must release
      if (as && as->odbc)
        corefree(atd);
      else
        release_table_d(atd);  
  }
  if (column_name)
  { int ind = column->FindString(wxString(column_name, *conv));
    if (ind!=-1) column->SetSelection(ind);
  }
}

#if 0

void fill_column_names(cdp_t cdp, const d_table * td, wxOwnerDrawnComboBox * table, wxOwnerDrawnComboBox * column, const char * column_name)
// If [schema]!=NULL and [dsn]!=NULL used schema and ds names from these combos. Otherwise use names stored in parent elements in the DAD tree.
// Fills columns from table selected in hTable; Fills columns from td when base_cursor selected
{ column->Clear();
  const d_table * atd = td;  ttablenum tablenum = 0;
  int ind = GetNewComboSelection(table);  //table->GetSelection();
  if (ind!=-1)
  { tablenum = (ttablenum)table->GetClientData(ind);
    if (tablenum!=0)  // 0 value used for -base cursor-
      atd = cd_get_table_d(cdp, tablenum, CATEG_TABLE);
  }
 // fill the list:
  if (atd) // unless error
  { for (int i=1;  i<atd->attrcnt;  i++)
      if (atd->attribs[i].multi==1)  // skip multiattributes
      { char name[ATTRNAMELEN+1];
        strcpy(name, atd->attribs[i].name);
        wb_small_string(cdp, name, false);
        column->Append(wxString(name, *cdp->sys_conv));
      }
    if (tablenum!=0) release_table_d(atd);  // local d_table, must return
  }
  if (column_name)
  { int ind = column->FindString(wxString(column_name, *cdp->sys_conv));
    if (ind!=-1) column->SetSelection(ind);
  }
}
#endif

char * GetStringAlloc(cdp_t cdp, wxString label)
{ 
  label.Trim(false);  label.Trim(true);
  if (label.IsEmpty()) return NULL;
  char * ptr = link_new_chars((int)label.Length()+1);
  if (ptr!=NULL) strcpy(ptr, label.mb_str(*cdp->sys_conv));
  return ptr;
}


#include "fsed9.h"
#include "querydsgn.h"
#include "xrc/XMLTranslationDlg.cpp"
#if WBVERS<1000
#include "xrc/XMLElementDlg.cpp"
#else
#include "xrc/XMLElement10Dlg.h"
#include "xrc/XMLKeyGenDlg.cpp"
#include "xrc/XMLElement10Dlg.cpp"
#endif
#include "xrc/XMLAttributeDlg.cpp"
#include "xrc/XMLTextDlg.cpp"
#include "xrc/XMLGlobalDlg.cpp"
#include "xrc/TextOutputDlg.cpp"
#include "xrc/XMLParamsDlg.cpp"

/////////////////////////////////// inserting items ///////////////////////////////////
void xml_designer::insert_element(wxTreeItemId item, dad_node * node, dad_element_node * newel, bool from_clipboard, int command)
// Inserts [newel] below [node] which is in the tree as [item]. Inserts default node if [newel]==NULL.
{// for a stop node, insert below corresponding start node instead, on the end
  wxTreeItemId hInsParent;
  if (command == XML_INS_ELEM)
    hInsParent = stop2start(this, item);  
  else  // for a start node, insert below its parent
  { hInsParent=GetItemParent(item);
    if (!hInsParent) return;  // cannot insert new root
  }
  dad_element_node * parel = (dad_element_node*)get_item_node(this, hInsParent);
 // create default item if inserting a new one:
  if (!newel)
  { newel = link_new_dad_element_node();
   // find upper node with a table reference, use it as an uplink:
    wxTreeItemId hAnc = hInsParent;
    while (hAnc.IsOk())
    { if (hAnc==GetRootItem()) break;  // must not call get_item_node() for the hidden root
      dad_element_node * anc_el = (dad_element_node*)get_item_node(this, hAnc);
      if (anc_el->type() == DAD_NODE_ELEMENT)
        if (*anc_el->lower_tab) 
        { strcpy(newel->upper_tab, anc_el->lower_table_name());
          break;
        }
      hAnc=GetItemParent(hAnc);
    } 
  }
 // find the element for inserting after and insert to the dad_tree:
  wxTreeItemId hInsertAfter;  t_insert_mode mode;  wxTreeItemIdValue cookie;
  if (parel->sub_elements==node || !parel->sub_elements || node->type()!=DAD_NODE_ELEMENT)  
  // insert before the first subelement because the 1st subelement is selected or attribute/text is selected
  { newel->next=parel->sub_elements;  parel->sub_elements=newel;
    hInsertAfter = GetFirstChild(hInsParent, cookie);  // this must be an attribute item
    if (!hInsertAfter.IsOk() || get_item_node(this, hInsertAfter)->type() == DAD_NODE_ELEMENT) 
      mode=IM_FIRSTCHILD;
    else // skip all non-element nodes, stop on the last of them:
    { mode=IM_AFTER_SPECIF;
      do
      { wxTreeItemId hNext = GetNextSibling(hInsertAfter);
        if (!hNext.IsOk()) break;  // next item does not exist
        if (get_item_node(this, hNext)->type() == DAD_NODE_ELEMENT) break; // next item is not an attribute
        hInsertAfter = hNext;  
      } while (true);
    }
  }
  else
  { dad_element_node ** pel = &parel->sub_elements;
    while (*pel && *pel!=node) pel=&(*pel)->next;
    if (*pel) // node found
    { mode = IM_AFTER_SPECIF;
      item = stop2start(this, item);
      hInsertAfter = GetPrevSibling(item);
    }
    else
      mode = IM_LASTCHILD;
    newel->next=*pel;  *pel=newel;
  }
 // insert to the tree view:
  wxTreeItemId hNewItem = to_tree_ctrl(&GetConv(cdp), newel, this, hInsParent, mode, hInsertAfter);
  if (!from_clipboard)
    if (!item_action(hNewItem, newel, XML_EDIT))
      item_action(hNewItem, newel, XML_DELETE);
    else SelectItem(hNewItem);
  else SelectItem(hNewItem);
  changed=true;
}

void xml_designer::insert_attribute(wxTreeItemId item, dad_node * node, dad_attribute_node * newat, bool from_clipboard)
{// for a stop node, insert below corresponding start node instead, on the end
  wxTreeItemId hInsParent;
  if (node->type()==DAD_NODE_ELEMENT)
    hInsParent = stop2start(this, item);  
  else  // text, attribute
    hInsParent=GetItemParent(item);
 // find the element for inserting after and insert to the dad_tree:
  dad_element_node * parel = (dad_element_node*)get_item_node(this, hInsParent);
  wxTreeItemId hInsertAfter;  t_insert_mode mode;  wxTreeItemIdValue cookie;
  if (parel->attributes==node || !parel->attributes)  // insert before the first attribute because the 1st attribute is selected
  { mode=IM_FIRSTCHILD;
    newat->next=parel->attributes;  parel->attributes=newat;
  }
  else
  { dad_attribute_node ** pat = &parel->attributes;
    while (*pat && *pat!=node) pat=&(*pat)->next;
    mode=IM_AFTER_SPECIF;
    if (*pat) // node found
      hInsertAfter = GetPrevSibling(item);
    else  // insert as the last attribute, must search:
    { hInsertAfter = GetFirstChild(hInsParent, cookie);  // this must be an attribute item
      do
      { wxTreeItemId hNext = GetNextSibling(hInsertAfter);
        if (!hNext) break;  // next item does not exist
        if (get_item_node(this, hNext)->type() != DAD_NODE_ATTRIBUTE) break; // next item is not an attribute
        hInsertAfter = hNext;  
      } while (true);
    }
    newat->next=*pat;  *pat=newat;
  }
 // insert to the tree view:
  wxTreeItemId hNewItem = to_tree_ctrl(&GetConv(cdp), newat, this, hInsParent, mode, hInsertAfter);
  if (!from_clipboard)
    if (!item_action(hNewItem, newat, XML_EDIT))
      item_action(hNewItem, newat, XML_DELETE);
    else SelectItem(hNewItem);
  else SelectItem(hNewItem);
  changed=true;
}

void xml_designer::insert_text(wxTreeItemId item, dad_node * node, t_dad_column_node * newcol, bool from_clipboard)
{// for a stop node, insert below corresponding start node instead, on the end
  wxTreeItemId hInsParent;
  if (node->type()==DAD_NODE_ELEMENT)
    hInsParent = stop2start(this, item);  
  else  // text, attribute
    hInsParent=GetItemParent(item);
 // find the element for inserting after and insert to the dad_tree:
  dad_element_node * parel = (dad_element_node*)get_item_node(this, hInsParent);
  bool is_in_tree = parel->is_linked_to_column();
 // insert to the tree view:
  wxTreeItemId hNewItem;  wxTreeItemIdValue cookie;
 // copy data from parameter:
  if (newcol)
    newcol->copy_to(&parel->text_column);
  if (!is_in_tree)  // must insert text node to the tree
  { wxTreeItemId hInsertAfter;  t_insert_mode mode;
    if (!parel->attributes)
      mode=IM_FIRSTCHILD;
    else  // insert after the last attribute item
    { hInsertAfter = GetFirstChild(hInsParent, cookie);  // this must be an attribute item
      mode=IM_AFTER_SPECIF;
      do
      { wxTreeItemId hNext = GetNextSibling(hInsertAfter);
        if (!hNext) break;  // next item does not exist
        if (get_item_node(this, hNext)->type() != DAD_NODE_ATTRIBUTE) break; // next item is not an attribute
        hInsertAfter = hNext;  
      } while (true);
    }
    hNewItem = to_tree_ctrl(&GetConv(cdp), &parel->text_column, this, hInsParent, mode, hInsertAfter);
  }
  else // text node exists, find the node and update it:
  { wxTreeItemId ex_item;
    ex_item = GetFirstChild(hInsParent, cookie);  // this must be an attribute item
    while (get_item_node(this, ex_item)->type() != DAD_NODE_COLUMN)
      ex_item=GetNextSibling(ex_item);
    char buf[100+2*OBJ_NAME_LEN+1+sizeof(t_dad_col_name)+1+1];
    node->get_caption(buf, false, sizeof(buf));
    SetItemText(ex_item, wxString(buf, *cdp->sys_conv));
  }
 // edit:
  if (!from_clipboard && hNewItem)
    if (!item_action(hNewItem, &parel->text_column, XML_EDIT))
      item_action(hNewItem, &parel->text_column, XML_DELETE);
    else SelectItem(hNewItem);
  changed=true;
}

/////////////////////////////////// item-related actions /////////////////////////////////////////
void string2alloc(cdp_t cdp, wxString str, char *& chars)
{ if (chars) link_delete_chars(chars);
  if (str.Length())
  { chars = link_new_chars((int)str.Length()+1);
    if (chars!=NULL) strcpy(chars, str.mb_str(*cdp->sys_conv));
  }
  else chars=NULL;
}

bool xml_designer::item_action(wxTreeItemId item, dad_node * node, int cmd)
{ switch (cmd)
  {
    case XML_EDIT:
    { wxTreeItemId hStart = stop2start(this, item);
      hItem=item;  // dade->anode=node;  not used any more?
      switch (node->type())
      { case DAD_NODE_ELEMENT:  
        { XMLElementDlg dlg(this, (dad_element_node*)node);
          dlg.Create(dialog_parent());
          if (dlg.ShowModal() != wxID_OK) return false;
          break;
        }
        case DAD_NODE_ATTRIBUTE:
        { dad_attribute_node * atnode = (dad_attribute_node *)node;
          XMLAttributeDlg dlg(this, atnode->attr_name, wxString(atnode->attrcond, *cdp->sys_conv), 
            wxString(atnode->column.format, *cdp->sys_conv), wxString(atnode->column.fixed_val, *cdp->sys_conv),
            atnode->column.column_name, atnode->column.table_name, atnode->column.link_type, 
            atnode->column.inconv_fnc, atnode->column.outconv_fnc, atnode->column.is_update_key);
          dlg.Create(dialog_parent());
          if (dlg.ShowModal() != wxID_OK) return false;
         // save new values:
          strcpy(atnode->attr_name, dlg.attr_name);
          string2alloc(cdp, dlg.attrcond,  atnode->attrcond);
          string2alloc(cdp, dlg.format,    atnode->column.format);
          string2alloc(cdp, dlg.fixed_val, atnode->column.fixed_val);
          strcpy(atnode->column.column_name, dlg.column_name);
          strcpy(atnode->column.table_name, dlg.table_name);
          atnode->column.link_type=dlg.link_type;
          strcpy(atnode->column.inconv_fnc, dlg.inconv_fnc);  
          strcpy(atnode->column.outconv_fnc, dlg.outconv_fnc);
          atnode->column.is_update_key=dlg.updkey;
          break;
        }
        case DAD_NODE_COLUMN:
        { t_dad_column_node * cnode = (t_dad_column_node *)node;
          XMLTextDlg dlg(this, cnode->column_name, cnode->table_name, cnode->link_type, wxString(cnode->format, *cdp->sys_conv), 
                         wxString(cnode->fixed_val, *cdp->sys_conv), cnode->inconv_fnc, cnode->outconv_fnc, cnode->is_update_key);
          dlg.Create(dialog_parent());
          if (dlg.ShowModal() != wxID_OK) return false;
         // save new values:
          strcpy(cnode->column_name, dlg.column_name);
          strcpy(cnode->table_name, dlg.table_name);
          cnode->link_type=dlg.link_type;
          string2alloc(cdp, dlg.format,    cnode->format);
          string2alloc(cdp, dlg.fixed_val, cnode->fixed_val);
          strcpy(cnode->inconv_fnc, dlg.inconv_fnc);  
          strcpy(cnode->outconv_fnc, dlg.outconv_fnc);
          cnode->is_update_key=dlg.updkey;
          break;
        }
        case DAD_NODE_TOP:
        { XMLGlobalDlg dlg(this);
          dlg.Create(dialog_parent());
          if (dlg.ShowModal() != wxID_OK) return false;
          break;
        }
      }
     // update start tag:
      char buf[100+2*OBJ_NAME_LEN+1+sizeof(t_dad_col_name)+1+1];
      node->get_caption(buf, false, sizeof(buf));
      //bool was_expanded = IsExpanded(hStart);
      SetItemText(hStart, wxString(buf, *cdp->sys_conv));
      //if (was_expanded) Expand(hStart);  // SetItemText collapses -- no, the double click does this!
     // update the stop tag, too:
      wxTreeItemId hStop = start2stop(this, hStart);
      if (hStart!=hStop)
      { node->get_caption(buf, true, sizeof(buf));
        SetItemText(hStop, wxString(buf, *cdp->sys_conv));
      }
      changed=true;
      break;
    }
    case XML_CUT:
      if (clipboard_node) { delete clipboard_node;  clipboard_node=NULL; }
      //cont.
    case XML_DELETE:
    { wxTreeItemId hStart = stop2start(this, item);
     // find parent:
      wxTreeItemId hParentItem = GetItemParent(hStart);
      if (!hParentItem.IsOk() || hParentItem==GetRootItem()) 
        { wxBell();  break; }  // cannot delete/cut top items
      dad_element_node * parent_node = (dad_element_node*)get_item_node(this, hParentItem);
     // find the node among the children of the parent: 
      switch (node->type())
      { case DAD_NODE_ELEMENT:
        {// deleting element with subelements: ask
          int disp = wxID_YES;
          if (cmd!=XML_CUT && ((dad_element_node*)node)->sub_elements) 
          { wx602MessageDialog md(dialog_parent(), _("This element has sub-elements. Do you want to delete them as well?"), STR_WARNING_CAPTION, wxYES_NO | wxCANCEL);
            disp = md.ShowModal();
            if (disp==wxID_CANCEL) return false;  // action cancelled
          }
         // remove from the list:   
          dad_element_node ** pel = &parent_node->sub_elements;
          while (*pel && *pel!=node) pel=&(*pel)->next;
          if (!*pel) return false;  // internal error
          *pel=(*pel)->next;  // [node] removed from the list
          ((dad_element_node*)node)->next=NULL;
         // subelements: 
          if (cmd==XML_CUT) clipboard_node=node; 
          else if (disp == wxID_YES) delete node; // deleting (no subelements or deleting confirmed)
          else // move subelements up:
          { dad_element_node * subel_list = ((dad_element_node*)node)->sub_elements, * subel;
            while (subel_list)
            { subel=subel_list;  subel_list=subel_list->next;
              subel->next=*pel;
              *pel=subel;  
              pel=&(*pel)->next;
            }  
            ((dad_element_node*)node)->sub_elements = NULL;
           // destroy and re-create the tree
            delete node;
            DeleteAllItems();
            dad2tree(cdp, this, dad_tree);
            changed=true;
            return true;  // must not break and delete again
          }
          break;
        }
        case DAD_NODE_ATTRIBUTE:
        { dad_attribute_node ** pat = &parent_node->attributes;
          while (*pat && *pat!=node) pat=&(*pat)->next;
          if (!*pat) return false;  // internal error
          *pat=(*pat)->next;  // [node] removed from the list
          ((dad_attribute_node*)node)->next=NULL;
          if (cmd==XML_CUT) clipboard_node=node; else delete node;
          break;
        }
        case DAD_NODE_COLUMN:
          if (cmd==XML_CUT) clipboard_node=link_new_t_dad_column_node(*(t_dad_column_node*)node);  // copy constructor
          *parent_node->text_column.column_name=0;
          if (parent_node->text_column.fixed_val)
            { link_delete_chars(parent_node->text_column.fixed_val);  parent_node->text_column.fixed_val=NULL; }
          break;
        default:  // DAD_NODE_TOP
          return false;  // disabled
      }
     // delete item(s) from the tree:
      wxTreeItemId hStop = start2stop(this, hStart);  // must be before deleting hStart!
      Delete(hStart);
      if (hStart!=hStop) Delete(hStop);
      changed=true;
      break;
    }
    case XML_PASTE:
    { if (!clipboard_node) 
        { wxBell();  break; } // activated by the key
      bool stop_item = stop2start(this, item)!=item;
      if (clipboard_node->type()==DAD_NODE_ELEMENT && 
          (GetItemParent(item) || stop_item))  // unless root element start
        insert_element(item, node, (dad_element_node*)clipboard_node, true, stop_item ? XML_INS_ELEM : XML_INS_ELEM_BEFORE);
      else if (clipboard_node->type()==DAD_NODE_ATTRIBUTE)
        insert_attribute(item, node, (dad_attribute_node*)clipboard_node, true);
      else if (clipboard_node->type()==DAD_NODE_COLUMN)
      { insert_text(item, node, (t_dad_column_node*)clipboard_node, true);
        delete clipboard_node;
      }
      else 
        { wxBell();  break; }
      clipboard_node=NULL;
      break;
    }
    case XML_INS_ELEM:
    case XML_INS_ELEM_BEFORE:
      insert_element(item, node, NULL, false, cmd);
      break;
    case XML_INS_ATTR:
      insert_attribute(item, node, link_new_dad_attribute_node(), false);
      break;
    case XML_INS_TEXT:
      insert_text(item, node, NULL, false);
      break;
    case XML_INS_ALL:
    { item=stop2start(this, item);  // for stop-item, insert new nodes below the corresponding sart-item
      dad_element_node * elnode = (dad_element_node*)get_item_node(this, item);
      create_default_design_analytical(td, find_server_connection_info(!*elnode->dsn ? cdp->locid_server_name : elnode->dsn), 
        elnode->lower_tab, elnode->lower_schema, elnode->lower_alias, dad_tree, this, elnode, item);
      changed=true;
      Refresh();
      break;
    }
  } // cmd==0 if menu selection cancelled
  return true;
}

void xml_designer::OnActivated(wxTreeEvent & event)
{
  dad_node * node = get_item_node(this, event.GetItem());
  if ((int)(size_t)node >> 16)
  { item_action(event.GetItem(), node, XML_EDIT);
    fromdclick = true;
    event.Veto();  // no default processing of DBLCLK -- does not help
    event.Skip(false);  // does not help
  }
  
}

void xml_designer::OnExpanding(wxTreeEvent & event)
{
  if (fromdclick)
  { fromdclick = false;
    event.Veto();
    event.Skip(false);
  }
}

bool xml_designer::is_target_my_subnode(wxTreeItemId htiTarget)
// Returns true iff htiTarget is same or subnode of dade->dragged_item or is a root node except root element stop.
{// first, check for root element start:
  if (!GetItemParent(htiTarget).IsOk() && stop2start(this, htiTarget)==htiTarget)
    return true;
  if (htiTarget==start2stop(this, dragged_item) || htiTarget==stop2start(this, dragged_item))
    return true;
  wxTreeItemId hI = htiTarget;
  while (hI.IsOk() && hI!=GetRootItem() && hI!=dragged_item)
    hI=GetItemParent(hI);
  return hI==dragged_item;
}

bool xml_designer::can_drop_endnode_here(wxTreeItemId htiTarget)
{
 // check for target being a start item of dragged item's direct child node:
  if (htiTarget!=start2stop(this, htiTarget) &&  // target is a start item
      GetItemParent(htiTarget) == dragged_item) // target is my child (dragged_item is not the dragged item, it is its start node)
  { if (!GetItemParent(stop2start(this, dragged_item))) return false;  // dragging the end item of the root element, the following elements would be unaccessible
    return true;
  }
 // check for target being the start item of a following sibling:
  if (htiTarget!=start2stop(this, htiTarget))  // target is a start item
  { wxTreeItemId hI = dragged_item;
    do
    { hI=GetNextSibling(hI);
      if (hI == htiTarget) return true;
    }
    while (hI);
  }
 // check for target being the end item of my parent:
  wxTreeItemId hI = stop2start(this, htiTarget);
  if (hI!=htiTarget &&  // target is an end item
      GetItemParent(dragged_item)==hI)  // and is end of my parent
    return true;
  return false;
}

void xml_designer::move_endnode_here(wxTreeItemId htiTarget)
{
 // check for target being a start item of dragged item's direct child node:
  if (htiTarget!=start2stop(this, htiTarget) &&  // target is a start item
      GetItemParent(htiTarget) == dragged_item) // target is my child (dragged_item is not the dragged item, it is its start node)
  { 
   // cut children and paste them to parent:
    wxTreeItemId hPasteTarget = GetNextSibling(start2stop(this, dragged_item));
    if (!hPasteTarget.IsOk()) hPasteTarget = start2stop(this, GetItemParent(dragged_item));
    wxTreeItemId hChild = htiTarget;
    do
    { wxTreeItemId hNextChild=GetNextSibling(GetNextSibling(hChild));
      item_action(hChild, get_item_node(this, hChild), XML_CUT);
      item_action(hPasteTarget, get_item_node(this, hPasteTarget), XML_PASTE);
      hChild=hNextChild;
    } while (hChild);
    return;
  }
 // check for target being the start item of a following sibling:
  if (htiTarget!=start2stop(this, htiTarget))  // target is a start item
  { wxTreeItemId hI = dragged_item;
    do
    { hI=GetNextSibling(hI);
      if (hI == htiTarget) 
      {// cut following siblings before htiTarget and paste it inside dragged_item
        hI=GetNextSibling(start2stop(this, dragged_item));
        while (hI && hI != htiTarget)
        { wxTreeItemId hNextSibl=GetNextSibling(GetNextSibling(hI));
          item_action(hI, get_item_node(this, hI), XML_CUT);
          item_action(start2stop(this, dragged_item), dragged_node, XML_PASTE);
          hI=hNextSibl;
        }
        return;
      }
    } while (hI);
    return;
  }
 // check for target being the end item of my parent:
  wxTreeItemId hI = stop2start(this, htiTarget);
  if (hI!=htiTarget &&  // target is an end item
      GetItemParent(dragged_item)==hI)  // and is end of my parent
  {
    hI=GetNextSibling(start2stop(this, dragged_item));
    while (hI)
    { wxTreeItemId hNextSibl=GetNextSibling(GetNextSibling(hI));
      item_action(hI, get_item_node(this, hI), XML_CUT);
      item_action(start2stop(this, dragged_item), dragged_node, XML_PASTE);
      hI=hNextSibl;
    }
  }
}

bool xml_designer::show_popup_menu(wxTreeItemId item, dad_node * node, wxPoint pt)
// Shows popup menu for the item and performs the selected action. Returns 1 if menu shown, 0 otherwise.
{ bool limited_operation=false;
  if ((int)(size_t)node >> 16)
  { wxMenu * popup_menu = new wxMenu;              
    switch (node->type())
    { case DAD_NODE_ELEMENT:  
      { bool stop_item = stop2start(this, item)!=item;  wxString menutext;
        dad_element_node * el_node = (dad_element_node*)node;
        AppendXpmItem(popup_menu, XML_EDIT,   _("&Edit\tEnter"),   dad_edit_xpm);
        popup_menu->AppendSeparator();
        AppendXpmItem(popup_menu, XML_CUT,    _("Cu&t\tCtrl+X"),   cut_xpm);
        if (el_node->recursive)
          limited_operation=true;
        else
          AppendXpmItem(popup_menu, XML_PASTE,  _("&Paste\tCtrl+V"), Vlozit_xpm);
        AppendXpmItem(popup_menu, XML_DELETE, _("&Delete\tDel"),   dad_delete_xpm);
        if (!el_node->recursive)
        { popup_menu->AppendSeparator();
          AppendXpmItem(popup_menu, XML_INS_ELEM_BEFORE, _("Insert Element Before this Element"), dad_element_xpm);
          menutext.Printf(_("Insert &Sub-element of Element <%s>"), wxString(el_node->el_name, *cdp->sys_conv).c_str());
          AppendXpmItem(popup_menu, XML_INS_ELEM, menutext, dad_element_xpm);
          menutext.Printf(_("Insert &Attribute of Element <%s>"),  wxString(el_node->el_name, *cdp->sys_conv).c_str());
          AppendXpmItem(popup_menu, XML_INS_ATTR, menutext, dad_attribute_xpm);
          menutext.Printf(_("Insert Te&xt of Element <%s>"),       wxString(el_node->el_name, *cdp->sys_conv).c_str());
          AppendXpmItem(popup_menu, XML_INS_TEXT, menutext, dad_text_xpm);
          popup_menu->AppendSeparator();
          AppendXpmItem(popup_menu, XML_INS_ALL, _("Insert sub-elements for all columns"));
        }
        break;
      }
      case DAD_NODE_ATTRIBUTE:
      case DAD_NODE_COLUMN:
        AppendXpmItem(popup_menu, XML_EDIT,   _("&Edit\tEnter"),   dad_edit_xpm);
        popup_menu->AppendSeparator();
        AppendXpmItem(popup_menu, XML_CUT,    _("Cu&t\tCtrl+X"),   cut_xpm);
        AppendXpmItem(popup_menu, XML_PASTE,  _("&Paste\tCtrl+V"), Vlozit_xpm);
        AppendXpmItem(popup_menu, XML_DELETE, _("&Delete\tDel"),   dad_delete_xpm);
        popup_menu->AppendSeparator();
        AppendXpmItem(popup_menu, XML_INS_ATTR, _("Insert &Attribute"),  dad_attribute_xpm);
        break;
      case DAD_NODE_TOP:
        AppendXpmItem(popup_menu, XML_EDIT,   _("&Edit\tEnter"),   dad_edit_xpm);
        break;
      default:
        delete popup_menu;  return false;  // allows the default processing
    }
   // load menu and update its items:
    if (!limited_operation)
    { if (clipboard_node==NULL && node->type()!=DAD_NODE_TOP)
        popup_menu->Enable(XML_PASTE, false);
      if (node->type()==DAD_NODE_ELEMENT)  // disable if text column already exists:
      { wxTreeItemId hInsParent = GetItemParent(item);
       // disable operation with the root item
        if (!hInsParent.IsOk() || hInsParent==GetRootItem())  // root element, disable all except EDIT
        { popup_menu->Enable(XML_DELETE, false);
          popup_menu->Enable(XML_CUT, false);
          popup_menu->Enable(XML_PASTE, false);
          popup_menu->Enable(XML_INS_ELEM_BEFORE, false);
          //popup_menu->Enable(XML_INS_ELEM, false);
          //popup_menu->Enable(XML_INS_ATTR, false);
          //popup_menu->Enable(XML_INS_TEXT, false);
        }
       // if text column exists, disable inserting it:
        if (((dad_element_node*)node)->is_linked_to_column())  
          popup_menu->Enable(XML_INS_TEXT, false);
       // inserting all columns is enabled for table nodes only:
        if (!*((dad_element_node*)node)->lower_tab)  
          popup_menu->Enable(XML_INS_ALL, false);
      }
    }
   // track the menu:
    PopupMenu(popup_menu, pt);
    delete popup_menu;
  }
  return true;
}

bool xml_designer::select_node_by_num(wxTreeItemId hParItem, int & err_node_num)
{ wxTreeItemId item;  wxTreeItemIdValue cookie;
  item = hParItem.IsOk() ? GetFirstChild(hParItem, cookie) : GetRootItem();
  while (item.IsOk())
  { if (!err_node_num) 
      { SelectItem(item==GetRootItem() ? GetFirstChild(item, cookie) : item);  return true; }  // found here
    err_node_num--;
    if (select_node_by_num(item, err_node_num)) return true;  // found below
    dad_node * node = get_item_node(this, item);
    item = GetNextSibling(item);
    if (node->type() == DAD_NODE_ELEMENT) // skip the terminating tag of the item
      item = GetNextSibling(item);
  }
  return false;
}

void xml_designer::OnRightButtonDown(wxMouseEvent & event)
// Selects the item
{ int flags;
  wxTreeItemId item = HitTest(ScreenToClient(wxGetMousePosition()), flags);
  if (flags & (wxTREE_HITTEST_ONITEMBUTTON | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL/*wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT*/))
  // VK_APPS is translated to right mouse click: minimising the damage by limiting the positions of the mouse
  { dad_node * node = get_item_node(this, item);
    SelectItem(item);
    //if (!show_popup_menu(item, node, ScreenToClient(wxGetMousePosition())))
    //  event.Skip();
  }
  event.Skip();
}

#ifdef LINUX
void xml_designer::OnRightClick(wxMouseEvent & event)
{ wxPoint position = wxPoint(event.m_x, event.m_y);
  int flags;
  wxTreeItemId item = HitTest(position, flags);
  // VK_APPS is translated to right mouse click: minimising the damage by limiting the positions of the mouse
  if (item.IsOk() && (flags & (wxTREE_HITTEST_ONITEMBUTTON | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL/*wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT*/)))
#else
void xml_designer::OnRightClick(wxTreeEvent & event)
{ wxPoint position = ScreenToClient(wxGetMousePosition());
  wxTreeItemId item = event.GetItem();
  // VK_APPS is translated to right mouse click: minimising the damage by limiting the positions of the mouse
  if (item.IsOk())
#endif
  { dad_node * node = get_item_node(this, item);
    if (!show_popup_menu(item, node, position))
      event.Skip();
  }
  else event.Skip();
}

void xml_designer::OnKeyDown(wxTreeEvent & event)
{ wxTreeItemId item = GetSelection();
  if (item.IsOk())
  { dad_node * node = get_item_node(this, item);
    if ((int)(size_t)node >> 16)
      switch (event.GetKeyCode())
      { case WXK_DELETE:
          item_action(item, node, XML_DELETE);  return;
        case 'X':
          if (event.GetKeyEvent().ControlDown())
            { item_action(item, node, XML_CUT);  return; }
          break;
        case 'V':
          if (event.GetKeyEvent().ControlDown())
            { item_action(item, node, XML_PASTE);  return; }
          break;
#if NEVER  // Return generates the activation event which calls item_action()
        case WXK_RETURN:
          item_action(item, node, XML_EDIT);  return;
#endif
        case WXK_WINDOWS_MENU:
        { wxRect rect;
          GetBoundingRect(item, rect, true);
          show_popup_menu(item, node, wxPoint(rect.GetX()+16, rect.GetY()+8));  
          return;
        }
        default:
          break;
      }
  }
  event.Skip(!OnCommonKeyDown((wxKeyEvent &)event.GetKeyEvent()));  // not said if calling Skip() in event.GetKeyEvent() affects the [event]
}

///////////////////////////////// initial design ////////////////////////////////
static void fill_elements_for_columns(cdp_t cdp, dad_element_node * rowel, const d_table * td, 
                                      wxTreeCtrl * tree, wxTreeItemId hRowItem)
// Inserts elements for all columns from [td] into [rowel] and into [tree] under [hRowItem] iff [tree] is specified.
// [cdp] is NULL fro ODBC data sources.
{ dad_element_node ** plist = &rowel->sub_elements;
  const d_attr * pdatr;  int i;
  for (i=first_user_attr(td), pdatr=ATTR_N(td,i);  i<td->attrcnt;  i++, pdatr=NEXT_ATTR(td,pdatr))
    if (pdatr->multi==1)
    { dad_element_node * col_el = link_new_dad_element_node();
      strcpy(col_el->el_name, pdatr->name);
      if (cdp) ToASCII(col_el->el_name, cdp->sys_charset);
      wb_small_string(cdp, col_el->el_name, false);
      strcpy(col_el->text_column.column_name, pdatr->name);
      wb_small_string(cdp, col_el->text_column.column_name, false);
      strcpy(col_el->text_column.table_name, rowel->lower_table_name());
      col_el->next = *plist;  *plist = col_el;  plist=&col_el->next;
      if (tree)
        to_tree_ctrl(&GetConv(cdp), col_el, tree, hRowItem);
    }
}

void create_default_design(cdp_t cdp, const d_table * td, t_dad_root * dad_tree, wxTreeCtrl * tree, const char * server)
// Inserts default export of td to dad_tree. If tree!=NULL, updates the tree view too.
{ dad_element_node * rowel = link_new_dad_element_node();
  wxTreeItemId hRowItem;  wxTreeItemIdValue cookie;
  strcpy(rowel->el_name, "row");
  strcpy(rowel->lower_tab, base_cursor_name8);
  dad_tree->root_element->sub_elements=rowel;
#if WBVERS>=1000
  strmaxcpy(dad_tree->top_dsn, server, sizeof(dad_tree->top_dsn));
#endif
  if (tree)
    hRowItem = to_tree_ctrl(&GetConv(cdp), rowel, tree, tree->GetNextSibling(tree->GetFirstChild(tree->GetRootItem(), cookie)));
 // insert elements for columns:
  fill_elements_for_columns(cdp, rowel, td, tree, hRowItem);
}

void create_default_design_analytical(const d_table * top_td, const t_avail_server * as, const char * tablename, const char * schemaname, 
                const char * alias, t_dad_root * dad_tree, wxTreeCtrl * tree, dad_element_node * rowel, wxTreeItemId hRowItem)
// Inserts default export of [tablename] columns to dad_tree. If [tree]!=NULL, updates the tree view too.
{ bool td_created_locally = false;
  if (!as) return;  // fuse
  if (as->odbc ? !as->conn : !as->cdp) return;  // DS not connected
 // prepare the [td]:
  const d_table * td = NULL;
  if (!stricmp(tablename, base_cursor_name8))
    td = top_td;  // base
  else if (as->odbc)
  { td = make_odbc_descriptor9(as->conn, tablename, schemaname, TRUE);
    td_created_locally = true; 
  }
  else
  { ttablenum tbnum;
    if (!cd_Find_prefixed_object(as->cdp, schemaname, tablename, CATEG_TABLE, &tbnum))
      td = cd_get_table_d(as->cdp, tbnum, CATEG_TABLE);
    td_created_locally = true; 
  }
  if (td)
  {// create the "row" element if it does not exist: 
    if (!rowel)
    { rowel = link_new_dad_element_node();
      wxTreeItemIdValue cookie;
      strmaxcpy(rowel->el_name, td->selfname, sizeof(rowel->el_name));
      if (as->cdp) ToASCII(rowel->el_name, as->cdp->sys_charset);
      strmaxcpy(rowel->lower_tab, td->selfname, sizeof(rowel->lower_tab));
      strmaxcpy(rowel->lower_alias, alias, sizeof(rowel->lower_alias));
      wb_small_string(as->cdp, rowel->lower_tab, true);
      strmaxcpy(rowel->lower_schema, schemaname, sizeof(rowel->lower_schema));
      strmaxcpy(rowel->dsn, as->locid_server_name, sizeof(rowel->dsn));
      if (tree)
        hRowItem = to_tree_ctrl(&GetConv(as->cdp), rowel, tree, tree->GetNextSibling(tree->GetFirstChild(tree->GetRootItem(), cookie)));
      dad_tree->root_element->sub_elements=rowel;
    }
   // insert elements for columns:
    fill_elements_for_columns(as->cdp, rowel, td, tree, hRowItem);
    if (td_created_locally)
      if (as && as->odbc)
        corefree(td);
      else
        release_table_d(td);  
  }
}


////////////////////////////////////////////////////////////////////////
#ifdef WINS
extern const char default_prolog[] = "version=\"1.0\" encoding=\"windows-1250\"";
#else
extern const char default_prolog[] = "version=\"1.0\" encoding=\"UTF-8\"";
#endif

d_table * get_expl_descriptor(cdp_t cdp, const char * sql_stmt, const char * dsn, xml_designer * dsgn, t_ucd_cl ** p_stmt_ucdp, tcursnum * top_curs)
// Creates [stmt_ucdp], [top_curs] and [td] for [sql_stmt] in [dsn]. When dsn is not specified, uses [cdp].
// When [dsgn]!=NULL uses the parameters from it. 
{ tcursnum cursnum;  d_table * new_td;  
  *p_stmt_ucdp=NULL;  t_ucd_cl * stmt_ucdp;  *top_curs=NOOBJECT;
 // prepare stmt_ucdp:
  if (dsn && *dsn)
  { xcd_t * xcdp = find_connection(dsn);
    if (!xcdp) return NULL;
    if (xcdp->get_odbcconn())  // ODBC
      stmt_ucdp = link_new_t_ucd_cl_odbc(xcdp->get_odbcconn(), cdp);
    else
      stmt_ucdp = link_new_t_ucd_cl_602(xcdp->get_cdp());
  }
  else 
    stmt_ucdp = link_new_t_ucd_cl_602(cdp);

  if (dsgn && dsgn->dad_tree->params.count())
    dsgn->materialize_params(stmt_ucdp);
  cursnum=NOOBJECT;  // ODBC open_cursor needs this
  if (stmt_ucdp->open_cursor(sql_stmt, &cursnum))
    { cd_Signalize(cdp);  link_delete_t_ucd_cl(stmt_ucdp);  return NULL; }
  if (dsgn && dsgn->dad_tree->params.count())
    stmt_ucdp->dematerialize_params();
  if (stmt_ucdp->ucd_type()==UCL_CLI_602 && !IS_CURSOR_NUM(cursnum)) 
    { link_delete_t_ucd_cl(stmt_ucdp);  return NULL; }
  new_td = (d_table *)stmt_ucdp->get_table_d(CATEG_DIRCUR, NULL, NULL, cursnum);
  if (!new_td)
    { cd_Signalize(cdp);  stmt_ucdp->close_cursor(cursnum);  link_delete_t_ucd_cl(stmt_ucdp);  return NULL; }
  //stmt_ucdp->close_cursor(cursnum);  -- must not close the cursor before releasing the [td], passing cursor ownership to the designer instead
  *top_curs=cursnum;
  *p_stmt_ucdp=stmt_ucdp;
#if 0
  BOOL res;  uns32 result[5];  // 5 to be safe
  if (dsgn && dsgn->dad_tree->params.count())
  { _t_ucd_cl cdcl(cdp);
    dsgn->materialize_params(cdcl.x());
    res=cd_SQL_host_execute(cdp, sql_stmt, result, cdcl.x()->hostvars, cdcl.x()->hostvars_count);
    cdcl.x()->dematerialize_params();
  }
  else 
    res=cd_SQL_execute(cdp, sql_stmt, result);  // scans for client variables
  //if (cd_Open_cursor_direct(cdp, dad_tree->sql_stmt, &topcurs)) 
  if (res)
    { cd_Signalize(cdp);  return NULL; }
  if (!IS_CURSOR_NUM(result[0])) return NULL;
  cursnum=result[0];
  if (cd_Get_descriptor(cdp, cursnum, CATEG_DIRCUR, &new_td))
    { cd_Signalize(cdp);  cd_Close_cursor(cdp, cursnum);  return NULL; }
  cd_Close_cursor(cdp, cursnum);
#endif
  return new_td;
}

wxMenu * xml_designer::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items: 
  any_designer::get_designer_menu();  
 // add specific items:
  AppendXpmItem(designer_menu, TD_CPM_XML_PARAMS,  _("&Parameters"),           params_xpm);
  AppendXpmItem(designer_menu, TD_CPM_OPTIONS,     _("&Global Properties"),    xml_properties_xpm);
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_XML_IMPORT,  _("&Import from XML File"), xml_import_xpm);
  AppendXpmItem(designer_menu, TD_CPM_XML_EXPORT,  _("&Export to XML File"),   xml_export_xpm);
  AppendXpmItem(designer_menu, TD_CPM_XML_TESTEXP, _("&Test Export"),          xml_test_xpm);
  return designer_menu;
}

void xml_designer::err_callback(const void *Owner, int ErrCode, DSPErrSource ErrSource, const wchar_t *ErrMsg)
{ //::MessageBoxW((HWND)((xml_designer *)Owner)->GetParent()->GetHWND(), ErrMsg, _("Error"), MB_OK | MB_ICONERROR);
   info_box(ErrMsg, ((xml_designer *)Owner)->GetParent());
}

int xml_designer::load_dad_design(t_ucd_cl * cdcl)
{ char * def = cd_Load_objdef(cdp, objnum, CATEG_PGMSRC, NULL);
  if (!def) return 2;
  def=link_convert2utf8(def);
  dad_tree = link_parse(cdcl, def);
  corefree(def);
  if (!dad_tree) { cd_Signalize(cdp);  return 3; }  // Create not called, "this" is not a child of [parent], must delete
  if (!dad_tree->root_element)
    { delete dad_tree;  dad_tree=NULL;  return 4; }  // Create not called, "this" is not a child of [parent], must delete
 // create td if explicit sql statement specified:
  if (dad_tree->explicit_sql_statement)
    td = get_expl_descriptor(cdp, dad_tree->sql_stmt, dad_tree->top_dsn, this, &top_ucdp, &top_curs);
  return 0;
}

bool xml_designer::open(wxWindow * parent, t_global_style my_style)
// [wpg]!=NULL for a new design.
{ int res=0;
  _t_ucd_cl cdcl(cdp);
  if (!link_init_platform_utils(cdcl.x())) { cd_Signalize(cdp);  return false; }
  if (modifying())
  { res=load_dad_design(cdcl.x());
    if (res) goto ex;
    cd_Read(cdp, OBJ_TABLENUM, objnum, OBJ_NAME_ATR, NULL, object_name);
  }
  else if (wpg->xml_init!=XST_FO) // new design: create initial state, except for FO
  { dad_tree = link_new_t_dad_root(cdp->sys_charset);
    dad_tree->prolog=link_new_chars((int)strlen(default_prolog)+1);
    if (dad_tree->prolog) strcpy(dad_tree->prolog, default_prolog);
    dad_tree->root_element = link_new_dad_element_node();
    if (dad_tree->root_element)
      strcpy(dad_tree->root_element->el_name, "root");
    *object_name=0;
  }
 // open the window:
  if (!Create(parent, COMP_FRAME_ID, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT))
    res=5;  // window not opened
  else 
  { competing_frame::focus_receiver = this;
    wxImageList * il = new wxImageList(16, 16, TRUE, 5);
    { wxBitmap bmp(elem_r_xpm);      il->Add(bmp); }
    { wxBitmap bmp(xml_atribut_xpm); il->Add(bmp); }
    { wxBitmap bmp(elem_text_xpm);   il->Add(bmp); }
    { wxBitmap bmp(xmlinfo_xpm);     il->Add(bmp); }
    { wxBitmap bmp(elem_l_xpm);      il->Add(bmp); }
    AssignImageList(il);  // takes ownership
#ifdef LINUX
    SetOwnBackgroundColour(wxGetApp().frame->GetDefaultAttributes().colBg);   // makes the backgroud consistent (otherwise in some themes text have different background than the tree view)
#endif
   // initial design:
    if (!modifying())
    { 
      if (wpg->xml_init==XST_TABLE)
      { create_default_design_analytical(td, find_server_connection_info(*wpg->server ? wpg->server: cdp->locid_server_name), 
          wpg->table_name, *wpg->schema ? wpg->schema : schema_name, wpg->table_alias, dad_tree, NULL, NULL, wxTreeItemId());
        changed=true;
      }
      else if (wpg->xml_init==XST_QUERY)
      { td=wpg->new_td;  // [td] created in the dialog when checking the explicit statement, take it!
        top_ucdp=wpg->new_ucdp;
        top_curs=wpg->new_top_curs;
        dad_tree->sql_stmt = link_new_chars((int)wpg->query.Length()+1);
        if (dad_tree->sql_stmt) strcpy(dad_tree->sql_stmt, wpg->query.mb_str(*cdp->sys_conv));
        dad_tree->explicit_sql_statement=true;
        create_default_design(cdp, td, dad_tree, NULL, wpg->server);
        changed=true;
      }
#if WBVERS>=950
      else if (wpg->xml_init==XST_XSD)
      { char schema_file[MAX_PATH];
        strcpy(schema_file, wpg->query.mb_str(*cdp->sys_conv));
        bool ok;
        { wxBusyCursor wait;
          ok = link_create_default_schema_design(cdp, this, schema_file, schema_name, folder_name, dad_tree, gen_params, err_callback);
        }
        if (!ok)
          { res=7;  goto ex; }
        if (!dad_tree->root_element)
          { error_box(_("The schema does not contain a root element."), parent);  res=8;  goto ex; }
        changed=true;
      }
      else if (wpg->xml_init==XST_FO)
      { char fo_file[MAX_PATH];
        strcpy(fo_file, wpg->query.mb_str(*cdp->sys_conv));
        { wxBusyCursor wait;  HANDLE Handle;  bool err;
          if (link_DSParserOpen(cdp, &Handle)) 
            { res=9;  goto ex; }
          link_DSParserSetStrProp(Handle, DSP_SCHEMANAME, schema_name);
          link_DSParserSetStrProp(Handle, DSP_FOLDERNAME, folder_name);
          link_DSParserSetStrProp(Handle, DSP_TABLEPREFIX, gen_params->tabname_prefix);
          link_DSParserSetStrProp(Handle, DSP_DADNAME, gen_params->dad_name);
          link_DSParserSetIntProp(Handle, DSP_DEFSTRLENGTH, gen_params->string_length);
          link_DSParserSetIntProp(Handle, DSP_UNICODE, gen_params->unicode_strings);
          link_DSParserSetIntProp(Handle, DSP_IDTYPE, gen_params->id_type);
          link_DSParserSetIntProp(Handle, DSP_REFINT, gen_params->refint);
          if (link_DSParserParse(Handle, fo_file))
            err=true;
          else err=link_DSParserCreateObjects(Handle)!=0;
          if (err)  // report the error
          { const char * errmsg;
            if (!link_DSParserGetStrProp(Handle, DSP_ERRMSG, &errmsg))
              error_box(wxString(errmsg, *wxConvCurrent));  // not owning the msg!
          }
          link_DSParserClose(Handle);
          if (!err)
          { cd_Find_object(cdp, gen_params->dad_name, CATEG_PGMSRC, &objnum);
            strcpy(object_name, gen_params->dad_name);
            lock_the_object();
            res=load_dad_design(cdcl.x());
            if (res) goto ex;
          }
          else
            { res=10;  goto ex; }
        }
      }
#endif
      else if (wpg->xml_init==XST_EMPTY)  // empty desing selected
      { 
      }
    }
   // convert the memory structure to the contents of the tree control:
    dad2tree(cdp, this, dad_tree);
    return true;  // OK
  }
 ex:
  link_close_platform_utils();
  return res==0;
}
/////////////////////////// virtual methods (commented in class any_designer): //////////////////////////////////
bool xml_designer::IsChanged(void) const
{ return changed; }

wxString xml_designer::SaveQuestion(void) const
{ return modifying() ?
    _("Your changes have not been saved. Do you want to save your changes?") :
    _("Your DAD design has not been saved. Do you want to save your design?");
}

bool xml_designer::save_design(bool save_as)
// Saves the domain, returns true on success.
{ wxBusyCursor wait;
  if (!save_design_generic(save_as, object_name, CO_FLAG_XML))  // using the generic save from any_designer
    return false;  
  changed=false;
  set_designer_caption();
  return true;
}

wxString get_xml_filter(void)
{ 
  wxString filter = _("XML files (*.xml)|*.xml|");
#ifdef WINS
  return filter+_("All files|*.*");
#else
  return filter+_("All files|*");
#endif
  return filter;
}

void xml_designer::OnCommand(wxCommandEvent & event)
{ 
  switch (event.GetId())
  { case TD_CPM_SAVE:
      if (!IsChanged()) break;  // no message if no changes
      if (!save_design(false)) break;  // error saving, break
      info_box(_("Your design has been saved."), dialog_parent());
      break;
    case TD_CPM_SAVE_AS:
      if (!save_design(true)) break;  // error saving, break
      info_box(_("Your design was saved under a new name."), dialog_parent());
      break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
    case TD_CPM_EXIT_FRAME:
      if (exit_request(this, true))
        destroy_designer(GetParent(), &xmldsng_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
      exit_request(this, false);
      destroy_designer(GetParent(), &xmldsng_coords);  // must not call Close, Close is directed here
      break;
    case TD_CPM_CHECK:
    { char * src = make_source(false);
      if (!src) break;
     // try to compile the source:
      _t_ucd_cl cdcl(cdp);
      src=link_convert2utf8(src);
      t_dad_root * dad_tree = link_parse(cdcl.x(), src);
      corefree(src);
      if (!dad_tree)
        cd_Signalize(cdp);
      else  // parsing OK
      { t_xml_context * xc = link_new_xml_context(dad_tree);  xc->ucdp=cdcl.x();
        xc->for_output=true;
        materialize_params(cdcl.x());
        { wxBusyCursor wait;
          xc->analyze_tree(NOOBJECT);
        }
        cdcl.x()->dematerialize_params();
        if (xc->err==XMLE_OK)
          info_box(_("The design is valid."), dialog_parent());
        else 
        { select_node_by_num(wxTreeItemId(), xc->err_node_num);
          cd_Signalize2(cdp, dialog_parent());
        }
        delete dad_tree;
		delete xc;
      }
      break;
    }
    case TD_CPM_XML_PARAMS:
    { XMLParamsDlg dlg(this);
      // wxTAB_TRAVERSAL causes the dialog to grab Return key and makes impossible to edit in the grid!!
      dlg.Create(dialog_parent(), -1);
      dlg.SetWindowStyleFlag(dlg.GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);
      dlg.ShowModal();
      break;
    }
    case TD_CPM_OPTIONS:
    { wxTreeItemId item;  wxTreeItemIdValue cookie;
      item = GetFirstChild(GetRootItem(), cookie);
      item_action(item, get_item_node(this, item), XML_EDIT);
      break;
    }
    case TD_CPM_XML_IMPORT:
    { wxString fname = GetImportFileName(_("*.xml"), get_xml_filter(), GetParent(), _("Select input XML file"));
      if (fname.IsEmpty()) break;
      char * src = make_source(false);  BOOL res;
      if (!src) break;
      src=link_convert2utf8(src);
      _t_ucd_cl cdcl(cdp);  int err_node_num;
      materialize_params(cdcl.x());
      { wxBusyCursor wait;
        res=link_import_from_file(cdcl.x(), src, fname.mb_str(*wxConvCurrent), &err_node_num, NULL);
      }
      if (!res)
      { if (err_node_num!=-1) select_node_by_num(wxTreeItemId(), err_node_num);
        cd_Signalize2(cdp, dialog_parent());
      }
      else info_box(_("Import complete."), dialog_parent());
      cdcl.x()->dematerialize_params();
      corefree(src);
      break;
    }
    case TD_CPM_XML_EXPORT:
    { wxString ifname;  BOOL res;
     // import file:
      if (*dad_tree->dad2)
      { ifname = GetImportFileName(_("*.xml"), get_xml_filter(), GetParent(), _("Select input XML file"));
        if (ifname.IsEmpty()) break;
      }
     // export file:
      wxString fname = GetExportFileName(wxEmptyString, get_xml_filter(), GetParent(), _("Select output XML file"));
      if (fname.IsEmpty()) break;
      if (!can_overwrite_file(fname.mb_str(*wxConvCurrent), NULL)) break; /* file exists, overwrite not confirmed */
      char * src = make_source(false);  
      if (!src) break;
      src=link_convert2utf8(src);
      _t_ucd_cl cdcl(cdp);  int err_node_num;
      materialize_params(cdcl.x());
      if (*dad_tree->dad2)
      { wxBusyCursor wait;  char * buf = NULL;
        res = link_import_from_file(cdcl.x(), src, ifname.mb_str(*wxConvCurrent), &err_node_num, &buf);
        if (res)  // write the buffer to the output file
        { t_file_outbuf * fo = link_new_t_file_outbuf();
          if (fo->open(fname.mb_str(*wxConvCurrent)))
          { fo->put(buf);
            fo->close();
          }
          link_delete_t_outbuf(fo);
        }
        corefree(buf);
      }
      else
      { wxBusyCursor wait;  
        t_file_outbuf * fo = link_new_t_file_outbuf();
        res=link_export_to_file(cdcl.x(), src, *fo, fname.mb_str(*wxConvCurrent), NOOBJECT, &err_node_num);
        link_delete_t_outbuf(fo);
      }
      if (!res)
      { if (err_node_num!=-1) select_node_by_num(wxTreeItemId(), err_node_num);
        cd_Signalize2(cdp, dialog_parent());
      }
      else info_box(_("Export complete."), dialog_parent());
      cdcl.x()->dematerialize_params();
      corefree(src);
      break;
    }
    case TD_CPM_XML_TESTEXP:
    { wxString ifname;  int enc_code;  char * buf;  BOOL res;
      if (*dad_tree->dad2)
      { ifname = GetImportFileName(_("*.xml"), get_xml_filter(), GetParent(), _("Select input XML file"));
        if (ifname.IsEmpty()) break;
        enc_code = XMLCODE_UTF8;  // required
        buf = NULL;  // will be allocated by the export
      }
      else
      { enc_code = dad_tree->get_encoding_from_prolog();
        buf = (char*)sigalloc(30000+16, 77);
        if (!buf) break;
      }
      char * src = make_source(false);  
      if (!src) { corefree(buf);  break; }
      src=link_convert2utf8(src);
      int extent;
      _t_ucd_cl cdcl(cdp);  int err_node_num;
      materialize_params(cdcl.x());
      
      if (*dad_tree->dad2)
      { wxBusyCursor wait;
        res = link_import_from_file(cdcl.x(), src, ifname.mb_str(*wxConvCurrent), &err_node_num, &buf);
        extent=0;  // prevents appending the "..."
      }
      else
      { wxBusyCursor wait;
        t_mem_outbuf * fo = link_new_t_mem_outbuf(buf, 30000);  
        res = link_export_to_memory(cdcl.x(), src, *fo, NOOBJECT, &err_node_num);
        fo->close(&extent);
        link_delete_t_outbuf(fo);
      }
     // display the result:
      if (!res) 
      { if (err_node_num!=-1) select_node_by_num(wxTreeItemId(), err_node_num);
        cd_Signalize2(cdp, dialog_parent());
      }
      else
      { if (extent>30000)
          if (buf[1]) strcpy(buf+30000, "...");
#if WINS
          else wuccpy((wuchar*)(buf+30000), L"...");
#else
          else wcscpy((wchar_t*)(buf+30000), L"...");
#endif
        TextOutputDlg dlg(dialog_parent());

        int charlen;
        if (enc_code==XMLCODE_UCS2)
          charlen=(int)wuclen((wuchar*)buf);
        else
          charlen=(int)strlen(buf);
        wxChar * ubuf = new wxChar[charlen+1];
        if (ubuf)
        { t_specif sspec;
          if (enc_code==XMLCODE_UCS2) sspec.wide_char=1;
          else sspec.charset = 
            enc_code==XMLCODE_ASCII ? 0   : enc_code==XMLCODE_UTF8  ? 7    : enc_code==XMLCODE_UTF7 ? 7 :
            enc_code==XMLCODE_852   ? 6   : enc_code==XMLCODE_1252  ? 3    : enc_code==XMLCODE_1250 ? 1 :
            enc_code==XMLCODE_88592 ? 129 : enc_code==XMLCODE_88591 ? 131  : 131;
          if (superconv(ATT_STRING, ATT_STRING, buf, (char *)ubuf, sspec, wx_string_spec, NULL) >= 0)
            dlg.mXMLText->SetValue(ubuf);  
          delete [] ubuf;
        }
        dlg.ShowModal();
      }
      cdcl.x()->dematerialize_params();
      corefree(buf);
      corefree(src);
      break;
    }
        
    case TD_CPM_HELP:
      wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/dadedit.html"));  
      break;
   // specific:
    case XML_EDIT:
    case XML_CUT:
    case XML_PASTE:
    case XML_DELETE:
    case XML_INS_ELEM:
    case XML_INS_ELEM_BEFORE:
    case XML_INS_ATTR:
    case XML_INS_TEXT:
    case XML_INS_ALL:
    { wxTreeItemId item = GetSelection();
      if (!item.IsOk()) break;
      dad_node * node = get_item_node(this, item);
      item_action(item, node, event.GetId());
      break;
    }
  }
}

t_tool_info xml_designer::tool_info[TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,        File_save_xpm,      wxTRANSLATE("Save design")),
  t_tool_info(TD_CPM_EXIT,        exit_xpm,           wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CHECK,       validateSQL_xpm,    wxTRANSLATE("Validate")),
  t_tool_info(TD_CPM_XML_PARAMS,  params_xpm,         wxTRANSLATE("Parameters")),             
  t_tool_info(TD_CPM_OPTIONS,     xml_properties_xpm, wxTRANSLATE("Global Properties")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_XML_IMPORT,  xml_import_xpm, wxTRANSLATE("Import data from an XML file")),
  t_tool_info(TD_CPM_XML_EXPORT,  xml_export_xpm, wxTRANSLATE("Export data to an XML file")),
  t_tool_info(TD_CPM_XML_TESTEXP, xml_test_xpm,   wxTRANSLATE("Test Export")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_HELP,        help_xpm,       wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

void xml_delete_bitmaps(void)
{ delete_bitmaps(xml_designer::tool_info); }

bool start_dad_designer(cdp_t cdp, tobjnum objnum, const char * folder_name, WizardPage3 * wpg, t_xsd_gen_params * gen_params)
{ 
  return open_standard_designer(new xml_designer(cdp, objnum, folder_name, cdp->sel_appl_name, wpg, gen_params));
}

#if 0
bool start_dad_designer(..., cdp_t cdp, tobjnum objnum, const char * folder_name, WizardPage3 * wpg, t_xsd_gen_params * gen_params)
// [wpg]!=NULL for a new design.
{ wxTopLevelWindow * dsgn_frame;  xml_designer * dsgn;
  if (global_style==ST_MDI)
  { DesignerFrameMDI * frm = new DesignerFrameMDI(wxGetApp().frame, -1, wxEmptyString);
    if (!frm) return false;
    dsgn_frame=frm;
    dsgn = new xml_designer(cdp, objnum, folder_name, cdp->sel_appl_name, dsgn_frame);
    if (!dsgn) goto err;
    frm->dsgn = dsgn;
  }
  else
  { DesignerFramePopup * frm = new DesignerFramePopup(wxGetApp().frame, -1, wxEmptyString, xmldsng_coords.get_pos(), xmldsng_coords.get_size());
    if (!frm) return false;
    dsgn_frame=frm;
    dsgn = new xml_designer(cdp, objnum, folder_name, cdp->sel_appl_name, dsgn_frame);
    if (!dsgn) goto err;
    frm->dsgn = dsgn;
  }
  if (dsgn->modifying() && !dsgn->lock_the_object())
    cd_Signalize(cdp);  // must Destroy dsgn because it has not been opened as child of designer_ch
  else if (dsgn->modifying() && !check_not_encrypted_object(cdp, CATEG_PGMSRC, objnum)) 
    ;  // must Destroy dsgn because it has not been opened as child of designer_ch  // error signalised inside
  else
    if (dsgn->open(dsgn_frame, wpg, gen_params)==0) 
      { dsgn_frame->SetIcon(wxIcon(_xml_xpm));  dsgn_frame->Show();  return true; } // designer opened OK
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



