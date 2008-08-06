// srvmng.cpp - local server management
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include "enumsrv.h"
#include "dirscan.cpp"

#define ZIP602_STATIC
#include "Zip602.h"

#ifdef LINUX
DWORD GetLastError(void);
#endif

void get_orig_dirs(const char * descr_pathname, char * ftx_dir, char * lob_dir)
{ *ftx_dir = *lob_dir = 0;
  FHANDLE hnd = CreateFile(descr_pathname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
  if (hnd!=INVALID_FHANDLE_VALUE)
  { DWORD rd;
    ReadFile(hnd, ftx_dir, 256, &rd, NULL);
    ReadFile(hnd, lob_dir, 256, &rd, NULL);
    CloseFile(hnd);
  }
}

void default_backup_dirs(const char * fil_direc, char * ftx_dir, char * lob_dir)
{
  if (!*ftx_dir) strcpy(ftx_dir, fil_direc);
  if (!*lob_dir) strcpy(lob_dir, fil_direc);
}

bool is_descriptor_file_name(const char * fname)
{ return !strcmp(fname+BACKUP_NAME_PATTERN_LEN, BACKUP_DESCRIPTOR_SUFFIX); }

bool is_lob_archive_name(const char * fname)
{ return !strcmp(fname+BACKUP_NAME_PATTERN_LEN, LOB_ARCHIVE_SUFFIX); }

bool is_fulltext_backup_name(const char * fname)
{ return fname[BACKUP_NAME_PATTERN_LEN]=='.' &&
         !strcmp(fname+strlen(fname)-4, EXT_FULLTEXT_SUFFIX); }

void rename_orig_file(const char * destfile, uns32 dt, uns32 tm, int part)
// [destfile] is the pathname of a part of the database file. Function renames it in the standard way.
{ char saved_file[MAX_PATH];
  strcpy(saved_file, destfile);
  sprintf(saved_file+strlen(saved_file)-strlen(FIL_NAME), "%02u%02u%02u%02u.%02u%c", Year(dt) % 100, Month(dt), Day(dt), Hours(tm), Minutes(tm), 'P'+part-1);
  DeleteFile(saved_file);  // MoveFile needs that the [saved_file] does not exist
  MoveFile(destfile, saved_file);
}

int unzip_lob_archive(const char * pathname, const char * lob_dir)
{ int err=0;  wchar_t wname[MAX_PATH];  INT_PTR hUnZip = 0;
  char filename[MAX_PATH], destfile[MAX_PATH];
  superconv(ATT_STRING, ATT_STRING, pathname, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), ext_unicode_specif, NULL);
  if (UnZipOpen(wname, &hUnZip)==Z_OK)
  { INT_PTR hFind, hFile;
    for (err = UnZipFindFirst(hUnZip, NULL, true, &hFind, &hFile);  err==Z_OK;  err = UnZipFindNext(hFind, &hFile))
    { UnZipGetFileName(hUnZip, hFile, wname, sizeof(wname) / sizeof(wchar_t));
      superconv(ATT_STRING, ATT_STRING, (char *)wname, filename, ext_unicode_specif, t_specif(0, GetHostCharSet(), 0, 0), NULL);
      strcpy(destfile, lob_dir);  append(destfile, filename);
      if (UnZipIsFolder(hUnZip, hFile))
        CreateDirectory(destfile, NULL);
      else  
      { DeleteFile(destfile);
        superconv(ATT_STRING, ATT_STRING, destfile, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), ext_unicode_specif, NULL);
        err=UnZipFile(hUnZip, hFile, wname);
      }  
    }
    if (err==Z_NOMOREFILES) err=Z_OK;
    if (hUnZip) UnZipClose(hUnZip);
  }
  return err;
}
  
