// cpactions.cpp
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
#include "xmldsgn.h"
#include "wprez9.h"
#include "regstr.h"
#include "cptree.h"
#include "cplist.h"
#include "impexpui.h"
#include "debugger.h"
#ifdef LINUX
#include <prefix.h>
#endif

#define DELETE_CONFIRM_PATTERN     L

// global class initialising translations in the constructor: must be before all global classes using translations in constructors!
class language_support
{
//  wxLocale glob_locale; -- global -- does not always work!
  int  client_language;
  bool client_load_catalog;
 public:
//  language_support(void);
  void set(wxLocale & locale);
};

#if 0
language_support::language_support(void)
{
  if (!read_profile_int("Locale", "Language", &client_language)) client_language=wxLANGUAGE_DEFAULT;
  client_load_catalog=read_profile_bool("Locale", "LoadCatalog", true);
  set(glob_locale);
}

#endif

language_support lsup;   
 
void language_support::set(wxLocale & locale)
{ 
  if (!read_profile_int("Locale", "Language", &client_language)) client_language=wxLANGUAGE_DEFAULT;
  client_load_catalog=read_profile_bool("Locale", "LoadCatalog", true);
#ifdef WINS // lookup prefix is . by default, we must specify an fixed location!
  wxChar path[MAX_PATH];
  int len = GetModuleFileName(NULL, path, MAX_PATH);
  while (len && path[len-1]!='\\') len--;
  path[len]=0;
  locale.AddCatalogLookupPathPrefix(path); 
#else
  char path[MAX_PATH];
  strcpy(path, PREFIX);  // module is located in lib/602sql95/, prefix ends with /lib/
  strcat(path, "/../share/locale");  // looks in path/<language>/LC_MESSAGES
  locale.AddCatalogLookupPathPrefix(wxString(path, *wxConvCurrent));
#endif
  locale.Init(client_language==wxLANGUAGE_DEFAULT ? wxLocale::GetSystemLanguage() : client_language,
              client_load_catalog ? wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING : 0);
    ;//if (!...) wxLogWarning(_("Error setting the locale."));
  // Must not pass wxLANGUAGE_DEFAULT as parameter: it is translated to	GetSystemLanguage() but
  // ... locale is not initialized properly then (windows do not get national charactes)		  
  if (client_load_catalog) 
  { wxString domain(GETTEXT_DOMAIN, *wxConvCurrent);
    if (!locale.AddCatalog(domain))  // must be before creating windows
      wxLogWarning(_("Message catalog not loaded"));
  }  
}

void set_language_support(wxLocale & locale)                  
{ lsup.set(locale); 
}

#include "xrc/CallRoutineDlg.cpp"
#include "xrc/NewFtx.cpp"
#include "xrc/XMLParamsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define CANNOT_WRITE_REGISTRY  _("Cannot write to the registry. You may have insufficient privileges.")
#define NEW_APPL_NAME   _("Enter the name of the new application:")
#define NEW_GROUP_NAME  _("Enter the name of the new group:")
#define NEW_ROLE_NAME   _("Enter a name of the new role:")

bool DevelOpt = true;

BOOL can_modify(tobjnum objnum, tcateg categ) // checks DevelOpt and locked_appl, reports errors
{ if (!DevelOpt) { error_box(_("Development tools not installed."));  return FALSE; }
  return TRUE;
}

bool can_modify(cdp_t cdp) // checks DevelOpt 
{ if (DevelOpt)
    return true;
  client_error(cdp, APPL_IS_LOCKED);
  return false;
}

void delete_object_from_cache(item_info & info)
{ Upcase9(info.cdp, info.object_name);
#if 0
  if (info.category==CATEG_CONNECTION)
  { t_connection * conn, ** pconn = &info.cdp->odbc.conns;
    while (*pconn != NULL && my_stricmp((*pconn)->dsn, info.object_name))
      pconn=&(*pconn)->next_conn;
    conn=*pconn;
    if (conn != NULL)
    { *pconn=conn->next_conn;
      free_connection(info.cdp, conn);
    }
    inval_table_d(info.cdp, NOOBJECT, CATEG_DIRCUR);  /* must remove odbc table info from the cache */
  }
  else
#endif
  { unsigned pos;
    t_dynar * d_comp=get_object_cache(info.cdp, info.category);
    if (d_comp!=NULL)
    { if (comp_dynar_search(d_comp, info.object_name, &pos))
      { tobjnum objn = ((t_comp*)d_comp->acc(pos))->objnum;
        d_comp->delet(pos);  // must first delete the table from the cache and the call inval_table_d which scans relations which needs descriptors of existing tables
        if (info.category==CATEG_TABLE || info.category==CATEG_CURSOR)
          inval_table_d(info.cdp, objn, info.category);
      }
    }
#if 0
    if (IS_ODBC_TABLEC(info.category, info.objnum))
      Unlink_odbc_table(info.cdp, info.objnum);
#endif      
  }
}

BOOL cd_Restart_sequence(cdp_t cdp, tobjnum objnum, const char * object_name, const char * schema_name)
{ char stmt[30+2*OBJ_NAME_LEN];
  sprintf(stmt, "ALTER SEQUENCE `%s`.`%s`", schema_name, object_name);
  return cd_SQL_execute(cdp, stmt, NULL);
}

#include "xrc/NewRoutineDlg.cpp"
#include "xrc/NewTriggerWiz.cpp"

tobjnum create_initial_trigger(cdp_t cdp, NewTriggerWiz & wiz)
{ tobjnum trigobj;
  if (cd_Insert_object(cdp, wiz.GetTriggerName().mb_str(*cdp->sys_conv), CATEG_TRIGGER, &trigobj))
      { cd_Signalize(cdp);  return NOOBJECT; }
  wxString initial_def;
  initial_def.Printf(wxT("TRIGGER `%s` "), wiz.trigger_name.c_str());
  initial_def = initial_def + (wiz.before ? wxT("BEFORE ") : wxT("AFTER ")); 
  initial_def = initial_def + (wiz.trigger_oper==TO_INSERT ? wxT("INSERT") :
                               wiz.trigger_oper==TO_DELETE ? wxT("DELETE") : wxT("UPDATE"));
  if (wiz.GetExplicitAttrs())
  { const d_table * td = cd_get_table_d(cdp, wiz.tabobj, CATEG_TABLE);
    if (td!=NULL)
    { bool first=true;
      for (int j=0, i=first_user_attr(td);  i<td->attrcnt;  i++)
      { const d_attr * att = ATTR_N(td,i);
        if (att->multi==1)
        { if (wiz.attrmap.has(j))
          { initial_def = initial_def + (first ? wxT(" OF `") : wxT(", `"));  
            initial_def = initial_def + wxString(att->name, *cdp->sys_conv);  
            initial_def = initial_def + wxT("`"); 
            first=false;
          }
          j++;
        }
      }
      release_table_d(td);
    }
  }
  initial_def = initial_def + wxT("\r\nON `");  
  initial_def = initial_def + wiz.GetTriggerTable();  
  initial_def = initial_def + wxT("`"); 
  if (!wiz.GetOldname().IsEmpty() || !wiz.GetNewname().IsEmpty())
  { initial_def = initial_def + wxT("\r\nREFERENCING");
    if (!wiz.GetOldname().IsEmpty()) { initial_def = initial_def + wxT(" OLD AS `");  initial_def = initial_def + wiz.GetOldname();  initial_def = initial_def + wxT('`'); }
    if (!wiz.GetNewname().IsEmpty()) { initial_def = initial_def + wxT(" NEW AS `");  initial_def = initial_def + wiz.GetNewname();  initial_def = initial_def + wxT('`'); }
  }
  initial_def = initial_def + (wiz.GetStatementGranularity() ? wxT("\r\nFOR EACH STATEMENT") : wxT("\r\nFOR EACH ROW"));
  initial_def = initial_def + wxT("\r\n{ WHEN <condition> }\r\nBEGIN\r\n  { <statement> }\r\nEND\r\n\r\n");
 // save the initial definition:
  if (cd_Write_var(cdp, OBJ_TABLENUM, trigobj, OBJ_DEF_ATR, NOINDEX, 0, (int)strlen(initial_def.mb_str(*cdp->sys_conv)), initial_def.mb_str(*cdp->sys_conv)))
    { cd_Signalize(cdp);  return NOOBJECT; }
  cd_Trigger_changed(cdp, trigobj, TRUE); // important when trigger not saved in the editor
  return trigobj;
}

#include "dsparser.h"
#include "transdsgn.h"
#if WBVERS<950
#include "xrc/NewDataTransferDlg.cpp"
#else
#include "xrc/NewDataTransfer95Dlg.cpp"
#endif
#include "xrc/TransportDataDlg.cpp"
#include "xrc/XMLRunDlg.cpp"
#include "xrc/UserPropDlg.cpp"
#include "xrc/NewSSDlg.cpp"
#include "xrc/FtxSearchDlg.cpp" 

#include "xrc/SubjectRelationshipDlg.cpp"

bool Edit_relation(cdp_t cdp, wxWindow * parent, tcateg subject1, tobjnum subjnum, tcateg subject2)
{ if (subject1==subject2)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return FALSE; }
  SubjectRelationshipDlg dlg(cdp, subject1, subjnum, subject2);
  dlg.Create(parent);
  return dlg.ShowModal()==wxID_OK;
}

void start_transport_wizard(item_info & info, t_ie_run * dsgn, bool xml)
// Deletes dsgn.
{ 
  t_xsd_gen_params gen_params;
#if WBVERS<950
  NewDataTransferDlg dlg(dsgn, xml, &gen_params);
#else
  NewDataTransfer95Dlg dlg(dsgn, xml, &gen_params);
#endif
  dlg.Create(wxGetApp().frame);                            
  if (dlg.Run2())
    if (dlg.xml_transport)
      start_dad_designer(info.cdp, NOOBJECT, info.folder_name, dlg.mPageXML, &gen_params);
    else
      start_transport_designer(info.cdp, NOOBJECT, info.folder_name, dlg.dsgn->conv_ie_to_source());
  delete dsgn;
}

bool create_trigger(wxWindow * parent, item_info & info, const char * tabname)
// Runs the trigger wizard and the editor. Returns true iff trigger has been created. Returns its objnum and name in [info].
{ if (!cd_Am_I_appl_author(info.cdp))
    { error_box(_("Triggers can only be created by the application author."), parent);  return false; }
  NewTriggerWiz wiz(info.cdp, tabname);
  wiz.Create(parent);
  if (!wiz.Run2()) 
    return false;
  tobjnum trigobj = create_initial_trigger(info.cdp, wiz);
  if (trigobj==NOOBJECT)   // open editor:
    return false;
  strcpy(info.object_name, wiz.trigger_name.mb_str(*info.cdp->sys_conv));
  info.objnum=trigobj;
  open_text_object_editor(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_DEF_ATR, CATEG_TRIGGER, info.folder_name, OPEN_IN_EDCONT);
  return true;
}  

