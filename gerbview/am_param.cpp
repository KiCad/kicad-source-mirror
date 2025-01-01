/**
 * @file am_param.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <am_param.h>
#include <am_primitive.h>
#include <aperture_macro.h>
#include <macros.h>

extern int    ReadInt( char*& text, bool aSkipSeparator = true );
extern double ReadDouble( char*& text, bool aSkipSeparator = true );
extern double Evaluate( AM_PARAM_EVAL_STACK& aExp );

/* Class AM_PARAM
 * holds a parameter value for an "aperture macro" as defined within
 * standard RS274X.  The parameter can be a constant, i.e. "immediate" parameter,
 * or depend on some defered values, defined in a D_CODE, by the ADD command.
 * Note the actual value could need an evaluation from an arithmetical expression
 * items in the expression are stored in .
 * A simple definition is just a value stored in one item in m_paramStack
 */
AM_PARAM::AM_PARAM( )
{
    m_index = -1;
}

/**
 * Function IsImmediate
 * tests if this AM_PARAM holds an immediate parameter or has parameter
 * held by an owning D_CODE.
 */
bool AM_PARAM::IsImmediate() const
{
    bool is_immediate = true;
    for( unsigned ii = 0; ii < m_paramStack.size(); ii++ )
    {
        if( m_paramStack[ii].IsDefered() )
        {   // a defered value is found in operand list,
            // so the parameter is not immediate
            is_immediate = false;
            break;
        }
    }
    return is_immediate;
}


double AM_PARAM::GetValueFromMacro( APERTURE_MACRO* aApertureMacro ) const
{
    // In macros, actual values are sometimes given by an expression like:
    // 0-$2/2-$4
    // Because arithmetic predence is used, the parameters (values (double) and operators)
    // are stored in a stack, with all numeric values converted to the actual values
    // when they are defered parameters
    // Each item is stored in a AM_PARAM_EVAL (a value or an operator)
    //
    // Then the stack with all values resolved is parsed and numeric values
    // calculated according to the precedence of operators
    double curr_value = 0.0;
    parm_item_type op_code;

    AM_PARAM_EVAL_STACK ops;

    for( unsigned ii = 0; ii < m_paramStack.size(); ii++ )
    {
        AM_PARAM_ITEM item = m_paramStack[ii];

        switch( item.GetType() )
        {
            case ADD:
            case SUB:
            case MUL:
            case DIV:       // just an operator for next parameter value
            case OPEN_PAR:
            case CLOSE_PAR: // Priority modifiers: store in stack
                op_code = item.GetType();
                ops.emplace_back( op_code );
                break;

            case PUSHPARM:
                // a defered value: get the actual parameter from the aperture macro
                if( aApertureMacro )    // should be always true here
                {
                    // Get the actual value
                    curr_value = aApertureMacro->GetLocalParamValue( item.GetIndex() );
               }
                else
                {
                    wxFAIL_MSG( wxT( "AM_PARAM::GetValue(): NULL param aApertureMacro" ) );
                }

                ops.emplace_back( curr_value );
                break;

            case PUSHVALUE: // a value is on the stack:
                curr_value = item.GetValue();
                ops.emplace_back( curr_value );
                break;

            default:
                wxFAIL_MSG( wxString::Format( wxT( "AM_PARAM::GetValue(): unexpected prm type %d" ),
                                                   item.GetType() ) );
                break;
        }
    }

    double result = Evaluate( ops );

    return result;
}


/**
 * add an operator/operand to the current stack
 * aType = NOP, PUSHVALUE, PUSHPARM, ADD, SUB, MUL, DIV, EQUATE
 * aValue required only for PUSHVALUE (double) or PUSHPARM (int) aType.
 */
void AM_PARAM::PushOperator( parm_item_type aType, double aValue )
{
    AM_PARAM_ITEM item( aType, aValue);
    m_paramStack.push_back( item );
}

void AM_PARAM::PushOperator( parm_item_type aType, int aValue )
{
    AM_PARAM_ITEM item( aType, aValue);
    m_paramStack.push_back( item );
}

/**
 * Function ReadParam
 * Read one aperture macro parameter
 * a parameter can be:
 *      a number
 *      a reference to an aperture definition parameter value: $1 ot $3 ...
 * a parameter definition can be complex and have operators between numbers and/or other parameter
 * like $1+3 or $2x2..
 * Note minus sign is not always an operator. It can be the sign of a value.
 * Parameters are separated by a comma ( of finish by *)
 * @param aText = pointer to the parameter to read. Will be modified to point to the next field
 * @return true if a param is read, or false
 */
bool AM_PARAM::ReadParamFromAmDef( char*& aText  )
{
    bool found = false;
    int ivalue;
    double dvalue;
    bool end = false;

    while( !end )
    {
        switch( *aText )
        {
        case ',':
            aText++;

            if( !found )    // happens when a string starts by ',' before any param
                break;      // just skip this separator

            KI_FALLTHROUGH;

        case '\n':
        case '\r':
        case 0:     // EOL
        case '*':   // Terminator in a gerber command
            end = true;
            break;

        case ' ':
            aText++;
            break;

        case '$':
            // defered value defined later, in ADD command which define defered parameters
            ++aText;
            ivalue = ReadInt( aText, false );

            if( m_index < 1 )
                SetIndex( ivalue );

            PushOperator( PUSHPARM, ivalue );
            found = true;
            break;

        case '/':
            PushOperator( DIV );
            aText++;
            break;

        case '(':   // Open a block to evaluate an expression between '(' and ')'
            PushOperator( OPEN_PAR );
            aText++;
            break;

        case ')':   // close a block between '(' and ')'
            PushOperator( CLOSE_PAR );
            aText++;
            break;

        case 'x':
        case 'X':
            PushOperator( MUL );
            aText++;
            break;

        case '-':
        case '+':
            // Test if this is an operator between 2 params, or the sign of a value
            if( m_paramStack.size() > 0 && !m_paramStack.back().IsOperator() )
            {   // Seems an operator
                PushOperator( *aText == '+' ? ADD : SUB );
                aText++;
            }
            else
            {   // seems the sign of a value
                dvalue = ReadDouble( aText, false );
                PushOperator( PUSHVALUE, dvalue );
                found = true;
            }
            break;

        case '=':   // A local definition found like $4=$3/2
            // At this point, one defered parameter is expected to be read.
            // this parameter value (the index) is stored in m_index.
            // The list of items is cleared
            aText++;
            m_paramStack.clear();
            found = false;
            break;

        default:
            dvalue = ReadDouble( aText, false );
            PushOperator( PUSHVALUE, dvalue );
            found = true;
            break;
        }
    }

    return found;
}
