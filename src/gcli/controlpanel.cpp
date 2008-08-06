// controlpanel.cpp
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
#include "cptree.h"
#include "cplist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

wxColour WindowColour;
wxColour WindowTextColour;
wxColour HighlightColour;
wxColour HighlightTextColour;

#ifdef CONTROL_PANEL_INPLECE_EDIT

BEGIN_EVENT_TABLE(CItemEdit, wxEvtHandler)
    EVT_KEY_DOWN(CItemEdit::OnKeyDown) 
    EVT_KILL_FOCUS(CItemEdit::OnKillFocus) 
END_EVENT_TABLE()

#ifdef WINS

void CItemEdit::OnKillFocus(wxFocusEvent &event)
{
#ifdef _DEBUG
    wxLogDebug(wxT("CItemEdit::OnKillFocus"));
#endif
    if (m_Parent->IsKindOf(CLASSINFO(wxTreeCtrl)))
        ((ControlPanelTree *)m_Parent)->EndEditLabel(((ControlPanelTree *)m_Parent)->m_EditItem, false);
    else
    {
        // wxListCtrl nema funkci na explicitni ukonceni editace, proto editaci ukoncim emulaci stisku Enteru
        if (!((ControlPanelList *)m_Parent)->m_EditCancelled)
        {
            wxTextCtrl *Edit = ((ControlPanelList *)m_Parent)->GetEditControl();
            wxKeyEvent KeyEvent(wxEVT_CHAR);
            KeyEvent.m_rawCode = WXK_RETURN;
            Edit->EmulateKeyPress(KeyEvent);
        }
    }
    event.Skip();
}

void CItemEdit::OnKeyDown(wxKeyEvent &event)
{
#ifdef _DEBUG
    wxLogDebug(wxT("CItemEdit::OnKeyDown"));
#endif
    // IF Esc v wxListCtrl, emulovat udalost wxEVT_COMMAND_LIST_END_LABEL_EDIT
    if (event.GetKeyCode() == WXK_ESCAPE && m_Parent->IsKindOf(CLASSINFO(wxListCtrl)))
    {
        wxCommandEvent nextEvent(myEVT_AFTEREDIT, /*LIST_ID*/m_Parent->GetId());
        ((ControlPanelList *)m_Parent)->m_EditCancelled = true;
        ((ControlPanelList *)m_Parent)->AddPendingEvent(nextEvent);
    }
    event.Skip();
}

#else  // LINUX

void CItemEdit::OnKeyDown(wxKeyEvent &event)
{
#ifdef _DEBUG
    wxLogDebug(wxT("CItemEdit::OnKeyDown"));
#endif
    if (event.m_keyCode == WXK_RETURN)
    {
        // Na Linuxu se pokud nedojde ke zmene nevygeneruje udalost EVT_LIST_END_LABEL_EDIT,
        // proto si ji musim vybavit sam
        wxString OldVal;
        if (m_Parent->IsKindOf(CLASSINFO(wxTreeCtrl)))
            OldVal = ((wxTreeCtrl *)m_Parent)->GetItemText(((ControlPanelTree *)m_Parent)->m_EditItem);
        else
            OldVal = ((ControlPanelList *)m_Parent)->GetItemText(((ControlPanelList *)m_Parent)->m_EditItem);
        wxString NewVal = ((wxTextCtrl *)event.GetEventObject())->GetValue();
        if (NewVal == OldVal)
        {
            wxCommandEvent nextEvent(myEVT_AFTEREDIT, m_Parent->GetId());
            m_Parent->AddPendingEvent(nextEvent);
        }
        m_InEdit = false;
#ifdef _DEBUG
        wxLogDebug(wxT("Old = %s   New = %s"), OldVal.c_str(), ((wxTextCtrl *)event.GetEventObject())->GetValue().c_str());
#endif
    }
    else if (event.GetKeyCode() == WXK_ESCAPE) // && m_Parent->IsKindOf(CLASSINFO(wxListCtrl)))
    {
        //((ControlPanelList *)m_Parent)->m_EditCancelled = true;
        m_InEdit = false;
    }
    event.Skip();
}

void CItemEdit::OnKillFocus(wxFocusEvent &event)
{
#ifdef _DEBUG
    wxLogDebug(wxT("CItemEdit::OnKillFocus"));
#endif
    // Na Linuxu se pokud nedojde ke zmene nevygeneruje udalost EVT_LIST_END_LABEL_EDIT,
    // proto si ji musim vybavit sam, navic OnKillFocus prijde dvakrat, tak je potreba ji podruhe zahodit
    if (m_InEdit)
    {
        wxString OldVal;
        if (m_Parent->IsKindOf(CLASSINFO(wxTreeCtrl)))
            OldVal = ((wxTreeCtrl *)m_Parent)->GetItemText(((ControlPanelTree *)m_Parent)->m_EditItem);
        else
            OldVal = ((ControlPanelList *)m_Parent)->GetItemText(((ControlPanelList *)m_Parent)->m_EditItem);
        wxString NewVal = ((wxTextCtrl *)event.GetEventObject())->GetValue();
#ifdef _DEBUG
        wxLogDebug(wxT("Old = %s   New = %s"), OldVal.c_str(), NewVal.c_str());
#endif
        if (NewVal == OldVal)
        {
            wxCommandEvent nextEvent(myEVT_AFTEREDIT, m_Parent->GetId());
            m_Parent->AddPendingEvent(nextEvent);
        }
        m_InEdit = false;
    }
    event.Skip();
}

