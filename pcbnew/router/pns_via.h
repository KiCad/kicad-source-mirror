/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __PNS_VIA_H
#define __PNS_VIA_H

#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>
#include <math/box2.h>

#include "pcb_track.h"

#include "pns_item.h"
#include "pns_linked_item.h"
#include "pns_hole.h"

namespace PNS {

class NODE;

// uniquely identifies a VIA within a NODE without using pointers. Used to
// simplify (or complexifiy, depending on the point of view) the pointer management
// in PNS::NODE. Sooner or later I'll have to fix it for good using smart pointers - twl
struct VIA_HANDLE
{
    bool        valid = false;
    VECTOR2I    pos;
    LAYER_RANGE layers;
    int         net = -1;
};

class VIA : public LINKED_ITEM
{
public:
    VIA() :
        LINKED_ITEM( VIA_T ),
        m_hole( nullptr )
    {
        m_diameter = 2;     // Dummy value
        m_drill = 1;        // Dummy value
        m_viaType = VIATYPE::THROUGH;
        m_isFree = false;
        m_isVirtual = false;
        SetHole( HOLE::MakeCircularHole( m_pos, m_drill / 2 ) );
    }

    VIA( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aDiameter, int aDrill,
         int aNet = -1, VIATYPE aViaType = VIATYPE::THROUGH ) :
        LINKED_ITEM( VIA_T ),
        m_hole( nullptr )
    {
        SetNet( aNet );
        SetLayers( aLayers );
        m_pos = aPos;
        m_diameter = aDiameter;
        m_drill = aDrill;
        m_shape = SHAPE_CIRCLE( aPos, aDiameter / 2 );
        SetHole( HOLE::MakeCircularHole( m_pos, aDrill / 2 ) );
        m_viaType = aViaType;
        m_isFree = false;
        m_isVirtual = false;
    }

    VIA( const VIA& aB ) :
        LINKED_ITEM( aB ),
        m_hole( nullptr )
    {
        SetNet( aB.Net() );
        SetLayers( aB.Layers() );
        m_pos = aB.m_pos;
        m_diameter = aB.m_diameter;
        m_shape = SHAPE_CIRCLE( m_pos, m_diameter / 2 );
        m_drill = aB.m_drill;
        SetHole( HOLE::MakeCircularHole( m_pos, m_drill / 2 ) );
        m_marker = aB.m_marker;
        m_rank = aB.m_rank;
        m_viaType = aB.m_viaType;
        m_isFree = aB.m_isFree;
        m_isVirtual = aB.m_isVirtual;
    }

    virtual ~VIA()
    {
        if ( m_hole && m_hole->BelongsTo( this ) )
            delete m_hole;
    }

    static inline bool ClassOf( const ITEM* aItem )
    {
        return aItem && VIA_T == aItem->Kind();
    }

    const VECTOR2I& Pos() const { return m_pos; }

    void SetPos( const VECTOR2I& aPos )
    {
        m_pos = aPos;
        m_shape.SetCenter( aPos );

        if( m_hole )
            m_hole->SetCenter( aPos );
    }

    VIATYPE ViaType() const { return m_viaType; }
    void SetViaType( VIATYPE aViaType ) { m_viaType = aViaType; }

    int Diameter() const { return m_diameter; }

    void SetDiameter( int aDiameter )
    {
        m_diameter = aDiameter;
        m_shape.SetRadius( m_diameter / 2 );
    }

    int Drill() const { return m_drill; }

    void SetDrill( int aDrill )
    {
        m_drill = aDrill;

        if( m_hole )
            m_hole->SetRadius( m_drill / 2 );
    }

    bool IsFree() const { return m_isFree; }
    void SetIsFree( bool aIsFree ) { m_isFree = aIsFree; }

    bool PushoutForce( NODE* aNode, const VECTOR2I& aDirection, VECTOR2I& aForce,
                       int aCollisionMask = ITEM::ANY_T, int aMaxIterations = 10 );

    bool PushoutForce( NODE* aNode, const ITEM* aOther, VECTOR2I& aForce );

    const SHAPE* Shape() const override { return &m_shape; }

    VIA* Clone() const override;

    const SHAPE_LINE_CHAIN Hull( int aClearance = 0, int aWalkaroundThickness = 0,
                                 int aLayer = -1 ) const override;

    virtual VECTOR2I Anchor( int n ) const override
    {
        return m_pos;
    }

    virtual int AnchorCount() const override
    {
        return 1;
    }

    OPT_BOX2I ChangedArea( const VIA* aOther ) const;

    const VIA_HANDLE MakeHandle() const;

    virtual void SetHole( HOLE* aHole ) override
    {
        if( m_hole && m_hole->BelongsTo( this ) )
            delete m_hole;

        m_hole = aHole;
        m_hole->SetParentPadVia( this );
        m_hole->SetOwner( this );
        m_hole->SetLayers( m_layers ); // fixme: backdrill vias can have hole layer set different
                                       // than copper layer set
    }

    virtual bool HasHole() const override { return true; }
    virtual HOLE *Hole() const override { return m_hole; }

    virtual const std::string Format() const override;

private:
    int          m_diameter;
    int          m_drill;
    VECTOR2I     m_pos;
    SHAPE_CIRCLE m_shape;
    VIATYPE      m_viaType;
    bool         m_isFree;
    HOLE*        m_hole;
};


class VVIA : public VIA
{
public:
    VVIA( const VECTOR2I& aPos, int aLayer, int aDiameter, int aNet ) :
        VIA( aPos, LAYER_RANGE( aLayer, aLayer ), aDiameter, aDiameter / 2, aNet )
    {
        m_isVirtual = true;
        //SetHole( SHAPE_CIRCLE( Pos(), 1 ) );
    }
};

}

#endif
