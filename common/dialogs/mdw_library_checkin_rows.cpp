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

#include <dialogs/mdw_library_checkin_rows.h>

#include <mdw/client/mdw_lifecycle.h>
#include <mdw/client/mdw_types.h>

#include <wx/intl.h>

#include <algorithm>


std::vector<MDW_CHECKIN_ROW> MdwCheckInRows( const std::vector<MDW_LOCAL_CHANGE>& aChanges,
                                             const MDW_LIBRARY_COPY& aCopy, const wxString& aLibraryId )
{
    std::vector<MDW_CHECKIN_ROW> rows;

    auto libraryOrder = [&]( const wxString& aId ) -> size_t
    {
        for( size_t ii = 0; ii < aCopy.libraries.size(); ++ii )
        {
            if( aCopy.libraries[ii].library_id == aId )
                return ii;
        }

        return aCopy.libraries.size();
    };

    for( const MDW_LOCAL_CHANGE& change : aChanges )
    {
        MDW_CHECKIN_ROW& row = rows.emplace_back();
        row.change = change;
        row.library = change.library_id;

        for( const MDW_COPY_LIBRARY& library : aCopy.libraries )
        {
            if( library.library_id == change.library_id && !library.nickname.IsEmpty() )
            {
                row.library = library.nickname;

                // A copy registers its symbol and footprint libraries under one nickname
                bool shared = std::count_if( aCopy.libraries.begin(), aCopy.libraries.end(),
                                             [&]( const MDW_COPY_LIBRARY& aOther )
                                             {
                                                 return aOther.nickname == library.nickname;
                                             } ) > 1;

                if( shared && library.type == MDW_LIBRARY_TYPE::SYMBOL )
                    row.library = wxString::Format( _( "%s (symbols)" ), library.nickname );
                else if( shared && library.type == MDW_LIBRARY_TYPE::FOOTPRINT )
                    row.library = wxString::Format( _( "%s (footprints)" ), library.nickname );

                break;
            }
        }

        row.allowed = change.checkin.state() != kiapi::workspace::PS_DENIED;

        row.ticked = row.allowed && change.library_id == aLibraryId;
    }

    std::stable_sort( rows.begin(), rows.end(),
                      [&]( const MDW_CHECKIN_ROW& aA, const MDW_CHECKIN_ROW& aB )
                      {
                          size_t orderA = libraryOrder( aA.change.library_id );
                          size_t orderB = libraryOrder( aB.change.library_id );

                          if( orderA != orderB )
                              return orderA < orderB;

                          return aA.change.name.CmpNoCase( aB.change.name ) < 0;
                      } );

    return rows;
}


void MdwCarryTicks( std::vector<MDW_CHECKIN_ROW>& aRows, const std::vector<MDW_CHECKIN_ROW>& aOld )
{
    for( MDW_CHECKIN_ROW& row : aRows )
    {
        for( const MDW_CHECKIN_ROW& old : aOld )
        {
            if( old.change.library_id == row.change.library_id && old.change.name == row.change.name )
            {
                row.ticked = row.allowed && old.ticked;
                break;
            }
        }
    }
}


std::vector<MDW_LOCAL_CHANGE> MdwTickedChanges( const std::vector<MDW_CHECKIN_ROW>& aRows )
{
    std::vector<MDW_LOCAL_CHANGE> changes;

    for( const MDW_CHECKIN_ROW& row : aRows )
    {
        if( row.allowed && row.ticked )
            changes.push_back( row.change );
    }

    return changes;
}


wxString MdwChangeText( MDW_CHANGE_TYPE aChange )
{
    switch( aChange )
    {
    case MDW_CHANGE_TYPE::ADDED: return _( "New" );
    case MDW_CHANGE_TYPE::REMOVED: return _( "Deleted" );
    case MDW_CHANGE_TYPE::MODIFIED: return _( "Modified" );
    case MDW_CHANGE_TYPE::UNKNOWN: break;
    }

    return _( "Changed" );
}


wxString MdwMemberOutcome( const MDW_MEMBER_RESULT& aResult )
{
    if( !aResult.status.IsOk() )
    {
        // A grid cell is one line, so the transport text follows on it
        wxString text = MdwErrorText( aResult.status );

        text.Replace( wxS( "\n" ), wxS( " " ) );
        return text;
    }

    if( aResult.change.outcome == MDW_CHANGE_OUTCOME::PENDING )
    {
        const MDW_PENDING_CHANGE& pending = aResult.change.pending;
        const wxString&           id = pending.display_id.IsEmpty() ? pending.change_id : pending.display_id;

        if( id.IsEmpty() )
            return _( "Waiting for approval" );

        return wxString::Format( _( "Waiting for approval in %s" ), id );
    }

    const wxString revision = MdwRevisionText( aResult.change.revision );

    if( revision.IsEmpty() )
        return _( "Checked in" );

    return wxString::Format( _( "Checked in as %s" ), revision );
}


std::vector<wxString> MdwAdoptedFiles( const std::vector<MDW_LOCAL_CHANGE>& aSent, const MDW_LIBRARY_CHECKIN& aCheckIn )
{
    std::vector<wxString> adopted;

    if( aCheckIn.snapshot.token.IsEmpty() && aCheckIn.copy_snapshot.token.IsEmpty() )
        return adopted;

    for( const MDW_LOCAL_CHANGE& change : aSent )
    {
        auto skipped = [&]( const MDW_SKIPPED_FILE& aSkipped )
        {
            return aSkipped.path == change.relative_path;
        };

        if( !change.format_digest.IsEmpty() && std::ranges::none_of( aCheckIn.skipped_files, skipped ) )
            adopted.push_back( change.relative_path );
    }

    return adopted;
}
