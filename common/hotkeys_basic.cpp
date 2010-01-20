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

#include <wx/apptrait.h>
#include <wx/stdpaths.h>


wxString g_CommonSectionTag( wxT( "[common]" ) );
wxString g_SchematicSectionTag( wxT( "[eeschema]" ) );
wxString g_LibEditSectionTag( wxT( "[libedit]" ) );
wxString g_BoardEditorSectionTag( wxT( "[pcbnew]" ) );
wxString g_ModuleEditSectionTag( wxT( "[footprinteditor]" ) );


/* 0 = files are in Home directory (usefull under unix)
 * 1 = kicad/template ( usefull only under windows )
 * 2 ... = unused
 */
int g_ConfigFileLocationChoice;


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


/* class to handle the printable name and the keycode
 */
struct hotkey_name_descr
{
    const wxChar* m_Name;
    int           m_KeyCode;
};

static struct hotkey_name_descr s_Hotkey_Name_List[] =
{
    { wxT( "F1" ),           WXK_F1                         },
    { wxT( "F2" ),           WXK_F2                         },
    { wxT( "F3" ),           WXK_F3                         },
    { wxT( "F4" ),           WXK_F4                         },
    { wxT( "F5" ),           WXK_F5                         },
    { wxT( "F6" ),           WXK_F6                         },
    { wxT( "F7" ),           WXK_F7                         },
    { wxT( "F8" ),           WXK_F8                         },
    { wxT( "F9" ),           WXK_F9                         },
    { wxT( "F10" ),          WXK_F10                        },
    { wxT( "F11" ),          WXK_F11                        },
    { wxT( "F12" ),          WXK_F12                        },

    { wxT( "Esc" ),          WXK_ESCAPE                     },
    { wxT( "Del" ),          WXK_DELETE                     },
    { wxT( "Tab" ),          '\t'                           },
    { wxT( "BkSp" ),         WXK_BACK                       },
    { wxT( "Ins" ),          WXK_INSERT                     },

    { wxT( "Home" ),         WXK_HOME                       },
    { wxT( "End" ),          WXK_END                        },
    { wxT( "PgUp" ),         WXK_PAGEUP                     },
    { wxT( "PgDn" ),         WXK_PAGEDOWN                   },
    { wxT( "+" ),            '+'                            },
    { wxT( "-" ),            '-'                            },

    { wxT( "Up" ),           WXK_UP                         },
    { wxT( "Down" ),         WXK_DOWN                       },
    { wxT( "Left" ),         WXK_LEFT                       },
    { wxT( "Right" ),        WXK_RIGHT                      },

    { wxT( "space" ),        ' '                            },
    { wxT( "?" ),            '?'                            },
    { wxT( "!" ),            '!'                            },
    { wxT( ":" ),            ':'                            },
    { wxT( "," ),            ','                            },
    { wxT( "*" ),            '*'                            },
    { wxT( "+" ),            '+'                            },
    { wxT( "-" ),            '-'                            },
    { wxT( "%%" ),           '%'                            },
    { wxT( "A" ),            'A'                            },
    { wxT( "B" ),            'B'                            },
    { wxT( "C" ),            'C'                            },
    { wxT( "D" ),            'D'                            },
    { wxT( "E" ),            'E'                            },
    { wxT( "F" ),            'F'                            },
    { wxT( "G" ),            'G'                            },
    { wxT( "H" ),            'H'                            },
    { wxT( "I" ),            'I'                            },
    { wxT( "J" ),            'J'                            },
    { wxT( "K" ),            'K'                            },
    { wxT( "L" ),            'L'                            },
    { wxT( "M" ),            'M'                            },
    { wxT( "N" ),            'N'                            },
    { wxT( "O" ),            'O'                            },
    { wxT( "P" ),            'P'                            },
    { wxT( "Q" ),            'Q'                            },
    { wxT( "R" ),            'R'                            },
    { wxT( "S" ),            'S'                            },
    { wxT( "T" ),            'T'                            },
    { wxT( "U" ),            'U'                            },
    { wxT( "V" ),            'V'                            },
    { wxT( "W" ),            'W'                            },
    { wxT( "X" ),            'X'                            },
    { wxT( "Y" ),            'Y'                            },
    { wxT( "Z" ),            'Z'                            },

