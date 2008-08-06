/******************************************************************************/
/* filbckup.cpp - database file periodical backup                             */
/******************************************************************************/
#include <time.h>
/* Externi interface: deklaruje se globalni instance tridy fil_backup a periodicky 
   se vola check_for_fil_backup
   Interni fungovani: Existuji dve metody casovani zaloh - pres periodu a nastaveny 
   cas - z nichz by mela byt aktivni nejvyse jedna. Periodicke zalohovani se provadi, 
   kdyz se precetl BackupIntervalHours a BackupIntervalMinutes a interval zalohovani
   je nenulovy. Zalohovani v urcity cas se provadi, kdyz se precetl nektery 
   BackupTimeDayX a neni -Nikdy-.
   Opakovanemu zalohovani pro jeden predepsany cas se brani takto: pro kazdy cas je 
   urcen interval od tohoto casu po dalsich 300 sekund. Kdyz jsem v intervalu a neni 
   nahozeno backup_done, zalohuji a nahodim. Kdyz jsem mimo interval, shodim backup_done.
   300 sekund: musi byt vetsi nez perioda testovani (50s) plus pozdrzeni vlakna.
   Pri spousteni a zastavovani serveru branim vytvoreni vice kopii tak, ze pro startu 
   nastavim backup_done pro ty casy, ktere jsou aktivni.
*/

fil_backup::fil_backup(void)
{ last_backup=stamp_now();  
 // if the current time is in the time window for a backup, disable it (otherwise restarting the server winthin the window would re-create the backup)
  time_t long_time;  struct tm * tmbuf;
  long_time=time(NULL);  tmbuf=localtime(&long_time);
  if (!tmbuf) return; // date > 2040
  uns32 clk = 60*(60*tmbuf->tm_hour+tmbuf->tm_min)+tmbuf->tm_sec; // seconds since midnight
  unsigned int day=tmbuf->tm_wday;  if (!day) day=7;  day++;  // 2..8
  for (int i=0;  i<BACKUP_TIME_COUNT;  i++)
    if (backup_day[i])
    { backup_done[i] = clk >= backup_time[i] && clk < backup_time[i]+300 && 
        (backup_day[i]==1 || day==backup_day[i] || backup_day[i]==9 && day>=2 && day<=6 || backup_day[i]==10 && day>=7 && day<=8);
//       denne               prave dnes            pracovni dny                            weekend
    }
    else backup_done[i]=FALSE;
}

void fil_backup::read_backup_params(void)
// Precte parametry zalohovani. ATTN: The database file may be closed!
{// backup interval: 
  backup_interval=60*(60*the_sp.BackupIntervalHours.val()+the_sp.BackupIntervalMinutes.val());
 // backup time:
  backup_day[0]=the_sp.BackupTimeDay1.val();
  if (backup_day[0]) backup_time[0] = 60*(60*the_sp.BackupTimeHour1.val()+the_sp.BackupTimeMin1.val());
  backup_day[1]=the_sp.BackupTimeDay2.val();
  if (backup_day[1]) backup_time[1] = 60*(60*the_sp.BackupTimeHour2.val()+the_sp.BackupTimeMin2.val());
  backup_day[2]=the_sp.BackupTimeDay3.val();
  if (backup_day[2]) backup_time[2] = 60*(60*the_sp.BackupTimeHour3.val()+the_sp.BackupTimeMin3.val());
  backup_day[3]=the_sp.BackupTimeDay4.val();
  if (backup_day[3]) backup_time[3] = 60*(60*the_sp.BackupTimeHour4.val()+the_sp.BackupTimeMin4.val());
}

void fil_backup::backup_now(cdp_t cdp)     // messages go to the log but do not appear in the server's windows because the message cycle is blocked now
// cdp is cds[0] here.
{ 
  ffa.open_fil_access(cdp, server_name);  // important when (the_sp.CloseFileWhenIdle.val()!=0)
  backup_entry(cdp, NULL);
  cdp->request_init();  // clear possible errors
  ffa.close_fil_access(cdp);
}

bool verify_name_pattern(const char * name, const char * name_suffix)
// Checks if [name] ia a backup file name with [name_suffix].
{// test the part with date and time:
  if (name[8]!='.') return false;
  for (int i=0;  i<BACKUP_NAME_PATTERN_LEN;  i++) if (i!=8)
    if (name[i]<'0' || name[i]>'9')
      return false;
 // test the suffix:
  if (name_suffix)  // suffix may be 'z' or 'Z'!
    return !stricmp(name+11, name_suffix);
  return name[11]>='A' && name[11]<='D' || name[11]>='a' && name[11]<='d';
}

