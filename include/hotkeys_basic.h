	/*******************/
	/* hotkeys_basic.h */
	/*******************/

/* Some functions to handle hotkeys in kicad
*/

#ifndef  HOTKEYS_BASIC_H
#define  HOTKEYS_BASIC_H

/* Class to handle hotkey commnands. hotkeys have a default value
This class allows (for the future..) the real key code changed by user(from a key code list file, TODO)
*/
class Ki_HotkeyInfo
{
public:
	int m_KeyCode;					// Key code (ascii value for ascii keys or wxWidgets code for function key
	wxString m_InfoMsg;				// info message.
	int m_Idcommand;	// internal id for the corresponding command (see hotkey_id_commnand list)

public:
	Ki_HotkeyInfo(const wxChar * infomsg, int idcommand, int keycode);
};

/* Functions:
*/
wxString ReturnKeyNameFromKeyCode(int keycode);
void DisplayHotkeyList(WinEDA_DrawFrame * frame, Ki_HotkeyInfo ** List);
int GetCommandCodeFromHotkey(int key, Ki_HotkeyInfo ** List);


#endif // HOTKEYS_BASIC_H

