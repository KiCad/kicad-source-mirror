/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <kiplatform/io.h>

#include <wx/crt.h>
#include <wx/string.h>
#include <wx/filename.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

FILE* KIPLATFORM::IO::SeqFOpen( const wxString& aPath, const wxString& aMode )
{
    FILE* fp = wxFopen( aPath, aMode );

    if( fp )
    {
        if( posix_fadvise( fileno( fp ), 0, 0, POSIX_FADV_SEQUENTIAL ) != 0 )
        {
            fclose( fp );
            fp = nullptr;
        }
    }

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

bool KIPLATFORM::IO::MakeWriteable( const wxString& aFilePath )
{
    struct stat fileStat;
    if( stat( aFilePath.fn_str(), &fileStat ) == 0 )
    {
        // Add user write permission to existing permissions
        mode_t newPermissions = fileStat.st_mode | S_IWUSR;
        if( chmod( aFilePath.fn_str(), newPermissions ) == 0 )
        {
            return true;
        }
    }
    return false;
}

bool KIPLATFORM::IO::IsFileHidden( const wxString& aFileName )
{
    wxFileName fn( aFileName );

    return fn.GetName().StartsWith( wxT( "." ) );
}


void KIPLATFORM::IO::LongPathAdjustment( wxFileName& aFilename )
{
    // no-op
}