void backup_count_limit(const char * direc, const char * name_suffix)
// If the number of backups is bigger than limit, erases the oldest one
// Scans only for files with names in the form DDDDDDDD.DD[name_suffix]. When [name_suffix] == NULL, uses A,B,C,D suffixes.
// [direc] contains a full pathname of a database file or its pattern, using the directory path only.
{ char fname[MAX_PATH];
  if (the_sp.BackupFilesLimit.val() <= 0) return;
 // find the directory, stopre it to [fname]:
  strcpy(fname, direc);
  int len=(int)strlen(fname);
  while (len>0 && fname[len-1]!=PATH_SEPARATOR) len--;
  fname[len]=0;
 // search the directory for backups with standard names: 
  int count=0;
  // [len] is the beginning of the file name in [fname]
#ifdef WINS
  strcpy(fname+len, "*.*");
  unsigned _int64 time64ref = 0;
  WIN32_FIND_DATA FindFileData;
  HANDLE ffhnd = FindFirstFile(fname, &FindFileData); 
  if (ffhnd!=INVALID_HANDLE_VALUE)
  { do
      if (verify_name_pattern(FindFileData.cFileName, name_suffix))
      { count++;
        unsigned _int64 time64 = *(unsigned _int64*)&FindFileData.ftLastWriteTime;
        if (!time64ref || time64 < time64ref)
          { time64ref=time64;  strcpy(fname+len, FindFileData.cFileName); }
      }
    while (FindNextFile(ffhnd, &FindFileData));
    FindClose(ffhnd);
  }
#else  // UNIX
  time_t oldest_time=0;
  struct dirent *entry;
  DIR *dir=opendir(fname);
  if (dir)
  { while (NULL!=(entry=readdir(dir)))
      if (verify_name_pattern(entry->d_name, name_suffix))
      { struct stat fileinfo;  char auxname[MAX_PATH];
		    strcpy(auxname, fname);  strcpy(auxname+len, entry->d_name);
		    if (stat(auxname, &fileinfo))  // must not overwrite fname, it contains the name of the oldest file (so far)
			    continue;
        count++;
		    if (!oldest_time || oldest_time > fileinfo.st_mtime) 
		      { oldest_time=fileinfo.st_mtime;  strcpy(fname+len, entry->d_name); }
      }
    closedir(dir);
  }
#endif
 // now, count is the number of backup files with [name_suffix] and [fname] is the name of the oldes of these files:
  if (name_suffix==NULL)
  { if (count > the_sp.BackupFilesLimit.val() * ffa.backupable_fil_parts())
    { len=(int)strlen(fname)-1;
      fname[len]='A';  DeleteFile(fname);
      fname[len]='B';  DeleteFile(fname);
      fname[len]='C';  DeleteFile(fname);
      fname[len]='D';  DeleteFile(fname);
    }
  }
  else
  { if (count > the_sp.BackupFilesLimit.val())
      DeleteFile(fname);
  }
}

void fil_backup::check_for_fil_backup(cdp_t cdp)
{// prevent backup while replaying journal (both change the header.closetime)
  if (replaying) return;
 // periodical re-read of backup parameters:
  read_backup_params();
 // check if backing up enabled:
  if (!*the_sp.BackupDirectory.val()) return; 
 // check the backup interval:
  timestamp stamp = stamp_now();
  if (backup_interval && last_backup < stamp-backup_interval+25) // testing every 50 seconds, can do it 25 seconds before
  { backup_now(cdp);
    last_backup = stamp;
  }
 // check for the backup time:
  time_t long_time;  struct tm * tmbuf;
  long_time=time(NULL);  tmbuf=localtime(&long_time);
  if (!tmbuf) return; // date > 2040
  uns32 clk = 60*(60*tmbuf->tm_hour+tmbuf->tm_min)+tmbuf->tm_sec; // seconds since midnight
  unsigned int day=tmbuf->tm_wday;  if (!day) day=7;  day++;  // 2..8
  for (int i=0;  i<BACKUP_TIME_COUNT;  i++)
    if (backup_day[i])
    { BOOL active = clk >= backup_time[i] && clk < backup_time[i]+300 && 
        (backup_day[i]==1 || day==backup_day[i] || backup_day[i]==9 && day>=2 && day<=6 || backup_day[i]==10 && day>=7 && day<=8);
//       denne               prave dnes            pracovni dny                            weekend
      if (!active) backup_done[i]=FALSE;
      else if (!backup_done[i])
      { backup_now(cdp);
        backup_done[i]=TRUE;
        break;
      }
    }
}
///////////////////////////////// remote connection ////////////////////////
static char remote_path[MAX_PATH];
static int remote_dir_connection_counter = 0;

