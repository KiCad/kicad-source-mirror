/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiface_i.h>
#include <hotkeys_basic.h>
#include <id.h>
#include <string_utils.h>
#include <eda_base_frame.h>
#include <eda_draw_frame.h>
#include <wildcards_and_files_ext.h>
#include <settings/settings_manager.h>

#include <tool/tool_manager.h>
#include "dialogs/dialog_hotkey_list.h"
#include <wx/apptrait.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <tool/tool_action.h>


/*
 * class to handle the printable name and the keycode
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
 *        "Alt+A","Ctrl+F1", ...
 */
#define KEY_NON_FOUND -1
static struct hotkey_name_descr hotkeyNameList[] =
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
    { wxT( "Back" ),         WXK_BACK                                                 },
    { wxT( "Ins" ),          WXK_INSERT                                               },

    { wxT( "Home" ),         WXK_HOME                                                 },
    { wxT( "End" ),          WXK_END                                                  },
    { wxT( "PgUp" ),         WXK_PAGEUP                                               },
    { wxT( "PgDn" ),         WXK_PAGEDOWN                                             },

    { wxT( "Up" ),           WXK_UP                                                   },
    { wxT( "Down" ),         WXK_DOWN                                                 },
    { wxT( "Left" ),         WXK_LEFT                                                 },
    { wxT( "Right" ),        WXK_RIGHT                                                },

    { wxT( "Return" ),       WXK_RETURN                                               },

    { wxT( "Space" ),        WXK_SPACE                                                },

    { wxT( "Num Pad 0" ),    WXK_NUMPAD0                                              },
    { wxT( "Num Pad 1" ),    WXK_NUMPAD1                                              },
    { wxT( "Num Pad 2" ),    WXK_NUMPAD2                                              },
    { wxT( "Num Pad 3" ),    WXK_NUMPAD3                                              },
    { wxT( "Num Pad 4" ),    WXK_NUMPAD4                                              },
    { wxT( "Num Pad 5" ),    WXK_NUMPAD5                                              },
    { wxT( "Num Pad 6" ),    WXK_NUMPAD6                                              },
    { wxT( "Num Pad 7" ),    WXK_NUMPAD7                                              },
    { wxT( "Num Pad 8" ),    WXK_NUMPAD8                                              },
    { wxT( "Num Pad 9" ),    WXK_NUMPAD9                                              },
    { wxT( "Num Pad +" ),    WXK_NUMPAD_ADD                                           },
    { wxT( "Num Pad -" ),    WXK_NUMPAD_SUBTRACT                                      },
    { wxT( "Num Pad *" ),    WXK_NUMPAD_MULTIPLY                                      },
    { wxT( "Num Pad /" ),    WXK_NUMPAD_DIVIDE                                        },
    { wxT( "Num Pad ." ),    WXK_NUMPAD_SEPARATOR                                     },

    { wxT( "" ),             0                                                        },

    { wxT( "Click" ),        PSEUDO_WXK_CLICK                                         },
    { wxT( "DblClick" ),     PSEUDO_WXK_DBLCLICK                                      },
    { wxT( "Wheel" ),        PSEUDO_WXK_WHEEL                                         },

    // Do not change this line: end of list
    { wxT( "" ),             KEY_NON_FOUND                                            }
};


// name of modifier keys.
// Note: the Ctrl key is Cmd key on Mac OS X.
// However, in wxWidgets defs, the key WXK_CONTROL is the Cmd key,
// so the code using WXK_CONTROL should be ok on any system.
// (on Mac OS X the actual Ctrl key code is WXK_RAW_CONTROL)
#ifdef __WXMAC__
#define USING_MAC_CMD
#endif

#ifdef USING_MAC_CMD
#define MODIFIER_CTRL       wxT( "Cmd+" )
#else
#define MODIFIER_CTRL       wxT( "Ctrl+" )
#endif
#define MODIFIER_CMD_MAC    wxT( "Cmd+" )
#define MODIFIER_CTRL_BASE  wxT( "Ctrl+" )
#define MODIFIER_ALT        wxT( "Alt+" )
#define MODIFIER_SHIFT      wxT( "Shift+" )


/**
 * Return the key name from the key code.
 *
 * Only some wxWidgets key values are handled for function key ( see hotkeyNameList[] )
 *
 * @param aKeycode key code (ASCII value, or wxWidgets value for function keys).
 * @param aIsFound a pointer to a bool to return true if found, or false. an be nullptr default).
 * @return the key name in a wxString.
 */
