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
#include <netlist_exporter_spice.h>
#include <sim/ngspice.h>
#include <sim/simulator_reporter.h>
#include <wx/ffile.h>
#include <mock_pgm_base.h>
#include <locale_io.h>

// A relative max error accepted when comparing 2 values
#define MAX_DEFAULT_REL_ERROR 2e-2


class TEST_NETLIST_EXPORTER_SPICE_FIXTURE : public TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_SPICE>
{
public:
    class SPICE_TEST_REPORTER : public SIMULATOR_REPORTER
    {
    public:
        SPICE_TEST_REPORTER( std::shared_ptr<wxString> aLog ) :
                m_log( std::move( aLog ) )
        {}

        REPORTER& Report( const wxString& aText,
                          SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
        {
            *m_log << aText << "\n";

            // You can add a debug trace here.
            return *this;
        }

        bool HasMessage() const override { return false; }

        void OnSimStateChange( SIMULATOR* aObject, SIM_STATE aNewState ) override { }

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
        // Don't use BOOST_CHECK_MESSAGE because it triggers a checkpoint which affects debugging
        if( !results.passed() )
        {
            BOOST_TEST_MESSAGE( "\nNGSPICE LOG\n===========\n" << *m_log );
        }
    }

    wxFileName SchematicQAPath( const wxString& aBaseName ) override
    {
        wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
        fn.AppendDir( "spice_netlists" );
        fn.AppendDir( aBaseName );
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

    void CompareNetlists() override
    {
        wxString netlistPath = GetNetlistPath( true );
        BOOST_TEST_CHECKPOINT( "Comparing netlist " << netlistPath );

        m_abort = false;

        // Our simulator is actually Ngspice.
        NGSPICE* ngspice = dynamic_cast<NGSPICE*>( m_simulator.get() );
        BOOST_REQUIRE( ngspice );

        ngspice->SetReporter( m_reporter.get() );

        wxFFile  file( netlistPath, "rt" );
        wxString netlist;

        BOOST_REQUIRE( file.IsOpened() );
        file.ReadAll( &netlist );

        //ngspice->Init();
        ngspice->Command( "set ngbehavior=ps" );
        ngspice->Command( "setseed 1" );
        BOOST_REQUIRE( ngspice->LoadNetlist( std::string( netlist.ToUTF8() ) ) );

        if( ngspice->Run() )
        {
            // wait for end of simulation.
            // calling wxYield() allows printing activity, and stopping ngspice from GUI
            // Also note: do not user wxSafeYield, because when using it we cannot stop
            // ngspice from the GUI
            do
            {
                wxMilliSleep( 50 );
                wxYield();
            } while( ngspice->IsRunning() );
        }

        // Test if ngspice cannot run a simulation (missing code models).
        // in this case the log contains "MIF-ERROR" and/or "Error: circuit not parsed"
        // when the simulation is not run the spice command "linearize" crashes.
        bool mif_error = m_log->Find( wxT( "MIF-ERROR" ) ) != wxNOT_FOUND;

        BOOST_TEST_INFO( "Cannot run ngspice. test skipped. Missing code model files?" );
        BOOST_CHECK( !mif_error );

        bool err_found = m_log->Find( wxT( "Error: circuit not parsed" ) ) != wxNOT_FOUND;

        BOOST_TEST_INFO( "Cannot run ngspice. test skipped. Install error?" );
        BOOST_CHECK( !err_found );

        if( mif_error || err_found )
        {
            m_abort = true;

            // Still display the original netlist in this case.
            *m_log << "Original Netlist\n";
            *m_log << "----------------\n";
            *m_log << netlist << "\n";

            return;
        }

        // We need to make sure that the number of points always the same.
        ngspice->Command( "linearize" );


        // Debug info.

        // Display all vectors.
        *m_log << "\n";
        ngspice->Command( "echo Available Vectors" );
        ngspice->Command( "echo -----------------" );
        ngspice->Command( "display" );

        // Display the original netlist.
        *m_log << "\n";
        *m_log << "Original Netlist\n";
        *m_log << "----------------\n";
        *m_log << netlist << "\n";

        // Display the expanded netlist.
        ngspice->Command( "echo Expanded Netlist" );
        ngspice->Command( "echo ----------------" );
        ngspice->Command( "listing runnable" );
    }

    void TestOpPoint( double aRefValue, const std::string& aVectorName,
                      double aMaxRelError = MAX_DEFAULT_REL_ERROR )
    {
        BOOST_TEST_CONTEXT( "Vector name: " << aVectorName )
        {
            NGSPICE* ngspice = static_cast<NGSPICE*>( m_simulator.get() );

            std::vector<double> vector = ngspice->GetRealVector( aVectorName );

            BOOST_REQUIRE_EQUAL( vector.size(), 1 );

            double maxError = abs( aRefValue * aMaxRelError );
            BOOST_CHECK_LE( abs( vector[0] - aRefValue ), aMaxRelError );
        }
    }

    void TestPoint( const std::string& aXVectorName, double aXValue,
                    const std::map<const std::string, double> aTestVectorsAndValues,
                    double aMaxRelError = MAX_DEFAULT_REL_ERROR )
    {
        // The default aMaxRelError is fairly large because we have some problems with determinism
        // in QA pipeline. We don't need to fix this for now because, if this has to be fixed in
        // the first place, this has to be done from Ngspice's side.

        BOOST_TEST_CONTEXT( "X vector name: " << aXVectorName << ", X value: " << aXValue )
        {
            NGSPICE* ngspice = static_cast<NGSPICE*>( m_simulator.get() );

            std::vector<double> xVector = ngspice->GetRealVector( aXVectorName );
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

            for( auto& [vectorName, refValue] : aTestVectorsAndValues )
            {
                std::vector<double> yVector = ngspice->GetGainVector( vectorName );

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
                        double aMaxRelError = MAX_DEFAULT_REL_ERROR )
    {
        TestPoint( "time", aTime, aTestVectorsAndValues, aMaxRelError );
    }

    void TestACPoint( double aFrequency,
                      const std::map<const std::string, double> aTestVectorsAndValues,
                      double aMaxRelError = MAX_DEFAULT_REL_ERROR )
    {
        TestPoint( "frequency", aFrequency, aTestVectorsAndValues, aMaxRelError );
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
               | NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS
               | NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS
               | NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS
               | NETLIST_EXPORTER_SPICE::OPTION_SIM_COMMAND;
    }

    std::shared_ptr<SPICE_SIMULATOR>     m_simulator;
    std::shared_ptr<wxString>            m_log;
    std::unique_ptr<SPICE_TEST_REPORTER> m_reporter;
    bool m_abort;       // set to true to force abort durint a test
};
