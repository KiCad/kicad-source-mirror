/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
                          const wxPoint& pt )> OUTLINE_ERROR_HANDLER;

/**
 * Function ConvertOutlineToPolygon
 * build a polygon (with holes) from a PCB_SHAPE list, which is expected to be
 * a outline, therefore a closed main outline with perhaps closed inner outlines.
 * These closed inner outlines are considered as holes in the main outline
 * @param aSegList the initial list of drawsegments (only lines, circles and arcs).
 * @param aPolygons will contain the complex polygon.
 * @param aErrorMax is the max error distance when polygonizing a curve (internal units)
 * @param aChainingEpsilon is the max distance from one endPt to the next startPt (internal units)
 * @param aErrorHandler = an optional error handler
 */
bool ConvertOutlineToPolygon( std::vector<PCB_SHAPE*>& aSegList, SHAPE_POLY_SET& aPolygons,
                              int aErrorMax, int aChainingEpsilon,
                              OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr );


/**
 * Extracts the board outlines and build a closed polygon from lines, arcs and circle items on
 * edge cut layer.  Any closed outline inside the main outline is a hole.  All contours should be
 * closed, i.e. are valid vertices for a closed polygon.
 * @return true if success, false if a contour is not valid
 */
extern bool BuildBoardPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines,
                                       int aErrorMax, int aChainingEpsilon,
                                       OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr );

