/****************************************************************************/
/* CSMSGS.CPP - Common client and server messages                           */
/****************************************************************************/
const int warning_messages[] = {
  WAS_IN_TRANS, NOT_IN_TRANS, ERROR_IN_CONSTRS, IS_NOT_DEL, ERROR_IN_DEFVAL, WORKING_WITH_DELETED_RECORD, IS_DEL, 
  NO_BIPTR, INDEX_OOR, WORKING_WITH_EMPTY_RECORD,
  0 }; // terminated by 0


#if !defined(ENGLISH) && ((WBVERS<900) || defined(SRVR))

const char * db_errors[] = {
     "Špatný modifikátor"
   , "Nemáte právo provést tuto akci"
   , "Špatné èíslo sloupce"
   , "Záznam mimo tabulku"
   , "Systém nespustil DETACHED rutinu"
   , "Špatné èíslo kurzoru"
   , "Špatný kód operace"
   , "Kurzor tuto operaci neumožòuje"
   , "S objektem %s nìkdo pracuje, nelze zamknout"
   , "Objekt %s neexistuje"
   , "Index mimo pole"//10
   , "Požadovaný záznam v kurzoru nenalezen (02000)"
   , "Záznam je uvolnìný, operaci nelze provést"
   , "Objekt neexistuje"
   , "Uvedeno nesprávné heslo pro uživatele %s"
   , "Odkaz na zrušený záznam"
   , "Neexistující odkaz"
   , "Vyèerpána operaèní pamì"
   , "Záznam neexistuje"
   , "Záznam je zrušený"
   , "Index %s neexistuje"//20
   , "Objekt %s nenalezen"
   , "Není dost pamìti"
   , "Chybná délka dat"
   , "Neèitelný blok"
   , "Klient se dosud úspìšnì nepøihlásil na server"
   , "Vyèerpána disková pamì"
   , "Operace zrušena"
   , "Ztráta spojení se SQL serverem"
   , "Chyba pøi práci se souborem"
   , "Nekompatibilní verze"//30
   , "Server odmítá požadavek"
   , "Sloupec %s nesmí mít prázdnou hodnotu (NULL) (40002)"
   , "Klient je pøihlášen bez licence s omezenými možnostmi a snaží se provést nepovolenou operaci"
   , "Tento objekt nelze zrušit, protože na nìj odkazuje objekt %s"
   , "Interní chyba OOD"
   , "Interní chyba OVERRUN"
   , "Interní chyba PAGING"
   , "Interní chyba MISSING DEFLOCK na %s"
   , "Interní chyba OOBSTACK"
   , "Tabulka %s je vážnì poškozená"//40
   , "Nelze zamknout server"
   , "Rollback bìhem vytváøení kurzoru"
   , "Deadlock: vzájemné zablokování klientù v jádøe"
   , "Duplicita klíèù v tabulce %s: již existuje záznam se stejným klíèem (40004)"
   , "Nesouhlas verzí klienta a serveru"
   , "Porušeno integritní omezení %s databázové tabulky (40005)"
   , "Porušena referenèní integrita %s mezi tabulkami (40006)"
   , "Nepøípustný typ argumentu v agregaèní funkci"
   , "Tabulka je zcela zaplnìna"
   , "Vnoøení požadavkù na server"//50
   , "Tuto operaci nelze provést pro ODBC kurzor"
   , "Chybný argument funkce"
   , "Zadaný ODBC kurzor není otevøen"
   , "ODBC driver není schopen požadované operace"
   , "Pøíliš složitá transakce"
   , "Interní signál"
   , "Konverze/porovnání øetìzcù %s není k dispozici"
   , "Není k dispozici pøepisovací pešek"
   , "Èekám na potvrzení pøedešlého paketu"
   , "Replikace dosud blokována"//60
   , "Operaci blokuje stav pøepisovacího peška"
   , "Tabulka nemá potøebné pøíznaky"
   , "Index k tabulce %s je poškozen"
   , "Vypršela platnost hesla, musíte zadat nové"
   , "Nemáte platný klíè, nelze podepisovat"
   , "V souboru je jiný klíè"
   , "Interní chyba serveru"
   , "Kurzor je v nepøípustném stavu (24000)"
   , "Neplatná specifikace savepointu (3B001)"
   , "Pøíliš mnoho savepointù (3B002)"//70
   , "Transakce je aktivní (25001)"
   , "Nepøípustné ukonèení transakce (2D000)"
   , "Transakce je READ ONLY (25006)"
   , "Èíselná hodnota mimo pøípustný rozsah (22003)"
   , "Øetìzec znakù nelze konvertovat (22018)"
   , "Øetìzec znakù seøíznut zprava (22001)"
   , "Dìlení nulou (22012)"
   , "Nesprávná velikost odpovìdi na dotaz (21000)"
   , "Chybný øetìzec znakù za ESCAPE (22019)"
   , "V pøíkazu CASE nenalezena vìtev (20000)"//80
   , "Uživatelská výjimka `%s` není ošetøena (45000)"
   , "Pøíkaz RESIGNAL proveden mimo handler (0K000)"
   , "Externí knihovna nebo funkce nenalezena (38001)"
   , "Ve funkci nebyl proveden pøíkaz RETURN (2F001)"
   , "Sloupec %s není editovatelný"
   , "Chyba v triggeru (09000)"
   , "Replikování není zapnuto"
   , "Chyba pøi posílání paketu"
   , "Neplatné jméno kurzoru (34000)"
   , "Role je z jiné aplikace než objekt"
   , "Vyèerpány všechny hodnoty ze sekvence %s"
   , "Nelze volat CURRVAL pøed NEXTVAL v sekvenci %s"
   , "Server nemá licenci pro pøístup z WWW"
   , "Vyèerpány licence pro pøístup klientù"
   , "Knihovna je umístìna v adresáøi, z nìhož není dovoleno spouštìt externí funkce"
   , "Knihovna %s obsahující externí funkci nebyla nalezena"
   , "Jazyková podpora %s není instalována"
   , "Nemohu konvertovat tento typ dokumentu (%s)"
   , "Server nemá licenci na fulltextové nástroje"
   , "Server bìží s právy roota, odmítám používat externí funkce"
   , "Není co replikovat"
   , "Objekt byl zmìnìn"
   , "Tabulka %s nemá unikátní klíè pro replikace"
   , "Vyèerpány rámy na diskové bloky, nutno zvìtšit vlastnost `Frame Space`"
   , "Nemáte právo konfigurovat databázi nebo øídit provoz"
   , "Nemáte právo nastavovat bezpeènostní parametry"
   , "Použití tohoto uživatelského jména je zakázáno"
   , "Toto schéma je otevøeno jiným klientem nebo vláknem a nelze jej proto zrušit"
   , "Chybný argument funkce"
   , "Server nemá licenci na XML rozšíøení"
   , "Pøekroèen limit %s"
   
   , "Neznámá chyba"
};

