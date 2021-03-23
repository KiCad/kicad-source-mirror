/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016-2018 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
        auto citem = static_cast<const BOARD_CONNECTED_ITEM*>( aItem );
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

    markItemNetAsDirty ( aItem );

    switch( aItem->Type() )
    {
    case PCB_NETINFO_T:
        MarkNetAsDirty( static_cast<NETINFO_ITEM*>( aItem )->GetNetCode() );
        break;

    case PCB_FOOTPRINT_T:
        for( PAD* pad : static_cast<FOOTPRINT*>( aItem )->Pads() )
        {
            if( m_itemMap.find( pad ) != m_itemMap.end() )
                return false;

            add( m_itemList, pad );
        }

        break;

    case PCB_PAD_T:
        if( m_itemMap.find ( aItem ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<PAD*>( aItem ) );
        break;

    case PCB_TRACE_T:
        if( m_itemMap.find( aItem ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<TRACK*>( aItem ) );
        break;

    case PCB_ARC_T:
        if( m_itemMap.find( aItem ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<ARC*>( aItem ) );
        break;

    case PCB_VIA_T:
        if( m_itemMap.find( aItem ) != m_itemMap.end() )
            return false;

        add( m_itemList, static_cast<VIA*>( aItem ) );
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

    return true;
}


void CN_CONNECTIVITY_ALGO::searchConnections()
{
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
        size_t parallelThreadCount = std::min<size_t>( std::thread::hardware_concurrency(),
                                                       ( dirtyItems.size() + 7 ) / 8 );

        std::atomic<size_t> nextItem( 0 );
        std::vector<std::future<size_t>> returns( parallelThreadCount );

        auto conn_lambda =
                [&nextItem, &dirtyItems]( CN_LIST* aItemList,
                                          PROGRESS_REPORTER* aReporter) -> size_t
                {
                    for( size_t i = nextItem++; i < dirtyItems.size(); i = nextItem++ )
                    {
                        CN_VISITOR visitor( dirtyItems[i] );
                        aItemList->FindNearby( dirtyItems[i], visitor );

                        if( aReporter )
                        {
                            if( aReporter->IsCancelled() )
                                break;
                            else
                                aReporter->AdvanceProgress();
                        }
                    }

                    return 1;
                };

        if( parallelThreadCount <= 1 )
            conn_lambda( &m_itemList, m_progressReporter );
        else
        {
            for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            {
                returns[ii] = std::async( std::launch::async, conn_lambda, &m_itemList,
                                          m_progressReporter );
            }

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
}


const CN_CONNECTIVITY_ALGO::CLUSTERS CN_CONNECTIVITY_ALGO::SearchClusters( CLUSTER_SEARCH_MODE aMode )
{
    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_ARC_T, PCB_PAD_T, PCB_VIA_T, PCB_ZONE_T,
                                  PCB_FOOTPRINT_T, EOT };
    constexpr KICAD_T no_zones[] = { PCB_TRACE_T, PCB_ARC_T, PCB_PAD_T, PCB_VIA_T,
                                     PCB_FOOTPRINT_T, EOT };

    if( aMode == CSM_PROPAGATE )
        return SearchClusters( aMode, no_zones, -1 );
    else
        return SearchClusters( aMode, types, -1 );
}


const CN_CONNECTIVITY_ALGO::CLUSTERS CN_CONNECTIVITY_ALGO::SearchClusters( CLUSTER_SEARCH_MODE aMode,
                                                                           const KICAD_T aTypes[],
                                                                           int aSingleNet )
{
    bool withinAnyNet = ( aMode != CSM_PROPAGATE );

    std::deque<CN_ITEM*> Q;
    std::set<CN_ITEM*> item_set;

    CLUSTERS clusters;

    if( m_itemList.IsDirty() )
        searchConnections();

    auto addToSearchList =
            [&item_set, withinAnyNet, aSingleNet, aTypes]( CN_ITEM *aItem )
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

                aItem->SetVisited( false );

                item_set.insert( aItem );
            };

    std::for_each( m_itemList.begin(), m_itemList.end(), addToSearchList );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return CLUSTERS();

    while( !item_set.empty() )
    {
        CN_CLUSTER_PTR cluster = std::make_shared<CN_CLUSTER>();
        CN_ITEM*       root;
        auto           it = item_set.begin();

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

            for( auto n : current->ConnectedItems() )
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
               []( CN_CLUSTER_PTR a, CN_CLUSTER_PTR b )
               {
                   return a->OriginNet() < b->OriginNet();
               } );

    return clusters;
}


void reportProgress( PROGRESS_REPORTER* aReporter, int aCount, int aSize, int aDelta )
{
    if( aReporter && ( ( aCount % aDelta ) == 0 || aCount == aSize -  1 ) )
    {
        aReporter->SetCurrentProgress( (double) aCount / (double) aSize );
        aReporter->KeepRefreshing( false );
    }
}


void CN_CONNECTIVITY_ALGO::Build( BOARD* aBoard, PROGRESS_REPORTER* aReporter )
{
    const int delta = 100;  // Number of additions between 2 calls to the progress bar
    int ii = 0;
    int size = 0;

    size += aBoard->Zones().size();
    size += aBoard->Tracks().size();

    for( FOOTPRINT* footprint : aBoard->Footprints() )
        size += footprint->Pads().size();

    size *= 2;      // Our caller us gets the other half of the progress bar

    for( ZONE* zone : aBoard->Zones() )
    {
        Add( zone );
        reportProgress( aReporter, ii++, size, delta );
    }

    for( TRACK* tv : aBoard->Tracks() )
    {
        Add( tv );
        reportProgress( aReporter, ii++, size, delta );
    }

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            Add( pad );
            reportProgress( aReporter, ii++, size, delta );
        }
    }
}