    { wxT( "Ctrl++" ),       GR_KB_CTRL + '+'               },
    { wxT( "Ctrl+-" ),       GR_KB_CTRL + '-'               },

    { wxT( "Ctrl+A" ),       GR_KB_CTRL + 'A'               },
    { wxT( "Ctrl+B" ),       GR_KB_CTRL + 'B'               },
    { wxT( "Ctrl+C" ),       GR_KB_CTRL + 'C'               },
    { wxT( "Ctrl+D" ),       GR_KB_CTRL + 'D'               },
    { wxT( "Ctrl+E" ),       GR_KB_CTRL + 'E'               },
    { wxT( "Ctrl+F" ),       GR_KB_CTRL + 'F'               },
    { wxT( "Ctrl+G" ),       GR_KB_CTRL + 'G'               },
    { wxT( "Ctrl+H" ),       GR_KB_CTRL + 'H'               },
    { wxT( "Ctrl+I" ),       GR_KB_CTRL + 'I'               },
    { wxT( "Ctrl+J" ),       GR_KB_CTRL + 'J'               },
    { wxT( "Ctrl+K" ),       GR_KB_CTRL + 'K'               },
    { wxT( "Ctrl+L" ),       GR_KB_CTRL + 'L'               },
    { wxT( "Ctrl+M" ),       GR_KB_CTRL + 'M'               },
    { wxT( "Ctrl+N" ),       GR_KB_CTRL + 'N'               },
    { wxT( "Ctrl+O" ),       GR_KB_CTRL + 'O'               },
    { wxT( "Ctrl+P" ),       GR_KB_CTRL + 'P'               },
    { wxT( "Ctrl+Q" ),       GR_KB_CTRL + 'Q'               },
    { wxT( "Ctrl+R" ),       GR_KB_CTRL + 'R'               },
    { wxT( "Ctrl+S" ),       GR_KB_CTRL + 'S'               },
    { wxT( "Ctrl+T" ),       GR_KB_CTRL + 'T'               },
    { wxT( "Ctrl+U" ),       GR_KB_CTRL + 'U'               },
    { wxT( "Ctrl+V" ),       GR_KB_CTRL + 'V'               },
    { wxT( "Ctrl+W" ),       GR_KB_CTRL + 'W'               },
    { wxT( "Ctrl+X" ),       GR_KB_CTRL + 'X'               },
    { wxT( "Ctrl+Y" ),       GR_KB_CTRL + 'Y'               },
    { wxT( "Ctrl+Z" ),       GR_KB_CTRL + 'Z'               },

    { wxT( "Shift+Ctrl++" ), GR_KB_SHIFT + GR_KB_CTRL + '+' },
    { wxT( "Shift+Ctrl+-" ), GR_KB_SHIFT + GR_KB_CTRL + '-' },

