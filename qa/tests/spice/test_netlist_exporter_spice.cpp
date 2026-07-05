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

#include <schematic.h>
#include <sch_symbol.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>

#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/utils.h>


// Test-only access to the exporter's private multi-unit helpers.
class NETLIST_EXPORTER_SPICE_PROBE
{
public:
    static std::vector<UNIT_PIN_MAP> CollectUnitPinMaps( NETLIST_EXPORTER_SPICE& aExporter,
                                                         SCH_SYMBOL& aSymbol,
                                                         const SCH_SHEET_PATH& aSheet,
                                                         const wxString& aVariant )
    {
        return aExporter.collectUnitPinMaps( aSymbol, aSheet, aVariant );
    }

    static SIM_DECOMPOSITION GetDecomposition( NETLIST_EXPORTER_SPICE& aExporter,
                                               SCH_SYMBOL& aSymbol, const SCH_SHEET_PATH& aSheet,
                                               const wxString& aVariant )
    {
        return aExporter.getDecomposition( aSymbol, aSheet, aVariant );
    }
};


// Normalizes volatile parts of a SPICE netlist so a checked-in golden file stays
// portable across machines and checkouts.  Only `.include` lines carry absolute
// filesystem paths; everything else (nets, model names, NC counters, pin order)
// is already deterministic.
static wxString normalizeSpiceNetlist( const wxString& aNetlist )
{
    wxString          out;
    wxStringTokenizer lines( aNetlist, wxS( "\n" ), wxTOKEN_RET_EMPTY_ALL );

    while( lines.HasMoreTokens() )
    {
        wxString line = lines.GetNextToken();

        if( line.StartsWith( wxS( ".include " ) ) )
        {
            wxFileName fn( line.AfterFirst( '"' ).BeforeLast( '"' ) );
            line = wxS( ".include \"" ) + fn.GetFullName() + wxS( "\"" );
        }

        out << line << wxS( "\n" );
    }

    return out;
}


// Splits a netlist line into whitespace-separated tokens.
static std::vector<wxString> splitNetlistLine( const wxString& aLine )
{
    std::vector<wxString> tokens;
    wxStringTokenizer     tokenizer( aLine, wxS( " \t" ), wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
        tokens.push_back( tokenizer.GetNextToken() );

    return tokens;
}


// Returns the first unit (in hierarchy traversal order) carrying the given reference, i.e. the
// primary unit the exporter processes.  Mirrors the test's need to drive the private helpers.
static SCH_SYMBOL* findPrimaryUnit( SCHEMATIC* aSchematic, const wxString& aRef,
                                    SCH_SHEET_PATH& aSheetOut )
{
    for( const SCH_SHEET_PATH& sheet : aSchematic->Hierarchy() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &sheet ) == aRef )
            {
                aSheetOut = sheet;
                return symbol;
            }
        }
    }

    return nullptr;
}


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


