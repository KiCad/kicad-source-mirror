/*****************/
/* am_param.h */
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

#ifndef _AM_PARAM_H_
#define _AM_PARAM_H_

/*
 *  An aperture macro defines a complex shape and is a list of aperture primitives.
 *  Each aperture primitive defines a simple shape (circle, rect, regular polygon...)
 *  Inside a given aperture primitive, a fixed list of parameters defines info
 *  about the shape: size, thickness, number of vertex ...
 *
 *  Each parameter can be an immediate value or a defered value.
 *  When value is defered, it is defined when the aperture macro is instancied by
 *  an ADD macro command
 *
 *  Actual values of a parameter can also be the result of an arithmetic operation.
 *
 *  Here is some examples:
 *  An immediate value:
 *  3.5
 *  A deferend value:
 *  $2  means: replace me by the second value given in the ADD command
 *  Actual value as arithmetic calculation:
 *  $2/2+1
 *
 *  Note also a defered parameter can be defined in aperture macro,
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
 *  Example of instanciation:
 *  %ADD19THERM19*%
 *
 *  A simple definition, one parameter:
 *  %AMCIRCLE*
 *  1,1,$1,0,0*
 *  Example of instanciation:
 *  %ADD11CIRCLE,.5*%
 *
 *  A definition, with parameters and arithmetic operations:
 *  %AMVECTOR*
 *  2,1,$1,0,0,$2+1,$3,-135*%
 *  Example of instanciation:
 *  %ADD12VECTOR,0.05X0X0*%
 *
 *  A more complicated aperture macro definition, with parameters and arihmetic operations:
 *  %AMRNDREC*
 *  0 this is a comment*
 *  21,1,$1+$1,$2+$2-$3-$3,0,0,0*
 *  21,1,$1+$1-$3-$3,$2+$2,0,0,0*
 *  1,1,$3+$3,$1-$3,$2-$3*
 *  1,1,$3+$3,$3-$1,$2-$3*
 *  1,1,$3+$3,$1-$3,$3-$2*
 *  1,1,$3+$3,$3-$1,$3-$2*%
 *  Example of instanciation:
 *
 *  A more complicated sample of aperture macro definition:
 *  G04 Rectangular Thermal Macro, params: W/2, H/2, T/2 *
 *  %AMRECTHERM*
 *  $4=$3/2*
 *  21,1,$1-$3,$2-$3,0-$1/2-$4,0-$2/2-$4,0*
 *  21,1,$1-$3,$2-$3,0-$1/2-$4,$2/2+$4,0*
 *  21,1,$1-$3,$2-$3,$1/2+$4,0-$2/2-$4,0*
 *  21,1,$1-$3,$2-$3,$1/2+$4,$2/2+$4,0*%
 *  Example of instanciation:
 *  %ADD28RECTHERM,0.035591X0.041496X0.005000*%
 */

#include <vector>

#include "dcode.h"

/**
 * Class AM_PARAM
 * holds a parameter for an "aperture macro" as defined within
 * standard RS274X.  The \a value field can be a constant, i.e. "immediate"
 * parameter or it may not be used if this param is going to defer to the
 * referencing aperture macro.  In that case, the \a index field is an index
 * into the aperture macro's parameters.
 */
class AM_PARAM
{
public: AM_PARAM() :
        index( -1 ),
        value( 0.0 )
    {}

    double GetValue( const D_CODE* aDcode ) const;

    void SetValue( double aValue )
    {
        value = aValue;
        index = -1;
    }


    /**
     * Function IsImmediate
     * tests if this AM_PARAM holds an immediate parameter or is a pointer
     * into a parameter held by an owning D_CODE.
     */
    bool IsImmediate() const { return index == -1; }

    unsigned GetIndex() const
    {
        return (unsigned) index;
    }


    void SetIndex( int aIndex )
    {
        index = aIndex;
    }


    /**
     * Function ReadParam
     * Read one aperture macro parameter
     * a parameter can be:
     *      a number
     *      a reference to an aperture definition parameter value: $1 ot $3 ...
     * a parameter definition can be complex and have operators between numbers and/or other parameter
     * like $1+3 or $2x2..
     * Parameters are separated by a comma ( of finish by *)
     * @param aText = pointer to the parameter to read. Will be modified to point to the next field
     * @return true if a param is read, or false
     */
    bool ReadParam( char*& aText  );

private:
    int    index;       ///< if -1, then \a value field is an immediate value,
                        //   else this is an index into parent's
                        //   D_CODE.m_am_params.
    double value;       ///< if IsImmediate()==true then use the value, else
                        //   not used.
};

typedef std::vector<AM_PARAM> AM_PARAMS;

#endif  // _AM_PARAM_H_
