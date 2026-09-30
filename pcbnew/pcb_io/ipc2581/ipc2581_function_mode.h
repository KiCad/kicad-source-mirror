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

#ifndef IPC2581_FUNCTION_MODE_H
#define IPC2581_FUNCTION_MODE_H

#include <bitset>
#include <initializer_list>
#include <optional>
#include <vector>

#include <exporters/fab_model/fab_sections.h>
#include <layer_ids.h>
#include <wx/string.h>

namespace IPC2581
{

/// Schema revision  The two revisions declare different identity constraints
enum class REVISION
{
    B,
    C
};

/**
 * Optional attributes to remove when their schema section is absent
 *
 * Removal is better than the addition of a section that the user did not request
 */
enum class SUPPRESS
{
    COMPONENT_REFDES,       ///< Component@\refDes
    COMPONENT_PACKAGEREF,   ///< Component\@packageRef
    PINREF_COMPONENTREF,    ///< Pad/PinRef\@componentRef and LogicalNet/PinRef\@componentRef
    PAD_PADSTACKDEFREF,     ///< Pad\@padstackDefRef, and the PadStackDef elements themselves
    BOM_REFDES,             ///< Bom/BomItem/RefDes and all its attributes
    BOM_REFDES_LAYERREF,    ///< Bom/BomItem/RefDes\@layerRef
    BOM_REFDES_PACKAGEREF,  ///< Bom/BomItem/RefDes\@packageRef
    BOM_HEADER_STEPREF,     ///< Bom/BomHeader/StepRef

    COUNT
};

/// Set of removed attributes
class SUPPRESS_SET : public std::bitset<static_cast<size_t>( SUPPRESS::COUNT )>
{
public:
    bool Contains( SUPPRESS aWhat ) const { return test( static_cast<size_t>( aWhat ) ); }

    SUPPRESS_SET& Set( SUPPRESS aWhat, bool aOn = true )
    {
        set( static_cast<size_t>( aWhat ), aOn );
        return *this;
    }
};

/// A requested combination that no legal file can hold
struct CONFLICT
{
    std::optional<SECTION> m_section;
    std::optional<SECTION> m_requires;
    wxString               m_message;
};

struct RESOLVE_RESULT
{
    SECTION_SET           m_included;
    SUPPRESS_SET          m_suppressions;
    std::vector<CONFLICT> m_conflicts;

    bool Ok() const { return m_conflicts.empty(); }
};

/**
 * True if KiCad can write @a aMode for @a aRevision
 *
 * Revision B has no DFX function mode and KiCad has no DFX measurements
 */
bool ModeSupported( MODE aMode, REVISION aRevision );

/// Schema section that holds the artwork of @a aLayer
std::optional<SECTION> SectionForBoardLayer( PCB_LAYER_ID aLayer );

/**
 * Schema section that holds the layers with @a aLayerFunction
 *
 * A conductor layer needs @a aSide to divide the outer copper from the inner copper
 */
std::optional<SECTION> SectionForLayerFunction( const wxString& aLayerFunction,
                                                const wxString& aSide = wxEmptyString );

/**
 * Compare @a aRequested with Table 4 and give the attributes to remove and the conflicts
 *
 * This function adds no schema section other than the Y cells of the function mode
 * It removes an attribute when the target section is absent
 * The caller must mask @a aRequested with the content that the board supplies
 */
RESOLVE_RESULT ResolveSections( REVISION aRevision, MODE aMode, const SECTION_SET& aRequested );

} // namespace IPC2581

#endif // IPC2581_FUNCTION_MODE_H
