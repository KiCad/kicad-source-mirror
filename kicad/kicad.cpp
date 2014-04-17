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

#include <kicad.h>
#include <kiway.h>
#include <pgm_kicad.h>
#include <tree_project_frame.h>
#include <online_help.h>
#include <wildcards_and_files_ext.h>
#include <boost/ptr_container/ptr_vector.hpp>

#include <build_version.h>

// a dummy to quiet linking with EDA_BASE_FRAME::config();
#include <kiface_i.h>
KIFACE_I& Kiface()
{
    wxASSERT( 0 );  // should never be called, only reference is from EDA_BASE_FRAME::config();
    return (KIFACE_I&) *(KIFACE_I*) 0;
}

static PGM_KICAD program;

PGM_KICAD& Pgm()
{
    return program;
}


bool PGM_KICAD::OnPgmInit( wxApp* aWxApp )
{
    m_wx_app = aWxApp;      // first thing.

    m_bm.Init();

#if 0   // copied from single_top.c, possibly for milestone B)
    wxString absoluteArgv0 = wxStandardPaths::Get().GetExecutablePath();

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogSysError( wxT( "No meaningful argv[0]" ) );
        return false;
    }

    // Set LIB_ENV_VAR *before* loading the DSO, in case the top-level DSO holding the
    // KIFACE has hard dependencies on subsidiary DSOs below it.
    SetLibEnvVar( absoluteArgv0 );
#endif

    if( !initPgm() )
        return false;

    // Add search paths to feed the PGM_KICAD::SysSearch() function,
    // currenly limited in support to only look for project templates
    {
        SEARCH_STACK bases;

        SystemDirsAppend( &bases );

        // DBG( bases.Show( (std::string(__func__) + " bases").c_str() );)

        for( unsigned i = 0; i < bases.GetCount(); ++i )
        {
            wxFileName fn( bases[i], wxEmptyString );

            // Add KiCad template file path to search path list.
            fn.AppendDir( wxT( "template" ) );
            m_bm.m_search.AddPaths( fn.GetPath() );
        }

        //DBG( m_bm.m_search.Show( (std::string( __func__ ) + " SysSearch()").c_str() );)
    }

    // Read current setup and reopen last directory if no filename to open on
    // command line.
    if( App().argc == 1  )
    {
        wxString dir;

        if( PgmSettings()->Read( workingDirKey, &dir ) && wxDirExists( dir ) )
        {
            wxSetWorkingDirectory( dir );
        }
    }

    KICAD_MANAGER_FRAME* frame = new KICAD_MANAGER_FRAME( NULL, wxT( "KiCad" ),
                                     wxDefaultPosition, wxDefaultSize );
    App().SetTopWindow( frame );

    bool prjloaded = false;    // true when the project is loaded

    if( App().argc > 1 )
        frame->m_ProjectFileName = App().argv[1];

    else if( GetFileHistory().GetCount() )
    {
        // Try to open the last opened project,
        // if a project name is not given when starting Kicad
        frame->m_ProjectFileName = GetFileHistory().GetHistoryFile( 0 );

        if( !frame->m_ProjectFileName.FileExists() )
            GetFileHistory().RemoveFileFromHistory( 0 );
        else
        {
            wxCommandEvent cmd( 0, wxID_FILE1 );

            frame->OnFileHistory( cmd );
            prjloaded = true;    // OnFileHistory() loads the project
        }
    }

    if( !frame->m_ProjectFileName.FileExists() )
    {
        wxFileName namelessProject( wxGetCwd(), NAMELESS_PROJECT,
                                    ProjectFileExtension );

        frame->m_ProjectFileName = namelessProject;
    }

    if( !prjloaded )
    {
        wxCommandEvent cmd( 0, wxID_ANY );

        frame->OnLoadProject( cmd );
    }

    frame->Show( true );
    frame->Raise();

    return true;
}


void PGM_KICAD::OnPgmExit()
{
    saveCommonSettings();

    // write common settings to disk, and destroy everything in PGM_KICAD,
    // especially wxSingleInstanceCheckerImpl earlier than wxApp and earlier
    // than static destruction would.
    destroy();
}


void PGM_KICAD::MacOpenFile( const wxString& aFileName )
{
#if 0   // I'm tired, need a rest.

    KICAD_MANAGER_FRAME* frame = (KICAD_MANAGER_FRAME*) GetTopWindow();

    wxFileName fn = aFileName;

    frame->m_ProjectFileName = fn;

    if( !frame->m_ProjectFileName.FileExists() && m_fileHistory.GetCount() )
    {
        m_fileHistory.RemoveFileFromHistory( 0 );
        return;
    }

    wxCommandEvent loadEvent;
    loadEvent.SetId( wxID_ANY );

    frame->OnLoadProject( loadEvent );

    wxString title = GetTitle() + wxT( " " ) + GetBuildVersion() +
                     wxT( " " ) + frame->m_ProjectFileName.GetFullPath();

    if( !fn.IsDirWritable() )
        title += _( " [Read Only]" );

    frame->SetTitle( title );

    frame->m_LeftWin->ReCreateTreePrj();

    frame->PrintPrjInfo();
#endif
}


void PGM_KICAD::destroy()
{
    // unlike a normal destructor, this is designed to be called more
    // than once safely:

    m_bm.End();

    PGM_BASE::destroy();
}


/**
 * Class KIWAY_MGR
 * is a container for all KIWAYS [and PROJECTS].  This class needs to work both
 * for a C++ project manager and an a wxPython one (after being moved into a
 * header later).
 */
class KIWAY_MGR
{
public:
    //KIWAY_MGR();
    // ~KIWAY_MGR();

    bool OnStart( wxApp* aProcess );

    void OnEnd();

    KIWAY& operator[]( int aIndex )
    {
        wxASSERT( m_kiways.size() );    // stuffed in OnStart()
        return m_kiways[aIndex];
    }

private:

    // KIWAYs may not be moved once doled out, since window DNA depends on the
    // pointer being good forever.
    // boost_ptr::vector however never moves the object pointed to.
    typedef boost::ptr_vector<KIWAY>    KIWAYS;

    KIWAYS  m_kiways;
};

static KIWAY_MGR   kiways;


/**
 * Struct APP_KICAD
 * is not publicly visible because most of the action is in PGM_KICAD these days.
 */
struct APP_KICAD : public wxApp
{
    bool OnInit()           // overload wxApp virtual
    {
        if( kiways.OnStart( this ) )
        {
            return Pgm().OnPgmInit( this );
        }
        return false;
    }

    int  OnExit()           // overload wxApp virtual
    {
        kiways.OnEnd();

        Pgm().OnPgmExit();

        return wxApp::OnExit();
    }

    /**
     * Function MacOpenFile
     * is specific to MacOSX (not used under Linux or Windows).
     * MacOSX requires it for file association.
     * @see http://wiki.wxwidgets.org/WxMac-specific_topics
     */
    void MacOpenFile( const wxString& aFileName )   // overload wxApp virtual
    {
        Pgm().MacOpenFile( aFileName );
    }
};

IMPLEMENT_APP( APP_KICAD );


// The C++ project manager supports one open PROJECT, so Prj() calls within
// this link image need this function.
PROJECT& Prj()
{
    return kiways[0].Prj();
}


bool KIWAY_MGR::OnStart( wxApp* aProcess )
{
    // The C++ project manager supports only one open PROJECT
    // We should need no copy constructor for KIWAY to push a pointer.
    m_kiways.push_back( new KIWAY() );

    return true;
}


void KIWAY_MGR::OnEnd()
{
}
