/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <functional>
#include <string>

#include <core/inplace_function.h>

BOOST_AUTO_TEST_SUITE( InplaceFunction )

// Mirror the DRC deferred-eval usage: a closure capturing a few pointers, moved into the slot.
BOOST_AUTO_TEST_CASE( MoveAssignAndInvoke )
{
    int a = 1, b = 2, c = 3;

    INPLACE_FUNCTION<int()> slot;
    BOOST_CHECK( !slot );

    slot = INPLACE_FUNCTION<int()>( [&a, &b, &c]() { return a + b + c; } );
    BOOST_CHECK( bool( slot ) );
    BOOST_CHECK_EQUAL( slot(), 6 );

    // Reassigning a live slot replaces the target rather than leaking the old one.
    slot = INPLACE_FUNCTION<int()>( [&a]() { return a * 10; } );
    BOOST_CHECK_EQUAL( slot(), 10 );
}

BOOST_AUTO_TEST_CASE( CopyKeepsSourceLive )
{
    int v = 7;
    INPLACE_FUNCTION<int()> src( [&v]() { return v; } );

    INPLACE_FUNCTION<int()> copy = src;
    BOOST_CHECK_EQUAL( copy(), 7 );
    BOOST_CHECK_EQUAL( src(), 7 );
}

BOOST_AUTO_TEST_CASE( MoveLeavesSourceEmpty )
{
    int v = 9;
    INPLACE_FUNCTION<int()> src( [&v]() { return v; } );

    INPLACE_FUNCTION<int()> dst = std::move( src );
    BOOST_CHECK_EQUAL( dst(), 9 );
    BOOST_CHECK( !src );
}

BOOST_AUTO_TEST_CASE( SelfAssignmentIsSafe )
{
    int v = 11;
    INPLACE_FUNCTION<int()> fn( [&v]() { return v; } );

    fn = fn;
    BOOST_CHECK_EQUAL( fn(), 11 );
}

// An empty INPLACE_FUNCTION must report bad_function_call, not dereference a null target.
BOOST_AUTO_TEST_CASE( EmptyInvocationThrows )
{
    INPLACE_FUNCTION<int()> empty;
    BOOST_CHECK_THROW( empty(), std::bad_function_call );
}

BOOST_AUTO_TEST_CASE( StringReturnType )
{
    INPLACE_FUNCTION<std::string()> fn( []() { return std::string( "ok" ); } );
    BOOST_CHECK_EQUAL( fn(), "ok" );
}

BOOST_AUTO_TEST_SUITE_END()
