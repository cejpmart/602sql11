#ifndef __WCS_H
#define __WCS_H

#define LICENCE_OK          0
#define BAD_PARAMETER       1
#define HTTP_LICENCE_ERROR  3 //kdyz server vrati HTTP_CONFLICT (409 RFC)
#define HTTP_ERROR          2
#define IO_ERROR            4
#define DLL_NEED_RELOAD     5
#define SECRET_STORE_F      6
#define STATUS_NOT_OK       7
#define BAD_AK_RESPONSE     9
#define EMPTY_IKNAME        10
#define ZERO_AKS            11
#define PROB_NOT_ACTIVATED  12
#define MISSING_AKRESP      13
#define SERVER_LIC_WARNING  14
#define DLL_RELOAD_FAIL     15
#define SET_NTFS_RIGHT_FAIL 16
#define LICENCE_DAMAGED     17
#define AK_DAMAGED          18
#define AKN_DAMAGED         19
#define DONT_WAIT_AKRESP    20
#define DLL_LOAD_FAIL       21
#define _LAST_ERROR         -1
#define STRNCPY_FAILED      1001

#ifdef WINS
 #define WINAPI __stdcall
 #define WCDLL               "wc.dll"
 #define WCSDLL              "wcs.dll"
#else
 #define WINAPI
 #define WCDLL               "libwc.so"
 #define WCSDLL              "libwcs.so"
#endif

#define LI_ORIG  1 //parametry licence_init a == regServer, b ==regApp


/**
 * Inicializace knihovny
 * Volat pri kazde session (soubor akci ohledne licenci)
 * @param softwareID id software, pro LNS je treba LS5CZ. Je treba zmenit pri upgradu softu,
 *  kdyz chceme, aby se za upgrade platilo.
 * @param licdir adresar, kam ukladat informace o licenci (uklada se jeste tajny soubor jinam).
 *  Pokud je null, vezme se adresar, kde lezi wc.dll
 * @param locale pro chyby, ve formatu "cs-CZ", "en-US". Tyhle dva jsou prozatim dostupne.
 * @param version zadavat LI_ORIG (nejcasteji)
 * @param a zadavat NULL, nebo viz LI_ORIG, ..
 * @param b zadavat NULL, nebo viz LI_ORIG, ..
 *
 * Vsechny funkce: Pokud navratova hodnota != LICENCE_OK, pak nastala chyba
*/
extern "C"
int WINAPI licence_init(void **lh, 
                   const char *softwareID,  
                   const char *licdir,
                   const char *locale, 
                   int version, 
                   const char *a, 
                   const char *b);
/**
 * Uvolneni knihovny - uvolni naalokovane retezce
 * Vsechny vracene retezce ve funkcich se nedealokuji, zajisti tato funkce.
 */
extern "C"
int WINAPI licence_close(void *licence_handle);  


/**
Protokol vypada:

1. IK1 je pipraveno skladem        prodej ->          uivatel zad�IK1 do nainstalovan�o SW
2.								<- http ([IK1], [null,GK1])     uivatel ��o aktivaci, 
																generuje GK1
3. 602Port� generuje AK,
   ukl��(IK1, GK1)				http(AK) ->		   SW se aktivuje (AK),
													   ukl��GK1
4. addon IK2 je pipraveno         prodej ->           uivatel zad�addon IK2 do nainstalovan�o SW
5. 602Portal zneplatn�(IK1, GK1)  
   generuje AK         
   ukl��(IK1, IK2, GK2)        <-http ([IK1, IK2],[GK1, GK2]) uivatel ��o aktivaci addon, generuje GK2 
6.								http(AK)->			   SW se aktivuje (AK),
 												       ukl��GK2
*/

/*krok 2 --BEGIN*/
/*Vytvori aktivacni pozadavek, ktery je potreba poslat pres http, nebo jinout cestou.
 Podpora offline pristupu. Bude se jednat o base64 et�ec.
 @param request [base64 retezec], dealokaci provede licence_close()
*/
extern "C"
int WINAPI generateActivationRequest(void *licence_handle, char** request);
/**
 Blokujici volani, ktere pouziva http a muze skoncit http chybami.
 Po uspesnem volani bude dostupna odpoved.
 @param request [base64 retezec] pozadavek, neni nutne delat neco s pameti. [base64 retezec]
 @param response [base64 retezec] odpoved neni nutne nic dealokovat, postara se licence_close()
 */
extern "C"
int WINAPI sendActivationRequest(void *licence_handle, char *request, char **response);
/*krok 2 --END*/

/*krok 3 -- BEGIN*/
/**
 * Na zaklade ulozeneho verejneho klice overi podpis,
   na zaklade ulozeneho tajneho GK desifruje,
   si ulozi aktivacni klic.

   //@todo muze obsahovat novou verzi knihovny ... zatim nedodelano,
   //ale bude to asi tak, ze navratova hodnota bude DLL_NEED_RELOAD,
   //a po znovunatazeni dll se opet zavola tato metoda.
 */
