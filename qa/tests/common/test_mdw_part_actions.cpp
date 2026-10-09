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


#include <qa_utils/wx_utils/unit_test_utils.h>

#include <gestfich.h>
#include <tool/actions.h>
#include <widgets/mdw_part_actions.h>
#include <widgets/mdw_part_controller.h>

#include <wx/file.h>
#include <wx/filename.h>

#include <algorithm>
#include <stdexcept>


namespace
{

bool offers( const MDW_PART_STATE& aState, MDW_PART_ACTION::ID aId )
{
    return std::ranges::any_of( aState.Actions(),
                                [aId]( const MDW_PART_ACTION& aAction )
                                {
                                    return aAction.id == aId && aAction.Allowed();
                                } );
}


/// An editor no dispatch may reach, for a run that must stop before asking it anything
class UNREACHED_EDITOR : public MDW_PART_EDITOR
{
public:
    bool SaveForRelease( std::vector<MDW_RELEASE_ITEM>& ) override { throw std::logic_error( "reached" ); }
    bool CanReloadPart( bool ) override { throw std::logic_error( "reached" ); }
    bool ReloadPart( bool ) override { throw std::logic_error( "reached" ); }

    std::optional<std::string> CurrentFormat( const std::string&, const std::vector<std::string>&, const std::string&,
                                              const wxString& ) override
    {
        throw std::logic_error( "reached" );
    }
    void GetLatest() override { throw std::logic_error( "reached" ); }
    MDW_PART_CONTROLLER& TeamPart() override { throw std::logic_error( "reached" ); }
    EDA_BASE_FRAME*      PartFrame() override { throw std::logic_error( "reached" ); }

    std::optional<MDW_PART_TARGET> ResolvePart( const LIB_ID& ) override { throw std::logic_error( "reached" ); }

    LIBRARY_TABLE_TYPE PartTableType() const override { throw std::logic_error( "reached" ); }

    bool OpenPart( const LIB_ID& ) override { throw std::logic_error( "reached" ); }
    void RefreshPart() override { throw std::logic_error( "reached" ); }
    void CheckInLibrary( const wxString& ) override { throw std::logic_error( "reached" ); }