wxString KeyNameFromKeyCode( int aKeycode, bool* aIsFound )
{
    wxString keyname, modifier, fullkeyname;
    int      ii;
    bool     found = false;

    // Assume keycode of 0 is "unassigned"
    if( (aKeycode & MD_CTRL) != 0 )
        modifier << MODIFIER_CTRL;

    if( (aKeycode & MD_ALT) != 0 )
        modifier << MODIFIER_ALT;

    if( (aKeycode & MD_SHIFT) != 0 )
        modifier << MODIFIER_SHIFT;

    aKeycode &= ~( MD_CTRL | MD_ALT | MD_SHIFT );

    if( (aKeycode > ' ') && (aKeycode < 0x7F ) )
    {
        found   = true;
        keyname.Append( (wxChar)aKeycode );
    }
    else
    {
        for( ii = 0; ; ii++ )
        {
            if( hotkeyNameList[ii].m_KeyCode == KEY_NON_FOUND ) // End of list
            {
                keyname = wxT( "<unknown>" );
                break;
            }

            if( hotkeyNameList[ii].m_KeyCode == aKeycode )
            {
                keyname = hotkeyNameList[ii].m_Name;
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
 * @param aText the base text on which to append the hotkey.
 * @param aHotKey the hotkey keycode.
 * @param aStyle IS_HOTKEY to add <tab><keyname> (shortcuts in menus, same as hotkeys).
 *               IS_COMMENT to add <spaces><(keyname)> mainly in tool tips.
 */
wxString AddHotkeyName( const wxString& aText, int aHotKey, HOTKEY_ACTION_TYPE aStyle )
{
    wxString msg = aText;
    wxString keyname = KeyNameFromKeyCode( aHotKey );

    if( !keyname.IsEmpty() )
    {
        switch( aStyle )
        {
        case IS_HOTKEY:
        {
            // Don't add a suffix for unassigned hotkeys:
            // WX spews debug from wxAcceleratorEntry::ParseAccel if it doesn't
            // recognize the keyname, which is the case for <unassigned>.
            if( aHotKey != 0 )
            {
                msg << wxT( "\t" ) << keyname;
            }
            break;
        }
        case IS_COMMENT:
        {
            msg << wxT( " (" ) << keyname << wxT( ")" );
            break;
        }
        }
    }

#ifdef USING_MAC_CMD
    // On OSX, the modifier equivalent to the Ctrl key of PCs
    // is the Cmd key, but in code we should use Ctrl as prefix in menus
    msg.Replace( MODIFIER_CMD_MAC, MODIFIER_CTRL_BASE );
#endif

    return msg;
}


/**
 * Return the key code from its user-friendly key name (ie: "Ctrl+M").
 */
int KeyCodeFromKeyName( const wxString& keyname )
{
    int ii, keycode = KEY_NON_FOUND;

    // Search for modifiers: Ctrl+ Alt+ and Shift+
    // Note: on Mac OSX, the Cmd key is equiv here to Ctrl
    wxString key = keyname;
    wxString prefix;
    int modifier = 0;

    while( true )
    {
        prefix.Empty();

        if( key.StartsWith( MODIFIER_CTRL_BASE ) )
        {
            modifier |= MD_CTRL;
            prefix = MODIFIER_CTRL_BASE;
        }
        else if( key.StartsWith( MODIFIER_CMD_MAC ) )
        {
            modifier |= MD_CTRL;
            prefix = MODIFIER_CMD_MAC;
        }
        else if( key.StartsWith( MODIFIER_ALT ) )
        {
            modifier |= MD_ALT;
            prefix = MODIFIER_ALT;
        }
        else if( key.StartsWith( MODIFIER_SHIFT ) )
        {
            modifier |= MD_SHIFT;
            prefix = MODIFIER_SHIFT;
        }
        else
        {
            break;
        }

        if( !prefix.IsEmpty() )
            key.Remove( 0, prefix.Len() );
    }

    if( (key.length() == 1) && (key[0] > ' ') && (key[0] < 0x7F) )
    {
        keycode = key[0];
        keycode += modifier;
        return keycode;
    }

    for( ii = 0; hotkeyNameList[ii].m_KeyCode != KEY_NON_FOUND; ii++ )
    {
        if( key.CmpNoCase( hotkeyNameList[ii].m_Name ) == 0 )
        {
            keycode = hotkeyNameList[ii].m_KeyCode + modifier;
            break;
        }
    }

    return keycode;
}


/*
 * Displays the hotkeys registered with the given tool manager.
 */
void DisplayHotkeyList( EDA_BASE_FRAME* aParent, TOOL_MANAGER* aToolManager )
{
    DIALOG_LIST_HOTKEYS dlg( aParent, aToolManager );
    dlg.ShowModal();
}


void ReadHotKeyConfig( const wxString& aFileName, std::map<std::string, int>& aHotKeys )
{
    wxString fileName = aFileName;

    if( fileName.IsEmpty() )
    {
        wxFileName fn( "user" );
        fn.SetExt( HotkeyFileExtension );
        fn.SetPath( SETTINGS_MANAGER::GetUserSettingsPath() );
        fileName = fn.GetFullPath();
    }

    if( !wxFile::Exists( fileName ) )
        return;

    wxFFile file( fileName, "rb" );

    if( !file.IsOpened() )       // There is a problem to open file
        return;

    wxString input;
    file.ReadAll( &input );
    input.Replace( "\r\n", "\n" );  // Convert Windows files to Unix line-ends
    wxStringTokenizer fileTokenizer( input, "\n", wxTOKEN_STRTOK );

    while( fileTokenizer.HasMoreTokens() )
    {
        wxStringTokenizer lineTokenizer( fileTokenizer.GetNextToken(), "\t" );

        wxString cmdName = lineTokenizer.GetNextToken();
        wxString keyName = lineTokenizer.GetNextToken();

        if( !cmdName.IsEmpty() )
            aHotKeys[ cmdName.ToStdString() ] = KeyCodeFromKeyName( keyName );
    }
}


int WriteHotKeyConfig( const std::map<std::string, TOOL_ACTION*>& aActionMap )
{
    std::map<std::string, int> hotkeys;
    wxFileName fn( "user" );

    fn.SetExt( HotkeyFileExtension );
    fn.SetPath( SETTINGS_MANAGER::GetUserSettingsPath() );

    // Read the existing config (all hotkeys)
    ReadHotKeyConfig( fn.GetFullPath(), hotkeys );

    // Overlay the current app's hotkey definitions onto the map
    for( const auto& ii : aActionMap )
        hotkeys[ ii.first ] = ii.second->GetHotKey();

    // Write entire hotkey set
    wxFFileOutputStream outStream( fn.GetFullPath() );
    wxTextOutputStream  txtStream( outStream, wxEOL_UNIX );

    for( const auto& ii : hotkeys )
        txtStream << wxString::Format( "%s\t%s", ii.first,
                                       KeyNameFromKeyCode( ii.second ) ) << endl;

    txtStream.Flush();
    outStream.Close();

    return 1;
}


int ReadLegacyHotkeyConfig( const wxString& aAppname, std::map<std::string, int>& aMap )
{
    // For Eeschema and Pcbnew frames, we read the new combined file.
    // For other kifaces, we read the frame-based file
    if( aAppname == LIB_EDIT_FRAME_NAME || aAppname == SCH_EDIT_FRAME_NAME )
    {
        return ReadLegacyHotkeyConfigFile( EESCHEMA_HOTKEY_NAME, aMap );
    }
    else if( aAppname == PCB_EDIT_FRAME_NAME || aAppname == FOOTPRINT_EDIT_FRAME_NAME )
    {
        return ReadLegacyHotkeyConfigFile( PCBNEW_HOTKEY_NAME, aMap );
    }

    return ReadLegacyHotkeyConfigFile( aAppname, aMap );
}


int ReadLegacyHotkeyConfigFile( const wxString& aFilename, std::map<std::string, int>& aMap )
{
    wxFileName fn( aFilename );

    fn.SetExt( HotkeyFileExtension );
    fn.SetPath( SETTINGS_MANAGER::GetUserSettingsPath() );

    if( !wxFile::Exists( fn.GetFullPath() ) )
        return 0;

    wxFFile cfgfile( fn.GetFullPath(), "rb" );

    if( !cfgfile.IsOpened() )       // There is a problem to open file
        return 0;

    // get length
    wxFileOffset size = cfgfile.Length();

    // read data
    std::vector<char> buffer( size );
    cfgfile.Read( buffer.data(), size );
    wxString data( buffer.data(), wxConvUTF8, size );

    // Is this the wxConfig format? If so, remove "Keys=" and parse the newlines.
    if( data.StartsWith( wxT("Keys="), &data ) )
        data.Replace( "\\n", "\n", true );

    // parse
    wxStringTokenizer tokenizer( data, L"\r\n", wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
    {
        wxString          line = tokenizer.GetNextToken();
        wxStringTokenizer lineTokenizer( line );

        wxString          line_type = lineTokenizer.GetNextToken();

        if( line_type[0]  == '#' ) // comment
            continue;

        if( line_type[0]  == '[' ) // tags ignored reading legacy hotkeys
            continue;

        if( line_type == wxT( "$Endlist" ) )
            break;

        if( line_type != wxT( "shortcut" ) )
            continue;

        // Get the key name
        lineTokenizer.SetString( lineTokenizer.GetString(), L"\"\r\n\t ", wxTOKEN_STRTOK );
        wxString keyname = lineTokenizer.GetNextToken();

        wxString remainder = lineTokenizer.GetString();

        // Get the command name
        wxString fctname = remainder.AfterFirst( '\"' ).BeforeFirst( '\"' );

        // Add the pair to the map
        aMap[ fctname.ToStdString() ] = KeyCodeFromKeyName( keyname );
    }

    // cleanup
    cfgfile.Close();
    return 1;
}


