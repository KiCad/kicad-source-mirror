/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#define BOOST_NO_AUTO_PTR

#include <boost/test/unit_test.hpp>
#include <turtle/mock.hpp>

#include <qa_utils/wx_utils/wx_assert.h>

#include <functional>
#include <optional>
#include <set>

#include <wx/gdicmn.h>
#include <wx/string.h>



template<class T>
struct PRINTABLE_OPT
{
    PRINTABLE_OPT( const std::optional<T>& aOpt ) : m_Opt( aOpt ){};
    PRINTABLE_OPT( const T& aVal ) : m_Opt( aVal ){};

    std::optional<T> m_Opt;
};


/**
 * Work around to allow printing std::optional types
 */
#define KI_CHECK_OPT_EQUAL( lhs, rhs )                                                            \
    BOOST_CHECK_EQUAL( PRINTABLE_OPT( lhs ), PRINTABLE_OPT( rhs ) )


template <class T>
inline std::ostream& operator<<( std::ostream& aOs, const PRINTABLE_OPT<T>& aOptional )
{
    if( aOptional.m_Opt.has_value() )
        aOs << *aOptional.m_Opt;
    else
        aOs << "nullopt";

    return aOs;
}


template <class L, class R>
inline bool operator==( const PRINTABLE_OPT<L>& aLhs, const PRINTABLE_OPT<R>& aRhs )
{
    if( !aLhs.m_Opt.has_value() && !aRhs.m_Opt.has_value() )
        return true; // both nullopt

    return aLhs.m_Opt.has_value() && aRhs.m_Opt.has_value() && *aLhs.m_Opt == *aRhs.m_Opt;
}


template <class L, class R>
inline bool operator!=( const PRINTABLE_OPT<L>& aLhs, const PRINTABLE_OPT<R>& aRhs )
{
    return !( aLhs == aRhs );
}


// boost_test_print_type has to be in the same namespace as the printed type
namespace std
{

/**
 * Boost print helper for generic vectors
 */
template <typename T>
std::ostream& boost_test_print_type( std::ostream& os, std::vector<T> const& aVec )
{
    os << "std::vector size " << aVec.size() << " [";

    for( const auto& i : aVec )
    {
        os << "\n    " << i;
    }

    os << "]";
    return os;
}

/**
 * Boost print helper for generic maps
 */
template <typename K, typename V>
std::ostream& boost_test_print_type( std::ostream& os, std::map<K, V> const& aMap )
{
    os << "std::map size " << aMap.size() << " [";

    for( const auto& [key, value] : aMap )
    {
        os << "\n    " << key << " = " << value;
    }

    os << "]";
    return os;
}

/**
 * Boost print helper for generic pairs
 */
template <typename K, typename V>
std::ostream& boost_test_print_type( std::ostream& os, std::pair<K, V> const& aPair )
{
    os << "[" << aPair.first << ", " << aPair.second << "]";
    return os;
}

} // namespace std


//-----------------------------------------------------------------------------+
// Boost.Test printing helpers for wx types / wide string literals
//-----------------------------------------------------------------------------+
namespace boost { namespace test_tools { namespace tt_detail {

template<>
struct print_log_value<wxString>
{
    void operator()( std::ostream& os, wxString const& v )
    {
#if wxUSE_UNICODE
        os << v.ToUTF8().data();
#else
        os << v;
#endif
    }
};

// Wide string literal arrays
template<std::size_t N>
struct print_log_value<wchar_t[ N ]>
{
    void operator()( std::ostream& os, const wchar_t (&ws)[ N ] )
    {
        wxString tmp( ws );
#if wxUSE_UNICODE
        os << tmp.ToUTF8().data();
#else
        os << tmp;
#endif
    }
};

}}} // namespace boost::test_tools::tt_detail


/**
 * Boost print helper for wxPoint. Note operator<< for this type doesn't
 * exist in non-DEBUG builds.
 */
std::ostream& boost_test_print_type( std::ostream& os, wxPoint const& aVec );

namespace KI_TEST
{

template <typename EXP_CONT> using EXP_OBJ = typename EXP_CONT::value_type;
template <typename FOUND_CONT> using FOUND_OBJ = typename FOUND_CONT::value_type;

/**
 * A match predicate: check that a "found" object is equivalent to or represents
 * an "expected" object, perhaps of a different type.
 *
 * Exactly what "equivalent to" means depends heavily on the context and what
 * is care about. For example, if you only care about a #FOOTPRINT's refdes,
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
void CheckUnorderedMatches( const EXP_CONT& aExpected, const FOUND_CONT& aFound,
                            MATCH_PRED aMatchPredicate )
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
        const bool was_expected = std::find_if( aExpected.begin(), aExpected.end(),
                [found]( const EXP_OBJ& aObj )
                {
                    return &aObj == found;
                } ) != aExpected.end();

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
 * A named data-driven test case.
 *
 * Inherit from this class to provide a printable name for a data-driven test case.
 * (you can also not use this class and provide you own name printer).
 */
struct NAMED_CASE
{
    std::string m_CaseName;

    friend std::ostream& operator<<( std::ostream& os, const NAMED_CASE& aCase )
    {
        os << aCase.m_CaseName;
        return os;
    }
};


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

/**
 * Get the configured location of Eeschema test data.
 *
 * By default, this is the test data in the source tree, but can be overridden
 * by the KICAD_TEST_EESCHEMA_DATA_DIR environment variable.
 *
 * @return a filename referring to the test data dir to use.
 */
std::string GetEeschemaTestDataDir();

std::string GetTestDataRootDir();

void SetMockConfigDir();

} // namespace KI_TEST

#endif // UNIT_TEST_UTILS__H