bool create_object(wxWindow * parent, item_info & info)
// Returns true if object has been created and info updated -> caller must add it to the control panel
// This function sets info.object_name and objnum, a new object has been created.
{
  if (info.syscat==SYSCAT_APPLS) info.category=CATEG_APPL;
 // check the privilege to insert records to system tables:
  uns8 priv_val[PRIVIL_DESCR_SIZE];
  if (cd_GetSet_privils(info.cdp, NOOBJECT, CATEG_USER, CATEG2SYSTAB(info.category), NORECNUM, OPER_GETEFF, priv_val)
      || !(*priv_val & RIGHT_INSERT))
  { client_error(info.cdp, NO_RIGHT);  cd_Signalize(info.cdp);
    return false;
  }
  if (info.syscat == SYSCAT_APPLS)
  { *info.object_name=0;
    return edit_name_and_create_object(info.cdp, parent, NULL, info.category, NEW_APPL_NAME, &info.objnum, info.schema_name);
  }    
  switch (info.category)
  { case CATEG_TABLE:
      open_table_designer(info.cdp, NOOBJECT, info.folder_name, info.schema_name);
      return false;  // The designer has been opened but the object has not been created yet.
    case CATEG_CURSOR:
      start_query_designer(info.cdp, NOOBJECT, info.folder_name);
      return false;
    case CATEG_PGMSRC:
      if (IsCtrlDown())
      {
        open_text_object_editor(info.cdp, OBJ_TABLENUM, NOOBJECT, OBJ_DEF_ATR, CATEG_PGMSRC, info.folder_name, OPEN_IN_EDCONT);
      }
      else
      { t_ie_run * dsgn = new t_ie_run(info.cdp, false);
        dsgn->source_type=IETYPE_WB;  dsgn->dest_type=IETYPE_CSV;
        *dsgn->inpath=*dsgn->outpath=0;
        *dsgn->cond=0;
        dsgn->index_past=false;
        start_transport_wizard(info, dsgn, true);
      }
      return false;
    case CATEG_DOMAIN:
      start_domain_designer(info.cdp, NOOBJECT, info.folder_name);
      return false;  // The designer has been opened but the object has not been created yet.
    case CATEG_SEQ:
      start_sequence_designer(info.cdp, NOOBJECT, info.folder_name);
      return false;  // The designer has been opened but the object has not been created yet.
    case CATEG_PROC:
    { NewRoutineDlg dlg(info.cdp);
      dlg.Create(parent);
      if (dlg.ShowModal()!=wxID_OK) break;
      strcpy(info.object_name, dlg.objname);
      wxString initial_def;
      if (dlg.GetObjtype()==0)
        initial_def=wxT("// Global module declarations\r\nDECLARE \r\n\r\n");
      else  if (dlg.GetObjtype()==1)  // procedure
        initial_def.Printf(wxT("PROCEDURE `%s`( { <parameters> } );\r\nBEGIN\r\n  { <statement> }\r\nEND\r\n\r\n"), wxString(info.object_name, *info.cdp->sys_conv).c_str());
      else  // function
        initial_def.Printf(wxT("FUNCTION `%s`( { <parameters> } ) RETURNS <type>;\r\nBEGIN\r\n  { <statement> }\r\n  RETURN <value>;\r\nEND\r\n\r\n"), wxString(info.object_name, *info.cdp->sys_conv).c_str());
     // insert the new object:
      if (cd_Insert_object(info.cdp, info.object_name, CATEG_PROC, &info.objnum)) 
        { cd_Signalize(info.cdp);  break; }
      cd_Write_var(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_DEF_ATR, NOINDEX, 0, (int)strlen(initial_def.mb_str(*info.cdp->sys_conv)), initial_def.mb_str(*info.cdp->sys_conv));
      uns32 stamp = stamp_now();
      cd_Write(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_MODIF_ATR,  NULL, &stamp, sizeof(uns32));
      open_text_object_editor(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_DEF_ATR, CATEG_PROC, info.folder_name, OPEN_IN_EDCONT);
      return true;   // adds the new object into the control panel
    }  
    case CATEG_TRIGGER:
      return create_trigger(parent, info, "");
    case CATEG_USER:
    { UserPropDlg dlg(info.cdp, NOOBJECT);
      dlg.Create(parent);
      dlg.ShowModal();
      return false;  // new user added to the object list by the dialog in the proper momment
    }
    case CATEG_GROUP:
      *info.object_name=0;
      if (!edit_name_and_create_object(info.cdp, parent, NULL, info.category, NEW_GROUP_NAME, &info.objnum, info.object_name)) break;
      return true;
    case CATEG_ROLE:
      *info.object_name=0;
      if (!edit_name_and_create_object(info.cdp, parent, info.schema_name, info.category, NEW_ROLE_NAME, &info.objnum, info.object_name)) break;
      return true;
    case CATEG_DRAWING:
      start_diagram_designer(info.cdp, NOOBJECT, info.folder_name);
      return false;  // The designer has been opened but the object has not been created yet.
    case CATEG_INFO:
    { uns32 state;  
      if (cd_Get_server_info(info.cdp, OP_GI_LICS_FULLTEXT, &state, sizeof(state)) || !state)
        { client_error(info.cdp, NO_FULLTEXT_LICENCE);  cd_Signalize(info.cdp);  break; }
      t_fulltext ftd;  
      ftd.basic_form=true;  ftd.with_substructures=false;  ftd.separated=false;  ftd.bigint_id=false;  // default settings
      NewFtx dlg(info.cdp, &ftd);
      dlg.Create(wxGetApp().frame);
      if (dlg.ShowModal()!=wxID_OK) break;
      if (!cd_Find_prefixed_object(info.cdp, ftd.schema, ftd.name, CATEG_INFO, &ftd.objnum))
      { Set_object_folder_name(info.cdp, ftd.objnum, CATEG_INFO, info.folder_name);
        Add_new_component(info.cdp, CATEG_INFO, ftd.schema, ftd.name, ftd.objnum, 0, info.folder_name, true);
        cd_Relist_objects_ex(info.cdp, TRUE);   // new tables, a new sequence
        start_fulltext_designer(info.cdp, ftd.objnum, info.folder_name);
      }
      return false;  // The designer has been opened but the object has not been created yet.
    }
#ifdef XMLFORMS
#ifdef WINS
    case CATEG_XMLFORM:
      wxGetApp().frame->control_panel->new_XML_form(info);
      return false;
#endif
    case CATEG_STSH:
    { NewSSDlg dlg(info.cdp);
      dlg.Create(wxGetApp().frame);
      if (dlg.ShowModal()!=wxID_OK) break;
      //open_text_object_editor(info.cdp, NOOBJECT, info.category, info.folder_name, OPEN_IN_EDCONT);
      strcpy(info.object_name, dlg.name.mb_str(*info.cdp->sys_conv));
      info.flags = dlg.mType->GetSelection() ? CO_FLAG_STS_CASC : 0;
      info.syscat = SYSCAT_OBJECT;
      info.object_count = 1;
      if (cd_Insert_object(info.cdp, info.object_name, CATEG_STSH, &info.objnum))
        { cd_Signalize(info.cdp);  return false; }
      Set_object_flags(info.cdp, info.objnum, CATEG_STSH, info.flags);
      bool alt_modif = get_stsh_editor_path((info.flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_STS_CASC).IsEmpty();
      modify_object(parent, info, alt_modif);
      return true;  // this calls AddComponent()
    }
#endif
  }
  return false;
}

const char * tabtab_form =
"TABTAB TABLEVIEW 0 0 445 316 "
"STYLE 688193 "
"CAPTION 'List of Tables' "
"BEGIN "
" LTEXT 'Deleted?' 5 0 4 112 28 0 PANE PAGEHEADER ENABLED false "
//" CHECKBOX '' 6 0 0 108 24 196613 ACCESS deleted "
" LTEXT 6 0 0 108 24 65536 VALUE deleted=true?'Deleted':deleted=false?'Valid':'Free'"
" LTEXT 'Table name' 3 120 4 140 28 0 PANE PAGEHEADER "
" LTEXT 7 120 4 140 20 65536 VALUE tab_name "
" LTEXT 'Application' 4 264 4 148 28 0 PANE PAGEHEADER "
" LTEXT 8 264 4 148 20 65536 VALUE apl_uuid "
"END";

const char * objtab_form =
"OBJTAB TABLEVIEW 0 0 566 317 "
"STYLE 688193 "
"CAPTION 'List of Objects' "
"BEGIN "
" LTEXT 'Deleted?' 5 0 4 112 28 0 PANE PAGEHEADER ENABLED false "
" CHECKBOX '' 6 0 0 108 24 196613 ACCESS deleted "
//" LTEXT 6 0 0 108 24 65536 VALUE deleted=true?'Deleted':deleted=false?'Valid':'Free'"
" LTEXT 'Category' 9 120 4 116 28 0 PANE PAGEHEADER "
//" LTEXT 10 120 4 116 20 65536 VALUE category "
" COMBOBOX 10 120 4 116 20 10551363 0 ACCESS category ENABLED false"
"  ENUMERATE 'View',2'View(Link)',130'Program',4'Program(Link)',132'Code',5'Query',3'Query(Link)',131'Menu',6'Menu(Link)',134'Application',7'User',1'Picture',8'Picture(Link)',136 "
"   'Role',10'ODBC connection',11'Relation',12'Drawing',13'Graph',14'Replication relation',15'Routine',16'Trigger',17'WWW object',18'Folder',19'Sequence',20'Fulltext',21'Domain',22"
"   'Role (Link)',138'ODBC connection (Link)',139'Relation (Link)',140'Drawing (Link)',141'Graph (Link)',142'Replication relation (Link)',143'Routine (Link)',144'Trigger (Link)',145'WWW object (Link)',146'Sequence (Link)',148'Fulltext (Link)',149'Domain (Link)',150"
"   'Style Sheet',23'XML Form'24"
" LTEXT 'Object name' 3 240 4 140 28 0 PANE PAGEHEADER "
" LTEXT 7 240 4 140 20 65536 VALUE obj_name "
" LTEXT 'Application' 4 384 4 148 28 0 PANE PAGEHEADER "
" LTEXT 8 384 4 148 20 65536 VALUE apl_uuid "
"END";

const char * usertab_form =
"usertab TABLEVIEW 0 10 630 299 "
"STYLE 688193 HDFTSTYLE 3276800 CAPTION 'Table of Users and Groups' "
"BEGIN"
" LTEXT 'Deleted?' 5 0 0 54 24 0 0 PANE PAGEHEADER ENABLED false "
" CHECKBOX 6 0 0 54 24 196613 0 ACCESS deleted"
//" LTEXT 6 0 0 54 24 65536 VALUE deleted=true?'Deleted':deleted=false?'Valid':'Free'"
" LTEXT 7 54 0 91 24 0 0 VALUE logname"
" LTEXT 'Login name' 8 54 0 91 24 0 0 PANE PAGEHEADER"
" LTEXT 'Category' 11 145 0 57 24 0 0 PANE PAGEHEADER"
" COMBOBOX 4 145 0 57 68 10551363 0 ACCESS category ENABLED false ENUMERATE 'User',1'Group',9"
" LTEXT 9 202 0 110 24 0 0 PRECISION 1 VALUE passcre"
" LTEXT 'Password created' 10 202 0 110 24 0 0 PANE PAGEHEADER"
" LTEXT 12 341 0 161 24 0 0 VALUE usr_uuid"
" LTEXT 'Identification' 13 341 0 161 24 0 0 PANE PAGEHEADER"
" END";

#ifdef GETTEXT_STRING  // mark these strings for translation
wxTRANSLATE("List of Tables")
wxTRANSLATE("List of Objects")
wxTRANSLATE("Table of Users and Groups")
wxTRANSLATE("Deleted?")
wxTRANSLATE("Table name")
wxTRANSLATE("Application")
wxTRANSLATE("Category")
wxTRANSLATE("Object name")
wxTRANSLATE("Login name")
wxTRANSLATE("Password created")
wxTRANSLATE("Identification")
#endif

const char * srvtab_form =
"srvtab TABLEVIEW 0 0 618 265 "
"STYLE 688193 HDFTSTYLE 3276800 CAPTION 'Tabulka of replication servers' "
"BEGIN"
" LTEXT 'Deleted?' 17 0 0 56 24 0 0 PANE PAGEHEADER ENABLED false "
" CHECKBOX 18 0 0 56 24 196613 0 ACCESS deleted"
//" LTEXT 18 0 0 56 24 65536 VALUE deleted=true?'Deleted':deleted=false?'Valid':'Free'"
" LTEXT 5 56 0 126 24 0 0 VALUE srvname"
" LTEXT 'Server name' 6 56 0 126 24 0 0 PANE PAGEHEADER"
" CTEXT 11 182 0 100 24 1 0 VALUE recv_num"
" LTEXT 'Last packet recvd' 12 182 0 100 24 0 0 PANE PAGEHEADER"
" CTEXT 13 282 0 109 24 1 0 VALUE send_num"
" LTEXT 'Last packet sended' 14 282 0 109 24 0 0 PANE PAGEHEADER"
" LTEXT 'Acknowl?' 3 391 0 62 20 0 0 PANE PAGEHEADER"
" CTEXT 4 391 0 62 24 1 0 VALUE ackn"
" LTEXT 7 453 0 255 24 0 0 VALUE addr1"
" LTEXT 'Server address 1' 8 453 0 255 24 0 0 PANE PAGEHEADER"
" LTEXT 9 708 0 252 24 0 0 VALUE addr2"
" LTEXT 'Server address 2' 10 708 0 252 24 0 0 PANE PAGEHEADER"
" LTEXT 15 960 0 167 24 0 0 VALUE srv_uuid"
" LTEXT 'Server identification' 16 960 0 167 24 0 0 PANE PAGEHEADER "
"END";

const char * repltab_form =
"repltab TABLEVIEW 0 0 631 299 "
"STYLE 688193 HDFTSTYLE 3276800 CAPTION 'Tabulka of replication rules' "
"BEGIN"
" LTEXT 'Deleted?' 4 0 0 57 24 0 0 PANE PAGEHEADER ENABLED false "
" CHECKBOX 18 0 0 57 24 196613 0 ACCESS deleted"
//" LTEXT 18 0 0 57 24 65536 VALUE deleted=true?'Deleted':deleted=false?'Valid':'Free'"
" LTEXT 5 57 0 160 24 0 0 VALUE source_uuid"
" LTEXT 'Source server' 6 57 0 160 24 0 0 PANE PAGEHEADER"
" LTEXT 7 217 0 157 24 0 0 VALUE dest_uuid"
" LTEXT 'Destination server' 8 217 0 157 24 0 0 PANE PAGEHEADER"
" LTEXT 9 374 0 158 24 0 0 VALUE appl_uuid"
" LTEXT 'Application' 10 374 0 158 24 0 0 PANE PAGEHEADER"
" LTEXT 11 532 0 110 24 0 0 VALUE tabname"
" LTEXT 'Table' 12 532 0 110 24 0 0 PANE PAGEHEADER"
" LTEXT 13 642 0 99 24 0 0 VALUE gcr_limit"
" LTEXT 'Update counter' 14 642 0 99 24 0 0 PANE PAGEHEADER"
" RTEXT 15 741 0 103 24 2 0 VALUE next_repl_time"
" LTEXT 'Next replication' 16 741 0 103 24 0 0 PANE PAGEHEADER"
" LTEXT 17 844 0 145 60 65568 0 ACCESS replcond"
" LTEXT 'Replication condition' 3 844 0 145 24 0 0 PANE PAGEHEADER "
"END";

const char * keytab_form =
"keytab TABLEVIEW 0 0 629 299 "
"STYLE 688193 HDFTSTYLE 3276800 CAPTION 'Tabulka of public keys' "
"BEGIN"
" LTEXT 'Deleted?' 13 0 0 55 24 0 0 PANE PAGEHEADER ENABLED false "
" CHECKBOX 14 0 0 55 24 196613 0 ACCESS deleted"
//" LTEXT 14 0 0 55 24 65536 VALUE deleted=true?'Deleted':deleted=false?'Valid':'Free'"
" LTEXT 'Key Identification' 3 55 0 167 20 0 0 PANE PAGEHEADER"
" LTEXT 4 55 0 167 24 0 0 VALUE key_uuid"
" LTEXT 5 222 0 168 24 0 0 VALUE user_uuid"
" LTEXT 'Key owner (user)' 6 222 0 168 24 0 0 PANE PAGEHEADER"
" LTEXT 7 390 0 117 24 0 0 VALUE credate"
" LTEXT 'Key created' 8 390 0 117 24 0 0 PANE PAGEHEADER"
" LTEXT 9 507 0 112 24 0 0 VALUE expdate"
" LTEXT 'Expiration date' 10 507 0 112 24 0 0 PANE PAGEHEADER"
" CHECKBOX 'certific' 11 619 0 81 24 196613 0 ACCESS certific ENABLED false"
" LTEXT 'Is owner the CA?' 12 619 0 81 24 0 0 PANE PAGEHEADER "
"END";

bool open_object_alt(wxWindow * parent, item_info & info)
{ switch (info.category)
  {
#ifdef WINS
#ifdef XMLFORMS
    case CATEG_XMLFORM:
    { if (!check_xml_library()) break;
     // prepare the parameters:
      t_dad_param_dynar params;
      tobjname dadname;  char dadref[1+OBJ_NAME_LEN+10];
      if (link_GetDadFromXMLForm(info.cdp, info.objnum, dadname))
        { cd_Signalize(info.cdp);  break; }
      if (*dadname)
      { dadref[0]='*';  strcpy(dadref+1, dadname);
        t_dad_param * pars;  int parcount;
        if (!link_Get_DAD_params(info.cdp, dadref, &pars, &parcount))
          break;
       // copy parametres:
        for (int i=0;  i<parcount;  i++)
        { t_dad_param * param = params.next();
          memcpy(param, &pars[i], sizeof(t_dad_param));
        }
        corefree(pars);
      }
      if (params.count())
      { XMLParamsDlg dlg(&params, info.cdp->sys_conv, NULL);
        dlg.Create(parent);
        if (dlg.ShowModal()!=wxID_OK) break;
      }
     // compose the URL: 
      wxString url;
      char host_name[70+1], host_path_prefix[70+1];
      sig32 emulating, using_emul;  char buf[10];
      cd_Get_server_info(info.cdp, OP_GI_WEB_RUNNING, &emulating, sizeof(emulating));
      if (emulating) cd_Get_property_value(info.cdp, sqlserver_owner, "UsingEmulation", 0, buf, sizeof(buf), NULL);
      using_emul = *buf=='1';
      cd_Get_property_value(info.cdp, sqlserver_owner, emulating && using_emul ? "WebPort" : "ExtWebServerPort", 0, buf, sizeof(buf), NULL);
      cutspaces(buf);
      if (!strcmp(buf, "80")) *buf=0;
      cd_Get_property_value(info.cdp, sqlserver_owner, "HostName", 0, host_name, sizeof(host_name));
      cd_Get_property_value(info.cdp, sqlserver_owner, "HostPathPrefix", 0, host_path_prefix, sizeof(host_path_prefix));
      if (!*buf)
        url.Printf(wxT("http://%s/%s602.php/"), wxString(host_name, *wxConvCurrent), wxString(host_path_prefix, *wxConvCurrent));
      else
        url.Printf(wxT("http://%s:%s/%s602.php/"), wxString(host_name, *wxConvCurrent), wxString(buf, *wxConvCurrent), wxString(host_path_prefix, *wxConvCurrent));
      url = url + wxString(info.server_name, *wxConvCurrent) + wxT("/") +
                  info.schema_name_str() + wxT("/COMPOSE/") + info.object_name_str();
      if (*dadname) url = url + wxT("!") + wxString(dadname, *info.cdp->sys_conv);
     // add parameters:
      if (params.count())
        for (int i=0;  i<params.count();  i++)
          url=url+(!i ? wxT("?") : wxT("&")) + wxString(params.acc0(i)->name, *info.cdp->sys_conv) + wxT("=")+wxString(params.acc0(i)->val);
#if 0  // I need the IE specifically
      return wxLaunchDefaultBrowser(url);  // does not work well on MWS+IE
#else
      STARTUPINFO         si;
      PROCESS_INFORMATION pi;
      memset(&si, 0, sizeof STARTUPINFO);
      memset(&pi, 0, sizeof PROCESS_INFORMATION);
      si.cb = sizeof(STARTUPINFO);
      si.dwFlags |= STARTF_USESHOWWINDOW;  si.wShowWindow = SW_SHOWNORMAL; 
      HKEY hKey;  wchar_t wbuf[MAX_PATH];
      wcscpy(wbuf, wxT("c:\\Program Files\\Internet Explorer\\iexplore.exe"));
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, wxT("Software\\Classes\\Applications\\iexplore.exe\\shell\\open\\command"), 0, KEY_READ, &hKey)==ERROR_SUCCESS)
      { DWORD key_len=sizeof(buf);
        RegQueryValueEx(hKey, NULL, NULL, NULL, (BYTE*)wbuf, &key_len);
        RegCloseKey(hKey);
      }
      wchar_t * p = wbuf;
      while (*p && *p!='%') p++;  *p=0;  // removes the %1
      wxString cmd = wbuf + wxString(wxT(" "))+url;
      if (CreateProcess(NULL, (wchar_t*)cmd.c_str(), NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_PROCESS_GROUP|NORMAL_PRIORITY_CLASS,
                        NULL, NULL, &si, &pi)) 
      { CloseHandle(pi.hThread);  
        CloseHandle(pi.hProcess);
      }
      else
      { GetLastError();
        error_box(_("Cannot start the Internet explorer."));
      }
#endif
    }
#endif
#endif
    default:
      break;
  }
  return false;
}
bool open_object(wxWindow * parent, item_info & info)
{
  switch (info.category)
  {
    case CATEG_TABLE:
      if (info.cdp && SYSTEM_TABLE(info.objnum))
      { const char * src;
        if      (info.objnum==TAB_TABLENUM)
          src = tabtab_form;
        else if (info.objnum==OBJ_TABLENUM)
          src = objtab_form;
        else if (info.objnum==USER_TABLENUM)
          src = usertab_form;
        else if (info.objnum==SRV_TABLENUM)
          src = srvtab_form;
        else if (info.objnum==REPL_TABLENUM)
          src = repltab_form;
        else if (info.objnum==KEY_TABLENUM)
          src = keytab_form;
        else break;
        open_form(NULL, info.cdp, src, NOOBJECT, NO_INSERT|DEL_RECS|COUNT_RECS|NO_EDIT);
        break;
      }
      // cont.
    case CATEG_CURSOR:
      open_data_grid(NULL, info.xcdp(), info.category, info.objnum, info.object_name, info.category==CATEG_TABLE && IsCtrlDown(), info.schema_name);
      break;
#ifdef WINS
#ifdef XMLFORMS
    case CATEG_XMLFORM:  // for parametrized DAD in the form making the export here
    { if (!check_xml_library()) break;
     // get DAD and parameter information:
      t_dad_param_dynar params;
      tobjname dadname;  char dadref[1+OBJ_NAME_LEN+10];
      if (link_GetDadFromXMLForm(info.cdp, info.objnum, dadname))
        { cd_Signalize(info.cdp);  break; }
      if (*dadname)
      { dadref[0]='*';  strcpy(dadref+1, dadname);
        t_dad_param * pars;  int parcount;
        if (!link_Get_DAD_params(info.cdp, dadref, &pars, &parcount))
        { cd_Signalize(info.cdp);
          break;
        }
       // copy parametres:
        for (int i=0;  i<parcount;  i++)
        { t_dad_param * param = params.next();
          memcpy(param, &pars[i], sizeof(t_dad_param));
        }
        corefree(pars);
      }
     // edit parameters, if the DAD contains any:
      char * data = NULL;
      if (params.count())
      { XMLParamsDlg dlg(&params, info.cdp->sys_conv, NULL);
        dlg.Create(parent);
        if (dlg.ShowModal()!=wxID_OK) break;
       // export data from the DAD:
        struct t_clivar * hostvars = NULL;  int hostvars_count;
        hostvars_from_params(params.acc0(0), params.count(), hostvars, hostvars_count);
        BOOL res;
        { wxBusyCursor wait;
          res = link_Export_to_XML_buffer_alloc(info.cdp, dadref, &data, NOOBJECT, hostvars, hostvars_count);
        }
        if (!res) 
          { cd_Signalize2(info.cdp, parent);  corefree(data);  break; }
       // merge form and data:
      }
      char xml_form_ref[1+OBJ_NAME_LEN+1];
      *xml_form_ref='*';  strcpy(xml_form_ref+1, info.object_name);
      char * final_form = link_Merge_XML_form(info.cdp, xml_form_ref, data);
      corefree(data);
      if (!final_form)
        error_box(_("The form is not signed."));
      else
      { fill_XML_form(info.cdp, final_form);
        corefree(final_form);
      }
      break;
    }
#endif
#endif
  }
  return true;
}

