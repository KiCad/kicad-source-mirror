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

#include <geometry/distribute.h>

#include <math/vector2d.h>


std::vector<int> GetDeltasForDistributeByGaps(const std::vector<std::pair<int, int>>& aItemExtents)
{
    std::vector<int> deltas(aItemExtents.size(), 0);

    // This only makes sense for 3 or more items
    if (aItemExtents.size() < 3)
        return deltas;

    // The space between the first and last items' inner edges
    const int totalSpace = aItemExtents.back().first - aItemExtents.front().second;
    int totalGap = totalSpace;

    for( size_t i = 1; i < aItemExtents.size() - 1; ++i )
    {
        const auto& [start, end] = aItemExtents[i];
        totalGap -= end - start;
    }

    const double perItemGap = totalGap / double( aItemExtents.size() - 1 );

    // Start counting at the end of the first item
    int targetPos = aItemExtents.begin()->second;

    // End-cap items don't need to be changed
    for( size_t i = 1; i < aItemExtents.size() - 1; ++i )
    {
        const auto& [start, end] = aItemExtents[i];

        // Take care not to stack rounding errors by keeping the integer accumulator
        // separate and always re-multiplying the gap
        const double accumulatedGaps = i * perItemGap;
        const int delta = targetPos - start + KiROUND(accumulatedGaps);

        deltas[i] = delta;

        // Step over one item span (width or height)
        targetPos += end - start;
    }

    return deltas;
}

std::vector<int> GetDeltasForDistributeByPoints( const std::vector<int>& aItemPositions )
{
    std::vector<int> deltas(aItemPositions.size(), 0);

    // This only makes sense for 3 or more items
    if (aItemPositions.size() < 3)
        return deltas;

    const int    startPos = aItemPositions.front();
    const int    totalGaps = aItemPositions.back() - startPos;
    const double itemGap = totalGaps / double( aItemPositions.size() - 1 );

    // End-cap items don't need to be changed
    for( size_t i = 1; i < aItemPositions.size() - 1; ++i )
    {
        const int      targetPos = startPos + KiROUND( i * itemGap );
        const int      delta = targetPos - aItemPositions[i];

        deltas[i] = delta;
    }

    return deltas;
}