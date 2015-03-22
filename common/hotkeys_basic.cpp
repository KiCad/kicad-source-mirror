/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, j-p.charras at wanadoo.fr
 * Copyright (C) 2010-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file hotkeys_basic.cpp
 * @brief Some functions to handle hotkeys in KiCad
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <hotkeys_basic.h>
#include <id.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <wxstruct.h>
#include <macros.h>
#include <dialog_hotkeys_editor.h>
#include <menus_helpers.h>

#include <wx/apptrait.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>

#define HOTKEYS_CONFIG_KEY wxT( "Keys" )

wxString g_CommonSectionTag( wxT( "[common]" ) );
wxString g_SchematicSectionTag( wxT( "[eeschema]" ) );
wxString g_LibEditSectionTag( wxT( "[libedit]" ) );
wxString g_BoardEditorSectionTag( wxT( "[pcbnew]" ) );
wxString g_ModuleEditSectionTag( wxT( "[footprinteditor]" ) );

wxString g_CommonSectionTitle( wxT( "Common" ) );
wxString g_SchematicSectionTitle( wxT( "Schematic Editor" ) );
wxString g_LibEditSectionTitle( wxT( "Library Editor" ) );
wxString g_BoardEditorSectionTitle( wxT( "Board Editor" ) );
wxString g_ModuleEditSectionTitle( wxT( "Footprint Editor" ) );


/* Class to handle hotkey commnands. hotkeys have a default value
 * This class allows the real key code changed by user from a key code list
 * file.
 */

EDA_HOTKEY::EDA_HOTKEY( const wxChar* infomsg, int idcommand, int keycode, int idmenuevent )
{
    m_KeyCode = keycode;            // Key code (ascii value for ascii keys

    // or wxWidgets code for function key
    m_InfoMsg   = infomsg;          // info message.
    m_Idcommand = idcommand;        // internal id for the corresponding

    // command (see hotkey_id_commnand list)
    m_IdMenuEvent = idmenuevent;    // id to call the corresponding event
    // (if any) (see id.h)
}


EDA_HOTKEY::EDA_HOTKEY( const EDA_HOTKEY* base )
{
    m_KeyCode     = base->m_KeyCode;
    m_InfoMsg     = base->m_InfoMsg;
    m_Idcommand   = base->m_Idcommand;
    m_IdMenuEvent = base->m_IdMenuEvent;
}


EDA_HOTKEY_CLIENT_DATA::~EDA_HOTKEY_CLIENT_DATA()
{
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

    { wxT( "Return" ),       WXK_RETURN                                                },

    { wxT( "Space" ),        WXK_SPACE                                                },

    // Do not change this line: end of list
    { wxT( "" ),             0                                                        }
};

#define MODIFIER_CTRL   wxT( "Ctrl+" )
#define MODIFIER_ALT    wxT( "Alt+" )
#define MODIFIER_SHIFT  wxT( "Shift+" )


/**
 * Function KeyNameFromKeyCode
 * return the key name from the key code
 * Only some wxWidgets key values are handled for function key ( see
 * s_Hotkey_Name_List[] )
 * @param aKeycode = key code (ascii value, or wxWidgets value for function keys)
 * @param aIsFound = a pointer to a bool to return true if found, or false. an be NULL default)
 * @return the key name in a wxString
 */
