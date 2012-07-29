/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <layers_id_colors_and_visibility.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_board_design_settings.h>
#include <class_marker_pcb.h>
#include <colors_selection.h>

#include <3d_viewer.h>
#include <trackball.h>

#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

extern void CheckGLError();

static void    Draw3D_FilledCircle( double posx, double posy, double rayon,
                                    double hole_rayon, double zpos );
static void    Draw3D_FilledSegment( double startx, double starty,
                                     double endx, double endy,
                                     double width, double zpos );
static void    Draw3D_FilledCylinder( double posx, double posy, double rayon,
                                      double height, double zpos );
static void    Draw3D_FilledSegmentWithHole( double startx, double starty,
                                             double endx, double endy,
                                             double width, double holex,
                                             double holey, double holeradius,
                                             double zpos );
static void    Draw3D_ArcSegment( double startx, double starty, double centrex,
                                  double centrey, double arc_angle, double width, double zpos );
static void    Draw3D_CircleSegment( double startx, double starty, double endx,
                                     double endy, double width, double zpos );
static int     Get3DLayerEnable( int act_layer );
static GLfloat Get3DLayerSide( int act_layer );


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

#if wxCHECK_VERSION( 2, 7, 0 )
    SetCurrent( *m_glRC );
#else
    SetCurrent();
#endif

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
    if( finish );
        glFinish();

    SwapBuffers();
}


