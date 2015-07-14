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
#include <modelparsers.h>

// Number of segments to approximate a circle by segments
#define SEGM_PER_CIRCLE 24

#ifndef CALLBACK
#define CALLBACK
#endif

// Variables used to pass a value to call back openGL functions
static float s_textureScale;
static double s_currentZpos;
static double s_biuTo3Dunits;
bool s_useTextures;

// CALLBACK functions for GLU_TESS
static void CALLBACK    tessBeginCB( GLenum which );
static void CALLBACK    tessEndCB();
static void CALLBACK    tessErrorCB( GLenum errorCode );
static void CALLBACK    tessCPolyPt2Vertex( const GLvoid* data );

void TransfertToGLlist( std::vector< S3D_VERTEX >& aVertices, double aBiuTo3DUnits );

/* Draw3D_VerticalPolygonalCylinder is a helper function.
 *
 * draws a "vertical cylinder" having a polygon shape
 * from Z position = aZpos to aZpos + aHeight
 * Used to create the vertical sides of 3D horizontal shapes with thickness.
 */
static void Draw3D_VerticalPolygonalCylinder( const CPOLYGONS_LIST& aPolysList,
                                              int aHeight, int aZpos,
                                              bool aInside, double aBiuTo3DUnits )
{
    if( aHeight == 0 )
        return;

    std::vector<S3D_VERTEX> coords;
    coords.resize( 4 );

    // Init Z position of the 4 points of a GL_QUAD
    if( aInside )
    {
        coords[0].z = aZpos;
        coords[1].z = aZpos + aHeight;
    }
    else
    {
        coords[0].z = aZpos + aHeight;
        coords[1].z = aZpos;
    }
    coords[2].z = coords[1].z;
    coords[3].z = coords[0].z;

    // Draw the vertical polygonal side
    int startContour = 0;
    for( unsigned ii = 0; ii < aPolysList.GetCornersCount(); ii++ )
    {
        unsigned jj = ii + 1;

        if( aPolysList.IsEndContour( ii ) || jj >= aPolysList.GetCornersCount() )
        {
            jj = startContour;
            startContour = ii + 1;
        }

        // Build the 4 vertices of each GL_QUAD
        coords[0].x = aPolysList.GetX( ii );
        coords[0].y = -aPolysList.GetY( ii );
        coords[1].x = coords[0].x;
        coords[1].y = coords[0].y;              // only z change
        coords[2].x = aPolysList.GetX( jj );
        coords[2].y = -aPolysList.GetY( jj );
        coords[3].x = coords[2].x;
        coords[3].y = coords[2].y;              // only z change

        // Creates the GL_QUAD
        TransfertToGLlist( coords, aBiuTo3DUnits );
    }
}


void SetGLColor( EDA_COLOR_T color, double alpha )
{
    const StructColors &colordata = g_ColorRefs[ColorGetBase( color )];

    float red     = colordata.m_Red / 255.0;
    float blue    = colordata.m_Blue / 255.0;
    float green   = colordata.m_Green / 255.0;
    glColor4f( red, green, blue, (float)alpha );
}


void SetGLColor( S3D_COLOR& aColor, float aTransparency )
{
    glColor4f( aColor.m_Red, aColor.m_Green, aColor.m_Blue, aTransparency );
}


void SetGLTexture( GLuint text_id, float scale )
{
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, text_id );
    s_textureScale = scale;     // for Tess callback functions
}


/* draw all solid polygons found in aPolysList
 * aZpos = z position in board internal units
 * aThickness = thickness in board internal units
 * If aThickness = 0, a polygon area is drawn in a XY plane at Z position = aZpos.
 * If aThickness > 0, a solid object is drawn.
 *  The top side is located at aZpos + aThickness / 2
 *  The bottom side is located at aZpos - aThickness / 2
 */
