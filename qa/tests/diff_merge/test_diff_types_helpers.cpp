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

#include <diff_merge/kicad_diff_types.h>

#include <map>
#include <set>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( DiffTypesHelpers )


// ---------------------------------------------------------------------------
// LibraryItemKiidPath
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( LibraryItemKiidPath_Deterministic )
{
    // Same input must produce the same output. The whole point of this helper
    // is to replace KIID(string)'s random-UUID fallback, so determinism is
    // load-bearing.
    const KIID_PATH a = LibraryItemKiidPath( wxS( "R_0402" ) );
    const KIID_PATH b = LibraryItemKiidPath( wxS( "R_0402" ) );
    BOOST_CHECK( a == b );
}


BOOST_AUTO_TEST_CASE( LibraryItemKiidPath_PinnedOutput )
{
    // The produced UUID is embedded in persisted diff/merge artifacts and used
    // to match library items across a round-trip. Pin the exact bytes for a
    // known name so a refactor of the underlying hashing can't silently shift
    // them and break matching against existing data.
    const KIID_PATH path = LibraryItemKiidPath( wxS( "R_0402" ) );
    BOOST_REQUIRE_EQUAL( path.size(), 1u );
    BOOST_CHECK_EQUAL( path.front().AsStdString(), "b75bc79b-7a70-e264-53e6-d29f96c4fad4" );
}


BOOST_AUTO_TEST_CASE( LibraryItemKiidPath_DistinctNames )
{
    // Different names must produce different paths. Collision probability
    // is 2^-128, but at least check the common short-string cases.
    BOOST_CHECK( LibraryItemKiidPath( wxS( "R_0402" ) )
                 != LibraryItemKiidPath( wxS( "R_0603" ) ) );
    BOOST_CHECK( LibraryItemKiidPath( wxS( "C_0402" ) )
                 != LibraryItemKiidPath( wxS( "R_0402" ) ) );
    BOOST_CHECK( LibraryItemKiidPath( wxS( "A" ) )
                 != LibraryItemKiidPath( wxS( "B" ) ) );
}


BOOST_AUTO_TEST_CASE( LibraryItemKiidPath_EmptyName )
{
    // Empty name must still produce a valid one-segment path with a non-nil
    // segment. operator== alone would pass for a default-constructed (empty)
    // KIID_PATH, so check size + non-niluuid explicitly.
    const KIID_PATH a = LibraryItemKiidPath( wxS( "" ) );
    const KIID_PATH b = LibraryItemKiidPath( wxS( "" ) );
    BOOST_REQUIRE_EQUAL( a.size(), 1u );
    BOOST_CHECK( a == b );
    BOOST_CHECK( a.front() != NilUuid() );
}


BOOST_AUTO_TEST_CASE( LibraryItemKiidPath_UnicodeName )
{
    // Names with non-ASCII characters should still hash deterministically.
    const KIID_PATH a = LibraryItemKiidPath( wxString::FromUTF8( "电阻_0402" ) );
    const KIID_PATH b = LibraryItemKiidPath( wxString::FromUTF8( "电阻_0402" ) );
    BOOST_CHECK( a == b );

    BOOST_CHECK( a != LibraryItemKiidPath( wxString::FromUTF8( "电阻_0603" ) ) );
}


// ---------------------------------------------------------------------------
// SchScreenSentinelKiid
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( SchScreenSentinelKiid_Stable )
{
    // The sentinel is used to distinguish a SCH_SCREEN-property resolution
    // from a SCH_SHEET symbol that lives at the same sheet path. Two calls
    // must return the SAME value (it's a static-local const).
    const KIID& a = SchScreenSentinelKiid();
    const KIID& b = SchScreenSentinelKiid();
    BOOST_CHECK( a == b );
    BOOST_CHECK( &a == &b );  // returned by const ref to the same static
}


BOOST_AUTO_TEST_CASE( SchScreenSentinelKiid_DistinctFromNiluuid )
{
    // The sentinel was deliberately chosen to be a non-nil v4 UUID so
    // consumers that check niluuid for "no item" still treat it as a real ID.
    BOOST_CHECK( SchScreenSentinelKiid() != NilUuid() );
}


