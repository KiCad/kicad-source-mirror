
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

/*
    This implements loading and saving a BOARD, behind the PLUGIN interface.

    The philosophy on loading:
    *) BIUs should be typed as such to distinguish them from ints.
    *) Do not assume that BIUs will always be int, doing a sscanf() into a BIU
       does not make sense in case the size of the BUI changes.
    *) variables are put onto the stack in an automatic, even when it might look
       more efficient to do otherwise.  This is so we can seem them with a debugger.
    *) Global variables should not be touched from within a PLUGIN, since it will eventually
       be in a DLL/DSO.  This includes window information too.  The PLUGIN API knows
       nothing of wxFrame.
    *) No wxWindowing calls are made in here, since the UI is going to process a bucket
       of detailed information thrown from here in the form of an exception if an error
       happens.
*/


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <kicad_plugin.h>   // implement this here

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

/// C string compare test for a specific length of characters.
/// The -1 is to omit the trailing \0 which is included in sizeof() on a
/// string constant.
#define TESTLINE( x )   (strncmp( line, x, sizeof(x) - 1 ) == 0)

/// Get the length of a string constant, at compile time
#define SZ( x )         (sizeof(x)-1)


static const char delims[] = " \t\r\n";

using namespace std;    // auto_ptr

/**
 * Function intParse
 * parses an ASCII integer string with possible leading whitespace into
 * an integer and updates the pointer at \a out if it is not NULL, just
 * like "man strtol()".  I can use this without casting, and its name says
 * what I am doing.
 */
static inline int intParse( const char* next, const char** out = NULL )
{
    // please just compile this and be quiet, hide casting ugliness:
    return (int) strtol( next, (char**) out, 10 );
}


/**
 * Function hexParse
 * parses an ASCII hex integer string with possible leading whitespace into
 * a long integer and updates the pointer at \a out if it is not NULL, just
 * like "man strtol".  I can use this without casting, and its name says
 * what I am doing.
 */
static inline long hexParse( const char* next, const char** out = NULL )
{
    // please just compile this and be quiet, hide casting ugliness:
    return strtol( next, (char**) out, 16 );
}


BOARD* KICAD_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

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
            loadTrackList( insertBeforeMe, PCB_TRACE_T );
        }

        else if( TESTLINE( "$NCLASS" ) )
        {
            loadNETCLASS();
        }

        else if( TESTLINE( "$CZONE_OUTLINE" ) )
        {
            loadZONE_CONTAINER();
        }

        else if( TESTLINE( "$COTATION" ) )
        {
            loadDIMENSION();
        }

        else if( TESTLINE( "$PCB_TARGET" ) )
        {
            loadPCB_TARGET();
        }

#if defined(PCBNEW)
        else if( TESTLINE( "$ZONE" ) )
        {
            SEGZONE* insertBeforeMe = doAppend ? NULL : m_board->m_Zone.GetFirst();
            loadTrackList( insertBeforeMe, PCB_ZONE_T );
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
                    line = aReader->Line();     // gobble til $EndSetup

                    if( TESTLINE( "$EndSETUP" ) )
                        break;
                }
            }
        }

        else if( TESTLINE( "$EndBOARD" ) )
            return;     // preferred exit
    }

    THROW_IO_ERROR( wxT( "Missing '$EndBOARD'" ) );
}


