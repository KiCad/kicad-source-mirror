/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <cstdio>
#include <cstring>
#include <fast_float/fast_float.h>
#include <pcb_io/kicad_legacy/pcb_io_kicad_legacy.h>   // implement this here
#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <boost/ptr_container/ptr_map.hpp>

#include <string_utils.h>
#include <macros.h>
#include <filter_reader.h>
#include <zones.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <core/ignore.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_text.h>
#include <zone.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <pcb_target.h>
#include <pcb_plot_params.h>
#include <pcb_plot_params_parser.h>
#include <trigo.h>
#include <confirm.h>
#include <math/util.h>      // for KiROUND
#include <progress_reporter.h>

typedef PCB_IO_KICAD_LEGACY::BIU      BIU;


typedef uint32_t                LEG_MASK;

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

#define PCB_LEGACY_TEXT_is_REFERENCE 0
#define PCB_LEGACY_TEXT_is_VALUE     1
#define PCB_LEGACY_TEXT_is_DIVERS    2 // French for "other"

// Old internal units definition (UI = decimil)
#define PCB_LEGACY_INTERNAL_UNIT 10000

/// Get the length of a string constant, at compile time
#define SZ( x )         (sizeof(x)-1)


static const char delims[] = " \t\r\n";


static bool inline isSpace( int c ) { return strchr( delims, c ) != nullptr; }

#define MASK(x)             (1<<(x))


void PCB_IO_KICAD_LEGACY::checkpoint()
{
    const unsigned PROGRESS_DELTA = 250;

    if( m_progressReporter )
    {
        unsigned curLine = m_reader->LineNumber();

        if( curLine > m_lastProgressLine + PROGRESS_DELTA )
        {
            m_progressReporter->SetCurrentProgress( ( (double) curLine )
                                                            / std::max( 1U, m_lineCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( _( "Open canceled by user." ) );

            m_lastProgressLine = curLine;
        }
    }
}


//-----<BOARD Load Functions>---------------------------------------------------

/// C string compare test for a specific length of characters.
#define TESTLINE( x )   ( !strncasecmp( line, x, SZ( x ) ) && isSpace( line[SZ( x )] ) )

/// C sub-string compare test for a specific length of characters.
#define TESTSUBSTR( x ) ( !strncasecmp( line, x, SZ( x ) ) )


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


static GR_TEXT_H_ALIGN_T horizJustify( const char* horizontal )
{
    if( !strcmp( "L", horizontal ) )
        return GR_TEXT_H_ALIGN_LEFT;

    if( !strcmp( "R", horizontal ) )
        return GR_TEXT_H_ALIGN_RIGHT;

    return GR_TEXT_H_ALIGN_CENTER;
}

static GR_TEXT_V_ALIGN_T vertJustify( const char* vertical )
{
    if( !strcmp( "T", vertical ) )
        return GR_TEXT_V_ALIGN_TOP;

    if( !strcmp( "B", vertical ) )
        return GR_TEXT_V_ALIGN_BOTTOM;

    return GR_TEXT_V_ALIGN_CENTER;
}


/// Count the number of set layers in the mask
int layerMaskCountSet( LEG_MASK aMask )
{
    int count = 0;

    while( aMask )
    {
        if( aMask & 1 )
            ++count;

        aMask >>= 1;
    }

    return count;
}


// return true if aLegacyLayerNum is a valid copper layer legacy id, therefore
// top, bottom or inner activated layer
inline bool is_leg_copperlayer_valid( int aCu_Count, int aLegacyLayerNum )
{
    return aLegacyLayerNum == LAYER_N_FRONT || aLegacyLayerNum < aCu_Count;
}


PCB_LAYER_ID PCB_IO_KICAD_LEGACY::leg_layer2new( int cu_count, int aLayerNum )
{
    int         newid;
    unsigned    old = aLayerNum;

    // this is a speed critical function, be careful.

    if( unsigned( old ) <= unsigned( LAYER_N_FRONT ) )
    {
        // In .brd files, the layers are numbered from back to front
        // (the opposite of the .kicad_pcb files)
        if( old == LAYER_N_FRONT )
        {
            newid = F_Cu;
        }
        else if( old == LAYER_N_BACK )
        {
            newid = B_Cu;
        }
        else
        {
            newid = BoardLayerFromLegacyId( cu_count - 1 - old );
            wxASSERT( newid >= 0 );

            // This is of course incorrect, but at least it avoid crashing pcbnew:
            if( newid < 0 )
                newid = 0;
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
            // Remap all illegal non copper layers to comment layer
            newid = Cmts_User;
        }
    }

    return PCB_LAYER_ID( newid );
}


LSET PCB_IO_KICAD_LEGACY::leg_mask2new( int cu_count, unsigned aMask )
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
 * Parse an ASCII integer string with possible leading whitespace into
 * an integer and updates the pointer at \a out if it is not NULL, just
 * like "man strtol()".  I can use this without casting, and its name says
 * what I am doing.
 */
static inline int intParse( const char* next, const char** out = nullptr )
{
    // please just compile this and be quiet, hide casting ugliness:
    return (int) strtol( next, (char**) out, 10 );
}


/**
 * Parse an ASCII hex integer string with possible leading whitespace into
 * a long integer and updates the pointer at \a out if it is not nullptr, just
 * like "man strtol".  I can use this without casting, and its name says
 * what I am doing.
 */
static inline uint32_t hexParse( const char* next, const char** out = nullptr )
{
    return (uint32_t) strtoul( next, (char**) out, 16 );
}


bool PCB_IO_KICAD_LEGACY::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    try
    {
        FILE_LINE_READER tempReader( aFileName );
        getVersion( &tempReader );
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    return true;
}


bool PCB_IO_KICAD_LEGACY::CanReadFootprint( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadFootprint( aFileName ) )
        return false;

    try
    {
        FILE_LINE_READER         freader( aFileName );
        WHITESPACE_FILTER_READER reader( freader );

        reader.ReadLine();
        char* line = reader.Line();

        if( !line )
            return false;

        if( !strncasecmp( line, FOOTPRINT_LIBRARY_HEADER, FOOTPRINT_LIBRARY_HEADER_CNT ) )
        {
            while( reader.ReadLine() )
            {
                if( !strncasecmp( line, "$MODULE", strlen( "$MODULE" ) ) )
                {
                    return true;
                }
            }
        }
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    return false;
}


BOARD* PCB_IO_KICAD_LEGACY::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                 const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    init( aProperties );

    std::unique_ptr<BOARD> boardDeleter;

    if( aAppendToMe )
    {
        m_board = aAppendToMe;
    }
    else
    {
        boardDeleter = std::make_unique<BOARD>();
        m_board = boardDeleter.get();
    }

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    FILE_LINE_READER    reader( aFileName );

    m_reader = &reader;

    m_loading_format_version = getVersion( m_reader );
    m_board->SetFileFormatVersionAtLoad( m_loading_format_version );

    if( m_progressReporter )
    {
        m_lineCount = 0;

        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "Open canceled by user." ) );

        while( reader.ReadLine() )
            m_lineCount++;

        reader.Rewind();
    }

    loadAllSections( bool( aAppendToMe ) );

    ignore_unused( boardDeleter.release() ); // give it up so we dont delete it on exit
    m_progressReporter = nullptr;
    return m_board;
}


void PCB_IO_KICAD_LEGACY::loadAllSections( bool doAppend )
{
    // $GENERAL section is first

    // $SHEETDESCR section is next

    // $SETUP section is next

    // Then follows $EQUIPOT and all the rest
    char* line;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        checkpoint();

        // put the more frequent ones at the top, but realize TRACKs are loaded as a group

        if( TESTLINE( "$MODULE" ) )
        {
            std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( m_board );

            LIB_ID      fpid;
            std::string fpName = StrPurge( line + SZ( "$MODULE" ) );

            // The footprint names in legacy libraries can contain the '/' and ':'
            // characters which will cause the FPID parser to choke.
            ReplaceIllegalFileNameChars( fpName );

            if( !fpName.empty() )
                fpid.Parse( fpName, true );

            footprint->SetFPID( fpid );

            loadFOOTPRINT( footprint.get());
            m_board->Add( footprint.release(), ADD_MODE::APPEND );
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
            // No longer supported; discard segment fills
            loadTrackList( NOT_USED );
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
                while( ( line = READLINE( m_reader ) ) != nullptr )
                {
                    // gobble until $EndSetup
                    if( TESTLINE( "$EndSETUP" ) )
                        break;
                }
            }
        }
        else if( TESTLINE( "$EndBOARD" ) )
        {
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndBOARD'" ) );
}


int PCB_IO_KICAD_LEGACY::getVersion( LINE_READER* aReader )
{
    // Read first line and TEST if it is a PCB file format header like this:
    // "PCBNEW-BOARD Version 1 ...."

    aReader->ReadLine();

    char* line = aReader->Line();

    if( !TESTLINE( "PCBNEW-BOARD" ) )
    {
        THROW_IO_ERROR( wxT( "Unknown file type" ) );
    }

    int ver = 1; // if sccanf fails
    sscanf( line, "PCBNEW-BOARD Version %d", &ver );

    // Some legacy files have a version number = 7, similar to version 2
    // So use in this case ver = 2
    if( ver == 7 )
        ver = 2;

#if !defined( DEBUG )
    if( ver > LEGACY_BOARD_FILE_VERSION )
    {
        THROW_IO_ERROR( wxString::Format( _( "File '%s' has an unrecognized version: %d." ),
                                          aReader->GetSource().GetData(), ver ) );
    }
#endif

    return ver;
}


