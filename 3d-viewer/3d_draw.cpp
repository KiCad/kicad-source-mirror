/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file 3d_draw.cpp
*/

#include <fctsys.h>
#include <common.h>
#include <trigo.h>
#include <pcbstruct.h>
#include <drawtxt.h>
//include <confirm.h>
#include <layers_id_colors_and_visibility.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_board_design_settings.h>
//#include <class_marker_pcb.h>
#include <colors_selection.h>

#include <3d_viewer.h>
#include <info3d_visu.h>
#include <trackball.h>

// angle increment to draw a circle, approximated by segments
#define ANGLE_INC(x) (3600/(x))

extern void CheckGLError();

/* draw a segment using 3D primitives, in a XY plane
 * aStartPos = 3D position of the starting point (3D units)
 * aEndx, aEndy = 3D position of the ending point (3D units)
 * aWidth = width of the segment (3D units)
 */
static void    Draw3D_FilledSegment( const S3D_VERTEX& aStartPos,
                                     double aEndx, double aEndy, double aWidth );

static void    Draw3D_SegmentWithHole( double startx, double starty,
                                             double endx, double endy,
                                             double width, double holex,
                                             double holey, double holeradius,
                                             double zpos );

/* draw an arc using 3D primitives, in a plane parallel to the XY plane
 * aCenterPos = 3D position of the center
 * aStartPointX, aStartPointY = start point coordinate of arc (3D units)
 * aWidth = width of the circle (3D units)
 * aArcAngle = arc angle in 1/10 degrees
 * aWidth = thickness of arc
 */
static void    Draw3D_ArcSegment( const S3D_VERTEX& aCenterPos,
                                  double aStartPointX, double aStartPointY,
                                  double aArcAngle, double aWidth );

/* draw a circle with hole using 3D primitives, in a XY plane
 * aCenterPos = 3D position of the center
 * aRadius = radius of the circle (3D units)
 * aHoleRadius = radius of the hole (3D units)
 * Used to draw vias and round pads with round hole
 */
static void    Draw3D_FilledCircleWithHole( const S3D_VERTEX& aPos, double aRadius, double aHoleRadius );

/* draw a circle using 3D primitives, in a plane parallel to the XY plane
 * aCenterPos = 3D position of the center
 * aRadius = radius of the circle (3D units)
 * aWidth = width of the circle (3D units)
 * Does the same think as Draw3D_FilledCircleWithHole, just does not use the same parmeters
 * Used to draw circular segments
 */
static void    Draw3D_ThickCircle( const S3D_VERTEX& aCenterPos, double aRadius, double aWidth );

 /* returns true if aLayer should be displayed, false otherwise
  */
static bool    Is3DLayerEnabled( int aLayer );

 /* returns the Z orientation parmeter 1.0 or -1.0 for aLayer
  * Z orientation is 1.0 for all layers but "back" layers:
  *  LAYER_N_BACK , ADHESIVE_N_BACK, SOLDERPASTE_N_BACK ), SILKSCREEN_N_BACK
  * used to calculate the Z orientation parmeter for glNormal3f
  */
static GLfloat  Get3DLayer_Z_Orientation( int aLayer );

/* draw a cylinder using 3D primitives.
 * the cylinder axis is parallel to the Z axis
 * aCenterPos = 3D position of the axis cylinder
 * aRadius = radius of the cylinder (3D units)
 * aHeight = height of the cylinder (3D units)
 */
static void    Draw3D_ZaxisCylinder( const S3D_VERTEX& aPos, double aRadius, double aHeight );

/* draw an oblong cylinder (oblong hole) using 3D primitives.
 * the cylinder axis are parallel to the Z axis
 * aCenterPos = 3D position of the first axis cylinder
 * aEndx, aEndy = 3D position of the second axis cylinder
 * aRadius = radius of the cylinder (3D units)
 * aHeight = height of the cylinder (3D units)
 */
static void Draw3D_ZaxisOblongCylinder( const S3D_VERTEX& aCenterPos,
                                        double aEndx, double aEndy,
                                        double aRadius, double aHeight );


#ifndef CALLBACK
#define CALLBACK
#endif

// CALLBACK functions for GLU_TESS
static void CALLBACK tessBeginCB( GLenum which );
static void CALLBACK tessEndCB();
static void CALLBACK tessErrorCB( GLenum errorCode );
static void CALLBACK tessCPolyPt2Vertex( const GLvoid* data );
static void CALLBACK tesswxPoint2Vertex( const GLvoid* data );


void EDA_3D_CANVAS::Redraw( bool finish )
{
    // SwapBuffer requires the window to be shown before calling
    if( !IsShown() )
        return;

    SetCurrent( *m_glRC );

    // Set the OpenGL viewport according to the client size of this canvas.
    // This is done here rather than in a wxSizeEvent handler because our
    // OpenGL rendering context (and thus viewport setting) is used with
    // multiple canvases: If we updated the viewport in the wxSizeEvent
    // handler, changing the size of one canvas causes a viewport setting that
    // is wrong when next another canvas is repainted.
    const wxSize ClientSize = GetClientSize();

    // *MUST* be called *after*  SetCurrent( ):
    glViewport( 0, 0, ClientSize.x, ClientSize.y );

    InitGL();

    glMatrixMode( GL_MODELVIEW );    // position viewer
    // transformations
    GLfloat mat[4][4];

    // Translate motion first, so rotations don't mess up the orientation...
    glTranslatef( m_draw3dOffset.x, m_draw3dOffset.y, 0.0F );

    build_rotmatrix( mat, g_Parm_3D_Visu.m_Quat );
    glMultMatrixf( &mat[0][0] );

    glRotatef( g_Parm_3D_Visu.m_Rot[0], 1.0, 0.0, 0.0 );
    glRotatef( g_Parm_3D_Visu.m_Rot[1], 0.0, 1.0, 0.0 );
    glRotatef( g_Parm_3D_Visu.m_Rot[2], 0.0, 0.0, 1.0 );

    if( m_gllist )
    {
        glCallList( m_gllist );
    }
    else
    {
        CreateDrawGL_List();
    }

    glFlush();
    if( finish )
        glFinish();

    SwapBuffers();
}


