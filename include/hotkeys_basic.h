/*******************/
/* hotkeys_basic.h */
/*******************/

/* Some functions to handle hotkeys in kicad
 */

#ifndef  HOTKEYS_BASIC_H
#define  HOTKEYS_BASIC_H

#define DEFAULT_HOTKEY_FILENAME_EXT wxT( ".key" )

/* define default path for config key file */
#ifdef __WINDOWS__
#define DEFAULT_HOTKEY_FILENAME_PATH EDA_Appl->m_BinDir + wxT( "../template/" )
#else
#define DEFAULT_HOTKEY_FILENAME_PATH wxGetHomeDir() + wxT( "/" )
#endif


/* Class to handle hotkey commnands. hotkeys have a default value
 *  This class allows (for the future..) the real key code changed by user(from a key code list file, TODO)
 */
class Ki_HotkeyInfo
{
public:
    int      m_KeyCode;             // Key code (ascii value for ascii keys or wxWidgets code for function key
    wxString m_InfoMsg;             // info message.
    int      m_Idcommand;           // internal id for the corresponding command (see hotkey_id_commnand list)

public:
    Ki_HotkeyInfo( const wxChar* infomsg, int idcommand, int keycode );
};

/* Functions:
 */
wxString    ReturnKeyNameFromKeyCode( int keycode );
wxString    ReturnKeyNameFromCommandId( Ki_HotkeyInfo** List, int CommandId );
wxString    AddHotkeyName( const wxString& text, Ki_HotkeyInfo** List, int CommandId );
void        DisplayHotkeyList( WinEDA_DrawFrame* frame, Ki_HotkeyInfo** List );
int         GetCommandCodeFromHotkey( int key, Ki_HotkeyInfo** List );


#endif // HOTKEYS_BASIC_H