void CN_CONNECTIVITY_ALGO::Build( const std::vector<BOARD_ITEM*>& aItems )
{
    for( auto item : aItems )
    {
        switch( item->Type() )
        {
            case PCB_TRACE_T:
            case PCB_ARC_T:
            case PCB_VIA_T:
            case PCB_PAD_T:
                Add( item );
                break;

            case PCB_FOOTPRINT_T:
                for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                    Add( pad );

                break;

            default:
                break;
        }
    }
}


void CN_CONNECTIVITY_ALGO::propagateConnections( BOARD_COMMIT* aCommit, PROPAGATE_MODE aMode )
{
    bool skipConflicts = ( aMode == PROPAGATE_MODE::SKIP_CONFLICTS );

    wxLogTrace( "CN", "propagateConnections: propagate skip conflicts? %d", skipConflicts );

    for( const auto& cluster : m_connClusters )
    {
        if( skipConflicts && cluster->IsConflicting() )
        {
            wxLogTrace( "CN", "Conflicting nets in cluster %p; skipping update", cluster.get() );
        }
        else if( cluster->IsOrphaned() )
        {
            wxLogTrace( "CN", "Skipping orphaned cluster %p [net: %s]", cluster.get(),
                    (const char*) cluster->OriginNetName().c_str() );
        }
        else if( cluster->HasValidNet() )
        {
            if( cluster->IsConflicting() )
            {
                wxLogTrace( "CN", "Conflicting nets in cluster %p; chose %d (%s)", cluster.get(),
                            cluster->OriginNet(), cluster->OriginNetName() );
            }

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
            {
                wxLogTrace( "CN", "Cluster %p : net : %d %s", cluster.get(),
                        cluster->OriginNet(), (const char*) cluster->OriginNetName().c_str() );
            }
            else
                wxLogTrace( "CN", "Cluster %p : nothing to propagate", cluster.get() );
        }
        else
        {
            wxLogTrace( "CN", "Cluster %p : connected to unused net", cluster.get() );
        }
    }
}


void CN_CONNECTIVITY_ALGO::PropagateNets( BOARD_COMMIT* aCommit, PROPAGATE_MODE aMode )
{
    m_connClusters = SearchClusters( CSM_PROPAGATE );
    propagateConnections( aCommit, aMode );
}


void CN_CONNECTIVITY_ALGO::FindIsolatedCopperIslands( ZONE* aZone, PCB_LAYER_ID aLayer,
                                                      std::vector<int>& aIslands )
{
    if( aZone->GetFilledPolysList( aLayer ).IsEmpty() )
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
                if( z->Parent() == aZone && z->Layer() == aLayer )
                {
                    aIslands.push_back( static_cast<CN_ZONE_LAYER*>(z)->SubpolyIndex() );
                }
            }
        }
    }

    wxLogTrace( "CN", "Found %u isolated islands\n", (unsigned)aIslands.size() );
}

