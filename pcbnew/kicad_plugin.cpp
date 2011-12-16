
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
//#include <build_version.h>
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

#include <wx/ffile.h>


//#define KICAD_NANOMETRE


#define VERSION_ERROR_FORMAT    _( "File '%s' is format version: %d.\nI only support format version <= %d.\nPlease upgrade PCBNew to load this file." )
#define UNKNOWN_GRAPHIC_FORMAT  _( "unknown graphic type: %d")
#define UNKNOWN_PAD_FORMAT      _( "unknown pad type: %d")
#define UNKNOWN_PAD_ATTRIBUTE   _( "unknown pad attribute: %d" )



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
            loadPCB_LINE();
        }

        else if( TESTLINE( "$EQUIPOT" ) )
        {
            loadNETINFO_ITEM();
        }

        else if( TESTLINE( "$TEXTPCB" ) )
        {
            loadPCB_TEXT();
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

    m_loading_format_version = ver;
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
            int layer_mask  = hexParse( line + SZ( "Ly" ) );
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
            int tmp = intParse( line + SZ( "NoConn" ) );
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

        /* Read the number of segments of type DRAW, TRACK, ZONE
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
        */

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
                            sheet->m_Size.x = intParse( text );

                        text = strtok( NULL, delims );

                        if( text )
                            sheet->m_Size.y = intParse( text );
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
            m_originAxisPosition.x = gx;
            m_originAxisPosition.y = gy;
            */
        }

