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

FILE* KIPLATFORM::IO::SeqFOpen( const wxString& aPath, const wxString& aMode )
{
    FILE* fp = wxFopen( aPath, aMode );

    posix_fadvise( fileno( fp ), 0, 0, POSIX_FADV_SEQUENTIAL );

    return fp;
}