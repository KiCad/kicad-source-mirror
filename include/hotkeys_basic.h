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

/* handle a Section name and the corresponding list of hotkeys (Ki_HotkeyInfo list) */
struct Ki_HotkeyInfoSectionDescriptor
{
public:
    wxString*       m_SectionTag;           // The section name
    Ki_HotkeyInfo** m_HK_InfoList;          // pointer on List of Ki_HotkeyInfo
    char*           m_Comment;              // comment: will be printed in the config file

/*
 *  public:
 *  Ki_HotkeyInfoSectionDescriptor( wxString * SectionTag, Ki_HotkeyInfo ** HK_InfoList )
 *  { m_SectionTag = SectionTag; m_HK_InfoList = HK_InfoList; }
 */
};

/* Identifiers (tags) in key code configuration file file
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

/* Functions:
 */
wxString    ReturnKeyNameFromKeyCode( int keycode );
wxString    ReturnKeyNameFromCommandId( Ki_HotkeyInfo** List, int CommandId );
wxString    AddHotkeyName( const wxString& text, Ki_HotkeyInfo** List, int CommandId );
wxString    AddHotkeyName( const wxString& text,
		struct Ki_HotkeyInfoSectionDescriptor* DescrList,
		int CommandId );
void        DisplayHotkeyList( WinEDA_DrawFrame* frame,
                               struct Ki_HotkeyInfoSectionDescriptor* List );
int         GetCommandCodeFromHotkey( int key, Ki_HotkeyInfo** List );


#endif // HOTKEYS_BASIC_H
