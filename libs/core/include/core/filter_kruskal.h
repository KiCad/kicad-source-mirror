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

#ifndef KICAD_CORE_FILTER_KRUSKAL_H
#define KICAD_CORE_FILTER_KRUSKAL_H

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>

#include <core/union_find.h>

/**
 * Minimum spanning forest by Filter-Kruskal.
 *
 * Plain Kruskal sorts every edge, although most edges of a dense graph close a cycle and are
 * discarded as soon as the scan reaches them.  Filter-Kruskal does not sort those.  It splits
 * the edges at a pivot, recurses into the lighter half, then discards each edge of the heavier
 * half whose endpoints the lighter half already joined.  The cycle property keeps the result
 * the same as plain Kruskal.
 *
 * Osipov, V., Sanders, P., & Singler, J. (2009). The Filter-Kruskal minimum spanning tree
 * algorithm. Proceedings of the Meeting on Algorithm Engineering & Experiments (ALENEX),
 * 52-61. SIAM.
 */
namespace KI_MST
{

///< Sort and scan a range of this size or smaller instead of splitting it again.
static constexpr size_t KRUSKAL_THRESHOLD = 1024;


/**
 * Build a minimum spanning forest over \a aEdges.
 *
 * @param aEdges is the candidate edge set.  This function reorders it.
 * @param aForest is the union-find state.  A caller can unite known-connected pairs first,
 *                because this function discards the edges those pairs imply.
 * @param aLess must be a strict total order on edges and not on their weights alone.  A tie
 *              that the order leaves open makes the chosen tree depend on the platform.
 * @param aEndpoints maps an edge to its two vertex indices, both below aForest.Size().
 * @param aEmit receives each selected edge in order of increasing weight.
 * @return the number of edges emitted.
 */
template <typename EDGE, typename LESS, typename ENDPOINTS, typename EMIT>
size_t FilterKruskal( std::span<EDGE> aEdges, KI_UNION_FIND& aForest, LESS aLess,
                      ENDPOINTS aEndpoints, EMIT aEmit )
{
    size_t emitted = 0;

    // Sort the range and run plain Kruskal over it
    auto kruskal =
            [&]( std::span<EDGE> aRange )
            {
                std::sort( aRange.begin(), aRange.end(), aLess );

                for( const EDGE& edge : aRange )
                {
                    const auto [u, v] = aEndpoints( edge );

                    if( aForest.Unite( u, v ) )
                    {
                        aEmit( edge );
                        ++emitted;
                    }
                }
            };

    // An edge whose endpoints are already joined closes a cycle, so no minimum spanning
    // forest can hold it and nothing needs to sort it
    auto filter =
            [&]( std::span<EDGE> aRange ) -> std::span<EDGE>
            {
                auto end = std::partition( aRange.begin(), aRange.end(),
                                           [&]( const EDGE& aEdge )
                                           {
                                               const auto [u, v] = aEndpoints( aEdge );

                                               return !aForest.Connected( u, v );
                                           } );

                return aRange.subspan( 0, std::distance( aRange.begin(), end ) );
            };

    auto recurse =
            [&]( auto&& aSelf, std::span<EDGE> aRange ) -> void
            {
                // Nothing later can be selected once everything is joined
                if( aRange.empty() || aForest.ComponentCount() == 1 )
                    return;

                if( aRange.size() <= KRUSKAL_THRESHOLD )
                {
                    kruskal( aRange );
                    return;
                }

                // nth_element places the median with the lighter edges ahead of it, so both
                // halves shrink.  It is also deterministic, unlike a random pivot
                size_t mid = aRange.size() / 2;
                std::nth_element( aRange.begin(), aRange.begin() + mid, aRange.end(), aLess );

                aSelf( aSelf, aRange.subspan( 0, mid ) );

                if( aForest.ComponentCount() == 1 )
                    return;

                aSelf( aSelf, filter( aRange.subspan( mid ) ) );
            };

    recurse( recurse, aEdges );

    return emitted;
}

} // namespace KI_MST

#endif // KICAD_CORE_FILTER_KRUSKAL_H
