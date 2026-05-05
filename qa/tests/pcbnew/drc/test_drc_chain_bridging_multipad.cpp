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
#include <netinfo.h>
#include <pcb_marker.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


// Chain net layout
// ----------------
// Two nets, NET_A and NET_B, are tagged into the same chain "SIG".  A single 3-pad footprint
// FB1 has two pads on NET_A and one on NET_B, simulating a ferrite bead with a center tap (or
// a transformer winding terminating on the same net).  Total copper length is 20 mm; the
// physical pad-to-pad span across the footprint is ~5 mm.
//
// Pre-fix (netcode-keyed map): netsInFp.size() == 2, so the bridging path picks the first pad
// emplaced for each netcode and contributes ~5 mm.  Total chain length reports as ~25 mm.
//
// Post-fix (pad-counting): chainPads.size() == 3 on FB1, so bridging contribution is 0 and
// the chain length reports as 20 mm.
//
// A net_chain_length budget of 22 mm catches the ambiguous pre-fix case as a length violation
// and lets the post-fix case pass.
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
    (net 1 "/NET_A")
    (net 2 "/NET_B")
    (segment (start 0 0) (end 10 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 15 0) (end 25 0) (width 0.2) (layer "F.Cu") (net 2))
    (footprint "TestFP:FB_3PAD" (layer "F.Cu") (at 12.5 0)
        (property "Reference" "FB1")
        (pad "1" smd rect (at -2.5 0) (size 0.5 0.5) (layers "F.Cu") (net 1 "/NET_A"))
        (pad "2" smd rect (at 0 0)    (size 0.5 0.5) (layers "F.Cu") (net 1 "/NET_A"))
        (pad "3" smd rect (at 2.5 0)  (size 0.5 0.5) (layers "F.Cu") (net 2 "/NET_B"))
    )
)
)KICAD";


static const char* DRU_TEXT = R"KICAD((version 1)

(rule "ChainBudget"
    (condition "A.NetClass == 'Default'")
    (constraint net_chain_length (min 0mm) (max 22mm))
)
)KICAD";


BOOST_AUTO_TEST_SUITE( DRCChainBridgingMultipad )


BOOST_AUTO_TEST_CASE( ThreePadFootprintIsExcludedFromBridging )
{
    namespace fs = std::filesystem;

    fs::path tmpDir = fs::temp_directory_path() / "kicad_drc_chain_bridging_multipad";
    fs::create_directories( tmpDir );

    fs::path pcbPath = tmpDir / "chain_bridge.kicad_pcb";
    fs::path druPath = tmpDir / "chain_bridge.kicad_dru";

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

    NETINFO_ITEM* netA = board->FindNet( wxS( "/NET_A" ) );
    NETINFO_ITEM* netB = board->FindNet( wxS( "/NET_B" ) );
    BOOST_REQUIRE( netA );
    BOOST_REQUIRE( netB );

    netA->SetNetChain( wxS( "SIG" ) );
    netB->SetNetChain( wxS( "SIG" ) );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    auto drcEngine = std::make_shared<DRC_ENGINE>( board.get(), &bds );
    wxFileName ruleFile( druPath.string() );
    drcEngine->InitEngine( ruleFile );
    bds.m_DRCEngine = drcEngine;

    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DANGLING_VIA] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DRILL_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LENGTH_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_ERROR;

    std::vector<DRC_ITEM> lengthViolations;

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_LENGTH_OUT_OF_RANGE )
                    lengthViolations.push_back( *aItem );
            } );

    drcEngine->RunTests( EDA_UNITS::MM, true, false );

    // Post-fix: bridging contribution from the 3-pad footprint must be skipped, so the chain
    // total is 20 mm copper and stays under the 22 mm budget.  Pre-fix, an ambiguous 5 mm pad
    // pair is silently added and the chain reports ~25 mm, raising a length violation.
    BOOST_CHECK_MESSAGE( lengthViolations.empty(),
                         "Expected no length violation with bridging skipped on multi-pad-same-net "
                         "footprint, got " << lengthViolations.size() );

    std::error_code ec;
    fs::remove( pcbPath, ec );
    fs::remove( druPath, ec );
}


BOOST_AUTO_TEST_SUITE_END()
