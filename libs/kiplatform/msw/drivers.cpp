/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/version_compare.h>
#include <kiplatform/drivers.h>

#include <Windows.h>
#include <iostream>

#define MIN_WIN_VERSION "10.7.2"

bool KIPLATFORM::DRIVERS::Valid3DConnexionDriverVersion()
{
    HKEY        hKey;
    std::string version;

    // Open the registry key for 3dConnexion
    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      L"Software\\3Dconnexion\\3DxWare Version",
                      0, KEY_READ, &hKey ) == ERROR_SUCCESS )
    {
        char  buffer[256];
        DWORD bufferSize = sizeof( buffer );

        // Query the version value
        if( RegQueryValueEx( hKey, nullptr, nullptr, nullptr, ( LPBYTE ) buffer, &bufferSize )
            == ERROR_SUCCESS )
        {
            version = buffer;
        }

        RegCloseKey( hKey );
    }

    return !version.empty() && compareVersionStrings( MIN_WIN_VERSION, version );
}