extern "C"
int WINAPI acceptActivationResponse(void **licence_handle, char *response);


/**
 * Ulozi instalacni klice (zakoupene pro aktivaci produktu)
 * @param ik pole C retezcu
 * @param len delka pole
 */
extern "C"
int WINAPI saveIKs(void *licence_handle, const char **iks, int len);

/**
 * Smaze vsechny IKs, aby se dalo prepisovat. SaveIKs vola v sobe clear...
 */
extern "C"
int WINAPI clearIKs(void *licence_handle);

/**
 * Ulozi jedno z IK. Je mozne volat opakovane v cyklu.
 */
extern "C"
int WINAPI saveOneIK(void *licence_handle, const char *ik);

/**
 * Ziska jeden ulozeny instalacni klic z pole z pozice pos
 * Cist dokud je navratova hodnota == 0.
 */
extern "C"
int WINAPI getOneIK(void *licence_handle, char **ik, int pos);


#define WCP_BETA          "beta"
#define WCP_COMPANY       "company"
#define WCP_INSTALL_NAME  "install_name"
#define WCP_INSTALL_DATE  "install_date"
#define WCP_LIBVER        "libver"
#define WCP_PROXY         "proxy"
#define WCP_VENDOR        "vendor"
#define WCP_WAIT_AKRESP   "wait_akresp"

/**
 * Protoze je potreba ukladat i dalsi vlastnosti, tak je zavedena metoda pro
 * cteni s parametrem nazev vlastnosti. Neni treba se starat o dealokaci.
 * Pouzijte preddef konstanty WCP_* 
 */
extern "C"
int WINAPI getProp(void *licence_handle, const char *prop_name, char **prop_value);

/*Uklada pouze WCP_COMPANY, WCP_INSTALL_NAME, WCP_PROXY, WCP_BETA */
extern "C"
int WINAPI saveProp(void *licence_handle, const char *prop_name, const char *prop_value);

/**
 * Ulozi informace o instalaci.
 * @param spolecnost,
 * @param nazev instalace - pojmenovani pocitace
 * OBSOLETE - Pouzijte saveProp/getProp(.,WCP_COMPANY/WCP_INSTALL_NAME,.)
 */
extern "C"
int WINAPI saveNames(void *licence_handle, const char *company, const char *installname);

/**
 * Cte udaje o instalaci. Navracene retezce neni treba dealokovat
 * OBSOLETE - Pouzijte saveProp/getProp(.,WCP_COMPANY/WCP_INSTALL_NAME,.)
 */
extern "C"
int WINAPI getNames(void *licence_handle, char **company, char **installname);

/**
 * Vrati datum instalace ve formatu 2007-12-31
 * OBSOLETE - Pouzijte getProp(.,WCP_INSTALL_DATE,.)
 */
extern "C"
int WINAPI getInstallDate(void *licence_handle, const char **installDate);

/**
 * Precte prvni vraceny aktivacni klic (licencni odpoved) a posune se na dalsi pozici.
 * Cist dokud return == 0. Aktivacni klic mohou byt binarni data, proto unsigned char.
 * Pokud akceptuje sw zpravy ve formatu "function=neco\nuser=100\n...", tak je lepsi pouzit
 * getLicenceSummedAK()
 */
extern "C"
int WINAPI getLicenceAK_move(void *licence_handle, unsigned char **ak, int *akLen);
/**
 * Rozumi formatu "function=neco\nuser=100\n...", umi scitat addony.
 * Po souctu vrati opet zpravu ve formatu function=neco\nuser=100\n..."
 * @param licence sectena licence, neni treba dealokovat
 * @param version zadavat 1
 */
extern "C"
int WINAPI getLicenceSummedAK(void *licence_handle, char **licence, int version);

/**
 * Vsechny funkce: Pokud navratova hodnota != LICENCE_OK, pak nastala chyba
 * Vrati textovou informaci o chybe. Widechar verze
 */ 
extern "C"
wchar_t * WINAPI WCGetLastErrorW(int errorCode, void *lh, const char *locale);
/**
 * Ansi verze.
 */
extern "C"
char * WINAPI WCGetLastErrorA(int errorCode, void *lh, const char *locale);

/**
 * Zavolat pri instalaci produktu - zalozi soubory s licencemi a nastavi jim prava.
 * Zapise datum instalace (kvuli trial)
 */
extern "C"
int WINAPI markInstallation(const char *softwareID, const char *licdir);

/**
 * Odstrani informace o licencich, krome datumu instalace
 */
extern "C"
int WINAPI clear(void *licence_handle);

/**
 * Podpora 602sql procedur.
 * Kopiruje src do dest.
 * O alokaci dest se stara volajici.
 * @return 0 nebo STRNCPY_FAILED pri chybe.
 */
extern "C"
int WINAPI WCstrncpy(char *dest, const int destLen, const char *src);

/**
 * Podpora 602sql procedur
 * Delka retezce
 */
extern "C" 
int WINAPI WCstrlen(const char *str);

#endif
