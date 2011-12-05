
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

    The philosophies:
    *) a BIU is a unit of length and is nanometers when this work is done, but deci-mils until done.
    *) BIUs should be typed as such to distinguish them from ints.  This is mostly
       for human readability, and having the type nearby in the source supports this readability.
    *) Do not assume that BIUs will always be int, doing a sscanf() into a BIU
       does not make sense in case the size of the BUI changes.
    *) variables are put onto the stack in an automatic, even when it might look
       more efficient to do otherwise.  This is so we can seem them with a debugger.
    *) Global variables should not be touched from within a PLUGIN, since it will eventually
       be in a DLL/DSO.  This includes window information too.  The PLUGIN API knows
       nothing of wxFrame or globals.
    *) No wxWindowing calls are made in here, since the UI resides higher up than in here,
       and is going to process a bucket of detailed information thrown from down here
       in the form of an exception if an error happens.
    *) Much of what we do in this source file is for human readability, not performance.
       Simply avoiding strtok() more often than the old code washes out performance losses.
       Remember strncmp() will bail as soon as a mismatch happens, not going all the way
       to end of string unless a full match.
    *) angles are in the process of migrating to doubles, and 'int' if used, is only shortterm.
*/


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <kicad_plugin.h>   // implement this here

#include <auto_ptr.h>
#include <kicad_string.h>
#include <macros.h>
#include <build_version.h>

//#include <fctsys.h>
//#include <confirm.h>
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
#include <class_edge_mod.h>
#include <3d_struct.h>
#include <pcb_plot_params.h>
#include <drawtxt.h>

#include <trigo.h>

#define VERSION_ERROR_FORMAT _( "File '%s' is format version %d.\nI only support format version <= %d.\nPlease upgrade PCBNew to load this file." )

/*
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <autorout.h>
*/



/// C string compare test for a specific length of characters.
/// The -1 is to omit the trailing \0 which is included in sizeof() on a
/// string constant.
#define TESTLINE( x )   (strncmp( line, x, sizeof(x) - 1 ) == 0)

/// Get the length of a string constant, at compile time
#define SZ( x )         (sizeof(x)-1)


#if 1
#define READLINE()     m_reader->ReadLine()

#else
/// The function and macro which follow comprise a shim which can be a
/// monitor on lines of text read in from the input file.
/// And it can be used as a trap.
static inline unsigned ReadLine( LINE_READER* rdr, const char* caller )
{
    unsigned ret = rdr->ReadLine();

    const char* line = rdr->Line();
    printf( "%-6u %s: %s", rdr->LineNumber(), caller, line );

#if 0   // trap
    if( !strcmp( "loadSETUP", caller ) && !strcmp( "$EndSETUP\n", line ) )
    {
        int breakhere = 1;
    }
#endif

    return ret;
}
#define READLINE()     ReadLine( m_reader, __FUNCTION__ )
#endif

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

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // delete on exception, iff I own m_board, according to aAppendToMe
    auto_ptr<BOARD> deleter( aAppendToMe ? NULL : m_board );

    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );
    if( !fp )
    {
        m_error.Printf( _( "Unable to open file '%s'" ), aFileName.GetData() );
        THROW_IO_ERROR( m_error );
    }

    // reader now owns fp, will close on exception or return
    FILE_LINE_READER    reader( fp, aFileName );

    m_reader = &reader;          // member function accessibility

    init( aProperties );

    checkVersion();

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

    while( READLINE() )
    {
        char* line = m_reader->Line();

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
                while( READLINE() )
                {
                    line = m_reader->Line();     // gobble until $EndSetup

                    if( TESTLINE( "$EndSETUP" ) )
                        break;
                }
            }
        }

        else if( TESTLINE( "$EndBOARD" ) )
            return;     // preferred exit

        /*
        else
        {
            printf( "ignored: '%s'", line );
        }
        */
    }

    THROW_IO_ERROR( "Missing '$EndBOARD'" );
}


void KICAD_PLUGIN::checkVersion()
{
    // Read first line and TEST if it is a PCB file format header like this:
    // "PCBNEW-BOARD Version 1 ...."

    m_reader->ReadLine();

    char* line = m_reader->Line();

    if( !TESTLINE( "PCBNEW-BOARD" ) )
    {
        THROW_IO_ERROR( "Unknown file type" );
    }

    int ver = 1;    // if sccanf fails
    sscanf( line, "PCBNEW-BOARD Version %d", &ver );

    if( ver > BOARD_FILE_VERSION )
    {
        m_error.Printf( VERSION_ERROR_FORMAT, ver );
        THROW_IO_ERROR( m_error );
    }
}


