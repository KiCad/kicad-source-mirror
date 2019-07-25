/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016-2018 CERN
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

#include <connectivity/connectivity_algo.h>
#include <widgets/progress_reporter.h>
#include <geometry/geometry_utils.h>
#include <board_commit.h>

#include <thread>
#include <mutex>
#include <algorithm>
#include <future>

#ifdef PROFILE
#include <profile.h>
#endif


bool CN_CONNECTIVITY_ALGO::Remove( BOARD_ITEM* aItem )
{
    markItemNetAsDirty( aItem );

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
        for( auto pad : static_cast<MODULE*>( aItem ) -> Pads() )
        {
            m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( pad ) ].MarkItemsAsInvalid();
            m_itemMap.erase( static_cast<BOARD_CONNECTED_ITEM*>( pad ) );
        }

        m_itemList.SetDirty( true );
        break;

    case PCB_PAD_T:
        m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( aItem ) ].MarkItemsAsInvalid();
        m_itemMap.erase( static_cast<BOARD_CONNECTED_ITEM*>( aItem ) );
        m_itemList.SetDirty( true );
        break;

    case PCB_TRACE_T:
        m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( aItem ) ].MarkItemsAsInvalid();
        m_itemMap.erase( static_cast<BOARD_CONNECTED_ITEM*>( aItem ) );
        m_itemList.SetDirty( true );
        break;

    case PCB_VIA_T:
        m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( aItem ) ].MarkItemsAsInvalid();
        m_itemMap.erase( static_cast<BOARD_CONNECTED_ITEM*>( aItem ) );
        m_itemList.SetDirty( true );
        break;

    case PCB_ZONE_AREA_T:
    {
        m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( aItem ) ].MarkItemsAsInvalid();
        m_itemMap.erase ( static_cast<BOARD_CONNECTED_ITEM*>( aItem ) );
        m_itemList.SetDirty( true );
        break;
    }

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
        auto citem = static_cast<const BOARD_CONNECTED_ITEM*>( aItem );
        MarkNetAsDirty( citem->GetNetCode() );
    }
    else
    {
        if( aItem->Type() == PCB_MODULE_T )
        {
            auto mod = static_cast <const MODULE*>( aItem );

            for( auto pad : mod->Pads() )
                MarkNetAsDirty( pad->GetNetCode() );
        }
    }
}


