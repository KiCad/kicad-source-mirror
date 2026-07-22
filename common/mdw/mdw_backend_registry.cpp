/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <mdw/mdw_backend_registry.h>
#include <mdw/mdw_proto_convert.h>

#include <env_vars.h>
#include <paths.h>
#include <pgm_base.h>
#include <trace_helpers.h>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>

#include <nlohmann/json.hpp>

#include <api/workspace/workspace_types.pb.h>

#include <algorithm>
#include <fstream>

namespace
{
/// False for a manifest missing a required field
bool parseManifest( const wxString& aFile, MDW_BACKEND_MANIFEST& aManifest )
{
    wxFileName jsonPath( aFile );
    std::ifstream stream( jsonPath.GetFullPath().fn_str() );

    if( !stream.is_open() )
        return false;

    nlohmann::json root = nlohmann::json::parse( stream );

    std::string id = root.value( "id", std::string() );
    std::string executable = root.value( "executable", std::string() );

    if( id.empty() || executable.empty() )
        return false;

    aManifest.backend_id = wxString::FromUTF8( id );
    aManifest.display_name = wxString::FromUTF8( root.value( "name", std::string() ) );

    wxFileName exePath( wxString::FromUTF8( executable ) );
    exePath.MakeAbsolute( jsonPath.GetPath() );

#ifdef __WXMSW__
    exePath.SetExt( wxT( "exe" ) );
#endif

    aManifest.executable_path = exePath.GetFullPath();

    // A stock manifest names a backend installed beside the KiCad executables
    if( !exePath.FileExists() && executable.find_first_of( "/\\" ) == std::string::npos )
    {
        wxString sibling = PATHS::ResolveSiblingExecutable( wxString::FromUTF8( executable ) );

        if( !sibling.IsEmpty() )
            aManifest.executable_path = sibling;
    }

    if( root.contains( "capability_hints" ) && root["capability_hints"].is_array() )
    {
        for( const nlohmann::json& hint : root["capability_hints"] )
        {
            kiapi::workspace::CapabilityGroup group = kiapi::workspace::CG_UNKNOWN;

            if( hint.is_string() && kiapi::workspace::CapabilityGroup_Parse( "CG_" + hint.get<std::string>(), &group )
                && group != kiapi::workspace::CG_UNKNOWN )
            {
                aManifest.capability_hints.insert( MdwFromProto( group ) );
            }
        }
    }

    // An unknown value is left as none
    if( root.value( "binding_source", std::string() ) == "git_remote" )
        aManifest.binding_source = MDW_BINDING_SOURCE::GIT_REMOTE;

    aManifest.binding_search = root.value( "binding_search", false );
    aManifest.binding_target_param = wxString::FromUTF8( root.value( "binding_target_param", std::string() ) );

    if( root.contains( "binding_fields" ) && root["binding_fields"].is_array() )
    {
        std::vector<MDW_BINDING_FIELD> fields;

        for( const nlohmann::json& entry : root["binding_fields"] )
        {
            std::string fieldId = entry.is_object() ? entry.value( "id", std::string() ) : std::string();

            // A field with no binding member cannot be stored, so it is left out
            if( fieldId != "workspace_id" && fieldId != "binding" )
                continue;

            fields.push_back( { wxString::FromUTF8( fieldId ),
                                wxString::FromUTF8( entry.value( "label", std::string() ) ),
                                wxString::FromUTF8( entry.value( "hint", std::string() ) ) } );
        }

        aManifest.binding_fields = std::move( fields );
    }

    return true;
}


bool readManifest( const wxString& aFile, MDW_BACKEND_MANIFEST& aManifest )
{
    try
    {
        if( !parseManifest( aFile, aManifest ) )
        {
            wxLogTrace( traceMdw, wxT( "MDW manifest %s missing required fields" ), aFile );
            return false;
        }
    }
    catch( const nlohmann::json::exception& e )
    {
        wxLogTrace( traceMdw, wxT( "MDW manifest %s failed to parse: %s" ), aFile, wxString::FromUTF8( e.what() ) );
        return false;
    }

    // A backend offered without its executable would only fail at launch
    if( !wxFileName::FileExists( aManifest.executable_path ) )
    {
        wxLogTrace( traceMdw, wxT( "MDW manifest %s names missing executable %s" ), aFile, aManifest.executable_path );
        return false;
    }

    return true;
}


/// The names in aDir matching aSpec and aFlags, sorted so an id clash has a stable winner
std::vector<wxString> sortedNames( wxDir& aDir, const wxString& aSpec, int aFlags )
{
    std::vector<wxString> names;
    wxString              name;

    for( bool more = aDir.GetFirst( &name, aSpec, aFlags ); more; more = aDir.GetNext( &name ) )
        names.push_back( name );

    std::sort( names.begin(), names.end() );
    return names;
}


/// The folder PCM extracts plugin packages into, one subfolder per package
wxString pcmPluginsPath()
{
    const ENV_VAR_MAP& env = Pgm().GetLocalEnvVariables();
    wxString           path = PATHS::GetDefault3rdPartyPath();

    if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( env, wxT( "3RD_PARTY" ) ) )
        path = *v;

