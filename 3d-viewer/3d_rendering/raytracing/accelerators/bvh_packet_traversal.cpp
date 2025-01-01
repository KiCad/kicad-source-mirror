/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file bvh_packet_traversal.cpp
 * @brief This file implements packet traversal over the BVH PBRT implementation.
 */

#include "bvh_pbrt.h"


#define BVH_RANGED_TRAVERSAL
//#define BVH_PARTITION_TRAVERSAL


#define MAX_TODOS 64


struct StackNode
{
    int             cell;
    unsigned int    ia;     // Index to the first alive ray
};


static inline unsigned int getFirstHit( const RAYPACKET& aRayPacket, const BBOX_3D& aBBox,
                                        unsigned int ia, HITINFO_PACKET* aHitInfoPacket )
{
    float hitT;

    if( aBBox.Intersect( aRayPacket.m_ray[ia], &hitT )
      && ( hitT < aHitInfoPacket[ia].m_HitInfo.m_tHit ) )
        return ia;

    if( !aRayPacket.m_Frustum.Intersect( aBBox ) )
        return RAYPACKET_RAYS_PER_PACKET;

    for( unsigned int i = ia + 1; i < RAYPACKET_RAYS_PER_PACKET; ++i )
    {
        if( aBBox.Intersect( aRayPacket.m_ray[i], &hitT )
          && ( hitT < aHitInfoPacket[i].m_HitInfo.m_tHit ) )
            return i;
    }

    return RAYPACKET_RAYS_PER_PACKET;
}


#ifdef BVH_RANGED_TRAVERSAL

static inline unsigned int getLastHit( const RAYPACKET& aRayPacket, const BBOX_3D& aBBox,
                                       unsigned int ia, HITINFO_PACKET* aHitInfoPacket )
{
    for( unsigned int ie = (RAYPACKET_RAYS_PER_PACKET - 1); ie > ia; --ie )
    {
        float hitT;

        if( aBBox.Intersect( aRayPacket.m_ray[ie], &hitT )
          && ( hitT < aHitInfoPacket[ie].m_HitInfo.m_tHit ) )
            return ie + 1;
    }

    return ia + 1;
}


// "Large Ray Packets for Real-time Whitted Ray Tracing"
// http://cseweb.ucsd.edu/~ravir/whitted.pdf

// Ranged Traversal
bool BVH_PBRT::Intersect( const RAYPACKET& aRayPacket, HITINFO_PACKET* aHitInfoPacket ) const
{
    if( m_nodes == nullptr )
        return false;

    if( &m_nodes[0] == nullptr )
        return false;

    bool anyHit = false;
    int todoOffset = 0, nodeNum = 0;
    StackNode todo[MAX_TODOS];

    unsigned int ia = 0;

    while( true )
    {
        const LinearBVHNode *curCell = &m_nodes[nodeNum];

        ia = getFirstHit( aRayPacket, curCell->bounds, ia, aHitInfoPacket );

        if( ia < RAYPACKET_RAYS_PER_PACKET )
        {
            if( curCell->nPrimitives == 0 )
            {
                StackNode& node = todo[todoOffset++];
                node.cell = curCell->secondChildOffset;
                node.ia = ia;
                nodeNum = nodeNum + 1;
                continue;
            }
            else
            {
                const unsigned int ie = getLastHit( aRayPacket, curCell->bounds, ia,
                                                    aHitInfoPacket );

                for( int j = 0; j < curCell->nPrimitives; ++j )
                {
                    const OBJECT_3D* obj = m_primitives[curCell->primitivesOffset + j];

                    if( aRayPacket.m_Frustum.Intersect( obj->GetBBox() ) )
                    {
                        for( unsigned int i = ia; i < ie; ++i )
                        {
                            const bool hit = obj->Intersect( aRayPacket.m_ray[i],
                                                             aHitInfoPacket[i].m_HitInfo );

                            if( hit )
                            {
                                anyHit |= hit;
                                aHitInfoPacket[i].m_hitresult |= hit;
                                aHitInfoPacket[i].m_HitInfo.m_acc_node_info = nodeNum;
                            }
                        }
                    }
                }
            }
        }

        if( todoOffset == 0 )
            break;

        const StackNode& node = todo[--todoOffset];

        nodeNum = node.cell;
        ia = node.ia;
    }

    return anyHit;

}
#endif


