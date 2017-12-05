/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016-2017 CERN
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

#include <connectivity_algo.h>
#include <widgets/progress_reporter.h>

#include <thread>
#include <mutex>

#ifdef PROFILE
#include <profile.h>
#endif

using namespace std::placeholders;

bool operator<( const CN_ANCHOR_PTR a, const CN_ANCHOR_PTR b )
{
    if( a->Pos().x == b->Pos().x )
        return a->Pos().y < b->Pos().y;
    else
        return a->Pos().x < b->Pos().x;
}


bool CN_ANCHOR::IsDirty() const
{
    return m_item->Dirty();
}


CN_CLUSTER::CN_CLUSTER()
{
    m_items.reserve( 64 );
    m_originPad = nullptr;
    m_originNet = -1;
    m_conflicting = false;
}


CN_CLUSTER::~CN_CLUSTER()
{
}


wxString CN_CLUSTER::OriginNetName() const
{
    if( !m_originPad || !m_originPad->Valid() )
        return "<none>";
    else
        return m_originPad->Parent()->GetNetname();
}


bool CN_CLUSTER::Contains( const CN_ITEM* aItem )
{
    return std::find( m_items.begin(), m_items.end(), aItem ) != m_items.end();
}


bool CN_CLUSTER::Contains( const BOARD_CONNECTED_ITEM* aItem )
{
    for( auto item : m_items )
    {
        if( item->Valid() && item->Parent() == aItem )
            return true;
    }

    return false;
}


void CN_ITEM::Dump()
{
    printf("    valid: %d, connected: \n", !!Valid());

    for( auto i : m_connected )
    {
        TRACK* t = static_cast<TRACK*>( i->Parent() );
        printf( "    - %p %d\n", t, t->Type() );
    }
}


void CN_CLUSTER::Dump()
{
    for( auto item : m_items )
    {
        wxLogTrace( "CN", " - item : %p bitem : %p type : %d inet %s\n", item, item->Parent(),
                item->Parent()->Type(), (const char*) item->Parent()->GetNetname().c_str() );
        printf( "- item : %p bitem : %p type : %d inet %s\n", item, item->Parent(),
                        item->Parent()->Type(), (const char*) item->Parent()->GetNetname().c_str() );
        item->Dump();
    }
}


void CN_CLUSTER::Add( CN_ITEM* item )
{
    m_items.push_back( item );

    if( m_originNet < 0 )
    {
        m_originNet = item->Net();
    }

    if( item->Parent()->Type() == PCB_PAD_T )
    {
        if( !m_originPad )
        {
            m_originPad = item;
            m_originNet = item->Net();
        }

        if( m_originPad && item->Net() != m_originNet )
        {
            m_conflicting = true;
        }
    }
}


CN_CONNECTIVITY_ALGO::CN_CONNECTIVITY_ALGO()
{
}


CN_CONNECTIVITY_ALGO::~CN_CONNECTIVITY_ALGO()
{
    Clear();
}


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

        m_padList.SetDirty( true );
        break;

    case PCB_PAD_T:
        m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( aItem ) ].MarkItemsAsInvalid();
        m_itemMap.erase( static_cast<BOARD_CONNECTED_ITEM*>( aItem ) );
        m_padList.SetDirty( true );
        break;

    case PCB_TRACE_T:
        m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( aItem ) ].MarkItemsAsInvalid();
        m_itemMap.erase( static_cast<BOARD_CONNECTED_ITEM*>( aItem ) );
        m_trackList.SetDirty( true );
        break;

    case PCB_VIA_T:
        m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( aItem ) ].MarkItemsAsInvalid();
        m_itemMap.erase( static_cast<BOARD_CONNECTED_ITEM*>( aItem ) );
        m_viaList.SetDirty( true );
        break;

    case PCB_ZONE_AREA_T:
    case PCB_ZONE_T:
    {
        m_itemMap[ static_cast<BOARD_CONNECTED_ITEM*>( aItem ) ].MarkItemsAsInvalid();
        m_itemMap.erase ( static_cast<BOARD_CONNECTED_ITEM*>( aItem ) );
        m_zoneList.SetDirty( true );
        break;
    }

    default:
        return false;
    }

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

            for( D_PAD* pad = mod->PadsList(); pad; pad = pad->Next() )
                MarkNetAsDirty( pad->GetNetCode() );
        }
    }
}


