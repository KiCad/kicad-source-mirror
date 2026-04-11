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

// Code under test
#include <core/typeinfo.h>
#include <properties/property_mgr.h>
#include <properties/property.h>


/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( Types )


// Make sure all available `KICAD_T` values are categorized properly.
BOOST_AUTO_TEST_CASE( AllCorrect )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        BOOST_CHECK_MESSAGE( IsTypeCorrect( type ), "Type " << type << " is not assigned to an application in typeinfo.h" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
