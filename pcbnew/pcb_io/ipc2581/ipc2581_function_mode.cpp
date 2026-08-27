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

#include "ipc2581_function_mode.h"

#include <array>
#include <utility>

#include <wx/translation.h>

namespace IPC2581
{

namespace
{

constexpr size_t SECTION_COUNT = static_cast<size_t>( SECTION::COUNT );
constexpr size_t MODE_COUNT = static_cast<size_t>( MODE::COUNT );

constexpr SECTION_RULE N = SECTION_RULE::EXCLUDED;
constexpr SECTION_RULE O = SECTION_RULE::OPTIONAL;
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

// Each layerFunction value that KiCad writes with its Table 4 schema section
constexpr std::array<std::pair<const char*, SECTION>, 27> LAYER_FUNCTION_SECTIONS = { {
        // EDGE_CHAMFER is in section 4.1.1.10 and in section 4.1.1.15
        // Table 4 makes R necessary for FABRICATION but F is optional
        { "DRILL",                  SECTION::DRILL_ROUT },
        { "ROUT",                   SECTION::DRILL_ROUT },
        { "V_CUT",                  SECTION::DRILL_ROUT },
        { "EDGE_CHAMFER",           SECTION::DRILL_ROUT },
        { "SOLDERMASK",             SECTION::SOLDERMASK },
        { "SOLDERPASTE",            SECTION::SOLDERPASTE },
        { "SILKSCREEN",             SECTION::SILKSCREEN },
        { "LEGEND",                 SECTION::SILKSCREEN },
        { "DOCUMENT",               SECTION::DOCUMENTATION },
        { "GRAPHIC",                SECTION::DOCUMENTATION },
        { "BOARD_OUTLINE",          SECTION::DOCUMENTATION },
        { "REWORK",                 SECTION::DOCUMENTATION },
        { "FIXTURE",                SECTION::DOCUMENTATION },
        { "PROBE",                  SECTION::DOCUMENTATION },
        { "COURTYARD",              SECTION::DOCUMENTATION },
        { "ASSEMBLY",               SECTION::COMPONENTS },
        { "COATINGCOND",            SECTION::MISC_FAB },
        { "COATINGNONCOND",         SECTION::MISC_FAB },
        { "CONDUCTIVE_ADHESIVE",    SECTION::MISC_FAB },
        { "GLUE",                   SECTION::MISC_FAB },
        { "HOLEFILL",               SECTION::MISC_FAB },
        { "SOLDERBUMP",             SECTION::MISC_FAB },
        { "THIEVING_KEEP_INOUT",    SECTION::MISC_FAB },
        { "EDGE_PLATING",           SECTION::MISC_FAB },
        { "STIFFENER",              SECTION::MISC_FAB },
        { "CAPACITIVE",             SECTION::MISC_FAB },
        { "RESISTIVE",              SECTION::MISC_FAB },
} };


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
    return sectionsWithRule( aMode, SECTION_RULE::OPTIONAL );
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


bool ModeSupported( MODE aMode, REVISION )
{
    // Revision B has no DFX function mode and KiCad writes no Dfx element
    return aMode != MODE::DFX;
}


std::optional<SECTION> SectionForBoardLayer( PCB_LAYER_ID aLayer )
{
    if( IsCopperLayer( aLayer ) )
        return IsExternalCopperLayer( aLayer ) ? SECTION::OUTER_COPPER : SECTION::INNER_COPPER;

    switch( aLayer )
    {
    case F_Adhes:
    case B_Adhes:
        return SECTION::MISC_FAB;

    case F_Paste:
    case B_Paste:
        return SECTION::SOLDERPASTE;

    case F_SilkS:
    case B_SilkS:
        return SECTION::SILKSCREEN;

    case F_Mask:
    case B_Mask:
        return SECTION::SOLDERMASK;

    case Edge_Cuts:
        return SECTION::PROFILE;

    case F_CrtYd:
    case B_CrtYd:
    case Margin:
        return SECTION::DOCUMENTATION;

    // Section 4.1.1.4 puts layerFunction ASSEMBLY in Component Assembly Data
    case F_Fab:
    case B_Fab:
        return SECTION::COMPONENTS;

    default:
        if( IsUserLayer( aLayer ) )
            return SECTION::DOCUMENTATION;

        return std::nullopt;
    }
}


std::optional<SECTION> SectionForLayerFunction( const wxString& aLayerFunction,
                                                const wxString& aSide )
{
    for( const auto& entry : LAYER_FUNCTION_SECTIONS )
    {
        if( aLayerFunction == wxString::FromAscii( entry.first ) )
            return entry.second;
    }

    if( aLayerFunction.StartsWith( wxT( "DIEL" ) ) )
        return SECTION::DIELECTRIC;

    // Revision B writes EMBEDDED_COMPONENT and revision C writes COMPONENT_EMBEDDED 
    // because why not?
    if( aLayerFunction.StartsWith( wxT( "COMPONENT" ) )
        || aLayerFunction == wxT( "EMBEDDED_COMPONENT" ) )
    {
        return SECTION::COMPONENTS;
    }

    // Sections 4.1.1.12 and 4.1.1.13 divide the conductor functions by side
    static const std::array<const char*, 6> conductors = { "CONDUCTOR", "CONDFILM", "CONDFOIL",
                                                           "PLANE", "SIGNAL", "MIXED" };

    for( const char* function : conductors )
    {
        if( aLayerFunction == wxString::FromAscii( function ) )
        {
            if( aSide == wxT( "INTERNAL" ) )
                return SECTION::INNER_COPPER;

            return aSide.IsEmpty() ? std::nullopt
                                   : std::optional<SECTION>( SECTION::OUTER_COPPER );
        }
    }

    return std::nullopt;
}


RESOLVE_RESULT ResolveSections( REVISION aRevision, MODE aMode, const SECTION_SET& aRequested )
{
    RESOLVE_RESULT result;

    if( !ModeSupported( aMode, aRevision ) )
    {
        result.m_conflicts.push_back( { std::nullopt, std::nullopt, _( "Cannot produce this IPC-2581 data set." ) } );
        return result;
    }

    // A requested section stays only if the function mode makes it optional
    result.m_included = ( ( aRequested & OptionalSections( aMode ) ) | RequiredSections( aMode ) )
                        & ~UnsupportedSections();

    if( result.m_included.none() )
    {
        result.m_conflicts.push_back( { std::nullopt, std::nullopt, _( "No IPC-2581 content selected." ) } );
        return result;
    }

    const bool haveBom = result.m_included.Contains( SECTION::BOM_AVL );
    const bool havePackages = result.m_included.Contains( SECTION::PACKAGES );
    const bool haveComponents = result.m_included.Contains( SECTION::COMPONENTS );
    const bool havePadstacks = result.m_included.Contains( SECTION::PADSTACKS );
    const bool haveOuterCopper = result.m_included.Contains( SECTION::OUTER_COPPER );

    // Component@layerRef is mandatory and names the copper layer of the footprint
    // You cannot remove it and it must not dangle
    if( haveComponents && !haveOuterCopper )
    {
        result.m_conflicts.push_back(
                { SECTION::COMPONENTS, SECTION::OUTER_COPPER,
                  _( "Cannot export assembly data without outer copper layers." ) } );
    }

    // Each physical net point names an outer copper layer
    if( result.m_included.Contains( SECTION::PHYSICAL_NET ) && !haveOuterCopper )
    {
        result.m_conflicts.push_back(
                { SECTION::PHYSICAL_NET, SECTION::OUTER_COPPER,
                  _( "Cannot export physical netlist without outer copper layers." ) } );
    }

    if( !havePadstacks )
        result.m_suppressions.Set( SUPPRESS::PAD_PADSTACKDEFREF );

    // No keyref selects RefDes@packageRef in the two revisions
    // Remove it to prevent a dangling name
    if( haveBom && !havePackages )
        result.m_suppressions.Set( SUPPRESS::BOM_REFDES_PACKAGEREF );

    // RefDes@layerRef names an outer copper layer and revision C puts it in layerKeyRef
    if( haveBom && !haveOuterCopper )
        result.m_suppressions.Set( SUPPRESS::BOM_REFDES_LAYERREF );

    if( aRevision == REVISION::C )
    {
        if( !haveBom )
        {
            result.m_suppressions.Set( SUPPRESS::COMPONENT_REFDES );
            result.m_suppressions.Set( SUPPRESS::PINREF_COMPONENTREF );
        }

        if( !havePackages )
            result.m_suppressions.Set( SUPPRESS::COMPONENT_PACKAGEREF );
    }
    else
    {
        // Can't remove Component@packageRef in RevB
        if( haveComponents && !havePackages )
        {
            result.m_conflicts.push_back(
                    { SECTION::COMPONENTS, SECTION::PACKAGES,
                      _( "Component assembly data cannot be exported without component "
                         "packages in IPC-2581B." ) } );
        }

        // We need component for RefDes and we store this in PinRef
        if( !haveComponents )
        {
            result.m_suppressions.Set( SUPPRESS::PINREF_COMPONENTREF );

            if( haveBom )
                result.m_suppressions.Set( SUPPRESS::BOM_REFDES );
        }
    }

    if( !NeedsCadData( result.m_included ) )
        result.m_suppressions.Set( SUPPRESS::BOM_HEADER_STEPREF );

    return result;
}

} // namespace IPC2581
