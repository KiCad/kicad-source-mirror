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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <qa_utils/wx_utils/unit_test_utils.h>

#include <dialogs/git/workspace_detail.h>

#include <mdw/client/mdw_lifecycle.h>
#include <widgets/mdw_member_index.h>

#include <wx/intl.h>


namespace
{
MDW_CAPABILITIES caps( std::initializer_list<MDW_CAPABILITY_GROUP> aGroups )
{
    MDW_CAPABILITIES c;
    c.groups.insert( aGroups.begin(), aGroups.end() );
    return c;
}


MDW_LIFECYCLE_STATE state( MDW_LIFECYCLE_CATEGORY aCategory, const wxString& aId = wxEmptyString )
{
    MDW_LIFECYCLE_STATE s;
    s.category = aCategory;
    s.state_id = aId;
    return s;
}


const WORKSPACE_DETAIL_ROW* find( const std::vector<WORKSPACE_DETAIL_ROW>& aRows, const wxString& aLabel )
{
    for( const WORKSPACE_DETAIL_ROW& row : aRows )
    {
        if( row.label == aLabel )
            return &row;
    }

    return nullptr;
}
} // namespace


BOOST_AUTO_TEST_SUITE( WorkspaceDetail )


BOOST_AUTO_TEST_CASE( NoLifecycleLeavesTheCellEmpty )
{
    BOOST_CHECK( WorkspaceStateCell( caps( {} ), state( MDW_LIFECYCLE_CATEGORY::RELEASED ), wxS( "3" ) ).IsEmpty() );
}


BOOST_AUTO_TEST_CASE( PlainEditableStateLeavesTheCellEmpty )
{
    MDW_CAPABILITIES c = caps( { MDW_CAPABILITY_GROUP::LIFECYCLE } );

    BOOST_CHECK( WorkspaceStateCell( c, state( MDW_LIFECYCLE_CATEGORY::EDITABLE ), wxS( "3" ) ).IsEmpty() );
}


BOOST_AUTO_TEST_CASE( ReleasedCellNamesTheRevision )
{
    MDW_CAPABILITIES    c = caps( { MDW_CAPABILITY_GROUP::LIFECYCLE } );
    MDW_LIFECYCLE_STATE s = state( MDW_LIFECYCLE_CATEGORY::RELEASED );

    BOOST_CHECK_EQUAL( WorkspaceStateCell( c, s, wxS( "Baseline 3" ) ),
                       MDW_MEMBER_INDEX::StateName( c, s ) + wxS( " \u00B7 Baseline 3" ) );
}


BOOST_AUTO_TEST_CASE( CustomStateUsesTheBackendName )
{
    MDW_CAPABILITIES c = caps( { MDW_CAPABILITY_GROUP::LIFECYCLE } );
    kiapi::workspace::CustomState& custom = c.custom_states.emplace_back();
    custom.set_state_id( "in_review" );
    custom.set_display_name( "In Review" );

    MDW_LIFECYCLE_STATE s = state( MDW_LIFECYCLE_CATEGORY::EDITABLE, wxS( "in_review" ) );

    BOOST_CHECK_EQUAL( WorkspaceStateCell( c, s, wxS( "" ) ), wxS( "In Review" ) );
}


BOOST_AUTO_TEST_CASE( RowsFollowCapabilities )
{
    MDW_ITEM_INFO item;
    MDW_LOCK_INFO lock;
    wxDateTime    now = wxDateTime::Now();

    auto none = WorkspaceDetailRows( caps( {} ), item, lock, std::nullopt, wxS( "" ), now );

    BOOST_CHECK( !find( none, _( "Lifecycle" ) ) );
    BOOST_CHECK( !find( none, _( "Editing" ) ) );

    auto both = WorkspaceDetailRows( caps( { MDW_CAPABILITY_GROUP::LIFECYCLE, MDW_CAPABILITY_GROUP::LOCKING } ), item,
                                     lock, std::nullopt, wxS( "" ), now );

    BOOST_CHECK( find( both, _( "Lifecycle" ) ) );
    BOOST_REQUIRE( find( both, _( "Editing" ) ) );
    BOOST_CHECK_EQUAL( find( both, _( "Editing" ) )->value, _( "nobody" ) );
}


BOOST_AUTO_TEST_CASE( ContentRowOnlyWhenKnown )
{
    MDW_ITEM_INFO item;
    MDW_LOCK_INFO lock;
    wxDateTime    now = wxDateTime::Now();

    auto unknown = WorkspaceDetailRows( caps( {} ), item, lock, std::nullopt, wxS( "" ), now );
    auto changed = WorkspaceDetailRows( caps( {} ), item, lock, true, wxS( "" ), now );
    auto clean = WorkspaceDetailRows( caps( {} ), item, lock, false, wxS( "" ), now );

    BOOST_CHECK( !find( unknown, _( "Content" ) ) );
    BOOST_REQUIRE( find( changed, _( "Content" ) ) );
    BOOST_CHECK_EQUAL( find( changed, _( "Content" ) )->value, _( "Modified" ) );
    BOOST_REQUIRE( find( clean, _( "Content" ) ) );
    BOOST_CHECK_EQUAL( find( clean, _( "Content" ) )->value, _( "Current" ) );
}


BOOST_AUTO_TEST_CASE( VersionRowFallsBackToTheShortId )
{
    MDW_ITEM_INFO item;
    MDW_LOCK_INFO lock;
    item.revision_id = wxS( "a1b2c3d4e5" );

    auto rows = WorkspaceDetailRows( caps( {} ), item, lock, std::nullopt, wxS( "" ), wxDateTime::Now() );

    BOOST_REQUIRE( find( rows, _( "Version" ) ) );
    BOOST_CHECK_EQUAL( find( rows, _( "Version" ) )->value, wxS( "a1b2c3d" ) );

    item.revision_label = wxS( "Rev B" );
    rows = WorkspaceDetailRows( caps( {} ), item, lock, std::nullopt, wxS( "" ), wxDateTime::Now() );
    BOOST_CHECK_EQUAL( find( rows, _( "Version" ) )->value, wxS( "Rev B" ) );
}


BOOST_AUTO_TEST_CASE( FolderAndLastChangeRows )
{
    MDW_ITEM_INFO item;
    MDW_LOCK_INFO lock;
    wxDateTime    now = wxDateTime::Now();

    auto rows = WorkspaceDetailRows( caps( {} ), item, lock, std::nullopt, wxS( "" ), now );

    BOOST_REQUIRE( find( rows, _( "Folder" ) ) );
    BOOST_CHECK_EQUAL( find( rows, _( "Folder" ) )->value, _( "(top)" ) );
    BOOST_CHECK( !find( rows, _( "Last change" ) ) );

    item.modified = wxDateTime( 14, wxDateTime::Nov, 2026, 12, 0 );
    rows = WorkspaceDetailRows( caps( {} ), item, lock, std::nullopt, wxS( "Parts/Power" ), now );

    BOOST_CHECK_EQUAL( find( rows, _( "Folder" ) )->value, wxS( "Parts/Power" ) );
    BOOST_CHECK( find( rows, _( "Last change" ) ) );
    BOOST_CHECK_EQUAL( rows.back().label, _( "Last change" ) );
}


BOOST_AUTO_TEST_SUITE_END()