wxString KeyNameFromKeyCode( int aKeycode, bool* aIsFound )
{
    wxString keyname, modifier, fullkeyname;
    int      ii;
    bool     found = false;

    // Assume keycode of 0 is "unassigned"
    if( aKeycode == 0 )
        return wxT( "<unassigned>");

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
        keyname.Append( (wxChar)aKeycode );
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


/*
 * helper function use in AddHotkeyName to calculate an accelerator string
 * In some menus, accelerators do not perform exactly the same action as
 * the hotkey that perform a similar action.
 * this is usually the case when this action uses the current mouse position
 * for instance zoom action is ran from the F1 key or the Zoom menu.
 * a zoom uses the mouse position from a hot key and not from the menu
 * In this case, the accelerator if Shift+<hotkey>
 * But for many keys, the Shift modifier is not usable, and the accelerator is Alt+<hotkey>
 */
static void AddModifierToKey( wxString& aFullKey, const wxString & aKey )
{
    if( (aKey.Length() == 1) && (aKey[0] >= 'A')  && (aKey[0] <= 'Z'))
        // We can use Shift+<key> as accelerator and <key> for hot key
        aFullKey << wxT( "\t" ) << MODIFIER_SHIFT << aKey;
    else
        // We must use Alt+<key> as accelerator and <key> for hot key
        aFullKey << wxT( "\t" ) << MODIFIER_ALT << aKey;
}


/* AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 *  aText = a wxString. returns aText + key name
 *  aList = pointer to a EDA_HOTKEY list of commands
 *  aCommandId = Command Id value
 *  aShortCutType = IS_HOTKEY to add <tab><keyname> (shortcuts in menus, same as hotkeys)
 *                  IS_ACCELERATOR to add <tab><Shift+keyname> (accelerators in menus, not hotkeys)
 *                  IS_COMMENT to add <spaces><(keyname)> mainly in tool tips
 *  Return a wxString (aTest + key name) if key found or aText without modification
 */
wxString AddHotkeyName( const wxString& aText, EDA_HOTKEY** aList,
                        int aCommandId, HOTKEY_ACTION_TYPE aShortCutType )
{
    wxString msg = aText;
    wxString keyname;

    if( aList )
        keyname = KeyNameFromCommandId( aList, aCommandId );

    if( !keyname.IsEmpty() )
    {
        switch( aShortCutType )
        {
        case IS_HOTKEY:
            msg << wxT( "\t" ) << keyname;
            break;

        case IS_ACCELERATOR:
            AddModifierToKey( msg, keyname );
            break;

        case IS_COMMENT:
            msg << wxT( " (" ) << keyname << wxT( ")" );
            break;
        }
    }

    return msg;
}


/* AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 *  aText = a wxString. returns aText + key name
 *  aList = pointer to a EDA_HOTKEY_CONFIG DescrList of commands
 *  aCommandId = Command Id value
 *  aShortCutType = IS_HOTKEY to add <tab><keyname> (active shortcuts in menus)
 *                  IS_ACCELERATOR to add <tab><Shift+keyname> (active accelerators in menus)
 *                  IS_COMMENT to add <spaces><(keyname)>
 * Return a wxString (aText + key name) if key found or aText without modification
 */
wxString AddHotkeyName( const wxString&           aText,
                        struct EDA_HOTKEY_CONFIG* aDescList,
                        int                       aCommandId,
                        HOTKEY_ACTION_TYPE        aShortCutType )
{
    wxString     msg = aText;
    wxString     keyname;
    EDA_HOTKEY** list;

    if( aDescList )
    {
        for( ; aDescList->m_HK_InfoList != NULL; aDescList++ )
        {
            list    = aDescList->m_HK_InfoList;
            keyname = KeyNameFromCommandId( list, aCommandId );

            if( !keyname.IsEmpty() )
            {
                switch( aShortCutType )
                {
                case IS_HOTKEY:
                    msg << wxT( "\t" ) << keyname;
                    break;

                case IS_ACCELERATOR:
                    AddModifierToKey( msg, keyname );
                    break;

                case IS_COMMENT:
                    msg << wxT( " (" ) << keyname << wxT( ")" );
                    break;
                }

                break;
            }
        }
    }

    return msg;
}


/**
 * Function KeyNameFromCommandId
 * return the key name from the Command id value ( m_Idcommand member value)
 * @param aList = pointer to a EDA_HOTKEY list of commands
 * @param aCommandId = Command Id value
 * @return the key name in a wxString
 */
wxString KeyNameFromCommandId( EDA_HOTKEY** aList, int aCommandId )
{
    wxString keyname;

    for( ; *aList != NULL; aList++ )
    {
        EDA_HOTKEY* hk_decr = *aList;

        if( hk_decr->m_Idcommand == aCommandId )
        {
            keyname = KeyNameFromKeyCode( hk_decr->m_KeyCode );
            break;
        }
    }

    return keyname;
}


/**
 * Function KeyCodeFromKeyName
 * return the key code from its key name
 * Only some wxWidgets key values are handled for function key
 * @param keyname = wxString key name to find in s_Hotkey_Name_List[],
 *   like F2 or space or an usual (ascii) char.
 * @return the key code
 */
int KeyCodeFromKeyName( const wxString& keyname )
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
        {
            break;
        }
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


/* DisplayHotkeyList
 * Displays the current hotkey list
 * aList = a EDA_HOTKEY_CONFIG list(Null terminated)
 */
#include <html_messagebox.h>

void DisplayHotkeyList( EDA_BASE_FRAME* aFrame, struct EDA_HOTKEY_CONFIG* aDescList )
{
    wxString     keyname;
    wxString     keymessage;
    EDA_HOTKEY** list;

    wxString     msg = wxT( "<html><body bgcolor=\"#E2E2E2\">" );

    msg += wxT( "<H3>" );
    msg += _( "Hotkeys List" );
    msg += wxT( "</H3> <table cellpadding=\"0\">" );

    for( ; aDescList->m_HK_InfoList != NULL; aDescList++ )
    {
        list = aDescList->m_HK_InfoList;

        for( ; *list != NULL; list++ )
        {
            EDA_HOTKEY* hk_decr = *list;

            if( !hk_decr->m_InfoMsg.Contains( wxT( "Macros" ) ) )
            {
                keyname = KeyNameFromKeyCode( hk_decr->m_KeyCode );
                keymessage = wxGetTranslation( hk_decr->m_InfoMsg );

                // Some chars are modified, using html encoding, to be
                // displayed by DisplayHtmlInfoMessage()
                keyname.Replace( wxT( "<" ), wxT( "&lt;" ) );
                keyname.Replace( wxT( ">" ), wxT( "&gt;" ) );
                msg    += wxT( "<tr><td>" ) + keymessage + wxT( "</td>" );
                msg    += wxT( "<td><b>&nbsp;&nbsp;" ) + keyname + wxT( "</b></td></tr>" );
            }
        }
    }

    msg += wxT( "</table></html></body>" );

#if 0   // Set to 1 to create a modal dialog (blocking)
    DisplayHtmlInfoMessage( aFrame, _( "Hotkeys List" ), msg, wxSize( 340, 750 ) );
#else
    // Create a non modal dialog, which shows the list of hotkeys until dismissed
    // but does not block the parent window
    HTML_MESSAGE_BOX *dlg = new HTML_MESSAGE_BOX( aFrame, _( "Hotkeys List" ),
                                        wxDefaultPosition, wxSize( 340, 750 ) );
    dlg->AddHTML_Text( msg );
    dlg->Show( true );
#endif
}


/**
 * Function GetDescriptorFromHotkey
 * Return a EDA_HOTKEY * pointer from a key code for OnHotKey() function
 * @param aKey = key code (ascii value, or wxWidgets value for function keys
 * @param aList = pointer to a EDA_HOTKEY list of commands
 * @return the corresponding EDA_HOTKEY pointer from the EDA_HOTKEY List
 */
EDA_HOTKEY* GetDescriptorFromHotkey( int aKey, EDA_HOTKEY** aList )
{
    for( ; *aList != NULL; aList++ )
    {
        EDA_HOTKEY* hk_decr = *aList;

        if( hk_decr->m_KeyCode == aKey )
            return hk_decr;
    }

    return NULL;
}


int EDA_BASE_FRAME::WriteHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList,
                                       wxString*                 aFullFileName )
{
    wxString msg;
    wxString keyname, infokey;

    msg = wxT( "$hotkey list\n" );

    // Print the current hotkey list
    EDA_HOTKEY** list;

    for( ; aDescList->m_HK_InfoList != NULL; aDescList++ )
    {
        if( aDescList->m_Title )
        {
            msg += wxT( "# " );
            msg += *aDescList->m_Title;
            msg += wxT( "\n" );
        }

        msg += *aDescList->m_SectionTag;
        msg += wxT( "\n" );

        list = aDescList->m_HK_InfoList;

        for( ; *list != NULL; list++ )
        {
            EDA_HOTKEY* hk_decr = *list;
            msg    += wxT( "shortcut   " );
            keyname = KeyNameFromKeyCode( hk_decr->m_KeyCode );
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
        {
            fputs( TO_UTF8( msg ), file );
            fclose( file );
        }
        else
        {
            msg.Printf( wxT( "Unable to write file %s" ), GetChars( *aFullFileName ) );
            return 0;
        }
    }
    else
    {
        wxFileName fn( GetName() );
        fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );
        wxConfigBase* config = GetNewConfig( fn.GetFullPath() );
        config->Write( HOTKEYS_CONFIG_KEY, msg );
        delete config;
    }

    return 1;
}


