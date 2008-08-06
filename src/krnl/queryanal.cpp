#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include "flstr.h"
#include "opstr.h"
#include <stdio.h>

#include "wbprezex.h"
#include "queryanal.h"
//
// SELECT statement compilator for query designer
//
//
// Compiles SELECT statement for query designer, returns error code and query description in Expr
//
CFNC DllKernel int WINAPI compile_query(cdp_t cdp, const char *defin, CQueryExpr *Expr)
{
    CQueryComp xCI(cdp, defin, Expr);
    return(compile(&xCI));
}
//
// Compiles column expression for query designer, returns error code and column description in Expr
//
CFNC DllKernel int WINAPI compile_column(cdp_t cdp, const char *defin, CQColumn *Expr)
{
    CQueryComp xCI(cdp, defin, Expr);
    return(compile(&xCI));
}
//
// Compiles WHERE condition for query designer, returns error code and condition description in Expr
//
CFNC DllKernel int WINAPI compile_cond(cdp_t cdp, const char *defin, CQCond *Expr)
{
    CQueryComp xCI(cdp, defin, Expr);
    return(compile(&xCI));
}
//
// Compiles SELECT statement, column expression or WHERE condition
//
void CQueryComp::Compile()
{
    if (m_CompType == CT_COLUMN)
        GetColumn((CQColumn *)univ_ptr);
    else if (m_CompType == CT_COND)
        GetWhereCond((CQCond *)univ_ptr);
    else
        GetRootQueryExpr((CQueryExpr *)univ_ptr);
}
//
// Compiles root query expression of statement, writes query description into QueryExpr
//
void CQueryComp::GetRootQueryExpr(CQueryExpr *QueryExpr)
{
    CIp_t CI = this;
    // Parse statement
    AnalyzeQuery(QueryExpr);
    // IF Statement terminated with ;, skip it
    if (cursym == ';')
        next_sym(this);
    // IF not end of statement, error
    if (cursym != S_SOURCE_END)
        CompilError(GARBAGE_AFTER_END);
    // IF only one SELECT in query expression, move ORDER and LIMIT clauses from query expression into query
    if (QueryExpr->Queries.GetCount() == 1)
    {
        CQUQuery *Query        = (CQUQuery *)QueryExpr->Queries.GetFirst();
        QueryExpr->OrderBy.Move(Query->OrderBy);
        Query->LimitOffset     = QueryExpr->LimitOffset;
        Query->LimitCount      = QueryExpr->LimitCount;
        QueryExpr->LimitOffset = NULL;
        QueryExpr->LimitCount  = NULL;
    }
}
//
// Compiles query expression, appends query expression description to ParentList, returns query descriptor
//
CQUQuery *CQueryComp::GetQueryExpr(CQueryObjList *ParentList)
{
    CIp_t CI = this;
    // Create new query expression descriptor 
    CQUQuery *Result = new CQueryExpr();
    if (!Result)
        CompilError(OUT_OF_MEMORY);
    // Append descriptor to query expression list
    ParentList->Add(Result);
    // Compile expression
    AnalyzeQuery((CQueryExpr *)Result);
    // IF only one SELECT in query expression
    if (((CQueryExpr *)Result)->Queries.GetCount() == 1)
    {
        // Move ORDER and LIMIT clauses from query expression into query
        CQUQuery *Query                     = (CQUQuery *)((CQueryExpr *)Result)->Queries.GetFirst(); 
        ((CQueryExpr *)Result)->OrderBy.Move(Query->OrderBy);
        Query->LimitOffset                  = ((CQueryExpr *)Result)->LimitOffset;
        Query->LimitCount                   = ((CQueryExpr *)Result)->LimitCount;
        ((CQueryExpr *)Result)->LimitOffset = NULL;
        ((CQueryExpr *)Result)->LimitCount  = NULL;
        // Replace query expression in ParentList with simple query
        ((CQueryExpr *)Result)->Queries.Remove(Query);
        ParentList->Remove(Result);
        delete Result;
        ParentList->Add(Query);
        Result = Query;
    }
    return(Result);
}
//
// Parses query expression, writes query expression description into QueryExpr
//
void CQueryComp::AnalyzeQuery(CQueryExpr *QueryExpr)
{ 
    CIp_t CI = this;
    CQQuery *OldQuery = m_CurQuery;
    const char *ps    = prepos;

    // Try INSENSITIVE CURSOR FOR
    if (cursym == S_IDENT)
    {
        if (lstrcmpi(curr_name, "INSENSITIVE") == 0)
            QueryExpr->CursorFlags |= CDF_INSENSITIVE;
        else if (lstrcmpi(curr_name, "SENSITIVE") == 0)
            QueryExpr->CursorFlags |= CDF_SENSITIVE;
        else if (lstrcmpi(curr_name, "LOCKED") == 0)
            QueryExpr->CursorFlags |= CDF_LOCKED;
        else
            CompilError(SQL_SYNTAX);
        next_sym(this);
        if (cursym == S_IDENT)
        {
            if (lstrcmpi(curr_name, "SCROLL") != 0)
                CompilError(SQL_SYNTAX);
            QueryExpr->CursorFlags |= CDF_SCROLL;
            next_sym(this);
        }
        if (cursym != SQ_CURSOR)
            CompilError(SQL_SYNTAX);
        next_sym(this);
        if (cursym != SQ_FOR)
            CompilError(SQL_SYNTAX);
        next_sym(this);
    }
    // Try VIEW Name AS
    else if (cursym == S_VIEW)
    {
        QueryExpr->CursorFlags |= CDF_VIEW;
        next_sym(this);
        if (cursym != S_IDENT)
            CompilError(SQL_SYNTAX);
        QueryExpr->ViewName = (const char *)corealloc(lstrlen(curr_name) + 1, 221);
        if (!QueryExpr->ViewName)
            CompilError(OUT_OF_MEMORY);
        lstrcpy((char *)QueryExpr->ViewName, curr_name);
        next_sym(this);
        if (cursym != S_AS)
            CompilError(SQL_SYNTAX);
        next_sym(this);
    }
    // Repeat 
    do
    {
        CQUQuery *Query;
        // IF Left bracket, parse query expression
        if (cursym == '(')
        {
            next_sym(this);
            Query = GetQueryExpr(&QueryExpr->Queries);
            if (cursym != ')')
                CompilError(RIGHT_PARENT_EXPECTED);
            next_sym(this);
        }
        // ELSE parse simple query
        else
        {
            // IF cursym != SELECT, error
            if (cursym != S_SELECT)
                CompilError(SELECT_EXPECTED);
            // Create new query descriptor
            m_CurQuery = new CQQuery();
            if (!m_CurQuery)
                CompilError(OUT_OF_MEMORY);
            // Append descriptor to query list
            QueryExpr->Queries.Add(m_CurQuery);
            // Parse query
            GetQuery();
            Query = m_CurQuery;
        }
        // IF cursym == UNION, INTERSECT OR EXCEPT
        if (cursym == S_UNION || cursym == SQ_INTERS || cursym == SQ_EXCEPT)
        {
            // Set query association
            if (cursym == S_UNION)
                Query->Assoc = QA_UNION;
            else if (cursym == SQ_INTERS)
                Query->Assoc = QA_INTERSECT;
            else if (cursym == SQ_EXCEPT)
                Query->Assoc = QA_EXCEPT;
            // Parse UNION parameters
            GetUnion(Query);
        }
    }
    // WHILE cursym is SELECT or (
    while (cursym == S_SELECT || cursym == '(');
    // IF cursym == ORDER BY
    if (cursym == S_ORDER)
    {
        next_sym(this);
        if (cursym != S_BY)
            CompilError(BY_EXPECTED);
        // Parse ORDER expression
        do
        {
            GetOrder(QueryExpr);
        }
        while (cursym == ',');
    }
    // IF cursym == LIMIT, parse LIMIT expression
    if (IsLimit()) //SQ_LIMIT)
        GetLimit(QueryExpr);
    // Shape query expression source text
    QueryExpr->Source = LineUp(this, ps, (int)(prepos - ps));
    m_CurQuery = OldQuery;
}
//
// Parses simple query
//
void CQueryComp::GetQuery()
{ 
    CIp_t CI = this;
    // IF cursym == DISTINCT, set Distinct flag
    CIpos Pos(this);
    next_sym(this);
    if (cursym == S_DISTINCT)
        m_CurQuery->Distinct = true;
    else
        Pos.restore(this);

    // REPEAT parse column WHILE cursym == ,
    do
    {
        GetColumn();
    }
    while (cursym == ',');

    // IF cursym != S_FROM, error
    if (cursym != S_FROM)
        CompilError(FROM_EXPECTED);
    // REPEAT parse table WHILE cursym == ,
    do
    {
        GetTable();
    }
    while (cursym == ',');

    // IF cursym == WHERE
    if (cursym == S_WHERE)
    {
        // REPEAT parse WHERE condition WHILE cursym == AND or OR
        do
            GetWhereConds(&m_CurQuery->Where);
        while (cursym == SQ_AND || cursym == SQ_OR);
    }

    // IF cursym == GROUP BY
    if (cursym == S_GROUP)
    {
        next_sym(this);
        if (cursym != S_BY)
            CompilError(BY_EXPECTED);
        // REPEAT parse GROUP BY expression WHILE cursym == ,
        do
        {
            next_sym(this);
            GetGroupExpr();
        }
        while (cursym == ',');

        // IF cursym == HAVING
        if (cursym == S_HAVING)
        {
            // REPEAT parse HAVING condition WHILE cursym == AND or OR
            do
                GetWhereConds(&m_CurQuery->Having);
            while (cursym == SQ_AND || cursym == SQ_OR);
        }
    }
}
//
// Parses single column definition
//
void CQueryComp::GetColumn()
{ 
    CIp_t CI = this;
    // IF cursym == FROM, error
    next_sym(this);
    if (cursym == S_FROM)
        CompilError(FAKTOR_EXPECTED);

    // Create column descriptor
    CQColumn *Column = new CQColumn;
    if (!Column)
        CompilError(OUT_OF_MEMORY);
    // Append descriptor to column list
    m_CurQuery->Select.Add(Column);
    GetColumn(Column);
}
//
// Parses single column definition, writes column description to Column
//
void CQueryComp::GetColumn(CQColumn *Column)
{ 
    CIp_t CI         = this;
    bool InSelect    = false;   // In SELECT statement flag
    bool AfterSelect = false;   // After SELECT statement flag

    const char *ps   = prepos;  // Column expression start
    const char *pe   = ps;      // Column expression end
    const char *pr   = prepos;  // Expression text before SELECT start
    const char *pl   = pr;      // Expression text before SELECT end
    int         cb   = 0;
    
    // REPEAT
    do
    {
        // IF cursym == left bracket, increment bracket reference
        if (cursym == '(' || cursym == '[')
        {
            cb++;
            pl = prepos;
        }
        // IF cursym == right bracket, decrement bracket reference
        else if (cursym == ')' || cursym == ']')
        {
            cb--;
            // IF in SELECT, set AfterSelect flag
            if (InSelect)
            {
                InSelect    = false;
                AfterSelect = true;
            }
        }
        // IF cursym == SELECT, parse query expression, set SELECT prefix
        if (cursym == S_SELECT)
        {
            InSelect        = true;
            CQUQuery *Query = GetQueryExpr(&Column->Queries);
            Query->Prefix   = LineUp(this, pr, (int)(pl - pr));
        }
        // ELSE 
        else
        {
            // Read next symbol
            next_sym(this);
            // Set expression end
            pe = prepos;
            // IF after select, set next prefix start
            if (AfterSelect)
            {
                pr = prepos;
                AfterSelect = false;
            }
        }
        // IF end of statement AND bracket reference != 0, error
        if (!cursym && cb > 0) 
            CompilError(RIGHT_PARENT_EXPECTED);
    }
    // UNTIL end of statement OR "," OR "AS" OR "INTO" OR "FROM"  AND bracket reference == 0
    while (cursym && ((cursym != ',' && cursym != S_AS && cursym != S_INTO && cursym != S_FROM) || (cb != 0)));
    // IF bracket reference != 0, error
    if (cb < 0) 
        CompilError(FROM_EXPECTED);
    if (cb > 0) 
        CompilError(RIGHT_PARENT_EXPECTED);
    // Shape column expression
    Column->Expr = LineUp(this, ps, (int)(pe - ps));
    // IF column expression contains SELECT statements, set expression posfix
    if (Column->Queries.GetCount() > 0 && pe > pr)
        Column->Postfix = LineUp(this, pr, (int)(pe - pr));

    // IF cursym == AS
    if (cursym == S_AS)
    {
        // IF next symbol is not ident, error
        next_sym(this);
        if (cursym != S_IDENT) 
            CompilError(IDENT_EXPECTED);
        // Set column alias
        ps = prepos;
        next_sym(this);
        Column->Alias = LineUp(this, ps, (int)(prepos - ps));
    }

    // IF cursym == INTO
    if (cursym == S_INTO)
    {
        // IF next symbol is not ident, error
        cursym = next_sym(this);
        if (cursym != S_IDENT) 
            CompilError(IDENT_EXPECTED);
        // Set column INTO value
        ps = prepos;
        next_sym(this);
        Column->Into = LineUp(this, ps, (int)(prepos - ps));
    }
}
//
// Parses single source table definition, returns table descriptor
//
CQTable *CQueryComp::GetTable()
{
    CIp_t CI = this;
    CQTable *Table = NULL;
    CQJoin  *Join  = NULL;
    int      cb    = 0;
    next_sym(this);

    // WHILE cursym == (, increment bracket reference
    while (cursym == '(')
    {
        cb++;
        next_sym(this);
    }
    // IF cursym == ident, parse table name
    if (cursym == S_IDENT)
    {
        // Create new table descriptor
        Table = new CQDBTable();
        if (!Table)
            CompilError(OUT_OF_MEMORY);
        // Append descriptor to source table list
        m_CurQuery->From.Add(Table);
        const char *ps = prepos;
        next_sym(this);
        // Set table name
        ((CQDBTable *)Table)->Name = LineUp(this, ps, (int)(prepos - ps));
        // IF cursym == .
        if (cursym == '.')
        {
            // IF next symbol is not ident, error
            next_sym(this);
            if (cursym != S_IDENT)
                CompilError(IDENT_EXPECTED);
            // Set schema and table name
            ps = prepos;
            next_sym(this);
            ((CQDBTable *)Table)->Schema = ((CQDBTable *)Table)->Name;
            ((CQDBTable *)Table)->Name   = LineUp(this, ps, (int)(prepos - ps));
        }
        // IF cursym == INDEX, set INDEX value
        if (cursym == S_INDEX)
        {
            next_sym(this);
            const char *ps = prepos;
            next_sym(this);
            ((CQDBTable *)Table)->Index = LineUp(this, ps, (int)(prepos - ps));
        }
    } 
    // ELSE SELECT
    else
    {
        // IF cursym != SELECT, error
        if (cursym != S_SELECT) 
            CompilError(TABLE_NAME_EXPECTED);
        // Parse query expression
        Table = GetQueryExpr(&m_CurQuery->From);
    }

    // WHILE cursym == ) and bracket reference > 0, decrement bracket reference
    while (cursym == ')' && cb > 0)
    {
        cb--;
        next_sym(this);
    }

    // IF cursym == AS, read next symbol
    if (cursym == S_AS)
        next_sym(this);
    // IF Alias, set table alias
    if (cursym == S_IDENT && strcmp(curr_name, "LIMIT") != 0)
    {
        const char *ps = prepos;
        next_sym(this);
        Table->Alias = LineUp(this, ps, (int)(prepos - ps));
    }

    // IF cursym == (, parse columns rename
    if (cursym == '(')
    {
        CQColumn *Column = NULL;
        // IF source table type is query, get first query column
        if (Table->TableType == TT_QUERY)
            Column = (CQColumn *)((CQQuery *)Table)->Select.GetFirst();
        // REPEAT 
        for (;;)
        {
            next_sym(this);
            // IF cursym != ident, error
            if (cursym != S_IDENT) 
                CompilError(IDENT_EXPECTED);
            const char *ps = prepos;
            next_sym(this);
            // Set column alias
            if (Column)
            {
                Column->Alias = LineUp(this, ps, (int)(prepos - ps));
                Column        = (CQColumn *)Column->GetNext();
            }
            // IF cursym != ',', break
            if (cursym != ',')
                break;
        }
        // IF cursym != ), error
        if (cursym != ')')
           CompilError(RIGHT_PARENT_EXPECTED);
        next_sym(this);
    }

    // WHILE cursym == ) and bracket reference > 0, decrement bracket reference
    while (cursym == ')' && cb > 0)
    {
        cb--;
        next_sym(this);
    }

    // WHILE cursym == JOIN
    while (cursym == S_JOIN || cursym == S_LEFT || cursym == S_RIGHT || cursym == SQ_INNER || cursym == SQ_CROSS || cursym == SQ_NATURAL || cursym == SQ_FULL)
    {
        // Create new join descriptor
        Join = new CQJoin();
        if (!Join)
            CompilError(OUT_OF_MEMORY);
        
        // Remove table from source table list, set table as join destination table, add join to source table list
        m_CurQuery->From.Remove(Table);
        m_CurQuery->From.Add(Join);
        Join->DstTable.Table = Table;
        // IF cursym == CROSS, NATURAL, INNER, LEFT, ..., set join type
        if (cursym == SQ_CROSS)
        {
            Join->JoinType |= JT_CROSS;
            next_sym(this);
        } 
        else
        {
            if (cursym == SQ_NATURAL)
            {
                Join->JoinType |= JT_NATURAL;
                next_sym(this);
            }
            if (cursym == SQ_INNER)
            {
                Join->JoinType |= JT_INNER;
                next_sym(this);
            } 
            else 
            {
                if (cursym == S_LEFT)
                    Join->JoinType |= JT_LEFT;
                else if (cursym == S_RIGHT)
                    Join->JoinType |= JT_RIGHT;
                else if (cursym == SQ_FULL)
                    Join->JoinType |= JT_FULL;
                if (Join->JoinType & (JT_LEFT | JT_RIGHT | JT_FULL))
                {
                    next_sym(this);
                    if (cursym == S_OUTER)
                    {
                        Join->JoinType |= JT_OUTER;
                        next_sym(this);
                    }
                }
            }
        }
        if (cursym != S_JOIN) 
            CompilError(JOIN_EXPECTED);
        // Parse join source table
        Table = GetTable();
        m_CurQuery->From.Remove(Table);
        Join->SrcTable.Table = Table;
        // IF cursym == ON
        if (cursym == S_ON)
        {
            // Set join type
            Join->JoinType |= JT_ON;
            next_sym(this);
            const char *ps = prepos;
            // Parse join condition
            for (;;)
            {
                while (cursym == '(')
                {
                    cb++;
                    next_sym(this);
                }
                GetValue();
                while (cb > 0 && cursym == ')')
                {
                    cb--;
                    next_sym(this);
                }
                if (cursym != SQ_AND && cursym != SQ_OR)
                    break;
                next_sym(this);
                if (cursym == SQ_NOT)
                    next_sym(this);
            }
            // Set join condition
            Join->Cond = LineUp(this, ps, (int)(prepos - ps));
        } 
        // IF cursym == USING
        else if (cursym == SQ_USING)
        {
            // Set join type
            Join->JoinType |= JT_USING;
            // Parse join condition
            next_sym(this);
            if (cursym != '(') 
                CompilError(LEFT_PARENT_EXPECTED);
            next_sym(this);
            const char *pe;
            const char *ps = prepos;
            do
            {
                pe = prepos;
                next_sym(this);
            } 
            while (cursym && cursym != ')');
            if (cursym != ')') 
                CompilError(RIGHT_PARENT_EXPECTED);
            // Set join condition
            Join->Cond = LineUp(this, ps, (int)(pe - ps));
            next_sym(this);
        }
        // WHILE cursym == ) and bracket reference > 0, decrement bracket reference
        while (cb > 0 && cursym == ')')  
        {
            cb--;
            next_sym(this);
        }
        // Check if join condition is of type Column1 operator Column2
        GetJoinAttrs(Join);
        Table = Join;
    }
    return(Table);
}
//
// Parses WHERE/HAVING conditions, condition description writes to Conds
//
void  CQueryComp::GetWhereConds(CQueryObjList *Conds)
{
    CIp_t CI = this;
    // Create new condition descriptor
    CQCond *Cond = new CQCond();
    if (!Cond)
        CompilError(OUT_OF_MEMORY);
    // Append descriptor to the list
    Conds->Add(Cond);
    next_sym(this);
    CIpos Pos(this);
    const char *pt = prepos;
    // IF cursym == (, parse set of subconditions
    if (cursym == '(')
    {
        do
             GetWhereConds(&Cond->SubConds);
        while (cursym == SQ_AND || cursym == SQ_OR);
        if (cursym != ')') 
            CompilError(RIGHT_PARENT_EXPECTED);
        next_sym(this);
    }
    // IF no subconditions, restore parse position, parse simple condition
    if (Cond->SubConds.GetCount() <= 1)
    {
        Cond->SubConds.Clear();
        Pos.restore(this);
        GetWhereCond(Cond);
    }
    // Set condition association
    if (cursym == SQ_AND)
        Cond->Assoc = CA_AND;
    else if (cursym == SQ_OR)
        Cond->Assoc = CA_OR;
}
//
// WHERE condition elements
//
symbol WhereCondSep[] = {S_IDENT, S_INTEGER, S_REAL, S_STRING, S_CHAR, S_DATE, S_TIME, S_MONEY, S_TIMESTAMP, S_LESS_OR_EQ, S_GR_OR_EQ, S_NOT_EQ, S_PREFIX, S_SUBSTR, S_BINLIT, 
                         '(', ')', '*', '+', '-', '.', '/', ':', '<', '=', '>', '[', ']', 
                         S_ALL, SQ_ANY, S_AVG, S_BETWEEN, SQ_CAST, S_COUNT, SQ_DIV, SQ_ESCAPE, S_EXISTS, SQ_IN, S_IS, SQ_LIKE, S_MAX, S_MIN, SQ_MOD, SQ_NOT, S_NULL, SQ_SOME, S_SUM, S_UNIQUE};
