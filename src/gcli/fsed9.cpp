// fsed9.cpp
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
#include "impexp.h"
#include "wbprezex.h"
#include "fsed9.h"
#include "comp.h"
#include "debugger.h"
#include <wx/caret.h>
#include <wx/clipbrd.h>
#include <wx/dnd.h>
#include "wx/notebook.h"
#include "wx/tooltip.h"
#include "compil.h"
//#define NEW_SCROLL
#include "xrc/EvalDlg.h"
#include "xrc/SearchObjectsDlg.h"
#include "xrc/WatchListDlg.h"
#include "xrc/QueryOptimizationDlg.h"
#include "queryopt.h"
#include "objlist9.h"
#include "controlpanel.h"
#include "xmldsgn.h"

#ifdef ADVTOOLTIP
#include "advtooltip.cpp"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif
void CALLBACK log_client_error(const char * text);
////////////////////////////////////////////////
#define FR_INSELECTION  8
string_history find_history(200);
string_history replace_history(200);

#include "xrc/ReplaceDlg.cpp"
#include "xrc/FindDlg.cpp"
wxString last_search_string;
////////////////////////////////////////////////

class editor_type;
class editor_container : public wxNotebook, public any_designer  // inheritance must be in this order!
{
  bool IsChanged(void) const;
  wxString SaveQuestion(void) const;
  bool save_design(bool save_as);
  void _set_focus(void)
    { if (GetHandle()) SetFocus(); }
 public:
  bool adding_page;
  editor_type * ed_owning_tools;
  void Activated(void);
  void Deactivated(void);
  void OnCommand(wxCommandEvent & event);
  t_tool_info * get_tool_info(void) const;
  editor_container(void);
  bool open(wxWindow * parent, t_global_style my_style);
  void set_designer_caption(void);
  void OnPageActivated(wxNotebookEvent & event);
  void OnDisconnecting(ServerDisconnectingEvent & event);
  void OnRecordUpdate(RecordUpdateEvent & event);
  void OnSetFocus(wxFocusEvent & event);
  void OnRightDown(wxMouseEvent & event);
  ~editor_container(void);
  void add_tools(const wxChar * menu_name = DESIGNER_MENU_STRING) {}
  void remove_tools(void) {}
  DECLARE_EVENT_TABLE()
};

editor_container * the_editor_container = NULL;
wxTopLevelWindow * the_editor_container_frame = NULL;

#define MAX_INDENT 60
static const wxChar indent_spaces[MAX_INDENT+2+1] = 
  wxT("\r\n                                                            ");

editor_container::~editor_container(void)  // clears the pinters to the only instance
{ the_editor_container=NULL;  the_editor_container_frame=NULL; }

bool editor_container::open(wxWindow * parent, t_global_style my_style)
{ 
  if (!Create(parent, COMP_FRAME_ID, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN | wxWANTS_CHARS, wxT("editor_container")))
    return false;
  competing_frame::focus_receiver = this;
  the_editor_container_frame=(wxTopLevelWindow*)parent;
  SetImageList(wxGetApp().frame->main_image_list);
  return true;
}

void editor_container::set_designer_caption(void)
{ 
  set_caption(_("Text Editor"));
}

void editor_container::OnRightDown(wxMouseEvent & event)
{ wxPoint pt;  long flags;
  pt = ScreenToClient(wxGetMousePosition());
  int sel = HitTest(pt, &flags);
  if (sel!=wxNOT_FOUND && (flags & (wxNB_HITTEST_ONICON | wxNB_HITTEST_ONLABEL | wxNB_HITTEST_ONITEM)) != 0)
  { SetSelection(sel);
    editor_type * ed = (editor_type*)GetPage(sel);
    ed->context_menu(true);
  }
}
////////////////////////////////// list of bookmarks ///////////////////////////////////////
void t_linenum_list::add(t_posit posit)  
// Does not check for duplicity. Memory allocation errors masked.
{ for (int i=0;  i<list.count();  i++)
    if (*list.acc0(i) == NO_LINENUM)
      { *list.acc0(i)=posit;  return; }
  t_posit * p = list.next();
  if (p!=NULL) *p=posit;
}

void t_linenum_list::del(t_posit posit)  
{ for (int i=0;  i<list.count();  i++)
    if (*list.acc0(i) == posit)
      { *list.acc0(i)=NO_LINENUM;  return; }
}

bool t_linenum_list::contains(t_posit posit)  
{ for (int i=0;  i<list.count();  i++)
    if (*list.acc0(i) == posit) return true; 
  return false;
}

BOOL t_linenum_list::del_all(void)  
{ BOOL any_found = FALSE;
  for (int i=0;  i<list.count();  i++)
    if (*list.acc0(i) != NO_LINENUM) any_found = TRUE;
  list.t_dynar::~t_dynar(); 
  return any_found;
}

t_posit t_linenum_list::next(t_posit posit)  
{ uns32 dist=(uns32)-1;  t_posit pos=NO_LINENUM;
  for (int i=0;  i<list.count();  i++)
  { t_posit aline = *list.acc0(i);
    if (aline != NO_LINENUM && aline!=posit)
      if ((uns32)(aline-posit) <= dist)
        { pos=aline;  dist=aline-posit; }
  } 
  return pos;
}

t_posit t_linenum_list::prev(t_posit posit)  
{ uns32 dist=(uns32)-1;  t_posit pos=NO_LINENUM;
  for (int i=0;  i<list.count();  i++)
  { t_posit aline = *list.acc0(i);
    if (aline != NO_LINENUM && aline!=posit)
      if ((uns32)(posit-aline) <= dist)
        { pos=aline;  dist=posit-aline; }
  } 
  return pos;
}

void t_linenum_list::adjust(wxChar * pos, int sz)
{ for (int i=0;  i<list.count();  i++)
  { t_posit * p = list.acc0(i);
    if (*p!=NO_LINENUM && pos<*p)
      if (sz>=0 || *p+sz>pos) *p +=sz; // >: must delete posit when joining its line with the previous one
      else *p=NO_LINENUM;//*p=pos;  // deletes it
  }
}

BOOL t_linenum_list::empty(void)
{ for (int i=0;  i<list.count();  i++)
    if (*list.acc0(i)!=NO_LINENUM) return FALSE;
  return TRUE;
}
/////////////////////////////////// drop target ///////////////////////////////////
class EditorTextDropTarget : public wxTextDropTarget
{ editor_type * ed;
 public: 
  EditorTextDropTarget(editor_type * edIn)
    { ed=edIn; }
  bool OnDropText(wxCoord x, wxCoord y, const wxString & data);
};

//////////////////////////////////// editor type ////////////////////////////////////////////////

static unsigned txlnlen(const wxChar * ln)
/* return the length of the line excl. delimiters */
{ unsigned l=0;
#if 0  // old CR-centric version
  while (*ln && (*ln!=0xd)) { ln++; l++; }
#else
  while (*ln && *ln!=0xa && *ln!=0xd) { ln++; l++; }
#endif
  return l;
}

static unsigned lnlen(const wxChar * ln)
/* return the length of the line incl. delimiters */
{ unsigned l=0;
#if 0  // old CR-centric version
  while (*ln && (*ln!=0xd)) { ln++; l++; }
  if (!*ln) return l;
  if (ln[1]==0xa) return l+2;
#else
  while (*ln && *ln!=0xa) { ln++; l++; }
  if (!*ln) return l;
#endif
  return l+1;
}

bool is_next_line(const wxChar * line)
{ return line[lnlen(line)]!=0; }


BEGIN_EVENT_TABLE( editor_type, wxScrolledWindow)
  EVT_SCROLLWIN(editor_type::OnScroll)
  EVT_SET_FOCUS(editor_type::OnSetFocus)
  EVT_SIZE(editor_type::OnSize)
  EVT_FIND(-1, editor_type::OnFindReplaceAction)
  EVT_FIND_NEXT(-1, editor_type::OnFindReplaceAction)
  EVT_FIND_REPLACE(-1, editor_type::OnFindReplaceAction)
  EVT_FIND_REPLACE_ALL(-1, editor_type::OnFindReplaceAction)
  EVT_FIND_CLOSE(-1, editor_type::OnFindReplaceAction)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( gutter_editor_panel, wxWindow)
  EVT_ERASE_BACKGROUND(gutter_editor_panel::OnErase)
  EVT_PAINT(gutter_editor_panel::OnPaint) 
  EVT_LEFT_DOWN(gutter_editor_panel::OnLeftButtonDown)
  EVT_LEFT_UP(gutter_editor_panel::OnLeftButtonUp)
  EVT_RIGHT_DOWN(gutter_editor_panel::OnRightButtonDown)
  EVT_MENU(-1, gutter_editor_panel::OnCommand)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( scrolled_editor_panel, wxWindow)
  EVT_SET_FOCUS(scrolled_editor_panel::OnSetFocus) 
  EVT_KILL_FOCUS(scrolled_editor_panel::OnKillFocus) 
  EVT_SIZE(scrolled_editor_panel::OnSize)
  EVT_ERASE_BACKGROUND(scrolled_editor_panel::OnErase)
  EVT_PAINT(scrolled_editor_panel::OnPaint) 
  EVT_KEY_DOWN(scrolled_editor_panel::OnKeyDown)
  EVT_CHAR(scrolled_editor_panel::OnChar)
  EVT_LEFT_DOWN(scrolled_editor_panel::OnLeftButtonDown)
  EVT_LEFT_UP(scrolled_editor_panel::OnLeftButtonUp)
  EVT_LEFT_DCLICK(scrolled_editor_panel::OnLeftButtonDclick)
  EVT_RIGHT_DOWN(scrolled_editor_panel::OnRightButtonDown)
  EVT_RIGHT_UP(scrolled_editor_panel::OnRightButtonUp)
  EVT_MOTION(scrolled_editor_panel::OnMouseMotion)
  EVT_MOUSEWHEEL(scrolled_editor_panel::OnMouseWheel)
#ifndef ADVTOOLTIP
  EVT_TIMER(TOOLTIP_TIMER_ID, scrolled_editor_panel::OnTooltipTimer)
#endif
END_EVENT_TABLE()

bool editor_type::OnCopy(void)
{ if (block_end > editbuf+txtsize)
    block_end = editbuf+txtsize;
  if (block_start > editbuf+txtsize)
    block_start = editbuf+txtsize;
  if (block_start >= block_end) return false; /* not necessary - fuse */
  if (wxTheClipboard->Open())
  { wxTheClipboard->SetData(new wxTextDataObject(wxString(block_start, block_end-block_start)));  // clipboard takes the ownership!
    wxTheClipboard->Close();
    return true;
  }
  else 
  { wxBell();
    return false;
  }
}

void editor_type::OnCut(void)
{ if (noedit) { wxBell();  return; }
  if (!OnCopy()) return;  // clipboard not opened -> do not remove the text
  delete_block();
  count_lines();
  move_caret();
}

void editor_type::OnPaste(void)
{ if (noedit) { wxBell();  return; }
  if (wxTheClipboard->Open())
  { if (wxTheClipboard->IsSupported( wxDF_TEXT ))
    { wxTextDataObject data;
      wxTheClipboard->GetData(data);
     // paste the text:
      int blocksize=(int)data.GetText().Length();  // GetTextLength may return the length including the terminator or not.
      if (txtsize+blocksize < edbsize)
      { seq_delete_block();
        insert_chars(data.GetText().c_str(), blocksize);
        block_start=cur_line_a+cur_char_n;
        block_end=block_start+blocksize;
        block_on=/*TRUE*/false;  /* notepad hides block */
        count_lines();
       // go to the end of the block:
        do
        { wxChar * n_line=next_line(cur_line_a);
          if (n_line>block_end || n_line == cur_line_a) break;
          cur_line_n++;
          cur_line_a = n_line;
        } while (true);
        int off = block_end-cur_line_a;
        int len=txlnlen(cur_line_a);
        cur_char_n = off < len ? off : len;
        scroll_to_cursor();
        scrolled_panel->Refresh();
      }
      else wxBell(); // our of editor space
    } 
    else wxBell(); 
    wxTheClipboard->Close();
  }
  else wxBell();
}

void editor_type::OnSelectAll(void)
{ block_start=editbuf;
  block_end=editbuf+txtsize;
  block_on=true;
  scrolled_panel->Refresh();
}

void editor_type::OnIndentBlock(void)
{ if (noedit) return;
  if (!block_on || block_start >= block_end) return;
  wxChar * pos=block_start;
  bool first=true;
  while (pos < block_end)
  {/* insert space: */
    if (txtsize>=edbsize) break;
    memmov(pos+1, pos, sizeof(wxChar) * ((++txtsize)-(pos-editbuf)));
    *pos=' ';
    adjust(pos,1);
    if (first) first=false;  else add_seq_flag();
    register_action(ACT_TP_INS, pos-editbuf, 0, (wxChar*)1);
    changed();
    if (pos<cur_line_a) cur_line_a++;
   /* next line: */
    wxChar * pos1=next_line(pos);
    if (pos1==pos) break;
    pos=pos1;
  }
  scrolled_panel->Refresh();
}

void editor_type::OnUnindentBlock(void)
{ if (noedit) return;
  if (!block_on || block_start >= block_end) return;
  wxChar * pos=block_start;
  bool first=true;
  while (pos < block_end)
  {/* delete space: */
    if (pos-editbuf >= txtsize) break;
    if (*pos==' ')
    { if (first) first=false; else add_seq_flag();
      register_action(ACT_TP_DEL1, pos-editbuf, *pos, NULL);
      memmov(pos, pos+1, sizeof(wxChar) * ((txtsize--)-(pos-editbuf)));
      adjust(pos,-1);
      if (pos<cur_line_a) cur_line_a--;
      changed();
    }
   /* next line: */
    wxChar * pos1=next_line(pos);
    if (pos1==pos) break;
    pos=pos1;
  }
  scrolled_panel->Refresh();
}

void editor_type::SetText(const wxChar *Text)
{
    int Len = (int)_tcslen(Text);
    if (!editbuf || (Len + 10000 >= edbsize))
    {
        edbsize = Len + 3000;
        editbuf = (wxChar *)corerealloc(editbuf, sizeof(wxChar)*(edbsize + 1));
        if (editbuf)
            *editbuf = 0;
        block_start = block_end = cur_line_a = editbuf;
    }
    _tcscpy(editbuf, Text);
    txtsize = Len;
    count_lines();
}

void scrolled_editor_panel::OnErase(wxEraseEvent & event)
// No action: implemented to prevent flickering
{ event.GetDC(); }

void gutter_editor_panel::OnErase(wxEraseEvent & event)
// No action: implemented to prevent flickering
{ event.GetDC(); }

void scrolled_editor_panel::OnSize(wxSizeEvent & event)
{ my_editor->win_xlen = (unsigned)(GetClientSize().GetWidth()  / my_editor->nCharWidth);  // event.GetSize() returns too much, perhaps the outer window size
  my_editor->win_ylen =            GetClientSize().GetHeight() / my_editor->nLineHeight;
  SetVirtualSize((int)(my_editor->nCharWidth*my_editor->virtual_width), 
                       my_editor->nLineHeight*(my_editor->total_line_n+1));  // in pixels
  event.Skip();
}

void editor_type::OnSize(wxSizeEvent & event)
{
//  Layout();  // not called automatically vecause editor_type is not a panel, frame or dialog
 // when using sizers, they use the virtual size and apply it to the child windows. Then scrollbars diappear because the 
 // The real size is the same as the virtual size.
  wxSize sz = GetClientSize();
  if (gutter_panel && scrolled_panel)
  { gutter_panel->SetSize(-1, -1, -1, sz.y, wxSIZE_USE_EXISTING);
    scrolled_panel->SetSize(-1, -1, sz.x-gutter_size(), sz.y, wxSIZE_USE_EXISTING);
  }
  event.Skip();
}

void editor_type::OnCloseRequest(bool close_all_pages)  
{ if (in_edcont)
  { if (close_all_pages)
    { ServerDisconnectingEvent event(NULL);
      the_editor_container->OnDisconnecting(event);
      if (!event.cancelled)
        //the_editor_container_frame->Destroy(); -- does not save the coords
        destroy_designer(the_editor_container_frame, &editor_cont_coords);
    }
    else
    { wxCommandEvent cmd;  cmd.SetId(TD_CPM_EXIT);
      the_editor_container->OnCommand(cmd);
    }
  }
  else
    if (exit_request(this, true))
      DoClose();
}

void editor_type::DoClose(void)
// Do not ask, do not save, just close the editor
{ 
  if (in_edcont)
  {// find the page:
    int pg=0;
    while (pg<the_editor_container->GetPageCount())
      if ((editor_type*)the_editor_container->GetPage(pg) == this)  // page found, delete it
      { the_editor_container->DeletePage(pg);  // removes the page and destroys it
        pg = the_editor_container->GetSelection();
#ifdef LINUX
       // on GTK the notification about page chnaging is switched OFF, must do it manually
        if (pg!=-1)
		    { editor_type * ed = (editor_type*)the_editor_container->GetPage(pg);
          ed->Activated();
        }  
#endif  // LINUX		
        break;
      }
      else pg++;
  }
  else
    destroy_designer(GetParent(), &editor_sep_coords);
}

void editor_type::OnScroll(wxScrollWinEvent & event)
// Necesary on GTK, without this the caret position is not updated when scrolling.
{
 // must update the caret position, but not via move_caret() because GetViewStart() does not return
 // the new scroll position in this message.
  wxCaret * caret = scrolled_panel->GetCaret();
  if (caret)
  { int start_x_su, start_y_su;
    GetViewStart(&start_x_su, &start_y_su);  // documentation error: must use physical coordinates, not logical!
    int pos=event.GetPosition();
    if (pos>=0)
      if (event.GetOrientation()==wxHORIZONTAL)
        start_x_su=pos;
      else  
        start_y_su=pos;
    caret->Move((int)((cur_char_n-start_x_su)*nCharWidth), (cur_line_n-start_y_su)*nLineHeight);
  }
  event.Skip();
}

void editor_type::OnSetFocus(wxFocusEvent & event)
// May not be necessary: panel gives own focus to a child by default
{ 
  if (scrolled_panel) 
  { scrolled_panel->SetFocus(); 
    //wxLogDebug(wxT("Focusig scroll panel"));
  }
  event.Skip();
}

void scrolled_editor_panel::OnSetFocus(wxFocusEvent & event)
{
  /*GetCaret()->Show(); -- this is a default caret logic implemented in wx */
  //wxLogDebug(wxT("Focused scroll panel"));
  event.Skip();
}

void scrolled_editor_panel::OnKillFocus(wxFocusEvent & event)
{ /*GetCaret()->Hide(); -- this is a default caret logic implemented in wx */ 
  event.Skip();
}

void scrolled_editor_panel::OnChar(wxKeyEvent & event)
{ my_editor->OnChar(event); }

void scrolled_editor_panel::OnKeyDown(wxKeyEvent & event)
{ my_editor->OnKeyDown(event); }

void editor_type::OnKeyDown(wxKeyEvent & event)
{ 
  int keycode = event.GetKeyCode();
  if (event.AltDown())
  { switch (keycode)
    { case WXK_F3:
        OnFindReplace(false);  // just opens the Find dialog
      default:
        event.Skip();  break;
    }
    return;
  }
  
  //wxString msg;
  //msg.Printf("OnKeyDown %d", event.GetKeyCode());
  //wxLogDebug(msg);
  
  bool moved=false;
  unsigned linelen=lnlen(cur_line_a), txlinelen=txlnlen(cur_line_a);
  wxChar * prepos=cur_line_a+((cur_char_n > txlinelen) ? linelen : cur_char_n);

  if (keycode>='A' && keycode<='Z' && !event.ControlDown())
    { event.Skip();  return; }

  switch (keycode)
  { case WXK_LEFT:
      if (!cur_char_n) { wxBell();  break; }
      if (event.ControlDown())
      { if (cur_char_n>txlnlen(cur_line_a)) cur_char_n=txlnlen(cur_line_a);
        else
        { while (cur_char_n && !is_AlphanumW(cur_line_a[cur_char_n-1], cdp->sys_charset)) cur_char_n--;
          while (cur_char_n &&  is_AlphanumW(cur_line_a[cur_char_n-1], cdp->sys_charset)) cur_char_n--;
        }
      }
      else cur_char_n--;  
      moved=true;  break;
    case WXK_RIGHT:
      if (event.ControlDown())
      { while (cur_char_n<txlinelen &&  is_AlphanumW(cur_line_a[cur_char_n], cdp->sys_charset)) cur_char_n++;
        while (cur_char_n<txlinelen && !is_AlphanumW(cur_line_a[cur_char_n], cdp->sys_charset)) cur_char_n++;
      }
      else cur_char_n++;
      moved=true;  break;
    case WXK_UP:
      if (!cur_line_n) { wxBell();  break; }
      if (event.ControlDown())  // scroll up
      { int start_x_su, start_y_su;
        GetViewStart(&start_x_su, &start_y_su); 
        if (!start_y_su) { wxBell();  break; }  // cannot scroll up, is on the top
        Scroll(-1, --start_y_su);
        if (cur_line_n>=start_y_su+win_ylen)  // must move the cursor up, too
        { cur_line_a=prev_line(cur_line_a);
          cur_line_n--;
        }
      }
      else
      { cur_line_a=prev_line(cur_line_a);
        cur_line_n--;
      }
      moved=true;  break;
    case WXK_DOWN:
      if (!is_next_line(cur_line_a)) { wxBell();  break; }
      if (event.ControlDown())  // scroll down
      { int start_x_su, start_y_su;
        GetViewStart(&start_x_su, &start_y_su); 
        Scroll(-1, ++start_y_su);
        if (cur_line_n<start_y_su)  // must move the cursor up, too
        { cur_line_a=next_line(cur_line_a);
          cur_line_n++;
        }
      }
      else
      { cur_line_a=next_line(cur_line_a);
        cur_line_n++;
      }
      moved=true;  break;
    case WXK_HOME:
      if (event.ControlDown())  // to start
      { cur_line_n=0;  cur_line_a=editbuf;
        Scroll(-1, 0);
      }
      cur_char_n=0;  
      moved=true;  break;
    case WXK_END:
      if (event.ControlDown())  // to the end
      { while (is_next_line(cur_line_a))
        { cur_line_a=next_line(cur_line_a);
          cur_line_n++;
        }
      }
      cur_char_n=txlnlen(cur_line_a);  
      moved=true;  break;
    case WXK_PRIOR:
    { int move_lines=win_ylen-1;
     // move cursor 1 page up (even if cannot scroll so much):
      for (int i=0;  i<move_lines && cur_line_n;  i++)
      { cur_line_a=prev_line(cur_line_a);
        cur_line_n--;
      }
     // scroll explicitly 1 page - independently of the cursor position:
      int start_x_su, start_y_su;
      GetViewStart(&start_x_su, &start_y_su);
      if (start_y_su<move_lines) move_lines=start_y_su;
      Scroll(-1, start_y_su-move_lines);
      moved=true;  break;  // must make cursor visible: when it was not visible, it may still be invisible
    }
    case WXK_NEXT:
    { int move_lines=win_ylen-1;
     // move cursor 1 page down (even if cannot scroll so much):
      for (int i=0;  i<move_lines;  i++)
      { if (!is_next_line(cur_line_a)) break;
        cur_line_a=next_line(cur_line_a);
        cur_line_n++;
      }
     // scroll explicitly 1 page - independently of the cursor position:
      int start_x_su, start_y_su;
      GetViewStart(&start_x_su, &start_y_su);
      Scroll(-1, start_y_su+move_lines);
      moved=true;  break;  // must make cursor visible: when it was not visible, it may still be invisible
    }
    case WXK_INSERT:  case WXK_NUMPAD_INSERT:
      //if (GKState(VK_SHIFT)) return Fsed_COMMAND(MI_FSED_PASTE, 0L);
      //else if (GKState(VK_CONTROL)) return Fsed_COMMAND(MI_FSED_COPY,  0L);
      //else 
      if (event.ControlDown())
      { if (block_on) OnCopy();
        else wxBell();
      }
      else if (event.ShiftDown())
        OnPaste();
      else
      { is_overwr=!is_overwr;
        scrolled_panel->new_caret();
      }
      break;
    case WXK_DELETE:  case WXK_NUMPAD_DELETE:
      if (noedit) break;
      if (event.ControlDown() && cur_char_n < txlnlen(cur_line_a))
      // delete word right, unless on the end of the line
      { if (block_on) delete_block(); 
        int len=txlnlen(cur_line_a);
        int linepos=cur_char_n;
        while (linepos < len && is_AlphanumW(cur_line_a[linepos], cdp->sys_charset))
          linepos++;
        while (linepos < len && cur_line_a[linepos]==' ') 
          linepos++;
        int cnt=linepos-cur_char_n;  // # of char to be deleted
        if (!cnt) cnt=1;  // delete single char if not on a word
        if (cnt)
        { del_chars(cur_line_a+cur_char_n, cnt, FALSE);
          //refresh_current_line(); -- syntax colouring problems
          refresh_from_line(cur_line_n);
        }
      }  
      else if (event.ShiftDown())  // standard shortcut
      { if (block_on) OnCut();
        else wxBell();
      }
      else if (block_on) delete_block();
      else if (cur_char_n < txlnlen(cur_line_a))
      { del_cur_char(FALSE);
        //refresh_current_line();-- syntax colouring problems
        refresh_from_line(cur_line_n);
      }
      else /* join with the next line */
      { if (is_next_line(cur_line_a))
        { delete_line_delim(false);
          refresh_from_line(cur_line_n);
        }
        else wxBell();  // positioned on the end of the last line or after it
      }
      scroll_to_cursor();
      break;
    case WXK_BACK:
      if (noedit) break;
      if (event.ControlDown() || event.ShiftDown()) break;  // no action defined
      if (block_on) delete_block();  
      else if (cur_char_n)
      { cur_char_n--;
        if (cur_char_n < txlnlen(cur_line_a))
        { del_cur_char(TRUE);
          //refresh_current_line();  -- syntax colouring problems
          refresh_from_line(cur_line_n);
        }
      }
      else  /* in the 1th column */
      if (cur_line_a==editbuf) wxBell();
      else
      { cur_line_a=prev_line(cur_line_a);
        cur_line_n--;
        delete_line_delim(true);
        refresh_from_line(cur_line_n);
      }
      scroll_to_cursor();
      break;
    case WXK_RETURN:  case WXK_NUMPAD_ENTER:
    {// count the current indent:
      int sz=0;
      while ((sz<cur_char_n) && (cur_line_a[sz]==' ')) sz++;
      if (is_overwr) // overwrite mode: just go to the next line
      { if (is_next_line(cur_line_a))
        { cur_line_a=next_line(cur_line_a);  cur_line_n++;  cur_char_n=sz;  
          scroll_to_cursor();
        }
        else wxBell();  // on the last line, no operetion
      }
      else
      { if (noedit) break;
        seq_delete_block();
       // if cursor is past the end of line, move it to the end:
        int llen=txlnlen(cur_line_a);
        if (cur_char_n > llen) cur_char_n=llen;
       // insert CR LF and indent spaces:
        if (sz>MAX_INDENT) sz=MAX_INDENT;
        if (cur_char_n<llen)  // do not indent by inserting real spaces into empty line
        { if (!insert_chars(indent_spaces, sz+2)) break; }
        else
          if (!insert_chars(indent_spaces, 2)) break;
        cur_line_a+=lnlen(cur_line_a);
        cur_line_n++;
        cur_char_n=sz;
       /* delete spaces on the end of line */
        wxChar * p=cur_line_a+sz;
        while (p<editbuf+txtsize && *p==' ') p++;
        if (*p==0xa || *p==0xd)
          while (cur_line_a+sz<editbuf+txtsize &&
                 cur_line_a[sz]==' ')
            del_cur_char(FALSE);
       // update the view:
        total_line_n++;
        update_virtual_size();
        refresh_from_line(cur_line_n-1);
        scroll_to_cursor();
      }
      break;
    }
    case WXK_TAB:
    { if (event.ControlDown()) // switching tabs in the edcont (does not work under Windows: translated to SYSCOMMAND and MDINEXT)
      { if (!in_edcont) event.Skip();
        else
        { int cnt=(int)the_editor_container->GetPageCount(),
              sel=(int)the_editor_container->GetSelection();
          if (cnt<=1 || sel==-1) event.Skip();
          else
            if (event.ShiftDown()) { if (--sel<0) sel=cnt-1; }
            else                   { if (++sel==cnt) sel=0;  }
            the_editor_container->SetSelection(sel);
        }
      }
      else
      { if (noedit) break;
        if (block_on && block_start < block_end)
        { if (event.ShiftDown()) OnUnindentBlock();
          else                   OnIndentBlock();
        }
        else
        { if (event.ShiftDown()) break;  // strange behaviour otherwise (inserted spaces marked as block, repeating deintents)
          int inscount = 4 - (cur_char_n & 3);
          if (insert_chars(wxT("    "), inscount)) cur_char_n+=inscount;
          refresh_current_line();
          scroll_to_cursor();
        }
      }
      break;
    }
    case WXK_F1:
      OnHelp();  break;
    case WXK_F3:  // Find without the dialog
      if (event.ControlDown()) // finding the current word
      { wxString word = read_current_word();
        if (word.IsEmpty()) { wxBell();  return; }
        wxFindDialogEvent find_event(wxEVT_COMMAND_FIND_NEXT, 0);
        find_event.SetFindString(word);
        if (!event.ShiftDown())  // forward
          find_event.SetFlags(last_search_flags | wxFR_DOWN);
        else                    // backward
          find_event.SetFlags(last_search_flags & ~wxFR_DOWN);
        OnFindReplaceAction(find_event);
      }
      else  // finding the last word
      { wxFindDialogEvent find_event(wxEVT_COMMAND_FIND_NEXT, 0);
        find_event.SetFindString(last_search_string);
        if (!event.ShiftDown())  // forward
          find_event.SetFlags(last_search_flags | wxFR_DOWN);
        else                    // backward
          find_event.SetFlags(last_search_flags & ~wxFR_DOWN);
        OnFindReplaceAction(find_event);
      }
      break;
    case WXK_F4:
      if (event.ControlDown()) OnCloseRequest(!event.ShiftDown());  
      else event.Skip();
      break;
    case WXK_F5: // F5 to start, Shift-F5 to continue
      if (event.ControlDown()) break;
      if (the_server_debugger==NULL || the_server_debugger->current_state!=DBGST_BREAKED)
        Run();  
      else
        the_server_debugger->OnCommand(DBG_RUN);
      return;
    case WXK_F7:
      if (Compile()) info_box(_("Syntax is correct."), dialog_parent());  return;

    case WXK_F8:  // originally it was F10 but the editor in a popup window receives only the every other key - collision with invoking the menu
      if (event.ShiftDown() || event.ControlDown() || the_server_debugger==NULL || the_server_debugger->current_state!=DBGST_BREAKED)
        { event.Skip();  return; }
      the_server_debugger->OnCommand(DBG_STEP_OVER);
      return;
    case WXK_F11:
      if (event.ShiftDown() || event.ControlDown() || the_server_debugger==NULL || the_server_debugger->current_state!=DBGST_BREAKED)
        { event.Skip();  return; }
      the_server_debugger->OnCommand(DBG_STEP_INTO);
      return;

    case WXK_F12:
      if (event.ShiftDown())
        if (event.ControlDown())     // CTRL Shift F12: Clear all bookmarks
          BookmarkClearAll();
        else                         // Shift F12: goto previous bookmark
          BookmarkPrev();
      else if (event.ControlDown())  // CTRL F12: toggle bookmark
          BookmarkToggle(cur_line_a);
        else                         // F12: goto next bookmark
          BookmarkNext();
      break;
    case WXK_WINDOWS_MENU:   // application context menu key
      context_menu(false);  break;    // must not skip, may have been closed from the menu
   // Ctrl-keys processed here, on GTK they do not generate Onchar message
    case 'C':
      OnCopy();  break;
    case 'X':
      OnCut();  break;
    case 'V':
      OnPaste();  break;
    case 'A':
      OnSelectAll();  break;
    case 'Z':
      undo_action();  break;
    case 'Y':
      redo_action();  break;
    case 'S':
      if (save_design(event.ShiftDown()))
        wxGetApp().frame->SetStatusText(_("Saved."), 0);
      break;  // Save As when Shift pressed
    case 'F':
      OnFindReplace(false);  break;
    case 'H':
      OnFindReplace(true);  break;
    case 'W':   // close the current editor
      OnCloseRequest(false);  
      break;
//#endif
    default:
      event.Skip();  return;
  }
  if (moved)
  { if (!event.ShiftDown()) hide_block();
    else block_change(prepos);
    scroll_to_cursor();
  }
}

void editor_type::OnChar(wxKeyEvent & event)
{ if (event.AltDown())
  { if (event.GetKeyCode()==WXK_BACK)
      if (event.ShiftDown()) redo_action();  else undo_action();  // must not skip, bells!
    else
      event.Skip();  
    return;
  }
  
  //wxString msg;
  //msg.Printf("OnKeyDown %d", event.GetKeyCode());
  //wxLogDebug(msg);
  
#ifdef STOP  // replaced by catching in OnKeyDown which is compatible with GTK
    //case WXK_BACK:  case WXK_DELETE:  case WXK_RETURN:  case WXK_TAB:    -- processed in OnKeyDown
    case WCTRLC:
      OnCopy();  break;
    case WCTRLX:
      OnCut();  break;
    case WCTRLV:
      OnPaste();  break;
    case WCTRLA:
      OnSelectAll();  break;
    case WCTRLZ:
      undo_action();  break;
    case WCTRLY:
      redo_action();  break;
    case WCTRLS:
      if (save_design(event.ShiftDown()))
        wxGetApp().frame->SetStatusText(_("Saved."), 0);
      break;  // Save As when Shift pressed
    case WCTRLF:
      OnFindReplace(false);  break;
    case WCTRLH:
      OnFindReplace(true);  break;
    case WCTRLW:   // close the current editor
      OnCloseRequest(false);  
      break;
#endif

#ifdef WINS
  if (event.GetKeyCode()>255)
    if (event.GetKeyCode()!=event.m_rawCode) // F4 has the same keycode as r with hacek, both have zero uniChar, the only difference is this
                            // numeric keypad produces 2 chars: 1st is filtered by the condition above, 2nd is OK
      { event.Skip();  return; }
  wxChar c = event.GetUnicodeKey();  // since 2.5.3
#else
  //wxChar c = event.m_uniChar;  // may be >255 for national characters
  //if (!c) 
  //  c = event.GetKeyCode();  // used by digits on the numeric keypad (till 2.5.2)
  wxChar c = event.GetUnicodeKey();  // since 2.5.3
#endif
  if (c>=' ')
  { if (noedit) return;
    bool deleted_block = false;
    if (block_on && block_start<block_end) 
      { seq_delete_block();  deleted_block=true; }
    if (is_overwr && !deleted_block && cur_char_n<txlnlen(cur_line_a))
      replace_char(c);
    else if (!insert_chars(&c, 1)) return;
#ifdef STOP
    if ((cur_char_n+1>=win_xlen) && ((uns8)wParam!=' ') &&
           is_wrap && (categ==0xff)) /* insert soft return */
    { /* find spaces to be replaced by soft return */
      cur_line_a[cur_char_n]=1;  /* char replaced by a mark */
      while (cur_char_n && (cur_line_a[cur_char_n]!=' '))
        cur_char_n--;
      if (cur_line_a[cur_char_n]==' ') cur_line_a[cur_char_n]=0xd;
      wxChar * old_a=cur_line_a;
      cur_line_n++;
      reformat(FALSE);
      cur_line_a=old_a; cur_char_n=0; sz=lnlen(cur_line_a);
      while (cur_line_a[cur_char_n]!=1)
        if (++cur_char_n==sz)
          { cur_line_a+=cur_char_n; cur_char_n=0; sz=lnlen(cur_line_a); }
      cur_line_a[cur_char_n]=(uns8)wParam;  /* mark replaced by the char */
      set_y_curs();
      if (++counter > COUNTER_LIMIT) count_lines();
      change=TRUE;
    }
#endif
    cur_char_n++;
    int start_x_su, start_y_su;
    GetViewStart(&start_x_su, &start_y_su);
    wxRect Rect(/*(cur_char_n-start_x_su-1)*nCharWidth*/0, (cur_line_n-start_y_su)*nLineHeight, (int)(win_xlen*nCharWidth), nLineHeight);
    // redrawing from start of the line, because syntax colouring may change
    //scrolled_panel->Refresh(false, &Rect);  // physical coords!
    // redrawing all because conditionl redrawing of the rest when colouring context changes does not work
    scrolled_panel->Refresh(false);
    scroll_to_cursor();
  }
  else // not a printable char
    event.Skip();  
}

