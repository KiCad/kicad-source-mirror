
#include <pgm_base.h>
#include <common.h>



/**
 * Function FindFileInSearchPaths
 * looks in "this" for \a aFilename, but first modifies every search
 * path by appending a list of path fragments from aSubdirs.  That modification
 * is not rentative.
 */
wxString FindFileInSearchPaths( const SEARCH_STACK& aStack,
        const wxString& aFilename, const wxArrayString* aSubdirs )
{
    wxPathList paths;

    for( unsigned i = 0; i < aStack.GetCount(); ++i )
    {
        wxFileName fn( aStack[i], wxEmptyString );

        if( aSubdirs )
        {
            for( unsigned j = 0; j < aSubdirs->GetCount(); j++ )
                fn.AppendDir( (*aSubdirs)[j] );
        }

        if( fn.DirExists() )
        {
            paths.Add( fn.GetPath() );
        }
    }

    return paths.FindValidPath( aFilename );
}


// See also FindKicadHelpPath.cpp.notused.
wxString SearchHelpFileFullPath( const SEARCH_STACK& aSStack, const wxString& aBaseName )
{
    wxArrayString   subdirs;
    wxArrayString   altsubdirs;
    SEARCH_STACK    ss = aSStack;

    // It might already be in aSStack, but why depend on other code
    // far away when it's so easy to add it again (to our copy) as the first place to look.
    // This is CMAKE_INSTALL_PREFIX:
    ss.AddPaths( wxT( DEFAULT_INSTALL_PATH ), 0 );

    // If there's a KICAD environment variable set, use that guy's path also
    ss.AddPaths( Pgm().GetKicadEnvVariable(), 0 );

#if 1 // && defined(__linux__)
    // Based on kicad-doc.bzr/CMakeLists.txt, line 20, the help files are
    // installed into "<CMAKE_INSTALL_PREFIX>/share/doc/kicad/help" for linux.
    // This is ${KICAD_HELP} var in that CMakeLists.txt file.
    // Below we account for an international subdirectory.
    subdirs.Add( wxT( "share" ) );
    subdirs.Add( wxT( "doc" ) );
    subdirs.Add( wxT( "kicad" ) );
    subdirs.Add( wxT( "help" ) );
#endif

#if 1 // && defined(__WINDOWS__)
    // Based on kicad-doc.bzr/CMakeLists.txt, line 35, the help files are
    // installed into "<CMAKE_INSTALL_PREFIX>/doc/help" for Windows.
    // This is ${KICAD_HELP} var in that CMakeLists.txt file.
    // Below we account for an international subdirectory.
    altsubdirs.Add( wxT( "doc" ) );
    altsubdirs.Add( wxT( "help" ) );
#endif

    /* Search for a help file.
     *  we *must* find a help file.
     *  so help is searched in directories in this order:
     *  help/<canonical name> like help/en_GB
     *  help/<short name> like help/en
     *  help/en
     */

    wxLocale* i18n = Pgm().GetLocale();

    // We try to find help file in help/<canonical name>
    // If fails, try to find help file in help/<short canonical name>
    // If fails, try to find help file in help/en
    wxArrayString locale_name_dirs;
    locale_name_dirs.Add( i18n->GetCanonicalName() );           // canonical name like fr_FR
    // wxLocale::GetName() does not return always the short name
    locale_name_dirs.Add( i18n->GetName().BeforeLast( '_' ) );  // short canonical name like fr
    locale_name_dirs.Add( wxT("en") );                          // default (en)

#if defined(DEBUG) && 0
    ss.Show( __func__ );
    printf( "%s: m_help_file:'%s'\n", __func__, TO_UTF8( aBaseName ) );
#endif

    // Help files can be html (.html ext) or pdf (.pdf ext) files.
    // Therefore, <BaseName>.html file is searched and if not found,
    // <BaseName>.pdf file is searched in the same paths
    wxString fn;

    for( unsigned ii = 0; ii < locale_name_dirs.GetCount(); ii++ )
    {
        subdirs.Add( locale_name_dirs[ii] );
        altsubdirs.Add( locale_name_dirs[ii] );

        fn = FindFileInSearchPaths( ss, aBaseName + wxT(".html"), &altsubdirs );

        if( !fn.IsEmpty() )
            break;

        fn = FindFileInSearchPaths( ss, aBaseName + wxT(".pdf"), &altsubdirs );

        if( !fn.IsEmpty() )
            break;

        fn = FindFileInSearchPaths( ss, aBaseName + wxT(".html"), &subdirs );

        if( !fn.IsEmpty() )
            break;

        fn = FindFileInSearchPaths( ss, aBaseName + wxT(".pdf"), &subdirs );

        if( !fn.IsEmpty() )
            break;

        subdirs.RemoveAt( subdirs.GetCount() - 1 );
        altsubdirs.RemoveAt( altsubdirs.GetCount() - 1 );
    }

    return fn;
}
