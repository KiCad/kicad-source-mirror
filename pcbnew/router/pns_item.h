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

#ifndef __PNS_ITEM_H
#define __PNS_ITEM_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "trace.h"

#include "pns_layerset.h"

class BOARD_CONNECTED_ITEM;
class PNS_NODE;

enum LineMarker {
    MK_HEAD         = ( 1 << 0 ),
    MK_VIOLATION    = ( 1 << 3 ),
    MK_LOCKED       = ( 1 << 4 ),
    MK_DP_COUPLED   = ( 1 << 5 )
};


/**
 * Class PNS_ITEM
 *
 * Base class for PNS router board items. Implements the shared properties of all PCB items -
 * net, spanned layers, geometric shape & refererence to owning model.
 */
class PNS_ITEM
{
public:
    static const int UnusedNet = INT_MAX;

    ///> Supported item types
    enum PnsKind
    {
        SOLID   = 1,
        LINE    = 2,
        JOINT   = 4,
        SEGMENT = 8,
        VIA     = 16,
        DIFF_PAIR = 32,
        ANY     = 0xff
    };

    PNS_ITEM( PnsKind aKind )
    {
        m_net = UnusedNet;
        m_movable = true;
        m_kind = aKind;
        m_parent = NULL;
        m_owner = NULL;
        m_marker = 0;
        m_rank = -1;
    }

    PNS_ITEM( const PNS_ITEM& aOther )
    {
        m_layers = aOther.m_layers;
        m_net = aOther.m_net;
        m_movable = aOther.m_movable;
        m_kind = aOther.m_kind;
        m_parent = aOther.m_parent;
        m_owner = NULL;
        m_marker = aOther.m_marker;
        m_rank = aOther.m_rank;
    }

    virtual ~PNS_ITEM();

    /**
     * Function Clone()
     *
     * Returns a deep copy of the item
     */
    virtual PNS_ITEM* Clone() const = 0;

    /*
     * Function Hull()
     *
     * Returns a convex polygon "hull" of a the item, that is used as the walk-around
     * path.
     * @param aClearance defines how far from the body of the item the hull should be,
     * @param aWalkaroundThickness is the width of the line that walks around this hull.
     */
    virtual const SHAPE_LINE_CHAIN Hull( int aClearance = 0, int aWalkaroundThickness = 0 ) const
    {
        return SHAPE_LINE_CHAIN();
    }

    /**
     * Function Kind()
     *
     * Returns the type (kind) of the item
     */
    PnsKind Kind() const
    {
        return m_kind;
    }

    /**
     * Function OfKind()
     *
     * Returns true if the item's type matches the mask aKindMask.
     */
    bool OfKind( int aKindMask ) const
    {
        return ( aKindMask & m_kind ) != 0;
    }

    /**
     * Function KindStr()
     *
     * Returns the kind of the item, as string
     */
    const std::string KindStr() const;

    /**
     * Function SetParent()
     *
     * Sets the corresponding parent object in the host application's model.
     */
    void SetParent( BOARD_CONNECTED_ITEM* aParent )
    {
        m_parent = aParent;
    }

    /**
     * Function Parent()
     *
     * Returns the corresponding parent object in the host application's model.
     */
    BOARD_CONNECTED_ITEM* Parent() const
    {
        return m_parent;
    }

    /**
     * Function SetNet()
     *
     * Sets the item's net to aNet
     */
    void SetNet( int aNet )
    {
        m_net = aNet;
    }

    /**
     * Function Net()
     *
     * Returns the item's net.
     */
    int Net() const
    {
        return m_net;
    }

    /**
     * Function SetLayers()
     *
     * Sets the layers spanned by the item to aLayers.
     */
    void SetLayers( const PNS_LAYERSET& aLayers )
    {
        m_layers = aLayers;
    }

    /**
     * Function SetLayer()
     *
     * Sets the layers spanned by the item to a single layer aLayer.
     */
    void SetLayer( int aLayer )
    {
        m_layers = PNS_LAYERSET( aLayer, aLayer );
    }

    /**
     * Function Layers()
     *
     * Returns the contiguous set of layers spanned by the item.
     */
    const PNS_LAYERSET& Layers() const
    {
        return m_layers;
    }

    /**
     * Function Layer()
     *
     * Returns the item's layer, for single-layered items only.
     */
    virtual int Layer() const
    {
        return Layers().Start();
    }

    /**
     * Function LayersOverlap()
     *
     * Returns true if the set of layers spanned by aOther overlaps our
     * layers.
     */
    bool LayersOverlap( const PNS_ITEM* aOther ) const
    {
        return Layers().Overlaps( aOther->Layers() );
    }

    /**
     * Functon SetOwner()
     *
     * Sets the node that owns this item. An item can belong to a single
     * PNS_NODE or stay unowned.
     */
    void SetOwner( PNS_NODE* aOwner )
    {
        m_owner = aOwner;
    }

    /**
     * Function BelongsTo()
     *
     * @return true if the item is owned by the node aNode.
     */
    bool BelongsTo( PNS_NODE* aNode ) const
    {
        return m_owner == aNode;
    }

    /**
     * Function Owner()
     *
     * Returns the owner of this item, or NULL if there's none.
     */
    PNS_NODE* Owner() const { return m_owner; }

    /**
     * Function Collide()
     *
     * Checks for a collision (clearance violation) with between us and item aOther.
     * Collision checking takes all PCB stuff into accound (layers, nets, DRC rules).
     * Optionally returns a minimum translation vector for force propagation
     * algorithm.
     *
     * @param aOther item to check collision against
     * @param aClearance desired clearance
     * @param aNeedMTV when true, the minimum translation vector is calculated
     * @param aMTV the minimum translation vector
     * @return true, if a collision was found.
     */
    virtual bool Collide( const PNS_ITEM* aOther, int aClearance, bool aNeedMTV,
            VECTOR2I& aMTV,  bool aDifferentNetsOnly = true ) const;

    /**
     * Function Collide()
     *
     * A shortcut for PNS_ITEM::Colllide() without MTV stuff.
     */
  	bool Collide( const PNS_ITEM* aOther, int aClearance, bool aDifferentNetsOnly = true ) const
    {
        VECTOR2I dummy;

        return Collide( aOther, aClearance, false, dummy, aDifferentNetsOnly );
    }

    /**
     * Function Shape()
     *
     * Returns the geometrical shape of the item. Used
     * for collision detection & spatial indexing.
     */
    virtual const SHAPE* Shape() const
    {
        return NULL;
    }

    virtual void Mark(int aMarker)
    {
        m_marker = aMarker;
    }

    virtual void Unmark ()
    {
        m_marker = 0;
    }

    virtual int Marker() const
    {
        return m_marker;
    }

    virtual void SetRank( int aRank )
    {
        m_rank = aRank;
    }

    virtual int Rank() const
    {
        return m_rank;
    }

    virtual VECTOR2I Anchor( int n ) const
    {
        return VECTOR2I ();
    }

    virtual int AnchorCount() const
    {
        return 0;
    }

private:
    bool collideSimple( const PNS_ITEM* aOther, int aClearance, bool aNeedMTV,
            VECTOR2I& aMTV, bool aDifferentNetsOnly ) const;

protected:
    PnsKind                 m_kind;

    BOARD_CONNECTED_ITEM*   m_parent;
    PNS_NODE*               m_owner;
    PNS_LAYERSET            m_layers;

    bool                    m_movable;
    int                     m_net;
    int                     m_marker;
    int                     m_rank;
};

#endif    // __PNS_ITEM_H