bool CN_CONNECTIVITY_ALGO::Add( BOARD_ITEM* aItem )
{
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

            add( m_padList, pad );
        }

        break;

    case PCB_PAD_T:
        if( m_itemMap.find ( static_cast<D_PAD*>( aItem ) ) != m_itemMap.end() )
            return false;

        add( m_padList, static_cast<D_PAD*>( aItem ) );

        break;

    case PCB_TRACE_T:
    {
        if( m_itemMap.find( static_cast<TRACK*>( aItem ) ) != m_itemMap.end() )
            return false;

        add( m_trackList, static_cast<TRACK*>( aItem ) );

        break;
    }

    case PCB_VIA_T:
        if( m_itemMap.find( static_cast<VIA*>( aItem ) ) != m_itemMap.end() )
            return false;

        add( m_viaList, static_cast<VIA*>( aItem ) );

        break;

    case PCB_ZONE_AREA_T:
    case PCB_ZONE_T:
    {
        auto zone = static_cast<ZONE_CONTAINER*>( aItem );

        if( m_itemMap.find( static_cast<ZONE_CONTAINER*>( aItem ) ) != m_itemMap.end() )
            return false;

        m_itemMap[zone] = ITEM_MAP_ENTRY();

        for( auto zitem : m_zoneList.Add( zone ) )
            m_itemMap[zone].Link(zitem);

        break;
    }

    default:
        return false;
    }

    return true;
}


