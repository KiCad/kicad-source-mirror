/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

class PCB_SHAPE;
class SHAPE_POLY_SET;
class wxString;
class wxPoint;

typedef
const std::function<void( const wxString& msg, BOARD_ITEM* itemA, BOARD_ITEM* itemB,
                          const VECTOR2I& pt )> OUTLINE_ERROR_HANDLER;

/**
 * Test a board graphic items on edge cut layer for validity.
 *
 * @param aBoard is the board to test.
 * @param aMinDist is the min length of a segment (or radius, or diagonal size of a rect)
 *        to be valid.
 * @param aErrorHandler is an optional error handler.
 */
bool TestBoardOutlinesGraphicItems( BOARD* aBoard, int aMinDist,
                                    OUTLINE_ERROR_HANDLER* aErrorHandler );

/**
 * Build a polygon set with holes from a #PCB_SHAPE list.
 *
 * The shape list is expected to be one or more top-level closed outlines with zero or more
 * holes in each.  Optionally, it can be limited to a single top-level closed outline.
 *
 * @param aShapeList the initial list of drawsegments (only lines, circles and arcs).
 * @param aPolygons will contain the complex polygon.
 * @param aErrorMax is the max error distance when polygonizing a curve (internal units).
 * @param aChainingEpsilon is the max distance from one endPt to the next startPt (internal units).
 * @param aAllowDisjoint indicates multiple top-level outlines are allowed.
 * @param aErrorHandler is an optional error handler.
 * @param aAllowUseArcsInPolygons is an option to allow adding arcs in #SHAPE_LINE_CHAIN
 *                                polylines/polygons when building outlines from aShapeList
 *                                This is mainly for export to STEP files.
 * @return true if success, false if a contour is not valid (self intersecting).
 */
bool ConvertOutlineToPolygon( std::vector<PCB_SHAPE*>& aShapeList, SHAPE_POLY_SET& aPolygons,
                              int aErrorMax, int aChainingEpsilon, bool aAllowDisjoint,
                              OUTLINE_ERROR_HANDLER* aErrorHandler,
                              bool aAllowUseArcsInPolygons = false );


/**
 * Extract the board outlines and build a closed polygon from lines, arcs and circle items on
 * edge cut layer.
 *
 * Any closed outline inside the main outline is a hole.  All contours should be closed, i.e. are
 * valid vertices for a closed polygon.
 *
 * @param aBoard is the board to build outlines.
 * @param aOutlines will contain the outlines ( complex polygons ).
 * @param aErrorMax is the max error distance when polygonizing a curve (internal units).
 * @param aChainingEpsilon is the max distance from one endPt to the next startPt (internal units)
 * @param aInferOutlineIfNecessary if the edges do not define a closed shape then we'll approximate the bounding
 *                                 box outline based on the edges, or failing that, any other items on the board
 * @param aErrorHandler = an optional error handler.
 * @param aAllowUseArcsInPolygons is an option to allow adding arcs in #SHAPE_LINE_CHAIN
 *                                polylines/polygons when building outlines from aShapeList
 *                                This is mainly for export to STEP files.
 * @return true if success, false if a contour is not valid.
 */
extern bool BuildBoardPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines,
                                       int aErrorMax, int aChainingEpsilon, bool aInferOutlineIfNecessary,
                                       OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr,
                                       bool aAllowUseArcsInPolygons = false );


/**
 * Extract a board outline for a footprint view.
 *
 * * Incomplete outlines will be closed by joining the end of the outline onto the bounding box
 *   (by simply projecting the end points) and then take the area that contains the copper.
 * * If all copper lies inside a closed outline, than that outline will be treated as an external
 *   board outline.
 * * If copper is located outside a closed outline, then that outline will be treated as a hole,
 *   and the outer edge will be formed using the bounding box.
 */
bool BuildFootprintPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines,
                                    int aErrorMax, int aChainingEpsilon,
                                    OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr );
