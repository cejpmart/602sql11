/****************************************************************************/
/* sqlcomp.cpp - compilation of sql statements (top level)                  */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#pragma hdrstop
#include "xsa.h"
#include "profiler.h"
#include "nlmtexts.h"
#include <stddef.h>
#ifndef WINS
extern "C" int sprintf(char *__s, const char *__format, ... );
#endif

static const char return_value_name[] = "#RETVAL";

void compile_sql_statement_list(CIp_t CI, sql_statement ** pso);
void compile_sql_statement(CIp_t CI, sql_statement ** pso);

void supply_marker_type(CIp_t CI, t_expr_dynpar * dynpar, int type, t_specif specif, t_parmode use)
{ if (CI->cdp->sel_stmt==NULL) return;
  if (dynpar->num >= CI->cdp->sel_stmt->markers.count()) return;  // not registered
  MARKER * marker = CI->cdp->sel_stmt->markers.acc0(dynpar->num);
  marker->type  =dynpar->type  =dynpar->origtype  =type;
  marker->specif=dynpar->specif=dynpar->origspecif=specif;
  marker->input_output = (t_parmode)((int)marker->input_output | use);
}

static void read_attr_list(CIp_t CI, ttablenum tabobj, t_privils_flex & priv_val, bool write)
// Reads the attribute list and sets the read or write privils for these attributes
{ do
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);  /* skip '(' or ',' */
    bool found=false;  int i;
    { table_descr_auto td(CI->cdp, tabobj, TABLE_DESCR_LEVEL_COLUMNS_INDS);
      if (td->me()==NULL) CI->propagate_error();
      for (i=1;  i<td->attrcnt;  i++)
        if (!strcmp(td->attrs[i].name, CI->curr_name))
          { found=true;  break; }
    }
    if (!found)
      { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(UNDEF_ATTRIBUTE); }
    if (write) priv_val.add_write(i);
    else       priv_val.add_read(i);
  } while (next_sym(CI) == ',');
  test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
}

BOOL get_table_name(CIp_t CI, char * tabname, char * schemaname, ttablenum * ptabobj,  const t_locdecl ** plocdecl, int * plevel)
/* Reads schema and table name, if (ptabobj) searches for the table. 
   If the local table is found, returns NOTBNUM in *ptabobj and its locdecl in *plocdecl. */
{ if (plocdecl!=NULL) *plocdecl=NULL;
  if (CI->cursym!=S_IDENT) c_error(TABLE_NAME_EXPECTED);
  strmaxcpy(tabname, CI->curr_name, OBJ_NAME_LEN+1);
  if (next_sym(CI)=='.')
  { strcpy(schemaname, tabname);
    reduce_schema_name(CI->cdp, schemaname);
    next_and_test(CI, S_IDENT, TABLE_NAME_EXPECTED);
    strmaxcpy(tabname, CI->curr_name, OBJ_NAME_LEN+1);
    next_sym(CI);
  }
  else schemaname[0]=0;
  if (ptabobj!=NULL)
  { if (!*schemaname && plocdecl)  // search among local tables first:
    { t_locdecl * locdecl = find_name_in_scopes(CI, tabname, *plevel, LC_TABLE);
      if (locdecl != NULL) // locally declared table
      { *ptabobj=NOTBNUM;  // the local table does not have its number in compile-time
        *plocdecl=locdecl;
        return FALSE;
      }
    }
    return name_find2_obj(CI->cdp, tabname, schemaname, CATEG_TABLE, ptabobj);
  }
  else return FALSE;
}

void get_schema_and_name(CIp_t CI, tobjname name, tobjname schema)
// Is before name on entry, after name on exit. Reads name and/or schema.
{ next_and_test(CI, S_IDENT, IDENT_EXPECTED);
  strmaxcpy(name, CI->curr_name, sizeof(tobjname));
  if (next_sym(CI)=='.')
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    strcpy(schema, name);
    reduce_schema_name(CI->cdp, schema);
    strmaxcpy(name, CI->curr_name, sizeof(tobjname));
    next_sym(CI);
  }
  else *schema=0;
}

static t_query_expr * compile_condition(CIp_t CI, tobjnum objnum,
    tptr tabname, tptr schemaname, tcateg cat, tptr current_cursor_name, t_optim ** popt)
// tabname=="", schemaname, cat, objnum undefined iff FROM omitted.
// Returns qe for DELETE or UPDATE statement unless full table operation
// required or positioned operation compiled.
// Stores current_cursor_name if current operation requred.
{ tptr src;  int disp=0;
 /* find the condition: */
  if (CI->cursym==S_WHERE)
    if (next_sym(CI)==S_CURRENT)
    { next_and_test(CI, SQ_OF, OF_EXPECTED);
      next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      strmaxcpy(current_cursor_name, CI->curr_name, sizeof(tobjname));
      next_sym(CI);
      return NULL;
    }
    else // extract the condition source:
    { if (!*tabname) c_error(FROM_EXPECTED);
      const char * start=CI->prepos;
      while (CI->cursym && (CI->cursym!=';')) next_sym(CI);
      tptr stop=(tptr)CI->prepos;
      char from_obj[4+2*OBJ_NAME_LEN+2];
      if (*schemaname)
        { *from_obj='`';  strcpy(from_obj+1, schemaname);  strcat(from_obj, "`."); }
      else *from_obj=0;
      strcat(from_obj, "`");  strcat(from_obj, tabname);  strcat(from_obj, "`");
      char aux_c = *stop;  *stop=0;
      src=assemble_subcursor_def(start, from_obj, TRUE); // uses table or cursor name
      *stop=aux_c;
      disp=23+(int)strlen(from_obj)-(int)(start-CI->univ_source);
    }
  else // no WHERE clause
  { if (!*tabname) c_error(FROM_EXPECTED);
    if (cat==CATEG_TABLE) return NULL; // no condition -> full table operation
    else // full cursor operation
      src=ker_load_objdef(CI->cdp, objtab_descr, objnum);
  }
 // compile the cursor (may be restricted by WHERE):
  if (src==NULL) c_error(OUT_OF_MEMORY);
  t_query_expr * qe;  t_optim * opt;  int upper_disp;
  if (CI->cdp->sel_stmt!=NULL) 
    { upper_disp=CI->cdp->sel_stmt->disp;  CI->cdp->sel_stmt->disp=disp; }
  BOOL ret=compile_query(CI->cdp, src, CI->sql_kont, &qe, &opt);
  if (CI->cdp->sel_stmt!=NULL) 
    CI->cdp->sel_stmt->disp=upper_disp;
  corefree(src);
  if (ret) CI->propagate_error();  // propagates compilation error
  *popt=opt;  return qe;
}

static void savepoint_specifier(CIp_t CI, t_idname & name, t_express *& target)
{ if (next_sym(CI)==S_IDENT)
  { int level;
    if (find_name_in_scopes(CI, CI->curr_name, level, LC_VAR) == NULL)
    { strmaxcpy(name, CI->curr_name, sizeof(t_idname));
      next_sym(CI);
      return;
    }
  }
 // target specification:
  *name=0;
  sql_selector(CI, &target, false, false);
  if (target->sqe==SQE_DYNPAR)
  { //t_specif specif;  specif.set_num(0, 9);
    supply_marker_type(CI, (t_expr_dynpar*)target, ATT_INT32, t_specif(0, 9), MODE_INOUT);
  }
  else
    if (target->type != ATT_INT32 && target->type != ATT_INT16 || target->specif.scale!=0)
      c_error(MUST_BE_INTEGER);
}

/////////////////////// local declarations, t_scope //////////////////////////
t_locdecl * t_scope::find_name(const char * name, t_loc_categ loccat) const
{ for (int i = 0;  i<locdecls.count();  i++)
  { t_locdecl * locdecl = locdecls.acc0(i);
    if (!strncmp(name, locdecl->name, sizeof(locdecl->name)-1))
      if (locdecl->loccat==loccat || loccat>=LC_VAR && locdecl->loccat>=LC_VAR)
        return locdecl;
  }
  return NULL;
}

t_locdecl * find_name_in_scopes(CIp_t CI, const char * name, int & level, t_loc_categ loccat)
// Finds the name in all active scopes.
{ t_scope * scope = CI->sql_kont->active_scope_list;
  while (scope!=NULL)
  { t_locdecl * locdecl = scope->find_name(name, loccat);
    if (locdecl!=NULL)  // name found, calculate level and return
    { level=0;
      while (scope->next_scope!=NULL) { scope=scope->next_scope;  level++; }
      return locdecl;
    }
    scope=scope->next_scope;
  }
 // try the global scope:
  if (CI->cdp->global_sscope)
  { t_locdecl * locdecl = CI->cdp->global_sscope->find_name(name, loccat);
    if (locdecl!=NULL)
    { level=GLOBAL_DECLS_LEVEL;
      return locdecl;
    }
  }
  return NULL;
}

t_locdecl * t_scope::add_name(CIp_t CI, const char * name, t_loc_categ loccat)
// Adds a new name unless it is declared here or on parameter scope.
{// check for conflict in the namespace:
  if (find_name(name, loccat) != NULL)
    { SET_ERR_MSG(CI->cdp, name);  c_error(ID_DECLARED_TWICE); }
  if (loccat==LC_LABEL)  // check all upper levels, in the same routine, but not params
  { t_scope * ascope = next_scope;
    while (ascope!=NULL && !ascope->params)
    { if (ascope->find_name(name, loccat) != NULL)
        { SET_ERR_MSG(CI->cdp, name);  c_error(ID_DECLARED_TWICE); }
      ascope=ascope->next_scope;
    }
  }
  else if (loccat==LC_EXCEPTION)  // check all upper levels (params not necessary)
  {
#ifdef STOP // not in the new SQL 3
    t_scope * ascope = next_scope;
    while (ascope!=NULL)
    { if (!ascope->params)
        if (ascope->find_name(name, loccat) != NULL)
          { SET_ERR_MSG(CI->cdp, name);  c_error(ID_DECLARED_TWICE); }
      ascope=ascope->next_scope;
    }
#endif
  }
  else if (loccat!=LC_HANDLER && loccat!=LC_CURSOR) // check the params level
  { if (!params)  // check for the name conflict with parameters
    { t_scope * parscope = next_scope;
      while (parscope!=NULL && !parscope->params) parscope=parscope->next_scope;
      if (parscope!=NULL)
        if (parscope->find_name(name, loccat) != NULL)
          { SET_ERR_MSG(CI->cdp, name);  c_error(ID_DECLARED_TWICE); }
    }
  }
 // add the name and loccat:
  t_locdecl * locdecl = locdecls.next();
  if (locdecl==NULL) c_error(OUT_OF_MEMORY);
  strmaxcpy(locdecl->name, name, sizeof(locdecl->name));
  locdecl->loccat=loccat;
  return locdecl;
}

void t_scope::store_type(int type, t_specif specif, BOOL to_constant, t_express * defval)
{ int size = simple_type_size(type, specif);
  for (int i = stored_pos;  i<locdecls.count();  i++)
  { t_locdecl * locdecl = locdecls.acc0(i);
    if (to_constant) locdecl->loccat=LC_CONST;
    locdecl->var.type  =type;
    locdecl->var.specif=specif.opqval;
    locdecl->var.offset=extent;
    locdecl->var.defval=defval;
    locdecl->var.structured_type=false;
    extent+=size;
  }
}

void t_scope::store_statement(t_handler_type rt, sql_statement * stmt)
{ for (int i = stored_pos;  i<locdecls.count();  i++)
  { t_locdecl * locdecl = locdecls.acc0(i);
    locdecl->handler.stmt  = stmt;
    locdecl->handler.rtype = rt;
  }
}

void t_scope::activate(CIp_t CI)
{ next_scope=CI->sql_kont->active_scope_list;
  CI->sql_kont->active_scope_list=this;
}
void t_scope::deactivate(CIp_t CI)
{ CI->sql_kont->active_scope_list=next_scope; }

void t_locdecl::store_var(const char * nameIn, int typeIn, t_specif specifIn, int offsetIn, t_express * defvalIn)
{ strmaxcpy(name, nameIn, sizeof(name));
  loccat=LC_VAR;
  var.type  =typeIn;
  var.specif=specifIn.opqval;
  var.offset=offsetIn; 
  var.defval=defvalIn;
  var.structured_type=false;
}

t_scope::t_scope(void)  // used only when creating the system scope
{ core_init(0);  // called as static constructor before other inits, so must init the memory management
  next_scope=NULL;  extent=0;  params=FALSE;  system=TRUE;
 // adding SQL system variables (must be ordered by name):
  if (locdecls.acc(SYSTEM_VARIABLE_COUNT-1)==NULL) return;
  locdecls.acc0(0)->store_var("@@ACTIVE_RI_IN_PROGRESS", ATT_BOOLEAN, 0,  SYSVAR_ID_ACTIVE_RI, NULL);
  locdecls.acc0(1)->store_var("@@ERROR_MESSAGE",    ATT_TEXT, 0,     SYSVAR_ID_ERROR_MESSAGE, NULL);
  locdecls.acc0(2)->store_var("@@FULLTEXT_POSITION",ATT_INT32, 9<<8, SYSVAR_ID_FULLTEXT_POSITION, NULL);
  locdecls.acc0(3)->store_var("@@FULLTEXT_WEIGHT",  ATT_INT32, 9<<8, SYSVAR_ID_FULLTEXT_WEIGHT, NULL);
  locdecls.acc0(4)->store_var("@@IDENTITY",         ATT_INT64, 18<<8,SYSVAR_ID_IDENTITY, NULL);
  locdecls.acc0(5)->store_var("@@LAST_EXCEPTION",   ATT_STRING,IDNAME_LEN, SYSVAR_ID_LAST_EXC, NULL);
  locdecls.acc0(6)->store_var("@@PLATFORM",         ATT_INT32, 9<<8, SYSVAR_ID_PLATFORM, NULL);
  locdecls.acc0(7)->store_var("@@ROLLED_BACK",      ATT_BOOLEAN, 0,  SYSVAR_ID_ROLLED_BACK, NULL);
  locdecls.acc0(8)->store_var("@@ROWCOUNT",         ATT_INT32, 9<<8, SYSVAR_ID_ROWCOUNT, NULL);
  locdecls.acc0(9)->store_var("@@SQLOPTIONS",       ATT_INT32, 9<<8, SYSVAR_ID_SQLOPTIONS, NULL);
  locdecls.acc0(10)->store_var("@@WAITING",          ATT_INT32, 9<<8, SYSVAR_ID_WAITING,  NULL);
  locdecls.acc0(11)->store_var("SQLCODE",           ATT_INT32, 9<<8, SYSVAR_ID_SQLCODE,  NULL);
  locdecls.acc0(12)->store_var("SQLSTATE",          ATT_STRING, 5,   SYSVAR_ID_SQLSTATE, NULL);
}

t_scope::~t_scope(void)
{ if (system) return; // no destructing actions in the system scope
  t_express   * last_defval = NULL;
  sql_statement * last_stmt = NULL;
  for (int i=0;  i<locdecls.count();  i++)
  { t_locdecl * locdecl = locdecls.acc0(i);
    switch (locdecl->loccat)
    { case LC_ROUTINE:
        if (locdecl->rout.routine) delete locdecl->rout.routine;  break;  // owned and never cached
      case LC_TABLE:
        if (locdecl->table.ta) delete locdecl->table.ta;  break;  // owned
      case LC_HANDLER:
        if (locdecl->handler.stmt!=last_stmt)
        { last_stmt=locdecl->handler.stmt;
          delete last_stmt;
        }
        break;
      case LC_CURSOR: // co-owning, FOR statement owns too, OPEN does not
        if (locdecl->cursor.qe !=NULL) locdecl->cursor.qe ->Release();  
        if (locdecl->cursor.opt!=NULL) locdecl->cursor.opt->Release();
        break;
      case LC_EXCEPTION:  case LC_LABEL:
        break;
      default:  // variable, constant, parameter
        if (locdecl->var.defval!=last_defval)
        { last_defval=locdecl->var.defval;
          delete last_defval;
        }
        if (locdecl->var.structured_type)
          delete locdecl->var.stype;
        break;
    }
  }
}

void destruct_scope(t_scope * scope)
{ delete scope; }

void access_locdecl_struct(t_rscope * rscope, int column_num, t_locdecl *& locdecl, t_row_elem *& rel)
{ locdecl = rscope->sscope->locdecls.acc0(0);  // the structured variable
  t_struct_type * row = locdecl->var.stype;
  rel = row->elems.acc0(column_num-1);
}

void t_locdecl::mark_core(void)
{ switch (loccat)
  { case LC_ROUTINE:
      if (rout.routine) rout.routine->mark_core();  break;
    case LC_TABLE:
      if (table.ta) table.ta->mark_core();  break;
    case LC_HANDLER:
      if (handler.stmt) handler.stmt->mark_core();  break;
    case LC_CURSOR:
      if (cursor.qe)  cursor.qe ->mark_core();  
      if (cursor.opt) cursor.opt->mark_core();  break;
    case LC_VAR: case LC_CONST:  case LC_INPAR:  case LC_OUTPAR:  case LC_INOUTPAR:  case LC_FLEX:
    case LC_INPAR_:  case LC_OUTPAR_:  case LC_PAR:
      if (var.defval) var.defval->mark_core();  
      if (var.structured_type)
      { var.stype->elems.mark_core();
        mark(var.stype);
      }
      break;
  }
}

static int get_scope_level(CIp_t CI)
// Returns level of the new scope, called before activating it.
{ int level=0;
  t_scope * scope=CI->sql_kont->active_scope_list;
  while (scope!=NULL) { scope=scope->next_scope;  level++; }
  return level;
}

bool get_routine_name(const t_scope * sscope, tobjname name)
{
  for (int i = 0;  i<sscope->locdecls.count();  i++)
  { t_locdecl * locdecl = sscope->locdecls.acc0(i);
    if (locdecl->loccat==LC_LABEL)  // LABEL in params is the routine name
      { strcpy(name, locdecl->name);  return true; }
  }
  return false;
}

BOOL t_struct_type::fill_from_kontext(db_kontext * kont)
{ valsize=0;
  for (int i=1;  i<kont->elems.count();  i++)
  { elem_descr * el = kont->elems.acc0(i);
    t_row_elem * rel = elems.next();
    if (rel==NULL) return TRUE;
    strcpy(rel->name, el->name);
    rel->type=el->type;  rel->specif=el->specif;
    rel->offset=valsize;  rel->used=FALSE;
    valsize+=simple_type_size(rel->type, rel->specif);
  }
  return FALSE;
}

BOOL t_struct_type::fill_from_table_descr(const table_descr * tbdf)
{ valsize=0;
  for (int i=1;  i<tbdf->attrcnt;  i++)
  { const attribdef * att = tbdf->attrs+i;
    t_row_elem * rel = elems.next();
    if (rel==NULL) return TRUE;
    strcpy(rel->name, att->name);
    rel->type=att->attrtype;  rel->specif=att->attrspecif;
    rel->offset=valsize;
    valsize+=simple_type_size(rel->type, rel->specif);
  }
  return FALSE;
}