    { wxT( "Shift+Ctrl+A" ), GR_KB_SHIFT + GR_KB_CTRL + 'A' },
    { wxT( "Shift+Ctrl+B" ), GR_KB_SHIFT + GR_KB_CTRL + 'B' },
    { wxT( "Shift+Ctrl+C" ), GR_KB_SHIFT + GR_KB_CTRL + 'C' },
    { wxT( "Shift+Ctrl+D" ), GR_KB_SHIFT + GR_KB_CTRL + 'D' },
    { wxT( "Shift+Ctrl+E" ), GR_KB_SHIFT + GR_KB_CTRL + 'E' },
    { wxT( "Shift+Ctrl+F" ), GR_KB_SHIFT + GR_KB_CTRL + 'F' },
    { wxT( "Shift+Ctrl+G" ), GR_KB_SHIFT + GR_KB_CTRL + 'G' },
    { wxT( "Shift+Ctrl+H" ), GR_KB_SHIFT + GR_KB_CTRL + 'H' },
    { wxT( "Shift+Ctrl+I" ), GR_KB_SHIFT + GR_KB_CTRL + 'I' },
    { wxT( "Shift+Ctrl+J" ), GR_KB_SHIFT + GR_KB_CTRL + 'J' },
    { wxT( "Shift+Ctrl+K" ), GR_KB_SHIFT + GR_KB_CTRL + 'K' },
    { wxT( "Shift+Ctrl+L" ), GR_KB_SHIFT + GR_KB_CTRL + 'L' },
    { wxT( "Shift+Ctrl+M" ), GR_KB_SHIFT + GR_KB_CTRL + 'M' },
    { wxT( "Shift+Ctrl+N" ), GR_KB_SHIFT + GR_KB_CTRL + 'N' },
    { wxT( "Shift+Ctrl+O" ), GR_KB_SHIFT + GR_KB_CTRL + 'O' },
    { wxT( "Shift+Ctrl+P" ), GR_KB_SHIFT + GR_KB_CTRL + 'P' },
    { wxT( "Shift+Ctrl+Q" ), GR_KB_SHIFT + GR_KB_CTRL + 'Q' },
    { wxT( "Shift+Ctrl+R" ), GR_KB_SHIFT + GR_KB_CTRL + 'R' },
    { wxT( "Shift+Ctrl+S" ), GR_KB_SHIFT + GR_KB_CTRL + 'S' },
    { wxT( "Shift+Ctrl+T" ), GR_KB_SHIFT + GR_KB_CTRL + 'T' },
    { wxT( "Shift+Ctrl+U" ), GR_KB_SHIFT + GR_KB_CTRL + 'U' },
    { wxT( "Shift+Ctrl+V" ), GR_KB_SHIFT + GR_KB_CTRL + 'V' },
    { wxT( "Shift+Ctrl+W" ), GR_KB_SHIFT + GR_KB_CTRL + 'W' },
    { wxT( "Shift+Ctrl+X" ), GR_KB_SHIFT + GR_KB_CTRL + 'X' },
    { wxT( "Shift+Ctrl+Y" ), GR_KB_SHIFT + GR_KB_CTRL + 'Y' },
    { wxT( "Shift+Ctrl+Z" ), GR_KB_SHIFT + GR_KB_CTRL + 'Z' },

    // Do not change this line: end of list
    { wxT( "" ),             0                              }
};


/** function ReturnKeyNameFromKeyCode
 * return the key name from the key code
 * Only some wxWidgets key values are handled for function key ( see
 * s_Hotkey_Name_List[] )
 * @param key = key code (ascii value, or wxWidgets value for function keys)
 * @return the key name in a wxString
 */
wxString ReturnKeyNameFromKeyCode( int keycode )
{
    wxString keyname, modifier, fullkeyname;
    int      ii;

    if( (keycode & GR_KB_CTRL) != 0 )
        modifier << wxT( "Ctrl+" );
    if( (keycode & GR_KB_ALT) != 0 )
        modifier << wxT( "Alt+" );
    if( (keycode & GR_KB_SHIFT) != 0 )
        modifier << wxT( "Shift+" );

    keycode &= ~( GR_KB_CTRL | GR_KB_ALT | GR_KB_SHIFT );
    for( ii = 0; ; ii++ )
    {
        if( s_Hotkey_Name_List[ii].m_KeyCode == 0 )
        {
            keyname = wxT( "<unknown>" );
            break;
        }
        if( s_Hotkey_Name_List[ii].m_KeyCode == keycode )
        {
            keyname = s_Hotkey_Name_List[ii].m_Name;
            break;
        }
    }

    fullkeyname = modifier + keyname;
    return fullkeyname;
}


/** function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param List = pointer to a Ki_HotkeyInfo list of commands
 * @param CommandId = Command Id value
 * @return text (key name) in a wxString if found or text without modification
 */
wxString AddHotkeyName( const wxString& text, Ki_HotkeyInfo** List,
                        int CommandId )
{
    wxString msg     = text;
    wxString keyname = ReturnKeyNameFromCommandId( List, CommandId );

    if( !keyname.IsEmpty() )
        msg << wxT( "\t" ) << keyname;
    return msg;
}


