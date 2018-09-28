/****************************************************************************/
/* CSMSGS.CPP - Common client and server messages                           */
/****************************************************************************/
const int warning_messages[] = {
  WAS_IN_TRANS, NOT_IN_TRANS, ERROR_IN_CONSTRS, IS_NOT_DEL, ERROR_IN_DEFVAL, WORKING_WITH_DELETED_RECORD, IS_DEL, 
  NO_BIPTR, INDEX_OOR, WORKING_WITH_EMPTY_RECORD,
  0 }; // terminated by 0


#if !defined(ENGLISH) && ((WBVERS<900) || defined(SRVR))

const char * db_errors[] = {
     "�patn� modifik�tor"
   , "Nem�te pr�vo prov�st tuto akci"
   , "�patn� ��slo sloupce"
   , "Z�znam mimo tabulku"
   , "Syst�m nespustil DETACHED rutinu"
   , "�patn� ��slo kurzoru"
   , "�patn� k�d operace"
   , "Kurzor tuto operaci neumo��uje"
   , "S objektem %s n�kdo pracuje, nelze zamknout"
   , "Objekt %s neexistuje"
   , "Index mimo pole"//10
   , "Po�adovan� z�znam v kurzoru nenalezen (02000)"
   , "Z�znam je uvoln�n�, operaci nelze prov�st"
   , "Objekt neexistuje"
   , "Uvedeno nespr�vn� heslo pro u�ivatele %s"
   , "Odkaz na zru�en� z�znam"
   , "Neexistuj�c� odkaz"
   , "Vy�erp�na opera�n� pam�"
   , "Z�znam neexistuje"
   , "Z�znam je zru�en�"
   , "Index %s neexistuje"//20
   , "Objekt %s nenalezen"
   , "Nen� dost pam�ti"
   , "Chybn� d�lka dat"
   , "Ne�iteln� blok"
   , "Klient se dosud �sp�n� nep�ihl�sil na server"
   , "Vy�erp�na diskov� pam�"
   , "Operace zru�ena"
   , "Ztr�ta spojen� se SQL serverem"
   , "Chyba p�i pr�ci se souborem"
   , "Nekompatibiln� verze"//30
   , "Server odm�t� po�adavek"
   , "Sloupec %s nesm� m�t pr�zdnou hodnotu (NULL) (40002)"
   , "Klient je p�ihl�en bez licence s omezen�mi mo�nostmi a sna�� se prov�st nepovolenou operaci"
   , "Tento objekt nelze zru�it, proto�e na n�j odkazuje objekt %s"
   , "Intern� chyba OOD"
   , "Intern� chyba OVERRUN"
   , "Intern� chyba PAGING"
   , "Intern� chyba MISSING DEFLOCK na %s"
   , "Intern� chyba OOBSTACK"
   , "Tabulka %s je v�n� po�kozen�"//40
   , "Nelze zamknout server"
   , "Rollback b�hem vytv��en� kurzoru"
   , "Deadlock: vz�jemn� zablokov�n� klient� v j�d�e"
   , "Duplicita kl��� v tabulce %s: ji� existuje z�znam se stejn�m kl��em (40004)"
   , "Nesouhlas verz� klienta a serveru"
   , "Poru�eno integritn� omezen� %s datab�zov� tabulky (40005)"
   , "Poru�ena referen�n� integrita %s mezi tabulkami (40006)"
   , "Nep��pustn� typ argumentu v agrega�n� funkci"
   , "Tabulka je zcela zapln�na"
   , "Vno�en� po�adavk� na server"//50
   , "Tuto operaci nelze prov�st pro ODBC kurzor"
   , "Chybn� argument funkce"
   , "Zadan� ODBC kurzor nen� otev�en"
   , "ODBC driver nen� schopen po�adovan� operace"
   , "P��li� slo�it� transakce"
   , "Intern� sign�l"
   , "Konverze/porovn�n� �et�zc� %s nen� k dispozici"
   , "Nen� k dispozici p�episovac� pe�ek"
   , "�ek�m na potvrzen� p�ede�l�ho paketu"
   , "Replikace dosud blokov�na"//60
   , "Operaci blokuje stav p�episovac�ho pe�ka"
   , "Tabulka nem� pot�ebn� p��znaky"
   , "Index k tabulce %s je po�kozen"
   , "Vypr�ela platnost hesla, mus�te zadat nov�"
   , "Nem�te platn� kl��, nelze podepisovat"
   , "V souboru je jin� kl��"
   , "Intern� chyba serveru"
   , "Kurzor je v nep��pustn�m stavu (24000)"
   , "Neplatn� specifikace savepointu (3B001)"
   , "P��li� mnoho savepoint� (3B002)"//70
   , "Transakce je aktivn� (25001)"
   , "Nep��pustn� ukon�en� transakce (2D000)"
   , "Transakce je READ ONLY (25006)"
   , "��seln� hodnota mimo p��pustn� rozsah (22003)"
   , "�et�zec znak� nelze konvertovat (22018)"
   , "�et�zec znak� se��znut zprava (22001)"
   , "D�len� nulou (22012)"
   , "Nespr�vn� velikost odpov�di na dotaz (21000)"
   , "Chybn� �et�zec znak� za ESCAPE (22019)"
   , "V p��kazu CASE nenalezena v�tev (20000)"//80
   , "U�ivatelsk� v�jimka `%s` nen� o�et�ena (45000)"
   , "P��kaz RESIGNAL proveden mimo handler (0K000)"
   , "Extern� knihovna nebo funkce nenalezena (38001)"
   , "Ve funkci nebyl proveden p��kaz RETURN (2F001)"
   , "Sloupec %s nen� editovateln�"
   , "Chyba v triggeru (09000)"
   , "Replikov�n� nen� zapnuto"
   , "Chyba p�i pos�l�n� paketu"
   , "Neplatn� jm�no kurzoru (34000)"
   , "Role je z jin� aplikace ne� objekt"
   , "Vy�erp�ny v�echny hodnoty ze sekvence %s"
   , "Nelze volat CURRVAL p�ed NEXTVAL v sekvenci %s"
   , "Server nem� licenci pro p��stup z WWW"
   , "Vy�erp�ny licence pro p��stup klient�"
   , "Knihovna je um�st�na v adres��i, z n�ho� nen� dovoleno spou�t�t extern� funkce"
   , "Knihovna %s obsahuj�c� extern� funkci nebyla nalezena"
   , "Jazykov� podpora %s nen� instalov�na"
   , "Nemohu konvertovat tento typ dokumentu (%s)"
   , "Server nem� licenci na fulltextov� n�stroje"
   , "Server b�� s pr�vy roota, odm�t�m pou��vat extern� funkce"
   , "Nen� co replikovat"
   , "Objekt byl zm�n�n"
   , "Tabulka %s nem� unik�tn� kl�� pro replikace"
   , "Vy�erp�ny r�my na diskov� bloky, nutno zv�t�it vlastnost `Frame Space`"
   , "Nem�te pr�vo konfigurovat datab�zi nebo ��dit provoz"
   , "Nem�te pr�vo nastavovat bezpe�nostn� parametry"
   , "Pou�it� tohoto u�ivatelsk�ho jm�na je zak�z�no"
   , "Toto sch�ma je otev�eno jin�m klientem nebo vl�knem a nelze jej proto zru�it"
   , "Chybn� argument funkce"
   , "Server nem� licenci na XML roz���en�"
   , "P�ekro�en limit %s"
   
   , "Nezn�m� chyba"
};

