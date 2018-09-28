#ifdef ENGLISH  
#ifdef WINS
#if !OPEN602
#define LIBINTL_STATIC
#endif
#include <libintl-msw.h>
#else // UNIX
#include <libintl.h>
#endif
#define _(string) gettext(string)
#define gettext_noop(string) string
#endif

// messages transformaed for the gettext usage:
#define serverStartDelimiter  "============================================="
#ifdef ENGLISH
// server start and stop:
#define serverStartMsg        gettext_noop("Server %s v. %s started")
#define serverStartMsgService gettext_noop("Server %s v. %s started as a service")
#define serverStopMsg         gettext_noop("Server stopped (%s)")
#define DR_SYSTEM             gettext_noop("by system")
#define DR_CONSOLE            gettext_noop("from its console")
#define DR_ENDSESSION         gettext_noop("session or system termination")
#define DR_FROM_CLIENT        gettext_noop("by its client")
#define DR_POWER              gettext_noop("power down")
#define DR_ABORT              gettext_noop("abort")
#define serverStartFailed     gettext_noop("Starting server %s caused an error: %s")
#define server_term_error     gettext_noop("An error occured, terminating the server")
#define default_nw_interf     gettext_noop("No interface is enabled: Starting the network interface by default.")

#define unlimitedLics         gettext_noop("Unlimited")

#define TRACELOG_NOTFOUND     gettext_noop("Trace log %s not found")

// server messages and operator communication:
#define noAttached	      gettext_noop("No users are attached")
#define localClient       gettext_noop("Local client")
#define backupOK          gettext_noop("Database file backup created.")
#define backupError       gettext_noop("ERROR: Database file backup failed, error %u.")
#define backupVetoed      gettext_noop("Database file backup vetoed.")
#define warningClients    gettext_noop("Warning the clients...")
#define killingSlaves     gettext_noop("Logging the clients out...")
#define memoryExtended    gettext_noop("Out of working memory, alocating more.")  // not used any more
#define memoryNotExtended gettext_noop("Cannot allocate more working memory!")
#define SERVER_LOST_BYTES gettext_noop("Unreachable memory: %u")
#define LICS_CONSUMED     gettext_noop("Consumed %u licences from %u in the pool.")
#define msg_licences_margin gettext_noop("ATTENTION, %d licenses are used of %d available.")
#define killingDeadUser   gettext_noop("Terminating a client connection that has been idle for a long period of time")
#define terminatingListen gettext_noop("Listening for connection terminated")
#define TCPIPClientDead   gettext_noop("TCP/IP client not responding, closing the session")

