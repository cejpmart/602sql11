enum { NO_WORD_ID = 0xffffffff };
enum { NO_DOC_ID  = (t_docid)-1 };
enum { FT_MAX_SUFFIX_LEN = 20 };

#define FT_WORD_STARTER(c) ((c)>='0' && (c)<='9' || (c)>='@' && (c)<='Z' || (c)>='a' && (c)<='z' || (unsigned char)(c)>=128 || (c)=='_')
#define FT_WORD_CONTIN(c)  ((c)>='0' && (c)<='9' || (c)>='@' && (c)<='Z' || (c)>='a' && (c)<='z' || (unsigned char)(c)>=128 || (c)=='_' || (c)=='-')
#define FT_WORD_WHITESPACE(c) ((c)==' ' || (c)=='\t' || (c)=='\v' || (c)=='\r' || (c)=='\n' || (c)=='\f' || (c)=='\b')

BOOL init_lemma(int language); // returns TRUE iff OK
void lemmatize(int language, const char * input, char * output, int output_bufsize);
CFNC DllKernel BOOL WINAPI simple_word(const char * word, int language);
extern const char lpszMainDictName[NUMBER_OF_LANGUAGES][13];

#define FTFLAG_FIRSTUPPER  1
#define FTFLAG_ALLUPPER    2

#ifdef WINS
/********************************************************************
*                   Interface definition for the                    *
*                           W5_XIMP.DLL                             *
*                             library                               *
*                                                                   *
*  This library serves for import, indexing and recognition         *
*  of the following file formats:                                   *
*                                                                   *
*    WPD ... 16/32-bit WinText, 602Text documents                   *
*    DOC ... Word 6.x, 95, 97, 2000 documents                       *
*    DOT ... Word 6.x, 95, 97, 2000 document templates              *
*    602 ... T602 DOS word processsor files                         *
*    TXT ... ASCII text files                                       *
*    TXT ... UTF16 UNICODE text files                               *
*    HTML .. HyperText Markup Language (HTML) and XHTML files       *
*    XML ... eXtended Markup Language (XML) files                   *
*                                                                   *
*  This library uses other libraries for importing specific formats.*
*  These DLLs are:                                                  *
*    W5_XW97.DLL  ... for DOC and DOT formats                       *
*    W5_XRTF.DLL  ... for the RTF format                            *
*    W5_XHTML.DLL ... for HTML and XHTML formats                    *
*                                                                   *
********************************************************************/
//-------------------------------------------------------------------
//               Interface types definitions
//-------------------------------------------------------------------                                                                                   enum TFileType {
enum TFileType {
  ftUnknown = 0,  // Unknown file type
  ftNonExistant,  // File doesn't exist or the user rights don't permit read access
  ftDirectory,    // File is a directory

