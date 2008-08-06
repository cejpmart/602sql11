// licence8.cpp - server licence management

/* Invalid keys are not removed but they are ignored. The may become valid again whe a temporary change
     in the computer configuration is undone.
*/
#ifdef WINS
#include "tchar.h"
HINSTANCE hLicLib;
int liclib_counter = 0;

typedef int WINAPI t_VerifyRKIK4(const char * RegistrationKey, const char * InstallationKey, int * RemainingDays);
typedef int WINAPI t_GetInfoFromIK(const char * InstallationKey, int * BuildNumber, int * OSType,
  int * SystemLanguage, int * BrowserType, int * Year, int * Day, int * Month, int * Hour, int * Min, int * Sec);
typedef int WINAPI t_VerifyIK(char * InstallationKey, int * RemainingDays);
typedef int WINAPI t_CreateIK3(char * ProductCategory, char * LicenceType, char * ProductLanguage,
  int BuildNumber, int OSType, int SystemLanguage, int BrowserType, int NumberOfUsers, char * InstallationKey);
// vytvori instalation key ve tvaru PPPTTJ-BBBOLW-LLL-XXXXXXXXX
// v InstallationKey vyzaduje predat naalokovanou pamet o delce >= 28 Byte

t_VerifyRKIK4 * p_VerifyRKIK4;
t_GetInfoFromIK * p_GetInfoFromIK;
t_VerifyIK * p_VerifyIK;
t_CreateIK3 * p_CreateIK3;

bool xload_liclib(void)
{ if (liclib_counter) { liclib_counter++;  return true; }
  hLicLib=LoadLibrary(_T("M602OU32.DLL"));
  if (!hLicLib) return false;
  p_VerifyRKIK4   = (t_VerifyRKIK4  *)GetProcAddress(hLicLib, "DontBeCurious13");
  p_GetInfoFromIK = (t_GetInfoFromIK*)GetProcAddress(hLicLib, "DontBeCurious11");
  p_VerifyIK      = (t_VerifyIK     *)GetProcAddress(hLicLib, "DontBeCurious14");
  p_CreateIK3     = (t_CreateIK3    *)GetProcAddress(hLicLib, "DontBeCurious06");
  if (!p_VerifyRKIK4 || !p_GetInfoFromIK || !p_VerifyIK || !p_CreateIK3) return false;
  liclib_counter=1;
  return true;
}

void unload_liclib(void)
{ if (--liclib_counter) return;
  FreeLibrary(hLicLib);  hLicLib=0;
}

#define VerifyRKIK4   (*p_VerifyRKIK4)
#define GetInfoFromIK (*p_GetInfoFromIK)
#define VerifyIK      (*p_VerifyIK)
#define CreateIK3     (*p_CreateIK3)


#else // LINUX
#define VerifyRKIK4   DontBeCurious13
#define GetInfoFromIK DontBeCurious11
#define VerifyIK      DontBeCurious14
#define CreateIK3     DontBeCurious06
extern unsigned char mac_address[6];
bool get_mac_address(void);

#if defined(__ia64__) || defined(__x86_64)  // temp. replacement

extern "C" int __stdcall VerifyRKIK4 (const char * RegistrationKey, const char * InstallationKey, int * RemainingDays)
{ return 0; }
extern "C" int __stdcall GetInfoFromIK (const char * InstallationKey, int * BuildNumber, int * OSType,
  int * SystemLanguage, int * BrowserType, int * Year, int * Day, int * Month, int * Hour, int * Min, int * Sec)
{ * Year=2003;  * Month=11; * Day=4;  sscanf(InstallationKey+8, "%d", BuildNumber); return 0; }
extern "C" int __stdcall VerifyIK (char * InstallationKey, int * RemainingDays)
{ return 0; }
extern "C" int __stdcall CreateIK3 (char * ProductCategory, char * LicenceType, char * ProductLanguage,
  int BuildNumber, int OSType, int SystemLanguage, int BrowserType, int NumberOfUsers, char * InstallationKey)
{ sprintf(InstallationKey, "SQALINUX%d", BuildNumber);  return 0; }

#else  // 32-bit Linux

extern "C" int __stdcall VerifyRKIK4 (const char * RegistrationKey, const char * InstallationKey, int * RemainingDays) __attribute__((stdcall));
//{ return 0; }
extern "C" int __stdcall GetInfoFromIK (const char * InstallationKey, int * BuildNumber, int * OSType,
  int * SystemLanguage, int * BrowserType, int * Year, int * Day, int * Month, int * Hour, int * Min, int * Sec) __attribute__((stdcall));
