/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file 3d_draw_basic_functions.h
 */
#ifndef _3D_DRAW_BASIC_FUNCTIONS_H_
#define _3D_DRAW_BASIC_FUNCTIONS_H_

// angle increment to draw a circle, approximated by segments
#define ANGLE_INC( x ) ( 3600 / (x) )

/** draw all solid polygons found in aPolysList
 * @param aPolysList = the poligon list to draw
 * @param aZpos = z position in board internal units
 * @param aThickness = thickness in board internal units
 * @param aBiuTo3DUnits = board internal units to 3D units scaling value
 * If aThickness = 0, a polygon area is drawn in a XY plane at Z position = aZpos.
 * If aThickness > 0, a solid object is drawn.
 *  The top side is located at aZpos + aThickness / 2
 *  The bottom side is located at aZpos - aThickness / 2
 */
void    Draw3D_SolidHorizontalPolyPolygons( const CPOLYGONS_LIST& aPolysList,
                                            int aZpos, int aThickness, double aBiuTo3DUnits );

/** draw the solid polygon found in aPolysList
 * The first polygonj is the main polygon, others are holes
 * @param aPolysList = the polygon with holes to draw
 * @param aZpos = z position in board internal units
 * @param aThickness = thickness in board internal units
 * @param aBiuTo3DUnits = board internal units to 3D units scaling value
 * If aThickness = 0, a polygon area is drawn in a XY plane at Z position = aZpos.
 * If aThickness > 0, a solid object is drawn.
 *  The top side is located at aZpos + aThickness / 2
 *  The bottom side is located at aZpos - aThickness / 2
 */
void    Draw3D_SolidHorizontalPolygonWithHoles( const CPOLYGONS_LIST& aPolysList,
                                                int aZpos, int aThickness, double aBiuTo3DUnits );

/** draw a thick segment using 3D primitives, in a XY plane
 * @param aStart = YX position of start point in board units
 * @param aEnd = YX position of end point in board units
 * @param aWidth = width of segment in board units
 * @param aThickness = thickness of segment in board units
 * @param aZpos = z position of segment in board units
 * @param aBiuTo3DUnits = board internal units to 3D units scaling value
 * If aThickness = 0, a polygon area is drawn in a XY plane at Z position = aZpos.
 * If aThickness > 0, a solid object is drawn.
 *  The top side is located at aZpos + aThickness / 2
 *  The bottom side is located at aZpos - aThickness / 2
 */
void    Draw3D_SolidSegment( const wxPoint& aStart, const wxPoint& aEnd,
                             int aWidth, int aThickness, int aZpos,
                             double aBiuTo3DUnits );

/** draw an arc using 3D primitives, in a XY plane
 * @param aCenterPos = XY position of the center in board units
 * @param aStartPoint = start point coordinate of arc in board units
 * @param aWidth = width of the circle in board units
 * @param aArcAngle = arc angle in 1/10 degrees
 * @param aWidth = thickness of arc
 * @param aThickness = thickness of segment in board units
 * @param aZpos = z position of segment in board units
 * @param aBiuTo3DUnits = board internal units to 3D units scaling value
 */
void Draw3D_ArcSegment( const wxPoint&  aCenterPos, const wxPoint& aStartPoint,
                        double aArcAngle, int aWidth, int aThickness,
                        int aZpos, double aBiuTo3DUnits );


/** draw a thick cylinder (a tube) using 3D primitives.
 * the cylinder axis is parallel to the Z axis
 * @param aCenterPos = XY position of the axis cylinder ( board internal units)
 * @param aRadius = radius of the cylinder ( board internal units)
 * @param aHeight = height of the cylinder ( boardinternal units)
 * @param aThickness = tichkness of tube ( boardinternal units)
 * @param aZpos = Z position of the bottom side of the cylinder ( board internal units)
 * @param aBiuTo3DUnits = board internal units to 3D units scaling value
 *
 * If aHeight = height of the cylinder is 0, only one ring will be drawn
 * If aThickness = 0, only one cylinder (not a tube) will be drawn
 */
void    Draw3D_ZaxisCylinder( wxPoint aCenterPos, int aRadius,
                              int aHeight, int aThickness,
                              int aZpos, double aBiuTo3DUnits );

/** draw an oblong cylinder (oblong tube) using 3D primitives.
 * the cylinder axis are parallel to the Z axis
 * @param aAxis1Pos = position of the first axis cylinder
 * @param aAxis2Pos = position of the second axis cylinder
 * @param aRadius = radius of the cylinder ( board internal units )
 * @param aHeight = height of the cylinder ( board internal units )
 * @param aThickness = tichkness of tube ( board internal units )
 * @param aZpos = Z position of the bottom side of the cylinder ( board internal units )
 * @param aBiuTo3DUnits = board internal units to 3D units scaling value
 */
void    Draw3D_ZaxisOblongCylinder( wxPoint aAxis1Pos, wxPoint aAxis2Pos,
                                    int aRadius, int aHeight, int aThickness,
                                    int aZpos, double aBiuTo3DUnits  );
/**
 * Set the current 3D color from a Kicad  color, with optional transparency
 * @param aColor = a EDA_COLOR_T kicad color index
 * @param aTransparency = the color transparency (default = 1.0 = no transparency)
 */
void SetGLColor( EDA_COLOR_T aColor, double aTransparency = 1.0 );


/**
 * Set a texture id and a scale to apply when rendering the polygons
 * @param text_id = texture ID created by glGenTextures
 * @param scale = scale to apply to texture coords
 */
void SetGLTexture( GLuint text_id, float scale );


#endif      // _3D_DRAW_BASIC_FUNCTIONS_H_