#define Client_connection_failed  gettext_noop("Client connection failed.")
#define detachedThreadStart gettext_noop("Error starting detached thread: ")
#define clientThreadStart  gettext_noop("Error starting client thread: ")
#define workerThreadStart  gettext_noop("Error starting worker thread: ")
#define no_receive_memory gettext_noop("No memory for the new request.")
#define waitingForOfflineThread  gettext_noop("Waiting for the offline threads to terminate...")
#define msg_login_trace   gettext_noop("User name of the client %d, has changed from %s to %s (%s), %d licenses of %d used.")
#define msg_login_normal  gettext_noop("Login")
#define msg_login_logout  gettext_noop("Logout")
#define msg_login_network gettext_noop("Network auto-login")
#define msg_login_fast    gettext_noop("Auto-login from the same workstation")
#define msg_login_process gettext_noop("Auto-login from the same process")
#define msg_login_web     gettext_noop("Web client login")
#define msg_login_imperson gettext_noop("Domain auto-login")
#define msg_connect       gettext_noop("Client number %d connected.")
#define msg_connect_net   gettext_noop("Client number %d connected from address %s.")
#define msg_disconnect    gettext_noop("Client number %d disconnected.")
#define msg_send_error    gettext_noop("Sending packet to client failed")
#define msg_trace_read    gettext_noop("Reading value")
#define msg_trace_write   gettext_noop("Writing value")
#define msg_trace_insert  gettext_noop("Inserting a record")
#define msg_trace_delete  gettext_noop("Deleting a record")
#define msg_trace_sql     gettext_noop("Executing an SQL statement")
#define msg_cursor_open   gettext_noop("Cursor %d opened")
#define msg_cursor_close  gettext_noop("Cursor %d closed")
#define msg_integrity     gettext_noop("Database file integrity damaged")
#define msg_index_error   gettext_noop("Index integrity damaged")
#define wwwAccessEnabled  gettext_noop(" + WWW access")
#define damagedTabdef     gettext_noop("Damaged table definition: %s (%u)")
#define indexDamaged      gettext_noop("Damaged index %s of the table %s (%u)")
#define dataDamaged       gettext_noop("Data damaged in the table %s (%u), record %u")
#define msg_implicit_rollback  gettext_noop("Implicit RollBack: changes discarded due to error")
#define msg_condition_rollback gettext_noop("Implicit RollBack before executing a handler of a rollback condition")
#define msg_trigger_error gettext_noop("Trigger %s(%u) not compiled, error %u.")
#define msg_global_module_decls  gettext_noop("Error in global module declarations")
#define msg_defval_error  gettext_noop("Error in the default value of column %s in the table %s")
#define msg_check_constr_error  gettext_noop("Error in the check constraint %s in the table %s")
#define msg_trace_proc_call  gettext_noop("Calling SQL procedure %s")
#define msg_trace_dll_call   gettext_noop("Calling external procedure %s from %s library")
#define msg_trace_trig_exec  gettext_noop("Executing trigger %s")
#define trialLicences     gettext_noop("Trial client access licenses enabled for the next %d days.")
#define trialFTLicence9    gettext_noop("Trial fulltext and XML enabled for the next %d days.")
#define trialFTLicence8    gettext_noop("Trial fulltext license enabled for the next %d days.")
#define normalLicences    gettext_noop("%d+1 client access licenses are available for the nest of servers")
#define unlimitedLicences gettext_noop("Unlimited client access licenses are available for this nest of servers")
#define msgError          gettext_noop("Error")
#define badServerLic      gettext_noop("Invalid license number specified")
#define storingLic        gettext_noop("Cannot store the license, this may be caused by insufficient privileges.")
#define serverLicenceURL  "http://eshop.software602.cz/cgi-bin/wbcgi.exe/eshop/registrace/licence2.htw?prefix=SA1&ik=%s\n"
#define lowSpaceOnDisc    gettext_noop("WARNING: The volume containing the database file has less than 10 MB of free space.")
#define noUnicodeSupport  gettext_noop("WARNING: The system does not have full Unicode and ISO code page support.")
#define lic_state1  gettext_noop("Memory allocation error")
#define lic_state2  gettext_noop("Registry read error") 
#define lic_state3  gettext_noop("Registry write error")
#define lic_state4  gettext_noop("The server licence number is invalid")
#define lic_statex  gettext_noop("Licence error")
#define trial_valid  gettext_noop("Trial server license is valid for the next %d days.")
#define trial_expired  gettext_noop("Trial server license has expired.")
#define server_licnum  gettext_noop("Server license number is %s.")
#define developerLicence  gettext_noop("This license is valid FOR DEVELOPMENT ONLY!")
#define global_scope_init_error gettext_noop("Error %d when initializing the global scope")

