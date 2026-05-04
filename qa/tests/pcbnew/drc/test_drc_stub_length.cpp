/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 */

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_marker.h>
#include <pcb_track.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


// Classifier-only fixture: the stub-length check today picks trunk/stub by
// terminal-pad ownership, not by routed topology, so the three nets need only
// be tagged into the same chain and given individually routed traces. Pads
// UA-pad1 and UC-pad1 are assigned as the chain's terminal pads, leaving
// MID_B as the only chain member without an endpoint assignment. Once real
// topological stub detection lands the segments will need to actually meet at
// series components.
static const char* BOARD_TEXT = R"KICAD(
(kicad_pcb
    (version 20250904)
    (generator "pcbnew")
    (generator_version "9.99")
    (layers
        (0 "F.Cu" signal)
        (2 "B.Cu" signal)
    )
    (net 0 "")
    (net 1 "/TRUNK_A")
    (net 2 "/MID_B")
    (net 3 "/TRUNK_C")
    (footprint "lib:UA"
        (layer "F.Cu")
        (uuid "11111111-1111-1111-1111-111111111111")
        (at 0 0)
        (pad "1" smd rect
            (at 0 0)
            (size 1 1)
            (layers "F.Cu")
            (uuid "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaa01")
            (net 1 "/TRUNK_A")
        )
    )
    (footprint "lib:UC"
        (layer "F.Cu")
        (uuid "33333333-3333-3333-3333-333333333333")
        (at 40 0)
        (pad "1" smd rect
            (at 0 0)
            (size 1 1)
            (layers "F.Cu")
            (uuid "cccccccc-cccc-cccc-cccc-cccccccccc01")
            (net 3 "/TRUNK_C")
        )
    )
    (segment (start 0 0) (end 2 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 5 0) (end 35 0) (width 0.2) (layer "F.Cu") (net 2))
    (segment (start 38 0) (end 40 0) (width 0.2) (layer "F.Cu") (net 3))
)
)KICAD";


// stub_length 0..5 mm. The 30 mm middle net is well over budget; the 2 mm
// endpoints are within budget *and* are protected by trunk membership anyway.
static const char* DRU_TEXT = R"KICAD((version 1)

(rule "StubBudget"
    (condition "A.NetClass == 'Default'")
    (constraint stub_length (min 0mm) (max 5mm))
)
)KICAD";


BOOST_AUTO_TEST_SUITE( DRCStubLength )


BOOST_AUTO_TEST_CASE( StubLengthFiresOnIntermediateNetOnly )
{
    namespace fs = std::filesystem;

    fs::path tmpDir = fs::temp_directory_path() / "kicad_drc_stub_length";
    fs::create_directories( tmpDir );

    fs::path pcbPath = tmpDir / "stub_length.kicad_pcb";
    fs::path druPath = tmpDir / "stub_length.kicad_dru";

    {
        std::ofstream pcbOut( pcbPath );
        pcbOut << BOARD_TEXT;
    }

    {
        std::ofstream druOut( druPath );
        druOut << DRU_TEXT;
    }

    PCB_IO_KICAD_SEXPR     plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    plugin.LoadBoard( pcbPath.string(), board.get() );
    board->BuildConnectivity();

    NETINFO_ITEM* netA = board->FindNet( wxS( "/TRUNK_A" ) );
    NETINFO_ITEM* netB = board->FindNet( wxS( "/MID_B" ) );
    NETINFO_ITEM* netC = board->FindNet( wxS( "/TRUNK_C" ) );

    BOOST_REQUIRE( netA );
    BOOST_REQUIRE( netB );
    BOOST_REQUIRE( netC );

    netA->SetNetChain( wxS( "SIG" ) );
    netB->SetNetChain( wxS( "SIG" ) );
    netC->SetNetChain( wxS( "SIG" ) );

    PAD* padA = nullptr;
    PAD* padC = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetNetCode() == netA->GetNetCode() )
                padA = pad;

            if( pad->GetNetCode() == netC->GetNetCode() )
                padC = pad;
        }
    }

    BOOST_REQUIRE( padA );
    BOOST_REQUIRE( padC );

    netA->SetTerminalPad( 0, padA );
    netC->SetTerminalPad( 1, padC );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    auto       drcEngine = std::make_shared<DRC_ENGINE>( board.get(), &bds );
    wxFileName ruleFile( druPath.string() );
    drcEngine->InitEngine( ruleFile );
    bds.m_DRCEngine = drcEngine;

    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DANGLING_VIA] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DRILL_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_NET_CHAIN_STUB_TOO_LONG] = SEVERITY::RPT_SEVERITY_ERROR;

    std::vector<wxString> stubMessages;

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_NET_CHAIN_STUB_TOO_LONG )
                    stubMessages.push_back( aItem->GetErrorMessage( false ) );
            } );

    drcEngine->RunTests( EDA_UNITS::MM, true, false );

    auto matchesNet = [&]( const wxString& netName )
    {
        for( const wxString& msg : stubMessages )
        {
            if( msg.Contains( wxString::Format( wxS( "'%s'" ), netName ) ) )
                return true;
        }

        return false;
    };

    bool aFlagged = matchesNet( netA->GetNetname() );
    bool bFlagged = matchesNet( netB->GetNetname() );
    bool cFlagged = matchesNet( netC->GetNetname() );

    BOOST_CHECK_MESSAGE( bFlagged,
                         "Intermediate stub net MID_B should fire stub_length violation" );
    BOOST_CHECK_MESSAGE( !aFlagged,
                         "Trunk endpoint net TRUNK_A (owns terminal_pad_0) must not fire" );
    BOOST_CHECK_MESSAGE( !cFlagged,
                         "Trunk endpoint net TRUNK_C (owns terminal_pad_1) must not fire" );

    std::error_code ec;
    fs::remove( pcbPath, ec );
    fs::remove( druPath, ec );
}


