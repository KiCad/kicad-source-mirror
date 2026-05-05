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
#include <string>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <netinfo.h>
#include <pcb_marker.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


// These tests pin down the bridging-length contribution of multi-pad chain footprints
// in the matched-length DRC.  History:
//
// fd87668d29 introduced gather-then-skip behavior: any 3+ pad chain-member footprint
// silently contributed zero bridging length, which the comment block in the provider
// claimed to be guarding against.  MR review C-2 flagged that as the bug it was trying
// to prevent.
//
// Post C-2 fix the bridging contribution is the maximum pairwise cross-net pad span
// across the chain-member pads on the footprint.  This is the most generous (upper
// bound) bridge such a device could create, keeps the contribution non-zero, and
// scales identically to any pad count >= 2.


namespace
{

static const char* DRU_HEADER = "(version 1)\n";


std::string makeChainBudgetRule( const wxString& aName, const wxString& aMinMM,
                                 const wxString& aMaxMM )
{
    return wxString::Format( "%s(rule \"%s\"\n"
                             "    (condition \"A.NetClass == 'Default'\")\n"
                             "    (constraint net_chain_length (min %smm) (max %smm))\n"
                             ")\n",
                             DRU_HEADER, aName, aMinMM, aMaxMM )
            .ToStdString();
}


// Build a temp board file plus a DRU file, run the matched-length DRC, return the count of
// DRCE_LENGTH_OUT_OF_RANGE markers raised on the chain "SIG".  Both nets named "/NET_*" are
// auto-tagged into chain "SIG".
size_t runChainLengthDrc( const std::string& aBoardText, const std::string& aRuleText,
                          const std::string& aTmpSubdir )
{
    namespace fs = std::filesystem;

    fs::path tmpDir = fs::temp_directory_path() / aTmpSubdir;
    fs::create_directories( tmpDir );

    fs::path pcbPath = tmpDir / "chain_bridge.kicad_pcb";
    fs::path druPath = tmpDir / "chain_bridge.kicad_dru";

    {
        std::ofstream pcbOut( pcbPath );
        pcbOut << aBoardText;
    }

    {
        std::ofstream druOut( druPath );
        druOut << aRuleText;
    }

    PCB_IO_KICAD_SEXPR     plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    plugin.LoadBoard( pcbPath.string(), board.get() );
    board->BuildConnectivity();

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( net && net->GetNetname().StartsWith( wxS( "/NET_" ) ) )
            net->SetNetChain( wxS( "SIG" ) );
    }

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

    size_t lengthViolations = 0;

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_LENGTH_OUT_OF_RANGE )
                    ++lengthViolations;
            } );

    drcEngine->RunTests( EDA_UNITS::MM, true, false );

    std::error_code ec;
    fs::remove( pcbPath, ec );
    fs::remove( druPath, ec );

    return lengthViolations;
}

}  // namespace


BOOST_AUTO_TEST_SUITE( DRCChainBridgingMultipad )


// 3-pad footprint, 2 nets (e.g. ferrite bead with centre tap).  Pads at x = -2.5, 0, +2.5.
// Nets A, A, B.  Track copper = 20 mm (10 mm on NET_A + 10 mm on NET_B).  Cross-net pad
// pairs span max 5 mm (-2.5 NET_A to +2.5 NET_B).  Total bridged chain = 25 mm.
BOOST_AUTO_TEST_CASE( ThreePadTwoNetFootprintContributesMaxPairwiseSpan )
{
    static const char* boardText = R"KICAD(
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

    // 26 mm budget passes (chain ~25 mm).
    size_t pass =
            runChainLengthDrc( boardText, makeChainBudgetRule( "Pass", "0", "26" ),
                               "kicad_drc_chain_bridging_3pad_pass" );
    BOOST_CHECK_MESSAGE( pass == 0, "Expected no length violation under 26 mm budget, got "
                                            << pass );

    // 22 mm budget fails: pre-fix would silently drop bridging and report ~20 mm.  Post-fix
    // the 5 mm max pairwise span is included.
    size_t fail =
            runChainLengthDrc( boardText, makeChainBudgetRule( "Fail", "0", "22" ),
                               "kicad_drc_chain_bridging_3pad_fail" );
    BOOST_CHECK_MESSAGE( fail == 1, "Expected one length violation under 22 mm budget, got "
                                            << fail );
}


