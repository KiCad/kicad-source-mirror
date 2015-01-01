/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2008 KiCad Developers, see change_log.txt for contributors.
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

/*
 * This file contains some functions used in the PCB
 * applications Pcbnew and CvPcb.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>

#include <pcbcommon.h>
#include <class_board.h>
#include <3d_viewer.h>

/**
 * attempts to set the environment variable given by aKiSys3Dmod to a valid path.
 * (typically "KISYS3DMOD" )
 * If the environment variable is already set, then it left as is to respect
 * the wishes of the user.
 *
 * The path is determined by attempting to find the path modules/packages3d
 * files in kicad tree.
 * This may or may not be the best path but it provides the best solution for
 * backwards compatibility with the previous 3D shapes search path implementation.
 *
 * @note This must be called after #SetBinDir() is called at least on Windows.
 * Otherwise, the kicad path is not known (Windows specific)
 *
 * @param aKiSys3Dmod = the value of environment variable, typically "KISYS3DMOD"
 * @param aProcess = the current process
 * @return false if the aKiSys3Dmod path is not valid.
 */
bool Set3DShapesDefaultPath( const wxString& aKiSys3Dmod, const PGM_BASE* aProcess )
{
    wxString    path;

    // Set the KISYS3DMOD environment variable for the current process,
    // if it is not already defined in the user's environment and valid.
    if( wxGetEnv( aKiSys3Dmod, &path ) && wxFileName::DirExists( path ) )
        return true;

#if 1
    // Try to find a valid path is standard KiCad paths
    SEARCH_STACK&   search = Kiface().KifaceSearch();
    path = search.FindValidPath( LIB3D_FOLDER );

    if( !path.IsEmpty() )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }
#endif

    // Attempt to determine where the 3D shape libraries were installed using the
    // legacy path:
    // on Unix: /usr/local/kicad/share/modules/packages3d
    // oor /usr/local/kicad/share/kicad/modules/packages3d
    // or  /usr/share/kicad/modules/packages3d
    // On Windows: bin../share/modules/packages3d
    wxString relpath( wxT( "modules/" ) );
    relpath += LIB3D_FOLDER;

// Apple MacOSx
#ifdef __WXMAC__
    path = wxT("/Library/Application Support/kicad/modules/packages3d/");

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

    path = wxString( wxGetenv( wxT( "HOME" ) ) ) + wxT("/Library/Application Support/kicad/modules/packages3d/");

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

#elif defined(__UNIX__)     // Linux and non-Apple Unix
    // Try the home directory:
    path.Empty();
    wxGetEnv( wxT("HOME"), &path );
    path += wxT("/kicad/share/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

    path.Empty();
    wxGetEnv( wxT("HOME"), &path );
    path += wxT("/kicad/share/kicad/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

    // Try the standard install path:
    path = wxT("/usr/local/kicad/share/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

    // Try the new standard install path:
    path = wxT("/usr/local/kicad/share/kicad/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }
    // Try the official distrib standard install path:
    path = wxT("/usr/share/kicad/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

#else   // Windows
    // On Windows, the install path is given by the path of executables
    wxFileName fn;
    fn.AssignDir( aProcess->GetExecutablePath() );
    fn.RemoveLastDir();
    path = fn.GetPathWithSep() + wxT("share/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }
#endif

    return false;
}


wxString LayerMaskDescribe( const BOARD *aBoard, LSET aMask )
{
    // Try the single or no- layer case (easy)
    LAYER_ID layer = aMask.ExtractLayer();

    switch( (int) layer )
    {
    case UNSELECTED_LAYER:
        return _( "No layers" );

    case UNDEFINED_LAYER:
        break;

    default:
        return aBoard->GetLayerName( layer );
    }

    // Try to be smart and useful, starting with outer copper
    // (which are more important than internal ones)
    wxString layerInfo;

    if( aMask[F_Cu] )
        AccumulateDescription( layerInfo, aBoard->GetLayerName( F_Cu ) );

    if( aMask[B_Cu] )
        AccumulateDescription( layerInfo, aBoard->GetLayerName( B_Cu ) );

    if( ( aMask & LSET::InternalCuMask() ).any() )
        AccumulateDescription( layerInfo, _("Internal" ) );

    if( ( aMask & LSET::AllNonCuMask() ).any() )
        AccumulateDescription( layerInfo, _("Non-copper" ) );

    return layerInfo;
}

