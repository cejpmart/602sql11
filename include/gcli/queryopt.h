#ifndef _QUERYOPT_H_
#define _QUERYOPT_H_
//
// CQueryOptDlg
//
#include "tlview.h"
extern char *optimize_xpm[];

#define ID_TREE        1
#define ID_EVAL        2

class CQueryOptDlg : public wxDialog
{
protected:

    wxImageList m_ImageList;
    wxButton    m_CloseBut;
    wxButton    m_EvalBut;
    wxButton    m_HelpBut;
    wbTreeCtrl  m_Tree;
    const char *m_Source;
    cdp_t       m_cdp;

    void OnEval(wxCommandEvent &event);
    void OnKeyDown(wxTreeEvent &event);
    void OnHelp(wxCommandEvent &event);

    void ShowOpt(bool Eval);

public:

    bool Create(wxWindow *Parent, cdp_t cdp, const char *Source);

    DECLARE_EVENT_TABLE()
};

#endif // _QUERYOPT_H_