//
// Parses simple WHERE/HAVING condition, condition description writes to Conds
//
void  CQueryComp::GetWhereCond(CQCond *Cond)
{
    CIp_t       CI   = this;
    const char *ps   = prepos;      // Condition text start
    const char *pe   = prepos;      // Condition text end
    const char *pr   = prepos;      // Text before SELECT statement start
    const char *pl   = pr;          // Text before SELECT statement end
    int         cb   = 0;
    int         cbb;
    bool InSelect    = false;       // Parsing SELECT statement
    bool AfterSelect = false;       // Parsing symbol after SELECT statement
    bool InBetween   = false;       // Parsing BETWEEN

    // REPEAT
    do
    {
        // IF cursym == left bracket, increment bracket reference, set SELECT prefix end
        if (cursym == '(' || cursym == '[')
        {
            cb++;
            pl = prepos;
        }
        // IF cursym == right bracket
        else if (cursym == ')' || cursym == ']')
        {
            // IF ) AND bracket reference == 0, terminate condition parse
            if (cursym == ')' && cb == 0) 
                break;
            // Decrement bracket reference
            cb--;
            // IF in SELECT statement, terminate SELECT statement parse
            if (InSelect)
            {
                InSelect    = false;
                AfterSelect = true;
            }
        }
        // IF cursym == BETWEEN, save current bracket reference
        else if (cursym == S_BETWEEN)
        {
            cbb       = cb;
            InBetween = true;
        }
        // IF cursym == SELECT, parse query, set SELECT statement prefix
        if (cursym == S_SELECT)
        {
            InSelect        = true;
            CQUQuery *Query = GetQueryExpr(&Cond->Queries);
            Query->Prefix   = LineUp(this, pr, (int)(pl - pr));
            pe              = prepos;
        } 
        else
        {
            next_sym(this);
            pe = prepos;
            // IF symbol folowing SELECT statement, set start of SELECT statement postfix
            if (AfterSelect)
            {
                pr = prepos;
                AfterSelect = false;
            }
            // IF cursym == AND and BETWEEN parse, ignore AND as condition terminator
            else if (cursym == SQ_AND && InBetween && cbb == cb)
            {
                InBetween = false;
                next_sym(this);
                continue;
            }
        }
        // IF statement end AND bracket reference > 0, error
        if (!cursym && cb > 0) 
            CompilError(RIGHT_PARENT_EXPECTED);
    } 
    // WHILE bracket reference > 0 OR not end of condition
    while (cb > 0 || (!IsLimit() && cursymIn(cursym, WhereCondSep, sizeof(WhereCondSep) / sizeof(symbol))));
    // Set condition text
    Cond->Expr = LineUp(this, ps, (int)(pe - ps));
    // IF condition contains SELECT statements, set SELECT statements postfix
    if (Cond->Queries.GetCount() > 0 && pe > pr)
        Cond->Postfix = LineUp(this, pr, (int)(pe - pr));
}
//
// Parses simple GROUP BY expression
//
void CQueryComp::GetGroupExpr()
{
    CIp_t CI = this;
    const char *ps = prepos;
    // Parse expression value
    GetValue();
    // Create new GROUP BY descriptor
    CQGroup *Group = new CQGroup();
    if (!Group)
        CompilError(OUT_OF_MEMORY);
    // Append descriptor to GROUP BY list
    m_CurQuery->GroupBy.Add(Group);
    // Set GROUP BY expression text
    Group->Expr = LineUp(this, ps, (int)(prepos - ps));
}
//
// Parses UNION parameters, writes UNION description to Query
//
void CQueryComp::GetUnion(CQUQuery *Query)
{
    CIp_t CI = this;
    next_sym(this);
    // IF cursym == ALL, set ALL flag
    if (cursym == S_ALL)
    {
        Query->All =  true;
        next_sym(this);
    }
    // IF cursym == CORRESPONDING, set CORRESPONDING flag, parse CORRESPONDING condition
    if (cursym == SQ_CORRESPONDING)
    {
        Query->Corresp = true;
        next_sym(this);
        if (cursym == S_BY)
        {
            next_sym(this);
            if (cursym != '(') 
                CompilError(LEFT_PARENT_EXPECTED);
            next_sym(this);
            const char *pe;
            const char *ps = prepos;
            do
            {
                next_sym(this);
                pe    = prepos;
            } 
            while (cursym && cursym != ')');
            if (cursym != ')')
                CompilError(RIGHT_PARENT_EXPECTED);
            Query->By = LineUp(this, ps, (int)(pe - ps));
            next_sym(this);
        }
    }
}
//
// Parses simple ORDER BY expression, writes ORDER BY description to Query
//
void  CQueryComp::GetOrder(CQueryExpr *Query)
{
    CIp_t CI = this;
    // Create new ORDER BY descriptor
    CQOrder *Order = new CQOrder();
    if (!Order)
        CompilError(OUT_OF_MEMORY);
    // Append descriptor to ORDER BY list
    Query->OrderBy.Add(Order);
    next_sym(this);
    const char *ps = prepos;
    // Parse expression
    GetValue();
    // Set expression text
    Order->Expr = LineUp(this, ps, (int)(prepos - ps)); 
    // IF cursym == ASC OR DESC, set DESC flag
    if (cursym == S_ASC || cursym == S_DESC)
    {
        Order->Desc = cursym == S_DESC;
        next_sym(this);
    }
}
//
// Parses LIMIT expression, writes LIMIT description to Query
//
void  CQueryComp::GetLimit(CQueryExpr *Query)
{
    next_sym(this);
    const char *pos = prepos;
    // Parse expression
    GetValue();
    // Set LIMIT count
    Query->LimitCount = LineUp(this, pos, (int)(prepos - pos));

    // IF cursym == ,
    if (cursym == ',')
    {
        next_sym(this);
        pos = prepos;
        // Parse expression
        GetValue();
        // Move count to LIMIT offset
        Query->LimitOffset = Query->LimitCount;
        // Set LIMIT count
        Query->LimitCount  = LineUp(this, pos, (int)(prepos - pos));
    }
}
//
// Checks if join condition has form Column1 operator Column2
// In such case we can display join query designer as line between two tables
//
bool CQueryComp::GetJoinAttrs(CQJoin *Join)
{
    char dSchema[OBJ_NAME_LEN + 3] = "";    // Destination schema
    char sSchema[OBJ_NAME_LEN + 3] = "";    // Source schema
    char dTable[OBJ_NAME_LEN + 3]  = "";    // Destination table
    char sTable[OBJ_NAME_LEN + 3]  = "";    // Source table
    char dAttr[OBJ_NAME_LEN + 3]   = "";    // Destination column
    char sAttr[OBJ_NAME_LEN + 3]   = "";    // Source column
    
    // IF JOIN ON, check if join form is Column1 operator Column2
    if (Join->JoinType & JT_ON)
    {
        IsEqualJoin(Join->Cond, dSchema, dTable, dAttr, sSchema, sTable, sAttr);
    } 
    // IF JOIN USING, check if one column only
    else if (Join->JoinType & JT_USING)
    {
        const char *Cond = Join->Cond;
        if (!strchr(Cond, ','))
        {
            strcpy(dAttr, Cond);
            strcpy(sAttr, dAttr);
        }
    } 
    // IF NATURAL JOIN, find corresponding columns
    else if (Join->JoinType & JT_NATURAL)
    {
        if (cdp && Join->DstTable.Table->TableType == TT_TABLE && Join->SrcTable.Table->TableType == TT_TABLE)
        {
            char Cmd[400];
            const char *Fmt = "SELECT Column_name "
                              "FROM  (SELECT Column_name FROM _IV_TABLE_COLUMNS WHERE Schema_name='%s' AND Table_name='%s') DstTable,"
                                    "(SELECT Column_name FROM _IV_TABLE_COLUMNS WHERE Schema_name='%s' AND Table_name='%s') SrcTable "
                              "WHERE DstTable.Column_name=SrcTable.Column_name";
            const char *ds = ((CQDBTable *)(Join->DstTable.Table))->Schema && ((CQDBTable *)(Join->DstTable.Table))->Schema[0] ? ((CQDBTable *)(Join->DstTable.Table))->Schema : cdp->sel_appl_name;
            const char *ss = ((CQDBTable *)(Join->SrcTable.Table))->Schema && ((CQDBTable *)(Join->SrcTable.Table))->Schema[0] ? ((CQDBTable *)(Join->SrcTable.Table))->Schema : cdp->sel_appl_name;
            sprintf(Cmd, Fmt, ds, ((CQDBTable *)(Join->DstTable.Table))->Name, ss, ((CQDBTable *)(Join->SrcTable.Table))->Name);
            if (!cd_SQL_value(cdp, Fmt, dAttr, sizeof(dAttr)))
                strcpy(sAttr, dAttr);
            else
                *dAttr = 0;
        }

    } 
    // IF destination and source column found
    if (*dAttr && *sAttr)
    {
        // Check if destination column belonns to join destination table AND 
        // source column belongs to join source table OR vice versa
        if ((TableFromAliasAttr(dSchema, dTable, dAttr, Join->DstTable.Table, &Join->DstTable)  &&
             TableFromAliasAttr(sSchema, sTable, sAttr, Join->SrcTable.Table, &Join->SrcTable)) ||
            (TableFromAliasAttr(dSchema, dTable, dAttr, Join->SrcTable.Table, &Join->SrcTable)  &&
             TableFromAliasAttr(sSchema, sTable, sAttr, Join->DstTable.Table, &Join->DstTable)))
        {
            // Set display line flag
            Join->JoinType |= JT_WIZARDABLE;
            return(true);
        }
    }
    return(false);
}
//
// Parses Schema.Table.Column symbol in CI and writes individual parts to Schema, Table and Attr
// Returns true on success
//
bool GetIdent(compil_info *CI, char *Schema, char *Table, char *Attr)
{
    const char *pss = NULL;
    const char *pts = NULL;
    const char *pse;
    const char *pte;
    const char *pas;
    const char *pae;

    *Schema = 0;
    *Table  = 0;
    *Attr   = 0;

    // IF cursym == ident, set column value
    if (CI->cursym != S_IDENT) 
        return(false);
    pas = CI->prepos;
    next_sym(CI);
    // IF cursym == '.'
    if (CI->cursym == '.')
    {
        // Move column value to table value
        pts = pas;
        pte = CI->prepos;
        next_sym(CI);
        if (CI->cursym != S_IDENT)
            return(false);
        // Set colmn value
        pas = CI->prepos;
        next_sym(CI);
        // IF cursym == '.'
        if (CI->cursym == '.')
        {
            // Move table value to schema value
            pss = pts;
            pse = pte;
            // Move column value to table value
            pts = pas; 
            pte = CI->prepos;
            next_sym(CI);
            if (CI->cursym != S_IDENT)
                return(false);
            // Set column value
            pas = CI->prepos;
            next_sym(CI);
        }
    }
    // IF Schema specified, write it to result
    if (pss)
    {
        int Len = (int)(pse - pss);
        strncpy(Schema, pss, Len);
        Schema[Len] = 0;
    }
    // IF Table specified, write it to result
    if (pts)
    {
        int Len = (int)(pte - pts);
        strncpy(Table, pts, Len);
        Table[Len] = 0;
    }
    // Trim white spaces on the end of column name
    for (pae = CI->prepos - 1; *pae == ' ' || *pae == '\t' || *pae == '\r' || *pae == '\n'; pae--);
    int Len = (int)(pae - pas + 1);
    // Write column name to result
    strncpy(Attr, pas, Len);
    Attr[Len] = 0;
    return(true);
}
//
// Parses join condition in CI, if it has form Column1 operator Column2, saves source and destination column specification
// to output arguments and returns true.
//
bool IsEqualJoin(compil_info *CI, char *DstSchema, char *DstTable, char *DstAttr, char *SrcSchema, char *SrcTable, char *SrcAttr)
{
    // Skip bracket 
    bool Bracket = CI->cursym == '(';
    if (Bracket)
        next_sym(CI);
    // IF cursym is column name
    if (GetIdent(CI, DstSchema, DstTable, DstAttr)) 
    {
        // Oper = cursym
        symbol Oper = CI->cursym;
        next_sym(CI);
        // IF cursym is column name
        if (GetIdent(CI, SrcSchema, SrcTable, SrcAttr))
        {
            if (Bracket && CI->cursym == ')')
                next_sym(CI);
            // IF end of expression, return true if Oper is operator
            if (!CI->cursym)
                return(Oper == '<' || Oper == '=' || Oper == '>' || Oper == S_NOT_EQ || Oper == S_LESS_OR_EQ || Oper == S_GR_OR_EQ);
        }
    }

    *DstAttr   = 0;
    *SrcAttr   = 0;
    return(false);
}
//
// Parses join condition in Expr, if it has form Column1 operator Column2, saves source and destination column specification
// to output arguments and returns true.
//
bool CQueryComp::IsEqualJoin(const char *Expr, char *DstSchema, char *DstTable, char *DstAttr, char *SrcSchema, char *SrcTable, char *SrcAttr)
{
    tobjname OldName;
    // Save CI state
    strcpy(OldName, curr_name);
    CIpos Pos(this);
    // Set compiled text
    compil_ptr  = Expr;
    curchar     = ' ';
    // Compile Expr
    next_sym(this);
    bool Result = ::IsEqualJoin(this, DstSchema, DstTable, DstAttr, SrcSchema, SrcTable, SrcAttr);
    // Restore CI state
    Pos.restore(this);
    strcpy(curr_name, OldName);
    return(Result);
}
//
// Checks if column specified by Schema, Alias and Attr belongs to ScanTable,
// if true sets ScanTable and Attr as source of displayable join
//
bool CQueryComp::TableFromAliasAttr(const char *Schema, const char *Alias, const char *Attr, CQTable *ScanTable, CQJoinTable *WrTable)
{
    CIp_t CI = this;
    bool Result = false;
    // IF Alias specified
    if (*Alias)
    {
        // IF Alias == Alias from ScanTable, found
        if (ScanTable->Alias && CompareIdents(cdp, Alias, ScanTable->Alias) == 0)
            Result = true;
        // IF query source table is database table
        else if (ScanTable->TableType == TT_TABLE)
        {
            // Check schema
            const char *ss = NULL;
            const char *ds = ((CQDBTable *)ScanTable)->Schema;
            if (Schema && *Schema)
            {
                ss = Schema;
                if (!ds && cdp)
                    ds = cdp->sel_appl_name;
            }
            else if (ds && cdp)
                ss = cdp->sel_appl_name;
            // IF schema matches AND, Alias == table name from ScanTable, found
            if ((!ss || CompareIdents(cdp, ss, ds) == 0) && CompareIdents(cdp, Alias, ((CQDBTable *)ScanTable)->Name) == 0)
                Result = true;
        }
    }
    // IF not found
    if (!Result)
    {
        // IF Source is join
        if (ScanTable->TableType == TT_JOIN)
        {
            // Try if column belongs to join destination or source table
            Result = TableFromAliasAttr(Schema, Alias, Attr, ((CQJoin *)ScanTable)->DstTable.Table, WrTable);
            if (!Result)
                Result = TableFromAliasAttr(Schema, Alias, Attr, ((CQJoin *)ScanTable)->SrcTable.Table, WrTable);
            return(Result);
        } 
        // IF Source is SELECT
        else if (ScanTable->TableType == TT_QUERY)
        {
            // Try if column belongs to some query source table
            for (CQTable *Table = (CQTable *)((CQQuery *)ScanTable)->From.GetFirst(); Table; Table = (CQTable *)Table->GetNext())
            {
                Result = TableFromAliasAttr(Schema, Alias, Attr, Table, WrTable);
                if (Result)
                    break;
            }
            if (!Result)
                return(false);
        } 
        // IF Source is query expression
        else if (ScanTable->TableType == TT_QUERYEXPR)
        {
            // Try if column belongs to some query expression source query
            for (CQUQuery *Query = (CQUQuery *)((CQueryExpr *)ScanTable)->Queries.GetFirst(); Query; Query = (CQUQuery *)Query->GetNext())
            {
                Result = TableFromAliasAttr(Schema, Alias, Attr, Query, WrTable);
                if (Result)
                    break;
            }
            if (!Result)
                return(false);
        }
        // ELSE try if column is from ScanTable column list
        else if (*Alias || GetAttrDef(ScanTable, Attr) == 0)
            return(false);
    }
    // IF found set join link source table and column
    WrTable->CondTable = ScanTable;
    WrTable->CondAttr  = (const char *)corealloc(lstrlen(Attr) + 1, 211);
    if (!WrTable->CondAttr)
        CompilError(OUT_OF_MEMORY);
    lstrcpy((char *)WrTable->CondAttr, Attr);
    return(true);
}
//
// Returns query source table that contains column specified by method arguments
//
CQTable *CQueryComp::TableFromAliasAttr(const char *Schema, const char *Alias, const char *Attr)
{
    CQTable *Result;
    // Scan all tables of current query
    for (CQTable *Table = (CQTable *)m_CurQuery->From.GetFirst(); Table; Table = (CQTable *)Table->GetNext())
    {
        Result = TableFromAliasAttr(Schema, Alias, Attr, Table);
        if (Result)
            return(Result);
    }
    return(NULL);
}
//
// Scans query source table Table if it or its source tables contains column specified by  Schema, Alias and Attr
// Returns source table, or NULL if not found
//
CQTable *CQueryComp::TableFromAliasAttr(const char *Schema, const char *Alias, const char *Attr, CQTable *Table)
{
    CQTable *Result = NULL;
    // IF Alias specified
    if (*Alias)
    {
        // IF Alias == Alias from Table, found
        if (CompareIdents(cdp, Alias, Table->Alias) == 0)
            Result = Table;
        // IF source table is database table
        else if (Table->TableType == TT_TABLE)
        {
            // Check schema
            const char *ss = NULL;
            const char *ds = ((CQDBTable *)Table)->Schema;
            if (Schema && *Schema)
            {
                ss = Schema;
                if (!ds && cdp)
                    ds = cdp->sel_appl_name;
            }
            else if (ds && cdp)
                ss = cdp->sel_appl_name;
            // Schema matches AND Alias == table name fro TABLE, found
            if ((!ss || CompareIdents(cdp, ss, ds) == 0) && CompareIdents(cdp, Alias, ((CQDBTable *)Table)->Name) == 0)
                Result = Table;
        }
    }
    // IF not found
    if (!Result)
    {
        // IF source table is join
        if (Table->TableType == TT_JOIN)
        {
            // Try if column belongs to join destination or source table
            Result = TableFromAliasAttr(Schema, Alias, Attr, ((CQJoin *)Table)->DstTable.Table);
            if (!Result)
                Result = TableFromAliasAttr(Schema, Alias, Attr, ((CQJoin *)Table)->SrcTable.Table);
        }
        // ELSE try if colun is from Table column list
        else if (!*Alias && GetAttrDef(Table, Attr))
            Result = Table;
    }
    return(Result);
}
//
// Returns column index and column definition of Attr, 0 if not found
//
int CQueryComp::GetAttrDef(CQTable *Table, const char *Attr, const d_attr **AttrDef)
{
    // Get table definition
    if (!Table->TabDef)
    {
        if (Table->TableType == TT_TABLE)
        {
            char Buf[2 * OBJ_NAME_LEN + 6];
            const char *pName = ((CQDBTable *)Table)->Name;
            if (((CQDBTable *)Table)->Schema)
            {
                strcpy(Buf, ((CQDBTable *)Table)->Schema);
                strcat(Buf, ".");
                strcpy(Buf, ((CQDBTable *)Table)->Name);
                pName = Buf;
            }
            Table->TabDef = CursTabDef(pName, CATEG_TABLE);
        }
        else
            Table->TabDef = CursTabDef(((CQueryExpr *)Table)->Source, CATEG_DIRCUR);
		if (!Table->TabDef)
			return(0);
    }
    // Scan column definitions
    for (int i = 1; i < Table->TabDef->attrcnt; i++)
    {
        if (CompareIdents(cdp, Attr, Table->TabDef->attribs[i].name) == 0)
        {
            if (AttrDef)
                *AttrDef = &Table->TabDef->attribs[i];
            return(i);
        }
    }
    return(0);
}
//
// Returns table definition for Source
//
const d_table *CQueryComp::CursTabDef(const char *Source, tcateg Categ)
{
    const d_table *Result = NULL;
    // IF 602sql connection
    if (cdp)
    {
        tcurstab CursTab = NOCURSOR;
        // IF source is SELECT statement, open cursor
        if (Categ == CATEG_DIRCUR || (strnicmp(Source, "SELECT", 6) == 0 && (Source[6] == ' ' || Source[6] == '\t' || Source[6] == '\r' || Source[6] == '\n')))
        {
            cd_Open_cursor_direct(cdp, Source, &CursTab);
        }
        // ELSE find source object
        else
        {
            cd_Find_object(cdp, Source, Categ, &CursTab);
            // IF not found, try stored cursor
            if (CursTab == NOOBJECT)
            {
                Categ = CATEG_CURSOR;
                cd_Find_object(cdp, Source, CATEG_CURSOR, &CursTab);
                // IF not found and source is system query, open cursor to system query 
                if (CursTab == NOOBJECT && strnicmp(Source, "_IV_", 4) == 0 && strlen(Source) <= OBJ_NAME_LEN)
                {
                    char SysQuery[16 + OBJ_NAME_LEN];
                    strcpy(SysQuery,      "SELECT * FROM "); 
                    strcpy(SysQuery + 14, Source);
                    cd_Open_cursor_direct(cdp, SysQuery, &CursTab);
                    Categ = CATEG_DIRCUR;
                }
            }
        }
        // Get table/cursor definition
        Result = cd_get_table_d(cdp, CursTab, Categ);
        if (CursTab != NOCURSOR && IS_CURSOR_NUM(CursTab))
            cd_Close_cursor(cdp, CursTab);
    }
#if WBVERS>=900
    // ELSE ODBC connection
    else
    {
        HSTMT hStmt;
        char  Buf[256];
        // Create SELECT statement
        if (Categ != CATEG_DIRCUR && (strnicmp(Source, "SELECT", 6) != 0 || (Source[6] != ' ' && Source[6] != '\t' && Source[6] != '\r' && Source[6] != '\n')))
        {
            strcpy(Buf, "SELECT * FROM ");
            strncpy(Buf + 14, Source, sizeof(Buf) - 15);
            Buf[sizeof(Buf) - 1] = 0;
            Source = Buf;
        }
        RETCODE rc = create_odbc_statement(m_xcdp->get_odbcconn(), &hStmt);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO || rc == SQL_NO_DATA)
        {
            // Execute statement
            rc = SQLExecDirect(hStmt, (SQLCHAR *)Source, SQL_NTS);
            if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO || rc == SQL_NO_DATA)
            {
                // Get result descriptor
                Result = make_result_descriptor9(m_xcdp->get_odbcconn(), hStmt);
                SQLCloseCursor(hStmt);
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
    }
#endif
    return(Result);
}
//
// Removes from string comments, white space sequence replaces with single space
//
const char *LineUp(CIp_t CI, const char *Text, int Len)
{
    const char *Res = (const char *)corealloc(Len + 1, 210);
    if (!Res)
        CompilError(OUT_OF_MEMORY);
    char *ps = (char *)Res;
    strmaxcpy(ps, Text, Len + 1);
    // WHILE not end of string
    while (*ps)
    {
        // Remove comment at current position
        const char *pe = SkipComment(ps);
        if (pe > ps)
            strcpy(ps, pe);
        // Skip string at current position
        if (*ps == '\'' || *ps == '"')
        {
            char c = *ps;
            do
            {
                ps++;
                if (*ps == c)
                {
                    ps++;
                    if (*ps != c)
                        break;
                }
            }
            while (*ps);
        }
        // IF white spaces
        else if (*ps == ' ' || *ps == '\r' || *ps == '\n' || *ps == '\t')
        {
            // pe = end of white space sequence
            pe = ps + 1;
            while (*pe == ' ' || *pe == '\r' || *pe == '\n' || *pe == '\t')
                pe++;
            // IF sigle space, skip
            if (pe == ps + 1 && *ps == ' ')
                ps++;
            else
            {
                // IF previous character is not bracket, write single space
                if (*pe && ps > Res && ps[-1] != '(' && *pe != ')')
                    *ps++ = ' ';
                // Remove rest of white space sequence
                strcpy(ps, pe);
            }
        }
        // ELSE next character
        else if (*ps)
        {
            ps++;
        }
    }
    ps--;
    // Trim spaces on the end of string
    while (ps >= Res && *ps == ' ')
        *ps-- = 0;
    return(Res);
}
//
// Returns pointer to end of comment starting at p
//
#define COMMENT_1 ('-' + ('-' << 8))
#define COMMENT_2 ('/' + ('/' << 8))
#define COMMENT_3 ('/' + ('*' << 8))
#define COMMENT_4 ('*' + ('/' << 8))

