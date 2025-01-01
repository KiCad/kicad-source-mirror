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

#ifndef TEST_SEXPR_TEST_UTILS__H
#define TEST_SEXPR_TEST_UTILS__H

#include <sexpr/sexpr.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

namespace KI_TEST
{


/**
 * Get the type of the s-expression.
 */
inline SEXPR::SEXPR_TYPE getType( const SEXPR::SEXPR& aSexpr )
{
    if( aSexpr.IsList() )
        return SEXPR::SEXPR_TYPE::SEXPR_TYPE_LIST;
    else if( aSexpr.IsSymbol() )
        return SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_SYMBOL;
    else if( aSexpr.IsString() )
        return SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_STRING;
    else if( aSexpr.IsDouble() )
        return SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_DOUBLE;
    else if( aSexpr.IsInteger() )
        return SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER;

    throw( std::invalid_argument( "<Unknown S-Expression Type!>" ) );
}

/**
 * Get a debug-friendly string for a given s-expr type
 */
inline std::string getDebugType( SEXPR::SEXPR_TYPE aType )
{
    switch( aType )
    {
    case SEXPR::SEXPR_TYPE::SEXPR_TYPE_LIST:
        return "List";
    case SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_SYMBOL:
        return "Symbol";
    case SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_STRING:
        return "String";
    case SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_DOUBLE:
        return "Double";
    case SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER:
        return "Integer";
    }

    return "<Unknown S-Expression Type!>";
}

inline std::string GetSexprDebugType( const SEXPR::SEXPR& aSexpr )
{
    return getDebugType( getType( aSexpr ) );
}

inline bool IsSexprOfType( const SEXPR::SEXPR& aSexpr, SEXPR::SEXPR_TYPE aType )
{
    if( getType( aSexpr ) != aType )
    {
        BOOST_TEST_MESSAGE( "Sexpr is not of type: " << getDebugType( aType ) );
        return false;
    }

    return true;
}

/**
 * Predicate to check two s-expr values (of the same type) are equal
 *
 * @return false if not equal (and output logging)
 */
template <typename VAL_T>
bool IsSexprValueEqual( const VAL_T& aGot, const VAL_T& aExpected )
{
    if( aGot != aExpected )
    {
        BOOST_TEST_MESSAGE( "Sexpr value not equal: got " << aGot << ", expected " << aExpected );
        return false;
    }

    return true;
}

/**
 * Test predicate: is the s-expression a symbol?
 */
inline bool SexprIsSymbol( const SEXPR::SEXPR& aSexpr )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_SYMBOL );
}

/**
 * Test predicate: is the s-expression a symbol with the given value?
 */
inline bool SexprIsSymbolWithValue( const SEXPR::SEXPR& aSexpr, const std::string& aVal )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_SYMBOL )
           && IsSexprValueEqual( aSexpr.GetSymbol(), aVal );
}

/**
 * Test predicate: is the s-expression a string?
 */
inline bool SexprIsString( const SEXPR::SEXPR& aSexpr )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_STRING );
}

/**
 * Test predicate: is the s-expression a string with the given value?
 */
inline bool SexprIsStringWithValue( const SEXPR::SEXPR& aSexpr, const std::string& aVal )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_STRING )
           && IsSexprValueEqual( aSexpr.GetString(), aVal );
}

/**
 * Test predicate: is the s-expression an integer?
 */
inline bool SexprIsInteger( const SEXPR::SEXPR& aSexpr )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER );
}

/**
 * Test predicate: is the s-expression an integer with the given value?
 */
inline bool SexprIsIntegerWithValue( const SEXPR::SEXPR& aSexpr, std::int64_t aVal )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER )
           && IsSexprValueEqual( aSexpr.GetLongInteger(), aVal );
}

/**
 * Test predicate: is the s-expression a double?
 */
inline bool SexprIsDouble( const SEXPR::SEXPR& aSexpr )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_DOUBLE );
}

/**
 * Test predicate: is the s-expression a double with the given value?
 */
inline bool SexprIsDoubleWithValue( const SEXPR::SEXPR& aSexpr, double aVal )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_ATOM_DOUBLE )
           && IsSexprValueEqual( aSexpr.GetDouble(), aVal );
}

/**
 * Test predicate: is the s-expression a double?
 */
inline bool SexprIsList( const SEXPR::SEXPR& aSexpr )
{
    return IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_LIST );
}

/**
 * Test predicate: is the s-expression a list with the given length?
 */
inline bool SexprIsListOfLength( const SEXPR::SEXPR& aSexpr, size_t aExpectedLength )
{
    if( !IsSexprOfType( aSexpr, SEXPR::SEXPR_TYPE::SEXPR_TYPE_LIST ) )
        return false;

    if( aSexpr.GetNumberOfChildren() != aExpectedLength )
    {
        BOOST_TEST_MESSAGE( "List is wrong length: got " << aSexpr.GetNumberOfChildren()
                                                         << ", expected " << aExpectedLength );
        return false;
    }

    return true;
}

/**
 * Predicate to check an SEXPR object converts to the expected string.
 * @param  aSexpr  s-expression
 * @param  aExpStr expected string
 * @return         true if match
 */
inline bool SexprConvertsToString( const SEXPR::SEXPR& aSexpr, const std::string& aExpStr )
{
    const std::string converted = aSexpr.AsString();

    bool ok = true;

    if( converted != aExpStr )
    {
        BOOST_TEST_INFO( "Sexpr string conversion mismatch: got '" << converted << "', expected '"
                                                                   << aExpStr << "'" );
        ok = false;
    }

    return ok;
}

} // namespace KI_TEST


namespace SEXPR
{
/**
 * Boost print helper for SEXPR objects
 */
inline std::ostream& boost_test_print_type( std::ostream& os, const SEXPR& aSexpr )
{
    os << "SEXPR [ " << KI_TEST::GetSexprDebugType( aSexpr ) << " ]\n    " << aSexpr.AsString();
    return os;
}

} // namespace SEXPR

#endif // TEST_SEXPR_TEST_UTILS__H