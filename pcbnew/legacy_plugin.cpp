
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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

    The definitions:

    *) a Board Internal Unit (BIU) is a unit of length that is used only internally
       to PCBNEW, and is nanometers when this work is done, but deci-mils until done.

    The philosophies:

    *) BIUs should be typed as such to distinguish them from ints.  This is mostly
       for human readability, and having the type nearby in the source supports this readability.
    *) Do not assume that BIUs will always be int, doing a sscanf() into a BIU
       does not make sense in case the size of the BIU changes.
    *) variables are put onto the stack in an automatic, even when it might look
       more efficient to do otherwise.  This is so we can seem them with a debugger.
    *) Global variables should not be touched from within a PLUGIN, since it will eventually
       be in a DLL/DSO.  This includes window information too.  The PLUGIN API knows
       nothing of wxFrame or globals and all error reporting must be done by throwing
       an exception.
    *) No wxWindowing calls are made in here, since the UI resides higher up than in here,
       and is going to process a bucket of detailed information thrown from down here
       in the form of an exception if an error happens.
    *) Much of what we do in this source file is for human readability, not performance.
       Simply avoiding strtok() more often than the old code washes out performance losses.
       Remember strncmp() will bail as soon as a mismatch happens, not going all the way
       to end of string unless a full match.
    *) angles are in the process of migrating to doubles, and 'int' if used, is
       only shortterm, and along with this a change, and transition from from
       "tenths of degrees" to simply "degrees" in the double (which has no problem
       representing any portion of a degree).
*/


#include <cmath>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wx/ffile.h>

#include <legacy_plugin.h>   // implement this here

#include <kicad_string.h>
#include <macros.h>
#include <zones.h>

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
#include <pcb_plot_params_parser.h>
#include <drawtxt.h>
#include <convert_to_biu.h>
#include <trigo.h>
#include <build_version.h>

#include <boost/make_shared.hpp>


typedef LEGACY_PLUGIN::BIU      BIU;


#define VERSION_ERROR_FORMAT    _( "File '%s' is format version: %d.\nI only support format version <= %d.\nPlease upgrade Pcbnew to load this file." )
#define UNKNOWN_GRAPHIC_FORMAT  _( "unknown graphic type: %d")
#define UNKNOWN_PAD_FORMAT      _( "unknown pad type: %d")
#define UNKNOWN_PAD_ATTRIBUTE   _( "unknown pad attribute: %d" )


typedef unsigned                LEG_MASK;

#define FIRST_LAYER             0
#define FIRST_COPPER_LAYER      0
#define LAYER_N_BACK            0
#define LAYER_N_2               1
#define LAYER_N_3               2
#define LAYER_N_4               3
#define LAYER_N_5               4
#define LAYER_N_6               5
#define LAYER_N_7               6
#define LAYER_N_8               7
#define LAYER_N_9               8
#define LAYER_N_10              9
#define LAYER_N_11              10
#define LAYER_N_12              11
#define LAYER_N_13              12
#define LAYER_N_14              13
#define LAYER_N_15              14
#define LAYER_N_FRONT           15
#define LAST_COPPER_LAYER       LAYER_N_FRONT

#define FIRST_NON_COPPER_LAYER  16
#define ADHESIVE_N_BACK         16
#define ADHESIVE_N_FRONT        17
#define SOLDERPASTE_N_BACK      18
#define SOLDERPASTE_N_FRONT     19
#define SILKSCREEN_N_BACK       20
#define SILKSCREEN_N_FRONT      21
#define SOLDERMASK_N_BACK       22
#define SOLDERMASK_N_FRONT      23
#define DRAW_N                  24
#define COMMENT_N               25
#define ECO1_N                  26
#define ECO2_N                  27
#define EDGE_N                  28
#define LAST_NON_COPPER_LAYER   28

// Masks to identify a layer by a bit map
typedef unsigned LAYER_MSK;
#define LAYER_BACK              (1 << LAYER_N_BACK)     ///< bit mask for copper layer
#define LAYER_2                 (1 << LAYER_N_2)        ///< bit mask for layer 2
#define LAYER_3                 (1 << LAYER_N_3)        ///< bit mask for layer 3
#define LAYER_4                 (1 << LAYER_N_4)        ///< bit mask for layer 4
#define LAYER_5                 (1 << LAYER_N_5)        ///< bit mask for layer 5
#define LAYER_6                 (1 << LAYER_N_6)        ///< bit mask for layer 6
#define LAYER_7                 (1 << LAYER_N_7)        ///< bit mask for layer 7
#define LAYER_8                 (1 << LAYER_N_8)        ///< bit mask for layer 8
#define LAYER_9                 (1 << LAYER_N_9)        ///< bit mask for layer 9
#define LAYER_10                (1 << LAYER_N_10)       ///< bit mask for layer 10
#define LAYER_11                (1 << LAYER_N_11)       ///< bit mask for layer 11
#define LAYER_12                (1 << LAYER_N_12)       ///< bit mask for layer 12
#define LAYER_13                (1 << LAYER_N_13)       ///< bit mask for layer 13
#define LAYER_14                (1 << LAYER_N_14)       ///< bit mask for layer 14
#define LAYER_15                (1 << LAYER_N_15)       ///< bit mask for layer 15
#define LAYER_FRONT             (1 << LAYER_N_FRONT)    ///< bit mask for component layer
#define ADHESIVE_LAYER_BACK     (1 << ADHESIVE_N_BACK)
#define ADHESIVE_LAYER_FRONT    (1 << ADHESIVE_N_FRONT)
#define SOLDERPASTE_LAYER_BACK  (1 << SOLDERPASTE_N_BACK)
#define SOLDERPASTE_LAYER_FRONT (1 << SOLDERPASTE_N_FRONT)
#define SILKSCREEN_LAYER_BACK   (1 << SILKSCREEN_N_BACK)
#define SILKSCREEN_LAYER_FRONT  (1 << SILKSCREEN_N_FRONT)
#define SOLDERMASK_LAYER_BACK   (1 << SOLDERMASK_N_BACK)
#define SOLDERMASK_LAYER_FRONT  (1 << SOLDERMASK_N_FRONT)
#define DRAW_LAYER              (1 << DRAW_N)
#define COMMENT_LAYER           (1 << COMMENT_N)
#define ECO1_LAYER              (1 << ECO1_N)
#define ECO2_LAYER              (1 << ECO2_N)
#define EDGE_LAYER              (1 << EDGE_N)

// Helpful global layer masks:
// ALL_AUX_LAYERS layers are technical layers, ALL_NO_CU_LAYERS has user
// and edge layers too!
#define ALL_NO_CU_LAYERS        0x1FFF0000
#define ALL_CU_LAYERS           0x0000FFFF
#define FRONT_TECH_LAYERS       (SILKSCREEN_LAYER_FRONT | SOLDERMASK_LAYER_FRONT \
                                    | ADHESIVE_LAYER_FRONT | SOLDERPASTE_LAYER_FRONT)
#define BACK_TECH_LAYERS        (SILKSCREEN_LAYER_BACK | SOLDERMASK_LAYER_BACK \
                                    | ADHESIVE_LAYER_BACK | SOLDERPASTE_LAYER_BACK)
#define ALL_TECH_LAYERS         (FRONT_TECH_LAYERS | BACK_TECH_LAYERS)
#define BACK_LAYERS             (LAYER_BACK | BACK_TECH_LAYERS)
#define FRONT_LAYERS            (LAYER_FRONT | FRONT_TECH_LAYERS)

#define ALL_USER_LAYERS         (DRAW_LAYER | COMMENT_LAYER | ECO1_LAYER | ECO2_LAYER )

#define NO_LAYERS               0x00000000


// Old internal units definition (UI = decimil)
#define PCB_LEGACY_INTERNAL_UNIT 10000

/// Get the length of a string constant, at compile time
#define SZ( x )         (sizeof(x)-1)


static const char delims[] = " \t\r\n";


static bool inline isSpace( int c ) { return strchr( delims, c ) != 0; }

#define MASK(x)             (1<<(x))

//-----<BOARD Load Functions>---------------------------------------------------

/// C string compare test for a specific length of characters.
#define TESTLINE( x )   ( !strnicmp( line, x, SZ( x ) ) && isSpace( line[SZ( x )] ) )

/// C sub-string compare test for a specific length of characters.
#define TESTSUBSTR( x ) ( !strnicmp( line, x, SZ( x ) ) )


#if 1
#define READLINE( rdr )     rdr->ReadLine()

