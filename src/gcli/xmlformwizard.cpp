/////////////////////////////////////////////////////////////////////////////
// Name:        xmlformwizard.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     27/02/2006 11:04:13
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////
//
// Generates XML form description, description is formed as tree of CXMLFormStruct, 
// CXMLFormStructLevel and CXMLFormStructElem structures. NewXMLForm function in 
// 602xml dll produces XML form file based on this structure set.
//
#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
//#pragma implementation "xmlformwizard.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "xmlformwizard.h"

#include "bmps/wizard.xpm"
#include "bmps/Form1.xpm"
#include "bmps/Form2.xpm"
#include "bmps/Form3.xpm"
#include "bmps/Form4.xpm"
#include "wx/file.h"
/*!
 * CXMLFormWizard type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CXMLFormWizard, wxWizard )

/*!
 * CXMLFormWizard event table definition
 */

BEGIN_EVENT_TABLE( CXMLFormWizard, wxWizard )

////@begin CXMLFormWizard event table entries
    EVT_WIZARD_FINISHED( ID_XMLFORMWIZARD, CXMLFormWizard::OnXmlformwizardFinished )
    EVT_WIZARD_HELP( ID_XMLFORMWIZARD, CXMLFormWizard::OnXmlformwizardHelp )

////@end CXMLFormWizard event table entries

END_EVENT_TABLE()

/*!
 * CXMLFormWizard constructors
 */

CXMLFormWizard::CXMLFormWizard( )
{
}

CXMLFormWizard::CXMLFormWizard( wxWindow* parent, wxWindowID id, const wxPoint& pos )
{
    Create(parent, NULL, id, pos);
}

CXMLFormWizard::CXMLFormWizard( wxWindow* parent, cdp_t cdp, const char *DAD)
{
    m_cdp   = cdp;
    Create(parent, DAD);
}
/*!
 * CXMLFormWizard creator
 */

bool CXMLFormWizard::Create( wxWindow* parent, const char *DAD, wxWindowID id, const wxPoint& pos )
{
////@begin CXMLFormWizard member initialisation
    LayoutPage = NULL;
    SourcePage = NULL;
    DestPage = NULL;
    StructPage = NULL;
////@end CXMLFormWizard member initialisation

////@begin CXMLFormWizard creation
    SetExtraStyle(GetExtraStyle()|wxWIZARD_EX_HELPBUTTON);
    wxBitmap wizardBitmap(GetBitmapResource(wxT("../cli/Debug95/wizard.xpm")));
    wxWizard::Create( parent, id, _("XML FormWizard"), wizardBitmap, pos, wxDEFAULT_DIALOG_STYLE|wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    CreateControls();
////@end CXMLFormWizard creation
    SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);
    // IF the wizard is invoked by DAD popup menu, page sequence will be LayoutPage - StructPage
    if (DAD)
    {
        if (!LoadDad(DAD))
            return false;
        LayoutPage->SetNext(StructPage);
        StructPage->SetPrev(LayoutPage);
        m_FromDAD = true;
    }
    // ELSE the wizard is invoked by form popup menu, page sequence will be LayoutPage - SourcePage - StructPage
    else
    {
        m_FromDAD = false;
        LayoutPage->SetNext(SourcePage);
        SourcePage->SetNext(StructPage);
        StructPage->SetPrev(SourcePage);
        SourcePage->SetPrev(LayoutPage);
    }
    m_DefaultDAD = false;

    return true;
}

/*!
 * Control creation for CXMLFormWizard
 */

void CXMLFormWizard::CreateControls()
{    
////@begin CXMLFormWizard content construction
    wxWizard* itemWizard1 = this;

    LayoutPage = new CXMLFormWizardLayoutPage( itemWizard1 );

    itemWizard1->FitToPage(LayoutPage);
    SourcePage = new CXMLFormWizardSourcePage( itemWizard1 );

    itemWizard1->FitToPage(SourcePage);
    DestPage = new CXMLFormWizardDestPage( itemWizard1 );

    itemWizard1->FitToPage(DestPage);
    StructPage = new CXMLFormWizardStructPage( itemWizard1 );

    itemWizard1->FitToPage(StructPage);
    wxWizardPageSimple* lastPage = NULL;
    if (lastPage)
        wxWizardPageSimple::Chain(lastPage, LayoutPage);
    lastPage = LayoutPage;
    if (lastPage)
        wxWizardPageSimple::Chain(lastPage, SourcePage);
    lastPage = SourcePage;
    if (lastPage)
        wxWizardPageSimple::Chain(lastPage, DestPage);
    lastPage = DestPage;
    if (lastPage)
        wxWizardPageSimple::Chain(lastPage, StructPage);
    lastPage = StructPage;
////@end CXMLFormWizard content construction
}
/*!
 * Should we show tooltips?
 */

bool CXMLFormWizard::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CXMLFormWizard::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
    return wxBitmap(/*xmlformwizard*/);
}

/*!
 * Get icon resources
 */

wxIcon CXMLFormWizard::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CXMLFormWizard icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CXMLFormWizard icon retrieval
}

/*!
 * Runs the wizard.
 */

bool CXMLFormWizard::Run()
{
    wxWizardPage* startPage = NULL;
    wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
    while (node)
    {
        wxWizardPage* startPage = wxDynamicCast(node->GetData(), wxWizardPage);
        if (startPage) return RunWizard(startPage);
        node = node->GetNext();
    }
    return false;
}

/*!
 * wxEVT_WIZARD_FINISHED event handler for ID_XMLFORMWIZARD
 */

void CXMLFormWizard::OnXmlformwizardFinished( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_FINISHED event handler for ID_XMLFORMWIZARD in CXMLFormWizard.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_FINISHED event handler for ID_XMLFORMWIZARD in CXMLFormWizard. 

    // IF source is table, stored query or SELECT statement
    if (!SourcePage->DADRB->GetValue())
    {
        tobjnum obj = NOOBJECT;
        // Compile default DAD definition
        char *src   = ::link_make_source(GetDadTree(), false, m_cdp); 
        BOOL  Err   = src == NULL;
        if (!Err)
        {
            // Insert new object
            wxCharBuffer DADName = GetDAD().mb_str(*m_cdp->sys_conv);
            Err = cd_Insert_object(m_cdp, DADName, CATEG_PGMSRC, &obj);
            if (Err)
            {
                if (cd_Sz_error(m_cdp) == KEY_DUPLICITY)
                    unpos_box(OBJ_NAME_USED);
                else
                    cd_Signalize(m_cdp);
            }
            else
            {
                int sz = (int)strlen(src);
                Err = cd_Write_var(m_cdp, OBJ_TABLENUM, obj, OBJ_DEF_ATR, NOINDEX, 0, sz, src) ||
                      cd_Write_len(m_cdp, OBJ_TABLENUM, obj, OBJ_DEF_ATR, NOINDEX, sz);
                if (Err)
                    cd_Signalize(m_cdp);
                else
                {
                    Set_object_flags(m_cdp,  obj,          CATEG_PGMSRC, CO_FLAG_XML);
                    Add_new_component(m_cdp, CATEG_PGMSRC, m_cdp->sel_appl_name, DADName, obj, CO_FLAG_XML, "", true);
                }
            }
        }
        if (src)
            corefree(src);
        if (Err)
        {
            if (obj != NOOBJECT)
                cd_Delete(m_cdp, OBJ_TABLENUM, obj);
            event.Skip(true);
            event.Veto();
        }
    }
}
//
// Returns DAD name
//
wxString CXMLFormWizard::GetDAD() const
{
    return SourcePage->GetDAD();
}
//
// Returns DAD tree
//
t_dad_root *CXMLFormWizard::GetDadTree() const
{
    return StructPage->DadTree;
}

/*!
 * wxEVT_WIZARD_HELP event handler for ID_XMLFORMWIZARD
 */
void CXMLFormWizard::OnXmlformwizardHelp( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_HELP event handler for ID_XMLFORMWIZARD in CXMLFormWizard.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_HELP event handler for ID_XMLFORMWIZARD in CXMLFormWizard. 
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_xmlform.html"));
}
//
// Replaces speces with '_' in DAD element names
//
void ReplaceSpace(dad_element_node *Node)
{
    for (char *p = Node->el_name; *p; p++)
        if (*p == ' ')
            *p = '_';
    for (dad_element_node *el = Node->sub_elements; el; el = el->next)
        ReplaceSpace(el);
}
//
// Generates DAD tree for Query statement, returns true on success
//
const char def_prolog[] = "version=\"1.0\" encoding=\"UTF-8\"";;

bool CXMLFormWizard::GetQueryDAD(const char *Query)
{
    bool  OK = false;
    uns32 Handle;
   
    // Prepare statement
    if (!cd_SQL_prepare(m_cdp, Query, &Handle))
    {
        d_table *td = NULL;
        // Compile statement
        if (!cd_SQL_get_result_set_info(m_cdp, Handle, 0, (d_table **)&td))
        {
            // Create DAD tree
            t_dad_root *DadTree = link_new_t_dad_root(m_cdp->sys_charset);
            DadTree->prolog     = link_new_chars((int)strlen(def_prolog) + 1);
            if (DadTree->prolog)
                strcpy(DadTree->prolog, def_prolog);
            DadTree->root_element = link_new_dad_element_node();
            if (DadTree->root_element)
                strcpy(DadTree->root_element->el_name, "root");
            DadTree->sql_stmt = link_new_chars((int)strlen(Query) + 1);
            strcpy(DadTree->sql_stmt, Query);
            DadTree->explicit_sql_statement = true;
            // Build DAD tree
            create_default_design(m_cdp, td, DadTree, NULL, "");
            // Replace spaces in DAD element names
            ReplaceSpace(DadTree->root_element);
            // Set DAD tree in form descriptor
            SetDadTree(DadTree);
            release_table_d(td);
            OK = true;
        }
        cd_SQL_drop(m_cdp, Handle);
    }
    return OK;
}
//
// Parses DAD tree from stored DAD and stores it to form descriptor
// Input:   DadName - DAD object name
//          Dst     - false = source DAD for report or update form, true = destination DAD for insert or update form
// Returns: true on success
//
bool CXMLFormWizard::LoadDad(const char *DadName, bool Dst)
{
    bool Result = false;
    tobjnum objnum;
    // Find DAD object
    if (!cd_Find2_object(m_cdp, DadName, NULL, CATEG_PGMSRC, &objnum))
    {
        // Load DAD definition
        char *dad = cd_Load_objdef(m_cdp, objnum, CATEG_PGMSRC, NULL);
        dad = link_convert2utf8(dad);
        if (dad)
        {
            t_ucd_cl *cdcl = link_new_t_ucd_cl_602(m_cdp);
            if (cdcl)
            {
                if (link_init_platform_utils(cdcl))
                {
                    // Parse DAD
                    t_dad_root *DadRoot = link_parse(cdcl, dad);
                    if (DadRoot)
                    {
                        // Set DAD tree in form descriptor
                        SetDadTree(DadRoot, Dst);
                        // Set DAD name in form descriptor
                        if (Dst)
                            strcpy(StructPage->DstDadName, DadName);
                        else
                            strcpy(StructPage->DadName, DadName);
                        Result = true;
                    }
                    link_close_platform_utils();
                }
                link_delete_t_ucd_cl(cdcl);
            }
            corefree(dad);
        }
    }
    if (!Result)
        cd_Signalize(m_cdp);
    return Result;
}
//
// Sets DAD tree in form descriptor
//
void CXMLFormWizard::SetDadTree(t_dad_root *aDadTree, bool Dst)
{
    StructPage->SetDadTree(aDadTree, Dst);
}
//
// Returns form layout struct
//
CXMLFormStruct *CXMLFormWizard::GetStruct()
{
    CXMLFormStruct *Result = (CXMLFormStruct *)StructPage;
    int i;
    // Set current form layout type
    for (i = 0; i < 3; i++)
    {
        if (((wxRadioButton *)LayoutPage->FindWindow(ID_LAYOUTBUT0 + i))->GetValue())
        {   
            Result->Layout = (CXMLFormLayout)i;
            break;
        }
    }
    return Result;
}
//
// Returns form data source type selected by radio group
//
TXMLFormSource CXMLFormWizard::GetSource() const
{
    if (SourcePage->TableRB->GetValue())
        return XFS_TABLE;
    else if (SourcePage->QueryRB->GetValue())
        return XFS_QUERY;
    else if (SourcePage->SQLRB->GetValue())
        return XFS_SQL;
    else
        return XFS_DAD;
}

