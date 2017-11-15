/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
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

#include <connectivity_data.h>
#include <connectivity_algo.h>
#include <ratsnest_data.h>

#ifdef USE_OPENMP
#include <omp.h>
#endif /* USE_OPENMP */

CONNECTIVITY_DATA::CONNECTIVITY_DATA()
{
    m_connAlgo.reset( new CN_CONNECTIVITY_ALGO );
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
    int lastNet = m_connAlgo->NetCount();

    #ifdef PROFILE
    PROF_COUNTER rnUpdate( "update-ratsnest" );
    #endif

    int nDirty = 0;

    int i;

    #ifdef USE_OPENMP
        #pragma omp parallel shared(lastNet) private(i)
    {
        #pragma omp for schedule(guided, 1)
    #else /* USE_OPENMP */
    {
    #endif

        // Start with net number 1, as 0 stands for not connected
        for( i = 1; i < lastNet; ++i )
        {
            if( m_nets[i]->IsDirty() )
            {
                m_nets[i]->Update();
                nDirty++;
            }
        }
    }          /* end of parallel section */

    #ifdef PROFILE
    rnUpdate.Show();
    #endif /* PROFILE */
}


void CONNECTIVITY_DATA::addRatsnestCluster( std::shared_ptr<CN_CLUSTER> aCluster )
{
    auto rnNet = m_nets[ aCluster->OriginNet() ];

    rnNet->AddCluster( aCluster );
}


void CONNECTIVITY_DATA::RecalculateRatsnest()
{
    m_connAlgo->PropagateNets();

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

    for( auto c : clusters )
    {
        int net = c->OriginNet();

        if( m_connAlgo->IsNetDirty( net ) )
        {
            addRatsnestCluster( c );
        }
    }

    m_connAlgo->ClearDirtyFlags();

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

    for( auto item : citems )
    {
        if ( m_connAlgo->ItemExists( item ) )
        {
            auto& entry = m_connAlgo->ItemEntry( item );

            for( auto cnItem : entry.GetItems() )
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
    m_connAlgo->FindIsolatedCopperIslands( aZone, aIslands );
}


void CONNECTIVITY_DATA::ComputeDynamicRatsnest( const std::vector<BOARD_ITEM*>& aItems )
{
    m_dynamicConnectivity.reset( new CONNECTIVITY_DATA );
    m_dynamicConnectivity->Build( aItems );

    m_dynamicRatsnest.clear();

    BlockRatsnestItems( aItems );

    for( unsigned int nc = 1; nc < m_dynamicConnectivity->m_nets.size(); nc++ )
    {
        auto dynNet = m_dynamicConnectivity->m_nets[nc];

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

    for( auto net : m_dynamicConnectivity->m_nets )
    {
        if( !net )
            continue;

        const auto& edges = net->GetUnconnected();

        if( edges.empty() )
            continue;

        for( const auto& edge : edges )
        {
            const auto& nodeA   = edge.GetSourceNode();
            const auto& nodeB   = edge.GetTargetNode();
            RN_DYNAMIC_LINE l;

            l.a = nodeA->Pos();
            l.b = nodeB->Pos();
            l.netCode = 0;
            m_dynamicRatsnest.push_back( l );
        }
    }
}


void CONNECTIVITY_DATA::ClearDynamicRatsnest()
{
    m_connAlgo->ForEachAnchor( [] (CN_ANCHOR_PTR anchor ) { anchor->SetNoLine( false ); } );
    HideDynamicRatsnest();
}


void CONNECTIVITY_DATA::HideDynamicRatsnest()
{
    m_dynamicConnectivity.reset();
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
        const KICAD_T aTypes[] ) const
{
    std::vector<BOARD_CONNECTED_ITEM*> rv;
    const auto clusters = m_connAlgo->SearchClusters( CN_CONNECTIVITY_ALGO::CSM_CONNECTIVITY_CHECK,
            aTypes, aItem->GetNetCode() );

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
    std::set<BOARD_CONNECTED_ITEM*> items;
    std::vector<BOARD_CONNECTED_ITEM*> rv;

    m_connAlgo->ForEachItem( [&items, aNetCode, &aTypes] ( CN_ITEM* aItem )
    {
        if( aItem->Valid() && aItem->Net() == aNetCode )
        {
            KICAD_T itemType = aItem->Parent()->Type();

            for( int i = 0; aTypes[i] > 0; ++i )
            {
                wxASSERT( aTypes[i] < MAX_STRUCT_TYPE_ID );

                if( itemType == aTypes[i] )
                {
                    items.insert( aItem->Parent() );
                    break;
                }
            }
        }
    } );

    std::copy( items.begin(), items.end(), std::back_inserter( rv ) );

    return rv;
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
            if( connected->Valid() && ( connected->Parent()->Type() == PCB_TRACE_T || connected->Parent()->Type() == PCB_VIA_T ) )
                tracks.insert( static_cast<TRACK*> ( connected->Parent() ) );
        }
    }

    std::copy( tracks.begin(), tracks.end(), std::back_inserter( rv ) );
    return rv;
}


const std::vector<D_PAD*> CONNECTIVITY_DATA::GetConnectedPads( const BOARD_CONNECTED_ITEM* aItem )
const
{
    auto& entry = m_connAlgo->ItemEntry( aItem );

    std::set<D_PAD*> pads;
    std::vector<D_PAD*> rv;

    for( auto citem : entry.GetItems() )
    {
        for( auto connected : citem->ConnectedItems() )
        {
            if( connected->Valid() && connected->Parent()->Type() == PCB_PAD_T )
                pads.insert( static_cast<D_PAD*> ( connected->Parent() ) );
        }
    }

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

    for( auto pad : m_connAlgo->PadList() )
    {
        if( !pad->Valid() )
            continue;

        auto dpad = static_cast<D_PAD*>( pad->Parent() );

        if( aNet < 0 || aNet == dpad->GetNetCode() )
        {
            n++;
        }
    }

    return n;
}


const std::vector<VECTOR2I> CONNECTIVITY_DATA::NearestUnconnectedTargets(
        const BOARD_CONNECTED_ITEM* aRef,
        const VECTOR2I& aPos,
        int aNet )
{
    CN_CLUSTER_PTR refCluster;
    int refNet = -1;

    if( aRef )
        refNet = aRef->GetNetCode();

    if( aNet >= 0 )
        refNet = aNet;

    if( aRef )
    {
        for( auto cl : m_connAlgo->GetClusters() )
        {
            if( cl->Contains( aRef ) )
            {
                refCluster = cl;
                break;
            }
        }
    }

    std::set <VECTOR2I> anchors;

    for( auto cl : m_connAlgo->GetClusters() )
    {
        if( cl != refCluster )
        {
            for( auto item : *cl )
            {
                if( item->Valid() && item->Parent()->GetNetCode() == refNet
                    && item->Parent()->Type() != PCB_ZONE_AREA_T )
                {
                    for( auto anchor : item->Anchors() )
                    {
                        anchors.insert( anchor->Pos() );
                    }
                }
            }
        }
    }


    std::vector<VECTOR2I> rv;

    std::copy( anchors.begin(), anchors.end(), std::back_inserter( rv ) );
    std::sort( rv.begin(), rv.end(), [aPos] ( const VECTOR2I& a, const VECTOR2I& b )
    {
        auto da = (a - aPos).EuclideanNorm();
        auto db = (b - aPos).EuclideanNorm();

        return da < db;
    } );

    return rv;
}


void CONNECTIVITY_DATA::GetUnconnectedEdges( std::vector<CN_EDGE>& aEdges) const
{
    for( auto rnNet : m_nets )
    {
        if( rnNet )
        {
            for( auto edge : rnNet->GetEdges() )
            {
                aEdges.push_back( edge );
            }
        }
    }
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
