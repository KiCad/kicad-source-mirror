/*******************/
/* hotkeys_basic.h */
/*******************/

/* Some functions to handle hotkeys in kicad
 */

#ifndef  HOTKEYS_BASIC_H
#define  HOTKEYS_BASIC_H

#define DEFAULT_HOTKEY_FILENAME_EXT wxT( "key" )


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
    Ki_HotkeyInfo( const Ki_HotkeyInfo* base);
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
    const wchar_t*      m_Comment;             // comment: will be printed in the config file
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


/* Functions:
 */
void            AddHotkeyConfigMenu( wxMenu* menu );
void            HandleHotkeyConfigMenuSelection( EDA_DRAW_FRAME* frame, int id );

/**
 * Function ReturnKeyNameFromKeyCode
 * return the key name from the key code
 * Only some wxWidgets key values are handled for function key ( see
 * s_Hotkey_Name_List[] )
 * @param aKeycode = key code (ascii value, or wxWidgets value for function keys)
 * @param aIsFound = a pointer to a bool to return true if found, or false. an be NULL default)
 * @return the key name in a wxString
 */
wxString        ReturnKeyNameFromKeyCode( int aKeycode, bool * aIsFound = NULL );

/**
 * Function ReturnKeyNameFromCommandId
 * return the key name from the Command id value ( m_Idcommand member value)
 * @param aList = pointer to a Ki_HotkeyInfo list of commands
 * @param aCommandId = Command Id value
 * @return the key name in a wxString
 */
wxString        ReturnKeyNameFromCommandId( Ki_HotkeyInfo** aList, int aCommandId );

/**
 * Function ReturnKeyCodeFromKeyName
 * return the key code from its key name
 * Only some wxWidgets key values are handled for function key
 * @param keyname = wxString key name to find in s_Hotkey_Name_List[],
 *   like F2 or space or an usual (ascii) char.
 * @return the key code
 */
int ReturnKeyCodeFromKeyName( const wxString& keyname );

/**
 * Function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param aText = a wxString. returns aText + key name
 * @param aList = pointer to a Ki_HotkeyInfo list of commands
 * @param aCommandId = Command Id value
 * @param aIsShortCut = true to add &lttab&gt&ltkeyname&gt (active shortcuts in menus)
 *                    = false to add &ltspaces&gt&lt(keyname)&gt
 * @return a wxString (aTest + key name) if key found or aText without modification
 */
wxString        AddHotkeyName( const wxString& aText, Ki_HotkeyInfo** aList,
                               int  aCommandId,
                               bool aIsShortCut = true);

/**
 * Function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param aText = a wxString. returns aText + key name
 * @param aDescrList = pointer to a Ki_HotkeyInfoSectionDescriptor DescrList of commands
 * @param aCommandId = Command Id value
 * @param aIsShortCut = true to add &lttab&gt&ltkeyname&gt (active shortcuts in menus)
 *                    = false to add &ltspaces&gt&lt(keyname)&gt
 * @return a wxString (aTest + key name) if key found or aText without modification
 */
wxString        AddHotkeyName( const wxString&                        aText,
                               struct Ki_HotkeyInfoSectionDescriptor* aDescrList,
                               int                                    aCommandId,
                               bool                                   aIsShortCut = true);

/**
 * Function DisplayHotkeyList
 * Displays the current hotkey list
 * @param aFrame = current active frame
 * @param aList = pointer to a Ki_HotkeyInfoSectionDescriptor list (Null terminated)
 */
void            DisplayHotkeyList( EDA_DRAW_FRAME*                        aFrame,
                                   struct Ki_HotkeyInfoSectionDescriptor* aList );

/**
 * Function GetDescriptorFromHotkey
 * Return a Ki_HotkeyInfo * pointer fron a key code for OnHotKey() function
 * @param aKey = key code (ascii value, or wxWidgets value for function keys
 * @param aList = pointer to a Ki_HotkeyInfo list of commands
 * @return the corresponding Ki_HotkeyInfo pointer from the Ki_HotkeyInfo List
 */
Ki_HotkeyInfo*  GetDescriptorFromHotkey( int aKey, Ki_HotkeyInfo** aList );

/**
 * Function ReadHotkeyConfig
 * Read hotkey configuration for a given app,
 * possibly before the frame for that app has been created
 * @param Appname = the value of the app's m_FrameName
 * @param aDescList = the hotkey data
*/
void            ReadHotkeyConfig( const wxString&                        Appname,
                                  struct Ki_HotkeyInfoSectionDescriptor* aDescList );

void            ParseHotkeyConfig( const wxString&                        data,
                                   struct Ki_HotkeyInfoSectionDescriptor* aDescList );


// common hotkeys event id
// these hotkey ID are used in many files, so they are define here only once.
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