int EDA_BASE_FRAME::ReadHotkeyConfigFile( const wxString&           aFilename,
                                          struct EDA_HOTKEY_CONFIG* aDescList )
{
    wxFileName fn( aFilename );
    fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );

    wxFile cfgfile( fn.GetFullPath() );

    if( !cfgfile.IsOpened() )       // There is a problem to open file
        return 0;

    // get length
    cfgfile.SeekEnd();
    wxFileOffset size = cfgfile.Tell();
    cfgfile.Seek( 0 );

    // read data
    char*    buffer = new char[size];
    cfgfile.Read( buffer, size );

    wxString data( buffer, wxConvUTF8 );

    // parse
    ParseHotkeyConfig( data, aDescList );

    // cleanup
    delete[] buffer;
    cfgfile.Close();
    return 1;
}


void ReadHotkeyConfig( const wxString& Appname, struct EDA_HOTKEY_CONFIG* aDescList )
{
    wxFileName fn( Appname );
    fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );

    wxConfigBase* config = GetNewConfig( fn.GetFullPath() );

    if( !config->HasEntry( HOTKEYS_CONFIG_KEY ) )
    {
        // assume defaults are ok
        return;
    }

    wxString data;
    config->Read( HOTKEYS_CONFIG_KEY, &data );
    delete config;

    ParseHotkeyConfig( data, aDescList );
}


