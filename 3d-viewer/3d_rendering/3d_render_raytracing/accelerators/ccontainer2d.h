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
 * @file  ccontainer2d.h
 * @brief
 */

#ifndef _CCONTAINER2D_H_
#define _CCONTAINER2D_H_

#include "../shapes2D/cobject2d.h"
#include <list>

typedef std::list<COBJECT2D *> LIST_OBJECT2D;
typedef std::list<const COBJECT2D *> CONST_LIST_OBJECT2D;


class  CGENERICCONTAINER2D
{
protected:
    CBBOX2D m_bbox;
    LIST_OBJECT2D m_objects;

public:
    explicit CGENERICCONTAINER2D( OBJECT2D_TYPE aObjType );

    virtual ~CGENERICCONTAINER2D();

    void Add( COBJECT2D *aObject )
    {
        if( aObject ) // Only add if it is a valid pointer
        {
            m_objects.push_back( aObject );
            m_bbox.Union( aObject->GetBBox() );
        }
    }

    void Clear();

    const LIST_OBJECT2D &GetList() const { return m_objects; }

    /**
     * @brief GetListObjectsIntersects - Get a list of objects that intersects a bbox
     * @param aBBox - a bbox to make the query
     * @param aOutList - A list of objects that intersects the bbox
     */
    virtual void GetListObjectsIntersects( const CBBOX2D & aBBox,
                                           CONST_LIST_OBJECT2D &aOutList ) const = 0;

private:
};


class  CCONTAINER2D : public CGENERICCONTAINER2D
{
public:
    CCONTAINER2D();

    // Imported from CGENERICCONTAINER2D
    void GetListObjectsIntersects( const CBBOX2D & aBBox,
                                   CONST_LIST_OBJECT2D &aOutList ) const override;
};


struct BVH_CONTAINER_NODE_2D
{
    CBBOX2D                 m_BBox;
    BVH_CONTAINER_NODE_2D   *m_Children[2];

    /// Store the list of objects if that node is a Leaf
    CONST_LIST_OBJECT2D     m_LeafList;
};


class  CBVHCONTAINER2D : public CGENERICCONTAINER2D
{
public:
    CBVHCONTAINER2D();
    ~CBVHCONTAINER2D();

    void BuildBVH();

private:
    bool m_isInitialized;
    std::list<BVH_CONTAINER_NODE_2D *> m_elements_to_delete;
    BVH_CONTAINER_NODE_2D   *m_Tree;

    void destroy();
    void recursiveBuild_MIDDLE_SPLIT( BVH_CONTAINER_NODE_2D *aNodeParent );
    void recursiveGetListObjectsIntersects( const BVH_CONTAINER_NODE_2D *aNode,
                                            const CBBOX2D & aBBox,
                                            CONST_LIST_OBJECT2D &aOutList ) const;

public:

    // Imported from CGENERICCONTAINER2D
    void GetListObjectsIntersects( const CBBOX2D & aBBox,
                                   CONST_LIST_OBJECT2D &aOutList ) const override;
};

#endif // _CCONTAINER2D_H_