#ifdef XMLFORMS
void call_ext_editor(const item_info &info, wxString editor_path)
{ 
  if (editor_path.IsEmpty()) return;
  wxString fName = wxFileName::CreateTempFileName(wxEmptyString);
  bool OK = load_XML_form(info.cdp, fName, info.objnum);  // copies object definition to a file
  if (OK)
  { time_t CreTime = wxFileModificationTime(fName); 
    wxString cmd = editor_path;
    if (editor_path.Find(wxT("%1"))==-1)
      cmd = cmd + wxT(" ") + fName;
    else
      cmd.Replace(wxT("%1"), fName, true);
    wxExecute(cmd, wxEXEC_SYNC);
    time_t ModTime = wxFileModificationTime(fName);
    if (ModTime != 0 && ModTime > CreTime)
    { OK = save_XML_form(info.cdp, fName, info.objnum);
      if (OK)
        component_change_notif(info.cdp, CATEG_XMLFORM, info.object_name, info.schema_name, info.flags, info.objnum);
    }
  }
  DeleteFileA(fName.mb_str(*wxConvCurrent));  // for LINUX compatibility
  if (!OK)
    cd_Signalize2(info.cdp, wxGetApp().frame);
}
#endif

bool modify_object(wxWindow * parent, item_info & info, bool alternative)
{ 
  if (info.syscat!=SYSCAT_OBJECT) 
    { wxBell();  return false; }  // used when category selected
 // if the gui designer is open, activate it instead of openning again:
  for (competing_frame * curr_frame = competing_frame_list;  curr_frame;  curr_frame=curr_frame->next)
  { if (curr_frame->cf_style!=ST_CHILD)
      if (curr_frame->IsDesignerOf(info.cdp, info.category, info.objnum))
      { curr_frame->to_top();
        return true;
      }
  }
 // open the proper designer:
  switch (info.category)
  {
    case CATEG_TABLE:
      if (IsCtrlDown())
        open_text_object_editor(info.cdp, TAB_TABLENUM, info.objnum, OBJ_DEF_ATR, info.category, info.folder_name, 0);  
      else  
      {// close Data page in the output window first:
        if (wxGetApp().frame->output_window)
          wxGetApp().frame->output_window->GetPage(DATA_PAGE_NUM)->DestroyChildren();
       // open the table designer:
        open_table_designer(info.cdp, info.objnum, info.folder_name, info.schema_name);  // folder_name used in SaveAs
      }  
      break;
    case CATEG_DOMAIN:
      start_domain_designer(info.cdp, info.objnum, info.folder_name);
      break;
    case CATEG_PGMSRC:
      switch (info.flags & CO_FLAGS_OBJECT_TYPE)
      { case CO_FLAG_TRANSPORT:
          start_transport_designer(info.cdp, info.objnum, info.folder_name, NULL);  break;
        case CO_FLAG_XML:
          if (!check_xml_library()) break;
          start_dad_designer(info.cdp, info.objnum, info.folder_name, NULL, NULL);  break;
        default:
          open_text_object_editor(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_DEF_ATR, info.category, info.folder_name, alternative ? 0 : OPEN_IN_EDCONT);  break;
      }
      break;
    case CATEG_SEQ:
      start_sequence_designer(info.cdp, info.objnum, info.folder_name);
      break;
    case CATEG_CURSOR:  // for openning the source in a separate window an other functon is used
      if (!alternative)
      { start_query_designer(info.cdp, info.objnum, info.folder_name);
        break;
      }
      // cont.
    case CATEG_TRIGGER:  case CATEG_PROC:  
      open_text_object_editor(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_DEF_ATR, info.category, info.folder_name, alternative ? 0 : OPEN_IN_EDCONT);
      break;
    case CATEG_DRAWING:
      start_diagram_designer(info.cdp, info.objnum, info.folder_name);
      break;
    case CATEG_INFO:
    { uns32 state;  
      if (cd_Get_server_info(info.cdp, OP_GI_LICS_FULLTEXT, &state, sizeof(state)) || !state)
        { client_error(info.cdp, NO_FULLTEXT_LICENCE);  cd_Signalize(info.cdp);  break; }
      start_fulltext_designer(info.cdp, info.objnum, info.folder_name);
      break;
    }  
    case CATEG_USER:
      if (info.objnum==ANONYMOUS_USER)
        info_box(_("The Anomymous user account does not have any editable properties.\nThis account can be disabled in the Security section."));
      else
      { bool can_open = info.objnum==info.cdp->logged_as_user || cd_Am_I_security_admin(info.cdp) || cd_Am_I_config_admin(info.cdp);
        if (!can_open)
        { uns8 priv_buf[PRIVIL_DESCR_SIZE];
          if (!cd_GetSet_privils(info.cdp, info.cdp->logged_as_user, CATEG_USER, USER_TABLENUM, info.objnum, OPER_GETEFF, priv_buf) &&
               HAS_WRITE_PRIVIL(priv_buf, USER_ATR_LOGNAME))
            can_open=true;
        }
        if (can_open)  // can_open if any part of the dialog is editable
        { UserPropDlg dlg(info.cdp, info.objnum);
          dlg.Create(wxGetApp().frame);
          dlg.ShowModal();
        }
        else
          error_box(_("You do not have any rights to use this user account."));
      }
      break;
#ifdef XMLFORMS
#ifdef WINS
    case CATEG_XMLFORM:
      modi_XML_form(info);
      break;
#endif
    case CATEG_STSH:
      if (alternative)
        open_text_object_editor(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_DEF_ATR, info.category, info.folder_name, 0);
      else
        call_ext_editor(info, get_stsh_editor_path((info.flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_STS_CASC));
      break;
#endif
  }
    //if (wxWindow::GetCapture() != NULL)
    //  error_box("wx capture mod");
    //if (::GetCapture() != NULL)
    //  error_box("msw capture mod");

  return true;
}

void call_routine(wxWindow * parent, cdp_t cdp, tobjnum objnum)
// Creates the modeless dialog for calling a SQL routine.
{ 
 // when a call dialog for a routine is open, activate it: 
  if (CallRoutineDlg::global_inst)
  { if (CallRoutineDlg::global_inst->IsIconized()) { CallRoutineDlg::global_inst->Iconize(false);  CallRoutineDlg::global_inst->Restore(); }  // don't know which is correct
    CallRoutineDlg::global_inst->Raise();
   // when a call dialog for other routine is open, stop: 
    if (CallRoutineDlg::global_inst->cdp!=cdp || CallRoutineDlg::global_inst->objnum!=objnum)
    //if (CallRoutineDlg::counter>0)
      error_box(_("You must complete the pending run first!"));  
    return;
  }
  CallRoutineDlg * dlg = new CallRoutineDlg(cdp, objnum);
  if (!dlg) return;
#if WINS  // when parent is specified then the dialog is modal
  dlg->Create(wxGetApp().frame);   // frame must be the parent! otherwise is modal!
  //dlg->SetWindowStyleFlag(dlg->GetWindowStyleFlag() | wxDIALOG_NO_PARENT);
#else
  dlg->Create(wxGetApp().frame);  // does not keep the dialog on the top of the main appl window, but closes it when the main window is closed!
#endif
  // wxTAB_TRAVERSAL causes the dialog to grab Return key and makes impossible to edit in the grid!!
  dlg->SetWindowStyleFlag(dlg->GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);
  dlg->Show();
}

void run_pgm9(cdp_t cdp, const char * srcname, tobjnum srcobj, BOOL recompile);

bool run_object(wxWindow * parent, item_info & info)
{
  switch (info.category)
  {
    case CATEG_PROC:
      if (!stricmp(info.object_name, MODULE_GLOBAL_DECLS))  // cannot run
        { error_box(_("The global declarations cannot be executed."));  break; }
      call_routine(parent, info.cdp, info.objnum);
      break;
    case CATEG_PGMSRC:
      switch (info.flags & CO_FLAGS_OBJECT_TYPE)
      { case CO_FLAG_TRANSPORT:
        {// create transport object, load transport definition:
          t_ie_run ier(info.cdp, false);
          if (ier.load_design(info.objnum) && find_servers(&ier, parent)) 
          {// allow updating source and target names in a dialog
            TransportDataDlg dlg(&ier);
            dlg.Create(parent);
            if (dlg.ShowModal()==wxID_OK)    
              if (!ier.analyse_dsgn())
                cd_Signalize(info.cdp);
              else
              { if (ier.creates_target_file())
                  if (!can_overwrite_file(ier.outpath, NULL))
                    break;  // target file exists and overwrite not confirmed
                BOOL res;
                { wxBusyCursor wait;
                  res=ier.do_transport();
                  ier.run_clear();
                }
                if (res) 
                { info_box(_("Data transport completed."));
                  if (ier.creates_target_table())
                    Add_new_component(info.cdp, CATEG_TABLE, info.cdp->sel_appl_name, ier.outpath, ier.out_obj, 0, "", false);
                }
                else cd_Signalize(info.cdp);
              }
          }
          break;
        }
        case CO_FLAG_XML:
        { if (!check_xml_library()) break;
          XMLRunDlg dlg(info);
          dlg.Create(parent);
          if (dlg.ShowModal()==wxID_OK)
            info_box(_("Data transport completed."));
          break;
        }
        case 0:  // client program
          run_pgm9(info.cdp, info.object_name, info.objnum, FALSE);
          break;
        default:
          break;
      }
      break;
    case CATEG_INFO:
    { uns32 state;  
      if (cd_Get_server_info(info.cdp, OP_GI_LICS_FULLTEXT, &state, sizeof(state)) || !state)
        { client_error(info.cdp, NO_FULLTEXT_LICENCE);  cd_Signalize(info.cdp);  break; }
      FtxSearchDlg dlg(info.cdp, info.objnum);
      dlg.Create(wxGetApp().frame);
      dlg.ShowModal();
      break;
    }
  }
  return true;
}

bool delete_server(wxWindow * parent, item_info & info, bool confirm)
{ if (!*info.server_name || !info.flags)  // no server or a local database
  { wxBell();  return false; }
  t_avail_server * as = find_server_connection_info(info.server_name);
  if (!as || as->notreg)
  { error_box(_("This server is not registered locally."));
    return false;
  }
  wxString box_text;
  box_text.Printf(_("Do you want to remove the local registration of SQL Server %s?"), wxString(info.server_name, *wxConvCurrent).c_str());
  wxMessageDialog md1(parent, box_text, STR_WARNING_CAPTION, wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION);
  if (md1.ShowModal()==wxID_NO) return false;
  if (!srv_Unregister_server(info.server_name, as && as->private_server))
  { error_box(_("Error unregistering the server/service, your privileges may not be sufficient."));
    return false;
  }
  // remove it from the list of servers
  remove_server(info.server_name);
  return true;
}

bool delete_appl(wxWindow * parent, item_info & info, bool confirm)
{ wchar_t wc[64];
  if (!yesno_boxp(_("Do you really want to delete the %s %s?"), parent, CCategList::Lower(info.cdp, CATEG_APPL, wc), TowxChar(info.schema_name, info.cdp->sys_conv)))
    return false;
  wxMessageDialog md2(parent, _("WARNING: You are about to delete the application!\nDo you really want to delete the entire application including all table data?\nOnce the application has been deleted, it CANNOT be restored!"),
    STR_WARNING_CAPTION, wxYES_NO|wxNO_DEFAULT|wxICON_EXCLAMATION);
  if (md2.ShowModal()==wxID_NO) return false;
  if (cd_Set_application_ex(info.cdp, info.schema_name, TRUE)) { cd_Signalize(info.cdp);  return false; }
  char comm[20+OBJ_NAME_LEN];
  strcpy(comm, "DROP SCHEMA `");  strcat(comm, info.schema_name);  strcat(comm, "`");
  if (cd_SQL_execute(info.cdp, comm, NULL))
  { cd_Signalize(info.cdp);  return false; }
  cd_Set_application_ex(info.cdp, "", FALSE);   // Clear schema name and object cache
  return true;
}

///////////////////////////////////// syntax checking /////////////////////////////////////////////
// Syntax report item has associated object number or 0 if object cannot be edited.
#include "compil.h"


static void report_syntax_error(cdp_t cdp, SearchResultList * hReportList, tcateg cat, const char * objname, int errnum, tobjnum objnum)
{ wxString tx, msg;
  if (errnum<128)
  { wxChar _buf[500];
    Get_warning_num_textW(cdp, errnum, _buf, sizeof(_buf)/sizeof(wxChar));
    tx=_buf;
  }
  else tx=Get_error_num_textWX(cdp, errnum);
  wchar_t wc[64];
  msg = wxString(CCategList::CaptFirst(cdp, cat, wc)) + wxT(" ") + wxString(objname, *cdp->sys_conv) + wxT(": ") + tx;
  for (int i = 0;  i<msg.Length();  i++)
    if (msg[(unsigned)i]<' ') 
      msg.GetWritableChar(i)=' ';
  search_result * data = new search_result(cat, objnum, 0);
  hReportList->Append(msg, data);
}

void Check_component_syntax(cdp_t cdp, const char * schema_name, SearchResultList * hReportList, wxTextCtrl * hObjectName, wxTextCtrl * hObjectCount)
{ unsigned object_count=0;
  char query[100+OBJ_NAME_LEN+2*UUID_SIZE];  tcursnum cursnum;  trecnum rec, limit, trec;
  { t_temp_appl_context tac(cdp, schema_name);
   // check for admin / author privils:  
    if (!cd_Am_I_appl_admin(cdp) && !cd_Am_I_appl_author(cdp) && !cd_Am_I_db_admin(cdp) && !cd_Am_I_config_admin(cdp))
      { error_box(_("You must be an administrator or the author of the application to start this action."));  return; }
   // start writing results:
    wxBeginBusyCursor();
    hReportList->ClearAll();
    strcpy(hReportList->server_name, cdp->locid_server_name);
    wxString msg;
    msg.Printf(_("Validating objects in application %s:"), wxString(cdp->sel_appl_name, *cdp->sys_conv).c_str());
    hReportList->Append(msg, (void*)0);
   // pass tables:
    make_apl_query(query, "TABTAB", cdp->sel_appl_uuid);
    if (cd_Open_cursor_direct(cdp, query, &cursnum)) cd_Signalize(cdp);
    else
    { if (cd_Rec_cnt(cdp, cursnum, &limit)) cd_Signalize(cdp);
      else for (rec=0;  rec<limit;  rec++)
      { tobjname objname;  tcateg cat;  uns16 flags;
        if      (cd_Read(cdp, cursnum, rec, APLOBJ_NAME,  NULL, objname)) cd_Signalize(cdp);
        else if (cd_Read(cdp, cursnum, rec, APLOBJ_CATEG, NULL, &cat))    cd_Signalize(cdp);
        else if (cd_Read(cdp, cursnum, rec, APLOBJ_FLAGS, NULL, &flags))  cd_Signalize(cdp); 
        else if (cd_Translate(cdp, cursnum, rec, 0, &trec)) cd_Signalize(cdp);
        else if (!(cat & IS_LINK) && !(flags & CO_FLAG_ODBC))  // not a link
        if (not_encrypted_object(cdp, cat, trec))
        { if (hObjectName) { hObjectName->SetValue(wxString(objname, *cdp->sys_conv));  wxGetApp().Yield(); }
  #ifdef STOP  // I want to report warnings, too
          trecnum cnt;
          if (cd_Rec_cnt(cdp, trec, &cnt))
            report_syntax_error(cdp, hReportList, cat, objname, cd_Sz_error(cdp), trec);
  #endif
          tptr def=cd_Load_objdef(cdp, trec, CATEG_TABLE, NULL);
          if (def)
          { tptr def2 = (tptr)corealloc(strlen(def)+2, 55);
            if (def2)
            { def2[0]=(char)QUERY_TEST_MARK;
              strcpy(def2+1, def);
              if (cd_SQL_execute(cdp, def2, NULL))
                report_syntax_error(cdp, hReportList, cat, objname, cd_Sz_error(cdp), trec);
              else if (cd_Sz_warning(cdp)==ERROR_IN_CONSTRS || cd_Sz_warning(cdp)==ERROR_IN_DEFVAL)
                report_syntax_error(cdp, hReportList, cat, objname, cd_Sz_warning(cdp), trec);
              corefree(def2);
            }
            corefree(def);
          }

          object_count++;
          if (hObjectCount) { msg.Printf(wxT("%u"), object_count);  hObjectCount->SetValue(msg); }
        }
      }
      cd_Close_cursor(cdp, cursnum);
    }
   // pass other objects:
    make_apl_query(query, "OBJTAB", cdp->sel_appl_uuid);
    if (cd_Open_cursor_direct(cdp, query, &cursnum)) cd_Signalize(cdp); 
    else
    { if (cd_Rec_cnt(cdp, cursnum, &limit)) cd_Signalize(cdp); 
      else for (rec=0;  rec<limit;  rec++)
      { tobjname objname;  tcateg cat;  uns16 flags;  
        if      (cd_Read(cdp, cursnum, rec, APLOBJ_NAME,  NULL, objname)) cd_Signalize(cdp); 
        else if (cd_Read(cdp, cursnum, rec, APLOBJ_CATEG, NULL, &cat))    cd_Signalize(cdp); 
        else if (cd_Read(cdp, cursnum, rec, APLOBJ_FLAGS, NULL, &flags))  cd_Signalize(cdp); 
        else if (cd_Translate(cdp, cursnum, rec, 0, &trec)) cd_Signalize(cdp); 
        else if (!(cat & IS_LINK))  // not a link
        if (not_encrypted_object(cdp, cat, trec))
        { if (hObjectName) { hObjectName->SetValue(wxString(objname, *cdp->sys_conv));  wxGetApp().Yield(); }
          switch (cat)
          { case CATEG_PROC:  case CATEG_TRIGGER:  case CATEG_CURSOR:
            { tptr def=cd_Load_objdef(cdp, trec, cat, NULL);
              if (def)
              { tptr def2 = (tptr)corealloc(strlen(def)+2, 55);
                if (def2)
                { def2[0]=(char)QUERY_TEST_MARK;
                  if (cat==CATEG_PROC && !stricmp(objname, MODULE_GLOBAL_DECLS)) def2[0]++;
                  strcpy(def2+1, def);
                  if (cd_SQL_execute(cdp, def2, NULL))
                    report_syntax_error(cdp, hReportList, cat, objname, cd_Sz_error(cdp), trec);
                  corefree(def2);
                }
                corefree(def);
              }
              break;
            }
            case CATEG_PGMSRC:
            { if ((flags & CO_FLAGS_OBJECT_TYPE)==CO_FLAG_INCLUDE ||
                  (flags & CO_FLAGS_OBJECT_TYPE)==CO_FLAG_TRANSPORT)
                break;  // no check
              if ((flags & CO_FLAGS_OBJECT_TYPE)==CO_FLAG_XML)
              { int line, column;
                //tptr def=cd_Load_objdef(cdp, trec, cat, NULL); -- do not load this, must convert to UTF-8
                //if (def)
                char dad_ref[1+OBJ_NAME_LEN+1];
                *dad_ref='*';  strcpy(dad_ref+1, objname);
                { BOOL res = link_Verify_DAD(cdp, dad_ref, &line, &column);
                  //corefree(def);
                  if (!res)
                  { cdp->last_error_is_from_client=wbtrue;
                    report_syntax_error(cdp, hReportList, cat, objname, cd_Sz_error(cdp), trec);
                  }
                }
                break;
              }
              tptr def=cd_Load_objdef(cdp, trec, cat, NULL);
              if (def)
              { compil_info xCI(cdp, def, DUMMY_OUT, program);
                int res=compile(&xCI);
                corefree(def);
                if (res) // compilation error, report it
                { cdp->last_error_is_from_client=wbtrue;
                  report_syntax_error(cdp, hReportList, cat, objname, res, trec);
                }
              }
              break;
            }
  #if 0
	    case CATEG_VIEW:
            { tptr def=cd_Load_objdef(cdp, trec, cat, NULL);
              if (def)
              { compil_info xCI(cdp, def, DUMMY_OUT, view_struct_comp);
                int res=compile(&xCI);
                corefree(def);
                if (res) // compilation error, report it
                { cdp->last_error_is_from_client=wbtrue;
                  report_syntax_error(cdp, hReportList, cat, objname, res, trec);
                }
              }
              break;
            }
            case CATEG_MENU:
            { tptr def=cd_Load_objdef(cdp, trec, cat, NULL);
              if (def)
              { int res=check_menu_def(cdp, def);
                if (res) // compilation error, report it
                { cdp->last_error_is_from_client=wbtrue;
                  report_syntax_error(cdp, hReportList, cat, objname, res, trec);
                }
                corefree(def);
              }
              break;
            }
  #endif
	    default:
              continue;
          }
          object_count++;
          if (hObjectCount) { msg.Printf(wxT("%u"), object_count);  hObjectCount->SetValue(msg); }
        } // not link
      }
      cd_Close_cursor(cdp, cursnum);
    }
   // write summary:
    msg.Printf(_("Validation completed. Number of checked objects: %d"), object_count);
    hReportList->Append(msg, (void*)0);
    hReportList->SetSelection(hReportList->GetCount()-1);
    hReportList->SetFocus();
    wxEndBusyCursor();
  }
}

void CPCopy(selection_iterator &si, bool Cut)
{
    if (si.info.syscat != SYSCAT_APPLTOP && si.info.syscat != SYSCAT_CATEGORY && (si.info.syscat != SYSCAT_OBJECT || (si.info.category == CATEG_TABLE && Cut)))
        return;
    t_copyobjctx ctx;
    memset(&ctx, 0, sizeof(t_copyobjctx));
    ctx.strsize = sizeof(t_copyobjctx);
    ctx.count   = si.total_count();
    if (Cut)
        ctx.flags = COF_CUT;
    if (!si.info.tree_info || si.info.IsCategory())
        strcpy(ctx.folder, si.info.folder_name);
    else if (*si.info.folder_name)
    {
        CObjectList ol(si.info.cdp, si.info.schema_name);
        ol.Find(si.info.folder_name, CATEG_FOLDER);
        strcpy(ctx.folder, ol.GetFolderName());
    }
    ctx.param     = &si;
    ctx.nextfunc  = get_next_sel_obj;
    CopySelObjects(si.info.cdp, &ctx);
}

////////////////////////// result list (search, validation) ///////////////////////////////
#include "fsed9.h"

BEGIN_EVENT_TABLE(SearchResultList, wxListBox)
    EVT_LISTBOX_DCLICK(-1, SearchResultList::OnDblClk)
END_EVENT_TABLE()

void SearchResultList::OnDblClk(wxCommandEvent & event)
{ int sel = GetSelection();
  if (sel==-1) return;
  search_result * data = (search_result *)GetClientData(sel);
  if (data==NULL) { wxBell();  return; } // "String not found" or such text double clicked
  if (data->categ==CATEG_USER || data->categ==CATEG_GROUP) return;  // transitive members of a role may be listed here
 // find server:
  t_avail_server * as = find_server_connection_info(server_name);
  if (!as || !as->cdp) 
    { unpos_box(_("The server is not connected."));  return; }
 // open the object in editor: ### replace by openning the GUI designer for tables, transports, diagrams?
  editor_type * edd = find_editor_by_objnum9(as->cdp, data->categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, data->objnum, OBJ_DEF_ATR, data->categ, true);
  if (!edd) return;  // internal error
  edd->position_on_offset(data->respos);
  edd->SetFocus();
}

void SearchResultList::ClearAll(void)
{ for (int i=0;  i<GetCount();  i++)
  { search_result * data = (search_result *)GetClientData(i);
    if (data) delete data;
  }
  Clear();
}
/////////////////////////////////// edit privils /////////////////////////////////////////////
class ObjectPrivilsDlg;

class t_privil_info
{ friend class ObjectPrivilsDlg;
  friend int create_list(wxListCtrl * mList, t_privil_info & pi);
 private:
  tobjnum * tbs;
  trecnum * recnums;   // may be NULL, if setting only global privils
  int       multioper;
  tobjnum   curr_subject;
  tcateg    curr_subject_categ;
  bool      record_privs;
  unsigned  attr_cnt, attr_cnt_upp, privil_descr_size;

 public:
  cdp_t     cdp;
  bool changed, obj_priv, sys_priv, new_privs, common_rw;
  ObjectPrivilsDlg * hDlg;
  t_privils_flex glob_pr;    // privileges: 1 granted, 0 not granted
  t_privils_flex glob_mask;  // 1 privilege is valid, 0 privilege is indeterminate
  t_privils_flex rec_pr;
  t_privils_flex rec_mask;
  t_privils_flex eff_pr;
  t_privils_flex eff_mask;
  t_privils_flex effrec_pr;
  t_privils_flex effrec_mask;
  uns8 basic_pr, basic_mask;
  int  atrbase, new_priv_start;

  t_privil_info(cdp_t init_cdp, tobjnum * init_tbs, trecnum * init_recnums, int init_multioper)
  { tbs=init_tbs;  cdp=init_cdp;  recnums=init_recnums;  multioper=init_multioper;  
    obj_priv=sys_priv=false;  record_privs=false;  new_privs=false;  common_rw=false;
    atrbase=1;  // shoud not be used
    attr_cnt=2;
    if (multioper!=MULTITAB)
    { const d_table * td= cd_get_table_d(cdp, tbs[0], CATEG_TABLE);
      if (td!=NULL)
      { //if (tbs[0]>=3) recnum=-1;
        if (td->tabdef_flags & TFL_REC_PRIVILS)
        { if (recnums!=NULL) record_privs=true; 
          new_privs=true;
        }
        atrbase=first_user_attr(td);
        attr_cnt = td->attrcnt;
        release_table_d(td);
      }
      if (tbs[0]==TAB_TABLENUM || tbs[0]==OBJ_TABLENUM || tbs[0]==USER_TABLENUM)
        if (!recnums || *recnums==NORECNUM) 
          { sys_priv=true;  new_privs=false; }
        else if (tbs[0]!=USER_TABLENUM)
          { obj_priv=true;  new_privs=false; }
    }
    else 
    { common_rw=true;
      for (int tabind=0;  tbs[tabind]!=NOOBJECT;  tabind++)
      { const d_table * td = cd_get_table_d(cdp, tbs[tabind], CATEG_TABLE);
        if (td!=NULL)
        { if (td->attrcnt > attr_cnt) attr_cnt = td->attrcnt;
          release_table_d(td);
        }
      }
    }
    attr_cnt_upp = attr_cnt > 255 ? attr_cnt : 255;
    privil_descr_size = SIZE_OF_PRIVIL_DESCR(attr_cnt);
    glob_pr.init(attr_cnt_upp);
    glob_mask.init(attr_cnt_upp);
    rec_pr.init(attr_cnt_upp);
    rec_mask.init(attr_cnt_upp);
    eff_pr.init(attr_cnt_upp);
    eff_mask.init(attr_cnt_upp);
    effrec_pr.init(attr_cnt_upp);
    effrec_mask.init(attr_cnt_upp);
    changed=false;
  }

  void init(wxWindow init_hDlg);
  void show_state(wxListCtrl * mList);
  bool load_state(wxOwnerDrawnComboBox * mSubject);
  void fill_subjects(tcateg set_subject_categ);
  bool set_relation(bool second);

  void set_subject_category(tcateg subject_categ, wxOwnerDrawnComboBox* mSubject, wxButton * mMemb1, wxButton * mMemb2, wxListCtrl * mList);
  void update_privils(wxListCtrl * mList, bool granting, bool record_level);

};

void t_privil_info::set_subject_category(tcateg subject_categ, wxOwnerDrawnComboBox* mSubject, wxButton * mMemb1, wxButton * mMemb2, wxListCtrl * mList)
{ curr_subject_categ=subject_categ;
 // list of subject names to combo:
  mSubject->Clear();
  move_object_names_to_combo(mSubject, cdp, subject_categ);
  mSubject->SetSelection(0);
  mSubject->SetValue(mSubject->GetString(0));
 // action names on buttons:
  switch (subject_categ)
  { case CATEG_USER:  mMemb1->SetLabel(_("  Groups containing the user  "));  mMemb2->SetLabel(_("  Roles containing the user  "));  break;
    case CATEG_GROUP: mMemb1->SetLabel(_("  Users in the group  "));          mMemb2->SetLabel(_("  Roles containing the group  "));  break;
    case CATEG_ROLE:  mMemb1->SetLabel(_("  Users in the role  "));      mMemb2->SetLabel(_("  Groups in the role  "));  break;
  }
 // load and show state:
  load_state(mSubject);
  show_state(mList);
}

bool t_privil_info::set_relation(bool second)
{ if (curr_subject==NOOBJECT) return false;
  switch (curr_subject_categ)
  { case CATEG_USER: return Edit_relation(cdp, (wxWindow*)hDlg, curr_subject_categ, curr_subject, second ? CATEG_ROLE  : CATEG_GROUP);
    case CATEG_GROUP:return Edit_relation(cdp, (wxWindow*)hDlg, curr_subject_categ, curr_subject, second ? CATEG_ROLE  : CATEG_USER);
    case CATEG_ROLE: return Edit_relation(cdp, (wxWindow*)hDlg, curr_subject_categ, curr_subject, second ? CATEG_GROUP : CATEG_USER);
  }
  return true;
}

void add_privils(const uns8 * source, uns8 * dest, uns8 * mask, bool first, unsigned privil_descr_size)
// Adds [source] do [dest] and sets the [mask].
{ if (first)
  { memcpy(dest, source, privil_descr_size);
    memset(mask, 0xff, privil_descr_size);
  }
  else
    for (int i=0;  i<privil_descr_size;  i++)
      mask[i] &= ~(source[i] ^ dest[i]);
}

bool t_privil_info::load_state(wxOwnerDrawnComboBox * mSubject)
{// find the current subject:
  tobjname subject_name;
  strcpy(subject_name, mSubject->/*GetString(mSubject->GetSelection())*/GetValue().mb_str(*cdp->sys_conv));
  if (!*subject_name)  // subject not selected
  { curr_subject = NOOBJECT;
    glob_pr.clear();
    glob_mask.clear();
    rec_pr.clear();
    rec_mask.clear();
    eff_pr.clear();
    eff_mask.clear();
    effrec_pr.clear();
    effrec_mask.clear();
    basic_pr=basic_mask=0;
    return true;
  }
  else // localize subject:
    if (cd_Find2_object(cdp, subject_name, NULL, curr_subject_categ, &curr_subject)) cd_Signalize(cdp);
 // load its privils:
  t_privils_flex priv_buf(attr_cnt_upp);  int tabind, recind;  bool first=true;
  for (tabind=recind=0;  first ||
          multioper==MULTITAB && tbs[tabind]!=NOOBJECT ||
          multioper==MULTIREC && recnums[recind]!=NORECNUM;
       first=false, (multioper==MULTITAB ? tabind++ : recind++))
  { trecnum therec=recnums==NULL ? NORECNUM : recnums[recind];
   // table global:
    if (!recind || multioper!=MULTIREC)
    { if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], NORECNUM, OPER_GET, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
      if (!common_rw) *priv_buf.the_map() &= ~(RIGHT_READ | RIGHT_WRITE);  // may cause problems when showing the removed privileges
      add_privils(priv_buf.the_map(), glob_pr.the_map(), glob_mask.the_map(), first, privil_descr_size);
    }
   // record:
    if (record_privs)
    { if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], therec, OPER_GET, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
      if (!common_rw) *priv_buf.the_map() &= ~(RIGHT_READ | RIGHT_WRITE);  // may cause problems when showing the removed privileges
      add_privils(priv_buf.the_map(), rec_pr.the_map(), rec_mask.the_map(), first, privil_descr_size);
    }
   // effective:
    if (true)
    { if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], NORECNUM, OPER_GETEFF, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
      if (!common_rw) *priv_buf.the_map() &= ~(RIGHT_READ | RIGHT_WRITE);  // may cause problems when showing the removed privileges
      add_privils(priv_buf.the_map(), eff_pr.the_map(), eff_mask.the_map(), first, privil_descr_size);
    }
   // rec effective:
    if (true)
    { if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], therec, OPER_GETEFF, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
      if (!common_rw) *priv_buf.the_map() &= ~(RIGHT_READ | RIGHT_WRITE);  // may cause problems when showing the removed privileges
      add_privils(priv_buf.the_map(), effrec_pr.the_map(), effrec_mask.the_map(), first, privil_descr_size);
    }
   // basic:
    if (new_privs && (!recind || multioper!=MULTIREC))
    { if (cd_GetSet_privils(cdp, EVERYBODY_GROUP, CATEG_GROUP, tbs[tabind], NORECNUM, OPER_GET, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
      if (first) { basic_pr=*priv_buf.the_map();  basic_mask=RIGHT_NEW_READ | RIGHT_NEW_WRITE | RIGHT_NEW_DEL; }
      else basic_mask &= ~(basic_pr ^ *priv_buf.the_map()); // same as in add_privils
    }
  }
  return true;
}

