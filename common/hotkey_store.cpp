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
    m_isValid = false;
    m_invalidityCauses = _( "Hotkeys not checked" );

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


bool HOTKEY_STORE::CheckKeyConflicts( long aKey, const wxString& aSectionTag, EDA_HOTKEY** aConfKey,
        EDA_HOTKEY_CONFIG** aConfSect, const int aIdCommand )
{
    EDA_HOTKEY* conflicting_key = nullptr;
    EDA_HOTKEY_CONFIG* conflicting_section = nullptr;

    for( auto& section : m_hk_sections )
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
            if( ( aKey == curr_hk.m_KeyCode ) && ( aIdCommand != curr_hk.m_Idcommand ) )
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


bool HOTKEY_STORE::CheckKeyValidity( long aKey, wxString& aMessage )
{
    // Extract the modifiers and the keycode
    int modifiers = aKey & ( GR_KB_SHIFT | GR_KB_CTRL | GR_KB_ALT );
    int keycode = aKey & 0x00FFFFFF;

    // Hotkeys may not contain the shift+SYMBOL sequence.
    // This sequence gets mapped to (UPPER SYMBOL) in the hotkey logic
    if( modifiers & GR_KB_SHIFT )
    {
        // These catch the ASCII codes for the special characters
        if( ( keycode <= 64 && keycode >= 32 ) ||
            ( keycode <= 96 && keycode >= 91 ) ||
            ( keycode <= 126 && keycode >= 123 )  )
        {
            aMessage = _( "A hotkey cannot contain the shift key and a symbol key." );
            return false;
        }
    }

// This code block can be used to test the key validity checks
#if 0
    if( modifiers & GR_KB_CTRL )
    {
        if( keycode == 'K' )
        {
            aMessage = "A hotkey cannot be ctrl+K";
            return false;
        }
    }
#endif

    return true;
}


bool HOTKEY_STORE::TestStoreValidity()
{
    m_isValid = true;
    m_invalidityCauses.Clear();

    // Iterate over every key to test it
    for( HOTKEY_SECTION& section : m_hk_sections )
    {
        for( CHANGED_HOTKEY& hotkey : section.m_hotkeys )
        {
            EDA_HOTKEY& curr_hk = hotkey.GetCurrentValue();
            wxString    validMessage;

            bool isValid = CheckKeyValidity( curr_hk.m_KeyCode, validMessage );

            // If the key isn't valid, set it and continue
            if( !isValid )
            {
                hotkey.SetValidity( false, validMessage );

                m_invalidityCauses << wxGetTranslation( curr_hk.m_InfoMsg );
                m_invalidityCauses << ": " << validMessage << "\n";
                m_isValid = false;
                continue;
            }

            // Test for duplication
            const wxString&    sectionTag = *section.m_section.m_SectionTag;
            EDA_HOTKEY*        conflicting_key = nullptr;
            EDA_HOTKEY_CONFIG* conflicting_section = nullptr;

            CheckKeyConflicts( curr_hk.m_KeyCode, sectionTag, &conflicting_key,
                    &conflicting_section, curr_hk.m_Idcommand );

            // Not valid if a conflicting key was found
            if( conflicting_key != nullptr )
            {
                wxString keyInfoMsg = wxGetTranslation( conflicting_key->m_InfoMsg );
                validMessage =
                        wxString::Format( _( "Duplicate of hotkey for \"%s\"" ), keyInfoMsg );

                hotkey.SetValidity( false, validMessage );

                m_invalidityCauses << wxGetTranslation( curr_hk.m_InfoMsg );
                m_invalidityCauses << ": " << validMessage << "\n";
                m_isValid = false;
                continue;
            }

            // If it made it this far, it is a valid hotkey
            validMessage = _( "Hotkey is valid" );
            hotkey.SetValidity( true, validMessage );
        }
    }

    return m_isValid;
}
