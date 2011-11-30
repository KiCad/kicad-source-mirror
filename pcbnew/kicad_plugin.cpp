
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
#include <errno.h>

#include <kicad_plugin.h>   // I implement this

#include <auto_ptr.h>
#include <kicad_string.h>
#include <macros.h>

//#include <fctsys.h>
//#include <confirm.h>
//#include <build_version.h>
//#include <wxPcbStruct.h">
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
#include <pcb_plot_params.h>
#include <drawtxt.h>

/*
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <autorout.h>
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

/// Test for a specific length of characters.
/// The -1 is to omit the trailing \0 which is included in sizeof() on a
/// string.
#define TESTLINE( x )   (strncmp( line, x, sizeof(x) - 1 ) == 0)

/// Get the length of a constant string, at compile time
#define SZ( x )         (sizeof(x)-1)


using namespace std;    // auto_ptr

BOARD* KICAD_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on then off the C locale.
    wxString    msg;

    m_board = aAppendToMe ? aAppendToMe : new BOARD( NULL );

    // delete on exception, iff I own m_board, according to aAppendToMe
    auto_ptr<BOARD> deleter( aAppendToMe ? NULL : m_board );

    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );
    if( !fp )
    {
        m_error.Printf( _( "Unable to open file '%s'" ), aFileName.GetData() );
        THROW_IO_ERROR( m_error );
    }

    FILE_LINE_READER    reader( fp, aFileName );

    aReader = &reader;

    init( aProperties );

    loadAllSections( bool( aAppendToMe ) );

    deleter.release();
    return m_board;
}


void KICAD_PLUGIN::loadAllSections( bool doAppend )
{
    // $GENERAL section is first

    // $SHEETDESCR section is next

    // $SETUP section is next

    // Then follows $EQUIPOT and all the rest

    while( aReader->ReadLine() )
    {
        char* line = aReader->Line();

        // put the more frequent ones at the top, but realize TRACKs are loaded as a group

        if( TESTLINE( "$MODULE" ) )
        {
            loadMODULE();
        }

        else if( TESTLINE( "$DRAWSEGMENT" ) )
        {
            loadDRAWSEGMENT();
        }

        else if( TESTLINE( "$EQUIPOT" ) )
        {
            loadNETINFO_ITEM();
        }

        else if( TESTLINE( "$TEXTPCB" ) )
        {
            loadPCB_TEXTE();
        }

        else if( TESTLINE( "$TRACK" ) )
        {
            TRACK* insertBeforeMe = doAppend ? NULL : m_board->m_Track.GetFirst();
            loadTrackList( insertBeforeMe, PCB_TRACE_T, NbTrack );
        }

        else if( TESTLINE( "$" BRD_NETCLASS ) )
        {
            loadNETCLASS();
        }

#if 0
        else if( TESTLINE( "$CZONE_OUTLINE" ) )
        {
            auto_ptr<ZONE_CONTAINER> zone_descr( new ZONE_CONTAINER( m_board ) );

            load( zone_descr.get() );

            if( zone_descr->GetNumCorners() > 2 )       // should always occur
                m_board->Add( zone_descr.release() );

            // else delete zone_descr; done by auto_ptr
        }

        else if( TESTLINE( "$COTATION" ) )
        {
            DIMENSION* dim = new DIMENSION( m_board );
            m_board->Add( dim, ADD_APPEND );
            load( dim );

        }

        else if( TESTLINE( "$PCB_TARGET" ) )
        {
            PCB_TARGET* t = new PCB_TARGET( m_board );
            m_board->Add( t, ADD_APPEND );
            load( t );

        }

        else if( TESTLINE( "$ZONE" ) )
        {
#if 0 && defined(PCBNEW)
            SEGZONE* insertBeforeMe = Append ? NULL : m_board->m_Zone.GetFirst();

            ReadListeSegmentDescr( aReader, insertBeforeMe, PCB_ZONE_T, NbZone );
#endif
        }
#endif

        else if( TESTLINE( "$GENERAL" ) )
        {
            loadGENERAL();
        }

        else if( TESTLINE( "$SHEETDESCR" ) )
        {
            loadSHEET();
        }

        else if( TESTLINE( "$SETUP" ) )
        {
            if( !doAppend )
            {
                loadSETUP();
            }
            else
            {
                while( aReader->ReadLine() )
                {
                    line = aReader->Line();

                    if( TESTLINE( "$EndSETUP" ) )
                        break;
                }
            }
        }

        else if( TESTLINE( "$EndPCB" ) )
            break;
    }
}


void KICAD_PLUGIN::loadGENERAL()
{
    static const char delims[] = " =\n\r";      // for this function only.

    while( aReader->ReadLine() )
    {
        char*       line = aReader->Line();
        const char* data;

        if( TESTLINE( "$EndGENERAL" ) )
            break;

        if( TESTLINE( "Units" ) )
        {
            // what are the engineering units of the dimensions in the BOARD?
            data = strtok( line + SZ("Units"), delims );

            if( strcmp( data, "mm" ) == 0 )
            {
#if defined(KICAD_NANOMETRE)
                diskToBiu = 1000000.0;
#else
                m_error = wxT( "May not load new *.brd file into 'PCBNew compiled for deci-mils'" );
                THROW_IO_ERROR( m_error );
#endif
            }
        }

        else if( TESTLINE( "EnabledLayers" ) )
        {
            int enabledLayers = 0;

            data = strtok( line + SZ( "EnabledLayers" ), delims );
            sscanf( data, "%X", &enabledLayers );

            // Setup layer visibility
            m_board->SetEnabledLayers( enabledLayers );
        }

        else if( TESTLINE( "Ly" ) )    // Old format for Layer count
        {
            int layer_mask = 1;

            data = strtok( line + SZ( "Ly" ), delims );
            sscanf( data, "%X", &layer_mask );

            int layer_count = 0;

            for( int ii = 0;  ii < NB_COPPER_LAYERS && layer_mask;  ++ii, layer_mask >>= 1 )
            {
                if( layer_mask & 1 )
                    layer_count++;
            }

            m_board->SetCopperLayerCount( layer_count );
        }

        else if( TESTLINE( "BoardThickness" ) )
        {
            data = strtok( line + SZ( "BoardThickness" ), delims );
            m_board->GetBoardDesignSettings()->m_BoardThickness = atoi( data );
        }

        else if( TESTLINE( "Links" ) )
        {
            // Info only, do nothing, but only for a short while.
        }

        else if( TESTLINE( "NoConn" ) )
        {
            data = strtok( line + SZ( "NoConn" ), delims );
            m_board->m_NbNoconnect = atoi( data );
        }

        else if( TESTLINE( "Di" ) )
        {
            // skip the first token "Di".
            // no use of strtok() in this one, don't want the nuls
            data = line + SZ( "Di" );

            BIU x1 = biuParse( data, &data );
            BIU y1 = biuParse( data, &data );
            BIU x2 = biuParse( data, &data );
            BIU y2 = biuParse( data );

            m_board->m_BoundaryBox.SetX( x1 );
            m_board->m_BoundaryBox.SetY( y1 );

            m_board->m_BoundaryBox.SetWidth( x2 - x1 );
            m_board->m_BoundaryBox.SetHeight( y2 - y1 );
        }

        // Read the number of segments of type DRAW, TRACK, ZONE
        else if( TESTLINE( "Ndraw" ) )
        {
            data   = strtok( line + SZ( "Ndraw" ), delims );
            NbDraw = atoi( data );
        }

        else if( TESTLINE( "Ntrack" ) )
        {
            data    = strtok( line + SZ( "Ntrack" ), delims );
            NbTrack = atoi( data );
        }

        else if( TESTLINE( "Nzone" ) )
        {
            data   = strtok( line + SZ( "Nzone" ), delims );
            NbZone = atoi( data );
        }

        else if( TESTLINE( "Nmodule" ) )
        {
            data  = strtok( line + SZ( "Nmodule" ), delims );
            NbMod = atoi( data );
        }

        else if( TESTLINE( "Nnets" ) )
        {
            data   = strtok( line + SZ( "Nnets" ), delims );
            NbNets = atoi( data );
        }
    }

    m_error = wxT( "Missing '$EndGENERAL'" );
    THROW_IO_ERROR( m_error );
}


void KICAD_PLUGIN::loadSHEET()
{
    char    buf[260];
    char*   text;

    static const char delims[] = " \t\n\r";      // for this function only.

    while( aReader->ReadLine() )
    {
        char* line = aReader->Line();

        if( TESTLINE( "$End" ) )
            return;             // preferred exit

        else if( TESTLINE( "Sheet" ) )
        {
            text = strtok( line, delims );
            text = strtok( NULL, delims );

            Ki_PageDescr* sheet = g_SheetSizeList[0];
            int           ii;

            for( ii = 0; sheet != NULL; ii++, sheet = g_SheetSizeList[ii] )
            {
                if( stricmp( TO_UTF8( sheet->m_Name ), text ) == 0 )
                {
//                  screen->m_CurrentSheetDesc = sheet;

                    if( sheet == &g_Sheet_user )
                    {
                        text = strtok( NULL, delims );

                        if( text )
                            sheet->m_Size.x = atoi( text );

                        text = strtok( NULL, delims );

                        if( text )
                            sheet->m_Size.y = atoi( text );
                    }

                    break;
                }
            }
        }

        else if( TESTLINE( "Title" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );

#if 0   // @todo "screen" not available here
            screen->m_Title = FROM_UTF8( buf );
        }

        else if( TESTLINE( "Date" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            screen->m_Date = FROM_UTF8( buf );
        }

        else if( TESTLINE( "Rev" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            screen->m_Revision = FROM_UTF8( buf );
        }

        else if( TESTLINE( "Comp" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            screen->m_Company = FROM_UTF8( buf );
        }

        else if( TESTLINE( "Comment1" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            screen->m_Commentaire1 = FROM_UTF8( buf );
        }

        else if( TESTLINE( "Comment2" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            screen->m_Commentaire2 = FROM_UTF8( buf );
        }

        else if( TESTLINE( "Comment3" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            screen->m_Commentaire3 = FROM_UTF8( buf );
        }

        else if( TESTLINE( "Comment4" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            screen->m_Commentaire4 = FROM_UTF8( buf );
#endif
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndSHEETDESCR'" ) );
}


void KICAD_PLUGIN::loadSETUP()
{
    NETCLASS* netclass_default = m_board->m_NetClasses.GetDefault();

    static const char delims[] = " =\n\r";      // for this function only

    while( aReader->ReadLine() )
    {
        char*       line = aReader->Line();
        const char* data;

        if( TESTLINE( "PcbPlotParams" ) )
        {
            PCB_PLOT_PARAMS_PARSER parser( &line[13], aReader->GetSource() );

            try
            {
                g_PcbPlotOptions.Parse( &parser );
            }
            catch( IO_ERROR& e )
            {
                wxString msg;

                msg.Printf( _( "Error reading PcbPlotParams from %s:\n%s" ),
                            aReader->GetSource().GetData(),
                            e.errorText.GetData() );
                wxMessageBox( msg, _( "Open Board File" ), wxICON_ERROR );
            }
            continue;
        }

        strtok( line, delims );
        data = strtok( NULL, delims );

        if( TESTLINE( "$EndSETUP" ) )
        {
            // Until such time as the *.brd file does not have the
            // global parameters:
            // "TrackWidth", "TrackMinWidth", "ViaSize", "ViaDrill",
            // "ViaMinSize", and "TrackClearence", put those same global
            // values into the default NETCLASS until later board load
            // code should override them.  *.brd files which have been
            // saved with knowledge of NETCLASSes will override these
            // defaults, old boards will not.
            //
            // @todo: I expect that at some point we can remove said global
            //        parameters from the *.brd file since the ones in the
            //        default netclass serve the same purpose.  If needed
            //        at all, the global defaults should go into a preferences
            //        file instead so they are there to start new board
            //        projects.
            m_board->m_NetClasses.GetDefault()->SetParams();

            return;
        }

        else if( TESTLINE( "AuxiliaryAxisOrg" ) )
        {
            BIU gx = biuParse( data, &data );
            BIU gy = biuParse( data );

            /* @todo
            m_Auxiliary_Axis_Position.x = gx;
            m_Auxiliary_Axis_Position.y = gy;
            */
        }