/** function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param List = pointer to a Ki_HotkeyInfoSectionDescriptor* DescrList of
 * commands
 * @param CommandId = Command Id value
 * @return text (key name) in a wxString if found or text without modification
 */
wxString AddHotkeyName( const wxString&                        text,
                        struct Ki_HotkeyInfoSectionDescriptor* DescList,
                        int                                    CommandId )
{
    wxString        msg = text;
    wxString        keyname;
    Ki_HotkeyInfo** List;

    for( ; DescList->m_HK_InfoList != NULL; DescList++ )
    {
        List    = DescList->m_HK_InfoList;
        keyname = ReturnKeyNameFromCommandId( List, CommandId );
        if( !keyname.IsEmpty() )
        {
            msg << wxT( "\t" ) << keyname;
            break;
        }
    }

    return msg;
}


/** function ReturnKeyNameFromCommandId
 * return the key name from the Command id value ( m_Idcommand member value)
 * @param List = pointer to a Ki_HotkeyInfo list of commands
 * @param CommandId = Command Id value
 * @return the key name in a wxString
 */
wxString ReturnKeyNameFromCommandId( Ki_HotkeyInfo** List, int CommandId )
{
    wxString keyname;

    for( ; *List != NULL; List++ )
    {
        Ki_HotkeyInfo* hk_decr = *List;
        if( hk_decr->m_Idcommand == CommandId )
        {
            keyname = ReturnKeyNameFromKeyCode( hk_decr->m_KeyCode );
            break;
        }
    }

    return keyname;
}


/** function ReturnKeyCodeFromKeyName
 * return the key code from its key name
 * Only some wxWidgets key values are handled for function key
 * @param keyname = wxString key name to find in s_Hotkey_Name_List[],
 *   like F2 or space or an usual (ascii) char.
 * @return the key code
 */
static int ReturnKeyCodeFromKeyName( const wxString& keyname )
{
    int ii, keycode = 0;

    for( ii = 0; ; ii++ )
    {
        if( s_Hotkey_Name_List[ii].m_KeyCode == 0 )  // End of list reached
            break;

        if( keyname.CmpNoCase( s_Hotkey_Name_List[ii].m_Name ) == 0 )
        {
            keycode = s_Hotkey_Name_List[ii].m_KeyCode;
            break;
        }
    }

    return keycode;
}


/** function DisplayHotkeyList
 * Displays the current hotkey list
 * @param frame = current active frame
 * @param List = pointer to a Ki_HotkeyInfoSectionDescriptor list
 *(Null terminated)
 * @return none
 */
void DisplayHotkeyList( WinEDA_DrawFrame*                      frame,
                        struct Ki_HotkeyInfoSectionDescriptor* DescList )
{
    wxString        keyname;
    Ki_HotkeyInfo** List;

    wxString        msg = _( "Current hotkey list:\n\n" );

    for( ; DescList->m_HK_InfoList != NULL; DescList++ )
    {
        List = DescList->m_HK_InfoList;
        for( ; *List != NULL; List++ )
        {
            Ki_HotkeyInfo* hk_decr = *List;
            msg    += _( "key " );
            keyname = ReturnKeyNameFromKeyCode( hk_decr->m_KeyCode );
            msg    += keyname + wxT( ":    " ) + hk_decr->m_InfoMsg + wxT( "\n" );
        }
    }

    DisplayInfoMessage( frame, msg );
}


/** function GetDescriptorFromHotkey
 * Return a Ki_HotkeyInfo * pointer fron a key code for OnHotKey() function
 * @param key = key code (ascii value, or wxWidgets value for function keys
 * @param List = pointer to a Ki_HotkeyInfo list of commands
 * @return the corresponding Ki_HotkeyInfo pointer from the Ki_HotkeyInfo List
 */
Ki_HotkeyInfo* GetDescriptorFromHotkey( int key, Ki_HotkeyInfo** List )
{
    for( ; *List != NULL; List++ )
    {
        Ki_HotkeyInfo* hk_decr = *List;
        if( hk_decr->m_KeyCode == key )
            return hk_decr;
    }

    return NULL;
}


