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

/**
 * @file object_3d.h
 */

#ifndef _OBJECT_3D_H_
#define _OBJECT_3D_H_

#include "bbox_3d.h"
#include "../material.h"

class BOARD_ITEM;
class HITINFOR;

enum class OBJECT_3D_TYPE
{
    CYLINDER,
    DUMMYBLOCK,
    LAYERITEM,
    XYPLANE,
    ROUNDSEG,
    TRIANGLE,
    MAX
};


class OBJECT_3D
{
public:
    explicit OBJECT_3D( OBJECT_3D_TYPE aObjType );

    void SetBoardItem( BOARD_ITEM* aBoardItem ) { m_boardItem = aBoardItem; }
    BOARD_ITEM* GetBoardItem() const { return m_boardItem; }

    void SetMaterial( const MATERIAL* aMaterial )
    {
        m_material = aMaterial;
        m_modelTransparency = aMaterial->GetTransparency(); // Default transparency is from material
    }

    const MATERIAL* GetMaterial() const { return m_material; }
    float GetModelTransparency() const { return m_modelTransparency; }
    void SetModelTransparency( float aModelTransparency )
    {
        m_modelTransparency = aModelTransparency;
    }

    virtual SFVEC3F GetDiffuseColor( const HITINFO& aHitInfo ) const = 0;

    virtual ~OBJECT_3D() {}

    /**
     * @return true if this object intersects \a aBBox.
     */
    virtual bool Intersects( const BBOX_3D& aBBox ) const = 0;


    /**
     * @return true if the \a aRay intersects the object.
     */
    virtual bool Intersect( const RAY& aRay, HITINFO& aHitInfo ) const = 0;

    /**
     * @param aMaxDistance is the maximum distance of the test.
     * @return true if \a aRay intersects the object.
     */
    virtual bool IntersectP( const RAY& aRay, float aMaxDistance ) const = 0;

    const BBOX_3D& GetBBox() const { return m_bbox; }

    const SFVEC3F& GetCentroid() const { return m_centroid; }

protected:
    BBOX_3D m_bbox;
    SFVEC3F m_centroid;
    OBJECT_3D_TYPE m_obj_type;
    const MATERIAL* m_material;

    BOARD_ITEM* m_boardItem;

    // m_modelTransparency combines the material and model opacity
    // 0.0 full opaque, 1.0 full transparent.
    float m_modelTransparency;
};


/// Implements a class for object statistics
/// using Singleton pattern
class OBJECT_3D_STATS
{
public:
    void ResetStats()
    {
        memset( m_counter, 0, sizeof( unsigned int ) * static_cast<int>( OBJECT_3D_TYPE::MAX ) );
    }

    unsigned int GetCountOf( OBJECT_3D_TYPE aObjType ) const
    {
        return m_counter[static_cast<int>( aObjType )];
    }

    void AddOne( OBJECT_3D_TYPE aObjType )
    {
        m_counter[static_cast<int>( aObjType )]++;
    }

    // void PrintStats();

    static OBJECT_3D_STATS& Instance()
    {
        if( !s_instance )
            s_instance = new OBJECT_3D_STATS;

        return *s_instance;
    }

private:
    OBJECT_3D_STATS(){ ResetStats(); }
    OBJECT_3D_STATS( const OBJECT_3D_STATS& old );
    const OBJECT_3D_STATS& operator=( const OBJECT_3D_STATS& old );
    ~OBJECT_3D_STATS() {}

    unsigned int m_counter[static_cast<int>( OBJECT_3D_TYPE::MAX )];

    static OBJECT_3D_STATS* s_instance;
};


#endif // _OBJECT_3D_H_
