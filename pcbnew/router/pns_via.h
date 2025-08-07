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

#ifndef __PNS_VIA_H
#define __PNS_VIA_H

#include <geometry/shape_index.h>
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
    VIA_HANDLE() : valid( false ) {};
    VIA_HANDLE( VECTOR2I aPos, PNS_LAYER_RANGE aLayers, NET_HANDLE aNet ) :
        valid( true ),
        pos( aPos ),
        layers (aLayers),
        net (aNet ) {};

    bool        valid = false;
    VECTOR2I    pos;
    PNS_LAYER_RANGE layers;
    NET_HANDLE  net = nullptr;
};

class VIA : public LINKED_ITEM
{
public:
    enum class STACK_MODE
    {
        // The via is the same size on every layer
        NORMAL,

        // The via can have three different sizes -- note that in this context, front means
        // m_layers.Start() and back means m_layers.End(), which does not align with KiCad in the
        // case of blind/buried vias.  Using this STACK_MODE only makes sense for vias that extend
        // through the whole PCB
        FRONT_INNER_BACK,

        // The via can have a different size on each layer
        CUSTOM
    };

    static constexpr int ALL_LAYERS = 0;
    static constexpr int INNER_LAYERS = 1;

    VIA() :
        LINKED_ITEM( VIA_T ),
        m_hole( nullptr )
    {
        m_stackMode = STACK_MODE::NORMAL;
        m_diameters[0] = 2; // Dummy value
        m_drill = 1;        // Dummy value
        m_viaType = VIATYPE::THROUGH;
        m_unconnectedLayerMode = PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL;
        m_isFree = false;
        m_isVirtual = false;
        SetHole( HOLE::MakeCircularHole( m_pos, m_drill / 2, PNS_LAYER_RANGE() ) );
    }

    VIA( const VECTOR2I& aPos, const PNS_LAYER_RANGE& aLayers, int aDiameter, int aDrill,
         NET_HANDLE aNet = nullptr, VIATYPE aViaType = VIATYPE::THROUGH ) :
        LINKED_ITEM( VIA_T ),
        m_hole( nullptr )
    {
        SetNet( aNet );
        SetLayers( aLayers );
        m_pos = aPos;
        m_stackMode = STACK_MODE::NORMAL;
        m_diameters[0] = aDiameter;
        m_drill = aDrill;
        m_shapes[0] = SHAPE_CIRCLE( aPos, aDiameter / 2 );
        SetHole( HOLE::MakeCircularHole( m_pos, aDrill / 2, PNS_LAYER_RANGE() ) );
        m_viaType = aViaType;
        m_unconnectedLayerMode = PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL;
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
        m_stackMode = aB.m_stackMode;
        m_diameters = aB.m_diameters;

        for( const auto& [layer, shape] : aB.m_shapes )
            m_shapes[layer] = SHAPE_CIRCLE( m_pos, shape.GetRadius() );

        m_drill = aB.m_drill;
        SetHole( HOLE::MakeCircularHole( m_pos, m_drill / 2, PNS_LAYER_RANGE() ) );
        m_marker = aB.m_marker;
        m_rank = aB.m_rank;
        m_viaType = aB.m_viaType;
        m_unconnectedLayerMode = aB.m_unconnectedLayerMode;
        m_isFree = aB.m_isFree;
        m_isVirtual = aB.m_isVirtual;
    }

    virtual ~VIA()
    {
        if ( m_hole && m_hole->BelongsTo( this ) )
            delete m_hole;
    }

    VIA& operator=( const VIA& aB )
    {
        SetParent( aB.Parent() );
        SetSourceItem( aB.GetSourceItem() );

        SetNet( aB.Net() );
        SetLayers( aB.Layers() );
        m_movable = aB.m_movable;
        m_pos = aB.m_pos;
        m_stackMode = aB.m_stackMode;
        m_diameters = aB.m_diameters;

        for( const auto& [layer, shape] : aB.m_shapes )
            m_shapes[layer] = SHAPE_CIRCLE( m_pos, shape.GetRadius() );

        m_drill = aB.m_drill;
        SetHole( HOLE::MakeCircularHole( m_pos, m_drill / 2, PNS_LAYER_RANGE() ) );
        m_marker = aB.m_marker;
        m_rank = aB.m_rank;
        m_routable = aB.m_routable;
        m_viaType = aB.m_viaType;
        m_unconnectedLayerMode = aB.m_unconnectedLayerMode;
        m_isFree = aB.m_isFree;
        m_isVirtual = aB.m_isVirtual;
        m_uid = aB.m_uid;
        return *this;
    }

    static inline bool ClassOf( const ITEM* aItem )
    {
        return aItem && VIA_T == aItem->Kind();
    }

    STACK_MODE StackMode() const { return m_stackMode; }

    void SetStackMode( STACK_MODE aStackMode );

    int EffectiveLayer( int aLayer ) const;

    std::vector<int> UniqueShapeLayers() const override;

    bool HasUniqueShapeLayers() const override { return true; }

    const VECTOR2I& Pos() const { return m_pos; }

    void SetPos( const VECTOR2I& aPos )
    {
        m_pos = aPos;

        for( auto& [layer, shape] : m_shapes )
            shape.SetCenter( aPos );

        if( m_hole )
            m_hole->SetCenter( aPos );
    }

    VIATYPE ViaType() const { return m_viaType; }
    void SetViaType( VIATYPE aViaType ) { m_viaType = aViaType; }

    PADSTACK::UNCONNECTED_LAYER_MODE UnconnectedLayerMode() const { return m_unconnectedLayerMode; }
    void SetUnconnectedLayerMode( PADSTACK::UNCONNECTED_LAYER_MODE aMode )
    {
        m_unconnectedLayerMode = aMode;
    }

    bool ConnectsLayer( int aLayer ) const;

    int Diameter( int aLayer ) const
    {
        int layer = EffectiveLayer( aLayer );
        wxCHECK( m_diameters.contains( layer ), m_diameters.begin()->second );
        return m_diameters.at( layer );
    }

    void SetDiameter( int aLayer, int aDiameter )
    {
        int layer = EffectiveLayer( aLayer );
        m_diameters[layer] = aDiameter;

        if( !m_shapes.contains( layer ) )
            m_shapes[layer] = SHAPE_CIRCLE( m_pos, aDiameter / 2 );
        else
            m_shapes[layer].SetRadius( aDiameter / 2 );
    }

    bool PadstackMatches( const VIA& aOther ) const;

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

    const SHAPE* Shape( int aLayer ) const override
    {
        int layer = EffectiveLayer( aLayer );
        wxCHECK( m_shapes.contains( layer ), nullptr );
        return &m_shapes.at( layer );
    }

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
    STACK_MODE   m_stackMode;

    /// May contain 1..n diameters depending on m_stackMode
    std::map<int, int> m_diameters;
    std::map<int, SHAPE_CIRCLE> m_shapes;

    int          m_drill;
    VECTOR2I     m_pos;
    VIATYPE      m_viaType;
    PADSTACK::UNCONNECTED_LAYER_MODE m_unconnectedLayerMode;
    bool         m_isFree;
    HOLE*        m_hole;
};


class VVIA : public VIA
{
public:
    VVIA( const VECTOR2I& aPos, int aLayer, int aDiameter, NET_HANDLE aNet ) :
        VIA( aPos, PNS_LAYER_RANGE( aLayer, aLayer ), aDiameter, aDiameter / 2, aNet )
    {
        m_isVirtual = true;
    }

    bool HasHole() const override { return false; }
};

}

#endif