// Linux messages
#define watchdog          gettext_noop("Unable to communicate with client, closing the session")
#define networkError      gettext_noop("Network error, closing the session")
#define downWarning       gettext_noop("Do you want to warn the clients, wait a minute, then shutdown the server? (y/n):")
/* servernw.c */
#define serverNamePresent gettext_noop("A 602SQL Server of the same name is already running on the network.")
#define networkInitError  gettext_noop("Network initialization error")
#if WBVERS>=1100
#define unloadingServer   gettext_noop("Unloading the 602SQL Open Server...")
#define winBaseQuit       gettext_noop("602SQL Open Server down\n")
#define winBaseAbort      gettext_noop("602SQL Open Server aborted\n")
#define initial_msg8      gettext_noop("602SQL Open Server %s, version %s (build %u)")
#define initial_msg9      gettext_noop("602SQL Open Server %s, version %s")
#define notUnloaded       gettext_noop("602SQL Open Server not stopped")
#else
#define unloadingServer   gettext_noop("Unloading the 602SQL Server...")
#define winBaseQuit       gettext_noop("602SQL Server down\n")
#define winBaseAbort      gettext_noop("602SQL Server aborted\n")
#define initial_msg8      gettext_noop("602SQL Server %s, version %s (build %u)")
#define initial_msg9      gettext_noop("602SQL Server %s, version %s")
#define notUnloaded       gettext_noop("602SQL Server not stopped")
#endif
#define loaded            gettext_noop("Loaded %s")
#define account           gettext_noop(",  Account: %s")
#define exitPrompt        gettext_noop("Press 'q' to exit, 'h' for help")
#define winBaseUsers      gettext_noop("\nNumber of logged in clients: ")
#define sureToExit        gettext_noop("Do you really want to stop the SQL server? (y/n):")
#define memorySize	      gettext_noop("Used memory (bytes): %u")
#define licenseInfo       gettext_noop("%u licenses used of %u available, this server uses %u licenses.")
#define linux_version_info	gettext_noop("%s v. %s\nCompiled %s on %s with %s.\n")
#define pressKey		    gettext_noop("<press any key to continue>")
#define user_log_on     gettext_noop("User error log started")
#define user_log_off    gettext_noop("User error log stopped")
#define trace_is_on     gettext_noop("Tracing started")
#define trace_is_off    gettext_noop("Tracing stopped")
#define replic_log_on   gettext_noop("Replication log started")
#define replic_log_off  gettext_noop("Replication log stopped")
#define stmt_info1      gettext_noop("Commands: U=list of users, I=Information on memory, L=Logging user errors,")
#define stmt_info2      gettext_noop("          T=Tracing events, R=Logging replication, Q=Quit")
#define noConversion    gettext_noop("\nConversion between UCS-2 and ISO646 is not available!")

// initial stderr messages:
#define dbNotSpecif       gettext_noop("%s: No database specified on the command line. Existing databases:\n")
#define pathNotDefined    gettext_noop("The database file directory for server %s is not defined\n")
#define isNotDir          gettext_noop("%s is not a directory!\n")
#define noDbInDir         gettext_noop("No database registered in the %s directory\n")
#define socketsNotActive  gettext_noop("Error creating socket, cannot start server\n")
#define nonOptionFound    gettext_noop("Unknown parameter: %s")
#define syntaxError       gettext_noop("\nSyntax error!")
#define serverCommLine    gettext_noop("\n%s <options>\n"\
                          "-n<name>     --dbname<name>  database name\n"\
			  "-f<dir>      --dbpath<dir>   full path to the database directory\n"\
			  "-p<password>                  password to the database\n"\
			  "-h           --help      list options\n"\
			  "-v           --version   version of the server\n"\
			  "-d run server as a deamon\n"\
			  "-t log network events\n"\
			  "-e log user errors\n"\
			  "-l write basic log to syslog\n"\
			  "-q disable system triggers\n"\
			  "-P print PID\n")
#define kernelInitError   gettext_noop("Server initialization error\n")

// console communication:
#define enterServerPassword  gettext_noop("Enter the password of the database file:")

#define cannotEditIni     gettext_noop("Server cannot read or write to /etc/602sql or create a file in /etc")
#define noRunLicence      "Server does not have its operational license. You can obtain it by e-mail after registering on the URL:\n" // WBVERS<900
#define enterRunLicence   "Store the license number to /etc/602sql into section [.ServerRegKey] on a separate line.\n"  // WBVERS<900

#define CantWriteDelRecs  gettext_noop("Record deletion cannot be written to table _REPLDELRECS because no unique index of table %s corresponds with a column of type UNIKEY")
#define msg_repetition    gettext_noop("Identical message repeated %u times")
///////////////////////////////////////// Windows only messages:
#if WBVERS>=1100
#define SERVER_CAPTION      gettext_noop("602SQL Open Server: %s")
#else
#define SERVER_CAPTION      gettext_noop("602SQL Server: %s")
#endif
#define CLOSE_WARNING       "Clients are connected to the server.\nDo you really want to close it?"  // WBVERS<900 
#define SERVER_WARNING_CAPT "Warning" // WBVERS<900 
#define NO_DATABASES_REG    gettext_noop("Local database not registered")

#define TASKBAR_MENU_ITEM_OPEN gettext_noop("&Open")
#define TASKBAR_MENU_ITEM_DOWN gettext_noop("&Stop server")
                                            

#else //////////////////////////////////////// national version ////////////////////////////////////////////

