/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  cpolygon2d.h
 * @brief
 */

#ifndef _CPOLYGON2D_H_
#define _CPOLYGON2D_H_

#include "cobject2d.h"
#include "../accelerators/ccontainer2d.h"
#include <geometry/shape_poly_set.h>
#include <vector>


typedef struct
{
    SFVEC2F m_Start;
    float   m_inv_JY_minus_IY;
    float   m_JX_minus_IX;
}POLYSEGMENT;


typedef struct
{
    SFVEC2F m_Start;
    SFVEC2F m_End;
}SEG_NORMALS;


typedef struct
{
    SFVEC2F     m_Start;
    SFVEC2F     m_Precalc_slope;
    SEG_NORMALS m_Normals;
}SEGMENT_WITH_NORMALS;


typedef std::vector< POLYSEGMENT > SEGMENTS;


/// This list will be used to test ray2d intersections. It will be a subset
/// of an original polygon. The normals will be passed already interpolated.
typedef std::vector< SEGMENT_WITH_NORMALS > SEGMENTS_WIDTH_NORMALS;


/// This structured will be used to handle a sub set of a polygon.
/// It can contain multiple closed polygons and holes.
/// It will be used to test if points are inside. A point will be inside the
/// polygon if it is not inside a hole and it is inside a Outer polygon.
typedef struct
{
    std::vector<SEGMENTS> m_Outers;
    std::vector<SEGMENTS> m_Holes;
}OUTERS_AND_HOLES;


/// This class represents a sub polygon block. This polygon block was created
/// from a general polygon definition that was sub divided and create blocks of
/// polygons. This polygon class represent a sub part of that main polygon.
/// There is information for the contours (used to test the ray2d intersection)
/// and a close definition of the block polygon to test if a point is inside.
class  CPOLYGONBLOCK2D : public COBJECT2D
{
private:
    /// This is the outter part of the polygon. This list is used to test a ray
    /// intersection with the boundaries of this sub polygon.
    /// It contains also the interpolated normals that are passed from the main
    /// polygon.
    SEGMENTS_WIDTH_NORMALS m_open_segments;

    /// A polygon block can have multiple polygon and holes
    OUTERS_AND_HOLES m_outers_and_holes;

public:
    CPOLYGONBLOCK2D( const SEGMENTS_WIDTH_NORMALS &aOpenSegmentList,
                     const OUTERS_AND_HOLES &aOuter_and_holes,
                     const BOARD_ITEM &aBoardItem );

    // Imported from COBJECT2D
    bool Overlaps( const CBBOX2D &aBBox ) const override;
    bool Intersects( const CBBOX2D &aBBox ) const override;
    bool Intersect( const RAYSEG2D &aSegRay, float *aOutT, SFVEC2F *aNormalOut ) const override;
    INTERSECTION_RESULT IsBBoxInside( const CBBOX2D &aBBox ) const override;
    bool IsPointInside( const SFVEC2F &aPoint ) const override;
};


/// This dummy block will be defined by a 2d box size. If the point is inside
/// the bounding box it will return allways true. However, the intersection with
/// a ray will return allways false.
/// This is used as a sub block extrated from polygon (pcb polygon areas) and
/// represents an area that is full filled.
class  CDUMMYBLOCK2D : public COBJECT2D
{

public:
    CDUMMYBLOCK2D( const SFVEC2F &aPbMin,
                   const SFVEC2F &aPbMax,
                   const BOARD_ITEM &aBoardItem );

    CDUMMYBLOCK2D( const CBBOX2D &aBBox, const BOARD_ITEM &aBoardItem );

     // Imported from COBJECT2D
     bool Overlaps( const CBBOX2D &aBBox ) const override;
     bool Intersects( const CBBOX2D &aBBox ) const override;
     bool Intersect( const RAYSEG2D &aSegRay, float *aOutT, SFVEC2F *aNormalOut ) const override;
     INTERSECTION_RESULT IsBBoxInside( const CBBOX2D &aBBox ) const override;
     bool IsPointInside( const SFVEC2F &aPoint ) const override;
};

/**
 * @brief Convert_path_polygon_to_polygon_blocks_and_dummy_blocks
 * This function will use a polygon in the format of the ClipperLib::Path
 * will process it and will create multiple 2d objects (CPOLYGONBLOCK2D and
 * CDUMMYBLOCK2D) that can be used to represent this polygon area.
 * @param aMainPath - the polygon are that was converted from the pcb board
 * @param aDstContainer - the destination container to put the created sub blocks
 * @param aBiuTo3DunitsScale - the rendering target 3d scale
 * @param aDivFactor - a division factor (in 3Dunits) to divide the polygon plane,
 * 0.0f will use the internal polygon segm statistics
 */
void Convert_path_polygon_to_polygon_blocks_and_dummy_blocks(
        const SHAPE_POLY_SET &aMainPath,
        CGENERICCONTAINER2D &aDstContainer,
        float aBiuTo3DunitsScale,
        float aDivFactor,
        const BOARD_ITEM &aBoardItem );

void Polygon2d_TestModule();

#endif // _CPOLYGON2D_H_