GLuint EDA_3D_CANVAS::CreateDrawGL_List()
{
    PCB_BASE_FRAME* pcbframe = Parent()->Parent();
    BOARD* pcb = pcbframe->GetBoard();
    TRACK* track;
    SEGZONE*             segzone;
    int ii;

    wxBusyCursor         dummy;

    m_gllist = glGenLists( 1 );

    EDA_RECT    bbbox = pcbframe->GetBoardBoundingBox();

    g_Parm_3D_Visu.m_BoardSettings = &pcb->GetDesignSettings();

    g_Parm_3D_Visu.m_BoardSize     = bbbox.GetSize();
    g_Parm_3D_Visu.m_BoardPos      = bbbox.Centre();

    g_Parm_3D_Visu.m_BoardPos.y    = -g_Parm_3D_Visu.m_BoardPos.y;
    g_Parm_3D_Visu.m_Layers        = pcb->GetCopperLayerCount();

    // Ensure the board has 2 sides for 3D views, because it is hard to find
    // a *really* single side board in the true life...
    if( g_Parm_3D_Visu.m_Layers < 2 )
        g_Parm_3D_Visu.m_Layers = 2;

    g_Parm_3D_Visu.m_BoardScale = 2.0 / max( g_Parm_3D_Visu.m_BoardSize.x,
                                             g_Parm_3D_Visu.m_BoardSize.y );

    // @TODO: epoxy_width (board thickness) must be set by user,
    // because all boards thickness no not match with this setup:
    // double epoxy_width = 1.6;    // epoxy width in mm

    g_Parm_3D_Visu.m_Epoxy_Width = pcb->GetDesignSettings().GetBoardThickness()
                                   * g_Parm_3D_Visu.m_BoardScale;

    // calculate z position for each layer
    for( ii = 0; ii < 32; ii++ )
    {
        if( ii < g_Parm_3D_Visu.m_Layers )
            g_Parm_3D_Visu.m_LayerZcoord[ii] = g_Parm_3D_Visu.m_Epoxy_Width
                                               * ii / (g_Parm_3D_Visu.m_Layers - 1);
        else
            g_Parm_3D_Visu.m_LayerZcoord[ii] = g_Parm_3D_Visu.m_Epoxy_Width;
    }

    GLfloat zpos_cu  = 10 * g_Parm_3D_Visu.m_BoardScale;
    GLfloat zpos_cmp = g_Parm_3D_Visu.m_Epoxy_Width + zpos_cu;
    g_Parm_3D_Visu.m_LayerZcoord[ADHESIVE_N_BACK]    = -zpos_cu * 2;
    g_Parm_3D_Visu.m_LayerZcoord[ADHESIVE_N_FRONT]   = zpos_cmp + zpos_cu;
    g_Parm_3D_Visu.m_LayerZcoord[SILKSCREEN_N_BACK]  = -zpos_cu;
    g_Parm_3D_Visu.m_LayerZcoord[SILKSCREEN_N_FRONT] = zpos_cmp;
    g_Parm_3D_Visu.m_LayerZcoord[DRAW_N]    = zpos_cmp + zpos_cu;
    g_Parm_3D_Visu.m_LayerZcoord[COMMENT_N] = zpos_cmp + zpos_cu;
    g_Parm_3D_Visu.m_LayerZcoord[ECO1_N]    = zpos_cmp + zpos_cu;
    g_Parm_3D_Visu.m_LayerZcoord[ECO2_N]    = zpos_cmp + zpos_cu;

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
    for( track = pcb->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            Draw3D_Via( (SEGVIA*) track );
        else
            Draw3D_Track( track );
    }

    if (g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_ZONE])
    {
        // Draw segments used to fill copper areas. outdated!
        for( segzone = pcb->m_Zone; segzone != NULL; segzone = segzone->Next() )
        {
            if( segzone->Type() == PCB_ZONE_T )
                Draw3D_Track( segzone );
        }

        // Draw new segments
        for( ii = 0; ii < pcb->GetAreaCount(); ii++ )
        {
            ZONE_CONTAINER* curr_zone = pcb->GetArea( ii );

            if( curr_zone->m_FillMode == 0 )
            {
                // solid polygons only are used to fill areas
                if( curr_zone->GetFilledPolysList().size() > 3 )
                {
                    Draw3D_SolidPolygonsInZones( curr_zone );
                }
            }
            else
            {
                // segments are used to fill areas
                for( unsigned iseg = 0; iseg < curr_zone->m_FillSegmList.size(); iseg++ )
                {
                    SEGZONE dummysegment( pcb );
                    dummysegment.SetLayer( curr_zone->GetLayer() );
                    dummysegment.m_Width = curr_zone->m_ZoneMinThickness;

                    dummysegment.m_Start.x = curr_zone->m_FillSegmList[iseg].m_Start.x;
                    dummysegment.m_Start.y = curr_zone->m_FillSegmList[iseg].m_Start.y;
                    dummysegment.m_End.x   = curr_zone->m_FillSegmList[iseg].m_End.x;
                    dummysegment.m_End.y   = curr_zone->m_FillSegmList[iseg].m_End.y;
                    Draw3D_Track( &dummysegment );
                }
            }
        }

        // Draw copper areas outlines
        for( ii = 0; ii < pcb->GetAreaCount(); ii++ )
        {
            ZONE_CONTAINER* zone = pcb->GetArea( ii );

            std::vector<CPolyPt> polysList = zone->GetFilledPolysList();

            if( polysList.size() == 0 )
                continue;

            if( zone->m_ZoneMinThickness <= 1 )
                continue;

            int      imax = polysList.size() - 1;
            CPolyPt* firstcorner = &polysList[0];
            CPolyPt* begincorner = firstcorner;
            SEGZONE  dummysegment( pcb );
            dummysegment.SetLayer( zone->GetLayer() );
            dummysegment.m_Width = zone->m_ZoneMinThickness;

            for( int ic = 1; ic <= imax; ic++ )
            {
                CPolyPt* endcorner = &polysList[ic];

                if( begincorner->m_utility == 0 )
                {
                    // Draw only basic outlines, not extra segments
                    dummysegment.m_Start.x = begincorner->x;
                    dummysegment.m_Start.y = begincorner->y;
                    dummysegment.m_End.x   = endcorner->x;
                    dummysegment.m_End.y   = endcorner->y;
                    Draw3D_Track( &dummysegment );
                }

                if( (endcorner->end_contour) || (ic == imax) )
                {
                    // the last corner of a filled area is found: draw it
                    if( endcorner->m_utility == 0 )
                    {
                        // Draw only basic outlines, not extra segments
                        dummysegment.m_Start.x = endcorner->x;
                        dummysegment.m_Start.y = endcorner->y;
                        dummysegment.m_End.x   = firstcorner->x;
                        dummysegment.m_End.y   = firstcorner->y;

                        Draw3D_Track( &dummysegment );
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
    }

    // draw graphic items
    EDA_ITEM* PtStruct;

    for( PtStruct = pcb->m_Drawings;  PtStruct != NULL;  PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_LINE_T:
            Draw3D_DrawSegment( (DRAWSEGMENT*) PtStruct );
            break;

        case PCB_TEXT_T:
            Draw3D_DrawText( (TEXTE_PCB*) PtStruct );
            break;

        default:
            break;
        }
    }

    // draw footprints
    MODULE* Module = pcb->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->Draw3D( this );
    }

    glEndList();

    // Test for errors
    CheckGLError();

    return m_gllist;
}


void EDA_3D_CANVAS::Draw3D_Track( TRACK* track )
{
    double zpos;
    int    layer = track->GetLayer();
    double ox, oy, fx, fy;
    double w;

    if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) == false )
        return;

    int color = g_ColorsSettings.GetLayerColor( layer );

    if( layer == LAST_COPPER_LAYER )
        layer = g_Parm_3D_Visu.m_Layers - 1;

    zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

    SetGLColor( color );
    glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );

    w  = track->m_Width * g_Parm_3D_Visu.m_BoardScale;
    ox = track->m_Start.x * g_Parm_3D_Visu.m_BoardScale;
    oy = track->m_Start.y * g_Parm_3D_Visu.m_BoardScale;
    fx = track->m_End.x * g_Parm_3D_Visu.m_BoardScale;
    fy = track->m_End.y * g_Parm_3D_Visu.m_BoardScale;
    Draw3D_FilledSegment( ox, -oy, fx, -fy, w, zpos );
}


void EDA_3D_CANVAS::Draw3D_SolidPolygonsInZones( ZONE_CONTAINER* aZone )
{
    double zpos;
    int    layer = aZone->GetLayer();

    if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) == false )
        return;

    int color = g_ColorsSettings.GetLayerColor( layer );

    if( layer == LAST_COPPER_LAYER )
        layer = g_Parm_3D_Visu.m_Layers - 1;

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

    //gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);

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
    for( layer = bottom_layer; layer < g_Parm_3D_Visu.m_Layers; layer++ )
    {
        zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

        if( layer < g_Parm_3D_Visu.m_Layers - 1 )
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

        // SetGLColor( LIGHTGRAY );
        glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );

        if( layer == LAYER_N_BACK )
            zpos = zpos - 5 * g_Parm_3D_Visu.m_BoardScale;
        else
            zpos = zpos + 5 * g_Parm_3D_Visu.m_BoardScale;

        Draw3D_FilledCircle( x, -y, r, hole, zpos );

        if( layer >= top_layer )
            break;
    }

    // Drawing hole:
    color = g_ColorsSettings.GetItemColor( VIAS_VISIBLE + via->m_Shape );
    SetGLColor( color );
    height = g_Parm_3D_Visu.m_LayerZcoord[top_layer] - g_Parm_3D_Visu.m_LayerZcoord[bottom_layer];
    Draw3D_FilledCylinder( x, -y, hole, height, g_Parm_3D_Visu.m_LayerZcoord[bottom_layer] );
}


void EDA_3D_CANVAS::Draw3D_DrawSegment( DRAWSEGMENT* segment )
{
    double x, y, xf, yf;
    double zpos, w;

    int    layer = segment->GetLayer();

    if( g_Parm_3D_Visu.m_BoardSettings->IsLayerVisible( layer ) == false )
        return;

    int color = g_ColorsSettings.GetLayerColor( layer );

    SetGLColor( color );

    w  = segment->GetWidth() * g_Parm_3D_Visu.m_BoardScale;

    x  = segment->GetStart().x * g_Parm_3D_Visu.m_BoardScale;
    y  = segment->GetStart().y * g_Parm_3D_Visu.m_BoardScale;

    xf = segment->GetEnd().x * g_Parm_3D_Visu.m_BoardScale;
    yf = segment->GetEnd().y * g_Parm_3D_Visu.m_BoardScale;

    if( layer == EDGE_N )
    {
        for( layer = 0; layer < g_Parm_3D_Visu.m_Layers; layer++ )
        {
            glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );
            zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

            switch( segment->GetShape() )
            {
            case S_ARC:
                Draw3D_ArcSegment( x, -y, xf, -yf, segment->GetAngle(), w, zpos );
                break;

            case S_CIRCLE:
                Draw3D_CircleSegment( x, -y, xf, -yf, w, zpos );
                break;

            default:
                Draw3D_FilledSegment( x, -y, xf, -yf, w, zpos );
                break;
            }
        }
    }
    else
    {
        glNormal3f( 0.0, 0.0, Get3DLayerSide( layer ) );
        zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

        if( Get3DLayerEnable( layer ) )
        {
            switch( segment->GetShape() )
            {
            case S_ARC:
                Draw3D_ArcSegment( x, -y, xf, -yf, segment->GetAngle(), w, zpos );
                break;

            case S_CIRCLE:
                Draw3D_CircleSegment( x, -y, xf, -yf, w, zpos );
                break;

            default:
                Draw3D_FilledSegment( x, -y, xf, -yf, w, zpos );
                break;
            }
        }
    }
}


static double s_Text3DWidth, s_Text3DZPos;
static void Draw3dTextSegm( int x0, int y0, int xf, int yf )
{
    double startx = x0 * g_Parm_3D_Visu.m_BoardScale;
    double starty = y0 * g_Parm_3D_Visu.m_BoardScale;
    double endx   = xf * g_Parm_3D_Visu.m_BoardScale;
    double endy   = yf * g_Parm_3D_Visu.m_BoardScale;

    Draw3D_FilledSegment( startx, -starty, endx, -endy, s_Text3DWidth, s_Text3DZPos );
}