const char * db_warnings[] = {
     "Zah�jen� transakce provedeno uvnit� transakce"
   , "Ukon�en� transakce provedeno mimo transakci"  
   , "Integritn� omezen� na tabulce je chybn�" 
   , "Z�znam nelze obnovit proto�e nen� zru�en�"  
   , "Implicitn� hodnota sloupce je chybn�" 
   , "Pr�ce se zru�en�m z�znamem"
   , "Z�znam nelze zru�it proto�e nen� platn�"  
   , "K obousm�rn�mu ukazateli neexistuje protism�rn� ukazatel" 
   , "Index p�ekra�uje po�et hodnot multiatributu" 
   , "Ignoruji z�znam zru�en� jin�m klientem"
   , "?"
};

const char not_answered[] = "�ek�m na odpov�� od serveru";

const char * compil_errors[] = {
 "P�eklad bez chyb",
 "B�h programu skon�il",
 "B�h programu skon�il",
 "Neuzav�en� koment��",
 "�et�zec znak� p�esahuje ��dku",
 "Rekurze na lok�ln�ch rutin�ch nen� povolena",
 "O�ek�v�m typ",
 "O�ek�v�m levou hranatou z�vorku",
 "O�ek�v�m pravou hranatou z�vorku",
 "O�ek�v�m dv� te�ky",
 "O�ek�v�m cel� ��slo",//860
 "O�ek�v�m 'of'",
 "Chyba v mez�ch pole",
 "Pole je p��li� velk�",
 "O�ek�v�m identifik�tor",
 "O�ek�v�m rovn�tko",
 "Nep��pustn� znak ve zdrojov�m textu",
 "O�ek�v�m ��rku",
 "O�ek�v�m st�edn�k nebo 'end'",
 "O�ek�v�m st�edn�k",
 "O�ek�v�m deklaraci nebo 'begin'",  //870
 "Podprogramy nelze vno�ovat",
 "O�ek�v�m levou z�vorku",
 "O�ek�v�m pravou z�vorku",
 "O�ek�v�m dvojte�ku",
 "Identifik�tor %s ozna�uje nep��pustn� objekt",
 "V�raz mus� b�t typu boolean",
 "O�ek�v�m 'do'",
 "O�ek�v�m ':='",
 "Zde mus� b�t ��slo",
 "Mus� b�t celo��seln�ho typu", //880
 "Nep��pustn� operace",
 "Typy nejsou kompatibiln�",
 "Pen�ze nelze navz�jem n�sobit",
 "Nen� typu z�znam",
 "Nen� to ukazatel",
 "Nen� typu pole",
 "Identifik�tor %s nen� deklarov�n",
 "O�ek�v�m identifik�tor slo�ky z�znamu",
 "Index mus� b�t celo��seln�ho typu",
 "O�ek�v�m 'then'",//890
 "O�ek�v�m ��rku nebo dvojte�ku",
 "P��li� mnoho parametr�",
 "O�ek�v�m te�ku",
 "��slo je p��li� velk�",
 "O�ek�v�m p��kaz",
 "P��li� rozs�hl� prom�nn�",
 "P��li� velk� prom�nn�",
 "Chyba na konci",
 "Identifik�tor %s deklarov�n dvakr�t",
 "Funkce mus� vracet jednoduch� typ nebo �et�zec",//900
 "Chyba ve v�stupu k�du - nen� dost pam�ti?",
 "Chyba na konci",
 "Vno�en� p��stup do datab�ze",
 "Chybn� d�lka",
 "Chybn� index",
 "Typ nen� strukturovan�",
 "Pohled neexistuje",
 "Chyba p�i �ten� definice z datab�ze",
 "Nen� dost pam�ti",
 "O�ek�v�m 'to' nebo 'downto'",//910
 "Nedefinovan� identifik�tor %s",
 "O�ek�v�m faktor",
 "Dotaz %s neexistuje",
 "Nep��pustn� argument agrega�n� funkce",
 "Chyba v konstant� typu TIME",
 "Chyba v konstant� typu DATE",
 "Chybn� datab�zov� p�i�azen�",
 "Nen� to sloupec prom�nn� velikosti",
 "Tabulka %s neexistuje",
 "Chybn� argument p��kazu ACTION",//920
 "O�ek�v�m obr�cen� lom�tko",
 "O�ek�v�m kl��ov� slovo UNTIL",
 "Nespr�vn� projekt",
 "O�ek�v�m kl��ov� slovo END",
 "Nespecifick� chyba v definici pohledu",
 "O�ek�v�m �et�zec znak�",
 "Nespecifick� chyba",
 "��slo mimo povolen� rozsah",
 "P��li� slo�it� pohled",
 "O�ek�v�m kl��ov� slovo definice",//930
 "�patn� identifik�tor",
 "Chybn� po�ad� skupin",
 "P��li� mnoho slo�ek pohledu",
 "O�ek�v�m slo�ku pohledu",
 "Chyba v definici dotazu",
 "P��li� mnoho slo�ek menu",
 "Include neexistuje",
 "Nelze osamostatnit lok�ln� intern� proceduru nebo standardn� funkci",
 "Duplicita identifikac� slo�ek pohledu",
 "P�ed ELSE nesm� b�t st�edn�k",//940
 "P��li� mnoho sloupc�",
 "P��li� mnoho index� k tabulce",
 "Nen� definov�n ��dn� sloupec",
 "Pro tento typ sloupce nelze pou��t []",
 "Duplicita jmen omezen� v n�vrhu tabulky",
 "Index m� p��li� mnoho ��st�, maximum je 8",
 "V indexu je sloupec nep��pustn�ho typu nebo multiatribut",
 "Kl�� indexu je p��li� dlouh�",
 "Chyba v syntaxi definice tabulky resp. indexu",
 "Index je p��li� slo�it�",//950
 "P��li� mnoho tabulek v dotazu",
 "O�ek�v�m FROM",
 "O�ek�v�m jm�no tabulky",
 "Chyba v syntaxi dotazu",
 "Chyba v predik�tu",
 "O�ek�v�m SELECT",
 "O�ek�v�m FROM nebo ��rku",
 "Klauzule WHERE ani GROUP BY nesm� obsahovat agrega�n� funkci",
 "Nelze vno�ovat agrega�n� funkce",
 "Hv�zdi�ku lze pou��t pouze ve funkci COUNT(*)",//960
 "Chybn� syntaxe v jazyce SQL",
 "Po BETWEEN o�ek�v�m slovo AND",
 "O�ek�v�m OUTER",
 "O�ek�v�m JOIN",
 "Chyba v klauzuli FROM",
 "Po ORDER resp. GROUP o�ek�v�m slovo BY",
 "Chybn� jm�no typu",
 "Syntaktick� chyba v popisu tabulky",
 "Po PRIMARY resp. FOREIGN o�ek�v�m slovo KEY",
 "Struktura kurzoru nen� zn�ma",//970
 "O�ek�v�m REFERENCES",
 "Identifik�tor %s nenalezen",
 "Po DOUBLE o�ek�v�m slovo PRECISION",
 "O�ek�v�m slovo NULL",
 "Toto nen� p��kaz jazyka SQL",
 "O�ek�v�m TO",
 "O�ek�v�m ON",
 "O�ek�v�m INTO",
 "O�ek�v�m VALUES",
 "Argument agrega�n� funkce je nep��pustn�ho typu",//980
 "O�ek�v�m SET",
 "Kurzor nen� editovateln� nebo vznikl z v�ce tabulek.\nNelze ru�it z�znamy.",
 "Kurzor nen� editovateln�\nNelze modifikovat jeho obsah",
 "Uvedeno p��li� mnoho hodnot v p��kazu INSERT",
 "Dotazy spojen� UNION/EXCEPT/INTERSECT maj� nekompatibiln� seznamy sloupc�",
 "Dotazy spojen� UNION/EXCEPT/INTERSECT maj� p��li� rozd�ln� seznamy sloupc�",
 "Neplatn� ��slo pozice v klauzuli ORDER BY",
 "Agrega�n� funkce nelze pou��t v ORDER BY, nahra�te jm�nem sloupce (AS)",
 "O�ek�v�m UPDATE",
 "O�ek�v�m NOT",//990
 "Chyba v referen�n� integrit�, nad�azen� tabulka neobsahuje vhodn� index",
 "Duplicita jmen sloupc� v popisu tabulky",
 "Tabulka nem��e m�t dva prim�rn� kl��e",
 "O�ek�v�m INDEX",
 "O�ek�v�m AS",
 "Ke sloupci nem�te pr�vo �ten�",
 "Tabulka neobsahuje vhodn� index",
 "Chyba v p��kazu SELECT ve slo�ce combo",
 "Podprogram je p��li� velk�",
 "Kurzor nen� editovateln� nebo vznikl z v�ce tabulek.\nNelze do n�j vkl�dat.",//1000
 "Chyba v implicitn� hodnot� sloupce",
 "Ke sloupci nem�te pr�vo p�episu",
 "Rekurze v definici dotazu",
 "Nep��pustn� znak v liter�lu",
 "Neuzav�ena p�edsunut� deklarace (FORWARD)",
 "Neuzav�ena deklarace ukazatele",
 "Neexistuje spole�n� sloupec",
 "Nem�te pr�vo odkazovat na sloupec %s",
 "Subdotaz nesm� b�t uvnit� agrega�n� funkce",
 "O�ek�v�m IN",//1010
 "O�ek�v�m prvek data nebo �asu",
 "O�ek�v�m konstantu typu Boolean",
 "NOT nen� zde p��pustn�",
 "Subdotaz nen� skal�rn�",
 "Tento p��kaz nen� dovolen v ATOMIC bloku",
 "O�ek�v�m IF",
 "O�ek�v�m LOOP",
 "O�ek�v�m RETURNS",
 "O�ek�v�m PROCEDURE",
 "O�ek�v�m kl��ov� parametr",//1020
 "Stejn� parametr specifikov�n dvakr�t",
 "Nevhodn� druh parametru",
 "P��kaz RETURN mus� b�t uvnit� funkce",
 "Nelze m�nit hodnotu konstanty",
 "Konstantn� prom�nn� mus� m�t specifikovanou hodnotu",
 "O�ek�v�m SQLSTATE",
 "O�ek�v�m FOR",
 "Chyba ve form�tu SQLSTATE",
 "O�ek�v�m FOUND",
 "Intern� chyba kompil�toru",//1030
 "Nep��tustn� objekt v klauzuli INTO",
 "O�ek�v�m HANDLER",
 "REDO nebo UNDO handler lze pou��t pouze v atomick�m p��kazu",
 "O�ek�v�m REPEAT",
 "O�ek�v�m WHILE",
 "O�ek�v�m CURSOR",
 "O�ek�v�m SAVEPOINT",
 "O�ek�v�m CHAIN",
 "O�ek�v�m TRANSACTION",
 "Dvoj� specifikace t�ho� �daje",//1040
 "Specifikace jsou ve vz�jemn�m rozporu",
 "O�ek�v�m CASE",
 "Tato operace nen� dovolena pro kurzor z p��kazu FOR",
 "Po�adavek SENSITIVE je v rozporu s definic� kurzoru",
 "Agrega�n� funkce mimo kontext",
 "O�ek�v�m NAME",
 "Tento typ nen� p��pustn�",
 "O�ek�v�m OBJECT",
 "O�ek�v�m BEFORE nebo AFTER",
 "O�ek�v�m INSERT, DELETE nebo UPDATE",//1050
 "O�ek�v�m OLD nebo NEW",
 "Transientn� tabulky nejou podporov�ny",
 "O�ek�v�m EACH",
 "O�ek�v�m ROW nebo STATEMENT",
 "O�ek�v�m TRIGGER",
 "P�i�azen� do nep��pustn�ho objektu",
 "Tento rys SQL nen� podporov�n",
 "Za * nesm� n�sledovat dal�� sloupec dotazu",
 "Nelze vytvo�it prom�nnou typu formul�� beze jm�na (instanci abstraktn� t��dy)",
 "Tento objekt vy�aduje projekt %s",
 "Tento druh slo�ky pohledu nem� vlastnost nebo metodu %s",
 "Metoda ani vlastnost %s neexistuje a slo�ky formul��e nejsou zn�my",
 "THISFORM nen� v tomto kontextu definov�no",
 "Nedefinov�na direktiva preprocesoru",
 "Konec souboru v rozvoji makra",
 "Tato direktiva nen� zde p��pustn�",
 "Konec souboru uvnit� podm�n�n�ho p�ekladu zapo�at�ho na ��dce %s",
 "Toto je chr�n�n� objekt. Nelze do n�j nahl�dnout.",
 "Typelib error",
 "O�ek�v�m WITH", // 1070
 "Konflikt v parametrech sekvence",
 "Syntaxe neopov�d� nastaven�m p��znak�m kompatibility",
 "V ulo�en� procedu�e a triggeru nelze pou��vat prom�nn� klienta ani dynamick� parametry",
 "Pro tuto chybu nelze definovat handler",
 "V modulu glob�ln�ch delarac� nelze definovat rutiny ani kurzory",
 "Tuto akci sm� prov�st pouze autor aplikace",
 "O�ek�v�m EXISTS",
 "P��li� slo�it� dotaz",
 "Debugger nem��e vyhodnotit funkci",
 "Syntaktick� chyba v klauzuli LIMITS",
 "Label sm� b�t pouze uvnit� bloku BEGIN...END"
 };

