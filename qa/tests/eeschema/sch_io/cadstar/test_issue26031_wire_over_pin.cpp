/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_issue26031_wire_over_pin.cpp
 *
 * Regression coverage for https://gitlab.com/kicad/code/kicad/-/issues/26031.
 *
 * The 0V connection in the attached file runs straight over U1 pin 2, which belongs to net VB.
 * KiCad connects any pin lying on a wire, so the import must route the wire around that pin.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <connection_graph.h>
#include <sch_io/cadstar/sch_io_cadstar_archive.h>
#include <sch_connection.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>


BOOST_AUTO_TEST_CASE( CadstarWireOverForeignPin )
{
    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );

    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_CADSTAR_ARCHIVE plugin;
    plugin.LoadSchematicFile( KI_TEST::GetEeschemaTestDataDir() + "io/cadstar/issue26031_wire_over_pin.csa",
                              schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    const SCH_SHEET_PATH& path = sheets.front();
    SCH_CONNECTION*       pin1Conn = nullptr;
    SCH_CONNECTION*       pin2Conn = nullptr;

    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &path, false ) != wxS( "U1" ) )
            continue;

        for( SCH_PIN* pin : symbol->GetPins( &path ) )
        {
            if( pin->GetNumber() == wxS( "1" ) )
                pin1Conn = pin->Connection( &path );
            else if( pin->GetNumber() == wxS( "2" ) )
                pin2Conn = pin->Connection( &path );
        }
    }

    BOOST_REQUIRE( pin1Conn );
    BOOST_REQUIRE( pin2Conn );

    BOOST_CHECK_NE( pin1Conn->NetCode(), pin2Conn->NetCode() );
}
