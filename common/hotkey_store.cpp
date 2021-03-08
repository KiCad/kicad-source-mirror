/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <hotkey_store.h>
#include <eda_base_frame.h>
#include <tool/tool_manager.h>
#include <tool/action_manager.h>
#include <tool/tool_event.h>
#include <tool/tool_action.h>


class GESTURE_PSEUDO_ACTION : public TOOL_ACTION
{
public:
    GESTURE_PSEUDO_ACTION( const wxString& aLabel, int aHotKey )
    {
        m_label = aLabel;
        m_hotKey = aHotKey;
    }
};

static GESTURE_PSEUDO_ACTION* g_gesturePseudoActions[] = {
    new GESTURE_PSEUDO_ACTION( _( "Pan Left/Right" ), MD_CTRL + PSEUDO_WXK_WHEEL ),
    new GESTURE_PSEUDO_ACTION( _( "Pan Up/Down" ), MD_SHIFT + PSEUDO_WXK_WHEEL ),
    new GESTURE_PSEUDO_ACTION( _( "Finish Drawing" ), PSEUDO_WXK_DBLCLICK ),
#ifdef __WXOSX_MAC__
    new GESTURE_PSEUDO_ACTION( _( "Show Clarify Selection Menu" ), MD_ALT + PSEUDO_WXK_CLICK ),
    new GESTURE_PSEUDO_ACTION( _( "Add to Selection" ), MD_SHIFT + PSEUDO_WXK_CLICK ),
    new GESTURE_PSEUDO_ACTION( _( "Toggle Selection State" ), MD_CTRL + PSEUDO_WXK_CLICK ),
    new GESTURE_PSEUDO_ACTION( _( "Remove from Selection" ), MD_SHIFT + MD_CTRL + PSEUDO_WXK_CLICK ),
#else
    new GESTURE_PSEUDO_ACTION( _( "Show Clarify Selection Menu" ), MD_CTRL + PSEUDO_WXK_CLICK ),
    new GESTURE_PSEUDO_ACTION( _( "Add to Selection" ), MD_SHIFT + PSEUDO_WXK_CLICK ),
    new GESTURE_PSEUDO_ACTION( _( "Toggle Selection State" ), MD_ALT + PSEUDO_WXK_CLICK ),
    new GESTURE_PSEUDO_ACTION( _( "Remove from Selection" ), MD_SHIFT + MD_ALT + PSEUDO_WXK_CLICK ),
#endif
    new GESTURE_PSEUDO_ACTION( _( "Ignore Grid Snaps" ), MD_ALT ),
    new GESTURE_PSEUDO_ACTION( _( "Ignore Other Snaps" ), MD_SHIFT ),
};


wxString HOTKEY_STORE::GetAppName( TOOL_ACTION* aAction )
{
    wxString name( aAction->GetName() );
    return name.BeforeFirst( '.' );
}


wxString HOTKEY_STORE::GetSectionName( TOOL_ACTION* aAction )
{
    std::map<wxString, wxString> s_AppNames = {
            { wxT( "common" ),   _( "Common" ) },
            { wxT( "kicad" ),    _( "Project Manager" ) },
            { wxT( "eeschema" ), _( "Schematic Editor" ) },
            { wxT( "pcbnew" ),   _( "PCB Editor" ) },
            { wxT( "plEditor" ), _( "Drawing Sheet Editor" ), },
            { wxT( "3DViewer" ), _( "3D Viewer" ) }
    };

    wxString appName = GetAppName( aAction );

    if( s_AppNames.count( appName ) )
        return s_AppNames[ appName ];
    else
        return appName;
}


HOTKEY_STORE::HOTKEY_STORE()
{
}