  ftEmpty,        // An empty, zero-length file
  ftHTML,         // Some version of HTML
  ftXML,          // XML
  ftXHTML,        // XHTML
  ftCSS,          // Cascading style sheets
  ftWPD16,        // old 16-bit WinText602
  ftWPD32,        // 32-bit WinText602 or 602Text
  ftWPD32Macro,   // 32-bit WinText602 or 602Text macro file
  ftT602,         // The old good T602
  ftWord6,        // Word 6.x and Word 95
  ftWord8,        // Word 97, 2000 and higher
  ftRTF,          // Rich Text Format (Microsoft Rich Text Format)
  ftASCII,        // Pure ASCII
  ftUTF16,        // Unicode in 16-bit encoding
  ftUTF16Swap,    // Unicode in 16-bit encoding, BOM has swapped bytes
  ftWPD16OldTemplate,// old 16-bit WinText602 template
  ftWPD16Old,     // old non OLE storage file WinText602 files
  ftCHeader,      // C header file
  ftCSource,      // C source file
  ftCPPHeader,    // C++ header file
  ftCPPSource,    // C++ source file
  ftMpegAudio,    // Mpeg audio stream data
  ftMpegSystem,   // Mpeg system stream data
  ftMpegVideo,    // Mpeg video stream data
  ftFliAnimation, // FLI animation format
  ftFlcAnimation, // FLC animation format
  ftSgiMovie,     // Silicon Graphics movie file
  ftQuickTime,    // Apple QuickTime movie file
  ftSoftwareToolsArchive, // Software Tools format archive text
  ftARC,          // ARC archive
  ftZOO,          // Rahul Dhesi's zoo archive format
  ftZIP,          // zip archive file
  ftLHarc,        // Lharc 1.x, 2.x archive
  ftLHa,          // Lha 1.x, 2.x archive
  ftRAR,          // RAR archiver
  ftJAM,          // JAM Archive volume format
  ftSQUISH,       // SQUISH archiver
  ftUC2,          // UC2 archiver
  ftARJ,          // ARJ archive data
  ftTAR,          // POSIX tar archive or GNU tar archive
  ftCompress,     // compress'd data
  ftSIT,          // StuffIt Archive
  ftCompressH,    // SCO compress -H (LZH) data
  ftSunNextAudio, // Sun/NeXT audio data file
  ftDecAudio,     // DEC audio data file
  ftCreativeMIDI, // Standard MIDI data file (Creative labs)
  ftCreativeCMF,  // Creative Music (CMF) data (Creative labs)
  ftCreativeSBI,  // SoundBlaster instrument data
  ftCreativeVoice,// Creative Voice File
  ftMultiTrack,   // MultiTrack sound data
  ftWAV,          // Microsoft WAVE format (RIFF)
  ftAVI,          // Microsoft AVI == Audio Video Interleave (RIFF)
  ftACON,         // RIFF Animated Cursor
  ftPAL,          // RIFF palette
  ftDIB,          // Microsoft Device Independent Bitmap (DIB)
  ftRiffMidi,     // Microsoft MIDI file (RIFF)
  ftRiffNiff,     // Notation Interchange File Format
  ftMOV,          // Microsoft multimedia movie file (RIFF)
  ftEMD,          // Extended MOD format (*.emd)
  ftRAD,          // Real Audio (Magic .ra\0375)
  ftMultiTracker, // MultiTracker Module sound file
  ftComposer669,  // Composer 669 Module sound data
  ftModuleSound,  // Module sound data
  ftMOD,          // Module: Extended Module sound data
  ftULT,          // ULT(imate) Module sound data
  ftSCRM,         // ScreamTracker III Module sound data
  ftBZIP,         // bzip compressed data
  ftGZIP,         // GNU ZIP (GZIP) compressed data
  ftPack,         // packed data
  ftSqueez,       // squeezed data (CP/M, DOS)
  ftCrunch,       // crunched data (CP/M, DOS)
  ftFreeze,       // frozen file 2.1 or frozen file 1.0 (or gzip 0.5)
  ftFAX,          // group 3 fax data
  ftChiWriter,    // ChiWriter file
  ftAIFF,         // AIFF audio (IFF format family)
  ftAIFF_C,       // AIFF-C compressed audio (IFF format family)
  ftSVX,          // 8SVX 8-bit sampled sound voice (IFF format family)
  ftSAMP,         // SAMP sampled audio (IFF format family)
  ftILBM,         // ILBM interleaved image (IFF format family)
  ftRGBN,         // RGBN 12-bit RGB image (IFF format family)
  ftRGB8,         // RGB8 24-bit RGB image (IFF format family)
  ftRGB2D,        // DR2D 2-D object (IFF format family)
  ftTDDD,         // TDDD 3-D rendering (IFF format family)
  ftFTXT,         // FTXT formatted text (IFF format family)
  ftPBM,          // PBM image text
  ftPGM,          // PGM image text
  ftPPM,          // PPM image text
  ftNIFF,         // NIFF image data
  ftTIFF,         // TIFF image data
  ftPNG,          // PNG image data
  ftGIF,          // GIF image data
  ftMIFF,         // MIFF image data
  ftArtisan,      // Artisan image data
  ftFIG,          // FIG image text
  ftPHIGS,        // PHIGS clear text archive
  ftSunPHIGS,     // SunPHIGS
  ftGKS,          // GKS Metafile
  ftCGM,          // (clear/binary/character) text Computer Graphics Metafile
  ftMGR,          // MGR bitmap
  ftFBM,          // FBM image data
  ftJPEG,         // JPEG image data
  ftJPEG_HSI,     // JPEG image data, HSI proprietary
  ftJFIF,         // JPEG image data (conforming to the JFIF standard)
  ftBMP_OS2,      // PC bitmap data (OS/2 1.x or 2.x)
  ftBMP,          // PC bitmap data (MS Windows)
  ftICO,          // PC icon or color icon data
  ftPointer,      // PC pointer or color pointer image data
  ftXPM,          // X pixmap image text
  ftCMU,          // CMU window manager raster image data
  ftRLE,          // RLE image data
  ftPhotoCD,      // Kodak Photo CD image pack file or overview pack file
  ftFITS,         // FITS image data
  ftIFF,          // iff image data
  ftFWS,          // Macromedia Flash data
  ftEXE_DOS,      // MS-DOS executable (EXE)
  ftEXE_Windows,  // OS/2 or MS Windows
  ftPS,           // PostScript document
  ftPDF,          // Adobe Acrobat document
  ftREG,          // Windows NT registry file
  ftDVI,          // TeX DVI file
  ftTeXGenericFont,// TeX generic font data
  ftTeXPackedFont,// TeX packed font data
  ftTeXVirtualFont,// TeX virtual font data
  ftTeXTranscriptText,// TeX transcript text
  ftTeXMetafont,  // METAFONT transcript text
  ftTeXFontMetric,// TeX font metric data
  ftPgpKeyPublicRing,// PGP key public ring
  ftPgpKeySecurityRing,// PGP key security ring
  ftPgpEncryptedData,// PGP encrypted data
  ftPgpArmoredData,// PGP armored data
  ftJavaBytecode, // Java bytecode
  ftGimpGradient, // GIMP gradient data
  ftGimpImage,    // GIMP XCF image data
  ftGimpPattern,  // GIMP pattern data
  ftGimpBrush,    // GIMP brush data
  ftWP,           // WordPerfect document
  ftZyXEL,        // ZyXEL voice data
  ftBinHex,       // BinHex binary text
  ftUUEncode,     // uuencoded or xxencoded text
  ftBTOA,         // btoa'd text
  ftShip,         // ship'd binary text
  ftBEncode,      // bencoded News text
  ftLotus123,     // Lotus 1-2-3
  ftWinHelp,      // MS Windows Help Data
  ftWMF,          // Microsoft Windows Metafile (Graphics)
  ftEMF,          // Microsoft Windows Enhanced Metafile (Graphics)
  ftMSP,          // Microsoft Paint (Graphics)
  ftTGA,          // Truevision Targa (Graphics)
  ftPCX,          // Zsoft PaintBrush (Graphics)
  ftPalmDoc,      // 3COM Palm program database file
  ftExcel5,       // Microsoft Excel 5.0
  ftWLS,          // Software602 MagicTab
  ftPowerPoint8   // Microsoft PowerPoint 97 (8.x)
};

