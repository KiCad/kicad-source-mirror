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
 * @file 3d_draw_basic_functions.cpp
 */

#include <fctsys.h>
#include <trigo.h>
#include <convert_basic_shapes_to_polygon.h>

#include <3d_viewer.h>
#include <info3d_visu.h>
#include <3d_draw_basic_functions.h>


// Imported function:
extern void Set_Object_Data( std::vector<S3D_VERTEX>& aVertices, double aBiuTo3DUnits );
extern void CheckGLError();


#ifndef CALLBACK
#define CALLBACK
#endif

// CALLBACK functions for GLU_TESS
static void CALLBACK    tessBeginCB( GLenum which );
static void CALLBACK    tessEndCB();
static void CALLBACK    tessErrorCB( GLenum errorCode );
static void CALLBACK    tessCPolyPt2Vertex( const GLvoid* data );
static void CALLBACK    tesswxPoint2Vertex( const GLvoid* data );

/** draw a ring using 3D primitives, in a plane parallel to the XY plane
 * @param aCenterPos = position of the center (Board internal units)
 * @param aOuterRadius = radius of the external circle (Board internal units)
 * @param aInnerRadius = radius of the circle (Board internal units)
 * @param aZpos = z position in board internal units
 * @param aBiuTo3DUnits = board internal units to 3D units scaling value
 */
static void             Draw3D_FlatRing( wxPoint aCenterPos, int aOuterRadius,
                                     int aInnerRadius, int aZpos, double aBiuTo3DUnits );


void SetGLColor( int color )
{
    double          red, green, blue;
    StructColors    colordata = ColorRefs[color & MASKCOLOR];

    red     = colordata.m_Red / 255.0;
    blue    = colordata.m_Blue / 255.0;
    green   = colordata.m_Green / 255.0;
    glColor3f( red, green, blue );
}


/* draw all solid polygons found in aPolysList
 * aZpos = z position in board internal units
 * aThickness = thickness in board internal units
 * If aThickness = 0, a polygon area is drawn in a XY plane at Z position = aZpos.
 * If aThickness 1 0, a solid object is drawn.
 *  The top side is located at aZpos + aThickness / 2
 *  The bottom side is located at aZpos - aThickness / 2
 */
void Draw3D_SolidHorizontalPolyPolygons( const std::vector<CPolyPt>& aPolysList,
                                         int aZpos, int aThickness, double aBiuTo3DUnits )
{
    GLUtesselator* tess = gluNewTess();

    gluTessCallback( tess, GLU_TESS_BEGIN, ( void (CALLBACK*) () )tessBeginCB );
    gluTessCallback( tess, GLU_TESS_END, ( void (CALLBACK*) () )tessEndCB );
    gluTessCallback( tess, GLU_TESS_ERROR, ( void (CALLBACK*) () )tessErrorCB );
    gluTessCallback( tess, GLU_TESS_VERTEX, ( void (CALLBACK*) () )tessCPolyPt2Vertex );

    GLdouble    v_data[3];
    double      zpos = ( aZpos + (aThickness / 2) ) * aBiuTo3DUnits;
    g_Parm_3D_Visu.m_CurrentZpos = zpos;
    v_data[2] = aZpos + (aThickness / 2);

    // Set normal to toward positive Z axis, for a solid object only (to draw the top side)
    if( aThickness )
        glNormal3f( 0.0, 0.0, 1.0 );

    // gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);

    // Draw solid areas contained in this list
    std::vector<CPolyPt> polylist = aPolysList;    // temporary copy for gluTessVertex

    for( int side = 0; side < 2; side++ )
    {
        int startContour = 1;

        for( unsigned ii = 0; ii < polylist.size(); ii++ )
        {
            if( startContour == 1 )
            {
                gluTessBeginPolygon( tess, NULL );
                gluTessBeginContour( tess );
                startContour = 0;
            }

            v_data[0]   = polylist[ii].x * aBiuTo3DUnits;
            v_data[1]   = -polylist[ii].y * aBiuTo3DUnits;
            // gluTessVertex store pointers on data, not data, so do not store
            // different corners values in a temporary variable
            // but send pointer on each CPolyPt value in polylist
            // before calling gluDeleteTess
            gluTessVertex( tess, v_data, &polylist[ii] );

            if( polylist[ii].end_contour == 1 )
            {
                gluTessEndContour( tess );
                gluTessEndPolygon( tess );
                startContour = 1;
            }
        }

        if( aThickness == 0 )
            break;

        // Prepare the bottom side of solid areas
        zpos = ( aZpos - (aThickness / 2) ) * aBiuTo3DUnits;
        g_Parm_3D_Visu.m_CurrentZpos = zpos;
        v_data[2] = zpos;
        // Now;, set normal to toward negative Z axis, for the solid object bottom side
        glNormal3f( 0.0, 0.0, -1.0 );
    }

    gluDeleteTess( tess );

    if( aThickness == 0 )
        return;

    // Build the 3D data : vertical sides
    std::vector<S3D_VERTEX> vertices;
    vertices.resize( 4 );

    vertices[0].z   = aZpos + (aThickness / 2);
    vertices[1].z   = aZpos - (aThickness / 2);
    vertices[2].z   = vertices[1].z;
    vertices[3].z   = vertices[0].z;

    int startContour = 0;
    for( unsigned ii = 0; ii < polylist.size(); ii++ )
    {
        int jj = ii + 1;

        if( polylist[ii].end_contour == 1 )
        {
            jj = startContour;
            startContour = ii + 1;
        }

        vertices[0].x   = polylist[ii].x;
        vertices[0].y   = -polylist[ii].y;
        vertices[1].x   = vertices[0].x;
        vertices[1].y   = vertices[0].y;    // Z only changes.
        vertices[2].x   = polylist[jj].x;
        vertices[2].y   = -polylist[jj].y;
        vertices[3].x   = vertices[2].x;
        vertices[3].y   = vertices[2].y;    // Z only changes.

        Set_Object_Data( vertices, aBiuTo3DUnits );
    }

    glNormal3f( 0.0, 0.0, 1.0 );
}


