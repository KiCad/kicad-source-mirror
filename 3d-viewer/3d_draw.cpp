/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 *
 */

#include <fctsys.h>
#include <common.h>
#include <trigo.h>
#include <pcbstruct.h>
#include <drawtxt.h>
#include <layers_id_colors_and_visibility.h>

#include <wxBasePcbFrame.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <colors_selection.h>
#include <convert_basic_shapes_to_polygon.h>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <gal/opengl/opengl_compositor.h>
#ifdef __WINDOWS__
#include <GL/glew.h>        // must be included before gl.h
#endif

#include <3d_viewer.h>
#include <3d_canvas.h>
#include <info3d_visu.h>
#include <trackball.h>
#include <3d_draw_basic_functions.h>

#include <CImage.h>
#include <reporter.h>


extern bool useFastModeForPolygons;

/* returns the Z orientation parameter 1.0 or -1.0 for aLayer
 * Z orientation is 1.0 for all layers but "back" layers:
 *  B_Cu , B_Adhes, B_Paste ), B_SilkS
 * used to calculate the Z orientation parameter for glNormal3f
 */
GLfloat  Get3DLayer_Z_Orientation( LAYER_NUM aLayer );


/**
 * Class STATUS_TEXT_REPORTER
 * is a wrapper for reporting to a wxString in a wxFrame status text.
 */
class STATUS_TEXT_REPORTER : public REPORTER
{
    wxFrame * m_frame;
    int m_position;
    bool m_hasMessage;


public:
    STATUS_TEXT_REPORTER( wxFrame* aFrame, int aPosition = 0 ) :
        REPORTER(),
        m_frame( aFrame ), m_position( aPosition )
    {
        m_hasMessage = false;
    }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_UNDEFINED )
    {
        if( !aText.IsEmpty() )
            m_hasMessage = true;

        m_frame->SetStatusText( aText, m_position );
        return *this;
    }

    bool HasMessage() const { return m_hasMessage; }
};


void EDA_3D_CANVAS::create_and_render_shadow_buffer( GLuint *aDst_gl_texture,
        GLuint aTexture_size, bool aDraw_body, int aBlurPasses )
{
    glDisable( GL_TEXTURE_2D );

    glViewport( 0, 0, aTexture_size, aTexture_size);

    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Render body and shapes

    if( aDraw_body && m_glLists[GL_ID_BODY] )
        glCallList( m_glLists[GL_ID_BODY] );

    if( m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] )
        glCallList( m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] );

    // Create and Initialize the float depth buffer

    float *depthbufferFloat = (float*) malloc( aTexture_size * aTexture_size * sizeof(float) );

    for( unsigned int i = 0; i < (aTexture_size * aTexture_size); i++ )
        depthbufferFloat[i] = 1.0f;

    glPixelStorei( GL_PACK_ALIGNMENT, 4 );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
    glReadBuffer( GL_BACK_LEFT );
    glReadPixels( 0, 0,
                  aTexture_size, aTexture_size,
                  GL_DEPTH_COMPONENT, GL_FLOAT, depthbufferFloat );

    CheckGLError( __FILE__, __LINE__ );

    glEnable( GL_TEXTURE_2D );
    glGenTextures( 1, aDst_gl_texture );
    glBindTexture( GL_TEXTURE_2D, *aDst_gl_texture );

    CIMAGE imgDepthBuffer( aTexture_size, aTexture_size );
    CIMAGE imgDepthBufferAux( aTexture_size, aTexture_size );

    imgDepthBuffer.SetPixelsFromNormalizedFloat( depthbufferFloat );

    free( depthbufferFloat );

    // Debug texture image
    //wxString filename;
    //filename.Printf( "imgDepthBuffer_%04d", *aDst_gl_texture );
    //imgDepthBuffer.SaveAsPNG( filename );

    while( aBlurPasses > 0 )
    {
        aBlurPasses--;
        imgDepthBufferAux.EfxFilter( &imgDepthBuffer, FILTER_GAUSSIAN_BLUR );
        imgDepthBuffer.EfxFilter( &imgDepthBufferAux, FILTER_GAUSSIAN_BLUR );
    }

    // Debug texture image
    //filename.Printf( "imgDepthBuffer_blur%04d", *aDst_gl_texture );
    //imgDepthBuffer.SaveAsPNG( filename );

    unsigned char *depthbufferRGBA = (unsigned char*) malloc( aTexture_size * aTexture_size * 4 );
    unsigned char *pPixels = imgDepthBuffer.GetBuffer();

    // Convert it to a RGBA buffer
    for( unsigned int i = 0; i < (aTexture_size * aTexture_size); i++ )
    {
        depthbufferRGBA[i * 4 + 0] = 0;
        depthbufferRGBA[i * 4 + 1] = 0;
        depthbufferRGBA[i * 4 + 2] = 0;
        depthbufferRGBA[i * 4 + 3] = 255 - pPixels[i];                  // Store in alpha channel the inversion of the image
    }

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, aTexture_size, aTexture_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, depthbufferRGBA );

    free( depthbufferRGBA );

    CheckGLError( __FILE__, __LINE__ );
}

