// dirscan.h
////////////////////////////////////// directory scanning class //////////////////////////////////
#ifndef WINS
#include <sys/types.h>
#include <dirent.h>
#endif

class t_dir_scan
{
#ifdef WINS
  WIN32_FIND_DATA FindFileData;
  HANDLE ffhnd;
  bool have_first;
#else  
  struct dirent *entry;
  DIR * dir;
#endif
 public:
  t_dir_scan(void);
  ~t_dir_scan(void);
  void open(const char * direc);
  void close(void);
  bool next(void);
  const char * file_name(void) const;
  bool is_directory(void) const;
};

