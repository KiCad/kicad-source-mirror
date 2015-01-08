/**
 * @file kicad.cpp
 * @brief Main KiCad Project manager file
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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


#include <macros.h>
#include <fctsys.h>
#include <wx/stdpaths.h>
#include <kicad.h>
#include <kiway.h>
#include <pgm_kicad.h>
#include <tree_project_frame.h>
#include <online_help.h>
#include <wildcards_and_files_ext.h>
#include <boost/ptr_container/ptr_vector.hpp>

#include <build_version.h>


/// Extend LIB_ENV_VAR list with the directory from which I came, prepending it.
static void set_lib_env_var( const wxString& aAbsoluteArgv0 )
{
    // POLICY CHOICE 2: Keep same path, so that installer MAY put the
    // "subsidiary DSOs" in the same directory as the kiway top process modules.
    // A subsidiary shared library is one that is not a top level DSO, but rather
    // some shared library that a top level DSO needs to even be loaded.  It is
    // a static link to a shared object from a top level DSO.

    // This directory POLICY CHOICE 2 is not the only dir in play, since LIB_ENV_VAR
    // has numerous path options in it, as does DSO searching on linux, windows, and OSX.
    // See "man ldconfig" on linux. What's being done here is for quick installs
    // into a non-standard place, and especially for Windows users who may not
    // know what the PATH environment variable is or how to set it.

    wxFileName  fn( aAbsoluteArgv0 );

    wxString    ld_path( LIB_ENV_VAR );
    wxString    my_path   = fn.GetPath();
    wxString    new_paths = PrePendPath( ld_path, my_path );

    wxSetEnv( ld_path, new_paths );

#if defined(DEBUG)
    {
        wxString    test;
        wxGetEnv( ld_path, &test );
        printf( "LIB_ENV_VAR:'%s'\n", TO_UTF8( test ) );
    }
#endif
}


// a dummy to quiet linking with EDA_BASE_FRAME::config();
#include <kiface_i.h>
KIFACE_I& Kiface()
{
    // This function should never be called.  It is only referenced from
    // EDA_BASE_FRAME::config() and this is only provided to satisfy the linker,
    // not to be actually called.
    wxLogFatalError( wxT( "Unexpected call to Kiface() in kicad/kicad.cpp" ) );

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

    wxString absoluteArgv0 = wxStandardPaths::Get().GetExecutablePath();

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogSysError( wxT( "No meaningful argv[0]" ) );
        return false;
    }

    // Set LIB_ENV_VAR *before* loading the KIFACE DSOs, in case they have hard
    // dependencies on subsidiary DSOs below it.
    set_lib_env_var( absoluteArgv0 );

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

    KICAD_MANAGER_FRAME* frame = new KICAD_MANAGER_FRAME( NULL, wxT( "KiCad" ),
                                     wxDefaultPosition, wxDefaultSize );
    App().SetTopWindow( frame );

    Kiway.SetTop( frame );

    bool prjloaded = false;    // true when the project is loaded

    if( App().argc > 1 )
        frame->SetProjectFileName( App().argv[1] );

    else if( GetFileHistory().GetCount() )
    {
        wxString last_pro = GetFileHistory().GetHistoryFile( 0 );

        if( !wxFileExists( last_pro ) )
        {
            GetFileHistory().RemoveFileFromHistory( 0 );

            wxFileName namelessProject( wxGetCwd(), NAMELESS_PROJECT,
                                        ProjectFileExtension );

            frame->SetProjectFileName( namelessProject.GetFullPath() );
        }
        else
        {
            // Try to open the last opened project,
            // if a project name is not given when starting Kicad
            frame->SetProjectFileName( last_pro );

            wxCommandEvent cmd( 0, wxID_FILE1 );

            frame->OnFileHistory( cmd );
            prjloaded = true;    // OnFileHistory() loads the project
        }
    }
    else	// there is no history
    {
            wxFileName namelessProject( wxGetCwd(), NAMELESS_PROJECT,
                                        ProjectFileExtension );

            frame->SetProjectFileName( namelessProject.GetFullPath() );
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
    Kiway.OnKiwayEnd();

    saveCommonSettings();

    // write common settings to disk, and destroy everything in PGM_KICAD,
    // especially wxSingleInstanceCheckerImpl earlier than wxApp and earlier
    // than static destruction would.
    destroy();
}


void PGM_KICAD::MacOpenFile( const wxString& aFileName )
{
#if defined(__WXMAC__)

    KICAD_MANAGER_FRAME* frame = (KICAD_MANAGER_FRAME*) App().GetTopWindow();

    frame->SetProjectFileName( aFileName );

    wxCommandEvent loadEvent( 0, wxID_ANY );

    frame->OnLoadProject( loadEvent );
#endif
}


void PGM_KICAD::destroy()
{
    // unlike a normal destructor, this is designed to be called more
    // than once safely:

    m_bm.End();

    PGM_BASE::destroy();
}


KIWAY  Kiway( &Pgm(), KFCTL_CPP_PROJECT_SUITE );


/**
 * Struct APP_KICAD
 * is not publicly visible because most of the action is in PGM_KICAD these days.
 */
struct APP_KICAD : public wxApp
{
    bool OnInit()           // overload wxApp virtual
    {
        // if( Kiways.OnStart( this ) )
        {
            return Pgm().OnPgmInit( this );
        }
        return false;
    }

    int  OnExit()           // overload wxApp virtual
    {
        // Kiways.OnEnd();

        Pgm().OnPgmExit();

        return wxApp::OnExit();
    }

    int OnRun()             // overload wxApp virtual
    {
        try
        {
            return wxApp::OnRun();
        }
        catch( const std::exception& e )
        {
            wxLogError( wxT( "Unhandled exception class: %s  what: %s" ),
                GetChars( FROM_UTF8( typeid(e).name() )),
                GetChars( FROM_UTF8( e.what() ) ) );;
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( GetChars( ioe.errorText ) );
        }
        catch(...)
        {
            wxLogError( wxT( "Unhandled exception of unknown type" ) );
        }

        return -1;
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
    return Kiway.Prj();
}


#if 0   // there can be only one in C++ project manager.

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

#endif