BOOST_AUTO_TEST_CASE( StubLengthQuietOnTwoNetChain )
{
    namespace fs = std::filesystem;

    fs::path tmpDir = fs::temp_directory_path() / "kicad_drc_stub_length";
    fs::create_directories( tmpDir );

    fs::path pcbPath = tmpDir / "stub_length_2net.kicad_pcb";
    fs::path druPath = tmpDir / "stub_length_2net.kicad_dru";

    // Two-net chain: every member owns a terminal pad assignment, so no member
    // is a stub. With the 30 mm trace on each net, a per-net stub_length would
    // fire if the trunk classifier wrongly excluded one of them.
    static const char* TWO_NET_BOARD = R"KICAD(
(kicad_pcb
    (version 20250904)
    (generator "pcbnew")
    (generator_version "9.99")
    (layers
        (0 "F.Cu" signal)
        (2 "B.Cu" signal)
    )
    (net 0 "")
    (net 1 "/A")
    (net 2 "/B")
    (footprint "lib:UA"
        (layer "F.Cu")
        (uuid "44444444-4444-4444-4444-444444444444")
        (at 0 0)
        (pad "1" smd rect
            (at 0 0)
            (size 1 1)
            (layers "F.Cu")
            (uuid "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaa02")
            (net 1 "/A")
        )
    )
    (footprint "lib:UB"
        (layer "F.Cu")
        (uuid "55555555-5555-5555-5555-555555555555")
        (at 60 0)
        (pad "1" smd rect
            (at 0 0)
            (size 1 1)
            (layers "F.Cu")
            (uuid "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbb02")
            (net 2 "/B")
        )
    )
    (segment (start 0 0) (end 30 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 30 0) (end 60 0) (width 0.2) (layer "F.Cu") (net 2))
)
)KICAD";

    static const char* TWO_NET_DRU = R"KICAD((version 1)

(rule "StubBudget"
    (condition "A.NetClass == 'Default'")
    (constraint stub_length (min 0mm) (max 5mm))
)
)KICAD";

    {
        std::ofstream pcbOut( pcbPath );
        pcbOut << TWO_NET_BOARD;
    }

    {
        std::ofstream druOut( druPath );
        druOut << TWO_NET_DRU;
    }

    PCB_IO_KICAD_SEXPR     plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    plugin.LoadBoard( pcbPath.string(), board.get() );
    board->BuildConnectivity();

    NETINFO_ITEM* netA = board->FindNet( wxS( "/A" ) );
    NETINFO_ITEM* netB = board->FindNet( wxS( "/B" ) );

    BOOST_REQUIRE( netA );
    BOOST_REQUIRE( netB );

    netA->SetNetChain( wxS( "SIG" ) );
    netB->SetNetChain( wxS( "SIG" ) );

    PAD* padA = nullptr;
    PAD* padB = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetNetCode() == netA->GetNetCode() )
                padA = pad;

            if( pad->GetNetCode() == netB->GetNetCode() )
                padB = pad;
        }
    }

    BOOST_REQUIRE( padA );
    BOOST_REQUIRE( padB );

    netA->SetTerminalPad( 0, padA );
    netB->SetTerminalPad( 1, padB );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    auto       drcEngine = std::make_shared<DRC_ENGINE>( board.get(), &bds );
    wxFileName ruleFile( druPath.string() );
    drcEngine->InitEngine( ruleFile );
    bds.m_DRCEngine = drcEngine;

    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DANGLING_VIA] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DRILL_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_NET_CHAIN_STUB_TOO_LONG] = SEVERITY::RPT_SEVERITY_ERROR;

    int stubCount = 0;

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_NET_CHAIN_STUB_TOO_LONG )
                    ++stubCount;
            } );

    drcEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_CHECK_MESSAGE( stubCount == 0,
                         "Two-net chain with both ends owning terminal pads must not "
                         "fire any stub_length violation, got "
                                 << stubCount );

    std::error_code ec;
    fs::remove( pcbPath, ec );
    fs::remove( druPath, ec );
}


BOOST_AUTO_TEST_SUITE_END()