void HOTKEY_STORE::Init( std::vector<TOOL_MANAGER*> aToolManagerList, bool aIncludeGestures )
{
    m_toolManagers = std::move( aToolManagerList );

    // Collect all action maps into a single master map.  This will re-group everything
    // and collect duplicates together
    std::map<std::string, HOTKEY> masterMap;

    for( TOOL_MANAGER* toolMgr : m_toolManagers )
    {
        for( const auto& entry : toolMgr->GetActions() )
        {
            // Internal actions probably shouldn't be allowed hotkeys
            if( entry.second->GetLabel().IsEmpty() )
                continue;

            HOTKEY& hotkey = masterMap[ entry.first ];
            hotkey.m_Actions.push_back( entry.second );
            hotkey.m_EditKeycode = entry.second->GetHotKey();
        }
    }

    wxString        currentApp;
    HOTKEY_SECTION* currentSection = nullptr;

    // If a previous list was built, ensure this previous list is cleared:
    m_hk_sections.clear();

    for( const auto& entry : masterMap )
    {
        TOOL_ACTION* entryAction = entry.second.m_Actions[ 0 ];
        wxString     entryApp = GetAppName( entryAction );

        if( !currentSection || entryApp != currentApp )
        {
            m_hk_sections.emplace_back( HOTKEY_SECTION() );
            currentApp = entryApp;
            currentSection = &m_hk_sections.back();
            currentSection->m_SectionName = GetSectionName( entryAction );
        }

        currentSection->m_HotKeys.emplace_back( HOTKEY( entry.second ) );
    }

    if( aIncludeGestures )
    {
        m_hk_sections.emplace_back( HOTKEY_SECTION() );
        currentSection = &m_hk_sections.back();
        currentSection->m_SectionName = _( "Gestures" );

        for( TOOL_ACTION* gesture : g_gesturePseudoActions )
            currentSection->m_HotKeys.emplace_back( HOTKEY( gesture ) );
    }
}


std::vector<HOTKEY_SECTION>& HOTKEY_STORE::GetSections()
{
    return m_hk_sections;
}


void HOTKEY_STORE::SaveAllHotkeys()
{
    for( HOTKEY_SECTION& section : m_hk_sections )
    {
        for( HOTKEY& hotkey : section.m_HotKeys )
        {
            for( TOOL_ACTION* action : hotkey.m_Actions )
                action->SetHotKey( hotkey.m_EditKeycode );
        }
    }
}


void HOTKEY_STORE::ResetAllHotkeysToDefault()
{
    for( HOTKEY_SECTION& section : m_hk_sections )
    {
        for( HOTKEY& hotkey : section.m_HotKeys )
            hotkey.m_EditKeycode = hotkey.m_Actions[ 0 ]->GetDefaultHotKey();
    }
}


void HOTKEY_STORE::ResetAllHotkeysToOriginal()
{
    for( HOTKEY_SECTION& section : m_hk_sections )
    {
        for( HOTKEY& hotkey : section.m_HotKeys )
            hotkey.m_EditKeycode = hotkey.m_Actions[ 0 ]->GetHotKey();
    }
}


bool HOTKEY_STORE::CheckKeyConflicts( TOOL_ACTION* aAction, long aKey, HOTKEY** aConflict )
{
    wxString sectionName = GetSectionName( aAction );

    // Create a fake "TOOL_ACTION" so we can get the section name for "Common" through the API.
    // Simply declaring a wxString with the value "Common" works, but the goal is to futureproof
    // the code here as much as possible.
    TOOL_ACTION commonAction( "common.Control.Fake", AS_GLOBAL, 0, "", "", "" );
    wxString    commonName = GetSectionName( &commonAction );

    for( HOTKEY_SECTION& section : m_hk_sections )
    {
        // We can have the same hotkey in multiple sections (i.e. Kicad programs), but if a hotkey
        // is in "Common" it can't be in any other section and vice versa.

        if( !( section.m_SectionName == sectionName || section.m_SectionName == commonName ) )
            continue;

        for( HOTKEY& hotkey : section.m_HotKeys )
        {
            if( hotkey.m_Actions[0] == aAction )
                continue;

            if( hotkey.m_EditKeycode == aKey )
            {
                *aConflict = &hotkey;
                return true;
            }
        }
    }

    return false;
}