#if defined(PCBNEW)

        else if( TESTLINE( "Layers" ) == 0 )
        {
            int tmp = atoi( data );
            m_board->SetCopperLayerCount( tmp );
        }

        else if( TESTLINE( "Layer[" ) )
        {
            const int LAYERKEYZ = sizeof("Layer[") - 1;

            // parse:
            // Layer[n]  <a_Layer_name_with_no_spaces> <LAYER_T>

            char* cp    = line + LAYERKEYZ;
            int   layer = atoi( cp );

            if( data )
            {
                wxString layerName = FROM_UTF8( data );
                m_board->SetLayerName( layer, layerName );

                data = strtok( NULL, delims );
                if( data )  // optional in old board files
                {
                    LAYER_T type = LAYER::ParseType( data );
                    m_board->SetLayerType( layer, type );
                }
            }
        }

        /* no more used
        else if( TESTLINE( "TrackWidth" ) == 0 )
        {
        }
        else if( TESTLINE( "ViaSize" ) )
        {
        }
        else if( TESTLINE( "MicroViaSize" ) == 0 )
        {
        }
        */

        else if( TESTLINE( "TrackWidthList" ) )
        {
            BIU tmp = biuParse( data );
            m_board->m_TrackWidthList.push_back( tmp );
        }

        else if( TESTLINE( "TrackClearence" ) )
        {
            BIU tmp = biuParse( data );
            netclass_default->SetClearance( tmp );
        }

        else if( TESTLINE( "TrackMinWidth" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_TrackMinWidth = tmp;
        }

        else if( TESTLINE( "ZoneClearence" ) )
        {
            BIU tmp = biuParse( data );
            g_Zone_Default_Setting.m_ZoneClearance = tmp;
        }

        else if( TESTLINE( "DrawSegmWidth" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_DrawSegmentWidth = tmp;
        }

        else if( TESTLINE( "EdgeSegmWidth" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_EdgeSegmentWidth = tmp;
        }

        else if( TESTLINE( "ViaMinSize" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_ViasMinSize = tmp;
        }

        else if( TESTLINE( "MicroViaMinSize" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_MicroViasMinSize = tmp;
        }

        else if( TESTLINE( "ViaSizeList" ) )
        {
            // e.g.  "ViaSizeList DIAMETER [DRILL]"

            BIU drill    = 0;
            BIU diameter = biuParse( data );

            data = strtok( NULL, delims );

            if( data )  // DRILL may not be present
                drill = biuParse( data );

            m_board->m_ViasDimensionsList.push_back( VIA_DIMENSION( diameter, drill ) );
        }

        else if( TESTLINE( "ViaDrill" ) )
        {
            BIU tmp = biuParse( data );
            netclass_default->SetViaDrill( tmp );
        }

        else if( TESTLINE( "ViaMinDrill" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_ViasMinDrill = tmp;
        }

        else if( TESTLINE( "MicroViaDrill" ) )
        {
            BIU tmp = biuParse( data );
            netclass_default->SetuViaDrill( tmp );
        }

        else if( TESTLINE( "MicroViaMinDrill" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_MicroViasMinDrill = tmp;
        }

        else if( TESTLINE( "MicroViasAllowed" ) )
        {
            m_board->GetBoardDesignSettings()->m_MicroViasAllowed = atoi( data );
        }

        else if( TESTLINE( "TextPcbWidth" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_PcbTextWidth = tmp;
        }

        else if( TESTLINE( "TextPcbSize" ) )
        {
            BIU x = biuParse( data, &data );
            BIU y = biuParse( data );

            m_board->GetBoardDesignSettings()->m_PcbTextSize = wxSize( x, y );
        }

        else if( TESTLINE( "EdgeModWidth" ) )
        {
            BIU tmp = biuParse( data );
            /* @todo
            g_ModuleSegmentWidth = tmp;
            */
        }

        else if( TESTLINE( "TextModWidth" ) )
        {
            BIU tmp = biuParse( data );
            /* @todo
            g_ModuleTextWidth = tmp;
            */
        }

        else if( TESTLINE( "TextModSize" ) )
        {
            BIU x = biuParse( data, &data );
            BIU y = biuParse( data );
            /* @todo
            g_ModuleTextSize = wxSize( x, y );
            */
        }

        else if( TESTLINE( "PadSize" ) )
        {
            BIU x = biuParse( data, &data );
            BIU y = biuParse( data );
            /* @todo
            g_Pad_Master.m_Size = wxSize( x, y );
            */
        }

        else if( TESTLINE( "PadDrill" ) )
        {
            BIU tmp = biuParse( data );
            /* @todo
            g_Pad_Master.m_Drill.x( tmp );
            g_Pad_Master.m_Drill.y( tmp );
            */
        }

        else if( TESTLINE( "Pad2MaskClearance" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_SolderMaskMargin = tmp;
        }

        else if( TESTLINE( "Pad2PasteClearance" ) )
        {
            BIU tmp = biuParse( data );
            m_board->GetBoardDesignSettings()->m_SolderPasteMargin = tmp;
        }

        else if( TESTLINE( "Pad2PasteClearanceRatio" ) )
        {
            double ratio = atof( data );
            m_board->GetBoardDesignSettings()->m_SolderPasteMarginRatio = ratio;
        }

        else if( TESTLINE( "GridOrigin" ) )
        {
            BIU gx = biuParse( data, &data );
            BIU gy = biuParse( data );

            /* @todo
            GetScreen()->m_GridOrigin.x = Ox;
            GetScreen()->m_GridOrigin.y = Oy;
            */
        }
#endif

    }

    // @todo: this code is currently unreachable, would need a goto, to get here.
    // that may be better handled with an #ifdef

    /* Ensure tracks and vias sizes lists are ok:
     * Sort lists by by increasing value and remove duplicates
     * (the first value is not tested, because it is the netclass value
     */
    sort( m_board->m_ViasDimensionsList.begin() + 1, m_board->m_ViasDimensionsList.end() );
    sort( m_board->m_TrackWidthList.begin() + 1, m_board->m_TrackWidthList.end() );

    for( unsigned ii = 1; ii < m_board->m_ViasDimensionsList.size() - 1; ii++ )
    {
        if( m_board->m_ViasDimensionsList[ii] == m_board->m_ViasDimensionsList[ii + 1] )
        {
            m_board->m_ViasDimensionsList.erase( m_board->m_ViasDimensionsList.begin() + ii );
            ii--;
        }
    }

    for( unsigned ii = 1; ii < m_board->m_TrackWidthList.size() - 1; ii++ )
    {
        if( m_board->m_TrackWidthList[ii] == m_board->m_TrackWidthList[ii + 1] )
        {
            m_board->m_TrackWidthList.erase( m_board->m_TrackWidthList.begin() + ii );
            ii--;
        }
    }
}


void KICAD_PLUGIN::loadMODULE()
{
    MODULE* module = new MODULE( m_board );

    // immediately save object in tree, so if exception, no memory leak
    m_board->Add( module, ADD_APPEND );

    char*   line = aReader->Line();
    char*   text = line + 3;

    S3D_MASTER* t3D = module->m_3D_Drawings;

    if( !t3D->m_Shape3DName.IsEmpty() )
    {
        S3D_MASTER* n3D = new S3D_MASTER( module );

        module->m_3D_Drawings.PushBack( n3D );

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
                return;
            }
            goto out;   // error

        case 'N':       // Shape File Name
            {
                char    buf[512];
                ReadDelimitedText( buf, text, 512 );

                t3D->m_Shape3DName = FROM_UTF8( buf );
            }
            break;

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

out:
    THROW_IO_ERROR( wxT( "Missing '$EndMODULE'" ) );
}


void KICAD_PLUGIN::loadDRAWSEGMENT()
{
    /* example:
        $DRAWSEGMENT
        Po 0 57500 -1000 57500 0 150
        De 24 0 900 0 0
        $EndDRAWSEGMENT
    */

    // immediately save object in tree, so if exception, no memory leak
    DRAWSEGMENT* dseg = new DRAWSEGMENT( m_board );
    m_board->Add( dseg, ADD_APPEND );

    while( aReader->ReadLine() )
    {
        char* line  = aReader->Line();

        if( strnicmp( line, "$End", 4 ) == 0 )
            return;     // Normal end matches here, because it's: $EndDRAWSEGMENT

        if( line[0] == 'P' )
        {
            // sscanf( line + 2, " %d %d %d %d %d %d", &m_Shape, &m_Start.x, &m_Start.y, &m_End.x, &m_End.y, &m_Width );
            const char* next = line + 2;

            BIU shape   = biuParse( next, &next );
            BIU start_x = biuParse( next, &next );
            BIU start_y = biuParse( next, &next );
            BIU end_x   = biuParse( next, &next );
            BIU end_y   = biuParse( next, &next );
            BIU width   = biuParse( next );

            if( width < 0 )
                width = 0;

            dseg->SetShape( shape );
            dseg->SetWidth( width );
            dseg->SetStart( wxPoint( start_x, start_y ) );
            dseg->SetEnd( wxPoint( end_x, end_y ) );
        }

        else if( line[0] == 'D' )
        {
            BIU     x = 0;
            BIU     y = 0;
            int     val;
            char*   token = strtok( line, " " );    // "De", skip it

            for( int i = 0;  (token = strtok( NULL," " )) != NULL;  ++i )
            {
                switch( i )
                {
                case 0:
                    sscanf( token, "%d", &val );

                    if( val < FIRST_NO_COPPER_LAYER )
                        val = FIRST_NO_COPPER_LAYER;

                    else if( val > LAST_NO_COPPER_LAYER )
                        val = LAST_NO_COPPER_LAYER;

                    dseg->SetLayer( val );
                    break;
                case 1:
                    sscanf( token, "%d", &val );
                    dseg->SetType( val );   // m_Type
                    break;
                case 2:
                    sscanf( token, "%d", &val );
                    dseg->SetAngle( val );  // m_Angle
                    break;
                case 3:
                    sscanf( token, "%lX", &dseg->m_TimeStamp );
                    break;
                case 4:
                    sscanf( token, "%X", &val );
                    dseg->SetState( val, ON );
                    break;

                    // Bezier Control Points
                case 5:
                    x = biuParse( token );
                    break;
                case 6:
                    y = biuParse( token );
                    dseg->SetBezControl1( wxPoint( x, y ) );
                    break;

                case 7:
                    x = biuParse( token );
                    break;
                case 8:
                    y = biuParse( token );
                    dseg->SetBezControl2( wxPoint( x, y ) );
                    break;

                default:
                    break;
                }
            }
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndDRAWSEGMENT'" ) );
}


void KICAD_PLUGIN::loadNETINFO_ITEM()
{
    char  buf[1024];

    NETINFO_ITEM* net = new NETINFO_ITEM( m_board );
    m_board->m_NetInfo->AppendNet( net );

    while( aReader->ReadLine() )
    {
        char*   line = aReader->Line();

        if( TESTLINE( "$End" ) )
            return;     // preferred exit

        else if( TESTLINE( "Na" ) )
        {
            int tmp = atoi( line + 2 );
            net->SetNet( tmp );

            // skips to the first quote char
            ReadDelimitedText( buf, line + 2, sizeof(buf) );
            net->SetNetname( FROM_UTF8( buf ) );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndEQUIPOT'" ) );
}


void KICAD_PLUGIN::loadPCB_TEXTE()
{
    /*  examples:
        For a single line text:
        ----------------------
        $TEXTPCB
        Te "Text example"
        Po 66750 53450 600 800 150 0
        From 24 1 0 Italic
        $EndTEXTPCB

        For a multi line text:
        ---------------------
        $TEXTPCB
        Te "Text example"
        Nl "Line 2"
        Po 66750 53450 600 800 150 0
        From 24 1 0 Italic
        $EndTEXTPCB
        Nl "line nn" is a line added to the current text
    */

    char    text[1024];

    // maybe someday a constructor that takes all this data in one call?
    TEXTE_PCB* pcbtxt = new TEXTE_PCB( m_board );
    m_board->Add( pcbtxt, ADD_APPEND );

    while( aReader->ReadLine() )
    {
        char* line = aReader->Line();

        if( TESTLINE( "$EndTEXTPCB" ) )
        {
            return;     // preferred exit
        }

        else if( TESTLINE( "Te" ) )     // Text line (or first line for multi line texts)
        {
            ReadDelimitedText( text, line + 2, sizeof(text) );
            pcbtxt->SetText( FROM_UTF8( text ) );
        }

        else if( TESTLINE( "nl" ) )     // next line of the current text
        {
            ReadDelimitedText( text, line + 2, sizeof(text) );
            pcbtxt->SetText( pcbtxt->GetText() + '\n' +  FROM_UTF8( text ) );
        }

        else if( TESTLINE( "Po" ) )
        {
            // sscanf( line + 2, " %d %d %d %d %d %d", &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Size.y, &m_Thickness, &m_Orient );
            const char* data = line + SZ( "Po" );

            wxSize  sz;

            BIU pos_x   = biuParse( data, &data );
            BIU pos_y   = biuParse( data, &data );

            sz.x        = biuParse( data, &data );
            sz.y        = biuParse( data, &data );

            BIU thickn  = biuParse( data, &data );
            int orient  = atoi( data );

            // Ensure the text has minimal size to see this text on screen:

            /* @todo wait until we are firmly in the nanometer world
            if( sz.x < 5 )
                sz.x = 5;

            if( sz.y < 5 )
                sz.y = 5;
            */

            // Set a reasonable width:
            if( thickn < 1 )
                thickn = 1;

            thickn = Clamp_Text_PenSize( thickn, sz );

            pcbtxt->SetThickness( thickn );
            pcbtxt->SetOrientation( orient );

            pcbtxt->m_Pos  = wxPoint( pos_x, pos_y );
            pcbtxt->SetSize( sz );
        }

        else if( TESTLINE( "De" ) )
        {
            char  style[256];

            style[0] = 0;

            int     normal_display = 1;
            char    hJustify = 'c';
            int     layer = FIRST_COPPER_LAYER;
            long    timestamp = 0;
            bool    italic = false;

            // sscanf( line + 2, " %d %d %lX %s %c\n", &m_Layer, &normal_display, &m_TimeStamp, style, &hJustify );

            sscanf( line + 2, " %d %d %lX %s %c\n", &layer, &normal_display, &timestamp, style, &hJustify );

            normal_display = normal_display ? false : true;

            if( layer < FIRST_COPPER_LAYER )
                layer = FIRST_COPPER_LAYER;

            else if( layer > LAST_NO_COPPER_LAYER )
                layer = LAST_NO_COPPER_LAYER;

            if( strnicmp( style, "Italic", 6 ) == 0 )
                italic = true;

            switch( hJustify )
            {
            case 'l':
            case 'L':
                hJustify = GR_TEXT_HJUSTIFY_LEFT;
                break;
            case 'c':
            case 'C':
                hJustify = GR_TEXT_HJUSTIFY_CENTER;
                break;
            case 'r':
            case 'R':
                hJustify = GR_TEXT_HJUSTIFY_RIGHT;
                break;
            default:
                hJustify = GR_TEXT_HJUSTIFY_CENTER;
                break;
            }

            pcbtxt->SetHorizJustify( GRTextHorizJustifyType( hJustify ) );
            pcbtxt->SetLayer( layer );
            pcbtxt->SetItalic( italic );
            pcbtxt->SetTimeStamp( timestamp );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndTEXTPCB'" ) );
}


void KICAD_PLUGIN::loadTrackList( TRACK* aInsertBeforeMe, int aStructType, int aSegCount )
{
    static const char delims[] = " \t\n\r";      // for this function only.

    while( aReader->ReadLine() )
    {
        // read two lines per loop iteration, each loop is one TRACK or VIA
        // example first line:
        // "Po 0 23994 28800 24400 28800 150 -1\r\n"

        char*       line = aReader->Line();
        BIU         drill = -1;     // SetDefault() if -1
        TRACK*      newTrack;

        if( line[0] == '$' )    // $EndTRACK
            return;             // preferred exit

        // int arg_count = sscanf( line + 2, " %d %d %d %d %d %d %d", &shape, &tempStartX, &tempStartY, &tempEndX, &tempEndY, &width, &drill );

        const char* data = line + SZ( "Po" );

        int shape   = (int) strtol( data, (char**) &data, 10 );
        BIU startX  = biuParse( data, &data );
        BIU startY  = biuParse( data, &data );
        BIU endX    = biuParse( data, &data );
        BIU endY    = biuParse( data, &data );
        BIU width   = biuParse( data, &data );

        // optional 7th drill parameter (must be optional in an old format?)
        data = strtok( (char*) data, delims );
        if( data )
        {
            drill = biuParse( data );
        }

        // Read the 2nd line to determine the exact type, one of:
        // PCB_TRACE_T, PCB_VIA_T, or PCB_ZONE_T.  The type field in 2nd line
        // differentiates between PCB_TRACE_T and PCB_VIA_T.  With virtual
        // functions in use, it is critical to instantiate the PCB_VIA_T
        // exactly.
        if( !aReader->ReadLine() )
            break;

        line = aReader->Line();

        // example second line:
        // "De 0 0 463 0 800000\r\n"

        if( line[0] == '$' )
        {
            // mandatory 2nd line is missing
            THROW_IO_ERROR( wxT( "Missing 2nd line of a TRACK def" ) );
        }

        int         makeType;
        long        timeStamp;
        int         layer, type, flags, net_code;

        // parse the 2nd line to determine the type of object
        sscanf( line + SZ( "De" ), " %d %d %d %lX %X", &layer, &type, &net_code, &timeStamp, &flags );

        if( aStructType==PCB_TRACE_T && type==1 )
            makeType = PCB_VIA_T;
        else
            makeType = aStructType;

        switch( makeType )
        {
        default:
        case PCB_TRACE_T:
            newTrack = new TRACK( m_board );
            m_board->m_Track.Insert( newTrack, aInsertBeforeMe );
            break;

        case PCB_VIA_T:
            newTrack = new SEGVIA( m_board );
            m_board->m_Track.Insert( newTrack, aInsertBeforeMe );
            break;

        case PCB_ZONE_T:     // this is now deprecated, but exist in old boards
            newTrack = new SEGZONE( m_board );
            m_board->m_Zone.Insert( (SEGZONE*) newTrack, (SEGZONE*) aInsertBeforeMe );
            break;
        }

        newTrack->SetTimeStamp( timeStamp );

        newTrack->SetPosition( wxPoint( startX, startY ) );
        newTrack->SetEnd( wxPoint( endX, endY ) );

        newTrack->SetWidth( width );
        newTrack->SetShape( shape );

        if( drill <= 0 )
            newTrack->SetDrillDefault();
        else
            newTrack->SetDrillValue( drill );

        newTrack->SetLayer( layer );

        if( makeType == PCB_VIA_T )     // Ensure layers are OK when possible:
        {
            if( newTrack->Shape() == VIA_THROUGH )
                ( (SEGVIA*) newTrack )->SetLayerPair( LAYER_N_FRONT, LAYER_N_BACK );
        }

        newTrack->SetNet( net_code );
        newTrack->SetState( flags, ON );
    }

    THROW_IO_ERROR( wxT( "Missing '$EndTRACK'" ) );
}


void KICAD_PLUGIN::loadNETCLASS()
{
    char        buf[1024];
    wxString    netname;

    // create an empty NETCLASS without a name, but do not add it to the BOARD
    // yet since that would bypass duplicate netclass name checking within the BOARD.
    // store it temporarily in an auto_ptr until successfully inserted into the BOARD
    // just before returning.
    auto_ptr<NETCLASS> netclass( new NETCLASS( m_board, wxEmptyString ) );

    while( aReader->ReadLine() )
    {
        char* line = aReader->Line();

        if( TESTLINE( "AddNet" ) )
        {
            ReadDelimitedText( buf, line + SZ( "AddNet" ), sizeof(buf) );
            netname = FROM_UTF8( buf );
            netclass->Add( netname );
        }

        else if( TESTLINE( "$end" BRD_NETCLASS ) )
        {
            if( m_board->m_NetClasses.Add( netclass.get() ) )
            {
                netclass.release();
            }
            else
            {
                // Must have been a name conflict, this is a bad board file.
                // User may have done a hand edit to the file.

                // auto_ptr will delete netclass on this code path

                m_error.Printf( _( "duplicate NETCLASS name '%s'" ), netclass->GetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            return;             // prefered exit
        }

        else if( TESTLINE( "Clearance" ) )
        {
            BIU tmp = biuParse( line + SZ( "Clearance" ) );
            netclass->SetClearance( tmp );
        }

        else if( TESTLINE( "TrackWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackWidth" ) );
            netclass->SetTrackWidth( tmp );
        }

        else if( TESTLINE( "ViaDia" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaDia" ) );
            netclass->SetViaDiameter( tmp );
        }

        else if( TESTLINE( "ViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaDrill" ) );
            netclass->SetViaDrill( tmp );
        }

        else if( TESTLINE( "uViaDia" ) )
        {
            BIU tmp = biuParse( line + SZ( "uViaDia" ) );
            netclass->SetuViaDiameter( tmp );
        }

        else if( TESTLINE( "uViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "uViaDrill" ) );
            netclass->SetuViaDrill( tmp );
        }

        else if( TESTLINE( "Name" ) )
        {
            ReadDelimitedText( buf, line + SZ( "Name" ), sizeof(buf) );
            netclass->SetName( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Desc" ) )
        {
            ReadDelimitedText( buf, line + SZ( "Desc" ), sizeof(buf) );
            netclass->SetDescription( FROM_UTF8( buf ) );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$End" BRD_NETCLASS ) );
}


std::string KICAD_PLUGIN::biuFmt( BIU aValue )
{
    double  engUnits = biuToDisk * aValue;
    char    temp[48];

    if( engUnits != 0.0 && fabs( engUnits ) <= 0.0001 )
    {
        // printf( "f: " );
        int len = sprintf( temp, "%.10f", engUnits );

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


BIU KICAD_PLUGIN::biuParse( const char* aValue, const char** nptrptr )
{
    char*   nptr;

    errno = 0;

    double fval = strtod( aValue, &nptr );

    if( errno || aValue == nptr )
    {
        m_error.Printf( _( "invalid float number in file: '%s' line: %d" ),
            aReader->GetSource().GetData(), aReader->LineNumber() );

        THROW_IO_ERROR( m_error );
    }

    if( nptrptr )
        *nptrptr = nptr;

    // There should be no rounding issues here, since the values in the file initially
    // came from integers via biuFmt(). In fact this product should be an integer, exactly.
    return BIU( fval * diskToBiu );
}


void KICAD_PLUGIN::init( PROPERTIES* aProperties )
{
    NbDraw = NbTrack = NbZone = NbMod = NbNets = -1;

#if defined(KICAD_NANOMETRE)
    biuToDisk = 1/1000000.0;        // BIUs are nanometers
#else
    biuToDisk = 1.0;                // BIUs are deci-mils
#endif

    // start by assuming the board is in deci-mils.
    // if we see "Units mm" in the $GENERAL section, switch to 1000000.0 then.

#if defined(KICAD_NANOMETRE)
    diskToBiu = 2540.0;             // BIUs are nanometers
#else
    diskToBiu = 1.0;                // BIUs are deci-mils
#endif

    m_board->m_Status_Pcb = 0;
    m_board->m_NetClasses.Clear();
}


void KICAD_PLUGIN::Save( const wxString* aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on then off the C locale.
}

