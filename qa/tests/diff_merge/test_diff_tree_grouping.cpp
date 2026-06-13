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

#include <diff_merge/diff_tree_grouping.h>


using namespace KICAD_DIFF;


namespace
{

ITEM_CHANGE MakeChange( const wxString& aType, CHANGE_KIND aKind, std::optional<wxString> aRefdes = std::nullopt )
{
    ITEM_CHANGE c;
    c.id = KIID_PATH( wxS( "/" ) + KIID().AsString() );
    c.typeName = aType;
    c.kind = aKind;
    c.refdes = std::move( aRefdes );
    return c;
}


std::array<bool, CATEGORY_COUNT> AllVisible()
{
    std::array<bool, CATEGORY_COUNT> out{};
    out.fill( true );
    return out;
}

} // namespace


BOOST_AUTO_TEST_SUITE( DiffTreeGrouping )


// ChangeKindLabel ----------------------------------------------------------

BOOST_AUTO_TEST_CASE( ChangeKindLabel_AllKinds )
{
    // Pin that every CHANGE_KIND produces a non-empty label. Locale-dependent
    // text content can't be asserted strictly but emptiness CAN.
    BOOST_CHECK( !ChangeKindLabel( CHANGE_KIND::ADDED ).IsEmpty() );
    BOOST_CHECK( !ChangeKindLabel( CHANGE_KIND::REMOVED ).IsEmpty() );
    BOOST_CHECK( !ChangeKindLabel( CHANGE_KIND::MODIFIED ).IsEmpty() );
    BOOST_CHECK( !ChangeKindLabel( CHANGE_KIND::COLLISION ).IsEmpty() );
    BOOST_CHECK( !ChangeKindLabel( CHANGE_KIND::DUPLICATE_UUID ).IsEmpty() );
}


// ChangeMatchesSearchFilter ------------------------------------------------

BOOST_AUTO_TEST_CASE( Filter_EmptyMatchesEverything )
{
    ITEM_CHANGE c = MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::MODIFIED );
    BOOST_CHECK( ChangeMatchesSearchFilter( c, wxS( "" ) ) );
}


BOOST_AUTO_TEST_CASE( Filter_TypeNameSubstring )
{
    ITEM_CHANGE c = MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::MODIFIED );
    BOOST_CHECK( ChangeMatchesSearchFilter( c, wxS( "track" ) ) );
    BOOST_CHECK( ChangeMatchesSearchFilter( c, wxS( "pcb" ) ) );
    BOOST_CHECK( !ChangeMatchesSearchFilter( c, wxS( "via" ) ) );
}


BOOST_AUTO_TEST_CASE( Filter_RefdesSubstring )
{
    ITEM_CHANGE c = MakeChange( wxS( "FOOTPRINT" ), CHANGE_KIND::MODIFIED, wxS( "R12" ) );
    BOOST_CHECK( ChangeMatchesSearchFilter( c, wxS( "r12" ) ) );
    BOOST_CHECK( ChangeMatchesSearchFilter( c, wxS( "r" ) ) );
    BOOST_CHECK( !ChangeMatchesSearchFilter( c, wxS( "c4" ) ) );
}


BOOST_AUTO_TEST_CASE( Filter_AbsentRefdesNoMatch )
{
    // No refdes -> only typeName can match. A filter that would match a
    // hypothetical refdes must not accidentally hit on a missing optional.
    ITEM_CHANGE c = MakeChange( wxS( "ZONE" ), CHANGE_KIND::ADDED );
    BOOST_CHECK( !ChangeMatchesSearchFilter( c, wxS( "u1" ) ) );
}


BOOST_AUTO_TEST_CASE( Filter_AssumesLowercaseInput )
{
    // The function explicitly requires the caller to pre-lowercase the
    // filter (otherwise repeated calls would re-lowercase the same string
    // for every item). Pin that contract: uppercase filter does NOT match.
    ITEM_CHANGE c = MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::MODIFIED );
    BOOST_CHECK( !ChangeMatchesSearchFilter( c, wxS( "TRACK" ) ) );
}


