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
 * Test suite for KiCad math code.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <inttypes.h>

// Code under test
#include <math/util.h>

struct TEST_RESCALE_I64_CASE
{
    int64_t m_numerator;
    int64_t m_value;
    int64_t m_denominator;
    int64_t m_result;
};

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( MathUtil )

BOOST_AUTO_TEST_CASE( test_rescale_int64 )
{
    // Order: numerator, value, denominator, result.
    const std::vector<TEST_RESCALE_I64_CASE> rescale_i64_cases = {
        { 10LL, 10LL, 1LL, 100LL },
        { 10LL, 10LL, -1LL, -100LL },
        { 10LL, -10LL, 1LL, -100LL },
        { 10LL, -10LL, -1LL, 100LL },

        { 1LL, 9LL, 1LL, 9LL },
        { 1LL, 9LL, -1LL, -9LL },
        { 1LL, -9LL, 1LL, -9LL },
        { 1LL, -9LL, -1LL, 9LL },

        { 10LL, 10LL, 2LL, 50LL },
        { 10LL, 10LL, -2LL, -50LL },
        { 10LL, -10LL, 2LL, -50LL },
        { 10LL, -10LL, -2LL, 50LL },

        { 1LL, 9LL, 2LL, 5LL },
        { 1LL, 9LL, -2LL, -5LL },
        { 1LL, -9LL, 2LL, -5LL },
        { 1LL, -9LL, -2LL, 5LL },

        { 1LL, 17LL, 4LL, 4LL },
        { 1LL, 17LL, -4LL, -4LL },
        { 1LL, -17LL, 4LL, -4LL },
        { 1LL, -17LL, -4LL, 4LL },

        { 1LL, 19LL, 4LL, 5LL },
        { 1LL, 19LL, -4LL, -5LL },
        { 1LL, -19LL, 4LL, -5LL },
        { 1LL, -19LL, -4LL, 5LL },

        { 1LL, 0LL, 4LL, 0LL },
        { 1LL, 0LL, -4LL, 0LL },
        { -1LL, 0LL, 4LL, 0LL },
        { -1LL, 0LL, -4LL, 0LL },

        // sqrt(2^63) = 3037000499.98..
        { 3037000499LL, 3037000499LL, 1LL, 9223372030926249001LL },
        { 3037000499LL, 3037000499LL, -1LL, -9223372030926249001LL },
        { 3037000499LL, -3037000499LL, 1LL, -9223372030926249001LL },
        { 3037000499LL, -3037000499LL, -1LL, 9223372030926249001LL },

        // sqrt(2^63 * 10) = 9603838834.99..
        { 9603838834LL, 9603838834LL, 10LL, 9223372034944647956LL },
        { 9603838834LL, 9603838834LL, -10LL, -9223372034944647956LL },
        { 9603838834LL, -9603838834LL, 10LL, -9223372034944647956LL },
        { 9603838834LL, -9603838834LL, -10LL, 9223372034944647956LL },

        // INT64_MAX = 9223372036854775807
        { INT64_MAX, 10LL, 10LL, INT64_MAX },
        { INT64_MAX, 10LL, -10LL, -INT64_MAX },
        { INT64_MAX, -10LL, 10LL, -INT64_MAX },
        { INT64_MAX, -10LL, -10LL, INT64_MAX },

        { INT64_MAX, 10LL, INT64_MAX, 10LL },
        { INT64_MAX, 10LL, -INT64_MAX, -10LL },
        { INT64_MAX, -10LL, INT64_MAX, -10LL },
        { INT64_MAX, -10LL, -INT64_MAX, 10LL },

        { INT64_MAX, INT64_MAX, INT64_MAX, INT64_MAX },
        { INT64_MAX, INT64_MAX, -INT64_MAX, -INT64_MAX },
        { INT64_MAX, -INT64_MAX, INT64_MAX, -INT64_MAX },
        { INT64_MAX, -INT64_MAX, -INT64_MAX, INT64_MAX },
    };

    for( const TEST_RESCALE_I64_CASE& entry : rescale_i64_cases )
    {
        int64_t  calculated = rescale( entry.m_numerator, entry.m_value, entry.m_denominator );
        wxString msg;

        msg << "rescale<int64_t>( " << entry.m_numerator << ", " << entry.m_value << ", "
            << entry.m_denominator << " ) failed. ";
        msg << "\nExpected: " << entry.m_result;
        msg << "\nGot: " << calculated;

        BOOST_CHECK_MESSAGE( calculated == entry.m_result, msg );
    }
}

BOOST_AUTO_TEST_SUITE_END()
