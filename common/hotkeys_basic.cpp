/*********************/
/* hotkeys_basic.cpp */
/*********************/

/* Some functions to handle hotkeys in kicad
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "hotkeys_basic.h"
#include "macros.h"
#include "bitmaps.h"
#include "id.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "wxstruct.h"
#include "dialog_hotkeys_editor.h"

#include <wx/apptrait.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>

#define HOTKEYS_CONFIG_KEY wxT( "Keys" )

wxString g_CommonSectionTag( wxT( "[common]" ) );
wxString g_SchematicSectionTag( wxT( "[eeschema]" ) );
wxString g_LibEditSectionTag( wxT( "[libedit]" ) );
wxString g_BoardEditorSectionTag( wxT( "[pcbnew]" ) );
wxString g_ModuleEditSectionTag( wxT( "[footprinteditor]" ) );


/* Class to handle hotkey commnands. hotkeys have a default value
 * This class allows the real key code changed by user from a key code list
 * file.
 */

Ki_HotkeyInfo::Ki_HotkeyInfo( const wxChar* infomsg, int idcommand,
                              int keycode, int idmenuevent )
{
    m_KeyCode = keycode;            // Key code (ascii value for ascii keys
    // or wxWidgets code for function key
    m_InfoMsg   = infomsg;          // info message.
    m_Idcommand = idcommand;        // internal id for the corresponding
    // command (see hotkey_id_commnand list)
    m_IdMenuEvent = idmenuevent;    // id to call the corresponding event
    // (if any) (see id.h)
}


Ki_HotkeyInfo::Ki_HotkeyInfo( const Ki_HotkeyInfo* base )
{
    m_KeyCode     = base->m_KeyCode;
    m_InfoMsg     = base->m_InfoMsg;
    m_Idcommand   = base->m_Idcommand;
    m_IdMenuEvent = base->m_IdMenuEvent;
}


/* class to handle the printable name and the keycode
 */
struct hotkey_name_descr
{
    const wxChar* m_Name;
    int           m_KeyCode;
};

/* table giving the hotkey name from the hotkey code, for special keys
 * Note : when modifiers (ATL, SHIFT, CTRL) do not modify
 * the code of the key, do need to enter the modified key code
 * For instance wxT( "F1" ), WXK_F1 handle F1, AltF1, CtrlF1 ...
 * Key names are:
 *        "Space","Ctrl+Space","Alt+Space" or
 *      "Alt+A","Ctrl+F1", ...
 */
static struct hotkey_name_descr s_Hotkey_Name_List[] =
{
    { wxT( "F1" ),           WXK_F1                                                   },
    { wxT( "F2" ),           WXK_F2                                                   },
    { wxT( "F3" ),           WXK_F3                                                   },
    { wxT( "F4" ),           WXK_F4                                                   },
    { wxT( "F5" ),           WXK_F5                                                   },
    { wxT( "F6" ),           WXK_F6                                                   },
    { wxT( "F7" ),           WXK_F7                                                   },
    { wxT( "F8" ),           WXK_F8                                                   },
    { wxT( "F9" ),           WXK_F9                                                   },
    { wxT( "F10" ),          WXK_F10                                                  },
    { wxT( "F11" ),          WXK_F11                                                  },
    { wxT( "F12" ),          WXK_F12                                                  },

    { wxT( "Esc" ),          WXK_ESCAPE                                               },
    { wxT( "Del" ),          WXK_DELETE                                               },
    { wxT( "Tab" ),          WXK_TAB                                                  },
    { wxT( "BkSp" ),         WXK_BACK                                                 },
    { wxT( "Ins" ),          WXK_INSERT                                               },

    { wxT( "Home" ),         WXK_HOME                                                 },
    { wxT( "End" ),          WXK_END                                                  },
    { wxT( "PgUp" ),         WXK_PAGEUP                                               },
    { wxT( "PgDn" ),         WXK_PAGEDOWN                                             },

    { wxT( "Up" ),           WXK_UP                                                   },
    { wxT( "Down" ),         WXK_DOWN                                                 },
    { wxT( "Left" ),         WXK_LEFT                                                 },
    { wxT( "Right" ),        WXK_RIGHT                                                },

    { wxT( "Space" ),        WXK_SPACE                                                },

    // Do not change this line: end of list
    { wxT( "" ),             0                                                        }
};

#define MODIFIER_CTRL wxT( "Ctrl+" )
#define MODIFIER_ALT  wxT( "Alt+" )
#define MODIFIER_SHIFT  wxT( "Shift+" )


