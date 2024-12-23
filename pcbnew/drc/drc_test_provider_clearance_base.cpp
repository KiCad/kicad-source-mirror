/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2019-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <drc/drc_test_provider_clearance_base.h>

DRC_CUSTOM_MARKER_HANDLER
DRC_TEST_PROVIDER_CLEARANCE_BASE::GetGraphicsHandler( const std::vector<PCB_SHAPE>& aShapes,
                                                      const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                                      int aLength )
{
    COLOR_SETTINGS* colorSettings = new COLOR_SETTINGS( COLOR_SETTINGS::COLOR_BUILTIN_DEFAULT );
    COLOR_SETTINGS* defaultSettings = colorSettings->CreateBuiltinColorSettings()[0];
    COLOR4D         errorColor = defaultSettings->GetColor( LAYER_DRC_ERROR );
    delete colorSettings;

    std::vector<PCB_SHAPE> shortestPathShapes1, shortestPathShapes2;

    // Add the path and its outlined area
    for( PCB_SHAPE sh : aShapes )
    {
        sh.SetStroke( false );
        sh.SetFilled( false );
        sh.SetLineColor( WHITE );
        shortestPathShapes1.push_back( sh );

        sh.SetFilled( true );
        sh.SetFillColor( errorColor.WithAlpha( 0.5 ) );
        sh.SetWidth( aLength / 10 );
        shortestPathShapes2.push_back( sh );
    }

    if( shortestPathShapes1.size() > 0 )
    {
        PCB_SHAPE s1, s2;
        s1.SetFilled( false );
        s2.SetFilled( false );
        VECTOR2I V1 = shortestPathShapes1[0].GetStart() - shortestPathShapes1[0].GetEnd();
        V1 = V1.Perpendicular().Resize( aLength / 30 );

        s1.SetStart( aStart + V1 );
        s1.SetEnd( aStart - V1 );
        s1.SetWidth( 0 );
        s1.SetLineColor( WHITE );


        VECTOR2I V2 = shortestPathShapes1.back().GetStart() - shortestPathShapes1.back().GetEnd();
        V2 = V2.Perpendicular().Resize( aLength / 30 );

        s2.SetStart( aEnd + V2 );
        s2.SetEnd( aEnd - V2 );
        s2.SetWidth( 0 );
        s2.SetLineColor( WHITE );

        shortestPathShapes1.push_back( s1 );
        shortestPathShapes1.push_back( s2 );
    }

    return [shortestPathShapes1, shortestPathShapes2]( PCB_MARKER* aMarker )
    {
        if( !aMarker )
            return;

        aMarker->SetShapes1( std::move( shortestPathShapes1 ) );
        aMarker->SetShapes2( std::move( shortestPathShapes2 ) );
    };
}


void DRC_TEST_PROVIDER_CLEARANCE_BASE::ReportAndShowPathCuToCu(
        std::shared_ptr<DRC_ITEM>& aDrce, const VECTOR2I& aMarkerPos, int aMarkerLayer,
        const BOARD_ITEM* aItem1, const BOARD_ITEM* aItem2, PCB_LAYER_ID layer, int aDistance )
{
    CreepageGraph graph( *m_board );
    std::shared_ptr<GraphNode> NetA = graph.AddNodeVirtual();
    std::shared_ptr<GraphNode> NetB = graph.AddNodeVirtual();

    // They need to be different or the algorithm won't compute the path.
    NetA->m_net = 1;
    NetB->m_net = 2;

    graph.Addshape( *( aItem1->GetEffectiveShape( layer ) ), NetA, nullptr );
    graph.Addshape( *( aItem2->GetEffectiveShape( layer ) ), NetB, nullptr );

    graph.GeneratePaths( aDistance * 2, layer );

    double           minValue = aDistance * 2;
    GraphConnection* minGc = nullptr;

    for( std::shared_ptr<GraphConnection> gc : graph.m_connections )
    {
        if( ( gc->m_path.weight < minValue ) && ( gc->m_path.weight > 0 ) )
        {
            minValue = gc->m_path.weight;
            minGc = gc.get();
        }
    }

    if( minGc )
    {
        PATH_CONNECTION pc = minGc->m_path;
        DRC_CUSTOM_MARKER_HANDLER handler =
                GetGraphicsHandler( minGc->GetShapes(), pc.a1, pc.a2, aDistance );
        reportViolation( aDrce, aMarkerPos, aMarkerLayer, &handler );
    }
    else
    {
        reportViolation( aDrce, aMarkerPos, aMarkerLayer );
    }
}
