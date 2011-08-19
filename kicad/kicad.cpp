/**
 * @file kicad.cpp
 * @brief Main kicad library manager file
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "bitmaps.h"
#include "colors.h"

#ifdef USE_SPLASH_IMAGE
  #define SPLASH_IMAGE logo_kicad.png
  #include "wx/splash.h"
  #include "wx/mediactrl.h"
#endif

#include "kicad.h"
#include "tree_project_frame.h"
#include "macros.h"

#include "build_version.h"

const wxString g_KicadPrjFilenameExtension( wxT( ".pro" ) );

/* Import functions */
char* GetFileName( char* FullPathName );
void  ShowLogo( char* FonteFileName );

/* Local functions */

/************************************/
/* Called to initialize the program */
/************************************/

// Create a new application object
IMPLEMENT_APP( WinEDA_App )

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void WinEDA_App::MacOpenFile( const wxString &fileName )
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


bool WinEDA_App::OnInit()
{
    KICAD_MANAGER_FRAME* frame;

    InitEDA_Appl( wxT( "KiCad" ), APP_TYPE_KICAD );

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
