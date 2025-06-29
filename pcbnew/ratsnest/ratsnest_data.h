/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

/**
 * @file ratsnest_data.h
 * @brief Class that computes missing connections on a PCB.
 */

#ifndef RATSNEST_DATA_H
#define RATSNEST_DATA_H

#include <algorithm>
#include <core/typeinfo.h>
#include <math/box2.h>

#include <set>
#include <vector>

#include <connectivity/connectivity_algo.h>

class BOARD_ITEM;
class BOARD_CONNECTED_ITEM;
class CN_CLUSTER;

struct CN_PTR_CMP
{
    bool operator()( const std::shared_ptr<CN_ANCHOR>& aItem,
                     const std::shared_ptr<CN_ANCHOR>& bItem ) const
    {
        if( aItem->Pos().x == bItem->Pos().x )
            return aItem->Pos().y < bItem->Pos().y;
        else
            return aItem->Pos().x < bItem->Pos().x;
    }
};

/**
 * Describe ratsnest for a single net.
 */
class RN_NET
{
public:
    RN_NET();

    /**
     * Return state of the 'dirty' flag, indicating that ratsnest for a given net is invalid
     * and requires an update.
     *
     * @return True if ratsnest requires recomputation, false otherwise.
     */
    bool IsDirty() const { return m_dirty; }

    /**
     * Recompute ratsnest for a net.
     */
    void UpdateNet();

    void RemoveInvalidRefs();

    /**
     * Find optimal ends of RNEdges.  The MST will have found the closest anchors, but when
     * zones are involved we might have points closer than the anchors.
     *
     * Normally called after UpdateNet(), but from a separate multi-threaded loop for safety.
     */
    void OptimizeRNEdges();

    void Clear();

    void AddCluster( std::shared_ptr<CN_CLUSTER> aCluster );

    unsigned int GetNodeCount() const { return m_nodes.size(); }

    const std::vector<CN_EDGE>& GetEdges() const
    {
        // Use a const_cast to allow sorting in the const method
        // This is safe because we're not changing the logical content of the vector,
        // just the ordering of elements
        std::stable_sort( const_cast<std::vector<CN_EDGE>&>( m_rnEdges ).begin(),
                          const_cast<std::vector<CN_EDGE>&>( m_rnEdges ).end(),
                          []( const CN_EDGE& a, const CN_EDGE& b )
                          {
                              return a.StableSortCompare( b );
                          } );
        return m_rnEdges;
    }
    std::vector<CN_EDGE>& GetEdges()
    {
        std::stable_sort( m_rnEdges.begin(), m_rnEdges.end(),
                          []( const CN_EDGE& a, const CN_EDGE& b )
                          {
                              return a.StableSortCompare( b );
                          } );
        return m_rnEdges;
    }

    bool NearestBicoloredPair( RN_NET* aOtherNet, VECTOR2I& aPos1, VECTOR2I& aPos2 ) const;

protected:
    ///< Recompute ratsnest from scratch.
    void compute();

    ///< Compute the minimum spanning tree using Kruskal's algorithm
    void kruskalMST( const std::vector<CN_EDGE> &aEdges );

protected:
    ///< Vector of nodes
    std::multiset<std::shared_ptr<CN_ANCHOR>, CN_PTR_CMP> m_nodes;

    ///< Vector of edges that make pre-defined connections
    std::vector<CN_EDGE> m_boardEdges;

    ///< Vector of edges that makes ratsnest for a given net.
    std::vector<CN_EDGE> m_rnEdges;

    ///< Flag indicating necessity of recalculation of ratsnest for a net.
    bool m_dirty;

    class TRIANGULATOR_STATE;

    std::shared_ptr<TRIANGULATOR_STATE> m_triangulator;
};

#endif /* RATSNEST_DATA_H */
