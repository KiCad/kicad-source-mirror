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
#include <mdw/mdw_claim_text.h>
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


namespace
{
using ID = WORKSPACE_ACTION::ID;


const WORKSPACE_ACTION* action( const std::vector<WORKSPACE_ACTION>& aActions, ID aId )
{
    for( const WORKSPACE_ACTION& a : aActions )
    {
        if( a.id == aId )
            return &a;
    }

    return nullptr;
}


WORKSPACE_ACTION_FACTS gitDesignFile()
{
    WORKSPACE_ACTION_FACTS facts;
    facts.locking = true;
    facts.inPlace = true;
    facts.localFile = true;
    facts.openable = true;
    facts.lock.known = true;
    return facts;
}


MDW_LOCK_INFO heldByOther()
{
    MDW_LOCK_INFO lock;
    lock.known = true;
    lock.is_locked = true;
    lock.locked_by_me = false;
    lock.advisory = true;
    lock.owner_display_name = wxS( "dave" );
    return lock;
}
} // namespace


BOOST_AUTO_TEST_SUITE( WorkspaceActions )


BOOST_AUTO_TEST_CASE( GitDesignFileOffersEditing )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );
    const WORKSPACE_ACTION*       editing = action( actions, ID::EDITING );

    BOOST_REQUIRE( editing );
    BOOST_CHECK_EQUAL( editing->label, MdwClaimActions( facts.lock ).front().label );
    BOOST_CHECK( editing->allowed );
    BOOST_CHECK( !action( actions, ID::CHECK_OUT ) );
    BOOST_CHECK( !action( actions, ID::CHECK_IN ) );
    BOOST_REQUIRE( actions.size() == 3 );
    BOOST_CHECK( actions[0].id == ID::OPEN );
    BOOST_CHECK( actions[1].id == ID::EDITING );
    BOOST_CHECK( actions[2].id == ID::SHOW_IN_FOLDER );
}


BOOST_AUTO_TEST_CASE( ClaimedByOtherFollowsClaimActions )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();
    facts.lock = heldByOther();

    const MDW_CLAIM_ACTION claim = MdwClaimActions( facts.lock ).front();
    BOOST_REQUIRE( !claim.enabled );

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );
    const WORKSPACE_ACTION*       editing = action( actions, ID::EDITING );

    BOOST_REQUIRE( editing );
    BOOST_CHECK_EQUAL( editing->label, claim.label );
    BOOST_CHECK( !editing->allowed );
    BOOST_CHECK_EQUAL( editing->reason, wxString::Format( _( "Being edited by %s" ), wxS( "dave" ) ) );

    // The holder's claim can be released from the same list
    const WORKSPACE_ACTION* release = action( actions, ID::RELEASE_CLAIM );

    BOOST_REQUIRE( release );
    BOOST_CHECK_EQUAL( release->label, MdwClaimRelease( facts.lock )->label );
    BOOST_CHECK( release->allowed );
    BOOST_CHECK( editing + 1 == release );
}


BOOST_AUTO_TEST_CASE( BlockedCheckOutAndInSayWhoHoldsTheItem )
{
    WORKSPACE_ACTION_FACTS facts;
    facts.locking = true;
    facts.localFile = true;
    facts.lock = heldByOther();
    facts.lock.advisory = false;

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );
    const wxString                heldBy = wxString::Format( _( "Being edited by %s" ), wxS( "dave" ) );

    BOOST_CHECK_EQUAL( action( actions, ID::CHECK_OUT )->reason, heldBy );
    BOOST_CHECK_EQUAL( action( actions, ID::CHECK_IN )->reason, heldBy );
    BOOST_CHECK( !action( actions, ID::RELEASE_CLAIM ) );

    facts.lock.locked_by_me = true;
    actions = WorkspaceItemActions( facts );
    BOOST_CHECK_EQUAL( action( actions, ID::CHECK_OUT )->reason,
                       wxString::Format( _( "Being edited by %s" ), _( "you" ) ) );
    BOOST_CHECK( action( actions, ID::CHECK_IN )->reason.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( UploadBackendOffersCheckOutAndIn )
{
    WORKSPACE_ACTION_FACTS facts;
    facts.locking = true;
    facts.localFile = true;
    facts.lock.known = true;

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );

    BOOST_CHECK( !action( actions, ID::EDITING ) );
    BOOST_REQUIRE( action( actions, ID::CHECK_OUT ) );
    BOOST_REQUIRE( action( actions, ID::CHECK_IN ) );
    BOOST_CHECK( action( actions, ID::CHECK_OUT )->allowed );
    BOOST_CHECK( action( actions, ID::CHECK_IN )->allowed );

    // A claim held by someone else blocks check out and check in, and ours or an advisory one does not
    facts.lock = heldByOther();
    facts.lock.advisory = false;
    actions = WorkspaceItemActions( facts );
    BOOST_CHECK( !action( actions, ID::CHECK_OUT )->allowed );
    BOOST_CHECK( !action( actions, ID::CHECK_IN )->allowed );

    facts.lock.advisory = true;
    actions = WorkspaceItemActions( facts );
    BOOST_CHECK( !action( actions, ID::CHECK_OUT )->allowed );
    BOOST_CHECK( action( actions, ID::CHECK_IN )->allowed );

    facts.lock.advisory = false;
    facts.lock.locked_by_me = true;
    actions = WorkspaceItemActions( facts );
    BOOST_CHECK( !action( actions, ID::CHECK_OUT )->allowed );
    BOOST_CHECK( action( actions, ID::CHECK_IN )->allowed );
}


