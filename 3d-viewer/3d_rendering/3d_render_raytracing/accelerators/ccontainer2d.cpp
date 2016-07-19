/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  ccontainer2d.cpp
 * @brief
 */

#include "ccontainer2d.h"
#include <vector>
#include <boost/range/algorithm/partition.hpp>
#include <boost/range/algorithm/nth_element.hpp>
#include <wx/debug.h>


// /////////////////////////////////////////////////////////////////////////////
// CGENERICCONTAINER2
// /////////////////////////////////////////////////////////////////////////////

CGENERICCONTAINER2D::CGENERICCONTAINER2D( OBJECT2D_TYPE aObjType )
{
    m_bbox.Reset();
}


void CGENERICCONTAINER2D::Clear()
{
    m_bbox.Reset();

    for( LIST_OBJECT2D::iterator ii = m_objects.begin();
         ii != m_objects.end();
         ++ii )
    {
        delete *ii;
        *ii = NULL;
    }

    m_objects.clear();
}


CGENERICCONTAINER2D::~CGENERICCONTAINER2D()
{
    Clear();
}




// /////////////////////////////////////////////////////////////////////////////
// CCONTAINER2D
// /////////////////////////////////////////////////////////////////////////////

CCONTAINER2D::CCONTAINER2D() : CGENERICCONTAINER2D( OBJ2D_CONTAINER )
{

}
/*

bool CCONTAINER2D::Intersects( const CBBOX2D &aBBox ) const
{
    return false;
}


bool CCONTAINER2D::Overlaps( const CBBOX2D &aBBox ) const
{
    // NOT IMPLEMENTED
    return false;
}


bool CCONTAINER2D::Intersect( const RAYSEG2D &aSegRay, float *aOutT, SFVEC2F *aNormalOut ) const
{
    if( !m_bbox.Intersect( aSegRay ) )
        return false;

    bool hitted = false;

    for( LIST_OBJECT2D::const_iterator ii = m_objects.begin();
         ii != m_objects.end();
         ii++ )
    {
        const COBJECT2D *object = static_cast<const COBJECT2D *>(*ii);

        float t;
        SFVEC2F hitNormal;
        if( object->Intersect( aSegRay, &t, &hitNormal ) )
            if( (hitted == false) || (t < *aOutT ) )
            {
                hitted = true;
                *aOutT = t;
                *aNormalOut = hitNormal;
            }
    }

    return hitted;
}


INTERSECTION_RESULT CCONTAINER2D::IsBBoxInside( const CBBOX2D &aBBox ) const
{
    return INTR_MISSES;
}


bool CCONTAINER2D::IsPointInside( const SFVEC2F &aPoint ) const
{
    if( !m_bbox.Inside( aPoint ) )
        return false;

    for( LIST_OBJECT2D::const_iterator ii = m_objects.begin();
         ii != m_objects.end();
         ii++ )
    {
        const COBJECT2D *object = static_cast<const COBJECT2D *>(*ii);

        if( object->IsPointInside( aPoint ) )
            return true;
    }

    return false;
}

*/

void CCONTAINER2D::GetListObjectsIntersects( const CBBOX2D & aBBox,
                                             CONST_LIST_OBJECT2D &aOutList ) const
{
    // !TODO:
}




// /////////////////////////////////////////////////////////////////////////////
// CBVHCONTAINER2D
// /////////////////////////////////////////////////////////////////////////////

CBVHCONTAINER2D::CBVHCONTAINER2D() : CGENERICCONTAINER2D( OBJ2D_BVHCONTAINER )
{
    m_isInitialized = false;
    m_bbox.Reset();
    m_elements_to_delete.clear();
    m_Tree = NULL;
}

