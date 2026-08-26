/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/mdw_part_state.h>

#include <mdw/mdw_claim_text.h>

#include <wx/intl.h>

#include <algorithm>


namespace
{

const wxString SEPARATOR = wxS( " · " );


MDW_PART_ACTION permitted( MDW_PART_ACTION::ID aId, const kiapi::workspace::Permission& aPermission )
{
    return MDW_PART_ACTION( aId,
                            aPermission.state() == kiapi::workspace::PS_DENIED ? MDW_PART_ACTION::STATE::DENIED
                                                                               : MDW_PART_ACTION::STATE::ALLOWED,
                            wxString::FromUTF8( aPermission.reason() ) );
}

} // namespace


bool MDW_PART_STATE::MatchesRelease() const
{
    if( !release )
        return false;

    // A release that recorded its whole member is matched on it, so a metadata edit is something new
    if( !release->member_digest.IsEmpty() && !memberDigest.IsEmpty() )
        return release->member_digest == memberDigest;

    return !release->content_digest.IsEmpty() && !digest.IsEmpty() && release->content_digest == digest;
}


bool MDW_PART_STATE::IsReadOnly() const
{
    if( !lifecycle )
        return false;

    // Content under review stays as submitted, even for the one who holds the claim
    if( pending || ( known && state.category == MDW_LIFECYCLE_CATEGORY::PENDING_REVIEW ) )
        return true;

    return !IsRevising() && known && state.category != MDW_LIFECYCLE_CATEGORY::EDITABLE;
}


kiapi::workspace::Permission MDW_PART_STATE::Permission( const std::string& aName ) const
{
    for( const std::vector<kiapi::workspace::Permission>* list : { &permissions, &sourcePermissions } )
    {
        auto it = std::find_if( list->begin(), list->end(),
                                [&]( const kiapi::workspace::Permission& aPermission )
                                {
                                    return aPermission.name() == aName;
                                } );

        if( it != list->end() )
            return *it;
    }

    kiapi::workspace::Permission unknown;
    unknown.set_name( aName );
    return unknown;
}


std::vector<MDW_PART_ACTION> MDW_PART_STATE::Actions() const
{
    using ID = MDW_PART_ACTION::ID;

    std::vector<MDW_PART_ACTION> actions;

    if( !listed || !IsReadOnly() )
        actions.push_back( permitted( ID::CHECK_IN, Permission( "checkin" ) ) );

    if( !listed || !lifecycle )
        return actions;

    // A part awaiting review is neither released nor editable; its approver decides
    const bool decided = !pending && state.category != MDW_LIFECYCLE_CATEGORY::PENDING_REVIEW;

    if( IsReadOnly() && decided && !IsClaimedByOther() )
        actions.push_back( permitted( ID::REVISE, Permission( "revise" ) ) );

    // Edits since the release are given up with the claim, by restoring the revision that release names
    if( IsRevising() && !pending && ( MatchesRelease() || ( release && !release->revision_id.IsEmpty() ) ) )
        actions.emplace_back( ID::CANCEL_REVISION );

    if( std::optional<MDW_CLAIM_ACTION> claim = MdwClaimRelease( lock ); claim && locking )
    {
        MDW_PART_ACTION releaseClaim = permitted( ID::RELEASE_CLAIM, Permission( "force_unlock" ) );
        releaseClaim.label = claim->label;
        actions.push_back( releaseClaim );
    }

    const kiapi::workspace::Permission mayRelease = Permission( "release" );

    // The source's refusal holds whatever the state, so a released part still says why it cannot be released
    if( mayRelease.state() == kiapi::workspace::PS_DENIED
        || ( !IsReadOnly() && !pending && !IsHeldByOther() && !MatchesRelease() ) )
    {
        actions.push_back( permitted( ID::RELEASE, mayRelease ) );
    }

    for( const kiapi::workspace::LifecycleTransition& transition : pending ? decltype( transitions )() : transitions )
    {
        MDW_PART_ACTION action = permitted( ID::TRANSITION, transition.permission() );
        action.target = wxString::FromUTF8( transition.target_state_id() );
        action.label = wxString::FromUTF8( transition.display_name() );
        actions.push_back( action );
    }

    return actions;
}


wxString MDW_PART_STATE::StateName() const
{
    return stateName.IsEmpty() ? MdwCategoryName( state.category ) : stateName;
}