const char * db_warnings[] = {
     "Zahájení transakce provedeno uvnitø transakce"
   , "Ukonèení transakce provedeno mimo transakci"  
   , "Integritní omezení na tabulce je chybné" 
   , "Záznam nelze obnovit protože není zrušený"  
   , "Implicitní hodnota sloupce je chybná" 
   , "Práce se zrušeným záznamem"
   , "Záznam nelze zrušit protože není platný"  
   , "K obousmìrnému ukazateli neexistuje protismìrný ukazatel" 
   , "Index pøekraèuje poèet hodnot multiatributu" 
   , "Ignoruji záznam zrušený jiným klientem"
   , "?"
};

const char not_answered[] = "Èekám na odpovìï od serveru";

const char * compil_errors[] = {
 "Pøeklad bez chyb",
 "Bìh programu skonèil",
 "Bìh programu skonèil",
 "Neuzavøený komentáø",
 "Øetìzec znakù pøesahuje øádku",
 "Rekurze na lokálních rutinách není povolena",
 "Oèekávám typ",
 "Oèekávám levou hranatou závorku",
 "Oèekávám pravou hranatou závorku",
 "Oèekávám dvì teèky",
 "Oèekávám celé èíslo",//860
 "Oèekávám 'of'",
 "Chyba v mezích pole",
 "Pole je pøíliš velké",
 "Oèekávám identifikátor",
 "Oèekávám rovnítko",
 "Nepøípustný znak ve zdrojovém textu",
 "Oèekávám èárku",
 "Oèekávám støedník nebo 'end'",
 "Oèekávám støedník",
 "Oèekávám deklaraci nebo 'begin'",  //870
 "Podprogramy nelze vnoøovat",
 "Oèekávám levou závorku",
 "Oèekávám pravou závorku",
 "Oèekávám dvojteèku",
 "Identifikátor %s oznaèuje nepøípustný objekt",
 "Výraz musí být typu boolean",
 "Oèekávám 'do'",
 "Oèekávám ':='",
 "Zde musí být èíslo",
 "Musí být celoèíselného typu", //880
 "Nepøípustná operace",
 "Typy nejsou kompatibilní",
 "Peníze nelze navzájem násobit",
 "Není typu záznam",
 "Není to ukazatel",
 "Není typu pole",
 "Identifikátor %s není deklarován",
 "Oèekávám identifikátor složky záznamu",
 "Index musí být celoèíselného typu",
 "Oèekávám 'then'",//890
 "Oèekávám èárku nebo dvojteèku",
 "Pøíliš mnoho parametrù",
 "Oèekávám teèku",
 "Èíslo je pøíliš velké",
 "Oèekávám pøíkaz",
 "Pøíliš rozsáhlé promìnné",
 "Pøíliš velká promìnná",
 "Chyba na konci",
 "Identifikátor %s deklarován dvakrát",
 "Funkce musí vracet jednoduchý typ nebo øetìzec",//900
 "Chyba ve výstupu kódu - není dost pamìti?",
 "Chyba na konci",
 "Vnoøený pøístup do databáze",
 "Chybná délka",
 "Chybný index",
 "Typ není strukturovaný",
 "Pohled neexistuje",
 "Chyba pøi ètení definice z databáze",
 "Není dost pamìti",
 "Oèekávám 'to' nebo 'downto'",//910
 "Nedefinovaný identifikátor %s",
 "Oèekávám faktor",
 "Dotaz %s neexistuje",
 "Nepøípustný argument agregaèní funkce",
 "Chyba v konstantì typu TIME",
 "Chyba v konstantì typu DATE",
 "Chybné databázové pøiøazení",
 "Není to sloupec promìnné velikosti",
 "Tabulka %s neexistuje",
 "Chybný argument pøíkazu ACTION",//920
 "Oèekávám obrácené lomítko",
 "Oèekávám klíèové slovo UNTIL",
 "Nesprávný projekt",
 "Oèekávám klíèové slovo END",
 "Nespecifická chyba v definici pohledu",
 "Oèekávám øetìzec znakù",
 "Nespecifická chyba",
 "Èíslo mimo povolený rozsah",
 "Pøíliš složitý pohled",
 "Oèekávám klíèové slovo definice",//930
 "Špatný identifikátor",
 "Chybné poøadí skupin",
 "Pøíliš mnoho složek pohledu",
 "Oèekávám složku pohledu",
 "Chyba v definici dotazu",
 "Pøíliš mnoho složek menu",
 "Include neexistuje",
 "Nelze osamostatnit lokální interní proceduru nebo standardní funkci",
 "Duplicita identifikací složek pohledu",
 "Pøed ELSE nesmí být støedník",//940
 "Pøíliš mnoho sloupcù",
 "Pøíliš mnoho indexù k tabulce",
 "Není definován žádný sloupec",
 "Pro tento typ sloupce nelze použít []",
 "Duplicita jmen omezení v návrhu tabulky",
 "Index má pøíliš mnoho èástí, maximum je 8",
 "V indexu je sloupec nepøípustného typu nebo multiatribut",
 "Klíè indexu je pøíliš dlouhý",
 "Chyba v syntaxi definice tabulky resp. indexu",
 "Index je pøíliš složitý",//950
 "Pøíliš mnoho tabulek v dotazu",
 "Oèekávám FROM",
 "Oèekávám jméno tabulky",
 "Chyba v syntaxi dotazu",
 "Chyba v predikátu",
 "Oèekávám SELECT",
 "Oèekávám FROM nebo èárku",
 "Klauzule WHERE ani GROUP BY nesmí obsahovat agregaèní funkci",
 "Nelze vnoøovat agregaèní funkce",
 "Hvìzdièku lze použít pouze ve funkci COUNT(*)",//960
 "Chybná syntaxe v jazyce SQL",
 "Po BETWEEN oèekávám slovo AND",
 "Oèekávám OUTER",
 "Oèekávám JOIN",
 "Chyba v klauzuli FROM",
 "Po ORDER resp. GROUP oèekávám slovo BY",
 "Chybné jméno typu",
 "Syntaktická chyba v popisu tabulky",
 "Po PRIMARY resp. FOREIGN oèekávám slovo KEY",
 "Struktura kurzoru není známa",//970
 "Oèekávám REFERENCES",
 "Identifikátor %s nenalezen",
 "Po DOUBLE oèekávám slovo PRECISION",
 "Oèekávám slovo NULL",
 "Toto není pøíkaz jazyka SQL",
 "Oèekávám TO",
 "Oèekávám ON",
 "Oèekávám INTO",
 "Oèekávám VALUES",
 "Argument agregaèní funkce je nepøípustného typu",//980
 "Oèekávám SET",
 "Kurzor není editovatelný nebo vznikl z více tabulek.\nNelze rušit záznamy.",
 "Kurzor není editovatelný\nNelze modifikovat jeho obsah",
 "Uvedeno pøíliš mnoho hodnot v pøíkazu INSERT",
 "Dotazy spojené UNION/EXCEPT/INTERSECT mají nekompatibilní seznamy sloupcù",
 "Dotazy spojené UNION/EXCEPT/INTERSECT mají pøíliš rozdílné seznamy sloupcù",
 "Neplatné èíslo pozice v klauzuli ORDER BY",
 "Agregaèní funkce nelze použít v ORDER BY, nahraïte jménem sloupce (AS)",
 "Oèekávám UPDATE",
 "Oèekávám NOT",//990
 "Chyba v referenèní integritì, nadøazená tabulka neobsahuje vhodný index",
 "Duplicita jmen sloupcù v popisu tabulky",
 "Tabulka nemùže mít dva primární klíèe",
 "Oèekávám INDEX",
 "Oèekávám AS",
 "Ke sloupci nemáte právo ètení",
 "Tabulka neobsahuje vhodný index",
 "Chyba v pøíkazu SELECT ve složce combo",
 "Podprogram je pøíliš velký",
 "Kurzor není editovatelný nebo vznikl z více tabulek.\nNelze do nìj vkládat.",//1000
 "Chyba v implicitní hodnotì sloupce",
 "Ke sloupci nemáte právo pøepisu",
 "Rekurze v definici dotazu",
 "Nepøípustný znak v literálu",
 "Neuzavøena pøedsunutá deklarace (FORWARD)",
 "Neuzavøena deklarace ukazatele",
 "Neexistuje spoleèný sloupec",
 "Nemáte právo odkazovat na sloupec %s",
 "Subdotaz nesmí být uvnitø agregaèní funkce",
 "Oèekávám IN",//1010
 "Oèekávám prvek data nebo èasu",
 "Oèekávám konstantu typu Boolean",
 "NOT není zde pøípustné",
 "Subdotaz není skalární",
 "Tento pøíkaz není dovolen v ATOMIC bloku",
 "Oèekávám IF",
 "Oèekávám LOOP",
 "Oèekávám RETURNS",
 "Oèekávám PROCEDURE",
 "Oèekávám klíèový parametr",//1020
 "Stejný parametr specifikován dvakrát",
 "Nevhodný druh parametru",
 "Pøíkaz RETURN musí být uvnitø funkce",
 "Nelze mìnit hodnotu konstanty",
 "Konstantní promìnná musí mít specifikovanou hodnotu",
 "Oèekávám SQLSTATE",
 "Oèekávám FOR",
 "Chyba ve formátu SQLSTATE",
 "Oèekávám FOUND",
 "Interní chyba kompilátoru",//1030
 "Nepøítustný objekt v klauzuli INTO",
 "Oèekávám HANDLER",
 "REDO nebo UNDO handler lze použít pouze v atomickém pøíkazu",
 "Oèekávám REPEAT",
 "Oèekávám WHILE",
 "Oèekávám CURSOR",
 "Oèekávám SAVEPOINT",
 "Oèekávám CHAIN",
 "Oèekávám TRANSACTION",
 "Dvojí specifikace téhož údaje",//1040
 "Specifikace jsou ve vzájemném rozporu",
 "Oèekávám CASE",
 "Tato operace není dovolena pro kurzor z pøíkazu FOR",
 "Požadavek SENSITIVE je v rozporu s definicí kurzoru",
 "Agregaèní funkce mimo kontext",
 "Oèekávám NAME",
 "Tento typ není pøípustný",
 "Oèekávám OBJECT",
 "Oèekávám BEFORE nebo AFTER",
 "Oèekávám INSERT, DELETE nebo UPDATE",//1050
 "Oèekávám OLD nebo NEW",
 "Transientní tabulky nejou podporovány",
 "Oèekávám EACH",
 "Oèekávám ROW nebo STATEMENT",
 "Oèekávám TRIGGER",
 "Pøiøazení do nepøípustného objektu",
 "Tento rys SQL není podporován",
 "Za * nesmí následovat další sloupec dotazu",
 "Nelze vytvoøit promìnnou typu formuláø beze jména (instanci abstraktní tøídy)",
 "Tento objekt vyžaduje projekt %s",
 "Tento druh složky pohledu nemá vlastnost nebo metodu %s",
 "Metoda ani vlastnost %s neexistuje a složky formuláøe nejsou známy",
 "THISFORM není v tomto kontextu definováno",
 "Nedefinována direktiva preprocesoru",
 "Konec souboru v rozvoji makra",
 "Tato direktiva není zde pøípustná",
 "Konec souboru uvnitø podmínìného pøekladu zapoèatého na øádce %s",
 "Toto je chránìný objekt. Nelze do nìj nahlédnout.",
 "Typelib error",
 "Oèekávám WITH", // 1070
 "Konflikt v parametrech sekvence",
 "Syntaxe neopovídá nastaveným pøíznakùm kompatibility",
 "V uložené proceduøe a triggeru nelze používat promìnné klienta ani dynamické parametry",
 "Pro tuto chybu nelze definovat handler",
 "V modulu globálních delarací nelze definovat rutiny ani kurzory",
 "Tuto akci smí provést pouze autor aplikace",
 "Oèekávám EXISTS",
 "Pøíliš složitý dotaz",
 "Debugger nemùže vyhodnotit funkci",
 "Syntaktická chyba v klauzuli LIMITS",
 "Label smí být pouze uvnitø bloku BEGIN...END"
 };