/// Scale factor to make a bigger BBox in order to blur the texture and dont have artifacts in the edges
#define SHADOW_BOUNDING_BOX_SCALE 1.25f

void EDA_3D_CANVAS::generateFakeShadowsTextures( REPORTER* aErrorMessages, REPORTER* aActivity )
{
    if( m_shadow_init == true )
    {
        return;
    }

    // Init info 3d parameters and create gl lists:
    CreateDrawGL_List( aErrorMessages, aActivity );

    DBG( unsigned strtime = GetRunningMicroSecs() );

    m_shadow_init = true;

    glClearColor( 0, 0, 0, 1 );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    const float zDistMax = Millimeter2iu( 3.5 ) * GetPrm3DVisu().m_BiuTo3Dunits;

    glOrtho( -GetPrm3DVisu().m_BoardSize.x * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             GetPrm3DVisu().m_BoardSize.x * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             -GetPrm3DVisu().m_BoardSize.y * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             GetPrm3DVisu().m_BoardSize.y * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             0.0, zDistMax );

    float zpos = GetPrm3DVisu().GetLayerZcoordBIU( F_Paste ) * GetPrm3DVisu().m_BiuTo3Dunits;

    // Render FRONT shadow
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0f, 0.0f, zpos );
    glRotatef( 180.0f, 0.0f, 1.0f, 0.0f );

    // move the board in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -GetPrm3DVisu().m_BoardPos.x * GetPrm3DVisu().m_BiuTo3Dunits,
                  -GetPrm3DVisu().m_BoardPos.y * GetPrm3DVisu().m_BiuTo3Dunits,
                  0.0f );

    create_and_render_shadow_buffer( &m_text_fake_shadow_front, 512, false, 1 );

    zpos = GetPrm3DVisu().GetLayerZcoordBIU( B_Paste ) * GetPrm3DVisu().m_BiuTo3Dunits;

    // Render BACK shadow
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0f, 0.0f, fabs( zpos ) );


    // move the board in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -GetPrm3DVisu().m_BoardPos.x * GetPrm3DVisu().m_BiuTo3Dunits,
                  -GetPrm3DVisu().m_BoardPos.y * GetPrm3DVisu().m_BiuTo3Dunits,
                  0.0f );

    create_and_render_shadow_buffer( &m_text_fake_shadow_back, 512, false, 1 );


    // Render ALL BOARD shadow
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    // Normalization scale to convert bouding box
    // to normalize 3D units between -1.0 and +1.0

    S3D_VERTEX v = m_fastAABBox_Shadow.Max() - m_fastAABBox_Shadow.Min();
    float BoundingBoxBoardiuTo3Dunits = 2.0f / glm::max( v.x, v.y );

    //float zDistance = (m_lightPos.z * zDistMax) / sqrt( (m_lightPos.z - m_fastAABBox_Shadow.Min().z) );
    float zDistance = (m_lightPos.z - m_fastAABBox_Shadow.Min().z) / 3.0f;

    glOrtho( -v.x * BoundingBoxBoardiuTo3Dunits / 2.0f,
              v.x * BoundingBoxBoardiuTo3Dunits / 2.0f,
             -v.y * BoundingBoxBoardiuTo3Dunits / 2.0f,
              v.y * BoundingBoxBoardiuTo3Dunits / 2.0f,
             0.0f, zDistance );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    // fits the bouding box to scale this size
    glScalef( BoundingBoxBoardiuTo3Dunits, BoundingBoxBoardiuTo3Dunits, 1.0f );

    // Place the eye in the lowerpoint of boudingbox and turn arround and look up to the model
    glTranslatef( 0.0f, 0.0f, m_fastAABBox_Shadow.Min().z );
    glRotatef( 180.0, 0.0f, 1.0f, 0.0f );

    // move the bouding box in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -(m_fastAABBox_Shadow.Min().x + v.x / 2.0f), -(m_fastAABBox_Shadow.Min().y + v.y / 2.0f), 0.0f );

    create_and_render_shadow_buffer( &m_text_fake_shadow_board, 512, true, 10 );

    DBG( printf( "  generateFakeShadowsTextures total time %f ms\n", (double) (GetRunningMicroSecs() - strtime) / 1000.0 ) );
}


void EDA_3D_CANVAS::Redraw()
{
    // SwapBuffer requires the window to be shown before calling
    if( !IsShownOnScreen() )
        return;

    wxString err_messages;
    WX_STRING_REPORTER errorReporter( &err_messages );
    STATUS_TEXT_REPORTER activityReporter( Parent(), 0 );

    // Display build time at the end of build
    unsigned strtime = GetRunningMicroSecs();

    SetCurrent( *m_glRC );

    // Set the OpenGL viewport according to the client size of this canvas.
    // This is done here rather than in a wxSizeEvent handler because our
    // OpenGL rendering context (and thus viewport setting) is used with
    // multiple canvases: If we updated the viewport in the wxSizeEvent
    // handler, changing the size of one canvas causes a viewport setting that
    // is wrong when next another canvas is repainted.
    wxSize size = GetClientSize();

    InitGL();

    if( isRealisticMode() && isEnabled( FL_RENDER_SHADOWS ) )
    {
        generateFakeShadowsTextures( &errorReporter, &activityReporter );
    }

    // *MUST* be called *after*  SetCurrent( ):
    glViewport( 0, 0, size.x, size.y );

    // clear color and depth buffers
    glClearColor( 0.95, 0.95, 1.0, 1.0 );
    glClearStencil( 0 );
    glClearDepth( 1.0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    glShadeModel( GL_SMOOTH );

    // Draw background
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glDisable( GL_LIGHTING );
    glDisable( GL_COLOR_MATERIAL );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );

    // Draw the background ( rectangle with color gradient)
    glBegin( GL_QUADS );
    SetGLColor( GetPrm3DVisu().m_BgColor_Top, 1.0 );
    glVertex2f( -1.0, 1.0 );    // Top left corner

    SetGLColor( GetPrm3DVisu().m_BgColor, 1.0 );
    glVertex2f( -1.0,-1.0 );    // bottom left corner
    glVertex2f( 1.0,-1.0 );     // bottom right corner

    SetGLColor( GetPrm3DVisu().m_BgColor_Top, 1.0 );
    glVertex2f( 1.0, 1.0 );     // top right corner

    glEnd();
    glEnable( GL_DEPTH_TEST );


    // set viewing projection
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

#define MAX_VIEW_ANGLE 160.0 / 45.0

    if( GetPrm3DVisu().m_Zoom > MAX_VIEW_ANGLE )
        GetPrm3DVisu().m_Zoom = MAX_VIEW_ANGLE;

     if( Parent()->ModeIsOrtho() )
     {
         // OrthoReductionFactor is chosen to provide roughly the same size as
         // Perspective View
         const double orthoReductionFactor = 400 / GetPrm3DVisu().m_Zoom;

         // Initialize Projection Matrix for Ortographic View
         glOrtho( -size.x / orthoReductionFactor, size.x / orthoReductionFactor,
                  -size.y / orthoReductionFactor, size.y / orthoReductionFactor, 1, 100 );
     }
     else
     {
         // Ratio width / height of the window display
         double ratio_HV = (double) size.x / size.y;

         // Initialize Projection Matrix for Perspective View
         gluPerspective( 45.0f * GetPrm3DVisu().m_Zoom, ratio_HV, 1, 100 );
     }

    // position viewer
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glTranslatef( 0.0f, 0.0f, -(m_ZBottom + m_ZTop) / 2.0f );

    // Setup light sources:
    SetLights();

    CheckGLError( __FILE__, __LINE__ );

    glMatrixMode( GL_MODELVIEW );    // position viewer

    // transformations
    GLfloat mat[4][4];

    // Translate motion first, so rotations don't mess up the orientation...
    glTranslatef( m_draw3dOffset.x, m_draw3dOffset.y, 0.0F );

    build_rotmatrix( mat, GetPrm3DVisu().m_Quat );
    glMultMatrixf( &mat[0][0] );

    glRotatef( GetPrm3DVisu().m_Rot[0], 1.0, 0.0, 0.0 );
    glRotatef( GetPrm3DVisu().m_Rot[1], 0.0, 1.0, 0.0 );
    glRotatef( GetPrm3DVisu().m_Rot[2], 0.0, 0.0, 1.0 );


    if( ! m_glLists[GL_ID_BOARD] || ! m_glLists[GL_ID_TECH_LAYERS] )
        CreateDrawGL_List( &errorReporter, &activityReporter );

    if( isEnabled( FL_AXIS ) && m_glLists[GL_ID_AXIS] )
        glCallList( m_glLists[GL_ID_AXIS] );

    // move the board in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -GetPrm3DVisu().m_BoardPos.x * GetPrm3DVisu().m_BiuTo3Dunits,
                  -GetPrm3DVisu().m_BoardPos.y * GetPrm3DVisu().m_BiuTo3Dunits,
                  0.0f );

    if( isEnabled( FL_MODULE ) )
    {
        if( ! m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] )
            CreateDrawGL_List( &errorReporter, &activityReporter );
    }

    glEnable( GL_LIGHTING );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    if( isRealisticMode() && isEnabled( FL_RENDER_TEXTURES ) )
        glEnable( GL_TEXTURE_2D );
    else
        glDisable( GL_TEXTURE_2D );

    // Set material for the board
    glEnable( GL_COLOR_MATERIAL );
    SetOpenGlDefaultMaterial();

    //glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, FALSE );

    // Board Body

    GLint shininess_value = 32;
    glMateriali( GL_FRONT_AND_BACK, GL_SHININESS, shininess_value );

    if( isEnabled( FL_SHOW_BOARD_BODY ) )
    {
        if( m_glLists[GL_ID_BODY] )
        {
            glCallList( m_glLists[GL_ID_BODY] );
        }
    }


    // Board

    // specify material parameters for the lighting model.
    shininess_value = 52;
    glMateriali( GL_FRONT_AND_BACK, GL_SHININESS, shininess_value );

    glm::vec4 specular( GetPrm3DVisu().m_CopperColor.m_Red   * 0.20f,
                        GetPrm3DVisu().m_CopperColor.m_Green * 0.20f,
                        GetPrm3DVisu().m_CopperColor.m_Blue  * 0.20f, 1.0f );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.x );

    if( m_glLists[GL_ID_BOARD] )
    {
        glCallList( m_glLists[GL_ID_BOARD] );
    }


    // Tech layers

    shininess_value = 32;
    glMateriali( GL_FRONT_AND_BACK, GL_SHININESS, shininess_value );

    glm::vec4 specularTech( 0.0f, 0.0f, 0.0f, 1.0f );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specularTech.x );

    if( m_glLists[GL_ID_TECH_LAYERS] )
    {
        glCallList( m_glLists[GL_ID_TECH_LAYERS] );
    }

    if( isEnabled( FL_COMMENTS ) || isEnabled( FL_ECO )  )
    {
        if( ! m_glLists[GL_ID_AUX_LAYERS] )
            CreateDrawGL_List( &errorReporter, &activityReporter );

        glCallList( m_glLists[GL_ID_AUX_LAYERS] );
    }

    //glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, TRUE );

    // Draw Component Shadow

    if( isEnabled( FL_MODULE )  && isRealisticMode() &&
        isEnabled( FL_RENDER_SHADOWS ) )
    {
        glEnable( GL_CULL_FACE );
        glDisable( GL_DEPTH_TEST );

        glEnable( GL_COLOR_MATERIAL ) ;
        SetOpenGlDefaultMaterial();
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        glEnable( GL_TEXTURE_2D );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        if( m_glLists[GL_ID_SHADOW_FRONT] )
        {
            glBindTexture( GL_TEXTURE_2D, m_text_fake_shadow_front );
            glCallList( m_glLists[GL_ID_SHADOW_FRONT] );
        }

        if( m_glLists[GL_ID_SHADOW_BACK] )
        {
            glBindTexture( GL_TEXTURE_2D, m_text_fake_shadow_back );
            glCallList( m_glLists[GL_ID_SHADOW_BACK] );
        }
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        glEnable( GL_DEPTH_TEST );
        glDisable( GL_TEXTURE_2D );
        glDisable( GL_CULL_FACE );
    }

    glEnable( GL_COLOR_MATERIAL );
    SetOpenGlDefaultMaterial();

    glDisable( GL_BLEND );


    // Draw Solid Shapes

    if( isEnabled( FL_MODULE ) )
    {
        if( ! m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] )
            CreateDrawGL_List( &errorReporter, &activityReporter );

        glCallList( m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] );
    }

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


    // Grid uses transparency: draw it after all objects

    if( isEnabled( FL_GRID ) )
    {
        if( ! m_glLists[GL_ID_GRID] )
            CreateDrawGL_List( &errorReporter, &activityReporter );

        glCallList( m_glLists[GL_ID_GRID] );
    }


    // Draw Board Shadow

    if( isRealisticMode() && isEnabled( FL_RENDER_SHADOWS ) )
    {
        if( m_glLists[GL_ID_SHADOW_BOARD] )
        {
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glColor4f( 1.0, 1.0, 1.0, 0.75f );
            glEnable( GL_CULL_FACE );
            glDisable( GL_COLOR_MATERIAL );
            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, m_text_fake_shadow_board );
            glCallList( m_glLists[GL_ID_SHADOW_BOARD] );
            glDisable( GL_CULL_FACE );
            glDisable( GL_TEXTURE_2D );
        }
    }

    // This list must be drawn last, because it contains the
    // transparent gl objects, which should be drawn after all
    // non transparent objects
    if(  isEnabled( FL_MODULE ) && m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] )
    {
        glEnable( GL_COLOR_MATERIAL );
        SetOpenGlDefaultMaterial();
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glCallList( m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] );
    }

    // Debug bounding boxes
    /*
    glDisable( GL_BLEND );
    glDisable( GL_COLOR_MATERIAL );
    glDisable( GL_LIGHTING );
    glColor4f( 1.0f, 0.0f, 1.0f, 1.0f );
    m_fastAABBox_Shadow.GLdebug();

    glColor4f( 0.0f, 1.0f, 1.0f, 1.0f );
    m_boardAABBox.GLdebug();
    */

    SwapBuffers();

    // Show calculation time if some activity was reported
    if( activityReporter.HasMessage() )
    {
        // Calculation time in seconds
        double calculation_time = (double)( GetRunningMicroSecs() - strtime) / 1e6;

        activityReporter.Report( wxString::Format( _( "Build time %.3f s" ),
                                 calculation_time ) );
    }
    else
        activityReporter.Report( wxEmptyString );

    if( !err_messages.IsEmpty() )
        wxLogMessage( err_messages );

}


