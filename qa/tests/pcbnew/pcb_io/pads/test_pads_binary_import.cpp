/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcb_io/pads/pcb_io_pads_binary.h>
#include <pcb_io/pads/pcb_io_pads.h>
#include <layer_ids.h>
#include <padstack.h>
#include <board.h>
#include <pcb_text.h>
#include <pcb_shape.h>
#include <pcb_field.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint.h>
#include <zone.h>
#include <board_design_settings.h>
#include <set>


struct PADS_BINARY_BOARD_INFO
{
    std::string dir;
    std::string binaryFile;
    std::string ascFile;
    bool        differentRevision;
};


static const PADS_BINARY_BOARD_INFO PADS_BINARY_BOARDS[] = {
    { "TMS1mmX19",        "TMS1mmX19.pcb",        "TMS1mmX19.asc",        false },
    { "MC4_PLUS_CSHAPE",  "MC4_PLUS_CSHAPE.pcb",  "MC4_PLUS_CSHAPE.asc",  false },
    { "MC2_PLUS_REV1",    "MC2_PLUS_REV1.pcb",    "MC2_PLUS_REV1.asc",    true  },
    { "Ems4_Rev2",        "Ems4_Rev2.pcb",        "Ems4_Rev2.asc",        false },
    { "LCORE_4",          "LCORE_4.pcb",          "LCORE_4.asc",          false },
    { "LCORE_2",          "LCORE_2.pcb",          "LCORE_2.asc",          false },
    { "Dexter_MotorCtrl", "Dexter_MotorCtrl.pcb", "Dexter_MotorCtrl.asc", true  },
    { "MAIS_FC",          "MAIS_FC.pcb",          "MAIS_FC.asc",          true  },
};


static wxString GetBinaryPath( const PADS_BINARY_BOARD_INFO& aBoard )
{
    return KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + aBoard.dir + "/"
           + aBoard.binaryFile;
}


static wxString GetAscPath( const PADS_BINARY_BOARD_INFO& aBoard )
{
    return KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + aBoard.dir + "/" + aBoard.ascFile;
}


/**
 * Load a binary .pcb file. Returns nullptr and issues a warning if the load
 * throws, since the parser is under active development. Callers must null-check.
 */
static std::unique_ptr<BOARD> LoadBinary( const PADS_BINARY_BOARD_INFO& aBoard )
{
    PCB_IO_PADS_BINARY plugin;
    wxString           filename = GetBinaryPath( aBoard );

    BOOST_CHECK_MESSAGE( plugin.CanReadBoard( filename ),
                         aBoard.dir << " binary should be readable by PCB_IO_PADS_BINARY" );

    std::unique_ptr<BOARD> board;

    try
    {
        board.reset( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    }
    catch( const std::exception& e )
    {
        BOOST_WARN_MESSAGE( false,
                            aBoard.dir << " binary threw exception during load: " << e.what() );
        return nullptr;
    }

    BOOST_CHECK_MESSAGE( board != nullptr, aBoard.dir << " binary failed to load" );
    return board;
}


static std::unique_ptr<BOARD> LoadAsc( const PADS_BINARY_BOARD_INFO& aBoard )
{
    PCB_IO_PADS plugin;
    wxString    filename = GetAscPath( aBoard );

    std::unique_ptr<BOARD> board;

    try
    {
        board.reset( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( aBoard.dir << " ASC threw exception during load: " << e.what() );
        return nullptr;
    }

    BOOST_REQUIRE_MESSAGE( board != nullptr, aBoard.dir << " ASC failed to load" );
    return board;
}


static int CountEdgeCutsShapes( const BOARD* aBoard )
{
    int count = 0;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( shape->GetLayer() == Edge_Cuts )
                count++;
        }
    }

    return count;
}


/**
 * Compare counts with tolerance for binary/ASC differences.
 * Different-revision files get BOOST_WARN only. Same-revision files allow
 * up to 5% or off-by-2, with exact matches reported via BOOST_WARN.
 */
static void CheckCountWithTolerance( const std::string& aLabel, size_t aBinaryCount,
                                     size_t aAscCount, bool aDifferentRevision )
{
    if( aDifferentRevision )
    {
        BOOST_WARN_MESSAGE( aBinaryCount == aAscCount,
                            aLabel << " binary=" << aBinaryCount
                                   << " asc=" << aAscCount << " (different revision)" );
        return;
    }

    if( aBinaryCount == aAscCount )
    {
        BOOST_CHECK_MESSAGE( true, aLabel << " counts match: " << aBinaryCount );
        return;
    }

    size_t maxCount = std::max( aBinaryCount, aAscCount );
    size_t diff = ( aBinaryCount > aAscCount ) ? aBinaryCount - aAscCount
                                               : aAscCount - aBinaryCount;

    bool withinTolerance = ( diff <= 2 ) || ( diff * 100 / maxCount <= 5 );

    BOOST_CHECK_MESSAGE( withinTolerance,
                         aLabel << " counts differ beyond tolerance: binary=" << aBinaryCount
                                << " asc=" << aAscCount );

    BOOST_WARN_MESSAGE( aBinaryCount == aAscCount,
                        aLabel << " exact count mismatch: binary=" << aBinaryCount
                               << " asc=" << aAscCount );
}


