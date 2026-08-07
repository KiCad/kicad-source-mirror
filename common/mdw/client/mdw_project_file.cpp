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

#include <mdw/client/mdw_project_file.h>
#include <mdw/client/mdw_lifecycle.h>

#include <gestfich.h>
#include <kiplatform/io.h>
#include <nlohmann/json.hpp>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>

#include <wx/filename.h>

#include <fstream>
#include <sstream>


namespace
{

bool warnsBeforeSave( MDW_LIFECYCLE_CATEGORY aCategory, const MDW_LOCK_INFO& aLock )
{
    return ( aLock.is_locked && !aLock.locked_by_me ) || MdwCategoryBlocksWrites( aCategory );
}


bool isProjectFile( const PROJECT& aProject, const wxString& aLocalPath )
{
    return wxFileName( aLocalPath ).SameAs( wxFileName( aProject.GetProjectFullName() ) );
}


/// Write the in-memory binding into a project file the backend just wrote, leaving every other setting as it came
bool writeWorkspaceSection( PROJECT& aProject )
{
    PROJECT_FILE& file = aProject.GetProjectFile();
    file.Store();

    std::optional<nlohmann::json> workspace = file.GetJson( "workspace" );
    nlohmann::json                root;

    {
        std::ifstream in( ToFsPath( aProject.GetProjectFullName() ), std::ios::binary );
        root = nlohmann::json::parse( in, nullptr, false, true );
    }

    if( root.is_discarded() || !root.is_object() )
        return false;

    const nlohmann::json before = root;
    nlohmann::json&      target = root["workspace"];

    if( !target.is_object() )
        target = nlohmann::json::object();

    // Library sources and pins are shared settings the file may have just brought down
    for( const char* key : { "backend", "workspace_id", "binding" } )
    {
        if( workspace && workspace->contains( key ) )
            target[key] = ( *workspace )[key];
        else
            target.erase( key );
    }

    if( target.empty() && !before.contains( "workspace" ) )
        root.erase( "workspace" );

    // Left byte for byte as the workspace holds it, so a design checked out whole still matches its revision
    if( root == before )
        return true;

    // Items and bases describe one copy and live in its local settings; ones left here would be a teammate's
    if( root.contains( "workspace" ) )
    {
        root["workspace"].erase( "items" );
        root["workspace"].erase( "bases" );
    }

    std::string payload = root.dump( 2 ) + '\n';

    if( !KIPLATFORM::IO::AtomicWriteFile( aProject.GetProjectFullName(), payload.data(), payload.size() ) )
        return false;

    aProject.NotifyGitRefresh();
    return true;
}


/// Keep the binding in a project file the backend just wrote, then take its settings into memory
bool recordInReplacedProjectFile( SETTINGS_MANAGER& aManager, PROJECT& aProject )
{
    bool written = writeWorkspaceSection( aProject );

    // Reloaded even after a failed write, so memory never stays behind the file on disk
    bool reloaded = aManager.ProjectFileReplaced( aProject );

    return written && reloaded;
}


/// Persist this copy's items and bases, rereading the project file's settings only if the backend rewrote it
bool persist( SETTINGS_MANAGER& aManager, PROJECT& aProject, bool aIsProjectFile, bool aReplaced )
{
    if( !aIsProjectFile && aProject.IsReadOnly() )
        return false;

    PROJECT_LOCAL_SETTINGS& local = aProject.GetLocalSettings();

    // A project save treats the local settings as best effort, but a lost item or base disarms the stale check
    local.SaveToFile( aProject.GetProjectPath() );

    const bool saved = !local.LastSaveFailed();

    // A save would put the stale settings in memory over the project file the backend just wrote
    if( aIsProjectFile && aReplaced )
        return recordInReplacedProjectFile( aManager, aProject ) && saved;

    return saved;
}

} // namespace


MDW_SAVE_GUARD::SAVE_ACTION MDW_SAVE_GUARD::SaveAction( bool aCachedForeignLock, SAVE_PHASE aPhase,
                                                        const MDW_SAVE_CHECK& aResult )
{
    if( aCachedForeignLock )
        return SAVE_ACTION::PROMPT;

    switch( aPhase )
    {
    case SAVE_PHASE::LOOKUP:    return SAVE_ACTION::SAVE;
    case SAVE_PHASE::LIFECYCLE: return SAVE_ACTION::SAVE_UNCHECKED;
    case SAVE_PHASE::SETTLED:   break;
    }

    if( !aResult.managed )
        return SAVE_ACTION::SAVE;

    return warnsBeforeSave( aResult.category, aResult.lock ) ? SAVE_ACTION::PROMPT : SAVE_ACTION::SAVE;
}