void editor_type::BookmarkClearAll(void)
{ if (!IsEventHandler()) 
    if (bookmarks.del_all()) 
      { scrolled_panel->Refresh();  gutter_panel->Refresh(); }
    else wxBell();
}

void editor_type::BookmarkPrev(void)
{ if (!IsEventHandler()) 
  { t_posit posit = bookmarks.prev(cur_line_a);
    if (posit!=NO_LINENUM) 
      { set_position(posit);  /*Refresh();*/  move_caret(); }
    else wxBell();
  }
}

void editor_type::BookmarkToggle(wxChar * line_a)
{ if (!IsEventHandler()) 
  { if (bookmarks.contains(line_a)) bookmarks.del(line_a);
    else                            bookmarks.add(line_a);
    if (line_a==cur_line_a) refresh_current_line();  else scrolled_panel->Refresh();
    gutter_panel->Refresh();
  }
}

void editor_type::BookmarkNext(void)
{ if (!IsEventHandler()) 
  { t_posit posit = bookmarks.next(cur_line_a);
    if (posit!=NO_LINENUM) 
      { set_position(posit);  /*Refresh();*/  move_caret(); }
    else wxBell();
  }
}

void editor_type::Refresh2(void)
{ scrolled_panel->Refresh();  
  gutter_panel->Refresh();
}

void editor_type::refresh_current_line(void)  // physical coords!
{ int start_x_su, start_y_su;
  GetViewStart(&start_x_su, &start_y_su);
  wxRect Rect(0, (cur_line_n-start_y_su)*nLineHeight, (int)(win_xlen*nCharWidth), nLineHeight);
//  Refresh(false, &Rect);
  scrolled_panel->Refresh(false, &Rect);  gutter_panel->Refresh(false, &Rect);
}

void editor_type::refresh_from_line(int start_line)  // physical coords!
{ int start_x_su, start_y_su;
  GetViewStart(&start_x_su, &start_y_su);
  wxRect Rect(0, (start_line-start_y_su)*nLineHeight, (int)(win_xlen*nCharWidth), (win_ylen+2)*nLineHeight);
  //Refresh(false, &Rect);
  scrolled_panel->Refresh(false, &Rect);  gutter_panel->Refresh(false, &Rect);
}

BOOL editor_type::insert_chars(const wxChar * chs, unsigned len)
// Inserts spaces if current position is after the end of the current line
{ int lnlen = txlnlen(cur_line_a);
  int prespaces = cur_char_n > lnlen ? cur_char_n - lnlen : 0;
  int totalins = prespaces+len;
  if (txtsize+totalins>=edbsize) return FALSE;
  wxChar * pos=cur_line_a+cur_char_n-prespaces;
  memmov(pos+totalins, pos, sizeof(wxChar) * (txtsize-(pos-editbuf)+1));
  txtsize+=totalins;
  adjust(pos,totalins);
  for (int i=0;  i<prespaces;  i++)
    pos[i]=' ';
  memcpy(pos+prespaces, chs, sizeof(wxChar) * len);
  register_action(ACT_TP_INS, pos-editbuf, 0, (wxChar *)(size_t)totalins);
  changed();
  return TRUE;
}

void editor_type::replace_char(wxChar ch)
// The current position is inside a line.
{ wxChar * pos=cur_line_a+cur_char_n;
  register_action(ACT_TP_REPL, pos-editbuf, *pos, NULL);
  *pos=ch;
  changed();
}


/////////////////////////////////////////// syntax colouring /////////////////////////////////
bool editor_type::srv_dbg_mode(void) 
{ return the_server_debugger && the_server_debugger->cdp==cdp && content_category()==CONTCAT_SQL; }

enum t_token { TOK_PLAIN, TOK_COMMENT, TOK_STRING, TOK_KEYWORD, TOK_STDDEF, TOK_DIRECTIVE };
#define MAX_TOKENS 32

struct t_line_tokens
{ unsigned off;
  t_token token;
};

class t_line_kontext  // kontext of a single line
{public: 
  virtual void create_kontext(editor_type * edd, const wxChar * line)=0; // create kontext for the beginning of the "line" in "edd"
  virtual BOOL same(const t_line_kontext * ktx) const=0;
  virtual void tokenize_line(cdp_t cdp, wxChar * line, unsigned len, t_line_tokens * lts, t_content_category ccateg)=0;
   // tokenizes "line" in "this" kontext and updates "this" kontext for the end of the line
};

class t_line_kontexts // array of line kontexts
{public: 
  virtual void store(int linenum, const t_line_kontext * ktx)=0;       // stores the line kontext ktx to   the linenum-th element of the array
  virtual void load (int linenum, t_line_kontext * ktx) const=0;       // loads  the line kontext ktx from the linenum-th element of the array
  virtual BOOL same (int linenum, const t_line_kontext * ktx) const=0; // compares the line kontext ktx to the linenum-th element of the array
};

class t_line_kontext_simple : public t_line_kontext
{ friend class t_line_kontexts_simple;
  wbbool pos_in_comment, pos_in_sql;
 public:
  virtual void create_kontext(editor_type * edd, const wxChar * line); 
  virtual BOOL same(const t_line_kontext * ktx) const;
  virtual void tokenize_line(cdp_t cdp, wxChar * line, unsigned len, t_line_tokens * lts, t_content_category ccateg);
};

class t_line_kontexts_simple : public t_line_kontexts // array of line kontexts
{ enum { MAX_FSED_LINES=250 };
  uns32 start_in_comment[MAX_FSED_LINES / 32 + 1];
  uns32 start_in_sql[MAX_FSED_LINES / 32 + 1];
 public:
  virtual void store(int linenum, const t_line_kontext * ktx); 
  virtual void load (int linenum, t_line_kontext * ktx) const; 
  virtual BOOL same (int linenum, const t_line_kontext * ktx) const;
  t_line_kontexts_simple(void)
  { memset(start_in_comment, 0, sizeof(start_in_comment));
    memset(start_in_sql, 0, sizeof(start_in_sql));
  }
};

/////////////////////////////////////// basic methods /////////////////////////////////////////
static void remove_from_chain(editor_type * editor);

editor_type::editor_type(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn, tcateg categIn, bool auto_read_onlyIn, char ** small_iconIn) : 
  any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, categIn, small_iconIn, &editor_sep_coords),
  auto_read_only(auto_read_onlyIn),
  undo_buffer(UNDO_BUFFER_SIZE), redo_buffer(REDO_BUFFER_SIZE),
  gutter_brush(wxColour(0xe0, 0xe0, 0xe0), wxSOLID),  // must be the same as the background in the gutter symbols!
  gutter_pen(wxColour(0xe0, 0xe0, 0xe0), 1, wxSOLID)
{ 
//  edbsize =edbsizeIn;
//  hOwner  =hParent;
//  position=positionIn;
//  attr    =attrIn;
//  index   =indexIn;
//  cached   =cachedIn;
  popup    =false;
//  is_wrap  =(bool)GetPP()->editor_wrap;  /* the initial state is common */
//  is_align =(bool)GetPP()->editor_align;
//  is_turbo =(bool)GetPP()->editor_turbo;
  is_overwr=false;
  in_edcont=false;
  noedit   =0;
  par_inst =NULL;
  lktx     =NULL;
  lktxs    =NULL;
  wheel_delta=0;
  gutter_panel=NULL;  scrolled_panel=NULL;
  last_gutter_offset=-1;
 // content state:
  block_on=_changed=false;
  y_curs=cur_char_n=0;
  cur_line_n=0;
  txtsize=0;  /* used when text not created yet */
#if 0
  clear_completeword_params();
#endif
  blockpos=NULL;
  cumulative_wheel_delta=0;
  bookmark_gutter=6;
  breakpoint_gutter=10;
  editbuf=NULL;
  m_tooltip=NULL;
  local_drop_flag=NULL;
  profile_mode=0;
  profiler_data=NULL;
}

gutter_editor_panel::gutter_editor_panel(editor_type * my_editorIn) : wxWindow(my_editorIn, 1, wxPoint(0, 0), wxSize(my_editorIn->gutter_size(), -1), 0, wxT("editor_gutter"))
{ my_editor=my_editorIn; 
  popup_line_a=NULL;
}

scrolled_editor_panel::scrolled_editor_panel(editor_type * my_editorIn) : wxWindow(my_editorIn, 2, wxPoint(my_editorIn->gutter_size(), 0), wxDefaultSize, wxWANTS_CHARS|wxFULL_REPAINT_ON_RESIZE, wxT("editor_scolled_pane"))
{ my_editor=my_editorIn; }  // wxFULL_REPAINT_ON_RESIZE used when displaying the profile

void editor_type::ToggleBreakpointLocal(int line_n)
{// store BP locally:
  wxChar * bpline = line_num2ptr(line_n);
  if (breakpts.contains(bpline)) breakpts.del(bpline);
  else                           breakpts.add(bpline);
  if (cur_line_n==line_n) refresh_current_line();  else Refresh2();
}

void gutter_editor_panel::OnLeftButtonDown(wxMouseEvent & event)
{ wxChar * line_a;  int line_n, char_n;
  my_editor->get_mouse_position(event, line_a, line_n, char_n);  // char_n is invalid here
  wxCoord x, y;
  event.GetPosition(&x, &y);  // physical coordinates related to the window client area
  if (my_editor->bookmark_gutter && x<my_editor->bookmark_gutter)
    my_editor->BookmarkToggle(line_a);
  else if (my_editor->breakpoint_gutter) // breakpoint
    if (my_editor->content_category()==CONTCAT_SQL)
      if (the_server_debugger && the_server_debugger->cdp==my_editor->cdp)
        the_server_debugger->ToggleBreakpoint(my_editor, line_n);
      else
        my_editor->ToggleBreakpointLocal(line_n);
    else wxBell();
  my_editor->scrolled_panel->SetFocus();  // return the focus
}

void gutter_editor_panel::OnLeftButtonUp(wxMouseEvent & event)
{ my_editor->scrolled_panel->SetFocus(); }  // return the focus

void gutter_editor_panel::OnRightButtonDown(wxMouseEvent & event)
{ int char_n;
  my_editor->get_mouse_position(event, popup_line_a, popup_line_n, char_n);  // char_n is invalid here
  wxMenu * popup_menu = new wxMenu;
  if (my_editor->bookmark_gutter  ) 
    popup_menu->Append(TD_GUTTER_BOOK,  _("Toggle bookmark"));
  if (my_editor->breakpoint_gutter) 
  { popup_menu->Append(TD_GUTTER_BREAK, _("Toggle breakpoint"));
    popup_menu->Enable(TD_GUTTER_BREAK, my_editor->content_category()==CONTCAT_SQL);
  }
  PopupMenu(popup_menu, ScreenToClient(wxGetMousePosition()));
  delete popup_menu;
  my_editor->scrolled_panel->SetFocus();  // return the focus
}

void gutter_editor_panel::OnCommand(wxCommandEvent & event)
{ if (!popup_line_a) return;
  if (event.GetId()==TD_GUTTER_BOOK && my_editor->bookmark_gutter)  // bookmark
    my_editor->BookmarkToggle(popup_line_a);
  else if (event.GetId()==TD_GUTTER_BREAK && my_editor->breakpoint_gutter)  // breakpoint
    if (my_editor->content_category()==CONTCAT_SQL)
      if (the_server_debugger && the_server_debugger->cdp==my_editor->cdp)
        the_server_debugger->ToggleBreakpoint(my_editor, popup_line_n);
      else
        my_editor->ToggleBreakpointLocal(popup_line_n);
    else wxBell();
}

void gutter_editor_panel::OnPaint(wxPaintEvent& event)
{ wxPaintDC dc(this);
  my_editor->DrawGutter(dc); 
}

void scrolled_editor_panel::OnPaint(wxPaintEvent& event)
{ wxPaintDC dc(this);
  my_editor->DrawScrolled(dc); 
}

bool editor_type::alloc_buffer(unsigned size)
// [size] is in wide characters.
{ edbsize=size;
  editbuf=(wxChar*)corealloc(sizeof(wxChar) * (size+1), 88);
  if (editbuf) *editbuf=0;
  block_start=block_end=cur_line_a=editbuf;
  return editbuf!=NULL;
}

editor_type::~editor_type(void)
{ 
#if 0
  if (object_locked)
    if (par_inst!=NULL)
      general_record_locking(par_inst, cursnum, objnum, FALSE, 2);
#endif
  remove_from_chain(this);
//  if (edit->hOwner) SetFocus(edit->hOwner);  /* restore the focus */ must not do this when closing WinBase
  if (wxGetApp().frame->GetStatusBar()!=NULL)  // unless closing the main window
    wxGetApp().frame->SetStatusText(wxEmptyString, 1);  // remove the cursor position indicator
  delete lktx;  delete lktxs;
  //delete GetDropTarget();  -- crashes
  corefree(editbuf);
  if (m_tooltip) delete m_tooltip;
  if (profiler_data) delete [] profiler_data;
}

void editor_type::set_font(wxFont Font)
{ hFont = Font;
  wxClientDC dc(this);
  dc.SetFont(hFont);
  wxCoord w, h, descent, externalLeading;
  dc.GetTextExtent(wxT("QpRySiwAjs"), &w, &h, &descent, &externalLeading);
  nLineHeight = h/*+descent+externalLeading*/;
  nCharWidth = (double)w/10;
}

bool editor_type::prep(wxWindow * parent)
{ 
  Create(parent, COMP_FRAME_ID, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxVSCROLL | wxCLIP_CHILDREN | wxSUNKEN_BORDER | wxWANTS_CHARS, wxT("editor_main") );
  competing_frame::focus_receiver = this;
  set_font(*text_editor_font);
  lktx =new t_line_kontext_simple;
  lktxs=new t_line_kontexts_simple;
  virtual_width=300;

 // create subwindows:
#ifdef NEW_SCROLL
  wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
  SetSizer(item2);
  SetAutoLayout(true);
  wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
  item2->Add(item3, 1, wxGROW|wxALL, 0);
#endif

  scrolled_panel = new scrolled_editor_panel(this);  // scrolled panel must be the 1st child -> will get focus
  gutter_panel = new gutter_editor_panel(this);

#ifdef NEW_SCROLL
  item3->Add(gutter_panel, 0, wxGROW|wxALL, 0);
  item3->Add(scrolled_panel, 1, wxGROW|wxALL, 0);
  //scrolled_panel->SetVirtualSizeHints(2000, 2000);
  //scrolled_panel->SetVirtualSize(2000, 2000);
  //SetVirtualSizeHints(2000, 2000);
#endif
  SetTargetWindow(scrolled_panel);
  SetScrollRate((int)nCharWidth, nLineHeight);   // in pixels, must be after creating windows and defining scroll target

  scrolled_panel->SetDropTarget(new EditorTextDropTarget(this));
  Show();
  scrolled_panel->new_caret();
  move_caret();
#ifdef ADVTOOLTIP
  m_tooltip = new MyToolTip(scrolled_panel, this); 
#else
  tooltip_timer.SetOwner(scrolled_panel, TOOLTIP_TIMER_ID);
  scrolled_panel->SetToolTip("");  // prepare for later use
#endif
  return true;
}

bool eval_expr_to_string(cdp_t cdp, wxString expr, wxString & res)
// Evaluates [expr] and returns its value as a string. Returns false on error, true iff OK.
{ if (expr.IsEmpty())  // used on entry without initial value and when evaluating an empty value
    { res=wxEmptyString;  return true; }
  wuchar buf2[3+MAX_FIXED_STRING_LENGTH+1];
  if (cd_Server_eval9(cdp, 0, expr.mb_str(*cdp->sys_conv), buf2))  // returns 8-bit string
    { res=_("<Error>");  return false; }
#ifdef WINS
  res=wxString(buf2);  
#else
  wxChar buf4[3+MAX_FIXED_STRING_LENGTH+1];
  superconv(ATT_STRING, ATT_STRING, buf2, (char*)buf4, t_specif(0, 0, 0, 1), t_specif(0, 0, 0, 2), NULL);
  res=wxString(buf4);  
#endif
  return true; 
}
        
#ifdef ADVTOOLTIP
wxString MyToolTip::GetText(const wxPoint& pos, wxRect& boundary)
{ boundary = wxRect(pos.x - 4, pos.y - 4, 8, 8);
  return ed->GetTooltipText(); 
}
//    wxString t;
//    t.Printf(_T("Tooltip for window %p at [%i,%i]"), GetWindow(), pos.x, pos.y);
//    
//    return t;

wxString editor_type::GetTooltipText(void)
{ wxMouseEvent mevent;  wxChar * line_a;  int line_n, char_n;   int width, height;
  if (!the_server_debugger) return wxEmptyString;
  scrolled_panel->GetClientSize(&width, &height);
  wxPoint pt = scrolled_panel->ScreenToClient(wxGetMousePosition());
  if (pt.x<0 || pt.y<0 || pt.x>=width || pt.y>=height || noedit!=2) 
    return wxEmptyString;  // mouse is outside
  mevent.m_x=pt.x;  mevent.m_y=pt.y;
  get_mouse_position(mevent, line_a, line_n, char_n);
  int len = txlnlen(line_a);
  wxString expr;
  if (char_n<len)
	if (block_on && line_a+char_n>=block_start && line_a+char_n<block_end && block_end-block_start<300)  
	{// inside a block
		expr=wxString(block_start, block_end-block_start);
	}
	else
	{ while (char_n        && is_AlphanumW(line_a[char_n-1],  cdp->sys_charset)) char_n--;
    unsigned sz=0;
    while (char_n+sz<len && is_AlphanumW(line_a[char_n+sz], cdp->sys_charset)) sz++;
    expr=wxString(line_a+char_n, sz);
	}
  wxString msg;
  if (!expr.IsEmpty() && eval_expr_to_string(cdp, expr, msg))
    msg = expr+wxT(" = ") + msg;
  return msg;
}
#endif

void editor_type::move_caret(void)
{ wxCaret * caret = scrolled_panel->GetCaret();
  if (caret)
  { int start_x_su, start_y_su;
    GetViewStart(&start_x_su, &start_y_su);  // documentation error: must use physical coordinates, not logical!
    caret->Move((int)((cur_char_n-start_x_su)*nCharWidth), (cur_line_n-start_y_su)*nLineHeight);
  }
  wxString pos_msg;
  pos_msg.Printf(_("Line %u : column %u"), cur_line_n+1, cur_char_n+1);
  wxGetApp().frame->SetStatusText(pos_msg, 1);
}

void scrolled_editor_panel::new_caret(void)
{ wxCaret * caret = GetCaret();
  if (caret)
  { /*if (FindFocus()==this)*/ caret->Hide();
    // delete caret;  -- must not do this, will be deleted in SetCaret()!!
  }
  caret = new wxCaret(this, my_editor->is_overwr ? (int)my_editor->nCharWidth : 2, my_editor->nLineHeight-1);
  SetCaret(caret);
  my_editor->move_caret();
  /*if (FindFocus()==this)*/ caret->Show();  // or Hide if it does not have focus?
}

void editor_type::hide_block(void)
{ if (block_on)
  { scrolled_panel->Refresh(false);
    block_on=false;
  }
  block_end=block_start;
}

void editor_type::block_change(wxChar * prepos)
{ unsigned linelen=lnlen(cur_line_a), txlinelen=txlnlen(cur_line_a);
  wxChar * pos=cur_line_a+(cur_char_n > txlinelen ? linelen : cur_char_n);  // prevents selecting more than the line with the caret or selecting CR without LF
  if (prepos != pos)
  { if      (block_on && (block_start==prepos))
      block_start=pos;
    else if (block_on && (block_end==prepos))
      block_end  =pos;
    else
    { block_start=pos;  block_end=prepos; }
    block_on=true;
    if (block_start > block_end)  /* switch */
    { pos=block_start;  block_start=block_end;
      block_end=pos;
    }
    scrolled_panel->Refresh(false);
  }
}

void editor_type::scroll_to_cursor(void)
// Performs minimal scroll making the cursor visible
// Sometimes win_xlen and win_ylen may be 0 -> should unscroll!
{ int new_x=-1, new_y=-1;  // -1==no scroll
  int start_x_su, start_y_su;
  GetViewStart(&start_x_su, &start_y_su);  // documentation error: must use physical coordinates, not logical!
 // x scroll:
  if (cur_char_n<start_x_su || !win_xlen) new_x=cur_char_n;
  else if (cur_char_n>=start_x_su+win_xlen) new_x=cur_char_n-win_xlen+1;
 // y scroll:
  if (cur_line_n<start_y_su || !win_ylen) new_y=cur_line_n;
  else if (cur_line_n>=start_y_su+win_ylen) new_y=cur_line_n-win_ylen+1;
 // check the virtual width:
  if (cur_char_n >= virtual_width)
  { virtual_width=cur_char_n+1;
    update_virtual_size();
  }
 // do scroll:
  //Scroll(win_xlen ? new_x : 0, win_ylen ? new_y : 0);
  Scroll(new_x, new_y);  // win_xlen and win_ylen are 0 when windows has just been opened (debugger opens the source)
  move_caret();
}

//////////////////////////////////////// tokenisation /////////////////////////////////////////////////
#if 0
void editor_type::get_dims(void)
{ RECT rRect;  TEXTMETRIC tm;  HDC hDC; 
  hDC=GetDC(edwnd);
  SelectObject(hDC, hFont);
  GetTextMetrics(hDC, &tm);
  nLineHeight=tm.tmHeight /*+ tm.tmExternalLeading*/;
  nCharWidth =tm.tmAveCharWidth;
  GetClientRect(edwnd, &rRect);
  win_ylen=rRect.bottom / nLineHeight + 1;
  win_xlen=rRect.right  / nCharWidth  + 1;
  ReleaseDC(edwnd, hDC);
}
#endif

wxChar * editor_type::next_line(wxChar * line) const
{ unsigned len=lnlen(line);
  return line[len] ? line+len : line; 
}

wxChar * editor_type:: prev_line(wxChar * line) const
{ unsigned offs;
  offs=line-editbuf;
  if (offs<2) return (wxChar*)editbuf;
#if 0  // old CR-centric version
  if (editbuf[offs-1]==0xa)
    if (offs<3) return (wxChar*)editbuf; else offs-=3;
  else offs-=2;
  while (editbuf[offs] && (editbuf[offs]!=0xd))
    if (offs) offs--; else return (wxChar*)editbuf;
 /* must not stop on LF, LF may have been imported without CR & is not line break */
  offs++;
  while (editbuf[offs]==0xa) offs++;
#else
  offs-=1;  // the last char of the previous line
  while (offs && editbuf[offs-1]!=0xa) offs--;
#endif
  return (wxChar*)editbuf+offs;
}


wxChar * editor_type::line_num2ptr(int num)
{ if (num<0) return NULL;
  wxChar * line_a;  int line_n;
  if (num>=cur_line_n/2) 
    { line_a = cur_line_a;  line_n=cur_line_n; } // start searching from the current line
  else
    { line_a = editbuf;     line_n=0; }          // start searching from the beginning
  while (num<line_n)
    { line_a = prev_line(line_a);  line_n--; }
  while (num>line_n)
  { wxChar * nx = next_line(line_a);
    if (nx==line_a) return NULL;  // line with this number does not exist
    line_a=nx;  line_n++;
  }
  return line_a;
}

int editor_type::line2num(wxChar * line)
{ int num = 0;
  do
  { wxChar * prev = prev_line(line);
    if (prev==line) break;
    num++;
    line=prev;
  } while (true);
  return num;
}

CFNC DllKernel int   WINAPI check_atr_name(char * name);
#define IDCL_IPL_KEY    1
#define IDCL_SQL_KEY    2
#define IDCL_STDID      4
#define IDCL_VIEW       8
#define IDCL_SQL_STDID 16

void editor_type::get_comment_kontext(const wxChar * line, wbbool * in_comment, wbbool * in_sql) const
// Used when redrawing all or all starting from a given line.
{ const wxChar * aline=line;  
  for (int i=0;  i<30;  i++) aline=cprev_line(aline);
  BOOL incomm=FALSE;
  *in_sql=FALSE;
  while (aline < line)
  { int off = 0, len=txlnlen(aline);
   // test for directives sqlbegin/sqlend: 
    const wxChar * pline=aline;  while (*pline==' ') pline++;
    if (*pline=='#')
    { //wxChar buf[10+1];
      //strmaxcpy(buf, pline, sizeof(buf));  Upcase(buf);     
      //if      (!memcmp(buf, "#SQLBEGIN", 9)) *in_sql=TRUE;
      //else if (!memcmp(buf, "#SQLEND",   7)) *in_sql=FALSE;
      if (!_tcsnicmp(pline, wxT("#SQLBEGIN"), 9)) *in_sql=TRUE;
      else if (!_tcsnicmp(pline, wxT("#SQLEND"), 7)) *in_sql=FALSE;
    }
   // test for comment beginning/end:
    wxChar active_quote = 0;  // none when 0, the quote character if any active
    while (off<len)
    { if (incomm)
      { if (aline[off]=='}') incomm=FALSE;
        else if (aline[off]=='*' && aline[off+1]=='/') { off++;  incomm=FALSE; }
      }
      else if (active_quote)
      { if (aline[off]==active_quote)
          active_quote=0;  // double quote inside the active quote - no special processing necessary, will be restarted
      }
      else // check for starting the comment/quote:
      { if (aline[off]=='"' || aline[off]=='\'' || aline[off]=='`')
          active_quote=aline[off];
        else if (aline[off]=='{') incomm=TRUE;
        else if (aline[off]=='/' && aline[off+1]=='*') { off++;  incomm=TRUE; }
        else if (aline[off]=='/' && aline[off+1]=='/' ||
                 aline[off]=='-' && aline[off+1]=='-') off=len-1;
      }
      off++;
    }
    aline=cnext_line(aline);
  }
  *in_comment=incomm;
}

//////////// t_line_kontext_simple:
void t_line_kontext_simple :: create_kontext(editor_type * edd, const wxChar * line)
{ edd->get_comment_kontext(line, &pos_in_comment, &pos_in_sql); }

BOOL t_line_kontext_simple :: same(const t_line_kontext * ktx) const
{ return pos_in_comment==((t_line_kontext_simple*)ktx)->pos_in_comment && pos_in_sql==((t_line_kontext_simple*)ktx)->pos_in_sql; }

void t_line_kontexts_simple::store(int linenum, const t_line_kontext * ktx)
{ t_line_kontext_simple * sktx = (t_line_kontext_simple*)ktx;
  if (sktx->pos_in_comment) start_in_comment[linenum/32] |= 1<<(linenum%32);
  else                     start_in_comment[linenum/32] &= ~(1<<(linenum%32));
  if (sktx->pos_in_sql)     start_in_sql[linenum/32]     |= 1<<(linenum%32);
  else                     start_in_sql[linenum/32]     &= ~(1<<(linenum%32));
}

void t_line_kontexts_simple::load (int linenum, t_line_kontext * ktx) const 
{ t_line_kontext_simple * sktx = (t_line_kontext_simple*)ktx;
  sktx->pos_in_comment=(start_in_comment[linenum/32] & 1<<(linenum%32))!=0;
  sktx->pos_in_sql    =(start_in_sql[linenum/32]     & 1<<(linenum%32))!=0;
}
BOOL t_line_kontexts_simple::same (int linenum, const t_line_kontext * ktx) const
{ t_line_kontext_simple aktx;
  load(linenum, &aktx);
  return aktx.same(ktx);
}

void t_line_kontext_simple::tokenize_line(cdp_t cdp, wxChar * line, unsigned len, t_line_tokens * lts, t_content_category ccateg)
{ unsigned off = 0, cnt = 0;  BOOL local_sql = FALSE;
  if (ccateg==CONTCAT_TEXT) // plain text
  { lts[0].off=0;  lts[0].token=TOK_PLAIN;  lts[1].off=(unsigned)-1;
    return;
  }
 // test for directive:
  wxChar * pline=line;  while (*pline==' ') pline++;
  if (*pline=='#')
  { off=pline-line;
    wxChar dirname[10+1];  int i=0;
    do 
    { if (i<10) dirname[i++]=line[off];
      off++; 
    }
    while (is_AlphaW(line[off], cdp->sys_charset));
    dirname[i]=0;
    lts[0].off=0;  lts[0].token=TOK_DIRECTIVE; 
    cnt=1;
    lts[1].off=off;  lts[1].token=TOK_PLAIN; // to the normal state
    if      (!_tcsicmp(dirname, wxT("#SQL")))      local_sql =TRUE;
    else if (!_tcsicmp(dirname, wxT("#SQLBEGIN"))) pos_in_sql=TRUE;
    else if (!_tcsicmp(dirname, wxT("#SQLEND")))   pos_in_sql=FALSE;
  }
 // INV: analyse the line from off, store token or terminator to lts[cnt]
  while (off<len && cnt+2<MAX_TOKENS)
  { lts[cnt].off=off;  // token start
   // INV: scanning token to to lts[cnt], its beginning stored to lts[cnt].off
    if (pos_in_comment)
    { lts[cnt].token=TOK_COMMENT;
      while (off < len)
      { if (line[off]=='}') { off++;  pos_in_comment=wbfalse;  break; }
        if (line[off]=='*' && off+1 < len && line[off+1]=='/') { off+=2;  pos_in_comment=wbfalse;  break; }
        off++;
      }
      cnt++;
    }
    else  // not in comment
    { lts[cnt].token=TOK_PLAIN;   // supposed token type
      while (off<len && cnt+2<MAX_TOKENS) // extending TOK_PLAIN in lts[cnt]
      { if (line[off]=='\'' || line[off]=='\"')  // start string token
        { if (off > lts[cnt].off) lts[++cnt].off=off;  // new token start
          lts[cnt].token=TOK_STRING; 
          off++;
          while (off<len)  // find the end of the string
          { if (line[off]=='\'' || line[off]=='\"')
              if (off+1>len || line[off+1]!='\'' && line[off+1]!='\"')
                { off++;  break; }
              else off+=2;
            else off++;
          }
          lts[++cnt].off=off;  lts[cnt].token=TOK_PLAIN; // to the normal state
        }
        else if (is_AlphaW(line[off], cdp->sys_charset))
        { tobjname name;  int i=0;
          unsigned idstart=off;
          do
          { if (i<OBJ_NAME_LEN) name[i++]=line[off];
          } while (++off<len && is_AlphanumW(line[off], cdp->sys_charset));
          name[i]=0;
          convert_to_uppercaseA(name, name, cdp->sys_charset);
          int idtype=check_atr_name(name);
          if ((idtype & IDCL_IPL_KEY) && ccateg==CONTCAT_CLIENT && !local_sql && !pos_in_sql ||
              (idtype & IDCL_SQL_KEY) && (ccateg==CONTCAT_SQL || ccateg==CONTCAT_SQL_NODEBUG || local_sql || pos_in_sql) ||
              (idtype & IDCL_VIEW)    && ccateg==CONTCAT_CLIENT)
          { if (idstart > lts[cnt].off) lts[++cnt].off=idstart;  
            lts[cnt].token=TOK_KEYWORD; 
            lts[++cnt].off=off;  lts[cnt].token=TOK_PLAIN; // to the normal state
          }
          else if (idtype & IDCL_STDID ||
                  (idtype & IDCL_SQL_STDID) && (ccateg==CONTCAT_SQL || ccateg==CONTCAT_SQL_NODEBUG))
          { if (idstart > lts[cnt].off) lts[++cnt].off=idstart;  
            lts[cnt].token=TOK_STDDEF; 
            lts[++cnt].off=off;  lts[cnt].token=TOK_PLAIN; // to the normal state
          }
        }
        else if (line[off]=='{' || line[off]=='/' && off+1<len && line[off+1]=='*')
        { pos_in_comment=wbtrue;  
          break;
        }
        else if ((line[off]=='-' || line[off]=='/') && off+1<len && line[off+1]==line[off])
        { if (off > lts[cnt].off) lts[++cnt].off=off;
          lts[cnt].token=TOK_COMMENT;  off=len;
        }
        else if (line[off]=='`')
        { while (line[++off]!='`' && off<len) ;
          off++;
        }
        else off++;
      }
      if (off > lts[cnt].off) cnt++;  // end of line or comment started
    } // not in comment
  }
  lts[cnt].off=(unsigned)-1;  // storing the terminator
}


void write_tokenized(wxDC & dc, int & act_x, int & act_y, wxChar * line, unsigned len, unsigned loff, t_line_tokens * lts)
// [loff] if [line]-[real start of the line]
// [len] if the number of character to be written
{ unsigned tpos=0, tokensize;  wxColour basic_text;  t_token tok;
  while (len)
  { if (lts)  // colouring ON
    { while (loff>=lts[tpos+1].off) tpos++;
      tokensize = lts[tpos+1].off-loff;
      if (tokensize > len) tokensize=len;
      tok = lts[tpos].token;
    }
    else      // colouring OFF
    { tokensize=len;
      tok=TOK_PLAIN;
    }
    if (tok!=TOK_PLAIN)
    { basic_text = dc.GetTextForeground();  wxColour rgb;
      switch (tok)
      { case TOK_COMMENT:   rgb=wxColour(0x0, 0x80, 0x0);  break;
        case TOK_STRING:    rgb=wxColour(0x0, 0x0, 0x80);  break;
        case TOK_KEYWORD:   rgb=wxColour(0x0, 0x0, 0xc0);  break;
        case TOK_STDDEF:    rgb=wxColour(0x80, 0x0, 0x0);  break;
        case TOK_DIRECTIVE: rgb=wxColour(0x80, 0x0, 0x80); break;
      }
      dc.SetTextForeground(rgb);
    }
    wxString str(line, tokensize < 3000 ? tokensize : 3000);  wxCoord w, h;  // MSW TextOut fails on long strings
    dc.GetTextExtent(str, &w, &h);
    dc.DrawText(str, act_x, act_y);
    act_x+=w;
    if (tok!=TOK_PLAIN) dc.SetTextForeground(basic_text);
    line+=tokensize;  loff+=tokensize;  len-=tokensize;
  }
}

#define GUTTER_SYMBOL_HEIGHT 10

#include "bmps/act_line_breakpoint.xpm"
#include "bmps/break_symbol.xpm"
#include "bmps/act_line.xpm"
#include "bmps/bookmark.xpm"

void editor_type::draw_gutter_symbol(wxDC & dc, int x, int y, int width, int height, char ** bitmap)
// x, y in logical coordinates.
// Centers the bitmap vertically, draws light gray above and below.
// If [bitmap]==NULL, fills space with light gray.
// Bitmap width is supposed to be the same as [width].
{ int space_above = (height > GUTTER_SYMBOL_HEIGHT) ? (height - GUTTER_SYMBOL_HEIGHT) / 2 : 0;
 // fill the space above the bitmap or all the space if the bitmap is not present:
  if (!bitmap) space_above=height;
  if (space_above)  
  { dc.SetBrush(gutter_brush);
    dc.SetPen(gutter_pen);
    dc.DrawRectangle(x, y, width, space_above);   
    dc.SetBrush(wxNullBrush);
    dc.SetPen(wxNullPen);
  }
  if (bitmap)
  { wxBitmap bmp(bitmap);
    dc.DrawBitmap(bmp, x, y+space_above, true);
   // fill space below the bitmap:
    if (height > GUTTER_SYMBOL_HEIGHT)
    { dc.SetBrush(gutter_brush);
      dc.SetPen(gutter_pen);
      dc.DrawRectangle(x, y+space_above+GUTTER_SYMBOL_HEIGHT, width, height-GUTTER_SYMBOL_HEIGHT-space_above);   
      dc.SetBrush(wxNullBrush);
      dc.SetPen(wxNullPen);
    }
  }
}

void editor_type::get_debug_pos(int & pcline, int & filenum)
{// In [pcline] returns line number of the current debugged program location if it is in the edited file, -1 otherwise.
 // In [filenum] returns the index in the edited file in the debugged program.
  pcline = -1;
  if (local_debug_break())
  { //filenum=find_file_num(cdp, (tobjnum)objnum);
    if (filenum==cdp->RV.bpi->current_file)
      pcline=cdp->RV.bpi->current_line;
  }
  else if (srv_dbg_mode())
  { if (objnum==the_server_debugger->stop_objnum)
      pcline=the_server_debugger->stop_line;
  }
}

