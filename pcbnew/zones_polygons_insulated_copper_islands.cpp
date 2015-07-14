/**
 * @file zones_polygons_insulated_copper_islands.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <common.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>
#include <polygon_test_point_inside.h>


void ZONE_CONTAINER::TestForCopperIslandAndRemoveInsulatedIslands( BOARD* aPcb )
{
    if( m_FilledPolysList.GetCornersCount() == 0 )
        return;

    // Build a list of points connected to the net:
    // list of coordinates of pads and vias on this layer and on this net.
    std::vector <wxPoint> listPointsCandidates;

    for( MODULE* module = aPcb->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad != NULL; pad = pad->Next() )
        {
            if( !pad->IsOnLayer( GetLayer() ) )
                continue;

            if( pad->GetNetCode() != GetNetCode() )
                continue;

            listPointsCandidates.push_back( pad->GetPosition() );
        }
    }

    for( TRACK* track = aPcb->m_Track; track; track = track->Next() )
    {
        if( !track->IsOnLayer( GetLayer() ) )
            continue;

        if( track->GetNetCode() != GetNetCode() )
            continue;

        listPointsCandidates.push_back( track->GetStart() );

        if( track->Type() != PCB_VIA_T )
            listPointsCandidates.push_back( track->GetEnd() );
    }

    // test if a point is inside
    unsigned indexstart = 0, indexend;
    bool     connected  = false;

    for( indexend = 0; indexend < m_FilledPolysList.GetCornersCount(); indexend++ )
    {
        if( m_FilledPolysList[indexend].end_contour )    // end of a filled sub-area found
        {
            EDA_RECT bbox = CalculateSubAreaBoundaryBox( indexstart, indexend );

            for( unsigned ic = 0; ic < listPointsCandidates.size(); ic++ )
            {
                // test if this area is connected to a board item:
                wxPoint pos = listPointsCandidates[ic];

                if( !bbox.Contains( pos ) )
                    continue;

                if( TestPointInsidePolygon( m_FilledPolysList, indexstart, indexend,
                                            pos.x, pos.y ) )
                {
                    connected = true;
                    break;
                }
            }

            if( connected )                 // this polygon is connected: analyse next polygon
            {
                indexstart = indexend + 1;  // indexstart points the first point of the next polygon
                connected  = false;
            }
            else                             // Not connected: remove this polygon
            {
                m_FilledPolysList.DeleteCorners( indexstart, indexend );
                indexend = indexstart;   /* indexstart points the first point of the next polygon
                                          * because the current poly is removed */
            }
        }
    }
}


EDA_RECT ZONE_CONTAINER::CalculateSubAreaBoundaryBox( int aIndexStart, int aIndexEnd )
{
    CPolyPt  start_point, end_point;
    EDA_RECT bbox;

    start_point = m_FilledPolysList[aIndexStart];
    end_point   = start_point;

    for( int ii = aIndexStart; ii <= aIndexEnd; ii++ )
    {
        CPolyPt ptst = m_FilledPolysList[ii];

        if( start_point.x > ptst.x )
            start_point.x = ptst.x;

        if( start_point.y > ptst.y )
            start_point.y = ptst.y;

        if( end_point.x < ptst.x )
            end_point.x = ptst.x;

        if( end_point.y < ptst.y )
            end_point.y = ptst.y;
    }

    bbox.SetOrigin( start_point.x, start_point.y );
    bbox.SetEnd( wxPoint( end_point.x, end_point.y ) );

    return bbox;
}
