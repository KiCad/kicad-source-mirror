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
 * @file test_issue26025_pin_renumber.cpp
 *
 * Regression coverage for https://gitlab.com/kicad/code/kicad/-/issues/26025.
 *
 * D1 in the attached file has terminal 1 as pin "2" and terminal 2 as pin "1". Renumbering the
 * library pins in place made the second lookup match both pins, leaving two pins numbered "1".
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>
#include <set>

#include <lib_symbol.h>
#include <sch_io/cadstar/sch_io_cadstar_archive.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>


BOOST_AUTO_TEST_CASE( CadstarTerminalPinRenumber )
{
    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );

    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_CADSTAR_ARCHIVE plugin;
    plugin.LoadSchematicFile( KI_TEST::GetEeschemaTestDataDir() + "io/cadstar/issue26025_pin_renumber.csa",
                              schematic.get() );

    SCH_SYMBOL* d1 = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "D1" ) )
                d1 = symbol;
        }
    }

    BOOST_REQUIRE( d1 );

    std::set<wxString> numbers;

    for( SCH_PIN* pin : d1->GetLibPins() )
        numbers.insert( pin->GetNumber() );

    BOOST_CHECK_EQUAL( d1->GetLibPins().size(), 2 );
    BOOST_CHECK( numbers == std::set<wxString>( { wxS( "1" ), wxS( "2" ) } ) );
}