void editor_type::DrawGutter(wxDC & dc)
{ int start_x_su, start_y_su;
  GetViewStart(&start_x_su, &start_y_su); 
#ifdef UPDATE_BY_REGION  // regions are bad!!!
  wxRect rectUpdate = GetUpdateRegion().GetBox();  // gutter is not a scrolled window, phys=log
  int lineFrom = rectUpdate.y / nLineHeight, lineTo = rectUpdate.GetBottom() / nLineHeight;  // update including(!) lineFrom and lineTo
#else
  int lineFrom=0, lineTo=win_ylen;  // drawing the full window
#endif
  last_gutter_offset = start_y_su;
  int ln = lineFrom;                    // counting related to the window
  int linenum = lineFrom + start_y_su;  // counting related to the text
 // find the text start for lineFrom into xline:
  wxChar * xline = line_num2ptr(linenum);
  int pcline, filenum;  get_debug_pos(pcline, filenum);
  if (xline)
  { while (ln <= lineTo)
    { wxCoord act_y = ln*nLineHeight;
     // get effects info:
      bool bp_here = /*local_debug_break() ? is_bp_set(cdp, filenum, linenum+1)==BP_NORMAL :*/ breakpts.contains(xline);
      bool current_pos_here = linenum+1==pcline; 
      bool bookmark_here = bookmarks.contains(xline);
      char ** bp_pos_symbol;
     // set text and background colour:
      if (bp_here)
        if (current_pos_here) // current position with BP
          bp_pos_symbol = act_line_breakpoint_xpm;
        else
          bp_pos_symbol = break_symbol_xpm;
      else  /* no BP here */
        if (current_pos_here)  // current position without BP
          bp_pos_symbol = act_line_xpm;
        else 
          bp_pos_symbol = NULL;
     // draw symbols on the gutter:
      if (bookmark_gutter) 
        draw_gutter_symbol(dc, 0, act_y, bookmark_gutter-1, nLineHeight, bookmark_here ? bookmark_xpm : NULL);
      if (breakpoint_gutter) 
        draw_gutter_symbol(dc, bookmark_gutter, act_y, breakpoint_gutter, nLineHeight, bp_pos_symbol);
     // go to the next line:
      linenum++;  ln++;
      wxChar * newl = next_line(xline);
      if (xline==newl) break;
      xline=newl;  
    }
  }
 // fill the space below lines:
  if (ln <= lineTo)
  { dc.SetBrush(gutter_brush);
    dc.SetPen(gutter_pen);
    dc.DrawRectangle(0, ln*nLineHeight, gutter_size(), (lineTo-ln+1)*nLineHeight);   
    dc.SetBrush(wxNullBrush);
    dc.SetPen(wxNullPen);
  }
 // draw the gutter division line:
  if (bookmark_gutter && breakpoint_gutter)
  { dc.SetPen(*wxWHITE_PEN);
    dc.DrawLine(bookmark_gutter-1, 0, bookmark_gutter-1, nLineHeight*(win_ylen+1));
    dc.SetPen(wxNullPen);
  }
}

void editor_type::OnDraw(wxDC& dc)
 { }

const t_prof_data * editor_type::get_profiler_data(uns32 linenum)
{ 
  t_prof_data * pd = profiler_data;
  if (pd)
    while (pd->line!=(uns32)-1)
      if (pd->line<linenum) pd++;
      else if (pd->line==linenum) return pd;
      else break;
  return NULL;  // line not found or profile not loaded
}

t_prof_data * load_profile_data(cdp_t cdp, tobjnum objnum)
{ char query[200];  tcursnum cursnum;  trecnum cnt, r;  
  t_prof_data * pd = NULL;
  sprintf(query, "select line_number,hit_count,brutto_time,netto_time from _iv_profile where object_number=%u and category_name='Source line' order by line_number", objnum);
  if (cd_Open_cursor_direct(cdp, query, &cursnum))
    cd_Signalize(cdp);
  else
  { cd_Rec_cnt(cdp, cursnum, &cnt);
    if (cnt)
    { pd = new t_prof_data[cnt+1];
      if (pd) 
      { uns32 maxbrutto = 0;
        for (r=0;  r<cnt;  r++)
        { cd_Read(cdp, cursnum, r, 1, NULL, &pd[r].line);
          cd_Read(cdp, cursnum, r, 2, NULL, &pd[r].hits);
          cd_Read(cdp, cursnum, r, 3, NULL, &pd[r].brutto);
          cd_Read(cdp, cursnum, r, 4, NULL, &pd[r].netto);
          if (maxbrutto < pd[r].brutto) maxbrutto = pd[r].brutto;
        }
        pd[cnt].line=(uns32)-1;  // terminator
       // normalize data into the interval 0..10000:
        if (maxbrutto)
          for (r=0;  r<cnt;  r++)
          { pd[r].brutto = pd[r].brutto * 10000 / maxbrutto;
            pd[r].netto  = pd[r].netto  * 10000 / maxbrutto;
          }
      }
    }
    cd_Close_cursor(cdp, cursnum);
  }
  return pd;  // NULL on errros or when no data are available
}

void editor_type::refresh_profile(void)
{ if (category!=CATEG_PROC) return;  // fuse
  if (profiler_data) { delete [] profiler_data;  profiler_data=NULL; }  // should be impossible bu who knows?
  profiler_data=load_profile_data(cdp, objnum);
  if (profiler_data)
  { if (!profile_mode) profile_mode=1;  // preserving other modes
  }
  else
    profile_mode=0;
  designer_menu->Check(TD_CPM_PROFILE, profiler_data!=NULL);
  my_toolbar()->ToggleTool(TD_CPM_PROFILE, profiler_data!=NULL);
  scrolled_panel->Refresh();
}

void editor_type::DrawScrolled(wxDC & dc)
{ 
  if (FindFocus()==this) GetCaret()->Hide();
  dc.SetFont(hFont);
  int start_x_su, start_y_su;
  GetViewStart(&start_x_su, &start_y_su); 
  if (start_y_su!=last_gutter_offset && gutter_panel) gutter_panel->Refresh();
#ifdef UPDATE_BY_REGION  // regions are bad!!!
  wxRegionIterator upd(GetUpdateRegion()); 
 // update region is always in device coords:
  while (upd)
  { wxRect rectUpdate = upd.GetRect();// = GetUpdateRegion().GetBox();
    int lineFrom = rectUpdate.y / nLineHeight, lineTo = rectUpdate.GetBottom() / nLineHeight;  // update including(!) lineFrom and lineTo
    { char buf[100];  sprintf(buf, "from line %d to %d", lineFrom, lineTo);
      log_client_error(buf);
    }
#else
    int lineFrom=0, lineTo=win_ylen;  // drawing the full window
#endif    
    int ln = lineFrom;                    // counting related to the window
    int linenum = lineFrom + start_y_su;  // counting related to the text
   // find the text start for lineFrom into xline:
    wxChar * xline = line_num2ptr(linenum);
    int pcline, filenum;  get_debug_pos(pcline, filenum);

    if (xline)
    { if (lineFrom==lineTo) lktxs->load(ln, lktx);  // get kontext from array of kontexts
      else lktx->create_kontext(this, xline); // calculate kontext for the 1st line
      while (ln <= lineTo)
      { //CalcScrolledPosition(0, linenum*nLineHeight, NULL, &phys_y);  // physical y-position for the line
        wxCoord act_y = ln*nLineHeight;
       // get effects info:
        bool bp_here = /*local_debug_break() ? is_bp_set(cdp, filenum, linenum+1)==BP_NORMAL :*/ breakpts.contains(xline);
        bool current_pos_here = (linenum+1==pcline); 
        bool bookmark_here = bookmarks.contains(xline);
        //char ** bp_pos_symbol;
       // set text and background colour:
        dc.SetBackgroundMode(profile_mode ? wxTRANSPARENT : wxSOLID);
        dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
        if (bp_here)
          if (current_pos_here) // current position with BP
          { dc.SetTextBackground(wxColour(0x0, 0x0, 0x7f));
            dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            //bp_pos_symbol = act_line_breakpoint_xpm;
          }
          else
          { dc.SetTextBackground(wxColour(0xff, 0x80, 0x80));
            //bp_pos_symbol = break_symbol_xpm;
          }
        else  /* no BP here */
          if (current_pos_here)  // current position without BP
          { dc.SetTextBackground(wxColour(0x0, 0x0, 0x7f));
            dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            //bp_pos_symbol = act_line_xpm;
          }
          else 
          { if (bookmark_here)
              dc.SetTextBackground(wxColour(0x80, 0xff, 0x80));
            else  // normal line
              dc.SetTextBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            //bp_pos_symbol = NULL;
          }

       // analyse the line:
        wxChar * line=xline;
        int fulllen=txlnlen(line);
       // save the kontext for the current line:
        if (lineFrom!=lineTo) lktxs->store(ln, lktx);
        t_line_tokens lts[MAX_TOKENS];
        lktx->tokenize_line(cdp, line, fulllen, lts, content_category());
       // if the line kontext changed for the next line, redraw the next lines:
       // causes infinite cycle of refreshing:
        //if (!lktxs->same(ln+1, lktx)) 
        //  refresh_from_line(/*lineTo+1*/ln+1);  // lineTo is win_ylen because regions do not work
       // profile background:
        if (profile_mode)
        { int xpos = 0;
          int displ = (int)(nCharWidth*start_x_su);
          wxBrush hBrush(wxColour(0x80, 0x80, 0xff), wxSOLID);
          wxPen hPen(wxColour(0x80, 0x80, 0xff), 0);
          dc.SetBrush(hBrush);
          dc.SetPen(hPen);
          const t_prof_data * prd = get_profiler_data(linenum+1);
          if (prd && prd->netto)
          { int pos2 = (int)(nCharWidth*win_xlen)*prd->netto/10000;
            dc.DrawRectangle(xpos-displ, act_y, pos2, nLineHeight);   
            xpos+=pos2;
          }
          if (prd && prd->brutto>prd->netto)
          { int pos2 = (int)(nCharWidth*win_xlen)*(prd->brutto-prd->netto)/10000;
            hBrush.SetColour(wxColour(0xc0, 0xc0, 0xff));
            dc.SetBrush(hBrush);
            hPen.SetColour(wxColour(0xc0, 0xc0, 0xff));
            dc.SetPen(hPen);
            dc.DrawRectangle(xpos-displ, act_y, pos2, nLineHeight);   
            xpos+=pos2;
          }
          hBrush.SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
          dc.SetBrush(hBrush);
          hPen.SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
          dc.SetPen(hPen);
          dc.DrawRectangle(xpos-displ, act_y, (int)(nCharWidth*(start_x_su+win_xlen+1)), nLineHeight);   
          dc.SetBrush(wxNullBrush);
          dc.SetPen(wxNullPen);
         // print hits:
          if (prd)
          { wxString hits;  hits.Printf(wxT("%u"), prd->hits);  wxCoord w, h;
            dc.GetTextExtent(hits, &w, &h);
            wxColour basic_text = dc.GetTextForeground();  
            dc.SetTextForeground(wxColour(0xff, 0x80, 0x80));
            dc.DrawText(hits, (int)(nCharWidth*(win_xlen-start_x_su))-w, act_y);
            dc.SetTextForeground(basic_text);
          }
        }
       // output:
        wxCoord act_x=0;  /* line rectangle */
    
        if (fulllen>start_x_su)
        {// calc off1 and off2: the start and end offset of the selection on the line 
          unsigned off1, off2;
          if (block_on && (line<block_end) && (line+fulllen>=block_start))
          { off1=(block_start<=line) ? 0 : block_start-line;
            off2=(block_end>=line+fulllen) ? fulllen : block_end-line;
          }
          else off1=off2=0;  // no selection on the line
         /* 1st non-selected part of the line */
          if (off1>start_x_su)  // implies off1>0
            write_tokenized(dc, act_x, act_y, line+start_x_su, off1-start_x_su, start_x_su, current_pos_here ? NULL : lts);
         /* selected part of the line: will overwrite the profile info */
          if (off2>off1 && off2>start_x_su)
          { if (profile_mode) dc.SetBackgroundMode(wxSOLID);
            wxColour oldbackcolor=dc.GetTextBackground();
            wxColour oldtextcolor=dc.GetTextForeground();
            dc.SetTextForeground(oldbackcolor);
            dc.SetTextBackground(oldtextcolor);
            int start = max(off1, start_x_su);
            write_tokenized(dc, act_x, act_y, line+start, off2-start, start, NULL);
           /* prepare writing the 3rd part */
            dc.SetTextForeground(oldtextcolor);
            dc.SetTextBackground(oldbackcolor);
            if (profile_mode) dc.SetBackgroundMode(wxTRANSPARENT);
          }
         // final non-selected part of the line:
          if (fulllen>off2 && fulllen>start_x_su) 
          { int start = max(off2, start_x_su);
            write_tokenized(dc, act_x, act_y, line+start, fulllen-start, start, current_pos_here ? NULL : lts);
          }
        }
       // fill the line tail:
        if (!profile_mode)
        { wxBrush hBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW), wxSOLID);
          dc.SetBrush(hBrush);
          dc.SetPen(*wxWHITE_PEN);
          dc.DrawRectangle(act_x, act_y, (int)(nCharWidth*(start_x_su+win_xlen+1)), nLineHeight);   
          dc.SetBrush(*wxWHITE_BRUSH); // wxNullBrush is incompatible with SetBackgroundMode()
          dc.SetPen(wxNullPen);
        }
       // write hard return mark:
    #if 0
        if (categ==0xff)
          if ((line[i]==0xd)&&(line[i+1]==0xa) || !line[i])
            TextOut(hDC, i*nCharWidth, rRect.top, HARD_RET_MARK, 1);
    #endif
       // go to the next line:
        linenum++;  ln++;
        wxChar * newl = next_line(xline);
        if (xline==newl) break;
        xline=newl;  
      }
    }
   // clear the space below the text (above cycle breaked on the end of the text):
    if (ln <= lineTo)
    { wxBrush hBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW), wxSOLID);
      dc.SetBrush(hBrush);
      dc.SetPen(*wxWHITE_PEN);
      dc.DrawRectangle(0, ln*nLineHeight, (int)(nCharWidth*(win_xlen+1)), nLineHeight*(win_ylen+1));   
      dc.SetBrush(wxNullBrush);
      dc.SetPen(wxNullPen);
    }
#ifdef STOP
    upd++;
  }
#endif
  if (FindFocus()==this) GetCaret()->Show();  // hidden above
}
///////////////////////////////////////////////////////////////////
void editor_type::register_action(uns8 action, unsigned offset, wxChar short_data, wxChar * ptr)
// Must be called before setting "changed".
{ uns8 flags = _changed ? 0 : ACTFL_WAS_UNCHANGED;
  if (undo_state==UDS_UNDOING)
    redo_buffer.add(action, flags, offset, short_data, ptr);
  else  // normal action or redoing
  { undo_buffer.add(action, flags, offset, short_data, ptr);
    if (undo_state==UDS_NORMAL) redo_buffer.clear_all();
  }
  update_toolbar();
}

BOOL editor_type::delete_block(void)
{ unsigned sz, xmv;
 /* initial tests */
  if (block_end   > editbuf+txtsize-2 && block_end[-2] == '\r' && block_end[-1] == '\n') block_end   = editbuf+txtsize-2; // must not delete CRLF
  if (block_start > editbuf+txtsize  ) block_start = editbuf+txtsize;
  if (!block_on || block_start>=block_end) return FALSE;
  sz=block_end-block_start;
  if (sz > txtsize) return FALSE;
  bool back = block_end <= cur_line_a + cur_char_n;
 /* if the current position OR current line start is inside the block,
    move it to the begining of the block */
  if (cur_line_a < block_end && cur_line_a+cur_char_n > block_start)
  { if (cur_line_a+cur_char_n > block_end)
      xmv=(unsigned)(cur_line_a+cur_char_n - block_end);
    else xmv=0;
    while (cur_line_a > block_start)
      cur_line_a=prev_line(cur_line_a);
    cur_char_n=(unsigned)(block_start-cur_line_a)+xmv;
  }
  while (cur_line_a > block_start)
    cur_line_a=prev_line(cur_line_a);
 /* delete the block */
  del_chars(block_start, sz, back);
 /* delete block marks */
  block_end=block_start;  block_on=false;
 /* check to see if current line exists: */
  if (cur_line_a >= editbuf+txtsize)
    cur_line_a=prev_line(cur_line_a);
  count_lines();   /* !!, must update curr_line_n */
  scrolled_panel->Refresh(false);  gutter_panel->Refresh(false);
  return TRUE;
}

void editor_type::adjust(wxChar * pos, int sz)
/* adjusts block pointers when text starting by pos is moved sz bytes up */
{ if (pos<block_start)
    if ((sz>=0) || (block_start+sz>=pos)) block_start+=sz;
    else block_start=pos;
  if (pos<block_end)
    if ((sz>=0) || (block_end  +sz>=pos)) block_end  +=sz;
    else block_end  =pos;
  bookmarks.adjust(pos, sz);
  breakpts.adjust(pos, sz);
}

void editor_type::del_chars(wxChar * pos, int count, BOOL back)
{ if (count==1)
    register_action(back ? ACT_TP_BS1 : ACT_TP_DEL1, pos-editbuf, *pos, NULL);
  else
  { wxChar * data = (wxChar *)corealloc(sizeof(wxChar) * (count+1), 74);
    if (data==NULL) undo_buffer.clear_all();  // cannot store undo, clear undo buffer
    else 
    { memcpy(data, pos, sizeof(wxChar) * count);  data[count]=0;
      register_action(back ? ACT_TP_BSn : ACT_TP_DELn, pos-editbuf, 0, data);
    }
  }
 // delete it:
  memmov(pos, pos+count, sizeof(wxChar) * (txtsize-(pos-editbuf)-count+1));
  txtsize-=count;
  adjust(pos,-count);
  changed();
}

bool any_profiling_on(cdp_t cdp)
{ uns32 state;
  cd_Get_server_info(cdp, OP_GI_PROFILING_ALL, &state, sizeof(state));
  if (state!=0) return true;
  tcursnum curs;  trecnum cnt=0;
  if (cd_SQL_execute(cdp, "select profiled_thread from _iv_logged_users where profiled_thread", (uns32*)&curs))
  { cd_Rec_cnt(cdp, curs, &cnt);
    cd_Close_cursor(cdp, curs);
  }
  return cnt>0;
}

void editor_type::toggle_profile_mode(void)
{ if (profile_mode) 
  { profile_mode=0;
    if (profiler_data) { delete [] profiler_data;  profiler_data=NULL; }
    designer_menu->Check(TD_CPM_PROFILE, false);
    my_toolbar()->ToggleTool(TD_CPM_PROFILE, false);
    scrolled_panel->Refresh();
  }
  else
  { refresh_profile();  // displays the profile
    if (!profiler_data)
    { uns32 state;
      cd_Get_server_info(cdp, OP_GI_PROFILING_LINES, &state, sizeof(state));
      if (state==0)  // not profiling lines
      { console_action(IND_PROFILER, cdp);
        info_box(_("Profiling source lines is off."), dialog_parent());
      }
      else if (!any_profiling_on(cdp))
      { console_action(IND_PROFILER, cdp);
        info_box(_("Profiling is off."), dialog_parent());
      }
      else
        info_box(_("No profiler data available for this routine."), dialog_parent());
    }
  }
}

void editor_type::changed(void)
{ if (!_changed) 
    { _changed=true;  ch_status(true); }  // [_changed] must be changed before calling ch_status()!
  else _changed=true;
 // hide the profile:
  if (profile_mode) toggle_profile_mode();
}

void editor_type::fill_line(void)
{ if (cur_char_n > txlnlen(cur_line_a))
  { cur_char_n--;
    insert_chars(wxT(" "), 1);
    cur_char_n++;
  }
}

static UINT counter=0;  /* counting enters */
#define COUNTER_LIMIT  8

void editor_type::update_virtual_size(void)
{ 
#ifdef NEW_SCROLL
  SetVirtualSize(nCharWidth*virtual_width, nLineHeight*(total_line_n+1));  // in pixels
  SetVirtualSizeHints(nCharWidth*virtual_width, nLineHeight*(total_line_n+1));
#endif
  int w = (int)(nCharWidth*virtual_width);
  int h = nLineHeight*(total_line_n+1);
  scrolled_panel->SetVirtualSize(w, h);  // in pixels
  SetVirtualSize(w, h);
  //SetScrollbars(nCharWidth, nLineHeight, virtual_width, total_line_n+1);
}

void editor_type::count_lines(void)
// Counts the lines, sets the virtual size.
{ wxChar * line;  UINT ntot, ncur;
  ntot=1;  ncur=0;  line=editbuf;
  do
  { if (cur_line_a > line) ncur++;
    unsigned len=lnlen(line);
    if (!line[len]) break;
    if (len>=virtual_width) virtual_width=len+1;
    line+=len;
    ntot++;
  } while (TRUE);
  total_line_n=ntot;
  cur_line_n=ncur;
  update_virtual_size();
  counter=0;
}

void editor_type::delete_line_delim(bool back)
// Supposes that the next line exists.
{ cur_char_n=txlnlen(cur_line_a);
  int cnt=0;
  if (cur_line_a[cur_char_n]=='\r') cnt=1;
  if (cur_line_a[cur_char_n+cnt]=='\n') cnt++;  /* CR LF deleted */
  if (cnt) del_chars(cur_line_a+cur_char_n, cnt, back);
 // update the total count of lines:
  total_line_n--;
  update_virtual_size();
}

void editor_type::undo_action(void)
{ undo_state=UDS_UNDOING;
  undo_buffer.do_top(this);
  undo_state=UDS_NORMAL;
  update_toolbar();
}
void editor_type::redo_action(void)
{ undo_state=UDS_REDOING;
  redo_buffer.do_top(this);
  undo_state=UDS_NORMAL;
  update_toolbar();
}

void t_action::undoit(editor_type * edd)
{ unsigned size;  wxChar * ptr;
  edd->set_position(edd->editbuf+offset);
  switch (action_type)
  { case ACT_TP_INS:
      edd->del_chars(edd->editbuf+offset, (unsigned)(size_t)data, TRUE);  
      break;
    case ACT_TP_REPL:
      edd->replace_char(short_data);
      break;
    case ACT_TP_BS1:
    case ACT_TP_DEL1:
      size=1;  ptr=&short_data;  goto inserting;
    case ACT_TP_BSn:
    case ACT_TP_DELn:
      size=(int)_tcslen(data);  ptr=data;
     inserting:
      edd->insert_chars(ptr, size);
      if (action_type==ACT_TP_BS1 || action_type==ACT_TP_BSn)
        edd->set_position(edd->editbuf+offset+size);
      break;
  }
  if (action_type==ACT_TP_INS || action_type==ACT_TP_BSn || action_type==ACT_TP_DELn)
    edd->count_lines();
  if (action_flags & ACTFL_WAS_UNCHANGED) 
    { edd->_changed=false;  edd->ch_status(false); }
  else edd->changed();
  edd->scrolled_panel->Refresh();
}

wxToolBarBase * editor_type::my_toolbar(void) const
{ wxToolBarBase * tb;
#if WBVERS>=1000
  if (global_style==ST_MDI)
    tb=designer_toolbar;
  else
    if (in_edcont)
      tb = (wxToolBar *)the_editor_container_frame->FindWindow(OWN_TOOLBAR_ID);
    else
      tb = (wxToolBar *)GetParent()->FindWindow(OWN_TOOLBAR_ID);
#else
  tb=designer_toolbar;
#endif
  return tb;
}

void editor_type::update_toolbar(void) const
{ wxToolBarBase * tb = my_toolbar();
  if (tb)
  { tb->EnableTool(TD_CPM_UNDO, can_undo());
    tb->EnableTool(TD_CPM_REDO, can_redo());
  }
}

void text_object_editor_type::update_toolbar(void) const
{ 
  editor_type::update_toolbar();
  wxToolBarBase * tb = my_toolbar();
  if (tb)
  { tb->EnableTool(TD_CPM_RUN,     category==CATEG_PROC && !the_server_debugger || 
                                   category==CATEG_CURSOR || (category==CATEG_PGMSRC && !(flags & CO_FLAGS_OBJECT_TYPE)));
    tb->EnableTool(TD_CPM_CHECK,  (category==CATEG_PROC || category==CATEG_TRIGGER) && !the_server_debugger || 
                                   category==CATEG_CURSOR || category==CATEG_PGMSRC /*&& (is_dad || !(flags & CO_FLAGS_OBJECT_TYPE))*/);
    tb->EnableTool(TD_CPM_PROFILE, category==CATEG_PROC);
    if (category==CATEG_PROC)
      tb->ToggleTool(TD_CPM_PROFILE, profiler_data!=NULL);
  }
}

void editor_type::set_position(wxChar * position)
{ 
 /* the line containing "position" will become the 2nd line in the
         editor window unless it is already on the screen or it is the 1st line
         of the document. */
 // find the line number for [position]:
  wxChar * ln = editbuf;  unsigned lnnum=0;
  do
  { wxChar * nx = next_line(ln);
    if (nx==ln || nx>position) break;
    ln=nx;  lnnum++;
  } while (true);
  int char_on_line = position-ln;
 // set the new current line:
  cur_line_a=ln;  cur_line_n=lnnum;  cur_char_n=char_on_line;
 // scroll (like scroll_to_cursor):
  int start_x_su, start_y_su, new_x=-1, new_y=-1;
  GetViewStart(&start_x_su, &start_y_su); 
  if (cur_line_n<start_y_su || cur_line_n>=start_y_su+win_ylen)
    new_y = cur_line_n ? cur_line_n-1 : 0;
  if (cur_char_n<start_x_su || cur_char_n>=start_x_su+win_xlen)
    new_x = cur_char_n ? cur_char_n-1 : 0;
  Scroll(new_x, new_y);
  move_caret();
}

void editor_type::position_on_line_column(unsigned line, unsigned column)   
// Sets the cursor position to the given line and column (lines counted from 1).
{ if (line<0) return;
  cur_line_n=0;
  if (line<=1) cur_line_a=editbuf;
  else
  { cur_line_a=editbuf;  
    line--;  /* 0-based now */
    while (line--)
    { wxChar * nline=next_line(cur_line_a);
      if (nline == cur_line_a) break;
      cur_line_a=nline;  cur_line_n++;
    }
  }
  cur_char_n=column;
  scroll_to_cursor();
}

void editor_type::position_on_offset(uns32 offset)
// Sets the cursor position to the given offset from the start of the text.
// Optimize it!
{ const wxChar * linestart, * p;  unsigned linenum;
  if (offset < txtsize)
  { p=editbuf;
    linestart=p;  linenum=1;
    while (offset--)
    { if (*p=='\n') { linestart=p+1;  linenum++; }
      p++;
    }
    position_on_line_column(linenum, p-linestart);
  }
}

///////////////////////////////////////// mouse & DnD support /////////////////////////////////////////////
void editor_type::get_mouse_position(wxMouseEvent & event, wxChar *& line_a, int & line_n, int & char_n)
// When mouse is below the last line, returns the last line.
// May return char_n after the end of the line.
{ wxCoord x, y;
  event.GetPosition(&x, &y);  // physical coordinates related to the window client area
  unsigned lineoff = y<0 ? 0 : y/ nLineHeight;
  unsigned charoff = x<0 ? 0 : (unsigned)((x+nCharWidth/4) / nCharWidth);
  int start_x_su, start_y_su;
  GetViewStart(&start_x_su, &start_y_su); 
  line_n = start_y_su+lineoff;
  char_n = start_x_su+charoff;
  do
  { line_a = line_num2ptr(line_n);
    if (line_a) break;
    line_n--;
  } while (true);
}

void scrolled_editor_panel::OnLeftButtonDown(wxMouseEvent & event)
{ my_editor->OnLeftButtonDown(event); }

void scrolled_editor_panel::OnLeftButtonUp(wxMouseEvent & event)
{ my_editor->OnLeftButtonUp(event); }

void scrolled_editor_panel::OnLeftButtonDclick(wxMouseEvent & event)
{ my_editor->OnLeftButtonDclick(event); }

void scrolled_editor_panel::OnRightButtonDown(wxMouseEvent & event)
{ my_editor->OnRightButtonDown(event); }

void scrolled_editor_panel::OnRightButtonUp(wxMouseEvent & event)
{ my_editor->OnRightButtonUp(event); }

void scrolled_editor_panel::OnMouseMotion(wxMouseEvent & event)
{ my_editor->OnMouseMotion(event); }

void scrolled_editor_panel::OnMouseWheel(wxMouseEvent & event)
{ my_editor->OnMouseWheel(event); }

#ifndef ADVTOOLTIP
void scrolled_editor_panel::OnTooltipTimer(wxTimerEvent & event)
{ my_editor->OnTooltipTimer(event); }

void editor_type::OnTooltipTimer(wxTimerEvent & event)
// Received when the mouse is idle for TOOPTIP_TIME
{ wxMouseEvent mevent;  wxChar * line_a;  int line_n, char_n;   int width, height;
  if (!the_server_debugger) return;
  scrolled_panel->GetClientSize(&width, &height);
  wxPoint pt = scrolled_panel->ScreenToClient(wxGetMousePosition());
  if (pt.x<0 || pt.y<0 || pt.x>=width || pt.y>=height || noedit!=2) 
    return;  // mouse is outside
  mevent.m_x=pt.x;  mevent.m_y=pt.y;
  get_mouse_position(mevent, line_a, line_n, char_n);
  int len = txlnlen(line_a);
  wxString expr;
  if (block_on && char_n<len && line_a+char_n>=block_start && line_a+char_n<block_end && block_end-block_start<300)  
  {// inside a block
    expr=wxString(block_start, block_end-block_start);
  }
  else
  { while (char_n && is_AlphanumW(line_a[char_n-1], cdp->sys_charset)) char_n--;
    unsigned sz=0;
    while (is_AlphanumW(line_a[char_n+sz], cdp->sys_charset)) sz++;
    expr=wxString(line_a+char_n, sz);
  }
  wxChar buf[MAX_FIXED_STRING_LENGTH+1];
  *buf=0;
  wxToolTip * tt = scrolled_panel->GetToolTip();
  if (expr.IsEmpty() || cd_Server_eval(cdp, 0, expr.c_str(), buf))
  { if (tt) tt->SetTip("");
    //wxGetApp().frame->SetStatusText("");
  }
  else
  { if (!tt) scrolled_panel->SetToolTip(buf);
    else tt->SetTip(buf);
    wxString msg;
    msg=expr+" = "+buf;
    //wxGetApp().frame->SetStatusText(msg);
  }
}
#endif

class MpsqlDropSource : public wxDropSource
{ editor_type * ed;
 public:
  MpsqlDropSource(editor_type * win) : wxDropSource(win) 
    { ed=win; }
  bool GiveFeedback(wxDragResult effect);
};

#define SCROLL_MARGIN 0

bool MpsqlDropSource::GiveFeedback(wxDragResult effect)
// Implements scrolling during the DnD. 
// Implementation inside WX does not support the situation when scrolling target is not the scroll window.
// Problem: No feedback on scrollbars of the scroll window. Resolved by creating a margin inside the window.
{ wxPoint pt = ed->scrolled_panel->ScreenToClient(wxGetMousePosition());
  int start_x_su, start_y_su;  int width, height;
  ed->GetViewStart(&start_x_su, &start_y_su); 
  ed->scrolled_panel->GetClientSize(&width, &height);
  if (pt.x<SCROLL_MARGIN && start_x_su>0)
    ed->Scroll(start_x_su-1, -1);
  else if (pt.x+2>width)
    ed->Scroll(start_x_su+1, -1);
  if (pt.y<SCROLL_MARGIN && start_y_su>0)
    ed->Scroll(-1, start_y_su-1);
  else if (pt.y+2>height)
    ed->Scroll(-1, start_y_su+1);
  return false;
}

void editor_type::OnLeftButtonDown(wxMouseEvent & event)
{ wxChar * line_a;  int line_n, char_n;
  get_mouse_position(event, line_a, line_n, char_n);
  int len = txlnlen(line_a);
  if (block_on && char_n<len && line_a+char_n>=block_start && line_a+char_n<block_end)  //  inside a block, start dragging
  { wxTextDataObject drag_data(wxString(block_start, block_end-block_start));
    MpsqlDropSource dragSource(this);
  	dragSource.SetData(drag_data);
    bool own_drop_result = true;
    local_drop_flag = &own_drop_result;
    wxDragResult result = dragSource.DoDragDrop(noedit ? /*wxDrag_CopyOnly*/ wxDrag_AllowMove : wxDrag_DefaultMove);
    if (result==wxDragMove && !noedit && own_drop_result) delete_block();
    local_drop_flag = NULL;
    // N.B.: DnD to Word works, to Wordpad freezes the source :-(
  }
  else // clicked outside a block
  { cur_line_a=line_a;  cur_line_n=line_n;  cur_char_n=char_n;
    block_on=false;
    move_caret();  // do not scroll here but in MouseUp event, otherwise block is selected when clicked near the right or bottom edge
    scrolled_panel->Refresh();  // hide the previous block
    if (char_n > len) char_n=len;  // must not select past the end of the line
    blockpos = line_a+char_n; // start of the block selection
    scrolled_panel->CaptureMouse();
  }
  event.Skip();
}

void editor_type::OnLeftButtonUp(wxMouseEvent & event)
{ wxChar * line_a;  int line_n, char_n;
  get_mouse_position(event, line_a, line_n, char_n);
  if (blockpos!=NULL)
  { int len = txlnlen(line_a);
    if (cur_line_a!=line_a || cur_char_n!=char_n) // a block selected (must compare before moving char_n before the end of the line)
    { if (char_n > len) char_n=len;  // must not select past the end of the line
      cur_line_a=line_a;  cur_line_n=line_n;  cur_char_n=char_n;  // move cursor to the end of the selection
      wxChar * ln = line_a+char_n;
      block_start=(ln<blockpos) ? ln : blockpos;
      block_end  =(ln<blockpos) ? blockpos : ln;
      block_on=true;
      scrolled_panel->Refresh();  // show the block
    }
    else block_on=false;
   // scroll so that the caret is visible:
    scroll_to_cursor();  // moves caret
    blockpos=NULL;  // end of selecting
    scrolled_panel->ReleaseMouse();
  }
  event.Skip();
}

void editor_type::OnLeftButtonDclick(wxMouseEvent & event)
// Selects the word. Caret is positioned by the preceeding click-events (unless a block is on).
{ 
 // when block_on, must position the cursor, not done in OnLeftButtonDown
  if (block_on)  
  { wxChar * line_a;  int line_n, char_n;
    get_mouse_position(event, line_a, line_n, char_n);
    cur_line_a=line_a;  cur_line_n=line_n;  cur_char_n=char_n;
    block_on=false;
    move_caret();  
    scrolled_panel->Refresh(); 
  }
 // find the word:
  unsigned start = cur_char_n;
  while (start && is_AlphanumW(cur_line_a[start-1], cdp->sys_charset)) start--;
  unsigned stop = start;
  while (is_AlphanumW(cur_line_a[stop], cdp->sys_charset)) stop++;
 // select the word, if found:
  if (stop > start)
  { block_start=cur_line_a+start;
    block_end  =cur_line_a+stop;
    block_on=true;
    refresh_current_line();
  }
}

void editor_type::OnRightButtonDown(wxMouseEvent & event)
{ wxChar * line_a;  int line_n, char_n;
  get_mouse_position(event, line_a, line_n, char_n);
  int len = txlnlen(line_a);
  if (block_on && char_n<len && line_a+char_n>=block_start && line_a+char_n<block_end)  
  { //  inside a block, no action
  }
  else // clicked outside a block, position the caret
  { cur_line_a=line_a;  cur_line_n=line_n;  cur_char_n=char_n;
    block_on=false;
    move_caret();  // do not scroll here
    scrolled_panel->Refresh();  // hide the previous block
  }
  context_menu(true);
  //event.Skip();  -- must not skip, may have been closed from the menu
}

void editor_type::OnRightButtonUp(wxMouseEvent & event)
// Caret has been positioned by the right-down event.
{ //kontext_menu(true);
  event.Skip();
}

