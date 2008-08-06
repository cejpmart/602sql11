//
// Declares XML form layout structures. XML form wizard generates set of these structures
// and NewXMLForm function in 602xml dll produces XML form file based on this set.
//
#ifndef _XMLFORMS_H_
#define _XMLFORMS_H_

enum CXMLFormAggrFunc           // Element value type
{
    XFA_NONE,                   // No aggregate, simple value
    XFA_COUNT,                  // Count
    XFA_SUM,                    // Sum
    XFA_AVG                     // Avg
    //, XFA_MIN, XFA_MAX};      // Not supported by XPath
}; 
//
// Form element and group ancestor
//
struct CXMLFormStructNode
{
    CXMLFormStructNode *Next;   // Next node link
    bool                IsElem; // Element/group flag

    virtual void Delete() = 0;
};
//
// Form element
//
struct CXMLFormStructElem : public CXMLFormStructNode
{
    CXMLFormAggrFunc AggrFunc;  // Aggregate function
    dad_node        *Node;      // DAD node
    char            *Label;     // Element label
    char            *AggrElem;  // Aggregate element name
    char            *Value;     // Source element/attribute name

    CXMLFormStructElem(dad_node *aNode, const char *Path)
    {
        int ValSz = (int)strlen(Path);
        char *Lab = aNode->type() == DAD_NODE_ELEMENT ? ((dad_element_node *)aNode)->el_name : ((dad_attribute_node *)aNode)->attr_name;
        int LabSz = (int)strlen(Lab) + 1;
        Value = (char *)corealloc(ValSz + LabSz + 3, 19);
        if (Value)
        {
            strcpy(Value, Path);
            Label            = Value + ValSz + 1;
            strcpy(Label, Lab);
            Label[LabSz - 1] = ':';
            Label[LabSz]     = 0;
            AggrFunc         = XFA_NONE;
            AggrElem         = Label + LabSz + 1;
            AggrElem[0]      = 0;
            Node             = aNode;
            Next             = NULL;
            IsElem           = true;
        }
    }

    ~CXMLFormStructElem()
    {
         corefree(Value);
    }

    bool IsAttribute(){return *Value == '@';}   // Returns element/attribute flag
    const char *PureValue() const               // Returns pure element name, without path
    {
        const char *v = Value;
        if (AggrFunc != XFA_NONE)
            v = AggrElem;
        const char *p = strrchr(v, '/');
        if (p)
            p++;
        else
            p = v;
        const char *r = strchr(p, ':');
        if (r)
            r++;
        else
            r = p;
        return r;
    }

    void Delete()                               // Deletes this struct and all its descendents
    {
        if (!this)
            return;
        if (Next)
            Next->Delete();
        delete this;
    }
};
//
// Form group
//
struct CXMLFormStructLevel : public CXMLFormStructNode
{
    CXMLFormStructNode *FirstNode;  // Elements/Subgroups list
    dad_element_node   *Element;    // DAD element
    //bool                Done;
    
    CXMLFormStructLevel (dad_element_node *aElement)
    {
        Element   = aElement;
        Next      = NULL;
        FirstNode = NULL;
        //Done      = false;
        IsElem    = false;
    }

    void Delete()                                           // Deletes this struct and all its descendents
    {
        if (Next)
            Next->Delete();
        if (FirstNode)
            FirstNode->Delete();
        delete this;
    }

    const char *Value() const {return Element->el_name;}    // Returns source element name

    CXMLFormStructNode **GetNode(int Index)                 // Returns pointer to Index-th node from the list
    {
        CXMLFormStructNode **Result = &FirstNode;
        for (int i = 0; i < Index && *Result; i++)
            Result = &(*Result)->Next;
        return Result;
    }

    // Insers new or changes existing form element
    bool SetElem(int Index, const char *Val, const char *Label, CXMLFormAggrFunc AggrFunc, const char *AggrElem, dad_node *Node)
    {
        int  ValSz  = (int)strlen(Val);
        int  LabSz  = (int)strlen(Label);
        int  SrcSz  = AggrElem ? (int)strlen(AggrElem) : 0;
        int  Sz     = ValSz + LabSz + SrcSz + 3;
        // Find element
        CXMLFormStructNode **pElem = GetNode(Index);
        CXMLFormStructElem   *Elem = (CXMLFormStructElem *)*pElem;
        // IF element does not exist, create new one
        if (!Elem)
        {
            Elem       = new CXMLFormStructElem(Node, Val);
            if (!Elem)
                return false;
            *pElem     = Elem;
        }
        // ELSE IF node on Index-th position is a group, insert new element on its place
        else if (!Elem->IsElem)
        {
            CXMLFormStructLevel *Level = (CXMLFormStructLevel *)*pElem;
            *pElem     = (*pElem)->Next;
            delete Level;
            Elem       = new CXMLFormStructElem(Node, Val);
            if (!Elem)
                return false;
            Elem->Next = *pElem;
            *pElem     = Elem;
        }
        // IF new size > old element size, reallocate
        if (Sz > coresize(Elem->Value))
        {
            Elem->Value = (char *)corerealloc(Elem->Value, Sz);
            if (!Elem->Value)
                return false;
        }
        // Set new element properties
        strcpy(Elem->Value, Val);
        Elem->Label    = Elem->Value + ValSz + 1;
        strcpy(Elem->Label, Label);
        Elem->AggrFunc = AggrFunc;
        Elem->AggrElem = Elem->Label + LabSz + 1;
        if (AggrElem)
            strcpy(Elem->AggrElem, AggrElem);
        else
            *Elem->AggrElem = 0;
        Elem->Node     = Node;
        return true;
    }

