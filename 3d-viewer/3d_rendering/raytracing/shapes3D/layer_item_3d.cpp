/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2022 Mario Luzeiro <mrluzeiro@ua.pt>
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

#include "layer_item_3d.h"
#include "3d_fastmath.h"
#include <wx/debug.h>
#include <advanced_config.h>


extern float g_BevelThickness3DU;


LAYER_ITEM::LAYER_ITEM( const OBJECT_2D* aObject2D, float aZMin, float aZMax ) :
    OBJECT_3D( OBJECT_3D_TYPE::LAYERITEM ),
    m_object2d( aObject2D )
{
    wxASSERT( aObject2D );

    BBOX_2D bbox2d = m_object2d->GetBBox();
    bbox2d.ScaleNextUp();
    bbox2d.ScaleNextUp();

    m_bbox.Reset();
    m_bbox.Set( SFVEC3F( bbox2d.Min().x, bbox2d.Min().y, aZMin ),
                SFVEC3F( bbox2d.Max().x, bbox2d.Max().y, aZMax ) );
    m_bbox.ScaleNextUp();
    m_bbox.Scale( 1.0001f );

    m_centroid = SFVEC3F( aObject2D->GetCentroid().x, aObject2D->GetCentroid().y,
                          ( aZMax + aZMin ) * 0.5f );
}