bool connect_remote_backup_dir(cdp_t cdp, const char * fname)  
// Returns the connected path in [remote_path] iff connected here.
{ *remote_path=0;
  if (!*the_sp.BackupNetPassword.val() || !*the_sp.BackupNetUsername.val())
    return true;
  { ProtectedByCriticalSection cs(&cs_short_term);  
    if (remote_dir_connection_counter)
    { remote_dir_connection_counter++;
      return true; 
    }
  }
#ifdef WINS
 // check fname for \\comp\share on its beginning, copy it to remote_path:
  if (fname[0]!='\\' || fname[1]!='\\') return true;
  const char * p = fname+2;
  while (*p && *p=='\\') p++;  // not necessary
  while (*p && *p!='\\') p++;  // skips comp name
  if (*p!='\\') return true;
  p++;
  if (!*p) return true;
  while (*p && *p!='\\') p++;  // skips share
  int len = (int)(p-fname);
  memcpy(remote_path, fname, len);  remote_path[len]=0;
 // validate the connection:
  if (_sqp_connect_disk(remote_path, NULL, NULL))
  { *remote_path=0;
    return false; 
  }
#endif  
  return true;
}

void disconnect_remote_backup_dir(void)
{
  if (*remote_path)
    _sqp_disconnect_disk(remote_path);
}

void hold_remote_backup_dir(void)
{ ProtectedByCriticalSection cs(&cs_short_term);  
  if (remote_dir_connection_counter) 
    remote_dir_connection_counter++;
}    
///////////////////////////////////////////////////////////////////////////
fil_backup a_backup;

void check_for_fil_backup(cdp_t cdp)
{ a_backup.check_for_fil_backup(cdp); }

void append_name_pattern(char * fname)
{ int i=(int)strlen(fname);
  if (fname[i-1]!=PATH_SEPARATOR) fname[i++]=PATH_SEPARATOR;
  uns32 dt=Today(); uns32 tm=Now();
  sprintf(fname+i, "%02u%02u%02u%02u.%02u", Year(dt)%100, Month(dt), Day(dt), Hours(tm), Minutes(tm));
}

int backup_entry(cdp_t cdp, const char * expl_fname)
// expl_fname is NULL for periodical server backups, NULLSTRING for client backup to a default directory and default name, 
//  other for specific pathname
// Returns 0 iff OK, 2 iff vetoed by the system trigger, error number on error.
// Errors cause calling request_error only if cdp!=cds[0].
// Writing message to the log moved here: I want all backups documented in the log, even the explicit ones.
{ char fname[MAX_PATH];
  if (expl_fname) a_backup.read_backup_params(); // called from client, must read the backup directory
  if (expl_fname && *expl_fname) strcpy(fname, expl_fname);
  else
  { strcpy(fname, the_sp.BackupDirectory.val());
    append_name_pattern(fname);
  }
 // connect: 
  if (!connect_remote_backup_dir(cdp, fname))
    return OS_FILE_ERROR;
 // pre-trigger:
  if (find_system_procedure(cdp, "_ON_SYSTEM_EVENT"))
  { char buf[100];  t_value res;
    sprintf(buf, "CALL _SYSEXT._ON_SYSTEM_EVENT(1,%d,\'\')", expl_fname!=NULL);
    tobjnum orig_user = cdp->prvs.luser();
    cdp->prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, TRUE);  // execute with CONF_ADMIN privils (hierarchy is necessary, I need to be the Admin os _sysext)
    exec_direct(cdp, buf, &res);
    cdp->prvs.set_user(orig_user, CATEG_USER, TRUE);  // return to original privils (may be DB_ADMIN)
    commit(cdp);
    cdp->request_init();
    if (res.intval==-1)  // Veto
    { char msg[100];
      form_message(msg, sizeof(msg), backupVetoed);
      display_server_info(msg);
      if (cdp!=cds[0])  // unless called by the server periodical planner
        request_error(cdp, INTERNAL_SIGNAL);
      return 2;
    }
  }

 // backup ([fname] contains the path and file name pattern):
  int err;
  if (the_sp.NonblockingBackups.val())
  { backup_status = 1;
    err = ffa.do_backup2(cdp, fname);
#if WBVERS>=1000
    if (err==ANS_OK) 
      err = backup_ext_ft(cdp, fname, true, true);
#endif
    backup_status = 0;
  }
  else
  { backup_status = 2;
    err = ffa.do_backup(cdp, fname);
#if WBVERS>=1000
    if (err==ANS_OK) 
      err = backup_ext_ft(cdp, fname, false, true);
#endif
    backup_status = 0;
  }
 // system trigger (called by CONF_ADMIN or system, no need to change the login):
  if (find_system_procedure(cdp, "_ON_BACKUP"))
  { char buf[40+MAX_PATH];
    sprintf(buf, "CALL _SYSEXT._ON_BACKUP(\'%s\',%s)", fname, err==ANS_OK ? "TRUE" : "FALSE");
    tobjnum orig_user = cdp->prvs.luser();
    cdp->prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, TRUE);  // execute with CONF_ADMIN privils (hierarchy is necessary, I need to be the Admin os _sysext)
    exec_direct(cdp, buf);
    cdp->prvs.set_user(orig_user, CATEG_USER, TRUE);  // return to original privils (may be DB_ADMIN)
    commit(cdp);
    cdp->request_init();
  }
 // disconnect if connected:
  WaitLocalManualEvent(&Zipper_stopped, INFINITE);
