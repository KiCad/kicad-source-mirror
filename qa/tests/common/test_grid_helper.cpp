/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <tool/grid_helper.h>

void TEST_CLEAR_ANCHORS( GRID_HELPER& helper )
{
    helper.clearAnchors();
}

BOOST_AUTO_TEST_SUITE( GridHelperTest )

BOOST_AUTO_TEST_CASE( DefaultConstructor )
{
    GRID_HELPER helper;

    // Test default state
    BOOST_CHECK( helper.GetSnap() );
    BOOST_CHECK( helper.GetUseGrid() );

    // Test that manual setters work
    helper.SetGridSize( VECTOR2D( 100, 100 ) );
    helper.SetOrigin( VECTOR2I( 50, 50 ) );
    helper.SetGridSnapping( true );

    VECTOR2I grid = helper.GetGrid();
    BOOST_CHECK_EQUAL( grid.x, 100 );
    BOOST_CHECK_EQUAL( grid.y, 100 );

    VECTOR2I origin = helper.GetOrigin();
    BOOST_CHECK_EQUAL( origin.x, 50 );
    BOOST_CHECK_EQUAL( origin.y, 50 );
}

BOOST_AUTO_TEST_CASE( AlignBasic )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 100, 100 ) );
    helper.SetOrigin( VECTOR2I( 0, 0 ) );
    helper.SetGridSnapping( true );

    // Test basic alignment - should round to nearest grid point
    VECTOR2I aligned = helper.Align( VECTOR2I( 149, 251 ) );
    BOOST_CHECK_EQUAL( aligned.x, 100 );
    BOOST_CHECK_EQUAL( aligned.y, 300 );

    // Test exact grid points
    aligned = helper.Align( VECTOR2I( 200, 300 ) );
    BOOST_CHECK_EQUAL( aligned.x, 200 );
    BOOST_CHECK_EQUAL( aligned.y, 300 );

    // Test negative coordinates
    aligned = helper.Align( VECTOR2I( -149, -251 ) );
    BOOST_CHECK_EQUAL( aligned.x, -100 );
    BOOST_CHECK_EQUAL( aligned.y, -300 );
}

BOOST_AUTO_TEST_CASE( AlignGridWithCustomGrid )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 50, 50 ) );
    helper.SetOrigin( VECTOR2I( 0, 0 ) );
    helper.SetGridSnapping( true );

    VECTOR2I aligned = helper.AlignGrid( VECTOR2I( 26, 74 ) );
    BOOST_CHECK_EQUAL( aligned.x, 50 );
    BOOST_CHECK_EQUAL( aligned.y, 50 );

    // Test AlignGrid with specific grid parameter
    aligned = helper.AlignGrid( VECTOR2I( 26, 74 ), VECTOR2D( 25, 25 ), VECTOR2D( 0, 0 ) );
    BOOST_CHECK_EQUAL( aligned.x, 25 );
    BOOST_CHECK_EQUAL( aligned.y, 75 );
}

BOOST_AUTO_TEST_CASE( AlignWithOriginOffset )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 100, 100 ) );
    helper.SetOrigin( VECTOR2I( 25, 25 ) );
    helper.SetGridSnapping( true );

    // When grid has an origin offset, alignment should work from the new reference point
    VECTOR2I aligned = helper.AlignGrid( VECTOR2I( 149, 251 ) );
    BOOST_CHECK_EQUAL( aligned.x, 125 );
    BOOST_CHECK_EQUAL( aligned.y, 225 );
}

