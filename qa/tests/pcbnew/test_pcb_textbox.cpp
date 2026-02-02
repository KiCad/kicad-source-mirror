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


/**
 * Verify that GetMinSize() returns height-only constraint for non-empty text.
 * Width should be 0 (unconstrained) so text can rewrap freely.
 */
BOOST_AUTO_TEST_CASE( GetMinSizeReturnsHeightOnly )
{
    VECTOR2I minSize = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSize.x, 0 );
    BOOST_CHECK_GT( minSize.y, 0 );
}


/**
 * Verify that GetMinSize() height grows when more text is added.
 */
BOOST_AUTO_TEST_CASE( GetMinSizeGrowsWithMoreText )
{
    VECTOR2I minSizeShort = m_textbox.GetMinSize();

    m_textbox.SetText( wxT( "Hello World\nSecond Line\nThird Line" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();
    VECTOR2I minSizeLong = m_textbox.GetMinSize();

    BOOST_CHECK_GT( minSizeLong.y, minSizeShort.y );
}


/**
 * Verify that GetMinSize() height grows when vertical margins are increased.
 */
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


/**
 * Verify that GetMinSize() height is at least the text height for a single line of text.
 */
BOOST_AUTO_TEST_CASE( GetMinSizeHeightAtLeastTextHeight )
{
    m_textbox.SetText( wxT( "A" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();
    VECTOR2I minSize = m_textbox.GetMinSize();

    BOOST_CHECK_GE( minSize.y, m_textbox.GetTextSize().y );
}


/**
 * Verify that GetMinSize() returns zero for both axes with empty text.
 */
BOOST_AUTO_TEST_CASE( GetMinSizeEmptyText )
{
    m_textbox.SetText( wxT( "" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSize = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSize.x, 0 );
    BOOST_CHECK_EQUAL( minSize.y, 0 );
}


/**
 * Verify that GetMinSize() swaps the constrained axis for 90-degree rotation.
 * At 0 degrees the height constraint is on y. At 90 degrees it moves to x.
 */
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


/**
 * Verify that width is always unconstrained regardless of text content.
 */
BOOST_AUTO_TEST_CASE( GetMinSizeAllowsWidthReduction )
{
    m_textbox.SetText( wxT( "This is a very long line of text that should exceed the box width" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSize = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSize.x, 0 );
    BOOST_CHECK_GT( minSize.y, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
