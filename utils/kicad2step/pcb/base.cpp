/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include <wx/log.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include "sexpr/sexpr.h"
#include "base.h"

static const char bad_position[] = "* corrupt module in PCB file; invalid position";


bool Get2DPositionAndRotation( SEXPR::SEXPR* data, DOUBLET& aPosition, double& aRotation )
{
    // form: (at X Y {rot})
    int nchild = data->GetNumberOfChildren();

    if( nchild < 3 )
    {
        std::ostringstream ostr;
        ostr << bad_position;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    if( data->GetChild( 0 )->GetSymbol() != "at" )
    {
        std::ostringstream ostr;
        ostr << "* SEXPR item is not a position string";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    SEXPR::SEXPR* child = data->GetChild( 1 );
    double x;

    if( child->IsDouble() )
        x = child->GetDouble();
    else if( child->IsInteger() )
        x = (double) child->GetInteger();
    else
    {
        std::ostringstream ostr;
        ostr << bad_position;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    child = data->GetChild( 2 );
    double y;

    if( child->IsDouble() )
        y = child->GetDouble();
    else if( child->IsInteger() )
        y = (double) child->GetInteger();
    else
    {
        std::ostringstream ostr;
        ostr << bad_position;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    aPosition.x = x;
    aPosition.y = y;

    if( nchild == 3 )
        return true;

    child = data->GetChild( 3 );
    double angle = 0.0;

    if( child->IsDouble() )
        angle = child->GetDouble();
    else if( child->IsInteger() )
        angle = (double) child->GetInteger();
    else
    {
        std::ostringstream ostr;
        ostr << bad_position;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    while( angle >= 360.0 )
        angle -= 360.0;

    while( angle <= -360.0 )
        angle += 360.0;

    aRotation = (angle / 180.0) * M_PI;

    return true;
}


bool Get2DCoordinate( SEXPR::SEXPR* data, DOUBLET& aCoordinate )
{
    // form: (at X Y {rot})
    int nchild = data->GetNumberOfChildren();

    if( nchild < 3 )
    {
        std::ostringstream ostr;
        ostr << bad_position;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    SEXPR::SEXPR* child = data->GetChild( 1 );
    double x;

    if( child->IsDouble() )
        x = child->GetDouble();
    else if( child->IsInteger() )
        x = (double) child->GetInteger();
    else
    {
        std::ostringstream ostr;
        ostr << bad_position;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    child = data->GetChild( 2 );
    double y;

    if( child->IsDouble() )
        y = child->GetDouble();
    else if( child->IsInteger() )
        y = (double) child->GetInteger();
    else
    {
        std::ostringstream ostr;
        ostr << bad_position;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    aCoordinate.x = x;
    aCoordinate.y = y;

    return true;
}


bool Get3DCoordinate( SEXPR::SEXPR* data, TRIPLET& aCoordinate )
{
    // form: (at X Y Z)
    int nchild = data->GetNumberOfChildren();

    if( nchild < 4 )
    {
        std::ostringstream ostr;
        ostr << bad_position;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    SEXPR::SEXPR* child;
    double val[3];

    for( int i = 1; i < 4; ++i )
    {
        child = data->GetChild( i );

        if( child->IsDouble() )
            val[i -1] = child->GetDouble();
        else if( child->IsInteger() )
            val[i -1] = (double) child->GetInteger();
        else
        {
            std::ostringstream ostr;
            ostr << bad_position;
            wxLogMessage( "%s\n", ostr.str().c_str() );
            return false;
        }
    }

    aCoordinate.x = val[0];
    aCoordinate.y = val[1];
    aCoordinate.z = val[2];

    return true;
}


bool GetXYZRotation( SEXPR::SEXPR* data, TRIPLET& aRotation )
{
    const char bad_rotation[] = "* invalid 3D rotation";

    if( !Get3DCoordinate( data, aRotation ) )
    {
        std::ostringstream ostr;
        ostr << bad_rotation;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    if( aRotation.x > 360.0 || aRotation.x < -360.0 )
    {
        int nr = (int) aRotation.x / 360;
        aRotation.x -= ( 360.0 * nr );
    }

    if( aRotation.y > 360.0 || aRotation.y < -360.0 )
    {
        int nr = (int) aRotation.y / 360;
        aRotation.y -= ( 360.0 * nr );
    }

    if( aRotation.z > 360.0 || aRotation.z < -360.0 )
    {
        int nr = (int) aRotation.z / 360;
        aRotation.z -= ( 360.0 * nr );
    }

    aRotation.x *= M_PI / 180.0;
    aRotation.y *= M_PI / 180.0;
    aRotation.z *= M_PI / 180.0;

    return true;
}
