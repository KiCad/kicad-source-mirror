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

#include <diff_merge/merge_validation_pipeline.h>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( ValidationPipeline )


// An empty input (no refdes, planRequiredRebuild and applierReportedRebuild
// both false, all schema versions zero) should pass cleanly.
BOOST_AUTO_TEST_CASE( EmptyInputPasses )
{
    VALIDATION_INPUT input;
    VALIDATION_REPORT report = RunPostApplyValidators( input );

    BOOST_CHECK( report.Passed() );
    BOOST_CHECK_EQUAL( report.Count(), 0u );
}


// A single refdes collision must surface as a failure with the colliding
// items pinned in relatedItems for cross-probe.
BOOST_AUTO_TEST_CASE( RefdesCollisionReportsRelatedItems )
{
    VALIDATION_INPUT input;
    KIID_PATH        idA;
    idA.push_back( KIID() );
    KIID_PATH        idB;
    idB.push_back( KIID() );

    input.refdesEntries.push_back( { wxS( "R1" ), idA } );
    input.refdesEntries.push_back( { wxS( "R1" ), idB } );

    VALIDATION_REPORT report = RunPostApplyValidators( input );

    BOOST_REQUIRE( !report.Passed() );
    BOOST_CHECK_EQUAL( report.Count(), 1u );

    const VALIDATION_FAILURE& f = report.failures[0];
    BOOST_CHECK( f.severity == RPT_SEVERITY_ERROR );
    BOOST_CHECK( !f.relatedItems.empty() );
}


// Plan-required + applier-reported = pass (the contract was honoured).
BOOST_AUTO_TEST_CASE( ConnectivityRebuildContractHonoured )
{
    VALIDATION_INPUT input;
    input.planRequiredRebuild    = true;
    input.applierReportedRebuild = true;

    VALIDATION_REPORT report = RunPostApplyValidators( input );
    BOOST_CHECK( report.Passed() );
}


// Plan-required + applier-NOT-reported = failure (the applier missed the
// rebuild the plan asked for).
BOOST_AUTO_TEST_CASE( ConnectivityRebuildContractBroken )
{
    VALIDATION_INPUT input;
    input.planRequiredRebuild    = true;
    input.applierReportedRebuild = false;

    VALIDATION_REPORT report = RunPostApplyValidators( input );
    BOOST_REQUIRE( !report.Passed() );

    bool foundRebuildFailure = false;

    for( const VALIDATION_FAILURE& f : report.failures )
    {
        if( f.validator.Find( wxS( "Connectivity" ) ) != wxNOT_FOUND )
            foundRebuildFailure = true;
    }

    BOOST_CHECK( foundRebuildFailure );
}


// Plan-NOT-required + applier-reported = passes (no harm in an extra rebuild).
BOOST_AUTO_TEST_CASE( ExtraRebuildIsNotFlagged )
{
    VALIDATION_INPUT input;
    input.planRequiredRebuild    = false;
    input.applierReportedRebuild = true;

    VALIDATION_REPORT report = RunPostApplyValidators( input );
    BOOST_CHECK( report.Passed() );
}


// Identical schema versions = pass.
BOOST_AUTO_TEST_CASE( SchemaVersionMatchPasses )
{
    VALIDATION_INPUT input;
    input.ancestorSchemaVersion = 20240101;
    input.oursSchemaVersion     = 20240101;
    input.theirsSchemaVersion   = 20240101;

    VALIDATION_REPORT report = RunPostApplyValidators( input );
    BOOST_CHECK( report.Passed() );
}


// Major schema version mismatch surfaces an error.
BOOST_AUTO_TEST_CASE( SchemaVersionMajorMismatchReportsError )
{
    VALIDATION_INPUT input;
    input.ancestorSchemaVersion = 20240101;
    input.oursSchemaVersion     = 20240101;
    input.theirsSchemaVersion   = 20990101;   // 5-year jump

    VALIDATION_REPORT report = RunPostApplyValidators( input );
    BOOST_REQUIRE( !report.Passed() );

    bool foundSchemaFailure = false;

    for( const VALIDATION_FAILURE& f : report.failures )
    {
        if( f.validator.Find( wxS( "Schema" ) ) != wxNOT_FOUND )
            foundSchemaFailure = true;
    }

    BOOST_CHECK( foundSchemaFailure );
}


// Multiple validator failures all surface in the same report -- the pipeline
// aggregates rather than short-circuiting on the first failure.
BOOST_AUTO_TEST_CASE( MultipleFailuresAggregate )
{
    VALIDATION_INPUT input;

    KIID_PATH idA;
    idA.push_back( KIID() );
    KIID_PATH idB;
    idB.push_back( KIID() );
    input.refdesEntries.push_back( { wxS( "U1" ), idA } );
    input.refdesEntries.push_back( { wxS( "U1" ), idB } );

    input.planRequiredRebuild    = true;
    input.applierReportedRebuild = false;

    VALIDATION_REPORT report = RunPostApplyValidators( input );

    BOOST_REQUIRE( !report.Passed() );
    BOOST_CHECK_GE( report.Count(), 2u );
}


// HasErrors() vs Passed(): a WARNING-only failure is NOT considered Passed
// (per VALIDATION_REPORT::Passed() == failures.empty()), but it's also
// distinguishable from an ERROR via HasErrors().
BOOST_AUTO_TEST_CASE( PassedAndHasErrorsAreDistinguishable )
{
    VALIDATION_INPUT input;
    input.planRequiredRebuild    = true;
    input.applierReportedRebuild = false;

    VALIDATION_REPORT report = RunPostApplyValidators( input );

    BOOST_CHECK( !report.Passed() );

    // The rebuild failure is ERROR severity in the existing validator impl.
    bool sawError = false;

    for( const VALIDATION_FAILURE& f : report.failures )
    {
        if( f.severity == RPT_SEVERITY_ERROR )
            sawError = true;
    }

    BOOST_CHECK_EQUAL( sawError, report.HasErrors() );
}


BOOST_AUTO_TEST_SUITE_END()
