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

#include <kiplatform/io.h>

#include <wx/string.h>

#include <windows.h>


FILE* KIPLATFORM::IO::SeqFOpen( const wxString& aPath, const wxString& mode )
{
#ifdef _MSC_VER
    // We need to use the win32 api to setup a file handle with sequential scan flagged
    // and pass it up the chain to create a normal FILE stream
    HANDLE hFile = INVALID_HANDLE_VALUE;
    hFile = CreateFileW( aPath.wc_str(),
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_SEQUENTIAL_SCAN,
                         NULL );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    int fd = _open_osfhandle( reinterpret_cast<intptr_t>( hFile ), 0 );

    if( fd == -1 )
    {
        // close the handle manually as the ownership didnt transfer
        CloseHandle( hFile );
        return NULL;
    }

    FILE* fp = _fdopen( fd, mode.c_str() );

    if( !fp )
    {
        // close the file descriptor manually as the ownership didnt transfer
        _close( fd );
    }

    return fp;
#else
    // Fallback for MSYS2
    return wxFopen( aPath, aMode );
#endif
}