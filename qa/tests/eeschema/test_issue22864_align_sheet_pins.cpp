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
 * Tests multiple scenarios:
 * - Select all items and align
 * - Select individual sheets
 * - Different grid spacings
 * - Verify all wire endpoints connect to pins after alignment
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


/**
 * Helper class to test grid alignment with configurable grid size.
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


struct ISSUE22864_FIXTURE
{
    ISSUE22864_FIXTURE() { }

    void LoadSchematic()
    {
        KI_TEST::LoadSchematic( m_settingsManager, "issue22864/Test_Move_Grid", m_schematic );
    }

    /**
     * Build a map of positions to connected wires for the callback.
     */
    std::map<VECTOR2I, std::vector<SCH_LINE*>> BuildPositionToWiresMap( SCH_SCREEN* aScreen )
    {
        std::map<VECTOR2I, std::vector<SCH_LINE*>> positionToWires;

        for( SCH_ITEM* item : aScreen->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* wire = static_cast<SCH_LINE*>( item );

            if( wire->GetLayer() != LAYER_WIRE )
                continue;

            positionToWires[wire->GetStartPoint()].push_back( wire );
            positionToWires[wire->GetEndPoint()].push_back( wire );
        }

        return positionToWires;
    }

    /**
     * Run alignment on selected items and verify wire connectivity.
     *
     * @param aScreen The schematic screen
     * @param aSelection Items to align
     * @param aGridSize Grid size in IU
     * @param aTestName Name for error messages
     * @return true if all wire endpoints connect to pins
     */
    bool RunAlignmentAndVerify( SCH_SCREEN* aScreen, std::vector<EDA_ITEM*>& aSelection,
                                const VECTOR2D& aGridSize, const std::string& aTestName )
    {
        TEST_GRID_HELPER grid( aGridSize );

        // Build position-to-wires map BEFORE alignment
        auto positionToWires = BuildPositionToWiresMap( aScreen );

        // Initialize items the same way AlignToGrid does in sch_move_tool.cpp (lines 2493-2508)
        // This sets storedPos and flags that the move logic depends on.
        std::set<EDA_ITEM*> selectionSet( aSelection.begin(), aSelection.end() );

        for( SCH_ITEM* it : aScreen->Items() )
        {
            if( selectionSet.find( it ) == selectionSet.end() )
                it->ClearFlags( STARTPOINT | ENDPOINT );
            else
                it->SetFlags( STARTPOINT | ENDPOINT );

            it->SetStoredPos( it->GetPosition() );

            if( it->Type() == SCH_SHEET_T )
            {
                for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( it )->GetPins() )
                    pin->SetStoredPos( pin->GetPosition() );
            }
        }

        SCH_ALIGNMENT_CALLBACKS callbacks;

        // Use the shared MoveSchematicItem function - the same logic used by production code
        callbacks.m_doMoveItem = []( EDA_ITEM* aItem, const VECTOR2I& aDelta )
        {
            MoveSchematicItem( aItem, aDelta );
        };

        callbacks.m_getConnectedDragItems =
                [&]( SCH_ITEM* aItem, const VECTOR2I& aPoint, EDA_ITEMS& aList )
                {
                    auto it = positionToWires.find( aPoint );

                    if( it != positionToWires.end() )
                    {
                        for( SCH_LINE* wire : it->second )
                        {
                            // Skip selected wires - they will be processed in their own
                            // SCH_LINE_T block. This matches the real getConnectedDragItems
                            // behavior in sch_move_tool.cpp.
                            if( wire->HasFlag( SELECTED ) )
                                continue;

                            if( std::find( aList.begin(), aList.end(), wire ) == aList.end() )
                            {
                                wire->SetFlags( STARTPOINT | ENDPOINT );

                                if( wire->GetStartPoint() == aPoint )
                                    wire->ClearFlags( ENDPOINT );
                                else
                                    wire->ClearFlags( STARTPOINT );

                                aList.push_back( wire );
                            }
                        }
                    }
                };

        // Run alignment
        AlignSchematicItemsToGrid( aScreen, aSelection, grid, GRID_CONNECTABLE, callbacks );

        // Collect all pin positions
        std::set<VECTOR2I> pinPositions;

        for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                pinPositions.insert( pin->GetPosition() );
        }

        // Verify all wire endpoints connect to pins
        bool allConnected = true;

        for( SCH_ITEM* item : aScreen->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* wire = static_cast<SCH_LINE*>( item );

            if( wire->GetLayer() != LAYER_WIRE )
                continue;

            VECTOR2I start = wire->GetStartPoint();
            VECTOR2I end = wire->GetEndPoint();

            bool startConnected = pinPositions.count( start ) > 0;
            bool endConnected = pinPositions.count( end ) > 0;

            if( !startConnected )
            {
                BOOST_TEST_MESSAGE( aTestName << ": Wire start (" << start.x << ", " << start.y
                                              << ") not connected to any pin" );

                for( const VECTOR2I& pinPos : pinPositions )
                {
                    BOOST_TEST_MESSAGE( "  Available pin at (" << pinPos.x << ", " << pinPos.y
                                                               << ")" );
                }

                allConnected = false;
            }

            if( !endConnected )
            {
                BOOST_TEST_MESSAGE( aTestName << ": Wire end (" << end.x << ", " << end.y
                                              << ") not connected to any pin" );

                for( const VECTOR2I& pinPos : pinPositions )
                {
                    BOOST_TEST_MESSAGE( "  Available pin at (" << pinPos.x << ", " << pinPos.y
                                                               << ")" );
                }

                allConnected = false;
            }
        }

        return allConnected;
    }

    /**
     * Run alignment and verify that wires are not skewed.
     * This is used for single-sheet tests where connectivity to unselected items may break,
     * but wires should remain straight (not skewed).
     *
     * @param aScreen the schematic screen
     * @param aSelection the items to align
     * @param aGridSize the grid size
     * @param aTestName name for debug output
     * @return true if no wires are skewed
     */
    bool RunAlignmentAndVerifyNoSkew( SCH_SCREEN* aScreen, const std::vector<EDA_ITEM*>& aSelection,
                                      const VECTOR2D& aGridSize, const std::string& aTestName )
    {
        std::set<EDA_ITEM*> selectionSet( aSelection.begin(), aSelection.end() );

        // Record original wire endpoints to check for skew
        std::map<SCH_LINE*, std::pair<VECTOR2I, VECTOR2I>> originalWireEndpoints;

        for( SCH_ITEM* it : aScreen->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* wire = static_cast<SCH_LINE*>( it );

            if( wire->GetLayer() == LAYER_WIRE )
                originalWireEndpoints[wire] = { wire->GetStartPoint(), wire->GetEndPoint() };
        }

        TEST_GRID_HELPER                             grid( aGridSize );
        std::map<VECTOR2I, std::vector<SCH_LINE*>> positionToWires =
                BuildPositionToWiresMap( aScreen );

        for( SCH_ITEM* it : aScreen->Items() )
        {
            if( selectionSet.find( it ) == selectionSet.end() )
                it->ClearFlags( STARTPOINT | ENDPOINT );
            else
                it->SetFlags( STARTPOINT | ENDPOINT );

            it->SetStoredPos( it->GetPosition() );

            if( it->Type() == SCH_SHEET_T )
            {
                for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( it )->GetPins() )
                    pin->SetStoredPos( pin->GetPosition() );
            }
        }

        SCH_ALIGNMENT_CALLBACKS callbacks;

        callbacks.m_doMoveItem = []( EDA_ITEM* aItem, const VECTOR2I& aDelta )
        {
            MoveSchematicItem( aItem, aDelta );
        };

        callbacks.m_getConnectedDragItems =
                [&]( SCH_ITEM* aItem, const VECTOR2I& aPoint, EDA_ITEMS& aList )
                {
                    auto it = positionToWires.find( aPoint );

                    if( it != positionToWires.end() )
                    {
                        for( SCH_LINE* wire : it->second )
                        {
                            if( wire->HasFlag( SELECTED ) )
                                continue;

                            if( std::find( aList.begin(), aList.end(), wire ) == aList.end() )
                            {
                                wire->SetFlags( STARTPOINT | ENDPOINT );

                                if( wire->GetStartPoint() == aPoint )
                                    wire->ClearFlags( ENDPOINT );
                                else
                                    wire->ClearFlags( STARTPOINT );

                                aList.push_back( wire );
                            }
                        }
                    }
                };

        // Run alignment
        AlignSchematicItemsToGrid( aScreen, aSelection, grid, GRID_CONNECTABLE, callbacks );

        // Check for wire skew - wires should maintain their original orientation
        bool noSkew = true;

        for( const auto& [wire, origEndpoints] : originalWireEndpoints )
        {
            VECTOR2I origStart = origEndpoints.first;
            VECTOR2I origEnd = origEndpoints.second;
            VECTOR2I newStart = wire->GetStartPoint();
            VECTOR2I newEnd = wire->GetEndPoint();

            // Check if wire was originally horizontal (same Y)
            bool wasHorizontal = ( origStart.y == origEnd.y );

            // Check if wire is still horizontal
            bool isHorizontal = ( newStart.y == newEnd.y );

            if( wasHorizontal && !isHorizontal )
            {
                BOOST_TEST_MESSAGE( aTestName << ": Wire became skewed! Original: ("
                                              << origStart.x << ", " << origStart.y << ") to ("
                                              << origEnd.x << ", " << origEnd.y << "), Now: ("
                                              << newStart.x << ", " << newStart.y << ") to ("
                                              << newEnd.x << ", " << newEnd.y << ")" );
                noSkew = false;
            }

            // Check if wire was originally vertical (same X)
            bool wasVertical = ( origStart.x == origEnd.x );
            bool isVertical = ( newStart.x == newEnd.x );

            if( wasVertical && !isVertical )
            {
                BOOST_TEST_MESSAGE( aTestName << ": Wire became skewed! Original: ("
                                              << origStart.x << ", " << origStart.y << ") to ("
                                              << origEnd.x << ", " << origEnd.y << "), Now: ("
                                              << newStart.x << ", " << newStart.y << ") to ("
                                              << newEnd.x << ", " << newEnd.y << ")" );
                noSkew = false;
            }
        }

        // Also check connectivity - collect all pin positions
        std::set<VECTOR2I> pinPositions;

        for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                pinPositions.insert( pin->GetPosition() );
        }

        bool allConnected = true;

        for( const auto& [wire, origEndpoints] : originalWireEndpoints )
        {
            VECTOR2I start = wire->GetStartPoint();
            VECTOR2I end = wire->GetEndPoint();

            bool startConnected = pinPositions.count( start ) > 0;
            bool endConnected = pinPositions.count( end ) > 0;

            if( !startConnected )
            {
                BOOST_TEST_MESSAGE( aTestName << ": Wire start (" << start.x << ", " << start.y
                                              << ") not connected to any pin" );
                allConnected = false;
            }

            if( !endConnected )
            {
                BOOST_TEST_MESSAGE( aTestName << ": Wire end (" << end.x << ", " << end.y
                                              << ") not connected to any pin" );
                allConnected = false;
            }
        }

        // Return true only if both no skew AND all connected
        return noSkew && allConnected;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_AUTO_TEST_SUITE( Issue22864AlignSheetPins )


/**
 * Test aligning all items with standard grid (2.54mm).
 */
BOOST_FIXTURE_TEST_CASE( AlignAllItemsStandardGrid, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    // Select all sheets
    std::vector<EDA_ITEM*> selection;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        selection.push_back( item );

    BOOST_REQUIRE_EQUAL( selection.size(), 2 );

    const VECTOR2D gridSize( schIUScale.mmToIU( 2.54 ), schIUScale.mmToIU( 2.54 ) );

    bool allConnected = RunAlignmentAndVerify( screen, selection, gridSize,
                                               "AlignAllItemsStandardGrid" );

    BOOST_CHECK_MESSAGE( allConnected, "All wire endpoints should connect to pins after "
                                       "aligning all items with 2.54mm grid" );
}


/**
 * Test aligning all items with fine grid (1.27mm).
 */
BOOST_FIXTURE_TEST_CASE( AlignAllItemsFineGrid, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    std::vector<EDA_ITEM*> selection;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        selection.push_back( item );

    const VECTOR2D gridSize( schIUScale.mmToIU( 1.27 ), schIUScale.mmToIU( 1.27 ) );

    bool allConnected = RunAlignmentAndVerify( screen, selection, gridSize,
                                               "AlignAllItemsFineGrid" );

    BOOST_CHECK_MESSAGE( allConnected, "All wire endpoints should connect to pins after "
                                       "aligning all items with 1.27mm grid" );
}


/**
 * Test aligning all items with coarse grid (5.08mm).
 */
BOOST_FIXTURE_TEST_CASE( AlignAllItemsCoarseGrid, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    std::vector<EDA_ITEM*> selection;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        selection.push_back( item );

    const VECTOR2D gridSize( schIUScale.mmToIU( 5.08 ), schIUScale.mmToIU( 5.08 ) );

    bool allConnected = RunAlignmentAndVerify( screen, selection, gridSize,
                                               "AlignAllItemsCoarseGrid" );

    BOOST_CHECK_MESSAGE( allConnected, "All wire endpoints should connect to pins after "
                                       "aligning all items with 5.08mm grid" );
}


/**
 * Test aligning only sheet1 with standard grid.
 * When only one sheet is selected, wires should stretch/shrink to maintain connectivity
 * without becoming skewed.
 */
BOOST_FIXTURE_TEST_CASE( AlignSheet1Only, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    SCH_SHEET* sheet1 = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        if( sheet->GetName() == "sheet1" )
        {
            sheet1 = sheet;
            break;
        }
    }

    BOOST_REQUIRE( sheet1 != nullptr );

    std::vector<EDA_ITEM*> selection{ sheet1 };
    const VECTOR2D gridSize( schIUScale.mmToIU( 2.54 ), schIUScale.mmToIU( 2.54 ) );

    bool success = RunAlignmentAndVerifyNoSkew( screen, selection, gridSize, "AlignSheet1Only" );
    BOOST_CHECK_MESSAGE( success, "Wires should remain straight and connected when aligning only sheet1" );
}


/**
 * Test aligning only sheet2 with standard grid.
 * When only one sheet is selected, wires should stretch/shrink to maintain connectivity
 * without becoming skewed.
 */
BOOST_FIXTURE_TEST_CASE( AlignSheet2Only, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    SCH_SHEET* sheet2 = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        if( sheet->GetName() == "sheet2" )
        {
            sheet2 = sheet;
            break;
        }
    }

    BOOST_REQUIRE( sheet2 != nullptr );

    std::vector<EDA_ITEM*> selection{ sheet2 };
    const VECTOR2D gridSize( schIUScale.mmToIU( 2.54 ), schIUScale.mmToIU( 2.54 ) );

    bool success = RunAlignmentAndVerifyNoSkew( screen, selection, gridSize, "AlignSheet2Only" );

    BOOST_CHECK_MESSAGE( success, "Wires should remain straight and connected when aligning only sheet2" );
}


/**
 * Test aligning sheet1 then sheet2 sequentially.
 * When sheets are aligned one at a time, wires should stretch/shrink to maintain connectivity
 * without becoming skewed.
 */
BOOST_FIXTURE_TEST_CASE( AlignSheet1ThenSheet2, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    SCH_SHEET* sheet1 = nullptr;
    SCH_SHEET* sheet2 = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        if( sheet->GetName() == "sheet1" )
            sheet1 = sheet;
        else if( sheet->GetName() == "sheet2" )
            sheet2 = sheet;
    }

    BOOST_REQUIRE( sheet1 != nullptr );
    BOOST_REQUIRE( sheet2 != nullptr );

    const VECTOR2D gridSize( schIUScale.mmToIU( 2.54 ), schIUScale.mmToIU( 2.54 ) );

    // First align sheet1
    std::vector<EDA_ITEM*> selection1{ sheet1 };
    bool success1 = RunAlignmentAndVerifyNoSkew( screen, selection1, gridSize,
                                                 "AlignSheet1ThenSheet2 (step 1)" );

    BOOST_CHECK_MESSAGE( success1, "Wires should remain straight and connected after aligning sheet1" );

    // Then align sheet2
    std::vector<EDA_ITEM*> selection2{ sheet2 };
    bool success2 = RunAlignmentAndVerifyNoSkew( screen, selection2, gridSize,
                                                 "AlignSheet1ThenSheet2 (step 2)" );

    BOOST_CHECK_MESSAGE( success2, "Wires should remain straight and connected after aligning sheet2" );
}


/**
 * Test aligning sheet2 then sheet1 sequentially.
 * When sheets are aligned one at a time, wires should stretch/shrink to maintain connectivity
 * without becoming skewed.
 */
BOOST_FIXTURE_TEST_CASE( AlignSheet2ThenSheet1, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    SCH_SHEET* sheet1 = nullptr;
    SCH_SHEET* sheet2 = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        if( sheet->GetName() == "sheet1" )
            sheet1 = sheet;
        else if( sheet->GetName() == "sheet2" )
            sheet2 = sheet;
    }

    BOOST_REQUIRE( sheet1 != nullptr );
    BOOST_REQUIRE( sheet2 != nullptr );

    const VECTOR2D gridSize( schIUScale.mmToIU( 2.54 ), schIUScale.mmToIU( 2.54 ) );

    // First align sheet2
    std::vector<EDA_ITEM*> selection2{ sheet2 };
    bool success2 = RunAlignmentAndVerifyNoSkew( screen, selection2, gridSize,
                                                 "AlignSheet2ThenSheet1 (step 1)" );

    BOOST_CHECK_MESSAGE( success2, "Wires should remain straight and connected after aligning sheet2" );

    // Then align sheet1
    std::vector<EDA_ITEM*> selection1{ sheet1 };
    bool success1 = RunAlignmentAndVerifyNoSkew( screen, selection1, gridSize,
                                                 "AlignSheet2ThenSheet1 (step 2)" );

    BOOST_CHECK_MESSAGE( success1, "Wires should remain straight and connected after aligning sheet1" );
}


/**
 * Test aligning with multiple different grid sizes in sequence.
 */
BOOST_FIXTURE_TEST_CASE( AlignMultipleGridSizes, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    std::vector<EDA_ITEM*> selection;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        selection.push_back( item );

    // Test with progressively finer grids
    std::vector<double> gridSizesMm = { 5.08, 2.54, 1.27, 0.635 };

    for( double gridMm : gridSizesMm )
    {
        // Reload schematic for fresh state
        LoadSchematic();
        screen = m_schematic->RootScreen();
        selection.clear();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
            selection.push_back( item );

        const VECTOR2D gridSize( schIUScale.mmToIU( gridMm ), schIUScale.mmToIU( gridMm ) );

        std::stringstream testName;
        testName << "AlignMultipleGridSizes (" << gridMm << "mm)";

        bool allConnected = RunAlignmentAndVerify( screen, selection, gridSize, testName.str() );

        BOOST_CHECK_MESSAGE( allConnected, "All wire endpoints should connect to pins with "
                                                   << gridMm << "mm grid" );
    }
}


/**
 * Test that aligning sheets preserves pin-to-wire Y coordinate match.
 */
BOOST_FIXTURE_TEST_CASE( VerifyPinWireYCoordinateMatch, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    std::vector<EDA_ITEM*> selection;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        selection.push_back( item );

    const VECTOR2D gridSize( schIUScale.mmToIU( 2.54 ), schIUScale.mmToIU( 2.54 ) );
    TEST_GRID_HELPER grid( gridSize );

    auto positionToWires = BuildPositionToWiresMap( screen );

    SCH_ALIGNMENT_CALLBACKS callbacks;

    callbacks.m_doMoveItem = []( EDA_ITEM* aItem, const VECTOR2I& aDelta )
    {
        if( aItem->Type() == SCH_SHEET_T )
            static_cast<SCH_SHEET*>( aItem )->Move( aDelta );
        else
            MoveSchematicItem( aItem, aDelta );
    };

    callbacks.m_getConnectedDragItems =
            [&]( SCH_ITEM* aItem, const VECTOR2I& aPoint, EDA_ITEMS& aList )
            {
                auto it = positionToWires.find( aPoint );

                if( it != positionToWires.end() )
                {
                    for( SCH_LINE* wire : it->second )
                    {
                        if( std::find( aList.begin(), aList.end(), wire ) == aList.end() )
                        {
                            wire->SetFlags( STARTPOINT | ENDPOINT );

                            if( wire->GetStartPoint() == aPoint )
                                wire->ClearFlags( ENDPOINT );
                            else
                                wire->ClearFlags( STARTPOINT );

                            aList.push_back( wire );
                        }
                    }
                }
            };

    AlignSchematicItemsToGrid( screen, selection, grid, GRID_CONNECTABLE, callbacks );

    // For each sheet, verify each pin has a wire endpoint at the same position
    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        for( SCH_SHEET_PIN* pin : sheet->GetPins() )
        {
            VECTOR2I pinPos = pin->GetPosition();
            bool foundWire = false;

            for( SCH_ITEM* wireItem : screen->Items().OfType( SCH_LINE_T ) )
            {
                SCH_LINE* wire = static_cast<SCH_LINE*>( wireItem );

                if( wire->GetLayer() != LAYER_WIRE )
                    continue;

                if( wire->GetStartPoint() == pinPos || wire->GetEndPoint() == pinPos )
                {
                    foundWire = true;
                    break;
                }
            }

            BOOST_CHECK_MESSAGE( foundWire,
                                 "Pin at (" << pinPos.x << ", " << pinPos.y
                                            << ") should have a connected wire endpoint" );
        }
    }
}


