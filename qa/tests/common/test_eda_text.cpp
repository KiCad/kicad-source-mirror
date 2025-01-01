/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <base_units.h>
#include <eda_text.h>


BOOST_AUTO_TEST_SUITE( EdaText )


BOOST_AUTO_TEST_CASE( Compare )
{
    std::hash<EDA_TEXT> hasher;
    EDA_TEXT a( unityScale );
    EDA_TEXT b( unityScale );

    BOOST_CHECK_EQUAL( a, b );
    BOOST_CHECK_EQUAL( hasher( a ), hasher( b ) );

    a.SetText( wxS( "A" ) );
    BOOST_CHECK_GT( a, b );
    BOOST_CHECK_NE( hasher( a ), hasher( b ) );

    b.SetText( wxS( "B" ) );
    BOOST_CHECK_LT( a, b );
    BOOST_CHECK_NE( hasher( a ), hasher( b ) );

    a.SetText( wxS( "B" ) );
    a.SetTextPos( VECTOR2I( 1, 0 ) );
    BOOST_CHECK_GT( a, b );

    a.SetTextPos( VECTOR2I( -1, 0 ) );
    BOOST_CHECK_LT( a, b );

    a.SetTextPos( VECTOR2I( 0, 0 ) );
    b.SetTextPos( VECTOR2I( 0, 1 ) );
    BOOST_CHECK_LT( a, b );

    b.SetTextPos( VECTOR2I( 0, -1 ) );
    BOOST_CHECK_GT( a, b );

    // Text attributes are tested in the TEXT_ATTRIBUTES unit tests.
}


BOOST_AUTO_TEST_SUITE_END()
