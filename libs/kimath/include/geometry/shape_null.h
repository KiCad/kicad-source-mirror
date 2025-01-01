/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __SHAPE_NULL_H
#define __SHAPE_NULL_H

#include <geometry/shape.h>
#include <math/box2.h>
#include <math/vector2d.h>

#include <algorithm>

class SHAPE_NULL : public SHAPE
{
public:
    SHAPE_NULL() :
        SHAPE( SH_NULL )
    {}

    SHAPE_NULL( const SHAPE_NULL& aOther ) :
        SHAPE( SH_NULL )
    {};

    ~SHAPE_NULL()
    {}

    SHAPE* Clone() const override
    {
        return new SHAPE_NULL( *this );
    }

    SHAPE_NULL& operator=( const SHAPE_NULL& ) = default;

    const BOX2I BBox( int aClearance = 0 ) const override
    {
        return BOX2I();
    }

    bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override
    {
        return false;
    }

    void Move( const VECTOR2I& aVector ) override
    {
    }

    void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override
    {
    }

    bool IsSolid() const override
    {
        return false;
    }

    void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                             ERROR_LOC aErrorLoc ) const override {}
};

#endif