const wxChar * state(const uns8 * pr, const uns8 * mask, int nbyte, int bit)
{ if (!(mask[nbyte] & bit)) return _("Various");
  return (pr[nbyte] & bit) ? _("Yes") : _("No");
}

void t_privil_info::show_state(wxListCtrl * mList)
{ mList->Freeze();
  const d_table * td = NULL;  int a, row;
  if (!sys_priv && !obj_priv && multioper!=MULTITAB)
    td = cd_get_table_d(cdp, tbs[0], CATEG_TABLE);
 // system (insert):
  if (sys_priv)
    mList->SetItem(0, 1, state(glob_pr.the_map(), glob_mask.the_map(), 0, RIGHT_INSERT));
 // table:
  else if (obj_priv)
  { mList->SetItem(0, 1, state(rec_pr.the_map(), rec_mask.the_map(), 1+(OBJ_DEF_ATR-1)/4, 1 << ((OBJ_DEF_ATR-1)%4*2)));
    mList->SetItem(1, 1, state(rec_pr.the_map(), rec_mask.the_map(), 1+(OBJ_DEF_ATR-1)/4, 1 << ((OBJ_DEF_ATR-1)%4*2+1)));
    mList->SetItem(2, 1, state(rec_pr.the_map(), rec_mask.the_map(), 0, RIGHT_DEL));
    mList->SetItem(3, 1, state(rec_pr.the_map(), rec_mask.the_map(), 0, RIGHT_GRANT));
  }
  else
  { mList->SetItem(0, 1, state(glob_pr.the_map(), glob_mask.the_map(), 0, RIGHT_INSERT));
    mList->SetItem(1, 1, state(glob_pr.the_map(), glob_mask.the_map(), 0, RIGHT_DEL));
    mList->SetItem(2, 1, state(glob_pr.the_map(), glob_mask.the_map(), 0, RIGHT_GRANT));
    row=3;
    if (td) for (a=atrbase;  a<td->attrcnt; a++)
    { mList->SetItem(row++, 1, state(glob_pr.the_map(), glob_mask.the_map(), 1+(a-1)/4, 1<<((a-1)%4*2  )));
      mList->SetItem(row++, 1, state(glob_pr.the_map(), glob_mask.the_map(), 1+(a-1)/4, 1<<((a-1)%4*2+1)));
    }
   // record:
    if (record_privs)
    { mList->SetItem(0, 3, wxEmptyString);
      mList->SetItem(1, 3, state(rec_pr.the_map(), rec_mask.the_map(), 0, RIGHT_DEL));
      mList->SetItem(2, 3, state(rec_pr.the_map(), rec_mask.the_map(), 0, RIGHT_GRANT));
      row=3;
      if (td) for (a=atrbase;  a<td->attrcnt; a++)
      { mList->SetItem(row++, 3, state(rec_pr.the_map(), rec_mask.the_map(), 1+(a-1)/4, 1<<((a-1)%4*2  )));
        mList->SetItem(row++, 3, state(rec_pr.the_map(), rec_mask.the_map(), 1+(a-1)/4, 1<<((a-1)%4*2+1)));
      }
    }
  }
 // effective:
  if (sys_priv)
    mList->SetItem(0, 2, state(eff_pr.the_map(), eff_mask.the_map(), 0, RIGHT_INSERT));
  else if (obj_priv)
  { mList->SetItem(0, 2, state(effrec_pr.the_map(), effrec_mask.the_map(), 1+(OBJ_DEF_ATR-1)/4, 1 << ((OBJ_DEF_ATR-1)%4*2)));
    mList->SetItem(1, 2, state(effrec_pr.the_map(), effrec_mask.the_map(), 1+(OBJ_DEF_ATR-1)/4, 1 << ((OBJ_DEF_ATR-1)%4*2+1)));
    mList->SetItem(2, 2, state(effrec_pr.the_map(), effrec_mask.the_map(), 0, RIGHT_DEL));
    mList->SetItem(3, 2, state(effrec_pr.the_map(), effrec_mask.the_map(), 0, RIGHT_GRANT));
  }
  else
  { mList->SetItem(0, 2, state(eff_pr.the_map(), eff_mask.the_map(), 0, RIGHT_INSERT));
    mList->SetItem(1, 2, state(eff_pr.the_map(), eff_mask.the_map(), 0, RIGHT_DEL));
    mList->SetItem(2, 2, state(eff_pr.the_map(), eff_mask.the_map(), 0, RIGHT_GRANT));
    row=3;
    if (td) for (a=atrbase;  a<td->attrcnt; a++)
    { mList->SetItem(row++, 2, state(eff_pr.the_map(), eff_mask.the_map(), 1+(a-1)/4, 1<<((a-1)%4*2  )));
      mList->SetItem(row++, 2, state(eff_pr.the_map(), eff_mask.the_map(), 1+(a-1)/4, 1<<((a-1)%4*2+1)));
    }
   // record:
    if (record_privs)
    { mList->SetItem(0, 4, wxEmptyString);
      mList->SetItem(1, 4, state(effrec_pr.the_map(), effrec_mask.the_map(), 0, RIGHT_DEL));
      mList->SetItem(2, 4, state(effrec_pr.the_map(), effrec_mask.the_map(), 0, RIGHT_GRANT));
      row=3;
      if (td) for (a=atrbase;  a<td->attrcnt; a++)
      { mList->SetItem(row++, 4, state(effrec_pr.the_map(), effrec_mask.the_map(), 1+(a-1)/4, 1<<((a-1)%4*2  )));
        mList->SetItem(row++, 4, state(effrec_pr.the_map(), effrec_mask.the_map(), 1+(a-1)/4, 1<<((a-1)%4*2+1)));
      }
    }
  }
 // basic:
  if (new_privs)
  { mList->SetItem(new_priv_start  , 1, state(&basic_pr, &basic_mask, 0, RIGHT_NEW_READ));
    mList->SetItem(new_priv_start+1, 1, state(&basic_pr, &basic_mask, 0, RIGHT_NEW_WRITE));
    mList->SetItem(new_priv_start+2, 1, state(&basic_pr, &basic_mask, 0, RIGHT_NEW_DEL));
  }
 // disable "roles" button for DB_ADMIN group and for system tables view:
//  else
//    EnableWindow(GetDlgItem(hDlg, CDPR_REL2), (curr_subject_categ!=CATEG_GROUP || curr_subject!=DB_ADMIN_GROUP)
//                                    && *cdp->sel_appl_name);
  mList->Thaw();
  if (td) release_table_d(td);
}