/* Function ReadHotkeyConfig
 * Read configuration data and fill the current hotkey list with hotkeys
 * aDescList is the current hotkey list descr. to initialize.
 */
int EDA_BASE_FRAME::ReadHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList )
{
    ::ReadHotkeyConfig( GetName(), aDescList );
    return 1;
}


/* Function ParseHotkeyConfig
 * the input format is: shortcut  "key"  "function"
 * lines starting by # are ignored (comments)
 * lines like [xxx] are tags (example: [common] or [libedit] which identify sections
 */
void ParseHotkeyConfig( const wxString&           data,
                        struct EDA_HOTKEY_CONFIG* aDescList )
{
    // Read the config
    wxStringTokenizer tokenizer( data, L"\r\n", wxTOKEN_STRTOK );
    EDA_HOTKEY**      CurrentHotkeyList = 0;

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
            EDA_HOTKEY_CONFIG* DList = aDescList;

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

        // Get the key name
        lineTokenizer.SetString( lineTokenizer.GetString(), L"\"\r\n\t ", wxTOKEN_STRTOK );
        wxString keyname = lineTokenizer.GetNextToken();

        wxString remainder = lineTokenizer.GetString();

        // Get the command name
        wxString fctname = remainder.AfterFirst( '\"' ).BeforeFirst( '\"' );