//{ * Year=2002;  * Month=9; * Day=4;  sscanf(InstallationKey+8, "%d", BuildNumber); return 0; }
extern "C" int __stdcall VerifyIK (char * InstallationKey, int * RemainingDays) __attribute__((stdcall));
//{ return 0; }
extern "C" int __stdcall CreateIK3 (char * ProductCategory, char * LicenceType, char * ProductLanguage,
  int BuildNumber, int OSType, int SystemLanguage, int BrowserType, int NumberOfUsers, char * InstallationKey) __attribute__((stdcall));
//{ sprintf(InstallationKey, "SQALINUX%d", BuildNumber);  return 0; }
#endif

bool xload_liclib(void)
{ return true; }

void unload_liclib(void)
{ }
#endif

#define MAX_BUFFER_SIZE    10000

#ifdef WINS
static const char lic_key[] = "\\Licences\\";
const char section_ik           [] = "InstallationKeys",
           section_server_reg   [] = "ServerRegKey",
           section_server_lic   [] = "ServerLicenceNumber",
           section_client_access[] = "ClientAccessLicences";
//           database_ik          [] = "InstallationKey";
#else
const char section_ik           [] = ".InstallationKeys",
           section_server_reg   [] = ".ServerRegKey",
           section_server_lic   [] = ".ServerLicenceNumber",
           section_client_access[] = ".ClientAccessLicences";
//           database_ik          [] = "InstallationKey";
#endif

#if WBVERS<900
#define IK_prefix "SA1"
#else
#define IK_prefix "SQ2"
#endif

bool store_licence_number(const char * section, const char * number)
// Stores the given licence number
{
#ifdef WINS
  HKEY hKey;  char keyname[200];
  strcpy(keyname, WB_inst_key);  strcat(keyname, lic_key);  strcat(keyname, section);
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyname, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
    return false;
  if (RegSetValueExA(hKey, number, NULL, REG_SZ, (BYTE*)NULLSTRING, 1) != ERROR_SUCCESS)
    { RegCloseKey(hKey);  return false; }
    RegCloseKey(hKey);
  return true;
#else
  bool res = WritePrivateProfileString(section, number, NULLSTRING, configfile) != FALSE;
#ifdef SRVR
  if (!res) dbg_err("Cannot write the configuration file");
#endif
  return res;
#endif
}

bool drop_licence_number(const char * section, const char * number)
// Removes the given licence number
{
#ifdef WINS
  HKEY hKey;  char keyname[200];
  strcpy(keyname, WB_inst_key);  strcat(keyname, lic_key);  strcat(keyname, section);
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyname, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
    return false;
  if (RegDeleteValueA(hKey, number) != ERROR_SUCCESS)
    { RegCloseKey(hKey);  return false; }
    RegCloseKey(hKey);
  return true;
#else
  bool res = WritePrivateProfileString(section, number, NULL, configfile) != FALSE;
#ifdef SRVR
  if (!res) dbg_err("Cannot write the configuration file");
#endif
  return res;
#endif
}

bool get_licence_list(const char * section, char * buf, int bufsize)
// Returns all licence numbers from a section
{
#ifdef WINS
  HKEY hKey;  char keyname[200];  bool res=true;
  strcpy(keyname, WB_inst_key);  strcat(keyname, lic_key);  strcat(keyname, section);
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyname, 0, "", REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL) != ERROR_SUCCESS)
    return false;
  int index=0, pos=0;
  do
  { char alic[MAX_LICENCE_LENGTH+1]; BYTE minibuf[2];
    DWORD namesize = sizeof(alic), valsize = sizeof(minibuf);
    LONG err = RegEnumValueA(hKey, index++, alic, &namesize, NULL, NULL, minibuf, &valsize);
    if (err == ERROR_MORE_DATA) continue;
    if (err != ERROR_SUCCESS) break;
    namesize=(int)strlen(alic);
    if (namesize)  // ignore the default value (if specified by an error)
    { if (pos+namesize+2 > bufsize) 
        { res=false;  break; }
      strcpy(buf+pos, alic);  pos+=namesize+1;
    }
  } while (true);
  if (bufsize) buf[pos]=0;  else res=false; // list terminator
  RegCloseKey(hKey);
  return res;
#else
  GetPrivateProfileString(section, NULL, NULLSTRING, buf, bufsize, configfile);
  return true;
#endif
}

#include <time.h>