#else
/// The function and macro which follow comprise a shim which can be a
/// monitor on lines of text read in from the input file.
/// And it can be used as a trap.
static inline char* ReadLine( LINE_READER* rdr, const char* caller )
{
    char* ret = rdr->ReadLine();

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
#define READLINE( rdr )     ReadLine( rdr, __FUNCTION__ )
#endif



using namespace std;    // auto_ptr


static EDA_TEXT_HJUSTIFY_T horizJustify( const char* horizontal )
{
    if( !strcmp( "L", horizontal ) )
        return GR_TEXT_HJUSTIFY_LEFT;
    if( !strcmp( "R", horizontal ) )
        return GR_TEXT_HJUSTIFY_RIGHT;
    return GR_TEXT_HJUSTIFY_CENTER;
}

static EDA_TEXT_VJUSTIFY_T vertJustify( const char* vertical )
{
    if( !strcmp( "T", vertical ) )
        return GR_TEXT_VJUSTIFY_TOP;
    if( !strcmp( "B", vertical ) )
        return GR_TEXT_VJUSTIFY_BOTTOM;
    return GR_TEXT_VJUSTIFY_CENTER;
}


/// Count the number of set layers in the mask
inline int layerMaskCountSet( LEG_MASK aMask )
{
    int count = 0;

    for( int i = 0;  aMask;  ++i, aMask >>= 1 )
    {
        if( aMask & 1 )
            ++count;
    }

    return count;
}


// return true if aLegacyLayerNum is a valid copper layer legacy id, therefore
// top, bottom or inner activated layer
inline bool is_leg_copperlayer_valid( int aCu_Count, LAYER_NUM aLegacyLayerNum )
{
    return ( aLegacyLayerNum == LAYER_N_FRONT ) || ( aLegacyLayerNum < aCu_Count );
}


LAYER_ID LEGACY_PLUGIN::leg_layer2new( int cu_count, LAYER_NUM aLayerNum )
{
    int         newid;
    unsigned    old = aLayerNum;

    // this is a speed critical function, be careful.

    if( unsigned( old ) <= unsigned( LAYER_N_FRONT ) )
    {
        if( old == LAYER_N_FRONT )
            newid = F_Cu;
        else if( old == LAYER_N_BACK )
            newid = B_Cu;
        else
        {
            newid = cu_count - 1 - old;
            wxASSERT( newid >= 0 );
        }
    }
    else
    {
        switch( old )
        {
        case ADHESIVE_N_BACK:       newid = B_Adhes;    break;
        case ADHESIVE_N_FRONT:      newid = F_Adhes;    break;
        case SOLDERPASTE_N_BACK:    newid = B_Paste;    break;
        case SOLDERPASTE_N_FRONT:   newid = F_Paste;    break;
        case SILKSCREEN_N_BACK:     newid = B_SilkS;    break;
        case SILKSCREEN_N_FRONT:    newid = F_SilkS;    break;
        case SOLDERMASK_N_BACK:     newid = B_Mask;     break;
        case SOLDERMASK_N_FRONT:    newid = F_Mask;     break;
        case DRAW_N:                newid = Dwgs_User;  break;
        case COMMENT_N:             newid = Cmts_User;  break;
        case ECO1_N:                newid = Eco1_User;  break;
        case ECO2_N:                newid = Eco2_User;  break;
        case EDGE_N:                newid = Edge_Cuts;  break;
        default:
//            wxASSERT( 0 );
            // Remap all illegal non copper layers to comment layer
            newid = Cmts_User;
        }
    }

    return LAYER_ID( newid );
}


LSET LEGACY_PLUGIN::leg_mask2new( int cu_count, unsigned aMask )
{
    LSET    ret;

    if( ( aMask & ALL_CU_LAYERS ) == ALL_CU_LAYERS )
    {
        ret = LSET::AllCuMask();

        aMask &= ~ALL_CU_LAYERS;
    }

    for( int i=0;  aMask;  ++i, aMask >>= 1 )
    {
        if( aMask & 1 )
            ret.set( leg_layer2new( cu_count, i ) );
    }

    return ret;
}


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
 * Function layerParse
 * Like intParse but returns a LAYER_NUM
 */
static inline LAYER_NUM layerParse( const char* next, const char** out = NULL )
{
    return intParse( next, out );
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


BOARD* LEGACY_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // delete on exception, iff I own m_board, according to aAppendToMe
    auto_ptr<BOARD> deleter( aAppendToMe ? NULL : m_board );

    FILE_LINE_READER    reader( aFileName );

    m_reader = &reader;          // member function accessibility

    checkVersion();

    loadAllSections( bool( aAppendToMe ) );

    deleter.release();
    return m_board;
}


void LEGACY_PLUGIN::loadAllSections( bool doAppend )
{
    // $GENERAL section is first

    // $SHEETDESCR section is next

    // $SETUP section is next

    // Then follows $EQUIPOT and all the rest
    char* line;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        // put the more frequent ones at the top, but realize TRACKs are loaded as a group

        if( TESTLINE( "$MODULE" ) )
        {
            auto_ptr<MODULE>    module( new MODULE( m_board ) );

            FPID        fpid;
            std::string fpName = StrPurge( line + SZ( "$MODULE" ) );

            // The footprint names in legacy libraries can contain the '/' and ':'
            // characters which will cause the FPID parser to choke.
            ReplaceIllegalFileNameChars( &fpName );

            if( !fpName.empty() )
                fpid = FPID( fpName );

            module->SetFPID( fpid );

            loadMODULE( module.get() );
            m_board->Add( module.release(), ADD_APPEND );
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
            loadTrackList( PCB_TRACE_T );
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

        else if( TESTLINE( "$PCB_TARGET" ) || TESTLINE( "$MIREPCB" ) )
        {
            loadPCB_TARGET();
        }

        else if( TESTLINE( "$ZONE" ) )
        {
            loadTrackList( PCB_ZONE_T );
        }

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
                while( ( line = READLINE( m_reader ) ) != NULL )
                {
                    // gobble until $EndSetup
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


void LEGACY_PLUGIN::checkVersion()
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

#if !defined(DEBUG)
    if( ver > LEGACY_BOARD_FILE_VERSION )
    {
        // "File '%s' is format version: %d.\nI only support format version <= %d.\nPlease upgrade Pcbnew to load this file."
        m_error.Printf( VERSION_ERROR_FORMAT,
            m_reader->GetSource().GetData(), ver, LEGACY_BOARD_FILE_VERSION );
        THROW_IO_ERROR( m_error );
    }
#endif

    m_loading_format_version = ver;
    m_board->SetFileFormatVersionAtLoad( m_loading_format_version );
}


void LEGACY_PLUGIN::loadGENERAL()
{
    char*   line;
    char*   saveptr;
    bool    saw_LayerCount = false;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

        if( TESTLINE( "Units" ) )
        {
            // what are the engineering units of the lengths in the BOARD?
            data = strtok_r( line + SZ("Units"), delims, &saveptr );

            if( !strcmp( data, "mm" ) )
            {
                diskToBiu = IU_PER_MM;
            }
        }

        else if( TESTLINE( "LayerCount" ) )
        {
            int tmp = intParse( line + SZ( "LayerCount" ) );
            m_board->SetCopperLayerCount( tmp );

            // This has to be set early so that leg_layer2new() works OK, and
            // that means before parsing "EnabledLayers" and "VisibleLayers".
            m_cu_count = tmp;

            saw_LayerCount = true;
        }

        else if( TESTLINE( "EnabledLayers" ) )
        {
            if( !saw_LayerCount )
                THROW_IO_ERROR( "Missing '$GENERAL's LayerCount" );

            LEG_MASK enabledLayers = hexParse( line + SZ( "EnabledLayers" ) );

            LSET new_mask = leg_mask2new( m_cu_count, enabledLayers );

            //DBG( printf( "EnabledLayers: %s\n", new_mask.FmtHex().c_str() );)

            m_board->SetEnabledLayers( new_mask );

            // layer visibility equals layer usage, unless overridden later via "VisibleLayers"
            // Must call SetEnabledLayers() before calling SetVisibleLayers().
            m_board->SetVisibleLayers( new_mask );
        }

        else if( TESTLINE( "VisibleLayers" ) )
        {
            if( !saw_LayerCount )
                THROW_IO_ERROR( "Missing '$GENERAL's LayerCount" );

            LEG_MASK visibleLayers = hexParse( line + SZ( "VisibleLayers" ) );

            LSET new_mask = leg_mask2new( m_cu_count, visibleLayers );

            m_board->SetVisibleLayers( new_mask );
        }

        else if( TESTLINE( "Ly" ) )    // Old format for Layer count
        {
            if( !saw_LayerCount )
            {
                LEG_MASK layer_mask  = hexParse( line + SZ( "Ly" ) );

                m_cu_count = layerMaskCountSet( layer_mask & ALL_CU_LAYERS );

                m_board->SetCopperLayerCount( m_cu_count );

                saw_LayerCount = true;
            }
        }

        else if( TESTLINE( "BoardThickness" ) )
        {
            BIU thickn = biuParse( line + SZ( "BoardThickness" ) );
            m_board->GetDesignSettings().SetBoardThickness( thickn );
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
            m_board->SetUnconnectedNetCount( tmp );
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

        /* This is no more usefull, so this info is no more parsed
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
        }*/

        else if( TESTLINE( "Nnets" ) )
        {
            m_netCodes.resize( intParse( line + SZ( "Nnets" ) ) );
        }

        else if( TESTLINE( "Nn" ) )     // id "Nnets" for old .brd files
        {
            m_netCodes.resize( intParse( line + SZ( "Nn" ) ) );
        }

        else if( TESTLINE( "$EndGENERAL" ) )
            return;     // preferred exit
    }

    THROW_IO_ERROR( "Missing '$EndGENERAL'" );
}


void LEGACY_PLUGIN::loadSHEET()
{
    char        buf[260];
    TITLE_BLOCK tb;
    char*       line;
    char*       saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        if( TESTLINE( "Sheet" ) )
        {
            // e.g. "Sheet A3 16535 11700"
            // width and height are in 1/1000th of an inch, always

            PAGE_INFO   page;
            char*       sname  = strtok_r( line + SZ( "Sheet" ), delims, &saveptr );

            if( sname )
            {
                wxString wname = FROM_UTF8( sname );
                if( !page.SetType( wname ) )
                {
                    m_error.Printf( _( "Unknown sheet type '%s' on line:%d" ),
                                wname.GetData(), m_reader->LineNumber() );
                    THROW_IO_ERROR( m_error );
                }

                char*   width  = strtok_r( NULL, delims, &saveptr );
                char*   height = strtok_r( NULL, delims, &saveptr );
                char*   orient = strtok_r( NULL, delims, &saveptr );

                // only parse the width and height if page size is custom ("User")
                if( wname == PAGE_INFO::Custom )
                {
                    if( width && height )
                    {
                        // legacy disk file describes paper in mils
                        // (1/1000th of an inch)
                        int w = intParse( width );
                        int h = intParse( height );

                        page.SetWidthMils(  w );
                        page.SetHeightMils( h );
                    }
                }

                if( orient && !strcmp( orient, "portrait" ) )
                {
                    page.SetPortrait( true );
                }

                m_board->SetPageSettings( page );
            }
        }

        else if( TESTLINE( "Title" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetTitle( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Date" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetDate( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Rev" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetRevision( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Comp" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetCompany( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Comment1" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment1( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Comment2" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment2( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Comment3" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment3( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "Comment4" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment4( FROM_UTF8( buf ) );
        }

        else if( TESTLINE( "$EndSHEETDESCR" ) )
        {
            m_board->SetTitleBlock( tb );
            return;             // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$EndSHEETDESCR'" );
}


void LEGACY_PLUGIN::loadSETUP()
{
    NETCLASSPTR             netclass_default = m_board->GetDesignSettings().GetDefault();
    // TODO Orson: is it really necessary to first operate on a copy and then apply it?
    // would not it be better to use reference here and apply all the changes instantly?
    BOARD_DESIGN_SETTINGS   bds = m_board->GetDesignSettings();
    ZONE_SETTINGS           zs  = m_board->GetZoneSettings();
    char*                   line;
    char*                   saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

        if( TESTLINE( "PcbPlotParams" ) )
        {
            PCB_PLOT_PARAMS plot_opts;

            PCB_PLOT_PARAMS_PARSER parser( line + SZ( "PcbPlotParams" ), m_reader->GetSource() );

            plot_opts.Parse( &parser );

            m_board->SetPlotOptions( plot_opts );
        }

        else if( TESTLINE( "AuxiliaryAxisOrg" ) )
        {
            BIU gx = biuParse( line + SZ( "AuxiliaryAxisOrg" ), &data );
            BIU gy = biuParse( data );

            // m_board->SetAuxOrigin( wxPoint( gx, gy ) ); gets overwritten by SetDesignSettings() below
            bds.m_AuxOrigin = wxPoint( gx, gy );
        }

        /* Done from $General above's "LayerCount"
        else if( TESTLINE( "Layers" ) )
        {
            int tmp = intParse( line + SZ( "Layers" ) );
            m_board->SetCopperLayerCount( tmp );

            m_cu_count = tmp;
        }
        */

        else if( TESTSUBSTR( "Layer[" ) )
        {
            // eg: "Layer[n]  <a_Layer_name_with_no_spaces> <LAYER_T>"

            LAYER_NUM   layer_num = layerParse( line + SZ( "Layer[" ), &data );
            LAYER_ID    layer_id  = leg_layer2new( m_cu_count, layer_num );

            /*
            switch( layer_num )
            {
            case LAYER_N_BACK:
                layer_id = B_Cu;
                break;

            case LAYER_N_FRONT:
                layer_id = F_Cu;
                break;

            default:
                layer_id = LAYER_ID( layer_num );
            }
            */

            data = strtok_r( (char*) data+1, delims, &saveptr );    // +1 for ']'
            if( data )
            {
                wxString layerName = FROM_UTF8( data );
                m_board->SetLayerName( layer_id, layerName );

                data = strtok_r( NULL, delims, &saveptr );
                if( data )  // optional in old board files
                {
                    LAYER_T type = LAYER::ParseType( data );
                    m_board->SetLayerType( layer_id, type );
                }
            }
        }

        else if( TESTLINE( "TrackWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackWidth" ) );
            netclass_default->SetTrackWidth( tmp );
        }

        else if( TESTLINE( "TrackWidthList" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackWidthList" ) );
            bds.m_TrackWidthList.push_back( tmp );
        }

        else if( TESTLINE( "TrackClearence" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackClearence" ) );
            netclass_default->SetClearance( tmp );
        }

        else if( TESTLINE( "TrackMinWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackMinWidth" ) );
            bds.m_TrackMinWidth = tmp;
        }

        else if( TESTLINE( "ZoneClearence" ) )
        {
            BIU tmp = biuParse( line + SZ( "ZoneClearence" ) );
            zs.m_ZoneClearance = tmp;
        }

        else if( TESTLINE( "Zone_45_Only" ) )
        {
            bool tmp = (bool) intParse( line + SZ( "Zone_45_Only" ) );
            zs.m_Zone_45_Only = tmp;
        }

        else if( TESTLINE( "DrawSegmWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "DrawSegmWidth" ) );
            bds.m_DrawSegmentWidth = tmp;
        }

        else if( TESTLINE( "EdgeSegmWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "EdgeSegmWidth" ) );
            bds.m_EdgeSegmentWidth = tmp;
        }

        else if( TESTLINE( "ViaMinSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaMinSize" ) );
            bds.m_ViasMinSize = tmp;
        }

        else if( TESTLINE( "MicroViaMinSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaMinSize" ) );
            bds.m_MicroViasMinSize = tmp;
        }

        else if( TESTLINE( "ViaSizeList" ) )
        {
            // e.g.  "ViaSizeList DIAMETER [DRILL]"

            BIU drill    = 0;
            BIU diameter = biuParse( line + SZ( "ViaSizeList" ), &data );

            data = strtok_r( (char*) data, delims, &saveptr );
            if( data )  // DRILL may not be present ?
                drill = biuParse( data );

            bds.m_ViasDimensionsList.push_back( VIA_DIMENSION( diameter,
                                                                                        drill ) );
        }

        else if( TESTLINE( "ViaSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaSize" ) );
            netclass_default->SetViaDiameter( tmp );
        }

        else if( TESTLINE( "ViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaDrill" ) );
            netclass_default->SetViaDrill( tmp );
        }

        else if( TESTLINE( "ViaMinDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaMinDrill" ) );
            bds.m_ViasMinDrill = tmp;
        }

        else if( TESTLINE( "MicroViaSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaSize" ) );
            netclass_default->SetuViaDiameter( tmp );
        }

        else if( TESTLINE( "MicroViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaDrill" ) );
            netclass_default->SetuViaDrill( tmp );
        }

        else if( TESTLINE( "MicroViaMinDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaMinDrill" ) );
            bds.m_MicroViasMinDrill = tmp;
        }

        else if( TESTLINE( "MicroViasAllowed" ) )
        {
            int tmp = intParse( line + SZ( "MicroViasAllowed" ) );
            bds.m_MicroViasAllowed = tmp;
        }

        else if( TESTLINE( "TextPcbWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TextPcbWidth" ) );
            bds.m_PcbTextWidth = tmp;
        }

        else if( TESTLINE( "TextPcbSize" ) )
        {
            BIU x = biuParse( line + SZ( "TextPcbSize" ), &data );
            BIU y = biuParse( data );

            bds.m_PcbTextSize = wxSize( x, y );
        }

        else if( TESTLINE( "EdgeModWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "EdgeModWidth" ) );
            bds.m_ModuleSegmentWidth = tmp;
        }

        else if( TESTLINE( "TextModWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TextModWidth" ) );
            bds.m_ModuleTextWidth = tmp;
        }

        else if( TESTLINE( "TextModSize" ) )
        {
            BIU x = biuParse( line + SZ( "TextModSize" ), &data );
            BIU y = biuParse( data );

            bds.m_ModuleTextSize = wxSize( x, y );
        }

        else if( TESTLINE( "PadSize" ) )
        {
            BIU x = biuParse( line + SZ( "PadSize" ), &data );
            BIU y = biuParse( data );

            bds.m_Pad_Master.SetSize( wxSize( x, y ) );
        }

        else if( TESTLINE( "PadDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "PadDrill" ) );
            bds.m_Pad_Master.SetDrillSize( wxSize( tmp, tmp ) );
        }

        else if( TESTLINE( "Pad2MaskClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( "Pad2MaskClearance" ) );
            bds.m_SolderMaskMargin = tmp;
        }

        else if( TESTLINE( "SolderMaskMinWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "SolderMaskMinWidth" ) );
            bds.m_SolderMaskMinWidth = tmp;
        }

        else if( TESTLINE( "Pad2PasteClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( "Pad2PasteClearance" ) );
            bds.m_SolderPasteMargin = tmp;
        }

        else if( TESTLINE( "Pad2PasteClearanceRatio" ) )
        {
            double ratio = atof( line + SZ( "Pad2PasteClearanceRatio" ) );
            bds.m_SolderPasteMarginRatio = ratio;
        }

        else if( TESTLINE( "GridOrigin" ) )
        {
            BIU x = biuParse( line + SZ( "GridOrigin" ), &data );
            BIU y = biuParse( data );

            // m_board->SetGridOrigin( wxPoint( x, y ) ); gets overwritten by SetDesignSettings() below
            bds.m_GridOrigin = wxPoint( x, y );
        }

        else if( TESTLINE( "VisibleElements" ) )
        {
            int visibleElements = hexParse( line + SZ( "VisibleElements" ) );
            bds.SetVisibleElements( visibleElements );
        }

        else if( TESTLINE( "$EndSETUP" ) )
        {
            m_board->SetDesignSettings( bds );
            m_board->SetZoneSettings( zs );

            // Very old *.brd file does not have  NETCLASSes
            // "TrackWidth", "ViaSize", "ViaDrill", "ViaMinSize",
            // and "TrackClearence", were defined in SETUP
            // these values are put into the default NETCLASS until later board load
            // code should override them.  *.brd files which have been
            // saved with knowledge of NETCLASSes will override these
            // defaults, very old boards (before 2009) will not and use the setup values.
            // However these values should be the same as default NETCLASS.

            return;     // preferred exit
        }
    }

    // @todo: this code is currently unreachable, would need a goto, to get here.
    // that may be better handled with an #ifdef

    /* Ensure tracks and vias sizes lists are ok:
     * Sort lists by by increasing value and remove duplicates
     * (the first value is not tested, because it is the netclass value
     */
    BOARD_DESIGN_SETTINGS& designSettings = m_board->GetDesignSettings();
    sort( designSettings.m_ViasDimensionsList.begin() + 1, designSettings.m_ViasDimensionsList.end() );
    sort( designSettings.m_TrackWidthList.begin() + 1, designSettings.m_TrackWidthList.end() );

    for( unsigned ii = 1; ii < designSettings.m_ViasDimensionsList.size() - 1; ii++ )
    {
        if( designSettings.m_ViasDimensionsList[ii] == designSettings.m_ViasDimensionsList[ii + 1] )
        {
            designSettings.m_ViasDimensionsList.erase( designSettings.m_ViasDimensionsList.begin() + ii );
            ii--;
        }
    }

    for( unsigned ii = 1; ii < designSettings.m_TrackWidthList.size() - 1; ii++ )
    {
        if( designSettings.m_TrackWidthList[ii] == designSettings.m_TrackWidthList[ii + 1] )
        {
            designSettings.m_TrackWidthList.erase( designSettings.m_TrackWidthList.begin() + ii );
            ii--;
        }
    }
}


void LEGACY_PLUGIN::loadMODULE( MODULE* aModule )
{
    char*   line;
    char*   saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

        // most frequently encountered ones at the top

        if( TESTSUBSTR( "D" ) && strchr( "SCAP", line[1] ) )  // read a drawing item, e.g. "DS"
        {
            loadMODULE_EDGE( aModule );
        }

        else if( TESTLINE( "$PAD" ) )
        {
            loadPAD( aModule );
        }

        // Read a footprint text description (ref, value, or drawing)
        else if( TESTSUBSTR( "T" ) )
        {
            // e.g. "T1 6940 -16220 350 300 900 60 M I 20 N "CFCARD"\r\n"

            int tnum = intParse( line + SZ( "T" ) );

            TEXTE_MODULE* textm = 0;

            switch( tnum )
            {
            case TEXTE_MODULE::TEXT_is_REFERENCE:
                textm = &aModule->Reference();
                break;

            case TEXTE_MODULE::TEXT_is_VALUE:
                textm = &aModule->Value();
                break;

            // All other fields greater than 1.
            default:
                textm = new TEXTE_MODULE( aModule );
                aModule->GraphicalItems().PushBack( textm );
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

            LAYER_NUM layer_num = layerParse( data, &data );
            LAYER_ID  layer_id  = leg_layer2new( m_cu_count,  layer_num );

            long edittime  = hexParse( data, &data );
            time_t timestamp = hexParse( data, &data );

            data = strtok_r( (char*) data+1, delims, &saveptr );

            // data is now a two character long string
            // Note: some old files do not have this field
            if( data && data[0] == 'F' )
                aModule->SetLocked( true );

            if( data && data[1] == 'P' )
                aModule->SetIsPlaced( true );

            aModule->SetPosition( wxPoint( pos_x, pos_y ) );
            aModule->SetLayer( layer_id );
            aModule->SetOrientation( orient );
            aModule->SetTimeStamp( timestamp );
            aModule->SetLastEditTime( edittime );
        }

        /* footprint name set earlier, immediately after MODULE construction
        else if( TESTLINE( "Li" ) )         // Library name of footprint
        {
            // There can be whitespace in the footprint name on some old libraries.
            // Grab everything after "Li" up to end of line:
            //aModule->SetFPID( FROM_UTF8( StrPurge( line + SZ( "Li" ) ) ) );
        }
        */

        else if( TESTLINE( "Sc" ) )         // timestamp
        {
            time_t timestamp = hexParse( line + SZ( "Sc" ) );
            aModule->SetTimeStamp( timestamp );
        }

        else if( TESTLINE( "Op" ) )         // (Op)tions for auto placement
        {
            int itmp1 = hexParse( line + SZ( "Op" ), &data );
            int itmp2 = hexParse( data );

            int cntRot180 = itmp2 & 0x0F;
            if( cntRot180 > 10 )
                cntRot180 = 10;

            aModule->SetPlacementCost180( cntRot180 );

            int cntRot90  = itmp1 & 0x0F;
            if( cntRot90 > 10 )
                cntRot90 = 0;

            itmp1 = (itmp1 >> 4) & 0x0F;
            if( itmp1 > 10 )
                itmp1 = 0;

            aModule->SetPlacementCost90( (itmp1 << 4) | cntRot90 );
        }

        else if( TESTLINE( "At" ) )         // (At)tributes of module
        {
            int attrs = MOD_DEFAULT;

            data = line + SZ( "At" );

            if( strstr( data, "SMD" ) )
                attrs |= MOD_CMS;

            if( strstr( data, "VIRTUAL" ) )
                attrs |= MOD_VIRTUAL;

            aModule->SetAttributes( attrs );
        }

        else if( TESTLINE( "AR" ) )         // Alternate Reference
        {
            // e.g. "AR /47BA2624/45525076"
            data = strtok_r( line + SZ( "AR" ), delims, &saveptr );
            if( data )
                aModule->SetPath( FROM_UTF8( data ) );
        }

        else if( TESTLINE( "$SHAPE3D" ) )
        {
            load3D( aModule );
        }

        else if( TESTLINE( "Cd" ) )
        {
            // e.g. "Cd Double rangee de contacts 2 x 4 pins\r\n"
            aModule->SetDescription( FROM_UTF8( StrPurge( line + SZ( "Cd" ) ) ) );
        }

        else if( TESTLINE( "Kw" ) )         // Key words
        {
            aModule->SetKeywords( FROM_UTF8( StrPurge( line + SZ( "Kw" ) ) ) );
        }

        else if( TESTLINE( ".SolderPasteRatio" ) )
        {
            double tmp = atof( line + SZ( ".SolderPasteRatio" ) );
            // Due to a bug in dialog editor in Modedit, fixed in BZR version 3565
            // this parameter can be broken.
            // It should be >= -50% (no solder paste) and <= 0% (full area of the pad)

            if( tmp < -0.50 )
                tmp = -0.50;
            if( tmp > 0.0 )
                tmp = 0.0;
            aModule->SetLocalSolderPasteMarginRatio( tmp );
        }

        else if( TESTLINE( ".SolderPaste" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderPaste" ) );
            aModule->SetLocalSolderPasteMargin( tmp );
        }

        else if( TESTLINE( ".SolderMask" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderMask" ) );
            aModule->SetLocalSolderMaskMargin( tmp );
        }

        else if( TESTLINE( ".LocalClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( ".LocalClearance" ) );
            aModule->SetLocalClearance( tmp );
        }

        else if( TESTLINE( ".ZoneConnection" ) )
        {
            int tmp = intParse( line + SZ( ".ZoneConnection" ) );
            aModule->SetZoneConnection( (ZoneConnection)tmp );
        }

        else if( TESTLINE( ".ThermalWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( ".ThermalWidth" ) );
            aModule->SetThermalWidth( tmp );
        }

        else if( TESTLINE( ".ThermalGap" ) )
        {
            BIU tmp = biuParse( line + SZ( ".ThermalGap" ) );
            aModule->SetThermalGap( tmp );
        }

        else if( TESTLINE( "$EndMODULE" ) )
        {
            aModule->CalculateBoundingBox();

            return;     // preferred exit
        }
    }

    wxString msg = wxString::Format(
        wxT( "Missing '$EndMODULE' for MODULE '%s'" ),
        GetChars( aModule->GetFPID().GetFootprintName() ) );

    THROW_IO_ERROR( msg );
}


void LEGACY_PLUGIN::loadPAD( MODULE* aModule )
{
    auto_ptr<D_PAD> pad( new D_PAD( aModule ) );
    char*           line;
    char*           saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

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
            while( isSpace( *data ) )
                ++data;

            unsigned char   padchar = (unsigned char) *data++;
            int             padshape;

            BIU     size_x   = biuParse( data, &data );
            BIU     size_y   = biuParse( data, &data );
            BIU     delta_x  = biuParse( data, &data );
            BIU     delta_y  = biuParse( data, &data );
            double  orient   = degParse( data );

            switch( padchar )
            {
            case 'C':   padshape = PAD_SHAPE_CIRCLE;      break;
            case 'R':   padshape = PAD_SHAPE_RECT;        break;
            case 'O':   padshape = PAD_SHAPE_OVAL;        break;
            case 'T':   padshape = PAD_SHAPE_TRAPEZOID;   break;
            default:
                m_error.Printf( _( "Unknown padshape '%c=0x%02x' on line: %d of footprint: '%s'" ),
                                padchar,
                                padchar,
                                m_reader->LineNumber(),
                                GetChars( aModule->GetFPID().GetFootprintName() )
                    );
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
            pad->SetShape( PAD_SHAPE_T( padshape ) );
            pad->SetSize( wxSize( size_x, size_y ) );
            pad->SetDelta( wxSize( delta_x, delta_y ) );
            pad->SetOrientation( orient );
        }

        else if( TESTLINE( "Dr" ) )         // (Dr)ill
        {
            // e.g. "Dr 350 0 0" or "Dr 0 0 0 O 0 0"
            // sscanf( PtLine, "%d %d %d %s %d %d", &m_Drill.x, &m_Offset.x, &m_Offset.y, BufCar, &dx, &dy );

            BIU drill_x = biuParse( line + SZ( "Dr" ), &data );
            BIU drill_y = drill_x;
            BIU offs_x  = biuParse( data, &data );
            BIU offs_y  = biuParse( data, &data );

            PAD_DRILL_SHAPE_T drShape = PAD_DRILL_SHAPE_CIRCLE;

            data = strtok_r( (char*) data, delims, &saveptr );
            if( data )  // optional shape
            {
                if( data[0] == 'O' )
                {
                    drShape = PAD_DRILL_SHAPE_OBLONG;

                    data    = strtok_r( NULL, delims, &saveptr );
                    drill_x = biuParse( data );

                    data    = strtok_r( NULL, delims, &saveptr );
                    drill_y = biuParse( data );
                }
            }

            pad->SetDrillShape( drShape );
            pad->SetOffset( wxPoint( offs_x, offs_y ) );
            pad->SetDrillSize( wxSize( drill_x, drill_y ) );
        }

        else if( TESTLINE( "At" ) )         // (At)tribute
        {
            // e.g. "At SMD N 00888000"
            // sscanf( PtLine, "%s %s %X", BufLine, BufCar, &m_layerMask );

            PAD_ATTR_T  attribute;

            data = strtok_r( line + SZ( "At" ), delims, &saveptr );

            if( !strcmp( data, "SMD" ) )
                attribute = PAD_ATTRIB_SMD;
            else if( !strcmp( data, "CONN" ) )
                attribute = PAD_ATTRIB_CONN;
            else if( !strcmp( data, "HOLE" ) )
                attribute = PAD_ATTRIB_HOLE_NOT_PLATED;
            else
                attribute = PAD_ATTRIB_STANDARD;

            strtok_r( NULL, delims, &saveptr );  // skip BufCar
            data = strtok_r( NULL, delims, &saveptr );

            LEG_MASK layer_mask = hexParse( data );

            pad->SetLayerSet( leg_mask2new( m_cu_count, layer_mask ) );
            pad->SetAttribute( attribute );
        }

        else if( TESTLINE( "Ne" ) )         // (Ne)tname
        {
            // e.g. "Ne 461 "V5.0"

            char    buf[1024];  // can be fairly long
            int     netcode = intParse( line + SZ( "Ne" ), &data );

            // Store the new code mapping
            pad->SetNetCode( getNetCode( netcode ) );

            // read Netname
            ReadDelimitedText( buf, data, sizeof(buf) );
#ifndef NDEBUG
            if( m_board )
                assert( m_board->FindNet( getNetCode( netcode ) )->GetNetname() ==
                        FROM_UTF8( StrPurge( buf ) ) );
#endif /* NDEBUG */
        }

        else if( TESTLINE( "Po" ) )         // (Po)sition
        {
            // e.g. "Po 500 -500"
            wxPoint pos;

            pos.x = biuParse( line + SZ( "Po" ), &data );
            pos.y = biuParse( data );

            pad->SetPos0( pos );
            // pad->SetPosition( pos ); set at function return
        }

        else if( TESTLINE( "Le" ) )
        {
            BIU tmp = biuParse( line + SZ( "Le" ) );
            pad->SetPadToDieLength( tmp );
        }

        else if( TESTLINE( ".SolderMask" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderMask" ) );
            pad->SetLocalSolderMaskMargin( tmp );
        }

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

        else if( TESTLINE( ".ZoneConnection" ) )
        {
            int tmp = intParse( line + SZ( ".ZoneConnection" ) );
            pad->SetZoneConnection( (ZoneConnection)tmp );
        }

        else if( TESTLINE( ".ThermalWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( ".ThermalWidth" ) );
            pad->SetThermalWidth( tmp );
        }

        else if( TESTLINE( ".ThermalGap" ) )
        {
            BIU tmp = biuParse( line + SZ( ".ThermalGap" ) );
            pad->SetThermalGap( tmp );
        }

        else if( TESTLINE( "$EndPAD" ) )
        {
            // pad's "Position" is not relative to the module's,
            // whereas Pos0 is relative to the module's but is the unrotated coordinate.

            wxPoint padpos = pad->GetPos0();

            RotatePoint( &padpos, aModule->GetOrientation() );

            pad->SetPosition( padpos + aModule->GetPosition() );

            aModule->Pads().PushBack( pad.release() );
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$EndPAD'" );
}


void LEGACY_PLUGIN::loadMODULE_EDGE( MODULE* aModule )
{
    STROKE_T    shape;
    char*       line = m_reader->Line();     // obtain current (old) line

    switch( line[1] )
    {
    case 'S':   shape = S_SEGMENT;   break;
    case 'C':   shape = S_CIRCLE;    break;
    case 'A':   shape = S_ARC;       break;
    case 'P':   shape = S_POLYGON;   break;
    default:
        m_error.Printf( wxT( "Unknown EDGE_MODULE type:'%c=0x%02x' on line:%d of module:'%s'" ),
                        (unsigned char) line[1],
                        (unsigned char) line[1],
                        m_reader->LineNumber(),
                        GetChars( aModule->GetFPID().GetFootprintName() )
                        );
        THROW_IO_ERROR( m_error );
    }

    auto_ptr<EDGE_MODULE> dwg( new EDGE_MODULE( aModule, shape ) );    // a drawing

    const char* data;

    // common to all cases, and we have to check their values uniformly at end
    BIU         width = 1;
    LAYER_NUM   layer = FIRST_NON_COPPER_LAYER;

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
            layer   = layerParse( data );

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
            layer   = layerParse( data );

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
            layer   = layerParse( data );

            dwg->m_Start0 = wxPoint( start0_x, start0_y );
            dwg->m_End0   = wxPoint( end0_x, end0_y );

            std::vector<wxPoint> pts;
            pts.reserve( ptCount );

            for( int ii = 0;  ii<ptCount;  ++ii )
            {
                if( ( line = READLINE( m_reader ) ) == NULL )
                {
                    THROW_IO_ERROR( "S_POLGON point count mismatch." );
                }

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
    if( layer < FIRST_LAYER || layer > LAST_NON_COPPER_LAYER )
        layer = SILKSCREEN_N_FRONT;

    dwg->SetWidth( width );
    dwg->SetLayer( leg_layer2new( m_cu_count,  layer ) );

    EDGE_MODULE* em = dwg.release();

    aModule->GraphicalItems().PushBack( em );

    // this had been done at the MODULE level before, presumably because the
    // EDGE_MODULE needs to be already added to a module before this function will work.
    em->SetDrawCoord();
}


void LEGACY_PLUGIN::loadMODULE_TEXT( TEXTE_MODULE* aText )
{
    const char* data;
    const char* txt_end;
    const char* line = m_reader->Line();     // current (old) line
    char*       saveptr;

    // sscanf( line + 1, "%d %d %d %d %d %d %d %s %s %d %s",
    //  &type, &m_Pos0.x, &m_Pos0.y, &m_Size.y, &m_Size.x,
    //  &m_Orient, &m_Thickness, BufCar1, BufCar2, &layer, BufCar3 ) >= 10 )

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
    txt_end = data + ReadDelimitedText( &m_field, data );

#if 1 && defined(DEBUG)
    if( m_field == wxT( "ARM_C8" ) )
    {
        int breakhere = 1;
        (void) breakhere;
    }
#endif

    aText->SetText( m_field );

    // after switching to strtok, there's no easy coming back because of the
    // embedded nul(s?) placed to the right of the current field.
    // (that's the reason why strtok was deprecated...)
    char*   mirror  = strtok_r( (char*) data, delims, &saveptr );
    char*   hide    = strtok_r( NULL, delims, &saveptr );
    char*   tmp     = strtok_r( NULL, delims, &saveptr );

    LAYER_NUM layer_num = tmp ? layerParse( tmp ) : SILKSCREEN_N_FRONT;

    char*   italic  = strtok_r( NULL, delims, &saveptr );

    char*   hjust   = strtok_r( (char*) txt_end, delims, &saveptr );
    char*   vjust   = strtok_r( NULL, delims, &saveptr );

    if( type != TEXTE_MODULE::TEXT_is_REFERENCE
     && type != TEXTE_MODULE::TEXT_is_VALUE )
        type = TEXTE_MODULE::TEXT_is_DIVERS;

    aText->SetType( static_cast<TEXTE_MODULE::TEXT_TYPE>( type ) );

    aText->SetPos0( wxPoint( pos0_x, pos0_y ) );
    aText->SetSize( wxSize( size0_x, size0_y ) );

    orient -= ( static_cast<MODULE*>( aText->GetParent() ) )->GetOrientation();

    aText->SetOrientation( orient );

    // @todo put in accessors?
    // Set a reasonable width:
    if( thickn < 1 )
        thickn = 1;

    /*  this is better left to the dialogs UIs
    aText->SetThickness( Clamp_Text_PenSize( thickn, aText->GetSize() ) );
    */

    aText->SetThickness( thickn );

    aText->SetMirrored( mirror && *mirror == 'M' );

    aText->SetVisible( !(hide && *hide == 'I') );

    aText->SetItalic( italic && *italic == 'I' );

    if( hjust )
        aText->SetHorizJustify( horizJustify( hjust ) );

    if( vjust )
        aText->SetVertJustify( vertJustify( vjust ) );

    if( layer_num < FIRST_LAYER )
        layer_num = FIRST_LAYER;
    else if( layer_num > LAST_NON_COPPER_LAYER )
        layer_num = LAST_NON_COPPER_LAYER;
    else if( layer_num == LAYER_N_BACK )
        layer_num = SILKSCREEN_N_BACK;
    else if( layer_num == LAYER_N_FRONT )
        layer_num = SILKSCREEN_N_FRONT;

    aText->SetLayer( leg_layer2new( m_cu_count,  layer_num ) );

    // Calculate the actual position.
    aText->SetDrawCoord();
}


void LEGACY_PLUGIN::load3D( MODULE* aModule )
{
    S3D_MASTER* t3D = aModule->Models();

    if( !t3D->GetShape3DName().IsEmpty() )
    {
        S3D_MASTER* n3D = new S3D_MASTER( aModule );

        aModule->Models().PushBack( n3D );

        t3D = n3D;
    }

    char*   line;
    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        if( TESTLINE( "Na" ) )     // Shape File Name
        {
            char    buf[512];
            ReadDelimitedText( buf, line + SZ( "Na" ), sizeof(buf) );
            t3D->SetShape3DName( FROM_UTF8( buf ) );
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


void LEGACY_PLUGIN::loadPCB_LINE()
{
    /* example:
        $DRAWSEGMENT
        Po 0 57500 -1000 57500 0 150
        De 24 0 900 0 0
        $EndDRAWSEGMENT
    */

    auto_ptr<DRAWSEGMENT>   dseg( new DRAWSEGMENT( m_board ) );

    char*   line;
    char*   saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

        if( TESTLINE( "Po" ) )
        {
            // sscanf( line + 2, " %d %d %d %d %d %d", &m_Shape, &m_Start.x, &m_Start.y, &m_End.x, &m_End.y, &m_Width );
            int shape   = intParse( line + SZ( "Po" ), &data );
            BIU start_x = biuParse( data, &data );
            BIU start_y = biuParse( data, &data );
            BIU end_x   = biuParse( data, &data );
            BIU end_y   = biuParse( data, &data );
            BIU width   = biuParse( data );

            if( width < 0 )
                width = 0;

            dseg->SetShape( STROKE_T( shape ) );
            dseg->SetWidth( width );
            dseg->SetStart( wxPoint( start_x, start_y ) );
            dseg->SetEnd( wxPoint( end_x, end_y ) );
        }

        else if( TESTLINE( "De" ) )
        {
            BIU     x = 0;
            BIU     y;

            data = strtok_r( line + SZ( "De" ), delims, &saveptr );
            for( int i = 0;  data;  ++i, data = strtok_r( NULL, delims, &saveptr ) )
            {
                switch( i )
                {
                case 0:
                    LAYER_NUM layer;
                    layer = layerParse( data );

                    if( layer < FIRST_NON_COPPER_LAYER )
                        layer = FIRST_NON_COPPER_LAYER;

                    else if( layer > LAST_NON_COPPER_LAYER )
                        layer = LAST_NON_COPPER_LAYER;

                    dseg->SetLayer( leg_layer2new( m_cu_count,  layer ) );
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
                    time_t timestamp;
                    timestamp = hexParse( data );
                    dseg->SetTimeStamp( timestamp );
                    break;
                case 4:
                    STATUS_FLAGS state;
                    state = static_cast<STATUS_FLAGS>( hexParse( data ) );
                    dseg->SetState( state, true );
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

void LEGACY_PLUGIN::loadNETINFO_ITEM()
{
    /* a net description is something like
     * $EQUIPOT
     * Na 5 "/BIT1"
     * St ~
     * $EndEQUIPOT
     */

    char  buf[1024];

    NETINFO_ITEM*   net = NULL;
    char*           line;
    int             netCode = 0;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

        if( TESTLINE( "Na" ) )
        {
            // e.g. "Na 58 "/cpu.sch/PAD7"\r\n"

            netCode = intParse( line + SZ( "Na" ), &data );

            ReadDelimitedText( buf, data, sizeof(buf) );

            if( net == NULL )
                net = new NETINFO_ITEM( m_board, FROM_UTF8( buf ), netCode );
            else
            {
                THROW_IO_ERROR( "Two net definitions in  '$EQUIPOT' block" );
            }
        }

        else if( TESTLINE( "$EndEQUIPOT" ) )
        {
            // net 0 should be already in list, so store this net
            // if it is not the net 0, or if the net 0 does not exists.
            if( net && ( net->GetNet() > 0 || m_board->FindNet( 0 ) == NULL ) )
            {
                m_board->AppendNet( net );

                // Be sure we have room to store the net in m_netCodes
                if( (int)m_netCodes.size() <= netCode )
                    m_netCodes.resize( netCode+1 );

                m_netCodes[netCode] = net->GetNet();
                net = NULL;
            }
            else
            {
                delete net;
                net = NULL;     // Avoid double deletion.
            }

            return;     // preferred exit
        }
    }

    // If we are here, there is an error.
    delete net;
    THROW_IO_ERROR( "Missing '$EndEQUIPOT'" );
}


void LEGACY_PLUGIN::loadPCB_TEXT()
{
    /*  examples:
        For a single line text:
        ----------------------
        $TEXTPCB
        Te "Text example"
        Po 66750 53450 600 800 150 0
        De 24 1 0 Italic
        $EndTEXTPCB

        For a multi line text:
        ---------------------
        $TEXTPCB
        Te "Text example"
        Nl "Line 2"
        Po 66750 53450 600 800 150 0
        De 24 1 0 Italic
        $EndTEXTPCB
        Nl "line nn" is a line added to the current text
    */

    char    text[1024];

    // maybe someday a constructor that takes all this data in one call?
    TEXTE_PCB*  pcbtxt = new TEXTE_PCB( m_board );
    m_board->Add( pcbtxt, ADD_APPEND );

    char*   line;
    char*   saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

        if( TESTLINE( "Te" ) )          // Text line (or first line for multi line texts)
        {
            ReadDelimitedText( text, line + SZ( "Te" ), sizeof(text) );
            pcbtxt->SetText( FROM_UTF8( text ) );
        }

        else if( TESTLINE( "nl" ) )     // next line of the current text
        {
            ReadDelimitedText( text, line + SZ( "nl" ), sizeof(text) );
            pcbtxt->SetText( pcbtxt->GetText() + wxChar( '\n' ) +  FROM_UTF8( text ) );
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

            pcbtxt->SetTextPosition( wxPoint( pos_x, pos_y ) );
        }

        else if( TESTLINE( "De" ) )
        {
            // e.g. "De 21 1 0 Normal C\r\n"
            // sscanf( line + 2, " %d %d %lX %s %c\n", &m_Layer, &normal_display, &m_TimeStamp, style, &hJustify );

            LAYER_NUM layer_num = layerParse( line + SZ( "De" ), &data );
            int     notMirrored = intParse( data, &data );
            time_t  timestamp   = hexParse( data, &data );
            char*   style       = strtok_r( (char*) data, delims, &saveptr );
            char*   hJustify    = strtok_r( NULL, delims, &saveptr );
            char*   vJustify    = strtok_r( NULL, delims, &saveptr );

            pcbtxt->SetMirrored( !notMirrored );
            pcbtxt->SetTimeStamp( timestamp );
            pcbtxt->SetItalic( !strcmp( style, "Italic" ) );

            if( hJustify )
                pcbtxt->SetHorizJustify( horizJustify( hJustify ) );
            else
            {
                // boom, somebody changed a constructor, I was relying on this:
                wxASSERT( pcbtxt->GetHorizJustify() == GR_TEXT_HJUSTIFY_CENTER );
            }

            if( vJustify )
                pcbtxt->SetVertJustify( vertJustify( vJustify ) );

            if( layer_num < FIRST_COPPER_LAYER )
                layer_num = FIRST_COPPER_LAYER;
            else if( layer_num > LAST_NON_COPPER_LAYER )
                layer_num = LAST_NON_COPPER_LAYER;

            if( layer_num >= FIRST_NON_COPPER_LAYER ||
                is_leg_copperlayer_valid( m_cu_count, layer_num ) )
                pcbtxt->SetLayer( leg_layer2new( m_cu_count, layer_num ) );
            else    // not perfect, but putting this text on front layer is a workaround
                pcbtxt->SetLayer( F_Cu );
        }

        else if( TESTLINE( "$EndTEXTPCB" ) )
        {
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$EndTEXTPCB'" );
}


void LEGACY_PLUGIN::loadTrackList( int aStructType )
{
    char*   line;
    char*   saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        // read two lines per loop iteration, each loop is one TRACK or VIA
        // example first line:
        // e.g. "Po 0 23994 28800 24400 28800 150 -1"  for a track
        // e.g. "Po 3 21086 17586 21086 17586 180 -1"  for a via (uses sames start and end)

        const char* data;

        if( line[0] == '$' )    // $EndTRACK
            return;             // preferred exit

        // int arg_count = sscanf( line + 2, " %d %d %d %d %d %d %d", &shape, &tempStartX, &tempStartY, &tempEndX, &tempEndY, &width, &drill );

        assert( TESTLINE( "Po" ) );

        VIATYPE_T viatype = static_cast<VIATYPE_T>( intParse( line + SZ( "Po" ), &data ));
        BIU start_x = biuParse( data, &data );
        BIU start_y = biuParse( data, &data );
        BIU end_x   = biuParse( data, &data );
        BIU end_y   = biuParse( data, &data );
        BIU width   = biuParse( data, &data );

        // optional 7th drill parameter (must be optional in an old format?)
        data = strtok_r( (char*) data, delims, &saveptr );

        BIU drill   = data ? biuParse( data ) : -1;     // SetDefault() if < 0

        // Read the 2nd line to determine the exact type, one of:
        // PCB_TRACE_T, PCB_VIA_T, or PCB_ZONE_T.  The type field in 2nd line
        // differentiates between PCB_TRACE_T and PCB_VIA_T.  With virtual
        // functions in use, it is critical to instantiate the PCB_VIA_T
        // exactly.
        READLINE( m_reader );

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
        time_t      timeStamp;
        LAYER_NUM   layer_num;
        int         type, net_code, flags_int;

        // parse the 2nd line to determine the type of object
        // e.g. "De 15 1 7 0 0"   for a via
        sscanf( line + SZ( "De" ), " %d %d %d %lX %X", &layer_num, &type, &net_code,
                &timeStamp, &flags_int );

        STATUS_FLAGS flags;

        flags = static_cast<STATUS_FLAGS>( flags_int );

        if( aStructType==PCB_TRACE_T && type==1 )
            makeType = PCB_VIA_T;
        else
            makeType = aStructType;

        TRACK* newTrack;

        switch( makeType )
        {
        default:
        case PCB_TRACE_T:
            newTrack = new TRACK( m_board );
            break;

        case PCB_VIA_T:
            newTrack = new VIA( m_board );
            break;

        case PCB_ZONE_T:     // this is now deprecated, but exist in old boards
            newTrack = new SEGZONE( m_board );
            break;
        }

        newTrack->SetTimeStamp( timeStamp );

        newTrack->SetPosition( wxPoint( start_x, start_y ) );
        newTrack->SetEnd( wxPoint( end_x, end_y ) );

        newTrack->SetWidth( width );

        if( makeType == PCB_VIA_T )     // Ensure layers are OK when possible:
        {
            VIA *via = static_cast<VIA*>( newTrack );
            via->SetViaType( viatype );

            if( drill < 0 )
                via->SetDrillDefault();
            else
                via->SetDrill( drill );

            if( via->GetViaType() == VIA_THROUGH )
                via->SetLayerPair( F_Cu, B_Cu );
            else
            {
                LAYER_ID  back  = leg_layer2new( m_cu_count, (layer_num >> 4) & 0xf );
                LAYER_ID  front = leg_layer2new( m_cu_count, layer_num & 0xf );

                if( is_leg_copperlayer_valid( m_cu_count, back ) &&
                    is_leg_copperlayer_valid( m_cu_count, front ) )
                    via->SetLayerPair( front, back );
                else
                {
                    delete via;
                    newTrack = NULL;
                }
            }
        }
        else
        {
            // A few legacy boards can have tracks on non existent layers, because
            // reducing the number of layers does not remove tracks on removed layers
            // If happens, skip them
            if( is_leg_copperlayer_valid( m_cu_count, layer_num ) )
                newTrack->SetLayer( leg_layer2new( m_cu_count, layer_num ) );
            else
            {
                delete newTrack;
                newTrack = NULL;
            }
        }

        if( newTrack )
        {
            newTrack->SetNetCode( getNetCode( net_code ) );
            newTrack->SetState( flags, true );

            m_board->Add( newTrack );
        }
    }

    THROW_IO_ERROR( "Missing '$EndTRACK'" );
}


void LEGACY_PLUGIN::loadNETCLASS()
{
    char        buf[1024];
    wxString    netname;
    char*       line;

    // create an empty NETCLASS without a name, but do not add it to the BOARD
    // yet since that would bypass duplicate netclass name checking within the BOARD.
    // store it temporarily in an auto_ptr until successfully inserted into the BOARD
    // just before returning.
    NETCLASSPTR nc = boost::make_shared<NETCLASS>( wxEmptyString );

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
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
            if( !m_board->GetDesignSettings().m_NetClasses.Add( nc ) )
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


void LEGACY_PLUGIN::loadZONE_CONTAINER()
{
    auto_ptr<ZONE_CONTAINER> zc( new ZONE_CONTAINER( m_board ) );

    CPolyLine::HATCH_STYLE outline_hatch = CPolyLine::NO_HATCH;
    bool    sawCorner = false;
    char    buf[1024];
    char*   line;
    char*   saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

        if( TESTLINE( "ZCorner" ) )         // new corner found
        {
            // e.g. "ZCorner 25650 49500 0"
            BIU x    = biuParse( line + SZ( "ZCorner" ), &data );
            BIU y    = biuParse( data, &data );
            int flag = intParse( data );

            if( !sawCorner )
                zc->Outline()->Start( zc->GetLayer(), x, y, outline_hatch );
            else
                zc->AppendCorner( wxPoint( x, y ) );

            sawCorner = true;

            if( flag )
                zc->Outline()->CloseLastContour();
        }

        else if( TESTLINE( "ZInfo" ) )      // general info found
        {
            // e.g. 'ZInfo 479194B1 310 "COMMON"'
            time_t  timestamp = hexParse( line + SZ( "ZInfo" ), &data );
            int     netcode   = intParse( data, &data );

            if( ReadDelimitedText( buf, data, sizeof(buf) ) > (int) sizeof(buf) )
            {
                THROW_IO_ERROR( "ZInfo netname too long" );
            }

            zc->SetTimeStamp( timestamp );
            // Init the net code only, not the netname, to be sure
            // the zone net name is the name read in file.
            // (When mismatch, the user will be prompted in DRC, to fix the actual name)
            zc->BOARD_CONNECTED_ITEM::SetNetCode( getNetCode( netcode ) );
        }

        else if( TESTLINE( "ZLayer" ) )     // layer found
        {
            LAYER_NUM layer_num = layerParse( line + SZ( "ZLayer" ) );
            zc->SetLayer( leg_layer2new( m_cu_count,  layer_num ) );
        }

        else if( TESTLINE( "ZAux" ) )       // aux info found
        {
            // e.g. "ZAux 7 E"
            int     ignore = intParse( line + SZ( "ZAux" ), &data );
            char*   hopt   = strtok_r( (char*) data, delims, &saveptr );

            if( !hopt )
            {
                m_error.Printf( wxT( "Bad ZAux for CZONE_CONTAINER '%s'" ), zc->GetNetname().GetData() );
                THROW_IO_ERROR( m_error );
            }

            switch( *hopt )   // upper case required
            {
            case 'N':   outline_hatch = CPolyLine::NO_HATCH;        break;
            case 'E':   outline_hatch = CPolyLine::DIAGONAL_EDGE;   break;
            case 'F':   outline_hatch = CPolyLine::DIAGONAL_FULL;   break;

            default:
                m_error.Printf( wxT( "Bad ZAux for CZONE_CONTAINER '%s'" ), zc->GetNetname().GetData() );
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

            if( smoothing >= ZONE_SETTINGS::SMOOTHING_LAST || smoothing < 0 )
            {
                m_error.Printf( wxT( "Bad ZSmoothing for CZONE_CONTAINER '%s'" ), zc->GetNetname().GetData() );
                THROW_IO_ERROR( m_error );
            }

            zc->SetCornerSmoothingType( smoothing );
            zc->SetCornerRadius( cornerRadius );
        }

        else if( TESTLINE( "ZKeepout" ) )
        {
            zc->SetIsKeepout( true );
            // e.g. "ZKeepout tracks N vias N pads Y"
           data = strtok_r( line + SZ( "ZKeepout" ), delims, &saveptr );

            while( data )
            {
                if( !strcmp( data, "tracks" ) )
                {
                    data = strtok_r( NULL, delims, &saveptr );
                    zc->SetDoNotAllowTracks( data && *data == 'N' );
                }
                else if( !strcmp( data, "vias" ) )
                {
                    data = strtok_r( NULL, delims, &saveptr );
                    zc->SetDoNotAllowVias( data && *data == 'N' );
                }
                else if( !strcmp( data, "copperpour" ) )
                {
                    data = strtok_r( NULL, delims, &saveptr );
                    zc->SetDoNotAllowCopperPour( data && *data == 'N' );
                }

                data = strtok_r( NULL, delims, &saveptr );
            }
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

            zc->SetArcSegmentCount( arcsegcount );
            zc->SetIsFilled( fillstate == 'S' );
            zc->SetThermalReliefGap( thermalReliefGap );
            zc->SetThermalReliefCopperBridge( thermalReliefCopperBridge );
        }

        else if( TESTLINE( "ZClearance" ) )     // Clearance and pad options info found
        {
            // e.g. "ZClearance 40 I"
            BIU     clearance = biuParse( line + SZ( "ZClearance" ), &data );
            char*   padoption = strtok_r( (char*) data, delims, &saveptr );  // data: " I"

            ZoneConnection popt;
            switch( *padoption )
            {
            case 'I': popt = PAD_ZONE_CONN_FULL;        break;
            case 'T': popt = PAD_ZONE_CONN_THERMAL;     break;
            case 'H': popt = PAD_ZONE_CONN_THT_THERMAL; break;
            case 'X': popt = PAD_ZONE_CONN_NONE;        break;

            default:
                m_error.Printf( wxT( "Bad ZClearance padoption for CZONE_CONTAINER '%s'" ),
                    zc->GetNetname().GetData() );
                THROW_IO_ERROR( m_error );
            }

            zc->SetZoneClearance( clearance );
            zc->SetPadConnection( popt );
        }

        else if( TESTLINE( "ZMinThickness" ) )
        {
            BIU thickness = biuParse( line + SZ( "ZMinThickness" ) );
            zc->SetMinThickness( thickness );
        }

        else if( TESTLINE( "ZPriority" ) )
        {
            int priority = intParse( line + SZ( "ZPriority" ) );
            zc->SetPriority( priority );
        }

        else if( TESTLINE( "$POLYSCORNERS" ) )
        {
            // Read the PolysList (polygons used for fill areas in the zone)
            SHAPE_POLY_SET polysList;

            bool makeNewOutline = true;

            while( ( line = READLINE( m_reader ) ) != NULL )
            {
                if( TESTLINE( "$endPOLYSCORNERS" ) )
                    break;

                // e.g. "39610 43440 0 0"
                BIU     x = biuParse( line, &data );
                BIU     y = biuParse( data, &data );

                if( makeNewOutline )
                    polysList.NewOutline();

                polysList.Append( x, y );

                bool end_contour = intParse( data, &data );  // end_countour was a bool when file saved, so '0' or '1' here
                intParse( data ); // skip corner utility flag

                makeNewOutline = end_contour;
            }

            zc->AddFilledPolysList( polysList );
        }

        else if( TESTLINE( "$FILLSEGMENTS" ) )
        {
            while( ( line = READLINE( m_reader ) ) != NULL )
            {
                if( TESTLINE( "$endFILLSEGMENTS" ) )
                    break;

                // e.g. ""%d %d %d %d\n"
                BIU sx = biuParse( line, &data );
                BIU sy = biuParse( data, &data );
                BIU ex = biuParse( data, &data );
                BIU ey = biuParse( data );

                zc->FillSegments().push_back( SEGMENT( wxPoint( sx, sy ), wxPoint( ex, ey ) ) );
            }
        }

        else if( TESTLINE( "$endCZONE_OUTLINE" ) )
        {
            // Ensure keepout does not have a net
            // (which have no sense for a keepout zone)
            if( zc->GetIsKeepout() )
                zc->SetNetCode( NETINFO_LIST::UNCONNECTED );

            // should always occur, but who knows, a zone without two corners
            // is no zone at all, it's a spot?

            if( zc->GetNumCorners() > 2 )
            {
                if( !zc->IsOnCopperLayer() )
                {
                    zc->SetFillMode( 0 );
                    zc->SetNetCode( NETINFO_LIST::UNCONNECTED );
                }

                // Hatch here, after outlines corners are read
                // Set hatch here, after outlines corners are read
                zc->Outline()->SetHatch( outline_hatch,
                                         Mils2iu( CPolyLine::GetDefaultHatchPitchMils() ),
                                         true );

                m_board->Add( zc.release() );
            }

            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( "Missing '$endCZONE_OUTLINE'" );
}


void LEGACY_PLUGIN::loadDIMENSION()
{
    auto_ptr<DIMENSION> dim( new DIMENSION( m_board ) );

    char*   line;
    char*   saveptr;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char*  data;

        if( TESTLINE( "$endCOTATION" ) )
        {
            m_board->Add( dim.release(), ADD_APPEND );
            return;     // preferred exit
        }

        else if( TESTLINE( "Va" ) )
        {
            BIU value = biuParse( line + SZ( "Va" ) );
            dim->SetValue( value );
        }

        else if( TESTLINE( "Ge" ) )
        {
            LAYER_NUM layer_num;
            time_t  timestamp;
            int     shape;
            int     ilayer;

            sscanf( line + SZ( "Ge" ), " %d %d %lX", &shape, &ilayer, &timestamp );

            if( ilayer < FIRST_NON_COPPER_LAYER )
                layer_num = FIRST_NON_COPPER_LAYER;
            else if( ilayer > LAST_NON_COPPER_LAYER )
                layer_num = LAST_NON_COPPER_LAYER;
            else
                layer_num = ilayer;

            dim->SetLayer( leg_layer2new( m_cu_count,  layer_num ) );
            dim->SetTimeStamp( timestamp );
            dim->SetShape( shape );
        }

        else if( TESTLINE( "Te" ) )
        {
            char  buf[2048];

            ReadDelimitedText( buf, line + SZ( "Te" ), sizeof(buf) );
            dim->SetText( FROM_UTF8( buf ) );
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
            char*   mirror = strtok_r( (char*) data, delims, &saveptr );

            // This sets both DIMENSION's position and internal m_Text's.
            // @todo: But why do we even know about internal m_Text?
            dim->SetPosition( wxPoint( pos_x, pos_y ) );
            dim->SetTextSize( wxSize( width, height ) );

            dim->Text().SetMirrored( mirror && *mirror == '0' );
            dim->Text().SetThickness( thickn );
            dim->Text().SetOrientation( orient );
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

            dim->m_crossBarO.x = crossBarOx;
            dim->m_crossBarO.y = crossBarOy;
            dim->m_crossBarF.x = crossBarFx;
            dim->m_crossBarF.y = crossBarFy;
            dim->SetWidth( width );
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

            dim->m_featureLineDO.x = featureLineDOx;
            dim->m_featureLineDO.y = featureLineDOy;
            dim->m_featureLineDF.x = featureLineDFx;
            dim->m_featureLineDF.y = featureLineDFy;
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

            dim->m_featureLineGO.x = featureLineGOx;
            dim->m_featureLineGO.y = featureLineGOy;
            dim->m_featureLineGF.x = featureLineGFx;
            dim->m_featureLineGF.y = featureLineGFy;
            (void) ignore;
        }

        else if( TESTLINE( "S1" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_arrowD1Ox, &m_arrowD1Oy, &m_arrowD1Fx, &m_arrowD1Fy, &Dummy );

            int ignore      = intParse( line + SZ( "S1" ), &data );
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );    // skipping excessive data
            BIU arrowD1Fx   = biuParse( data, &data );
            BIU arrowD1Fy   = biuParse( data );

            dim->m_arrowD1F.x = arrowD1Fx;
            dim->m_arrowD1F.y = arrowD1Fy;
            (void) ignore;
        }

        else if( TESTLINE( "S2" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_arrowD2Ox, &m_arrowD2Oy, &m_arrowD2Fx, &m_arrowD2Fy, &Dummy );

            int ignore    = intParse( line + SZ( "S2" ), &data );
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );    // skipping excessive data
            BIU arrowD2Fx = biuParse( data, &data );
            BIU arrowD2Fy = biuParse( data, &data );

            dim->m_arrowD2F.x = arrowD2Fx;
            dim->m_arrowD2F.y = arrowD2Fy;
            (void) ignore;
        }

        else if( TESTLINE( "S3" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d\n", &Dummy, &m_arrowG1Ox, &m_arrowG1Oy, &m_arrowG1Fx, &m_arrowG1Fy, &Dummy );
            int ignore    = intParse( line + SZ( "S3" ), &data );
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );    // skipping excessive data
            BIU arrowG1Fx = biuParse( data, &data );
            BIU arrowG1Fy = biuParse( data, &data );

            dim->m_arrowG1F.x = arrowG1Fx;
            dim->m_arrowG1F.y = arrowG1Fy;
            (void) ignore;
        }

        else if( TESTLINE( "S4" ) )
        {
            // sscanf( Line + 2, " %d %d %d %d %d %d", &Dummy, &m_arrowG2Ox, &m_arrowG2Oy, &m_arrowG2Fx, &m_arrowG2Fy, &Dummy );
            int ignore    = intParse( line + SZ( "S4" ), &data );
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );    // skipping excessive data
            BIU arrowG2Fx = biuParse( data, &data );
            BIU arrowG2Fy = biuParse( data, &data );

            dim->m_arrowG2F.x = arrowG2Fx;
            dim->m_arrowG2F.y = arrowG2Fy;
            (void) ignore;
        }
    }

    THROW_IO_ERROR( "Missing '$endCOTATION'" );
}


void LEGACY_PLUGIN::loadPCB_TARGET()
{
    char* line;

    while( ( line = READLINE( m_reader ) ) != NULL )
    {
        const char* data;

        if( TESTLINE( "$EndPCB_TARGET" ) || TESTLINE( "$EndMIREPCB" ) )
        {
            return;     // preferred exit
        }

        else if( TESTLINE( "Po" ) )
        {
            // sscanf( Line + 2, " %X %d %d %d %d %d %lX", &m_Shape, &m_Layer, &m_Pos.x, &m_Pos.y, &m_Size, &m_Width, &m_TimeStamp );

            int shape = intParse( line + SZ( "Po" ), &data );

            LAYER_NUM layer_num = layerParse( data, &data );

            BIU pos_x = biuParse( data, &data );
            BIU pos_y = biuParse( data, &data );
            BIU size  = biuParse( data, &data );
            BIU width = biuParse( data, &data );
            time_t timestamp = hexParse( data );

            if( layer_num < FIRST_NON_COPPER_LAYER )
                layer_num = FIRST_NON_COPPER_LAYER;

            else if( layer_num > LAST_NON_COPPER_LAYER )
                layer_num = LAST_NON_COPPER_LAYER;

            PCB_TARGET* t = new PCB_TARGET( m_board, shape, leg_layer2new( m_cu_count,  layer_num ),
                                    wxPoint( pos_x, pos_y ), size, width );
            m_board->Add( t, ADD_APPEND );

            t->SetTimeStamp( timestamp );
        }
    }

    THROW_IO_ERROR( "Missing '$EndDIMENSION'" );
}


BIU LEGACY_PLUGIN::biuParse( const char* aValue, const char** nptrptr )
{
    char*   nptr;

    errno = 0;

    double fval = strtod( aValue, &nptr );

    if( errno )
    {
        m_error.Printf( _( "invalid float number in file: '%s'\nline: %d, offset: %d" ),
            m_reader->GetSource().GetData(),
            m_reader->LineNumber(), aValue - m_reader->Line() + 1 );

        THROW_IO_ERROR( m_error );
    }

    if( aValue == nptr )
    {
        m_error.Printf( _( "missing float number in file: '%s'\nline: %d, offset: %d" ),
            m_reader->GetSource().GetData(),
            m_reader->LineNumber(), aValue - m_reader->Line() + 1 );

        THROW_IO_ERROR( m_error );
    }

    if( nptrptr )
        *nptrptr = nptr;

    fval *= diskToBiu;

    // fval is up into the whole number realm here, and should be bounded
    // within INT_MIN to INT_MAX since BIU's are nanometers.
    return KiROUND( fval );
}


double LEGACY_PLUGIN::degParse( const char* aValue, const char** nptrptr )
{
    char*   nptr;

    errno = 0;

    double fval = strtod( aValue, &nptr );

    if( errno )
    {
        m_error.Printf( _( "invalid float number in file: '%s'\nline: %d, offset: %d" ),
            m_reader->GetSource().GetData(), m_reader->LineNumber(), aValue - m_reader->Line() + 1 );

        THROW_IO_ERROR( m_error );
    }

    if( aValue == nptr )
    {
        m_error.Printf( _( "missing float number in file: '%s'\nline: %d, offset: %d" ),
            m_reader->GetSource().GetData(), m_reader->LineNumber(), aValue - m_reader->Line() + 1 );

        THROW_IO_ERROR( m_error );
    }

    if( nptrptr )
        *nptrptr = nptr;

    return fval;
}


void LEGACY_PLUGIN::init( const PROPERTIES* aProperties )
{
    m_loading_format_version = 0;
    m_cu_count = 16;
    m_board = NULL;
    m_props = aProperties;

    // conversion factor for saving RAM BIUs to KICAD legacy file format.
    biuToDisk = 1.0/IU_PER_MM;      // BIUs are nanometers & file is mm

    // Conversion factor for loading KICAD legacy file format into BIUs in RAM
    // Start by assuming the *.brd file is in deci-mils.
    // If we see "Units mm" in the $GENERAL section, set diskToBiu to 1000000.0
    // then, during the file loading process, to start a conversion from
    // mm to nanometers.  The deci-mil legacy files have no such "Units" marker
    // so we must assume the file is in deci-mils until told otherwise.

    diskToBiu = IU_PER_DECIMILS;    // BIUs are nanometers
}


void LEGACY_PLUGIN::SaveModule3D( const MODULE* me ) const
{
    for( S3D_MASTER* t3D = me->Models();  t3D;  t3D = t3D->Next() )
    {
        if( !t3D->GetShape3DName().IsEmpty() )
        {
            fprintf( m_fp, "$SHAPE3D\n" );

            fprintf( m_fp, "Na %s\n", EscapedUTF8( t3D->GetShape3DName() ).c_str() );

            fprintf(m_fp,
#if defined(DEBUG)
                    // use old formats for testing, just to verify compatibility
                    // using "diff", then switch to more concise form for release builds.
                    "Sc %lf %lf %lf\n",
#else
                    "Sc %.10g %.10g %.10g\n",
#endif
                    t3D->m_MatScale.x,
                    t3D->m_MatScale.y,
                    t3D->m_MatScale.z );

            fprintf(m_fp,
#if defined(DEBUG)
                    "Of %lf %lf %lf\n",
#else
                    "Of %.10g %.10g %.10g\n",
#endif
                    t3D->m_MatPosition.x,
                    t3D->m_MatPosition.y,
                    t3D->m_MatPosition.z );

            fprintf(m_fp,
#if defined(DEBUG)
                    "Ro %lf %lf %lf\n",
#else
                    "Ro %.10g %.10g %.10g\n",
#endif
                    t3D->m_MatRotation.x,
                    t3D->m_MatRotation.y,
                    t3D->m_MatRotation.z );

            fprintf( m_fp, "$EndSHAPE3D\n" );
        }
    }
}


#if 0

//-----<BOARD Save Functions>---------------------------------------------------


static inline const char* ShowVertJustify( EDA_TEXT_VJUSTIFY_T vertical )
{
    const char* rs;
    switch( vertical )
    {
    case GR_TEXT_VJUSTIFY_TOP:      rs = "T";   break;
    case GR_TEXT_VJUSTIFY_CENTER:   rs = "C";   break;
    case GR_TEXT_VJUSTIFY_BOTTOM:   rs = "B";   break;
    default:                        rs = "?";   break;
    }
    return rs;
}


static inline const char* ShowHorizJustify( EDA_TEXT_HJUSTIFY_T horizontal )
{
    const char* rs;
    switch( horizontal )
    {
    case GR_TEXT_HJUSTIFY_LEFT:     rs = "L";   break;
    case GR_TEXT_HJUSTIFY_CENTER:   rs = "C";   break;
    case GR_TEXT_HJUSTIFY_RIGHT:    rs = "R";   break;
    default:                        rs = "?";   break;
    }
    return rs;
}


#define SPBUFZ  50      // wire all usages of this together.

int LEGACY_PLUGIN::biuSprintf( char* buf, BIU aValue ) const
{
    double  engUnits = biuToDisk * aValue;
    int     len;

    if( engUnits != 0.0 && fabsl( engUnits ) <= 0.0001 )
    {
        len = snprintf( buf, SPBUFZ, "%.10f", engUnits );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        ++len;
    }
    else
    {
        // The %.10g is about optimal since we are dealing with a bounded
        // range on aValue, and we can be sure that there will never
        // be a reason to have more than 6 digits to the right of the
        // decimal point because we are converting from integer
        // (signed whole numbers) nanometers to mm.  A value of
        // 0.000001 is one nanometer, the smallest positive nonzero value
        // that we can ever have here.  If you ever see a board file with
        // more digits to the right of the decimal point than 6, this is a
        // possibly a bug in a formatting string nearby.
        len = snprintf( buf, SPBUFZ, "%.10g", engUnits );
    }
    return len;
}


std::string LEGACY_PLUGIN::fmtBIU( BIU aValue ) const
{
    char    temp[SPBUFZ];

    int len = biuSprintf( temp, aValue );

    return std::string( temp, len );
}


std::string LEGACY_PLUGIN::fmtDEG( double aAngle ) const
{
    char    temp[50];

    // @todo a hook site to convert from tenths degrees to degrees for BOARD_FORMAT_VERSION 2.

    // MINGW: snprintf() comes from gcc folks, sprintf() comes from Microsoft.
    int len = snprintf( temp, sizeof( temp ), "%.10g", aAngle );

    return std::string( temp, len );
}


std::string LEGACY_PLUGIN::fmtBIUPair( BIU first, BIU second ) const
{
    char    temp[2*SPBUFZ+2];
    char*   cp = temp;

    cp += biuSprintf( cp, first );

    *cp++ = ' ';

    cp += biuSprintf( cp, second );

    return std::string( temp, cp - temp );
}

void LEGACY_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    FILE* fp = wxFopen( aFileName, wxT( "w" ) );
    if( !fp )
    {
        m_error.Printf( _( "Unable to open file '%s'" ), aFileName.GetData() );
        THROW_IO_ERROR( m_error );
    }

    m_filename = aFileName;

    // wxf now owns fp, will close on exception or return
    wxFFile wxf( fp );

    m_fp = fp;          // member function accessibility

    wxString header = wxString::Format(
        wxT( "PCBNEW-BOARD Version %d date %s\n\n# Created by Pcbnew%s\n\n" ),
        LEGACY_BOARD_FILE_VERSION, DateAndTime().GetData(),
        GetBuildVersion().GetData() );

    // save a file header, if caller provided one (with trailing \n hopefully).
    fprintf( m_fp, "%s", TO_UTF8( header ) );

    SaveBOARD( aBoard );
}


wxString LEGACY_PLUGIN::writeError() const
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


// With the advent of the LSET expansion it was agreed to abort the legacy save since
// we'd have to expand the old format in order to suppor the new LAYER_IDs.

void LEGACY_PLUGIN::SaveBOARD( const BOARD* aBoard ) const
{
    m_mapping->SetBoard( aBoard );

    saveGENERAL( aBoard );

    saveSHEET( aBoard );

    saveSETUP( aBoard );

    saveBOARD_ITEMS( aBoard );
}


void LEGACY_PLUGIN::saveGENERAL( const BOARD* aBoard ) const
{
    fprintf( m_fp, "$GENERAL\n" );
    fprintf( m_fp, "encoding utf-8\n" );

    // tell folks the units used within the file, as early as possible here.
    fprintf( m_fp, "Units mm\n" );

    // Write copper layer count
    fprintf( m_fp, "LayerCount %d\n", aBoard->GetCopperLayerCount() );

    /*  No, EnabledLayers has this information, plus g_TabAllCopperLayerMask is
        global and globals are not allowed in a plugin.
    fprintf( m_fp,
             "Ly %8X\n",
             g_TabAllCopperLayerMask[NbLayers - 1] | ALL_NO_CU_LAYERS );
    */

    fprintf( m_fp, "EnabledLayers %08X\n",  aBoard->GetEnabledLayers() );

    if( aBoard->GetEnabledLayers() != aBoard->GetVisibleLayers() )
        fprintf( m_fp, "VisibleLayers %08X\n", aBoard->GetVisibleLayers() );

    fprintf( m_fp, "Links %d\n",            aBoard->GetRatsnestsCount() );
    fprintf( m_fp, "NoConn %d\n",           aBoard->GetUnconnectedNetCount() );

    // Write Bounding box info
    EDA_RECT bbbox = ((BOARD*)aBoard)->ComputeBoundingBox();

    fprintf( m_fp,  "Di %s %s\n",
                    fmtBIUPair( bbbox.GetX(), bbbox.GetY() ).c_str(),
                    fmtBIUPair( bbbox.GetRight(), bbbox.GetBottom() ).c_str() );

    fprintf( m_fp, "Ndraw %d\n",            aBoard->m_Drawings.GetCount() );
    fprintf( m_fp, "Ntrack %d\n",           aBoard->GetNumSegmTrack() );
    fprintf( m_fp, "Nzone %d\n",            aBoard->GetNumSegmZone() );
    fprintf( m_fp, "BoardThickness %s\n",   fmtBIU( aBoard->GetDesignSettings().GetBoardThickness() ).c_str() );
    fprintf( m_fp, "Nmodule %d\n",          aBoard->m_Modules.GetCount() );
    fprintf( m_fp, "Nnets %d\n",            m_mapping->GetSize() );
    fprintf( m_fp, "$EndGENERAL\n\n" );
}


void LEGACY_PLUGIN::saveSHEET( const BOARD* aBoard ) const
{
    const PAGE_INFO&    pageInfo = aBoard->GetPageSettings();
    const TITLE_BLOCK&  tb = ((BOARD*)aBoard)->GetTitleBlock();

    fprintf( m_fp, "$SHEETDESCR\n" );

    // paper is described in mils
    fprintf( m_fp,  "Sheet %s %d %d%s\n",
                    TO_UTF8( pageInfo.GetType() ),
                    pageInfo.GetWidthMils(),
                    pageInfo.GetHeightMils(),
                    !pageInfo.IsCustom() && pageInfo.IsPortrait() ?
                        " portrait" : ""
                    );

    fprintf( m_fp, "Title %s\n",        EscapedUTF8( tb.GetTitle() ).c_str() );
    fprintf( m_fp, "Date %s\n",         EscapedUTF8( tb.GetDate() ).c_str() );
    fprintf( m_fp, "Rev %s\n",          EscapedUTF8( tb.GetRevision() ).c_str() );
    fprintf( m_fp, "Comp %s\n",         EscapedUTF8( tb.GetCompany() ).c_str() );
    fprintf( m_fp, "Comment1 %s\n",     EscapedUTF8( tb.GetComment1() ).c_str() );
    fprintf( m_fp, "Comment2 %s\n",     EscapedUTF8( tb.GetComment2() ).c_str() );
    fprintf( m_fp, "Comment3 %s\n",     EscapedUTF8( tb.GetComment3() ).c_str() );
    fprintf( m_fp, "Comment4 %s\n",     EscapedUTF8( tb.GetComment4() ).c_str() );
    fprintf( m_fp, "$EndSHEETDESCR\n\n" );
}


void LEGACY_PLUGIN::saveSETUP( const BOARD* aBoard ) const
{
    const BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();
    NETCLASSPTR netclass_default     = bds.GetDefault();

    fprintf( m_fp, "$SETUP\n" );

    /*  Internal units are nobody's business, they are internal.
        Units used in the file are now in the "Units" attribute of $GENERAL.
    fprintf( m_fp,, "InternalUnit %f INCH\n", 1.0 / PCB_LEGACY_INTERNAL_UNIT );
    */

    fprintf( m_fp, "Layers %d\n", aBoard->GetCopperLayerCount() );

    unsigned layerMask = ALL_CU_LAYERS & aBoard->GetEnabledLayers();

    for( LAYER_NUM layer = FIRST_LAYER; layer <= LAST_COPPER_LAYER; ++layer )
    {
        if( layerMask & MASK( layer ) )
        {
            fprintf( m_fp, "Layer[%d] %s %s\n", layer,
                     TO_UTF8( aBoard->GetLayerName( layer ) ),
                     LAYER::ShowType( aBoard->GetLayerType( layer ) ) );
        }
    }

    // Save current default track width, for compatibility with older Pcbnew version;
    fprintf( m_fp, "TrackWidth %s\n",
             fmtBIU( aBoard->GetDesignSettings().GetCurrentTrackWidth() ).c_str() );

    // Save custom tracks width list (the first is not saved here: this is the netclass value
    for( unsigned ii = 1; ii < aBoard->GetDesignSettings().m_TrackWidthList.size(); ii++ )
        fprintf( m_fp, "TrackWidthList %s\n", fmtBIU( aBoard->GetDesignSettings().m_TrackWidthList[ii] ).c_str() );

    fprintf( m_fp, "TrackClearence %s\n",  fmtBIU( netclass_default->GetClearance() ).c_str() );

    // ZONE_SETTINGS
    fprintf( m_fp, "ZoneClearence %s\n", fmtBIU( aBoard->GetZoneSettings().m_ZoneClearance ).c_str() );
    fprintf( m_fp, "Zone_45_Only %d\n", aBoard->GetZoneSettings().m_Zone_45_Only );

    fprintf( m_fp, "TrackMinWidth %s\n", fmtBIU( bds.m_TrackMinWidth ).c_str() );

    fprintf( m_fp, "DrawSegmWidth %s\n", fmtBIU( bds.m_DrawSegmentWidth ).c_str() );
    fprintf( m_fp, "EdgeSegmWidth %s\n", fmtBIU( bds.m_EdgeSegmentWidth ).c_str() );

    // Save current default via size, for compatibility with older Pcbnew version;
    fprintf( m_fp, "ViaSize %s\n",  fmtBIU( netclass_default->GetViaDiameter() ).c_str() );
    fprintf( m_fp, "ViaDrill %s\n", fmtBIU( netclass_default->GetViaDrill() ).c_str() );
    fprintf( m_fp, "ViaMinSize %s\n", fmtBIU( bds.m_ViasMinSize ).c_str() );
    fprintf( m_fp, "ViaMinDrill %s\n", fmtBIU( bds.m_ViasMinDrill ).c_str() );

    // Save custom vias diameters list (the first is not saved here: this is
    // the netclass value
    for( unsigned ii = 1; ii < aBoard->GetDesignSettings().m_ViasDimensionsList.size(); ii++ )
        fprintf( m_fp, "ViaSizeList %s %s\n",
                 fmtBIU( aBoard->GetDesignSettings().m_ViasDimensionsList[ii].m_Diameter ).c_str(),
                 fmtBIU( aBoard->GetDesignSettings().m_ViasDimensionsList[ii].m_Drill ).c_str() );

    // for old versions compatibility:
    fprintf( m_fp, "MicroViaSize %s\n", fmtBIU( netclass_default->GetuViaDiameter() ).c_str() );
    fprintf( m_fp, "MicroViaDrill %s\n", fmtBIU( netclass_default->GetuViaDrill() ).c_str() );
    fprintf( m_fp, "MicroViasAllowed %s\n", fmtBIU( bds.m_MicroViasAllowed ).c_str() );
    fprintf( m_fp, "MicroViaMinSize %s\n", fmtBIU( bds.m_MicroViasMinSize ).c_str() );
    fprintf( m_fp, "MicroViaMinDrill %s\n", fmtBIU( bds.m_MicroViasMinDrill ).c_str() );

    fprintf( m_fp, "TextPcbWidth %s\n", fmtBIU( bds.m_PcbTextWidth ).c_str() );
    fprintf( m_fp, "TextPcbSize %s\n",  fmtBIUSize( bds.m_PcbTextSize ).c_str() );

    fprintf( m_fp, "EdgeModWidth %s\n", fmtBIU( bds.m_ModuleSegmentWidth ).c_str() );
    fprintf( m_fp, "TextModSize %s\n", fmtBIUSize( bds.m_ModuleTextSize ).c_str() );
    fprintf( m_fp, "TextModWidth %s\n", fmtBIU( bds.m_ModuleTextWidth ).c_str() );

    fprintf( m_fp, "PadSize %s\n", fmtBIUSize( bds.m_Pad_Master.GetSize() ).c_str() );
    fprintf( m_fp, "PadDrill %s\n", fmtBIU( bds.m_Pad_Master.GetDrillSize().x ).c_str() );

    fprintf( m_fp, "Pad2MaskClearance %s\n", fmtBIU( bds.m_SolderMaskMargin ).c_str() );
    fprintf( m_fp, "SolderMaskMinWidth %s\n", fmtBIU( bds.m_SolderMaskMinWidth ).c_str() );

    if( bds.m_SolderPasteMargin != 0 )
        fprintf( m_fp, "Pad2PasteClearance %s\n", fmtBIU( bds.m_SolderPasteMargin ).c_str() );

    if( bds.m_SolderPasteMarginRatio != 0 )
        fprintf( m_fp, "Pad2PasteClearanceRatio %g\n", bds.m_SolderPasteMarginRatio );

    fprintf( m_fp, "GridOrigin %s\n", fmtBIUPoint( aBoard->GetGridOrigin() ).c_str() );
    fprintf( m_fp, "AuxiliaryAxisOrg %s\n", fmtBIUPoint( aBoard->GetAuxOrigin() ).c_str() );

    fprintf( m_fp, "VisibleElements %X\n", bds.GetVisibleElements() );

    {
        STRING_FORMATTER sf;

        aBoard->GetPlotOptions().Format( &sf, 0 );

        wxString record = FROM_UTF8( sf.GetString().c_str() );

        record.Replace( wxT("\n"), wxT(""), true );
        record.Replace( wxT("  "), wxT(" "), true);

        fprintf( m_fp, "PcbPlotParams %s\n", TO_UTF8( record ) );
    }

    fprintf( m_fp, "$EndSETUP\n\n" );
}


void LEGACY_PLUGIN::saveBOARD_ITEMS( const BOARD* aBoard ) const
{
    // save the nets
    for( NETINFO_MAPPING::iterator net = m_mapping->begin(), netEnd = m_mapping->end();
            net != netEnd; ++net )
    {
        saveNETINFO_ITEM( *net );
    }

    // Saved nets do not include netclass names, so save netclasses after nets.
    saveNETCLASSES( &aBoard->GetDesignSettings().m_NetClasses );

    // save the modules
    for( MODULE* m = aBoard->m_Modules;  m;  m = (MODULE*) m->Next() )
        saveMODULE( m );

    // save the graphics owned by the board (not owned by a module)
    for( BOARD_ITEM* gr = aBoard->m_Drawings;  gr;  gr = gr->Next() )
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
            saveDIMENSION( (DIMENSION*) gr );
            break;
        default:
            THROW_IO_ERROR( wxString::Format( UNKNOWN_GRAPHIC_FORMAT, gr->Type() ) );
        }
    }

    // do not save MARKER_PCBs, they can be regenerated easily

    // save the tracks & vias
    fprintf( m_fp, "$TRACK\n" );
    for( TRACK* track = aBoard->m_Track;  track; track = track->Next() )
        saveTRACK( track );
    fprintf( m_fp, "$EndTRACK\n" );

    // save the old obsolete zones which were done by segments (tracks)
    fprintf( m_fp, "$ZONE\n" );
    for( SEGZONE* zone = aBoard->m_Zone;  zone;  zone = zone->Next() )
        saveTRACK( zone );
    fprintf( m_fp, "$EndZONE\n" );

    // save the polygon (which are the newer technology) zones
    for( int i=0;  i < aBoard->GetAreaCount();  ++i )
        saveZONE_CONTAINER( aBoard->GetArea( i ) );

    fprintf( m_fp, "$EndBOARD\n" );

    CHECK_WRITE_ERROR();
}


void LEGACY_PLUGIN::saveNETINFO_ITEM( const NETINFO_ITEM* aNet ) const
{
    fprintf( m_fp, "$EQUIPOT\n" );
    fprintf( m_fp, "Na %d %s\n", m_mapping->Translate( aNet->GetNet() ),
                                 EscapedUTF8( aNet->GetNetname() ).c_str() );
    fprintf( m_fp, "St %s\n", "~" );
    fprintf( m_fp, "$EndEQUIPOT\n" );

    CHECK_WRITE_ERROR();
}


void LEGACY_PLUGIN::saveNETCLASSES( const NETCLASSES* aNetClasses ) const
{
    // save the default first.
    saveNETCLASS( aNetClasses->GetDefault() );

    // the rest will be alphabetical in the *.brd file.
    for( NETCLASSES::const_iterator it = aNetClasses->begin();  it != aNetClasses->end();  ++it )
    {
        NETCLASSPTR   netclass = it->second;
        saveNETCLASS( netclass );
    }

    CHECK_WRITE_ERROR();
}


void LEGACY_PLUGIN::saveNETCLASS( const NETCLASSPTR nc ) const
{
    fprintf( m_fp, "$NCLASS\n" );
    fprintf( m_fp, "Name %s\n", EscapedUTF8( nc->GetName() ).c_str() );
    fprintf( m_fp, "Desc %s\n", EscapedUTF8( nc->GetDescription() ).c_str() );

    fprintf( m_fp, "Clearance %s\n",    fmtBIU( nc->GetClearance() ).c_str() );
    fprintf( m_fp, "TrackWidth %s\n",   fmtBIU( nc->GetTrackWidth() ).c_str() );

    fprintf( m_fp, "ViaDia %s\n",       fmtBIU( nc->GetViaDiameter() ).c_str() );
    fprintf( m_fp, "ViaDrill %s\n",     fmtBIU( nc->GetViaDrill() ).c_str() );

    fprintf( m_fp, "uViaDia %s\n",      fmtBIU( nc->GetuViaDiameter() ).c_str() );
    fprintf( m_fp, "uViaDrill %s\n",    fmtBIU( nc->GetuViaDrill() ).c_str() );

    for( NETCLASS::const_iterator it = nc->begin();  it!=nc->end();  ++it )
        fprintf( m_fp, "AddNet %s\n", EscapedUTF8( *it ).c_str() );

    fprintf( m_fp, "$EndNCLASS\n" );

    CHECK_WRITE_ERROR();
}


void LEGACY_PLUGIN::saveMODULE_TEXT( const TEXTE_MODULE* me ) const
{
    MODULE* parent = (MODULE*) me->GetParent();
    double  orient = me->GetOrientation();

    // Due to the Pcbnew history, m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    if( parent )
        orient += parent->GetOrientation();

    wxString txt = me->GetText();

    fprintf( m_fp,  "T%d %s %s %s %s %c %c %d %c %s",
                    me->GetType(),
                    fmtBIUPoint( me->GetPos0() ).c_str(),   // m_Pos0.x, m_Pos0.y,

                    // legacy has goofed reversed order: ( y, x )
                    fmtBIUPair( me->GetSize().y, me->GetSize().x ).c_str(),

                    fmtDEG( orient ).c_str(),
                    fmtBIU( me->GetThickness() ).c_str(),   // m_Thickness,
                    me->IsMirrored() ? 'M' : 'N',
                    me->IsVisible() ? 'V' : 'I',
                    me->GetLayer(),
                    me->IsItalic() ? 'I' : 'N',
                    EscapedUTF8( txt ).c_str()
                    );

    if( me->GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER ||
        me->GetVertJustify()  != GR_TEXT_VJUSTIFY_CENTER )
    {
        fprintf( m_fp,  " %s %s\n",
                        ShowHorizJustify( me->GetHorizJustify() ),
                        ShowVertJustify( me->GetVertJustify() )
                        );
    }
    else
        fprintf( m_fp, "\n" );

    CHECK_WRITE_ERROR();
}


void LEGACY_PLUGIN::saveMODULE_EDGE( const EDGE_MODULE* me ) const
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


void LEGACY_PLUGIN::savePAD( const D_PAD* me ) const
{
    fprintf( m_fp, "$PAD\n" );

    int cshape;

    switch( me->GetShape() )
    {
    case PAD_SHAPE_CIRCLE:    cshape = 'C';   break;
    case PAD_SHAPE_RECT:      cshape = 'R';   break;
    case PAD_SHAPE_OVAL:      cshape = 'O';   break;
    case PAD_SHAPE_TRAPEZOID: cshape = 'T';   break;

    default:
        THROW_IO_ERROR( wxString::Format( UNKNOWN_PAD_FORMAT, me->GetShape() ) );
    }

#if BOARD_FORMAT_VERSION == 1       // saving mode is a compile time option

    wxString    wpadname = me->GetPadName();    // universal character set padname
    std::string spadname;

    for( unsigned i = 0; wpadname.size(); ++i )
    {
        // truncate from universal character down to 8 bit foreign jibber
        // jabber byte.  This basically duplicates what was done in the old
        // BOARD_FORMAT_VERSION 1 code.  Any characters that were in the 8 bit
        // character space were OK.
        spadname += (char) wpadname[i];
    }

    fprintf( m_fp,  "Sh \"%s\" %c %s %s %s\n",
                    spadname.c_str(),  // probably ASCII, but possibly jibber jabber
#else

    fprintf( m_fp,  "Sh %s %c %s %s %s\n",
                    // legacy VERSION 2 simply uses UTF8, wrapped in quotes,
                    // and 99.99 % of the time there is no difference between 1 & 2,
                    // since ASCII is a subset of UTF8.  But if they were not using
                    // ASCII pad names, then there is a difference in the file.
                    EscapedUTF8( me->GetPadName() ).c_str(),
#endif
                    cshape,
                    fmtBIUSize( me->GetSize() ).c_str(),
                    fmtBIUSize( me->GetDelta() ).c_str(),
                    fmtDEG( me->GetOrientation() ).c_str() );

    fprintf( m_fp,  "Dr %s %s",
                    fmtBIU( me->GetDrillSize().x ).c_str(),
                    fmtBIUPoint( me->GetOffset() ).c_str() );

    if( me->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG )
    {
        fprintf( m_fp, " %c %s", 'O', fmtBIUSize( me->GetDrillSize() ).c_str() );
    }

    fprintf( m_fp, "\n" );

    const char* texttype;

    switch( me->GetAttribute() )
    {
    case PAD_ATTRIB_STANDARD:          texttype = "STD";       break;
    case PAD_ATTRIB_SMD:               texttype = "SMD";       break;
    case PAD_ATTRIB_CONN:              texttype = "CONN";      break;
    case PAD_ATTRIB_HOLE_NOT_PLATED:   texttype = "HOLE";      break;

    default:
        THROW_IO_ERROR( wxString::Format( UNKNOWN_PAD_ATTRIBUTE, me->GetAttribute() ) );
    }

    fprintf( m_fp, "At %s N %08X\n", texttype, me->GetLayerSet() );

    fprintf( m_fp, "Ne %d %s\n", m_mapping->Translate( me->GetNetCode() ),
             EscapedUTF8( me->GetNetname() ).c_str() );

    fprintf( m_fp, "Po %s\n", fmtBIUPoint( me->GetPos0() ).c_str() );

    if( me->GetPadToDieLength() != 0 )
        fprintf( m_fp, "Le %s\n", fmtBIU( me->GetPadToDieLength() ).c_str() );

    if( me->GetLocalSolderMaskMargin() != 0 )
        fprintf( m_fp, ".SolderMask %s\n", fmtBIU( me->GetLocalSolderMaskMargin() ).c_str() );

    if( me->GetLocalSolderPasteMargin() != 0 )
        fprintf( m_fp, ".SolderPaste %s\n", fmtBIU( me->GetLocalSolderPasteMargin() ).c_str() );

    double ratio = me->GetLocalSolderPasteMarginRatio();
    if( ratio != 0.0 )
        fprintf( m_fp, ".SolderPasteRatio %g\n", ratio );

    if( me->GetLocalClearance() != 0 )
        fprintf( m_fp, ".LocalClearance %s\n", fmtBIU( me->GetLocalClearance( ) ).c_str() );

    if( me->GetZoneConnection() != PAD_ZONE_CONN_INHERITED )
        fprintf( m_fp, ".ZoneConnection %d\n", me->GetZoneConnection() );

    if( me->GetThermalWidth() != 0 )
        fprintf( m_fp, ".ThermalWidth %s\n", fmtBIU( me->GetThermalWidth() ).c_str() );

    if( me->GetThermalGap() != 0 )
        fprintf( m_fp, ".ThermalGap %s\n", fmtBIU( me->GetThermalGap() ).c_str() );

    fprintf( m_fp, "$EndPAD\n" );

    CHECK_WRITE_ERROR();
}


void LEGACY_PLUGIN::saveMODULE( const MODULE* me ) const
{
    char        statusTxt[3];
    double      orient = me->GetOrientation();

    // Do not save full FPID.  Only the footprint name.  The legacy file format should
    // never support FPIDs.
    fprintf( m_fp, "$MODULE %s\n", me->GetFPID().GetFootprintName().c_str() );

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

    fprintf( m_fp, "Li %s\n", me->GetFPID().GetFootprintName().c_str() );

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
    fprintf( m_fp, "Op %X %X 0\n", me->GetPlacementCost90(), me->GetPlacementCost180() );

    if( me->GetLocalSolderMaskMargin() != 0 )
        fprintf( m_fp, ".SolderMask %s\n", fmtBIU( me->GetLocalSolderMaskMargin() ).c_str() );

    if( me->GetLocalSolderPasteMargin() != 0 )
        fprintf( m_fp, ".SolderPaste %s\n", fmtBIU( me->GetLocalSolderPasteMargin() ).c_str() );

    double ratio = me->GetLocalSolderPasteMarginRatio();
    if( ratio != 0.0 )
        fprintf( m_fp, ".SolderPasteRatio %g\n", ratio );

    if( me->GetLocalClearance() != 0 )
        fprintf( m_fp, ".LocalClearance %s\n", fmtBIU( me->GetLocalClearance( ) ).c_str() );

    if( me->GetZoneConnection() != PAD_ZONE_CONN_INHERITED )
        fprintf( m_fp, ".ZoneConnection %d\n", me->GetZoneConnection() );

    if( me->GetThermalWidth() != 0 )
        fprintf( m_fp, ".ThermalWidth %s\n", fmtBIU( me->GetThermalWidth() ).c_str() );

    if( me->GetThermalGap() != 0 )
        fprintf( m_fp, ".ThermalGap %s\n", fmtBIU( me->GetThermalGap() ).c_str() );

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

    saveMODULE_TEXT( &me->Reference() );

    saveMODULE_TEXT( &me->Value() );

    // save drawing elements
    for( BOARD_ITEM* gr = me->GraphicalItems();  gr;  gr = gr->Next() )
    {
        switch( gr->Type() )
        {
        case PCB_MODULE_TEXT_T:
            saveMODULE_TEXT( static_cast<TEXTE_MODULE*>( gr ));
            break;
        case PCB_MODULE_EDGE_T:
            saveMODULE_EDGE( static_cast<EDGE_MODULE*>( gr ));
            break;
        default:
            THROW_IO_ERROR( wxString::Format( UNKNOWN_GRAPHIC_FORMAT, gr->Type() ) );
        }
    }

    for( D_PAD* pad = me->Pads();  pad;  pad = pad->Next() )
        savePAD( pad );

    SaveModule3D( me );

    fprintf( m_fp, "$EndMODULE %s\n", me->GetFPID().GetFootprintName().c_str() );

    CHECK_WRITE_ERROR();
}


void LEGACY_PLUGIN::savePCB_TARGET( const PCB_TARGET* me ) const
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


void LEGACY_PLUGIN::savePCB_LINE( const DRAWSEGMENT* me ) const
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


void LEGACY_PLUGIN::saveTRACK( const TRACK* me ) const
{
    int type = 0;
    VIATYPE_T viatype = VIA_NOT_DEFINED;
    int drill = UNDEFINED_DRILL_DIAMETER;

    if( me->Type() == PCB_VIA_T )
    {
        const VIA *via = static_cast<const VIA *>(me);
        type = 1;
        viatype = via->GetViaType();
        drill = via->GetDrill();
    }

    fprintf(m_fp, "Po %d %s %s %s %s\n",
            viatype,
            fmtBIUPoint( me->GetStart() ).c_str(),
            fmtBIUPoint( me->GetEnd() ).c_str(),
            fmtBIU( me->GetWidth() ).c_str(),
            drill == UNDEFINED_DRILL_DIAMETER ?
                "-1" :  fmtBIU( drill ).c_str() );

    fprintf(m_fp, "De %d %d %d %lX %X\n",
            me->GetLayer(), type, m_mapping->Translate( me->GetNetCode() ),
            me->GetTimeStamp(), me->GetStatus() );
}


void LEGACY_PLUGIN::saveZONE_CONTAINER( const ZONE_CONTAINER* me ) const
{
    fprintf( m_fp, "$CZONE_OUTLINE\n" );

    // Save the outline main info
    // For keepout zones, net code and net name are irrelevant, so we store a dummy value
    // just for ZONE_CONTAINER compatibility
    fprintf( m_fp,  "ZInfo %lX %d %s\n",
                    me->GetTimeStamp(),
                    me->GetIsKeepout() ? 0 : m_mapping->Translate( me->GetNetCode() ),
                    EscapedUTF8( me->GetIsKeepout() ? wxT("") : me->GetNetname() ).c_str() );

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

    if( me->GetPriority() > 0 )
        fprintf( m_fp, "ZPriority %d\n", me->GetPriority() );

    // Save pad option and clearance
    char padoption;

    switch( me->GetPadConnection() )
    {
    default:
    case PAD_ZONE_CONN_FULL:        padoption = 'I';  break;
    case PAD_ZONE_CONN_THERMAL:     padoption = 'T';  break;
    case PAD_ZONE_CONN_THT_THERMAL: padoption = 'H';  break; // H is for 'hole' since it reliefs holes only
    case PAD_ZONE_CONN_NONE:        padoption = 'X';  break;
    }

    fprintf( m_fp,  "ZClearance %s %c\n",
                    fmtBIU( me->GetZoneClearance() ).c_str(),
                    padoption );

    fprintf( m_fp, "ZMinThickness %s\n", fmtBIU( me->GetMinThickness() ).c_str() );

    fprintf( m_fp,  "ZOptions %d %d %c %s %s\n",
                    me->GetFillMode(),
                    me->GetArcSegmentCount(),
                    me->IsFilled() ? 'S' : 'F',
                    fmtBIU( me->GetThermalReliefGap() ).c_str(),
                    fmtBIU( me->GetThermalReliefCopperBridge() ).c_str() );

    if( me->GetIsKeepout() )
    {
        fprintf( m_fp,  "ZKeepout tracks %c vias %c copperpour %c\n",
                        me->GetDoNotAllowTracks() ? 'N' : 'Y',
                        me->GetDoNotAllowVias() ? 'N' : 'Y',
                        me->GetDoNotAllowCopperPour() ? 'N' : 'Y' );
    }

    fprintf( m_fp,  "ZSmoothing %d %s\n",
                    me->GetCornerSmoothingType(),
                    fmtBIU( me->GetCornerRadius() ).c_str() );

    // Save the corner list
    const CPOLYGONS_LIST& cv = me->Outline()->m_CornersList;

    for( unsigned it = 0; it < cv.GetCornersCount(); ++it )
    {
        fprintf( m_fp,  "ZCorner %s %d\n",
                        fmtBIUPair( cv.GetX( it ), cv.GetY( it ) ).c_str(),
                        cv.IsEndContour( it ) );
    }

    // Save the PolysList
    const CPOLYGONS_LIST& fv = me->GetFilledPolysList();
    if( fv.GetCornersCount() )
    {
        fprintf( m_fp, "$POLYSCORNERS\n" );

        for( unsigned it = 0; it < fv.GetCornersCount(); ++it )
        {
            fprintf( m_fp, "%s %d %d\n",
                           fmtBIUPair( fv.GetX( it ), fv.GetY( it ) ).c_str(),
                           fv.IsEndContour( it ),
                           fv.GetUtility( it )  );
        }

        fprintf( m_fp, "$endPOLYSCORNERS\n" );
    }

    typedef std::vector< SEGMENT > SEGMENTS;

    // Save the filling segments list
    const SEGMENTS& segs = me->FillSegments();

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


void LEGACY_PLUGIN::saveDIMENSION( const DIMENSION* me ) const
{
    // note: COTATION was the previous name of DIMENSION
    // this old keyword is used here for compatibility
    fprintf( m_fp, "$COTATION\n" );

    fprintf( m_fp, "Ge %d %d %lX\n", me->GetShape(), me->GetLayer(), me->GetTimeStamp() );

    fprintf( m_fp, "Va %s\n", fmtBIU( me->GetValue() ).c_str() );

    if( !me->GetText().IsEmpty() )
        fprintf( m_fp, "Te %s\n", EscapedUTF8( me->GetText() ).c_str() );
    else
        fprintf( m_fp, "Te \"?\"\n" );

    fprintf( m_fp,  "Po %s %s %s %s %d\n",
                    fmtBIUPoint( me->Text().GetTextPosition() ).c_str(),
                    fmtBIUSize( me->Text().GetSize() ).c_str(),
                    fmtBIU( me->Text().GetThickness() ).c_str(),
                    fmtDEG( me->Text().GetOrientation() ).c_str(),
                    me->Text().IsMirrored() ? 0 : 1     // strange but true
                    );

    fprintf( m_fp,  "Sb %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_crossBarO.x, me->m_crossBarO.y ).c_str(),
                    fmtBIUPair( me->m_crossBarF.x, me->m_crossBarF.y ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "Sd %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_featureLineDO.x, me->m_featureLineDO.y ).c_str(),
                    fmtBIUPair( me->m_featureLineDF.x, me->m_featureLineDF.y ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "Sg %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_featureLineGO.x, me->m_featureLineGO.y ).c_str(),
                    fmtBIUPair( me->m_featureLineGF.x, me->m_featureLineGF.y ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "S1 %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_crossBarF.x, me->m_crossBarF.y ).c_str(),
                    fmtBIUPair( me->m_arrowD1F.x, me->m_arrowD1F.y ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "S2 %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_crossBarF.x, me->m_crossBarF.y ).c_str(),
                    fmtBIUPair( me->m_arrowD2F.x, me->m_arrowD2F.y ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "S3 %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_crossBarO.x, me->m_crossBarO.y ).c_str(),
                    fmtBIUPair( me->m_arrowG1F.x, me->m_arrowG1F.y ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp,  "S4 %d %s %s %s\n", S_SEGMENT,
                    fmtBIUPair( me->m_crossBarO.x, me->m_crossBarO.y ).c_str(),
                    fmtBIUPair( me->m_arrowG2F.x, me->m_arrowG2F.y ).c_str(),
                    fmtBIU( me->GetWidth() ).c_str() );

    fprintf( m_fp, "$endCOTATION\n" );

    CHECK_WRITE_ERROR();
}


void LEGACY_PLUGIN::savePCB_TEXT( const TEXTE_PCB* me ) const
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
                    fmtBIUPoint( me->GetTextPosition() ).c_str(),
                    fmtBIUSize( me->GetSize() ).c_str(),
                    fmtBIU( me->GetThickness() ).c_str(),
                    fmtDEG( me->GetOrientation() ).c_str() );

    fprintf( m_fp,  "De %d %d %lX %s",
                    me->GetLayer(),
                    !me->IsMirrored(),
                    me->GetTimeStamp(),
                    me->IsItalic() ? "Italic" : "Normal" );

    if( me->GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER ||
        me->GetVertJustify()  != GR_TEXT_VJUSTIFY_CENTER )
    {
        fprintf( m_fp,  " %s %s\n",
                        ShowHorizJustify( me->GetHorizJustify() ),
                        ShowVertJustify( me->GetVertJustify() )
                        );
    }
    else
        fprintf( m_fp, "\n" );

    fprintf( m_fp, "$EndTEXTPCB\n" );
}

#endif  // NO LEGACY_PLUGIN::Save()


//-----<FOOTPRINT LIBRARY FUNCTIONS>--------------------------------------------

/*

    The legacy file format is being obsoleted and this code will have a short
    lifetime, so it only needs to be good enough for a short duration of time.
    Caching all the MODULEs is a bit memory intensive, but it is a considerably
    faster way of fulfilling the API contract. Otherwise, without the cache, you
    would have to re-read the file when searching for any MODULE, and this would
    be very problematic filling a FOOTPRINT_LIST via this PLUGIN API. If memory
    becomes a concern, consider the cache lifetime policy, which determines the
    time that a LP_CACHE is in RAM. Note PLUGIN lifetime also plays a role in
    cache lifetime.

*/


#include <boost/ptr_container/ptr_map.hpp>
#include <wx/filename.h>

typedef boost::ptr_map< std::string, MODULE >   MODULE_MAP;
typedef MODULE_MAP::iterator                    MODULE_ITER;
typedef MODULE_MAP::const_iterator              MODULE_CITER;


/**
 * Class LP_CACHE
 * assists only for the footprint portion of the PLUGIN API, and only for the
 * LEGACY_PLUGIN, so therefore is private to this implementation file, i.e. not placed
 * into a header.
 */
struct LP_CACHE
{
    LEGACY_PLUGIN*  m_owner;        // my owner, I need its LEGACY_PLUGIN::loadMODULE()
    wxString        m_lib_path;
    wxDateTime      m_mod_time;
    MODULE_MAP      m_modules;      // map or tuple of footprint_name vs. MODULE*
    bool            m_writable;

    LP_CACHE( LEGACY_PLUGIN* aOwner, const wxString& aLibraryPath );

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any PLUGIN.
    // Catch these exceptions higher up please.

    /// save the entire legacy library to m_lib_path;
    void Save();

    void SaveHeader( FILE* aFile );

    void SaveIndex( FILE* aFile );

    void SaveModules( FILE* aFile );

    void SaveEndOfFile( FILE* aFile )
    {
        fprintf( aFile, "$EndLIBRARY\n" );
    }

    void Load();

    void ReadAndVerifyHeader( LINE_READER* aReader );

    void SkipIndex( LINE_READER* aReader );

    void LoadModules( LINE_READER* aReader );

    wxDateTime  GetLibModificationTime();
};


LP_CACHE::LP_CACHE( LEGACY_PLUGIN* aOwner, const wxString& aLibraryPath ) :
    m_owner( aOwner ),
    m_lib_path( aLibraryPath ),
    m_writable( true )
{
}


wxDateTime LP_CACHE::GetLibModificationTime()
{
    wxFileName  fn( m_lib_path );

    // update the writable flag while we have a wxFileName, in a network this
    // is possibly quite dynamic anyway.
    m_writable = fn.IsFileWritable();

    return fn.GetModificationTime();
}


void LP_CACHE::Load()
{
    FILE_LINE_READER    reader( m_lib_path );

    ReadAndVerifyHeader( &reader );
    SkipIndex( &reader );
    LoadModules( &reader );

    // Remember the file modification time of library file when the
    // cache snapshot was made, so that in a networked environment we will
    // reload the cache as needed.
    m_mod_time = GetLibModificationTime();
}


void LP_CACHE::ReadAndVerifyHeader( LINE_READER* aReader )
{
    char* line = aReader->ReadLine();
    char* saveptr;

    if( !line )
        goto L_bad_library;

    if( !TESTLINE( "PCBNEW-LibModule-V1" ) )
        goto L_bad_library;

    while( ( line = aReader->ReadLine() ) != NULL )
    {
        if( TESTLINE( "Units" ) )
        {
            const char* units = strtok_r( line + SZ( "Units" ), delims, &saveptr );

            if( !strcmp( units, "mm" ) )
            {
                m_owner->diskToBiu = IU_PER_MM;
            }

        }
        else if( TESTLINE( "$INDEX" ) )
            return;
    }

L_bad_library:
    THROW_IO_ERROR( wxString::Format( _( "File '%s' is empty or is not a legacy library" ),
        m_lib_path.GetData() ) );
}


void LP_CACHE::SkipIndex( LINE_READER* aReader )
{
    // Some broken INDEX sections have more than one section, due to prior bugs.
    // So we must read the next line after $EndINDEX tag,
    // to see if this is not a new $INDEX tag.
    bool    exit = false;
    char*   line = aReader->Line();

    do
    {
        if( TESTLINE( "$INDEX" ) )
        {
            exit = false;

            while( ( line = aReader->ReadLine() ) != NULL )
            {
                if( TESTLINE( "$EndINDEX" ) )
                {
                    exit = true;
                    break;
                }
            }
        }
        else if( exit )
            break;
    } while( ( line = aReader->ReadLine() ) != NULL );
}


void LP_CACHE::LoadModules( LINE_READER* aReader )
{
    m_owner->SetReader( aReader );

    char*   line = aReader->Line();

    do
    {
        // test first for the $MODULE, even before reading because of INDEX bug.
        if( TESTLINE( "$MODULE" ) )
        {
            auto_ptr<MODULE>    module( new MODULE( m_owner->m_board ) );

            std::string         footprintName = StrPurge( line + SZ( "$MODULE" ) );

            // The footprint names in legacy libraries can contain the '/' and ':'
            // characters which will cause the FPID parser to choke.
            ReplaceIllegalFileNameChars( &footprintName );

            // set the footprint name first thing, so exceptions can use name.
            module->SetFPID( FPID( footprintName ) );

#if 0 && defined( DEBUG )
            printf( "%s\n", footprintName.c_str() );
            if( footprintName == "QFN40" )
            {
                int breakhere = 1;
                (void) breakhere;
            }
#endif

            m_owner->loadMODULE( module.get() );

            MODULE* m = module.release();   // exceptions after this are not expected.

            // Not sure why this is asserting on debug builds.  The debugger shows the
            // strings are the same.  If it's not really needed maybe it can be removed.
//            wxASSERT( footprintName == m->GetFPID().GetFootprintName() );

            /*

            There was a bug in old legacy library management code
            (pre-LEGACY_PLUGIN) which was introducing duplicate footprint names
            in legacy libraries without notification. To best recover from such
            bad libraries, and use them to their fullest, there are a few
            strategies that could be used. (Note: footprints must have unique
            names to be accepted into this cache.) The strategy used here is to
            append a differentiating version counter to the end of the name as:
            _v2, _v3, etc.

            */

            MODULE_CITER it = m_modules.find( footprintName );

            if( it == m_modules.end() )  // footprintName is not present in cache yet.
            {
                std::pair<MODULE_ITER, bool> r = m_modules.insert( footprintName, m );

                wxASSERT_MSG( r.second, wxT( "error doing cache insert using guaranteed unique name" ) );
                (void) r;
            }

            // Bad library has a duplicate of this footprintName, generate a
            // unique footprint name and load it anyway.
            else
            {
                bool    nameOK = false;
                int     version = 2;
                char    buf[48];

                while( !nameOK )
                {
                    std::string newName = footprintName;

                    newName += "_v";
                    sprintf( buf, "%d", version++ );
                    newName += buf;

                    it = m_modules.find( newName );

                    if( it == m_modules.end() )
                    {
                        nameOK = true;

                        m->SetFPID( FPID( newName ) );
                        std::pair<MODULE_ITER, bool> r = m_modules.insert( newName, m );

                        wxASSERT_MSG( r.second, wxT( "error doing cache insert using guaranteed unique name" ) );
                        (void) r;
                    }
                }
            }
        }

    } while( ( line = aReader->ReadLine() ) != NULL );
}


#if 0
void LP_CACHE::Save()
{
    if( !m_writable )
    {
        THROW_IO_ERROR( wxString::Format(
            _( "Legacy library file '%s' is read only" ), m_lib_path.GetData() ) );
    }

    wxString tempFileName;

    // a block {} scope to fire wxFFile wxf()'s destructor
    {
        // CreateTempFileName works better with an absolute path
        wxFileName abs_lib_name( m_lib_path );

        abs_lib_name.MakeAbsolute();
        tempFileName = wxFileName::CreateTempFileName( abs_lib_name.GetFullPath() );

        //wxLogDebug( wxT( "tempFileName:'%s'  m_lib_path:'%s'\n" ), TO_UTF8( tempFileName ), TO_UTF8( m_lib_path ) );

        FILE* fp = wxFopen( tempFileName, wxT( "w" ) );
        if( !fp )
        {
            THROW_IO_ERROR( wxString::Format(
                _( "Unable to open or create legacy library file '%s'" ),
                m_lib_path.GetData() ) );
        }

        // wxf now owns fp, will close on exception or exit from
        // this block {} scope
        wxFFile wxf( fp );

        SaveHeader( fp );
        SaveIndex( fp );
        SaveModules( fp );
        SaveEndOfFile( fp );
    }

    // fp is now closed here, and that seems proper before trying to rename
    // the temporary file to m_lib_path.

    wxRemove( m_lib_path );     // it is not an error if this does not exist

    // Even on linux you can see an _intermittent_ error when calling wxRename(),
    // and it is fully inexplicable.  See if this dodges the error.
    wxMilliSleep( 250L );

    if( !wxRenameFile( tempFileName, m_lib_path ) )
    {
        THROW_IO_ERROR( wxString::Format(
            _( "Unable to rename tempfile '%s' to library file '%s'" ),
            tempFileName.GetData(),
            m_lib_path.GetData() ) );
    }
}


void LP_CACHE::SaveHeader( FILE* aFile )
{
    fprintf( aFile, "%s  %s\n", FOOTPRINT_LIBRARY_HEADER, TO_UTF8( DateAndTime() ) );
    fprintf( aFile, "# encoding utf-8\n" );
    fprintf( aFile, "Units mm\n" );
}


void LP_CACHE::SaveIndex( FILE* aFile )
{
    fprintf( aFile, "$INDEX\n" );

    for( MODULE_CITER it = m_modules.begin();  it != m_modules.end();  ++it )
    {
        fprintf( aFile, "%s\n", it->first.c_str() );
    }

    fprintf( aFile, "$EndINDEX\n" );
}


void LP_CACHE::SaveModules( FILE* aFile )
{
    m_owner->SetFilePtr( aFile );

    for( MODULE_CITER it = m_modules.begin();  it != m_modules.end();  ++it )
    {
        m_owner->saveMODULE( it->second );
    }
}
#endif

void LEGACY_PLUGIN::cacheLib( const wxString& aLibraryPath )
{
    if( !m_cache || m_cache->m_lib_path != aLibraryPath ||
        // somebody else on a network touched the library:
        m_cache->m_mod_time != m_cache->GetLibModificationTime() )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new LP_CACHE( this, aLibraryPath );
        m_cache->Load();
    }
}


wxArrayString LEGACY_PLUGIN::FootprintEnumerate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    cacheLib( aLibraryPath );

    const MODULE_MAP&   mods = m_cache->m_modules;

    wxArrayString   ret;

    for( MODULE_CITER it = mods.begin();  it != mods.end();  ++it )
    {
        ret.Add( FROM_UTF8( it->first.c_str() ) );
    }

    return ret;
}


MODULE* LEGACY_PLUGIN::FootprintLoad( const wxString& aLibraryPath,
        const wxString& aFootprintName, const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    cacheLib( aLibraryPath );

    const MODULE_MAP&   mods = m_cache->m_modules;

    MODULE_CITER it = mods.find( TO_UTF8( aFootprintName ) );

    if( it == mods.end() )
    {
        /*
        THROW_IO_ERROR( wxString::Format( _( "No '%s' footprint in library '%s'" ),
            aFootprintName.GetData(), aLibraryPath.GetData() ) );
        */

        return NULL;
    }

    // copy constructor to clone the already loaded MODULE
    return new MODULE( *it->second );
}


#if 0   // omit FootprintSave()

void LEGACY_PLUGIN::FootprintSave( const wxString& aLibraryPath,
        const MODULE* aFootprint, const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    cacheLib( aLibraryPath );

    if( !m_cache->m_writable )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library '%s' is read only" ), aLibraryPath.GetData() ) );
    }

    std::string footprintName = aFootprint->GetFPID().GetFootprintName();

    MODULE_MAP&  mods = m_cache->m_modules;

    // quietly overwrite any by same name.
    MODULE_CITER it = mods.find( footprintName );
    if( it != mods.end() )
    {
        mods.erase( footprintName );
    }

    // I need my own copy for the cache
    MODULE* my_module = new MODULE( *aFootprint );

    // and it's time stamp must be 0, it should have no parent, orientation should
    // be zero, and it should be on the front layer.

    my_module->SetTimeStamp( 0 );
    my_module->SetParent( 0 );

    my_module->SetOrientation( 0 );

    if( my_module->GetLayer() != F_Cu )
        my_module->Flip( my_module->GetPosition() );

    mods.insert( footprintName, my_module );

    m_cache->Save();
}


void LEGACY_PLUGIN::FootprintDelete( const wxString& aLibraryPath,
        const wxString& aFootprintName, const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( NULL );

    cacheLib( aLibraryPath );

    if( !m_cache->m_writable )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library '%s' is read only" ), aLibraryPath.GetData() ) );
    }

    std::string footprintName = TO_UTF8( aFootprintName );

    size_t erasedCount = m_cache->m_modules.erase( footprintName );

    if( erasedCount != 1 )
    {
        THROW_IO_ERROR( wxString::Format(
            _( "library '%s' has no footprint '%s' to delete" ),
            aLibraryPath.GetData(), aFootprintName.GetData() ) );
    }

    m_cache->Save();
}


void LEGACY_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    if( wxFileExists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format(
            _( "library '%s' already exists, will not create a new" ),
            aLibraryPath.GetData() ) );
    }

    LOCALE_IO   toggle;

    init( NULL );

    delete m_cache;
    m_cache = new LP_CACHE( this, aLibraryPath );
    m_cache->Save();
    m_cache->Load();    // update m_writable and m_mod_time
}

#endif  // omit FootprintSave()


bool LEGACY_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    wxFileName fn = aLibraryPath;

    if( !fn.FileExists() )
        return false;

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( wxRemove( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format(
            _( "library '%s' cannot be deleted" ),
            aLibraryPath.GetData() ) );
    }

    if( m_cache && m_cache->m_lib_path == aLibraryPath )
    {
        delete m_cache;
        m_cache = 0;
    }

    return true;
}


bool LEGACY_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
#if 0   // no support for 32 Cu layers in legacy format
    return false;
#else
    LOCALE_IO   toggle;

    init( NULL );

    cacheLib( aLibraryPath );

    return m_cache->m_writable;
#endif
}


LEGACY_PLUGIN::LEGACY_PLUGIN() :
    m_cu_count( 16 ),               // for FootprintLoad()
    m_board( 0 ),
    m_props( 0 ),
    m_reader( 0 ),
    m_fp( 0 ),
    m_cache( 0 ),
    m_mapping( new NETINFO_MAPPING() )
{
    init( NULL );
}


LEGACY_PLUGIN::~LEGACY_PLUGIN()
{
    delete m_cache;
    delete m_mapping;
}
