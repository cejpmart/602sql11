// enumsrv.c - server enumerator
#include "enumsrv.h"
#include "regstr.h"

#ifdef LINUX
const char *configfile="/etc/602sql";
char locconffile[100];
#endif

void prep_regdb_access(void)
{ 
#ifdef LINUX
  strcpy(locconffile, getenv("HOME"));
  strcat(locconffile, "/602sql");
#endif
}

bool GetDatabaseString(const char * server_name, const char * value_name, void * dest, int bufsize, bool private_server)
{ bool result=true;
#ifdef WINS
  char key[160];  HKEY hKey;
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegOpenKeyExA(private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)!=ERROR_SUCCESS)
    result=false;
  else
  { DWORD key_len=bufsize;
    if (RegQueryValueExA(hKey, value_name, NULL, NULL, (BYTE*)dest, &key_len)!=ERROR_SUCCESS)
      result=false;
    RegCloseKey(hKey);
  }
#else
  const char * cfile = private_server ? locconffile : configfile;
  if (bufsize>sizeof(uns32))
  { char *buf=(char *)alloca(bufsize);
    int len = GetPrivateProfileString(server_name, value_name, "\1", buf, bufsize, cfile);
    if (*buf==1) result=false;
    else memcpy(dest, buf, len+1);  // must not use strcpy, when value_name==NULL then buf may contain 0s
  }
  else  // reading a number
  { int val = GetPrivateProfileInt(server_name, value_name, -12345, cfile);
    if (val==-12345) result=false;
    else if (bufsize==sizeof(int))   *(int  *)dest=val;
    else if (bufsize==sizeof(short)) *(short*)dest=(short)val;
    else if (bufsize==sizeof(char))  *(char *)dest=(char)val;
  }
#endif
  return result;
}

bool SetDatabaseString(const char * server_name, const char * value_name, const char * val, bool private_server)
// Saves the database-related value in the registry or in /etc/602sql
{
#ifdef WINS
  char key[160];  HKEY hKey;
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegOpenKeyExA(private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS_EX, &hKey)!=ERROR_SUCCESS)
    return false;
  if (RegSetValueExA(hKey, value_name, NULL, REG_SZ, (BYTE*)val, (DWORD)strlen(val)+1)!=ERROR_SUCCESS)
    { RegCloseKey(hKey);  return false; }
  RegCloseKey(hKey);
  return true;
#else
  return WritePrivateProfileString(server_name, value_name, val, private_server ? locconffile : configfile) != FALSE;
#endif
}


#ifndef WINS
#ifndef SRVR  
BOOL WritePrivateProfileInt(const char * section, const char * entry, int val, const char * conffile)
{ char buf[20];
  sprintf(buf, "%d", val);
#ifdef SRVR  
  return WritePrivateProfileString(section, entry, buf, conffile);
#else
  return /*x*/WritePrivateProfileString(section, entry, buf, conffile);
#endif  
}
#endif  
#endif

bool SetDatabaseNum(const char * server_name, const char * value_name, sig32 val, bool private_server)
// Saves the database-related value in the registry or in /etc/602sql
{
#ifdef WINS
  char key[160];  HKEY hKey;
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegOpenKeyExA(private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS_EX, &hKey)!=ERROR_SUCCESS)
    return false;
  DWORD dval = val;
  if (RegSetValueExA(hKey, value_name, NULL, REG_DWORD, (BYTE*)&dval, sizeof(DWORD))!=ERROR_SUCCESS)
    { RegCloseKey(hKey);  return false; }
  RegCloseKey(hKey);
  return true;
#else
  const char * cfile = private_server ? locconffile : configfile;
  return WritePrivateProfileInt(server_name, value_name, val, cfile) != FALSE;
#endif
}

bool IsServerRegistered(const char * server_name)
{ 
#ifdef WINS
  char key[160];  HKEY hKey;  bool found = false;
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS) 
    { found=true;  RegCloseKey(hKey); }
  else
    if (RegOpenKeyExA(HKEY_CURRENT_USER, key, 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS) 
      { found=true;  RegCloseKey(hKey); }
  return found;
#else
  char vers[10];
  GetPrivateProfileString(server_name, Version_str, "\1", vers, sizeof(vers), configfile);
  if (vers[0]!=1) return true;
  GetPrivateProfileString(server_name, Version_str, "\1", vers, sizeof(vers), locconffile);
  return vers[0]!=1;
#endif
}

bool GetPrimaryPath(const char * server_name, bool private_server, char * path)
// Returns false and empty string in [path] when path is nos specified or server not registered.
{ *path=0;
  return GetDatabaseString(server_name, "Path",  path, MAX_PATH, private_server) ||
         GetDatabaseString(server_name, "Path1", path, MAX_PATH, private_server);
}

bool GetFilPath(const char * server_name, bool private_server, int part, char * path)
// Returns false and empty string in [path] when path is nos specified or server not registered.
{ 
  if (part==1) 
    return GetPrimaryPath(server_name, private_server, path);
  else
  { *path=0;
    char keyname[4+1+1];
    sprintf(keyname, "Path%u", part);
    return GetDatabaseString(server_name, keyname,  path, MAX_PATH, private_server);
  }  
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void t_enum_srv::open_passing(void)
{
#ifdef WINS
  char key[80];
  strcpy(key, WB_inst_key);  strcat(key, Database_str);
  if (RegOpenKeyExA(private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey) != ERROR_SUCCESS) 
    hKey=0;
#else
  list = new char[20001];
  if (list)
    length = GetPrivateProfileString(NULL, NULL, "", list, 20000, private_server ? locconffile : configfile);
#endif
  ind=0;
}

void t_enum_srv::close_passing(void)  // must be idempotent!
{
#ifdef WINS
  if (hKey) { RegCloseKey(hKey);  hKey=0; }
#else
  if (list) { delete [] list;  list = NULL; }
#endif
}

t_enum_srv::t_enum_srv(t_enum_class classIn, const char * versionIn) : version_req(versionIn)
{ 
  private_server = classIn==ENUM_SRV_PRIV;  // start with system servers when classIn==ENUM_SRV_BOTH
  both_classes = classIn==ENUM_SRV_BOTH;
  open_passing();
}

t_enum_srv::~t_enum_srv(void)
{
  close_passing();
}

bool t_enum_srv::next(tobjname server_name)
{ char vers[10];
  do  // cycle on classes
  {
#ifdef WINS
    if (!hKey) return false;
    while (RegEnumKeyA(hKey, ind, server_name, OBJ_NAME_LEN+1) == ERROR_SUCCESS)
    { ind++;
      *vers=0;
      GetDatabaseString(server_name, Version_str, vers, sizeof(vers));
      if (!*vers || !strcmp(vers, version_req))
        return true;
    }
#else
    if (!list) return false;
    while (ind+1<length)
    { if (list[ind]!='.')
      { strncpy(server_name, list+ind, sizeof(tobjname));  server_name[OBJ_NAME_LEN]=0;
        ind+=strlen(list+ind)+1;
        *vers=0;
        GetDatabaseString(server_name, Version_str, vers, sizeof(vers));
        if (!*vers || !strcmp(vers, version_req))
          return true;
      }
      else
        ind+=strlen(list+ind)+1;
    }
#endif
   // no new server found in the current class, may try another:
    close_passing();
    if (!both_classes || private_server) break;
    private_server=true;
    open_passing();
  } while (true);
  return false;
}