void EDA_3D_CANVAS::Draw3D_DrawText( TEXTE_PCB* text )
{
    int layer = text->GetLayer();

    if( !Get3DLayerEnable( layer ) )
        return;

    int color = g_ColorsSettings.GetLayerColor( layer );

    SetGLColor( color );
    s_Text3DZPos  = g_Parm_3D_Visu.m_LayerZcoord[layer];
    s_Text3DWidth = text->GetThickness() * g_Parm_3D_Visu.m_BoardScale;
    glNormal3f( 0.0, 0.0, Get3DLayerSide( layer ) );
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
        for( int layer = 0; layer < g_Parm_3D_Visu.m_Layers; layer++ )
        {
            glNormal3f( 0.0, 0.0, (layer == LAYER_N_BACK) ? -1.0 : 1.0 );
            zpos = g_Parm_3D_Visu.m_LayerZcoord[layer];

            switch( m_Shape )
            {
            case S_SEGMENT:
                Draw3D_FilledSegment( x, -y, fx, -fy, w, zpos );
                break;

            case S_CIRCLE:
                Draw3D_CircleSegment( x, -y, fx, -fy, w, zpos );
                break;

            case S_ARC:
                Draw3D_ArcSegment( x, -y, fx, -fy, (double) m_Angle, w, zpos );
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
            Draw3D_FilledSegment( x, -y, fx, -fy, w, zpos );
            break;

        case S_CIRCLE:
            Draw3D_CircleSegment( x, -y, fx, -fy, w, zpos );
            break;

        case S_ARC:
            Draw3D_ArcSegment( x, -y, fx, -fy, (double) m_Angle, w, zpos );
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
    double  scale;
    double  zpos;
    wxPoint shape_pos;
    double  x, y, r, w, hole, holeX, holeY;
    double  drillx, drilly;
    bool    Oncu, Oncmp, Both;
    int     color;

    scale = g_Parm_3D_Visu.m_BoardScale;
    holeX = (double) m_Drill.x * scale / 2;
    holeY = (double) m_Drill.y * scale / 2;
    hole  = fmin( holeX, holeY );

    // Calculate the center of the pad.
    shape_pos = ReturnShapePos();
    ux0 = shape_pos.x;
    uy0 = shape_pos.y;
    xc  = ux0;
    yc  = uy0;

    dx = dx0 = m_Size.x >> 1;
    dy = dy0 = m_Size.y >> 1;

    angle  = m_Orient;
    drillx = m_Pos.x * scale;
    drilly = m_Pos.y * scale;

    // Draw the pad hole (TODO: draw OBLONG hole)
    if( holeX && holeY )
    {
        SetGLColor( DARKGRAY );
        Draw3D_FilledCylinder( drillx, -drilly, hole,
                               g_Parm_3D_Visu.m_LayerZcoord[LAYER_N_FRONT], 0.0 );
    }

    glNormal3f( 0.0, 0.0, 1.0 ); // Normal is Z axis
    nlmax = g_Parm_3D_Visu.m_Layers - 1;
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

            Draw3D_FilledCircle( x, -y, r, hole, zpos );
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

                Draw3D_FilledSegmentWithHole( ox, -oy, fx, -fy, w, drillx, -drilly, hole, zpos );
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


static void Draw3D_FilledCircle( double posx, double posy,
                                 double rayon, double hole, double zpos )
{
    int    ii, slice = 16;
    double x, y;

    glBegin( GL_QUAD_STRIP );

    for( ii = 0; ii <= slice; ii++ )
    {
        x = hole;
        y = 0.0;
        RotatePoint( &x, &y, ii * 225 );
        glVertex3f( x + posx, y + posy, zpos );
        x = rayon;
        y = 0.0;
        RotatePoint( &x, &y, ii * 225 );
        glVertex3f( x + posx, y + posy, zpos );
    }

    glEnd();
}


static void Draw3D_FilledCylinder( double posx, double posy, double rayon,
                                   double height, double zpos )
{
    int        ii;
    double     x, y;

#define NB_SEGM 12
    std::vector< S3D_Vertex > coords;
    coords.resize( 4 );

    double     tmp = DataScale3D;

    DataScale3D = 1.0; // Coordinate is already in range for Set_Object_Data();
    coords[0].x = coords[1].x = posx + rayon;
    coords[0].y = coords[1].y = posy;
    coords[0].z = coords[3].z = zpos;
    coords[1].z = coords[2].z = zpos + height;

    for( ii = 0; ii <= NB_SEGM; ii++ )
    {
        x = rayon;
        y = 0.0;
        RotatePoint( &x, &y, ii * (3600 / NB_SEGM) );
        coords[2].x = coords[3].x = posx + x;
        coords[2].y = coords[3].y = posy + y;
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
static void Draw3D_FilledSegment( double startx, double starty, double endx,
                                  double endy, double width, double zpos )
{
    double dx, dy, x, y, firstx = 0, firsty = 0;
    int    ii, angle;

    // Calculate the coordinates of the segment assumed horizontal.
    // Then turn the strips of the desired angle.
    dx    = endx - startx;
    dy    = endy - starty;
    angle = (int) ( ( atan2( dy, dx ) * 1800 / M_PI ) + 0.5 );

    RotatePoint( &dx, &dy, angle );
    width /= 2;

    glBegin( GL_POLYGON );

    // Trace the flare to right (1st half polygon at the end of the segment)
    for( ii = 0; ii <= 8; ii++ )
    {
        x = 0.0;
        y = -width;
        RotatePoint( &x, &y, -ii * 225 );
        x += dx;
        RotatePoint( &x, &y, -angle );
        glVertex3f( startx + x, starty + y, zpos );

        if( ii == 0 )
        {
            firstx = startx + x;
            firsty = starty + y;
        }
    }

    // Rounding the left (2nd half polygon is the origin of the segment)
    for( ii = 0; ii <= 8; ii++ )
    {
        int jj = ii * 225;
        x = 0.0;
        y = width;
        RotatePoint( &x, &y, -angle - jj );
        glVertex3f( startx + x, starty + y, zpos );
    }

    glVertex3f( firstx, firsty, zpos );
    glEnd();
}


/* Draw a polygon similar to a segment ends with round hole
 */
static void Draw3D_FilledSegmentWithHole( double startx, double starty,
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
    // All calculations are done with startx, starty as the origin of the route
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


static void Draw3D_ArcSegment( double startx, double starty, double centrex,
                               double centrey, double arc_angle, double width, double zpos )
{
    int    ii;
    int    slice = 36;             // Number of segments to approximate a circle by segments
    double hole, rayon;
    double arcStart_Angle;

    arcStart_Angle = (atan2( startx - centrex, starty - centrey ) * 1800 / M_PI );
    rayon = hypot( startx - centrex, starty - centrey ) + ( width / 2);
    hole  = rayon - width;

    // Calculate the number of segments to approximate this arc
    int imax = (int) ( (double) arc_angle * slice / 3600.0 );

    if( imax < 0 )
        imax = -imax;

    if( imax == 0 )
        imax = 1;

    // Adjust delta_angle to have exactly imax segments in arc_angle
    // i.e. arc_angle = imax delta_agnle.
    double delta_angle = (double) arc_angle / imax;

    glBegin( GL_QUAD_STRIP );

    for( ii = 0; ii <= imax; ii++ )
    {
        double angle = (double) ii * delta_angle;
        angle += arcStart_Angle + 900;
        double dx = hole;
        double dy = 0.0;
        RotatePoint( &dx, &dy, (int) angle );
        glVertex3f( dx + startx, dy + starty, zpos );
        dx = rayon;
        dy = 0.0;
        RotatePoint( &dx, &dy, (int) angle );
        glVertex3f( dx + startx, dy + starty, zpos );
    }

    glEnd();
}


static void Draw3D_CircleSegment( double startx, double starty, double endx,
                                  double endy, double width, double zpos )
{
    int    ii, slice = 36;
    double x, y, hole, rayon;

    rayon = hypot( startx - endx, starty - endy ) + ( width / 2);
    hole  = rayon - width;

    glBegin( GL_QUAD_STRIP );

    for( ii = 0; ii <= slice; ii++ )
    {
        x = hole; y = 0.0;
        RotatePoint( &x, &y, ii * 3600 / slice );
        glVertex3f( x + startx, y + starty, zpos );
        x = rayon; y = 0.0;
        RotatePoint( &x, &y, ii * 3600 / slice );
        glVertex3f( x + startx, y + starty, zpos );
    }

    glEnd();
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

    //gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);

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


static int Get3DLayerEnable( int act_layer )
{
    int i = -1;
    // see if layer needs to be shown
    // check the flags
    switch (act_layer)
    {
        case DRAW_N:
            i=g_Parm_3D_Visu.FL_DRAWINGS;
            break;

        case COMMENT_N:
            i=g_Parm_3D_Visu.FL_COMMENTS;
            break;

        case ECO1_N:
            i=g_Parm_3D_Visu.FL_ECO1;
            break;

        case ECO2_N:
            i=g_Parm_3D_Visu.FL_ECO2;
            break;
    }
    // the layer was not a layer with a flag, so show it
    if (i < 0)
        return true;

    // if the layer has a flag, return the flag
    return g_Parm_3D_Visu.m_DrawFlags[i];
}


static GLfloat Get3DLayerSide( int act_layer )
{
    GLfloat nZ;

    nZ = 1.0;

    if( ( act_layer <= LAST_COPPER_LAYER - 1 )
       || ( act_layer == ADHESIVE_N_BACK )
       || ( act_layer == SOLDERPASTE_N_BACK )
       || ( act_layer == SILKSCREEN_N_BACK )
       || ( act_layer == SOLDERMASK_N_BACK ) )
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