#endif // WINS

#endif // CONTROL_PANEL_INPLECE_EDIT

list_selection_iterator::list_selection_iterator(ControlPanelList * cplIn) : cpl(cplIn) 
{ count=cplIn->GetSelectedItemCount();  pos=-1; info = *cpl->m_BranchInfo; whole_folder = false;};

bool list_selection_iterator::next(void)
{ pos=cpl->GetNextItem(pos, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (pos==-1) return false;
  cpl->add_list_item_info(pos, info);
  return true;
}

////////////////////////////////////// Drag&Drop in control panel ///////////////////////////////////////////

// wx does not enable to DropTarget discover what is in the DataObject and corectly answer if draged 
// data are acceptable or not, so I store DataObject to static variable, where DropTarget could it find
C602SQLObjects *CCPDragSource::m_DraggedObjects;
//
// Constructor
//
#ifdef WINS    
CCPDragSource::CCPDragSource(wxWindow *Owner, C602SQLDataObject *dobj)
#else    

char *no_xpm[] = 
{
    "1 1 1 1",
    "0 c none",
    "0"
};

CCPDragSource::CCPDragSource(wxWindow *Owner, C602SQLDataObject *dobj, int Ico) : wxDropSource(Owner, Ico == -1 ? wxIcon(no_xpm) : ((MyFrame *)wxGetApp().frame)->main_image_list->GetIcon(Ico))
#endif    
{
    m_Owner   = Owner;
    SetData(*dobj);

    m_DraggedObjects = dobj->m_ObjData;
}
//
// On Windows checks if mouse cursor is over control panel tree or list or query designer tables panel,
// if true shows and moves gragged objects image, if false hides image. On Linux image dragging 
// conflicts with wx Drag&Drop implementation, so it is not used.
//
bool CCPDragSource::GiveFeedback(wxDragResult effect)
{
#ifdef WINS
    bool Show = false;
    // Get window from mouse position
    wxPoint   Pos = wxGetMousePosition();
    wxWindow *Wnd = wxFindWindowAtPointer(Pos);
    if (Wnd)
    {
        wxString wName = Wnd->GetName();
        // Check window type
        if (wxStrcmp(wName, CPTreeName) == 0 || wxStrcmp(wName, CPListName) == 0 || wxStrcmp(wName, QDTablePanelName) == 0)
            Show = true;
#ifdef LINUX
        Wnd = Wnd->GetParent();
        if (Wnd)
        {
            wName = Wnd->GetName(); 
            if (wxStrcmp(wName, CPListName) == 0)
                Show = true;
        }
#endif            
    }

    // IF appropriate window, show and move dragged objects image
    if (Show)
    {
        CWBDragImage::Image->Show();
        CWBDragImage::Image->Move(m_Owner->ScreenToClient(Pos));
    }
    else 
    {
        CWBDragImage::Image->Hide();
    }

#endif
    return(false);
}
//
// Intrinsic draged data drop
//
wxDragResult CCPDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    // Read dragged object list from DataObject
    GetData();
    CPasteObjCtx Ctx(m_cdp, m_Schema, m_Folder);
    C602SQLObjects *objs = ((C602SQLDataObject *)GetDataObject())->GetObjects();
    // IF move is requested, set move flag
    if (GetResult() == wxDragMove)
        objs->m_Flags |= CBO_CUT;
    // Paste objects
    Ctx.PasteObjects(objs);
    return(def);
}

#ifdef WINS
CWBDragImage *CWBDragImage::Image;
#endif