    return wxFileName( path, wxT( "plugins" ) ).GetFullPath();
}
} // namespace


MDW_BACKEND_REGISTRY::MDW_BACKEND_REGISTRY()
{
}


void MDW_BACKEND_REGISTRY::Discover()
{
    std::vector<wxString> dirs;

    // A build tree runs the backends it built, before any installed ones
    if( PATHS::RunningFromBuildDir() )
        dirs.push_back( PATHS::GetStockDataPath() + wxT( "/plugins/mdw" ) );

    dirs.push_back( PATHS::GetStockPluginsPath() + wxT( "/mdw" ) );

    // Backends ship in-tree or as PCM packages, and a stock backend wins an id clash with a package
    dirs.push_back( pcmPluginsPath() );

    Discover( dirs );
}


void MDW_BACKEND_REGISTRY::Discover( const std::vector<wxString>& aDirs )
{
    m_manifests.clear();

    for( const wxString& dir : aDirs )
        scanDirectory( dir, true );
}


const MDW_BACKEND_MANIFEST* MDW_BACKEND_REGISTRY::Find( const wxString& aBackendId ) const
{
    for( const MDW_BACKEND_MANIFEST& manifest : m_manifests )
    {
        if( manifest.backend_id == aBackendId )
            return &manifest;
    }

    return nullptr;
}


void MDW_BACKEND_REGISTRY::scanDirectory( const wxString& aDir, bool aWithFolders )
{
    if( !wxDir::Exists( aDir ) )
    {
        wxLogTrace( traceMdw, wxT( "MDW registry skipping missing directory %s" ), aDir );
        return;
    }

    wxDir dir( aDir );

    if( !dir.IsOpened() )
    {
        wxLogTrace( traceMdw, wxT( "MDW registry cannot open directory %s" ), aDir );
        return;
    }

    for( const wxString& filename : sortedNames( dir, wxT( "*.kicad-mdw.json" ), wxDIR_FILES ) )
    {
        wxFileName           manifestFile( aDir, filename );
        MDW_BACKEND_MANIFEST manifest;

        if( !readManifest( manifestFile.GetFullPath(), manifest ) )
            continue;

        // Earlier directories win an id clash
        if( const MDW_BACKEND_MANIFEST* winner = Find( manifest.backend_id ) )
        {
            wxLogTrace( traceMdw, wxT( "MDW manifest %s shadowed by %s" ), manifestFile.GetFullPath(),
                        winner->executable_path );
            continue;
        }

        m_manifests.push_back( std::move( manifest ) );
    }

    if( !aWithFolders )
        return;

    // A backend dropped in as a folder keeps its manifest beside its executable
    for( const wxString& folder : sortedNames( dir, wxEmptyString, wxDIR_DIRS ) )
        scanDirectory( wxFileName( aDir, folder ).GetFullPath(), false );
}
