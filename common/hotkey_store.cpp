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
#include <tool/tool_manager.h>
#include <tool/tool_action.h>


wxString HOTKEY_STORE::GetAppName( TOOL_ACTION* aAction )
{
    wxString name( aAction->GetName() );
    return name.BeforeFirst( '.' );
}


wxString HOTKEY_STORE::GetSectionName( TOOL_ACTION* aAction )
{
    std::map<wxString, wxString> s_AppNames = {
            { wxT( "common" ),   _( "Common" ) },
            { wxT( "kicad" ),    _( "Kicad Manager" ) },
            { wxT( "eeschema" ), _( "Eeschema" ) },
            { wxT( "pcbnew" ),   _( "Pcbnew" ) },
            { wxT( "plEditor" ), _( "Page Layout Editor" ), },
            { wxT( "3DViewer" ), _( "3D Viewer" ) }
    };

    wxString appName = GetAppName( aAction );
    
    if( s_AppNames.count( appName ) )
        return s_AppNames[ appName ];
    else
        return wxT( "XXX" + appName );
}


HOTKEY_STORE::HOTKEY_STORE()
{
}


void HOTKEY_STORE::Init( std::vector<TOOL_MANAGER*> aToolManagerList )
{
    m_toolManagers = std::move( aToolManagerList );
    
    // Collect all action maps into a single master map.  This will re-group everything
    // and elimate duplicates
    std::map<std::string, TOOL_ACTION*> masterMap;
    
    for( TOOL_MANAGER* toolMgr : m_toolManagers )
    {
        for( const auto& entry : toolMgr->GetActions() )
        {
            // Internal actions probably shouldn't be allowed hotkeys
            if( entry.second->GetLabel().IsEmpty() )
                continue;
            
            masterMap[ entry.first ] = entry.second;
        }
    }
    
    wxString        currentApp;
    HOTKEY_SECTION* currentSection = nullptr;
    HOTKEY*         currentHotKey = nullptr;

    for( const auto& entry : masterMap )
    {
        wxString thisApp = GetAppName( entry.second );
        
        if( thisApp != currentApp )
        {
            m_hk_sections.emplace_back( HOTKEY_SECTION() );
            currentApp = thisApp;
            currentSection = &m_hk_sections.back();
            currentSection->m_SectionName = GetSectionName( entry.second );
        }

        currentSection->m_HotKeys.emplace_back( HOTKEY() );
        currentHotKey = &currentSection->m_HotKeys.back();
        currentHotKey->m_Parent = entry.second;
        currentHotKey->m_EditKeycode = entry.second->GetHotKey();
    }
}


std::vector<HOTKEY_SECTION>& HOTKEY_STORE::GetSections()
{
    return m_hk_sections;
}


void HOTKEY_STORE::SaveAllHotkeys()
{
    for( HOTKEY_SECTION& section: m_hk_sections )
    {
        for( HOTKEY& hotkey: section.m_HotKeys )
            hotkey.m_Parent->SetHotKey( hotkey.m_EditKeycode );
    }
}


void HOTKEY_STORE::ResetAllHotkeysToDefault()
{
    for( HOTKEY_SECTION& section: m_hk_sections )
    {
        for( HOTKEY& hotkey: section.m_HotKeys )
            hotkey.m_EditKeycode = hotkey.m_Parent->GetDefaultHotKey();
    }
}


void HOTKEY_STORE::ResetAllHotkeysToOriginal()
{
    for( HOTKEY_SECTION& section: m_hk_sections )
    {
        for( HOTKEY& hotkey: section.m_HotKeys )
            hotkey.m_EditKeycode = hotkey.m_Parent->GetHotKey();
    }
}


bool HOTKEY_STORE::CheckKeyConflicts( TOOL_ACTION* aAction, long aKey, HOTKEY** aConflict )
{
    wxString sectionName = GetSectionName( aAction );

    for( HOTKEY_SECTION& section: m_hk_sections )
    {
        if( section.m_SectionName != sectionName )
            continue;
        
        for( HOTKEY& hotkey: section.m_HotKeys )
        {
            if( hotkey.m_Parent == aAction )
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