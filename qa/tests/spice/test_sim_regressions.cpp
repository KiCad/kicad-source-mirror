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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/results_collector.hpp> // To check if the current test failed (to be moved?).
#include <eeschema_test_utils.h>
#include <test_netlist_exporter_spice.h>
#include <wx/ffile.h>
#include <mock_pgm_base.h>
#include <locale_io.h>


class TEST_SIM_REGRESSIONS_FIXTURE : public TEST_NETLIST_EXPORTER_SPICE_FIXTURE
{
public:
    TEST_SIM_REGRESSIONS_FIXTURE() :
            TEST_NETLIST_EXPORTER_SPICE_FIXTURE()
    {
    }

    ~TEST_SIM_REGRESSIONS_FIXTURE()
    {
    }

    wxFileName SchematicQAPath( const wxString& aBaseName ) override
    {
        wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
        fn.SetName( aBaseName );
        fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        return fn;
    }

    wxString GetNetlistPath( bool aTest = false ) override
    {
        wxFileName netFile = m_schematic->Project().GetProjectFullName();

        if( aTest )
            netFile.SetName( netFile.GetName() + "_test" );

        netFile.SetExt( "spice" );
        return netFile.GetFullPath();
    }

    unsigned GetNetlistOptions() override
    {
        unsigned options = NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS
                            | NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS
                            | NETLIST_EXPORTER_SPICE::OPTION_SIM_COMMAND;

        if( m_SaveCurrents )
            options |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;

        if( m_SaveVoltages )
            options |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;

        if( m_SavesDissipations )
            options |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS;

        return options;
    }

public:
    bool  m_SavesDissipations = false;
    bool  m_SaveVoltages      = true;
    bool  m_SaveCurrents      = true;
};


BOOST_FIXTURE_TEST_CASE( WindowsPaths, TEST_SIM_REGRESSIONS_FIXTURE )
{
    LOCALE_IO dummy;

    // const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    // MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "issue13591" );
    TestTranPoint( 100e-6, { { "I(R1)", 0 }, { "I(R2)", 0 } }, 0.00001 );
    TestTranPoint( 500e-6, { { "I(R1)", 0 }, { "I(R2)", 0 } }, 0.00001 );
}


BOOST_FIXTURE_TEST_CASE( ImmediateSBCKT, TEST_SIM_REGRESSIONS_FIXTURE )
{
    LOCALE_IO dummy;

    m_SaveCurrents = false;

    TestNetlist( "issue13431" );
    TestTranPoint( 0.005, { { "V(/soft_start)", 2.489 } } );
    TestTranPoint( 0.012, { { "V(/soft_start)", 5.100 } } );
}


// This test is flaky and fails on ngspice-42 / Linux.
// Please replace it with something that is more stable
#if 0
BOOST_FIXTURE_TEST_CASE( LegacyFixups, TEST_SIM_REGRESSIONS_FIXTURE )
{
    LOCALE_IO dummy;

    const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "issue13112" );
    TestTranPoint( 0.01, { { "V(out)", -0.060 } } );
    TestTranPoint( 0.02, { { "V(out)", 0.856 } } );
}
#endif


BOOST_FIXTURE_TEST_CASE( DualNMOSAmp, TEST_SIM_REGRESSIONS_FIXTURE )
{
    LOCALE_IO dummy;

    // const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    // MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "issue13162" );
    TestTranPoint( 0.030, { { "V(out)", 0.000829682 } } );
    TestTranPoint( 0.035, { { "V(out)", -0.000829692 } } );
}


BOOST_FIXTURE_TEST_CASE( VariableSubstitutions, TEST_SIM_REGRESSIONS_FIXTURE )
{
    LOCALE_IO dummy;

    // const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    // MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "issue12505" );
    TestTranPoint( 0.015, { { "V(Net-_R1-Pad2_)", -311 } } );
    TestTranPoint( 0.025, { { "V(Net-_R1-Pad2_)", 311 } } );
}


BOOST_FIXTURE_TEST_CASE( IBISSim, TEST_SIM_REGRESSIONS_FIXTURE )
{
    LOCALE_IO dummy;

    // const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    // MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "issue16223" );
    TestTranPoint( 0.0, { { "V(PRBS_OUTPUT)", 5.114 } } );
    TestTranPoint( 1e-6, { { "V(PRBS_OUTPUT)", -0.1144 } } );
}