/*
 * Create a configuration file (*.key) from the current hotkey list
 * @param Filename = default full file name to create. If void, A filename
 *    will be asked
 * @param List = pointer to the current hotkey list.
 * the ouput format is: shortcut  "key"  "function"
 * lines starting with # are comments
 *
 */
int WinEDA_BasicFrame::WriteHotkeyConfigFile(
    const wxString&                        Filename,
    struct Ki_HotkeyInfoSectionDescriptor* DescList,
    bool                                   verbose )
{
    wxString FullFilename = Filename;
    FILE*    cfgfile;
    wxString msg;

    if( FullFilename.IsEmpty() || verbose )
    {
        wxString Mask, Path, Ext;
        Ext  = DEFAULT_HOTKEY_FILENAME_EXT;
        Mask = wxT( "*." ) + Ext;
        Path = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFilename = EDA_FileSelector( _( "Save Hotkey Configuration File:" ),
                                         Path,
                                         FullFilename,
                                         Ext,
                                         Mask,
                                         this,
                                         wxFD_SAVE,
                                         TRUE );
    }

    if( FullFilename.IsEmpty() )
        return 0;

    cfgfile = wxFopen( FullFilename, wxT( "wt" ) );

    if( cfgfile == NULL )
    {
        if( verbose )
        {
            msg = _( "Unable to create " ) + FullFilename;
            DisplayError( this, msg );
        }
        return 0;
    }

    wxString keyname, infokey;

    msg = wxT( "$hotkey list\n" );
    fprintf( cfgfile, "%s", CONV_TO_UTF8( msg ) );

    /* print the allowed keys, for info
     */
    msg = wxT( "# " ); msg += _( "Allowed keys:\n" );
    fprintf( cfgfile, "%s", CONV_TO_UTF8( msg ) );
    msg.Empty();
    for( int ii = 0; ; ii++ )
    {
        if( s_Hotkey_Name_List[ii].m_KeyCode == 0 )
            break;;
        if( msg.IsEmpty() )
            msg = wxT( "# " );
        else
            msg += wxT( ", " );
        msg += s_Hotkey_Name_List[ii].m_Name;
        if( msg.Len() > 60 )
        {
            msg += wxT( "\n" );
            fprintf( cfgfile, "%s", CONV_TO_UTF8( msg ) );
            msg.Empty();
        }
    }

    /* print the last line of the info section */
    if( !msg.IsEmpty() )
        msg += wxT( "\n" );
    msg += wxT( "#\n#\n" );
    fprintf( cfgfile, "%s", CONV_TO_UTF8( msg ) );

    /* Print the current hotkey list */
    Ki_HotkeyInfo** List;
    for( ; DescList->m_HK_InfoList != NULL; DescList++ )
    {
        if( DescList->m_Comment )
        {
            fprintf( cfgfile, "# " );
            fprintf( cfgfile, "%s\n", DescList->m_Comment );
        }
        msg = *DescList->m_SectionTag;
        fprintf( cfgfile, "%s\n", CONV_TO_UTF8( msg ) );
        List = DescList->m_HK_InfoList;
        for( ; *List != NULL; List++ )
        {
            Ki_HotkeyInfo* hk_decr = *List;
            msg     = wxT( "shortcut   " );
            keyname = ReturnKeyNameFromKeyCode( hk_decr->m_KeyCode );
            AddDelimiterString( keyname );
            infokey = hk_decr->m_InfoMsg;
            AddDelimiterString( infokey );
            msg += keyname + wxT( ":    " ) + infokey + wxT( "\n" );
            fprintf( cfgfile, "%s", CONV_TO_UTF8( msg ) );
        }
    }

    msg = wxT( "$Endlist\n" );
    fprintf( cfgfile, "%s\n", CONV_TO_UTF8( msg ) );
    fclose( cfgfile );
    return 1;
}


/*
 * Read a configuration file (<file>.key) and fill the current hotkey list
 * with hotkeys
 * @param Filename = default full file name to create. If void, A filename
 *    will be asked
 * @param DescList = current hotkey list descr. to initialise.
 * the input format is: shortcut  "key"  "function"
 * lines starting by # are ignored (comments)
 * lines like [xxx] are tags (example: [common] or [libedit] which identify
 * sections
 *
 */
