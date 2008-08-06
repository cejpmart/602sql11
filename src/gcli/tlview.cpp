//
// Osetruje chyby nebo nedostatky wxTreeCtrl, wxListCtrl a wxDragImage, nebo 
// se snazi prekryt rozdily implementaci na Windows a Linuxu
//
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

#include "tlview.h"


BEGIN_EVENT_TABLE(wbTreeCtrl, wxTreeCtrl)
    EVT_KEY_DOWN(wbTreeCtrl::OnKeyDown)
#ifdef LINUX
    EVT_WINDOW_CREATE(wbTreeCtrl::OnCreate)
	EVT_RIGHT_UP(wbTreeCtrl::OnRightClick) 
#endif
END_EVENT_TABLE()    
//
// wxTreeCtrl implicitne nepropousti zpravu o stisknuti klavesy, ani nenabizi nejakou odpovidajici udalost,
// takze pokud ma focus wxTreeCtrl, rodicovske okno nema sanci se o stisknuti klavesy dozvedet
//
void wbTreeCtrl::OnKeyDown(wxKeyEvent &event)
{
    event.ResumePropagation(1);
    event.Skip();
}

#ifdef LINUX
//
// wxTreeCtrl ma na Linuxu defaultne zbytecne velky font a pak je toho ve stromu malo videt
//
void wbTreeCtrl::OnCreate(wxWindowCreateEvent &event)
{
#ifdef STOP  // the font is used-defined    
    wxFont Font = GetFont();
    if (Font.GetPointSize() > 8)
    {
        Font.SetPointSize(8);
        SetFont(Font);
    }    
#endif
   // reduce the spacing between lines:    
    wxClientDC dc(this);
    PrepareDC( dc );
    wxCoord w, h;
    dc.GetTextExtent(wxT("ĚÓg_y"), &w, &h);
    if (m_lineHeight > h+1)
        m_lineHeight = h+1;
}
//
// wxTreeCtrl::HitTest vraci na Windows a na Linuxu uplne jine vysledky, DragTest se snazi rozdily srovnat,
// aby se pri D&D dalo zjistit, kde vlastne jsem. Napr. na Linuxu jsou mezi polozkami mezery, pro ktere HitTest
// vraci neplatne wxTreeItemId a wxTREE_HITTEST_NOWHERE a kdyz jsem v horni casti okna, potreboval bych vedet,
// nad kterou polozkou, abych mohl zarolovat na tu predchozi, tak jsem nad mezerou. Obchazim to tim, ze v m_LastHit
// si pamatuju posledni platnou polozku, nad kterou jsem byl;
// 
CPHitResult wbTreeCtrl::DragTest(wxCoord x, wxCoord y, wxTreeItemId *pItem)
{
    pItem->Unset();
    int Flags;
    wxTreeItemId Item = HitTest(wxPoint(x, y), Flags);
    if (Item.IsOk())
        m_LastHit = Item;
#ifdef _DEBUG
    //wxLogDebug(wxT("wbTreeCtrl::DragTest  %d  %s"), Flags, Item.IsOk() ? GetItemText(Item).c_str() : wxT("None"));
#endif    
    // IF mys na zacatku okna
    if (y < 5)
    {
        int sx, sy;
        // IF scroll na zacatku
        GetViewStart(&sx, &sy);
        if (sy <= 0) 
        {
            m_LastHit.Unset();
            return(CPHR_NOWHERE);
        }
        *pItem = m_LastHit;
        return(CPHR_ABOVE);
    }
    wxTreeItemId Last = GetLastItem();
    wxRect Rect;
    GetBoundingRect(Last, Rect);
    // IF za posledni polozkou
    if (y > Rect.y + Rect.height)
    {
        m_LastHit.Unset();
        return(CPHR_NOWHERE);
    }
    int Width, Height;
    GetClientSize(&Width, &Height);
    // IF mys na konci okna
    if (y > Height - 5)
    {
        int sx, sy, vx, vy, ux, uy;
        GetViewStart(&sx, &sy);
        GetVirtualSize(&vx, &vy);
        GetScrollPixelsPerUnit(&ux, &uy);
        // IF scroll na konci
        if (sy * uy + Height >= vy)
        {
            m_LastHit.Unset();
            return(CPHR_NOWHERE);
        }
        *pItem = m_LastHit;
        return(CPHR_BELOW);
    }
    *pItem = m_LastHit;
    return(CPHR_ONITEM);
}
//
// wxTreeCtrl na Linuxu nepropousti zpravu o stisknuti tlacitka, ani neposkytuje odpovidajici udalost
//
void wbTreeCtrl::OnRightClick(wxMouseEvent &event)
{
    event.ResumePropagation(1);
    event.Skip();
}

