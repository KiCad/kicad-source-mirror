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
 * @file  cobject.h
 * @brief
 */

#ifndef _COBJECT_H_
#define _COBJECT_H_

#include "cbbox.h"
#include "../hitinfo.h"
#include "../cmaterial.h"


enum OBJECT3D_TYPE
{
    OBJ3D_CYLINDER,
    OBJ3D_DUMMYBLOCK,
    OBJ3D_LAYERITEM,
    OBJ3D_XYPLANE,
    OBJ3D_ROUNDSEG,
    OBJ3D_TRIANGLE,
    OBJ3D_MAX
};


class  COBJECT
{
protected:
    CBBOX   m_bbox;
    SFVEC3F m_centroid;
    OBJECT3D_TYPE m_obj_type;
    const CMATERIAL *m_material;

public:

    explicit COBJECT( OBJECT3D_TYPE aObjType );

    void SetMaterial( const CMATERIAL *aMaterial ) { m_material = aMaterial; }
    const CMATERIAL *GetMaterial() const { return m_material; }

    virtual SFVEC3F GetDiffuseColor( const HITINFO &aHitInfo ) const = 0;

    virtual ~COBJECT() {}

    /** Function Intersects
     * @brief Intersects - a.Intersects(b) ⇔ !a.Disjoint(b) ⇔ !(a ∩ b = ∅)
     * It intersects if the result intersection is not null
     * @param aBBox
     * @return
     */
    virtual bool Intersects( const CBBOX &aBBox ) const = 0;


    /** Functions Intersect
     * @brief Intersect
     * @param aRay
     * @param aHitInfo
     * @return TRUE if the aRay intersects the object
     */
    virtual bool Intersect( const RAY &aRay, HITINFO &aHitInfo ) const = 0;

    /** Functions Intersect for shadow test
     * @brief Intersect
     * @param aRay
     * @param aDistance - max distance of the test
     * @return TRUE if the aRay intersects the object
     */
    virtual bool IntersectP( const RAY &aRay, float aMaxDistance ) const = 0;

    const CBBOX &GetBBox() const { return m_bbox; }

    const SFVEC3F &GetCentroid() const { return m_centroid; }
};


/// Implements a class for object statistics
/// using Singleton pattern
class COBJECT3D_STATS
{
public:

    void ResetStats() { memset( m_counter,
                                0,
                                sizeof( unsigned int ) * OBJ3D_MAX ); }

    unsigned int GetCountOf( OBJECT3D_TYPE aObjType ) const { return m_counter[aObjType]; }

    void AddOne( OBJECT3D_TYPE aObjType ) { m_counter[aObjType]++; }

    void PrintStats();

    static COBJECT3D_STATS &Instance()
    {
        if( !s_instance )
            s_instance = new COBJECT3D_STATS;

        return *s_instance;
    }

private:
    COBJECT3D_STATS(){ ResetStats(); }
    COBJECT3D_STATS( const COBJECT3D_STATS &old );
    const COBJECT3D_STATS &operator=( const COBJECT3D_STATS &old );
    ~COBJECT3D_STATS(){}

private:
    unsigned int m_counter[OBJ3D_MAX];

    static COBJECT3D_STATS *s_instance;
};


#endif // _COBJECT_H_
