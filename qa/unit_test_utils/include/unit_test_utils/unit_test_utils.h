/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef UNIT_TEST_UTILS__H
#define UNIT_TEST_UTILS__H

#include <boost/test/unit_test.hpp>

#include <unit_test_utils/wx_assert.h>

#include <functional>
#include <set>

#include <wx/gdicmn.h>
/**
 * If HAVE_EXPECTED_FAILURES is defined, this means that
 * boost::unit_test::expected_failures is available.
 *
 * Wrap expected-to-fail tests with this to prevent them being compiled
 * on platforms with older (<1.59) Boost versions.
 *
 * This can be removed when our minimum boost version is 1.59 or higher.
 */
#if BOOST_VERSION >= 105900
#define HAVE_EXPECTED_FAILURES
#endif

/**
 * BOOST_TEST, while extremely handy, is not available in Boost < 1.59.
 * Undef it here to prevent use. Using it can cause older packaging like
 * Ubuntu LTS (which is on Boost 1.58) to fail.
 *
 * Use BOOST_CHECK_{EQUAL,NE,etc} instead.
 *
 * This can be removed when our minimum boost version is 1.59 or higher.
 */
#undef BOOST_TEST


#if BOOST_VERSION < 105900

/*
 * BOOST_TEST_INFO is not available before 1.59. It's not critical for
 * test pass/fail, it's just info, so just pass along to a logging
 * function.
 *
 * This can be removed when our minimum boost version is 1.59 or higher.
 */
#define BOOST_TEST_INFO( A ) BOOST_TEST_MESSAGE( A )

/*
 *
 * BOOST_TEST_CONTEXT provides scoped info, but again, only after 1.59.
 * Replacing with a call to BOOST_TEST_MESSAGE will work, and the
 * scoping will still work for newer boosts.
 *
 * This can be removed when our minimum boost version is 1.59 or higher.
 */
#define BOOST_TEST_CONTEXT( A ) BOOST_TEST_MESSAGE( A );

#endif

/*
 * Boost hides the configuration point for print_log_value in different
 * namespaces between < 1.59 and >= 1.59.
 *
 * The macros can be used to open and close the right level of namespacing
 * based on the version.
 *
 * We could just use a conditionally defined namespace alias, but that
 * doesn't work in GCC <7 (GCC bug #56480)
 *
 * From Boost 1.64, this should be done with boost_test_print_type,
 * and these defines can be removed once all logging functions use that.
 */
