/*
    * Copyright The KiCad Developers.
    * Copyright (C) 2024 Fabien Corona f.corona<at>laposte.net
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

#include <common.h>
#include <macros.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <zone.h>
#include <advanced_config.h>
#include <geometry/shape_rect.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_creepage_utils.h>

#include <geometry/shape_circle.h>


/*
    Physical creepage tests.

    Errors generated:
    - DRCE_CREEPAGE
*/

class DRC_TEST_PROVIDER_CREEPAGE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_CREEPAGE() :
            DRC_TEST_PROVIDER()
    {}

    virtual ~DRC_TEST_PROVIDER_CREEPAGE() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "creepage" ); };

    double GetMaxConstraint( const std::vector<int>& aNetCodes );

private:
    int testCreepage();
    int testCreepage( CREEPAGE_GRAPH& aGraph, int aNetCodeA, int aNetCodeB, PCB_LAYER_ID aLayer );

    void CollectBoardEdges( std::vector<BOARD_ITEM*>& aVector,
                            std::vector<std::unique_ptr<PCB_SHAPE>>& aOwned );
    void CollectNetCodes( std::vector<int>& aVector );

    std::set<std::pair<const BOARD_ITEM*, const BOARD_ITEM*>> m_reportedPairs;
};


bool DRC_TEST_PROVIDER_CREEPAGE::Run()
{
    m_board = m_drcEngine->GetBoard();
    m_reportedPairs.clear();

    if( !m_drcEngine->HasRulesForConstraintType( CREEPAGE_CONSTRAINT ) )
    {
        REPORT_AUX( wxT( "No creepage constraints found. Tests not run." ) );
        return true;    // continue with other tests
    }

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_CREEPAGE ) )
    {
        if( !reportPhase( _( "Checking creepage..." ) ) )
            return false; // DRC cancelled

        testCreepage();
    }

    return !m_drcEngine->IsCancelled();
}


int DRC_TEST_PROVIDER_CREEPAGE::testCreepage( CREEPAGE_GRAPH& aGraph, int aNetCodeA, int aNetCodeB,
                                              PCB_LAYER_ID aLayer )
{
    PCB_TRACK bci1( m_board );
    PCB_TRACK bci2( m_board );
    bci1.SetNetCode( aNetCodeA );
    bci2.SetNetCode( aNetCodeB );
    bci1.SetLayer( aLayer );
    bci2.SetLayer( aLayer );

    DRC_CONSTRAINT constraint;
    constraint = m_drcEngine->EvalRules( CREEPAGE_CONSTRAINT, &bci1, &bci2, aLayer );
    double creepageValue = constraint.Value().Min();
    aGraph.SetTarget( creepageValue );

    if( creepageValue <= 0 )
        return 0;

    // Let's make a quick "clearance test"
    NETINFO_ITEM* netA = m_board->FindNet( aNetCodeA );
    NETINFO_ITEM* netB = m_board->FindNet( aNetCodeB );

    if ( !netA || !netB )
        return 0;

    if ( netA->GetBoundingBox().Distance( netB->GetBoundingBox() ) > creepageValue )
        return 0;

    std::shared_ptr<GRAPH_NODE> NetA = aGraph.AddNetElements( aNetCodeA, aLayer, creepageValue );
    std::shared_ptr<GRAPH_NODE> NetB = aGraph.AddNetElements( aNetCodeB, aLayer, creepageValue );

    aGraph.GeneratePaths( creepageValue, aLayer );

    std::vector<std::shared_ptr<GRAPH_NODE>> temp_nodes;

    std::copy_if( aGraph.m_nodes.begin(), aGraph.m_nodes.end(), std::back_inserter( temp_nodes ),
                  []( std::shared_ptr<GRAPH_NODE> aNode )
                  {
                      return !!aNode && aNode->m_parent && !aNode->m_parent->IsConductive()
                             && !aNode->m_connectDirectly && aNode->m_type == GRAPH_NODE::POINT;
                  } );

    alg::for_all_pairs( temp_nodes.begin(), temp_nodes.end(),
                        [&]( std::shared_ptr<GRAPH_NODE> aN1, std::shared_ptr<GRAPH_NODE> aN2 )
                        {
                            if( aN1 == aN2 )
                                return;

                            if( !aN1 || !aN2 )
                                return;

                            if( !( aN1->m_parent ) || !( aN2->m_parent ) )
                                return;

                            if( ( aN1->m_parent ) != ( aN2->m_parent ) )
                                return;

                            aN1->m_parent->ConnectChildren( aN1, aN2, aGraph );
                        } );

    std::vector<std::shared_ptr<GRAPH_CONNECTION>> shortestPath;
    shortestPath.clear();
    double distance = aGraph.Solve( NetA, NetB, shortestPath );

    if( !shortestPath.empty() && ( shortestPath.size() >= 4 ) && ( distance - creepageValue < 0 ) )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CREEPAGE );
        drcItem->SetErrorDetail( formatMsg( _( "(%s creepage %s; actual %s)" ),
                                            constraint.GetName(),
                                            creepageValue,
                                            distance ) );
        drcItem->SetViolatingRule( constraint.GetParentRule() );

        std::shared_ptr<GRAPH_CONNECTION> gc1 = shortestPath[1];
        std::shared_ptr<GRAPH_CONNECTION> gc2 = shortestPath[shortestPath.size() - 2];

        if( gc1->n1 && gc2->n2 )
        {
            const BOARD_ITEM* item1 = gc1->n1->m_parent->GetParent();
            const BOARD_ITEM* item2 = gc2->n2->m_parent->GetParent();

            if( m_reportedPairs.insert( std::make_pair( item1, item2 ) ).second )
                drcItem->SetItems( item1, item2 );
            else
                return 1;
        }

        VECTOR2I               startPoint = gc1->m_path.a2;
        VECTOR2I               endPoint = gc2->m_path.a2;
        std::vector<PCB_SHAPE> path;

        for( const std::shared_ptr<GRAPH_CONNECTION>& gc : shortestPath )
            gc->GetShapes( path );

        reportViolation( drcItem, gc1->m_path.a2, aLayer,
                         [&]( PCB_MARKER* aMarker )
                         {
                             aMarker->SetPath( path, startPoint, endPoint );
                         } );
    }

    return 1;
}