const char *SkipComment(const char *p)
{
    while (*p)
    {
        // IF // or --, find end of line
        if (*(WORD *)p == COMMENT_1 || *(WORD *)p == COMMENT_2)
        {
            const char *d = strchr(p, '\n');
            if (!d)
            {
                p += strlen(p);
                break;
            }
            p = d + 1;
            continue;
        }
        // IF /*, find */
        if (*(WORD *)p == COMMENT_3)
        {
            p += 2;
            while (*p)
            {
                p = strchr(p, '*');
                if (!p)
                    break;
                p++;
                if (*p == '/')
                    break;
            }
            if (!p || !*p)
                break;
            p++;
            continue;
        }
        // IF {, find }
        if (*p == '{')
        {
            p = strchr(p, '}');
            if (!p)
                break;
            p++;
            continue;
        }
        break;
    }
    return(p);
}
//
// Parses expression
//
void GetValue(compil_info *CI)
{
    int cb = 0;
    // REPEAT
    do
    {
        next_sym(CI);
        // IF cursym is left bracket, increment bracket reference
        if (CI->cursym == '(' || CI->cursym == '[')
            cb++;
        // IF cursym is right bracket
        else if (CI->cursym == ')' || CI->cursym == ']')
        {
            // IF bracket reference == 0, end of expression
            if (cb == 0)
                break;
            // decrement bracket reference
            cb--;
        }
    } 
    // WHILE bracket reference > 0 or cursym != expression terminator
    while (cb > 0 || (!IsLimit(CI) && cursymIn(CI->cursym, WhereCondSep, sizeof(WhereCondSep) / sizeof(symbol))));
}
//
// Returns Ident without `
//
const char *UnApostrIdent(const char *Ident, char *Buf)
{
    const char *pa = strchr(Ident, '`');
    if (!pa)
        return(Ident);

    const char *ps = Ident;
    char       *pd = Buf;

    do
    {
        if (pa > ps)
        {
            int len = (int)(pa - ps);
            memcpy(pd, ps, len);
            pd[len] = 0;
            pd     += len;
        }
        pa++;
        ps = pa;
        pa = strchr(pa, '`');
    }
    while (pa);
    if (*ps)
        strcpy(pd, ps);
    return(Buf);
}
//
// Compares two identificators
//
int CompareIdents(cdp_t cdp, const char *Ident1, const char *Ident2)
{
    char Buf1[256], Buf2[256];
    return(wb_stricmp(cdp, UnApostrIdent(Ident1, Buf1), UnApostrIdent(Ident2, Buf2))); 
}
#if 0
//
// Returns true if defin is SQL identifikator
//
CFNC DllKernel bool WINAPI is_sql_ident(cdp_t cdp, const char *defin)
{
    CIdentComp xCI(cdp, defin);
    return(compile(&xCI) == 0);
}

