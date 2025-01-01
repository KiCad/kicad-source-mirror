/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <cmath>
#include <iostream>
#include <sstream>
#include <wx/log.h>

#include "plugins/3dapi/sg_base.h"


SGCOLOR::SGCOLOR()
{
    red = 0.0;
    green = 0.0;
    blue = 0.0;
}

SGCOLOR::SGCOLOR( float aRVal, float aGVal, float aBVal )
{
    if( !checkRange( aRVal, aGVal, aBVal ) )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] invalid value passed to constructor" ),
                    __FILE__, __FUNCTION__, __LINE__ );
        red = 0.0;
        green = 0.0;
        blue = 0.0;
        return;
    }

    red = aRVal;
    green = aGVal;
    blue = aBVal;
}


void SGCOLOR::GetColor( float& aRedVal, float& aGreenVal, float& aBlueVal ) const noexcept
{
    aRedVal = red;
    aGreenVal = green;
    aBlueVal = blue;
}


void SGCOLOR::GetColor( SGCOLOR& aColor ) const noexcept
{
    aColor.red = red;
    aColor.green = green;
    aColor.blue = blue;
}


void SGCOLOR::GetColor( SGCOLOR* aColor ) const noexcept
{
    wxCHECK_MSG( aColor, /* void */, wxT( "NULL pointer passed for aRGBColor" ) );

    aColor->red = red;
    aColor->green = green;
    aColor->blue = blue;
}


bool SGCOLOR::SetColor( float aRedVal, float aGreenVal, float aBlueVal )
{
    if( !checkRange( aRedVal, aGreenVal, aBlueVal ) )
        return false;

    red = aRedVal;
    green = aGreenVal;
    blue = aBlueVal;

    return true;
}


bool SGCOLOR::SetColor( const SGCOLOR& aColor ) noexcept
{
    red = aColor.red;
    green = aColor.green;
    blue = aColor.blue;
    return true;
}


bool SGCOLOR::SetColor( const SGCOLOR* aColor ) noexcept
{
    wxCHECK_MSG( aColor, false, wxT( "NULL pointer passed for aRGBColor" ) );

    red = aColor->red;
    green = aColor->green;
    blue = aColor->blue;
    return true;
}


bool SGCOLOR::checkRange( float aRedVal, float aGreenVal, float aBlueVal ) const noexcept
{
    bool ok = true;

    if( aRedVal < 0.0 || aRedVal > 1.0 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] invalid RED value: %g" ),
                    __FILE__, __FUNCTION__, __LINE__, aRedVal );

        ok = false;
    }

    if( aGreenVal < 0.0 || aGreenVal > 1.0 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] invalid GREEN value: %g" ),
                    __FILE__, __FUNCTION__, __LINE__, aGreenVal );

        ok = false;
    }

    if( aBlueVal < 0.0 || aBlueVal > 1.0 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] invalid BLUE value: %g" ),
                    __FILE__, __FUNCTION__, __LINE__, aBlueVal );

        ok = false;
    }

    return ok;
}


SGPOINT::SGPOINT()
{
    x = 0.0;
    y = 0.0;
    z = 0.0;
}


SGPOINT::SGPOINT( double aXVal, double aYVal, double aZVal ) noexcept
{
    x = aXVal;
    y = aYVal;
    z = aZVal;
}


void SGPOINT::GetPoint( const double& aXVal, const double& aYVal, const double& aZVal ) noexcept
{
    x = aXVal;
    y = aYVal;
    z = aZVal;
}


void SGPOINT::GetPoint( const SGPOINT& aPoint ) noexcept
{
    x = aPoint.x;
    y = aPoint.y;
    z = aPoint.z;
}


void SGPOINT::GetPoint( const SGPOINT* aPoint ) noexcept
{
    wxCHECK_MSG( aPoint, /* void */, wxT( "NULL pointer passed for aPoint" ) );

    x = aPoint->x;
    y = aPoint->y;
    z = aPoint->z;
}


void SGPOINT::SetPoint( double aXVal, double aYVal, double aZVal ) noexcept
{
    x = aXVal;
    y = aYVal;
    z = aZVal;
}


void SGPOINT::SetPoint( const SGPOINT& aPoint ) noexcept
{
    x = aPoint.x;
    y = aPoint.y;
    z = aPoint.z;
}


SGVECTOR::SGVECTOR()
{
    vx = 0.0;
    vy = 0.0;
    vz = 1.0;
}


SGVECTOR::SGVECTOR( double aXVal, double aYVal, double aZVal )
{
    vx = aXVal;
    vy = aYVal;
    vz = aZVal;
    normalize();
}


void SGVECTOR::GetVector( double& aXVal, double& aYVal, double& aZVal ) const noexcept
{
    aXVal = vx;
    aYVal = vy;
    aZVal = vz;
}


void SGVECTOR::SetVector( double aXVal, double aYVal, double aZVal )
{
    vx = aXVal;
    vy = aYVal;
    vz = aZVal;
    normalize();
}


void SGVECTOR::SetVector( const SGVECTOR& aVector )
{
    aVector.GetVector( vx, vy, vz );
}


void SGVECTOR::normalize( void ) noexcept
{
    double dx = vx * vx;
    double dy = vy * vy;
    double dz = vz * vz;
    double dv2 = sqrt( dx + dy + dz );

    if( ( dx + dy + dz ) < 1e-8 )
    {
        // use the default; the numbers are too small to be believable
        vx = 0.0;
        vy = 0.0;
        vz = 1.0;
        return;
    }

    vx /= dv2;
    vy /= dv2;
    vz /= dv2;
}


SGVECTOR& SGVECTOR::operator=( const SGVECTOR& source ) noexcept
{
    vx = source.vx;
    vy = source.vy;
    vz = source.vz;
    return *this;
}
