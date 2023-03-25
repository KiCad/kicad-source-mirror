/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/spinlock.h>

#include <memory>
#include <mutex>
#include <vector>
#include <wx/string.h>

#include <math/vector2d.h>
#include <geometry/shape_poly_set.h>
#include <zone.h>

class FROM_TO_CACHE;
class CN_CLUSTER;
class CN_CONNECTIVITY_ALGO;
class CN_EDGE;
class BOARD;
class BOARD_COMMIT;
class BOARD_CONNECTED_ITEM;
class BOARD_ITEM;
class ZONE;
class RN_DATA;
class RN_NET;
class PCB_TRACK;
class PAD;
class FOOTPRINT;
class PROGRESS_REPORTER;


struct CN_DISJOINT_NET_ENTRY
{
    int net;
    BOARD_CONNECTED_ITEM *a, *b;
    VECTOR2I anchorA, anchorB;
};


struct RN_DYNAMIC_LINE
{
    int netCode;
    VECTOR2I a, b;
};


/**
 * Controls how nets are propagated through clusters
 */
enum class PROPAGATE_MODE
{
    SKIP_CONFLICTS,     ///< Clusters with conflicting drivers are not updated (default)
    RESOLVE_CONFLICTS   ///< Clusters with conflicting drivers are updated to the most popular net
};


// a wrapper class encompassing the connectivity computation algorithm and the
class CONNECTIVITY_DATA
{
public:
    CONNECTIVITY_DATA();
    ~CONNECTIVITY_DATA();

    CONNECTIVITY_DATA( const std::vector<BOARD_ITEM*>& aItems, bool aSkipItems = false );

    /**
     * Function Build()
     * Builds the connectivity database for the board aBoard.
     */
    bool Build( BOARD* aBoard, PROGRESS_REPORTER* aReporter = nullptr );

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
     * Moves the connectivity list anchors.  N.B., this does not move the bounding
     * boxes for the RTree, so the use of this function will invalidate the
     * connectivity data for uses other than the dynamic ratsnest
     *
     * @param aDelta vector for movement of the tree
     */
    void Move( const VECTOR2I& aDelta );

    /**
     * Function Clear()
     * Erases the connectivity database.
     */
    void ClearRatsnest();

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
     * Propagates the net codes from the source pads to the tracks/vias.
     * @param aCommit is used to save the undo state of items modified by this call
     * @param aMode controls how conflicts between pads are resolved
     */
    void PropagateNets( BOARD_COMMIT* aCommit = nullptr );

    /**
     * Fill the isolate islands list for each layer of each zone.  Isolated islands are individual
     * polygons in a zone fill that don't connect to a net.
     */
    void FillIsolatedIslandsMap( std::map<ZONE*, std::map<PCB_LAYER_ID, ISOLATED_ISLANDS>>& aMap,
                                 bool aConnectivityAlreadyRebuilt = false );

    /**
     * Function RecalculateRatsnest()
     * Updates the ratsnest for the board.
     * @param aCommit is used to save the undo state of items modified by this call
     */
    void RecalculateRatsnest( BOARD_COMMIT* aCommit = nullptr );

    /**
     * @param aVisibleOnly include only visbile edges in the count
     * @return the number of remaining edges in the ratsnest
     */
    unsigned int GetUnconnectedCount( bool aVisibileOnly ) const;

    bool IsConnectedOnLayer( const BOARD_CONNECTED_ITEM* aItem, int aLayer,
                             const std::initializer_list<KICAD_T>& aTypes = {} ) const;

    unsigned int GetNodeCount( int aNet = -1 ) const;

    unsigned int GetPadCount( int aNet = -1 ) const;

    const std::vector<PCB_TRACK*> GetConnectedTracks( const BOARD_CONNECTED_ITEM* aItem ) const;