// "Ray Tracing Deformable Scenes Using Dynamic Bounding Volume Hierarchies"
// http://www.cs.cmu.edu/afs/cs/academic/class/15869-f11/www/readings/wald07_packetbvh.pdf

#ifdef BVH_PARTITION_TRAVERSAL

static inline unsigned int getLastHit( const RAYPACKET& aRayPacket, const BBOX_3D& aBBox,
                                       unsigned int ia, const unsigned int* aRayIndex,
                                       HITINFO_PACKET* aHitInfoPacket )
{
    for( unsigned int ie = (RAYPACKET_RAYS_PER_PACKET - 1); ie > ia; --ie )
    {
        float hitT;

        if( aBBox.Intersect( aRayPacket.m_ray[ aRayIndex[ie] ], &hitT )
          && ( hitT < aHitInfoPacket[ aRayIndex[ie] ].m_HitInfo.m_tHit ) )
            return ie + 1;
    }

    return ia + 1;
}


static inline unsigned int partRays( const RAYPACKET& aRayPacket, const BBOX_3D& aBBox,
                                     unsigned int ia, unsigned int* aRayIndex,
                                     HITINFO_PACKET* aHitInfoPacket )
{

    if( !aRayPacket.m_Frustum.Intersect( aBBox ) )
        return RAYPACKET_RAYS_PER_PACKET;

    unsigned int ie = 0;

    for( unsigned int i = 0; i < ia; ++i )
    {
        float hitT;

        if( aBBox.Intersect( aRayPacket.m_ray[ aRayIndex[i] ], &hitT )
          && ( hitT < aHitInfoPacket[ aRayIndex[i] ].m_HitInfo.m_tHit ) )
            std::swap( aRayIndex[ie++], aRayIndex[i] );
    }

    return ie;
}


bool BVH_PBRT::Intersect( const RAYPACKET& aRayPacket, HITINFO_PACKET* aHitInfoPacket ) const
{
    bool anyHit = false;
    int todoOffset = 0, nodeNum = 0;
    StackNode todo[MAX_TODOS];

    unsigned int I[RAYPACKET_RAYS_PER_PACKET];

    memcpy( I, m_I, RAYPACKET_RAYS_PER_PACKET * sizeof( unsigned int ) );

    unsigned int ia = 0;

    while( true )
    {
        const LinearBVHNode *curCell = &m_nodes[nodeNum];

        ia = partRays( aRayPacket, curCell->bounds, ia, I, aHitInfoPacket );

        if( ia < RAYPACKET_RAYS_PER_PACKET )
        {
            if( curCell->nPrimitives == 0 )
            {
                StackNode& node = todo[todoOffset++];
                node.cell = curCell->secondChildOffset;
                node.ia = ia;
                nodeNum = nodeNum + 1;
                continue;
            }
            else
            {
                unsigned int ie = getLastHit( aRayPacket, curCell->bounds, ia, I,
                                              aHitInfoPacket );

                for( int j = 0; j < curCell->nPrimitives; ++j )
                {
                    const OBJECT_3D* obj = m_primitives[curCell->primitivesOffset + j];

                    if( aRayPacket.m_Frustum.Intersect( obj->GetBBox() ) )
                    {
                        for( unsigned int i = 0; i < ie; ++i )
                        {
                            unsigned int idx = I[i];

                            bool hit = obj->Intersect( aRayPacket.m_ray[idx],
                                                       aHitInfoPacket[idx].m_HitInfo );

                            if( hit )
                            {
                                anyHit |= hit;
                                aHitInfoPacket[idx].m_hitresult |= hit;
                                aHitInfoPacket[idx].m_HitInfo.m_acc_node_info = nodeNum;
                            }
                        }
                    }
                }
            }
        }

        if( todoOffset == 0 )
            break;

        const StackNode& node = todo[--todoOffset];

        nodeNum = node.cell;
        ia = node.ia;
    }

    return anyHit;
}
#endif
