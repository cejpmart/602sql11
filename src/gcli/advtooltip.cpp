///////////////////////////////////////////////////////////////////////////////
// Name:        generic/advtooltip.cpp
// Purpose:     wxAdvToolTip
// Author:      Vaclav Slavik
// Modified by:
// Created:     2004/06/30
// RCS-ID:      $Id: advtooltip.cpp,v 1.1 2007/08/30 11:36:05 drozd Exp $
// Copyright:   (c) 2004 Vaclav Slavik <vaclav.slavik@matfyz.cz>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    //#pragma implementation "advtooltip.h"
#endif

#include "advtooltip.h"
#include "wx/log.h"
#include "wx/dcclient.h"
#include "wx/settings.h"

#ifdef __WXGTK__
    #include <gtk/gtk.h>
    #include "wx/gtk/win_gtk.h"
#endif

#if wxUSE_POPUPWIN

// ----------------------------------------------------------------------------
// wxAdvToolTipPopup
// ----------------------------------------------------------------------------

class wxAdvToolTipPopup : public wxPopupWindow
{
public:
    wxAdvToolTipPopup(wxWindow *parent);

    void Popup();
    void Dismiss();

private:
    wxWindow *m_child;
};

        
wxAdvToolTipPopup::wxAdvToolTipPopup(wxWindow *parent)
    : wxPopupWindow(parent), m_child(NULL)
{
#ifdef __WXGTK__
    // this should be done in child view, not here, but there's
    // something wrong with wxGTK' style handling, effective style
    // is parent's style
    gtk_widget_set_name(m_widget, "gtk-tooltips");
#endif

    // NB: Create the popup hidden, because it's supposed to by shown by
    //     call to Popup(). Without this, the popup misbehaves under wxGTK and
    //     the window is initially shown at position (0,0) despite having
    //     called Move() on it.
    Show(false);
}

void wxAdvToolTipPopup::Popup()
{
    Dismiss();

    const wxWindowList& children = GetChildren();
    if ( children.GetCount() )
    {
        m_child = children.GetFirst()->GetData();
    }
    else
    {
        m_child = this;
    }

    // we can't capture mouse before the window is shown in wxGTK, so do it
    // first
    Show();

    m_child->CaptureMouse();
}

void wxAdvToolTipPopup::Dismiss()
{
    if (m_child)
    {
        if (m_child->HasCapture())
            m_child->ReleaseMouse();
        m_child = NULL;
    }
    Hide();
}
 

// ----------------------------------------------------------------------------
// wxAdvToolTipView
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxAdvToolTipView, wxWindow)

BEGIN_EVENT_TABLE(wxAdvToolTipView, wxWindow)
    EVT_ERASE_BACKGROUND(wxAdvToolTipView::OnEraseBackground)
    EVT_MOUSE_EVENTS(wxAdvToolTipView::OnMouseEvent)
END_EVENT_TABLE()
    
wxAdvToolTipView::wxAdvToolTipView(wxAdvToolTipPopup *parent)
    : wxWindow(parent, wxID_ANY), m_popup(parent)
{
#ifndef __WXGTK__
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));
    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));
#endif
}

void wxAdvToolTipView::OnEraseBackground(wxEraseEvent& event)
{
#ifdef __WXGTK__
    gtk_paint_flat_box(GetParent()->m_widget->style,
                       GTK_PIZZA(m_wxwindow)->bin_window,
                       GTK_STATE_NORMAL, GTK_SHADOW_OUT, 
                       NULL, GetParent()->m_widget, "tooltip",
                       0, 0, -1, -1);
#else
    wxDC *dc = event.GetDC();
    dc->SetBrush(wxBrush(GetBackgroundColour(), wxSOLID));
    dc->SetPen(wxPen(GetForegroundColour(), 1, wxSOLID));
    dc->DrawRectangle(wxRect(wxPoint(0,0), GetSize()));
#endif
}