void CN_CONNECTIVITY_ALGO::searchConnections( bool aIncludeZones )
{
    std::mutex cnListLock;

    int totalDirtyCount = 0;

    if( m_lastSearchWithZones != aIncludeZones )
    {
        m_padList.MarkAllAsDirty();
        m_viaList.MarkAllAsDirty();
        m_trackList.MarkAllAsDirty();
        m_zoneList.MarkAllAsDirty();
    }

    m_lastSearchWithZones = aIncludeZones;

    auto checkForConnection = [ &cnListLock ] ( const CN_ANCHOR_PTR point, CN_ITEM* aRefItem, int aMaxDist = 0 )
    {
        const auto parent = aRefItem->Parent();

        assert( point->Item() );
        assert( point->Item()->Parent() );
        assert( aRefItem->Parent() );

        if( !point->Item()->Valid() )
            return;

        if( !aRefItem->Valid() )
            return;

        if( parent == point->Item()->Parent() )
            return;

        if( !( parent->GetLayerSet() &
                point->Item()->Parent()->GetLayerSet() ).any() )
            return;

        switch( parent->Type() )
        {
            case PCB_PAD_T:
            case PCB_VIA_T:

                if( parent->HitTest( wxPoint( point->Pos().x, point->Pos().y ) ) )
                {
                    std::lock_guard<std::mutex> lock( cnListLock );
                    CN_ITEM::Connect( aRefItem, point->Item() );
                }

                break;

            case PCB_TRACE_T:
            {
                const auto track = static_cast<TRACK*> ( parent );

                const VECTOR2I d_start( VECTOR2I( track->GetStart() ) - point->Pos() );
                const VECTOR2I d_end( VECTOR2I( track->GetEnd() ) - point->Pos() );

                if( d_start.EuclideanNorm() < aMaxDist
                    || d_end.EuclideanNorm() < aMaxDist )
                {
                    std::lock_guard<std::mutex> lock( cnListLock );
                    CN_ITEM::Connect( aRefItem, point->Item() );
                }
                break;
            }

            case PCB_ZONE_T:
            case PCB_ZONE_AREA_T:
            {
                const auto zone = static_cast<ZONE_CONTAINER*> ( parent );
                auto zoneItem = static_cast<CN_ZONE*> ( aRefItem );

                if( point->Item()->Net() != parent->GetNetCode() )
                    return;

                if( !( zone->GetLayerSet() &
                                            point->Item()->Parent()->GetLayerSet() ).any() )
                    return;

                if( zoneItem->ContainsAnchor( point ) )
                {
                    std::lock_guard<std::mutex> lock( cnListLock );
                    CN_ITEM::Connect( zoneItem, point->Item() );
                }

                break;

            }
            default :
                assert( false );
        }
    };

    auto checkInterZoneConnection = [ &cnListLock ] ( CN_ZONE* testedZone, CN_ZONE* aRefZone )
    {
        const auto parentZone = static_cast<const ZONE_CONTAINER*>( aRefZone->Parent() );

        if( testedZone->Parent()->Type () != PCB_ZONE_AREA_T )
            return;

        if( testedZone == aRefZone )
             return;

        if( testedZone->Parent() == aRefZone->Parent() )
            return;

        if( testedZone->Net() != parentZone->GetNetCode() )
            return; // we only test zones belonging to the same net

        if( !( testedZone->Parent()->GetLayerSet() & parentZone->GetLayerSet() ).any() )
            return; // and on same layer

        const auto& outline = parentZone->GetFilledPolysList().COutline( aRefZone->SubpolyIndex() );

        for( int i = 0; i < outline.PointCount(); i++ )
        {
            if( testedZone->ContainsPoint( outline.CPoint( i ) ) )
            {
                std::lock_guard<std::mutex> lock( cnListLock );

                CN_ITEM::Connect( aRefZone, testedZone );
                return;
            }
        }

        const auto testedZoneParent = static_cast<const ZONE_CONTAINER*>( testedZone->Parent() );

        const auto& outline2 = testedZoneParent->GetFilledPolysList().COutline( testedZone->SubpolyIndex() );

        for( int i = 0; i < outline2.PointCount(); i++ )
        {
            if( aRefZone->ContainsPoint( outline2.CPoint( i ) ) )
            {
                std::lock_guard<std::mutex> lock( cnListLock );

                CN_ITEM::Connect( aRefZone, testedZone );
                return;
            }
        }
    };

#ifdef CONNECTIVITY_DEBUG
    printf("Search start\n");
#endif

    std::vector<CN_ITEM*> garbage;
    garbage.reserve( 1024 );

    m_padList.RemoveInvalidItems( garbage );
    m_viaList.RemoveInvalidItems( garbage );
    m_trackList.RemoveInvalidItems( garbage );
    m_zoneList.RemoveInvalidItems( garbage );

    for( auto item : garbage )
        delete item;

    //auto all = allItemsInBoard();

    #ifdef CONNECTIVITY_DEBUG
        for( auto item : m_padList )
            if( all.find( item->Parent() ) == all.end() ) { printf("Failing pad : %p\n", item->Parent() ); assert ( false ); }

        for( auto item : m_viaList )
            if( all.find( item->Parent() ) == all.end() ) { printf("Failing via : %p\n", item->Parent() ); assert ( false ); }

        for( auto item : m_trackList )
            if( all.find( item->Parent() ) == all.end() ) { printf("Failing track : %p\n", item->Parent() ); assert ( false ); }

        for( auto item : m_zoneList )
            if( all.find( item->Parent() ) == all.end() ) { printf("Failing zome : %p\n", item->Parent() ); assert ( false ); }
    #endif

#ifdef PROFILE
    PROF_COUNTER search_cnt( "search-connections" );
    PROF_COUNTER search_basic( "search-basic" );
#endif

    if( m_padList.IsDirty() || m_trackList.IsDirty() || m_viaList.IsDirty() )
    {
        totalDirtyCount++;

        for( auto padItem : m_padList )
        {
            auto pad = static_cast<D_PAD*> ( padItem->Parent() );
            auto searchPads = std::bind( checkForConnection, _1, padItem );

            m_padList.FindNearby( pad->ShapePos(), pad->GetBoundingRadius(), searchPads );
            m_trackList.FindNearby( pad->ShapePos(), pad->GetBoundingRadius(), searchPads );
            m_viaList.FindNearby( pad->ShapePos(), pad->GetBoundingRadius(), searchPads );
        }

        for( auto& trackItem : m_trackList )
        {
            auto track = static_cast<TRACK*> ( trackItem->Parent() );
            int dist_max = track->GetWidth() / 2;
            auto searchTracks = std::bind( checkForConnection, _1, trackItem, dist_max );

            m_trackList.FindNearby( track->GetStart(), dist_max, searchTracks );
            m_trackList.FindNearby( track->GetEnd(), dist_max, searchTracks );
        }

        for( auto& viaItem : m_viaList )
        {
            auto via = static_cast<VIA*> ( viaItem->Parent() );
            int dist_max = via->GetWidth() / 2;
            auto searchVias = std::bind( checkForConnection, _1, viaItem, dist_max );

            totalDirtyCount++;
            m_viaList.FindNearby( via->GetStart(), dist_max, searchVias );
            m_trackList.FindNearby( via->GetStart(), dist_max, searchVias );
        }
    }

#ifdef PROFILE
    search_basic.Show();
#endif

    if( aIncludeZones )
    {
        int cnt = 0;

        if( m_progressReporter )
        {
            m_progressReporter->SetMaxProgress( m_zoneList.Size() );
        }

        #ifdef USE_OPENMP
            #pragma omp parallel
        #endif
        {
            #ifdef USE_OPENMP
                #pragma omp master
            #endif
            if (m_progressReporter)
            {
                m_progressReporter->KeepRefreshing();
            }

            #ifdef USE_OPENMP
                #pragma omp for schedule(dynamic)
            #endif
            for(int i = 0; i < m_zoneList.Size(); i++ )
            {
                auto item = m_zoneList[i];
                auto zoneItem = static_cast<CN_ZONE *> (item);
                auto searchZones = std::bind( checkForConnection, _1, zoneItem );

                if( zoneItem->Dirty() || m_padList.IsDirty() || m_trackList.IsDirty() || m_viaList.IsDirty() )
                {
                    totalDirtyCount++;
                    m_viaList.FindNearby( zoneItem->BBox(), searchZones );
                    m_trackList.FindNearby( zoneItem->BBox(), searchZones );
                    m_padList.FindNearby( zoneItem->BBox(), searchZones );
                    m_zoneList.FindNearbyZones( zoneItem->BBox(), std::bind( checkInterZoneConnection, _1, zoneItem ) );
                }

                {
                    std::lock_guard<std::mutex> lock( cnListLock );
                    cnt++;

                    if (m_progressReporter)
                    {
                        m_progressReporter->AdvanceProgress();
                    }
                }
            }
        }

        m_zoneList.ClearDirtyFlags();
    }

    m_padList.ClearDirtyFlags();
    m_viaList.ClearDirtyFlags();
    m_trackList.ClearDirtyFlags();

#ifdef CONNECTIVITY_DEBUG
    printf("Search end\n");
#endif

#ifdef PROFILE
    search_cnt.Show();
#endif
}


