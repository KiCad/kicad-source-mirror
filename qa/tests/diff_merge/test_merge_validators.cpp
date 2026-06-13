/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <diff_merge/merge_validators.h>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( MergeValidators )


BOOST_AUTO_TEST_CASE( RefdesUniquenessPassesOnUnique )
{
    KIID::SeedGenerator( 300 );
    KIID id1, id2, id3;

    std::vector<REFDES_ENTRY> entries = {
        { wxS( "R1" ), KIID_PATH( id1.AsString() ) },
        { wxS( "R2" ), KIID_PATH( id2.AsString() ) },
        { wxS( "C1" ), KIID_PATH( id3.AsString() ) },
    };

    VALIDATION_REPORT r = CheckRefdesUniqueness( entries );
    BOOST_CHECK( r.Passed() );
}


BOOST_AUTO_TEST_CASE( RefdesUniquenessDetectsDuplicates )
{
    KIID::SeedGenerator( 301 );
    KIID id1, id2;

    std::vector<REFDES_ENTRY> entries = {
        { wxS( "R1" ), KIID_PATH( id1.AsString() ) },
        { wxS( "R1" ), KIID_PATH( id2.AsString() ) },
    };

    VALIDATION_REPORT r = CheckRefdesUniqueness( entries );
    BOOST_REQUIRE_EQUAL( r.failures.size(), 1u );
    BOOST_CHECK( r.HasErrors() );
    BOOST_CHECK_EQUAL( r.failures[0].validator.ToStdString(), "RefdesUniqueness" );
    BOOST_CHECK_EQUAL( r.failures[0].relatedItems.size(), 2u );
}


BOOST_AUTO_TEST_CASE( RefdesUniquenessIgnoresEmptyAndPlaceholder )
{
    KIID::SeedGenerator( 302 );
    KIID id1, id2, id3;

    std::vector<REFDES_ENTRY> entries = {
        { wxS( "" ),   KIID_PATH( id1.AsString() ) },
        { wxS( "" ),   KIID_PATH( id2.AsString() ) },   // both empty — OK
        { wxS( "R?" ), KIID_PATH( id3.AsString() ) },   // placeholder — OK
    };

    VALIDATION_REPORT r = CheckRefdesUniqueness( entries );
    BOOST_CHECK( r.Passed() );
}


BOOST_AUTO_TEST_CASE( ConnectivityFlagDetectsMissedRebuild )
{
    BOOST_CHECK( CheckConnectivityRebuildFlag( true,  true  ).Passed() );
    BOOST_CHECK( CheckConnectivityRebuildFlag( false, false ).Passed() );
    BOOST_CHECK( CheckConnectivityRebuildFlag( false, true  ).Passed() ); // OK to over-rebuild

    VALIDATION_REPORT r = CheckConnectivityRebuildFlag( true, false );
    BOOST_REQUIRE_EQUAL( r.failures.size(), 1u );
    BOOST_CHECK( r.HasErrors() );
}


BOOST_AUTO_TEST_CASE( SchemaVersionEqualPasses )
{
    VALIDATION_REPORT r = CheckSchemaVersions( 20240108, 20240108, 20240108 );
    BOOST_CHECK( r.Passed() );
}


BOOST_AUTO_TEST_CASE( SchemaVersionMinorMismatchWarns )
{
    VALIDATION_REPORT r = CheckSchemaVersions( 20240108, 20240115, 20240108 );
    BOOST_REQUIRE_EQUAL( r.failures.size(), 1u );
    BOOST_CHECK( !r.HasErrors() );   // WARNING only
}


BOOST_AUTO_TEST_CASE( SchemaVersionMajorMismatchErrors )
{
    VALIDATION_REPORT r = CheckSchemaVersions( 20180101, 20240115, 20240108 );
    BOOST_CHECK( r.HasErrors() );
}


BOOST_AUTO_TEST_CASE( ReportMergePreservesFailures )
{
    KIID::SeedGenerator( 303 );
    KIID id1, id2;

    VALIDATION_REPORT a = CheckRefdesUniqueness( {
        { wxS( "R1" ), KIID_PATH( id1.AsString() ) },
        { wxS( "R1" ), KIID_PATH( id2.AsString() ) },
    } );

    VALIDATION_REPORT b = CheckConnectivityRebuildFlag( true, false );

    std::size_t expectedSize = a.failures.size() + b.failures.size();
    a.Merge( std::move( b ) );
    BOOST_CHECK_EQUAL( a.failures.size(), expectedSize );
}


