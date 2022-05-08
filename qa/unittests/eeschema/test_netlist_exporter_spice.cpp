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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>
#include <netlist_exporter_spice.h>


class TEST_NETLIST_EXPORTER_SPICE_FIXTURE : public TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_SPICE>
{
public:
    wxString GetSchematicPath( const wxString& aBaseName ) override
    {
        wxFileName fn = KI_TEST::GetEeschemaTestDataDir();
        fn.AppendDir( "spice_netlists" );
        fn.AppendDir( aBaseName );
        fn.SetName( aBaseName );
        fn.SetExt( KiCadSchematicFileExtension );

        return fn.GetFullPath();
    }

    wxString GetNetlistPath( bool aTest = false ) override
    {
        wxFileName netFile = m_schematic.Prj().GetProjectFullName();

        if( aTest )
            netFile.SetName( netFile.GetName() + "_test" );

        netFile.SetExt( "cir" );
        return netFile.GetFullPath();
    }

    void CompareNetlists() override
    {
        FILE_LINE_READER refReader( GetNetlistPath() );
        FILE_LINE_READER resultReader( GetNetlistPath( true ) );
        char* refLine = nullptr;
        char* resultLine = nullptr;

        while( true )
        {
            refLine = refReader.ReadLine();
            resultLine = resultReader.ReadLine();

            if( !refLine || !resultReader )
                break;

            BOOST_REQUIRE_EQUAL( std::string( refReader.Line() ),
                                 std::string( resultReader.Line() ) );
        }

        BOOST_REQUIRE_EQUAL( std::string( refReader.Line() ),
                             std::string( resultReader.Line() ) );
    }

    unsigned GetNetlistOptions() override
    {
        return NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES
               | NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;
    }
};


BOOST_FIXTURE_TEST_SUITE( NetlistExporterSpice, TEST_NETLIST_EXPORTER_SPICE_FIXTURE )


BOOST_AUTO_TEST_CASE( Rectifier )
{
    TestNetlist( "rectifier" );
}


BOOST_AUTO_TEST_CASE( Chirp )
{
    TestNetlist( "chirp" );
}


BOOST_AUTO_TEST_CASE( Opamp )
{
    TestNetlist( "opamp" );
}


BOOST_AUTO_TEST_CASE( NpnCeAmp )
{
    TestNetlist( "npn_ce_amp" );
}


BOOST_AUTO_TEST_CASE( Passives )
{
    TestNetlist( "passives" );
}


BOOST_AUTO_TEST_SUITE_END()
