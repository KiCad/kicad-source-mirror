/**
 * @file am_param.h
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

#ifndef _AM_PARAM_H_
#define _AM_PARAM_H_

/*
 *  An aperture macro defines a complex shape and is a list of aperture primitives.
 *  Each aperture primitive defines a simple shape (circle, rect, regular polygon...)
 *  Inside a given aperture primitive, a fixed list of parameters defines info
 *  about the shape: size, thickness, number of vertex ...
 *
 *  Each parameter can be an immediate value or a deferred value.
 *  When value is deferred, it is defined when the aperture macro is instanced by
 *  an ADD macro command
 *
 *  Actual values of a parameter can also be the result of an arithmetic operation.
 *
 *  Here is some examples:
 *  An immediate value:
 *  3.5
 *  A deferred value:
 *  $2  means: replace me by the second value given in the ADD command
 *  Actual value as arithmetic calculation:
 *  $2/2+1
 *
 *  Note also a defer ed parameter can be defined in aperture macro,
 *  but outside aperture primitives. Example
 *  %AMRECTHERM*
 *  $4=$3/2*    parameter $4 is half value of parameter $3
 *  21,1,$1-$3,$2-$3,0-$1/2-$4,0-$2/2-$4,0*
 *  For the aperture primitive, parameters $1 to $3 will be defined in ADD command,
 *  and $4 is defined inside the macro
 *
 *  Some examples of aperture macro definition
 *  A simple definition, no parameters:
 *  %AMMOIRE10*
 *  6,0,0,0.350000,0.005,0.050,3,0.005,0.400000,0.0*%
 *  Example of instantiation:
 *  %ADD19MOIRE10*%
 *
 *  A simple definition, one parameter:
 *  %AMCIRCLE*
 *  1,1,$1,0,0*
 *  Example of instantiation:
 *  %ADD11CIRCLE,.5*%
 *
 *  A definition, with parameters and arithmetic operations:
 *  %AMVECTOR*
 *  2,1,$1,0,0,$2+1,$3,-135*%
 *  Example of instantiation:
 *  %ADD12VECTOR,0.05X0X0*%
 *
 *  A more complicated aperture macro definition, with parameters and arithmetic operations:
 *  %AMRNDREC*
 *  0 this is a comment*
 *  21,1,$1+$1,$2+$2-$3-$3,0,0,0*
 *  21,1,$1+$1-$3-$3,$2+$2,0,0,0*
 *  1,1,$3+$3,$1-$3,$2-$3*
 *  1,1,$3+$3,$3-$1,$2-$3*
 *  1,1,$3+$3,$1-$3,$3-$2*
 *  1,1,$3+$3,$3-$1,$3-$2*%
 *  Example of instantiation:
 *
 *  A more complicated sample of aperture macro definition:
 *  G04 Rectangular Thermal Macro, params: W/2, H/2, T/2 *
 *  %AMRECTHERM*
 *  $4=$3/2*
 *  21,1,$1-$3,$2-$3,0-$1/2-$4,0-$2/2-$4,0*
 *  21,1,$1-$3,$2-$3,0-$1/2-$4,$2/2+$4,0*
 *  21,1,$1-$3,$2-$3,$1/2+$4,0-$2/2-$4,0*
 *  21,1,$1-$3,$2-$3,$1/2+$4,$2/2+$4,0*%
 *  Example of instantiation:
 *  %ADD28RECTHERM,0.035591X0.041496X0.005000*%
 */

#include <vector>

#include <dcode.h>
/*
Values of a parameter can be the result of an arithmetic operation,
between immediate values and deferred value.
From an idea found in Gerbv, here is the way to evaluate a parameter.
a AM_PARAM_ITEM holds info about operands and operators in a parameter definition
( a AM_PARAM ) like $2+$2-$3-$3/2

Precedence was recently actually defined in gerber RS274X
(Previously, there was no actual info about this precedence)
This is the usual arithmetic precedence between + - x / ( ), the only ones used in Gerber

Before 2015 apr 10, actual value was calculated step to step:
no precedence, and '(' ')' are ignored.

Since 2015 apr 10 precedence is in use.

Parameter definition is described by a very primitive assembler.
This "program "should describe how to calculate the parameter.
The assembler consist of 10 instruction intended for a stackbased machine.
The instructions are:
NOP, PUSHVALUE, PUSHPARM, ADD, SUB, MUL, DIV, OPEN_PAR, CLOSE_PAR, EQUATE

The instructions
----------------
NOP  : The no operation. This is the default instruction and are
       added as a security measure.
PUSHVALUE : Pushes an arithmetical value on the stack. This machine only works with floats
       on the stack.
PUSHPARM: Pushes a deferred parameter onto the stack. Gerber aperture macros accepts
       parameters to be set when later declared, so the same macro can
       be used at several instances. Which parameter to be set is an integer
       and starts with 1. definition is like $1 or $3
ADD  : The mathematical operation +. Takes the two uppermost values on the
       the stack, adds them and pushes the result back onto the stack.
SUB  : Same as ADD, but with -.
MUL  : Same as ADD, but with *.
DIV  : Same as ADD, but with /.
OPEN_PAR  : Opening parenthesis: modify the precedence of operators by opening a local block.
CLOSE_PAR : Closing parenthesis: modify the precedence of operators by closing the local block.
POPVALUE : used when evaluate the expression: store current calculated value
*/

enum parm_item_type
{
    NOP, PUSHVALUE, PUSHPARM, ADD, SUB, MUL, DIV, OPEN_PAR, CLOSE_PAR, POPVALUE
};

