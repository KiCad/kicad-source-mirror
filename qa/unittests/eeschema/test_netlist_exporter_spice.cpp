/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifdef KICAD_SPICE

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/results_collector.hpp> // To check if the current test failed (to be moved?).
#include <eeschema_test_utils.h>
#include <netlist_exporter_spice.h>
#include <sim/ngspice.h>
#include <sim/spice_reporter.h>
#include <wx/ffile.h>
#include <mock_pgm_base.h>


class TEST_NETLIST_EXPORTER_SPICE_FIXTURE : public TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_SPICE>
{
public:
    class SPICE_TEST_REPORTER : public SPICE_REPORTER
    {
    public:
        SPICE_TEST_REPORTER( std::shared_ptr<wxString> aLog ) : m_log( std::move( aLog ) ) {}

        REPORTER& Report( const wxString& aText,
                          SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
        {
            *m_log << aText << "\n";

            // You can add a debug trace here.
            return *this;
        }

        bool HasMessage() const override { return false; }

        void OnSimStateChange( SPICE_SIMULATOR* aObject, SIM_STATE aNewState ) override
        {
        }

    private:
        std::shared_ptr<wxString> m_log;
    };

    TEST_NETLIST_EXPORTER_SPICE_FIXTURE() :
            TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_SPICE>(),
            m_simulator( SPICE_SIMULATOR::CreateInstance( "ngspice" ) ),
            m_log( std::make_shared<wxString>() ),
            m_reporter( std::make_unique<SPICE_TEST_REPORTER>( m_log ) ),
            m_abort( false )
    {
    }

    ~TEST_NETLIST_EXPORTER_SPICE_FIXTURE()
    {
        using namespace boost::unit_test;

        test_case::id_t id = framework::current_test_case().p_id;
        test_results    results = results_collector.results( id );

        // Output a log if the test has failed.
        BOOST_CHECK_MESSAGE( results.passed(), "\nNGSPICE LOG\n===========\n" << *m_log );
    }

    wxFileName GetSchematicPath( const wxString& aBaseName ) override
    {
        wxFileName fn = KI_TEST::GetEeschemaTestDataDir();
        fn.AppendDir( "spice_netlists" );
        fn.AppendDir( aBaseName );
        fn.SetName( aBaseName );
        fn.SetExt( KiCadSchematicFileExtension );

        return fn;
    }

    wxString GetNetlistPath( bool aTest = false ) override
    {
        wxFileName netFile = m_schematic.Prj().GetProjectFullName();

        if( aTest )
            netFile.SetName( netFile.GetName() + "_test" );

        netFile.SetExt( "spice" );
        return netFile.GetFullPath();
    }

    void CompareNetlists() override
    {
        m_abort = false;

        // Our simulator is actually Ngspice.
        NGSPICE* ngspice = dynamic_cast<NGSPICE*>( m_simulator.get() );
        BOOST_REQUIRE( ngspice );

        ngspice->SetReporter( m_reporter.get() );

        wxFFile file( GetNetlistPath( true ), "rt" );
        wxString netlist;

        file.ReadAll( &netlist );

        //ngspice->Init();
        ngspice->Command( "set ngbehavior=ps" );
        ngspice->Command( "setseed 1" );
        BOOST_REQUIRE( ngspice->LoadNetlist( netlist.ToStdString() ) );
        BOOST_REQUIRE( ngspice->Run() );

        // Test if ngspice cannot run a simulation (missing code models).
        // in this case the log contains "MIF-ERROR" and/or "Error: circuit not parsed"
        // when the simulation is not run the spice command "linearize" crashes.
        bool err_found = m_log->Find( wxT( "Error: circuit not parsed" ) ) != wxNOT_FOUND
                         || m_log->Find( wxT( "MIF-ERROR" ) ) != wxNOT_FOUND;

        if( err_found )
        {
            if( m_log->Find( wxT( "MIF-ERROR" ) ) != wxNOT_FOUND )
                wxLogWarning( wxT( "Cannot run ngspice. test skipped. Missing code model files?" ) );
            else
                wxLogWarning( wxT( "Cannot run ngspice. test skipped. Install error?" ) );

            m_abort = true;

            return;
        }

        // We need to make sure that the number of points always the same.
        ngspice->Command( "linearize" );


        // Debug info.

        // Display all vectors.
        ngspice->Command( "echo Available Vectors" );
        ngspice->Command( "echo -----------------" );
        ngspice->Command( "display" );

        // Display the original netlist.
        *m_log << "Original Netlist\n";
        *m_log << "----------------\n";
        *m_log << netlist << "\n";

        // Display the expanded netlist.
        ngspice->Command( "echo Expanded Netlist" );
        ngspice->Command( "echo ----------------" );
        ngspice->Command( "listing runnable" );
    }

