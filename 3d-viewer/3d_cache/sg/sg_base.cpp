/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <iostream>
#include <cmath>
#include "plugins/3dapi/sg_base.h"


SGCOLOR::SGCOLOR()
{
    red = 0.0;
    green = 0.0;
    blue = 0.0;

    return;
}

SGCOLOR::SGCOLOR( float aRVal, float aGVal, float aBVal )
{
    if( !checkRange( aRVal, aGVal, aBVal ) )
    {
#ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid value passed to constructor\n";
#endif
        red = 0.0;
        green = 0.0;
        blue = 0.0;
        return;
    }

    red = aRVal;
    green = aGVal;
    blue = aBVal;
    return;
}


void SGCOLOR::GetColor( float& aRedVal, float& aGreenVal, float& aBlueVal ) const
{
    aRedVal = red;
    aGreenVal = green;
    aBlueVal = blue;
    return;
}


void SGCOLOR::GetColor( SGCOLOR& aColor ) const
{
    aColor.red = red;
    aColor.green = green;
    aColor.blue = blue;
    return;
}


void SGCOLOR::GetColor( SGCOLOR* aColor ) const
{
    if( NULL == aColor )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aColor\n";
        return;
    }

    aColor->red = red;
    aColor->green = green;
    aColor->blue = blue;
    return;
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


bool SGCOLOR::SetColor( const SGCOLOR& aColor )
{
    red = aColor.red;
    green = aColor.green;
    blue = aColor.blue;
    return true;
}


bool SGCOLOR::SetColor( const SGCOLOR* aColor )
{
    if( NULL == aColor )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aColor\n";
        return false;
    }

    red = aColor->red;
    green = aColor->green;
    blue = aColor->blue;
    return true;
}


bool SGCOLOR::checkRange( float aRedVal, float aGreenVal, float aBlueVal ) const
{
    bool ok = true;

    if( aRedVal < 0.0 || aRedVal > 1.0 )
    {
#ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid RED value: " << aRedVal << "\n";
#endif
        ok = false;
    }

    if( aGreenVal < 0.0 || aGreenVal > 1.0 )
    {
#ifdef DEBUG
        if( ok )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        }
        std::cerr << " * [BUG] invalid GREEN value: " << aGreenVal << "\n";
#endif
        ok = false;
    }

    if( aBlueVal < 0.0 || aBlueVal > 1.0 )
    {
#ifdef DEBUG
        if( ok )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        }
        std::cerr << " * [BUG] invalid BLUE value: " << aBlueVal << "\n";
#endif
        ok = false;
    }

    return ok;
}


SGPOINT::SGPOINT()
{
    x = 0.0;
    y = 0.0;
    z = 0.0;
    return;
}


SGPOINT::SGPOINT( double aXVal, double aYVal, double aZVal )
{
    x = aXVal;
    y = aYVal;
    z = aZVal;
}


void SGPOINT::GetPoint( double& aXVal, double& aYVal, double& aZVal )
{
    x = aXVal;
    y = aYVal;
    z = aZVal;
    return;
}


void SGPOINT::GetPoint( SGPOINT& aPoint )
{
    x = aPoint.x;
    y = aPoint.y;
    z = aPoint.z;
    return;
}


void SGPOINT::GetPoint( SGPOINT* aPoint )
{
    if( NULL == aPoint )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aPoint\n";
        return;
    }

    x = aPoint->x;
    y = aPoint->y;
    z = aPoint->z;
    return;
}


void SGPOINT::SetPoint( double aXVal, double aYVal, double aZVal )
{
    x = aXVal;
    y = aYVal;
    z = aZVal;
    return;
}


void SGPOINT::SetPoint( const SGPOINT& aPoint )
{
    x = aPoint.x;
    y = aPoint.y;
    z = aPoint.z;
    return;
}


SGVECTOR::SGVECTOR()
{
    vx = 0.0;
    vy = 0.0;
    vz = 1.0;
    return;
}


SGVECTOR::SGVECTOR( double aXVal, double aYVal, double aZVal )
{
    vx = aXVal;
    vy = aYVal;
    vz = aZVal;
    normalize();
    return;
}


void SGVECTOR::GetVector( double& aXVal, double& aYVal, double& aZVal ) const
{
    aXVal = vx;
    aYVal = vy;
    aZVal = vz;
    return;
}


void SGVECTOR::SetVector( double aXVal, double aYVal, double aZVal )
{
    vx = aXVal;
    vy = aYVal;
    vz = aZVal;
    normalize();
    return;
}


void SGVECTOR::SetVector( const SGVECTOR& aVector )
{
    aVector.GetVector( vx, vy, vz );
    return;
}


void SGVECTOR::normalize( void )
{
    double dx = vx * vx;
    double dy = vy * vy;
    double dz = vz * vz;
    double dv2 = dx + dy + dz;

    // note: placing the limit at 1e-14 ensures that
    // models representing mm to 1e-4 precision can
    // be accepted before the calculations blow up
    if( (dx + dy + dz) < 1e-14 )
    {
        // use the default; the numbers are too small
        // to be believable
        vx = 0.0;
        vy = 0.0;
        vz = 1.0;
        return;
    }

    dx /= dv2;
    dy /= dv2;
    dz /= dv2;

    vx = sqrt( dx );
    vy = sqrt( dy );
    vz = sqrt( dz );

    return;
}


SGVECTOR& SGVECTOR::operator=( const SGVECTOR& source )
{
    vx = source.vx;
    vy = source.vy;
    vz = source.vz;
    return *this;
}