void t_scope::compile_declarations(CIp_t CI, int own_level, sql_statement ** p_init_stmt, BOOL in_atomic_block, BOOL routines_allowed, tobjnum container_objnum)
// Compiles declarations of the scope. Scope is supposed to be active.
{ int upper_an=CI->sql_kont->atomic_nesting;  CI->sql_kont->atomic_nesting=0;
  do
  { if (CI->cursym==SQ_DECLARE)
    { next_sym(CI);
      if (CI->cursym==SQ_PROCEDURE || CI->cursym==SQ_FUNCTION)
      { if (!routines_allowed) c_error(CANNOT_ON_GLOBAL_LEVEL);
        symbol sym1 = CI->cursym;  tobjname name, schema;
        get_schema_and_name(CI, name, schema);
        t_locdecl * locdecl = add_name(CI, name, LC_ROUTINE);  // schema ignored in local routines
        locdecl->rout.routine=new t_routine(CATEG_PROC, NOOBJECT);  
        if (!locdecl->rout.routine) c_error(OUT_OF_KERNEL_MEMORY);
        locdecl->rout.routine->compile_routine(CI, sym1, name, schema);
      }
      else if (CI->cursym==SQ_TABLE)
      { if (!routines_allowed) c_error(CANNOT_ON_GLOBAL_LEVEL);
       // compile table def to ta (ta not owned here):
        table_all * ta = new table_all;  if (!ta) c_error(OUT_OF_KERNEL_MEMORY);
        table_def_comp(CI, ta, NULL, CREATE_TABLE);
        add_implicit_constraint_names(ta);
       // add it to the scope:
        t_locdecl * locdecl = add_name(CI, ta->selfname, LC_TABLE);  // schema ignored in local tables
        locdecl->table.ta=ta;  
        locdecl->table.offset=extent;
        extent+=sizeof(ttablenum);
      }
      else if (CI->cursym==SQ_REDO || CI->cursym==SQ_UNDO ||
               CI->cursym==SQ_EXIT || CI->cursym==SQ_CONTINUE)
      { t_handler_type rt = CI->cursym==SQ_REDO ? HND_REDO :
                            CI->cursym==SQ_UNDO ? HND_UNDO :
                            CI->cursym==SQ_EXIT ? HND_EXIT : HND_CONTINUE;
        if (rt==HND_REDO || rt==HND_UNDO)
          if (!in_atomic_block) c_error(REDO_UNDO_ATOMIC);
        next_and_test(CI, SQ_HANDLER, HANDLER_EXPECTED);
        next_and_test(CI, SQ_FOR, FOR_EXPECTED);
        store_pos();
        do // FOR or comma is the current symbol
        { next_sym(CI);
         //next_value:
          if (CI->cursym==SQ_NOT)
          { if (next_sym(CI)!=S_IDENT || strcmp(CI->curr_name, "FOUND"))
              c_error(FOUND_EXPECTED);
            add_name(CI, not_found_class_name, LC_HANDLER);
            next_sym(CI);
          }
          else if (CI->cursym==SQ_SQLEXCEPTION || CI->cursym==SQ_SQLWARNING)
          { add_name(CI, CI->curr_name, LC_HANDLER);
            next_sym(CI);
          }
          else if (CI->cursym==SQ_SQLSTATE)
          { next_sym(CI);
            if (CI->is_ident("VALUE")) next_sym(CI);
            if (CI->cursym!=S_STRING) c_error(STRING_EXPECTED);
            if (strlen(CI->curr_string()) != 5) c_error(SQLSTATE_FORMAT);
            t_idname name;  *name=SQLSTATE_EXCEPTION_MARK;
            strcpy(name+1, CI->curr_string());
            if (rt==HND_CONTINUE || rt==HND_EXIT)
            { int error_code;
              if (!sqlstate_to_wb_error(CI->curr_string(), &error_code) || !may_have_handler(error_code, 0))
                c_error(CANNOT_HANDLE_THE_ERROR);
            }
            add_name(CI, name, LC_HANDLER);
            next_sym(CI);
          }
          else // declared exception name
          { if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
            int level;
            t_locdecl * locdecl=find_name_in_scopes(CI, CI->curr_name, level, LC_EXCEPTION);
            if (locdecl==NULL)
              { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); }
            add_name(CI, CI->curr_name, LC_HANDLER); // invalidates locdecl!!
            locdecl=find_name_in_scopes(CI, CI->curr_name, level, LC_EXCEPTION); // re-find
            if (locdecl->exc.sqlstate[0])  // there is a sqlstate assotiated with the exception
            { if (rt==HND_CONTINUE || rt==HND_EXIT)
              { int error_code;
                if (!sqlstate_to_wb_error(locdecl->exc.sqlstate, &error_code) || !may_have_handler(error_code, 0))
                  c_error(CANNOT_HANDLE_THE_ERROR);
              }
              t_idname name;  *name=SQLSTATE_EXCEPTION_MARK;
              strcpy(name+1, locdecl->exc.sqlstate);
              add_name(CI, name, LC_HANDLER);
            }
            next_sym(CI);
          }
        } while (CI->cursym==',');
       // handler action:
        t_locdecl * handler_decl = locdecls.acc0(locdecls.count()-1);
        sql_statement ** pstmt = &handler_decl->handler.stmt;
        handler_decl->handler.container_objnum = container_objnum;
        compile_sql_statement(CI, pstmt);
        store_statement(rt, *pstmt);
      }
      else // ident must follow
      { if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
        store_pos();
        t_idname name1;  strmaxcpy(name1, CI->curr_name, sizeof(t_idname));
        next_sym(CI);
        if (CI->cursym==SQ_CONDITION) // condition declaration
        { t_locdecl * locdecl = add_name(CI, name1, LC_EXCEPTION);
          if (next_sym(CI)==SQ_FOR)
          { next_sym(CI);
            test_and_next(CI, SQ_SQLSTATE, SQLSTATE_EXPECTED);
            if (CI->is_ident("VALUE")) next_sym(CI);
            if (CI->cursym!=S_STRING) c_error(STRING_EXPECTED);
            if (strlen(CI->curr_string()) != 5) c_error(SQLSTATE_FORMAT);
            memcpy(locdecl->exc.sqlstate, CI->curr_string(), sizeof(locdecl->exc.sqlstate));
            next_sym(CI);
          }
          else locdecl->exc.sqlstate[0]=0;
        }
        else if (CI->cursym==SQ_CURSOR || // cursor declaration
            CI->cursym==S_IDENT &&
            (!strcmp(CI->curr_name, "SCROLL") ||      // Intermadiate
             !strcmp(CI->curr_name, "LOCKED") ||      // private
             !strcmp(CI->curr_name, "INSENSITIVE") || // Full
             !strcmp(CI->curr_name, "SENSITIVE")))    // SQL3
        { if (!routines_allowed) c_error(CANNOT_ON_GLOBAL_LEVEL);
          t_locdecl * locdecl = add_name(CI, name1, LC_CURSOR);
          sql_view(CI, &locdecl->cursor.qe);  // locdecl owns the qe
          locdecl->cursor.opt=optimize_qe(CI->cdp, locdecl->cursor.qe);
          if (locdecl->cursor.opt==NULL) CI->propagate_error();
         // allocate local space for open cursor number:
          locdecl->cursor.for_cursor=FALSE;
          locdecl->cursor.offset=extent;
          extent+=sizeof(tcursnum);
        }
        else // comma or type name: variable declaration
        { add_name(CI, name1, LC_VAR);
          while (CI->cursym==',')
          { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
            add_name(CI, CI->curr_name, LC_VAR);
            next_sym(CI);
          }
         // CONSTANT or UPDATABLE:
          BOOL is_constant = FALSE;
          if (CI->cursym==SQ_CONSTANT) { is_constant=TRUE;  next_sym(CI); }
          else if (CI->cursym==SQ_UPDATABLE) next_sym(CI);
         // type:
          int tp;  t_specif specif;
          t_express ** pdefval = &locdecls.acc0(locdecls.count()-1)->var.defval;
          *pdefval=NULL;
          analyse_variable_type(CI, tp, specif, pdefval);
          if (CI->cursym==S_DEFAULT)
          { if (*pdefval!=NULL) { delete *pdefval;  *pdefval=NULL; }  // DEFAULT specified in the domain too
            next_sym(CI);
            sqlbin_expression(CI, pdefval, FALSE);
            convert_value(CI, *pdefval, tp, specif);
          }
          else if (is_constant) c_error(CONSTANT_MUST_HAVE_VAL);
          store_type(tp, specif, is_constant, *pdefval);
        }
      }
    } // DECLARE ...
    else break;
    test_and_next(CI, ';', SEMICOLON_EXPECTED);
  } while (TRUE);
  CI->sql_kont->atomic_nesting=upper_an;

 // compile assignment of default values of variables to p_init_stmt:
  for (int i=0;  i<locdecls.count();  i++)
  { t_locdecl * locdecl = locdecls.acc0(i);
    if (locdecl->loccat==LC_CONST || locdecl->loccat==LC_VAR)
      if (locdecl->var.defval!=NULL)
      { sql_stmt_set * st = new sql_stmt_set(locdecl->var.defval);  if (!st) c_error(OUT_OF_KERNEL_MEMORY);
        st->source_line = CI->prev_line;
        *p_init_stmt=st;
        p_init_stmt=&st->next_statement;
        st->assgn.dest=new t_expr_var (locdecl->var.type, locdecl->var.specif, own_level, locdecl->var.offset);
        if (!st->assgn.dest) c_error(OUT_OF_KERNEL_MEMORY);
      }
  }
}

void WINAPI compile_declarations_entry(CIp_t CI)
// Compiles GLOBAL declarations. CI->univ_ptr must point to t_global_scope.
{ t_global_scope * gssc = (t_global_scope*)CI->univ_ptr;
  //gssc->activate(CI);  -- if activated, references to this scope will have level==0, must have GLOBAL_DECLS_LEVEL
  t_global_scope * saved_scope = CI->cdp->global_sscope;
  CI->cdp->global_sscope = gssc; // this is the proper way of making the scope accessible
  gssc->compile_declarations(CI, GLOBAL_DECLS_LEVEL, &gssc->init_stmt, FALSE, FALSE, gssc->objnum);
  //gssc->deactivate(CI);
  CI->cdp->global_sscope = saved_scope;
  if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);
}