/* draw the solid polygon found in aPolysList
 * The first polygonj is the main polygon, others are holes
 * See Draw3D_SolidHorizontalPolyPolygons for more info
 */
void Draw3D_SolidHorizontalPolygonWithHoles( const std::vector<CPolyPt>& aPolysList,
                                             int aZpos, int aThickness, double aBiuTo3DUnits )
{
    std::vector<CPolyPt> polygon;

    ConvertPolysListWithHolesToOnePolygon( aPolysList, polygon );
    Draw3D_SolidHorizontalPolyPolygons( polygon, aZpos, aThickness, aBiuTo3DUnits );
}


/* draw a cylinder (a tube) using 3D primitives.
 * the cylinder axis is parallel to the Z axis
 * If aHeight = height of the cylinder is 0, only one ring will be drawn
 * If aThickness = 0, only one cylinder will be drawn
 */
void Draw3D_ZaxisCylinder( wxPoint aCenterPos, int aRadius,
                           int aHeight, int aThickness,
                           int aZpos, double aBiuTo3DUnits )
{
    const int slice = 12;
    std::vector <CPolyPt> outer_cornerBuffer;

    TransformCircleToPolygon( outer_cornerBuffer, aCenterPos,
                              aRadius + (aThickness / 2), slice );

    std::vector<S3D_VERTEX> coords;
    coords.resize( 4 );

    if( aHeight )
    {
        // Draw the outer vertical side

        // Init Z position of the 4 points of a GL_QUAD
        coords[0].z = aZpos;
        coords[1].z = aZpos + aHeight;
        coords[2].z = coords[1].z;
        coords[3].z = coords[0].z;

        for( int ii = 0; ii < slice; ii++ )
        {
            int jj = ii + 1;

            if( jj >= slice )
                jj = 0;

            // Build the 4 vertices of each GL_QUAD
            coords[0].x = outer_cornerBuffer[jj].x;
            coords[0].y = -outer_cornerBuffer[jj].y;
            coords[1].x = coords[0].x;
            coords[1].y = coords[0].y;              // only z change
            coords[2].x = outer_cornerBuffer[ii].x;
            coords[2].y = -outer_cornerBuffer[ii].y;
            coords[3].x = coords[2].x;
            coords[3].y = coords[2].y;              // only z change

            // Creates the GL_QUAD
            Set_Object_Data( coords, aBiuTo3DUnits );
        }

        glNormal3f( 0.0, 0.0, 1.0 );    // Normal is Z axis
    }

    if( aThickness == 0 )
        return;

    // draw top (front) and bottom (back) horizontal sides (rings)
    S3D_VERTEX centerPos;
    centerPos.x = aCenterPos.x * aBiuTo3DUnits;
    centerPos.y = -aCenterPos.y * aBiuTo3DUnits;

    Draw3D_FlatRing( aCenterPos, aRadius + aThickness / 2, aRadius - aThickness / 2,
                 aZpos + aHeight, aBiuTo3DUnits );


    glNormal3f( 0.0, 0.0, -1.0 );
    Draw3D_FlatRing( aCenterPos, aRadius + aThickness / 2, aRadius - aThickness / 2,
                 aZpos, aBiuTo3DUnits );


    if( aHeight )
    {
    // Draws the vertical inner side (hole)
        std::vector <CPolyPt> inner_cornerBuffer;
        TransformCircleToPolygon( inner_cornerBuffer, aCenterPos,
                                  aRadius - (aThickness / 2), slice );

        for( int ii = 0; ii < slice; ii++ )
        {
            int jj = ii + 1;

            if( jj >= slice )
                jj = 0;

            // Build the 4 vertices of each GL_QUAD
            coords[0].x = inner_cornerBuffer[ii].x;
            coords[0].y = -inner_cornerBuffer[ii].y;
            coords[1].x = coords[0].x;
            coords[1].y = coords[0].y;              // only z change
            coords[2].x = inner_cornerBuffer[jj].x;
            coords[2].y = -inner_cornerBuffer[jj].y;
            coords[3].x = coords[2].x;
            coords[3].y = coords[2].y;              // only z change

            Set_Object_Data( coords, aBiuTo3DUnits );
        }
    }

    glNormal3f( 0.0, 0.0, 1.0 );    // Normal is Z axis
}


