/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "fab_sections.h"

#include <array>

namespace IPC2581
{

namespace
{

constexpr size_t SECTION_COUNT = static_cast<size_t>( SECTION::COUNT );
constexpr size_t MODE_COUNT = static_cast<size_t>( MODE::COUNT );

constexpr SECTION_RULE N = SECTION_RULE::EXCLUDED;
constexpr SECTION_RULE O = SECTION_RULE::RULE_OPTIONAL;
constexpr SECTION_RULE Y = SECTION_RULE::REQUIRED;

// IPC-2581C Table 4 p 80
// The rows follow SECTION and the columns follow MODE
constexpr std::array<std::array<SECTION_RULE, MODE_COUNT>, SECTION_COUNT> TABLE_4 = { {
    //           UserDef  BOM  Stackup  Fab  Assembly  Test  Stencil  DFX
    /* K */    { {  O,     N,     N,     O,     O,      N,      N,     N } },
    /* B */    { {  O,     Y,     O,     O,     Y,      Y,      N,     N } },
    /* C */    { {  O,     N,     N,     N,     Y,      Y,      O,     N } },
    /* A */    { {  O,     N,     N,     N,     Y,      Y,      N,     N } },
    /* S */    { {  O,     N,     Y,     Y,     N,      N,      N,     N } },
    /* U */    { {  O,     N,     O,     Y,     Y,      Y,      Y,     N } },
    /* M */    { {  O,     N,     N,     Y,     N,      N,      O,     N } },
    /* P */    { {  O,     N,     N,     N,     O,      N,      Y,     N } },
    /* L */    { {  O,     N,     N,     Y,     Y,      Y,      O,     N } },
    /* R */    { {  O,     N,     O,     Y,     Y,      Y,      O,     N } },
    /* D */    { {  O,     N,     O,     O,     O,      O,      O,     N } },
    /* O */    { {  O,     N,     Y,     Y,     Y,      Y,      O,     N } },
    /* I */    { {  O,     N,     Y,     Y,     N,      N,      N,     N } },
    /* E */    { {  O,     N,     O,     O,     N,      N,      N,     N } },
    /* F */    { {  O,     N,     O,     O,     N,      N,      N,     N } },
    /* G */    { {  O,     N,     N,     O,     O,      O,      N,     N } },
    /* Y */    { {  O,     N,     N,     Y,     O,      Y,      N,     N } },
    /* X */    { {  O,     O,     O,     O,     O,      O,      O,     Y } },
} };

constexpr std::array<char, SECTION_COUNT> SECTION_KEYS = {
    'K', 'B', 'C', 'A', 'S', 'U', 'M', 'P', 'L', 'R', 'D', 'O', 'I', 'E', 'F', 'G', 'Y', 'X'
};

constexpr std::array<const char*, MODE_COUNT> MODE_TOKENS = {
    "USERDEF", "BOM", "STACKUP", "FABRICATION", "ASSEMBLY", "TEST", "STENCIL", "DFX"
};

// A new function mode in the middle of MODE moves each token after it
static_assert( MODE_TOKENS[static_cast<size_t>( MODE::DFX )][0] == 'D',
               "MODE_TOKENS is out of step with MODE" );

SECTION_SET sectionsWithRule( MODE aMode, SECTION_RULE aRule )
{
    SECTION_SET set;

    for( size_t ii = 0; ii < SECTION_COUNT; ++ii )
    {
        SECTION section = static_cast<SECTION>( ii );

        if( SectionRule( aMode, section ) == aRule )
            set.Set( section );
    }

    return set;
}

} // namespace


char SectionKeyChar( SECTION aSection )
{
    return SECTION_KEYS[static_cast<size_t>( aSection )];
}


std::optional<SECTION> SectionFromKeyChar( char aKey )
{
    for( size_t ii = 0; ii < SECTION_COUNT; ++ii )
    {
        if( SECTION_KEYS[ii] == aKey )
            return static_cast<SECTION>( ii );
    }

    return std::nullopt;
}


SECTION_RULE SectionRule( MODE aMode, SECTION aSection )
{
    return TABLE_4[static_cast<size_t>( aSection )][static_cast<size_t>( aMode )];
}


SECTION_SET RequiredSections( MODE aMode )
{
    return sectionsWithRule( aMode, SECTION_RULE::REQUIRED );
}


SECTION_SET OptionalSections( MODE aMode )
{
    return sectionsWithRule( aMode, SECTION_RULE::RULE_OPTIONAL );
}


SECTION_SET RecommendedOptionalSections( MODE aMode )
{
    switch( aMode )
    {
    // This set keeps an unconfigured export the same as before
    case MODE::USERDEF:
        return LegacySections();

    case MODE::STACKUP:
        return { SECTION::DIELECTRIC };

    // Section 4.1.3.3 gives KSUMLROIEF as its FABRICATION example
    case MODE::FABRICATION:
        return { SECTION::PADSTACKS, SECTION::DIELECTRIC, SECTION::MISC_FAB };

    case MODE::ASSEMBLY:
        return { SECTION::PADSTACKS, SECTION::SOLDERPASTE };

    default:
        return {};
    }
}


SECTION_SET UnsupportedSections()
{
    return { SECTION::DFX };
}


SECTION_SET LegacySections()
{
    SECTION_SET set;
    set.set();

    // We didn't emit these previously, so maintain the same output
    // until/unless this is set by user
    set.Set( SECTION::LOGICAL_NET, false );
    set.Set( SECTION::PHYSICAL_NET, false );

    return set & ~UnsupportedSections();
}


bool NeedsCadData( const SECTION_SET& aSet )
{
    SECTION_SET inCadData = aSet;
    inCadData.Set( SECTION::BOM_AVL, false );

    return inCadData.any();
}


wxString SectionKeyString( const SECTION_SET& aSet )
{
    wxString key;

    for( size_t ii = 0; ii < SECTION_COUNT; ++ii )
    {
        if( aSet.test( ii ) )
            key << SECTION_KEYS[ii];
    }

    return key;
}


bool SectionSetFromKeyString( const wxString& aKey, SECTION_SET& aResult )
{
    SECTION_SET set;

    for( wxUniChar ch : aKey )
    {
        if( !ch.IsAscii() )
            return false;

        std::optional<SECTION> section = SectionFromKeyChar( static_cast<char>( ch ) );

        if( !section )
            return false;

        set.Set( *section );
    }

    aResult = set;
    return true;
}


wxString ModeToken( MODE aMode )
{
    return wxString::FromAscii( MODE_TOKENS[static_cast<size_t>( aMode )] );
}


std::optional<MODE> ModeFromToken( const wxString& aToken )
{
    // wxString::Upper() uses the locale
    // In tr_TR it changes i to a dotted capital and no lowercase name agrees
    wxString token;

    for( wxUniChar ch : aToken )
    {
        if( ch >= 'a' && ch <= 'z' )
            token << static_cast<wxChar>( ch - 'a' + 'A' );
        else
            token << ch;
    }

    for( size_t ii = 0; ii < MODE_COUNT; ++ii )
    {
        if( token == wxString::FromAscii( MODE_TOKENS[ii] ) )
            return static_cast<MODE>( ii );
    }

    return std::nullopt;
}


} // namespace IPC2581
