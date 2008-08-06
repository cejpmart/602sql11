/****************************************************************************/
/* The S-Pascal compiler - statement part - comp1.c                         */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#if WBVERS<900
#include "wprez.h"
#endif
#include <stddef.h>

static void declarations(CIp_t CI, BOOL outmost, unsigned * loc_ordr,
                                              unsigned * loc_offset);
static object * newtype(CIp_t CI, BOOL ref_par_type);
//extern objtable standard_table;  /* the type is different, in fakt */

void new_id_table(CIp_t CI) /* adds new empty table for local objects */
{ objtable * tb;
  tb=(objtable*)comp_alloc(CI,
      sizeof(objtable)+(START_LOCAL_IDS-1)*sizeof(objdef));
  tb->objnum=0;
  tb->tabsize=START_LOCAL_IDS;
  tb->next_table=CI->id_tables;
  CI->id_tables=tb;
}

BOOL is_curr_id(CIp_t CI, char * name)
{ if (CI->cursym!=S_IDENT) return FALSE;
  return !strcmp(CI->curr_name, name);
}

static void add_object(CIp_t CI, char * name, object * descr, objtable ** ptab)
/* adds an object to the an array, checks if the object is not present */
{ int ind;  objtable * tab = *ptab;
//  if (CI->id_tables==&standard_table)
//    c_error(ID_DECLARED_TWICE);  /* this is a different error */
  if (!search_table(name,(tptr)tab->objects,tab->objnum,sizeof(objdef),&ind))
    { SET_ERR_MSG(CI->cdp, name);  c_error(ID_DECLARED_TWICE); }
  if (tab->objnum==tab->tabsize)
    tab=adjust_table(CI, tab, tab->tabsize+NEXT_LOCAL_IDS);
  if (ind<tab->objnum) memmov(tab->objects+ind+1, tab->objects+ind,
                              (tab->objnum-ind)*sizeof(objdef));
  strmaxcpy(tab->objects[ind].name, name, NAMELEN+1);
  tab->objects[ind].descr=descr;
  tab->objnum++;
  *ptab=tab;
}

/************** variable, record item & parameter definition ****************/
static void list_and_type(CIp_t CI, uns8 category, unsigned * loc_ordr, unsigned * loc_offset)
{ object * ob, * lastobject;  typeobj * mytype;  unsigned valsize;
  unsigned varcnt, my_offset;  BOOL is_multi;
  lastobject=NULL;  varcnt=0;
  do
  { if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
    ob=(object*)comp_alloc(CI, sizeof(varobj));
    ob->var.categ=category;
    ob->var.ordr=(uns8)((*loc_ordr)++);
    ob->var.type=(typeobj*)lastobject; lastobject=ob;
    add_object(CI, CI->curr_name, ob, &CI->id_tables);
    varcnt++;
    if (next_sym(CI)==',') next_sym(CI); else break;
  } while (TRUE);

  if (CI->cursym==':')
  { next_sym(CI);
    is_multi=is_curr_id(CI, "MULTI");
    if (is_multi)
    { next_sym(CI);
      if (category!=O_RECELEM) c_error(TYPE_EXPECTED);
    }

    mytype=&(newtype(CI, category==O_REFPAR)->type);
    valsize=ipj_typesize(mytype);
    if (category==O_VALPAR)  /* round to a stack element boundary */
      valsize=((valsize-1)/sizeof(void*)+1)*sizeof(void*);  /* round to a word/dword boundary */
    //if (category!=O_REFPAR) // must not be abstract object for VALPAR, VAR, RECELEM
    //  if (mytype->type.anyt.typecateg==T_VIEW &&
    //      !*mytype->type.view.viewname)
    //    c_error(ABSTRACT_CLASS_INSTANCE);
    if (is_multi)  /* type & size changes on "MULTI" */
    { valsize=0;  /* variable not accessible unless from database */
      if (mytype->type.anyt.typecateg==T_SIMPLE)
      { if (mytype==&att_file) c_error(TYPE_EXPECTED);
      }
      else  /* COMPOSED type */
        if (!CHAR_ARR(mytype)) c_error(TYPE_EXPECTED);
    }
  }
  else // : missing
  { if (category!=O_REFPAR) c_error(COMMA_OR_SEMIC_EXPECTED);
    mytype=&att_nil;  is_multi=false;
  }

 /* write the type information, allocate space for variables */
  if (category==O_REFPAR) valsize=sizeof(void*);
  *loc_offset+=varcnt*valsize;  my_offset=*loc_offset;
  do
  { ob=lastobject;  lastobject=(object*)ob->var.type;
    ob->var.type=mytype;
    my_offset-=valsize;
    ob->var.offset=my_offset;
    ob->var.multielem=is_multi;
  } while (lastobject);
}

sig32 num_val(CIp_t CI, sig32 lbd, sig32 hbd)
{ sig32 val=0;  BOOL minus;  uns8 obj_level;  object * obj;
  short oper = '+';
  do
  { if (CI->cursym=='-')
    { minus=TRUE;  next_sym(CI); }
    else
    { if (CI->cursym=='+') next_sym(CI);
      minus=FALSE;
    }
    if (CI->cursym!=S_INTEGER)
    { if (CI->cursym==S_IDENT)
      { obj=search_obj(CI, CI->curr_name,&obj_level);
        if (obj)  /* program-defined identifier */
          if (obj->any.categ==O_CONST)
          { if (obj->cons.type==&att_int32)
              { CI->curr_int=obj->cons.value.int32;  goto is_const; }
            if (obj->cons.type==&att_int16)
            { CI->curr_int = (obj->cons.value.int16==(sig16)0x8000) ? NONEINTEGER :
                obj->cons.value.int16;
              goto is_const;
            }
          }
      }
      c_error(INTEGER_EXPECTED);
    }
   is_const:
    if (minus) CI->curr_int=-CI->curr_int;
    if      (oper=='|') val |= (sig32)CI->curr_int;
    else if (oper=='&') val &= (sig32)CI->curr_int;
    else if (oper=='+') val += (sig32)CI->curr_int;
    else                val -= (sig32)CI->curr_int;
    oper=next_sym(CI);
    if ((oper != '|') && (oper != '&') && (oper != '+') && (oper != '-'))
      break;
    next_sym(CI);
  } while (TRUE);
  if (val<lbd || val>hbd) c_error(INT_OUT_OF_BOUND);
  return val;
}

