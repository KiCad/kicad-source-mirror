/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <board.h>
#include <pcb_track.h>
#include <netinfo.h>
#include <pad.h>
#include <footprint.h>
#include <router/pns_kicad_iface.h>
#include <filesystem>
#include <fstream>

static const long long MM = 1000000LL; // internal units

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
	(net 1 "Net-(R1-Pad1)")
	(net 2 "Net-(R1-Pad2)")
	(net 3 "Net-(R2-Pad2)")
	(net 4 "Net-(R3-Pad2)")
	(segment (start 17.015 0.005) (end 17.005 -0.005) (width 0.2) (layer "F.Cu") (net 3))
	(segment (start 15.355 -0.005) (end 15.345 0.005) (width 0.2) (layer "F.Cu") (net 2))
	(segment (start 30 0.125) (end 29.75 -0.125) (width 0.2) (layer "F.Cu") (net 4))
	(segment (start 22.15 0) (end 30 0) (width 0.2) (layer "F.Cu") (net 4))
	(segment (start 8 -0.1) (end 7.8 0.1) (width 0.2) (layer "F.Cu") (net 1))
	(segment (start 9.65 0) (end 15.355 0) (width 0.2) (layer "F.Cu") (net 2))
	(footprint "MountingHole:MountingHole_2.2mm_M2_DIN965_Pad" (layer "F.Cu") (at 0 0)
		(property "Reference" "TP2")
		(pad "1" thru_hole circle (at 0 0) (size 3.8 3.8) (drill 2.2) (layers "*.Cu" "*.Mask") (net 1 "Net-(R1-Pad1)"))
	)
	(segment (start 17.005 0) (end 20.5 0) (width 0.2) (layer "F.Cu") (net 3))
	(footprint "MountingHole:MountingHole_2.2mm_M2_DIN965_Pad" (layer "F.Cu") (at 30 0)
		(property "Reference" "TP1")
		(pad "1" thru_hole circle (at 0 0) (size 3.8 3.8) (drill 2.2) (layers "*.Cu" "*.Mask") (net 4 "Net-(R3-Pad2)"))
	)
	(segment (start 0 0) (end 8 0) (width 0.2) (layer "F.Cu") (net 1))
	(footprint "Resistor_SMD:R_0603_1608Metric" (layer "F.Cu") (at 8.825 0)
		(property "Reference" "R1")
		(pad "1" smd roundrect (at -0.825 0) (size 0.8 0.95) (layers "F.Cu" "F.Mask" "F.Paste") (net 1 "Net-(R1-Pad1)"))
		(pad "2" smd roundrect (at 0.825 0) (size 0.8 0.95) (layers "F.Cu" "F.Mask" "F.Paste") (net 2 "Net-(R1-Pad2)"))
	)
	(footprint "Resistor_SMD:R_0603_1608Metric" (layer "F.Cu") (at 16.18 0)
		(property "Reference" "R2")
		(pad "1" smd roundrect (at -0.825 0) (size 0.8 0.95) (layers "F.Cu" "F.Mask" "F.Paste") (net 2 "Net-(R1-Pad2)"))
		(pad "2" smd roundrect (at 0.825 0) (size 0.8 0.95) (layers "F.Cu" "F.Mask" "F.Paste") (net 3 "Net-(R2-Pad2)"))
	)
	(footprint "Resistor_SMD:R_0603_1608Metric" (layer "F.Cu") (at 21.325 0)
		(property "Reference" "R3")
		(pad "1" smd roundrect (at -0.825 0) (size 0.8 0.95) (layers "F.Cu" "F.Mask" "F.Paste") (net 3 "Net-(R2-Pad2)"))
		(pad "2" smd roundrect (at 0.825 0) (size 0.8 0.95) (layers "F.Cu" "F.Mask" "F.Paste") (net 4 "Net-(R3-Pad2)"))
	)
)
)KICAD";

BOOST_AUTO_TEST_SUITE( SignalTotalLengthSexpr )