const char * kse_errors[] = {
  "Chyba inicializace serveru"
, "Nelze otev��t okno serveru pod Windows"
, "Fale�n� identita SQL serveru"
, "Neshoda klienta a serveru na �ifrov�n� komunikace"
, "Klientovi se nepoda�ilo spustit lok�ln� SQL server"
, "Chyba p�i alokaci pam�ti"
, "Datab�zov� soubor byl konvertov�n pro CD-ROM a nebyl pot� korektn� instalov�n"
, "Server nelze spustit, jeho jm�no nen� registrov�no"
, "Datab�zov� soubor je v�n� po�kozen"
, "Platforma nepodporuje syst�mov� jazyk databazov�ho souboru"
, "P��stup na server je do�asn� uzam�en"//10
, "Verze datab�zov�ho souboru neodpov�d� verzi serveru"
, "Nelze inicializovat zvolen� s�ov� protokol"
, "Verze serveru nen� kompatibiln� s verz� klienta"
, "Server nebyl nalezen v s�ti ani pro n�j nen� specifikov�na platn� IP adresa"
, "Nepoda�ilo se nav�zat spojen� s SQL serverem"
, "P�ekro�eno maximum spojen� mezi klienty a serverem"
, "Intern� chyba spojen�"
, "Server nem� licenci pro s�ov� p��stup"
, "HTTP tunel nen� zapnut na serveru (na stejn�m portu b�� web XML interface)"
, ""//20
, ""
, "Nen� dost ��dic�ch blok� NetBIOS"
, "Nad stejnou datab�z� ji� pracuje jin� server nebo nedostatek pr�v k db souboru"
, "V s�ti ji� b�� datab�zov� server stejn�ho jm�na "
, "Nelze spustit dal�� vl�kno na serveru"
, "Nelze vytvo�it synchoniza�n� objekt"
, "Neda�� se mapovat pam�ov� soubor"
, ""
, "Lok�ln� server neodpov�d�l v �asov�m limitu"
, "Knihovna TCP/IP socket� nefunguje"//30
, "Chyba p�i pr�ci se socketem"
, ""
, ""
, "Nelze p�idat jm�no pro NetBIOS\n(32-bitov� klient na stejn�m stroji jako server?)"
, "Nenalezen SOCKS server"
, "Chyba p�i komunikaci se SOCKS serverem"
, "SOCKS server (firewall) odm�tl vytvo�it spojen� na SQL server"
, "Zad�no chybn� heslo k datab�zov�mu souboru"
, "Upu�t�no od startu serveru"
, "Protokol IPX nen� instalov�n"//40
, ""
, "Server nem��e vytvo�it datab�zov� soubor"
, "Server nem��e otev��t datab�zov� soubor"
, "Server nem��e otev��t transak�n� soubor"
, "P��stup z t�to IP adresy na server nen� povolen"
, ""
, "Instala�n� kl�� pro datab�zi nen� uveden nebo nen� platn�"
, "SQL server nem� licenci pro provoz"
, "Nezn�m� chyba"
};