bool LAYER_ITEM::Intersect( const RAY& aRay, HITINFO& aHitInfo ) const
{
    float tBBoxStart;
    float tBBoxEnd;

    if( !m_bbox.Intersect( aRay, &tBBoxStart, &tBBoxEnd ) )
        return false;

    if( tBBoxStart >= aHitInfo.m_tHit )
        return false;

    if( fabs( tBBoxStart - tBBoxEnd ) <= FLT_EPSILON )
        return false;

    const bool startedInside = m_bbox.Inside( aRay.m_Origin );

    if( !startedInside )
    {
        float tTop = FLT_MAX;
        float tBot = FLT_MAX;
        bool hit_top = false;
        bool hit_bot = false;

        if( (float) fabs( aRay.m_Dir.z ) > FLT_EPSILON )
        {
            tBot = ( m_bbox.Min().z - aRay.m_Origin.z ) * aRay.m_InvDir.z;
            tTop = ( m_bbox.Max().z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

            float tBBoxStartAdjusted = NextFloatUp( tBBoxStart );

            if( tBot > FLT_EPSILON )
            {
                hit_bot = tBot <= tBBoxStartAdjusted;
                tBot = NextFloatDown( tBot );
            }

            if( tTop > FLT_EPSILON )
            {
                hit_top = tTop <= tBBoxStartAdjusted;
                tTop = NextFloatDown( tTop );
            }
        }

        SFVEC2F topHitPoint2d;
        SFVEC2F botHitPoint2d;

        if( hit_top )
            topHitPoint2d = SFVEC2F( aRay.m_Origin.x + aRay.m_Dir.x * tTop,
                                     aRay.m_Origin.y + aRay.m_Dir.y * tTop );

        if( hit_bot )
            botHitPoint2d = SFVEC2F( aRay.m_Origin.x + aRay.m_Dir.x * tBot,
                                     aRay.m_Origin.y + aRay.m_Dir.y * tBot );

        if( hit_top && hit_bot )
        {
            if( tBot < tTop )
            {
                if( m_object2d->IsPointInside( botHitPoint2d ) )
                {
                    if( tBot < aHitInfo.m_tHit )
                    {
                        aHitInfo.m_tHit = tBot;
                        aHitInfo.m_HitPoint = aRay.at( tBot );
                        aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, -1.0f );
                        aHitInfo.pHitObject = this;

                        m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                        return true;
                    }

                    return false;
                }
            }
            else
            {
                if( m_object2d->IsPointInside( topHitPoint2d ) )
                {
                    if( tTop < aHitInfo.m_tHit )
                    {
                        aHitInfo.m_tHit = tTop;
                        aHitInfo.m_HitPoint = aRay.at( tTop );
                        aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, 1.0f );
                        aHitInfo.pHitObject = this;

                        m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                        return true;
                    }

                    return false;
                }
            }
        }
        else
        {
            if( hit_top )
            {
                if( tTop < tBot )
                {
                    if( m_object2d->IsPointInside( topHitPoint2d ) )
                    {
                        if( tTop < aHitInfo.m_tHit )
                        {
                            aHitInfo.m_tHit = tTop;
                            aHitInfo.m_HitPoint = aRay.at( tTop );
                            aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, 1.0f );
                            aHitInfo.pHitObject = this;

                            m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                            return true;
                        }

                        return false;
                    }
                }
            }
            else
            {
                if( hit_bot )
                {
                    if( tBot < tTop )
                    {
                        if( m_object2d->IsPointInside( botHitPoint2d ) )
                        {
                            if( tBot < aHitInfo.m_tHit )
                            {
                                aHitInfo.m_tHit = tBot;
                                aHitInfo.m_HitPoint = aRay.at( tBot );
                                aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, -1.0f );
                                aHitInfo.pHitObject = this;

                                m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                                return true;
                            }

                            return false;
                        }
                    }
                }
                else
                {
                    // At this point, the ray miss the two planes but it still
                    // hits the box. It means that the rays are "(almost)parallel"
                    // to the planes, so must calc the intersection
                }
            }
        }

        SFVEC3F boxHitPointStart = aRay.at( tBBoxStart );
        SFVEC3F boxHitPointEnd = aRay.at( tBBoxEnd );

        SFVEC2F boxHitPointStart2D( boxHitPointStart.x, boxHitPointStart.y );
        SFVEC2F boxHitPointEnd2D( boxHitPointEnd.x, boxHitPointEnd.y );

        float tOut;
        SFVEC2F outNormal;
        RAYSEG2D raySeg( boxHitPointStart2D, boxHitPointEnd2D );

        if( m_object2d->Intersect( raySeg, &tOut, &outNormal ) )
        {
            // The hitT is a hit value for the segment length 'start' - 'end',
            // so it ranges from 0.0 - 1.0. We now convert it to a 3D hit position
            // and calculate the real hitT of the ray.
            SFVEC3F hitPoint = boxHitPointStart + ( boxHitPointEnd - boxHitPointStart ) * tOut;

            const float t = glm::length( hitPoint - aRay.m_Origin );

            if( t < aHitInfo.m_tHit )
            {
                aHitInfo.m_tHit = t;
                aHitInfo.m_HitPoint = hitPoint;
                aHitInfo.pHitObject = this;

                const float zNormalDir = hit_top?1.0f:hit_bot?-1.0f:0.0f;

                if( ( outNormal.x == 0.0f ) && ( outNormal.y == 0.0f ) )
                {
                    aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, zNormalDir );
                }
                else
                {
                    // Calculate smooth bevel normal
                    float zBend = 0.0f;

                    if( hit_top || hit_bot )
                    {
                        float zDistanceToTopOrBot;

                        // Calculate the distance from hitpoint z to the Max/Min z of the layer
                        if( hit_top )
                        {
                            zDistanceToTopOrBot = ( m_bbox.Max().z - hitPoint.z );
                        }
                        else
                        {
                            zDistanceToTopOrBot = ( hitPoint.z - m_bbox.Min().z );
                        }

                        // For items that are > than g_BevelThickness3DU
                        // (eg on board vias / plated holeS) use a factor based on
                        // m_bbox.GetExtent().z
                        const float bevelThickness = glm::max(
                                g_BevelThickness3DU,
                                m_bbox.GetExtent().z
                                        * (float) ADVANCED_CFG::GetCfg().m_3DRT_BevelExtentFactor );

                        if( ( zDistanceToTopOrBot > 0.0f )
                          && ( zDistanceToTopOrBot < bevelThickness ) )
                        {
                            // Invert and Normalize the distance 0..1
                            zBend = ( bevelThickness - zDistanceToTopOrBot ) / bevelThickness;
                        }
                    }

                    const SFVEC3F normalLateral = SFVEC3F( outNormal.x, outNormal.y, 0.0f );
                    const SFVEC3F normalTopBot = SFVEC3F( 0.0f, 0.0f, zNormalDir );

                    // Interpolate between the regular lateral normal and the top/bot normal
                    aHitInfo.m_HitNormal = glm::mix( normalLateral, normalTopBot, zBend );
                }

                m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                return true;
            }
        }

        return false;
    }
    else
    {
        /// @todo Either fix the code below or get rid of it.
        // Disabled due to refraction artifacts
        // this will mostly happen inside the board body
#if 0
        // Started inside
        const SFVEC3F boxHitPointStart = aRay.at( tBBoxStart );
        const SFVEC3F boxHitPointEnd = aRay.at( tBBoxEnd );

        const SFVEC2F boxHitPointStart2D( boxHitPointStart.x, boxHitPointStart.y );

        const SFVEC2F boxHitPointEnd2D( boxHitPointEnd.x, boxHitPointEnd.y );

        if( !( m_object2d->IsPointInside( boxHitPointStart2D ) &&
               m_object2d->IsPointInside( boxHitPointEnd2D ) ) )
            return false;

        float tOut;
        SFVEC2F outNormal;
        RAYSEG2D raySeg( boxHitPointStart2D, boxHitPointEnd2D );

        if( ( m_object2d->IsPointInside( boxHitPointStart2D )
            && m_object2d->IsPointInside( boxHitPointEnd2D ) ) )
        {
            if( tBBoxEnd < aHitInfo.m_tHit )
            {
                aHitInfo.m_tHit = tBBoxEnd;
                aHitInfo.m_HitPoint = aRay.at( tBBoxEnd );
                aHitInfo.pHitObject = this;

                if( aRay.m_Dir.z > 0.0f )
                    aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, -1.0f );
                else
                    aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f,  1.0f );

                m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                return true;
            }
        }
        else
        {
            if( m_object2d->Intersect( raySeg, &tOut, &outNormal ) )
            {
                // The hitT is a hit value for the segment length 'start' - 'end',
                // so it ranges from 0.0 - 1.0. We now convert it to a 3D hit position
                // and calculate the real hitT of the ray.
                const SFVEC3F hitPoint = boxHitPointStart +
                                         ( boxHitPointEnd - boxHitPointStart ) * tOut;

                const float t = glm::length( hitPoint - aRay.m_Origin );

                if( t < aHitInfo.m_tHit )
                {
                    aHitInfo.m_tHit = t;
                    aHitInfo.m_HitPoint = hitPoint;
                    aHitInfo.m_HitNormal = SFVEC3F( outNormal.x, outNormal.y, 0.0f );
                    aHitInfo.pHitObject = this;

                    m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                    return true;
                }
            }
        }