void CIdentComp::Compile()
{
    CIp_t CI = this;
    for (int i = 0; cursym == S_IDENT && i < 3; i++)
    {
        next_sym(this);
        if (cursym == S_SOURCE_END)
            return;
        if (cursym != '.')
            break;
        next_sym(this);
    }
    CompilError(SQL_SYNTAX);
}
#endif
//
// If defin has form Column1 operator Column2, stores Column1 and Column2 specification to output arguments
// and returns true
// 
CFNC DllKernel bool WINAPI is_join_cond(cdp_t cdp, const char *defin, char *DstSchema, char *DstTable, char *DstAttr, char *SrcSchema, char *SrcTable, char *SrcAttr)
{
    if (!defin || !*defin)
        return(false);
    CJoinCondComp xCI(cdp, defin, DstSchema, DstTable, DstAttr, SrcSchema, SrcTable, SrcAttr);
    return(compile(&xCI) == 0);
}

void CJoinCondComp::Compile()
{
    CIp_t CI = this;
    if (!IsEqualJoin(this, m_DstSchema, m_DstTable, m_DstAttr, m_SrcSchema, m_SrcTable, m_SrcAttr))
        CompilError(SQL_SYNTAX);
}
//
// Compiles WHERE condition specified in defin to set of filter descriptors used by
// Filter and order dialog, returns error code
//
CFNC DllKernel int WINAPI compile_filter(cdp_t cdp, const char *defin, CFilterConds *Conds)
{
    CFilterComp xCI(cdp, defin, Conds);
    return(compile(&xCI));
}
//
// Compiles WHERE condition to set of filter conditons 
//
void CFilterComp::Compile()
{
    CIp_t CI = this;
    // REPEAT Parse condition UNTIL cursym != AND or OR
    for (;;)
    {
        const char *pt = prepos;
        GetWhereConds((CFilterConds *)univ_ptr);
        if (pt == prepos)
            CompilError(SQL_SYNTAX);
        if (cursym != SQ_AND && cursym != SQ_OR)
            break;
        next_sym(this);
    }
    // IF not end of condition, error
    if (cursym != S_SOURCE_END)
        CompilError(SQL_SYNTAX);
}
//
// Parses WHERE condition to set of filter conditons 
//
void CFilterComp::GetWhereConds(CFilterConds *Conds)
{
    CIp_t CI = this;
    // Create new filter decriptor
    CFilterCond *Cond = new CFilterCond();
    if (!Cond)
        CompilError(OUT_OF_MEMORY);
    // Append descriptor to the filter list
    Conds->Add(Cond);
    // Save parse state
    CIpos Pos(this);
    const char *pt = prepos;
    // IF cursym == '(', parse set of subconditions
    if (cursym == '(')
    {
        do
        {
            next_sym(this);
            GetWhereConds((CFilterConds *)&Cond->SubConds);
        }
        while (cursym == SQ_AND || cursym == SQ_OR);
        if (cursym != ')') 
            CompilError(RIGHT_PARENT_EXPECTED);
        next_sym(this);
    }
    // IF no subconditions, restore parse position, parse simple condition
    if (Cond->SubConds.GetCount() <= 1)
    {
        Cond->SubConds.Clear();
        Pos.restore(this);
        GetWhereCond(Cond);
    }
    // Set subconditions association
    if (cursym == SQ_AND)
        Cond->Assoc = CA_AND;
    else if (cursym == SQ_OR)
        Cond->Assoc = CA_OR;
}
//
// Parses simple WHERE condition, writes condition description to Cond
//
symbol RelOperators[] = {S_LESS_OR_EQ, S_GR_OR_EQ, S_NOT_EQ, S_PREFIX, S_SUBSTR, '<', '=', '>', S_BETWEEN, S_EXISTS, SQ_IN, S_IS, SQ_LIKE};
symbol ExprElements[] = {S_IDENT, S_INTEGER, S_REAL, S_STRING, S_CHAR, S_DATE, S_TIME, S_MONEY, S_TIMESTAMP, S_BINLIT, 
                         '(', ')', '*', '+', '-', '.', '/', ':', '[', ']', S_ALL, SQ_ANY, SQ_ESCAPE, SQ_NOT, S_NULL, SQ_SOME, S_UNIQUE};