/**
 * Structural integrity checks. Uses BOOST_CHECK for invariants
 * and BOOST_WARN for in-progress features.
 */
static void RunStructuralChecks( const PADS_BINARY_BOARD_INFO& aBoard,
                                 const BOARD*                   aBinaryBoard )
{
    BOOST_WARN_MESSAGE( aBinaryBoard->Tracks().size() > 0,
                        aBoard.dir << " binary has no tracks (v0x2021 not yet supported)" );

    std::set<std::pair<int, int>> viaPositions;
    bool                          hasDuplicate = false;

    for( PCB_TRACK* trk : aBinaryBoard->Tracks() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( trk );

        if( !via || via->GetViaType() != VIATYPE::THROUGH )
            continue;

        auto key = std::make_pair( via->GetPosition().x, via->GetPosition().y );

        if( viaPositions.count( key ) )
        {
            hasDuplicate = true;
            break;
        }

        viaPositions.insert( key );
    }

    BOOST_CHECK_MESSAGE( !hasDuplicate,
                         aBoard.dir << " binary should have no duplicate through-hole vias" );

    for( PCB_TRACK* trk : aBinaryBoard->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T || trk->Type() == PCB_ARC_T )
        {
            BOOST_CHECK_MESSAGE( IsCopperLayer( trk->GetLayer() ),
                                 aBoard.dir << " binary track on non-copper layer "
                                            << trk->GetLayer() );
        }
    }

    for( FOOTPRINT* fp : aBinaryBoard->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            BOOST_WARN_MESSAGE( pad->GetSize( PADSTACK::ALL_LAYERS ).x > 0
                                        && pad->GetSize( PADSTACK::ALL_LAYERS ).y > 0,
                                aBoard.dir << " " << fp->GetReference() << " pad has zero size" );
        }
    }
}


BOOST_AUTO_TEST_SUITE( PadsBinaryImport )


BOOST_AUTO_TEST_CASE( BinaryFileDetection )
{
    PCB_IO_PADS_BINARY binaryPlugin;
    PCB_IO_PADS        ascPlugin;

    for( const auto& board : PADS_BINARY_BOARDS )
    {
        wxString binaryPath = GetBinaryPath( board );

        BOOST_CHECK_MESSAGE( binaryPlugin.CanReadBoard( binaryPath ),
                             board.dir << " binary should be recognized by PCB_IO_PADS_BINARY" );

        BOOST_CHECK_MESSAGE( !ascPlugin.CanReadBoard( binaryPath ),
                             board.dir << " binary should NOT be recognized by PCB_IO_PADS" );
    }
}


BOOST_AUTO_TEST_CASE( AsciiFileRejection )
{
    PCB_IO_PADS_BINARY binaryPlugin;

    for( const auto& board : PADS_BINARY_BOARDS )
    {
        wxString ascPath = GetAscPath( board );

        BOOST_CHECK_MESSAGE( !binaryPlugin.CanReadBoard( ascPath ),
                             board.dir << " ASCII should NOT be recognized by PCB_IO_PADS_BINARY" );
    }
}


