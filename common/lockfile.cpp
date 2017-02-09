/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <lockfile.h>

#include <wx/filename.h>
#include <wx/snglinst.h>

#include <common.h>

std::unique_ptr<wxSingleInstanceChecker> LockFile( const wxString& aFileName )
{
    // first make absolute and normalize, to avoid that different lock files
    // for the same file can be created
    wxFileName fn( aFileName );

    fn.MakeAbsolute();

    wxString lockFileName = fn.GetFullPath() + ".lock";

    lockFileName.Replace( "/", "_" );

    // We can have filenames coming from Windows, so also convert Windows separator
    lockFileName.Replace( "\\", "_" );

    auto p = std::make_unique<wxSingleInstanceChecker>( lockFileName,
                                                        GetKicadLockFilePath() );

    if( p->IsAnotherRunning() )
    {
        p = nullptr;
    }

    return p;
}


wxString GetKicadLockFilePath()
{
    wxFileName lockpath;
    lockpath.AssignDir( wxGetHomeDir() ); // Default wx behavior

#if defined( __WXMAC__ )
    // In OSX use the standard per user cache directory
    lockpath.AppendDir( "Library" );
    lockpath.AppendDir( "Caches" );
    lockpath.AppendDir( "kicad" );
#elif defined( __UNIX__ )
    wxString envstr;
    // Try first the standard XDG_RUNTIME_DIR, falling back to XDG_CACHE_HOME
    if( wxGetEnv( "XDG_RUNTIME_DIR", &envstr ) && !envstr.IsEmpty() )
    {
        lockpath.AssignDir( envstr );
    }
    else if( wxGetEnv( "XDG_CACHE_HOME", &envstr ) && !envstr.IsEmpty() )
    {
        lockpath.AssignDir( envstr );
    }
    else
    {
        // If all fails, just use ~/.cache
        lockpath.AppendDir( ".cache" );
    }

    lockpath.AppendDir( "kicad" );
#endif

#if defined( __WXMAC__ ) || defined( __UNIX__ )
    if( !lockpath.DirExists() )
    {
        // Lockfiles should be only readable by the user
        lockpath.Mkdir( 0700, wxPATH_MKDIR_FULL );
    }
#endif
    return lockpath.GetPath();
}
