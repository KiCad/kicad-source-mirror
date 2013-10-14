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

#ifndef __PNS_SEGMENT_H
#define __PNS_SEGMENT_H

#include <math/vector2d.h>

#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_item.h"
#include "pns_line.h"

class PNS_NODE;

class PNS_SEGMENT : public PNS_ITEM
{
public:
    PNS_SEGMENT() :
        PNS_ITEM( SEGMENT )
    {};

    PNS_SEGMENT( const SEG& aSeg, int aNet ) :
        PNS_ITEM( SEGMENT )
    {
        m_net = aNet;
        m_shape.Clear();
        m_shape.Append( aSeg.A );
        m_shape.Append( aSeg.B );
    };

    PNS_SEGMENT( const PNS_LINE& aParentLine, const SEG& aSeg ) :
        PNS_ITEM( SEGMENT )
    {
        m_net = aParentLine.GetNet();
        m_layers = aParentLine.GetLayers();
        m_width = aParentLine.GetWidth();
        m_shape.Clear();
        m_shape.Append( aSeg.A );
        m_shape.Append( aSeg.B );
    };


    PNS_SEGMENT* Clone() const;

    const SHAPE* GetShape() const
    {
        return static_cast<const SHAPE*>( &m_shape );
    }

    void SetLayer( int aLayer )
    {
        SetLayers( PNS_LAYERSET( aLayer ) );
    }

    int GetLayer() const
    {
        return GetLayers().Start();
    }

    const SHAPE_LINE_CHAIN& GetCLine() const
    {
        return m_shape;
    }

    void SetWidth( int aWidth )
    {
        m_width = aWidth;
    }

    int GetWidth() const
    {
        return m_width;
    }

    const SEG GetSeg() const
    {
        assert( m_shape.PointCount() >= 1 );

        if( m_shape.PointCount() == 1 )
            return SEG( m_shape.CPoint( 0 ), m_shape.CPoint( 0 ) );

        return SEG( m_shape.CPoint( 0 ), m_shape.CPoint( 1 ) );
    }

    void SetEnds( const VECTOR2I& a, const VECTOR2I& b )
    {
        m_shape.Clear();
        m_shape.Append( a );
        m_shape.Append( b );
    }

    void SwapEnds()
    {
        m_shape = m_shape.Reverse();
    }

    const SHAPE_LINE_CHAIN Hull( int aClearance, int aWalkaroundThickness ) const;

private:
    SHAPE_LINE_CHAIN m_shape;
    int m_width;
};

#endif
