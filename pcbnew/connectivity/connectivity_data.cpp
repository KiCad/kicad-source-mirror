/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifdef PROFILE
#include <core/profile.h>
#endif

#include <algorithm>
#include <future>
#include <initializer_list>

#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/from_to_cache.h>
#include <board_item.h>
#include <project/net_settings.h>
#include <board_design_settings.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <ratsnest/ratsnest_data.h>
#include <progress_reporter.h>
#include <thread_pool.h>
#include <trigo.h>
#include <drc/drc_rtree.h>

CONNECTIVITY_DATA::CONNECTIVITY_DATA() :
        m_skipRatsnestUpdate( false )
{
    m_connAlgo.reset( new CN_CONNECTIVITY_ALGO( this ) );
    m_progressReporter = nullptr;
    m_fromToCache.reset( new FROM_TO_CACHE );
}


CONNECTIVITY_DATA::CONNECTIVITY_DATA( std::shared_ptr<CONNECTIVITY_DATA> aGlobalConnectivity,
                                      const std::vector<BOARD_ITEM*>& aLocalItems,
                                      bool aSkipRatsnestUpdate ) :
        m_skipRatsnestUpdate( aSkipRatsnestUpdate )
{
    Build( aGlobalConnectivity, aLocalItems );
    m_progressReporter = nullptr;
    m_fromToCache.reset( new FROM_TO_CACHE );
}


CONNECTIVITY_DATA::~CONNECTIVITY_DATA()
{
    for( RN_NET* net : m_nets )
        delete net;

    m_nets.clear();
}


bool CONNECTIVITY_DATA::Add( BOARD_ITEM* aItem )
{
    m_connAlgo->Add( aItem );
    return true;
}


bool CONNECTIVITY_DATA::Remove( BOARD_ITEM* aItem )
{
    m_connAlgo->Remove( aItem );
    return true;
}


bool CONNECTIVITY_DATA::Update( BOARD_ITEM* aItem )
{
    m_connAlgo->Remove( aItem );
    m_connAlgo->Add( aItem );
    return true;
}


bool CONNECTIVITY_DATA::Build( BOARD* aBoard, PROGRESS_REPORTER* aReporter )
{
    aBoard->CacheTriangulation( aReporter );

    std::unique_lock<KISPINLOCK> lock( m_lock, std::try_to_lock );

    if( !lock )
        return false;

    if( aReporter )
    {
        aReporter->Report( _( "Updating nets..." ) );
        aReporter->KeepRefreshing( false );
    }

    for( RN_NET* net : m_nets )
        delete net;

    m_nets.clear();

    m_connAlgo.reset( new CN_CONNECTIVITY_ALGO( this ) );
    m_connAlgo->Build( aBoard, aReporter );

    m_netSettings = aBoard->GetDesignSettings().m_NetSettings;

    RefreshNetcodeMap( aBoard );

    if( aReporter )
    {
        aReporter->SetCurrentProgress( 0.75 );
        aReporter->KeepRefreshing( false );
    }

    internalRecalculateRatsnest();

    if( aReporter )
    {
        aReporter->SetCurrentProgress( 1.0 );
        aReporter->KeepRefreshing( false );
    }

    return true;
}


void CONNECTIVITY_DATA::RefreshNetcodeMap( BOARD* aBoard )
{
    m_netcodeMap.clear();

    for( NETINFO_ITEM* net : aBoard->GetNetInfo() )
        m_netcodeMap[net->GetNetCode()] = net->GetNetname();
}


void CONNECTIVITY_DATA::Build( std::shared_ptr<CONNECTIVITY_DATA>& aGlobalConnectivity,
                               const std::vector<BOARD_ITEM*>& aLocalItems )
{
    std::unique_lock<KISPINLOCK> lock( m_lock, std::try_to_lock );

    if( !lock )
        return;

    m_connAlgo.reset( new CN_CONNECTIVITY_ALGO( this ) );
    m_connAlgo->LocalBuild( aGlobalConnectivity, aLocalItems );

    internalRecalculateRatsnest();
}


void CONNECTIVITY_DATA::Move( const VECTOR2I& aDelta )
{
    m_connAlgo->ForEachAnchor( [&aDelta]( CN_ANCHOR& anchor )
                               {
                                   anchor.Move( aDelta );
                               } );
}