void editor_type::OnMouseMotion(wxMouseEvent & event)
{ wxChar * line_a;  int line_n, char_n;
#ifndef ADVTOOLTIP
  //scrolled_panel->SetToolTip(NULL);         // prevents displaying the old tooltip value on a new place
  tooltip_timer.Start(TOOPTIP_TIME, true);  // one-shot, stops first if it is started
#endif
  get_mouse_position(event, line_a, line_n, char_n);
  if (blockpos!=NULL)
  {// scroll if positioned near the edge:
    wxCoord x, y;  int width, height;  int start_x_su, start_y_su;
    event.GetPosition(&x, &y);  // physical coordinates related to the window client area
   // returns negative values outside the client area, but sometimes returns coords relative to another window!!
    GetViewStart(&start_x_su, &start_y_su); 
    scrolled_panel->GetClientSize(&width, &height);
    if (x<SCROLL_MARGIN && start_x_su>0)
      Scroll(start_x_su-1, -1);
    else if (x+SCROLL_MARGIN>width)
      Scroll(start_x_su+1, -1);
    if (y<SCROLL_MARGIN && start_y_su>0)
      Scroll(-1, start_y_su-1);
    else if (y+SCROLL_MARGIN>height)
      Scroll(-1, start_y_su+1);
   // change the block selection:
    if (cur_line_a!=line_a || cur_char_n!=char_n) // a block selected
    { //cur_line_a=line_a;  cur_line_n=line_n;  cur_char_n=char_n;  // move cursor to the end of the selection
      //scroll_to_cursor();  // moves caret, scrolling is not very good for block selection, but necessary when clicked near the edge
      int len = txlnlen(line_a);
      wxChar * ln = line_a + (char_n<len ? char_n:len);
      block_start=(ln<blockpos) ? ln : blockpos;
      block_end  =(ln<blockpos) ? blockpos : ln;
      block_on=true;
      scrolled_panel->Refresh();  // show the block
    }
    else block_on=false;
  }
  event.Skip();
}

void editor_type::OnMouseWheel(wxMouseEvent & event)
{ int delta = event.GetWheelDelta();
  int start_x_su, start_y_su, lines;
  GetViewStart(&start_x_su, &start_y_su);
  cumulative_wheel_delta += event.GetWheelRotation();
  int lines_unit = event.IsPageScroll() ? win_ylen : event.GetLinesPerAction();
  int new_start_t;
  if (cumulative_wheel_delta>0)
  { lines = lines_unit * (cumulative_wheel_delta/delta);
    cumulative_wheel_delta %= delta;
    new_start_t=start_y_su>lines ? start_y_su-lines : 0;
  }
  else
  { lines = lines_unit * ((-cumulative_wheel_delta)/delta);
    cumulative_wheel_delta = -((-cumulative_wheel_delta) % delta);
    new_start_t=start_y_su+lines;
  }
 // must hide, show and update the caret position, otherwise caret sometimes stops blinking, appears in multiple positions:
  wxCaret * caret = scrolled_panel->GetCaret();
  if (caret) caret->Hide();
  Scroll(-1, new_start_t);
  if (caret) caret->Show();
  if (caret)
    caret->Move((int)((cur_char_n-start_x_su)*nCharWidth), (cur_line_n-new_start_t)*nLineHeight);
  //event.Skip();  -- some scrolling is automatical
}

bool EditorTextDropTarget::OnDropText(wxCoord x, wxCoord y, const wxString & data)
{ return ed->OnDropText(x, y, data); }

bool editor_type::OnDropText(long x, long y, const wxString & data)
{ if (noedit) return false;
 // find the drop position:
  unsigned lineoff = y<0 ? 0 : y/ nLineHeight;
  unsigned charoff = x<0 ? 0 : (unsigned)((x+nCharWidth/4) / nCharWidth);
  int start_x_su, start_y_su;
  GetViewStart(&start_x_su, &start_y_su); 
  int line_n = start_y_su+lineoff;
  int char_n = start_x_su+charoff;
  wxChar * line_a = line_num2ptr(line_n);
  if (!line_a) return false;  // rejected
 // check if the drop position is inside the block - must convert the positions after the end of the line to the end of the line
  int norm_char_pos = txlnlen(line_a);
  if (char_n<norm_char_pos) norm_char_pos=char_n;
  if (block_on && line_a+norm_char_pos>=block_start && line_a+norm_char_pos<block_end)  
  { if (local_drop_flag) *local_drop_flag=false;  // patching error in GTK WX
    wxBell();
    return false;  // rejected, problems when dropping from itself
  }  
 // set there the current position and insert:
  cur_line_a=line_a;  cur_line_n=line_n;  cur_char_n=char_n;
  move_caret();
  insert_chars(data.c_str(), (int)data.Length());  // may insert after the end of the line
  count_lines();
  scrolled_panel->Refresh();
  return true;  // data accepted
}
/////////////////////////////////////// context menu //////////////////////////////////////////////////////////
#define WORD_LENGTH_LIMIT 200

wxString editor_type::read_current_word(void)
{ const wxChar * pos;  int len;
  int i=txlnlen(cur_line_a);
  if (i > cur_char_n) i=cur_char_n;
  if (block_on && cur_line_a+i >= block_start && cur_line_a+i <= block_end && block_end>block_start)
  { pos=block_start;
    while (*pos=='\r' || *pos=='\n' || *pos==' ') pos++;
    len=0;
    while (pos[len]!='\r' && pos[len]!='\n' && pos[len]!=0 && len<WORD_LENGTH_LIMIT && pos+len<block_end)
      len++;
    wxString str(pos, len);
    str.Trim(true);  // trim from right
    return str;
  }
  else
  { while (i && is_AlphanumW(cur_line_a[i-1], cdp->sys_charset)) i--;
    pos=cur_line_a+i;
    len=0;
    while (len<WORD_LENGTH_LIMIT && is_AlphanumW(pos[len], cdp->sys_charset)) len++;
    wxString str(pos, len);
    return str;
  }
}

void editor_type::OnHelp(void)
{ wxString word = read_current_word();
  if (word.IsEmpty()) wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/texteditor.html"));
  else wxGetApp().frame->help_ctrl.KeywordSearch(word);
}

void editor_type::context_menu(bool by_mouse)
{ wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(TD_CPM_SAVE, _("&Save\tCtrl+S"));
  popup_menu->Enable(TD_CPM_SAVE, _changed && !noedit);
  popup_menu->Append(TD_CPM_EXIT, _("C&lose\tCtrl+W"));
  popup_menu->Append(TD_CPM_HELP, _("&Help\tF1"));
  popup_menu->AppendSeparator();
  popup_menu->Append(TD_CPM_CUT, _("Cu&t\tCtrl+X"));
  popup_menu->Enable(TD_CPM_CUT, block_on && block_end>block_start && !noedit);
  popup_menu->Append(TD_CPM_COPY, _("&Copy\tCtrl+C"));
  popup_menu->Enable(TD_CPM_COPY, block_on && block_end>block_start);
  popup_menu->Append(TD_CPM_PASTE, _("&Paste\tCtrl+V"));
  if (noedit)
    popup_menu->Enable(TD_CPM_PASTE, false);
  else
    if (wxTheClipboard->Open())
    { if (!wxTheClipboard->IsSupported(wxDF_TEXT)) popup_menu->Enable(TD_CPM_PASTE, false);
      wxTheClipboard->Close();
    }

  popup_menu->Append(TD_CPM_TOGGLE_BOOKMARK, _("Toggle &bookmark\tCtrl+F12"));

  wxString str, word = read_current_word();
  if (word.Length() > OBJ_NAME_LEN) word=word.Left(OBJ_NAME_LEN-3)+wxT("...");
  if (!word.IsEmpty())
  { popup_menu->AppendSeparator();
    str.Printf(_("&Find \"%s\" in this document\tCtrl+F"), word.c_str());
    popup_menu->Append(TD_CPM_FIND, str);
    str.Printf(_("F&ind \"%s\" in aplication objects"), word.c_str());
    popup_menu->Append(TD_CPM_FIND_ALL, str);
    str.Printf(_("&Open editor on object \"%s\""), word.c_str());
    popup_menu->Append(TD_CPM_OPEN, str);
    popup_menu->Enable(TD_CPM_OPEN, word.Length() <= OBJ_NAME_LEN);
  }
  if (content_category()==CONTCAT_SQL)
  { popup_menu->AppendSeparator();
    popup_menu->Append(TD_CPM_TOGGLE_BREAKPOINT, _("Toggle b&reakpoint"));
    if (the_server_debugger && the_server_debugger->current_state==DBGST_BREAKED && !word.IsEmpty())
    { str.Printf(_("&Evaluate \"%s\""), word.c_str());
      popup_menu->Append(TD_CPM_EVAL, str);
      str.Printf(_("&Watch \"%s\""), word.c_str());
      popup_menu->Append(TD_CPM_WATCH, str);
    }
  }
 // run the menu: 
  wxWindow * processor = wxGetApp().frame;  // not [this], crashes on GTK when editor is closed in the editor container
  wxPoint pt;
  if (by_mouse) pt = processor->ScreenToClient(wxGetMousePosition());
  else
  { int start_x_su, start_y_su;
    GetViewStart(&start_x_su, &start_y_su);
    pt.x=(int)((cur_char_n-start_x_su)*nCharWidth);
    pt.y=      (cur_line_n-start_y_su)*nLineHeight;
    pt = ClientToScreen(pt);
    pt = processor->ScreenToClient(pt);
  }
  processor->PopupMenu(popup_menu, pt);
  delete popup_menu;
}

/////////////////////////////////////// find and replace //////////////////////////////////////
/* Na MSW pouziva MSW std dialog a subclasuje parenta, aby dostal jeho zpravy. Obcas se dostane do nekonecne smycky.
MSW std. dialog je omezeny - nema historii stringu ani replace v selection atd.
Proto pouziji vlastni dlalog.

The find history and is common for the find and Replace dialogs.
When an editor is closed or deactivated then its modeless Find or Replace dialog is destroyed.
All instances of these dialogs share the history.
When the Find or Replace dialog is called and the other dialog type already exists, it is destroyed.
Find in selection in not supported. The only operation in a selection is ReplaceAll.
*/

void editor_type::OnFindReplace(bool replace)
{ wxString word;  bool multiline_selection = false;
  if (block_on)
  { wxChar * p = block_start;
    while (p<block_end)
    { if (*p=='\n') { multiline_selection=true;  break; }
      p++;
    }
  }
  if (!multiline_selection)
    word = read_current_word();
  if (replace)
  { if (pending_find_dialog) pending_find_dialog->Destroy();
    if (pending_replace_dialog) pending_replace_dialog->Raise();
    else 
    { pending_replace_dialog = new ReplaceDlg(this, word, block_on && block_start<block_end, multiline_selection);
      pending_replace_dialog->Create(dialog_parent());
      pending_replace_dialog->Show();
      pending_replace_dialog->m_textFind->SetFocus();
    }
  }
  else  // find
  { if (pending_replace_dialog) pending_replace_dialog->Destroy();
    if (pending_find_dialog) pending_find_dialog->Raise();
    else 
    { pending_find_dialog = new FindDlg(this, word);
      pending_find_dialog->Create(dialog_parent());
      pending_find_dialog->Show();
      pending_find_dialog->m_textFind->SetFocus();
    }
  }
}

void editor_type::OnFindReplaceAction(wxFindDialogEvent & event)
// Selection is ignored (and unually changed) except for wxEVT_COMMAND_FIND_REPLACE_ALL
{ int find_flags = 0;  
  if (event.GetEventType()==wxEVT_COMMAND_FIND_REPLACE)
  { // if positioned on the find string, replace:
    if (block_on && event.GetFindString().Length()==block_end-block_start)
    { seq_delete_block();
      insert_chars(event.GetReplaceString().c_str(), (int)event.GetReplaceString().Length());
    }
    // continue by finding the next occurrence:
    event.SetEventType(wxEVT_COMMAND_FIND_NEXT);
  }
  if (event.GetEventType()==wxEVT_COMMAND_FIND_NEXT || event.GetEventType()==wxEVT_COMMAND_FIND || event.GetEventType()==wxEVT_COMMAND_FIND_REPLACE_ALL)
  {// get the options:
    last_search_flags = event.GetFlags();
    if (!(last_search_flags & wxFR_DOWN))   find_flags |= SEARCH_BACKWARDS;
    if (last_search_flags & wxFR_MATCHCASE) find_flags |= SEARCH_CASE_SENSITIVE;
    if (last_search_flags & wxFR_WHOLEWORD) find_flags |= SEARCH_WHOLE_WORDS;
    last_search_string = event.GetFindString();
    if (!(last_search_flags & wxFR_MATCHCASE))
      WXUpcase(cdp, last_search_string);
    wxChar * start, * stop;  uns32 respos;   int find_len;
    find_len = (int)last_search_string.Length();
    wxGetApp().frame->SetStatusText(wxEmptyString);
    if (event.GetEventType()==wxEVT_COMMAND_FIND_REPLACE_ALL)
    { wxChar * start, * stop;
      int res_len = (int)event.GetReplaceString().Length();
      if (last_search_flags & FR_INSELECTION)
        { start=block_start;  stop=block_end; }
      else
        { start=editbuf;  stop=editbuf+txtsize; }
      while (start<stop && Search_in_blockT(start, stop-start, NULL, last_search_string.c_str(), find_len, find_flags, cdp->sys_charset, &respos))
      { position_on_offset(start+respos-editbuf);  // cannot insert chars without positioning
        del_chars(start+respos, find_len, false);
        add_seq_flag();
        insert_chars(event.GetReplaceString().c_str(), res_len);
        start+=respos+res_len;
        stop-=find_len;  stop+=res_len;
      }
      position_on_offset(start-editbuf);
      scrolled_panel->Refresh();  // position_on_offset may call it but we need this to be called unconditionally because the block has been changed
    }
    else
    {// get current pos:
      wxChar * pos;   
      if (cur_char_n > txlnlen(cur_line_a))
        pos=cur_line_a+txlnlen(cur_line_a);
      else
        pos=cur_line_a+cur_char_n;
     // start and stop:
      if (last_search_flags & wxFR_DOWN)
        { start=pos;  stop=editbuf+txtsize; }
      else
        { start=editbuf;  stop=pos; }
     // find:
      bool found=false;
      if (start<stop && Search_in_blockT(start, stop-start, NULL,
                  last_search_string.c_str(), find_len, find_flags, cdp->sys_charset, &respos))
        found=true;
      else // continuing the search in the other part of the text
      { wxGetApp().frame->SetStatusText(_("Passed the end of the text."));  wxBell();
        if (last_search_flags & wxFR_DOWN)
          { start=editbuf;  stop=pos; }
        else
          { start=pos;  stop=editbuf+txtsize; }
        if (start<stop && Search_in_blockT(start, stop-start, NULL,
                  last_search_string.c_str(), find_len, find_flags, cdp->sys_charset, &respos))
          found=true;
        else   // 'Not found' message
        { wxString msg;
          msg=_("String '")+event.GetFindString()+_("' not found.");
          info_box(msg.c_str(), dialog_parent());
        }
      }
      if (found)
      { block_start=start+respos;  block_end=start+respos+find_len;  block_on=true; 
       // set current position so that the continuation will not find the current occurrence:
        if (last_search_flags & wxFR_DOWN)
        { position_on_offset(start+respos         -editbuf);  // trying to make visible the whole found text...
          position_on_offset(start+respos+find_len-editbuf);  // ...but then positioning on the end
        }
        else
          position_on_offset(start+respos-editbuf);
        scrolled_panel->Refresh();  // position_on_offset may call it but we need this to be called unconditionally because the block has been changed
      }
    }
  }
}

#if 0 /////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <commctrl.h>
#include <commdlg.h>
#include "replic.h"

#include "wbviewed.h"
#define EDCONT_WINDOW_LONG 123  // different from valid pointers

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#define WHEEL_DELTA                     120     /* Value for rolling one detent */
#endif /* if (_WIN32_WINNT < 0x0400) */

extern "C" {
DllPrezen void WINAPI link_forwards(objtable * id_tab);
}
static LRESULT CALLBACK FsedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK EdContWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void watch_recalc(cdp_t cdp);
static void update_stack(cdp_t cdp, HWND hDlg);
/* Turbo editing mode vs. Windows mode:
  paste, typing, RETURN (but not TAB): deletes block before action
  DELETE, Backspace: deletes block if marked instead of deleting character
  move cursor: hides block in Windows mode
  mouse click anywhere: hides block in Windows mode
*/
#define OW_PGRUN     13
#define OW_PREZ      10
static char FsedClassName[]   = "WBFullScreenEditor32";
static char EdContClassName[] = "WBEditorContainer32";
#define HARD_RET_MARK "\xb6"
static int code_text=0;
char last_path[MAX_PATH]="";  /* public */
static HMENU ed_prog_menu=0, ed_text_menu=0, ed_object_menu=0, debug_menu=0, srvdbg_menu=0;
static HCURSOR drag_block_cursor=0;
static wbbool dragging_block = wbfalse;
static RECT edcont_rect = { -1, -1, -1, -1 }, edit_rect = { -1, -1, -1, -1 }, popup_rect = { -1, -1, -1, -1 };

extern char EMPTYMENU[10];
#define MAX_FIND_STRING  200
#define MAX_FIND_HISTORY 160
static wxChar eval_history[MAX_FIND_HISTORY]="\x1";

// hodnoty parametru lParam funkce Fsed_COMMAND() pro command MI_FSED_COMPLETE_WORD
#define MICW_SEARCH_BACKWARD  1
#define MICW_SEARCH_FORWARD 2

#if 0
static t_tb_descr toolbar_ed_prog[] = {
{sizeof(t_tb_descr),TBFLAG_RIGHT,   TBB_HELP,      SZM_HELP,        3,   "" },
{sizeof(t_tb_descr),0,              TBB_CUT,       WM_CUT,          3,   "" },
{sizeof(t_tb_descr),0,              TBB_COPY,      WM_COPY,         25,  "" },
{sizeof(t_tb_descr),0,              TBB_PASTE,     WM_PASTE,        47,  "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_UNDO,      MI_FSED_UNDO   , 69,  "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REDO,      MI_FSED_REDO   , 91,  "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_SAVE,      MI_FSED_SAVE   , 127, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_VECLOSE,   MI_FSED_CLOSE1 , 155, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_PRINT,     MI_FSED_PRINT  , 183, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_FIND,      MI_FSED_FIND   , 219, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REPLACE,   MI_FSED_REPLACE, 241, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REFIND,    MI_FSED_REFIND , 263, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_COMPILE,   MI_FSED_COMPILE, 299, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_RUN,       MI_FSED_RUN    , 321, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_PR_COMP,   MI_FSED_PR_COMP, 357, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_PR_RUN,    MI_FSED_PR_RUN , 379, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_DEBUG,     MI_FSED_DEBUG  , 415, "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM,TBB_BREAK,    MI_DBG_TOGGLE,   437, "" },
{sizeof(t_tb_descr),TBFLAG_STOP, 0, 0, 0, "" } };

#define TBPOS_DBGMODE   16
#define TBPOS_UNDO       4
#define TBPOS_REDO       5
#define TBPOS_RUN       13