// 3-pad footprint, 3 nets (transformer-style).  Pads at x = -2.5, 0, +2.5 on nets A, B, C.
// Track copper = 30 mm split evenly.  Max cross-net pairwise span = 5 mm.  Pre-fix the whole
// footprint was silently skipped because chainPads.size() > 2 broke the gather loop.
BOOST_AUTO_TEST_CASE( ThreePadThreeNetFootprintContributesMaxPairwiseSpan )
{
    static const char* boardText = R"KICAD(
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
    (net 3 "/NET_C")
    (segment (start 0 0)  (end 10 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 15 0) (end 25 0) (width 0.2) (layer "F.Cu") (net 2))
    (segment (start 30 0) (end 40 0) (width 0.2) (layer "F.Cu") (net 3))
    (footprint "TestFP:XFMR_3PAD" (layer "F.Cu") (at 12.5 0)
        (property "Reference" "T1")
        (pad "1" smd rect (at -2.5 0) (size 0.5 0.5) (layers "F.Cu") (net 1 "/NET_A"))
        (pad "2" smd rect (at 0 0)    (size 0.5 0.5) (layers "F.Cu") (net 2 "/NET_B"))
        (pad "3" smd rect (at 2.5 0)  (size 0.5 0.5) (layers "F.Cu") (net 3 "/NET_C"))
    )
)
)KICAD";

    // Single T1 footprint gives 30 mm copper + 5 mm bridging = 35 mm.  Budget 36 mm passes.
    size_t pass =
            runChainLengthDrc( boardText, makeChainBudgetRule( "Pass", "0", "36" ),
                               "kicad_drc_chain_bridging_3net_pass" );
    BOOST_CHECK_MESSAGE( pass == 0, "Expected no length violation under 36 mm budget, got "
                                            << pass );

    // 32 mm budget fails: pre-fix the 3-pad-3-net footprint was silently dropped (reporting
    // 30 mm copper).  Post-fix the 5 mm max pairwise span pushes total to 35 mm.
    size_t fail =
            runChainLengthDrc( boardText, makeChainBudgetRule( "Fail", "0", "32" ),
                               "kicad_drc_chain_bridging_3net_fail" );
    BOOST_CHECK_MESSAGE( fail == 1, "Expected one length violation under 32 mm budget, got "
                                            << fail );
}


// 4-pad footprint, 2 nets (e.g. dual-winding chip with split netting).  Pads at
// x = -3, -1, +1, +3 on nets A, A, B, B.  Track copper = 20 mm.  Cross-net pad pairs:
// (-3 A, +1 B) = 4, (-3 A, +3 B) = 6, (-1 A, +1 B) = 2, (-1 A, +3 B) = 4.  Max = 6 mm.
BOOST_AUTO_TEST_CASE( FourPadFootprintContributesMaxPairwiseSpan )
{
    static const char* boardText = R"KICAD(
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
    (segment (start 0 0)  (end 10 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 16 0) (end 26 0) (width 0.2) (layer "F.Cu") (net 2))
    (footprint "TestFP:DUAL_4PAD" (layer "F.Cu") (at 13 0)
        (property "Reference" "U1")
        (pad "1" smd rect (at -3 0) (size 0.5 0.5) (layers "F.Cu") (net 1 "/NET_A"))
        (pad "2" smd rect (at -1 0) (size 0.5 0.5) (layers "F.Cu") (net 1 "/NET_A"))
        (pad "3" smd rect (at 1 0)  (size 0.5 0.5) (layers "F.Cu") (net 2 "/NET_B"))
        (pad "4" smd rect (at 3 0)  (size 0.5 0.5) (layers "F.Cu") (net 2 "/NET_B"))
    )
)
)KICAD";

    // Chain total: 20 mm copper + 6 mm bridging = 26 mm.  Budget 27 mm passes.
    size_t pass =
            runChainLengthDrc( boardText, makeChainBudgetRule( "Pass", "0", "27" ),
                               "kicad_drc_chain_bridging_4pad_pass" );
    BOOST_CHECK_MESSAGE( pass == 0, "Expected no length violation under 27 mm budget, got "
                                            << pass );

    // 24 mm budget fails: pre-fix dropped bridging, post-fix adds 6 mm max pairwise span.
    size_t fail =
            runChainLengthDrc( boardText, makeChainBudgetRule( "Fail", "0", "24" ),
                               "kicad_drc_chain_bridging_4pad_fail" );
    BOOST_CHECK_MESSAGE( fail == 1, "Expected one length violation under 24 mm budget, got "
                                            << fail );
}


// 3-pad footprint, single net.  All pads on NET_A.  No cross-net bridging; must contribute 0.
BOOST_AUTO_TEST_CASE( ThreePadSingleNetFootprintContributesZero )
{
    static const char* boardText = R"KICAD(
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
    (segment (start 20 0) (end 30 0) (width 0.2) (layer "F.Cu") (net 2))
    (footprint "TestFP:STAR_3PAD" (layer "F.Cu") (at 5 5)
        (property "Reference" "U2")
        (pad "1" smd rect (at -2 0) (size 0.5 0.5) (layers "F.Cu") (net 1 "/NET_A"))
        (pad "2" smd rect (at 0 0)  (size 0.5 0.5) (layers "F.Cu") (net 1 "/NET_A"))
        (pad "3" smd rect (at 2 0)  (size 0.5 0.5) (layers "F.Cu") (net 1 "/NET_A"))
    )
)
)KICAD";

    // Pure copper sum is 20 mm with no bridging contribution.  Budget 21 mm passes.
    size_t pass =
            runChainLengthDrc( boardText, makeChainBudgetRule( "Pass", "0", "21" ),
                               "kicad_drc_chain_bridging_singlenet_pass" );
    BOOST_CHECK_MESSAGE( pass == 0, "Single-net 3-pad footprint must not contribute bridging; "
                                    "got " << pass << " violations under 21 mm budget" );
}


BOOST_AUTO_TEST_SUITE_END()