#define serverStartMsg        "Server %s start (task), v. %s"
#define serverStartMsgService "Server %s start (service), v. %s"
#define serverStopMsg         "Server ukon�en"
#define DR_SYSTEM             "syst�mem"
#define DR_CONSOLE            "z konzole"
#define DR_ENDSESSION         "konec session"
#define DR_FROM_CLIENT        "z klienta"
#define DR_POWER              "nap�jen�"
#define DR_ABORT              "p�d"
#define serverStartFailed     "P�i spou�t�n� serveru %s do�lo k chyb�: %s"
#define server_term_error     "Ukon�uji server kv�li v�n� chyb�"
#define default_nw_interf     "Nen� zapnuto ��dn� rozhran� serveru, proto zap�n�m s�ov� rozhran�."

#define unlimitedLics         "Neomezen�"

#define TRACELOG_NOTFOUND     "Log serveru %s nenalezen"

#define noAttached     "Nejsou p�ipojeni u�ivatel�"
#define localClient       "Lok�ln� klient"
#define Client_connection_failed  "Ne�sp�n� spojen� s nov�m klientem."
#define detachedThreadStart "Chyba p�i spou�t�n� samostatn�ho vl�kna: "
#define clientThreadStart   "Chyba p�i spou�t�n� vl�kna klienta: "
#define workerThreadStart   "Chyba p�i spou�t�n� pracovn�ho vl�kna: "
#define killingSlaves     "Odhla�uji klienty..."
#define no_receive_memory "Nelze p�ijmout po�adavek, do�la pam�."
#define memoryExtended    "Vy�erp�na pracovn� pam�, alokuji dal��."
#define memoryNotExtended "Nelze alokovat dal�� pracovn� pam�!"
#define waitingForOfflineThread  "�ek�m na ukon�en� pomocn�ch vl�ken..."
#define msg_login_trace   "Zm�na u�ivatelsk�ho jm�na klienta %d z %s na %s (%s), %d licenc� z %d vyu�ito."
#define msg_login_normal  "Login"
#define msg_login_logout  "Logout"
#define msg_login_network "S�ov� autologin"
#define msg_login_fast    "Autologin ze stejn� stanice"
#define msg_login_process "Autologin ze stejn�ho procesu"
#define msg_login_web     "Login Web klienta"
#define msg_login_imperson "Dom�nov� autologin"
#define msg_connect       "P�ipojil se klient ��slo %d."
#define msg_connect_net   "P�ipojil se klient ��slo %d z adresy %s."
#define msg_disconnect    "Odpojil se klient ��slo %d."
#define msg_send_error    "Chyba p�i odes�l�n� paketu klientovi"
#define msg_trace_read    "�tu hodnotu"
#define msg_trace_write   "Zapisuji hodnotu"
#define msg_trace_insert  "Vkl�d�m z�znam"
#define msg_trace_delete  "Ru��m z�znam"
#define msg_trace_sql     "Prov�d�m SQL p��kaz"
#define msg_cursor_open   "Kurzor %d otev�en"
#define msg_cursor_close  "Kurzor %d uzav�en"
#define msg_integrity     "Poru�ena integrita datab�zov�ho souboru"
#define msg_index_error   "Poru�ena integrita index�"
#define backupOK          "Vytvo�ena z�lo�n� kopie datab�zov�ho souboru."
#define backupError       "CHYBA %u: Nelze vytvo�it z�lo�n� kopii datab�zov�ho souboru."
#define backupVetoed      "Vytvo�en� z�lo�n� kopie datab�zov�ho souboru odvol�no."
#define killingDeadUser   "Odpojuji klienta, kter� nekomunikuje po dlouhou dobu"
#define wwwAccessEnabled  " + WWW p��stup"
#define terminatingListen "Kon��m p��jem p�ihl�en� klient�"
#define damagedTabdef     "Po�kozena definice tabulky %s (%u)"
#define indexDamaged      "Po�kozen index %s k tabulce %s (%u)"
#define dataDamaged       "Po�kozena data v tabulce %s (%u), z�znam %u"
#define msg_implicit_rollback  "Implicitn� rollback: zm�ny odvol�ny kv�li chyb�"
#define msg_condition_rollback "Implicit RollBack before executing a handler of a rollback condition"
#define msg_trigger_error "Trigger %s(%u) nelze p�elo�it, chyba %u."
#define msg_global_module_decls  "Chyba v glob�ln�ch deklarac�ch modulu"
#define msg_defval_error  "Chyba v implicitn� hodnot� sloupce %s v tabulce %s"
#define msg_check_constr_error  "Chyba v integritn�m omezen� %s v tabulce %s"
#define msg_trace_proc_call  "Vol�m SQL proceduru %s"
#define msg_trace_dll_call   "Vol�m extern� proceduru %s z knihovny %s"
#define msg_trace_trig_exec  "Spou�t�m trigger %s"
#define trialLicences     "5 testovac�ch klientsk�ch licenc� je k dispozici je�t� %d dn�."
#define trialFTLicence9    "Testovac� fulltextov� a XML licence je k dispozici je�t� %d dn�."
#define trialFTLicence8    "Testovac� fulltextov� licence je k dispozici je�t� %d dn�."
#define normalLicences    "%d+1 klientsk�ch licenc� je k dispozici pro toto hn�zdo server�"
#define unlimitedLicences "Neomezen� po�et klientsk�ch licenc� je k dispozici pro toto hn�zdo server�"
#define msgError          "Chyba"
#define badServerLic      "Nen� zad�na platn� licence"
#define storingLic        "Nelze zaznamenat licenci, z�ejm� nedostatek p��stupov�ch pr�v."
#define serverLicenceURL  "http://eshop.software602.cz/cgi-bin/wbcgi.exe/eshop/registrace/licence2.htw?prefix=SA1&ik=%s\n"
#define msg_licences_margin "POZOR, je ji� vy�erp�no %d licenc� z %d mo�n�ch."
#define lowSpaceOnDisc    "Varov�n�: Diskov� svazek obsahuj�c� datab�zov� soubor m� m�n� ne� 10 MB voln�ho prostoru."
#define noUnicodeSupport  "Varov�n�: Opera�n� syst�m nepodporuje pln� Unicode ani k�dov� str�nky ISO."
#define lic_state1  "Chyba pri alokaci pameti"       // no diacritics!
#define lic_state2  "Chyba pri cteni z registru"     // no diacritics!
#define lic_state3  "Chyba pri zapisu do registru"   // no diacritics!
#define lic_state4  "Licencni cislo serveru je neplatne"  // no diacritics!
#define lic_statex  "Chyba v licencovani"            // no diacritics!
#define trial_valid  "Zku�ebn� licence serveru plat� je�t� %d dn�."
#define trial_expired  "Zku�ebn� licence serveru ji� vypr�ela."
#define server_licnum  "Licen�n� ��slo serveru je %s."
#define developerLicence  "Tento server m� licenci POUZE PRO V�VOJ!"
#define warningClients    "Varuji klienty..."
#define global_scope_init_error "Chyba %d p�i inicializaci glob�ln�ho scope."