#ifdef WINS
  disconnect_remote_backup_dir();
#endif
 // message to the log:
  { char msg[100];
    form_message(msg, sizeof(msg), err==ANS_OK ? backupOK : backupError, err);
    display_server_info(msg);
  }
  return err;
}

//////////////////////////////////////// hot backup support ///////////////////////////////////////////////
/* Logic: Hot backup is in progress when [backup_support]!=NULL in the class describing the file access.
Phase 1: - database file is being copied
         - all write operations go to the auxiliary file, all blocks stored in the auxiliary file 
           are listed in [list]
         - all read operations check the [list] first, if the block is listed, it is read 
           from the auxiliary file, if not, then it is read from the database file 
         - if the file should grow, the new size (in blocks) is saved in [new_db_file_size] instead
Phase 2: - the copy is complete
         - blocks listed in the [list] are being copied from the auxiliary file to the database file 
           and removed from the list
         - all write operations go to the database file, if the block is in the [list] then
           it is removed from the [list]
         - all read operation go the save way as in phase 1  
After completing the backup:
- if new_db_file_size if bigger than the read file file, the file is expanded
- the auxiliary file is closed and deleted
If server terminates/crashes during the backup:
- in phase 1 is not completed, the backup file is not completed
- in any phase the contents of the auxiliary file must be moved into the database file on the next start
- this is done by function complete_broken_backup() called before other server threads start.
*/

bool t_backup_support::write_block(tblocknum blocknum, const void * ptr)
// Returns false on error.
{
 // search the list:
  int pos = start;
  while (pos<list.count() && *list.acc0(pos)!=blocknum) pos++;
  if (pos==list.count())
  { tblocknum * pbl = list.next();
    if (pbl==NULL) return false;
    *pbl=blocknum;
  }
 // write at [pos]:
  total_writes++;
  DWORD wr;
  if (!position2(hnd, (uns64)pos*(sizeof(tblocknum)+block_size())) || 
    !WriteFile(hnd, &blocknum, sizeof(tblocknum), &wr, NULL) ||
    !WriteFile(hnd, ptr, block_size(), &wr, NULL) || wr!=block_size())
    return false;
  return true;
}

void t_backup_support::invalidate_block(tblocknum blocknum)
{
 // search the list:
  int pos = start;
  while (pos<list.count() && *list.acc0(pos)!=blocknum) pos++;
 // exit if not found:
  if (pos==list.count()) return; 
  invals++; 
 // remove the record from the memory list (prevent synchronize(false) from writing it):
  *list.acc0(pos) = (tblocknum)-1;
 // remove the record from the file (prevent synchronize(true) from writing it):
  DWORD wr;
  blocknum = (tblocknum)-1;
  if (position2(hnd, (uns64)pos*(sizeof(tblocknum)+block_size())))
    WriteFile(hnd, &blocknum, sizeof(tblocknum), &wr, NULL);
}

bool t_backup_support::rd(unsigned pos, void * ptr)
{ DWORD rd;
  return position2(hnd, (uns64)pos*(sizeof(tblocknum)+block_size())+sizeof(tblocknum)) &&
         ReadFile(hnd, ptr, block_size(), &rd, NULL) && rd==block_size();
}

bool t_backup_support::read_block(tblocknum blocknum, void * ptr)
// Returns true if READ, false if must read from the DB file. Same operation in both phases.
{// search the list:
  int pos = start;
  while (pos<list.count() && *list.acc0(pos)!=blocknum) pos++;
 // exist if the block is not contained in the list (must read it from the database file):
  if (pos==list.count()) 
    return false;
 // write at [pos]:
  return rd(pos, ptr);
}

