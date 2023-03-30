/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016-2018 CERN
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
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


#include <algorithm>
#include <future>
#include <mutex>

#include <connectivity/connectivity_algo.h>
#include <progress_reporter.h>
#include <geometry/geometry_utils.h>
#include <board_commit.h>
#include <thread_pool.h>

#include <wx/log.h>

#ifdef PROFILE
#include <profile.h>
#endif


bool CN_CONNECTIVITY_ALGO::Remove( BOARD_ITEM* aItem )
{
    markItemNetAsDirty( aItem );

    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
        for( PAD* pad : static_cast<FOOTPRINT*>( aItem )->Pads() )
        {
            m_itemMap[pad].MarkItemsAsInvalid();
            m_itemMap.erase( pad );
        }

        m_itemList.SetDirty( true );
        break;

    case PCB_PAD_T:
        m_itemMap[aItem].MarkItemsAsInvalid();
        m_itemMap.erase( aItem );
        m_itemList.SetDirty( true );
        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
        m_itemMap[aItem].MarkItemsAsInvalid();
        m_itemMap.erase( aItem );
        m_itemList.SetDirty( true );
        break;

    case PCB_VIA_T:
        m_itemMap[aItem].MarkItemsAsInvalid();
        m_itemMap.erase( aItem );
        m_itemList.SetDirty( true );
        break;

    case PCB_ZONE_T:
        m_itemMap[aItem].MarkItemsAsInvalid();
        m_itemMap.erase ( aItem );
        m_itemList.SetDirty( true );
        break;

    default:
        return false;
    }

    // Once we delete an item, it may connect between lists, so mark both as potentially invalid
    m_itemList.SetHasInvalid( true );

    return true;
}


void CN_CONNECTIVITY_ALGO::markItemNetAsDirty( const BOARD_ITEM* aItem )
{
    if( aItem->IsConnected() )
    {
        const BOARD_CONNECTED_ITEM* citem = static_cast<const BOARD_CONNECTED_ITEM*>( aItem );
        MarkNetAsDirty( citem->GetNetCode() );
    }
    else
    {
        if( aItem->Type() == PCB_FOOTPRINT_T )
        {
            const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( aItem );

            for( PAD* pad : footprint->Pads() )
                MarkNetAsDirty( pad->GetNetCode() );
        }
    }
}


