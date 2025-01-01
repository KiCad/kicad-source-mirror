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
 * @file layer_item_2d.cpp
 */

#include "layer_item_2d.h"
#include "../ray.h"
#include "3d_fastmath.h"
#include <wx/debug.h>


LAYER_ITEM_2D::LAYER_ITEM_2D( const OBJECT_2D* aObjectA, std::vector<const OBJECT_2D*>* aObjectB,
                              const OBJECT_2D* aObjectC, const BOARD_ITEM& aBoardItem ) :
        OBJECT_2D( OBJECT_2D_TYPE::CSG, aBoardItem ),
        m_objectA( aObjectA ),
        m_objectB( aObjectB ),
        m_objectC( aObjectC )
{
    wxASSERT( aObjectA );

    m_bbox.Reset();
    m_bbox.Set( aObjectA->GetBBox() );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();

    wxASSERT( m_bbox.IsInitialized() );
}


LAYER_ITEM_2D::~LAYER_ITEM_2D()
{
    if( ( (void*) m_objectB != CSGITEM_EMPTY ) && ( (void*) m_objectB != CSGITEM_FULL ) )
    {
        delete m_objectB;
        m_objectB = nullptr;
    }
}


bool LAYER_ITEM_2D::Intersects( const BBOX_2D& aBBox ) const
{
    return m_bbox.Intersects( aBBox );
    // !TODO: improve this implementation
    //return false;
}


bool LAYER_ITEM_2D::Overlaps( const BBOX_2D& aBBox ) const
{
    // NOT IMPLEMENTED, why?
    return false;
}


// Based on ideas and implementation by Nick Chapman
// http://homepages.paradise.net.nz/nickamy/raytracer/raytracer.htm
bool LAYER_ITEM_2D::Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const
{
    if( m_objectA->GetObjectType() == OBJECT_2D_TYPE::DUMMYBLOCK )
        return false;

    SFVEC2F  currentRayPos = aSegRay.m_Start;
    SFVEC2F  currentNormal;
    RAYSEG2D currentRay = aSegRay;

    if( !m_objectA->IsPointInside( aSegRay.m_Start ) )
    {
        //move ray point to start of main object
        float tmpRayDist;

        if( !m_objectA->Intersect( aSegRay, &tmpRayDist, &currentNormal ) )
            return false;

        currentRayPos = aSegRay.atNormalized( tmpRayDist + 0.003f );

        currentRay = RAYSEG2D( currentRayPos, aSegRay.m_End );
    }

    // move through the union of subtracted regions
    if( m_objectB )
    {
        for( unsigned int l = 0; l < ( m_objectB->size() * 2 ); ++l )
        {
            bool wasCrossedSubVol = false;

            //check against all subbed objects
            for( unsigned int i = 0; i < m_objectB->size(); ++i )
            {
                if( ( (const OBJECT_2D*) ( *m_objectB )[i] )->IsPointInside( currentRayPos ) )
                {
                    // ray point is inside a subtracted region,  so move it to the end of the
                    // subtracted region
                    float   hitDist;
                    SFVEC2F tmpNormal;

                    if( !( (const OBJECT_2D*) ( *m_objectB )[i] )
                                 ->Intersect( currentRay, &hitDist, &tmpNormal ) )
                        return false; // ray hit main object but did not leave subtracted volume

                    wxASSERT( hitDist <= 1.0f );

                    if( hitDist > FLT_EPSILON )
                    {
                        wasCrossedSubVol = true;

                        currentRayPos =
                                currentRay.atNormalized( glm::min( hitDist + 0.0001f, 1.0f ) );

                        currentRay = RAYSEG2D( currentRayPos, aSegRay.m_End );

                        currentNormal = tmpNormal * -1.0f;
                    }
                }
            }

            if( !wasCrossedSubVol )
                break;
        }
    }

    if( aNormalOut )
        *aNormalOut = currentNormal;

    if( aOutT )
        *aOutT = glm::min(
                glm::max( glm::length( currentRayPos - aSegRay.m_Start ) / aSegRay.m_Length, 0.0f ),
                1.0f );

    return true;
}


INTERSECTION_RESULT LAYER_ITEM_2D::IsBBoxInside( const BBOX_2D& aBBox ) const
{

    // !TODO:

    return INTERSECTION_RESULT::MISSES;
}


bool LAYER_ITEM_2D::IsPointInside( const SFVEC2F& aPoint ) const
{
    // Perform the operation (A - B) /\ C
    if( m_objectA->IsPointInside( aPoint ) )
    {

        if( m_objectB != CSGITEM_EMPTY )
        {
            for( unsigned int i = 0; i < m_objectB->size(); i++ )
            {
                if( ( *m_objectB )[i]->IsPointInside( aPoint ) )
                    return false;
            }
        }

        // !TODO: not yet implemented
        //if( m_objectC && m_objectC != CSGITEM_FULL )
        //    return m_objectC->IsPointInside( aPoint );

        return true;
    }

    return false;
}
