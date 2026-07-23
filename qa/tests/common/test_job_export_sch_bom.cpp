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
#include <jobs/job_export_sch_bom.h>
#include <nlohmann/json.hpp>
#include <settings/bom_settings.h>


BOOST_AUTO_TEST_SUITE( JobExportSchBom )


BOOST_AUTO_TEST_CASE( ByteOrderMarkFormatPresetRoundTripAndLegacyDefault )
{
    BOM_FMT_PRESET preset = BOM_FMT_PRESET::CSV();
    preset.includeByteOrderMark = true;

    BOM_FMT_PRESET withoutByteOrderMark = preset;
    withoutByteOrderMark.includeByteOrderMark = false;
    BOOST_CHECK( preset != withoutByteOrderMark );

    nlohmann::json j = preset;

    BOOST_REQUIRE( j.contains( "include_byte_order_mark" ) );
    BOOST_CHECK( j.at( "include_byte_order_mark" ).get<bool>() );

    BOM_FMT_PRESET loaded = j.get<BOM_FMT_PRESET>();
    BOOST_CHECK( loaded.includeByteOrderMark );

    j.erase( "include_byte_order_mark" );
    loaded.includeByteOrderMark = true;
    from_json( j, loaded );
    BOOST_CHECK( !loaded.includeByteOrderMark );
}


BOOST_AUTO_TEST_CASE( VariantRoundTrip )
{
    JOB_EXPORT_SCH_BOM job;
    job.SetSelectedVariant( wxS( "VAR A" ) );
    job.m_includeByteOrderMark = true;

    nlohmann::json j;
    job.ToJson( j );

    JOB_EXPORT_SCH_BOM loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.GetSelectedVariant() == wxS( "VAR A" ) );
    BOOST_CHECK( loaded.m_includeByteOrderMark );
}


BOOST_AUTO_TEST_CASE( EmptyVariantRoundTrip )
{
    JOB_EXPORT_SCH_BOM job;

    nlohmann::json j;
    job.ToJson( j );

    JOB_EXPORT_SCH_BOM loaded;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.GetSelectedVariant().IsEmpty() );
}


// Issue #23932: saving a job used to append the choice every time, so the list grew
// without bound. Setting the selection must replace the list, never grow it.
BOOST_AUTO_TEST_CASE( VariantNoAccumulation )
{
    JOB_EXPORT_SCH_BOM job;
    job.SetSelectedVariant( wxS( "VAR A" ) );
    job.SetSelectedVariant( wxS( "VAR A" ) );
    job.SetSelectedVariant( wxS( "VAR B" ) );

    BOOST_CHECK_EQUAL( job.m_variantNames.size(), 1u );
    BOOST_CHECK( job.GetSelectedVariant() == wxS( "VAR B" ) );
}


// Selecting the default variant leaves the list empty.
BOOST_AUTO_TEST_CASE( DefaultVariantClearsList )
{
    JOB_EXPORT_SCH_BOM job;
    job.SetSelectedVariant( wxS( "VAR A" ) );
    job.SetSelectedVariant( wxEmptyString );

    BOOST_CHECK( job.m_variantNames.empty() );
    BOOST_CHECK( job.GetSelectedVariant().IsEmpty() );
}


// A CLI multi-variant list is reported through its first entry.
BOOST_AUTO_TEST_CASE( VariantFromList )
{
    nlohmann::json j = { { "variant_names", { "VAR A", "VAR B" } } };

    JOB_EXPORT_SCH_BOM loaded;
    loaded.m_includeByteOrderMark = true;
    loaded.FromJson( j );

    BOOST_CHECK( loaded.GetSelectedVariant() == wxS( "VAR A" ) );
    BOOST_CHECK( !loaded.m_includeByteOrderMark );
}


BOOST_AUTO_TEST_SUITE_END()
