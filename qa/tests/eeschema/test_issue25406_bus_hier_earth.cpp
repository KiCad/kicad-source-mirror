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

/**
 * @file test_issue25406_bus_hier_earth.cpp
 *
 * Test for issue #25406: nets are not resolved through buses and hierarchical sheets.
 *
 * The mains earth leaves the input connector on a bus and passes untouched through the meter and
 * isolator sheets to the output connector.  Both pass-through sheets short their incoming bus
 * member to their outgoing one, and only the last such short used to survive.
 */

#include <boost/test/unit_test.hpp>
#include <eeschema_test_utils.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <connection_graph.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <settings/settings_manager.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <locale_io.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>


struct ISSUE25406_FIXTURE
{
    ISSUE25406_FIXTURE()
    {
        wxFileName projectPath( KI_TEST::GetEeschemaTestDataDir() );
        projectPath.AppendDir( "issue25406" );
        projectPath.SetName( "issue25406" );
        projectPath.SetExt( FILEEXT::ProjectFileExtension );

        m_settingsManager.LoadProject( projectPath.GetFullPath().ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );

        wxFileName rootFile( projectPath );
        rootFile.SetExt( FILEEXT::KiCadSchematicFileExtension );

        SCH_IO_KICAD_SEXPR io;
        SCH_SHEET*         root = io.LoadSchematicFile( rootFile.GetFullPath(), m_schematic.get() );

        BOOST_REQUIRE( root );

        m_schematic->SetTopLevelSheets( { root } );

        SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

        sheets.UpdateSymbolInstanceData( m_schematic->RootScreen()->GetSymbolInstances() );
        sheets.UpdateSheetInstanceData( m_schematic->RootScreen()->GetSheetInstances() );
        sheets.AnnotatePowerSymbols();

        for( SCH_SHEET_PATH& sheet : sheets )
            sheet.UpdateAllScreenReferences();

        m_schematic->ConnectionGraph()->Recalculate( sheets, true );
    }

    ~ISSUE25406_FIXTURE() { m_schematic.reset(); }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( Issue25406EarthThroughPassThroughSheets, ISSUE25406_FIXTURE )
{
    LOCALE_IO dummy;

    std::map<wxString, int> pinNetCodes;

    for( const auto& [key, subgraphs] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin    = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    wxString ref = symbol->GetRef( &subgraph->GetSheet() ) + "." + pin->GetNumber();
                    pinNetCodes[ref] = key.Netcode;
                }
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( pinNetCodes.count( "J1.2" ), "Input connector earth pin should be found" );
    BOOST_REQUIRE_MESSAGE( pinNetCodes.count( "J2.2" ), "Output connector earth pin should be found" );

    BOOST_CHECK_MESSAGE( pinNetCodes["J1.2"] == pinNetCodes["J2.2"],
                         "Input and output earth should be on the same net" );
}