static t_tb_descr toolbar_ed_obj[]  = {
{sizeof(t_tb_descr),TBFLAG_RIGHT,   TBB_HELP,      SZM_HELP,        3,   "" },
{sizeof(t_tb_descr),0,              TBB_CUT,       WM_CUT,          3,   "" },
{sizeof(t_tb_descr),0,              TBB_COPY,      WM_COPY,         25,  "" },
{sizeof(t_tb_descr),0,              TBB_PASTE,     WM_PASTE,        47,  "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_UNDO,      MI_FSED_UNDO   , 69,  "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REDO,      MI_FSED_REDO   , 91,  "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_SAVE,      MI_FSED_SAVE   , 127, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_VECLOSE,   MI_FSED_CLOSE1 , 155, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_PRINT,     MI_FSED_PRINT  , 183, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_FIND,      MI_FSED_FIND   , 219, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REPLACE,   MI_FSED_REPLACE, 241, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REFIND,    MI_FSED_REFIND , 263, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_COMPILE,   MI_FSED_COMPILE, 299, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_RUNOBJ,    MI_FSED_RUN    , 321, "" },

{sizeof(t_tb_descr),TBFLAG_FRAMECOMM,TBB_DEBUG,    MI_FSED_DEBUG,   357, "" },
{sizeof(t_tb_descr),TBFLAG_STOP, 0, 0, 0, "" } };

t_tb_descr toolbar_ed_text[] = {
{sizeof(t_tb_descr),TBFLAG_RIGHT,   TBB_HELP,      SZM_HELP,        3,   "" },
{sizeof(t_tb_descr),0,              TBB_CUT,       WM_CUT,          3,   "" },
{sizeof(t_tb_descr),0,              TBB_COPY,      WM_COPY,         25,  "" },
{sizeof(t_tb_descr),0,              TBB_PASTE,     WM_PASTE,        47,  "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_UNDO,      MI_FSED_UNDO   , 69,  "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REDO,      MI_FSED_REDO   , 91,  "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_SAVE,      MI_FSED_SAVE   , 127, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_VECLOSE,   MI_FSED_CLOSE1 , 155, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_PRINT,     MI_FSED_PRINT  , 183, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_FIND,      MI_FSED_FIND   , 219, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REPLACE,   MI_FSED_REPLACE, 241, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_REFIND,    MI_FSED_REFIND , 263, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND, TBB_FORMAT,    MI_FSED_FORMAT,  299, "" },
{sizeof(t_tb_descr),TBFLAG_STOP, 0, 0, 0, "" } };

static t_tb_descr toolbar_debug[] = {
{sizeof(t_tb_descr),TBFLAG_RIGHT,     TBB_HELP,      SZM_HELP,        3,   "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_RUN,       MI_DBG_RUN,      25,  "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_STEP,      MI_DBG_STEP,     47,  "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_TRACE,     MI_DBG_TRACE,    69,  "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_TORET,     MI_DBG_TORET,    91,  "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_GOCURS,    MI_DBG_GOTO,     113,  "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_BREAK,     MI_DBG_TOGGLE,   135,  "" },

{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_EVAL,      MI_DBG_EVAL,     163, "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_WATCH,     MI_DBG_ADDWATCH, 185, "" },

{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_WATCHES,   MI_DBG_WATCHES,  213, "" },
{sizeof(t_tb_descr),TBFLAG_FRAMECOMM, TBB_CALLSTK,   MI_DBG_CALLS,    235, "" },

{sizeof(t_tb_descr),TBFLAG_COMMAND,   TBB_FIND,      MI_FSED_FIND   , 275, "" },
{sizeof(t_tb_descr),TBFLAG_COMMAND,   TBB_REFIND,    MI_FSED_REFIND , 297, "" },
{sizeof(t_tb_descr),TBFLAG_STOP, 0, 0, 0, "" } };
#endif  // WX

#define CALL_STACK_POS 10

#define PST_AXEHCOMP 1

//static unsigned yesno_buttons[2] =
//{ BTEXT_YES | ESCAPE_BUTTON | SELECTED_BUTTON,  BTEXT_NO | LAST_BUTTON };
#if 0
static BOOL general_record_locking(view_dyn * inst, tcursnum cursnum, trecnum erec, BOOL read_lock, BOOL unlock)
{ BOOL res;
  if (erec==NORECNUM) return FALSE;  /* fictive rec, nothing to be done */
  if (cursnum==inst->dt->cursnum) return view_record_locking(inst, erec, read_lock, unlock);
 // editing text linked via pointer, not from the view cache:
  if (IS_ODBC_TABLE(cursnum)) return FALSE;  // impossible, nothing can be done
  if (unlock)
    if (read_lock) res=cd_Read_unlock_record (inst->cdp, cursnum, erec);
    else           res=cd_Write_unlock_record(inst->cdp, cursnum, erec);
  else
    if (read_lock) res=cd_Read_lock_record   (inst->cdp, cursnum, erec);
    else           res=cd_Write_lock_record  (inst->cdp, cursnum, erec);
  if (!unlock && res) stat_signalize(inst->cdp);
  return res;
}
#endif

void editor_type::clear_completeword_params()
{
  *cw_searchword='\0'; cw_start=cw_stop=NULL;
  cw_already_found_words.reset();
}
int editor_type::add_word_to_found_words(const wxChar *w)
{
  if( w==NULL ) return 0;
  size_t wlen=strlen(w);
  if( wlen==0 ) return 0;
  const wxChar *found;
  wxChar *where=cw_already_found_words.get();
  while( (found=strstr(where,w))!=NULL )
  {
    if( (found==where||found[-1]==' ')&&found[wlen]==' ' ) return -1; // slovo tam skutecne je, je obklopeno z obou stran mezerou
    where=strchr(found,' '); // posunout se k dalsimu zaznamenanemu slovu
    // a hledat znovu
  }
  cw_already_found_words.put(w);
  cw_already_found_words.putc(' ');
  return 1;
}

int editor_type::posit2line(wxChar * pos)
{ wxChar * p = editbuf;  int line=0;
  while (*p && p<pos)
  { if (*p=='\n') line++;
    p++;
  }
  return line;
}

void t_linenum_list::set_bps(editor_type * edd)  // sets all stored breakpoints
{ if (edd->cursnum!=OBJ_TABLENUM) return;  /* impossible, I think */
  if (edd->categ==CATEG_PGMSRC && !edd->is_dad)
  { uns8 filenum=find_file_num(edd->cdp, (tobjnum)edd->objnum);
    if (filenum==0xff) return;   /* this program is not debugged */
    for (int i=0;  i<list.count();  i++)
    { t_posit aline = *list.acc0(i);
      if (aline!=NO_LINENUM)
        if (!toggle_bp(edd->cdp, filenum, edd->posit2line(aline)+1))
            { MessageBeep(-1);  Sleep(100); }
    }
  }
  else if (edd->categ==CATEG_PROC || edd->categ==CATEG_TRIGGER)
    for (int i=0;  i<list.count();  i++)
    { t_posit * paline = list.acc0(i);  
      if (*paline!=NO_LINENUM)
      { int posin=edd->posit2line(*paline)+1, posout;
        if (cd_Server_debug(edd->cdp, DBGOP_ADD_BP, edd->objnum, posin, NULL, NULL, &posout))
          { MessageBeep(-1);  Sleep(100); }
        if (posin<posout)
        { while (posin<posout) { *paline=edd->next_line(*paline);  posin++; }
          InvalidateRect(edd->edwnd, NULL, TRUE); 
        }
      }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
class t_edcont
{ void activated(int index);
 public:
  HWND hEdCont, hTab, hDispl, hTT; // hTT is the copy of the hTT in editor_type off all open editors
  t_edcont(void)
  { hEdCont=0; }
  LRESULT EdContWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void activate(int index);
  HWND attach(editor_type * med, int x, int y, int cx, int cy);
  void detach_current(void);
};

t_edcont the_edcont;

#ifdef STOP
BOOL RegisterFsed(HINSTANCE hInst)
{ WNDCLASS FsedWndClass;
 /* loading all the editor menus */
  ed_text_menu  =LoadMenu(hPrezInst, MAKEINTRESOURCE(MENU_FSED_TEXT));
  ed_prog_menu  =LoadMenu(hPrezInst, MAKEINTRESOURCE(MENU_FSED_PROG));
  ed_object_menu=LoadMenu(hPrezInst, MAKEINTRESOURCE(MENU_FSED_OBJECT));
  debug_menu    =LoadMenu(hPrezInst, MAKEINTRESOURCE(MENU_DEBUG));
  srvdbg_menu   =LoadMenu(hPrezInst, MAKEINTRESOURCE(MENU_FSED_SRVDBG));
  drag_block_cursor=(HCURSOR)LoadImage(hPrezInst, MAKEINTRESOURCE(DRAG_BLOCK_CURSOR), 
    IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
 /* registering fsed class */
  FsedWndClass.lpszClassName=FsedClassName;
  FsedWndClass.hInstance=hInst;
  FsedWndClass.lpfnWndProc=FsedWndProc;
  FsedWndClass.style=CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS|CS_DBLCLKS;
  FsedWndClass.lpszMenuName=NULL;
  FsedWndClass.hIcon=LoadIcon(hInst, MAKEINTRESOURCE(EDITICON));
  FsedWndClass.hCursor=LoadCursor(0, IDC_ARROW);
  FsedWndClass.hbrBackground=(HBRUSH)GetStockObject(HOLLOW_BRUSH);
  FsedWndClass.cbClsExtra=0;
  FsedWndClass.cbWndExtra=sizeof(void*);
  if (!RegisterClass(&FsedWndClass)) return FALSE;
 // editor container:
  FsedWndClass.lpszClassName=EdContClassName;
  FsedWndClass.lpfnWndProc=EdContWndProc;
  return RegisterClass(&FsedWndClass) != 0;
}

void UnregisterFsed(void)
{ WNDCLASS WndClass;
  DestroyMenu(ed_text_menu);
  DestroyMenu(ed_prog_menu);
  DestroyMenu(ed_object_menu);
  DestroyMenu(debug_menu);
  DestroyMenu(srvdbg_menu);
  DestroyCursor(drag_block_cursor);
  if (GetClassInfo(hPrezInst, FsedClassName, &WndClass)) DestroyIcon(WndClass.hIcon);
  UnregisterClass(FsedClassName, hPrezInst);
}
#endif

//static editor_type * editor;               /* for strictly local use! */
static editor_type * created_editor;       /* used when openning the window only */
static HWND hWatches = 0, hCalls = 0;

static unsigned txlnlen(const wxChar * ln);
/**************************** find & replace ********************************/
#define H_DELIM 1  /* history delimiter is a char with code 1 */
static wxChar     find_history[MAX_FIND_HISTORY]="\x1";
static wxChar     repl_history[MAX_FIND_HISTORY]="\x1";
#define SEARCH_BLOCKED 0x2000
static wxChar   str_find[MAX_FIND_STRING+1] = "";
static wxChar   str_repl[MAX_FIND_STRING+1] = "";
static unsigned  find_flags = SEARCH_BLOCKED | SEARCH_FROM_CURSOR;

BOOL editor_type::read_current_word_to_cursor(wxChar * keyword, unsigned bufsize)
{ wxChar * pos=cur_line_a;
  unsigned sz, i=txlnlen(pos);
  if (i > cur_char_n) i=cur_char_n;
  while (i && is_AlphanumW(pos[i-1], cdp->sys_charset)) i--;
  sz=0;
  while (sz<bufsize-1 && is_AlphanumW(pos[i], cdp->sys_charset) && i<cur_char_n)
    keyword[sz++]=pos[i++];
  keyword[sz]=0;
  return sz>0;
}

BOOL editor_type::read_word_at_pos(wxChar * text_start,wxChar * pos,wxChar * keyword, unsigned bufsize)
{ 
  // nejdrive najdeme zacatek slova, ktere je na pozici pos v textu, ktery zacina na text_start
  while( pos>text_start && is_AlphanumW(pos[-1], cdp->sys_charset) ) pos--;
  unsigned sz=0, i=0;
  // pak najdeme konec slova a zaroven jej zkopirujeme do keyword
  while (sz<bufsize-1 && is_AlphanumW(pos[i], cdp->sys_charset))
    keyword[sz++]=pos[i++];
  // pridame koncovou nulu
  keyword[sz]=0;
  return sz>0;
}

/********************************* printing *********************************/

static BOOL gen_ed_page(editor_type * edd, HDC hPrnDC, char ** start, char * stop, unsigned xPext, unsigned yPext)
{ TEXTMETRIC tm;  UINT fulllen, linedist, pagespaceleft;  BOOL fullline;
  char * lstart = *start, * nx;  SIZE sz;
  GetTextMetrics(hPrnDC, &tm);
  linedist=tm.tmHeight/*+tm.tmExternalLeading*/;
  pagespaceleft=yPext;
  while ((pagespaceleft >= linedist) && (lstart < stop))
  { fulllen=txlnlen(lstart);
    if (lstart+fulllen > stop)
      { fulllen = (unsigned)(stop-lstart);  fullline=FALSE; }
    else fullline=TRUE;
    while (fulllen>1)
    { GetTextExtentPoint(hPrnDC, lstart, fulllen, &sz);
      if (sz.cx <= xPext) break;
      fulllen--;  fullline=FALSE;
    }
    TextOut(hPrnDC, 0, yPext-pagespaceleft, lstart, fulllen);
    if (fullline)
    { nx=edd->next_line(lstart);
      if ((nx==lstart) || (nx >= stop)) return FALSE;
      lstart=nx;
    }
         else
    { lstart=lstart+fulllen;
    }
    pagespaceleft -= linedist;
  }
  *start=lstart;   /* start of the next line */
  return lstart < stop;
}

#if 0
CFNC DllPrezen HFONT WINAPI create_log_font(LOGFONT * lf, HDC hDC, HWND hWnd)
/* updates the font hight according to hDC, if hDC not specified, retrieves it from hWnd */
{ BOOL release;  HFONT hFont;
  if (!hDC) { hDC=GetDC(hWnd);  release=TRUE; }
  else release=FALSE;
  int ykoef=GetDeviceCaps(hDC, LOGPIXELSY);
  if (release) ReleaseDC(hWnd, hDC);
  int old_height=lf->lfHeight;
  lf->lfHeight=-(int)(((long)lf->lfHeight * ykoef + 36) / 72);
  hFont=CreateFontIndirect(lf);
  lf->lfHeight=old_height;
  return hFont;
}

UINT APIENTRY FontSelHook(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
/* Hides the "Text Color" combo and its caption. */
{ if (msg==WM_INITDIALOG)
  { ShowWindow(GetDlgItem(hDlg, 0x443), SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x473), SW_HIDE);
  }
  return FALSE;
}

CFNC DllPrezen BOOL WINAPI font_dialog(HWND hOwner, LOGFONT * lf, uns32 cfFlags)
/* Template se pouziva pouze proto, ze nelze odstranit color bez underline a strikeout.
   V template vsak chybi script pro Win95. */
{ CHOOSEFONT cf;  HDC hDC;  BOOL res, DC_created;
 /* convert font size from point to pixel: */
  hDC=GetDC(hOwner);
  int logpix=GetDeviceCaps(hDC, LOGPIXELSY);
  ReleaseDC(hOwner, hDC);
  int oldHeight=lf->lfHeight;
  lf->lfHeight=-(int)(((long)lf->lfHeight * logpix + 36) / 72);
 /* fill the CHOOSEFONT parameters: */
  memset(&cf, 0, sizeof(CHOOSEFONT));
  cf.lStructSize = sizeof(CHOOSEFONT);
  cf.hwndOwner = hOwner;
  cf.hDC=hDC=GetPrinterDC(NULL, PRI_DIR_PRINTER, &DC_created);
  if (!hDC) cfFlags &= ~(LONG)CF_PRINTERFONTS;
  cf.lpLogFont = lf;
  cf.Flags = cfFlags | CF_INITTOLOGFONTSTRUCT;
  cf.lpfnHook=FontSelHook;
  cf.Flags |= CF_ENABLEHOOK;
  cf.hInstance=hPrezInst;
  res=ChooseFont(&cf);
  lf->lfHeight = res ? (cf.iPointSize+4)/10 : oldHeight; /* convert to point size back */
  if (hDC)
    if (DC_created) DeleteDC(hDC);  else ReleaseDC(NULL, hDC);
  return res;
}

void print_editor_text(editor_type * edd, char * start, char * stop)
{ HDC hPrnDC;  char print_msg[40];
  BOOL is_next_page;  HFONT hFont;
  int xPext, yPext;

  hPrnDC = GetPrinterDC(NULL, PRI_DIR_PRINTER, NULL);
  if (!hPrnDC) { errbox(MAKEINTRESOURCE(NO_PRINT_SEL));  return; }
  xPext=GetDeviceCaps(hPrnDC, HORZRES);
  yPext=GetDeviceCaps(hPrnDC, VERTRES);
  hFont=create_log_font(&GetPP()->editor_screen_font, hPrnDC, 0);
  if (hFont)
  { LoadString(hPrezInst, EDPRINT_LABEL, print_msg, sizeof(print_msg));
    DOCINFO DocInfo = {sizeof(DOCINFO), print_msg, NULL};
    SetAbortProc(hPrnDC, AbortProc);
    if (StartDoc(hPrnDC, &DocInfo) != SP_ERROR)
    { hPrintDlg=CreateDialog(hPrezInst, MAKEINTRESOURCE(DLG_PRINT),
                                     GetFocus(), PrintingDlgProc);
      UINT current_page_number=0;
      print_cancel_request=wbfalse;
      while (!print_cancel_request)
      {/* new page number */
        SetDlgItemInt(hPrintDlg, PRINT_PAGE, ++current_page_number, FALSE);
        UpdateWindow(GetDlgItem(hPrintDlg, PRINT_PAGE));
        if (StartPage(hPrnDC) <= 0) break;
        SelectObject(hPrnDC, hFont);
        is_next_page=gen_ed_page(edd, hPrnDC, &start, stop, xPext, yPext);
        SelectObject(hPrnDC, (HFONT)GetStockObject(OEM_FIXED_FONT));
        if (print_cancel_request) break;
        if (EndPage(hPrnDC) <= 0) break;
        if (!is_next_page) break;
       /* react to messages */
        deliver_messages(edd->cdp, FALSE);
        SetFocus(hPrintDlg);  /* somebody steals the focus, return it */
      }
      DestroyWindow(hPrintDlg);
      EndDoc(hPrnDC);
    }
    DeleteObject(hFont);
  }
  if (hPrnDC) DeleteDC(hPrnDC);
}
#endif

#ifdef STOP
static BOOL CALLBACK FsedPrintDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ editor_type * edd;
  switch (msg)
  { case WM_INITDIALOG:
      edd=(editor_type*)lP;  SetWindowLong(hDlg, DWL_USER, lP);
      CheckRadioButton(hDlg, CD_PRINT_ALL, CD_PRINT_SEL, CD_PRINT_ALL);
      EnableWindow(GetDlgItem(hDlg, CD_PRINT_SEL), 
        edd->block_on && edd->block_start < edd->block_end);
      write_printer_name(GetDlgItem(hDlg, CD_PRINT_NAME));
      return TRUE;
    case WM_COMMAND:
    { edd=(editor_type*)GetWindowLong(hDlg, DWL_USER);
      switch (GET_WM_COMMAND_ID(wP,lP))
      { case CD_PRINT_SELPRI:
          Printer_dialog(hDlg);
          write_printer_name(GetDlgItem(hDlg, CD_PRINT_NAME));
          return TRUE;
        case CD_PRINT_FONT:
          if (font_dialog(hDlg, &GetPP()->editor_printer_font, CF_PRINTERFONTS))
            save_prez_params(NULL);
          return TRUE;
        case 2:
          EndDialog(hDlg, FALSE);  return TRUE;
        case 1:
          if (IsDlgButtonChecked(hDlg, CD_PRINT_ALL))
            print_editor_text(edd, edd->editbuf, edd->editbuf+edd->txtsize);
          else
            print_editor_text(edd, edd->block_start, edd->block_end);
          EndDialog(hDlg, TRUE);  return TRUE;
      }
    }
  }
  return FALSE;
}
#endif

/**************************** supporting functions **************************/
#if 0
static UINT obj_filters[] = /* indexex by categories */
{ FILTER_TDF, FILTER_USR, FILTER_VWD, FILTER_CRS, FILTER_PGM, 0, FILTER_MNU,
  0, FILTER_PIC, 0, 0, 0, 0, FILTER_DRW, 0, FILTER_RRL, FILTER_PSM,
  FILTER_TRG, FILTER_WWW, 0, FILTER_SEQ, 0, FILTER_DOM };

CFNC DllPrezen UINT WINAPI get_filter(tcateg categ)
// Returns ID of the resource string containing object files filter
{ return categ==0xff ? FILTER_TXT : obj_filters[categ]; }

CFNC DllPrezen char * WINAPI get_suffix(tcateg categ)
// Returns category suffix starting with a dot
{ return first_suffix(get_filter(categ)); }
#endif
/******************************* save & name entering ***********************/
static wbbool prevent_unlink=wbfalse;

BOOL editor_type::wr_text(BOOL save_as)
{ BOOL err;  tobjnum codeobj;  BOOL added=FALSE;  HCURSOR oldcurs;
  if (categ==0xff || categ==CATEG_PROC || categ==CATEG_TRIGGER) save_as=FALSE; // fuse
  if (!save_as)
    if (!_changed || noedit) return TRUE;  // noedit is a fuse only
  if (cached)
  { indir_obj * iobj=(indir_obj*)load_cache_addr(hOwner, objnum, attr, index, TRUE);
    if (!iobj) return FALSE;
    HGLOBAL hnd = GlobalAlloc(GMEM_MOVEABLE, txtsize+HPCNTRSZ+1);
    if (!hnd) { no_memory();  return FALSE; }
    cached_longobj * lobj = (cached_longobj*)GlobalLock(hnd);
    lobj->maxsize=txtsize;
    memcpy(lobj->data, editbuf, txtsize);
    ((char *)lobj->data)[txtsize]=0;
    if (iobj->obj != NULL)
    { GlobalUnlock(GlobalPtrHandle(iobj->obj));
      GlobalFree  (GlobalPtrHandle(iobj->obj));
    }
    iobj->obj=lobj;
    iobj->actsize=txtsize;
   /* mark cached atribute as changed: */
    if (par_inst!=NULL)
    { register_edit(par_inst, attr);
      if (!(par_inst->stat->vwHdFt & REC_SYNCHRO))
      { prevent_unlink=wbtrue;
        cache_synchro(par_inst, FALSE, TRUE);
        prevent_unlink=wbfalse;
      }
    }
  }
  else  /* not cached text -> object definition: */
 /* return, saved OK */
  saved_now();
  _changed=false;  ch_status(false);
  post_notif(hOwner, NOTIF_CHANGE);
  return TRUE;
}

CFNC DllPrezen BOOL WINAPI save_new_source(tobjnum objnum)
{ HWND hwChild;  
  editor_type * edd = find_editor_by_objnum(get_RV()->hClient, objnum, OBJ_TABLENUM, &hwChild, NULL, FALSE, 0);
  if (edd==NULL) return FALSE;
  if (!edd->_changed) return FALSE;
  return !edd->wr_text(FALSE);
}

//CFNC DllPrezen void WINAPI object_fname(char * dest, const char * src, const char * app)
//{ if (GetPP()->longnames)  // use full object name
//  { strmaxcpy(dest, src, OBJ_NAME_LEN+1);  
//    for (int i=0;  dest[i];  i++)
//      if (dest[i]=='<' || dest[i]=='>' || dest[i]=='|' || dest[i]=='\"' || dest[i]=='\\' || dest[i]=='/' || dest[i]==';' || dest[i]=='?' || dest[i]==':')
//        dest[i]='_';
//    strcat(dest, app); 
//  }
//  else strip_diacr(dest, src, app);
//}

CFNC DllPrezen void WINAPI strip_diacr(char * dest, const char * src, const char * app)
/* forms fname in dest from src (without diacritics) & app */
{ strmaxcpy(dest, src, 8+1);
  for (int i=0;  dest[i];  i++)
    if ((unsigned char)dest[i]>=128)
      dest[i]=bez_outcode[(unsigned char)dest[i]-128];
    else if (dest[i]<'0' || dest[i]>'Z' && dest[i]<'a' ||
             dest[i]>'z' && dest[i]<128 || dest[i]>'9' && dest[i]<'A')
      if (dest[i]!='*') dest[i]='_';
  strcat(dest, app);
}

BOOL editor_type::check_xpos(void)
// Updates x_dsp so that current x position is visible, 
// returns TRUE iff window has to be redrawn.
{ BOOL change=FALSE;
  if (win_xlen==1) 
  { change = cur_char_n != x_dsp;
    x_dsp=cur_char_n;
  }
  else
  { while (cur_char_n < x_dsp)  /* don't use "if" ! */
    { unsigned i=win_xlen/2;
      if (x_dsp>i) x_dsp-=i; else x_dsp=0;
      change=TRUE;
    }
    while (cur_char_n-x_dsp >= win_xlen-1)
    { x_dsp+=win_xlen/2;
      change=TRUE;
    }
  }
  if (change) SetScrollPos(edwnd, SB_HORZ, x_dsp, TRUE);
  return change;
}

/************************* info box *****************************************/
static HWND hCompInfo = 0;

static BOOL CALLBACK ModelessCompInfoDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ RECT Rect;
  switch (msg)
  { case WM_INITDIALOG:
      GetWindowRect(GetWindow(hDlg, GW_OWNER), &Rect);
      SetWindowPos(hDlg, 0, Rect.left+100, Rect.top+100, 0, 0,
                   SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOSIZE|SWP_NOZORDER);
      EnableWindow(GetDlgItem(hDlg, CD_COMP_OK), FALSE);
      return TRUE;
    case WM_COMMAND:
      if ((GET_WM_COMMAND_ID(wP,lP)==CD_COMP_OK) || (GET_WM_COMMAND_ID(wP,lP)==1) || (GET_WM_COMMAND_ID(wP,lP)==2))
      { EnableWindow(GetFrame(GetParent(hDlg)), TRUE);
        DestroyWindow(hDlg);  hCompInfo=0;  return TRUE;
      }
      break;
//    case WM_HELP:  not used so far in the resource
//      return process_context_popup(GetcurrTaskPtr, (HELPINFO*)lP);
//      return TRUE;
  }
  return FALSE;
}

CFNC DllPrezen BOOL WINAPI check_object_privils(cdp_t cdp, tcateg categ, tobjnum objnum, int privtype)
// privtype==0: READ, 1: write, 2: delete
{ uns8 priv_val[PRIVIL_DESCR_SIZE];
  if (cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, CATEG2SYSTAB(categ), objnum, OPER_GETEFF, priv_val))
    return FALSE;
  if (privtype==0) return HAS_READ_PRIVIL(priv_val,OBJ_DEF_ATR) != 0;
  if (privtype==1) return HAS_WRITE_PRIVIL(priv_val,OBJ_DEF_ATR) != 0 && HAS_READ_PRIVIL(priv_val,OBJ_DEF_ATR) != 0;
    // most object editors need to read the current definition, so I am checking the READ and WRITE privils.
//if (privtype==2) 
    return (*priv_val & RIGHT_DEL) != 0;  // READ privil not checked since 8.0.5.2, no reason
}

CFNC DllPrezen BOOL WINAPI readable_object(cdp_t cdp, tcateg categ, tobjnum objnum)
{ if (!ENCRYPTED_CATEG(categ)) return TRUE;
  uns16 flags;
  cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_FLAGS_ATR, NULL, &flags);
  return !(flags & CO_FLAG_ENCRYPTED);
}

CFNC DllPrezen BOOL WINAPI check_readable_object(cdp_t cdp, tcateg categ, tobjnum objnum)
{ if (readable_object(cdp, categ, objnum)) return TRUE;
  client_error(cdp, APPL_IS_LOCKED);  cd_Signalize(cdp);
  return FALSE;
}

CFNC DllPrezen void WINAPI DestroyMdiChildren(HWND hwClient, BOOL close_editors)
{ HWND hwChild;  /*view_dyn * inst;*/  int mdi_child_type;  BOOL remove;
  BOOL hidden=FALSE;
  hwChild=GetWindow(hwClient, GW_CHILD);
 /* removes all MDI-children from the chain (views, editors) except APLWIN,
    must start from beginning after each removal */
  while (hwChild)
  { remove=!GetWindow(hwChild, GW_OWNER);  /* FALSE for icon title */
    if (remove)
    { mdi_child_type=(int)SendMessage(hwChild, SZM_MDI_CHILD_TYPE, 0, 0);
      if (mdi_child_type==APPL_MDI_CHILD)
        remove=FALSE;
      else if (mdi_child_type==PROGEDIT_MDI_CHILD || mdi_child_type==OBJEDIT_MDI_CHILD)
        if (close_editors)
        { SendMessage(hwClient, WM_MDIACTIVATE, (WPARAM)hwChild, 0); /* peeks editor window */
          SendMessage(hwChild, SZM_CLOSE_EDITOR, 0, 0);  /* saving changes in text */
        }
        else remove=FALSE;
        // must not close view editors: view compilation may set new project and close current project, which calls DestroyMdiChildren!
//      else if (mdi_child_type==EDIT_MDI_CHILD)
//      { SendMessage(hwClient, WM_MDIACTIVATE, (WPARAM)hwChild, 0); /* peeks editor window */
//        SendMessage(hwChild, SZM_CLOSE_EDITOR, 0, 0);  /* saving changes in text */
//      }
      else if (mdi_child_type==PANE_MDI_CHILD)
        remove=close_editors;
      else if (mdi_child_type==DRAWING_MDI_CHILD)
        if (close_editors==2)
        { SendMessage(hwClient, WM_MDIACTIVATE, (WPARAM)hwChild, 0); /* peeks editor window */
          SendMessage(hwChild, SZM_CLOSE_EDITOR, 0, 0);  /* saving changes in text */
        }
        else remove=FALSE;
    }
    if (remove)
    { if (!hidden) { ShowWindow(hwClient, SW_HIDE);  hidden=TRUE; }
      if (mdi_child_type==VIEW_MDI_CHILD)
        SendMessage(hwChild, SZM_SYNCHRO, TRUE, TRUE);
      SendMessage(hwClient, WM_MDIDESTROY, (WPARAM)hwChild, 0L);
      hwChild=GetWindow(hwClient, GW_CHILD);       /* restart the search */
    }
    else hwChild=GetWindow(hwChild, GW_HWNDNEXT);  /* check the next child */
  }
  cancel_printing();
  if (hidden) ShowWindow(hwClient, SW_SHOWNA);
}

static void hide_watches(void)
{ if (hWatches) ShowWindow(hWatches, SW_HIDE);
  CheckMenuItem(debug_menu,  MI_DBG_WATCHES, MF_BYCOMMAND|MF_UNCHECKED);
  CheckMenuItem(srvdbg_menu, MI_DBG_WATCHES, MF_BYCOMMAND|MF_UNCHECKED);
  toolbar_debug[9].flags &= ~TBFLAG_DOWN;
}

static void after_program_run(cdp_t cdp, int res)
{ HWND hClient = cdp->RV.hClient, active_child;  MSG msg;  int clres;
 /* must deliver the pending WM_MDIDESTROY messages before DestrMDICh, otherwise
    new windowns may get them! */
  BOOL quit_request = FALSE;
  Set_timer(cdp, 0, 0);
  while (PeekMessage(&msg, 0, WM_MDIDESTROY, WM_MDIDESTROY, PM_NOYIELD|PM_REMOVE))
  { if (msg.message==WM_QUIT) quit_request = TRUE;  /* otherwise msg is lost */
    else DispatchMessage(&msg);
  }
  cd_Roll_back(cdp);
  cdp->RV.global_state=DEVELOP_MENU;
  cdp->RV.default_tool_bar=NULL;
  if (cdp->RV.prev_run_state)  /* inner debugged run */ return;
 // destroy open views, then destruct views allocated on the heap:
  if (!cdp->non_mdi_client)
    DestroyMdiChildren(hClient, FALSE);
  clres=0;
  while (clres++<20)
  { view_dyn * inst = cdp->all_views;
    while (inst && inst->stat && (inst->stat->vwStyle3 & SYSTEM_FORM)) // skip system forms!
      inst=inst->next_view;
    if (inst && inst->stat) inst->destruct();
    else break;
  }

  cdp->RV.break_request=wbfalse;

  SendMessage(GetFrame(cdp->RV.hClient), WM_COMMAND, MAKELONG(IDM_MONITOR, 12345), 0); // close the monitor
  cd_Help_file(cdp, NULLSTRING);
  Set_status_text(NULLSTRING);
#if 0
  Set_status_nums(-1, -1);
#endif
  if (res!=EXECUTED_OK)  /* run error */
    if (res==PGM_BREAK) infbox(MAKEINTRESOURCE(RUN_BREAKED));
    else { client_error(cdp, res);  Signalize(); }
  /* foll. lines must not be called before reporting run error! */
  clres=close_run(cdp);  /* closes files & cursors */
  if (!cd_Close_cursor(cdp, (tcursnum)-2)) clres|=2;
  if (res==EXECUTED_OK)  /* no error, display warnings: */
  { if (clres & 1) wrnbox(MAKEINTRESOURCE(FILES_NOT_CLOSED));
    if (clres & 2) wrnbox(MAKEINTRESOURCE(CURSORS_NOT_CLOSED));
  }
 // close debugger objects:
  if (cdp->global_debug_mode)  /* debugged run */
  { hide_watches();
    if (hCalls) DestroyWindow(hCalls);
    debug_edit(hClient, TRUE);
  }
 // restore the GUI:
  if (quit_request) PostQuitMessage(0);
  else
  { Main_menu(NULL); /* user menu destroyed, development menu restored */
    active_child=(HWND)SendMessage(hClient, WM_MDIGETACTIVE, 0, 0);
    SendMessage(GetFrame(hClient), SZM_SHOWAPPL, TRUE, 0);
    PostMessage(hClient, WM_MDIACTIVATE, (WPARAM)active_child, 0);  // does not work, nor Send 
    //if (SendMessage(active_child, SZM_MDI_CHILD_TYPE, 0, 0) != APPL_MDI_CHILD)
  }
}
                                                       
CFNC DllPrezen short WINAPI run_pgm_chained(cdp_t cdp,
  const char * srcname, tobjnum srcobj, HWND hWnd, BOOL recompile)
/* on compilation error CompInfo remains on the screen */
{ int res;  HWND hFrame;  MSG msg;  run_state * RV = &cdp->RV;

  if (RV->global_state==PROG_RUNNING)
    { errbox(MAKEINTRESOURCE(PROG_IS_RUNNING));  return NO_RUN_RIGHT; }
  hFrame=GetFrame(hWnd);
  res=set_project_up(cdp, srcname, srcobj, recompile, FALSE);
  if (!res)
  { dynmem_start();
   /* remove menu & hAplWnd, allow to redraw the  status bar */
    SendMessage(hFrame, SZM_SHOWAPPL, FALSE, 0);
    RV->global_state=PROG_RUNNING;
    set_frame_menu(cdp->RV.hClient, (HMENU)GetProp(hWnd, EMPTYMENU), 0);
    Set_tool_bar(cdp, NULL);
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE | PM_NOYIELD))
    { TranslateMessage(&msg);      /* Translates virtual key codes         */
      DispatchMessage(&msg);       /* Dispatches message to window         */
    }
    RV->int_msg_queue.clear();  /* clear the internal msgs queue */
    RV->default_tool_bar=NULL;
    if (cdp->global_debug_mode)
    { run_prep(cdp, (tptr)RV->run_project.common_proj_code, RV->run_project.common_proj_code->main_entry, SET_BP);
      set_bp(cdp, 255, 0xffff, RV->run_project.common_proj_code->main_entry+3, BP_TEMPORARY);
      res=run_pgm(cdp);   /* executes the initial jump */
      if (res==PGM_BREAKED)  /* enter the "debug_breaked" state */
      { debug_edit(hWnd, FALSE); // may be wrong when not called from editor
        enter_breaked_state(cdp);
        return res;
      }
    }
    else
    { run_prep(cdp, (tptr)RV->run_project.common_proj_code, RV->run_project.common_proj_code->main_entry);
      res=run_pgm(cdp);
    }
    after_program_run(cdp, res);
  }
  return res;
}

/*************************** fsed utils *************************************/
BOOL editor_type::mkspace(tptr dest, tptr * psrc, unsigned size)
{ tptr src=*psrc;
  if (dest+size<=src) return TRUE;  /* space ok */
  size+=10;
  if (txtsize+size>=edbsize) return FALSE; /* no space, will overwrite text */
  memmov(src+size, src, sizeof(wxChar) * (txtsize+1-(src-editbuf)));
  txtsize+=size;
  adjust(src,size);
  *psrc+=size;
  return TRUE;
}

void editor_type::reformat(BOOL multiline)
{ tptr src, dest, p;  int indent, chars, words, wrdlen;  /* signed! */
  unsigned lns=0, mksp, w, mv;  BOOL hard;  int remd;  BOOL any_ref=FALSE;
  src=cur_line_a;  indent=0;
  while (*src==' ') { src++; indent++; }
  if (indent>win_xlen-1) indent=win_xlen-1;
  dest=src; hard=FALSE;
  do /* cycle through lines inside paragraph */
  { /* count the words & chars */
    chars=words=0; p=src;
    do /* cycle through words */
    { while (*p==' ') p++;   /* spaces before the word */
      wrdlen=0;
      while ((*p!=' ') && (*p!=0xd) && *p) { p++; wrdlen++; }
      if (wrdlen)
        if (chars)
          if (chars+wrdlen+1<win_xlen-indent) /* must be signed */
            { chars+=wrdlen+1; words++; }
          else break;
        else /* the first word on the line: accepct, partially at least */
          if ((chars=wrdlen)+1 >= win_xlen-indent) break; /* words is 0 now! */
          else words++;
      while (*p==0xd) p++;  /* soft returns ignored */
      hard=!*p || (*p==0xa);
    } while (!hard);  /* breaked when the line is full */
    if (!words) /* word filling the line */
    { mv=(wrdlen<win_xlen-indent-1) ? wrdlen : win_xlen-indent-1;
      memmov(dest,src,sizeof(wxChar) * mv);  any_ref=TRUE;
      dest+=mv; src+=mv;
    }
    else
    { if (hard || (words==1) || (categ!=0xff) || !is_align) { mksp=1; remd=0; }
      else
      { mksp=win_xlen-1-indent-chars;
        remd=mksp % (words-1);  mksp=(mksp / (words-1)) + 1;
      }
      for (w=0; w<words; w++)
      { if (w)    /* spaces between words */
        { while ((*src==' ')||(*src==0xd)) src++;
          if (!mkspace(dest, &src, mksp)) return;
          memset(dest,' ',mksp); dest+=mksp;  any_ref=TRUE;
        }
        /* copy the word */
        while ((*src!=' ') && (*src!=0xd) && (*src!=0xa) && *src)
          { *dest=*src; dest++; src++;  any_ref=TRUE; }
        if (w+remd+1==words) mksp++;  /* more-spaces line part */
      }
    }
    while ((*src==0xd)||(*src==' ')) src++;  /* soft returns ignored */
    if (!mkspace(dest, &src, 1)) return;
    *(dest++)=0xd;  any_ref=TRUE;
    lns++;
    indent=0;  /* next lines without indent */
  } while (!hard && multiline); /* next line of the paragraph */
  /* erase the workspace if any */
  if (src > dest)
  { memmov(dest,src, sizeof(wxChar) * (txtsize+1-(unsigned)(src-editbuf)));
    txtsize-=(remd=(int)(src-dest));
    adjust(src,-remd);
  }
  /* move cur_line_a, f_line_a */
  while (lns--) cur_line_a=next_line(cur_line_a);
  cur_char_n=0;
  do
  { set_y_curs();
    if (y_curs < win_ylen) break;
    f_line_a=next_line(f_line_a);
  } while (TRUE);
  if (any_ref) { undo_buffer.clear_all();  redo_buffer.clear_all();  update_toolbar(); }
}

/**********************************************************************************************
* t_line_kontext_www t_line_kontexts_www
* vytvareni kontextu radku ve zdrojaku WWW objektu
*/
static sig16 get_object_flag(cdp_t cdp,tobjnum objnum)
{
  sig16 flag;
  if( cd_Read_ind(cdp,OBJ_TABLENUM,objnum,OBJ_FLAGS_ATR,NOINDEX,&flag) ) return 0;
  return flag;
}
enum konektor_line_type { KONEKTOR_LINE_SQL,KONEKTOR_LINE_PROGRAM };
class t_line_kontext_www : public t_line_kontext
{ friend class t_line_kontexts_www;
  wbbool in_comment,in_html_tag,in_htw_tag,
	  in_konektor,in_string,pos_in_comment,pos_in_sql,in_program,in_sql_statement,in_script_tag,in_script_comment;
 public:
  virtual void create_kontext(editor_type * edd, const char * line); 
  virtual BOOL same(const t_line_kontext * ktx) const;
  virtual void tokenize_line(cdp_t cdp, tptr line, unsigned len, t_line_tokens * lts, t_content_category ccateg);
  virtual void tokenize_konektor_line(konektor_line_type type,tptr line,unsigned len,t_line_tokens *lts);
  virtual int starts_tag(tptr line,const char *tag,int endtag);
  virtual char *contains_tag(tptr line,const char *tag,int endtag);
};

int t_line_kontext_www::starts_tag(tptr line,const char *tag,int endtag)
{
  if( *line!='<' ) return 0;
  line++;
  for( ;*line!='\0'&&(*line==' '||*line=='\t'||*line=='\v');line++ ) ;
  if( endtag )
  {
    if( *line!='/' ) return 0;
    line++;
    for( ;*line!='\0'&&(*line==' '||*line=='\t'||*line=='\v');line++ ) ;
  }
  if( _strnicmp(line,tag,strlen(tag))!=0 ) return 0;
  return 1;
}
char *t_line_kontext_www::contains_tag(tptr line,const char *tag,int endtag)
{
  while( *line!='\0' )
  {
    if( starts_tag(line,tag,endtag) ) return line;
    line++;
  }
  return NULL;
}
void t_line_kontext_www::create_kontext(editor_type *edd,const char *line)
{
  switch( get_object_flag(edd->cdp,edd->objnum)&CO_FLAGS_OBJECT_TYPE )
  {
  case 0: // sablona
    in_comment=in_html_tag=in_htw_tag=in_konektor=in_string=pos_in_comment=pos_in_sql=in_program=in_sql_statement=in_script_tag=in_script_comment=FALSE;
    break; // musi se udelat analyza
  case CO_FLAG_WWWCONN: // konektor
    in_comment=in_html_tag=in_htw_tag=in_string=pos_in_comment=pos_in_sql=in_program=in_sql_statement=in_script_tag=in_script_comment=FALSE;
    in_konektor=TRUE;
    break; // musi se udelat analyza
  case CO_FLAG_WWWSEL: // selektor
  default:
    in_comment=in_html_tag=in_htw_tag=in_konektor=in_string=pos_in_comment=pos_in_sql=in_program=in_sql_statement=in_script_tag=in_script_comment=FALSE;
    return;
  }
  const char *aline=line;
  for( int i=0;i<512;i++ ) aline=edd->cprev_line(aline);
  t_line_tokens lts[MAX_TOKENS];
  while( aline<line )
  {
    unsigned len=txlnlen(aline);
    tokenize_line(cdp, (char*)aline,len,lts,CATEG_WWW);
    aline=edd->cnext_line(aline);
  }
}

BOOL t_line_kontext_www::same(const t_line_kontext *ktx) const
{
  BOOL ret=in_comment==((t_line_kontext_www*)ktx)->in_comment &&
    in_html_tag==((t_line_kontext_www*)ktx)->in_html_tag &&
    in_htw_tag==((t_line_kontext_www*)ktx)->in_htw_tag &&
    in_konektor==((t_line_kontext_www*)ktx)->in_konektor &&
    in_string==((t_line_kontext_www*)ktx)->in_string;
  return ret &&
    pos_in_comment==((t_line_kontext_www*)ktx)->pos_in_comment &&
    pos_in_sql==((t_line_kontext_www*)ktx)->pos_in_sql &&
    in_program==((t_line_kontext_www*)ktx)->in_program &&
    in_sql_statement==((t_line_kontext_www*)ktx)->in_sql_statement &&
    in_script_tag==((t_line_kontext_www*)ktx)->in_script_tag &&
    in_script_comment==((t_line_kontext_www*)ktx)->in_script_comment;
}

void t_line_kontext_www::tokenize_konektor_line(konektor_line_type type,tptr line,unsigned len,t_line_tokens *lts)
{ unsigned off = 0, cnt = 0;  BOOL local_sql = FALSE;
 // test for directive:
  if (*line=='#')
  { char dirname[10+1];  int i=0;
    do 
    { if (i<10) dirname[i++]=line[off];
      off++; 
    }
    while (is_AlphaW(line[off], cdp->sys_charset));
    dirname[i]=0;
    lts[0].off=0;  lts[0].token=TOK_DIRECTIVE; 
    cnt=1;
    lts[1].off=off;  lts[1].token=TOK_PLAIN; // to the normal state
    Upcase(dirname);
    if      (!strcmp(dirname, "#SQL"))      local_sql =TRUE;
    else if (!strcmp(dirname, "#SQLBEGIN")) pos_in_sql=TRUE;
    else if (!strcmp(dirname, "#SQLEND"))   pos_in_sql=FALSE;
  }
 // INV: analyse the line from off, store token or terminator to lts[cnt]
  while (off<len && cnt+2<MAX_TOKENS)
  { lts[cnt].off=off;  // token start
   // INV: scanning token to to lts[cnt], its beginning stored to lts[cnt].off
    if (pos_in_comment)
    { lts[cnt].token=TOK_COMMENT;
      while (off < len)
      { if (line[off]=='}') { off++;  pos_in_comment=wbfalse;  break; }
        if (line[off]=='*' && off+1 < len && line[off+1]=='/') { off+=2;  pos_in_comment=wbfalse;  break; }
        off++;
      }
      cnt++;
    }
    else  // not in comment
    { lts[cnt].token=TOK_PLAIN;   // supposed token type
      while (off<len && cnt+2<MAX_TOKENS) // extending TOK_PLAIN in lts[cnt]
      { if (line[off]=='\'' || line[off]=='\"')  // start string token
        { if (off > lts[cnt].off) lts[++cnt].off=off;  // new token start
          lts[cnt].token=TOK_STRING; 
          off++;
          while (off<len)  // find the end of the string
          { if (line[off]=='\'' || line[off]=='\"')
              if (off+1>len || line[off+1]!='\'' && line[off+1]!='\"')
                { off++;  break; }
              else off+=2;
            else off++;
          }
          lts[++cnt].off=off;  lts[cnt].token=TOK_PLAIN; // to the normal state
        }
        else if (is_AlphaW(line[off], cdp->sys_charset))
        { tobjname name;  int i=0;
          unsigned idstart=off;
          do
          { if (i<OBJ_NAME_LEN) name[i++]=upcase_charW(line[off], cdp->sys_charset);
          } while (++off<len && is_AlphanumW(line[off], cdp->sys_charset));
          name[i]=0;
          int idtype=check_atr_name(name);
          if( (idtype & IDCL_IPL_KEY) && type==KONEKTOR_LINE_PROGRAM ||
              (idtype & IDCL_SQL_KEY) && (type==KONEKTOR_LINE_SQL || pos_in_sql || local_sql) )
          { if (idstart > lts[cnt].off) lts[++cnt].off=idstart;  
            lts[cnt].token=TOK_KEYWORD; 
            lts[++cnt].off=off;  lts[cnt].token=TOK_PLAIN; // to the normal state
          }
          else if (idtype & IDCL_STDID ||
                  (idtype & IDCL_SQL_STDID) && (type==KONEKTOR_LINE_SQL || pos_in_sql || local_sql) )
          { if (idstart > lts[cnt].off) lts[++cnt].off=idstart;  
            lts[cnt].token=TOK_STDDEF; 
            lts[++cnt].off=off;  lts[cnt].token=TOK_PLAIN; // to the normal state
          }
        }
        else if (line[off]=='{' || line[off]=='/' && off+1<len && line[off+1]=='*')
        { pos_in_comment=wbtrue;  
          break;
        }
        else if ((line[off]=='-' || line[off]=='/') && off+1<len && line[off+1]==line[off])
        { if (off > lts[cnt].off) lts[++cnt].off=off;
          lts[cnt].token=TOK_COMMENT;  off=len;
        }
        else if (line[off]=='`')
        { while (line[++off]!='`' && off<len) ;
          off++;
        }
        else off++;
      }
      if (off > lts[cnt].off) cnt++;  // end of line or comment started
    } // not in comment
  }
  lts[cnt].off=(unsigned)-1;  // storing the terminator
}

void t_line_kontext_www::tokenize_line(cdp_t cdp, tptr line,unsigned len,t_line_tokens *lts,t_content_category ccateg)
{ // roztokenizovat a zmenit kontext tak, aby odpovidal konci tohoto radku
  if( len==0 || line==NULL ) { lts[0].off=0;lts[0].token=TOK_PLAIN; lts[1].off=(unsigned)-1; return; }
  size_t off=0;
  int cnt=0;
  if( in_konektor )
  {
    if( *line==';' || (line[0]=='/' && line[1]=='/') ) { lts[0].off=0; lts[0].token=TOK_COMMENT; lts[1].off=(unsigned)-1; return; }
    if( _strnicmp(line,"EndProgram",10)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=10; cnt=1; in_program=FALSE; }
    else if( _strnicmp(line,"End",3)==0 && _strnicmp(line,"EndSQL",6)!=0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=3; cnt=1; in_program=FALSE; }
    else if( in_program )
    { // radek v BeginProgram ... EndProgram
      tokenize_konektor_line(KONEKTOR_LINE_PROGRAM,line,len,lts);
      return;
    }
    else if( _strnicmp(line,"SQLStatement",12)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=12; cnt=1; in_sql_statement=TRUE; pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Program",7)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=7; cnt=1; in_program=TRUE; in_sql_statement=pos_in_comment=pos_in_sql=FALSE; }
		else if( _strnicmp(line,"BeginProgram",12)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=12; cnt=1; in_program=TRUE; in_sql_statement=pos_in_comment=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"RunProgram",10)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=10; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Template",8)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=8; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"ErrorTemplate",13)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=13; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
		else if( _strnicmp(line,"RequiredValues",14)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=14; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"MaxRecords",10)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=10; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"GetVariable",11)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=11; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"GetVar",6)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=6; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"EndSQLStatement",15)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=15; cnt=1; in_sql_statement=FALSE; pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"EndSQL",6)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=6; cnt=1; in_sql_statement=FALSE; pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Start_transaction",17)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=17; cnt=1; in_sql_statement=FALSE; pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Commit",6)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=6; cnt=1; in_sql_statement=FALSE; pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Roll_back",9)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=9; cnt=1; in_sql_statement=FALSE; pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"SetVariable",11)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=11; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"SetVar",6)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=6; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"DeclareVariable",15)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=15; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"DeclareVar",10)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=10; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Content-type",12)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=12; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"SendFile",8)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=8; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Set-cookie",10)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=10; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Location",8)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=8; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"ImportFile",10)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=10; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"Error",5)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=5; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( _strnicmp(line,"IgnoreBlanks",12)==0 ) { lts[0].off=0; lts[0].token=TOK_KEYWORD; off=12; cnt=1; in_sql_statement=pos_in_comment=in_program=pos_in_sql=FALSE; }
    else if( *line=='#' ) { lts[0].off=0; lts[0].token=TOK_DIRECTIVE; off=len-1; cnt=1; }
    else if( in_sql_statement && *line=='+' )
    {
      tokenize_konektor_line(KONEKTOR_LINE_SQL,line,len,lts);
      return;
    }
    else
    {
      lts[0].off=0; lts[0].token=TOK_PLAIN; cnt=1;
      lts[cnt].off=(unsigned)-1; // konec tokenu
      // otestovat na pritomnost ukoncovace konektorove sekce <% /wbc %>
      char *par=NULL;
      if( (par=strstr(line,"<%"))==NULL ) return; // neni tam
      if( par-line>=len ) return;
      char *tmp=par+2;
      while( (*tmp!='\r' && *tmp!='\0')&&(*tmp==' ' || *tmp=='\t' || *tmp=='\v') ) tmp++;
      if( _strnicmp(tmp,"/wbc",4)!=0 ) return;
      tmp+=4;
      while( (*tmp!='\r' && *tmp!='\0')&&(*tmp==' ' || *tmp=='\t' || *tmp=='\v') ) tmp++;
      if( tmp[0]!='%' || tmp[1]!='>' ) return;
      lts[cnt].off=par-line; lts[cnt++].token=TOK_DIRECTIVE;
      off=tmp-line+2; // znak za %>
      in_konektor=pos_in_comment=pos_in_sql=in_program=in_sql_statement=FALSE;
      lts[cnt].off=(unsigned)-1;
      // a pokracuje se v tokenizovani podle sablony
      goto continue_tokenize_template;
    }
    if( lts[0].token==TOK_KEYWORD )
    {
      lts[cnt].off=off;
      lts[cnt++].token=TOK_PLAIN;
    }
    lts[cnt].off=(unsigned)-1; // konec tokenu
    return;
  }
