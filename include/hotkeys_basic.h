/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  HOTKEYS_BASIC_H
#define  HOTKEYS_BASIC_H

#include <map>
#include <common.h>

#define DEFAULT_HOTKEY_FILENAME_EXT wxT( "hotkeys" )
#define EESCHEMA_HOTKEY_NAME wxT( "Eeschema" )
#define PCBNEW_HOTKEY_NAME wxT( "PcbNew" )

// A define to allow translation of Hot Key message Info in hotkey help menu
// We do not want to use the _( x ) usual macro from wxWidgets, which calls wxGetTranslation(),
// because the English string is used in key file configuration
// The translated string is used only when displaying the help window.
// Therefore translation tools have to use the "_" and the "_HKI" prefix to extract
// strings to translate
#include <i18n_utility.h>       // _HKI definition

class TOOL_ACTION;
class TOOL_MANAGER;
class EDA_BASE_FRAME;


/* Identifiers (tags) in key code configuration file (or section names)
 *  .m_SectionTag member of a EDA_HOTKEY_CONFIG
 */
extern wxString g_CommonSectionTag;

/**
 * class EDA_HOTKEY
 * is a class to handle hot key commands.  Hot keys have a default value.
 * This class allows the real key code changed by user(from a key code list file)
 */
class EDA_HOTKEY
{
public:
    int      m_KeyCode;      // Key code (ascii value for ascii keys or wxWidgets code for function key
    wxString m_InfoMsg;      // info message.
    int      m_Idcommand;    // internal id for the corresponding command (see hotkey_id_command list)
    int      m_IdMenuEvent;  // id to call the corresponding event (if any) (see id.h)

public:
    EDA_HOTKEY( const wxChar* infomsg, int idcommand, int keycode, int idmenuevent = 0 );
};


/**
 * Structure EDA_HOTKEY_CONFIG
 * contains the information required to save hot key information to a configuration file.
 * a Section name and the corresponding list of hotkeys (EDA_HOTKEY list)
 * hotkeys are grouped by section.
 * a section is a list of hotkey infos ( a EDA_HOTKEY list).
 * A full list of hotkeys can used one or many sections
 * for instance:
 *    the schematic editor uses a common section (zoom hotkeys list ..) and a specific section
 *    the library editor uses the same common section and a specific section
 * this feature avoid duplications and made hotkey file config easier to understand and edit
 */
struct EDA_HOTKEY_CONFIG
{
public:
    wxString*     m_SectionTag;   // The configuration file section name.
    EDA_HOTKEY**  m_HK_InfoList;  // List of EDA_HOTKEY pointers
    wxString*     m_Title;        // Title displayed in hotkey editor and used as comment in file
};


/**
 * Function KeyNameFromKeyCode
 * return the key name from the key code
 * * Only some wxWidgets key values are handled for function key ( see
 * s_Hotkey_Name_List[] )
 * @param aKeycode = key code (ascii value, or wxWidgets value for function keys)
 * @param aIsFound = a pointer to a bool to return true if found, or false. an be NULL default)
 * @return the key name in a wxString
 */
wxString KeyNameFromKeyCode( int aKeycode, bool * aIsFound = nullptr );

/**
 * Function KeyNameFromCommandId
 * return the key name from the Command id value ( m_Idcommand member value)
 * @param aList = pointer to a EDA_HOTKEY list of commands
 * @param aCommandId = Command Id value
 * @return the key name in a wxString
 */
wxString KeyNameFromCommandId( EDA_HOTKEY** aList, int aCommandId );

/**
 * An helper enum for AddHotkeyName function
 * In menus we can add a hot key, or an accelerator , or sometimes just a comment
 * Hot keys can perform actions using the current mouse cursor position
 * Accelerators perform the same action as the associated menu
 * A comment is used in tool tips for some tools (zoom ..)
 *    to show the hot key that performs this action
 */
enum HOTKEY_ACTION_TYPE
{
    IS_HOTKEY,
    IS_COMMENT
};

/**
 * AddHotkeyName
 * @param aText - the base text on which to append the hotkey
 * @param aHotKey - the hotkey keycode
 * @param aStyle - IS_HOTKEY to add <tab><keyname> (shortcuts in menus, same as hotkeys)
 *                 IS_COMMENT to add <spaces><(keyname)> mainly in tool tips
 */
wxString AddHotkeyName(  const wxString& aText, int aHotKey, 
                         HOTKEY_ACTION_TYPE aStyle = IS_HOTKEY);

/**
 * Function AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 * @param aText = a wxString. returns aText + key name
 * @param aDescrList = pointer to a EDA_HOTKEY_CONFIG DescrList of commands
 * @param aCommandId = Command Id value
 * @param aShortCutType The #HOTKEY_ACTION_TYPE of the shortcut.
 * @return a wxString (aTest + key name) if key found or aText without modification
 */
wxString AddHotkeyName( const wxString&           aText,
                        struct EDA_HOTKEY_CONFIG* aDescrList,
                        int                       aCommandId,
                        HOTKEY_ACTION_TYPE        aShortCutType = IS_HOTKEY );

/**
 * Function DisplayHotkeyList
 * Displays the current hotkey list
 * @param aFrame = current active frame
 * @param aToolMgr = the tool manager holding the registered actions from which the hotkeys
 *                   will be harvested
 */
void DisplayHotkeyList( EDA_BASE_FRAME* aFrame, TOOL_MANAGER* aToolMgr );

/**
 * Function WriteHotKeyConfig
 * Updates the hotkeys config file with the hotkeys from the given actions map.
 */
int WriteHotKeyConfig( std::map<std::string, TOOL_ACTION*> aActionMap );

/**
 * Function ReadLegacyHotkeyConfigFile
 * Read hotkey configuration for a given app,
 * possibly before the frame for that app has been created
 * @param aFilename = the filename to save the hotkeys as
 * @param aMap The list of keycodes mapped by legacy property names 
 * @return 1 on success, 0 on failure
*/
int ReadLegacyHotkeyConfigFile( const wxString& aFilename, std::map<std::string, int>& aMap );

/**
 * Function ReadLegacyHotkeyConfig
 * Read configuration data and fill the current hotkey list with hotkeys
 * @param aAppname = the value of the app's m_FrameName
 * @param aMap The list of keycodes mapped by legacy property names 
 */
int ReadLegacyHotkeyConfig( const wxString& aAppname, std::map<std::string, int>& aMap );

#endif // HOTKEYS_BASIC_H