void merge_privils(uns8 * orig, const uns8 * mask, bool extend_common, bool granting, unsigned privil_descr_size)
{ uns8 bas=*orig;
  for (int i=0;  i<privil_descr_size;  i++)
    if (granting) orig[i] |= mask[i];
    else orig[i] &= ~mask[i];
#ifdef STOP  // when multiple tables are selected, column privils are not affected
  if (extend_common)
  { if (*mask & RIGHT_READ)
      if (bas & RIGHT_READ)
        for (i=1;  i<PRIVIL_DESCR_SIZE;  i++) orig[i]|=0x55;
      else
        for (i=1;  i<PRIVIL_DESCR_SIZE;  i++) orig[i]&=0xaa;
    if (*mask & RIGHT_WRITE)
      if (bas & RIGHT_WRITE)
        for (i=1;  i<PRIVIL_DESCR_SIZE;  i++) orig[i]|=0xaa;
      else
        for (i=1;  i<PRIVIL_DESCR_SIZE;  i++) orig[i]&=0x55;
  }
#endif
}

void t_privil_info::update_privils(wxListCtrl * mList, bool granting, bool record_level)
{ t_privils_flex mask(attr_cnt_upp);  uns8 basic_mask;
  const d_table * td = NULL;  int a, row;
  if (!sys_priv && !obj_priv && multioper!=MULTITAB)
    td = cd_get_table_d(cdp, tbs[0], CATEG_TABLE);
 // get map of selected privils:
  mask.clear();  basic_mask=0;
  long sel = mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (sys_priv)
  { if (sel==0) mask.the_map()[0] |= RIGHT_INSERT;
  }
  else if (obj_priv)
  { if (sel==0) 
    { mask.the_map()[1+(OBJ_DEF_ATR-1)/4]    |= (1 << ((OBJ_DEF_ATR   -1)%4*2));
      mask.the_map()[1+(OBJ_FLAGS_ATR-1)/4]  |= (1 << ((OBJ_FLAGS_ATR -1)%4*2));
      mask.the_map()[1+(OBJ_FOLDER_ATR-1)/4] |= (1 << ((OBJ_FOLDER_ATR-1)%4*2));
      mask.the_map()[1+(OBJ_MODIF_ATR-1)/4]  |= (1 << ((OBJ_MODIF_ATR -1)%4*2));    
      sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); 
    }
    if (sel==1) 
    { mask.the_map()[1+(OBJ_DEF_ATR   -1)/4] |= (1 << ((OBJ_DEF_ATR   -1)%4*2+1));
      mask.the_map()[1+(OBJ_FLAGS_ATR -1)/4] |= (1 << ((OBJ_FLAGS_ATR -1)%4*2+1));
      mask.the_map()[1+(OBJ_FOLDER_ATR-1)/4] |= (1 << ((OBJ_FOLDER_ATR-1)%4*2+1));
      mask.the_map()[1+(OBJ_MODIF_ATR -1)/4] |= (1 << ((OBJ_MODIF_ATR -1)%4*2+1));  
      sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); 
    }
    if (sel==2) { mask.the_map()[0] |= RIGHT_DEL;    sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
    if (sel==3) { mask.the_map()[0] |= RIGHT_GRANT;  sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
  }
  else
  { if (sel==0) { mask.the_map()[0] |= RIGHT_INSERT; sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
    if (sel==1) { mask.the_map()[0] |= RIGHT_DEL;    sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
    if (sel==2) { mask.the_map()[0] |= RIGHT_GRANT;  sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
   // columns:
    row=3;
    if (td) for (a=atrbase;  a<td->attrcnt; a++)
    { if (sel==row++) { mask.the_map()[1+(a-1)/4] |= 1 << ((a-1)%4*2)  ;  sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
      if (sel==row++) { mask.the_map()[1+(a-1)/4] |= 1 << ((a-1)%4*2+1);  sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
    }
   // basic:
    if (sel==row++) { basic_mask |= RIGHT_NEW_READ;   sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
    if (sel==row++) { basic_mask |= RIGHT_NEW_WRITE;  sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
    if (sel==row++) { basic_mask |= RIGHT_NEW_DEL;    sel = mList->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }
  }
  if (td) release_table_d(td);
 // save:
  t_privils_flex priv_buf(attr_cnt_upp);  int tabind, recind;  bool first=true;
  for (tabind=recind=0;  first ||
          multioper==MULTITAB && tbs[tabind]!=NOOBJECT ||
          multioper==MULTIREC && recnums[recind]!=NORECNUM;
       first=false, (multioper==MULTITAB ? tabind++ : recind++))
  { trecnum therec=recnums==NULL ? NORECNUM : recnums[recind];
   // system (insert):
    if (sys_priv)
    { if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], NORECNUM, OPER_GET, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
      if (granting) *priv_buf.the_map() |= *mask.the_map();
      else *priv_buf.the_map() &= ~*mask.the_map();
      if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], NORECNUM, OPER_SET, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
    }
    else if (obj_priv)
    { if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], therec, OPER_GET, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
      merge_privils(priv_buf.the_map(), mask.the_map(), common_rw, granting, privil_descr_size);
      if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], therec, OPER_SET, priv_buf.the_map()))
        { cd_Signalize(cdp);  break; }
    }
    else // data privils
    { if (!record_level)  // table global:
      { if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], NORECNUM, OPER_GET, priv_buf.the_map()))
          { cd_Signalize(cdp);  break; }
        merge_privils(priv_buf.the_map(), mask.the_map(), common_rw, granting, privil_descr_size);
        if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], NORECNUM, OPER_SET, priv_buf.the_map()))
          { cd_Signalize(cdp);  break; }
      }
      else if (record_privs) // record:
      { if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], therec, OPER_GET, priv_buf.the_map()))
          { cd_Signalize(cdp);  break; }
        merge_privils(priv_buf.the_map(), mask.the_map(), common_rw, granting, privil_descr_size);
        if (cd_GetSet_privils(cdp, curr_subject, curr_subject_categ, tbs[tabind], therec, OPER_SET, priv_buf.the_map()))
          { cd_Signalize(cdp);  break; }
      }
     // basic:
      if (new_privs && (!recind || multioper!=MULTIREC))
      { if (cd_GetSet_privils(cdp, EVERYBODY_GROUP, CATEG_GROUP,     tbs[tabind], NORECNUM, OPER_GET, priv_buf.the_map()))
          { cd_Signalize(cdp);  break; }
        if (granting) *priv_buf.the_map() |= basic_mask;
        else *priv_buf.the_map() &= ~basic_mask;
        if (cd_GetSet_privils(cdp, EVERYBODY_GROUP, CATEG_GROUP,     tbs[tabind], NORECNUM, OPER_SET, priv_buf.the_map()))
          { cd_Signalize(cdp);  break; }
      }
    }
  }
}

