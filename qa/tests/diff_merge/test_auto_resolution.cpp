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

#include <diff_merge/auto_resolution.h>

#include <wx/intl.h>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( AutoResolution )


// ParseAutoResolutionJson ---------------------------------------------------

BOOST_AUTO_TEST_CASE( Parse_ValidObject )
{
    const std::string json =
            R"({"/abc": "take_ours", "/def": "take_theirs", "/ghi": "take_ancestor"})";

    auto r = ParseAutoResolutionJson( json );
    BOOST_REQUIRE( r.status == AUTO_RESOLUTION_STATUS::OK );
    BOOST_REQUIRE_EQUAL( r.resolutions.size(), 3u );
    BOOST_CHECK( r.resolutions.at( wxS( "/abc" ) ) == ITEM_RES::TAKE_OURS );
    BOOST_CHECK( r.resolutions.at( wxS( "/def" ) ) == ITEM_RES::TAKE_THEIRS );
    BOOST_CHECK( r.resolutions.at( wxS( "/ghi" ) ) == ITEM_RES::TAKE_ANCESTOR );
}


BOOST_AUTO_TEST_CASE( Parse_EmptyObject )
{
    auto r = ParseAutoResolutionJson( "{}" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::OK );
    BOOST_CHECK( r.resolutions.empty() );
}


