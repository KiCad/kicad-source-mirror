/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <board_statistics.h>


void CollectDrillLineItems( BOARD* board, std::vector<DRILL_LINE_ITEM>& out )
{
    out.clear();

    auto addOrIncrement =
            [&]( const DRILL_LINE_ITEM& d )
            {
                for( DRILL_LINE_ITEM& e : out )
                {
                    if( e == d )
                    {
                        e.m_Qty++;
                        return;
                    }
                }

                DRILL_LINE_ITEM n = d;
                n.m_Qty = 1;
                out.push_back( n );
            };

    if( !board )
        return;

    // Pads
    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( !pad->HasHole() )
                continue;

            int xs = pad->GetDrillSize().x;
            int ys = pad->GetDrillSize().y;

            if( xs <= 0 || ys <= 0 )
                continue;

            PCB_LAYER_ID top, bottom;

            if( pad->GetLayerSet().CuStack().empty() )
            {
                top = UNDEFINED_LAYER;
                bottom = UNDEFINED_LAYER;
            }
            else
            {
                top = pad->GetLayerSet().CuStack().front();
                bottom = pad->GetLayerSet().CuStack().back();
            }

            DRILL_LINE_ITEM d( xs, ys, pad->GetDrillShape(), pad->GetAttribute() != PAD_ATTRIB::NPTH, true, top,
                               bottom );

            addOrIncrement( d );
        }
    }

    // Vias
    for( PCB_TRACK* t : board->Tracks() )
    {
        if( t->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( t );
        int      dmm = via->GetDrillValue();

        if( dmm <= 0 )
            continue;

        DRILL_LINE_ITEM d( dmm, dmm, PAD_DRILL_SHAPE::CIRCLE, true, false, via->TopLayer(), via->BottomLayer() );
        addOrIncrement( d );
    }
}