#include "xrc/ObjectPrivilsDlg.cpp"

bool Edit_privileges(cdp_t cdp, wxWindow * hParent, ttablenum * tbs, trecnum * recnums, int multioper)
{ ttablenum * ptbs;  ttablenum tbnum;
 // analyse params:
  if (!hParent) hParent=wxGetApp().frame;
  if (tbs==NULL || *tbs==NOOBJECT) return false;
  if (SYSTEM_TABLE(tbs[0]) && tbs[0]!=TAB_TABLENUM && tbs[0]!=OBJ_TABLENUM && tbs[0]!=USER_TABLENUM)
    if (recnums==NULL) return false;  // not allowed
  if (multioper && multioper!=MULTITAB && multioper!=MULTIREC) return false;
 // convert cursor record to a table record:
  if (IS_CURSOR_NUM(*tbs))
  { uns16 num=1;  
    if (cd_Get_base_tables(cdp, *tbs, &num, &tbnum)) return false;
    if (tbnum < 0) return FALSE;
    if (!multioper && recnums!=NULL)
      if (*recnums!=NORECNUM)
        if (cd_Translate(cdp, *tbs, *recnums, 0, recnums)) return false;
    ptbs=&tbnum;
  }
  else ptbs=tbs;
 // create class and open dialog:
  t_privil_info pi = t_privil_info(cdp, ptbs, recnums, multioper);
  if (pi.sys_priv)  
    if (!cd_Am_I_db_admin(cdp) && !cd_Am_I_config_admin(cdp)) 
      { client_error(cdp, NO_RIGHT);  cd_Signalize(cdp);  return false; }
  ObjectPrivilsDlg dlg(pi);
  pi.hDlg=&dlg;
  dlg.Create(hParent);  
  dlg.ShowModal();
  return true;
}

