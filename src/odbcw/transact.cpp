/*
** TRANSACT.C - This is the ODBC driver code for processing transactions.
*/

#include "wbdrv.h"
#pragma hdrstop  

//	-	-	-	-	-	-	-	-	-

//	SQLC transaction control functions.

//	-	-	-	-	-	-	-	-	-

static RETCODE SQL_API local_SQLTransact(HENV henv, DBC * dbc, UWORD	fType)
// henv not used because hdbc should not be NULL in the driver
{ 
  CLEAR_ERR(dbc);
  if (!dbc->connected)
  { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
    return SQL_ERROR;
  }
 /* test asynchron. execution */
  if (dbc->autocommit == SQL_AUTOCOMMIT_ON)  /* no action */
    return SQL_SUCCESS;
 /* manual commit or roll-back */
  BOOL res;
  if (fType == SQL_COMMIT) res=cd_Commit(dbc->cdp);
  else res=cd_Roll_back(dbc->cdp);
  if (res) raise_db_err_dbc(dbc);  /* if not before Start_transaction, error is lost */
  cd_Start_transaction(dbc->cdp);
  return res ? SQL_ERROR : SQL_SUCCESS;
}

SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT CompletionType)
{ switch (HandleType) 
  { case SQL_HANDLE_ENV: // DM 3.0 does not map it to connections, driver has to
    { if (!IsValidHENV(Handle)) return SQL_INVALID_HANDLE;
      ENV * env = (ENV*)Handle;  
      CLEAR_ERR(env);
      SQLRETURN r, retcode = SQL_SUCCESS;
      for (DBC * dbc = env->dbcs;  dbc!=NULL;  dbc=dbc->next_dbc)
      { r=local_SQLTransact(SQL_NULL_HENV, dbc, CompletionType);
        if (r==SQL_ERROR) retcode=SQL_ERROR;
        else if (r==SQL_SUCCESS_WITH_INFO && retcode==SQL_SUCCESS)
          retcode=SQL_SUCCESS_WITH_INFO;
      }
      return retcode;
  }
  case SQL_HANDLE_DBC: 
      return local_SQLTransact(SQL_NULL_HENV, (DBC*)Handle, CompletionType);
      default: return SQL_ERROR;  // never performed
  }
}
  
