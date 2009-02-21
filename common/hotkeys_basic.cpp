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


/* Class to handle hotkey commnands. hotkeys have a default value
 *  This class allows the real key code changed by user from a key code list file
 */

Ki_HotkeyInfo::Ki_HotkeyInfo( const wxChar* infomsg, int idcommand, int keycode, int idmenuevent )
{
    m_KeyCode     = keycode;        // Key code (ascii value for ascii keys or wxWidgets code for function key
    m_InfoMsg     = infomsg;        // info message.
    m_Idcommand   = idcommand;      // internal id for the corresponding command (see hotkey_id_commnand list)
    m_IdMenuEvent = idmenuevent;    // id to call the corresponding event (if any) (see id.h)
}


/* class to handle the printable name and the keycode
 */
struct hotkey_name_descr
{
    const wxChar* m_Name;
    int     m_KeyCode;
};


static struct hotkey_name_descr s_Hotkey_Name_List[] =
{
    { wxT( "F1" ),        WXK_F1           },
    { wxT( "F2" ),        WXK_F2           },
    { wxT( "F3" ),        WXK_F3           },
    { wxT( "F4" ),        WXK_F4           },
    { wxT( "F5" ),        WXK_F5           },
    { wxT( "F6" ),        WXK_F6           },
    { wxT( "F7" ),        WXK_F7           },
    { wxT( "F8" ),        WXK_F8           },
    { wxT( "F9" ),        WXK_F9           },
    { wxT( "F10" ),       WXK_F10          },
    { wxT( "F11" ),       WXK_F11          },
    { wxT( "F12" ),       WXK_F12          },

    { wxT( "Esc" ),       WXK_ESCAPE       },
    { wxT( "Delete" ),    WXK_DELETE       },
    { wxT( "Tab" ),       '\t'             },
    { wxT( "Backspace" ), WXK_BACK         },
    { wxT( "Insert" ),    WXK_INSERT       },

    { wxT( "End" ),       WXK_END          },
    { wxT( "Page Up" ),   WXK_PAGEUP       },
    { wxT( "Page Down" ), WXK_PAGEDOWN     },
    { wxT( "+" ),         '+'              },
    { wxT( "-" ),         '-'              },

    { wxT( "Up" ),        WXK_UP           },
    { wxT( "Down" ),      WXK_DOWN         },
    { wxT( "Left" ),      WXK_LEFT         },
    { wxT( "Right" ),     WXK_RIGHT        },

    { wxT( "space" ),     ' '              },
    { wxT( "?" ),         '?'              },
    { wxT( "!" ),         '!'              },
    { wxT( ":" ),         ':'              },
    { wxT( "," ),         ','              },
    { wxT( "*" ),         '*'              },
    { wxT( "+" ),         '+'              },
    { wxT( "-" ),         '-'              },
    { wxT( "\%" ),        '%'              },
    { wxT( "A" ),         'A'              },
    { wxT( "B" ),         'B'              },
    { wxT( "C" ),         'C'              },
    { wxT( "D" ),         'D'              },
    { wxT( "E" ),         'E'              },
    { wxT( "F" ),         'F'              },
    { wxT( "G" ),         'G'              },
    { wxT( "H" ),         'H'              },
    { wxT( "I" ),         'I'              },
    { wxT( "J" ),         'J'              },
    { wxT( "K" ),         'K'              },
    { wxT( "L" ),         'L'              },
    { wxT( "M" ),         'M'              },
    { wxT( "N" ),         'N'              },
    { wxT( "O" ),         'O'              },
    { wxT( "P" ),         'P'              },
    { wxT( "Q" ),         'Q'              },
    { wxT( "R" ),         'R'              },
    { wxT( "S" ),         'S'              },
    { wxT( "T" ),         'T'              },
    { wxT( "U" ),         'U'              },
    { wxT( "V" ),         'V'              },
    { wxT( "W" ),         'W'              },
    { wxT( "X" ),         'X'              },
    { wxT( "Y" ),         'Y'              },
    { wxT( "Z" ),         'Z'              },