// BuildChangeTreeGroups ----------------------------------------------------

BOOST_AUTO_TEST_CASE( Groups_EmptyDiffProducesEmptyResult )
{
    DOCUMENT_DIFF diff;
    auto          groups = BuildChangeTreeGroups( diff, wxS( "" ), AllVisible() );
    BOOST_CHECK( groups.empty() );
}


BOOST_AUTO_TEST_CASE( Groups_BucketsByKind )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "PCB_VIA" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::REMOVED ) );

    auto groups = BuildChangeTreeGroups( diff, wxS( "" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 2u );

    // std::map iterates in key (enum value) order; ADDED < REMOVED.
    BOOST_CHECK( groups[0].kind == CHANGE_KIND::ADDED );
    BOOST_CHECK_EQUAL( groups[0].entries.size(), 2u );
    BOOST_CHECK( groups[1].kind == CHANGE_KIND::REMOVED );
    BOOST_CHECK_EQUAL( groups[1].entries.size(), 1u );
}


BOOST_AUTO_TEST_CASE( Groups_RecursesIntoChildren )
{
    // Footprint with two child pad changes; each child must land in its
    // own bucket entry so per-pad highlighting works in the dialog.
    DOCUMENT_DIFF diff;
    ITEM_CHANGE   fp = MakeChange( wxS( "FOOTPRINT" ), CHANGE_KIND::MODIFIED );
    fp.children.push_back( MakeChange( wxS( "PAD" ), CHANGE_KIND::MODIFIED ) );
    fp.children.push_back( MakeChange( wxS( "PAD" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( std::move( fp ) );

    auto groups = BuildChangeTreeGroups( diff, wxS( "" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 2u );

    // ADDED bucket: the added-pad child.
    BOOST_CHECK( groups[0].kind == CHANGE_KIND::ADDED );
    BOOST_CHECK_EQUAL( groups[0].entries.size(), 1u );

    // MODIFIED bucket: the parent footprint + the modified-pad child.
    BOOST_CHECK( groups[1].kind == CHANGE_KIND::MODIFIED );
    BOOST_CHECK_EQUAL( groups[1].entries.size(), 2u );
}


BOOST_AUTO_TEST_CASE( Groups_CollapsesSameNetRoutingChanges )
{
    DOCUMENT_DIFF diff;

    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::MODIFIED, wxS( "Net-(U1-Pad1)" ) ) );
    diff.changes.push_back( MakeChange( wxS( "PCB_VIA" ), CHANGE_KIND::MODIFIED, wxS( "Net-(U1-Pad1)" ) ) );
    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::MODIFIED, wxS( "GND" ) ) );

    auto groups = BuildChangeTreeGroups( diff, wxS( "" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_REQUIRE_EQUAL( groups[0].entries.size(), 2u );
    BOOST_CHECK( groups[0].entries[0].itemLabel == wxS( "NET [GND]" ) );
    BOOST_CHECK( groups[0].entries[1].itemLabel == wxS( "NET [Net-(U1-Pad1)]" ) );
}


BOOST_AUTO_TEST_CASE( Groups_LabelIncludesRefdes )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "FOOTPRINT" ), CHANGE_KIND::ADDED, wxS( "U7" ) ) );

    auto groups = BuildChangeTreeGroups( diff, wxS( "" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_REQUIRE_EQUAL( groups[0].entries.size(), 1u );
    BOOST_CHECK( groups[0].entries[0].itemLabel == wxS( "FOOTPRINT [U7]" ) );
}


BOOST_AUTO_TEST_CASE( Groups_LabelBaseWhenRefdesAbsent )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::ADDED ) );

    auto groups = BuildChangeTreeGroups( diff, wxS( "" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_CHECK( groups[0].entries[0].itemLabel == wxS( "ZONE" ) );
}


BOOST_AUTO_TEST_CASE( Groups_HiddenCategoryDropsBucket )
{
    // ADDED maps to CATEGORY::ADDED. Hide it; the bucket must vanish.
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::REMOVED ) );

    std::array<bool, CATEGORY_COUNT> visible{};
    visible.fill( true );
    visible[static_cast<std::size_t>( CATEGORY::ADDED )] = false;

    auto groups = BuildChangeTreeGroups( diff, wxS( "" ), visible );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_CHECK( groups[0].kind == CHANGE_KIND::REMOVED );
}


