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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "import_local_footprint_libs.h"

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>

#include <footprint_library_adapter.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <pgm_base.h>
#include <project.h>
#include <project_pcb.h>
#include <reporter.h>
#include <pcb_io/pcb_io_mgr.h>


std::vector<wxString> RegisterLocalFootprintLibraries( PROJECT& aProject, const wxString& aSourceProjectDir,
                                                       const std::vector<wxString>& aLibraries,
                                                       REPORTER&                    aReporter )
{
    std::vector<wxString> added;

    if( aLibraries.empty() )
        return added;

    LIBRARY_MANAGER&              manager = Pgm().GetLibraryManager();
    FOOTPRINT_LIBRARY_ADAPTER*    adapter = PROJECT_PCB::FootprintLibAdapter( &aProject );
    std::optional<LIBRARY_TABLE*> optTable =
            manager.Table( LIBRARY_TABLE_TYPE::FOOTPRINT, LIBRARY_TABLE_SCOPE::PROJECT );

    wxCHECK_MSG( adapter && optTable.has_value(), added, "Could not load footprint lib table." );

    LIBRARY_TABLE* table = optTable.value();
    wxString       projectPath = aProject.GetProjectPath();

    for( const wxString& file : aLibraries )
    {
        if( file.empty() )
            continue;

        wxFileName             fn( file );

        // a project can list a library that was never shipped
        if( !fn.FileExists() && !wxDirExists( fn.GetFullPath() ) )
            continue;

        PCB_IO_MGR::PCB_FILE_T type = PCB_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

        if( type == PCB_IO_MGR::FILE_TYPE_NONE )
        {
            wxLogTrace( "KIWAY", "Unknown file type: %s", fn.GetFullPath() );
            continue;
        }

        // Only libraries that live under the source project are relocated; a plain path prefix would
        // also match a sibling directory sharing the project name's stem.
        wxFileName relFn( fn );
        bool       isProjectLocal = !aSourceProjectDir.IsEmpty() && relFn.MakeRelativeTo( aSourceProjectDir )
                                    && !relFn.IsAbsolute() && !relFn.GetFullPath().StartsWith( wxS( ".." ) );
        wxString   libTableUri;

        if( isProjectLocal )
        {
            // Copy a project-local library into the KiCad project directory and reference it
            // with a project-relative path so the fp-lib-table stays portable.
            if( !fn.FileExists() )
                continue;

            wxFileName projectFn( projectPath, fn.GetFullName() );

            if( fn.GetFullPath() != projectFn.GetFullPath() && !projectFn.FileExists()
                && !wxCopyFile( fn.GetFullPath(), projectFn.GetFullPath(), false ) )
            {
                aReporter.Report( wxString::Format( _( "Error copying footprint library '%s'." ), fn.GetFullPath() ),
                                  RPT_SEVERITY_ERROR );
                continue;
            }

            libTableUri = wxS( "${KIPRJMOD}/" ) + fn.GetFullName();
        }
        else
        {
            // External library referenced by absolute path. Preserve the original path.
            libTableUri = fn.GetFullPath();
        }

        if( !table->HasRow( fn.GetName() ) )
        {
            LIBRARY_TABLE_ROW& row = table->InsertRow();
            row.SetNickname( fn.GetName() );
            row.SetURI( libTableUri );
            row.SetType( PCB_IO_MGR::ShowType( type ) );
            added.emplace_back( fn.GetName() );
        }
    }

    if( added.empty() )
        return added;

    bool success = true;

    table->Save().map_error(
            [&]( const LIBRARY_ERROR& aError )
            {
                aReporter.Report( _( "Error saving project library table:\n\n" ) + aError.message,
                                  RPT_SEVERITY_ERROR );
                success = false;
            } );

    if( !success )
        return {};

    manager.AbortAsyncLoads();
    manager.LoadProjectTables( { LIBRARY_TABLE_TYPE::FOOTPRINT } );

    for( const wxString& nick : added )
        adapter->LoadOne( nick );

    return added;
}