#if WBVERS>=810
bool get_8081_upgrade(int & year, int & month, int & day)
{ bool res = false;  unsigned upg_info;
#ifdef WINS
  HKEY hKey;  char keyname[200];  
  strcpy(keyname, WB_inst_key);  strcat(keyname, lic_key);  strcat(keyname, section_server_reg);
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyname, 0, "", REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL) == ERROR_SUCCESS)
  { 
    DWORD key_len = sizeof(upg_info);
    if (RegQueryValueExA(hKey, "UpgradeInfo", NULL, NULL, (BYTE*)&upg_info, &key_len) == ERROR_SUCCESS)
      res=true;
    RegCloseKey(hKey);
  }
#else
  upg_info = GetPrivateProfileInt("Upgrade", "Info", 0, configfile);
  res = upg_info!=0;
#endif
  if (res)
  { year=2003+(upg_info % 7);  upg_info /= 7;
    month = upg_info % 17;     upg_info /= 17;
    day = upg_info;
    if (year>2006 || !month || month>12 || !day || day>31) res=false;
  }
  return res;
}

bool store_8081_upgrade(void)
{ int year, month, day;
  if (get_8081_upgrade(year, month, day)) return true; // already stored, do not overwrite
  time_t long_time;  struct tm * tmbuf;
  long_time=time(NULL);  tmbuf=localtime(&long_time);
  if (tmbuf==NULL) return false;
  day =tmbuf->tm_mday;  month=tmbuf->tm_mon+1;  year=1900+tmbuf->tm_year;
  unsigned upg_info = 7 * (17*day+month) + (year-2003);
  bool res = false;
#ifdef WINS
  HKEY hKey;  char keyname[200];
  strcpy(keyname, WB_inst_key);  strcat(keyname, lic_key);  strcat(keyname, section_server_reg);
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyname, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
  { res = RegSetValueExA(hKey, "UpgradeInfo", NULL, REG_DWORD, (BYTE*)&upg_info, sizeof(upg_info)) == ERROR_SUCCESS;
    RegCloseKey(hKey);
  }
#else
  char numstr[20];
  sprintf(numstr, "%u", upg_info);
  res = WritePrivateProfileString("Upgrade", "Info", numstr, configfile) != FALSE;
#ifdef SRVR
  if (!res) dbg_err("Cannot write the configuration file");  // no _() !
#endif
#endif
  return res;
}

#endif
////////////////////////// special relation /////////////////////////////////
// Special registration is related to IK (same chars 3-9).
// It contains OPL number (chars 10-12) and X or Y in char 2.
// When checking the server registration, the IK must be related to the location of program, not database!

CFNC DllSpecial BOOL WINAPI get_any_valid_ik(char * ik, const char * env_info);

unsigned char2num36(char c)
{ if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='Z') return c-'A'+10;
  if (c>='a' && c<='z') return c-'a'+10;
  return 36;
}

unsigned char2num35(char c)
{ if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='M') return c-'A'+10;
  if (c>='a' && c<='m') return c-'a'+10;
  if (c>='P' && c<='Z') return c-'A'+9;
  if (c>='p' && c<='z') return c-'a'+9;
  if (c=='o' || c=='O') return 0;
  return 35;
}

char num2char35(unsigned num)
{ if (num<=9) return '0'+num;
  if (num<35) 
  { char c = 'A'+num-10;
    if (c>='O') c++;
    return c;
  }
  return '-';
}

void make_ik_digest(uns64 & dig, const char * ik)
{ while (*ik)
  { 
#ifdef WINS
    dig = dig % 2183614814579;
#else
    dig = dig % 2183614814579ull;
#endif
    dig = 36*dig+char2num36(*ik);
    ik++;
  }
}

void make_cksum(char * res, uns64 dig)
{ int i=0;
  while (dig)
  { res[i++]=num2char35((unsigned)(dig % 35));
    dig /= 35;
  }
  if (i<9) memcpy(res+i, "M5QU8X2RW", 9-i);
  res[9]=0;
  char c=res[8];  res[8]=res[2];  res[2]=c;
  c=res[7];  res[7]=res[4];  res[4]=c;
}