/**
 * Test aligning when both sheets AND wires are selected together.
 * This is the critical case from issue #22864 - when "Select All" includes wires,
 * the wires must still follow their connected sheet pins.
 */
BOOST_FIXTURE_TEST_CASE( AlignSheetsAndWiresTogether, ISSUE22864_FIXTURE )
{
    LOCALE_IO dummy;
    LoadSchematic();

    SCH_SCREEN* screen = m_schematic->RootScreen();
    BOOST_REQUIRE( screen != nullptr );

    // Select ALL sheets AND wires (simulates Ctrl+A or Select All)
    std::vector<EDA_ITEM*> selection;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
    {
        item->SetFlags( SELECTED );
        selection.push_back( item );
    }

    for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
    {
        SCH_LINE* wire = static_cast<SCH_LINE*>( item );

        if( wire->GetLayer() == LAYER_WIRE )
        {
            wire->SetFlags( SELECTED );
            selection.push_back( wire );
        }
    }

    BOOST_REQUIRE_GE( selection.size(), 4 );  // At least 2 sheets + 2 wires

    const VECTOR2D gridSize( schIUScale.mmToIU( 2.54 ), schIUScale.mmToIU( 2.54 ) );

    bool allConnected = RunAlignmentAndVerify( screen, selection, gridSize,
                                               "AlignSheetsAndWiresTogether" );

    BOOST_CHECK_MESSAGE( allConnected,
                         "All wire endpoints should connect to pins after aligning "
                         "both sheets AND wires together (Select All scenario)" );
}


/**
 * Test aligning sheets and wires together with multiple grid sizes.
 */
BOOST_FIXTURE_TEST_CASE( AlignSheetsAndWiresMultipleGrids, ISSUE22864_FIXTURE )
{
    std::vector<double> gridSizesMm = { 5.08, 2.54, 1.27, 0.635 };

    for( double gridMm : gridSizesMm )
    {
        LOCALE_IO dummy;
        LoadSchematic();

        SCH_SCREEN* screen = m_schematic->RootScreen();
        BOOST_REQUIRE( screen != nullptr );

        // Select ALL sheets AND wires
        std::vector<EDA_ITEM*> selection;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        {
            item->SetFlags( SELECTED );
            selection.push_back( item );
        }

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* wire = static_cast<SCH_LINE*>( item );

            if( wire->GetLayer() == LAYER_WIRE )
            {
                wire->SetFlags( SELECTED );
                selection.push_back( wire );
            }
        }

        const VECTOR2D gridSize( schIUScale.mmToIU( gridMm ), schIUScale.mmToIU( gridMm ) );

        std::stringstream testName;
        testName << "AlignSheetsAndWiresMultipleGrids (" << gridMm << "mm)";

        bool allConnected = RunAlignmentAndVerify( screen, selection, gridSize, testName.str() );

        BOOST_CHECK_MESSAGE( allConnected,
                             "All wire endpoints should connect to pins with "
                                     << gridMm << "mm grid when sheets AND wires are selected" );
    }
}


BOOST_AUTO_TEST_SUITE_END()