struct SFileTypeFlags {
  BYTE isDocFile          :1;   // File is a DOCfile (OLE storage file)
  BYTE formatRecognized   :1;   // If FALSE, the type of the file was guessed
                                // only by its extension (.doc, .txt, .wpd etc.)
                                // internal format of the file was analyzed but
                                // was not recognized!
};

typedef enum
{
  kNothing,
  kAutopara,
  kMMerge,
  kFootNote,
  kPageNum,
  kChapterNum,
  kTime,
  kDate,
  kCurrPrintTime,
  kCurrPrintDate,
  kTitle,
  kSubject,
  kAuthor,
  kKeywords,
  kComments,
  kTemplate,
  kAppName,
  kFootNoteText,
  kFileName,
  kColSum,
  kRowSum,
  kHTMLMark,
  kHTMLSymbol,
  kHTMLAddr,
  kFComment,
  kSymbol,
  kEnum,
  kActIndex,
  kActTOC,
  kUserInfo,
  kMMNext,
  kFMTText,
  kFMTChkBx,
  kFMTList,
  kNumPages,
  kPage_NumPages
} FIELDTP;
#define kMaxInt       0x7FFFFFFF
typedef enum {  tNoPen,  tHairline,  tBlackHalfPt,  tBlack1Pt,  tBlack2Pt,  tBlack4Pt,  tBlack6Pt, tBlack8Pt,
  tBlack12Pt,  tThinThin,   tThinThick,  tThickThin,  tUserPen,  tPensAmb = kMaxInt } PENTYPE;
