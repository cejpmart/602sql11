// Updated helpchm.h: class is renamed, AddBook member added

#ifndef _WX_HELPCHM_H_
#define _WX_HELPCHM_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "helpchm.h"
#endif

#if wxUSE_MS_HTML_HELP

#include "wx/helpbase.h"

class WXDLLEXPORT wxCHMHelpController2 : public wxHelpControllerBase
{
public:
    wxCHMHelpController2() { }
    virtual ~wxCHMHelpController2();

    // Must call this to set the filename
    virtual bool Initialize(const wxString& file);
    virtual bool Initialize(const wxString& file, int WXUNUSED(server) ) { return Initialize( file ); }
    // Add the interface function of HtmlHelp:
    bool AddBook(const wxString& file, bool showWaitMsg)
      { return Initialize(file) && wxFile::Exists(file); }
    // If file is "", reloads file given in Initialize
    virtual bool LoadFile(const wxString& file = wxEmptyString);
    virtual bool DisplayContents();
    virtual bool DisplaySection(int sectionNo);
    virtual bool DisplaySection(const wxString& section);
    virtual bool DisplayBlock(long blockNo);
    virtual bool DisplayContextPopup(int contextId);
    virtual bool DisplayTextPopup(const wxString& text, const wxPoint& pos);
    virtual bool KeywordSearch(const wxString& k,
                               wxHelpSearchMode mode = wxHELP_SEARCH_ALL);
    virtual bool Quit();

    wxString GetHelpFile() const { return m_helpFile; }

protected:
    // Append extension if necessary.
    wxString GetValidFilename(const wxString& file) const;

protected:
    wxString m_helpFile;

    DECLARE_CLASS(wxCHMHelpController2)
};

#endif // wxUSE_MS_HTML_HELP

#endif
// _WX_HELPCHM_H_
