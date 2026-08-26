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

#ifndef MDW_LIBRARY_CHECKIN_ROWS_H
#define MDW_LIBRARY_CHECKIN_ROWS_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_library_sources.h>

#include <wx/string.h>

#include <vector>


/// One local change of a team library copy, as the check-in dialog lists it
struct MDW_CHECKIN_ROW
{
    MDW_LOCAL_CHANGE change;
    wxString         library;         ///< The nickname the editors show, typed when two libraries share it
    bool             allowed = false; ///< The source does not deny it a check-in
    bool             ticked = false;
};


/**
 * The rows for aChanges in a copy's library order, then by name. A row is allowed unless its change's checkin
 * permission is denied, and ticked when allowed and in aLibraryId, the library the user started from.
 */
KICOMMON_API std::vector<MDW_CHECKIN_ROW> MdwCheckInRows( const std::vector<MDW_LOCAL_CHANGE>& aChanges,
                                                          const MDW_LIBRARY_COPY& aCopy, const wxString& aLibraryId );

/**
 * Carry the user's ticks from aOld onto aRows after the list is read again. A member is matched by library and
 * name, since provisional ids do not survive a re-list. Rows that are new keep their default tick.
 */
KICOMMON_API void MdwCarryTicks( std::vector<MDW_CHECKIN_ROW>& aRows, const std::vector<MDW_CHECKIN_ROW>& aOld );

/// The ticked rows' changes, which is what a check-in sends
KICOMMON_API std::vector<MDW_LOCAL_CHANGE> MdwTickedChanges( const std::vector<MDW_CHECKIN_ROW>& aRows );

KICOMMON_API wxString MdwChangeText( MDW_CHANGE_TYPE aChange );

/// What a check-in did with one member, for the report
KICOMMON_API wxString MdwMemberOutcome( const MDW_MEMBER_RESULT& aResult );

/// The files aSent checked in whole for their format alone that aCheckIn committed. They name no member, so no
/// member result reports them
KICOMMON_API std::vector<wxString> MdwAdoptedFiles( const std::vector<MDW_LOCAL_CHANGE>& aSent,
                                                    const MDW_LIBRARY_CHECKIN&           aCheckIn );

#endif // MDW_LIBRARY_CHECKIN_ROWS_H