std::string MDW_SAVE_GUARD::ProjectFileSnapshot( const PROJECT& aProject )
{
    std::ifstream      in( ToFsPath( aProject.GetProjectFullName() ), std::ios::binary );
    std::ostringstream bytes;

    if( in )
        bytes << in.rdbuf();

    return bytes.str();
}


bool MDW_SAVE_GUARD::RecordCheckout( SETTINGS_MANAGER& aManager, PROJECT& aProject, const wxString& aLocalPath,
                                     const wxString& aItemId, const MDW_ITEM_BASE& aBase,
                                     const std::string& aSnapshot )
{
    if( aLocalPath.IsEmpty() )
        return true;

    const bool projectFile = isProjectFile( aProject, aLocalPath );

    // Compared before this writes the file itself, so only the backend's write counts
    const bool replaced = projectFile && ProjectFileSnapshot( aProject ) != aSnapshot;

    // Saved even when unchanged in memory, so a write that failed earlier is retried
    aProject.GetLocalSettings().SetWorkspaceItemId( aLocalPath, aItemId );
    aProject.GetLocalSettings().SetWorkspaceBase( aLocalPath, aBase );

    return persist( aManager, aProject, projectFile, replaced );
}


bool MDW_SAVE_GUARD::RecordBase( SETTINGS_MANAGER& aManager, PROJECT& aProject, const wxString& aLocalPath,
                                 const wxString& aItemId, const MDW_ITEM_BASE& aBase, const std::string& aSnapshot )
{
    if( aLocalPath.IsEmpty() )
        return true;

    const bool projectFile = isProjectFile( aProject, aLocalPath );
    const bool replaced = projectFile && ProjectFileSnapshot( aProject ) != aSnapshot;

    // A download of another item into a bound file says nothing about the bound item's base
    if( !aItemId.IsEmpty() && aProject.GetLocalSettings().GetWorkspaceItemId( aLocalPath ) == aItemId )
        aProject.GetLocalSettings().SetWorkspaceBase( aLocalPath, aBase );
    else if( !projectFile )
        return true;

    return persist( aManager, aProject, projectFile, replaced );
}


bool MDW_SAVE_GUARD::RecordBinding( SETTINGS_MANAGER& aManager, PROJECT& aProject,
                                    const MDW_PROJECT_BINDING& aBinding )
{
    PROJECT_FILE&                                     file = aProject.GetProjectFile();
    PROJECT_LOCAL_SETTINGS&                           local = aProject.GetLocalSettings();
    const MDW_PROJECT_BINDING                         before = file.WorkspaceBinding();
    const PROJECT_LOCAL_SETTINGS::WORKSPACE_CHECKOUTS checkouts = local.WorkspaceCheckouts();
    const bool                                        disabledHere = local.m_WorkspaceDisabledHere;
    const bool                                        changed = before != aBinding;

    // A failed rebind keeps the checkouts, which a later save would otherwise write away
    auto restore =
            [&]()
            {
                file.SetWorkspaceBinding( before );
                local.RestoreWorkspaceCheckouts( checkouts );
                local.m_WorkspaceDisabledHere = disabledHere;
            };

    if( changed )
    {
        file.SetWorkspaceBinding( aBinding );
        local.ClearWorkspaceItems();
    }

    local.m_WorkspaceDisabledHere = false;

    // A project save treats the local settings as best effort, and a lost clear brings the old ids back
    local.SaveToFile( aProject.GetProjectPath() );

    if( local.LastSaveFailed() )
    {
        restore();
        return false;
    }

    // Saving writes every default an editor has filled in, which would leave the file changed for nothing
    if( !changed || aManager.SaveProject( wxEmptyString, &aProject ) )
        return true;

    restore();
    local.SaveToFile( aProject.GetProjectPath() );
    return false;
}


wxString MDW_SAVE_GUARD::CheckinBase( const PROJECT& aProject, const wxString& aLocalPath, const wxString& aItemId )
{
    const PROJECT_LOCAL_SETTINGS& local = aProject.GetLocalSettings();
    std::optional<MDW_ITEM_BASE>  base = local.GetWorkspaceBase( aLocalPath );

    // Another item's base would read as a stale one and refuse a sound check-in
    if( !base || local.GetWorkspaceItemId( aLocalPath ) != aItemId )
        return wxEmptyString;

    return base->revision_id;
}