    { wxT( "Ctrl A" ),    GR_KB_CTRL + 'A' },
    { wxT( "Ctrl B" ),    GR_KB_CTRL + 'B' },
    { wxT( "Ctrl C" ),    GR_KB_CTRL + 'C' },
    { wxT( "Ctrl D" ),    GR_KB_CTRL + 'D' },
    { wxT( "Ctrl E" ),    GR_KB_CTRL + 'E' },
    { wxT( "Ctrl F" ),    GR_KB_CTRL + 'F' },
    { wxT( "Ctrl G" ),    GR_KB_CTRL + 'G' },
    { wxT( "Ctrl H" ),    GR_KB_CTRL + 'H' },
    { wxT( "Ctrl I" ),    GR_KB_CTRL + 'I' },
    { wxT( "Ctrl J" ),    GR_KB_CTRL + 'J' },
    { wxT( "Ctrl K" ),    GR_KB_CTRL + 'K' },
    { wxT( "Ctrl L" ),    GR_KB_CTRL + 'L' },
    { wxT( "Ctrl M" ),    GR_KB_CTRL + 'M' },
    { wxT( "Ctrl N" ),    GR_KB_CTRL + 'N' },
    { wxT( "Ctrl O" ),    GR_KB_CTRL + 'O' },
    { wxT( "Ctrl P" ),    GR_KB_CTRL + 'P' },
    { wxT( "Ctrl Q" ),    GR_KB_CTRL + 'Q' },
    { wxT( "Ctrl R" ),    GR_KB_CTRL + 'R' },
    { wxT( "Ctrl S" ),    GR_KB_CTRL + 'S' },
    { wxT( "Ctrl T" ),    GR_KB_CTRL + 'T' },
    { wxT( "Ctrl U" ),    GR_KB_CTRL + 'U' },
    { wxT( "Ctrl V" ),    GR_KB_CTRL + 'V' },
    { wxT( "Ctrl W" ),    GR_KB_CTRL + 'W' },
    { wxT( "Ctrl X" ),    GR_KB_CTRL + 'X' },
    { wxT( "Ctrl Y" ),    GR_KB_CTRL + 'Y' },
    { wxT( "Ctrl Z" ),    GR_KB_CTRL + 'Z' },

    { wxT( "" ),          0                } // Do not change: end of list
};


/****************************************************/
wxString ReturnKeyNameFromKeyCode( int keycode )
/****************************************************/

/*
 * return the key name from the key code
 * Only some wxWidgets key values are handled for function key ( see s_Hotkey_Name_List[] )
 * @param key = key code (ascii value, or wxWidgets value for function keys)
 * @return the key name in a wxString
 */
{
    wxString keyname, modifier, fullkeyname;
    int      ii;

    if( (keycode & GR_KB_CTRL) != 0 )
        modifier << wxT( "Ctrl " );
    if( (keycode & GR_KB_ALT) != 0 )
        modifier << wxT( "Alt " );
    if( (keycode & GR_KB_SHIFT) != 0 )
        modifier << wxT( "Shift " );

    keycode &= ~(GR_KB_CTRL | GR_KB_ALT | GR_KB_SHIFT);
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


/**********************************************************************************/
wxString AddHotkeyName( const wxString& text, Ki_HotkeyInfo** List, int CommandId )
/**********************************************************************************/

/*
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param List = pointer to a Ki_HotkeyInfo list of commands
 * @param CommandId = Command Id value
 * @return text (key name) in a wxString if found or text without modification
 */
{
    wxString msg     = text;
    wxString keyname = ReturnKeyNameFromCommandId( List, CommandId );

    if( !keyname.IsEmpty() )
        msg << wxT( " (" ) << keyname << wxT( ")" );
    return msg;
}


/***********************************************************/
wxString    AddHotkeyName( const wxString&                        text,
                           struct Ki_HotkeyInfoSectionDescriptor* DescList,
                           int                                    CommandId )
/***********************************************************/

/*
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param List = pointer to a Ki_HotkeyInfoSectionDescriptor* DescrList of commands
 * @param CommandId = Command Id value
 * @return text (key name) in a wxString if found or text without modification
 */
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
            msg << wxT( " (" ) << keyname << wxT( ")" );
            break;
        }
    }

    return msg;
}


/*************************************************************************/
wxString ReturnKeyNameFromCommandId( Ki_HotkeyInfo** List, int CommandId )
/*************************************************************************/

/*
 * return the key name from the Command id value ( m_Idcommand member value)
 * @param List = pointer to a Ki_HotkeyInfo list of commands
 * @param CommandId = Command Id value
 * @return the key name in a wxString
 */
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


/************************************************************/
static int ReturnKeyCodeFromKeyName( const wxString& keyname )
/************************************************************/

/*
 * return the key code from its key name
 * Only some wxWidgets key values are handled for function key
 * @param keyname = wxString key name to find in s_Hotkey_Name_List[], like F2 or space or an usual (ascii) char
 * @return the key code
 */
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


