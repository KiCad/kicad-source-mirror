/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Kicad Developers, see AUTHORS.txt for contributors.
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

HOTKEY_STORE::HOTKEY_STORE( EDA_HOTKEY_CONFIG* aHotkeys )
{
    for( EDA_HOTKEY_CONFIG* section = aHotkeys; section->m_HK_InfoList; ++section )
    {
        m_hk_sections.push_back( genSection( *section ) );
    }
}


HOTKEY_SECTION HOTKEY_STORE::genSection( EDA_HOTKEY_CONFIG& aSection )
{
    HOTKEY_SECTION generated_section { {}, {}, aSection };

    generated_section.m_name = wxGetTranslation( *aSection.m_Title );

    for( EDA_HOTKEY** info_ptr = aSection.m_HK_InfoList; *info_ptr; ++info_ptr )
    {
        generated_section.m_hotkeys.push_back( { **info_ptr, *aSection.m_SectionTag } );
    }

    return generated_section;
}


std::vector<HOTKEY_SECTION>& HOTKEY_STORE::GetSections()
{
    return m_hk_sections;
}


CHANGED_HOTKEY* HOTKEY_STORE::FindHotkey( const wxString& aTag, int aCmdId )
{
    CHANGED_HOTKEY* found_key = nullptr;

    for( auto& section: m_hk_sections )
    {
        if( *section.m_section.m_SectionTag != aTag)
            continue;

        for( auto& hotkey: section.m_hotkeys )
        {
            auto& curr_hk = hotkey.GetCurrentValue();
            if( curr_hk.m_Idcommand == aCmdId )
            {
                found_key = &hotkey;
                break;
            }
        }
    }

    return found_key;
}


void HOTKEY_STORE::SaveAllHotkeys()
{
    for( auto& section: m_hk_sections )
    {
        for( auto& hotkey: section.m_hotkeys )
        {
            hotkey.SaveHotkey();
        }
    }
}


void HOTKEY_STORE::ResetAllHotkeysToDefault()
{
    for( auto& section: m_hk_sections )
    {
        for( auto& hotkey: section.m_hotkeys )
        {
            hotkey.GetCurrentValue().ResetKeyCodeToDefault();
        }
    }
}


void HOTKEY_STORE::ResetAllHotkeysToOriginal()
{
    for( auto& section: m_hk_sections )
    {
        for( auto& hotkey: section.m_hotkeys )
        {
            hotkey.GetCurrentValue().m_KeyCode = hotkey.GetOriginalValue().m_KeyCode;
        }
    }
}


bool HOTKEY_STORE::CheckKeyConflicts( long aKey, const wxString& aSectionTag,
        EDA_HOTKEY** aConfKey, EDA_HOTKEY_CONFIG** aConfSect )
{
    EDA_HOTKEY* conflicting_key = nullptr;
    EDA_HOTKEY_CONFIG* conflicting_section = nullptr;

    for( auto& section: m_hk_sections )
    {
        const auto& sectionTag = *section.m_section.m_SectionTag;

        if( aSectionTag != g_CommonSectionTag
            && sectionTag != g_CommonSectionTag
            && sectionTag != aSectionTag )
        {
            // This key and its conflict candidate are in orthogonal sections, so skip.
            continue;
        }

        // See if any *current* hotkeys are in conflict
        for( auto& hotkey: section.m_hotkeys )
        {
            auto& curr_hk = hotkey.GetCurrentValue();
            if( aKey == curr_hk.m_KeyCode )
            {
                conflicting_key = &curr_hk;
                conflicting_section = &section.m_section;
            }
        }
    }

    // Write the outparams
    if( aConfKey )
        *aConfKey = conflicting_key;

    if( aConfSect )
        *aConfSect = conflicting_section;

    return conflicting_key == nullptr;
}