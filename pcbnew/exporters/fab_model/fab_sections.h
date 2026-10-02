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

#ifndef FAB_SECTIONS_H
#define FAB_SECTIONS_H

#include <bitset>
#include <initializer_list>
#include <optional>
#include <wx/string.h>

#include <jobs/job_export_pcb_fab.h>

namespace FAB
{

/**
 * Schema sections of the IPC-2581C function mode table, Table 4 p 80
 *
 * The order follows the table rows so that #SectionKeyString gives the @c sectionKey order
 */
enum class SECTION
{
    PADSTACKS,          ///< K
    BOM_AVL,            ///< B
    PACKAGES,           ///< C
    COMPONENTS,         ///< A
    STACKUP,            ///< S
    PROFILE,            ///< U
    SOLDERMASK,         ///< M
    SOLDERPASTE,        ///< P
    SILKSCREEN,         ///< L
    DRILL_ROUT,         ///< R
    DOCUMENTATION,      ///< D
    OUTER_COPPER,       ///< O
    INNER_COPPER,       ///< I
    DIELECTRIC,         ///< E
    MISC_FAB,           ///< F
    LOGICAL_NET,        ///< G
    PHYSICAL_NET,       ///< Y
    DFX,                ///< X

    COUNT
};

/// Columns of Table 4  The job owns the enum so that every layer shares one type
using MODE = JOB_EXPORT_PCB_FAB::DATA_SET;

/// Table 4 cell values N O and Y
enum class SECTION_RULE
{
    EXCLUDED,
    RULE_OPTIONAL,
    REQUIRED
};

/// Set of schema sections
class SECTION_SET : public std::bitset<static_cast<size_t>( SECTION::COUNT )>
{
public:
    using BASE = std::bitset<static_cast<size_t>( SECTION::COUNT )>;

    SECTION_SET() = default;

    // The inherited bitset operators return BASE and this converts the result back
    SECTION_SET( const BASE& aBits ) : BASE( aBits ) {}

    SECTION_SET( std::initializer_list<SECTION> aSections )
    {
        for( SECTION section : aSections )
            Set( section );
    }

    bool Contains( SECTION aSection ) const { return test( static_cast<size_t>( aSection ) ); }

    SECTION_SET& Set( SECTION aSection, bool aOn = true )
    {
        set( static_cast<size_t>( aSection ), aOn );
        return *this;
    }
};

/// Return the Table 4 key character for @a aSection
char SectionKeyChar( SECTION aSection );

/// Return the schema section for a Table 4 key character
std::optional<SECTION> SectionFromKeyChar( char aKey );

/// Return the Table 4 cell for @a aMode and @a aSection
SECTION_RULE SectionRule( MODE aMode, SECTION aSection );

/// Schema sections that Table 4 marks Y for @a aMode
SECTION_SET RequiredSections( MODE aMode );

/// Schema sections that Table 4 marks O for @a aMode
SECTION_SET OptionalSections( MODE aMode );

/// Optional schema sections that @a aMode selects by default
SECTION_SET RecommendedOptionalSections( MODE aMode );

/// Schema sections that no KiCad board supplies
SECTION_SET UnsupportedSections();

/**
 * Schema sections of the content that KiCad wrote before the function modes
 *
 * An unconfigured export keeps this content but now gives its correct section key
 */
SECTION_SET LegacySections();

/// True if an included schema section needs an Ecad/CadData element
bool NeedsCadData( const SECTION_SET& aSet );

/// Give @a aSet as a @c sectionKey attribute value
wxString SectionKeyString( const SECTION_SET& aSet );

/// Read a @c sectionKey attribute value  Return false for an unknown character
bool SectionSetFromKeyString( const wxString& aKey, SECTION_SET& aResult );

/// Table 4 function mode token such as FABRICATION
wxString ModeToken( MODE aMode );

/// Read a Table 4 function mode token in upper case or in lower case
std::optional<MODE> ModeFromToken( const wxString& aToken );

} // namespace FAB

#endif
