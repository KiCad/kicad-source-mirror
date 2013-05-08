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
#include <layers_id_colors_and_visibility.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <colors_selection.h>
#include <convert_basic_shapes_to_polygon.h>

#include <3d_viewer.h>
#include <3d_canvas.h>
#include <info3d_visu.h>
#include <trackball.h>
#include <3d_draw_basic_functions.h>

// Imported function:
extern void     SetGLColor( EDA_COLOR_T color );
extern void     Set_Object_Data( std::vector<S3D_VERTEX>& aVertices, double aBiuTo3DUnits );
extern void     CheckGLError();

/* returns true if aLayer should be displayed, false otherwise
 */
static bool     Is3DLayerEnabled( LAYER_NUM aLayer );

/* returns the Z orientation parameter 1.0 or -1.0 for aLayer
 * Z orientation is 1.0 for all layers but "back" layers:
 *  LAYER_N_BACK , ADHESIVE_N_BACK, SOLDERPASTE_N_BACK ), SILKSCREEN_N_BACK
 * used to calculate the Z orientation parameter for glNormal3f
 */
static GLfloat  Get3DLayer_Z_Orientation( LAYER_NUM aLayer );

/* Helper function BuildPadShapeThickOutlineAsPolygon:
 * Build a pad shape outline as polygon, to draw pads on silkscreen layer
 * with a line thickness = aWidth
 * Used only to draw pads outlines on silkscreen layers.
 */
static void BuildPadShapeThickOutlineAsPolygon( D_PAD*          aPad,
                                                CPOLYGONS_LIST& aCornerBuffer,
                                                int             aWidth,
                                                int             aCircleToSegmentsCount,
                                                double          aCorrectionFactor )
{
    if( aPad->GetShape() == PAD_CIRCLE )    // Draw a ring
    {
        TransformRingToPolygon( aCornerBuffer, aPad->ReturnShapePos(),
                                aPad->GetSize().x / 2, aCircleToSegmentsCount, aWidth );
        return;
    }

    // For other shapes, draw polygon outlines
    CPOLYGONS_LIST corners;
    aPad->BuildPadShapePolygon( corners, wxSize( 0, 0 ),
                                aCircleToSegmentsCount, aCorrectionFactor );

    // Add outlines as thick segments in polygon buffer
    for( unsigned ii = 0, jj = corners.size() - 1; ii < corners.size(); jj = ii, ii++ )
    {
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                                              corners.GetPos( jj ),
                                              corners.GetPos( ii ),
                                              aCircleToSegmentsCount, aWidth );
    }
}


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
        glCallList( m_gllist );
    else
        CreateDrawGL_List();

    glFlush();

    if( finish )
        glFinish();

    SwapBuffers();
}


