/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DRILL_SYMBOL_ASSIGNER_H
#define DRILL_SYMBOL_ASSIGNER_H

#include <map>
#include <string>
#include <vector>

#include <drill/drill_chart_model.h>
#include <drill/drill_symbol_profile.h>

class BOARD;


/**
 * Give every group a mark, keeping the ones the profile already records.
 *
 * With freeze on, an existing group never changes symbol because a new hole size appeared;
 * new groups take the lowest mark nobody is using. Fabrication drawings get re-approved when
 * symbols move, so churn here is expensive for the user.
 *
 * @param aGroups is updated in place with the resolved marks.
 * @param aProfile gains assignments for any group that did not have one.
 */
void AssignDrillSymbols( std::vector<DRILL_CHART_GROUP>& aGroups, DRILL_SYMBOL_PROFILE& aProfile );

/**
 * The symbol every hole should carry, without touching the board.
 *
 * A drill map has to draw the same marks whether or not a chart has ever been placed, so
 * consumers resolve through here rather than relying on the profile already holding
 * assignments. Assignment happens on a copy. Only a chart rebuild commits new ones.
 */
std::map<std::string, DRILL_SYMBOL_ASSIGNMENT> ResolveDrillSymbols( const BOARD& aBoard );

/**
 * One drawable mark, with the geometry the renderer needs to place it.
 *
 * The diameter is carried because a size-text mark has to print the hole it labels. Deriving
 * it from the glyph size would label every hole with the glyph size instead.
 */
struct DRILL_SYMBOL_ENTRY
{
    DRILL_SYMBOL_ASSIGNMENT m_Symbol;
    DRILL_SPAN              m_Span;

    /**
     * Where the mark is drawn, which is the hole itself. Kept so a drill map can be hit
     * tested against its marks without walking the board.
     */
    VECTOR2I                m_Position;

    DRILL_OP_KIND           m_Kind = DRILL_OP_KIND::PRIMARY_DRILL;
    int                     m_Diameter = 0;
    VECTOR2I                m_SizeXY;
    EDA_ANGLE               m_Orientation;
    bool                    m_IsSlot = false;
};

/**
 * The same answer keyed by the item that owns the holes.
 *
 * Built once alongside ResolveDrillSymbols so the painter can look a hole up rather than
 * rescanning every operation, which would make a repaint quadratic in the hole count.
 *
 * One item maps to several entries. A backdrilled via owns its primary drill and both
 * backdrills, and they carry different symbols. Keeping only the primary would silently
 * drop the marks a backdrill map exists to show.
 */
std::map<KIID, std::vector<DRILL_SYMBOL_ENTRY>> ResolveDrillSymbolsByItem(
        const BOARD& aBoard, const std::map<std::string, DRILL_SYMBOL_ASSIGNMENT>& aResolved );

#endif // DRILL_SYMBOL_ASSIGNER_H
