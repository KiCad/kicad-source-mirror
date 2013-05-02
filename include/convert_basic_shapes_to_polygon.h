/**
 * @file convert_basic_shapes_to_polygon.h
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#ifndef CONVERT_BASIC_SHAPES_TO_POLYGON_H
#define CONVERT_BASIC_SHAPES_TO_POLYGON_H

#include <vector>

#include <fctsys.h>
#include <trigo.h>
#include <macros.h>
#include <PolyLine.h>

/**
 * Helper function CopyPolygonsFromKiPolygonListToPolysList
 * We are using a lots polygons in calculations.
 * and we are using 2 descriptions,
 * one easy to use with boost::polygon (KI_POLYGON_SET)
 * one easy to use in zones and in draw functions (std::vector<CPolyPt>)
 * Copy polygons from a KI_POLYGON_SET set of polygons to
 * a std::vector<CPolyPt> polygon list
 * Therefore we need conversion functions between these 2 descriptions
 * This function converts a KI_POLYGON_SET description to a
 * std::vector<CPolyPt> description
 * @param aKiPolyList = source (set of polygons)
 * @param aPolysList = destination (set of polygons using CPolyPt corners descr)
 */
void CopyPolygonsFromKiPolygonListToPolysList( KI_POLYGON_SET& aKiPolyList,
                                               std::vector<CPolyPt>& aPolysList );

/**
 * Helper function AddPolygonCornersToKiPolygonList
 * This function adds a KI_POLYGON_SET description to a
 * std::vector<CPolyPt> description
 * @param aCornersBuffer = source (set of polygons using CPolyPt corners descr)
 * @param aPolysList = destination (set of polygons)
 */
void AddPolygonCornersToKiPolygonList( std::vector <CPolyPt>& aCornersBuffer,
                                       KI_POLYGON_SET&        aKiPolyList );

/**
 * Function TransformCircleToPolygon
 * convert a circle to a polygon, using multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCenter = the center of the circle
 * @param aRadius = the radius of the circle
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * Note: the polygon is inside the circle, so if you want to have the polygon
 * outside the circle, you should give aRadius calculated with a correction factor
 */
void TransformCircleToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                                           wxPoint aCenter, int aRadius,
                                           int aCircleToSegmentsCount );

/**
 * Function TransformRoundedEndsSegmentToPolygon
 * convert a segment with rounded ends to a polygon
 * Convert arcs to multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aStart = the segment start point coordinate
 * @param aEnd = the segment end point coordinate
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = the segment width
 * Note: the polygon is inside the arc ends, so if you want to have the polygon
 * outside the circle, you should give aStart and aEnd calculated with a correction factor
 */
void TransformRoundedEndsSegmentToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                                           wxPoint aStart, wxPoint aEnd,
                                           int aCircleToSegmentsCount,
                                           int aWidth );


/**
 * Function TransformArcToPolygon
 * Creates a polygon from an Arc
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aStart = start point of the arc, or a point on the circle
 * @param aArcAngle = arc angle in 0.1 degrees. For a circle, aArcAngle = 3600
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = width (thickness) of the line
 */
void TransformArcToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                            wxPoint aCentre, wxPoint aStart, int aArcAngle,
                            int aCircleToSegmentsCount, int aWidth );

/**
 * Function TransformRingToPolygon
 * Creates a polygon from a ring
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aRadius = radius of the circle
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = width (thickness) of the ring
 */
void TransformRingToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                            wxPoint aCentre, int aRadius,
                            int aCircleToSegmentsCount, int aWidth );

#endif     // CONVERT_BASIC_SHAPES_TO_POLYGON_H
