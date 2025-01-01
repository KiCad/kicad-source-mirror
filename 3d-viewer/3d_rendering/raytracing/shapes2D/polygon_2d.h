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
 * @file polygon_2d.h
 */

#ifndef _CPOLYGON2D_H_
#define _CPOLYGON2D_H_

#include "object_2d.h"
#include "../accelerators/container_2d.h"
#include <geometry/shape_poly_set.h>
#include <vector>


struct POLYSEGMENT
{
    SFVEC2F m_Start;
    float   m_inv_JY_minus_IY = 0.0;
    float   m_JX_minus_IX = 0.0;
};


struct SEG_NORMALS
{
    SFVEC2F m_Start;
    SFVEC2F m_End;
};


struct SEGMENT_WITH_NORMALS
{
    SFVEC2F     m_Start;
    SFVEC2F     m_Precalc_slope;
    SEG_NORMALS m_Normals;
};


typedef std::vector< POLYSEGMENT > SEGMENTS;


/**
 * List used to test ray2d intersections.
 *
 * It will be a subset of an original polygon. The normals will be passed already interpolated.
 */
typedef std::vector< SEGMENT_WITH_NORMALS > SEGMENTS_WIDTH_NORMALS;


/**
 * Handle a subset of a polygon.
 *
 * It can contain multiple closed polygons and holes and us used to test if points are inside.
 * A point will be inside the polygon if it is not inside a hole and it is inside an outer polygon.
 */
struct OUTERS_AND_HOLES
{
    std::vector<SEGMENTS> m_Outers;
    std::vector<SEGMENTS> m_Holes;
};


/**
 * Represent a sub polygon block.
 *
 * This polygon block was created from a general polygon definition that was sub divided and
 * to create blocks of polygons. This polygon class represent a sub part of that main polygon.
 * There is information for the contours (used to test the ray2d intersection) and a close
 * definition of the block polygon to test if a point is inside.
 */
class POLYGON_2D : public OBJECT_2D
{
public:
    POLYGON_2D( const SEGMENTS_WIDTH_NORMALS& aOpenSegmentList,
                const OUTERS_AND_HOLES& aOuterAndHoles, const BOARD_ITEM& aBoardItem );

    bool Overlaps( const BBOX_2D& aBBox ) const override;
    bool Intersects( const BBOX_2D& aBBox ) const override;
    bool Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const override;
    INTERSECTION_RESULT IsBBoxInside( const BBOX_2D& aBBox ) const override;
    bool IsPointInside( const SFVEC2F& aPoint ) const override;

private:
    /**
     * The outer part of the polygon.
     *
     * This list is used to test a ray intersection with the boundaries of this sub polygon.
     * It contains also the interpolated normals that are passed from the main polygon.
     */
    SEGMENTS_WIDTH_NORMALS m_open_segments;

    ///< A polygon block can have multiple polygon and holes
    OUTERS_AND_HOLES m_outers_and_holes;
};


/**
 * A dummy block defined by a 2d box size.
 *
 * If the point is inside the bounding box it will return always true. However, the
 * intersection with a ray will return always false.  This is used as a sub block
 * extracted from polygon (pcb polygon areas) and represents an area that is fully filled.
 */
class DUMMY_BLOCK_2D : public OBJECT_2D
{
public:
    DUMMY_BLOCK_2D( const SFVEC2F& aPbMin, const SFVEC2F& aPbMax, const BOARD_ITEM& aBoardItem );

    DUMMY_BLOCK_2D( const BBOX_2D& aBBox, const BOARD_ITEM& aBoardItem );

     bool Overlaps( const BBOX_2D& aBBox ) const override;
     bool Intersects( const BBOX_2D& aBBox ) const override;
     bool Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const override;
     INTERSECTION_RESULT IsBBoxInside( const BBOX_2D& aBBox ) const override;
     bool IsPointInside( const SFVEC2F& aPoint ) const override;
};


/**
 * Use a polygon in the format of the ClipperLib::Path and process it and create multiple 2d
 * objects (POLYGON_2D and DUMMY_BLOCK_2D) that can be used to represent this polygon area.
 *
 * @param aMainPath the polygon are that was converted from the pcb board
 * @param aDstContainer the destination container to put the created sub blocks
 * @param aBiuTo3dUnitsScale the rendering target 3d scale
 * @param aDivFactor a division factor (in 3Dunits) to divide the polygon plane,
 *                   0.0f will use the internal polygon segm statistics
 */
void ConvertPolygonToBlocks( const SHAPE_POLY_SET& aMainPath, CONTAINER_2D_BASE& aDstContainer,
                            float aBiuTo3dUnitsScale, float aDivFactor,
                            const BOARD_ITEM& aBoardItem, int aPolyIndex );


#endif // _CPOLYGON2D_H_
