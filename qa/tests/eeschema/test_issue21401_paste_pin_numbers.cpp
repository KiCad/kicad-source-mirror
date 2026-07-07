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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_issue21401_paste_pin_numbers.cpp
 *
 * Test for issue #21401: pasting a symbol with modified pin numbering into a project that
 * already contains the original (unmodified) version of the same library symbol must preserve
 * the modified pin numbers from the clipboard.
 *
 * The reproduction is clipboard-driven and only fully exercisable through the GUI Paste action,
 * which the eeschema unit-test harness cannot drive (no frame/tool manager/clipboard). The bug,
 * however, lives entirely in which cached library symbol the paste selects, so these tests drive
 * the production selection helper SCH_EDITOR_CONTROL::ChoosePasteLibSymbol directly and confirm
 * the chosen definition survives SCH_SYMBOL::SetLibSymbol() without losing the copied pin numbers.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>
#include <set>

#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_screen.h>
#include <tools/sch_editor_control.h>


namespace
{

LIB_SYMBOL* MakeTestLibSymbol( const wxString& aName, const wxString& aPin1Num,
                               const wxString& aPin2Num )
{
    LIB_SYMBOL* sym = new LIB_SYMBOL( aName, nullptr );
    sym->SetLibId( LIB_ID( "testlib", aName ) );

    SCH_PIN* pin1 = new SCH_PIN( sym );
    pin1->SetNumber( aPin1Num );
    pin1->SetName( "pin_a" );
    pin1->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
    pin1->SetPosition( VECTOR2I( 0, 0 ) );
    sym->AddDrawItem( pin1 );

    SCH_PIN* pin2 = new SCH_PIN( sym );
    pin2->SetNumber( aPin2Num );
    pin2->SetName( "pin_b" );
    pin2->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
    pin2->SetPosition( VECTOR2I( 0, schIUScale.MilsToIU( 100 ) ) );
    sym->AddDrawItem( pin2 );

    return sym;
}


std::set<wxString> PinNumbers( SCH_SYMBOL& aSymbol, const SCH_SHEET_PATH& aPath )
{
    std::set<wxString> nums;

    for( SCH_PIN* pin : aSymbol.GetPins( &aPath ) )
        nums.insert( pin->GetNumber() );

    return nums;
}

}  // anonymous namespace


/**
 * The core bug mechanism: SetLibSymbol()/UpdatePins() drives pin numbers from the chosen lib
 * symbol. If paste selects a lib symbol whose pin numbers differ from the pasted instance, the
 * instance's modified numbers are clobbered. This documents why selecting the wrong cache reverts
 * the edits and is the failure mode the fix avoids.
 */
BOOST_AUTO_TEST_CASE( Issue21401_WrongLibSymbolRevertsPinNumbers )
{
    std::unique_ptr<LIB_SYMBOL> modifiedLib( MakeTestLibSymbol( "TestPart", "A1", "A2" ) );

    SCH_SHEET_PATH path;
    SCH_SYMBOL symbol( *modifiedLib, modifiedLib->GetLibId(), &path, 0, 0, VECTOR2I( 0, 0 ) );
    symbol.UpdatePins();

    BOOST_REQUIRE_EQUAL( symbol.GetPins( &path ).size(), 2 );
    BOOST_CHECK( PinNumbers( symbol, path ) == ( std::set<wxString>{ "A1", "A2" } ) );

    // Applying the original (unmodified) definition reverts the numbering.
    symbol.SetLibSymbol( MakeTestLibSymbol( "TestPart", "1", "2" ) );

    BOOST_CHECK( PinNumbers( symbol, path ) == ( std::set<wxString>{ "1", "2" } ) );
}


/**
 * Issue #21401: with both clipboard and destination caching the same-named symbol, the paste
 * helper must pick the clipboard copy so its modified pin numbers survive the paste.
 */
BOOST_AUTO_TEST_CASE( Issue21401_PastePrefersClipboardLibSymbol )
{
    SCH_SCREEN destScreen;
    destScreen.AddLibSymbol( MakeTestLibSymbol( "TestPart", "1", "2" ) );

    SCH_SCREEN clipScreen;
    clipScreen.AddLibSymbol( MakeTestLibSymbol( "TestPart", "A1", "A2" ) );

    std::unique_ptr<LIB_SYMBOL> clipLib( MakeTestLibSymbol( "TestPart", "A1", "A2" ) );

    SCH_SHEET_PATH path;
    SCH_SYMBOL symbol( *clipLib, clipLib->GetLibId(), &path, 0, 0, VECTOR2I( 0, 0 ) );
    symbol.UpdatePins();

    const LIB_SYMBOL* source = SCH_EDITOR_CONTROL::ChoosePasteLibSymbol(
            &clipScreen, &destScreen, symbol.GetSchSymbolLibraryName() );

    BOOST_REQUIRE( source );

    symbol.SetLibSymbol( new LIB_SYMBOL( *source ) );

    BOOST_CHECK_MESSAGE( PinNumbers( symbol, path ) == ( std::set<wxString>{ "A1", "A2" } ),
                         "Paste must keep the clipboard's modified pin numbers" );
}


/**
 * Regression guard for issue #22162: the clipboard copy must still win when the destination caches
 * a same-named symbol with a different power type, so the pasted symbol keeps its power type.
 */
BOOST_AUTO_TEST_CASE( Issue22162_PastePrefersClipboardPowerType )
{
    LIB_SYMBOL* destPower = MakeTestLibSymbol( "PWR", "1", "1" );
    destPower->SetGlobalPower();

    SCH_SCREEN destScreen;
    destScreen.AddLibSymbol( destPower );

    LIB_SYMBOL* clipPower = MakeTestLibSymbol( "PWR", "1", "1" );
    clipPower->SetLocalPower();

    SCH_SCREEN clipScreen;
    clipScreen.AddLibSymbol( clipPower );

    const wxString    lookupName = clipPower->GetLibId().Format().wx_str();
    const LIB_SYMBOL* source = SCH_EDITOR_CONTROL::ChoosePasteLibSymbol( &clipScreen, &destScreen,
                                                                         lookupName );

    BOOST_REQUIRE( source );
    BOOST_CHECK_MESSAGE( source->IsLocalPower(),
                         "Paste must keep the clipboard symbol's local power type" );
}


/**
 * When the clipboard carries no cached copy (e.g. legacy clipboard content), the helper falls back
 * to the destination cache.
 */
BOOST_AUTO_TEST_CASE( Issue21401_FallsBackToDestinationWhenClipboardEmpty )
{
    SCH_SCREEN destScreen;
    LIB_SYMBOL* destLib = MakeTestLibSymbol( "TestPart", "1", "2" );
    destScreen.AddLibSymbol( destLib );

    SCH_SCREEN clipScreen;  // no cached symbol

    const wxString    lookupName = destLib->GetLibId().Format().wx_str();
    const LIB_SYMBOL* source = SCH_EDITOR_CONTROL::ChoosePasteLibSymbol( &clipScreen, &destScreen,
                                                                         lookupName );

    BOOST_CHECK_EQUAL( source, destLib );
}
