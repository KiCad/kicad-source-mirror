	/*********************/
	/* hotkeys_basic.cpp */
	/*********************/

/* Some functions to handle hotkeys in kicad
*/
#include "fctsys.h"
#include "common.h"
#include "hotkeys_basic.h"

/* Class to handle hotkey commnands. hotkeys have a default value
This class allows (for the future..) the real key code changed by user(from a key code list file, TODO)
*/

Ki_HotkeyInfo::Ki_HotkeyInfo(const wxChar * infomsg, int idcommand, int keycode)
{
	m_KeyCode = keycode;		// Key code (ascii value for ascii keys or wxWidgets code for function key
	m_InfoMsg = infomsg;		// info message.
	m_Idcommand = idcommand;	// internal id for the corresponding command (see hotkey_id_commnand list)
}

/****************************************************/
wxString ReturnKeyNameFromKeyCode(int keycode)
/****************************************************/
/*
	* return the key name from the key code
	* Only some wxWidgets key values are handled for function key
	* @param key = key code (ascii value, or wxWidgets value for function keys)
	* @return the key name in a wxString
*/
{
wxString keyname, modifier, fullkeyname;
	
	if ( (keycode & GR_KB_CTRL) != 0 ) modifier << wxT("Ctrl ");
	if ( (keycode & GR_KB_ALT) != 0 ) modifier << wxT("Alt ");
	if ( (keycode & GR_KB_SHIFT) != 0 ) modifier << wxT("Shift ");

	switch ( keycode)
	{
		default:
			keycode &= ~(GR_KB_CTRL|GR_KB_ALT|GR_KB_SHIFT);
			keyname.Printf(wxT("%c"), keycode);
			break;
		
		case WXK_ESCAPE:
			keyname = wxT("Esc");
			break;

		case WXK_F1:
		case WXK_F2:
		case WXK_F3:
		case WXK_F4:
		case WXK_F5:
		case WXK_F6:
		case WXK_F7:
		case WXK_F8:
		case WXK_F9:
		case WXK_F10:
		case WXK_F11:
		case WXK_F12:
			keyname.Printf(wxT("F%d"), keycode - WXK_F1 + 1);
			break;
			
		case ' ':
			keyname = wxT("space");
			break;

		case '\t':
			keyname = wxT("Tab");
			break;

		case WXK_DELETE:
			keyname = wxT("Delete");
			break;

		case WXK_BACK:
			keyname = wxT("Backspace");
			break;

		case WXK_INSERT:
			keyname = wxT("Insert");
			break;

		case WXK_END:
			keyname = wxT("End");
			break;
		
		case WXK_PAGEUP:
			keyname = wxT("Page Up");
			break;
		
		case WXK_PAGEDOWN:
			keyname = wxT("Page Down");
			break;
		
		case WXK_ADD:
			keyname = wxT("+");
			break;
		
		case WXK_SUBTRACT:
			keyname = wxT("-");
			break;
		
		}
	
	fullkeyname = modifier + keyname;
	return fullkeyname;
}

/****************************************************************************/
void DisplayHotkeyList(WinEDA_DrawFrame * frame, Ki_HotkeyInfo ** List)
/*****************************************************************************/
/*
	* Displays the current hotkey list
	* @param frame = current open frame
	* @param List = pointer to a Ki_HotkeyInfo list of commands
	* @return none
*/
{
wxString keyname;
	
	wxString msg = _("Current hotkey list:\n\n");
	for ( ; * List != NULL; List++ )
	{
		Ki_HotkeyInfo * hk_decr = * List;
		if ( hk_decr->m_InfoMsg.IsEmpty() ) break;
		msg += _("key ");
		keyname = ReturnKeyNameFromKeyCode(hk_decr->m_KeyCode);
		msg += keyname + wxT(":    ") + hk_decr->m_InfoMsg + wxT("\n");
	}
	DisplayInfo(frame, msg);
}

/******************************************************************/
int GetCommandCodeFromHotkey(int key, Ki_HotkeyInfo ** List)
/******************************************************************/
/*
	* Return an id identifier fron a key code for OnHotKey() function
	* @param key = key code (ascii value, or wxWidgets value for function keys
	* @param List = pointer to a Ki_HotkeyInfo list of commands
	* @return the corresponding function identifier from the Ki_HotkeyInfo  List
*/
{
	for ( ; * List != NULL; List++ )
	{
		Ki_HotkeyInfo * hk_decr = * List;
		if ( hk_decr->m_KeyCode == key ) return hk_decr->m_Idcommand;
	}
	
	return 0;
}

