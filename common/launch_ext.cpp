/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <launch_ext.h>
#include <wx/utils.h>


bool LaunchExternal( const wxString& aPath )
{
#ifdef __WXMAC__

    const wchar_t* args[] = { L"open", aPath.wc_str(), nullptr };
    return wxExecute( const_cast<wchar_t**>( args ) ) != -1;

#elif defined( __WXGTK__ ) && !wxCHECK_VERSION( 3, 1, 1 )
    // On Unix systems `wxLaunchDefaultApplication()` before wxWidgets 3.1.1 mistakenly uses
    // `wxExecute(xdg_open + " " + document)`, thereby failing for filenames with spaces. Below is
    // a backport of the fixed `wxLaunchDefaultApplication()`, to be used until we switch to a
    // newer version of wxWidgets.

    wxString PATH, xdg_open;

    if( wxGetEnv( "PATH", &PATH ) && wxFindFileInPath( &xdg_open, PATH, "xdg-open" ) )
    {
        const char* argv[3];
        argv[0] = xdg_open.fn_str();
        argv[1] = aPath.fn_str();
        argv[2] = nullptr;

        if( wxExecute( const_cast<char**>( argv ) ) )
            return true;
    }

    return false;

#else

    wxString path( aPath );
    return wxLaunchDefaultApplication( path );

#endif
}
