#include "myctrls.h"

BEGIN_EVENT_TABLE(CMyGridTextEditor, wxEvtHandler)
    EVT_KEY_DOWN(CMyGridTextEditor::OnKeyDown)
END_EVENT_TABLE()

void CMyGridTextEditor::OnKeyDown(wxKeyEvent &event)
{
    int Code = event.GetKeyCode();
    if (Code == WXK_DOWN || (Code == WXK_UP && m_Grid->GetCursorRow() > 0))
    {
        m_Grid->DisableCellEditControl();
        m_Grid->ProcessEvent(event);
        return;
    }
    event.Skip();
}

BEGIN_EVENT_TABLE(CMyGridComboEditor, wxEvtHandler)
	EVT_SET_FOCUS(CMyGridComboEditor::OnSetFocus) 
	EVT_KILL_FOCUS(CMyGridComboEditor::OnKillFocus) 
    EVT_IDLE(CMyGridComboEditor::OnIdle) 
END_EVENT_TABLE()

void CMyGridComboEditor::OnSetFocus(wxFocusEvent& event)
{
    m_InSetFocus = true;
    event.Skip();
}

void CMyGridComboEditor::OnKillFocus(wxFocusEvent& event)
{
#ifdef _DEBUG
	wxLogDebug(wxT("CMyGridEditor::OnKillFocus"));
#endif	
    // Editovatelne combo dostane KillFocus i v pripade, ze focus dostava edit, ktery je childem comba,
    // defaultni handler, preda udalost gridu a ten CellEditor hned zavre. 
    if (!m_InSetFocus)
        event.Skip();
}

void CMyGridComboEditor::OnIdle(wxIdleEvent& event)
{
#ifdef _DEBUG
	wxLogDebug(wxT("CMyGridEditor::OnIdle"));
#endif	
    m_InSetFocus = false;
}

void CMyGridComboEditor::StartingKey(wxKeyEvent& event)
{
#ifdef WINS
    SetFocus(GetWindow((HWND)GetControl()->GetHWND(), GW_CHILD));
    wxUint32 code = event.GetRawKeyCode();
    if (code >= 'a' && code <= 'z')
        code &= ~0x20;
    ::keybd_event(code, 0, 0, 0);
    ::keybd_event(code, 0, KEYEVENTF_KEYUP, 0);
#else	
    wxChar ch = 0;
    int keycode = event.GetKeyCode();
    switch ( keycode )
    {
    case WXK_NUMPAD0:
	case WXK_NUMPAD1:
	case WXK_NUMPAD2:
	case WXK_NUMPAD3:
	case WXK_NUMPAD4:
	case WXK_NUMPAD5:
	case WXK_NUMPAD6:
	case WXK_NUMPAD7:
	case WXK_NUMPAD8:
	case WXK_NUMPAD9:
		ch = _T('0') + keycode - WXK_NUMPAD0;
		break;

	case WXK_MULTIPLY:
	case WXK_NUMPAD_MULTIPLY:
		ch = _T('*');
		break;

	case WXK_ADD:
	case WXK_NUMPAD_ADD:
		ch = _T('+');
		break;

	case WXK_SUBTRACT:
	case WXK_NUMPAD_SUBTRACT:
		ch = _T('-');
		break;

	case WXK_DECIMAL:
	case WXK_NUMPAD_DECIMAL:
		ch = _T('.');
		break;

	case WXK_DIVIDE:
	case WXK_NUMPAD_DIVIDE:
		ch = _T('/');
		break;

	default:
#if wxUSE_UNICODE
		if (event.GetUnicodeKey())  // added by Slavik, J.D.
			ch=event.GetUnicodeKey();
		else  
#endif            
		if ( keycode < 256 && keycode >= 0 && wxIsprint(keycode) )
		{
			// FIXME this is not going to work for non letters...
			if ( !event.ShiftDown() )
			{
				keycode = wxTolower(keycode);
			}
			ch = (wxChar)keycode;
		}
		else
		{
			ch = _T('\0');
		}
    }

    if ( ch )
	{
		wxComboBox *Combo = (wxComboBox *)GetControl();
        Combo->SetValue(ch);
		Combo->SetInsertionPointEnd();
	}
#endif // WINS
    event.Skip();
}
