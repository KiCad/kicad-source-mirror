/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/results_collector.hpp> // To check if the current test failed (to be moved?).
#include <test_netlist_exporter_spice.h>
#include <sim/simulator_reporter.h>
#include <mock_pgm_base.h>
#include <locale_io.h>

#include <wx/tokenzr.h>


BOOST_FIXTURE_TEST_SUITE( NetlistExporterSpice, TEST_NETLIST_EXPORTER_SPICE_FIXTURE )


BOOST_AUTO_TEST_CASE( Rectifier )
{
    LOCALE_IO dummy;

    // const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    // MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "rectifier" );
    TestTranPoint( 0, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 9250e-6, { { "V(/in)", 5 }, { "V(/out)", 4.26 } } );
    TestTranPoint( 10e-3, { { "V(/in)", 0 }, { "V(/out)", 4.24 } } );
}

// FIXME: Fails due to some nondeterminism, seems related to convergence problems.

/*BOOST_AUTO_TEST_CASE( Chirp )
{
    LOCALE_IO dummy;
    TestNetlist( "chirp", { "V(/out)", "I(R1)" } );
}*/


BOOST_AUTO_TEST_CASE( Opamp )
{
    LOCALE_IO dummy;
    // Instead of Simulation_SPICE:OPAMP, we use Amplifier_Operational:MCP6001-OT because its pins
    // are not ordered by pin numbers, which is a possible failure condition.

    // const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    // MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "opamp" );
    TestTranPoint( 0, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 250e-6, { { "V(/in)", 500e-3 }, { "V(/out)", 1 } } );
    TestTranPoint( 500e-6, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 750e-6, { { "V(/in)", -500e-3 }, { "V(/out)", -1 } } );
    TestTranPoint( 1e-3, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
}


BOOST_AUTO_TEST_CASE( NpnCeAmp )
{
    LOCALE_IO dummy;
    // This test intentionally uses non-inferred voltage sources to test them.

    TestNetlist( "npn_ce_amp" );
    TestTranPoint( 900e-6, { { "V(/in)", 0 }, { "V(/out)", 5.32 } } );
    TestTranPoint( 925e-6, { { "V(/in)", 10e-3 }, { "V(/out)", 5.30 } } );
    TestTranPoint( 950e-6, { { "V(/in)", 0 }, { "V(/out)", 5.88 } } );
    TestTranPoint( 975e-6, { { "V(/in)", -10e-3 }, { "V(/out)", 5.91 } } );
    TestTranPoint( 1e-3, { { "V(/in)", 0 }, { "V(/out)", 5.32 } } );
}


BOOST_AUTO_TEST_CASE( Rlc )
{
    LOCALE_IO dummy;
    TestNetlist( "rlc" );
    TestTranPoint( 9.43e-3, { { "V(/Vp)", -19e-3 }, { "I(Rs1)", 19e-3 } } );
    TestTranPoint( 9.74e-3, { { "V(/Vp)", 19e-3 }, { "I(Rs1)", -19e-3 } } );
}


BOOST_AUTO_TEST_CASE( Potentiometers )
{
    TestNetlist( "potentiometers" );
    TestOpPoint( 0.5, "V(/out1)" );
    TestOpPoint( 0.7, "V(/out2)" );
    TestOpPoint( 0.9, "V(/out3)" );
}


BOOST_AUTO_TEST_CASE( Tlines )
{
    LOCALE_IO dummy;
    TestNetlist( "tlines" );
    TestTranPoint( 910e-6, { { "V(/z0_in)", 1 }, { "V(/z0_out)", 0 },
                             { "V(/rlgc_in)", 1 }, { "V(/rlgc_out)", 0 } } );
    TestTranPoint( 970e-6, { { "V(/z0_in)", 0 }, { "V(/z0_out)", 1 },
                             { "V(/rlgc_in)", 0 }, { "V(/rlgc_out)", 1 } } );
}


/*BOOST_AUTO_TEST_CASE( Sources )
{
    LOCALE_IO dummy;
    TestNetlist( "sources", { "V(/vdc)", "V(/idc)",
                              "V(/vsin)", "V(/isin)",
                              "V(/vpulse)", "V(/ipulse)",
                              "V(/vexp)", "V(/iexp)",
                              "V(/vpwl)", "V(/ipwl)",
                              "V(/vbehavioral)", "V(/ibehavioral)" } );

    // TODO: Make some tests for random and noise sources, e.g. check their RMS or spectra.
    //"V(/vwhitenoise)", "V(/iwhitenoise)",
    //"V(/vpinknoise)", "V(/ipinknoise)",
    //"V(/vburstnoise)", "V(/iburstnoise)",
    //"V(/vranduniform)", "V(/iranduniform)",
    //"V(/vrandnormal)", "V(/iranduniform)",
    //"V(/vrandexp)", "V(/irandexp)",
}*/


BOOST_AUTO_TEST_CASE( CmosNot )
{
    LOCALE_IO dummy;
    TestNetlist( "cmos_not" );
    TestTranPoint( 0, { { "V(/in)", 2.5 }, { "V(/out)", 2.64 } } );
    TestTranPoint( 250e-6, { { "V(/in)", 5 }, { "V(/out)", 0.013 } } );
    TestTranPoint( 500e-6, { { "V(/in)", 2.5 }, { "V(/out)", 2.64 } } );
    TestTranPoint( 750e-6, { { "V(/in)", 0 }, { "V(/out)", 5 } } );
    TestTranPoint( 1e-3, { { "V(/in)", 2.5 }, { "V(/out)", 2.64 } } );
}


/*BOOST_AUTO_TEST_CASE( InstanceParams )
{
    // TODO.
    //TestNetlist( "instance_params", {} );
}*/


BOOST_AUTO_TEST_CASE( FliegeFilter )
{
    LOCALE_IO dummy;
    // We test a multi-unit part here, as Fliege topology uses two op amps (power supply pins are a
    // third part).

    TestNetlist( "fliege_filter" );
    TestACPoint( 0.8e3, { { "V(/in)", 1 }, { "V(/out)", 1 } } );
    TestACPoint( 1.061e3, { { "V(/in)", 1 }, { "V(/out)", 0 } } );
    TestACPoint( 1.2e3, { { "V(/in)", 1 }, { "V(/out)", 1 } } );
}


BOOST_AUTO_TEST_CASE( Switches )
{
    LOCALE_IO dummy;
    TestNetlist( "switches" );
    TestTranPoint( 0.5e-3, { { "V(/inswv)", 0 }, { "V(/outswv)", 0 } } );
    TestTranPoint( 1.5e-3, { { "V(/inswv)", 1 }, { "V(/outswv)", 5 } } );
    TestTranPoint( 2.5e-3, { { "V(/inswv)", 0 }, { "V(/outswv)", 0 } } );

    // TODO: Current switch, when it's fixed in Ngspice.
}


// This test is sometimes failing on certain platforms for unknown reasons
// Disabling it for now so that it doesn't prevent packages from building
#if 0
BOOST_AUTO_TEST_CASE( Directives )
{
    LOCALE_IO dummy;
    TestNetlist( "directives" );
    TestTranPoint( 9.25e-3, { { "V(/in)", 1 }, { "V(/out)", -900e-3 }, { "I(XR1)", 1e-3 } } );
    TestTranPoint( 9.50e-3, { { "V(/in)", 0 }, { "V(/out)", 0 }, { "I(XR1)", 1e-3 } } );
    TestTranPoint( 9.75e-3, { { "V(/in)", -1 }, { "V(/out)", 900e-3 }, { "I(XR1)", 1e-3 } } );
    TestTranPoint( 10e-3,    { { "V(/in)", 0 }, { "V(/out)", 0 }, { "I(XR1)", 1e-3 } } );
}
#endif


// This test is flaky and fails on ngspice-42 / Linux.
// Please replace it with something that is more stable
#if 0
BOOST_AUTO_TEST_CASE( LegacyLaserDriver )
{
    LOCALE_IO dummy;

    TestNetlist( "legacy_laser_driver" );

    if( m_abort )
        return;

    // Test D1 current before the pulse
    TestTranPoint( 95e-9, { { "I(D1)", 0 } } );
    // Test D1 current during the pulse
    TestTranPoint( 110e-9, { { "I(D1)", 0.770 } }, 0.1 );
    // Test D1 current after the pulse
    TestTranPoint( 150e-9, { { "I(D1)", 0 } } );
}
#endif

BOOST_AUTO_TEST_CASE( LegacyPotentiometer )
{
    TestNetlist( "legacy_pot" );
    TestOpPoint( 0.5, "V(/out1)" );
    TestOpPoint( 0.7, "V(/out2)" );
    TestOpPoint( 0.9, "V(/out3)" );
}

BOOST_AUTO_TEST_CASE( LegacyPspice )
{
    LOCALE_IO dummy;
    TestNetlist( "legacy_pspice" );
    TestACPoint( 190, { { "V(/VIN)", pow( 10, -186e-3 / 20 ) },
                        { "V(VOUT)", pow( 10, 87e-3 / 20 ) } } );
}


BOOST_AUTO_TEST_CASE( LegacyRectifier )
{
    LOCALE_IO dummy;
    TestNetlist( "legacy_rectifier" );
    TestTranPoint( 0, { { "V(/signal_in)", 0 },
                        { "V(/rect_out)", 0 } } );
    TestTranPoint( 9.75e-3, { { "V(/signal_in)", 1.5 },
                              { "V(/rect_out)", 823e-3 } } );
}


BOOST_AUTO_TEST_CASE( LegacySallenKey )
{
    LOCALE_IO dummy;
    TestNetlist( "legacy_sallen_key" );

    if( m_abort )
        return;

    TestACPoint( 1, { { "V(/lowpass)", pow( 10, 0.0 / 20 ) } } );
    TestACPoint( 1e3, { { "V(/lowpass)", pow( 10, -2.9 / 20 ) } } );
}


/*BOOST_AUTO_TEST_CASE( LegacySources )
{
    LOCALE_IO dummy;
    TestNetlist( "legacy_sources", { "V(/vam)", "V(/iam)",
                                     "V(/vdc)", "V(/idc)",
                                     "V(/vexp)", "V(/iexp)",
                                     "V(/vpulse)", "V(/ipulse)",
                                     "V(/vpwl)", "V(/ipwl)",
                                     "V(/vsffm)", "V(/isffm)",
                                     "V(/vsin)", "V(/isin)" } );
                                     //"V(/vtrnoise)", "V(/itrnoise)",
                                     //"V(/vtrrandom)", "V(/itrrandom)" } );
}*/


BOOST_AUTO_TEST_CASE( LegacyOpamp )
{
    LOCALE_IO dummy;
    // Amplifier_Operational:AD797 model is used to test symbols that have more pins than the model.

    TestNetlist( "legacy_opamp" );
    TestTranPoint( 0, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 250e-6, { { "V(/in)", 500e-3 }, { "V(/out)", 1 } } );
    TestTranPoint( 500e-6, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 750e-6, { { "V(/in)", -500e-3 }, { "V(/out)", -1 } } );
    TestTranPoint( 1e-3, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/1779
// Multi-unit symbols should emit a single subcircuit instance whose pin map
// is the union of the Sim.Pins fields collected from every unit, even when
// each individual unit only carries a partial Sim.Pins mapping.
BOOST_AUTO_TEST_CASE( MultiUnitSplitPinMap )
{
    LOCALE_IO dummy;

    LoadSchematic( SchematicQAPath( "multiunit_pinmap_split" ) );
    WriteNetlist();

    wxString netlistPath = GetNetlistPath( true );
    wxFFile  netlistFile( netlistPath, "rt" );
    BOOST_REQUIRE( netlistFile.IsOpened() );

    wxString netlist;
    netlistFile.ReadAll( &netlist );
    netlistFile.Close();

    BOOST_TEST_INFO( "Netlist:\n" << netlist );

    wxString          xu1Line;
    wxStringTokenizer lines( netlist, wxS( "\n" ) );

    while( lines.HasMoreTokens() )
    {
        wxString line = lines.GetNextToken();

        if( line.StartsWith( wxS( "XU1 " ) ) )
        {
            BOOST_REQUIRE_MESSAGE( xu1Line.IsEmpty(),
                                   "Multiple XU1 lines found; multi-unit symbols must emit a "
                                   "single subcircuit instance, not one per unit" );
            xu1Line = line;
        }
    }

    BOOST_REQUIRE_MESSAGE( !xu1Line.IsEmpty(),
                           "XU1 subcircuit instance was not generated" );

    wxStringTokenizer     tokens( xu1Line, wxS( " \t" ), wxTOKEN_STRTOK );
    std::vector<wxString> parts;

    while( tokens.HasMoreTokens() )
        parts.push_back( tokens.GetNextToken() );

    // The uopamp_lvl2_2x subckt declares its nodes in the order
    // VCC VEE +IN1 -IN1 OUT1 +IN2 -IN2 OUT2. The merged Sim.Pins maps every model
    // node to a specific symbol pin number, so each position in the instance line is
    // determined by the schematic's connection on that symbol pin. Validating the
    // full token list catches regressions in any unit's merge behavior independently.
    const std::vector<wxString> expectedParts = {
        wxS( "XU1" ),
        wxS( "Net-_U1C-V+_" ),   // VCC <- symbol pin 8 (unit 3 V+)
        wxS( "Net-_U1C-V-_" ),   // VEE <- symbol pin 4 (unit 3 V-)
        wxS( "Net-_U1A-+_" ),    // +IN1 <- symbol pin 3 (unit 1 +IN)
        wxS( "Net-_U1A--_" ),    // -IN1 <- symbol pin 2 (unit 1 -IN)
        wxS( "/out" ),           // OUT1 <- symbol pin 1 (unit 1 OUT, labeled /out)
        wxS( "Net-_U1B-+_" ),    // +IN2 <- symbol pin 5 (unit 2 +IN)
        wxS( "Net-_U1A--_" ),    // -IN2 <- symbol pin 6 (unit 2 -IN, tied to unit 1 -IN)
        wxS( "Net-_C2-Pad1_" ),  // OUT2 <- symbol pin 7 (unit 2 OUT)
        wxS( "uopamp_lvl2_2x" )
    };

    BOOST_REQUIRE_EQUAL( parts.size(), expectedParts.size() );

    for( size_t ii = 0; ii < expectedParts.size(); ++ii )
    {
        BOOST_CHECK_MESSAGE( parts[ii] == expectedParts[ii],
                             "Expected XU1 token " << ii << " to be '" << expectedParts[ii]
                             << "', got '" << parts[ii]
                             << "'. Per-unit Sim.Pins merge is missing or incorrect" );
    }

    Cleanup();
}


BOOST_AUTO_TEST_SUITE_END()
