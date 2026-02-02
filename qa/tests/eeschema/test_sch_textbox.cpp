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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <sch_textbox.h>


struct SCH_TEXTBOX_FIXTURE
{
    SCH_TEXTBOX m_textbox;

    SCH_TEXTBOX_FIXTURE() :
            m_textbox( LAYER_NOTES, 0, FILL_T::NO_FILL, wxT( "Hello World" ) )
    {
        m_textbox.SetTextSize( VECTOR2I( schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 50 ) ) );
        m_textbox.SetStart( VECTOR2I( 0, 0 ) );
        m_textbox.SetEnd( VECTOR2I( schIUScale.MilsToIU( 2000 ), schIUScale.MilsToIU( 500 ) ) );
    }
};


BOOST_FIXTURE_TEST_SUITE( SchTextbox, SCH_TEXTBOX_FIXTURE )


/**
 * Verify that GetMinSize() returns height-only constraint for non-empty text.
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

    int margin = schIUScale.MilsToIU( 100 );
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
 * Verify that GetMinSize() swaps the constrained axis for vertical text.
 */
BOOST_AUTO_TEST_CASE( GetMinSizeVerticalText )
{
    m_textbox.SetText( wxT( "Test" ) );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSizeH = m_textbox.GetMinSize();

    m_textbox.SetTextAngle( ANGLE_VERTICAL );
    m_textbox.ClearBoundingBoxCache();
    m_textbox.ClearRenderCache();

    VECTOR2I minSizeV = m_textbox.GetMinSize();

    BOOST_CHECK_EQUAL( minSizeH.x, 0 );
    BOOST_CHECK_GT( minSizeH.y, 0 );
    BOOST_CHECK_GT( minSizeV.x, 0 );
    BOOST_CHECK_EQUAL( minSizeV.y, 0 );
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