/*!
 * CXMLFormWizardLayoutPage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CXMLFormWizardLayoutPage, wxWizardPageSimple )

/*!
 * CXMLFormWizardLayoutPage event table definition
 */

BEGIN_EVENT_TABLE( CXMLFormWizardLayoutPage, wxWizardPageSimple )

////@begin CXMLFormWizardLayoutPage event table entries
    EVT_WIZARD_PAGE_CHANGING( -1, CXMLFormWizardLayoutPage::OnLayoutpagePageChanging )

    EVT_RADIOBUTTON( ID_TYPEREPORT, CXMLFormWizardLayoutPage::OnTypebutSelected )

    EVT_RADIOBUTTON( ID_TYPEINS, CXMLFormWizardLayoutPage::OnTypebutSelected )

    EVT_RADIOBUTTON( ID_TYPEUPD, CXMLFormWizardLayoutPage::OnTypebutSelected )

    EVT_BUTTON( ID_BITMAPBUT0, CXMLFormWizardLayoutPage::OnButtonClick )

    EVT_BUTTON( ID_BITMAPBUTTON3, CXMLFormWizardLayoutPage::OnButtonClick )

    EVT_BUTTON( ID_BITMAPBUTTON2, CXMLFormWizardLayoutPage::OnButtonClick )

////@end CXMLFormWizardLayoutPage event table entries

END_EVENT_TABLE()

/*!
 * CXMLFormWizardLayoutPage constructors
 */

CXMLFormWizardLayoutPage::CXMLFormWizardLayoutPage( )
{
}

CXMLFormWizardLayoutPage::CXMLFormWizardLayoutPage( wxWizard* parent )
{
    Create( parent );
}

/*!
 * CXMLFormWizardPage2 creator
 */

bool CXMLFormWizardLayoutPage::Create( wxWizard* parent )
{
////@begin CXMLFormWizardLayoutPage member initialisation
    FormNameED = NULL;
////@end CXMLFormWizardLayoutPage member initialisation

////@begin CXMLFormWizardLayoutPage creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end CXMLFormWizardLayoutPage creation
    return true;
}

/*!
 * Control creation for CXMLFormWizardPage2
 */

