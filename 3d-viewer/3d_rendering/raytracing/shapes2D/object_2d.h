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
 * @file object_2d.h
 */

#ifndef _OBJECT_2D_H_
#define _OBJECT_2D_H_

#include "bbox_2d.h"

class BOARD_ITEM;

enum class INTERSECTION_RESULT
{
    MISSES,
    INTERSECTS,
    FULL_INSIDE
};


enum class OBJECT_2D_TYPE
{
    FILLED_CIRCLE,
    CSG,
    POLYGON,
    DUMMYBLOCK,
    POLYGON4PT,
    RING,
    ROUNDSEG,
    TRIANGLE,
    CONTAINER,
    BVHCONTAINER,
    MAX
};


class OBJECT_2D
{
public:
    OBJECT_2D( OBJECT_2D_TYPE aObjType, const BOARD_ITEM& aBoardItem );
    virtual ~OBJECT_2D() {}

    const BOARD_ITEM& GetBoardItem() const { return m_boardItem; }

    /**
     * Test if the box overlaps the object.
     *
     * Conformance
     * Implements the Overlaps function from the OGC Simple Feature Specification at
     * http://www.opengeospatial.org/standards/sfa.
     * a.Overlaps(b) ⇔ ( dim(I(a)) = dim(I(b)) = dim(I(a) ∩ I(b))) ∧ (a ∩ b ≠ a) ∧ (a ∩ b ≠ b)
     * It means that the result dimension of an overlap is the same dimensions
     * of the bounding box (so the overlap cannot be a point or a line) and one
     * of the boxes cannot full contain the other box.
     *
     * @param aBBox is the bounding box to test.
     * @return true if the BBox intersects the object or is inside it.
     */
    virtual bool Overlaps( const BBOX_2D& aBBox ) const = 0;

    /**
     * a.Intersects(b) ⇔ !a.Disjoint(b) ⇔ !(a ∩ b = ∅)
     */
    virtual bool Intersects( const BBOX_2D& aBBox ) const = 0;

    /**
     * @param aOutT a value between 0.0 and 1.0 in relation to the time of the hit of the segment.
     */
    virtual bool Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const = 0;

    /**
     * Test this object if it's completely outside, intersects, or is completely inside \a aBBox.
     *
     * @return INTERSECTION_RESULT
     */
    virtual INTERSECTION_RESULT IsBBoxInside( const BBOX_2D& aBBox ) const = 0;

    virtual bool IsPointInside( const SFVEC2F& aPoint ) const = 0;

    const BBOX_2D& GetBBox() const { return m_bbox; }

    const SFVEC2F& GetCentroid() const { return m_centroid; }

    OBJECT_2D_TYPE GetObjectType() const { return m_obj_type; }

protected:
    BBOX_2D          m_bbox;
    SFVEC2F          m_centroid;
    OBJECT_2D_TYPE   m_obj_type;

    const BOARD_ITEM& m_boardItem;
};



class OBJECT_2D_STATS
{
public:
    void ResetStats()
    {
        memset( m_counter, 0, sizeof( unsigned int ) * static_cast<int>( OBJECT_2D_TYPE::MAX ) );
    }

    unsigned int GetCountOf( OBJECT_2D_TYPE aObjType ) const
    {
        return m_counter[static_cast<int>( aObjType )];
    }

    void AddOne( OBJECT_2D_TYPE aObjType )
    {
        m_counter[static_cast<int>( aObjType )]++;
    }

    static OBJECT_2D_STATS& Instance()
    {
        if( !s_instance )
            s_instance = new OBJECT_2D_STATS;

        return *s_instance;
    }

private:
    OBJECT_2D_STATS(){ ResetStats(); }
    OBJECT_2D_STATS( const OBJECT_2D_STATS& old );
    const OBJECT_2D_STATS& operator=( const OBJECT_2D_STATS& old );
    ~OBJECT_2D_STATS(){}

    unsigned int m_counter[static_cast<int>( OBJECT_2D_TYPE::MAX )];

    static OBJECT_2D_STATS* s_instance;
};

#endif // _OBJECT_2D_H_
