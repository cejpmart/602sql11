/*wrapper knihovna pro dynamicke pouziti wc.dll*/
#include <stdio.h>
#include <string>
#include "wcs.h"

#ifdef WINS
 #include <windows.h>
 #define PATH_SEP "\\"
#else
 #include <dlfcn.h>
 #include <unistd.h>
 #define FARPROC void*
 #define HINSTANCE void* 
 #define HMODULE void*
 #define PATH_SEP "/"
#endif

#define PATHSZ              1024

//Pri dynamicke reloadovani je nutne chranit pointry na funkce,
//a to v pripade licence_init, licence_close - to se meni pocet uzivatelu knihovny
//               acceptLicence - to se muze reloadnout knihovna, to pouze v pripade,
//                               kdy je pocet threadsUsed==0

class LibInfo {
public:
      FARPROC f_licence_init;
      FARPROC f_licence_close;
      FARPROC f_acceptActivationResponse;
      FARPROC f_generateActivationRequest;
      FARPROC f_sendActivationRequest;
      FARPROC f_saveIKs;
      FARPROC f_clearIKs;
      FARPROC f_saveOneIK;
      FARPROC f_getOneIK;
      FARPROC f_saveNames;
      FARPROC f_getNames;
      FARPROC f_getProp;
      FARPROC f_saveProp;
      FARPROC f_getLicenceAK_move;
      FARPROC f_getLicenceSummedAK;
      FARPROC f_WCGetLastErrorW;
      FARPROC f_WCGetLastErrorA;      
      FARPROC f_getLSInitInfo;
      FARPROC f_markInstallation;
      FARPROC f_getInstallDate;
      FARPROC f_clear;

    LibInfo();
    ~LibInfo();

    //protected variables
    HINSTANCE lib;
#ifdef WINS
    HANDLE mutex;
#endif 
    int threadsUsed;

    //unprotected variable
    int reloadFailCount;

    int reload_library();    
private:
    void load_library();    
};

//globalni, pro linux
std::string _licdir="";

#ifdef WINS
std::string getDllDir(const char *dll) {
    #define dllname_SZ 512
    char dllname[dllname_SZ];
	HINSTANCE hModule =  GetModuleHandleA(dll);	
	DWORD err = GetModuleFileNameA( hModule, dllname, dllname_SZ);
    if (!err) {
        //error
        return "";
    }
    int i;
    for (i=(int)strlen(dllname); i>0 && dllname[i]!='\\' && dllname[i]!='/' ; i--);
    return std::string(dllname,i);
}
#else //LINUX
std::string getDllDir(const char *dll) {
   //pod linuxem musi dll lezet v licdir adresari
   return _licdir;
}
#endif


FARPROC WCGetProcAddress(HMODULE mod, const char* name) {
    if (mod==NULL) return NULL;
    #ifdef WINS
     return GetProcAddress(mod, name);
    #else
     return dlsym(mod, name);
    #endif
}