GLuint EDA_3D_CANVAS::CreateDrawGL_List()
{
    PCB_BASE_FRAME* pcbframe = Parent()->Parent();
    BOARD* pcb = pcbframe->GetBoard();

    wxBusyCursor         dummy;

    m_gllist = glGenLists( 1 );

    EDA_RECT    bbbox = pcbframe->GetBoardBoundingBox();

    g_Parm_3D_Visu.m_BoardSettings = &pcb->GetDesignSettings();

    g_Parm_3D_Visu.m_BoardSize     = bbbox.GetSize();
    g_Parm_3D_Visu.m_BoardPos      = bbbox.Centre();

    g_Parm_3D_Visu.m_BoardPos.y    = -g_Parm_3D_Visu.m_BoardPos.y;
    g_Parm_3D_Visu.m_CopperLayersCount = pcb->GetCopperLayerCount();

    // Ensure the board has 2 sides for 3D views, because it is hard to find
    // a *really* single side board in the true life...
    if( g_Parm_3D_Visu.m_CopperLayersCount < 2 )
        g_Parm_3D_Visu.m_CopperLayersCount = 2;

    g_Parm_3D_Visu.m_BoardScale = 2.0 / std::max( g_Parm_3D_Visu.m_BoardSize.x,
                                                  g_Parm_3D_Visu.m_BoardSize.y );

    g_Parm_3D_Visu.m_EpoxyThickness = pcb->GetDesignSettings().GetBoardThickness()
                                   * g_Parm_3D_Visu.m_BoardScale;
    // Arbitrary choose a thickness for non copper layers:
    g_Parm_3D_Visu.m_NonCopperLayerThickness = g_Parm_3D_Visu.m_EpoxyThickness / 20;

    // Init  Z position of each layer
    // calculate z position for each copper layer
    int layer;
    int copper_layers_cnt = g_Parm_3D_Visu.m_CopperLayersCount;
    for( layer = 0; layer < copper_layers_cnt; layer++ )
    {
        g_Parm_3D_Visu.m_LayerZcoord[layer] =
            g_Parm_3D_Visu.m_EpoxyThickness * layer / (copper_layers_cnt - 1);
    }
    double zpos_copper_back  = g_Parm_3D_Visu.m_LayerZcoord[0];
    double zpos_copper_front = g_Parm_3D_Visu.m_EpoxyThickness;

    // Fill remaining unused copper layers and front layer zpos
    // with g_Parm_3D_Visu.m_EpoxyThickness
    for( ; layer <= LAST_COPPER_LAYER; layer++ )
    {
        g_Parm_3D_Visu.m_LayerZcoord[layer] = g_Parm_3D_Visu.m_EpoxyThickness;
    }

    // calculate z position for each non copper layer
    for( int layer_id = FIRST_NO_COPPER_LAYER; layer_id < NB_LAYERS; layer_id++ )
    {
        double zpos;
        switch( layer_id )
        {
            case ADHESIVE_N_BACK:
                zpos = zpos_copper_back -
                       4 * g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

            case ADHESIVE_N_FRONT:
                zpos = zpos_copper_front +
                       4 * g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

            case SOLDERPASTE_N_BACK:
                zpos = zpos_copper_back -
                       3 * g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

            case SOLDERPASTE_N_FRONT:
                zpos = zpos_copper_front +
                       3 * g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

            case SOLDERMASK_N_BACK:
                zpos = zpos_copper_back -
                       1 * g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

            case SOLDERMASK_N_FRONT:
                zpos = zpos_copper_front +
                       1 * g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

            case SILKSCREEN_N_BACK:
                zpos = zpos_copper_back -
                       2 * g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

            case SILKSCREEN_N_FRONT:
                zpos = zpos_copper_front +
                       2 * g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

            default:
                zpos = zpos_copper_front +
                       (layer_id - FIRST_NO_COPPER_LAYER + 5) *
                       g_Parm_3D_Visu.m_NonCopperLayerThickness;
                break;

        }

        g_Parm_3D_Visu.m_LayerZcoord[layer_id] = zpos;
    }

    glNewList( m_gllist, GL_COMPILE_AND_EXECUTE );

    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    // draw axis
    if (g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_AXIS])
    {
        glEnable( GL_COLOR_MATERIAL );
        SetGLColor( WHITE );
        glBegin( GL_LINES );
        glNormal3f( 0.0f, 0.0f, 1.0f );     // Normal is Z axis
        glVertex3f( 0.0f, 0.0f, 0.0f );
        glVertex3f( 1.0f, 0.0f, 0.0f );     // X axis
        glVertex3f( 0.0f, 0.0f, 0.0f );
        glVertex3f( 0.0f, -1.0f, 0.0f );    // Y axis
        glNormal3f( 1.0f, 0.0f, 0.0f );     // Normal is Y axis
        glVertex3f( 0.0f, 0.0f, 0.0f );
        glVertex3f( 0.0f, 0.0f, 0.3f );     // Z axis
        glEnd();
    }

    // Draw epoxy limits (do not use, works and test in progress)
    // TODO

    // move the board in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -g_Parm_3D_Visu.m_BoardPos.x * g_Parm_3D_Visu.m_BoardScale,
                  -g_Parm_3D_Visu.m_BoardPos.y * g_Parm_3D_Visu.m_BoardScale,
                  0.0F );

    glNormal3f( 0.0, 0.0, 1.0 ); // Normal is Z axis

    // draw tracks and vias :
    for( TRACK* track = pcb->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            Draw3D_Via( (SEGVIA*) track );
        else
        {
            int    layer = track->GetLayer();

            if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) )
                Draw3D_Track( track );
        }
    }

    if (g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_ZONE])
    {
        for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
        {
            int layer = pcb->GetArea( ii )->GetLayer();

            if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer )  )
                Draw3D_Zone( pcb->GetArea( ii ) );
        }
    }

    // draw graphic items
    EDA_ITEM* PtStruct;

    for( PtStruct = pcb->m_Drawings;  PtStruct != NULL;  PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_LINE_T:
        {
            DRAWSEGMENT* segment = (DRAWSEGMENT*) PtStruct;
            if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( segment->GetLayer() ) )
                Draw3D_DrawSegment( segment );
        }
            break;

        case PCB_TEXT_T:
        {
            TEXTE_PCB* text = (TEXTE_PCB*) PtStruct;
            if( Is3DLayerEnabled( text->GetLayer() ) )
                Draw3D_DrawText( text );
        }
            break;

        default:
            break;
        }
    }

    // draw footprints
    MODULE* Module = pcb->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
        Module->Draw3D( this );

    // Draw grid
    if( g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_GRID] )
    DrawGrid( g_Parm_3D_Visu.m_3D_Grid );

    glEndList();

    // Test for errors
    CheckGLError();

    return m_gllist;
}

/* Draw a zone (solid copper areas in aZone)
 */
void EDA_3D_CANVAS::Draw3D_Zone( ZONE_CONTAINER* aZone )
{
    int    layer = aZone->GetLayer();
    int color = g_ColorsSettings.GetLayerColor( layer );

    if( layer == LAST_COPPER_LAYER )
        layer = g_Parm_3D_Visu.m_CopperLayersCount - 1;

    double zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];
    double width  = aZone->m_ZoneMinThickness * g_Parm_3D_Visu.m_BoardScale;

    SetGLColor( color );
    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );

    if( aZone->m_FillMode == 0 )
    {
        // solid polygons only are used to fill areas
        if( aZone->GetFilledPolysList().size() > 3 )
        {
            Draw3D_SolidPolygonsInZones( aZone );
        }
    }
    else
    {
        // segments are used to fill areas
        for( unsigned iseg = 0; iseg < aZone->m_FillSegmList.size(); iseg++ )
        {
            double ox = aZone->m_FillSegmList[iseg].m_Start.x * g_Parm_3D_Visu.m_BoardScale;
            double oy = aZone->m_FillSegmList[iseg].m_Start.y * g_Parm_3D_Visu.m_BoardScale;
            double fx = aZone->m_FillSegmList[iseg].m_End.x * g_Parm_3D_Visu.m_BoardScale;
            double fy = aZone->m_FillSegmList[iseg].m_End.y * g_Parm_3D_Visu.m_BoardScale;
            S3D_VERTEX pos( ox, -oy, zpos );
            Draw3D_FilledSegment( pos, fx, -fy, width );
        }
    }

    // Draw copper area outlines
    std::vector<CPolyPt> polysList = aZone->GetFilledPolysList();

    if( polysList.size() == 0 )
        return;

    if( aZone->m_ZoneMinThickness <= 1 )
        return;

    int      imax = polysList.size() - 1;
    CPolyPt* firstcorner = &polysList[0];
    CPolyPt* begincorner = firstcorner;

    for( int ic = 1; ic <= imax; ic++ )
    {
        CPolyPt* endcorner = &polysList[ic];

        if( begincorner->m_utility == 0 )
        {
            // Draw only basic outlines, not extra segments
            double ox = begincorner->x * g_Parm_3D_Visu.m_BoardScale;
            double oy = begincorner->y * g_Parm_3D_Visu.m_BoardScale;
            double fx = endcorner->x * g_Parm_3D_Visu.m_BoardScale;
            double fy = endcorner->y * g_Parm_3D_Visu.m_BoardScale;
            S3D_VERTEX pos( ox, -oy, zpos );
            Draw3D_FilledSegment( pos, fx, -fy, width );
        }

        if( (endcorner->end_contour) || (ic == imax) )
        {
            // the last corner of a filled area is found: draw it
            if( endcorner->m_utility == 0 )
            {
                // Draw only basic outlines, not extra segments
                double ox = endcorner->x * g_Parm_3D_Visu.m_BoardScale;
                double oy = endcorner->y * g_Parm_3D_Visu.m_BoardScale;
                double fx = firstcorner->x * g_Parm_3D_Visu.m_BoardScale;
                double fy = firstcorner->y * g_Parm_3D_Visu.m_BoardScale;
                S3D_VERTEX pos( ox, -oy, zpos );
                Draw3D_FilledSegment( pos, fx, -fy, width );
            }

            ic++;

            if( ic < imax - 1 )
                begincorner = firstcorner = &polysList[ic];
        }
        else
        {
            begincorner = endcorner;
        }
    }
}