/**
 * Function buildBoard3DAuxLayers
 * Called by CreateDrawGL_List()
 * Fills the OpenGL GL_ID_BOARD draw list with items
 * on aux layers only
 */
void EDA_3D_CANVAS::buildBoard3DAuxLayers( REPORTER* aErrorMessages, REPORTER* aActivity )
{
    const int   segcountforcircle   = 18;
    double      correctionFactor    = 1.0 / cos( M_PI / (segcountforcircle * 2) );
    BOARD*      pcb = GetBoard();

    SHAPE_POLY_SET  bufferPolys;

    static const LAYER_ID sequence[] = {
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
        Edge_Cuts,
        Margin
    };

    for( LSEQ aux( sequence, sequence+DIM(sequence) );  aux;  ++aux )
    {
        LAYER_ID layer = *aux;

        if( !is3DLayerEnabled( layer ) )
            continue;

        if( aActivity )
            aActivity->Report( wxString::Format( _( "Build layer %s" ), LSET::Name( layer ) ) );

        bufferPolys.RemoveAllContours();

        for( BOARD_ITEM* item = pcb->m_Drawings; item; item = item->Next() )
        {
            if( !item->IsOnLayer( layer ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:
                ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                    bufferPolys, 0, segcountforcircle, correctionFactor );
                break;

            case PCB_TEXT_T:
                ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet(
                    bufferPolys, 0, segcountforcircle, correctionFactor );
                break;

            default:
                break;
            }
        }

        for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
        {
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
        if( bufferPolys.IsEmpty() )
            continue;

        bufferPolys.Simplify( useFastModeForPolygons );

        int         thickness = GetPrm3DVisu().GetLayerObjectThicknessBIU( layer );
        int         zpos = GetPrm3DVisu().GetLayerZcoordBIU( layer );
        // for Draw3D_SolidHorizontalPolyPolygons,
        // zpos it the middle between bottom and top sides.
        // However for top layers, zpos should be the bottom layer pos,
        // and for bottom layers, zpos should be the top layer pos.
        if( Get3DLayer_Z_Orientation( layer ) > 0 )
            zpos += thickness/2;
        else
            zpos -= thickness/2 ;

        float zNormal = 1.0f; // When using thickness it will draw first the top and then botton (with z inverted)

        // If we are not using thickness, then the znormal must face the layer direction
        // because it will draw just one plane
        if( !thickness )
            zNormal = Get3DLayer_Z_Orientation( layer );

        setGLTechLayersColor( layer );
        Draw3D_SolidHorizontalPolyPolygons( bufferPolys, zpos,
                                            thickness, GetPrm3DVisu().m_BiuTo3Dunits, false,
                                            zNormal );
    }
}

void EDA_3D_CANVAS::buildShadowList( GLuint aFrontList, GLuint aBacklist, GLuint aBoardList )
{
    // Board shadows are based on board dimension.

    float xmin    = m_boardAABBox.Min().x;
    float xmax    = m_boardAABBox.Max().x;
    float ymin    = m_boardAABBox.Min().y;
    float ymax    = m_boardAABBox.Max().y;

    float zpos = GetPrm3DVisu().GetLayerZcoordBIU( F_Paste ) * GetPrm3DVisu().m_BiuTo3Dunits;

    // Shadow FRONT
    glNewList( aFrontList, GL_COMPILE );

    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( F_Paste ) );

    glBegin (GL_QUADS);
    glTexCoord2f( 1.0, 0.0 ); glVertex3f( xmin, ymin, zpos );
    glTexCoord2f( 0.0, 0.0 ); glVertex3f( xmax, ymin, zpos );
    glTexCoord2f( 0.0, 1.0 ); glVertex3f( xmax, ymax, zpos );
    glTexCoord2f( 1.0, 1.0 ); glVertex3f( xmin, ymax, zpos );
    glEnd();

    glEndList();


    // Shadow BACK
    zpos = GetPrm3DVisu().GetLayerZcoordBIU( B_Paste ) * GetPrm3DVisu().m_BiuTo3Dunits;

    glNewList( aBacklist, GL_COMPILE );

    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( B_Paste ) );

    glBegin (GL_QUADS);
    glTexCoord2f( 0.0, 0.0 ); glVertex3f( xmin, ymin, zpos );
    glTexCoord2f( 0.0, 1.0 ); glVertex3f( xmin, ymax, zpos );
    glTexCoord2f( 1.0, 1.0 ); glVertex3f( xmax, ymax, zpos );
    glTexCoord2f( 1.0, 0.0 ); glVertex3f( xmax, ymin, zpos );
    glEnd();

    glEndList();

    // Shadow BOARD

    // Floor shadow is based on axis alighned bounding box dimension
    xmin = m_fastAABBox_Shadow.Min().x;
    xmax = m_fastAABBox_Shadow.Max().x;
    ymin = m_fastAABBox_Shadow.Min().y;
    ymax = m_fastAABBox_Shadow.Max().y;

    glNewList( aBoardList, GL_COMPILE );
    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( F_Paste ) );

    glBegin (GL_QUADS);
    glTexCoord2f( 1.0, 0.0 ); glVertex3f( xmin, ymin, m_fastAABBox_Shadow.Min().z );
    glTexCoord2f( 0.0, 0.0 ); glVertex3f( xmax, ymin, m_fastAABBox_Shadow.Min().z );
    glTexCoord2f( 0.0, 1.0 ); glVertex3f( xmax, ymax, m_fastAABBox_Shadow.Min().z );
    glTexCoord2f( 1.0, 1.0 ); glVertex3f( xmin, ymax, m_fastAABBox_Shadow.Min().z );
    glEnd();

    glEndList();
}