/************************* type definition **********************************/
static object * newtype(CIp_t CI, BOOL ref_par_type)
{ typeobj * td;
  switch (CI->cursym)
  { case S_IDENT:
    { object * curr_obj;  uns8 obj_level;
      if (is_curr_id(CI, "FLEXIBLE"))
        { next_sym(CI);  return (object*)&att_nospec; }
      curr_obj=search_obj(CI, CI->curr_name, &obj_level);
      if (!curr_obj) c_error(TYPE_EXPECTED);
      next_sym(CI);
      if (curr_obj->any.categ==O_TYPE) return curr_obj;
      c_error(TYPE_EXPECTED);
    }
    case S_FILE:
      next_sym(CI);
      return (object*)&att_file;
    case S_ARRAY:
    { sig32 low_limit, high_limit;  object * elemtype;  
      next_and_test(CI,'[',LEFT_BRACE_EXPECTED);
      next_sym(CI);
      low_limit =num_val(CI, -0x7fffffffL, 0x7fffffff);
      test_and_next(CI,S_DDOT,DOUBLE_DOT_EXPECTED);
      high_limit=num_val(CI, -0x7fffffffL, 0x7fffffff);
      if (high_limit < low_limit) c_error(ARRAY_BOUNDS);
      test_and_next(CI,']',RIGHT_BRACE_EXPECTED);
      test_and_next(CI,S_OF, OF_EXPECTED);
      elemtype=newtype(CI, FALSE);
      td=(typeobj *)comp_alloc(CI, 1+sizeof(arraytype));   /* categ + typedescr */
      td->categ=O_TYPE;
      td->type.array.typecateg=T_ARRAY;
      td->type.array.lowerbound=low_limit;
      td->type.array.elemtype=&(elemtype->type);
      td->type.array.elemcount=high_limit-low_limit+1;
      td->type.array.valsize=td->type.array.elemcount * elemtype->type.type.anyt.valsize;
      break;
    }
    case S_STRNG:  case S_CSSTRNG:  case S_CSISTRNG:
    { int charset = CI->cursym==S_CSSTRNG || CI->cursym==S_CSISTRNG ? 1 : 0;
      BOOL ignore_case = CI->cursym==S_CSISTRNG;
      if (next_sym(CI)=='[')
      { td=(typeobj *)comp_alloc(CI, sizeof(stringtype)+2);   /* categ + typedescr, is packed */
        td->categ = O_TYPE;
        td->type.string.typecateg = T_STR;
        next_sym(CI);
        int length = num_val(CI, 1, 0x7ffff);
        if (CI->cursym!=']') c_error(RIGHT_BRACE_EXPECTED);
        next_sym(CI);
        if (CI->cursym==SC_COLLATE)
        { next_sym(CI);
          if      (CI->cursym==S_CSSTRNG)  { charset=1;  ignore_case=FALSE; }
          else if (CI->cursym==S_CSISTRNG) { charset=1;  ignore_case=TRUE; }
          else
          { if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
            charset = find_collation_name(CI->curr_name);
            if (charset<0) c_error(IDENT_NOT_DEFINED);
          }
          next_sym(CI);
        }
        if (CI->cursym==S_IDENT && !strcmp(CI->curr_name,"IGNORE_CASE")) 
          { ignore_case=TRUE;  next_sym(CI); }
        td->type.string.specif=t_specif(length, charset, ignore_case, FALSE).opqval;
        td->type.string.valsize=length+1;
      }
      else if (ref_par_type)
        return (object*)&att_string;
      else c_error(LEFT_BRACE_EXPECTED);
      break;
    }
    case S_BINARY:
      if (next_sym(CI)=='[')
      { td=(typeobj *)comp_alloc(CI, sizeof(binarytype)+2);   /* categ + typedescr */
        td->categ=O_TYPE;
        td->type.string.typecateg=T_BINARY;
        next_sym(CI);
        td->type.string.valsize=num_val(CI, 1, 0xff);
        if (CI->cursym!=']') c_error(RIGHT_BRACE_EXPECTED);
        next_sym(CI);
      }
      else if (ref_par_type) 
        return (object*)&att_binary;
      else c_error(LEFT_BRACE_EXPECTED);
      break;
    case S_RECORD:
    { unsigned field_offset, field_ordr;
      next_sym(CI);
      new_id_table(CI);
      field_ordr=0;  field_offset=0;
      while (CI->cursym!=S_END)
      { list_and_type(CI, O_RECELEM, &field_ordr, &field_offset);
        if (CI->cursym==';') next_sym(CI);
        else if (CI->cursym!=S_END) c_error(END_OR_SEMICOLON_EXPECTED);
      }
      next_sym(CI);
      CI->id_tables=adjust_table(CI, CI->id_tables, CI->id_tables->objnum);  /* before comp_alloc */
      td=(typeobj *)comp_alloc(CI, 1+sizeof(recordtype));   /* categ + typedescr */
      td->categ=O_TYPE;
      td->type.record.typecateg=T_RECORD;
      td->type.record.valsize=field_offset;
      td->type.record.items=CI->id_tables;
      CI->id_tables=CI->id_tables->next_table;
      break;
    }
    case S_CURSOR:
      td=(typeobj *)comp_alloc(CI, 1+sizeof(recordtype));   /* categ + typedescr */
      td->categ=O_TYPE;
      td->type.record.typecateg=T_RECCUR;
      td->type.record.valsize=3*sizeof(tobjnum);  /* for possible subcursors */
      next_sym(CI);
      if ((CI->cursym==S_IDENT) || (CI->cursym==S_RECORD))
      { typeobj * subtype = &newtype(CI, FALSE)->type;
        if (DIRECT(subtype) || (subtype->type.anyt.typecateg!=T_RECORD))
          c_error(NOT_A_REC);
        td->type.record.items=subtype->type.record.items;
      }
      else td->type.record.items=NULL;
      break;
#if WBVERS<900
    case S_FORM:
      td=(typeobj *)comp_alloc(CI, 1+sizeof(viewtype));   /* categ + typedescr */
      td->categ=O_TYPE;
      td->type.view.typecateg=T_VIEW;
      td->type.view.valsize=sizeof(view_dyn);
      if (next_sym(CI) == S_IDENT)
      { strcpy(td->type.view.viewname, CI->curr_name);
        cd_Find_object(CI->cdp, td->type.view.viewname, CATEG_VIEW, &td->type.view.objnum);
        // result used when compiling access to view controls
        next_sym(CI);
      }
      else 
      { td->type.view.viewname[0]=0;
        td->type.view.objnum=NOOBJECT;
      }
      break;
#endif
    case '^':
    { uns8 obj_level;
      next_sym(CI);
      td=(typeobj *)comp_alloc(CI, 1+sizeof(ptrtype));   /* categ + typedescr */
      if (CI->cursym==S_IDENT && !search_obj(CI, CI->curr_name, &obj_level))
      { td=(typeobj *)comp_alloc(CI, 1+sizeof(fwdptrtype));   /* categ + typedescr */
        td->type.fwdptr.domaintype=NULL;
        td->type.fwdptr.typecateg=T_PTRFWD;
        strcpy(td->type.fwdptr.domainname, CI->curr_name);
        next_sym(CI);
      }
      else
      { td=(typeobj *)comp_alloc(CI, 1+sizeof(ptrtype));   /* categ + typedescr */
        object * elemtype=newtype(CI, FALSE);  /* the domain type */
        td->type.ptr.domaintype=&(elemtype->type);
        td->type.ptr.typecateg=T_PTR;
      }
      td->categ=O_TYPE;
      td->type.anyt.valsize=sizeof(void*);
      break;
    }
    default: c_error(TYPE_EXPECTED);
  }
  return (object*)td;
}

/* The following procedure is called with CI->cursym==S_TYPE, returns with the
   first symbol after types definition in CI->cursym */
static void typedefs(CIp_t CI)
{ tobjname name;  object * obj;
  next_and_test(CI, S_IDENT,IDENT_EXPECTED);
  do
  { strcpy(name, CI->curr_name);
    next_and_test(CI,'=',EQUAL_EXPECTED);
    next_sym(CI);
    obj=newtype(CI, FALSE);
    add_object(CI, name, obj, &CI->id_tables);
    test_and_next(CI,';', SEMICOLON_EXPECTED);
  } while (CI->cursym==S_IDENT);
}

/*************************** subroutine *************************************/
CFNC DllKernel void WINAPI exec_constructors(cdp_t cdp, BOOL constructing)
// Executes constructor or desctructor calls for all GLOBAL objects.
//  (these calls are not included in the program code)
// Called when setting up or releasing the project.
{ if (cdp->RV.run_project.proj_decls==NULL || cdp->RV.glob_vars->vars==NULL) return;
 // clear variables (untyped vars & others need this):
  if (constructing)
    memset(cdp->RV.glob_vars, 0, cdp->RV.run_project.common_proj_code->proj_vars_size+sizeof(scopevars));  
 // construct objects:
  objtable * id_tab = cdp->RV.run_project.global_decls;
  for (int i=0;  i<id_tab->objnum;  i++)
  { objdef * objd = &id_tab->objects[i];
    if (objd->descr->any.categ==O_VAR)
      if (objd->descr->var.type->type.anyt.typecateg==T_VIEW)
      { tptr p = cdp->RV.glob_vars->vars + objd->descr->var.offset;
#ifdef WINS
#if WBVERS<900
        if (constructing)
          construct_object(cdp, p, objd->descr->var.type->type.view.viewname);
        else destruct_object(cdp, p);
#endif
#endif
      }
  }
}