// draw mm grid..
void EDA_3D_CANVAS::DrawGrid( double aGriSizeMM )
{
    double zpos = -g_Parm_3D_Visu.m_NonCopperLayerThickness/2;
    int gridcolor = DARKGRAY;           // Color of grid lines
    int gridcolor_marker = LIGHTGRAY;   // Color of grid lines every 5 lines
    double scale = g_Parm_3D_Visu.m_BoardScale;

    glNormal3f( 0.0, 0.0, 1.0 );

    wxSize brd_size = g_Parm_3D_Visu.m_BoardSize;
    wxPoint brd_center_pos = g_Parm_3D_Visu.m_BoardPos;
    NEGATE( brd_center_pos.y );

    int xsize  = std::max( brd_size.x, Millimeter2iu( 100 ) );
    int ysize  = std::max( brd_size.y, Millimeter2iu( 100 ) );

    // Grid limits, in 3D units
    double xmin = (brd_center_pos.x - xsize/2) * scale;
    double xmax = (brd_center_pos.x + xsize/2) * scale;
    double ymin = (brd_center_pos.y - ysize/2) * scale;
    double ymax = (brd_center_pos.y + ysize/2) * scale;
    double zmin = Millimeter2iu( -50 ) * scale;
    double zmax = Millimeter2iu( 100 ) * scale;

    // Draw horizontal grid centered on 3D origin (center of the board)
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            SetGLColor( gridcolor );
        else
            SetGLColor( gridcolor_marker );

        int delta = KiROUND( ii * aGriSizeMM * IU_PER_MM );

        if( delta <= xsize/2 )    // Draw grid lines parallel to X axis
        {
            glBegin(GL_LINES);
            glVertex3f( (brd_center_pos.x + delta) * scale, -ymin, zpos );
            glVertex3f( (brd_center_pos.x + delta) * scale, -ymax, zpos );
            glEnd();

            if( ii != 0 )
            {
                glBegin(GL_LINES);
                glVertex3f( (brd_center_pos.x - delta) * scale, -ymin, zpos );
                glVertex3f( (brd_center_pos.x - delta) * scale, -ymax, zpos );
                glEnd();
            }
        }

        if( delta <= ysize/2 )    // Draw grid lines parallel to Y axis
        {
            glBegin(GL_LINES);
            glVertex3f( xmin, -(brd_center_pos.y + delta) * scale, zpos );
            glVertex3f( xmax, -(brd_center_pos.y + delta) * scale, zpos );
            glEnd();
            if( ii != 0 )
            {
                glBegin(GL_LINES);
                glVertex3f( xmin, -(brd_center_pos.y - delta) * scale, zpos );
                glVertex3f( xmax, -(brd_center_pos.y - delta) * scale, zpos );
                glEnd();
            }
        }

        if( ( delta > ysize/2 ) && ( delta > xsize/2 ) )
            break;
    }

    // Draw vertical grid n Z axis
    glNormal3f( 0.0, -1.0, 0.0 );

    // Draw vertical grid lines (parallel to Z axis)
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            SetGLColor( gridcolor );
        else
            SetGLColor( gridcolor_marker );

        double delta = ii * aGriSizeMM * IU_PER_MM;

        glBegin(GL_LINES);
        glVertex3f( (brd_center_pos.x + delta) * scale, -brd_center_pos.y * scale, zmin );
        glVertex3f( (brd_center_pos.x + delta) * scale, -brd_center_pos.y * scale, zmax );
        glEnd();

        if( ii != 0 )
        {
            glBegin(GL_LINES);
            glVertex3f( (brd_center_pos.x - delta) * scale, -brd_center_pos.y * scale, zmin );
            glVertex3f( (brd_center_pos.x - delta) * scale, -brd_center_pos.y * scale, zmax );
            glEnd();
        }

        if( delta > xsize/2 )
            break;
    }

    // Draw horizontal grid lines on Z axis
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            SetGLColor( gridcolor );
        else
            SetGLColor( gridcolor_marker );
        double delta = ii * aGriSizeMM * IU_PER_MM  * scale;

        if( delta <= zmax )
        {   // Draw grid lines on Z axis (positive Z axis coordinates)
            glBegin(GL_LINES);
            glVertex3f(xmin, -brd_center_pos.y * scale, delta);
            glVertex3f(xmax, -brd_center_pos.y * scale, delta);
            glEnd();
        }

        if( delta <= -zmin && ( ii != 0 ) )
        {   // Draw grid lines on Z axis (negative Z axis coordinates)
            glBegin(GL_LINES);
            glVertex3f(xmin, -brd_center_pos.y * scale, -delta);
            glVertex3f(xmax, -brd_center_pos.y * scale, -delta);
            glEnd();
        }

        if( ( delta > zmax ) && ( delta > -zmin ) )
            break;
    }

}

void EDA_3D_CANVAS::Draw3D_Track( TRACK* aTrack )
{
    int    layer = aTrack->GetLayer();
    int color = g_ColorsSettings.GetLayerColor( layer );

    if( layer == LAST_COPPER_LAYER )
        layer = g_Parm_3D_Visu.m_CopperLayersCount - 1;

    double zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

    SetGLColor( color );
    glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );

    double w  = aTrack->m_Width * g_Parm_3D_Visu.m_BoardScale;
    double ox = aTrack->m_Start.x * g_Parm_3D_Visu.m_BoardScale;
    double oy = aTrack->m_Start.y * g_Parm_3D_Visu.m_BoardScale;
    double fx = aTrack->m_End.x * g_Parm_3D_Visu.m_BoardScale;
    double fy = aTrack->m_End.y * g_Parm_3D_Visu.m_BoardScale;

    S3D_VERTEX pos( ox, -oy, zpos );
    Draw3D_FilledSegment( pos, fx, -fy, w );
}


