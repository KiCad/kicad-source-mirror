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
        REPORTER& Report( const wxString& aText,
                          SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
        {
            // You can add a debug trace here.
            return *this;
        }

        bool HasMessage() const override { return false; }

        void OnSimStateChange( SPICE_SIMULATOR* aObject, SIM_STATE aNewState ) override
        {
        }
    };

    TEST_NETLIST_EXPORTER_SPICE_FIXTURE() :
        TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_SPICE>(),
        m_simulator( SPICE_SIMULATOR::CreateInstance( "ngspice" ) ),
        m_reporter( std::make_unique<SPICE_TEST_REPORTER>() )
    {
        
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
        // Our simulator is actually Ngspice.
        NGSPICE* ngspice = dynamic_cast<NGSPICE*>( m_simulator.get() );
        ngspice->SetReporter( m_reporter.get() );

        wxFFile file( GetNetlistPath( true ), "rt" );
        wxString netlist;

        file.ReadAll( &netlist );
        //ngspice->Init();
        ngspice->Command( "set ngbehavior=ps" );
        ngspice->Command( "setseed 1" );
        ngspice->Command( "set filetype=ascii" );
        ngspice->LoadNetlist( netlist.ToStdString() );
        ngspice->Run();


        wxString vectors;
        for( const wxString& vector : m_plottedVectors )
            vectors << vector << " ";

        ngspice->Command( wxString::Format( "write %s %s", GetResultsPath( true ),
                                            vectors ).ToStdString() );


        FILE_LINE_READER refReader( GetResultsPath() );
        FILE_LINE_READER resultReader( GetResultsPath( true ) );
        char* refLine = nullptr;
        char* resultLine = nullptr;

        while( true )
        {
            refLine = refReader.ReadLine();
            resultLine = resultReader.ReadLine();

            // Ignore the date.
            if( wxString( resultLine ).StartsWith( "Date: " ) )
                continue;

            if( !refLine || !resultReader )
                break;

            BOOST_REQUIRE_EQUAL( std::string( refReader.Line() ),
                                 std::string( resultReader.Line() ) );
        }

        BOOST_REQUIRE_EQUAL( std::string( refReader.Line() ),
                             std::string( resultReader.Line() ) );
    }

    void TestNetlist( const wxString& aBaseName, const std::vector<wxString> aPlottedVectors )
    {
        m_plottedVectors = aPlottedVectors;
        TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_SPICE>::TestNetlist( aBaseName );
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

    std::unique_ptr<SPICE_TEST_REPORTER> m_reporter;
    std::shared_ptr<SPICE_SIMULATOR> m_simulator;
    std::vector<wxString> m_plottedVectors;
};


BOOST_FIXTURE_TEST_SUITE( NetlistExporterSpice, TEST_NETLIST_EXPORTER_SPICE_FIXTURE )


/*BOOST_AUTO_TEST_CASE( Rectifier )
{
    const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "rectifier", { "V(/in)", "V(/out)" } );
}*/

// FIXME: Fails due to some nondeterminism, seems related to convergence problems.

/*BOOST_AUTO_TEST_CASE( Chirp )
{
    TestNetlist( "chirp", { "V(/out)", "I(R1)" } );
}*/


BOOST_AUTO_TEST_CASE( Opamp )
{
    const MOCK_PGM_BASE& program = static_cast<MOCK_PGM_BASE&>( Pgm() );
    MOCK_EXPECT( program.GetLocalEnvVariables ).returns( ENV_VAR_MAP() );

    TestNetlist( "opamp", { "V(/in)", "V(/out)", "I(XU1:+IN)", "I(XU1:-IN)", "I(XU1:OUT)" } );
}


/*BOOST_AUTO_TEST_CASE( NpnCeAmp )
{
    TestNetlist( "npn_ce_amp", { "V(/in)", "V(/out)" } );
}*/

// Incomplete. TODO.

/*BOOST_AUTO_TEST_CASE( Passives )
{
    TestNetlist( "passives" );
}*/


BOOST_AUTO_TEST_CASE( Tlines )
{
    TestNetlist( "tlines", { "V(/z0_in)", "V(/z0_out)", "V(/rlgc_in)", "V(/rlgc_out)" } );
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


/*BOOST_AUTO_TEST_CASE( CmosNot )
{
    TestNetlist( "cmos_not", { "V(/in)", "V(/out)" } );
}*/


/*BOOST_AUTO_TEST_CASE( InstanceParams )
{
    // TODO.
    //TestNetlist( "instance_params", {} );
}*/


/*BOOST_AUTO_TEST_CASE( LegacyLaserDriver )
{
    TestNetlist( "legacy_laser_driver", { "V(/out)" } );
}


BOOST_AUTO_TEST_CASE( LegacyPspice )
{
    TestNetlist( "legacy_pspice", { "V(/VIN)", "V(VOUT)" } );
}*/


/*BOOST_AUTO_TEST_CASE( LegacyRectifier )
{
    TestNetlist( "legacy_rectifier", { "V(/signal_in)", "V(/rect_out)" } );
}*/


/*BOOST_AUTO_TEST_CASE( LegacySallenKey )
{
    TestNetlist( "legacy_sallen_key", { "V(/lowpass)" } );
}*/


BOOST_AUTO_TEST_CASE( LegacySources )
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
}


BOOST_AUTO_TEST_SUITE_END()

#endif // KICAD_SPICE
