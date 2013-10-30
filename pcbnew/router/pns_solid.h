/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#ifndef __PNS_SOLID_H
#define __PNS_SOLID_H

#include <math/vector2d.h>

#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_item.h"

class PNS_SOLID : public PNS_ITEM
{
public:
    PNS_SOLID() : PNS_ITEM( SOLID )
    {
        m_movable = false;
        m_shape = NULL;
    }

    PNS_ITEM* Clone() const;

    const SHAPE* GetShape() const { return m_shape; }

    const SHAPE_LINE_CHAIN Hull( int aClearance = 0, int aWalkaroundThickness = 0 ) const;

    void SetShape( SHAPE* shape )
    {
        if( m_shape )
            delete m_shape;

        m_shape = shape;
    }

    const VECTOR2I& GetCenter() const
    {
        return m_center;
    }

    void SetCenter( const VECTOR2I& aCenter )
    {
        m_center = aCenter;
    }

private:
    VECTOR2I    m_center;
    SHAPE*      m_shape;
};

#endif
