#ifndef _MYCTRLS_H_
#define _MYCTRLS_H_

class CMyGridEditor : public wxEvtHandler
{
protected:
    
    wxGrid *m_Grid;

public:

    CMyGridEditor(wxGrid *Grid)
    {
        m_Grid       = Grid;
    } 
};

class CMyGridTextEditor : public CMyGridEditor, public wxGridCellTextEditor
{
protected:

    DECLARE_EVENT_TABLE()

    void OnKeyDown(wxKeyEvent &event);

public:

    CMyGridTextEditor(wxGrid *Grid) : wxGridCellTextEditor(), CMyGridEditor(Grid){}
};

class CMyGridComboEditor : public CMyGridEditor, public wxGridCellChoiceEditor
{
protected:

    bool    m_InSetFocus;

    DECLARE_EVENT_TABLE()

#ifdef WINS
    bool IsDropped(){return(::SendMessage((HWND)GetControl()->GetHWND(), CB_GETDROPPEDSTATE, 0, 0) != 0);}
#else
    bool IsDropped(){return(false);}
#endif
    void OnSetFocus(wxFocusEvent &event);
    void OnKillFocus(wxFocusEvent &event);
    void OnIdle(wxIdleEvent& event);

public:

    CMyGridComboEditor(wxGrid *Grid, const wxArrayString& choices) : wxGridCellChoiceEditor(choices, true), CMyGridEditor(Grid)
    {
        m_InSetFocus = false;
    }
    virtual void StartingKey(wxKeyEvent& event);
};


#endif // _MYCTRLS_H_