t_global_scope * create_global_sscope(cdp_t cdp, const WBUUID schema_uuid)
// Create global static scope for the schema and adds it to the list of installed global scopes
{ if (null_uuid(schema_uuid)) return NULL;  // is faster
 // creating the global scope: 
  t_global_scope * ssc = new t_global_scope(schema_uuid);
  if (!ssc) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  tobjnum objnum = find2_object(cdp, CATEG_PROC, schema_uuid, MODULE_GLOBAL_DECLS);
  if (objnum!=NOOBJECT)
  { tptr src=ker_load_objdef(cdp, objtab_descr, objnum);
    if (!src) { delete ssc;  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
    compil_info xCI(cdp, src, compile_declarations_entry);
    t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  xCI.univ_ptr=ssc;
    int err=compile_masked(&xCI);  // no error if not compiled, otherwise would block the schema!
    corefree(src);
    if (err) 
    { if (!(cdp->ker_flags & KFL_DISABLE_REFINT)) // disables error messages when importing/deleting a schema
      { char buf[81];
        trace_msg_general(cdp, TRACE_BCK_OBJ_ERROR, form_message(buf, sizeof(buf), msg_global_module_decls), NOOBJECT);
      }
      delete ssc;  return NULL;
    }  
  }
  ssc->objnum=objnum;
 // adding to the shared list:
  { ProtectedByCriticalSection cs(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
    t_global_scope * gs = installed_global_scopes;
    while (gs!=NULL && memcmp(schema_uuid, gs->schema_uuid, UUID_SIZE))
      gs=gs->next_gscope;
    if (gs) // created by another thread, use it and delete the own
      { delete ssc;  ssc=gs; }
    else
      { ssc->next_gscope=installed_global_scopes;  installed_global_scopes=ssc; }
  }
  return ssc;
}

t_rscope * create_global_rscope(cdp_t cdp)
// Create global dynamic scope for the selected global static scope and adds it to the list of global scopes
{ t_rscope * rscope = new(cdp->global_sscope->extent) t_rscope(cdp->global_sscope);
  if (rscope==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  memset(rscope->data, 0, cdp->global_sscope->extent);  // CLOB and BLOB vars must be init.
 // initialize local cursors as closed and LOBs as empty:
  rscope_init(rscope);
  rscope->level=GLOBAL_DECLS_LEVEL;  // must be after rscope_init which sets level to 0
 // add it to the list:
  rscope->next_rscope=cdp->created_global_rscopes;  cdp->created_global_rscopes=rscope;
 // execute init statements (must be after adding rscope to the list):
  cdp->global_rscope=rscope;  // execution needs it
#if 0
  if (cdp->admin_role!=NOOBJECT)
  { t_temp_account tacc(cdp, cdp->admin_role, CATEG_ROLE);  // execute with role ADMIN privils, called always in the context of the proper application 
    exec_stmt_list(cdp, cdp->global_sscope->init_stmt);
  }
  else
    exec_stmt_list(cdp, cdp->global_sscope->init_stmt);
#else
  cdp->in_admin_mode++;
  exec_stmt_list(cdp, cdp->global_sscope->init_stmt);
  cdp->in_admin_mode--;
#endif
 // report background error, if not initialized!
  if (cdp->is_an_error())
  { char buf[100];  
    form_message(buf, sizeof(buf), global_scope_init_error, cdp->get_return_code());
    trace_msg_general(cdp, TRACE_BCK_OBJ_ERROR, buf, NOOBJECT);
  }
  return rscope;
}

void mark_global_sscopes(void)
{ t_global_scope * scope = installed_global_scopes;
  while (scope!=NULL)
  { scope->mark_core();  // does not mark the init statements
    sql_statement * stmt = scope->init_stmt;
    while (stmt)
      { stmt->mark_core();  stmt=stmt->next_statement; }
    mark(scope);
    scope=scope->next_gscope;
  }
}

void refresh_global_scopes(cdp_t cdp)
{// 1. delete rscopes depending on sscopes
  { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
    for (int i=0;  i<=max_task_index;  i++)   // system process may have it too!
      if (cds[i]) cds[i]->drop_global_scopes();
  }
 // 2. delete global sscopes:
  while (installed_global_scopes!=NULL)
  { t_global_scope * scope = installed_global_scopes;
    installed_global_scopes=scope->next_gscope;
    delete scope;
  }
 // 3. restore global scopes for clients:
  { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
    for (int i=1;  i<=max_task_index;  i++)   // system process supposed not to have a context
      if (cds[i]) 
      { cdp_t cdp = cds[i];
        cdp->except_type=EXC_NO;  cdp->except_name[0]=0;  cdp->processed_except_name[0]=0;
        cdp->global_sscope=find_global_sscope(cdp, cdp->current_schema_uuid);
        cdp->global_rscope=find_global_rscope(cdp);
      }
  }
}

bool add_cursor_structure_to_scope(cdp_t cdp, CIp_t CI, t_scope & scope, db_kontext * kont, const char * name)
{ t_locdecl * locdecl = scope.add_name(CI, name, LC_VAR);
  locdecl->var.type  =0;
  locdecl->var.specif=0;
  locdecl->var.offset=scope.extent;
  locdecl->var.defval=NULL;
  t_struct_type * row = new t_struct_type;  if (!row) return true;
  locdecl->var.stype=row;
  locdecl->var.structured_type=true;
  if (row->fill_from_kontext(kont)) return true;
  scope.extent+=row->valsize;
  return false;
}

t_scope * make_scope_from_cursor(cdp_t cdp, cur_descr * cd)
{ t_scope * scope = new t_scope(FALSE);
  if (!scope) return NULL;
 // I do not have CI so I must prevent error in add_cursor_structure_to_scope.
 // Decl. twice is not possible. Preventing allocation error by allocating here.
  if (!scope->locdecls.acc(0)) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  scope->locdecls.delet(0);
  if (add_cursor_structure_to_scope(cdp, NULL, *scope, &cd->qe->kont, NULLSTRING))
    { delete scope;  return NULL; }
  return scope;
}

/////////////////////////// routine invocation ///////////////////////////////
void sql_stmt_call::routine_invocation(CIp_t CI, t_routine * routine, t_routine_ownership routine_ownership)
// Current symbol if the following the routine name, locdecl is checked to be routine.
{ t_scope fict_scope(TRUE);
  if (CI->sql_kont->from_debugger) c_error(CANNOT_IN_DEBUGGER);
 // set members:
  type  =origtype=routine->rettype;
  specif=origspecif=routine->retspecif;
  if (routine_ownership==ROUT_GLOBAL_SHARED) 
  { temp_owned_routine=routine;
    objnum=routine->objnum;
    memcpy(callee_version, routine->version, sizeof(callee_version));
  }
  else // ROUT_LOCAL or ROUT_EXTENSION
  { called_local_routine=routine;
    if (routine_ownership==ROUT_EXTENSION) 
      temp_owned_routine=routine;
  }
 // analyse parameters:
  const t_scope * pscope = routine->param_scope();
//if (CI->cursym!='(') c_error(LEFT_PARENT_EXPECTED); // in SQL () is mandatory
  if (CI->cursym=='(')  // ODBC requires procedure call without () too
  { fict_scope.activate(CI);  // scope of parameters, active when executing
    int parnum = routine->extension_routine ? 1 : 0;  
    BOOL keypars=FALSE;
    do // ( or , is the current symbol
    { next_sym(CI);
      if (CI->cursym!=')' && CI->cursym!=',') // unless empty list or skipped parameter
      {// find the next formal param description, check for the keyword parameter:
        t_locdecl * param;
        param=NULL;
        if (CI->cursym==S_IDENT)
        { param=pscope->find_name(CI->curr_name, LC_VAR);
          if (param!=NULL)
          { char c1, c2;  const char * p = CI->compil_ptr;
            if (CI->curchar==' ')
              { while (*p==' ') p++;  c1=*p;  c2=p[1]; }
            else { c1=CI->curchar;  c2=*p; }
            if (c1=='=' && c2=='>' || c1=='{' || c1=='/' && c2=='/')
            { next_sym(CI);  next_sym(CI);  next_sym(CI);
              //test_and_next(CI, '=', EQUAL_EXPECTED);
              keypars=TRUE;  // end of positional params
              parnum=(int)(param-pscope->locdecls.acc0(0));
            }
            else param=NULL;
          }
        }
        if (param==NULL) // not a keyword parameter
        { if (keypars) c_error(KEYWORD_PARAM_EXPECTED);
          if (parnum >= pscope->locdecls.count()) c_error(TOO_MANY_PARAMS);
          param=pscope->locdecls.acc0(parnum);
          if (param->loccat==LC_LABEL) c_error(TOO_MANY_PARAMS);
        }
       // register the actual parameter:
        t_express ** pactpar = actpars.acc(parnum);
        if (pactpar==NULL) c_error(OUT_OF_MEMORY);
        if (*pactpar!=NULL) c_error(DUPLICATE_ACTUAL_PARAM);
        if (param->loccat==LC_INPAR || param->loccat==LC_INPAR_ || param->loccat==LC_PAR)
        // LC_PAR added here when CALL stared to be compiled without having compiled the body before
        { sqlbin_expression(CI, pactpar, FALSE);
          if ((*pactpar)->sqe==SQE_DYNPAR)
            supply_marker_type(CI, (t_expr_dynpar*)(*pactpar), param->var.type, param->var.specif, MODE_IN);
          else
            convert_value(CI, *pactpar, param->var.type, param->var.specif);
        }
        else // OUT or INOUT parameter
        { sql_selector(CI, pactpar, false, false);
          if ((*pactpar)->sqe==SQE_DYNPAR)
            supply_marker_type(CI, (t_expr_dynpar*)(*pactpar), param->var.type, param->var.specif,
                param->loccat==LC_OUTPAR || param->loccat==LC_OUTPAR_ ? MODE_OUT : MODE_INOUT);
          else // allow different legth in INOUT strings & texts
            if ((*pactpar)->type != param->var.type)
              if (!is_string((*pactpar)->type) && (*pactpar)->type!=ATT_TEXT || 
                  !is_string( param->var.type) &&  param->var.type!=ATT_TEXT)
                c_error(INCOMPATIBLE_TYPES);
        }
      }
      parnum++;
    } while (CI->cursym==',');
    test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
    fict_scope.deactivate(CI);
  }
  if (routine_ownership==ROUT_GLOBAL_SHARED)
  { temp_owned_routine=NULL;
    psm_cache.put(routine);
  }
}

sql_stmt_call::~sql_stmt_call(void)
{ for (int i=0;  i<actpars.count();  i++)
    delete *actpars.acc0(i);
  if (temp_owned_routine)  // cannot be local
    if (temp_owned_routine->extension_routine)
      delete temp_owned_routine;  // extension routine owned by the invocation
    else  // shared routine, error in compilation occured
      psm_cache.put(temp_owned_routine); 
  if (std_routine) delete std_routine;
}

/////////////////////// analyse the list of tranasction modes /////////////////
static void transaction_modes(CIp_t CI, BOOL & rw, t_isolation & isolation)
// 1st mode symbol is current
{ BOOL isol_specif=FALSE, rw_specif=FALSE;
  if (CI->cursym==S_IDENT)
    do
    { if (!strcmp(CI->curr_name, "READ"))
      { if (rw_specif) c_error(DUPLICATE_SPECIFICATION);
        next_and_test(CI, S_IDENT, GENERAL_SQL_SYNTAX);
        if      (!strcmp(CI->curr_name, "WRITE")) rw=TRUE;
        else if (!strcmp(CI->curr_name, "ONLY" )) rw=FALSE;
        else c_error(GENERAL_SQL_SYNTAX);
        rw_specif=TRUE;
      }
      else if (!strcmp(CI->curr_name, "ISOLATION"))
      { if (isol_specif) c_error(DUPLICATE_SPECIFICATION);
        next_and_test(CI, S_IDENT, GENERAL_SQL_SYNTAX);
        if (strcmp(CI->curr_name, "LEVEL")) c_error(GENERAL_SQL_SYNTAX);
        next_and_test(CI, S_IDENT, GENERAL_SQL_SYNTAX);
        if (!strcmp(CI->curr_name, "READ"))
        { next_and_test(CI, S_IDENT, GENERAL_SQL_SYNTAX);
          if      (!strcmp(CI->curr_name, "UNCOMMITTED")) isolation=READ_UNCOMMITTED;
          else if (!strcmp(CI->curr_name, "COMMITTED" ))  isolation=READ_COMMITTED;
          else c_error(GENERAL_SQL_SYNTAX);
        }
        else if (!strcmp(CI->curr_name, "REPEATABLE"))
        { next_and_test(CI, S_IDENT, GENERAL_SQL_SYNTAX);
          if (strcmp(CI->curr_name, "READ")) c_error(GENERAL_SQL_SYNTAX);
          isolation=REPEATABLE_READ;
        }
        else if (!strcmp(CI->curr_name, "SERIALIZABLE"))
          isolation=SERIALIZABLE;
        else c_error(GENERAL_SQL_SYNTAX);
        isol_specif=TRUE;
      }
      else c_error(GENERAL_SQL_SYNTAX);
      if (next_sym(CI)!=',') break;
      next_and_test(CI, S_IDENT, GENERAL_SQL_SYNTAX);
    } while (TRUE);
 // check the specifications, add implicit values:
  if (!isol_specif) isolation = SERIALIZABLE;  // implicit
  if (!rw_specif)   rw        = isolation!=READ_UNCOMMITTED;
  if (rw && isolation==READ_UNCOMMITTED) c_error(SPECIF_CONFLICT);
}

//////////////////////////////////// compile UPDATE statement //////////////////////////////////////
void sql_stmt_update::compile(CIp_t CI)
{ next_sym(CI);
  source_line = CI->prev_line;
  tobjname tabname, schemaname;  tcateg categ;  const t_locdecl * locdecl = NULL;
  if (CI->cursym==S_IDENT) // find the table or cursor (ignored if CURRENT OF):
    if (!get_table_name(CI, tabname, schemaname, &objnum, &locdecl, &locdecl_level))
    { categ=CATEG_TABLE;
      if (locdecl!=NULL) locdecl_offset=locdecl->table.offset;
    }
    else if (!name_find2_obj(CI->cdp, tabname, schemaname, CATEG_CURSOR, &objnum))
      categ=CATEG_CURSOR;
    else { SET_ERR_MSG(CI->cdp, tabname);  c_error(TABLE_DOES_NOT_EXIST); }
  else tabname[0]=0;
  if (CI->cursym!=S_SET) c_error(SET_EXPECTED);
 // skip the SET clause and analyse the WHERE clause:
  CIpos pos(CI);     // save compil. position
  int par_lev=0;
  while (CI->cursym && CI->cursym!=';' && (CI->cursym!=S_WHERE || par_lev))
  { if      (CI->cursym==')') par_lev--;
    else if (CI->cursym=='(') par_lev++;
    next_sym(CI);
  }
 // compile optional WHERE clause and prepare cursor:
  qe=compile_condition(CI, objnum, tabname, schemaname, categ, current_cursor_name, &opt);
 // check the cursor:
  if (qe!=NULL)
    if (!(qe->qe_oper & QE_IS_UPDATABLE))
      c_error(CANNOT_UPDATE_IN_THIS_VIEW);
  pos.restore(CI);   // restore the saved position

 // prepare kontext for assignments:
  db_kontext *pkont;  bool translate_kontext;
  if (*current_cursor_name) // positioned UPDATE ... WHERE CURRENT OF
  { cur_descr * cd;  int level;
   // looking for the cursor, must check scopes first (FOR cursors), before open or global cursors!
    t_locdecl * locdecl = find_name_in_scopes(CI, current_cursor_name, level, LC_CURSOR);
    if (locdecl != NULL) // locally declared cursor
    { if (!(locdecl->cursor.qe->qe_oper & QE_IS_UPDATABLE))
        c_error(CANNOT_UPDATE_IN_THIS_VIEW);
      pkont=&locdecl->cursor.qe->kont;
    }
   // cursor not in the scope, try open cursors
    else if (find_named_cursor(CI->cdp, current_cursor_name, cd) != NOCURSOR)
      pkont=&cd->qe->kont;
    else // cursor not open, try global cursor
    { tobjnum objnum;
      if (name_find2_obj(CI->cdp, current_cursor_name, NULL, CATEG_CURSOR, &objnum))
        { SET_ERR_MSG(CI->cdp, current_cursor_name);  c_error(TABLE_DOES_NOT_EXIST); }
      tptr src=ker_load_objdef(CI->cdp, objtab_descr, objnum);
      if (src==NULL) c_error(OUT_OF_MEMORY);
      t_optim * aux_opt;
      if (compile_query(CI->cdp, src, CI->sql_kont, &aux_qe, &aux_opt))
        { corefree(src);  CI->propagate_error(); } // propagates compilation error
      delete aux_opt;  corefree(src);
      pkont=&aux_qe->kont;
    }
    translate_kontext=true;
  }
  else if (qe==NULL) // full table operation
  { if (locdecl_offset!=-1) // local table
    { const table_all * ta = locdecl->table.ta;
      tabkont.fill_from_ta(CI->cdp, ta);
    }
    else  // permanent table
    { table_descr_auto td(CI->cdp, objnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
      if (td->me()==NULL) CI->propagate_error();  // propagates compilation error
      tabkont.fill_from_td(CI->cdp, td->me());
    }
    pkont=&tabkont;
    translate_kontext=false;
  }
  else
  { pkont=&qe->kont;
    translate_kontext=true;
  }
  temp_sql_kont_add tk(CI->sql_kont, pkont); // aktivuje kontext pro preklad prirazeni

 // compile assignments:
  if (!iuts.start_columns(pkont->elems.count())) c_error(OUT_OF_KERNEL_MEMORY);
  do
  { next_sym(CI);   /* skips SET or comma */
   // left side:
    t_express * dex, * sex;
    sql_selector(CI, &dex, false, false);
    if (dex->sqe!=SQE_ATTR)
      { delete dex;  SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(BAD_TARGET); }
    if (!iuts.register_column(((t_expr_attr*)dex)->elemnum, translate_kontext ? pkont : NULL, ((t_expr_attr*)dex)->indexexpr))
      { delete dex;  SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(BAD_TARGET); }
    ((t_expr_attr*)dex)->indexexpr = NULL;  // unlinking
    int dest_type = dex->type;  t_specif dest_specif = dex->specif;
    delete dex;  
    test_and_next(CI, '=', EQUAL_EXPECTED);
   // right side (no need to check fo NULL, this variant is contained in sql_expression):
    if (CI->cursym==S_DEFAULT)
      { sex=NULL;  next_sym(CI); }
    else
    { sqlbin_expression(CI, &sex, FALSE);
      if (sex->sqe==SQE_DYNPAR)
        supply_marker_type(CI, (t_expr_dynpar*)sex, dest_type, dest_specif, MODE_IN);
      else
        convert_value(CI, sex, dest_type, dest_specif);
    }
    iuts.add_expr(sex);
  } while (CI->cursym==',');
  if (CI->cursym!=S_WHERE && CI->cursym!=';' && CI->cursym)
    c_error(GARBAGE_AFTER_END);
  iuts.stop_columns();

 // skip the WHERE clause:
  while (CI->cursym && CI->cursym!=';') next_sym(CI);
}
//////////////////////////////// compile INSERT statement ////////////////////////////////
void sql_stmt_insert::compile(CIp_t CI)
{ next_sym(CI);
  source_line = CI->prev_line;
  test_and_next(CI, S_INTO, INTO_EXPECTED);

 // analyse destination table/cursor, pkont:=kontext for searching the column names:
 // defines tabnum, td
  int i;  db_kontext *pkont;  
  { tobjname tabname, schemaname;  tobjnum query_obj;  const t_locdecl * locdecl;
    if (!get_table_name(CI, tabname, schemaname, &objnum, &locdecl, &locdecl_level))
    { if (locdecl!=NULL)
      { const table_all * ta = locdecl->table.ta;
        tabkont.fill_from_ta(CI->cdp, ta);
        locdecl_offset=locdecl->table.offset;
      }
      else
      { table_descr_auto td(CI->cdp, objnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
        if (td->me()==NULL) CI->propagate_error();  // propagates compilation error
       // create local kontext to the table:
        tabkont.fill_from_td(CI->cdp, td->me());
      }
      pkont=&tabkont;
    }
    else if (!name_find2_obj(CI->cdp, tabname, schemaname, CATEG_CURSOR, &query_obj))
    { tptr src=ker_load_objdef(CI->cdp, objtab_descr, query_obj);
     // compile the cursor (may be restricted by WHERE):
      if (src==NULL) c_error(OUT_OF_MEMORY);
      if (compile_query(CI->cdp, src, CI->sql_kont, &qe, &opt))
        { corefree(src);  CI->propagate_error(); } // propagates compilation error
      corefree(src);
      if ((qe->qe_oper & (QE_IS_UPDATABLE|QE_IS_INSERTABLE)) != (QE_IS_UPDATABLE|QE_IS_INSERTABLE))
        c_error(CANNOT_INSERT_INTO_THIS_VIEW);
      pkont=&qe->kont;
     // find the underlying table, store it to objnum:
      t_query_expr * aqe = qe;
      while (aqe->qe_type==QE_SPECIF && ((t_qe_specif*)aqe)->grouping_type==GRP_NO)
        aqe=((t_qe_specif*)aqe)->from_tables;
      if (aqe->qe_type==QE_TABLE) objnum=((t_qe_table*)aqe)->tabnum;
      else c_error(CANNOT_INSERT_INTO_THIS_VIEW);
    }
    else
      { SET_ERR_MSG(CI->cdp, tabname);  c_error(TABLE_DOES_NOT_EXIST); }
  }

 // create list of columns, store column numbers related to [pkont] in [orig_colnums]:
  t_colnum_dynar orig_colnums;  /* column numbers in the cursor, not in the table */
  int column_count;
  if (CI->cursym=='(')   // read specified column names
  { column_count=0;
    do
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);  /* skip '(' or ',' */
     // find ident in the kontext:
      tobjname name;  elem_descr * el;  int elemnum;
      strcpy(name, CI->curr_name);
      if (next_sym(CI)=='.')
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        tobjname name2;  strcpy(name2, CI->curr_name);
        next_sym(CI);
        el=pkont->find(name, name2, elemnum);
        if (el==NULL) { SET_ERR_MSG(CI->cdp, name2);  c_error(ATTRIBUTE_NOT_FOUND); }
      }
      else
      { el=pkont->find(NULL, name,  elemnum);
        if (el==NULL) { SET_ERR_MSG(CI->cdp, name);   c_error(ATTRIBUTE_NOT_FOUND); }
      }
     // save column number:  
      tattrib * pcolnum = orig_colnums.next();
      if (pcolnum==NULL) c_error(OUT_OF_MEMORY);
      *pcolnum=(tattrib)elemnum;
      column_count++;
    } while (CI->cursym == ',');
    test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
  }
  else // columns not specified, take all columns
  { column_count=pkont->elems.count();
   // skip system columns, fill the rest: 
    int col=pkont->first_nonsys_col();
    column_count-=col;
    if (orig_colnums.acc(column_count-1) == NULL) c_error(OUT_OF_MEMORY);
    for (i=0;  i<column_count;  i++) *orig_colnums.acc0(i)=col++;
  }

 // process list of columns:
  if (!iuts.start_columns(pkont->elems.count())) c_error(OUT_OF_KERNEL_MEMORY);
 // compile the source values:
  if (CI->cursym==S_DEFAULT)
  { next_and_test(CI, S_VALUES, VALUES_EXPECTED);
    next_sym(CI);
    // list of columns will be empty
  }
  else if (CI->cursym==S_SELECT) /* analyse the query: */
  { top_query_expression(CI, &src_qe);
    src_opt=optimize_qe(CI->cdp, src_qe);
    if (src_opt==NULL) CI->propagate_error();
   // skip system columns in the source if the destination does not contain (explicitly specified) system columns:
    int idst=0, isrc=1;
    if (!IS_SYSTEM_COLUMN(pkont->elems.acc0(*orig_colnums.acc0(0))->name))
      isrc=src_qe->kont.first_nonsys_col();
    if (src_qe->kont.elems.count()-isrc > column_count) c_error(TOO_MANY_INSERT_VALUES);
   // add the source operands to the assignments:
    while (isrc<src_qe->kont.elems.count())
    {// get the information about the destination column: 
      int dest_column = *orig_colnums.acc0(idst);
      elem_descr * el = pkont->elems.acc0(dest_column);
      if (!iuts.register_column(dest_column, qe ? pkont : NULL), NULL)
        { SET_ERR_MSG(CI->cdp, el->name);  c_error(BAD_TARGET); }
     // create source value expression accessing a column in the source query and convert its value:
      t_express * sex = new t_expr_attr(isrc, &src_qe->kont);  if (!sex) c_error(OUT_OF_KERNEL_MEMORY);
      convert_value(CI, sex, el->type, el->specif);
      iuts.add_expr(sex);
      isrc++;  idst++;
    }
  }
  else  /* row of values: */
  { test_and_next(CI, S_VALUES, VALUES_EXPECTED);
    if (CI->cursym!='(') c_error(LEFT_PARENT_EXPECTED);
   // compile vector of expressions:
    i=0;  
    do
    { next_sym(CI);  // skips ( or ,
      if (i>=column_count) c_error(TOO_MANY_INSERT_VALUES);
     // get the information about the destination column: 
      int dest_column = *orig_colnums.acc0(i);
      elem_descr * el = pkont->elems.acc0(dest_column);
      if (!iuts.register_column(dest_column, qe ? pkont : NULL), NULL)
        { SET_ERR_MSG(CI->cdp, el->name);  c_error(BAD_TARGET); }
     // create expression for the specied value and convert its type:
      t_express * sex = NULL;
      sqlbin_expression(CI, &sex, FALSE);
      if (sex->sqe==SQE_DYNPAR)
        supply_marker_type(CI, (t_expr_dynpar*)sex, el->type, el->specif, MODE_IN);
      else
        convert_value(CI, sex, el->type, el->specif);
      iuts.add_expr(sex);
      i++;
    } while (CI->cursym == ',');
    test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
  }
  iuts.stop_columns();
}

//////////////////////////////// compile DELETE statement ////////////////////////////////
void sql_stmt_delete::compile(CIp_t CI)
{ next_sym(CI);
  source_line = CI->prev_line;
  tobjname tabname, schemaname;  tcateg categ;  const t_locdecl * locdecl;
  if (CI->cursym==S_FROM) // find the table or cursor (ignored if CURRENT OF):
  { next_sym(CI);
    if (!get_table_name(CI, tabname, schemaname, &objnum, &locdecl, &locdecl_level))
    { categ=CATEG_TABLE;
      if (locdecl!=NULL) locdecl_offset=locdecl->table.offset;
    }
    else if (!name_find2_obj(CI->cdp, tabname, schemaname, CATEG_CURSOR, &objnum))
      categ=CATEG_CURSOR;
    else { SET_ERR_MSG(CI->cdp, tabname);  c_error(TABLE_DOES_NOT_EXIST); }
  }
  else tabname[0]=0;
 // compile optional WHERE clause and prepare cursor:
  qe=compile_condition(CI, objnum, tabname, schemaname, categ, current_cursor_name, &opt);
 // check the cursor:
  if (qe!=NULL)
    if (!(qe->qe_oper & QE_IS_DELETABLE))
      c_error(CANNOT_DELETE_IN_THIS_VIEW);
 // check the name of the current cursor:
  if (*current_cursor_name) // positioned DELETE ... WHERE CURRENT OF
  { cur_descr * cd;  tobjnum objnum;  int level;
   // looking for the cursor, must check scopes first (FOR cursors), before open or global cursors!
    t_locdecl * locdecl = find_name_in_scopes(CI, current_cursor_name, level, LC_CURSOR);
    if (locdecl != NULL) // locally declared cursor
    { if (!(locdecl->cursor.qe->qe_oper & QE_IS_DELETABLE))
        c_error(CANNOT_DELETE_IN_THIS_VIEW);
    }
    else
      if (find_named_cursor(CI->cdp, current_cursor_name, cd) == NOCURSOR)
        if (name_find2_obj(CI->cdp, current_cursor_name, NULL, CATEG_CURSOR, &objnum))
          { SET_ERR_MSG(CI->cdp, current_cursor_name);  c_error(TABLE_DOES_NOT_EXIST); }
  }
}
//////////////////////////////////////////////////////////////////////////////
static tptr save_sql_source(const char * start, const char * stop)
// Creates own copy of the non-null-terminated string from start to stop.
// Returns NULL iff no memory.
{ int len=(int)(stop-start);
  tptr s = (tptr)corealloc(len+1, 96);
  if (s!=NULL)
    { memcpy(s, start, len);  s[len]=0; }
  return s;
}

#ifdef STOP // not used
int WINAPI typesize(elem_descr * el)
{ attribdef atr;  atr.attrtype=el->type;  atr.attrspecif=el->specif;
  return TYPESIZE((&atr)); 
}
#endif

static bool get_cascade_restrict(CIp_t CI)
{ if (CI->cursym==S_CASCADE) { next_sym(CI);  return true; }
  if (CI->cursym==S_RESTRICT) next_sym(CI);  // default
  return false;
}

static tcateg analyse_category(CIp_t CI)
{ test_and_next(CI, SQ_IN, IN_EXPECTED);
  if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
  if (!strcmp(CI->curr_name, "CATEG_VIEW"))     return CATEG_VIEW;      // not used any more
  if (!strcmp(CI->curr_name, "CATEG_PGMSRC"))   return CATEG_PGMSRC;
  if (!strcmp(CI->curr_name, "CATEG_MENU"))     return CATEG_MENU;      // not used any more
  if (!strcmp(CI->curr_name, "CATEG_RELATION")) return CATEG_RELATION;  // not used any more
  if (!strcmp(CI->curr_name, "CATEG_DRAWING"))  return CATEG_DRAWING;   // not used any more
  if (!strcmp(CI->curr_name, "CATEG_WWW"))      return CATEG_WWW;       // not used any more
  if (!strcmp(CI->curr_name, "CATEG_XMLFORM"))  return CATEG_XMLFORM;
  if (!strcmp(CI->curr_name, "CATEG_STSH"))     return CATEG_STSH;
  SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(BAD_IDENT_CLASS);
}  
/////////////////////// compile statement ////////////////////////////////////
void compile_sql_statement(CIp_t CI, sql_statement ** pso)
// Compiles single statement. For empty statements returns sql_stmt_empty.
{ int general_flag;
  const char * source_start=CI->prepos;

  t_idname label_name;  label_name[0]=0;
  if (CI->cursym==S_IDENT)  // look for the following colon
  { 
#ifdef STOP   // better way found
    const char * p = CI->compil_ptr;
    while (*p==' ' || *p=='\r' || *p=='\n') p++;
    if (CI->curchar==':' || CI->curchar==' ' && *p==':')
#endif
    while (CI->curchar==' ' || CI->curchar=='\r' || CI->curchar=='\n') next_char(CI);
    if (CI->curchar==':')
    { strmaxcpy(label_name, CI->curr_name, sizeof(label_name));
      next_and_test(CI, ':', COLON_EXPECTED);
      next_sym(CI);
      source_start=CI->prepos;  // update the position of the beginning of the statement
    }
  }

  switch (CI->cursym)
  { case S_CREATE:
    { bool conditional;
      if (next_sym(CI)==SQ_IF)
      { next_and_test(CI, SQ_NOT, NOT_EXPECTED);
        next_and_test(CI, S_EXISTS, EXISTS_EXPECTED);
        next_sym(CI);
        conditional=true;
      }
      else conditional=false;
      switch (CI->cursym)
      { case SQ_TABLE: // not storing the copy of the source - definition will be restored from [ta]
        { sql_stmt_create_table * so = new sql_stmt_create_table(conditional);  
          *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
         // compile table def to ta:
          so->ta=new table_all;  if (!so->ta) c_error(OUT_OF_KERNEL_MEMORY);
          table_def_comp(CI, so->ta, NULL, CREATE_TABLE);
          add_implicit_constraint_names(so->ta);
          break;
        }
        case S_UNIQUE:
        case S_INDEX:
        { sql_stmt_alter_table * so = new sql_stmt_alter_table(SQL_STAT_CREATE_INDEX, conditional);  *pso=so;
          if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->old_alter_table_syntax = false;
          { table_all ta_new, ta_old;
            table_def_comp(CI, &ta_new, &ta_old, conditional ? CREATE_INDEX_COND : CREATE_INDEX);
          }
         // save statement source:
          so->source=save_sql_source(source_start, CI->prepos);
          if (so->source==NULL) c_error(OUT_OF_MEMORY);
          break;
        }
        case S_VIEW:
        { sql_stmt_create_view * so = new sql_stmt_create_view(conditional);  
          *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          source_start=CI->prepos;  // position of "VIEW"
         // extract view name:
          CIpos pos(CI);     // save compil. position
          get_schema_and_name(CI, so->name, so->schema);
          pos.restore(CI);   // restore the saved position
          CI->cursym=S_VIEW;
          WBUUID schema_uuid;
          if (*so->schema)
          { if (ker_apl_name2id(CI->cdp, so->schema, schema_uuid, NULL))
              {  SET_ERR_MSG(CI->cdp, so->schema);  c_error(OBJECT_NOT_FOUND); }
          }
          else memcpy(schema_uuid, CI->cdp->current_schema_uuid, UUID_SIZE);
         // compile view (testing only):
          { t_short_term_schema_context stsc(CI->cdp, schema_uuid);
            sql_view(CI, &so->qe);
            delete so->qe;  so->qe=NULL;
          }
         // save view source:
          so->source=save_sql_source(source_start, CI->prepos);
          if (so->source==NULL) c_error(OUT_OF_MEMORY);
          break;
        }
        case SQ_PROCEDURE:
        case SQ_FUNCTION:
        { sql_stmt_create_alter_rout * so = new sql_stmt_create_alter_rout(true, conditional);   
          *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->compile(CI);
          break;
        }
        case SQ_TRIGGER:
        { sql_stmt_create_alter_trig * so = new sql_stmt_create_alter_trig(true, conditional);   
          *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->compile(CI);
          break;
        }
        case SQ_SEQUENCE:
        { sql_stmt_create_alter_seq  * so = new sql_stmt_create_alter_seq(true, conditional);   
          *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          CI->univ_ptr=&so->seq;
          so->seq.modifying=FALSE;  so->seq.recursive=FALSE;
          sequence_anal(CI); 
          break;
        }
        case SQ_DOMAIN:
        { sql_stmt_create_alter_dom * so = new sql_stmt_create_alter_dom(true, conditional);     
          *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->compile(CI);
          break;
        }
        case SQ_SCHEMA:
        { sql_stmt_create_schema * so = new sql_stmt_create_schema();               
          *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->conditional=conditional;
          so->source_line = CI->prev_line;
          next_and_test(CI, S_IDENT, IDENT_EXPECTED);
          strmaxcpy(so->schema_name, CI->curr_name, OBJ_NAME_LEN+1);
          next_sym(CI);
          break;
        }
        case S_USER:
          general_flag=CATEG_USER;
         cre_subj:
          { sql_stmt_create_drop_subject * so = new sql_stmt_create_drop_subject(TRUE);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->categ=general_flag;
            so->conditional=conditional;
            so->source_line = CI->prev_line;
            next_and_test(CI, S_IDENT, IDENT_EXPECTED);
            strmaxcpy(so->subject_name, CI->curr_name, sizeof(so->subject_name));
            next_sym(CI);
            if (general_flag==CATEG_USER)
              if (CI->is_ident("PASSWORD"))
              { next_sym(CI);
                if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
                int len = (int)strlen(CI->curr_string());
                so->password=(char*)corealloc(len+1, 77);
                if (!so->password) c_error(OUT_OF_KERNEL_MEMORY);
                strcpy(so->password, CI->curr_string());
                sys_Upcase(so->password);
                next_sym(CI);
              }
            break;
          }
        case S_GROUP:
          general_flag=CATEG_GROUP;
          goto cre_subj;
        case S_IDENT:
        { if (!strcmp("ROLE", CI->curr_name)) 
          { sql_stmt_create_drop_subject * so = new sql_stmt_create_drop_subject(TRUE);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->categ=CATEG_ROLE;
            so->conditional=conditional;
            so->source_line = CI->prev_line;
            get_schema_and_name(CI, so->subject_name, so->subject_schema);
          }
          else if (!strcmp("FULLTEXT", CI->curr_name)) 
          { sql_stmt_create_alter_fulltext * so = new sql_stmt_create_alter_fulltext(true, conditional);  
            *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->source_line = CI->prev_line;
            so->ftdef.modifying=false;
            so->ftdef.compile_intern(CI);
           // save view source:
            so->source=save_sql_source(source_start, CI->prepos);
            if (so->source==NULL) c_error(OUT_OF_MEMORY);
          }
          else if (!strcmp("FRONT_OBJECT", CI->curr_name)) 
          { sql_stmt_create_alter_front_object * so = new sql_stmt_create_alter_front_object(true);   
            *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->source_line = CI->prev_line;
            so->conditional=conditional;
            get_schema_and_name(CI, so->name, so->schema);
            so->categ=analyse_category(CI);
            next_and_test(CI, S_AS, AS_EXPECTED);
            next_and_test(CI, S_STRING, STRING_EXPECTED);
            so->definition=(tptr)corealloc(strlen(CI->curr_string())+1, 81);
            if (!so->definition) c_error(OUT_OF_KERNEL_MEMORY);
            strcpy(so->definition, CI->curr_string());
            next_sym(CI);
          }
          else c_error(NOT_AN_SQL_STATEMENT);
          break;
        }
        default: c_error(NOT_AN_SQL_STATEMENT);
      }
      break;
    }
    case S_DROP:
    { sql_stmt_drop * so;  bool conditional;
      if (next_sym(CI)==SQ_IF)
      { next_and_test(CI, S_EXISTS, EXISTS_EXPECTED);
        next_sym(CI);
        conditional=true;
      }
      else conditional=false;
      switch (CI->cursym)
      { case SQ_SCHEMA:
        { *pso = so = new sql_stmt_drop(CATEG_APPL);  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->conditional=conditional;
          so->source_line = CI->prev_line;
          next_and_test(CI, S_IDENT, IDENT_EXPECTED);
          strmaxcpy(so->schema, CI->curr_name, OBJ_NAME_LEN+1);
          next_sym(CI);
          so->cascade=get_cascade_restrict(CI);
          break;
        }
        case SQ_TABLE:
        { *pso = so = new sql_stmt_drop(CATEG_TABLE);    if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->conditional=conditional;
          so->source_line = CI->prev_line;
          next_sym(CI);
          get_table_name(CI, so->main, so->schema, NULL);
          so->cascade=get_cascade_restrict(CI);
          break;
        }
        case S_INDEX:
        { sql_stmt_alter_table * so = new sql_stmt_alter_table(SQL_STAT_DROP_INDEX, conditional);  *pso=so;
          if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->old_alter_table_syntax = false;
          { table_all ta_new, ta_old;
            table_def_comp(CI, &ta_new, &ta_old, conditional ? DROP_INDEX_COND : DROP_INDEX);
          }
         // save statement source:
          so->source=save_sql_source(source_start, CI->prepos);
          if (so->source==NULL) c_error(OUT_OF_MEMORY);
          break;
        }
        case S_VIEW:
          *pso = so = new sql_stmt_drop(CATEG_CURSOR);     if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->conditional=conditional;
          so->source_line = CI->prev_line;
          get_schema_and_name(CI, so->main, so->schema);
          so->cascade=get_cascade_restrict(CI);
          break;
        case SQ_PROCEDURE:  case SQ_FUNCTION:
          *pso = so = new sql_stmt_drop(CATEG_PROC);  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->conditional=conditional;
          so->source_line = CI->prev_line;
          get_schema_and_name(CI, so->main, so->schema);
          break;
        case SQ_TRIGGER:
          *pso = so = new sql_stmt_drop(CATEG_TRIGGER);  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->conditional=conditional;
          so->source_line = CI->prev_line;
          get_schema_and_name(CI, so->main, so->schema);
          break;
        case SQ_SEQUENCE:
          *pso = so = new sql_stmt_drop(CATEG_SEQ);      if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->conditional=conditional;
          so->source_line = CI->prev_line;
          get_schema_and_name(CI, so->main, so->schema);
          break;
        case SQ_DOMAIN:
          *pso = so = new sql_stmt_drop(CATEG_DOMAIN);   if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->conditional=conditional;
          so->source_line = CI->prev_line;
          get_schema_and_name(CI, so->main, so->schema);
          so->cascade=get_cascade_restrict(CI);
          break;
        case S_USER:
          general_flag=CATEG_USER;
         drop_subj:
          { sql_stmt_create_drop_subject * so = new sql_stmt_create_drop_subject(FALSE);  *pso=so;
            if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->conditional=conditional;
            so->categ=general_flag;
            so->source_line = CI->prev_line;
            next_and_test(CI, S_IDENT, IDENT_EXPECTED);
            strmaxcpy(so->subject_name, CI->curr_name, sizeof(so->subject_name));
            next_sym(CI);
            break;
          }
        case S_GROUP:
          general_flag=CATEG_GROUP;
          goto drop_subj;
        case S_IDENT:
          if (!strcmp("ROLE", CI->curr_name)) 
          { sql_stmt_create_drop_subject * so = new sql_stmt_create_drop_subject(FALSE);  *pso=so;
            if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->conditional=conditional;
            so->source_line = CI->prev_line;
            so->categ=CATEG_ROLE;
            get_schema_and_name(CI, so->subject_name, so->subject_schema);
          }
          else if (!strcmp("FULLTEXT", CI->curr_name)) 
          { *pso = so = new sql_stmt_drop(CATEG_INFO);     if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->conditional=conditional;
            so->source_line = CI->prev_line;
            get_schema_and_name(CI, so->main, so->schema);
            so->cascade=get_cascade_restrict(CI);
            break;
          }
          else if (!strcmp("FRONT_OBJECT", CI->curr_name)) 
          { *pso = so = new sql_stmt_drop(CATEG_PGMSRC);   if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->conditional=conditional;
            so->source_line = CI->prev_line;
            get_schema_and_name(CI, so->main, so->schema);
            so->categ=analyse_category(CI);
            next_sym(CI);
            break;
          }  
          else 
            c_error(NOT_AN_SQL_STATEMENT);
          break;
        default: c_error(NOT_AN_SQL_STATEMENT);
      }
      break;
    }

    case S_ALTER:
      switch (next_sym(CI))
      { case SQ_TABLE:
        { sql_stmt_alter_table * so = new sql_stmt_alter_table(SQL_STAT_ALTER_TABLE, false);  *pso=so;
          if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->old_alter_table_syntax = (CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE) != 0;
          { table_all ta_new, ta_old;
            table_def_comp(CI, &ta_new, &ta_old, ALTER_TABLE);
          }
         // save statement source:
          so->source=save_sql_source(source_start, CI->prepos);
          if (so->source==NULL) c_error(OUT_OF_MEMORY);
          break;
        }
        case SQ_PROCEDURE:  case SQ_FUNCTION: 
        // allows storing routine with syntax errors, supposes that ATLER is not followed by other statements.
        { sql_stmt_create_alter_rout * so = new sql_stmt_create_alter_rout(FALSE);  *pso=so;
          if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->get_text(CI);
          break;
        }
        case SQ_TRIGGER:
        // allows storing trigger with syntax errors, supposes that ATLER is not followed by other statements.
        { sql_stmt_create_alter_trig * so = new sql_stmt_create_alter_trig(FALSE);  *pso=so;
          if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->get_text(CI);
          break;
        }
        case SQ_SEQUENCE:
        { sql_stmt_create_alter_seq  * so = new sql_stmt_create_alter_seq (FALSE);  *pso=so;
          if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          CI->univ_ptr=&so->seq;
          so->seq.modifying=TRUE;  so->seq.recursive=FALSE;
          sequence_anal(CI); 
          break;
        }
        case SQ_DOMAIN:
        { sql_stmt_create_alter_dom * so = new sql_stmt_create_alter_dom(FALSE);     *pso=so;  
          if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          //so->get_text(CI);
          so->compile(CI);  // this requires correct syntax but allows using ALTER DOMAIN with other statements
          break;
        }
        case S_IDENT:
          if (!strcmp("FULLTEXT", CI->curr_name)) 
          { sql_stmt_create_alter_fulltext * so = new sql_stmt_create_alter_fulltext(false);  
            *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->source_line = CI->prev_line;
            so->ftdef.modifying=true;
            so->ftdef.compile_intern(CI);
           // save view source:
            so->source=save_sql_source(source_start, CI->prepos);
            if (so->source==NULL) c_error(OUT_OF_MEMORY);
          }
          else if (!strcmp("FRONT_OBJECT", CI->curr_name)) 
          { sql_stmt_create_alter_front_object * so = new sql_stmt_create_alter_front_object(false);   
            *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->source_line = CI->prev_line;
            get_schema_and_name(CI, so->name, so->schema);
            so->categ=analyse_category(CI);
            next_and_test(CI, S_AS, AS_EXPECTED);
            next_and_test(CI, S_STRING, STRING_EXPECTED);
            so->definition=(tptr)corealloc(strlen(CI->curr_string())+1, 81);
            if (!so->definition) c_error(OUT_OF_KERNEL_MEMORY);
            strcpy(so->definition, CI->curr_string());
            next_sym(CI);
          }
          else c_error(NOT_AN_SQL_STATEMENT);
          break;
        default: c_error(NOT_AN_SQL_STATEMENT);
      } // ALTER switch
      break;
    //////////////////////// syntax checking /////////////////////////
    case SQ_PROCEDURE:  case SQ_FUNCTION:
    { sql_stmt_create_alter_rout * so = new sql_stmt_create_alter_rout(TRUE);  *pso=so;
      if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->compile(CI);
      delete so;  *pso=NULL;
      if (CI->cursym==';') next_sym(CI);  // allows ';' after the body
      if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);  // following statements not allowed, they may cause confusion about the content of the procedure
      break;
    }
    case SQ_TRIGGER:
    { sql_stmt_create_alter_trig * so = new sql_stmt_create_alter_trig(TRUE);  *pso=so;
      if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->compile(CI);
      delete so;  *pso=NULL;
      if (CI->cursym==';') next_sym(CI);  // allows ';' after the body
      if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);  // following statements not allowed, they may cause confusion about the content of the procedure
      break;
    }
    case SQ_DOMAIN:
    { sql_stmt_create_alter_dom * so = new sql_stmt_create_alter_dom(true);     
      *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->compile(CI);
      delete so;  *pso=NULL;
      if (CI->cursym==';') next_sym(CI);  // allows ';' after the body
      if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);  // following statements not allowed, they may cause confusion about the content of the procedure
      break;
    }
    
    case S_INSERT: 
    { sql_stmt_insert * so = new sql_stmt_insert();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->compile(CI);
      break;
    }

    case S_DELETE: 
    { sql_stmt_delete * so = new sql_stmt_delete();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->compile(CI);
      break;
    }
    case S_UPDATE:
    { sql_stmt_update * so = new sql_stmt_update();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->compile(CI);
      break;
    }

    case S_IDENT: // not exact, view may start with id, too
     // special statements starting with a non-reserved identifier
      if (!strcmp(CI->curr_name, "RENAME"))
      { tcateg category;
        switch (next_sym(CI))
        { case SQ_TABLE:
            category=CATEG_TABLE;  break;
          case SQ_DOMAIN:
            category=CATEG_DOMAIN;  break;
          case SQ_PROCEDURE:   case SQ_FUNCTION:
            category=CATEG_PROC;  break;
          case SQ_TRIGGER:
            category=CATEG_TRIGGER;  break;
          case SQ_SEQUENCE:
            category=CATEG_SEQ;  break;
          case S_VIEW:
            category=CATEG_CURSOR;  break;
          case SQ_SCHEMA:
            category=CATEG_APPL;  break;
          case S_USER:
            category=CATEG_USER;  break;
          case S_GROUP:
            category=CATEG_GROUP;  break;
          case S_IDENT:
            if (CI->is_ident("ROLE"))       { category=CATEG_ROLE;  break; }
            if (CI->is_ident("STYLESHEET")) { category=CATEG_STSH;  break; }
            // cont.
          default: c_error(NOT_AN_SQL_STATEMENT);
        } // RENAME switch
        sql_stmt_rename_object * so = new sql_stmt_rename_object();  *pso=so;
        if (!so) c_error(OUT_OF_KERNEL_MEMORY);
        so->categ=category;
        get_schema_and_name(CI, so->orig_name, so->schema);
        if (category==CATEG_USER || category==CATEG_GROUP || category==CATEG_APPL)
          if (*so->schema) c_error(GENERAL_SQL_SYNTAX);
        test_and_next(CI, SQ_TO, TO_EXPECTED);
        if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
        strcpy(so->new_name, CI->curr_name);
        next_sym(CI);
        break;
      }

      if (strcmp(CI->curr_name, "SCROLL") && 
          strcmp(CI->curr_name, "INSENSITIVE") &&
          strcmp(CI->curr_name, "LOCKED") &&
          strcmp(CI->curr_name, "SENSITIVE")) c_error(NOT_AN_SQL_STATEMENT);   
    case SQ_TABLE:
    case S_SELECT:
    case S_VIEW:    /* opening cursor created as a viewed table */
    case SQ_CURSOR:
    case '(':
    { sql_stmt_select * so = new sql_stmt_select();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      CI->sql_kont->dynpar_referenced=FALSE;  // statement level, upper values should not be important
      CI->sql_kont->embvar_referenced=FALSE;
      sql_view(CI, &so->qe); // must use the outer kontext with local decls
      so->dynpar_referenced=CI->sql_kont->dynpar_referenced;
      so->embvar_referenced=CI->sql_kont->embvar_referenced;
      so->source_line = CI->prev_line;
      so->source=save_sql_source(source_start, CI->prepos);
      so->is_into = so->qe->qe_type==QE_SPECIF && ((t_qe_specif*)so->qe)->into.count();
      if (!(CI->cdp->ker_flags & KFL_NO_OPTIM))
      { so->opt=optimize_qe(CI->cdp, so->qe);
        if (so->opt==NULL) 
        { if (!so->is_into) { delete so->qe;  so->qe=NULL; }
          CI->propagate_error(); 
        }
      }
      break;
    }

    default:  c_error(NOT_AN_SQL_STATEMENT); 

    case SQ_BEGIN:
    { sql_stmt_block * so = new sql_stmt_block(label_name);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
     // check for ATOMIC or NOT ATOMIC (atomic member set to FALSE in constructor)
      if (next_sym(CI)==SQ_NOT)
      { next_sym(CI);
        if (!CI->is_ident("ATOMIC")) c_error(GENERAL_SQL_SYNTAX);
        next_sym(CI);
      }
      else if (CI->is_ident("ATOMIC"))
        { so->atomic=TRUE;  CI->sql_kont->atomic_nesting++;  next_sym(CI); }
      so->source_line = CI->prev_line;
     // calculate scope level, activate it and add routine name to it:
      so->scope_level=get_scope_level(CI);
      so->scope.activate(CI);  // DEFAULT clauses must be compiled in the same scopes as they will be executed
      if (*label_name)
        CI->sql_kont->active_scope_list->add_name(CI, label_name, LC_LABEL);
     // compile declarations and statements:
      so->scope.compile_declarations(CI, so->scope_level, &so->body, so->atomic, TRUE, CI->sql_kont->compiled_psm);
      sql_statement ** pdest = &so->body;
      while (*pdest) pdest=&((*pdest)->next_statement);
      compile_sql_statement_list(CI, pdest);
      test_and_next(CI, SQ_END, END_EXPECTED);
      so->scope.deactivate(CI);
     // skip own terminating label, if any (bad label remains on input):
      if (CI->cursym==S_IDENT && !strncmp(CI->curr_name, label_name, sizeof(label_name)-1))
        next_sym(CI);
     // leave own atomic kontext:
      if (so->atomic) CI->sql_kont->atomic_nesting--;
      so->source_line2 = CI->prev_line;  // BP on END
      break;
    }

    case SQ_CALL:
    { sql_stmt_call * so = new sql_stmt_call();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);
      so->source_line = CI->prev_line;
      if (CI->is_ident("DETACHED"))
      { so->detached=TRUE;
        if (next_sym(CI)==S_INTEGER)
        { so->term_event=check_integer(CI, 0, 0xffffffff);
          next_sym(CI);
        }
      }
     // read the routine name, search among local objects:
      tobjname routname, schemaname;  t_locdecl * locdecl;  int level;  tobjnum objnum;
      if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
      strcpy(routname, CI->curr_name);
      if (next_sym(CI)=='.')
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strcpy(schemaname, routname);
        reduce_schema_name(CI->cdp, schemaname);
        strcpy(routname, CI->curr_name);
        next_sym(CI);
      }
      else 
      { *schemaname=0;
       // look for locally declared routines, must not be prefixed
        locdecl = find_name_in_scopes(CI, routname, level, LC_VAR);
        if (locdecl!=NULL)
        { if (locdecl->loccat!=LC_ROUTINE) 
            { SET_ERR_MSG(CI->cdp, routname);  c_error(BAD_IDENT_CLASS); }
          if (so->detached /*&& locdecl->rout.routine->is_internal()*/) // internal && local && detached: not allowed
            c_error(CANNOT_DETACH);
          t_locdecl * locdecl_err = find_name_in_scopes(CI, routname, level, LC_LABEL);
          if (locdecl_err)
            c_error(LOCAL_RECURSION);   // local routine calling itself or calling upper local routine
          so->routine_invocation(CI, locdecl->rout.routine, ROUT_LOCAL);  // routine owned by the scope
          break;
        }
      }
     // prefixed name or name not declared locally, look for global:
      if (!name_find2_obj(CI->cdp, routname, schemaname, CATEG_PROC, &objnum)) // if found
      { t_routine * routine = get_stored_routine(CI->cdp, objnum, NULL, FALSE);
        if (routine==NULL) CI->propagate_error();
        so->routine_invocation(CI, routine, ROUT_GLOBAL_SHARED);   // routine deleted inside
        break;
      }
     // not local nor global, try standard:
      if (!*schemaname)
      { object * obj = search_obj(routname); // searches the object
        if (obj!=NULL)  // standard object found
        { if (obj->any.categ!=OBT_SPROC)
            { SET_ERR_MSG(CI->cdp, routname);  c_error(BAD_IDENT_CLASS); }
          if (so->detached) c_error(CANNOT_DETACH);
          t_expr_funct * fex = new t_expr_funct(obj->proc.sprocnum);  if (!fex) c_error(OUT_OF_KERNEL_MEMORY);
          so->std_routine=fex;
          if (obj->proc.pars==NULL)
          { fex->type=fex->origtype=obj->proc.retvaltype;  // may be changed below
            fex->specif.set_null();  fex->origspecif.set_null(); // may be changed
            special_function(CI, fex);
          }
          else call_gen(CI, &obj->proc, fex);
          break;
        }
      }
     // try extensions:
      HINSTANCE hInst;
      extension_object * exobj = search_obj_in_ext(CI->cdp, routname, schemaname, &hInst);
      if (exobj) 
      { SET_ERR_MSG(CI->cdp, routname);  // for errors here and in compile_ext_object()
        delete so;  so=NULL;  *pso=NULL;
        sql_stmt_call * call;
        compile_ext_object(CI, &call, exobj, false, hInst);
        *pso=call;
        return;
      }
     // not found anywhere:
      SET_ERR_MSG(CI->cdp, routname);  c_error(IDENT_NOT_DEFINED); 
      break;
    }
    case SQ_RETURN:
    { sql_stmt_return * so = new sql_stmt_return();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);
      so->source_line = CI->prev_line;
     // search for the function:
      t_scope * parscope = CI->sql_kont->active_scope_list;
      while (parscope!=NULL && !parscope->params)
        parscope=parscope->next_scope;
      if (parscope==NULL || parscope->params==2) c_error(RETURN_OUTSIDE_FUNCTION);
      t_locdecl * retval = parscope->find_name(return_value_name, LC_VAR);
      if (retval==NULL) c_error(RETURN_OUTSIDE_FUNCTION);
     // find the name:
      get_routine_name(parscope, so->function_name);
     // calculate the level:
      int level=0;
      while (parscope->next_scope!=NULL)
        { parscope=parscope->next_scope;  level++; }
      so->assgn.dest=new t_expr_var(retval->var.type, retval->var.specif, level, retval->var.offset);
      if (!so->assgn.dest) c_error(OUT_OF_KERNEL_MEMORY);
      sqlbin_expression(CI, &so->assgn.source, FALSE);
      convert_value(CI, so->assgn.source, so->assgn.dest->type, so->assgn.dest->specif);
      break;
    }
    case S_SET:
      if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "TRANSACTION"))
      { sql_stmt_set_trans * so = new sql_stmt_set_trans();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
        if (next_sym(CI)!=S_IDENT) c_error(GENERAL_SQL_SYNTAX);
        so->source_line = CI->prev_line;
        transaction_modes(CI, so->rw, so->isolation);
      }
      else // assignment statement
      { sql_stmt_set * so = new sql_stmt_set(NULL);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
        so->source_line = CI->prev_line;
        sql_selector(CI, &so->assgn.dest, false, false);
        test_and_next(CI, '=', EQUAL_EXPECTED);
        if (CI->cursym==SQ_CALL) next_sym(CI);
        sqlbin_expression(CI, &so->assgn.source, FALSE);
        if (so->assgn.source->sqe==SQE_DYNPAR)
          if (so->assgn.dest->sqe==SQE_DYNPAR)
            c_error(INCOMPATIBLE_TYPES);
          else
            supply_marker_type(CI, (t_expr_dynpar*)so->assgn.source, so->assgn.dest->type, so->assgn.dest->specif, MODE_IN);
        else
          if (so->assgn.dest->sqe==SQE_DYNPAR)
            supply_marker_type(CI, (t_expr_dynpar*)so->assgn.dest, so->assgn.source->type, so->assgn.source->specif, MODE_OUT);
          else
          {// allow atr:=multi, multi:=atr, used by table designer 
            BOOL smult = (so->assgn.source->type & 0x80) && so->assgn.source->sqe==SQE_ATTR;
            BOOL dmult = (so->assgn.dest  ->type & 0x80) && so->assgn.dest  ->sqe==SQE_ATTR;
            if (smult && !dmult)
            { t_expr_attr * atex = (t_expr_attr*)so->assgn.source;  sig32 zero=0;
              atex->indexexpr=new t_expr_sconst(ATT_INT32, 0, &zero, sizeof(sig32));
              if (!atex->indexexpr) c_error(OUT_OF_KERNEL_MEMORY);
              so->assgn.source->type &= 0x7f;
            }
            else if (dmult && !smult)
            { t_expr_attr * atex = (t_expr_attr*)so->assgn.dest;  sig32 zero=0;
              atex->indexexpr=new t_expr_sconst(ATT_INT32, 0, &zero, sizeof(sig32));
              if (!atex->indexexpr) c_error(OUT_OF_KERNEL_MEMORY);
              so->assgn.dest->type &= 0x7f;
            }
            convert_value(CI, so->assgn.source, so->assgn.dest->type, so->assgn.dest->specif);
          }
      }
      break;

    case SQ_IF:
    { sql_statement ** so_dest = pso;
      do // IF or ELSEIF is the current symbol
      { sql_stmt_if * so = new sql_stmt_if();  *so_dest=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
        next_sym(CI);
        so->source_line = CI->prev_line;
        search_condition(CI, &so->condition);
        test_and_next(CI, SQ_THEN, THEN_EXPECTED);
        compile_sql_statement_list(CI, &so->then_stmt);
        so_dest=&so->else_stmt;
      } while (CI->cursym==SQ_ELSEIF);
      if (CI->cursym==SQ_ELSE)
      { next_sym(CI);
        compile_sql_statement_list(CI, so_dest);
      }
      test_and_next(CI, SQ_END, END_EXPECTED);
      test_and_next(CI, SQ_IF,  IF_EXPECTED);
      break;
    }
    case SQ_CASE:
      if (next_sym(CI)==SQ_WHEN) // searched CASE
      { sql_stmt_case * so;
        sql_stmt_case * * pres = (sql_stmt_case**)pso;  // result to be assigned here
        do
        { next_sym(CI);
          so = new sql_stmt_case(true);  *pres=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          search_condition(CI, &so->cond);
          test_and_next(CI, SQ_THEN, THEN_EXPECTED);
          compile_sql_statement_list(CI, &so->branch);
          pres=&so->contin;
        } while (CI->cursym==SQ_WHEN);
        if (CI->cursym==SQ_ELSE)
        { next_sym(CI);
          so = new sql_stmt_case(true);  *pres=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          compile_sql_statement_list(CI, &so->branch);
        }
      }

      else // simple case, must coalesce types of labels
      // 1st sql_stmt_case contains patters value in cond and empty branch.
      { t_expr_null labelaux;  labelaux.specif.set_null();  // label supertype
        sql_stmt_case * so, * top;
        so = top = new sql_stmt_case(false);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
        so->source_line = CI->prev_line;
        sql_expression(CI, &so->cond);  // pattern value
        add_type(CI, &labelaux, so->cond);

        sql_stmt_case * * pres = &so->contin;  // result to be assigned here
        while (CI->cursym==SQ_WHEN)
        { next_sym(CI);
          so = new sql_stmt_case(false);  *pres=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          sql_expression(CI, &so->cond);
          add_type(CI, &labelaux,  so->cond);
          test_and_next(CI, SQ_THEN, THEN_EXPECTED);
          compile_sql_statement_list(CI, &so->branch);
          pres=&so->contin;
        }
        if (CI->cursym==SQ_ELSE)
        { next_sym(CI);
          so = new sql_stmt_case(false);  *pres=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          compile_sql_statement_list(CI, &so->branch);
        }
       // add conversions to the common type and comparison operator:
        t_expr_binary aux2;
        convert_value(CI, top->cond, labelaux.type, labelaux.specif);
        binary_conversion(CI, '=', top->cond, top->cond, &aux2);
        top->eq_oper=aux2.oper;
        so=top->contin;
        while (so!=NULL && so->cond!=NULL)
        { convert_value(CI, so->cond, labelaux.type, labelaux.specif);
          so=so->contin;
        }
      }
      test_and_next(CI, SQ_END, END_EXPECTED);
      test_and_next(CI, SQ_CASE, CASE_EXPECTED);
      break;

    case SQ_WHILE:
    { sql_stmt_loop * so = new sql_stmt_loop(label_name, true);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      if (*label_name)
      { if (!CI->sql_kont->active_scope_list) c_error(LABEL_OUTSIDE_BLOCK);
        CI->sql_kont->active_scope_list->add_name(CI, label_name, LC_LABEL);
      }
      next_sym(CI);
      so->source_line = CI->prev_line;
      search_condition(CI, &so->condition);
      test_and_next(CI, SQ_DO,   DO_EXPECTED);
      compile_sql_statement_list(CI, &so->stmt);
      test_and_next(CI, SQ_END,   END_EXPECTED);
      test_and_next(CI, SQ_WHILE, WHILE_EXPECTED);
      so->source_line2 = CI->prev_line;
     // skip own terminating label, if any (bad label remains on input):
      if (CI->cursym==S_IDENT && !strncmp(CI->curr_name, label_name, sizeof(label_name)-1))
        next_sym(CI);
      break;
    }
    case SQ_LOOP:
    { sql_stmt_loop * so = new sql_stmt_loop(label_name, false);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      if (*label_name)
      { if (!CI->sql_kont->active_scope_list) c_error(LABEL_OUTSIDE_BLOCK);
        CI->sql_kont->active_scope_list->add_name(CI, label_name, LC_LABEL);
      }
      next_sym(CI);
      so->source_line = CI->prev_line;
      compile_sql_statement_list(CI, &so->stmt);
      test_and_next(CI, SQ_END,  END_EXPECTED);
      test_and_next(CI, SQ_LOOP, LOOP_EXPECTED);
      so->source_line2 = CI->prev_line;
     // skip own terminating label, if any (bad label remains on input):
      if (CI->cursym==S_IDENT && !strncmp(CI->curr_name, label_name, sizeof(label_name)-1))
        next_sym(CI);
      break;
    }
    case SQ_REPEAT:
    { sql_stmt_loop * so = new sql_stmt_loop(label_name, false);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      if (*label_name)
      { if (!CI->sql_kont->active_scope_list) c_error(LABEL_OUTSIDE_BLOCK);
        CI->sql_kont->active_scope_list->add_name(CI, label_name, LC_LABEL);
      }
      next_sym(CI);
      so->source_line = CI->prev_line;
      compile_sql_statement_list(CI, &so->stmt);
      test_and_next(CI, SQ_UNTIL,  UNTIL_EXPECTED);
      search_condition(CI, &so->condition);
      so->source_line2 = CI->prev_line;  // stops on condition, not on END REPEAT
      test_and_next(CI, SQ_END,    END_EXPECTED);
      test_and_next(CI, SQ_REPEAT, REPEAT_EXPECTED);
     // skip own terminating label, if any (bad label remains on input):
      if (CI->cursym==S_IDENT && !strncmp(CI->curr_name, label_name, sizeof(label_name)-1))
        next_sym(CI);
      break;
    }
    case SQ_LEAVE:
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      sql_stmt_leave * so = new sql_stmt_leave(CI->curr_name);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->source_line = CI->prev_line;
      int level;
      if (find_name_in_scopes(CI, CI->curr_name, level, LC_LABEL) == NULL)
        { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); }
      next_sym(CI);
      break;
    }
    case SQ_FOR:
    { sql_stmt_for * so = new sql_stmt_for(label_name);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      if (*label_name)
      { if (!CI->sql_kont->active_scope_list) c_error(LABEL_OUTSIDE_BLOCK);
        CI->sql_kont->active_scope_list->add_name(CI, label_name, LC_LABEL);
      }
     // for variable:
      next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      so->source_line = CI->prev_line;
      t_idname for_variable;
      strmaxcpy(for_variable, CI->curr_name, sizeof(for_variable));
      next_and_test(CI, S_AS, AS_EXPECTED);
     // compile cursor decl.:
      if (next_sym(CI)==S_IDENT)  // cursor name
      { strmaxcpy(so->cursor_name, CI->curr_name, sizeof(so->cursor_name));
        next_sym(CI);
      }
      else // unnamed cursor
        { so->cursor_name[0]=0;  so->cursnum_offset=0; }
      sql_view(CI, &so->qe);
      so->opt=optimize_qe(CI->cdp, so->qe);
      if (so->opt==NULL) CI->propagate_error();
      // open cursor number will be in an automatic variable (recursion!)
      test_and_next(CI, SQ_DO, DO_EXPECTED);
     // new scope for the  statement list: calculate level and activate it
      so->scope_level=get_scope_level(CI);
      so->scope.activate(CI);
      if (add_cursor_structure_to_scope(CI->cdp, CI, so->scope, &so->qe->kont, for_variable)) c_error(OUT_OF_KERNEL_MEMORY);
     // for named cursor declare local name, used by CURRENT OF:
     // cursor name is expected to be AFTER for variable!
      if (so->cursor_name[0])
      { t_locdecl * locdecl = so->scope.add_name(CI, so->cursor_name, LC_CURSOR);
        locdecl->cursor.qe =so->qe;   so->qe->AddRef();
        locdecl->cursor.opt=so->opt;  so->opt->AddRef();
        locdecl->cursor.for_cursor=TRUE;
        locdecl->cursor.offset=so->cursnum_offset=so->scope.extent;
        so->scope.extent+=sizeof(tcursnum);
      }
     // compile stament list and drop the scope:
      compile_sql_statement_list(CI, &so->body);
      so->scope.deactivate(CI);
      test_and_next(CI, SQ_END, END_EXPECTED);
      so->source_line2 = CI->prev_line;
      test_and_next(CI, SQ_FOR, FOR_EXPECTED);
     // skip own terminating label, if any (bad label remains on input):
      if (CI->cursym==S_IDENT && !strncmp(CI->curr_name, label_name, sizeof(label_name)-1))
        next_sym(CI);
      break;
    }

    case SQ_SIGNAL:
    { sql_stmt_signal * so = new sql_stmt_signal;  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      if (next_sym(CI)==SQ_SQLSTATE)
      { next_sym(CI);
        if (CI->is_ident("VALUE")) next_sym(CI);
        if (CI->cursym!=S_STRING) c_error(STRING_EXPECTED);
        if (strlen(CI->curr_string()) != 5) c_error(SQLSTATE_FORMAT);
        strcpy(so->name, CI->curr_string());
        so->is_sqlstate=TRUE;
      }
      else
      { if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
        strmaxcpy(so->name, CI->curr_name, sizeof(so->name));
        so->is_sqlstate=FALSE;
      }
      so->source_line = CI->prev_line;
      next_sym(CI);
      break;
    }
    case SQ_RESIGNAL:
    { sql_stmt_resignal * so = new sql_stmt_resignal;  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      if (next_sym(CI)==SQ_SQLSTATE)
      { next_sym(CI);
        if (CI->is_ident("VALUE")) next_sym(CI);
        if (CI->cursym!=S_STRING) c_error(STRING_EXPECTED);
        if (strlen(CI->curr_string()) != 5) c_error(SQLSTATE_FORMAT);
        strcpy(so->name, CI->curr_string());
        so->is_sqlstate=TRUE;
        next_sym(CI);
      }
      else if (CI->cursym==S_IDENT)
      { strmaxcpy(so->name, CI->curr_name, sizeof(so->name));
        so->is_sqlstate=FALSE;
        next_sym(CI);
      }
      else
        so->name[0]=0;
      so->source_line = CI->prev_line;
      break;
    }

    ////////////////////////// cursor statements /////////////////////////////
    case SQ_OPEN:
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      sql_stmt_open * so = new sql_stmt_open(CI->curr_name);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->source_line = CI->prev_line;
      t_locdecl * locdecl = find_name_in_scopes(CI, CI->curr_name, so->level, LC_CURSOR);
      if (locdecl != NULL) // locally declared cursor
      { if (locdecl->cursor.for_cursor) c_error(FOR_CURSOR_INVALID);
        so->offset =locdecl->cursor.offset;
        so->loc_qe =locdecl->cursor.qe;
        so->loc_opt=locdecl->cursor.opt;
      }
      else so->offset=-1;  // global cursor
      // existence of global cursor not tested here
      // { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); }
      next_sym(CI);
      break;
    }
    case SQ_CLOSE:
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      sql_stmt_close * so = new sql_stmt_close(CI->curr_name);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->source_line = CI->prev_line;
      t_locdecl * locdecl = find_name_in_scopes(CI, CI->curr_name, so->level, LC_CURSOR);
      if (locdecl != NULL) // locally declared cursor
      { if (locdecl->cursor.for_cursor) c_error(FOR_CURSOR_INVALID);
        so->offset =locdecl->cursor.offset;
        so->loc_qe =locdecl->cursor.qe;
        so->loc_opt=locdecl->cursor.opt;
      }
      else so->offset=-1;  // global cursor
      // existence of global cursor not tested here
      // { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); }
      next_sym(CI);
      break;
    }

    case SQ_FETCH:
    { sql_stmt_fetch * so = new sql_stmt_fetch();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      t_fetch_type fetch_type = FT_UNSPEC;
      if (next_sym(CI)==S_IDENT) // orientation or cursor name
      { if (!strcmp(CI->curr_name, "NEXT"))     fetch_type=FT_NEXT;  else
        if (!strcmp(CI->curr_name, "PRIOR"))    fetch_type=FT_PRIOR; else
        if (!strcmp(CI->curr_name, "FIRST"))    fetch_type=FT_FIRST; else
        if (!strcmp(CI->curr_name, "LAST"))     fetch_type=FT_LAST;  else
        if (!strcmp(CI->curr_name, "ABSOLUTE")) fetch_type=FT_ABS;   else
        if (!strcmp(CI->curr_name, "RELATIVE")) fetch_type=FT_REL;
        if (fetch_type != FT_UNSPEC)  // direction specified
        { next_sym(CI);
          if (fetch_type==FT_ABS || fetch_type==FT_REL)
          { sql_expression(CI, &so->step);
            if (so->step->type!=ATT_INT32 && so->step->type!=ATT_INT16 && so->step->type!=ATT_INT8 || so->step->specif.scale!=0)
              c_error(MUST_BE_INTEGER);
          }
          test_and_next(CI, S_FROM, FROM_OMITTED);
        }
        else fetch_type=FT_NEXT;
      }
      else // not ident
      { test_and_next(CI, S_FROM, FROM_OMITTED);
        fetch_type=FT_NEXT;
      }
      so->source_line = CI->prev_line;

      so->fetch_type=fetch_type;
     // cursor name:
      if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
      strmaxcpy(so->cursor_name, CI->curr_name, sizeof(so->cursor_name));
      t_locdecl * locdecl = find_name_in_scopes(CI, CI->curr_name, so->level, LC_CURSOR);
      if (locdecl != NULL) // locally declared cursor
      { if (locdecl->cursor.for_cursor) c_error(FOR_CURSOR_INVALID);
        so->offset=locdecl->cursor.offset;
      }
      else
        so->offset=-1;  // global cursor
     // INTO clause:
      int elnum=1;
      if (next_sym(CI)==S_INTO)
      {// remove the DB context when compiling the INTO clause, avoidin conflict of names between columns and variables:
        db_kontext * db_kont = CI->sql_kont->active_kontext_list;  CI->sql_kont->active_kontext_list = NULL;
        do
        { next_sym(CI);
          t_assgn * assgn = so->assgns.next();
          if (assgn==NULL) c_error(OUT_OF_MEMORY);
          sql_selector(CI, &assgn->dest, false, false);
          if (locdecl)  // check the assignment compatibility
          { elem_descr * el = locdecl->cursor.qe->kont.elems.acc(elnum);
            if (el)
            { if (get_type_cast(el->type, assgn->dest->type, el->specif, assgn->dest->specif, TRUE)==CONV_ERROR)
                c_error(INCOMPATIBLE_TYPES);
            }
            else c_error(INCOMPATIBLE_TYPES);
          }
          elnum++;
        } while (CI->cursym==',');
        CI->sql_kont->active_kontext_list = db_kont;
      }
      // else c_error(INTO_EXPECTED);  -- WinBase extension
      // existence of global cursor not tested here
      // assignment source added in fetch execution
      // { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); }
      break;
    }
    ////////////////////// transaction statements ////////////////////////////
    case SQ_START:
    { sql_stmt_start_tra * so = new sql_stmt_start_tra();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      if (next_sym(CI)!=S_IDENT || strcmp(CI->curr_name, "TRANSACTION"))
        c_error(TRANSACTION_EXPECTED);
      so->source_line = CI->prev_line;
      next_sym(CI);
      transaction_modes(CI, so->rw, so->isolation);
      break;
    }

    case SQ_COMMIT:
    { sql_stmt_commit * so = new sql_stmt_commit();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->chain=FALSE;
      if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "WORK")) next_sym(CI);
      so->source_line = CI->prev_line;
      if (CI->cursym==SQ_AND)
      { if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "NO")) next_sym(CI);
        else so->chain=TRUE;
        if (!CI->is_ident("CHAIN")) c_error(CHAIN_EXPECTED);
        next_sym(CI);
      }
      if (CI->sql_kont->atomic_nesting) c_error(INVALID_IN_ATOMIC_BLOCK);
      break;
    }
    case SQ_ROLLBACK:
    { sql_stmt_rollback * so = new sql_stmt_rollback();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->chain=FALSE;
      if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "WORK")) next_sym(CI);
      so->source_line = CI->prev_line;
      if (CI->cursym==SQ_AND)
      { if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "NO")) next_sym(CI);
        else so->chain=TRUE;
        if (!CI->is_ident("CHAIN")) c_error(CHAIN_EXPECTED);
        next_sym(CI);
      }
      if (CI->cursym==SQ_TO)
      { next_and_test(CI, SQ_SAVEPOINT, SAVEPOINT_EXPECTED);
        if (so->chain) c_error(SPECIF_CONFLICT);
        savepoint_specifier(CI, so->sapo_name, so->sapo_target);
      }
      else // global ROLLBACK
      { so->sapo_name[0]=0;   // sapo_targer is set to NULL in constructor
        if (CI->sql_kont->atomic_nesting) c_error(INVALID_IN_ATOMIC_BLOCK);
      }
      break;
    }
    case SQ_SAVEPOINT:
    { sql_stmt_sapo * so = new sql_stmt_sapo();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      savepoint_specifier(CI, so->sapo_name, so->sapo_target);
      so->source_line = CI->prev_line;
      break;
    }

    case SQ_RELEASE:
    { sql_stmt_rel_sapo * so = new sql_stmt_rel_sapo();  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      next_and_test(CI, SQ_SAVEPOINT, SAVEPOINT_EXPECTED);
      so->source_line = CI->prev_line;
      savepoint_specifier(CI, so->sapo_name, so->sapo_target);
      break;
    }

    case ';':  case S_SOURCE_END:   /* empty statement */
    { sql_statement * so = new sql_statement(SQL_STAT_NO);  *pso=so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->source_line = CI->prev_line;
      break;
    }

    case S_GRANT:
      general_flag=TRUE;
      goto grant_revoke;
    case S_REVOKE:
      general_flag=FALSE;
     grant_revoke:
    { sql_stmt_grant_revoke * so;
      *pso = so = general_flag ? new sql_stmt_grant_revoke(SQL_STAT_GRANT) : 
                                 new sql_stmt_grant_revoke(SQL_STAT_REVOKE);
      if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      BOOL grant_option_flag=FALSE;
     /* find the table name */
      CIpos pos(CI);     // save compil. position
      while (next_sym(CI)!=S_ON)  /* skips privileges */
        if (CI->cursym==S_SOURCE_END) c_error(ON_EXPECTED);
      so->source_line = CI->prev_line;
      next_sym(CI);
      if (CI->cursym==SQ_PROCEDURE || CI->cursym==SQ_FUNCTION)
        { so->is_routine=TRUE;  next_sym(CI); }
      else so->is_routine=FALSE; // default
      if (CI->cursym!=S_IDENT) c_error(TABLE_NAME_EXPECTED);
      strmaxcpy(so->objname, CI->curr_name, sizeof(so->objname));
      if (next_sym(CI)=='.')
      { strcpy(so->schemaname, so->objname);
        next_and_test(CI, S_IDENT, TABLE_NAME_EXPECTED);
        strmaxcpy(so->objname, CI->curr_name, sizeof(so->objname));
        next_sym(CI);
      }
      else so->schemaname[0]=0;
      tobjnum objnum;  BOOL found_table=FALSE;
      if (!so->is_routine)
        found_table=!name_find2_obj(CI->cdp, so->objname, so->schemaname, CATEG_TABLE, &objnum);
      if (found_table)
      { table_descr_auto td(CI->cdp, objnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
        so->priv_val.init(td->attrcnt);
      }
      else
      { if (name_find2_obj(CI->cdp, so->objname, so->schemaname, CATEG_PROC, &objnum))
          { SET_ERR_MSG(CI->cdp, so->objname);  c_error(OBJECT_NOT_FOUND); }
        so->is_routine=TRUE;
        so->priv_val.init(objtab_descr->attrcnt);
      }
      pos.restore(CI);
     /* analyse the privileges: */
      next_sym(CI);
      if (!general_flag)
      { if (CI->cursym==S_GRANT)
        { if (next_sym(CI)!=S_IDENT || strcmp(CI->curr_name, "OPTION"))
            c_error(GENERAL_SQL_SYNTAX);
          next_and_test(CI, SQ_FOR, FOR_EXPECTED);
          next_sym(CI);
          grant_option_flag=TRUE;
        }
      }
      if (CI->cursym==S_ALL)
      { so->priv_val.set_all_rw();
        *so->priv_val.the_map()=RIGHT_READ|RIGHT_WRITE|RIGHT_INSERT|RIGHT_DEL;
        next_sym(CI);
        // in WB "PRIVILEGES" is optional (had been forgotten)
        if (CI->is_ident("PRIVILEGES")) next_sym(CI);
      }
      else
      { so->priv_val.clear();
        bool all_read=false, all_write=false;
        do
        { switch (CI->cursym)
          { case S_DELETE: *so->priv_val.the_map()|=RIGHT_DEL;     next_sym(CI);  break;
            case S_INSERT: *so->priv_val.the_map()|=RIGHT_INSERT;  next_sym(CI);  break;
            case S_GRANT:  *so->priv_val.the_map()|=RIGHT_GRANT;   next_sym(CI);  break;
            case S_SELECT:
            case S_REFERENCES:
            { if (next_sym(CI)=='(')
                read_attr_list(CI, objnum, so->priv_val, false);
              else 
              { *so->priv_val.the_map()|=RIGHT_READ;  // cannot call set_all, must OR
                all_read=true;
              }
              break;
            }
            case S_UPDATE:
            { if (next_sym(CI)=='(')
                read_attr_list(CI, objnum, so->priv_val, true);
              else 
              { *so->priv_val.the_map()|=RIGHT_WRITE; 
                 if (so->is_routine) so->priv_val.add_write(OBJ_DEF_ATR);
                 else // cannot call set_all  -- must OR
                   all_write=true;

              }
              break;
            }
            case S_IDENT:
              if (!strcmp(CI->curr_name, "EXECUTE"))
              { *so->priv_val.the_map()|=RIGHT_READ;  so->priv_val.add_read(OBJ_DEF_ATR);  next_sym(CI);
              }
              break;
          }
          if (CI->cursym!=',') break;
          next_sym(CI);
        } while (TRUE);
        if (all_read || all_write)
          for (int ii=0;  ii<so->priv_val.colbytes();  ii++)
            so->priv_val.the_map()[1+ii] |= all_read && all_write ? 0xff : all_write ? 0xaa : 0x55;
      }
      test_and_next(CI, S_ON, ON_EXPECTED);
      if (CI->cursym==SQ_PROCEDURE || CI->cursym==SQ_FUNCTION)
        next_sym(CI);
      test_and_next(CI, S_IDENT, IDENT_EXPECTED);
      if (CI->cursym=='.')  { next_sym(CI);  next_sym(CI); }
     /* analyse subject names: */
      if (so->statement_type==SQL_STAT_GRANT)
        test_and_next(CI, SQ_TO, TO_EXPECTED);
      else
        test_and_next(CI, S_FROM, FROM_OMITTED);
      if (CI->cursym==S_PUBLIC)  // define EVERYBODY subject iff PUBLIC specified:
      { t_uname * subj = so->user_names.next();
        if (!subj) c_error(OUT_OF_MEMORY);
        *subj->subject_name=0;  subj->category=CATEG_GROUP;
        next_sym(CI); 
      }
      else // subjects enumerated:
        do
        {// create space for the new subject: 
          t_uname * subj = so->user_names.next();
          if (!subj) c_error(OUT_OF_MEMORY);
         // read the category specification, if present:
          if      (CI->cursym==S_USER)  { subj->category=CATEG_USER;   next_sym(CI); }
          else if (CI->cursym==S_GROUP) { subj->category=CATEG_GROUP;  next_sym(CI); }
          else if (CI->is_ident("ROLE")){ subj->category=CATEG_ROLE;   next_sym(CI); }
          else                            subj->category=0; // category not specified
         // read the subject name (may be qualified if category==CATEG_ROLE):
          if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
          strmaxcpy(subj->subject_name, CI->curr_name, sizeof(subj->subject_name));
          if (next_sym(CI)=='.' && subj->category==CATEG_ROLE)
          { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
            strcpy(subj->subject_schema_name, subj->subject_name);
            strmaxcpy(subj->subject_name, CI->curr_name, sizeof(subj->subject_name));
            next_sym(CI);
          }
         // check for the next subject:
          if (CI->cursym==',') next_sym(CI);
          else break;
        } while (TRUE);
     // process the "WITH GRANT OPTION" clause:
      if (so->statement_type==SQL_STAT_GRANT)
      { if (CI->cursym==SQ_WITH)
        { next_and_test(CI, S_GRANT, GENERAL_SQL_SYNTAX);
          if (next_sym(CI)!=S_IDENT || strcmp(CI->curr_name, "OPTION"))
            c_error(GENERAL_SQL_SYNTAX);
          next_sym(CI);
          grant_option_flag=TRUE;
        }
      }
      if (grant_option_flag) *so->priv_val.the_map() |= RIGHT_GRANT;
     // compile optional WHERE clause and prepare cursor:
      if (!so->is_routine)
        so->qe=compile_condition(CI, objnum, so->objname, so->schemaname, CATEG_TABLE, so->current_cursor_name, &so->opt);
    }
    break;
  } // switch
} // compile_sql_statement