const char * kse_errors[] = {
  "Chyba inicializace serveru"
, "Nelze otevøít okno serveru pod Windows"
, "Falešná identita SQL serveru"
, "Neshoda klienta a serveru na šifrování komunikace"
, "Klientovi se nepodaøilo spustit lokální SQL server"
, "Chyba pøi alokaci pamìti"
, "Databázový soubor byl konvertován pro CD-ROM a nebyl poté korektnì instalován"
, "Server nelze spustit, jeho jméno není registrováno"
, "Databázový soubor je vážnì poškozen"
, "Platforma nepodporuje systémový jazyk databazového souboru"
, "Pøístup na server je doèasnì uzamèen"//10
, "Verze databázového souboru neodpovídá verzi serveru"
, "Nelze inicializovat zvolený síový protokol"
, "Verze serveru není kompatibilní s verzí klienta"
, "Server nebyl nalezen v síti ani pro nìj není specifikována platná IP adresa"
, "Nepodaøilo se navázat spojení s SQL serverem"
, "Pøekroèeno maximum spojení mezi klienty a serverem"
, "Interní chyba spojení"
, "Server nemá licenci pro síový pøístup"
, "HTTP tunel není zapnut na serveru (na stejném portu bìží web XML interface)"
, ""//20
, ""
, "Není dost øídicích blokù NetBIOS"
, "Nad stejnou databází již pracuje jiný server nebo nedostatek práv k db souboru"
, "V síti již bìží databázový server stejného jména "
, "Nelze spustit další vlákno na serveru"
, "Nelze vytvoøit synchonizaèní objekt"
, "Nedaøí se mapovat pamìový soubor"
, ""
, "Lokální server neodpovìdìl v èasovém limitu"
, "Knihovna TCP/IP socketù nefunguje"//30
, "Chyba pøi práci se socketem"
, ""
, ""
, "Nelze pøidat jméno pro NetBIOS\n(32-bitový klient na stejném stroji jako server?)"
, "Nenalezen SOCKS server"
, "Chyba pøi komunikaci se SOCKS serverem"
, "SOCKS server (firewall) odmítl vytvoøit spojení na SQL server"
, "Zadáno chybné heslo k databázovému souboru"
, "Upuštìno od startu serveru"
, "Protokol IPX není instalován"//40
, ""
, "Server nemùže vytvoøit databázový soubor"
, "Server nemùže otevøít databázový soubor"
, "Server nemùže otevøít transakèní soubor"
, "Pøístup z této IP adresy na server není povolen"
, ""
, "Instalaèní klíè pro databázi není uveden nebo není platný"
, "SQL server nemá licenci pro provoz"
, "Neznámá chyba"
};