        // search the hotkey in current hotkey list
        for( EDA_HOTKEY** list = CurrentHotkeyList; *list != NULL; list++ )
        {
            EDA_HOTKEY* hk_decr = *list;

            if( hk_decr->m_InfoMsg == fctname )
            {
                int code = KeyCodeFromKeyName( keyname );

                if( code )
                    hk_decr->m_KeyCode = code;

                break;
            }
        }
    }
}


void EDA_BASE_FRAME::ImportHotkeyConfigFromFile( EDA_HOTKEY_CONFIG* aDescList,
                                            const wxString& aDefaultShortname )
{
    wxString ext  = DEFAULT_HOTKEY_FILENAME_EXT;
    wxString mask = wxT( "*." ) + ext;

#if 0   // pass in the project dir as an argument
    wxString path = wxPathOnly( Prj().GetProjectFullName() );
#else
    wxString path = wxGetCwd();
#endif
    wxFileName fn( aDefaultShortname );
    fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );

    wxString  filename = EDA_FileSelector( _( "Read Hotkey Configuration File:" ),
                                 path,
                                 fn.GetFullPath(),
                                 ext,
                                 mask,
                                 this,
                                 wxFD_OPEN,
                                 true );

    if( filename.IsEmpty() )
        return;

    ReadHotkeyConfigFile( filename, aDescList );
}


void EDA_BASE_FRAME::ExportHotkeyConfigToFile( EDA_HOTKEY_CONFIG* aDescList,
                                        const wxString& aDefaultShortname )
{
    wxString ext  = DEFAULT_HOTKEY_FILENAME_EXT;
    wxString mask = wxT( "*." ) + ext;

#if 0
    wxString path = wxPathOnly( Prj().GetProjectFullName() );
#else
    wxString path = wxGetCwd();
#endif
    wxFileName fn( aDefaultShortname );
    fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );

    wxString filename = EDA_FileSelector( _( "Write Hotkey Configuration File:" ),
                                 path,
                                 fn.GetFullPath(),
                                 ext,
                                 mask,
                                 this,
                                 wxFD_OPEN | wxFD_SAVE,
                                 true );

    if( filename.IsEmpty() )
        return;

    WriteHotkeyConfig( aDescList, &filename );
}


/* add hotkey config options submenu to aMenu
 */
void AddHotkeyConfigMenu( wxMenu* aMenu )
{
    if( aMenu == NULL )
        return;

    wxMenu*     HotkeySubmenu = new wxMenu();

    // List existing hotkey menu
    AddMenuItem( HotkeySubmenu,
                 ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                 _( "&List Current Keys" ),
                 _( "Displays the current hotkeys list and corresponding commands" ),
                 KiBitmap( info_xpm ) );

    // Call hotkeys editor
    AddMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_SHOW_EDITOR,
                 _( "&Edit Hotkeys" ),
                 _( "Call the hotkeys editor" ),
                 KiBitmap( editor_xpm ) );

    HotkeySubmenu->AppendSeparator();

    // create hotkey file to export current hotkeys config
    AddMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_EXPORT_CONFIG,
                 _( "E&xport Hotkeys" ),
                 _( "Create a hotkey configuration file to export the current hotkeys" ),
                 KiBitmap( save_setup_xpm ) );

    // Reload hotkey file
    AddMenuItem( HotkeySubmenu, ID_PREFERENCES_HOTKEY_IMPORT_CONFIG,
                 _( "&Import Hotkeys" ),
                 _( "Load an existing hotkey configuration file" ),
                 KiBitmap( reload_xpm ) );

    // Append HotkeySubmenu to menu
    AddMenuItem( aMenu, HotkeySubmenu,
                 wxID_ANY, _( "&Hotkeys" ),
                 _( "Hotkeys configuration and preferences" ),
                 KiBitmap( hotkeys_xpm ) );
}
