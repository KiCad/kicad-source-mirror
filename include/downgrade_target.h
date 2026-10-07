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

#pragma once

#include <vector>
#include <wx/string.h>

/// An older KiCad release we can export a project back to.
/// The version fields are that release's frozen file-format and schema stamps.
struct DOWNGRADE_TARGET
{
    wxString m_name;          ///< User-facing name, e.g. "KiCad 10.0"
    wxString m_id;            ///< Stable machine key for CLI and jobsets, e.g. "10.0"
    int      m_boardVersion;  ///< SEXPR_BOARD_FILE_VERSION for the target
    int      m_schVersion;    ///< SEXPR_SCHEMATIC_FILE_VERSION for the target
    int      m_symLibVersion; ///< SEXPR_SYMBOL_LIB_FILE_VERSION for the target
};


/// Supported downgrade targets, newest first.
/// Only the last release or two. Exporting further back loses too much.
inline const std::vector<DOWNGRADE_TARGET>& GetDowngradeTargets()
{
    static const std::vector<DOWNGRADE_TARGET> targets = {
        // name              id         board      sch        symLib
        { wxT( "KiCad 10.0" ), wxT( "10.0" ), 20260206, 20260306, 20251024 },
        { wxT( "KiCad 9.0" ), wxT( "9.0" ), 20241229, 20250114, 20241209 },
    };

    return targets;
}


/// Match a user-supplied target string against name or stable id, case-insensitively.
inline const DOWNGRADE_TARGET* FindDowngradeTarget( const wxString& aNameOrId )
{
    for( const DOWNGRADE_TARGET& target : GetDowngradeTargets() )
    {
        if( target.m_name.CmpNoCase( aNameOrId ) == 0 || target.m_id.CmpNoCase( aNameOrId ) == 0 )
            return &target;
    }

    return nullptr;
}


/// The valid target choices as one user-facing list, for error messages.
inline wxString DowngradeTargetNames()
{
    wxString names;

    for( const DOWNGRADE_TARGET& target : GetDowngradeTargets() )
    {
        names += wxString::Format( names.IsEmpty() ? wxT( "'%s' ('%s')" ) : wxT( ", '%s' ('%s')" ), target.m_name,
                                   target.m_id );
    }

    return names;
}
