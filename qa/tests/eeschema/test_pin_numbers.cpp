/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_pin_numbers.cpp
 *
 * Test suite for Pin number comparison functions.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pin_numbers.h>
#include <wx/string.h>


class TEST_PIN_NUMBERS
{
public:
    TEST_PIN_NUMBERS()
    {
    }
};


struct TEST_PIN_NUMBER_CMP_CASE
{
    wxString    m_lhs;
    wxString    m_rhs;
    int         m_return;
};

/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchInternalUnits, TEST_PIN_NUMBERS )


BOOST_AUTO_TEST_CASE( ComparePinNumbers )
{

    const std::vector<TEST_PIN_NUMBER_CMP_CASE> cases = {
            {
                wxT( "+V" ),
                wxT( "+V" ),
                0,
            },
            {
                wxT( "+V1234" ),
                wxT( "+V" ),
                2,
            },
            {
                wxT( "+V" ),
                wxT( "+V1234" ),
                -2,
            },
            {
                wxT( "Pin1" ),
                wxT( "Pin2" ),
                -1
            },
            {
                wxT( "Pin2" ),
                wxT( "Pin1" ),
                1
            },
            {
                wxT( "Pin1" ),
                wxT( "Pin1" ),
                0
            },
            {
                wxT( "1Pin" ),
                wxT( "2Pin" ),
                -1
            },
            {
                wxT( "2Pin" ),
                wxT( "1Pin" ),
                1
            },
            {
                wxT( "1Pin" ),
                wxT( "1Pin" ),
                0
            },
            {
                wxT( "+3V3" ),
                wxT( "+3.3" ),
                0
            },
            {
                wxT( "+5V" ),
                wxT( "+6V" ),
                -1
            },
            {
                wxT( "+6V" ),
                wxT( "+5V" ),
                1
            }

    };

    for( auto& el : cases )
    {
        int retval = PIN_NUMBERS::Compare( el.m_lhs, el.m_rhs );
        wxString msg;

        msg.Printf( "Comparing %s and %s failed [%d != %d]", el.m_lhs, el.m_rhs, retval, el.m_return );
        BOOST_CHECK_MESSAGE( retval == el.m_return,  msg.ToStdString() );
    }
}

BOOST_AUTO_TEST_SUITE_END()
