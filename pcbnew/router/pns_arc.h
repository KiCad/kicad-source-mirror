/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Seth Hillbrand <hillbrand@ucdavis.edu>
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

#ifndef __PNS_ARC_H
#define __PNS_ARC_H

#include <math/vector2d.h>

#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>

#include "pns_line.h"
#include "pns_linked_item.h"

namespace PNS {

class NODE;

class ARC : public LINKED_ITEM
{
public:
    ARC() :
        LINKED_ITEM( ARC_T )
    {}

    ARC( const SHAPE_ARC& aArc, NET_HANDLE aNet ) :
        LINKED_ITEM( ARC_T ),
        m_arc( aArc )
    {
        m_net = aNet;
    }

    ARC( const ARC& aParentArc, const SHAPE_ARC& aArc ) :
        LINKED_ITEM( ARC_T ),
        m_arc( aArc )
    {
        m_net = aParentArc.Net();
        m_layers = aParentArc.Layers();
        m_marker = aParentArc.Marker();
        m_rank = aParentArc.Rank();
    }

    ARC( const LINE& aParentLine, const SHAPE_ARC& aArc ) :
        LINKED_ITEM( ARC_T ),
        m_arc( aArc.GetP0(), aArc.GetArcMid(), aArc.GetP1(), aParentLine.Width() )
    {
        m_net = aParentLine.Net();
        m_layers = aParentLine.Layers();
        m_marker = aParentLine.Marker();
        m_rank = aParentLine.Rank();
    }

    static inline bool ClassOf( const ITEM* aItem )
    {
        return aItem && ARC_T == aItem->Kind();
    }

    ARC* Clone() const override;

    const SHAPE* Shape( int aLayer ) const override
    {
        return static_cast<const SHAPE*>( &m_arc );
    }

    void SetWidth( int aWidth ) override
    {
        m_arc.SetWidth(aWidth);
    }

    int Width() const override
    {
        return m_arc.GetWidth();
    }

    const SHAPE_LINE_CHAIN CLine() const
    {
        return SHAPE_LINE_CHAIN( m_arc );
    }

    const SHAPE_LINE_CHAIN Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const override;

    virtual VECTOR2I Anchor( int n ) const override
    {
        if( n == 0 )
            return m_arc.GetP0();
        else
            return m_arc.GetP1();
    }

    virtual int AnchorCount() const override
    {
        return 2;
    }

    OPT_BOX2I ChangedArea( const ARC* aOther ) const;

    SHAPE_ARC& Arc() { return m_arc; }
    const SHAPE_ARC& CArc() const { return m_arc; }

private:
    SHAPE_ARC m_arc;
};

}

#endif
