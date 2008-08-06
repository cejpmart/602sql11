///////////////////////////////////////////////////////////////////////////////
// Name:        wx/advtooltip.h
// Purpose:     wxAdvToolTip
// Author:      Vaclav Slavik
// Modified by:
// Created:     2004/06/30
// RCS-ID:      $Id: advtooltip.h,v 1.1 2007/08/30 11:36:00 drozd Exp $
// Copyright:   (c) 2004 Vaclav Slavik <vaclav.slavik@matfyz.cz>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ADVTOOLTIP_H_
#define _WX_ADVTOOLTIP_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    //#pragma interface "advtooltip.h"
#endif

#include "wx/popupwin.h"
#include "wx/timer.h"

#if wxUSE_POPUPWIN

class wxAdvToolTipView;
class wxAdvToolTipPopup;

// ----------------------------------------------------------------------------
// wxAdvToolTip
// ----------------------------------------------------------------------------

// Tooltip class
class wxAdvToolTip : public wxEvtHandler
{
public:
    // Creates the tooltip and associates it with 'window'.
    wxAdvToolTip(wxWindow *window);
    ~wxAdvToolTip();
 
protected:
    // Creates view that contains tooltip for given position and fills
    // 'boundary' with rectangle for which the tooltip is valid (it will
    // be hidden when the mouse leaves it). Returns NULL if no tooltip
    // should be displayed. The created tooltip must be children of the 
    // window returned by GetViewParent():
    virtual wxAdvToolTipView *CreateView(const wxPoint& position,
                                              wxRect& boundary) = 0;

    // Returns wxWindow that should be used by CreateView as view's parent:
    wxAdvToolTipPopup *GetViewParent();

    // Returns the associated window:
    wxWindow *GetWindow() const { return m_window; }

    // Shows the tooltip, called from event handlers
    virtual void PopupToolTip();

    // Returns true if tooltip can be shown now
    virtual bool CanPopupWindow() const;

private:
    void HandleAnyEvent(wxEvent& event);

    void OnTimer(wxTimerEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void OnActivate(wxActivateEvent& event);
    
    // associated window:
    wxWindow *m_window;
    // popup window used to implement the tooltip + the view:
    wxAdvToolTipPopup *m_popup;
    wxAdvToolTipView  *m_view;

    wxTimer *m_timer;

    DECLARE_EVENT_TABLE()
    DECLARE_ABSTRACT_CLASS(wxAdvToolTip)
};

// ----------------------------------------------------------------------------
// wxAdvToolTipView
// ----------------------------------------------------------------------------

// wxAdvToolTipView is used to render tooltip's content:
class wxAdvToolTipView : public wxWindow
{
public:
    wxAdvToolTipView(wxAdvToolTipPopup *parent);

    void SetBoundaryRect(const wxRect& rect);

protected:
    wxAdvToolTipPopup *m_popup;
    wxRect             m_boundary;
    
private:
    void OnEraseBackground(wxEraseEvent& event);
    void OnMouseEvent(wxMouseEvent& event);

    DECLARE_EVENT_TABLE()   
    DECLARE_ABSTRACT_CLASS(wxAdvToolTipView)
};


// ----------------------------------------------------------------------------
// wxAdvToolTipText
// ----------------------------------------------------------------------------

// Simple tooltip class that displays text-only tooltips:
class wxAdvToolTipText : public wxAdvToolTip
{
public:
    wxAdvToolTipText(wxWindow *window)
        : wxAdvToolTip(window) {}

protected:
    // Called before the tooltip is shown, should return tooltip for given
    // position and fill-in 'boundary' with rectangle for which the tooltip
    // is valid. No tooltip is shown if empty string is returned.
    virtual wxString GetText(const wxPoint& position, wxRect& boundary) = 0;
    
    wxAdvToolTipView *CreateView(const wxPoint& position,
                                      wxRect& boundary);

    DECLARE_ABSTRACT_CLASS(wxAdvToolTipText)
};


#endif // wxUSE_POPUPWIN

#endif // _WX_ADVTOOLTIP_H_