static void constructor_calls(CIp_t CI, BOOL constructing)
// Generates constructor or desctructor calls for all LOCAL forms in the subroutine.
{ for (int i=0;  i<CI->id_tables->objnum;  i++)
  { objdef * objd = &CI->id_tables->objects[i];
    if (objd->descr->any.categ==O_VAR)
      if (objd->descr->var.type->type.anyt.typecateg==T_VIEW)
      { if (objd->descr->var.offset>0xffff)
          { gen(CI,I_LOADADR4); gen(CI,0);  gen4(CI,objd->descr->var.offset); }
        else
          { gen(CI,I_LOADADR);  gen(CI,0);  gen2(CI,objd->descr->var.offset); }
        if (constructing)
        { gen(CI, I_CONSTRUCT);
          genstr(CI, objd->descr->var.type->type.view.viewname);
        }
        else gen(CI, I_DESTRUCT);
      }
  }
}

void reorder_decls(CIp_t CI, unsigned total_vars, unsigned total_alloc)
//#ifdef WINS   /* stdcall */
{}
#ifdef STOP
//#else
{ for (unsigned ordr=0;  ordr<total_vars;  ordr++)
  { objdef * od=CI->id_tables->objects;
    int i=0;
    while (i < CI->id_tables->objnum)
    { uns8 cl=od->descr->any.categ;
      if (cl==O_VALPAR || cl==O_REFPAR || cl==O_VAR)
      if (od->descr->var.ordr==ordr) /* found */
      {/* update offset: */
	      unsigned valsize=(cl==O_REFPAR) ? sizeof(void*) : ipj_typesize(od->descr->var.type);
        if (cl==O_VALPAR) valsize=((valsize-1)/sizeof(int)+1)*sizeof(int);  /* round to a word/dword boundary */
	      total_alloc-=valsize;
	      od->descr->var.offset=total_alloc;
	      break;
      }
      i++;  od++;
    }
  }
}
#endif

void subr_def(CIp_t CI)
{ typeobj * ftype;  object * ob;  unsigned valsize, subr_ordr, subr_offset;
  tobjname subrname, case_subrname, retval_name;  
  BOOL continuing_forward=FALSE;

  BOOL funct = CI->cursym==S_FUNCTION;
  next_and_test(CI,S_IDENT, IDENT_EXPECTED);
  subrobj * subr=(subrobj*)comp_alloc(CI, sizeof(subrobj));
  subr->categ = funct ? O_FUNCT : O_PROC;
  strcpy(subrname, CI->curr_name);  strcpy(case_subrname, CI->name_with_case);

 // check for FORWARD continuation:
  object * curr_obj;  uns8 obj_level;
  curr_obj=search_obj(CI, CI->curr_name, &obj_level);
  if (curr_obj!=NULL && (funct ? curr_obj->subr.categ==O_FUNCT :
                                 curr_obj->subr.categ==O_PROC))
  { next_sym(CI);
    test_and_next(CI,';',SEMICOLON_EXPECTED);
   // restore info:
    corefree(subr);
    subr=&curr_obj->subr;
    subr_offset=subr->localsize;
    subr_ordr=subr->saved_ordr-1;
    subr->locals->next_table=CI->id_tables;
    CI->id_tables=subr->locals;
   // mark function as completed:
    subr->saved_ordr=0;
   // store the link to the real entry point:
    setcadr4(CI, subr->code+1);
   // change the entry address:
    subr->code=CI->code_offset;
    if (funct)
    {// re-create retval name:
      strcpy(retval_name, subrname);
      if (strlen(retval_name) < NAMELEN) strcat(retval_name, "#");
      else retval_name[NAMELEN-1]='#';
     // find the retval object, restore valsize:
      ob=search_obj(CI, retval_name, &obj_level);
      valsize=ipj_typesize(ob->var.type);
     // restore the converted retval type in ftype:
      ftype=subr->retvaltype;
      if (!DIRECT(ftype))
        if (ftype->type.anyt.typecateg==T_PTR || ftype->type.anyt.typecateg==T_PTRFWD)
          ftype=&att_int32;  // used when assigning the result
        else 
          ftype=simple_types[TtoATT(ftype)];  // ...I do not understand the reason
    }
    continuing_forward=TRUE;
    goto contin;
  }
  else

  { add_object(CI, CI->curr_name, (object*)subr, &CI->id_tables);
   new_id_table(CI);   /* just entered a subroutine */
    subr_ordr=0;  subr_offset=0;
    subr->locals=CI->id_tables; /* the new table */
    subr->saved_ordr=0;  // says "this is not a forward"

    if (next_sym(CI)=='(')
    { if (next_sym(CI) != ')')
        do
        { if (CI->cursym==S_VAR)
          { next_sym(CI);
            list_and_type(CI, O_REFPAR, &subr_ordr, &subr_offset);
          }
          else list_and_type(CI, O_VALPAR, &subr_ordr, &subr_offset);
          if (CI->cursym == ')') break;
          test_and_next(CI,';', SEMICOLON_EXPECTED);
        } while (TRUE);
      next_sym(CI);
    }
    if (funct)
    { test_and_next(CI,':',COLON_EXPECTED);
      subr->retvaltype=ftype=&newtype(CI, FALSE)->type;
      if (!DIRECT(ftype))
      { if (ftype->type.anyt.typecateg==T_PTR || ftype->type.anyt.typecateg==T_PTRFWD)
          ftype=&att_int32;  // used when assigning the result
        else 
        { if (!CHAR_ARR(ftype)) c_error(BAD_FUNCTION_TYPE);
          ftype=simple_types[TtoATT(ftype)];  // ...I do not understand the reason
        }
      }
     /* allocating space for the return value: */
      ob=(object*)comp_alloc(CI, sizeof(varobj));    /* fictive RETVAL-object */
      ob->var.categ=O_VAR;
      ob->var.type=subr->retvaltype;  /* must not use ftype, otherwise cannot assign the return value to RETVAL-object */
      ob->var.ordr=(uns8)(subr_ordr++);
      valsize=ipj_typesize(ob->var.type);
      if ((uns32)subr_offset+valsize >= 65520L)
        c_error(TOO_MANY_ALLOC);  /* params must not be bigger even in 32-bit (due to return value offset) */
     /* create retval_name: */
      strcpy(retval_name, subrname);
      if (strlen(retval_name) < NAMELEN) strcat(retval_name, "#");
      else retval_name[NAMELEN-1]='#';
    } else subr->retvaltype=NULL;
    test_and_next(CI,';',SEMICOLON_EXPECTED);

    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "EXTERNAL"))
    { next_sym(CI);
      if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
      reorder_decls(CI, subr_ordr, subr_offset);
      subr->localsize=subr_offset;
      subr->code=CI->code_offset;
      gen(CI,I_DLLCALL);
      dllprocdef dlldef;
      dlldef.procadr=NULL;
      dlldef.parsize=subr_offset;
      dlldef.fnctype=(uns8)(funct ? ATT_TYPE(ftype) : 0);
      code_out(CI, (tptr)&dlldef, CI->code_offset, offsetof(dllprocdef, names));  
      CI->code_offset+=offsetof(dllprocdef, names);

      char * p;  char shortname[8+1+3+1];
      p = CI->curr_string();
      int len=(int)strlen(CI->curr_string());
      if (len <= 8)
      { strcpy(shortname, CI->curr_string());  p=shortname;
        if ((!len    || (shortname[len-1]!='.')) &&
            ((len<2) || (shortname[len-2]!='.')) &&
            ((len<3) || (shortname[len-3]!='.')) &&
            ((len<4) || (shortname[len-4]!='.')))
        strcat(shortname, ".DLL"); /* add default suffix if dot not present */
      }
#ifdef WINS   // translate sys library names
      if (!stricmp(CI->curr_string(), "USER.DLL")   || !stricmp(CI->curr_string(), "USER.EXE") ||
          !stricmp(CI->curr_string(), "USER"))
        { strcpy(shortname, "USER32.DLL");     p=shortname; }
      if (!stricmp(CI->curr_string(), "GDI.DLL")    || !stricmp(CI->curr_string(), "GDI.EXE") ||
          !stricmp(CI->curr_string(), "GDI"))
        { strcpy(shortname, "GDI32.DLL");      p=shortname; }
      if (!stricmp(CI->curr_string(), "KRNL386.DLL")|| !stricmp(CI->curr_string(), "KRNL386.EXE") ||
          !stricmp(CI->curr_string(), "KERNEL.DLL") || !stricmp(CI->curr_string(), "KERNEL.EXE")  ||
          !stricmp(CI->curr_string(), "KERNEL")     || !stricmp(CI->curr_string(), "KRNL386"))
        { strcpy(shortname, "KERNEL32.DLL");   p=shortname; }
      if (!stricmp(CI->curr_string(), "SHELL.DLL")  || !stricmp(CI->curr_string(), "SHELL.EXE") ||
          !stricmp(CI->curr_string(), "SHELL"))
        { strcpy(shortname, "SHELL32.DLL");    p=shortname; }
#endif
      while (*p) gen(CI, *(p++));  gen(CI,0);
      next_sym(CI);
      if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "NAME"))
      { next_sym(CI);
        if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
        p=CI->curr_string();
        while (*p) gen(CI, *(p++));
        next_sym(CI);
      }
      else 
      { p=case_subrname;
        while (*p) gen(CI, *(p++));
      }
      gen(CI,0);  gen(CI,0);   /* 2nd null used by WINS when adding 'A' */
    }
    else
    { if (funct)   /* add variable for the function value */
      { add_object(CI, retval_name, ob, &CI->id_tables);
        ob->var.offset=subr_offset;  subr_offset+=valsize;
      }

      if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "FORWARD"))
      { next_sym(CI);
        subr->localsize=subr_offset;  // save parameter size
        subr->saved_ordr=subr_ordr+1;   // save parameter #, FORWARD flag
        subr->locals=CI->id_tables;   // save table of locals (may have been overwr.)
        subr->code=gen_forward_jump4(CI, I_JUMP4)-1;   // entry point address
      }
      else   /* normal proc/funct */
      {contin:
        declarations(CI, FALSE, &subr_ordr, &subr_offset);
        reorder_decls(CI, subr_ordr, subr_offset);
        subr->localsize=subr_offset;
        subr->code=CI->code_offset;
        subr->locals=CI->id_tables;   /* necessary for recursive procs, table may have been reallocated */
        if (continuing_forward)
        { gen(CI, I_REALLOC_FRAME);
          gen4(CI, subr_offset);
        }
        constructor_calls(CI, TRUE);
        compound_statement(CI);
        constructor_calls(CI, FALSE);
        if (funct)
        { gen(CI,I_FUNCTRET); gen2(CI,ob->var.offset);
          gen(CI,(uns8)(STR_ARR(ftype) ? 0 : valsize));
        }
        else gen(CI,I_PROCRET);
      }
    }
   subr->locals=CI->id_tables=adjust_table(CI, CI->id_tables, CI->id_tables->objnum);
    CI->id_tables=CI->id_tables->next_table;
  }
}