void EDA_3D_CANVAS::CreateDrawGL_List( REPORTER* aErrorMessages, REPORTER* aActivity )
{
    BOARD* pcb = GetBoard();

    wxBusyCursor    dummy;

    // Build 3D board parameters:
    GetPrm3DVisu().InitSettings( pcb );

    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    // Create axis gl list (if it is not shown, the list will be not called
    draw3DAxis();

    // Create Board full gl lists:

    if( ! m_glLists[GL_ID_BOARD] )
    {
        DBG( unsigned strtime = GetRunningMicroSecs() );

        m_glLists[GL_ID_BOARD] = glGenLists( 1 );
        m_glLists[GL_ID_BODY] = glGenLists( 1 );
        buildBoard3DView(m_glLists[GL_ID_BOARD], m_glLists[GL_ID_BODY], aErrorMessages, aActivity );
        CheckGLError( __FILE__, __LINE__ );

        DBG( printf( "  buildBoard3DView total time %f ms\n", (double) (GetRunningMicroSecs() - strtime) / 1000.0 ) );
    }

    if( ! m_glLists[GL_ID_TECH_LAYERS] )
    {
        DBG( unsigned strtime = GetRunningMicroSecs() );

        m_glLists[GL_ID_TECH_LAYERS] = glGenLists( 1 );
        glNewList( m_glLists[GL_ID_TECH_LAYERS], GL_COMPILE );
        // when calling BuildTechLayers3DView,
        // do not show warnings, which are the same as buildBoard3DView
        buildTechLayers3DView( aErrorMessages, aActivity );
        glEndList();
        CheckGLError( __FILE__, __LINE__ );

        DBG( printf( "  buildTechLayers3DView total time %f ms\n", (double) (GetRunningMicroSecs() - strtime) / 1000.0 ) );
    }

    if( ! m_glLists[GL_ID_AUX_LAYERS] )
    {
        DBG( unsigned strtime = GetRunningMicroSecs() );

        m_glLists[GL_ID_AUX_LAYERS] = glGenLists( 1 );
        glNewList( m_glLists[GL_ID_AUX_LAYERS], GL_COMPILE );
        buildBoard3DAuxLayers( aErrorMessages, aActivity );
        glEndList();
        CheckGLError( __FILE__, __LINE__ );

        DBG( printf( "  buildBoard3DAuxLayers total time %f ms\n", (double) (GetRunningMicroSecs() - strtime) / 1000.0 ) );
    }

    // draw modules 3D shapes
    if( ! m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] && isEnabled( FL_MODULE ) )
    {
        m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] = glGenLists( 1 );

        // GL_ID_3DSHAPES_TRANSP_FRONT is an auxiliary list for 3D shapes;
        // Ensure it is cleared before rebuilding it
        if( m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] )
            glDeleteLists( m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT], 1 );

        bool useMaterial = g_Parm_3D_Visu.GetFlag( FL_RENDER_MATERIAL );

        if( useMaterial )
            m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] = glGenLists( 1 );
        else
            m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] = 0;

        buildFootprintShape3DList( m_glLists[GL_ID_3DSHAPES_SOLID_FRONT],
                                   m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT],
                                   aErrorMessages, aActivity );

        CheckGLError( __FILE__, __LINE__ );
    }

    calcBBox();

    // Create grid gl list
    if( ! m_glLists[GL_ID_GRID] )
    {
        m_glLists[GL_ID_GRID] = glGenLists( 1 );
        glNewList( m_glLists[GL_ID_GRID], GL_COMPILE );

        draw3DGrid( GetPrm3DVisu().m_3D_Grid );

        glEndList();
    }

    if( !m_glLists[GL_ID_SHADOW_FRONT] )
        m_glLists[GL_ID_SHADOW_FRONT] = glGenLists( 1 );

    if( !m_glLists[GL_ID_SHADOW_BACK] )
        m_glLists[GL_ID_SHADOW_BACK]  = glGenLists( 1 );

    if( !m_glLists[GL_ID_SHADOW_BOARD] )
        m_glLists[GL_ID_SHADOW_BOARD] = glGenLists( 1 );

    buildShadowList( m_glLists[GL_ID_SHADOW_FRONT],
                     m_glLists[GL_ID_SHADOW_BACK],
                     m_glLists[GL_ID_SHADOW_BOARD] );

    CheckGLError( __FILE__, __LINE__ );
}