void CONNECTIVITY_DATA::updateRatsnest()
{
#ifdef PROFILE
    PROF_TIMER rnUpdate( "update-ratsnest" );
#endif

    std::vector<RN_NET*> dirty_nets;

    // Start with net 1 as net 0 is reserved for not-connected
    // Nets without nodes are also ignored
    std::copy_if( m_nets.begin() + 1, m_nets.end(), std::back_inserter( dirty_nets ),
            [] ( RN_NET* aNet )
            {
                return aNet->IsDirty() && aNet->GetNodeCount() > 0;
            } );

    thread_pool& tp = GetKiCadThreadPool();

    auto results = tp.submit_loop( 0, dirty_nets.size(),
                            [&]( const int ii )
                            {
                                dirty_nets[ii]->UpdateNet();
                            } );
    results.wait();

    auto results2 = tp.submit_loop( 0, dirty_nets.size(),
                            [&]( const int ii )
                            {
                                dirty_nets[ii]->OptimizeRNEdges();
                            } );
    results2.wait();

#ifdef PROFILE
    rnUpdate.Show();
#endif
}


void CONNECTIVITY_DATA::addRatsnestCluster( const std::shared_ptr<CN_CLUSTER>& aCluster )
{
    RN_NET* rnNet = m_nets[ aCluster->OriginNet() ];

    rnNet->AddCluster( aCluster );
}


void CONNECTIVITY_DATA::RecalculateRatsnest( BOARD_COMMIT* aCommit  )
{

    // We can take over the lock here if called in the same thread
    // This is to prevent redraw during a RecalculateRatsnets process
    std::unique_lock<KISPINLOCK> lock( m_lock );

    internalRecalculateRatsnest( aCommit );

}

void CONNECTIVITY_DATA::internalRecalculateRatsnest( BOARD_COMMIT* aCommit  )
{
    m_connAlgo->PropagateNets( aCommit );

    int lastNet = m_connAlgo->NetCount();

    if( lastNet >= (int) m_nets.size() )
    {
        unsigned int prevSize = m_nets.size();
        m_nets.resize( lastNet + 1 );

        for( unsigned int i = prevSize; i < m_nets.size(); i++ )
            m_nets[i] = new RN_NET;
    }
    else
    {
        for( size_t ii = lastNet; ii < m_nets.size(); ++ii )
            m_nets[ii]->Clear();
    }

    const std::vector<std::shared_ptr<CN_CLUSTER>>& clusters = m_connAlgo->GetClusters();

    for( int net = 0; net < lastNet; net++ )
    {
        if( m_connAlgo->IsNetDirty( net ) )
            m_nets[net]->Clear();
    }

    for( const std::shared_ptr<CN_CLUSTER>& c : clusters )
    {
        int net = c->OriginNet();

        // Don't add intentionally-kept zone islands to the ratsnest
        if( c->IsOrphaned() && c->Size() == 1 )
        {
            if( dynamic_cast<CN_ZONE_LAYER*>( *c->begin() ) )
                continue;
        }

        if( m_connAlgo->IsNetDirty( net ) )
            addRatsnestCluster( c );
    }

    m_connAlgo->ClearDirtyFlags();

    if( !m_skipRatsnestUpdate )
        updateRatsnest();
}


void CONNECTIVITY_DATA::BlockRatsnestItems( const std::vector<BOARD_ITEM*>& aItems )
{
    std::vector<BOARD_CONNECTED_ITEM*> citems;

    for( BOARD_ITEM* item : aItems )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( PAD* pad : static_cast<FOOTPRINT*>(item)->Pads() )
                citems.push_back( pad );
        }
        else
        {
            if( BOARD_CONNECTED_ITEM* citem = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
                citems.push_back( citem );
        }
    }

    for( const BOARD_CONNECTED_ITEM* item : citems )
    {
        if ( m_connAlgo->ItemExists( item ) )
        {
            CN_CONNECTIVITY_ALGO::ITEM_MAP_ENTRY& entry = m_connAlgo->ItemEntry( item );

            for( CN_ITEM* cnItem : entry.GetItems() )
            {
                for( const std::shared_ptr<CN_ANCHOR>& anchor : cnItem->Anchors() )
                    anchor->SetNoLine( true );
            }
        }
    }
}


