/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Test suite for SCH_COMPONENT object.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <sch_symbol.h>

#include <sch_edit_frame.h>

class TEST_SCH_SYMBOL_FIXTURE
{
public:
    TEST_SCH_SYMBOL_FIXTURE()
    {
    }

    ///> #SCH_COMPONENT object with no extra data set.
    SCH_COMPONENT m_symbol;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchSymbol, TEST_SCH_SYMBOL_FIXTURE )


/**
 * Check that we can get the default properties as expected.
 */
BOOST_AUTO_TEST_CASE( DefaultProperties )
{
}


/**
 * Test the orientation transform changes.
 */
BOOST_AUTO_TEST_CASE( Orientation )
{
    TRANSFORM t = m_symbol.GetTransform();

    m_symbol.SetOrientation( CMP_ORIENT_90 );
    t = m_symbol.GetTransform();
    m_symbol.SetTransform( TRANSFORM() );
    m_symbol.SetOrientation( CMP_ORIENT_180 );
    t = m_symbol.GetTransform();
    m_symbol.SetTransform( TRANSFORM() );
    m_symbol.SetOrientation( CMP_ORIENT_270 );
    t = m_symbol.GetTransform();
}


BOOST_AUTO_TEST_SUITE_END()