void CN_CONNECTIVITY_ALGO::FindIsolatedCopperIslands( std::vector<CN_ZONE_ISOLATED_ISLAND_LIST>& aZones )
{
    for( auto& z : aZones )
    {
        Remove( z.m_zone );
        Add( z.m_zone );
    }

    m_connClusters = SearchClusters( CSM_CONNECTIVITY_CHECK );

    for( CN_ZONE_ISOLATED_ISLAND_LIST& zone : aZones )
    {
        for( PCB_LAYER_ID layer : zone.m_zone->GetLayerSet().Seq() )
        {
            if( zone.m_zone->GetFilledPolysList( layer ).IsEmpty() )
                continue;

            for( const CN_CLUSTER_PTR& cluster : m_connClusters )
            {
                if( cluster->Contains( zone.m_zone ) && cluster->IsOrphaned() )
                {
                    for( CN_ITEM* z : *cluster )
                    {
                        if( z->Parent() == zone.m_zone && z->Layer() == layer )
                        {
                            zone.m_islands[layer].push_back(
                                    static_cast<CN_ZONE_LAYER*>( z )->SubpolyIndex() );
                        }
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
    if( aZoneLayer->Net() != aItem->Net() && !aItem->CanChangeNet() )
        return;

    if( !aZoneLayer->BBox().Intersects( aItem->BBox() ) )
        return;

    int accuracy = 0;

    if( aItem->Parent()->Type() == PCB_VIA_T
            || aItem->Parent()->Type() == PCB_TRACE_T
            || aItem->Parent()->Type() == PCB_ARC_T )
    {
        accuracy = ( static_cast<TRACK*>( aItem->Parent() )->GetWidth() + 1 ) / 2;
    }

    for( int i = 0; i < aItem->AnchorCount(); ++i )
    {
        if( aZoneLayer->ContainsPoint( aItem->GetAnchor( i ), accuracy ) )
        {
            aZoneLayer->Connect( aItem );
            aItem->Connect( aZoneLayer );
            return;
        }
    }
}

void CN_VISITOR::checkZoneZoneConnection( CN_ZONE_LAYER* aZoneLayerA, CN_ZONE_LAYER* aZoneLayerB )
{
    const ZONE* zoneA = static_cast<const ZONE*>( aZoneLayerA->Parent() );
    const ZONE* zoneB = static_cast<const ZONE*>( aZoneLayerB->Parent() );

    if( aZoneLayerA->Layer() != aZoneLayerB->Layer() )
        return;

    if( aZoneLayerB->Net() != aZoneLayerA->Net() )
        return; // we only test zones belonging to the same net

    const BOX2I& boxA = aZoneLayerA->BBox();
    const BOX2I& boxB = aZoneLayerB->BBox();

    int radiusA = 0;
    int radiusB = 0;

    if( zoneA->GetFilledPolysUseThickness() )
        radiusA = ( zoneA->GetMinThickness() + 1 ) / 2;

    if( zoneB->GetFilledPolysUseThickness() )
        radiusB = ( zoneB->GetMinThickness() + 1 ) / 2;

    PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( aZoneLayerA->Layer() );

    const SHAPE_LINE_CHAIN& outline =
            zoneA->GetFilledPolysList( layer ).COutline( aZoneLayerA->SubpolyIndex() );

    for( int i = 0; i < outline.PointCount(); i++ )
    {
        if( !boxB.Contains( outline.CPoint( i ) ) )
            continue;

        if( aZoneLayerB->ContainsPoint( outline.CPoint( i ), radiusA ) )
        {
            aZoneLayerA->Connect( aZoneLayerB );
            aZoneLayerB->Connect( aZoneLayerA );
            return;
        }
    }

    const SHAPE_LINE_CHAIN& outline2 =
            zoneB->GetFilledPolysList( layer ).COutline( aZoneLayerB->SubpolyIndex() );

    for( int i = 0; i < outline2.PointCount(); i++ )
    {
        if( !boxA.Contains( outline2.CPoint( i ) ) )
            continue;

        if( aZoneLayerA->ContainsPoint( outline2.CPoint( i ), radiusB ) )
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

    if( !( parentA->GetLayerSet() & parentB->GetLayerSet() ).any() )
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

    int accuracyA = 0;
    int accuracyB = 0;

    if( parentA->Type() == PCB_VIA_T
            || parentA->Type() == PCB_TRACE_T
            || parentA->Type() == PCB_ARC_T)
        accuracyA = ( static_cast<const TRACK*>( parentA )->GetWidth() + 1 ) / 2;

    if( parentB->Type() == PCB_VIA_T
            || parentB->Type() == PCB_TRACE_T
            || parentB->Type() == PCB_ARC_T )
        accuracyB = ( static_cast<const TRACK*>( parentB )->GetWidth() + 1 ) / 2;

    // Items do not necessarily have reciprocity as we only check for anchors
    //  therefore, we check HitTest both directions A->B & B->A
    for( int i = 0; i < aCandidate->AnchorCount(); ++i )
    {
        if( parentB->HitTest( wxPoint( aCandidate->GetAnchor( i ) ), accuracyA ) )
        {
            m_item->Connect( aCandidate );
            aCandidate->Connect( m_item );
            return true;
        }
    }

    for( int i = 0; i < m_item->AnchorCount(); ++i )
    {
        if( parentA->HitTest( wxPoint( m_item->GetAnchor( i ) ), accuracyB ) )
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
