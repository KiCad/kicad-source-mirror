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


// Two-net board.  CHAIN_NET is 25 mm long and tagged with chain "SIG"; PLAIN_NET is 25 mm long
// and not in any chain.  The .kicad_dru rule matches the default netclass that both nets
// inherit.
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
    (net 1 "/CHAIN_NET")
    (net 2 "/PLAIN_NET")
    (segment (start 0 0) (end 25 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 0 5) (end 25 5) (width 0.2) (layer "F.Cu") (net 2))
)
)KICAD";


// length 0..5 mm fails for any 25 mm net.  net_chain_length 0..200 mm passes for the chain.
// The bug: pre-fix, presence of net_chain_length suppresses the per-net length check on every
// net the rule matches, so neither CHAIN_NET nor PLAIN_NET reports a length violation.
static const char* DRU_TEXT = R"KICAD((version 1)

(rule "FastTopo"
    (condition "A.NetClass == 'Default'")
    (constraint length (min 0mm) (max 5mm))
    (constraint net_chain_length (min 0mm) (max 200mm))
)
)KICAD";


BOOST_AUTO_TEST_SUITE( DRCLengthWithChain )


BOOST_AUTO_TEST_CASE( PerNetLengthFiresAlongsideChainLength )
{
    namespace fs = std::filesystem;

    fs::path tmpDir = fs::temp_directory_path() / "kicad_drc_length_chain";
    fs::create_directories( tmpDir );

    fs::path pcbPath = tmpDir / "len_chain.kicad_pcb";
    fs::path druPath = tmpDir / "len_chain.kicad_dru";

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

    // Tag CHAIN_NET into a signal chain so the chain branch fires; leave PLAIN_NET unchained
    NETINFO_ITEM* chainNet = board->FindNet( wxS( "/CHAIN_NET" ) );
    BOOST_REQUIRE( chainNet );
    chainNet->SetNetChain( wxS( "SIG" ) );

    NETINFO_ITEM* plainNet = board->FindNet( wxS( "/PLAIN_NET" ) );
    BOOST_REQUIRE( plainNet );
    BOOST_CHECK( plainNet->GetNetChain().IsEmpty() );

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

    // Pre-fix behaviour: 0 violations (the chain branch suppresses length on every matched net).
    // Post-fix behaviour: at least one length violation per matched net (CHAIN_NET, PLAIN_NET).
    BOOST_CHECK_MESSAGE( lengthViolations.size() >= 2,
                         "Expected per-net length violations on both chain and plain nets, got "
                                 << lengthViolations.size() );

    bool chainNetFlagged = false;
    bool plainNetFlagged = false;

    for( const DRC_ITEM& item : lengthViolations )
    {
        for( const KIID& id : item.GetIDs() )
        {
            BOARD_ITEM* bi = board->ResolveItem( id );

            if( !bi )
                continue;

            if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( bi ) )
            {
                if( bci->GetNetCode() == chainNet->GetNetCode() )
                    chainNetFlagged = true;

                if( bci->GetNetCode() == plainNet->GetNetCode() )
                    plainNetFlagged = true;
            }
        }
    }

    BOOST_CHECK_MESSAGE( chainNetFlagged, "Length violation missing for chain-bearing net" );
    BOOST_CHECK_MESSAGE( plainNetFlagged, "Length violation missing for non-chain net" );

    std::error_code ec;
    fs::remove( pcbPath, ec );
    fs::remove( druPath, ec );
}


BOOST_AUTO_TEST_SUITE_END()
