/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <eeschema_helpers.h>
#include <sch_edit_frame.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <schematic.h>
#include <sch_io_mgr.h>
#include <locale_io.h>
#include <wx/app.h>
#include <sch_label.h>
#include <connection_graph.h>

SCH_EDIT_FRAME*   EESCHEMA_HELPERS::s_SchEditFrame = nullptr;
SETTINGS_MANAGER* EESCHEMA_HELPERS::s_SettingsManager = nullptr;


void EESCHEMA_HELPERS::SetSchEditFrame( SCH_EDIT_FRAME* aSchEditFrame )
{
    s_SchEditFrame = aSchEditFrame;
}


SETTINGS_MANAGER* EESCHEMA_HELPERS::GetSettingsManager()
{
    if( !s_SettingsManager )
    {
        if( s_SchEditFrame )
        {
            s_SettingsManager = s_SchEditFrame->GetSettingsManager();
        }
        else
        {
            s_SettingsManager = new SETTINGS_MANAGER( true );
        }
    }

    return s_SettingsManager;
}


PROJECT* EESCHEMA_HELPERS::GetDefaultProject()
{
    // For some reasons, LoadProject() needs a C locale, so ensure we have the right locale
    // This is mainly when running QA Python tests
    LOCALE_IO dummy;

    PROJECT* project = GetSettingsManager()->GetProject( "" );

    if( !project )
    {
        GetSettingsManager()->LoadProject( "" );
        project = GetSettingsManager()->GetProject( "" );
    }

    return project;
}


SCHEMATIC* EESCHEMA_HELPERS::LoadSchematic( wxString& aFileName )
{
    if( aFileName.EndsWith( KiCadSchematicFileExtension ) )
        return LoadSchematic( aFileName, SCH_IO_MGR::SCH_KICAD );
    else if( aFileName.EndsWith( LegacySchematicFileExtension ) )
        return LoadSchematic( aFileName, SCH_IO_MGR::SCH_LEGACY );

    // as fall back for any other kind use the legacy format
    return LoadSchematic( aFileName, SCH_IO_MGR::SCH_LEGACY );
}


SCHEMATIC* EESCHEMA_HELPERS::LoadSchematic( wxString& aFileName, SCH_IO_MGR::SCH_FILE_T aFormat )
{
    wxFileName pro = aFileName;
    pro.SetExt( ProjectFileExtension );
    pro.MakeAbsolute();
    wxString projectPath = pro.GetFullPath();

    // Ensure the "C" locale is temporary set, before reading any file
    // It also avoid wxWidget alerts about locale issues, later, when using Python 3
    LOCALE_IO dummy;

    PROJECT* project = GetSettingsManager()->GetProject( projectPath );

    if( !project )
    {
        if( wxFileExists( projectPath ) )
        {
            GetSettingsManager()->LoadProject( projectPath, false );
            project = GetSettingsManager()->GetProject( projectPath );
        }
    }
    else if( s_SchEditFrame && project == &GetSettingsManager()->Prj() )
    {
        // Project is already loaded?  Then so is the board
        return &s_SchEditFrame->Schematic();
    }

    // Board cannot be loaded without a project, so create the default project
    if( !project )
        project = GetDefaultProject();

    SCH_PLUGIN*                     plugin = SCH_IO_MGR::FindPlugin( aFormat );
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( plugin );

    SCHEMATIC* schematic = new SCHEMATIC( project );

    try
    {
        schematic->SetRoot( pi->Load( aFileName, schematic ) );
    }
    catch( ... )
    {
        return nullptr;
    }

    SCH_SHEET_LIST sheetList = schematic->GetSheets();
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

    sheetList.AnnotatePowerSymbols();

    schematic->ConnectionGraph()->Reset();

    schematic->SetSheetNumberAndCount();
    schematic->RecomputeIntersheetRefs( []( SCH_GLOBALLABEL* aGlobal )
                                        {
                                            for( SCH_FIELD& field : aGlobal->GetFields() )
                                                field.ClearBoundingBoxCache();

                                            aGlobal->ClearBoundingBoxCache();
                                        } );

    for( SCH_SHEET_PATH& sheet : sheetList )
    {
        sheet.UpdateAllScreenReferences();
        sheet.LastScreen()->TestDanglingEnds( nullptr, nullptr );
    }

    schematic->ConnectionGraph()->Recalculate( sheetList, true );


    return schematic;
}
