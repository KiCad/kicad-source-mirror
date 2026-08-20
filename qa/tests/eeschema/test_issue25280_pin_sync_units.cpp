/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Tests for SYMBOL_EDITOR_PIN_TOOL::SynchronizeOtherUnits(), the synchronized pins mode
 * propagation that runs after the pin properties dialog is accepted (issue #25280).
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <tools/symbol_editor_pin_tool.h>

#include <wx/filename.h>

#include <memory>


/**
 * Load 74LS125, a quad tri-state buffer whose four interchangeable units carry identically
 * placed pins plus a common power unit.  Its units are not locked, which is what enables
 * synchronized pins mode in the symbol editor.
 */
static std::unique_ptr<LIB_SYMBOL> loadQuadBuffer()
{
    wxFileName libPath( KI_TEST::GetEeschemaTestDataDir() );
    libPath.AppendDir( "variants" );
    libPath.SetFullName( "pic_programmer.kicad_sym" );

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    // The plugin cache owns the returned symbol, so hand back a copy
    LIB_SYMBOL* cached = pi->LoadSymbol( libPath.GetFullPath(), wxS( "74LS125" ) );

    return cached ? std::make_unique<LIB_SYMBOL>( *cached ) : nullptr;
}


/**
 * The input pin of the first gate, i.e. one of the pins the other three gates replicate.
 */
static SCH_PIN* firstGateInput( LIB_SYMBOL& aSymbol )
{
    for( SCH_PIN* pin : aSymbol.GetPins() )
    {
        if( pin->GetUnit() == 1 && pin->GetType() == ELECTRICAL_PINTYPE::PT_INPUT
            && pin->GetOrientation() == PIN_ORIENTATION::PIN_RIGHT )
        {
            return pin;
        }
    }

    return nullptr;
}


/**
 * Pins of the other units that the edit is expected to subsume: same position, orientation,
 * type, visibility and name as the pin before the edit.
 */
static int countMatchingPins( LIB_SYMBOL& aSymbol, const SCH_PIN& aPin )
{
    int count = 0;

    for( SCH_PIN* pin : aSymbol.GetPins() )
    {
        if( pin != &aPin && pin->GetUnit() != aPin.GetUnit()
            && pin->GetPosition() == aPin.GetPosition()
            && pin->GetOrientation() == aPin.GetOrientation()
            && pin->GetType() == aPin.GetType() && pin->IsVisible() == aPin.IsVisible()
            && pin->GetName() == aPin.GetName() )
        {
            count++;
        }
    }

    return count;
}


BOOST_AUTO_TEST_SUITE( Issue25280PinSyncUnits )


/**
 * Making a pin common to all units deletes the pins it subsumes in the other units.  The
 * deleted pins must not be read back or deleted a second time.
 */
BOOST_AUTO_TEST_CASE( CommonToAllUnitsRemovesSubsumedPins )
{
    std::unique_ptr<LIB_SYMBOL> symbol = loadQuadBuffer();
    BOOST_REQUIRE( symbol );

    SCH_PIN* edited = firstGateInput( *symbol );
    BOOST_REQUIRE( edited );

    const VECTOR2I pinPos = edited->GetPosition();
    const size_t   pinsBefore = symbol->GetPins().size();
    const int      expectedRemovals = countMatchingPins( *symbol, *edited );

    BOOST_REQUIRE_GT( expectedRemovals, 0 );

    // The dialog copies the pin before the edit and writes unit 0 when "Common to all units
    // in symbol" is checked.
    SCH_PIN originalPin( *edited );
    edited->SetUnit( 0 );

    int removed = SYMBOL_EDITOR_PIN_TOOL::SynchronizeOtherUnits( symbol.get(), edited,
                                                                 originalPin );

    BOOST_CHECK_EQUAL( removed, expectedRemovals );
    BOOST_CHECK_EQUAL( symbol->GetPins().size(),
                       pinsBefore - static_cast<size_t>( expectedRemovals ) );

    // The edited pin survives, and no unit kept a duplicate of it.
    int survivorsAtPos = 0;

    for( SCH_PIN* pin : symbol->GetPins() )
    {
        BOOST_CHECK( pin->GetParentSymbol() == symbol.get() );

        if( pin->GetPosition() == pinPos )
            survivorsAtPos++;
    }

    BOOST_CHECK_EQUAL( survivorsAtPos, 1 );
    BOOST_CHECK_EQUAL( edited->GetUnit(), 0 );
}


/**
 * A pin that stays bound to its own unit propagates its properties to the matching pins of
 * the other units without deleting any of them.
 */
BOOST_AUTO_TEST_CASE( PerUnitEditPropagatesWithoutRemoving )
{
    std::unique_ptr<LIB_SYMBOL> symbol = loadQuadBuffer();
    BOOST_REQUIRE( symbol );

    SCH_PIN* edited = firstGateInput( *symbol );
    BOOST_REQUIRE( edited );

    const VECTOR2I pinPos = edited->GetPosition();
    const size_t   pinsBefore = symbol->GetPins().size();
    const int      expectedMatches = countMatchingPins( *symbol, *edited );

    BOOST_REQUIRE_GT( expectedMatches, 0 );

    SCH_PIN originalPin( *edited );

    edited->SetName( wxS( "SYNCED" ) );
    edited->SetNameTextSize( edited->GetNameTextSize() + schIUScale.MilsToIU( 10 ) );

    int removed = SYMBOL_EDITOR_PIN_TOOL::SynchronizeOtherUnits( symbol.get(), edited,
                                                                 originalPin );

    BOOST_CHECK_EQUAL( removed, 0 );
    BOOST_CHECK_EQUAL( symbol->GetPins().size(), pinsBefore );

    int propagated = 0;

    for( SCH_PIN* pin : symbol->GetPins() )
    {
        if( pin == edited || pin->GetPosition() != pinPos )
            continue;

        BOOST_CHECK_EQUAL( pin->GetName(), edited->GetName() );
        BOOST_CHECK_EQUAL( pin->GetNameTextSize(), edited->GetNameTextSize() );
        propagated++;
    }

    BOOST_CHECK_EQUAL( propagated, expectedMatches );
}


BOOST_AUTO_TEST_SUITE_END()