BOOST_AUTO_TEST_CASE( SchScreenSentinelKiid_PinnedLiteral )
{
    // The sentinel is a hard-coded constant in the header — pin its exact
    // value so an accidental change is loud. Any consumer that has stored
    // this UUID in a marker file or test fixture would break silently
    // otherwise.
    BOOST_CHECK_EQUAL( SchScreenSentinelKiid().AsStdString(),
                       "5c50ee00-0000-4000-8000-000000000000" );
}


BOOST_AUTO_TEST_CASE( SchScreenSentinelKiid_DoesntCollideWithGeneratedKiids )
{
    // Defense in depth: the sentinel is v4-shaped, so the generator
    // could in principle return the same UUID. Probability is 2^-128
    // but 1024 samples cheaply guards against the sentinel accidentally
    // sitting in a deterministic output range during testing.
    for( int i = 0; i < 1024; ++i )
    {
        KIID generated;
        BOOST_REQUIRE( generated != SchScreenSentinelKiid() );
    }
}


// ---------------------------------------------------------------------------
// SummarizeSeverities
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( SummarizeSeverities_EmptyMap )
{
    std::map<int, int> empty;
    const std::string  s = SummarizeSeverities( empty );

    // Empty map's hash is 0 (no XOR contributions). Pin both the count and
    // the literal hash term — `s.find("hash ")` alone would match the format
    // string even if hashing was broken.
    BOOST_CHECK( s.find( "0 override(s)" ) != std::string::npos );
    BOOST_CHECK( s.find( "hash 0)" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( SummarizeSeverities_NonEmpty )
{
    std::map<int, int> m{ { 1, 2 }, { 3, 4 } };
    const std::string  s = SummarizeSeverities( m );

    // Tighter pin: the count is correct AND the hash field is non-zero (it
    // would be "hash 0)" if every entry contributed an empty bucket).
    BOOST_CHECK( s.find( "2 override(s)" ) != std::string::npos );
    BOOST_CHECK( s.find( "hash " ) != std::string::npos );
    BOOST_CHECK( s.find( "hash 0)" ) == std::string::npos );
}


BOOST_AUTO_TEST_CASE( SummarizeSeverities_SameContentSameHash )
{
    // The hash component must be deterministic — distinct callers must
    // produce identical strings for identical content (this is what makes
    // the diff value comparable across runs).
    std::map<int, int> a{ { 1, 2 }, { 3, 4 } };
    std::map<int, int> b{ { 1, 2 }, { 3, 4 } };
    BOOST_CHECK_EQUAL( SummarizeSeverities( a ), SummarizeSeverities( b ) );
}


BOOST_AUTO_TEST_CASE( SummarizeSeverities_DifferentContentDifferentHash )
{
    // Same size, different values must produce different hashes. This is the
    // bug the hash was added to catch — "5 override(s)" vs "5 override(s)"
    // with different severities would otherwise render as identical.
    std::map<int, int> a{ { 1, 2 } };
    std::map<int, int> b{ { 1, 3 } };
    BOOST_CHECK( SummarizeSeverities( a ) != SummarizeSeverities( b ) );
}


BOOST_AUTO_TEST_CASE( SummarizeSeverities_OrderInvariant )
{
    // std::map iterates in key order, so insertion order shouldn't matter.
    std::map<int, int> a;
    a[3] = 30;
    a[1] = 10;

    std::map<int, int> b;
    b[1] = 10;
    b[3] = 30;

    BOOST_CHECK_EQUAL( SummarizeSeverities( a ), SummarizeSeverities( b ) );
}


// ---------------------------------------------------------------------------
// DOC_PROP constants are unique
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( DocPropConstants_AllDistinct )
{
    // A typo on one side that accidentally equals another constant would
    // silently desync PCB and SCH. Verify all DOC_PROP_* strings are
    // distinct.
    const std::set<wxString> all = {
        DOC_PROP_PAGE_FORMAT,
        DOC_PROP_PAGE_ORIENTATION,
        DOC_PROP_BOARD_THICKNESS,
        DOC_PROP_LAYER_STACKUP,
        DOC_PROP_DRC_SEVERITIES,
        DOC_PROP_ERC_SEVERITIES,
        DOC_PROP_DRAWING_SHEET,
    };

    BOOST_CHECK_EQUAL( all.size(), 7 );
}


BOOST_AUTO_TEST_CASE( DocPropConstants_PinnedValues )
{
    // These names are the document-level delta keys written into .kicad_merge
    // markers; changing a value silently breaks marker compatibility. Pin the
    // exact strings (also catches a single empty/typo that _AllDistinct misses).
    BOOST_CHECK( DOC_PROP_PAGE_FORMAT      == wxString( "Page Format" ) );
    BOOST_CHECK( DOC_PROP_PAGE_ORIENTATION == wxString( "Page Orientation" ) );
    BOOST_CHECK( DOC_PROP_BOARD_THICKNESS  == wxString( "Board Thickness" ) );
    BOOST_CHECK( DOC_PROP_LAYER_STACKUP    == wxString( "Layer Stackup" ) );
    BOOST_CHECK( DOC_PROP_DRC_SEVERITIES   == wxString( "DRC Severity Overrides" ) );
    BOOST_CHECK( DOC_PROP_ERC_SEVERITIES   == wxString( "ERC Severity Overrides" ) );
    BOOST_CHECK( DOC_PROP_DRAWING_SHEET    == wxString( "Drawing Sheet File" ) );
}


// ---------------------------------------------------------------------------
// Side-effect predicates (ChangeInvalidatesZone / ChangeRequiresConnectivityRebuild)
// ---------------------------------------------------------------------------

namespace
{

ITEM_CHANGE MakeChange( const wxString& aTypeName )
{
    ITEM_CHANGE c;
    c.typeName = aTypeName;
    c.kind = CHANGE_KIND::MODIFIED;
    return c;
}

} // namespace


BOOST_AUTO_TEST_CASE( ChangeInvalidatesZone_CopperConductors )
{
    BOOST_CHECK( ChangeInvalidatesZone( MakeChange( wxS( "ZONE" ) ) ) );
    BOOST_CHECK( ChangeInvalidatesZone( MakeChange( wxS( "PCB_TRACK" ) ) ) );
    BOOST_CHECK( ChangeInvalidatesZone( MakeChange( wxS( "PCB_ARC" ) ) ) );
    BOOST_CHECK( ChangeInvalidatesZone( MakeChange( wxS( "PCB_VIA" ) ) ) );
    BOOST_CHECK( ChangeInvalidatesZone( MakeChange( wxS( "FOOTPRINT" ) ) ) );
    BOOST_CHECK( ChangeInvalidatesZone( MakeChange( wxS( "PAD" ) ) ) );
}


BOOST_AUTO_TEST_CASE( ChangeInvalidatesZone_NonCopperRejected )
{
    // Items the predicate deliberately leaves out — see the header doc for
    // why PCB_TEXT/PCB_SHAPE are excluded (their zone-affecting status
    // depends on layer, which this type-only predicate can't see).
    BOOST_CHECK( !ChangeInvalidatesZone( MakeChange( wxS( "PCB_TEXT" ) ) ) );
    BOOST_CHECK( !ChangeInvalidatesZone( MakeChange( wxS( "PCB_DIMENSION" ) ) ) );
    BOOST_CHECK( !ChangeInvalidatesZone( MakeChange( wxS( "SCH_LABEL" ) ) ) );
    BOOST_CHECK( !ChangeInvalidatesZone( MakeChange( wxS( "" ) ) ) );
    // Case-sensitive — a typo on the type name must not silently slip in.
    BOOST_CHECK( !ChangeInvalidatesZone( MakeChange( wxS( "pcb_track" ) ) ) );
}


BOOST_AUTO_TEST_CASE( ChangeRequiresConnectivityRebuild_NetCarriers )
{
    // The full set of GetClass() names the differ emits that carry nets or
    // act as connection markers. Missing any of these means the merge engine
    // saves a board / schematic with a stale netlist — pin them all here so
    // the predicate can't silently drop one in a future refactor.
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "PCB_TRACK" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "PCB_ARC" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "PCB_VIA" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "PCB_SHAPE" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "PAD" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_LINE" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_PIN" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_SYMBOL" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_LABEL" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_GLOBALLABEL" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_HIERLABEL" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_DIRECTIVE_LABEL" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_JUNCTION" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_NO_CONNECT" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_SHEET_PIN" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_BUS_WIRE_ENTRY" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_BUS_BUS_ENTRY" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_SHEET" ) ) ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_RULE_AREA" ) ) ) );
}


