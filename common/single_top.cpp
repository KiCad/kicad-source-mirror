/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

    This is a program launcher for a single KIFACE DSO. It only mimics a KIWAY,
    not actually implements one, since only a single DSO is supported by it.

    It is compiled multiple times, once for each standalone program and as such
    gets different compiler command line supplied #defines from CMake.

*/


#include <typeinfo>
#include <macros.h>
#include <fctsys.h>
#include <wx/dynlib.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/snglinst.h>

#include <kiway.h>
#include <pgm_base.h>
#include <kiway_player.h>
#include <confirm.h>


// The functions we use will cause the program launcher to pull stuff in
// during linkage, keep the map file in mind to see what's going into it.

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


// Only a single KIWAY is supported in this single_top top level component,
// which is dedicated to loading only a single DSO.
KIWAY    Kiway( &Pgm(), KFCTL_STANDALONE );


// implement a PGM_BASE and a wxApp side by side:

/**
 * Struct PGM_SINGLE_TOP
 * implements PGM_BASE with its own OnPgmInit() and OnPgmExit().
 */
static struct PGM_SINGLE_TOP : public PGM_BASE
{
    bool OnPgmInit( wxApp* aWxApp );                    // overload PGM_BASE virtual
    void OnPgmExit();                                   // overload PGM_BASE virtual
    void MacOpenFile( const wxString& aFileName );      // overload PGM_BASE virtual
} program;


PGM_BASE& Pgm()
{
    return program;
}


/**
 * Struct APP_SINGLE_TOP
 * implements a bare naked wxApp (so that we don't become dependent on
 * functionality in a wxApp derivative that we cannot deliver under wxPython).
 */
struct APP_SINGLE_TOP : public wxApp
{
    bool OnInit()           // overload wxApp virtual
    {
        try
        {
            return Pgm().OnPgmInit( this );
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

        Pgm().OnPgmExit();

        return false;
    }

    int  OnExit()           // overload wxApp virtual
    {
        return wxApp::OnExit();
    }

    int OnRun()             // overload wxApp virtual
    {
        int ret = -1;

        try
        {
            ret = wxApp::OnRun();
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

        Pgm().OnPgmExit();

        return ret;
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

IMPLEMENT_APP( APP_SINGLE_TOP );


bool PGM_SINGLE_TOP::OnPgmInit( wxApp* aWxApp )
{
    // first thing: set m_wx_app
    m_wx_app = aWxApp;

    wxString absoluteArgv0 = wxStandardPaths::Get().GetExecutablePath();

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogSysError( wxT( "No meaningful argv[0]" ) );
        return false;
    }

    // Set LIB_ENV_VAR *before* loading the DSO, in case the top-level DSO holding the
    // KIFACE has hard dependencies on subsidiary DSOs below it.
    set_lib_env_var( absoluteArgv0 );

    if( !initPgm() )
        return false;

#if !defined(BUILD_KIWAY_DLL)
    // Get the getter, it is statically linked into this binary image.
    KIFACE_GETTER_FUNC* getter = &KIFACE_GETTER;

    int  kiface_version;

    // Get the KIFACE.
    KIFACE* kiface = getter( &kiface_version, KIFACE_VERSION, this );

    // Trick the KIWAY into thinking it loaded a KIFACE, by recording the KIFACE
    // in the KIWAY.  It needs to be there for KIWAY::OnKiwayEnd() anyways.
    Kiway.set_kiface( KIWAY::KifaceType( TOP_FRAME ), kiface );
#endif

    // Use KIWAY to create a top window, which registers its existence also.
    // "TOP_FRAME" is a macro that is passed on compiler command line from CMake,
    // and is one of the types in FRAME_T.
    KIWAY_PLAYER* frame = Kiway.Player( TOP_FRAME, true );

    Kiway.SetTop( frame );

    App().SetTopWindow( frame );      // wxApp gets a face.

    // Open project or file specified on the command line:
    int argc = App().argc;

    if( argc > 1 )
    {
        /*
            gerbview handles multiple project data files, i.e. gerber files on
            cmd line. Others currently do not, they handle only one. For common
            code simplicity we simply pass all the arguments in however, each
            program module can do with them what they want, ignore, complain
            whatever.  We don't establish policy here, as this is a multi-purpose
            launcher.
        */

        std::vector<wxString>   argSet;

        for( int i=1;  i<argc;  ++i )
        {
            argSet.push_back( App().argv[i] );
        }

        // special attention to the first argument: argv[1] (==argSet[0])
        wxFileName argv1( argSet[0] );

        if( argc == 2 )
        {
#if defined(PGM_DATA_FILE_EXT)
            // PGM_DATA_FILE_EXT, if present, may be different for each compile,
            // it may come from CMake on the compiler command line, but often does not.
            // This facillity is mostly useful for those program modules
            // supporting a single argv[1].
            if( !argv1.GetExt() )
                argv1.SetExt( wxT( PGM_DATA_FILE_EXT ) );

#endif
            argv1.MakeAbsolute();

            argSet[0] = argv1.GetFullPath();

            if( !Pgm().LockFile( argSet[0] ) )
            {
                wxLogSysError( _( "This file is already open." ) );
                return false;
            }
        }

        // @todo: setting CWD is taboo in a multi-project environment, this
        // will not be possible soon.
        if( argv1.GetPath().size() )   // path only
        {
            // wxSetWorkingDirectory() does not like empty paths
            wxSetWorkingDirectory( argv1.GetPath() );
        }

        // Use the KIWAY_PLAYER::OpenProjectFiles() API function:
        if( !frame->OpenProjectFiles( argSet ) )
        {
            // OpenProjectFiles() API asks that it report failure to the UI.
            // Nothing further to say here.

            // Fail the process startup if the file could not be opened,
            // although this is an optional choice, one that can be reversed
            // also in the KIFACE specific OpenProjectFiles() return value.
            return false;
        }
    }
    else
    {
        /*

            The lean single_top program launcher has no access to program
            settings, for if it did, it would not be lean. That kind of
            functionality is in the KIFACE now, but it cannot assume that it is
            the only KIFACE in memory. So this looks like a dead concept here,
            or an expensive one in terms of code size.

        wxString dir;

        if( m_pgmSettings->Read( workingDirKey, &dir ) && wxDirExists( dir ) )
        {
            wxSetWorkingDirectory( dir );
        }
        */
    }

    frame->Show();

    return true;
}


void PGM_SINGLE_TOP::OnPgmExit()
{
    Kiway.OnKiwayEnd();

    saveCommonSettings();

    // Destroy everything in PGM_BASE, especially wxSingleInstanceCheckerImpl
    // earlier than wxApp and earlier than static destruction would.
    PGM_BASE::destroy();
}


void PGM_SINGLE_TOP::MacOpenFile( const wxString& aFileName )
{
    wxFileName  filename( aFileName );

    if( filename.FileExists() )
    {
#if 0
        // this pulls in EDA_DRAW_FRAME type info, which we don't want in
        // the single_top link image.
        KIWAY_PLAYER* frame = dynamic_cast<KIWAY_PLAYER*>( App().GetTopWindow() );
#else
        KIWAY_PLAYER* frame = (KIWAY_PLAYER*) App().GetTopWindow();
#endif
        if( frame )
            frame->OpenProjectFiles( std::vector<wxString>( 1, aFileName ) );
    }
}