CFNC DllSpecial BOOL WINAPI check_special_relation(const char * ik, const char * lcs)
{ uns64 dig = 0;  char cpy[MAX_LICENCE_LENGTH+1];
  make_ik_digest(dig, ik);
  if (memcmp(lcs, "SQ", 2)) return FALSE;
  if (lcs[2]!='X' && lcs[2]!='Y') return FALSE;
  if (memcmp(lcs+3, ik+3, 7)) return FALSE;
  //if (memcmp(lcs+13, "-000-", 5)) return FALSE;
  if (lcs[13]!='-' || lcs[17]!='-') return FALSE;
  memcpy(cpy, lcs, 17);
  cpy[17]=0;
  make_ik_digest(dig, cpy);
  cpy[17]='-';
  make_cksum(cpy+18, dig);
  return !memcmp(lcs, cpy, MAX_LICENCE_LENGTH);
}

static void create_special_relation(const char * ik, char tp, unsigned opl_code, unsigned opl_lic_cnt, char * lcs)
{ uns64 dig = 0;
  make_ik_digest(dig, ik);
  memcpy(lcs, "SQ", 2);  
  lcs[2]=tp;
  memcpy(lcs+3, ik+3, 7);  
  lcs[10]=num2char35(opl_code % 35);  opl_code /= 35;
  lcs[11]=num2char35(opl_code % 35);  opl_code /= 35;
  lcs[12]=num2char35(opl_code % 35);  
  lcs[13]='-';
  lcs[14]='0'+opl_lic_cnt/100;  opl_lic_cnt %= 100;
  lcs[15]='0'+opl_lic_cnt/10;   opl_lic_cnt %= 10;
  lcs[16]='0'+opl_lic_cnt;
  //memcpy(lcs+13, "-000", 5);
  lcs[17]=0;
  make_ik_digest(dig, lcs);
  lcs[17]='-';
  make_cksum(lcs+18, dig);
}

CFNC DllSpecial void WINAPI special_registration_ik(const char * ik, const char * opl_key, int type, char * lcs, int * result)
{ int BuildNumber, OSType, SystemLanguage, BrowserType, Year, Day, Month, Hour, Min, Sec;  unsigned opl_lic_cnt;
 // check and analyse the OPL key:
  if (memcmp(opl_key, IK_prefix, 3)) { *result=6;  return; }
  opl_lic_cnt=atoi(opl_key+14);
  if (!xload_liclib()) { *result=7;  return; }
  int res = GetInfoFromIK (opl_key, &BuildNumber, &OSType, &SystemLanguage, &BrowserType, &Year, &Day, &Month, &Hour, &Min, &Sec);
  unload_liclib();
  if (res) { *result=1;  return; }
 // check for the expiration of the OPL key:
  time_t long_time;  struct tm * tmbuf;
  long_time=time(NULL);  tmbuf=localtime(&long_time);
  if (tmbuf==NULL) { *result=2;  return; }
  if (Year<1900+tmbuf->tm_year || 
      Year==1900+tmbuf->tm_year && 
      (Month<tmbuf->tm_mon+1 || Month==tmbuf->tm_mon+1 && Day<tmbuf->tm_mday))
    { *result=3;  return; }
  if (BuildNumber % 7 != 3) { *result=4;  return; }
  store_licence_number(section_ik, ik);  // may cause duplicity error, ignored
  create_special_relation(ik, type ? 'X' : 'Y', BuildNumber / 7, opl_lic_cnt, lcs);
  store_licence_number(section_server_reg, lcs);  // may cause duplicity error, ignored
  *result=0;  
}

CFNC DllSpecial void WINAPI special_registration(const char * env_info, const char * opl_key, int type, char * lcs, int * result)
// env_info must specify the location of programs, not database!
{ char ik[MAX_LICENCE_LENGTH+1];
  if (!get_any_valid_ik(ik, env_info)) { *result=5;  return; }
  special_registration_ik(ik, opl_key, type, lcs, result);
}

bool create_opl_key(char * ik, int opl_num, unsigned opl_lic_cnt)
{ bool result = false;
  *ik=0;
  if (xload_liclib()) 
  { 
#ifdef ENGLISH
    if (CreateIK3 (IK_prefix, "T1", "A", 7*opl_num+3, 0, 6, 1, opl_lic_cnt, ik) == 0)
#else
    if (CreateIK3 (IK_prefix, "T1", "C", 7*opl_num+3, 0, 6, 1, opl_lic_cnt, ik) == 0)
#endif
      result=true;
    unload_liclib();
  }
  return result;
}

/////////////////////////////////////////////////////////////////////////////
bool find_licence_number(const char * section, const char * number)
{ char * buf = (tptr)corealloc(MAX_BUFFER_SIZE, 77);
  if (!buf) return false;
  bool res=false;
  if (get_licence_list(section, buf, MAX_BUFFER_SIZE))
    for (char * lic=buf;  *lic;  lic += strlen(lic)+1)
      if (!strcmp(lic, number))
        { res=true;  break; }
  corefree(buf);
  return res;
}

