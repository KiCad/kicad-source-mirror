/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
    bool operator()( const CN_ANCHOR_PTR& aItem, const CN_ANCHOR_PTR& bItem ) const
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
     * Set state of the visibility flag.
     *
     * @param aEnabled is new state. True if ratsnest for a given net is meant to be displayed,
     *                 false otherwise.
     */
    void SetVisible( bool aEnabled );

    /**
     * Mark ratsnest for given net as 'dirty', i.e. requiring recomputation.
     */
    void MarkDirty() { m_dirty = true; }

    /**
     * Return state of the 'dirty' flag, indicating that ratsnest for a given net is invalid
     * and requires an update.
     *
     * @return True if ratsnest requires recomputation, false otherwise.
     */
    bool IsDirty() const { return m_dirty; }

    /**
     * Return pointer to a vector of edges that makes ratsnest for a given net.
     */
    const std::vector<CN_EDGE> GetUnconnected() const { return m_rnEdges; }

    /**
     * Recompute ratsnest for a net.
     */
    void Update();
    void Clear();

    void AddCluster( std::shared_ptr<CN_CLUSTER> aCluster );

    unsigned int GetNodeCount() const { return m_nodes.size(); }

    const std::vector<CN_EDGE>& GetEdges() const { return m_rnEdges; }

    bool NearestBicoloredPair( const RN_NET& aOtherNet, CN_ANCHOR_PTR& aNode1,
                               CN_ANCHOR_PTR& aNode2 ) const;

protected:
    ///< Recompute ratsnest from scratch.
    void compute();

    ///< Compute the minimum spanning tree using Kruskal's algorithm
    void kruskalMST( const std::vector<CN_EDGE> &aEdges );

    ///< Vector of nodes
    std::multiset<CN_ANCHOR_PTR, CN_PTR_CMP> m_nodes;

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
