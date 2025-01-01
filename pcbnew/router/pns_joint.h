/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#ifndef __PNS_JOINT_H
#define __PNS_JOINT_H

#include <vector>

#include <math/vector2d.h>

#include "pns_item.h"
#include "pns_segment.h"
#include "pns_itemset.h"

namespace PNS {

/**
 * A 2D point on a given set of layers and belonging to a certain net, that links
 * together a number of board items.
 *
 * A hash table of joints is used by the router to follow connectivity between the items.
 */
class JOINT : public ITEM
{
public:
    ///< Joints are hashed by their position, layers and net.
    ///<  Linked items are, obviously, not hashed.
    struct HASH_TAG
    {
        VECTOR2I pos;
        NET_HANDLE net;
    };

    struct JOINT_TAG_HASH
    {
        std::size_t operator()( const JOINT::HASH_TAG& aP ) const
        {
            using std::size_t;
            using std::hash;
            using std::string;

            return ( (hash<int>()( aP.pos.x )
                      ^ (hash<int>()( aP.pos.y ) << 1) ) >> 1 )
                   ^ (hash<void*>()( aP.net ) << 1);
        }
    };

    JOINT() :
        ITEM( JOINT_T ), m_tag(), m_locked( false ) {}

    JOINT( const VECTOR2I& aPos, const PNS_LAYER_RANGE& aLayers, NET_HANDLE aNet = nullptr ) :
        ITEM( JOINT_T )
    {
        m_tag.pos = aPos;
        m_tag.net = aNet;
        m_layers = aLayers;
        m_locked = false;
    }

    JOINT( const JOINT& aB ) :
        ITEM( JOINT_T )
    {
        m_layers = aB.m_layers;
        m_tag.pos = aB.m_tag.pos;
        m_tag.net = aB.m_tag.net;
        m_linkedItems = aB.m_linkedItems;
        m_layers = aB.m_layers;
        m_locked = aB.m_locked;
    }

    ITEM* Clone( ) const override
    {
        assert( false );
        return nullptr;
    }

    /**
     * Checks if a joint connects two segments of the same net, layer, and width.
     * @param aAllowLockedSegs will consider joints between locked and unlocked segments as trivial
     * @return true if the joint is a trivial line corner
     */
    bool IsLineCorner( bool aAllowLockedSegs = false ) const
    {
        if( m_linkedItems.Size() == 2 && m_linkedItems.Count( SEGMENT_T | ARC_T ) == 2 )
        {
            LINKED_ITEM* seg1 = static_cast<LINKED_ITEM*>( m_linkedItems[0] );
            LINKED_ITEM* seg2 = static_cast<LINKED_ITEM*>( m_linkedItems[1] );

            if( !aAllowLockedSegs && ( seg1->IsLocked() || seg2->IsLocked() ) )
                return false;

            // joints between segments of different widths are not considered trivial.
            return seg1->Width() == seg2->Width();
        }
        else if( m_linkedItems.Size() > 2 && m_linkedItems.Count( SEGMENT_T | ARC_T ) == 2 )
        {
            if( !aAllowLockedSegs )
                return false;

            // There will be multiple VVIAs on joints between two locked segments, because we
            // naively add a VVIA to each end of a locked segment.
            const LINKED_ITEM* seg1 = nullptr;
            const LINKED_ITEM* seg2 = nullptr;

            for( const ITEM* item : m_linkedItems.CItems() )
            {
                if( item->IsVirtual() )
                    continue;

                if( item->Kind() == SEGMENT_T || item->Kind() == ARC_T )
                {
                    if( !seg1 )
                        seg1 = static_cast<const LINKED_ITEM*>( item );
                    else
                        seg2 = static_cast<const LINKED_ITEM*>( item );
                }
                else
                {
                    return false;
                }
            }

            if( seg1 && seg2 )
                return seg1->Width() == seg2->Width();
        }

        return false;
    }

    bool IsNonFanoutVia() const
    {
        int vias = 0;
        int segs = 0;
        int realItems = 0;

        for( const ITEM* item : m_linkedItems.CItems() )
        {
            if( item->IsVirtual() )
                continue;

            if( item->Kind() == VIA_T )
                vias++;
            else if( item->Kind() == SEGMENT_T || item->Kind() == ARC_T )
                segs++;

            realItems++;
        }

        return ( realItems == 3 && vias == 1 && segs == 2 );
    }

    bool IsStitchingVia() const
    {
        return ( m_linkedItems.Size() == 1 && m_linkedItems.Count( VIA_T ) == 1 );
    }

