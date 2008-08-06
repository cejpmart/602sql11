/////////////////////////////////////////////////////////////////////////////
// Name:        SQLConsoleDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/23/04 13:37:35
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "SQLConsoleDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "SQLConsoleDlg.h"

////@begin XPM images
////@end XPM images

#ifdef LINUX
#if WBVERS<1000
#define SQLErrorA SQLError  
#endif
#endif

class CtrlEnterHandler : public wxEvtHandler
{ SQLConsoleDlg * console;
 public:
  CtrlEnterHandler(SQLConsoleDlg * consoleIn)
    { console=consoleIn; }
  void OnKeyDown(wxKeyEvent & event);
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( CtrlEnterHandler, wxEvtHandler )
  EVT_KEY_DOWN(CtrlEnterHandler::OnKeyDown)
END_EVENT_TABLE()

void CtrlEnterHandler::OnKeyDown(wxKeyEvent & event)
{
  if (event.GetKeyCode()==WXK_F1)
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/sqlconsole.html"));
  else if (event.GetKeyCode()!=WXK_RETURN)
    event.Skip();
  else if (event.ControlDown())
  { int pos = console->mCmd->GetInsertionPoint();
    console->mCmd->Replace(pos, pos, wxT("\r\n"));
  }
  else
  { wxCommandEvent event;
    console->OnCdSqlconsExecClick(event);
  }
}

/*!
 * SQLConsoleDlg type definition
 */

IMPLEMENT_CLASS( SQLConsoleDlg, wxPanel )

/*!
 * SQLConsoleDlg event table definition
 */

BEGIN_EVENT_TABLE( SQLConsoleDlg, wxPanel )

////@begin SQLConsoleDlg event table entries
    EVT_TEXT_ENTER( CD_SQLCONS_STMT, SQLConsoleDlg::OnCdSqlconsStmtEnter )

    EVT_BUTTON( CD_SQLCONS_EXEC, SQLConsoleDlg::OnCdSqlconsExecClick )

    EVT_BUTTON( CD_SQLCONS_PREV, SQLConsoleDlg::OnCdSqlconsPrevClick )

    EVT_BUTTON( CD_SQLCONS_NEXT, SQLConsoleDlg::OnCdSqlconsNextClick )

////@end SQLConsoleDlg event table entries

END_EVENT_TABLE()

/*!
 * SQLConsoleDlg constructors
 */

SQLConsoleDlg::SQLConsoleDlg( )
{ stmt_list_pos=-1;
}

SQLConsoleDlg::~SQLConsoleDlg(void)
{
  if (mCmd) mCmd->PopEventHandler(true);
}

/*!
 * SQLConsoleDlg creator
 */

bool SQLConsoleDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SQLConsoleDlg member initialisation
    mSchema = NULL;
    mCmd = NULL;
    mDebug = NULL;
    mExec = NULL;
    mPrev = NULL;
    mNext = NULL;
////@end SQLConsoleDlg member initialisation

////@begin SQLConsoleDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SQLConsoleDlg creation
    return TRUE;
}

#include "bmps/previous.xpm"


/*!
 * Control creation for SQLConsoleDlg
 */

