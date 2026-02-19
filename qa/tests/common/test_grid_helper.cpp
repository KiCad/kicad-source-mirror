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


BOOST_AUTO_TEST_CASE( AlignGridWithNonPageOrigin )
{
    // Issue #21800: Grid snapping should produce positions that are exact multiples
    // of the grid size relative to the grid origin, regardless of display origin setting.
    // When grid sizes go through VECTOR2D, floating-point imprecision in the
    // VECTOR2D -> VECTOR2I truncation could produce incorrect grid positions.

    GRID_HELPER helper;
    helper.SetGridSnapping( true );

    // PCB IU_PER_MM = 1000000 (nanometers)
    constexpr int IU_PER_MM = 1000000;

    struct TestCase
    {
        const char* name;
        double      gridSizeMM;
        int         gridOriginX;
        int         gridOriginY;
        int         pointX;
        int         pointY;
        int         expectedX;
        int         expectedY;
    };

    // Test various grid sizes, including ones that don't convert exactly from mm to nm
    std::vector<TestCase> cases = {
        // 1mm grid with non-zero origin
        { "1mm grid, origin at 47.3mm",
          1.0, 47300000, 25300000,
          127400000, 95400000,
          0, 0 },

        // 0.5mm grid
        { "0.5mm grid, origin at 47.3mm",
          0.5, 47300000, 25300000,
          127400000, 95400000,
          0, 0 },

        // 0.1mm grid (0.1 is NOT exact in IEEE 754 double)
        { "0.1mm grid, origin at 47.3mm",
          0.1, 47300000, 25300000,
          127340000, 95340000,
          0, 0 },

        // 25 mil = 0.635mm (0.635 is NOT exact in IEEE 754 double)
        { "25mil grid, origin at 47.625mm",
          0.635, 47625000, 25400000,
          127960000, 95250000,
          0, 0 },

        // 10 mil = 0.254mm (0.254 is NOT exact in IEEE 754 double)
        { "10mil grid, origin at 47.752mm",
          0.254, 47752000, 25400000,
          127960000, 95250000,
          0, 0 },

        // 50 mil = 1.27mm
        { "50mil grid, origin at 47.625mm",
          1.27, 47625000, 25400000,
          127960000, 95250000,
          0, 0 },
    };

    // Compute all expected values using integer grid size (the ground truth)
    for( auto& tc : cases )
    {
        int gridSizeIU = KiROUND( tc.gridSizeMM * IU_PER_MM );

        tc.expectedX = KiROUND( double( tc.pointX - tc.gridOriginX ) / gridSizeIU )
                       * gridSizeIU + tc.gridOriginX;
        tc.expectedY = KiROUND( double( tc.pointY - tc.gridOriginY ) / gridSizeIU )
                       * gridSizeIU + tc.gridOriginY;
    }

    for( const auto& tc : cases )
    {
        BOOST_TEST_CONTEXT( tc.name )
        {
            int gridSizeIU = KiROUND( tc.gridSizeMM * IU_PER_MM );

            // Simulate the grid size coming through VECTOR2D (as it does from GAL)
            VECTOR2D gridD( tc.gridSizeMM * IU_PER_MM, tc.gridSizeMM * IU_PER_MM );
            VECTOR2D offsetD( tc.gridOriginX, tc.gridOriginY );

            helper.SetGridSize( gridD );
            helper.SetOrigin( VECTOR2I( tc.gridOriginX, tc.gridOriginY ) );

            VECTOR2I point( tc.pointX, tc.pointY );

            // Test AlignGrid with VECTOR2D parameters (the path that goes through
            // implicit VECTOR2D -> VECTOR2I truncation in computeNearest)
            VECTOR2I resultD = helper.AlignGrid( point, gridD, offsetD );

            // Test AlignGrid with no parameters (uses GetGrid() which rounds properly)
            VECTOR2I resultI = helper.AlignGrid( point );

            BOOST_CHECK_EQUAL( resultD.x, tc.expectedX );
            BOOST_CHECK_EQUAL( resultD.y, tc.expectedY );
            BOOST_CHECK_EQUAL( resultI.x, tc.expectedX );
            BOOST_CHECK_EQUAL( resultI.y, tc.expectedY );

            // Verify that the result is on-grid
            BOOST_CHECK_EQUAL( ( resultD.x - tc.gridOriginX ) % gridSizeIU, 0 );
            BOOST_CHECK_EQUAL( ( resultD.y - tc.gridOriginY ) % gridSizeIU, 0 );
        }
    }
}


BOOST_AUTO_TEST_CASE( TruncationVsRoundingForGridSizes )
{
    // Test that VECTOR2D -> VECTOR2I truncation gives the same result as rounding
    // for all standard KiCad grid sizes (defined in app_settings.cpp).
    // If truncation differs from rounding, computeNearest gets a wrong grid size.

    constexpr int IU_PER_MM = 1000000;

    // All standard grid sizes from KiCad's default grid list (in mm)
    std::vector<double> gridSizesMM = {
        0.001, 0.0025, 0.005, 0.01, 0.025, 0.05,
        0.1, 0.2, 0.25, 0.5,
        1.0, 2.0, 2.5, 5.0, 10.0, 25.0, 50.0,
        // Imperial equivalents (mils to mm)
        0.0254,   // 1 mil
        0.0508,   // 2 mil
        0.1,      // ~3.937 mil
        0.127,    // 5 mil
        0.254,    // 10 mil
        0.508,    // 20 mil
        0.635,    // 25 mil
        1.27,     // 50 mil
        2.54,     // 100 mil
    };

    int failCount = 0;

    for( double gridMM : gridSizesMM )
    {
        double gridD = gridMM * IU_PER_MM;
        int gridTruncated = static_cast<int>( gridD );
        int gridRounded = KiROUND( gridD );

        if( gridTruncated != gridRounded )
        {
            BOOST_TEST_MESSAGE( "Grid " << gridMM << "mm: double=" << gridD
                               << " truncated=" << gridTruncated
                               << " rounded=" << gridRounded
                               << " MISMATCH" );
            failCount++;
        }
    }

    BOOST_CHECK_EQUAL( failCount, 0 );
}