void Draw3D_SolidHorizontalPolyPolygons( const CPOLYGONS_LIST& aPolysList,
                                         int aZpos, int aThickness, double aBiuTo3DUnits,
                                         bool aUseTextures,
                                         float aNormal_Z_Orientation )
{
    // for Tess callback functions:
    s_biuTo3Dunits = aBiuTo3DUnits;
    s_useTextures = aUseTextures;

    GLUtesselator* tess = gluNewTess();

    gluTessCallback( tess, GLU_TESS_BEGIN, ( void (CALLBACK*) () )tessBeginCB );
    gluTessCallback( tess, GLU_TESS_END, ( void (CALLBACK*) () )tessEndCB );
    gluTessCallback( tess, GLU_TESS_ERROR, ( void (CALLBACK*) () )tessErrorCB );
    gluTessCallback( tess, GLU_TESS_VERTEX, ( void (CALLBACK*) () )tessCPolyPt2Vertex );

    GLdouble v_data[3];
    double   zpos = ( aZpos + (aThickness / 2.0) ) * aBiuTo3DUnits;
    s_currentZpos = zpos;     // for Tess callback functions
    v_data[2]     = zpos;

    // Set normal toward positive Z axis, for a solid object on the top side

    //gluTessProperty( tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE );
    //gluTessProperty( tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD );

    glNormal3f( 0.0, 0.0, aNormal_Z_Orientation );

    // Draw solid areas contained in this list
    CPOLYGONS_LIST polylist = aPolysList;    // temporary copy for gluTessVertex

    int startContour;

    for( int side = 0; side < 2; side++ )
    {
        startContour = 1;

        for( unsigned ii = 0; ii < polylist.GetCornersCount(); ii++ )
        {
            if( startContour == 1 )
            {
                gluTessBeginPolygon( tess, NULL );
                gluTessBeginContour( tess );
                startContour = 0;
            }

            v_data[0]   = polylist.GetX( ii ) * aBiuTo3DUnits;
            v_data[1]   = -polylist.GetY( ii ) * aBiuTo3DUnits;
            // gluTessVertex store pointers on data, not data, so do not store
            // different corners values in a temporary variable
            // but send pointer on each CPolyPt value in polylist
            // before calling gluDeleteTess
            gluTessVertex( tess, v_data, &polylist[ii] );


            if( polylist.IsEndContour( ii ) )
            {
                gluTessEndContour( tess );
                gluTessEndPolygon( tess );
                startContour = 1;
            }
        }

        if( aThickness == 0 )
            break;

        // Prepare the bottom side of solid areas
        zpos = ( aZpos - (aThickness / 2.0) ) * aBiuTo3DUnits;
        s_currentZpos = zpos;     // for Tess callback functions
        v_data[2] = zpos;

        glNormal3f( 0.0, 0.0, -aNormal_Z_Orientation );
    }

    if( startContour == 0 )
    {
        gluTessEndContour( tess );
        gluTessEndPolygon( tess );
    }

    gluDeleteTess( tess );

    if( aThickness == 0 )
        return;

    // Build the 3D data : vertical side
    Draw3D_VerticalPolygonalCylinder( polylist, aThickness, aZpos - (aThickness / 2.0),
                                      true, aBiuTo3DUnits );
}


/* draw the solid polygon found in aPolysList
 * The first polygon is the main polygon, others are holes
 * See Draw3D_SolidHorizontalPolyPolygons for more info
 */
void Draw3D_SolidHorizontalPolygonWithHoles( const CPOLYGONS_LIST& aPolysList,
                                             int aZpos, int aThickness,
                                             double aBiuTo3DUnits, bool aUseTextures,
                                             float aNormal_Z_Orientation )
{
    CPOLYGONS_LIST polygon;

    ConvertPolysListWithHolesToOnePolygon( aPolysList, polygon );
    Draw3D_SolidHorizontalPolyPolygons( polygon, aZpos, aThickness, aBiuTo3DUnits, aUseTextures,
                                        aNormal_Z_Orientation );
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
    const int slice = SEGM_PER_CIRCLE;
    CPOLYGONS_LIST outer_cornerBuffer;

    TransformCircleToPolygon( outer_cornerBuffer, aCenterPos,
                              aRadius + (aThickness / 2), slice );

    std::vector<S3D_VERTEX> coords;
    coords.resize( 4 );

    CPOLYGONS_LIST inner_cornerBuffer;
    if( aThickness )    // build the the vertical inner polygon (hole)
        TransformCircleToPolygon( inner_cornerBuffer, aCenterPos,
                                  aRadius - (aThickness / 2), slice );

    if( aHeight )
    {

        // Draw the vertical outer side
        Draw3D_VerticalPolygonalCylinder( outer_cornerBuffer,
                                      aHeight, aZpos, false, aBiuTo3DUnits );

        if( aThickness )
            // Draws the vertical inner side (hole)
            Draw3D_VerticalPolygonalCylinder( inner_cornerBuffer,
                                          aHeight, aZpos, true, aBiuTo3DUnits );
    }

    if( aThickness )
    {
        // draw top (front) and bottom (back) horizontal sides (rings)
        outer_cornerBuffer.Append( inner_cornerBuffer );
        CPOLYGONS_LIST polygon;

        ConvertPolysListWithHolesToOnePolygon( outer_cornerBuffer, polygon );
        // draw top (front) horizontal ring
        Draw3D_SolidHorizontalPolyPolygons( polygon, aZpos + aHeight, 0, aBiuTo3DUnits, false,
                                            1.0f );

        if( aHeight )
        {
            // draw bottom (back) horizontal ring
            Draw3D_SolidHorizontalPolyPolygons( polygon, aZpos, 0, aBiuTo3DUnits, false,
                                                -1.0f );
        }
    }
}