void KICAD_PLUGIN::loadGENERAL()
{
    while( READLINE() )
    {
        char*       line = m_reader->Line();
        const char* data;

        if( TESTLINE( "Units" ) )
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
            m_board->GetDesignSettings().m_BoardThickness = thickn;
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

            EDA_RECT bbbox( wxPoint( x1, y1 ), wxSize( x2-x1, y2-y1 ) );

            m_board->SetBoundingBox( bbbox );
        }

        // Read the number of segments of type DRAW, TRACK, ZONE
        else if( TESTLINE( "Ndraw" ) )
        {
            NbDraw = intParse( line + SZ( "Ndraw" ) );
        }

        else if( TESTLINE( "Ntrack" ) )
        {
            NbTrack = intParse( line + SZ( "Ntrack" ) );
        }

        else if( TESTLINE( "Nzone" ) )
        {
            NbZone = intParse( line + SZ( "Nzone" ) );
        }

        else if( TESTLINE( "Nmodule" ) )
        {
            NbMod = intParse( line + SZ( "Nmodule" ) );
        }

        else if( TESTLINE( "Nnets" ) )
        {
            NbNets = intParse( line + SZ( "Nnets" ) );
        }

        else if( TESTLINE( "$EndGENERAL" ) )
            return;     // preferred exit
    }

    THROW_IO_ERROR( "Missing '$EndGENERAL'" );
}


