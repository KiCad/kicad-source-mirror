/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2020 Mark Roszko <mark.roszko@gmail.com>
* Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiplatform/app.h>

#include <wx/string.h>

#include <windows.h>
#include <strsafe.h>


bool KIPLATFORM::APP::RegisterApplicationRestart( const wxString& aCommandLine )
{
    HRESULT hr = S_OK; // not if registering for recovery and restart fails.
    WCHAR   wsCommandLine[RESTART_MAX_CMD_LINE];
    RtlZeroMemory( wsCommandLine, sizeof( wsCommandLine ) );

    StringCchCopyW( wsCommandLine, sizeof( wsCommandLine ), aCommandLine.wc_str() );

    hr = ::RegisterApplicationRestart( wsCommandLine, RESTART_NO_PATCH );

    return SUCCEEDED( hr );
}


bool KIPLATFORM::APP::UnregisterApplicationRestart()
{
    // Note, this isn't required to be used on Windows if you are just closing the program
    return SUCCEEDED( ::UnregisterApplicationRestart() );
}