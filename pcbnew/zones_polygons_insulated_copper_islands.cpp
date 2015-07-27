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
    if( m_FilledPolysList.IsEmpty() )
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

    for( int outline = 0; outline < m_FilledPolysList.OutlineCount(); outline++ )
    {
        bool connected = false;

        for( unsigned ic = 0; ic < listPointsCandidates.size(); ic++ )
        {
            // test if this area is connected to a board item:
            wxPoint pos = listPointsCandidates[ic];

            if( m_FilledPolysList.Contains( VECTOR2I( pos.x, pos.y ), outline ) )
            {
                connected = true;
                break;
            }
        }

        if( !connected )                 // this polygon is connected: analyse next polygon
        {
            m_FilledPolysList.DeletePolygon( outline );
            outline--;
        }
    }
}