#endif
    }

    return false;
}


bool LAYER_ITEM::IntersectP( const RAY& aRay, float aMaxDistance ) const
{
    float tBBoxStart;
    float tBBoxEnd;

    if( !m_bbox.Intersect( aRay, &tBBoxStart, &tBBoxEnd ) )
            return false;

    if( ( tBBoxStart > aMaxDistance ) || ( fabs( tBBoxStart - tBBoxEnd ) < FLT_EPSILON ) )
        return false;

    float tTop = FLT_MAX;
    float tBot = FLT_MAX;
    bool hit_top = false;
    bool hit_bot = false;

    if( (float)fabs( aRay.m_Dir.z ) > FLT_EPSILON )
    {
        tBot = ( m_bbox.Min().z - aRay.m_Origin.z ) * aRay.m_InvDir.z;
        tTop = ( m_bbox.Max().z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        const float tBBoxStartAdjusted = NextFloatUp( tBBoxStart );

        if( tBot > FLT_EPSILON )
        {
            hit_bot = tBot <= tBBoxStartAdjusted;
            tBot = NextFloatDown( tBot );
        }

        if( tTop > FLT_EPSILON )
        {
            hit_top = tTop <= tBBoxStartAdjusted;
            tTop = NextFloatDown( tTop );
        }
    }

    tBBoxStart = NextFloatDown( tBBoxStart );
    tBBoxEnd   = NextFloatUp( tBBoxEnd );

    SFVEC2F topHitPoint2d;
    SFVEC2F botHitPoint2d;

    if( hit_top )
        topHitPoint2d = SFVEC2F( aRay.m_Origin.x + aRay.m_Dir.x * tTop,
                                 aRay.m_Origin.y + aRay.m_Dir.y * tTop );

    if( hit_bot )
        botHitPoint2d = SFVEC2F( aRay.m_Origin.x + aRay.m_Dir.x * tBot,
                                 aRay.m_Origin.y + aRay.m_Dir.y * tBot );

    if( hit_top && hit_bot )
    {
        if( tBot < tTop )
        {
            if( m_object2d->IsPointInside( botHitPoint2d ) )
            {
                if( tBot < aMaxDistance )
                    return true;

                return false;
            }
        }
        else
        {
            if( m_object2d->IsPointInside( topHitPoint2d ) )
            {
                if( tTop < aMaxDistance )
                    return true;

                return false;
            }
        }
    }
    else
    {
        if( hit_top )
        {
            if( tTop < tBot )
            {
                if( m_object2d->IsPointInside( topHitPoint2d ) )
                {
                    if( tTop < aMaxDistance )
                        return true;

                    return false;
                }
            }
        }
        else
        {
            if( hit_bot )
            {
                if( tBot < tTop )
                {
                    if( m_object2d->IsPointInside( botHitPoint2d ) )
                    {
                        if( tBot < aMaxDistance )
                            return true;

                        return false;
                    }
                }
            }
            else
            {
                // At this point, the ray miss the two planes but it still
                // hits the box. It means that the rays are "(almost)parallel"
                // to the planes, so must calc the intersection
            }
        }
    }

    SFVEC3F boxHitPointStart = aRay.at( tBBoxStart );
    SFVEC3F boxHitPointEnd = aRay.at( tBBoxEnd );

    SFVEC2F boxHitPointStart2D( boxHitPointStart.x, boxHitPointStart.y );

    SFVEC2F boxHitPointEnd2D( boxHitPointEnd.x, boxHitPointEnd.y );

    float tOut;
    SFVEC2F outNormal;
    RAYSEG2D raySeg( boxHitPointStart2D, boxHitPointEnd2D );

    if( m_object2d->Intersect( raySeg, &tOut, &outNormal ) )
    {
        //if( (tOut > FLT_EPSILON) && (tOut < 1.0f) )
        {
            // The hitT is a hit value for the segment length 'start' - 'end',
            // so it ranges from 0.0 - 1.0. We now convert it to a 3D hit position
            // and calculate the real hitT of the ray.
            const SFVEC3F hitPoint = boxHitPointStart +
                                     ( boxHitPointEnd - boxHitPointStart ) * tOut;
            const float t = glm::length( hitPoint - aRay.m_Origin );

            if( ( t < aMaxDistance ) && ( t > FLT_EPSILON ) )
                return true;
        }
    }

    return false;
}


bool LAYER_ITEM::Intersects( const BBOX_3D& aBBox ) const
{
    if( !m_bbox.Intersects( aBBox ) )
        return false;

    const BBOX_2D bbox2D( SFVEC2F( aBBox.Min().x, aBBox.Min().y ),
                          SFVEC2F( aBBox.Max().x, aBBox.Max().y ) );

    return m_object2d->Intersects( bbox2D );
}


SFVEC3F LAYER_ITEM::GetDiffuseColor( const HITINFO& /* aHitInfo */ ) const
{
    return m_diffusecolor;
}
