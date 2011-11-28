
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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


/* This implements loading and saving a BOARD, behind the PLUGIN interface.
*/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <kicad_plugin.h>   // I implement this

#include <auto_ptr.h>
#include <kicad_string.h>


//#include <fctsys.h>
//#include <confirm.h>
//#include <build_version.h>
//#include <wxPcbStruct.h">
//#include <macros.h>
//#include <pcbcommon.h>

#include <zones.h>

#ifdef CVPCB
//#include <cvpcb.h>
#endif

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_pcb_text.h>
#include <class_zone.h>
#include <class_dimension.h>
#include <class_drawsegment.h>
#include <class_mire.h>
#include <3d_struct.h>


/*
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <autorout.h>
#include <pcb_plot_params.h>
*/


/* ASCII format of structures:
 *
 * Structure PAD:
 *
 * $PAD
 * Sh "name" form DIMVA dimH dV dH East: general form dV, dH = delta size
 * Dr. diam dV dH: drill: diameter drilling offsets
 * At Type S / N layers: standard, cms, conn, hole, meca.,
 *    Stack / Normal, 32-bit hexadecimal: occupation layers
 * Nm net_code netname
 * Po posrefX posrefy: reFX position, Y (0 = east position / anchor)
 * $EndPAD
 *
 * Module Structure
 *
 * $MODULE namelib
 * Po ax ay east layer masquelayer m_TimeCode
 *    ax ay ord = anchor (position module)
 *    east = east to 0.1 degree
 *    layer = layer number
 *    masquelayer = silkscreen layer for
 *    m_TimeCode internal use (groups)
 * Li <namelib>
 *
 * Cd <text> description of the component (Component Doc)
 * Kw <text> List of key words
 *
 * Sc schematic timestamp, reference schematic
 *
 * Op rot90 rot180 placement Options Auto (court rot 90, 180)
 *    rot90 is about 2x4-bit:
 *    lsb = cost rot 90, rot court msb = -90;
 *
 * Tn px py DIMVA dimh East thickness mirror visible "text"
 *    n = type (0 = ref, val = 1,> 1 = qcq
 *    Texts POS x, y / anchor and orient module 0
 *    DIMVA dimh East
 *    mirror thickness (Normal / Mirror)
 *    Visible V / I
 * DS ox oy fx fy w
 *    Edge: coord segment ox, oy has fx, fy, on
 *    was the anchor and orient 0
 *    thickness w
 * DC ox oy fx fy w descr circle (center, 1 point, thickness)
 * $PAD
 * $EndPAD section pads if available
 * $Endmodule
 */


#define MM_PER_BIU      1e-6
#define UM_PER_BIU      1e-3


using namespace std;

BOARD* KICAD_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    wxString    msg;
    BOARD*      board = aAppendToMe ? aAppendToMe : new BOARD( NULL );

    // delete on exception, iff I own it, according to aAppendToMe
    auto_ptr<BOARD> deleter( aAppendToMe ? NULL : board );

    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );
    if( !fp )
    {
        msg.Printf( _( "Unable to open file '%s'" ), aFileName.GetData() );
        THROW_IO_ERROR( msg );
    }

    FILE_LINE_READER    reader( fp, aFileName );

    aReader = &reader;

    init( board, aProperties );

    // Put a dollar sign in front, and test for a specific length of characters
    // The -1 is to omit the trailing \0 which is included in sizeof() on a
    // string.
