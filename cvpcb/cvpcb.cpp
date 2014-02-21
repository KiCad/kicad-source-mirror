/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file cvpcb.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <appl_wxstruct.h>
#include <wxstruct.h>
#include <confirm.h>
#include <gestfich.h>

#include <3d_viewer.h>
#include <cvpcb.h>
#include <zones.h>
#include <cvpcb_mainframe.h>
#include <colors_selection.h>
#include <cvpcb_id.h>

#include <build_version.h>

#include <wx/snglinst.h>

// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings;

// Constant string definitions for CvPcb
const wxString RetroFileExtension( wxT( "stf" ) );
const wxString FootprintAliasFileExtension( wxT( "equ" ) );

// Wildcard for schematic retroannotation (import footprint names in schematic):
const wxString FootprintAliasFileWildcard( _( "KiCad footprint alias files (*.equ)|*.equ" ) );

const wxString titleLibLoadError( _( "Library Load Error" ) );


/*
 * MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void EDA_APP::MacOpenFile( const wxString& aFileName )
{
    wxFileName  filename = aFileName;
    wxString    oldPath;

    CVPCB_MAINFRAME* frame = (CVPCB_MAINFRAME*) GetTopWindow();

    if( !filename.FileExists() )
        return;

    if( frame->m_NetlistFileName.DirExists() )
        oldPath = frame->m_NetlistFileName.GetPath();

    // Update the library search path list.
    if( wxGetApp().GetLibraryPathList().Index( oldPath ) != wxNOT_FOUND )
        wxGetApp().GetLibraryPathList().Remove( oldPath );

    wxGetApp().GetLibraryPathList().Insert( filename.GetPath(), 0 );

    frame->m_NetlistFileName = filename;
    frame->ReadNetListAndLinkFiles();
}


// Create a new application object
IMPLEMENT_APP( EDA_APP )


/************************************/
/* Called to initialize the program */
/************************************/

bool EDA_APP::OnInit()
{
    wxFileName       filename;
    wxString         message;
    CVPCB_MAINFRAME* frame = NULL;

    InitEDA_Appl( wxT( "CvPcb" ), APP_CVPCB_T );

    SetFootprintLibTablePath();

    // Set 3D shape path from environment variable KISYS3DMOD
    Set3DShapesPath( wxT(KISYS3DMOD) );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "CvPcb is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
    {
        filename = argv[1];
        wxSetWorkingDirectory( filename.GetPath() );
    }

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    g_DrawBgColor = BLACK;

    wxString Title = GetTitle() + wxT( " " ) + GetBuildVersion();
    frame = new CVPCB_MAINFRAME( Title );

    // Show the frame
    SetTopWindow( frame );
    frame->Show( true );
    frame->m_NetlistFileExtension = wxT( "net" );

    if( filename.IsOk() && filename.FileExists() )
    {
        frame->m_NetlistFileName = filename;
        frame->LoadProjectFile( filename.GetFullPath() );

        if( frame->ReadNetListAndLinkFiles() )
        {
            frame->m_NetlistFileExtension = filename.GetExt();
            return true;
        }
    }

    frame->UpdateTitle();

    return true;
}
