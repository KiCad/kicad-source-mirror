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


#include <dialogs/git/workspace_detail.h>

#include <mdw/client/mdw_baseline_design.h>
#include <mdw/client/mdw_lifecycle.h>
#include <mdw/mdw_claim_text.h>
#include <widgets/mdw_member_index.h>
#include <widgets/mdw_part_state.h>

#include <wx/intl.h>

#include <algorithm>


namespace
{
/// The Lifecycle, Editing and Content rows an item and a member share
std::vector<WORKSPACE_DETAIL_ROW> stateRows( const MDW_CAPABILITIES& aCaps, const MDW_LIFECYCLE_STATE& aState,
                                             const MDW_LOCK_INFO& aLock, std::optional<bool> aModified,
                                             const wxDateTime& aNow )
{
    std::vector<WORKSPACE_DETAIL_ROW> rows;

    if( aCaps.Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) )
        rows.push_back( { _( "Lifecycle" ), MDW_MEMBER_INDEX::StateName( aCaps, aState ) } );

    if( aCaps.Has( MDW_CAPABILITY_GROUP::LOCKING ) )
    {
        wxString claim = MdwClaimLabel( aLock, aNow );
        rows.push_back( { _( "Editing" ), claim.IsEmpty() ? _( "nobody" ) : claim } );
    }

    if( aModified.has_value() )
        rows.push_back( { _( "Content" ), *aModified ? _( "Modified" ) : _( "Current" ) } );

    return rows;
}
} // namespace


wxString WorkspaceStateCell( const MDW_CAPABILITIES& aCaps, const MDW_LIFECYCLE_STATE& aState,
                             const wxString& aRevisionLabel )
{
    if( !aCaps.Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) )
        return wxString();

    wxString name = MDW_MEMBER_INDEX::StateName( aCaps, aState );

    // Editable is the unremarkable default, so only a state the backend names is worth a cell
    if( aState.category == MDW_LIFECYCLE_CATEGORY::EDITABLE && name == MdwCategoryName( aState.category ) )
        return wxString();

    if( aState.category == MDW_LIFECYCLE_CATEGORY::RELEASED && !aRevisionLabel.IsEmpty() )
        return name + wxS( " \u00B7 " ) + aRevisionLabel;

    return name;
}


wxString WorkspaceMemberStateCell( const MDW_CAPABILITIES& aCaps, const MDW_MEMBER_ENTRY& aEntry )
{
    if( !aCaps.Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) )
        return wxString();

    // The claim has the Editing column, and the library tree's edit note is about an editor's buffer
    return MDW_PART_STATE::TreeCell( MDW_MEMBER_INDEX::StateName( aCaps, aEntry.member.state ), MDW_LOCK_INFO(),
                                     false );
}


std::vector<WORKSPACE_DETAIL_ROW> WorkspaceDetailRows( const MDW_CAPABILITIES& aCaps, const MDW_ITEM_INFO& aItem,
                                                       const MDW_LOCK_INFO& aLock, std::optional<bool> aModified,
                                                       const wxString& aFolderPath, const wxDateTime& aNow )
{
    std::vector<WORKSPACE_DETAIL_ROW> rows = stateRows( aCaps, aItem.state, aLock, aModified, aNow );

    // The backend id is a hash, so a short prefix identifies a revision that has no label
    wxString version = aItem.revision_label.IsEmpty() ? aItem.revision_id.Left( 7 ) : aItem.revision_label;
    rows.push_back( { _( "Version" ), version } );

    rows.push_back( { _( "Folder" ), aFolderPath.IsEmpty() ? _( "(top)" ) : aFolderPath } );

    if( aItem.modified.IsValid() )
        rows.push_back( { _( "Last change" ), aItem.modified.Format( wxS( "%x %H:%M" ) ) } );

    return rows;
}


std::vector<WORKSPACE_DETAIL_ROW> WorkspaceMemberDetailRows( const MDW_CAPABILITIES&              aCaps,
                                                             const MDW_MEMBER_ENTRY&              aEntry,
                                                             const std::optional<MDW_PART_FACTS>& aFacts,
                                                             std::optional<bool> aModified, const wxDateTime& aNow )
{
    std::vector<WORKSPACE_DETAIL_ROW> rows = stateRows( aCaps, aEntry.member.state, aEntry.lock, aModified, aNow );

    if( aFacts )
        rows.push_back( { _( "Release" ), MdwPlacedStatusText( *aFacts ) } );

    return rows;
}