void EDA_3D_CANVAS::calcBBox()
{
    BOARD* pcb = GetBoard();

    m_fastAABBox.Reset();

    for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
    {
        CBBOX tmpFastAABBox;

        // Compute the transformation matrix for this module based on translation, rotation and orientation.
        float  zpos = GetPrm3DVisu().GetModulesZcoord3DIU( module->IsFlipped() );
        wxPoint pos = module->GetPosition();

        glm::mat4 fullTransformMatrix;
        fullTransformMatrix = glm::translate( glm::mat4(),  S3D_VERTEX( (float)(pos.x * GetPrm3DVisu().m_BiuTo3Dunits),
                                                                        (float)(-pos.y * GetPrm3DVisu().m_BiuTo3Dunits),
                                                                        zpos ) );

        if( module->GetOrientation() )
            fullTransformMatrix = glm::rotate( fullTransformMatrix,
                                               glm::radians( (float)(module->GetOrientation() / 10.0f) ),
                                               S3D_VERTEX( 0.0f, 0.0f, 1.0f ) );

        if( module->IsFlipped() )
        {
            fullTransformMatrix = glm::rotate( fullTransformMatrix, glm::radians( 180.0f ), S3D_VERTEX( 0.0f, 1.0f, 0.0f ) );
            fullTransformMatrix = glm::rotate( fullTransformMatrix, glm::radians( 180.0f ), S3D_VERTEX( 0.0f, 0.0f, 1.0f ) );
        }

        // Compute a union bounding box for all the shapes of the model

        S3D_MASTER* shape3D = module->Models();

        for( ; shape3D; shape3D = shape3D->Next() )
        {
            if( shape3D->Is3DType( S3D_MASTER::FILE3D_VRML ) )
                tmpFastAABBox.Union( shape3D->getFastAABBox() );
        }

        tmpFastAABBox.ApplyTransformationAA( fullTransformMatrix );

        m_fastAABBox.Union( tmpFastAABBox );
    }

    // Create a board bounding box based on board size
    wxSize  brd_size = getBoardSize();
    wxPoint brd_center_pos = getBoardCenter();

    float xsize   = brd_size.x;
    float ysize   = brd_size.y;

    float scale   = GetPrm3DVisu().m_BiuTo3Dunits;
    float xmin    = (brd_center_pos.x - xsize / 2.0) * scale;
    float xmax    = (brd_center_pos.x + xsize / 2.0) * scale;
    float ymin    = (brd_center_pos.y - ysize / 2.0) * scale;
    float ymax    = (brd_center_pos.y + ysize / 2.0) * scale;

    float zmin = GetPrm3DVisu().GetLayerZcoordBIU( B_Adhes ) * scale;
    float zmax = GetPrm3DVisu().GetLayerZcoordBIU( F_Adhes ) * scale;

    m_boardAABBox = CBBOX(  S3D_VERTEX(xmin, ymin, zmin),
                            S3D_VERTEX(xmax, ymax, zmax) );

    // Add BB board with BB models and scale it a bit
    m_fastAABBox.Union( m_boardAABBox );
    m_fastAABBox_Shadow = m_fastAABBox;
    m_fastAABBox_Shadow.Scale( SHADOW_BOUNDING_BOX_SCALE );
}


