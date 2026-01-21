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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>
#include <drc/rule_editor/drc_re_overlay_types.h>

#include <algorithm>
#include <vector>

#include <wx/gdicmn.h>


/**
 * Helper functions that mirror the logic from DRC_RE_BITMAP_OVERLAY_PANEL.
 * These allow testing the mathematical operations without requiring wxWidgets GUI.
 */
namespace BITMAP_OVERLAY_TEST_HELPERS
{

/**
 * Compute the scale factor from a raw content scale factor.
 * Mirrors GetScaleFactor() logic from DRC_RE_BITMAP_OVERLAY_PANEL.
 */
double ComputeScaleFactor( double aRawScale )
{
    if( aRawScale < 1.25 )
        return 1.0;
    else if( aRawScale < 1.75 )
        return 1.5;
    else
        return 2.0;
}


/**
 * Scale a position from 1x bitmap coordinates to display coordinates.
 * Mirrors ScalePosition() logic from DRC_RE_BITMAP_OVERLAY_PANEL.
 */
wxPoint ScalePosition( int aX, int aY, double aScaleFactor )
{
    return wxPoint( static_cast<int>( aX * aScaleFactor ), static_cast<int>( aY * aScaleFactor ) );
}


/**
 * Scale a size from 1x bitmap coordinates to display coordinates.
 * Mirrors ScaleSize() logic from DRC_RE_BITMAP_OVERLAY_PANEL.
 */
wxSize ScaleSize( int aWidth, int aHeight, double aScaleFactor )
{
    return wxSize( static_cast<int>( aWidth * aScaleFactor ), static_cast<int>( aHeight * aScaleFactor ) );
}


/**
 * Check if a field position exceeds bitmap bounds.
 * Mirrors bounds check logic from PositionFields() in DRC_RE_BITMAP_OVERLAY_PANEL.
 */
bool IsOutOfBounds( const wxPoint& aScaledPos, const wxSize& aScaledSize, const wxSize& aBitmapSize )
{
    return ( aScaledPos.x + aScaledSize.GetWidth() > aBitmapSize.GetWidth() ||
             aScaledPos.y + aScaledSize.GetHeight() > aBitmapSize.GetHeight() );
}


/**
 * Sort field positions by tab order.
 * Mirrors expected keyboard navigation order.
 */
void SortByTabOrder( std::vector<DRC_RE_FIELD_POSITION>& aPositions )
{
    std::sort( aPositions.begin(), aPositions.end(),
               []( const DRC_RE_FIELD_POSITION& a, const DRC_RE_FIELD_POSITION& b )
               {
                   return a.tabOrder < b.tabOrder;
               } );
}

}  // namespace BITMAP_OVERLAY_TEST_HELPERS


BOOST_AUTO_TEST_SUITE( DrcBitmapOverlayPanel )


// ============================================================================
// T025: ScalePosition Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( ScalePosition_1x )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    double scale = 1.0;

    // Test various positions at 1x scale
    wxPoint result1 = ScalePosition( 0, 0, scale );
    BOOST_CHECK_EQUAL( result1.x, 0 );
    BOOST_CHECK_EQUAL( result1.y, 0 );

    wxPoint result2 = ScalePosition( 100, 50, scale );
    BOOST_CHECK_EQUAL( result2.x, 100 );
    BOOST_CHECK_EQUAL( result2.y, 50 );

    wxPoint result3 = ScalePosition( 256, 128, scale );
    BOOST_CHECK_EQUAL( result3.x, 256 );
    BOOST_CHECK_EQUAL( result3.y, 128 );

    // Test edge case with odd numbers
    wxPoint result4 = ScalePosition( 127, 63, scale );
    BOOST_CHECK_EQUAL( result4.x, 127 );
    BOOST_CHECK_EQUAL( result4.y, 63 );
}


BOOST_AUTO_TEST_CASE( ScalePosition_1_5x )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    double scale = 1.5;

    // Test various positions at 1.5x scale
    wxPoint result1 = ScalePosition( 0, 0, scale );
    BOOST_CHECK_EQUAL( result1.x, 0 );
    BOOST_CHECK_EQUAL( result1.y, 0 );

    wxPoint result2 = ScalePosition( 100, 50, scale );
    BOOST_CHECK_EQUAL( result2.x, 150 );
    BOOST_CHECK_EQUAL( result2.y, 75 );

    wxPoint result3 = ScalePosition( 200, 100, scale );
    BOOST_CHECK_EQUAL( result3.x, 300 );
    BOOST_CHECK_EQUAL( result3.y, 150 );

    // Test truncation behavior with odd numbers
    wxPoint result4 = ScalePosition( 127, 63, scale );
    BOOST_CHECK_EQUAL( result4.x, static_cast<int>( 127 * 1.5 ) );  // 190
    BOOST_CHECK_EQUAL( result4.y, static_cast<int>( 63 * 1.5 ) );   // 94
}