void CXMLFormWizardLayoutPage::CreateControls()
{    
////@begin CXMLFormWizardLayoutPage content construction
    CXMLFormWizardLayoutPage* itemWizardPageSimple2 = this;

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemWizardPageSimple2->SetSizer(itemBoxSizer3);

    wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox(itemWizardPageSimple2, wxID_ANY, _(" Form name "));
    wxStaticBoxSizer* itemStaticBoxSizer4 = new wxStaticBoxSizer(itemStaticBoxSizer4Static, wxHORIZONTAL);
    itemBoxSizer3->Add(itemStaticBoxSizer4, 0, wxGROW|wxBOTTOM, 5);

    wxStaticText* itemStaticText5 = new wxStaticText( itemWizardPageSimple2, wxID_STATIC, _("Name for new form:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    FormNameED = new wxTextCtrl( itemWizardPageSimple2, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    FormNameED->SetMaxLength(31);
    itemStaticBoxSizer4->Add(FormNameED, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxGridSizer* itemGridSizer7 = new wxGridSizer(1, 2, 0, 0);
    itemBoxSizer3->Add(itemGridSizer7, 1, wxGROW, 5);

    wxStaticBox* itemStaticBoxSizer8Static = new wxStaticBox(itemWizardPageSimple2, wxID_ANY, _("Form type"));
    wxStaticBoxSizer* itemStaticBoxSizer8 = new wxStaticBoxSizer(itemStaticBoxSizer8Static, wxVERTICAL);
    itemGridSizer7->Add(itemStaticBoxSizer8, 1, wxGROW|wxGROW|wxRIGHT, 5);

    wxGridSizer* itemGridSizer9 = new wxGridSizer(3, 1, 0, 0);
    itemStaticBoxSizer8->Add(itemGridSizer9, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxRadioButton* itemRadioButton10 = new wxRadioButton( itemWizardPageSimple2, ID_TYPEREPORT, _("Report"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton10->SetValue(true);
    itemGridSizer9->Add(itemRadioButton10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxRadioButton* itemRadioButton11 = new wxRadioButton( itemWizardPageSimple2, ID_TYPEINS, _("Insert form"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton11->SetValue(false);
    itemGridSizer9->Add(itemRadioButton11, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxRadioButton* itemRadioButton12 = new wxRadioButton( itemWizardPageSimple2, ID_TYPEUPD, _("Update form"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton12->SetValue(false);
    itemGridSizer9->Add(itemRadioButton12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer13Static = new wxStaticBox(itemWizardPageSimple2, wxID_ANY, _(" Form layout "));
    wxStaticBoxSizer* itemStaticBoxSizer13 = new wxStaticBoxSizer(itemStaticBoxSizer13Static, wxVERTICAL);
    itemGridSizer7->Add(itemStaticBoxSizer13, 0, wxGROW|wxGROW|wxLEFT, 5);

    wxFlexGridSizer* itemFlexGridSizer14 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer14->AddGrowableRow(0);
    itemFlexGridSizer14->AddGrowableRow(1);
    itemFlexGridSizer14->AddGrowableRow(2);
    itemStaticBoxSizer13->Add(itemFlexGridSizer14, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxRadioButton* itemRadioButton15 = new wxRadioButton( itemWizardPageSimple2, ID_LAYOUTBUT0, _T(""), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton15->SetValue(true);
    itemFlexGridSizer14->Add(itemRadioButton15, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 20);

    wxBitmap itemBitmapButton16Bitmap(wxNullBitmap);
    wxBitmapButton* itemBitmapButton16 = new wxBitmapButton( itemWizardPageSimple2, ID_BITMAPBUT0, itemBitmapButton16Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemFlexGridSizer14->Add(itemBitmapButton16, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxRadioButton* itemRadioButton17 = new wxRadioButton( itemWizardPageSimple2, ID_LAYOUTBUT1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton17->SetValue(false);
    itemFlexGridSizer14->Add(itemRadioButton17, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 20);

    wxBitmap itemBitmapButton18Bitmap(wxNullBitmap);
    wxBitmapButton* itemBitmapButton18 = new wxBitmapButton( itemWizardPageSimple2, ID_BITMAPBUTTON3, itemBitmapButton18Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemFlexGridSizer14->Add(itemBitmapButton18, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxRadioButton* itemRadioButton19 = new wxRadioButton( itemWizardPageSimple2, ID_LAYOUTBUT2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton19->SetValue(true);
    itemFlexGridSizer14->Add(itemRadioButton19, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 20);

    wxBitmap itemBitmapButton20Bitmap(wxNullBitmap);
    wxBitmapButton* itemBitmapButton20 = new wxBitmapButton( itemWizardPageSimple2, ID_BITMAPBUTTON2, itemBitmapButton20Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemFlexGridSizer14->Add(itemBitmapButton20, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

////@end CXMLFormWizardLayoutPage content construction

    wxBitmap f1(Form1);
    itemBitmapButton16->SetBitmapLabel(f1);
    wxBitmap f3(Form3);
    itemBitmapButton18->SetBitmapLabel(f3);
    wxBitmap f4(Form4);
    itemBitmapButton20->SetBitmapLabel(f4);

    itemRadioButton10->SetValue(true);
    itemRadioButton15->SetValue(true);
}

/*!
 * Should we show tooltips?
 */

bool CXMLFormWizardLayoutPage::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CXMLFormWizardLayoutPage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CXMLFormWizardLayoutPage bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end CXMLFormWizardLayoutPage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CXMLFormWizardLayoutPage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CXMLFormWizardLayoutPage icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CXMLFormWizardLayoutPage icon retrieval
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_TYPEBUT0
 */

void CXMLFormWizardLayoutPage::OnTypebutSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_TYPEBUT0 in CXMLFormWizardLayoutPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_TYPEBUT0 in CXMLFormWizardLayoutPage. 

    CXMLFormWizard *Wizard = (CXMLFormWizard *)GetParent();
    if (Wizard->m_FromDAD)
        return;
    // IF form type is report, page sequence will be LayoutPage - SourcePage - StructPage
    if (event.GetId() == ID_TYPEREPORT)
    {
        Wizard->LayoutPage->SetNext(Wizard->SourcePage);
        Wizard->SourcePage->SetNext(Wizard->StructPage);
        Wizard->StructPage->SetPrev(Wizard->SourcePage);
        Wizard->SourcePage->SetPrev(Wizard->LayoutPage);
    }
    // IF form type is insert form, page sequence will be LayoutPage - DestPage - StructPage
    else if (event.GetId() == ID_TYPEINS)
    {
        Wizard->LayoutPage->SetNext(Wizard->DestPage);
        Wizard->DestPage->SetNext(Wizard->StructPage);
        Wizard->StructPage->SetPrev(Wizard->DestPage);
        Wizard->DestPage->SetPrev(Wizard->LayoutPage);
    }
    // IF form type is update form, page sequence will be LayoutPage - SourcePage - DestPage - StructPage
    else
    {
        Wizard->LayoutPage->SetNext(Wizard->SourcePage);
        Wizard->SourcePage->SetNext(Wizard->DestPage);
        Wizard->DestPage->SetNext(Wizard->StructPage);
        Wizard->StructPage->SetPrev(Wizard->DestPage);
        Wizard->DestPage->SetPrev(Wizard->SourcePage);
        Wizard->SourcePage->SetPrev(Wizard->LayoutPage);
    }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON
 */

void CXMLFormWizardLayoutPage::OnButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON in CXMLFormWizardLayoutPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON in CXMLFormWizardLayoutPage.

    // Set corresponding radio button
    int SenderID = event.GetId();
    wxRadioButton *DestRB = (wxRadioButton *)FindWindow(SenderID + (ID_LAYOUTBUT0 - ID_BITMAPBUT0));
    DestRB->SetValue(true);
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_LAYOUTPAGE
 */

void CXMLFormWizardLayoutPage::OnLayoutpagePageChanging( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_PAGE_CHANGING event handler for ID_LAYOUTPAGE in CXMLFormWizardLayoutPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_PAGE_CHANGING event handler for ID_LAYOUTPAGE in CXMLFormWizardLayoutPage. 

    // Check form name
    wxString FormName = FormNameED->GetValue().Trim(true).Trim(false);
    bool Error = false;
    if (FormName.IsEmpty())
    {
        error_box(_("Enter form name"), this);
        Error = true;
    }
    cdp_t cdp = ((CXMLFormWizard *)GetParent())->m_cdp;
    char *aName = ((CXMLFormWizard *)GetParent())->StructPage->FormName;
    strcpy(aName, FormName.mb_str(*cdp->sys_conv)); 
    tobjnum obj;
    if (cd_Find2_object(cdp, aName, NULL, CATEG_XMLFORM, &obj) == 0)
    {
        error_boxp(_("Form named %ls already exists"), this, FormName.c_str());
        FormNameED->SetSelection(-1, -1);
        Error = true;
    }
    if (Error)
    {
        FormNameED->SetFocus();
        event.Skip(false);
        event.Veto();
        return;
    }
    // Set form type in form layout descriptor
    for (int i = 0; i < 3; i++)
    {
        if (((wxRadioButton *)FindWindow(ID_TYPEREPORT + i))->GetValue())
        {   
            CXMLFormWizardStructPage *StructPage = ((CXMLFormWizard *)GetParent())->StructPage;
            // IF form type is changed from report to insert or update form, remove aggregates
            if (StructPage->Type == XFT_REPORT && (CXMLFormType)i != XFT_REPORT)
                StructPage->RemoveAggrs(StructPage->FirstLevel);
            StructPage->Type = (CXMLFormType)i;
            break;
        }
    }
}

/*!
 * CXMLFormWizardSourcePage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CXMLFormWizardSourcePage, wxWizardPageSimple )

/*!
 * CXMLFormWizardSourcePage event table definition
 */

BEGIN_EVENT_TABLE( CXMLFormWizardSourcePage, wxWizardPageSimple )

////@begin CXMLFormWizardSourcePage event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, CXMLFormWizardSourcePage::OnSourcepagePageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, CXMLFormWizardSourcePage::OnSourcepagePageChanging )

    EVT_RADIOBUTTON( ID_FROMTABLE, CXMLFormWizardSourcePage::OnRadiobuttonSelected )

    EVT_RADIOBUTTON( ID_FROMQUERY, CXMLFormWizardSourcePage::OnRadiobuttonSelected )

    EVT_RADIOBUTTON( ID_FROMSQL, CXMLFormWizardSourcePage::OnRadiobuttonSelected )

    EVT_RADIOBUTTON( ID_FROMDAD, CXMLFormWizardSourcePage::OnRadiobuttonSelected )

    EVT_LISTBOX( ID_LISTBOX1, CXMLFormWizardSourcePage::OnListSelected )

    EVT_TEXT( ID_QUERYED, CXMLFormWizardSourcePage::OnQueryedUpdated )

////@end CXMLFormWizardSourcePage event table entries

END_EVENT_TABLE()

/*!
 * CXMLFormWizardSourcePage constructors
 */

CXMLFormWizardSourcePage::CXMLFormWizardSourcePage( )
{
}

CXMLFormWizardSourcePage::CXMLFormWizardSourcePage( wxWizard* parent )
{
    Create( parent );
}

/*!
 * CXMLFormWizardSourcePage creator
 */

bool CXMLFormWizardSourcePage::Create( wxWizard* parent )
{
////@begin CXMLFormWizardSourcePage member initialisation
    Sizer = NULL;
    TableRB = NULL;
    QueryRB = NULL;
    SQLRB = NULL;
    DADRB = NULL;
    List = NULL;
    QueryED = NULL;
    DADNameLab = NULL;
    DADNameED = NULL;
////@end CXMLFormWizardSourcePage member initialisation

    m_ListLoaded = false;
    m_Changed    = true;
    m_cdp        = ((CXMLFormWizard *)parent)->m_cdp;

////@begin CXMLFormWizardSourcePage creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end CXMLFormWizardSourcePage creation
    return true;
}

/*!
 * Control creation for CXMLFormWizardSourcePage
 */

void CXMLFormWizardSourcePage::CreateControls()
{    
////@begin CXMLFormWizardSourcePage content construction
    CXMLFormWizardSourcePage* itemWizardPageSimple21 = this;

    wxStaticBox* itemStaticBoxSizer22Static = new wxStaticBox(itemWizardPageSimple21, wxID_ANY, _(" Form data source "));
    Sizer = new wxStaticBoxSizer(itemStaticBoxSizer22Static, wxVERTICAL);
    itemWizardPageSimple21->SetSizer(Sizer);

    TableRB = new wxRadioButton( itemWizardPageSimple21, ID_FROMTABLE, _("&Table"), wxDefaultPosition, wxDefaultSize, 0 );
    TableRB->SetValue(false);
    Sizer->Add(TableRB, 0, wxALIGN_LEFT|wxALL, 5);

    QueryRB = new wxRadioButton( itemWizardPageSimple21, ID_FROMQUERY, _("&Stored query"), wxDefaultPosition, wxDefaultSize, 0 );
    QueryRB->SetValue(true);
    Sizer->Add(QueryRB, 0, wxALIGN_LEFT|wxALL, 5);

    SQLRB = new wxRadioButton( itemWizardPageSimple21, ID_FROMSQL, _("&SQL query"), wxDefaultPosition, wxDefaultSize, 0 );
    SQLRB->SetValue(true);
    Sizer->Add(SQLRB, 0, wxALIGN_LEFT|wxALL, 5);

    DADRB = new wxRadioButton( itemWizardPageSimple21, ID_FROMDAD, _("&DAD"), wxDefaultPosition, wxDefaultSize, 0 );
    DADRB->SetValue(true);
    Sizer->Add(DADRB, 0, wxALIGN_LEFT|wxALL, 5);

    wxString* ListStrings = NULL;
    List = new wxListBox( itemWizardPageSimple21, ID_LISTBOX1, wxDefaultPosition, wxDefaultSize, 0, ListStrings, wxLB_SINGLE );
    Sizer->Add(List, 1, wxGROW|wxALL, 5);

    QueryED = new wxTextCtrl( itemWizardPageSimple21, ID_QUERYED, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    QueryED->Show(false);
    Sizer->Add(QueryED, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxHORIZONTAL);
    Sizer->Add(itemBoxSizer29, 0, wxGROW|wxLEFT|wxFIXED_MINSIZE, 5);

    DADNameLab = new wxStaticText( itemWizardPageSimple21, wxID_STATIC, _("Name for DAD:"), wxDefaultPosition, wxDefaultSize, 0 );
    DADNameLab->Enable(false);
    itemBoxSizer29->Add(DADNameLab, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxFIXED_MINSIZE, 5);

    DADNameED = new wxTextCtrl( itemWizardPageSimple21, ID_DADNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    DADNameED->Enable(false);
    itemBoxSizer29->Add(DADNameED, 1, wxGROW|wxALL|wxFIXED_MINSIZE, 5);

////@end CXMLFormWizardSourcePage content construction
}

/*!
 * Should we show tooltips?
 */

bool CXMLFormWizardSourcePage::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CXMLFormWizardSourcePage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CXMLFormWizardSourcePage bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end CXMLFormWizardSourcePage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CXMLFormWizardSourcePage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CXMLFormWizardSourcePage icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CXMLFormWizardSourcePage icon retrieval
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_SOURCEPAGE
 */

void CXMLFormWizardSourcePage::OnSourcepagePageChanged( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_PAGE_CHANGED event handler for ID_SOURCEPAGE in CXMLFormWizardSourcePage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_PAGE_CHANGED event handler for ID_SOURCEPAGE in CXMLFormWizardSourcePage. 
    
    // IF object list is not loaded
    if (!m_ListLoaded)
    {
        CObjectList ol(Getcdp());
        // IF tables count > 0, enable table source
        bool te = ol.GetCount(CATEG_TABLE) > 0;
        TableRB->Enable(te);
        // IF stored query count > 0, enable stored query source
        bool qe = ol.GetCount(CATEG_CURSOR) > 0;
        QueryRB->Enable(qe);
        // IF DAD count > 0, enable DAD source
        bool de = false;
        for (bool found = ol.FindFirst(CATEG_PGMSRC); found; found = ol.FindNext())
        {
            if ((ol.GetFlags() & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_XML)
            {
                de = true;
                break;
            }
        }
        tcateg cat = CATEG_PGMSRC;
        DADRB->Enable(de);
        // IF DAD count == 0
        if (!de)
        {
            // IF tables count > 0, select table source
            if (te)
            {
                cat = CATEG_TABLE;
                TableRB->SetValue(true);
            }
            // ELSE IF stored query count > 0, select stored query source
            else if (qe)
            {
                cat = CATEG_CURSOR;
                QueryRB->SetValue(true);
            }
            // ELSE select SELECT source
            else
            {
                cat = CATEG_DIRCUR;
                SQLRB->SetValue(true);
            }
        }

        wxSetCursor(*wxSTANDARD_CURSOR);
        // Load object list
        LoadList(cat);
        m_ListLoaded = true;
    }
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_FROMTABLE
 */

void CXMLFormWizardSourcePage::OnRadiobuttonSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_FROMTABLE in CXMLFormWizardPage1.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_FROMTABLE in CXMLFormWizardPage1. 

    tcateg Categ;
    // Load list of selected objects
    if (event.GetId() == ID_FROMTABLE)
        Categ = CATEG_TABLE;
    else if (event.GetId() == ID_FROMQUERY)
        Categ = CATEG_CURSOR;
    else if (event.GetId() == ID_FROMSQL)
        Categ = CATEG_DIRCUR;
    else
        Categ = CATEG_PGMSRC;
    LoadList(Categ);
    m_Changed = true;
}
/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX1
 */

void CXMLFormWizardSourcePage::OnListSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX1 in CXMLFormWizardSourcePage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX1 in CXMLFormWizardSourcePage. 

    // IF source is table, stored query or SELECT statement, set default DAD name
    if (!DADRB->GetValue())
    {
        SetDADName();
    }
    // IF form layout defined, reset it
    CXMLFormWizard *Wizard = (CXMLFormWizard *)GetParent();
    CXMLFormStruct *Struct = Wizard->GetStruct();
    if (Struct->FirstLevel)
    {
        Wizard->StructPage->m_Loaded = false;
    }

    m_Changed = true;
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_QUERYED
 */

void CXMLFormWizardSourcePage::OnQueryedUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_QUERYED in CXMLFormWizardSourcePage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_QUERYED in CXMLFormWizardSourcePage. 
    m_Changed = true;
}
/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_SOURCEPAGE
 */

void CXMLFormWizardSourcePage::OnSourcepagePageChanging( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_PAGE_CHANGING event handler for ID_SOURCEPAGE in CXMLFormWizardSourcePage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_PAGE_CHANGING event handler for ID_SOURCEPAGE in CXMLFormWizardSourcePage. 

    wxString Query;
    // EXIT if back step
    if (!event.GetDirection())
        return;
    // IF source is SELECT statement, check statement text
    if (SQLRB->GetValue())
    {
        Query = QueryED->GetValue().Trim(true).Trim(false);
        if (Query.IsEmpty())
        {
            error_box(_("SQL statement is empty"), this);
            event.Skip(false);
            event.Veto();
            return;
        }
    }
    // ELSE check list selection
    else if (List->GetSelection() == wxNOT_FOUND)
    {
        error_box(_("Nothing is selected"), this);
        event.Skip(false);
        event.Veto();
        return;
    }
    // IF source is DAD, check DAD name
    if (!DADRB->GetValue() && DADNameED->GetValue().Trim(true).Trim(false).IsEmpty())
    {
        error_box(_("Enter DAD name"), this);
        event.Skip(false);
        event.Veto();
        return;
    }
    // IF page values are changed
    if (m_Changed)
    {
        ((CXMLFormWizard *)GetParent())->m_DefaultDAD = true;
        // IF source is table, generate default DAD for table
        if (TableRB->GetValue())
        {
            wxString Table      = List->GetStringSelection();
            t_dad_root *DadTree = link_new_t_dad_root(m_cdp->sys_charset);
            DadTree->prolog     = link_new_chars((int)strlen(def_prolog) + 1);
            if (DadTree->prolog)
                strcpy(DadTree->prolog, def_prolog);
            DadTree->root_element = link_new_dad_element_node();
            if (DadTree->root_element)
                strcpy(DadTree->root_element->el_name, "root");
            create_default_design_analytical(NULL, find_server_connection_info(m_cdp->locid_server_name), Table.mb_str(*m_cdp->sys_conv), "", "", DadTree, NULL, NULL, wxTreeItemId());
            // Replace spaces in DAD element names
            ReplaceSpace(DadTree->root_element);
            // Set DAD tree in form descriptor
            SetDadTree(DadTree);
            m_Changed = false;
        }
        // IF source is stored query, generate default DAD for stored query
        else if (QueryRB->GetValue())
        {
            tobjnum  querynum;
            bool     OK = false;
            wxString Query = List->GetStringSelection();
            // Find query object
            if (!cd_Find2_object(m_cdp, Query.mb_str(*m_cdp->sys_conv), NULL, CATEG_CURSOR, &querynum))
            {
                // Load query definition
                char *def = cd_Load_objdef(m_cdp, querynum, CATEG_CURSOR, NULL);
                if (def)
                {
                    // Generate default DAD
                    OK = ((CXMLFormWizard *)GetParent())->GetQueryDAD(def);
                    if (OK)
                        m_Changed = false;
                    corefree(def);
                }
            }
            if (!OK)
            {
                cd_Signalize(m_cdp);
                event.Skip(false);
                event.Veto();
            }
        }
        // IF source is SELECT statement, generate default DAD for SELECT
        else if (SQLRB->GetValue())
        {
            if (((CXMLFormWizard *)GetParent())->GetQueryDAD(Query.mb_str(*m_cdp->sys_conv)))
            {
                m_Changed = false;
            }
            else
            {
                cd_Signalize(m_cdp);
                event.Skip(false);
                event.Veto();
            }
        }
        // ELSE source is DAD, parse DAD tree and store it to form layout descriptor
        else
        {
            wxString DAD = List->GetStringSelection();
            if (!((CXMLFormWizard *)GetParent())->LoadDad(DAD.mb_str(*m_cdp->sys_conv)))
            {
                event.Skip(false);
                event.Veto();
            }
            m_Changed = false;
            ((CXMLFormWizard *)GetParent())->m_DefaultDAD = false;
        }
        // Set DAD name in form layout decriptor
        strcpy(((CXMLFormWizard *)GetParent())->StructPage->DadName, GetDAD().mb_str(*m_cdp->sys_conv));
    }
}
//
// Generates free DAD name and sets it to the DAD name editor
//
void CXMLFormWizardSourcePage::SetDADName()
{
    wxString wObjName = List->GetStringSelection();
    tobjname aObjName;
    Getcdp()->sys_conv->WC2MB(aObjName, wObjName.c_str(), sizeof(aObjName));
    if (GetFreeObjName(Getcdp(), aObjName, CATEG_PGMSRC))
        DADNameED->SetValue(wxString(aObjName, *Getcdp()->sys_conv));
}
//
// Load list of form data source objects
//
void CXMLFormWizardSourcePage::LoadList(tcateg Categ)
{
    // IF source is SELECT statement, hide list and show statement editor
    if (Categ == CATEG_DIRCUR)
    {
        QueryED->Show(true);
        List->Show(false);
        Sizer->RecalcSizes();
    }
    // ELSE source is table, stored query or DAD
    else
    {
        // Load object list
        CObjectList ol(Getcdp());
        List->Clear();
        for (bool Found = ol.FindFirst(Categ); Found; Found = ol.FindNext())
        {
            if (Categ != CATEG_PGMSRC || (ol.m_Flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_XML)
                List->Append(GetSmallStringName(ol));
        }
        // IF list is empty, clear DAD name editor
        if (List->IsEmpty())
            DADNameED->Clear();
        else
        {
            int i = 0;
            // IF source is DAD and DAD name is specified allready, select corresponding list item, othervise select first item
            if (Categ == CATEG_PGMSRC)
            {
                const char *DadName = ((CXMLFormWizard *)GetParent())->GetStruct()->DadName;
                if (*DadName)
                {
                    i = List->FindString(wxString(DadName, *m_cdp->sys_conv));
                    if (i == wxNOT_FOUND)
                        i = 0;
                }
            }
            List->SetSelection(i);
            // IF source is DAD, clear DAD name editor 
            if (Categ == CATEG_PGMSRC)
                DADNameED->Clear();
            // ELSE set default DAD name
            else
                SetDADName();
        }
        // Show list and hide statement editor
        QueryED->Show(false);
        List->Show(true);
    }
    // Enable/disable DAD name editor
    DADNameLab->Enable(Categ != CATEG_PGMSRC);
    DADNameED->Enable(Categ != CATEG_PGMSRC);
}
//
// IF form data source is DAD, returns DAD list selection, else returns user input from DADNameED
//
wxString CXMLFormWizardSourcePage::GetDAD() const
{
    if (DADRB->GetValue())
        return List->GetStringSelection();
    else
        return DADNameED->GetValue().Trim(true).Trim(false);
}
//
// Sets DAD tree
//
void CXMLFormWizardSourcePage::SetDadTree(t_dad_root *aDadTree)
{
    ((CXMLFormWizard *)GetParent())->StructPage->SetDadTree(aDadTree);
}

/*!
 * CXMLFormWizardDestPage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CXMLFormWizardDestPage, wxWizardPageSimple )

/*!
 * CXMLFormWizardDestPage event table definition
 */

BEGIN_EVENT_TABLE( CXMLFormWizardDestPage, wxWizardPageSimple )

////@begin CXMLFormWizardDestPage event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, CXMLFormWizardDestPage::OnDestpagePageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, CXMLFormWizardDestPage::OnDestpagePageChanging )

    EVT_LISTBOX( ID_LISTBOX, CXMLFormWizardDestPage::OnListBoxSelected )

////@end CXMLFormWizardDestPage event table entries

END_EVENT_TABLE()

/*!
 * CXMLFormWizardDestPage constructors
 */

CXMLFormWizardDestPage::CXMLFormWizardDestPage( )
{
}

CXMLFormWizardDestPage::CXMLFormWizardDestPage( wxWizard* parent )
{
    Create( parent );
}

/*!
 * CXMLFormWizardPage1 creator
 */

bool CXMLFormWizardDestPage::Create( wxWizard* parent )
{
////@begin CXMLFormWizardDestPage member initialisation
    Sizer = NULL;
    List = NULL;
////@end CXMLFormWizardDestPage member initialisation
    m_ListLoaded = false;
    m_Changed    = true;
    m_cdp        = ((CXMLFormWizard *)parent)->m_cdp;

////@begin CXMLFormWizardDestPage creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end CXMLFormWizardDestPage creation
    return true;
}

/*!
 * Control creation for CXMLFormWizardPage1
 */

void CXMLFormWizardDestPage::CreateControls()
{    
////@begin CXMLFormWizardDestPage content construction
    CXMLFormWizardDestPage* itemWizardPageSimple32 = this;

    wxStaticBox* itemStaticBoxSizer33Static = new wxStaticBox(itemWizardPageSimple32, wxID_ANY, _(" DAD for input from XML "));
    Sizer = new wxStaticBoxSizer(itemStaticBoxSizer33Static, wxVERTICAL);
    itemWizardPageSimple32->SetSizer(Sizer);

    wxString* ListStrings = NULL;
    List = new wxListBox( itemWizardPageSimple32, ID_LISTBOX, wxDefaultPosition, wxDefaultSize, 0, ListStrings, wxLB_SINGLE );
    Sizer->Add(List, 1, wxGROW|wxALL, 5);

////@end CXMLFormWizardDestPage content construction
}

/*!
 * Should we show tooltips?
 */

bool CXMLFormWizardDestPage::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CXMLFormWizardDestPage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CXMLFormWizardDestPage bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end CXMLFormWizardDestPage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CXMLFormWizardDestPage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CXMLFormWizardDestPage icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CXMLFormWizardDestPage icon retrieval
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_DESTPAGE
 */

void CXMLFormWizardDestPage::OnDestpagePageChanged( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_PAGE_CHANGED event handler for ID_DESTPAGE in CXMLFormWizardDestPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_PAGE_CHANGED event handler for ID_DESTPAGE in CXMLFormWizardDestPage. 

    CObjectList ol(m_cdp);
    // EXIT IF object list is loaded allready
    if (m_ListLoaded)
        return;
    m_ListLoaded = true;
    t_ucd_cl *cdcl = link_new_t_ucd_cl_602(m_cdp);
    if (cdcl)
    {
        if (link_init_platform_utils(cdcl))
        {
            // Traverse DADs and analytical DAD names add to list
            for (bool Found = ol.FindFirst(CATEG_PGMSRC); Found; Found = ol.FindNext())
            {
                if ((ol.m_Flags & CO_FLAGS_OBJECT_TYPE) != CO_FLAG_XML)
                    continue;
                char *dad = cd_Load_objdef(m_cdp, ol.m_ObjNum, CATEG_PGMSRC, NULL);
                dad = link_convert2utf8(dad);
                if (dad)
                {
                    t_dad_root *DadRoot = link_parse(cdcl, dad);
                    if (DadRoot && !DadRoot->sql_stmt)
                        List->Append(GetSmallStringName(ol));
                    corefree(dad);
                }
            }
            link_close_platform_utils();
            // IF DAD name is defined, select it the list
            const char *dn = ((CXMLFormWizard *)GetParent())->StructPage->DadName;
            if (*dn)
            {
                int Sel = List->FindString(wxString(dn, *m_cdp->sys_conv));
                if (Sel != wxNOT_FOUND)
                    List->SetSelection(Sel);
            }
        }
        link_delete_t_ucd_cl(cdcl);
    }
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX
 */

void CXMLFormWizardDestPage::OnListBoxSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX in CXMLFormWizardDestPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX in CXMLFormWizardDestPage. 
    m_Changed = true;
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_DESTPAGE
 */

void CXMLFormWizardDestPage::OnDestpagePageChanging( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_PAGE_CHANGING event handler for ID_DESTPAGE in CXMLFormWizardDestPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_PAGE_CHANGING event handler for ID_DESTPAGE in CXMLFormWizardDestPage. 

    // IF page values are changed
    if (m_Changed)
    {
        wxString DAD = List->GetStringSelection();
        // Parse DAD tree and store it to form layout descriptor
        if (!((CXMLFormWizard *)GetParent())->LoadDad(DAD.mb_str(*m_cdp->sys_conv), true))
        {
            event.Skip(false);
            event.Veto();
        }

        m_Changed = false;
    }
}

/*!
 * CXMLFormWizardStructPage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CXMLFormWizardStructPage, wxWizardPageSimple )

/*!
 * CXMLFormWizardStructPage event table definition
 */

BEGIN_EVENT_TABLE( CXMLFormWizardStructPage, wxWizardPageSimple )

////@begin CXMLFormWizardStructPage event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, CXMLFormWizardStructPage::OnStructpagePageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, CXMLFormWizardStructPage::OnStructpagePageChanging )

    EVT_TREE_SEL_CHANGED( ID_TREECTRL, CXMLFormWizardStructPage::OnTreectrlSelChanged )

    EVT_GRID_LABEL_LEFT_CLICK( CXMLFormWizardStructPage::OnLabelLeftClick )
    EVT_GRID_CMD_CELL_CHANGE( ID_GRID1, CXMLFormWizardStructPage::OnElemCellChange )
    EVT_GRID_CMD_SELECT_CELL( ID_GRID1, CXMLFormWizardStructPage::OnElemSelectCell )
    EVT_LEFT_DOWN( CXMLFormWizardStructPage::OnElemLeftDown )
    EVT_LEFT_UP( CXMLFormWizardStructPage::OnLeftUp )
    EVT_MOTION( CXMLFormWizardStructPage::OnMotion )
    EVT_KEY_DOWN( CXMLFormWizardStructPage::OnKeyDown )

    EVT_BUTTON( ID_BUTTON, CXMLFormWizardStructPage::OnAddButtonClick )

    EVT_BUTTON( ID_BUTTON1, CXMLFormWizardStructPage::OnRemoveButton1Click )

////@end CXMLFormWizardStructPage event table entries

END_EVENT_TABLE()

/*!
 * CXMLFormWizardStructPage constructors
 */

CXMLFormWizardStructPage::CXMLFormWizardStructPage( )
{
}

CXMLFormWizardStructPage::CXMLFormWizardStructPage( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage creator
 */

bool CXMLFormWizardStructPage::Create( wxWizard* parent )
{
    m_SelLevel       = NULL;
    m_ElemSelRow     = -1;
    m_Loaded         = false;
    m_ElemChanged    = false;
    m_InElemSelCell  = false;
    m_InDrag         = false;
    m_DragPos        = 0;
    m_Dragged        = -1;
    m_cdp            = ((CXMLFormWizard *)parent)->m_cdp;

////@begin CXMLFormWizardStructPage member initialisation
    RepLevelTree = NULL;
    RepElemGrid = NULL;
    AddBut = NULL;
    RemoveBut = NULL;
////@end CXMLFormWizardStructPage member initialisation

////@begin CXMLFormWizardStructPage creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end CXMLFormWizardStructPage creation
    return true;
}

/*!
 * Control creation for WizardPage
 */

static wxString Aggregates[]  = {wxT("COUNT"), wxT("SUM"), wxT("AVG")}; //, wxT("MIN"), wxT("MAX")};

void CXMLFormWizardStructPage::CreateControls()
{
////@begin CXMLFormWizardStructPage content construction
    CXMLFormWizardStructPage* itemWizardPageSimple35 = this;

    wxFlexGridSizer* itemFlexGridSizer36 = new wxFlexGridSizer(2, 1, 0, 0);
    itemFlexGridSizer36->AddGrowableRow(0);
    itemFlexGridSizer36->AddGrowableRow(1);
    itemFlexGridSizer36->AddGrowableCol(0);
    itemWizardPageSimple35->SetSizer(itemFlexGridSizer36);

    wxStaticBox* itemStaticBoxSizer37Static = new wxStaticBox(itemWizardPageSimple35, wxID_ANY, _(" Repeatable sections structuring "));
    wxStaticBoxSizer* itemStaticBoxSizer37 = new wxStaticBoxSizer(itemStaticBoxSizer37Static, wxHORIZONTAL);
    itemFlexGridSizer36->Add(itemStaticBoxSizer37, 2, wxGROW|wxGROW, 0);

    RepLevelTree = new wxTreeCtrl( itemWizardPageSimple35, ID_TREECTRL, wxDefaultPosition, wxSize(100, 100), wxTR_SINGLE );
    itemStaticBoxSizer37->Add(RepLevelTree, 1, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer39Static = new wxStaticBox(itemWizardPageSimple35, wxID_ANY, _(" Elements in repeatable section "));
    wxStaticBoxSizer* itemStaticBoxSizer39 = new wxStaticBoxSizer(itemStaticBoxSizer39Static, wxVERTICAL);
    itemFlexGridSizer36->Add(itemStaticBoxSizer39, 3, wxGROW|wxGROW|wxTOP, 5);

    RepElemGrid = new wxGrid( itemWizardPageSimple35, ID_GRID1, wxDefaultPosition, wxSize(436, 120), wxSUNKEN_BORDER|wxWANTS_CHARS|wxHSCROLL|wxVSCROLL );
    RepElemGrid->SetDefaultColSize(92);
    RepElemGrid->SetDefaultRowSize(21);
    RepElemGrid->SetColLabelSize(21);
    RepElemGrid->SetRowLabelSize(26);
    RepElemGrid->CreateGrid(1, 2, wxGrid::wxGridSelectCells);
    itemStaticBoxSizer39->Add(RepElemGrid, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer41 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer39->Add(itemBoxSizer41, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 3);

    AddBut = new wxButton( itemWizardPageSimple35, ID_BUTTON, _("&Add all"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer41->Add(AddBut, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10);

    RemoveBut = new wxButton( itemWizardPageSimple35, ID_BUTTON1, _("&Remove all"), wxDefaultPosition, wxDefaultSize, 0 );
    RemoveBut->SetDefault();
    RemoveBut->Enable(false);
    itemBoxSizer41->Add(RemoveBut, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10);

////@end CXMLFormWizardStructPage content construction

    SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);

    // Elem grid
    RepElemGrid->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CXMLFormWizardStructPage::OnElemLeftDown), NULL, this);
    RepElemGrid->GetGridRowLabelWindow()->Connect(wxEVT_MOTION, wxMouseEventHandler(CXMLFormWizardStructPage::OnLabelMotion), NULL, this);
    RepElemGrid->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
    RepElemGrid->SetSelectionMode(wxGrid::wxGridSelectRows);
    
    RepElemGrid->SetColLabelValue(0, _("Element"));

    RepElemGrid->SetColLabelValue(1, _("Label"));

    ShowAggrCols(Type == XFT_REPORT);
        
    //AddBut->Enable(false);
    //RemoveBut->Enable(false);

    wxWindow *Grid = RepElemGrid->GetGridWindow();
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
        CellAttr->SetOverflow(false);
        RepElemGrid->SetColAttr(0, CellAttr);
    }
    CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
        CellAttr->SetOverflow(false);
        RepElemGrid->SetColAttr(1, CellAttr);
    }
}

/*!
 * Should we show tooltips?
 */

bool CXMLFormWizardStructPage::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CXMLFormWizardStructPage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CXMLFormWizardStructPage bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end CXMLFormWizardStructPage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CXMLFormWizardStructPage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CXMLFormWizardStructPage icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CXMLFormWizardStructPage icon retrieval
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_STRUCTPAGE
 */

void CXMLFormWizardStructPage::OnStructpagePageChanged( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_PAGE_CHANGED event handler for ID_STRUCTPAGE in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_PAGE_CHANGED event handler for ID_STRUCTPAGE in CXMLFormWizardStructPage. 

    // IF page content is not loaded yet, of if it is not valid
    if (!m_Loaded)
    {
        // Delete old content
        DeleteLevels();
        RepLevelTree->DeleteAllItems();
        char Path[256] = "";
        // Build form layout tree
        if (!BuildTree(RepLevelTree->GetRootItem(), GetDadTree()->root_element, (CXMLFormStructNode **)&FirstLevel, Path))
            return;
        // Find first not empty group and select it
        wxTreeItemId Item = FindNotEmptyLevel(RepLevelTree->GetRootItem());
        if (Item.IsOk())
            RepLevelTree->SelectItem(Item);

        m_Loaded = true;
    }
    // IF form type is report and selected group has subgroup, show aggregate columns, otherwise hide them
    CXMLFormStructLevel *Level = m_SelLevel;
    if (!Level)
        Level = FirstLevel; 
    ShowAggrCols(Type == XFT_REPORT && Level->HasSubLevel());
}
//
// Shows or hides aggregate columns
//
void CXMLFormWizardStructPage::ShowAggrCols(bool Show)
{
    // !!! Agregatove sloupce nema smysl zobrazovat pro levely na nejnizsi urovni, jen pro ty, co obsahuji dalsi level 
    if (Show)
    {
        // IF columns are not shown yet
        if (RepElemGrid->GetNumberCols() == 2)
        {
            // Create Aggregate column
            RepElemGrid->AppendCols(2);
            RepElemGrid->SetColLabelValue(2, _("Aggregate"));
            wxGridCellODChoiceEditor *ed = new wxGridCellODChoiceEditor(sizeof(Aggregates) / sizeof(wxString), Aggregates, false);
            RepElemGrid->SetCellEditor(0, 2, ed);

            // Create Aggregate element column
            wxString Hdr = _("Aggregate element");
            RepElemGrid->SetColLabelValue(3, Hdr);
            wxClientDC dc(this);
            wxCoord w, h;
            dc.GetTextExtent(Hdr, &w, &h, NULL, NULL, &RepElemGrid->GetLabelFont());
            RepElemGrid->SetColSize(3, w + 10);
            wxGridCellAttr *CellAttr = new wxGridCellAttr();
            if (CellAttr)
            {
                CellAttr->SetOverflow(false);
                RepElemGrid->SetColAttr(2, CellAttr);
            }
            CellAttr = new wxGridCellAttr();
            if (CellAttr)
            {
                CellAttr->SetOverflow(false);
                RepElemGrid->SetColAttr(3, CellAttr);
            }
        }
    }
    else
    {
        // IF columns are not hidden yet, remove aggregate columns
        if (RepElemGrid->GetNumberCols() == 4)
        {
            RepElemGrid->DeleteCols(2, 2);
        }
    }
}
//
// Removes aggregate elements from form struct when form type is changed from report to insert or update form
//
void CXMLFormWizardStructPage::RemoveAggrs(CXMLFormStructLevel *Level)
{
    if (!Level)
        return;
    for (CXMLFormStructNode *Elem = Level->FirstNode; Elem; Elem = Elem->Next)
    {
        if (Elem->IsElem)
        {
            ((CXMLFormStructElem *)Elem)->AggrFunc = XFA_NONE;
            ((CXMLFormStructElem *)Elem)->AggrElem = NULL;
        }
        else
        {
            RemoveAggrs((CXMLFormStructLevel *)Elem);
        }
    }
}

//
// Finds first group that contains at least one element
// Returns form layout tree item ID
//
wxTreeItemId CXMLFormWizardStructPage::FindNotEmptyLevel(const wxTreeItemId &Parent)
{
    CLevelTreeItemData *ItemData = (CLevelTreeItemData *)RepLevelTree->GetItemData(Parent);
    // Search current group content
    for (CXMLFormStructNode *Node = ItemData->Level->FirstNode; Node; Node = Node->Next)
    {
        // IF group contains element, return groups item ID
        if (Node->IsElem)
            return Parent;
    }
    // IF not empty group not found, try subgroups
    wxTreeItemIdValue cookie;
    for (wxTreeItemId Item = RepLevelTree->GetFirstChild(Parent, cookie); Item.IsOk(); Item = RepLevelTree->GetNextSibling(Item))
    {
        wxTreeItemId Result = FindNotEmptyLevel(Item);
        if (Result.IsOk())
            return Result;
    }
    return wxTreeItemId();
}
//
// Builds form layout tree
// Input:   Parent - Parent tree item
//          Node   - DAD node 
//          pLevel - Pointer to current group decriptor
//          Path   - Relative element path
// Returns: Pointer to next group decriptor
//
CXMLFormStructNode **CXMLFormWizardStructPage::BuildTree(const wxTreeItemId &Parent, const dad_element_node *Node, CXMLFormStructNode **pLevel, const char *Path)
{
    char SubPath[256];
    // Default tree item title is DAD element name
    const char *Title = Node->el_name;
    // IF Path is not empty, title will be elementname - elementpath
    if (*Path)
    {
        strcpy(SubPath, Node->el_name);
        strcat(SubPath, " - ");
        strcat(SubPath, Path);
        if (SubPath[strlen(SubPath) - 1] != '/')
            strcat(SubPath, "/");
        strcat(SubPath, Node->el_name);
        Title = SubPath;
    }

    // Append tree item
    wxTreeItemId Item = RepLevelTree->AppendItem(Parent, wxString(Title, *m_cdp->sys_conv));
    if (!Item.IsOk())
        return NULL;
    // Create group struct
    CXMLFormStructLevel *Level = new CXMLFormStructLevel((dad_element_node *)Node);
    if (!Level)
        return NULL;
    // Set it as tree item data
    CLevelTreeItemData *ItemData = new CLevelTreeItemData(Level);
    if (!ItemData)
        return NULL;
    RepLevelTree->SetItemData(Item, ItemData);
    *pLevel = Level;
    // Build subgroups
    *SubPath = 0;
    CXMLFormStructNode **pElem = BuildNode(Item, Node, &Level->FirstNode, SubPath);
    if (!pElem)
        return NULL;
    // Expand current tree item
    RepLevelTree->Expand(Item);
    return &Level->Next;;
}
//
// Builds element group content
// Input:   Parent - Parent tree item
//          Node   - DAD node 
//          pLevel - Pointer to current element decriptor
//          Path   - Relative element path
// Returns: Pointer to next node decriptor
//
CXMLFormStructNode **CXMLFormWizardStructPage::BuildNode(const wxTreeItemId &Parent, const dad_element_node *Node, CXMLFormStructNode **pElem, char *Path)
{
    char *pp = Path + strlen(Path);

    // IF DAD node can have attributes
    if (!((CXMLFormWizard *)GetParent())->m_DefaultDAD || Node != DadTree->root_element)
    {
        // Traverse attributes
        dad_attribute_node *at;
        for (at = Node->attributes; at; at = at->next)
        {
            // IF attribute is linked to column, create form element descriptor for it
            if (at->is_linked_to_column())
            {
                *pp = '@';
                strcpy(pp + 1, at->attr_name);
                CXMLFormStructElem *Elem = new CXMLFormStructElem(at, Path);
                if (!Elem)
                    return NULL;
                *pElem = Elem;
                pElem  = &(*pElem)->Next;
            }
        }
    }
    // Traverse DAD node subelements
    dad_element_node *el;
    for (el = Node->sub_elements; el; el = el->next)
    {
        if (!((CXMLFormWizard *)GetParent())->m_DefaultDAD || Node != DadTree->root_element)
        {
            // IF DAD element is linked to column or empty, create form element descriptor for it
            if (el->is_linked_to_column() || !el->sub_elements)
            {
                strcpy(pp, el->el_name);
                CXMLFormStructElem *Elem = new CXMLFormStructElem(el, Path);
                if (!Elem)
                    return NULL;
                *pElem = Elem;
                pElem = &(*pElem)->Next;
            }
        }
        // IF repeatable DAD element, build group
        if (el->multi || *el->lower_table_name())
            pElem = BuildTree(Parent, el, pElem, Path);
        // ELSE IF not repeatable DAD element, add element name to relative path and build node content
        else
        {
            strcpy(pp, el->el_name);
            char *pd = pp + strlen(pp);
            *pd++ = '/';
            *pd = 0;
            pElem = BuildNode(Parent, el, pElem, Path);
        }
        if (!pElem)
            return NULL;
    }
    *pp = 0;
    return pElem;
}

#ifdef WINS
//
// Prevents wizard close when Escape is pressed during grid cell editing  
//
bool CXMLFormWizardStructPage::MSWProcessMessage(WXMSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
    {
        wxWindow *const wndThis = wxGetWindowFromHWND((WXHWND)pMsg->hwnd);
        wxWindow *const wndPar  = wndThis->GetParent();
        if (wndPar && wndPar->GetParent() == RepElemGrid)
        {
            RepElemGrid->EnableCellEditControl(false);
            return true;
        }
    }
    return false;
}

#endif
//
// Shows combo list on Alt + Down
// Hides combo list on Enter
//
void DropDownCombo(wxGrid *Grid, bool Show = true)
{
#ifdef WINS
    wxGridCellEditor *ed = Grid->GetCellEditor(Grid->GetGridCursorRow(), Grid->GetGridCursorCol());
    ::SendMessage((HWND)ed->GetControl()->GetHWND(), CB_SHOWDROPDOWN, Show, 0);
    ed->DecRef();
#endif
}

/*!
 * wxEVT_KEY_DOWN event handler for ID_GRID
 */

void CXMLFormWizardStructPage::OnKeyDown( wxKeyEvent& event )
{
////@begin wxEVT_KEY_DOWN event handler for ID_GRID in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_KEY_DOWN event handler for ID_GRID in CXMLFormWizardStructPage. 
    
    int Code = event.GetKeyCode();
    //if (Code == WXK_SHIFT)  // Jinak stisknuti shift vyvola edit mode
    //    return;
    wxWindow *Foc;
    wxGrid   *Grid;
    int Col; 
    int Row; 

    // IF Return pressed
    if (Code == WXK_RETURN || Code == WXK_NUMPAD_ENTER)
    {
        Foc = GetSrc(&Grid, &Row, &Col);
        // IF Return pressed in dropped combo, set combo value into grid cell
        if (IsCombo(Grid, Col))
        {
            wxGridCellEditor *ed = Grid->GetCellEditor(Row, Col);
            if (Foc == ed->GetControl())
            {
                event.Skip(false);
                DropDownCombo(Grid, false);
                wxString Val = ((wxOwnerDrawnComboBox *)Foc)->GetValue();
                Grid->EnableCellEditControl(false);
                Grid->SetCellValue(Row, Col, Val);
                if (Grid == RepElemGrid && Col == 0)
                    RepElemGrid->SetCellValue(Row, 1, Val + wxT(":"));

                return;
            }
            ed->DecRef();
        }
    }
    // IF Down pressed
    else if (Code == WXK_DOWN)
    {
        Foc = GetSrc(&Grid, &Row, &Col);
        if (Foc == Grid->GetGridWindow() || Foc == Grid->GetGridCornerLabelWindow())
        {
            // IF Alt + Down is pressed on combo, drop down combo
            if (event.AltDown() && IsCombo(Grid, Col) && !Grid->IsReadOnly(Row, Col))
            {
                Grid->EnableCellEditControl();
                DropDownCombo(Grid);
                event.Skip(false);
            }
            // IF Down is pressed on grid, select next row
            else if (Grid)
            {
                if (!NextElemRow(m_ElemSelRow))
                    event.Skip(false);
            }
        }
    }
    // IF Delete pressed
    else if (Code == WXK_DELETE)
    {
        // IF Delete pressed on grid
        Foc = GetSrc(&Grid, &Row, &Col);
        if (Grid)
        {
            CXMLFormStructLevel *Level = m_SelLevel;
            if (Level)
            {
                // IF whole row is selected
                wxArrayInt SelRows = Grid->GetSelectedRows();
                if (SelRows.Count() > 0)
                {
                    Row = SelRows.Item(0);
                    CXMLFormStructNode **pNode = Level->GetNode(Row);
                    CXMLFormStructNode  *Node  = *pNode;
                    // Not empty group cannot be deleted
                    if (Node)
                    {
                        if (!Node->IsElem && !((CXMLFormStructLevel *)Node)->Empty())
                        {
                            event.Skip(false);
                            return;
                        }
                        *pNode = Node->Next;
                        delete Node;
                    }
                    // IF last row, clear cells content
                    if (Row >= Grid->GetNumberRows() - 1)
                    {
                        Grid->SetCellValue(Row, 0, wxEmptyString);
                        Grid->SetCellValue(Row, 1, wxEmptyString);
                        if (Grid->GetNumberCols() > 2)
                        {
                            Grid->SetCellValue(Row, 2, wxEmptyString);
                            Grid->SetCellValue(Row, 3, wxEmptyString);
                        }
                    }
                    // ELSE not last row, delete row
                    else
                    {
                        Grid->DeleteRows(Row);
                    }
                    event.Skip(false);
                }
                // ELSE single cell is selected
                else //if (Col >= 3)
                {
                    CXMLFormStructNode *Node = *Level->GetNode(Row);
                    // Not empty group cannot be deleted
                    if (Node && !Node->IsElem && !((CXMLFormStructLevel *)Node)->Empty())
                    {
                        event.Skip(false);
                        return;
                    }
                    // Clear cell value
                    Grid->SetCellValue(Row, Col, wxEmptyString);
                    m_ElemChanged = true;
                    event.Skip(false);
                }
            }
        }
    }
    // IF Escape pressed
    else if (Code == WXK_ESCAPE)
    {
        // IF row drag is in process, cancel row drag
        if (m_InDrag)
        {
            StopDrag();
            event.Skip(false);
        }
    }
}
//
// Returns window with input focus, if the window is RepElemGrid, returns pointer to it in Grid
// and slected cell coordinates
//
wxWindow *CXMLFormWizardStructPage::GetSrc(wxGrid **Grid, int *Row, int *Col)
{
    *Grid          = NULL;
    wxWindow *Ctrl = FindFocus();
    for (wxWindow *w = Ctrl; ; w = w->GetParent())
    {
        if (w == RepElemGrid)
        {
            *Grid = (wxGrid *)w;
            *Row  = ((wxGrid *)w)->GetGridCursorRow(); 
            *Col  = ((wxGrid *)w)->GetGridCursorCol(); 
            break;
        }
    }
    return Ctrl;
}
//
// Returns true if Col-th column in the Grid is combo
//
bool CXMLFormWizardStructPage::IsCombo(wxGrid *Grid, int Col)
{
    return Col == 0 || Col == 2;
}
//
// If grid cell editor is shown, returns editors empty flag 
// otherwise returns grid cell value empty flag
//
bool IsEmpty(wxGrid *Grid, int Row, int Col)
{
    wxGridCellEditor *ed = Grid->GetCellEditor(Row, Col);
    wxControl *Ctrl = ed->GetControl();
    ed->DecRef();
    if (Ctrl && Ctrl->IsShown())
        return Ctrl->GetLabel().IsEmpty();
    else 
        return Grid->GetCellValue(Row, Col).IsEmpty();
}
//
// IF current row does not contain valid values, returns false
// IF requested row greater or equal row count, appends new one
//
bool CXMLFormWizardStructPage::NextElemRow(int Row)
{
    if (!IsElemValid(RepElemGrid->GetGridCursorRow()))
        return false;
    int Cnt = RepElemGrid->GetNumberRows();
    if (Row >= Cnt - 1 && !::IsEmpty(RepElemGrid, Row, 0))
    {
        RepElemGrid->AppendRows();
        RepElemGrid->SetCellEditor(Cnt, 0, RepElemGrid->GetCellEditor(0, 0));
        RepElemGrid->SetCellEditor(Cnt, 2, RepElemGrid->GetCellEditor(0, 2));
    }
    return true;
}

/*!
 * wxEVT_GRID_SELECT_CELL event handler for ID_GRID1
 */

void CXMLFormWizardStructPage::OnElemSelectCell( wxGridEvent& event )
{
////@begin wxEVT_GRID_SELECT_CELL event handler for ID_GRID1 in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_SELECT_CELL event handler for ID_GRID1 in CXMLFormWizardStructPage. 

    // EXIT IF cell select locked
    if (m_InElemSelCell)
        return;
    // Lock cell select
    m_InElemSelCell = true;
    // Close cell editor
    RepElemGrid->EnableCellEditControl(false);
    // IF current row changed, save changes
    int Row = event.GetRow();
    if (Row != m_ElemSelRow)
    {
        SaveElem(m_ElemSelRow);
        m_ElemSelRow = Row;
        wxArrayString Elems;
    }
    // Unlock cell select
    m_InElemSelCell = false;
}

/*!
 * wxEVT_LEFT_DOWN event handler for ID_GRID1
 */

void CXMLFormWizardStructPage::OnElemLeftDown( wxMouseEvent& event )
{
////@begin wxEVT_LEFT_DOWN event handler for ID_GRID1 in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_LEFT_DOWN event handler for ID_GRID1 in CXMLFormWizardStructPage. 

    // Recalc mouse position to grid cell
    wxPoint pos = RepElemGrid->CalcUnscrolledPosition(event.GetPosition());
    int Row = RepElemGrid->YToRow(pos.y);
    int Col = RepElemGrid->XToCol(pos.x);
    if (Row == -1 || Col == -1)
    {
        // Close cell editor
        RepElemGrid->EnableCellEditControl(false);
        // Save changes
        SaveElem(m_ElemSelRow);
        // IF current row does not contain valid values, disable new row selection
        if (!NextElemRow(RepElemGrid->GetNumberRows() - 1))
            event.Skip(false);
    }
}

/*!
 * wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID1
 */

void CXMLFormWizardStructPage::OnElemCellChange( wxGridEvent& event )
{
////@begin wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID1 in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID1 in CXMLFormWizardStructPage. 

    m_ElemChanged = true;
    int Row = event.GetRow();
    int Col = event.GetCol();
    // IF element name changed
    if (Col == 0)
    {
        wxString Val = RepElemGrid->GetCellValue(Row, 0);
        // IF Value is group name, set element name and label cell as read only
        bool     ro  = Val[0] == L'<';
        RepElemGrid->SetReadOnly(Row, 0, ro && !LevelIsEmpty(Row));
        RepElemGrid->SetReadOnly(Row, 1, ro);
        bool Aggrtbl  = false;
        if (!ro)
            Aggrtbl = IsAggregatableElem(Val);
        // IF grid has 4 columns and element is not aggregatable, set aggregate function and aggregate element cell as read only 
        if (RepElemGrid->GetNumberCols() > 2)
        {
            RepElemGrid->SetReadOnly(Row, 2, !Aggrtbl);
            RepElemGrid->SetReadOnly(Row, 3, !Aggrtbl);
        }
        // IF Value is element and label is empty, set default label as pure element name + ':'
        if (!ro && RepElemGrid->GetCellValue(Row, 1).IsEmpty())
        {
            Val          = Val.AfterLast(wxT('/'));
            if (Val[0] == wxT('@'))
                Val = Val.Mid(1);
            RepElemGrid->SetCellValue(Row, 1, Val + wxT(":"));
        }
    }
    // IF aggregate function changed and aggregate element is empty
    if (Col == 2 && RepElemGrid->GetCellValue(Row, 3).IsEmpty())
    {
        // IF aggregate function specified, set default aggregate element name as element name + '_' + aggregate function name
        wxString Aggr = RepElemGrid->GetCellValue(Row, 2);
        if (!Aggr.IsEmpty())
        {
            wxString Val  = RepElemGrid->GetCellValue(Row, 0);
            Val          = Val.AfterLast(wxT('/'));
            if (Val[0] == wxT('@'))
                Val = Val.Mid(1);
            RepElemGrid->SetCellValue(Row, 3, Val + wxT("_") + Aggr);
        }
    }
}
//
// Returns true if group defined by Row-th row is empty
//
bool CXMLFormWizardStructPage::LevelIsEmpty(int Row)
{
    CXMLFormStructNode *Node = *m_SelLevel->GetNode(Row);
    return ((CXMLFormStructLevel*)Node)->Empty();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void CXMLFormWizardStructPage::OnAddButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON in CXMLFormWizardStructPage. 

    // Fill Elems with m_SelLevels text elements and attributes
    wxArrayString Elems;
    if (!GetDataElements(Elems, m_SelLevel, false))
        return;

    int rc = RepElemGrid->GetNumberRows();  // rc = grid row count
    int nr = rc - 1;                        // nr = last row number
    // FOR each item in Elems
    for (int i = 0; i < Elems.Count(); i++)
    {
        int r;
        wxString Val = Elems[i];
        // IF item is present in the grid allready, continue
        for (r = 0; r < rc && Val.CmpNoCase(RepElemGrid->GetCellValue(r, 0)); r++);
        if (r < rc)
            continue;
        // Grid has no rows or last row is not empty, append new row
        if (nr < 0 || !RepElemGrid->GetCellValue(nr, 0).IsEmpty())
        {
            RepElemGrid->AppendRows();
            nr++;
            if (nr > 0)
            {
                RepElemGrid->SetCellEditor(nr, 0, RepElemGrid->GetCellEditor(0, 0));
                RepElemGrid->SetCellEditor(nr, 2, RepElemGrid->GetCellEditor(0, 2));
            }
        }
        // Set element name
        RepElemGrid->SetCellValue(nr, 0, Val);
        // Set default label as pure element name + ':' 
        Val = Val.AfterLast(wxT('/'));
        if (Val[0] == wxT('@'))
            Val = Val.Mid(1);
        RepElemGrid->SetCellValue(nr, 1, Val + wxT(':'));
        // Save changes
        m_ElemChanged = true;
        SaveElem(nr);
    }
    // Enable Remove button if grid is not empty
    RemoveBut->Enable(nr > 0);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
 */

void CXMLFormWizardStructPage::OnRemoveButton1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1 in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1 in CXMLFormWizardStructPage. 

    // Remove elements from form descriptor
    RemoveElems(&m_SelLevel->FirstNode);
    // Clear grid content
    FillGrid();
}
//
// Removes elements from form descriptor
//
void CXMLFormWizardStructPage::RemoveElems(CXMLFormStructNode **Elem)
{
    if (!*Elem)
        return;
    RemoveElems(&(*Elem)->Next);
    if ((*Elem)->IsElem)
    {
        CXMLFormStructNode *dElem = *Elem;
        *Elem = dElem->Next;
        delete dElem;
    }
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_STRUCTPAGE
 */

void CXMLFormWizardStructPage::OnStructpagePageChanging( wxWizardEvent& event )
{
////@begin wxEVT_WIZARD_PAGE_CHANGING event handler for ID_STRUCTPAGE in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_WIZARD_PAGE_CHANGING event handler for ID_STRUCTPAGE in CXMLFormWizardStructPage. 

    // Close cell editor
    RepElemGrid->EnableCellEditControl(false);
    // IF current row values are valid, save changes
    int Row = RepElemGrid->GetGridCursorRow();
    if (IsElemValid(Row))
        SaveElem(Row);
    // ELSE disable page change
    else
    {
        event.Skip(false);
        event.Veto();
    }
}
//
// Returns true if grid row contains valid values
//
bool CXMLFormWizardStructPage::IsElemValid(int Row)
{
    int Col;
    // IF row is last row and contains empty cells only, return true
    if (Row >= RepElemGrid->GetNumberRows() - 1)
    {
        for (Col = 0; Col < RepElemGrid->GetNumberCols() && RepElemGrid->GetCellValue(Row, Col).IsEmpty(); Col++);
        if (Col >= RepElemGrid->GetNumberCols())
        {
            CXMLFormStructNode *Node = *m_SelLevel->GetNode(Row);
            if (!Node)
                return true;
        }
    }

    // IF element name is not empty
    Col = 0;
    if (!RepElemGrid->GetCellValue(Row, Col).IsEmpty())
    {
        // IF form is input or update form or no aggregates, return true
        if (Type != XFT_REPORT || RepElemGrid->GetNumberCols() <= 2)
            return true;
        // IF aggr function is empty and aggr element is empty or aggr function is not empty and aggr element is not empty, return true
        Col = 2;
        bool c2e = RepElemGrid->GetCellValue(Row, 2).IsEmpty();
        if (c2e == RepElemGrid->GetCellValue(Row, 3).IsEmpty())
            return true;
        if (!c2e)
            Col = 3;
    }
    // Select empty cell and show error message
    RepElemGrid->SetGridCursor(Row, Col);
    error_box(_("Field cannot be empty"), this);
    return false;
}    

/*!
 * wxEVT_GRID_LABEL_LEFT_CLICK event handler for ID_GRID
 */

void CXMLFormWizardStructPage::OnLabelLeftClick( wxGridEvent& event )
{
////@begin wxEVT_GRID_LABEL_LEFT_CLICK event handler for ID_GRID in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_LABEL_LEFT_CLICK event handler for ID_GRID in CXMLFormWizardStructPage. 

    // Close cell editor
    RepElemGrid->EnableCellEditControl(false);
    // Save changes
    SaveElem(m_ElemSelRow);
    // Set focus back to the grid
    ((wxGrid *)event.GetEventObject())->SetFocus();
}
//
// Mouse motion on grid row label event handler
//
void CXMLFormWizardStructPage::OnLabelMotion( wxMouseEvent& event )
{
    event.Skip();
    // IF left button is down
    if (event.LeftIsDown())
    {
        // IF row drag not in process
        if (!m_InDrag)
        {
            // IF grid has more then one row
            int rc;
            CXMLFormStructNode *Node;
            for (rc = 0, Node = m_SelLevel->FirstNode; Node; Node = Node->Next, rc++);
            if (rc > 1)
            {
                // m_Dragged = selected row number
                wxPoint pos = event.GetPosition();
                pos         = RepElemGrid->CalcUnscrolledPosition(pos);
                m_Dragged   = RepElemGrid->YToRow(pos.y);
                if (m_Dragged != wxNOT_FOUND)
                {
                    m_InDrag  = true;
                    // Capture mouse events for me
                    CaptureMouse();
                }
            }
        }
        event.Skip(false);
    }
}

/*!
 * wxEVT_MOTION event handler for ID_GRID1
 */

void CXMLFormWizardStructPage::OnMotion( wxMouseEvent& event )
{
////@begin wxEVT_MOTION event handler for ID_GRID1 in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_MOTION event handler for ID_GRID1 in CXMLFormWizardStructPage. 

    // IF row drag in process
    if (m_InDrag)
    {
        // Recalc mouse position to cell coordinates
        wxPoint pos  = event.GetPosition();
        wxWindow *gw = RepElemGrid->GetGridWindow();
        pos          = ClientToScreen(pos);
        pos          = gw->ScreenToClient(pos);
        pos          = RepElemGrid->CalcUnscrolledPosition(pos);
        int Row      = RepElemGrid->YToRow(pos.y);
        wxRect Rect;
        bool vsbl = false;
        // IF Row is valid
        if (Row != wxNOT_FOUND)
        {
            // Row is not visible, scroll
            vsbl   = RepElemGrid->IsVisible(Row, 0);
            if (!vsbl)
                RepElemGrid->MakeCellVisible(Row, 0);
            // Rect = cell[Row, 0] rectangle
            Rect = RepElemGrid->CellToRect(Row, 0);
        }
        // IF y < 0, Rect = cell[0, 0] rectangle
        else if (pos.y < 0)
        {
            Rect = RepElemGrid->CellToRect(0, 0);
        }
        // ELEE y > row count, Rect = cell[row count, 0] rectangle
        else
        {
            Row  = RepElemGrid->GetNumberRows();
            Rect = RepElemGrid->CellToRect(Row - 1, 0);
            Rect.y += Rect.height;
        }

        // IF current y != last y
        pos = RepElemGrid->CalcScrolledPosition(Rect.GetPosition());
        if (m_DragPos != pos.y)
        {
            wxRect *pRect = NULL;
            // IF current row is visible, refresh only changed rectangle, else refresh whole grid
            if (vsbl)
            {
                Rect.x      = 0;
                Rect.y      = m_DragPos - 1;
                Rect.width  = gw->GetClientSize().GetWidth();
                Rect.height = 3;  
                pRect       = &Rect;
            }
            gw->Refresh(false, pRect);
            gw->Update();
            m_DragPos   = pos.y;

            // Draw black line on current drag position
            wxClientDC dc(gw);
            wxPen Pen(*wxBLACK, 3);
            dc.SetPen(Pen);

            int gw, gh;
            RepElemGrid->GetClientSize(&gw, &gh);
            int c = RepElemGrid->GetNumberCols() - 1;
            Rect  = RepElemGrid->CellToRect(0, c);
            gh    = Rect.GetRight();
            if (gh < gw)
                gw = gh;

            dc.DrawLine(0, pos.y, gw, pos.y);
        }
        event.Skip(false);
    }
}

/*!
 * wxEVT_LEFT_UP event handler for ID_GRID1
 */

void CXMLFormWizardStructPage::OnLeftUp( wxMouseEvent& event )
{
////@begin wxEVT_LEFT_UP event handler for ID_GRID1 in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_LEFT_UP event handler for ID_GRID1 in CXMLFormWizardStructPage. 

    // IF row draf in process
    if (m_InDrag)
    {
        // Recalc mouse position to cell coordinates
        wxPoint pos  = event.GetPosition();
        wxWindow *gw = RepElemGrid->GetGridWindow();
        pos          = ClientToScreen(pos);
        pos          = gw->ScreenToClient(pos);
        pos          = RepElemGrid->CalcUnscrolledPosition(pos);
        int Row      = RepElemGrid->YToRow(pos.y);
        // IF destination row is not valid
        if (Row == wxNOT_FOUND)
        {
            // IF y < 0, destination row is first row
            if (pos.y < 0)
                Row = 0;
            // ELSE y > row count, destination row is after last row
            else
                Row = RepElemGrid->GetNumberRows();
        }
        // IF destination row is not dragged row, move element and refill grid 
        if (Row != m_Dragged)
        {
            m_SelLevel->MoveElem(m_Dragged, Row);
            FillGrid();
            RepElemGrid->MakeCellVisible(Row + 1, 0);
        }
        StopDrag();
    }
}
//
// Releases mouse capture and clears black line
//
void CXMLFormWizardStructPage::StopDrag()
{
    ReleaseMouse();
    RepElemGrid->GetGridWindow()->Refresh(false);
    m_InDrag = false;
}

/*!
 * wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL
 */

void CXMLFormWizardStructPage::OnTreectrlSelChanged( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL in CXMLFormWizardStructPage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL in CXMLFormWizardStructPage. 

    // Close cell editor, save changes
    RepElemGrid->EnableCellEditControl(false);
    SaveElem(m_ElemSelRow);
    // EXIT IF current group is not changed
    wxTreeItemId        SelItem  = event.GetItem();
    CLevelTreeItemData *ItemData = (CLevelTreeItemData *)RepLevelTree->GetItemData(SelItem);
    CXMLFormStructLevel *Level   = ItemData->Level;
    if (Level == m_SelLevel)
        return;
    m_SelLevel    = Level;
    m_ElemSelRow  = -1;
    // IF form is report and aggregates are posible, show aggregate columns
    if (Type == XFT_REPORT)
        ShowAggrCols(Level->HasSubLevel());
    // Refill grid
    FillGrid();
}
//
// Saves form element/group definition
//
void CXMLFormWizardStructPage::SaveElem(int Row)
{
    if (m_ElemChanged)
    {
        wxCharBuffer SrcName;
        CXMLFormAggrFunc af = XFA_NONE;
        // IF form is report and aggregate defined, af = aggregate function, SrcName = aggregate element
        if (Type == XFT_REPORT && RepElemGrid->GetNumberCols() > 2)
        {
            wxString AggrFunc = RepElemGrid->GetCellValue(Row, 2);
            if (AggrFunc == Aggregates[0])
                af = XFA_COUNT;
            else if (AggrFunc == Aggregates[1])
                af = XFA_SUM;
            else if (AggrFunc == Aggregates[2])
                af = XFA_AVG;
            SrcName = RepElemGrid->GetCellValue(Row, 3).mb_str(*m_cdp->sys_conv);
        }
        // Name = element/group name
        wxCharBuffer Name = RepElemGrid->GetCellValue(Row, 0).mb_str(*m_cdp->sys_conv);
        // Node = correasponding DAD node
        dad_node    *Node = NULL;
        if (m_SelLevel->Element)
            Node = GetNode(m_SelLevel->Element, Name); 
        // IF group, save group changes
        if (*Name == '<')
            m_SelLevel->SetLevel(Row, Node);
        // ELSE save element changes
        else
        {
            m_SelLevel->SetElem(Row, Name, RepElemGrid->GetCellValue(Row, 1).mb_str(*m_cdp->sys_conv), af, SrcName, Node);
            RemoveBut->Enable(true);
        }
        m_ElemChanged = false;
    }
}
//
// Finds DAD node for form element/group
// Input:   Node - Parent section node
//          Name - form element/group name
// Returns: DAD node
//
dad_node *CXMLFormWizardStructPage::GetNode(dad_element_node *Node, const char *Name)
{
    dad_element_node *el;
    // IF element/group is in current section
    const char *p = strchr(Name, '/');
    if (!p)
    {
        // IF form element is attribute, traverse attributes
        if (*Name == '@')
        {
            const char *nm = Name + 1;
            for (dad_attribute_node *at = Node->attributes; at; at = at->next)
            {
                if (strcmp(at->attr_name, nm) == 0)
                    return at;
            }
        }
        // IF group, traverse subelements
        if (*Name == '<')
        {
            int sz = (int)strlen(Name) - 2;
            for (el = Node->sub_elements; el; el = el->next)
            {
                if (strncmp(el->el_name, Name + 1, sz) == 0 && el->el_name[sz] == 0)
                    return el;
            }
        }
        // ELSE text element, traverse subelements
        else
        {
            for (el = Node->sub_elements; el; el = el->next)
            {
                if (strcmp(el->el_name, Name) == 0)
                    return el;
            }
        }
        return NULL;
    }
    // IF element/group is in sub section, traverse subsection
    for (el = Node->sub_elements; el; el = el->next)
    {
        if (strncmp(el->el_name, Name, p - Name) == 0 && el->el_name[p - Name] == 0)
        {
            dad_node *Result = GetNode(el, p + 1);
            if (Result)
                return Result;
        }
    }
    return NULL;
}
//
// Fills grid with current group content
//
void CXMLFormWizardStructPage::FillGrid()
{
    wxGridCellODChoiceEditor *ed2;
    // IF aggregate collumns are visible, ed2 = aggregate function editor
    bool AggrCols = RepElemGrid->GetNumberCols() > 2;
    if (AggrCols)
        ed2 = (wxGridCellODChoiceEditor *)RepElemGrid->GetCellEditor(0, 2);
    // Clear old grid content
    RepElemGrid->DeleteRows(0, RepElemGrid->GetNumberRows());
    // Fill Elems with text elements, atributes and subgroups
    wxArrayString Elems;
    GetDataElements(Elems, m_SelLevel, true);
    // ed0 = form element/group name editor, fill combo with elems
    wxGridCellODChoiceEditor *ed0  = new wxGridCellODChoiceEditor(Elems, true);
    // IF current group is empty
    if (m_SelLevel->Empty())
    {
        bool ReadOnly = Elems.IsEmpty();
        // Append one empty row
        RepElemGrid->AppendRows();
        // Set form element/group name editor
        RepElemGrid->SetCellEditor(0, 0, ed0);
        ed0->IncRef();
        // IF no elements are available, set name and label cell as read only
        RepElemGrid->SetReadOnly(0, 0, ReadOnly);
        RepElemGrid->SetReadOnly(0, 1, ReadOnly);
        // IF aggragates posible
        if (AggrCols)
        {
            // Set aggregate function editor
            RepElemGrid->SetCellEditor(0, 2, ed2);
            ed2->IncRef();
            // IF no elements are available, set aggregate function and aggregate element cell as read only
            RepElemGrid->SetReadOnly(0, 2, ReadOnly);
            RepElemGrid->SetReadOnly(0, 3, ReadOnly);
        }
        // Enable Add button if elements are available
        AddBut->Enable(!ReadOnly);
        // Disable Remove button
        RemoveBut->Enable(false);
    }
    // ELSE current group is not empty
    else
    {
        bool ec = false;
        int  r  = 0;
        // FOR each node in current group
        for (CXMLFormStructNode *Node = m_SelLevel->FirstNode; Node; Node = Node->Next, r++)
        {
            // Append row
            RepElemGrid->AppendRows();
            // form element/group name editor
            RepElemGrid->SetCellEditor(r, 0, ed0);
            ed0->IncRef();
            // IF aggragates posible, set aggregate function editor
            if (AggrCols)
            {
                RepElemGrid->SetCellEditor(r, 2, ed2);
                ed2->IncRef();
            }
            // IF node is group
            if (!Node->IsElem)
            {
                // Set group name
                RepElemGrid->SetCellValue(r, 0, wxT('<') + wxString(((CXMLFormStructLevel *)Node)->Element->el_name, *m_cdp->sys_conv) + wxT('>'));
                // IF group is not empty, set cell as read only
                RepElemGrid->SetReadOnly(r, 0, ((CXMLFormStructLevel *)Node)->FirstNode != NULL);
                // Set label cell as read only
                RepElemGrid->SetReadOnly(r, 1, true);
                // IF aggragates columns visible, set them as read only
                if (AggrCols)
                {
                    RepElemGrid->SetReadOnly(r, 2, true);
                    RepElemGrid->SetReadOnly(r, 3, true);
                }
            }
            // ELSE form element
            else
            {
                CXMLFormStructElem *Elem = (CXMLFormStructElem *)Node;
                wxString            Val  = wxString(Elem->Value, *m_cdp->sys_conv);
                // Set element name
                RepElemGrid->SetCellValue(r, 0, Val);
                // Sel element label
                RepElemGrid->SetCellValue(r, 1, wxString(Elem->Label, *m_cdp->sys_conv));
                // IF aggragates posible
                if (AggrCols)
                {
                    // IF aggregate function is defined, set aggregate function cell
                    if (Elem->AggrFunc != XFA_NONE)
                        RepElemGrid->SetCellValue(r, 2, Aggregates[(int)Elem->AggrFunc - 1]);
                    // Set aggregate element cell
                    RepElemGrid->SetCellValue(r, 3, wxString(Elem->AggrElem, *m_cdp->sys_conv));
                    // IF element is not aggregatable, set aggregate function and aggregate element cell as read only
                    bool Aggrtbl = IsAggregatableElem(Val);
                    RepElemGrid->SetReadOnly(r, 2, !Aggrtbl);
                    RepElemGrid->SetReadOnly(r, 3, !Aggrtbl);
                }
                ec = true;
            }
        }
        // Enable Add button
        AddBut->Enable(true);
        // IF grid is not empty, enable Remove button
        RemoveBut->Enable(ec);
    }
    ed0->DecRef();
    if (AggrCols)
        ed2->DecRef();
}
//
// Fills Elems list with group text elements, attributes and subgroups eventually
// Input:   Elems    - Destinalion list
//          Level    - group
//          SubLevel - false: text elements and attributes only, true: sub groups too
//
bool CXMLFormWizardStructPage::GetDataElements(wxArrayString &Elems, CXMLFormStructLevel *Level, bool SubLevel)
{
    AddDataElems(Level->Element, wxEmptyString, Elems, SubLevel);
    return !Elems.IsEmpty();
}
//
// Fills Elems list with DAD node text elements, attributes and repeatable sections eventually
// Input:   node     - DAD node
//          xPath    - Current xPath
//          Elems    - Destinalion list
//          SubLevel - false: text elements and attributes only, true: repeatable sections too
//
void CXMLFormWizardStructPage::AddDataElems(dad_element_node *node, const wxString &xPath, wxArrayString &Elems, bool SubLevel)
{   
    dad_element_node *el;
    // Add node attributes to Elems
    for (dad_attribute_node *at = node->attributes; at; at = at->next)
        Elems.Add(xPath + wxT('@') + wxString(at->attr_name, *m_cdp->sys_conv));
    // Add node subelements linked to column or empty to Elems 
    for (el = node->sub_elements; el; el = el->next)
    {
        if (el->is_linked_to_column() || !el->sub_elements)
            Elems.Add(xPath + wxString(el->el_name, *m_cdp->sys_conv));
    }
    // Traverse subelements
    for (el = node->sub_elements; el; el = el->next)
    {
        // IF repeatable sections too and subelement is repeatable, add it to Elems
        if (SubLevel && (el->multi || *el->lower_table_name()))
            Elems.Add(L'<' + wxString(el->el_name, *m_cdp->sys_conv) + L'>');
        // IF subelement has subsubelements, add them to Elems
        if (el->sub_elements)
            AddDataElems(el, xPath + wxString(el->el_name, *m_cdp->sys_conv) + wxT('/'), Elems, false);
    }
}
//
// 602XML supports only aggregates for two levels deep elements
//
// <Parent>
//     <Aggr2>          // Aggr2 can be aggregate of Elem
//     <Level2>
//         <Aggr1>      // Aggr1 cannot be aggregate of Elem
//         <Level1>
//             <Elem>
//             ...
//
// IsAggregatableElem returns true if element specified by Path can be aggregatable
//
bool CXMLFormWizardStructPage::IsAggregatableElem(const wxString &Path)
{
    // Find path separator
    wxCharBuffer aPath = Path.mb_str(*m_cdp->sys_conv);
    char *l1 = (char*)strchr(aPath, '/');
    // IF path separator not found, return false
    if (!l1)
        return false;
    *l1++ = 0;

    // IF string before path separator is not subelement name, return false
    dad_element_node *Node;
    for (Node = m_SelLevel->Element->sub_elements; Node; Node = Node->next)
    {
        if (strcmp(Node->el_name, aPath) == 0)
            break;
    }
    if (!Node)
        return false;

    // Find next path separator
    char *l2 = strchr(l1, '/');
    // IF next path separator not found, return false
    if (!l2)
        return false;
    *l2++ = 0;

    // IF string before next path separator is not subsubelement name, return false
    for (Node = Node->sub_elements; Node; Node = Node->next)
    {
        if (strcmp(Node->el_name, l1) == 0)
            break;
    }
    return Node != NULL;
}
