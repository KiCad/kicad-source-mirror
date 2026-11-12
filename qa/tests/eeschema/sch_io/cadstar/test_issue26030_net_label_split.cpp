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
 * @file test_issue26030_net_label_split.cpp
 *
 * Regression coverage for https://gitlab.com/kicad/code/kicad/-/issues/26030.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <set>

#include <connection_graph.h>
#include <sch_connection.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <sch_io/cadstar/sch_io_cadstar_archive.h>


BOOST_AUTO_TEST_CASE( CadstarNetLabelOnSecondSheetJoinsSignal )
{
    SETTINGS_MANAGER settings;
    settings.LoadProject( "" );

    SCHEMATIC schematic( nullptr );
    schematic.SetProject( &settings.Prj() );

    SCH_IO_CADSTAR_ARCHIVE plugin;
    wxString               path = KI_TEST::GetEeschemaTestDataDir() + "io/cadstar/issue26030_net_label_split.csa";

    BOOST_REQUIRE( plugin.LoadSchematicFile( path, &schematic ) );

    SCH_SHEET_LIST sheets = schematic.BuildSheetListSortedByPageNumbers();
    schematic.ConnectionGraph()->Recalculate( sheets, true );

    std::set<wxString> netNames;
    int                pinCount = 0;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( item )->GetPins( &sheet ) )
            {
                if( pin->GetNumber() != wxT( "1" ) )
                    continue;

                pinCount++;
                netNames.insert( pin->Connection( &sheet )->GetNetName() );
            }
        }
    }

    // R1, R2, R3 pin 1 and the VNEG power symbol are one CADSTAR signal
    BOOST_CHECK_EQUAL( pinCount, 4 );
    BOOST_CHECK( netNames == std::set<wxString>{ wxT( "VNEG" ) } );
}
