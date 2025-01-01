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

#ifndef DISTRIBUTE_H
#define DISTRIBUTE_H

#include <utility>
#include <vector>

/**
 * Given a list of 'n' item spans (e.g. left-right edge positions), return the deltas for
 * each item to produce even gaps between each item.
 *
 * The first and last items will not be moved.
 *
 * @param aItemExtents a list of 'n' item spans, each a pair of integers representing the
 *                     extents of an item. This should be sorted in a meaningful way to
 *                     the caller, for example, left-right based on the item's left edge
 *                     or by anchor.
 *
 * @return a vector of n deltas, the first and last will be 0.
 */
std::vector<int> GetDeltasForDistributeByGaps( const std::vector<std::pair<int, int>>& aItemExtents);

std::vector<int> GetDeltasForDistributeByPoints( const std::vector<int>& aItemPositions );

#endif // DISTRIBUTE_H