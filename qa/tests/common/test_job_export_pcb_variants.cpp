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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_export_pcb_plot.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_pcb_render.h>
#include <nlohmann/json.hpp>


// Regression coverage for issue #24092: variant fields on JOBSET-resident jobs
// must round-trip through ToJson/FromJson.

BOOST_AUTO_TEST_SUITE( JobExportPcbVariants )


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
    job.m_variant = wxS( "VarOdb" );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant", "" ), "VarOdb" );

    JOB_EXPORT_PCB_ODB loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.m_variant == wxS( "VarOdb" ) );
}


BOOST_AUTO_TEST_CASE( PcbIpc2581VariantRoundTrip )
{
    JOB_EXPORT_PCB_IPC2581 job;
    job.m_variant = wxS( "VarIpc" );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant", "" ), "VarIpc" );

    JOB_EXPORT_PCB_IPC2581 loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.m_variant == wxS( "VarIpc" ) );
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