#ifndef SRVR
const char * client_errors[] = {
  "Knihova %s, generující èárový kód, nebyla nalezena"
, "Zprávu o délce %d nelze vyjádøit tímto druhem èárového kódu"
, "Zpráva %s obsahuje znak, který není pøípustný v tomto druhu èárového kódu"
, "Knihovna generující èárový kód ohlásila chybu %d. Pravdìpodobnì nepøípustné hodnoty vlastností."
, "Složka èíslo %d nemá definovaný žádný obsah"
, "Poškozena správa pamìti, ukonèuji program"
, "Nezdaøilo se rozšíøení pracovní pamìti klienta"
, "Klient rozšiøuje svoji pracovní pamì"
, "Složka DTP nebyla vytvoøena protože její obsah není typu datum, èas ani timestamp"
, "Složka Kalendáø nebyla vytvoøena, protože její obsah není typu datum ani timestamp"
, "Knihovna 602dvlp8.DLL obsahující návrháøe nebyla nalezena"
, "Knihovna 602wed8.DLL pro návrh WWW objektù nebyla nalezena"
, "Chybná verze knihovny 602dvlp8.DLL"
, "Chybná verze knihovny 602wed8.DLL"
, "Složku nelze vytvoøit protože není implementována v této verzi COMCTL32.DLL"
, "Subformuláø neotevøen protože není zadáno jeho jméno"
, "Nelze otevøít subformuláø se stejným jménem %s jako má hlavní formuláø"
, "Relace %s obsahuje jména sloupcù nevyskytujících se v kurzorech pohledù"
, "Tuto operaci nelze provést na fiktivním záznamu"
, "Anonymní uživatel nemùže podepisovat"
, "Nelze exportovat chránìný objekt bez zašifrování"
, "Nelze zanoøovat volání popup menu"
, "Selhání cache objektových descriptorù"
, "%s nelze použít v obecném formuláøi"
, "%s nelze použít ve složce standardního formuláøe"
, "%s nelze použít v zavøeném formuláøi"
, "Pohled %s nenalezen"
, "Knihovna obsahující podporu XML nebyla nalezena"
, "Chybná verze knihovny s podporou XML"
, "Objekt nelze pøejmenovat"
, "Nelze pøejmenovat tabulku, na kterou jsou odkazy v referenèní integritì"   
, "Nenalezen MEMO soubor"
, "Chyba konverze dat na výstupním záznamu èíslo %s."
};