    void TestNetlist( const wxString& aBaseName )
    {
        // We actually ended up only generating the netlist here.
        TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_SPICE>::TestNetlist( aBaseName );
    }

    void TestPoint( const std::string& aXVectorName, double aXValue,
                    const std::map<const std::string, double> aTestVectorsAndValues,
                    double aMaxRelError = 1e-2 )
    {
        // The default aMaxRelError is fairly large because we have some problems with determinism
        // in QA pipeline. We don't need to fix this for now because, if this has to be fixed in
        // the first place, this would have to be done from Ngspice's side.

        BOOST_TEST_CONTEXT( "X vector name: " << aXVectorName << ", X value: " << aXValue )
        {
            NGSPICE* ngspice = static_cast<NGSPICE*>( m_simulator.get() );

            std::vector<double> xVector = ngspice->GetRealPlot( aXVectorName );
            std::size_t i = 0;

            for(; i < xVector.size(); ++i )
            {
                double inf = std::numeric_limits<double>::infinity();

                double leftDelta = ( aXValue - ( i >= 1 ? xVector[i - 1] : -inf ) );
                double middleDelta = ( aXValue - xVector[i] );
                double rightDelta = ( aXValue - ( i < xVector.size() - 1 ? xVector[i + 1] : inf ) );

                // Check if this point is the closest one.
                if( abs( middleDelta ) <= abs( leftDelta )
                    && abs( middleDelta ) <= abs( rightDelta ) )
                {
                    break;
                }
            }

            BOOST_REQUIRE_LT( i, xVector.size() );

            for( auto&& [vectorName, refValue] : aTestVectorsAndValues )
            {
                std::vector<double> yVector = ngspice->GetMagPlot( vectorName );

                BOOST_REQUIRE_GE( yVector.size(), i + 1 );

                BOOST_TEST_CONTEXT( "Y vector name: " << vectorName
                                    << ", Ref value: " << refValue
                                    << ", Actual value: " << yVector[i] )
                {
                    double maxError = abs( refValue * aMaxRelError );
                    if( maxError == 0 )
                    {
                        // If refValue is 0, we need a obtain the max. error differently.
                        maxError = aMaxRelError;
                    }

                    BOOST_CHECK_LE( abs( yVector[i] - refValue ), maxError );
                }
            }
        }
    }

    void TestTranPoint( double aTime,
                        const std::map<const std::string, double> aTestVectorsAndValues,
                        double aMaxAbsError = 1e-2 )
    {
        TestPoint( "time", aTime, aTestVectorsAndValues, aMaxAbsError );
    }

    void TestACPoint( double aFrequency,
                      const std::map<const std::string, double> aTestVectorsAndValues,
                      double aMaxAbsError = 1e-2 )
    {
        TestPoint( "frequency", aFrequency, aTestVectorsAndValues, aMaxAbsError );
    }

    wxString GetResultsPath( bool aTest = false )
    {
        wxFileName netlistPath( GetNetlistPath( aTest ) );
        netlistPath.SetExt( "csv" );

        return netlistPath.GetFullPath();
    }

    unsigned GetNetlistOptions() override
    {
        return NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES
               | NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS
               | NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS;
    }

    std::shared_ptr<SPICE_SIMULATOR>     m_simulator;
    std::shared_ptr<wxString>            m_log;
    std::unique_ptr<SPICE_TEST_REPORTER> m_reporter;
    bool m_abort;       // set to true to force abort durint a test
};


BOOST_FIXTURE_TEST_SUITE( NetlistExporterSpice, TEST_NETLIST_EXPORTER_SPICE_FIXTURE )


BOOST_AUTO_TEST_CASE( Rectifier )
{
    const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "rectifier" );
    TestTranPoint( 0, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 9250e-6, { { "V(/in)", 5 }, { "V(/out)", 4.26 } } );
    TestTranPoint( 10e-3, { { "V(/in)", 0 }, { "V(/out)", 4.24 } } );
}

// FIXME: Fails due to some nondeterminism, seems related to convergence problems.

/*BOOST_AUTO_TEST_CASE( Chirp )
{
    TestNetlist( "chirp", { "V(/out)", "I(R1)" } );
}*/