double DRC_TEST_PROVIDER_CREEPAGE::GetMaxConstraint( const std::vector<int>& aNetCodes )
{
    double         maxConstraint = 0;
    DRC_CONSTRAINT constraint;

    PCB_TRACK bci1( m_board );
    PCB_TRACK bci2( m_board );

    alg::for_all_pairs( aNetCodes.begin(), aNetCodes.end(),
            [&]( int aNet1, int aNet2 )
            {
                if( aNet1 == aNet2 )
                    return;

                bci1.SetNetCode( aNet1 );
                bci2.SetNetCode( aNet2 );

                for( PCB_LAYER_ID layer : LSET::AllCuMask( m_board->GetCopperLayerCount() ) )
                {
                    bci1.SetLayer( layer );
                    bci2.SetLayer( layer );
                    constraint = m_drcEngine->EvalRules( CREEPAGE_CONSTRAINT, &bci1, &bci2, layer );
                    double value = constraint.Value().Min();
                    maxConstraint = value > maxConstraint ? value : maxConstraint;
                }
            } );

    return maxConstraint;
}


void DRC_TEST_PROVIDER_CREEPAGE::CollectNetCodes( std::vector<int>& aVector )
{
    NETCODES_MAP nets = m_board->GetNetInfo().NetsByNetcode();

    for( auto it = nets.begin(); it != nets.end(); it++ )
        aVector.push_back( it->first );
}


void DRC_TEST_PROVIDER_CREEPAGE::CollectBoardEdges( std::vector<BOARD_ITEM*>& aVector,
                                                    std::vector<std::unique_ptr<PCB_SHAPE>>& aOwned )
{
    if( !m_board )
        return;

    for( BOARD_ITEM* drawing : m_board->Drawings() )
    {
        if( !drawing )
            continue;

        if( drawing->IsOnLayer( Edge_Cuts ) )
            aVector.push_back( drawing );
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        if( !fp )
            continue;

        for( BOARD_ITEM* drawing : fp->GraphicalItems() )
        {
            if( !drawing )
                continue;

            if( drawing->IsOnLayer( Edge_Cuts ) )
                aVector.push_back( drawing );
        }
    }

    for( const PAD* p : m_board->GetPads() )
    {
        if( !p )
            continue;

        if( p->GetAttribute() != PAD_ATTRIB::NPTH )
            continue;

        std::shared_ptr<SHAPE_SEGMENT> hole = p->GetEffectiveHoleShape();

        if( !hole )
            continue;

        VECTOR2I ptA = hole->GetSeg().A;
        VECTOR2I ptB = hole->GetSeg().B;
        int      radius = hole->GetWidth() / 2;

        if( ptA == ptB )
        {
            // Circular hole: add as a single circle.
            auto s = std::make_unique<PCB_SHAPE>( nullptr, SHAPE_T::CIRCLE );
            s->SetRadius( radius );
            s->SetPosition( ptA );
            aVector.push_back( s.get() );
            aOwned.push_back( std::move( s ) );
        }
        else
        {
            // Oblong slot: add the two semicircular end caps and two straight sides.
            // The slot outline is the border that creepage paths must not cross.
            VECTOR2I axis = ptB - ptA;
            VECTOR2I perp = axis.Perpendicular().Resize( radius );

            // Side segments connecting the two end caps.
            auto seg1 = std::make_unique<PCB_SHAPE>( nullptr, SHAPE_T::SEGMENT );
            seg1->SetStart( ptA + perp );
            seg1->SetEnd( ptB + perp );
            aVector.push_back( seg1.get() );
            aOwned.push_back( std::move( seg1 ) );

            auto seg2 = std::make_unique<PCB_SHAPE>( nullptr, SHAPE_T::SEGMENT );
            seg2->SetStart( ptA - perp );
            seg2->SetEnd( ptB - perp );
            aVector.push_back( seg2.get() );
            aOwned.push_back( std::move( seg2 ) );

            // Semicircular arc at ptA end (180 degrees, away from ptB).
            VECTOR2I midA = ptA - axis.Resize( radius );
            auto     arcA = std::make_unique<PCB_SHAPE>( nullptr, SHAPE_T::ARC );
            arcA->SetArcGeometry( ptA + perp, midA, ptA - perp );
            aVector.push_back( arcA.get() );
            aOwned.push_back( std::move( arcA ) );

            // Semicircular arc at ptB end (180 degrees, away from ptA).
            VECTOR2I midB = ptB + axis.Resize( radius );
            auto     arcB = std::make_unique<PCB_SHAPE>( nullptr, SHAPE_T::ARC );
            arcB->SetArcGeometry( ptB - perp, midB, ptB + perp );
            aVector.push_back( arcB.get() );
            aOwned.push_back( std::move( arcB ) );
        }
    }
}


