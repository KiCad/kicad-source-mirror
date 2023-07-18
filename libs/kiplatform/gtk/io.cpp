/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <wx/crt.h>
#include <wx/string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

FILE* KIPLATFORM::IO::SeqFOpen( const wxString& aPath, const wxString& aMode )
{
    FILE* fp = wxFopen( aPath, aMode );

    if( fp )
        posix_fadvise( fileno( fp ), 0, 0, POSIX_FADV_SEQUENTIAL );

    return fp;
}

bool KIPLATFORM::IO::DuplicatePermissions( const wxString &aSrc, const wxString &aDest )
{
    struct stat sourceStat;
    if( stat( aSrc.fn_str(), &sourceStat ) == 0 )
    {
        mode_t permissions = sourceStat.st_mode & ( S_IRWXU | S_IRWXG | S_IRWXO );
        if( chmod( aDest.fn_str(), permissions ) == 0 )
        {
            return true;
        }
        else
        {
            // Handle error
            return false;
        }
    }
    else
    {
        // Handle error
        return false;
    }
}
