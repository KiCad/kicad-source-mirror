/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/app.h>


std::vector<PCB_SHAPE> DRC_TEST_PROVIDER_CLEARANCE_BASE::GetShapes( const std::vector<PCB_SHAPE>& aShapes,
                                                                    const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                                                    int aLength )
{
    STROKE_PARAMS          hairline( 1.0 );     // Segments of width 1.0 will get drawn as lines by PCB_PAINTER
    std::vector<PCB_SHAPE> shortestPathShapes;

    // Add the path
    for( PCB_SHAPE shape : aShapes )
    {
        shape.SetStroke( hairline );
        shortestPathShapes.push_back( std::move( shape ) );
    }

    // Draw perpendicular begin/end stops
    if( shortestPathShapes.size() > 0 )
    {
        VECTOR2I V1 = shortestPathShapes[0].GetStart() - shortestPathShapes[0].GetEnd();
        VECTOR2I V2 = shortestPathShapes.back().GetStart() - shortestPathShapes.back().GetEnd();
        V1 = V1.Perpendicular().Resize( std::max( aLength / 24, pcbIUScale.mmToIU( 0.05 ) ) );
        V2 = V2.Perpendicular().Resize( std::max( aLength / 24, pcbIUScale.mmToIU( 0.05 ) ) );

        PCB_SHAPE s( nullptr, SHAPE_T::SEGMENT );
        s.SetStroke( hairline );

        s.SetStart( aStart + V1 );
        s.SetEnd( aStart - V1 );
        shortestPathShapes.push_back( s );

        s.SetStart( aEnd + V2 );
        s.SetEnd( aEnd - V2 );
        shortestPathShapes.push_back( s );
    }

    // Add shaded areas
    for( PCB_SHAPE shape : aShapes )
    {
        shape.SetWidth( std::max( aLength / 10, pcbIUScale.mmToIU( 0.2 ) ) );
        shortestPathShapes.push_back( std::move( shape ) );
    }

    return shortestPathShapes;
}


void DRC_TEST_PROVIDER_CLEARANCE_BASE::ReportAndShowPathCuToCu( std::shared_ptr<DRC_ITEM>& aDrce,
                                                                const VECTOR2I& aMarkerPos,
                                                                int aMarkerLayer,
                                                                const BOARD_ITEM* aItem1,
                                                                const BOARD_ITEM* aItem2,
                                                                PCB_LAYER_ID layer, int aDistance )
{
    std::shared_ptr<SHAPE> aShape1 = aItem1->GetEffectiveShape( layer );
    std::shared_ptr<SHAPE> aShape2 = aItem2->GetEffectiveShape( layer );

    VECTOR2I ptA, ptB;

    // Don't try showing graphics if we don't have a GUI instance
    if( wxApp::GetGUIInstance() && aShape1->NearestPoints( aShape2.get(), ptA, ptB ) )
    {
        PCB_SHAPE ptAShape( nullptr, SHAPE_T::SEGMENT );
        ptAShape.SetStart( ptA );
        ptAShape.SetEnd( ptB );
        reportViolation( aDrce, aMarkerPos, aMarkerLayer, GetShapes( { ptAShape }, ptA, ptB, aDistance ) );
    }
    else
    {
        reportViolation( aDrce, aMarkerPos, aMarkerLayer );
    }
}
