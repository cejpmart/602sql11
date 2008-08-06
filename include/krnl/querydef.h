#ifndef _QUERYDEF_H_
#define _QUERYDEF_H_
//
// Declares SELECT statement description structures, CQueryComp parses SELECT statement into tree of these structures
// CQueryDesigner uses it for visual SELECT statement editing
//
//
// Common query object ancestor
//
class CQueryObj
{
protected:

    CQueryObj *m_Next;  // Pointer to next object

public:

    CQueryObj(){m_Next = NULL;}
    virtual ~CQueryObj(){}
    CQueryObj *GetNext(){return(m_Next);}

    friend class CQueryObjList;
};
//
// Query object list
//
class CQueryObjList
{
protected:

    CQueryObj  *m_Hdr;  // First object in the list
    CQueryObj **m_Tail; // Pointer to last object in the list

public:

    CQueryObjList()
    {
        m_Hdr  = NULL;
        m_Tail = &m_Hdr;
    }

    virtual ~CQueryObjList()
    {
        Clear();
    }

    // Clears the list
    void Clear()
    {
        CQueryObj *dObj;
        for (CQueryObj *nObj = m_Hdr; nObj;)
        {
            dObj = nObj;
            nObj = nObj->m_Next;
            delete dObj;
        }
        m_Hdr  = NULL;
        m_Tail = &m_Hdr;
    }
    // Adds new object to the list
    void Add(CQueryObj *Obj)
    {
        *m_Tail = Obj;
        m_Tail = &Obj->m_Next;
    }
    // Removes object from the list
    void Remove(CQueryObj *Obj)
    {
        for (CQueryObj **pObj = &m_Hdr; *pObj; pObj = &((*pObj)->m_Next))
        {
            if (*pObj == Obj)
            {
                if (m_Tail == &Obj->m_Next)
                    m_Tail = pObj;
                *pObj = Obj->m_Next;
                break;
            }
        }
    }
    // Returns object count
    int GetCount() const
    {
        int Result = 0;
        for (CQueryObj *Obj = m_Hdr; Obj; Result++, Obj = Obj->m_Next);
        return(Result);
    }
    // Returns first object in the list
    CQueryObj *GetFirst(){return(m_Hdr);}
    // Moves content of this list to DstList
    void Move(CQueryObjList &DstList)
    {
        DstList.m_Hdr = m_Hdr;
        m_Hdr         = NULL;
        if (m_Tail != &m_Hdr)
        {
            DstList.m_Tail = m_Tail;
            m_Tail         = &m_Hdr;
        }
    }
};
//
// SELECT source table type
//
enum TTableType
{
    TT_TABLE,       // Table
    TT_QUERY,       // Simple query
    TT_QUERYEXPR,   // Query expression
    TT_JOIN         // Join
};
//
// SELECT source table ancestor
//
class CQTable : public CQueryObj
{
public:
    
    TTableType     TableType;   // Table type
    const char    *Alias;       // Table alias
    int            TableID;     // Unique table ID
    const d_table *TabDef;      // Table definition

    CQTable()
    {
        Alias   = NULL;
        TableID = 0;
        TabDef  = NULL;
    }
    virtual ~CQTable()
    {
        if (Alias)
            corefree(Alias);
    }
};
//
// Database table descriptor
//
class CQDBTable : public CQTable
{
public:

    const char *Schema;     // Schema name
    const char *Name;       // Table name
    const char *Index;      // Advised index

    CQDBTable()
    {
        TableType = TT_TABLE;
        Schema    = NULL;
        Name      = NULL;
        Index     = NULL;
    }
    virtual ~CQDBTable()
    {
        if (Schema)
            corefree(Schema);
        if (Name)
            corefree(Name);
        if (Index)
            corefree(Index);
    }
};
//
// Join source table descriptor
//
class CQJoinTable
{
public:

    CQTable    *Table;          // Source table
    CQTable    *CondTable;      // Table that figures in join condition
    const char *CondAttr;       // Column that figures in join condition