/************************* table & cursor definition ************************/
static void tabcursdef(CIp_t CI, uns8 cat)
{ tobjnum objnum;  object * ob;  
  do
  { next_and_test(CI,S_IDENT, IDENT_EXPECTED);
    if (cd_Find_object(CI->cdp, CI->curr_name, cat, &objnum))
    { SET_ERR_MSG(CI->cdp, CI->curr_name);
      c_error((cat==CATEG_TABLE) ? TABLE_DOES_NOT_EXIST : CURSOR_DOES_NOT_EXIST);
    }
    ob=(object*)comp_alloc(CI, sizeof(cursobj));
    ob->curs.categ=(cat==CATEG_TABLE) ? O_TABLE : O_CURSOR;
    ob->curs.defnum=objnum;
    ob->curs.offset=CI->code_offset;
    ob->curs.type=NULL;  /* ADDD_object needs it */
    add_object(CI, CI->curr_name, ob, &CI->id_tables);
    for (int i=0;  i<OBJ_NAME_LEN;  i++) gen(CI,CI->curr_name[i]);
    gen(CI,0);
    gen4(CI, (unsigned)NOCURSOR);  /* prevents using the cursor before openning */
    if (cat==CATEG_CURSOR)
    { gen4(CI, (unsigned)NOOBJECT);  /* cursor definition number */
      gen4(CI, (unsigned)NOCURSOR);  /* the supercursor number */
    }
  } while (next_sym(CI)==',');
  test_and_next(CI,';',SEMICOLON_EXPECTED);
}
/***************************** constant definition **************************/
static void constdefs(CIp_t CI)  /* CI->cursym==S_CONST when called */
{ tobjname name;  object * obj;  BOOL minus;
  next_and_test(CI,S_IDENT,IDENT_EXPECTED);
  do
  { strcpy(name,CI->curr_name);
    next_and_test(CI,'=',EQUAL_EXPECTED);
    obj=(object*)comp_alloc(CI, sizeof(constobj));
    obj->cons.categ=O_CONST;
    if (next_sym(CI)=='-') { minus=TRUE;  next_sym(CI); }
    else minus=FALSE;
    switch (CI->cursym)
    { case S_INTEGER:
        obj->cons.type=&att_int32;
        obj->cons.value.int32 = (sig32)(minus ? -CI->curr_int : CI->curr_int);
        break;
      case S_REAL:  /* cannot start with '-' */
        obj->cons.type=&att_float;
        obj->cons.value.real = minus ? -CI->curr_real : CI->curr_real;
        break;
      case S_CHAR:
        obj->cons.type=&att_char;
        obj->cons.value.int32=(sig32)(uns8)CI->curr_string()[0];
        if (minus) c_error(INCOMPATIBLE_TYPES);
        break;
      case S_DATE:
        obj->cons.type=&att_date;
        obj->cons.value.int32=(uns32)CI->curr_int;
        if (minus) c_error(INCOMPATIBLE_TYPES);
        break;
      case S_TIME:
        obj->cons.type=&att_time;
        obj->cons.value.int32=(uns32)CI->curr_int;
        if (minus) c_error(INCOMPATIBLE_TYPES);
        break;
      case S_MONEY:
        obj->cons.type=&att_money;
        memcpy(&obj->cons.value.moneys, &CI->curr_money, MONEY_SIZE);
        if (minus) money_neg(&obj->cons.value.moneys);
        break;
      case S_IDENT:
      { uns8 obj_level;
        object * objx=search_obj(CI, CI->curr_name, &obj_level);
        if (objx==NULL)  /* not def. in pgm nor in kontext */
          { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); }
        if (objx->any.categ!=O_CONST)  /* standard type or not const */
          { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(BAD_IDENT_CLASS); }
        obj->cons.type=objx->cons.type;
        if (obj->cons.type==&att_float)
          obj->cons.value.real = minus ? -objx->cons.value.real : objx->cons.value.real;
        else if (obj->cons.type==&att_money)
        { memcpy(&obj->cons.value.moneys, &objx->cons.value.moneys, MONEY_SIZE);
          if (minus) money_neg(&obj->cons.value.moneys);
        }
        else
        { obj->cons.value.int32 = minus ? -objx->cons.value.int32 : objx->cons.value.int32;
          if (minus && objx->cons.type!=&att_int32) c_error(INCOMPATIBLE_TYPES);
        }
        break;
      }
      default: c_error(INCOMPATIBLE_TYPES);
    }
    add_object(CI, name, obj, &CI->id_tables);
    next_sym(CI);
    test_and_next(CI,';', SEMICOLON_EXPECTED);
  } while (CI->cursym==S_IDENT);
}
/*************************** block ******************************************/
static BOOL close_forward_ptrs(CIp_t CI)
{ BOOL still_open=FALSE;
  for (int i=0;  i<CI->id_tables->objnum;  i++)
  { objdef * objd = &CI->id_tables->objects[i];
    if (objd->descr->any.categ==O_TYPE)
      if (objd->descr->type.type.anyt.typecateg==T_PTRFWD)
      { typeobj * td = &objd->descr->type;
        uns8 obj_level;
        object * ob = search_obj(CI, td->type.fwdptr.domainname, &obj_level);
        if (ob==NULL || ob->any.categ!=O_TYPE) still_open=TRUE;
        else td->type.fwdptr.domaintype=&ob->type;
      }
  }
  return still_open;
}