BOOST_AUTO_TEST_CASE( DisplayOriginDoesNotAffectSnapping )
{
    // Verify that grid snapping produces the same result regardless of what
    // display origin is used. The display origin only affects how coordinates
    // are shown to the user, not where items snap.

    GRID_HELPER helper;
    helper.SetGridSnapping( true );

    constexpr int IU_PER_MM = 1000000;

    // Grid: 1mm, origin at (47.3mm, 25.3mm)
    int gridSize = 1 * IU_PER_MM;
    VECTOR2I gridOrigin( 47300000, 25300000 );
    VECTOR2D gridD( gridSize, gridSize );
    VECTOR2D offsetD( gridOrigin );

    helper.SetGridSize( gridD );
    helper.SetOrigin( gridOrigin );

    VECTOR2I testPoint( 127400000, 95400000 );

    // Snap the point
    VECTOR2I snapped = helper.AlignGrid( testPoint, gridD, offsetD );

    // Expected: nearest grid point is 127300000, 95300000
    BOOST_CHECK_EQUAL( snapped.x, 127300000 );
    BOOST_CHECK_EQUAL( snapped.y, 95300000 );

    // Verify the position is on-grid
    BOOST_CHECK_EQUAL( ( snapped.x - gridOrigin.x ) % gridSize, 0 );
    BOOST_CHECK_EQUAL( ( snapped.y - gridOrigin.y ) % gridSize, 0 );

    // Simulate the display path. With display origin = grid origin, the displayed
    // value should be an exact multiple of the grid size.
    long long displayX = static_cast<long long>( snapped.x ) - gridOrigin.x;
    double displayMM = static_cast<double>( displayX ) / IU_PER_MM;

    // The displayed value should be exactly 80.0 mm
    BOOST_CHECK_EQUAL( displayMM, 80.0 );

    // Verify with different display origins
    VECTOR2I auxOrigin( 30000000, 20000000 );
    long long displayFromAux = static_cast<long long>( snapped.x ) - auxOrigin.x;
    double displayFromAuxMM = static_cast<double>( displayFromAux ) / IU_PER_MM;

    // Should be exactly 97.3 mm
    BOOST_CHECK_CLOSE( displayFromAuxMM, 97.3, 1e-10 );
}


BOOST_AUTO_TEST_CASE( DragMovementPreservesGridAlignment )
{
    // Simulate a drag operation: the movement delta between two grid-aligned
    // cursor positions should keep the item on-grid.

    GRID_HELPER helper;
    helper.SetGridSnapping( true );

    constexpr int IU_PER_MM = 1000000;

    // Grid: 1mm, origin at (47.3mm, 25.3mm)
    int gridSize = 1 * IU_PER_MM;
    VECTOR2I gridOrigin( 47300000, 25300000 );
    VECTOR2D gridD( gridSize, gridSize );
    VECTOR2D offsetD( gridOrigin );

    helper.SetGridSize( gridD );
    helper.SetOrigin( gridOrigin );

    // Footprint starts at a grid-aligned position
    VECTOR2I fpPosition( 127300000, 95300000 );
    BOOST_CHECK_EQUAL( ( fpPosition.x - gridOrigin.x ) % gridSize, 0 );
    BOOST_CHECK_EQUAL( ( fpPosition.y - gridOrigin.y ) % gridSize, 0 );

    // Simulate several drag steps
    struct DragStep
    {
        VECTOR2I mousePos;
    };

    std::vector<DragStep> steps = {
        { { 128500000, 96500000 } },
        { { 129200000, 97100000 } },
        { { 130800000, 98900000 } },
        { { 131300000, 99300000 } },
    };

    VECTOR2I prevCursor = fpPosition;

    for( const auto& step : steps )
    {
        // Snap cursor to grid
        VECTOR2I cursor = helper.AlignGrid( step.mousePos, gridD, offsetD );

        // Compute movement
        VECTOR2I movement = cursor - prevCursor;

        // Apply movement
        fpPosition += movement;
        prevCursor = cursor;

        // Verify footprint is still on-grid after the move
        BOOST_CHECK_EQUAL( ( fpPosition.x - gridOrigin.x ) % gridSize, 0 );
        BOOST_CHECK_EQUAL( ( fpPosition.y - gridOrigin.y ) % gridSize, 0 );

        // Verify display with grid origin shows a clean integer mm value
        long long displayX = static_cast<long long>( fpPosition.x ) - gridOrigin.x;
        double displayMM = static_cast<double>( displayX ) / IU_PER_MM;

        // Should be an exact integer (no fractional mm)
        BOOST_CHECK_EQUAL( displayMM, std::floor( displayMM ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()