BOOST_AUTO_TEST_CASE( ChangeRequiresConnectivityRebuild_NonNetItemsRejected )
{
    // ZONE membership flips connectivity at fill-time but not via this
    // predicate — the engine uses ChangeInvalidatesZone for that path.
    // FOOTPRINT changes that move pads are handled by the PAD entry above.
    // PCB_TEXT changes do not carry nets even on copper.
    BOOST_CHECK( !ChangeRequiresConnectivityRebuild( MakeChange( wxS( "ZONE" ) ) ) );
    BOOST_CHECK( !ChangeRequiresConnectivityRebuild( MakeChange( wxS( "FOOTPRINT" ) ) ) );
    BOOST_CHECK( !ChangeRequiresConnectivityRebuild( MakeChange( wxS( "PCB_TEXT" ) ) ) );
    BOOST_CHECK( !ChangeRequiresConnectivityRebuild( MakeChange( wxS( "SCH_BITMAP" ) ) ) );
    BOOST_CHECK( !ChangeRequiresConnectivityRebuild( MakeChange( wxS( "" ) ) ) );
}


BOOST_AUTO_TEST_CASE( SideEffectPredicates_IndependentOfChangeKind )
{
    // The predicates dispatch on typeName only — kind (ADDED/MODIFIED/REMOVED)
    // must not influence the result. A removed track still invalidates zones.
    ITEM_CHANGE removed;
    removed.typeName = wxS( "PCB_TRACK" );
    removed.kind = CHANGE_KIND::REMOVED;
    BOOST_CHECK( ChangeInvalidatesZone( removed ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( removed ) );

    ITEM_CHANGE added;
    added.typeName = wxS( "PCB_TRACK" );
    added.kind = CHANGE_KIND::ADDED;
    BOOST_CHECK( ChangeInvalidatesZone( added ) );
    BOOST_CHECK( ChangeRequiresConnectivityRebuild( added ) );
}


// ---------------------------------------------------------------------------
// IndexChangesByKiid / IndexPropertiesByName
// ---------------------------------------------------------------------------

namespace
{

ITEM_CHANGE MakeChangeWithId( const wxString& aTypeName, const wxString& aIdSegment )
{
    ITEM_CHANGE c;
    c.typeName = aTypeName;
    c.kind = CHANGE_KIND::MODIFIED;
    c.id = KIID_PATH( wxS( "/" ) + KIID().AsString() );
    // Stash the id segment in refdes so tests can identify which change came
    // back from the index without depending on KIID equality across copies.
    c.refdes = aIdSegment;
    return c;
}

} // namespace


BOOST_AUTO_TEST_CASE( IndexChangesByKiid_FlatList )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChangeWithId( wxS( "FOOTPRINT" ), wxS( "F1" ) ) );
    diff.changes.push_back( MakeChangeWithId( wxS( "ZONE" ),      wxS( "Z1" ) ) );

    const auto index = IndexChangesByKiid( diff );

    BOOST_REQUIRE_EQUAL( index.size(), 2u );
    BOOST_CHECK( index.count( diff.changes[0].id ) == 1 );
    BOOST_CHECK( index.count( diff.changes[1].id ) == 1 );
    BOOST_CHECK( index.at( diff.changes[0].id ) == &diff.changes[0] );
}