/*
 * Function Draw3D_ZaxisOblongCylinder:
 * draw a segment with an oblong hole.
 * Used to draw oblong holes
 */
void Draw3D_ZaxisOblongCylinder( wxPoint aAxis1Pos, wxPoint aAxis2Pos,
                                 int aRadius, int aHeight, int aThickness,
                                 int aZpos, double aBiuTo3DUnits  )
{
    const int slice = 12;

    // Build the points to approximate oblong cylinder by segments
    std::vector <CPolyPt> cornerBuffer;

    TransformRoundedEndsSegmentToPolygon( cornerBuffer, aAxis1Pos,
                                          aAxis2Pos, slice, aRadius * 2 );

    // Draw the cylinder
    std::vector<S3D_VERTEX> coords;
    coords.resize( 4 );

    // Init Z position of the 4 points of a GL_QUAD
    coords[0].z = aZpos;
    coords[1].z = aZpos + aHeight;
    coords[2].z = coords[1].z;
    coords[3].z = coords[0].z;

    for( int ii = 0; ii < slice; ii++ )
    {
        int jj = ii + 1;

        if( jj >= slice )
            jj = 0;

        // Build the 4 vertices of each GL_QUAD
        coords[0].x = cornerBuffer[ii].x;
        coords[0].y = -cornerBuffer[ii].y;
        coords[1].x = coords[0].x;
        coords[1].y = coords[0].y;              // only z change
        coords[2].x = cornerBuffer[jj].x;
        coords[2].y = -cornerBuffer[jj].y;
        coords[3].x = coords[2].x;
        coords[3].y = coords[2].y;              // only z change

        Set_Object_Data( coords, aBiuTo3DUnits );
    }

    glNormal3f( 0.0, 0.0, 1.0 );    // Normal is Z axis
}


/* draw a thick segment using 3D primitives, in a XY plane
 * wxPoint aStart, wxPoint aEnd = YX position of end in board units
 * aWidth = width of segment in board units
 * aThickness = thickness of segment in board units
 * aZpos = z position of segment in board units
 */
