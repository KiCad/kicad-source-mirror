/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_layerset.h"

class BOARD_CONNECTED_ITEM;

namespace PNS {

class NODE;

enum LineMarker {
    MK_HEAD         = ( 1 << 0 ),
    MK_VIOLATION    = ( 1 << 3 ),
    MK_LOCKED       = ( 1 << 4 ),
    MK_DP_COUPLED   = ( 1 << 5 )
};


/**
 * Class ITEM
 *
 * Base class for PNS router board items. Implements the shared properties of all PCB items -
 * net, spanned layers, geometric shape & refererence to owning model.
 */
class ITEM
{
public:
    static const int UnusedNet = INT_MAX;

    ///> Supported item types
    enum PnsKind
    {
        SOLID_T     =    1,
        LINE_T      =    2,
        JOINT_T     =    4,
        SEGMENT_T   =    8,
        VIA_T       =   16,
        DIFF_PAIR_T =   32,
        ANY_T       = 0xff
    };

    ITEM( PnsKind aKind )
    {
        m_net = UnusedNet;
        m_movable = true;
        m_kind = aKind;
        m_parent = NULL;
        m_owner = NULL;
        m_marker = 0;
        m_rank = -1;
        m_routable = true;
    }

    ITEM( const ITEM& aOther )
    {
        m_layers = aOther.m_layers;
        m_net = aOther.m_net;
        m_movable = aOther.m_movable;
        m_kind = aOther.m_kind;
        m_parent = aOther.m_parent;
        m_owner = aOther.m_owner; // fixme: wtf this was null?
        m_marker = aOther.m_marker;
        m_rank = aOther.m_rank;
        m_routable = aOther.m_routable;
    }

    virtual ~ITEM();

    /**
     * Function Clone()
     *
     * Returns a deep copy of the item
     */
    virtual ITEM* Clone() const = 0;

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
    std::string KindStr() const;

    void SetParent( BOARD_CONNECTED_ITEM* aParent ) { m_parent = aParent; }
    BOARD_CONNECTED_ITEM* Parent() const { return m_parent; }

    void SetNet( int aNet ) { m_net = aNet; }
    int Net() const { return m_net;  }

    const LAYER_RANGE& Layers() const { return m_layers; }
    void SetLayers( const LAYER_RANGE& aLayers ) { m_layers = aLayers; }

    void SetLayer( int aLayer ) { m_layers = LAYER_RANGE( aLayer, aLayer ); }
    virtual int Layer() const { return Layers().Start(); }

    /**
     * Function LayersOverlap()
     *
     * Returns true if the set of layers spanned by aOther overlaps our
     * layers.
     */
    bool LayersOverlap( const ITEM* aOther ) const
    {
        return Layers().Overlaps( aOther->Layers() );
    }

    /**
     * Function Owner()
     *
     * Returns the owner of this item, or NULL if there's none.
     */
    NODE* Owner() const { return m_owner; }

    /**
     * Functon SetOwner()
     *
     * Sets the node that owns this item. An item can belong to a single NODE or be unowned.
     */
    void SetOwner( NODE* aOwner ) { m_owner = aOwner; }

    /**
     * Function BelongsTo()
     *
     * @return true if the item is owned by the node aNode.
     */
    bool BelongsTo( NODE* aNode ) const
    {
        return m_owner == aNode;
    }

    /**
     * Function Collide()
     *
     * Checks for a collision (clearance violation) with between us and item aOther.
     * Collision checking takes all PCB stuff into accound (layers, nets, DRC rules).
     * Optionally returns a minimum translation vector for force propagation algorithm.
     *
     * @param aOther item to check collision against
     * @param aClearance desired clearance
     * @param aNeedMTV when true, the minimum translation vector is calculated
     * @param aMTV the minimum translation vector
     * @return true, if a collision was found.
     */
    virtual bool Collide( const ITEM* aOther, int aClearance, bool aNeedMTV, VECTOR2I* aMTV,
                          const NODE* aParentNode, bool aDifferentNetsOnly = true ) const;

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

    virtual void Mark( int aMarker ) { m_marker = aMarker; }
    virtual void Unmark( int aMarker = -1 ) { m_marker &= ~aMarker; }
    virtual int Marker() const { return m_marker; }

    virtual void SetRank( int aRank ) { m_rank = aRank; }
    virtual int Rank() const { return m_rank; }

    virtual VECTOR2I Anchor( int n ) const
    {
        return VECTOR2I();
    }

    virtual int AnchorCount() const
    {
        return 0;
    }

    bool IsLocked() const
    {
        return Marker() & MK_LOCKED;
    }

    void SetRoutable( bool aRoutable ) { m_routable = aRoutable; }
    bool IsRoutable() const { return m_routable; }

private:
    bool collideSimple( const ITEM* aOther, int aClearance, bool aNeedMTV, VECTOR2I* aMTV,
                        const NODE* aParentNode, bool aDifferentNetsOnly ) const;

protected:
    PnsKind                 m_kind;

    BOARD_CONNECTED_ITEM*   m_parent;
    NODE*                   m_owner;
    LAYER_RANGE             m_layers;

    bool                    m_movable;
    int                     m_net;
    int                     m_marker;
    int                     m_rank;
    bool                    m_routable;
};

template< typename T, typename S >
std::unique_ptr< T > ItemCast( std::unique_ptr< S > aPtr )
{
    static_assert(std::is_base_of< ITEM, S >::value, "Need to be handed a ITEM!");
    static_assert(std::is_base_of< ITEM, T >::value, "Need to cast to an ITEM!");
    return std::unique_ptr< T >( static_cast<T*>(aPtr.release()) );
}

template< typename T >
std::unique_ptr< typename std::remove_const< T >::type > Clone( const T& aItem )
{
    static_assert(std::is_base_of< ITEM, T >::value, "Need to be handed an ITEM!");
    return std::unique_ptr< typename std::remove_const< T >::type >( aItem.Clone() );
}

}

#endif    // __PNS_ITEM_H
