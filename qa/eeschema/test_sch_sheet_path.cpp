/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file
 * Test suite for SCH_SHEET_PATH
 */

#include <unit_test_utils/unit_test_utils.h>

// Code under test
#include <sch_sheet_path.h>

#include "timestamp_test_utils.h"

#include <sch_sheet.h>

#include <sstream>

class TEST_SCH_SHEET_PATH_FIXTURE
{
public:
    TEST_SCH_SHEET_PATH_FIXTURE()
    {
        for( unsigned i = 0; i < 4; ++i )
        {
            m_sheets.emplace_back( wxPoint( i, i ) );

            std::ostringstream ss;
            ss << "Sheet" << i;
            m_sheets[i].SetName( ss.str() );
        }

        // 0->1->2
        m_linear.push_back( &m_sheets[0] );
        m_linear.push_back( &m_sheets[1] );
        m_linear.push_back( &m_sheets[2] );
    }

    SCH_SHEET_PATH m_empty_path;

    /**
     * We look at sheet 2 in the hierarchy:
     * Sheets: 0 -> 1 -> 2
     */
    SCH_SHEET_PATH m_linear;

    /// handy store of SCH_SHEET objects
    std::vector<SCH_SHEET> m_sheets;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchSheetPath, TEST_SCH_SHEET_PATH_FIXTURE )


/**
 * Check properties of an empty SCH_SHEET_PATH
 */
BOOST_AUTO_TEST_CASE( Empty )
{
    BOOST_CHECK_EQUAL( m_empty_path.size(), 0 );

    BOOST_CHECK_THROW( m_empty_path.at( 0 ), std::out_of_range );

    BOOST_CHECK_EQUAL( m_empty_path.GetPageNumber(), 0 );

    // These accessors return nullptr when empty (i.e. they don't crash)
    BOOST_CHECK_EQUAL( m_empty_path.Last(), nullptr );
    BOOST_CHECK_EQUAL( m_empty_path.LastScreen(), nullptr );
    BOOST_CHECK_EQUAL( m_empty_path.LastDrawList(), nullptr );
    BOOST_CHECK_EQUAL( m_empty_path.FirstDrawList(), nullptr );

    BOOST_CHECK_EQUAL( m_empty_path.Path(), "/" );
    BOOST_CHECK_EQUAL( m_empty_path.PathHumanReadable(), "/" );
}


/**
 * Check properties of a non-empty SCH_SHEET_PATH
 */
BOOST_AUTO_TEST_CASE( NonEmpty )
{
    BOOST_CHECK_EQUAL( m_linear.size(), 3 );

    BOOST_CHECK_EQUAL( m_linear.at( 0 ), &m_sheets[0] );
    BOOST_CHECK_EQUAL( m_linear.at( 1 ), &m_sheets[1] );
    BOOST_CHECK_EQUAL( m_linear.at( 2 ), &m_sheets[2] );

    BOOST_CHECK_EQUAL( m_linear.GetPageNumber(), 0 );

    BOOST_CHECK_EQUAL( m_linear.Last(), &m_sheets[2] );
    BOOST_CHECK_EQUAL( m_linear.LastScreen(), nullptr );
    BOOST_CHECK_EQUAL( m_linear.LastDrawList(), nullptr );
    BOOST_CHECK_EQUAL( m_linear.FirstDrawList(), nullptr );

    // don't know what the timestamps will be, but we know the format: /<8 chars>/<8 chars>/
    BOOST_CHECK_PREDICATE(
            KI_TEST::IsTimestampStringWithLevels, ( m_linear.Path().ToStdString() )( 2 ) );

    // Sheet0 is the root sheet and isn't in the path
    BOOST_CHECK_EQUAL( m_linear.PathHumanReadable(), "/Sheet1/Sheet2/" );
}


BOOST_AUTO_TEST_CASE( Compare )
{
    SCH_SHEET_PATH otherEmpty;

    BOOST_CHECK( m_empty_path == otherEmpty );

    BOOST_CHECK( m_empty_path != m_linear );
}

BOOST_AUTO_TEST_SUITE_END()