BOOST_AUTO_TEST_CASE( ScalePosition_2x )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    double scale = 2.0;

    // Test various positions at 2x scale
    wxPoint result1 = ScalePosition( 0, 0, scale );
    BOOST_CHECK_EQUAL( result1.x, 0 );
    BOOST_CHECK_EQUAL( result1.y, 0 );

    wxPoint result2 = ScalePosition( 100, 50, scale );
    BOOST_CHECK_EQUAL( result2.x, 200 );
    BOOST_CHECK_EQUAL( result2.y, 100 );

    wxPoint result3 = ScalePosition( 256, 128, scale );
    BOOST_CHECK_EQUAL( result3.x, 512 );
    BOOST_CHECK_EQUAL( result3.y, 256 );

    // Test that coordinates double exactly
    wxPoint result4 = ScalePosition( 127, 63, scale );
    BOOST_CHECK_EQUAL( result4.x, 254 );
    BOOST_CHECK_EQUAL( result4.y, 126 );
}


BOOST_AUTO_TEST_CASE( ScaleSize_AllFactors )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    // Test size scaling at all three scale factors
    int width = 80;
    int height = 24;

    wxSize size1x = ScaleSize( width, height, 1.0 );
    BOOST_CHECK_EQUAL( size1x.GetWidth(), 80 );
    BOOST_CHECK_EQUAL( size1x.GetHeight(), 24 );

    wxSize size1_5x = ScaleSize( width, height, 1.5 );
    BOOST_CHECK_EQUAL( size1_5x.GetWidth(), 120 );
    BOOST_CHECK_EQUAL( size1_5x.GetHeight(), 36 );

    wxSize size2x = ScaleSize( width, height, 2.0 );
    BOOST_CHECK_EQUAL( size2x.GetWidth(), 160 );
    BOOST_CHECK_EQUAL( size2x.GetHeight(), 48 );
}


// ============================================================================
// T026: GetScaleFactor Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( GetScaleFactor_Returns1x_ForLowDPI )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    // Values below 1.25 should return 1.0
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.0 ), 1.0 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.1 ), 1.0 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.24 ), 1.0 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.249 ), 1.0 );

    // Edge case at boundary (1.25 should NOT return 1.0)
    BOOST_CHECK( ComputeScaleFactor( 1.25 ) != 1.0 );
}


BOOST_AUTO_TEST_CASE( GetScaleFactor_Returns1_5x_ForMediumDPI )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    // Values at or above 1.25 but below 1.75 should return 1.5
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.25 ), 1.5 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.5 ), 1.5 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.74 ), 1.5 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.749 ), 1.5 );

    // Edge case at boundary (1.75 should NOT return 1.5)
    BOOST_CHECK( ComputeScaleFactor( 1.75 ) != 1.5 );
}


BOOST_AUTO_TEST_CASE( GetScaleFactor_Returns2x_ForHighDPI )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    // Values at or above 1.75 should return 2.0
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.75 ), 2.0 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 2.0 ), 2.0 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 2.5 ), 2.0 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 3.0 ), 2.0 );
}


BOOST_AUTO_TEST_CASE( GetScaleFactor_BoundaryValues )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    // Test exact boundary values
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.24999 ), 1.0 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.25 ), 1.5 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.74999 ), 1.5 );
    BOOST_CHECK_EQUAL( ComputeScaleFactor( 1.75 ), 2.0 );
}


// ============================================================================
// T027: Tab Order Sorting Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( TabOrderSorting_AlreadySorted )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    std::vector<DRC_RE_FIELD_POSITION> fields = {
        { 10, 50, 20, 1 },   // tabOrder = 1
        { 60, 100, 20, 2 },  // tabOrder = 2
        { 110, 150, 20, 3 }  // tabOrder = 3
    };

    SortByTabOrder( fields );

    BOOST_CHECK_EQUAL( fields[0].tabOrder, 1 );
    BOOST_CHECK_EQUAL( fields[1].tabOrder, 2 );
    BOOST_CHECK_EQUAL( fields[2].tabOrder, 3 );
}


