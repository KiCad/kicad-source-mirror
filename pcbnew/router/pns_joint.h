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

#ifndef __PNS_JOINT_H
#define __PNS_JOINT_H

#include <vector>
#include <boost/functional/hash.hpp>

#include <math/vector2d.h>

#include "pns_item.h"
#include "pns_segment.h"
#include "pns_itemset.h"

/**
 * Class PNS_JOINT
 *
 * Represents a 2D point on a given set of layers and belonging to a certain
 * net, that links together a number of board items.
 * A hash table of joints is used by the router to follow connectivity between
 * the items.
 **/
class PNS_JOINT : public PNS_ITEM
{
public:
    typedef std::deque<PNS_ITEM*> LINKED_ITEMS;

    ///> Joints are hashed by their position, layers and net.
    ///  Linked items are, obviously, not hashed
    struct HASH_TAG
    {
        VECTOR2I pos;
        int net;
    };

    PNS_JOINT() :
        PNS_ITEM( JOINT ), m_locked( false ) {}

    PNS_JOINT( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers, int aNet = -1 ) :
        PNS_ITEM( JOINT )
    {
        m_tag.pos = aPos;
        m_tag.net = aNet;
        m_layers = aLayers;
        m_locked = false;
    }

    PNS_JOINT( const PNS_JOINT& aB ) :
        PNS_ITEM( JOINT )
    {
        m_layers = aB.m_layers;
        m_tag.pos = aB.m_tag.pos;
        m_tag.net = aB.m_tag.net;
        m_linkedItems = aB.m_linkedItems;
        m_layers = aB.m_layers;
        m_locked = false;
    }

    PNS_ITEM* Clone( ) const
    {
        assert( false );
        return NULL;
    }

    ///> Returns true if the joint is a trivial line corner, connecting two
    /// segments of the same net, on the same layer.
    bool IsLineCorner() const
    {
        if( m_locked )
            return false;

        if( m_linkedItems.Size() != 2 )
            return false;

        if( m_linkedItems[0]->Kind() != SEGMENT || m_linkedItems[1]->Kind() != SEGMENT )
            return false;

        PNS_SEGMENT* seg1 = static_cast<PNS_SEGMENT*>( m_linkedItems[0] );
        PNS_SEGMENT* seg2 = static_cast<PNS_SEGMENT*>( m_linkedItems[1] );

        // joints between segments of different widths are not considered trivial.
        return seg1->Width() == seg2->Width();
    }

    bool IsNonFanoutVia() const
    {
        if( m_linkedItems.Size() != 3 )
            return false;

        int vias = 0, segs = 0;

        for( int i = 0; i < 3; i++ )
        {
            vias += m_linkedItems[i]->Kind() == VIA ? 1 : 0;
            segs += m_linkedItems[i]->Kind() == SEGMENT ? 1 : 0;
        }

        return ( vias == 1 && segs == 2 );
    }

    ///> Links the joint to a given board item (when it's added to the PNS_NODE)
    void Link( PNS_ITEM* aItem )
    {
        if( m_linkedItems.Contains( aItem ) )
            return;

        m_linkedItems.Add( aItem );
    }

    ///> Unlinks a given board item from the joint (upon its removal from a PNS_NODE)
    ///> Returns true if the joint became dangling after unlinking.
    bool Unlink( PNS_ITEM* aItem )
    {
        m_linkedItems.Erase( aItem );
        return m_linkedItems.Size() == 0;
    }

    ///> For trivial joints, returns the segment adjacent to (aCurrent). For non-trival ones, returns
    ///> NULL, indicating the end of line.
    PNS_SEGMENT* NextSegment( PNS_SEGMENT* aCurrent ) const
    {
        if( !IsLineCorner() )
            return NULL;

        return static_cast<PNS_SEGMENT*>( m_linkedItems[m_linkedItems[0] == aCurrent ? 1 : 0] );
    }

    PNS_VIA* Via()
    {
        BOOST_FOREACH( PNS_ITEM* item, m_linkedItems.Items() )
        {
            if( item->OfKind( VIA ) )
                return static_cast<PNS_VIA*>( item );
        }

        return NULL;
    }


    /// trivial accessors
    const HASH_TAG& Tag() const
    {
        return m_tag;
    }

    const VECTOR2I& Pos() const
    {
        return m_tag.pos;
    }

    int Net() const
    {
        return m_tag.net;
    }

    const LINKED_ITEMS& LinkList() const
    {
        return m_linkedItems.CItems();
    }

    const PNS_ITEMSET& CLinks() const
    {
        return m_linkedItems;
    }

    PNS_ITEMSET& Links()
    {
        return m_linkedItems;
    }

    int LinkCount( int aMask = -1 ) const
    {
        return m_linkedItems.Count( aMask );
    }

    void Dump() const;

    bool operator==( const PNS_JOINT& rhs )  const
    {
        return m_tag.pos == rhs.m_tag.pos && m_tag.net == rhs.m_tag.net;
    }

    void Merge( const PNS_JOINT& aJoint )
    {
        if( !Overlaps( aJoint ) )
            return;

        m_layers.Merge( aJoint.m_layers );

        BOOST_FOREACH( PNS_ITEM* item, aJoint.LinkList() )
        {
            m_linkedItems.Add( item );
        }
    }

    bool Overlaps( const PNS_JOINT& rhs ) const
    {
        return m_tag.pos == rhs.m_tag.pos &&
            m_tag.net == rhs.m_tag.net && m_layers.Overlaps( rhs.m_layers );
    }

    void Lock( bool aLock = true )
    {
        m_locked = aLock;
    }

    bool IsLocked() const
    {
        return m_locked;
    }

private:
    ///> hash tag for unordered_multimap
    HASH_TAG m_tag;

    ///> list of items linked to this joint
    PNS_ITEMSET m_linkedItems;

    ///> locked (non-movable) flag
    bool m_locked;
};

inline bool operator==( PNS_JOINT::HASH_TAG const& aP1, PNS_JOINT::HASH_TAG const& aP2 )
{
    return aP1.pos == aP2.pos && aP1.net == aP2.net;
}

inline std::size_t hash_value( PNS_JOINT::HASH_TAG const& aP )
{
    std::size_t seed = 0;
    boost::hash_combine( seed, aP.pos.x );
    boost::hash_combine( seed, aP.pos.y );
    boost::hash_combine( seed, aP.net );

    return seed;
}

#endif    // __PNS_JOINT_H