/**
 * Function ReturnKeyNameFromKeyCode
 * return the key name from the key code
 * Only some wxWidgets key values are handled for function key ( see
 * s_Hotkey_Name_List[] )
 * @param aKeycode = key code (ascii value, or wxWidgets value for function keys)
 * @param aIsFound = a pointer to a bool to return true if found, or false. an be NULL default)
 * @return the key name in a wxString
 */
wxString ReturnKeyNameFromKeyCode( int aKeycode, bool* aIsFound )
{
    wxString keyname, modifier, fullkeyname;
    int      ii;
    bool     found = false;

    if( (aKeycode & GR_KB_CTRL) != 0 )
        modifier << MODIFIER_CTRL;
    if( (aKeycode & GR_KB_ALT) != 0 )
        modifier << MODIFIER_ALT;
    if( (aKeycode & GR_KB_SHIFT) != 0 )
        modifier << MODIFIER_SHIFT;

    aKeycode &= ~( GR_KB_CTRL | GR_KB_ALT | GR_KB_SHIFT );

    if( (aKeycode > ' ') && (aKeycode < 0x7F ) )
    {
        found   = true;
        keyname.Append((wxChar)aKeycode);
    }
    else
    {
        for( ii = 0; ; ii++ )
        {
            if( s_Hotkey_Name_List[ii].m_KeyCode == 0 ) // End of list
            {
                keyname = wxT( "<unknown>" );
                break;
            }
            if( s_Hotkey_Name_List[ii].m_KeyCode == aKeycode )
            {
                keyname = s_Hotkey_Name_List[ii].m_Name;
                found   = true;
                break;
            }
        }
    }

    if( aIsFound )
        *aIsFound = found;
    fullkeyname = modifier + keyname;
    return fullkeyname;
}


/**
 * Function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param aText = a wxString. returns aText + key name
 * @param aList = pointer to a Ki_HotkeyInfo list of commands
 * @param aCommandId = Command Id value
 * @param aIsShortCut = true to add <tab><keyname> (active shortcuts in menus)
 *                    = false to add <spaces><(keyname)>
 * @return a wxString (aTest + key name) if key found or aText without modification
 */
wxString AddHotkeyName( const wxString& aText, Ki_HotkeyInfo** aList,
                        int aCommandId, bool aIsShortCut )
{
    wxString msg = aText;
    wxString keyname;

    if( aList )
        keyname = ReturnKeyNameFromCommandId( aList, aCommandId );

    if( !keyname.IsEmpty() )
    {
        if( aIsShortCut )
            msg << wxT( "\t" ) << keyname;
        else
            msg << wxT( " <" ) << keyname << wxT( ">" );
    }
    return msg;
}


/**
 * Function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param aText = a wxString. returns aText + key name
 * @param aList = pointer to a Ki_HotkeyInfoSectionDescriptor DescrList of commands
 * @param aCommandId = Command Id value
 * @param aIsShortCut = true to add <tab><keyname> (active shortcuts in menus)
 *                    = false to add <spaces><(keyname)>
 * @return a wxString (aTest + key name) if key found or aText without modification
 */
wxString AddHotkeyName( const wxString&                        aText,
                        struct Ki_HotkeyInfoSectionDescriptor* aDescList,
                        int                                    aCommandId,
                        bool                                   aIsShortCut )
{
    wxString        msg = aText;
    wxString        keyname;
    Ki_HotkeyInfo** List;

    if( aDescList )
    {
        for( ; aDescList->m_HK_InfoList != NULL; aDescList++ )
        {
            List    = aDescList->m_HK_InfoList;
            keyname = ReturnKeyNameFromCommandId( List, aCommandId );
            if( !keyname.IsEmpty() )
            {
                if( aIsShortCut )
                    msg << wxT( "\t" ) << keyname;
                else
                    msg << wxT( " <" ) << keyname << wxT( ">" );
                break;
            }
        }
    }

    return msg;
}


/**
 * Function ReturnKeyNameFromCommandId
 * return the key name from the Command id value ( m_Idcommand member value)
 * @param aList = pointer to a Ki_HotkeyInfo list of commands
 * @param aCommandId = Command Id value
 * @return the key name in a wxString
 */
wxString ReturnKeyNameFromCommandId( Ki_HotkeyInfo** aList, int aCommandId )
{
    wxString keyname;

    for( ; *aList != NULL; aList++ )
    {
        Ki_HotkeyInfo* hk_decr = *aList;
        if( hk_decr->m_Idcommand == aCommandId )
        {
            keyname = ReturnKeyNameFromKeyCode( hk_decr->m_KeyCode );
            break;
        }
    }

    return keyname;
}


