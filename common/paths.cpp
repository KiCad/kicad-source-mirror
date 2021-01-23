#include <wx/filename.h>
#include <wx/string.h>

#include <kiplatform/environment.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>


wxString PATHS::GetUserPluginsPath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetDocumentsDir() );
    tmp.AppendDir( "kicad" );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
    tmp.AppendDir( "plugins" );

    return tmp.GetFullPath();
}


wxString PATHS::GetUserPlugins3DPath()
{
    wxFileName tmp;

    tmp.AssignDir( PATHS::GetUserPluginsPath() );
    tmp.AppendDir( "3d" );

    return tmp.GetFullPath();
}


wxString PATHS::GetUserScriptingPath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetDocumentsDir() );
    tmp.AppendDir( "kicad" );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
    tmp.AppendDir( "scripting" );

    return tmp.GetFullPath();
}


wxString PATHS::GetUserTemplatesPath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetDocumentsDir() );
    tmp.AppendDir( "kicad" );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
    tmp.AppendDir( "templates" );

    return tmp.GetFullPath();
}


wxString PATHS::GetDefaultUserProjectsPath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetDocumentsDir() );
    tmp.AppendDir( "kicad" );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
    tmp.AppendDir( "projects" );

    return tmp.GetFullPath();
}


wxString PATHS::GetStockScriptingPath()
{
    wxString path;

    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        // Allow debugging from build dir by placing a "scripting" folder in the build root
        path = Pgm().GetExecutablePath() + wxT( "../scripting" );
    }
    else
    {
        //TODO(snh) break out the directory functions into KIPLATFORM
#if defined( __WXMAC__ )
        path = GetOSXKicadDataDir() + wxT( "/scripting" );
#elif defined( __WXMSW__ )
        path = Pgm().GetExecutablePath() + wxT( "../share/kicad/scripting" );
#else
        path = wxString( KICAD_DATA ) + wxS( "/scripting" );
#endif
    }

    return path;
}


wxString PATHS::GetStockPluginsPath()
{
    wxFileName fn;

#if defined( __WXMAC__ )
    fn.Assign( Pgm().GetExecutablePath() );
    fn.AppendDir( wxT( "Contents" ) );
    fn.AppendDir( wxT( "PlugIns" ) );
#elif defined( __WXMSW__ )
    fn.Assign( Pgm().GetExecutablePath() + wxT( "../plugins/" ) );
#else
    // PLUGINDIR = CMAKE_INSTALL_FULL_LIBDIR path is the absolute path
    // corresponding to the install path used for constructing KICAD_USER_PLUGIN
    wxString tfname = wxString::FromUTF8Unchecked( PLUGINDIR );
    fn.Assign( tfname, "" );
    fn.AppendDir( "kicad" );
    fn.AppendDir( wxT( "plugins" ) );
#endif

    return fn.GetPathWithSep();
}


wxString PATHS::GetStockPlugins3DPath()
{
    wxFileName fn;

    fn.Assign( PATHS::GetStockPluginsPath() );
    fn.AppendDir( "3d" );

    return fn.GetPathWithSep();
}