#define HW_IDENT_LIMIT 40000

static unsigned get_hw_ident(const char * env_info)
{
#ifdef WINS
  char path[MAX_PATH];  DWORD serial;
 // get the volume name terminated by '\'
  if (env_info && *env_info) strcpy(path, env_info);
  else GetModuleFileNameA(NULL, path, sizeof(path));
  if (path[1]==':') strcpy(path+2, PATH_SEPARATOR_STR);
  else if (path[0]==PATH_SEPARATOR && path[1]==PATH_SEPARATOR)
  { int i=2;
    while (path[i] && path[i]!=PATH_SEPARATOR) i++;  // skip computer name
    if (path[i])
    { i++;
      while (path[i] && path[i]!=PATH_SEPARATOR) i++;  // skip share name
      path[i+1]=0;                                     // stop after the PATH_SEPARATOR
    }
  }
  else path[0]=0;
 // get serial number:
  DWORD aux1, aux2;
  //MessageBox(0, path, "path", MB_OK|MB_APPLMODAL);
  GetVolumeInformationA(path, NULL, 0, &serial, &aux1, &aux2, NULL, 0);
  return serial % HW_IDENT_LIMIT;
#else
  uns64 mac = (uns64)*(uns32*)mac_address + (((uns64)*(uns16*)(mac_address+4)) << 32);
  return mac % HW_IDENT_LIMIT;
#endif
}

#ifndef SRVR
CFNC DllSpecial bool WINAPI verify_ik(const char * InstallationKey, const char * env_info, int * year=NULL, int * month=NULL, int * day=NULL);
#endif

CFNC DllSpecial bool WINAPI verify_ik(const char * InstallationKey, const char * env_info, int * year, int * month, int * day)
{ int BuildNumber, OSType, SystemLanguage, BrowserType, Year, Day, Month, Hour, Min, Sec;
  if (memcmp(InstallationKey, IK_prefix, 3)) return false;
  if (!xload_liclib()) return false;
  bool res = GetInfoFromIK (InstallationKey, &BuildNumber, &OSType, &SystemLanguage, &BrowserType, &Year, &Day, &Month, &Hour, &Min, &Sec)==0;
  unload_liclib();
 // check the hardware link:
  if (BuildNumber!=get_hw_ident(env_info)) 
#ifdef WINS  // the default env_info is allowed as well (compatibility)
    if (env_info==NULL || BuildNumber!=get_hw_ident(NULL))
#endif
      res=false; 
 // if the IK is OK, return its data:
  if (res)
  { if (year)  *year =Year;
    if (month) *month=Month;
    if (day)   *day  =Day;
  }
  return res;
}

CFNC DllSpecial bool WINAPI verify_ik2(const char * InstallationKey, const char * env_info, int * year=NULL, int * month=NULL, int * day=NULL);

CFNC DllSpecial bool WINAPI verify_ik2(const char * InstallationKey, const char * env_info, int * year, int * month, int * day)
{ int BuildNumber, OSType, SystemLanguage, BrowserType, Year, Day, Month, Hour, Min, Sec;
  if (memcmp(InstallationKey, IK_prefix, 3)) return false;
  if (!xload_liclib()) return false;
  bool res = GetInfoFromIK (InstallationKey, &BuildNumber, &OSType, &SystemLanguage, &BrowserType, &Year, &Day, &Month, &Hour, &Min, &Sec)==0;
  unload_liclib();
 // check the hardware link:
  if (BuildNumber!=get_hw_ident(env_info)) 
    res=false;
 // if the IK is OK, return its data:
  if (res)
  { if (year)  *year =Year;
    if (month) *month=Month;
    if (day)   *day  =Day;
  }
  return res;
}

CFNC DllSpecial bool WINAPI create_ik(char * ik, const char * env_info)
{ bool result = false;
  *ik=0;
  if (xload_liclib()) 
  { 
#ifdef ENGLISH
    if (CreateIK3 (IK_prefix, "T1", "A", get_hw_ident(env_info), 0, 6, 1,   0, ik) == 0)
#else
    if (CreateIK3 (IK_prefix, "T1", "C", get_hw_ident(env_info), 0, 6, 1,   0, ik) == 0)
#endif
      result=store_licence_number(section_ik, ik);
    unload_liclib();
  }
  return result;
}

