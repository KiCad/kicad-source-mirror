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


#if !wxCHECK_VERSION( 3, 0, 0 )

// implement missing wx2.8 function until >= wx3.0 pervades.
static wxString wxJoin(const wxArrayString& arr, const wxChar sep,
                const wxChar escape = '\\')
{
    size_t count = arr.size();
    if ( count == 0 )
        return wxEmptyString;

    wxString str;

    // pre-allocate memory using the estimation of the average length of the
    // strings in the given array: this is very imprecise, of course, but
    // better than nothing
    str.reserve(count*(arr[0].length() + arr[count-1].length()) / 2);

    if ( escape == wxT('\0') )
    {
        // escaping is disabled:
        for ( size_t i = 0; i < count; i++ )
        {
            if ( i )
                str += sep;
            str += arr[i];
        }
    }
    else // use escape character
    {
        for ( size_t n = 0; n < count; n++ )
        {
            if ( n )
                str += sep;

            for ( wxString::const_iterator i = arr[n].begin(),
                                         end = arr[n].end();
                  i != end;
                  ++i )
            {
                const wxChar ch = *i;
                if ( ch == sep )
                    str += escape;      // escape this separator
                str += ch;
            }
        }
    }

    str.Shrink(); // release extra memory if we allocated too much
    return str;
}
#endif


/// Put aPriorityPath in front of all paths in the value of aEnvVar.
const wxString PrePendPath( const wxString& aEnvVar, const wxString& aPriorityPath )
{
    wxPathList  paths;

    paths.AddEnvList( aEnvVar );
    paths.Insert( aPriorityPath, 0 );

    return wxJoin( paths, wxPATH_SEP[0] );
}


/// Extend LIB_ENV_VAR list with the directory from which I came, prepending it.
void SetLibEnvVar( const wxString& aAbsoluteArgv0 )
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

// POLICY CHOICE 1: return the full path of the DSO to load from single_top.
static const wxString dso_full_path( const wxString& aAbsoluteArgv0 )
{
    // Prefix basename with ${KIFACE_PREFIX} and change extension to ${KIFACE_SUFFIX}

    // POLICY CHOICE 1: Keep same path, and therefore installer must put the kiface DSO
    // in same dir as top process module.  Obviously alternatives are possible
    // and that is why this is a separate function.  One alternative would be to use
    // a portion of CMAKE_INSTALL_PREFIX and navigate to a "lib" dir, but that
    // would require a recompile any time you chose to install into a different place.

    // It is my decision to treat _eeschema.kiface and _pcbnew.kiface as "executables",
    // not "libraries" in this regard, since most all program functionality lives
    // in them. They are basically spin-offs from what was once a top process module.
    // That may not make linux package maintainers happy, but that is not my job.
    // Get over it.  KiCad is not a trivial suite, and multiple platforms come
    // into play, not merely linux.  For starters they will use extension ".kiface",
    // but later in time morph to ".so".  They are not purely libraries, else they
    // would begin with "lib" in basename.  Like I said, get over it, we're serving
    // too many masters here: python, windows, linux, OSX, multiple versions of wx...

    wxFileName  fn( aAbsoluteArgv0 );
    wxString    basename( KIFACE_PREFIX );  // start with special prefix

    basename += fn.GetName();               // add argv[0]'s basename
    fn.SetName( basename );

    // Here a "suffix" == an extension with a preceding '.',
    // so skip the preceding '.' to get an extension
    fn.SetExt( KIFACE_SUFFIX + 1 );         // + 1 => &KIFACE_SUFFIX[1]

    return fn.GetFullPath();
}