#define TESTLINE( x ) (strncmp( line, "$" x, sizeof("$" x) - 1 ) == 0)

    while( aReader->ReadLine() )
    {
        char* line = aReader->Line();

        // put the more frequent ones at the top

        if( TESTLINE( "MODULE" ) )
        {
            MODULE* module = new MODULE( board );
            board->Add( module, ADD_APPEND );
            load( module );
            continue;
        }

        if( TESTLINE( "DRAWSEGMENT" ) )
        {
            DRAWSEGMENT* dseg = new DRAWSEGMENT( board );
            board->Add( dseg, ADD_APPEND );
            load( dseg );
            continue;
        }

        if( TESTLINE( "EQUIPOT" ) )
        {
            NETINFO_ITEM* net = new NETINFO_ITEM( board );
            board->m_NetInfo->AppendNet( net );
            load( net );
            continue;
        }

        if( TESTLINE( "TEXTPCB" ) )
        {
            TEXTE_PCB* pcbtxt = new TEXTE_PCB( board );
            board->Add( pcbtxt, ADD_APPEND );
            load( pcbtxt );
            continue;
        }

        if( TESTLINE( "TRACK" ) )
        {
#if 0 && defined(PCBNEW)
            TRACK* insertBeforeMe = Append ? NULL : board->m_Track.GetFirst();
            ReadListeSegmentDescr( aReader, insertBeforeMe, PCB_TRACE_T, NbTrack );
#endif
            continue;
        }

        if( TESTLINE( BRD_NETCLASS ) )
        {
/*
            // create an empty NETCLASS without a name.
            NETCLASS* netclass = new NETCLASS( board, wxEmptyString );

            // fill it from the *.brd file, and establish its name.
            netclass->ReadDescr( aReader );

            if( !board->m_NetClasses.Add( netclass ) )
            {
                // Must have been a name conflict, this is a bad board file.
                // User may have done a hand edit to the file.
                // Delete netclass if board could not take ownership of it.
                delete netclass;

                // @todo: throw an exception here, this is a bad board file.
            }
*/
            continue;
        }

        if( TESTLINE( "CZONE_OUTLINE" ) )
        {
            auto_ptr<ZONE_CONTAINER> zone_descr( new ZONE_CONTAINER( board ) );

            load( zone_descr.get() );

            if( zone_descr->GetNumCorners() > 2 )       // should always occur
                board->Add( zone_descr.release() );

            // else delete zone_descr; done by auto_ptr

            continue;
        }

        if( TESTLINE( "COTATION" ) )
        {
            DIMENSION* dim = new DIMENSION( board );
            board->Add( dim, ADD_APPEND );
            load( dim );
            continue;
        }

        if( TESTLINE( "PCB_TARGET" ) )
        {
            PCB_TARGET* t = new PCB_TARGET( board );
            board->Add( t, ADD_APPEND );
            load( t );
            continue;
        }

        if( TESTLINE( "ZONE" ) )
        {
#if 0 && defined(PCBNEW)
            SEGZONE* insertBeforeMe = Append ? NULL : board->m_Zone.GetFirst();

            ReadListeSegmentDescr( aReader, insertBeforeMe, PCB_ZONE_T, NbZone );
#endif
            continue;
        }

        if( TESTLINE( "GENERAL" ) )
        {
            loadGeneral( board );
            continue;
        }

        if( TESTLINE( "SHEETDESCR" ) )
        {
            loadSheet( board );
            continue;
        }

        if( TESTLINE( "SETUP" ) )
        {
            if( !aAppendToMe )
            {
                loadSetup( board );
            }
            else
            {
                while( aReader->ReadLine() )
                {
                    line = aReader->Line();

                    if( TESTLINE( "EndSETUP" ) )
                        break;
                }
            }
            continue;
        }

        if( TESTLINE( "EndPCB" ) )
            break;
    }

    deleter.release();  // no exceptions possible between here and return
    return board;
}


void KICAD_PLUGIN::load( MODULE* me )
{
    char*   line = aReader->Line();
    char*   text = line + 3;

    S3D_MASTER* t3D = me->m_3D_Drawings;

    if( !t3D->m_Shape3DName.IsEmpty() )
    {
        S3D_MASTER* n3D = new S3D_MASTER( me );

        me->m_3D_Drawings.PushBack( n3D );

        t3D = n3D;
    }

    while( aReader->ReadLine() )
    {
        line = aReader->Line();

        switch( line[0] )
        {
        case '$':
            if( line[1] == 'E' )
            {
                return 0;
            }
            return 1;

        case 'N':       // Shape File Name
        {
            char    buf[512];
            ReadDelimitedText( buf, text, 512 );

            t3D->m_Shape3DName = FROM_UTF8( buf );
            break;
        }

        case 'S':       // Scale
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatScale.x,
                    &t3D->m_MatScale.y,
                    &t3D->m_MatScale.z );
            break;

        case 'O':       // Offset
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatPosition.x,
                    &t3D->m_MatPosition.y,
                    &t3D->m_MatPosition.z );
            break;

        case 'R':       // Rotation
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatRotation.x,
                    &t3D->m_MatRotation.y,
                    &t3D->m_MatRotation.z );
            break;

        default:
            break;
        }
    }
}


std::string KICAD_PLUGIN::biuFmt( BIU aValue )
{
    BFU     engUnits = biuToDiskUnits * aValue;
    char    temp[48];

    if( engUnits != 0.0 && fabsl( engUnits ) <= 0.0001 )
    {
        // printf( "f: " );
        sprintf( temp, "%.10f", engUnits );

        int len = strlen( temp );

        while( --len > 0 && temp[len] == '0' )
            temp[len] = '\0';
    }
    else
    {
        // printf( "g: " );
        sprintf( temp, "%.10g", engUnits );
    }

    return temp;
}


void KICAD_PLUGIN::init( BOARD* aBoard, PROPERTIES* aProperties )
{
    NbDraw = NbTrack = NbZone = NbMod = NbNets = -1;

    aBoard->m_Status_Pcb = 0;
    aBoard->m_NetClasses.Clear();
}


void KICAD_PLUGIN::Save( const wxString* aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{



}

