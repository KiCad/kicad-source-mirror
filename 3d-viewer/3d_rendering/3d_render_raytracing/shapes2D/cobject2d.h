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
 * @file  cobject2d.h
 * @brief
 */

#ifndef _COBJECT2D_H_
#define _COBJECT2D_H_

#include "cbbox2d.h"
#include <string.h>

#include <class_board_item.h>

enum INTERSECTION_RESULT
{
    INTR_MISSES,
    INTR_INTERSECTS,
    INTR_FULL_INSIDE
};


enum OBJECT2D_TYPE
{
    OBJ2D_FILLED_CIRCLE,
    OBJ2D_CSG,
    OBJ2D_POLYGON,
    OBJ2D_DUMMYBLOCK,
    OBJ2D_POLYGON4PT,
    OBJ2D_RING,
    OBJ2D_ROUNDSEG,
    OBJ2D_TRIANGLE,
    OBJ2D_CONTAINER,
    OBJ2D_BVHCONTAINER,
    OBJ2D_MAX
};


class  COBJECT2D
{
protected:
    CBBOX2D          m_bbox;
    SFVEC2F          m_centroid;
    OBJECT2D_TYPE    m_obj_type;

    const BOARD_ITEM &m_boardItem;
public:

    COBJECT2D( OBJECT2D_TYPE aObjType, const BOARD_ITEM &aBoardItem );
    virtual ~COBJECT2D() {}

    const BOARD_ITEM &GetBoardItem() const { return m_boardItem; }

    /** Function Overlaps
     * @brief Test if the box overlaps the object
     * Conformance
     * The function overlaps implements function Overlaps from the OGC
     * Simple Feature Specification.
     * http://www.opengeospatial.org/standards/sfa
     * a.Overlaps(b) ⇔ ( dim(I(a)) = dim(I(b)) = dim(I(a) ∩ I(b))) ∧ (a ∩ b ≠ a) ∧ (a ∩ b ≠ b)
     * It means that the result dimension of an overlap is the same dimentions
     * of the bounding box (so the overlap cannot be a point or a line) and one
     * of the boxes cannot full contain the other box.
     * @param aBBox - The bounding box to test
     * @return true if the BBox intersects the object or is inside it
     */
    virtual bool Overlaps( const CBBOX2D &aBBox ) const = 0;

    /** Function Intersects
     * @brief Intersects - a.Intersects(b) ⇔ !a.Disjoint(b) ⇔ !(a ∩ b = ∅)
     * It intersects if the result intersection is not null
     * @param aBBox
     * @return
     */
    virtual bool Intersects( const CBBOX2D &aBBox ) const = 0;

    /** Function Intersect
     * @brief Intersect
     * @param aSegRay
     * @param aOutT a value between 0.0 and 1.0 in relation to the time of the
     * hit of the segment
     * @param aNormalOut
     * @return
     */
    virtual bool Intersect( const RAYSEG2D &aSegRay,
                            float *aOutT,
                            SFVEC2F *aNormalOut ) const = 0;

    /** Function IsBBoxInside
     * @brief Tests if the bouding is out, intersects or is complety inside
     * @param aBBox - The bounding box to test
     * @return INTERSECTION_RESULT
     */
    virtual INTERSECTION_RESULT IsBBoxInside( const CBBOX2D &aBBox ) const = 0;

    virtual bool IsPointInside( const SFVEC2F &aPoint ) const = 0;

    const CBBOX2D &GetBBox() const { return m_bbox; }

    const SFVEC2F &GetCentroid() const { return m_centroid; }

    OBJECT2D_TYPE GetObjectType() const { return m_obj_type; }
};



/// Implements a class for object statistics
/// using Singleton pattern
class COBJECT2D_STATS
{
public:
    void ResetStats() { memset( m_counter, 0, sizeof( unsigned int ) * OBJ2D_MAX ); }

    unsigned int GetCountOf( OBJECT2D_TYPE aObjType ) const
    {
        return m_counter[aObjType];
    }

    void AddOne( OBJECT2D_TYPE aObjType ) { m_counter[aObjType]++; }

    void PrintStats();

    static COBJECT2D_STATS &Instance()
    {
        if( !s_instance )
            s_instance = new COBJECT2D_STATS;

        return *s_instance;
    }

private:
    COBJECT2D_STATS(){ ResetStats(); }
    COBJECT2D_STATS( const COBJECT2D_STATS &old );
    const COBJECT2D_STATS &operator=( const COBJECT2D_STATS &old );
    ~COBJECT2D_STATS(){}

private:
    unsigned int m_counter[OBJ2D_MAX];

    static COBJECT2D_STATS *s_instance;
};

#endif // _COBJECT2D_H_