BOOST_AUTO_TEST_CASE( TabOrderSorting_ReversedOrder )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    std::vector<DRC_RE_FIELD_POSITION> fields = {
        { 10, 50, 20, 3 },   // tabOrder = 3
        { 60, 100, 20, 2 },  // tabOrder = 2
        { 110, 150, 20, 1 }  // tabOrder = 1
    };

    SortByTabOrder( fields );

    BOOST_CHECK_EQUAL( fields[0].tabOrder, 1 );
    BOOST_CHECK_EQUAL( fields[1].tabOrder, 2 );
    BOOST_CHECK_EQUAL( fields[2].tabOrder, 3 );

    // Verify the associated coordinates moved with the tab order
    BOOST_CHECK_EQUAL( fields[0].xStart, 110 );  // Was originally tabOrder 1
    BOOST_CHECK_EQUAL( fields[1].xStart, 60 );   // Was originally tabOrder 2
    BOOST_CHECK_EQUAL( fields[2].xStart, 10 );   // Was originally tabOrder 3
}


BOOST_AUTO_TEST_CASE( TabOrderSorting_MixedOrder )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    std::vector<DRC_RE_FIELD_POSITION> fields = {
        { 10, 50, 20, 5 },
        { 60, 100, 40, 1 },
        { 110, 150, 60, 3 },
        { 160, 200, 80, 2 },
        { 210, 250, 100, 4 }
    };

    SortByTabOrder( fields );

    BOOST_CHECK_EQUAL( fields[0].tabOrder, 1 );
    BOOST_CHECK_EQUAL( fields[1].tabOrder, 2 );
    BOOST_CHECK_EQUAL( fields[2].tabOrder, 3 );
    BOOST_CHECK_EQUAL( fields[3].tabOrder, 4 );
    BOOST_CHECK_EQUAL( fields[4].tabOrder, 5 );
}


BOOST_AUTO_TEST_CASE( TabOrderSorting_DuplicateOrders )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    // Duplicate tab orders should maintain relative position (stable sort behavior)
    std::vector<DRC_RE_FIELD_POSITION> fields = {
        { 10, 50, 20, 2 },
        { 60, 100, 40, 1 },
        { 110, 150, 60, 2 },  // Duplicate tabOrder = 2
        { 160, 200, 80, 1 }   // Duplicate tabOrder = 1
    };

    SortByTabOrder( fields );

    // All tabOrder 1 fields should come before tabOrder 2 fields
    BOOST_CHECK_EQUAL( fields[0].tabOrder, 1 );
    BOOST_CHECK_EQUAL( fields[1].tabOrder, 1 );
    BOOST_CHECK_EQUAL( fields[2].tabOrder, 2 );
    BOOST_CHECK_EQUAL( fields[3].tabOrder, 2 );
}


BOOST_AUTO_TEST_CASE( TabOrderSorting_SingleField )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    std::vector<DRC_RE_FIELD_POSITION> fields = { { 10, 50, 20, 1 } };

    SortByTabOrder( fields );

    BOOST_CHECK_EQUAL( fields.size(), 1 );
    BOOST_CHECK_EQUAL( fields[0].tabOrder, 1 );
}


BOOST_AUTO_TEST_CASE( TabOrderSorting_EmptyVector )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    std::vector<DRC_RE_FIELD_POSITION> fields;

    SortByTabOrder( fields );

    BOOST_CHECK( fields.empty() );
}


// ============================================================================
// T028: Bounds Warning Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( BoundsWarning_WithinBounds )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    wxSize bitmapSize( 300, 200 );

    // Field completely within bounds
    wxPoint pos1( 10, 10 );
    wxSize size1( 80, 24 );
    BOOST_CHECK( !IsOutOfBounds( pos1, size1, bitmapSize ) );

    // Field at edge of bounds (exactly fitting)
    wxPoint pos2( 220, 176 );
    wxSize size2( 80, 24 );
    BOOST_CHECK( !IsOutOfBounds( pos2, size2, bitmapSize ) );

    // Field at origin
    wxPoint pos3( 0, 0 );
    wxSize size3( 100, 50 );
    BOOST_CHECK( !IsOutOfBounds( pos3, size3, bitmapSize ) );
}


