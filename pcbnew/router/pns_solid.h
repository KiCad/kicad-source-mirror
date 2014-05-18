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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    PNS_SOLID() : PNS_ITEM( SOLID ), m_shape( NULL )
    {
        m_movable = false;
    }

    ~PNS_SOLID()
    {
        delete m_shape;
    }

    PNS_SOLID( const PNS_SOLID& aSolid ) :
        PNS_ITEM ( aSolid )
    {
        m_shape = aSolid.m_shape->Clone();
        m_pos = aSolid.m_pos;
    }
    
    PNS_ITEM* Clone() const;

    const SHAPE* Shape() const { return m_shape; }

    const SHAPE_LINE_CHAIN Hull( int aClearance = 0, int aWalkaroundThickness = 0 ) const;

    void SetShape( SHAPE* shape )
    {
        if( m_shape )
            delete m_shape;

        m_shape = shape;
    }

    const VECTOR2I& Pos() const
    {
        return m_pos;
    }

    void SetPos( const VECTOR2I& aCenter )
    {
        m_pos = aCenter;
    }

    virtual VECTOR2I Anchor( int aN ) const
    {
        return m_pos;
    }

    virtual int AnchorCount() const 
    {
        return 1;
    }

private:
    VECTOR2I    m_pos;
    SHAPE*      m_shape;
};

#endif