bool CN_CONNECTIVITY_ALGO::Add( BOARD_ITEM* aItem )
{
    if( !aItem->IsOnCopperLayer() )
        return false;

    markItemNetAsDirty ( aItem );

    switch( aItem->Type() )
    {
    case PCB_NETINFO_T:
        {
            MarkNetAsDirty( static_cast<NETINFO_ITEM*>( aItem )->GetNet() );
            break;
        }
    case PCB_MODULE_T:
        for( auto pad : static_cast<MODULE*>( aItem ) -> Pads() )
        {
            if( m_itemMap.find( pad ) != m_itemMap.end() )
                return false;

            add( m_itemList, pad );
        }

        break;

    case PCB_PAD_T:
        if( m_itemMap.find ( static_cast<D_PAD*>( aItem ) ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<D_PAD*>( aItem ) );

        break;

    case PCB_TRACE_T:
    {
        if( m_itemMap.find( static_cast<TRACK*>( aItem ) ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<TRACK*>( aItem ) );

        break;
    }

    case PCB_VIA_T:
        if( m_itemMap.find( static_cast<VIA*>( aItem ) ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<VIA*>( aItem ) );

        break;

    case PCB_ZONE_AREA_T:
    {
        auto zone = static_cast<ZONE_CONTAINER*>( aItem );

        if( m_itemMap.find( static_cast<ZONE_CONTAINER*>( aItem ) ) != m_itemMap.end() )
            return false;

        m_itemMap[zone] = ITEM_MAP_ENTRY();

        for( auto zitem : m_itemList.Add( zone ) )
            m_itemMap[zone].Link(zitem);

        break;
    }

    default:
        return false;
    }

    return true;
}


void CN_CONNECTIVITY_ALGO::searchConnections()
{
#ifdef CONNECTIVITY_DEBUG
    printf("Search start\n");
#endif

#ifdef PROFILE
    PROF_COUNTER garbage_collection( "garbage-collection" );
#endif
    std::vector<CN_ITEM*> garbage;
    garbage.reserve( 1024 );

    m_itemList.RemoveInvalidItems( garbage );

    for( auto item : garbage )
        delete item;

#ifdef PROFILE
    garbage_collection.Show();
    PROF_COUNTER search_basic( "search-basic" );
#endif

    std::vector<CN_ITEM*> dirtyItems;
    std::copy_if( m_itemList.begin(), m_itemList.end(), std::back_inserter( dirtyItems ),
            [] ( CN_ITEM* aItem ) { return aItem->Dirty(); } );

    if( m_progressReporter )
    {
        m_progressReporter->SetMaxProgress( dirtyItems.size() );
        m_progressReporter->KeepRefreshing();
    }

    if( m_itemList.IsDirty() )
    {
        size_t parallelThreadCount = std::min<size_t>( std::thread::hardware_concurrency(),
                ( dirtyItems.size() + 7 ) / 8 );

        std::atomic<size_t> nextItem( 0 );
        std::vector<std::future<size_t>> returns( parallelThreadCount );

        auto conn_lambda = [&nextItem, &dirtyItems]
                            ( CN_LIST* aItemList, PROGRESS_REPORTER* aReporter) -> size_t
        {
            for( size_t i = nextItem++; i < dirtyItems.size(); i = nextItem++ )
            {
                CN_VISITOR visitor( dirtyItems[i] );
                aItemList->FindNearby( dirtyItems[i], visitor );

                if( aReporter )
                    aReporter->AdvanceProgress();
            }

            return 1;
        };

        if( parallelThreadCount <= 1 )
            conn_lambda( &m_itemList, m_progressReporter );
        else
        {
            for( size_t ii = 0; ii < parallelThreadCount; ++ii )
                returns[ii] = std::async( std::launch::async, conn_lambda,
                        &m_itemList, m_progressReporter );

            for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            {
                // Here we balance returns with a 100ms timeout to allow UI updating
                std::future_status status;
                do
                {
                    if( m_progressReporter )
                        m_progressReporter->KeepRefreshing();

                    status = returns[ii].wait_for( std::chrono::milliseconds( 100 ) );
                } while( status != std::future_status::ready );
            }
        }

        if( m_progressReporter )
            m_progressReporter->KeepRefreshing();
    }

#ifdef PROFILE
        search_basic.Show();
#endif

    m_itemList.ClearDirtyFlags();

#ifdef CONNECTIVITY_DEBUG
    printf("Search end\n");
#endif

}


const CN_CONNECTIVITY_ALGO::CLUSTERS CN_CONNECTIVITY_ALGO::SearchClusters( CLUSTER_SEARCH_MODE aMode )
{
    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_PAD_T, PCB_VIA_T, PCB_ZONE_AREA_T, PCB_MODULE_T, EOT };
    constexpr KICAD_T no_zones[] = { PCB_TRACE_T, PCB_PAD_T, PCB_VIA_T, PCB_MODULE_T, EOT };

    if( aMode == CSM_PROPAGATE )
        return SearchClusters( aMode, no_zones, -1 );
    else
        return SearchClusters( aMode, types, -1 );
}


const CN_CONNECTIVITY_ALGO::CLUSTERS CN_CONNECTIVITY_ALGO::SearchClusters( CLUSTER_SEARCH_MODE aMode,
        const KICAD_T aTypes[], int aSingleNet )
{
    bool withinAnyNet = ( aMode != CSM_PROPAGATE );

    std::deque<CN_ITEM*> Q;
    CN_ITEM* head = nullptr;
    CLUSTERS clusters;

    if( m_itemList.IsDirty() )
        searchConnections();

    auto addToSearchList = [&head, withinAnyNet, aSingleNet, aTypes] ( CN_ITEM *aItem )
    {
        if( withinAnyNet && aItem->Net() <= 0 )
            return;

        if( !aItem->Valid() )
            return;

        if( aSingleNet >=0 && aItem->Net() != aSingleNet )
            return;

        bool found = false;

        for( int i = 0; aTypes[i] != EOT; i++ )
        {
            if( aItem->Parent()->Type() == aTypes[i] )
            {
                found = true;
                break;
            }
        }

        if( !found )
            return;

        aItem->ListClear();
        aItem->SetVisited( false );

        if( !head )
            head = aItem;
        else
            head->ListInsert( aItem );
    };

    std::for_each( m_itemList.begin(), m_itemList.end(), addToSearchList );

    while( head )
    {
        CN_CLUSTER_PTR cluster ( new CN_CLUSTER() );

        Q.clear();
        CN_ITEM* root = head;
        root->SetVisited ( true );

        head = root->ListRemove();

        Q.push_back( root );

        while( Q.size() )
        {
            CN_ITEM* current = Q.front();

            Q.pop_front();
            cluster->Add( current );

            for( auto n : current->ConnectedItems() )
            {
                if( withinAnyNet && n->Net() != root->Net() )
                    continue;

                if( !n->Visited() && n->Valid() )
                {
                    n->SetVisited( true );
                    Q.push_back( n );
                    head = n->ListRemove();
                }
            }
        }

        clusters.push_back( cluster );
    }


    std::sort( clusters.begin(), clusters.end(), []( CN_CLUSTER_PTR a, CN_CLUSTER_PTR b ) {
        return a->OriginNet() < b->OriginNet();
    } );

#ifdef CONNECTIVITY_DEBUG
    printf("Active clusters: %d\n", clusters.size() );

    for( auto cl : clusters )
    {
        printf( "Net %d\n", cl->OriginNet() );
        cl->Dump();
    }
#endif

    return clusters;
}


void CN_CONNECTIVITY_ALGO::Build( BOARD* aBoard )
{
    for( int i = 0; i<aBoard->GetAreaCount(); i++ )
    {
        auto zone = aBoard->GetArea( i );
        Add( zone );
    }

    for( auto tv : aBoard->Tracks() )
        Add( tv );

    for( auto mod : aBoard->Modules() )
    {
        for( auto pad : mod->Pads() )
            Add( pad );
    }

    /*wxLogTrace( "CN", "zones : %lu, pads : %lu vias : %lu tracks : %lu\n",
            m_zoneList.Size(), m_padList.Size(),
            m_viaList.Size(), m_trackList.Size() );*/
}


void CN_CONNECTIVITY_ALGO::Build( const std::vector<BOARD_ITEM*>& aItems )
{
    for( auto item : aItems )
    {
        switch( item->Type() )
        {
            case PCB_TRACE_T:
            case PCB_VIA_T:
            case PCB_PAD_T:
                Add( item );
                break;

            case PCB_MODULE_T:
            {
                for( auto pad : static_cast<MODULE*>( item )->Pads() )
                {
                    Add( pad );
                }

                break;
            }

            default:
                break;
        }
    }
}


void CN_CONNECTIVITY_ALGO::propagateConnections( BOARD_COMMIT* aCommit )
{
    for( const auto& cluster : m_connClusters )
    {
        if( cluster->IsConflicting() )
        {
            wxLogTrace( "CN", "Conflicting nets in cluster %p\n", cluster.get() );
        }
        else if( cluster->IsOrphaned() )
        {
            wxLogTrace( "CN", "Skipping orphaned cluster %p [net: %s]\n", cluster.get(),
                    (const char*) cluster->OriginNetName().c_str() );
        }
        else if( cluster->HasValidNet() )
        {
            // normal cluster: just propagate from the pads
            int n_changed = 0;

            for( auto item : *cluster )
            {
                if( item->CanChangeNet() )
                {
                    if( item->Valid() && item->Parent()->GetNetCode() != cluster->OriginNet() )
                    {
                        MarkNetAsDirty( item->Parent()->GetNetCode() );
                        MarkNetAsDirty( cluster->OriginNet() );

                        if( aCommit )
                            aCommit->Modify( item->Parent() );

                        item->Parent()->SetNetCode( cluster->OriginNet() );
                        n_changed++;
                    }
                }
            }

            if( n_changed )
                wxLogTrace( "CN", "Cluster %p : net : %d %s\n", cluster.get(),
                        cluster->OriginNet(), (const char*) cluster->OriginNetName().c_str() );
            else
                wxLogTrace( "CN", "Cluster %p : nothing to propagate\n", cluster.get() );
        }
        else
        {
            wxLogTrace( "CN", "Cluster %p : connected to unused net\n", cluster.get() );
        }
    }
}


void CN_CONNECTIVITY_ALGO::PropagateNets( BOARD_COMMIT* aCommit )
{
    m_connClusters = SearchClusters( CSM_PROPAGATE );
    propagateConnections( aCommit );
}


void CN_CONNECTIVITY_ALGO::FindIsolatedCopperIslands( ZONE_CONTAINER* aZone, std::vector<int>& aIslands )
{
    if( aZone->GetFilledPolysList().IsEmpty() )
        return;

    aIslands.clear();

    Remove( aZone );
    Add( aZone );

    m_connClusters = SearchClusters( CSM_CONNECTIVITY_CHECK );

    for( const auto& cluster : m_connClusters )
    {
        if( cluster->Contains( aZone ) && cluster->IsOrphaned() )
        {
            for( auto z : *cluster )
            {
                if( z->Parent() == aZone )
                {
                    aIslands.push_back( static_cast<CN_ZONE*>(z)->SubpolyIndex() );
                }
            }
        }
    }

    wxLogTrace( "CN", "Found %u isolated islands\n", (unsigned)aIslands.size() );
}

void CN_CONNECTIVITY_ALGO::FindIsolatedCopperIslands( std::vector<CN_ZONE_ISOLATED_ISLAND_LIST>& aZones )
{
    for ( auto& z : aZones )
        Remove( z.m_zone );

    for ( auto& z : aZones )
    {
        if( !z.m_zone->GetFilledPolysList().IsEmpty() )
            Add( z.m_zone );
    }

    m_connClusters = SearchClusters( CSM_CONNECTIVITY_CHECK );

    for ( auto& zone : aZones )
    {
        if( zone.m_zone->GetFilledPolysList().IsEmpty() )
            continue;

        for( const auto& cluster : m_connClusters )
        {
            if( cluster->Contains( zone.m_zone ) && cluster->IsOrphaned() )
            {
                for( auto z : *cluster )
                {
                    if( z->Parent() == zone.m_zone )
                    {
                        zone.m_islands.push_back( static_cast<CN_ZONE*>(z)->SubpolyIndex() );
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


void CN_VISITOR::checkZoneItemConnection( CN_ZONE* aZone, CN_ITEM* aItem )
{
    auto zoneItem = static_cast<CN_ZONE*> ( aZone );

    if( zoneItem->Net() != aItem->Net() && !aItem->CanChangeNet() )
        return;

    if( zoneItem->ContainsPoint( aItem->GetAnchor( 0 ) ) ||
            ( aItem->Parent()->Type() == PCB_TRACE_T &&
              zoneItem->ContainsPoint( aItem->GetAnchor( 1 ) ) ) )
    {
        zoneItem->Connect( aItem );
        aItem->Connect( zoneItem );
    }
}

void CN_VISITOR::checkZoneZoneConnection( CN_ZONE* aZoneA, CN_ZONE* aZoneB )
{
    const auto refParent = static_cast<const ZONE_CONTAINER*>( aZoneA->Parent() );
    const auto testedParent = static_cast<const ZONE_CONTAINER*>( aZoneB->Parent() );

    if( testedParent->Type () != PCB_ZONE_AREA_T )
        return;

    if( aZoneB == aZoneA  || refParent == testedParent )
        return;

    if( aZoneB->Net() != aZoneA->Net() )
        return; // we only test zones belonging to the same net

    const auto& outline = refParent->GetFilledPolysList().COutline( aZoneA->SubpolyIndex() );

    for( int i = 0; i < outline.PointCount(); i++ )
    {
        if( aZoneB->ContainsPoint( outline.CPoint( i ) ) )
        {
            aZoneA->Connect( aZoneB );
            aZoneB->Connect( aZoneA );
            return;
        }
    }

    const auto& outline2 = testedParent->GetFilledPolysList().COutline( aZoneB->SubpolyIndex() );

    for( int i = 0; i < outline2.PointCount(); i++ )
    {
        if( aZoneA->ContainsPoint( outline2.CPoint( i ) ) )
        {
            aZoneA->Connect( aZoneB );
            aZoneB->Connect( aZoneA );
            return;
        }
    }
}


bool CN_VISITOR::operator()( CN_ITEM* aCandidate )
{
    const auto parentA = aCandidate->Parent();
    const auto parentB = m_item->Parent();

    if( !aCandidate->Valid() || !m_item->Valid() )
        return true;

    if( parentA == parentB )
        return true;

    if( !( parentA->GetLayerSet() & parentB->GetLayerSet() ).any() )
        return true;

    // If both m_item and aCandidate are marked dirty, they will both be searched
    // Since we are reciprocal in our connection, we arbitrarily pick one of the connections
    // to conduct the expensive search
    if( aCandidate->Dirty() && aCandidate < m_item )
        return true;

    // We should handle zone-zone connection separately
    if ( parentA->Type() == PCB_ZONE_AREA_T && parentB->Type() == PCB_ZONE_AREA_T )
    {
        checkZoneZoneConnection( static_cast<CN_ZONE*>( m_item ),
                static_cast<CN_ZONE*>( aCandidate ) );
        return true;
    }

    if( parentA->Type() == PCB_ZONE_AREA_T )
    {
        checkZoneItemConnection( static_cast<CN_ZONE*>( aCandidate ), m_item );
        return true;
    }

    if( parentB->Type() == PCB_ZONE_AREA_T )
    {
        checkZoneItemConnection( static_cast<CN_ZONE*>( m_item ), aCandidate );
        return true;
    }

    // Items do not necessarily have reciprocity as we only check for anchors
    //  therefore, we check HitTest both directions A->B & B->A
    // TODO: Check for collision geometry on extended features
    wxPoint ptA1( aCandidate->GetAnchor( 0 ).x, aCandidate->GetAnchor( 0 ).y );
    wxPoint ptA2( aCandidate->GetAnchor( 1 ).x, aCandidate->GetAnchor( 1 ).y );
    wxPoint ptB1( m_item->GetAnchor( 0 ).x, m_item->GetAnchor( 0 ).y );
    wxPoint ptB2( m_item->GetAnchor( 1 ).x, m_item->GetAnchor( 1 ).y );
    if( parentA->HitTest( ptB1 ) || parentB->HitTest( ptA1 ) ||
            ( parentA->Type() == PCB_TRACE_T && parentB->HitTest( ptA2 ) ) ||
            ( parentB->Type() == PCB_TRACE_T && parentA->HitTest( ptB2 ) ) )
    {
        m_item->Connect( aCandidate );
        aCandidate->Connect( m_item );
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


void CN_CONNECTIVITY_ALGO::ForEachItem( const std::function<void( CN_ITEM& )>& aFunc )
{
    for( auto item : m_itemList )
        aFunc( *item );
}


void CN_CONNECTIVITY_ALGO::ForEachAnchor( const std::function<void( CN_ANCHOR& )>& aFunc )
{
    ForEachItem( [aFunc] ( CN_ITEM& item ) {
        for( const auto& anchor : item.Anchors() )
            aFunc( *anchor );
        }
    );
}


void CN_CONNECTIVITY_ALGO::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
}
