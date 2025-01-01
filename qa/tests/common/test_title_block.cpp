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

#include <title_block.h>


struct TitleBlockFixture
{
    TitleBlockFixture()
    {
        m_tb.SetTitle( "title" );
        m_tb.SetDate( "date" );
        m_tb.SetCompany( "company" );

        // leave revision blank
        //m_tb.SetRevision( "revision" );

        // set more than one comment to make sure the indexing of comments works
        m_tb.SetComment( 0, "comment1" );
        m_tb.SetComment( 1, "comment2" );
        m_tb.SetComment( 2, "comment3" );
        m_tb.SetComment( 3, "comment4" );
    }

    TITLE_BLOCK m_tb;
};


/**
 * Declares a struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( TitleBlock, TitleBlockFixture )

/**
 * Check basic setting and getting of values
 */
BOOST_AUTO_TEST_CASE( SimpleAccess )
{
    BOOST_CHECK_EQUAL( "title", m_tb.GetTitle() );
    BOOST_CHECK_EQUAL( "date", m_tb.GetDate() );
    BOOST_CHECK_EQUAL( "company", m_tb.GetCompany() );

    // This one is blank
    BOOST_CHECK_EQUAL( "", m_tb.GetRevision() );

    BOOST_CHECK_EQUAL( "comment1", m_tb.GetComment( 0 ) );
    BOOST_CHECK_EQUAL( "comment2", m_tb.GetComment( 1 ) );
    BOOST_CHECK_EQUAL( "comment3", m_tb.GetComment( 2 ) );
    BOOST_CHECK_EQUAL( "comment4", m_tb.GetComment( 3 ) );
}

/*
 * Check copy construction
 */
BOOST_AUTO_TEST_CASE( Copy )
{
    TITLE_BLOCK tb_cpy = m_tb;

    // Check that values came through
    BOOST_CHECK_EQUAL( "title", tb_cpy.GetTitle() );
    BOOST_CHECK_EQUAL( "comment1", tb_cpy.GetComment( 0 ) );
    BOOST_CHECK_EQUAL( "comment2", tb_cpy.GetComment( 1 ) );
}


BOOST_AUTO_TEST_SUITE_END()
