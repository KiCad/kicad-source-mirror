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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math/vector2d.h>
#include <sch_rtree.h>

namespace JUNCTION_HELPERS
{

/**
 * A selection of information about a point in the schematic that might
 * be eligible for turning into a junction.
*/
struct POINT_INFO
{
    /// True if the point has 3+ wires and/or 3+ buses meeting there
    bool isJunction;

    /// True if there is already junction dot at the point
    bool hasExplicitJunctionDot;

    /// True if there is a bus entry at the point (either end)
    bool hasBusEntry;

    /// True if there is a bus entry at the point and it connects to more than one wire
    bool hasBusEntryToMultipleWires;
};

/**
 * Check a tree of items for a confluence at a given point and work out what kind of junction
 * it is, if any.
 */
POINT_INFO AnalyzePoint( const EE_RTREE& aItem, const VECTOR2I& aPosition, bool aBreakCrossings );

/**
 * Determine the points where explicit junctions would be required if the given
 * temporary items were committed to the schematic.
 *
 * @param aScreen  The schematic screen containing the existing items.
 * @param aItems   Temporary items not yet added to the screen.
 * @return Locations of needed junctions represented as new SCH_JUNCTION items.
 */
std::vector<class SCH_JUNCTION*> PreviewJunctions( const class SCH_SCREEN* aScreen,
                                                   const std::vector<class SCH_ITEM*>& aItems );

} // namespace JUNCTION_HELPERS
