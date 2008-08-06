/////////////////////////////////////////////////////////////////////////////
// Name:        ClientsThreadsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/17/04 14:10:52
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "ClientsThreadsDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "ClientsThreadsDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ClientsThreadsDlg type definition
 */

IMPLEMENT_CLASS( ClientsThreadsDlg, wxPanel )

/*!
 * ClientsThreadsDlg event table definition
 */

BEGIN_EVENT_TABLE( ClientsThreadsDlg, wxPanel )

////@begin ClientsThreadsDlg event table entries
////@end ClientsThreadsDlg event table entries

END_EVENT_TABLE()

/*!
 * ClientsThreadsDlg constructors
 */

ClientsThreadsDlg::ClientsThreadsDlg( )
{ }

/*!
 * ClientsThreadsDlg creator
 */

bool ClientsThreadsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ClientsThreadsDlg member initialisation
////@end ClientsThreadsDlg member initialisation

////@begin ClientsThreadsDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ClientsThreadsDlg creation
    return TRUE;
}

/*!
 * Control creation for ClientsThreadsDlg
 */

void ClientsThreadsDlg::CreateControls()
{    
////@begin ClientsThreadsDlg content construction

    ClientsThreadsDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxTextCtrl* item3 = new wxTextCtrl;
    item3->Create( item1, CD_THR_PLACEHOLDER, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    item2->Add(item3, 1, wxGROW|wxALL, 5);

////@end ClientsThreadsDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool ClientsThreadsDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_THR_REFRESH
 */

static const char user_query[] =  
 "SELECT login_name, client_number, sel_schema_name, case when transaction_open then 'Open' else 'Closed' end AS t_o, case when isolation_level<=1 then 'Read_comitted' else 'Serializable' end As i_l, state, detached, worker_thread, own_connection, sql_options, lock_waiting_timeout, case connection when 0 then 'Direct' when 1 then 'TCP/IP' when 2 then 'IPX/SPX' when 3 then 'NetBEUI' when 4 then 'HTTP' else '' end AS conn, net_address, case when COMM_ENCRYPTION>0 then 'Encrypted' else 'Plain' end AS enc,session_number,profiled_thread,thread_name,req_proc_time,proc_sql_stmt FROM _iv_logged_users";

static const char user_form[] =  
"x CURSORVIEW 0 72 846 299 "
"STYLE 622657 HDFTSTYLE 3276800 _STYLE3 0 FONT 8 238 34 'MS Sans Serif' HELP 4101 "
"BEGIN"
" LTEXT 'Name' 3 0 0 97 20 0 0 PANE PAGEHEADER"
" LTEXT 4 0 0 97 20 65536 0 VALUE _ login_name _"
" LTEXT 'Number' 5 97 0 33 20 0 0 PANE PAGEHEADER"
" RTEXT 6 97 0 33 20 65538 0 VALUE _ client_number _ COLOUR _ detached ? 255 : worker_thread ? 256*256*255 : 0 _"
" LTEXT 'Schema' 7 130 0 92 20 0 0 PANE PAGEHEADER"
" LTEXT 8 130 0 92 20 65536 0 VALUE _ sel_schema_name _"
" LTEXT 'Thread name' 35 140 0 92 20 0 0 PANE PAGEHEADER"
" LTEXT 36 140 0 92 20 65536 0 VALUE _ thread_name _"
" LTEXT 'Transaction' 21 222 0 55 20 0 0 PANE PAGEHEADER"
" LTEXT 22 222 0 55 20 65536 0 VALUE _ t_o _"
" LTEXT 'Isolation' 23 277 0 82 20 0 0 PANE PAGEHEADER"
" LTEXT 24 277 0 82 20 65536 0 VALUE _ i_l _"
" LTEXT 'State' 9 359 0 30 20 0 0 PANE PAGEHEADER"
" RTEXT 10 359 0 30 20 65538 0 VALUE _ state _"
" LTEXT 'Detached' 15 389 0 62 20 0 0 PANE PAGEHEADER"
" CTEXT 16 389 0 62 20 65537 0 VALUE _ detached _"
" LTEXT 'Working' 17 451 0 50 20 0 0 PANE PAGEHEADER"
" CTEXT 18 451 0 50 20 65537 0 VALUE _ worker_thread _"
" LTEXT 'Me?' 19 501 0 26 20 0 0 PANE PAGEHEADER"
" CTEXT 20 501 0 26 20 65537 0 VALUE _ own_connection _"
" LTEXT 'Sql options' 25 527 0 63 20 0 0 PANE PAGEHEADER"
" RTEXT 26 527 0 63 20 65538 0 VALUE _ sql_options _"
" LTEXT 'Waiting' 27 590 0 43 20 0 0 PANE PAGEHEADER"
" RTEXT 28 590 0 43 20 65538 0 VALUE _ lock_waiting_timeout _"
" LTEXT 'Connection' 11 633 0 63 20 0 0 PANE PAGEHEADER"
" LTEXT 12 633 0 63 20 65536 0 VALUE _ conn _"
" LTEXT 'Network address' 13 696 0 121 20 0 0 PANE PAGEHEADER"
" LTEXT 14 696 0 121 20 65536 0 VALUE _ net_address _"
" LTEXT 'Communication' 29 817 0 78 20 0 0 PANE PAGEHEADER"
" LTEXT 30 817 0 78 20 65536 0 VALUE _ enc _"
" LTEXT 'Session' 31 885 0 63 20 0 0 PANE PAGEHEADER"
" RTEXT 32 885 0 63 20 65538 0 VALUE _ session_number _"
" LTEXT 'Profiling' 33 948 0 50 20 0 0 PANE PAGEHEADER"
" CHECKBOX 34 948 0 50 20 65537 0 VALUE _ profiled_thread _"                                                     
" LTEXT 'Processing' 37 998 0 43 20 0 0 PANE PAGEHEADER"
" RTEXT 38 998 0 43 20 65538 0 VALUE _ req_proc_time _"
" LTEXT 'SQL statement' 39 1041 0 400 20 0 0 PANE PAGEHEADER"
" LTEXT 40 1041 0 400 20 65536 0 VALUE _ proc_sql_stmt _"
" END";

#ifdef TRANSLATION_SUPPORT
_("Name")
_("Number")
_("Schema")
_("Transaction")
_("Isolation")
_("State")
_("Detached")
_("Working")
_("Me?")
_("SQL options")
_("Waiting")
_("Connection")
_("Network address")
_("Communication")
_("Session")
_("Profiling")
_("Processing")
_("SQL statement")
#endif

static const char locks_query[] =
#if 0
 "SELECT schema_name, table_name, record_number, lock_type, case lock_type when 128 then '�en� when 64 then 'Pepis' when 32 then 'Pepis temp' when 16 then '�en�temp' else 'Kombinace' END AS lock_tp, owner_usernum, owner_name, object_name FROM _iv_locks";
#else
 "SELECT schema_name, table_name, record_number, lock_type, case lock_type when 128 then 'Read' when 64 then 'Write' when 32 then 'Write temp' when 16 then 'Read temp' else 'Combined' END AS lock_tp, owner_usernum, owner_name, object_name FROM _iv_locks";
#endif

static const char locks_form[] =  
"x CURSORVIEW 0 62 622 300 STYLE 622657 HDFTSTYLE 3276800 _STYLE3 0 FONT 8 238 34 'MS Sans Serif' HELP 4010 "
"BEGIN "
" LTEXT 'Schema' 3 0 0 89 20 0 0 PANE PAGEHEADER "
" LTEXT 4 0 0 89 20 65536 0 VALUE _ schema_name _ "
" LTEXT 'Table' 5 89 0 98 20 0 0 PANE PAGEHEADER "
" LTEXT 6 89 0 98 20 65536 0 VALUE _ table_name _ "
" LTEXT 'Record/Page' 7 187 0 84 20 0 0 PANE PAGEHEADER "
" LTEXT 8 187 0 84 20 65536 0 VALUE _ record_number _ "
" LTEXT 'Lock type' 9 271 0 69 20 0 0 PANE PAGEHEADER "
" LTEXT 10 271 0 69 20 65536 0 VALUE _ lock_tp _ "
" LTEXT 'Client #' 11 340 0 65 20 0 0 PANE PAGEHEADER "
" LTEXT 12 340 0 65 20 65536 0 VALUE _ owner_usernum _ "
" LTEXT 'User name' 13 405 0 82 20 0 0 PANE PAGEHEADER "
" LTEXT 14 405 0 82 20 65536 0 VALUE _ owner_name _ "
" LTEXT 'Object name' 15 487 0 82 20 0 0 PANE PAGEHEADER "
" LTEXT 16 487 0 82 20 65536 0 VALUE _ object_name _ "
"END";

#ifdef TRANSLATION_SUPPORT
_("Schema")
_("Table")
_("Record/Page")
_("Lock type")
_("Client #")
_("User name")
_("Object name")
#endif

static const char trace_form[] =  
"x CURSORVIEW 0 52 620 299 STYLE 98377 HDFTSTYLE 139264 _STYLE3 4 FONT 8 238 34 'MS Sans Serif' HELP 4000 "
"BEGIN "
" LTEXT 'Log name' 3 0 0 79 20 0 0 PANE PAGEHEADER "
" LTEXT 4 0 0 79 20 65536 0 VALUE _ log_name _ "
" LTEXT 'Situation' 5 79 0 120 20 0 0 PANE PAGEHEADER "
" COMBOBOX 6 79 0 120 68 10551363 0 ACCESS _ situation _ ENUMERATE 'User error',1 'Server failure',2 'Network communication',4 "
    "'Executing SQL statement',64 'Login and Logout',128 'Cursor open and close',256 'Implicit RollBack',512 'Calling Log_write',1024 'Server start and stop',16384 "
    "'Server console communication',65536 "
    "'Reading a value',131072 'Writing a value',262144 'Inserting a record',524288 'Deleting a record',1048576 'Background object error',2097152 'SQL procedure call',4194304 "
    "'Executing a trigger',8388608 'User warning',16777216 'Lock error',67108864 'Web request',134217728 'Convertor error',268435456"
" LTEXT 'Extent' 11 314 0 120 20 0 0 PANE PAGEHEADER "
" COMBOBOX 12 314 0 120 64 8454147 0 ACCESS _ context _ ENUMERATE 'No context',1'Single context level',2'Full context',3 "
" LTEXT 'User name' 7 199 0 115 20 0 0 PANE PAGEHEADER "
" LTEXT 8 199 0 115 20 65536 0 VALUE _ username _ "
" LTEXT 'Table name' 9 434 0 141 20 0 0 PANE PAGEHEADER "
" EDITTEXT 10 434 0 141 20 65536 0 VALUE _ tabname _ "
"END";

#ifdef TRANSLATION_SUPPORT
_("Log name")
_("Event")
_("User error")
_("Server failure")
_("Network communication")
_("Executing SQL statement")
_("Login and Logout")
_("Cursor open and close")
_("Implicit RollBack")
_("Calling Log_write")
_("Server start and stop")
_("Server console communication")
_("Reading a value")
_("Writing a value")
_("Inserting a record")
_("Deleting a record")
_("Background object error")
_("SQL procedure call")
_("Executing a trigger")
_("User warning")
_("Lock error")
_("Web request")
_("Convertor error")

_("Situation")
_("Extent")
_("User name")
_("Table name")

_("No context")
_("Single context level")
_("Full context")
#endif

//'Replication',8 'Replication conflict',32768 

static const char trace_query[] =
 "SELECT log_name, situation, case usernum when -1 then \'\' else "
           "(select logname from usertab where @=usernum)end as username,"
           "case tabnum when -1 then \'\' else"
           "(select obj_name from objtab where category=chr(7) and apl_uuid=(select apl_uuid from tabtab where @=tabnum))"
           "+'.'+(select tab_name from tabtab where @=tabnum)end as tabname,"
           "context FROM _iv_pending_log_reqs";

void ClientsThreadsDlg::open_subwindow(wxWindow * wnd)
{ wxWindow * oldgrid = FindWindow(CD_THR_PLACEHOLDER);
  if (oldgrid) oldgrid->Destroy();
  if (wnd)
  { wnd->SetId(CD_THR_PLACEHOLDER);
    GetSizer()->Prepend(wnd, 1, wxGROW|wxALL, 0);
    Layout();
  }  
}

bool ClientsThreadsDlg::open_monitor_page(cdp_t cdp, DataGrid * grid, const char * query, const char * form)
// Deletes [grid] on error (e.g. server disconnected) and returns false.
{ tcursnum curs;
  if (cd_Open_cursor_direct(cdp, query, &curs))
    { cd_Signalize(cdp);  /*delete grid;*/  return false; }  // grid must be lost - WX crashes in the destructor if the constructor without Create has been used
  cd_Make_persistent(cdp, curs);
  grid->translate_captions=true;
  if (!grid->open_form(this, cdp, form, curs, AUTO_CURSOR | CHILD_VIEW))  // deletes the grid on error, closes the cursor
    return false;
  //cache_resize(grid->inst, 1);  // because the window is not visible now, must init the cache before Layout() is called
  open_subwindow(grid); 
  return true;
}