void compile_sql_statement_list(CIp_t CI, sql_statement ** pso)
// Compiles nested list of statements. Returns NULL for empty list.
{ while (CI->cursym != S_SOURCE_END && CI->cursym != SQ_END &&
         CI->cursym != SQ_ELSE      && CI->cursym != SQ_ELSEIF &&
         CI->cursym != SQ_UNTIL     && CI->cursym != SQ_WHEN)
  { compile_sql_statement(CI, pso);
    if (*pso!=NULL) pso=&((*pso)->next_statement);
    test_and_next(CI, ';', SEMICOLON_EXPECTED);
  }
}

void sql_stmt_create_alter_rout::compile(CIp_t CI)
// "PROCEDURE" or "FUNCTION" is current now.
{ const char * source_start=CI->prepos;  // position of "PROCEDURE" or "FUNCTION"
 // extract routine name:
  symbol sym1 = CI->cursym;  
  get_schema_and_name(CI, name, schema);
 // compile routine (testing only) into the holding member rout:
  rout = new t_routine(CATEG_PROC, NOOBJECT);  if (!rout) c_error(OUT_OF_KERNEL_MEMORY);
  rout->compile_routine(CI, sym1, name, schema);
  delete rout;  rout=NULL;
 // save routine source:
  source=save_sql_source(source_start, CI->prepos);
  if (source==NULL) c_error(OUT_OF_MEMORY);
}