/********************************************************************************************/
void DisplayHotkeyList( WinEDA_DrawFrame* frame, struct Ki_HotkeyInfoSectionDescriptor* DescList )
/***************************************************************************************/

/*
 * Displays the current hotkey list
 * @param frame = current active frame
 * @param List = pointer to a Ki_HotkeyInfoSectionDescriptor list (Null terminated)
 * @return none
 */
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
            msg += keyname + wxT( ":    " ) + hk_decr->m_InfoMsg + wxT( "\n" );
        }
    }

    DisplayInfo( frame, msg );
}


/************************************************************************/
Ki_HotkeyInfo* GetDescriptorFromHotkey( int key, Ki_HotkeyInfo** List )
/***********************************************************************/

/*
 * Return a Ki_HotkeyInfo * pointer fron a key code for OnHotKey() function
 * @param key = key code (ascii value, or wxWidgets value for function keys
 * @param List = pointer to a Ki_HotkeyInfo list of commands
 * @return the corresponding Ki_HotkeyInfo * pointer from the Ki_HotkeyInfo  List
 */
{
    for( ; *List != NULL; List++ )
    {
        Ki_HotkeyInfo* hk_decr = *List;
        if( hk_decr->m_KeyCode == key )
            return hk_decr;
    }

    return NULL;
}


/*************************************************************************/
int WinEDA_BasicFrame::WriteHotkeyConfigFile( const wxString&                        Filename,
                                              struct Ki_HotkeyInfoSectionDescriptor* DescList,
                                              bool                                   verbose )
/*************************************************************************/

/*
 * Create a configuration file (*.key) from the current hotkey list
 * @param Filename = default full file name to create. If void, A filename will be asked
 * @param List = pointer to the current hotkey list.
 * the ouput format is: shortcut  "key"  "function"
 * lines starting with # are comments
 *
 */
{
    wxString FullFilename = Filename;
    FILE*    cfgfile;
    wxString msg;

    if( FullFilename.IsEmpty() || verbose )
    {
        wxString Mask, Path, Ext;
        Ext  = DEFAULT_HOTKEY_FILENAME_EXT;
        Mask = wxT( "*" ) + Ext;
        Path = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFilename = EDA_FileSelector( _( "Save Hotkey Configuration File:" ),
                                         Path,          /* Chemin par defaut */
                                         FullFilename,  /* nom fichier par defaut */
                                         Ext,           /* extension par defaut */
                                         Mask,          /* Masque d'affichage */
                                         this,
                                         wxFD_SAVE,
                                         TRUE
                                         );
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
    fprintf( cfgfile, CONV_TO_UTF8( msg ) );

    /* print the allowed keys, for info
     */
    msg = wxT( "# " ); msg += _( "Allowed keys:\n" );
    fprintf( cfgfile, CONV_TO_UTF8( msg ) );
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
            fprintf( cfgfile, CONV_TO_UTF8( msg ) );
            msg.Empty();
        }
    }

    /* print the last line of the info section */
    if( !msg.IsEmpty() )
        msg += wxT( "\n" );
    msg += wxT( "#\n#\n" );
    fprintf( cfgfile, CONV_TO_UTF8( msg ) );

    /* Print the current hotkey list */
    Ki_HotkeyInfo** List;
    for( ; DescList->m_HK_InfoList != NULL; DescList++ )
    {
        if( DescList->m_Comment )
        {
            fprintf( cfgfile, "# " );
            fprintf( cfgfile, DescList->m_Comment );
            fprintf( cfgfile, "\n" );
        }
        msg = *DescList->m_SectionTag;
        fprintf( cfgfile, CONV_TO_UTF8( msg ) );
        fprintf( cfgfile, "\n" );
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
            fprintf( cfgfile, CONV_TO_UTF8( msg ) );
        }
    }

    msg = wxT( "$Endlist\n" );
    fprintf( cfgfile, CONV_TO_UTF8( msg ) );
    fclose( cfgfile );
    return 1;
}


/********************************************************************************************/
int WinEDA_BasicFrame::ReadHotkeyConfigFile( const wxString&                        Filename,
                                             struct Ki_HotkeyInfoSectionDescriptor* DescList,
                                             bool                                   verbose )
/********************************************************************************************/