continue_tokenize_template:
  while( off<len && cnt<MAX_TOKENS-2 )
  { // v tomto cyklu se vzdy prida jeden token
    lts[cnt].off=off;
    lts[cnt].token=TOK_PLAIN;
    while( off<len && cnt<MAX_TOKENS-2 )
    { // cyklus se opousti v okamziku, kdy najdeme konec noveho tokenu
      if( in_comment || (!(in_script_tag&&!in_html_tag)&&strncmp(line+off,"<!--",4)==0) )
      { // zacatek komentare
        if( off>lts[cnt].off ) lts[++cnt].off=off;
        lts[cnt].token=TOK_COMMENT;
        if( !in_comment ) { off+=4; in_comment=TRUE; }
        char *endc=strstr(line+off,"-->");
        if( endc==NULL ) { off=len+1; }
        else if( endc-line>=len ) { off=len+1; }
        else { off=endc-line+3; in_comment=FALSE; }
        break;
      }
      else if( in_script_tag && !in_html_tag && !in_htw_tag && (in_script_comment || !(line[off]=='<' && line[off+1]=='%') ) )
      {
        if( off>lts[cnt].off ) lts[++cnt].off=off;
        char *endc=NULL;
        unsigned origoff=off;
        if( in_script_comment || strncmp(line+off,"/*",2)==0 )
        {
          lts[cnt].token=TOK_COMMENT;
          if( !in_script_comment ) { in_script_comment=TRUE; off+=2; }
          endc=strstr(line+off,"*/");
          if( endc!=NULL && endc-line<len )
          {
            off=endc-line+2;
            in_script_comment=FALSE;
          }
          else { off=len+1; }
        }
        else if( strncmp(line+off,"<!--",4)==0 || (line[off]=='/' && line[off+1]=='/') )
        {
          lts[cnt].token=TOK_COMMENT;
          off=len+1;
        }
        else
        {
          lts[cnt].token=TOK_PLAIN;
          endc=strstr(line+off,"<!--");
          char *tmpendc=strstr(line+off,"//");
          if( tmpendc!=NULL ) if( endc==NULL || tmpendc<endc ) endc=tmpendc;
          tmpendc=strstr(line+off,"/*");
          if( tmpendc!=NULL ) if( endc==NULL || tmpendc<endc ) endc=tmpendc;
          if( endc==NULL ) { off=len+1; }
          else if( endc-line>=len ) { off=len+1; }
          else { off=endc-line; }
        }
        endc=contains_tag(line+origoff,"script",1);
        char *tmpendc=contains_tag(line+origoff,"style",1);
        if( tmpendc!=NULL && endc!=NULL ) { if( tmpendc<endc) endc=tmpendc; }
        else { if( tmpendc!=NULL ) endc=tmpendc; }
        int htwstart=0;
        if( !in_script_comment )
        { tmpendc=strstr(line+origoff,"<%");
          if( tmpendc!=NULL && endc!=NULL ) { if( tmpendc<endc && lts[cnt].token!=TOK_COMMENT ) { endc=tmpendc; htwstart=1; } }
          else { if( tmpendc!=NULL && lts[cnt].token!=TOK_COMMENT ) { endc=tmpendc; htwstart=1; } }
        }
        if( endc==NULL  ) { /*off=len+1;*/ }
        else if( endc-line>=len ) { /*off=len+1;*/ }
        else if( endc-line<off ) { off=endc-line; if( !htwstart ) { in_script_tag=in_script_comment=FALSE; } }
        break;
      }
      else if( in_htw_tag || (line[off]=='<' && line[off+1]=='%') )
      {
        if( off>lts[cnt].off ) lts[++cnt].off=off;
        lts[cnt].token=TOK_STDDEF;
        if( !in_htw_tag ) { in_htw_tag=TRUE; off+=2; }
        while( off<len && (line[off]==' '||line[off]=='\t'||line[off]=='\v') ) off++;
        if( _strnicmp(line+off,"wbc",3)==0 )
        {
          off+=3;
          while( off<len && (line[off]==' '||line[off]=='\t'||line[off]=='\v') ) off++;
          if( off<len )
          {
            if( line[off]=='%' && line[off+1]=='>' )
            { // zacatek konektoru
              in_konektor=TRUE;
			        pos_in_comment=pos_in_sql=in_program=in_sql_statement=FALSE;
              in_htw_tag=FALSE;
              lts[cnt].token=TOK_DIRECTIVE;
              if( cnt<MAX_TOKENS-3 )
              {
                cnt++;//abychom neprepsali TOK_DIRECTIVE
                lts[cnt].off=off+2;
                lts[cnt].token=TOK_PLAIN; // zbytek radku za <%wbc%>
              }
              off=len+1; // koncime analyzu radku
              break;
            }
          }
        }
        while( off<len && (line[off]!='%' || line[off+1]!='>') ) off++;
        if( off<len )
        { // htw tag konci jeste na tomto radku
          in_htw_tag=FALSE;
          off+=2;
        }
        break;
      }
      else if( in_string || line[off]=='\"' )
      { // string
        if( off>lts[cnt].off ) lts[++cnt].off=off;
        lts[cnt].token=TOK_STRING;
        if( !in_string ) { off++; in_string=TRUE; }
        while( off<len && line[off]!='\"' )
        {
          if( line[off]=='<' && line[off+1]=='%' ) { break; }
          off++;
        }
        if( off<len && line[off]=='\"' )
        {
          in_string=FALSE;
          off++;
        }
        break;
      }
      else if( in_html_tag || line[off]=='<' )
      { // HTML tag
        if( off>lts[cnt].off ) lts[++cnt].off=off;
        lts[cnt].token=TOK_KEYWORD;
        if( !in_html_tag ) 
        { off++; in_html_tag=TRUE;
          for( ;off<len&&line[off]!='>'&&(line[off]==' '||line[off]=='\t'||line[off]=='\v');off++ ) ;
          if( off<len )
          { if( _strnicmp(line+off,"script",6)==0 ) { in_script_tag=TRUE; off+=6; }
            if( _strnicmp(line+off,"style",5)==0 ) { in_script_tag=TRUE; off+=5; }
          }
        }
        while( off<len && line[off]!='>' )
        {
          if( line[off]=='\"' ) { break; }
          else if( line[off]=='<' && line[off+1]=='%' ) { break; }
          off++;
        }
        if( off<len && line[off]=='>' )
        { // konec HTML tagu
          in_html_tag=FALSE;
          off++;
        }
        break;
      }
      else
      { // obecny text
        off++;
      }
    }
    if( off>lts[cnt].off ) cnt++;
  }
  lts[cnt].off=(unsigned)-1;
}
//---------------------------------------------------------------
class t_line_kontexts_www : public t_line_kontexts
{ enum { MAX_FSED_LINES=512 };
  uns32 start_in_comment[MAX_FSED_LINES/32+1];
  uns32 start_in_html_tag[MAX_FSED_LINES/32+1];
  uns32 start_in_htw_tag[MAX_FSED_LINES/32+1];
  uns32 start_in_konektor[MAX_FSED_LINES/32+1];
  uns32 start_in_string[MAX_FSED_LINES/32+1];
  uns32 start_pos_in_comment[MAX_FSED_LINES/32+1];
  uns32 start_pos_in_sql[MAX_FSED_LINES/32+1];
  uns32 start_in_program[MAX_FSED_LINES/32+1];
  uns32 start_in_sql_statement[MAX_FSED_LINES/32+1];
  uns32 start_in_tscript[MAX_FSED_LINES/32+1];
  uns32 start_in_cscript[MAX_FSED_LINES/32+1];
 public:
  virtual void store(int linenum, const t_line_kontext * ktx); 
  virtual void load (int linenum, t_line_kontext * ktx) const; 
  virtual BOOL same (int linenum, const t_line_kontext * ktx) const;
};

void t_line_kontexts_www::store(int linenum, const t_line_kontext * ktx)
{ t_line_kontext_www * sktx = (t_line_kontext_www*)ktx;
  if( sktx->in_comment ) start_in_comment[linenum/32] |= 1<<(linenum%32);
  else start_in_comment[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->in_html_tag ) start_in_html_tag[linenum/32] |= 1<<(linenum%32);
  else start_in_html_tag[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->in_htw_tag ) start_in_htw_tag[linenum/32] |= 1<<(linenum%32);
  else start_in_htw_tag[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->in_konektor ) start_in_konektor[linenum/32] |= 1<<(linenum%32);
  else start_in_konektor[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->in_string ) start_in_string[linenum/32] |= 1<<(linenum%32);
  else start_in_string[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->pos_in_comment ) start_pos_in_comment[linenum/32] |= 1<<(linenum%32);
  else start_pos_in_comment[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->pos_in_sql ) start_pos_in_sql[linenum/32] |= 1<<(linenum%32);
  else start_pos_in_sql[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->in_program ) start_in_program[linenum/32] |= 1<<(linenum%32);
  else start_in_program[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->in_sql_statement ) start_in_sql_statement[linenum/32] |= 1<<(linenum%32);
  else start_in_sql_statement[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->in_script_tag ) start_in_tscript[linenum/32] |= 1<<(linenum%32);
  else start_in_tscript[linenum/32] &= ~(1<<(linenum%32));
  if( sktx->in_script_comment ) start_in_cscript[linenum/32] |= 1<<(linenum%32);
  else start_in_cscript[linenum/32] &= ~(1<<(linenum%32));
}

void t_line_kontexts_www::load (int linenum, t_line_kontext * ktx) const 
{ t_line_kontext_www * sktx = (t_line_kontext_www*)ktx;
  sktx->in_comment = (start_in_comment[linenum/32] & 1<<(linenum%32))!=0;
  sktx->in_html_tag = (start_in_html_tag[linenum/32] & 1<<(linenum%32))!=0;
  sktx->in_htw_tag = (start_in_htw_tag[linenum/32] & 1<<(linenum%32))!=0;
  sktx->in_konektor = (start_in_konektor[linenum/32] & 1<<(linenum%32))!=0;
  sktx->in_string = (start_in_string[linenum/32] & 1<<(linenum%32))!=0;
  sktx->pos_in_comment = (start_pos_in_comment[linenum/32] & 1<<(linenum%32))!=0;
  sktx->pos_in_sql = (start_pos_in_sql[linenum/32] & 1<<(linenum%32))!=0;
  sktx->in_program = (start_in_program[linenum/32] & 1<<(linenum%32))!=0;
  sktx->in_sql_statement = (start_in_sql_statement[linenum/32] & 1<<(linenum%32))!=0;
  sktx->in_script_tag = (start_in_tscript[linenum/32] & 1<<(linenum%32))!=0;
  sktx->in_script_comment = (start_in_cscript[linenum/32] & 1<<(linenum%32))!=0;
}
BOOL t_line_kontexts_www::same (int linenum, const t_line_kontext * ktx) const
{ t_line_kontext_www aktx;
  load(linenum, &aktx);
  return aktx.same(ktx);
}

/***********************************************************************************************/

void editor_type::move_block(BOOL copying)
{ tptr pos=cur_line_a+cur_char_n;
  if (block_end > editbuf+txtsize-2) block_end = editbuf+txtsize-2;
  if (block_start > editbuf+txtsize) block_start = editbuf+txtsize;
  if (block_start>=block_end || pos>=block_start && pos<block_end) return; // no block or moving to inside
  fill_line();
  unsigned sz=block_end-block_start;
  if (txtsize+sz >= edbsize) { MessageBeep(-1);  return; }
  register_action(ACT_TP_INS, pos-editbuf, 0, (tptr)sz);
  changed();
  memmov(pos+sz, pos, sizeof(wxChar) * (txtsize-(pos-editbuf)+1));  txtsize+=sz;
  adjust(pos,sz);
  memcpy(pos, block_start, sizeof(wxChar) * sz);
  if (copying) /* copy: mark the new copy of the block */
  { block_start=pos; block_end=pos+sz;
    set_y_curs();
  }
  else  /* "move block" continues by deleting the original block */
  { add_seq_flag();
    delete_block();
    if (pos>=block_end) pos-=sz;
    block_start=pos; block_end=pos+sz;
  }
}

static tptr  blockpos;
// data describing the state before break in the program:
static uns8 breaked_state;
static LRESULT breaked_menu;
static HWND breaked_active_win;
static BOOL disabled_state;

unsigned text_save_buttons[3] =
{ BTEXT_SAVE | SELECTED_BUTTON,
  BTEXT_NOSAVE,  BTEXT_CANCEL | LAST_BUTTON | ESCAPE_BUTTON };
static unsigned confirm_changes_buttons[3] =
{ BTEXT_TX_CONFIRM | SELECTED_BUTTON,
  BTEXT_TX_UNDO,  BTEXT_CANCEL | LAST_BUTTON | ESCAPE_BUTTON };
static unsigned save2_buttons[2] =
{ BTEXT_SAVE | SELECTED_BUTTON,  BTEXT_NOSAVE | LAST_BUTTON };

static void set_eval_history(HWND hCtrl)
{ tptr p;
  SendMessage(hCtrl, CB_RESETCONTENT, 0, 0);
  p=eval_history;
  while (*p!=H_DELIM)
  { SendMessage(hCtrl, CB_ADDSTRING, 0, (DWORD)p);
    p+=strlen(p)+1;
  }
  SendMessage(hCtrl, CB_SETCURSEL, 0, 0);
}

void editor_type::get_kontext_string(tptr buf, int maxsize)
{ int size, i;  tptr pos;
  if (block_on && (block_start < block_end) &&
                (block_start+maxsize > block_end))
  { size=block_end-block_start;
    if (size>=maxsize) size=maxsize-1;
    memcpy(buf, block_start, sizeof(wxChar) * size);
  }
  else
  { pos=cur_line_a;
    i=txlnlen(pos);
    if (i > cur_char_n) i=cur_char_n;
    while (i && is_AlphanumW(pos[i-1], cdp->sys_charset)) i--;
    size=0;  BOOL digits_only=TRUE;
    if (is_AlphanumW(pos[i], cdp->sys_charset))
      while (size<maxsize-1)
      { char c=pos[i];
        if (is_AlphanumW(c, cdp->sys_charset))
        { if ((c<'0' || c>'9') && c!='e' && c!='E') digits_only=FALSE;
        }
        else if (!digits_only) break;
        else if (c!='$' && c!='+' && c!='-' && c!='.') break;
        buf[size++]=c;  i++;
      }
  }
  buf[size]=0;
}

static editor_type * get_active_program_editor(cdp_t cdp)
{ HWND hEd=(HWND)SendMessage(cdp->RV.hClient, WM_MDIGETACTIVE, 0, 0);
  if (!hEd || SendMessage(hEd, SZM_MDI_CHILD_TYPE, 0, 0) != PROGEDIT_MDI_CHILD)
    return NULL;
  editor_type * edd=(editor_type*)GetWindowLong(hEd, 0);
  if (edd==(editor_type*)EDCONT_WINDOW_LONG) // MDI child is editor container, search in it
    edd=(editor_type*)GetWindowLong(the_edcont.hDispl, 0);
  return edd;
}
    
////////////////////////////////////// server debug //////////////////////////////////////
static void CALLBACK DebugTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

////////////////////////////////////// IPL debug /////////////////////////////////////////
void get_watch_def(HWND hList, unsigned sel, tptr buf, short maxsize)
{ char fbuf[100+256+10];  int i;
  i=0;
  if (SendMessage(hList, LB_GETTEXTLEN, sel, 0) < sizeof(fbuf))
  { SendMessage(hList, LB_GETTEXT, sel, (LPARAM)fbuf);
         while (fbuf[i] && (fbuf[i]!=':')) i++;
         if (i>=maxsize) i=maxsize-1;
    memcpy(buf, fbuf, i);
  }
  buf[i]=0;
}

objtable * find_kontext(cdp_t cdp, BOOL cursor_kontext)
/* sets auto_vars & return the additional compilation kontext */
{ uns32 my_startadr, startadr, progentry, clo_diff, coffset, procend, hugepc;
  uns8 filenum;  scopevars * sc;  
  int i, clo_ind;  objtable * id_tab, * local_table;  uns8 cat;
  run_state * RV = &cdp->RV;
  local_table=NULL;
  run_prep(cdp, NULL, 0);  /* must be before the auto_vars change! */
  hugepc=RV->prev_run_state->pc;
  if (cursor_kontext)
  { editor_type * edd = get_active_program_editor(cdp);
    if (edd==NULL || edd->cursnum!=OBJ_TABLENUM) return NULL;
    filenum=find_file_num(cdp, (tobjnum)edd->objnum);
    if (filenum==0xff) return NULL;
    offset_from_line(cdp, edd->cur_line_n+1, filenum, &coffset);
  }
  else coffset=hugepc;

 /* find compilation kontext based on "coffset" */
  progentry=RV->run_project.common_proj_code->main_entry;
  if (coffset >= progentry)
          { clo_diff=coffset - progentry;  clo_ind=-1; }
  else clo_diff=0xffff;
  id_tab=RV->run_project.global_decls;
  for (i=0; i<id_tab->objnum; i++)
  { if (!HIWORD(id_tab->objects[i].descr)) continue; /* standard type alias */
    cat=id_tab->objects[i].descr->any.categ;
    if ((cat==O_FUNCT) || (cat==O_PROC))
    { startadr=id_tab->objects[i].descr->subr.code;
      if ((startadr <= coffset) && (coffset-startadr < clo_diff))
                  { clo_diff=coffset-startadr;  clo_ind=i; }
    }
  }
  if ((clo_diff!=0xffff) && (clo_ind!=-1))
  { local_table=id_tab->objects[clo_ind].descr->subr.locals;
   /* search for procedure code end */
    my_startadr=id_tab->objects[clo_ind].descr->subr.code;
          procend=progentry;
    id_tab=RV->run_project.global_decls;
    for (i=0; i<id_tab->objnum; i++)
    { if (!HIWORD(id_tab->objects[i].descr)) continue; /* standard type alias */
      cat=id_tab->objects[i].descr->any.categ;
      if ((cat==O_FUNCT) || (cat==O_PROC))
      { startadr=id_tab->objects[i].descr->subr.code;
        if ((startadr > my_startadr) && (startadr < procend))
          procend=startadr;
      }
    }
   /* selected procedure is from my_startadr to procend.
      Find its stack frame. */
          sc=RV->prev_run_state->auto_vars;
    do
    { if (sc==RV->prev_run_state->glob_vars) return NULL; /* stack frame not found */
      if ((hugepc>=my_startadr) && (hugepc<procend)) break;
      hugepc=sc->retadr;
      sc=sc->next_scope;
    } while (TRUE);
    RV->auto_vars=sc;   /* important side-efect! */
  }
  return local_table;
}

static void convert_value(cdp_t cdp, basicval * val, typeobj * tobj, tptr expr)
{ uns8 tp, tpcat;  int binary_length=12;
  if (COMPOSED(tobj))
  { tpcat=tobj->type.anyt.typecateg;
    if (tpcat==T_STR)
      tp=ATT_STRING;
    else if (tpcat==T_RECCUR || tpcat==T_PTR || tpcat==T_PTRFWD)
      tp=ATT_INT32;
    else if (tpcat==T_BINARY) 
    { tp=ATT_BINARY;
      binary_length=tobj->type.anyt.valsize;
      if (binary_length>128) binary_length=128;  // value buffer size limit
    }
    else if (tpcat==T_ARRAY && tobj->type.array.elemtype==MAKE_TOBJ(ATT_CHAR)) 
      tp=ATT_STRING;
    else tp=0xff;
  }
  else tp=(uns8)DIRTYPE(tobj);
  if (tp!=0xff)
  { if (tp==ATT_UNTYPED)
    { if (IS_DB_OBJ(tobj))  /* untyped database access */
      { basicval bval;  bval.ptr=expr;
        if (untyped_assignment(cdp, &bval, val, ATT_STRING, ATT_UNTYPED, UNT_SRCDB, 255))
          strcpy(expr, "*");
        return;  /* returning undecorated value */
      }
      else   /* untyped variable */
      { tp=val->unt->uany.type;
        if (!tp) { expr[0]=0;  return; }
        if (is_string(tp)) val->ptr=val->unt->uptr.val;
        else memmov(val, val->unt->uany.val, 8);
      }
    }
    else if (tp==ATT_TABLE || tp==ATT_CURSOR)
      tp=ATT_INT32;

    if (is_string(tp))
    { expr[0]='"';
      strmaxcpy(expr+1, val->ptr, 254);  /* string may be very long! */
      strcat(expr, "\"");
    }
    else if (tp==ATT_BINARY)
    { expr[0]='X';  expr[1]='\'';
      conv2str((basicval0*)val, tp, expr+2, binary_length);
      strcat(expr, "\'");
    }
    else if (IS_HEAP_TYPE(tp))
      strcpy(expr, "*");
    else  /* non-string */
    { int param;
      param = tp==ATT_TIME  ? 2 : tp==ATT_DATE  ? 1 : tp==ATT_FLOAT ? -5 :
              tp==ATT_MONEY ? 1 : tp==ATT_DATIM ? 5 : tp==ATT_TIMESTAMP ? 5 : 0;
      conv2str((basicval0*)val, tp, expr, param);
      if (tp==ATT_FLOAT)
      { tptr p=expr;  char sep=*get_separator(0);
        while (*p) { if (*p==sep) *p='.';  p++; }
      }
      else if (tp==ATT_INT16)
        sprintf(expr+strlen(expr), " (0x%x)", (unsigned)val->int16);
      else if (tp==ATT_INT32)
        sprintf(expr+strlen(expr), " (0x%lx)", val->int32);
      else if (tp==ATT_CHAR)
        sprintf(expr+strlen(expr), " (0x%x)", (unsigned)(uns8)val->int8);
    }
  }
  else strcpy(expr, "*");
}

static typeobj * evaluate(cdp_t cdp, tptr expr, BOOL cursor_kontext, BOOL disable_comp_error)
// Evaluates extr to TOS, returns its type or NULL on error.
{ compil_info xCI(cdp, expr, MEMORY_OUT, view_value);
 // find the compilation context:
  xCI.id_tables=find_kontext(cdp, cursor_kontext);
 // compile expr: 
  xCI.kntx=(kont_descr*)1;
  int res=compile(&xCI);
  if (res) { if (!disable_comp_error) client_error(cdp, res);  return NULL; }
  typeobj * tobj = (typeobj*)xCI.univ_ptr;
 // evaluate the value:
  cdp->RV.runned_code=xCI.univ_code;  /* base for view definitions */
  cdp->RV.pc         =0;
  cdp->RV.wexception =NO_EXCEPTION;
  res=run_pgm(cdp);
  corefree(xCI.univ_code);
  if (res!=EXECUTED_OK) { client_error(cdp, res);  return NULL; }
  return tobj;
}

static void watch_recalc(cdp_t cdp)
{ char srcbuf[100+2+256+2+1]; unsigned sel, cnt;  int len;  HWND hList;
  if (!hWatches) return;
  if (!IsWindowVisible(hWatches)) return;
  hList = GetDlgItem(hWatches, CD_WATCH_LIST);
  cnt=(unsigned)SendMessage(hList, LB_GETCOUNT, 0, 0);
  if (cnt==(unsigned)LB_ERR) return;
  for (sel=0; sel<cnt; sel++)
  { get_watch_def(hList, sel, srcbuf, 100);
    if (*srcbuf)
    { len=strlen(srcbuf);
      if (srv_dbg_mode)
      { if (cd_Server_eval(cdp, 0, srcbuf, srcbuf+len+2))
          Get_error_num_text(cdp, cd_Sz_error(cdp), srcbuf+len+2, 250);
      }
      else
      { typeobj * tobj = evaluate(cdp, srcbuf, FALSE, FALSE);
        if (tobj==NULL) LoadString(hPrezInst, CANNOT_EVALUATE, srcbuf+len+2, 30);
        else convert_value(cdp, cdp->RV.stt, tobj, srcbuf+len+2);
      }
      srcbuf[len]=':';  srcbuf[len+1]=' ';
      SendMessage(hList, LB_DELETESTRING, sel, 0);
      SendMessage(hList, LB_INSERTSTRING, sel, (LPARAM)srcbuf);
    }
  }
}

static void list_record(cdp_t cdp, HWND hList, tptr ptr, recordtype * rtobj)
{ char buf[NAMELEN+1+256+2+1];  objtable * ot = rtobj->items;
  for (int i=0;  i<ot->objnum;  i++)      
    for (int j=0;  j<ot->objnum;  j++)      
      if (ot->objects[j].descr->var.ordr==i)
      { strcpy(buf, ot->objects[j].name);  small_string(buf, FALSE);
        int len=strlen(buf);
        buf[len++]='\t';
        typeobj * etobj = ot->objects[j].descr->var.type;
        tptr valptr=ptr+ot->objects[j].descr->var.offset;
        if (COMPOSED(etobj) ?
            (etobj->type.anyt.typecateg==T_STR || etobj->type.anyt.typecateg==T_BINARY ||
             etobj->type.anyt.typecateg==T_ARRAY && etobj->type.array.elemtype==MAKE_TOBJ(ATT_CHAR)) :
            is_string(DIRTYPE(etobj)))
        { basicval bval;  bval.ptr=valptr;
          convert_value(cdp, &bval, etobj, buf+len);
        }
        else convert_value(cdp, (basicval*)valptr, etobj, buf+len);
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)buf);
        break;
      }
}

static void list_dbrec(cdp_t cdp, HWND hList, curpos * wbacc)
{ const d_table * td = cd_get_table_d(cdp, wbacc->cursnum, CATEG_TABLE);
  if (!td) return; // list will be empty
  int i;  const d_attr * pattr;
  for (i=1, pattr=ATTR_N(td,1);  i<td->attrcnt;  i++, pattr=NEXT_ATTR(td,pattr))
  { char val[OBJ_NAME_LEN+1+256];
    strcpy(val, pattr->name);  small_string(val, FALSE);  strcat(val, "\t");
    if      (pattr->multi!=1)           strcat(val, "- MULTI -");
    else if (IS_HEAP_TYPE(pattr->type)) strcat(val, "- FLEXIBLE -");
    else 
    { wbacc->attr=i;
      convert_value(cdp, (basicval *)wbacc, MAKE_TOBJ(ATT_UNTYPED | DB_OBJ), val+strlen(val));
    }
    SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)val);
  }
  release_table_d(td);
}

static void list_array(cdp_t cdp, HWND hList, tptr ptr, arraytype * atobj)
{ char buf[10+1+256+2+1];
  typeobj * etobj = atobj->elemtype;
  unsigned elemsize = ipj_typesize(etobj);
  for (int i=0;  i<atobj->elemcount;  i++)      
  { if (i>1000) break;   // protection
    sprintf(buf, "[%d]\t", atobj->lowerbound+i);
    if (COMPOSED(etobj) ?
        (etobj->type.anyt.typecateg==T_STR || etobj->type.anyt.typecateg==T_BINARY ||
         etobj->type.anyt.typecateg==T_ARRAY && etobj->type.array.elemtype==MAKE_TOBJ(ATT_CHAR)) :
        is_string(DIRTYPE(etobj)))
    { basicval bval;  bval.ptr=ptr;
      convert_value(cdp, &bval, etobj, buf+strlen(buf));
    }
    else convert_value(cdp, (basicval*)ptr, etobj, buf+strlen(buf));
    ptr+=elemsize;
    SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)buf);
  }
}

struct t_eval_data
{ cdp_t cdp;
  BOOL server_dbg;
  tptr expr;
};

static void evaluate_in_dlg(HWND hDlg, t_eval_data * evd)
{ char expr[256];  char buf[256+3];  BOOL list, is_data_source=FALSE;
  HWND hList = GetDlgItem(hDlg, CD_EVAL_LIST);
  strcpy(buf, "*");
  GetDlgItemText(hDlg, CD_EVAL_EXPR, expr, sizeof(expr));
  if (evd->server_dbg)
  { list=FALSE;
    if (cd_Server_eval(evd->cdp, 0, expr, buf))
      Get_error_num_text(evd->cdp, cd_Sz_error(evd->cdp), buf, sizeof(buf));
  }
  else
  { typeobj * tobj = evaluate(evd->cdp, expr, TRUE, FALSE);
    if (tobj!=NULL)
    { if (COMPOSED(tobj))
      { uns8 tpcat=tobj->type.anyt.typecateg;
        if (tpcat==T_PTR || tpcat==T_PTRFWD)
        // dereferencing unless NULL
        { if (evd->cdp->RV.stt->ptr==NULL) 
          { list=FALSE;  strcpy(buf, "NIL"); }
          else
          { list=FALSE;
            convert_value(evd->cdp, evd->cdp->RV.stt, MAKE_TOBJ(ATT_INT32), buf);
          }
        }
        else if (tpcat==T_RECORD)
        { SendMessage(hList, LB_RESETCONTENT, 0, 0);
          list_record(evd->cdp, hList, evd->cdp->RV.stt->ptr, &tobj->type.record);
          list=TRUE;
        }
        else if (tpcat==T_ARRAY || tpcat==T_STR || tpcat==T_BINARY)
          if (tobj->type.anyt.valsize>256 || tpcat==T_ARRAY && tobj->type.array.elemtype!=MAKE_TOBJ(ATT_CHAR))
          { SendMessage(hList, LB_RESETCONTENT, 0, 0);
            if (tpcat==T_ARRAY)
              list_array(evd->cdp, hList, evd->cdp->RV.stt->ptr, &tobj->type.array);
            else
            { arraytype atobj;  atobj.elemtype=MAKE_TOBJ(ATT_CHAR);  atobj.lowerbound=1;  
              atobj.elemcount=strlen(evd->cdp->RV.stt->ptr)+1;
              list_array(evd->cdp, hList, evd->cdp->RV.stt->ptr, &atobj);
            }
            list=TRUE;
          }
          else // display as string (valsize <=256 implies #chars <= 255)
          { list=FALSE;
            convert_value(evd->cdp, evd->cdp->RV.stt, tobj, buf);
          }
        else *buf=0;
      }
      else if (tobj==MAKE_TOBJ(ATT_DBREC))
      { SendMessage(hList, LB_RESETCONTENT, 0, 0);
        if (evd->cdp->RV.stt->wbacc.cursnum==NOCURSOR)
        { list=FALSE;
          strcpy(buf, "- Cursor not open -");
        }
        else
        { list_dbrec(evd->cdp, hList, &evd->cdp->RV.stt->wbacc);
          list=TRUE;
        }
      }
      else  // DIRECT type
      { list=FALSE;
        convert_value(evd->cdp, evd->cdp->RV.stt, tobj, buf);
        is_data_source = tobj==MAKE_TOBJ(ATT_TABLE) || tobj==MAKE_TOBJ(ATT_CURSOR);
      }
     // add to the eval history, show history in the combo list:
      in_history(expr, eval_history);
      set_eval_history(GetDlgItem(hDlg, CD_EVAL_EXPR));
    }
    else
    { list=FALSE;
      Get_error_num_text(evd->cdp, cd_Sz_error(evd->cdp), buf, sizeof(buf));
    }
  }
  ShowWindow(GetDlgItem(hDlg, CD_EVAL_VAL),  list ? SW_HIDE : SW_SHOW);
  ShowWindow(hList, list ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hDlg, CD_EVAL_DOWN), list || is_data_source ? SW_SHOW : SW_HIDE);
  if (!list) SetDlgItemText(hDlg, CD_EVAL_VAL, buf);
  else EnableWindow(GetDlgItem(hDlg, CD_EVAL_DOWN), FALSE);  // will be enabled when a row is selected
  EnableWindow(GetDlgItem(hDlg, CD_EVAL_DOEVAL), FALSE);
  EnableWindow(GetDlgItem(hDlg, CD_EVAL_MODIF),  FALSE);
  EnableWindow(GetDlgItem(hDlg, CD_EVAL_WATCH),  !list);
  SetFocus(hDlg);
}

static HWND debug_hwnd;

CFNC DllPrezen void WINAPI closing_client(cdp_t cdp)
{ if (srv_dbg_mode)
  { if (debug_timer) { KillTimer(0, debug_timer);  debug_timer=0; }
    cd_Server_debug(cdp, DBGOP_CLOSE, 0, 0, NULL, NULL, NULL);
    srv_dbg_mode=wbfalse;
  }
}

editor_type * open_project_editor(cdp_t cdp)
/* used only by fsed, activated by message to apl panel */
{ tobjnum objnum;  int i;  char name[OBJ_NAME_LEN+1];
 /* starter program (if any exists) must be the project: */
  if (*cdp->appl_starter && cdp->appl_starter_categ==CATEG_PGMSRC)
  { if (cd_Find_object(cdp, cdp->appl_starter, CATEG_PGMSRC, &objnum)) return 0;
    strcpy(name, cdp->appl_starter);
  }
  else /* look for programs: */
  { comp_dynar * d_comp = &cdp->odbc.apl_programs;
    *name=0;
    for (i=0;  i<d_comp->count();  i++)
    { t_comp * acomp = d_comp->acc(i);
      if ((acomp->flags & CO_FLAGS_OBJECT_TYPE) == 0)   /* a program in the int. lang. */
        if (*name)  /* multiple programs */ return NULL;
        else { strcpy(name, acomp->name);  objnum=acomp->objnum; }
    }
    if (!*name) return NULL;
  }
 /* check rights: */
  if (!check_object_privils(cdp, CATEG_PGMSRC, objnum, TRUE))
    { client_error(cdp, NO_RIGHT);  Signalize();  return NULL; }
 /* look for it, open it: */
  HWND hEd = open_or_activate_editor(cdp, objnum, CATEG_PGMSRC, 0);
  if (!hEd) return NULL;
  return (editor_type*)GetWindowLong(hEd, 0);
}

/* Toto je funkce Search_in_block z baselib.cpp upravena pro potreby doplnovani slov (MI_FSED_COMPLETE_WORD).
   Na rozdil od originalu tato funkce hleda pouze ty vyskyty vzorku, ktere jsou uvedeny na zacatku slov,
   ne uvnitr slov.
   Zmenene casti originalni funkce jsou oznaceny.
*/
static BOOL Complete_word_search_in_block(const char * block, uns32 blocksize,
                         const char * next, const char * patt, uns16 pattsize,
                         uns16 flags, uns32 * respos)
{ unsigned pos, matched;  char c;
  if (flags & SEARCH_BACKWARDS)  /* "next" not used if searching backwards */
  { if (pattsize>blocksize) return FALSE;
	 pos=blocksize-pattsize;
    do
    { matched=0;
      do
      { if (!(flags & SEARCH_CASE_SENSITIVE)) c=upcase_charA(block[pos], cdp->sys_charset);
        else c=block[pos];
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)   if (is_AlphanumA(block[pos-matched-1], cdp->sys_charset)) break;
              if (pos<blocksize) if (is_AlphanumA(block[pos], cdp->sys_charset)) break;
            }
				    *respos=pos-matched;
            // uprava: TRUE se vrati pouze tehdy, kdyz pred nalezenym vyskytem neni znak, ktery by mohl byt soucasti identifikatoru
            if( !is_AlphanumA(block[pos-matched-1], cdp->sys_charset) ) return TRUE;
            // konec upravy
          }
        }
        else break;
      } while (TRUE);
		pos=pos-matched;
    } while (pos--);
  }
  else
  { pos=0;
    while (pos < blocksize)
    { matched=0;
      do
      { if (pos<blocksize) c=block[pos];
        else
          if (next) c=next[pos-blocksize];
          else break;
        if (!(flags & SEARCH_CASE_SENSITIVE)) c=upcase_charA(c, cdp->sys_charset);
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)
                if (is_AlphanumA(block[pos-matched-1], cdp->sys_charset)) break;
                else if (flags & SEARCH_NOCONT) break;
              if (pos<blocksize)
                if (is_AlphanumA(block[pos], cdp->sys_charset)) break;
                else if (next) if (is_AlphanumA(block[pos-blocksize], cdp->sys_charset)) break;
            }
				    *respos=pos-matched;
            // uprava: TRUE se vrati pouze tehdy, kdyz pred nalezenym vyskytem neni znak, ktery by mohl byt soucasti identifikatoru
            if( !is_AlphanumA(block[pos-matched-1], cdp->sys_charset) ) return TRUE;
            // konec upravy
          }
        }
        else break;
      } while (TRUE);
		pos=pos-matched+1;
    }
  }
  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////



