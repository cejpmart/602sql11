// dirscan.cpp
#include <dirscan.h>
////////////////////////////////////// directory scanning API //////////////////////////////////
t_dir_scan::t_dir_scan(void)
{
#ifdef WINS
  ffhnd=INVALID_HANDLE_VALUE;
#else  
  dir=NULL;
#endif
}

t_dir_scan::~t_dir_scan(void)
{ close(); }

void t_dir_scan::open(const char * direc)
{ close();
#ifdef WINS
  char fname[MAX_PATH];
  strcpy(fname, direc);  append(fname, "*.*");
  ffhnd = FindFirstFile(fname, &FindFileData); 
  if (ffhnd!=INVALID_HANDLE_VALUE)
    have_first=true;
#else  
  dir=opendir(direc);
#endif
}

void t_dir_scan::close(void)
{
#ifdef WINS
  if (ffhnd!=INVALID_HANDLE_VALUE) 
    { FindClose(ffhnd);  ffhnd=INVALID_HANDLE_VALUE; }
#else
  if (dir)
    { closedir(dir);  dir=NULL; }
#endif
}

bool t_dir_scan::next(void)
{
#ifdef WINS
  if (ffhnd==INVALID_HANDLE_VALUE) return false;
  if (have_first) 
    { have_first=false;  return true; }
  return FindNextFile(ffhnd, &FindFileData)==TRUE;
#else
  if (!dir) return false;
  entry=readdir(dir);
  return entry != NULL;
#endif
}

const char * t_dir_scan::file_name(void) const
// Supposed that next() returned true.
{
#ifdef WINS
  return FindFileData.cFileName;
#else
  return entry->d_name;
#endif
}

bool t_dir_scan::is_directory(void) const
{
#ifdef WINS
  return (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
  return false; // ###
#endif
}