#else  // WINS

CPHitResult wbTreeCtrl::DragTest(wxCoord x, wxCoord y, wxTreeItemId *pItem)
{
    int Flags;
    *pItem = HitTest(wxPoint(x, y), Flags);
    if (!pItem->IsOk()) 
        return(CPHR_NOWHERE);
    // IF mys na zacatku
    if (y < 5)
        return(CPHR_ABOVE);
    int Width, Height;
    GetClientSize(&Width, &Height);
    // IF mys na konci
    if (y > Height - 5)
        return(CPHR_BELOW);
    return(CPHR_ONITEM);
}

#endif // LINUX

#ifdef LINUX

BEGIN_EVENT_TABLE(wbListCtrl, wxListCtrl)
    EVT_WINDOW_CREATE(wbListCtrl::OnCreate)
END_EVENT_TABLE()    
//
// wxListCtrl ma na Linuxu defaultne zbytecne velky font a pak je toho ve seznamu malo videt
//
void wbListCtrl::OnCreate(wxWindowCreateEvent &event)
{
    if (m_mainWin)
        UpdateSize(this);
}

CPHitResult wbListCtrl::DragTest(wxCoord x, wxCoord y, long *pItem)
{
#ifdef _DEBUG
    //wxLogDebug(wxT("wbListCtrl::DragTest IN"));
#endif    
    *pItem = -1;
    int Flags;
    long Item = HitTest(wxPoint(x, y), Flags);
    // IF mys na zacatku
    if (y < 5)
    {
        //int sx, sy;
        // IF scroll na zacatku
        //Owner()->GetViewStart(&sx, &sy);
        //if (sy <= 0) 
        if (Item <= 0)
            return(CPHR_NOWHERE);
        *pItem = Item; //Item.IsOk() ? Item : m_Item;
        return(CPHR_ABOVE);
    }
    wxRect Rect;
    GetItemRect(0, Rect);
    // IF za posledni polozkou
    if (y > GetItemCount() * Rect.height)
        return(CPHR_NOWHERE);                
    int Width, Height;
#ifdef WINS
    GetClientSize(&Width, &Height);
#else
    ((wxWindow *)m_mainWin)->GetClientSize(&Width, &Height);
#endif
#ifdef _DEBUG
    //wxLogDebug(wxT("HitTest %d %d     %d"), x, y, Height);
#endif    
    // IF mys na konci
    if (y > Height - 5)
    {
        //int sx, sy, vx, vy, ux, uy;
        //Owner()->GetViewStart(&sx, &sy);
        //Owner()->GetVirtualSize(&vx, &vy);
        //Owner()->GetScrollPixelsPerUnit(&ux, &uy);
        // IF scroll na konci
        //if (sy * uy + Height >= vy)
        if (Item == -1)
            return(CPHR_NOWHERE);
        if (Item >= GetItemCount() - 1)
        {
            GetItemRect(Item, Rect);
            if (Rect.y + Rect.height - m_headerHeight <= Height)
                return(CPHR_NOWHERE);
        }
        *pItem = Item; //Item.IsOk() ? Item : m_Item;
        return(CPHR_BELOW);
    }
    *pItem = Item; //Item.IsOk() ? Item : m_Item;
#ifdef _DEBUG
    //wxLogDebug(wxT("wbListCtrl::DragTest OUT"));
#endif    
    return(CPHR_ONITEM);
}

