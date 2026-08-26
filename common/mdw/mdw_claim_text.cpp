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

#include <mdw/mdw_claim_text.h>
#include <mdw/client/mdw_events.h>

#include <nlohmann/json.hpp>

#include <algorithm>

#include <wx/intl.h>
#include <wx/datetime.h>


wxString MdwClaimHolder( const MDW_LOCK_INFO& aLock )
{
    if( !aLock.owner_display_name.IsEmpty() )
        return aLock.owner_display_name;

    return aLock.owner_id.IsEmpty() ? _( "another user" ) : aLock.owner_id;
}


wxString MdwClaimLabel( const MDW_LOCK_INFO& aLock, const wxDateTime& aNow )
{
    if( !aLock.is_locked )
        return aLock.known ? wxString() : _( "Editing claim unknown" );

    wxString          who = aLock.locked_by_me ? _( "you" ) : MdwClaimHolder( aLock );
    const wxDateTime& since = aLock.lock_timestamp;

    if( !since.IsValid() )
        return wxString::Format( _( "Being edited by %s" ), who );

    if( since.IsSameDate( aNow ) )
        return wxString::Format( _( "Being edited by %s since %s" ), who, since.Format( wxS( "%H:%M" ) ) );

    // A claim left standing for weeks is the one worth questioning, so its age is spelled out
    int days = std::max( 1, ( aNow - since ).GetDays() );

    return wxString::Format( _( "Being edited by %s since %s (%s)" ), who, since.FormatDate(),
                             wxString::Format( wxPLURAL( "%d day", "%d days", days ), days ) );
}


wxString MdwClaimSavePrompt( const MDW_LOCK_INFO& aLock, const wxString& aItem )
{
    wxString who = MdwClaimHolder( aLock );

    if( aLock.advisory )
    {
        return wxString::Format( _( "%s is editing \"%s\".\n\nYou can save and share your changes, but %s will "
                                    "have to merge them." ),
                                 who, aItem, who );
    }

    return wxString::Format( _( "\"%s\" is checked out by %s in the Design Workspace.\n\nYour local changes "
                                "cannot be checked in until %s releases it." ),
                             aItem, who, who );
}


wxString MdwClaimCommitWarning( const MDW_LOCK_INFO& aLock, const wxString& aItem, bool aIHoldOne, size_t aContested )
{
    if( !aLock.known && aContested > 1 )
    {
        return wxString::Format( _( "Editing claims on %zu of these files have not been read yet, so a teammate may be "
                                    "editing them. Sharing these changes may require a merge." ),
                                 aContested );
    }

    if( !aLock.known )
    {
        return wxString::Format( _( "The editing claim on %s has not been read yet, so a teammate may be editing it. "
                                    "Sharing these changes may require a merge." ),
                                 aItem );
    }

    if( aContested > 1 && aIHoldOne )
    {
        return wxString::Format( _( "Teammates have also marked %zu of these files for editing. Sharing these "
                                    "changes may require a merge." ),
                                 aContested );
    }

    if( aContested > 1 )
    {
        return wxString::Format( _( "Teammates have marked %zu of these files for editing. Sharing these "
                                    "changes may require a merge." ),
                                 aContested );
    }

    if( aIHoldOne )
    {
        return wxString::Format( _( "%s has also marked %s for editing. Sharing these changes may require a "
                                    "merge." ),
                                 MdwClaimHolder( aLock ), aItem );
    }

    return wxString::Format( _( "%s has marked %s for editing. Sharing these changes may require a merge." ),
                             MdwClaimHolder( aLock ), aItem );
}


std::optional<wxString> MdwClaimNotice( const MDW_EVENT& aEvent )
{
    if( aEvent.type != MDW_EVENT_TYPE::ITEM_UNLOCKED )
        return std::nullopt;

    nlohmann::json payload = nlohmann::json::parse( aEvent.payload.utf8_string(), nullptr, false );

    if( !payload.is_object() )
        return std::nullopt;

    auto flag = [&]( const char* aKey )
    {
        auto it = payload.find( aKey );
        return it != payload.end() && it->is_boolean() && it->get<bool>();
    };

    auto text = [&]( const char* aKey )
    {
        auto it = payload.find( aKey );
        return it != payload.end() && it->is_string() ? wxString::FromUTF8( it->get<std::string>() ) : wxString();
    };

    // The backend judges both by user, so a release from this user's other client is no news
    if( !flag( "locked_by_me" ) || flag( "released_by_me" ) )
        return std::nullopt;

    wxString name = aEvent.path_hint.IsEmpty() ? aEvent.item_id : aEvent.path_hint;
    wxString by = text( "released_by" );
    wxString reason = text( "reason" );

    // A backend that cannot tell who released it may be reporting this user's own release, so nobody is blamed
    if( by.IsEmpty() )
    {
        auto advisory = payload.find( "advisory" );

        if( advisory != payload.end() && advisory->is_boolean() && !advisory->get<bool>() )
            return wxString::Format( _( "Your check out of %s is no longer held." ), name );

        return wxString::Format( _( "Your editing claim on %s is no longer held." ), name );
    }

    if( reason.IsEmpty() )
        return wxString::Format( _( "%s released your editing claim on %s." ), by, name );

    return wxString::Format( _( "%s released your editing claim on %s: %s" ), by, name, reason );
}


std::vector<MDW_CLAIM_ACTION> MdwClaimActions( const MDW_LOCK_INFO& aLock )
{
    using KIND = MDW_CLAIM_ACTION::KIND;

    // Unread claims may hide a teammate's, so starting stays possible but never reads as free
    if( !aLock.is_locked && !aLock.known )
        return { { KIND::START, _( "Start Editing (claims not read yet)" ), true, wxEmptyString, MDW_LOCK_INFO() } };

    if( !aLock.is_locked )
        return { { KIND::START, _( "Start Editing" ), true, wxEmptyString, MDW_LOCK_INFO() } };

    if( aLock.locked_by_me )
    {
        return { { KIND::STOP, aLock.advisory ? _( "Stop Editing" ) : _( "Undo Check Out" ), true, wxEmptyString,
                   MDW_LOCK_INFO() } };
    }

    const wxString who = MdwClaimHolder( aLock );

    MDW_CLAIM_ACTION start{ KIND::START, wxString::Format( _( "Start Editing (%s is editing)" ), who ), false,
                            wxEmptyString, MDW_LOCK_INFO() };
    MDW_CLAIM_ACTION release;
    release.kind = KIND::RELEASE;
    release.claim = aLock;

    if( aLock.advisory )
    {
        release.label = wxString::Format( _( "Release %s's Claim..." ), who );
        release.confirm =
                wxString::Format( _( "Release %s's editing claim? %s will be told, with your reason." ), who, who );
    }
    else
    {
        release.label = wxString::Format( _( "Break %s's Lock..." ), who );
        release.confirm = wxString::Format( _( "Break the lock %s holds? Changes %s has not checked in will have "
                                               "to be merged." ),
                                            who, who );
    }

    return { start, release };
}


std::optional<MDW_CLAIM_ACTION> MdwClaimRelease( const MDW_LOCK_INFO& aLock )
{
    for( const MDW_CLAIM_ACTION& action : MdwClaimActions( aLock ) )
    {
        if( action.kind == MDW_CLAIM_ACTION::KIND::RELEASE )
            return action;
    }

    return std::nullopt;
}
