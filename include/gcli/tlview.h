#ifndef _TLVIEW_H_
#define _TLVIEW_H_

#include "wx/treectrl.h"
#include "wx/listctrl.h"
#include "wx/dragimag.h"

enum CPHitResult 
{
    CPHR_NOWHERE,   // Mimo polozky
    CPHR_ONITEM,    // Na polozce
    CPHR_ABOVE,     // U horniho okraje okna, bude potreba zarolovat
    CPHR_BELOW      // U spodniho okraje okna, bude potreba zarolovat
};

class wbTreeCtrl : public wxTreeCtrl
{
public:

    CPHitResult DragTest(wxCoord x, wxCoord y, wxTreeItemId *pItem);

protected:

    void OnKeyDown(wxKeyEvent &event);

#ifdef LINUX

    wxTreeItemId m_LastHit;    // Na LINUXU vraci HitTest wxTREE_HITTEST_NOWHERE i kdyz je mys mezi dvema polozkama
    
    void OnCreate(wxWindowCreateEvent &event);
	void OnRightClick(wxMouseEvent &event);

    wxTreeItemId GetLastItem() const
    {
        wxTreeItemId id;
        for (id = GetRootItem(); IsExpanded(id); id = GetLastChild(id));
        return id;
    }
   

#endif  // LINUX

    DECLARE_EVENT_TABLE()
};

class wbListCtrl : public wxListCtrl
{
public:

    CPHitResult DragTest(wxCoord x, wxCoord y, long *pItem);

#ifdef LINUX

protected:
    
    void OnCreate(wxWindowCreateEvent &event);

    DECLARE_EVENT_TABLE()

#endif  // LINUX
};


#ifdef WINS

inline void UpdateSize(wxWindow *Wnd)
{}

class CWBDragImage : public wxDragImage
{
protected:

    bool m_Visible;
    
#ifdef LINUX
    void CreateImage(const wxWindow &Wnd, const wxString &Text, wxImageList *ImageList, int ImageIndex);
#endif        

public:

    static CWBDragImage *Image;

#ifdef WINS    
    CWBDragImage(const wxTreeCtrl& treeCtrl, wxTreeItemId& id) : wxDragImage(treeCtrl, id){Image = this; m_Visible = false;}
    CWBDragImage(const wxListCtrl& ListCtrl, long id)          : wxDragImage(ListCtrl, id){Image = this; m_Visible = false;}
#else
    CWBDragImage(const wxTreeCtrl& treeCtrl, wxTreeItemId& id);
    CWBDragImage(const wxListCtrl& ListCtrl, long id);
#endif   
 
    CWBDragImage(const wxBitmap& image)                        : wxDragImage(image)       {Image = this; m_Visible = false;}
    ~CWBDragImage()
    {
#ifdef LINUX    
        wxWindow *Win = wxFindWindowAtPoint(wxGetMousePosition());
#ifdef _DEBUG
        wxLogDebug(wxT("~CWBDragImage %08X  %s"), Win, Win ? Win->GetName().c_str() : wxT("None"));
#endif            
        if (Win)
            Win->Refresh();
#endif            
        Image = NULL;
    }
    void Show()
    {
        if (this != NULL && !m_Visible)
        {
#ifdef _DEBUG
//            wxLogDebug("CWBDragImage::Show()");
#endif            
            wxDragImage::Show();
            m_Visible = true;
        }
    }
    void Hide()
    {
        if (this != NULL && m_Visible)
        {
#ifdef _DEBUG
//            wxLogDebug("CWBDragImage::Hide()");
#endif            
            wxDragImage::Hide();
            m_Visible = false;
        }
    }
};

#else

void UpdateSize(wxWindow *Wnd);


#endif  // WINS
#endif  // _TLVIEW_H_
