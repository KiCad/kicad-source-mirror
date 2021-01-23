#include <wx/filename.h>
#include <wx/string.h>

#include <kiplatform/environment.h>
#include <paths.h>
#include <settings/settings_manager.h>

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