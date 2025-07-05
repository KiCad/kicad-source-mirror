/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <algorithm>
#include <vector>

#include <schematic.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct STACKED_PIN_FIXTURE
{
    STACKED_PIN_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};

static std::vector<wxString> ToVector( const std::initializer_list<const char*>& init )
{
    std::vector<wxString> out;
    for( const char* s : init )
        out.emplace_back( wxString::FromUTF8( s ) );
    return out;
}

BOOST_FIXTURE_TEST_CASE( StackedPinNomenclature_ExpandsCorrectly, STACKED_PIN_FIXTURE )
{
    LOCALE_IO dummy;

    // Load the custom schematic with bracketed pin numbers
    KI_TEST::LoadSchematic( m_settingsManager, "stacked_pin_nomenclature", m_schematic );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    BOOST_REQUIRE( sheets.size() >= 1 );
    SCH_SCREEN*    screen = sheets.at( 0 ).LastScreen();

    // Find the Device:R symbol on the sheet
    SCH_SYMBOL* resistor = nullptr;
    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        if( sym->GetSchSymbolLibraryName() == wxT( "Device:R" ) )
        {
            resistor = sym;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( resistor, "Resistor symbol not found in test schematic" );

    // Collect both pins (each with bracketed numbers)
    std::vector<SCH_PIN*> rpins = resistor->GetPins( &sheets.at( 0 ) );
    BOOST_REQUIRE_EQUAL( rpins.size(), 2 );

    // For determinism, sort by local Y position
    std::sort( rpins.begin(), rpins.end(), []( SCH_PIN* a, SCH_PIN* b ) {
        return a->GetLocalPosition().y < b->GetLocalPosition().y;
    } );

    // Top pin is [1-5]
    bool validTop = false;
    std::vector<wxString> top = rpins[0]->GetStackedPinNumbers( &validTop );
    BOOST_CHECK( validTop );
    std::vector<wxString> expectedTop = ToVector( { "1", "2", "3", "4", "5" } );
    BOOST_CHECK_EQUAL_COLLECTIONS( top.begin(), top.end(), expectedTop.begin(), expectedTop.end() );

    // Bottom pin is [6,7,9-11]
    bool validBot = false;
    std::vector<wxString> bot = rpins[1]->GetStackedPinNumbers( &validBot );
    BOOST_CHECK( validBot );
    std::vector<wxString> expectedBot = ToVector( { "6", "7", "9", "10", "11" } );
    BOOST_CHECK_EQUAL_COLLECTIONS( bot.begin(), bot.end(), expectedBot.begin(), expectedBot.end() );

    // Total expanded count across both pins should be 10
    size_t total = top.size() + bot.size();
    BOOST_CHECK_EQUAL( total, 10 );
}
