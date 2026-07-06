/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <base_units.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <page_info.h>
#include <text_var_dependency.h>
#include <title_block.h>


namespace
{
struct DefaultLayoutFixture
{
    DefaultLayoutFixture()
    {
        // The drawing-sheet system is singleton-based; tests depend on a
        // populated default layout to find text items to scan.
        DS_DATA_MODEL::GetTheInstance().SetDefaultLayout();
    }
};
}


BOOST_FIXTURE_TEST_SUITE( DrawingSheetReactive, DefaultLayoutFixture )


BOOST_AUTO_TEST_CASE( CollectKeysFindsTitleBlockSources )
{
    // Default drawing sheet template references PROJECTNAME, REVISION, TITLE,
    // COMMENT[1..9], ISSUE_DATE, COMPANY, etc. — the set is defined by the
    // drawing sheet parser. Collect them and confirm several well-known ones
    // are present.
    PAGE_INFO    page;
    TITLE_BLOCK  tb;

    DS_PROXY_VIEW_ITEM proxy( unityScale, &page, nullptr, &tb, nullptr );

    const auto keys = proxy.CollectTextVarKeys();

    auto hasKey = [&]( TEXT_VAR_REF_KEY::KIND k, const wxString& primary )
    {
        return std::any_of( keys.begin(), keys.end(),
                            [&]( const TEXT_VAR_REF_KEY& key )
                            { return key.kind == k && key.primary == primary; } );
    };

    // The default drawing sheet always surfaces at least these tokens.
    BOOST_CHECK( hasKey( TEXT_VAR_REF_KEY::KIND::TITLE_BLOCK, wxT( "REVISION" ) ) );
    BOOST_CHECK( hasKey( TEXT_VAR_REF_KEY::KIND::TITLE_BLOCK, wxT( "TITLE" ) ) );
    BOOST_CHECK( hasKey( TEXT_VAR_REF_KEY::KIND::TITLE_BLOCK, wxT( "COMPANY" ) ) );
}


BOOST_AUTO_TEST_CASE( AttachRegistersProxyAsDependent )
{
    PAGE_INFO    page;
    TITLE_BLOCK  tb;

    DS_PROXY_VIEW_ITEM proxy( unityScale, &page, nullptr, &tb, nullptr );

    TEXT_VAR_TRACKER tracker;
    proxy.AttachToTracker( &tracker );

    // The proxy should be registered as a dependent on every title-block key
    // its template references.
    BOOST_CHECK(
            tracker.Index().DependentCount(
                    TEXT_VAR_REF_KEY::FromToken( wxT( "REVISION" ) ) ) > 0u );
    BOOST_CHECK(
            tracker.Index().DependentCount(
                    TEXT_VAR_REF_KEY::FromToken( wxT( "TITLE" ) ) ) > 0u );
}


BOOST_AUTO_TEST_CASE( InvalidationReachesProxyViaListener )
{
    PAGE_INFO    page;
    TITLE_BLOCK  tb;

    DS_PROXY_VIEW_ITEM proxy( unityScale, &page, nullptr, &tb, nullptr );
    TEXT_VAR_TRACKER   tracker;
    proxy.AttachToTracker( &tracker );

    // Frames install a long-lived listener that routes invalidations to the
    // current drawing sheet proxy. Verify the dispatch semantics with a
    // minimal listener.
    EDA_ITEM* seenDep = nullptr;
    (void) tracker.AddInvalidateListener(
            [&]( EDA_ITEM* dep, const TEXT_VAR_REF_KEY& )
            {
                if( dep == &proxy )
                    seenDep = dep;
            } );

    tracker.InvalidateKey( TEXT_VAR_REF_KEY::FromToken( wxT( "REVISION" ) ) );

    BOOST_CHECK_EQUAL( seenDep, &proxy );
}


BOOST_AUTO_TEST_CASE( DetachUnregistersProxy )
{
    PAGE_INFO    page;
    TITLE_BLOCK  tb;

    DS_PROXY_VIEW_ITEM proxy( unityScale, &page, nullptr, &tb, nullptr );
    TEXT_VAR_TRACKER   tracker;

    proxy.AttachToTracker( &tracker );
    BOOST_CHECK(
            tracker.Index().DependentCount(
                    TEXT_VAR_REF_KEY::FromToken( wxT( "REVISION" ) ) ) > 0u );

    proxy.AttachToTracker( nullptr );
    BOOST_CHECK_EQUAL(
            tracker.Index().DependentCount(
                    TEXT_VAR_REF_KEY::FromToken( wxT( "REVISION" ) ) ),
            0u );
}


BOOST_AUTO_TEST_CASE( DestructorAutoUnregisters )
{
    PAGE_INFO    page;
    TITLE_BLOCK  tb;

    TEXT_VAR_TRACKER tracker;

    {
        DS_PROXY_VIEW_ITEM proxy( unityScale, &page, nullptr, &tb, nullptr );
        proxy.AttachToTracker( &tracker );
        BOOST_REQUIRE( tracker.Index().ItemCount() > 0u );
    }

    // Proxy destructor must clean up the index so no dangling pointer remains
    // for a future invalidation to match against.
    BOOST_CHECK_EQUAL( tracker.Index().ItemCount(), 0u );
}


BOOST_AUTO_TEST_CASE( RepeatedTextKeepsAllInstancesWithNegativeStep )
{
    // Regression: a repeated text used to lose every copy when the step was
    // negative, because IsInsidePage() tested a phantom end point anchored to
    // the bottom-right corner (text has no real end point). See #24309.
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();
    model.ClearList();

    DS_DATA_ITEM_TEXT* text = new DS_DATA_ITEM_TEXT( wxT( "1" ) );
    text->SetStart( 60.0, 60.0, LT_CORNER );
    text->m_RepeatCount = 8;
    text->m_IncrementLabel = 1;
    text->m_IncrementVector = VECTOR2D( -5.0, -5.0 );
    model.Append( text );

    PAGE_INFO   page;
    TITLE_BLOCK tb;

    DS_DRAW_ITEM_LIST drawList( unityScale );
    drawList.BuildDrawItemsList( page, tb );

    int textCount = 0;

    for( DS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item; item = drawList.GetNext() )
    {
        if( item->Type() == WSG_TEXT_T )
            textCount++;
    }

    BOOST_CHECK_EQUAL( textCount, 8 );
}


BOOST_AUTO_TEST_SUITE_END()