void EDA_3D_CANVAS::Draw3D_SolidPolygonsInZones( ZONE_CONTAINER* aZone )
{
    double zpos;
    int    layer = aZone->GetLayer();

    int color = g_ColorsSettings.GetLayerColor( layer );

    if( layer == LAST_COPPER_LAYER )
        layer = g_Parm_3D_Visu.m_CopperLayersCount - 1;

    zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];
    g_Parm_3D_Visu.m_ActZpos = zpos;

    SetGLColor( color );
    glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );

    GLUtesselator* tess = gluNewTess();
    gluTessCallback( tess, GLU_TESS_BEGIN, ( void (CALLBACK*)() )tessBeginCB );
    gluTessCallback( tess, GLU_TESS_END, ( void (CALLBACK*)() )tessEndCB );
    gluTessCallback( tess, GLU_TESS_ERROR, ( void (CALLBACK*)() )tessErrorCB );
    gluTessCallback( tess, GLU_TESS_VERTEX, ( void (CALLBACK*)() )tessCPolyPt2Vertex );

    GLdouble v_data[3];
    v_data[2] = zpos;

    //gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);

    // Draw solid areas contained in this zone
    int StartContour = 1;

    std::vector<CPolyPt> polysList = aZone->GetFilledPolysList();
    for( unsigned ii = 0; ii < polysList.size(); ii++ )
    {
        if( StartContour == 1 )
        {
            gluTessBeginPolygon( tess, NULL );
            gluTessBeginContour( tess );
            StartContour = 0;
        }

        v_data[0] = polysList[ii].x * g_Parm_3D_Visu.m_BoardScale;
        v_data[1] = -polysList[ii].y * g_Parm_3D_Visu.m_BoardScale;
        gluTessVertex( tess, v_data, &polysList[ii] );

        if( polysList[ii].end_contour == 1 )
        {
            gluTessEndContour( tess );
            gluTessEndPolygon( tess );
            StartContour = 1;
        }
    }

    gluDeleteTess( tess );
}


void EDA_3D_CANVAS::Draw3D_Via( SEGVIA* via )
{
    double x, y, r, hole;
    int    layer, top_layer, bottom_layer;
    double zpos, height;
    int    color;

    r     = via->m_Width * g_Parm_3D_Visu.m_BoardScale / 2;
    hole  = via->GetDrillValue();
    hole *= g_Parm_3D_Visu.m_BoardScale / 2;
    x     = via->m_Start.x * g_Parm_3D_Visu.m_BoardScale;
    y     = via->m_Start.y * g_Parm_3D_Visu.m_BoardScale;

    via->ReturnLayerPair( &top_layer, &bottom_layer );

    // Drawing filled circles:
    for( layer = bottom_layer; layer < g_Parm_3D_Visu.m_CopperLayersCount; layer++ )
    {
        zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

        if( layer < g_Parm_3D_Visu.m_CopperLayersCount - 1 )
        {
            if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) == false )
                continue;

            color = g_ColorsSettings.GetLayerColor( layer );
        }
        else
        {
            if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( LAYER_N_FRONT ) == false )
                continue;

            color = g_ColorsSettings.GetLayerColor( LAYER_N_FRONT );
        }

        SetGLColor( color );

        glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );

        if( layer == LAYER_N_BACK )
            zpos = zpos - 5 * g_Parm_3D_Visu.m_BoardScale;
        else
            zpos = zpos + 5 * g_Parm_3D_Visu.m_BoardScale;

        S3D_VERTEX pos( x, -y, zpos);
        Draw3D_FilledCircleWithHole( pos, r, hole );

        if( layer >= top_layer )
            break;
    }

    // Drawing hole:
    color = g_ColorsSettings.GetItemColor( VIAS_VISIBLE + via->m_Shape );
    SetGLColor( color );
    height = g_Parm_3D_Visu.m_LayerZcoord[top_layer] - g_Parm_3D_Visu.m_LayerZcoord[bottom_layer];
    S3D_VERTEX position( x, -y, g_Parm_3D_Visu.m_LayerZcoord[bottom_layer] );
    Draw3D_ZaxisCylinder( position, hole, height );
}


void EDA_3D_CANVAS::Draw3D_DrawSegment( DRAWSEGMENT* segment )
{
    int    layer = segment->GetLayer();
    int color = g_ColorsSettings.GetLayerColor( layer );

    SetGLColor( color );

    double w  = segment->GetWidth() * g_Parm_3D_Visu.m_BoardScale;
    double x  = segment->GetStart().x * g_Parm_3D_Visu.m_BoardScale;
    double y  = segment->GetStart().y * g_Parm_3D_Visu.m_BoardScale;
    double xf = segment->GetEnd().x * g_Parm_3D_Visu.m_BoardScale;
    double yf = segment->GetEnd().y * g_Parm_3D_Visu.m_BoardScale;

    if( layer == EDGE_N )
    {
        for( layer = 0; layer < g_Parm_3D_Visu.m_CopperLayersCount; layer++ )
        {
            glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );
            double zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

            switch( segment->GetShape() )
            {
            case S_ARC:
            {
                S3D_VERTEX pos( xf, -yf, zpos );
                Draw3D_ArcSegment( pos, x, -y, segment->GetAngle(), w );
            }
                break;

            case S_CIRCLE:
            {
                S3D_VERTEX pos( x, -y, zpos );
                Draw3D_ThickCircle( pos, hypot( x - xf, y - yf ), w );
            }
                break;

            default:
            {
                S3D_VERTEX pos( x, -y, zpos );
                Draw3D_FilledSegment( pos, xf, -yf, w );
            }
                break;
            }
        }
    }
    else
    {
        glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );
        double zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

        if( Is3DLayerEnabled( layer ) )
        {
            switch( segment->GetShape() )
            {
            case S_ARC:
            {
                S3D_VERTEX pos( xf, -yf, zpos );
                Draw3D_ArcSegment( pos, x, -y, segment->GetAngle(), w );
            }
                break;

            case S_CIRCLE:
            {
                S3D_VERTEX pos( x, -y, zpos );
                Draw3D_ThickCircle( pos, hypot( x - xf, y - yf ), w );
            }
                break;

            default:
            {
                S3D_VERTEX pos( x, -y, zpos );
                Draw3D_FilledSegment( pos , xf, -yf, w );
            }
                break;
            }
        }
    }
}


// These variables are used in Draw3dTextSegm.
// But Draw3dTextSegm is a call back function, so we cannot send them as arguments,
// so they are static.
static double s_Text3DWidth, s_Text3DZPos;

// This is a call back function, used by DrawGraphicText to draw the 3D text shape:
static void Draw3dTextSegm( int x0, int y0, int xf, int yf )
{
    double startx = x0 * g_Parm_3D_Visu.m_BoardScale;
    double starty = y0 * g_Parm_3D_Visu.m_BoardScale;
    double endx   = xf * g_Parm_3D_Visu.m_BoardScale;
    double endy   = yf * g_Parm_3D_Visu.m_BoardScale;

    S3D_VERTEX pos( startx, -starty, s_Text3DZPos );
    Draw3D_FilledSegment( pos, endx, -endy, s_Text3DWidth );
}