#ifndef SRVR
const char * client_errors[] = {
  "Knihova %s, generuj�c� ��rov� k�d, nebyla nalezena"
, "Zpr�vu o d�lce %d nelze vyj�d�it t�mto druhem ��rov�ho k�du"
, "Zpr�va %s obsahuje znak, kter� nen� p��pustn� v tomto druhu ��rov�ho k�du"
, "Knihovna generuj�c� ��rov� k�d ohl�sila chybu %d. Pravd�podobn� nep��pustn� hodnoty vlastnost�."
, "Slo�ka ��slo %d nem� definovan� ��dn� obsah"
, "Po�kozena spr�va pam�ti, ukon�uji program"
, "Nezda�ilo se roz���en� pracovn� pam�ti klienta"
, "Klient roz�i�uje svoji pracovn� pam�"
, "Slo�ka DTP nebyla vytvo�ena proto�e jej� obsah nen� typu datum, �as ani timestamp"
, "Slo�ka Kalend�� nebyla vytvo�ena, proto�e jej� obsah nen� typu datum ani timestamp"
, "Knihovna 602dvlp8.DLL obsahuj�c� n�vrh��e nebyla nalezena"
, "Knihovna 602wed8.DLL pro n�vrh WWW objekt� nebyla nalezena"
, "Chybn� verze knihovny 602dvlp8.DLL"
, "Chybn� verze knihovny 602wed8.DLL"
, "Slo�ku nelze vytvo�it proto�e nen� implementov�na v t�to verzi COMCTL32.DLL"
, "Subformul�� neotev�en proto�e nen� zad�no jeho jm�no"
, "Nelze otev��t subformul�� se stejn�m jm�nem %s jako m� hlavn� formul��"
, "Relace %s obsahuje jm�na sloupc� nevyskytuj�c�ch se v kurzorech pohled�"
, "Tuto operaci nelze prov�st na fiktivn�m z�znamu"
, "Anonymn� u�ivatel nem��e podepisovat"
, "Nelze exportovat chr�n�n� objekt bez za�ifrov�n�"
, "Nelze zano�ovat vol�n� popup menu"
, "Selh�n� cache objektov�ch descriptor�"
, "%s nelze pou��t v obecn�m formul��i"
, "%s nelze pou��t ve slo�ce standardn�ho formul��e"
, "%s nelze pou��t v zav�en�m formul��i"
, "Pohled %s nenalezen"
, "Knihovna obsahuj�c� podporu XML nebyla nalezena"
, "Chybn� verze knihovny s podporou XML"
, "Objekt nelze p�ejmenovat"
, "Nelze p�ejmenovat tabulku, na kterou jsou odkazy v referen�n� integrit�"   
, "Nenalezen MEMO soubor"
, "Chyba konverze dat na v�stupn�m z�znamu ��slo %s."
};

const char * ipjrt_errors[] = {
       "Nedefinovan� instrukce (po�kozen� k�d)"
,      "P�ete�en� z�sobn�ku (p��li� slo�it� v�raz)"
,      "Funkce %s nenalezena v DLL"
,      "DLL knihovna %s nebyla nalezena"
, ""
, ""
,      "Ukazatel m� hodnotu NIL"
,      "Nespecifick� chyba"
,      "Sloupec neexistuje"
,      "Vy�erp�na b�hov� pam�"
,      "Aritmetick� chyba"
,      "P�eru�eno u�ivatelem"   /* not used, replaced by RUN_BREAKED */
,      "Tabulka/dotaz/pohled neexistuje"
,      "Vy�erp�na pam� pro dynamick� alokace"
,      "Nem�te pr�vo nastavit projekt ani spustit program"
,      "Objekt odkazuje na projekt, kter� byl zm�n�n."
,      "Typy nelze konvertovat"
,      "P�ekro�ena horn� mez pole"
,      "Chyba p�i vol�n� metody ActiveX objektu"
,      "Chybn� po�et parametr�"
,      "Chybn� typ parametru"
,      "Vol�n� metody zp�sobilo v�jimku"     
,      "Nezn�m� metoda nebo vlastnost"
,      "P�ete�en� p�i konverzi parametru"
};