void EDA_3D_CANVAS::BuildBoard3DView()
{
    PCB_BASE_FRAME* pcbframe = Parent()->Parent();
    BOARD*          pcb = pcbframe->GetBoard();

    // Number of segments to draw a circle using segments
    const int       segcountforcircle   = 16;
    double          correctionFactor    = 1.0 / cos( M_PI / (segcountforcircle * 2) );
    const int       segcountLowQuality  = 12;   // segments to draw a circle with low quality
                                                // to reduce time calculations
                                                // for holes and items which do not need
                                                // a fine representation
    double          correctionFactorLQ = 1.0 / cos( M_PI / (segcountLowQuality * 2) );
    CPOLYGONS_LIST  bufferPolys;

    bufferPolys.reserve( 200000 );                  // Reserve for large board (tracks mainly)
    CPOLYGONS_LIST  bufferZonesPolys;
    bufferPolys.reserve( 500000 );                  // Reserve for large board ( copper zones mainly )
    CPOLYGONS_LIST  currLayerHoles;                 // Contains holes for the current layer
    CPOLYGONS_LIST  allLayerHoles;                  // Contains through holes, calculated only once
    allLayerHoles.reserve( 20000 );
    bool            throughHolesListBuilt = false;  // flag to build the through hole polygon list only once
    bool            hightQualityMode = false;

    for( LAYER_NUM layer = FIRST_COPPER_LAYER; layer <= LAST_COPPER_LAYER;
         layer++ )
    {
        if( layer != LAST_COPPER_LAYER
            && layer >= g_Parm_3D_Visu.m_CopperLayersCount )
            continue;

        if( !g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) )
            continue;

        bufferPolys.clear();
        bufferZonesPolys.clear();
        currLayerHoles.clear();

        // Draw tracks:
        for( TRACK* track = pcb->m_Track; track != NULL; track = track->Next() )
        {
            if( !track->IsOnLayer( layer ) )
                continue;

            track->TransformShapeWithClearanceToPolygon( bufferPolys,
                                                         0, segcountforcircle,
                                                         correctionFactor );

            // Add via hole
            if( track->Type() == PCB_VIA_T )
            {
                int shape = track->GetShape();
                int holediameter    = track->GetDrillValue();
                int thickness       = g_Parm_3D_Visu.GetCopperThicknessBIU();
                int hole_outer_radius = (holediameter + thickness) / 2;

                if( shape != VIA_THROUGH )
                    TransformCircleToPolygon( currLayerHoles,
                                              track->GetStart(), hole_outer_radius,
                                              segcountLowQuality );
                else if( !throughHolesListBuilt )
                    TransformCircleToPolygon( allLayerHoles,
                                              track->GetStart(), hole_outer_radius,
                                              segcountLowQuality );
            }
        }

        // draw pads
        for( MODULE* module = pcb->m_Modules; module != NULL; module = module->Next() )
        {
            module->TransformPadsShapesWithClearanceToPolygon( layer,
                                                               bufferPolys,
                                                               0,
                                                               segcountforcircle,
                                                               correctionFactor );

            // Micro-wave modukes may have items on copper layers
            module->TransformGraphicShapesWithClearanceToPolygonSet( layer,
                                                                     bufferPolys,
                                                                     0,
                                                                     segcountforcircle,
                                                                     correctionFactor );

            // Add pad hole, if any
            if( !throughHolesListBuilt )
            {
                D_PAD* pad = module->Pads();

                for( ; pad != NULL; pad = pad->Next() )
                    pad->BuildPadDrillShapePolygon( allLayerHoles, 0,
                                                    segcountLowQuality );
            }
        }

        // Draw copper zones
        if( g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_ZONE] )
        {
            for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
            {
                ZONE_CONTAINER* zone = pcb->GetArea( ii );
                LAYER_NUM       zonelayer = zone->GetLayer();

                if( zonelayer == layer )
                    zone->TransformSolidAreasShapesToPolygonSet(
                        hightQualityMode ? bufferPolys : bufferZonesPolys,
                        segcountLowQuality, correctionFactorLQ );
            }
        }

        // draw graphic items
        for( BOARD_ITEM* item = pcb->m_Drawings; item; item = item->Next() )
        {
            if( !item->IsOnLayer( layer ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:
                ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                    bufferPolys, 0,
                    segcountforcircle,
                    correctionFactor );
                break;

            case PCB_TEXT_T:
                ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet(
                    bufferPolys, 0, segcountforcircle, correctionFactor );
                break;

            default:
                break;
            }
        }

        // bufferPolys contains polygons to merge. Many overlaps . Calculate merged polygons
        if( bufferPolys.size() == 0 )
            continue;

        KI_POLYGON_SET  currLayerPolyset;
        KI_POLYGON_SET  polysetHoles;

        // Add polygons, without holes
        bufferPolys.ExportTo( currLayerPolyset );

        // Add holes in polygon list
        currLayerHoles.Append( allLayerHoles );

        if( currLayerHoles.size() > 0 )
            currLayerHoles.ExportTo( polysetHoles );

        // Merge polygons, remove holes
        currLayerPolyset -= polysetHoles;

        EDA_COLOR_T color = g_ColorsSettings.GetLayerColor( layer );
        int         thickness = g_Parm_3D_Visu.GetLayerObjectThicknessBIU( layer );
        int         zpos = g_Parm_3D_Visu.GetLayerZcoordBIU( layer );

        SetGLColor( color );
        glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );

        bufferPolys.clear();
        bufferPolys.ImportFrom( currLayerPolyset );
        Draw3D_SolidHorizontalPolyPolygons( bufferPolys, zpos,
                                            thickness,
                                            g_Parm_3D_Visu.m_BiuTo3Dunits );

        if( bufferZonesPolys.size() )
            Draw3D_SolidHorizontalPolyPolygons( bufferZonesPolys, zpos,
                                                thickness,
                                                g_Parm_3D_Visu.m_BiuTo3Dunits );

        throughHolesListBuilt = true;
    }

    // Draw vias holes (vertical cylinders)
    for( TRACK* track = pcb->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            Draw3DViaHole( (SEGVIA*) track );
    }

    // Draw pads holes (vertical cylinders)
    for( MODULE* module = pcb->m_Modules; module != NULL; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad != NULL; pad = pad->Next() )
            Draw3DPadHole( pad );
    }

    // draw graphic items, not on copper layers
    for( LAYER_NUM layer = FIRST_NON_COPPER_LAYER; layer <= LAST_NON_COPPER_LAYER;
         layer++ )
    {
        if( !Is3DLayerEnabled( layer ) )
            continue;

        if( !g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) )
            continue;

        bufferPolys.clear();

        for( BOARD_ITEM* item = pcb->m_Drawings; item; item = item->Next() )
        {
            if( !item->IsOnLayer( layer ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:
                ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                    bufferPolys, 0,
                    segcountforcircle,
                    correctionFactor );
                break;

            case PCB_TEXT_T:
                ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet(
                    bufferPolys, 0, segcountforcircle, correctionFactor );
                break;

            default:
                break;
            }
        }

        for( MODULE* module = pcb->m_Modules; module != NULL; module = module->Next() )
        {
            if( layer == SILKSCREEN_N_FRONT || layer == SILKSCREEN_N_BACK )
            {
                D_PAD*  pad = module->Pads();
                int     linewidth = g_DrawDefaultLineThickness;

                for( ; pad != NULL; pad = pad->Next() )
                {
                    if( !pad->IsOnLayer( layer ) )
                        continue;

                    BuildPadShapeThickOutlineAsPolygon( pad, bufferPolys,
                                                        linewidth,
                                                        segcountforcircle, correctionFactor );
                }
            }
            else
                module->TransformPadsShapesWithClearanceToPolygon( layer,
                                                                   bufferPolys,
                                                                   0,
                                                                   segcountforcircle,
                                                                   correctionFactor );

            module->TransformGraphicShapesWithClearanceToPolygonSet( layer,
                                                                     bufferPolys,
                                                                     0,
                                                                     segcountforcircle,
                                                                     correctionFactor );
        }

        // bufferPolys contains polygons to merge. Many overlaps .
        // Calculate merged polygons and remove pads and vias holes
        if( bufferPolys.size() == 0 )
            continue;

        KI_POLYGON_SET  currLayerPolyset;
        KI_POLYGON_SET  polyset;
        bufferPolys.ExportTo( polyset );
        // merge polys:
        currLayerPolyset += polyset;

        EDA_COLOR_T color = g_ColorsSettings.GetLayerColor( layer );
        int         thickness = g_Parm_3D_Visu.GetLayerObjectThicknessBIU( layer );
        int         zpos = g_Parm_3D_Visu.GetLayerZcoordBIU( layer );

        if( layer == EDGE_N )
        {
            thickness = g_Parm_3D_Visu.GetLayerZcoordBIU( LAYER_N_FRONT )
                        - g_Parm_3D_Visu.GetLayerZcoordBIU( LAYER_N_BACK );
            zpos = g_Parm_3D_Visu.GetLayerZcoordBIU( LAYER_N_BACK )
                   + (thickness / 2);
        }

        SetGLColor( color );
        glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );

        bufferPolys.clear();
        bufferPolys.ImportFrom( currLayerPolyset );
        Draw3D_SolidHorizontalPolyPolygons( bufferPolys, zpos,
                                            thickness, g_Parm_3D_Visu.m_BiuTo3Dunits );
    }

    // draw modules 3D shapes
    for( MODULE* module = pcb->m_Modules; module != NULL; module = module->Next() )
        module->ReadAndInsert3DComponentShape( this );
}