void  CFilterComp::GetWhereCond(CFilterCond *Cond)
{
    CIp_t CI = this;
    const char *ps   = prepos;
    int         cb   = 0;
    int         cbb;
    bool InSelect    = false;
    bool AfterSelect = false;
    bool InBetween   = false;

    // IF cursym == NOT, set NOT flag
    if (cursym == SQ_NOT)
    {
        Cond->Not = true;
        next_sym(this);
        ps = prepos;
    }

    do
    {
        // IF cursym is left bracket, increment bracket reference
        if (cursym == '(' || cursym == '[')
        {
            cb++;
        }
        // IF cursym is right bracket 
        else if (cursym == ')' || cursym == ']')
        {
            // IF cursym == ')' AND bracket reference == 0, end of condition 
            if (cursym == ')' && cb == 0) 
                break;
            // Decrement bracket reference
            cb--;
        }

        next_sym(this);
        // IF cursym is relational operator
        if (cursymIn(cursym, RelOperators, sizeof(RelOperators) / sizeof(symbol)))
        {
            // Store previous text to ColExpr
            Cond->ColExpr = LineUp(this, ps, (int)(prepos - ps)); 
            // Remove white spaces 
            for (ps = compil_ptr - 2; ps > prepos && (*ps == ' ' || *ps == '\t' || *ps == '\r' || *ps == '\n'); ps--);
            // Store operator to Oper
            strmaxcpy(Cond->Oper, prepos, ps - prepos + 2);
            // IF cursym == BETWEEN, store bracket reference
            if (cursym == S_BETWEEN)
            {
                cbb       = cb;
                InBetween = true;
            }
            next_sym(this);
            ps = prepos;
        }
        // IF cursym == AND and BETWEEN parse, ignore AND as condition terminator
        if (cursym == SQ_AND && InBetween && cbb == cb)
        {
            InBetween = false;
            next_sym(this);
            continue;
        }
        // IF end of conditon AND bracket reference > 0, error
        if (!cursym && cb > 0) 
            CompilError(RIGHT_PARENT_EXPECTED);
    } 
    // WHILE bracket reference > 0 AND sursym != condition terminator
    while (cb > 0 || (!IsLimit(this) && cursymIn(cursym, ExprElements, sizeof(ExprElements) / sizeof(symbol))));
    // IF some text after operator
    if (prepos > ps)
    {
        // IF ColExpr specified, set ValExpr value
        if (Cond->ColExpr)
            Cond->ValExpr = LineUp(this, ps, (int)(prepos - ps));
        // ELSE set ColExpr value
        else
            Cond->ColExpr = LineUp(this, ps, (int)(prepos - ps));
    }
}
//
// Compiles ORDER BY expression specified by defin to set of order descriptors used by
// Filter and order dialog, returns error code
//
CFNC DllKernel int WINAPI compile_order(cdp_t cdp, const char *defin, COrderConds *Conds)
{
    COrderComp xCI(cdp, defin, Conds);
    return(compile(&xCI));
}
//
// Compiles ORDER BY expression specified by defin to set of order descriptors
//
void COrderComp::Compile()
{
    CIp_t CI = this;
    for (;;)
    {
        // Create new order descriptor
        CQOrder *Order = new CQOrder();
        if (!Order)
            CompilError(OUT_OF_MEMORY);
        // Append descriptor to order list
        ((COrderConds *)univ_ptr)->Add(Order);
        const char *ps = prepos;
        // Parse expression
        GetValue(this);
        if (ps == prepos)
            CompilError(SQL_SYNTAX);
        // Set expression text
        Order->Expr = LineUp(this, ps, (int)(prepos - ps)); 
        // IF cursym == ASC OR DESC, set DESC flag
        if (cursym == S_ASC || cursym == S_DESC)
        {
            Order->Desc = cursym == S_DESC;
            next_sym(this);
        }
        // IF cursym != ',', break
        if (cursym != ',')
            break;
        next_sym(this);
    }
    // IF cursym != end of expression, error
    if (cursym != S_SOURCE_END)
        CompilError(COMMA_EXPECTED);
}

