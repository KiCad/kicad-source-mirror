/*
 * Copyright (C) 2016 Mark Roszko <mark.roszko@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sexpr/sexpr.h"
#include <cctype>
#include <iterator>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <wx/debug.h>

namespace SEXPR
{
    SEXPR::SEXPR( SEXPR_TYPE aType, size_t aLineNumber ) :
        m_type( aType ), m_lineNumber( aLineNumber )
    {
    }

    SEXPR::SEXPR(SEXPR_TYPE aType) :
        m_type( aType ), m_lineNumber( 1 )
    {
    }

    SEXPR_VECTOR const * SEXPR::GetChildren() const
    {
        if( m_type != SEXPR_TYPE::SEXPR_TYPE_LIST )
        {
            throw INVALID_TYPE_EXCEPTION("SEXPR is not a list type!");
        }

        return &static_cast< SEXPR_LIST const * >(this)->m_children;
    }

    SEXPR* SEXPR::GetChild( size_t aIndex ) const
    {
        if( m_type != SEXPR_TYPE::SEXPR_TYPE_LIST )
        {
            throw INVALID_TYPE_EXCEPTION("SEXPR is not a list type!");
        }

        return static_cast< SEXPR_LIST const * >(this)->m_children[aIndex];
    }

    void SEXPR::AddChild( SEXPR* aChild )
    {
        if( m_type != SEXPR_TYPE::SEXPR_TYPE_LIST )
        {
            throw INVALID_TYPE_EXCEPTION("SEXPR is not a list type!");
        }

        SEXPR_LIST* list = static_cast< SEXPR_LIST * >( this );

        list->m_children.push_back( aChild );
    }

    size_t SEXPR::GetNumberOfChildren() const
    {
        if( m_type != SEXPR_TYPE::SEXPR_TYPE_LIST )
        {
            throw INVALID_TYPE_EXCEPTION("SEXPR is not a list type!");
        }

        return static_cast< SEXPR_LIST const * >(this)->m_children.size();
    }

    std::string const & SEXPR::GetString() const
    {
        if( m_type != SEXPR_TYPE::SEXPR_TYPE_ATOM_STRING )
        {
            throw INVALID_TYPE_EXCEPTION("SEXPR is not a string type!");
        }

        return static_cast< SEXPR_STRING const * >(this)->m_value;
    }

    int32_t SEXPR::GetInteger() const
    {
        return static_cast< int >( GetLongInteger() );
    }

    int64_t SEXPR::GetLongInteger() const
    {
        if( m_type != SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER )
        {
            throw INVALID_TYPE_EXCEPTION("SEXPR is not a integer type!");
        }

        return static_cast< SEXPR_INTEGER const * >(this)->m_value;
    }

    double SEXPR::GetDouble() const
    {
        // we may end up parsing "intended" floats/doubles as ints
        // so here we allow silent casting back to doubles
        if( m_type == SEXPR_TYPE::SEXPR_TYPE_ATOM_DOUBLE )
        {
            return static_cast< SEXPR_DOUBLE const * >(this)->m_value;
        }
        else if( m_type == SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER )
        {
            return static_cast< SEXPR_INTEGER const * >(this)->m_value;
        }
        else
        {
            throw INVALID_TYPE_EXCEPTION("SEXPR is not a double type!");
        }
    }

    float SEXPR::GetFloat() const
    {
        return static_cast< float >( GetDouble() );
    }

    std::string const & SEXPR::GetSymbol() const
    {
        if( m_type != SEXPR_TYPE::SEXPR_TYPE_ATOM_SYMBOL )
        {
            std::string err_msg( "GetSymbol(): SEXPR is not a symbol type! error line ");
            err_msg += std::to_string( GetLineNumber() );
            throw INVALID_TYPE_EXCEPTION( err_msg );
        }

        return static_cast< SEXPR_SYMBOL const * >(this)->m_value;
    }


    SEXPR_LIST* SEXPR::GetList()
    {
        if( m_type != SEXPR_TYPE::SEXPR_TYPE_LIST )
        {
            throw INVALID_TYPE_EXCEPTION("SEXPR is not a list type!");
        }

        return static_cast< SEXPR_LIST* >(this);
    }

    std::string SEXPR::AsString( size_t aLevel ) const
    {
        std::string result;

        if( IsList() )
        {
            if( aLevel != 0 )
            {
                result = "\n";
            }

            result.append( aLevel * 2, ' ' );
            aLevel++;
            result += "(";

            SEXPR_VECTOR const* list = GetChildren();

            for( std::vector<SEXPR *>::const_iterator it = list->begin(); it != list->end(); ++it )
            {
                result += (*it)->AsString( aLevel );

                if( it != list->end() - 1 )
                {
                    result += " ";
                }
            }
            result += ")";

            aLevel--;
        }
        else if( IsString() )
        {
            result += "\"" + GetString() + "\"";
        }
        else if( IsSymbol() )
        {
            result += GetSymbol();
        }
        else if( IsInteger() )
        {
            std::stringstream out;
            out << GetInteger();
            result += out.str();
        }
        else if( IsDouble() )
        {
            std::stringstream out;
            out << std::setprecision( 16 ) << GetDouble();
            result += out.str();
        }

        return result;
    }

    SEXPR_LIST::~SEXPR_LIST()
    {
        for( auto child : m_children )
        {
            delete child;
        }

        m_children.clear();
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, const ISEXPRABLE& obj )
    {
        SEXPR* sobj = obj.SerializeSEXPR();
        list.AddChild( sobj );

        return list;
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, int64_t value )
    {
        list.AddChild( new SEXPR_INTEGER( value ) );
        return list;
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, int32_t value )
    {
        list.AddChild( new SEXPR_INTEGER( value ) );
        return list;
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, float value )
    {
        list.AddChild( new SEXPR_DOUBLE( value ) );
        return list;
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, double value )
    {
        list.AddChild( new SEXPR_DOUBLE( value ) );
        return list;
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, std::string value )
    {
        list.AddChild( new SEXPR_STRING( value ) );
        return list;
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, SEXPR* obj )
    {
        list.AddChild( obj );
        return list;
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, const _OUT_STRING setting )
    {
        SEXPR *res;

        if( setting._Symbol )
        {
            res = new SEXPR_SYMBOL( setting._String );
        }
        else
        {
            res = new SEXPR_STRING( setting._String );
        }

        list.AddChild( res );

        return list;
    }

    SEXPR_LIST& operator>> ( SEXPR_LIST& input, ISEXPRABLE& obj )
    {
        obj.DeserializeSEXPR( input );

        return input;
    }

    SEXPR_LIST& operator>> ( SEXPR_LIST& input, int32_t& inte )
    {
        SEXPR* child = input.GetChild( input.m_inStreamChild );

        if( child->IsInteger() )
        {
            inte = child->GetInteger();
            input.m_inStreamChild++;
        }
        else
        {
            throw std::invalid_argument( "SEXPR is not a integer type!" );
        }

        return input;
    }

    SEXPR_LIST& operator>> ( SEXPR_LIST& input, std::string& str )
    {
        SEXPR* child = input.GetChild( input.m_inStreamChild );

        if( child->IsString() || child->IsSymbol() )
        {
            str = child->GetString();
            input.m_inStreamChild++;
        }
        else
        {
            throw std::invalid_argument( "SEXPR is not a string type!" );
        }

        return input;
    }

    SEXPR_LIST& operator>> ( SEXPR_LIST& input, int64_t& lint )
    {
        SEXPR* child = input.GetChild( input.m_inStreamChild );

        if( child->IsInteger() )
        {
            lint = child->GetLongInteger();
            input.m_inStreamChild++;
        }
        else
        {
            throw std::invalid_argument("SEXPR is not a long integer type!");
        }

        return input;
    }

    SEXPR_LIST& operator>> ( SEXPR_LIST& input, float& fl )
    {
        SEXPR* child = input.GetChild( input.m_inStreamChild );
        if( child->IsDouble() )
        {
            fl = child->GetFloat();
            input.m_inStreamChild++;
        }
        else
        {
            throw std::invalid_argument( "SEXPR is not a float type!" );
        }

        return input;
    }

    SEXPR_LIST& operator>> ( SEXPR_LIST& input, double& dbl )
    {
        SEXPR* child = input.GetChild( input.m_inStreamChild );

        if( child->IsDouble() )
        {
            dbl = child->GetDouble();
            input.m_inStreamChild++;
        }
        else
        {
            throw std::invalid_argument( "SEXPR is not a double type!" );
        }

        return input;
    }

    SEXPR_LIST& operator>> ( SEXPR_LIST& input, const _IN_STRING is )
    {
        SEXPR* child = input.GetChild( input.m_inStreamChild );

        if( is._Symbol )
        {
            if( child->IsSymbol() )
            {
                is._String = child->GetSymbol();
                input.m_inStreamChild++;
            }
            else
            {
                throw std::invalid_argument( "operator>>: SEXPR is not a symbol type!" );
            }
        }
        else
        {
            if( child->IsString() )
            {
                is._String = child->GetString();
                input.m_inStreamChild++;
            }
            else
            {
                throw std::invalid_argument( "SEXPR is not a string type!" );
            }
        }

        return input;
    }

    SEXPR_LIST& operator<< ( SEXPR_LIST& list, SEXPR_LIST* list2 )
    {
        list.AddChild( list2 );

        return list;
    }

	size_t SEXPR_LIST::doScan( const SEXPR_SCAN_ARG *args, size_t num_args )
	{
		size_t i = 0;

		for( i = 0; i < num_args; i++ )
		{
			SEXPR* child = GetChild( i );
			const SEXPR_SCAN_ARG& arg = args[i];

			try
			{
				if( arg.type == SEXPR_SCAN_ARG::Type::DOUBLE )
				{
					*arg.u.dbl_value = child->GetDouble();
				}
				else if( arg.type == SEXPR_SCAN_ARG::Type::INT )
				{
					*arg.u.dbl_value = child->GetInteger();
				}
				else if( arg.type == SEXPR_SCAN_ARG::Type::STRING )
				{
					if( child->IsSymbol() )
					{
						*arg.u.str_value = child->GetSymbol();
					}
					else if( child->IsString() )
					{
						*arg.u.str_value = child->GetString();
					}
				}
				else if( arg.type == SEXPR_SCAN_ARG::Type::LONGINT )
				{
					*arg.u.lint_value = child->GetLongInteger();
				}
				else if( arg.type == SEXPR_SCAN_ARG::Type::SEXPR_STRING )
				{
					if( arg.u.sexpr_str->_Symbol )
					{
						arg.u.sexpr_str->_String = child->GetSymbol();
					}
					else
					{
						arg.u.sexpr_str->_String = child->GetString();
					}

				}
				else if( arg.type == SEXPR_SCAN_ARG::Type::STRING_COMP )
				{
					if( child->IsSymbol() )
					{
						if( child->GetSymbol() != arg.str_value )
						{
							return i;
						}
					}
					else if( child->IsString() )
					{
						if( child->GetString() != arg.str_value )
						{
							return i;
						}
					}
				}
				else
				{
					throw std::invalid_argument( "unsupported argument type, this shouldn't have happened" );
				}
			}
			catch( const INVALID_TYPE_EXCEPTION& )
			{
				return i;
			}
		}

		return i;
	}

	void SEXPR_LIST::doAddChildren( const SEXPR_CHILDREN_ARG *args, size_t num_args )
	{
		size_t i = 0;

		for( i = 0; i < num_args; i++ )
		{
			const SEXPR_CHILDREN_ARG& arg = args[i];

			if( arg.type == SEXPR_CHILDREN_ARG::Type::DOUBLE )
			{
				AddChild( new SEXPR_DOUBLE( arg.u.dbl_value ) );
			}
			else if( arg.type == SEXPR_CHILDREN_ARG::Type::INT )
			{
				AddChild( new SEXPR_INTEGER( arg.u.int_value ) );
			}
			else if( arg.type == SEXPR_CHILDREN_ARG::Type::LONGINT )
			{
				AddChild( new SEXPR_INTEGER( arg.u.lint_value ) );
			}
			else if( arg.type == SEXPR_CHILDREN_ARG::Type::STRING )
			{
				AddChild( new SEXPR_STRING( arg.str_value ) );
			}
			else if( arg.type == SEXPR_CHILDREN_ARG::Type::SEXPR_ATOM )
			{
				AddChild( arg.u.sexpr_ptr );
			}
			else if( arg.type == SEXPR_CHILDREN_ARG::Type::SEXPR_STRING )
			{
				if( arg.u.symbol )
				{
					AddChild( new SEXPR_SYMBOL( arg.str_value ) );
				}
				else
				{
					AddChild( new SEXPR_STRING( arg.str_value ) );
				}
			}
			else
			{
				throw std::invalid_argument( "unexpected argument type, this shouldn't have happened" );
			}
		}
	}
}
