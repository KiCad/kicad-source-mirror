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

/**
 * @file sg_base.h
 * defines the low level classes common to scene graph nodes
 */


#ifndef SG_BASE_H
#define SG_BASE_H

#include "plugins/3dapi/ifsg_defs.h"

#ifndef SGLIB_API
    #if defined (COMPILE_SGLIB)
        #define SGLIB_API APIEXPORT
    #else
        #define SGLIB_API APIIMPORT
    #endif
#endif

class SGLIB_API SGCOLOR
{
protected:
    float red;
    float green;
    float blue;

public:
    SGCOLOR();
    SGCOLOR( float aRVal, float aGVal, float aBVal );

    void GetColor( float& aRedVal, float& aGreenVal, float& aBlueVal ) const noexcept;
    void GetColor( SGCOLOR& aColor ) const noexcept;
    void GetColor( SGCOLOR* aColor ) const noexcept;

    bool SetColor( float aRedVal, float aGreenVal, float aBlueVal );
    bool SetColor( const SGCOLOR& aColor ) noexcept;
    bool SetColor( const SGCOLOR* aColor ) noexcept;

private:
    bool checkRange( float aRedVal, float aGreenVal, float aBlueVal ) const noexcept;
};


class SGLIB_API SGPOINT
{
public:
    double x;
    double y;
    double z;

public:
    SGPOINT();
    SGPOINT( double aXVal, double aYVal, double aZVal ) noexcept;

    void GetPoint( const double& aXVal, const double& aYVal, const double& aZVal ) noexcept;
    void GetPoint( const SGPOINT& aPoint ) noexcept;
    void GetPoint( const SGPOINT* aPoint ) noexcept;

    void SetPoint( double aXVal, double aYVal, double aZVal ) noexcept;
    void SetPoint( const SGPOINT& aPoint ) noexcept;
};


class SGLIB_API SGVECTOR
{
private:
    void normalize( void ) noexcept;

    double vx;
    double vy;
    double vz;

public:
    SGVECTOR();
    SGVECTOR( double aXVal, double aYVal, double aZVal );

    void GetVector( double& aXVal, double& aYVal, double& aZVal ) const noexcept;

    void SetVector( double aXVal, double aYVal, double aZVal );
    void SetVector( const SGVECTOR& aVector );

    SGVECTOR& operator=( const SGVECTOR& source ) noexcept;
};


#endif  // SG_BASE_H