void KICAD_PLUGIN::loadSHEET()
{
    char    buf[260];
    char*   text;

    while( READLINE() )
    {
        char* line = m_reader->Line();

        if( TESTLINE( "Sheet" ) )
        {
            text = strtok( line, delims );
            text = strtok( NULL, delims );

            Ki_PageDescr* sheet = g_SheetSizeList[0];
            int           ii;

            for( ii = 0; sheet != NULL; ii++, sheet = g_SheetSizeList[ii] )
            {
                if( !stricmp( TO_UTF8( sheet->m_Name ), text ) )
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

        else if( TESTLINE( "$EndSHEETDESCR" ) )
            return;             // preferred exit
    }

    THROW_IO_ERROR( "Missing '$EndSHEETDESCR'" );
}


void KICAD_PLUGIN::loadSETUP()
{
    NETCLASS* netclass_default = m_board->m_NetClasses.GetDefault();

    while( READLINE() )
    {
        const char* data;
        char* line = m_reader->Line();

        if( TESTLINE( "PcbPlotParams" ) )
        {
            PCB_PLOT_PARAMS_PARSER parser( line + SZ( "PcbPlotParams" ), m_reader->GetSource() );
            g_PcbPlotOptions.Parse( &parser );
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

        else if( TESTLINE( "Layers" ) )
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
        else if( TESTLINE( "TrackWidth" ) )
        {
        }
        else if( TESTLINE( "ViaSize" ) )
        {
        }
        else if( TESTLINE( "MicroViaSize" ) )
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
            m_board->GetDesignSettings().m_TrackMinWidth = tmp;
        }

        else if( TESTLINE( "ZoneClearence" ) )
        {
            BIU tmp = biuParse( line + SZ( "ZoneClearence" ) );
            g_Zone_Default_Setting.m_ZoneClearance = tmp;
        }

        else if( TESTLINE( "DrawSegmWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "DrawSegmWidth" ) );
            m_board->GetDesignSettings().m_DrawSegmentWidth = tmp;
        }

        else if( TESTLINE( "EdgeSegmWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "EdgeSegmWidth" ) );
            m_board->GetDesignSettings().m_EdgeSegmentWidth = tmp;
        }

        else if( TESTLINE( "ViaMinSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaMinSize" ) );
            m_board->GetDesignSettings().m_ViasMinSize = tmp;
        }

        else if( TESTLINE( "MicroViaMinSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaMinSize" ) );
            m_board->GetDesignSettings().m_MicroViasMinSize = tmp;
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
            m_board->GetDesignSettings().m_ViasMinDrill = tmp;
        }

        else if( TESTLINE( "MicroViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaDrill" ) );
            netclass_default->SetuViaDrill( tmp );
        }

        else if( TESTLINE( "MicroViaMinDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaMinDrill" ) );
            m_board->GetDesignSettings().m_MicroViasMinDrill = tmp;
        }

        else if( TESTLINE( "MicroViasAllowed" ) )
        {
            int tmp = atoi( line + SZ( "MicroViasAllowed" ) );
            m_board->GetDesignSettings().m_MicroViasAllowed = tmp;
        }

        else if( TESTLINE( "TextPcbWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TextPcbWidth" ) );
            m_board->GetDesignSettings().m_PcbTextWidth = tmp;
        }

        else if( TESTLINE( "TextPcbSize" ) )
        {
            BIU x = biuParse( line + SZ( "TextPcbSize" ), &data );
            BIU y = biuParse( data );

            m_board->GetDesignSettings().m_PcbTextSize = wxSize( x, y );
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
            m_board->GetDesignSettings().m_SolderMaskMargin = tmp;
        }

        else if( TESTLINE( "Pad2PasteClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( "Pad2PasteClearance" ) );
            m_board->GetDesignSettings().m_SolderPasteMargin = tmp;
        }

        else if( TESTLINE( "Pad2PasteClearanceRatio" ) )
        {
            double ratio = atof( line + SZ( "Pad2PasteClearanceRatio" ) );
            m_board->GetDesignSettings().m_SolderPasteMarginRatio = ratio;
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

            return;     // preferred exit
        }
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

    while( READLINE() )
    {
        const char* data;
        char* line = m_reader->Line();

        // most frequently encountered ones at the top

        if( TESTLINE( "D" ) )          // read a drawing item, e.g. "DS"
        {
            loadEDGE_MODULE( module.get() );
            /*
            EDGE_MODULE * edge;
            edge = new EDGE_MODULE( this );
            m_Drawings.PushBack( edge );
            edge->ReadDescr( m_reader );
            edge->SetDrawCoord();
            */
        }

        else if( TESTLINE( "$PAD" ) )
        {
            loadPAD( module.get() );
        }

        // Read a footprint text description (ref, value, or drawing)
        else if( TESTLINE( "T" ) )
        {
            // e.g. "T1 6940 -16220 350 300 900 60 M I 20 N "CFCARD"\r\n"

            int tnum = intParse( line + SZ( "T" ) );

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
            // e.g. "Po 19120 39260 900 0 4E823D06 46EAAFA5 ~~\r\n"

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

        else if( TESTLINE( "$SHAPE3D" ) )
        {
            load3D( module.get() );
        }

        else if( TESTLINE( "Cd" ) )
        {
            // e.g. "Cd Double rangee de contacts 2 x 4 pins\r\n"
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

    THROW_IO_ERROR( "Missing '$EndMODULE'" );
}


void KICAD_PLUGIN::loadPAD( MODULE* aModule )
{
    auto_ptr<D_PAD> pad( new D_PAD( aModule ) );

    while( READLINE() )
    {
        const char* data;
        char* line = m_reader->Line();

        if( TESTLINE( "Sh" ) )              // (Sh)ape and padname
        {
            // e.g. "Sh "A2" C 520 520 0 0 900"
            // or   "Sh "1" R 157 1378 0 0 900"

            char    padname[sizeof(pad->m_Padname)+1];

            data = line + SZ( "Sh" ) + 1;   // +1 skips trailing whitespace

            data = data + ReadDelimitedText( padname, data, sizeof(padname) ) + 1;  // +1 trailing whitespace

            // sscanf( PtLine, " %s %d %d %d %d %d", BufCar, &m_Size.x, &m_Size.y, &m_DeltaSize.x, &m_DeltaSize.y, &m_Orient );

            int     padshape = *data++;
            BIU     size_x   = biuParse( data, &data );
            BIU     size_y   = biuParse( data, &data );
            BIU     delta_x  = biuParse( data, &data );
            BIU     delta_y  = biuParse( data, &data );
            double  orient   = degParse( data );

            switch( padshape )
            {
            case 'C':   padshape = PAD_CIRCLE;      break;
            case 'R':   padshape = PAD_RECT;        break;
            case 'O':   padshape = PAD_OVAL;        break;
            case 'T':   padshape = PAD_TRAPEZOID;   break;
            default:
                m_error.Printf( _( "Unknown padshape '%s' on line:%d" ),
                    FROM_UTF8( line ).GetData(), m_reader->LineNumber() );
                THROW_IO_ERROR( m_error );
            }

            pad->SetPadName( padname );
            pad->SetShape( padshape );
            pad->SetSize( wxSize( size_x, size_y ) );
            pad->SetDelta( wxSize( delta_x, delta_y ) );
            pad->SetOrientation( orient );
            pad->ComputeShapeMaxRadius();
        }

        else if( TESTLINE( "Dr" ) )         // (Dr)ill
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

        else if( TESTLINE( "At" ) )         // (At)tribute
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

        else if( TESTLINE( "Ne" ) )         // (Ne)tname
        {
            // e.g. "Ne 461 "V5.0"

            char    buf[1024];  // can be fairly long
            int     netcode = intParse( line + SZ( "Ne" ), &data );

            pad->SetNet( netcode );

            // read Netname
            ReadDelimitedText( buf, data, sizeof(buf) );
            pad->SetNetname( FROM_UTF8( StrPurge( buf ) ) );
        }

        else if( TESTLINE( "Po" ) )         // (Po)sition
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
            RotatePoint( &pad->m_Pos, aModule->GetOrientation() );

            pad->m_Pos += aModule->GetPosition();

            aModule->m_Pads.PushBack( pad.release() );
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$EndPAD'" );
}


void KICAD_PLUGIN::loadEDGE_MODULE( MODULE* aModule )
{
    STROKE_T shape;
    char* line = m_reader->Line();     // obtain current (old) line

    switch( line[1] )
    {
    case 'S':   shape = S_SEGMENT;   break;
    case 'C':   shape = S_CIRCLE;    break;
    case 'A':   shape = S_ARC;       break;
    case 'P':   shape = S_POLYGON;   break;
    default:
        m_error.Printf( wxT( "Unknown EDGE_MODULE type '%s'" ), FROM_UTF8( line ).GetData() );
        THROW_IO_ERROR( m_error );
    }

    auto_ptr<EDGE_MODULE> dwg( new EDGE_MODULE( aModule, shape ) );    // a drawing

    const char* data;

    // common to all cases, and we have to check their values uniformly at end
    BIU     width = 1;
    int     layer = FIRST_NON_COPPER_LAYER;

    switch( shape )
    {
    case S_ARC:
        {
            // sscanf( Line + 3, "%d %d %d %d %d %d %d", &m_Start0.x, &m_Start0.y, &m_End0.x, &m_End0.y, &m_Angle, &m_Width, &m_Layer );
            BIU     start0_x = biuParse( line + SZ( "DA" ), &data );
            BIU     start0_y = biuParse( data, &data );
            BIU     end0_x   = biuParse( data, &data );
            BIU     end0_y   = biuParse( data, &data );
            double  angle    = degParse( data, &data );

            width   = biuParse( data, &data );
            layer   = intParse( data );

            dwg->SetAngle( angle );
            dwg->m_Start0 = wxPoint( start0_x, start0_y );
            dwg->m_End0   = wxPoint( end0_x, end0_y );
        }
        break;

    case S_SEGMENT:
    case S_CIRCLE:
        {
            // e.g. "DS -7874 -10630 7874 -10630 50 20\r\n"
            // sscanf( Line + 3, "%d %d %d %d %d %d", &m_Start0.x, &m_Start0.y, &m_End0.x, &m_End0.y, &m_Width, &m_Layer );

            BIU     start0_x = biuParse( line + SZ( "DS" ), &data );
            BIU     start0_y = biuParse( data, &data );
            BIU     end0_x   = biuParse( data, &data );
            BIU     end0_y   = biuParse( data, &data );

            width   = biuParse( data, &data );
            layer   = intParse( data );

            dwg->m_Start0 = wxPoint( start0_x, start0_y );
            dwg->m_End0   = wxPoint( end0_x, end0_y );
        }
        break;

    case S_POLYGON:
        {
            // e.g. "DP %d %d %d %d %d %d %d\n"
            // sscanf( Line + 3, "%d %d %d %d %d %d %d", &m_Start0.x, &m_Start0.y, &m_End0.x, &m_End0.y, &pointCount, &m_Width, &m_Layer );

            BIU start0_x = biuParse( line + SZ( "DP" ), &data );
            BIU start0_y = biuParse( data, &data );
            BIU end0_x   = biuParse( data, &data );
            BIU end0_y   = biuParse( data, &data );
            int ptCount  = intParse( data, &data );

            width   = biuParse( data, &data );
            layer   = intParse( data );

            dwg->m_Start0 = wxPoint( start0_x, start0_y );
            dwg->m_End0   = wxPoint( end0_x, end0_y );

            std::vector<wxPoint>& pts = dwg->GetPolyPoints();
            pts.reserve( ptCount );

            for( int ii = 0;  ii<ptCount;  ++ii )
            {
                if( !READLINE() )
                {
                    THROW_IO_ERROR( "S_POLGON point count mismatch." );
                }

                line = m_reader->Line();

                // e.g. "Dl 23 44\n"

                if( !TESTLINE( "Dl" ) )
                {
                    THROW_IO_ERROR( "Missing Dl point def" );
                }

                BIU x = biuParse( line + SZ( "Dl" ), &data );
                BIU y = biuParse( data );

                pts.push_back( wxPoint( x, y ) );
            }
        }
        break;

    default:
        // first switch code above prevents us from getting here.
        break;
    }

    // Check for a reasonable width:

    /* @todo no MAX_WIDTH in out of reach header.
    if( width <= 1 )
        width = 1;
    else if( width > MAX_WIDTH )
        width = MAX_WIDTH;
    */

    // Check for a reasonable layer:
    // m_Layer must be >= FIRST_NON_COPPER_LAYER, but because microwave footprints
    // can use the copper layers m_Layer < FIRST_NON_COPPER_LAYER is allowed.
    // @todo: changes use of EDGE_MODULE these footprints and allows only
    // m_Layer >= FIRST_NON_COPPER_LAYER
    if( layer < 0 || layer > LAST_NON_COPPER_LAYER )
        layer = SILKSCREEN_N_FRONT;

    dwg->SetWidth( width );
    dwg->SetLayer( layer );

    EDGE_MODULE* em = dwg.release();

    aModule->m_Drawings.PushBack( em );

    // this had been done at the MODULE level before, presumably because it needs
    // to be already added to a module before this function will work.
    em->SetDrawCoord();
}


void KICAD_PLUGIN::loadTEXTE_MODULE( TEXTE_MODULE* aText )
{
    const char* data;
    char* line = m_reader->Line();     // current (old) line

    // sscanf( line + 1, "%d %d %d %d %d %d %d %s %s %d %s", &type, &m_Pos0.x, &m_Pos0.y, &m_Size.y, &m_Size.x,
    //     &m_Orient, &m_Thickness,               BufCar1, BufCar2, &layer, BufCar3 ) >= 10 )

    // e.g. "T1 6940 -16220 350 300 900 60 M I 20 N "CFCARD"\r\n"

    int     type    = intParse( line+1, &data );
    BIU     pos0_x  = biuParse( data, &data );
    BIU     pos0_y  = biuParse( data, &data );
    BIU     size0_y = biuParse( data, &data );      // why y?
    BIU     size0_x = biuParse( data, &data );
    double  orient  = degParse( data, &data );
    BIU     thickn  = biuParse( data, &data );

    // after switching to strtok, there's no easy coming back because of the
    // embedded nul(s?) placed to the right of the current field.
    char*   mirror  = strtok( (char*) data, delims );
    char*   hide    = strtok( NULL, delims );
    char*   tmp     = strtok( NULL, delims );
    int     layer   = tmp ? intParse( tmp ) : SILKSCREEN_N_FRONT;
    char*   italic  = strtok( NULL, delims );
    char*   text    = strtok( NULL, delims );

    if( type != TEXT_is_REFERENCE && type != TEXT_is_VALUE )
        type = TEXT_is_DIVERS;

    aText->SetType( type );

    aText->SetPos0( wxPoint( pos0_x, pos0_y ) );

    /* @todo move to accessor?  cannot reach these defines from here
        pcbnew.h off limit because of globals in there
    // Test for a reasonable size:
    if( size0_x < TEXTS_MIN_SIZE )
        size0_x = TEXTS_MIN_SIZE;
    if( size0_y < TEXTS_MIN_SIZE )
        size0_y = TEXTS_MIN_SIZE;
    */

    aText->SetSize( wxSize( size0_x, size0_y ) );

    // Due to the Pcbnew history, .m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint

    // @todo is there now an opportunity for a better way as we move to degrees and
    // a new file format?
    orient -= ( (MODULE*) aText->GetParent() )->GetOrientation();

    aText->SetOrientation( orient );

    // @todo put in accessors?
    // Set a reasonable width:
    if( thickn < 1 )
        thickn = 1;

    aText->SetThickness( Clamp_Text_PenSize( thickn, aText->GetSize() ) );

    aText->SetMirrored( mirror && *mirror == 'M' );

    aText->SetVisible( !(hide && *hide == 'I') );

    aText->SetItalic( italic && *italic == 'I' );

    // @todo put in accessor?
    // Test for a reasonable layer:
    if( layer < 0 )
        layer = 0;
    if( layer > LAST_NO_COPPER_LAYER )
        layer = LAST_NO_COPPER_LAYER;
    if( layer == LAYER_N_BACK )
        layer = SILKSCREEN_N_BACK;
    else if( layer == LAYER_N_FRONT )
        layer = SILKSCREEN_N_FRONT;

    aText->SetLayer( layer );

    // Calculate the actual position.
    aText->SetDrawCoord();

    // convert the "quoted, escaped, UTF8, text" to a wxString
    ReadDelimitedText( &m_field, text ? text : "" );

    aText->SetText( m_field );
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

    while( READLINE() )
    {
        char* line = m_reader->Line();

        if( TESTLINE( "Na" ) )     // Shape File Name
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

        else if( TESTLINE( "$EndSHAPE3D" ) )
            return;         // preferred exit
    }

    THROW_IO_ERROR( "Missing '$EndSHAPE3D'" );
}


void KICAD_PLUGIN::loadDRAWSEGMENT()
{
    /* example:
        $DRAWSEGMENT
        Po 0 57500 -1000 57500 0 150
        De 24 0 900 0 0
        $EndDRAWSEGMENT
    */

    auto_ptr<DRAWSEGMENT> dseg( new DRAWSEGMENT( m_board ) );

    while( READLINE() )
    {
        const char* data;
        char* line  = m_reader->Line();

        if( TESTLINE( "Po" ) )
        {
            // sscanf( line + 2, " %d %d %d %d %d %d", &m_Shape, &m_Start.x, &m_Start.y, &m_End.x, &m_End.y, &m_Width );
            int shape   = intParse( line + SZ( "Po" ), &data );
            BIU start_x = biuParse( data, &data );
            BIU start_y = biuParse( data, &data );
            BIU end_x   = biuParse( data, &data );
            BIU end_y   = biuParse( data, &data );
            BIU width   = biuParse( data );

            // @todo put in accessor?  why 0?
            if( width < 0 )
                width = 0;

            dseg->SetShape( shape );
            dseg->SetWidth( width );
            dseg->SetStart( wxPoint( start_x, start_y ) );
            dseg->SetEnd( wxPoint( end_x, end_y ) );
        }

        else if( TESTLINE( "De" ) )
        {
            BIU     x = 0;
            BIU     y;

            data = strtok( line + SZ( "De" ), delims );
            for( int i = 0;  data;  ++i, data = strtok( NULL, delims ) )
            {
                switch( i )
                {
                case 0:
                    int layer;
                    layer = intParse( data );

                    // @todo: put in accessor?
                    if( layer < FIRST_NO_COPPER_LAYER )
                        layer = FIRST_NO_COPPER_LAYER;

                    else if( layer > LAST_NO_COPPER_LAYER )
                        layer = LAST_NO_COPPER_LAYER;

                    dseg->SetLayer( layer );
                    break;
                case 1:
                    int mtype;
                    mtype = intParse( data );
                    dseg->SetType( mtype );   // m_Type
                    break;
                case 2:
                    double angle;
                    angle = degParse( data );
                    dseg->SetAngle( angle );    // m_Angle
                    break;
                case 3:
                    long    timestamp;
                    timestamp = hexParse( data );
                    dseg->SetTimeStamp( timestamp );
                    break;
                case 4:
                    int state;
                    state = hexParse( data );
                    dseg->SetState( state, ON );
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

        else if( TESTLINE( "$EndDRAWSEGMENT" ) )
        {
            m_board->Add( dseg.release(), ADD_APPEND );
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$EndDRAWSEGMENT'" );
}


void KICAD_PLUGIN::loadNETINFO_ITEM()
{
    char  buf[1024];

    NETINFO_ITEM* net = new NETINFO_ITEM( m_board );
    m_board->m_NetInfo->AppendNet( net );

    while( READLINE() )
    {
        const char* data;
        char* line = m_reader->Line();

        if( TESTLINE( "Na" ) )
        {
            // e.g. "Na 58 "/cpu.sch/PAD7"\r\n"

            int tmp = intParse( line + SZ( "Na" ), &data );
            net->SetNet( tmp );

            ReadDelimitedText( buf, data, sizeof(buf) );
            net->SetNetname( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "$EndEQUIPOT" ) )
            return;     // preferred exit
    }

    THROW_IO_ERROR( "Missing '$EndEQUIPOT'" );
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

    while( READLINE() )
    {
        const char* data;
        char* line = m_reader->Line();

        if( TESTLINE( "Te" ) )          // Text line (or first line for multi line texts)
        {
            ReadDelimitedText( text, line + 2, sizeof(text) );
            pcbtxt->SetText( FROM_UTF8( text ) );
        }

        else if( TESTLINE( "nl" ) )     // next line of the current text
        {
            ReadDelimitedText( text, line + SZ( "nl" ), sizeof(text) );
            pcbtxt->SetText( pcbtxt->GetText() + '\n' +  FROM_UTF8( text ) );
        }

        else if( TESTLINE( "Po" ) )
        {
            // sscanf( line + 2, " %d %d %d %d %d %d", &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Size.y, &m_Thickness, &m_Orient );
            wxSize  size;

            BIU pos_x   = biuParse( line + SZ( "Po" ), &data );
            BIU pos_y   = biuParse( data, &data );
            size.x      = biuParse( data, &data );
            size.y      = biuParse( data, &data );
            BIU thickn  = biuParse( data, &data );
            int orient  = intParse( data );

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

            thickn = Clamp_Text_PenSize( thickn, size );
            pcbtxt->SetSize( size );

            pcbtxt->SetThickness( thickn );
            pcbtxt->SetOrientation( orient );

            pcbtxt->SetPosition( wxPoint( pos_x, pos_y ) );
        }

        else if( TESTLINE( "De" ) )
        {
            // e.g. "De 21 1 0 Normal C\r\n"
            // sscanf( line + 2, " %d %d %lX %s %c\n", &m_Layer, &normal_display, &m_TimeStamp, style, &hJustify );

            int     layer       = intParse( line + SZ( "De" ), &data );
            int     notMirrored = intParse( data, &data );
            long    timestamp   = hexParse( data, &data );
            char*   style       = strtok( (char*) data, delims );
            char*   hJustify    = strtok( NULL, delims );

            pcbtxt->SetMirrored( !notMirrored );
            pcbtxt->SetTimeStamp( timestamp );
            pcbtxt->SetItalic( !strcmp( style, "Italic" ) );

            GRTextHorizJustifyType hj;

            switch( *hJustify )
            {
            default:
            case 'C':   hj = GR_TEXT_HJUSTIFY_CENTER;     break;
            case 'L':   hj = GR_TEXT_HJUSTIFY_LEFT;       break;
            case 'R':   hj = GR_TEXT_HJUSTIFY_RIGHT;      break;
            }

            pcbtxt->SetHorizJustify( hj );

            if( layer < FIRST_COPPER_LAYER )
                layer = FIRST_COPPER_LAYER;
            else if( layer > LAST_NO_COPPER_LAYER )
                layer = LAST_NO_COPPER_LAYER;

            pcbtxt->SetLayer( layer );

        }

        else if( TESTLINE( "$EndTEXTPCB" ) )
        {
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$EndTEXTPCB'" );
}


void KICAD_PLUGIN::loadTrackList( TRACK* aInsertBeforeMe, int aStructType )
{
    while( READLINE() )
    {
        // read two lines per loop iteration, each loop is one TRACK or VIA
        // example first line:
        // "Po 0 23994 28800 24400 28800 150 -1\r\n"

        char*       line = m_reader->Line();

        if( line[0] == '$' )    // $EndTRACK
            return;             // preferred exit

        // int arg_count = sscanf( line + 2, " %d %d %d %d %d %d %d", &shape, &tempStartX, &tempStartY, &tempEndX, &tempEndY, &width, &drill );

        assert( TESTLINE( "Po" ) );

        const char* data = line + SZ( "Po" );

        int shape   = intParse( data, &data );
        BIU start_x = biuParse( data, &data );
        BIU start_y = biuParse( data, &data );
        BIU end_x   = biuParse( data, &data );
        BIU end_y   = biuParse( data, &data );
        BIU width   = biuParse( data, &data );

        // optional 7th drill parameter (must be optional in an old format?)
        data = strtok( (char*) data, delims );

        BIU drill   = data ? biuParse( data ) : -1;     // SetDefault() if -1

        // Read the 2nd line to determine the exact type, one of:
        // PCB_TRACE_T, PCB_VIA_T, or PCB_ZONE_T.  The type field in 2nd line
        // differentiates between PCB_TRACE_T and PCB_VIA_T.  With virtual
        // functions in use, it is critical to instantiate the PCB_VIA_T
        // exactly.
        READLINE();

        line = m_reader->Line();

        // example second line:
        // "De 0 0 463 0 800000\r\n"

        if( !TESTLINE( "De" ) )
        {
            // mandatory 2nd line is missing
            THROW_IO_ERROR( "Missing 2nd line of a TRACK def" );
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

        newTrack->SetPosition( wxPoint( start_x, start_y ) );
        newTrack->SetEnd( wxPoint( end_x, end_y ) );

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

    THROW_IO_ERROR( "Missing '$EndTRACK'" );
}


void KICAD_PLUGIN::loadNETCLASS()
{
    char        buf[1024];
    wxString    netname;

    // create an empty NETCLASS without a name, but do not add it to the BOARD
    // yet since that would bypass duplicate netclass name checking within the BOARD.
    // store it temporarily in an auto_ptr until successfully inserted into the BOARD
    // just before returning.
    auto_ptr<NETCLASS> nc( new NETCLASS( m_board, wxEmptyString ) );

    while( READLINE() )
    {
        char* line = m_reader->Line();

        if( TESTLINE( "AddNet" ) )      // most frequent type of line
        {
            // e.g. "AddNet "V3.3D"\n"
            ReadDelimitedText( buf, line + SZ( "AddNet" ), sizeof(buf) );
            netname = FROM_UTF8( buf );
            nc->Add( netname );
        }

        else if( TESTLINE( "Clearance" ) )
        {
            BIU tmp = biuParse( line + SZ( "Clearance" ) );
            nc->SetClearance( tmp );
        }

        else if( TESTLINE( "TrackWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackWidth" ) );
            nc->SetTrackWidth( tmp );
        }

        else if( TESTLINE( "ViaDia" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaDia" ) );
            nc->SetViaDiameter( tmp );
        }

        else if( TESTLINE( "ViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaDrill" ) );
            nc->SetViaDrill( tmp );
        }

        else if( TESTLINE( "uViaDia" ) )
        {
            BIU tmp = biuParse( line + SZ( "uViaDia" ) );
            nc->SetuViaDiameter( tmp );
        }

        else if( TESTLINE( "uViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "uViaDrill" ) );
            nc->SetuViaDrill( tmp );
        }

        else if( TESTLINE( "Name" ) )
        {
            ReadDelimitedText( buf, line + SZ( "Name" ), sizeof(buf) );
            nc->SetName( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Desc" ) )
        {
            ReadDelimitedText( buf, line + SZ( "Desc" ), sizeof(buf) );
            nc->SetDescription( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "$EndNCLASS" ) )
        {
            if( m_board->m_NetClasses.Add( nc.get() ) )
            {
                nc.release();
            }
            else
            {
                // Must have been a name conflict, this is a bad board file.
                // User may have done a hand edit to the file.

                // auto_ptr will delete nc on this code path

                m_error.Printf( _( "duplicate NETCLASS name '%s'" ), nc->GetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$EndNCLASS'" );
}


void KICAD_PLUGIN::loadZONE_CONTAINER()
{
    auto_ptr<ZONE_CONTAINER> zc( new ZONE_CONTAINER( m_board ) );

    int     outline_hatch = CPolyLine::NO_HATCH;
    bool    sawCorner = false;
    char    buf[1024];

    while( READLINE() )
    {
        const char* data;
        char* line = m_reader->Line();

        if( TESTLINE( "ZCorner" ) )         // new corner found
        {
            // e.g. "ZCorner 25650 49500 0"
            BIU x    = biuParse( line + SZ( "ZCorner" ), &data );
            BIU y    = biuParse( data, &data );
            int flag = atoi( data );

            if( !sawCorner )
                zc->m_Poly->Start( zc->GetLayer(), x, y, outline_hatch );
            else
                zc->AppendCorner( wxPoint( x, y ) );

            sawCorner = true;

            if( flag )
                zc->m_Poly->Close();
        }

        else if( TESTLINE( "ZInfo" ) )      // general info found
        {
            // e.g. 'ZInfo 479194B1 310 "COMMON"'
            long    timestamp = hexParse( line + SZ( "ZInfo" ), &data );
            int     netcode   = intParse( data, &data );

            if( ReadDelimitedText( buf, data, sizeof(buf) ) > (int) sizeof(buf) )
            {
                THROW_IO_ERROR( "ZInfo netname too long" );
            }

            zc->SetTimeStamp( timestamp );
            zc->SetNet( netcode );
            zc->SetNetName( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "ZLayer" ) )     // layer found
        {
            int layer = intParse( line + SZ( "ZLayer" ) );
            zc->SetLayer( layer );
        }

        else if( TESTLINE( "ZAux" ) )       // aux info found
        {
            // e.g. "ZAux 7 E"
            int     ignore = intParse( line + SZ( "ZAux" ), &data );
            char*   hopt   = strtok( (char*) data, delims );

            if( !hopt )
            {
                m_error.Printf( wxT( "Bad ZAux for CZONE_CONTAINER '%s'" ), zc->GetNetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            switch( *hopt )   // upper case required
            {
            case 'N':   outline_hatch = CPolyLine::NO_HATCH;        break;
            case 'E':   outline_hatch = CPolyLine::DIAGONAL_EDGE;   break;
            case 'F':   outline_hatch = CPolyLine::DIAGONAL_FULL;   break;

            default:
                m_error.Printf( wxT( "Bad ZAux for CZONE_CONTAINER '%s'" ), zc->GetNetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            (void) ignore;

            // Set hatch mode later, after reading corner outline data
        }

        else if( TESTLINE( "ZSmoothing" ) )
        {
            // e.g. "ZSmoothing 0 0"
            int     smoothing    = intParse( line + SZ( "ZSmoothing" ), &data );
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
            int     fillmode    = intParse( line + SZ( "ZOptions" ), &data );
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
            BIU     clearance = biuParse( line + SZ( "ZClearance" ), &data );
            char*   padoption = strtok( (char*) data, delims );  // data: " I"

            int     popt;
            switch( *padoption )
            {
            case 'I':   popt = PAD_IN_ZONE;        break;
            case 'T':   popt = THERMAL_PAD;        break;
            case 'X':   popt = PAD_NOT_IN_ZONE;    break;

            default:
                m_error.Printf( wxT( "Bad ZClearance padoption for CZONE_CONTAINER '%s'" ),
                    zc->GetNetName().GetData() );
                THROW_IO_ERROR( m_error );
            }

            zc->SetZoneClearance( clearance );
            zc->SetPadOption( popt );
        }

        else if( TESTLINE( "ZMinThickness" ) )
        {
            BIU thickness = biuParse( line + SZ( "ZMinThickness" ) );
            zc->SetMinThickness( thickness );
        }

        else if( TESTLINE( "$POLYSCORNERS" ) )
        {
            // Read the PolysList (polygons used for fill areas in the zone)

            while( READLINE() )
            {
                line = m_reader->Line();

                if( TESTLINE( "$endPOLYSCORNERS" ) )
                    break;

                // e.g. "39610 43440 0 0"
                BIU     x = biuParse( line, &data );
                BIU     y = biuParse( data, &data );

                bool    end_contour = intParse( data, &data );  // end_countour was a bool when file saved, so '0' or '1' here
                int     utility     = intParse( data );

                zc->m_FilledPolysList.push_back( CPolyPt( x, y, end_contour, utility ) );
            }
        }

        else if( TESTLINE( "$FILLSEGMENTS" ) )
        {
            while( READLINE() )
            {
                line = m_reader->Line();

                if( TESTLINE( "$endFILLSEGMENTS" ) )
                    break;

                // e.g. ""%d %d %d %d\n"
                BIU sx = biuParse( line, &data );
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

                // Set hatch here, after outlines corners are read
                zc->m_Poly->SetHatch( outline_hatch );

                m_board->Add( zc.release() );
            }

            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$endCZONE_OUTLINE'" );
}


void KICAD_PLUGIN::loadDIMENSION()
{
    auto_ptr<DIMENSION> dim( new DIMENSION( m_board ) );

    while( READLINE() )
    {
        const char*  data;
        char* line = m_reader->Line();

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

    THROW_IO_ERROR( "Missing '$EndDIMENSION'" );
}


void KICAD_PLUGIN::loadPCB_TARGET()
{
    while( READLINE() )
    {
        char* line = m_reader->Line();

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

    THROW_IO_ERROR( "Missing '$EndDIMENSION'" );
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
        m_error.Printf( _( "invalid float number in\nfile: '%s'\nline: %d\noffset: %d" ),
            m_reader->GetSource().GetData(), m_reader->LineNumber(), aValue - m_reader->Line() + 1 );

        THROW_IO_ERROR( m_error );
    }

    if( aValue == nptr )
    {
        m_error.Printf( _( "missing float number in\nfile: '%s'\nline: %d\noffset: %d" ),
            m_reader->GetSource().GetData(), m_reader->LineNumber(), aValue - m_reader->Line() + 1 );

        THROW_IO_ERROR( m_error );
    }

    if( nptrptr )
        *nptrptr = nptr;

    // There should be no rounding issues here, since the values in the file initially
    // came from integers via biuFmt(). In fact this product should be an integer, exactly.
    return BIU( fval * diskToBiu );
}


double KICAD_PLUGIN::degParse( const char* aValue, const char** nptrptr )
{
    char*   nptr;

    errno = 0;

    double fval = strtod( aValue, &nptr );

    if( errno )
    {
        m_error.Printf( _( "invalid float number in\nfile: '%s'\nline: %d\noffset: %d" ),
            m_reader->GetSource().GetData(), m_reader->LineNumber(), aValue - m_reader->Line() + 1 );

        THROW_IO_ERROR( m_error );
    }

    if( aValue == nptr )
    {
        m_error.Printf( _( "missing float number in\nfile: '%s'\nline: %d\noffset: %d" ),
            m_reader->GetSource().GetData(), m_reader->LineNumber(), aValue - m_reader->Line() + 1 );

        THROW_IO_ERROR( m_error );
    }

    if( nptrptr )
        *nptrptr = nptr;

    return fval;
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


void KICAD_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on then off the C locale.
}