typedef enum PARAINHERITTYPE
{
  kNoInherit,
  kInheritHNL,
  kInheritRegularPara
} PARAINHERITTYPE;
typedef enum
  {
  tModeWP,                     /* Continuous view ( Word Processor ) */
  tModeDTP,                    /* Page view ( Desk Top Publishing ) */
  tModeOL,                     /* Outline View */
  tModeHTML                    // HTMLVIEW
  } DISPLAYMODE;
#define CREATEMETHODS(type, var) \
  T_CC97ARG(type xxx) {var = xxx;}; \
  T_CC97ARG& operator = (type xxx) {var = xxx; return *this;}; \
  operator type () {return var;}; \

union T_CC97ARG {
  LPVOID      p;
  LONG        l;
  WORD        w;
  BYTE        b;
  int         i;
  FIELDTP     ft;
  COLORREF    cr;
  PENTYPE     pt;
  PARAINHERITTYPE  pit;
  DISPLAYMODE    dm;

  T_CC97ARG() {p = NULL;};
  CREATEMETHODS(LPVOID, p)
  CREATEMETHODS(LONG, l)
  CREATEMETHODS(WORD, w)
  CREATEMETHODS(BYTE, b)
  CREATEMETHODS(int, i)
  CREATEMETHODS(FIELDTP, ft)
  CREATEMETHODS(COLORREF, cr)
  CREATEMETHODS(PENTYPE, pt)
  CREATEMETHODS(PARAINHERITTYPE, pit)
  CREATEMETHODS(DISPLAYMODE, dm)
  };
#else

#define CREATEMETHODS(type, var) \
	type var; \
	T_CC97ARG(type xxx) {var = xxx;}; \
	T_CC97ARG& operator = (type xxx) {var = xxx; return *this;}; \
	operator type () {return var;}; \

union T_CC97ARG {
	CREATEMETHODS(LPVOID, p)
	CREATEMETHODS(LONG, l)
	CREATEMETHODS(WORD, w)
	CREATEMETHODS(BYTE, b)
	CREATEMETHODS(int, i)
	T_CC97ARG() {p = NULL;};
};
		
#endif

typedef void * DOC_ID;    // parameter passed to the callback
typedef int FILEERROR;