#endif

const char * mail_errors[] = {
  "Po�ta nen� inicializov�na"
, "Nezn�m� chyba po�ty"
, "Po�tovn� klient nen� vzd�len�"
, "Chybn� typ adresy"
, "Chyba p�i pokusu o p�ihl�en�"
, "Chybn� profil po�ty"
, "Chybn� ID u�ivatele"
, "Chybn� adresa (po�tovn� server neumo��uje odesl�n� z�silky na tuto adresu)"
, "Soubor nenalezen"
, "Server se nem��e p�ihl�sit do po�ty, proto�e b�� na syst�mov� ��et"
, "Nepoda�ilo se nav�zat telefonick� spojen� s po�tovn�m serverem"
, "Po�ta u� je inicializov�na s jin�mi parametry"
, "Profil po�ty nenalezen"
, "V profilu chyb� parametr"
, "Odesl�n� zadan�ho souboru nen� povoleno"
, "I/O chyba p�i s�ov� komunikaci"
, "SMTP/POP3 server nenalezen"
, "Nepoda�ilo se nav�zat spojen� s po�tovn�m serverem"
, "V z�silce nejsou dal�� p�ipojen� soubory"
, "P�ipojen� soubor nenalezen"
, "Schr�nka je zam�en�"
, "Z�silka nenalezena"
, "Z�silka je po�kozen� nebo m� nezn�m� form�t"
, "Nenalezena DLL knihovna"
, "Nenalezena funkce v DLL knihovn�"
, "Po�adovan� po�tovn� slu�ba nen� podporov�na"
, "Po�adavek byl za�azen do fronty, bude odesl�n p�i p��t�m nav�z�n� telefonick�ho spojen�"
, "Po�tovn� profil u� existuje"
};

#else  // ENGLISH or new client

#define gettext_noop(string) string

const char * db_errors[] = {
     gettext_noop("Bad modifier")
   , gettext_noop("You do not have the rights to perform this action")
   , gettext_noop("Bad column number")
   , gettext_noop("Record out of table range")
   , gettext_noop("The system did not start the DETACHED routine")
   , gettext_noop("Bad cursor number")
   , gettext_noop("Bad operation code")
   , gettext_noop("Cursor does not allow this action")
   , gettext_noop("Object %s is in use, cannot lock")
   , gettext_noop("Object %s does not exist")
   , gettext_noop("Index out of array")//10
   , gettext_noop("The specified record in the cursor does not exist (02000)")
   , gettext_noop("Operation cannot be completed because the record has been released")
   , gettext_noop("Object does not exist")
   , gettext_noop("Incorrect password for user %s")
   , gettext_noop("Reference to deleted record")
   , gettext_noop("Reference does not exist")
   , gettext_noop("Not enough memory")
   , gettext_noop("Record does not exist")
   , gettext_noop("Record is deleted")
   , gettext_noop("Index %s does not exist")//20
   , gettext_noop("Object %s not found")
   , gettext_noop("Not enough memory")
   , gettext_noop("Bad data length")
   , gettext_noop("Unreadable block")
   , gettext_noop("Client has not successfully logged in yet")
   , gettext_noop("Out of disk memory")
   , gettext_noop("Operation canceled")
   , gettext_noop("Connection to the SQL server lost")
   , gettext_noop("File error")
   , gettext_noop("Uncompatible version")//30
   , gettext_noop("Server refused request")
   , gettext_noop("Column %s must not have an empty value (NULL) (40002)")
   , gettext_noop("The client is logged in without a licenses and this operation is not allowed")
   , gettext_noop("This object cannot be deleted because it is referenced by object %s")
   , gettext_noop("Internal error OOD")
   , gettext_noop("Internal error OVERRUN")
   , gettext_noop("Internal error PAGING")
   , gettext_noop("Internal error MISSING DEFLOCK on %s")
   , gettext_noop("Internal error OOBSTACK")
   , gettext_noop("Table %s is damaged")//40
   , gettext_noop("Cannot lock server")
   , gettext_noop("Rollback during cursor creation")
   , gettext_noop("Deadlock")
   , gettext_noop("Key duplicity in table %s occured (40004)")
   , gettext_noop("Client and server versions do not match")
   , gettext_noop("Table constraint %s exception (40005)")
   , gettext_noop("Referential integrity %s exception (40006)")
   , gettext_noop("Incorrect argument type in the aggregate function")
   , gettext_noop("Database table is full")
   , gettext_noop("Nesting of server requests")//50
   , gettext_noop("ODCB cursor does not support this function")
   , gettext_noop("Error in function argument")
   , gettext_noop("The ODBC cursor is not open")
   , gettext_noop("The ODBC driver does not support this operation")
   , gettext_noop("Transaction is too complex")
   , gettext_noop("Internal signal")
   , gettext_noop("String conversion/comparison %s is not available")
   , gettext_noop("Write token is not available")
   , gettext_noop("Waiting for acknowledgement of the last packet")
   , gettext_noop("Replication is currently blocked")//60
   , gettext_noop("Operation is disabled due to the token state")
   , gettext_noop("Table does not have the necessary properties")
   , gettext_noop("Index on table %s is damaged")
   , gettext_noop("Your password has expired.")
   , gettext_noop("You do not have a valid key")
   , gettext_noop("File contains a different key")
   , gettext_noop("Internal server error")
   , gettext_noop("Invalid cursor state (24000)")
   , gettext_noop("Invalid savepoint specification (3B001)")
   , gettext_noop("Too many savepoints (3B002)")//70
   , gettext_noop("Transaction state active (25001)")
   , gettext_noop("Invalid transaction termination (2D000)")
   , gettext_noop("Transaction state READ ONLY (25006)")
   , gettext_noop("Numeric value out of range (22003)")
   , gettext_noop("Invalid character value for cast (22018)")
   , gettext_noop("String data right truncation (22001)")
   , gettext_noop("Division by zero (22012)")
   , gettext_noop("Cardinality violation (21000)")
   , gettext_noop("Invalid escape character (22019)")
   , gettext_noop("Case for found (20000)")//80
   , gettext_noop("Unhandled user exception `%s` (45000)")
   , gettext_noop("RESIGNAL when handler is not active (0K000)")
   , gettext_noop("External library or function not found (38001)")
   , gettext_noop("Function did not execute the return statement (2F001)")
   , gettext_noop("Column %s is not editable")
   , gettext_noop("Triggered statement exception (09000)")
   , gettext_noop("Replication is disabled")
   , gettext_noop("Error sending packet")
   , gettext_noop("Invalid cursor name (34000)")
   , gettext_noop("Role is from different application than the object")
   , gettext_noop("All values of sequence %s have been used")
   , gettext_noop("Cannot call CURRVAL before NEXTVAL in sequence %s")
   , gettext_noop("The server does not have the WWW access license")
   , gettext_noop("No more client licenses")
   , gettext_noop("Library is located in a directory that is not listed on the external functions directory list")
   , gettext_noop("Could not find the library %s containing the external function")
   , gettext_noop("%s language support is not installed")
   , gettext_noop("Cannot convert document in this format (%s)")
   , gettext_noop("The server does not have a license for the fulltext search")
   , gettext_noop("The server is running under the root account. External functions are disabled.")
   , gettext_noop("No changes to replicate")
   , gettext_noop("Object has been changed")
   , gettext_noop("Table %s has no key for replication")
   , gettext_noop("Disk block frames are exhausted, increase the Frame Space property value")
   , gettext_noop("The configuration privilege is not granted")
   , gettext_noop("The privilege to control security is not granted")
   , gettext_noop("This user account is disabled")
   , gettext_noop("The schema is opened by another client or thread and cannot be dropped")
   , gettext_noop("Error in function argument")
   , gettext_noop("The server does not have a license for the XML extension")
   , gettext_noop("Limit %s exceeded")

   , gettext_noop("Unknown error")
};