int CONNECTIVITY_DATA::GetNetCount() const
{
    return m_connAlgo->NetCount();
}


void CONNECTIVITY_DATA::FillIsolatedIslandsMap( std::map<ZONE*, std::map<PCB_LAYER_ID, ISOLATED_ISLANDS>>& aMap,
                                                bool aConnectivityAlreadyRebuilt )
{
    m_connAlgo->FillIsolatedIslandsMap( aMap, aConnectivityAlreadyRebuilt );
}


void CONNECTIVITY_DATA::ComputeLocalRatsnest( const std::vector<BOARD_ITEM*>& aItems,
                                              const CONNECTIVITY_DATA* aDynamicData,
                                              VECTOR2I aInternalOffset )
{
    if( !aDynamicData )
        return;

    m_dynamicRatsnest.clear();
    std::mutex dynamic_ratsnest_mutex;

    // This gets connections between the stationary board and the
    // moving selection

    auto update_lambda = [&]( int nc )
    {
        RN_NET* dynamicNet = aDynamicData->m_nets[nc];
        RN_NET* staticNet  = m_nets[nc];

        /// We don't need to compute the dynamic ratsnest in two cases:
        /// 1) We are not moving any net elements
        /// 2) We are moving all net elements
        if( dynamicNet->GetNodeCount() != 0 && dynamicNet->GetNodeCount() != staticNet->GetNodeCount() )
        {
            VECTOR2I pos1, pos2;

            if( staticNet->NearestBicoloredPair( dynamicNet, pos1, pos2 ) )
            {
                RN_DYNAMIC_LINE l;
                l.a = pos1;
                l.b = pos2;
                l.netCode = nc;

                std::lock_guard<std::mutex> lock( dynamic_ratsnest_mutex );
                m_dynamicRatsnest.push_back( l );
            }
        }
    };

    thread_pool& tp = GetKiCadThreadPool();
    size_t num_nets = std::min( m_nets.size(), aDynamicData->m_nets.size() );

    auto results = tp.submit_loop( 1, num_nets,
                            [&]( const int ii )
                            {
                                update_lambda( ii );
                            });
    results.wait();

    // This gets the ratsnest for internal connections in the moving set
    const std::vector<CN_EDGE>& edges = GetRatsnestForItems( aItems );

    for( const CN_EDGE& edge : edges )
    {
        const std::shared_ptr<const CN_ANCHOR>& nodeA = edge.GetSourceNode();
        const std::shared_ptr<const CN_ANCHOR>& nodeB = edge.GetTargetNode();

        if( !nodeA || nodeA->Dirty() || !nodeB || nodeB->Dirty() )
            continue;

        RN_DYNAMIC_LINE l;

        // Use the parents' positions
        l.a = nodeA->Parent()->GetPosition() + aInternalOffset;
        l.b = nodeB->Parent()->GetPosition() + aInternalOffset;
        l.netCode = 0;
        m_dynamicRatsnest.push_back( l );
    }
}


void CONNECTIVITY_DATA::ClearLocalRatsnest()
{
    m_connAlgo->ForEachAnchor( []( CN_ANCHOR& anchor )
                               {
                                   anchor.SetNoLine( false );
                               } );
    HideLocalRatsnest();
}


void CONNECTIVITY_DATA::HideLocalRatsnest()
{
    m_dynamicRatsnest.clear();
}


void CONNECTIVITY_DATA::PropagateNets( BOARD_COMMIT* aCommit )
{
    m_connAlgo->PropagateNets( aCommit );
}