BOOST_AUTO_TEST_CASE( AlignWithAuxiliaryAxes )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 100, 100 ) );
    helper.SetOrigin( VECTOR2I( 0, 0 ) );
    helper.SetGridSnapping( true );

    // Set auxiliary axis at (75, 75)
    helper.SetAuxAxes( true, VECTOR2I( 75, 75 ) );

    // Point closer to aux axis than grid should snap to aux axis
    VECTOR2I aligned = helper.Align( VECTOR2I( 80, 80 ) );
    BOOST_CHECK_EQUAL( aligned.x, 75 );  // Closer to aux axis X
    BOOST_CHECK_EQUAL( aligned.y, 75 );  // Closer to aux axis Y

    // Point closer to grid than aux axis should snap to grid
    aligned = helper.Align( VECTOR2I( 95, 95 ) );
    BOOST_CHECK_EQUAL( aligned.x, 100 );  // Closer to grid
    BOOST_CHECK_EQUAL( aligned.y, 100 );  // Closer to grid

    // Disable aux axes
    helper.SetAuxAxes( false );
    aligned = helper.Align( VECTOR2I( 80, 80 ) );
    BOOST_CHECK_EQUAL( aligned.x, 100 );  // Should snap to grid only
    BOOST_CHECK_EQUAL( aligned.y, 100 );
}

BOOST_AUTO_TEST_CASE( GridSnappingDisabled )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 100, 100 ) );
    helper.SetOrigin( VECTOR2I( 0, 0 ) );
    helper.SetGridSnapping( false );  // Disable grid snapping

    // When grid snapping is disabled, Align should return original point
    VECTOR2I original( 149, 251 );
    VECTOR2I aligned = helper.Align( original );
    BOOST_CHECK_EQUAL( aligned.x, original.x );
    BOOST_CHECK_EQUAL( aligned.y, original.y );

    // AlignGrid should still work regardless of grid snapping setting
    aligned = helper.AlignGrid( original );
    BOOST_CHECK_EQUAL( aligned.x, 100 );
    BOOST_CHECK_EQUAL( aligned.y, 300 );
}

BOOST_AUTO_TEST_CASE( UseGridDisabled )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 100, 100 ) );
    helper.SetOrigin( VECTOR2I( 0, 0 ) );
    helper.SetGridSnapping( true );
    helper.SetUseGrid( false );  // Disable grid usage

    // When grid usage is disabled, Align should return original point
    VECTOR2I original( 149, 251 );
    VECTOR2I aligned = helper.Align( original );
    BOOST_CHECK_EQUAL( aligned.x, original.x );
    BOOST_CHECK_EQUAL( aligned.y, original.y );
}

BOOST_AUTO_TEST_CASE( AsymmetricGrid )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 25, 75 ) );  // Different X and Y grid sizes
    helper.SetOrigin( VECTOR2I( 0, 0 ) );
    helper.SetGridSnapping( true );

    VECTOR2I aligned = helper.Align( VECTOR2I( 30, 100 ) );
    BOOST_CHECK_EQUAL( aligned.x, 25 );   // Nearest 25-unit boundary
    BOOST_CHECK_EQUAL( aligned.y, 75 );   // Nearest 75-unit boundary

    aligned = helper.Align( VECTOR2I( 40, 120 ) );
    BOOST_CHECK_EQUAL( aligned.x, 50 );   // Next 25-unit boundary
    BOOST_CHECK_EQUAL( aligned.y, 150 );  // Next 75-unit boundary
}

BOOST_AUTO_TEST_CASE( SnapFlags )
{
    GRID_HELPER helper;

    // Test snap flag getters/setters
    BOOST_CHECK( helper.GetSnap() );  // Default should be true

    helper.SetSnap( false );
    BOOST_CHECK( !helper.GetSnap() );

    helper.SetSnap( true );
    BOOST_CHECK( helper.GetSnap() );

    // Test grid usage flag
    BOOST_CHECK( helper.GetUseGrid() );  // Default should be true

    helper.SetUseGrid( false );
    BOOST_CHECK( !helper.GetUseGrid() );

    helper.SetUseGrid( true );
    BOOST_CHECK( helper.GetUseGrid() );
}

BOOST_AUTO_TEST_CASE( MaskOperations )
{
    GRID_HELPER helper;

    // Test mask operations
    helper.SetMask( GRID_HELPER::CORNER | GRID_HELPER::OUTLINE );
    helper.SetMaskFlag( GRID_HELPER::SNAPPABLE );
    helper.ClearMaskFlag( GRID_HELPER::CORNER );

    // These don't have getters, so we can't verify the mask state directly
    // but we can verify the methods don't crash
}