const char * db_warnings[] = {
     gettext_noop("Transaction started inside a transaction")
   , gettext_noop("Transaction closed while no outstanding transaction exists")  
   , gettext_noop("Error in CHECK constrain") 
   , gettext_noop("The record cannot be restored, since it is not deleted")  
   , gettext_noop("Error in the default value of a column") 
   , gettext_noop("Working with a deleted record")
   , gettext_noop("The record cannot be deleted because it is not valid")  
   , gettext_noop("The pointer is bidirectional, but the pointer for one direction is missing") 
   , gettext_noop("The multi-attribute does not have enough values") 
   , gettext_noop("Skipping record deleted by other client") 
   , "?"
};

const char not_answered[] = gettext_noop("Waiting for a response from the server");

const char * compil_errors[] = {
         gettext_noop("Compilation successful")
,        gettext_noop("Program finished")
,        gettext_noop("Program finished")
,        gettext_noop("Comment not closed")
,        gettext_noop("The string exceeds a line")
,        gettext_noop("Recursion on a local routine is not allowed")
,        gettext_noop("Type expected")
,        gettext_noop("Left bracket expected")
,        gettext_noop("Right bracket expected")
,        gettext_noop("Two points expected")
,        gettext_noop("Integer expected")//860
,        gettext_noop("'of'expected")
,        "Array dimension error"
,        "Array too large"
,        gettext_noop("Identifier expected")
,        gettext_noop("Equal sign expected")
,        gettext_noop("Illegal character in the source text")
,        gettext_noop("Comma expected")
,        gettext_noop("Semicolon or 'end' expected")
,        gettext_noop("Semicolon expected")
,        gettext_noop("Declaration or 'begin' expected")//870
,        gettext_noop("Cannot nest subprograms")
,        gettext_noop("Left parenthes expected")
,        gettext_noop("Right parenthes expected")
,        gettext_noop("Colon expected")
,        gettext_noop("Identifier %s belongs to a wrong class")
,        gettext_noop("Expression must be Boolean")
,        gettext_noop("'do' expected")
,       gettext_noop("':=' expected")
,       gettext_noop("A number must be here")
,       gettext_noop("Must have an integer value")//880
,       gettext_noop("Illegal operation")
,       gettext_noop("Types are not compatible")
,       gettext_noop("Cannot multiple Money by Money")
,       gettext_noop("Not a record")
,       "Not a pointer"
,       "Not an array"
,       gettext_noop("Identifier %s is not declared")
,       gettext_noop("Identifier of a record field expected")
,       gettext_noop("Index must be a whole number")
,       gettext_noop("'then' expected")//890
,       gettext_noop("Comma or colon expected")
,       gettext_noop("Too many parameters")
,       gettext_noop("Period expected")
,       gettext_noop("A number is too large")
,       gettext_noop("Statement expected")
,       gettext_noop("Variables too large")
,       gettext_noop("Variables too big")
,       gettext_noop("Error at the end")
,       gettext_noop("Identifier %s has been declared twice")
,       gettext_noop("Function must return a simple type or a string")//900
,       gettext_noop("Output code error. This may be caused by insufficient memory.")
,       gettext_noop("Error at the end")
,       gettext_noop("Nested approach into a database")
,       gettext_noop("Bad length")
,       gettext_noop("Incorrect index")
,       gettext_noop("Type not structured")
,       "The view doesn't exist"
,       gettext_noop("Error reading definition from the database")
,       gettext_noop("Not enough memory")
,       gettext_noop("'to' or 'downto' expected")//910
,       gettext_noop("Undefined identifier %s")
,       gettext_noop("Factor expected")
,       gettext_noop("Query %s doesn't exist")
,       gettext_noop("Illegal argument of an aggregate function")
,       gettext_noop("TIME constant error")
,       gettext_noop("DATE constant error")
,       gettext_noop("Illegal database assignment")
,       gettext_noop("Not a column of variable size")
,       gettext_noop("Table %s does not exist")
,       gettext_noop("Illegal argument of ACTION statement")//920
,       gettext_noop("Backslash expected")
,       gettext_noop("UNTIL expected")
,       "Incorrect project"
,       gettext_noop("END expected")
,       "View definition error"
,       gettext_noop("String expected")
,       gettext_noop("Unspecific error")
,       gettext_noop("A number is out of range")
,       "View too complex"
,       gettext_noop("Keyword of definition expected")//930
,       gettext_noop("Illegal identifier")
,       gettext_noop("Bad group order")
,       "Too many controls in a view"
,       "View control expected"
,       gettext_noop("Query definition error")
,       "Too many menu items"
,       "Include doesn't exist"
,       gettext_noop("Cannot detach a local internal procedure or a standard function")
,       "Duplication of view control identifications"
,       "Semicolon in front of ELSE not allowed"//940
,       gettext_noop("Too many columns")
,       gettext_noop("Too many table indexes")
,       gettext_noop("No columns are defined")
,       "Impossible to use [] for this type of column"
,       gettext_noop("Constrain name duplicity in the table design")
,       gettext_noop("Index has too many parts, a maximum of 8 is allowed")
,       gettext_noop("A column of an illegal type is in an index definition")
,       gettext_noop("A key of an index is too long")
,       gettext_noop("Table or index definition error")
,       gettext_noop("Index too complex")//950
,       gettext_noop("Too many tables in a query")
,       gettext_noop("FROM expected")
,       gettext_noop("A name of a table is expected")
,       gettext_noop("Query definition error")
,       gettext_noop("Predicate error")
,       gettext_noop("SELECT expected")
,       gettext_noop("FROM or comma expected")
,       gettext_noop("WHERE and GROUP BY clauses cannot contain aggregate functions")
,       gettext_noop("Aggregate functions cannot be nested")
,       gettext_noop("Asterisk may only be used as the COUNT() argument")//960
,       gettext_noop("General error in SQL syntax")
,       gettext_noop("AND expected in BETWEEN predicate")
,       gettext_noop("OUTER expected")
,       gettext_noop("JOIN expected")
,       gettext_noop("Error in FROM clause")
,       gettext_noop("BY expected after ORDER or GROUP")
,       gettext_noop("Unknown or incorrect type name")
,       gettext_noop("General error in table definition")
,       gettext_noop("KEY expected after PRIMARY or FOREIGN")
,       gettext_noop("Cursor structure is unknown")//970
,       gettext_noop("REFERENCES expected")
,       gettext_noop("Identifier %s not found")
,       gettext_noop("PRECISION expected after DOUBLE")
,       gettext_noop("NULL expected")
,       gettext_noop("This is not an SQL statement")
,       gettext_noop("TO expected")
,       gettext_noop("ON expected")
,       gettext_noop("INTO expected")
,       gettext_noop("VALUES expected")
,       gettext_noop("Invalid type of the aggregate function argument")//980
,       gettext_noop("SET expected")
,       gettext_noop("Cannot perform a DELETE statement in this query")
,       gettext_noop("This query is not updatable")
,       gettext_noop("Too many values in the INSERT statement")
,       gettext_noop("The column lists are not compatible in the UNION/EXCEPT/INTERSECT")
,       gettext_noop("The column lists are different in the UNION/EXCEPT/INTERSECT")
,       gettext_noop("Invalid position number in the ORDER BY clause")
,       gettext_noop("Cannot use aggregate functions in ORDER BY, replace with column name")
,       gettext_noop("UPDATE expected")
,       gettext_noop("NOT expected")//990
,       gettext_noop("Foreign key constrain error, target table does not have a proper index")
,       gettext_noop("Column name duplicity in the table")
,       gettext_noop("Table cannot have two primary keys")
,       gettext_noop("INDEX expected")
,       gettext_noop("AS expected")
,       gettext_noop("No privilege for reading this column")
,       gettext_noop("Table does not have a proper index")
,       gettext_noop("Error in the SELECT statement of the combobox control")
,       gettext_noop("Subroutine is too large")
,       gettext_noop("Cannot perform INSERT statement in uneditable or multi-table cursor")//1000
,       gettext_noop("Error in the DEFAULT value of the column")
,       gettext_noop("No privilege for updating this column")
,       gettext_noop("Recursion in the query definition")
,       gettext_noop("Bad character in the hex literal")
,       "A FORWARD declatarion is not closed"
,       "A pointer declaration is not closed"
,       gettext_noop("No common column found")
,       gettext_noop("No reference privilege to column %s")
,       gettext_noop("Subquery in set function is not allowed")
, gettext_noop("IN expected")      //1010
, gettext_noop("Datetime field expected")      
, gettext_noop("Boolean constant expected")      
, gettext_noop("NOT is not allowed here")
, gettext_noop("Subquery is not scalar")
, gettext_noop("Transaction statement is invalid in an atomic block")
, gettext_noop("IF expected")
, gettext_noop("LOOP expected")
, gettext_noop("RETURNS expected")
, gettext_noop("PROCEDURE expected")
, gettext_noop("Keyword parameter expected")//1020
, gettext_noop("Duplicate parameter specification")
, gettext_noop("Bad parameter mode")
, gettext_noop("RETURN statement must be inside a function")
, gettext_noop("Cannot change the constant value")
, gettext_noop("CONSTANT variable must have a DEFAULT value")
, gettext_noop("SQLSTATE expected")  
, gettext_noop("FOR expected")      
, gettext_noop("Error in sqlstate format")      
, gettext_noop("FOUND expected")
, gettext_noop("Internal compiler error")//1030
, gettext_noop("Bad object in the INTO clause")
, gettext_noop("HANDLER expected")
, gettext_noop("REDO or UNDO handler can only be used in an atomic statement") 
, gettext_noop("REPEAT expected")      
, gettext_noop("WHILE expected")
, gettext_noop("CURSOR expected")
, gettext_noop("SAVEPOINT expected")
, gettext_noop("CHAIN expected")
, gettext_noop("TRANSACTION expected")
, gettext_noop("Double specification of the same item")//1040
, gettext_noop("Invalid combination of specifications")
, gettext_noop("CASE expected")
, gettext_noop("This statement cannot use the cursor from the FOR statement")
, gettext_noop("The cursor cannot be SENSITIVE")
, gettext_noop("The set function is out of context")
, gettext_noop("NAME expected")
, gettext_noop("This type is not allowed")
, gettext_noop("OBJECT expected")
, gettext_noop("BEFORE or AFTER expected")
, gettext_noop("INSERT, DELETE or UPDATE expected")//1050
, gettext_noop("OLD or NEW expected")
, gettext_noop("Transient (temporary) tables are not supported here")
, gettext_noop("EACH expected")
, gettext_noop("ROW or STATEMENT expected")
, gettext_noop("TRIGGER expected")
, gettext_noop("Invalid target for the assignment")
, gettext_noop("Unsupported feature")
, gettext_noop("The * cannot be followed by other columns")
, "Cannot create variable of the unnamed form type (abstract class instance)",
 "This project requires project %s", // 1060
 "This control does not have the property or method %s",
 "Form does not have method nor property %s and its controls are not known",
 "THISFORM undefined in this kontext",
 "Undefined preprocessor directive",
 "End of file in the macro expansion",
 "The preprocessor directive is not allowed here",
 "End of file in the conditional compilation started on line %s",
 gettext_noop("This is a protected object. It cannot be viewed.")
, gettext_noop("Typelib error")
, gettext_noop("WITH expected") // 1070
, gettext_noop("Conflicting SEQUENCE parameters")
, gettext_noop("Syntax does not conform to the active compatibility flags")
, gettext_noop("The SQL procedure or trigger cannot reference host variables or dynamic parameters")
, gettext_noop("This sqlstate cannot have a handler")
, gettext_noop("Global declarations cannot contain routines or cursors")
, gettext_noop("Only the author of the application is allowed to do this")
, gettext_noop("EXISTS expected")
, gettext_noop("Query is too complex")
, gettext_noop("Debugger cannot evaluate the function")
, gettext_noop("Syntax error in the LIMITS clause")
, gettext_noop("The label can only be inside a BEGIN...END block")
};