enum CONVCMDS {
  C97_INVALID,
  C97_ADD_COLOR,
  C97_ADD_FACE,
  C97_ADD_STYLE,
  C97_SET_STYLE_PARENT,
  C97_SET_PARA_OVERRIDES,
  C97_SET_PARA_STYLE,
  C97_INS_CHAR_OVERRIDES,
  C97_INS_BREAK,
  C97_INS_BREAK_AT_END_OF_DOC,
  C97_INS_BREAK_AND_CHAR_OVERRIDE,
  C97_INS_BREAK_AFTER_FRAMES,
  C97_INS_ROWCOL_BREAK,
  C97_CHANGE_BREAK_TYPE,
  C97_END_TEXT_FRAME_INS_BREAK,
  C97_END_TEXT_TABLE_INS_BREAK,
  C97_SET_PAGE_LAYOUT,
  C97_INS_FIELD,
  C97_SET_SUMMARY,
  C97_INS_TEXT,
  C97_INS_TAB,
  C97_INS_HARDSPACE,
  C97_INS_HARDHYPHEN,
  C97_INS_OPTHYPHEN,
  C97_SET_SECTION_OVERRIDES,
  C97_SET_CHAPTER_OVERRIDES,
  C97_IT_IS_ALL,
  C97_INS_MARK,
  C97_INS_MARK_END,
  C97_INS_REVISION,
  C97_INS_REVISION_END,
  C97_INS_IDXTOC,
  C97_INS_IDXTOC_END,
  C97_ADD_INDEX_ENTRY,
  C97_SET_TABLE_CELL,
  C97_SET_TABLE_ROW,
  C97_INS_FOOTNOTE,
  C97_INS_FOOTNOTE_END,
  C97_CREATE_OBJ,
  C97_CREATE_OLE_OBJ,
  C97_CHG_OBJ_SETTINGS,
  C97_PLACE_OBJ,
  C97_DELETE_OBJ,
  C97_REMOVE_OBJ_ANCHOR,
  C97_ADVANCE_WORDART,
  C97_CREATE_AND_PLACE_TEXT_FRAME,
  C97_CREATE_TEXT_FRAME,
  C97_END_TEXT_FRAME,
  C97_CREATE_AND_PLACE_TEXT_TABLE,
  C97_CREATE_TEXT_TABLE,
  C97_END_TEXT_TABLE,
  C97_START_TEXT_TABLE_FORMAT,
  C97_ADD_OBJ_TO_GROUP,
  C97_SET_DOC_VIEW,
  C97_INS_POS_MARKER,
  C97_GO_POS_MARKER,
  C97_DEL_POS_MARKER,
  C97_GET_CHAPTER_NO,
  C97_GET_PARAINDEX,
  C97_GET_SECTION_PARAINDEX,
  C97_SET_FONT_AND_CHARSET,
  C97_SET_EDIT_PROT,
  C97_SET_SUMMARY_ITEM,
  C97_CHANGE_PARA_OVERRIDES,
  C97_CREATE_TEXT_FRAME_FROM_PARA,
  C97_EXCLUDE_PARA_FROM_TEXT_FRAME,
  C97_CREATE_TEXT_TABLE_FROM_PARA,
  C97_EXCLUDE_PARA_FROM_TEXT_TABLE,
  C97_E_CREATE_GROUP_SHAPE,
  C97_E_CREATE_PRIMITIVE_SHAPE_WITH_TEXT,
  C97_E_CREATE_PRIMITIVE_SHAPE,
  C97_E_SET_SHAPE_PROPERTIES,
  C97_E_GOTO_CHILD_GROUP,
  C97_E_GOTO_PARENT_GROUP,
  C97_E_GET_SHAPE_ID,
  C97_E_GOTO_SHAPE,
  C97_E_CARET_TO_SHAPE_TEXT,
  C97_E_CARET_TO_MARKED_POSITION,
  C97_SET_PARA_STYLE_INDEX,
  C97_E_CREATE_SOLITAIRE_PRIMITIVE_SHAPE,
  C97_INS_BREAK_SMART,
  C97_ACQUIRE_CONFIG_PARAMS
  };

typedef T_CC97ARG CONVERTCALLBACK97(DOC_ID npDoc, CONVCMDS iCmd, T_CC97ARG argA, T_CC97ARG argB, T_CC97ARG argC, T_CC97ARG argD);

//-------------------------------------------------------------------
//               Interface functions definitions
//-------------------------------------------------------------------
#define IMPORTFILE_FNCNAME "_ImportFile@12"

typedef FILEERROR WINAPI IMPORTFILE(DOC_ID npDoc, LPCSTR szFileName, CONVERTCALLBACK97 * callback);

#ifdef WINS
#define IMPORTFILEEX_FNCNAME "_ImportFileEx@20"

typedef FILEERROR WINAPI IMPORTFILEEX(DOC_ID npDoc, LPCSTR szFileName, CONVERTCALLBACK97 * callback,
  TFileType,            // The type of the file stated explicitly
  LPCSTR szDllPath);    // Null if you wish to search directories ".\", "Common Files\Soft602" and "Common Files\"


#define ANALYZEFILE_FNCNAME "_AnalyzeFile@8"

typedef TFileType WINAPI ANALYZEFILE(LPCSTR szFileName,    // The full path of the file to be analyzed
  SFileTypeFlags &flags); // Flags output by the function