// Repeat-per-unit decomposition (issue #1779): a multi-unit symbol whose model is a single-unit
// vendor model is netlisted as one outer X<refdes> referencing a generated wrapper .subckt that
// instantiates the base model once per functional unit, with the supply pins shared.
BOOST_AUTO_TEST_CASE( MultiUnitRepeatOpamp )
{
    LOCALE_IO dummy;

    LoadSchematic( SchematicQAPath( wxS( "multiunit_repeat_opamp" ) ) );
    WriteNetlist();

    wxString netlistPath = GetNetlistPath( true );
    wxFFile  netlistFile( netlistPath, "rt" );
    BOOST_REQUIRE( netlistFile.IsOpened() );

    wxString netlist;
    netlistFile.ReadAll( &netlist );
    netlistFile.Close();

    BOOST_TEST_INFO( "Netlist:\n" << netlist );

    wxString              xu1Line;
    wxString              subcktLine;
    std::vector<wxString> innerInstanceLines;
    bool                  hasBaseInclude = false;

    wxStringTokenizer lines( netlist, wxS( "\n" ) );

    while( lines.HasMoreTokens() )
    {
        wxString line = lines.GetNextToken();

        if( line.StartsWith( wxS( "XU1 " ) ) )
        {
            BOOST_REQUIRE_MESSAGE( xu1Line.IsEmpty(), "multiple XU1 lines; expected exactly one" );
            xu1Line = line;
        }
        else if( line.StartsWith( wxS( ".subckt kicad_mu_" ) ) )
        {
            BOOST_REQUIRE_MESSAGE( subcktLine.IsEmpty(),
                                   "wrapper .subckt emitted more than once" );
            subcktLine = line;
        }
        else if( line.StartsWith( wxS( "X" ) ) && line.EndsWith( wxS( "uopamp_single" ) ) )
        {
            innerInstanceLines.push_back( line );
        }
        else if( line.Contains( wxS( "uopamp_single.lib.spice" ) )
                 && line.StartsWith( wxS( ".include" ) ) )
        {
            hasBaseInclude = true;
        }
    }

    BOOST_REQUIRE_MESSAGE( !xu1Line.IsEmpty(), "outer XU1 line was not generated" );
    BOOST_REQUIRE_MESSAGE( !subcktLine.IsEmpty(), "wrapper .subckt was not generated" );
    BOOST_CHECK_MESSAGE( hasBaseInclude, "base model library was not included" );

    std::vector<wxString> subcktTokens = splitNetlistLine( subcktLine );
    BOOST_REQUIRE_GE( subcktTokens.size(), 2u );
    wxString wrapperName = subcktTokens[1];

    // The outer instance references exactly the generated wrapper.
    std::vector<wxString> xu1Tokens = splitNetlistLine( xu1Line );
    BOOST_REQUIRE_GE( xu1Tokens.size(), 2u );
    BOOST_CHECK_EQUAL( xu1Tokens.back(), wrapperName );

    // Two functional units -> two inner instances, both of the single-unit base model.
    BOOST_REQUIRE_EQUAL( innerInstanceLines.size(), 2u );

    std::vector<wxString> x1 = splitNetlistLine( innerInstanceLines[0] );
    std::vector<wxString> x2 = splitNetlistLine( innerInstanceLines[1] );

    // Inner instance node order is +IN -IN VCC VEE OUT; the shared VCC (index 3) and VEE (index 4)
    // nodes must be identical across both instances.
    BOOST_REQUIRE_EQUAL( x1.size(), 7u );
    BOOST_REQUIRE_EQUAL( x2.size(), 7u );
    BOOST_CHECK_EQUAL( x1[3], x2[3] );
    BOOST_CHECK_EQUAL( x1[4], x2[4] );

    // The outer nets follow the synthetic pin order (instance-1 signal pins, instance-2 signal
    // pins, then shared), each connected to the schematic net on that symbol pin.
    const std::vector<wxString> expectedOuterNets = {
        wxS( "Net-_U1A-+_" ),    // +IN1 (symbol pin 3)
        wxS( "Net-_U1A--_" ),    // -IN1 (symbol pin 2)
        wxS( "/out" ),           // OUT1 (symbol pin 1)
        wxS( "Net-_U1B-+_" ),    // +IN2 (symbol pin 5)
        wxS( "Net-_U1A--_" ),    // -IN2 (symbol pin 6, tied to -IN1)
        wxS( "Net-_C2-Pad1_" ),  // OUT2 (symbol pin 7)
        wxS( "Net-_U1C-V+_" ),   // VCC  (symbol pin 8, shared)
        wxS( "Net-_U1C-V-_" )    // VEE  (symbol pin 4, shared)
    };

    std::vector<wxString> outerNets( xu1Tokens.begin() + 1, xu1Tokens.end() - 1 );
    BOOST_CHECK( outerNets == expectedOuterNets );

    Cleanup();
}


// The synthesized wrapper subcircuit must be valid, simulatable SPICE: ngspice has to parse and
// run the generated netlist (a malformed .subckt would fail to parse).  This is the automated
// equivalent of opening the project and running a simulation.
BOOST_AUTO_TEST_CASE( MultiUnitRepeatOpampSimulates )
{
    LOCALE_IO dummy;

    LoadSchematic( SchematicQAPath( wxS( "multiunit_repeat_opamp" ) ) );
    WriteNetlist();

    // CompareNetlists() loads the generated netlist into ngspice, runs it, and asserts there is no
    // parse error ("circuit not parsed") or missing-code-model error.
    CompareNetlists();

    Cleanup();
}


