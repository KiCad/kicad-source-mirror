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
#include <info3d_visu.h>
#include <trackball.h>
#include <3d_draw_basic_functions.h>

// Imported function:
extern void SetGLColor( int color );
extern void Set_Object_Data( std::vector< S3D_VERTEX >& aVertices, double aBiuTo3DUnits );
extern void CheckGLError();


 /* returns true if aLayer should be displayed, false otherwise
  */
static bool    Is3DLayerEnabled( int aLayer );

 /* returns the Z orientation parmeter 1.0 or -1.0 for aLayer
  * Z orientation is 1.0 for all layers but "back" layers:
  *  LAYER_N_BACK , ADHESIVE_N_BACK, SOLDERPASTE_N_BACK ), SILKSCREEN_N_BACK
  * used to calculate the Z orientation parmeter for glNormal3f
  */
static GLfloat  Get3DLayer_Z_Orientation( int aLayer );

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

    // Build 3D board parameters:
    g_Parm_3D_Visu.InitSettings( pcb );

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

    // move the board in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -g_Parm_3D_Visu.m_BoardPos.x * g_Parm_3D_Visu.m_BiuTo3Dunits,
                  -g_Parm_3D_Visu.m_BoardPos.y * g_Parm_3D_Visu.m_BiuTo3Dunits,
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


    // Draw epoxy limits: TODO

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
    int layer = aZone->GetLayer();
    int color = g_ColorsSettings.GetLayerColor( layer );
    int thickness = g_Parm_3D_Visu.GetLayerObjectThicknessBIU( layer );

    if( layer == LAST_COPPER_LAYER )
        layer = g_Parm_3D_Visu.m_CopperLayersCount - 1;

    int zpos = KiROUND( g_Parm_3D_Visu.m_LayerZcoord[layer] / g_Parm_3D_Visu.m_BiuTo3Dunits );

    SetGLColor( color );
    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );

    if( aZone->m_FillMode == 0 )
    {
        // solid polygons only are used to fill areas
        if( aZone->GetFilledPolysList().size() > 3 )
        {
            Draw3D_SolidHorizontalPolyPolygons( aZone->GetFilledPolysList(),
                                  g_Parm_3D_Visu.GetLayerZcoordBIU( layer ),
                                  thickness, g_Parm_3D_Visu.m_BiuTo3Dunits );
        }
    }
    else
    {
        // segments are used to fill areas
        for( unsigned iseg = 0; iseg < aZone->m_FillSegmList.size(); iseg++ )
            Draw3D_SolidSegment( aZone->m_FillSegmList[iseg].m_Start,
                                 aZone->m_FillSegmList[iseg].m_End,
                                 aZone->m_ZoneMinThickness, thickness, zpos,
                                 g_Parm_3D_Visu.m_BiuTo3Dunits );
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
            wxPoint start( begincorner->x, begincorner->y  );
            wxPoint end( endcorner->x, endcorner->y );
            Draw3D_SolidSegment( start, end,
                                 aZone->m_ZoneMinThickness, thickness, zpos,
                                 g_Parm_3D_Visu.m_BiuTo3Dunits );
        }

        if( (endcorner->end_contour) || (ic == imax) )
        {
            // the last corner of a filled area is found: draw it
            if( endcorner->m_utility == 0 )
            {
                // Draw only basic outlines, not extra segments
                wxPoint start( endcorner->x, endcorner->y );
                wxPoint end( firstcorner->x, firstcorner->y );
                Draw3D_SolidSegment( start, end,
                                     aZone->m_ZoneMinThickness, thickness, zpos,
                                     g_Parm_3D_Visu.m_BiuTo3Dunits );
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


// draw a 3D grid: an horizontal grid (XY plane and Z = 0,
// and a vertical grid (XZ plane and Y = 0)
void EDA_3D_CANVAS::DrawGrid( double aGriSizeMM )
{
    double zpos = 0.0;
    int gridcolor = DARKGRAY;           // Color of grid lines
    int gridcolor_marker = LIGHTGRAY;   // Color of grid lines every 5 lines
    double scale = g_Parm_3D_Visu.m_BiuTo3Dunits;

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
    int thickness = g_Parm_3D_Visu.GetCopperThicknessBIU();

    if( layer == LAST_COPPER_LAYER )
        layer = g_Parm_3D_Visu.m_CopperLayersCount - 1;

    int zpos = KiROUND( g_Parm_3D_Visu.m_LayerZcoord[layer] / g_Parm_3D_Visu.m_BiuTo3Dunits );

    SetGLColor( color );
    glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );

    Draw3D_SolidSegment( aTrack->m_Start, aTrack->m_End,
                         aTrack->m_Width, thickness, zpos,
                         g_Parm_3D_Visu.m_BiuTo3Dunits );
}

void EDA_3D_CANVAS::Draw3D_Via( SEGVIA* via )
{
    int    layer, top_layer, bottom_layer;
    int    color;
    double biu_to_3Dunits = g_Parm_3D_Visu.m_BiuTo3Dunits ;

    int outer_radius = via->m_Width / 2;
    int inner_radius = via->GetDrillValue() / 2;
    int thickness = g_Parm_3D_Visu.GetCopperThicknessBIU();

    via->ReturnLayerPair( &top_layer, &bottom_layer );

    // Drawing horizontal thick rings:
    for( layer = bottom_layer; layer < g_Parm_3D_Visu.m_CopperLayersCount; layer++ )
    {
        int zpos = KiROUND( g_Parm_3D_Visu.m_LayerZcoord[layer] / biu_to_3Dunits );

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

        if( thickness == 0 )
            glNormal3f( 0.0, 0.0, layer == LAYER_N_BACK ? -1.0 : 1.0 );

        Draw3D_ZaxisCylinder( via->m_Start, (outer_radius + inner_radius)/2,
                                  thickness, outer_radius - inner_radius,
                                  zpos - (thickness/2), biu_to_3Dunits );
        if( layer >= top_layer )
            break;
    }

    // Drawing via hole:
    color = g_ColorsSettings.GetItemColor( VIAS_VISIBLE + via->m_Shape );
    SetGLColor( color );
    int height = g_Parm_3D_Visu.GetLayerZcoordBIU(top_layer) -
                 g_Parm_3D_Visu.m_LayerZcoord[bottom_layer] - thickness;
    int zpos = g_Parm_3D_Visu.GetLayerZcoordBIU(bottom_layer) + thickness/2;

    Draw3D_ZaxisCylinder( via->m_Start, inner_radius + thickness/2, height,
                          thickness, zpos, biu_to_3Dunits );
}


void EDA_3D_CANVAS::Draw3D_DrawSegment( DRAWSEGMENT* segment )
{
    int layer = segment->GetLayer();
    int color = g_ColorsSettings.GetLayerColor( layer );
    int thickness = g_Parm_3D_Visu.GetLayerObjectThicknessBIU( layer );

    SetGLColor( color );

    if( layer == EDGE_N )
    {
        for( layer = 0; layer < g_Parm_3D_Visu.m_CopperLayersCount; layer++ )
        {
            glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );
            int zpos = g_Parm_3D_Visu.GetLayerZcoordBIU(layer);

            switch( segment->GetShape() )
            {
            case S_ARC:
                Draw3D_ArcSegment( segment->GetCenter(), segment->GetArcStart(),
                                   segment->GetAngle(), segment->GetWidth(), thickness,
                                   zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
                break;

            case S_CIRCLE:
                {
                int radius = KiROUND( hypot( double(segment->GetStart().x - segment->GetEnd().x),
                                             double(segment->GetStart().y - segment->GetEnd().y) )
                                    );
                Draw3D_ZaxisCylinder( segment->GetStart(), radius,
                                      thickness, segment->GetWidth(),
                                      zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
                }
                break;

            default:
                Draw3D_SolidSegment( segment->GetStart(), segment->GetEnd(),
                                    segment->GetWidth(), thickness,
                                    zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
                break;
            }
        }
    }
    else
    {
        glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );
        int zpos = g_Parm_3D_Visu.GetLayerZcoordBIU(layer);

        if( Is3DLayerEnabled( layer ) )
        {
            switch( segment->GetShape() )
            {
            case S_ARC:
                Draw3D_ArcSegment( segment->GetCenter(), segment->GetArcStart(),
                                   segment->GetAngle(), segment->GetWidth(), thickness,
                                   zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
                break;

            case S_CIRCLE:
            {
                int radius = KiROUND( hypot( double(segment->GetStart().x - segment->GetEnd().x),
                                             double(segment->GetStart().y - segment->GetEnd().y) )
                                    );
                Draw3D_ZaxisCylinder( segment->GetStart(), radius,
                                      thickness, segment->GetWidth(),
                                      zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
            }
                break;

            default:
                Draw3D_SolidSegment( segment->GetStart(), segment->GetEnd(),
                                    segment->GetWidth(), thickness,
                                    zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
                break;
            }
        }
    }
}


// These variables are used in Draw3dTextSegm.
// But Draw3dTextSegm is a call back function, so we cannot send them as arguments,
// so they are static.
int s_Text3DWidth, s_Text3DZPos, s_thickness;

// This is a call back function, used by DrawGraphicText to draw the 3D text shape:
static void Draw3dTextSegm( int x0, int y0, int xf, int yf )
{
    Draw3D_SolidSegment( wxPoint( x0, y0), wxPoint( xf, yf ),
                        s_Text3DWidth, s_thickness, s_Text3DZPos,
                        g_Parm_3D_Visu.m_BiuTo3Dunits );
}


void EDA_3D_CANVAS::Draw3D_DrawText( TEXTE_PCB* text )
{
    int layer = text->GetLayer();
    int color = g_ColorsSettings.GetLayerColor( layer );

    SetGLColor( color );
    s_Text3DZPos  = KiROUND( g_Parm_3D_Visu.m_LayerZcoord[layer] / g_Parm_3D_Visu.m_BiuTo3Dunits );
    s_Text3DWidth = text->GetThickness();
    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );
    wxSize size = text->m_Size;
    s_thickness = g_Parm_3D_Visu.GetLayerObjectThicknessBIU( layer );

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
    for( ; pad != NULL; pad = pad->Next() )
        pad->Draw3D( glcanvas );

    // Draw module shape: 3D shape if exists (or module outlines if not exists)
    S3D_MASTER* Struct3D  = m_3D_Drawings;
    bool        As3dShape = false;

    if (g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_MODULE])
    {
        glPushMatrix();

        glTranslatef( m_Pos.x * g_Parm_3D_Visu.m_BiuTo3Dunits,
                      -m_Pos.y * g_Parm_3D_Visu.m_BiuTo3Dunits,
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
    if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( m_Layer ) == false )
        return;

    int color = g_ColorsSettings.GetLayerColor( m_Layer );
    SetGLColor( color );

    // for outline shape = S_POLYGON:
    // We must compute true coordinates from m_PolyPoints
    // which are relative to module position and module orientation = 0
    std::vector<CPolyPt> polycorners;

    if( m_Shape == S_POLYGON )
    {
        polycorners.reserve( m_PolyPoints.size() );
        MODULE* module = (MODULE*) m_Parent;

        CPolyPt corner;
        for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
        {
            corner.x = m_PolyPoints[ii].x;
            corner.y = m_PolyPoints[ii].y;

            RotatePoint( &corner.x, &corner.y, module->GetOrientation() );
            if( module )
            {
                corner.x += module->m_Pos.x;
                corner.y += module->m_Pos.y;
            }
            polycorners.push_back( corner );
        }

        polycorners.back().end_contour = true;
    }

    if( m_Layer == EDGE_N )
    {
        for( int layer = 0; layer < g_Parm_3D_Visu.m_CopperLayersCount; layer++ )
        {
            glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );
            int zpos = g_Parm_3D_Visu.GetLayerZcoordBIU( layer );
            int thickness = g_Parm_3D_Visu.GetLayerObjectThicknessBIU( m_Layer );

            switch( m_Shape )
            {
            case S_SEGMENT:
                Draw3D_SolidSegment( m_Start, m_End, m_Width,
                                     thickness, zpos,
                                     g_Parm_3D_Visu.m_BiuTo3Dunits );
                break;

            case S_CIRCLE:
            {
               int radius = KiROUND( hypot( double(m_Start.x - m_End.x),
                                             double(m_Start.y - m_End.y) )
                                    );
                Draw3D_ZaxisCylinder( m_Start, radius,
                                      thickness, GetWidth(),
                                      zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
            }
                break;

            case S_ARC:
                Draw3D_ArcSegment( GetCenter(), GetArcStart(),
                                   GetAngle(), GetWidth(), thickness,
                                   zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
                break;

            case S_POLYGON:
                Draw3D_SolidHorizontalPolyPolygons( polycorners, zpos, thickness,
                                                    g_Parm_3D_Visu.m_BiuTo3Dunits);
                break;

            default:
                D( printf( "Error: Shape nr %d not implemented!\n", m_Shape ); )
                break;
            }
        }
    }
    else
    {
        int thickness = g_Parm_3D_Visu.GetLayerObjectThicknessBIU( m_Layer );
        glNormal3f( 0.0, 0.0, (m_Layer == LAYER_N_BACK) ? -1.0 : 1.0 );
        int zpos = g_Parm_3D_Visu.GetLayerZcoordBIU(m_Layer);

        switch( m_Shape )
        {
        case S_SEGMENT:
                Draw3D_SolidSegment( m_Start, m_End, m_Width,
                                     thickness, zpos,
                                     g_Parm_3D_Visu.m_BiuTo3Dunits );
            break;

        case S_CIRCLE:
        {
            int radius = KiROUND( hypot( double(m_Start.x - m_End.x),
                                         double(m_Start.y - m_End.y) )
                                );
            Draw3D_ZaxisCylinder( m_Start, radius,
                                  thickness, GetWidth(),
                                  zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
        }
            break;

        case S_ARC:
            Draw3D_ArcSegment( GetCenter(), GetArcStart(),
                               GetAngle(), GetWidth(), thickness,
                               zpos, g_Parm_3D_Visu.m_BiuTo3Dunits );
            break;

        case S_POLYGON:
            Draw3D_SolidHorizontalPolyPolygons( polycorners, zpos, thickness,
                                                g_Parm_3D_Visu.m_BiuTo3Dunits );
            break;

        default:
            D( printf( "Error: Shape nr %d not implemented!\n", m_Shape ); )
            break;
        }
    }
}


// Draw 3D pads.
void D_PAD::Draw3D( EDA_3D_CANVAS* glcanvas )
{
    int layer, nlmax;
    bool    Oncu, Oncmp, Both;
    int     color;

    double scale = g_Parm_3D_Visu.m_BiuTo3Dunits;

    // Calculate the center of the pad shape.
    wxPoint shape_pos = ReturnShapePos();

    int height = g_Parm_3D_Visu.GetLayerZcoordBIU(LAYER_N_FRONT) -
                    g_Parm_3D_Visu.GetLayerZcoordBIU(LAYER_N_BACK);
    int thickness = g_Parm_3D_Visu.GetCopperThicknessBIU();

    // Store here the points to approximate hole by segments
    std::vector <CPolyPt> holecornersBuffer;
    const int slice = 12;   // number of segments to approximate a circle

    // Draw the pad hole
    bool hasHole = m_Drill.x && m_Drill.y;

    if( hasHole )
    {
        SetGLColor( DARKGRAY );
        int holeZpoz = g_Parm_3D_Visu.GetLayerZcoordBIU(LAYER_N_BACK) + thickness/2;
        int holeHeight = height - thickness;

        if( m_Drill.x == m_Drill.y )    // usual round hole
        {
            Draw3D_ZaxisCylinder( m_Pos,  (m_Drill.x + thickness) / 2, holeHeight,
                                  thickness, holeZpoz, scale );
            TransformCircleToPolygon( holecornersBuffer, m_Pos, m_Drill.x/2, slice );
        }
        else    // Oblong hole
        {
            wxPoint ends_offset;
            int width;

            if( m_Drill.x > m_Drill.y )    // Horizontal oval
            {
                ends_offset.x = ( m_Drill.x - m_Drill.y ) / 2;
                width = m_Drill.y;
            }
            else    // Vertical oval
            {
                ends_offset.y = ( m_Drill.y - m_Drill.x ) / 2;
                width = m_Drill.x;
            }

            RotatePoint( &ends_offset, m_Orient );

            wxPoint start  = m_Pos + ends_offset;
            wxPoint end  = m_Pos - ends_offset;
            int hole_radius = ( width + thickness ) / 2;

            // Prepare the shape creation
            TransformRoundedEndsSegmentToPolygon( holecornersBuffer, start, end, slice, width );

            // Draw the hole
            Draw3D_ZaxisOblongCylinder( start, end, hole_radius, holeHeight,
                                        thickness, holeZpoz, scale );
        }
    }

    glNormal3f( 0.0, 0.0, 1.0 ); // Normal is Z axis

    nlmax = g_Parm_3D_Visu.m_CopperLayersCount - 1;
    Oncu  = (m_layerMask & LAYER_BACK) ? true : false;
    Oncmp = (m_layerMask & LAYER_FRONT) ? true : false;
    Both  = Oncu && Oncmp;

    // Store here the points to approximate pad shape by segments
    std::vector<CPolyPt> polyPadShape;

    switch( GetShape() )
    {
    case PAD_CIRCLE:
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
            int zpos = g_Parm_3D_Visu.GetLayerZcoordBIU( layer );
            int ring_radius = (m_Size.x + m_Drill.x) / 4;
            if( thickness == 0 )
                glNormal3f( 0.0, 0.0, layer == LAYER_N_BACK ? -1.0 : 1.0 );

            Draw3D_ZaxisCylinder(shape_pos, ring_radius,
                                  thickness, ( m_Size.x - m_Drill.x) / 2,
                                  zpos - (thickness/2), scale );
            }

        break;

    case PAD_OVAL:
        {
        wxPoint ends_offset;
        int width;
        if( m_Size.x > m_Size.y ) // Horizontal ellipse
        {
            ends_offset.x = ( m_Size.x - m_Size.y ) / 2;
            width = m_Size.y;
        }
        else // Vertical ellipse
        {
            ends_offset.y = ( m_Size.y - m_Size.x ) / 2;
            width = m_Size.x;
        }

        RotatePoint( &ends_offset, m_Orient );
        wxPoint start  = shape_pos + ends_offset;
        wxPoint end  = shape_pos - ends_offset;
        TransformRoundedEndsSegmentToPolygon( polyPadShape, start, end, slice, width );
        if( hasHole )
            polyPadShape.insert( polyPadShape.end(), holecornersBuffer.begin(), holecornersBuffer.end() );
        }
        break;

    case PAD_RECT:
    case PAD_TRAPEZOID:
        {
        wxPoint coord[5];
        BuildPadPolygon( coord, wxSize(0,0), m_Orient );
        for( int ii = 0; ii < 4; ii ++ )
        {
            CPolyPt pt( coord[ii].x + shape_pos.x, coord[ii].y+ shape_pos.y );
            polyPadShape.push_back( pt );
        }
        polyPadShape.back().end_contour = true;

        if( hasHole )
            polyPadShape.insert( polyPadShape.end(), holecornersBuffer.begin(), holecornersBuffer.end() );
        }
    break;

    default:
        break;
    }

    if( polyPadShape.size() )
    {
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

            if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) == false )
                continue;

            color = g_ColorsSettings.GetLayerColor( layer );
            SetGLColor( color );

            if( thickness == 0 )
                glNormal3f( 0.0, 0.0, layer == LAYER_N_BACK ? -1.0 : 1.0 );

            // If not hole: draw a single polygon
            int zpos = g_Parm_3D_Visu.GetLayerZcoordBIU( layer );
            if( hasHole )
            {
                Draw3D_SolidHorizontalPolygonWithHoles( polyPadShape, zpos,
                                thickness, g_Parm_3D_Visu.m_BiuTo3Dunits );
            }

            else
            {
                Draw3D_SolidHorizontalPolyPolygons( polyPadShape, zpos,
                                          thickness, g_Parm_3D_Visu.m_BiuTo3Dunits );
            }
        }
    }

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
