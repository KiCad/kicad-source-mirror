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

#include <diff_merge/lib_merge_applier.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <wx/string.h>


using namespace KICAD_DIFF;


namespace
{

// Minimal copy-constructible stand-in. The applier only does
// std::make_unique<ITEM>(*src) on the chosen side, so anything
// copyable works — we just stamp a side tag so the test can verify
// which side flowed through.
struct TestItem
{
    wxString name;
    wxString side;   // "anc" / "ours" / "theirs" — set by the fixture, never copied from
};


using ItemStore = std::vector<TestItem>;
using ItemMap   = std::map<wxString, const TestItem*>;


ItemMap MakeMap( ItemStore& aStore )
{
    ItemMap m;

    for( const TestItem& it : aStore )
        m[it.name] = &it;

    return m;
}


ITEM_RESOLUTION Action( const wxString& aName, ITEM_RES aKind )
{
    ITEM_RESOLUTION r;
    r.id = LibraryItemKiidPath( aName );
    r.kind = aKind;
    return r;
}


const TestItem* Find( const std::vector<std::unique_ptr<TestItem>>& aOut, const wxString& aName )
{
    for( const auto& up : aOut )
    {
        if( up && up->name == aName )
            return up.get();
    }

    return nullptr;
}

} // namespace


BOOST_AUTO_TEST_SUITE( LibMergeApplier )


BOOST_AUTO_TEST_CASE( TakeOurs_PicksOursSide )
{
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::TAKE_OURS ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    BOOST_CHECK( out[0]->name == wxS( "R" ) );
    BOOST_CHECK( out[0]->side == wxS( "ours" ) );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsTakenOurs, 1 );
}


BOOST_AUTO_TEST_CASE( TakeTheirs_PicksTheirsSide )
{
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::TAKE_THEIRS ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    BOOST_CHECK( out[0]->side == wxS( "theirs" ) );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsTakenTheirs, 1 );
}


BOOST_AUTO_TEST_CASE( TakeAncestor_PicksAncestorSide )
{
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::TAKE_ANCESTOR ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    BOOST_CHECK( out[0]->side == wxS( "anc" ) );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsTakenAncestor, 1 );
}


BOOST_AUTO_TEST_CASE( TakeOurs_MissingInOurs_ErasesFromOutput )
{
    // TAKE_OURS where ours doesn't have the item — applier must erase
    // the live entry rather than leaving the ancestor copy.
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours;   // empty
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::TAKE_OURS ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_CHECK( out.empty() );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsTakenOurs, 1 );
}


BOOST_AUTO_TEST_CASE( Delete_RemovesItem )
{
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::DELETE_ITEM ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_CHECK( out.empty() );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsDeleted, 1 );
}


BOOST_AUTO_TEST_CASE( Keep_PrefersAncestor )
{
    // KEEP with ancestor present should pick ancestor.
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::KEEP ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    BOOST_CHECK( out[0]->side == wxS( "anc" ) );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsKept, 1 );
}


BOOST_AUTO_TEST_CASE( Keep_FallsBackToOurs_WhenAncestorMissing )
{
    // The both-added case: no ancestor, item exists on both sides.
    // KEEP walks anc -> ours -> theirs so ours wins.
    ItemStore anc;
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::KEEP ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    BOOST_CHECK( out[0]->side == wxS( "ours" ) );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsKept, 1 );
}


BOOST_AUTO_TEST_CASE( Keep_FallsBackToTheirs_WhenAncestorAndOursMissing )
{
    // Only theirs has the item. KEEP should still produce it (otherwise an
    // independent same-name add on theirs vanishes — exactly the documented
    // bug the fallback walk guards against).
    ItemStore anc;
    ItemStore ours;
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::KEEP ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    BOOST_CHECK( out[0]->side == wxS( "theirs" ) );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsKept, 1 );
}


BOOST_AUTO_TEST_CASE( Keep_AllSidesMissing_ProducesNothing )
{
    // Defensive: a KEEP resolution for an item that's in none of the maps.
    // Should not crash; should produce no output and still bump itemsKept.
    ItemStore anc, ours, theirs;

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "Ghost" ), ITEM_RES::KEEP ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_CHECK( out.empty() );
    // The action is keyed by KIID_PATH; with no item present in any map,
    // the action is unreachable (allNames is empty). itemsKept stays 0.
    BOOST_CHECK_EQUAL( applier.GetReport().itemsKept, 0 );
}


BOOST_AUTO_TEST_CASE( MergeProps_FallsBackToOurs_AndTracksId )
{
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::MERGE_PROPS ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    BOOST_CHECK( out[0]->side == wxS( "ours" ) );
    BOOST_CHECK_EQUAL( applier.GetReport().mergePropsFallback, 1 );
    BOOST_REQUIRE_EQUAL( applier.GetReport().mergePropsFallbackIds.size(), 1 );
    BOOST_CHECK( applier.GetReport().mergePropsFallbackIds[0]
                 == LibraryItemKiidPath( wxS( "R" ) ) );
}