bool t_backup_support::prepare(bool restoring)
// Prepares the auxiliary file for hot backup - creates it or opens it.
{
  if (!GetDatabaseString(server_name, "TransactPath", auxi_file_name, sizeof(auxi_file_name), my_private_server) || !*auxi_file_name)
    strcpy(auxi_file_name, ffa.last_fil_path());  
  append(auxi_file_name, get_auxi_file_name());
#ifdef WINS
  hnd=CreateFile(auxi_file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, restoring ? OPEN_EXISTING : CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#else
  hnd=restoring ? _lopen(auxi_file_name, OF_READWRITE|OF_SHARE_DENY_WRITE) : _lcreat(auxi_file_name, 0666);
#endif
  if (!FILE_HANDLE_VALID(hnd)) 
    return false;  // when called from complete_broken_backup() this says that there is no broken backup available. Otherwise it is an error.
 // restore [list] from the auxiliary file, if [restoring]:
  if (restoring)
  { DWORD rd;  tblocknum blk;
    start=0;
    while (position2(hnd, (uns64)start*(sizeof(tblocknum)+block_size())) &&
           ReadFile(hnd, &blk, sizeof(tblocknum), &rd, NULL) && rd==sizeof(tblocknum))
    { *list.acc(start) = blk;  // [blk]==-1 if it has already been written
      start++;
    }  
  }
 // init variables: 
  total_writes=invals=0;
  start=0;
  phase2=false;   
 // save the current size of the database file:
  new_db_file_size = get_file_block_count();
  return true;
}


void t_backup_support::unprepare(void ** p_backup_support)
// Must be destructed in the CS, no other thread may call any member
{ 
 // delete the auxiliary file:
  CloseHandle(hnd);
  DeleteFile(auxi_file_name);
 // destroy the backup_support data structure:
  delete this;  // calls the virtual destructor!
  *p_backup_support=NULL;
}

void t_backup_support::synchronize(bool restoring, void ** p_backup_support)
{ char * buf;  unsigned count;
  buf = (char*)corealloc(block_size(), 55);
  { ProtectedByCriticalSection cs(file_cs);  // may wait here for completing the last addition into [list]
   // start phase 2: prevent adding new changes to the auxi file:
    phase2=true;
    count=list.count();
   // expand the database file if any blocks allocated ouside it during the backup and have not been written then:
   // must be done before block numbers are removed from the list, because this redirects read operations to the original file
    if (new_db_file_size > get_file_block_count())
      expand_file_size();
  }  
 // write old changes to the database file (without blocking the CS for the full time):
  for (start=0;  start<count;  start++)
  { tblocknum blk;
    { ProtectedByCriticalSection cs(file_cs);
      blk = *list.acc0(start);
     // copy the block unless invalidated or written before:
      if (blk!=(tblocknum)-1)
      {// copy the block from the aux file to the database file:
        if (rd(start, buf))
          write_buf_to_file(blk, buf);
       // remove the record from the aux file (prevents new writing it if the server terminates/crashes before completing phase 2):
        blk=(tblocknum)-1;
        DWORD wr;
        position2(hnd, (uns64)start*(sizeof(tblocknum)+block_size()));
        WriteFile(hnd, &blk, sizeof(tblocknum), &wr, NULL);
       // remove the record from the list:
        *list.acc0(start) = blk;
      }
    } // CS protection 
  } // cycle
  { ProtectedByCriticalSection cs(file_cs);  // must be destructed in the CS, no other thread may call any member
    unprepare(p_backup_support);
  }
  ffa.flush();  // until unprepare() the flush was blocked, flushing now
  corefree(buf);
}

#if WBVERS>=950
#include "extbtree.h"

void t_backup_support_ftx::write_buf_to_file(tblocknum blk, char * buf)
{
  eif->write(blk, (cbnode_ptr)buf);
}

tblocknum t_backup_support_ftx::get_file_block_count(void) const
{
  return eif->block_count;
}

void t_backup_support_ftx::expand_file_size(void)
// Called in the CS.
{
  eif->contain_block(new_db_file_size-2);
}

unsigned t_backup_support_ftx::block_size(void) const
{ return cb_cluster_size; }

#endif 

////////////////////////////////////// SQL backup API //////////////////////////////////////////
#include <dirscan.cpp>

#define ZIP602_STATIC
#include "Zip602.h"

typedef char t_backup_name[BACKUP_NAME_PATTERN_LEN+1];

bool is_name_pattern(const char * name)
// Checks if [name] ia a backup file name pattern.
{ 
  if (name[8]!='.') return false;
  for (int i=0;  i<BACKUP_NAME_PATTERN_LEN;  i++) if (i!=8)
    if (name[i]<'0' || name[i]>'9')
      return false;
  return true;    
}

void _sqp_backup_get_pathname_pattern(const char * directory_path, char * pathname_pattern)
{
  strcpy(pathname_pattern, *directory_path               ? directory_path : 
                           *the_sp.BackupDirectory.val() ? the_sp.BackupDirectory.val() :
                                                           ffa.last_fil_path());
  append_name_pattern(pathname_pattern);
}

int get_lob_name_type(const char * aname)
// Returns 1 for LOB file, 2 for LOB directory, 0 otherwise
{ int cnt = 0;
  do
  { if (aname[cnt]>='0' && aname[cnt]<='9' || aname[cnt]>='a' && aname[cnt]<='f' || aname[cnt]>='A' && aname[cnt]<='F')
      cnt++;
    else if (cnt & 1)  // odd number of digits
      return 0;
    else if (!aname[cnt])
      return 2;
    else if (!stricmp(aname+cnt, ".lob"))
      return 1;
    else 
      return 0;
  } while (true);      
}

int add_lobs_in_dir(INT_PTR hZip, const char * dirpath)
{ t_dir_scan ds;  
  ds.open(dirpath);
  while (ds.next())
  { const char * aname = ds.file_name();
    int tp = get_lob_name_type(aname);
    if (tp)
    { char itemname[MAX_PATH];  wchar_t wname[MAX_PATH];
      strcpy(itemname, dirpath);  append(itemname, aname);
      superconv(ATT_STRING, ATT_STRING, itemname, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
      int Err = ZipAdd(hZip, wname);
      if (Err) return Err;
      if (tp==2)
      { Err = add_lobs_in_dir(hZip, itemname);
        if (Err) return Err;
      }  
    }
  }
  ds.close();
  return 0;
}

int backup_ext_lobs(cdp_t cdp, const char * pathname_pattern)
{ char extlob_zip[MAX_PATH];  INT_PTR hZip = 0;  int Err;
  strcpy(extlob_zip, pathname_pattern);  strcat(extlob_zip, LOB_ARCHIVE_SUFFIX);
 // find the extlob directory: 
  const char * extlobdir = extlob_directory();
  if (!*extlobdir) extlobdir = ffa.last_fil_path();
  wchar_t wname[MAX_PATH];
  wchar_t wroot[MAX_PATH];
  superconv(ATT_STRING, ATT_STRING, extlob_zip, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
  superconv(ATT_STRING, ATT_STRING, extlobdir,  (char *)wroot, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
  Err = ZipCreate(wname, wroot, &hZip);
  if (!Err)
    Err = add_lobs_in_dir(hZip, extlobdir);
  if (hZip)
    ZipClose(hZip);
  return Err;
}

int _sqp_backup(cdp_t cdp, const char * pathname_pattern, bool nonblocking, int extent)
{
  int err=ANS_OK;
  if (extent & 1)
  { if (nonblocking)
    { backup_status = 1;
      { ProtectedByCriticalSection cs1(&commit_sem);  // the committed state if the database file must be freeded
        ProtectedByCriticalSection cs2(&ffa.fil_sem);
        if (ffa.backup_support) return INTERNAL_SIGNAL; // another backup in progress
        ffa.backup_support = new t_backup_support_fil(&ffa.fil_sem);
        if (!ffa.backup_support) return OUT_OF_KERNEL_MEMORY;
       // must not exit the CS before successfull prepare()
        if (!ffa.backup_support->prepare(false))
          { propagate_system_error("Error %s when openning auxiliary hot-backup file %s.", ffa.backup_support->my_auxi_file_name());
            delete ffa.backup_support;  ffa.backup_support=NULL;  
            return OS_FILE_ERROR;
          }  
      }
      // all write operations are redirected now
    }
    else
    { backup_status = 2;
      EnterCriticalSection(&commit_sem);  // must be before entering fil_sem
      EnterCriticalSection(&disksp.bpools_sem);  // must be before close_block_cache
      commit(cdp);       // before saving pieces must remove any error (it may occur in _on_server_event()!)
      cdp->request_init();
      save_pieces(cdp);  /* must be done before saving blocks! */
      commit(cdp);       // writes saved pieces
      disksp.close_block_cache(cdp);
      EnterCriticalSection(&ffa.remap_sem);  // protects the access to the [header]
      header.sezam_open=wbfalse;
      header.closetime=stamp_now();
      ffa.write_header();  
      EnterCriticalSection(&ffa.fil_sem);
    }
   // copy file:
    char fnames[MAX_PATH];
    strcpy(fnames, pathname_pattern);
    make_directory_path(fnames);  // creates the directory path for fname if it does not exist
  #ifdef WINS
    SYSTEMTIME systime;  GetSystemTime(&systime);
    FILETIME filetime;  SystemTimeToFileTime(&systime, &filetime);
  #endif
    int len = (int)strlen(fnames);  char filname[MAX_PATH];  
    char errmsg[256] = "";
    for (int i=0;  i<ffa.fil_parts;  i++)
    { 
  #ifdef STOP   // temporarily removed because of problems in Archibald  
      t_temp_priority_change tpc(backup_support==NULL);  // ABOVE if blocking, BELOW if nonblocking
  #endif    
      if (i || !ffa.first_part_read_only)
      { fnames[len]='A'+i;  fnames[len+1]=0;
        strcpy(filname, ffa.fil_path[i]);  append(filname, FIL_NAME);
        if (!CopyFileEx(filname, fnames, NULL, NULL, &bCancelCopying, 0)) 
        { propagate_system_error("Error %s when creating the database file backup in %s.", fnames, errmsg);
          err=OS_FILE_ERROR;
        }          
  #ifdef WINS
        else  // set file date and time
        { FHANDLE hnd=CreateFile(fnames, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
          if (FILE_HANDLE_VALID(hnd))
          { SetFileTime(hnd, &filetime, &filetime, &filetime);
            CloseFile(hnd);
          }
        }
  #endif
      }
    }  
    
   // unlock all after completing the backup:
    if (nonblocking)
    {// synchronize changes:
      ffa.backup_support->synchronize(false, (void**)&ffa.backup_support);
    }
    else
    { reload_pieces(cdp);
      header.sezam_open=wbtrue;
      ffa.write_header();
      LeaveCriticalSection(&ffa.fil_sem);
      LeaveCriticalSection(&ffa.remap_sem);  // protects the access to the [header]
      LeaveCriticalSection(&disksp.bpools_sem);  // must be before close_block_cache
      LeaveCriticalSection(&commit_sem);  // must be before entering fil_sem
    }
  }
#if WBVERS>=1000
 // backup external fulltext systems: 
  if (extent & 2)
    if (err==ANS_OK) 
      err = backup_ext_ft(cdp, pathname_pattern, nonblocking, false);
 // backup external LOB files: 
  if (extent & 4)
    if (err==ANS_OK) 
      err = backup_ext_lobs(cdp, pathname_pattern);
 // create the descriptor file     
  if (extent & (2|4))    
    if (err==ANS_OK) 
    { char fname[MAX_PATH];
      strcpy(fname, pathname_pattern);  strcat(fname, BACKUP_DESCRIPTOR_SUFFIX);
      FHANDLE hnd = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
      if (hnd!=INVALID_FHANDLE_VALUE)
      { DWORD wr;  const char * name;
        name = the_sp.ExtFulltextDir.val();
        WriteFile(hnd, name, 256, &wr, NULL);
        name = the_sp.extlob_directory.val();
        WriteFile(hnd, name, (int)strlen(name)+1, &wr, NULL);
        CloseFile(hnd);
      }
    }
#endif
  backup_status = 0;
  return err;
}

int _sqp_backup_reduce(const char * directory_path, int max_count)
{ int err=0;
  if (max_count<0)
  { max_count=the_sp.BackupFilesLimit.val();
    if (max_count <= 0) return 0;
  }  
  t_backup_name * backup_names = (t_backup_name*)corealloc(sizeof(t_backup_name)*(max_count+1), 49);
  if (!backup_names) return OUT_OF_KERNEL_MEMORY;
  int backup_count = 0;
 // search the directory for backups with standard names: 
 // find [max_count] newest backups:
  t_dir_scan ds;  
  ds.open(directory_path);
  while (ds.next())
  { const char * aname = ds.file_name();
    if (is_name_pattern(aname))
    {// find the name in [backup_names]
      int pos=0, res;
      while (pos<backup_count)
      { res=memcmp(backup_names[pos], aname, sizeof(t_backup_name)-1);
        if (res>0) pos++;
        else break;
      }  
      if (pos<max_count)
        if (pos==backup_count || res<0)  // insert the name into the [pos] position
        {// move following items:
          int mv = backup_count-pos;
          if (backup_count==max_count) mv--;
          else backup_count++;
          if (mv>0)
            memmove(backup_names+pos+1, backup_names+pos, sizeof(t_backup_name)*mv);
         // write it:  
          memcpy(backup_names[pos], aname, sizeof(t_backup_name));
          backup_names[pos][sizeof(t_backup_name)-1]=0;
        }  
    }
  }
  ds.close();  
 // new pass, deletes backups not contained in [backup_names]:
  ds.open(directory_path);
  while (ds.next())
  { const char * aname = ds.file_name();
    if (is_name_pattern(aname))
    {// find the name in [backup_names]
      int pos=0;
      do
      { if (pos==backup_count)
        { char fname[MAX_PATH];
          strcpy(fname, directory_path);  append(fname, aname);
          if (!DeleteFile(fname))
            err=-(int)GetLastError();
          break;  
        }
        else
          if (!memcmp(backup_names[pos], aname, sizeof(t_backup_name)-1)) break;
        pos++;
      } while (true); 
    }
  }
  ds.close();
  corefree(backup_names);
  return err;
}

int _sqp_backup_zip(const char * pathname_pattern, const char * dest_dir, const char * move_dir)
// Creates the standard name for the zip file in [dest_dir].
// Returns ZIP error.
{ INT_PTR hZip = 0;  int Err;
  char fil_copy_name[MAX_PATH], zip_pathname[MAX_PATH], search_name[MAX_PATH];
  strcpy(fil_copy_name, pathname_pattern);
  int len = (int)strlen(pathname_pattern), namestart;
  strcpy(search_name, pathname_pattern);  search_name[len-11]=0;  // directory only
    
 // prepare the pathname for the zip: 
  if (!dest_dir || !*dest_dir)
    strcpy(zip_pathname, pathname_pattern);
  else  // concatenate the [dest_dir] and name pattern:
  { strcpy(zip_pathname, dest_dir);
    namestart = len-1;
    while (namestart && pathname_pattern[namestart-1]!=PATH_SEPARATOR) namestart--;
    append(zip_pathname, pathname_pattern+namestart);
  }
  strcat(zip_pathname, "Z");  
 // create the ZIP file and add backup files into it:
  wchar_t wname[MAX_PATH];
  wchar_t wroot[MAX_PATH];
  wchar_t *root = NULL;
  superconv(ATT_STRING, ATT_STRING, zip_pathname, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
  superconv(ATT_STRING, ATT_STRING, fil_copy_name, (char *)wroot, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
  wchar_t *p = wcsrchr(wroot, PATH_SEPARATOR);
  if (p)
  { *p = 0;
    root = wroot;
  }
  Err = ZipCreate(wname, root, &hZip);
  if (!Err)
  { t_dir_scan ds;  
    ds.open(search_name);
    while (ds.next())
    { const char * aname = ds.file_name();
      if (is_name_pattern(aname))
        if (!memcmp(aname, pathname_pattern+len-11, 11) && aname[11]!='Z')
        { strcpy(fil_copy_name+len-11, aname);
          superconv(ATT_STRING, ATT_STRING, fil_copy_name, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
          Err = ZipAdd(hZip, wname);
          if (Err) break;
        }
    }
    ds.close();
  }
#ifdef STOP  // old version
  for (i=0;  !Err && i<ffa.fil_parts;  i++)
    if (i || !ffa.first_part_read_only)
    { fil_copy_name[len]='A'+i;  fil_copy_name[len+1]=0;
      superconv(ATT_STRING, ATT_STRING, fil_copy_name, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
      Err = ZipAdd(hZip, wname);
    }
#endif    
  if (Err)
  { if (hZip)
    { char msg[256] = "";
      ZipGetErrMsgA(hZip, msg, sizeof(msg));
      if (*msg) dbg_err(msg);
    }  
    else
      propagate_system_error("Error %s when zipping the backup.", NULL);
  }
  if (hZip)
    ZipClose(hZip);
 // delete the copy of the FIL:
  if (!Err)
  { t_dir_scan ds;  
    ds.open(search_name);
    while (ds.next())
    { const char * aname = ds.file_name();
      if (is_name_pattern(aname))
      if (!memcmp(aname, pathname_pattern+len-11, 11) && aname[11]!='Z')
      { strcpy(fil_copy_name+len-11, aname);
        DeleteFile(fil_copy_name);
      }
    }
    ds.close();  
   // move the zipped file:
    if (move_dir && *move_dir)
    { char move_name[MAX_PATH];
      strcpy(move_name, move_dir);
      append(move_name, pathname_pattern+namestart);
      strcat(move_name, "Z");  
      if (CopyFileEx(zip_pathname, move_name, NULL, NULL, &bCancelCopying, 0))
        DeleteFile(zip_pathname);  // delete source if copied OK
      else Err=-(int)GetLastError();  
    }
  }  
  return Err;
}

int _sqp_connect_disk(const char * path, const char * username, const char * password)
{
#ifdef WINS
  char UserName[100];  DWORD buflen = sizeof(UserName);
  if (WNetGetUser(path, UserName, &buflen) == NO_ERROR)
    return -1;  // connected
 // connect:
  NETRESOURCE NetResource;  
  NetResource.dwType=RESOURCETYPE_DISK;
  NetResource.lpLocalName=NULL;
  NetResource.lpProvider=NULL;
  NetResource.lpRemoteName=(LPSTR)path;
  if (!username || !*username) username = the_sp.BackupNetUsername.val();
  if (!password || !*password) password = the_sp.BackupNetPassword.val();
  DWORD res = WNetAddConnection2(&NetResource, password, username, 0);
  if (res!=NO_ERROR) 
    return -(int)res;
#if 0    
  if (res==NO_ERROR)
  { 
    HANDLE hnd = CreateFile("\\\\acer-honza\\backup\\xxx.rtf", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    if (hnd!=INVALID_HANDLE_VALUE)
    { sprintf(msg, "file handle %lx\r\n", hnd);
      log_msg(msg);
      CloseHandle(hnd);
    }
    else
    { sprintf(msg, "file open error %u\r\n", GetLastError());
      log_msg(msg);
    }  
    hnd = CreateFile("\\\\acer-honza\\backup\\testfile.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    if (hnd!=INVALID_HANDLE_VALUE)
    { sprintf(msg, "file handle %lx\r\n", hnd);
      log_msg(msg);
      WriteFile(hnd, "to je ono", 9, &wr, NULL);
      CloseHandle(hnd);
    }
    else
    { sprintf(msg, "file open error %u\r\n", GetLastError());
      log_msg(msg);
    }  
    res=WNetCancelConnection2(rem_name, 0, FALSE);
    sprintf(msg, "disconnection result %x\r\n", res);
    log_msg(msg);
  }  
  log_msg("service start completed\r\n");
  CloseHandle(hlogger);
#endif  
#else //  LINUX
  return -1;
#endif  
  return 0;
}

void _sqp_disconnect_disk(const char * path)
{
#ifdef WINS
  WNetCancelConnection(path, FALSE);
#endif  
}
