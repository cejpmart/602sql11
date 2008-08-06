#ifdef WINS
#define SYSTRAY_NO_TASKBAR  // replaces taskbar icon by the systray icon
/* Problems: MSW: Alt-Tab shows empy icon for this window in the list of running tasks
             GTK: The window cannot be minimized */
#endif

#include <wx/taskbar.h>
class MngFrame;

class MngTaskBarIcon : public wxTaskBarIcon
{
  void OnLeftClick(wxTaskBarIconEvent & event);
  void OnRestoreCmd(wxCommandEvent & event);
  void OnCloseCmd(wxCommandEvent & event);
  wxMenu * CreatePopupMenu(void);
  DECLARE_EVENT_TABLE()
  MngFrame * owner;
 public:
  MngTaskBarIcon(MngFrame * ownerIn) : owner(ownerIn) { }
};

class MngFrame : public wxFrame
{ void OnIconize(wxIconizeEvent & event);
  DECLARE_EVENT_TABLE()
 public:
#ifdef SYSTRAY_NO_TASKBAR
  MngTaskBarIcon tbi;
#endif
  MngFrame(void);
};

class MngApp: public wxApp
{ 
 public:
  MngFrame *frame; // not owning
  bool OnInit(void);
};

DECLARE_APP(MngApp);

struct t_server_info
{ tobjname name;
  bool running;
  cdp_t cdp;  // NULL iff noc connected
  char service_name[100+1];  // "" if not defined
  char addr[100+1];  
  unsigned long port;
  void get_server_state(void);
};

extern t_server_info * loc_server_list;
extern int server_count;
