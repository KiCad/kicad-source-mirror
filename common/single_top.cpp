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

    This is a program launcher for a single DSO. Initially it will only mimic a
    KIWAY not actually implement one, since only a single DSO is supported by
    it.

*/

#include <macros.h>
#include <fctsys.h>
#include <wx/dynlib.h>
#include <wx/filename.h>
#include <kiway.h>
#include <wx/stdpaths.h>


/**
 * Class PROCESS
 * provides its own OnInit() handler.
 */
class PROCESS : public wxApp
{
public:

    bool OnInit();
};


IMPLEMENT_APP( PROCESS )


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


// POLICY CHOICE: return the name of the DSO to load as single_top.
static const wxString dso_name( const wxString& aAbsoluteArgv0 )
{
    // Prefix basename with '_' and change extension to DSO_EXT.

    // POLICY CHOICE: Keep same path, and therefore installer must put the major DSO
    // in same dir as top process module.  Obviously alternatives are possible
    // and that is why this is a separate function.  One alternative would be to use
    // a portion of CMAKE_INSTALL_PREFIX and navigate to a "lib" dir, but that
    // would require a recompile any time you chose to install into a different place.

    // It is my decision to treat _eeschema.so and _pcbnew.so as "executables",
    // not "libraries" in this regard, since most all program functionality lives
    // in them. They are basically spin-offs from what was once a top process module.
    // That may not make linux package maintainers happy, but that is not my job.
    // Get over it.  KiCad is not a trivial suite, and multiple platforms come
    // into play, not merely linux.  If it freaks you out, we can use a different
    // file extension than ".so", but they are not purely libraries, else they
    // would begin with "lib" in basename.  Like I said, get over it, we're serving
    // too many masters here: python, windows, linux, OSX, multiple versions of wx...

    wxFileName  fn( aAbsoluteArgv0 );
    wxString    basename( wxT( '_' ) );

    basename += fn.GetName();
    fn.SetName( basename );
    fn.SetExt( DSO_EXT );

    return fn.GetFullPath();
}


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
    // POLICY CHOICE: Keep same path, so that installer MAY put the
    // "subsidiary shared libraries" in the same directory as the top process module.
    // A subsidiary shared library is one that is not a top level DSO, but rather
    // some shared library that a top level DSO needs to even be loaded.

    // This directory POLICY CHOICE is not the only dir in play, since LIB_ENV_VAR
    // has numerous path options in it, as does DSO searching on linux.
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


// Only a single KIWAY is supported in this single_top to level component,
// which is dedicated to loading only a single DSO.
static KIWAY    standalone;

// Use of this is arbitrary, remember single_top only knows about a single DSO.
// Could have used one from the KIWAY also.
static wxDynamicLibrary dso;


/**
 * Function get_kiface_getter
 * returns a KIFACE_GETTER_FUNC for the current process's main implemation link image.
 *
 * @param aDSOName is an absolute full path to the DSO to load and find KIFACE_GETTER_FUNC within.
 *
 * @return KIFACE_GETTER_FUNC* - a pointer to a function which can be called to get the KIFACE
 *   or NULL if not found or not version compatible.
 */
static KIFACE_GETTER_FUNC* get_kiface_getter( const wxString& aDSOName )
{
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

    return (KIFACE_GETTER_FUNC*) addr;
}


static KIFACE*  kiface;
static int      kiface_version;


bool PROCESS::OnInit()
{
    // Choose to use argv command line processing in base class's OnInit().
    // That choice is not mandatory, see wx's appbase.cpp OnInit().
    if( !wxApp::OnInit() )
        return false;

    wxStandardPathsBase& paths = wxStandardPaths::Get();

    wxString dir = paths.GetLocalizedResourcesDir( wxT( "de" ),
                        wxStandardPaths::ResourceCat_None );

    printf( "LocalizeResourcesDir:'%s'\n",  TO_UTF8( dir ) );

    wxString dummy( _( "translate this" ) );

    wxString absoluteArgv0 = paths.GetExecutablePath();

#if 0 || defined(DEBUG)
    printf( "argv[0]:'%s' absoluteArgv0:'%s'\n",
        TO_UTF8( wxString( argv[0] ) ),
        TO_UTF8( absoluteArgv0 )
        );
#endif

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogSysError( wxT( "No meaningful argv[0]" ) );
        return false;
    }

    // Set LIB_ENV_VAR *before* loading the DSO, in case the module holding the
    // KIFACE has hard dependencies on subsidiary DSOs below it, *before* loading
    // the KIFACE.
    SetLibEnvVar( absoluteArgv0 );

    wxString dname = dso_name( absoluteArgv0 );

    // Get the getter.
    KIFACE_GETTER_FUNC* getter = get_kiface_getter( dname );

    // get_kiface_getter() returned NULL? If so it handled the UI message, so
    // we can fail without any further UI.
    if( !getter )
        return false;

    // Get the KIFACE, and give the DSO a single chance to do its
    // "process level" initialization.
    kiface = getter( &kiface_version, KIFACE_VERSION, &wxGetApp() );

    if( !kiface )
    {
        // get_kiface_getter() did its own UI error window, because it called
        // functions in wxDynamicLibrary which did so using wxLogSysError().
        // Therefore say nothing on failure, it's already been said.
        // Return false telling startup code to fail the program immediately.
        return false;
    }

    // Using the KIFACE, create a window that the KIFACE knows about,
    // pass classId=0 for now.  This uses a virtual function KIFACE::CreateWindow()
    // so we don't need to link to it.
    wxFrame* frame = (wxFrame*) kiface->CreateWindow( 0, &standalone );

    SetTopWindow( frame );

    frame->Centre();

    frame->Show();

    return true;
}