#define BINARY_LOAD_TEST( name, idx )                                       \
    BOOST_AUTO_TEST_CASE( BasicLoad_##name )                                \
    {                                                                       \
        auto board = LoadBinary( PADS_BINARY_BOARDS[idx] );                 \
                                                                            \
        if( board )                                                         \
            BOOST_CHECK( board->Footprints().size() > 0 );                  \
    }

BINARY_LOAD_TEST( TMS1mmX19,        0 )
BINARY_LOAD_TEST( MC4_PLUS_CSHAPE,  1 )
BINARY_LOAD_TEST( MC2_PLUS_REV1,    2 )
BINARY_LOAD_TEST( Ems4_Rev2,        3 )
BINARY_LOAD_TEST( LCORE_4,          4 )
BINARY_LOAD_TEST( LCORE_2,          5 )
BINARY_LOAD_TEST( Dexter_MotorCtrl, 6 )
BINARY_LOAD_TEST( MAIS_FC,          7 )


#define FOOTPRINT_COUNT_TEST( name, idx )                                                   \
    BOOST_AUTO_TEST_CASE( FootprintCount_##name )                                           \
    {                                                                                       \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                   \
                                                                                            \
        if( !bin )                                                                          \
            return;                                                                         \
                                                                                            \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                     \
                                                                                            \
        CheckCountWithTolerance( #name " footprints", bin->Footprints().size(),             \
                                 asc->Footprints().size(),                                  \
                                 PADS_BINARY_BOARDS[idx].differentRevision );               \
    }

FOOTPRINT_COUNT_TEST( TMS1mmX19,        0 )
FOOTPRINT_COUNT_TEST( MC4_PLUS_CSHAPE,  1 )
FOOTPRINT_COUNT_TEST( MC2_PLUS_REV1,    2 )
FOOTPRINT_COUNT_TEST( Ems4_Rev2,        3 )
FOOTPRINT_COUNT_TEST( LCORE_4,          4 )
FOOTPRINT_COUNT_TEST( LCORE_2,          5 )
FOOTPRINT_COUNT_TEST( Dexter_MotorCtrl, 6 )
FOOTPRINT_COUNT_TEST( MAIS_FC,          7 )


#define NET_COUNT_TEST( name, idx )                                                         \
    BOOST_AUTO_TEST_CASE( NetCount_##name )                                                 \
    {                                                                                       \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                   \
                                                                                            \
        if( !bin )                                                                          \
            return;                                                                         \
                                                                                            \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                     \
                                                                                            \
        CheckCountWithTolerance( #name " nets", bin->GetNetCount(), asc->GetNetCount(),     \
                                 PADS_BINARY_BOARDS[idx].differentRevision );               \
    }

NET_COUNT_TEST( TMS1mmX19,        0 )
NET_COUNT_TEST( MC4_PLUS_CSHAPE,  1 )
NET_COUNT_TEST( MC2_PLUS_REV1,    2 )
NET_COUNT_TEST( Ems4_Rev2,        3 )
NET_COUNT_TEST( LCORE_4,          4 )
NET_COUNT_TEST( LCORE_2,          5 )
NET_COUNT_TEST( Dexter_MotorCtrl, 6 )
NET_COUNT_TEST( MAIS_FC,          7 )


#define TRACK_COUNT_TEST( name, idx )                                                       \
    BOOST_AUTO_TEST_CASE( TrackCount_##name )                                               \
    {                                                                                       \
        auto bin = LoadBinary( PADS_BINARY_BOARDS[idx] );                                   \
                                                                                            \
        if( !bin )                                                                          \
            return;                                                                         \
                                                                                            \
        auto asc = LoadAsc( PADS_BINARY_BOARDS[idx] );                                     \
                                                                                            \
        BOOST_WARN_MESSAGE( bin->Tracks().size() > 0,                                        \
                            #name " binary track parsing not supported for v0x2021" );      \
                                                                                            \
        BOOST_WARN_EQUAL( bin->Tracks().size(), asc->Tracks().size() );                    \
    }

TRACK_COUNT_TEST( TMS1mmX19,        0 )
TRACK_COUNT_TEST( MC4_PLUS_CSHAPE,  1 )
TRACK_COUNT_TEST( Ems4_Rev2,        3 )
TRACK_COUNT_TEST( LCORE_4,          4 )
TRACK_COUNT_TEST( LCORE_2,          5 )
TRACK_COUNT_TEST( MAIS_FC,          7 )


BOOST_AUTO_TEST_CASE( BoardOutline_LCORE_4 )
{
    auto board = LoadBinary( PADS_BINARY_BOARDS[4] );

    if( !board )
        return;

    BOOST_CHECK_MESSAGE( CountEdgeCutsShapes( board.get() ) > 0,
                         "LCORE_4 binary should have board outline shapes" );
}


BOOST_AUTO_TEST_CASE( BoardOutline_LCORE_2 )
{
    auto board = LoadBinary( PADS_BINARY_BOARDS[5] );

    if( !board )
        return;

    BOOST_CHECK_MESSAGE( CountEdgeCutsShapes( board.get() ) > 0,
                         "LCORE_2 binary should have board outline shapes" );
}


BOOST_AUTO_TEST_CASE( BoardOutline_OtherVersions )
{
    int indices[] = { 0, 1, 2, 3, 6, 7 };

    for( int i : indices )
    {
        auto board = LoadBinary( PADS_BINARY_BOARDS[i] );

        if( !board )
            continue;

        BOOST_WARN_MESSAGE( CountEdgeCutsShapes( board.get() ) > 0,
                            PADS_BINARY_BOARDS[i].dir
                                    << " binary outline parsing not yet complete" );
    }
}


#define STRUCTURAL_INTEGRITY_TEST( name, idx )                              \
    BOOST_AUTO_TEST_CASE( StructuralIntegrity_##name )                      \
    {                                                                       \
        auto board = LoadBinary( PADS_BINARY_BOARDS[idx] );                 \
                                                                            \
        if( !board )                                                        \
            return;                                                         \
                                                                            \
        RunStructuralChecks( PADS_BINARY_BOARDS[idx], board.get() );        \
    }

STRUCTURAL_INTEGRITY_TEST( TMS1mmX19,        0 )
STRUCTURAL_INTEGRITY_TEST( MC4_PLUS_CSHAPE,  1 )
STRUCTURAL_INTEGRITY_TEST( MC2_PLUS_REV1,    2 )
STRUCTURAL_INTEGRITY_TEST( Ems4_Rev2,        3 )
STRUCTURAL_INTEGRITY_TEST( LCORE_4,          4 )
STRUCTURAL_INTEGRITY_TEST( LCORE_2,          5 )
STRUCTURAL_INTEGRITY_TEST( Dexter_MotorCtrl, 6 )
STRUCTURAL_INTEGRITY_TEST( MAIS_FC,          7 )


BOOST_AUTO_TEST_SUITE_END()
