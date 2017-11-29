/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_file_io.h>

#include <io_mgr.h>
#include <kicad_plugin.h>

#include <class_board.h>
#include <class_zone.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_drawsegment.h>
#include <class_module.h>

BOARD* loadBoard( const std::string& filename )
{
    PLUGIN::RELEASER pi( new PCB_IO );
    BOARD* brd = nullptr;

    try
    {
        brd = pi->Load( wxString( filename.c_str() ), NULL, NULL );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading board.\n%s" ),
                ioe.Problem() );

        printf( "%s\n", (const char*) msg.mb_str() );
        return nullptr;
    }

    return brd;
}


void process( const BOARD_CONNECTED_ITEM* item, int net )
{
    if( item->GetNetCode() != net )
        return;

    SHAPE_POLY_SET pset;

    const int segsPerCircle = 64;

    double correctionFactor = 1.0 / cos( M_PI / (double) segsPerCircle );


    item->TransformShapeWithClearanceToPolygon( pset, 1, segsPerCircle, correctionFactor );

    SHAPE_FILE_IO shapeIo;    // default = stdout
    shapeIo.Write( &pset );
}


int main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        printf( "A sample tool for dumping board geometry as a set of polygons.\n" );
        printf( "usage : %s board_file.kicad_pcb\n\n", argv[0] );
        return -1;
    }

    std::unique_ptr<BOARD> brd( loadBoard( argv[1] ) );

    if( !brd )
        return -1;

    for( int net = 0; net < brd->GetNetCount(); net++ )
    {
        printf( "net %d\n", net );

        for( auto track : brd->Tracks() )
            process( track, net );

        for( auto mod : brd->Modules() )
        {
            for( auto pad : mod->Pads() )
                process( pad, net );
        }

        for( auto zone : brd->Zones() )
            process( zone, net );

        printf( "endnet\n" );
    }
}