bool CN_CONNECTIVITY_ALGO::Add( BOARD_ITEM* aItem )
{
    if( !aItem->IsOnCopperLayer() )
        return false;

    switch( aItem->Type() )
    {
    case PCB_NETINFO_T:
        MarkNetAsDirty( static_cast<NETINFO_ITEM*>( aItem )->GetNetCode() );
        break;

    case PCB_FOOTPRINT_T:
    {
        if( static_cast<FOOTPRINT*>( aItem )->GetAttributes() & FP_JUST_ADDED )
            return false;

        for( PAD* pad : static_cast<FOOTPRINT*>( aItem )->Pads() )
        {
            if( m_itemMap.find( pad ) != m_itemMap.end() )
                return false;

            add( m_itemList, pad );
        }

        break;
    }

    case PCB_PAD_T:
    {
        if( FOOTPRINT* fp = aItem->GetParentFootprint() )
        {
            if( fp->GetAttributes() & FP_JUST_ADDED )
                return false;
        }

        if( m_itemMap.find( aItem ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<PAD*>( aItem ) );
        break;
    }

    case PCB_TRACE_T:
        if( m_itemMap.find( aItem ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<PCB_TRACK*>( aItem ) );
        break;

    case PCB_ARC_T:
        if( m_itemMap.find( aItem ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<PCB_ARC*>( aItem ) );
        break;

    case PCB_VIA_T:
        if( m_itemMap.find( aItem ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<PCB_VIA*>( aItem ) );
        break;

    case PCB_ZONE_T:
    {
        ZONE* zone = static_cast<ZONE*>( aItem );

        if( m_itemMap.find( aItem ) != m_itemMap.end() )
            return false;

        m_itemMap[zone] = ITEM_MAP_ENTRY();

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            for( CN_ITEM* zitem : m_itemList.Add( zone, layer ) )
                m_itemMap[zone].Link( zitem );
        }
    }
        break;

    default:
        return false;
    }

    markItemNetAsDirty( aItem );

    return true;
}


void CN_CONNECTIVITY_ALGO::searchConnections()
{
#ifdef PROFILE
    PROF_TIMER garbage_collection( "garbage-collection" );
#endif
    std::vector<CN_ITEM*> garbage;
    garbage.reserve( 1024 );

    m_itemList.RemoveInvalidItems( garbage );

    for( CN_ITEM* item : garbage )
        delete item;

#ifdef PROFILE
    garbage_collection.Show();
    PROF_TIMER search_basic( "search-basic" );
#endif

    thread_pool& tp = GetKiCadThreadPool();
    std::vector<CN_ITEM*> dirtyItems;
    std::copy_if( m_itemList.begin(), m_itemList.end(), std::back_inserter( dirtyItems ),
                  [] ( CN_ITEM* aItem )
                  {
                      return aItem->Dirty();
                  } );

    if( m_progressReporter )
    {
        m_progressReporter->SetMaxProgress( dirtyItems.size() );

        if( !m_progressReporter->KeepRefreshing() )
            return;
    }

    if( m_itemList.IsDirty() )
    {

        std::vector<std::future<size_t>> returns( dirtyItems.size() );

        auto conn_lambda =
                [&dirtyItems]( size_t aItem, CN_LIST* aItemList,
                               PROGRESS_REPORTER* aReporter) -> size_t
                {
                    if( aReporter && aReporter->IsCancelled() )
                        return 0;

                    CN_VISITOR visitor( dirtyItems[aItem] );
                    aItemList->FindNearby( dirtyItems[aItem], visitor );

                    if( aReporter )
                        aReporter->AdvanceProgress();

                    return 1;
                };

        for( size_t ii = 0; ii < dirtyItems.size(); ++ii )
            returns[ii] = tp.submit( conn_lambda, ii, &m_itemList, m_progressReporter );

        for( const std::future<size_t>& ret : returns )
        {
            // Here we balance returns with a 250ms timeout to allow UI updating
            std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

            while( status != std::future_status::ready )
            {
                if( m_progressReporter )
                    m_progressReporter->KeepRefreshing();

                status = ret.wait_for( std::chrono::milliseconds( 250 ) );
            }
        }

        if( m_progressReporter )
            m_progressReporter->KeepRefreshing();
    }

#ifdef PROFILE
        search_basic.Show();
#endif

    m_itemList.ClearDirtyFlags();
}


const CN_CONNECTIVITY_ALGO::CLUSTERS CN_CONNECTIVITY_ALGO::SearchClusters( CLUSTER_SEARCH_MODE aMode )
{
    if( aMode == CSM_PROPAGATE )
    {
        return SearchClusters( aMode,
                               { PCB_TRACE_T, PCB_ARC_T, PCB_PAD_T, PCB_VIA_T, PCB_FOOTPRINT_T },
                               -1 );
    }
    else
    {
        return SearchClusters( aMode,
                               { PCB_TRACE_T, PCB_ARC_T, PCB_PAD_T, PCB_VIA_T, PCB_ZONE_T, PCB_FOOTPRINT_T },
                               -1 );
    }
}


const CN_CONNECTIVITY_ALGO::CLUSTERS
CN_CONNECTIVITY_ALGO::SearchClusters( CLUSTER_SEARCH_MODE aMode,
                                      const std::initializer_list<KICAD_T>& aTypes,
                                      int aSingleNet, CN_ITEM* rootItem )
{
    bool withinAnyNet = ( aMode != CSM_PROPAGATE );

    std::deque<CN_ITEM*> Q;
    std::set<CN_ITEM*> item_set;

    CLUSTERS clusters;

    if( m_itemList.IsDirty() )
        searchConnections();

    auto addToSearchList =
            [&item_set, withinAnyNet, aSingleNet, &aTypes, rootItem ]( CN_ITEM *aItem )
            {
                if( withinAnyNet && aItem->Net() <= 0 )
                    return;

                if( !aItem->Valid() )
                    return;

                if( aSingleNet >=0 && aItem->Net() != aSingleNet )
                    return;

                bool found = false;

                for( KICAD_T type : aTypes )
                {
                    if( aItem->Parent()->Type() == type )
                    {
                        found = true;
                        break;
                    }
                }

                if( !found && aItem != rootItem )
                    return;

                aItem->SetVisited( false );

                item_set.insert( aItem );
            };

    std::for_each( m_itemList.begin(), m_itemList.end(), addToSearchList );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return CLUSTERS();

    while( !item_set.empty() )
    {
        std::shared_ptr<CN_CLUSTER> cluster = std::make_shared<CN_CLUSTER>();
        CN_ITEM*                    root;
        auto                        it = item_set.begin();

        while( it != item_set.end() && (*it)->Visited() )
            it = item_set.erase( item_set.begin() );

        if( it == item_set.end() )
            break;

        root = *it;
        root->SetVisited( true );

        Q.clear();
        Q.push_back( root );

        while( Q.size() )
        {
            CN_ITEM* current = Q.front();

            Q.pop_front();
            cluster->Add( current );

            for( CN_ITEM* n : current->ConnectedItems() )
            {
                if( withinAnyNet && n->Net() != root->Net() )
                    continue;

                if( !n->Visited() && n->Valid() )
                {
                    n->SetVisited( true );
                    Q.push_back( n );
                }
            }
        }

        clusters.push_back( cluster );
    }

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return CLUSTERS();

    std::sort( clusters.begin(), clusters.end(),
               []( const std::shared_ptr<CN_CLUSTER>& a, const std::shared_ptr<CN_CLUSTER>& b )
               {
                   return a->OriginNet() < b->OriginNet();
               } );

    return clusters;
}


void CN_CONNECTIVITY_ALGO::Build( BOARD* aBoard, PROGRESS_REPORTER* aReporter )
{
    // Generate CN_ZONE_LAYERs for each island on each layer of each zone
    //
    std::vector<CN_ZONE_LAYER*> zitems;

    for( ZONE* zone : aBoard->Zones() )
    {
        if( zone->IsOnCopperLayer() )
        {
            m_itemMap[zone] = ITEM_MAP_ENTRY();
            markItemNetAsDirty( zone );

            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                if( IsCopperLayer( layer ) )
                {
                    for( int j = 0; j < zone->GetFilledPolysList( layer )->OutlineCount(); j++ )
                        zitems.push_back( new CN_ZONE_LAYER( zone, layer, j ) );
                }
            }
        }
    }

    // Setup progress metrics
    //
    int    progressDelta = 50;
    double size = 0.0;

    size += zitems.size();        // Once for building RTrees
    size += zitems.size();        // Once for adding to connectivity
    size += aBoard->Tracks().size();

    for( FOOTPRINT* footprint : aBoard->Footprints() )
        size += footprint->Pads().size();

    size *= 1.5;                  // Our caller gets the other third of the progress bar

    progressDelta = std::max( progressDelta, (int) size / 4 );

    auto report =
            [&]( int progress )
            {
                if( aReporter && ( progress % progressDelta ) == 0 )
                {
                    aReporter->SetCurrentProgress( progress / size );
                    aReporter->KeepRefreshing( false );
                }
            };

    // Generate RTrees for CN_ZONE_LAYER items (in parallel)
    //
    thread_pool& tp = GetKiCadThreadPool();
    std::vector<std::future<size_t>> returns( zitems.size() );

    auto cache_zones =
            [aReporter]( CN_ZONE_LAYER* aZoneLayer ) -> size_t
            {
                if( aReporter && aReporter->IsCancelled() )
                    return 0;

                aZoneLayer->BuildRTree();

                if( aReporter )
                    aReporter->AdvanceProgress();

                return 1;
            };

    for( size_t ii = 0; ii < zitems.size(); ++ii )
        returns[ii] = tp.submit( cache_zones, zitems[ii] );

    for( const std::future<size_t>& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            if( aReporter )
                aReporter->KeepRefreshing();

            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }

    }

    // Add CN_ZONE_LAYERS, tracks, and pads to connectivity
    //
    int ii = zitems.size();

    for( CN_ZONE_LAYER* zitem : zitems )
    {
        m_itemList.Add( zitem );
        m_itemMap[ zitem->Parent() ].Link( zitem );
        report( ++ii );
    }

    for( PCB_TRACK* tv : aBoard->Tracks() )
    {
        Add( tv );
        report( ++ii );
    }

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            Add( pad );
            report( ++ii );
        }
    }

    if( aReporter )
    {
        aReporter->SetCurrentProgress( (double) ii / (double) size );
        aReporter->KeepRefreshing( false );
    }
}


