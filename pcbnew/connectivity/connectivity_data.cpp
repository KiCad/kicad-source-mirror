/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <profile.h>
#endif

#include <thread>
#include <algorithm>
#include <future>

#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <ratsnest/ratsnest_data.h>

CONNECTIVITY_DATA::CONNECTIVITY_DATA()
{
    m_connAlgo.reset( new CN_CONNECTIVITY_ALGO );
    m_progressReporter = nullptr;
}


CONNECTIVITY_DATA::CONNECTIVITY_DATA( const std::vector<BOARD_ITEM*>& aItems, bool aSkipRatsnest )
    : m_skipRatsnest( aSkipRatsnest )
{
    Build( aItems );
    m_progressReporter = nullptr;
}


CONNECTIVITY_DATA::~CONNECTIVITY_DATA()
{
    Clear();
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


void CONNECTIVITY_DATA::Build( BOARD* aBoard )
{
    m_connAlgo.reset( new CN_CONNECTIVITY_ALGO );
    m_connAlgo->Build( aBoard );
    RecalculateRatsnest();
}


void CONNECTIVITY_DATA::Build( const std::vector<BOARD_ITEM*>& aItems )
{
    m_connAlgo.reset( new CN_CONNECTIVITY_ALGO );
    m_connAlgo->Build( aItems );

    RecalculateRatsnest();
}


void CONNECTIVITY_DATA::updateRatsnest()
{
    #ifdef PROFILE
    PROF_COUNTER rnUpdate( "update-ratsnest" );
    #endif
    std::vector<RN_NET*> dirty_nets;

    // Start with net 1 as net 0 is reserved for not-connected
    // Nets without nodes are also ignored
    std::copy_if( m_nets.begin() + 1, m_nets.end(), std::back_inserter( dirty_nets ),
            [] ( RN_NET* aNet ) { return aNet->IsDirty() && aNet->GetNodeCount() > 0; } );

    // We don't want to spin up a new thread for fewer than 8 nets (overhead costs)
    size_t parallelThreadCount = std::min<size_t>( std::thread::hardware_concurrency(),
            ( dirty_nets.size() + 7 ) / 8 );

    std::atomic<size_t> nextNet( 0 );
    std::vector<std::future<size_t>> returns( parallelThreadCount );

    auto update_lambda = [&nextNet, &dirty_nets]() -> size_t
    {
        for( size_t i = nextNet++; i < dirty_nets.size(); i = nextNet++ )
            dirty_nets[i]->Update();

        return 1;
    };

    if( parallelThreadCount == 1 )
        update_lambda();
    else
    {
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii] = std::async( std::launch::async, update_lambda );

        // Finalize the ratsnest threads
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii].wait();
    }

    #ifdef PROFILE
    rnUpdate.Show();
    #endif /* PROFILE */
}


void CONNECTIVITY_DATA::addRatsnestCluster( const std::shared_ptr<CN_CLUSTER>& aCluster )
{
    auto rnNet = m_nets[ aCluster->OriginNet() ];

    rnNet->AddCluster( aCluster );
}


void CONNECTIVITY_DATA::RecalculateRatsnest( BOARD_COMMIT* aCommit  )
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

    auto clusters = m_connAlgo->GetClusters();

    int dirtyNets = 0;

    for( int net = 0; net < lastNet; net++ )
    {
        if( m_connAlgo->IsNetDirty( net ) )
        {
            m_nets[net]->Clear();
            dirtyNets++;
        }
    }

    for( const auto& c : clusters )
    {
        int net = c->OriginNet();

        // Don't add intentionally-kept zone islands to the ratsnest
        if( c->IsOrphaned() && c->Size() == 1 )
        {
            if( dynamic_cast<CN_ZONE*>( *c->begin() ) )
                continue;
        }

        if( m_connAlgo->IsNetDirty( net ) )
        {
            addRatsnestCluster( c );
        }
    }

    m_connAlgo->ClearDirtyFlags();

    if( !m_skipRatsnest )
        updateRatsnest();
}


void CONNECTIVITY_DATA::BlockRatsnestItems( const std::vector<BOARD_ITEM*>& aItems )
{
    std::vector<BOARD_CONNECTED_ITEM*> citems;

    for( auto item : aItems )
    {
        if( item->Type() == PCB_MODULE_T )
        {
            for( auto pad : static_cast<MODULE*>(item)->Pads() )
                citems.push_back( pad );
        }
        else
        {
            citems.push_back( static_cast<BOARD_CONNECTED_ITEM*>(item) );
        }
    }

    for( const auto& item : citems )
    {
        if ( m_connAlgo->ItemExists( item ) )
        {
            auto& entry = m_connAlgo->ItemEntry( item );

            for( const auto& cnItem : entry.GetItems() )
            {
                for( auto anchor : cnItem->Anchors() )
                    anchor->SetNoLine( true );
            }
        }
    }
}