/*
 * Read a configuration file (<file>.key) and fill the current hotkey list with hotkeys
 * @param Filename = default full file name to create. If void, A filename will be asked
 * @param DescList = current hotkey list descr. to initialise.
 * the input format is: shortcut  "key"  "function"
 * lines starting by # are ignored (comments)
 * lines like [xxx] are tags (example: [common] or [libedit] which identify sections
 *
 */
{
    wxString FullFilename = Filename;
    FILE*    cfgfile;
    wxString msg;

    if( FullFilename.IsEmpty() || verbose )
    {
        wxString Mask, Path, Ext;
        Ext  = DEFAULT_HOTKEY_FILENAME_EXT;
        Mask = wxT( "*" ) + Ext;
        Path = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFilename = EDA_FileSelector( _( "Open Hotkey Configuration File:" ),
                                         Path,          /* Chemin par defaut */
                                         FullFilename,  /* nom fichier par defaut */
                                         Ext,           /* extension par defaut */
                                         Mask,          /* Masque d'affichage */
                                         this,
                                         wxFD_OPEN,
                                         TRUE
                                         );
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
        msg = CONV_FROM_UTF8( fctname );

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


/****************************************************/
wxString    ReturnHotkeyConfigFilePath( int choice )
/****************************************************/

/* return the hotkey config file path
 * @param choice : 0 = home, 1 = kicad/share/template
 */
{
    wxString path;

    switch( choice )
    {
    case 0:
        path = wxGetHomeDir() + wxT( "/" );
        break;

    case 1:
        path = ReturnKicadDatasPath() + wxT( "template/" );
        break;

    default:
        break;
    }

    return path;
}


/***************************************/
void AddHotkeyConfigMenu( wxMenu* menu )
/***************************************/

/** add hotkey config options to a menu
 * @param menu : initial menu
 */
{
    wxMenuItem* item;

    if( menu == NULL )
        return;

    item = new wxMenuItem( menu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                           _( "Show Current Hotkey List" ),
                           _( "Show the current hotkey config" )
                           );
    item->SetBitmap( info_xpm );
    menu->Append( item );

    item = new wxMenuItem( menu, ID_PREFERENCES_CREATE_CONFIG_HOTKEYS,
                           _( "Create Hotkey config file" ),
                           _( "Create or Recreate the hotkey config file from current hotkey list" )
                           );
    item->SetBitmap( save_setup_xpm );
    menu->Append( item );

    item = new wxMenuItem( menu, ID_PREFERENCES_READ_CONFIG_HOTKEYS,
                          _( "Reread Hotkey config file" ),
                          _( "Reread the hotkey config file" ) );
    item->SetBitmap( reload_xpm );
    menu->Append( item );
    item = new wxMenuItem( menu, ID_PREFERENCES_EDIT_CONFIG_HOTKEYS,
                          _( "Edit Hotkey config file" ),
                          _( "Run the text editor and edit the hotkey config file" ) );
    item->SetBitmap( editor_xpm );
    menu->Append( item );

    wxMenu* submenu_hkcfg = new wxMenu();
    item = new wxMenuItem( submenu_hkcfg, ID_PREFERENCES_HOTKEY_PATH_IS_HOME,
                           _( "home directory" ),
                           _( "Use home directory to load or store Hotkey config files" ),
                           wxITEM_CHECK );
    submenu_hkcfg->Append( item );

    item = new wxMenuItem( submenu_hkcfg, ID_PREFERENCES_HOTKEY_PATH_IS_KICAD,
                           _( "kicad/template directory" ),
                           _( "Use kicad/template directory to load or store Hotkey config files" ),
                           wxITEM_CHECK );
    submenu_hkcfg->Append( item );

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( menu, submenu_hkcfg,
                                        -1,
                                        _( "Hotkey config location" ),
                                        _(
                                           "Hotkey config file location selection (home directory or kicad tree)" ),
                                        right_xpm );
    submenu_hkcfg->Check( ID_PREFERENCES_HOTKEY_PATH_IS_HOME,
                          g_ConfigFileLocationChoice == 0 );
    submenu_hkcfg->Check( ID_PREFERENCES_HOTKEY_PATH_IS_KICAD,
                          g_ConfigFileLocationChoice == 1 );
}


/************************************************************************/
void  HandleHotkeyConfigMenuSelection( WinEDA_DrawFrame* frame, int id )
/************************************************************************/

/* called on hotkey file location selecton menu
 *  @param frame = current WinEDA_DrawFrame
 *  @param id = selected menu id
 *  @return g_ConfigFileLocationChoice (global) = new selection
 */
{
    wxMenuBar* menu = frame->GetMenuBar();
    wxConfig * config = wxGetApp().m_EDA_CommonConfig;
    wxASSERT( config != NULL );

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
