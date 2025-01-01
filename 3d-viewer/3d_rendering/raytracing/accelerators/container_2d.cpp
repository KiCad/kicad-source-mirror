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
 * @file container_2d.cpp
 */

#include "container_2d.h"
#include "../ray.h"
#include <boost/range/algorithm/partition.hpp>
#include <boost/range/algorithm/nth_element.hpp>
#include <wx/debug.h>


CONTAINER_2D_BASE::CONTAINER_2D_BASE( OBJECT_2D_TYPE aObjType )
{
    m_bbox.Reset();
}


void CONTAINER_2D_BASE::Clear()
{
    std::lock_guard<std::mutex> lock( m_lock );
    m_bbox.Reset();

    for( LIST_OBJECT2D::iterator ii = m_objects.begin(); ii != m_objects.end(); ++ii )
    {
        delete *ii;
    }

    m_objects.clear();
}


CONTAINER_2D_BASE::~CONTAINER_2D_BASE()
{
    Clear();
}


CONTAINER_2D::CONTAINER_2D() : CONTAINER_2D_BASE( OBJECT_2D_TYPE::CONTAINER )
{

}


void CONTAINER_2D::GetIntersectingObjects( const BBOX_2D& aBBox,
                                           CONST_LIST_OBJECT2D& aOutList ) const
{
    /// @todo Determine what to do with this code.
}


bool CONTAINER_2D::IntersectAny( const RAYSEG2D& aSegRay ) const
{
    /// @todo Determine what what needs done because someone wrote TODO here.
    return false;
}


BVH_CONTAINER_2D::BVH_CONTAINER_2D() : CONTAINER_2D_BASE( OBJECT_2D_TYPE::BVHCONTAINER )
{
    m_isInitialized = false;
    m_bbox.Reset();
    m_elementsToDelete.clear();
    m_tree = nullptr;
}


void BVH_CONTAINER_2D::Clear()
{
    CONTAINER_2D_BASE::Clear();
    destroy();
}


void BVH_CONTAINER_2D::destroy()
{
    for( std::list<BVH_CONTAINER_NODE_2D*>::iterator ii = m_elementsToDelete.begin();
         ii != m_elementsToDelete.end();
         ++ii )
    {
        delete *ii;
    }

    m_elementsToDelete.clear();
    m_tree = nullptr;
    m_isInitialized = false;
}


BVH_CONTAINER_2D::~BVH_CONTAINER_2D()
{
    destroy();
}


#define BVH_CONTAINER2D_MAX_OBJ_PER_LEAF 4


void BVH_CONTAINER_2D::BuildBVH()
{
    if( m_isInitialized )
        destroy();

    m_isInitialized = true;

    if( m_objects.empty() )
    {
        return;
    }

    m_tree = new BVH_CONTAINER_NODE_2D;

    m_elementsToDelete.push_back( m_tree );
    m_tree->m_BBox = m_bbox;

    for( LIST_OBJECT2D::const_iterator ii = m_objects.begin(); ii != m_objects.end(); ++ii )
    {
        m_tree->m_LeafList.push_back( static_cast<const OBJECT_2D*>( *ii ) );
    }

    recursiveBuild_MIDDLE_SPLIT( m_tree );
}


// Based on a blog post by VADIM KRAVCENKO
// http://www.vadimkravcenko.com/bvh-tree-building
// Implements:

// "Split in the middle of the longest Axis"
// "Creates a binary tree with Top-Down approach.
//  Fastest BVH building, but least [speed] accuracy."
static bool sortByCentroidX( const OBJECT_2D* a, const OBJECT_2D* b )
{
    return a->GetCentroid()[0] < b->GetCentroid()[0];
}


static bool sortByCentroidY( const OBJECT_2D* a, const OBJECT_2D* b )
{
    return a->GetCentroid()[0] < b->GetCentroid()[0];
}


static bool sortByCentroidZ( const OBJECT_2D* a, const OBJECT_2D* b )
{
    return a->GetCentroid()[0] < b->GetCentroid()[0];
}


