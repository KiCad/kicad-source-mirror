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
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_screen.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct LEGACY_POWER_SYMBOLS_TEST_FIXTURE
{
    LEGACY_POWER_SYMBOLS_TEST_FIXTURE()
    { }

    void CheckSymbols()
    {
        for( SCH_ITEM* item : m_schematic->RootScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // Fix pre-8.0 legacy power symbols with invisible pins
            // that have mismatched pin names and value fields
            if( symbol->GetLibSymbolRef()
                && symbol->GetLibSymbolRef()->IsGlobalPower()
                && symbol->GetAllLibPins().size() > 0
                && symbol->GetAllLibPins()[0]->IsGlobalPower()
                && !symbol->GetAllLibPins()[0]->IsVisible() )
            {
                BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetText(),
                                   symbol->GetAllLibPins()[0]->GetName() );
            }
        }
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( LegacyPowerFixup, LEGACY_POWER_SYMBOLS_TEST_FIXTURE )
{
    KI_TEST::LoadSchematic( m_settingsManager, "netlists/legacy_power/legacy_power", m_schematic );

    CheckSymbols();
}


BOOST_FIXTURE_TEST_CASE( LegacyPower4Fixup, LEGACY_POWER_SYMBOLS_TEST_FIXTURE )
{
    KI_TEST::LoadSchematic( m_settingsManager, "netlists/legacy_power/legacy_power", m_schematic );

    CheckSymbols();
}