void KICAD_PLUGIN::loadGENERAL()
{
    while( aReader->ReadLine() )
    {
        char*       line = aReader->Line();
        const char* data;

        if( TESTLINE( "$EndGENERAL" ) )
            return;     // preferred exit

        else if( TESTLINE( "Units" ) )
        {
            // what are the engineering units of the lengths in the BOARD?
            data = strtok( line + SZ("Units"), delims );

            if( !strcmp( data, "mm" ) )
            {
#if defined(KICAD_NANOMETRE)
                diskToBiu = 1000000.0;
#else
                THROW_IO_ERROR( _( "May not load new *.brd file into 'PCBNew compiled for deci-mils'" ) );
#endif
            }
        }

        else if( TESTLINE( "EnabledLayers" ) )
        {
            int enabledLayers = hexParse( line + SZ( "EnabledLayers" ) );

            // Setup layer visibility
            m_board->SetEnabledLayers( enabledLayers );
        }

        else if( TESTLINE( "Ly" ) )    // Old format for Layer count
        {
            int layer_mask = hexParse( line + SZ( "Ly" ) );

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
            BIU thickn = biuParse( line + SZ( "BoardThickness" ) );
            m_board->GetBoardDesignSettings()->m_BoardThickness = thickn;
        }

        /*
        else if( TESTLINE( "Links" ) )
        {
            // Info only, do nothing, but only for a short while.
        }
        */

        else if( TESTLINE( "NoConn" ) )
        {
            int tmp = atoi( line + SZ( "NoConn" ) );
            m_board->m_NbNoconnect = tmp;
        }

        else if( TESTLINE( "Di" ) )
        {
            BIU x1 = biuParse( line + SZ( "Di" ), &data );
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
            data   = line + SZ( "Ndraw" );
            NbDraw = atoi( data );
        }

        else if( TESTLINE( "Ntrack" ) )
        {
            data    = line + SZ( "Ntrack" );
            NbTrack = atoi( data );
        }

        else if( TESTLINE( "Nzone" ) )
        {
            data   = line + SZ( "Nzone" );
            NbZone = atoi( data );
        }

        else if( TESTLINE( "Nmodule" ) )
        {
            data  = line + SZ( "Nmodule" );
            NbMod = atoi( data );
        }

        else if( TESTLINE( "Nnets" ) )
        {
            data   = line + SZ( "Nnets" );
            NbNets = atoi( data );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndGENERAL'" ) );
}


void KICAD_PLUGIN::loadSHEET()
{
    char    buf[260];
    char*   text;

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
//  @todo           screen->m_CurrentSheetDesc = sheet;

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

    while( aReader->ReadLine() )
    {
        const char* data;

        char* line = aReader->Line();

        if( TESTLINE( "PcbPlotParams" ) )
        {
            PCB_PLOT_PARAMS_PARSER parser( line + SZ( "PcbPlotParams" ), aReader->GetSource() );

//            try
            {
                g_PcbPlotOptions.Parse( &parser );
            }
/* move this higher up
            catch( IO_ERROR& e )
            {
                wxString msg;

                msg.Printf( _( "Error reading PcbPlotParams from %s:\n%s" ),
                            aReader->GetSource().GetData(),
                            e.errorText.GetData() );
                wxMessageBox( msg, _( "Open Board File" ), wxICON_ERROR );
            }
*/
        }

        else if( TESTLINE( "$EndSETUP" ) )
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
            BIU gx = biuParse( line + SZ( "AuxiliaryAxisOrg" ), &data );
            BIU gy = biuParse( data );

            /* @todo
            m_Auxiliary_Axis_Position.x = gx;
            m_Auxiliary_Axis_Position.y = gy;
            */
        }

#if defined(PCBNEW)

        else if( TESTLINE( "Layers" ) == 0 )
        {
            int tmp = atoi( line + SZ( "Layers" ) );
            m_board->SetCopperLayerCount( tmp );
        }

        else if( TESTLINE( "Layer[" ) )
        {
            // eg: "Layer[n]  <a_Layer_name_with_no_spaces> <LAYER_T>"

            int   layer = intParse( line + SZ( "Layer[" ), &data );

            data = strtok( (char*) data+1, delims );    // +1 for ']'
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
            BIU tmp = biuParse( line + SZ( "TrackWidthList" ) );
            m_board->m_TrackWidthList.push_back( tmp );
        }

        else if( TESTLINE( "TrackClearence" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackClearence" ) );
            netclass_default->SetClearance( tmp );
        }

        else if( TESTLINE( "TrackMinWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackMinWidth" ) );
            m_board->GetBoardDesignSettings()->m_TrackMinWidth = tmp;
        }

        else if( TESTLINE( "ZoneClearence" ) )
        {
            BIU tmp = biuParse( line + SZ( "ZoneClearence" ) );
            g_Zone_Default_Setting.m_ZoneClearance = tmp;
        }

        else if( TESTLINE( "DrawSegmWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "DrawSegmWidth" ) );
            m_board->GetBoardDesignSettings()->m_DrawSegmentWidth = tmp;
        }

        else if( TESTLINE( "EdgeSegmWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "EdgeSegmWidth" ) );
            m_board->GetBoardDesignSettings()->m_EdgeSegmentWidth = tmp;
        }

        else if( TESTLINE( "ViaMinSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaMinSize" ) );
            m_board->GetBoardDesignSettings()->m_ViasMinSize = tmp;
        }

        else if( TESTLINE( "MicroViaMinSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaMinSize" ) );
            m_board->GetBoardDesignSettings()->m_MicroViasMinSize = tmp;
        }

        else if( TESTLINE( "ViaSizeList" ) )
        {
            // e.g.  "ViaSizeList DIAMETER [DRILL]"

            BIU drill    = 0;
            BIU diameter = biuParse( line + SZ( "ViaSizeList" ), &data );

            data = strtok( (char*) data, delims );
            if( data )  // DRILL may not be present ?
                drill = biuParse( data );

            m_board->m_ViasDimensionsList.push_back( VIA_DIMENSION( diameter, drill ) );
        }

        else if( TESTLINE( "ViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaDrill" ) );
            netclass_default->SetViaDrill( tmp );
        }

        else if( TESTLINE( "ViaMinDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaMinDrill" ) );
            m_board->GetBoardDesignSettings()->m_ViasMinDrill = tmp;
        }

        else if( TESTLINE( "MicroViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaDrill" ) );
            netclass_default->SetuViaDrill( tmp );
        }

        else if( TESTLINE( "MicroViaMinDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaMinDrill" ) );
            m_board->GetBoardDesignSettings()->m_MicroViasMinDrill = tmp;
        }

        else if( TESTLINE( "MicroViasAllowed" ) )
        {
            int tmp = atoi( line + SZ( "MicroViasAllowed" ) );
            m_board->GetBoardDesignSettings()->m_MicroViasAllowed = tmp;
        }

        else if( TESTLINE( "TextPcbWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TextPcbWidth" ) );
            m_board->GetBoardDesignSettings()->m_PcbTextWidth = tmp;
        }

        else if( TESTLINE( "TextPcbSize" ) )
        {
            BIU x = biuParse( line + SZ( "TextPcbSize" ), &data );
            BIU y = biuParse( data );

            m_board->GetBoardDesignSettings()->m_PcbTextSize = wxSize( x, y );
        }

        else if( TESTLINE( "EdgeModWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "EdgeModWidth" ) );
            /* @todo
            g_ModuleSegmentWidth = tmp;
            */
        }

        else if( TESTLINE( "TextModWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TextModWidth" ) );
            /* @todo
            g_ModuleTextWidth = tmp;
            */
        }

        else if( TESTLINE( "TextModSize" ) )
        {
            BIU x = biuParse( line + SZ( "TextModSize" ), &data );
            BIU y = biuParse( data );
            /* @todo
            g_ModuleTextSize = wxSize( x, y );
            */
        }

        else if( TESTLINE( "PadSize" ) )
        {
            BIU x = biuParse( line + SZ( "PadSize" ), &data );
            BIU y = biuParse( data );
            /* @todo
            g_Pad_Master.m_Size = wxSize( x, y );
            */
        }

        else if( TESTLINE( "PadDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "PadDrill" ) );
            /* @todo
            g_Pad_Master.m_Drill.x( tmp );
            g_Pad_Master.m_Drill.y( tmp );
            */
        }

        else if( TESTLINE( "Pad2MaskClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( "Pad2MaskClearance" ) );
            m_board->GetBoardDesignSettings()->m_SolderMaskMargin = tmp;
        }

        else if( TESTLINE( "Pad2PasteClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( "Pad2PasteClearance" ) );
            m_board->GetBoardDesignSettings()->m_SolderPasteMargin = tmp;
        }

        else if( TESTLINE( "Pad2PasteClearanceRatio" ) )
        {
            double ratio = atof( line + SZ( "Pad2PasteClearanceRatio" ) );
            m_board->GetBoardDesignSettings()->m_SolderPasteMarginRatio = ratio;
        }

        else if( TESTLINE( "GridOrigin" ) )
        {
            BIU gx = biuParse( line + SZ( "GridOrigin" ), &data );
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
    auto_ptr<MODULE> module( new MODULE( m_board ) );

    while( aReader->ReadLine() )
    {
        const char* data;
        char* line = aReader->Line();

        // most frequently encountered ones at the top

        if( TESTLINE( "D" ) )          // read a drawing item
        {
            loadEDGE_MODULE( module.get() );
            /*
            EDGE_MODULE * edge;
            edge = new EDGE_MODULE( this );
            m_Drawings.PushBack( edge );
            edge->ReadDescr( aReader );
            edge->SetDrawCoord();
            */
        }

        else if( TESTLINE( "$PAD" ) )
        {
            loadPAD( module.get() );
        }

        else if( TESTLINE( "T" ) )
        {
            // Read a footprint text description (ref, value, or drawing)
            int tnum = atoi( line + SZ( "T" ) );

            TEXTE_MODULE* textm;

            if( tnum == TEXT_is_REFERENCE )
                textm = module->m_Reference;
            else if( tnum == TEXT_is_VALUE )
                textm = module->m_Value;
            else
            {
                // text is a drawing
                textm = new TEXTE_MODULE( module.get() );
                module->m_Drawings.PushBack( textm );
            }
            loadTEXTE_MODULE( textm );
        }

        else if( TESTLINE( "Po" ) )
        {
            // sscanf( PtLine, "%d %d %d %d %lX %lX %s", &m_Pos.x, &m_Pos.y, &m_Orient, &m_Layer, &m_LastEdit_Time, &m_TimeStamp, BufCar1 );

            BIU pos_x  = biuParse( line + SZ( "Po" ), &data );
            BIU pos_y  = biuParse( data, &data );
            int orient = intParse( data, &data );
            int layer  = intParse( data, &data );

            long edittime  = hexParse( data, &data );
            long timestamp = hexParse( data, &data );

            data = strtok( (char*) data+1, delims );

            // data is now a two character long string
            if( data[0] == 'F' )
                module->SetLocked( true );

            if( data[1] == 'P' )
                module->SetIsPlaced( true );

            module->SetPosition( wxPoint( pos_x, pos_y ) );
            module->SetLayer( layer );
            module->SetOrientation( orient );
            module->SetTimeStamp( timestamp );
            module->SetLastEditTime( edittime );
        }

        else if( TESTLINE( "Li" ) )         // Library name of footprint
        {
            module->m_LibRef = FROM_UTF8( StrPurge( line + SZ( "Li" ) ) );
        }

        else if( TESTLINE( "Sc" ) )         // timestamp
        {
            long timestamp = hexParse( line + SZ( "Sc" ) );
            module->SetTimeStamp( timestamp );
        }

        else if( TESTLINE( "Op" ) )         // (Op)tions for auto placement
        {
            int itmp1 = hexParse( line + SZ( "Op" ), &data );
            int itmp2 = hexParse( data );

            int cntRot180 = itmp2 & 0x0F;
            if( cntRot180 > 10 )
                cntRot180 = 10;

            module->m_CntRot180 = cntRot180;

            int cntRot90  = itmp1 & 0x0F;
            if( cntRot90 > 10 )
                cntRot90 = 0;

            itmp1 = (itmp1 >> 4) & 0x0F;
            if( itmp1 > 10 )
                itmp1 = 0;

            module->m_CntRot90 = (itmp1 << 4) | cntRot90;
        }

        else if( TESTLINE( "At" ) )         // (At)tributes of module
        {
            data = line + SZ( "At" );

            if( strstr( data, "SMD" ) )
                module->m_Attributs |= MOD_CMS;

            if( strstr( data, "VIRTUAL" ) )
                module->m_Attributs |= MOD_VIRTUAL;
        }

        else if( TESTLINE( "AR" ) )         // Alternate Reference
        {
            // e.g. "AR /47BA2624/45525076"
            data = strtok( line + SZ( "AR" ), delims );
            module->m_Path = FROM_UTF8( data );
        }

        else if( TESTLINE( "SHAPE3D" ) )
        {
            load3D( module.get() );
        }

        else if( TESTLINE( "Cd" ) )
        {
            module->m_Doc = FROM_UTF8( StrPurge( line + SZ( "Cd" ) ) );
        }

        else if( TESTLINE( "Kw" ) )         // Key words
        {
            module->m_KeyWord = FROM_UTF8( StrPurge( line + SZ( "Kw" ) ) );
        }

        else if( TESTLINE( "$EndMODULE" ) )
        {
            module->CalculateBoundingBox();

            m_board->Add( module.release(), ADD_APPEND );

            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndMODULE'" ) );
}


void KICAD_PLUGIN::loadPAD( MODULE* aModule )
{
    auto_ptr<D_PAD> pad( new D_PAD( aModule ) );

    while( aReader->ReadLine() )
    {
        const char* data;
        char* line = aReader->Line();

        if( TESTLINE( "Sh" ) )     // padname and shape
        {
            // e.g. "Sh "A2" C 520 520 0 0 900"
            //      "Sh "1" R 157 1378 0 0 900"

            char    padname[sizeof(pad->m_Padname)+1];

            data = line + SZ( "Sh" );

            data = data + ReadDelimitedText( padname, data, sizeof(padname) ) + 1;

            // sscanf( PtLine, " %s %d %d %d %d %d", BufCar, &m_Size.x, &m_Size.y, &m_DeltaSize.x, &m_DeltaSize.y, &m_Orient );

            int padshape = *data++;
            BIU size_x   = biuParse( data, &data );
            BIU size_y   = biuParse( data, &data );
            BIU delta_x  = biuParse( data, &data );
            BIU delta_y  = biuParse( data, &data );
            int orient   = intParse( data );

            switch( padshape )
            {
            default:
            case 'C':   padshape = PAD_CIRCLE;      break;
            case 'R':   padshape = PAD_RECT;        break;
            case 'O':   padshape = PAD_OVAL;        break;
            case 'T':   padshape = PAD_TRAPEZOID;   break;
            }

            pad->SetSize( wxSize( size_x, size_y ) );
            pad->SetDelta( wxSize( delta_x, delta_y ) );
            pad->SetOrientation( orient );
            pad->ComputeShapeMaxRadius();
        }

        else if( TESTLINE( "Dr" ) )     // drill
        {
            // e.g. "Dr 350 0 0" or "Dr 0 0 0 O 0 0"
            // sscanf( PtLine, "%d %d %d %s %d %d", &m_Drill.x, &m_Offset.x, &m_Offset.y, BufCar, &dx, &dy );

            BIU drill_x = biuParse( line + SZ( "Dr" ), &data );
            BIU drill_y = drill_x;
            BIU offs_x  = biuParse( data, &data );
            BIU offs_y  = biuParse( data, &data );
            int drShape = PAD_CIRCLE;

            data = strtok( (char*) data, delims );
            if( data )  // optional shape
            {
                if( data[0] == 'O' )
                {
                    drShape = PAD_OVAL;

                    data    = strtok( NULL, delims );
                    drill_x = biuParse( data );

                    data    = strtok( NULL, delims );
                    drill_y = biuParse( data );
                }
            }

            pad->SetDrillShape( drShape );
            pad->SetOffset( wxSize( offs_x, offs_y ) );
            pad->SetDrillSize( wxSize( drill_x, drill_y ) );
        }

        else if( TESTLINE( "At" ) )
        {
            // e.g. "At SMD N 00888000"
            // sscanf( PtLine, "%s %s %X", BufLine, BufCar, &m_layerMask );

            int attribute;
            int layer_mask;

            data = strtok( line + SZ( "At" ), delims );

            if( !strcmp( data, "SMD" ) )
                attribute = PAD_SMD;
            else if( !strcmp( data, "CONN" ) )
                attribute = PAD_CONN;
            else if( !strcmp( data, "HOLE" ) )
                attribute = PAD_HOLE_NOT_PLATED;
            else
                attribute = PAD_STANDARD;

            data = strtok( NULL, delims );  // skip BufCar
            data = strtok( NULL, delims );

            layer_mask = hexParse( data );

            pad->SetLayerMask( layer_mask );
            pad->SetAttribute( attribute );
        }

        else if( TESTLINE( "Ne" ) )         // netname
        {
            // e.g. "Ne 461 "V5.0"

            char    buf[1024];  // can be fairly long
            int     netcode = intParse( line + SZ( "Ne" ), &data );

            pad->SetNet( netcode );

            // read Netname
            ReadDelimitedText( buf, data, sizeof(buf) );
            pad->SetNetname( FROM_UTF8( StrPurge( buf ) ) );
        }

        else if( TESTLINE( "Po" ) )
        {
            // e.g. "Po 500 -500"
            wxPoint pos;

            pos.x = biuParse( line + SZ( "Po" ), &data );
            pos.y = biuParse( data );

            pad->SetPos0( pos );
            pad->SetPosition( pos );
        }

        else if( TESTLINE( "Le" ) )
        {
            BIU tmp = biuParse( line + SZ( "Le" ) );
            pad->SetDieLength( tmp );
        }

        else if( TESTLINE( ".SolderMask" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderMask" ) );
            pad->SetSolderMaskMargin( tmp );
        }

        else if( TESTLINE( ".SolderPaste" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderPaste" ) );
            pad->SetSolderPasteMargin( tmp );
        }

        else if( TESTLINE( ".SolderPasteRatio" ) )
        {
            double tmp = atof( line + SZ( ".SolderPasteRatio" ) );
            pad->SetSolderPasteRatio( tmp );
        }

        else if( TESTLINE( ".LocalClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( ".LocalClearance" ) );
            pad->SetPadClearance( tmp );
        }

        else if( TESTLINE( "$EndPAD" ) )
        {
            aModule->m_Pads.PushBack( pad.release() );
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndPAD'" ) );
}


void KICAD_PLUGIN::loadEDGE_MODULE( MODULE* aModule )
{
    // @todo
}


void KICAD_PLUGIN::loadTEXTE_MODULE( TEXTE_MODULE* aText )
{
    // @todo
}


void KICAD_PLUGIN::load3D( MODULE* aModule )
{
    S3D_MASTER* t3D = aModule->m_3D_Drawings;

    if( !t3D->m_Shape3DName.IsEmpty() )
    {
        S3D_MASTER* n3D = new S3D_MASTER( aModule );

        aModule->m_3D_Drawings.PushBack( n3D );

        t3D = n3D;
    }

    while( aReader->ReadLine() )
    {
        char* line = aReader->Line();

        if( TESTLINE( "$EndSHAPE3D" ) )
            return;         // preferred exit

        else if( TESTLINE( "Na" ) )     // Shape File Name
        {
            char    buf[512];
            ReadDelimitedText( buf, line + SZ( "Na" ), sizeof(buf) );
            t3D->m_Shape3DName = FROM_UTF8( buf );
        }

        else if( TESTLINE( "Sc" ) )     // Scale
        {
            sscanf( line + SZ( "Sc" ), "%lf %lf %lf\n",
                    &t3D->m_MatScale.x,
                    &t3D->m_MatScale.y,
                    &t3D->m_MatScale.z );
        }

        else if( TESTLINE( "Of" ) )     // Offset
        {
            sscanf( line + SZ( "Of" ), "%lf %lf %lf\n",
                    &t3D->m_MatPosition.x,
                    &t3D->m_MatPosition.y,
                    &t3D->m_MatPosition.z );
        }

        else if( TESTLINE( "Ro" ) )     // Rotation
        {
            sscanf( line + SZ( "Ro" ), "%lf %lf %lf\n",
                    &t3D->m_MatRotation.x,
                    &t3D->m_MatRotation.y,
                    &t3D->m_MatRotation.z );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndSHAPE3D'" ) );
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

        if( TESTLINE( "$EndDRAWSEGMENT" ) )
            return;     // preferred exit

        else if( TESTLINE( "Po" ) )
        {
            // sscanf( line + 2, " %d %d %d %d %d %d", &m_Shape, &m_Start.x, &m_Start.y, &m_End.x, &m_End.y, &m_Width );
            const char* data = line + SZ( "Po" );

            BIU shape   = biuParse( data, &data );
            BIU start_x = biuParse( data, &data );
            BIU start_y = biuParse( data, &data );
            BIU end_x   = biuParse( data, &data );
            BIU end_y   = biuParse( data, &data );
            BIU width   = biuParse( data );

            if( width < 0 )
                width = 0;

            dseg->SetShape( shape );
            dseg->SetWidth( width );
            dseg->SetStart( wxPoint( start_x, start_y ) );
            dseg->SetEnd( wxPoint( end_x, end_y ) );
        }

        else if( TESTLINE( "De" ) )
        {
            const char* data = strtok( line, " " );    // "De", skip it

            BIU     x = 0;
            BIU     y;
            int     val;

            for( int i = 0;  (data = strtok( NULL, " " )) != NULL;  ++i )
            {
                switch( i )
                {
                case 0:
                    val = atoi( data );

                    if( val < FIRST_NO_COPPER_LAYER )
                        val = FIRST_NO_COPPER_LAYER;

                    else if( val > LAST_NO_COPPER_LAYER )
                        val = LAST_NO_COPPER_LAYER;

                    dseg->SetLayer( val );
                    break;
                case 1:
                    val = atoi( data );
                    dseg->SetType( val );   // m_Type
                    break;
                case 2:
                    val = atoi( data );
                    dseg->SetAngle( val );  // m_Angle
                    break;
                case 3:
                    long    timestamp;
                    timestamp = hexParse( data );
                    dseg->SetTimeStamp( timestamp );
                    break;
                case 4:
                    val = hexParse( data );
                    dseg->SetState( val, ON );
                    break;

                    // Bezier Control Points
                case 5:
                    x = biuParse( data );
                    break;
                case 6:
                    y = biuParse( data );
                    dseg->SetBezControl1( wxPoint( x, y ) );
                    break;

                case 7:
                    x = biuParse( data );
                    break;
                case 8:
                    y = biuParse( data );
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


void KICAD_PLUGIN::loadTrackList( TRACK* aInsertBeforeMe, int aStructType )
{
    while( aReader->ReadLine() )
    {
        // read two lines per loop iteration, each loop is one TRACK or VIA
        // example first line:
        // "Po 0 23994 28800 24400 28800 150 -1\r\n"

        char*       line = aReader->Line();
        BIU         drill = -1;     // SetDefault() if -1

        if( line[0] == '$' )    // $EndTRACK
            return;             // preferred exit

        // int arg_count = sscanf( line + 2, " %d %d %d %d %d %d %d", &shape, &tempStartX, &tempStartY, &tempEndX, &tempEndY, &width, &drill );

        assert( TESTLINE( "Po" ) );

        const char* data = line + SZ( "Po" );

        int shape   = intParse( data, &data );
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

        if( !TESTLINE( "De" ) )
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

        TRACK*  newTrack;   // BOARD insert this new one immediately after instantiation

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

        if( TESTLINE( "AddNet" ) )      // most frequent type of line
        {
            ReadDelimitedText( buf, line + SZ( "AddNet" ), sizeof(buf) );
            netname = FROM_UTF8( buf );
            netclass->Add( netname );
        }

        else if( TESTLINE( "$EndNCLASS" ) )
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

            return;             // preferred exit
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

    THROW_IO_ERROR( wxT( "Missing '$EndNCLASS'" ) );
}


void KICAD_PLUGIN::loadZONE_CONTAINER()
{
    auto_ptr<ZONE_CONTAINER> zc( new ZONE_CONTAINER( m_board ) );

    int     outline_hatch = CPolyLine::NO_HATCH;
    bool    sawCorner = false;
    int     layer = 0;

    while( aReader->ReadLine() )
    {
        char* line = aReader->Line();

        if( TESTLINE( "ZCorner" ) )     // new corner found
        {
            // e.g. "ZCorner 25650 49500 0"

            const char* data = line + SZ( "ZCorner" );

            BIU x    = biuParse( data, &data );
            BIU y    = biuParse( data, &data );
            int flag = atoi( data );

            if( !sawCorner )
                zc->m_Poly->Start( layer, x, y, outline_hatch );
            else
                zc->AppendCorner( wxPoint( x, y ) );

            sawCorner = true;

            if( flag )
                zc->m_Poly->Close();
        }

        else if( TESTLINE( "ZInfo" ) )      // general info found
        {
            // e.g. 'ZInfo 479194B1 310 "COMMON"'

            const char* data = line + SZ( "ZInfo" );

            char    buf[1024];
            long    timestamp = hexParse( data, &data );
            int     netcode   = intParse( data, &data );

            if( ReadDelimitedText( buf, data, sizeof(buf) ) > (int) sizeof(buf) )
            {
                THROW_IO_ERROR( wxT( "ZInfo netname too long" ) );
            }

            zc->SetTimeStamp( timestamp );
            zc->SetNet( netcode );
            zc->SetNetName( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "ZLayer" ) )     // layer found
        {
            char*   data = line + SZ( "ZLayer" );

            layer = atoi( data );
        }

        else if( TESTLINE( "ZAux" ) )       // aux info found
        {
            // e.g. "ZAux 7 E"

            char*   data = line + SZ( "ZAux" );
            int     x;
            char    hopt[10];
            int     ret  = sscanf( data, "%d %c", &x, hopt );

            if( ret < 2 )
            {
                m_error.Printf( wxT( "Bad ZAux for CZONE_CONTAINER '%s'" ), zc->GetNetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            switch( hopt[0] )   // upper case required
            {
            case 'N':
                outline_hatch = CPolyLine::NO_HATCH;
                break;

            case 'E':
                outline_hatch = CPolyLine::DIAGONAL_EDGE;
                break;

            case 'F':
                outline_hatch = CPolyLine::DIAGONAL_FULL;
                break;

            default:
                m_error.Printf( wxT( "Bad ZAux for CZONE_CONTAINER '%s'" ), zc->GetNetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            // Set hatch mode later, after reading corner outline data
        }

        else if( TESTLINE( "ZSmoothing" ) )
        {
            // e.g. "ZSmoothing 0 0"

            const char* data = line + SZ( "ZSmoothing" );

            int     smoothing    = intParse( data, &data );
            BIU     cornerRadius = biuParse( data );

            if( smoothing >= ZONE_SETTING::SMOOTHING_LAST || smoothing < 0 )
            {
                m_error.Printf( wxT( "Bad ZSmoothing for CZONE_CONTAINER '%s'" ), zc->GetNetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            zc->SetCornerSmoothingType( smoothing );
            zc->SetCornerRadius( cornerRadius );
        }

        else if( TESTLINE( "ZOptions" ) )
        {
            // e.g. "ZOptions 0 32 F 200 200"

            const char* data = line + SZ( "ZOptions" );

            int     fillmode    = intParse( data, &data );
            int     arcsegcount = intParse( data, &data );
            char    fillstate   = data[1];      // here e.g. " F"
            BIU     thermalReliefGap = biuParse( data += 2 , &data );  // +=2 for " F"
            BIU     thermalReliefCopperBridge = biuParse( data );

            zc->SetFillMode( fillmode ? 1 : 0 );

            if( arcsegcount >= 32 /* ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF: don't really want pcbnew.h in here, after all, its a PLUGIN and global data is evil. */ )
                arcsegcount = 32;

            zc->SetArcSegCount( arcsegcount );
            zc->SetIsFilled( fillstate == 'S' ? true : false );
            zc->SetThermalReliefGap( thermalReliefGap );
            zc->SetThermalReliefCopperBridge( thermalReliefCopperBridge );
        }

        else if( TESTLINE( "ZClearance" ) )     // Clearance and pad options info found
        {
            // e.g. "ZClearance 40 I"

            const char* data = line + SZ( "ZClearance" );

            BIU     clearance = biuParse( data, &data );
            int     padoption = data[1];            // e.g. " I"

            zc->SetZoneClearance( clearance );

            switch( padoption )
            {
            case 'I':
                padoption = PAD_IN_ZONE;
                break;

            case 'T':
                padoption = THERMAL_PAD;
                break;

            case 'X':
                padoption = PAD_NOT_IN_ZONE;
                break;

            default:
                m_error.Printf( wxT( "Bad ZClearance padoption for CZONE_CONTAINER '%s'" ), zc->GetNetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            zc->SetPadOption( padoption );
        }

        else if( TESTLINE( "ZMinThickness" ) )
        {
            char*   data = line + SZ( "ZMinThickness" );
            BIU     thickness = biuParse( data );

            zc->SetMinThickness( thickness );
        }

        else if( TESTLINE( "$POLYSCORNERS" ) )
        {
            // Read the PolysList (polygons used for fill areas in the zone)

            while( aReader->ReadLine() )
            {
                line = aReader->Line();

                if( TESTLINE( "$endPOLYSCORNERS" ) )
                    break;

                // e.g. "39610 43440 0 0"

                const char* data = line;

                BIU x = biuParse( data, &data );
                BIU y = biuParse( data, &data );

                bool    end_contour = (data[1] == '1');   // end_countour was a bool when file saved, so '0' or '1' here
                int     utility = atoi( data + 3 );

                zc->m_FilledPolysList.push_back( CPolyPt( x, y, end_contour, utility ) );
            }
        }

        else if( TESTLINE( "$FILLSEGMENTS" ) )
        {
            while( aReader->ReadLine() )
            {
                line = aReader->Line();

                if( TESTLINE( "$endFILLSEGMENTS" ) )
                    break;

                const char* data = line;

                BIU sx = biuParse( data, &data );
                BIU sy = biuParse( data, &data );
                BIU ex = biuParse( data, &data );
                BIU ey = biuParse( data );

                zc->m_FillSegmList.push_back( SEGMENT(
                        wxPoint( sx, sy ),
                        wxPoint( ex, ey ) ) );
            }
        }

        else if( TESTLINE( "$endCZONE_OUTLINE" ) )
        {
            // should always occur, but who knows, a zone without two corners
            // is no zone at all, it's a spot?

            if( zc->GetNumCorners() > 2 )
            {
                if( !zc->IsOnCopperLayer() )
                {
                    zc->SetFillMode( 0 );
                    zc->SetNet( 0 );
                }

                // Set hatch here, when outlines corners are read
                zc->m_Poly->SetHatch( outline_hatch );

                m_board->Add( zc.release() );
            }

            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$endCZONE_OUTLINE'" ) );
}


void KICAD_PLUGIN::loadDIMENSION()
{
    auto_ptr<DIMENSION> dim( new DIMENSION( m_board ) );

    while( aReader->ReadLine() )
    {
        const char*  data;
        char* line = aReader->Line();

        if( TESTLINE( "$EndDIMENSION" ) )
        {
            m_board->Add( dim.release(), ADD_APPEND );
            return;     // preferred exit
        }

        else if( TESTLINE( "Va" ) )
        {
            BIU value = biuParse( line + SZ( "Va" ) );
            dim->m_Value = value;
        }

        else if( TESTLINE( "Ge" ) )
        {
            int     layer;
            long    timestamp;
            int     shape;

            sscanf( line + SZ( "Ge" ), " %d %d %lX", &shape, &layer, &timestamp );

            if( layer < FIRST_NO_COPPER_LAYER )
                layer = FIRST_NO_COPPER_LAYER;

            else if( layer > LAST_NO_COPPER_LAYER )
                layer = LAST_NO_COPPER_LAYER;

            dim->SetLayer( layer );
            dim->SetTimeStamp( timestamp );
            dim->m_Shape = shape;
        }

        else if( TESTLINE( "Te" ) )
        {
            char  buf[2048];

            ReadDelimitedText( buf, line + SZ( "Te" ), sizeof(buf) );
            dim->m_Text->SetText( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Po" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d %d", &m_Text->m_Pos.x, &m_Text->m_Pos.y,
            // &m_Text->m_Size.x, &m_Text->m_Size.y, &thickness, &orientation, &normal_display );

            int normal_display = 1;

            BIU pos_x  = biuParse( line + SZ( "Po" ), &data );
            BIU pos_y  = biuParse( data, &data );
            BIU width  = biuParse( data, &data );
            BIU height = biuParse( data, &data );
            BIU thickn = biuParse( data, &data );
            int orient = intParse( data, &data );

            data = strtok( (char*) data, delims );
            if( data )  // optional from old days?
                normal_display = intParse( data );

            // This sets both DIMENSION's position and internal m_Text's.
            // @todo: But why do we even know about internal m_Text?
            dim->SetPosition( wxPoint( pos_x, pos_y ) );
            dim->SetTextSize( wxSize( width, height ) );

            dim->m_Text->m_Mirror = normal_display ? false : true;

            dim->m_Text->SetThickness( thickn );
            dim->m_Text->SetOrientation( orient );
        }

        else if( TESTLINE( "Sb" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_crossBarOx, &m_crossBarOy, &m_crossBarFx, &m_crossBarFy, &m_Width );

            int ignore     = biuParse( line + SZ( "Sb" ), &data );
            BIU crossBarOx = biuParse( data, &data );
            BIU crossBarOy = biuParse( data, &data );
            BIU crossBarFx = biuParse( data, &data );
            BIU crossBarFy = biuParse( data, &data );
            BIU width      = biuParse( data );

            dim->m_crossBarOx = crossBarOx;
            dim->m_crossBarOy = crossBarOy;
            dim->m_crossBarFx = crossBarFx;
            dim->m_crossBarFy = crossBarFy;
            dim->m_Width = width;
            (void) ignore;
        }

        else if( TESTLINE( "Sd" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_featureLineDOx, &m_featureLineDOy, &m_featureLineDFx, &m_featureLineDFy, &Dummy );

            int ignore         = intParse( line + SZ( "Sd" ), &data );
            BIU featureLineDOx = biuParse( data, &data );
            BIU featureLineDOy = biuParse( data, &data );
            BIU featureLineDFx = biuParse( data, &data );
            BIU featureLineDFy = biuParse( data );

            dim->m_featureLineDOx = featureLineDOx;
            dim->m_featureLineDOy = featureLineDOy;
            dim->m_featureLineDFx = featureLineDFx;
            dim->m_featureLineDFy = featureLineDFy;
            (void) ignore;
        }

        else if( TESTLINE( "Sg" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_featureLineGOx, &m_featureLineGOy, &m_featureLineGFx, &m_featureLineGFy, &Dummy );

            int ignore         = intParse( line + SZ( "Sg" ), &data );
            BIU featureLineGOx = biuParse( data, &data );
            BIU featureLineGOy = biuParse( data, &data );
            BIU featureLineGFx = biuParse( data, &data );
            BIU featureLineGFy = biuParse( data );

            dim->m_featureLineGOx = featureLineGOx;
            dim->m_featureLineGOy = featureLineGOy;
            dim->m_featureLineGFx = featureLineGFx;
            dim->m_featureLineGFy = featureLineGFy;
            (void) ignore;
        }

        else if( TESTLINE( "S1" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_arrowD1Ox, &m_arrowD1Oy, &m_arrowD1Fx, &m_arrowD1Fy, &Dummy );

            int ignore      = intParse( line + SZ( "S1" ), &data );
            BIU arrowD10x   = biuParse( data, &data );
            BIU arrowD10y   = biuParse( data, &data );
            BIU arrowD1Fx   = biuParse( data, &data );
            BIU arrowD1Fy   = biuParse( data );

            dim->m_arrowD1Ox = arrowD10x;
            dim->m_arrowD1Oy = arrowD10y;
            dim->m_arrowD1Fx = arrowD1Fx;
            dim->m_arrowD1Fy = arrowD1Fy;
            (void) ignore;
        }

        else if( TESTLINE( "S2" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_arrowD2Ox, &m_arrowD2Oy, &m_arrowD2Fx, &m_arrowD2Fy, &Dummy );

            int ignore    = intParse( line + SZ( "S2" ), &data );
            BIU arrowD2Ox = biuParse( data, &data );
            BIU arrowD2Oy = biuParse( data, &data );
            BIU arrowD2Fx = biuParse( data, &data );
            BIU arrowD2Fy = biuParse( data, &data );

            dim->m_arrowD2Ox = arrowD2Ox;
            dim->m_arrowD2Oy = arrowD2Oy;
            dim->m_arrowD2Fx = arrowD2Fx;
            dim->m_arrowD2Fy = arrowD2Fy;
            (void) ignore;
        }

        else if( TESTLINE( "S3" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d\n", &Dummy, &m_arrowG1Ox, &m_arrowG1Oy, &m_arrowG1Fx, &m_arrowG1Fy, &Dummy );
            int ignore    = intParse( line + SZ( "S3" ), &data );
            BIU arrowG1Ox = biuParse( data, &data );
            BIU arrowG1Oy = biuParse( data, &data );
            BIU arrowG1Fx = biuParse( data, &data );
            BIU arrowG1Fy = biuParse( data, &data );

            dim->m_arrowG1Ox = arrowG1Ox;
            dim->m_arrowG1Oy = arrowG1Oy;
            dim->m_arrowG1Fx = arrowG1Fx;
            dim->m_arrowG1Fy = arrowG1Fy;
            (void) ignore;
        }

        else if( TESTLINE( "S4" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_arrowG2Ox, &m_arrowG2Oy, &m_arrowG2Fx, &m_arrowG2Fy, &Dummy );
            int ignore    = intParse( line + SZ( "S4" ), &data );
            BIU arrowG2Ox = biuParse( data, &data );
            BIU arrowG2Oy = biuParse( data, &data );
            BIU arrowG2Fx = biuParse( data, &data );
            BIU arrowG2Fy = biuParse( data, &data );

            dim->m_arrowG2Ox = arrowG2Ox;
            dim->m_arrowG2Oy = arrowG2Oy;
            dim->m_arrowG2Fx = arrowG2Fx;
            dim->m_arrowG2Fy = arrowG2Fy;
            (void) ignore;
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndDIMENSION'" ) );
}


void KICAD_PLUGIN::loadPCB_TARGET()
{
    while( aReader->ReadLine() )
    {
        char* line = aReader->Line();

        if( TESTLINE( "$EndPCB_TARGET" ) )
        {
            return;     // preferred exit
        }

        else if( TESTLINE( "Po" ) )
        {
            const char* data;

            // sscanf( Line + 2, " %X %d %d %d %d %d %lX", &m_Shape, &m_Layer, &m_Pos.x, &m_Pos.y, &m_Size, &m_Width, &m_TimeStamp );

            int shape = intParse( line + SZ( "Po" ), &data );
            int layer = intParse( data, &data );
            BIU pos_x = biuParse( data, &data );
            BIU pos_y = biuParse( data, &data );
            BIU size  = biuParse( data, &data );
            BIU width = biuParse( data, &data );
            long timestamp = hexParse( data );

            if( layer < FIRST_NO_COPPER_LAYER )
                layer = FIRST_NO_COPPER_LAYER;

            else if( layer > LAST_NO_COPPER_LAYER )
                layer = LAST_NO_COPPER_LAYER;

            PCB_TARGET* t = new PCB_TARGET( m_board, shape, layer, wxPoint( pos_x, pos_y ), size, width );
            m_board->Add( t, ADD_APPEND );

            t->SetTimeStamp( timestamp );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndDIMENSION'" ) );
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

    if( errno )
    {
        m_error.Printf( _( "invalid float number in file: '%s' on line: %d" ),
            aReader->GetSource().GetData(), aReader->LineNumber() );

        THROW_IO_ERROR( m_error );
    }

    if( aValue == nptr )
    {
        m_error.Printf( _( "missing float number in file: '%s' on line: %d" ),
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

