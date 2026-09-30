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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_export_pcb_plot.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_pcb_render.h>
#include <json_common.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <filesystem>
#include <fstream>


// Regression coverage for issue #24092: variant fields on JOBSET-resident jobs
// must round-trip through ToJson/FromJson.

BOOST_AUTO_TEST_SUITE( JobExportPcbVariants )


BOOST_AUTO_TEST_CASE( FabJobMissingKeysKeepDefaults )
{
    nlohmann::json missingKeys = nlohmann::json::object();

    JOB_EXPORT_PCB_ODB odb;
    odb.FromJson( missingKeys );
    nlohmann::json odbJson;
    odb.ToJson( odbJson );
    BOOST_CHECK_EQUAL( odbJson.at( "precision" ).get<int>(), 6 );
    BOOST_CHECK_EQUAL( odbJson.at( "units" ).get<std::string>(), "mm" );
    BOOST_CHECK( !odbJson.at( "check_zones" ).get<bool>() );
    BOOST_CHECK_EQUAL( odbJson.at( "variant" ).get<std::string>(), "" );

    JOB_EXPORT_PCB_IPC2581 ipc;
    ipc.FromJson( missingKeys );
    nlohmann::json ipcJson;
    ipc.ToJson( ipcJson );
    BOOST_CHECK_EQUAL( ipcJson.at( "precision" ).get<int>(), 6 );
    BOOST_CHECK_EQUAL( ipcJson.at( "units" ).get<std::string>(), "mm" );
    BOOST_CHECK( !ipcJson.value( "check_zones", true ) );

    BOOST_CHECK_EQUAL( ipcJson.at( "variant" ).get<std::string>(), "" );

    nlohmann::json emptyVariant = { { "variant", "" } };
    JOB_EXPORT_PCB_ODB legacyOdb;
    JOB_EXPORT_PCB_IPC2581 legacyIpc;
    legacyOdb.FromJson( emptyVariant );
    legacyIpc.FromJson( emptyVariant );
    BOOST_CHECK( legacyOdb.m_variantNames.empty() );
    BOOST_CHECK( legacyIpc.m_variantNames.empty() );
}


BOOST_AUTO_TEST_CASE( FabV10JobsLoadUnchanged )
{
    std::filesystem::path fixtureDir = std::filesystem::path( KI_TEST::GetTestDataRootDir() ) / "common" / "jobs";
    std::ifstream odbFile( fixtureDir / "odb_job_v10.json" );
    std::ifstream ipcFile( fixtureDir / "ipc2581_job_v10.json" );
    BOOST_REQUIRE( odbFile );
    BOOST_REQUIRE( ipcFile );
    nlohmann::json odbV10 = nlohmann::json::parse( odbFile );
    nlohmann::json ipcV10 = nlohmann::json::parse( ipcFile );

    JOB_EXPORT_PCB_ODB odb;
    odb.FromJson( odbV10 );
    nlohmann::json odbRoundTrip;
    odb.ToJson( odbRoundTrip );

    for( auto it = odbV10.begin(); it != odbV10.end(); ++it )
    {
        BOOST_CHECK_MESSAGE( odbRoundTrip.at( it.key() ) == it.value(), "ODB key " << it.key() );
    }

    JOB_EXPORT_PCB_IPC2581 ipc;
    ipc.FromJson( ipcV10 );
    nlohmann::json ipcRoundTrip;
    ipc.ToJson( ipcRoundTrip );

    for( auto it = ipcV10.begin(); it != ipcV10.end(); ++it )
    {
        BOOST_CHECK_MESSAGE( ipcRoundTrip.at( it.key() ) == it.value(), "IPC key " << it.key() );
    }

    BOOST_CHECK( odbRoundTrip.contains( "variant_names" ) );
    BOOST_CHECK( ipcRoundTrip.contains( "variant_names" ) );
}


BOOST_AUTO_TEST_CASE( Ipc2581CheckZonesOptionExists )
{
    JOB_EXPORT_PCB_IPC2581 ipc;
    nlohmann::json options = { { "check_zones", true } };
    ipc.FromJson( options );
    nlohmann::json roundTrip;
    ipc.ToJson( roundTrip );
    BOOST_CHECK( roundTrip.value( "check_zones", false ) );
}