void CN_CONNECTIVITY_ALGO::LocalBuild( const std::vector<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
        case PCB_PAD_T:
        case PCB_FOOTPRINT_T:
            Add( item );
            break;

        default:
            break;
        }
    }
}


void CN_CONNECTIVITY_ALGO::propagateConnections( BOARD_COMMIT* aCommit )
{
    for( const std::shared_ptr<CN_CLUSTER>& cluster : m_connClusters )
    {
        if( cluster->IsConflicting() )
        {
            // Conflicting pads in cluster: we don't know the user's intent so best to do
            // nothing.
            wxLogTrace( wxT( "CN" ), wxT( "Conflicting pads in cluster %p; skipping propagation" ),
                        cluster.get() );
        }
        else if( cluster->HasValidNet() )
        {
            // Propagate from the origin (will be a pad if there are any, or another item if
            // there are no pads).
            int n_changed = 0;

            for( CN_ITEM* item : *cluster )
            {
                if( item->Valid() && item->CanChangeNet()
                        && item->Parent()->GetNetCode() != cluster->OriginNet() )
                {
                    MarkNetAsDirty( item->Parent()->GetNetCode() );
                    MarkNetAsDirty( cluster->OriginNet() );

                    if( aCommit )
                        aCommit->Modify( item->Parent() );

                    item->Parent()->SetNetCode( cluster->OriginNet() );
                    n_changed++;
                }
            }

            if( n_changed )
            {
                wxLogTrace( wxT( "CN" ), wxT( "Cluster %p: net: %d %s" ),
                            cluster.get(),
                            cluster->OriginNet(),
                            (const char*) cluster->OriginNetName().c_str() );
            }
            else
            {
                wxLogTrace( wxT( "CN" ), wxT( "Cluster %p: no changeable items to propagate to" ),
                            cluster.get() );
            }
        }
        else
        {
            wxLogTrace( wxT( "CN" ), wxT( "Cluster %p: connected to unused net" ),
                        cluster.get() );
        }
    }
}


