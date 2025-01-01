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
 * @file test_sch_biu.cpp
 *
 * Test suite for schematic base internal units (1 = 100nm).
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>


class TEST_SCH_INTERNAL_UNITS
{
public:
    TEST_SCH_INTERNAL_UNITS()
    {
    }
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchInternalUnits, TEST_SCH_INTERNAL_UNITS )


BOOST_AUTO_TEST_CASE( ConvertToInternalUnits )
{
    BOOST_CHECK_EQUAL( schIUScale.mmToIU( 1.0 ), 10000 );
}


BOOST_AUTO_TEST_CASE( ConvertFromInternalUnits )
{
    BOOST_CHECK_EQUAL( static_cast< int >( schIUScale.IUTomm( 10000 ) ), 1 );
}


BOOST_AUTO_TEST_SUITE_END()