const char * kse_errors[] = {
     gettext_noop("Initialize Error")
   , gettext_noop("Cannot open the server window")
   , gettext_noop("Cannot confirm SQL Server identity")
   , gettext_noop("The client and server encryption requirements do not match")
   , gettext_noop("The client is unable to start the local SQL Server")
   , gettext_noop("Error allocating memory")
   , gettext_noop("The database file has been prepared for a CD-ROM and has not been properly installed")
   , gettext_noop("Cannot start the server, the name has not been registered")
   , gettext_noop("Database file is seriously damaged")
   , gettext_noop("The platform does not support the system language of the database file")
   , gettext_noop("Access to the server is temporarily disabled")//10
   , gettext_noop("The database file version and server do not match")
   , gettext_noop("Cannot initialize the network protocol")
   , gettext_noop("The version of the server is not compatible with the version of the client")
   , gettext_noop("The server is not found on the network or an invalid IP address was specified")
   , gettext_noop("Connection to the SQL Server is not established")
   , gettext_noop("Too many client connections to the server")
   , gettext_noop("Connection internal error")
   , gettext_noop("The SQL Server does not have a license for network access")
   , gettext_noop("HTTP tunnel is not enabled on the SQL server (web XML interface is running on the port)")
   , ""//20
   , ""
   , gettext_noop("Not enough NetBIOS control blocks")
   , gettext_noop("Another server is already running on the same database or you have insufficient privileges")
   , gettext_noop("An SQL Server with the same name is already running on the network.")
   , gettext_noop("Cannot start a new thread on the server")
   , gettext_noop("Cannot create synchonization object")
   , gettext_noop("Cannot map a memory-mapped file")
   , ""
   , gettext_noop("The local server did not respond in the time allowed")
   , gettext_noop("Cannot use TCP/IP, the socket library does not work")//30
   , gettext_noop("Socket function error")
   , ""
   , ""
   , gettext_noop("Cannot add NetBIOS name")
   , gettext_noop("SOCKS server not found")
   , gettext_noop("SOCKS server does not respond")
   , gettext_noop("SOCKS server denied the connection to the SQL Server")
   , gettext_noop("The password to the database file is invalid")
   , gettext_noop("Server not started")
   , gettext_noop("IPX Protocol is not installed")//40
   , ""
   , gettext_noop("Server cannot create the database file")
   , gettext_noop("Server cannot open the database file")
   , gettext_noop("Server cannot open the transaction file")
   , gettext_noop("Server cannot be accessed from your IP address")
   , ""
   , gettext_noop("Installation key for the database is not specified or is invalid")
   , gettext_noop("The SQL Server does not have an operational license")
   , gettext_noop("Unknown error")
};