// Only a single KIWAY is supported in this single_top top level component,
// which is dedicated to loading only a single DSO.
static KIWAY    kiway;


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
        return Pgm().OnPgmInit( this );
    }

    int  OnExit()           // overload wxApp virtual
    {
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
            wxLogError( wxT( "Unhandled exception class: %s  what: %s" ),
                GetChars( FROM_UTF8( typeid( ioe ).name() ) ),
                GetChars( ioe.errorText ) );
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

IMPLEMENT_APP( APP_SINGLE_TOP );


/**
 * Function get_kiface_getter
 * returns a KIFACE_GETTER_FUNC for the current process's main implementation
 * link image.
 *
 * @param aDSOName is an absolute full path to the DSO to load and find
 *  KIFACE_GETTER_FUNC within.
 *
 * @return KIFACE_GETTER_FUNC* - a pointer to a function which can be called to
 *  get the KIFACE or NULL if the getter func was not found.  If not found,
 *  it is possibly not version compatible since the lookup is done by name and
 *  the name contains the API version.
 */
static KIFACE_GETTER_FUNC* get_kiface_getter( const wxString& aDSOName )
{
#if defined(BUILD_KIWAY_DLL)

    // Remember single_top only knows about a single DSO.  Using an automatic
    // with a defeated destructor, see Detach() below, so that the DSO program
    // image stays in RAM until process termination, and specifically
    // beyond the point in time at which static destructors are run.  Otherwise
    // a static wxDynamicLibrary's destructor might create an out of sequence
    // problem.  This was never detected, so it's only a preventative strategy.
    wxDynamicLibrary dso;

    void*   addr = NULL;

    if( !dso.Load( aDSOName, wxDL_VERBATIM | wxDL_NOW ) )
    {
        // Failure: error reporting UI was done via wxLogSysError().
        // No further reporting required here.
    }

    else if( ( addr = dso.GetSymbol( wxT( KIFACE_INSTANCE_NAME_AND_VERSION ) ) ) == NULL )
    {
        // Failure: error reporting UI was done via wxLogSysError().
        // No further reporting required here.
    }

    // Tell dso's wxDynamicLibrary destructor not to Unload() the program image.
    (void) dso.Detach();

    return (KIFACE_GETTER_FUNC*) addr;

#else
    return &KIFACE_GETTER;

#endif
}


static KIFACE*  kiface;
static int      kiface_version;



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
    SetLibEnvVar( absoluteArgv0 );

    if( !initPgm() )
        return false;

    wxString dname = dso_full_path( absoluteArgv0 );

    // Get the getter.
    KIFACE_GETTER_FUNC* getter = get_kiface_getter( dname );

    if( !getter )
    {
        // get_kiface_getter() failed & already showed the UI message.
        // Return failure without any further UI.
        return false;
    }

    // Get the KIFACE.
    kiface = getter( &kiface_version, KIFACE_VERSION, this );

    // KIFACE_GETTER_FUNC function comment (API) says the non-NULL is unconditional.
    wxASSERT_MSG( kiface, wxT( "attempted DSO has a bug, failed to return a KIFACE*" ) );

    // Give the DSO a single chance to do its "process level" initialization.
    // "Process level" specifically means stay away from any projects in there.
    if( !kiface->OnKifaceStart( this ) )
        return false;

    // Use KIFACE to create a top window that the KIFACE knows about.
    // TOP_FRAME is passed on compiler command line from CMake, and is one of
    // the types in ID_DRAWFRAME_TYPE.
    // KIFACE::CreateWindow() is a virtual so we don't need to link to it.
    // Remember its in the *.kiface DSO.
#if 0
    // this pulls in EDA_DRAW_FRAME type info, which we don't want in
    // the single_top link image.
    KIWAY_PLAYER* frame = dynamic_cast<KIWAY_PLAYER*>( kiface->CreateWindow(
                                NULL, TOP_FRAME, &kiway, KFCTL_STANDALONE ) );
#else
    KIWAY_PLAYER* frame = (KIWAY_PLAYER*) kiface->CreateWindow(
                                NULL, TOP_FRAME, &kiway, KFCTL_STANDALONE );
#endif

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

            argSet[0] = argv1.GetFullPath();
#endif
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
    if( kiface )
        kiface->OnKifaceEnd();

    saveCommonSettings();

    // write common settings to disk, and destroy everything in PGM_BASE,
    // especially wxSingleInstanceCheckerImpl earlier than wxApp and earlier
    // than static destruction would.
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