#else // WINS

CPHitResult wbListCtrl::DragTest(wxCoord x, wxCoord y, long *pItem)
{
    int Flags;
    *pItem = HitTest(wxPoint(x, y), Flags);
    if (*pItem == -1) 
        return(CPHR_NOWHERE);
    // IF mys na zacatku
    if (y < 5)
        return(CPHR_ABOVE);
    int Width, Height;
    GetClientSize(&Width, &Height);
    // IF mys na konci
    if (y > Height - 5)
        return(CPHR_BELOW);
    return(CPHR_ONITEM);
}

#endif // LINUX

#ifdef LINUX

void UpdateSize(wxWindow *Wnd)
{
#ifdef STOP  // the font is used-defined    
    wxFont Font = Wnd->GetFont();
    if (Font.GetPointSize() > 8)
    {
        Font.SetPointSize(8);
        Wnd->SetFont(Font);
    }
#endif
}
//
// CWBDragImage reprezentuje obrazek polozky, ktery se pri D&D pohybuje po obrazovce, CWBDragImage hlida
// sparovani Show a Hide, aby obrazek nezustal zobrazeny i po dropu nebo, aby naopak nezmizel kurzor.
// wxDragImage::BeginDrag si privlastnuje mouse capture a to ve wx2.8.0 na Linuxu koliduje s implementaci D&D
// a tim zpusobuje skoro uplne zatuhnuti pocitace. Proto je CWBDragImage pouzit jen na Windows.
//
#ifdef NEVER

CWBDragImage::CWBDragImage(const wxTreeCtrl& treeCtrl, wxTreeItemId& id)
{
    Image     = this; 
    m_Visible = false;
    CreateImage(treeCtrl, treeCtrl.GetItemText(id), treeCtrl.GetImageList(), treeCtrl.GetItemImage(id));
}

CWBDragImage::CWBDragImage(const wxListCtrl& ListCtrl, long id)
{
    Image     = this; 
    m_Visible = false;
    wxListItem iinfo;  
    iinfo.SetId(id);  
    iinfo.m_mask  = wxLIST_MASK_IMAGE;
    ListCtrl.GetItem(iinfo);
    CreateImage(ListCtrl, ListCtrl.GetItemText(id), ListCtrl.GetImageList(wxIMAGE_LIST_SMALL), iinfo.m_image);
}

void CWBDragImage::CreateImage(const wxWindow &Wnd, const wxString &Text, wxImageList *ImageList, int ImageIndex)
{
    wxFont Font = Wnd.GetFont();

    long w, h;
    wxScreenDC dc;
    dc.SetFont(Font);
    dc.GetTextExtent(Text, &w, &h);
    dc.SetFont(wxNullFont);
    if (ImageList)
    {
        w += 20;
        if (h < 16)
            h = 16;
    }
    int x = 1;

    wxMemoryDC dc2;

    // Sometimes GetTextExtent isn't accurate enough, so make it longer
    wxBitmap bitmap(w + 2, h + 2);
    wxColour NoneCol(0, 128, 128);
    dc2.SelectObject(bitmap);
    dc2.SetFont(Font);
    dc2.SetBackground(wxBrush(NoneCol)); //*wxWHITE_BRUSH);
    dc2.Clear();
    dc2.SetBackgroundMode(wxTRANSPARENT);
    if (ImageList)
    {
        ImageList->Draw(ImageIndex, dc2, 1, 1, wxIMAGELIST_DRAW_TRANSPARENT, true);
        x = 20;
    }

    dc2.SetTextForeground(*wxBLACK);
    dc2.DrawText(Text, x, 1);
    dc2.SelectObject(wxNullBitmap);

    wxImage image = bitmap.ConvertToImage();
    image.SetMaskColour(0, 128, 128); //255, 255, 255);
    bitmap = wxBitmap(image);
    
    Create(bitmap, wxNullCursor);
}

#endif // NEVER
#endif // LINUX