    bool IsTrivialEndpoint() const
    {
        // fixme: Arcs & trivial endpoint vias
        return m_linkedItems.Size() == 1 && m_linkedItems.Count( SEGMENT_T ) == 1;
    }


    bool IsTraceWidthChange() const
    {
        if( m_linkedItems.Count( SEGMENT_T ) != 2 )
            return false;

        const LINKED_ITEM* seg1 = nullptr;
        const LINKED_ITEM* seg2 = nullptr;

        for( const ITEM* item : m_linkedItems.CItems() )
        {
            if( item->IsVirtual() )
                continue;

            if( item->Kind() == VIA_T )
            {
                return false;
            }
            else if( item->Kind() == SEGMENT_T || item->Kind() == ARC_T )
            {
                if( !seg1 )
                    seg1 = static_cast<const LINKED_ITEM*>( item );
                else
                    seg2 = static_cast<const LINKED_ITEM*>( item );
            }
        }

        wxCHECK( seg1 && seg2, false );

        return seg1->Width() != seg2->Width();
    }

    ///< Link the joint to a given board item (when it's added to the NODE).
    void Link( ITEM* aItem )
    {
        if( m_linkedItems.Contains( aItem ) )
            return;

        m_linkedItems.Add( aItem );
    }

    ///< Unlink a given board item from the joint (upon its removal from a NODE)
    ///< @return true if the joint became dangling after unlinking.
    bool Unlink( ITEM* aItem )
    {
        m_linkedItems.Erase( aItem );
        if( m_linkedItems.Size() == 0 )
            m_layers = PNS_LAYER_RANGE( -1 );
        return m_linkedItems.Size() == 0;
    }

    ///< For trivial joints, return the segment adjacent to (aCurrent). For non-trival ones,
    ///< return NULL, indicating the end of line.
    LINKED_ITEM* NextSegment( LINKED_ITEM* aCurrent, bool aAllowLockedSegs = false ) const
    {
        const std::vector<ITEM*>& citems = m_linkedItems.CItems();
        const size_t              size = citems.size();

        LINKED_ITEM* otherItem = nullptr;

        for( size_t i = 0; i < size; i++ )
        {
            ITEM* item = m_linkedItems[i];

            if( item != aCurrent )
            {
                if ( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
                {
                    if ( item->Net() == aCurrent->Net() && item->Layers().Overlaps( aCurrent->Layers() ) )
                    {
                        if( otherItem )
                            return nullptr;

                        if( !item->IsLocked() || aAllowLockedSegs )
                            otherItem = static_cast<LINKED_ITEM*>( item );
                    }
                }
                else if ( item->OfKind( ITEM::SOLID_T | ITEM::VIA_T ) )
                {
                    if( item->Kind() == ITEM::VIA_T && item->IsVirtual() && aAllowLockedSegs )
                    {
                        // Virtual via will be added at the joint between an unlocked and locked seg
                        continue;
                    }

                    return nullptr;
                }
            }
        }

        return otherItem;
    }

    VIA* Via() const
    {
        for( ITEM* item : m_linkedItems.CItems() )
        {
            if( item->OfKind( VIA_T ) )
                return static_cast<VIA*>( item ); // fixme: const correctness
        }

        return nullptr;
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

    NET_HANDLE Net() const override
    {
        return m_tag.net;
    }

    const std::vector<ITEM*>& LinkList() const
    {
        return m_linkedItems.CItems();
    }

    const ITEM_SET& CLinks() const
    {
        return m_linkedItems;
    }

    ITEM_SET& Links()
    {
        return m_linkedItems;
    }

    int LinkCount( int aMask = -1 ) const
    {
        return m_linkedItems.Count( aMask );
    }

    void Dump() const;

    bool operator==( const JOINT& rhs )  const
    {
        return m_tag.pos == rhs.m_tag.pos && m_tag.net == rhs.m_tag.net;
    }

    void Merge( const JOINT& aJoint )
    {
        if( !Overlaps( aJoint ) )
            return;

        m_layers.Merge( aJoint.m_layers );

        if( aJoint.IsLocked() )
            m_locked = true;

        for( ITEM* item : aJoint.LinkList() )
        {
            m_linkedItems.Add( item );
        }
    }

    bool Overlaps( const JOINT& rhs ) const
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
    ///< hash tag for unordered_multimap
    HASH_TAG m_tag;

    ///< list of items linked to this joint
    ITEM_SET m_linkedItems;

    ///< locked (non-movable) flag
    bool m_locked;
};

inline bool operator==( JOINT::HASH_TAG const& aP1, JOINT::HASH_TAG const& aP2 )
{
    return aP1.pos == aP2.pos && aP1.net == aP2.net;
}

}

#endif    // __PNS_JOINT_H
