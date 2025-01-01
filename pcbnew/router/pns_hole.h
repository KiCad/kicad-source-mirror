/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PNS_HOLE_H
#define __PNS_HOLE_H

#include "pns_item.h"

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

namespace PNS
{

class HOLE : public ITEM
{
public:
    HOLE( SHAPE* aShape ) :
            ITEM( ITEM::HOLE_T ),
            m_holeShape( aShape ),
            m_parentPadVia( nullptr )
    {
    }

    HOLE( const ITEM& aOther ) :
            ITEM( aOther ),
            m_parentPadVia( nullptr )
    {
    }

    virtual ~HOLE();

    /**
     * Return a deep copy of the item.
     */
    virtual HOLE* Clone() const override;

    virtual NET_HANDLE Net() const override
    {
        if( m_parentPadVia )
            return m_parentPadVia->Net();

        return m_net;
    }

    virtual const SHAPE_LINE_CHAIN Hull( int aClearance = 0, int aWalkaroundThickness = 0,
                                         int aLayer = -1 ) const override;

    int Radius() const;

    const SHAPE* Shape( int aLayer ) const override { return m_holeShape; }

    void SetParentPadVia( ITEM* aParent ) { m_parentPadVia = aParent; }
    ITEM* ParentPadVia() const override { return m_parentPadVia; }

    BOARD_ITEM* BoardItem() const override
    {
        if( m_parent )
            return m_parent;

        if( m_parentPadVia )
            return m_parentPadVia->Parent();

        return nullptr;
    }

    void SetCenter( const VECTOR2I& aCenter );
    void SetRadius( int aRadius );

    void Move( const VECTOR2I& delta );

    static HOLE* MakeCircularHole( const VECTOR2I& pos, int radius, PNS_LAYER_RANGE aLayers );


private:
    SHAPE* m_holeShape;
    ITEM*  m_parentPadVia;
};

}; // namespace PNS

#endif