BOOST_AUTO_TEST_CASE( NoActionForItem_PassesAncestorThrough )
{
    // An item present in ancestor but with no plan action stays in `live`
    // because the applier seeds `live = m_ancestor` and only mutates entries
    // that have a matching action.
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) } };
    ItemStore theirs;

    MERGE_PLAN plan;   // empty — no actions

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    BOOST_CHECK( out[0]->side == wxS( "anc" ) );
}


BOOST_AUTO_TEST_CASE( ActionForUnknownName_IsHarmless )
{
    // A plan action whose id doesn't match any item in any map should be
    // silently ignored (allNames doesn't contain it, so the inner loop
    // never visits that action).
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours, theirs;

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "Phantom" ), ITEM_RES::DELETE_ITEM ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );   // R passed through
    BOOST_CHECK( out[0]->name == wxS( "R" ) );
    BOOST_CHECK_EQUAL( applier.GetReport().itemsDeleted, 0 );
}


BOOST_AUTO_TEST_CASE( OutputSortedByName )
{
    // live is std::map<wxString, ...>, so output should be in name order.
    ItemStore anc{ { wxS( "C" ), wxS( "anc" ) }, { wxS( "A" ), wxS( "anc" ) },
                   { wxS( "B" ), wxS( "anc" ) } };
    ItemStore ours, theirs;

    MERGE_PLAN plan;   // no actions — pure pass-through

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 3 );
    BOOST_CHECK( out[0]->name == wxS( "A" ) );
    BOOST_CHECK( out[1]->name == wxS( "B" ) );
    BOOST_CHECK( out[2]->name == wxS( "C" ) );
}


BOOST_AUTO_TEST_CASE( OutputIsFreshCopy_NotPointerAlias )
{
    // Applier returns owned unique_ptrs via std::make_unique<ITEM>(*src).
    // Mutating the source map's item after Apply() must not affect output.
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours, theirs;

    MERGE_PLAN plan;   // no actions

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_REQUIRE_EQUAL( out.size(), 1 );
    anc[0].side = wxS( "MUTATED" );
    BOOST_CHECK( out[0]->side == wxS( "anc" ) );
}


BOOST_AUTO_TEST_CASE( MixedActions_AllCountersAdvance )
{
    // One action of each kind in a single plan. Verifies counters increment
    // independently and the same applier instance can handle a mixed plan.
    ItemStore anc{
        { wxS( "Keep" ),       wxS( "anc" ) },
        { wxS( "DelMe" ),      wxS( "anc" ) },
        { wxS( "Anc" ),        wxS( "anc" ) },
        { wxS( "MergeProps" ), wxS( "anc" ) },
    };
    ItemStore ours{
        { wxS( "Keep" ),       wxS( "ours" ) },
        { wxS( "DelMe" ),      wxS( "ours" ) },
        { wxS( "Anc" ),        wxS( "ours" ) },
        { wxS( "OursOnly" ),   wxS( "ours" ) },
        { wxS( "MergeProps" ), wxS( "ours" ) },
    };
    ItemStore theirs{
        { wxS( "Keep" ),       wxS( "theirs" ) },
        { wxS( "TheirsOnly" ), wxS( "theirs" ) },
        { wxS( "MergeProps" ), wxS( "theirs" ) },
    };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "Keep" ),       ITEM_RES::KEEP ) );
    plan.actions.push_back( Action( wxS( "DelMe" ), ITEM_RES::DELETE_ITEM ) );
    plan.actions.push_back( Action( wxS( "Anc" ),        ITEM_RES::TAKE_ANCESTOR ) );
    plan.actions.push_back( Action( wxS( "OursOnly" ),   ITEM_RES::TAKE_OURS ) );
    plan.actions.push_back( Action( wxS( "TheirsOnly" ), ITEM_RES::TAKE_THEIRS ) );
    plan.actions.push_back( Action( wxS( "MergeProps" ), ITEM_RES::MERGE_PROPS ) );

    ItemMap ancMap = MakeMap( anc );

    ItemMap oursMap = MakeMap( ours );

    ItemMap theirsMap = MakeMap( theirs );

    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    const auto& r = applier.GetReport();
    BOOST_CHECK_EQUAL( r.itemsKept,           1 );
    BOOST_CHECK_EQUAL( r.itemsDeleted,        1 );
    BOOST_CHECK_EQUAL( r.itemsTakenAncestor,  1 );
    BOOST_CHECK_EQUAL( r.itemsTakenOurs,      1 );
    BOOST_CHECK_EQUAL( r.itemsTakenTheirs,    1 );
    BOOST_CHECK_EQUAL( r.mergePropsFallback,  1 );

    // Five live items: Keep, Anc, OursOnly, TheirsOnly, MergeProps; DelMe was
    // deleted. Pinning the exact size catches accidental over-emission.
    BOOST_REQUIRE_EQUAL( out.size(), 5 );

    const TestItem* keep       = Find( out, wxS( "Keep" ) );
    const TestItem* anc_pick   = Find( out, wxS( "Anc" ) );
    const TestItem* ours_only  = Find( out, wxS( "OursOnly" ) );
    const TestItem* theirs_one = Find( out, wxS( "TheirsOnly" ) );
    const TestItem* merge      = Find( out, wxS( "MergeProps" ) );

    BOOST_REQUIRE( keep );
    BOOST_REQUIRE( anc_pick );
    BOOST_REQUIRE( ours_only );
    BOOST_REQUIRE( theirs_one );
    BOOST_REQUIRE( merge );
    BOOST_CHECK( keep->side       == wxS( "anc" ) );
    BOOST_CHECK( anc_pick->side   == wxS( "anc" ) );
    BOOST_CHECK( ours_only->side  == wxS( "ours" ) );
    BOOST_CHECK( theirs_one->side == wxS( "theirs" ) );
    BOOST_CHECK( merge->side      == wxS( "ours" ) );
    BOOST_CHECK( Find( out, wxS( "DelMe" ) ) == nullptr );
}