bool CONNECTIVITY_DATA::IsConnectedOnLayer( const BOARD_CONNECTED_ITEM *aItem, int aLayer,
                                            const std::initializer_list<KICAD_T>& aTypes ) const
{
    CN_CONNECTIVITY_ALGO::ITEM_MAP_ENTRY &entry = m_connAlgo->ItemEntry( aItem );

    FOOTPRINT* parentFootprint = aItem->GetParentFootprint();

    auto matchType =
            [&]( KICAD_T aItemType )
            {
                if( aTypes.size() == 0 )
                    return true;

                return alg::contains( aTypes, aItemType);
            };

    for( CN_ITEM* citem : entry.GetItems() )
    {
        for( CN_ITEM* connected : citem->ConnectedItems() )
        {
            CN_ZONE_LAYER* zoneLayer = dynamic_cast<CN_ZONE_LAYER*>( connected );

            // lyIdx is compatible with StartLayer() and EndLayer() notation in CN_ITEM
            // items, where B_Cu is set to INT_MAX (std::numeric_limits<int>::max())
            int lyIdx = aLayer;

            if( aLayer == B_Cu )
                lyIdx = std::numeric_limits<int>::max();

            if( connected->Valid()
                    && connected->StartLayer() <= lyIdx && connected->EndLayer() >= lyIdx
                    && matchType( connected->Parent()->Type() )
                    && connected->Net() == aItem->GetNetCode() )
            {
                BOARD_ITEM* connectedItem = connected->Parent();

                if( connectedItem == aItem )
                    continue;

                if( parentFootprint && connectedItem
                        && connectedItem->GetParentFootprint() == parentFootprint )
                {
                    continue;
                }

                if( aItem->Type() == PCB_PAD_T && connectedItem
                        && connectedItem->Type() == PCB_PAD_T )
                {
                    const PAD* thisPad = static_cast<const PAD*>( aItem );
                    const PAD* otherPad = static_cast<const PAD*>( connectedItem );

                    auto flashesConditionally = []( UNCONNECTED_LAYER_MODE aMode )
                            {
                                return aMode == UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END
                                        || aMode == UNCONNECTED_LAYER_MODE::REMOVE_ALL;
                            };

                    if( flashesConditionally( thisPad->Padstack().UnconnectedLayerMode() )
                            && flashesConditionally( otherPad->Padstack().UnconnectedLayerMode() ) )
                    {
                        continue;
                    }
                }

                if( aItem->Type() == PCB_PAD_T && zoneLayer )
                {
                    const PAD*    pad = static_cast<const PAD*>( aItem );
                    ZONE*         zone = static_cast<ZONE*>( zoneLayer->Parent() );
                    int           islandIdx = zoneLayer->SubpolyIndex();

                    if( zone->IsFilled() )
                    {
                        PCB_LAYER_ID pcbLayer = ToLAYER_ID( aLayer );
                        const SHAPE_POLY_SET*   zoneFill = zone->GetFill( pcbLayer );
                        const SHAPE_LINE_CHAIN& padHull = pad->GetEffectivePolygon( pcbLayer,
                                                                                    ERROR_INSIDE )->Outline( 0 );

                        for( const VECTOR2I& pt : zoneFill->COutline( islandIdx ).CPoints() )
                        {
                            // If the entire island is inside the pad's flashing then the pad
                            // won't actually connect to anything else, so only return true if
                            // part of the island is *outside* the pad's flashing.

                            if( !padHull.PointInside( pt ) )
                                return true;
                        }
                    }

                    continue;
                }
                else if( aItem->Type() == PCB_VIA_T && zoneLayer )
                {
                    const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem );
                    ZONE*          zone = static_cast<ZONE*>( zoneLayer->Parent() );
                    int            islandIdx = zoneLayer->SubpolyIndex();

                    if( zone->IsFilled() )
                    {
                        PCB_LAYER_ID          layer = ToLAYER_ID( aLayer );
                        const SHAPE_POLY_SET* zoneFill = zone->GetFill( layer );
                        SHAPE_CIRCLE          viaHull( via->GetCenter(), via->GetWidth( layer ) / 2 );

                        for( const VECTOR2I& pt : zoneFill->COutline( islandIdx ).CPoints() )
                        {
                            // If the entire island is inside the via's flashing then the via
                            // won't actually connect to anything else, so only return true if
                            // part of the island is *outside* the via's flashing.

                            if( !viaHull.SHAPE::Collide( pt ) )
                                return true;
                        }
                    }

                    continue;
                }

                return true;
            }
        }
    }

    return false;
}


unsigned int CONNECTIVITY_DATA::GetUnconnectedCount( bool aVisibleOnly ) const
{
    unsigned int unconnected = 0;

    for( RN_NET* net : m_nets )
    {
        if( !net )
            continue;

        for( const CN_EDGE& edge : net->GetEdges() )
        {
            if( edge.IsVisible() || !aVisibleOnly )
                ++unconnected;
        }
    }

    return unconnected;
}


void CONNECTIVITY_DATA::ClearRatsnest()
{
    for( RN_NET* net : m_nets )
        net->Clear();
}


