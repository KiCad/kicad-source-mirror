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
#include <jobs/job_export_sch_plot.h>
#include <nlohmann/json.hpp>


BOOST_AUTO_TEST_SUITE( JobExportSchPlot )


BOOST_AUTO_TEST_CASE( VariantRoundTrip )
{
    JOB_EXPORT_SCH_PLOT_PDF job;
    job.m_variant = wxS( "var2" );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant_name", "" ), "var2" );

    JOB_EXPORT_SCH_PLOT_PDF loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.m_variant == wxS( "var2" ) );
}


BOOST_AUTO_TEST_CASE( EmptyVariantRoundTrip )
{
    JOB_EXPORT_SCH_PLOT_PDF job;

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "variant_name", "" ), "" );

    JOB_EXPORT_SCH_PLOT_PDF loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.m_variant.IsEmpty() );
    BOOST_CHECK( loaded.m_variantNames.empty() );
}


BOOST_AUTO_TEST_CASE( VariantFromJobsetJson )
{
    // Simulate loading a jobset that specifies variant_name but not m_variantNames.
    // This is the exact scenario from issue #22891.
    nlohmann::json j = {
        { "variant_name", "var2" },
        { "format", "pdf" },
        { "plot_all", true },
        { "black_and_white", false }
    };

    JOB_EXPORT_SCH_PLOT_PDF loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.m_variant == wxS( "var2" ) );
    BOOST_CHECK( loaded.m_variantNames.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