BOOST_AUTO_TEST_CASE( ReportMergePreservesFailureContent )
{
    // Size alone isn't enough — verify the actual validator names from
    // both sides land in the merged report. The move-out side must also
    // be empty (Merge takes by rvalue and clear()s in the impl).
    KIID::SeedGenerator( 304 );
    KIID id1, id2;

    VALIDATION_REPORT a = CheckRefdesUniqueness( {
        { wxS( "R1" ), KIID_PATH( id1.AsString() ) },
        { wxS( "R1" ), KIID_PATH( id2.AsString() ) },
    } );
    VALIDATION_REPORT b = CheckConnectivityRebuildFlag( true, false );

    a.Merge( std::move( b ) );

    BOOST_CHECK( b.failures.empty() );

    bool sawRefdes        = false;
    bool sawConnectivity  = false;

    for( const VALIDATION_FAILURE& f : a.failures )
    {
        if( f.validator == wxS( "RefdesUniqueness" ) )
            sawRefdes = true;
        if( f.validator == wxS( "ConnectivityRebuild" ) )
            sawConnectivity = true;
    }

    BOOST_CHECK( sawRefdes );
    BOOST_CHECK( sawConnectivity );
}


BOOST_AUTO_TEST_CASE( HasErrorsMixedSeverityFindsError )
{
    // HasErrors must short-circuit on a single ERROR even when WARNINGs
    // also exist. A naive "all entries are errors" implementation would
    // miss this case.
    VALIDATION_REPORT r;

    VALIDATION_FAILURE w;
    w.severity = RPT_SEVERITY_WARNING;
    w.validator = wxS( "WarnOnly" );
    r.failures.push_back( w );

    VALIDATION_FAILURE e;
    e.severity = RPT_SEVERITY_ERROR;
    e.validator = wxS( "RealError" );
    r.failures.push_back( e );

    BOOST_CHECK( r.HasErrors() );
    BOOST_CHECK( !r.Passed() );
}


BOOST_AUTO_TEST_CASE( HasErrorsAllWarningsIsClean )
{
    // Only-warning reports must read as "passed for the purposes of
    // errors" — the caller can still surface warnings separately, but
    // HasErrors is the gate that blocks the merge.
    VALIDATION_REPORT r;

    VALIDATION_FAILURE w1;
    w1.severity = RPT_SEVERITY_WARNING;
    r.failures.push_back( w1 );

    VALIDATION_FAILURE w2;
    w2.severity = RPT_SEVERITY_WARNING;
    r.failures.push_back( w2 );

    BOOST_CHECK( !r.HasErrors() );
    BOOST_CHECK( !r.Passed() );   // warnings still fail Passed()
}


BOOST_AUTO_TEST_CASE( HasErrorsEmptyReportIsClean )
{
    VALIDATION_REPORT r;
    BOOST_CHECK( !r.HasErrors() );
    BOOST_CHECK( r.Passed() );
}


BOOST_AUTO_TEST_CASE( SchemaVersionBoundaryDeltaJustBelowMajor )
{
    // 48885 < MAJOR_EPOCH_DELTA (50000), so this should warn, not error.
    // Without an explicit boundary test the threshold could drift silently.
    VALIDATION_REPORT r = CheckSchemaVersions( 20240115, 20290000, 20240115 );
    BOOST_REQUIRE_EQUAL( r.failures.size(), 1u );
    BOOST_CHECK( !r.HasErrors() );
}


BOOST_AUTO_TEST_CASE( SchemaVersionBoundaryDeltaExactly50000 )
{
    // Delta == 50000 exactly. The implementation uses `>=` so this is the
    // first ERROR; a future refactor to `>` would silently downgrade this
    // case to WARNING and let a borderline-stale schema land. Pin the
    // inclusive comparison explicitly.
    VALIDATION_REPORT r = CheckSchemaVersions( 20200000, 20250000, 20200000 );
    BOOST_REQUIRE( !r.failures.empty() );
    BOOST_CHECK( r.HasErrors() );
}


BOOST_AUTO_TEST_CASE( RefdesUniquenessSingleEntryNeverFails )
{
    // A single entry can't collide with itself — the < 2 guard prevents
    // emitting a vacuous self-collision report.
    KIID::SeedGenerator( 305 );
    KIID id1;
    std::vector<REFDES_ENTRY> entries = {
        { wxS( "R1" ), KIID_PATH( id1.AsString() ) },
    };
    BOOST_CHECK( CheckRefdesUniqueness( entries ).Passed() );
}


BOOST_AUTO_TEST_CASE( RefdesUniquenessThreeWayCollision )
{
    // Triple collision should surface all three ids in relatedItems, not
    // just the first pair detected.
    KIID::SeedGenerator( 306 );
    KIID id1, id2, id3;
    std::vector<REFDES_ENTRY> entries = {
        { wxS( "U1" ), KIID_PATH( id1.AsString() ) },
        { wxS( "U1" ), KIID_PATH( id2.AsString() ) },
        { wxS( "U1" ), KIID_PATH( id3.AsString() ) },
    };

    VALIDATION_REPORT r = CheckRefdesUniqueness( entries );
    BOOST_REQUIRE_EQUAL( r.failures.size(), 1u );
    BOOST_CHECK_EQUAL( r.failures[0].relatedItems.size(), 3u );
}


BOOST_AUTO_TEST_SUITE_END()
