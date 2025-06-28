/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <geometry/shape_file_io.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include <pcbnew_utils/board_file_utils.h>

#include <qa_utils/utility_registry.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>


void process( const BOARD_CONNECTED_ITEM* item, int net )
{
    if( item->GetNetCode() != net )
        return;

    SHAPE_POLY_SET pset;

    item->TransformShapeToPolygon( pset, UNDEFINED_LAYER, 1, item->GetMaxError(), ERROR_OUTSIDE );

    SHAPE_FILE_IO shapeIo; // default = stdout
    shapeIo.Write( &pset );
}


enum POLY_GEN_RET_CODES
{
    LOAD_FAILED = KI_TEST::RET_CODES::TOOL_SPECIFIC,
};


int polygon_gererator_main( int argc, char* argv[] )
{
    if( argc < 2 )
        return KI_TEST::RET_CODES::BAD_CMDLINE;

    std::string filename;

    if( argc > 1 )
        filename = argv[1];

    auto brd = KI_TEST::ReadBoardFromFileOrStream( filename );

    if( !brd )
        return POLY_GEN_RET_CODES::LOAD_FAILED;

    for( unsigned net = 0; net < brd->GetNetCount(); net++ )
    {
        for( PCB_TRACK* track : brd->Tracks() )
            process( track, net );

        for( FOOTPRINT* fp : brd->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
                process( pad, net );
        }

        for( ZONE* zone : brd->Zones() )
            process( zone, net );
    }

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "polygon_generator",
        "Dump board geometry as a set of polygons",
        polygon_gererator_main,
} );