    // Inserts new or changes existing form group
    bool SetLevel(int Index, dad_node *Node)
    {
        // Find group
        CXMLFormStructNode **pLevel = GetNode(Index);
        CXMLFormStructLevel *Level  = (CXMLFormStructLevel *)*pLevel;
        // IF group does not exist, create new one
        if (!Level)
        {

            Level   = new CXMLFormStructLevel((dad_element_node *)Node);
            if (!Level)
                return false;
            *pLevel = Level;
        }
        // ELSE IF node on Index-th position is an element, insert new group on its place
        else if (Level->IsElem)
        {
            CXMLFormStructElem *Elem = (CXMLFormStructElem *)*pLevel;
            *pLevel     = (*pLevel)->Next;
            delete Elem;
            Level       = new CXMLFormStructLevel((dad_element_node *)Node);
            if (!Level)
                return false;
            Level->Next = *pLevel;
            *pLevel     = Level;
        }
        return true;
    }

    bool Empty()                                    // Returns true if group is empty i.e. contains no elements
    {
        return FirstNode == NULL;
    }

    const char *PureValue() const                   // Returns pure element name, without path
    {
        const char *p = strchr(Value(), ':');
        if (p)
            p++;
        else
            p = Value();
        return p;
    }

    void MoveElem(int SrcIndex, int DstIndex)       // Moves form element from SrcIndex-th to DstIndex-th position
    {
        CXMLFormStructNode **SrcElem = GetNode(SrcIndex);
        CXMLFormStructNode **DstElem = GetNode(DstIndex);
        CXMLFormStructNode  *Elem    = *SrcElem;
        *SrcElem                     = Elem->Next;
        Elem->Next                   = *DstElem;
        *DstElem                     = Elem;
    }

    bool HasSubLevel()                              // Returns true if group has subgroups
    {
        for (CXMLFormStructNode *Node = FirstNode; Node; Node = Node->Next)
        {
            if (!Node->IsElem)
                return true;
        }
        return false;
    }
};
//
// Form layout root
//
enum CXMLFormType                       // XML form type
{
    XFT_REPORT,                         // Report
    XFT_INSFORM,                        // Insert form
    XFT_UPDFORM                         // Update form
};
enum CXMLFormLayout                     // XML form layout
{
    XFL_GRID,                           // Grid
    XFL_VERTLIST,                       // Vertical list
    XFL_RANDOM                          // Random layout
};

struct CXMLFormStruct
{
    CXMLFormType         Type;          // XML form type
    CXMLFormLayout       Layout;        // XML form layout
    CXMLFormStructLevel *FirstLevel;    // Top group
    t_dad_root          *DadTree;       // Source output DAD, DB -> form 
    t_dad_root          *DstDAD;        // Destination input DAT, form -> DB
    tobjname             FormName;      // Form object name
    tobjname             DadName;       // Object name for new output DAD if the form source is table or query
    tobjname             DstDadName;    // Object name for new input DAD if the form source is table or query
    bool                 Base;          // Form will be a DSP_XMLFORMSOURCE for DSParser

    CXMLFormStruct()
    {
        Type          = XFT_REPORT;
        Layout        = XFL_GRID;
        FirstLevel    = NULL;
        DadTree       = NULL;
        DstDAD        = NULL;
        DadName[0]    = 0;
        DstDadName[0] = 0;
        Base          = false;
    }

    ~CXMLFormStruct()
    {
        DeleteLevels();
        if (DadTree)
            delete DadTree;
        if (DstDAD)
            delete DstDAD;
    }

    t_dad_root *GetDadTree() const                          // Returns DAD tree
    {
        t_dad_root *Result = DadTree;
        if (!Result)
            Result = DstDAD;
        return Result;
    }

    void SetDadTree(t_dad_root *aDadTree, bool Dst = false) // Sets DAD tree
    {
        if (Dst)
        {
            if (DstDAD)
                delete DstDAD;
            DstDAD = aDadTree;
        }
        else
        {
            if (DadTree)
                delete DadTree;
            DadTree = aDadTree;
        }
    }

    void DeleteLevels()                                     // Clears form layout
    {
        if (FirstLevel)
        {
            FirstLevel->Delete();
            FirstLevel = NULL;
        }
    }        
    
    bool IsEmpty() const                                    // Returns true if form layout is empty
    {
        return this == NULL || FirstLevel == NULL;
    }
    
    const char *nsPrefix() const                            // Returns DAD namespace prefix
    {
        t_dad_root *dt = GetDadTree();
        if (dt && dt->nmsp_prefix && *dt->nmsp_prefix)
            return dt->nmsp_prefix;
        else
            return "md";
    }
};

#endif  // _XMLFORMS_H_
