/*******************/
/* hotkeys_basic.h */
/*******************/

/* Some functions to handle hotkeys in kicad
 */

#ifndef  HOTKEYS_BASIC_H
#define  HOTKEYS_BASIC_H

#define DEFAULT_HOTKEY_FILENAME_EXT wxT( "key" )

/* keyword idetifier in kicad config use ti store/retrieve path option */
#define HOTKEY_CFG_PATH_OPT wxT( "HotkeyPathOption" )


/* Class to handle hotkey commnands. hotkeys have a default value
 *  This class allows the real key code changed by user(from a key code list file)
 */
class Ki_HotkeyInfo
{
public:
    int      m_KeyCode;             // Key code (ascii value for ascii keys or wxWidgets code for function key
    wxString m_InfoMsg;             // info message.
    int      m_Idcommand;           // internal id for the corresponding command (see hotkey_id_commnand list)
    int      m_IdMenuEvent;         // id to call the corresponding event (if any) (see id.h)

public:
    Ki_HotkeyInfo( const wxChar* infomsg, int idcommand, int keycode, int idmenuevent = 0 );
};

/* handle a Section name and the corresponding list of hotkeys (Ki_HotkeyInfo list)
 * hotkeys are grouped by section.
 * a section is a list of hotkey infos ( a Ki_HotkeyInfo list).
 * A full list of hoteys can used one or many sections
 * for instance:
 *    the schematic editor uses a common section (zoom hotkeys list ..) and a specific section
 *    the library editor uses the same common section and a specific section
 * this feature avoid duplications and made hotkey file config easier to understand and edit
 */
struct Ki_HotkeyInfoSectionDescriptor
{
public:
    wxString*       m_SectionTag;           // The section name
    Ki_HotkeyInfo** m_HK_InfoList;          // List of Ki_HotkeyInfo pointers
    const char*      m_Comment;             // comment: will be printed in the config file
                                            // Info usage only
};

/* Identifiers (tags) in key code configuration file (or section names)
 *  .m_SectionTag member of a Ki_HotkeyInfoSectionDescriptor
 */
extern wxString g_CommonSectionTag;
extern wxString g_SchematicSectionTag;
extern wxString g_LibEditSectionTag;
extern wxString g_BoardEditorSectionTag;
extern wxString g_ModuleEditSectionTag;

extern int g_ConfigFileLocationChoice;


/* Functions:
 */
wxString        ReturnHotkeyConfigFilePath( int choice );
void            AddHotkeyConfigMenu( wxMenu* menu );
void            HandleHotkeyConfigMenuSelection( WinEDA_DrawFrame* frame, int id );

/** function ReturnKeyNameFromKeyCode
 * return the key name from the key code
 * Only some wxWidgets key values are handled for function key ( see
 * s_Hotkey_Name_List[] )
 * @param aKeycode = key code (ascii value, or wxWidgets value for function keys)
 * @return the key name in a wxString
 */
wxString        ReturnKeyNameFromKeyCode( int aKeycode );

/** function ReturnKeyNameFromCommandId
 * return the key name from the Command id value ( m_Idcommand member value)
 * @param aList = pointer to a Ki_HotkeyInfo list of commands
 * @param aCommandId = Command Id value
 * @return the key name in a wxString
 */
wxString        ReturnKeyNameFromCommandId( Ki_HotkeyInfo** aList, int aCommandId );

/** function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param aText = a wxString. returns aText + key name
 * @param aList = pointer to a Ki_HotkeyInfo list of commands
 * @param aCommandId = Command Id value
 * @param aIsShortCut = true to add <tab><keyname> (active shortcuts in menus)
 *                    = false to add <spaces><(keyname)>
 * @return a wxString (aTest + key name) if key found or aText without modification
 */
wxString        AddHotkeyName( const wxString& aText, Ki_HotkeyInfo** aList,
                               int  aCommandId,
                               bool aIsShortCut = true);

/** function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param aText = a wxString. returns aText + key name
 * @param aList = pointer to a Ki_HotkeyInfoSectionDescriptor DescrList of commands
 * @param aCommandId = Command Id value
 * @param aIsShortCut = true to add <tab><keyname> (active shortcuts in menus)
 *                    = false to add <spaces><(keyname)>
 * @return a wxString (aTest + key name) if key found or aText without modification
 */
wxString        AddHotkeyName( const wxString&                        aText,
                               struct Ki_HotkeyInfoSectionDescriptor* aDescrList,
                               int                                    aCommandId,
                               bool                                   aIsShortCut = true);

void            DisplayHotkeyList( WinEDA_DrawFrame*                      frame,
                                   struct Ki_HotkeyInfoSectionDescriptor* List );
Ki_HotkeyInfo*  GetDescriptorFromHotkey( int key, Ki_HotkeyInfo** List );


// common hotkeys event id
enum common_hotkey_id_commnand {
    HK_NOT_FOUND = 0,
    HK_RESET_LOCAL_COORD,
    HK_HELP,
    HK_ZOOM_IN,
    HK_ZOOM_OUT,
    HK_ZOOM_REDRAW,
    HK_ZOOM_CENTER,
    HK_ZOOM_AUTO,
	HK_UNDO,
	HK_REDO,
    HK_COMMON_END
};

#endif // HOTKEYS_BASIC_H