    const std::vector<PAD*> GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem ) const;

    void GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem, std::set<PAD*>* pads ) const;

    /**
     * Function GetConnectedItemsAtAnchor()
     * Returns a list of items connected to a source item aItem at position aAnchor
     * with an optional maximum distance from the defined anchor.
     * @param aItem is the reference item to find other connected items.
     * @param aAnchor is the position to find connected items on.
     * @param aTypes allows one to filter by item types.
     * @param aMaxError Maximum distance of the found items' anchors to aAnchor in IU
     * @return
     */
    const std::vector<BOARD_CONNECTED_ITEM*>
    GetConnectedItemsAtAnchor( const BOARD_CONNECTED_ITEM* aItem, const VECTOR2I& aAnchor,
                               const std::initializer_list<KICAD_T>& aTypes,
                               const int& aMaxError = 0 ) const;

    void RunOnUnconnectedEdges( std::function<bool( CN_EDGE& )> aFunc );

    bool TestTrackEndpointDangling( PCB_TRACK* aTrack, VECTOR2I* aPos = nullptr ) const;

    /**
     * Function ClearLocalRatsnest()
     * Erases the temporary, selection-based ratsnest (i.e. the ratsnest lines that pcbnew
     * displays when moving an item/set of items).
     */
    void ClearLocalRatsnest();

    /**
     * Hides the temporary, selection-based ratsnest lines.
     */
    void HideLocalRatsnest();

    /**
     * Function ComputeLocalRatsnest()
     * Calculates the temporary (usually selection-based) ratsnest for the set of \a aItems.
     */
    void ComputeLocalRatsnest( const std::vector<BOARD_ITEM*>& aItems,
                               const CONNECTIVITY_DATA* aDynamicData,
                               VECTOR2I aInternalOffset = { 0, 0 } );

    const std::vector<RN_DYNAMIC_LINE>& GetLocalRatsnest() const { return m_dynamicRatsnest; }

    /**
     * Function GetConnectedItems()
     * Returns a list of items connected to a source item aItem.
     * @param aItem is the reference item to find other connected items.
     * @param aTypes allows one to filter by item types.
     */
    const std::vector<BOARD_CONNECTED_ITEM*>
    GetConnectedItems( const BOARD_CONNECTED_ITEM* aItem,
                       const std::initializer_list<KICAD_T>& aTypes,
                       bool aIgnoreNetcodes = false ) const;

    /**
     * Function GetNetItems()
     * Returns the list of items that belong to a certain net.
     * @param aNetCode is the net code.
     * @param aTypes allows one to filter by item types.
     */
    const std::vector<BOARD_CONNECTED_ITEM*>
    GetNetItems( int aNetCode, const std::initializer_list<KICAD_T>& aTypes ) const;

    void BlockRatsnestItems( const std::vector<BOARD_ITEM*>& aItems );

    std::shared_ptr<CN_CONNECTIVITY_ALGO> GetConnectivityAlgo() const { return m_connAlgo; }

    KISPINLOCK& GetLock() { return m_lock; }

    void MarkItemNetAsDirty( BOARD_ITEM* aItem );
    void SetProgressReporter( PROGRESS_REPORTER* aReporter );

    const std::map<int, wxString>& GetNetclassMap() const { return m_netclassMap; }

#ifndef SWIG
    const std::vector<CN_EDGE> GetRatsnestForItems( const std::vector<BOARD_ITEM*> aItems );

    const std::vector<CN_EDGE> GetRatsnestForPad( const PAD* aPad );

    const std::vector<CN_EDGE> GetRatsnestForComponent( FOOTPRINT* aComponent,
                                                        bool aSkipInternalConnections = false );
#endif

    std::shared_ptr<FROM_TO_CACHE> GetFromToCache() { return m_fromToCache; }

private:

    /**
     * Updates the ratsnest for the board without locking the connectivity mutex.
     * @param aCommit is used to save the undo state of items modified by this call
     */
    void internalRecalculateRatsnest( BOARD_COMMIT* aCommit = nullptr );
    void updateRatsnest();

    void addRatsnestCluster( const std::shared_ptr<CN_CLUSTER>& aCluster );

private:
    std::shared_ptr<CN_CONNECTIVITY_ALGO> m_connAlgo;

    std::shared_ptr<FROM_TO_CACHE>  m_fromToCache;
    std::vector<RN_DYNAMIC_LINE>    m_dynamicRatsnest;
    std::vector<RN_NET*>            m_nets;

    /// Used to suppress ratsnest calculations on dynamic ratsnests
    bool                            m_skipRatsnest = false;

    KISPINLOCK                      m_lock;

    /// Map of netcode -> netclass the net is a member of; used for ratsnest painting
    std::map<int, wxString>         m_netclassMap;

    PROGRESS_REPORTER*              m_progressReporter;
};

#endif