#if 1 // defined(PCBNEW)

        else if( TESTLINE( "Layers" ) )
        {
            int tmp = intParse( line + SZ( "Layers" ) );
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
            int tmp = intParse( line + SZ( "MicroViasAllowed" ) );
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
            loadMODULE_EDGE( module.get() );
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
            loadMODULE_TEXT( textm );
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
            int attrs = MOD_DEFAULT;

            data = line + SZ( "At" );

            if( strstr( data, "SMD" ) )
                attrs |= MOD_CMS;

            if( strstr( data, "VIRTUAL" ) )
                attrs |= MOD_VIRTUAL;

            module->SetAttributes( attrs );
        }

        else if( TESTLINE( "AR" ) )         // Alternate Reference
        {
            // e.g. "AR /47BA2624/45525076"
            data = strtok( line + SZ( "AR" ), delims );
            module->SetPath( FROM_UTF8( data ) );
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

        // test this longer similar string before the shorter ".SolderPaste"
        else if( TESTLINE( ".SolderPasteRatio" ) )
        {
            double tmp = atof( line + SZ( ".SolderPasteRatio" ) );
            module->SetLocalSolderPasteMarginRatio( tmp );
        }

        else if( TESTLINE( ".SolderPaste" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderPaste" ) );
            module->SetLocalSolderPasteMargin( tmp );
        }

        else if( TESTLINE( ".SolderMask" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderMask" ) );
            module->SetLocalSolderMaskMargin( tmp );
        }

        else if( TESTLINE( ".LocalClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( ".LocalClearance" ) );
            module->SetLocalClearance( tmp );
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

            // mypadname is LATIN1/CRYLIC for BOARD_FORMAT_VERSION 1,
            // but for BOARD_FORMAT_VERSION 2, it is UTF8 from disk.
            // So we have to go through two code paths.  Moving forward
            // padnames will be in UTF8 on disk, as are all KiCad strings on disk.
            char        mypadname[50];

            data = line + SZ( "Sh" ) + 1;   // +1 skips trailing whitespace

            data = data + ReadDelimitedText( mypadname, data, sizeof(mypadname) ) + 1;  // +1 trailing whitespace

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

            // go through a wxString to establish a universal character set properly
            wxString    padname;

            if( m_loading_format_version == 1 )
            {
                // add 8 bit bytes, file format 1 was KiCad font type byte,
                // simply promote those 8 bit bytes up into UNICODE. (subset of LATIN1)
                const unsigned char* cp = (unsigned char*) mypadname;
                while( *cp )
                {
                    padname += *cp++;  // unsigned, ls 8 bits only
                }
            }
            else
            {
                // version 2, which is UTF8.
                padname = FROM_UTF8( mypadname );
            }
            // chances are both were ASCII, but why take chances?

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
            pad->SetLocalSolderMaskMargin( tmp );
        }

        // test this before the similar but shorter ".SolderPaste"
        else if( TESTLINE( ".SolderPasteRatio" ) )
        {
            double tmp = atof( line + SZ( ".SolderPasteRatio" ) );
            pad->SetLocalSolderPasteMarginRatio( tmp );
        }

        else if( TESTLINE( ".SolderPaste" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderPaste" ) );
            pad->SetLocalSolderPasteMargin( tmp );
        }

        else if( TESTLINE( ".LocalClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( ".LocalClearance" ) );
            pad->SetLocalClearance( tmp );
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


void KICAD_PLUGIN::loadMODULE_EDGE( MODULE* aModule )
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

            std::vector<wxPoint> pts;
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

            dwg->SetPolyPoints( pts );
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

    // this had been done at the MODULE level before, presumably because the
    // EDGE_MODULE needs to be already added to a module before this function will work.
    em->SetDrawCoord();
}


void KICAD_PLUGIN::loadMODULE_TEXT( TEXTE_MODULE* aText )
{
    const char* data;
    char* line = m_reader->Line();     // current (old) line

    // sscanf( line + 1, "%d %d %d %d %d %d %d %s %s %d %s", &type, &m_Pos0.x, &m_Pos0.y, &m_Size.y, &m_Size.x,
    //     &m_Orient, &m_Thickness,               BufCar1, BufCar2, &layer, BufCar3 ) >= 10 )

    // e.g. "T1 6940 -16220 350 300 900 60 M I 20 N "CFCARD"\r\n"
    // or    T1 0 500 600 400 900 80 M V 20 N"74LS245"
    // ouch, the last example has no space between N and "74LS245" !
    // that is an older version.

    int     type    = intParse( line+1, &data );
    BIU     pos0_x  = biuParse( data, &data );
    BIU     pos0_y  = biuParse( data, &data );
    BIU     size0_y = biuParse( data, &data );
    BIU     size0_x = biuParse( data, &data );
    double  orient  = degParse( data, &data );
    BIU     thickn  = biuParse( data, &data );

    // read the quoted text before the first call to strtok() which introduces
    // NULs into the string and chops it into mutliple C strings, something
    // ReadDelimitedText() cannot traverse.

    // convert the "quoted, escaped, UTF8, text" to a wxString, find it by skipping
    // as far forward as needed until the first double quote.
    ReadDelimitedText( &m_field, data );

    aText->SetText( m_field );

    // after switching to strtok, there's no easy coming back because of the
    // embedded nul(s?) placed to the right of the current field.
    char*   mirror  = strtok( (char*) data, delims );
    char*   hide    = strtok( NULL, delims );
    char*   tmp     = strtok( NULL, delims );
    int     layer   = tmp ? intParse( tmp ) : SILKSCREEN_N_FRONT;
    char*   italic  = strtok( NULL, delims );

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

    /*  this is better left to the save function, or to the accessor, since we will
        be supporting more than one board format.
    aText->SetThickness( Clamp_Text_PenSize( thickn, aText->GetSize() ) );
    */
    aText->SetThickness( thickn );

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


void KICAD_PLUGIN::loadPCB_LINE()
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
                    long timestamp;
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
    m_board->AppendNet( net );

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


void KICAD_PLUGIN::loadPCB_TEXT()
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
            double angle = degParse( data );

            // Ensure the text has minimal size to see this text on screen:

            /* @todo wait until we are firmly in the nanometer world
            if( sz.x < 5 )
                sz.x = 5;

            if( sz.y < 5 )
                sz.y = 5;
            */

            pcbtxt->SetSize( size );

            /* @todo move into an accessor
            // Set a reasonable width:
            if( thickn < 1 )
                thickn = 1;

            thickn = Clamp_Text_PenSize( thickn, size );
            */

            pcbtxt->SetThickness( thickn );
            pcbtxt->SetOrientation( angle );

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

            if( hJustify )
            {
                switch( *hJustify )
                {
                default:
                case 'C':   hj = GR_TEXT_HJUSTIFY_CENTER;     break;
                case 'L':   hj = GR_TEXT_HJUSTIFY_LEFT;       break;
                case 'R':   hj = GR_TEXT_HJUSTIFY_RIGHT;      break;
                }
            }
            else
                hj = GR_TEXT_HJUSTIFY_CENTER;

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

        const char* data;
        char* line = m_reader->Line();

        if( line[0] == '$' )    // $EndTRACK
            return;             // preferred exit

        // int arg_count = sscanf( line + 2, " %d %d %d %d %d %d %d", &shape, &tempStartX, &tempStartY, &tempEndX, &tempEndY, &width, &drill );

        assert( TESTLINE( "Po" ) );

        int shape   = intParse( line + SZ( "Po" ), &data );
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

#if 1
        assert( TESTLINE( "De" ) );
#else
        if( !TESTLINE( "De" ) )
        {
            // mandatory 2nd line is missing
            THROW_IO_ERROR( "Missing 2nd line of a TRACK def" );
        }
#endif

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
            newTrack->SetDrill( drill );

        newTrack->SetLayer( layer );

        if( makeType == PCB_VIA_T )     // Ensure layers are OK when possible:
        {
            if( newTrack->GetShape() == VIA_THROUGH )
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
            int flag = intParse( data );

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

            // @todo ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF: don't really want pcbnew.h
            // in here, after all, its a PLUGIN and global data is evil.
            // put in accessor
            if( arcsegcount >= 32 )
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

        if( TESTLINE( "$endCOTATION" ) )
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
            dim->SetShape( shape );
        }

        else if( TESTLINE( "Te" ) )
        {
            char  buf[2048];

            ReadDelimitedText( buf, line + SZ( "Te" ), sizeof(buf) );
            dim->m_Text.SetText( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Po" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d %d", &m_Text->m_Pos.x, &m_Text->m_Pos.y,
            // &m_Text->m_Size.x, &m_Text->m_Size.y, &thickness, &orientation, &normal_display );

            BIU     pos_x  = biuParse( line + SZ( "Po" ), &data );
            BIU     pos_y  = biuParse( data, &data );
            BIU     width  = biuParse( data, &data );
            BIU     height = biuParse( data, &data );
            BIU     thickn = biuParse( data, &data );
            double  orient = degParse( data, &data );
            char*   mirror = strtok( (char*) data, delims );

            // This sets both DIMENSION's position and internal m_Text's.
            // @todo: But why do we even know about internal m_Text?
            dim->SetPosition( wxPoint( pos_x, pos_y ) );
            dim->SetTextSize( wxSize( width, height ) );

            dim->m_Text.SetMirrored( mirror && *mirror == '0' );

            dim->m_Text.SetThickness( thickn );
            dim->m_Text.SetOrientation( orient );
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

    THROW_IO_ERROR( "Missing '$endCOTATION'" );
}


void KICAD_PLUGIN::loadPCB_TARGET()
{
    while( READLINE() )
    {
        const char* data;
        char* line = m_reader->Line();

        if( TESTLINE( "$EndPCB_TARGET" ) )
        {
            return;     // preferred exit
        }

        else if( TESTLINE( "Po" ) )
        {
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


int KICAD_PLUGIN::biuSprintf( char* buf, BIU aValue ) const
{
    double  engUnits = biuToDisk * aValue;
    int     len;

    if( engUnits != 0.0 && fabs( engUnits ) <= 0.0001 )
    {
        // printf( "f: " );
        len = sprintf( buf, "%.10f", engUnits );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        ++len;
    }
    else
    {
        // printf( "g: " );
        len = sprintf( buf, "%.10g", engUnits );
    }
    return len;
}


std::string KICAD_PLUGIN::fmtBIU( BIU aValue ) const
{
    char    temp[50];

    int len = biuSprintf( temp, aValue );

    return std::string( temp, len );
}


std::string KICAD_PLUGIN::fmtDEG( double aAngle ) const
{
    char    temp[50];

    // @todo a hook site to convert from tenths degrees to degrees for BOARD_FORMAT_VERSION 2.

    int len = sprintf( temp, "%.10g", aAngle );

    return std::string( temp, len );
}


std::string KICAD_PLUGIN::fmtBIUPair( BIU first, BIU second ) const
{
    char    temp[100];
    char*   cp = temp;

    cp += biuSprintf( cp, first );

    *cp++ = ' ';

    cp += biuSprintf( cp, second );

    return std::string( temp, cp - temp );
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
    m_props = aProperties;

    // conversion factor for saving RAM BIUs to KICAD legacy file format.
#if defined(KICAD_NANOMETRE)
    biuToDisk = 1/1000000.0;        // BIUs are nanometers & file is mm
#else
    biuToDisk = 1.0;                // BIUs are deci-mils
#endif


    // conversion factor for loading KICAD legacy file format into BIUs in RAM

    // Start by assuming the *.brd file is in deci-mils.
    // if we see "Units mm" in the $GENERAL section, set diskToBiu to 1000000.0
    // then, during the file loading process, to start a conversion from
    // mm to nanometers.

#if defined(KICAD_NANOMETRE)
    diskToBiu = 2540.0;             // BIUs are nanometers
#else
    diskToBiu = 1.0;                // BIUs are deci-mils
#endif
}


//-----<Save() Functions>-------------------------------------------------------

void KICAD_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    m_board = aBoard;

    FILE* fp = wxFopen( aFileName, wxT( "wt" ) );
    if( !fp )
    {
        m_error.Printf( _( "Unable to open file '%s'" ), aFileName.GetData() );
        THROW_IO_ERROR( m_error );
    }

    m_filename = aFileName;

    // wxf now owns fp, will close on exception or return
    wxFFile wxf( fp );

    m_fp = fp;          // member function accessibility

    init( aProperties );

    if( m_props )
    {
        // save a file header, if caller provided one (with trailing \n hopefully).
        fprintf( m_fp, "%s", TO_UTF8( (*m_props)[ wxT("header") ] ) );
    }

    saveAllSections();
}


wxString KICAD_PLUGIN::writeError() const
{
    return wxString::Format( _( "error writing to file '%s'" ), m_filename.GetData() );
}

#define CHECK_WRITE_ERROR() \
do { \
    if( ferror( m_fp ) ) \
    { \
        THROW_IO_ERROR( writeError() ); \
    } \
} while(0)


void KICAD_PLUGIN::saveAllSections() const
{


    saveGENERAL();

    saveSHEET();

    saveSETUP();

    saveBOARD();
}


void KICAD_PLUGIN::saveGENERAL() const
{
    fprintf( m_fp, "$GENERAL\n" );
    fprintf( m_fp, "encoding utf-8\n" );

    // tell folks the units used within the file, as early as possible here.
#if defined(KICAD_NANOMETRE)
    fprintf( m_fp, "Units mm\n" );
#else
    fprintf( m_fp, "Units deci-mils\n" );
#endif

    // Write copper layer count
    fprintf( m_fp, "LayerCount %d\n", m_board->GetCopperLayerCount() );

    /*  No, EnabledLayers has this information, plus g_TabAllCopperLayerMask is
        global and globals are not allowed in a plugin.
    fprintf( m_fp,
             "Ly %8X\n",
             g_TabAllCopperLayerMask[NbLayers - 1] | ALL_NO_CU_LAYERS );
    */

    fprintf( m_fp, "EnabledLayers %08X\n",  m_board->GetEnabledLayers() );
    fprintf( m_fp, "Links %d\n",            m_board->GetRatsnestsCount() );
    fprintf( m_fp, "NoConn %d\n",           m_board->m_NbNoconnect );

    // Write Bounding box info
    EDA_RECT bbbox = m_board->ComputeBoundingBox();
    fprintf( m_fp,  "Di %s %s\n",
                    fmtBIUPair( bbbox.GetX(), bbbox.GetY() ).c_str(),
                    fmtBIUPair( bbbox.GetRight(), bbbox.GetBottom() ).c_str() );

    fprintf( m_fp, "Ndraw %d\n",            m_board->m_Drawings.GetCount() );
    fprintf( m_fp, "Ntrack %d\n",           m_board->GetNumSegmTrack() );
    fprintf( m_fp, "Nzone %d\n",            m_board->GetNumSegmZone() );
    fprintf( m_fp, "BoardThickness %s\n",   fmtBIU( m_board->GetDesignSettings().m_BoardThickness ).c_str() );
    fprintf( m_fp, "Nmodule %d\n",          m_board->m_Modules.GetCount() );
    fprintf( m_fp, "Nnets %d\n",            m_board->GetNetCount() );
    fprintf( m_fp, "$EndGENERAL\n\n" );
}


void KICAD_PLUGIN::saveSHEET() const
{
#if 0   // @todo sheet not available here.  The sheet needs to go into the board if it is important enough to be saved with the board
    Ki_PageDescr* sheet = screen->m_CurrentSheetDesc;

    fprintf( m_fp, "$SHEETDESCR\n" );

    fprintf( m_fp, "Sheet %s %d %d\n",
                    TO_UTF8( sheet->m_Name ), sheet->m_Size.x, sheet->m_Size.y );   // in mm ?

    fprintf( m_fp, "Title %s\n",        EscapedUTF8( screen->m_Title ).c_str() );
    fprintf( m_fp, "Date %s\n",         EscapedUTF8( screen->m_Date ).c_str() );
    fprintf( m_fp, "Rev %s\n",          EscapedUTF8( screen->m_Revision ).c_str() );
    fprintf( m_fp, "Comp %s\n",         EscapedUTF8( screen->m_Company ).c_str() );
    fprintf( m_fp, "Comment1 %s\n",     EscapedUTF8( screen->m_Commentaire1 ).c_str() );
    fprintf( m_fp, "Comment2 %s\n",     EscapedUTF8( screen->m_Commentaire2 ).c_str() );
    fprintf( m_fp, "Comment3 %s\n",     EscapedUTF8( screen->m_Commentaire3 ).c_str() );
    fprintf( m_fp, "Comment4 %s\n",     EscapedUTF8( screen->m_Commentaire4 ).c_str() );

    fprintf( m_fp, "$EndSHEETDESCR\n\n" );
#endif
}


void KICAD_PLUGIN::saveSETUP() const
{
    NETCLASS* netclass_default = m_board->m_NetClasses.GetDefault();

    fprintf( m_fp, "$SETUP\n" );

    /*  Internal units are nobody's business, they are internal.
        Units used in the file are now in the "Units" attribute of $GENERAL.
    fprintf( m_fp,, "InternalUnit %f INCH\n", 1.0 / PCB_INTERNAL_UNIT );
    */

    fprintf( m_fp, "Layers %d\n", m_board->GetCopperLayerCount() );

    unsigned layerMask = ALL_CU_LAYERS & m_board->GetEnabledLayers();

    for( int layer = 0;  layerMask;  ++layer, layerMask >>= 1 )
    {
        if( layerMask & 1 )
        {
            fprintf( m_fp, "Layer[%d] %s %s\n", layer,
                     TO_UTF8( m_board->GetLayerName( layer ) ),
                     LAYER::ShowType( m_board->GetLayerType( layer ) ) );
        }
    }

    // Save current default track width, for compatibility with older Pcbnew version;
    fprintf( m_fp, "TrackWidth %s\n",  fmtBIU( m_board->GetCurrentTrackWidth() ).c_str() );

    // Save custom tracks width list (the first is not saved here: this is the netclass value
    for( unsigned ii = 1; ii < m_board->m_TrackWidthList.size(); ii++ )
        fprintf( m_fp, "TrackWidthList %s\n", fmtBIU( m_board->m_TrackWidthList[ii] ).c_str() );

    fprintf( m_fp, "TrackClearence %s\n",  fmtBIU( netclass_default->GetClearance() ).c_str() );

    /* @todo no globals in a plugin.
    fprintf( m_fp, "ZoneClearence %d\n", g_Zone_Default_Setting.m_ZoneClearance );
    */

    fprintf( m_fp, "TrackMinWidth %s\n", fmtBIU( m_board->GetDesignSettings().m_TrackMinWidth ).c_str() );

    fprintf( m_fp, "DrawSegmWidth %s\n", fmtBIU( m_board->GetDesignSettings().m_DrawSegmentWidth ).c_str() );
    fprintf( m_fp, "EdgeSegmWidth %s\n", fmtBIU( m_board->GetDesignSettings().m_EdgeSegmentWidth ).c_str() );

    // Save current default via size, for compatibility with older Pcbnew version;
    fprintf( m_fp, "ViaSize %s\n",  fmtBIU( netclass_default->GetViaDiameter() ).c_str() );
    fprintf( m_fp, "ViaDrill %s\n", fmtBIU( netclass_default->GetViaDrill() ).c_str() );
    fprintf( m_fp, "ViaMinSize %s\n", fmtBIU( m_board->GetDesignSettings().m_ViasMinSize ).c_str() );
    fprintf( m_fp, "ViaMinDrill %s\n", fmtBIU( m_board->GetDesignSettings().m_ViasMinDrill ).c_str() );

    // Save custom vias diameters list (the first is not saved here: this is
    // the netclass value
    for( unsigned ii = 1; ii < m_board->m_ViasDimensionsList.size(); ii++ )
        fprintf( m_fp, "ViaSizeList %s %s\n",
                 fmtBIU( m_board->m_ViasDimensionsList[ii].m_Diameter ).c_str(),
                 fmtBIU( m_board->m_ViasDimensionsList[ii].m_Drill ).c_str() );

    // for old versions compatibility:
    fprintf( m_fp, "MicroViaSize %s\n", fmtBIU( netclass_default->GetuViaDiameter() ).c_str() );
    fprintf( m_fp, "MicroViaDrill %s\n", fmtBIU( netclass_default->GetuViaDrill() ).c_str() );
    fprintf( m_fp, "MicroViasAllowed %s\n", fmtBIU( m_board->GetDesignSettings().m_MicroViasAllowed ).c_str() );
    fprintf( m_fp, "MicroViaMinSize %s\n", fmtBIU( m_board->GetDesignSettings().m_MicroViasMinSize ).c_str() );
    fprintf( m_fp, "MicroViaMinDrill %s\n", fmtBIU( m_board->GetDesignSettings().m_MicroViasMinDrill ).c_str() );

    fprintf( m_fp, "TextPcbWidth %s\n", fmtBIU( m_board->GetDesignSettings().m_PcbTextWidth ).c_str() );
    fprintf( m_fp, "TextPcbSize %s\n",  fmtBIUSize( m_board->GetDesignSettings().m_PcbTextSize ).c_str() );

    /* @todo no globals
    fprintf( m_fp, "EdgeModWidth %d\n", g_ModuleSegmentWidth );
    fprintf( m_fp, "TextModSize %d %d\n", g_ModuleTextSize.x, g_ModuleTextSize.y );
    fprintf( m_fp, "TextModWidth %d\n", g_ModuleTextWidth );
    fprintf( m_fp, "PadSize %d %d\n", g_Pad_Master.m_Size.x, g_Pad_Master.m_Size.y );
    fprintf( m_fp, "PadDrill %d\n", g_Pad_Master.m_Drill.x );
    */

    fprintf( m_fp, "Pad2MaskClearance %s\n", fmtBIU( m_board->GetDesignSettings().m_SolderMaskMargin ).c_str() );

    if( m_board->GetDesignSettings().m_SolderPasteMargin != 0 )
        fprintf( m_fp, "Pad2PasteClearance %s\n", fmtBIU( m_board->GetDesignSettings().m_SolderPasteMargin ).c_str() );

    if( m_board->GetDesignSettings().m_SolderPasteMarginRatio != 0 )
        fprintf( m_fp, "Pad2PasteClearanceRatio %g\n", m_board->GetDesignSettings().m_SolderPasteMarginRatio );

    /* @todo no aFrame
    if ( aFrame->GetScreen()->m_GridOrigin != wxPoint( 0, 0 ) )
    {
        fprintf( m_fp, "GridOrigin %s\n", fmtBIUPoint( aFrame->GetScreen()->m_GridOrigin ).c_str() );
    }
    */

    /* @todo no aFrame in a plugin
    fprintf( m_fp, "AuxiliaryAxisOrg %s\n",   fmtBIUPoint( aFrame->GetOriginAxisPosition() ).c_str() );
    */

    /* @todo no globals
    {
        STRING_FORMATTER sf;

        g_PcbPlotOptions.Format( &sf, 0 );

        wxString record = FROM_UTF8( sf.GetString().c_str() );

        record.Replace( wxT("\n"), wxT(""), true );
        record.Replace( wxT("  "), wxT(" "), true);

        fprintf( m_fp, "PcbPlotParams %s\n", TO_UTF8( record ) );
    }
    */

    fprintf( m_fp, "$EndSETUP\n\n" );
}


void KICAD_PLUGIN::saveBOARD() const
{
    // save the nets
    int netcount = m_board->GetNetCount();
    for( int i = 0; i < netcount;  ++i )
        saveNETINFO_ITEM( m_board->FindNet( i ) );

    // Saved nets do not include netclass names, so save netclasses after nets.
    saveNETCLASSES();

    // save the modules
    for( MODULE* m = m_board->m_Modules;  m;  m = (MODULE*) m->Next() )
        saveMODULE( m );

    // save the graphics owned by the board (not owned by a module)
    for( BOARD_ITEM* gr = m_board->m_Drawings;  gr;  gr = gr->Next() )
    {
        switch( gr->Type() )
        {
        case PCB_TEXT_T:
            savePCB_TEXT( (TEXTE_PCB*) gr );
            break;
        case PCB_LINE_T:
            savePCB_LINE( (DRAWSEGMENT*) gr );
            break;
        case PCB_TARGET_T:
            savePCB_TARGET( (PCB_TARGET*) gr );
            break;
        case PCB_DIMENSION_T:
            saveDIMENTION( (DIMENSION*) gr );
            break;
        default:
            THROW_IO_ERROR( wxString::Format( UNKNOWN_GRAPHIC_FORMAT, gr->Type() ) );
        }
    }

    // do not save MARKER_PCBs, they can be regenerated easily

    // save the tracks & vias
    fprintf( m_fp, "$TRACK\n" );
    for( TRACK* track = m_board->m_Track;  track; track = track->Next() )
        saveTRACK( track );
    fprintf( m_fp, "$EndTRACK\n" );

    // save the old obsolete zones which were done by segments (tracks)
    fprintf( m_fp, "$ZONE\n" );
    for( SEGZONE* zone = m_board->m_Zone;  zone;  zone = zone->Next() )
        saveTRACK( zone );
    fprintf( m_fp, "$EndZONE\n" );

    // save the polygon (which are the newer technology) zones
    for( int i=0;  i < m_board->GetAreaCount();  ++i )
        saveZONE_CONTAINER( m_board->GetArea( i ) );

    fprintf( m_fp, "$EndBOARD\n" );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::saveNETINFO_ITEM( const NETINFO_ITEM* aNet ) const
{
    fprintf( m_fp, "$EQUIPOT\n" );
    fprintf( m_fp, "Na %d %s\n", aNet->GetNet(), EscapedUTF8( aNet->GetNetname() ).c_str() );
    fprintf( m_fp, "St %s\n", "~" );
    fprintf( m_fp, "$EndEQUIPOT\n" );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::saveNETCLASSES() const
{
    const NETCLASSES& nc = m_board->m_NetClasses;

    // save the default first.
    saveNETCLASS( nc.GetDefault() );

    // the rest will be alphabetical in the *.brd file.
    for( NETCLASSES::const_iterator it = nc.begin();  it != nc.end();  ++it )
    {
        NETCLASS*   netclass = it->second;
        saveNETCLASS( netclass );
    }

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::saveNETCLASS( const NETCLASS* nc ) const
{
    fprintf( m_fp, "$NCLASS\n" );
    fprintf( m_fp, "Name %s\n", EscapedUTF8( nc->GetName() ).c_str() );
    fprintf( m_fp, "Desc %s\n", EscapedUTF8( nc->GetDescription() ).c_str() );

    fprintf( m_fp, "Clearance %d\n",    nc->GetClearance() );
    fprintf( m_fp, "TrackWidth %d\n",   nc->GetTrackWidth() );

    fprintf( m_fp, "ViaDia %d\n",       nc->GetViaDiameter() );
    fprintf( m_fp, "ViaDrill %d\n",     nc->GetViaDrill() );

    fprintf( m_fp, "uViaDia %d\n",      nc->GetuViaDiameter() );
    fprintf( m_fp, "uViaDrill %d\n",    nc->GetuViaDrill() );

    for( NETCLASS::const_iterator it = nc->begin();  it!=nc->end();  ++it )
        fprintf( m_fp, "AddNet %s\n", EscapedUTF8( *it ).c_str() );

    fprintf( m_fp, "$EndNCLASS\n" );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::saveMODULE_TEXT( const TEXTE_MODULE* me ) const
{
    MODULE* parent = (MODULE*) me->GetParent();
    double  orient = me->GetOrientation();

    // Due to the Pcbnew history, m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    if( parent )
        orient += parent->GetOrientation();

    wxString txt = me->GetText();

    fprintf( m_fp,  "T%d %s %s %s %s %c %c %d %c %s\n",
                    me->GetType(),
                    fmtBIUPoint( me->GetPos0() ).c_str(),   // m_Pos0.x, m_Pos0.y,
#if 0
                    fmtBIUSize( me->GetSize() ).c_str(),                    // m_Size.y, m_Size.x,
#else
                    fmtBIUPair( me->GetSize().y, me->GetSize().x ).c_str(),     // m_Size.y, m_Size.x,
#endif
                    fmtDEG( orient ).c_str(),
                    fmtBIU( me->GetThickness() ).c_str(),   // m_Thickness,
                    me->IsMirrored() ? 'M' : 'N',
                    me->IsVisible() ? 'V' : 'I',
                    me->GetLayer(),
                    me->IsItalic() ? 'I' : 'N',
                    EscapedUTF8( txt ).c_str()
                    );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::saveMODULE_EDGE( const EDGE_MODULE* me ) const
{
    switch( me->GetShape() )
    {
    case S_SEGMENT:
        fprintf( m_fp,  "DS %s %s %s %d\n",
                        fmtBIUPoint( me->m_Start0 ).c_str(),
                        fmtBIUPoint( me->m_End0 ).c_str(),
                        fmtBIU( me->GetWidth() ).c_str(),
                        me->GetLayer() );
        break;

    case S_CIRCLE:
        fprintf( m_fp,  "DC %s %s %s %d\n",
                        fmtBIUPoint( me->m_Start0 ).c_str(),
                        fmtBIUPoint( me->m_End0 ).c_str(),
                        fmtBIU( me->GetWidth() ).c_str(),
                        me->GetLayer() );
        break;

    case S_ARC:
        fprintf( m_fp,  "DA %s %s %s %s %d\n",
                        fmtBIUPoint( me->m_Start0 ).c_str(),
                        fmtBIUPoint( me->m_End0 ).c_str(),
                        fmtDEG( me->GetAngle() ).c_str(),
                        fmtBIU( me->GetWidth() ).c_str(),
                        me->GetLayer() );
        break;

    case S_POLYGON:
        {
            const std::vector<wxPoint>& polyPoints = me->GetPolyPoints();

            fprintf(    m_fp, "DP %s %s %d %s %d\n",
                        fmtBIUPoint( me->m_Start0 ).c_str(),
                        fmtBIUPoint( me->m_End0 ).c_str(),
                        (int) polyPoints.size(),
                        fmtBIU( me->GetWidth() ).c_str(),
                        me->GetLayer() );

            for( unsigned i = 0;  i<polyPoints.size();  ++i )
                fprintf( m_fp, "Dl %s\n", fmtBIUPoint( polyPoints[i] ).c_str() );
        }
        break;

    default:
        THROW_IO_ERROR( wxString::Format( UNKNOWN_GRAPHIC_FORMAT, me->GetShape() ) );
    }

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::savePAD( const D_PAD* me ) const
{
    fprintf( m_fp, "$PAD\n" );

    int cshape;

    switch( me->GetShape() )
    {
    case PAD_CIRCLE:    cshape = 'C';   break;
    case PAD_RECT:      cshape = 'R';   break;
    case PAD_OVAL:      cshape = 'O';   break;
    case PAD_TRAPEZOID: cshape = 'T';   break;

    default:
        THROW_IO_ERROR( wxString::Format( UNKNOWN_PAD_FORMAT, me->GetShape() ) );
    }

    // universal character set padname
    wxString padname = me->GetPadName();

#if BOARD_FORMAT_VERSION == 1

    char mypadname[PADNAMEZ+1];

    int i;
    for( i = 0; i<PADNAMEZ && padname[i]; ++i )
    {
        // truncate from universal character down to 8 bit foreign jibber jabber byte
        mypadname[i] = (char) padname[i];
    }

    mypadname[i] = 0;

    fprintf( m_fp,  "Sh \"%s\" %c %s %s %s\n",
                    mypadname,         // probably ASCII, but possibly jibber jabber
#else
    fprintf( m_fp,  "Sh %s %c %s %s %s\n",
                    EscapedUTF8( me->GetPadName() ).c_str(),
#endif
                    cshape,
                    fmtBIUSize( me->GetSize() ).c_str(),
                    fmtBIUSize( me->GetDelta() ).c_str(),
                    fmtDEG( me->GetOrientation() ).c_str() );

    fprintf( m_fp,  "Dr %s %s",
                    fmtBIU( me->GetDrillSize().x ).c_str(),
                    fmtBIUSize( me->GetOffset() ).c_str() );

    if( me->GetDrillShape() == PAD_OVAL )
    {
        fprintf( m_fp, " %c %s", 'O', fmtBIUSize( me->GetDrillSize() ).c_str() );
    }

    fprintf( m_fp, "\n" );

    const char* texttype;

    switch( me->GetAttribute() )
    {
    case PAD_STANDARD:          texttype = "STD";       break;
    case PAD_SMD:               texttype = "SMD";       break;
    case PAD_CONN:              texttype = "CONN";      break;
    case PAD_HOLE_NOT_PLATED:   texttype = "HOLE";      break;

    default:
        THROW_IO_ERROR( wxString::Format( UNKNOWN_PAD_ATTRIBUTE, me->GetAttribute() ) );
    }

    fprintf( m_fp, "At %s N %08X\n", texttype, me->GetLayerMask() );

    fprintf( m_fp, "Ne %d %s\n", me->GetNet(), EscapedUTF8( me->GetNetname() ).c_str() );

    fprintf( m_fp, "Po %s\n", fmtBIUPoint( me->GetPos0() ).c_str() );

    if( me->GetDieLength() != 0 )
        fprintf( m_fp, "Le %s\n", fmtBIU( me->GetDieLength() ).c_str() );

    if( me->GetLocalSolderMaskMargin() != 0 )
        fprintf( m_fp, ".SolderMask %s\n", fmtBIU( me->GetLocalSolderMaskMargin() ).c_str() );

    if( me->GetLocalSolderPasteMargin() != 0 )
        fprintf( m_fp, ".SolderPaste %s\n", fmtBIU( me->GetLocalSolderPasteMargin() ).c_str() );

    if( me->GetLocalSolderPasteMarginRatio() != 0 )
        fprintf( m_fp, ".SolderPasteRatio %g\n", me->GetLocalSolderPasteMarginRatio() );

    if( me->GetLocalClearance() != 0 )
        fprintf( m_fp, ".LocalClearance %s\n", fmtBIU( me->GetLocalClearance( ) ).c_str() );

    fprintf( m_fp, "$EndPAD\n" );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::saveMODULE( const MODULE* me ) const
{
    char        statusTxt[3];
    double      orient = me->GetOrientation();

    fprintf( m_fp, "$MODULE %s\n", TO_UTF8( me->GetLibRef() ) );

    statusTxt[0] = me->IsLocked() ? 'F' : '~';
    statusTxt[1] = me->IsPlaced() ? 'P' : '~';
    statusTxt[2] = '\0';

    fprintf( m_fp,  "Po %s %s %d %08lX %08lX %s\n",
                    fmtBIUPoint( me->GetPosition() ).c_str(),    // m_Pos.x, m_Pos.y,
                    fmtDEG( orient ).c_str(),
                    me->GetLayer(),
                    me->GetLastEditTime(),
                    me->GetTimeStamp(),
                    statusTxt );

    fprintf( m_fp, "Li %s\n", TO_UTF8( me->GetLibRef() ) );

    if( !me->GetDescription().IsEmpty() )
    {
        fprintf( m_fp, "Cd %s\n", TO_UTF8( me->GetDescription() ) );
    }

    if( !me->GetKeywords().IsEmpty() )
    {
        fprintf( m_fp, "Kw %s\n", TO_UTF8( me->GetKeywords() ) );
    }

    fprintf( m_fp, "Sc %lX\n", me->GetTimeStamp() );
    fprintf( m_fp, "AR %s\n", TO_UTF8( me->GetPath() ) );
    fprintf( m_fp, "Op %X %X 0\n", me->m_CntRot90, me->m_CntRot180 );

    if( me->GetLocalSolderMaskMargin() != 0 )
        fprintf( m_fp, ".SolderMask %s\n", fmtBIU( me->GetLocalSolderMaskMargin() ).c_str() );

    if( me->GetLocalSolderPasteMargin() != 0 )
        fprintf( m_fp, ".SolderPaste %s\n", fmtBIU( me->GetLocalSolderPasteMargin() ).c_str() );

    if( me->GetLocalSolderPasteMarginRatio() != 0 )
        fprintf( m_fp, ".SolderPasteRatio %g\n", me->GetLocalSolderPasteMarginRatio() );

    if( me->GetLocalClearance() != 0 )
        fprintf( m_fp, ".LocalClearance %s\n", fmtBIU( me->GetLocalClearance( ) ).c_str() );

    // attributes
    if( me->GetAttributes() != MOD_DEFAULT )
    {
        fprintf( m_fp, "At" );

        if( me->GetAttributes() & MOD_CMS )
            fprintf( m_fp, " SMD" );

        if( me->GetAttributes() & MOD_VIRTUAL )
            fprintf( m_fp, " VIRTUAL" );

        fprintf( m_fp, "\n" );
    }

    saveMODULE_TEXT( me->m_Reference );

    saveMODULE_TEXT( me->m_Value );

    // save drawing elements
    for( BOARD_ITEM* gr = me->m_Drawings;  gr;  gr = gr->Next() )
    {
        switch( gr->Type() )
        {
        case PCB_MODULE_TEXT_T:
            saveMODULE_TEXT( (TEXTE_MODULE*) gr );
            break;
        case PCB_MODULE_EDGE_T:
            saveMODULE_EDGE( (EDGE_MODULE*) gr );
            break;
        default:
            THROW_IO_ERROR( wxString::Format( UNKNOWN_GRAPHIC_FORMAT, gr->Type() ) );
        }
    }

    for( D_PAD* pad = me->m_Pads;  pad;  pad = pad->Next() )
        savePAD( pad );

    save3D( me );

    fprintf( m_fp, "$EndMODULE %s\n", TO_UTF8( me->GetLibRef() ) );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::save3D( const MODULE* me ) const
{
    for( S3D_MASTER* t3D = me->m_3D_Drawings;  t3D;  t3D = t3D->Next() )
    {
        if( !t3D->m_Shape3DName.IsEmpty() )
        {
            fprintf( m_fp, "$SHAPE3D\n" );

            fprintf( m_fp, "Na %s\n", EscapedUTF8( t3D->m_Shape3DName ).c_str() );

            fprintf(m_fp,
#if defined(DEBUG)
                    // use old formats for testing, just to verify compatibility
                    // using "diff", then switch to more concise form for release builds.
                    "Sc %lf %lf %lf\n",
#else
                    "Sc %.16g %.16g %.16g\n",
#endif
                    t3D->m_MatScale.x,
                    t3D->m_MatScale.y,
                    t3D->m_MatScale.z );

            fprintf(m_fp,
#if defined(DEBUG)
                    "Of %lf %lf %lf\n",
#else
                    "Of %.16g %.16g %.16g\n",
#endif
                    t3D->m_MatPosition.x,
                    t3D->m_MatPosition.y,
                    t3D->m_MatPosition.z );

            fprintf(m_fp,
#if defined(DEBUG)
                    "Ro %lf %lf %lf\n",
#else
                    "Ro %.16g %.16g %.16g\n",
#endif
                    t3D->m_MatRotation.x,
                    t3D->m_MatRotation.y,
                    t3D->m_MatRotation.z );

            fprintf( m_fp, "$EndSHAPE3D\n" );
        }
    }
}


void KICAD_PLUGIN::savePCB_TARGET( const PCB_TARGET* me ) const
{
    fprintf( m_fp, "$PCB_TARGET\n" );

    fprintf( m_fp, "Po %X %d %s %s %s %lX\n",
             me->GetShape(),
             me->GetLayer(),
             fmtBIUPoint( me->GetPosition() ).c_str(),
             fmtBIU( me->GetSize() ).c_str(),
             fmtBIU( me->GetWidth() ).c_str(),
             me->GetTimeStamp()
             );

    fprintf( m_fp, "$EndPCB_TARGET\n" );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::savePCB_LINE( const DRAWSEGMENT* me ) const
{
    fprintf( m_fp, "$DRAWSEGMENT\n" );

    fprintf( m_fp, "Po %d %s %s %s\n",
             me->GetShape(),
             fmtBIUPoint( me->GetStart() ).c_str(),
             fmtBIUPoint( me->GetEnd() ).c_str(),
             fmtBIU( me->GetWidth() ).c_str()
             );

    if( me->GetType() != S_CURVE )
    {
        fprintf( m_fp, "De %d %d %s %lX %X\n",
                 me->GetLayer(),
                 me->GetType(),
                 fmtDEG( me->GetAngle() ).c_str(),
                 me->GetTimeStamp(),
                 me->GetStatus()
                 );
    }
    else
    {
        fprintf( m_fp, "De %d %d %s %lX %X %s %s\n",
                 me->GetLayer(),
                 me->GetType(),
                 fmtDEG( me->GetAngle() ).c_str(),
                 me->GetTimeStamp(),
                 me->GetStatus(),
                 fmtBIUPoint( me->GetBezControl1() ).c_str(),
                 fmtBIUPoint( me->GetBezControl2() ).c_str()
                 );
    }

    fprintf( m_fp, "$EndDRAWSEGMENT\n" );
}


void KICAD_PLUGIN::saveTRACK( const TRACK* me ) const
{
    int type = 0;

    if( me->Type() == PCB_VIA_T )
        type = 1;

    fprintf(m_fp, "Po %d %s %s %s %s\n",
            me->GetShape(),
            fmtBIUPoint( me->GetStart() ).c_str(),
            fmtBIUPoint( me->GetEnd() ).c_str(),
            fmtBIU( me->GetWidth() ).c_str(),
            fmtBIU( me->GetDrill() ).c_str() );

    fprintf(m_fp, "De %d %d %d %lX %X\n",
            me->GetLayer(), type, me->GetNet(),
            me->GetTimeStamp(), me->GetStatus() );
}


void KICAD_PLUGIN::saveZONE_CONTAINER( const ZONE_CONTAINER* me ) const
{
    fprintf( m_fp, "$CZONE_OUTLINE\n" );

    // Save the outline main info
    fprintf( m_fp,  "ZInfo %lX %d %s\n",
                    me->GetTimeStamp(), me->GetNet(),
                    EscapedUTF8( me->GetNetName() ).c_str() );

    // Save the outline layer info
    fprintf( m_fp, "ZLayer %d\n", me->GetLayer() );

    // Save the outline aux info
    int outline_hatch;

    switch( me->GetHatchStyle() )
    {
    default:
    case CPolyLine::NO_HATCH:       outline_hatch = 'N';    break;
    case CPolyLine::DIAGONAL_EDGE:  outline_hatch = 'E';    break;
    case CPolyLine::DIAGONAL_FULL:  outline_hatch = 'F';    break;
    }

    fprintf( m_fp, "ZAux %d %c\n", me->GetNumCorners(), outline_hatch );

    // Save pad option and clearance
    char padoption;

    switch( me->GetPadOption() )
    {
    default:
    case PAD_IN_ZONE:       padoption = 'I';  break;
    case THERMAL_PAD:       padoption = 'T';  break;
    case PAD_NOT_IN_ZONE:   padoption = 'X';  break;
    }

    fprintf( m_fp,  "ZClearance %s %c\n",
                    fmtBIU( me->GetZoneClearance() ).c_str(),
                    padoption );

    fprintf( m_fp, "ZMinThickness %s\n", fmtBIU( me->GetMinThickness() ).c_str() );

    fprintf( m_fp,  "ZOptions %d %d %c %s %s\n",
                    me->GetFillMode(),
                    me->GetArcSegCount(),
                    me->IsFilled() ? 'S' : 'F',
                    fmtBIU( me->GetThermalReliefGap() ).c_str(),
                    fmtBIU( me->GetThermalReliefCopperBridge() ).c_str() );

    fprintf( m_fp,  "ZSmoothing %d %s\n",
                    me->GetCornerSmoothingType(),
                    fmtBIU( me->GetCornerRadius() ).c_str() );

    typedef std::vector< CPolyPt >    CPOLY_PTS;

    // Save the corner list
    const CPOLY_PTS& cv = me->m_Poly->corner;
    for( CPOLY_PTS::const_iterator it = cv.begin();  it != cv.end();  ++it )
    {
        fprintf( m_fp,  "ZCorner %s %d\n",
                        fmtBIUPair( it->x, it->y ).c_str(),
                        it->end_contour );
    }

    // Save the PolysList
    const CPOLY_PTS& fv = me->m_FilledPolysList;
    if( fv.size() )
    {
        fprintf( m_fp, "$POLYSCORNERS\n" );

        for( CPOLY_PTS::const_iterator it = fv.begin();  it != fv.end();  ++it )
        {
            fprintf( m_fp, "%s %d %d\n",
                           fmtBIUPair( it->x, it->y ).c_str(),
                           it->end_contour,
                           it->utility );
        }

        fprintf( m_fp, "$endPOLYSCORNERS\n" );
    }

    typedef std::vector< SEGMENT > SEGMENTS;

    // Save the filling segments list
    const SEGMENTS& segs = me->m_FillSegmList;
    if( segs.size() )
    {
        fprintf( m_fp, "$FILLSEGMENTS\n" );

        for( SEGMENTS::const_iterator it = segs.begin();  it != segs.end();  ++it )
        {
            fprintf( m_fp, "%s %s\n",
                           fmtBIUPoint( it->m_Start ).c_str(),
                           fmtBIUPoint( it->m_End ).c_str() );
        }

        fprintf( m_fp, "$endFILLSEGMENTS\n" );
    }

    fprintf( m_fp, "$endCZONE_OUTLINE\n" );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::saveDIMENTION( const DIMENSION* me ) const
{
    // note: COTATION was the previous name of DIMENSION
    // this old keyword is used here for compatibility
    fprintf( m_fp, "$COTATION\n" );

    fprintf( m_fp, "Ge %d %d %lX\n", me->GetShape(), me->GetLayer(), me->GetTimeStamp() );

    fprintf( m_fp, "Va %d\n", me->m_Value );

    if( !me->m_Text.GetText().IsEmpty() )
        fprintf( m_fp, "Te %s\n", EscapedUTF8( me->m_Text.GetText() ).c_str() );
    else
        fprintf( m_fp, "Te \"?\"\n" );

    fprintf( m_fp,  "Po %s %s %s %s %d\n",
                    fmtBIUPoint( me->m_Text.GetPosition() ).c_str(),
                    fmtBIUSize( me->m_Text.GetSize() ).c_str(),
                    fmtBIU( me->m_Text.GetThickness() ).c_str(),
                    fmtDEG( me->m_Text.GetOrientation() ).c_str(),
                    me->m_Text.IsMirrored() ? 0 : 1     // strange but true
                    );

    fprintf( m_fp,  "Sb %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_crossBarOx, me->m_crossBarOy ).c_str(),
                    fmtBIUPair( me->m_crossBarFx, me->m_crossBarFy ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "Sd %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_featureLineDOx, me->m_featureLineDOy ).c_str(),
                    fmtBIUPair( me->m_featureLineDFx, me->m_featureLineDFy ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "Sg %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_featureLineGOx, me->m_featureLineGOy ).c_str(),
                    fmtBIUPair( me->m_featureLineGFx, me->m_featureLineGFy ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "S1 %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_arrowD1Ox, me->m_arrowD1Oy ).c_str(),
                    fmtBIUPair( me->m_arrowD1Fx, me->m_arrowD1Fy ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "S2 %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_arrowD2Ox, me->m_arrowD2Oy ).c_str(),
                    fmtBIUPair( me->m_arrowD2Fx, me->m_arrowD2Fy ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "S3 %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_arrowG1Ox, me->m_arrowG1Oy ).c_str(),
                    fmtBIUPair( me->m_arrowG1Fx, me->m_arrowG1Fy ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "S4 %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_arrowG2Ox, me->m_arrowG2Oy ).c_str(),
                    fmtBIUPair( me->m_arrowG2Fx, me->m_arrowG2Fy ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp, "$endCOTATION\n" );

    CHECK_WRITE_ERROR();
}


void KICAD_PLUGIN::savePCB_TEXT( const TEXTE_PCB* me ) const
{
    if( me->GetText().IsEmpty() )
        return;

    fprintf( m_fp, "$TEXTPCB\n" );

    wxArrayString* list = wxStringSplit( me->GetText(), '\n' );

    for( unsigned ii = 0; ii < list->Count(); ii++ )
    {
        wxString txt  = list->Item( ii );

        if ( ii == 0 )
            fprintf( m_fp, "Te %s\n", EscapedUTF8( txt ).c_str() );
        else
            fprintf( m_fp, "nl %s\n", EscapedUTF8( txt ).c_str() );
    }

    delete list;

    fprintf( m_fp,  "Po %s %s %s %s\n",
                    fmtBIUPoint( me->GetPosition() ).c_str(),
                    fmtBIUSize( me->GetSize() ).c_str(),
                    fmtBIU( me->GetThickness() ).c_str(),
                    fmtDEG( me->GetOrientation() ).c_str() );

    char hJustify;

    switch( me->GetHorizJustify() )
    {
    default:
    case GR_TEXT_HJUSTIFY_CENTER:   hJustify = 'C';     break;
    case GR_TEXT_HJUSTIFY_LEFT:     hJustify = 'L';     break;
    case GR_TEXT_HJUSTIFY_RIGHT:    hJustify = 'R';     break;
    }

    fprintf( m_fp,  "De %d %d %lX %s %c\n",
                    me->GetLayer(),
                    !me->IsMirrored(),
                    me->GetTimeStamp(),
                    me->IsItalic() ? "Italic" : "Normal",
                    hJustify );

    fprintf( m_fp, "$EndTEXTPCB\n" );
}
