
#include <wx/stdpaths.h>

#include <common.h>
#include <search_stack.h>
#include <pgm_base.h>


// put your best guesses in here, send the computer on a wild goose chase, its
// got nothing else to do.

void SystemDirsAppend( SEARCH_STACK* aSearchStack )
{
    // No clearing is done here, the most general approach is NOT to assume that
    // our appends will be the only thing in the stack.  This function has no
    // knowledge of caller's intentions.

    // wxPathList::AddEnvList() is broken, use SEARCH_STACK::AddPaths().
    // SEARCH_STACK::AddPaths() will verify readability and existence of
    // each directory before adding.
    SEARCH_STACK maybe;

    // User environment variable path is the first search path.  Chances are
    // if the user is savvy enough to set an environment variable they know
    // what they are doing.  It should take precedence over anything else.
    // Otherwise don't set it.
    maybe.AddPaths( wxGetenv( wxT( "KICAD" ) ) );

#ifndef __WXMAC__
    // This is from CMAKE_INSTALL_PREFIX.
    // Useful when KiCad is installed by `make install`.
    // Use as second ranked place.
    maybe.AddPaths( wxT( DEFAULT_INSTALL_PATH ) );
#endif

    // Add the directory for the user-dependent, program specific, data files:
    // Unix: ~/.appname
    // Windows: C:\Documents and Settings\username\Application Data\appname
    // Mac: ~/Library/Application Support/appname
    maybe.AddPaths( wxStandardPaths::Get().GetUserDataDir() );

#ifdef __WXMAC__
    // global machine specific application data
    maybe.AddPaths( wxT( "/Library/Application Support/kicad" ) );

    // Dir of the global (not user-specific), application specific, data files.
    // From wx docs:
    // Unix: prefix/share/appname
    // Windows: the directory where the executable file is located
    // Mac: appname.app/Contents/SharedSupport bundle subdirectory
    maybe.AddPaths( wxStandardPaths::Get().GetDataDir() );
#else
    {
        // Should be full path to this program executable.
        wxString   bin_dir = Pgm().GetExecutablePath();

#if defined(__MINGW32__)
        // bin_dir uses unix path separator.  So to parse with wxFileName
        // use windows separator, especially important for server inclusion:
        // like: \\myserver\local_path .
        bin_dir.Replace( wxFileName::GetPathSeparator( wxPATH_UNIX ),
                         wxFileName::GetPathSeparator( wxPATH_WIN ) );
#endif

        wxFileName bin_fn( bin_dir, wxEmptyString );

        // Dir of the global (not user-specific), application specific, data files.
        // From wx docs:
        // Unix: prefix/share/appname
        // Windows: the directory where the executable file is located
        // Mac: appname.app/Contents/SharedSupport bundle subdirectory
        wxString data_dir = wxStandardPaths::Get().GetDataDir();

        if( bin_fn.GetPath() != data_dir )
        {
            // add data_dir if it is different from the bin_dir
            maybe.AddPaths( data_dir );
        }

        // Up one level relative to binary path with "share" appended below.
        bin_fn.RemoveLastDir();
        maybe.AddPaths( bin_fn.GetPath() );
    }

    /* The normal OS program file install paths allow for a binary to be
     * installed in a different path from the library files.  This is
     * useful for development purposes so the library and documentation
     * files do not need to be installed separately.  If someone can
     * figure out a way to implement this without #ifdef, please do.
     */
#if defined(__MINGW32__)
    maybe.AddPaths( wxGetenv( wxT( "PROGRAMFILES" ) ) );
#else
    maybe.AddPaths( wxGetenv( wxT( "PATH" ) ) );
#endif
#endif

#if defined(DEBUG) && 0
    maybe.Show( "maybe wish list" );
#endif

    // Append 1) kicad, 2) kicad/share, 3) share, and 4) share/kicad to each
    // possible base path in 'maybe'. Since SEARCH_STACK::AddPaths() will verify
    // readability and existence of each directory, not all of these will be
    // actually appended.
    for( unsigned i = 0; i < maybe.GetCount();  ++i )
    {
        wxFileName fn( maybe[i], wxEmptyString );

#ifndef __WXMAC__
        if( fn.GetPath().AfterLast( fn.GetPathSeparator() ) == wxT( "bin" ) )
        {
            fn.RemoveLastDir();

            if( !fn.GetDirCount() )
                continue;               // at least on linux
        }
#endif

        aSearchStack->AddPaths( fn.GetPath() );

#ifndef __WXMAC__
        fn.AppendDir( wxT( "kicad" ) );
        aSearchStack->AddPaths( fn.GetPath() );     // add maybe[i]/kicad

        fn.AppendDir( wxT( "share" ) );
        aSearchStack->AddPaths( fn.GetPath() );     // add maybe[i]/kicad/share

        fn.RemoveLastDir();                         // ../  clear share
        fn.RemoveLastDir();                         // ../  clear kicad

        fn.AppendDir( wxT( "share" ) );
        aSearchStack->AddPaths( fn.GetPath() );     // add maybe[i]/share

        fn.AppendDir( wxT( "kicad" ) );
        aSearchStack->AddPaths( fn.GetPath() );     // add maybe[i]/share/kicad
#endif
    }

#if defined(DEBUG) && 0
    // final results:
    aSearchStack->Show( __func__ );
#endif
}
