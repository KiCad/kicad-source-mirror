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
 * @file
 * Test suite for #ARRAY_AXIS
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <array_axis.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( ArrayAxis )

struct VALID_OFFSET_CASE
{
    ARRAY_AXIS::NUMBERING_TYPE m_axis_type;
    std::string                m_offset_str;
    bool                       m_exp_valid;
    int                        m_exp_offset;
};


/**
 * Check we can get valid (or invalid) offsets as expected
 */
BOOST_AUTO_TEST_CASE( ValidOffsets )
{
    // clang-format off
    const std::vector<VALID_OFFSET_CASE> cases = {
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "0",
            true,
            0,
        },
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "1",
            true,
            1,
        },
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "1234",
            true,
            1234,
        },
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "",
            false,
            0,
        },
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "www",
            false,
            0,
        },
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_FULL,
            "A",
            true,
            0,
        },
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_FULL,
            "XY",
            true,
            648,
        },
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_HEX,
            "A0",
            true,
            160,
        },
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_HEX,
            "G0",
            false,
            0,
        },
    };
    // clang-format on

    for( const auto& c : cases )
    {
        ARRAY_AXIS axis;
        axis.SetAxisType( c.m_axis_type );

        bool offset_ok = axis.SetOffset( c.m_offset_str );

        BOOST_CHECK_EQUAL( offset_ok, c.m_exp_valid );

        if( c.m_exp_valid )
        {
            BOOST_CHECK_EQUAL( axis.GetOffset(), c.m_exp_offset );
        }
    }
}

/**
 * Data for testing a single array axis
 */
struct ARRAY_AXIS_NAMING_PARAMS
{
    ARRAY_AXIS::NUMBERING_TYPE m_axis_type;
    std::string                m_start_at;
    int                        m_step;
};

struct ARRAY_AXIS_NAMING_CASE
{
    std::string              m_case_name;
    ARRAY_AXIS_NAMING_PARAMS m_prms;
    int                      m_num;
    std::vector<std::string> m_exp_names;
};


// clang-format off
static const std::vector<ARRAY_AXIS_NAMING_CASE> axis_name_cases = {
    {
        "Numeric",
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "1",
            1,
        },
        6,
        { "1", "2", "3", "4", "5", "6" },
    },
    {
        // Test alphabetical
        "Alpha",
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_FULL,
            "A",
            1,
        },
        3,
        { "A", "B", "C" },
    },
    {
        // Test alphabetical with 2nd col
        "Alpha 2nd col",
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_FULL,
            "Y",
            1,
        },
        4,
        { "Y", "Z", "AA", "AB" },
    },
    {
        "Numeric skip",
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "11",
            2,
        },
        6,
        { "11", "13", "15", "17", "19", "21" },
    },
};
// clang-format on

/**
 * Test of the naming cases
 */
BOOST_AUTO_TEST_CASE( Numbering )
{
    for( const auto& c : axis_name_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            ARRAY_AXIS axis;
            axis.SetAxisType( c.m_prms.m_axis_type );
            axis.SetStep( c.m_prms.m_step );

            bool start_ok = axis.SetOffset( c.m_prms.m_start_at );

            // All these examples have valid start offsets
            BOOST_CHECK( start_ok );

            std::vector<std::string> names;

            for( int i = 0; i < c.m_num; i++ )
            {
                names.push_back( axis.GetItemNumber( i ).ToStdString() );
            }

            BOOST_CHECK_EQUAL_COLLECTIONS(
                    names.begin(), names.end(), c.m_exp_names.begin(), c.m_exp_names.end() );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