void EDA_3D_CANVAS::Draw3D_DrawText( TEXTE_PCB* text )
{
    int layer = text->GetLayer();
    int color = g_ColorsSettings.GetLayerColor( layer );

    SetGLColor( color );
    s_Text3DZPos  = g_Parm_3D_Visu.m_LayerZcoord[layer];
    s_Text3DWidth = text->GetThickness() * g_Parm_3D_Visu.m_BoardScale;
    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );
    wxSize size = text->m_Size;

    if( text->m_Mirror )
        NEGATE( size.x );

    if( text->m_MultilineAllowed )
    {
        wxPoint        pos  = text->m_Pos;
        wxArrayString* list = wxStringSplit( text->m_Text, '\n' );
        wxPoint        offset;

        offset.y = text->GetInterline();

        RotatePoint( &offset, text->GetOrientation() );

        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            DrawGraphicText( NULL, NULL, pos, (EDA_COLOR_T) color,
                             txt, text->GetOrientation(), size,
                             text->m_HJustify, text->m_VJustify,
                             text->GetThickness(), text->m_Italic,
                             true, Draw3dTextSegm );
            pos += offset;
        }

        delete list;
    }
    else
    {
        DrawGraphicText( NULL, NULL, text->m_Pos, (EDA_COLOR_T) color,
                         text->m_Text, text->GetOrientation(), size,
                         text->m_HJustify, text->m_VJustify,
                         text->GetThickness(), text->m_Italic,
                         true,
                         Draw3dTextSegm );
    }
}


void MODULE::Draw3D( EDA_3D_CANVAS* glcanvas )
{
    D_PAD* pad = m_Pads;

    // Draw pads
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    glNormal3f( 0.0, 0.0, 1.0 ); // Normal is Z axis

    for( ; pad != NULL; pad = pad->Next() )
    {
        pad->Draw3D( glcanvas );
    }

    // Draw module shape: 3D shape if exists (or module outlines if not exists)
    S3D_MASTER* Struct3D  = m_3D_Drawings;
    bool        As3dShape = false;

    if (g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_MODULE])
    {
        glPushMatrix();

        glTranslatef( m_Pos.x * g_Parm_3D_Visu.m_BoardScale,
                      -m_Pos.y * g_Parm_3D_Visu.m_BoardScale,
                      g_Parm_3D_Visu.m_LayerZcoord[m_Layer] );

        if( m_Orient )
        {
            glRotatef( (double) m_Orient / 10, 0.0, 0.0, 1.0 );
        }

        if( m_Layer == LAYER_N_BACK )
        {
            glRotatef( 180.0, 0.0, 1.0, 0.0 );
            glRotatef( 180.0, 0.0, 0.0, 1.0 );
        }

        DataScale3D = g_Parm_3D_Visu.m_BoardScale * UNITS3D_TO_UNITSPCB;

        for( ; Struct3D != NULL; Struct3D = Struct3D->Next() )
        {
            if( !Struct3D->m_Shape3DName.IsEmpty() )
            {
                As3dShape = true;
                Struct3D->ReadData();
            }
        }

        glPopMatrix();
    }

    EDA_ITEM* Struct = m_Drawings;
    glNormal3f( 0.0, 0.0, 1.0 ); // Normal is Z axis

    for( ; Struct != NULL; Struct = Struct->Next() )
    {
        switch( Struct->Type() )
        {
        case PCB_MODULE_TEXT_T:
            break;

        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* edge = (EDGE_MODULE*) Struct;

            // Draw module edges when no 3d shape exists.
            // Always draw pcb edges.
            if( !As3dShape || edge->GetLayer() == EDGE_N )
                edge->Draw3D( glcanvas );
        }
        break;

        default:
            break;
        }
    }
}


void EDGE_MODULE::Draw3D( EDA_3D_CANVAS* glcanvas )
{
    wxString s;
    int      dx, dy;
    double   x, y, fx, fy, w, zpos;

    if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( m_Layer ) == false )
        return;

    int color = g_ColorsSettings.GetLayerColor( m_Layer );

    SetGLColor( color );

    dx   = m_End.x;
    dy   = m_End.y;
    w    = m_Width * g_Parm_3D_Visu.m_BoardScale;
    x    = m_Start.x * g_Parm_3D_Visu.m_BoardScale;
    y    = m_Start.y * g_Parm_3D_Visu.m_BoardScale;
    fx   = dx * g_Parm_3D_Visu.m_BoardScale;
    fy   = dy * g_Parm_3D_Visu.m_BoardScale;


    if( m_Layer == EDGE_N )
    {
        for( int layer = 0; layer < g_Parm_3D_Visu.m_CopperLayersCount; layer++ )
        {
            glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );
            zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

            switch( m_Shape )
            {
            case S_SEGMENT:
            {
                S3D_VERTEX pos( x, -y, zpos );
                Draw3D_FilledSegment( pos, fx, -fy, w );
            }
                break;

            case S_CIRCLE:
            {
                S3D_VERTEX pos( x, -y, zpos );
                Draw3D_ThickCircle( pos, hypot( x - fx, y - fy ), w);
            }
                break;

            case S_ARC:
            {
                S3D_VERTEX pos( fx, -fy, zpos );
                Draw3D_ArcSegment( pos, x, -y, (double) m_Angle, w );
            }
                break;

            case S_POLYGON:
            {
                // We must compute true coordinates from m_PolyPoints
                // which are relative to module position and module orientation = 0
                std::vector<wxPoint> points = m_PolyPoints;
                MODULE* module = (MODULE*) m_Parent;

                if( module == NULL )
                    break;

                for( unsigned ii = 0; ii < points.size(); ii++ )
                {
                    wxPoint& pt = points[ii];

                    RotatePoint( &pt.x, &pt.y, module->GetOrientation() );
                    pt += module->m_Pos;
                }

                glcanvas->Draw3D_Polygon( points, zpos );
            }
            break;

            default:
                s.Printf( wxT( "Error: Shape nr %d not implemented!\n" ), m_Shape );
                D( printf( "%s", TO_UTF8( s ) ); )
                break;
            }
        }
    }
    else
    {
        glNormal3f( 0.0, 0.0, (m_Layer == LAYER_N_BACK) ? -1.0 : 1.0 );
        zpos = g_Parm_3D_Visu.m_LayerZcoord[m_Layer];

        switch( m_Shape )
        {
        case S_SEGMENT:
        {
            S3D_VERTEX pos( x, -y, zpos );
            Draw3D_FilledSegment( pos, fx, -fy, w );
        }
            break;

        case S_CIRCLE:
        {
            S3D_VERTEX pos( x, -y, zpos );
            Draw3D_ThickCircle( pos, hypot( x - fx, y - fy ), w );
        }
            break;

        case S_ARC:
        {
            S3D_VERTEX pos( fx, -fy, zpos );
            Draw3D_ArcSegment( pos, x, -y, (double) m_Angle, w );
        }
            break;

        case S_POLYGON:
        {
            // We must compute true coordinates from m_PolyPoints
            // which are relative to module position and module orientation = 0
            std::vector<wxPoint> points = m_PolyPoints;
            MODULE* module = (MODULE*) m_Parent;

            if( module == NULL )
                break;

            for( unsigned ii = 0; ii < points.size(); ii++ )
            {
                wxPoint& pt = points[ii];

                RotatePoint( &pt.x, &pt.y, module->GetOrientation() );
                pt += module->m_Pos;
            }

            glcanvas->Draw3D_Polygon( points, zpos );
        }
        break;

        default:
            s.Printf( wxT( "Error: Shape nr %d not implemented!\n" ), m_Shape );
            D( printf( "%s", TO_UTF8( s ) ); )
            break;
        }
    }
}