void BVH_CONTAINER_2D::recursiveBuild_MIDDLE_SPLIT( BVH_CONTAINER_NODE_2D* aNodeParent )
{
    wxASSERT( aNodeParent != nullptr );
    wxASSERT( aNodeParent->m_BBox.IsInitialized() == true );
    wxASSERT( aNodeParent->m_LeafList.size() > 0 );

    if( aNodeParent->m_LeafList.size() > BVH_CONTAINER2D_MAX_OBJ_PER_LEAF )
    {
        // Create Leaf Nodes
        BVH_CONTAINER_NODE_2D* leftNode  = new BVH_CONTAINER_NODE_2D;
        BVH_CONTAINER_NODE_2D* rightNode = new BVH_CONTAINER_NODE_2D;
        m_elementsToDelete.push_back( leftNode );
        m_elementsToDelete.push_back( rightNode );

        leftNode->m_BBox.Reset();
        rightNode->m_BBox.Reset();
        leftNode->m_LeafList.clear();
        rightNode->m_LeafList.clear();

        // Decide which axis to split
        const unsigned int axis_to_split = aNodeParent->m_BBox.MaxDimension();

        // Divide the objects
        switch( axis_to_split )
        {
            case 0: aNodeParent->m_LeafList.sort( sortByCentroidX ); break;
            case 1: aNodeParent->m_LeafList.sort( sortByCentroidY ); break;
            case 2: aNodeParent->m_LeafList.sort( sortByCentroidZ ); break;
        }

        unsigned int i = 0;

        for( CONST_LIST_OBJECT2D::const_iterator ii = aNodeParent->m_LeafList.begin();
             ii != aNodeParent->m_LeafList.end();
             ++ii )
        {
            const OBJECT_2D* object = static_cast<const OBJECT_2D*>( *ii );

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

        wxASSERT( aNodeParent->m_LeafList.size() == 0 );
    }
    else
    {
        // It is a Leaf
        aNodeParent->m_Children[0] = nullptr;
        aNodeParent->m_Children[1] = nullptr;
    }

    wxASSERT( aNodeParent != nullptr );
    wxASSERT( aNodeParent->m_BBox.IsInitialized() == true );
}


bool BVH_CONTAINER_2D::IntersectAny( const RAYSEG2D& aSegRay ) const
{
    wxASSERT( m_isInitialized == true );

    if( m_tree )
        return recursiveIntersectAny( m_tree, aSegRay );

    return false;
}


bool BVH_CONTAINER_2D::recursiveIntersectAny( const BVH_CONTAINER_NODE_2D* aNode,
                                              const RAYSEG2D& aSegRay ) const
{
    wxASSERT( aNode != nullptr );

    if( aNode->m_BBox.Inside( aSegRay.m_Start ) || aNode->m_BBox.Inside( aSegRay.m_End ) ||
        aNode->m_BBox.Intersect( aSegRay ) )
    {
        if( !aNode->m_LeafList.empty() )
        {
            wxASSERT( aNode->m_Children[0] == nullptr );
            wxASSERT( aNode->m_Children[1] == nullptr );

            // Leaf
            for( const OBJECT_2D* obj : aNode->m_LeafList )
            {
                if( obj->IsPointInside( aSegRay.m_Start ) ||
                    obj->IsPointInside( aSegRay.m_End ) ||
                    obj->Intersect( aSegRay, nullptr, nullptr ) )
                    return true;
            }
        }
        else
        {
            wxASSERT( aNode->m_Children[0] != nullptr );
            wxASSERT( aNode->m_Children[1] != nullptr );

            // Node
            if( recursiveIntersectAny( aNode->m_Children[0], aSegRay ) )
                return true;

            if( recursiveIntersectAny( aNode->m_Children[1], aSegRay ) )
                return true;
        }
    }

    return false;
}


void BVH_CONTAINER_2D::GetIntersectingObjects( const BBOX_2D& aBBox,
                                               CONST_LIST_OBJECT2D& aOutList ) const
{
    wxASSERT( aBBox.IsInitialized() == true );
    wxASSERT( m_isInitialized == true );

    aOutList.clear();

    if( m_tree )
        recursiveGetListObjectsIntersects( m_tree, aBBox, aOutList );
}


void BVH_CONTAINER_2D::recursiveGetListObjectsIntersects( const BVH_CONTAINER_NODE_2D* aNode,
                                                          const BBOX_2D& aBBox,
                                                          CONST_LIST_OBJECT2D& aOutList ) const
{
    wxASSERT( aNode != nullptr );
    wxASSERT( aBBox.IsInitialized() == true );

    if( aNode->m_BBox.Intersects( aBBox ) )
    {
        if( !aNode->m_LeafList.empty() )
        {
            wxASSERT( aNode->m_Children[0] == nullptr );
            wxASSERT( aNode->m_Children[1] == nullptr );

            // Leaf
            for( CONST_LIST_OBJECT2D::const_iterator ii = aNode->m_LeafList.begin();
                 ii != aNode->m_LeafList.end();
                 ++ii )
            {
                const OBJECT_2D* obj = static_cast<const OBJECT_2D*>( *ii );

                if( obj->Intersects( aBBox ) )
                    aOutList.push_back( obj );
            }
        }
        else
        {
            wxASSERT( aNode->m_Children[0] != nullptr );
            wxASSERT( aNode->m_Children[1] != nullptr );

            // Node
            recursiveGetListObjectsIntersects( aNode->m_Children[0], aBBox, aOutList );
            recursiveGetListObjectsIntersects( aNode->m_Children[1], aBBox, aOutList );
        }
    }
}