BOOST_AUTO_TEST_CASE( SkipPoint )
{
    GRID_HELPER helper;

    // Test skip point operations
    helper.SetSkipPoint( VECTOR2I( 100, 100 ) );
    helper.ClearSkipPoint();

    // These methods should not crash
}

BOOST_AUTO_TEST_CASE( GridTypeAlignment )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 100, 100 ) );
    helper.SetOrigin( VECTOR2I( 0, 0 ) );
    helper.SetGridSnapping( true );

    // Test alignment with specific grid type
    VECTOR2I aligned = helper.Align( VECTOR2I( 149, 251 ), GRID_CURRENT );
    BOOST_CHECK_EQUAL( aligned.x, 100 );
    BOOST_CHECK_EQUAL( aligned.y, 300 );

    aligned = helper.AlignGrid( VECTOR2I( 149, 251 ), GRID_CURRENT );
    BOOST_CHECK_EQUAL( aligned.x, 100 );
    BOOST_CHECK_EQUAL( aligned.y, 300 );
}

BOOST_AUTO_TEST_CASE( EdgeCases )
{
    GRID_HELPER helper;
    helper.SetGridSize( VECTOR2D( 1, 1 ) );  // Very small grid
    helper.SetOrigin( VECTOR2I( 0, 0 ) );
    helper.SetGridSnapping( true );

    // Test with very small grid
    VECTOR2I aligned = helper.Align( VECTOR2I( 5, 5 ) );
    BOOST_CHECK_EQUAL( aligned.x, 5 );
    BOOST_CHECK_EQUAL( aligned.y, 5 );

    // Test with zero point
    aligned = helper.Align( VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( aligned.x, 0 );
    BOOST_CHECK_EQUAL( aligned.y, 0 );

    // Test with large grid
    helper.SetGridSize( VECTOR2D( 10000, 10000 ) );
    aligned = helper.Align( VECTOR2I( 3000, 7000 ) );
    BOOST_CHECK_EQUAL( aligned.x, 0 );     // Closer to 0 than 10000
    BOOST_CHECK_EQUAL( aligned.y, 10000 ); // Closer to 10000 than 0
}

BOOST_AUTO_TEST_CASE( GetGridSize )
{
    GRID_HELPER helper;

    // Test GetGridSize with different grid types
    helper.SetGridSize( VECTOR2D( 50, 75 ) );

    VECTOR2D gridSize = helper.GetGridSize( GRID_CURRENT );
    BOOST_CHECK_EQUAL( gridSize.x, 50 );
    BOOST_CHECK_EQUAL( gridSize.y, 75 );

    // Other grid types should return the same in the base implementation
    gridSize = helper.GetGridSize( GRID_CONNECTABLE );
    BOOST_CHECK_EQUAL( gridSize.x, 50 );
    BOOST_CHECK_EQUAL( gridSize.y, 75 );
}

BOOST_AUTO_TEST_CASE( VisibleGrid )
{
    GRID_HELPER helper;
    helper.SetVisibleGridSize( VECTOR2D( 25, 35 ) );

    VECTOR2D visibleGrid = helper.GetVisibleGrid();
    BOOST_CHECK_EQUAL( visibleGrid.x, 25 );
    BOOST_CHECK_EQUAL( visibleGrid.y, 35 );
}

BOOST_AUTO_TEST_CASE( SnapPointManagement )
{
    GRID_HELPER helper;

    // Initially should have no snapped point
    auto snappedPoint = helper.GetSnappedPoint();
    BOOST_CHECK( !snappedPoint.has_value() );

    // After clearing anchors, still no snapped point
    TEST_CLEAR_ANCHORS( helper );
    snappedPoint = helper.GetSnappedPoint();
    BOOST_CHECK( !snappedPoint.has_value() );
}

BOOST_AUTO_TEST_SUITE_END()