void CN_ITEM::RemoveInvalidRefs()
{
    auto lastConn = std::remove_if(m_connected.begin(), m_connected.end(), [] ( CN_ITEM * item) {
        return !item->Valid();
    } );

    m_connected.resize( lastConn - m_connected.begin() );
}


void CN_LIST::RemoveInvalidItems( std::vector<CN_ITEM*>& aGarbage )
{
    auto lastAnchor = std::remove_if(m_anchors.begin(), m_anchors.end(),
        [] ( const CN_ANCHOR_PTR anchor ) {
            return !anchor->Valid();
        } );

    m_anchors.resize( lastAnchor - m_anchors.begin() );

    auto lastItem = std::remove_if(m_items.begin(), m_items.end(), [&aGarbage] ( CN_ITEM* item ) {
        if( !item->Valid() )
        {
            aGarbage.push_back ( item );
            return true;
        }

        return false;
    } );

    m_items.resize( lastItem - m_items.begin() );

    // fixme: mem leaks

    for( auto item : m_items )
        item->RemoveInvalidRefs();
}


bool CN_CONNECTIVITY_ALGO::isDirty() const
{
    return m_viaList.IsDirty() || m_trackList.IsDirty() || m_zoneList.IsDirty() || m_padList.IsDirty();
}


const CN_CONNECTIVITY_ALGO::CLUSTERS CN_CONNECTIVITY_ALGO::SearchClusters( CLUSTER_SEARCH_MODE aMode )
{
    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_PAD_T, PCB_VIA_T, PCB_ZONE_AREA_T, PCB_MODULE_T, EOT };
    return SearchClusters( aMode, types, -1 );
}