static void declarations(CIp_t CI, BOOL outmost, unsigned * loc_ordr, unsigned * loc_offset)
{ do
  { if      (CI->cursym==S_TYPE) typedefs(CI);
    else if (CI->cursym==S_CONST) constdefs(CI);
    else if (CI->cursym==S_VAR)
    { next_sym(CI);
      do
      { list_and_type(CI, O_VAR, loc_ordr, loc_offset);
        test_and_next(CI,';', SEMICOLON_EXPECTED);
      } while (CI->cursym==S_IDENT);
    }
    else if (outmost)
    {      if (CI->cursym==S_CURSOR) tabcursdef(CI, CATEG_CURSOR);
      else if (CI->cursym==S_TABLE)  tabcursdef(CI, CATEG_TABLE);
      else if ((CI->cursym==S_PROCEDURE)||(CI->cursym==S_FUNCTION))
      { close_forward_ptrs(CI);
        subr_def(CI);
        test_and_next(CI,';',SEMICOLON_EXPECTED);
      }
      else break;
    }
    else break;
  } while (TRUE);
  if (CI->cursym!=S_BEGIN) c_error(BEGIN_EXPECTED);
 // check if all FORWARDs are closed:
  for (int i=0;  i<CI->id_tables->objnum;  i++)
  { objdef * objd = &CI->id_tables->objects[i];
    if (objd->descr->any.categ==O_FUNCT || objd->descr->any.categ==O_PROC)
      if (objd->descr->subr.saved_ordr)
        c_error(FORWARD_OPEN);
    if (objd->descr->any.categ==O_TYPE)
      if (objd->descr->type.type.anyt.typecateg==T_PTRFWD)
      { typeobj * td = &objd->descr->type;
        uns8 obj_level;
        object * ob = search_obj(CI, td->type.fwdptr.domainname, &obj_level);
        if (ob==NULL || ob->any.categ!=O_TYPE) c_error(PTR_OPEN);
        td->type.fwdptr.domaintype=&ob->type;
      }
  }
}

/************************ program & project *********************************/

static uns8 objdefsize[] = { 0, sizeof(constobj), sizeof(typeobj),
 sizeof(subrobj), sizeof(procobj), sizeof(subrobj),
 sizeof(varobj),  sizeof(varobj),  sizeof(varobj),  sizeof(varobj),
 0,               sizeof(cursobj), sizeof(aggrobj), sizeof(cursobj),
 sizeof(methobj), sizeof(propobj) };
static uns32 add_objtable(CIp_t CI, objtable * ot, uns32 * logoffs);

static void out_decl(CIp_t CI, tptr info, unsigned size, uns32 * logoffs)
{ for (unsigned i=0; i<size; i++)
  { gen(CI,info[i]);
    if ((uns8)info[i]==RELOC_MARK) { gen2(CI,0xffff);  gen(CI,0xff); }
  }
  *logoffs+=size;
}

static void gen_rel_addr(CIp_t CI, uns32 ladr)
{ gen(CI, RELOC_MARK);  gen2(CI, (uns16)(ladr & 0xffff));  gen(CI, (uns8)(ladr>>16)); }

//#define DECLS_MARGIN  0xf800

static void * nullptr = NULL;

void out_type_ref(CIp_t CI, typeobj * tobj, uns32 ladr, uns32 * logoffs)
// The value of [ladr]==0xffffffff indicates the T_SIMPLE type.
{ if (ladr==0xffffffff)
  { out_decl(CI, (tptr)&tobj->type.simple.typenum, 1, logoffs);
    out_decl(CI, (tptr)&nullptr,     sizeof(void*)-1, logoffs);
  }
  else
  { gen_rel_addr(CI, ladr);  
    *logoffs += sizeof(void*);
  }
}

#define ADD_BEFORE(tobj) ((*(uns8*)(tobj)==ADDRESS_MARK) || !DIRECT(tobj))