BOOST_AUTO_TEST_CASE( NoLockingOffersNoEditingAction )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();
    facts.locking = false;

    BOOST_CHECK( !action( WorkspaceItemActions( facts ), ID::EDITING ) );
}


BOOST_AUTO_TEST_CASE( NoTransitionsNoChangeState )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();

    BOOST_CHECK( !action( WorkspaceItemActions( facts ), ID::CHANGE_STATE ) );

    facts.transitions.emplace_back();
    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );
    const WORKSPACE_ACTION*       change = action( actions, ID::CHANGE_STATE );

    BOOST_REQUIRE( change );
    BOOST_CHECK( change->allowed );
    BOOST_CHECK_EQUAL( change->label, _( "Change State..." ) );
}


BOOST_AUTO_TEST_CASE( DeniedTransitionCarriesTheReason )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();
    facts.transitions.emplace_back();
    facts.transition.set_state( kiapi::workspace::PS_DENIED );
    facts.transition.set_reason( "Only approvers may change state" );

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );
    const WORKSPACE_ACTION*       change = action( actions, ID::CHANGE_STATE );

    BOOST_REQUIRE( change );
    BOOST_CHECK( !change->allowed );
    BOOST_CHECK_EQUAL( change->reason, wxString( "Only approvers may change state" ) );
}


BOOST_AUTO_TEST_CASE( EveryTransitionDeniedLeavesNothingToChoose )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();

    kiapi::workspace::LifecycleTransition release;
    release.mutable_permission()->set_state( kiapi::workspace::PS_DENIED );
    release.mutable_permission()->set_reason( "Needs a review" );
    facts.transitions.push_back( release );

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );
    const WORKSPACE_ACTION*       change = action( actions, ID::CHANGE_STATE );

    BOOST_REQUIRE( change );
    BOOST_CHECK( !change->allowed );
    BOOST_CHECK_EQUAL( change->reason, wxString( "Needs a review" ) );

    // One state still open is enough to offer the choice
    facts.transitions.emplace_back();
    actions = WorkspaceItemActions( facts );
    change = action( actions, ID::CHANGE_STATE );

    BOOST_REQUIRE( change );
    BOOST_CHECK( change->allowed );
    BOOST_CHECK( change->reason.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( MemberOffersOpenAndChangeStateOnly )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();
    facts.member = true;
    facts.transitions.emplace_back();

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );

    BOOST_CHECK( !action( actions, ID::EDITING ) );
    BOOST_CHECK( !action( actions, ID::CHECK_OUT ) );
    BOOST_CHECK( !action( actions, ID::CHECK_IN ) );
    BOOST_CHECK( action( actions, ID::OPEN ) );
    BOOST_CHECK( action( actions, ID::CHANGE_STATE ) );

    facts.inPlace = false;
    actions = WorkspaceItemActions( facts );
    BOOST_CHECK( !action( actions, ID::CHECK_OUT ) );
    BOOST_CHECK( !action( actions, ID::CHECK_IN ) );
}


BOOST_AUTO_TEST_CASE( HostThatCannotOpenOffersNoOpen )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();
    facts.openable = false;

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );

    BOOST_CHECK( !action( actions, ID::OPEN ) );
    BOOST_CHECK( action( actions, ID::SHOW_IN_FOLDER ) );
}


BOOST_AUTO_TEST_CASE( ChangeStateWaitsForTheTransitionsRead )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();
    facts.transitionsReading = true;

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );
    const WORKSPACE_ACTION*       change = action( actions, ID::CHANGE_STATE );

    BOOST_REQUIRE( change );
    BOOST_CHECK( !change->allowed );
    BOOST_CHECK_EQUAL( change->reason, _( "Reading states..." ) );
}


BOOST_AUTO_TEST_CASE( MissingFileOffersNoOpen )
{
    WORKSPACE_ACTION_FACTS facts = gitDesignFile();
    facts.localFile = false;

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );

    BOOST_CHECK( !action( actions, ID::OPEN ) );
    BOOST_CHECK( !action( actions, ID::SHOW_IN_FOLDER ) );
    BOOST_CHECK( action( actions, ID::EDITING ) );
}


BOOST_AUTO_TEST_SUITE_END()