/* Theory:
Editablitity: Disabled for table and some other categories definitions and for texts from not-editable views.
 Temporarily disabled for programs during debugging (noedit==2 set for open program editors
 when starting debugging and for new program editors opened during debugging.
Locking: All editable records and temp. noneditable records are write-locked. Locking
 when openning the editor and when saving the fictive record. 
 Writing edited text from fictive record to cache does not mean locking: record remains fictive.

Lines are terminated by CR (soft return) or CR LF (hard return).
Text is stored in editbuf, is length is txtsize, its maximal length is edbsize.
Text is terminated by 0 (not included in txtsize). There is a CR LF before this 0.
  Cursor cannot be positioned on terminating 0.
  Terminating CR LF must not be deleted by Del, CTRL Y, block deletion.

UNDO: Single action may be reversed by multiple undo-actions, e.g. replacing selected block by a char.
  Reformating a block cannot be undone (including the automatic reformat when typing).
  Reading text from a file cannot be undone.
  Flags ACTFL_WAS_UNCHANGED marks the actions whose undoing changes the text into the state synchronized
    with the database. This flags must be removed when saving the text.
  When single user action is stored in multiple items of the undo buffer, all of them except the last
    one are marked with ACTFL_SEQUENCED. Thi flag causes executing them in a single undo-action.

BREAKPOINTS: positions with breakpoints are stored with the editor
  When starting the debugged run, for all modules of the program their editors are searched and
  their breakpoints are created in the code.
  When toggling BP during debugging, both code BP and editor BP are toggled.
  During debugging, code BVs are shown, editor BPs are shown otherwise.
  Problems: editor BPs can be set on empty lines, but they are invisible.

TOOLTIPS: Created for every editor window (shared by editors in editor container). 
  Return text only in debug mode. Functions are evaluated when showing the tip!

*/
#include "dynar.cpp"

#endif

void editor_type::remove_tools_internal(void)
{
#if WBVERS>=1000
 // remove tools from the toolbar of the container
  if (in_edcont && the_editor_container)
    if (global_style!=ST_MDI)
      if (the_editor_container->ed_owning_tools==this)
      { wxToolBarBase * tb = my_toolbar();
        if (tb)
          while (tb->GetToolsCount()) 
            tb->DeleteToolByPos(0);
      }

#endif
  remove_tools();
}
                  
void editor_type::remove_tools(void)
{
  if (pending_find_dialog   ) pending_find_dialog   ->Destroy();
  if (pending_replace_dialog) pending_replace_dialog->Destroy();
  competing_frame::remove_tools();
}

void editor_type::Activated(void)
{ 
#if WBVERS>=1000  // is necessary, when called from activation of the page. Bliking, when focusing the editor container
  if (in_edcont)
    if (global_style!=ST_MDI)
      if (the_editor_container->ed_owning_tools!=this)   // prevents toolbar blinking when debugging
      { wxToolBarBase * tb = my_toolbar();
        if (tb)
        { while (tb->GetToolsCount()) 
            tb->DeleteToolByPos(0);
          create_tools(tb, get_tool_info());
          update_toolbar();
          the_editor_container->ed_owning_tools=this;
        }
      }
#endif
  any_designer::Activated();
}
/////////////////////////////// editor for text objects stored in the database ////////////////////

//bool editor_type::save_design(bool save_as) -- is in subclasses

bool editor_type::IsChanged(void) const
{ return _changed; }

wxString editor_type::SaveQuestion(void) const
{ return _("The changes have not been saved.\nDo you want to save the changes?"); }

void editor_type::OnCommand(wxCommandEvent & event)
// Processing command from main menu, popup menu, the editor container.
{
  switch (event.GetId())
  { case TD_CPM_SAVE:
      if (!IsChanged()) break;  // no message if no changes
      if (!save_design(false)) break;  // error saving, break
      wxGetApp().frame->SetStatusText(_("Saved."), 0);
      break;
    case TD_CPM_SAVE_AS:
      if (!save_design(true)) break;  // error saving, break
      info_box(_("Your design was saved under a new name."), dialog_parent());
      break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
    case TD_CPM_EXIT_FRAME:
      OnCloseRequest(false);  
      break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
      if (in_edcont)
      { wxCommandEvent cmd;  cmd.SetId(TD_CPM_EXIT_UNCOND);
        the_editor_container->OnCommand(cmd);
      }
      else
      { exit_request(this, false);
        DoClose();
      }
      break;
    case TD_CPM_CHECK:
      if (Compile()) info_box(_("Syntax is correct."), dialog_parent());
      break;
    case TD_CPM_RUN:
      Run();  break;
    case TD_CPM_OPTIM:
    { wxString uni(editbuf);
      wxCharBuffer bytes = uni.mb_str(*cdp->sys_conv);
      QueryOptimizationDlg dlg(cdp, bytes);
      dlg.Create(dialog_parent());
      dlg.ShowModal();
      break;
    }
    case TD_CPM_UNDO:
      undo_action();  break;
    case TD_CPM_REDO:
      redo_action();  break;
    case TD_CPM_HELP:
      OnHelp();  break;
    case TD_CPM_CUT:        // commands from the main toolbar (menu)
      OnCut();  break;
    case TD_CPM_COPY:       // commands from the main toolbar (menu)
      OnCopy();  break;
    case TD_CPM_PASTE:      // commands from the main toolbar (menu)
      OnPaste();  break;
    case TD_CPM_UNINDENT_BLOCK:
      OnUnindentBlock();  break;
    case TD_CPM_INDENT_BLOCK:
      OnIndentBlock();  break;
    case TD_CPM_FIND:
      OnFindReplace(false);  break;
    case TD_CPM_REPLACE:
      OnFindReplace(true);  break;
    case TD_CPM_FIND_ALL:
    { wxString word = read_current_word();
      SearchObjectsDlg dlg(cdp, word, cdp->sel_appl_name, NULL);
      dlg.Create(this);
      dlg.ShowModal();
      break;
    }
    case TD_CPM_OPEN:
    { wxString word = read_current_word();
      tcateg cat;  tobjnum objnum;
      if (cd_Find_object(cdp, word.mb_str(*cdp->sys_conv), cat=CATEG_PGMSRC, &objnum))
      if (cd_Find_object(cdp, word.mb_str(*cdp->sys_conv), cat=CATEG_PROC,   &objnum))
      if (cd_Find_object(cdp, word.mb_str(*cdp->sys_conv), cat=CATEG_TRIGGER,&objnum))
      if (cd_Find_object(cdp, word.mb_str(*cdp->sys_conv), cat=CATEG_TABLE,  &objnum))
      if (cd_Find_object(cdp, word.mb_str(*cdp->sys_conv), cat=CATEG_CURSOR, &objnum))
      if (cd_Find_object(cdp, word.mb_str(*cdp->sys_conv), cat=CATEG_MENU,   &objnum))
      if (cd_Find_object(cdp, word.mb_str(*cdp->sys_conv), cat=CATEG_STSH,   &objnum))
        { cd_Signalize(cdp);  break; }
      if (!check_not_encrypted_object(cdp, cat, objnum)) break;
      open_text_object_editor(cdp, cat==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_DEF_ATR, cat, "", OPEN_IN_EDCONT);
      break;
    }
    case TD_CPM_EVAL:
    { EvalDlg dlg(cdp, read_current_word());  
      dlg.Create(wxGetApp().frame);  dlg.ShowModal();
      break;
    }
    case TD_CPM_WATCH:
      the_server_debugger->watch_view->append(read_current_word(), 0);  break;
    case TD_CPM_TOGGLE_BOOKMARK:
      BookmarkToggle(cur_line_a);  break;
    case TD_CPM_NEXT_BOOKMARK:
      BookmarkNext();  break;
    case TD_CPM_PREV_BOOKMARK:
      BookmarkPrev();  break;
    case TD_CPM_CLEAR_BOOKMARKS:
      BookmarkClearAll();  break;
    case TD_CPM_TOGGLE_BREAKPOINT:
      if (the_server_debugger && the_server_debugger->cdp==cdp)
        the_server_debugger->ToggleBreakpoint(this, cur_line_n);
      else
        ToggleBreakpointLocal(cur_line_n);
      break;
    case TD_CPM_PROFILE:
      toggle_profile_mode();
      break;
  }
}

void editor_type::set_designer_caption(void)
{ if (in_edcont)
    the_editor_container->SetPageText(the_editor_container->GetSelection(), get_title(false)); 
  else 
    set_caption(get_title(true));
}

wxString text_object_editor_type::get_title(bool long_title)
{ wxString caption;
  if (long_title)
  { if (!modifying()) 
      caption = category==CATEG_CURSOR ? _("Design a New Query") : 
                category==CATEG_STSH   ? _("Design a New Style Sheet") : 
                                         _("Design a New Transport");
    else 
    { caption.Printf(category==CATEG_PROC    ? _("Editing Procedure %s") :
                     category==CATEG_TRIGGER ? _("Editing Trigger %s") : 
                     category==CATEG_STSH    ? _("Editing Style Sheet %s") : 
                     category==CATEG_TABLE   ? _("Table %s") :    // opened from the search results
                     category==CATEG_DRAWING ? _("Diagram %s") :  // opened from the search results
                     category==CATEG_SEQ     ? _("Sequence %s") : // opened from the search results
                     category==CATEG_DOMAIN  ? _("Domain %s") :   // opened from the search results
                     category==CATEG_INFO    ? _("Fulltext %s") : // opened from the search results
                     category==CATEG_CURSOR  ? _("Editing Query %s") : _("Editing Transport  %s"), 
                     wxString(objname, *cdp->sys_conv).c_str());
      if (IsChanged()) caption=caption+wxT(" *");
    }
  }
  else  // short title
  { tobjname nicename;
    if (!modifying()) strcpy(nicename, wxString(_("Unnamed")).mb_str(*cdp->sys_conv));
    else strcpy(nicename, objname);
    wb_small_string(cdp, nicename, true);
    caption=wxString(nicename, *cdp->sys_conv);
    if (IsChanged()) caption=caption+wxT(" *");
  }
  return caption;
}

wxString CLOB_editor_type::get_title(bool long_title)
{ wxString caption;
  if (long_title)
    caption.Printf(_("CLOB text %u"), objnum);
  else
  { caption.Printf(_("CLOB %u"), objnum);
    if (IsChanged()) caption=caption+wxT(" *");
  }
  return caption;
}

t_tool_info * text_object_editor_type::get_tool_info(void) const
{ return category==CATEG_PROC || category==CATEG_CURSOR || (category==CATEG_PGMSRC && (/*is_dad ||*/ !(flags & CO_FLAGS_OBJECT_TYPE))) ?
           txed_tool_info : // Run is available
         category==CATEG_TRIGGER || category==CATEG_PGMSRC ? 
           trigger_txed_tool_info : // no Run
           CLOB_editor_type::CLOB_tool_info;  // no compile: used for XML forms a and style sheets too
}

void replaceCRbyLF(wxChar * buf)
// replaces CRs by LFs (old texts from version 8.1):
{
  while (*buf)
  { if (*buf==0xd && buf[1]>=' ') *buf=0xa;
    buf++;
  }  
}

bool editor_type::load(uns32 len)
// Just loads the CLOB, without any conversion
{ if (objnum==NOOBJECT) 
    len=0;
  else if (cd_Read_var(cdp, cursnum, objnum, attr, NOINDEX, 0, len, editbuf, &len))
    { cd_Signalize(cdp);  return false; }
  *(wchar_t*)(((char*)editbuf)+len)=0;  // wide terminator
  return true;
}

bool text_object_editor_type::load(uns32 len)
{ 
 // read length, alloc buffer ad load:
  if (objnum!=NOOBJECT) cd_Read_len(cdp, cursnum, objnum, attr, NOINDEX, &len);
  if (!alloc_buffer(len + 300000)) 
    return false;
  if (!editor_type::load(len))
    return false;
 // convert text to unicode using sys_conv:
  char * bytes = (char*)editbuf;
  if (!alloc_buffer(len + 300000)) return false;
  cdp->sys_conv->MB2WC(editbuf, bytes, edbsize);  // buffer size must be in wide characters
  corefree(bytes);
  txtsize=(unsigned)wcslen(editbuf);  // length in characters
 // replace CRs by LFs (old texts from version 8.1):
  replaceCRbyLF(editbuf);
 // read the object name:
  cd_Read(cdp, cursnum, objnum, OBJ_NAME_ATR, NULL, objname);
  return true;
}

bool CLOB_editor_type::load(uns32 len)
{// find "specif":
  const d_table * td = cd_get_table_d(cdp, cursnum, CATEG_TABLE);
  if (td && attr<td->attrcnt)
  { specif=td->attribs[attr].specif;
    release_table_d(td);
  }
 // read length, alloc buffer ad load:
  if (objnum!=NOOBJECT) cd_Read_len(cdp, cursnum, objnum, attr, NOINDEX, &len);
  txtsize = specif.wide_char ? len/sizeof(wuchar) : len;  // length in characters
  if (specif.wide_char) len &= 0x7ffffffe;  // sometimes the length is odd, but the terminating zero must be on an even address
  if (!alloc_buffer(txtsize + 300000)) 
    return false;
  if (!editor_type::load(len))
    return false;
 // convert narrow text (or wuc on Linux) to Unicode using [specif]:
  wxString res;
  res.GetWriteBuf(txtsize+1);  // length must not be 0!
  int err=superconv(ATT_STRING, ATT_STRING, editbuf, (char*)res.c_str(), specif, wx_string_spec, NULL);
  res.UngetWriteBuf();
  if (err<0) return false;
  wcscpy(editbuf, res.c_str());
  txtsize=(unsigned)wcslen(editbuf);  // update the length in characters
 // replace CRs by LFs (old texts from version 8.1):
  replaceCRbyLF(editbuf);
  return true;
}

bool CLOB_editor_type::save_design(bool save_as)  // saving CLOB
{
  if (!_changed || noedit) return true;  // noedit is a fuse only
 // convert Unicode to a narrow text (or wuc on Linux) using [specif]:
  wxString res;
  res.GetWriteBuf(txtsize+1);  // length must not be 0!
  specif.length=0;  // must not limit the length to 0xffff!
  int err=superconv(ATT_STRING, ATT_STRING, editbuf, (char*)res.c_str(), wx_string_spec, specif, NULL);
  res.UngetWriteBuf();
  if (err<0) return false;
  const void * p = res.c_str();
  int savelen = specif.wide_char ? (int)sizeof(wuchar)*(int)wuclen((wuchar*)p) : (int)strlen((char*)p);
  if (cd_Write_var(cdp, cursnum, objnum, attr, NOINDEX, 0, savelen, p) ||
      cd_Write_len(cdp, cursnum, objnum, attr, NOINDEX, savelen))
    { cd_Signalize2(cdp, dialog_parent());  return false; }
  saved_now();
  _changed=false;  ch_status(false);
 // send notifications to all frames:
  RecordUpdateEvent ruev(cdp, cursnum, recnum);
  for (competing_frame * afrm = competing_frame_list;  afrm;  afrm=afrm->next)
    if (afrm->cf_style!=ST_EDCONT)
      afrm->OnRecordUpdate(ruev);  // this updates child grids, too
  return true;
}


bool text_object_editor_type::save_design(bool save_as)  // saving application object
{ bool adding_new_object = false;
  if (category==CATEG_PROC || category==CATEG_TRIGGER) save_as=false; // fuse
  if (!save_as)
    if (!_changed || noedit) return true;  // noedit is a fuse only
  const char * save_objdef;
  wxString uni(editbuf);
  wxCharBuffer bytes = uni.mb_str(*cdp->sys_conv);
  save_objdef = bytes;
  if (*editbuf && (!save_objdef || !*save_objdef))  // conversion error
  { error_box(_("The text is not convertible to the SQL Server charset."), dialog_parent());  
    return false;
  }
  if (!modifying() || save_as)
  {// Procedures and triggers are created by wizards, not by this editor
#ifdef STOP // provlems with leading commnet and object descriptor
    if (categ==CATEG_PROC || categ==CATEG_TRIGGER) // Save As disabled
    { memmov(editbuf+7, editbuf, txtsize+1);
      memcpy(editbuf, "CREATE ", 7);
      trecnum recnum;
      err = cd_SQL_execute(cdp, editbuf, &recnum);
      memmov(editbuf, editbuf+7, txtsize+1);
      if (!err) 
      { objnum=(tobjnum)recnum;
        cd_Read(cdp, OBJ_TABLENUM, objnum, OBJ_NAME_ATR, NULL, text_name);
        if (!cd_Write_lock_record(cdp, OBJ_TABLENUM, objnum)) object_locked=wbtrue;
        set_editor_caption();   /* add program name into the caption */
        added=TRUE;
        goto ok;
      }
      prompt = categ==CATEG_PROC ? NEW_PROC_NAME : NEW_TRIGGER_NAME;
#endif
    tobjnum new_objnum;
    if (!edit_name_and_create_object(cdp, dialog_parent(), schema_name, category, 
           category==CATEG_CURSOR ? _("Enter the name of the new query:") :
           category==CATEG_STSH   ? _("Enter the name of the new style sheet:") : 
           _("Enter the name of the new program:"), 
           &new_objnum, objname)) 
      return false;  // cancelled
    unlock_the_object();  // SaveAs needs this!
    recnum=objnum=new_objnum;
    SetFocus();
    lock_the_object();
    if (cd_Write_var(cdp, OBJ_TABLENUM, objnum, attr, NOINDEX, 0, txtsize, save_objdef) ||
        cd_Write_len(cdp, OBJ_TABLENUM, objnum, attr, NOINDEX, txtsize))
      { cd_Signalize2(cdp, dialog_parent());  return false; }
    adding_new_object=true;
  }
  else // changing existing object
  { if (category==CATEG_TRIGGER) cd_Trigger_changed(cdp, objnum, FALSE);  // unregistering the original trigger
   /* save text: */
#ifdef STOP
    if (categ==CATEG_PROC || categ==CATEG_TRIGGER)
    { memmov(editbuf+7, editbuf, txtsize+1);
      memcpy(editbuf, "ALTER  ", 7);
      err = cd_SQL_execute(cdp, editbuf, NULL);
      memmov(editbuf, editbuf+7, txtsize+1);
    }
    if (categ!=CATEG_PROC && categ!=CATEG_TRIGGER || err)
#endif
    if (cd_Write_var(cdp, OBJ_TABLENUM, objnum, attr, NOINDEX, 0, txtsize, save_objdef) ||
        cd_Write_len(cdp, OBJ_TABLENUM, objnum, attr, NOINDEX, txtsize))
      { cd_Signalize2(cdp, dialog_parent());  return false; }
   // SQL server notification:
    if (category==CATEG_PROC) cd_Procedure_changed(cdp);
    else if (category==CATEG_TRIGGER) cd_Trigger_changed(cdp, objnum, TRUE);  // registering the new/changed trigger
  }
 /* store the new component and refill apl-panel: */
  if (adding_new_object)
  { Set_object_folder_name(cdp, objnum, category, folder_name);
    Add_new_component(cdp, category, schema_name, objname, objnum, 0, folder_name, true);
    set_designer_caption();
  }
  else 
  { 
#ifdef STOP  // no need to change existing flags
    int flags;
    if (category==CATEG_PGMSRC)
    { if (is_dad) flags = CO_FLAG_XML;
      else  // for DAD flags are never changed
      { flags=program_flags(cdp, editbuf);
        Set_object_flags(cdp, objnum, category, flags);
      }
    }
    else flags=0;
#endif
    component_change_notif(cdp, category, objname, schema_name, flags, objnum);
  }

 /* delete code if edititng program source, invalidate cursor descriptor: */
  if (category==CATEG_PGMSRC && !is_dad)
  { tobjnum codeobj;
    if (!cd_Find2_object(cdp, objname, NULL, CATEG_PGMEXE, &codeobj))  /* compiled code deleted */
      cd_Write_len(cdp, OBJ_TABLENUM, (trecnum)codeobj, OBJ_DEF_ATR, NOINDEX, 0);
   // free the project unless used by running object:
    if (cdp->RV.global_state!=PROG_RUNNING && cdp->RV.global_state!=MENU_RUNNING &&
        cdp->RV.global_state!=VIEW_RUNNING)
      free_project(cdp, FALSE);
  }
  if (category==CATEG_CURSOR)
    inval_table_d(cdp, objnum, CATEG_CURSOR);

  saved_now();
  _changed=false;  ch_status(false);
  if (category==CATEG_PROC || category==CATEG_TRIGGER)
  { uns32 state;  uns8 del;
    cd_Get_server_info(cdp, OP_GI_PROFILING_LINES, &state, sizeof(state));
    if (state!=0) // profiling lines
    {// check if the profile is empty:
      tcursnum cursnum;  trecnum cnt=0;  bool empty=true;
      if (!cd_Open_cursor_direct(cdp, "select * from _iv_profile where line_number is not NULL", &cursnum))
      { cd_Rec_cnt(cdp, cursnum, &cnt);
        for (trecnum r=0;  empty && r<cnt;  r++)
          if (!cd_Read(cdp, cursnum, r, DEL_ATTR_NUM, NULL, &del) && !del)
            empty=false;
        cd_Close_cursor(cdp, cursnum);
      }
     // clear the profile, if it contains line numbers:
      if (!empty)
      { if (!cd_SQL_execute(cdp, "call _sqp_profile_reset()", NULL))
          info_box(_("The execution profile has been cleared"), dialog_parent());
        if (profile_mode) toggle_profile_mode();
      }
    }
  }
  return true;
}

//////////////////////////////////////////// compile & run /////////////////////////////////////////////
bool text_object_editor_type::Compile(void)
// If OK, returns true; otherwise reports the error, shows the position and returns false.
{ bool result = true;
#if 0
  if (cdp->RV.global_state!=DEVELOP_MENU)  /* menu editor may be open */
    { unpbox(MAKEINTRESOURCE(NOT_IN_MAIN_STATE));  return; }
#endif
  switch (category)
  { 
    case CATEG_PGMSRC:
    { if (is_dad)
      { int line, column;
      //wxString uni(editbuf);
      //wxCharBuffer bytes = uni.mb_str(*cdp->sys_conv);
      // must convert to UTF8
        int len = (int)wcslen(editbuf);
        char * buf=(char*)corealloc(2*len+1, 78);
        superconv(ATT_STRING, ATT_STRING, editbuf, buf, wx_string_spec, t_specif(0,7,0,0), NULL);
        if (!link_Verify_DAD(cdp, buf, &line, &column))
        { if (line!=-1 && column!=-1) position_on_line_column(line, column);
          cd_Signalize2(cdp, dialog_parent());
          SetFocus();
          result=false;
        }
        corefree(buf);
      }
      else if (!(flags & CO_FLAGS_OBJECT_TYPE))  // unless a data transport
      { if (!save_design(false)) return false;  /* no run if cannot write text */
        int res=set_project_up(cdp, objname, objnum, TRUE, TRUE);
        if (res) return false;  /* the right window already focused */
      }
      break;
    }
    case CATEG_PROC:  case CATEG_TRIGGER:  case CATEG_CURSOR:
    { 
      wxString uni(editbuf);
      uni=wxT(" ")+uni;
      wxCharBuffer bytes = uni.mb_str(*cdp->sys_conv);
      bytes.data()[0]=(char)QUERY_TEST_MARK;
      if (category==CATEG_PROC && !stricmp(objname, MODULE_GLOBAL_DECLS)) bytes.data()[0]++;
      BOOL res = cd_SQL_execute(cdp, bytes, NULL);
      if (res)
      { cd_Signalize2(cdp, dialog_parent());
        //SetFocus();   /* restore focus on editor window */
        uns32 line;  uns16 column;  char buf[PRE_KONTEXT+POST_KONTEXT+2];
        if (!cd_Get_error_info(cdp, &line, &column, buf) && *buf)
          position_on_line_column(line, column);
        result=false;
      }
      break;
    }
    default:
      wxBell();  break;
  }
  return result;
}

bool text_object_editor_type::Run(void)
{ bool result = true;
  switch (category)
  { case CATEG_PROC:
    { if (!stricmp(objname, MODULE_GLOBAL_DECLS))  // cannot run
        { error_box(_("The global declarations cannot be executed."));  break; }
      if (!Compile()) break;  // is is better to report compilation error right now and not show the dialog for parameters
      if (!save_design(false)) break;  // error saving, break
      call_routine(this, cdp, objnum);
      break;
    }
    case CATEG_CURSOR:
    { tcursnum curs;
      wxString uni(editbuf);
      wxCharBuffer bytes = uni.mb_str(*cdp->sys_conv);
      if (cd_Open_cursor_direct(cdp, bytes, &curs)) cd_Signalize(cdp);
      else
        open_data_grid(NULL, cdp, CATEG_DIRCUR, curs, "", false, NULL);
      break;
    }
    case CATEG_PGMSRC:
      if ((flags & CO_FLAGS_OBJECT_TYPE))  // data transport or xml transport: run not allowed
        break;
      if (!save_design(false)) break;  // error saving, break
      run_pgm9(cdp, objname, objnum, FALSE);
      break;
    default:
      wxBell();  break;
  }
  return result;
}

#include "bmps/find_text.xpm"
#include "bmps/replace.xpm"

t_tool_info text_object_editor_type::txed_tool_info[TXED_TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,    File_save_xpm, wxTRANSLATE("Save design")),
  t_tool_info(TD_CPM_EXIT,    exit_xpm,      wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CUT,     cut_xpm,       wxTRANSLATE("Cut Text")),
  t_tool_info(TD_CPM_COPY,    Kopirovat_xpm, wxTRANSLATE("Copy Text")),
  t_tool_info(TD_CPM_PASTE,   Vlozit_xpm,    wxTRANSLATE("Paste Text")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_FIND,    find_text_xpm, wxTRANSLATE("Find")),
  t_tool_info(TD_CPM_REPLACE, replace_xpm,   wxTRANSLATE("Replace")),
  t_tool_info(TD_CPM_UNDO,    Undo_xpm,      wxTRANSLATE("Undo")),
  t_tool_info(TD_CPM_REDO,    Redo_xpm,      wxTRANSLATE("Redo")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CHECK,   compile_xpm,   wxTRANSLATE("Compile the Object")),
  t_tool_info(TD_CPM_RUN,     run2_xpm,      wxTRANSLATE("Start Execution")),
  t_tool_info(TD_CPM_PROFILE, profile_xpm,   wxTRANSLATE("Display Profile"), wxITEM_CHECK),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_HELP,    help_xpm,      wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

t_tool_info text_object_editor_type::trigger_txed_tool_info[TRIGGER_TXED_TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,    File_save_xpm, wxTRANSLATE("Save design")),
  t_tool_info(TD_CPM_EXIT,    exit_xpm,      wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CUT,     cut_xpm,       wxTRANSLATE("Cut Text")),
  t_tool_info(TD_CPM_COPY,    Kopirovat_xpm, wxTRANSLATE("Copy Text")),
  t_tool_info(TD_CPM_PASTE,   Vlozit_xpm,    wxTRANSLATE("Paste Text")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_FIND,    find_text_xpm, wxTRANSLATE("Find")),      
  t_tool_info(TD_CPM_REPLACE, replace_xpm,   wxTRANSLATE("Replace")),
  t_tool_info(TD_CPM_UNDO,    Undo_xpm,      wxTRANSLATE("Undo")),
  t_tool_info(TD_CPM_REDO,    Redo_xpm,      wxTRANSLATE("Redo")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CHECK,   compile_xpm,   wxTRANSLATE("Compile")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_HELP,    help_xpm,      wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

t_tool_info CLOB_editor_type::CLOB_tool_info[CLOB_TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,    File_save_xpm, wxTRANSLATE("Save design")),
  t_tool_info(TD_CPM_EXIT,    exit_xpm,      wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CUT,     cut_xpm,       wxTRANSLATE("Cut Text")),
  t_tool_info(TD_CPM_COPY,    Kopirovat_xpm, wxTRANSLATE("Copy Text")),
  t_tool_info(TD_CPM_PASTE,   Vlozit_xpm,    wxTRANSLATE("Paste Text")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_FIND,    find_text_xpm, wxTRANSLATE("Find")),
  t_tool_info(TD_CPM_REPLACE, replace_xpm,   wxTRANSLATE("Replace")),
  t_tool_info(TD_CPM_UNDO,    Undo_xpm,      wxTRANSLATE("Undo")),
  t_tool_info(TD_CPM_REDO,    Redo_xpm,      wxTRANSLATE("Redo")),
  t_tool_info(0,  NULL, NULL)
};

bool text_object_editor_type::analyse_object(void)
// If cannot lock the object then if [auto_read_only] then open it in the read-only mode, otherwise ask the user.
{// text object specific prepare:
  is_dad=false;
  if (objnum==NORECNUM) noedit=0;
  else
  { if (!check_not_encrypted_object(cdp, category, objnum))
      return false;
   // decide is the category allows editing the object:
    noedit = category==CATEG_TABLE || category==CATEG_SEQ || category==CATEG_DRAWING || category==CATEG_RELATION || 
             category==CATEG_REPLREL || category==CATEG_DOMAIN || category==CATEG_INFO ? 1 : 0;
    cd_Read(cdp, cursnum, objnum, OBJ_FLAGS_ATR, NULL, &flags);
    if (category==CATEG_PGMSRC) 
    { if      ((flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_TRANSPORT) noedit = 0;  // transport object - allowed in 9.5
      else if ((flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_XML) is_dad = true;
      else noedit = /*cdp->RV.global_state==DEBUG_BREAK ? 2 :*/  0;
    }
    if (the_server_debugger)  // new source opened during the debugging session
      if (noedit==0) 
        noedit=2;
   // locking:
    if (noedit!=1)
    { if (!lock_the_object())  // cannot lock
      { if (!auto_read_only)
        { wx602MessageDialog md(wxGetApp().frame, _("The object is being edited by another user or you are not allowed to overwrite this object. Open in the object in read-only mode?"), STR_WARNING_CAPTION, wxYES_NO);
          if (md.ShowModal() == wxID_NO) return false;
        }
        read_only_mode = true;  // disables another attempt to lock the object when the designer is being opened
        noedit = 1;
      }
    }
  }
  return true;
}

static void add_editor_menu_items(wxMenu * designer_menu)
{ designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_FIND,           _("&Find\tCtrl+F"),       find_text_xpm);
  AppendXpmItem(designer_menu, TD_CPM_REPLACE,        _("&Replace\tCtrl+H"),    replace_xpm);
  AppendXpmItem(designer_menu, TD_CPM_UNDO,           _("Undo change\tCtrl+Z"), Undo_xpm);
  AppendXpmItem(designer_menu, TD_CPM_REDO,           _("Redo change\tCtrl+Y"), Redo_xpm);
  designer_menu->AppendSeparator();
  AppendXpmItem(designer_menu, TD_CPM_INDENT_BLOCK,   _("&Indent Block\t<Tab>"));
  AppendXpmItem(designer_menu, TD_CPM_UNINDENT_BLOCK, _("&Unindent Block\tShift+Tab"));

  wxMenu * bkm_menu = new wxMenu;
  AppendXpmItem(bkm_menu, TD_CPM_TOGGLE_BOOKMARK,     _("&Toggle\tCtrl+F12"));
  AppendXpmItem(bkm_menu, TD_CPM_NEXT_BOOKMARK,       _("Go to &Next\tF12"));
  AppendXpmItem(bkm_menu, TD_CPM_PREV_BOOKMARK,       _("Go to &Previous\tShift+F12"));
  AppendXpmItem(bkm_menu, TD_CPM_CLEAR_BOOKMARKS,     _("&Clear All\tCtrl+Shift+F12"));
  wxMenuItem * mi = new wxMenuItem(designer_menu, TD_CMP_BOOKMARKS, _("&Bookmarks"), wxEmptyString, wxITEM_NORMAL, bkm_menu);
  mi->SetBitmap(empty_xpm);
  designer_menu->Append(mi);
}

wxMenu * text_object_editor_type::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items: 
  any_designer::get_designer_menu();  
 // disable Save As for most categories:
  designer_menu->Enable(TD_CPM_SAVE_AS, category==CATEG_CURSOR || category==CATEG_PGMSRC || category==CATEG_STSH);
 // update menu item label for the compilation:
  designer_menu->SetLabel(TD_CPM_CHECK, _("&Compile\tF7"));
  designer_menu->Enable(TD_CPM_CHECK, category==CATEG_PROC || category==CATEG_CURSOR || category==CATEG_TRIGGER || category==CATEG_PGMSRC /*&& (is_dad || !(flags & CO_FLAGS_OBJECT_TYPE))*/);
 // update shortcut in the Close menu item (Ctrl F4 closes all):
  designer_menu->SetLabel(TD_CPM_EXIT, _("Close\tCtrl+W"));
 // add specific items:
  if (category==CATEG_PROC || category==CATEG_CURSOR ||
      category==CATEG_PGMSRC && !(flags & CO_FLAGS_OBJECT_TYPE))
    AppendXpmItem(designer_menu, TD_CPM_RUN,  _("&Execute\tF5"), run2_xpm);
  if (category==CATEG_CURSOR) 
    AppendXpmItem(designer_menu, TD_CPM_OPTIM,  _("&Optimize"));
  if (category==CATEG_PROC)
  { designer_menu->AppendCheckItem(TD_CPM_PROFILE, _("&Display Profile")/*, profile_xpm*/);
    designer_menu->Check(TD_CPM_PROFILE, false);
  }
  add_editor_menu_items(designer_menu);
  return designer_menu;
}

wxMenu * CLOB_editor_type::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items: 
  any_designer::get_designer_menu();  
 // disable Save As for most categories:
  designer_menu->Enable(TD_CPM_SAVE_AS, false);
 // disable menu item label for the compilation:
  designer_menu->Enable(TD_CPM_CHECK, false);
 // update shortcut in the Close menu item (Ctrl F4 closes all):
  designer_menu->SetLabel(TD_CPM_EXIT, _("Close\tCtrl+W"));
  add_editor_menu_items(designer_menu);
  return designer_menu;
}

void editor_type::update_designer_menu(void)
{ bool en = !noedit && block_on && block_start < block_end;
  designer_menu->Enable(TD_CPM_INDENT_BLOCK, en);
  designer_menu->Enable(TD_CPM_UNINDENT_BLOCK, en);
  designer_menu->Enable(TD_CPM_SAVE, !noedit);
  designer_menu->Enable(TD_CPM_REPLACE, !noedit);
  designer_menu->Enable(TD_CPM_UNDO, can_undo());
  designer_menu->Enable(TD_CPM_REDO, can_redo());
}

void text_object_editor_type::update_designer_menu(void)
{
  editor_type::update_designer_menu();
  designer_menu->Enable(TD_CPM_CHECK, !the_server_debugger);
  if (category==CATEG_PROC)
    designer_menu->Enable(TD_CPM_RUN, !the_server_debugger);
}
///////////////////////////////////// editor container ////////////////////////////
//#include "bmps/_modif.xpm"

editor_container::editor_container(void) :
    any_designer(NULL, NOOBJECT, "", "", (tcateg)-1, source_xpm, &editor_cont_coords)
{ adding_page=false;  ed_owning_tools=NULL; }

// the_editor_container has multiple inheritance. 
// Gnu cannot call its member in EVT_NOTEBOOK_PAGE_CHANGED() and Windows crashes. wxSpecEvtHandler created instead.

class wxSpecEvtHandler : public wxEvtHandler
{public:
  void OnPageActivated(wxNotebookEvent & event);
};

void wxSpecEvtHandler::OnPageActivated(wxNotebookEvent & event)
{ 
#ifdef WINS 
 // there is an error in wxNotebook::OnSelChange - SetFocus is called before the current selection is changed 
 // and the invisible page obtains the focus. I corrent this here by explicit calling the SetFocus.
 // Errorneus OnSelChange must be called BEFORE SetFocus, not after it via event.Skip()!
  the_editor_container->OnSelChange(event);  
#endif
 // toolbar needs the deactivation to be first 
  int old_sel = event.GetOldSelection();
  if (old_sel!=-1 && old_sel<the_editor_container->GetPageCount())  // test needed in wx 2.8.0.
  { editor_type * ed = (editor_type*)the_editor_container->GetPage(old_sel);
    ed->remove_tools_internal();
  }
  int new_sel = event.GetSelection();
  if (new_sel!=-1 && new_sel<the_editor_container->GetPageCount())  // test needed in wx 2.8.0.
  { editor_type * ed = (editor_type*)the_editor_container->GetPage(new_sel);
    ed->Activated();
    ed->SetFocus(); 
  }
#ifndef WINS  
  event.Skip();  // this passes the focus to the previously activated page
#endif  
}