wxString MDW_PART_STATE::ElideName( const wxString& aName )
{
    // Counted in code points, since a wxString on Windows holds UTF-16 and Left() could cut a pair apart
    const std::string   utf8 = aName.utf8_string();
    std::vector<size_t> starts;

    for( size_t i = 0; i < utf8.size(); ++i )
    {
        if( ( utf8[i] & 0xC0 ) != 0x80 )
            starts.push_back( i );
    }

    if( starts.size() <= MAX_NAME_LENGTH )
        return aName;

    // Footprint names differ at both ends, so keep the tail as well as the head
    const size_t tail = MAX_NAME_LENGTH / 2;
    const size_t head = MAX_NAME_LENGTH - tail - 1;

    return wxString::FromUTF8( utf8.substr( 0, starts[head] ) ) + wxS( "…" )
           + wxString::FromUTF8( utf8.substr( starts[starts.size() - tail] ) );
}


wxString MDW_PART_STATE::TreeCell( const wxString& aState, const MDW_LOCK_INFO& aLock, bool aModified )
{
    wxString cell = aState;

    if( aLock.is_locked && aLock.locked_by_me )
    {
        cell += SEPARATOR + _( "revising (you)" );
    }
    else if( aLock.is_locked )
    {
        // Short enough to fit beside the part name in a default editor pane
        cell += SEPARATOR + wxString::Format( _( "%s editing" ), MdwClaimHolder( aLock ) );
    }

    if( aModified )
        cell += SEPARATOR + _( "modified" );

    return cell;
}


wxString MDW_PART_STATE::Describe( const wxString& aName ) const
{
    if( !listed )
        return ElideName( aName ) + SEPARATOR + _( "Not checked in" );

    if( !lifecycle )
        return wxEmptyString;

    wxString line = ElideName( aName );

    if( known )
        line += SEPARATOR + StateName();

    if( pending )
    {
        line += SEPARATOR + ( pending->display_id.IsEmpty()
                                      ? _( "release awaiting approval" )
                                      : wxString::Format( _( "release awaiting approval in %s" ),
                                                          pending->display_id ) );
    }
    else if( release && MatchesRelease() )
    {
        line += SEPARATOR + wxString::Format( _( "matches Rev %s" ), release->label );
    }
    else if( release && !release->content_digest.IsEmpty() && !digest.IsEmpty() )
    {
        line += SEPARATOR + wxString::Format( _( "differs from Rev %s" ), release->label );
    }
    else if( release )
    {
        line += SEPARATOR + wxString::Format( _( "Rev %s" ), release->label );
    }
    else if( known )
    {
        line += SEPARATOR + _( "never released" );
    }

    if( IsRevising() )
        line += SEPARATOR + _( "revising" );

    if( IsClaimedByOther() )
        line += SEPARATOR + wxString::Format( _( "%s is editing this part" ), MdwClaimHolder( lock ) );

    return line;
}


wxString MDW_PART_STATE::EditRefusal( const wxString& aName ) const
{
    if( pending || ( known && state.category == MDW_LIFECYCLE_CATEGORY::PENDING_REVIEW ) )
        return wxString::Format( _( "%s is awaiting approval, so the change was not kept." ), aName );

    const std::vector<MDW_PART_ACTION> offered = Actions();

    auto allowed = [&]( MDW_PART_ACTION::ID aId )
    {
        return std::ranges::find_if( offered,
                                     [aId]( const MDW_PART_ACTION& aAction )
                                     {
                                         return aAction.id == aId && aAction.Allowed();
                                     } );
    };

    if( IsClaimedByOther() )
    {
        const wxString holder = MdwClaimHolder( lock );
        auto           releaseClaim = allowed( MDW_PART_ACTION::ID::RELEASE_CLAIM );
        wxString       command;

        if( releaseClaim == offered.end() )
            return wxString::Format( _( "%s is being revised by %s, so the change was not kept." ), aName, holder );

        if( !releaseClaim->label.EndsWith( wxS( "..." ), &command ) )
            command = releaseClaim->label;

        return wxString::Format( _( "%s is being revised by %s, so the change was not kept. To change it, choose %s, "
                                    "then Revise." ),
                                 aName, holder, command );
    }

    if( allowed( MDW_PART_ACTION::ID::REVISE ) == offered.end() )
        return wxString::Format( _( "%s is released, so the change was not kept." ), aName );

    return wxString::Format( _( "%s is released, so the change was not kept. Choose Revise to change it." ), aName );
}