/**
 * Function ReturnKeyCodeFromKeyName
 * return the key code from its key name
 * Only some wxWidgets key values are handled for function key
 * @param keyname = wxString key name to find in s_Hotkey_Name_List[],
 *   like F2 or space or an usual (ascii) char.
 * @return the key code
 */
int ReturnKeyCodeFromKeyName( const wxString& keyname )
{
    int ii, keycode = 0;

    // Search for modifiers: Ctrl+ Alt+ and Shift+
    wxString key = keyname;
    int modifier = 0;
    while( 1 )
    {
        if( key.StartsWith( MODIFIER_CTRL ) )
        {
            modifier |= GR_KB_CTRL;
            key.Remove( 0, 5 );
        }

        else if( key.StartsWith( MODIFIER_ALT ) )
        {
            modifier |= GR_KB_ALT;
            key.Remove( 0, 4 );
        }
        else if( key.StartsWith( MODIFIER_SHIFT ) )
        {
            modifier |= GR_KB_SHIFT;
            key.Remove( 0, 6 );
        }
        else
            break;
    }

    if( (key.length() == 1) && (key[0] > ' ') && (key[0] < 0x7F) )
    {
        keycode = key[0];
        keycode += modifier;
        return keycode;
    }

    for( ii = 0; ; ii++ )
    {
        if( s_Hotkey_Name_List[ii].m_KeyCode == 0 )  // End of list reached
            break;

        if( key.CmpNoCase( s_Hotkey_Name_List[ii].m_Name ) == 0 )
        {
            keycode = s_Hotkey_Name_List[ii].m_KeyCode + modifier;
            break;
        }
    }

    return keycode;
}


/**
 * Function DisplayHotkeyList
 * Displays the current hotkey list
 * @param aFrame = current active frame
 * @param aList = pointer to a Ki_HotkeyInfoSectionDescriptor list
 *(Null terminated)
 * @return none
 */
void DisplayHotkeyList( WinEDA_DrawFrame*                      aFrame,
                        struct Ki_HotkeyInfoSectionDescriptor* aDescList )
{
    wxString        keyname;
    Ki_HotkeyInfo** List;

    wxString        msg = _( "Current hotkey list:\n\n" );

    for( ; aDescList->m_HK_InfoList != NULL; aDescList++ )
    {
        List = aDescList->m_HK_InfoList;
        for( ; *List != NULL; List++ )
        {
            Ki_HotkeyInfo* hk_decr = *List;
            msg    += _( "key " );
            keyname = ReturnKeyNameFromKeyCode( hk_decr->m_KeyCode );
            msg    += keyname + wxT( ":    " ) + hk_decr->m_InfoMsg + wxT( "\n" );
        }
    }

    DisplayInfoMessage( aFrame, msg );
}


/**
 * Function GetDescriptorFromHotkey
 * Return a Ki_HotkeyInfo * pointer fron a key code for OnHotKey() function
 * @param aKey = key code (ascii value, or wxWidgets value for function keys
 * @param aList = pointer to a Ki_HotkeyInfo list of commands
 * @return the corresponding Ki_HotkeyInfo pointer from the Ki_HotkeyInfo List
 */
Ki_HotkeyInfo* GetDescriptorFromHotkey( int aKey, Ki_HotkeyInfo** aList )
{
    for( ; *aList != NULL; aList++ )
    {
        Ki_HotkeyInfo* hk_decr = *aList;
        if( hk_decr->m_KeyCode == aKey )
            return hk_decr;
    }

    return NULL;
}


/**
 * Function WriteHotkeyConfig
 * Store the current hotkey list
 * It is stored using the standard wxConfig mechanism or a file.
 *
 * @param aDescList = pointer to the current hotkey list.
 * @param aFullFileName = a wxString pointer to a fuill file name.
 *  if NULL, use the standard wxConfig mechanism (default)
 * the output format is: shortcut  "key"  "function"
 * lines starting with # are comments
 */