void PCB_IO_KICAD_LEGACY::loadGENERAL()
{
    char*   line;
    char*   saveptr;
    bool    saw_LayerCount = false;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        if( TESTLINE( "Units" ) )
        {
            // what are the engineering units of the lengths in the BOARD?
            data = strtok_r( line + SZ("Units"), delims, &saveptr );

            if( !strcmp( data, "mm" ) )
            {
                diskToBiu = pcbIUScale.IU_PER_MM;
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
                THROW_IO_ERROR( wxT( "Missing '$GENERAL's LayerCount" ) );

            LEG_MASK enabledLayers = hexParse( line + SZ( "EnabledLayers" ) );
            LSET new_mask = leg_mask2new( m_cu_count, enabledLayers );

            m_board->SetEnabledLayers( new_mask );

            // layer visibility equals layer usage, unless overridden later via "VisibleLayers"
            // Must call SetEnabledLayers() before calling SetVisibleLayers().
            m_board->SetVisibleLayers( new_mask );

            // Ensure copper layers count is not modified:
            m_board->SetCopperLayerCount( m_cu_count );
        }
        else if( TESTLINE( "VisibleLayers" ) )
        {
        // Keep all enabled layers visible.
        // the old visibility control does not make sense in current Pcbnew version
        // However, this code works.
        #if 0
            if( !saw_LayerCount )
                THROW_IO_ERROR( wxT( "Missing '$GENERAL's LayerCount" ) );

            LEG_MASK visibleLayers = hexParse( line + SZ( "VisibleLayers" ) );

            LSET new_mask = leg_mask2new( m_cu_count, visibleLayers );

            m_board->SetVisibleLayers( new_mask );
        #endif
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
        else if( TESTLINE( "NoConn" ) )
        {
            // ignore
            intParse( line + SZ( "NoConn" ) );
        }
        else if( TESTLINE( "Di" ) )
        {
            biuParse( line + SZ( "Di" ), &data );
            biuParse( data, &data );
            biuParse( data, &data );
            biuParse( data );
        }
        else if( TESTLINE( "Nnets" ) )
        {
            m_netCodes.resize( intParse( line + SZ( "Nnets" ) ) );
        }
        else if( TESTLINE( "Nn" ) )     // id "Nnets" for old .brd files
        {
            m_netCodes.resize( intParse( line + SZ( "Nn" ) ) );
        }
        else if( TESTLINE( "$EndGENERAL" ) )
        {
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndGENERAL'" ) );
}


void PCB_IO_KICAD_LEGACY::loadSHEET()
{
    char        buf[260];
    TITLE_BLOCK tb;
    char*       line;
    char*       data;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        if( TESTLINE( "Sheet" ) )
        {
            // e.g. "Sheet A3 16535 11700"
            // width and height are in 1/1000th of an inch, always
            PAGE_INFO   page;
            char*       sname  = strtok_r( line + SZ( "Sheet" ), delims, &data );

            if( sname )
            {
                wxString wname = From_UTF8( sname );

                if( !page.SetType( wname ) )
                {
                    m_error.Printf( _( "Unknown sheet type '%s' on line: %d." ),
                                    wname.GetData(),
                                    (int) m_reader->LineNumber() );
                    THROW_IO_ERROR( m_error );
                }

                char*   width  = strtok_r( nullptr, delims, &data );
                char*   height = strtok_r( nullptr, delims, &data );
                char*   orient = strtok_r( nullptr, delims, &data );

                // only parse the width and height if page size is custom ("User")
                if( page.GetType() == PAGE_SIZE_TYPE::User )
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
            tb.SetTitle( From_UTF8( buf ) );
        }
        else if( TESTLINE( "Date" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetDate( From_UTF8( buf ) );
        }
        else if( TESTLINE( "Rev" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetRevision( From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comp" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetCompany( From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment1" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 0, From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment2" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 1, From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment3" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 2, From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment4" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 3, From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment5" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 4, From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment6" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 5, From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment7" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 6, From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment8" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 7, From_UTF8( buf ) );
        }
        else if( TESTLINE( "Comment9" ) )
        {
            ReadDelimitedText( buf, line, sizeof(buf) );
            tb.SetComment( 8, From_UTF8( buf ) );
        }
        else if( TESTLINE( "$EndSHEETDESCR" ) )
        {
            m_board->SetTitleBlock( tb );
            return;             // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndSHEETDESCR'" ) );
}


void PCB_IO_KICAD_LEGACY::loadSETUP()
{
    BOARD_DESIGN_SETTINGS&    bds             = m_board->GetDesignSettings();
    ZONE_SETTINGS             zoneSettings    = bds.GetDefaultZoneSettings();
    std::shared_ptr<NETCLASS> defaultNetclass = bds.m_NetSettings->GetDefaultNetclass();
    char*                     line;
    char*                     saveptr;

    m_board->m_LegacyDesignSettingsLoaded = true;
    m_board->m_LegacyNetclassesLoaded = true;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        if( TESTLINE( "PcbPlotParams" ) )
        {
            PCB_PLOT_PARAMS plot_opts;

            PCB_PLOT_PARAMS_PARSER parser( line + SZ( "PcbPlotParams" ), m_reader->GetSource() );

            plot_opts.Parse( &parser );

            m_board->SetPlotOptions( plot_opts );

            if( plot_opts.GetLegacyPlotViaOnMaskLayer().has_value() )
            {
                bool tent = *plot_opts.GetLegacyPlotViaOnMaskLayer();
                m_board->GetDesignSettings().m_TentViasFront = tent;
                m_board->GetDesignSettings().m_TentViasBack = tent;
            }
        }

        else if( TESTLINE( "AuxiliaryAxisOrg" ) )
        {
            BIU gx = biuParse( line + SZ( "AuxiliaryAxisOrg" ), &data );
            BIU gy = biuParse( data );

            bds.SetAuxOrigin( VECTOR2I( gx, gy ) );
        }
        else if( TESTSUBSTR( "Layer[" ) )
        {
            // eg: "Layer[n]  <a_Layer_name_with_no_spaces> <LAYER_T>"

            int          layer_num = intParse( line + SZ( "Layer[" ), &data );
            PCB_LAYER_ID layer_id  = leg_layer2new( m_cu_count, layer_num );

            data = strtok_r( (char*) data+1, delims, &saveptr );    // +1 for ']'

            if( data )
            {
                wxString layerName = From_UTF8( data );
                m_board->SetLayerName( layer_id, layerName );

                data = strtok_r( nullptr, delims, &saveptr );

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
            defaultNetclass->SetTrackWidth( tmp );
        }
        else if( TESTLINE( "TrackWidthList" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackWidthList" ) );
            bds.m_TrackWidthList.push_back( tmp );
        }
        else if( TESTLINE( "TrackClearence" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackClearence" ) );
            defaultNetclass->SetClearance( tmp );
        }
        else if( TESTLINE( "TrackMinWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TrackMinWidth" ) );
            bds.m_TrackMinWidth = tmp;
        }
        else if( TESTLINE( "ZoneClearence" ) )
        {
            BIU tmp = biuParse( line + SZ( "ZoneClearence" ) );
            zoneSettings.m_ZoneClearance = tmp;
        }
        else if( TESTLINE( "Zone_45_Only" ) )   // No longer used
        {
            /* bool tmp = (bool) */ intParse( line + SZ( "Zone_45_Only" ) );
        }
        else if( TESTLINE( "DrawSegmWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "DrawSegmWidth" ) );
            bds.m_LineThickness[ LAYER_CLASS_COPPER ] = tmp;
        }
        else if( TESTLINE( "EdgeSegmWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "EdgeSegmWidth" ) );
            bds.m_LineThickness[ LAYER_CLASS_EDGES ] = tmp;
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

            data = strtok_r( (char*) data, delims, (char**) &data );
            if( data )  // DRILL may not be present ?
                drill = biuParse( data );

            bds.m_ViasDimensionsList.emplace_back( diameter, drill );
        }
        else if( TESTLINE( "ViaSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaSize" ) );
            defaultNetclass->SetViaDiameter( tmp );
        }
        else if( TESTLINE( "ViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaDrill" ) );
            defaultNetclass->SetViaDrill( tmp );
        }
        else if( TESTLINE( "ViaMinDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "ViaMinDrill" ) );
            bds.m_MinThroughDrill = tmp;
        }
        else if( TESTLINE( "MicroViaSize" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaSize" ) );
            defaultNetclass->SetuViaDiameter( tmp );
        }
        else if( TESTLINE( "MicroViaDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaDrill" ) );
            defaultNetclass->SetuViaDrill( tmp );
        }
        else if( TESTLINE( "MicroViaMinDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "MicroViaMinDrill" ) );
            bds.m_MicroViasMinDrill = tmp;
        }
        else if( TESTLINE( "MicroViasAllowed" ) )
        {
            intParse( line + SZ( "MicroViasAllowed" ) );
        }
        else if( TESTLINE( "TextPcbWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TextPcbWidth" ) );
            bds.m_TextThickness[ LAYER_CLASS_COPPER ] = tmp;
        }
        else if( TESTLINE( "TextPcbSize" ) )
        {
            BIU x = biuParse( line + SZ( "TextPcbSize" ), &data );
            BIU y = biuParse( data );

            bds.m_TextSize[ LAYER_CLASS_COPPER ] = VECTOR2I( x, y );
        }
        else if( TESTLINE( "EdgeModWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "EdgeModWidth" ) );
            bds.m_LineThickness[ LAYER_CLASS_SILK ] = tmp;
            bds.m_LineThickness[ LAYER_CLASS_OTHERS ] = tmp;
        }
        else if( TESTLINE( "TextModWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( "TextModWidth" ) );
            bds.m_TextThickness[ LAYER_CLASS_SILK ] = tmp;
            bds.m_TextThickness[ LAYER_CLASS_OTHERS ] = tmp;
        }
        else if( TESTLINE( "TextModSize" ) )
        {
            BIU x = biuParse( line + SZ( "TextModSize" ), &data );
            BIU y = biuParse( data );

            bds.m_TextSize[LAYER_CLASS_SILK] = VECTOR2I( x, y );
            bds.m_TextSize[LAYER_CLASS_OTHERS] = VECTOR2I( x, y );
        }
        else if( TESTLINE( "PadSize" ) )
        {
            BIU x = biuParse( line + SZ( "PadSize" ), &data );
            BIU y = biuParse( data );

            bds.m_Pad_Master->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( x, y ) );
        }
        else if( TESTLINE( "PadDrill" ) )
        {
            BIU tmp = biuParse( line + SZ( "PadDrill" ) );
            bds.m_Pad_Master->SetDrillSize( VECTOR2I( tmp, tmp ) );
        }
        else if( TESTLINE( "Pad2MaskClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( "Pad2MaskClearance" ) );
            bds.m_SolderMaskExpansion = tmp;
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

            bds.SetGridOrigin( VECTOR2I( x, y ) );
        }
        else if( TESTLINE( "VisibleElements" ) )
        {
            // Keep all elements visible.
            // the old visibility control does not make sense in current Pcbnew version,
            // and this code does not work.
#if 0
            uint32_t visibleElements = hexParse( line + SZ( "VisibleElements" ) );

            // Does not work: each old item should be tested one by one to set
            // visibility of new item list
            GAL_SET visibles;

            for( size_t i = 0; i < visibles.size(); i++ )
                visibles.set( i, visibleElements & ( 1u << i ) );

            m_board->SetVisibleElements( visibles );
#endif
        }
        else if( TESTLINE( "$EndSETUP" ) )
        {
            bds.SetDefaultZoneSettings( zoneSettings );

            // Very old *.brd file does not have  NETCLASSes
            // "TrackWidth", "ViaSize", "ViaDrill", "ViaMinSize", and "TrackClearence" were
            // defined in SETUP; these values are put into the default NETCLASS until later board
            // load code should override them.  *.brd files which have been saved with knowledge
            // of NETCLASSes will override these defaults, very old boards (before 2009) will not
            // and use the setup values.
            // However these values should be the same as default NETCLASS.

            return;     // preferred exit
        }
    }

    /*
     * Ensure tracks and vias sizes lists are ok:
     * Sort lists by by increasing value and remove duplicates
     * (the first value is not tested, because it is the netclass value)
     */
    BOARD_DESIGN_SETTINGS& designSettings = m_board->GetDesignSettings();
    sort( designSettings.m_ViasDimensionsList.begin() + 1, designSettings.m_ViasDimensionsList.end() );
    sort( designSettings.m_TrackWidthList.begin() + 1, designSettings.m_TrackWidthList.end() );

    for( int ii = 1; ii < (int) designSettings.m_ViasDimensionsList.size() - 1; ii++ )
    {
        if( designSettings.m_ViasDimensionsList[ii] == designSettings.m_ViasDimensionsList[ii + 1] )
        {
            designSettings.m_ViasDimensionsList.erase( designSettings.m_ViasDimensionsList.begin() + ii );
            ii--;
        }
    }

    for( int ii = 1; ii < (int) designSettings.m_TrackWidthList.size() - 1; ii++ )
    {
        if( designSettings.m_TrackWidthList[ii] == designSettings.m_TrackWidthList[ii + 1] )
        {
            designSettings.m_TrackWidthList.erase( designSettings.m_TrackWidthList.begin() + ii );
            ii--;
        }
    }
}


void PCB_IO_KICAD_LEGACY::loadFOOTPRINT( FOOTPRINT* aFootprint )
{
    char*   line;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        // most frequently encountered ones at the top

        if( TESTSUBSTR( "D" ) && strchr( "SCAP", line[1] ) )  // read a drawing item, e.g. "DS"
        {
            loadFP_SHAPE( aFootprint );
        }
        else if( TESTLINE( "$PAD" ) )
        {
            loadPAD( aFootprint );
        }
        else if( TESTSUBSTR( "T" ) )  // Read a footprint text description (ref, value, or drawing)
        {
            // e.g. "T1 6940 -16220 350 300 900 60 M I 20 N "CFCARD"\r\n"
            int tnum = intParse( line + SZ( "T" ) );

            PCB_TEXT* text = nullptr;

            switch( tnum )
            {
            case PCB_LEGACY_TEXT_is_REFERENCE:
                text = &aFootprint->Reference();
                break;

            case PCB_LEGACY_TEXT_is_VALUE:
                text = &aFootprint->Value();
                break;

            // All other fields greater than 1.
            default:
                text = new PCB_TEXT( aFootprint );
                aFootprint->Add( text );
            }

            loadMODULE_TEXT( text );

            // Convert hidden footprint text (which is no longer supported) to a hidden field
            if( !text->IsVisible() && text->Type() == PCB_TEXT_T )
            {
                aFootprint->Remove( text );
                aFootprint->Add( new PCB_FIELD( *text, FIELD_T::USER ) );
                delete text;
            }
        }
        else if( TESTLINE( "Po" ) )
        {
            // e.g. "Po 19120 39260 900 0 4E823D06 68183921-93a5-49ac-91b0-49d05a0e1647 ~~\r\n"
            BIU          pos_x     = biuParse( line + SZ( "Po" ), &data );
            BIU          pos_y     = biuParse( data, &data );
            int          orient    = intParse( data, &data );
            int          layer_num = intParse( data, &data );
            PCB_LAYER_ID layer_id  = leg_layer2new( m_cu_count,  layer_num );

            [[maybe_unused]] uint32_t edittime = hexParse( data, &data );

            char*        uuid      = strtok_r( (char*) data, delims, (char**) &data );

            data = strtok_r( (char*) data+1, delims, (char**) &data );

            // data is now a two character long string
            // Note: some old files do not have this field
            if( data && data[0] == 'F' )
                aFootprint->SetLocked( true );

            if( data && data[1] == 'P' )
                aFootprint->SetIsPlaced( true );

            aFootprint->SetPosition( VECTOR2I( pos_x, pos_y ) );
            aFootprint->SetLayer( layer_id );
            aFootprint->SetOrientation( EDA_ANGLE( orient, TENTHS_OF_A_DEGREE_T ) );
            const_cast<KIID&>( aFootprint->m_Uuid ) = KIID( uuid );
        }
        else if( TESTLINE( "Sc" ) )         // timestamp
        {
            char* uuid = strtok_r( (char*) line + SZ( "Sc" ), delims, (char**) &data );
            const_cast<KIID&>( aFootprint->m_Uuid ) = KIID( uuid );
        }
        else if( TESTLINE( "Op" ) )         // (Op)tions for auto placement (no longer supported)
        {
            hexParse( line + SZ( "Op" ), &data );
            hexParse( data );
        }
        else if( TESTLINE( "At" ) )         // (At)tributes of footprint
        {
            int attrs = 0;

            data = line + SZ( "At" );

            if( strstr( data, "SMD" ) )
                attrs |= FP_SMD;
            else if( strstr( data, "VIRTUAL" ) )
                attrs |= FP_EXCLUDE_FROM_POS_FILES | FP_EXCLUDE_FROM_BOM;
            else
                attrs |= FP_THROUGH_HOLE | FP_EXCLUDE_FROM_POS_FILES;

            aFootprint->SetAttributes( attrs );
        }
        else if( TESTLINE( "AR" ) )         // Alternate Reference
        {
            // e.g. "AR /68183921-93a5-49ac-e164-49d05a0e1647/93a549d0-49d0-e164-91b0-49d05a0e1647"
            data = strtok_r( line + SZ( "AR" ), delims, (char**) &data );

            if( data )
                aFootprint->SetPath( KIID_PATH( From_UTF8( data ) ) );
        }
        else if( TESTLINE( "$SHAPE3D" ) )
        {
            load3D( aFootprint );
        }
        else if( TESTLINE( "Cd" ) )
        {
            // e.g. "Cd Double rangee de contacts 2 x 4 pins\r\n"
            aFootprint->SetLibDescription( From_UTF8( StrPurge( line + SZ( "Cd" ) ) ) );
        }
        else if( TESTLINE( "Kw" ) )         // Key words
        {
            aFootprint->SetKeywords( From_UTF8( StrPurge( line + SZ( "Kw" ) ) ) );
        }
        else if( TESTLINE( ".SolderPasteRatio" ) )
        {
            double tmp = atof( line + SZ( ".SolderPasteRatio" ) );

            // Due to a bug in dialog editor in Footprint Editor, fixed in BZR version 3565
            // this parameter can be broken.
            // It should be >= -50% (no solder paste) and <= 0% (full area of the pad)

            if( tmp < -0.50 )
                tmp = -0.50;

            if( tmp > 0.0 )
                tmp = 0.0;

            aFootprint->SetLocalSolderPasteMarginRatio( tmp );
        }
        else if( TESTLINE( ".SolderPaste" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderPaste" ) );
            aFootprint->SetLocalSolderPasteMargin( tmp );
        }
        else if( TESTLINE( ".SolderMask" ) )
        {
            BIU tmp = biuParse( line + SZ( ".SolderMask" ) );
            aFootprint->SetLocalSolderMaskMargin( tmp );
        }
        else if( TESTLINE( ".LocalClearance" ) )
        {
            BIU tmp = biuParse( line + SZ( ".LocalClearance" ) );
            aFootprint->SetLocalClearance( tmp );
        }
        else if( TESTLINE( ".ZoneConnection" ) )
        {
            int tmp = intParse( line + SZ( ".ZoneConnection" ) );
            aFootprint->SetLocalZoneConnection((ZONE_CONNECTION) tmp );
        }
        else if( TESTLINE( ".ThermalWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( ".ThermalWidth" ) );
            ignore_unused( tmp );
        }
        else if( TESTLINE( ".ThermalGap" ) )
        {
            BIU tmp = biuParse( line + SZ( ".ThermalGap" ) );
            ignore_unused( tmp );
        }
        else if( TESTLINE( "$EndMODULE" ) )
        {
            return;     // preferred exit
        }
    }

    wxString msg = wxString::Format( _( "Missing '$EndMODULE' for MODULE '%s'." ),
                                     aFootprint->GetFPID().GetLibItemName().wx_str() );
    THROW_IO_ERROR( msg );
}


void PCB_IO_KICAD_LEGACY::loadPAD( FOOTPRINT* aFootprint )
{
    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aFootprint );
    char*                line;
    char*                saveptr;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        if( TESTLINE( "Sh" ) )              // (Sh)ape and padname
        {
            // e.g. "Sh "A2" C 520 520 0 0 900"
            // or   "Sh "1" R 157 1378 0 0 900"

            // mypadnumber is LATIN1/CRYLIC for BOARD_FORMAT_VERSION 1, but for
            // BOARD_FORMAT_VERSION 2, it is UTF8 from disk.
            // Moving forward padnumbers will be in UTF8 on disk, as are all KiCad strings on disk.
            char        mypadnumber[50];

            data = line + SZ( "Sh" ) + 1;   // +1 skips trailing whitespace

            // +1 trailing whitespace.
            data = data + ReadDelimitedText( mypadnumber, data, sizeof( mypadnumber ) ) + 1;

            while( isSpace( *data ) )
                ++data;

            unsigned char   padchar = (unsigned char) *data++;
            int             padshape;

            BIU      size_x  = biuParse( data, &data );
            BIU      size_y  = biuParse( data, &data );
            BIU      delta_x = biuParse( data, &data );
            BIU      delta_y = biuParse( data, &data );
            EDA_ANGLE orient = degParse( data );

            switch( padchar )
            {
            case 'C':   padshape = static_cast<int>( PAD_SHAPE::CIRCLE );      break;
            case 'R':   padshape = static_cast<int>( PAD_SHAPE::RECTANGLE );   break;
            case 'O':   padshape = static_cast<int>( PAD_SHAPE::OVAL );        break;
            case 'T':   padshape = static_cast<int>( PAD_SHAPE::TRAPEZOID );   break;
            default:
                m_error.Printf( _( "Unknown padshape '%c=0x%02x' on line: %d of footprint: '%s'." ),
                                padchar,
                                padchar,
                                (int) m_reader->LineNumber(),
                                aFootprint->GetFPID().GetLibItemName().wx_str() );
                THROW_IO_ERROR( m_error );
            }

            // go through a wxString to establish a universal character set properly
            wxString    padNumber;

            if( m_loading_format_version == 1 )
            {
                // add 8 bit bytes, file format 1 was KiCad font type byte,
                // simply promote those 8 bit bytes up into UNICODE. (subset of LATIN1)
                const unsigned char* cp = (unsigned char*) mypadnumber;

                while( *cp )
                    padNumber += *cp++;  // unsigned, ls 8 bits only
            }
            else
            {
                // version 2, which is UTF8.
                padNumber = From_UTF8( mypadnumber );
            }

            // chances are both were ASCII, but why take chances?

            pad->SetNumber( padNumber );
            pad->SetShape( PADSTACK::ALL_LAYERS, static_cast<PAD_SHAPE>( padshape ) );
            pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( size_x, size_y ) );
            pad->SetDelta( PADSTACK::ALL_LAYERS, VECTOR2I( delta_x, delta_y ) );
            pad->SetOrientation( orient );
        }
        else if( TESTLINE( "Dr" ) )         // (Dr)ill
        {
            // e.g. "Dr 350 0 0" or "Dr 0 0 0 O 0 0"
            BIU drill_x = biuParse( line + SZ( "Dr" ), &data );
            BIU drill_y = drill_x;
            BIU offs_x  = biuParse( data, &data );
            BIU offs_y  = biuParse( data, &data );

            PAD_DRILL_SHAPE drShape = PAD_DRILL_SHAPE::CIRCLE;

            data = strtok_r( (char*) data, delims, &saveptr );

            if( data )  // optional shape
            {
                if( data[0] == 'O' )
                {
                    drShape = PAD_DRILL_SHAPE::OBLONG;

                    data    = strtok_r( nullptr, delims, &saveptr );
                    drill_x = biuParse( data );

                    data    = strtok_r( nullptr, delims, &saveptr );
                    drill_y = biuParse( data );
                }
            }

            pad->SetDrillShape( drShape );
            pad->SetOffset( PADSTACK::ALL_LAYERS, VECTOR2I( offs_x, offs_y ) );
            pad->SetDrillSize( VECTOR2I( drill_x, drill_y ) );
        }
        else if( TESTLINE( "At" ) )         // (At)tribute
        {
            // e.g. "At SMD N 00888000"
            // sscanf( PtLine, "%s %s %X", BufLine, BufCar, &m_layerMask );

            PAD_ATTRIB  attribute;

            data = strtok_r( line + SZ( "At" ), delims, &saveptr );

            if( !strcmp( data, "SMD" ) )
                attribute = PAD_ATTRIB::SMD;
            else if( !strcmp( data, "CONN" ) )
                attribute = PAD_ATTRIB::CONN;
            else if( !strcmp( data, "HOLE" ) )
                attribute = PAD_ATTRIB::NPTH;
            else
                attribute = PAD_ATTRIB::PTH;

            strtok_r( nullptr, delims, &saveptr );  // skip unused prm
            data = strtok_r( nullptr, delims, &saveptr );

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

            if( m_board )
            {
                wxASSERT( m_board->FindNet( getNetCode( netcode ) )->GetNetname()
                          == ConvertToNewOverbarNotation( From_UTF8( StrPurge( buf ) ) ) );
            }
        }
        else if( TESTLINE( "Po" ) )         // (Po)sition
        {
            // e.g. "Po 500 -500"
            VECTOR2I pos;

            pos.x = biuParse( line + SZ( "Po" ), &data );
            pos.y = biuParse( data );

            pad->SetFPRelativePosition( pos );
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
            pad->SetLocalZoneConnection( (ZONE_CONNECTION) tmp );
        }
        else if( TESTLINE( ".ThermalWidth" ) )
        {
            BIU tmp = biuParse( line + SZ( ".ThermalWidth" ) );
            pad->SetLocalThermalSpokeWidthOverride( tmp );
        }
        else if( TESTLINE( ".ThermalGap" ) )
        {
            BIU tmp = biuParse( line + SZ( ".ThermalGap" ) );
            pad->SetLocalThermalGapOverride( tmp );
        }
        else if( TESTLINE( "$EndPAD" ) )
        {
            if( pad->GetSizeX() > 0 && pad->GetSizeY() > 0 )
            {
                aFootprint->Add( pad.release() );
            }
            else
            {
                wxLogError( _( "Invalid zero-sized pad ignored in\nfile: %s" ),
                            m_reader->GetSource() );
            }

            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndPAD'" ) );
}


void PCB_IO_KICAD_LEGACY::loadFP_SHAPE( FOOTPRINT* aFootprint )
{
    SHAPE_T shape;
    char*   line = m_reader->Line();     // obtain current (old) line

    switch( line[1] )
    {
    case 'S': shape = SHAPE_T::SEGMENT; break;
    case 'C': shape = SHAPE_T::CIRCLE;  break;
    case 'A': shape = SHAPE_T::ARC;     break;
    case 'P': shape = SHAPE_T::POLY;    break;
    default:
        m_error.Printf( _( "Unknown PCB_SHAPE type:'%c=0x%02x' on line %d of footprint '%s'." ),
                        (unsigned char) line[1],
                        (unsigned char) line[1],
                        (int) m_reader->LineNumber(),
                        aFootprint->GetFPID().GetLibItemName().wx_str() );
        THROW_IO_ERROR( m_error );
    }

    std::unique_ptr<PCB_SHAPE> dwg = std::make_unique<PCB_SHAPE>( aFootprint, shape );    // a drawing

    const char* data;

    // common to all cases, and we have to check their values uniformly at end
    BIU width = 1;
    int layer = FIRST_NON_COPPER_LAYER;

    switch( shape )
    {
    case SHAPE_T::ARC:
    {
        BIU       center0_x = biuParse( line + SZ( "DA" ), &data );
        BIU       center0_y = biuParse( data, &data );
        BIU       start0_x = biuParse( data, &data );
        BIU       start0_y = biuParse( data, &data );
        EDA_ANGLE angle = degParse( data, &data );

        width = biuParse( data, &data );
        layer = intParse( data );

        dwg->SetCenter( VECTOR2I( center0_x, center0_y ) );
        dwg->SetStart( VECTOR2I( start0_x, start0_y ) );
        dwg->SetArcAngleAndEnd( angle, true );
        break;
    }

    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    {
        // e.g. "DS -7874 -10630 7874 -10630 50 20\r\n"
        BIU start0_x = biuParse( line + SZ( "DS" ), &data );
        BIU start0_y = biuParse( data, &data );
        BIU end0_x = biuParse( data, &data );
        BIU end0_y = biuParse( data, &data );

        width = biuParse( data, &data );
        layer = intParse( data );

        dwg->SetStart( VECTOR2I( start0_x, start0_y ) );
        dwg->SetEnd( VECTOR2I( end0_x, end0_y ) );
        break;
    }

    case SHAPE_T::POLY:
    {
        // e.g. "DP %d %d %d %d %d %d %d\n"
        BIU start0_x = biuParse( line + SZ( "DP" ), &data );
        BIU start0_y = biuParse( data, &data );
        BIU end0_x = biuParse( data, &data );
        BIU end0_y = biuParse( data, &data );
        int ptCount = intParse( data, &data );

        width = biuParse( data, &data );
        layer = intParse( data );

        dwg->SetStart( VECTOR2I( start0_x, start0_y ) );
        dwg->SetEnd( VECTOR2I( end0_x, end0_y ) );

        std::vector<VECTOR2I> pts;
        pts.reserve( ptCount );

        for( int ii = 0; ii < ptCount; ++ii )
        {
            if( ( line = READLINE( m_reader ) ) == nullptr )
            {
                THROW_IO_ERROR( wxT( "S_POLGON point count mismatch." ) );
            }

            // e.g. "Dl 23 44\n"

            if( !TESTLINE( "Dl" ) )
            {
                THROW_IO_ERROR( wxT( "Missing Dl point def" ) );
            }

            BIU x = biuParse( line + SZ( "Dl" ), &data );
            BIU y = biuParse( data );

            pts.emplace_back( x, y );
        }

        dwg->SetPolyPoints( pts );
        break;
    }

    default:
        // first switch code above prevents us from getting here.
        break;
    }

    // Check for a reasonable layer:
    // layer must be >= FIRST_NON_COPPER_LAYER, but because microwave footprints can use the
    // copper layers, layer < FIRST_NON_COPPER_LAYER is allowed.
    if( layer < FIRST_LAYER || layer > LAST_NON_COPPER_LAYER )
        layer = SILKSCREEN_N_FRONT;

    dwg->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
    dwg->SetLayer( leg_layer2new( m_cu_count,  layer ) );

    dwg->Rotate( { 0, 0 }, aFootprint->GetOrientation() );
    dwg->Move( aFootprint->GetPosition() );
    aFootprint->Add( dwg.release() );
}


void PCB_IO_KICAD_LEGACY::loadMODULE_TEXT( PCB_TEXT* aText )
{
    const char* data;
    const char* txt_end;
    const char* line = m_reader->Line();     // current (old) line

    // e.g. "T1 6940 -16220 350 300 900 60 M I 20 N "CFCARD"\r\n"
    // or    T1 0 500 600 400 900 80 M V 20 N"74LS245"
    // ouch, the last example has no space between N and "74LS245" !
    // that is an older version.

    int       type    = intParse( line+1, &data );
    BIU       pos0_x  = biuParse( data, &data );
    BIU       pos0_y  = biuParse( data, &data );
    BIU       size0_y = biuParse( data, &data );
    BIU       size0_x = biuParse( data, &data );
    EDA_ANGLE orient  = degParse( data, &data );
    BIU       thickn  = biuParse( data, &data );

    // read the quoted text before the first call to strtok() which introduces
    // NULs into the string and chops it into multiple C strings, something
    // ReadDelimitedText() cannot traverse.

    // convert the "quoted, escaped, UTF8, text" to a wxString, find it by skipping
    // as far forward as needed until the first double quote.
    txt_end = data + ReadDelimitedText( &m_field, data );
    m_field.Replace( wxT( "%V" ), wxT( "${VALUE}" ) );
    m_field.Replace( wxT( "%R" ), wxT( "${REFERENCE}" ) );
    m_field = ConvertToNewOverbarNotation( m_field );
    aText->SetText( m_field );

    // after switching to strtok, there's no easy coming back because of the
    // embedded nul(s?) placed to the right of the current field.
    // (that's the reason why strtok was deprecated...)
    char*   mirror  = strtok_r( (char*) data, delims, (char**) &data );
    char*   hide    = strtok_r( nullptr, delims, (char**) &data );
    char*   tmp     = strtok_r( nullptr, delims, (char**) &data );

    int     layer_num = tmp ? intParse( tmp ) : SILKSCREEN_N_FRONT;

    char*   italic  = strtok_r( nullptr, delims, (char**) &data );

    char*   hjust   = strtok_r( (char*) txt_end, delims, (char**) &data );
    char*   vjust   = strtok_r( nullptr, delims, (char**) &data );

    if( type != PCB_LEGACY_TEXT_is_REFERENCE && type != PCB_LEGACY_TEXT_is_VALUE )
        type = PCB_LEGACY_TEXT_is_DIVERS;

    aText->SetFPRelativePosition( VECTOR2I( pos0_x, pos0_y ) );
    aText->SetTextSize( VECTOR2I( size0_x, size0_y ) );

    aText->SetTextAngle( orient );

    aText->SetTextThickness( thickn < 1 ? 0 : thickn );

    aText->SetMirrored( mirror && *mirror == 'M' );

    aText->SetVisible( !(hide && *hide == 'I') );

    aText->SetItalic( italic && *italic == 'I' );

    if( hjust )
        aText->SetHorizJustify( horizJustify( hjust ) );

    if( vjust )
        aText->SetVertJustify( vertJustify( vjust ) );

     // A protection against mal formed (or edited by hand) files:
    if( layer_num < FIRST_LAYER )
        layer_num = FIRST_LAYER;
    else if( layer_num > LAST_NON_COPPER_LAYER )
        layer_num = LAST_NON_COPPER_LAYER;
    else if( layer_num == LAYER_N_BACK )
        layer_num = SILKSCREEN_N_BACK;
    else if( layer_num == LAYER_N_FRONT )
        layer_num = SILKSCREEN_N_FRONT;
    else if( layer_num < LAYER_N_FRONT )    // this case is a internal layer
        layer_num = SILKSCREEN_N_FRONT;

    aText->SetLayer( leg_layer2new( m_cu_count, layer_num ) );
}


void PCB_IO_KICAD_LEGACY::load3D( FOOTPRINT* aFootprint )
{
    FP_3DMODEL t3D;

    // Lambda to parse three space-separated doubles using wxString::ToCDouble with C locale
    auto parseThreeDoubles =
            []( const char* str, double& x, double& y, double& z ) -> bool
            {
                wxString wxStr( str );
                wxStr.Trim( true ).Trim( false );

                wxStringTokenizer tokenizer( wxStr, " \t", wxTOKEN_STRTOK );

                if( !tokenizer.HasMoreTokens() )
                    return false;

                wxString token1 = tokenizer.GetNextToken();

                if( !token1.ToCDouble( &x ) || !tokenizer.HasMoreTokens() )
                    return false;

                wxString token2 = tokenizer.GetNextToken();

                if( !token2.ToCDouble( &y ) || !tokenizer.HasMoreTokens() )
                    return false;

                wxString token3 = tokenizer.GetNextToken();

                if( !token3.ToCDouble( &z ) )
                    return false;

                return true;
            };

    char* line;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        if( TESTLINE( "Na" ) )     // Shape File Name
        {
            char    buf[512];
            ReadDelimitedText( buf, line + SZ( "Na" ), sizeof(buf) );
            t3D.m_Filename = buf;
        }
        else if( TESTLINE( "Sc" ) )     // Scale
        {
            if (!parseThreeDoubles(line + SZ("Sc"), t3D.m_Scale.x, t3D.m_Scale.y, t3D.m_Scale.z))
            {
                THROW_IO_ERROR( wxT( "Invalid scale values in 3D model" ) );
            }
        }
        else if( TESTLINE( "Of" ) )     // Offset
        {
            if (!parseThreeDoubles(line + SZ("Of"), t3D.m_Offset.x, t3D.m_Offset.y, t3D.m_Offset.z))
            {
                THROW_IO_ERROR( wxT( "Invalid offset values in 3D model" ) );
            }
        }
        else if( TESTLINE( "Ro" ) )     // Rotation
        {
            if (!parseThreeDoubles(line + SZ("Ro"), t3D.m_Rotation.x, t3D.m_Rotation.y, t3D.m_Rotation.z))
            {
                THROW_IO_ERROR( wxT( "Invalid rotation values in 3D model" ) );
            }
        }
        else if( TESTLINE( "$EndSHAPE3D" ) )
        {
            aFootprint->Models().push_back( t3D );
            return;         // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndSHAPE3D'" ) );
}


void PCB_IO_KICAD_LEGACY::loadPCB_LINE()
{
    /* example:
        $DRAWSEGMENT
        Po 0 57500 -1000 57500 0 150
        De 24 0 900 0 0
        $EndDRAWSEGMENT
    */

    std::unique_ptr<PCB_SHAPE> dseg = std::make_unique<PCB_SHAPE>( m_board );

    char*   line;
    char*   saveptr;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        if( TESTLINE( "Po" ) )
        {
            int shape   = intParse( line + SZ( "Po" ), &data );
            BIU start_x = biuParse( data, &data );
            BIU start_y = biuParse( data, &data );
            BIU end_x   = biuParse( data, &data );
            BIU end_y   = biuParse( data, &data );
            BIU width   = biuParse( data );

            if( width < 0 )
                width = 0;

            dseg->SetShape( static_cast<SHAPE_T>( shape ) );
            dseg->SetFilled( false );
            dseg->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );

            if( dseg->GetShape() == SHAPE_T::ARC )
            {
                dseg->SetCenter( VECTOR2I( start_x, start_y ) );
                dseg->SetStart( VECTOR2I( end_x, end_y ) );
            }
            else
            {
                dseg->SetStart( VECTOR2I( start_x, start_y ) );
                dseg->SetEnd( VECTOR2I( end_x, end_y ) );
            }
        }
        else if( TESTLINE( "De" ) )
        {
            BIU     x = 0;
            BIU     y;

            data = strtok_r( line + SZ( "De" ), delims, &saveptr );

            for( int i = 0;  data;  ++i, data = strtok_r( nullptr, delims, &saveptr ) )
            {
                switch( i )
                {
                case 0:
                    int layer;
                    layer = intParse( data );

                    if( layer < FIRST_NON_COPPER_LAYER )
                        layer = FIRST_NON_COPPER_LAYER;

                    else if( layer > LAST_NON_COPPER_LAYER )
                        layer = LAST_NON_COPPER_LAYER;

                    dseg->SetLayer( leg_layer2new( m_cu_count,  layer ) );
                    break;

                case 1:
                    ignore_unused( intParse( data ) );
                    break;

                case 2:
                {
                    EDA_ANGLE angle = degParse( data );

                    if( dseg->GetShape() == SHAPE_T::ARC )
                        dseg->SetArcAngleAndEnd( angle );

                    break;
                }

                case 3:
                    const_cast<KIID&>( dseg->m_Uuid ) = KIID( data );
                    break;

                case 4:
                    // Ignore state data
                    hexParse( data );
                    break;

                // Bezier Control Points
                case 5:
                    x = biuParse( data );
                    break;
                case 6:
                    y = biuParse( data );
                    dseg->SetBezierC1( VECTOR2I( x, y ) );
                    break;
                case 7:
                    x = biuParse( data );
                    break;
                case 8:
                    y = biuParse( data );
                    dseg->SetBezierC2( VECTOR2I( x, y ) );
                    break;

                default:
                    break;
                }
            }
        }
        else if( TESTLINE( "$EndDRAWSEGMENT" ) )
        {
            m_board->Add( dseg.release(), ADD_MODE::APPEND );
            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndDRAWSEGMENT'" ) );
}

void PCB_IO_KICAD_LEGACY::loadNETINFO_ITEM()
{
    /* a net description is something like
     * $EQUIPOT
     * Na 5 "/BIT1"
     * St ~
     * $EndEQUIPOT
     */

    char  buf[1024];

    NETINFO_ITEM*   net = nullptr;
    char*           line;
    int             netCode = 0;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        if( TESTLINE( "Na" ) )
        {
            // e.g. "Na 58 "/cpu.sch/PAD7"\r\n"

            netCode = intParse( line + SZ( "Na" ), &data );

            ReadDelimitedText( buf, data, sizeof(buf) );

            if( net == nullptr )
            {
                net = new NETINFO_ITEM( m_board, ConvertToNewOverbarNotation( From_UTF8( buf ) ),
                                        netCode );
            }
            else
            {
                THROW_IO_ERROR( wxT( "Two net definitions in  '$EQUIPOT' block" ) );
            }
        }
        else if( TESTLINE( "$EndEQUIPOT" ) )
        {
            // net 0 should be already in list, so store this net
            // if it is not the net 0, or if the net 0 does not exists.
            if( net && ( net->GetNetCode() > 0 || m_board->FindNet( 0 ) == nullptr ) )
            {
                m_board->Add( net );

                // Be sure we have room to store the net in m_netCodes
                if( (int)m_netCodes.size() <= netCode )
                    m_netCodes.resize( netCode+1 );

                m_netCodes[netCode] = net->GetNetCode();
                net = nullptr;
            }
            else
            {
                delete net;
                net = nullptr;     // Avoid double deletion.
            }

            return;     // preferred exit
        }
    }

    // If we are here, there is an error.
    delete net;
    THROW_IO_ERROR( wxT( "Missing '$EndEQUIPOT'" ) );
}


void PCB_IO_KICAD_LEGACY::loadPCB_TEXT()
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
    PCB_TEXT*  pcbtxt = new PCB_TEXT( m_board );
    m_board->Add( pcbtxt, ADD_MODE::APPEND );

    char*   line;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        if( TESTLINE( "Te" ) )          // Text line (or first line for multi line texts)
        {
            ReadDelimitedText( text, line + SZ( "Te" ), sizeof(text) );
            pcbtxt->SetText( ConvertToNewOverbarNotation( From_UTF8( text ) ) );
        }
        else if( TESTLINE( "nl" ) )     // next line of the current text
        {
            ReadDelimitedText( text, line + SZ( "nl" ), sizeof(text) );
            pcbtxt->SetText( pcbtxt->GetText() + wxChar( '\n' ) +  From_UTF8( text ) );
        }
        else if( TESTLINE( "Po" ) )
        {
            VECTOR2I size;
            BIU    pos_x = biuParse( line + SZ( "Po" ), &data );
            BIU    pos_y = biuParse( data, &data );

            size.x = biuParse( data, &data );
            size.y = biuParse( data, &data );

            BIU       thickn = biuParse( data, &data );
            EDA_ANGLE angle = degParse( data );

            pcbtxt->SetTextSize( size );
            pcbtxt->SetTextThickness( thickn );
            pcbtxt->SetTextAngle( angle );

            pcbtxt->SetTextPos( VECTOR2I( pos_x, pos_y ) );
        }
        else if( TESTLINE( "De" ) )
        {
            // e.g. "De 21 1 68183921-93a5-49ac-91b0-49d05a0e1647 Normal C\r\n"
            int   layer_num   = intParse( line + SZ( "De" ), &data );
            int   notMirrored = intParse( data, &data );
            char* uuid        = strtok_r( (char*) data, delims, (char**) &data );
            char* style       = strtok_r( nullptr, delims, (char**) &data );
            char* hJustify    = strtok_r( nullptr, delims, (char**) &data );
            char* vJustify    = strtok_r( nullptr, delims, (char**) &data );

            pcbtxt->SetMirrored( !notMirrored );
            const_cast<KIID&>( pcbtxt->m_Uuid ) = KIID( uuid );
            pcbtxt->SetItalic( !strcmp( style, "Italic" ) );

            if( hJustify )
            {
                pcbtxt->SetHorizJustify( horizJustify( hJustify ) );
            }
            else
            {
                // boom, somebody changed a constructor, I was relying on this:
                wxASSERT( pcbtxt->GetHorizJustify() == GR_TEXT_H_ALIGN_CENTER );
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

    THROW_IO_ERROR( wxT( "Missing '$EndTEXTPCB'" ) );
}


void PCB_IO_KICAD_LEGACY::loadTrackList( int aStructType )
{
    char*   line;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        checkpoint();

        // read two lines per loop iteration, each loop is one TRACK or VIA
        // example first line:
        // e.g. "Po 0 23994 28800 24400 28800 150 -1"  for a track
        // e.g. "Po 3 21086 17586 21086 17586 180 -1"  for a via (uses sames start and end)
        const char* data;

        if( line[0] == '$' )    // $EndTRACK
            return;             // preferred exit

        assert( TESTLINE( "Po" ) );

        // legacy via type is 3 (through via) 2 (BLIND/BURIED) or 1 (MICROVIA)
        int legacy_viatype = intParse( line + SZ( "Po" ), &data );

        BIU start_x = biuParse( data, &data );
        BIU start_y = biuParse( data, &data );
        BIU end_x   = biuParse( data, &data );
        BIU end_y   = biuParse( data, &data );
        BIU width   = biuParse( data, &data );

        // optional 7th drill parameter (must be optional in an old format?)
        data = strtok_r( (char*) data, delims, (char**) &data );

        BIU drill   = data ? biuParse( data ) : -1;     // SetDefault() if < 0

        // Read the 2nd line to determine the exact type, one of:
        // PCB_TRACE_T, PCB_VIA_T, or PCB_SEGZONE_T.  The type field in 2nd line
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
            THROW_IO_ERROR( wxT( "Missing 2nd line of a TRACK def" ) );
        }
#endif

        int           makeType;

        // parse the 2nd line to determine the type of object
        // e.g. "De 15 1 7 68183921-93a5-49ac-91b0-49d05a0e1647 0" for a via
        int   layer_num = intParse( line + SZ( "De" ), &data );
        int   type      = intParse( data, &data );
        int   net_code  = intParse( data, &data );
        char* uuid      = strtok_r( (char*) data, delims, (char**) &data );

        // Discard flags data
        intParse( data, (const char**) &data );

        if( aStructType == PCB_TRACE_T )
        {
            makeType = ( type == 1 ) ? PCB_VIA_T : PCB_TRACE_T;
        }
        else if (aStructType == NOT_USED )
        {
            continue;
        }
        else
        {
            wxFAIL_MSG( wxT( "Segment type unknown" ) );
            continue;
        }

        PCB_TRACK* newTrack = nullptr;
        PCB_VIA* newVia = nullptr;

        switch( makeType )
        {
        default:
        case PCB_TRACE_T: newTrack = new PCB_TRACK( m_board ); break;
        case PCB_VIA_T:   newVia = new PCB_VIA( m_board );     break;
        }

        if( makeType == PCB_VIA_T )     // Ensure layers are OK when possible:
        {
            VIATYPE viatype = VIATYPE::THROUGH;

            if( legacy_viatype == 1 )
                viatype = VIATYPE::MICROVIA;
            else if( legacy_viatype == 2 )
                viatype = VIATYPE::BLIND;

            newVia->SetViaType( viatype );
            newVia->SetWidth( PADSTACK::ALL_LAYERS, width );

            const_cast<KIID&>( newVia->m_Uuid ) = KIID( uuid );
            newVia->SetPosition( VECTOR2I( start_x, start_y ) );
            newVia->SetEnd( VECTOR2I( end_x, end_y ) );

            if( drill < 0 )
                newVia->SetDrillDefault();
            else
                newVia->SetDrill( drill );

            if( newVia->GetViaType() == VIATYPE::THROUGH )
            {
                newVia->SetLayerPair( F_Cu, B_Cu );
            }
            else
            {
                PCB_LAYER_ID back  = leg_layer2new( m_cu_count, (layer_num >> 4) & 0xf );
                PCB_LAYER_ID front = leg_layer2new( m_cu_count, layer_num & 0xf );

                if( is_leg_copperlayer_valid( m_cu_count, back ) &&
                    is_leg_copperlayer_valid( m_cu_count, front ) )
                {
                    newVia->SetLayerPair( front, back );
                }
                else
                {
                    delete newVia;
                    newVia = nullptr;
                }
            }
        }
        else
        {
            newTrack->SetWidth( width );

            const_cast<KIID&>( newTrack->m_Uuid ) = KIID( uuid );
            newTrack->SetPosition( VECTOR2I( start_x, start_y ) );
            newTrack->SetEnd( VECTOR2I( end_x, end_y ) );

            // A few legacy boards can have tracks on non existent layers, because
            // reducing the number of layers does not remove tracks on removed layers
            // If happens, skip them
            if( is_leg_copperlayer_valid( m_cu_count, layer_num ) )
            {
                newTrack->SetLayer( leg_layer2new( m_cu_count, layer_num ) );
            }
            else
            {
                delete newTrack;
                newTrack = nullptr;
            }
        }

        if( newTrack )
        {
            newTrack->SetNetCode( getNetCode( net_code ) );
            m_board->Add( newTrack );
        }

        if( newVia )
        {
            newVia->SetNetCode( getNetCode( net_code ) );
            m_board->Add( newVia );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndTRACK'" ) );
}


void PCB_IO_KICAD_LEGACY::loadNETCLASS()
{
    char        buf[1024];
    wxString    netname;
    char*       line;

    // create an empty NETCLASS without a name, but do not add it to the BOARD
    // yet since that would bypass duplicate netclass name checking within the BOARD.
    // store it temporarily in an unique_ptr until successfully inserted into the BOARD
    // just before returning.
    std::shared_ptr<NETCLASS> nc = std::make_shared<NETCLASS>( wxEmptyString );

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        if( TESTLINE( "AddNet" ) )      // most frequent type of line
        {
            // e.g. "AddNet "V3.3D"\n"
            ReadDelimitedText( buf, line + SZ( "AddNet" ), sizeof(buf) );
            netname = ConvertToNewOverbarNotation( From_UTF8( buf ) );

            m_board->GetDesignSettings().m_NetSettings->SetNetclassPatternAssignment(
                    netname, nc->GetName() );
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
            nc->SetName( From_UTF8( buf ) );
        }
        else if( TESTLINE( "Desc" ) )
        {
            ReadDelimitedText( buf, line + SZ( "Desc" ), sizeof(buf) );
            nc->SetDescription( From_UTF8( buf ) );
        }
        else if( TESTLINE( "$EndNCLASS" ) )
        {
            if( m_board->GetDesignSettings().m_NetSettings->HasNetclass( nc->GetName() ) )
            {
                // Must have been a name conflict, this is a bad board file.
                // User may have done a hand edit to the file.

                // unique_ptr will delete nc on this code path

                m_error.Printf( _( "Duplicate NETCLASS name '%s'." ), nc->GetName() );
                THROW_IO_ERROR( m_error );
            }
            else
            {
                m_board->GetDesignSettings().m_NetSettings->SetNetclass( nc->GetName(), nc );
            }

            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndNCLASS'" ) );
}


void PCB_IO_KICAD_LEGACY::loadZONE_CONTAINER()
{
    std::unique_ptr<ZONE> zc = std::make_unique<ZONE>( m_board );

    ZONE_BORDER_DISPLAY_STYLE outline_hatch = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;
    bool    endContour = false;
    int     holeIndex = -1;     // -1 is the main outline; holeIndex >= 0 = hole index
    char    buf[1024];
    char*   line;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        if( TESTLINE( "ZCorner" ) ) // new corner of the zone outlines found
        {
            // e.g. "ZCorner 25650 49500 0"
            BIU x    = biuParse( line + SZ( "ZCorner" ), &data );
            BIU y    = biuParse( data, &data );

            if( endContour )
            {
                // the previous corner was the last corner of a contour.
                // so this corner is the first of a new hole
                endContour = false;
                zc->NewHole();
                holeIndex++;
            }

            zc->AppendCorner( VECTOR2I( x, y ), holeIndex );

            // Is this corner the end of current contour?
            // the next corner (if any) will be stored in a new contour (a hole)
            // intParse( data )returns 0 = usual corner, 1 = last corner of the current contour:
            endContour = intParse( data );
        }
        else if( TESTLINE( "ZInfo" ) )      // general info found
        {
            // e.g. 'ZInfo 68183921-93a5-49ac-91b0-49d05a0e1647 310 "COMMON"'
            char* uuid    = strtok_r( (char*) line + SZ( "ZInfo" ), delims, (char**) &data  );
            int   netcode = intParse( data, &data );

            if( ReadDelimitedText( buf, data, sizeof(buf) ) > (int) sizeof(buf) )
                THROW_IO_ERROR( wxT( "ZInfo netname too long" ) );

            const_cast<KIID&>( zc->m_Uuid ) = KIID( uuid );

            // Init the net code only, not the netname, to be sure
            // the zone net name is the name read in file.
            // (When mismatch, the user will be prompted in DRC, to fix the actual name)
            zc->BOARD_CONNECTED_ITEM::SetNetCode( getNetCode( netcode ) );
        }
        else if( TESTLINE( "ZLayer" ) )     // layer found
        {
            int layer_num = intParse( line + SZ( "ZLayer" ) );
            zc->SetLayer( leg_layer2new( m_cu_count,  layer_num ) );
        }
        else if( TESTLINE( "ZAux" ) )       // aux info found
        {
            // e.g. "ZAux 7 E"
            ignore_unused( intParse( line + SZ( "ZAux" ), &data ) );
            char*   hopt   = strtok_r( (char*) data, delims, (char**) &data );

            if( !hopt )
            {
                m_error.Printf( _( "Bad ZAux for CZONE_CONTAINER '%s'" ),
                                zc->GetNetname().GetData() );
                THROW_IO_ERROR( m_error );
            }

            switch( *hopt ) // upper case required
            {
            case 'N': outline_hatch = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;      break;
            case 'E': outline_hatch = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE; break;
            case 'F': outline_hatch = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL; break;
            default:
                m_error.Printf( _( "Bad ZAux for CZONE_CONTAINER '%s'" ),
                                zc->GetNetname().GetData() );
                THROW_IO_ERROR( m_error );
            }

            // Set hatch mode later, after reading corner outline data
        }
        else if( TESTLINE( "ZSmoothing" ) )
        {
            // e.g. "ZSmoothing 0 0"
            int     smoothing    = intParse( line + SZ( "ZSmoothing" ), &data );
            BIU     cornerRadius = biuParse( data );

            if( smoothing >= ZONE_SETTINGS::SMOOTHING_LAST || smoothing < 0 )
            {
                m_error.Printf( _( "Bad ZSmoothing for CZONE_CONTAINER '%s'" ),
                                zc->GetNetname().GetData() );
                THROW_IO_ERROR( m_error );
            }

            zc->SetCornerSmoothingType( smoothing );
            zc->SetCornerRadius( cornerRadius );
        }
        else if( TESTLINE( "ZKeepout" ) )
        {
            char* token;
            zc->SetIsRuleArea( true );
            zc->SetDoNotAllowPads( false );        // Not supported in legacy
            zc->SetDoNotAllowFootprints( false );  // Not supported in legacy

            // e.g. "ZKeepout tracks N vias N pads Y"
            token = strtok_r( line + SZ( "ZKeepout" ), delims, (char**) &data );

            while( token )
            {
                if( !strcmp( token, "tracks" ) )
                {
                    token = strtok_r( nullptr, delims, (char**) &data );
                    zc->SetDoNotAllowTracks( token && *token == 'N' );
                }
                else if( !strcmp( token, "vias" ) )
                {
                    token = strtok_r( nullptr, delims, (char**) &data );
                    zc->SetDoNotAllowVias( token && *token == 'N' );
                }
                else if( !strcmp( token, "copperpour" ) )
                {
                    token = strtok_r( nullptr, delims, (char**) &data );
                    zc->SetDoNotAllowZoneFills( token && *token == 'N' );
                }

                token = strtok_r( nullptr, delims, (char**) &data );
            }
        }
        else if( TESTLINE( "ZOptions" ) )
        {
            // e.g. "ZOptions 0 32 F 200 200"
            int     fillmode    = intParse( line + SZ( "ZOptions" ), &data );
            ignore_unused( intParse( data, &data ) );
            char    fillstate   = data[1];      // here e.g. " F"
            BIU     thermalReliefGap = biuParse( data += 2 , &data );  // +=2 for " F"
            BIU     thermalReliefCopperBridge = biuParse( data );

            if( fillmode)
            {
                if( m_showLegacySegmentZoneWarning )
                {
                    wxLogWarning( _( "The legacy segment zone fill mode is no longer supported.\n"
                                     "Zone fills will be converted on a best-effort basis." ) );

                    m_showLegacySegmentZoneWarning = false;
                }
            }

            zc->SetFillMode( ZONE_FILL_MODE::POLYGONS );
            zc->SetIsFilled( fillstate == 'S' );
            zc->SetThermalReliefGap( thermalReliefGap );
            zc->SetThermalReliefSpokeWidth( thermalReliefCopperBridge );
        }
        else if( TESTLINE( "ZClearance" ) )     // Clearance and pad options info found
        {
            // e.g. "ZClearance 40 I"
            BIU     clearance = biuParse( line + SZ( "ZClearance" ), &data );
            char*   padoption = strtok_r( (char*) data, delims, (char**) &data );  // data: " I"

            ZONE_CONNECTION popt;
            switch( *padoption )
            {
            case 'I': popt = ZONE_CONNECTION::FULL;        break;
            case 'T': popt = ZONE_CONNECTION::THERMAL;     break;
            case 'H': popt = ZONE_CONNECTION::THT_THERMAL; break;
            case 'X': popt = ZONE_CONNECTION::NONE;        break;
            default:
                m_error.Printf( _( "Bad ZClearance padoption for CZONE_CONTAINER '%s'" ),
                                zc->GetNetname().GetData() );
                THROW_IO_ERROR( m_error );
            }

            zc->SetLocalClearance( clearance );
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
            zc->SetAssignedPriority( priority );
        }
        else if( TESTLINE( "$POLYSCORNERS" ) )
        {
            // Read the PolysList (polygons that are the solid areas in the filled zone)
            SHAPE_POLY_SET polysList;

            bool makeNewOutline = true;

            while( ( line = READLINE( m_reader ) ) != nullptr )
            {
                if( TESTLINE( "$endPOLYSCORNERS" ) )
                    break;

                // e.g. "39610 43440 0 0"
                BIU     x = biuParse( line, &data );
                BIU     y = biuParse( data, &data );

                if( makeNewOutline )
                    polysList.NewOutline();

                polysList.Append( x, y );

                // end_countour was a bool when file saved, so '0' or '1' here
                bool end_contour = intParse( data, &data );
                intParse( data ); // skip corner utility flag

                makeNewOutline = end_contour;
            }

            zc->SetFilledPolysList( zc->GetFirstLayer(), polysList );
        }
        else if( TESTLINE( "$FILLSEGMENTS" ) )
        {
            while( ( line = READLINE( m_reader ) ) != nullptr )
            {
                if( TESTLINE( "$endFILLSEGMENTS" ) )
                    break;

                // e.g. ""%d %d %d %d\n"
                ignore_unused( biuParse( line, &data ) );
                ignore_unused( biuParse( data, &data ) );
                ignore_unused( biuParse( data, &data ) );
                ignore_unused( biuParse( data ) );
            }
        }
        else if( TESTLINE( "$endCZONE_OUTLINE" ) )
        {
            // Ensure keepout does not have a net
            // (which have no sense for a keepout zone)
            if( zc->GetIsRuleArea() )
                zc->SetNetCode( NETINFO_LIST::UNCONNECTED );

            if( zc->GetMinThickness() > 0 )
            {
                // Inflate the fill polygon
                PCB_LAYER_ID   layer = zc->GetFirstLayer();
                SHAPE_POLY_SET inflatedFill = SHAPE_POLY_SET( *zc->GetFilledPolysList( layer ) );

                inflatedFill.InflateWithLinkedHoles( zc->GetMinThickness() / 2,
                                                     CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                                     ARC_HIGH_DEF / 2 );

                zc->SetFilledPolysList( layer, inflatedFill );
            }

            // should always occur, but who knows, a zone without two corners
            // is no zone at all, it's a spot?

            if( zc->GetNumCorners() > 2 )
            {
                if( !zc->IsOnCopperLayer() )
                {
                    zc->SetFillMode( ZONE_FILL_MODE::POLYGONS );
                    zc->SetNetCode( NETINFO_LIST::UNCONNECTED );
                }

                // HatchBorder here, after outlines corners are read
                // Set hatch here, after outlines corners are read
                zc->SetBorderDisplayStyle( outline_hatch, ZONE::GetDefaultHatchPitch(), true );

                m_board->Add( zc.release() );
            }

            return;     // preferred exit
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$endCZONE_OUTLINE'" ) );
}


void PCB_IO_KICAD_LEGACY::loadDIMENSION()
{
    std::unique_ptr<PCB_DIM_ALIGNED> dim = std::make_unique<PCB_DIM_ALIGNED>( m_board,
                                                                              PCB_DIM_ALIGNED_T );
    VECTOR2I crossBarO;
    VECTOR2I crossBarF;

    char*   line;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char*  data;

        if( TESTLINE( "$endCOTATION" ) )
        {
            dim->UpdateHeight( crossBarF, crossBarO );

            m_board->Add( dim.release(), ADD_MODE::APPEND );
            return;     // preferred exit
        }
        else if( TESTLINE( "Va" ) )
        {
            BIU value = biuParse( line + SZ( "Va" ) );

            // unused; dimension value is calculated from coordinates
            ( void )value;
        }
        else if( TESTLINE( "Ge" ) )
        {
            // e.g. "Ge 1 21 68183921-93a5-49ac-91b0-49d05a0e1647\r\n"
            int   shape     = intParse( line + SZ( "De" ), (const char**) &data );
            int   layer_num = intParse( data, &data );
            char* uuid      = strtok_r( (char*) data, delims, (char**) &data );

            dim->SetLayer( leg_layer2new( m_cu_count, layer_num ) );
            const_cast<KIID&>( dim->m_Uuid ) = KIID( uuid );

            // not used
            ( void )shape;
        }
        else if( TESTLINE( "Te" ) )
        {
            char  buf[2048];

            ReadDelimitedText( buf, line + SZ( "Te" ), sizeof(buf) );
            dim->SetOverrideText( From_UTF8( buf ) );
            dim->SetOverrideTextEnabled( true );
            dim->SetUnitsFormat( DIM_UNITS_FORMAT::NO_SUFFIX );
            dim->SetAutoUnits();
        }
        else if( TESTLINE( "Po" ) )
        {
            BIU       pos_x  = biuParse( line + SZ( "Po" ), &data );
            BIU       pos_y  = biuParse( data, &data );
            BIU       width  = biuParse( data, &data );
            BIU       height = biuParse( data, &data );
            BIU       thickn = biuParse( data, &data );
            EDA_ANGLE orient = degParse( data, &data );
            char*     mirror = strtok_r( (char*) data, delims, (char**) &data );

            dim->SetTextPos( VECTOR2I( pos_x, pos_y ) );
            dim->SetTextSize( VECTOR2I( width, height ) );
            dim->SetMirrored( mirror && *mirror == '0' );
            dim->SetTextThickness( thickn );
            dim->SetTextAngle( orient );
        }
        else if( TESTLINE( "Sb" ) )
        {
            ignore_unused( biuParse( line + SZ( "Sb" ), &data ) );
            BIU crossBarOx = biuParse( data, &data );
            BIU crossBarOy = biuParse( data, &data );
            BIU crossBarFx = biuParse( data, &data );
            BIU crossBarFy = biuParse( data, &data );
            BIU width      = biuParse( data );

            dim->SetLineThickness( width );
            crossBarO = VECTOR2I( crossBarOx, crossBarOy );
            crossBarF = VECTOR2I( crossBarFx, crossBarFy );
        }
        else if( TESTLINE( "Sd" ) )
        {
            ignore_unused( intParse( line + SZ( "Sd" ), &data ) );
            BIU featureLineDOx = biuParse( data, &data );
            BIU featureLineDOy = biuParse( data, &data );

            ignore_unused( biuParse( data, &data ) );
            ignore_unused( biuParse( data ) );

            dim->SetStart( VECTOR2I( featureLineDOx, featureLineDOy ) );
        }
        else if( TESTLINE( "Sg" ) )
        {
            ignore_unused( intParse( line + SZ( "Sg" ), &data ) );
            BIU featureLineGOx = biuParse( data, &data );
            BIU featureLineGOy = biuParse( data, &data );

            ignore_unused( biuParse( data, &data ) );
            ignore_unused( biuParse( data ) );

            dim->SetEnd( VECTOR2I( featureLineGOx, featureLineGOy ) );
        }
        else if( TESTLINE( "S1" ) )        // Arrow: no longer imported
        {
            ignore_unused( intParse( line + SZ( "S1" ), &data ) );
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );
            biuParse( data );
        }
        else if( TESTLINE( "S2" ) )        // Arrow: no longer imported
        {
            ignore_unused( intParse( line + SZ( "S2" ), &data ) );
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );
            biuParse( data, &data );
        }
        else if( TESTLINE( "S3" ) )        // Arrow: no longer imported
        {
            ignore_unused( intParse( line + SZ( "S3" ), &data ) );
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );
            biuParse( data, &data );
        }
        else if( TESTLINE( "S4" ) )        // Arrow: no longer imported
        {
            ignore_unused( intParse( line + SZ( "S4" ), &data ) );
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );    // skipping excessive data
            biuParse( data, &data );
            biuParse( data, &data );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$endCOTATION'" ) );
}


void PCB_IO_KICAD_LEGACY::loadPCB_TARGET()
{
    char* line;

    while( ( line = READLINE( m_reader ) ) != nullptr )
    {
        const char* data;

        if( TESTLINE( "$EndPCB_TARGET" ) || TESTLINE( "$EndMIREPCB" ) )
        {
            return;     // preferred exit
        }
        else if( TESTLINE( "Po" ) )
        {
            int   shape     = intParse( line + SZ( "Po" ), &data );
            int   layer_num = intParse( data, &data );
            BIU   pos_x     = biuParse( data, &data );
            BIU   pos_y     = biuParse( data, &data );
            BIU   size      = biuParse( data, &data );
            BIU   width     = biuParse( data, &data );
            char* uuid      = strtok_r( (char*) data, delims, (char**) &data  );

            if( layer_num < FIRST_NON_COPPER_LAYER )
                layer_num = FIRST_NON_COPPER_LAYER;
            else if( layer_num > LAST_NON_COPPER_LAYER )
                layer_num = LAST_NON_COPPER_LAYER;

            PCB_TARGET* t = new PCB_TARGET( m_board, shape, leg_layer2new( m_cu_count,  layer_num ),
                                            VECTOR2I( pos_x, pos_y ), size, width );
            m_board->Add( t, ADD_MODE::APPEND );

            const_cast<KIID&>( t->m_Uuid ) = KIID( uuid );
        }
    }

    THROW_IO_ERROR( wxT( "Missing '$EndDIMENSION'" ) );
}


BIU PCB_IO_KICAD_LEGACY::biuParse( const char* aValue, const char** nptrptr )
{
    const char* end = aValue;
    double      fval{};
    fast_float::from_chars_result result = fast_float::from_chars( aValue, aValue + strlen( aValue ), fval,
                                                                   fast_float::chars_format::skip_white_space );
    end = result.ptr;

    if( result.ec != std::errc() )
    {
        m_error.Printf( _( "Invalid floating point number in file: '%s'\nline: %d, offset: %d" ),
                        m_reader->GetSource().GetData(),
                        (int) m_reader->LineNumber(),
                        (int)( aValue - m_reader->Line() + 1 ) );

        THROW_IO_ERROR( m_error );
    }

    if( aValue == end )
    {
        m_error.Printf( _( "Missing floating point number in file: '%s'\nline: %d, offset: %d" ),
                        m_reader->GetSource().GetData(),
                        (int) m_reader->LineNumber(),
                        (int)( aValue - m_reader->Line() + 1 ) );

        THROW_IO_ERROR( m_error );
    }

    if( nptrptr )
        *nptrptr = end;

    fval *= diskToBiu;

    // fval is up into the whole number realm here, and should be bounded
    // within INT_MIN to INT_MAX since BIU's are nanometers.
    return KiROUND( fval );
}


EDA_ANGLE PCB_IO_KICAD_LEGACY::degParse( const char* aValue, const char** nptrptr )
{
    const char*                   end = aValue;
    double                        fval{};
    fast_float::from_chars_result result = fast_float::from_chars( aValue, aValue + strlen( aValue ), fval,
                                                                   fast_float::chars_format::skip_white_space );
    end = result.ptr;

    if( result.ec != std::errc() )
    {
        m_error.Printf( _( "Invalid floating point number in file: '%s'\nline: %d, offset: %d" ),
                        m_reader->GetSource().GetData(),
                        (int) m_reader->LineNumber(),
                        (int)( aValue - m_reader->Line() + 1 ) );

        THROW_IO_ERROR( m_error );
    }

    if( aValue == end )
    {
        m_error.Printf( _( "Missing floating point number in file: '%s'\nline: %d, offset: %d" ),
                        m_reader->GetSource().GetData(),
                        (int) m_reader->LineNumber(),
                        (int)( aValue - m_reader->Line() + 1 ) );

        THROW_IO_ERROR( m_error );
    }

    if( nptrptr )
        *nptrptr = end;

    return EDA_ANGLE( fval, TENTHS_OF_A_DEGREE_T );
}


void PCB_IO_KICAD_LEGACY::init( const std::map<std::string, UTF8>* aProperties )
{
    m_loading_format_version = 0;
    m_cu_count = 16;
    m_board = nullptr;
    m_showLegacySegmentZoneWarning = true;
    m_props = aProperties;

    // conversion factor for saving RAM BIUs to KICAD legacy file format.
    biuToDisk = 1.0 / pcbIUScale.IU_PER_MM; // BIUs are nanometers & file is mm

    // Conversion factor for loading KICAD legacy file format into BIUs in RAM
    // Start by assuming the *.brd file is in deci-mils.
    // If we see "Units mm" in the $GENERAL section, set diskToBiu to 1000000.0
    // then, during the file loading process, to start a conversion from
    // mm to nanometers.  The deci-mil legacy files have no such "Units" marker
    // so we must assume the file is in deci-mils until told otherwise.

    diskToBiu = pcbIUScale.IU_PER_MILS / 10;    // BIUs are nanometers
}


//-----<FOOTPRINT LIBRARY FUNCTIONS>--------------------------------------------

/*

    The legacy file format is being obsoleted and this code will have a short
    lifetime, so it only needs to be good enough for a short duration of time.
    Caching all the MODULEs is a bit memory intensive, but it is a considerably
    faster way of fulfilling the API contract. Otherwise, without the cache, you
    would have to re-read the file when searching for any FOOTPRINT, and this would
    be very problematic filling a FOOTPRINT_LIST via this PLUGIN API. If memory
    becomes a concern, consider the cache lifetime policy, which determines the
    time that a LP_CACHE is in RAM. Note PLUGIN lifetime also plays a role in
    cache lifetime.

*/


typedef boost::ptr_map< std::string, FOOTPRINT >   FOOTPRINT_MAP;


/**
 * The footprint portion of the PLUGIN API, and only for the PCB_IO_KICAD_LEGACY, so therefore is
 * private to this implementation file, i.e. not placed into a header.
 */
struct LP_CACHE
{
    LP_CACHE( PCB_IO_KICAD_LEGACY* aOwner, const wxString& aLibraryPath );

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any PLUGIN.
    // Catch these exceptions higher up please.

    void Load();

    void ReadAndVerifyHeader( LINE_READER* aReader );

    void SkipIndex( LINE_READER* aReader );

    void LoadModules( LINE_READER* aReader );

    bool IsModified();
    static long long GetTimestamp( const wxString& aLibPath );

    PCB_IO_KICAD_LEGACY*  m_owner;            // my owner, I need its PCB_IO_KICAD_LEGACY::loadFOOTPRINT()
    wxString        m_lib_path;
    FOOTPRINT_MAP   m_footprints;       // map or tuple of footprint_name vs. FOOTPRINT*
    bool            m_writable;

    bool            m_cache_dirty;      // Stored separately because it's expensive to check
                                        // m_cache_timestamp against all the files.
    long long       m_cache_timestamp;  // A hash of the timestamps for all the footprint
                                        // files.
};


LP_CACHE::LP_CACHE( PCB_IO_KICAD_LEGACY* aOwner, const wxString& aLibraryPath ) :
    m_owner( aOwner ),
    m_lib_path( aLibraryPath ),
    m_writable( true ),
    m_cache_dirty( true ),
    m_cache_timestamp( 0 )
{
}


bool LP_CACHE::IsModified()
{
    m_cache_dirty = m_cache_dirty || GetTimestamp( m_lib_path ) != m_cache_timestamp;

    return m_cache_dirty;
}


long long LP_CACHE::GetTimestamp( const wxString& aLibPath )
{
    wxFileName fn( aLibPath );

    if( fn.IsFileReadable() && fn.GetModificationTime().IsValid() )
        return fn.GetModificationTime().GetValue().GetValue();
    else
        return 0;
}


void LP_CACHE::Load()
{
    m_cache_dirty = false;

    FILE_LINE_READER    reader( m_lib_path );

    ReadAndVerifyHeader( &reader );
    SkipIndex( &reader );
    LoadModules( &reader );

    // Remember the file modification time of library file when the
    // cache snapshot was made, so that in a networked environment we will
    // reload the cache as needed.
    m_cache_timestamp = GetTimestamp( m_lib_path );
}


void LP_CACHE::ReadAndVerifyHeader( LINE_READER* aReader )
{
    char* line = aReader->ReadLine();
    char* data;

    if( !line )
        THROW_IO_ERROR( wxString::Format( _( "File '%s' is empty." ), m_lib_path ) );

    if( !TESTLINE( "PCBNEW-LibModule-V1" ) )
        THROW_IO_ERROR( wxString::Format( _( "File '%s' is not a legacy library." ), m_lib_path ) );

    while( ( line = aReader->ReadLine() ) != nullptr )
    {
        if( TESTLINE( "Units" ) )
        {
            const char* units = strtok_r( line + SZ( "Units" ), delims, &data );

            if( !strcmp( units, "mm" ) )
                m_owner->diskToBiu = pcbIUScale.IU_PER_MM;

        }
        else if( TESTLINE( "$INDEX" ) )
        {
            return;
        }
    }
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

            while( ( line = aReader->ReadLine() ) != nullptr )
            {
                if( TESTLINE( "$EndINDEX" ) )
                {
                    exit = true;
                    break;
                }
            }
        }
        else if( exit )
        {
            break;
        }
    } while( ( line = aReader->ReadLine() ) != nullptr );
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
            std::unique_ptr<FOOTPRINT> fp_ptr = std::make_unique<FOOTPRINT>( m_owner->m_board );

            std::string         footprintName = StrPurge( line + SZ( "$MODULE" ) );

            // The footprint names in legacy libraries can contain the '/' and ':'
            // characters which will cause the LIB_ID parser to choke.
            ReplaceIllegalFileNameChars( footprintName );

            // set the footprint name first thing, so exceptions can use name.
            fp_ptr->SetFPID( LIB_ID( wxEmptyString, footprintName ) );

            m_owner->loadFOOTPRINT( fp_ptr.get());

            FOOTPRINT* fp = fp_ptr.release();   // exceptions after this are not expected.

            // Not sure why this is asserting on debug builds.  The debugger shows the
            // strings are the same.  If it's not really needed maybe it can be removed.

            /*

            There was a bug in old legacy library management code
            (pre-PCB_IO_KICAD_LEGACY) which was introducing duplicate footprint names
            in legacy libraries without notification. To best recover from such
            bad libraries, and use them to their fullest, there are a few
            strategies that could be used. (Note: footprints must have unique
            names to be accepted into this cache.) The strategy used here is to
            append a differentiating version counter to the end of the name as:
            _v2, _v3, etc.

            */

            FOOTPRINT_MAP::const_iterator it = m_footprints.find( footprintName );

            if( it == m_footprints.end() )  // footprintName is not present in cache yet.
            {
                if( !m_footprints.insert( footprintName, fp ).second )
                {
                    wxFAIL_MSG( wxT( "error doing cache insert using guaranteed unique name" ) );
                }
            }
            else
            {
                // Bad library has a duplicate of this footprintName, generate a
                // unique footprint name and load it anyway.
                bool    nameOK = false;
                int     version = 2;
                char    buf[48];

                while( !nameOK )
                {
                    std::string newName = footprintName;

                    newName += "_v";
                    snprintf( buf, sizeof(buf), "%d", version++ );
                    newName += buf;

                    it = m_footprints.find( newName );

                    if( it == m_footprints.end() )
                    {
                        nameOK = true;

                        fp->SetFPID( LIB_ID( wxEmptyString, newName ) );

                        if( !m_footprints.insert( newName, fp ).second )
                        {
                            wxFAIL_MSG( wxT( "error doing cache insert using guaranteed unique "
                                             "name" ) );
                        }
                    }
                }
            }
        }

    } while( ( line = aReader->ReadLine() ) != nullptr );
}


long long PCB_IO_KICAD_LEGACY::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return LP_CACHE::GetTimestamp( aLibraryPath );
}


void PCB_IO_KICAD_LEGACY::cacheLib( const wxString& aLibraryPath )
{
    if( !m_cache || m_cache->m_lib_path != aLibraryPath || m_cache->IsModified() )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new LP_CACHE( this, aLibraryPath );
        m_cache->Load();
    }
}


void PCB_IO_KICAD_LEGACY::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibPath,
                                        bool aBestEfforts, const std::map<std::string, UTF8>* aProperties )
{
    wxString  errorMsg;

    init( aProperties );

    try
    {
        cacheLib( aLibPath );
    }
    catch( const IO_ERROR& ioe )
    {
        errorMsg = ioe.What();
    }

    // Some of the files may have been parsed correctly so we want to add the valid files to
    // the library.

    for( const auto& footprint : m_cache->m_footprints )
        aFootprintNames.Add( From_UTF8( footprint.first.c_str() ) );

    if( !errorMsg.IsEmpty() && !aBestEfforts )
        THROW_IO_ERROR( errorMsg );
}


FOOTPRINT* PCB_IO_KICAD_LEGACY::FootprintLoad( const wxString& aLibraryPath,
                                               const wxString& aFootprintName, bool aKeepUUID,
                                               const std::map<std::string, UTF8>* aProperties )
{
    init( aProperties );

    cacheLib( aLibraryPath );

    const FOOTPRINT_MAP&          footprints = m_cache->m_footprints;
    FOOTPRINT_MAP::const_iterator it = footprints.find( TO_UTF8( aFootprintName ) );

    if( it == footprints.end() )
        return nullptr;

    // Return copy of already loaded FOOTPRINT
    FOOTPRINT* copy = (FOOTPRINT*) it->second->Duplicate( IGNORE_PARENT_GROUP );
    copy->SetParent( nullptr );
    return copy;
}


bool PCB_IO_KICAD_LEGACY::DeleteLibrary( const wxString& aLibraryPath,
                                         const std::map<std::string, UTF8>* aProperties )
{
    wxFileName fn = aLibraryPath;

    if( !fn.FileExists() )
        return false;

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( wxRemove( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint library '%s' cannot be deleted." ),
                                          aLibraryPath.GetData() ) );
    }

    if( m_cache && m_cache->m_lib_path == aLibraryPath )
    {
        delete m_cache;
        m_cache = nullptr;
    }

    return true;
}


bool PCB_IO_KICAD_LEGACY::IsLibraryWritable( const wxString& aLibraryPath )
{
    init( nullptr );

    cacheLib( aLibraryPath );

    return m_cache->m_writable;
}


PCB_IO_KICAD_LEGACY::PCB_IO_KICAD_LEGACY() : PCB_IO( wxS( "KiCad-Legacy" ) ),
    m_cu_count( 16 ),               // for FootprintLoad()
    m_progressReporter( nullptr ),
    m_lastProgressLine( 0 ),
    m_lineCount( 0 ),
    m_reader( nullptr ),
    m_fp( nullptr ),
    m_cache( nullptr )
{
    init( nullptr );
}


PCB_IO_KICAD_LEGACY::~PCB_IO_KICAD_LEGACY()
{
    delete m_cache;
}
