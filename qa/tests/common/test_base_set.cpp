/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "base_set.h"

BOOST_AUTO_TEST_SUITE( BaseSetTests )

BOOST_AUTO_TEST_CASE( ConstructionAndSize )
{
    BASE_SET bs(10);
    BOOST_CHECK_EQUAL(bs.size(), 10);
    BOOST_CHECK_EQUAL(bs.count(), 0);

    bs.resize(20);
    BOOST_CHECK_EQUAL(bs.size(), 20);
    BOOST_CHECK_EQUAL(bs.count(), 0);
}

BOOST_AUTO_TEST_CASE( BitSettingAndResetting )
{
    BASE_SET bs(10);
    bs.set(2);
    BOOST_CHECK(bs.test(2));
    BOOST_CHECK_EQUAL(bs.count(), 1);

    bs.reset(2);
    BOOST_CHECK(!bs.test(2));
    BOOST_CHECK_EQUAL(bs.count(), 0);
}

BOOST_AUTO_TEST_CASE( OutOfRange )
{
    BASE_SET bs(10);
    BOOST_CHECK_THROW(bs.set(10), std::out_of_range);
    BOOST_CHECK_THROW(bs.reset(10), std::out_of_range);
    BOOST_CHECK_THROW(bs.test(10), std::out_of_range);
}

BOOST_AUTO_TEST_CASE( IteratingSetBits )
{
    BASE_SET bs(10);
    bs.set(2);
    bs.set(4);

    auto it = bs.set_bits_begin();
    BOOST_CHECK_EQUAL(*it, 2);
    ++it;
    BOOST_CHECK_EQUAL(*it, 4);
    ++it;
    BOOST_CHECK(it == bs.set_bits_end());

    // Custom reverse iterator test
    std::vector<size_t> reverse_set_bits;
    for (auto rit = bs.set_bits_rbegin(); rit != bs.set_bits_rend(); ++rit)
    {
        reverse_set_bits.push_back(*rit);
    }

    BOOST_CHECK_EQUAL(reverse_set_bits.size(), 2);
    BOOST_CHECK_EQUAL(reverse_set_bits[0], 4);
    BOOST_CHECK_EQUAL(reverse_set_bits[1], 2);
}

BOOST_AUTO_TEST_SUITE_END()