int CONNECTIVITY_DATA::GetNetCount() const
{
    return m_connAlgo->NetCount();
}


void CONNECTIVITY_DATA::FindIsolatedCopperIslands( ZONE_CONTAINER* aZone,
        std::vector<int>& aIslands )
{
    // TODO(JE) ZONES
#if 0
    m_connAlgo->FindIsolatedCopperIslands( aZone, aIslands );
#endif
}

void CONNECTIVITY_DATA::FindIsolatedCopperIslands( std::vector<CN_ZONE_ISOLATED_ISLAND_LIST>& aZones )
{
    m_connAlgo->FindIsolatedCopperIslands( aZones );
}


void CONNECTIVITY_DATA::ComputeDynamicRatsnest( const std::vector<BOARD_ITEM*>& aItems )
{
    m_dynamicRatsnest.clear();

    if( std::none_of( aItems.begin(), aItems.end(), []( const BOARD_ITEM* aItem )
            { return( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_PAD_T ||
                      aItem->Type() == PCB_ARC_T || aItem->Type() == PCB_ZONE_AREA_T ||
                      aItem->Type() == PCB_MODULE_T || aItem->Type() == PCB_VIA_T ); } ) )
    {
        return ;
    }

    CONNECTIVITY_DATA connData( aItems, true );
    BlockRatsnestItems( aItems );

    for( unsigned int nc = 1; nc < connData.m_nets.size(); nc++ )
    {
        auto dynNet = connData.m_nets[nc];

        if( dynNet->GetNodeCount() != 0 )
        {
            auto ourNet = m_nets[nc];
            CN_ANCHOR_PTR nodeA, nodeB;

            if( ourNet->NearestBicoloredPair( *dynNet, nodeA, nodeB ) )
            {
                RN_DYNAMIC_LINE l;
                l.a = nodeA->Pos();
                l.b = nodeB->Pos();
                l.netCode = nc;

                m_dynamicRatsnest.push_back( l );
            }
        }
    }

    const auto& edges = GetRatsnestForItems( aItems );

    for( const auto& edge : edges )
    {
        const auto& nodeA   = edge.GetSourceNode();
        const auto& nodeB   = edge.GetTargetNode();
        RN_DYNAMIC_LINE l;

        // Use the parents' positions
        l.a = nodeA->Parent()->GetPosition();
        l.b = nodeB->Parent()->GetPosition();
        l.netCode = 0;
        m_dynamicRatsnest.push_back( l );
    }
}


void CONNECTIVITY_DATA::ClearDynamicRatsnest()
{
    m_connAlgo->ForEachAnchor( [] ( CN_ANCHOR& anchor ) { anchor.SetNoLine( false ); } );
    HideDynamicRatsnest();
}


void CONNECTIVITY_DATA::HideDynamicRatsnest()
{
    m_dynamicRatsnest.clear();
}


void CONNECTIVITY_DATA::PropagateNets()
{
    m_connAlgo->PropagateNets();
}


unsigned int CONNECTIVITY_DATA::GetUnconnectedCount() const
{
    unsigned int unconnected = 0;

    for( auto net : m_nets )
    {
        if( !net )
            continue;

        const auto& edges = net->GetUnconnected();

        if( edges.empty() )
            continue;

        unconnected += edges.size();
    }

    return unconnected;
}


void CONNECTIVITY_DATA::Clear()
{
    for( auto net : m_nets )
        delete net;

    m_nets.clear();
}


const std::vector<BOARD_CONNECTED_ITEM*> CONNECTIVITY_DATA::GetConnectedItems(
        const BOARD_CONNECTED_ITEM* aItem,
        const KICAD_T aTypes[],
        bool aIgnoreNetcodes ) const
{
    std::vector<BOARD_CONNECTED_ITEM*> rv;
    const auto clusters = m_connAlgo->SearchClusters(
            aIgnoreNetcodes ?
                    CN_CONNECTIVITY_ALGO::CSM_PROPAGATE :
                    CN_CONNECTIVITY_ALGO::CSM_CONNECTIVITY_CHECK, aTypes,
            aIgnoreNetcodes ? -1 : aItem->GetNetCode() );

    for( auto cl : clusters )
    {
        if( cl->Contains( aItem ) )
        {
            for( const auto item : *cl )
            {
                if( item->Valid() )
                    rv.push_back( item->Parent() );
            }
        }
    }

    return rv;
}


