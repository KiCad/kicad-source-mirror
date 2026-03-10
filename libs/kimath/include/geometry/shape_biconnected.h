/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
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

#pragma once

#include <core/mirror.h>     // for FLIP_DIRECTION
#include <geometry/shape.h>
#include <base_units.h>
#include <math/vector2d.h>   // for VECTOR2I

class SHAPE_BICONNECTED : public SHAPE
{
    public:
        SHAPE_BICONNECTED( SHAPE_TYPE aType ) :
            SHAPE( aType )
    {

    }

    virtual const VECTOR2I &GetStart() const = 0;
    virtual const VECTOR2I &GetEnd() const = 0;

    std::unique_ptr<SHAPE_BICONNECTED> Reversed() const
    {
        return std::unique_ptr<SHAPE_BICONNECTED>( this->reversedImpl() );
    }

    bool Contains( const VECTOR2I& aP ) const { return false; }

    static int Intersect( const SHAPE_BICONNECTED& aA, const SHAPE_BICONNECTED& aB, std::vector<VECTOR2I>& aIntersections ) { return 0; }
    
    virtual SEG::ecoord SquaredShapeDistance( const SHAPE_BICONNECTED& aOtherShape ) const { return 0; }
    virtual int Length() const { return 0; }
    virtual VECTOR2I NearestPoint( const VECTOR2I& aP ) const { return VECTOR2I(); }

    
    virtual int IntersectLine( const SEG& aSeg, std::vector<VECTOR2I>* aIpsBuffer ) const { return 0; };

    virtual VECTOR2I TangentVector( bool aTakeStartPoint ) const { return VECTOR2I(); }

protected:
    virtual SHAPE_BICONNECTED* reversedImpl() const = 0;
};