// Draw 3D pads.
void D_PAD::Draw3D( EDA_3D_CANVAS* glcanvas )
{
    int ii, ll, layer, nlmax;
    int ux0, uy0,
        dx, dx0, dy, dy0,
        delta_cx, delta_cy,
        xc, yc;
    int     angle;
    double  zpos;
    double  x, y, r, w;
    bool    Oncu, Oncmp, Both;
    int     color;

    double scale = g_Parm_3D_Visu.m_BoardScale;
    double holeX = (double) m_Drill.x * scale / 2;
    double holeY = (double) m_Drill.y * scale / 2;
    double hole  = fmin( holeX, holeY );

    // Calculate the center of the pad.
    wxPoint shape_pos = ReturnShapePos();
    ux0 = shape_pos.x;
    uy0 = shape_pos.y;
    xc  = ux0;
    yc  = uy0;

    dx = dx0 = m_Size.x >> 1;
    dy = dy0 = m_Size.y >> 1;

    angle  = m_Orient;
    double drillx = m_Pos.x * scale;
    double drilly = m_Pos.y * scale;
    double height = g_Parm_3D_Visu.m_LayerZcoord[LAYER_N_FRONT] -
                    g_Parm_3D_Visu.m_LayerZcoord[LAYER_N_BACK];

    // Draw the pad hole
    if( holeX && holeY )
    {
        SetGLColor( DARKGRAY );
        S3D_VERTEX position( drillx, -drilly, g_Parm_3D_Visu.m_LayerZcoord[LAYER_N_BACK] );

        if( holeX == holeY )    // usual round hole
            Draw3D_ZaxisCylinder( position, hole, height );
        else    // Oval hole
        {
                double ldx, ldy;

                if( holeX > holeY )    // Horizontal oval
                {
                    ldx = holeX - holeY;
                    ldy = 0;
                    w = m_Size.y * scale;
                }
                else    // Vertical oval
                {
                    ldx = 0;
                    ldy = holeY - holeX;
                    w = m_Size.x * scale;
                }

                RotatePoint( &ldx, &ldy, angle );

                {
                    double ox  = ux0 * scale + ldx;
                    double oy  = uy0 * scale + ldy;
                    double fx  = ux0  * scale - ldx;
                    double fy  = uy0 * scale - ldy;

                    S3D_VERTEX position( ox, -oy, g_Parm_3D_Visu.m_LayerZcoord[LAYER_N_BACK] );
                    Draw3D_ZaxisOblongCylinder( position, fx, -fy, hole, height );
                }
        }
    }

    glNormal3f( 0.0, 0.0, 1.0 ); // Normal is Z axis
    nlmax = g_Parm_3D_Visu.m_CopperLayersCount - 1;
    Oncu  = (m_layerMask & LAYER_BACK) ? true : false;
    Oncmp = (m_layerMask & LAYER_FRONT) ? true : false;
    Both  = Oncu && Oncmp;

    switch( m_PadShape & 0x7F )
    {
    case PAD_CIRCLE:
        x = xc * scale;
        y = yc * scale;
        r = (double) dx * scale;

        for( layer = FIRST_COPPER_LAYER; layer <= LAST_COPPER_LAYER; layer++ )
        {
            if( layer && (layer == nlmax) )
                layer = LAYER_N_FRONT;

            if( (layer == LAYER_N_FRONT) && !Oncmp )
                continue;

            if( (layer == LAYER_N_BACK) && !Oncu )
                continue;

            if( (layer > FIRST_COPPER_LAYER) && (layer < LAST_COPPER_LAYER) && !Both )
                continue;

            color = g_ColorsSettings.GetLayerColor( layer );

            if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) == false )
                continue;

            SetGLColor( color );
            glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );
            zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

            if( layer == LAYER_N_BACK )
                zpos = zpos - 5 * g_Parm_3D_Visu.m_BoardScale;
            else
                zpos = zpos + 5 * g_Parm_3D_Visu.m_BoardScale;

            S3D_VERTEX position( x, -y, zpos );
            Draw3D_FilledCircleWithHole( position, r, hole );
        }

        break;

    case PAD_OVAL:
        if( dx > dy ) // Horizontal ellipse
        {
            delta_cx = dx - dy;
            delta_cy = 0;
            w = m_Size.y * scale;
        }
        else // Vertical ellipse
        {
            delta_cx = 0;
            delta_cy = dy - dx;
            w = m_Size.x * scale;
        }

        RotatePoint( &delta_cx, &delta_cy, angle );

        {
            double ox, oy, fx, fy;
            ox = (double) ( ux0 + delta_cx ) * scale;
            oy = (double) ( uy0 + delta_cy ) * scale;
            fx = (double) ( ux0 - delta_cx ) * scale;
            fy = (double) ( uy0 - delta_cy ) * scale;

            for( layer = FIRST_COPPER_LAYER; layer <= LAST_COPPER_LAYER; layer++ )
            {
                if( layer && (layer == nlmax) )
                    layer = LAYER_N_FRONT;

                if( (layer == LAYER_N_FRONT) && !Oncmp )
                    continue;

                if( (layer == LAYER_N_BACK) && !Oncu )
                    continue;

                if( (layer > FIRST_COPPER_LAYER) && (layer < LAST_COPPER_LAYER) && !Both )
                    continue;

                color = g_ColorsSettings.GetLayerColor( layer );
                glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );

                if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) == false )
                    continue;

                SetGLColor( color );
                zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

                if( layer == LAYER_N_BACK )
                    zpos = zpos - 5 * g_Parm_3D_Visu.m_BoardScale;
                else
                    zpos = zpos + 5 * g_Parm_3D_Visu.m_BoardScale;

                Draw3D_SegmentWithHole( ox, -oy, fx, -fy, w, drillx, -drilly, hole, zpos );
            }
        }

        break;

    case PAD_RECT:
    case PAD_TRAPEZOID:
    {
        wxPoint  coord[5];
        wxRealPoint  fcoord[8], f_hole_coord[8];
        BuildPadPolygon( coord, wxSize(0,0), angle );

        for( ii = 0; ii < 4; ii++ )
        {
            coord[ii].x += ux0;
            coord[ii].y += uy0;
            ll = ii * 2;
            fcoord[ll].x = coord[ii].x *scale;
            fcoord[ll].y = coord[ii].y *scale;
        }

        for( ii = 0; ii < 7; ii += 2 )
        {
            ll = ii + 2;

            if( ll > 7 )
                ll -= 8;

            fcoord[ii + 1].x = (fcoord[ii].x + fcoord[ll].x) / 2;
            fcoord[ii + 1].y = (fcoord[ii].y + fcoord[ll].y) / 2;
        }

        for( ii = 0; ii < 8; ii++ )
        {
            f_hole_coord[ii].x = -hole * 0.707;
            f_hole_coord[ii].y = hole * 0.707;
            RotatePoint( &f_hole_coord[ii].x, &f_hole_coord[ii].y, angle - (ii * 450) );
            f_hole_coord[ii].x += drillx;
            f_hole_coord[ii].y += drilly;
        }

        for( layer = FIRST_COPPER_LAYER; layer <= LAST_COPPER_LAYER; layer++ )
        {
            if( layer && (layer == nlmax) )
                layer = LAYER_N_FRONT;

            if( (layer == LAYER_N_FRONT) && !Oncmp )
                continue;

            if( (layer == LAYER_N_BACK) && !Oncu )
                continue;

            if( (layer > FIRST_COPPER_LAYER) && (layer < LAST_COPPER_LAYER) && !Both )
                continue;

            color = g_ColorsSettings.GetLayerColor( layer );
            glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );

            if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) == false )
                continue;

            SetGLColor( color );
            zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

            if( layer == LAYER_N_BACK )
                zpos = zpos - 5 * g_Parm_3D_Visu.m_BoardScale;
            else
                zpos = zpos + 5 * g_Parm_3D_Visu.m_BoardScale;

            glBegin( GL_QUAD_STRIP );

            for( ii = 0; ii < 8; ii++ )
            {
                glVertex3f( f_hole_coord[ii].x, -f_hole_coord[ii].y, zpos );
                glVertex3f( fcoord[ii].x, -fcoord[ii].y, zpos );
            }

            glVertex3f( f_hole_coord[0].x, -f_hole_coord[0].y, zpos );
            glVertex3f( fcoord[0].x, -fcoord[0].y, zpos );
            glEnd();
        }
    }
    break;

    default:
        break;
    }
}


