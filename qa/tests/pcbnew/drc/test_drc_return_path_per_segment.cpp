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


// Trunk on F.Cu from x=0..30 at y=0.  B.Cu zone covers only x=0..15.  Per-segment
// check should report exactly one return-path break, anchored at the
// uncovered run (around x=22.5).
static const char* PARTIAL_PCB = R"(
(kicad_pcb
    (version 20250904)
    (generator "pcbnew")
    (generator_version "9.99")
    (layers
        (0 "F.Cu" signal)
        (2 "B.Cu" signal)
        (44 "Edge.Cuts" user)
    )
    (net 0 "")
    (net 1 "/CHAIN_PARTIAL")
    (net 2 "/GND")
    (gr_line (start -5 -5) (end 35 -5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 35 -5) (end 35 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 35 5) (end -5 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 5) (end -5 -5) (layer "Edge.Cuts") (width 0.05))
    (segment (start 0 0) (end 15 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 15 0) (end 30 0) (width 0.2) (layer "F.Cu") (net 1))
    (zone (net 2) (net_name "/GND") (layer "B.Cu") (name "gnd_left") (hatch edge 0.508)
        (connect_pads (clearance 0))
        (min_thickness 0.254) (filled_areas_thickness no)
        (fill (thermal_gap 0.508) (thermal_bridge_width 0.508))
        (polygon
            (pts (xy -1 -1) (xy 15 -1) (xy 15 1) (xy -1 1))
        )
    )
)
)";


// Trunk fully off-zone: a single track from (0,0) to (30,0) with a B.Cu
// zone only at x=40..50 — no overlap.  Expect exactly one marker (covering
// the whole track with no zone overlap).
static const char* FULLY_UNSHADOWED_PCB = R"(
(kicad_pcb
    (version 20250904)
    (generator "pcbnew")
    (generator_version "9.99")
    (layers
        (0 "F.Cu" signal)
        (2 "B.Cu" signal)
        (44 "Edge.Cuts" user)
    )
    (net 0 "")
    (net 1 "/CHAIN_FULL")
    (net 2 "/GND")
    (gr_line (start -5 -5) (end 55 -5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 55 -5) (end 55 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 55 5) (end -5 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 5) (end -5 -5) (layer "Edge.Cuts") (width 0.05))
    (segment (start 0 0) (end 30 0) (width 0.2) (layer "F.Cu") (net 1))
    (zone (net 2) (net_name "/GND") (layer "B.Cu") (name "gnd_far") (hatch edge 0.508)
        (connect_pads (clearance 0))
        (min_thickness 0.254) (filled_areas_thickness no)
        (fill (thermal_gap 0.508) (thermal_bridge_width 0.508))
        (polygon
            (pts (xy 40 -1) (xy 50 -1) (xy 50 1) (xy 40 1))
        )
    )
)
)";


static const char* DRU_TEXT = R"((version 1)
(rule "ReturnPath"
    (condition "A.NetClass == 'Default'")
    (constraint length (min 0mm) (max 200mm))
    (constraint return_path (layer "B.Cu"))
)
)";


namespace
{
size_t runReturnPathDrc( const char* aPcb, const wxString& aChainName,
                         const wxString& aChainTag,
                         const std::string& aSubdir )
{
    namespace fs = std::filesystem;
    fs::path tmpDir = fs::temp_directory_path() / aSubdir;
    fs::create_directories( tmpDir );
    fs::path pcbPath = tmpDir / "ret.kicad_pcb";
    fs::path druPath = tmpDir / "ret.kicad_dru";

    {
        std::ofstream out( pcbPath );
        out << aPcb;
    }

    {
        std::ofstream out( druPath );
        out << DRU_TEXT;
    }

    PCB_IO_KICAD_SEXPR     plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    plugin.LoadBoard( pcbPath.string(), board.get() );
    board->BuildConnectivity();

    NETINFO_ITEM* sig = board->FindNet( aChainName );
    BOOST_REQUIRE( sig );
    sig->SetNetChain( aChainTag );

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
    bds.m_DRCSeverities[DRCE_NET_CHAIN_RETURN_PATH_BREAK] = SEVERITY::RPT_SEVERITY_ERROR;

    size_t markerCount = 0;

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_NET_CHAIN_RETURN_PATH_BREAK )
                    markerCount++;
            } );

    drcEngine->RunTests( EDA_UNITS::MM, true, false );

    fs::remove( pcbPath );
    fs::remove( druPath );

    return markerCount;
}
}


BOOST_AUTO_TEST_SUITE( DRCReturnPathPerSegment )


BOOST_AUTO_TEST_CASE( PartiallyShadowedTrunkReportsOneMarker )
{
    size_t n = runReturnPathDrc( PARTIAL_PCB, wxS( "/CHAIN_PARTIAL" ),
                                 wxS( "PARTIAL" ),
                                 "kicad_drc_ret_partial" );
    BOOST_CHECK_EQUAL( n, 1u );
}


BOOST_AUTO_TEST_CASE( FullyUnshadowedTrunkReportsOneMarker )
{
    size_t n = runReturnPathDrc( FULLY_UNSHADOWED_PCB, wxS( "/CHAIN_FULL" ),
                                 wxS( "FULL" ),
                                 "kicad_drc_ret_full" );
    BOOST_CHECK_EQUAL( n, 1u );
}


BOOST_AUTO_TEST_SUITE_END()