/*
 * Function Draw3D_ZaxisOblongCylinder:
 * draw a segment with an oblong hole.
 * Used to draw oblong holes
 * If aHeight = height of the cylinder is 0, only one ring will be drawn
 * If aThickness = 0, only one cylinder will be drawn
 */
void Draw3D_ZaxisOblongCylinder( wxPoint aAxis1Pos, wxPoint aAxis2Pos,
                                 int aRadius, int aHeight, int aThickness,
                                 int aZpos, double aBiuTo3DUnits  )
{
    const int slice = SEGM_PER_CIRCLE;

    // Build the points to approximate oblong cylinder by segments
    CPOLYGONS_LIST outer_cornerBuffer;

    int segm_width = (aRadius * 2) + aThickness;
    TransformRoundedEndsSegmentToPolygon( outer_cornerBuffer, aAxis1Pos,
                                          aAxis2Pos, slice, segm_width );

    // Draw the oblong outer cylinder
    if( aHeight )
        Draw3D_VerticalPolygonalCylinder( outer_cornerBuffer, aHeight, aZpos,
                                          false, aBiuTo3DUnits );

    if( aThickness )
    {
        CPOLYGONS_LIST inner_cornerBuffer;
        segm_width = aRadius * 2;
        TransformRoundedEndsSegmentToPolygon( inner_cornerBuffer, aAxis1Pos,
                                              aAxis2Pos, slice, segm_width );

        // Draw the oblong inner cylinder
        if( aHeight )
            Draw3D_VerticalPolygonalCylinder( inner_cornerBuffer, aHeight,
                                              aZpos, true, aBiuTo3DUnits );

        // Build the horizontal full polygon shape
        // (outer polygon shape - inner polygon shape)
        outer_cornerBuffer.Append( inner_cornerBuffer );

        CPOLYGONS_LIST polygon;
        ConvertPolysListWithHolesToOnePolygon( outer_cornerBuffer, polygon );

        // draw top (front) horizontal side (ring)
        Draw3D_SolidHorizontalPolyPolygons( polygon, aZpos + aHeight, 0, aBiuTo3DUnits, false,
                                            1.0f );

        if( aHeight )
        {
            // draw bottom (back) horizontal side (ring)
            Draw3D_SolidHorizontalPolyPolygons( polygon, aZpos, 0, aBiuTo3DUnits, false,
                                                -1.0f );
        }
    }
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
    CPOLYGONS_LIST   cornerBuffer;
    const int               slice = SEGM_PER_CIRCLE;

    TransformRoundedEndsSegmentToPolygon( cornerBuffer, aStart, aEnd, slice, aWidth );

    Draw3D_SolidHorizontalPolyPolygons( cornerBuffer, aZpos, aThickness, aBiuTo3DUnits, false, 1.0f );
}


void Draw3D_ArcSegment( const wxPoint&  aCenterPos, const wxPoint& aStartPoint,
                        double aArcAngle, int aWidth, int aThickness,
                        int aZpos, double aBiuTo3DUnits )
{
    const int   slice = SEGM_PER_CIRCLE;

    CPOLYGONS_LIST cornerBuffer;
    TransformArcToPolygon( cornerBuffer, aCenterPos, aStartPoint, aArcAngle,
                           slice, aWidth );

    Draw3D_SolidHorizontalPolyPolygons( cornerBuffer, aZpos, aThickness, aBiuTo3DUnits, false, 1.0f );
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

    if( s_useTextures )
    {
        glTexCoord2f( ptr->x * s_biuTo3Dunits * s_textureScale,
                      -ptr->y * s_biuTo3Dunits * s_textureScale);
    }

    glVertex3d( ptr->x * s_biuTo3Dunits, -ptr->y * s_biuTo3Dunits, s_currentZpos );
}


void CALLBACK tessErrorCB( GLenum errorCode )
{
#if defined(DEBUG)
    const GLubyte* errorStr;

    errorStr = gluErrorString( errorCode );

    // DEBUG //
    DBG( printf( "Tess ERROR: %s\n", errorStr ); )
#endif
}