void SetGLColor( int color )
{
    double       red, green, blue;
    StructColors colordata = ColorRefs[color & MASKCOLOR];

    red   = colordata.m_Red / 255.0;
    blue  = colordata.m_Blue / 255.0;
    green = colordata.m_Green / 255.0;
    glColor3f( red, green, blue );
}


void Draw3D_FilledCircleWithHole( const S3D_VERTEX& aPos, double radius, double hole_radius )
{
    const int slice = 16;
    const int rot_angle = ANGLE_INC(slice);

    glBegin( GL_QUAD_STRIP );

    for( int ii = 0; ii <= slice; ii++ )
    {
        double x = hole_radius;
        double y = 0.0;
        RotatePoint( &x, &y, ii * rot_angle );
        glVertex3f( x + aPos.x, y + aPos.y, aPos.z );
        x = radius;
        y = 0.0;
        RotatePoint( &x, &y, ii * rot_angle );
        glVertex3f( x + aPos.x, y + aPos.y, aPos.z );
    }

    glEnd();
}

static void Draw3D_ZaxisCylinder( const S3D_VERTEX& aPos, double aRadius, double aHeight )
{
    const int slice = 12;

    std::vector< S3D_VERTEX > coords;
    coords.resize( 4 );

    double     tmp = DataScale3D;
    DataScale3D = 1.0; // Coordinate is already in range for Set_Object_Data();

    coords[0].x = coords[1].x = aPos.x + aRadius;
    coords[0].y = coords[1].y = aPos.y;
    coords[0].z = coords[3].z = aPos.z;
    coords[1].z = coords[2].z = aPos.z + aHeight;

    for( int ii = 0; ii <= slice; ii++ )
    {
        double x = aRadius;
        double y = 0.0;
        RotatePoint( &x, &y, ii * ANGLE_INC(slice) );
        coords[2].x = coords[3].x = aPos.x + x;
        coords[2].y = coords[3].y = aPos.y + y;
        Set_Object_Data( coords );
        coords[0].x = coords[2].x;
        coords[0].y = coords[2].y;
        coords[1].x = coords[3].x;
        coords[1].y = coords[3].y;
    }

    glNormal3f( 0.0, 0.0, 1.0 ); // Normal is Z axis
    DataScale3D = tmp;
}


// Draw a polygon similar to a segment has rounded tips
static void Draw3D_FilledSegment( const S3D_VERTEX& aStartPos,
                                  double aEndx, double aEndy, double aWidth )
{
    double dx, dy, x, y, firstx = 0, firsty = 0;
    int    ii, angle;

    // Calculate the coordinates of the segment assumed horizontal.
    // Then turn the strips of the desired angle.
    dx    = aEndx - aStartPos.x;
    dy    = aEndy - aStartPos.y;
    angle = (int) ( ( atan2( dy, dx ) * 1800 / M_PI ) + 0.5 );

    RotatePoint( &dx, &dy, angle );
    aWidth /= 2;

    glBegin( GL_POLYGON );

    // Trace the flare to right (1st half polygon at the end of the segment)
    const int slice = 16;
    for( ii = 0; ii <= slice/2; ii++ )
    {
        x = 0.0;
        y = -aWidth;
        RotatePoint( &x, &y, -ii * ANGLE_INC(slice) );
        x += dx;
        RotatePoint( &x, &y, -angle );
        glVertex3f( aStartPos.x + x, aStartPos.y + y, aStartPos.z );

        if( ii == 0 )
        {
            firstx = aStartPos.x + x;
            firsty = aStartPos.y + y;
        }
    }

    // Rounding the left (2nd half polygon is the origin of the segment)
    for( ii = 0; ii <= slice/2; ii++ )
    {
        int jj = ii * ANGLE_INC(slice);
        x = 0.0;
        y = aWidth;
        RotatePoint( &x, &y, -angle - jj );
        glVertex3f( aStartPos.x + x, aStartPos.y + y, aStartPos.z );
    }

    glVertex3f( firstx, firsty, aStartPos.z );
    glEnd();
}


/* Draw a polygon similar to a segment ends with round hole
 */
static void Draw3D_SegmentWithHole( double startx, double starty,
                                          double endx, double endy,
                                          double width, double holex,
                                          double holey, double holeradius,
                                          double zpos )
{
    double x, y, xin, yin;
    double firstx = 0, firsty = 0, firstxin = 0, firstyin = 0;
    int    ii, angle, theta;

    // Calculate the coordinates of the segment assumed horizontal
    // Then turn the strips of the desired angle
    // All calculations are done with startx, starty as local origin
    endx  -= startx;
    endy  -= starty;
    holex -= startx;
    holey -= starty;
    angle  = (int) ( ( atan2( endy, endx ) * 1800 / M_PI ) + 0.5 );

    RotatePoint( &endx, &endy, angle );
    RotatePoint( &holex, &holey, angle );
    width /= 2;

    glBegin( GL_QUAD_STRIP );

    // Path of the flare to right (1st half polygon at the end of the segment)
    // around the half-hole drilling
    for( ii = 0; ii <= 8; ii++ )
    {
        x     = 0.0;
        y     = -width;
        xin   = 0.0;
        yin   = -holeradius;
        theta = -ii * 225;
        RotatePoint( &x, &y, theta );
        RotatePoint( &xin, &yin, theta );
        x += endx;
        RotatePoint( &x, &y, -angle );
        xin += holex;
        RotatePoint( &xin, &yin, -angle );
        glVertex3f( startx + xin, starty + yin, zpos );
        glVertex3f( startx + x, starty + y, zpos );

        if( ii == 0 )
        {
            firstx   = startx + x;
            firsty   = starty + y;
            firstxin = startx + xin;
            firstyin = starty + yin;
        }
    }

    // Layout of the rounded left (2nd half polygon is the origin of the
    // segment)
    for( ii = 0; ii <= 8; ii++ )
    {
        theta = -ii * 225;
        x     = 0.0;
        y     = width;
        RotatePoint( &x, &y, -angle + theta );
        xin = 0.0;
        yin = holeradius;
        RotatePoint( &xin, &yin, theta );
        xin += holex;
        RotatePoint( &xin, &yin, -angle );
        glVertex3f( startx + xin, starty + yin, zpos );
        glVertex3f( startx + x, starty + y, zpos );
    }

    glVertex3f( firstxin, firstyin, zpos );
    glVertex3f( firstx, firsty, zpos );
    glEnd();
}


static void Draw3D_ArcSegment( const S3D_VERTEX& aCenterPos,
                               double aStartPointX, double aStartPointY,
                               double aArcAngle, double aWidth )
{
    const int slice = 16;             // Number of segments to approximate a circle by segments
    double arcStart_Angle;

    arcStart_Angle = (atan2( aStartPointX - aCenterPos.x, aStartPointY - aCenterPos.y ) * 1800 / M_PI );
    double radius = hypot( aStartPointX - aCenterPos.x, aStartPointY - aCenterPos.y )
                    + ( aWidth / 2);
    double hole  = radius - aWidth;

    // Calculate the number of segments to approximate this arc
    int imax = (int) ( (double) aArcAngle / ANGLE_INC(slice) );

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
        double angle = (double) ii * delta_angle;
        angle += arcStart_Angle + 900;
        double dx = hole;
        double dy = 0.0;
        RotatePoint( &dx, &dy, (int) angle );
        glVertex3f( dx + aStartPointX, dy + aStartPointY, aCenterPos.z );
        dx = radius;
        dy = 0.0;
        RotatePoint( &dx, &dy, (int) angle );
        glVertex3f( dx + aStartPointX, dy + aStartPointY, aCenterPos.z );
    }

    glEnd();
}