inline bool is_temp_server_reg(const char * RegistrationKey)
{ return !memcmp(RegistrationKey, "SQX", 3) || !memcmp(RegistrationKey, "SQY", 3); }

bool valid_addon_prefix(const char * lc)
{ return !memcmp(lc, IK_prefix, 3) 
#ifdef ENGLISH
      || !memcmp(lc, "SE", 2)
#endif
; }

bool check_lcs(const char * RegistrationKey, const char * InstallationKey)
// Checks if the installation key and the registration key of the server (or other licences key) match
{ bool res;
 // special checking for SQX and SQY server registration
  if (!memcmp(RegistrationKey, "SQY", 3))
    return /*!strcmp(RegistrationKey+3, InstallationKey+3) ||*/ check_special_relation(InstallationKey, RegistrationKey)!=0;
  if (!xload_liclib()) return false;
  if (!memcmp(RegistrationKey, "SQX", 3))
  { res= /*!strcmp(RegistrationKey+3, InstallationKey+3) ||*/ check_special_relation(InstallationKey, RegistrationKey)!=0;
#ifdef STOP  // SQX: registration does not expire any more
    if (res)  // for Klinger-type licences check the expiration of IK:
    { int year, month, day;
      if (!verify_ik(InstallationKey, NULL, &year, &month, &day)) res=false;
      else
#ifndef INST  // INST is defined for cdsetup - functions Make_date and Today are not avaiable
      { uns32 reg_date = Make_date(day, month, year);  int diff;
        reg_date=dt_plus(reg_date, 30);
        diff = dt_diff(reg_date, Today());
        res = diff>=0;
      }
#else
      res = false;
#endif
    }
#endif
  }
  else  // normal registration key
  { int RemainingDays;  
    int ires = VerifyRKIK4 (RegistrationKey, InstallationKey, &RemainingDays);
    res = ires==0 || ires==11;  // expiration of trial ignored here!
  }
  unload_liclib();
  return res;
}

bool get_server_registration(const char * ik, char * srvreg, int * year, int * month, int * day)
// Checks if the server is registered
// If ik is specified, prefers returning srvreg related to the IK.
// Prefers returning normal registration over temp. reg.
{ int result_quality = 0;
  int year1, month1, day1;
//  if (year) { * year=2002; * month=1; * day=1; }  return true;
  char * buf  = (tptr)corealloc(MAX_BUFFER_SIZE, 77);
  char * buf2 = (tptr)corealloc(MAX_BUFFER_SIZE, 77);
  if (buf && buf2) 
  { if (get_licence_list(section_server_reg, buf, MAX_BUFFER_SIZE))
      if (get_licence_list(section_ik, buf2, MAX_BUFFER_SIZE))
      {// fast version: first search sergistration for the given ik, then for any ik:
        if (ik && *ik)
          for (char * server_lic=buf;  *server_lic;  server_lic += strlen(server_lic)+1)
          { if (check_lcs(server_lic, ik))
            { if (verify_ik(ik, NULL, &year1, &month1, &day1))
              { int quality = 3;
                if (!is_temp_server_reg(server_lic)) quality+=1;
                if (quality > result_quality)
                { if (year) *year=year1;  if (month) *month=month1;  if (day) *day=day1;
                  strcpy(srvreg, server_lic);
                  result_quality = quality;
                }
              }
            }
          }
        if (!result_quality)  //  good quality not found, look for poor:
          for (char * server_lic=buf;  *server_lic;  server_lic += strlen(server_lic)+1)
          { for (char * aik = buf2;  *aik;  aik += strlen(aik)+1)
            { if (check_lcs(server_lic, aik))
              { if (verify_ik(aik, NULL, &year1, &month1, &day1))
                { int quality = 1;
                  if (!is_temp_server_reg(server_lic)) quality+=1;
                  if (quality > result_quality)
                  { if (year) *year=year1;  if (month) *month=month1;  if (day) *day=day1;
                    strcpy(srvreg, server_lic);
                    result_quality = quality;
                    if (!year) goto stop;  // just checking the registration, no need to look for the better quality
                  }
                }
              }
            }
          }
      }
 stop:
    corefree(buf);  corefree(buf2);
  }
#if WBVERS>=810
  if (year && month && day)
    if (get_8081_upgrade(year1, month1, day1))
      if (year1>*year || year1==*year && (month1>*month || month1>*month && day1>*day))
        { *year=year1;  *month=month1;  *day=day1; }
#endif
  return result_quality > 0;
}