wxString WorkspaceWhereUsedText( const std::vector<wxString>& aReferences )
{
    if( aReferences.empty() )
        return _( "Not placed in this design" );

    wxString text = aReferences.front();

    for( size_t ii = 1; ii < aReferences.size(); ++ii )
        text << wxS( ", " ) << aReferences[ii];

    return text;
}


std::vector<WORKSPACE_ACTION> WorkspaceItemActions( const WORKSPACE_ACTION_FACTS& aFacts )
{
    using ID = WORKSPACE_ACTION::ID;

    const MDW_LOCK_INFO&          lock = aFacts.lock;
    std::vector<WORKSPACE_ACTION> actions;

    // The same words as MdwClaimLabel without the time, so a greyed command says who holds the item
    const wxString heldBy = lock.is_locked ? wxString::Format( _( "Being edited by %s" ),
                                                               lock.locked_by_me ? _( "you" ) : MdwClaimHolder( lock ) )
                                           : wxString();

    if( aFacts.localFile && aFacts.openable )
        actions.push_back( { ID::OPEN, _( "Open" ), true, wxString() } );

    // A part is claimed through Revise in its editor, so only design files offer these
    if( !aFacts.member )
    {
        if( aFacts.inPlace )
        {
            if( aFacts.locking )
            {
                for( const MDW_CLAIM_ACTION& claim : MdwClaimActions( lock ) )
                {
                    ID id = claim.kind == MDW_CLAIM_ACTION::KIND::RELEASE ? ID::RELEASE_CLAIM : ID::EDITING;

                    actions.push_back(
                            { id, claim.label, claim.enabled, claim.enabled ? wxString() : heldBy, claim.kind } );
                }
            }
        }
        else
        {
            bool checkOut = !lock.is_locked;
            bool checkIn = !lock.is_locked || lock.locked_by_me || lock.advisory;

            actions.push_back( { ID::CHECK_OUT, _( "Check Out" ), checkOut, checkOut ? wxString() : heldBy } );
            actions.push_back( { ID::CHECK_IN, _( "Check In..." ), checkIn, checkIn ? wxString() : heldBy } );
        }
    }

    if( aFacts.transitionsReading )
    {
        actions.push_back( { ID::CHANGE_STATE, _( "Change State..." ), false, _( "Reading states..." ) } );
    }
    else if( !aFacts.transitions.empty() )
    {
        using kiapi::workspace::PS_DENIED;

        auto open = std::ranges::find_if( aFacts.transitions,
                                          []( const kiapi::workspace::LifecycleTransition& aTransition )
                                          {
                                              return aTransition.permission().state() != PS_DENIED;
                                          } );

        // Every state refusing the move leaves nothing to choose, so the first refusal says why
        std::string reason;

        if( aFacts.transition.state() == PS_DENIED )
            reason = aFacts.transition.reason();
        else if( open == aFacts.transitions.end() )
            reason = aFacts.transitions.front().permission().reason();

        bool allowed = aFacts.transition.state() != PS_DENIED && open != aFacts.transitions.end();

        actions.push_back( { ID::CHANGE_STATE, _( "Change State..." ), allowed, wxString::FromUTF8( reason ) } );
    }

    if( aFacts.localFile )
        actions.push_back( { ID::SHOW_IN_FOLDER, _( "Show in Folder" ), true, wxString() } );

    return actions;
}


wxString WorkspaceActionRefusal( const WORKSPACE_ACTION& aAction )
{
    if( aAction.allowed || aAction.reason.IsEmpty() || aAction.id == WORKSPACE_ACTION::ID::EDITING )
        return wxString();

    wxString label = aAction.label;

    // The ellipsis promises a dialog, which the line under the buttons does not open
    if( !label.EndsWith( wxS( "..." ), &label ) )
        label.EndsWith( wxS( "\u2026" ), &label );

    return wxString::Format( _( "%s (%s)" ), label, aAction.reason );
}