BOOST_AUTO_TEST_CASE( Parse_InvalidJson )
{
    auto r = ParseAutoResolutionJson( "this is not json {{{" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::INVALID_JSON );
    BOOST_CHECK( !r.errorContext.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( Parse_EmptyString )
{
    auto r = ParseAutoResolutionJson( "" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::INVALID_JSON );
}


BOOST_AUTO_TEST_CASE( Parse_NotObject )
{
    auto r = ParseAutoResolutionJson( "[1, 2, 3]" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::NOT_OBJECT );
}


BOOST_AUTO_TEST_CASE( Parse_NotObject_String )
{
    auto r = ParseAutoResolutionJson( R"("take_ours")" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::NOT_OBJECT );
}


BOOST_AUTO_TEST_CASE( Parse_BadValueType )
{
    // Value is a number, not a string. Errors out at the first bad entry.
    auto r = ParseAutoResolutionJson( R"({"/abc": "take_ours", "/def": 42})" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::BAD_VALUE );
    BOOST_CHECK( r.errorContext == wxS( "/def" ) );
}


BOOST_AUTO_TEST_CASE( Parse_UnknownKindString )
{
    auto r = ParseAutoResolutionJson( R"({"/abc": "take_neither"})" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::UNKNOWN_KIND );
    BOOST_CHECK( r.errorContext == wxS( "/abc" ) );
}


BOOST_AUTO_TEST_CASE( Parse_EngineInternalKindRejected_Keep )
{
    auto r = ParseAutoResolutionJson( R"({"/abc": "keep"})" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::ENGINE_INTERNAL_KIND );
    BOOST_CHECK( r.errorContext == wxS( "/abc" ) );
}


BOOST_AUTO_TEST_CASE( Parse_EngineInternalKindRejected_Delete )
{
    auto r = ParseAutoResolutionJson( R"({"/abc": "delete"})" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::ENGINE_INTERNAL_KIND );
}


BOOST_AUTO_TEST_CASE( Parse_EngineInternalKindRejected_MergeProps )
{
    auto r = ParseAutoResolutionJson( R"({"/abc": "merge_props"})" );
    BOOST_CHECK( r.status == AUTO_RESOLUTION_STATUS::ENGINE_INTERNAL_KIND );
}


BOOST_AUTO_TEST_CASE( Parse_NonOkStatusClearsResolutions )
{
    // Header doc: "resolutions is populated only when status == OK". Pin
    // that contract — a first-valid-then-bad entry must not leave the
    // first entry in the map.
    auto r = ParseAutoResolutionJson(
            R"({"/abc": "take_ours", "/def": "not_a_kind"})" );
    BOOST_REQUIRE( r.status == AUTO_RESOLUTION_STATUS::UNKNOWN_KIND );
    BOOST_CHECK( r.resolutions.empty() );

    auto r2 = ParseAutoResolutionJson(
            R"({"/abc": "take_ours", "/def": "keep"})" );
    BOOST_REQUIRE( r2.status == AUTO_RESOLUTION_STATUS::ENGINE_INTERNAL_KIND );
    BOOST_CHECK( r2.resolutions.empty() );

    auto r3 = ParseAutoResolutionJson(
            R"({"/abc": "take_ours", "/def": 42})" );
    BOOST_REQUIRE( r3.status == AUTO_RESOLUTION_STATUS::BAD_VALUE );
    BOOST_CHECK( r3.resolutions.empty() );
}


BOOST_AUTO_TEST_CASE( Parse_UnicodeKeys )
{
    // Non-ASCII keys must round-trip through UTF-8 — KIID_PATH strings are
    // ASCII in practice but the parser shouldn't lose data on unicode input.
    auto r = ParseAutoResolutionJson( "{\"/élément\": \"take_ours\"}" );
    BOOST_REQUIRE( r.status == AUTO_RESOLUTION_STATUS::OK );
    BOOST_CHECK( r.resolutions.count( wxString::FromUTF8( "/élément" ) ) == 1 );
}


// ApplyAutoResolutions ------------------------------------------------------

namespace
{

KIID_PATH MakePath( int aSeed )
{
    KIID::SeedGenerator( aSeed );
    return KIID_PATH( wxS( "/" ) + KIID().AsString() );
}


MERGE_PLAN MakePlanWithConflicts( const std::vector<KIID_PATH>& aConflictIds )
{
    MERGE_PLAN plan;

    for( const KIID_PATH& id : aConflictIds )
    {
        ITEM_RESOLUTION r;
        r.id   = id;
        r.kind = ITEM_RES::KEEP;
        plan.actions.push_back( r );
        plan.unresolved.push_back( id );
    }

    return plan;
}

} // namespace


BOOST_AUTO_TEST_CASE( Apply_AllConflictsCovered )
{
    const KIID_PATH idA = MakePath( 1000 );
    const KIID_PATH idB = MakePath( 1001 );

    MERGE_PLAN plan = MakePlanWithConflicts( { idA, idB } );

    std::vector<std::size_t>     conflictIdx = { 0, 1 };
    std::map<wxString, ITEM_RES> autoMap = {
        { idA.AsString(), ITEM_RES::TAKE_OURS },
        { idB.AsString(), ITEM_RES::TAKE_THEIRS },
    };

    auto result = ApplyAutoResolutions( plan, conflictIdx, autoMap );

    BOOST_CHECK( result.status == APPLY_AUTO_RESOLUTIONS_STATUS::COMPLETE );
    BOOST_CHECK_EQUAL( result.appliedCount, 2u );
    BOOST_CHECK( plan.actions[0].kind == ITEM_RES::TAKE_OURS );
    BOOST_CHECK( plan.actions[1].kind == ITEM_RES::TAKE_THEIRS );
    BOOST_CHECK( plan.unresolved.empty() );
}


BOOST_AUTO_TEST_CASE( Apply_PartialCoverageLeavesPlanUntouched )
{
    // One conflict has no matching entry. The plan must NOT be mutated —
    // the caller relies on observing the original plan for fallback
    // reporting after a PARTIAL result.
    const KIID_PATH idA = MakePath( 1002 );
    const KIID_PATH idB = MakePath( 1003 );

    MERGE_PLAN plan = MakePlanWithConflicts( { idA, idB } );

    std::vector<std::size_t>     conflictIdx = { 0, 1 };
    std::map<wxString, ITEM_RES> autoMap = {
        { idA.AsString(), ITEM_RES::TAKE_OURS },
        // idB intentionally missing
    };

    auto result = ApplyAutoResolutions( plan, conflictIdx, autoMap );

    BOOST_CHECK( result.status == APPLY_AUTO_RESOLUTIONS_STATUS::PARTIAL );
    BOOST_CHECK( result.firstMissingId == idB );
    // Both actions should still be at their pre-apply kind.
    BOOST_CHECK( plan.actions[0].kind == ITEM_RES::KEEP );
    BOOST_CHECK( plan.actions[1].kind == ITEM_RES::KEEP );
    BOOST_CHECK_EQUAL( plan.unresolved.size(), 2u );
}


BOOST_AUTO_TEST_CASE( Apply_NoConflicts )
{
    MERGE_PLAN                   plan;
    std::vector<std::size_t>     conflictIdx;
    std::map<wxString, ITEM_RES> autoMap;

    auto result = ApplyAutoResolutions( plan, conflictIdx, autoMap );
    BOOST_CHECK( result.status == APPLY_AUTO_RESOLUTIONS_STATUS::NO_CONFLICTS );
    BOOST_CHECK_EQUAL( result.appliedCount, 0u );
}


// BuildConflictList ---------------------------------------------------------

BOOST_AUTO_TEST_CASE( BuildConflictList_OnlyUnresolvedActions )
{
    // Plan has three actions, but only two are flagged unresolved.
    const KIID_PATH idA = MakePath( 2000 );
    const KIID_PATH idB = MakePath( 2001 );
    const KIID_PATH idC = MakePath( 2002 );

    MERGE_PLAN plan;

    ITEM_RESOLUTION a; a.id = idA; a.kind = ITEM_RES::TAKE_OURS;
    ITEM_RESOLUTION b; b.id = idB; b.kind = ITEM_RES::KEEP;
    ITEM_RESOLUTION c; c.id = idC; c.kind = ITEM_RES::KEEP;
    plan.actions.push_back( a );
    plan.actions.push_back( b );
    plan.actions.push_back( c );
    plan.unresolved.push_back( idB );
    plan.unresolved.push_back( idC );

    auto entries = BuildConflictList( plan );
    BOOST_REQUIRE_EQUAL( entries.size(), 2u );
    BOOST_CHECK_EQUAL( entries[0].actionIndex, 1u );
    BOOST_CHECK( entries[0].id == idB );
    BOOST_CHECK_EQUAL( entries[1].actionIndex, 2u );
    BOOST_CHECK( entries[1].id == idC );
}


BOOST_AUTO_TEST_CASE( BuildConflictList_LongLabelsAreTruncated )
{
    // KIID_PATH strings can get long for nested-sheet hierarchies. The
    // dialog uses a 40-char display rule with right-truncation so the
    // disambiguating segment stays visible.
    KIID::SeedGenerator( 2100 );
    KIID a, b, c, d;
    const wxString longSegmentString = wxS( "/" ) + a.AsString() + wxS( "/" )
                                       + b.AsString() + wxS( "/" )
                                       + c.AsString() + wxS( "/" ) + d.AsString();
    const KIID_PATH longPath( longSegmentString );

    MERGE_PLAN plan;
    ITEM_RESOLUTION r;
    r.id   = longPath;
    r.kind = ITEM_RES::KEEP;
    plan.actions.push_back( r );
    plan.unresolved.push_back( longPath );

    auto entries = BuildConflictList( plan );
    BOOST_REQUIRE_EQUAL( entries.size(), 1u );
    BOOST_CHECK_EQUAL( entries[0].label.Length(), 40u );
    BOOST_CHECK( entries[0].label.StartsWith( wxS( "…" ) ) );
    // Last 39 chars of the original must be at the end of the label.
    BOOST_CHECK( entries[0].label.EndsWith( longPath.AsString().Right( 39 ) ) );
}


BOOST_AUTO_TEST_CASE( BuildConflictList_ShortLabelNotTruncated )
{
    const KIID_PATH shortPath = MakePath( 2200 );

    MERGE_PLAN plan;
    ITEM_RESOLUTION r;
    r.id   = shortPath;
    r.kind = ITEM_RES::KEEP;
    plan.actions.push_back( r );
    plan.unresolved.push_back( shortPath );

    auto entries = BuildConflictList( plan );
    BOOST_REQUIRE_EQUAL( entries.size(), 1u );
    BOOST_CHECK( entries[0].label == shortPath.AsString() );
}


BOOST_AUTO_TEST_CASE( BuildConflictList_EmptyPlan )
{
    MERGE_PLAN plan;
    BOOST_CHECK( BuildConflictList( plan ).empty() );
}


// CollectUnresolvedConflicts -----------------------------------------------

BOOST_AUTO_TEST_CASE( CollectUnresolved_AllConcreteChoices_Empty )
{
    const KIID_PATH idA = MakePath( 3000 );
    const KIID_PATH idB = MakePath( 3001 );

    MERGE_PLAN plan;
    ITEM_RESOLUTION a; a.id = idA; a.kind = ITEM_RES::TAKE_OURS;
    ITEM_RESOLUTION b; b.id = idB; b.kind = ITEM_RES::TAKE_THEIRS;
    plan.actions.push_back( a );
    plan.actions.push_back( b );

    BOOST_CHECK( CollectUnresolvedConflicts( plan, { 0, 1 } ).empty() );
}


BOOST_AUTO_TEST_CASE( CollectUnresolved_KeepIsUnresolved )
{
    const KIID_PATH idA = MakePath( 3100 );

    MERGE_PLAN plan;
    ITEM_RESOLUTION a; a.id = idA; a.kind = ITEM_RES::KEEP;
    plan.actions.push_back( a );

    auto unresolved = CollectUnresolvedConflicts( plan, { 0 } );
    BOOST_REQUIRE_EQUAL( unresolved.size(), 1u );
    BOOST_CHECK( unresolved[0] == idA );
}


BOOST_AUTO_TEST_CASE( CollectUnresolved_MergePropsIsUnresolved )
{
    const KIID_PATH idA = MakePath( 3200 );

    MERGE_PLAN plan;
    ITEM_RESOLUTION a; a.id = idA; a.kind = ITEM_RES::MERGE_PROPS;
    plan.actions.push_back( a );

    auto unresolved = CollectUnresolvedConflicts( plan, { 0 } );
    BOOST_REQUIRE_EQUAL( unresolved.size(), 1u );
    BOOST_CHECK( unresolved[0] == idA );
}


BOOST_AUTO_TEST_CASE( CollectUnresolved_DeleteIsUnresolved )
{
    // DELETE is also engine-internal — the user is supposed to actively
    // pick a TAKE_* side, not accept the engine's default of "delete".
    const KIID_PATH idA = MakePath( 3300 );

    MERGE_PLAN plan;
    ITEM_RESOLUTION a;
    a.id = idA;
    a.kind = ITEM_RES::DELETE_ITEM;
    plan.actions.push_back( a );

    auto unresolved = CollectUnresolvedConflicts( plan, { 0 } );
    BOOST_REQUIRE_EQUAL( unresolved.size(), 1u );
}


BOOST_AUTO_TEST_CASE( CollectUnresolved_OutOfRangeIndexSkipped )
{
    // CollectUnresolvedConflicts is public; a caller can pass a stale index.
    // Pin that out-of-range indices are silently skipped rather than UB.
    const KIID_PATH idA = MakePath( 3350 );

    MERGE_PLAN plan;
    ITEM_RESOLUTION a; a.id = idA; a.kind = ITEM_RES::KEEP;
    plan.actions.push_back( a );

    // Index 0 valid (KEEP -> unresolved), index 99 stale (skipped).
    auto unresolved = CollectUnresolvedConflicts( plan, { 0, 99 } );
    BOOST_REQUIRE_EQUAL( unresolved.size(), 1u );
    BOOST_CHECK( unresolved[0] == idA );
}


BOOST_AUTO_TEST_CASE( Apply_ValidThenStaleIndex_NoMidLoopMutation )
{
    // The staged-write pattern in ApplyAutoResolutions must NOT mutate any
    // action when a later index is stale. A naive implementation that
    // assigned as it iterated would have mutated index 0 before hitting
    // the bad index 99 — pin that the rollback property holds.
    const KIID_PATH idA = MakePath( 3370 );
    const KIID_PATH idB = MakePath( 3371 );

    MERGE_PLAN plan = MakePlanWithConflicts( { idA, idB } );

    std::vector<std::size_t>     conflictIdx = { 0, 99 };   // first valid, second stale
    std::map<wxString, ITEM_RES> autoMap = {
        { idA.AsString(), ITEM_RES::TAKE_OURS },
        { idB.AsString(), ITEM_RES::TAKE_THEIRS },
    };

    auto result = ApplyAutoResolutions( plan, conflictIdx, autoMap );
    BOOST_CHECK( result.status == APPLY_AUTO_RESOLUTIONS_STATUS::PARTIAL );

    // Both actions must still be at their pre-apply kind. If the apply
    // mutated index 0 mid-loop, plan.actions[0].kind would be TAKE_OURS
    // even though the overall call returned PARTIAL.
    BOOST_CHECK( plan.actions[0].kind == ITEM_RES::KEEP );
    BOOST_CHECK( plan.actions[1].kind == ITEM_RES::KEEP );
    BOOST_CHECK_EQUAL( plan.unresolved.size(), 2u );
}


BOOST_AUTO_TEST_CASE( Apply_OutOfRangeIndexBailsAsPartial )
{
    const KIID_PATH idA = MakePath( 3360 );

    MERGE_PLAN plan = MakePlanWithConflicts( { idA } );

    // Index 99 is stale.
    std::vector<std::size_t>     conflictIdx = { 99 };
    std::map<wxString, ITEM_RES> autoMap = {
        { idA.AsString(), ITEM_RES::TAKE_OURS },
    };

    auto result = ApplyAutoResolutions( plan, conflictIdx, autoMap );
    BOOST_CHECK( result.status == APPLY_AUTO_RESOLUTIONS_STATUS::PARTIAL );
    BOOST_CHECK( plan.actions[0].kind == ITEM_RES::KEEP );   // untouched
}


BOOST_AUTO_TEST_CASE( CollectUnresolved_MixedSet )
{
    const KIID_PATH idA = MakePath( 3400 );
    const KIID_PATH idB = MakePath( 3401 );
    const KIID_PATH idC = MakePath( 3402 );

    MERGE_PLAN plan;
    ITEM_RESOLUTION a; a.id = idA; a.kind = ITEM_RES::TAKE_OURS;
    ITEM_RESOLUTION b; b.id = idB; b.kind = ITEM_RES::KEEP;
    ITEM_RESOLUTION c; c.id = idC; c.kind = ITEM_RES::TAKE_ANCESTOR;
    plan.actions.push_back( a );
    plan.actions.push_back( b );
    plan.actions.push_back( c );

    auto unresolved = CollectUnresolvedConflicts( plan, { 0, 1, 2 } );
    BOOST_REQUIRE_EQUAL( unresolved.size(), 1u );
    BOOST_CHECK( unresolved[0] == idB );
}


// ResolveConflictBBox -------------------------------------------------------

namespace
{

const BOX2I VALID_OURS    ( VECTOR2I( 100, 100 ), VECTOR2I( 50, 50 ) );
const BOX2I VALID_THEIRS  ( VECTOR2I( 200, 200 ), VECTOR2I( 50, 50 ) );
const BOX2I VALID_ANCESTOR( VECTOR2I( 300, 300 ), VECTOR2I( 50, 50 ) );
const BOX2I VALID_PRIMARY ( VECTOR2I( 400, 400 ), VECTOR2I( 50, 50 ) );
const BOX2I DEGENERATE    ( VECTOR2I( 0, 0 ),     VECTOR2I( 0, 0 ) );

} // namespace


BOOST_AUTO_TEST_CASE( BBox_PrimaryWinsWhenPresent )
{
    const KIID_PATH id = MakePath( 4000 );

    std::map<KIID_PATH, BOX2I> primary = { { id, VALID_PRIMARY } };
    std::map<KIID_PATH, BOX2I> ours    = { { id, VALID_OURS } };
    std::map<KIID_PATH, BOX2I> theirs  = { { id, VALID_THEIRS } };
    std::map<KIID_PATH, BOX2I> ancestor = { { id, VALID_ANCESTOR } };

    auto bbox = ResolveConflictBBox( id, primary, ours, theirs, ancestor );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK( *bbox == VALID_PRIMARY );
}


BOOST_AUTO_TEST_CASE( BBox_PrimaryEmptyFallsThroughToOurs )
{
    // Caller signals "no preferred side" by passing an empty map for
    // primary (the API no longer accepts nullptr).
    const KIID_PATH id = MakePath( 4001 );

    std::map<KIID_PATH, BOX2I> ours    = { { id, VALID_OURS } };
    std::map<KIID_PATH, BOX2I> theirs  = { { id, VALID_THEIRS } };
    std::map<KIID_PATH, BOX2I> ancestor = { { id, VALID_ANCESTOR } };

    auto bbox = ResolveConflictBBox( id, std::map<KIID_PATH, BOX2I>{}, ours, theirs, ancestor );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK( *bbox == VALID_OURS );
}


BOOST_AUTO_TEST_CASE( BBox_FallsThroughOursMissingToTheirs )
{
    const KIID_PATH id = MakePath( 4002 );

    std::map<KIID_PATH, BOX2I> primary;   // doesn't contain id
    std::map<KIID_PATH, BOX2I> ours;
    std::map<KIID_PATH, BOX2I> theirs   = { { id, VALID_THEIRS } };
    std::map<KIID_PATH, BOX2I> ancestor = { { id, VALID_ANCESTOR } };

    auto bbox = ResolveConflictBBox( id, primary, ours, theirs, ancestor );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK( *bbox == VALID_THEIRS );
}


BOOST_AUTO_TEST_CASE( BBox_FallsThroughToAncestor )
{
    const KIID_PATH id = MakePath( 4003 );

    std::map<KIID_PATH, BOX2I> primary;
    std::map<KIID_PATH, BOX2I> ours;
    std::map<KIID_PATH, BOX2I> theirs;
    std::map<KIID_PATH, BOX2I> ancestor = { { id, VALID_ANCESTOR } };

    auto bbox = ResolveConflictBBox( id, primary, ours, theirs, ancestor );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK( *bbox == VALID_ANCESTOR );
}


BOOST_AUTO_TEST_CASE( BBox_DegenerateBBoxSkipped )
{
    // A side with a zero-area bbox for the id is treated the same as a
    // side with no entry — the next map in the chain wins.
    const KIID_PATH id = MakePath( 4004 );

    std::map<KIID_PATH, BOX2I> primary = { { id, DEGENERATE } };
    std::map<KIID_PATH, BOX2I> ours    = { { id, VALID_OURS } };
    std::map<KIID_PATH, BOX2I> theirs;
    std::map<KIID_PATH, BOX2I> ancestor;

    auto bbox = ResolveConflictBBox( id, primary, ours, theirs, ancestor );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK( *bbox == VALID_OURS );
}


BOOST_AUTO_TEST_CASE( BBox_AllMissingReturnsNullopt )
{
    const KIID_PATH id = MakePath( 4005 );
    std::map<KIID_PATH, BOX2I> empty;

    auto bbox = ResolveConflictBBox( id, std::map<KIID_PATH, BOX2I>{}, empty, empty, empty );
    BOOST_CHECK( !bbox.has_value() );
}


BOOST_AUTO_TEST_CASE( BBox_FallbackOrderOursThenTheirsThenAncestor )
{
    // When primary is empty and ours has a degenerate entry, we should
    // fall through to theirs. This pins the documented fallback order
    // beyond just "first map-with-entry wins" — degenerate entries are
    // equivalent to absence.
    const KIID_PATH id = MakePath( 4006 );

    std::map<KIID_PATH, BOX2I> ours    = { { id, DEGENERATE } };
    std::map<KIID_PATH, BOX2I> theirs  = { { id, VALID_THEIRS } };
    std::map<KIID_PATH, BOX2I> ancestor = { { id, VALID_ANCESTOR } };

    auto bbox = ResolveConflictBBox( id, std::map<KIID_PATH, BOX2I>{}, ours, theirs, ancestor );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK( *bbox == VALID_THEIRS );
}


// BuildConflictDetailText --------------------------------------------------

BOOST_AUTO_TEST_CASE( DetailText_ContainsIdAndKind )
{
    ITEM_RESOLUTION r;
    r.id   = MakePath( 4100 );
    r.kind = ITEM_RES::TAKE_OURS;

    wxString text = BuildConflictDetailText( r );
    BOOST_CHECK( text.Contains( r.id.AsString() ) );

    // Run the comparison strings through `_()` so the assertions stay
    // locale-stable: if a translation is loaded, both the production
    // text and these expected strings go through the same wx machinery
    // and match each other. The label strings (positive + negative pins)
    // identify the kind's labelled prefix without depending on the exact
    // English wording surviving a translation.
    BOOST_CHECK( text.Contains( _( "Take ours" ) ) );
    BOOST_CHECK( !text.Contains( _( "Property-level merge" ) ) );
    BOOST_CHECK( !text.Contains( _( "Keep (default)" ) ) );
}


BOOST_AUTO_TEST_CASE( DetailText_IncludesPropertyCount )
{
    ITEM_RESOLUTION r;
    r.id   = MakePath( 4101 );
    r.kind = ITEM_RES::MERGE_PROPS;

    PROPERTY_RESOLUTION p1; p1.name = wxS( "A" ); r.props.push_back( p1 );
    PROPERTY_RESOLUTION p2; p2.name = wxS( "B" ); r.props.push_back( p2 );

    wxString text = BuildConflictDetailText( r );
    BOOST_CHECK( text.Contains( wxS( "2" ) ) );
}


BOOST_AUTO_TEST_CASE( DetailText_KnownResolutionKindsProduceText )
{
    // Snapshot of the six ITEM_RES values at time of writing. The list
    // is NOT compile-time exhaustive — adding a new enum value won't
    // automatically extend the array (C++ can't enumerate enum values
    // without external libs). If a new ITEM_RES is introduced, add it
    // here and update `BuildConflictDetailText`'s switch in the same
    // commit; otherwise the new kind silently produces a label-less
    // detail block.
    const ITEM_RES kinds[] = {
        ITEM_RES::TAKE_OURS,   ITEM_RES::TAKE_THEIRS, ITEM_RES::TAKE_ANCESTOR,
        ITEM_RES::MERGE_PROPS, ITEM_RES::DELETE_ITEM, ITEM_RES::KEEP,
    };

    // Format the expected count line through the same `_()` machinery the
    // production code uses so the assertion stays locale-stable. A
    // translated harness ends up comparing translated against translated.
    const wxString expectedCountLine = wxString::Format(
            _( "%zu property edit(s) in this resolution.\n" ),
            static_cast<std::size_t>( 0 ) );

    for( ITEM_RES kind : kinds )
    {
        ITEM_RESOLUTION r;
        r.id   = MakePath( 4200 );
        r.kind = kind;

        wxString text = BuildConflictDetailText( r );
        BOOST_CHECK_MESSAGE( text.Length() > 30,
                             "Detail text too short for kind "
                                 << static_cast<int>( kind ) );
        BOOST_CHECK( text.Contains( expectedCountLine ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