const std::vector<BOARD_CONNECTED_ITEM*> CONNECTIVITY_DATA::GetNetItems( int aNetCode,
        const KICAD_T aTypes[] ) const
{
    std::vector<BOARD_CONNECTED_ITEM*> items;
    items.reserve( 32 );

    std::bitset<MAX_STRUCT_TYPE_ID> type_bits;

    for( unsigned int i = 0; aTypes[i] != EOT; ++i )
    {
        wxASSERT( aTypes[i] < MAX_STRUCT_TYPE_ID );
        type_bits.set( aTypes[i] );
    }

    m_connAlgo->ForEachItem( [&]( CN_ITEM& aItem ) {
        if( aItem.Valid() && ( aItem.Net() == aNetCode ) && type_bits[aItem.Parent()->Type()] )
            items.push_back( aItem.Parent() );
    } );

    std::sort( items.begin(), items.end() );
    items.erase( std::unique( items.begin(), items.end() ), items.end() );
    return items;
}


bool CONNECTIVITY_DATA::CheckConnectivity( std::vector<CN_DISJOINT_NET_ENTRY>& aReport )
{
    RecalculateRatsnest();

    for( auto net : m_nets )
    {
        if( net )
        {
            for( const auto& edge : net->GetEdges() )
            {
                CN_DISJOINT_NET_ENTRY ent;
                ent.net = edge.GetSourceNode()->Parent()->GetNetCode();
                ent.a   = edge.GetSourceNode()->Parent();
                ent.b   = edge.GetTargetNode()->Parent();
                ent.anchorA = edge.GetSourceNode()->Pos();
                ent.anchorB = edge.GetTargetNode()->Pos();
                aReport.push_back( ent );
            }
        }
    }

    return aReport.empty();
}


const std::vector<TRACK*> CONNECTIVITY_DATA::GetConnectedTracks( const BOARD_CONNECTED_ITEM* aItem )
const
{
    auto& entry = m_connAlgo->ItemEntry( aItem );

    std::set<TRACK*> tracks;
    std::vector<TRACK*> rv;

    for( auto citem : entry.GetItems() )
    {
        for( auto connected : citem->ConnectedItems() )
        {
            if( connected->Valid() &&
                    ( connected->Parent()->Type() == PCB_TRACE_T ||
                            connected->Parent()->Type() == PCB_VIA_T ||
                            connected->Parent()->Type() == PCB_ARC_T ) )
                tracks.insert( static_cast<TRACK*> ( connected->Parent() ) );
        }
    }

    std::copy( tracks.begin(), tracks.end(), std::back_inserter( rv ) );
    return rv;
}


void CONNECTIVITY_DATA::GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem,
                                                std::set<D_PAD*>* pads ) const
{
    for( auto citem : m_connAlgo->ItemEntry( aItem ).GetItems() )
    {
        for( auto connected : citem->ConnectedItems() )
        {
            if( connected->Valid() && connected->Parent()->Type() == PCB_PAD_T )
                pads->insert( static_cast<D_PAD*> ( connected->Parent() ) );
        }
    }
}


const std::vector<D_PAD*> CONNECTIVITY_DATA::GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem )
const
{
    std::set<D_PAD*> pads;
    std::vector<D_PAD*> rv;

    GetConnectedPads( aItem, &pads );

    std::copy( pads.begin(), pads.end(), std::back_inserter( rv ) );
    return rv;
}