/*
bool CBVHCONTAINER2D::Intersects( const CBBOX2D &aBBox ) const
{
    // !TODO: implement the BVH
    return m_bbox.Intersects( aBBox );
}


bool CBVHCONTAINER2D::Overlaps( const CBBOX2D &aBBox ) const
{
    // NOT IMPLEMENTED
    return false;
}


bool CBVHCONTAINER2D::Intersect( const RAYSEG2D &aSegRay,
                                 float *aOutT, SFVEC2F *aNormalOut ) const
{
    // !TODO: implement the BVH

    if( !m_bbox.Intersect( aSegRay ) )
        return false;

    bool hitted = false;

    for( LIST_OBJECT2D::const_iterator ii = m_objects.begin();
         ii != m_objects.end();
         ii++ )
    {
        const COBJECT2D *object = static_cast<const COBJECT2D *>(*ii);

        float t;
        SFVEC2F hitNormal;
        if( object->Intersect( aSegRay, &t, &hitNormal ) )
            if( (hitted == false) || (t < *aOutT ) )
            {
                hitted = true;
                *aOutT = t;
                *aNormalOut = hitNormal;
            }
    }

    return hitted;
}


INTERSECTION_RESULT CBVHCONTAINER2D::IsBBoxInside( const CBBOX2D &aBBox ) const
{
    return INTR_MISSES;
}


bool CBVHCONTAINER2D::IsPointInside( const SFVEC2F &aPoint ) const
{
    // !TODO: implement the BVH

    if( !m_bbox.Inside( aPoint ) )
        return false;

    for( LIST_OBJECT2D::const_iterator ii = m_objects.begin();
         ii != m_objects.end();
         ii++ )
    {
        const COBJECT2D *object = static_cast<const COBJECT2D *>(*ii);

        if( object->IsPointInside( aPoint ) )
            return true;
    }

    return false;
}
*/

void CBVHCONTAINER2D::destroy()
{
    for( std::list<BVH_CONTAINER_NODE_2D *>::iterator ii = m_elements_to_delete.begin();
         ii != m_elements_to_delete.end();
         ++ii )
    {
        delete *ii;
        *ii = NULL;
    }
    m_elements_to_delete.clear();

    m_isInitialized = false;
}


CBVHCONTAINER2D::~CBVHCONTAINER2D()
{
    destroy();
}


#define BVH_CONTAINER2D_MAX_OBJ_PER_LEAF 4


void CBVHCONTAINER2D::BuildBVH()
{
    if( m_isInitialized )
        destroy();

    if( m_objects.empty() )
    {
        return;
    }

    m_isInitialized = true;
    m_Tree = new BVH_CONTAINER_NODE_2D;

    m_elements_to_delete.push_back( m_Tree );
    m_Tree->m_BBox = m_bbox;

    for( LIST_OBJECT2D::const_iterator ii = m_objects.begin();
         ii != m_objects.end();
         ++ii )
    {
        m_Tree->m_LeafList.push_back( static_cast<const COBJECT2D *>(*ii) );
    }

    recursiveBuild_MIDDLE_SPLIT( m_Tree );
}


// Based on a blog post by VADIM KRAVCENKO
// http://www.vadimkravcenko.com/bvh-tree-building
// Implements:

// "Split in the middle of the longest Axis"
// "Creates a binary tree with Top-Down approach.
//  Fastest BVH building, but least [speed] accuracy."

static bool sortByCentroid_X( const COBJECT2D *a, const COBJECT2D *b )
{
    return a->GetCentroid()[0] < b->GetCentroid()[0];
}

static bool sortByCentroid_Y( const COBJECT2D *a, const COBJECT2D *b )
{
    return a->GetCentroid()[0] < b->GetCentroid()[0];
}

static bool sortByCentroid_Z( const COBJECT2D *a, const COBJECT2D *b )
{
    return a->GetCentroid()[0] < b->GetCentroid()[0];
}

