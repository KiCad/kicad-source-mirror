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

#include <connectivity_algo.h>
#include <widgets/progress_reporter.h>
#include <geometry/geometry_utils.h>

#include <thread>
#include <mutex>

#ifdef PROFILE
#include <profile.h>
#endif


using namespace std::placeholders;

bool operator<( const CN_ANCHOR_PTR& a, const CN_ANCHOR_PTR& b )
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

    case PCB_SEGZONE_T:
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

            for( D_PAD* pad = mod->PadsList(); pad; pad = pad->Next() )
                MarkNetAsDirty( pad->GetNetCode() );
        }
    }
}


bool CN_CONNECTIVITY_ALGO::Add( BOARD_ITEM* aItem )
{
    if( !IsCopperLayer( aItem->GetLayer() ) )
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

    //N.B. SEGZONE items are deprecated and not to used for connectivity
    case PCB_SEGZONE_T:
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

    size_t numDirty = std::count_if( m_itemList.begin(), m_itemList.end(), [] ( CN_ITEM* aItem )
            { return aItem->Dirty(); } );

    if( m_progressReporter )
    {
        m_progressReporter->SetMaxProgress( numDirty );
        m_progressReporter->KeepRefreshing();
    }

    if( m_itemList.IsDirty() )
    {
        std::atomic<int> nextItem( 0 );
        std::atomic<size_t> threadsFinished( 0 );

        size_t parallelThreadCount = std::min<size_t>(
                std::max<size_t>( std::thread::hardware_concurrency(), 2 ),
                numDirty );

        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
        {
            std::thread t = std::thread( [&nextItem, &threadsFinished, this]()
            {
                for( int i = nextItem.fetch_add( 1 );
                         i < m_itemList.Size();
                         i = nextItem.fetch_add( 1 ) )
                {
                    auto item = m_itemList[i];
                    if( item->Dirty() )
                    {
                        CN_VISITOR visitor( item, &m_listLock );
                        m_itemList.FindNearby( item, visitor );

                        if( m_progressReporter )
                            m_progressReporter->AdvanceProgress();
                    }
                }

                threadsFinished++;
            } );

            t.detach();
        }

        // Finalize the connectivity threads
        while( threadsFinished < parallelThreadCount )
        {
            if( m_progressReporter )
                m_progressReporter->KeepRefreshing();

            // This routine is called every click while routing so keep the sleep time minimal
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }

#ifdef PROFILE
        search_basic.Show();
#endif

    m_itemList.ClearDirtyFlags();

#ifdef CONNECTIVITY_DEBUG
    printf("Search end\n");
#endif

}


void CN_ITEM::RemoveInvalidRefs()
{
    for( auto it = m_connected.begin(); it != m_connected.end(); )
    {
        if( !(*it)->Valid() )
            it = m_connected.erase( it );
        else
            ++it;
    }
}


void CN_LIST::RemoveInvalidItems( std::vector<CN_ITEM*>& aGarbage )
{
    if( !m_hasInvalid )
        return;

    auto lastItem = std::remove_if(m_items.begin(), m_items.end(), [&aGarbage] ( CN_ITEM* item )
    {
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

    for( auto item : aGarbage )
        m_index.Remove( item );

    m_hasInvalid = false;
}


bool CN_CONNECTIVITY_ALGO::isDirty() const
{
    return m_itemList.IsDirty();
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

    if( isDirty() )
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

            //N.B. SEGZONE items are deprecated and not to used for connectivity
            case PCB_SEGZONE_T:
            default:
                break;
        }
    }
}


void CN_CONNECTIVITY_ALGO::propagateConnections()
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
        std::lock_guard<std::mutex> lock( *m_listLock );
        CN_ITEM::Connect( zoneItem, aItem );
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
            std::lock_guard<std::mutex> lock( *m_listLock );
            CN_ITEM::Connect( aZoneA, aZoneB );
            return;
        }
    }

    const auto& outline2 = testedParent->GetFilledPolysList().COutline( aZoneB->SubpolyIndex() );

    for( int i = 0; i < outline2.PointCount(); i++ )
    {
        if( aZoneA->ContainsPoint( outline2.CPoint( i ) ) )
        {
            std::lock_guard<std::mutex> lock( *m_listLock );
            CN_ITEM::Connect( aZoneA, aZoneB );
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
        std::lock_guard<std::mutex> lock( *m_listLock );
        CN_ITEM::Connect( m_item, aCandidate );
    }

    return true;
};


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


bool CN_ANCHOR::IsDangling() const
{
    if( !m_cluster )
        return true;

    // Calculate the item count connected to this anchor.
    // m_cluster groups all items connected, but they are not necessary connected
    // at this coordinate point (they are only candidates)
    BOARD_CONNECTED_ITEM* item_ref = Parent();
    LSET layers = item_ref->GetLayerSet() & LSET::AllCuMask();

    // the number of items connected to item_ref at ths anchor point
    int connected_items_count = 0;

    // the minimal number of items connected to item_ref
    // at this anchor point to decide the anchor is *not* dangling
    int minimal_count = 1;

    // a via can be removed if connected to only one other item.
    // the minimal_count is therefore 2
    if( item_ref->Type() == PCB_VIA_T )
        minimal_count = 2;

    for( CN_ITEM* item : *m_cluster )
    {
        if( !item->Valid() )
            continue;

        BOARD_CONNECTED_ITEM* brd_item = item->Parent();

        if( brd_item == item_ref )
            continue;

        // count only items on the same layer at this coordinate (especially for zones)
        if( !( brd_item->GetLayerSet() & layers ).any() )
            continue;

        if( brd_item->Type() == PCB_ZONE_AREA_T )
        {
            ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( brd_item );

            if( zone->HitTestInsideZone( wxPoint( Pos() ) ) )
                connected_items_count++;
        }
        else if( brd_item->HitTest( wxPoint( Pos() ) ) )
            connected_items_count++;
    }

    return connected_items_count < minimal_count;
}

void CN_CONNECTIVITY_ALGO::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
}
