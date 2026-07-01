/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include "eeschema_helpers.h"

#include <algorithm>

#include <connection_graph.h>
#include <locale_io.h>
#include <project/project_file.h>
#include <schematic.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_label.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <tool/tool_manager.h>
#include <kiface_base.h>

#include <wx/app.h>


SCH_EDIT_FRAME*   EESCHEMA_HELPERS::s_SchEditFrame = nullptr;


void EESCHEMA_HELPERS::SetSchEditFrame( SCH_EDIT_FRAME* aSchEditFrame )
{
    s_SchEditFrame = aSchEditFrame;
}


SCHEMATIC* EESCHEMA_HELPERS::LoadSchematic( const wxString& aFileName, bool aSetActive,
                                            bool aForceDefaultProject, PROJECT* aProject, bool aCalculateConnectivity )
{
    if( aFileName.EndsWith( FILEEXT::KiCadSchematicFileExtension ) )
        return LoadSchematic( aFileName, SCH_IO_MGR::SCH_KICAD, aSetActive, aForceDefaultProject,
                              aProject, aCalculateConnectivity );
    else if( aFileName.EndsWith( FILEEXT::LegacySchematicFileExtension ) )
        return LoadSchematic( aFileName, SCH_IO_MGR::SCH_LEGACY, aSetActive, aForceDefaultProject,
                              aProject, aCalculateConnectivity );

    // as fall back for any other kind use the legacy format
    return LoadSchematic( aFileName, SCH_IO_MGR::SCH_LEGACY, aSetActive, aForceDefaultProject, aProject,
                          aCalculateConnectivity );
}


SCHEMATIC* EESCHEMA_HELPERS::LoadSchematic( const wxString& aFileName,
                                            SCH_IO_MGR::SCH_FILE_T aFormat,
                                            bool aSetActive,
                                            bool aForceDefaultProject,
                                            PROJECT* aProject,
                                            bool aCalculateConnectivity )
{
    wxFileName pro = aFileName;
    pro.SetExt( FILEEXT::ProjectFileExtension );
    pro.MakeAbsolute();
    wxString projectPath = pro.GetFullPath();

    // Ensure the "C" locale is temporary set, before reading any file
    // It also avoid wxWidget alerts about locale issues, later, when using Python 3
    LOCALE_IO dummy;

    PROJECT* project = aProject;
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    if( !project )
    {
        project = mgr.GetProject( projectPath );
    }

    if( !aForceDefaultProject )
    {
        if( !project )
        {
            if( wxFileExists( projectPath ) )
            {
                mgr.LoadProject( projectPath, aSetActive );
                project = mgr.GetProject( projectPath );
            }
        }
        else if( s_SchEditFrame && project == &mgr.Prj() )
        {
            // Project is already loaded?  Then so is the board
            return &s_SchEditFrame->Schematic();
        }
    }

    // Board cannot be loaded without a project, so create the default project
    if( !project || aForceDefaultProject )
        project = &mgr.Prj();

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( aFormat ) );

    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( project );
    schematic->CreateDefaultScreens();

    wxFileName schFile = aFileName;
    schFile.MakeAbsolute();

    try
    {
        SCH_SHEET* rootSheet = pi->LoadSchematicFile( schFile.GetFullPath(), schematic.get() );

        if( !rootSheet )
        {
            schematic->SetProject( nullptr );
            return nullptr;
        }

        std::vector<SCH_SHEET*> topLevelSheets = schematic->GetTopLevelSheets();
        bool rootIsTopLevel = std::find( topLevelSheets.begin(), topLevelSheets.end(), rootSheet )
                              != topLevelSheets.end();
        bool rootIsVirtualRoot = rootSheet == &schematic->Root() || rootSheet->IsVirtualRootSheet();

        if( !rootIsTopLevel && !rootIsVirtualRoot )
            schematic->SetTopLevelSheets( { rootSheet } );

        // Make ${SHEETNAME} work on the root sheet until we properly support naming the root
        // sheet.  Prefer the display name from the matching schematic.top_level_sheets entry in
        // the project file so CLI/API exports show the same name the GUI does.
        if( rootSheet->GetName().IsEmpty() )
        {
            wxString rootName = _( "Root" );

            for( const TOP_LEVEL_SHEET_INFO& info : project->GetProjectFile().GetTopLevelSheets() )
            {
                wxFileName candidate( project->GetProjectPath(), info.filename );

                if( candidate.SameAs( schFile ) && !info.name.IsEmpty() )
                {
                    rootName = info.name;
                    break;
                }
            }

            rootSheet->SetName( rootName );
        }
    }
    catch( ... )
    {
        schematic->SetProject( nullptr );
        return nullptr;
    }

    SCH_SHEET_LIST sheetList = schematic->BuildSheetListSortedByPageNumbers();
    SCH_SCREENS    screens( schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    if( schematic->RootScreen()->GetFileFormatVersionAtLoad() < 20221002 )
        sheetList.UpdateSymbolInstanceData( schematic->RootScreen()->GetSymbolInstances());

    sheetList.UpdateSheetInstanceData( schematic->RootScreen()->GetSheetInstances());

    if( schematic->RootScreen()->GetFileFormatVersionAtLoad() < 20230221 )
        screens.FixLegacyPowerSymbolMismatches();

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->MigrateSimModels();

    schematic->LoadVariants();

    wxString projectName = project->GetProjectName();

    if( projectName.IsEmpty() )
        projectName = schFile.GetName();

    // Check must run before pruning so variant data on a stale instance path is migrated
    // onto the new instance before the orphan is removed.
    sheetList.CheckForMissingSymbolInstances( projectName );
    screens.PruneOrphanedSymbolInstances( projectName, sheetList );
    screens.PruneOrphanedSheetInstances( projectName, sheetList );

    sheetList.AnnotatePowerSymbols();

    if( sheetList.AllSheetPageNumbersEmpty() )
        sheetList.SetInitialPageNumbers();
    else
        sheetList.RepairPageNumbers();

    schematic->ConnectionGraph()->Reset();

    TOOL_MANAGER* toolManager = new TOOL_MANAGER;
    toolManager->SetEnvironment( schematic.get(), nullptr, nullptr, Kiface().KifaceSettings(), nullptr );

    if( aCalculateConnectivity )
    {
        SCH_COMMIT dummyCommit( toolManager );
        schematic->RecalculateConnections( &dummyCommit, GLOBAL_CLEANUP, toolManager );
    }

    schematic->ResolveERCExclusionsPostUpdate();

    schematic->SetSheetNumberAndCount();
    schematic->RecomputeIntersheetRefs();

    for( SCH_SHEET_PATH& sheet : sheetList )
    {
        sheet.UpdateAllScreenReferences();
        sheet.LastScreen()->TestDanglingEnds( nullptr, nullptr );
    }

    if( aCalculateConnectivity )
        schematic->ConnectionGraph()->Recalculate( sheetList, true );

    return schematic.release();
}