void EDA_3D_CANVAS::buildFootprintShape3DList( GLuint aOpaqueList,
                                               GLuint aTransparentList,
                                               REPORTER* aErrorMessages,
                                               REPORTER* aActivity )
{
    DBG( unsigned strtime = GetRunningMicroSecs() );

    if( aActivity )
        aActivity->Report( _( "Load 3D Shapes" ) );

    // clean the parser list if it have any already loaded files
    m_model_parsers_list.clear();
    m_model_filename_list.clear();

    BOARD* pcb = GetBoard();

    for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
        read3DComponentShape( module );

    DBG( printf( "  read3DComponentShape total time %f ms\n", (double) (GetRunningMicroSecs() - strtime) / 1000.0 ) );

    DBG( strtime = GetRunningMicroSecs() );

    bool useMaterial = g_Parm_3D_Visu.GetFlag( FL_RENDER_MATERIAL );

    if( useMaterial )
    {
        // aOpaqueList is the gl list for non transparent items
        // aTransparentList is the gl list for non transparent items,
        // which need to be drawn after all other items

        glNewList( aOpaqueList, GL_COMPILE );
        bool loadOpaqueObjects = true;

        for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
            render3DComponentShape( module,  loadOpaqueObjects,
                                             !loadOpaqueObjects );

        glEndList();


        glNewList( aTransparentList, GL_COMPILE );
        bool loadTransparentObjects = true;

        for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
            render3DComponentShape( module, !loadTransparentObjects,
                                            loadTransparentObjects );

        glEndList();
    }
    else
    {
        // Just create one list
        glNewList( aOpaqueList, GL_COMPILE );

        for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
            render3DComponentShape( module, false, false );
        glEndList();
    }

    DBG( printf( "  render3DComponentShape total time %f ms\n", (double) (GetRunningMicroSecs() - strtime) / 1000.0 ) );
}