void CBVHCONTAINER2D::recursiveBuild_MIDDLE_SPLIT( BVH_CONTAINER_NODE_2D *aNodeParent )
{
    wxASSERT( aNodeParent != NULL );
    wxASSERT( aNodeParent->m_BBox.IsInitialized() == true );
    wxASSERT( aNodeParent->m_LeafList.size() > 0 );

    if( aNodeParent->m_LeafList.size() > BVH_CONTAINER2D_MAX_OBJ_PER_LEAF )
    {
        // Create Leaf Nodes
        BVH_CONTAINER_NODE_2D *leftNode  = new BVH_CONTAINER_NODE_2D;
        BVH_CONTAINER_NODE_2D *rightNode = new BVH_CONTAINER_NODE_2D;
        m_elements_to_delete.push_back( leftNode );
        m_elements_to_delete.push_back( rightNode );

        leftNode->m_BBox.Reset();
        rightNode->m_BBox.Reset();
        leftNode->m_LeafList.clear();
        rightNode->m_LeafList.clear();

        // Decide wich axis to split
        const unsigned int axis_to_split = aNodeParent->m_BBox.MaxDimension();

        // Divide the objects
        switch( axis_to_split )
        {
            case 0: aNodeParent->m_LeafList.sort( sortByCentroid_X );
            case 1: aNodeParent->m_LeafList.sort( sortByCentroid_Y );
            case 2: aNodeParent->m_LeafList.sort( sortByCentroid_Z );
        }

        unsigned int i = 0;

        for( CONST_LIST_OBJECT2D::const_iterator ii = aNodeParent->m_LeafList.begin();
             ii != aNodeParent->m_LeafList.end();
             ++ii )
        {
            const COBJECT2D *object = static_cast<const COBJECT2D *>(*ii);

            if( i < (aNodeParent->m_LeafList.size() / 2 ) )
            {
                leftNode->m_BBox.Union( object->GetBBox() );
                leftNode->m_LeafList.push_back( object );
            }
            else
            {
                rightNode->m_BBox.Union( object->GetBBox() );
                rightNode->m_LeafList.push_back( object );
            }

            i++;
        }

        wxASSERT( leftNode->m_LeafList.size() > 0 );
        wxASSERT( rightNode->m_LeafList.size() > 0 );
        wxASSERT( ( leftNode->m_LeafList.size() + rightNode->m_LeafList.size() ) ==
                  aNodeParent->m_LeafList.size() );

        aNodeParent->m_Children[0] = leftNode;
        aNodeParent->m_Children[1] = rightNode;
        aNodeParent->m_LeafList.clear();

        recursiveBuild_MIDDLE_SPLIT( leftNode );
        recursiveBuild_MIDDLE_SPLIT( rightNode );
    }
    else
    {
        // It is a Leaf
        aNodeParent->m_Children[0] = NULL;
        aNodeParent->m_Children[1] = NULL;
    }
}


void CBVHCONTAINER2D::GetListObjectsIntersects( const CBBOX2D &aBBox,
                                                CONST_LIST_OBJECT2D &aOutList ) const
{
    wxASSERT( aBBox.IsInitialized() == true );
    wxASSERT( m_isInitialized == true );

    aOutList.clear();

    if( m_Tree )
        recursiveGetListObjectsIntersects( m_Tree, aBBox, aOutList );
}


void CBVHCONTAINER2D::recursiveGetListObjectsIntersects( const BVH_CONTAINER_NODE_2D *aNode,
                                                         const CBBOX2D & aBBox,
                                                         CONST_LIST_OBJECT2D &aOutList ) const
{
    wxASSERT( aNode != NULL );
    wxASSERT( aBBox.IsInitialized() == true );

    if( aNode->m_BBox.Intersects( aBBox ) )
    {
        if( !aNode->m_LeafList.empty() )
        {
            wxASSERT( aNode->m_Children[0] == NULL );
            wxASSERT( aNode->m_Children[1] == NULL );

            // Leaf
            for( CONST_LIST_OBJECT2D::const_iterator ii = aNode->m_LeafList.begin();
                 ii != aNode->m_LeafList.end();
                 ++ii )
            {
                const COBJECT2D *obj = static_cast<const COBJECT2D *>(*ii);

                if( obj->Intersects( aBBox ) )
                    aOutList.push_back( obj );
            }
        }
        else
        {
            wxASSERT( aNode->m_Children[0] != NULL );
            wxASSERT( aNode->m_Children[1] != NULL );

            // Node
            recursiveGetListObjectsIntersects( aNode->m_Children[0], aBBox, aOutList );
            recursiveGetListObjectsIntersects( aNode->m_Children[1], aBBox, aOutList );
        }
    }
}