// The per-unit Sim.Pins of a multi-unit symbol are gathered into one map per unit, ordered by
// unit number, with each unit's written token order preserved and repeats across units kept.
BOOST_AUTO_TEST_CASE( CollectUnitPinMaps )
{
    LOCALE_IO dummy;

    LoadSchematic( SchematicQAPath( wxS( "multiunit_repeat_opamp" ) ) );

    SCH_SHEET_PATH primarySheet;
    SCH_SYMBOL*    primary = findPrimaryUnit( m_schematic.get(), wxS( "U1" ), primarySheet );
    BOOST_REQUIRE( primary );

    NETLIST_EXPORTER_SPICE    exporter( m_schematic.get() );
    std::vector<UNIT_PIN_MAP> maps = NETLIST_EXPORTER_SPICE_PROBE::CollectUnitPinMaps(
            exporter, *primary, primarySheet, wxEmptyString );

    BOOST_REQUIRE_EQUAL( maps.size(), 3 );

    BOOST_CHECK_EQUAL( maps[0].unit, 1 );
    BOOST_CHECK_EQUAL( maps[1].unit, 2 );
    BOOST_CHECK_EQUAL( maps[2].unit, 3 );

    const std::vector<std::pair<wxString, wxString>> unit1 = {
        { wxS( "1" ), wxS( "OUT" ) }, { wxS( "2" ), wxS( "-IN" ) }, { wxS( "3" ), wxS( "+IN" ) } };
    const std::vector<std::pair<wxString, wxString>> unit2 = {
        { wxS( "5" ), wxS( "+IN" ) }, { wxS( "6" ), wxS( "-IN" ) }, { wxS( "7" ), wxS( "OUT" ) } };
    const std::vector<std::pair<wxString, wxString>> unit3 = {
        { wxS( "4" ), wxS( "VEE" ) }, { wxS( "8" ), wxS( "VCC" ) } };

    BOOST_CHECK( maps[0].pins == unit1 );
    BOOST_CHECK( maps[1].pins == unit2 );
    BOOST_CHECK( maps[2].pins == unit3 );
}


// With no Sim.Decomposition field, a symbol resolves to the safe whole-device default.
// The component-level Sim.Decomposition is read for the symbol regardless of which unit carries
// it (the fixture places it on a single unit only).
BOOST_AUTO_TEST_CASE( DecompositionReadsRepeat )
{
    LOCALE_IO dummy;

    LoadSchematic( SchematicQAPath( wxS( "multiunit_repeat_opamp" ) ) );

    SCH_SHEET_PATH primarySheet;
    SCH_SYMBOL*    primary = findPrimaryUnit( m_schematic.get(), wxS( "U1" ), primarySheet );
    BOOST_REQUIRE( primary );

    NETLIST_EXPORTER_SPICE exporter( m_schematic.get() );
    SIM_DECOMPOSITION      dec = NETLIST_EXPORTER_SPICE_PROBE::GetDecomposition(
            exporter, *primary, primarySheet, wxEmptyString );

    BOOST_CHECK( dec.mode == SIM_DECOMPOSITION::MODE::REPEAT_PER_UNIT );
    BOOST_REQUIRE_EQUAL( dec.sharedModelPins.size(), 2 );
    BOOST_CHECK_EQUAL( dec.sharedModelPins[0], wxS( "VCC" ) );
    BOOST_CHECK_EQUAL( dec.sharedModelPins[1], wxS( "VEE" ) );
}