CFNC DllSpecial BOOL WINAPI check_server_registration(void)
#if WBVERS>=900
{ return TRUE; }   // no registration since 9.0
#else
#ifdef __ia64__  // temp. replacement
{ return TRUE; }
#else
{ char ServerLicenceNumber[MAX_LICENCE_LENGTH+1];
  return get_server_registration("", ServerLicenceNumber, NULL, NULL, NULL);
}
#endif
#endif

#ifdef SRVR
int get_client_lics_from_OPL_registration(const char * ik)
// Returns OPL licences stored in the OPL server registration linked to the given IK.
{ int opl_lic_cnt = 0;
  char * buf  = (tptr)corealloc(MAX_BUFFER_SIZE, 77);
  if (buf) 
  { if (get_licence_list(section_server_reg, buf, MAX_BUFFER_SIZE))
      for (char * server_lic=buf;  *server_lic;  server_lic += strlen(server_lic)+1)
        if (is_temp_server_reg(server_lic))
          if (check_lcs(server_lic, ik))
            { opl_lic_cnt=atoi(server_lic+14);  break; }
    corefree(buf);  
  }
  return opl_lic_cnt;
}
#endif

static int extract_licnum(const char * lc)
{ char buf[4];
  buf[0]=lc[14];  buf[1]=lc[15];  buf[2]=lc[16];  buf[3]=0;
  return atoi(buf);
}

#ifdef __ia64__  // temp. replacement
bool get_client_licences(char * ik, const char * prefix, int * lics)
{ *lics=10;  return true; }
#else
bool get_client_licences(char * ik, const char * prefix, int * lics)
// Must not verify the IK against the hardware! Used by the console for various IKs. 
// When server uses this, its IK is already verified.
{ int num, maxnum = 0;  int RemainingDays;
  bool result = false;
  if (!xload_liclib()) return false;
  int ires = VerifyIK (ik, &RemainingDays);
  if (!ires || ires==11)
  { char * buf  = (tptr)corealloc(MAX_BUFFER_SIZE, 77);
    if (buf) 
    { if (get_licence_list(section_client_access, buf, MAX_BUFFER_SIZE))
      { char * lc = buf;
        while (*lc)
        { if (prefix ? !strnicmp(prefix, lc, strlen(prefix)) : valid_addon_prefix(lc))
          { if (check_lcs(lc, ik))
            { num = extract_licnum(lc);
#ifdef ENGLISH  // specialni prani V.H.
              if (!strnicmp("SE1", lc, 3)) num+=5;
#endif
              if (num>maxnum) maxnum=num;
            }    
          }
          lc += strlen(lc)+1;
        }
        result=true;
      }
      corefree(buf);
    }
  }
  *lics=maxnum;
  unload_liclib();
  return true;
}
#endif

#if WBVERS<900
static const char hidden_ik_key[] = "CLSID\\{F2B21463-D307-11CE-BAB9-00A024384E7C}\\InProcServer32";
#else
static const char hidden_ik_key[] = "CLSID\\{F9B21463-D307-11CE-BAB9-00A024384E7C}\\InProcServer32";
#endif
static const char hidden_ik_val[] = "Data";
                           
