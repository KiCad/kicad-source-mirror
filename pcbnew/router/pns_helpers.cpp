/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <board.h>
#include <geometry/shape_line_chain.h>
#include <math/vector2d.h>
#include <netinfo.h>
#include <pcb_track.h>
#include <router/pns_helpers.h>
#include <router/pns_arc.h>


PNS::LINKED_ITEM* PNS::HELPERS::PickSegment( PNS::ROUTER* aRouter, const VECTOR2I& aWhere, int aLayer,
                                             VECTOR2I& aPointOut, const SHAPE_LINE_CHAIN& aBaseline )
{
    int maxSlopRadius = aRouter->Sizes().Clearance() + aRouter->Sizes().TrackWidth() / 2;

    static const int  candidateCount = 2;
    PNS::LINKED_ITEM* prioritized[candidateCount];
    SEG::ecoord       dist[candidateCount];
    SEG::ecoord       distBaseline[candidateCount];
    VECTOR2I          point[candidateCount];

    for( int i = 0; i < candidateCount; i++ )
    {
        prioritized[i] = nullptr;
        dist[i] = VECTOR2I::ECOORD_MAX;
        distBaseline[i] = VECTOR2I::ECOORD_MAX;
    }

    for( int slopRadius : { 0, maxSlopRadius } )
    {
        PNS::ITEM_SET candidates = aRouter->QueryHoverItems( aWhere, slopRadius );

        for( PNS::ITEM* item : candidates.Items() )
        {
            if( !item->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) )
                continue;

            if( !item->IsRoutable() )
                continue;

            if( !item->Layers().Overlaps( aLayer ) )
                continue;

            PNS::LINKED_ITEM* linked = static_cast<PNS::LINKED_ITEM*>( item );

            if( item->Kind() & PNS::ITEM::ARC_T )
            {
                PNS::ARC* pnsArc = static_cast<PNS::ARC*>( item );

                VECTOR2I    nearest = pnsArc->Arc().NearestPoint( aWhere );
                SEG::ecoord d0 = ( nearest - aWhere ).SquaredEuclideanNorm();

                if( d0 > dist[1] )
                    continue;

                if( aBaseline.PointCount() > 0 )
                {
                    SEG::ecoord dcBaseline;
                    VECTOR2I    target = pnsArc->Arc().GetArcMid();

                    if( aBaseline.SegmentCount() > 0 )
                        dcBaseline = aBaseline.SquaredDistance( target );
                    else
                        dcBaseline = ( aBaseline.CPoint( 0 ) - target ).SquaredEuclideanNorm();

                    if( dcBaseline > distBaseline[1] )
                        continue;

                    distBaseline[1] = dcBaseline;
                }

                prioritized[1] = linked;
                dist[1] = d0;
                point[1] = nearest;
            }
            else if( item->Kind() & PNS::ITEM::SEGMENT_T )
            {
                PNS::SEGMENT* segm = static_cast<PNS::SEGMENT*>( item );

                VECTOR2I    nearest = segm->CLine().NearestPoint( aWhere, false );
                SEG::ecoord dd = ( aWhere - nearest ).SquaredEuclideanNorm();

                if( dd > dist[1] )
                    continue;

                if( aBaseline.PointCount() > 0 )
                {
                    SEG::ecoord dcBaseline;
                    VECTOR2I    target = segm->Shape( -1 )->Centre();

                    if( aBaseline.SegmentCount() > 0 )
                        dcBaseline = aBaseline.SquaredDistance( target );
                    else
                        dcBaseline = ( aBaseline.CPoint( 0 ) - target ).SquaredEuclideanNorm();

                    if( dcBaseline > distBaseline[1] )
                        continue;

                    distBaseline[1] = dcBaseline;
                }

                prioritized[1] = segm;
                dist[1] = dd;
                point[1] = nearest;
            }
        }
    }

    PNS::LINKED_ITEM* rv = nullptr;

    for( int i = 0; i < candidateCount; i++ )
    {
        PNS::LINKED_ITEM* item = prioritized[i];

        if( item && ( aLayer < 0 || item->Layers().Overlaps( aLayer ) ) )
        {
            rv = item;
            aPointOut = point[i];
            break;
        }
    }

    return rv;
}


VECTOR2I PNS::HELPERS::SnapToNearestTrack( const VECTOR2I& aP, BOARD* aBoard, NETINFO_ITEM* aNet,
                                           PCB_TRACK** aNearestTrack )
{
    SEG::ecoord minDist_sq = VECTOR2I::ECOORD_MAX;
    VECTOR2I    closestPt = aP;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( aNet && track->GetNet() != aNet )
            continue;

        VECTOR2I nearest;

        if( track->Type() == PCB_ARC_T )
        {
            PCB_ARC*  pcbArc = static_cast<PCB_ARC*>( track );
            SHAPE_ARC arc( pcbArc->GetStart(), pcbArc->GetMid(), pcbArc->GetEnd(), pcbArc->GetWidth() );

            nearest = arc.NearestPoint( aP );
        }
        else
        {
            SEG seg( track->GetStart(), track->GetEnd() );
            nearest = seg.NearestPoint( aP );
        }

        SEG::ecoord dist_sq = ( nearest - aP ).SquaredEuclideanNorm();

        if( dist_sq < minDist_sq )
        {
            minDist_sq = dist_sq;
            closestPt = nearest;

            if( aNearestTrack )
                *aNearestTrack = track;
        }
    }

    return closestPt;
}


VECTOR2I PNS::HELPERS::GetSnappedStartPoint( LINKED_ITEM* aStartItem, VECTOR2I aStartPoint )
{
    if( aStartItem->Kind() == ITEM::SEGMENT_T )
    {
        return static_cast<SEGMENT*>( aStartItem )->Seg().NearestPoint( aStartPoint );
    }
    else
    {
        wxASSERT( aStartItem->Kind() == ITEM::ARC_T );
        ARC* arc = static_cast<ARC*>( aStartItem );

        if( ( VECTOR2I( arc->Anchor( 0 ) - aStartPoint ) ).SquaredEuclideanNorm()
            <= ( VECTOR2I( arc->Anchor( 1 ) - aStartPoint ) ).SquaredEuclideanNorm() )
        {
            return arc->Anchor( 0 );
        }
        else
        {
            return arc->Anchor( 1 );
        }
    }
}