//
// Vraci informaci o tom jestli slozka obsahuje nejaky objekt dane kategorie
// Items   = true   - Prohledavat zadanou slozku
// Folders = true   - Prohledavat jeji podslozky
// Hodnota = FST_EMPTY - ve slozce neni nic, FST_HASITEMS - ve slozce jsou objekty, FST_HASFOLDERS = ve slozce jsou podslozky
//
int GetFolderState(cdp_t cdp, const char *SchemaName, const char *FolderName, tcateg category, bool Items, bool Folders)
{ 
    int Result = FST_EMPTY;
    if (cdp)
    { // Ineffective for non-cached categories.
      CObjectList ol(cdp, SchemaName);
      bool ItemFound;
      if (Items)
      {
          for (ItemFound = ol.FindFirst(category, FolderName); ItemFound; ItemFound = ol.FindNext())
          {
              Result = FST_HASITEMS;
              break;
          }
      }
      if (Folders)
      {
          for (ItemFound = ol.FindFirst(CATEG_FOLDER, FolderName); ItemFound; ItemFound = ol.FindNext())
          {
              if (GetFolderState(cdp, SchemaName, ol.GetName(), category, true, true) != FST_EMPTY)
              {
                  Result |= FST_HASFOLDERS;
                  break;
              }
          }
      }
    }
    else
      Result = category==CATEG_TABLE || category==CATEG_CURSOR ? FST_HASITEMS : FST_EMPTY;
    return Result;
}
//
// Vraci informaci o tom jestli slozka obsahuje nejaky objekt jakekoli kategorie
//
int GetFolderState(cdp_t cdp, const char *SchemaName, const char *FolderName, bool Items, bool Folders)
{
    int Result = FST_EMPTY;
    int Mask   = 0;
    if (Items)
        Mask |= FST_HASITEMS;
    if (Folders)
        Mask |= FST_HASFOLDERS;

    CCategList cl;
    do
    {
        Result |= GetFolderState(cdp, SchemaName, FolderName, cl.GetCateg(), Items, Folders);
        if (Result == Mask)
            return(Result);
    }
    while (cl.FindNext());
    return(Result);
}

item_info &wxCPSplitterWindow ::GetRootInfo()
{return tree_view->root_info;}
void wxCPSplitterWindow::RefreshSelItem(bool ListNo)
{RefreshItem(tree_view->m_SelItem, ListNo);}

/////////////////////////////////////////// CP toolbar //////////////////////////////////////////////////
void update_CP_toolbar(MyFrame * frame, item_info & info)
// Update the activity of the CP toolbar controls according to the selected item [info].
{ wxToolBarBase * tb = frame->MainToolBar;
  if (!tb) return;  // 
  wxMenuBar *MenuBar = frame->GetMenuBar();
  int index  = MenuBar->FindMenu(ACTION_MENU_STRING);  
  if (index!=wxNOT_FOUND)
  { wxMenu * object_menu = MenuBar->GetMenu(index);
    tb->EnableTool(CPM_NEW   , object_menu->FindItem(CPM_NEW   )!=NULL && object_menu->FindItem(CPM_NEW   )->IsEnabled());
    tb->EnableTool(CPM_MODIFY, object_menu->FindItem(CPM_MODIFY)!=NULL && object_menu->FindItem(CPM_MODIFY)->IsEnabled());
    tb->EnableTool(CPM_DELETE, object_menu->FindItem(CPM_DELETE)!=NULL && object_menu->FindItem(CPM_DELETE)->IsEnabled());
    tb->EnableTool(CPM_CUT,    object_menu->FindItem(CPM_CUT)   !=NULL && object_menu->FindItem(CPM_CUT   )->IsEnabled());
    tb->EnableTool(CPM_COPY,   object_menu->FindItem(CPM_COPY)  !=NULL && object_menu->FindItem(CPM_COPY  )->IsEnabled());
    tb->EnableTool(CPM_PASTE,  object_menu->FindItem(CPM_PASTE) !=NULL && object_menu->FindItem(CPM_PASTE )->IsEnabled());
  }
#if 0

  frame->MainToolBar->EnableTool(CPM_MODIFY, m_SelInfo.syscat == SYSCAT_APPLS && !m_SelInfo.cdp);
  frame->MainToolBar->EnableTool(CPM_DELETE, (m_SelInfo.IsServer() && !m_SelInfo.cdp) || m_SelInfo.IsSchema() || m_SelInfo.IsFolder() || (m_SelInfo.IsCategory() && !m_SelInfo.IsSysCategory()));
  frame->MainToolBar->EnableTool(CPM_CUT,    (m_SelInfo.category!=CATEG_INFO && (m_SelInfo.IsFolder() || m_SelInfo.IsCategory())));
  frame->MainToolBar->EnableTool(CPM_COPY,   (m_SelInfo.category!=CATEG_INFO && (m_SelInfo.IsFolder() || m_SelInfo.IsCategory())));

  frame->MainToolBar->EnableTool(CPM_MODIFY, (info.IsServer() && !info.cdp) || (info.IsObject() && m_BranchInfo->cti != IND_SYSTABLE) || (info.topcat == TOPCAT_CLIENT && info.cti == IND_MAILPROF));
  frame->MainToolBar->EnableTool(CPM_DELETE, (info.IsServer() && !info.cdp) || info.IsSchema() || info.IsFolder() || (info.IsCategory() && !info.IsSysCategory()) || (info.IsObject() && m_BranchInfo->cti != IND_SYSTABLE));
  frame->MainToolBar->EnableTool(CPM_CUT,    (info.IsObject() && info.CanCopy()) || (info.category!=CATEG_INFO && (info.IsFolder() || info.IsCategory())));
  frame->MainToolBar->EnableTool(CPM_COPY,   (info.IsObject() && info.CanCopy()) || (info.category!=CATEG_INFO && (info.IsFolder() || info.IsCategory())));
#endif
}