#if BOOST_VERSION >= 105900
#define BOOST_TEST_PRINT_NAMESPACE_OPEN \
    boost                               \
    {                                   \
    namespace test_tools                \
    {                                   \
    namespace tt_detail
#define BOOST_TEST_PRINT_NAMESPACE_CLOSE }}
#else
#define BOOST_TEST_PRINT_NAMESPACE_OPEN \
    boost                               \
    {                                   \
    namespace test_tools
#define BOOST_TEST_PRINT_NAMESPACE_CLOSE }
#endif

/**
 * Before Boost 1.64, nullptr_t wasn't handled. Provide our own logging
 * for nullptr_t's, which helps when doing BOOST_CHECK/REQUIRES on pointers.
 *
 * This can be removed when our minimum boost version is 1.64 or higher.
 */
#if BOOST_VERSION < 106400

namespace BOOST_TEST_PRINT_NAMESPACE_OPEN
{
template <>
struct print_log_value<std::nullptr_t>
{
    inline void operator()( std::ostream& os, std::nullptr_t const& p )
    {
        os << "nullptr";
    }
};
}
BOOST_TEST_PRINT_NAMESPACE_CLOSE

#endif


namespace BOOST_TEST_PRINT_NAMESPACE_OPEN
{

/**
 * Boost print helper for generic vectors
 */
template <typename T>
struct print_log_value<std::vector<T>>
{
    inline void operator()( std::ostream& os, std::vector<T> const& aVec )
    {
        os << "std::vector size " << aVec.size() << "[";

        for( const auto& i : aVec )
        {
            os << "\n    ";
            print_log_value<T>()( os, i );
        }

        os << "]";
    }
};

/**
 * Boost print helper for wxPoint. Note operator<< for this type doesn't
 * exist in non-DEBUG builds.
 */
template <>
struct print_log_value<wxPoint>
{
    void operator()( std::ostream& os, wxPoint const& aVec );
};
}
BOOST_TEST_PRINT_NAMESPACE_CLOSE


namespace KI_TEST
{

template <typename EXP_CONT> using EXP_OBJ = typename EXP_CONT::value_type;
template <typename FOUND_CONT> using FOUND_OBJ = typename FOUND_CONT::value_type;

/**
 * A match predicate: check that a "found" object is equivalent to or represents
 * an "expected" object, perhaps of a different type.
 *
 * Exactly what "equivalent to" means depends heavily on the context and what
 * is care about. For example, if you only care about a #MODULE's refdes,
 * std::string is sufficient to indicate a "match".
 *
 * This can be used, for example, for checking a set of results without having
 * to instantiate a full result object for checking by equality.
 *
 * @tparam EXP_OBJ      the "expected" object type
 * @tparam FOUND_OBJ    the "found" object type
 *
 * @return true if the "found" object represents the "expected" object
 */
template <typename EXP_OBJ, typename FOUND_OBJ>
using MATCH_PRED = std::function<bool( const EXP_OBJ&, const FOUND_OBJ& )>;

/**
 * Check that a container of "found" objects matches a container of "expected"
 * objects. This means that:
 *
 * * Every "expected" object is "found"
 * * Every "found" object is "expected"
 *
 * This is a very generic function: all you need are two containers of any type
 * and a function to check if a given "found" object corresponds to a given
 * "expected object". Conditions:
 *
 * * The expected object type needs `operator<<` (for logging)
 * * The expected object container does not contain multiple references to the
 *   same object.
 * * Identical values are also can't be present as the predicate can't tell which
 *   one to match up.
 *
 * Not needed:
 *
 * * Equality or ordering operators
 *
 * This is a slightly more complex way of doing it that, say, sorting both
 * lists and checking element-by-element matches. However, it can tell you
 * exactly which objects are problematic, as well as a simple go/no-go.
 *
 * When you have two containers of identical types (or you have a suitable
 * `operator==`) and ordering is important, you can use `BOOST_CHECK_EQUAL_COLLECTIONS`
 *
 *@param aExpected  a container of "expected" items, usually from a test case
 *@param aMatched   a container of "found" items, usually the result of some
 *                  routine under test
 *@param aMatchPredicate a predicate that determines if a given "found" object
 *                  matches a given "expected" object.
 */
template <typename EXP_CONT, typename FOUND_CONT, typename MATCH_PRED>
void CheckUnorderedMatches(
        const EXP_CONT& aExpected, const FOUND_CONT& aFound, MATCH_PRED aMatchPredicate )
{
    using EXP_OBJ = typename EXP_CONT::value_type;

    // set of object we've already found
    std::set<const EXP_OBJ*> matched;

    // fill the set of object that match
    for( const auto& found : aFound )
    {
        for( const auto& expected : aExpected )
        {
            if( aMatchPredicate( expected, found ) )
            {
                matched.insert( &expected );
                break;
            }
        }
    }

    // first check every expected object was "found"
    for( const EXP_OBJ& exp : aExpected )
    {
        BOOST_CHECK_MESSAGE( matched.count( &exp ) > 0, "Expected item was not found. Expected: \n"
                                                                << exp );
    }

    // check every "found" object was expected
    for( const EXP_OBJ* found : matched )
    {
        const bool was_expected =
                std::find_if( aExpected.begin(), aExpected.end(),
                        [found]( const EXP_OBJ& aObj ) { return &aObj == found; } )
                != aExpected.end();

        BOOST_CHECK_MESSAGE( was_expected, "Found item was not expected. Found: \n" << *found );
    }
}


/**
 * Predicate to check a collection has no duplicate elements
 */
template <typename T>
bool CollectionHasNoDuplicates( const T& aCollection )
{
    T sorted = aCollection;
    std::sort( sorted.begin(), sorted.end() );

    return std::adjacent_find( sorted.begin(), sorted.end() ) == sorted.end();
}


/**
 * A test macro to check a wxASSERT is thrown.
 *
 * This only happens in DEBUG builds, so prevent test failures in Release builds
 * by using this macro.
 */
#ifdef DEBUG
#define CHECK_WX_ASSERT( STATEMENT ) BOOST_CHECK_THROW( STATEMENT, KI_TEST::WX_ASSERT_ERROR );
#else
#define CHECK_WX_ASSERT( STATEMENT )
#endif

} // namespace KI_TEST

#endif // UNIT_TEST_UTILS__H