bool EDA_3D_CANVAS::read3DComponentShape( MODULE* module )
{
    if( module )
    {
        S3D_MASTER* shape3D = module->Models();

        for( ; shape3D; shape3D = shape3D->Next() )
        {
            if( shape3D->Is3DType( S3D_MASTER::FILE3D_VRML ) )
            {
                bool found = false;

                unsigned int i;
                wxString shape_filename = shape3D->GetShape3DFullFilename();

                // Search for already loaded files
                for( i = 0; i < m_model_filename_list.size(); i++ )
                {
                    if( shape_filename.Cmp(m_model_filename_list[i]) == 0 )
                    {
                        found = true;
                        break;
                    }
                }

                if( found == false )
                {
                    // Create a new parser
                    S3D_MODEL_PARSER *newParser = S3D_MODEL_PARSER::Create( shape3D, shape3D->GetShape3DExtension() );

                    if( newParser )
                    {
                        // Read file
                        if( shape3D->ReadData( newParser ) == 0 )
                        {
                            // Store this couple filename / parsed file
                            m_model_filename_list.push_back( shape_filename );
                            m_model_parsers_list.push_back( newParser );
                        }
                    }
                }
                else
                {
                    // Reusing file
                    shape3D->m_parser = m_model_parsers_list[i];
                }
            }
        }
    }

    return true;
}


void EDA_3D_CANVAS::render3DComponentShape( MODULE* module,
                                            bool aIsRenderingJustNonTransparentObjects,
                                            bool aIsRenderingJustTransparentObjects )
{
    double zpos = GetPrm3DVisu().GetModulesZcoord3DIU( module->IsFlipped() );

    glPushMatrix();

    wxPoint pos = module->GetPosition();

    glTranslatef(  pos.x * GetPrm3DVisu().m_BiuTo3Dunits,
                  -pos.y * GetPrm3DVisu().m_BiuTo3Dunits,
                   zpos );

    if( module->GetOrientation() )
        glRotatef( (double) module->GetOrientation() / 10.0, 0.0f, 0.0f, 1.0f );

    if( module->IsFlipped() )
    {
        glRotatef( 180.0f, 0.0f, 1.0f, 0.0f );
        glRotatef( 180.0f, 0.0f, 0.0f, 1.0f );
    }

    S3D_MASTER* shape3D = module->Models();

    for( ; shape3D; shape3D = shape3D->Next() )
    {
        if( shape3D->Is3DType( S3D_MASTER::FILE3D_VRML ) )
        {
            glPushMatrix();

            shape3D->Render( aIsRenderingJustNonTransparentObjects,
                             aIsRenderingJustTransparentObjects );

            if( isEnabled( FL_RENDER_SHOW_MODEL_BBOX ) )
            {
                // Set the alpha current color to opaque
                float currentColor[4];
                glGetFloatv( GL_CURRENT_COLOR,currentColor );
                currentColor[3] = 1.0f;
                glColor4fv( currentColor );

                CBBOX thisBBox = shape3D->getBBox();
                thisBBox.GLdebug();
            }

            glPopMatrix();

            // Debug AABBox
            //thisBBox = shape3D->getfastAABBox();
            //thisBBox.GLdebug();
        }
    }

    glPopMatrix();
}


bool EDA_3D_CANVAS::is3DLayerEnabled( LAYER_ID aLayer ) const
{
    DISPLAY3D_FLG flg;

    // see if layer needs to be shown
    // check the flags
    switch( aLayer )
    {
    case B_Adhes:
    case F_Adhes:
        flg = FL_ADHESIVE;
        break;

    case B_Paste:
    case F_Paste:
        flg = FL_SOLDERPASTE;
        break;

    case B_SilkS:
    case F_SilkS:
        flg = FL_SILKSCREEN;
        break;

    case B_Mask:
    case F_Mask:
        flg = FL_SOLDERMASK;
        break;

    case Dwgs_User:
    case Cmts_User:
        if( isRealisticMode() )
            return false;

        flg = FL_COMMENTS;
        break;

    case Eco1_User:
    case Eco2_User:
        if( isRealisticMode() )
            return false;

        flg = FL_ECO;
        break;

    case B_Cu:
    case F_Cu:
        return GetPrm3DVisu().m_BoardSettings->IsLayerVisible( aLayer )
               || isRealisticMode();
        break;

    default:
        // the layer is an internal copper layer, used the visibility
        //
        if( isRealisticMode() )
            return false;

        return GetPrm3DVisu().m_BoardSettings->IsLayerVisible( aLayer );
    }

    // The layer has a flag, return the flag
    return isEnabled( flg );
}