int DRC_TEST_PROVIDER_CREEPAGE::testCreepage()
{
    if( !m_board )
        return -1;

    std::vector<int> netcodes;

    this->CollectNetCodes( netcodes );
    double maxConstraint = GetMaxConstraint( netcodes );

    if( maxConstraint <= 0 )
        return 0;

    SHAPE_POLY_SET outline;

    if( !m_board->GetBoardPolygonOutlines( outline, false ) )
        return -1;

    const DRAWINGS drawings = m_board->Drawings();
    CREEPAGE_GRAPH graph( *m_board );

    if( ADVANCED_CFG::GetCfg().m_EnableCreepageSlot )
        graph.m_minGrooveWidth = m_board->GetDesignSettings().m_MinGrooveWidth;
    else
        graph.m_minGrooveWidth = 0;

    graph.m_boardOutline = &outline;

    this->CollectBoardEdges( graph.m_boardEdge, graph.m_ownedBoardEdges );
    graph.TransformEdgeToCreepShapes();
    graph.RemoveDuplicatedShapes();
    graph.TransformCreepShapesToNodes( graph.m_shapeCollection );

    graph.GeneratePaths( maxConstraint, Edge_Cuts );

    int  beNodeSize = graph.m_nodes.size();
    int  beConnectionsSize = graph.m_connections.size();
    bool prevTestChangedGraph = false;

    size_t current = 0;
    size_t total = ( netcodes.size() * ( netcodes.size() - 1 ) ) / 2 * m_board->GetCopperLayerCount();
    LSET layers = m_board->GetLayerSet();

    alg::for_all_pairs( netcodes.begin(), netcodes.end(),
                        [&]( int aNet1, int aNet2 )
                        {
                            if( aNet1 == aNet2 )
                                return;

                            for( auto it = layers.copper_layers_begin(); it != layers.copper_layers_end(); ++it )
                            {
                                PCB_LAYER_ID layer = *it;

                                reportProgress( current++, total );

                                if( prevTestChangedGraph )
                                {
                                    size_t vectorSize = graph.m_connections.size();

                                    for( size_t i = beConnectionsSize; i < vectorSize; i++ )
                                    {
                                        // We need to remove the connection from its endpoints' lists.
                                        graph.RemoveConnection( graph.m_connections[i], false );
                                    }

                                    graph.m_connections.resize( beConnectionsSize, nullptr );

                                    vectorSize = graph.m_nodes.size();
                                    graph.m_nodes.resize( beNodeSize, nullptr );

                                    // Rebuild m_nodeset to match the surviving board-edge
                                    // prefix.  Without this, stale per-net nodes from the
                                    // previous iteration remain in the set and corrupt
                                    // subsequent FindNode/AddNode lookups.
                                    graph.m_nodeset.clear();

                                    for( int i = 0; i < beNodeSize; ++i )
                                    {
                                        if( graph.m_nodes[i] )
                                            graph.m_nodeset.insert( graph.m_nodes[i] );
                                    }
                                }

                                prevTestChangedGraph = testCreepage( graph, aNet1, aNet2, layer );
                            }
                        } );

    return 1;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_CREEPAGE> dummy;
}