void LibInfo::load_library() {
    std::string path=getDllDir(WCSDLL)+PATH_SEP+WCDLL;
#ifdef WINS
    lib=LoadLibraryExA(path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
    const char * errtxt;
    lib=dlopen(path.c_str(), RTLD_LAZY);
    if (!lib)
      errtxt=dlerror();
#endif    
    //22.8.07 pod linuxem problem, ze se fce z wc.dll jmenovaly stejne 
    //jako ve wcs.dll, proto dostaly prefix wc_
    f_licence_init            =WCGetProcAddress(lib, "wc_licence_init");
    f_licence_close           =WCGetProcAddress(lib, "wc_licence_close");
    f_acceptActivationResponse=WCGetProcAddress(lib, "wc_acceptActivationResponse");
    f_generateActivationRequest=WCGetProcAddress(lib,"wc_generateActivationRequest");
    f_sendActivationRequest   =WCGetProcAddress(lib, "wc_sendActivationRequest");
    f_saveIKs                 =WCGetProcAddress(lib, "wc_saveIKs");
    f_clearIKs                =WCGetProcAddress(lib, "wc_clearIKs");
    f_saveOneIK               =WCGetProcAddress(lib, "wc_saveOneIK"); 
    f_getOneIK                =WCGetProcAddress(lib, "wc_getOneIK");
    f_saveNames               =WCGetProcAddress(lib, "wc_saveNames");
    f_getNames                =WCGetProcAddress(lib, "wc_getNames");
    f_getProp                 =WCGetProcAddress(lib, "wc_getProp");
    f_saveProp                =WCGetProcAddress(lib, "wc_saveProp");
    f_getLicenceAK_move       =WCGetProcAddress(lib, "wc_getLicenceAK_move");
    f_getLicenceSummedAK      =WCGetProcAddress(lib, "wc_getLicenceSummedAK");
    f_WCGetLastErrorW         =WCGetProcAddress(lib, "wc_WCGetLastErrorW");
    f_WCGetLastErrorA         =WCGetProcAddress(lib, "wc_WCGetLastErrorA");    
    f_getLSInitInfo           =WCGetProcAddress(lib, "wc_getLSInitInfo");
    f_markInstallation        =WCGetProcAddress(lib, "wc_markInstallation");
    f_getInstallDate          =WCGetProcAddress(lib, "wc_getInstallDate");
    f_clear                   =WCGetProcAddress(lib, "wc_clear");    
}

LibInfo::LibInfo() {
    threadsUsed=0;
    reloadFailCount=0;
#ifdef WINS
    mutex = CreateMutex(NULL, FALSE, NULL);
#endif
    load_library();
}

LibInfo::~LibInfo() {
#ifdef WINS
    FreeLibrary(lib);
    CloseHandle(mutex);
#else //LINUX
    dlclose(lib);
#endif
}


//vlastni fce pro kopirovani souboru
int copy(const char *src, const char *dest) {
   FILE *fsrc = fopen(src,  "rb"),
         *fdest= fopen(dest, "wb");
   size_t nread, nwrite;
   if (!fsrc || fdest) return DLL_RELOAD_FAIL;
   #define BSZ 4096
   unsigned char buf[BSZ];
   for (;;) {
     nread=fread(buf, sizeof(unsigned char), BSZ, fsrc);
     if (nread<=0) break;
     nwrite=fwrite(buf, sizeof(unsigned char), BSZ, fdest);
     if (nwrite<=0) return DLL_RELOAD_FAIL;
   }
   return LICENCE_OK;
}

int LibInfo::reload_library() {
 int err=LICENCE_OK,
     nTries=0;
     while (nTries<500) {
       #ifdef WINS
         WaitForSingleObject(mutex, INFINITE);
       #endif 
         if (threadsUsed==0) break; 
         else {
            #ifdef WINS
	      ReleaseMutex(mutex);
	    #else
	      sleep(1000); //aspon chvilku spi
	    #endif
	 }
         nTries++;
     }
     if (nTries==500 && threadsUsed!=0) return DLL_RELOAD_FAIL;

     //je mozne reloadnout knihovnu
     //0.zjisti jmena knihoven
     std::string wclib = getDllDir(WCSDLL)+PATH_SEP+WCDLL;

     //1.unload
     #ifdef WINS
      FreeLibrary(lib);
     #else
      dlclose(lib);
     #endif
     //2.copy (bylo rename, kvuli pravum je bezpecnejsi copy - aby se zachovala prava wclib, wclib+"o", wclib+"n")
     err=copy(wclib.c_str(), (wclib+"o").c_str());
     if (err!=LICENCE_OK) return DLL_RELOAD_FAIL;
     err=copy((wclib+"n").c_str(), wclib.c_str());
     if (err!=LICENCE_OK) return DLL_RELOAD_FAIL;
     //3.load
     load_library();
     //4.success, release mutex
     #ifdef WINS
      ReleaseMutex(mutex);
     #endif
     return LICENCE_OK;
}

//musel jsem udelat dynamicky, nelze v DllMain volat LoadLibrary - v prostredi
//MSVC 2005
static LibInfo *p_libinfo=NULL;
LibInfo& get_libinfo() {
    if (p_libinfo==NULL) p_libinfo=new LibInfo();
    return *p_libinfo;
}

typedef void t_mark_callback(const void * ptr);

void mark_wcs(t_mark_callback * mark_callback)
{
  (*mark_callback)(p_libinfo);
  (*mark_callback)(_licdir.c_str());
}

//nikdy zatim nepouziji - nechce se mi pri kazdem licence_close() odloadovavat dll
//necham to na ukonceni procesu.
void release_libinfo() {
    if (p_libinfo) delete p_libinfo;
}
#define libinfo get_libinfo()

// -----------  JEDNOTLIVE FUNKCE --------------

typedef int (WINAPI *F_licence_init)(void **lh, 
                   const char *softwareID, 
                   const char *licdir,
                   const char *locale, 
                   int version, 
                   const char *a, 
                   const char *b);
int WINAPI licence_init(void **lh, 
                   const char *softwareID, 
                   const char *licdir,
                   const char *locale, 
                   int version, 
                   const char *a, 
                   const char *b) {
      //pro linux
      _licdir=licdir;
#ifdef WINS
      WaitForSingleObject(libinfo.mutex, INFINITE);
      libinfo.threadsUsed++;
      ReleaseMutex(libinfo.mutex);
#else
      //@todo linux zamykat?
#endif
      if (!libinfo.f_licence_init) { *lh=NULL;  return DLL_RELOAD_FAIL; }
      return ((F_licence_init)libinfo.f_licence_init)
             (lh, softwareID, licdir, locale, version, a, b);
}

typedef int (WINAPI *F_licence_close)(void *licence_handle);
int WINAPI licence_close(void *licence_handle) {    
#ifdef WINS
      WaitForSingleObject(libinfo.mutex, INFINITE);
      libinfo.threadsUsed--;
      ReleaseMutex(libinfo.mutex);
#else
      //@todo linux zamykat?
#endif
      if (!libinfo.f_licence_close) return DLL_RELOAD_FAIL;
      return ((F_licence_close)libinfo.f_licence_close)(licence_handle);
}

typedef struct {
    const char  *softwareID,
                *licdir,
                *locale,
                *regServer,
                *regApp;
    int version;
  } LSInitInfo;
typedef int (WINAPI *F_getLSInitInfo)(void *licence_handle, LSInitInfo *lsi);
void restr(char **str) {
    char *r = new char[strlen(*str)+1];
    strcpy(r, *str);
    *str = r;
}
typedef int (WINAPI *F_acceptActivationResponse)(void *licence_handle, char *response, int ignoreLIB);

//pri reloadu knihovny muze vytvorit novy licence_handle
int WINAPI acceptActivationResponse(void **licence_handle, char *response) {
    if (!libinfo.f_acceptActivationResponse) return DLL_LOAD_FAIL;
    int err=((F_acceptActivationResponse)libinfo.f_acceptActivationResponse)
            (*licence_handle,response,0);

    if (err==DLL_NEED_RELOAD) {
        if (libinfo.reloadFailCount>3) return DLL_RELOAD_FAIL;
        //+unload knihovny zneplatni response, licence_handle
        //alokace
        char *responseDUP = new char[strlen(response)+1];
        strcpy(responseDUP, response);

        LSInitInfo lsi;
        err=((F_getLSInitInfo)libinfo.f_getLSInitInfo)(*licence_handle, &lsi);
        if (err!=LICENCE_OK) return err;

        //je treba prealokovat jednotlive stringy
        restr((char**)&lsi.softwareID);
        restr((char**)&lsi.locale);
        restr((char**)&lsi.regServer);
        restr((char**)&lsi.regApp);

        licence_close(*licence_handle);
 
        //-unload knihovny, priprava konec

        //muze blokovat dlouho - ceka nez vsichni ostatni opusti knihovnu
        err=libinfo.reload_library();
        if (err==DLL_RELOAD_FAIL) {
            libinfo.reloadFailCount++;
        }
        else    {
            libinfo.reloadFailCount=0;
            //posledni parametr 1 znamena ignoruj aktualizaci knihovny
            void *new_licence_handle;

            licence_init(&new_licence_handle, lsi.softwareID, lsi.licdir, lsi.locale, 
                lsi.version, lsi.regServer, lsi.regApp);
            *licence_handle = new_licence_handle;

            err=((F_acceptActivationResponse)libinfo.f_acceptActivationResponse)
                (*licence_handle,responseDUP,1);
        }

        //dealokace
            delete responseDUP;
            delete lsi.softwareID;
            delete lsi.locale;
            delete lsi.regServer;
            delete lsi.regApp;
        //-dealokace konec
    }
    return err;
}

/// --- KONEC VYZNACNYCH FCI ---- ///

typedef int (WINAPI *F_generateActivationRequest)(void *licence_handle, char** request);
int WINAPI generateActivationRequest(void *licence_handle, char** request) {
    if (!libinfo.f_generateActivationRequest) return DLL_LOAD_FAIL;
    return ((F_generateActivationRequest)libinfo.f_generateActivationRequest)
            (licence_handle, request);
}


typedef int (WINAPI *F_sendActivationRequest)(void *licence_handle, char *request, char **response);
int WINAPI sendActivationRequest(void *licence_handle, char *request, char **response) {
    if (!libinfo.f_sendActivationRequest) return DLL_LOAD_FAIL;
    return ((F_sendActivationRequest)libinfo.f_sendActivationRequest)
           (licence_handle, request, response);
}

typedef int (WINAPI *F_saveIKs)(void *licence_handle, const char **iks, int len);
int WINAPI saveIKs(void *licence_handle, const char **iks, int len) {
    if (!libinfo.f_saveIKs) return DLL_LOAD_FAIL;
    return ((F_saveIKs)libinfo.f_saveIKs)
           (licence_handle, iks, len);
}

typedef int (WINAPI *F_clearIKs)(void *licence_handle);
int WINAPI clearIKs(void *licence_handle) {
    if (!libinfo.f_clearIKs) return DLL_LOAD_FAIL;
    return ((F_clearIKs)libinfo.f_clearIKs)
           (licence_handle);
}

typedef int (WINAPI *F_saveOneIK)(void *licence_handle, const char *ik);
int WINAPI saveOneIK(void *licence_handle, const char *ik) {
    if (!libinfo.f_saveOneIK) return DLL_LOAD_FAIL;
    return ((F_saveOneIK)libinfo.f_saveOneIK)
           (licence_handle, ik);
}

typedef int (WINAPI *F_getOneIK)(void *licence_handle, char **ik, int pos);
int WINAPI getOneIK(void *licence_handle, char **ik, int pos) {
    if (!libinfo.f_getOneIK) return DLL_LOAD_FAIL;
    return ((F_getOneIK)libinfo.f_getOneIK)
           (licence_handle, ik, pos);
}

typedef int (WINAPI *F_saveNames)(void *licence_handle, const char *company, const char *installname);
int WINAPI saveNames(void *licence_handle, const char *company, const char *installname) {
    if (!libinfo.f_saveNames) return DLL_LOAD_FAIL;
    return ((F_saveNames)libinfo.f_saveNames)
           (licence_handle, company, installname);
}

typedef int (WINAPI *F_getNames)(void *licence_handle, char **company, char **installname);
int WINAPI getNames(void *licence_handle, char **company, char **installname) {
    if (!libinfo.f_getNames) return DLL_LOAD_FAIL;
    return ((F_getNames)libinfo.f_getNames)
           (licence_handle, company, installname);
}

typedef int (WINAPI *F_getProp)(void *licence_handle, const char *prop_name, char **prop_value);
int WINAPI getProp(void *licence_handle, const char *prop_name, char **prop_value) {
    if (!libinfo.f_getProp) return DLL_LOAD_FAIL;
    return ((F_getProp)libinfo.f_getProp)
           (licence_handle, prop_name, prop_value);
}

typedef int (WINAPI *F_saveProp)(void *licence_handle, const char *prop_name, const char *prop_value);
int WINAPI saveProp(void *licence_handle, const char *prop_name, const char *prop_value) {
    if (!libinfo.f_saveProp) return DLL_LOAD_FAIL;
    return ((F_saveProp)libinfo.f_saveProp)
           (licence_handle, prop_name, prop_value);
} 


typedef int (WINAPI *F_getLicenceAK_move)(void *licence_handle, unsigned char **ak, int *akLen);
int WINAPI getLicenceAK_move(void *licence_handle, unsigned char **ak, int *akLen) {
    if (!libinfo.f_getLicenceAK_move) return DLL_LOAD_FAIL;
    return ((F_getLicenceAK_move)libinfo.f_getLicenceAK_move)
           (licence_handle, ak, akLen);
}

typedef int (WINAPI *F_getLicenceSummedAK)(void *licence_handle, char **licence, int version);
int WINAPI getLicenceSummedAK(void *licence_handle, char **licence, int version) {
    if (!libinfo.f_getLicenceSummedAK) return DLL_LOAD_FAIL;
    return ((F_getLicenceSummedAK)libinfo.f_getLicenceSummedAK)
           (licence_handle, licence, version);
}

typedef const wchar_t* (WINAPI *F_WCGetLastErrorW)(int errorCode, void *lh, const char *locale);
wchar_t* WINAPI WCGetLastErrorW(int errorCode, void *lh, const char *locale) {
    if (!libinfo.f_WCGetLastErrorW) return L"";
    return (wchar_t*)((F_WCGetLastErrorW)libinfo.f_WCGetLastErrorW)
           (errorCode, lh, locale);
}

typedef const char* (WINAPI *F_WCGetLastErrorA)(int errorCode, void *lh, const char *locale);
char* WINAPI WCGetLastErrorA(int errorCode, void *lh, const char *locale) {
    if (!libinfo.f_WCGetLastErrorA) return "";
    return (char*)((F_WCGetLastErrorA)libinfo.f_WCGetLastErrorA)
           (errorCode, lh, locale);
}

typedef int (WINAPI *F_markInstallation)(const char *softwareID, const char *licdir);
 int WINAPI markInstallation(const char *softwareID, const char *licdir) {
    //pro linux
    _licdir=licdir;
    if (!libinfo.f_markInstallation) return DLL_LOAD_FAIL;
    return ((F_markInstallation)libinfo.f_markInstallation)(softwareID, licdir);   
}

typedef int (WINAPI *F_getInstallDate)(void *licence_handle, const char **installDate);
int WINAPI getInstallDate(void *licence_handle, const char **installDate) {
    if (!libinfo.f_getInstallDate) return DLL_LOAD_FAIL;
    return ((F_getInstallDate)libinfo.f_getInstallDate)(licence_handle, installDate);
}

typedef int (WINAPI *F_clear)(void *licence_handle);
int WINAPI clear(void *licence_handle) {
    if (!libinfo.f_clear) DLL_LOAD_FAIL;
    return ((F_clear)libinfo.f_clear)(licence_handle);
}

#define MIN(a,b) (a) < (b) ? (a) : (b)
int WINAPI WCstrncpy(char *dest, const int destLen, const char *src) {
    if (destLen<0 || dest==NULL || src==NULL) return STRNCPY_FAILED;
    int len=MIN(destLen-1, (int)strlen(src));
    memcpy(dest, src,  len*sizeof(char));
    dest[len+1]=0x0;
    return LICENCE_OK;
}

int WINAPI WCstrlen(const char *str) {
    if (!str) return 0;
    return (int)strlen(str);
}