void sql_stmt_create_alter_rout::get_text(CIp_t CI)
{ const char * source_start=CI->prepos;  // position of "PROCEDURE" or "FUNCTION"
 // extract routine name:
  get_schema_and_name(CI, name, schema);
 // skip routine text:
  while (CI->cursym!=S_SOURCE_END) next_sym(CI);
 // save routine source:
  source=save_sql_source(source_start, CI->prepos);
  if (source==NULL) c_error(OUT_OF_MEMORY);
}

void sql_stmt_create_alter_trig::compile(CIp_t CI)
// "TRIGGER" is current now.
{ const char * source_start=CI->prepos;  // position of "TRIGGER"
 // extract trigger name and schema name:
  get_schema_and_name(CI, name, schema);
  WBUUID schema_uuid;
  if (*schema)
  { if (ker_apl_name2id(CI->cdp, schema, schema_uuid, NULL))
      {  SET_ERR_MSG(CI->cdp, schema);  c_error(OBJECT_NOT_FOUND); }
  }
  else memcpy(schema_uuid, CI->cdp->current_schema_uuid, UUID_SIZE);
 // compile trigger (testing only) to the holding member trig:
  { t_short_term_schema_context stsc(CI->cdp, schema_uuid);
    trig = new t_trigger(NOOBJECT);  if (!trig) c_error(OUT_OF_KERNEL_MEMORY);  
    trig->compile_trigger(CI, name, schema, FALSE);  
  }
 // save trigger source:
  source=save_sql_source(source_start, CI->prepos);
  if (source==NULL) c_error(OUT_OF_MEMORY);
}

