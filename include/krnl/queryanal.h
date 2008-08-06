#ifndef _QUERYANAL_H_
#define _QUERYANAL_H_

#include "querydef.h"
//
// SELECT statement compilator for query designer
//
#ifdef WINS
#define CompilError(mess) RaiseException(mess,0,0,NULL)
#else
#define CompilError(mess) MyThrow(CI->comp_jmp,mess)
#endif

enum TCompType {CT_QUERY, CT_COLUMN, CT_COND}; 

void GetValue(compil_info *CI);
const char *SkipComment(const char *p);
const char *LineUp(CIp_t CI, const char *Src, int Len);
int CompareIdents(cdp_t cdp, const char *Ident1, const char *Ident2);
//
// Provides SQLECT statement, single column expression or single WHERE condition compilation
// used by query designer
//
class CQueryComp : public compil_info
{
protected:

    xcdp_t    m_xcdp;
    TCompType m_CompType;
    CQQuery  *m_CurQuery;

    void AnalyzeQuery(CQueryExpr *QueryExpr);
    void GetRootQueryExpr(CQueryExpr *QueryExpr);
    void GetQuery();
    void GetColumn(CQColumn *Column);
    void GetColumn();
    void GetUnion(CQUQuery *Query);
    void GetOrder(CQueryExpr *QueryExpr);
    void GetWhereConds(CQueryObjList *Conds);
    void GetWhereCond(CQCond *Cond);
    void GetGroupExpr();
    void GetLimit(CQueryExpr *Query);
    CQUQuery *GetQueryExpr(CQueryObjList *ParentList);

    bool GetJoinAttrs(CQJoin *Join);
    bool TableFromAliasAttr(const char *Schema, const char *Alias, const char *Attr, CQTable *ScanTable, CQJoinTable *WrTable);
    bool IsEqualJoin(const char *Expr, char *DstSchema, char *DstTable, char *DstAttr, char *SrcSchema, char *SrcTable, char *SrcAttr);
    bool IsLimit(){return(cursym == S_IDENT && strcmp(curr_name, "LIMIT") == 0);}
    CQTable *TableFromAliasAttr(const char *Schema, const char *Alias, const char *Attr);
    CQTable *TableFromAliasAttr(const char *Schema, const char *Alias, const char *Attr, CQTable *Table);
    int GetAttrDef(CQTable *Table, const char *Attr, const d_attr **AttrDef = NULL);
    const d_table *CursTabDef(const char *Source, tcateg Categ);

    CQTable *GetTable();
    void     GetValue(){::GetValue(this);};

public:

    CQueryComp(cdp_t cdp, const char *Query, CQueryExpr *Expr) : compil_info(cdp, Query, DUMMY_OUT, Compile)
    {
        m_CompType = CT_QUERY;
        univ_ptr   = Expr;
        set_compiler_keywords(this, NULL, 0, 1);  // SQL keys
    }
    CQueryComp(cdp_t cdp, const char *Query, CQColumn *Expr) : compil_info(cdp, Query, DUMMY_OUT, Compile)
    {
        m_CompType = CT_COLUMN;
        univ_ptr   = Expr;
        set_compiler_keywords(this, NULL, 0, 1);  // SQL keys
    }
    CQueryComp(cdp_t cdp, const char *Query, CQCond *Expr) : compil_info(cdp, Query, DUMMY_OUT, Compile)
    {
        m_CompType = CT_COND;
        univ_ptr   = Expr;
        set_compiler_keywords(this, NULL, 0, 1);  // SQL keys
    }

    static void WINAPI Compile(CIp_t CI){ ((CQueryComp *)CI)->Compile(); }

    void Compile();
};

#if 0
class CIdentComp : public compil_info
{
public:

    CIdentComp(cdp_t cdp, const char *Defin) : compil_info(cdp, Defin, DUMMY_OUT, Compile)
    {
        set_compiler_keywords(this, NULL, 0, 1);  // SQL keys
    }

    static void WINAPI Compile(CIp_t CI){ ((CIdentComp *)CI)->Compile(); }

    void Compile();
};
#endif
//
// Compiles join condition which has Column1 operator Column2 form
//
class CJoinCondComp : public compil_info
{
protected:

    char *m_DstSchema;      // Destination column schema
    char *m_DstTable;       // Destination column table
    char *m_DstAttr;        // Destination column
    char *m_SrcSchema;      // Source column schema
    char *m_SrcTable;       // Source column table
    char *m_SrcAttr;        // Source column

public:

    CJoinCondComp(cdp_t cdp, const char *Defin, char *DstSchema, char *DstTable, char *DstAttr, char *SrcSchema, char *SrcTable, char *SrcAttr) : compil_info(cdp, Defin, DUMMY_OUT, Compile)
    {
        m_DstSchema = DstSchema;
        m_DstTable  = DstTable;
        m_DstAttr   = DstAttr;
        m_SrcSchema = SrcSchema;
        m_SrcTable  = SrcTable;
        m_SrcAttr   = SrcAttr;
        set_compiler_keywords(this, NULL, 0, 1);  // SQL keys
    }

    static void WINAPI Compile(CIp_t CI){ ((CJoinCondComp *)CI)->Compile(); }

    void Compile();
};
//
// Compiles WHERE condition for Filter and order dialog
//
class CFilterComp : public compil_info
{
protected:

    void GetWhereConds(CFilterConds *Conds);
    void GetWhereCond(CFilterCond *Cond);

public:

    CFilterComp(cdp_t cdp, const char *Defin, CFilterConds *Conds) : compil_info(cdp, Defin, DUMMY_OUT, Compile)
    {
        univ_ptr = Conds;
        set_compiler_keywords(this, NULL, 0, 1);  // SQL keys
    }

    static void WINAPI Compile(CIp_t CI){ ((CFilterComp *)CI)->Compile(); }

    void Compile();
};
//
// Compiles ORDER BY expression for Filter and order dialog
//
class COrderComp : public compil_info
{
public:

    COrderComp(cdp_t cdp, const char *Defin, COrderConds *Conds) : compil_info(cdp, Defin, DUMMY_OUT, Compile)
    {
        univ_ptr = Conds;
        set_compiler_keywords(this, NULL, 0, 1);  // SQL keys
    }

    static void WINAPI Compile(CIp_t CI){ ((COrderComp *)CI)->Compile(); }

    void Compile();
};
//
// Returns true if sym is contained in list psym
//
bool cursymIn(symbol sym, symbol *psym, int Len)
{
    for (int i = 0; i < Len; i++)
    {
        if (sym == psym[i])
            return(true);
        if (sym < psym[i])
            break;
    }

    return(false);
}
//
// Returns true if current symbol is LIMIT (LIMIT is not keyword)
//
bool IsLimit(compil_info *CI){return(CI->cursym == S_IDENT && strcmp(CI->curr_name, "LIMIT") == 0);}

#endif // _QUERYANAL_H_