// Byte-exact safety rail for the default (whole-device) netlist path.  The
// multi-unit decomposition work (issue #1779) must never perturb the output of
// schematics that use today's behavior; a diff here is a backward-compatibility
// regression and a hard stop.  Regenerate the .golden files (after an
// intentional, reviewed format change) by setting KICAD_RECORD_GOLDEN=1.
BOOST_AUTO_TEST_CASE( WholeDeviceGoldenNetlist )
{
    LOCALE_IO dummy;

    const std::vector<wxString> fixtures = { wxS( "multiunit_pinmap_split" ),
                                             wxS( "fliege_filter" ) };

    for( const wxString& fixture : fixtures )
    {
        BOOST_TEST_CONTEXT( "Fixture: " << fixture )
        {
            LoadSchematic( SchematicQAPath( fixture ) );
            WriteNetlist();

            wxString netlistPath = GetNetlistPath( true );
            wxFFile  netlistFile( netlistPath, "rt" );
            BOOST_REQUIRE( netlistFile.IsOpened() );

            wxString netlist;
            netlistFile.ReadAll( &netlist );
            netlistFile.Close();

            wxString normalized = normalizeSpiceNetlist( netlist );

            wxFileName goldenFn = SchematicQAPath( fixture );
            goldenFn.SetExt( "golden" );
            wxString goldenPath = goldenFn.GetFullPath();

            if( wxGetEnv( wxS( "KICAD_RECORD_GOLDEN" ), nullptr ) )
            {
                wxFFile out( goldenPath, "wt" );
                BOOST_REQUIRE( out.IsOpened() );
                out.Write( normalized );
                out.Close();
                BOOST_TEST_MESSAGE( "Recorded golden netlist: " << goldenPath );
            }
            else
            {
                wxFFile goldenFile( goldenPath, "rt" );
                BOOST_REQUIRE_MESSAGE( goldenFile.IsOpened(),
                                       "Missing golden netlist; regenerate with "
                                       "KICAD_RECORD_GOLDEN=1" );

                wxString golden;
                goldenFile.ReadAll( &golden );
                goldenFile.Close();

                BOOST_TEST_INFO( "Normalized netlist:\n" << normalized );
                BOOST_CHECK_EQUAL( normalized, golden );
            }

            Cleanup();
        }
    }
}


// The simulator builds one SPICE_CIRCUIT_MODEL and exports from it repeatedly, so the model-name
// generator must start each netlist clean.  An externally defined subcircuit name (opamp) must be
// emitted verbatim every time, and a KiCad-defined .model name (the Gummel-Poon NPN) must not drift
// run to run.  Either failure means a reused exporter leaks model-name state across exports.
// The parallel_caps fixture is the schematic from https://gitlab.com/kicad/code/kicad/-/issues/20238
// with C1 replaced by two parallel caps sharing one library diode.  Re-running the simulation
// renamed the included .model reference to DIODE1#1, so the diode became undefined and the sim broke.
BOOST_AUTO_TEST_CASE( RepeatedExportIsDeterministic )
{
    LOCALE_IO dummy;

    const std::vector<wxString> fixtures = { wxS( "opamp" ), wxS( "npn_ce_amp" ), wxS( "parallel_caps" ) };

    for( const wxString& fixture : fixtures )
    {
        BOOST_TEST_CONTEXT( "Fixture: " << fixture )
        {
            LoadSchematic( SchematicQAPath( fixture ) );

            NETLIST_EXPORTER_SPICE exporter( m_schematic.get() );

            auto exportOnce =
                    [&]() -> wxString
                    {
                        wxString           path = GetNetlistPath( true );
                        WX_STRING_REPORTER reporter;

                        BOOST_REQUIRE( exporter.WriteNetlist( path, GetNetlistOptions(), reporter ) );

                        wxFFile  file( path, "rt" );
                        BOOST_REQUIRE( file.IsOpened() );

                        wxString netlist;
                        file.ReadAll( &netlist );
                        file.Close();

                        return netlist;
                    };

            wxString first = exportOnce();
            wxString second = exportOnce();

            BOOST_TEST_INFO( "First export:\n" << first );
            BOOST_TEST_INFO( "Second export:\n" << second );
            BOOST_CHECK_EQUAL( first, second );

            Cleanup();
        }
    }
}


// The model-name uniquifier must record every accepted name so later collisions are detected, and
// Clear() must forget them so a reused exporter starts each netlist clean.
BOOST_AUTO_TEST_CASE( NameGeneratorUniqueness )
{
    NAME_GENERATOR gen;

    std::string first = gen.Generate( "model_A" );
    BOOST_CHECK_EQUAL( first, "model_A" );

    std::string second = gen.Generate( "model_A" );
    BOOST_CHECK_EQUAL( second, "model_A#1" );

    std::string third = gen.Generate( "model_A" );
    BOOST_CHECK_EQUAL( third, "model_A#2" );

    std::string different = gen.Generate( "model_B" );
    BOOST_CHECK_EQUAL( different, "model_B" );

    gen.Clear();

    std::string afterClear = gen.Generate( "model_A" );
    BOOST_CHECK_EQUAL( afterClear, "model_A" );
}


BOOST_AUTO_TEST_SUITE_END()
