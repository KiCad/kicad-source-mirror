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
#include <jobs/job_sch_erc.h>
#include <nlohmann/json.hpp>


BOOST_AUTO_TEST_SUITE( JobSchErc )


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23967
// A transient working output path (applied when the configured filename is empty)
// must not be serialized back into the jobset file.
BOOST_AUTO_TEST_CASE( WorkingOutputPathNotPersisted )
{
    JOB_SCH_ERC job;

    BOOST_REQUIRE( job.GetConfiguredOutputPath().IsEmpty() );

    job.SetWorkingOutputPath( wxS( "my-project-erc.rpt" ) );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK( j.contains( "output_filename" ) );
    BOOST_CHECK_EQUAL( j.value( "output_filename", "non-empty" ), "" );
    BOOST_CHECK( job.GetConfiguredOutputPath().IsEmpty() );
}


// A configured (user-supplied) output path must round-trip through JSON.
BOOST_AUTO_TEST_CASE( ConfiguredOutputPathPersisted )
{
    JOB_SCH_ERC job;
    job.SetConfiguredOutputPath( wxS( "custom-erc.rpt" ) );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK_EQUAL( j.value( "output_filename", "" ), "custom-erc.rpt" );
}


// Setting a configured path after a transient working path was seeded (from a prior run with an
// empty filename) must let the new configured path win. Otherwise a user who runs the job once
// with an empty filename and then edits the config to supply a real filename will still hit the
// stale generated fallback on the next run.
BOOST_AUTO_TEST_CASE( ConfiguredOutputPathOverridesWorkingPath )
{
    JOB_SCH_ERC job;

    job.SetWorkingOutputPath( wxS( "generated-erc.rpt" ) );
    job.SetConfiguredOutputPath( wxS( "custom-erc.rpt" ) );

    BOOST_CHECK( job.GetWorkingOutputPath().IsEmpty() );
    BOOST_CHECK( job.GetFullOutputPath( nullptr ) == wxS( "custom-erc.rpt" ) );
}


BOOST_AUTO_TEST_SUITE_END()