///////////////////////////////// listing triggers for a table ////////////////////////////////////
const wxChar * trigger_type_name(int trg_type)
{ switch (trg_type & TRG_TYPE_MASK)
  { case TRG_BEF_INS_ROW: return wxT("Before INSERT (per ROW)");   
    case TRG_AFT_INS_ROW: return wxT("After INSERT (per ROW)");
    case TRG_BEF_UPD_ROW: return wxT("Before UPDATE (per ROW)");
    case TRG_AFT_UPD_ROW: return wxT("After UPDATE (per ROW)");
    case TRG_BEF_DEL_ROW: return wxT("Before DELETE (per ROW)");
    case TRG_AFT_DEL_ROW: return wxT("After DELETE (per ROW)");
    case TRG_BEF_INS_STM: return wxT("Before INSERT (per statement)");
    case TRG_AFT_INS_STM: return wxT("After INSERT (per statement)");
    case TRG_BEF_UPD_STM: return wxT("Before UPDATE (per statement)");
    case TRG_AFT_UPD_STM: return wxT("After UPDATE (per statement)");
    case TRG_BEF_DEL_STM: return wxT("Before DELETE (per statement)");
    case TRG_AFT_DEL_STM: return wxT("After DELETE (per statement)");
  }
  return wxEmptyString;
}