//////////////////////
enum T_ObjAnchor {
  oanHeader,
  oanFooter,
  oanRepeatable,
  oanHeader1stPage,
  oanFooter1stPage,
  oanRepeatable1stPage,
  oanFixed,
  oanFloatYPage,
  oanFloatYPara,
  oanFloatYParaXColumn,
  oanFloatChar,
  oanFloatChar0InPara,
  oanInvalid
  };
typedef WORD TShapeID;
typedef WORD         NINDEX;
typedef NINDEX PARAINDEX;
typedef enum {  tOLT_Hidden,  tOLT_Odd,  tOLT_Even,  tOLT_All } OBJECTEVENODDTYPE;
enum REPEATABLETYPE {  tOLT_None,  tOLT_Document,  tOLT_Chapter,  tOLT_Page };
typedef enum {  tWrapNone,  tWrapAll,   tWrapTopBottom,  tWrapInLine,  tWrapAmb = kMaxInt } WRAPTYPE;
typedef enum {  tShadesNone,  tShadesDiagSWNE,        tShadesDiagNWSE,        tShadesDiagCross,       tShadesHorz,          
  tShadesVert,   tShadesCross,    tShadesDiagNarrowSw,    tShadesDiagNarrowNw,    tShadesNarrowHor,   tShadesNarrowVert,    
  tShadesNarrowHv,   tShadesBlack,  tShades250,   tShades375,   tShades500,   tShades625,   tShades750,   tShades875,           
  tShadesWhite,    tShadesLast = tShadesWhite,    tShadesAmb = kMaxInt } SHADESTYPE;
typedef enum {  tNoLink,  tLinkChart,  tLinkSpreadsheet,  tBitmap,  tMetafile,  tImportedTiff,  tLinkDummy,  tLinkText,
  tOLE,  tTable,  tGroup,  tForm,  tJPEG,  tPNG,  tGIF,  tTIFF,  tCOMPRESSED,  tDIB,  tBarCode,  tEscher } LINKTYPE;
typedef enum {
  tNoObject,
  tVertLineObject,
  tHVObject,                   // Horizontal line
  tSqreObject,                 // Square
  tCrclObject,                 // Circle
  tRcrnObject,                 // Rounded corner
  tFrame,                      // Frame
  tHeaderFrame,                // Header frame, temporary use only,
                               // cannot remain selected
  tFooterFrame,                // Footer frame, temporary use only,
                               // cannot remain selected
  tGroupObject
} OBJECTSELECTTYPE;
typedef enum {
  tSourceInvalid,
  tSourceMemory,
  tSourceDisk,
  tSourceClipboard,
  tSourceIMGhandle,
  tSourceEmpty,
  tSourceDIB,
} LINKSOURCE;
typedef class BITMAPDATA * LPBITMAPDATA;
#define MAXHTMLSTRINGS 10
#define MAXHTMLINTS     5
#define LFLAG_RUBBER            0x0200
class HTMLINFO
{
public:
  int   data[MAXHTMLINTS];
  BYTE  numStr;
  UINT  str[MAXHTMLSTRINGS];
};
typedef HTMLINFO * LPHTMLINFO;
typedef struct
{
  LONG        sInitialWidth;   // Required position
  LONG        sInitialHeight;  // Required position
  RECT        rRange;
  LINKSOURCE  linkSource;
  //HANDLE      handle;
  LPBITMAPDATA pData;
} S_LINK_PICTURE;
struct LINK
{
  LINKTYPE             linkType;
  DWORD                wFlags;
  LONG                 lLastUpdated;
  LONG                 lSize;
  LONG                 lOffset;
  union {
    //S_LINK_CHART       cht;
    //S_LINK_OLE         ole;
    S_LINK_PICTURE     pic;
    //S_LINK_SS          ss;
    //S_LINK_TEXTFRAME   oldtxt;
    //S_LINK_TEXTFRAME30 txt;
    //S_LINK_TEXTTABLE   tbl;
    //S_LINK_GROUP       grp;
    //S_LINK_FORM        form;
    //S_LINK_BARCOODE    barcode;
    //S_LINK_ESCHER      escher;
  };
  //602 PB 15.8.96 presunuto z union
  LPHTMLINFO         pHTMLinfo;
//  DWORD              A[15];
  DWORD              A[6];
};

