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


// Two F.Cu chains, each 30 mm long.  CHAIN_COVERED is shadowed by a B.Cu zone in the
// (0,0)..(30,1) corridor; CHAIN_UNCOVERED is at y=20 with no overlying B.Cu zone.
// Pre-fix the global zone count is 1 board-wide so the return-path check never fires.
// Post-fix the spatial test catches CHAIN_UNCOVERED while leaving CHAIN_COVERED clean.
static const char* BOARD_TEXT = R"KICAD(
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
    (net 1 "/CHAIN_COVERED")
    (net 2 "/CHAIN_UNCOVERED")
    (net 3 "/GND")
    (gr_line (start 0 -5) (end 40 -5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 40 -5) (end 40 30) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 40 30) (end 0 30) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 0 30) (end 0 -5) (layer "Edge.Cuts") (width 0.05))
    (segment (start 0 0) (end 30 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 0 20) (end 30 20) (width 0.2) (layer "F.Cu") (net 2))
    (zone (net 3) (net_name "/GND") (layer "B.Cu") (name "gnd_corridor") (hatch edge 0.508)
        (connect_pads (clearance 0))
        (min_thickness 0.254) (filled_areas_thickness no)
        (fill (thermal_gap 0.508) (thermal_bridge_width 0.508))
        (polygon
            (pts
                (xy -1 -1)
                (xy 31 -1)
                (xy 31 1)
                (xy -1 1)
            )
        )
    )
)
)KICAD";


// length 0..200 mm passes; the only firing constraint is return_path which requires a
// continuous B.Cu reference under the F.Cu chain.
static const char* DRU_TEXT = R"KICAD((version 1)

(rule "ReturnPath"
    (condition "A.NetClass == 'Default'")
    (constraint length (min 0mm) (max 200mm))
    (constraint return_path (layer "B.Cu"))
)
)KICAD";


BOOST_AUTO_TEST_SUITE( DRCReturnPathZoneBBox )


BOOST_AUTO_TEST_CASE( ZoneSpatialFilterFiresForUnshadowedChain )
{
    namespace fs = std::filesystem;

    fs::path tmpDir = fs::temp_directory_path() / "kicad_drc_return_path_bbox";
    fs::create_directories( tmpDir );

    fs::path pcbPath = tmpDir / "ret_path.kicad_pcb";
    fs::path druPath = tmpDir / "ret_path.kicad_dru";

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

    NETINFO_ITEM* coveredNet = board->FindNet( wxS( "/CHAIN_COVERED" ) );
    BOOST_REQUIRE( coveredNet );
    coveredNet->SetNetChain( wxS( "COVERED" ) );

    NETINFO_ITEM* uncoveredNet = board->FindNet( wxS( "/CHAIN_UNCOVERED" ) );
    BOOST_REQUIRE( uncoveredNet );
    uncoveredNet->SetNetChain( wxS( "UNCOVERED" ) );

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

    std::vector<DRC_ITEM> returnPathViolations;

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_NET_CHAIN_RETURN_PATH_BREAK )
                    returnPathViolations.push_back( *aItem );
            } );

    drcEngine->RunTests( EDA_UNITS::MM, true, false );

    // Exactly one chain (UNCOVERED) should report a return-path break.  Pre-fix the
    // board-wide zone count of 1 produced zero violations even though only COVERED is
    // actually shadowed.
    BOOST_CHECK_EQUAL( returnPathViolations.size(), 1u );

    bool coveredFlagged = false;
    bool uncoveredFlagged = false;

    for( const DRC_ITEM& item : returnPathViolations )
    {
        const wxString msg = item.GetErrorMessage( false );

        if( msg.Find( wxS( "COVERED" ) ) != wxNOT_FOUND
            && msg.Find( wxS( "UNCOVERED" ) ) == wxNOT_FOUND )
        {
            coveredFlagged = true;
        }

        if( msg.Find( wxS( "UNCOVERED" ) ) != wxNOT_FOUND )
            uncoveredFlagged = true;
    }

    BOOST_CHECK_MESSAGE( !coveredFlagged,
                         "Return-path break wrongly reported for chain shadowed by a zone" );
    BOOST_CHECK_MESSAGE( uncoveredFlagged,
                         "Return-path break missing for chain with no overlying zone" );

    std::error_code ec;
    fs::remove( pcbPath, ec );
    fs::remove( druPath, ec );
}


BOOST_AUTO_TEST_SUITE_END()