static void Draw3D_ThickCircle( const S3D_VERTEX& aCenterPos, double aRadius, double aWidth )
{
    double outer_radius = aRadius + ( aWidth / 2);
    double inner_radius  = outer_radius - aWidth;

    Draw3D_FilledCircleWithHole( aCenterPos, outer_radius, inner_radius );
}

/*
 * Function Draw3D_ZaxisOblongCylinder:
 * draw a segment with an oblong hole.
 * Used to draw oblong holes
 */
void Draw3D_ZaxisOblongCylinder( const S3D_VERTEX& aCenterPos,
                                 double aEndx, double aEndy,
                                 double aRadius, double aHeight )
{
    const int slice = 16;
    std::vector<S3D_VERTEX> coords;

    coords.resize( 4 );

    double tmp = DataScale3D;

    DataScale3D = 1.0;    // Coordinate is already in range for Set_Object_Data();

    double dx  = aEndx - aCenterPos.x;
    double dy  = aEndy - aCenterPos.y;
    double angle = atan2( dy, dx ) * 1800 / M_PI;
    dx    = 0;
    dy    = aRadius;

    // draws the first rectangle between half cylinders
    RotatePoint( &dx, &dy, angle );
    coords[0].x = coords[1].x = aCenterPos.x + dx;
    coords[0].y = coords[1].y = aCenterPos.y - dy;
    coords[2].x = coords[3].x = aEndx + dx;
    coords[2].y = coords[3].y = aEndy - dy;
    coords[0].z = coords[3].z = aCenterPos.z;
    coords[1].z = coords[2].z = aCenterPos.z + aHeight;
    Set_Object_Data( coords );

    // Draw the first half cylinder
    for( int ii = 1; ii <= slice/2; ii++ )
    {
        double ddx = dx;
        double ddy = -dy;
        RotatePoint( &ddx, &ddy, -ii * ANGLE_INC(slice) );
        coords[0].x = coords[2].x;
        coords[0].y = coords[2].y;
        coords[1].x = coords[3].x;
        coords[1].y = coords[3].y;
        coords[2].x = coords[3].x = aEndx + ddx;
        coords[2].y = coords[3].y = aEndy + ddy;
        Set_Object_Data( coords );
    }

    // draws the second rectangle between half cylinders
    coords[0].x = coords[1].x = aEndx - dx;
    coords[0].y = coords[1].y = aEndy + dy;
    coords[2].x = coords[3].x = aCenterPos.x - dx;
    coords[2].y = coords[3].y = aCenterPos.y + dy;
    coords[0].z = coords[3].z = aCenterPos.z;
    coords[1].z = coords[2].z = aCenterPos.z + aHeight;
    Set_Object_Data( coords );

    // Draw the second half cylinder
    for( int ii = 1; ii <= slice/2; ii++ )
    {
        double ddx = -dx;
        double ddy = dy;
        RotatePoint( &ddx, &ddy, -ii * ANGLE_INC(slice) );
        coords[0].x = coords[2].x;
        coords[0].y = coords[2].y;
        coords[1].x = coords[3].x;
        coords[1].y = coords[3].y;
        coords[2].x = coords[3].x = aCenterPos.x + ddx;
        coords[2].y = coords[3].y = aCenterPos.y + ddy;
        Set_Object_Data( coords );
    }

    glNormal3f( 0.0, 0.0, 1.0 );    // Normal is Z axis
    DataScale3D = tmp;
}


void EDA_3D_CANVAS::Draw3D_Polygon( std::vector<wxPoint>& aCornersList, double aZpos )
{
    g_Parm_3D_Visu.m_ActZpos = aZpos;

    GLUtesselator* tess = gluNewTess();
    gluTessCallback( tess, GLU_TESS_BEGIN, ( void (CALLBACK*)() )tessBeginCB );
    gluTessCallback( tess, GLU_TESS_END, ( void (CALLBACK*)() )tessEndCB );
    gluTessCallback( tess, GLU_TESS_ERROR, ( void (CALLBACK*)() )tessErrorCB );
    gluTessCallback( tess, GLU_TESS_VERTEX, ( void (CALLBACK*)() )tesswxPoint2Vertex );

    GLdouble v_data[3];
    v_data[2] = aZpos;

    //gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);

    // Draw solid polygon
    gluTessBeginPolygon( tess, NULL );
    gluTessBeginContour( tess );

    for( unsigned ii = 0; ii < aCornersList.size(); ii++ )
    {
        v_data[0] = aCornersList[ii].x * g_Parm_3D_Visu.m_BoardScale;
        v_data[1] = -aCornersList[ii].y * g_Parm_3D_Visu.m_BoardScale;
        // gluTessVertex store pointers on data, not data, so do not store
        // different corners values in a temporary variable
        // but send pointer on each corner value in aCornersList
        gluTessVertex( tess, v_data, &aCornersList[ii] );
    }

    gluTessEndContour( tess );
    gluTessEndPolygon( tess );

    gluDeleteTess( tess );
}


bool Is3DLayerEnabled( int aLayer )
{
    int flg = -1;
    // see if layer needs to be shown
    // check the flags
    switch (aLayer)
    {
        case DRAW_N:
            flg=g_Parm_3D_Visu.FL_DRAWINGS;
            break;

        case COMMENT_N:
            flg=g_Parm_3D_Visu.FL_COMMENTS;
            break;

        case ECO1_N:
            flg=g_Parm_3D_Visu.FL_ECO1;
            break;

        case ECO2_N:
            flg=g_Parm_3D_Visu.FL_ECO2;
            break;
    }
    // the layer was not a layer with a flag, so show it
    if( flg < 0 )
        return true;

    // if the layer has a flag, return the flag
    return g_Parm_3D_Visu.m_DrawFlags[flg];
}


GLfloat Get3DLayer_Z_Orientation( int aLayer )
{
    double nZ;

    nZ = 1.0;

    if( ( aLayer == LAYER_N_BACK )
       || ( aLayer == ADHESIVE_N_BACK )
       || ( aLayer == SOLDERPASTE_N_BACK )
       || ( aLayer == SILKSCREEN_N_BACK )
       || ( aLayer == SOLDERMASK_N_BACK ) )
        nZ = -1.0;

    return nZ;
}


///////////////////////////////////////////////////////////////////////////////
// GLU_TESS CALLBACKS
///////////////////////////////////////////////////////////////////////////////

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

    glVertex3f( ptr->x * g_Parm_3D_Visu.m_BoardScale,
                -ptr->y * g_Parm_3D_Visu.m_BoardScale,
                g_Parm_3D_Visu.m_ActZpos );
}

void CALLBACK tesswxPoint2Vertex( const GLvoid* data )
{
    const wxPoint* ptr = (const wxPoint*) data;

    glVertex3f( ptr->x * g_Parm_3D_Visu.m_BoardScale,
                -ptr->y * g_Parm_3D_Visu.m_BoardScale,
                g_Parm_3D_Visu.m_ActZpos );
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