struct OBJDEF {
  WORD             iObj;    // VYPLNUJE WINTEXT:
                            // index objektu pokud byl vytvoren nebo -1 pokud nebyl
  OBJECTSELECTTYPE sNewObject;
  T_ObjAnchor      oan;     // Popisuje ukotveni objektu
  WORD             wZOrder; // Popisuje prekryvani objektu
  TShapeID         spid;    // Shape ID pro 
  union {                   // Popisuje ukotveni objektu
    WORD           wPgNum;  // Cislo stranky ke ktere je navazan (pevne umisteny objekt)
    PARAINDEX      iPara;   // Index odstavce ke kteremu je navazan (plovouci s odstavcem)
    LPVOID         lpChov;  // Ukazatel na override na ktery je navazan (plovouci se znakem)
    struct {
      WORD              wChaptNum;     // Cislo kapitoly ke ktere je opakovaci objekt vazan
      OBJECTEVENODDTYPE eventOddType;  // Bude na sudych/lichych/vsech strankach
      REPEATABLETYPE    repeType;      // Opakovani na vsech/sudych/lichych strankach
      BOOL              excl1Page;     // Chybi zahlavi na prvni strance (ma odlisne)
      } repe;
  } anchor;

  WORD        iMasterObj;    // Master objekt pokud je odfTopAnchor a neni odfMasterTopAnchor
  long        flags;         // Priznaky udavajici ktere cleny struktury a jak jsou pouzity
  double      dContrast;     // Contrast (0 .. 100%), 50% has no effect, the default
  double      dBrightness;   // Brightness adjustment (-100% .. 100%) 0% has no effect, the default
  // Prevzato z C19_OBJECT
  RECT        extent;        // Object boundary
  RECT        wrapBorder;    // Wrap boundary
  RECT        margins;       // Used to store crop info
  LINK        linkData;      // Specification for contents
  WRAPTYPE    sWrap;         // Wrap type
  int         iYoffset;      // For floating frames
  int         iXoffset;      // For floating frames
  PENTYPE     sLine;         // Border line pen
  union {
    COLORREF    cref;        // Border line color (defined in RGB)
    BYTE        b;           // Border line color (used only if clrLineColor == 0xFFFFFFFF)
  } lineColor;
  SHADESTYPE  sShade;        // Background shade type
  union {
    COLORREF    cref;        // Color of backgroung shading (define in RGB)
    BYTE        b;           // Color of backgroung shading (used only if clrShadeColor == 0xFFFFFFFF)
  } shadeColor;
};

struct OLEOBJDEF : public OBJDEF {
  HMETAFILE hMeta;
  HENHMETAFILE hEnhMeta;
  LPVOID pOLEStream;
  BOOL bIsWordArt;
};

#endif

#define PID_TITLE         0x00000002
#define PID_SUBJECT       0x00000003
#define PID_AUTHOR        0x00000004
#define PID_KEYWORDS      0x00000005
#define PID_COMMENTS      0x00000006
#define PID_TEMPLATE      0x00000007
#define PID_LASTAUTHOR    0x00000008
#define PID_REVNUMBER     0x00000009
#define PID_EDITTIME      0x0000000A
#define PID_LASTPRINTED   0x0000000B
#define PID_CREATE_DTM_RO 0x0000000C
#define PID_LASTSAVE_DTM  0x0000000D
#define PID_PAGECOUNT     0x0000000E
#define PID_WORDCOUNT     0x0000000F
#define PID_CHARCOUNT     0x00000010
#define PID_THUMBNAIL     0x00000011
#define PID_APPNAME       0x00000012

struct TConfigParams {
  WORD wSize; // Size of this structure - for backwards compatibility
  struct {
    BYTE bIgnore602Contents;  // Don't exit prematurely if 602Text data stream found
    BYTE bIgnoreObjects;      // Don't import pictures, OLE and drawing objects
    } flags;

  TConfigParams() {
    SetDefaults();};
  void SetDefaults() {
    memset(this, 0, sizeof(*this)); wSize = (WORD)sizeof(*this);};
};
