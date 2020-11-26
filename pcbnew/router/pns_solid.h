/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

namespace PNS {

class SOLID : public ITEM
{
public:
    SOLID() : ITEM( SOLID_T ), m_shape( NULL ), m_alternateShape( NULL )
    {
        m_movable = false;
        m_padToDie = 0;
    }

    ~SOLID()
    {
        delete m_shape;
        delete m_alternateShape;
    }

    SOLID( const SOLID& aSolid ) :
        ITEM( aSolid )
    {
        if( aSolid.m_shape )
            m_shape = aSolid.m_shape->Clone();
        else
            m_shape = nullptr;

        if( aSolid.m_alternateShape )
            m_alternateShape = aSolid.m_alternateShape->Clone();
        else
            m_alternateShape = nullptr;

        m_pos = aSolid.m_pos;
        m_padToDie = aSolid.m_padToDie;
    }

    static inline bool ClassOf( const ITEM* aItem )
    {
        return aItem && SOLID_T == aItem->Kind();
    }

    ITEM* Clone() const override;

    const SHAPE* Shape() const override { return m_shape; }

    const SHAPE* AlternateShape() const override
    {
        return m_alternateShape;
    }

    const SHAPE_LINE_CHAIN Hull( int aClearance = 0, int aWalkaroundThickness = 0,
                                 int aLayer = -1 ) const override;

    void SetShape( SHAPE* shape )
    {
        delete m_shape;
        m_shape = shape;
    }

    void SetAlternateShape( SHAPE* shape )
    {
        delete m_alternateShape;
        m_alternateShape = shape;
    }

    const VECTOR2I& Pos() const
    {
        return m_pos;
    }

    void SetPos( const VECTOR2I& aCenter );

    int GetPadToDie() const
    {
        return m_padToDie;
    }

    void SetPadToDie( int aLen )
    {
        m_padToDie = aLen;
    }

    virtual VECTOR2I Anchor( int aN ) const override
    {
        return m_pos;
    }

    virtual int AnchorCount() const override
    {
        return 1;
    }

    VECTOR2I Offset() const
    {
        return m_offset;
    }

    void SetOffset( const VECTOR2I& aOffset )
    {
        m_offset = aOffset;
    }

private:
    VECTOR2I    m_pos;
    SHAPE*      m_shape;
    SHAPE*      m_alternateShape;
    VECTOR2I    m_offset;
    int         m_padToDie;
};

}

#endif
