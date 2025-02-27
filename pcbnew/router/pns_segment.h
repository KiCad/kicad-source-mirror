/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PNS_SEGMENT_H
#define PNS_SEGMENT_H

#include <math/vector2d.h>

#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_line_chain.h>

#include "pns_line.h"
#include "pns_linked_item.h"

namespace PNS {

class NODE;

class SEGMENT : public LINKED_ITEM
{
public:
    SEGMENT() :
        LINKED_ITEM( SEGMENT_T )
    {}

    SEGMENT( const SEG& aSeg, NET_HANDLE aNet ) :
        LINKED_ITEM( SEGMENT_T ),
        m_seg( aSeg, 0 )
    {
        m_net = aNet;
    }

    SEGMENT( const LINE& aParentLine, const SEG& aSeg ) :
        LINKED_ITEM( SEGMENT_T ),
        m_seg( aSeg, aParentLine.Width() )
    {
        m_parent = nullptr;
        m_sourceItem = aParentLine.GetSourceItem();

        m_net = aParentLine.Net();
        m_layers = aParentLine.Layers();
        m_marker = aParentLine.Marker();
        m_rank = aParentLine.Rank();
    }

    explicit SEGMENT( const LINKED_ITEM& aParent ) :
        LINKED_ITEM( aParent )
    {
        assert( aParent.Kind() == SEGMENT_T );
    }

    static inline bool ClassOf( const ITEM* aItem )
    {
        return aItem && SEGMENT_T == aItem->Kind();
    }

    SEGMENT* Clone() const override;

    const SHAPE* Shape( int aLayer ) const override
    {
        return static_cast<const SHAPE*>( &m_seg );
    }

    void SetWidth( int aWidth ) override
    {
        m_seg.SetWidth(aWidth);
    }

    int Width() const override
    {
        return m_seg.GetWidth();
    }

    const SEG& Seg() const
    {
        return m_seg.GetSeg();
    }

    const SHAPE_LINE_CHAIN CLine() const
    {
        return SHAPE_LINE_CHAIN( { m_seg.GetSeg().A, m_seg.GetSeg().B } );
    }

    void SetEnds( const VECTOR2I& a, const VECTOR2I& b )
    {
        m_seg.SetSeg( SEG ( a, b ) );
    }

    void SwapEnds()
    {
        SEG tmp = m_seg.GetSeg();
        m_seg.SetSeg( SEG (tmp.B , tmp.A ) );
    }

    const SHAPE_LINE_CHAIN Hull( int aClearance, int aWalkaroundThickness,
                                 int aLayer = -1 ) const override;

    virtual VECTOR2I Anchor( int n ) const override
    {
        if( n == 0 )
            return m_seg.GetSeg().A;
        else
            return m_seg.GetSeg().B;
    }

    virtual int AnchorCount() const override
    {
        return 2;
    }

    virtual const std::string Format() const override;

    void SetShape( const SHAPE_SEGMENT& aShape )
    {
        m_seg = aShape;
    }

private:
    SHAPE_SEGMENT m_seg;
};

}

#endif