BOOST_AUTO_TEST_CASE( BoundsWarning_ExceedsWidth )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    wxSize bitmapSize( 300, 200 );

    // Field exceeds right edge
    wxPoint pos( 250, 10 );
    wxSize size( 80, 24 );

    BOOST_CHECK( IsOutOfBounds( pos, size, bitmapSize ) );
}


BOOST_AUTO_TEST_CASE( BoundsWarning_ExceedsHeight )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    wxSize bitmapSize( 300, 200 );

    // Field exceeds bottom edge
    wxPoint pos( 10, 180 );
    wxSize size( 80, 30 );

    BOOST_CHECK( IsOutOfBounds( pos, size, bitmapSize ) );
}


BOOST_AUTO_TEST_CASE( BoundsWarning_ExceedsBothDimensions )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    wxSize bitmapSize( 300, 200 );

    // Field exceeds both edges
    wxPoint pos( 250, 180 );
    wxSize size( 80, 30 );

    BOOST_CHECK( IsOutOfBounds( pos, size, bitmapSize ) );
}


BOOST_AUTO_TEST_CASE( BoundsWarning_FieldLargerThanBitmap )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    wxSize bitmapSize( 300, 200 );

    // Field larger than entire bitmap
    wxPoint pos( 0, 0 );
    wxSize size( 400, 300 );

    BOOST_CHECK( IsOutOfBounds( pos, size, bitmapSize ) );
}


BOOST_AUTO_TEST_CASE( BoundsWarning_AtScaledPositions )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    // Simulate a field at 1x coordinates that fits, but may not fit when scaled
    int baseX = 250;
    int baseY = 180;
    int baseWidth = 40;
    int baseHeight = 15;

    // At 1x, bitmap is 300x200
    wxSize bitmap1x( 300, 200 );
    wxPoint pos1x = ScalePosition( baseX, baseY, 1.0 );
    wxSize size1x = ScaleSize( baseWidth, baseHeight, 1.0 );
    BOOST_CHECK( !IsOutOfBounds( pos1x, size1x, bitmap1x ) );

    // At 1.5x, bitmap becomes 450x300
    wxSize bitmap1_5x( 450, 300 );
    wxPoint pos1_5x = ScalePosition( baseX, baseY, 1.5 );
    wxSize size1_5x = ScaleSize( baseWidth, baseHeight, 1.5 );
    BOOST_CHECK( !IsOutOfBounds( pos1_5x, size1_5x, bitmap1_5x ) );

    // At 2x, bitmap becomes 600x400
    wxSize bitmap2x( 600, 400 );
    wxPoint pos2x = ScalePosition( baseX, baseY, 2.0 );
    wxSize size2x = ScaleSize( baseWidth, baseHeight, 2.0 );
    BOOST_CHECK( !IsOutOfBounds( pos2x, size2x, bitmap2x ) );
}


BOOST_AUTO_TEST_CASE( BoundsWarning_EdgeCases )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    wxSize bitmapSize( 300, 200 );

    // Field exactly at boundary (should be within bounds)
    wxPoint posExact( 220, 176 );
    wxSize sizeExact( 80, 24 );
    BOOST_CHECK( !IsOutOfBounds( posExact, sizeExact, bitmapSize ) );

    // Field 1 pixel over width
    wxPoint posOverWidth( 221, 176 );
    BOOST_CHECK( IsOutOfBounds( posOverWidth, sizeExact, bitmapSize ) );

    // Field 1 pixel over height
    wxPoint posOverHeight( 220, 177 );
    BOOST_CHECK( IsOutOfBounds( posOverHeight, sizeExact, bitmapSize ) );
}


BOOST_AUTO_TEST_CASE( BoundsWarning_ZeroSizedField )
{
    using namespace BITMAP_OVERLAY_TEST_HELPERS;

    wxSize bitmapSize( 300, 200 );

    // Zero-sized field should never be out of bounds
    wxPoint pos( 0, 0 );
    wxSize zeroSize( 0, 0 );
    BOOST_CHECK( !IsOutOfBounds( pos, zeroSize, bitmapSize ) );

    // Zero-sized field at edge
    wxPoint posEdge( 300, 200 );
    BOOST_CHECK( !IsOutOfBounds( posEdge, zeroSize, bitmapSize ) );
}


BOOST_AUTO_TEST_SUITE_END()