void Draw3D_SolidSegment( const wxPoint& aStart, const wxPoint& aEnd,
                          int aWidth, int aThickness, int aZpos, double aBiuTo3DUnits )
{
    std::vector <CPolyPt>   cornerBuffer;
    const int               slice = 16;

    TransformRoundedEndsSegmentToPolygon( cornerBuffer, aStart, aEnd, slice, aWidth );

    Draw3D_SolidHorizontalPolyPolygons( cornerBuffer, aZpos, aThickness, aBiuTo3DUnits );
}


void Draw3D_ArcSegment( const S3D_VERTEX& aCenterPos,
                        double aStartPointX, double aStartPointY,
                        double aArcAngle, double aWidth )
{
    const int   slice = 16;           // Number of segments to approximate a circle by segments
    double      arcStart_Angle;

    arcStart_Angle =
        (atan2( aStartPointX - aCenterPos.x, aStartPointY - aCenterPos.y ) * 1800 / M_PI );
    double  radius = hypot( aStartPointX - aCenterPos.x, aStartPointY - aCenterPos.y )
                     + ( aWidth / 2);
    double  hole = radius - aWidth;

    // Calculate the number of segments to approximate this arc
    int     imax = (int) ( (double) aArcAngle / ANGLE_INC( slice ) );

    if( imax < 0 )
        imax = -imax;

    if( imax == 0 )
        imax = 1;

    // Adjust delta_angle to have exactly imax segments in arc_angle
    // i.e. arc_angle = imax delta_agnle.
    double delta_angle = (double) aArcAngle / imax;

    glBegin( GL_QUAD_STRIP );

    for( int ii = 0; ii <= imax; ii++ )
    {
        double  angle = (double) ii * delta_angle;
        angle += arcStart_Angle + 900;
        double  dx  = hole;
        double  dy  = 0.0;
        RotatePoint( &dx, &dy, (int) angle );
        glVertex3f( dx + aStartPointX, dy + aStartPointY, aCenterPos.z );
        dx  = radius;
        dy  = 0.0;
        RotatePoint( &dx, &dy, (int) angle );
        glVertex3f( dx + aStartPointX, dy + aStartPointY, aCenterPos.z );
    }

    glEnd();
}


/** draw a ring using 3D primitives, in a plane parallel to the XY plane
 * @param aCenterPos = position of the center (Board internal units)
 * @param aOuterRadius = radius of the external circle (Board internal units)
 * @param aInnerRadius = radius of the circle (Board internal units)
 * @param aZpos = z position in board internal units
 * @param aBiuTo3DUnits = board internal units to 3D units scaling value
 */
void Draw3D_FlatRing( wxPoint aCenterPos, int aOuterRadius,
                  int aInnerRadius, int aZpos, double aBiuTo3DUnits )
{
    const int   slice = 16;
    const int   rot_angle   = ANGLE_INC( slice );
    double      cposx       = aCenterPos.x * aBiuTo3DUnits;
    double      cposy       = - aCenterPos.y * aBiuTo3DUnits;

    glBegin( GL_QUAD_STRIP );

    double zpos = aZpos * aBiuTo3DUnits;

    for( int ii = 0; ii <= slice; ii++ )
    {
        double  x   = aInnerRadius * aBiuTo3DUnits;
        double  y   = 0.0;
        RotatePoint( &x, &y, ii * rot_angle );
        glVertex3f( x + cposx, y + cposy, zpos );
        x   = aOuterRadius * aBiuTo3DUnits;
        y   = 0.0;
        RotatePoint( &x, &y, ii * rot_angle );
        glVertex3f( x + cposx, y + cposy, zpos );
    }

    glEnd();
}


/* draw one solid polygon
 * aCornersList = a std::vector<wxPoint> list of corners, in board internal units
 * aZpos = z position in board internal units
 * aThickness = thickness in board internal units
 * aIu_to_3Dunits = board internal units to 3D units scaling value
 */