int WinEDA_BasicFrame::WriteHotkeyConfig( struct Ki_HotkeyInfoSectionDescriptor* aDescList,
                                          wxString*                              aFullFileName )
{
    wxString msg;
    wxString keyname, infokey;

    msg = wxT( "$hotkey list\n" );

    /* Print the current hotkey list */
    Ki_HotkeyInfo** List;
    for( ; aDescList->m_HK_InfoList != NULL; aDescList++ )
    {
        if( aDescList->m_Comment )
        {
            msg += wxT( "# " );
            msg += wxString( aDescList->m_Comment );
            msg += wxT( "\n" );
        }
        msg += *aDescList->m_SectionTag;
        msg += wxT( "\n" );

        List = aDescList->m_HK_InfoList;
        for( ; *List != NULL; List++ )
        {
            Ki_HotkeyInfo* hk_decr = *List;
            msg    += wxT( "shortcut   " );
            keyname = ReturnKeyNameFromKeyCode( hk_decr->m_KeyCode );
            AddDelimiterString( keyname );
            infokey = hk_decr->m_InfoMsg;
            AddDelimiterString( infokey );
            msg += keyname + wxT( ":    " ) + infokey + wxT( "\n" );
        }
    }

    msg += wxT( "$Endlist\n" );

    if( aFullFileName )
    {
        FILE* file = wxFopen( *aFullFileName, wxT( "wt" ) );
        if( file )
            fputs( CONV_TO_UTF8( msg ), file );
        else
        {
            msg.Printf( wxT( "Unable to write file %s" ), GetChars( *aFullFileName ) );
            return 0;
        }
    }
    else
    {
        wxConfig config( m_FrameName );
        config.Write( HOTKEYS_CONFIG_KEY, msg );
    }

    return 1;
}


/**
 * Function ReadHotkeyConfigFile
 * Read an old configuration file (&ltfile&gt.key) and fill the current hotkey list
 * with hotkeys
 * @param aFilename = file name to read.
 * @param aDescList = current hotkey list descr. to initialise.
 */
int WinEDA_BasicFrame::ReadHotkeyConfigFile(
    const wxString&                        aFilename,
    struct Ki_HotkeyInfoSectionDescriptor* aDescList )
{
    wxFile cfgfile( aFilename );

    /* get length */
    cfgfile.SeekEnd();
    wxFileOffset size = cfgfile.Tell();
    cfgfile.Seek( 0 );

    /* read data */
    char*    buffer = new char[size];
    cfgfile.Read( buffer, size );

    wxString data( buffer, wxConvUTF8 );

    /* parse */
    ParseHotkeyConfig( data, aDescList );

    /* cleanup */
    delete buffer;
    cfgfile.Close();
    return 1;
}


void ReadHotkeyConfig( const wxString&                        Appname,
                       struct Ki_HotkeyInfoSectionDescriptor* aDescList )
{
    wxConfig config( Appname );

    if( !config.HasEntry( HOTKEYS_CONFIG_KEY ) )
    {
        // assume defaults are ok
        return;
    }

    wxString data;
    config.Read( HOTKEYS_CONFIG_KEY, &data );

    ParseHotkeyConfig( data, aDescList );
}


/**
 * Function ReadHotkeyConfig
 * Read configuration data and fill the current hotkey list with hotkeys
 * @param aDescList = current hotkey list descr. to initialise.
 */
int WinEDA_BasicFrame::ReadHotkeyConfig( struct Ki_HotkeyInfoSectionDescriptor* aDescList )
{
    ::ReadHotkeyConfig( m_FrameName, aDescList );
    return 1;
}


/**
 * Function ParseHotkeyConfig
 * the input format is: shortcut  "key"  "function"
 * lines starting by # are ignored (comments)
 * lines like [xxx] are tags (example: [common] or [libedit] which identify
 * sections
 */
void ParseHotkeyConfig(
    const wxString&                        data,
    struct Ki_HotkeyInfoSectionDescriptor* DescList )
{
    /* Read the config */
    wxStringTokenizer tokenizer( data, L"\r\n", wxTOKEN_STRTOK );
    Ki_HotkeyInfo**   CurrentHotkeyList = 0;