unsigned int CONNECTIVITY_DATA::GetNodeCount( int aNet ) const
{
    int sum = 0;

    if( aNet < 0 )      // Node count for all nets
    {
        for( const auto& net : m_nets )
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

    for( auto&& pad : m_connAlgo->ItemList() )
    {
        if( !pad->Valid() || pad->Parent()->Type() != PCB_PAD_T)
            continue;

        auto dpad = static_cast<D_PAD*>( pad->Parent() );

        if( aNet < 0 || aNet == dpad->GetNetCode() )
        {
            n++;
        }
    }

    return n;
}


void CONNECTIVITY_DATA::GetUnconnectedEdges( std::vector<CN_EDGE>& aEdges) const
{
    for( auto rnNet : m_nets )
    {
        if( rnNet )
        {
            for( const auto& edge : rnNet->GetEdges() )
            {
                aEdges.push_back( edge );
            }
        }
    }
}


bool CONNECTIVITY_DATA::TestTrackEndpointDangling( TRACK* aTrack, wxPoint* aPos )
{
    auto items = GetConnectivityAlgo()->ItemEntry( aTrack ).GetItems();

    // Not in the connectivity system.  This is a bug!
    if( items.empty() )
    {
        wxFAIL_MSG( "track not in connectivity system" );
        return false;
    }

    CN_ITEM* citem = items.front();

    if( !citem->Valid() )
        return false;

    for( const std::shared_ptr<CN_ANCHOR>& anchor : citem->Anchors() )
    {
        if( anchor->IsDangling() )
        {
            *aPos = (wxPoint) anchor->Pos();
            return true;
        }
    }

    return false;
}


const std::vector<BOARD_CONNECTED_ITEM*> CONNECTIVITY_DATA::GetConnectedItems(
        const BOARD_CONNECTED_ITEM* aItem, const VECTOR2I& aAnchor, KICAD_T aTypes[] )
{
    auto& entry = m_connAlgo->ItemEntry( aItem );
    std::vector<BOARD_CONNECTED_ITEM* > rv;

    for( auto cnItem : entry.GetItems() )
    {
        for( auto anchor : cnItem->Anchors() )
        {
            if( anchor->Pos() == aAnchor )
            {
                for( int i = 0; aTypes[i] > 0; i++ )
                {
                    if( cnItem->Valid() && cnItem->Parent()->Type() == aTypes[i] )
                    {
                        rv.push_back( cnItem->Parent() );
                        break;
                    }
                }
            }
        }
    }

    return rv;
}


RN_NET* CONNECTIVITY_DATA::GetRatsnestForNet( int aNet )
{
    if ( aNet < 0 || aNet >= (int) m_nets.size() )
    {
        return nullptr;
    }

    return m_nets[ aNet ];
}


void CONNECTIVITY_DATA::MarkItemNetAsDirty( BOARD_ITEM *aItem )
{
    if (aItem->Type() == PCB_MODULE_T)
    {
        for ( auto pad : static_cast<MODULE*>( aItem )->Pads() )
        {
            m_connAlgo->MarkNetAsDirty( pad->GetNetCode() );
        }
    }
    if (aItem->IsConnected() )
    {
        m_connAlgo->MarkNetAsDirty( static_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode() );
    }
}


void CONNECTIVITY_DATA::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
    m_connAlgo->SetProgressReporter( m_progressReporter );
}


const std::vector<CN_EDGE> CONNECTIVITY_DATA::GetRatsnestForItems( std::vector<BOARD_ITEM*> aItems )
{
    std::set<int> nets;
    std::vector<CN_EDGE> edges;
    std::set<BOARD_CONNECTED_ITEM*> item_set;

    for( auto item : aItems )
    {
        auto conn_item = dyn_cast<BOARD_CONNECTED_ITEM*>( item );

        if( !conn_item )
            continue;

        if( item->Type() == PCB_MODULE_T )
        {
            auto component = static_cast<MODULE*>( item );

            for( auto pad : component->Pads() )
            {
                nets.insert( pad->GetNetCode() );
                item_set.insert( pad );
            }
        }
        else
        {
            item_set.insert( conn_item );
            nets.insert( conn_item->GetNetCode() );
        }
    }

    for( const auto& netcode : nets )
    {
        const auto& net = GetRatsnestForNet( netcode );

        for( const auto& edge : net->GetEdges() )
        {
            auto srcNode = edge.GetSourceNode();
            auto dstNode = edge.GetTargetNode();

            auto srcParent = srcNode->Parent();
            auto dstParent = dstNode->Parent();

            bool srcFound = ( item_set.find(srcParent) != item_set.end() );
            bool dstFound = ( item_set.find(dstParent) != item_set.end() );

            if ( srcFound && dstFound )
                edges.push_back( edge );
        }
    }

    return edges;
}


const std::vector<CN_EDGE> CONNECTIVITY_DATA::GetRatsnestForComponent( MODULE* aComponent, bool aSkipInternalConnections )
{
    std::set<int> nets;
    std::set<const D_PAD*> pads;
    std::vector<CN_EDGE> edges;

    for( auto pad : aComponent->Pads() )
    {
        nets.insert( pad->GetNetCode() );
        pads.insert( pad );
    }

    for( const auto& netcode : nets )
    {
        const auto& net = GetRatsnestForNet( netcode );

        for( const auto& edge : net->GetEdges() )
        {
            auto srcNode = edge.GetSourceNode();
            auto dstNode = edge.GetTargetNode();

            auto srcParent = static_cast<const D_PAD*>( srcNode->Parent() );
            auto dstParent = static_cast<const D_PAD*>( dstNode->Parent() );

            bool srcFound = ( pads.find(srcParent) != pads.end() );
            bool dstFound = ( pads.find(dstParent) != pads.end() );

            if ( srcFound && dstFound && !aSkipInternalConnections )
            {
                edges.push_back( edge );
            }
            else if ( srcFound || dstFound )
            {
                edges.push_back( edge );
            }
        }
    }

    return edges;
}