void wxAdvToolTipView::OnMouseEvent(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
   
    if (event.Leaving() || event.Entering())
    {
    }
    else if (event.Moving())
    {
        if (!m_boundary.Contains(pos))
        {
            m_popup->Dismiss();
            wxLogTrace(_T("advtooltip"),
                       _T("dismissing tooltip, out of boundary"));
        }
    
        event.Skip();
    }
    else // some button pressed/released
    {
        wxLogTrace(_T("advtooltip"),
                   _T("dismissing tooltip, mouse click"));
        
        m_popup->Dismiss();
        
        // the event that led to dismissing the tooltip should still be
        // delivered to the window under it:
        wxMouseEvent new_event(event);
        ClientToScreen(&new_event.m_x, &new_event.m_y);
        wxWindow *win = wxFindWindowAtPoint(new_event.GetPosition());
        if (win)
        {
            win->ScreenToClient(&new_event.m_x, &new_event.m_y);
            new_event.SetEventObject(win);
            wxPostEvent(win, new_event);
        }
    }
}
    
void wxAdvToolTipView::SetBoundaryRect(const wxRect& rect)
{
    m_boundary = rect;
}


// ----------------------------------------------------------------------------
// wxAdvToolTip
// ----------------------------------------------------------------------------
    
IMPLEMENT_ABSTRACT_CLASS(wxAdvToolTip, wxEvtHandler)

BEGIN_EVENT_TABLE(wxAdvToolTip, wxEvtHandler)
    EVT_TIMER(-1, wxAdvToolTip::OnTimer)
    EVT_MOUSE_EVENTS(wxAdvToolTip::OnMouseEvent)
    EVT_KEY_DOWN(wxAdvToolTip::OnKeyDown)
    EVT_KILL_FOCUS(wxAdvToolTip::OnKillFocus)
    EVT_ACTIVATE(wxAdvToolTip::OnActivate)
END_EVENT_TABLE()

wxAdvToolTip::wxAdvToolTip(wxWindow *window)
{
    m_window = window;
    m_popup = NULL;
    m_view = NULL;
    m_timer = new wxTimer(this);

    window->PushEventHandler(this);
}

wxAdvToolTip::~wxAdvToolTip()
{
    delete m_timer;

    if (m_window)
        m_window->RemoveEventHandler(this);

    if (m_popup)
    {
        if (m_popup->IsShown())
            m_popup->Dismiss();
        m_popup->Destroy();
    }
}
    
wxAdvToolTipPopup *wxAdvToolTip::GetViewParent()
{
    if (!m_popup)
    {
        m_popup = new wxAdvToolTipPopup(m_window);
    }
    return m_popup;
}

void wxAdvToolTip::HandleAnyEvent(wxEvent& event)
{
    if (!m_popup || !m_popup->IsShown())
    {
        m_timer->Start(500, wxTIMER_ONE_SHOT); // FIXME: hardcoded timeout
    }
    event.Skip();
}

void wxAdvToolTip::OnMouseEvent(wxMouseEvent& event)
{
    if (event.Leaving())
        m_timer->Stop();
    else
        HandleAnyEvent(event);

    event.Skip();
}
    
void wxAdvToolTip::OnKeyDown(wxKeyEvent& event)
{
    if (m_popup && m_popup->IsShown())
        m_popup->Dismiss();
    else
        HandleAnyEvent(event);
    event.Skip();
}

void wxAdvToolTip::OnKillFocus(wxFocusEvent& event)
{
    if (m_popup && m_popup->IsShown())
        m_popup->Dismiss();
    event.Skip();
}

void wxAdvToolTip::OnActivate(wxActivateEvent& event)
{
    m_timer->Stop();
    
    if (!event.GetActive() && m_popup && m_popup->IsShown())
        m_popup->Dismiss();
 
    event.Skip();
}

void wxAdvToolTip::OnTimer(wxTimerEvent& event)
{
    wxLogTrace(_T("advtooltip"), _T("*"));
    if (event.GetEventObject() == m_timer)
    {
        wxLogTrace(_T("advtooltip"), _T("timer triggered"));
        PopupToolTip();
    }
    else
        event.Skip();
}