/**
 * This helper class hold a value or an arithmetic operator to calculate
 * the final value of a aperture macro parameter, using usual
 * arithmetic operator precedence
 * Only operators ADD, SUB, MUL, DIV, OPEN_PAR, CLOSE_PAR have meaning when calculating
 * a value
 */
class AM_PARAM_EVAL
{
public:
    AM_PARAM_EVAL( parm_item_type aType )
            : m_type( aType), m_dvalue( 0.0 )
    {}

    AM_PARAM_EVAL( double aValue )
            : m_type( parm_item_type::NOP ), m_dvalue( aValue )
    {}

    parm_item_type GetType() const
    {
        return m_type;
    }

    bool IsOperator() const { return m_type != NOP; }
    double GetValue() const { return m_dvalue; }
    parm_item_type GetOperator() const { return m_type; }
    int GetPriority() const { return GetPriority( GetOperator() ); }

    static int GetPriority( parm_item_type aType )
    {
        switch( aType )
        {
        case ADD:
        case SUB:
            return 1;

        case MUL:
        case DIV:
            return 2;

        case OPEN_PAR:
        case CLOSE_PAR:
            return 3;

        default:
            break;
        }

        return 0;
    }

private:
    parm_item_type m_type;  // the type of item
    double m_dvalue;        // the value, for a numerical value
                            // used only when m_type == NOP
};

typedef std::vector<AM_PARAM_EVAL> AM_PARAM_EVAL_STACK;

/**
 * Hold an operand for an AM_PARAM as defined within standard RS274X.
 *
 * The \a value field can be a constant, i.e. "immediate" parameter or it may not be used if
 * this param is going to defer to the referencing aperture macro.  In that case, the \a index
 * field is an index into the aperture macro's parameters.
 */
class AM_PARAM_ITEM
{
public:
    AM_PARAM_ITEM( parm_item_type aType, double aValue )
    {
        m_type = aType;
        m_dvalue = aValue;
        m_ivalue = 0;
    }

    AM_PARAM_ITEM( parm_item_type aType, int aValue )
    {
        m_type = aType;
        m_dvalue = 0.0;
        m_ivalue = aValue;
    }

    void SetValue( double aValue )
    {
        m_dvalue = aValue;
    }

    double GetValue( ) const
    {
        return m_dvalue;
    }

    parm_item_type GetType() const
    {
        return m_type;
    }

    unsigned GetIndex() const
    {
        return (unsigned) m_ivalue;
    }

    bool IsOperator() const
    {
        return m_type == ADD || m_type == SUB || m_type == MUL || m_type == DIV;
    }
    bool IsOperand() const
    {
        return m_type == PUSHVALUE || m_type == PUSHPARM;
    }

    bool IsDefered() const
    {
        return m_type == PUSHPARM;
    }

private:
    parm_item_type m_type;      // the type of item
    double m_dvalue;            // the value, for PUSHVALUE type item
    int    m_ivalue;            // the integer value, for PUSHPARM type item
};

/**
 * Hold a parameter value for an "aperture macro" as defined within standard RS274X.
 *
 * The parameter can be a constant, i.e. "immediate" parameter, or depend on some deferred
 * values, defined in a D_CODE, by the ADD command.  Note the actual value could need an
 * evaluation from an arithmetical expression items in the expression are stored in.  A
 * simple definition is just a value stored in one item in m_paramStack.
 */
class AM_PARAM
{
public:
    AM_PARAM();

    /**
     * Add an operator/operand to the current stack.
     *
     * @param aType is the type of item (NOP, PUSHVALUE, PUSHPARM, ADD, SUB, MUL, DIV, EQUATE)
     * @param aValue is the item value, double for PUSHVALUE or int for PUSHPARM type.
     */
    void PushOperator( parm_item_type aType, double aValue );
    void PushOperator( parm_item_type aType, int aValue = 0);

    double GetValueFromMacro( APERTURE_MACRO* aApertureMacro ) const;

    /**
     * Test if this AM_PARAM holds an immediate parameter or is a pointer into a parameter held
     * by an owning D_CODE.
     *
     * @return true if the value is immediate, i.e. no deferred value in operands used in its
     *         definition.
     */
    bool IsImmediate() const;

    unsigned GetIndex() const
    {
        return (unsigned) m_index;
    }

    void SetIndex( int aIndex )
    {
        m_index = aIndex;
    }

    /**
     * Read one aperture macro parameter.
     *
     * a parameter can be:
     *      a number
     *      a reference to an aperture definition parameter value: $1 to $3 ...
     * a parameter definition can be complex and have operators between numbers and/or other
     * parameter
     * like $1+3 or $2x2..
     * Parameters are separated by a comma ( of finish by *)
     * @param aText = pointer to the parameter to read. Will be modified to point to the next field
     * @return true if a param is read, or false
     */
    bool ReadParamFromAmDef( char*& aText  );

private:
    /**
     * has meaning to define parameter local to an aperture macro
     * this is the id of a parameter defined like
     * $n = ....
     * n is the index
     */
    int    m_index;

    /**
     * List of operands/operators to evaluate the actual value if a par def is $3/2,
     * there are 3 items in stack: 3 (type PUSHPARM) , / (type DIV), 2 (type PUSHVALUE).
     */
    std::vector<AM_PARAM_ITEM> m_paramStack;

};

typedef std::vector<AM_PARAM> AM_PARAMS;

#endif  // _AM_PARAM_H_
