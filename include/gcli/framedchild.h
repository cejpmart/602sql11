#ifndef _FRAMEDCHILD_H_
#define _FRAMEDCHILD_H_

#include "wx/wx.h"

typedef enum {tbrNone, tbrNord, tbrNordWest, tbrWest, tbrSouthWest, tbrSouth, tbrSouthEast, tbrEast, tbrNordEast, tbrMove} TTBResizeFlag;
//
// Child window with frame
//
class wbFramedChild : public wxControl
{
protected:

    wxWindowBase *m_Child;          // Child control
    wxString      m_Title;          // Window title
    TTBResizeFlag m_Resize;         // Current resize or move operation
    wxPoint       m_MovePos;        // Position of mouse on start of move operation
    
    void OnSize(wxSizeEvent &event);

    void OnCreate(wxWindowCreateEvent &event);
    void OnPaint(wxPaintEvent &event);
    void OnMouseMove(wxMouseEvent &event);
    void OnEnterWindow(wxMouseEvent &event);
    void OnLeaveWindow(wxMouseEvent &event);
    void OnLeftMouseDown(wxMouseEvent &event);
    void OnLeftMouseUp(wxMouseEvent &event);
    
    void SetCursor(int x, int y);

public:

    wbFramedChild()
    {
        m_Child  = NULL;
        m_Resize = tbrNone;
    }

    wbFramedChild(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& title = wxEmptyString, const wxString& name = wxControlNameStr)
    {
        m_Child  = NULL;
        m_Resize = tbrNone;
        Create(parent, id, pos, size, style, validator, title, name);
    }

    bool Create(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& title = wxEmptyString, const wxString& name = wxControlNameStr);
    
    virtual void AddChild(wxWindowBase *child);

    virtual void SetTitle(const wxString &title)
    {
        m_Title = title;
        Refresh();
    }
    static int GetTitleHeight();

    DECLARE_EVENT_TABLE()
};

#endif // _FRAMEDCHILD_H_
