/**
 * @file kicad.cpp
 * @brief Main KiCad Project manager file
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <appl_wxstruct.h>

#ifdef USE_SPLASH_IMAGE
  #define SPLASH_IMAGE logo_kicad.png
  #include "wx/splash.h"
  #include "wx/mediactrl.h"
#endif

#include <kicad.h>
#include <tree_project_frame.h>
#include <wildcards_and_files_ext.h>

#include <build_version.h>

const wxString g_KicadPrjFilenameExtension( wxT( ".pro" ) );

/* Import functions */
char* GetFileName( char* FullPathName );
void  ShowLogo( char* FonteFileName );

/* Local functions */

/************************************/
/* Called to initialize the program */
/************************************/

// Create a new application object
IMPLEMENT_APP( EDA_APP )

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void EDA_APP::MacOpenFile( const wxString &fileName )
{
    KICAD_MANAGER_FRAME* frame = (KICAD_MANAGER_FRAME*) GetTopWindow();
    wxFileName fn = fileName;

    frame->m_ProjectFileName = fn;

    if( m_fileHistory.GetCount() )
    {
        frame->m_ProjectFileName = m_fileHistory.GetHistoryFile( 0 );

        if( !frame->m_ProjectFileName.FileExists() )
        {
            m_fileHistory.RemoveFileFromHistory( 0 );
        }
        else
        {
            wxCommandEvent cmd( 0, wxID_FILE1 );
            frame->OnFileHistory( cmd );
        }
    }

    wxString title = GetTitle() + wxT( " " ) + GetBuildVersion() +
                     wxT( " " ) + frame->m_ProjectFileName.GetFullPath();

    if( !fn.IsDirWritable() )
        title += _( " [Read Only]" );

    frame->SetTitle( title );

    frame->m_LeftWin->ReCreateTreePrj();

    frame->PrintMsg( _( "Working dir: " ) + frame->m_ProjectFileName.GetPath() +
                     _( "\nProject: " ) + frame->m_ProjectFileName.GetFullName() +
                     wxT( "\n" ) );
}


bool EDA_APP::OnInit()
{
    KICAD_MANAGER_FRAME* frame;

    InitEDA_Appl( wxT( "KiCad" ), APP_KICAD_T );

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    /* Make nameless project translatable */
    wxFileName namelessProject( wxGetCwd(), NAMELESS_PROJECT, ProjectFileExtension );

    frame = new KICAD_MANAGER_FRAME( NULL, wxT( "KiCad" ),
                                     wxPoint( 30, 20 ), wxSize( 600, 400 ) );

    if( argc > 1 )
    {
        frame->m_ProjectFileName = argv[1];
    }
    else if( m_fileHistory.GetCount() )
    {
        frame->m_ProjectFileName = m_fileHistory.GetHistoryFile( 0 );

        if( !frame->m_ProjectFileName.FileExists() )
        {
            m_fileHistory.RemoveFileFromHistory( 0 );
        }
        else
        {
            wxCommandEvent cmd( 0, wxID_FILE1 );
            frame->OnFileHistory( cmd );
        }
    }

    if( !frame->m_ProjectFileName.FileExists() )
    {
        wxCommandEvent cmd( 0, wxID_ANY );
        frame->m_ProjectFileName = namelessProject;
        frame->OnLoadProject( cmd );
    }

    wxString title = GetTitle() + wxT( " " ) + GetBuildVersion() +
                     wxT( " " ) + frame->m_ProjectFileName.GetFullPath();

    if( !namelessProject.IsDirWritable() )
        title += _( " [Read Only]" );

    frame->SetTitle( title );
    frame->ReCreateMenuBar();
    frame->RecreateBaseHToolbar();

    frame->m_LeftWin->ReCreateTreePrj();
    SetTopWindow( frame );

    /* Splash screen logo */
#ifdef USE_SPLASH_IMAGE
    wxBitmap bmp;
    wxString binDir = GetTraits()->GetStandardPaths().GetExecutablePath() +
                      wxFileName::GetPathSeparator();

    if( bmp.LoadFile( binDir + _T( "logokicad.png" ), wxBITMAP_TYPE_PNG ) )
    {
        wxSplashScreen* splash = new wxSplashScreen( splash_screen,
                                                     wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
                                                     3000,
                                                     frame,
                                                     wxID_ANY,
                                                     wxDefaultPosition,
                                                     wxDefaultSize,
                                                     wxSIMPLE_BORDER | wxSTAY_ON_TOP );
    }
#endif /* USE_SPLASH_IMAGE */

    frame->Show( true );
    frame->Raise();

    return true;
}