BOOST_AUTO_TEST_CASE( Opamp )
{
    const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "opamp" );
    TestTranPoint( 0, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 250e-6, { { "V(/in)", 500e-3 }, { "V(/out)", 1 } } );
    TestTranPoint( 500e-6, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 750e-6, { { "V(/in)", -500e-3 }, { "V(/out)", -1 } } );
    TestTranPoint( 1e-3, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
}


BOOST_AUTO_TEST_CASE( NpnCeAmp )
{
    // This test intentionally uses non-inferred voltage sources to test them.

    TestNetlist( "npn_ce_amp" );
    TestTranPoint( 900e-6, { { "V(/in)", 0 }, { "V(/out)", 5.32 } } );
    TestTranPoint( 925e-6, { { "V(/in)", 10e-3 }, { "V(/out)", 5.30 } } );
    TestTranPoint( 950e-6, { { "V(/in)", 0 }, { "V(/out)", 5.88 } } );
    TestTranPoint( 975e-6, { { "V(/in)", -10e-3 }, { "V(/out)", 5.91 } } );
    TestTranPoint( 1e-3, { { "V(/in)", 0 }, { "V(/out)", 5.32 } } );
}

// Incomplete. TODO.

BOOST_AUTO_TEST_CASE( Rlc )
{
    TestNetlist( "rlc" );
    TestTranPoint( 9.43e-3, { { "V(/Vp)", -19e-3 }, { "I(Rs1)", 19e-3 } } );
    TestTranPoint( 9.74e-3, { { "V(/Vp)", 19e-3 }, { "I(Rs1)", -19e-3 } } );
}


BOOST_AUTO_TEST_CASE( Tlines )
{
    TestNetlist( "tlines" );
    TestTranPoint( 910e-6, { { "V(/z0_in)", 1 }, { "V(/z0_out)", 0 },
                             { "V(/rlgc_in)", 1 }, { "V(/rlgc_out)", 0 } } );
    TestTranPoint( 970e-6, { { "V(/z0_in)", 0 }, { "V(/z0_out)", 1 },
                             { "V(/rlgc_in)", 0 }, { "V(/rlgc_out)", 1 } } );
}


/*BOOST_AUTO_TEST_CASE( Sources )
{
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
    // We test a multi-unit part here, as Fliege topology uses two op amps (power supply pins are a
    // third part).

    TestNetlist( "fliege_filter" );
    TestACPoint( 0.8e3, { { "V(/in)", 1 }, { "V(/out)", 1 } } );
    TestACPoint( 1.061e3, { { "V(/in)", 1 }, { "V(/out)", 0 } } );
    TestACPoint( 1.2e3, { { "V(/in)", 1 }, { "V(/out)", 1 } } );
}


BOOST_AUTO_TEST_CASE( LegacyLaserDriver )
{
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


BOOST_AUTO_TEST_CASE( LegacyPspice )
{
    TestNetlist( "legacy_pspice" );
    TestACPoint( 190, { { "V(/VIN)", pow( 10, -186e-3 / 20 ) },
                        { "V(VOUT)", pow( 10, 87e-3 / 20 ) } } );
}


BOOST_AUTO_TEST_CASE( LegacyRectifier )
{
    TestNetlist( "legacy_rectifier" );
    TestTranPoint( 0, { { "V(/signal_in)", 0 },
                        { "V(/rect_out)", 0 } } );
    TestTranPoint( 9.75e-3, { { "V(/signal_in)", 1.5 },
                              { "V(/rect_out)", 823e-3 } } );
}


BOOST_AUTO_TEST_CASE( LegacySallenKey )
{
    TestNetlist( "legacy_sallen_key" );

    if( m_abort )
        return;

    TestACPoint( 1, { { "V(/lowpass)", pow( 10, 0.0 / 20 ) } } );
    TestACPoint( 1e3, { { "V(/lowpass)", pow( 10, -2.9 / 20 ) } } );
}


/*BOOST_AUTO_TEST_CASE( LegacySources )
{
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
    TestNetlist( "legacy_opamp" );
    TestTranPoint( 0, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 250e-6, { { "V(/in)", 500e-3 }, { "V(/out)", 1 } } );
    TestTranPoint( 500e-6, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
    TestTranPoint( 750e-6, { { "V(/in)", -500e-3 }, { "V(/out)", -1 } } );
    TestTranPoint( 1e-3, { { "V(/in)", 0 }, { "V(/out)", 0 } } );
}


BOOST_AUTO_TEST_SUITE_END()

#endif // KICAD_SPICE