static uns32 ADDD_object(CIp_t CI, object * ob, uns32 * logoffs)
{ uns32 myaddr, ladr=0xffffffff, ladr2;
  if (*(uns8*)ob==ADDRESS_MARK)  /* has already been added */
    return *(uns32*)((tptr)ob+1) & 0xffffffL;
//  if ((*logoffs & 0xffff) > DECLS_MARGIN)  /* align */
//    while (*logoffs & 0xffff) { gen(CI, 0);  (*logoffs)++; }
  switch (ob->any.categ)
  { case O_VAR:  case O_VALPAR:  case O_REFPAR:  case O_RECELEM:
      if (ADD_BEFORE(ob->var.type))  /* add the type */
        ladr=ADDD_object(CI, (object*)ob->var.type, logoffs);
      myaddr=*logoffs;
      out_decl(CI, (tptr)&ob->var.categ, 1, logoffs);
      out_type_ref(CI, ob->var.type, ladr, logoffs);
      out_decl(CI, (tptr)&ob->var.offset, sizeof(unsigned)+1+1, logoffs);
      break;
    case O_CONST:
      if (ADD_BEFORE(ob->cons.type))
        ladr=ADDD_object(CI, (object*)ob->cons.type, logoffs);
      myaddr=*logoffs;
      out_decl(CI, (tptr)&ob->cons.categ, 1+sizeof(basicval0), logoffs);
      out_type_ref(CI, ob->cons.type, ladr, logoffs);
      break;
    case O_PROC:  case O_FUNCT:
      if (ob->subr.retvaltype)
        if (ADD_BEFORE(ob->subr.retvaltype))
          ladr=ADDD_object(CI, (object*)ob->subr.retvaltype, logoffs);
      ladr2=add_objtable(CI, ob->subr.locals, logoffs);
      myaddr=*logoffs;
      out_decl(CI, (tptr)&ob->subr.categ, 1+sizeof(code_addr)+sizeof(unsigned), logoffs);
      gen_rel_addr(CI, ladr2);  *logoffs+=sizeof(void*);
      if (ob->subr.retvaltype)
        out_type_ref(CI, ob->subr.retvaltype, ladr, logoffs);
      else // NULL ptr, direct output
        out_decl(CI, (tptr)&ob->subr.retvaltype, sizeof(void*), logoffs);
      out_decl(CI, (tptr)&ob->subr.saved_ordr, sizeof(uns16), logoffs);
      break;
    case O_CURSOR:
      if (ob->curs.type)
        if (ADD_BEFORE(ob->curs.type))
          ladr=ADDD_object(CI, (object*)ob->curs.type, logoffs);
      myaddr=*logoffs;
      out_decl(CI, (tptr)&ob->curs.categ, 1+sizeof(code_addr)+sizeof(tobjnum), logoffs);
      if (ob->curs.type)
        out_type_ref(CI, ob->curs.type, ladr, logoffs);
      else // NULL ptr, direct output
        out_decl(CI, (tptr)&ob->curs.type, sizeof(void*), logoffs);
      break;
    case O_TYPE:
      myaddr=*logoffs;  // may be overwritten
      //if (!DIRECT(&ob->type))
      { switch (ob->type.type.anyt.typecateg)
        { case T_SIMPLE:
            return ob->type.type.simple.typenum;
          case T_ARRAY:
            if (ADD_BEFORE(ob->type.type.array.elemtype))
              ladr=ADDD_object(CI, (object*)ob->type.type.array.elemtype, logoffs);
            myaddr=*logoffs;
            out_decl(CI, (tptr)&ob->type.categ, 1+1+sizeof(unsigned)+2*sizeof(sig32), logoffs);
            out_type_ref(CI, ob->type.type.array.elemtype, ladr, logoffs);
            break;
          case T_STR:  
            out_decl(CI, (tptr)ob, 1+sizeof(stringtype), logoffs);
            break;
          case T_BINARY:
            out_decl(CI, (tptr)ob, 1+sizeof(binarytype), logoffs);
            break;
          case T_VIEW:
            out_decl(CI, (tptr)ob, 1+sizeof(viewtype),   logoffs);
            break;
          case T_PTR:
            if (ADD_BEFORE(ob->type.type.ptr.domaintype))
              ladr=ADDD_object(CI, (object*)ob->type.type.ptr.domaintype, logoffs);
            myaddr=*logoffs;
            out_decl(CI, (tptr)&ob->type.categ, 1+1+sizeof(unsigned), logoffs);
            out_type_ref(CI, ob->type.type.ptr.domaintype, ladr, logoffs);
            break;
          case T_PTRFWD:  // domain type pointer invalid
            ob->type.type.fwdptr.domaintype=NULL;
            out_decl(CI, (tptr)ob, 1+sizeof(fwdptrtype), logoffs);
            break;
          case T_RECORD:
            ladr=add_objtable(CI, ob->type.type.record.items, logoffs);
            myaddr=*logoffs;
            out_decl(CI, (tptr)&ob->type.categ, 1+1+sizeof(unsigned), logoffs);
            gen_rel_addr(CI, ladr);  *logoffs+=sizeof(void*);
            break;
          case T_RECCUR:
            if (ob->type.type.record.items != NULL)
            { ladr=add_objtable(CI, ob->type.type.record.items, logoffs);
              myaddr=*logoffs;
              out_decl(CI, (tptr)&ob->type.categ, 1+1+sizeof(unsigned), logoffs);
              gen_rel_addr(CI, ladr);  *logoffs+=sizeof(void*);
            }
            else out_decl(CI, (tptr)&ob->type.categ, 1+sizeof(recordtype), logoffs);
            break;
        }
      }
      break;
    default:
      myaddr=*logoffs;
      out_decl(CI, (tptr)ob, objdefsize[ob->any.categ], logoffs);
      break;
  }
  *(tptr)ob=(uns8)ADDRESS_MARK;  memcpy((tptr)ob+1, &myaddr, 3);
  return myaddr;
}

static uns32 add_objtable(CIp_t CI, objtable * ot, uns32 * logoffs)
// Outputs objtable and all its subobjects, returns logical offset of the table.
/* Object tables must be protected against double adding, like objects.
   Important for record item tables, if record is used in a cursor. */
{ uns32 myaddr;  uns32 * ptr;  int i;
 // if the table has already been serialised, return its offset only:
  if (*(uns8*)ot==ADDRESS_MARK && ((uns8*)ot)[4]==0xdc && ((uns8*)ot)[5]==0xdd)
    return *(uns32*)((tptr)ot+1) & 0xffffffL;  /* has already been added */
 // serialise the descriptions of all objects if the table and replace their pointers by offsets:
  for (i=0; i<ot->objnum; i++)  /* add the subobjects */
    //if (COMPOSED(ot->objects[i].descr))  -- valid for all now
    { ptr=(uns32*)&ot->objects[i].descr;  /* ptr to object ptr */
      *ptr=ADDD_object(CI, ot->objects[i].descr, logoffs);
    }
 // store the offset of the table to be returned later:
  myaddr=*logoffs;
 // serialise the header of the table:
  out_decl(CI, (tptr)ot, sizeof(objtable)-sizeof(objdef), logoffs);
 // serialise the objects:
  for (i=0; i<ot->objnum; i++)  /* copy the subobjects */
  { out_decl(CI, (tptr)(ot->objects+i), sizeof(objdef)-sizeof(void*), logoffs);
    gen_rel_addr(CI, *(uns32*)&ot->objects[i].descr);  *logoffs+=sizeof(void*);
  }
 // remeber the offset of the table and mark it as being serialised:
  *(tptr)ot=(uns8)ADDRESS_MARK;  memcpy((tptr)ot+1, &myaddr, 3);
  ((tptr)ot)[4]=(uns8)0xdc;  ((tptr)ot)[5]=(uns8)0xdd;
  return myaddr;
}

CFNC DllKernel void WINAPI update_object_nums(cdp_t cdp)
/* Updates table & cursor numbers in project decls */
{ int i;
  objtable * ot = cdp->RV.run_project.global_decls;
  for (i=0;  i<ot->objnum;  i++)  /* add the subobjects */
    //if (COMPOSED(ot->objects[i].descr)) -- valid for all now
    { object * ob=ot->objects[i].descr;  tobjnum objnum;
      switch (ob->any.categ)
      { case O_CURSOR:
          ob->curs.defnum=
            cd_Find_object(cdp, ot->objects[i].name, CATEG_CURSOR, &objnum) ?
              NOOBJECT : objnum;  break;
        case O_TABLE:
          ob->curs.defnum=
            cd_Find_object(cdp, ot->objects[i].name, CATEG_TABLE,  &objnum) ?
              NOOBJECT : objnum;  break;
      }
    }
}

#ifndef WINS
#if WBVERS<900
#include "project.cpp"
#endif
#endif

void add_source_to_list(CIp_t CI)
{ if (CI->phdr && CI->source_counter < MAX_SOURCE_NUMS)
    CI->phdr->sources[CI->source_counter++]=CI->src_objnum;
}