GLuint EDA_3D_CANVAS::CreateDrawGL_List()
{
    PCB_BASE_FRAME* pcbframe = Parent()->Parent();
    BOARD*          pcb = pcbframe->GetBoard();

    wxBusyCursor    dummy;

    m_gllist = glGenLists( 1 );

    // Build 3D board parameters:
    g_Parm_3D_Visu.InitSettings( pcb );

    glNewList( m_gllist, GL_COMPILE_AND_EXECUTE );

    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    // draw axis
    if( g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_AXIS] )
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

    // move the board in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -g_Parm_3D_Visu.m_BoardPos.x * g_Parm_3D_Visu.m_BiuTo3Dunits,
                  -g_Parm_3D_Visu.m_BoardPos.y * g_Parm_3D_Visu.m_BiuTo3Dunits,
                  0.0F );

    // Draw Board:
// For testing purpose only display calculation time to generate 3D data
// #define PRINT_CALCULATION_TIME

#ifdef PRINT_CALCULATION_TIME
    unsigned strtime = GetRunningMicroSecs();
#endif

    BuildBoard3DView();

    // Draw grid
    if( g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_GRID] )
        DrawGrid( g_Parm_3D_Visu.m_3D_Grid );

    glEndList();

    // Test for errors
    CheckGLError();