BOOST_AUTO_TEST_CASE( IndexChangesByKiid_RecursesIntoChildren )
{
    // A footprint with a pad-level child change. Without recursive walking,
    // the pad's id never appears in the index and conflict detection on
    // intra-footprint edits would silently auto-merge — exactly the bug the
    // recursion was added to prevent.
    DOCUMENT_DIFF diff;
    ITEM_CHANGE   fp = MakeChangeWithId( wxS( "FOOTPRINT" ), wxS( "FP" ) );
    fp.children.push_back( MakeChangeWithId( wxS( "PAD" ), wxS( "P1" ) ) );
    fp.children.push_back( MakeChangeWithId( wxS( "PAD" ), wxS( "P2" ) ) );
    diff.changes.push_back( std::move( fp ) );

    const auto index = IndexChangesByKiid( diff );

    BOOST_REQUIRE_EQUAL( index.size(), 3u );
    BOOST_CHECK( index.at( diff.changes[0].id )->typeName == wxS( "FOOTPRINT" ) );
    BOOST_CHECK( index.at( diff.changes[0].children[0].id )->typeName == wxS( "PAD" ) );
    BOOST_CHECK( index.at( diff.changes[0].children[1].id )->typeName == wxS( "PAD" ) );
}


BOOST_AUTO_TEST_CASE( IndexChangesByKiid_DeepNesting )
{
    // Three levels deep. Tests recursion depth handling.
    DOCUMENT_DIFF diff;
    ITEM_CHANGE   level1 = MakeChangeWithId( wxS( "SHEET" ), wxS( "L1" ) );
    ITEM_CHANGE   level2 = MakeChangeWithId( wxS( "SYMBOL" ), wxS( "L2" ) );
    ITEM_CHANGE   level3 = MakeChangeWithId( wxS( "FIELD" ), wxS( "L3" ) );
    level2.children.push_back( std::move( level3 ) );
    level1.children.push_back( std::move( level2 ) );
    diff.changes.push_back( std::move( level1 ) );

    const auto index = IndexChangesByKiid( diff );
    BOOST_CHECK_EQUAL( index.size(), 3u );
}


