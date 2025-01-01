/**
 * @file evaluate.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean-Pierre Charras <jp.charras at wanadoo.fr>
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

/* How to evaluate an arithmetic expression like those used in Aperture Macro Definition in Gerber?
 *
 *  See http://stackoverflow.com/questions/28256/equation-expression-parser-with-precedence
 *
 *  The shunting yard algorithm is the right tool for this.
 *  Wikipedia is really confusing about this, but basically the algorithm works like this:
 *
 *  Say, you want to evaluate 1 + 2 * 3 + 4. Intuitively, you "know" you have to do the 2 * 3 first,
 *  but how do you get this result?
 *  The key is to realize that when you're scanning the string from left to right, you will evaluate
 *  an operator when the operator that follows it has a lower (or equal to) precedence.
 *
 *  In the context of the example, here's what you want to do:
 *
 *   Look at: 1 + 2, don't do anything.
 *   Now look at 1 + 2 * 3, still don't do anything.
 *   Now look at 1 + 2 * 3 + 4, now you know that 2 * 3 has to to be evaluated because
 *  the next operator has lower precedence.
 *
 *  How do you implement this?
 *
 *  You want to have two stacks, one for numbers, and another for operators.
 *  You push numbers onto the stack all the time.
 *  You compare each new operator with the one at the top of the stack,
 *  if the one on top of the stack has higher priority, you pop it off the operator stack,
 *  pop the operands off the number stack, apply the operator and push the result onto the number stack.
 *
 *  Now you repeat the comparison with the top of stack operator.
 *
 *  Coming back to the example, it works like this:
 *
 *  N = [ ] Ops = [ ]
 *
 *   Read 1. N = [1], Ops = [ ]
 *   Read +. N = [1], Ops = [+]
 *   Read 2. N = [1 2], Ops = [+]
 *   Read *. N = [1 2], Ops = [+ *]
 *   Read 3. N = [1 2 3], Ops = [+ *]
 *   Read +. N = [1 2 3], Ops = [+ *]
 *      Pop 3, 2 and execute 2*3, and push result onto N. N = [1 6], Ops = [+]
 *      is left associative, so you want to pop 1, 6 off as well and execute the +. N = [7], Ops = [].
 *       Finally push the [+] onto the operator stack. N = [7], Ops = [+].
 *   Read 4. N = [7 4]. Ops = [+].
 *
 *  You're run out off input, so you want to empty the stacks now.
 *  Upon which you will get the result 11.
 */

#include <am_param.h>

/**
 * Evaluate an basic arithmetic expression (infix notation) with precedence
 * The expression is a sequence of numbers (double) and arith operators:
 * operators are + - x / ( and )
 * the expression is stored in a std::vector
 * each item is a AM_PARAM_EVAL (each item is an operator or a double)
 * @param aExp = the arithmetic expression to evaluate
 * @return the value
 */

 /*
 The instructions ( subset of parm_item_type)
----------------
NOP  : The no operation. the AM_PARAM_EVAL item stores a value.
ADD
SUB
MUL
DIV
OPEN_PAR  : Opening parenthesis: modify the precedence of operators inside ( and )
CLOSE_PAR : Closing parenthesis: modify the precedence of operators by closing the local block.
POPVALUE : used to initialize a sequence
*/

double Evaluate( AM_PARAM_EVAL_STACK& aExp )
{
    class OP_CODE   // A small class to store a operator and its priority
    {
    public:
        parm_item_type m_Optype;
        int m_Priority;

        OP_CODE( AM_PARAM_EVAL& aAmPrmEval )
            : m_Optype( aAmPrmEval.GetOperator() ),
              m_Priority( aAmPrmEval.GetPriority() )
        {}

        OP_CODE( parm_item_type aOptype )
            : m_Optype( aOptype ), m_Priority( 0 )
        {}
    };

    double result = 0.0;

    std::vector<double> values;     // the current list of values
    std::vector<OP_CODE> optype;    // the list of arith operators

    double curr_value = 0.0;
    int extra_priority = 0;

    for( unsigned ii = 0; ii < aExp.size(); ii++ )
    {
        AM_PARAM_EVAL& prm = aExp[ii];

        if( prm.IsOperator() )
        {
            if( prm.GetOperator() == OPEN_PAR )
            {
                extra_priority += AM_PARAM_EVAL::GetPriority( OPEN_PAR );
            }
            else if( prm.GetOperator() == CLOSE_PAR )
            {
                extra_priority -= AM_PARAM_EVAL::GetPriority( CLOSE_PAR );
            }
            else
            {
                optype.emplace_back( prm );
                optype.back().m_Priority += extra_priority;
            }
        }
        else    // we have a value:
        {
            values.push_back( prm.GetValue() );

            if( optype.size() < 2 )
                continue;

            OP_CODE& previous_optype = optype[optype.size() - 2];

            if( optype.back().m_Priority > previous_optype.m_Priority )
            {
                double op1 = 0.0;

                double op2 = values.back();
                values.pop_back();

                if( values.size() )
                {
                    op1 = values.back();
                    values.pop_back();
                }

                switch( optype.back().m_Optype )
                {
                    case ADD:
                        values.push_back( op1+op2 );
                        break;

                    case SUB:
                        values.push_back( op1-op2 );
                        break;

                    case MUL:
                        values.push_back( op1*op2 );
                        break;

                    case DIV:
                        values.push_back( op1/op2 );
                        break;

                    default:
                        break;
                }

                optype.pop_back();
            }
        }
    }

    // Now all operators have the same priority, or those having the higher priority
    // are before others, calculate the final result by combining initial values and/or
    // replaced values.
    if( values.size() > optype.size() )
        // If there are n values, the number of operator is n-1 or n if the first
        // item of the expression to evaluate is + or - (like -$1/2)
        // If the number of operator is n-1 the first value is just copied to result
        optype.insert( optype.begin(), OP_CODE( POPVALUE ) );

    wxASSERT( values.size() == optype.size() );

    for( unsigned idx = 0; idx < values.size(); idx++ )
    {
        curr_value = values[idx];

        switch( optype[idx].m_Optype )
        {
            case POPVALUE:
                result = curr_value;
                break;

            case ADD:
                result += curr_value;
                break;

            case SUB:
                result -= curr_value;
                break;

            case MUL:
                result *= curr_value;
                break;

            case DIV:
                result /= curr_value;
                break;

            default:
                break;
        }
    }

    return result;
}