CFNC DllKernel int WINAPI srv_Restore(const char * server_name, const char * pathname_pattern, BOOL save_original_database)
{ int err = 0;  char pathname[MAX_PATH], destfile[MAX_PATH];  wchar_t wname[MAX_PATH];  INT_PTR hUnZip = 0;
  char ftx_dir[MAX_PATH]= "", lob_dir[MAX_PATH]= "", filename[MAX_PATH];
  uns32 tm = Now(), dt = Today();
 // define [private_server]:
  bool private_server=false;
  if (!GetPrimaryPath(server_name, private_server, pathname) || !*pathname)
    private_server=true;
 // find the (last) database file path for the server: 
  char fil_direc[MAX_PATH];
  int part = MAX_FIL_PARTS;
  while (part && (!GetFilPath(server_name, private_server, part, fil_direc) || !*fil_direc))
    part--;
 // find and open the main zip file, if present:
  strcpy(pathname, pathname_pattern);  strcat(pathname, "Z");
  superconv(ATT_STRING, ATT_STRING, pathname, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), ext_unicode_specif, NULL);
  if (UnZipOpen(wname, &hUnZip)!=Z_OK)
  { hUnZip=0;
    strcpy(pathname, pathname_pattern);  strcat(pathname, "z");
    superconv(ATT_STRING, ATT_STRING, pathname, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), ext_unicode_specif, NULL);
    if (UnZipOpen(wname, &hUnZip)!=Z_OK)
      hUnZip=0;
  }
  if (hUnZip)
  { INT_PTR hFind, hFile;
   // find, unzip and read the descriptor file:
    bool descr_found=false;
    for (err = UnZipFindFirst(hUnZip, NULL, true, &hFind, &hFile);  err==Z_OK;  err = UnZipFindNext(hFind, &hFile))
    { if (UnZipIsFolder(hUnZip, hFile))
        continue;
      UnZipGetFileName(hUnZip, hFile, wname, sizeof(wname) / sizeof(wchar_t));
      superconv(ATT_STRING, ATT_STRING, (char *)wname, filename, ext_unicode_specif, t_specif(0, GetHostCharSet(), 0, 0), NULL);
      if (is_descriptor_file_name(filename))
      { strcpy(destfile, fil_direc);  append(destfile, filename);
        superconv(ATT_STRING, ATT_STRING, destfile, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), ext_unicode_specif, NULL);
        DeleteFile(destfile);
        if (UnZipFile(hUnZip, hFile, wname)==Z_OK)
        { get_orig_dirs(destfile, ftx_dir, lob_dir);
          DeleteFile(destfile);
          descr_found=true;
        }  
      }
    }
    if (err==Z_NOMOREFILES) err=Z_OK;
   // prepare directories if not specified in the descriptor file:
    default_backup_dirs(fil_direc, ftx_dir, lob_dir);
   // unzip all other files: 
    for (err = UnZipFindFirst(hUnZip, NULL, true, &hFind, &hFile);  err==Z_OK;  err = UnZipFindNext(hFind, &hFile))
    { if (UnZipIsFolder(hUnZip, hFile))
        continue;
      UnZipGetFileName(hUnZip, hFile, wname, sizeof(wname) / sizeof(wchar_t));
      superconv(ATT_STRING, ATT_STRING, (char *)wname, filename, ext_unicode_specif, t_specif(0, GetHostCharSet(), 0, 0), NULL);
      if (is_descriptor_file_name(filename)) continue;  // the descriptor file -> skip it
      int zlen=(int)strlen(filename);  
      if (zlen==BACKUP_NAME_PATTERN_LEN+1)
      { char id = filename[BACKUP_NAME_PATTERN_LEN] & 0xdf;  // A-D
        part=1+(id-'A');
        if (part<MAX_FIL_PARTS && GetFilPath(server_name, private_server, part, destfile) && *destfile)
        {// unless a bad file in the zip (ignored):
          append(destfile, FIL_NAME);  // forms the pathname of the database file, adds PATH_SEPARATOR, if not present
         // rename or remove the existing database file: 
          if (save_original_database)  // rename
            rename_orig_file(destfile, dt, tm, part);
          else // erase current
            DeleteFile(destfile);
         // copy the backup part to database file part (direc->fil_direc):
          superconv(ATT_STRING, ATT_STRING, destfile, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), ext_unicode_specif, NULL);
          err = UnZipFile(hUnZip, hFile, wname);
          if (err)
            break;
        }
      }
      else if (is_lob_archive_name(filename))
      {// unzip the LOB archive to the fil directory, temporarily:
        strcpy(destfile, fil_direc);  append(destfile, filename);
        superconv(ATT_STRING, ATT_STRING, destfile, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), ext_unicode_specif, NULL);
        DeleteFile(destfile);
        err = UnZipFile(hUnZip, hFile, wname);
        if (err) break;
       // unzip LOBs:
        err=unzip_lob_archive(destfile, lob_dir);
        if (err) break;
       // delete the LOB archive:
        DeleteFile(destfile);
      }
      else if (is_fulltext_backup_name(filename)) // extftx file supposed, unzip it to the EXTFT directory
      { strcpy(destfile, ftx_dir);  append(destfile, filename+BACKUP_NAME_PATTERN_LEN+1);
        superconv(ATT_STRING, ATT_STRING, destfile, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), ext_unicode_specif, NULL);
        DeleteFile(destfile);
        err = UnZipFile(hUnZip, hFile, wname);
        if (err) break;
      } // other files ignored
    } // cycle on files in the archive 
    if (err==Z_NOMOREFILES) err=Z_OK;
    if (hUnZip) UnZipClose(hUnZip);
  } 
  else  // zip not found, try restoring the normal files
  {// find directories, from the descriptor file or use the default: 
    strcpy(filename, pathname_pattern);  strcat(filename, BACKUP_DESCRIPTOR_SUFFIX);
    get_orig_dirs(filename, ftx_dir, lob_dir);
    default_backup_dirs(fil_direc, ftx_dir, lob_dir);
   // scan files:
    strcpy(pathname, pathname_pattern);
    int len = (int)strlen(pathname);
    while (len && pathname[len]!=PATH_SEPARATOR) len--;
    pathname[len]=0;
    const char * pattern = pathname_pattern + len + 1;
    t_dir_scan ds;  
    ds.open(pathname);
    while (ds.next() && !err)
    { const char * aname = ds.file_name();
      strcpy(pathname, pathname_pattern);  strcpy(pathname+len+1, aname);
      if (!memcmp(aname, pattern, BACKUP_NAME_PATTERN_LEN))
      { if (is_lob_archive_name(aname))
          err=unzip_lob_archive(pathname, lob_dir);
        else if (is_fulltext_backup_name(aname))
        { strcpy(destfile, ftx_dir);  append(destfile, aname+BACKUP_NAME_PATTERN_LEN+1);
          if (!CopyFile(pathname, destfile, FALSE))
            err=GetLastError();
        }  
        else if (strlen(aname)==BACKUP_NAME_PATTERN_LEN+1)
        { char id = aname[BACKUP_NAME_PATTERN_LEN] & 0xdf;  // A-D
          part=1+(id-'A');
          if (part<MAX_FIL_PARTS && GetFilPath(server_name, private_server, part, destfile))
          { append(destfile, FIL_NAME);
           // rename or remove the existing database file: 
            if (save_original_database)  // rename
              rename_orig_file(destfile, dt, tm, part);
            else // erase current
              DeleteFile(destfile);
            if (!CopyFile(pathname, destfile, FALSE))
              err=GetLastError();
          }
        }  
      }
    }
    ds.close();
  } 
  return err;
}