// Linux messages
#define watchdog          "Nemohu komunikovat s klientem, uzav�r�m relaci"
#define networkError      "Chybov� stav v s�ti, uzav�r�m relaci"
#define unloadingServer   "Ukon�uji 602SQL Server..."
#define downWarning       "Chcete varovat klienty a minutu po�kat? (a/n):"
#define TCPIPClientDead   "TCP/IP klient nereaguje, uzav�r�m relaci"
/* servernw.c */
#define serverNamePresent "V s�ti je ji� spu�t�n server stejn�ho jm�na."
#define networkInitError  "Chyba pri inicializaci s�e"
#define winBaseQuit       "602SQL Server ukon�en\n"
#define winBaseAbort   	  "602SQL Server abnorm�ln� ukon�en\n"
#define initial_msg8      "602SQL server %s, verze %s (build %u)"
#define initial_msg9      "602SQL server %s, verze %s"
#define loaded            "Spu�t�n %s"
#define account           ",  ��et: %s"
#define exitPrompt        "K ukon�en� stiskn�te 'q', help 'h'"
#define winBaseUsers      "\nPo�et p�ihl�en�ch klient�: "
#define sureToExit        "Opravdu chcete skon�it? (a/n):"
#define notUnloaded       "602SQL server nen� ukon�en"
#define memorySize	       "Po�et bajt� vyu�it� pam�ti 602SQL: %u"
#define licenseInfo       "Vyu�ito %u licenc� z %u dostupn�ch, tento server vyu��v� %u z nich."
#define linux_version_info "%s verze %s\nKompilov�no %s v %s s %s.\n"
#define pressKey		     "<stiskn�te kl�vesu>"
#define user_log_on      "Zapnut log u�ivatelsk�ch chyb"
#define user_log_off     "Vypnut log u�ivatelsk�ch chyb"
#define trace_is_on      "Zapnuto trasov�n� ud�lost�"
#define trace_is_off     "Vypnuto trasov�n� ud�lost�"
#define replic_log_on    "Zapnut log replikac�"
#define replic_log_off   "Vypnut log replikac�"
#define stmt_info1       "P��kazy: U=v�pis u�ivatel�, I=informace o pam�ti, L=logov�n� u�ivatelsk�ch chyb"
#define stmt_info2       "         T=trasov�n� ud�lost�, R=logov�n� replikac�, Q=Konec"
#define noConversion     "\nNen� dostupn� konverze mezi UCS-2 a ISO646!"

