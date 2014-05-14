/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#ifndef __PNS_SEGMENT_H
#define __PNS_SEGMENT_H

#include <math/vector2d.h>

#include <geometry/seg.h>
#include <geometry/shape_segment.h>
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
        PNS_ITEM( SEGMENT ), m_seg(aSeg, 0)
    {
        m_net = aNet;
    };

    PNS_SEGMENT( const PNS_LINE& aParentLine, const SEG& aSeg ) :
        PNS_ITEM( SEGMENT ), 
        m_seg(aSeg, aParentLine.Width())
    {
        m_net = aParentLine.Net();
        m_layers = aParentLine.Layers();
        m_marker = aParentLine.Marker();
        m_rank = aParentLine.Rank();
    };


    PNS_SEGMENT* Clone( ) const;

    const SHAPE* Shape() const
    {
        return static_cast<const SHAPE*>( &m_seg );
    }

    void SetLayer( int aLayer )
    {
        SetLayers( PNS_LAYERSET( aLayer ) );
    }

    int Layer() const
    {
        return Layers().Start();
    }

    void SetWidth( int aWidth )
    {
        m_seg.SetWidth(aWidth);
    }

    int Width() const
    {
        return m_seg.GetWidth();
    }

    const SEG& Seg() const
    {
        return m_seg.GetSeg();
    }

    const SHAPE_LINE_CHAIN CLine() const
    {
        return SHAPE_LINE_CHAIN( m_seg.GetSeg().A, m_seg.GetSeg().B );
    }

    void SetEnds( const VECTOR2I& a, const VECTOR2I& b )
    {
        m_seg.SetSeg( SEG ( a, b ) );
	}

    void SwapEnds()
    {
        SEG tmp = m_seg.GetSeg();
        m_seg.SetSeg( SEG (tmp.B , tmp.A ));
    }

    const SHAPE_LINE_CHAIN Hull( int aClearance, int aWalkaroundThickness ) const;

    virtual VECTOR2I Anchor(int n) const 
    {
        if(n == 0)
            return m_seg.GetSeg().A;
        else
            return m_seg.GetSeg().B;
    }

    virtual int AnchorCount() const 
    {
        return 2; 
    }

private:
    SHAPE_SEGMENT m_seg;
};

#endif