CFNC DllSpecial BOOL WINAPI get_any_valid_ik(char * ik, const char * env_info)
{ char * buf = (tptr)corealloc(MAX_BUFFER_SIZE, 77);
  if (!buf) return false;
  bool result=false;
  if (get_licence_list(section_ik, buf, MAX_BUFFER_SIZE))
  { for (char * anik = buf;  *anik;  anik += strlen(anik)+1)
      if (verify_ik2(anik, env_info))
        { strcpy(ik, anik);  result=true;  break; }
    if (!result)
      for (char * anik = buf;  *anik;  anik += strlen(anik)+1)
        if (verify_ik(anik, env_info))
          { strcpy(ik, anik);  result=true;  break; }
  }
  corefree(buf);
  if (result) return true;
#ifdef WINS
 // look in the hidden place:
  HKEY hKey;  
  if (RegOpenKeyExA(HKEY_CLASSES_ROOT, hidden_ik_key, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  { DWORD key_len=MAX_LICENCE_LENGTH;
    if (RegQueryValueExA(hKey, hidden_ik_val, NULL, NULL, (BYTE*)ik, &key_len) == ERROR_SUCCESS)
      if (verify_ik(ik, env_info)) // may be from a different volume!!!
        result=true;
    RegCloseKey(hKey);
  }
  if (result) 
    return store_licence_number(section_ik, ik);  // must store it on the proper place!!!
#endif
 // create an IK if none found
  result=create_ik(ik, env_info);
#ifdef WINS
 // save in the hidden place:
  if (result) 
  { HKEY hKey;  
    if (RegCreateKeyExA(HKEY_CLASSES_ROOT, hidden_ik_key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
    { DWORD key_len=MAX_LICENCE_LENGTH;
      RegSetValueExA(hKey, hidden_ik_val, NULL, REG_BINARY, (BYTE*)ik, key_len);
      RegCloseKey(hKey);
    }
  }
#endif
  return result;
}

CFNC DllSpecial bool WINAPI insert_server_licence(const char * number)
{ return store_licence_number(section_server_reg, number); }

CFNC DllSpecial bool WINAPI drop_server_licence(const char * number)
{ return drop_licence_number(section_server_reg, number); }

//////////////////////////////////////////// 9.0 //////////////////////////////////////////////
/* Rules:
IK contains date -> must not create IK for the non-network server, it would start the TRIAL period.
*/
#if WBVERS>=900

bool store_server_licence(const char * number)
// Stores the given licence number
{
#ifdef WINS
  HKEY hKey;  char keyname[200];
  strcpy(keyname, WB_inst_key);  strcat(keyname, lic_key);  strcat(keyname, section_server_lic);
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyname, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
    return false;
  if (RegSetValueExA(hKey, section_server_lic, NULL, REG_SZ, (BYTE*)number, (int)strlen(number)+1) != ERROR_SUCCESS)
    { RegCloseKey(hKey);  return false; }
    RegCloseKey(hKey);
  return true;
#else
  bool res = WritePrivateProfileString(section_server_lic, section_server_lic, number, configfile) != FALSE;
#ifdef SRVR
  if (!res) dbg_err("Cannot write the configuration file");
#endif
  return res;
#endif
}

bool get_server_licence(char * number)
// Returns all licence numbers from a section
{ *number=0;
#ifdef WINS
  HKEY hKey;  char keyname[200];  
  strcpy(keyname, WB_inst_key);  strcat(keyname, lic_key);  strcat(keyname, section_server_lic);
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, keyname, 0, "", REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL) != ERROR_SUCCESS)
    return false;
  DWORD key_len = MAX_LICENCE_LENGTH+1;
  if (RegQueryValueExA(hKey, section_server_lic, NULL, NULL, (BYTE*)number, &key_len) != ERROR_SUCCESS)
    { RegCloseKey(hKey);  return false; }
  RegCloseKey(hKey);             
  return true;
#else
  GetPrivateProfileString(section_server_lic, section_server_lic, NULLSTRING, number, MAX_LICENCE_LENGTH+1, configfile);
  return true;
#endif
}

#define DEVELOPER_LICENCE(licnum) !memcmp(licnum+14, "000", 3)

int get_licence_information(const char * path, char * installation_key, char * ServerLicenceNumber, 
                            int * year, int * month, int * day)
{
  *ServerLicenceNumber=0;
  *installation_key=0;
 // find server licence number:
  get_server_licence(ServerLicenceNumber); // may return false when licence does not exist!
  if (!*ServerLicenceNumber) 
    return 0;  // non-network server
 // find a valid IK, create if it does not exist:
  if (!get_any_valid_ik(installation_key, path)) 
    return -3;
 // for TRIAL, get the installation date:
  if (!stricmp(ServerLicenceNumber, "TRIAL"))
  { verify_ik(installation_key, path, year, month, day);
    return 1;  // TRIAL, data returned
  }

 // otherwise verify the licence number agains any IK (multiple IKs may be necessary in MSW):
  bool found = false;
  char * buf  = (tptr)corealloc(MAX_BUFFER_SIZE, 77);
  if (!buf) return -1;
  if (!get_licence_list(section_ik, buf, MAX_BUFFER_SIZE))
    { corefree(buf);  return -2; }
  for (char * aik=buf;  *aik;  aik += strlen(aik)+1)
  { if (check_lcs(ServerLicenceNumber, aik))
    { //if (verify_ik(ik, NULL, &year1, &month1, &day1))
      found=true;  
      strcpy(installation_key, aik);  // must return the proper IK
      break;
    }
  }               
  corefree(buf);
  if (!found) return -4;
  return DEVELOPER_LICENCE(ServerLicenceNumber) ? 3 : 2;
}

#endif // 9.0