CFNC DllKernel void WINAPI program(CIp_t CI)
{ pgm_header_type pgm_header;  uns32 logoffs;  unsigned prog_ordr, prog_offset;
 /* preparing the default invalid program header */
  pgm_header.version=0xffff;   /* invalid compiler version number */
  (*code_out)(CI, (tptr)&pgm_header, 0, sizeof(pgm_header_type));
  CI->code_offset=sizeof(pgm_header_type);
 // init source list:
  CI->source_counter=0;
  CI->phdr=&pgm_header;
  add_source_to_list(CI);
 // compile global decls:
  new_id_table(CI);
  prog_ordr=0;  prog_offset=0;
  declarations(CI, TRUE, &prog_ordr, &prog_offset);
  /* reorder_decls(prog_ordr);  -- no need (no support for std type alias) */
  pgm_header.main_entry=CI->code_offset;
 /* allocate main vars */
  pgm_header.proj_vars_size=prog_offset;
  code_addr adr=gen_forward_jump(CI,I_JUMP); /* start of debugged program needs it */
  setcadr(CI, adr);
  compound_statement(CI);
  test_and_next(CI,'.',DOT_EXPECTED);
  c_end_check(CI);
  gen(CI,I_STOP);
  pgm_header.code_size=CI->code_offset;

  CI->compil_ptr=NULL;   /* signalizes generating declar. info (code_out) */
  logoffs=0;  
  if (CI->output_type==DB_OUT) /* output of decl. table */
    pgm_header.pd_offset=add_objtable(CI, CI->id_tables, &logoffs);
 /* compilation OK, write the proper header values */
  pgm_header.decl_size=CI->code_offset-pgm_header.code_size;
  pgm_header.unpacked_decl_size=logoffs;
  pgm_header.version=EXX_CODE_VERSION;   /* valid compiler version number */
  pgm_header.platform=CURRENT_PLATFORM;
  pgm_header.language=CI->cdp->selected_lang;
  if (CI->source_counter < MAX_SOURCE_NUMS)
    pgm_header.sources[CI->source_counter]=NOOBJECT; // terminator
  pgm_header.compil_timestamp=stamp_now();
  (*code_out)(CI, (tptr)&pgm_header, 0, sizeof(pgm_header_type));
}

/***************************** statements ***********************************/
#ifndef SRVR

static char STR_READ[5] = "READ", STR_WRITE[6] = "WRITE", STR_WRITELN[8] = "WRITELN";

void stat_seq(CIp_t CI)
{ BOOL first=TRUE, was_if=FALSE;
  while ((CI->cursym!=S_UNTIL) && (CI->cursym!=S_END)  && (CI->cursym!=S_SOURCE_END))
  { if (first) first=FALSE;
    else if (CI->cursym==';') next_sym(CI);
    else c_error(SEMICOLON_EXPECTED);
    if (was_if && CI->cursym==S_ELSE) c_error(SEMICOLON_ELSE);
    was_if=CI->cursym==S_IF;
    statement(CI);
  }
}

CFNC DllKernel void WINAPI statement_seq(CIp_t CI)
{ stat_seq(CI);
  c_end_check(CI);
}

CFNC DllKernel void WINAPI event_handler(CIp_t CI)
{ CI->thisform=1;
  new_id_table(CI);
  subr_def(CI);
  objtable * tb = CI->id_tables;
  CI->id_tables = tb->next_table;
  corefree(tb);
  if (CI->cursym==';') next_sym(CI);
  c_end_check(CI);
}

void compound_statement(CIp_t CI)
{ /* CI->cursym==S_BEGIN */
  next_sym(CI);
  stat_seq(CI);
  test_and_next(CI,S_END, END_EXPECTED);
}

void if_statement(CIp_t CI)    /* CI->cursym==S_IF */
{ code_addr adr1_point, adr2_point;
  next_sym(CI);
  bool_expr(CI);
  adr1_point=gen_forward_jump4(CI, I_FALSEJUMP4);
  test_and_next(CI,S_THEN,THEN_EXPECTED);
  statement(CI);
  if (CI->cursym==S_ELSE)
  { adr2_point=gen_forward_jump4(CI, I_JUMP4);
    setcadr4(CI, adr1_point);
    next_sym(CI);
    statement(CI);
    adr1_point=adr2_point;
  }
  setcadr4(CI, adr1_point);
}

static void sql_execute_statement(CIp_t CI)
{ geniconst(CI, NUM_SQLEXECUTE);
  gen(CI, I_PREPCALL);  gen4(CI, 4);  // parameter size
  gen(CI,I_STRINGCONST);  gen2(CI,(uns16)strlen(CI->curr_string()));
  genstr(CI, CI->curr_string());
  gen(CI, I_P_STORE4);
  gen(CI, I_SCALL);
  gen(CI, I_DROP);
  next_sym(CI);
}

#define MAX_CASE_CONSTS 30

void case_statement(CIp_t CI)    /* CI->cursym==S_CASE */
{ code_addr next_test_point, else_addr = (code_addr)-1;
  int c_cnt;  code_addr jmps[MAX_CASE_CONSTS];  BOOL else_branch;  sig32 val;
  code_addr * links;  int linknum=0, linkmax, i;
  links=(code_addr*)comp_alloc(CI, sizeof(code_addr) * (linkmax=10));
  next_sym(CI);
  int_expression(CI);
  next_test_point=0; 
  test_and_next(CI,S_OF, OF_EXPECTED);
  while (CI->cursym!=S_END)
  { if (next_test_point) setcadr4(CI, next_test_point);
    else_branch=FALSE;
    c_cnt=0;
    do  /* processing of constants */
    { if (c_cnt >= MAX_CASE_CONSTS) c_error(INTEGER_EXPECTED);
      if ((CI->cursym==S_ELSE) && (else_addr == (code_addr)-1))
        { else_branch=TRUE;  next_sym(CI); }
      else //if (CI->cursym==S_INTEGER)
      { val=num_val(CI, (sig32)0x80000000L, 0x7fffffffL); //CI->curr_int;
        //next_sym(CI);
        gen(CI,I_DUP);
        geniconst(CI, val);
        gen(CI,(uns8)(I_INTOPER+opindex(S_NOT_EQ)));
        jmps[c_cnt++]=gen_forward_jump4(CI, I_FALSEJUMP4);   /* jump iff equal */
      }
//      else c_error(INTEGER_EXPECTED);
      if (CI->cursym != ',') break;
      next_sym(CI);
    } while (TRUE);
    next_test_point=gen_forward_jump4(CI, I_JUMP4);  /* go to next test if not matched */
    test_and_next(CI,':', COLON_EXPECTED);
    while (c_cnt) setcadr4(CI, jmps[--c_cnt]);
    if (else_branch) else_addr=CI->code_offset;
    statement(CI);
    if (linknum == linkmax)
    { links=(code_addr*)corerealloc(links, sizeof(code_addr) * (linkmax+=6));
      if (!links) c_error(OUT_OF_MEMORY);
    }
    links[linknum++]=gen_forward_jump4(CI, I_JUMP4);
    if (CI->cursym==';') next_sym(CI);
    else if (CI->cursym!=S_END) c_error(SEMICOLON_EXPECTED);
  }
  if (else_addr != (code_addr)-1) /* not matched: jump to the else-branch */
  { sig32 adr = else_addr-next_test_point;
    code_out(CI, (tptr)&adr, next_test_point, sizeof(sig32));
  }
  else setcadr4(CI, next_test_point); /* jump here if not matched and no ELSE used */
  for (i=0; i<linknum; i++) setcadr4(CI, links[i]); /* on branch exit jump here */
  corefree(links);
  next_sym(CI);
  gen(CI,I_DROP);
}

static void gen_jump(CIp_t CI, BOOL unconditional, code_addr destination)
{ sig32 diff = destination-(CI->code_offset+1);
  if (destination > (int)0x7fff || destination < -(int)0x7fff)
  { gen(CI, unconditional ? I_JUMP4 : I_FALSEJUMP4); 
    gen4(CI, diff);
  }
  else // near jump
  { gen(CI, unconditional ? I_JUMP : I_FALSEJUMP); 
    gen2(CI, (sig16)diff);
  }
}

void repeat_statement(CIp_t CI)  /* CI->cursym==S_REPEAT */
{ code_addr loop_point;
  loop_point=(code_addr)CI->code_offset;
  next_sym(CI);
  stat_seq(CI);
  test_and_next(CI,S_UNTIL, UNTIL_EXPECTED);
  bool_expr(CI);
  gen_jump(CI, FALSE, loop_point);
}

