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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connectivity/conn_pin_name.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityPinName )

BOOST_AUTO_TEST_CASE( NamesUseUnitsButPadOnlyNamesDoNot )
{
    PIN_NAME_FACT pin{ "IN", "3", "3", "3" };
    PIN_NAME_REFERENCE ref{ "U2", "U2B", "symbol-id" };
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref ), "Net-(U2B-IN)" );
    pin.name = "3";
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref ), "Net-(U2-Pad3)" );
    pin.name.clear();
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref ), "Net-(U2-Pad3)" );
}

BOOST_AUTO_TEST_CASE( DuplicateNamesAndNoConnectUsePadSuffix )
{
    PIN_NAME_FACT pin{ "IN", "3", "3", "3" };
    PIN_NAME_REFERENCE ref{ "U2", "U2B", "symbol-id" };
    pin.hasDuplicateName = true;
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref ), "Net-(U2B-IN-Pad3)" );
    pin.hasDuplicateName = false;
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref, true ), "unconnected-(U2B-IN-Pad3)" );
    pin.noConnect = true;
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref ), "unconnected-(U2B-IN-Pad3)" );
}

BOOST_AUTO_TEST_CASE( EscapingAndStackedPadSelection )
{
    PIN_NAME_FACT pin{ "A/B\n", "[10,2]", "[10,2]", "2" };
    PIN_NAME_REFERENCE ref{ "U2", "U2B", "symbol-id" };
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref, true ), "unconnected-(U2B-A{slash}B-Pad2)" );
    pin.padNumber = "2/3";
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref, true ), "unconnected-(U2B-A{slash}B-Pad2{slash}3)" );
}

BOOST_AUTO_TEST_CASE( UnannotatedAndMissingReferencesUseUuid )
{
    PIN_NAME_FACT pin{ "IN", "[10,2]", "[10,2]", "2" };
    PIN_NAME_REFERENCE ref{ "U?", "U?B", "00000000-0000-0000-0000-000000000001" };
    const wxString expected = wxString::Format( "Net-(U?-%08x-Pad2)",
                                               static_cast<unsigned>( KIID( ref.symbolUuid ).Hash() & 0xFFFFFFFF ) );
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref ), expected );
    ref.symbolUuid = "symbol-id";
    ref.reference.clear();
    ref.referenceWithUnit.clear();
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref, true ), "unconnected-(symbol-id-Pad2)" );
    pin.number = "3/4";
    pin.padNumber = pin.shownNumber;
    BOOST_CHECK_EQUAL( RenderPinNetName( pin, ref ), "Net-(symbol-id-Pad3/4)" );
}

BOOST_AUTO_TEST_CASE( OriginalAlternateFixtureUsesLibraryNumberAndReplacesCachedNcName )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue22286/bugtest", schematic );
    const SCH_SHEET_PATH path = schematic->Hierarchy()[0];
    SCH_PIN* target = nullptr;

    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &path ) != "J1" )
            continue;

        for( SCH_PIN* pin : symbol->GetPins( &path ) )
        {
            if( pin->GetNumber() == "8" )
                target = pin;
        }
    }

    BOOST_REQUIRE( target );
    BOOST_CHECK_EQUAL( target->GetShownName(), "8.pow" );
    BOOST_CHECK_EQUAL( target->GetDefaultNetName( path ), "Net-(J1-Pad8)" );
    BOOST_CHECK_EQUAL( target->GetDefaultNetName( path, true ), "unconnected-(J1-Pad8)" );
    BOOST_CHECK_EQUAL( target->GetDefaultNetName( path ), "Net-(J1-Pad8)" );
}

BOOST_AUTO_TEST_SUITE_END()