void List_triggers(cdp_t cdp, MyFrame * frame, const char * objname, tobjnum objnum)
{ t_trig_info * trgs;  wxString msg;  tobjname objname2;
  wxNotebook * output = frame->show_output_page(SEARCH_PAGE_NUM);
  if (!output) return;  // impossible
  SearchResultList * srd = (SearchResultList*)output->GetPage(SEARCH_PAGE_NUM);
  if (cd_List_triggers(cdp, objnum, &trgs)) cd_Signalize(cdp);
  else
  { srd->ClearAll();
    strcpy(srd->server_name, cdp->locid_server_name);
    strcpy(objname2, objname);  wb_small_string(cdp, objname2, true);
    msg.Printf(_("Triggers on table %s:"), wxString(objname2, *cdp->sys_conv).c_str());
    srd->Append(msg, (void*)NULL);
    for (int i=0;  trgs[i].trigger_type;  i++)
    { strcpy(objname2, trgs[i].name);  wb_small_string(cdp, objname2, true);
      msg.Printf(wxT("%s:  %s"), wxString(objname2, *cdp->sys_conv).c_str(), trigger_type_name(trgs[i].trigger_type));
      tobjnum trig_objnum;  search_result * data = NULL;
      if (!cd_Find2_object(cdp, trgs[i].name, NULL, CATEG_TRIGGER, &trig_objnum))
        data = new search_result(CATEG_TRIGGER, trig_objnum, 0);
      srd->Append(msg, data);
    }
    corefree(trgs);
  }
}

//////////////////////////// listing users in a role (transitive) ////////////////////////////////////
void List_users(cdp_t cdp, MyFrame * frame, const char * objname, tobjnum objnum)
{ wxString msg;  tobjname objname2;
  wxNotebook * output = frame->show_output_page(SEARCH_PAGE_NUM);
  if (!output) return;  // impossible
  SearchResultList * srd = (SearchResultList*)output->GetPage(SEARCH_PAGE_NUM);
  srd->ClearAll();
  strcpy(objname2, objname);  wb_small_string(cdp, objname2, true);
  msg.Printf(_("Users contained in the role %s:"), wxString(objname2, *cdp->sys_conv).c_str());
  srd->Append(msg, (void*)NULL);
  trecnum user_count;
  cd_Rec_cnt(cdp, USER_TABLENUM, &user_count);
  for (trecnum urec=0;  urec<user_count;  urec++)
  { uns32 relation;  tcateg subj_categ;  uns8 del;
    if (cd_Read(cdp, USER_TABLENUM, urec, DEL_ATTR_NUM, NULL, &del) || del!=NOT_DELETED) 
      continue;
    if (cd_Read(cdp, USER_TABLENUM, urec, USER_ATR_CATEG, NULL, &subj_categ) || subj_categ!=CATEG_USER) 
      continue;
    if (cd_GetSet_group_role(cdp, urec, objnum, CATEG_ROLE, OPER_GETEFF, &relation) || !relation)
      continue;
    cd_Read(cdp, USER_TABLENUM, urec, USER_ATR_LOGNAME, NULL, &objname2);
    wb_small_string(cdp, objname2, true);
    srd->Append(wxString(objname2, *cdp->sys_conv).c_str(), (void*)NULL);
  }
}