const std::vector<BOARD_CONNECTED_ITEM*>
CONNECTIVITY_DATA::GetConnectedItems( const BOARD_CONNECTED_ITEM *aItem, int aFlags ) const
{
    using CN_CONNECTIVITY_ALGO::CSM_PROPAGATE;
    using CN_CONNECTIVITY_ALGO::CSM_CONNECTIVITY_CHECK;

    std::vector<BOARD_CONNECTED_ITEM*> rv;

    auto clusters = m_connAlgo->SearchClusters( ( aFlags & IGNORE_NETS ) ? CSM_PROPAGATE : CSM_CONNECTIVITY_CHECK,
                                                ( aFlags & EXCLUDE_ZONES ),
                                                ( aFlags & IGNORE_NETS ) ? -1 : aItem->GetNetCode() );

    for( const std::shared_ptr<CN_CLUSTER>& cl : clusters )
    {
        if( cl->Contains( aItem ) )
        {
            for( const CN_ITEM* item : *cl )
            {
                if( item->Valid() )
                    rv.push_back( item->Parent() );
            }
        }
    }

    return rv;
}


const std::vector<BOARD_CONNECTED_ITEM*>
CONNECTIVITY_DATA::GetNetItems( int aNetCode, const std::vector<KICAD_T>& aTypes ) const
{
    std::vector<BOARD_CONNECTED_ITEM*> items;
    items.reserve( 32 );

    std::bitset<MAX_STRUCT_TYPE_ID> type_bits;

    for( KICAD_T scanType : aTypes )
    {
        wxASSERT( scanType < MAX_STRUCT_TYPE_ID );
        type_bits.set( scanType );
    }

    m_connAlgo->ForEachItem(
            [&]( CN_ITEM& aItem )
            {
                if( aItem.Valid() && ( aItem.Net() == aNetCode ) && type_bits[aItem.Parent()->Type()] )
                    items.push_back( aItem.Parent() );
            } );

    std::sort( items.begin(), items.end() );
    items.erase( std::unique( items.begin(), items.end() ), items.end() );
    return items;
}


const std::vector<PCB_TRACK*>
CONNECTIVITY_DATA::GetConnectedTracks( const BOARD_CONNECTED_ITEM* aItem ) const
{
    CN_CONNECTIVITY_ALGO::ITEM_MAP_ENTRY& entry = m_connAlgo->ItemEntry( aItem );

    std::set<PCB_TRACK*> tracks;
    std::vector<PCB_TRACK*> rv;

    for( CN_ITEM* citem : entry.GetItems() )
    {
        for( CN_ITEM* connected : citem->ConnectedItems() )
        {
            if( connected->Valid() &&
                    ( connected->Parent()->Type() == PCB_TRACE_T ||
                            connected->Parent()->Type() == PCB_VIA_T ||
                            connected->Parent()->Type() == PCB_ARC_T ) )
            {
                tracks.insert( static_cast<PCB_TRACK*> ( connected->Parent() ) );
            }
        }
    }

    std::copy( tracks.begin(), tracks.end(), std::back_inserter( rv ) );
    return rv;
}


void CONNECTIVITY_DATA::GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem, std::set<PAD*>* pads ) const
{
    for( CN_ITEM* citem : m_connAlgo->ItemEntry( aItem ).GetItems() )
    {
        for( CN_ITEM* connected : citem->ConnectedItems() )
        {
            if( connected->Valid() && connected->Parent()->Type() == PCB_PAD_T )
                pads->insert( static_cast<PAD*> ( connected->Parent() ) );
        }
    }
}


const std::vector<PAD*> CONNECTIVITY_DATA::GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem )
const
{
    std::set<PAD*>    pads;
    std::vector<PAD*> rv;

    GetConnectedPads( aItem, &pads );

    std::copy( pads.begin(), pads.end(), std::back_inserter( rv ) );
    return rv;
}


void CONNECTIVITY_DATA::GetConnectedPadsAndVias( const BOARD_CONNECTED_ITEM* aItem, std::vector<PAD*>* pads,
                                                 std::vector<PCB_VIA*>* vias )
{
    for( CN_ITEM* citem : m_connAlgo->ItemEntry( aItem ).GetItems() )
    {
        for( CN_ITEM* connected : citem->ConnectedItems() )
        {
            if( connected->Valid() )
            {
                BOARD_CONNECTED_ITEM* parent = connected->Parent();

                if( parent->Type() == PCB_PAD_T )
                    pads->push_back( static_cast<PAD*>( parent ) );
                else if( parent->Type() == PCB_VIA_T )
                    vias->push_back( static_cast<PCB_VIA*>( parent ) );
            }
        }
    }
}


