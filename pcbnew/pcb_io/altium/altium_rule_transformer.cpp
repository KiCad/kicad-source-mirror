/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include "altium_rule_transformer.h"

#include <iostream>
#include <wx/wxcrt.h>   // for wxIsspace

const ALTIUM_RULE_TOKEN& ALTIUM_RULE_TOKENIZER::Next()
{
    m_currentToken = m_nextToken;

    // skip whitespaces
    for( ; m_it != m_expr.end() && wxIsspace( curChar() ); nextChar() )
        ;

    // check for end of string
    if( m_it == m_expr.end() )
    {
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, m_pos };
        return m_currentToken;
    }

    const size_t    startPos = m_pos;
    const wxUniChar curCh = curChar();
    wxUniChar       nextCh = nextChar();

    if( curCh == '(' )
    {
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::LPAR, startPos };
        return m_currentToken;
    }
    else if( curCh == ')' )
    {
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::RPAR, startPos };
        return m_currentToken;
    }
    else if( curCh == '*' )
    {
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::MUL, startPos };
        return m_currentToken;
    }
    else if( curCh == '/' )
    {
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::DIV, startPos };
        return m_currentToken;
    }
    else if( curCh == '=' )
    {
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::EQUAL, startPos };
        return m_currentToken;
    }
    else if( curCh == '<' )
    {
        if( nextCh == '=' )
        {
            nextChar();
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::LESS_EQUAL, startPos };
        }
        else if( nextCh == '>' )
        {
            nextChar();
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::NOT_EQUAL, startPos };
        }
        else
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::LESS, startPos };
        }
        return m_currentToken;
    }
    else if( curCh == '>' )
    {
        if( nextCh == '=' )
        {
            nextChar();
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::GREATER_EQUAL, startPos };
        }
        else
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::GREATER, startPos };
        }
        return m_currentToken;
    }
    else if( curCh == '&' && nextCh == '&' )
    {
        nextChar();
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::LOW_AND, startPos };
        return m_currentToken;
    }
    else if( curCh == '|' && nextCh == '|' )
    {
        nextChar();
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::LOW_OR, startPos };
        return m_currentToken;
    }
    else if( curCh == '\'' )
    {
        wxString constString;
        while( m_it != m_expr.end() && nextCh != '\'' )
        {
            constString += nextCh; // TODO: escaping?
            nextCh = nextChar();
        }

        if( m_it != m_expr.end() )
        {
            nextChar(); // TODO: exception if EOF reached?
        }

        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::CONST_STRING, startPos, constString };
        return m_currentToken;
    }
    else if( curCh == '+' && !wxIsdigit( nextCh ) )
    {
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::ADD, startPos };
        return m_currentToken;
    }
    else if( curCh == '-' && !wxIsdigit( nextCh ) )
    {
        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::SUB, startPos };
        return m_currentToken;
    }
    else if( curCh == '+' || curCh == '-' || wxIsdigit( curCh ) )
    {
        wxString digitString = curCh;
        while( wxIsdigit( nextCh ) )
        {
            digitString += nextCh;
            nextCh = nextChar();
        }

        long value;
        digitString.ToLong( &value ); // TODO: check return value

        m_nextToken = { ALTIUM_RULE_TOKEN_KIND::CONST_INT, startPos, value };
        return m_currentToken;
    }
    else
    {
        wxString identString = curCh;
        while( wxIsalnum( nextCh ) )
        {
            identString += nextCh;
            nextCh = nextChar();
        }

        if( identString.IsSameAs( wxT( "True" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::CONST_TRUE, startPos };
        }
        else if( identString.IsSameAs( wxT( "False" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::CONST_FALSE, startPos };
        }
        else if( identString.IsSameAs( wxT( "Div" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::INTEGRAL_DIV, startPos };
        }
        else if( identString.IsSameAs( wxT( "Mod" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::MOD, startPos };
        }
        else if( identString.IsSameAs( wxT( "And" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::AND, startPos };
        }
        else if( identString.IsSameAs( wxT( "Or" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::OR, startPos };
        }
        else if( identString.IsSameAs( wxT( "Xor" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::XOR, startPos };
        }
        else if( identString.IsSameAs( wxT( "Not" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::NOT, startPos };
        }
        else if( identString.IsSameAs( wxT( "Between" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::BETWEEN, startPos };
        }
        else if( identString.IsSameAs( wxT( "Like" ), false ) )
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::LIKE, startPos };
        }
        else
        {
            m_nextToken = { ALTIUM_RULE_TOKEN_KIND::IDENT, startPos, identString };
        }

        return m_currentToken;
    }
}

const ALTIUM_RULE_TOKEN& ALTIUM_RULE_TOKENIZER::Peek() const
{
    return m_nextToken;
}