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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <board.h>
#include <pcb_textbox.h>


struct PCB_TEXTBOX_FIXTURE
{
    BOARD       m_board;
    PCB_TEXTBOX m_textbox;

    PCB_TEXTBOX_FIXTURE() :
            m_board(),
            m_textbox( &m_board )
    {
        m_textbox.SetText( wxT( "Hello World" ) );
        m_textbox.SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
        m_textbox.SetStart( VECTOR2I( 0, 0 ) );
        m_textbox.SetEnd( VECTOR2I( pcbIUScale.mmToIU( 20.0 ), pcbIUScale.mmToIU( 5.0 ) ) );
    }
};


BOOST_FIXTURE_TEST_SUITE( PcbTextbox, PCB_TEXTBOX_FIXTURE )


// Width is unconstrained so text can rewrap, height is bounded by the wrapped content.
BOOST_AUTO_TEST_CASE( GetMinSizeReturnsHeightOnly )
{
    VECTOR2I minSize = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSize.x, 0 );
    BOOST_CHECK_GT( minSize.y, 0 );
}


BOOST_AUTO_TEST_CASE( GetMinSizeGrowsWithMoreText )
{
    VECTOR2I minSizeShort = m_textbox.GetMinSize();

    m_textbox.SetText( wxT( "Hello World\nSecond Line\nThird Line" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();
    VECTOR2I minSizeLong = m_textbox.GetMinSize();

    BOOST_CHECK_GT( minSizeLong.y, minSizeShort.y );
}


BOOST_AUTO_TEST_CASE( GetMinSizeIncludesMargins )
{
    m_textbox.SetText( wxT( "A" ) );
    m_textbox.SetMarginLeft( 0 );
    m_textbox.SetMarginRight( 0 );
    m_textbox.SetMarginTop( 0 );
    m_textbox.SetMarginBottom( 0 );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSizeNoMargin = m_textbox.GetMinSize();

    int margin = pcbIUScale.mmToIU( 1.0 );
    m_textbox.SetMarginTop( margin );
    m_textbox.SetMarginBottom( margin );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSizeWithMargin = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSizeNoMargin.x, 0 );
    BOOST_CHECK_EQUAL( minSizeWithMargin.x, 0 );
    BOOST_CHECK_GT( minSizeWithMargin.y, minSizeNoMargin.y );
}


BOOST_AUTO_TEST_CASE( GetMinSizeHeightAtLeastTextHeight )
{
    m_textbox.SetText( wxT( "A" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();
    VECTOR2I minSize = m_textbox.GetMinSize();

    BOOST_CHECK_GE( minSize.y, m_textbox.GetTextSize().y );
}


BOOST_AUTO_TEST_CASE( GetMinSizeEmptyText )
{
    m_textbox.SetText( wxT( "" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSize = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSize.x, 0 );
    BOOST_CHECK_EQUAL( minSize.y, 0 );
}


// At 0 degrees the bound is on y, at 90 degrees it moves to x.
BOOST_AUTO_TEST_CASE( GetMinSizeRotated90 )
{
    m_textbox.SetText( wxT( "Wide Text" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSize0 = m_textbox.GetMinSize();

    m_textbox.SetTextAngle( EDA_ANGLE( 90.0, DEGREES_T ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSize90 = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSize0.x, 0 );
    BOOST_CHECK_GT( minSize0.y, 0 );
    BOOST_CHECK_GT( minSize90.x, 0 );
    BOOST_CHECK_EQUAL( minSize90.y, 0 );
}


BOOST_AUTO_TEST_CASE( GetMinSizeAllowsWidthReduction )
{
    m_textbox.SetText( wxT( "This is a very long line of text that should exceed the box width" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSize = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSize.x, 0 );
    BOOST_CHECK_GT( minSize.y, 0 );
}


// Guards a past snap-back where 3x30 degrees landed on cardinal and
// GetCornersInSequence returned an un-rotated AABB.
BOOST_AUTO_TEST_CASE( ThirtyDegreeIncrementsToCardinalKeepsVisualRotated )
{
    for( int i = 0; i < 3; ++i )
        m_textbox.Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 30.0, DEGREES_T ) );

    BOOST_REQUIRE_CLOSE( m_textbox.GetTextAngle().AsDegrees(), 90.0, 1e-6 );

    std::vector<VECTOR2I> corners = m_textbox.GetCorners();
    BOOST_REQUIRE_EQUAL( corners.size(), 4u );

    BOX2I visual;
    for( const VECTOR2I& p : corners )
        visual.Merge( p );

    const int origW = pcbIUScale.mmToIU( 20.0 );
    const int origH = pcbIUScale.mmToIU( 5.0 );
    BOOST_CHECK_MESSAGE( std::abs( visual.GetWidth() - origH ) <= 1,
                         "visual width " << visual.GetWidth() << " expected " << origH );
    BOOST_CHECK_MESSAGE( std::abs( visual.GetHeight() - origW ) <= 1,
                         "visual height " << visual.GetHeight() << " expected " << origW );
}


// Guards a past crash where rendering a RECTANGLE textbox at a non-cardinal
// angle walked an empty polygon vector.
BOOST_AUTO_TEST_CASE( NonCardinalRotationDoesNotCrash )
{
    m_textbox.Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 30.0, DEGREES_T ) );

    std::vector<VECTOR2I> corners = m_textbox.GetCorners();
    BOOST_REQUIRE_EQUAL( corners.size(), 4u );

    BOX2I bbox;
    for( const VECTOR2I& p : corners )
        bbox.Merge( p );

    BOOST_CHECK_GT( bbox.GetWidth(), 0 );
    BOOST_CHECK_GT( bbox.GetHeight(), 0 );

    BOOST_CHECK_NO_THROW( m_textbox.GetDrawPos() );
}


BOOST_AUTO_TEST_CASE( CardinalRotationsRoundTrip )
{
    VECTOR2I origStart = m_textbox.GetStart();
    VECTOR2I origEnd = m_textbox.GetEnd();

    VECTOR2I pivot( pcbIUScale.mmToIU( 7.0 ), pcbIUScale.mmToIU( 11.0 ) );

    for( int i = 0; i < 4; ++i )
        m_textbox.Rotate( pivot, EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( m_textbox.GetStart().x, origStart.x );
    BOOST_CHECK_EQUAL( m_textbox.GetStart().y, origStart.y );
    BOOST_CHECK_EQUAL( m_textbox.GetEnd().x, origEnd.x );
    BOOST_CHECK_EQUAL( m_textbox.GetEnd().y, origEnd.y );
}


// The on-disk form must stay a rectangle even after a tilted rotation.
BOOST_AUTO_TEST_CASE( NonCardinalRotationKeepsLibShapeRectangle )
{
    m_textbox.Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 30.0, DEGREES_T ) );

    BOOST_CHECK( m_textbox.GetLibraryShape() == SHAPE_T::RECTANGLE );
}


BOOST_AUTO_TEST_SUITE_END()