void CN_CONNECTIVITY_ALGO::PropagateNets( BOARD_COMMIT* aCommit )
{
    m_connClusters = SearchClusters( CSM_PROPAGATE );
    propagateConnections( aCommit );
}


void CN_CONNECTIVITY_ALGO::FillIsolatedIslandsMap(
                                std::map<ZONE*, std::map<PCB_LAYER_ID, ISOLATED_ISLANDS>>& aMap,
                                bool aConnectivityAlreadyRebuilt )
{
    int progressDelta = 50;
    int ii = 0;

    progressDelta = std::max( progressDelta, (int) aMap.size() / 4 );

    if( !aConnectivityAlreadyRebuilt )
    {
        for( const auto& [ zone, islands ] : aMap )
        {
            Remove( zone );
            Add( zone );
            ii++;

            if( m_progressReporter && ( ii % progressDelta ) == 0 )
            {
                m_progressReporter->SetCurrentProgress( (double) ii / (double) aMap.size() );
                m_progressReporter->KeepRefreshing( false );
            }

            if( m_progressReporter && m_progressReporter->IsCancelled() )
                return;
        }
    }

    m_connClusters = SearchClusters( CSM_CONNECTIVITY_CHECK );

    for( auto& [ zone, zoneIslands ] : aMap )
    {
        for( auto& [ layer, layerIslands ] : zoneIslands )
        {
            if( zone->GetFilledPolysList( layer )->IsEmpty() )
                continue;

            for( const std::shared_ptr<CN_CLUSTER>& cluster : m_connClusters )
            {
                for( CN_ITEM* item : *cluster )
                {
                    if( item->Parent() == zone && item->Layer() == layer )
                    {
                        CN_ZONE_LAYER* z = static_cast<CN_ZONE_LAYER*>( item );

                        if( cluster->IsOrphaned() )
                            layerIslands.m_IsolatedOutlines.push_back( z->SubpolyIndex() );
                        else if( z->HasSingleConnection() )
                            layerIslands.m_SingleConnectionOutlines.push_back( z->SubpolyIndex() );
                    }
                }
            }
        }
    }
}


