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
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <wx/app.h>

DRC_CUSTOM_MARKER_HANDLER
DRC_TEST_PROVIDER_CLEARANCE_BASE::GetGraphicsHandler( const std::vector<PCB_SHAPE>& aShapes,
                                                      const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                                      int aLength )
{
    // todo: Move this to a board-level object instead of getting it from the DRC Engine
    COLOR4D   errorColor = COLOR4D( RED );

    if( PgmOrNull() )
    {
        COLOR_SETTINGS* colorSettings = ::GetColorSettings( DEFAULT_THEME );
        errorColor = colorSettings->GetColor( LAYER_DRC_ERROR );
    }

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
        DRC_CUSTOM_MARKER_HANDLER handler = GetGraphicsHandler( { ptAShape }, ptA, ptB, aDistance );
        reportViolation( aDrce, aMarkerPos, aMarkerLayer, &handler );
    }
    else
    {
        reportViolation( aDrce, aMarkerPos, aMarkerLayer );
    }
}