    CQJoinTable()
    {
        CondTable = NULL;
        CondAttr  = NULL;
    }
};
//
// Join type flags
//
typedef int TJoinType;

#define JT_LEFT       0x001
#define JT_RIGHT      0x002
#define JT_INNER      0x004
#define JT_OUTER      0x008
#define JT_CROSS      0x010
#define JT_NATURAL    0x020
#define JT_FULL       0x040
#define JT_ON         0x080
#define JT_USING      0x100
#define JT_WIZARDABLE 0x200     // Join can be display in query designer as line between two tables
//
// Join descriptor
//
class CQJoin : public CQTable
{
public:

    TJoinType   JoinType;   // Join flags
    CQJoinTable DstTable;   // Join destination table
    CQJoinTable SrcTable;   // Join source table
    const char *Cond;       // Join condition

    CQJoin()
    {
        TableType = TT_JOIN;
        JoinType  = 0;
        Cond      = NULL;
    }
    virtual ~CQJoin()
    {
        if (Cond)
            corefree(Cond);
    }
};
//
// ORDER BY expression decriptor
//
class CQOrder : public CQueryObj
{
public:

    const char *Expr;   // Single ORDER BY expression text
    bool        Desc;   // DESC flag
    
    CQOrder()
    {
        Expr = NULL;
        Desc = false;
    }
    virtual ~CQOrder()
    {
        if (Expr)
            corefree(Expr);
    }
    // Returns next ORDER BY expression
    CQOrder *GetNext(){return((CQOrder *)m_Next);}
};
//
// Query association type
//
enum TQueryAssoc {QA_NONE, QA_UNION, QA_INTERSECT, QA_EXCEPT};
//
// WHERE condition associatin type
//
enum TAndOr {CA_NONE, CA_AND, CA_OR};
//
// Query and query expression common ancestor
//
class CQUQuery : public CQTable
{
public:

    TQueryAssoc   Assoc;            // Query association type
    bool          All;              // ALL flag
    bool          Corresp;          // CORRESPONDING BY flag
    const char   *By;               // CORRESPONDING BY value
    const char   *Prefix;           // Text before SELECT in column or condition definition
    CQueryObjList OrderBy;          // ORDER BY expression list
    const char   *LimitOffset;      // LIMIT offset value
    const char   *LimitCount;       // LIMIT count value

    CQUQuery()
    {
        Assoc       = QA_NONE;
        All         = false;
        Corresp     = false;
        By          = NULL;
        Prefix      = NULL;
        LimitOffset = NULL;
        LimitCount  = NULL;
    }
    virtual ~CQUQuery()
    {
        if (By)
            corefree(By);
        if (Prefix)
            corefree(Prefix);
        if (LimitOffset)
            corefree(LimitOffset);
        if (LimitCount)
            corefree(LimitCount);
    }
};
//
// Column or condition definition ancestor
//
class CQQueryTerm : public CQueryObj
{
public:

    CQueryObjList Queries;  // SELECT statement list
    const char   *Expr;     // Definition text
    const char   *Postfix;  // Text after last SELECT statement in definition text

    CQQueryTerm()
    {
        Expr    = NULL;
        Postfix = NULL;
    }
    virtual ~CQQueryTerm()
    {
        if (Expr)
            corefree(Expr);
        if (Postfix)
            corefree(Postfix);
    }
};
//
// Query column descriptor
//
class CQColumn : public CQQueryTerm
{
public:

    const char *Alias;  // Column alias
    const char *Into;   // INTO value

    CQColumn()
    {
        Alias = NULL;
        Into  = NULL;
    }
    virtual ~CQColumn()
    {
        if (Alias)
            corefree(Alias);
        if (Into)
            corefree(Into);
    }
};
//
// WHERE condition descriptor
// If simple condition, Expr and Queries has value, SubConds is empty
// IF subcondition set, Expr and Queries are empty, SubConds has values
// 
class CQCond : public CQQueryTerm
{
public:

    CQueryObjList SubConds;     // Subcondition list
    TAndOr        Assoc;        // Association to next condition

    CQCond()
    {
        Assoc = CA_NONE;
    }
    virtual ~CQCond(){}
};
//
// Simple GROUP BY expression
//
class CQGroup : public CQueryObj
{
public:

    const char *Expr;           // Expression text

    CQGroup()
    {
        Expr = NULL;
    }
    virtual ~CQGroup()
    {
        if (Expr)
            corefree(Expr);
    }
};
//
// Simple query descriptor
//
class CQQuery : public CQUQuery
{
public:

    bool          Distinct;     // DISTINCT flag
    CQueryObjList Select;       // Column list
    CQueryObjList From;         // Source table list
    CQueryObjList Where;        // WHERE condition list
    CQueryObjList GroupBy;      // GROUP BY expression list
    CQueryObjList Having;       // HAVING condition list

    CQQuery()
    {
        TableType   = TT_QUERY;
        Distinct    = false;
    }
    virtual ~CQQuery(){}
};
//
// Cursor flags
//
#define CDF_INSENSITIVE 0x01
#define CDF_SENSITIVE   0x02
#define CDF_LOCKED      0x04
#define CDF_SCROLL      0x08
#define CDF_VIEW        0x10
//
// Query expression descriptor
//
class CQueryExpr : public CQUQuery
{
public:

    CQueryObjList Queries;      // Source queries list
    const char    *Source;      // Source text
    const char    *ViewName;    // View name
    int           CursorFlags;  // Cursor flags

    CQueryExpr()
    {
        TableType   = TT_QUERYEXPR;
        Source      = NULL;
        ViewName    = NULL;
        CursorFlags = 0;
    }
    virtual ~CQueryExpr()
    {
        if (Source)
            corefree(Source);
        if (ViewName)
            corefree(ViewName);
    }
};
//
// Simple filter condition used by Filter and order dialog
//
class CFilterCond : public CQueryObj
{
public:

    bool        Not;        // NOT flag
    const char *ColExpr;    // Column name
    char        Oper[8];    // Operator
    const char *ValExpr;    // Column value

    CQueryObjList SubConds; // Subcondition list
    TAndOr        Assoc;    // Association to next condition

    CFilterCond()
    {
        Not     = false;
        ColExpr = NULL;
        Oper[0] = 0;
        ValExpr = NULL;
        Assoc   = CA_NONE;
    }
    virtual ~CFilterCond()
    {
        if (ColExpr)
            corefree(ColExpr);
        if (ValExpr)
            corefree(ValExpr);
    }
    CFilterCond *GetNext(){return((CFilterCond *)m_Next);}
};
//
// Filter condition list used by Filter and order dialog
//
class CFilterConds : public CQueryObjList
{
public:

    CFilterCond *GetFirst(){return((CFilterCond *)m_Hdr);}
};
//
// Order condition list used by Filter and order dialog
//
class COrderConds : public CQueryObjList
{
public:

    CQOrder *GetFirst(){return((CQOrder *)m_Hdr);}
};

CFNC DllKernel int  WINAPI compile_query(cdp_t cdp, const char *defin, CQueryExpr *Expr);
CFNC DllKernel int  WINAPI compile_column(cdp_t cdp, const char *defin, CQColumn *Expr);
CFNC DllKernel int  WINAPI compile_cond(cdp_t cdp, const char *defin, CQCond *Expr);
CFNC DllKernel bool WINAPI is_sql_ident(cdp_t cdp, const char *defin);
CFNC DllKernel bool WINAPI is_join_cond(cdp_t cdp, const char *defin, char *DstSchema, char *DstTable, char *DstAttr, char *SrcSchema, char *SrcTable, char *SrcAttr);
CFNC DllKernel int  WINAPI compile_filter(cdp_t cdp, const char *defin, CFilterConds *Conds);
CFNC DllKernel int  WINAPI compile_order(cdp_t cdp, const char *defin, COrderConds *Conds);

#endif // _QUERYDEF_H_
