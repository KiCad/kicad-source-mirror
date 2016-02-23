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

    return;
}

SGCOLOR::SGCOLOR( float aRVal, float aGVal, float aBVal )
{
    if( !checkRange( aRVal, aGVal, aBVal ) )
    {
#ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] invalid value passed to constructor";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
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
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] NULL pointer passed for aColor";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

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
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] NULL pointer passed for aColor";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

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
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] invalid RED value: " << aRedVal;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
#endif
        ok = false;
    }

    if( aGreenVal < 0.0 || aGreenVal > 1.0 )
    {
#ifdef DEBUG
        if( ok )
        {
            wxLogTrace( MASK_3D_SG, "%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );
        }
        wxLogTrace( MASK_3D_SG, " * [BUG] invalid GREEN value: %f\n", aGreenVal );
#endif
        ok = false;
    }

    if( aBlueVal < 0.0 || aBlueVal > 1.0 )
    {
#ifdef DEBUG
        if( ok )
        {
            wxLogTrace( MASK_3D_SG, "%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );
        }
        wxLogTrace( MASK_3D_SG, " * [BUG] invalid BLUE value: %f\n", aBlueVal );
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
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] NULL pointer passed for aPoint";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

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
    double dv2 = sqrt( dx + dy + dz );

    if( (dx + dy + dz) < 1e-8 )
    {
        // use the default; the numbers are too small
        // to be believable
        vx = 0.0;
        vy = 0.0;
        vz = 1.0;
        return;
    }

    vx /= dv2;
    vy /= dv2;
    vz /= dv2;

    return;
}


SGVECTOR& SGVECTOR::operator=( const SGVECTOR& source )
{
    vx = source.vx;
    vy = source.vy;
    vz = source.vz;
    return *this;
}