#ifdef PRINT_CALCULATION_TIME
    unsigned    endtime = GetRunningMicroSecs();
    wxString    msg;
    msg.Printf( "Built data %.1f ms", (double) (endtime - strtime) / 1000 );
    Parent()->SetStatusText( msg, 0 );
#endif

    return m_gllist;
}


// draw a 3D grid: an horizontal grid (XY plane and Z = 0,
// and a vertical grid (XZ plane and Y = 0)
void EDA_3D_CANVAS::DrawGrid( double aGriSizeMM )
{
    double      zpos = 0.0;
    EDA_COLOR_T gridcolor = DARKGRAY;           // Color of grid lines
    EDA_COLOR_T gridcolor_marker = LIGHTGRAY;   // Color of grid lines every 5 lines
    double      scale = g_Parm_3D_Visu.m_BiuTo3Dunits;

    glNormal3f( 0.0, 0.0, 1.0 );

    wxSize  brd_size = g_Parm_3D_Visu.m_BoardSize;
    wxPoint brd_center_pos = g_Parm_3D_Visu.m_BoardPos;
    NEGATE( brd_center_pos.y );

    int     xsize   = std::max( brd_size.x, Millimeter2iu( 100 ) );
    int     ysize   = std::max( brd_size.y, Millimeter2iu( 100 ) );

    // Grid limits, in 3D units
    double  xmin    = (brd_center_pos.x - xsize / 2) * scale;
    double  xmax    = (brd_center_pos.x + xsize / 2) * scale;
    double  ymin    = (brd_center_pos.y - ysize / 2) * scale;
    double  ymax    = (brd_center_pos.y + ysize / 2) * scale;
    double  zmin    = Millimeter2iu( -50 ) * scale;
    double  zmax    = Millimeter2iu( 100 ) * scale;

    // Draw horizontal grid centered on 3D origin (center of the board)
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            SetGLColor( gridcolor );
        else
            SetGLColor( gridcolor_marker );

        int delta = KiROUND( ii * aGriSizeMM * IU_PER_MM );

        if( delta <= xsize / 2 )    // Draw grid lines parallel to X axis
        {
            glBegin( GL_LINES );
            glVertex3f( (brd_center_pos.x + delta) * scale, -ymin, zpos );
            glVertex3f( (brd_center_pos.x + delta) * scale, -ymax, zpos );
            glEnd();

            if( ii != 0 )
            {
                glBegin( GL_LINES );
                glVertex3f( (brd_center_pos.x - delta) * scale, -ymin, zpos );
                glVertex3f( (brd_center_pos.x - delta) * scale, -ymax, zpos );
                glEnd();
            }
        }

        if( delta <= ysize / 2 )    // Draw grid lines parallel to Y axis
        {
            glBegin( GL_LINES );
            glVertex3f( xmin, -(brd_center_pos.y + delta) * scale, zpos );
            glVertex3f( xmax, -(brd_center_pos.y + delta) * scale, zpos );
            glEnd();

            if( ii != 0 )
            {
                glBegin( GL_LINES );
                glVertex3f( xmin, -(brd_center_pos.y - delta) * scale, zpos );
                glVertex3f( xmax, -(brd_center_pos.y - delta) * scale, zpos );
                glEnd();
            }
        }

        if( ( delta > ysize / 2 ) && ( delta > xsize / 2 ) )
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

        glBegin( GL_LINES );
        glVertex3f( (brd_center_pos.x + delta) * scale, -brd_center_pos.y * scale, zmin );
        glVertex3f( (brd_center_pos.x + delta) * scale, -brd_center_pos.y * scale, zmax );
        glEnd();

        if( ii != 0 )
        {
            glBegin( GL_LINES );
            glVertex3f( (brd_center_pos.x - delta) * scale, -brd_center_pos.y * scale, zmin );
            glVertex3f( (brd_center_pos.x - delta) * scale, -brd_center_pos.y * scale, zmax );
            glEnd();
        }

        if( delta > xsize / 2 )
            break;
    }

    // Draw horizontal grid lines on Z axis
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            SetGLColor( gridcolor );
        else
            SetGLColor( gridcolor_marker );

        double delta = ii * aGriSizeMM * IU_PER_MM * scale;

        if( delta <= zmax )
        {
            // Draw grid lines on Z axis (positive Z axis coordinates)
            glBegin( GL_LINES );
            glVertex3f( xmin, -brd_center_pos.y * scale, delta );
            glVertex3f( xmax, -brd_center_pos.y * scale, delta );
            glEnd();
        }

        if( delta <= -zmin && ( ii != 0 ) )
        {
            // Draw grid lines on Z axis (negative Z axis coordinates)
            glBegin( GL_LINES );
            glVertex3f( xmin, -brd_center_pos.y * scale, -delta );
            glVertex3f( xmax, -brd_center_pos.y * scale, -delta );
            glEnd();
        }

        if( ( delta > zmax ) && ( delta > -zmin ) )
            break;
    }
}


