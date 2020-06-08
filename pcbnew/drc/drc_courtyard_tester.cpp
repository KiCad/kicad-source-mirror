/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <drc/drc_courtyard_tester.h>

#include <class_module.h>
#include <drc/drc.h>

#include <widgets/ui_common.h>

#include <memory>


DRC_COURTYARD_TESTER::DRC_COURTYARD_TESTER( MARKER_HANDLER aMarkerHandler ) :
        DRC_TEST_PROVIDER( std::move( aMarkerHandler ) )
{
}


bool DRC_COURTYARD_TESTER::RunDRC( EDA_UNITS aUnits, BOARD& aBoard )
{
    // Detects missing (or malformed) footprint courtyards and courtyard incursions (for those
    // with a courtyard).
    wxString msg;
    bool     success = true;

    // Update courtyard polygons, and test for missing courtyard definition:
    for( MODULE* footprint : aBoard.Modules() )
    {
        if( footprint->BuildPolyCourtyard() )
        {
            if( !aBoard.GetDesignSettings().Ignore( DRCE_MISSING_COURTYARD )
                    && footprint->GetPolyCourtyardFront().OutlineCount() == 0
                    && footprint->GetPolyCourtyardBack().OutlineCount() == 0 )
            {
                DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_MISSING_COURTYARD );
                drcItem->SetItems( footprint );
                HandleMarker( new MARKER_PCB( drcItem, footprint->GetPosition() ) );
                success = false;
            }
            else
            {
                footprint->GetPolyCourtyardFront().BuildBBoxCaches();
                footprint->GetPolyCourtyardBack().BuildBBoxCaches();
            }
        }
        else
        {
            if( !aBoard.GetDesignSettings().Ignore( DRCE_MALFORMED_COURTYARD ) )
            {
                DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_MALFORMED_COURTYARD );

                msg.Printf( drcItem->GetErrorText() + _( " (not a closed shape)" ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( footprint );
                HandleMarker( new MARKER_PCB( drcItem, footprint->GetPosition() ) );
                success = false;
            }
        }
    }

    if( !aBoard.GetDesignSettings().Ignore( DRCE_OVERLAPPING_FOOTPRINTS ) )
    {
        for( auto it1 = aBoard.Modules().begin(); it1 != aBoard.Modules().end(); it1++ )
        {
            MODULE*         footprint = *it1;
            SHAPE_POLY_SET& footprintFront = footprint->GetPolyCourtyardFront();
            SHAPE_POLY_SET& footprintBack = footprint->GetPolyCourtyardBack();

            if( footprintFront.OutlineCount() == 0 && footprintBack.OutlineCount() == 0 )
                continue; // No courtyards defined

            for( auto it2 = it1 + 1; it2 != aBoard.Modules().end(); it2++ )
            {
                MODULE*         test = *it2;
                SHAPE_POLY_SET& testFront = test->GetPolyCourtyardFront();
                SHAPE_POLY_SET& testBack = test->GetPolyCourtyardBack();
                SHAPE_POLY_SET  intersection;
                bool            overlap = false;
                wxPoint         pos;

                if( footprintFront.OutlineCount() > 0 && testFront.OutlineCount() > 0
                    && footprintFront.BBoxFromCaches().Intersects( testFront.BBoxFromCaches() ) )
                {
                    intersection.RemoveAllContours();
                    intersection.Append( footprintFront );

                    // Build the common area between footprint and the test:
                    intersection.BooleanIntersection( testFront, SHAPE_POLY_SET::PM_FAST );

                    // If the intersection exists then they overlap
                    if( intersection.OutlineCount() > 0 )
                    {
                        overlap = true;
                        pos = (wxPoint) intersection.CVertex( 0, 0, -1 );
                    }
                }

                if( footprintBack.OutlineCount() > 0 && testBack.OutlineCount() > 0
                    && footprintBack.BBoxFromCaches().Intersects( testBack.BBoxFromCaches() ) )
                {
                    intersection.RemoveAllContours();
                    intersection.Append( footprintBack );

                    intersection.BooleanIntersection( testBack, SHAPE_POLY_SET::PM_FAST );

                    if( intersection.OutlineCount() > 0 )
                    {
                        overlap = true;
                        pos = (wxPoint) intersection.CVertex( 0, 0, -1 );
                    }
                }

                if( overlap )
                {
                    DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_OVERLAPPING_FOOTPRINTS );
                    drcItem->SetItems( footprint, test );
                    HandleMarker( new MARKER_PCB( drcItem, pos ) );
                    success = false;
                }
            }
        }
    }

    if( !aBoard.GetDesignSettings().Ignore( DRCE_PTH_IN_COURTYARD )
            || !aBoard.GetDesignSettings().Ignore( DRCE_NPTH_IN_COURTYARD ) )
    {
        for( MODULE* footprint : aBoard.Modules() )
        {
            SHAPE_POLY_SET& footprintFront = footprint->GetPolyCourtyardFront();
            SHAPE_POLY_SET& footprintBack = footprint->GetPolyCourtyardBack();

            if( footprintFront.OutlineCount() == 0 && footprintBack.OutlineCount() == 0 )
                continue; // No courtyards defined

            for( MODULE* candidate : aBoard.Modules() )
            {
                if( footprint == candidate )
                    continue;

                for( D_PAD* pad : candidate->Pads() )
                {
                    if( pad->GetDrillSize().x == 0 || pad->GetDrillSize().y == 0 )
                        continue;

                    wxPoint pos = pad->GetPosition();

                    if( footprintFront.Contains( pos, -1, 0, true /* use bbox caches */ )
                            || footprintBack.Contains( pos, -1, 0, true /* use bbox caches */ ) )
                    {
                        int code = pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED ?
                                                                       DRCE_NPTH_IN_COURTYARD :
                                                                       DRCE_PTH_IN_COURTYARD;
                        DRC_ITEM* drcItem = DRC_ITEM::Create( code );
                        drcItem->SetItems( footprint, pad );
                        HandleMarker( new MARKER_PCB( drcItem, pos ) );
                        success = false;
                    }
                }
            }
        }
    }

    return success;
}