unsigned int CONNECTIVITY_DATA::GetNodeCount( int aNet ) const
{
    int sum = 0;

    if( aNet < 0 )      // Node count for all nets
    {
        for( const RN_NET* net : m_nets )
            sum += net->GetNodeCount();
    }
    else if( aNet < (int) m_nets.size() )
    {
        sum = m_nets[aNet]->GetNodeCount();
    }

    return sum;
}


unsigned int CONNECTIVITY_DATA::GetPadCount( int aNet ) const
{
    int n = 0;

    for( CN_ITEM* pad : m_connAlgo->ItemList() )
    {
        if( !pad->Valid() || pad->Parent()->Type() != PCB_PAD_T)
            continue;

        PAD* dpad = static_cast<PAD*>( pad->Parent() );

        if( aNet < 0 || aNet == dpad->GetNetCode() )
            n++;
    }

    return n;
}


void CONNECTIVITY_DATA::RunOnUnconnectedEdges( std::function<bool( CN_EDGE& )> aFunc )
{
    for( RN_NET* rnNet : m_nets )
    {
        if( rnNet )
        {
            for( CN_EDGE& edge : rnNet->GetEdges() )
            {
                if( !aFunc( edge ) )
                    return;
            }
        }
    }
}


static int getMinDist( BOARD_CONNECTED_ITEM* aItem, const VECTOR2I& aPoint )
{
    switch( aItem->Type() )
    {
    case PCB_TRACE_T:
    case PCB_ARC_T:
    {
        PCB_TRACK* track = static_cast<PCB_TRACK*>( aItem );

        return std::min( track->GetStart().Distance(aPoint ), track->GetEnd().Distance( aPoint ) );
    }

    default:
        return aItem->GetPosition().Distance( aPoint );
    }
}


bool CONNECTIVITY_DATA::TestTrackEndpointDangling( PCB_TRACK* aTrack, bool aIgnoreTracksInPads,
                                                   VECTOR2I* aPos ) const
{
    const std::list<CN_ITEM*>& items = GetConnectivityAlgo()->ItemEntry( aTrack ).GetItems();

    // Not in the connectivity system.  This is a bug!
    if( items.empty() )
    {
        wxFAIL_MSG( wxT( "track not in connectivity system" ) );
        return false;
    }

    CN_ITEM* citem = items.front();

    if( !citem->Valid() )
        return false;

    if( aTrack->Type() == PCB_TRACE_T || aTrack->Type() == PCB_ARC_T )
    {
        // Test if a segment is connected on each end.
        //
        // NB: be wary of short segments which can be connected to the *same* other item on
        // each end.  If that's their only connection then they're still dangling.

        PCB_LAYER_ID layer = aTrack->GetLayer();
        int          accuracy = KiROUND( aTrack->GetWidth() / 2.0 );
        int          start_count = 0;
        int          end_count = 0;

        for( CN_ITEM* connected : citem->ConnectedItems() )
        {
            BOARD_CONNECTED_ITEM* item = connected->Parent();
            ZONE*                 zone = dynamic_cast<ZONE*>( item );
            DRC_RTREE*            rtree = nullptr;
            bool                  hitStart = false;
            bool                  hitEnd = false;

            if( item->GetFlags() & IS_DELETED )
                continue;

            if( zone )
                rtree = zone->GetBoard()->m_CopperZoneRTreeCache[ zone ].get();

            if( rtree )
            {
                SHAPE_CIRCLE start( aTrack->GetStart(), accuracy );
                SHAPE_CIRCLE end( aTrack->GetEnd(), accuracy );

                hitStart = rtree->QueryColliding( start.BBox(), &start, layer );
                hitEnd = rtree->QueryColliding( end.BBox(), &end, layer );
            }
            else
            {
                std::shared_ptr<SHAPE> shape = item->GetEffectiveShape( layer );

                hitStart = shape->Collide( aTrack->GetStart(), accuracy );
                hitEnd = shape->Collide( aTrack->GetEnd(), accuracy );
            }

            if( hitStart && hitEnd )
            {
                if( zone )
                {
                    // Both start and end in a zone: track may be redundant, but it's not dangling
                    return false;
                }
                else if( item->Type() == PCB_PAD_T || item->Type() == PCB_VIA_T )
                {
                    // Both start and end are under a pad: see what the caller wants us to do
                    if( aIgnoreTracksInPads )
                        return false;
                }

                if( getMinDist( item, aTrack->GetStart() ) < getMinDist( item, aTrack->GetEnd() ) )
                    start_count++;
                else
                    end_count++;
            }
            else if( hitStart )
            {
                start_count++;
            }
            else if( hitEnd )
            {
                end_count++;
            }

            if( start_count > 0 && end_count > 0 )
                return false;
        }

        if( aPos )
            *aPos = (start_count == 0 ) ? aTrack->GetStart() : aTrack->GetEnd();

        return true;
    }
    else if( aTrack->Type() == PCB_VIA_T )
    {
        // Test if a via is only connected on one layer

        const std::vector<CN_ITEM*>& connected = citem->ConnectedItems();

        if( connected.empty() )
        {
            // No connections AND no-net is not an error
            if( aTrack->GetNetCode() <= 0 )
                return false;

            if( aPos )
                *aPos = aTrack->GetPosition();

            return true;
        }

        // Here, we check if the via is connected only to items on a single layer
        int first_layer = UNDEFINED_LAYER;

        for( CN_ITEM* item : connected )
        {
            if( item->Parent()->GetFlags() & IS_DELETED )
                continue;

            if( first_layer == UNDEFINED_LAYER )
                first_layer = item->Layer();
            else if( item->Layer() != first_layer )
               return false;
        }

        if( aPos )
            *aPos = aTrack->GetPosition();

        return true;
    }
    else
    {
        wxFAIL_MSG( wxT( "CONNECTIVITY_DATA::TestTrackEndpointDangling: unknown track type" ) );
    }

    return false;
}