const CN_CONNECTIVITY_ALGO::CLUSTERS CN_CONNECTIVITY_ALGO::SearchClusters( CLUSTER_SEARCH_MODE aMode,
        const KICAD_T aTypes[], int aSingleNet )
{
    bool includeZones = ( aMode != CSM_PROPAGATE );
    bool withinAnyNet = ( aMode != CSM_PROPAGATE );

    std::deque<CN_ITEM*> Q;
    CN_ITEM* head = nullptr;
    CLUSTERS clusters;

    if( isDirty() )
        searchConnections( includeZones );

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

    std::for_each( m_padList.begin(), m_padList.end(), addToSearchList );
    std::for_each( m_trackList.begin(), m_trackList.end(), addToSearchList );
    std::for_each( m_viaList.begin(), m_viaList.end(), addToSearchList );

    if( includeZones )
    {
        std::for_each( m_zoneList.begin(), m_zoneList.end(), addToSearchList );
    }


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
    printf("Active clusters: %d\n");

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
            case PCB_ZONE_T:
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


void CN_CONNECTIVITY_ALGO::propagateConnections()
{
    for( auto cluster : m_connClusters )
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


void CN_CONNECTIVITY_ALGO::PropagateNets()
{
    //searchConnections( false );
    m_connClusters = SearchClusters( CSM_PROPAGATE );
    propagateConnections();
}


void CN_CONNECTIVITY_ALGO::FindIsolatedCopperIslands( ZONE_CONTAINER* aZone, std::vector<int>& aIslands )
{
    if( aZone->GetFilledPolysList().IsEmpty() )
        return;

    aIslands.clear();

    Remove( aZone );
    Add( aZone );

    m_connClusters = SearchClusters( CSM_CONNECTIVITY_CHECK );

    for( auto cluster : m_connClusters )
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
    {
        if( z.m_zone->GetFilledPolysList().IsEmpty() )
            continue;

        Remove( z.m_zone );
        Add( z.m_zone );
    }

    m_connClusters = SearchClusters( CSM_CONNECTIVITY_CHECK );

    for ( auto& zone : aZones )
    {
        if( zone.m_zone->GetFilledPolysList().IsEmpty() )
            continue;

        for( auto cluster : m_connClusters )
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
        m_dirtyNets.resize( aNet + 1 );

    m_dirtyNets[aNet] = true;
}


int CN_ITEM::AnchorCount() const
{
    if( !m_valid )
        return 0;

    return m_parent->Type() == PCB_TRACE_T ? 2 : 1;
}


const VECTOR2I CN_ITEM::GetAnchor( int n ) const
{
    if( !m_valid )
        return VECTOR2I();

    switch( m_parent->Type() )
    {
        case PCB_PAD_T:
            return static_cast<const D_PAD*>( m_parent )->ShapePos();
            break;

        case PCB_TRACE_T:
        {
            auto tr = static_cast<const TRACK*>( m_parent );
            return ( n == 0 ? tr->GetStart() : tr->GetEnd() );

            break;
        }

        case PCB_VIA_T:
            return static_cast<const VIA*>( m_parent )->GetStart();

        default:
            assert( false );
            return VECTOR2I();
    }
}


int CN_ZONE::AnchorCount() const
{
    if( !Valid() )
        return 0;

    const auto zone = static_cast<const ZONE_CONTAINER*>( Parent() );
    const auto& outline = zone->GetFilledPolysList().COutline( m_subpolyIndex );

    return outline.PointCount() ? 1 : 0;
}


const VECTOR2I CN_ZONE::GetAnchor( int n ) const
{
    if( !Valid() )
        return VECTOR2I();

    const auto zone = static_cast<const ZONE_CONTAINER*> ( Parent() );
    const auto& outline = zone->GetFilledPolysList().COutline( m_subpolyIndex );

    return outline.CPoint( 0 );
}


int CN_ITEM::Net() const
{
    if( !m_parent || !m_valid )
        return -1;

    return m_parent->GetNetCode();
}


BOARD_CONNECTED_ITEM* CN_ANCHOR::Parent() const
{
    assert( m_item->Valid() );
    return m_item->Parent();
}


bool CN_ANCHOR::Valid() const
{
    if( !m_item )
        return false;

    return m_item->Valid();
}


void CN_CONNECTIVITY_ALGO::Clear()
{
    m_ratsnestClusters.clear();
    m_connClusters.clear();
    m_itemMap.clear();
    m_padList.Clear();
    m_trackList.Clear();
    m_viaList.Clear();
    m_zoneList.Clear();

}


void CN_CONNECTIVITY_ALGO::ForEachItem( std::function<void(CN_ITEM*)> aFunc )
{
    for( auto item : m_padList )
        aFunc( item );

    for( auto item : m_viaList )
        aFunc( item );

    for( auto item : m_trackList )
        aFunc( item );

    for( auto item : m_zoneList )
        aFunc( item );
}


void CN_CONNECTIVITY_ALGO::ForEachAnchor( std::function<void(CN_ANCHOR_PTR)> aFunc )
{
    for( auto anchor : m_padList.Anchors() )
        aFunc( anchor );

    for( auto anchor : m_viaList.Anchors() )
        aFunc( anchor );

    for( auto anchor : m_trackList.Anchors() )
        aFunc( anchor );

    for( auto anchor : m_zoneList.Anchors() )
        aFunc( anchor );
}


bool CN_ANCHOR::IsDangling() const
{
    if( !m_cluster )
        return true;

    int validCount = 0;

    for( auto item : *m_cluster )
    {
        if( item->Valid() )
            validCount++;
    }

    return validCount <= 1;
}

void CN_CONNECTIVITY_ALGO::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
}
