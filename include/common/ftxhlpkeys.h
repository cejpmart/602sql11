#ifndef __ftxhlpkeys_h__
#define __ftxhlpkeys_h__

/* Names of regdb keys and values for fulltext helpers.
 * Windows: parameters are stored in regdb
 * Linux: in /etc/602sql */

// regdb key (win)
#define FTXHLP_KEY "Fulltext Helpers"
// subsection name (linux)
#define FTXHLP_SECTION ".Fulltext Helpers"

// regdb value names (win) or INI key names (linux)
#define FTXHLP_VALNAME_USEOOO "Use OpenOffice.org"
#define FTXHLP_VALNAME_OOOBINARY "OpenOffice.org Binary"
#define FTXHLP_VALNAME_OOOHOSTNAME "OpenOffice.org Hostname"
#define FTXHLP_VALNAME_OOOPORT "OpenOffice.org Port"
#define FTXHLP_VALNAME_OOOPROGRAMDIR "OpenOffice.org Program Directory"
#define FTXHLP_VALNAME_DOC "DOC Helper"
#define FTXHLP_VALNAME_XLS "XLS Helper"
#define FTXHLP_VALNAME_PPT "PPT Helper"
#define FTXHLP_VALNAME_RTF "RTF Helper"

// values
#define FTXHLP_HELPER_ANTIWORD "antiword"
#define FTXHLP_HELPER_CATDOC "catdoc"
// OOo have two possible values
#define FTXHLP_HELPER_OOO "ooo"
#define FTXHLP_HELPER_OOO_ALT "OpenOffice.org"
#define FTXHLP_HELPER_XLS2CSV "xls2csv"
#define FTXHLP_HELPER_CATPPT "catppt"
#define FTXHLP_HELPER_RTF2HTML "rtf2html"

#endif