BOOST_AUTO_TEST_CASE( IndexChangesByKiid_DuplicateKiidLastWins )
{
    // KIID_PATHs are unique by construction in real diffs, but a corrupt or
    // hand-crafted diff with collisions falls through the recursive walk:
    // operator[] last-write-wins is the documented contract. Pin it so a
    // future refactor that flips to first-wins (or asserts) is loud.
    const KIID_PATH sharedId = KIID_PATH( wxS( "/" ) + KIID().AsString() );

    DOCUMENT_DIFF diff;

    ITEM_CHANGE first;
    first.id       = sharedId;
    first.typeName = wxS( "FIRST" );
    first.kind     = CHANGE_KIND::MODIFIED;
    diff.changes.push_back( first );

    ITEM_CHANGE second;
    second.id       = sharedId;
    second.typeName = wxS( "SECOND" );
    second.kind     = CHANGE_KIND::MODIFIED;
    diff.changes.push_back( second );

    const auto index = IndexChangesByKiid( diff );
    BOOST_REQUIRE_EQUAL( index.size(), 1u );
    BOOST_CHECK( index.at( sharedId )->typeName == wxS( "SECOND" ) );
}


BOOST_AUTO_TEST_CASE( IndexChangesByKiid_EmptyDiff )
{
    DOCUMENT_DIFF diff;
    BOOST_CHECK( IndexChangesByKiid( diff ).empty() );
}


BOOST_AUTO_TEST_CASE( IndexPropertiesByName_FlatList )
{
    ITEM_CHANGE c;
    PROPERTY_DELTA p1;
    p1.name = wxS( "Width" );
    p1.before = DIFF_VALUE::FromInt( 10 );
    p1.after  = DIFF_VALUE::FromInt( 20 );
    c.properties.push_back( p1 );

    PROPERTY_DELTA p2;
    p2.name = wxS( "Layer" );
    p2.before = DIFF_VALUE::FromInt( 1 );
    p2.after  = DIFF_VALUE::FromInt( 2 );
    c.properties.push_back( p2 );

    const auto index = IndexPropertiesByName( c );

    BOOST_REQUIRE_EQUAL( index.size(), 2u );
    BOOST_CHECK( index.count( wxS( "Width" ) ) == 1 );
    BOOST_CHECK( index.count( wxS( "Layer" ) ) == 1 );
    BOOST_CHECK( index.at( wxS( "Width" ) ) == &c.properties[0] );
}


BOOST_AUTO_TEST_CASE( IndexPropertiesByName_EmptyChange )
{
    ITEM_CHANGE c;
    BOOST_CHECK( IndexPropertiesByName( c ).empty() );
}


BOOST_AUTO_TEST_SUITE_END()