#ifndef SRVR
const char * client_errors[] = {
  "Bar code library %s not found"
, "Message with the lenght of %d bytes cannot be encoded by this bar code"
, "Message %s contains an invalid character (with respect to the bar code type) "
, "Bar code library returned error number %d. Probably a wrong property value."
, "Control number %d does not have any contents defined"
, gettext_noop("Memory management corrupted, aborting...")
, gettext_noop("An attempt to extend the working memory failed")
, "Client extended its working memory"
, "The DTP control was not created because the type of its contents is not date, time nor timestamp"
, "The Month Calendar control was not created because the type of its contents is not date nor timestamp"
, "The 602dvlp8.DLL library with designers was not found"
, "The 602wed8.DLL library designing WWW object was not found"
, "Bad version if the 602dvlp8.DLL library"
, "Bad version if the 602wed8.DLL library"
, "Control is not implemented in this version of COMCTL32.DLL"
, "Subform not opened because its name is not specified"
, "Cannot open subform with the same name %s as the name of the main form"
, "Relation %s contains columns not appering in the cursors of forms"
, gettext_noop("Cannot do this on a fictitious record")
, "Anonymous user cannot sign documents"
, "Cannot export the protected object without encrypting"
, "Tracking popup menu cannot be nested"
, "Object descriptor cache failure"
, "%s cannot be used in a general form"
, "%s cannot be used in a control of a standard form"
, "%s cannot be used in a closed form"
, "Form or report %s not found"
, gettext_noop("The XML library was not found")
, gettext_noop("Incorrect version of the XML library")
, gettext_noop("This object cannot be renamed")
, gettext_noop("Table with referential integrity references cannot be renamed")
, gettext_noop("MEMO file not found")
, gettext_noop("Data conversion error on output record %s.")
, gettext_noop("This operation is impossible, since other users are currently connected to the database")
, gettext_noop("The daabase server cannot be locked")
, gettext_noop("Not connected as configuration administrator")
, gettext_noop("Not connected as security or data administrator")
, gettext_noop("Cannot create a file")
, gettext_noop("The new database file is not empty")
, gettext_noop("Cannot move or delete the old database file")
};

const char * ipjrt_errors[] = {
        "Undefined instruction (code is damaged)"
,       "Stack overflow (too complex expression)"
,       "Function %s not found in the DLL"
,       "Dynamic library %s not found"
,""
,""
,       "Pointer has the NIL value"
,       "Nonspecific error"
,       "Column does not exist"
,       "Not enough run-time memory"
,       "Arithmetic error"
,       "Canceled by user"   /* not used, replaced by RUN_BREAKED */
,       "Table/query/view doesn't exist"
,       "Out of memory for dynamic allocations"
,       "No right to open the project nor run the program"
,       "Project referenced by the object has been changed"
,       "Type cannot be converted"
,       "Array index out of range"
,       "ActiveX method call failure"
,       "Bad parameter count"
,       "Parameter type mismatch"
,       "ActiveX method call raises an exception"     
,       "Unknown method or property"
,       "Parametr could not be coerced to the specified type"
};

#endif

const char * mail_errors[] = {
  "Mail not initialized"
, "Unknown mail error"
, "Mail client is not remote"
, "Incorrect mail address type"
, "Mail logon failed"
, "Incorrect mail profile"
, "Incorrect mail user ID"
, "Incorrect mail address (relaying to this address is denied)"
, "File not found"
, "602SQL Server cannot login to the mail, because is running on system account"
, "Dialup connection with mail server failed"
, "Mail has already been initialized with different parameters"
, "Mail profile not found"
, "Mail profile is not complete"
, "Sending specified file is not allowed"
, "Socket I/O error"
, "SMTP/POP3 server not found"
, "Connection with mail server failed"
, "No more attached files"
, "Attached file not found"
, "Mailbox locked"
, "Message not found"
, "Message is corrupted or has unknown format"
, "DLL library not found"
, "DLL library function not found"
, "Requested mail service not supported"
, "Request queued, it will be sent by next phone connection"
, "Mail profile already exists"
};   

#endif
               
#ifdef ENGLISH
const char month_name[12][10] = {
"January", "February", "March", "April", "May", "June",
"July", "August", "September", "October", "November", "December" };
const char mon3_name [12][4] = {
"Jan", "Feb", "Mar", "Apr", "May", "Jun",
"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
const char day_name  [7][10] = {
"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char day3_name [7][4] = {
"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

#else
                             
const char month_name[12][10] = {
"leden", "�nor", "b�ezen", "duben", "kv�ten", "�erven",
"�ervenec", "srpen", "z���", "��jen", "listopad", "prosinec" };
const char mon3_name [12][4] = {
"ldn", "unr", "brn", "dbn", "kvt", "cen",
"cec", "srp", "zri", "rjn", "lst", "prs" };
const char day_name  [7][10] = {
"Ned�le", "Pond�l�", "�ter�", "St�eda", "�tvrtek", "P�tek", "Sobota" };
const char day3_name [7][4] = {
"Ned", "Pon", "Ute", "Str", "Ctv", "Pat", "Sob" };
#endif
