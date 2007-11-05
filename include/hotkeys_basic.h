/*******************/
/* hotkeys_basic.h */
/*******************/

/* Some functions to handle hotkeys in kicad
 */

#ifndef  HOTKEYS_BASIC_H
#define  HOTKEYS_BASIC_H

#ifndef COMMON_GLOBL
#define COMMON_GLOBL extern
#endif

#define DEFAULT_HOTKEY_FILENAME_EXT wxT( ".key" )

/* define default path for config key file */
#define DEFAULT_HOTKEY_FILENAME_PATH_IS_HOME  wxGetHomeDir() + wxT( "/" )
#define DEFAULT_HOTKEY_FILENAME_PATH_IS_KICAD g_EDA_Appl->m_BinDir + wxT( "../template/" )

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
 * this feature avoid duplications and made hotkey file config easier to understand ane edit
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
COMMON_GLOBL wxString g_CommonSectionTag
#ifdef EDA_BASE
( wxT( "[common]" ) )
#endif
;
COMMON_GLOBL wxString g_SchematicSectionTag
#ifdef EDA_BASE
( wxT( "[eeschema]" ) )
#endif
;
COMMON_GLOBL wxString g_LibEditSectionTag
#ifdef EDA_BASE
( wxT( "[libedit]" ) )
#endif
;
COMMON_GLOBL wxString g_BoardEditorSectionTag
#ifdef EDA_BASE
( wxT( "[pcbnew]" ) )
#endif
;
COMMON_GLOBL wxString g_ModuleEditSectionTag
#ifdef EDA_BASE
( wxT( "[footprinteditor]" ) )
#endif
;

COMMON_GLOBL int g_ConfigFileLocationChoice;    /* 0 = files are in Home directory (usefull under unix)
                                                 * 1 = kicad/template ( usefull only under windows )
                                                 * 2 ... = unused
                                                 */

/* Functions:
 */
wxString        ReturnHotkeyConfigFilePath( int choice );
void            AddHotkeyConfigMenu( wxMenu* menu );
void            HandleHotkeyConfigMenuSelection( WinEDA_DrawFrame* frame, int id );
wxString        ReturnKeyNameFromKeyCode( int keycode );
wxString        ReturnKeyNameFromCommandId( Ki_HotkeyInfo** List, int CommandId );
wxString        AddHotkeyName( const wxString& text, Ki_HotkeyInfo** List, int CommandId );
wxString        AddHotkeyName( const wxString&                        text,
                               struct Ki_HotkeyInfoSectionDescriptor* DescrList,
                               int                                    CommandId );
void            DisplayHotkeyList( WinEDA_DrawFrame*                      frame,
                                   struct Ki_HotkeyInfoSectionDescriptor* List );
Ki_HotkeyInfo*  GetDescriptorFromHotkey( int key, Ki_HotkeyInfo** List );


#endif // HOTKEYS_BASIC_H