    while( tokenizer.HasMoreTokens() )
    {
        wxString          line = tokenizer.GetNextToken();
        wxStringTokenizer lineTokenizer( line );

        wxString          line_type = lineTokenizer.GetNextToken();
        if( line_type[0]  == '#' ) //comment
            continue;

        if( line_type[0]  == '[' ) // A tag is found. search infos in list
        {
            CurrentHotkeyList = 0;
            Ki_HotkeyInfoSectionDescriptor* DList = DescList;
            for( ; DList->m_HK_InfoList; DList++ )
            {
                if( *DList->m_SectionTag == line_type )
                {
                    CurrentHotkeyList = DList->m_HK_InfoList;
                    break;
                }
            }

            continue;
        }
        if( line_type == wxT( "$Endlist" ) )
            break;
        if( line_type != wxT( "shortcut" ) )
            continue;
        if( CurrentHotkeyList == NULL )
            continue;

        /* Get the key name */
        lineTokenizer.SetString( lineTokenizer.GetString(), L"\"\r\n\t ", wxTOKEN_STRTOK );
        wxString keyname = lineTokenizer.GetNextToken();

        wxString remainder = lineTokenizer.GetString();

        /* Get the command name */
        wxString fctname = remainder.AfterFirst( '\"' ).BeforeFirst( '\"' );

        /* search the hotkey in current hotkey list */
        for( Ki_HotkeyInfo** List = CurrentHotkeyList; *List != NULL; List++ )
        {
            Ki_HotkeyInfo* hk_decr = *List;
            if( hk_decr->m_InfoMsg == fctname )
            {
                int code = ReturnKeyCodeFromKeyName( keyname );
                if( code )
                    hk_decr->m_KeyCode = code;

                break;
            }
        }
    }
}


/**
 * Function ImportHotkeyConfigFromFile
 * Prompt the user for an old hotkey file to read, and read it.
 * @param aDescList = current hotkey list descr. to initialise.
 */
void WinEDA_BasicFrame::ImportHotkeyConfigFromFile(
    struct Ki_HotkeyInfoSectionDescriptor* aDescList )
{
    wxString ext  = DEFAULT_HOTKEY_FILENAME_EXT;
    wxString mask = wxT( "*." ) + ext;
    wxString path = wxGetCwd();
    wxString filename;

    filename = EDA_FileSelector( _( "Read Hotkey Configuration File:" ),
                                 path,
                                 filename,
                                 ext,
                                 mask,
                                 this,
                                 wxFD_OPEN,
                                 TRUE );

    if( filename.IsEmpty() )
        return;

    ReadHotkeyConfigFile( filename, aDescList );
}


/**
 * Function ExportHotkeyConfigToFile
 * Prompt the user for an old hotkey file to read, and read it.
 * @param aDescList = current hotkey list descr. to initialise.
 */
void WinEDA_BasicFrame::ExportHotkeyConfigToFile(
    struct Ki_HotkeyInfoSectionDescriptor* aDescList )
{
    wxString ext  = DEFAULT_HOTKEY_FILENAME_EXT;
    wxString mask = wxT( "*." ) + ext;
    wxString path = wxGetCwd();
    wxString filename;

    filename = EDA_FileSelector( _( "Read Hotkey Configuration File:" ),
                                 path,
                                 filename,
                                 ext,
                                 mask,
                                 this,
                                 wxFD_OPEN,
                                 TRUE );

    if( filename.IsEmpty() )
        return;

    WriteHotkeyConfig( aDescList, &filename );
}


/** add hotkey config options submenu to a menu
 * @param menu : root menu
 */
void AddHotkeyConfigMenu( wxMenu* aMenu )
{
    if( aMenu == NULL )
        return;

    wxMenuItem* item;
    wxMenu*     HotkeySubmenu = new wxMenu();

    /* List existing hotkey menu*/
    item = new wxMenuItem( HotkeySubmenu,
                          ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                          _( "List Current Keys" ),
                          _( "Displays the current hotkeys list and corresponding commands" ) );
    item->SetBitmap( info_xpm );
    HotkeySubmenu->Append( item );

    /* Call hotkeys editor*/
    item = new wxMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_SHOW_EDITOR,
                          _( "Edit Hotkeys" ),
                          _( "Call the hotkeys editor" ) );
    item->SetBitmap( editor_xpm );
    HotkeySubmenu->Append( item );

    HotkeySubmenu->AppendSeparator();

    /* create hotkey file to export current hotkeys config */
    item = new wxMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_EXPORT_CONFIG,
                           _( "Export Hotkeys Config" ),
                           _(
                               "Create a hotkey configuration file to export the current hotkey config" )
                           );
    item->SetBitmap( save_setup_xpm );
    HotkeySubmenu->Append( item );

    /* Reload hotkey file */
    item = new wxMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_IMPORT_CONFIG,
                          _( "Import Hotkeys Config" ),
                          _( "Load an existing hotkey configuration file" ) );
    item->SetBitmap( reload_xpm );
    HotkeySubmenu->Append( item );

    /* Append HotkeySubmenu to menu */
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( aMenu, HotkeySubmenu,
                                        ID_PREFERENCES_HOTKEY_SUBMENU, _( "Hotkeys" ),
                                        _( "Hotkeys configuration and preferences" ), hotkeys_xpm );
}