// initial stderr messages:
#define dbNotSpecif       "%s: Nen� uveden parametr specifikuj�c� datab�zi. Existuj� datab�ze:\n"
#define socketsNotActive  "Chyba vytvo�en� socketu, nelze spustit server\n"
#define pathNotDefined    "Adres�� datab�zov�ho souboru pro server %s nen� definov�n\n"
#define isNotDir          "%s nen� adres��!\n"
#define noDbInDir         "V adres��i %s nen� registrov�na ��dn� datab�ze\n"
#define nonOptionFound    "Nezn�m� parametr: %s"
#define syntaxError       "\nChyba syntaxe!"
#define serverCommLine    "\n%s <options>\n"\
        "-n<name>     --dbname<name>  jm�no datab�ze\n"\
			  "-f<dir>      --dbpath<dir>   cesta k adres��i s datab�z�\n"\
			  "-p<password>                 heslo k datab�zi\n"\
			  "-h           --help          vypsat seznam parametr�\n"\
			  "-v           --version       vypsat ��slo verze serveru\n"\
			  "-d spustit server jako deamona\n"\
			  "-t logovat s�\n"\
			  "-e logovat u�ivatelsk� chyby\n"\
			  "-r logovat replikace\n"\
			  "-l psat z�kladn� log do syslogu\n"\
			  "-q vypnout syst�mov� triggery\n"\
			  "-P vypsat PID\n"
#define kernelInitError   "Chyba pri inicializaci serveru\n"

// console communication:
#define enterServerPassword  "Zadejte heslo serveru:"

#define cannotEditIni     "Server nem� pr�vo ��st nebo p�epsat /etc/602sql nebo vytvo�it soubor v /etc"
#define noRunLicence      "Server dosud nem� provozn� licenci. Obdr��te ji po�tou po registraci na adrese:\n"
#define enterRunLicence   "Licen�n� ��slo pot� zapi�te do /etc/602sql do sekce [.ServerRegKey] na samostatn� ��dek a ukon�ete rovn�tkem.\n"

#define CantWriteDelRecs  "Informace o smaz�n� z�znamu nemohla b�t zaps�na do tabulky _REPLDELRECS, proto�e ��dn� z unik�tn�ch index� tabulky %s neodpov�d� typu sloupce UNIKEY"
#define msg_repetition    "Identick� zpr�va se opakuje %u-kr�t"

///////////////////////////////////////// Windows only messages:
#define SERVER_CAPTION      "602SQL Server: %s"
#define CLOSE_WARNING       "Na server jsou p�ipojeni u�ivatel�.\nP�ejete si p�esto skon�it?"
#define SERVER_WARNING_CAPT "Varov�n�"
#define NO_DATABASES_REG    "Nen� registrov�na ��dn� lok�ln� datab�ze"
#define SERVER_LOST_BYTES   "Nedosa�iteln� pam�: %u"
#define LICS_CONSUMED       "Spot�ebov�no %u licenc� z %u dostupn�ch."

#define TASKBAR_MENU_ITEM_OPEN "&Otev��t"
#define TASKBAR_MENU_ITEM_DOWN "&Ukon�it server"

#endif  // end of national version