const CN_CONNECTIVITY_ALGO::CLUSTERS& CN_CONNECTIVITY_ALGO::GetClusters()
{
    m_ratsnestClusters = SearchClusters( CSM_RATSNEST );
    return m_ratsnestClusters;
}


void CN_CONNECTIVITY_ALGO::MarkNetAsDirty( int aNet )
{
    if( aNet < 0 )
        return;

    if( (int) m_dirtyNets.size() <= aNet )
    {
        int lastNet = m_dirtyNets.size() - 1;

        if( lastNet < 0 )
            lastNet = 0;

        m_dirtyNets.resize( aNet + 1 );

        for( int i = lastNet; i < aNet + 1; i++ )
            m_dirtyNets[i] = true;
    }

    m_dirtyNets[aNet] = true;
}


void CN_VISITOR::checkZoneItemConnection( CN_ZONE_LAYER* aZoneLayer, CN_ITEM* aItem )
{
    PCB_LAYER_ID          layer = aZoneLayer->GetLayer();
    BOARD_CONNECTED_ITEM* item = aItem->Parent();

    if( !item->IsOnLayer( layer ) )
        return;

    auto connect =
            [&]()
            {
                aZoneLayer->Connect( aItem );
                aItem->Connect( aZoneLayer );
            };

    // Try quick checks first...
    if( item->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( item );

        if( pad->ConditionallyFlashed( layer )
            && pad->GetZoneLayerOverride( layer ) == ZLO_FORCE_NO_ZONE_CONNECTION )
        {
            return;
        }
    }
    else if( item->Type() == PCB_VIA_T )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( item );

        if( via->ConditionallyFlashed( layer )
            && via->GetZoneLayerOverride( layer ) == ZLO_FORCE_NO_ZONE_CONNECTION )
        {
            return;
        }
    }

    for( int i = 0; i < aItem->AnchorCount(); ++i )
    {
        if( aZoneLayer->ContainsPoint( aItem->GetAnchor( i ) ) )
        {
            connect();
            return;
        }
    }

    if( item->Type() == PCB_VIA_T || item->Type() == PCB_PAD_T )
    {
        // As long as the pad/via crosses the zone layer, check for the full effective shape
        // We check for the overlapping layers above
        if( aZoneLayer->Collide( item->GetEffectiveShape( layer, FLASHING::ALWAYS_FLASHED ).get() ) )
            connect();

        return;
    }

    if( aZoneLayer->Collide( item->GetEffectiveShape( layer ).get() ) )
        connect();
}

void CN_VISITOR::checkZoneZoneConnection( CN_ZONE_LAYER* aZoneLayerA, CN_ZONE_LAYER* aZoneLayerB )
{
    const ZONE* zoneA = static_cast<const ZONE*>( aZoneLayerA->Parent() );
    const ZONE* zoneB = static_cast<const ZONE*>( aZoneLayerB->Parent() );

    const BOX2I& boxA = aZoneLayerA->BBox();
    const BOX2I& boxB = aZoneLayerB->BBox();

    PCB_LAYER_ID layer = aZoneLayerA->GetLayer();

    if( aZoneLayerB->GetLayer() != layer )
        return;

    if( !boxA.Intersects( boxB ) )
        return;

    const SHAPE_LINE_CHAIN& outline =
            zoneA->GetFilledPolysList( layer )->COutline( aZoneLayerA->SubpolyIndex() );

    for( int i = 0; i < outline.PointCount(); i++ )
    {
        if( !boxB.Contains( outline.CPoint( i ) ) )
            continue;

        if( aZoneLayerB->ContainsPoint( outline.CPoint( i ) ) )
        {
            aZoneLayerA->Connect( aZoneLayerB );
            aZoneLayerB->Connect( aZoneLayerA );
            return;
        }
    }

    const SHAPE_LINE_CHAIN& outline2 =
            zoneB->GetFilledPolysList( layer )->COutline( aZoneLayerB->SubpolyIndex() );

    for( int i = 0; i < outline2.PointCount(); i++ )
    {
        if( !boxA.Contains( outline2.CPoint( i ) ) )
            continue;

        if( aZoneLayerA->ContainsPoint( outline2.CPoint( i ) ) )
        {
            aZoneLayerA->Connect( aZoneLayerB );
            aZoneLayerB->Connect( aZoneLayerA );
            return;
        }
    }
}