BOOST_AUTO_TEST_CASE( Pcb3dVariantRoundTrip )
{
    JOB_EXPORT_PCB_3D job;
    job.m_variant = wxS( "var-A" );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant", "" ), "var-A" );

    JOB_EXPORT_PCB_3D loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.m_variant == wxS( "var-A" ) );
}


BOOST_AUTO_TEST_CASE( PcbPlotVariantRoundTrip )
{
    JOB_EXPORT_PCB_PLOT job( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::PDF, "plot", false );
    job.m_variant = wxS( "VarPlot" );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant", "" ), "VarPlot" );

    JOB_EXPORT_PCB_PLOT loaded( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::PDF, "plot", false );
    loaded.FromJson( j );

    BOOST_CHECK( loaded.m_variant == wxS( "VarPlot" ) );
}


BOOST_AUTO_TEST_CASE( PcbOdbVariantRoundTrip )
{
    JOB_EXPORT_PCB_ODB job;
    job.m_variantNames = { wxS( "VarOdb" ) };

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant", "" ), "VarOdb" );

    JOB_EXPORT_PCB_ODB loaded;
    loaded.FromJson( j );

    BOOST_REQUIRE_EQUAL( loaded.m_variantNames.size(), 1u );
    BOOST_CHECK( loaded.m_variantNames.front() == wxS( "VarOdb" ) );
}


BOOST_AUTO_TEST_CASE( PcbIpc2581VariantRoundTrip )
{
    JOB_EXPORT_PCB_IPC2581 job;
    job.m_variantNames = { wxS( "VarIpc" ) };

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant", "" ), "VarIpc" );

    JOB_EXPORT_PCB_IPC2581 loaded;
    loaded.FromJson( j );

    BOOST_REQUIRE_EQUAL( loaded.m_variantNames.size(), 1u );
    BOOST_CHECK( loaded.m_variantNames.front() == wxS( "VarIpc" ) );
}


BOOST_AUTO_TEST_CASE( PcbRenderVariantRoundTrip )
{
    JOB_PCB_RENDER job;
    job.m_variant = wxS( "VarRender" );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant", "" ), "VarRender" );

    JOB_PCB_RENDER loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.m_variant == wxS( "VarRender" ) );
}


BOOST_AUTO_TEST_CASE( SchNetlistVariantNamesRoundTrip )
{
    JOB_EXPORT_SCH_NETLIST job;
    job.m_variantNames = { wxS( "var1" ), wxS( "var2" ) };

    nlohmann::json j;
    job.ToJson( j );

    BOOST_REQUIRE( j.contains( "variant_names" ) );
    BOOST_REQUIRE( j.at( "variant_names" ).is_array() );
    BOOST_CHECK_EQUAL( j.at( "variant_names" ).size(), 2u );

    JOB_EXPORT_SCH_NETLIST loaded;
    loaded.FromJson( j );

    BOOST_REQUIRE_EQUAL( loaded.m_variantNames.size(), 2u );
    BOOST_CHECK( loaded.m_variantNames[0] == wxS( "var1" ) );
    BOOST_CHECK( loaded.m_variantNames[1] == wxS( "var2" ) );
}


BOOST_AUTO_TEST_CASE( SchPlotVariantNamesRoundTrip )
{
    JOB_EXPORT_SCH_PLOT job( false );
    job.m_variantNames = { wxS( "var1" ), wxS( "var2" ) };

    nlohmann::json j;
    job.ToJson( j );

    BOOST_REQUIRE( j.contains( "variant_names" ) );
    BOOST_REQUIRE( j.at( "variant_names" ).is_array() );
    BOOST_CHECK_EQUAL( j.at( "variant_names" ).size(), 2u );

    JOB_EXPORT_SCH_PLOT loaded( false );
    loaded.FromJson( j );

    BOOST_REQUIRE_EQUAL( loaded.m_variantNames.size(), 2u );
    BOOST_CHECK( loaded.m_variantNames[0] == wxS( "var1" ) );
    BOOST_CHECK( loaded.m_variantNames[1] == wxS( "var2" ) );
}


BOOST_AUTO_TEST_SUITE_END()
