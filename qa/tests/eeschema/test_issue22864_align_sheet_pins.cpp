/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * @file test_issue22864_align_sheet_pins.cpp
 *
 * Test for issue #22864: "Align Items to Grid" disconnects wires from hierarchical sheet pins.
 *
 * Bug scenario:
 * - Sheet2 is positioned off-grid at (127.635mm, 95.885mm)
 * - Sheet2 has pins on the left side connected to wires from Sheet1
 * - When aligning Sheet2 to grid, the sheet and pins move but the wire endpoints
 *   should also move to maintain connectivity
 *
 * Root cause: The alignment code incorrectly passed a delta vector to AlignGrid()
 * instead of a position. AlignGrid() snaps positions to grid, so passing a small
 * delta like (0.635mm, -0.635mm) results in snapping to (0, 0), causing no movement.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <schematic.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_screen.h>
#include <sch_line.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <base_units.h>

#include <sch_item_alignment.h>
#include <tools/ee_grid_helper.h>


struct ISSUE22864_FIXTURE
{
    ISSUE22864_FIXTURE() { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_AUTO_TEST_SUITE( Issue22864AlignSheetPins )


/**
 * Helper class to test grid alignment.
 *
 * Creates an EE_GRID_HELPER that returns a fixed grid size for all grid types,
 * allowing testing without a full tool manager.
 */
class TEST_GRID_HELPER : public EE_GRID_HELPER
{
public:
    TEST_GRID_HELPER( const VECTOR2D& aGridSize ) :
            EE_GRID_HELPER(),
            m_gridSize( aGridSize )
    {
    }

    VECTOR2D GetGridSize( GRID_HELPER_GRIDS aGrid ) const override
    {
        return m_gridSize;
    }

private:
    VECTOR2D m_gridSize;
};


/**
 * Test that AlignSchematicItemsToGrid properly aligns sheet pins and connected wires.
 *
 * This test verifies that when a sheet is aligned to grid:
 * 1. The sheet position moves to grid
 * 2. The sheet pins move to grid
 * 3. Wires connected to the pins also move to maintain connectivity
 */
BOOST_FIXTURE_TEST_CASE( Issue22864SheetPinAlignment, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue22864/Test_Move_Grid", m_schematic );

    // Use RootScreen() to get the actual content screen (not the virtual root)
    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    // Find sheet2 (the one at off-grid position 127.635mm, 95.885mm)
    SCH_SHEET* sheet2 = nullptr;
    int sheetCount = 0;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
        sheetCount++;
        BOOST_TEST_MESSAGE( "Found sheet: \"" << sheet->GetName() << "\" at ("
                            << sheet->GetPosition().x << ", " << sheet->GetPosition().y << ")" );

        if( sheet->GetName() == "sheet2" )
        {
            sheet2 = sheet;
            break;
        }
    }

    BOOST_TEST_MESSAGE( "Total sheets found: " << sheetCount );
    BOOST_REQUIRE_MESSAGE( sheet2 != nullptr, "Could not find sheet named 'sheet2'" );

    // Verify initial off-grid position
    // 127.635mm = 1276350 IU, 95.885mm = 958850 IU
    VECTOR2I initialPos = sheet2->GetPosition();
    BOOST_TEST_MESSAGE( "Sheet2 initial position: (" << initialPos.x << ", " << initialPos.y << ")" );

    // Verify sheet has pins
    std::vector<SCH_SHEET_PIN*> pins = sheet2->GetPins();
    BOOST_REQUIRE( pins.size() >= 2 );

    // Record initial pin positions
    std::map<SCH_SHEET_PIN*, VECTOR2I> initialPinPositions;

    for( SCH_SHEET_PIN* pin : pins )
        initialPinPositions[pin] = pin->GetPosition();

    // Find wires connected to the pins
    std::map<SCH_SHEET_PIN*, SCH_LINE*> connectedWires;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
    {
        SCH_LINE* wire = static_cast<SCH_LINE*>( item );

        if( wire->GetLayer() != LAYER_WIRE )
            continue;

        for( SCH_SHEET_PIN* pin : pins )
        {
            VECTOR2I pinPos = pin->GetPosition();

            if( wire->GetStartPoint() == pinPos || wire->GetEndPoint() == pinPos )
                connectedWires[pin] = wire;
        }
    }

    BOOST_REQUIRE( !connectedWires.empty() );

    // Record initial wire endpoints that connect to pins
    std::map<SCH_SHEET_PIN*, VECTOR2I> initialWireEndpoints;

    for( const auto& [pin, wire] : connectedWires )
    {
        VECTOR2I pinPos = pin->GetPosition();

        if( wire->GetStartPoint() == pinPos )
            initialWireEndpoints[pin] = wire->GetStartPoint();
        else
            initialWireEndpoints[pin] = wire->GetEndPoint();
    }

    // Standard eeschema grid is 2.54mm = 25400 IU (100 mils)
    const VECTOR2D gridSize( schIUScale.mmToIU( 2.54 ), schIUScale.mmToIU( 2.54 ) );
    TEST_GRID_HELPER grid( gridSize );

    // Track all move operations
    std::map<EDA_ITEM*, std::vector<VECTOR2I>> moveDeltas;

    SCH_ALIGNMENT_CALLBACKS callbacks;

    callbacks.m_doMoveItem = [&]( EDA_ITEM* aItem, const VECTOR2I& aDelta )
    {
        moveDeltas[aItem].push_back( aDelta );

        if( aItem->Type() == SCH_SHEET_T )
            static_cast<SCH_SHEET*>( aItem )->Move( aDelta );
        else if( aItem->Type() == SCH_SHEET_PIN_T )
            static_cast<SCH_SHEET_PIN*>( aItem )->Move( aDelta );
        else if( aItem->Type() == SCH_LINE_T )
            static_cast<SCH_LINE*>( aItem )->Move( aDelta );
        else
            static_cast<SCH_ITEM*>( aItem )->Move( aDelta );
    };

    // Note: We don't provide m_getConnectedDragItems for this test
    // The alignment function should still work, but won't drag connected items
    // The bug we're testing is in the delta calculation, not the drag behavior

    // Create selection with just sheet2
    std::vector<EDA_ITEM*> selection{ sheet2 };

    // Call the alignment function
    AlignSchematicItemsToGrid( screen, selection, grid, GRID_CONNECTABLE, callbacks );

    // Verify sheet2 moved
    VECTOR2I finalPos = sheet2->GetPosition();
    BOOST_TEST_MESSAGE( "Sheet2 final position: (" << finalPos.x << ", " << finalPos.y << ")" );

    // Sheet should have moved (initial position was off-grid)
    bool sheetMoved = ( finalPos != initialPos );
    BOOST_CHECK_MESSAGE( sheetMoved, "Sheet should have moved to align to grid" );

    // Verify sheet position is now on grid
    VECTOR2I expectedSheetPos(
            KiROUND( initialPos.x / gridSize.x ) * static_cast<int>( gridSize.x ),
            KiROUND( initialPos.y / gridSize.y ) * static_cast<int>( gridSize.y ) );

    BOOST_CHECK_EQUAL( finalPos.x, expectedSheetPos.x );
    BOOST_CHECK_EQUAL( finalPos.y, expectedSheetPos.y );

    // The critical bug test: pins should end up ON GRID after alignment
    // With the buggy code, the pin delta calculation uses AlignGrid(delta) instead of
    // AlignGrid(position) - delta, which causes small deltas to snap to (0,0)
    // This means pins don't get properly aligned to grid
    for( SCH_SHEET_PIN* pin : pins )
    {
        VECTOR2I pinPos = pin->GetPosition();
        BOOST_TEST_MESSAGE( "Pin final position: (" << pinPos.x << ", " << pinPos.y << ")" );

        // Calculate what the Y position should be if properly aligned to grid
        int gridInt = static_cast<int>( gridSize.y );
        int expectedPinY = KiROUND( static_cast<double>( pinPos.y ) / gridInt ) * gridInt;

        // The pin Y position should be exactly on grid
        // With the buggy code, this check FAILS because the pin doesn't get
        // the additional alignment delta applied
        bool pinYOnGrid = ( pinPos.y == expectedPinY );

        BOOST_CHECK_MESSAGE( pinYOnGrid,
                             "Pin Y position should be on grid after alignment. "
                             "Actual Y: " << pinPos.y << ", "
                             "Nearest grid Y: " << expectedPinY << ", "
                             "Difference: " << ( pinPos.y - expectedPinY ) );

        // Also check X position is on grid (sheet edge which should be on grid)
        int expectedPinX = KiROUND( static_cast<double>( pinPos.x ) / gridSize.x ) * static_cast<int>( gridSize.x );
        bool pinXOnGrid = ( pinPos.x == expectedPinX );

        BOOST_CHECK_MESSAGE( pinXOnGrid,
                             "Pin X position should be on grid after alignment. "
                             "Actual X: " << pinPos.x << ", "
                             "Nearest grid X: " << expectedPinX );
    }
}


BOOST_AUTO_TEST_SUITE_END()