const std::vector<BOARD_CONNECTED_ITEM*>
CONNECTIVITY_DATA::GetConnectedItemsAtAnchor( const BOARD_CONNECTED_ITEM* aItem, const VECTOR2I& aAnchor,
                                              const std::vector<KICAD_T>& aTypes, const int& aMaxError ) const
{
    CN_CONNECTIVITY_ALGO::ITEM_MAP_ENTRY& entry = m_connAlgo->ItemEntry( aItem );
    std::vector<BOARD_CONNECTED_ITEM*>    rv;
    SEG::ecoord                           maxError_sq = (SEG::ecoord) aMaxError * aMaxError;

    for( CN_ITEM* cnItem : entry.GetItems() )
    {
        for( CN_ITEM* connected : cnItem->ConnectedItems() )
        {
            for( const std::shared_ptr<CN_ANCHOR>& anchor : connected->Anchors() )
            {
                if( ( anchor->Pos() - aAnchor ).SquaredEuclideanNorm() <= maxError_sq )
                {
                    for( KICAD_T type : aTypes )
                    {
                        if( connected->Valid() && connected->Parent()->Type() == type )
                        {
                            rv.push_back( connected->Parent() );
                            break;
                        }
                    }

                    break;
                }
            }
        }
    }

    return rv;
}


RN_NET* CONNECTIVITY_DATA::GetRatsnestForNet( int aNet )
{
    if ( aNet < 0 || aNet >= (int) m_nets.size() )
        return nullptr;

    return m_nets[ aNet ];
}


void CONNECTIVITY_DATA::MarkItemNetAsDirty( BOARD_ITEM *aItem )
{
    if ( aItem->Type() == PCB_FOOTPRINT_T)
    {
        for( PAD* pad : static_cast<FOOTPRINT*>( aItem )->Pads() )
            m_connAlgo->MarkNetAsDirty( pad->GetNetCode() );
    }

    if (aItem->IsConnected() )
        m_connAlgo->MarkNetAsDirty( static_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode() );
}


void CONNECTIVITY_DATA::RemoveInvalidRefs()
{
    m_connAlgo->RemoveInvalidRefs();

    for( RN_NET* rnNet : m_nets )
        rnNet->RemoveInvalidRefs();
}