void EDA_3D_CANVAS::Draw3DViaHole( SEGVIA* aVia )
{
    LAYER_NUM   top_layer, bottom_layer;
    int         inner_radius    = aVia->GetDrillValue() / 2;
    int         thickness       = g_Parm_3D_Visu.GetCopperThicknessBIU();

    aVia->ReturnLayerPair( &top_layer, &bottom_layer );

    // Drawing via hole:
    EDA_COLOR_T color = g_ColorsSettings.GetItemColor( VIAS_VISIBLE + aVia->GetShape() );
    SetGLColor( color );
    int         height = g_Parm_3D_Visu.GetLayerZcoordBIU( top_layer ) -
                         g_Parm_3D_Visu.GetLayerZcoordBIU( bottom_layer ) - thickness;
    int         zpos = g_Parm_3D_Visu.GetLayerZcoordBIU( bottom_layer ) + thickness / 2;

    Draw3D_ZaxisCylinder( aVia->GetStart(), inner_radius + thickness / 2, height,
                          thickness, zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
}


void MODULE::ReadAndInsert3DComponentShape( EDA_3D_CANVAS* glcanvas )
{
    // Draw module shape: 3D shape if exists (or module outlines if not exists)
    S3D_MASTER* struct3D = m_3D_Drawings;

    if( g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_MODULE] )
    {
        double zpos;

        if( IsFlipped() )
            zpos = g_Parm_3D_Visu.GetModulesZcoord3DIU( true );
        else
            zpos = g_Parm_3D_Visu.GetModulesZcoord3DIU( false );

        glPushMatrix();

        glTranslatef( m_Pos.x * g_Parm_3D_Visu.m_BiuTo3Dunits,
                      -m_Pos.y * g_Parm_3D_Visu.m_BiuTo3Dunits,
                      zpos );

        if( m_Orient )
            glRotatef( (double) m_Orient / 10, 0.0, 0.0, 1.0 );

        if( IsFlipped() )
        {
            glRotatef( 180.0, 0.0, 1.0, 0.0 );
            glRotatef( 180.0, 0.0, 0.0, 1.0 );
        }

        for( ; struct3D != NULL; struct3D = struct3D->Next() )
        {
            if( !struct3D->m_Shape3DName.IsEmpty() )
                struct3D->ReadData();
        }

        glPopMatrix();
    }
}


