// enumsrv.h
#ifndef ENUMSRV_H
#define ENUMSRV_H

#ifdef LINUX
extern const char *configfile;
extern char locconffile[100];
#endif

void prep_regdb_access(void);

#define KEY_READ_EX KEY_READ //| KEY_WOW64_64KEY
#define KEY_ALL_ACCESS_EX KEY_ALL_ACCESS //| KEY_WOW64_64KEY
#define KEY_QUERY_VALUE_EX KEY_QUERY_VALUE //| KEY_WOW64_64KEY
#define KEY_SET_VALUE_EX KEY_SET_VALUE //| KEY_WOW64_64KEY
#define KEY_WRITE_EX KEY_WRITE //| KEY_WOW64_64KEY
#define KEY_ENUMERATE_SUB_KEYS_EX KEY_ENUMERATE_SUB_KEYS //| KEY_WOW64_64KEY

bool GetDatabaseString(const char * server_name, const char * value_name, void * dest, int bufsize, bool private_server);
bool SetDatabaseString(const char * server_name, const char * value_name, const char * val, bool private_server);
bool SetDatabaseNum(const char * server_name, const char * value_name, sig32 val, bool private_server);
bool IsServerRegistered(const char * server_name);
bool GetPrimaryPath(const char * server_name, bool private_server, char * path);
bool GetFilPath(const char * server_name, bool private_server, int part, char * path);

class t_enum_srv
{ bool both_classes;
  bool private_server;
  const char * version_req;
  int ind;
#ifdef WINS
  HKEY hKey;  
#else
  char * list;  int length;
#endif
  void open_passing(void);
  void close_passing(void);
 public:
  enum t_enum_class { ENUM_SRV_SYS=0, ENUM_SRV_PRIV=1, ENUM_SRV_BOTH=2 };
  t_enum_srv(t_enum_class classIn, const char * versionIn);
  ~t_enum_srv(void);
  bool next(tobjname server_name);
  inline bool is_private_server(void) const
    { return private_server; }
  inline bool GetDatabaseString(const char * server_name, const char * value_name, void * dest, int bufsize)
    { return ::GetDatabaseString(server_name, value_name, dest, bufsize, private_server); }
  inline bool GetPrimaryPath(const char * server_name, char * path)
    { return ::GetPrimaryPath(server_name, private_server, path); }
};

#endif // ENUMSRV_H

