/*****************/
/* am_param.cpp */
/*****************/

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#include "class_am_param.h"
#include "class_aperture_macro.h"

extern int    ReadInt( char*& text, bool aSkipSeparator = true );
extern double ReadDouble( char*& text, bool aSkipSeparator = true );

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
    bool isimmediate = true;
    for( unsigned ii = 0; ii < m_paramStack.size(); ii++ )
    {
        if( m_paramStack[ii].IsDefered() )
        {   // a defered value is found in operand list,
            // so the parameter is not immediate
            isimmediate = false;
            break;
        }
    }
    return isimmediate;
}

double AM_PARAM::GetValue( const D_CODE* aDcode ) const
{
    double paramvalue = 0.0;
    double curr_value = 0.0;
    parm_item_type state = POPVALUE;
    for( unsigned ii = 0; ii < m_paramStack.size(); ii++ )
    {
        AM_PARAM_ITEM item = m_paramStack[ii];
        switch( item.GetType() )
        {
            case ADD:
            case SUB:
            case MUL:
            case DIV:   // just an operator for next parameter value: store it
                state = item.GetType();
                break;

            case PUSHPARM:
                // get the parameter from the aDcode
                if( aDcode && item.GetIndex() <= aDcode->GetParamCount() )
                    curr_value = aDcode->GetParam( item.GetIndex() );
                else    // Get parameter from local param definition
                {
                    const APERTURE_MACRO * am_parent = aDcode->GetMacro();
                    curr_value = am_parent->GetLocalParam( aDcode, item.GetIndex() );
                }
                // Fall through
            case PUSHVALUE: // a value is on the stack:
                if( item.GetType() == PUSHVALUE )
                    curr_value = item.GetValue();
                switch( state )
                {
                    case POPVALUE:
                        paramvalue = curr_value;
                        break;

                    case ADD:
                        paramvalue += curr_value;
                        break;

                    case SUB:
                        paramvalue -= curr_value;
                        break;

                    case MUL:
                        paramvalue *= curr_value;
                        break;

                    case DIV:
                        paramvalue /= curr_value;
                        break;

                    default:
                        wxLogDebug( wxT( "AM_PARAM::GetValue() : unexpected operator\n" ) );
                        break;
                }
                break;

            default:
                wxLogDebug( wxT( "AM_PARAM::GetValue(): unexpected type\n" ) );
                break;
        }
    }
    return paramvalue;
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
bool AM_PARAM::ReadParam( char*& aText  )
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
                // fall through
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
