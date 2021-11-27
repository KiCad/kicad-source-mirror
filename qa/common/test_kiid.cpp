/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiid.h>


BOOST_AUTO_TEST_SUITE( Kiid )


BOOST_AUTO_TEST_CASE( Seeding )
{
    KIID::SeedGenerator( 0l );

    KIID a0;
    KIID b0;
    KIID c0;
    KIID d0;

    KIID::SeedGenerator( 0l );

    KIID a;
    BOOST_CHECK_EQUAL( a.Hash(), a0.Hash() );

    KIID b;
    BOOST_CHECK_EQUAL( b.Hash(), b0.Hash() );

    KIID c;
    BOOST_CHECK_EQUAL( c.Hash(), c0.Hash() );

    KIID d;
    BOOST_CHECK_EQUAL( d.Hash(), d0.Hash() );
}


BOOST_AUTO_TEST_SUITE_END()
