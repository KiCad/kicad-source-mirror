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

    KIID a;
    BOOST_CHECK_EQUAL( a.Hash(), 15552532309556242017ul );

    KIID b;
    BOOST_CHECK_EQUAL( b.Hash(), 13842873335846156666ul );

    KIID c;
    BOOST_CHECK_EQUAL( c.Hash(), 15995408467689523943ul );

    KIID d;
    BOOST_CHECK_EQUAL( d.Hash(), 4943106325342180035ul );
}


BOOST_AUTO_TEST_SUITE_END()