BOOST_AUTO_TEST_CASE( MergeProps_OursMissing_StillTracksIdAndErasesLive )
{
    // MERGE_PROPS where ours doesn't have the item: take(m_ours, ...) must
    // erase the live entry (because pick(m_ours) returns null), and the
    // mergePropsFallbackIds vector must still record the ID so the job
    // handler can surface the unresolved conflict to the user.
    ItemStore anc{ { wxS( "R" ), wxS( "anc" ) } };
    ItemStore ours;
    ItemStore theirs{ { wxS( "R" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::MERGE_PROPS ) );

    ItemMap ancMap = MakeMap( anc );
    ItemMap oursMap = MakeMap( ours );
    ItemMap theirsMap = MakeMap( theirs );
    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    auto out = applier.Apply();

    BOOST_CHECK( out.empty() );
    BOOST_CHECK_EQUAL( applier.GetReport().mergePropsFallback, 1 );
    BOOST_REQUIRE_EQUAL( applier.GetReport().mergePropsFallbackIds.size(), 1 );
    BOOST_CHECK( applier.GetReport().mergePropsFallbackIds[0]
                 == LibraryItemKiidPath( wxS( "R" ) ) );
}


BOOST_AUTO_TEST_CASE( NonMergeProps_DoesNotPopulateFallbackIds )
{
    // Pin that fallbackIds is only touched by the MERGE_PROPS branch. A plan
    // of pure non-MERGE_PROPS actions must leave the vector empty.
    ItemStore anc{ { wxS( "A" ), wxS( "anc" ) }, { wxS( "B" ), wxS( "anc" ) },
                   { wxS( "C" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "A" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "B" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "A" ), ITEM_RES::TAKE_OURS ) );
    plan.actions.push_back( Action( wxS( "B" ), ITEM_RES::TAKE_THEIRS ) );
    plan.actions.push_back( Action( wxS( "C" ), ITEM_RES::DELETE_ITEM ) );

    ItemMap ancMap = MakeMap( anc );
    ItemMap oursMap = MakeMap( ours );
    ItemMap theirsMap = MakeMap( theirs );
    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    applier.Apply();

    BOOST_CHECK_EQUAL( applier.GetReport().mergePropsFallback, 0 );
    BOOST_CHECK( applier.GetReport().mergePropsFallbackIds.empty() );
}


BOOST_AUTO_TEST_CASE( ReportResetOnReapply )
{
    // GetReport() must reset on every Apply() — otherwise counters would
    // double when callers re-run the applier (e.g., regression-test loops).
    // Include a MERGE_PROPS action so the vector field (mergePropsFallbackIds)
    // is also exercised: a non-trivial reset path must clear the vector too.
    ItemStore anc{ { wxS( "M" ), wxS( "anc" ) } };
    ItemStore ours{ { wxS( "R" ), wxS( "ours" ) }, { wxS( "M" ), wxS( "ours" ) } };
    ItemStore theirs{ { wxS( "M" ), wxS( "theirs" ) } };

    MERGE_PLAN plan;
    plan.actions.push_back( Action( wxS( "R" ), ITEM_RES::TAKE_OURS ) );
    plan.actions.push_back( Action( wxS( "M" ), ITEM_RES::MERGE_PROPS ) );

    ItemMap ancMap = MakeMap( anc );
    ItemMap oursMap = MakeMap( ours );
    ItemMap theirsMap = MakeMap( theirs );
    LIB_MERGE_APPLIER<TestItem> applier( ancMap, oursMap, theirsMap, std::move( plan ) );
    applier.Apply();
    applier.Apply();

    BOOST_CHECK_EQUAL( applier.GetReport().itemsTakenOurs, 1 );
    BOOST_CHECK_EQUAL( applier.GetReport().mergePropsFallback, 1 );
    BOOST_CHECK_EQUAL( applier.GetReport().mergePropsFallbackIds.size(), 1u );
}


BOOST_AUTO_TEST_SUITE_END()