// Draw 3D pads.
void EDA_3D_CANVAS::Draw3DPadHole( D_PAD* aPad )
{
    // Draw the pad hole
    wxSize  drillsize   = aPad->GetDrillSize();
    bool    hasHole     = drillsize.x && drillsize.y;

    if( !hasHole )
        return;

    // Store here the points to approximate hole by segments
    CPOLYGONS_LIST  holecornersBuffer;
    int             thickness   = g_Parm_3D_Visu.GetCopperThicknessBIU();
    int             height      = g_Parm_3D_Visu.GetLayerZcoordBIU( LAYER_N_FRONT ) -
                                  g_Parm_3D_Visu.GetLayerZcoordBIU( LAYER_N_BACK );

    SetGLColor( DARKGRAY );
    int holeZpoz    = g_Parm_3D_Visu.GetLayerZcoordBIU( LAYER_N_BACK ) + thickness / 2;
    int holeHeight  = height - thickness;

    if( drillsize.x == drillsize.y )    // usual round hole
    {
        Draw3D_ZaxisCylinder( aPad->GetPosition(),
                              (drillsize.x + thickness) / 2, holeHeight,
                              thickness, holeZpoz, g_Parm_3D_Visu.m_BiuTo3Dunits );
    }
    else    // Oblong hole
    {
        wxPoint ends_offset;
        int     width;

        if( drillsize.x > drillsize.y )    // Horizontal oval
        {
            ends_offset.x = ( drillsize.x - drillsize.y ) / 2;
            width = drillsize.y;
        }
        else    // Vertical oval
        {
            ends_offset.y = ( drillsize.y - drillsize.x ) / 2;
            width = drillsize.x;
        }

        RotatePoint( &ends_offset, aPad->GetOrientation() );

        wxPoint start   = aPad->GetPosition() + ends_offset;
        wxPoint end     = aPad->GetPosition() - ends_offset;
        int     hole_radius = ( width + thickness ) / 2;

        // Draw the hole
        Draw3D_ZaxisOblongCylinder( start, end, hole_radius, holeHeight,
                                    thickness, holeZpoz, g_Parm_3D_Visu.m_BiuTo3Dunits );
    }
}


bool Is3DLayerEnabled( LAYER_NUM aLayer )
{
    int flg;

    // see if layer needs to be shown
    // check the flags
    switch( aLayer )
    {
    case DRAW_N:
        flg = g_Parm_3D_Visu.FL_DRAWINGS;
        break;

    case COMMENT_N:
        flg = g_Parm_3D_Visu.FL_COMMENTS;
        break;

    case ECO1_N:
        flg = g_Parm_3D_Visu.FL_ECO1;
        break;

    case ECO2_N:
        flg = g_Parm_3D_Visu.FL_ECO2;
        break;

    default:
        // the layer was not a layer with a flag, so show it
        return true;
    }

    // if the layer has a flag, return the flag
    return g_Parm_3D_Visu.m_DrawFlags[flg];
}


GLfloat Get3DLayer_Z_Orientation( LAYER_NUM aLayer )
{
    double nZ = 1.0;

    if( ( aLayer == LAYER_N_BACK )
        || ( aLayer == ADHESIVE_N_BACK )
        || ( aLayer == SOLDERPASTE_N_BACK )
        || ( aLayer == SILKSCREEN_N_BACK )
        || ( aLayer == SOLDERMASK_N_BACK ) )
        nZ = -1.0;

    return nZ;
}
