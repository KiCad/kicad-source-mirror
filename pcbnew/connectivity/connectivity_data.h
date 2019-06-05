/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __CONNECTIVITY_DATA_H
#define __CONNECTIVITY_DATA_H

#include <core/typeinfo.h>

#include <memory>
#include <mutex>
#include <vector>
#include <wx/string.h>

#include <math/vector2d.h>
#include <geometry/shape_poly_set.h>
#include <class_zone.h>

class CN_CLUSTER;
class CN_CONNECTIVITY_ALGO;
class CN_EDGE;
class BOARD;
class BOARD_COMMIT;
class BOARD_CONNECTED_ITEM;
class BOARD_ITEM;
class ZONE_CONTAINER;
class RN_DATA;
class RN_NET;
class TRACK;
class D_PAD;
class MODULE;
class PROGRESS_REPORTER;

struct CN_DISJOINT_NET_ENTRY
{
    int net;
    BOARD_CONNECTED_ITEM *a, *b;
    VECTOR2I anchorA, anchorB;
};

struct CN_ZONE_ISOLATED_ISLAND_LIST
{
    CN_ZONE_ISOLATED_ISLAND_LIST( ZONE_CONTAINER* aZone ) :
        m_zone( aZone )
    {}

    ZONE_CONTAINER*      m_zone;
    std::vector<int>     m_islands;
};

struct RN_DYNAMIC_LINE
{
    int netCode;
    VECTOR2I a, b;
};

// a wrapper class encompassing the connectivity computation algorithm and the
class CONNECTIVITY_DATA
{
public:
    CONNECTIVITY_DATA();
    ~CONNECTIVITY_DATA();

    CONNECTIVITY_DATA( const std::vector<BOARD_ITEM*>& aItems );

    /**
     * Function Build()
     * Builds the connectivity database for the board aBoard.
     */
    void Build( BOARD* aBoard );

    /**
     * Function Build()
     * Builds the connectivity database for a set of items aItems.
     */
    void Build( const std::vector<BOARD_ITEM*>& aItems );

    /**
     * Function Add()
     * Adds an item to the connectivity data.
     * @param aItem is an item to be added.
     * @return True if operation succeeded.
     */
    bool Add( BOARD_ITEM* aItem );

    /**
     * Function Remove()
     * Removes an item from the connectivity data.
     * @param aItem is an item to be updated.
     * @return True if operation succeeded.
     */
    bool Remove( BOARD_ITEM* aItem );

    /**
     * Function Update()
     * Updates the connectivity data for an item.
     * @param aItem is an item to be updated.
     * @return True if operation succeeded.
     */
    bool Update( BOARD_ITEM* aItem );

    /**
     * Function Clear()
     * Erases the connectivity database.
     */
    void Clear();

    /**
     * Function GetNetCount()
     * Returns the total number of nets in the connectivity database.
     */
    int GetNetCount() const;

    /**
     * Function GetRatsnestForNet()
     * Returns the ratsnest, expressed as a set of graph edges for a given net.
     */
    RN_NET* GetRatsnestForNet( int aNet );

    /**
     * Function PropagateNets()
     * Propagates the net codes from the source pads to the tracks/vias.
     */
    void PropagateNets();

    bool CheckConnectivity( std::vector<CN_DISJOINT_NET_ENTRY>& aReport );

    /**
     * Function FindIsolatedCopperIslands()
     * Searches for copper islands in zone aZone that are not connected to any pad.
     * @param aZone zone to test
     * @param aIslands list of islands that have no connections (outline indices in the polygon set)
     */
    void FindIsolatedCopperIslands( ZONE_CONTAINER* aZone, std::vector<int>& aIslands );
    void FindIsolatedCopperIslands( std::vector<CN_ZONE_ISOLATED_ISLAND_LIST>& aZones );

    /**
     * Function RecalculateRatsnest()
     * Updates the ratsnest for the board.
     * @param aCommit is used to save the undo state of items modified by this call
     */
    void RecalculateRatsnest( BOARD_COMMIT* aCommit = nullptr );

    /**
     * Function GetUnconnectedCount()
     * Returns the number of remaining edges in the ratsnest.
     */
    unsigned int GetUnconnectedCount() const;

    unsigned int GetNodeCount( int aNet = -1 ) const;

    unsigned int GetPadCount( int aNet = -1 ) const;

    const std::vector<TRACK*> GetConnectedTracks( const BOARD_CONNECTED_ITEM* aItem ) const;

    const std::vector<D_PAD*> GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem ) const;

    const void GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem, std::set<D_PAD*>* pads ) const;

    const std::vector<BOARD_CONNECTED_ITEM*> GetConnectedItems( const BOARD_CONNECTED_ITEM* aItem, const VECTOR2I& aAnchor, KICAD_T aTypes[] );

    void GetUnconnectedEdges( std::vector<CN_EDGE>& aEdges ) const;

    /**
     * Function ClearDynamicRatsnest()
     * Erases the temporary dynamic ratsnest (i.e. the ratsnest lines that
     * pcbnew displays when moving an item/set of items)
     */
    void ClearDynamicRatsnest();

    /**
     * Hides the temporary dynamic ratsnest lines.
     */
    void HideDynamicRatsnest();

    /**
     * Function ComputeDynamicRatsnest()
     * Calculates the temporary dynamic ratsnest (i.e. the ratsnest lines that)
     * for the set of items aItems.
     */
    void ComputeDynamicRatsnest( const std::vector<BOARD_ITEM*>& aItems );

    const std::vector<RN_DYNAMIC_LINE>& GetDynamicRatsnest() const
    {
        return m_dynamicRatsnest;
    }

    /**
     * Function GetConnectedItems()
     * Returns a list of items connected to a source item aItem.
     * @param aItem is the reference item to find other connected items.
     * @param aTypes allows one to filter by item types.
     */
    const std::vector<BOARD_CONNECTED_ITEM*> GetConnectedItems( const BOARD_CONNECTED_ITEM* aItem,
            const KICAD_T aTypes[], bool aIgnoreNetcodes = false ) const;

    /**
     * Function GetNetItems()
     * Returns the list of items that belong to a certain net.
     * @param aNetCode is the net code.
     * @param aTypes allows one to filter by item types.
     */
    const std::vector<BOARD_CONNECTED_ITEM*> GetNetItems( int aNetCode,
            const KICAD_T aTypes[] ) const;

    const std::vector<VECTOR2I> NearestUnconnectedTargets( const BOARD_CONNECTED_ITEM* aRef,
            const VECTOR2I& aPos,
            int aMaxCount = -1 );

    void BlockRatsnestItems( const std::vector<BOARD_ITEM*>& aItems );

    std::shared_ptr<CN_CONNECTIVITY_ALGO> GetConnectivityAlgo() const
    {
        return m_connAlgo;
    }

    std::mutex& GetLock()
    {
        return m_lock;
    }

    void MarkItemNetAsDirty( BOARD_ITEM* aItem );
    void SetProgressReporter( PROGRESS_REPORTER* aReporter );

#ifndef SWIG
    const std::vector<CN_EDGE> GetRatsnestForComponent( MODULE* aComponent, bool aSkipInternalConnections = false );
#endif

private:

    void    updateRatsnest();
    void    addRatsnestCluster( const std::shared_ptr<CN_CLUSTER>& aCluster );

    std::shared_ptr<CN_CONNECTIVITY_ALGO> m_connAlgo;

    std::vector<RN_DYNAMIC_LINE> m_dynamicRatsnest;
    std::vector<RN_NET*> m_nets;

    PROGRESS_REPORTER* m_progressReporter;

    std::mutex m_lock;
};

#endif