int WinEDA_BasicFrame::ReadHotkeyConfigFile(
    const wxString&                        Filename,
    struct Ki_HotkeyInfoSectionDescriptor* DescList,
    bool                                   verbose )
{
    wxString FullFilename = Filename;
    FILE*    cfgfile;
    wxString msg;

    if( FullFilename.IsEmpty() || verbose )
    {
        wxString Mask, Path, Ext;
        Ext  = DEFAULT_HOTKEY_FILENAME_EXT;
        Mask = wxT( "*." ) + Ext;
        Path = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFilename = EDA_FileSelector( _( "Open Hotkey Configuration File:" ),
                                         Path,
                                         FullFilename,
                                         Ext,
                                         Mask,
                                         this,
                                         wxFD_OPEN,
                                         TRUE );
        if( FullFilename.IsEmpty() )
            return 0;
    }

    cfgfile = wxFopen( FullFilename, wxT( "rt" ) );

    if( cfgfile == NULL )
    {
        if( verbose )
        {
            msg = _( "Unable to read " ) + FullFilename;
            DisplayError( this, msg );
        }
        return 0;
    }

    wxString        keyname;
    char            Line[1024];
    int             LineNum = 0;
    Ki_HotkeyInfo** CurrentHotkeyList = NULL;

    /* Read the file */
    while(  GetLine( cfgfile, Line, &LineNum ) != NULL )
    {
        char* line_type, * keyname, * fctname;
        line_type = strtok( Line, " \t\n\r" );
        msg = CONV_FROM_UTF8( line_type );
        if( msg[0]  == '[' ) // A tag is found. search infos in list
        {
            CurrentHotkeyList = NULL;
            Ki_HotkeyInfoSectionDescriptor* DList = DescList;
            for( ; DList->m_HK_InfoList != NULL; DList++ )
            {
                if( *DList->m_SectionTag == msg )
                {
                    CurrentHotkeyList = DList->m_HK_InfoList;
                    break;
                }
            }

            continue;
        }
        if( msg != wxT( "shortcut" ) )
            continue;
        if( msg == wxT( "$Endlist" ) )
            break;
        if( CurrentHotkeyList == NULL )
            continue;

        /* Get the key name */
        strtok( NULL, "\"\n\r" );
        keyname = strtok( NULL, "\"\n\r" );

        strtok( NULL, "\"\n\r" );

        /* Get the command name */
        fctname = strtok( NULL, "\"\n\r" );
        msg     = CONV_FROM_UTF8( fctname );

        /* search the hotkey in current hotkey list */
        for( Ki_HotkeyInfo** List = CurrentHotkeyList; *List != NULL; List++ )
        {
            Ki_HotkeyInfo* hk_decr = *List;
            if( hk_decr->m_InfoMsg == msg )
            {
                msg = CONV_FROM_UTF8( keyname );

                int code = ReturnKeyCodeFromKeyName( msg );
                if( code )
                    hk_decr->m_KeyCode = code;

                break;
            }
        }
    }

    fclose( cfgfile );
    return 1;
}


/* return the hotkey config file path
 * @param choice : 0 = home, 1 = kicad/share/template
 */
wxString ReturnHotkeyConfigFilePath( int choice )
{
    wxString     path;
    wxAppTraits* traits = wxGetApp().GetTraits();

    switch( choice )
    {
    case 0:
        path = traits->GetStandardPaths().GetUserConfigDir() +
               wxFileName::GetPathSeparator();

    case 1:

        /* TODO: This is broken under a normal Poxis system.  Users
         *       generally do no have write permissions to this path
         *       and there is no provision for prompting for the root
         *       password.  Suggest we remove this unless someone has
         *       a workable solution (Wayne).
         */
        path = ReturnKicadDatasPath() + wxT( "template/" );
        break;

    default:
        break;
    }

    return path;
}