void sql_stmt_create_alter_trig::get_text(CIp_t CI)
{ const char * source_start=CI->prepos;  // position of "TRIGGER"
 // extract trigger schema and name:
  get_schema_and_name(CI, name, schema);
 // skip trigger text:
  while (CI->cursym!=S_SOURCE_END) next_sym(CI);
 // save routine source:
  source=save_sql_source(source_start, CI->prepos);
  if (source==NULL) c_error(OUT_OF_MEMORY);
}

void sql_stmt_create_alter_dom::compile(CIp_t CI)
// "DOMAIN" is current now.
{ const char * source_start=CI->prepos;  // position of "DOMAIN"
 // extract domain name and schema name:
  get_schema_and_name(CI, name, schema);
  WBUUID schema_uuid;
  if (*schema)
  { if (ker_apl_name2id(CI->cdp, schema, schema_uuid, NULL))
      {  SET_ERR_MSG(CI->cdp, schema);  c_error(OBJECT_NOT_FOUND); }
  }
  else memcpy(schema_uuid, CI->cdp->current_schema_uuid, UUID_SIZE);
  if (CI->cursym==S_AS) next_sym(CI);
 // compile domain (testing only) to the holding member trig:
  { t_short_term_schema_context stsc(CI->cdp, schema_uuid);
    analyse_type(CI, tp, specif, NULL, 0);  // history and pointers not supported
    t_type_specif_info tsi;
    compile_domain_ext(CI, &tsi);
   // compile the default value:
    if (tsi.defval!=NULL)
    { attribdef att;
      att.name[0]=0;  att.attrtype=tp;  att.attrspecif=specif;  att.attrmult=1;  att.defvalexpr=NULL;
      compil_info xCI(CI->cdp, tsi.defval, default_value_link);
      t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  // must be defined, even with empty kontext
      xCI.univ_ptr=&att;
      int res=compile_masked(&xCI);
      if (att.defvalexpr)
        { delete att.defvalexpr;  att.defvalexpr=NULL; }
      if (res) c_error(res);
    }
   // compile the check constrain:
    if (tsi.check!=NULL)
    { table_all ta;
      atr_all * al = ta.attrs.acc(0);
      if (!al) c_error(OUT_OF_KERNEL_MEMORY);
      al->type=al->orig_type=tp;  al->specif=specif;  al->multi=1;  al->name[0]=0;
      dd_check cc(1);  cc.constr_name[0]=0;  
      compil_info xCI(CI->cdp, tsi.check, check_comp_link);
      t_sql_kont sql_kont;  sql_kont.kont_ta=&ta;  xCI.sql_kont=&sql_kont;
      sql_kont.column_number_for_value_keyword = 0;
      xCI.univ_ptr=&cc;
      int res=compile_masked(&xCI);
      if (cc.expr)  // ? should be done in the destructor of cc, but no harm
        { delete cc.expr;  cc.expr=NULL; }
      if (res) c_error(res);
    }
  }
 // save domain source:
  source=save_sql_source(source_start, CI->prepos);
  if (source==NULL) c_error(OUT_OF_MEMORY);
}

#ifdef STOP
void sql_stmt_create_alter_dom::get_text(CIp_t CI)
{ const char * source_start=CI->prepos;  // position of "DOMAIN"
 // extract domain and schema name:
  get_schema_and_name(CI, name, schema);
 // skip trigger text:
  while (CI->cursym!=S_SOURCE_END) next_sym(CI);
 // save routine source:
  source=save_sql_source(source_start, CI->prepos);
  if (source==NULL) c_error(OUT_OF_MEMORY);
}
#endif
////////////////// entry points to SQL compilation & execution ///////////////
void WINAPI sql_statement_seq(CIp_t CI)
// Compiles sequence of sql statements separated by semicolons.
// Returns ptr to the (nonempty!) linked list in stmt_ptr.
{ sql_statement ** so_dest = (sql_statement**)&CI->stmt_ptr;
  do
  { compile_sql_statement(CI, so_dest);
    if (*so_dest!=NULL) so_dest=&((*so_dest)->next_statement);
    if (CI->cursym==';') next_sym(CI); 
    else break;
  } while (CI->cursym != S_SOURCE_END);  // condition used when the last statement is terminated by ;
  if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);
}

void WINAPI sql_statement_1(CIp_t CI)
// Compiles a sql statement. Returns ptr to it in stmt_ptr.
{ compile_sql_statement(CI, (sql_statement**)&CI->stmt_ptr);
  if (CI->cursym==';') next_sym(CI);
  if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);
}

sql_statement * sql_submitted_comp(cdp_t cdp, tptr sql_statements, t_scope * active_scope, bool mask_errors)
// Compiles statement sequence with possible DP markers into a linked list.
// Returns NULL on compilation error, calls request_error.
{ compil_info xCI(cdp, sql_statements, sql_statement_seq);
  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;
  xCI.sql_kont->active_scope_list=active_scope;
  uns16 mask = xCI.cdp->mask_compile_error;
  xCI.cdp->mask_compile_error = mask_errors;  
  int err=compile(&xCI);
  xCI.cdp->mask_compile_error = mask;
  sql_statement * so=(sql_statement*)xCI.stmt_ptr;
  if (err) /* error in compilation */ { if (so) delete so;  return NULL; }
  return so;
}

BOOL sql_exec_direct(cdp_t cdp, tptr source)
// Compiles & executes single SQL statement. Returns FALSE if OK, TRUE on error.
{ compil_info xCI(cdp, source, sql_statement_1);
  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;
  int err=compile(&xCI);
  sql_statement * so=(sql_statement*)xCI.stmt_ptr;
  if (err) /* error in compilation */ { delete so;  return TRUE; }
  BOOL res=so->exec(cdp);
  process_rollback_conditions_by_cont_handler(cdp);
  delete so;
  return res;
}

BOOL sql_open_cursor(cdp_t cdp, tptr curdef, tcursnum & cursnum, cur_descr ** pcd, bool direct_stmt)
// Opens given cursor. Returns FALSE and its number in cursnum if OK,
// TRUE and NOCURSOR in cursnum on error.
{ *pcd=NULL;
 // compile the cursor def:
  compil_info xCI(cdp, curdef, sql_statement_1);
  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;
  int err=compile(&xCI);
  sql_statement * so = (sql_statement*)xCI.stmt_ptr;
  if (err) /* error in compilation */ { cursnum=NOCURSOR;  goto exit; }
 // check the statement type:
  if (so==NULL ||
      so->next_statement != NULL || // multiple statements submitted! */
      so->statement_type != SQL_STAT_SELECT ||  /* not a select statement */
      ((sql_stmt_select*)so)->is_into)
  { request_error(cdp, SELECT_EXPECTED);
    cursnum=NOCURSOR;  goto exit;
  }
 // open the cursor:
  prof_time start, orig_lock_waiting_time;
  start = get_prof_time();  cdp->lower_level_time=0;  
  orig_lock_waiting_time=cdp->lock_waiting_time;
  cdp->last_line_num = (uns32)-1; 
  cursnum=create_cursor(cdp, ((sql_stmt_select*)so)->qe, curdef, ((sql_stmt_select*)so)->opt, pcd);
  if (PROFILING_ON(cdp) && direct_stmt) 
    add_hit_and_time2(get_prof_time()-start, cdp->lower_level_time, cdp->lock_waiting_time-orig_lock_waiting_time, PROFCLASS_SQL, 0, 0, curdef);
#ifdef STOP  // should not be necessary
  if (cursnum!=NOCURSOR)
    if (((sql_stmt_select*)so)->dynpar_referenced || ((sql_stmt_select*)so)->embvar_referenced)   // for dynpar not necessary, makes it slow
      make_complete(*pcd);
#endif
 exit:
  delete so;
  return cursnum==NOCURSOR;
}

tcursnum open_cursor_in_current_kontext(cdp_t cdp, const char * query, cur_descr ** pcd)
{ 
  t_rscope * rscope = cdp->rscope; // starting with cdp->rscope, used when the server processes a DAD with parameters from the URL
  // if DAD operation called from the procedure, the params scope of the extenal proc is temporarily removed
  t_scope * scope_list = create_scope_list_from_run_scopes(rscope);  // rscope changed by this call, must not use cdp->rscope!
  compil_info xCI(cdp, query, sql_statement_1);
  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  xCI.sql_kont->active_scope_list=scope_list;
  int err=compile(&xCI);
  sql_statement * so = (sql_statement*)xCI.stmt_ptr;
  if (err) /* error in compilation */ 
  { delete so;  
    return NOCURSOR;
  }
  if (so==NULL ||
      so->next_statement != NULL || // multiple statements submitted! */
      so->statement_type != SQL_STAT_SELECT ||  /* not a select statement */
      ((sql_stmt_select*)so)->is_into)
  { request_error(cdp, SELECT_EXPECTED);
    return NOCURSOR;
  }
 // open the cursor:
  tcursnum cursnum=create_cursor(cdp, ((sql_stmt_select*)so)->qe, (char*)query, ((sql_stmt_select*)so)->opt, pcd);
  unregister_cursor_creation(cdp, *pcd);
  make_complete(*pcd);
  delete so;
  return cursnum;
}

///////////////////////////// routine /////////////////////////////////////////
void t_routine::compute_external_frame_size(void)
{ external_frame_size=0;
  for (int i=0;  i<scope.locdecls.count();  i++)
  { t_locdecl * locdecl = scope.locdecls.acc0(i);
    if      (locdecl->loccat==LC_INPAR || locdecl->loccat==LC_INPAR_)
    { int thisparsize = simple_type_size(locdecl->var.type, locdecl->var.specif);
#ifdef __ia64__
      int thisparspace = ((thisparsize-1)/sizeof(long)+1)*sizeof(long);
#else
      int thisparspace = ((thisparsize-1)/sizeof(int)+1)*sizeof(int);
#endif
      external_frame_size+=thisparspace;
    }
    else if (locdecl->loccat==LC_OUTPAR || locdecl->loccat==LC_OUTPAR_ || locdecl->loccat==LC_INOUTPAR || locdecl->loccat==LC_FLEX)
      external_frame_size+=sizeof(void*);
  }
}

void t_routine::compile_routine(CIp_t CI, symbol sym, tobjname nameIn, tobjname schemaIn)  
// Symbol after routine name is current on entry.
// [schemaIn]=="" if schema not specified.
{ CI->sql_kont->in_procedure_compilation++;
  if (sym==SQ_FUNCTION) scope.extent=1; // space for no_return_executed_yet flag
  strcpy(name, nameIn);  
  if (!*schemaIn) 
    memcpy(schema_uuid, CI->cdp->current_schema_uuid, UUID_SIZE);
  else
  { if (ker_apl_name2id(CI->cdp, schemaIn, schema_uuid, NULL))
      {  SET_ERR_MSG(CI->cdp, schemaIn);  c_error(OBJECT_NOT_FOUND); }
  }
  { t_short_term_schema_context stsc(CI->cdp, schema_uuid);
   // calculate scope level:
    scope_level=get_scope_level(CI);
   // compile formal parameters list:
    test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
    if (CI->cursym!=')')
    do
    { int tp;  t_specif specif;  t_loc_categ param_cat;
     // parameter mode not allowed for functions, WB extension:
      if      (CI->cursym==SQ_IN)        { param_cat=LC_INPAR;     next_sym(CI); }
      else if (CI->cursym==SQ_OUT)       { param_cat=LC_OUTPAR;    next_sym(CI); }
      else if (CI->cursym==SQ_INOUT)     { param_cat=LC_INOUTPAR;  next_sym(CI); }
      else if (CI->is_ident("FLEXIBLE")) { param_cat=LC_FLEX;      next_sym(CI); }
      else                                 param_cat=LC_PAR;
     // parameter name, if any
      scope.store_pos();
      if (CI->cursym==S_IDENT)
      { scope.add_name(CI, CI->curr_name, param_cat);
        next_sym(CI);
      }
      else // unnamed parameter, create fictive name in the form #PARnum
      { char buf[16] = "#PAR";
        int2str(scope.locdecls.count(), buf+4);
        scope.add_name(CI, buf, param_cat);
      }
      t_express ** pdefval = &scope.locdecls.acc0(scope.locdecls.count()-1)->var.defval;
      *pdefval=NULL;
      analyse_variable_type(CI, tp, specif, pdefval);
      scope.store_type(tp, specif, FALSE, *pdefval);
      if (CI->cursym==S_DEFAULT)
      { if (*pdefval!=NULL) { delete *pdefval;  *pdefval=NULL; }  // DEFAULT specified in the domain too
        next_sym(CI);
        sqlbin_expression(CI, pdefval, FALSE);
      }
      if (CI->cursym != ',') break;
      next_sym(CI);
    } while (TRUE);
    test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
    t_locdecl * locdecl = scope.add_name(CI, name, LC_LABEL); // must be after params
    locdecl->label.rout = this;
   // return value for functions:
    if (sym==SQ_FUNCTION)
    { test_and_next(CI, SQ_RETURNS, RETURNS_EXPECTED);
      analyse_variable_type(CI, rettype, retspecif, NULL);
      scope.store_pos();
      scope.add_name(CI, return_value_name, LC_VAR);
      scope.store_type(rettype, retspecif, FALSE, NULL);
    }
    else rettype=0;  // procedure!
    test_and_next(CI, ';', SEMICOLON_EXPECTED);

   // compile body unless only routine info needed:
    if (category==CATEG_PROC)
    {// activate the scope:
      scope.activate(CI);
     // routine body:
      if (CI->cursym==SQ_EXTERNAL)
      { if (IS_HEAP_TYPE(rettype)) c_error(THIS_TYPE_NOT_ENABLED);
        if (next_sym(CI)!=S_IDENT || strcmp(CI->curr_name, "NAME"))
          c_error(NAME_EXPECTED);
        next_and_test(CI, S_STRING, STRING_EXPECTED);  // single-character names not supported here (S_CHAR)
        tptr nm = CI->curr_string();
        names = (tptr)corealloc(strlen(nm)+2, 86);
        if (names==NULL) c_error(OUT_OF_MEMORY);
        strcpy(names, nm);
        nm=names;
       // analyse the name:
        while (*nm && *nm!='@') nm++;
        if (*nm) *nm=0;  else { SET_ERR_MSG(CI->cdp, "<unnamed>");  c_error(LIBRARY_NOT_FOUND); }
        next_sym(CI);
        procptr=NULL;  // not found yet
       // check parameter types and convert modes:
        int i;
        for (i=0;  i<scope.locdecls.count();  i++)
        { t_locdecl * locdecl = scope.locdecls.acc0(i);
          if (locdecl->loccat==LC_PAR) locdecl->loccat=LC_INPAR;
          if (locdecl->loccat==LC_INPAR || locdecl->loccat==LC_OUTPAR ||
              locdecl->loccat==LC_INPAR_|| locdecl->loccat==LC_OUTPAR_)  // INOUT allowed, but must not grow, FLEX if OK
            if (IS_HEAP_TYPE(locdecl->var.type)) c_error(THIS_TYPE_NOT_ENABLED);
        }
       // calc. ext. param size:
        compute_external_frame_size();
      }
      else
      { compile_sql_statement(CI, &stmt);
        if (!stmt) c_error(NOT_AN_SQL_STATEMENT);
        if (CI->cursym==S_IDENT && !stricmp(name, CI->curr_name))
          next_sym(CI); // closing END may be followed by the routine name which is the implicit label of BEGIN
      }
      scope.deactivate(CI);
    } // compiling the body
  } // short-term context
  CI->sql_kont->in_procedure_compilation--;
}
/////////////////////////// trigger ///////////////////////////////////////////
t_trigger::~t_trigger(void)  
{ delete condition;  delete stmt; }

void t_trigger::mark_core(void)  // marks itself too, does not mark recursivety in the list
{ if (condition) condition->mark_core();
  if (stmt) stmt->mark_core();
  scope.mark_core();
  tabkont.mark_core();   
  column_map.mark_core();   
  rscope_usage_map.mark_core();   
  mark(this);
}