void CONNECTIVITY_DATA::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
    m_connAlgo->SetProgressReporter( m_progressReporter );
}


const std::vector<CN_EDGE>
CONNECTIVITY_DATA::GetRatsnestForItems( const std::vector<BOARD_ITEM*>& aItems )
{
    std::set<int>                   nets;
    std::vector<CN_EDGE>            edges;
    std::set<BOARD_CONNECTED_ITEM*> item_set;

    for( BOARD_ITEM* item : aItems )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

            for( PAD* pad : footprint->Pads() )
            {
                nets.insert( pad->GetNetCode() );
                item_set.insert( pad );
            }
        }
        else if( item->IsConnected() )
        {
            BOARD_CONNECTED_ITEM* conn_item = static_cast<BOARD_CONNECTED_ITEM*>( item );

            item_set.insert( conn_item );
            nets.insert( conn_item->GetNetCode() );
        }
    }

    for( int netcode : nets )
    {
        RN_NET* net = GetRatsnestForNet( netcode );

        if( !net )
            continue;

        for( const CN_EDGE& edge : net->GetEdges() )
        {
            std::shared_ptr<const CN_ANCHOR> srcNode = edge.GetSourceNode();
            std::shared_ptr<const CN_ANCHOR> dstNode = edge.GetTargetNode();

            if( !srcNode || srcNode->Dirty() || !dstNode || dstNode->Dirty() )
                continue;

            BOARD_CONNECTED_ITEM* srcParent = srcNode->Parent();
            BOARD_CONNECTED_ITEM* dstParent = dstNode->Parent();

            bool srcFound = ( item_set.find( srcParent ) != item_set.end() );
            bool dstFound = ( item_set.find( dstParent ) != item_set.end() );

            if ( srcFound && dstFound )
                edges.push_back( edge );
        }
    }

    return edges;
}


const std::vector<CN_EDGE> CONNECTIVITY_DATA::GetRatsnestForPad( const PAD* aPad )
{
    std::vector<CN_EDGE> edges;
    RN_NET* net = GetRatsnestForNet( aPad->GetNetCode() );

    if( !net )
        return edges;

    for( const CN_EDGE& edge : net->GetEdges() )
    {
        if( !edge.GetSourceNode() || edge.GetSourceNode()->Dirty() )
            continue;

        if( !edge.GetTargetNode() || edge.GetTargetNode()->Dirty() )
            continue;

        if( edge.GetSourceNode()->Parent() == aPad || edge.GetTargetNode()->Parent() == aPad )
            edges.push_back( edge );
    }

    return edges;
}


const std::vector<CN_EDGE> CONNECTIVITY_DATA::GetRatsnestForComponent( FOOTPRINT* aComponent,
                                                                       bool aSkipInternalConnections )
{
    std::set<int> nets;
    std::set<const PAD*> pads;
    std::vector<CN_EDGE> edges;

    for( PAD* pad : aComponent->Pads() )
    {
        nets.insert( pad->GetNetCode() );
        pads.insert( pad );
    }

    for( int netcode : nets )
    {
        RN_NET* net = GetRatsnestForNet( netcode );

        if( !net )
            continue;

        for( const CN_EDGE& edge : net->GetEdges() )
        {
            const std::shared_ptr<const CN_ANCHOR>& srcNode = edge.GetSourceNode();
            const std::shared_ptr<const CN_ANCHOR>& dstNode = edge.GetTargetNode();

            if( !srcNode || srcNode->Dirty() || !dstNode || dstNode->Dirty() )
                continue;

            const PAD* srcParent = static_cast<const PAD*>( srcNode->Parent() );
            const PAD* dstParent = static_cast<const PAD*>( dstNode->Parent() );

            bool srcFound = ( pads.find(srcParent) != pads.end() );
            bool dstFound = ( pads.find(dstParent) != pads.end() );

            if ( srcFound && dstFound && !aSkipInternalConnections )
                edges.push_back( edge );
            else if ( srcFound || dstFound )
                edges.push_back( edge );
        }
    }

    return edges;
}


const NET_SETTINGS* CONNECTIVITY_DATA::GetNetSettings() const
{
    if( std::shared_ptr<NET_SETTINGS> netSettings = m_netSettings.lock() )
        return netSettings.get();
    else
        return nullptr;
}