    bool DescribeForRelease( MDW_RELEASE_ITEM&, const std::vector<LIB_ID>& ) override
    {
        throw std::logic_error( "reached" );
    }
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwPartActions )


// Every Lifecycle action but Change State comes without a state parameter
BOOST_AUTO_TEST_CASE( LifecycleActionWithoutStateRuns )
{
    UNREACHED_EDITOR editor;

    BOOST_CHECK_NO_THROW( MdwRunPartToolAction( editor, ACTIONS::teamPartHistory.MakeEvent(), {} ) );
}


// A revise saved back unchanged has nothing to release, so only cancelling it can end the revision. An edited one
// can still be abandoned, by restoring the release
BOOST_AUTO_TEST_CASE( RevisionOffersCancelRevision )
{
    MDW_REVISION release;
    release.revision_id = wxS( "f650295" );
    release.label = wxS( "1" );
    release.kind = MDW_REVISION_KIND::RELEASE;
    release.content_digest = wxS( "v1:capacitor" );

    MDW_PART_STATE state;
    state.lifecycle = true;
    state.known = true;
    state.release = release;
    state.digest = release.content_digest;
    state.lock.is_locked = true;
    state.lock.locked_by_me = true;

    BOOST_CHECK( offers( state, MDW_PART_ACTION::ID::CANCEL_REVISION ) );
    BOOST_CHECK( !offers( state, MDW_PART_ACTION::ID::RELEASE ) );

    state.digest = wxS( "v1:capacitor-filters" );

    BOOST_CHECK( offers( state, MDW_PART_ACTION::ID::CANCEL_REVISION ) );
    BOOST_CHECK( offers( state, MDW_PART_ACTION::ID::RELEASE ) );
}


// A teammate's git claim does not block writing, but revising would claim the part over it and fail
BOOST_AUTO_TEST_CASE( AdvisoryClaimOfOtherBlocksRevise )
{
    MDW_PART_STATE state;
    state.lifecycle = true;
    state.known = true;
    state.state.category = MDW_LIFECYCLE_CATEGORY::RELEASED;
    state.lock.is_locked = true;
    state.lock.advisory = true;

    BOOST_CHECK( !offers( state, MDW_PART_ACTION::ID::REVISE ) );
}


// Another user's claim is released from the part itself, and a refused edit points there
BOOST_AUTO_TEST_CASE( OtherClaimOffersItsRelease )
{
    using ID = MDW_PART_ACTION::ID;

    MDW_PART_STATE state;
    state.lifecycle = true;
    state.known = true;
    state.state.category = MDW_LIFECYCLE_CATEGORY::RELEASED;
    state.lock.is_locked = true;
    state.lock.advisory = true;
    state.lock.owner_display_name = wxS( "Alice" );
    state.locking = true;

    BOOST_REQUIRE( offers( state, ID::RELEASE_CLAIM ) );

    const std::vector<MDW_PART_ACTION> actions = state.Actions();

    auto release = std::ranges::find( actions, ID::RELEASE_CLAIM, &MDW_PART_ACTION::id );

    BOOST_CHECK_EQUAL( MDW_PART_CONTROLLER::Label( *release ), wxS( "Release Alice's Claim..." ) );
    BOOST_CHECK( state.EditRefusal( wxS( "R_TEST" ) ).Contains( wxS( "choose Release Alice's Claim, then Revise" ) ) );
}


// A part saved into a team library but never checked in still offers Check In
BOOST_AUTO_TEST_CASE( UnlistedPartOffersCheckIn )
{
    MDW_PART_TARGET target;
    MDW_PART_STATE  state = target.ListedState();

    state.lifecycle = true;

    BOOST_CHECK( offers( state, MDW_PART_ACTION::ID::CHECK_IN ) );
    BOOST_CHECK( !offers( state, MDW_PART_ACTION::ID::RELEASE ) );
    BOOST_CHECK_EQUAL( state.Describe( wxS( "C_Small" ) ), wxS( "C_Small · Not checked in" ) );
}


// A claimed or edited part must not read as plain Released
BOOST_AUTO_TEST_CASE( TreeCellShowsClaimAndEdits )
{
    MDW_LOCK_INFO bob;
    bob.is_locked = true;
    bob.owner_id = wxS( "bob" );

    MDW_LOCK_INFO mine;
    mine.is_locked = true;
    mine.locked_by_me = true;

    BOOST_CHECK_EQUAL( MDW_PART_STATE::TreeCell( wxS( "Released" ), bob, false ),
                       wxS( "Released · bob editing" ) );
    BOOST_CHECK_EQUAL( MDW_PART_STATE::TreeCell( wxS( "Released" ), mine, true ),
                       wxS( "Released · revising (you) · modified" ) );
}


BOOST_AUTO_TEST_CASE( TeamPartFileFollowsTheEditors )
{
    SCOPED_TEMP_PATH folder;
    BOOST_REQUIRE( folder.MakeDirectory( wxS( "mdw-part-file" ) ) );

    const wxString dir = folder.Path();
    const wxString file = wxFileName( dir, wxS( "passives" ), wxS( "kicad_sym" ) ).GetFullPath();

    BOOST_CHECK_EQUAL( MdwTeamPartFile( MDW_LIBRARY_TYPE::FOOTPRINT, dir, wxS( "R_0603" ), nullptr ),
                       wxFileName( dir, wxS( "R_0603" ), wxS( "kicad_mod" ) ).GetFullPath() );
    BOOST_CHECK_EQUAL( MdwTeamPartFile( MDW_LIBRARY_TYPE::SYMBOL, file, wxS( "R" ), nullptr ), file );

    const wxString own = wxFileName( dir, wxS( "R" ), wxS( "kicad_sym" ) ).GetFullPath();

    BOOST_CHECK_EQUAL( MdwTeamPartFile( MDW_LIBRARY_TYPE::SYMBOL, dir, wxS( "R" ), nullptr ), own );

    MDW_LIBRARY_MEMBER member;
    member.library_id = wxS( "passives" );
    member.path_hint = wxS( "passives/shared.kicad_sym" );

    // The named file wins only once it exists in the folder
    BOOST_CHECK_EQUAL( MdwTeamPartFile( MDW_LIBRARY_TYPE::SYMBOL, dir, wxS( "R" ), &member ), own );

    const wxString shared = wxFileName( dir, wxS( "shared.kicad_sym" ) ).GetFullPath();
    wxFile         create;
    BOOST_REQUIRE( create.Create( shared ) );
    create.Close();

    BOOST_CHECK_EQUAL( MdwTeamPartFile( MDW_LIBRARY_TYPE::SYMBOL, dir, wxS( "R" ), &member ), shared );
}


BOOST_AUTO_TEST_SUITE_END()
