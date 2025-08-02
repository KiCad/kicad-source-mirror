/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
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

#pragma once

#include "../shapes2D/object_2d.h"
#include <list>
#include <mutex>

struct RAYSEG2D;

typedef std::list<OBJECT_2D*> LIST_OBJECT2D;
typedef std::list<const OBJECT_2D*> CONST_LIST_OBJECT2D;


class CONTAINER_2D_BASE
{
public:
    explicit CONTAINER_2D_BASE( OBJECT_2D_TYPE aObjType );

    virtual ~CONTAINER_2D_BASE();

    void Add( OBJECT_2D* aObject )
    {
        if( aObject )
        {
            std::lock_guard<std::mutex> lock( m_lock );
            m_objects.push_back( aObject );
            m_bbox.Union( aObject->GetBBox() );
        }
    }

    const BBOX_2D& GetBBox() const
    {
        return m_bbox;
    }

    virtual void Clear();

    const LIST_OBJECT2D& GetList() const { return m_objects; }

    /**
     * Get a list of objects that intersects a bounding box.
     *
     * @param aBBox The bounding box to test.
     * @param aOutList The list of objects that intersects the bounding box.
     */
    virtual void GetIntersectingObjects( const BBOX_2D& aBBox, CONST_LIST_OBJECT2D& aOutList ) const = 0;

    /**
     * Intersect and check if a segment ray hits a object or is inside it.
     *
     * @param aSegRay The segment to intersect with objects.
     * @return true if it hits any of the objects or is inside any object.
     */
    virtual bool IntersectAny( const RAYSEG2D& aSegRay ) const = 0;

protected:
    BBOX_2D       m_bbox;
    LIST_OBJECT2D m_objects;

private:
    std::mutex    m_lock;
};


class CONTAINER_2D : public CONTAINER_2D_BASE
{
public:
    CONTAINER_2D();

    void GetIntersectingObjects( const BBOX_2D& aBBox, CONST_LIST_OBJECT2D& aOutList ) const override;

    bool IntersectAny( const RAYSEG2D& aSegRay ) const override;
};


struct BVH_CONTAINER_NODE_2D
{
    BBOX_2D                 m_BBox;
    BVH_CONTAINER_NODE_2D*  m_Children[2];

    /// Store the list of objects if that node is a Leaf
    CONST_LIST_OBJECT2D     m_LeafList;
};


class BVH_CONTAINER_2D : public CONTAINER_2D_BASE
{
public:
    BVH_CONTAINER_2D();
    ~BVH_CONTAINER_2D();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    BVH_CONTAINER_2D( const BVH_CONTAINER_2D& ) = delete;
    BVH_CONTAINER_2D& operator=( const BVH_CONTAINER_2D& ) = delete;

    void BuildBVH();

    void Clear() override;

    void GetIntersectingObjects( const BBOX_2D& aBBox, CONST_LIST_OBJECT2D& aOutList ) const override;

    bool IntersectAny( const RAYSEG2D& aSegRay ) const override;

private:
    void destroy();
    void recursiveBuild_MIDDLE_SPLIT( BVH_CONTAINER_NODE_2D* aNodeParent );
    void recursiveGetListObjectsIntersects( const BVH_CONTAINER_NODE_2D* aNode, const BBOX_2D& aBBox,
                                            CONST_LIST_OBJECT2D& aOutList ) const;
    bool recursiveIntersectAny( const BVH_CONTAINER_NODE_2D* aNode, const RAYSEG2D& aSegRay ) const;

private:
    bool                              m_isInitialized;
    std::list<BVH_CONTAINER_NODE_2D*> m_elementsToDelete;
    BVH_CONTAINER_NODE_2D*            m_tree;

};

