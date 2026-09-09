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

#include <schematic_file_util.h>
#include <qa_utils/wx_utils/unit_test_utils.h> // GetEeschemaTestDataDir()

#include <stdexcept>
#include <unordered_set>

#include <settings/settings_manager.h>

#include <connection_graph.h>
#include <project.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_rule_area.h>

// For SCH parsing
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_parser.h>
#include <richio.h>

#include <qa_utils/stdstream_line_reader.h>

namespace KI_TEST
{

void DumpSchematicToFile( SCHEMATIC& aSchematic, SCH_SHEET& aSheet, const std::string& aFilename )
{
    SCH_IO_KICAD_SEXPR io;
    io.SaveSchematicFile( aFilename, &aSheet, &aSchematic );
}

std::unique_ptr<SCHEMATIC> LoadHierarchyFromRoot( const std::string& rootFilename,
                                                  PROJECT* project )
{
    auto schematic = std::make_unique<SCHEMATIC>( project );
    schematic->Reset();
    SCH_SHEET* defaultSheet = schematic->GetTopLevelSheet( 0 );

    SCH_IO_KICAD_SEXPR io;
    SCH_SHEET* rootSheet = io.LoadSchematicFile( rootFilename, schematic.get() );
    schematic->AddTopLevelSheet( rootSheet );
    schematic->RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    if( !io.GetError().IsEmpty() )
        throw std::runtime_error( io.GetError().ToStdString( wxConvUTF8 ) );

    return schematic;
}

std::unique_ptr<SCHEMATIC> ReadSchematicFromStream( std::istream& aStream, PROJECT* aProject )
{
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( aProject );
    SCH_SHEET* rootSheet = new SCH_SHEET( schematic.get() );
    SCH_SCREEN* screen = new SCH_SCREEN( schematic.get() );
    rootSheet->SetScreen( screen );
    screen->SetParent( schematic.get() );
    schematic->SetTopLevelSheets( { rootSheet } );

    // Parse from provided stream using existing parser infra
    STDISTREAM_LINE_READER reader;
    reader.SetStream( aStream );
    SCH_IO_KICAD_SEXPR_PARSER parser( &reader, nullptr, 0, rootSheet );
    parser.ParseSchematic( rootSheet );

    // Link symbol instances like LoadSchematic does (single sheet case)
    rootSheet->GetScreen()->UpdateLocalLibSymbolLinks();
    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    sheets.UpdateSymbolInstanceData( schematic->RootScreen()->GetSymbolInstances() );
    sheets.UpdateSheetInstanceData( schematic->RootScreen()->GetSheetInstances() );
    sheets.AnnotatePowerSymbols();
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();

    // The IO layer normally pipes parsed override maps into the connection graph;
    // this reload-only path has to do the same so manual chains survive a reload.
    if( schematic->ConnectionGraph() )
    {
        schematic->ConnectionGraph()->SetNetChainNetClassOverrides( parser.GetNetChainNetClasses() );
        schematic->ConnectionGraph()->SetNetChainColorOverrides( parser.GetNetChainColors() );

        std::map<wxString, CONNECTION_GRAPH::CHAIN_TERMINAL_REFS> termRefs;

        for( const auto& [name, terms] : parser.GetNetChainTerminalRefs() )
        {
            termRefs[name] = { { terms.first.ref, terms.first.pin },
                               { terms.second.ref, terms.second.pin } };
        }

        schematic->ConnectionGraph()->SetNetChainTerminalRefOverrides( termRefs );
        schematic->ConnectionGraph()->SetNetChainMemberNetOverrides( parser.GetNetChainMemberNets() );
    }

    return schematic;
}


void LoadSchematic( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                    std::unique_ptr<SCHEMATIC>& aSchematic )
{
    if( aSchematic )
    {
        PROJECT* prj = &aSchematic->Project();

        aSchematic->SetProject( nullptr );
        aSettingsManager.UnloadProject( prj, false );
        aSchematic->Reset();
    }

    std::string absPath = GetEeschemaTestDataDir() + aRelPath.ToStdString();
    wxFileName  projectFile( absPath + ".kicad_pro" );
    wxFileName  legacyProject( absPath + ".pro" );
    std::string schematicPath = absPath + ".kicad_sch";

    if( projectFile.Exists() )
        aSettingsManager.LoadProject( projectFile.GetFullPath() );
    else if( legacyProject.Exists() )
        aSettingsManager.LoadProject( legacyProject.GetFullPath() );
    else
        aSettingsManager.LoadProject( "" );

    aSettingsManager.Prj().SetElem( PROJECT::ELEM::LEGACY_SYMBOL_LIBS, nullptr );

    aSchematic = LoadHierarchyFromRoot( schematicPath, &aSettingsManager.Prj() );

    SCH_SCREENS screens( aSchematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    SCH_SHEET_LIST sheets = aSchematic->BuildSheetListSortedByPageNumbers();

    // Restore all of the loaded symbol instances from the root sheet screen.
    sheets.UpdateSymbolInstanceData( aSchematic->RootScreen()->GetSymbolInstances() );
    sheets.UpdateSheetInstanceData( aSchematic->RootScreen()->GetSheetInstances() );

    if( aSchematic->RootScreen()->GetFileFormatVersionAtLoad() < 20230221 )
        screens.FixLegacyPowerSymbolMismatches();

    if( aSchematic->RootScreen()->GetFileFormatVersionAtLoad() < 20221206 )
    {
        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
            screen->MigrateSimModels();
    }


    sheets.AnnotatePowerSymbols();

    // NOTE: This is required for multi-unit symbols to be correct
    // Normally called from SCH_EDIT_FRAME::FixupJunctions() but could be refactored
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();

    // NOTE: SchematicCleanUp is not called; QA schematics must already be clean or else
    // SchematicCleanUp must be freed from its UI dependencies.

    std::unordered_set<SCH_SCREEN*> allScreens;

    for( const SCH_SHEET_PATH& path : sheets )
        allScreens.insert( path.LastScreen() );

    SCH_RULE_AREA::UpdateRuleAreasInScreens( allScreens, nullptr );
    aSchematic->ConnectionGraph()->Recalculate( sheets, true );
}

} // namespace KI_TEST