void   Draw3D_HorizontalPolygon( std::vector<wxPoint>& aCornersList, int aZpos,
                                 int aThickness, double aBiuTo3DUnits )
{
    GLUtesselator* tess = gluNewTess();

    gluTessCallback( tess, GLU_TESS_BEGIN, ( void (CALLBACK*) () )tessBeginCB );
    gluTessCallback( tess, GLU_TESS_END, ( void (CALLBACK*) () )tessEndCB );
    gluTessCallback( tess, GLU_TESS_ERROR, ( void (CALLBACK*) () )tessErrorCB );
    gluTessCallback( tess, GLU_TESS_VERTEX, ( void (CALLBACK*) () )tesswxPoint2Vertex );

    GLdouble v_data[3];
    v_data[2] = ( aZpos + (aThickness / 2) ) * aBiuTo3DUnits;
    g_Parm_3D_Visu.m_CurrentZpos = v_data[2];

    // gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);

    // Draw solid polygon
    if( aThickness )
        glNormal3f( 0.0, 0.0, 1.0 );

    for( int side = 0; side < 2; side++ )
    {
        gluTessBeginPolygon( tess, NULL );
        gluTessBeginContour( tess );

        for( unsigned ii = 0; ii < aCornersList.size(); ii++ )
        {
            v_data[0]   = aCornersList[ii].x * g_Parm_3D_Visu.m_BiuTo3Dunits;
            v_data[1]   = -aCornersList[ii].y * g_Parm_3D_Visu.m_BiuTo3Dunits;
            // gluTessVertex store pointers on data, not data, so do not store
            // different corners values in a temporary variable
            // but send pointer on each corner value in aCornersList
            gluTessVertex( tess, v_data, &aCornersList[ii] );
        }

        gluTessEndContour( tess );
        gluTessEndPolygon( tess );

        if( aThickness == 0 )
            break;

        glNormal3f( 0.0, 0.0, -1.0 );
        v_data[2] = ( aZpos - (aThickness / 2) ) * aBiuTo3DUnits;
        g_Parm_3D_Visu.m_CurrentZpos = v_data[2];
    }

    gluDeleteTess( tess );

    if( aThickness == 0 )
        return;

    // Build the 3D data : vertical sides
    std::vector<S3D_VERTEX> vertices;
    vertices.resize( 4 );

    vertices[0].z   = aZpos + (aThickness / 2);
    vertices[1].z   = aZpos - (aThickness / 2);
    vertices[2].z   = vertices[1].z;
    vertices[3].z   = vertices[0].z;

    int slice = (int) aCornersList.size();

    for( int ii = 0; ii < slice; ii++ )
    {
        int jj = ii + 1;

        if( jj >=slice )
            jj = 0;

        vertices[0].x   = aCornersList[ii].x;
        vertices[0].y   = -aCornersList[ii].y;
        vertices[1].x   = vertices[0].x;
        vertices[1].y   = vertices[0].y;    // Z only changes.
        vertices[2].x   = aCornersList[jj].x;
        vertices[2].y   = -aCornersList[jj].y;
        vertices[3].x   = vertices[2].x;
        vertices[3].y   = vertices[2].y;    // Z only changes.

        Set_Object_Data( vertices, aBiuTo3DUnits );
    }
}


// /////////////////////////////////////////////////////////////////////////////
// GLU_TESS CALLBACKS
// /////////////////////////////////////////////////////////////////////////////

void CALLBACK tessBeginCB( GLenum which )
{
    glBegin( which );
}


void CALLBACK tessEndCB()
{
    glEnd();
}


void CALLBACK tessCPolyPt2Vertex( const GLvoid* data )
{
    // cast back to double type
    const CPolyPt* ptr = (const CPolyPt*) data;

    glVertex3f( ptr->x * g_Parm_3D_Visu.m_BiuTo3Dunits,
                -ptr->y * g_Parm_3D_Visu.m_BiuTo3Dunits,
                g_Parm_3D_Visu.m_CurrentZpos );
}


void CALLBACK tesswxPoint2Vertex( const GLvoid* data )
{
    const wxPoint* ptr = (const wxPoint*) data;

    glVertex3f( ptr->x * g_Parm_3D_Visu.m_BiuTo3Dunits,
                -ptr->y * g_Parm_3D_Visu.m_BiuTo3Dunits,
                g_Parm_3D_Visu.m_CurrentZpos );
}


void CALLBACK tessErrorCB( GLenum errorCode )
{
#if defined(DEBUG)
    const GLubyte* errorStr;

    errorStr = gluErrorString( errorCode );

    // DEBUG //
    D( printf( "Tess ERROR: %s\n", errorStr ); )
#endif
}
