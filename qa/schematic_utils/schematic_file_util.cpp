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
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

#include <schematic_file_util.h>

#include <settings/settings_manager.h>

#include <connection_graph.h>
#include <lib_textbox.h>
#include <schematic.h>
#include <sch_screen.h>

// For SCH parsing
#include <sch_plugins/kicad/sch_sexpr_plugin.h>
#include <sch_plugins/kicad/sch_sexpr_parser.h>
#include <richio.h>

#include <qa_utils/stdstream_line_reader.h>

namespace KI_TEST
{

#ifndef QA_EESCHEMA_DATA_LOCATION
    #define QA_EESCHEMA_DATA_LOCATION "???"
#endif

std::string getEeschemaTestDataDir()
{
    const char* env = std::getenv( "KICAD_TEST_EESCHEMA_DATA_DIR" );
    std::string fn;

    if( !env )
    {
        // Use the compiled-in location of the data dir (i.e. where the files were at build time)
        fn = QA_EESCHEMA_DATA_LOCATION;
    }
    else
    {
        // Use whatever was given in the env var
        fn = env;
    }

    // Ensure the string ends in / to force a directory interpretation
    fn += "/";

    return fn;
}


void DumpSchematicToFile( SCHEMATIC& aSchematic, SCH_SHEET& aSheet, const std::string& aFilename )
{
    SCH_SEXPR_PLUGIN io;
    io.Save( aFilename, &aSheet, &aSchematic );
}


std::unique_ptr<SCHEMATIC> ReadSchematicFromStream( std::istream& aStream, PROJECT* aProject )
{
    STDISTREAM_LINE_READER reader;
    reader.SetStream( aStream );

    SCH_SEXPR_PARSER           parser( &reader );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );

    try
    {
        schematic->SetProject( aProject );
        SCH_SHEET* newSheet = new SCH_SHEET( schematic.get() );
        schematic->SetRoot( newSheet );
        SCH_SCREEN* rootScreen = new SCH_SCREEN( schematic.get() );
        schematic->Root().SetScreen( rootScreen );
        parser.ParseSchematic( newSheet );
    }
    catch( const IO_ERROR& )
    {
    }

    return schematic;
}


std::unique_ptr<SCHEMATIC> ReadSchematicFromFile( const std::string& aFilename, PROJECT* aProject )
{
    std::ifstream file_stream;
    file_stream.open( aFilename );

    wxASSERT( file_stream.is_open() );

    return ReadSchematicFromStream( file_stream, aProject );
}


void LoadSchematic( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                std::unique_ptr<SCHEMATIC>& aSchematic )
{
    if( aSchematic )
    {
        PROJECT* prj = &aSchematic->Prj();

        aSchematic->SetProject( nullptr );
        aSettingsManager.UnloadProject( prj, false );
        aSchematic->Reset();
    }

    std::string absPath = getEeschemaTestDataDir() + aRelPath.ToStdString();
    wxFileName  projectFile( absPath + ".kicad_pro" );
    wxFileName  legacyProject( absPath + ".pro" );
    std::string schematicPath = absPath + ".kicad_sch";

    if( projectFile.Exists() )
        aSettingsManager.LoadProject( projectFile.GetFullPath() );
    else if( legacyProject.Exists() )
        aSettingsManager.LoadProject( legacyProject.GetFullPath() );
    else
        aSettingsManager.LoadProject( "" );

    aSettingsManager.Prj().SetElem( PROJECT::ELEM_SCH_SYMBOL_LIBS, nullptr );

    aSchematic = ReadSchematicFromFile( schematicPath, &aSettingsManager.Prj() );

    aSchematic->CurrentSheet().push_back( &aSchematic->Root() );

   SCH_SCREENS screens( aSchematic->Root() );

   for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
       screen->UpdateLocalLibSymbolLinks();

   SCH_SHEET_LIST sheets = aSchematic->GetSheets();

   // Restore all of the loaded symbol instances from the root sheet screen.
   sheets.UpdateSymbolInstances( aSchematic->RootScreen()->GetSymbolInstances() );
   sheets.UpdateSheetInstances( aSchematic->RootScreen()->GetSheetInstances() );

   sheets.AnnotatePowerSymbols();

   // NOTE: This is required for multi-unit symbols to be correct
   // Normally called from SCH_EDIT_FRAME::FixupJunctions() but could be refactored
   for( SCH_SHEET_PATH& sheet : sheets )
       sheet.UpdateAllScreenReferences();

   // NOTE: SchematicCleanUp is not called; QA schematics must already be clean or else
   // SchematicCleanUp must be freed from its UI dependencies.

   aSchematic->ConnectionGraph()->Recalculate( sheets, true );
}

} // namespace KI_TEST