BEGIN_EVENT_TABLE( editor_container, wxNotebook)
  EVT_NOTEBOOK_PAGE_CHANGED(-1, wxSpecEvtHandler::OnPageActivated)
  EVT_SET_FOCUS(editor_container::OnSetFocus)
  EVT_RIGHT_DOWN(editor_container::OnRightDown) 
END_EVENT_TABLE()

bool editor_container::IsChanged(void) const
  { return ((editor_type*)((editor_container*)this)->GetPage(GetSelection()))->IsChanged(); }
wxString editor_container::SaveQuestion(void) const
  { return ((editor_type*)((editor_container*)this)->GetPage(GetSelection()))->SaveQuestion(); }
bool editor_container::save_design(bool save_as)
  { return ((editor_type*)GetPage(GetSelection()))->save_design(save_as); }
t_tool_info * editor_container::get_tool_info(void) const
  { return !GetPageCount() ? NULL : ((editor_type*)((editor_container*)this)->GetPage(GetSelection()))->get_tool_info(); }

void editor_container::OnSetFocus(wxFocusEvent & event)
{ int sel = GetSelection();
  event.Skip();
  if (sel==-1) return;
#ifdef LINUX
  if (sel>=GetPageCount()) return;
#endif
  ((editor_type*)GetPage(sel))->SetFocus();
}

void editor_container::Activated(void)
{ if (adding_page) return;  // AddPage->FocusPage->ActivateContainer->Activate previous page wrong!
  int sel = GetSelection();
  if (sel==-1) return;
  ((editor_type*)GetPage(sel))->Activated();
}

void editor_container::Deactivated(void)
{ int sel = GetSelection();
  if (sel==-1) return;
#ifdef LINUX
  if (sel>=GetPageCount()) return;
#endif
  ((editor_type*)GetPage(sel))->Deactivated();
}

void editor_container::OnCommand(wxCommandEvent & event)
{ 
  if (event.GetId()==TD_CPM_EXIT_FRAME)
  { ServerDisconnectingEvent event(NULL);
    the_editor_container->OnDisconnecting(event);
    if (!event.cancelled)
      //the_editor_container_frame->Destroy();-- does not save the coords
      destroy_designer(the_editor_container_frame, &editor_cont_coords);
    return;
  }

  int pg = GetSelection();
  if (pg!=-1)
  { editor_type * ed = (editor_type*)GetPage(pg);
    if (event.GetId()==TD_CPM_EXIT)
    { if (ed->exit_request(this, true))
        ed->DoClose();
    }
    else if (event.GetId()==TD_CPM_EXIT_UNCOND)
    { do  // destroy all pages
      { ed->exit_request(this, false);
        ed->remove_tools_internal();  // must be before RemovePage which adds the tools of the activated page!
        RemovePage(pg);
        ed->Destroy();  // must first remove the page from the notebook and the destroy it
#ifdef LINUX
        if (pg == GetSelection() && pg>=GetPageCount())
          if (pg) { SetSelection(pg-1);  pg = GetSelection(); }
          else pg=-1; // patching a GTK error
#else
        pg = GetSelection();
#endif
      } while (pg!=-1);
    }
    else ed->OnCommand(event);
  }
 // close the container if it is empty:
  pg = GetSelection();
  if (pg==-1)
    destroy_designer(the_editor_container_frame, &editor_cont_coords);
}

void editor_container::OnDisconnecting(ServerDisconnectingEvent & event)
// Iterates on pages.
{ for (int pg = 0;  pg<GetPageCount();  )
  { editor_type * ed = (editor_type*)GetPage(pg);
    SetSelection(pg);  // select the page for the user to see what is to be saved
    ServerDisconnectingEvent disev2(event.xcdp);
    ed->OnDisconnecting(disev2);
    if (!disev2.cancelled && !disev2.ignored)  // must not use such closing of the editor which would close the container
    { ed->remove_tools_internal();  // must be before RemovePage which adds the tools of the activated page!
      RemovePage(pg);
      ed->Destroy();  // must first remove the page from the notebook and then destroy it
    }
    else  // saving cancelled
    { event.cancelled=disev2.cancelled;  event.ignored=disev2.ignored;
      if (disev2.cancelled) break;
      pg++;
    }
  }
}

void editor_container::OnRecordUpdate(RecordUpdateEvent & event)
{ 
  if (event.closing_cursor)  // sent by the datagrid when closing it
  { int pages = (int)GetPageCount();
    for (int pg = 0;  pg<pages;  )
    { editor_type * ed = (editor_type*)GetPage(pg);  // selectiong the page is inside ed->OnRecordUpdate()
      RecordUpdateEvent ruev2(event.xcdp, event.cursnum, event.unconditional);
      ed->OnRecordUpdate(ruev2);
      if (ruev2.cancelled)
      { event.cancelled=true;
        break;
      }
     // increase [pg] unless the page has been deleted:
      if (pages > GetPageCount())
        pages = (int)GetPageCount();
      else
        pg++;
    }
  }
}

#if 0
bool editor_container::IsDesignerOf(cdp_t a_cdp, tcateg a_category, tobjnum an_objnum) const
{ 
  for (int pg = 0;  pg<GetPageCount();  pg++)
  { const editor_type * ed = (const editor_type*)((editor_container*)this)->GetPage(pg);  // must un-const this
    if (ed->IsDesignerOf(a_cdp, a_category, an_objnum))
    //if (ed->cdp==a_cdp && ed->category==a_category && ed->objnum==an_objnum)
    { ((editor_container*)this)->SetSelection(pg);  
      return true;
    }
  }
  return false;
}
#endif

void editor_type::OnRecordUpdate(RecordUpdateEvent & event)
{
  if (event.closing_cursor)  // sent by the datagrid when closing it
    if (event.xcdp==cdp && event.cursnum==cursnum && IS_CURSOR_NUM(cursnum))  // close the editor!
    {// find the page, user must see it during the "Save?" question
      if (in_edcont)
      { int pg=0;
        while (pg<the_editor_container->GetPageCount())
          if ((editor_type*)the_editor_container->GetPage(pg) == this)  // page found, delete it
          { the_editor_container->SetSelection(pg);
            break;
          }
      }
      else if (cf_style==ST_AUINB)
      { int page = aui_nb->GetPageIndex(GetParent());
        if (page != wxNOT_FOUND)
          aui_nb->SetSelection(page);
      }
      if (!IsChanged()) 
        DoClose();
      else  // changed, saving?
        if (exit_request(dialog_parent(), !event.unconditional))
          DoClose();
        else
          event.cancelled=true;  // set only when !event.unconditional
    }
}

wxTopLevelWindow * editor_type::dialog_parent(void)
{ return global_style==ST_MDI ? wxGetApp().frame : in_edcont ? the_editor_container_frame : dsgn_mng_wnd; }

void editor_type::ch_status(bool is_changed)
{ int i;
  if (in_edcont) 
  {// find tab index of the editor:
    for (i=0;  i<the_editor_container->GetPageCount();  i++)
      if (this==(editor_type*)the_editor_container->GetPage(i)) break;
    if (i==the_editor_container->GetPageCount()) return;  // strange error
#ifdef STOP  // changing the icon replaced (below) by adding * after the object name
   // change the icon:
    the_editor_container->SetPageImage(i, is_changed ? 0 : -1);
    scrolled_panel->Refresh();  // otherwise all or part of the page are cleared (on Windows)
    Refresh();  // necessary to update the scrollbars
#endif
  }
  set_designer_caption();
}
/////////////////////////////////// editor list ///////////////////////////////////////
////////////////////////////// editor list //////////////////////////////
// Theory: Vsechny alokovane editory jsou v seznamu. Pri otevirani noveho editoru
// se otestuje, zda v seznamu neni editor se stejnou referenci do databaze.
// -Pokud je, aktivuje se existujici editor, od otevirani noveho se upusti.
// -Pokud neni, vlozi se do seznamu.
// Aktivace existujiciho editoru z modalniho pohledu neni k nicemu,
//  ale plni ucel zabraneni dvojnadobne editaci.
static editor_type * editor_chain = NULL;  /* list of all open editors */

static void remove_from_chain(editor_type * editor)
{ editor_type ** ep;
  ep=&editor_chain;
  while (*ep)
  { if (*ep==editor)  // found, remove it
      { *ep=editor->next_editor;  break; }
    ep=&(*ep)->next_editor;
  }
}

static void add_to_chain(editor_type * editor)
// inserts editor into the chain
{ editor->next_editor=editor_chain;
  editor_chain=editor;
}

bool editor_type::IsSame(const editor_type *editor)
{ if (cdp!=editor->cdp) return false;
  if (recnum==NORECNUM) return false;
  return cursnum==editor->cursnum && recnum==editor->recnum &&
         attr   ==editor->attr    && index ==editor->index;
}

void competing_frame::to_top(void)
{
  wxTopLevelWindow * tlw = dsgn_mng_wnd;
  switch (cf_style)
  { case ST_POPUP:
      if (tlw)
      { if (tlw->IsIconized()) tlw->Iconize(false);
        tlw->Raise();  // should work both for managed windows and for child windows
      }
      break;
    case ST_MDI:
      if (tlw && tlw->IsKindOf(CLASSINFO(DesignerFrameMDI)))
      { DesignerFrameMDI * frm = (DesignerFrameMDI *)tlw;
        frm->Restore();
        frm->Activate();
      }
      break;
    case ST_AUINB:
    { int page = aui_nb->GetPageIndex(focus_receiver->GetParent());
      if (page != wxNOT_FOUND)  // unless a child
        aui_nb->SetSelection(page);
      break;
    }
    case ST_CHILD:  // no action!
      break;
    case ST_EDCONT:
    {// move the container to the top (may be POPUP or MDI)
      the_editor_container->to_top();
     // select the proper page:
      for (int pg=0;  pg<the_editor_container->GetPageCount();  pg++)
        if ((competing_frame*)(editor_type*)the_editor_container->GetPage(pg) == this)
          { the_editor_container->SetSelection(pg);  break; }
      break;
    }
  }
}
      


editor_type * find_editor_by_objnum9(cdp_t cdp, tcursnum cursnum, tobjnum objnum, tattrib attrnum, tcateg category, bool open_if_not_opened)
// When category==NONECATEG then category is CATEG_PROC or CATEG_TRIGGER.
{ 
  if (objnum==NOOBJECT) return NULL;
  for (editor_type * ed = editor_chain;  ed;  ed=ed->next_editor)
  { if (objnum==ed->recnum && ed->cursnum==cursnum && ed->attr==attrnum)  // found, activate it
    { ed->to_top();
#if 0
      if (ed->in_edcont)
      { if (the_editor_container_frame->IsIconized()) { the_editor_container_frame->Iconize(false);  the_editor_container_frame->Restore(); }  // don't know which is correct
        if (global_style==ST_MDI)
          ((DesignerFrameMDI*)the_editor_container_frame)->Activate();
        else
          the_editor_container_frame->Raise();
       // select the proper page:
        for (int pg=0;  pg<the_editor_container->GetPageCount();  pg++)
          if ((editor_type*)the_editor_container->GetPage(pg) == ed)
            { the_editor_container->SetSelection(pg);  break; }
      }
      else
      { wxFrame * designer = (wxFrame*)ed->GetParent();
        if (designer->IsIconized()) { designer->Iconize(false);  designer->Restore(); }  // don't know which is correct
        if (global_style==ST_MDI)
        	((DesignerFrameMDI*)designer)->Activate();
        else
          designer->Raise();
      }
      //if (ed->popup) SetActiveWindow(hWnd);
#endif
      return ed;
    }
  }
 // editor not found:
  if (open_if_not_opened)
  { if (category==NONECATEG)
      cd_Read(cdp, OBJ_TABLENUM, objnum, OBJ_CATEG_ATR, NULL, (tptr)&category);
    open_text_object_editor(cdp, cursnum, objnum, attrnum, category, "", OPEN_IN_EDCONT|AUTO_READ_ONLY);
    return find_editor_by_objnum9(cdp, cursnum, objnum, attrnum, category, false);
  }
  return NULL;
}

void show_pgm_line(cdp_t cdp, tobjnum src_objnum, tcateg category, int src_line, int src_column)
{
  editor_type * edd = find_editor_by_objnum9(cdp, OBJ_TABLENUM, src_objnum, OBJ_DEF_ATR, category, true);
  if (edd)
  { edd->position_on_line_column(src_line, src_column);
    edd->Refresh2();  // show the active line
  }
}

void realize_bps(cdp_t cdp)
{ for (editor_type * ed = editor_chain;  ed;  ed=ed->next_editor)
    if (ed->content_category()==CONTCAT_SQL)
    { for (int i = 0;  i<ed->breakpts.list.count();  i++)
      { t_posit line_a = *ed->breakpts.list.acc0(i);
        if (line_a!=NO_LINENUM)
        { int line_n = ed->line2num(line_a);
          int pos;
          if (cd_Server_debug(cdp, DBGOP_ADD_BP, ed->objnum, line_n+1, NULL, NULL, &pos))
            *ed->breakpts.list.acc0(i)=NO_LINENUM;  // BP cannot be set, delete
          else if (pos!=line_n+1)  // BP moved to the next valid line
            *ed->breakpts.list.acc0(i)=ed->line_num2ptr(pos-1);
        }
      }
      ed->Refresh2();
    }
}

void debug_edit_enable(bool enable)
// Enters or leaves the state of temporarily disabled editing
{ for (editor_type * ed = editor_chain;  ed;  ed=ed->next_editor)
  { if (enable)  // re-enable editing if disabled temporarily
    { if (ed->noedit==2) 
      { ed->noedit=0;
      }
    }
    else         // disable editing if enabled
    { if (!ed->noedit) 
      { ed->noedit=2; 
      }
    }
    ed->update_toolbar();
  }
}

bool editor_type::open(wxWindow * parent, t_global_style my_style)
{
  if (!prep(parent))
    return false;
  if (!load(0))
    return false;
  count_lines();  // defines the virtual size
  return true;
}

editor_type * new_editor(cdp_t cdp, tcursnum cursnum, tobjnum objnum, tattrib attrnum, tcateg category, const char * folder_name, bool auto_read_onlyIn)
{
   if (category==(tcateg)-1)
     return new CLOB_editor_type(cdp, cursnum, objnum, attrnum, auto_read_onlyIn);
   else
     return new text_object_editor_type(cdp, objnum, category, folder_name, auto_read_onlyIn);
}

bool open_text_object_editor(cdp_t cdp, tcursnum cursnum, tobjnum objnum, tattrib attrnum, tcateg category, const char * folder_name, int ce_flags)
{ editor_type * ed;
  if (find_editor_by_objnum9(cdp, cursnum, objnum, attrnum, category, false)) 
    return true;
  ed = new_editor(cdp, cursnum, objnum, attrnum, category, folder_name, (ce_flags & AUTO_READ_ONLY)!=0);
  if (!ed) return false;
  if (!ed->analyse_object()) 
    { delete ed;  return false; }

  bool in_container = false;
  if (ce_flags & OPEN_IN_EDCONT)
    if (global_style!=ST_AUINB)
#ifndef WINS  // not using the editor containter in MDI outside MSW because the MDI is a container
      if (global_style!=ST_MDI)
#endif
        in_container=true;

  if (in_container)
  {// open or restore the editor container:
    if (!the_editor_container_frame)
    { the_editor_container = new editor_container();
      if (!open_standard_designer(the_editor_container))
        { the_editor_container = NULL;  return false; }
    }
    else
      the_editor_container->to_top();
   // create the editor inside the container:
    ed->in_edcont=true;  // must be before open()
    if (the_editor_container->GetPageCount()>0)  // must remove existing tools first
    { editor_type * ed2 = (editor_type*)the_editor_container->GetPage(the_editor_container->GetSelection());
      ed2->remove_tools_internal();
    }
    ed->cf_style=ST_EDCONT;
    if (!ed->open(the_editor_container, ST_EDCONT))
    { if (ed->focus_receiver) ed->focus_receiver->Destroy();  else delete ed;  // if created, use Destroy(), delete otherwise
      return false;
    }
    ed->add_tools();  
    ed->write_status_message();
   // add the editor as a page:
    the_editor_container->adding_page=true;
    the_editor_container->AddPage(ed, wxEmptyString, true, category==(tcateg)-1 ? IND_INCL : get_image_index(category, 0/*ed->flags*/));
    the_editor_container->adding_page=false;
    ed->set_designer_caption();
    ed->Refresh2();  // without this the 2nd and following pages are not drawn
#ifdef LINUX        
    if (global_style==ST_POPUP) ed->Activated();  // not called in AppPage when the 2nd and next pages are created, necessary to create the tools on the toolbar
#endif        
    ed->update_toolbar();
  }
  else  // normal openning:
  { if (!open_standard_designer(ed))
      return false;
  }
  add_to_chain(ed);
  return true;
}

#if 0
bool open_text_object_editor(wxMDIParentFrame * frame, cdp_t cdp, tcursnum cursnum, tobjnum objnum, tattrib attrnum, tcateg category, const char * folder_name, int ce_flags)
{ wxWindow * editor_parent;  editor_type * ed;
  if (find_editor_by_objnum9(cdp, cursnum, objnum, attrnum, category, false)) 
    return true;
#ifndef WINS  // not using the editor containter in MDI outside MSW because the MDI is a container
  if (global_style==ST_MDI)
    ce_flags &= ~OPEN_IN_EDCONT;
#endif
  if (ce_flags & OPEN_IN_EDCONT)
  { if (!the_editor_container_frame)
    { if (global_style==ST_MDI)
      { DesignerFrameMDI * frm = new DesignerFrameMDI(frame, -1, _("Text Editor"));
        if (!frm) return false;
        the_editor_container_frame = frm;
        //frm->Maximize(true);
        the_editor_container = new editor_container(NULL, NOOBJECT, "", "", the_editor_container_frame);
        if (!the_editor_container)
          { delete the_editor_container_frame;  the_editor_container_frame=NULL;  return false; }
        frm->dsgn=the_editor_container;
      }
      else
      { DesignerFramePopup * frm = new DesignerFramePopup(frame, -1, _("Text Editor"), editor_cont_coords.get_pos(), editor_cont_coords.get_size());
        if (!frm) return false;
        the_editor_container_frame = frm;
        the_editor_container = new editor_container(NULL, NOOBJECT, "", "", the_editor_container_frame);
        if (!the_editor_container)
          { delete the_editor_container_frame;  the_editor_container_frame=NULL;  return false; }
        frm->dsgn=the_editor_container;
      }
      the_editor_container->Create(the_editor_container_frame, -1, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN | wxWANTS_CHARS, wxT("editor_container"));
      the_editor_container->SetImageList(wxGetApp().frame->main_image_list);
      the_editor_container_frame->SetIcon(wxIcon(source_xpm));
      the_editor_container->put_designer_into_frame(the_editor_container_frame, the_editor_container);
      the_editor_container_frame->Show();
    }
    else
      if (the_editor_container_frame->IsIconized()) { the_editor_container_frame->Iconize(false);  the_editor_container_frame->Restore(); }  // don't know which is correct
    editor_parent = the_editor_container;
    ed = new_editor(cdp, cursnum, objnum, attrnum, category, folder_name, NULL);
    if (!ed) goto ex0;
    ed->in_edcont=true;
    if (the_editor_container->GetPageCount()>0)  // must remove existing tools first
    { editor_type * ed2 = (editor_type*)the_editor_container->GetPage(the_editor_container->GetSelection());
      ed2->remove_tools_internal();
    }
  }
  else
  { wxTopLevelWindow * dsgn_frame;
    if (global_style==ST_MDI)
    { DesignerFrameMDI * frm = new DesignerFrameMDI(wxGetApp().frame, -1, wxEmptyString);
      if (!frm) return false;
      editor_parent = dsgn_frame = frm;
      ed = new_editor(cdp, cursnum, objnum, attrnum, category, folder_name, frm);
      if (!ed) goto ex0;
      frm->dsgn = ed;
    }
    else
    { DesignerFramePopup * frm = new DesignerFramePopup(wxGetApp().frame, -1, wxEmptyString, editor_sep_coords.get_pos(), editor_sep_coords.get_size());
      if (!frm) return false;
      editor_parent = dsgn_frame = frm;
      ed = new_editor(cdp, cursnum, objnum, attrnum, category, folder_name, frm);
      if (!ed) goto ex0;
      frm->dsgn = ed;
    }
    dsgn_frame->SetIcon(wxIcon(category==CATEG_PROC    ? _proc_xpm  : category==CATEG_TRIGGER ? _trigger_xpm :
                               category==CATEG_CURSOR  ? _query_xpm :
                               category==CATEG_PGMSRC  ? _xml_xpm   : _program_xpm));
  }

  if (!ed->analyse_object((ce_flags & AUTO_READ_ONLY)!=0)) { delete ed;  goto ex0; }

  ed->add_tools();  ed->write_status_message();

  if (ed->prep(editor_parent))
  { if (ed->load(0))
    { ed->count_lines();  // defines the virtual size
      if (ce_flags & OPEN_IN_EDCONT)
      {// add to the notebook:
        the_editor_container->adding_page=true;
        the_editor_container->AddPage(ed, wxT("edit"), true, category==(tcateg)-1 ? IND_INCL : get_image_index(category, 0/*ed->flags*/));
        the_editor_container->adding_page=false;
        ed->Refresh2();  // without this the 2nd and following pages are not drawn
#ifdef LINUX        
        if (global_style==ST_POPUP) ed->Activated();  // not called in AppPage when the 2nd and next pages are created, necessary to create the tools on the toolbar
#endif        
        ed->update_toolbar();
      }
      else
        ed->put_designer_into_frame(editor_parent, ed);
      ed->set_designer_caption();
      add_to_chain(ed);
      editor_parent->Show();
  //    if (ce_flags & OPEN_IN_EDCONT)
  //      the_editor_container_frame->Layout();  // necessary on Linux
      ed->scrolled_panel->SetFocus();
      wxGetApp().frame->SetFocusDelayed(ed->scrolled_panel);
      return true;
    }
    delete ed;
  }
 ex0:
  if (!(ce_flags & OPEN_IN_EDCONT))
    editor_parent->Destroy();
  return false;
}
#endif

void fsed_delete_bitmaps(void)
{ 
  delete_bitmaps(text_object_editor_type::txed_tool_info); 
  delete_bitmaps(text_object_editor_type::trigger_txed_tool_info); 
  delete_bitmaps(CLOB_editor_type::CLOB_tool_info); 
}


CFNC void unlink_cache_editors(HWND hOwner, trecnum recnum)
/* Called when a record is synchronized or a view is closed (recnum==NORECNUM then).
   Makes the editors working with the record independent. */
{ 
#if 0
  HWND hwNext, hwClient=GetClient(hOwner),
               hwChild =GetWindow(hwClient, GW_CHILD);
  while (hwChild)
  { hwNext=GetWindow(hwChild, GW_HWNDNEXT);  /* check the next child */
    SendMessage(hwChild, SZM_UNLINK_CACHE_ED, (WPARAM)hOwner, recnum);
    hwChild=hwNext;
  }
#endif
}
/////////////////////////////////////////// project //////////////////////////////////////////////////////
static int compile_program9(cdp_t cdp, tptr source, int output_type,
          void (WINAPI *startnet)(CIp_t CI), BOOL report_success, BOOL debug_info,
          const char * srcname, tobjnum srcobj, tobjnum codeobj, void * univ_ptr)
/* compiles the program from the prepared source & puts result on the screen */
{ compil_info xCI(cdp, source, output_type, startnet);
  xCI.gener_dbg_info=(wbbool)debug_info;
  xCI.outname=srcname;
  strcpy(xCI.src_name, srcname ? srcname : NULLSTRING);
  xCI.src_objnum=srcobj;
  xCI.univ_ptr=univ_ptr;
  xCI.cb_object=codeobj;
  int res=compile(&xCI);
  if (res)
  { delete xCI.dbgi;  cdp->RV.dbgi=NULL;
    show_pgm_line(cdp, srcobj, CATEG_PGMSRC, xCI.src_line, xCI.sc_col);
    client_error(cdp, res);
    wchar_t buf[256];
    Get_error_num_textW(cdp, res, buf, sizeof(buf)/sizeof(wchar_t));
    error_box(buf, wxGetApp().frame);
  }
  else 
    cdp->RV.dbgi=xCI.dbgi;   /* move debug info */

  return res;
}

static BOOL timestamps_ok(cdp_t cdp, pgm_header_type * h)
{ for (int i=0;  i<MAX_SOURCE_NUMS && h->sources[i]!=NOOBJECT;  i++)
  { uns32 stamp;
    if (Get_obj_modif_time(cdp, h->sources[i], CATEG_PGMSRC, &stamp))
      if (stamp > h->compil_timestamp)
        return FALSE;
  }
  return TRUE;
}

#define MAX_RETRY 5 // nesmi byt prilis moc, pokud nemam pravo cist program, dlouho ceka

int synchronize9(cdp_t cdp, const char * srcname, tobjnum srcobj, tobjnum *codeobj, BOOL recompile, BOOL debug_info, BOOL report_success)
/* return error codes, comp. results are put into the info box */
// Doplnena castecna ochrana pred soubeznym prekladanim tehoz programu z ruznych klientu, deje se ve WBCGI, kdyz vice frames otevira stejny projekt
{ int res=0;  BOOL locked=FALSE;
 /* checking to see if compilation is necessary */
  BOOL comp_needed=TRUE;  int retry_counter=0;
 retry:
  if (!cd_Find2_object(cdp, srcname, NULL, CATEG_PGMEXE, codeobj))
  {// must not try to lock nor read in cannot read 
    uns8 priv_val[PRIVIL_DESCR_SIZE];
    cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, OBJ_TABLENUM, *codeobj, OPER_GETEFF, priv_val);
    if (!HAS_READ_PRIVIL(priv_val, OBJ_DEF_ATR))
    { client_error(cdp, NO_RUN_RIGHT);  
      return NO_RUN_RIGHT;  
    }
    pgm_header_type h;  uns32 rdsize;
    if (!cd_Read_var(cdp, OBJ_TABLENUM, (trecnum)*codeobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(pgm_header_type), (tptr)&h, &rdsize))
      if (rdsize==sizeof(pgm_header_type))  /* non-empty code */
        if (h.version==EXX_CODE_VERSION && h.language==cdp->selected_lang && h.platform==CURRENT_PLATFORM)  /* compiler version number & language OK */
          if (timestamps_ok(cdp, &h))
            comp_needed=FALSE;
   // locking moved after checking the time in order to prevent NOT_LOCKED error messages when many clients simultaneously try to open a project.
    if (comp_needed)
      if (HAS_WRITE_PRIVIL(priv_val, OBJ_DEF_ATR))
       // try to lock and check if it has to be compiled    // cd_waiting(cdp, 70); -- does not work for Write_lock_...
        if (!cd_Write_lock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj)) locked=TRUE;
        else if (retry_counter++<MAX_RETRY) { Sleep(1000);  goto retry; }
  }
  else // create the code object
  { if (cd_Insert_object(cdp, srcname, CATEG_PGMEXE, codeobj))
      if (retry_counter++<MAX_RETRY) goto retry;
    if (!cd_Write_lock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj)) locked=TRUE;
    else if (retry_counter++<MAX_RETRY) goto retry;
    uns8 priv_val[PRIVIL_DESCR_SIZE];
    //*priv_val=RIGHT_DEL;
    //memset(priv_val+1, 0xff, PRIVIL_DESCR_SIZE-1); -- must not set privils which I do not have!
    cd_GetSet_privils(cdp, NOOBJECT,        CATEG_USER,  OBJ_TABLENUM, *codeobj, OPER_GET, priv_val);
    cd_GetSet_privils(cdp, EVERYBODY_GROUP, CATEG_GROUP, OBJ_TABLENUM, *codeobj, OPER_SET, priv_val);
  }
  if (srcobj==NOOBJECT)
    if (cd_Find2_object(cdp, srcname, NULL, CATEG_PGMSRC, &srcobj))
      if (comp_needed)  /* no source, but code exists */
        { res=OBJECT_NOT_FOUND;  goto ex; }
      else
      { 
        goto ex;
      }

 // check the source run privilege:
  if (comp_needed || recompile || cdp->global_debug_mode != (cdp->RV.bpi!=NULL))   
  /* compilation necessary */
  { tptr src=cd_Load_objdef(cdp, srcobj, CATEG_PGMSRC);
    if (!src) { client_error(cdp, OUT_OF_MEMORY);  cd_Signalize(cdp);  res=OUT_OF_MEMORY;  goto ex; }
    res=compile_program9(cdp, src, DB_OUT, program, report_success, debug_info, srcname, srcobj, *codeobj, NULL);
    corefree(src);
  }
 ex:
  if (locked) cd_Write_unlock_record(cdp, OBJ_TABLENUM, (trecnum)*codeobj);
  return res;
}

////////////////////////////////////// direct copy from project.cpp ///////////////////////////////
void convert_type(typeobj *& tobj, objtable * id_tab);
CFNC void WINAPI link_forwards(objtable * id_tab);

void conv_type_inner(typeobj * tobj, objtable * id_tab)
// Converts type references used in the description of a type. Not called for T_SIMPLE.
{ switch (tobj->type.anyt.typecateg)
  { 
    case T_ARRAY:
      convert_type(tobj->type.array.elemtype, id_tab);  break;
    case T_PTR:
      convert_type(tobj->type.ptr.domaintype, id_tab);  break;
    case T_PTRFWD:
    { object * obd = search1(tobj->type.fwdptr.domainname, id_tab);
      if (obd==NULL || obd->any.categ!=O_TYPE)  // impossible, I hope
        tobj->type.fwdptr.domaintype=&att_int32;
      else
        tobj->type.fwdptr.domaintype=&obd->type;
      break;
    }
    case T_RECORD:
      link_forwards(tobj->type.record.items);  break;
    case T_RECCUR:
      if (tobj->type.record.items)
        link_forwards(tobj->type.record.items);  
      break;
  }
}

void convert_type(typeobj *& tobj, objtable * id_tab)
{ if (!tobj) return; // may ne the NULL ptr for retvaltype
  if ((size_t)tobj < ATT_AFTERLAST)
    tobj = simple_types[(size_t)tobj];
  else
    conv_type_inner(tobj, id_tab);
}

CFNC void WINAPI link_forwards(objtable * id_tab)
{ for (int i=0;  i<id_tab->objnum;  i++)
  { objdef * objd = &id_tab->objects[i];
    object * ob = objd->descr;
    switch (ob->any.categ)
    { case O_VAR:  case O_VALPAR:  case O_REFPAR:  case O_RECELEM:
        convert_type(ob->var.type, id_tab);
        break;
      case O_CONST:
        convert_type(ob->cons.type, id_tab);
        break;
      case O_TYPE:
      { typeobj * tobj = &ob->type;
        if (tobj->type.anyt.typecateg)
        { convert_type(tobj, id_tab);  
          ob=(object*)tobj;
        }
        else
          conv_type_inner(tobj, id_tab);
        break;
      }
      case O_FUNCT:  case O_PROC:
        convert_type(ob->subr.retvaltype, id_tab);
        link_forwards(ob->subr.locals);
        break;
      case O_CURSOR:
        convert_type(ob->curs.type, id_tab);
        break;
    }
  }
}
///////////////////////////////////////////////////////////

CFNC DllKernel objtable standard_table;

CFNC int WINAPI set_project_up(cdp_t cdp, const char * progname, tobjnum srcobj,
                  BOOL recompile, BOOL report_success)
{ int res;  tobjnum codeobj;  pgm_header_type h;  run_state * RV = &cdp->RV;
  tptr prog_code, prog_vars;  uns8 bt;  char * prog_decl, * prog_decl2;
  uns32 rdsize;  
  if (!recompile && !strcmp(RV->run_project.project_name, progname) &&
      cdp->global_debug_mode == (cdp->RV.bpi!=NULL))
    return 0;  /* the project is already set-up */

  free_project(cdp, FALSE);
  res=synchronize9(cdp, progname, srcobj, &codeobj, recompile, cdp->global_debug_mode, report_success);
  /* 1st call to synchronize opens the info-window. It's placed below the windesk! */
  /* close if compilation OK and !recompile */
  if (res) return res;
 /* loading program header, code & decls: */
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(pgm_header_type), (tptr)&h, &rdsize))
    { cd_Signalize(cdp);  return 1; }
  prog_code=(tptr)corealloc(h.code_size+255, 255); /* +255 prevents GPF when writing strings near the segment's end */
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, 0, h.code_size, prog_code, &rdsize))
    { cd_Signalize(cdp);  corefree(prog_code);  return 1; }
  unsigned decl_space = h.decl_size > h.unpacked_decl_size ? h.decl_size : h.unpacked_decl_size; // unpacked_decl_size is usually bigger on 64-bit systems
  prog_decl =(tptr)corealloc(h.decl_size+1, 255); /* when relocating I will read 1 byte more */
  prog_decl2=(tptr)corealloc(h.unpacked_decl_size, 255);
  if (!prog_decl || !prog_decl2) { no_memory();  corefree(prog_code);  corefree(prog_decl);  corefree(prog_decl2);  return 1; }
  if (cd_Read_var(cdp, OBJ_TABLENUM, codeobj, OBJ_DEF_ATR, NOINDEX, h.code_size, h.decl_size, prog_decl, &rdsize))
    { cd_Signalize(cdp);  corefree(prog_decl);  corefree(prog_decl2);  corefree(prog_code);  return 1; }
 /* create vars */
  prog_vars=(tptr)corealloc(h.proj_vars_size+sizeof(scopevars)+255, 255);  /* +255 prevents GPF when writing strings near the segment's end */
  if (!prog_vars)
    { no_memory();  corefree(prog_decl);  corefree(prog_decl2);  corefree(prog_code);  return 1; }
  memset(prog_vars, 0, h.proj_vars_size+sizeof(scopevars));
 /* relink the decls */
  uns32 src, dest, offset;
  src=0;  dest=0;
  while (src<h.decl_size)
  { bt=prog_decl[src++];
    if (bt==RELOC_MARK)
    { offset=*(uns32*)(prog_decl+src) & 0xffffffL;
      src+=3;
      if (offset!=0xffffffL)
      { *(tptr *)(prog_decl2+dest)=(tptr)(prog_decl2+offset);
        dest+=sizeof(tptr);
      }
      else prog_decl2[dest++]=bt;
    }
    else prog_decl2[dest++]=bt;
  }
  corefree(prog_decl);
 // update forward pointers:
  objtable * id_tab=(objtable*)(prog_decl2+h.pd_offset);
  id_tab->next_table=&standard_table;   // used when the project is set during the compilation of menu or view
  link_forwards(id_tab);
 /* set up the project info */
  RV->run_project.proj_decls = prog_decl2;
  RV->run_project.global_decls = id_tab;
  RV->run_project.common_proj_code=(pgm_header_type*)prog_code;
  strcpy(RV->run_project.project_name, progname);  
  Upcase(RV->run_project.project_name);
  RV->glob_vars=(scopevars*)prog_vars;  RV->glob_vars->next_scope=NULL;
  exec_constructors(cdp, TRUE);
 // new proj_decls & global_decls are valid, delete the previous:
  if (RV->run_project.prev_proj_decls) corefree(RV->run_project.prev_proj_decls);
  RV->run_project.prev_proj_decls=NULL;  RV->run_project.prev_global_decls=NULL;
  return 0;
}


void run_pgm9(cdp_t cdp, const char * srcname, tobjnum srcobj, BOOL recompile)
/* on compilation error CompInfo remains on the screen */
{ int res;  run_state * RV = &cdp->RV;

  res=set_project_up(cdp, srcname, srcobj, recompile, FALSE);
  if (!res)
  { run_prep(cdp, (tptr)RV->run_project.common_proj_code, RV->run_project.common_proj_code->main_entry);
    res=run_pgm(cdp);
    if (res!=EXECUTED_OK)  /* run error */
      { client_error(cdp, res);  cd_Signalize(cdp); }
    /* foll. lines must not be called before reporting run error! */
    int clres=close_run(cdp);  /* closes files & cursors */
    if (!cd_Close_cursor(cdp, (tcursnum)-2)) clres|=2;
    if (res==EXECUTED_OK)  /* no error, display warnings: */
    { if (clres & 1) info_box(wxT("FILES_NOT_CLOSED"));
      if (clres & 2) info_box(wxT("CURSORS_NOT_CLOSED"));
      info_box(wxT("Program completed OK."));
    }
  }
}