void t_trigger::compile_trigger(CIp_t CI, tobjname nameIn, tobjname schemaIn, BOOL partial_comp)
// Symbol after trigger name is current on entry.
// Partial compilation only iff [partial_comp].
// When [partial_comp] is FALSE then the current schema kontext is set to the schema of the trigger.
{ if (CI->sql_kont) CI->sql_kont->in_procedure_compilation++;
  strcpy(trigger_name, nameIn);  strcpy(trigger_schema, schemaIn);
 // trigger time:
  if      (CI->cursym==SQ_BEFORE) time_before=TRUE;
  else if (CI->cursym==SQ_AFTER)  time_before=FALSE;
  else c_error(BEFORE_OR_AFTER_EXPECTED);
 // trigger event:
  next_sym(CI);
  CIpos pos(CI);     // save compil. position
  if      (CI->cursym==S_DELETE) { tevent=TGA_DELETE;  next_sym(CI); }
  else if (CI->cursym==S_INSERT) { tevent=TGA_INSERT;  next_sym(CI); }
  else if (CI->cursym==S_UPDATE) 
  { tevent=TGA_UPDATE;
    if (next_sym(CI)==SQ_OF)
    { column_map_explicit=TRUE;
      next_sym(CI);
      if (CI->cursym=='=') { list_comparison=TRIG_COMP_EQUAL;  next_sym(CI); }
      else if (CI->cursym==SQ_ANY) { list_comparison=TRIG_COMP_ANY;  next_sym(CI); }
      do // 1st pass -> skipping column list, cursym must be a column name
      { test_and_next(CI, S_IDENT, IDENT_EXPECTED);
        if (CI->cursym!=',') break;
        next_sym(CI);
      } while (TRUE);
    }
    else 
      column_map_explicit=FALSE;
  }
  else c_error(TRIGGER_EVENT_EXPECTED);
 // ON table name:
  test_and_next(CI, S_ON, ON_EXPECTED);
  if (CI->cursym!=S_IDENT) c_error(TABLE_NAME_EXPECTED);
  strcpy(table_name, CI->curr_name);
  int table_column_count;
  if (!partial_comp)  // unless partial compilation 
  { ttablenum tablenum = find_object (CI->cdp, CATEG_TABLE, table_name); // full compilation of the trigger, schema context has been set outside
    if (tablenum==NOOBJECT) { SET_ERR_MSG(CI->cdp, table_name);  c_error(TABLE_DOES_NOT_EXIST); }
    table_descr_auto td(CI->cdp, tablenum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
    if (td->me()==NULL) CI->propagate_error();  // propagates compilation error
    table_column_count=td->attrcnt;
    tabkont.fill_from_td(CI->cdp, td->me());
  }
 // analyse explicit column names:
  if (tevent==TGA_UPDATE && column_map_explicit)
  { if (!partial_comp)
    { column_map.init(tabkont.elems.count());
      column_map.clear();
    }
    pos.restore(CI);  next_sym(CI);  // restored on UPDATE, moved on OF
    next_sym(CI);  // '=', ANY or a column name
    if (CI->cursym=='=' || CI->cursym==SQ_ANY) next_sym(CI); 
    do // 1st pass -> skipping column list, cursym is a column name
    { if (!partial_comp)  // unless partial compilation - tabkont not set!
      { int elemnum;
        if (tabkont.find(NULL, CI->curr_name, elemnum) == NULL)
          { SET_ERR_MSG(CI->cdp, CI->curr_name);   c_error(ATTRIBUTE_NOT_FOUND); }
        column_map.add(elemnum);
      }
      if (next_sym(CI)!=',') break;
      next_sym(CI);
    } while (TRUE);
    next_sym(CI);  // skip ON, table name is the current symbol again
  }
 // REFERENCING (may be next):
  *old_name = *new_name = 0;
  if (next_sym(CI)==SQ_REFERENCING)
  { BOOL first = TRUE;
    do // OLD or NEW is expected to be current
    { tptr dest = NULL;
      if (next_sym(CI) == S_IDENT) 
        if      (!strcmp(CI->curr_name, "OLD")) 
          { dest=old_name;  if (tevent==TGA_INSERT) c_error(SPECIF_CONFLICT); }
        else if (!strcmp(CI->curr_name, "NEW")) 
          { dest=new_name;  if (tevent==TGA_DELETE) c_error(SPECIF_CONFLICT); }
      if (dest==NULL) if (first) c_error(OLD_OR_NEW_EXPECTED); else break;
      if (next_sym(CI) == SQ_TABLE) c_error(TRANSIENT_TABLES_NOT_SUPP);
      if (CI->is_ident("ROW")) next_sym(CI);
      if (CI->cursym==S_AS) next_sym(CI);
      if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
      if (*dest) c_error(SPECIF_CONFLICT);
      strcpy(dest, CI->curr_name);
      first=FALSE;
    } while (TRUE);
    if (!strcmp(old_name, new_name)) c_error(NAME_DUPLICITY);  // nepresna chyba
  }
 // FOR may be current:
  if (CI->cursym==SQ_FOR)
  { next_and_test(CI, S_IDENT, EACH_EXPECTED);
    if (strcmp(CI->curr_name, "EACH")) c_error(EACH_EXPECTED);
    next_and_test(CI, S_IDENT, ROW_OR_STAT_EXPECTED);
    if      (!strcmp(CI->curr_name, "ROW"      )) row_granularity=TRUE;
    else if (!strcmp(CI->curr_name, "STATEMENT")) row_granularity=FALSE;
    else c_error(ROW_OR_STAT_EXPECTED);
    next_sym(CI);
  }
  else row_granularity=FALSE;  // implicit
  if ((*old_name || *new_name) && !row_granularity) c_error(SPECIF_CONFLICT);
  trigger_type=_trigger_type();
  if (!partial_comp)  // partial compilation stops here!
  {// prepare the compilation kontext (variable copy):
    BOOL scope_activated=FALSE;
    t_struct_type * row;
   // scope necessary in before triggers even if not referenced
   // The trigger is always executed with a rscope (possible fictive), so I thinkt it is better to have a sscope always.
    //if (/*time_before ? *new_name : *old_name*/ time_before || *old_name)
    { tptr prefix = time_before ? new_name : old_name;
      if (!*prefix) strcpy(prefix, "#");   // names disabled by this prefix (but scope must exist?)
      t_locdecl * locdecl = scope.add_name(CI, prefix, LC_VAR);
      locdecl->var.type  =0;             locdecl->var.specif=0;
      locdecl->var.offset=scope.extent;  locdecl->var.defval=NULL;
      row = new t_struct_type;    if (!row) c_error(OUT_OF_KERNEL_MEMORY);
      locdecl->var.stype=row;            locdecl->var.structured_type=true;
      if (row->fill_from_kontext(&tabkont)) c_error(OUT_OF_MEMORY);
      scope.extent+=row->valsize;
      scope.activate(CI);  scope_activated=TRUE;
    }
   // set the compilation kontext (database access):
    if (has_db_access())
    { tabkont.set_compulsory_prefix(time_before ? old_name : new_name);
      tabkont.next=NULL;  CI->sql_kont->active_kontext_list=&tabkont;
    }
   // compile WHEN:
    if (CI->cursym==SQ_WHEN)
    { next_and_test(CI, '(', LEFT_PARENT_EXPECTED);
      next_sym(CI);
      search_condition(CI, &condition);
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
    }
   // statement:
    compile_sql_statement(CI, &stmt);
    rscope_usage_map.init(table_column_count);
    rscope_usage_map.clear();
   // drop the kontext:
    if (scope_activated)
    { scope.deactivate(CI);  
     // create the rscope usage_map:
      for (int i=0;  i<row->elems.count();  i++)
        if (row->elems.acc0(i)->used)  // [used] has been initialised in fill_from_kontext, the scope is not reused for compilation
          rscope_usage_map.add(i+1);
    }
    CI->sql_kont->active_kontext_list=NULL;
  }
  if (CI->sql_kont) CI->sql_kont->in_procedure_compilation--;
}

static void WINAPI trigger_compile_entry(CIp_t CI)
{ t_trigger * trig = (t_trigger *)CI->stmt_ptr;
  if (CI->cursym==SQ_DECLARE || CI->cursym==S_CREATE) next_sym(CI); // CREATE: for wider compatibility
  if (CI->cursym!=SQ_TRIGGER) c_error(TRIGGER_EXPECTED);
  tobjname name, schema;  get_schema_and_name(CI, name, schema);
  trig->compile_trigger(CI, name, schema, NULL!=CI->univ_ptr);
}

t_trigger * obtain_trigger(cdp_t cdp, tobjnum objnum)
// Returns the trigger specified by objnum. Uses the psm_cache.
{ t_trigger * trig;
 // try the cache:
  trig = psm_cache.get_trigger(objnum);
  if (trig!=NULL) return trig;
 // load the trigger source:
  tptr src=ker_load_objdef(cdp, objtab_descr, objnum);
  if (src==NULL) 
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  trig = new t_trigger(objnum);  
  if (trig==NULL)
    { corefree(src);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
 // find the application kontext of the trigger and set it:
  WBUUID uuid; // schema uuid
  tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)uuid);
  t_short_term_schema_context stsc(cdp, uuid);
 // compile it:
  compil_info xCI(cdp, src, trigger_compile_entry);
  xCI.stmt_ptr=trig;
  xCI.univ_ptr=(void*)FALSE;  // indicates the full compilation
  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  sql_kont.compiled_psm=objnum;
  cdp->in_trigger++; // trigger compilation flag
  int err=compile_masked(&xCI);  // client error not set when error in trigger encountered
  cdp->in_trigger--;
  if (err)  // compilation error
  { if (!(cdp->ker_flags & KFL_DISABLE_REFINT)) // disables error messages when importing/deleting a schema
    { tobjname trigger_name;
      tb_read_atr(cdp, objtab_descr, objnum, OBJ_NAME_ATR, trigger_name);
      char buf[OBJ_NAME_LEN+100];  //sprintf(buf, msg_trigger_error, trigger_name, objnum, err);
      form_message(buf, sizeof(buf), msg_trigger_error, trigger_name, objnum, err);
      trace_msg_general(cdp, TRACE_BCK_OBJ_ERROR, buf, NOOBJECT);
    }
    delete trig;  trig=NULL; 
  }
  corefree(src);
  return trig;
}

BOOL get_trigger_info(cdp_t cdp, tobjnum objnum, tobjname table_name, int & trigger_type)
// Loads the trigger source, partially compiles it and extracts table name and trigger type.
{ tptr src=ker_load_objdef(cdp, objtab_descr, objnum);
  if (src==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  t_trigger * trig = new t_trigger(objnum);  
  if (trig==NULL)
    { corefree(src);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  compil_info xCI(cdp, src, trigger_compile_entry);
  xCI.stmt_ptr=trig;
  xCI.univ_ptr=(void*)TRUE;  // indicates the partial compilation
  int err=compile_masked(&xCI);  // client error not set when error in trigger encountered
  corefree(src);
  if (!err)
  { strcpy(table_name, trig->table_name);
    trigger_type = trig->trigger_type;
  }
  delete trig;  // partial trigger, must not put it into the cache!
  return !err;
}

/////////////////////////////// the registry of triggers ////////////////////////////////
/* The association of tables and its triggers is stored in __RELTAB with REL_CLASS_TRIG.
     A record represents every trigger for which its table name and type can be determined.
     The trigger definition may contain errors and its table does not need to exist.
   Changes in __RELTAB record are performed by the register_unregister_trigger procedure.
     It is called when executing CREATE TRIGGER, DROP TRIGGER and ALTER TRIGGER (twice).
     It is called by the client API function cd_Trigger_changed too.
   cd_Trigger_changed is called when wizard creates a new trigger, when the trigger's definition 
     is saved (twice) and when trigger is imported. Triggers are deleted by DROP TRIGGER only.
   List of triggers associated with a table can be loaded into table_descr by calling [prepare_trigger_info].
     This must be done before using trigger_map or calling triggers.
   Creating/saving a trigger which cannot be compiled (e.g. drag&drop a trigger before its table is copied, saving initial
     trigger from the wizard, saving edited trigger from the text editor, import): Cannot use CREATE/ALTER TRIGGER,
     must call Insert_object, Write the definition and call cd_Trigger_changed to unregister/register.
*/

void register_unregister_trigger(cdp_t cdp, tobjnum objnum, BOOL registering)
// When [registering] is TRUE, adds a record about trigger [objnum] into __RELTAB.
// When [registering] is FALSE, removes the record about trigger [objnum] from __RELTAB.
{// get trigger schema uuid and name:
  WBUUID uuid;  tobjname trig_name;
  tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)uuid);
  tb_read_atr(cdp, objtab_descr, objnum, OBJ_NAME_ATR, trig_name);
 // get table name and trigger type:
  tobjname table_name;  int trigger_type;
  if (!get_trigger_info(cdp, objnum, table_name, trigger_type))
    return;  // cannot analyse the trigger
 // find the table and invalidate its list of triggers:
  ttablenum tabnum = find2_object(cdp, CATEG_TABLE, uuid, table_name);
  if (tabnum!=NOOBJECT)
  { EnterCriticalSection(&cs_tables);
    if (tables[tabnum] && tables[tabnum]!=(table_descr*)-1) tables[tabnum]->invalidate_trigger_info();
    LeaveCriticalSection(&cs_tables);
  }
  if (registering)
  {// insert the information into __RELTAB:
    { table_descr_auto td(cdp, REL_TABLENUM);
      if (td->me())
      { t_reltab_vector rtv(REL_CLASS_TRIG, table_name, uuid, trig_name, uuid, trigger_type);
        t_vector_descr vector(reltab_coldescr, &rtv);
        tb_new(cdp, td->me(), FALSE, &vector);
      }
    }
  }
  else // unregistering
  {// delete the information from __RELTAB:
    t_reltab_primary_key start(REL_CLASS_TRIG, table_name, uuid, trig_name, uuid),
                         stop (REL_CLASS_TRIG, table_name, uuid, trig_name, uuid);
    t_index_interval_itertor iii(cdp);
    if (!iii.open(REL_TABLENUM, REL_TAB_PRIM_KEY, &start, &stop)) 
      return;
    trecnum rec=iii.next();
    if (rec!=NORECNUM) 
      tb_del(cdp, iii.td, rec);
  }
}

void table_descr::load_list_od_triggers(cdp_t cdp)
// Triggers may have been loaded before, so must drop the original list
// Must be called inside the CS.
{ bool err=false;
  if (trigger_info_valid) return;  // tested outside the CS, but must test again
  if (!SYSTEM_TABLE(tbnum) && tbnum>0)
  { t_reltab_primary_key start(REL_CLASS_TRIG, selfname, schema_uuid, NULLSTRING, schema_uuid),
                         stop (REL_CLASS_TRIG, selfname, schema_uuid, "\xff",    schema_uuid);
    t_index_interval_itertor iii(cdp);
    if (!iii.open(REL_TABLENUM, REL_TAB_PRIM_KEY, &start, &stop)) 
      return;
   // drop the original info:
    trigger_map=0;
    triggers.clear();
   // load the new info:
    do
    { trecnum rec=iii.next();
      if (rec==NORECNUM) break;
     // find the referencing table:
      WBUUID schema_uuid;  tobjname trig_name;  uns32 trigger_type;
      fast_table_read(cdp, iii.td, rec, REL_CLD_UUID_COL, schema_uuid);
      fast_table_read(cdp, iii.td, rec, REL_CLD_NAME_COL, trig_name);
      fast_table_read(cdp, iii.td, rec, REL_INFO_COL,     &trigger_type);
      tobjnum trigobjnum = find2_object(cdp, CATEG_TRIGGER, schema_uuid, trig_name);
     // add the trigger to the list:
      if (trigobjnum!=NOOBJECT)  // NOOBJECT if trigger deleted from OBJTAB, but not from __RELTAB (inconsistency)
      { t_trigger_type_and_num * trigger_type_and_num = triggers.next();
        if (trigger_type_and_num==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  err=true;  break; }
        trigger_type_and_num->trigger_objnum=trigobjnum;
        trigger_type_and_num->trigger_type  =trigger_type;
        trigger_map|=trigger_type; 
      }
    } while (true);  
  }
  if (err) { triggers.clear();  trigger_map=0; }
  else trigger_info_valid=TRUE;
}

BOOL t_trigger::exec(cdp_t cdp)  // supposes the rscope to be set
{ BOOL err;
 // trace:
  if (cdp->trace_enabled & TRACE_TRIGGER_EXEC)
  { char buf[100+OBJ_NAME_LEN];
    trace_msg_general(cdp, TRACE_TRIGGER_EXEC, form_message(buf, sizeof(buf), msg_trace_trig_exec, trigger_name), NOOBJECT);
  }
  t_exkont_trig ekt(cdp, this);
  if (condition!=NULL) 
  { t_value res;
    expr_evaluate(cdp, condition, &res);
    if (cdp->except_name[0]) return TRUE;
    if (!res.is_true()) return FALSE;
  }
 // complete the profile of the calling line before entering the new scope (before changing top_objnum):
  CHANGING_SCOPE(cdp);
  cdp->stk_add(objnum, trigger_name);
  cdp->top_objnum = objnum;
  err=stmt->exec(cdp);
  process_rollback_conditions_by_cont_handler(cdp);
 // complete the profile of the last line before changing scopes (before changing top_objnum):
  CHANGING_SCOPE(cdp);
  cdp->stk_drop();
  return err;
}

void t_exkont_trig::get_descr(cdp_t cdp, char * buf, int buflen)
{ char mybuf[2*OBJ_NAME_LEN+50];  
  strcpy(mybuf, " [Trigger ");
  if (*trigger->trigger_schema) { strcat(mybuf, trigger->trigger_schema);  strcat(mybuf, "."); }
  strcat(mybuf, trigger->trigger_name);  
  if (cdp->top_objnum == trigger->objnum && cdp->last_line_num!=(uns32)-1)
    sprintf(mybuf+strlen(mybuf), " line %u", cdp->last_line_num);
  strcat(mybuf, "]");
  if (buflen>0) strmaxcpy(buf, mybuf, buflen);
}

void t_exkont_trig::get_info(cdp_t cdp, t_exkont_info * info)
{ t_exkont::get_info(cdp, info);  
  info->par1 = trigger->objnum; 
  info->par2 = cdp->top_objnum == trigger->objnum ? cdp->last_line_num : (uns32)-1;
} 

static inline bool have_common_col(const uns32 * map1, const uns32 * map2, unsigned cnt)
{ cnt = (cnt-1) / 32 + 1;  // number of 32-bit units
  do
  { if (*map1 & *map2) return true;
    map1++;  map2++;
  } while (--cnt);
  return false;
}

BOOL execute_triggers(cdp_t cdp, table_descr * tbdf, int trigger_type, t_rscope * rscope, trecnum posrec, t_atrmap_flex * column_map)
// rscope is NULL only in the API cd_Insert/cd_Append. 
{ //if (!(tbdf->trigger_map & trigger_type)) return FALSE;  -- tested above
  t_short_term_schema_context stsc(cdp, tbdf->schema_uuid);  // using table schema uuid, same as trigger schema
  for (int i=0;  i<tbdf->triggers.count();  i++)
  { t_trigger_type_and_num * trigger_type_and_num = tbdf->triggers.acc0(i);
    if (trigger_type_and_num->trigger_type & trigger_type)
    { t_trigger * trig = obtain_trigger(cdp, trigger_type_and_num->trigger_objnum);
      if (!trig) continue;
     // check column list, if specified: 
      if (column_map!=NULL && trig->column_map_explicit)
        if (trig->list_comparison==TRIG_COMP_EQUAL || trig->list_comparison==TRIG_COMP_DEFAULT && cdp->sqloptions & SQLOPT_COL_LIST_EQUAL ? 
              !(trig->column_map==*column_map) : !trig->column_map.intersects(column_map))
          { psm_cache.put(trig);  continue; }
     // set rscope and position:
      if (rscope!=NULL) { rscope->next_rscope=cdp->rscope;  cdp->rscope=rscope; }
      trig->position(posrec);  // position (only triggers compiled in db_kontext need this)
     // save and set the database kontext for debugging purposes:
      db_kontext * upper_kont = cdp->last_active_trigger_tabkont;
      if (trig->has_db_access())
        cdp->last_active_trigger_tabkont = &trig->tabkont;
      cdp->in_trigger++;
      cdp->in_admin_mode++;
      prof_time start, saved_lower_level_time, orig_lock_waiting_time;
      if (PROFILING_ON(cdp)) 
      { saved_lower_level_time = cdp->lower_level_time;  cdp->lower_level_time=0;
        orig_lock_waiting_time=cdp->lock_waiting_time;
        start = get_prof_time();  // important when the procedure starts profiling!
      }
      BOOL res=trig->exec(cdp);
      if (PROFILING_ON(cdp)) 
      { prof_time interv = get_prof_time()-start;
        add_hit_and_time2(interv, cdp->lower_level_time, cdp->lock_waiting_time-orig_lock_waiting_time, PROFCLASS_TRIGGER, trig->objnum, 0, NULL);
        cdp->lower_level_time = saved_lower_level_time + interv;
      }  
      cdp->in_admin_mode--;
      cdp->in_trigger--;
      cdp->last_active_trigger_tabkont = upper_kont;
      if (rscope!=NULL) { cdp->rscope=rscope->next_rscope;  rscope->next_rscope=NULL; } // leak_test() needs this!! 
      psm_cache.put(trig);
      if (res)  // errors changed to SQ_TRIGGERED_ACTION
      { if (may_have_handler(cdp->get_return_code(), 0))  // limited to standard errors, unclear
          if (cdp->get_return_code()!=SQ_INVAL_TRANS_TERM)
            { cdp->except_name[0]=0;  request_error(cdp, SQ_TRIGGERED_ACTION); }
        return TRUE; 
      }
    }
  }
  return FALSE;
}

t_shared_scope * get_trigger_scope(cdp_t cdp, table_descr * tbdf)
// Returns the static scope description of any trigger, increments the reference count of the scope.
// May return NULL on error!
{ t_shared_scope * ssc;
  ProtectedByCriticalSection cs(&tbdf->lockbase.cs_locks);  // must not allow 2 thread working with this in the same time!
  if (!tbdf->trigger_scope)
  { ssc = new t_shared_scope(2);
    if (!ssc) return NULL;
    t_struct_type * row = new t_struct_type;
    if (row==NULL) goto err;
    t_locdecl * locdecl = ssc->locdecls.next();
    if (!locdecl) goto err;
    locdecl->name[0]=0;  // name ignored in the run-time
    locdecl->loccat=LC_VAR;
    locdecl->var.stype=row;  locdecl->var.specif=0;  locdecl->var.defval=NULL;  locdecl->var.structured_type=true;
    locdecl->var.offset=0;  // ssc->extent is 0 now
    if (row->fill_from_table_descr(tbdf)) goto err;
    ssc->extent=row->valsize;  // ssc->extent was 0 here
    tbdf->trigger_scope = ssc;
  }
  else
    ssc = tbdf->trigger_scope;
  ssc->AddRef();  // adds the caller as the new co-owner of ssc
  return ssc;
 err: // scope not created OK, drop the partially created object
  ssc->Release();  // new() completed, ref_cnt is 1 
  return NULL;
}

