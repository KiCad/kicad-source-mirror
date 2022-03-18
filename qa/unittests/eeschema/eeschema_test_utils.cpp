/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "eeschema_test_utils.h"

#include <cstdlib>
#include <memory>

#include <eeschema/sch_io_mgr.h>
#include <eeschema/sch_screen.h>
#include <eeschema/schematic.h>
#include <eeschema/connection_graph.h>


#ifndef QA_EESCHEMA_DATA_LOCATION
    #define QA_EESCHEMA_DATA_LOCATION "???"
#endif

wxFileName KI_TEST::GetEeschemaTestDataDir()
{
    const char* env = std::getenv( "KICAD_TEST_EESCHEMA_DATA_DIR" );
    wxString fn;

    if( !env )
    {
        // Use the compiled-in location of the data dir
        // (i.e. where the files were at build time)
        fn << QA_EESCHEMA_DATA_LOCATION;
    }
    else
    {
        // Use whatever was given in the env var
        fn << env;
    }

    // Ensure the string ends in / to force a directory interpretation
    fn << "/";

    return wxFileName{ fn };
}


std::unique_ptr<SCHEMATIC> ReadSchematicFromFile( const std::string& aFilename )
{
    auto pi = SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD );
    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( nullptr );

    schematic->Reset();
    schematic->SetRoot( pi->Load( aFilename, schematic.get() ) );
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_SCREENS screens( schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    SCH_SHEET_LIST sheets = schematic->GetSheets();

    // Restore all of the loaded symbol instances from the root sheet screen.
    sheets.UpdateSymbolInstances( schematic->RootScreen()->GetSymbolInstances() );

    sheets.AnnotatePowerSymbols();

    // NOTE: This is required for multi-unit symbols to be correct
    // Normally called from SCH_EDIT_FRAME::FixupJunctions() but could be refactored
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();

    // NOTE: SchematicCleanUp is not called; QA schematics must already be clean or else
    // SchematicCleanUp must be freed from its UI dependencies.

    schematic->ConnectionGraph()->Recalculate( sheets, true );

    return schematic;
}
