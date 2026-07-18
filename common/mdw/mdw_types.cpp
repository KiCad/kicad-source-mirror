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

#include <mdw/client/mdw_types.h>

#include <wx/translation.h>


wxString MdwErrorMessage( MDW_ERROR_CODE aCode )
{
    // Every code is listed and there is no default, so adding one to the enum
    // fails the build here rather than reaching a user as a generic message
    switch( aCode )
    {
    case MDW_ERROR_CODE::SUCCESS:                  return wxEmptyString;
    case MDW_ERROR_CODE::NOT_SUPPORTED:            return _( "The backend does not support this operation." );
    case MDW_ERROR_CODE::NOT_AUTHENTICATED:        return _( "Not authenticated to the workspace." );
    case MDW_ERROR_CODE::AUTH_FAILED:              return _( "Authentication failed." );
    case MDW_ERROR_CODE::PERMISSION_DENIED:        return _( "Permission denied." );
    case MDW_ERROR_CODE::NETWORK:                  return _( "A network error occurred." );
    case MDW_ERROR_CODE::TIMEOUT:                  return _( "The operation timed out." );
    case MDW_ERROR_CODE::LOCK_CONFLICT:            return _( "The item is locked by another user." );
    case MDW_ERROR_CODE::NOT_LOCK_OWNER:           return _( "You do not hold the lock on this item." );
    case MDW_ERROR_CODE::ITEM_NOT_FOUND:           return _( "The item was not found." );
    case MDW_ERROR_CODE::REVISION_NOT_FOUND:       return _( "The revision was not found." );
    case MDW_ERROR_CODE::VERSION_CONFLICT:         return _( "The item changed in the workspace since it was "
                                                             "checked out." );
    case MDW_ERROR_CODE::INVALID_STATE_TRANSITION: return _( "That lifecycle transition is not allowed." );
    case MDW_ERROR_CODE::NO_CHANGES:               return _( "There are no changes to check in." );
    case MDW_ERROR_CODE::CANCELLED:                return _( "The operation was cancelled." );
    case MDW_ERROR_CODE::INVALID_PARAMETER:        return _( "The workspace rejected a value as invalid." );
    case MDW_ERROR_CODE::BACKEND_LOST:             return _( "The workspace backend is not available." );
    case MDW_ERROR_CODE::PROTOCOL:                 return _( "The backend sent an unexpected reply." );
    case MDW_ERROR_CODE::INTERNAL:                 return _( "The workspace backend reported an internal "
                                                             "error." );
    case MDW_ERROR_CODE::RATE_LIMITED:             return _( "The workspace server is limiting requests. Try again "
                                                             "later." );
    case MDW_ERROR_CODE::BUSY:                     return _( "The workspace is busy with another operation. Try "
                                                             "again shortly." );
    case MDW_ERROR_CODE::UNTRUSTED_HOST:           return _( "The workspace server's identity has not been "
                                                             "confirmed." );
    case MDW_ERROR_CODE::COPY_BEHIND:              return _( "The library copy is out of date. Get the latest "
                                                             "version, then try again." );
    }

    return _( "The workspace operation failed." );
}


wxString MdwErrorText( const MDW_RESULT& aResult )
{
    constexpr size_t MAX_DETAIL = 300;

    const wxString headline = MdwErrorMessage( aResult );
    wxString       detail = aResult.detail;

    if( detail.IsEmpty() )
        return headline;

    // A refusal explains itself at the end, so the tail is kept, from a whole line when one is left
    if( detail.length() > MAX_DETAIL )
    {
        detail = detail.Right( MAX_DETAIL );

        const int lineEnd = detail.Find( wxS( '\n' ) );

        if( lineEnd != wxNOT_FOUND && lineEnd + 1 < static_cast<int>( detail.length() ) )
            detail = detail.Mid( lineEnd + 1 );

        detail = wxS( "..." ) + detail;
    }

    return headline + wxS( "\n\n" ) + detail;
}


wxString MdwErrorMessage( const MDW_RESULT& aResult )
{
    if( !aResult.message.IsEmpty() )
        return aResult.message;

    const bool signIn =
            aResult.error == MDW_ERROR_CODE::AUTH_FAILED || aResult.error == MDW_ERROR_CODE::NOT_AUTHENTICATED;

    if( !signIn || !aResult.auth_challenge || aResult.auth_challenge->realm.IsEmpty() )
        return MdwErrorMessage( aResult.error );

    // The realm is an address, so its scheme says nothing the user needs
    wxString       server = aResult.auth_challenge->realm;
    const wxString user = aResult.auth_challenge->username_hint;
    const int      scheme = server.Find( wxS( "://" ) );

    if( scheme != wxNOT_FOUND )
        server = server.Mid( scheme + 3 );

    if( aResult.error == MDW_ERROR_CODE::NOT_AUTHENTICATED )
        return wxString::Format( _( "The server at %s needs you to sign in." ), server );

    if( user.IsEmpty() )
        return wxString::Format( _( "The server at %s refused the credentials." ), server );

    return wxString::Format( _( "The server at %s refused the credentials for %s." ), server, user );
}