void rscope_init(t_rscope * rscope)
{ 
  rscope->level=0;  // the outmost level
  for (int i=0;  i<rscope->sscope->locdecls.count();  i++)
  { const t_locdecl * locdecl = rscope->sscope->locdecls.acc0(i);
    if      (locdecl->loccat==LC_CURSOR)
      *(tcursnum *)(rscope->data+locdecl->cursor.offset) = NOCURSOR;  // cursor is not open
    else if (locdecl->loccat==LC_TABLE)
      *(ttablenum*)(rscope->data+locdecl->table.offset ) = NOTBNUM;   // table is not created so far
    else if (locdecl->loccat==LC_VAR || locdecl->loccat==LC_CONST)
    { if (locdecl->var.structured_type)
      { const t_struct_type * st = locdecl->var.stype;
        for (int j=0;  j<st->elems.count();  j++)
          if (IS_HEAP_TYPE(st->elems.acc0(j)->type) || IS_EXTLOB_TYPE(st->elems.acc0(j)->type))
          { t_value * val = (t_value*)(rscope->data+locdecl->var.offset+st->elems.acc0(j)->offset);
            val->init();  val->length=0;  // length 0 is usefull when code fails to check [vmode]  
          }
      }
      else if (IS_HEAP_TYPE(locdecl->var.type) || IS_EXTLOB_TYPE(locdecl->var.type))
      { t_value * val = (t_value*)(rscope->data+locdecl->var.offset);
        val->init();  val->length=0;  // length 0 is usefull when code fails to check [vmode]
      }
    }
  }
}

void rscope_clear(t_rscope * rscope)
{ if (rscope==NULL) return;
  const t_scope * scope = rscope->sscope;
  for (int i=0;  i<scope->locdecls.count();  i++)
  { const t_locdecl * locdecl = scope->locdecls.acc0(i);
    if (locdecl->loccat==LC_CURSOR)  // ??
      *(tcursnum*)(rscope->data+locdecl->cursor.offset) = NOCURSOR;
    else if (locdecl->loccat==LC_VAR || locdecl->loccat==LC_CONST)
    { if (locdecl->var.structured_type)
      { t_struct_type * st = locdecl->var.stype;
        for (int j=0;  j<st->elems.count();  j++)
          if (IS_HEAP_TYPE(st->elems.acc0(j)->type) || IS_EXTLOB_TYPE(st->elems.acc0(j)->type))
          { t_value * val = (t_value*)(rscope->data+locdecl->var.offset+st->elems.acc0(j)->offset);
            val->set_null();
            val->length=0;  // safer
          }
      }
      else if (IS_HEAP_TYPE(locdecl->var.type) || IS_EXTLOB_TYPE(locdecl->var.type))
      { t_value * val = (t_value*)(rscope->data+locdecl->var.offset);
        val->set_null();
        val->length=0;  // safer
      }
    }
  }
}

void rscope_set_null(t_rscope * rscope) // affects the non-heap variables only!
{ const t_scope * scope = rscope->sscope;
  for (int i=0;  i<scope->locdecls.count();  i++)
  { t_locdecl * locdecl = scope->locdecls.acc0(i);
    if (locdecl->loccat==LC_VAR || locdecl->loccat==LC_CONST)
    { if (locdecl->var.structured_type)
      { t_struct_type * st = locdecl->var.stype;
        for (int j=0;  j<st->elems.count();  j++)
        { t_row_elem * sel = st->elems.acc0(j);
          if (!IS_HEAP_TYPE(sel->type) && !IS_EXTLOB_TYPE(sel->type))
            qset_null(rscope->data+locdecl->var.offset+sel->offset, sel->type, sel->specif.length);
        }
      }
      else if (!IS_HEAP_TYPE(locdecl->var.type) && !IS_EXTLOB_TYPE(locdecl->var.type))
        qset_null(rscope->data+locdecl->var.offset, locdecl->var.type, locdecl->var.specif);
    }
  }
}

BOOL load_record_to_rscope(cdp_t cdp, table_descr * tbdf, trecnum rec, 
             t_rscope * rscope1, const t_atrmap_flex * map1, t_rscope * rscope2, const t_atrmap_flex * map2)
{ const char * dt;  tfcbnum fcbn=NOFCBNUM;  int page=-1;  attribdef * att;  int i;
  tptr dest1 = rscope1 ? rscope1->data : NULL;
  tptr dest2 = rscope2 ? rscope2->data : NULL;
  for (i=1, att=tbdf->attrs+1;  i<tbdf->attrcnt;  i++, att++)
  { if (att->attrpage!=page)
    { unfix_page(cdp, fcbn);
      dt=fix_attr_read(cdp, tbdf, rec, i, &fcbn);
      if (dt==NULL) return TRUE;
      dt-=att->attroffset;
      page = att->attrpage;
    }
    if (IS_HEAP_TYPE(att->attrtype))
    { heapacc * tmphp = (heapacc*)(dt+att->attroffset);
      if (dest1)
      { if (map1->has(i))
        { t_value * val = (t_value*)dest1;
          if (val->allocate(tmphp->len)) { unfix_page(cdp, fcbn);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
          tptr vdest = val->valptr();
          hp_read(cdp, &tmphp->pc, 1, 0, tmphp->len, vdest);
          val->length=tmphp->len;
          vdest[val->length]=vdest[val->length+1]=0;
        }
        dest1+=sizeof(t_value);
      }
      if (dest2) 
      { if (map2->has(i))
        { t_value * val = (t_value*)dest2;
          if (val->allocate(tmphp->len)) { unfix_page(cdp, fcbn);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
          tptr vdest = val->valptr();
          hp_read(cdp, &tmphp->pc, 1, 0, tmphp->len, vdest);
          val->length=tmphp->len;
          vdest[val->length]=vdest[val->length+1]=0;
        }
        dest2+=sizeof(t_value);
      }
    }
    else if (IS_EXTLOB_TYPE(att->attrtype))
    { uns64 lob_id = *(uns64*)(dt+att->attroffset);
      uns32 length = read_ext_lob_length(cdp, lob_id);
      if (dest1)
      { if (map1->has(i))
        { t_value * val = (t_value*)dest1;
          if (val->allocate(length)) { unfix_page(cdp, fcbn);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
          tptr vdest = val->valptr();
          read_ext_lob(cdp, lob_id, 0, length, vdest);
          val->length=length;
          vdest[val->length]=vdest[val->length+1]=0;
        }
        dest1+=sizeof(uns64);
      }
      if (dest2)
      { if (map2->has(i))
        { t_value * val = (t_value*)dest2;
          if (val->allocate(length)) { unfix_page(cdp, fcbn);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
          tptr vdest = val->valptr();
          read_ext_lob(cdp, lob_id, 0, length, vdest);
          val->length=length;
          vdest[val->length]=vdest[val->length+1]=0;
        }
        dest2+=sizeof(uns64);
      }
    }
    else 
    { if (dest1)
      { if (map1->has(i)) Fil2Mem_copy(dest1, dt+att->attroffset, att->attrtype, att->attrspecif);
        dest1 += simple_type_size(att->attrtype, att->attrspecif);
      }
      if (dest2)
      { if (map2->has(i)) Fil2Mem_copy(dest2, dt+att->attroffset, att->attrtype, att->attrspecif);
        dest2 += simple_type_size(att->attrtype, att->attrspecif);
      }
    }
  }
  unfix_page(cdp, fcbn);
  return FALSE;
}


/* 
Routines and triggers have to be compiled before execution. 
The result of the compilation is a linked structure pointed by (t_routine *) or (t_trigger *).
Compiled routines and triggers are not reentrant beacuse the structures may contain variable data 
  changing during the execution.
A routine may be compiled either completely or partially - header only. Partially complied routine 
  has category==CATEG_INFO and is used when only the parametr's description is necessary.
Compiled routines and triggers may be put into the psm_cache. When a routine or a trigger is called, 
  the cache is searched first. If found, the routine/trigger is removed from the cache and and executed.
  If not found, the routine/trigger is compiled and executed. 
The compiled routine/trigger is returned to the cache after its execution (on the end of the CALL statement, 
  end of the detached thread, end of trigger execution).
Optimisation: At most one copy of a compiled routine/trigger is preserved in the cache. 
  Any other copy is deleted upon attempt of insertion. Completely compiled routine is preferred over the
  partially compiled one.
Exception: Routines declared as local in other routines or triggers are never compiled separately. 
  Their structures are owned by the local declaration table. They are never inserted into the cache
  nor searched in it.

When compiling CALL statement or implicit call in an expression, the reference to the routine is taken from the table of local declarations
  {rel_ref is FALSE then) or an owning pointer is returned by get_stored_routine (rel_ref is TRUE then).
Routines in cache may be external. They must not be local (with objnum==NOOBJECT).

When routine is invalidated, it is simply removed from the cache. The problem is that it can be returned to the cache by an owner.
*/

#define max_number_of_cached_objects 90

void t_psm_cache::init(void)
{ for (int i=0;  i<PSM_CACHE_SIZE;  i++)
    tab[i]=NULL;  
  InitializeCriticalSection(&cs);
  number_of_objects_in_cache=0;
}

void t_psm_cache::destroy(void)
{ ProtectedByCriticalSection using_cs(&cs);
  for (unsigned i=0;  i<PSM_CACHE_SIZE;  i++)
  { t_cachable_psm * ptr = tab[i], * next;
    while (ptr!=NULL)
    { next=ptr->next_in_list; 
      delete ptr;
      ptr=next;
    }
    tab[i]=0;
  }
  number_of_objects_in_cache=0;
}

t_psm_cache::t_psm_cache(void)
{ init(); }

t_psm_cache::~t_psm_cache(void)
{ DeleteCriticalSection(&cs); }

void t_psm_cache::mark_core(void)
{ for (unsigned i=0;  i<PSM_CACHE_SIZE;  i++)
  { t_cachable_psm * ptr = tab[i];
    while (ptr!=NULL)
      { ptr->mark_core();  ptr=ptr->next_in_list; }
  }
}

t_cachable_psm * t_psm_cache::get(tobjnum objnum, tcateg category)
// Searches the compiled object in the cache. If found, removes and returns it, otherwise returns NULL.
// When routine info is requested, the routine may be returned instead.
{ ProtectedByCriticalSection using_cs(&cs);
  t_cachable_psm * ptr, ** pptr = &tab[hash_fnc(objnum)];
  do
  { ptr = *pptr;
    if (ptr==NULL) break; // not found
    if (ptr->objnum==objnum)
       if (ptr->category==category || ptr->category==CATEG_PROC && category==CATEG_INFO) 
    { *pptr=ptr->next_in_list;
      number_of_objects_in_cache--;
      break;  // found
    }
    pptr=&ptr->next_in_list;
  } while (TRUE);
  return ptr;
}

void t_psm_cache::put(t_cachable_psm * compobj)
// Inserts the compiled routine/trigger into the cache.
{ ProtectedByCriticalSection using_cs(&cs);
  t_cachable_psm ** start = &tab[hash_fnc(compobj->objnum)], ** pptr = start, *ptr = *pptr;
  while (ptr!=NULL)
  { if (ptr->objnum==compobj->objnum) 
      if (ptr->category==compobj->category || ptr->category==CATEG_PROC && compobj->category==CATEG_INFO) 
      { delete compobj;  // found same in the cache: delete the parameter
        return;          // cache remains unchanged
      }
      else if (ptr->category==CATEG_INFO && compobj->category==CATEG_PROC)
      { *pptr=ptr->next_in_list;  number_of_objects_in_cache--;  // remove it from the cache
        delete ptr;      // delete the INFO object 
        break;           // and add the procedure later
      }
    pptr=&ptr->next_in_list;  ptr=*pptr;
  } 
 // check to see if the cache is full, drop the current list if it is
  if (number_of_objects_in_cache>=max_number_of_cached_objects)
  { pptr = start; 
    while (*pptr!=NULL)
    { ptr = *pptr;  number_of_objects_in_cache--;  // remove it from the cache
      *pptr=ptr->next_in_list;
      delete ptr;
    }
  }
 // number_of_objects_in_cache<max_number_of_cached_objects is NOT guaranteed, number_of_objects_in_cache does not grow to much
 // store the object in the cache
  compobj->next_in_list=*start;
  *start=compobj;
  number_of_objects_in_cache++;
}

void t_psm_cache::invalidate(tobjnum objnum, tcateg category)
// Uses the axiom that there is only one instance of the object in the cache
// Uses the axiom that either routine or its info may be in the cache, but not both.
{ 
  if (objnum==NOOBJECT) destroy();
  else if (category==CATEG_PROC)
  { t_routine * rout = get_routine_info(objnum);  // may return routine or routine_info
    if (rout!=NULL) delete rout;
  }
  else
  { t_trigger * trig = get_trigger(objnum);
    if (trig!=NULL) delete trig;
  }
}

t_psm_cache psm_cache;
////////////////////////////////// admin mode store ///////////////////////////////
static objnum_dynar admin_mode_objs;
static uuid_dynar   admin_mode_apls;

static t_amo * load_admin_mode_data(cdp_t cdp, apx_header * apx, tobjnum aplobj)
{ if (tb_read_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, 0, sizeof(apx_header), (tptr)apx)) return NULL;
  if (apx->admin_mode_count > 1000) apx->admin_mode_count=0;  // error in the apx
  t_amo * amos = new t_amo[apx->admin_mode_count+1];  // space for adding another item allocated
  if (!amos) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  tb_read_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, offsetof(apx_header,amo), sizeof(t_amo)*apx->admin_mode_count, (tptr)amos);
  return amos;
}

BOOL is_in_admin_mode(cdp_t cdp, tobjnum objnum, WBUUID schema_uuid)
{ int i;
  for (i=0;  i<admin_mode_objs.count();  i++)
    if (objnum==*admin_mode_objs.acc0(i)) return TRUE;  // object found
  for (i=0;  i<admin_mode_apls.count();  i++)
    if (!memcmp(schema_uuid, *admin_mode_apls.acc0(i), UUID_SIZE)) return FALSE; // schema uuid found, so object is NOT in admin mode
 // load schema info:
  { ProtectedByCriticalSection cs(&cs_medium_term, cdp, WAIT_CS_SHORT_TERM);
    if (null_uuid(schema_uuid)) return FALSE;
    trecnum aplobj;
    if (ker_apl_id2name(cdp, schema_uuid, NULL, &aplobj)) return FALSE;  // error, schema does not exist
    apx_header apx;  memset(&apx, 0, sizeof(apx));
    t_amo * amos = load_admin_mode_data(cdp, &apx, aplobj);
    if (!amos) return FALSE;
    BOOL found = FALSE;
    for (int i=0; i<apx.admin_mode_count; i++)
    { tobjnum objnum2 = find2_object(cdp, amos[i].categ, schema_uuid, amos[i].objname);
      if (objnum2!=NOOBJECT)
      { tobjnum * pobjnum = admin_mode_objs.next();
        if (pobjnum) *pobjnum=objnum2;
        else request_error(cdp, OUT_OF_KERNEL_MEMORY);
        if (objnum2==objnum) found=TRUE;
      }
    }
    delete [] amos;
    WBUUID * uuid = admin_mode_apls.next();
    if (uuid) memcpy(uuid, schema_uuid, UUID_SIZE);
    return found;
  }
}

BOOL set_admin_mode(cdp_t cdp, tobjnum objnum, WBUUID schema_uuid)
{ tobjname name;  tcateg categ;  int i;
  tb_read_atr(cdp, objtab_descr, objnum, OBJ_NAME_ATR,  name);
  tb_read_atr(cdp, objtab_descr, objnum, OBJ_CATEG_ATR, (tptr)&categ);
  trecnum aplobj;
  if (ker_apl_id2name(cdp, schema_uuid, NULL, &aplobj)) return NONEINTEGER;  // error, schema does not exist
  apx_header apx;  memset(&apx, 0, sizeof(apx));
  { ProtectedByCriticalSection cs(&cs_medium_term, cdp, WAIT_CS_SHORT_TERM);
    t_amo * amos = load_admin_mode_data(cdp, &apx, aplobj);
    if (!amos) return NONEINTEGER;
   // search for it:
    for (i=0;  i<apx.admin_mode_count;  i++)
      if (amos[i].categ==categ && !strcmp(amos[i].objname, name))
        return TRUE;  // already is in admin mode
   // not found, add it:
    amos[apx.admin_mode_count].categ=categ;
    strcpy(amos[apx.admin_mode_count].objname, name);
    apx.admin_mode_count++;
    apx.version=CURRENT_APX_VERSION;
    tb_write_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, 0, offsetof(apx_header,amo), (tptr)&apx);
    tb_write_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, offsetof(apx_header,amo), sizeof(t_amo)*apx.admin_mode_count, (tptr)amos);
    delete [] amos;
   // reload:
    admin_mode_objs.clear();
    admin_mode_apls.clear();
  }
  return FALSE;
}

BOOL clear_admin_mode(cdp_t cdp, tobjnum objnum, WBUUID schema_uuid)
{ tobjname name;  tcateg categ;  int i;
  tb_read_atr(cdp, objtab_descr, objnum, OBJ_NAME_ATR,  name);
  tb_read_atr(cdp, objtab_descr, objnum, OBJ_CATEG_ATR, (tptr)&categ);
  trecnum aplobj;
  if (ker_apl_id2name(cdp, schema_uuid, NULL, &aplobj)) return NONEINTEGER;  // error, schema does not exist
  apx_header apx;  memset(&apx, 0, sizeof(apx));
  { ProtectedByCriticalSection cs(&cs_medium_term, cdp, WAIT_CS_SHORT_TERM);
    t_amo * amos = load_admin_mode_data(cdp, &apx, aplobj);
    if (!amos) return NONEINTEGER;
   // search for it:
    for (i=0;  i<apx.admin_mode_count;  i++)
      if (amos[i].categ==categ && !strcmp(amos[i].objname, name))
        break;  // already is in admin mode
    if (i==apx.admin_mode_count) return FALSE;  // was not in admin mode
   // remove it:
    apx.admin_mode_count--;
    amos[i].categ=amos[apx.admin_mode_count].categ;
    strcpy(amos[i].objname, amos[apx.admin_mode_count].objname);
    apx.version=CURRENT_APX_VERSION;
    tb_write_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, 0, offsetof(apx_header,amo), (tptr)&apx);
    tb_write_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, offsetof(apx_header,amo), sizeof(t_amo)*apx.admin_mode_count, (tptr)amos);
    delete [] amos;
   // reload:
    admin_mode_objs.clear();
    admin_mode_apls.clear();
  }
  return TRUE;
}

void mark_admin_mode_list(void)
{ admin_mode_objs.mark_core();  admin_mode_apls.mark_core(); }
/////////////////////////////////////////////////////////////////////////////
static void WINAPI routine_compile_entry(CIp_t CI)
{ t_routine * rout = (t_routine *)CI->stmt_ptr;
  if (CI->cursym==SQ_DECLARE) next_sym(CI);
  if (CI->cursym!=SQ_PROCEDURE && CI->cursym!=SQ_FUNCTION) c_error(PROCEDURE_EXPECTED);
  symbol sym1 = CI->cursym;  tobjname name, schema;
  get_schema_and_name(CI, name, schema);
  rout->compile_routine(CI, sym1, name, schema);
}

t_routine * get_stored_routine(cdp_t cdp, tobjnum objnum, t_zcr * required_version, BOOL complete)
// Returns owning pointer to the routine. Routine is either taken from the cache or compiled.
// On error calls request_error and returns NULL.
{// trying to find it in the cache:
  t_routine * rout = complete==TRUE ? psm_cache.get_routine(objnum) : psm_cache.get_routine_info(objnum);
  if (rout!=NULL) 
  { if (required_version!=NULL)
    { if (!memcmp(rout->version, *required_version, ZCR_SIZE))
        return rout;
    }
    else // otherwise requiring the current version:
    { t_zcr curr_version;
      tb_read_atr(cdp, objtab_descr, objnum, objtab_descr->zcr_attr, (tptr)curr_version);
      if (!memcmp(rout->version, curr_version, ZCR_SIZE))
        return rout;
    }
    delete rout;  
  }
 // load source and compile:
  tptr src=ker_load_objdef(cdp, objtab_descr, objnum);
  if (src==NULL) return NULL; 
  rout = new t_routine(complete==TRUE ? CATEG_PROC : CATEG_INFO, objnum);
  if (rout==NULL)
    { corefree(src);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
 // find the application kontext of the routine and set it:
  WBUUID uuid; // application uuid
  tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)uuid);
  t_short_term_schema_context stsc(cdp, uuid);
 // compile:
  compil_info xCI(cdp, src, routine_compile_entry);  int err;
  xCI.stmt_ptr=rout;
  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  sql_kont.compiled_psm=objnum;
  err = complete==2 ? compile_masked(&xCI) : compile(&xCI); // prevents propagating compilation error in metadata views
  corefree(src);
  if (err)  // compilation error
  { if (rout!=NULL) delete rout;  
    if (complete!=2) make_exception_from_error(cdp, err);  // this is necessary to break the execution of a procedure calling another procedure which cannot be compiled: compile does not call the request_error()!
    return NULL; 
  }
  if (rout) // store the object number and version
  { tb_read_atr(cdp, objtab_descr, objnum, objtab_descr->zcr_attr, (tptr)rout->version);
    if (required_version && memcmp(rout->version, *required_version, ZCR_SIZE))
      { request_error(cdp, OBJECT_VERSION_NOT_AVAILABLE);  delete rout;  return NULL; }
  }
  return rout;
}

#include "flstr.h"
#include "sequence.cpp"
#include "ftanal.cpp"
