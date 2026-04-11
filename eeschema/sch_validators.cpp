/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file sch_validators.cpp
 * @brief Implementation of control validators for schematic dialogs.
 */

#include <wx/combo.h>
#include <wx/msgdlg.h>

#include <sch_connection.h>
#include <sch_validators.h>
#include <project/net_settings.h>


// Match opening curly brace, preceeded by start-of-line or by a character not including $_^~
wxRegEx SCH_NETNAME_VALIDATOR::m_busGroupRegex( R"((^|[^$_\^~]){)", wxRE_ADVANCED );


wxString SCH_NETNAME_VALIDATOR::IsValid( const wxString& str ) const
{
    wxString msg = NETNAME_VALIDATOR::IsValid( str );

    if( !msg.IsEmpty() )
        return msg;

    // We don't do single-character validation here
    if( str.Length() == 1 )
        return wxString();

    // Figuring out if the user "meant" to make a bus group is somewhat tricky because curly
    // braces are also used for formatting and variable expansion

    if( m_busGroupRegex.Matches( str ) && str.Contains( '}' ) )
    {
        if( !NET_SETTINGS::ParseBusGroup( str, nullptr, nullptr ) )
            return _( "Signal name contains '{' and '}' but is not a valid bus name" );
    }
    else if( str.Contains( '[' ) || str.Contains( ']' ) )
    {
        if( !NET_SETTINGS::ParseBusVector( str, nullptr, nullptr ) )
            return _( "Signal name contains '[' or ']' but is not a valid bus name." );
    }

    return wxString();
}
