//
// Child window with frame - used in query designer
//
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#include "wx/caret.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop

#include "framedchild.h"

static wxCoord TitleHeight;

int wbFramedChild::GetTitleHeight(){return(TitleHeight);}

BEGIN_EVENT_TABLE(wbFramedChild, wxControl)
    EVT_SIZE(wbFramedChild::OnSize)
    EVT_WINDOW_CREATE(wbFramedChild::OnCreate)
    EVT_PAINT(wbFramedChild::OnPaint)
    EVT_LEFT_DOWN(wbFramedChild::OnLeftMouseDown)
    EVT_ENTER_WINDOW(wbFramedChild::OnEnterWindow) 
    EVT_LEAVE_WINDOW(wbFramedChild::OnLeaveWindow) 
    EVT_MOTION(wbFramedChild::OnMouseMove)
    EVT_LEFT_UP(wbFramedChild::OnLeftMouseUp)
END_EVENT_TABLE()
//
// Creates window
//
bool wbFramedChild::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& title, const wxString& name)
{
    // IF Title height not defined, calculate title height
    if (!TitleHeight)
    {
        wxScreenDC dc;
#ifdef WINS
        dc.SetFont(*wxNORMAL_FONT);
#else
        dc.SetFont(*wxSMALL_FONT);
#endif
        wxCoord w;
        dc.GetTextExtent(wxT("pqgyČÝŮ"), &w, &TitleHeight);
#ifdef WINS
        TitleHeight += 2;
#endif
    }
    m_Title = title;
    // IF border not defined, set wxRAISED_BORDER
    if ((style & (wxSIMPLE_BORDER | wxDOUBLE_BORDER | wxSUNKEN_BORDER | wxRAISED_BORDER | wxSTATIC_BORDER)) == 0)
        style |= wxRAISED_BORDER;

    return(wxControl::Create(parent, id, pos, size, style, validator, name));
}
//
// Sets window child control
//
void wbFramedChild::AddChild(wxWindowBase *child)
{
    wxControl::AddChild(child);
    m_Child = child;
}
//
// Resise event handler - resizes child control, updates parent window
//
void wbFramedChild::OnSize(wxSizeEvent &event)
{
    if (m_Child)
    {
        wxSize Size = GetClientSize();
        m_Child->SetSize(1, TitleHeight + 2, Size.x - 2, Size.y - TitleHeight - 3);
    }
    GetParent()->Refresh();
    event.Skip();
}
//
// After create event handler
//
void wbFramedChild::OnCreate(wxWindowCreateEvent &event)
{
    wxWindowList     &List = GetChildren();
    wxWindowListNode *Node = List.GetFirst();
    // IF child window exists, resize it
    if (Node)
    {
        wxSizeEvent sevent(GetSize());
        OnSize(sevent);
        event.Skip();
    }
    // ELSE repeat event
    else
    {
        AddPendingEvent(event);
    }
}
//
// Paint event handler
//
void wbFramedChild::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);
    
    wxSize Size = GetClientSize();
    wxColour BackCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    // Draw window frame
    dc.SetPen(*wxMEDIUM_GREY_PEN);
    dc.DrawRectangle(0, 0, Size.x, Size.y); 
    wxBrush Brush(BackCol);
    // Draw caption rectangle
    dc.SetBrush(Brush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(1, 1, Size.x - 2, TitleHeight); 
    dc.SetTextBackground(BackCol);
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    // Draw title
#ifdef WINS
    dc.SetFont(*wxNORMAL_FONT);
#else
    dc.SetFont(*wxSMALL_FONT);
#endif
    dc.DrawText(m_Title, 4, 1);
}
//
// Left mouse event handler
//
void wbFramedChild::OnLeftMouseDown(wxMouseEvent &event)
{
    wxSize Size = GetClientSize();
    // Set operation type according mouse position
    if (event.GetX() < 2)
    {
        if (event.GetY() < 10)
            m_Resize = tbrNordWest;
        else if (event.GetY() < Size.y - 10)
            m_Resize = tbrWest;
        else
            m_Resize = tbrSouthWest;
    }
    else if (event.GetX() > Size.x - 3)
    {
        if (event.GetY() < 10)
            m_Resize = tbrNordEast;
        if (event.GetY() < Size.y - 10)
            m_Resize = tbrEast;
        else
            m_Resize = tbrSouthEast;
    }
    else if (event.GetY() < 2)
    {
        if (event.GetX() < 10)
            m_Resize = tbrNordWest;
        else if (event.GetX() > Size.x - 10)
            m_Resize = tbrNordEast;
        else
            m_Resize = tbrNord;
    }
    else if (event.GetY() > Size.y - 3)
    {
        if (event.GetX() < 10)
            m_Resize = tbrSouthWest;
        else if (event.GetX() > Size.x - 10)
            m_Resize = tbrSouthEast;
        else
            m_Resize = tbrSouth;
    }
    else if (event.GetY() < TitleHeight)
    {
        m_MovePos.x = event.GetX();
        m_MovePos.y = event.GetY();
        m_Resize    = tbrMove;
    }

    // IF some operation set, capture mouse
    if (m_Resize)
        CaptureMouse();
}
//
// Sets cursor according mouse position
//
void wbFramedChild::SetCursor(int x, int y)
{
    wxSize cSize = GetClientSize();
    
    int cId; 
    if (x < 2)
    {
        if (y < 10)
            cId = wxCURSOR_SIZENWSE;
        else if (y < cSize.y - 10)
            cId = wxCURSOR_SIZEWE;
        else
            cId = wxCURSOR_SIZENESW;
    }
    else if (x > cSize.x - 3)
    {
        if (y < 10)
            cId = wxCURSOR_SIZENESW;
        else if (y < cSize.y - 10)
            cId = wxCURSOR_SIZEWE;
        else
            cId = wxCURSOR_SIZENWSE;
    }
    else if (y < 2)
    {
        if (x < 10)
            cId = wxCURSOR_SIZENWSE;
        else if (x < cSize.x - 10)
            cId = wxCURSOR_SIZENS;
        else
            cId = wxCURSOR_SIZENESW;
    }
    else if (y > cSize.y - 3)
    {
        if (x < 10)
            cId = wxCURSOR_SIZENESW;
        else if (x < cSize.x - 10)
            cId = wxCURSOR_SIZENS;
        else
            cId = wxCURSOR_SIZENWSE;
    }
    else 
    {
        cId = wxCURSOR_ARROW;
    }
    wxCursor Cursor(cId);
    wxSetCursor(Cursor);
    wxWindow::SetCursor(Cursor);
}
//
// Enter window event handler - sets cursor according mouse position
//
void wbFramedChild::OnEnterWindow(wxMouseEvent &event)
{
    SetCursor(event.GetX(), event.GetY());
} 
//
// Leave window event handler - sets standard cursor
//
void wbFramedChild::OnLeaveWindow(wxMouseEvent &event)
{
    wxSetCursor(*wxSTANDARD_CURSOR);
} 
//
// Mouse move event handler
//
void wbFramedChild::OnMouseMove(wxMouseEvent &event)
{
    wxSize Size  = GetSize();
    wxSize cSize = GetClientSize();
    wxPoint Pos  = GetPosition();
    // IF no resize/move, set cursor according mouse position
    if (m_Resize == tbrNone)
    {
        SetCursor(event.GetX(), event.GetY());
    }
    // IF move operation,
    else if (m_Resize == tbrMove)
    {
        // Calculate new window position
        Pos.x  += event.GetX() - m_MovePos.x;
        Pos.y  += event.GetY() - m_MovePos.y;
        if (Pos.x < 0)
            Pos.x = 0;
        if (Pos.y < 0)
            Pos.y = 0;
        // Move to new window position
        Move(Pos);
#ifdef LINUX       
        // Linux doesn't generate wxMoveEvent
        Pos = ClientToScreen(wxPoint(0, 0));
        Pos = GetParent()->ScreenToClient(Pos);
        wxMoveEvent mevent(Pos, GetId());
        mevent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(mevent);
#endif        
    }
    // ELSE resize oparation
    else
    {
        // Calculate new size
        switch (m_Resize)
        {
        case tbrSouthWest:
            Size.y += event.GetY() - cSize.y;
        case tbrWest:
            Pos.x  += event.GetX();
            Size.x -= event.GetX();
            break;
        case tbrSouthEast:
            Size.y += event.GetY() - cSize.y;
        case tbrEast:
            Size.x += event.GetX() - cSize.x;
            break;
        case tbrSouth:
            Size.y += event.GetY() - cSize.y;
            break;
        case tbrNordWest:
            Size.x -= event.GetX();
            Size.y -= event.GetY();
            Pos.x  += event.GetX();
            Pos.y  += event.GetY();
            break;
        case tbrNordEast:
            Size.y -= event.GetY();
            Size.x += event.GetX() - cSize.x;
            Pos.y  += event.GetY();
            break;
        case tbrNord:
            Size.y -= event.GetY();
            Pos.y  += event.GetY();
            break;    
        }
        // Resize window
        SetSize(Pos.x, Pos.y, Size.x, Size.y);         
    }
    event.Skip();    
}
//
// Left mouse up event handler
//
void wbFramedChild::OnLeftMouseUp(wxMouseEvent &event)
{
    // IF resize/move operation, release capture, clear resize/move flag
    if (m_Resize)
    {
        ReleaseMouse();
        m_Resize = tbrNone;
    }
    event.Skip();
}