const char * ipjrt_errors[] = {
       "Nedefinovaná instrukce (poškozený kód)"
,      "Pøeteèení zásobníku (pøíliš složitý výraz)"
,      "Funkce %s nenalezena v DLL"
,      "DLL knihovna %s nebyla nalezena"
, ""
, ""
,      "Ukazatel má hodnotu NIL"
,      "Nespecifická chyba"
,      "Sloupec neexistuje"
,      "Vyèerpána bìhová pamì"
,      "Aritmetická chyba"
,      "Pøerušeno uživatelem"   /* not used, replaced by RUN_BREAKED */
,      "Tabulka/dotaz/pohled neexistuje"
,      "Vyèerpána pamì pro dynamické alokace"
,      "Nemáte právo nastavit projekt ani spustit program"
,      "Objekt odkazuje na projekt, který byl zmìnìn."
,      "Typy nelze konvertovat"
,      "Pøekroèena horní mez pole"
,      "Chyba pøi volání metody ActiveX objektu"
,      "Chybný poèet parametrù"
,      "Chybný typ parametru"
,      "Volání metody zpùsobilo výjimku"     
,      "Neznámá metoda nebo vlastnost"
,      "Pøeteèení pøi konverzi parametru"
};

#endif

const char * mail_errors[] = {
  "Pošta není inicializována"
, "Neznámá chyba pošty"
, "Poštovní klient není vzdálený"
, "Chybný typ adresy"
, "Chyba pøi pokusu o pøihlášení"
, "Chybný profil pošty"
, "Chybné ID uživatele"
, "Chybná adresa (poštovní server neumožòuje odeslání zásilky na tuto adresu)"
, "Soubor nenalezen"
, "Server se nemùže pøihlásit do pošty, protože bìží na systémový úèet"
, "Nepodaøilo se navázat telefonické spojení s poštovním serverem"
, "Pošta už je inicializována s jinými parametry"
, "Profil pošty nenalezen"
, "V profilu chybí parametr"
, "Odeslání zadaného souboru není povoleno"
, "I/O chyba pøi síové komunikaci"
, "SMTP/POP3 server nenalezen"
, "Nepodaøilo se navázat spojení s poštovním serverem"
, "V zásilce nejsou další pøipojené soubory"
, "Pøipojený soubor nenalezen"
, "Schránka je zamèená"
, "Zásilka nenalezena"
, "Zásilka je poškozená nebo má neznámý formát"
, "Nenalezena DLL knihovna"
, "Nenalezena funkce v DLL knihovnì"
, "Požadovaná poštovní služba není podporována"
, "Požadavek byl zaøazen do fronty, bude odeslán pøi pøíštím navázání telefonického spojení"
, "Poštovní profil už existuje"
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
"leden", "únor", "bøezen", "duben", "kvìten", "èerven",
"èervenec", "srpen", "záøí", "øíjen", "listopad", "prosinec" };
const char mon3_name [12][4] = {
"ldn", "unr", "brn", "dbn", "kvt", "cen",
"cec", "srp", "zri", "rjn", "lst", "prs" };
const char day_name  [7][10] = {
"Nedìle", "Pondìlí", "Úterý", "Støeda", "Ètvrtek", "Pátek", "Sobota" };
const char day3_name [7][4] = {
"Ned", "Pon", "Ute", "Str", "Ctv", "Pat", "Sob" };
#endif