void SQLConsoleDlg::CreateControls()
{    
////@begin SQLConsoleDlg content construction
    SQLConsoleDlg* itemPanel1 = this;

    this->SetBackgroundColour(wxColour(192, 192, 192));
    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxTOP|wxBOTTOM, 2);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemPanel1, wxID_STATIC, _("SQL statement:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_BOTTOM|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 2);

    mSchema = new wxStaticText;
    mSchema->Create( itemPanel1, CD_SQLCONS_SCHEMA, _T(""), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
    itemBoxSizer4->Add(mSchema, 1, wxALIGN_BOTTOM|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 2);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer7, 1, wxGROW|wxALL, 0);

    mCmd = new wxTextCtrl;
    mCmd->Create( itemPanel1, CD_SQLCONS_STMT, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxHSCROLL );
    if (ShowToolTips())
        mCmd->SetToolTip(_("Type an SQL statement here and click Execute or hit Enter. Use Ctrl+Enter to insert a line break."));
    itemBoxSizer7->Add(mCmd, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 2);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer7->Add(itemBoxSizer9, 0, wxGROW|wxALL, 0);

    mDebug = new wxCheckBox;
    mDebug->Create( itemPanel1, CD_SQLCONS_DEBUG, _("Debug mode"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mDebug->SetValue(FALSE);
    if (ShowToolTips())
        mDebug->SetToolTip(_("If checked, procedures and triggers called during execution of the SQL statement can be debugged."));
    itemBoxSizer9->Add(mDebug, 0, wxALIGN_RIGHT|wxTOP|wxBOTTOM, 3);

    mExec = new wxButton;
    mExec->Create( itemPanel1, CD_SQLCONS_EXEC, _("Execute"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        mExec->SetToolTip(_("Click this to execute the SQL statement"));
    itemBoxSizer9->Add(mExec, 0, wxALIGN_RIGHT|wxRIGHT, 5);

    itemBoxSizer9->Add(1, 1, 1, wxALIGN_RIGHT|wxALL, 0);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer9->Add(itemBoxSizer13, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxBitmap mPrevBitmap(wxNullBitmap);
    mPrev = new wxBitmapButton;
    mPrev->Create( itemPanel1, CD_SQLCONS_PREV, mPrevBitmap, wxDefaultPosition, wxSize(18, 18), wxBU_AUTODRAW|wxBU_EXACTFIT );
    if (ShowToolTips())
        mPrev->SetToolTip(_("Show previous statement"));
    itemBoxSizer13->Add(mPrev, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxBitmap mNextBitmap(wxNullBitmap);
    mNext = new wxBitmapButton;
    mNext->Create( itemPanel1, CD_SQLCONS_NEXT, mNextBitmap, wxDefaultPosition, wxSize(18, 18), wxBU_AUTODRAW|wxBU_EXACTFIT );
    if (ShowToolTips())
        mNext->SetToolTip(_("Show next statement"));
    itemBoxSizer13->Add(mNext, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3);

////@end SQLConsoleDlg content construction
  SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));  // since wx 2.6.0 the panel inherits background colour of the main windows, which is wrong
 // set the bitmaps:
  wxBitmap bmp1(previous_xpm);
  mPrev->SetBitmapLabel(bmp1);
  wxBitmap bmp2(openselector_xpm);
  mNext->SetBitmapLabel(bmp2);
 // add ctrl-enter handler:
  CtrlEnterHandler * handler;
  handler = new CtrlEnterHandler(this);
  mCmd->PushEventHandler(handler);
  SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // since 2.8 necessary to obtain the Ctrl-Return
}

/*!
 * Should we show tooltips?
 */
bool SQLConsoleDlg::ShowToolTips()
{
    return TRUE;
}
/////////////////////////////////////////////// SQL console /////////////////////////////////////////
void SQLConsoleDlg::add_line(wxString buf)
// Writes [buf] to the "SQL output" window.
{ wxNotebook * output = wxGetApp().frame->show_output_page(SQL_OUTPUT_PAGE_NUM);
  if (!output) return;  // impossible
#ifdef LIST_BOX_SQL_OUTPUT
  wxListBox * SQLres = (wxListBox*)output->GetPage(SQL_OUTPUT_PAGE_NUM);
  if (SQLres->GetCount() > MAX_RESULT_LINES) SQLres->Delete(0);  // if there are more lines than MAX_RESULT_LINES in the list box, delete the oldest
  SQLres->Append(buf);
  SQLres->SetSelection(SQLres->GetCount()-1);  // selects the new item in order to make it visible (does not work on GTK)
#else
  wxListCtrl * SQLres = (wxListCtrl*)output->GetPage(SQL_OUTPUT_PAGE_NUM);
  if (SQLres->GetItemCount() > MAX_RESULT_LINES) SQLres->DeleteItem(0);  // if there are more lines than MAX_RESULT_LINES in the list box, delete the oldest
  long item = SQLres->InsertItem(SQLres->GetItemCount(), buf);
  SQLres->EnsureVisible(item);
#endif
 // If the debugging session ends now, the resizing of the output window will follow
 // and the last line may not be displayed on some systems. Calling Yield helps.
#if wxMAJOR_VERSION==2 && wxMINOR_VERSION<8  // recurses since 2.8
  wxGetApp().Yield();  // crashes some themes with qtengine
#endif
}

void SQLConsoleDlg::add_error(cdp_t acdp)
{ wxString msg;
  msg =  _(" >>Error: ") + Get_error_num_textWX(acdp, cd_Sz_error(acdp));
  add_unbroken(msg);
}

void SQLConsoleDlg::add_unbroken(wxString msg)
{
  for (int i = 0;  i<msg.Length();  i++)
    if ((unsigned)msg[(unsigned)i]<' ') msg.GetWritableChar(i)=' ';   // removing /r/n's
  add_line(msg);
}

bool SQLConsoleDlg::process_stmt(wxString & stmt, bool debugged)
// Executes the SQL statement and writes the result to the SQL output window or opens a form.
{ uns32 results[MAX_SQL_STATEMENTS];
 // copy the statement to the SQL output:
  add_unbroken(stmt);
 // execute with debugger:
  if (debugged)  // false on ODBC
  { cdp_t cdp2 = clone_connection(cdp);
    if (!cdp2) return false;
   // start debugging:
    the_server_debugger = new server_debugger(cdp);
    if (the_server_debugger)
    { the_server_debugger->dlg2=this;
      if (the_server_debugger->start_debugging(cdp2, &stmt))
      { add_line(_("Debug execution started..."));
        return true;  // cdp2 is now owned by the_server_debugger
      }
    }
    wx_disconnect(cdp2);
    delete cdp2;
    return false;
  }
 // execute without debugger:
  memset(results, 0xff, sizeof(uns32)*MAX_SQL_STATEMENTS);
  results[0]=0;
  wxCharBuffer bytes = stmt.mb_str(GetConv(cdp));
  const char * stmt8 = bytes;
  if (!stmt.IsEmpty() && (!stmt8 || !*stmt8))  // conversion error
    add_line(_("The text is not convertible to the SQL Server charset."));  
  else if (cdp)
  { BOOL res;
    { wxBusyCursor busy;
      res = cd_SQL_execute(cdp, stmt8, results);
    }
    if (res)
      add_error(cdp);
    else
    { inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  // statement may by ALTER TABLE etc.
      bool any_specific=false;
      for (int i=0;  i<MAX_SQL_STATEMENTS;  i++)
        if (results[i]!=0xffffffff)
          if (IS_CURSOR_NUM(results[i]))
          { add_line(_(" >Cursor opened, the contents will be shown in a separate window."));
           // open the form:
            tcursnum curs = results[i];
            if (!open_data_grid(NULL, cdp, CATEG_DIRCUR, curs, "", false, NULL))
              cd_Close_cursor(cdp, curs);
            any_specific=true;
          }
          else // count-returning statements
          { wxString buf;
            buf.Printf(_(" >Statement has been executed, number of processed records or object number is: %u"), results[i]);
            add_line(buf);
            any_specific=true;
          }
      if (!any_specific)
        add_line(_(" >OK"));
      check_open_transaction(cdp, wxGetApp().frame);  
    }
  }
  else // ODBC
  {// allocate a new ODBC statement (needed for the SELECT):
    HSTMT hStmt;  SQLRETURN result;  
    result = create_odbc_statement(xcdp->get_odbcconn(), &hStmt);
    if (result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO) 
    { bool deallocate_stmt = true;
      { wxBusyCursor busy;
        result=SQLExecDirectA(hStmt, (uns8*)stmt8, SQL_NTS);
      }
      if (result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO || result==SQL_NO_DATA)
      { //if (result==SQL_NO_DATA) results[0]=0;
        //else 
        { SQLSMALLINT column_count;  SQLLEN RowCount;
          SQLNumResultCols(hStmt, &column_count);
          if (column_count==0)
          { SQLRETURN result2 = SQLRowCount(hStmt, &RowCount);
            if (result2==SQL_SUCCESS && RowCount>=0)  // count-returning statements
            { wxString buf;
              buf.Printf(_(" >Statement has been executed, number of processed records: %u"), RowCount);
              add_line(buf);
            }
            else  // result2==SQL_SUCCESS && RowCount==-1 in CREATE TABLE statement on Access driver
              add_line(_(" >OK"));
          }
          else   // result set created
          { wxWindow * frame = open_data_grid(NULL, xcdp, CATEG_DIRCUR, (tcursnum)(size_t)hStmt, "", false, NULL);
            if (frame)
            { DataGrid * grid = (DataGrid *)frame->GetChildren().Item(0)->GetData();
              deallocate_stmt = false;  
             // save the source statement with the grid:
              grid->inst->dt->select_st = (char*)corealloc(strlen(stmt8)+1, 24);
              if (grid->inst->dt->select_st)
                strcpy(grid->inst->dt->select_st, stmt8);
              add_line(_(" >Cursor opened, the contents will be shown in a separate window."));
            }
            else
              SQLCloseCursor(hStmt);
          }
        }
      }
      if (result==SQL_ERROR || result==SQL_SUCCESS_WITH_INFO)
      { SDWORD native;  RETCODE retcode;
#ifndef LINUX // (WBVERS>=1000)
        wchar_t text[SQL_MAX_MESSAGE_LENGTH+1], state[SQL_SQLSTATE_SIZE+1];
        do
        { retcode=SQLError(SQL_NULL_HENV, SQL_NULL_HDBC, hStmt, state, &native, text, sizeof(text), NULL);
          if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) break;
          add_unbroken(text);
        } while (true);
#else
        char text[SQL_MAX_MESSAGE_LENGTH+1], state[SQL_SQLSTATE_SIZE+1];
        do
        { retcode=SQLErrorA(SQL_NULL_HENV, SQL_NULL_HDBC, hStmt, (SQLCHAR*)state, &native, (SQLCHAR*)text, sizeof(text), NULL);
          if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) break;
          add_unbroken(wxString(text, *wxConvCurrent));
        } while (true);
#endif  
      }
      if (deallocate_stmt)
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    }
  }
  return true;
}

void SQLConsoleDlg::display_results(uns32 * results, cdp_t cdp2)
// Called by the debugger only, never for ODBC.
{ if (cd_Sz_error(cdp2))
    add_error(cdp2);
  else
    for (int i=0;  i<MAX_SQL_STATEMENTS;  i++)
      if (results[i]!=0xffffffff)
        if (IS_CURSOR_NUM(results[i]))  
          add_line(_(" >Cursor opened (contents not shown)"));
        else // count-returning statements (cursors closed by the cloed thread)
        { wxString buf;
          buf.Printf(_(" >Statement has been executed, number of processed records or object number is: %u"), results[i]);
          add_line(buf);
        }
  add_line(_("Debug execution completed."));
  EnableControls(true);
  stmt_list_pos=-1;
  write_old_stmts();  // enables PREV-NEXT buttons
}

void SQLConsoleDlg::EnableControls(bool enable)
{ mExec ->Enable(enable);
  mPrev ->Enable(enable);
  mNext ->Enable(enable);
  mCmd  ->Enable(enable);
  mDebug->Enable(enable && cdp!=NULL);
}

/* The history of statements:
old_sql_statements contains the recent statemens, [0] is the most recent. The list is terminated by an empty statement.
When browsing among recent statements stmt_list_pos is the current index.
  Previous statement exists when stmt_list_pos>0 and has index stmt_list_pos-1.
  Next statement exists when stmt_list_pos+1<MAX_OLD_STMTS and has index stmt_list_pos+1, unless it is empty.
  The history contains only non-empty statements.
*/

void SQLConsoleDlg::add_to_stmt_list(wxString & stmt)
// Adds the executed statement on the beginning of the history list.
// If the list contains the same statement, removes it. Otherwise, if list is full, removes the last item in the list.
// Ignore empty statements (the damage the list logic)
{ int i, j; 
  if (stmt.IsEmpty()) return;
 // look for the same statement in the list:
  for (i=0;  i<MAX_OLD_STMTS;  i++)
    if (old_sql_statements[i].IsEmpty() || !_tcscmp(old_sql_statements[i].c_str(), stmt.c_str()))
      break;
 // remove the old statement or the last statement:
  if (i==MAX_OLD_STMTS) i--;
 // move the statements from the beginning one index higher, i-th will be destroyed:
  for (j=i;  j>0;  j--) old_sql_statements[j]=old_sql_statements[j-1];
 // add to the list:
  old_sql_statements[0]=stmt; // copy constructor, I hope
}

void SQLConsoleDlg::write_old_stmts(void)
{ mCmd->SetValue(stmt_list_pos<0 ? wxString() : old_sql_statements[stmt_list_pos]);
  mNext->Enable(stmt_list_pos>=0);
  mPrev->Enable(stmt_list_pos+1<MAX_OLD_STMTS && !old_sql_statements[stmt_list_pos+1].IsEmpty());
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SQLCONS_EXEC
 */

void SQLConsoleDlg::OnCdSqlconsExecClick( wxCommandEvent& event )
{ 
  if (!xcdp) // just to be safe, the button should be disabled
    { wxBell();  return; }
  wxString stmt = mCmd->GetValue();
  bool debugged = cdp!=NULL && mDebug->GetValue();  // never debugged on ODBC
  if (process_stmt(stmt, debugged) && debugged)  // debug in progress, controls disabled
  { add_to_stmt_list(stmt);
    mCmd->SetValue(wxEmptyString);
  }
  else
  { add_to_stmt_list(stmt);
   // this clears the statement edit box and resets the browsing in the history
    stmt_list_pos=-1;
    write_old_stmts();
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SQLCONS_PREV
 */

void SQLConsoleDlg::OnCdSqlconsPrevClick( wxCommandEvent& event )
// Displays the previous statement in the history. If it is not available, does nothing.
{ if (stmt_list_pos+1<MAX_OLD_STMTS && !old_sql_statements[stmt_list_pos+1].IsEmpty())
    stmt_list_pos++;
  write_old_stmts();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SQLCONS_NEXT
 */

void SQLConsoleDlg::OnCdSqlconsNextClick( wxCommandEvent& event )
{ if (stmt_list_pos>=0)
    stmt_list_pos--;
  write_old_stmts();
  event.Skip();
}

void SQLConsoleDlg::Push_cdp(xcdp_t xcdpIn)
// Couples the SQL console with a server, or detaches it if cdpIn==NULL
{ if (xcdp)
    if (xcdp == xcdpIn && (!xcdpIn->get_cdp() || strcmp(xcdpIn->get_cdp()->sel_appl_name, schema) == 0))
      return;
 // xcdp changed or schema changed in 602SQL cdp
 // save the new xcdp, cdp and schema:
  xcdp = xcdpIn;
  if (xcdp) cdp=xcdpIn->get_cdp();
  else cdp=NULL;
  if (cdp)
    strcpy(schema, cdp->sel_appl_name);
 // update the window:
  if (xcdp==NULL || cdp && !*cdp->sel_appl_name)
  { EnableControls(false);
    mSchema->SetLabel(_(" (Server/schema not selected)"));
  }
  else
  { stmt_list_pos=-1;
    EnableControls(true);
    write_old_stmts();  // disables the PREV and NEXT buttons
    wxString buf;
    if (cdp)
      buf.Printf(/*_(" (Server %s, schema %s.)"*/ _(" (on %s/%s)"), wxString(xcdp->locid_server_name, *cdp->sys_conv).c_str(), wxString(cdp->sel_appl_name, *cdp->sys_conv).c_str());
    else
      buf.Printf(_(" (on %s)"), wxString(xcdp->locid_server_name, *wxConvCurrent).c_str());
    mSchema->SetLabel(buf);
  }
}

void SQLConsoleDlg::Disconnecting(xcdp_t dis_xcdp)
{ if (xcdp==dis_xcdp)
    Push_cdp(NULL);
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for CD_SQLCONS_STMT
 */

void SQLConsoleDlg::OnCdSqlconsStmtEnter( wxCommandEvent& event )
{
    OnCdSqlconsExecClick(event);
    event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap SQLConsoleDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin SQLConsoleDlg bitmap retrieval
    return wxNullBitmap;
////@end SQLConsoleDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon SQLConsoleDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin SQLConsoleDlg icon retrieval
    return wxNullIcon;
////@end SQLConsoleDlg icon retrieval
}