BOOST_AUTO_TEST_CASE( Groups_CollisionAndDuplicateBothMapToConflictCategory )
{
    // Hiding CATEGORY::CONFLICT must drop both kinds at once.
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::COLLISION ) );
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::DUPLICATE_UUID ) );
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::MODIFIED ) );

    std::array<bool, CATEGORY_COUNT> visible{};
    visible.fill( true );
    visible[static_cast<std::size_t>( CATEGORY::CONFLICT )] = false;

    auto groups = BuildChangeTreeGroups( diff, wxS( "" ), visible );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_CHECK( groups[0].kind == CHANGE_KIND::MODIFIED );
}


BOOST_AUTO_TEST_CASE( Groups_SearchFilterDropsNonMatching )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::MODIFIED ) );
    diff.changes.push_back( MakeChange( wxS( "PCB_VIA" ), CHANGE_KIND::MODIFIED ) );
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::MODIFIED ) );

    // Filter for "track" — only PCB_TRACK matches.
    auto groups = BuildChangeTreeGroups( diff, wxS( "track" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_REQUIRE_EQUAL( groups[0].entries.size(), 1u );
    BOOST_CHECK( groups[0].entries[0].change->typeName == wxS( "PCB_TRACK" ) );
}


BOOST_AUTO_TEST_CASE( Groups_FilterIsCaseInsensitive )
{
    // User input doesn't have to be lowercased — `BuildChangeTreeGroups`
    // lowercases internally. This is the contract distinguishing the
    // higher-level function from `ChangeMatchesSearchFilter` (which
    // requires pre-lowercased input).
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::ADDED ) );

    auto groups = BuildChangeTreeGroups( diff, wxS( "TRACK" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_CHECK_EQUAL( groups[0].entries.size(), 1u );
}


BOOST_AUTO_TEST_CASE( Groups_GroupLabelShowsTotal_NoFilter )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::ADDED ) );

    auto groups = BuildChangeTreeGroups( diff, wxS( "" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    // "Kind (3)" format when there's no filter.
    BOOST_CHECK( groups[0].groupLabel.EndsWith( wxS( "(3)" ) ) );
}


BOOST_AUTO_TEST_CASE( Groups_GroupLabelShowsVisibleSlashTotal_WithFilter )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "PCB_VIA" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "ZONE" ), CHANGE_KIND::ADDED ) );

    // Filter matches only PCB_TRACK -> 1 of 3 visible.
    auto groups = BuildChangeTreeGroups( diff, wxS( "track" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_CHECK( groups[0].groupLabel.EndsWith( wxS( "(1/3)" ) ) );
}


BOOST_AUTO_TEST_CASE( Groups_GroupLabelOmitsSlashWhenFilterMatchesAll )
{
    // Filter matches every item in the group — the V/N form would just
    // be N/N, which is noisy. The function falls back to (N) in that case.
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::ADDED ) );

    auto groups = BuildChangeTreeGroups( diff, wxS( "track" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_CHECK( groups[0].groupLabel.EndsWith( wxS( "(2)" ) ) );
}


BOOST_AUTO_TEST_CASE( Groups_EmptyBucketAfterFilterIsOmitted )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back( MakeChange( wxS( "PCB_TRACK" ), CHANGE_KIND::ADDED ) );
    diff.changes.push_back( MakeChange( wxS( "PCB_VIA" ), CHANGE_KIND::REMOVED ) );

    // Filter matches only the ADDED bucket; the REMOVED bucket must
    // disappear (UI doesn't show empty groups).
    auto groups = BuildChangeTreeGroups( diff, wxS( "track" ), AllVisible() );
    BOOST_REQUIRE_EQUAL( groups.size(), 1u );
    BOOST_CHECK( groups[0].kind == CHANGE_KIND::ADDED );
}


BOOST_AUTO_TEST_SUITE_END()