void while_statement(CIp_t CI)
{ /* CI->cursym==S_WHILE */
  code_addr loop_point, outadr_point;
  loop_point=CI->code_offset;
  next_sym(CI);
  bool_expr(CI);
  outadr_point=gen_forward_jump4(CI, I_FALSEJUMP4);
  test_and_next(CI,S_DO,DO_EXPECTED);
  statement(CI);
  gen_jump(CI, TRUE, loop_point);
  setcadr4(CI, outadr_point);
}

void for_statement(CIp_t CI)
{ typeobj * obj, * tobj;  BOOL upwards;  code_addr loop_point, outadr_point;  int aflag, eflag;
  next_and_test(CI,S_IDENT, IDENT_EXPECTED);
  obj=selector(CI, SEL_ADDRESS, aflag);
  if (obj!=&att_int16 && obj!=&att_int32 || (aflag & (DB_OBJ | CACHED_ATR | MULT_OBJ)))
    c_error(MUST_BE_INTEGER);    /* do not use int_check, no conversion */
  test_and_next(CI,S_ASSIGN,ASSIGN_EXPECTED);
  gen(CI,I_DUP);   /* control variable address duplicated */
  tobj=expression(CI, &eflag);
  assignment(CI, obj, aflag, tobj, eflag, FALSE);
  if (CI->cursym==S_TO) upwards=TRUE;
  else
  { if (CI->cursym!=S_DOWNTO) c_error(TO_OR_DOWNTO_EXPECTED);
    upwards=FALSE;
  }
  next_sym(CI);
 loop_point=CI->code_offset;
  gen(CI,I_DUP);   /* control variable address duplicated */
  gen(CI, (uns8)((DIRTYPE(obj)==ATT_INT16) ? I_LOAD2 : I_LOAD4) );
  tobj=expression(CI, &eflag);
  binary_op(CI, obj, aflag, tobj, eflag, (symbol)(upwards ? S_LESS_OR_EQ : S_GR_OR_EQ));
  outadr_point=gen_forward_jump4(CI, I_FALSEJUMP4);
  test_and_next(CI,S_DO,DO_EXPECTED);
  statement(CI);
  gen(CI,I_DUP);  gen(CI,I_DUP);
  gen(CI, (uns8)((DIRTYPE(obj)==ATT_INT16) ? I_LOAD2 : I_LOAD4) );
  geniconst(CI, 1);
  gen(CI, (uns8)(upwards ? I_INTPLUS : I_INTMINUS));
  gen(CI, (uns8)((DIRTYPE(obj)==ATT_INT16) ? I_STORE2 : I_STORE4) );
 gen_jump(CI, TRUE, loop_point);
  setcadr4(CI, outadr_point);
  gen(CI,I_DROP);  /* control variable address removed */
}

extern kont_descr kont_from_selector;  /* temp. variable for the kontext */

void with_statement(CIp_t CI)
{ typeobj * obj;  kont_descr mykont;  tobjname name;  int aflag;
  next_sym(CI);
  strmaxcpy(name, CI->curr_name, sizeof(tobjname));
  obj=selector(CI, SEL_ADDRESS, aflag);
  if (obj!=&att_dbrec)
    { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); }
  mykont=kont_from_selector;
  if (mykont.kontcateg==CATEG_RECCUR &&
      ((typeobj*)mykont.record_tobj)->type.record.items == NULL)
    c_error(CURSOR_STRUCTURE_UNKNOWN);   /* structure not described */
  test_and_next(CI,S_DO, DO_EXPECTED);
  gen(CI,I_PUSH_KONTEXT);
  mykont.next_kont=CI->kntx;
  CI->kntx=&mykont;
    statement(CI);
  CI->kntx=CI->kntx->next_kont;
  gen(CI,I_DROP_KONTEXT);
}

void write_statement(CIp_t CI, BOOL nl)
{ typeobj * tobj;
  next_sym(CI);
  test_and_next(CI,'(', LEFT_PARENT_EXPECTED);
  tobj=expression(CI);
  if (tobj!=&att_file) c_error(INCOMPATIBLE_TYPES);
  gen(CI,I_SAVE);
  if (CI->cursym==')') { next_sym(CI); goto put_nl; }
  test_and_next(CI,',', COMMA_EXPECTED);
  do
  { tobj=expression(CI);
    if (tobj->type.anyt.typecateg==T_SIMPLE || tobj->type.anyt.typecateg==T_SIMPLE_DB)  // ?? DB
    { if (IS_HEAP_TYPE(tobj->type.simple.typenum)) c_error(INCOMPATIBLE_TYPES);
    }
    else
      if (CHAR_ARR(tobj)) tobj=&att_string;
      else c_error(INCOMPATIBLE_TYPES);
    if (CI->cursym==':')
    { next_sym(CI);
      int_expression(CI);
    }
    else geniconst(CI, tobj==&att_date ? 1 : tobj==&att_time ? 2 : 5);
    gen(CI,I_RESTORE);
    gen(CI,I_WRITE); gen(CI,(uns8)DIRTYPE(tobj));
    if (CI->cursym!=',')
      if (CI->cursym==')') break;
      else c_error(COMMA_EXPECTED);
    next_sym(CI);
  } while (TRUE);
  next_sym(CI);
put_nl:
  if (nl)
  { gen(CI, (uns8)I_RESTORE);
    gen(CI, (uns8)I_WRITE); gen(CI,0xff);
  }
}

CFNC DllKernel void WINAPI statement(CIp_t CI)
{ typeobj * tobj, * obj;  
  switch (CI->cursym)
  { case S_WHILE:  while_statement(CI);     break;
    case S_REPEAT: repeat_statement(CI);    break;
    case S_IF:     if_statement(CI);        break;
    case S_CASE:   case_statement(CI);      break;
    case S_FOR:    for_statement(CI);       break;
    case S_BEGIN:  compound_statement(CI);  break;
    case S_WITH:   with_statement(CI);      break;
    case S_SQLSTRING: sql_execute_statement(CI);  break;
    case S_HALT:
      /*geniconst(-1);  gen(CI,I_EVENTRET); -- very bad, RETURN must not be in program
        (RETURNS stops the current program & sends a message to the upper program,
        if both are the same, the message box will appear). */
      next_sym(CI);  gen(CI,I_STOP);
      break;
    case S_RETURN:
      next_sym(CI);
      int_expression(CI);
      gen(CI,I_EVENTRET);
      break;
    case S_IDENT:
      if (!strcmp(CI->curr_name, STR_READ))    { read_statement(CI);  gen(CI,I_DROP);  break; }
      if (!strcmp(CI->curr_name, STR_WRITE))   { write_statement(CI, FALSE);  break; }
      if (!strcmp(CI->curr_name, STR_WRITELN)) { write_statement(CI, TRUE);  break; }
      //cont.
    case S_THISFORM:
    { int aflag, eflag;
      obj=selector(CI, SEL_ADDRESS, aflag);
      if (obj)   /* unless a procedure called */
      { test_and_next(CI,S_ASSIGN,ASSIGN_EXPECTED);
        t_specif aux3=CI->glob_db_specif;
        tobj=expression(CI, &eflag);
        if ((obj==&att_dbrec) && (tobj==&att_dbrec))
          c_error(INCOMPATIBLE_TYPES);  /* assgnment cannot recognize it */
        CI->glob_db_specif=aux3;
        if (obj==&att_table) c_error(INCOMPATIBLE_TYPES);
        /* not checked by assignment, ATT_TABLE possible for parameter transfer */
        assignment(CI, obj, aflag, tobj, eflag, FALSE);
      }
      break;
    }
    case ';':   case S_END:  case S_SOURCE_END:  case S_ELSE:  case S_UNTIL:
      break;  /* empty statement */
    default:
      c_error(STATEMENT_EXPECTED);
      break;
  }
}
#endif
