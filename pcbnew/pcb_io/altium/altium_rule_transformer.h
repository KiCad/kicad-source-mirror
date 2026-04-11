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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ALTIUM_RULE_TRANSFORMER_H
#define ALTIUM_RULE_TRANSFORMER_H

#include "wx/string.h"

// See: https://www.altium.com/documentation/altium-designer/query-operators-ad
enum class ALTIUM_RULE_TOKEN_KIND
{
    UNKNOWN,

    IDENT,
    CONST_INT,
    CONST_FLOAT,
    CONST_STRING,

    CONST_TRUE,
    CONST_FALSE,

    LPAR,
    RPAR,

    ADD,
    SUB,
    MUL,
    DIV,
    INTEGRAL_DIV,
    MOD,

    AND,
    LOW_AND,
    OR,
    LOW_OR,
    XOR,
    NOT,

    LESS,
    LESS_EQUAL,
    GREATER_EQUAL,
    GREATER,
    NOT_EQUAL,
    EQUAL,

    BETWEEN,
    LIKE,

    END_OF_EXPR
};

struct ALTIUM_RULE_TOKEN
{
    ALTIUM_RULE_TOKEN_KIND kind;
    size_t                 pos;

    long     iValue;
    double   fValue;
    wxString sValue;

    ALTIUM_RULE_TOKEN() :
            kind( ALTIUM_RULE_TOKEN_KIND::UNKNOWN ), pos( 0 ), iValue( 0 ), fValue( 0. ),
            sValue( "" )
    {
    }

    ALTIUM_RULE_TOKEN( ALTIUM_RULE_TOKEN_KIND aKind, size_t aPos ) :
            kind( aKind ), pos( aPos ), iValue( 0 ), fValue( 0. ), sValue( "" )
    {
    }

    ALTIUM_RULE_TOKEN( ALTIUM_RULE_TOKEN_KIND aKind, size_t aPos, long aValue ) :
            kind( aKind ), pos( aPos ), iValue( aValue ), fValue( 0. ), sValue( "" )
    {
    }

    ALTIUM_RULE_TOKEN( ALTIUM_RULE_TOKEN_KIND aKind, size_t aPos, float aValue ) :
            kind( aKind ), pos( aPos ), iValue( 0 ), fValue( aValue ), sValue( "" )
    {
    }

    ALTIUM_RULE_TOKEN( ALTIUM_RULE_TOKEN_KIND aKind, size_t aPos, wxString aValue ) :
            kind( aKind ), pos( aPos ), iValue( 0 ), fValue( 0. ), sValue( aValue )
    {
    }
};

class ALTIUM_RULE_TOKENIZER
{
public:
    ALTIUM_RULE_TOKENIZER( const wxString& aExpr ) : m_pos( 0 ), m_expr( aExpr )
    {
        m_it = m_expr.begin();
        Next();
    }

    const ALTIUM_RULE_TOKEN& Next();

    const ALTIUM_RULE_TOKEN& Peek() const;

private:
    wxUniChar curChar() { return *m_it; }

    wxUniChar nextChar()
    {
        if( m_it != m_expr.end() )
        {
            m_it++;
            m_pos++;

            if( m_it != m_expr.end() )
                return *( m_it );
        }

        return wxUniChar();
    }

    size_t                   m_pos;
    const wxString           m_expr;
    wxString::const_iterator m_it;

    ALTIUM_RULE_TOKEN m_currentToken;
    ALTIUM_RULE_TOKEN m_nextToken;
};


#endif //ALTIUM_RULE_TRANSFORMER_H