/** add hotkey config options submenu to a menu
 * @param menu : initial menu
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

    /* (Re)create hotkey file */
    item = new wxMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_CREATE_CONFIG,
                           _( "(Re)create Hotkeys File" ),
                           _(
                               "Create or recreate the hotkey configuration file from current hotkey list" )
                           );
    item->SetBitmap( save_setup_xpm );
    HotkeySubmenu->Append( item );

    /* Reload hotkey file */
    item = new wxMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_READ_CONFIG,
                          _( "Reload Hotkeys File" ),
                          _( "Reload the hotkey configuration file" ) );
    item->SetBitmap( reload_xpm );
    HotkeySubmenu->Append( item );

    /* Edit hotkey file */
    item = new wxMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_EDIT_CONFIG,
                          _( "Edit Hotkeys File" ),
                          _( "Edit the hotkey configuration file in a text editor" ) );
    item->SetBitmap( editor_xpm );
    HotkeySubmenu->Append( item );

    /* Append HotkeySubmenu to menu */
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( aMenu, HotkeySubmenu,
                                        ID_PREFERENCES_HOTKEY_SUBMENU, _( "Hotkeys" ),
                                        _( "Hotkeys configuration and preferences" ), hotkeys_xpm );

    /* Hotkey path */
    wxMenu* HotkeyLocationSubmenu = new wxMenu();

    /* Home directory */
    item = new wxMenuItem( HotkeyLocationSubmenu,
                           ID_PREFERENCES_HOTKEY_PATH_IS_HOME,
                           _( "Home directory" ),
                           _( "Use home directory to load or store Hotkey config files" ),
                           wxITEM_CHECK );
    HotkeyLocationSubmenu->Append( item );

    /* KiCad template directory */
    item = new wxMenuItem( HotkeyLocationSubmenu,
                           ID_PREFERENCES_HOTKEY_PATH_IS_KICAD,
                           _( "KiCad template directory" ),
                           _( "Use kicad/template directory to load or store Hotkey config files" ),
                           wxITEM_CHECK );
    HotkeyLocationSubmenu->Append( item );

    /* Append location submenu to HotkeySubmenu */
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( HotkeySubmenu, HotkeyLocationSubmenu,
                                        -1, _( "Location" ),
                                        _( "Select hotkey configuration file location" ),
                                        right_xpm );
    HotkeyLocationSubmenu->Check( ID_PREFERENCES_HOTKEY_PATH_IS_HOME,
                                  g_ConfigFileLocationChoice == 0 );
    HotkeyLocationSubmenu->Check( ID_PREFERENCES_HOTKEY_PATH_IS_KICAD,
                                  g_ConfigFileLocationChoice == 1 );
}


/* called on hotkey file location selecton menu
 *  @param frame = current WinEDA_DrawFrame
 *  @param id = selected menu id
 *  @return g_ConfigFileLocationChoice (global) = new selection
 */
void HandleHotkeyConfigMenuSelection( WinEDA_DrawFrame* frame, int id )
{
    wxMenuBar* menu = frame->GetMenuBar();

    wxConfig*  config = wxGetApp().m_EDA_CommonConfig;

    switch( id )
    {
    case ID_PREFERENCES_HOTKEY_PATH_IS_HOME:
        if( g_ConfigFileLocationChoice != 0 )
        {
            g_ConfigFileLocationChoice = 0;
            menu->Check( ID_PREFERENCES_HOTKEY_PATH_IS_HOME, true );
            menu->Check( ID_PREFERENCES_HOTKEY_PATH_IS_KICAD, false );
            config->Write( HOTKEY_CFG_PATH_OPT, g_ConfigFileLocationChoice );
        }
        break;

    case ID_PREFERENCES_HOTKEY_PATH_IS_KICAD:
        if( g_ConfigFileLocationChoice != 1 )
        {
            g_ConfigFileLocationChoice = 1;
            menu->Check( ID_PREFERENCES_HOTKEY_PATH_IS_HOME, false );
            menu->Check( ID_PREFERENCES_HOTKEY_PATH_IS_KICAD, true );
            config->Write( HOTKEY_CFG_PATH_OPT, g_ConfigFileLocationChoice );
        }
        break;

    default:
        break;
    }
}