bool CN_VISITOR::operator()( CN_ITEM* aCandidate )
{
    const BOARD_CONNECTED_ITEM* parentA = aCandidate->Parent();
    const BOARD_CONNECTED_ITEM* parentB = m_item->Parent();

    if( !aCandidate->Valid() || !m_item->Valid() )
        return true;

    if( parentA == parentB )
        return true;

    // Don't connect items in different nets that can't be changed
    if( !aCandidate->CanChangeNet() && !m_item->CanChangeNet() && aCandidate->Net() != m_item->Net() )
        return true;

    // If both m_item and aCandidate are marked dirty, they will both be searched
    // Since we are reciprocal in our connection, we arbitrarily pick one of the connections
    // to conduct the expensive search
    if( aCandidate->Dirty() && aCandidate < m_item )
        return true;

    // We should handle zone-zone connection separately
    if ( parentA->Type() == PCB_ZONE_T && parentB->Type() == PCB_ZONE_T )
    {
        checkZoneZoneConnection( static_cast<CN_ZONE_LAYER*>( m_item ),
                                 static_cast<CN_ZONE_LAYER*>( aCandidate ) );
        return true;
    }

    if( parentA->Type() == PCB_ZONE_T )
    {
        checkZoneItemConnection( static_cast<CN_ZONE_LAYER*>( aCandidate ), m_item );
        return true;
    }

    if( parentB->Type() == PCB_ZONE_T )
    {
        checkZoneItemConnection( static_cast<CN_ZONE_LAYER*>( m_item ), aCandidate );
        return true;
    }

    LSET commonLayers = parentA->GetLayerSet() & parentB->GetLayerSet();

    for( PCB_LAYER_ID layer : commonLayers.Seq() )
    {
        FLASHING flashingA = FLASHING::NEVER_FLASHED;
        FLASHING flashingB = FLASHING::NEVER_FLASHED;

        if( const PAD* pad = dyn_cast<const PAD*>( parentA ) )
        {
            if( !pad->ConditionallyFlashed( layer ) )
                flashingA = FLASHING::ALWAYS_FLASHED;
        }
        else if( const PCB_VIA* via = dyn_cast<const PCB_VIA*>( parentA ) )
        {
            if( !via->ConditionallyFlashed( layer ) )
                flashingA = FLASHING::ALWAYS_FLASHED;
        }

        if( const PAD* pad = dyn_cast<const PAD*>( parentB ) )
        {
            if( !pad->ConditionallyFlashed( layer ) )
                flashingB = FLASHING::ALWAYS_FLASHED;
        }
        else if( const PCB_VIA* via = dyn_cast<const PCB_VIA*>( parentB ) )
        {
            if( !via->ConditionallyFlashed( layer ) )
                flashingB = FLASHING::ALWAYS_FLASHED;
        }

        if( parentA->GetEffectiveShape( layer, flashingA )->Collide(
                parentB->GetEffectiveShape( layer, flashingB ).get() ) )
        {
            m_item->Connect( aCandidate );
            aCandidate->Connect( m_item );
            return true;
        }
    }

    return true;
};


void CN_CONNECTIVITY_ALGO::Clear()
{
    m_ratsnestClusters.clear();
    m_connClusters.clear();
    m_itemMap.clear();
    m_itemList.Clear();

}

void CN_CONNECTIVITY_ALGO::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
}