bool wxAdvToolTip::CanPopupWindow() const
{
    // When some other window has mouse capture, it's safe bet that the
    // user doesn't want to be interrupted with tooltips:
    if (wxWindow::GetCapture())
        return false;
    
#ifdef __WXGTK__
    // If there's some other window that grabbed the mouse, don't try
    // to show the tooltip -- it would not only be shown at wrong moment
    // (when the user is interacting with some other control), it could
    // also result in grab deadlock. (This is esentially the same thing as
    // about test, but works for native GTK+ widgets as well.)    
    if (gdk_pointer_is_grabbed())
        return false;

    // Modal dialogs grab events in wxGTK using gtk_grab_add(). This has an
    // unfortunate effect: it conflicts with mouse capture and the X11 desktop
    // is locked in state when it doesn't accept any mouse events. 
    //
    // We can't return false if some window has grab, because it's ok if our
    // parent dialog has it. Let's test for this case:
    GtkWidget *grabbed = gtk_grab_get_current();
    if (grabbed)
    {
        if (grabbed != wxGetTopLevelParent(m_window)->m_widget)
            return false;
    }
#endif

    return true;
}

void wxAdvToolTip::PopupToolTip()
{
    if (m_view)
    {
        if (m_popup->IsShown())
            m_popup->Dismiss();
        m_view->Destroy();
        m_view = NULL;
    }
 
    if (!CanPopupWindow())
        return;

    wxPoint mouse = wxGetMousePosition();
    wxPoint mouseClient = m_window->ScreenToClient(mouse);

    if (!wxRect(wxPoint(0,0), m_window->GetSize()).Contains(mouseClient))
        return;
        
    wxRect boundary(-1,-1,-1,-1);
    m_view = CreateView(mouseClient, boundary);

    if (m_view)
    {
        wxASSERT_MSG( boundary.GetSize() != wxDefaultSize,
                      _T("invalid boundary rectangle returned") );
            
        m_view->Move(0, 0);
        wxSize size = m_view->GetSize();
        m_popup->SetSize(size);
        m_popup->SetSizeHints(size, size);
    
        mouse.y += wxSystemSettings::GetMetric(wxSYS_CURSOR_Y);
        m_popup->Position(mouse, wxSize(0, 0));
        m_popup->Popup();
    
        boundary.SetPosition(
                    m_view->ScreenToClient(
                        m_window->ClientToScreen(boundary.GetPosition())));
        m_view->SetBoundaryRect(boundary);
    }
}

// ----------------------------------------------------------------------------
// wxAdvToolTipText
// ----------------------------------------------------------------------------

class wxAdvToolTipTextView : public wxAdvToolTipView
{
public:
    wxAdvToolTipTextView(wxAdvToolTipPopup *parent,
                         const wxString& text);

private:
    void OnPaint(wxPaintEvent& event);

    wxString m_text;
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxAdvToolTipTextView, wxAdvToolTipView)
    EVT_PAINT(wxAdvToolTipTextView::OnPaint)
END_EVENT_TABLE()

static const int TEXT_MARGIN = 4;

wxAdvToolTipTextView::wxAdvToolTipTextView(
                    wxAdvToolTipPopup *parent,
                    const wxString& text)
    : wxAdvToolTipView(parent), m_text(text)
{
    int w, h;
    GetTextExtent(text, &w, &h);
    SetSize(w + 2 * TEXT_MARGIN, h + 2 * TEXT_MARGIN);
}

void wxAdvToolTipTextView::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
#ifdef __WXMSW__
    dc.SetFont(GetFont());
#endif
    dc.DrawText(m_text, TEXT_MARGIN, TEXT_MARGIN);
}

 
IMPLEMENT_ABSTRACT_CLASS(wxAdvToolTipText, wxAdvToolTip)
    
wxAdvToolTipView *wxAdvToolTipText::CreateView(
                            const wxPoint& position, wxRect& boundary)
{
    wxString text = GetText(position, boundary);
    if (!text.empty())
        return new wxAdvToolTipTextView(GetViewParent(), text);
    else
        return NULL;
}


#endif // wxUSE_POPUPWIN

