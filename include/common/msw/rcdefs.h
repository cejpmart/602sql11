// rcdefs.h - minimal definitions for compiling RC files with VC Express
#define WS_CHILD            0x40000000L
#define WS_VISIBLE          0x10000000L
#define WS_GROUP            0x00020000L
#define WS_TABSTOP          0x00010000L
#define WS_BORDER           0x00800000L
#define WS_DLGFRAME         0x00400000L
#define WS_VSCROLL          0x00200000L
#define WS_HSCROLL          0x00100000L
#define WS_SYSMENU          0x00080000L
#define WS_CAPTION          0x00C00000L     /* WS_BORDER | WS_DLGFRAME  */
#define WS_POPUP            0x80000000L

#define WS_EX_TOPMOST           0x00000008L
#define WS_EX_DLGMODALFRAME     0x00000001L
#define WS_EX_CLIENTEDGE        0x00000200L
#define WS_EX_CONTEXTHELP       0x00000400L

#define DS_MODALFRAME       0x80L   /* Can be combined with WS_CAPTION  */
#define DS_3DLOOK           0x0004L
#define DS_CONTEXTHELP      0x2000L

#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BITMAP           0x0000000EL
#define SS_CENTERIMAGE      0x00000200L
#define SS_BLACKFRAME       0x00000007L

#define ES_LEFT             0x0000L
#define ES_CENTER           0x0001L
#define ES_RIGHT            0x0002L
#define ES_MULTILINE        0x0004L
#define ES_UPPERCASE        0x0008L
#define ES_LOWERCASE        0x0010L
#define ES_PASSWORD         0x0020L
#define ES_AUTOVSCROLL      0x0040L
#define ES_AUTOHSCROLL      0x0080L
#define ES_READONLY         0x0800L

#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_HASSTRINGS        0x0040L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_STANDARD          (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

#define BS_PUSHBUTTON       0x00000000L
#define BS_DEFPUSHBUTTON    0x00000001L
#define BS_CHECKBOX         0x00000002L
#define BS_AUTOCHECKBOX     0x00000003L
#define BS_CENTER           0x00000300L
#define BS_AUTORADIOBUTTON  0x00000009L

#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L

#define IDOK                1
#define IDCANCEL            2
#define IDHELP              9