BOOST_AUTO_TEST_CASE( SignalAggregateMatchesPadSpacing )
{
    PCB_IO_KICAD_SEXPR plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    auto tmpFile = std::filesystem::temp_directory_path() / "net_chain_total_length_sexpr.kicad_pcb";
    {
        std::ofstream ofs( tmpFile );
        ofs << BOARD_TEXT;
    }
    plugin.LoadBoard( tmpFile.string(), board.get() );
    board->BuildConnectivity();

    // Assign signal name
    for( NETINFO_ITEM* net : board->GetNetInfo() )
        if( net->GetNetCode() > 0 ) net->SetNetChain( wxS("Signal1") );

    // Map net -> two pads (terminal pads) using pad net codes
    std::map<int, std::vector<PAD*>> netPads;
    for( FOOTPRINT* fp : board->Footprints() )
        for( PAD* pad : fp->Pads() )
            if( pad->GetNetCode() > 0 ) netPads[ pad->GetNetCode() ].push_back( pad );

    for( auto& [code, pads] : netPads )
    {
        if( pads.size() >= 2 )
        {
            NETINFO_ITEM* net = board->FindNet( code );
            net->SetTerminalPad( 0, pads[0] );
            net->SetTerminalPad( 1, pads[1] );
        }
    }

    NETINFO_ITEM* n1 = board->FindNet( 1 );
    BOOST_REQUIRE( n1 );

    PNS_KICAD_IFACE_BASE ifaceBase; ifaceBase.SetBoard( board.get() );
    long long extraLen = 0, extraDelay = 0;
    bool ok = ifaceBase.GetSignalAggregate( n1, n1, extraLen, extraDelay );
    BOOST_CHECK( ok );

	// Compute actual routed length (with wiggles) and linear span for net1
	long long net1TrackLen = 0;
	bool havePoint = false; long long minX = 0, maxX = 0;
	for( BOARD_ITEM* bi : board->Tracks() )
		if( auto tr = dynamic_cast<PCB_TRACK*>( bi ) )
			if( tr->GetNetCode() == 1 )
			{
				net1TrackLen += ( tr->GetStart() - tr->GetEnd() ).EuclideanNorm();
				long long sx = tr->GetStart().x; long long ex = tr->GetEnd().x;
				if( !havePoint ) { minX = std::min( sx, ex ); maxX = std::max( sx, ex ); havePoint = true; }
				else { minX = std::min( minX, std::min( sx, ex ) ); maxX = std::max( maxX, std::max( sx, ex ) ); }
			}
	long long net1Span = havePoint ? ( maxX - minX ) : 0;
	BOOST_CHECK( net1Span > 0 );
	BOOST_CHECK( net1TrackLen >= net1Span ); // jog increases physical path

	// Chain layout: TP2(0,0) — net1 — R1 — net2 — R2 — net3 — R3 — net4 — TP1(30,0)
	//
	// Copper per net (tracks + jog segments):
	//   net1: 8.000 + 0.283 = 8.283 mm
	//   net2: 5.705 + 0.014 = 5.719 mm
	//   net3: 3.495 + 0.014 = 3.509 mm
	//   net4: 7.850 + 0.354 = 8.204 mm
	//   Total copper: 25.715 mm
	//
	// Bridging through resistors (pad-to-pad, not copper):
	//   R1: 1.65 mm, R2: 1.65 mm, R3: 1.65 mm = 4.95 mm
	//
	// Full chain: 25.715 + 4.95 = 30.665 mm
	//
	// GetSignalAggregate returns copper only (no bridging).
	// extraLen = net2 + net3 + net4 copper = ~17.432 mm
	long long totalCopper = net1TrackLen + extraLen;
	long long expectedCopper = 25 * MM;
	long long padSpacing = 30 * MM;
	BOOST_CHECK_MESSAGE( totalCopper >= expectedCopper,
		"RoutedTotal=" << totalCopper << " expected>=" << expectedCopper
		<< " net1Track=" << net1TrackLen << " extra=" << extraLen );
	BOOST_CHECK( totalCopper < padSpacing ); // copper alone is less than pad spacing

	// Compute bridging: pad-to-pad distance through each 2-net series component
	long long bridging = 0;
	for( FOOTPRINT* fp : board->Footprints() )
	{
		std::map<int, PAD*> chainPads;
		for( PAD* pad : fp->Pads() )
		{
			NETINFO_ITEM* pn = pad->GetNet();
			if( pn && !pn->GetNetChain().IsEmpty() )
				chainPads.emplace( pn->GetNetCode(), pad );
		}
		if( chainPads.size() == 2 )
		{
			auto it = chainPads.begin();
			PAD* p1 = it->second; ++it; PAD* p2 = it->second;
			bridging += ( p1->GetCenter() - p2->GetCenter() ).EuclideanNorm();
		}
	}
	BOOST_CHECK( bridging > 0 );

	// Full chain length (copper + bridging) must cover the pad spacing
	long long fullChain = totalCopper + bridging;
	long long tol = 1 * MM;
	BOOST_CHECK_MESSAGE( fullChain >= padSpacing && fullChain < padSpacing + tol,
		"FullChain=" << fullChain << " padSpacing=" << padSpacing
		<< " copper=" << totalCopper << " bridging=" << bridging );
}

BOOST_AUTO_TEST_SUITE_END()
