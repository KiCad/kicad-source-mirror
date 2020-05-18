/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <drc/drc_keepout_tester.h>

#include <class_module.h>
#include <drc/drc.h>


DRC_KEEPOUT_TESTER::DRC_KEEPOUT_TESTER( MARKER_HANDLER aMarkerHandler ) :
        DRC_TEST_PROVIDER( std::move( aMarkerHandler ) ),
        m_units( EDA_UNITS::MILLIMETRES ),
        m_board( nullptr )
{
}


bool DRC_KEEPOUT_TESTER::RunDRC( EDA_UNITS aUnits, BOARD& aBoard )
{
    bool success = true;

    m_units = aUnits;
    m_board = &aBoard;

    // Get a list of all zones to inspect, from both board and footprints
    std::list<ZONE_CONTAINER*> areasToInspect = m_board->GetZoneList( true );

    // Test keepout areas for vias, tracks and pads inside keepout areas
    for( ZONE_CONTAINER* area : areasToInspect )
    {
        if( area->GetIsKeepout() )
        {
            if( area->GetDoNotAllowTracks() || area->GetDoNotAllowVias() )
                success &= checkTracksAndVias( area );

            if( area->GetDoNotAllowPads() || area->GetDoNotAllowFootprints() )
                success &= checkFootprints( area );
        }
    }

    return success;
}


bool DRC_KEEPOUT_TESTER::checkTracksAndVias( ZONE_CONTAINER* aKeepout )
{
    bool success = true;

    for( TRACK* segm : m_board->Tracks() )
    {
        if( segm->Type() == PCB_TRACE_T && aKeepout->GetDoNotAllowTracks() )
        {
            // Ignore if the keepout zone is not on the same layer
            if( !aKeepout->IsOnLayer( segm->GetLayer() ) )
                continue;

            int         widths = segm->GetWidth() / 2;
            SEG         trackSeg( segm->GetStart(), segm->GetEnd() );
            SEG::ecoord center2center_squared = aKeepout->Outline()->SquaredDistance( trackSeg );

            if( center2center_squared <= SEG::Square( widths) )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACK_INSIDE_KEEPOUT );
                drcItem->SetItems( segm, aKeepout );

                HandleMarker( new MARKER_PCB( drcItem, DRC::GetLocation( segm, aKeepout ) ) );
                success = false;
            }
        }
        else if( segm->Type() == PCB_VIA_T && aKeepout->GetDoNotAllowVias() )
        {
            if( !aKeepout->CommonLayerExists( segm->GetLayerSet() ) )
                continue;

            int         widths = segm->GetWidth() / 2;
            wxPoint     viaPos = segm->GetPosition();
            SEG::ecoord center2center_squared = aKeepout->Outline()->SquaredDistance( viaPos );

            if( center2center_squared <= SEG::Square( widths) )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_VIA_INSIDE_KEEPOUT );
                drcItem->SetItems( segm, aKeepout );

                HandleMarker( new MARKER_PCB( drcItem, DRC::GetLocation( segm, aKeepout ) ) );
                success = false;
            }
        }
    }

    return success;
}


bool DRC_KEEPOUT_TESTER::checkFootprints( ZONE_CONTAINER* aKeepout )
{
    bool     success = true;
    EDA_RECT areaBBox = aKeepout->GetBoundingBox();
    bool     checkFront = aKeepout->CommonLayerExists( LSET::FrontMask() );
    bool     checkBack = aKeepout->CommonLayerExists( LSET::BackMask() );

    for( MODULE* fp : m_board->Modules() )
    {
        if( aKeepout->GetDoNotAllowFootprints() && ( fp->IsFlipped() ? checkBack : checkFront ) )
        {
            // Fast test to detect a footprint inside the keepout area bounding box.
            if( areaBBox.Intersects( fp->GetBoundingBox() ) )
            {
                SHAPE_POLY_SET outline;

                if( fp->BuildPolyCourtyard() )
                {
                    outline = fp->IsFlipped() ? fp->GetPolyCourtyardBack()
                                              : fp->GetPolyCourtyardFront();
                }

                if( outline.OutlineCount() == 0 )
                    outline = fp->GetBoundingPoly();

                // Build the common area between footprint and the keepout area:
                outline.BooleanIntersection( *aKeepout->Outline(), SHAPE_POLY_SET::PM_FAST );

                // If it's not empty then we have a violation
                if( outline.OutlineCount() )
                {
                    const VECTOR2I& pt = outline.CVertex( 0, 0, -1 );
                    DRC_ITEM* drcItem = new DRC_ITEM( DRCE_FOOTPRINT_INSIDE_KEEPOUT );
                    drcItem->SetItems( fp, aKeepout );

                    HandleMarker( new MARKER_PCB( drcItem, (wxPoint) pt ) );
                    success = false;
                }
            }
        }

        if( aKeepout->GetDoNotAllowPads() )
        {
            for( D_PAD* pad : fp->Pads() )
            {
                if( !aKeepout->CommonLayerExists( pad->GetLayerSet() ) )
                    continue;

                // Fast test to detect a pad inside the keepout area bounding box.
                EDA_RECT padBBox( pad->ShapePos(), wxSize() );
                padBBox.Inflate( pad->GetBoundingRadius() );

                if( areaBBox.Intersects( padBBox ) )
                {
                    SHAPE_POLY_SET outline;
                    pad->TransformShapeWithClearanceToPolygon( outline, 0 );

                    // Build the common area between pad and the keepout area:
                    outline.BooleanIntersection( *aKeepout->Outline(), SHAPE_POLY_SET::PM_FAST );

                    // If it's not empty then we have a violation
                    if( outline.OutlineCount() )
                    {
                        const VECTOR2I& pt = outline.CVertex( 0, 0, -1 );
                        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_PAD_INSIDE_KEEPOUT );
                        drcItem->SetItems( pad, aKeepout );

                        HandleMarker( new MARKER_PCB( drcItem, (wxPoint) pt ) );
                        success = false;
                    }
                }
            }
        }
    }

    return success;
}


