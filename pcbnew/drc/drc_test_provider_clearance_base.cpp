/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <class_board.h>
#include <class_track.h>
#include <geometry/seg.h>
#include <drc/drc_test_provider_clearance_base.h>

const int UI_EPSILON = Mils2iu( 5 );


wxPoint DRC_TEST_PROVIDER_CLEARANCE_BASE::getLocation( PCB_LAYER_ID aLayer, TRACK* aTrack,
                                                       ZONE_CONTAINER* aZone )
{
    SHAPE_POLY_SET* zonePoly = nullptr;

    if( aZone->IsFilled() && aZone->HasFilledPolysForLayer( aLayer ) )
        zonePoly = const_cast<SHAPE_POLY_SET*>( &aZone->GetFilledPolysList( aLayer ) );

    if( !zonePoly || zonePoly->IsEmpty() )
        zonePoly = aZone->Outline();

    SEG         trackSeg( aTrack->GetStart(), aTrack->GetEnd() );
    SEG::ecoord closestDist_sq = VECTOR2I::ECOORD_MAX;
    SEG         closestSeg;

    for( auto it = zonePoly->CIterateSegments( 0, -1, true ); it; it++ )
    {
        SEG::ecoord dist_sq = trackSeg.SquaredDistance( *it );

        if( dist_sq < closestDist_sq )
        {
            closestDist_sq = dist_sq;
            closestSeg = *it;
        }
    }

    VECTOR2I pt1 = closestSeg.A;
    VECTOR2I pt2 = closestSeg.B;

    // Do a binary search for a "good enough" marker location
    while( GetLineLength( (wxPoint) pt1, (wxPoint) pt2 ) > UI_EPSILON )
    {
        if( trackSeg.SquaredDistance( pt1 ) < trackSeg.SquaredDistance( pt2 ) )
            pt2 = ( pt1 + pt2 ) / 2;
        else
            pt1 = ( pt1 + pt2 ) / 2;
    }

    // Once we're within UI_EPSILON pt1 and pt2 are "equivalent"
    return (wxPoint) pt1;
}

wxPoint DRC_TEST_PROVIDER_CLEARANCE_BASE::getLocation( TRACK* aTrack, const SEG& aConflictSeg )
{
    wxPoint pt1 = aTrack->GetPosition();
    wxPoint pt2 = aTrack->GetEnd();

    // Do a binary search along the track for a "good enough" marker location
    while( GetLineLength( pt1, pt2 ) > UI_EPSILON )
    {
        if( aConflictSeg.SquaredDistance( pt1 ) < aConflictSeg.SquaredDistance( pt2 ) )
            pt2 = ( pt1 + pt2 ) / 2;
        else
            pt1 = ( pt1 + pt2 ) / 2;
    }

    // Once we're within UI_EPSILON pt1 and pt2 are "equivalent"
    return pt1;
}
