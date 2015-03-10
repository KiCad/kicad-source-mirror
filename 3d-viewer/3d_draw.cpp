/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifdef __WINDOWS__
#include <GL/glew.h>        // must be included before gl.h
#endif

#include <3d_viewer.h>
#include <3d_canvas.h>
#include <info3d_visu.h>
#include <trackball.h>
#include <3d_draw_basic_functions.h>

/* returns the Z orientation parameter 1.0 or -1.0 for aLayer
 * Z orientation is 1.0 for all layers but "back" layers:
 *  B_Cu , B_Adhes, B_Paste ), B_SilkS
 * used to calculate the Z orientation parameter for glNormal3f
 */
static GLfloat  Get3DLayer_Z_Orientation( LAYER_NUM aLayer );


/* Based on the tutorial http://www.ulrichmierendorff.com/software/opengl_blur.html
 * It will blur a openGL texture
 */
static void blur_tex( GLuint aTex, int aPasses, GLuint aTexture_size )
{
    int i, x, y;

    glFlush();
    glFinish();

    glPushAttrib( GL_ALL_ATTRIB_BITS );

    glDisable( GL_LIGHTING );

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_ALPHA_TEST );
    glDisable( GL_COLOR_MATERIAL );

    glReadBuffer( GL_BACK_LEFT );
    glPixelStorei( GL_PACK_ALIGNMENT, 1 );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );


    glViewport( 0, 0, aTexture_size, aTexture_size );

    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();

    glOrtho( 0.0f, aTexture_size, aTexture_size, 0.0f, -1.0f, 1.0f );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, aTex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    while (aPasses > 0)
    {
        i = 0;
        for (x = 0; x < 2; x++)
        {
            for (y = 0; y < 2; y++, i++)
            {
                glColor4f (1.0f,1.0f,1.0f,1.0 / (i + 1.0));
                glBegin(GL_TRIANGLE_STRIP);
                glTexCoord2f( 0 + (x - 0.5)/aTexture_size, 1 + (y-0.5)/aTexture_size );
                glVertex2f( 0, 0 );
                glTexCoord2f( 0 + (x - 0.5)/aTexture_size, 0 + (y-0.5)/aTexture_size );
                glVertex2f( 0, aTexture_size );
                glTexCoord2f( 1 + (x - 0.5)/aTexture_size, 1 + (y-0.5)/aTexture_size );
                glVertex2f( aTexture_size, 0 );
                glTexCoord2f( 1 + (x - 0.5)/aTexture_size, 0 + (y-0.5)/aTexture_size );
                glVertex2f( aTexture_size, aTexture_size );
                glEnd ();
            }
        }
        glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, aTexture_size, aTexture_size, 0 );
        aPasses--;
    }

    glFlush();
    glFinish();
    glDisable( GL_BLEND );

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    glPopAttrib();

    glEnable(GL_DEPTH_TEST);
}


void EDA_3D_CANVAS::Create_and_Render_Shadow_Buffer( GLuint *aDst_gl_texture,
        GLuint aTexture_size, bool aDraw_body, int aBlurPasses )
{

    float *depthbufferFloat = (float*) malloc( aTexture_size * aTexture_size * sizeof(float) );
    unsigned char *depthbufferRGBA = (unsigned char*) malloc( aTexture_size * aTexture_size * 4 );

    glDisable( GL_TEXTURE_2D );

    glViewport( 0, 0, aTexture_size, aTexture_size );

    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // move the board in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -GetPrm3DVisu().m_BoardPos.x * GetPrm3DVisu().m_BiuTo3Dunits,
                  -GetPrm3DVisu().m_BoardPos.y * GetPrm3DVisu().m_BiuTo3Dunits,
                  0.0F );

    if( aDraw_body )
    {
        if( m_glLists[GL_ID_BODY] )
        {
            glCallList( m_glLists[GL_ID_BODY] );
        }
    }

    // Call model list
    glCallList( m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] );

    if( m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] )
    {
        glCallList( m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] );
    }

    glPixelStorei( GL_PACK_ALIGNMENT, 4 );
    glReadBuffer( GL_BACK_LEFT );

    glFinish();

    glReadPixels( 0, 0,
                  aTexture_size, aTexture_size,
                  GL_DEPTH_COMPONENT, GL_FLOAT, depthbufferFloat );


    glFinish();

    for( unsigned int i = 0; i< (aTexture_size * aTexture_size); i++ )
    {
        unsigned char v = depthbufferFloat[i] * 255;
        depthbufferRGBA[i * 4 + 0] = v;
        depthbufferRGBA[i * 4 + 1] = v;
        depthbufferRGBA[i * 4 + 2] = v;
        depthbufferRGBA[i * 4 + 3] = 255;
    }

    glFinish();

    glEnable( GL_TEXTURE_2D );
    glGenTextures( 1, aDst_gl_texture );
    glBindTexture( GL_TEXTURE_2D, *aDst_gl_texture );
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );


    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, aTexture_size, aTexture_size, 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, depthbufferRGBA );

    glFlush();
    glFinish();

    CheckGLError( __FILE__, __LINE__ );

    blur_tex( *aDst_gl_texture, aBlurPasses, aTexture_size );

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, *aDst_gl_texture );

    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, depthbufferRGBA );

    for(unsigned int i = 0; i< (aTexture_size * aTexture_size); i++)
    {
        float v = (float)depthbufferRGBA[i * 4] / 255.0f;
        v = v * v;
        depthbufferRGBA[i * 4 + 0] = 0;
        depthbufferRGBA[i * 4 + 1] = 0;
        depthbufferRGBA[i * 4 + 2] = 0;
        depthbufferRGBA[i * 4 + 3] = 255 - (unsigned char)(v * 255);
    }

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, aTexture_size, aTexture_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, depthbufferRGBA );

    free( depthbufferRGBA );
    free( depthbufferFloat );
}

#define SHADOW_BOARD_SCALE 1.5f

void EDA_3D_CANVAS::GenerateFakeShadowsTextures()
{
    if( m_shadow_init == true )
    {
        return;
    }

    // Init info 3d parameters and create gl lists:
    CreateDrawGL_List();

    m_shadow_init = true;

    glClearColor( 0, 0, 0, 1 );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    const double ZDIST_MAX = Millimeter2iu( 3.5 ) * GetPrm3DVisu().m_BiuTo3Dunits;
    glOrtho( -GetPrm3DVisu().m_BoardSize.x * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             GetPrm3DVisu().m_BoardSize.x * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             -GetPrm3DVisu().m_BoardSize.y * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             GetPrm3DVisu().m_BoardSize.y * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             0.0, ZDIST_MAX );

    // Render FRONT shadow
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0, 0, 0.03 );
    glRotatef( 180, 0.0, 1.0, 0.0 );

    Create_and_Render_Shadow_Buffer( &m_text_fake_shadow_front, 512, false, 5 );


    // Render BACK shadow
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0, 0, 0.03 );
    ///glRotatef( 0.0, 0.0, 1.0, 0.0 );

    Create_and_Render_Shadow_Buffer( &m_text_fake_shadow_back, 512, false, 5 );

    // Render ALL BOARD shadow
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( -GetPrm3DVisu().m_BoardSize.x * SHADOW_BOARD_SCALE * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             GetPrm3DVisu().m_BoardSize.x * SHADOW_BOARD_SCALE * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             -GetPrm3DVisu().m_BoardSize.y * SHADOW_BOARD_SCALE * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             GetPrm3DVisu().m_BoardSize.y * SHADOW_BOARD_SCALE * GetPrm3DVisu().m_BiuTo3Dunits / 2.0f,
             0.0, 6.0f * ZDIST_MAX );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0, 0, -0.4f );
    glRotatef( 180.0, 0.0, 1.0, 0.0 );

    Create_and_Render_Shadow_Buffer( &m_text_fake_shadow_board, 512, true, 20 );
}


void EDA_3D_CANVAS::Redraw()
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
    wxSize size = GetClientSize();

    InitGL();

    if( isEnabled( FL_MODULE ) && isRealisticMode() &&
        isEnabled( FL_RENDER_SHADOWS ) )
    {
        GenerateFakeShadowsTextures();
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
         gluPerspective( 45.0 * GetPrm3DVisu().m_Zoom, ratio_HV, 1, 100 );
     }

    // position viewer
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glTranslatef( 0.0F, 0.0F, -( m_ZBottom + m_ZTop) / 2 );

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
        CreateDrawGL_List();

    if( isEnabled( FL_AXIS ) && m_glLists[GL_ID_AXIS] )
        glCallList( m_glLists[GL_ID_AXIS] );

    // move the board in order to draw it with its center at 0,0 3D coordinates
    glTranslatef( -GetPrm3DVisu().m_BoardPos.x * GetPrm3DVisu().m_BiuTo3Dunits,
                  -GetPrm3DVisu().m_BoardPos.y * GetPrm3DVisu().m_BiuTo3Dunits,
                  0.0F );

    // draw all objects in lists
    // transparent objects should be drawn after opaque objects

    if( isEnabled( FL_MODULE ) )
    {
        if( ! m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] )
            CreateDrawGL_List();
    }

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	if( isEnabled( FL_SHOW_BOARD_BODY ) )
	{
		if( !isEnabled( FL_RENDER_TEXTURES ) ||
             isEnabled( FL_SOLDERMASK ) || !isRealisticMode() )
		{
			glDisable( GL_TEXTURE_2D );
		}
		else
		{
			glEnable( GL_TEXTURE_2D );
		}

		glDisable( GL_LIGHTING );

		if( m_glLists[GL_ID_BODY] )
		{
			glCallList( m_glLists[GL_ID_BODY] );
		}

		glEnable( GL_LIGHTING );
	}

	glEnable( GL_COLOR_MATERIAL );
	SetOpenGlDefaultMaterial();
    glm::vec4 specular( GetPrm3DVisu().m_CopperColor.m_Red   * 0.3,
                        GetPrm3DVisu().m_CopperColor.m_Green * 0.3,
                        GetPrm3DVisu().m_CopperColor.m_Blue  * 0.3, 1.0 );
    GLint shininess_value = 8;

    glMateriali ( GL_FRONT_AND_BACK, GL_SHININESS, shininess_value );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.x );

    if( isRealisticMode() && isEnabled( FL_RENDER_TEXTURES ) )
    {
    	glEnable( GL_TEXTURE_2D );
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
	}

    if( m_glLists[GL_ID_BOARD] )
    {
        glCallList( m_glLists[GL_ID_BOARD] );
    }

	SetOpenGlDefaultMaterial();

    if( m_glLists[GL_ID_TECH_LAYERS] )
    {
        glCallList( m_glLists[GL_ID_TECH_LAYERS] );
    }

    if( isEnabled( FL_COMMENTS ) || isEnabled( FL_COMMENTS )  )
    {
        if( ! m_glLists[GL_ID_AUX_LAYERS] )
            CreateDrawGL_List();

        glCallList( m_glLists[GL_ID_AUX_LAYERS] );
    }

    // Draw Component Shadow
    if( isEnabled( FL_MODULE )  && isRealisticMode() &&
        isEnabled( FL_RENDER_SHADOWS ) )
    {
        glEnable( GL_CULL_FACE );
        glDisable( GL_DEPTH_TEST );

        glEnable( GL_COLOR_MATERIAL ) ;
        SetOpenGlDefaultMaterial();
        glColor4f( 1.0, 1.0, 1.0, 1.0 );

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
        glColor4f( 1.0, 1.0, 1.0, 1.0 );

        glEnable( GL_DEPTH_TEST );
        glDisable( GL_TEXTURE_2D );
        glDisable( GL_CULL_FACE );
    }

    glEnable(GL_COLOR_MATERIAL);
    SetOpenGlDefaultMaterial();


    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glColor4f( 1.0, 1.0, 1.0, 1.0 );

    // Draw Solid Shapes
    if( isEnabled( FL_MODULE ) )
    {
        if( ! m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] )
            CreateDrawGL_List();

        glCallList( m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] );
    }

    // Grid uses transparency: draw it after all objects
    if( isEnabled( FL_GRID ) && m_glLists[GL_ID_GRID] )
        glCallList( m_glLists[GL_ID_GRID] );

    // This list must be drawn last, because it contains the
    // transparent gl objects, which should be drawn after all
    // non transparent objects
    if(  isEnabled( FL_MODULE ) && m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] )
        glCallList( m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] );

    // Draw Board Shadow
    if( isEnabled( FL_MODULE ) && isRealisticMode() &&
        isEnabled( FL_RENDER_SHADOWS ) )
    {
        if( m_glLists[GL_ID_SHADOW_BOARD] )
        {
            glEnable( GL_CULL_FACE );
            glDisable( GL_COLOR_MATERIAL );
            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, m_text_fake_shadow_board );
            glCallList( m_glLists[GL_ID_SHADOW_BOARD] );
            glDisable( GL_CULL_FACE );
        }
    }

    SwapBuffers();
}


void EDA_3D_CANVAS::BuildShadowList( GLuint aFrontList, GLuint aBacklist, GLuint aBoardList )
{
    // Use similar calculation as Grid limits, in 3D units
    wxSize  brd_size = getBoardSize();
    wxPoint brd_center_pos = getBoardCenter();

    float xsize   = brd_size.x;
    float ysize   = brd_size.y;

    float scale   = GetPrm3DVisu().m_BiuTo3Dunits;
    float xmin    = (brd_center_pos.x - xsize / 2.0) * scale;
    float xmax    = (brd_center_pos.x + xsize / 2.0) * scale;
    float ymin    = (brd_center_pos.y - ysize / 2.0) * scale;
    float ymax    = (brd_center_pos.y + ysize / 2.0) * scale;

    float zpos = GetPrm3DVisu().GetLayerZcoordBIU( F_Paste );
    zpos *= GetPrm3DVisu().m_BiuTo3Dunits;

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
    zpos = GetPrm3DVisu().GetLayerZcoordBIU( B_Paste );
    zpos *= GetPrm3DVisu().m_BiuTo3Dunits;

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
    xsize   = brd_size.x * SHADOW_BOARD_SCALE;
    ysize   = brd_size.y * SHADOW_BOARD_SCALE;

    scale = GetPrm3DVisu().m_BiuTo3Dunits;
    xmin    = (brd_center_pos.x - xsize / 2.0) * scale;
    xmax    = (brd_center_pos.x + xsize / 2.0) * scale;
    ymin    = (brd_center_pos.y - ysize / 2.0) * scale;
    ymax    = (brd_center_pos.y + ysize / 2.0) * scale;


    glNewList( aBoardList, GL_COMPILE );
    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( F_Paste ) );

    glBegin (GL_QUADS);
    glTexCoord2f( 1.0, 0.0 ); glVertex3f( xmin, ymin, zpos * 30.0);
    glTexCoord2f( 0.0, 0.0 ); glVertex3f( xmax, ymin, zpos * 30.0);
    glTexCoord2f( 0.0, 1.0 ); glVertex3f( xmax, ymax, zpos * 30.0);
    glTexCoord2f( 1.0, 1.0 ); glVertex3f( xmin, ymax, zpos * 30.0);
    glEnd();

    glEndList();
}


void EDA_3D_CANVAS::BuildBoard3DView(GLuint aBoardList, GLuint aBodyOnlyList)
{
    BOARD* pcb = GetBoard();

    // If FL_RENDER_SHOW_HOLES_IN_ZONES is true, holes are correctly removed from copper zones areas.
    // If FL_RENDER_SHOW_HOLES_IN_ZONES is false, holes are not removed from copper zones areas,
    // but the calculation time is twice shorter.
    bool remove_Holes = isEnabled( FL_RENDER_SHOW_HOLES_IN_ZONES );

    bool realistic_mode = isRealisticMode();
    bool useTextures = isRealisticMode() && isEnabled( FL_RENDER_TEXTURES );

    // Number of segments to convert a circle to polygon
    // Boost polygon (at least v 1.54, v1.55 and previous) in very rare cases crashes
    // when using 16 segments to approximate a circle.
    // So using 18 segments is a workaround to try to avoid these crashes
    // ( We already used this trick in plot_board_layers.cpp,
    // see PlotSolderMaskLayer() )
    const int       segcountforcircle   = 18;
    double          correctionFactor    = 1.0 / cos( M_PI / (segcountforcircle * 2) );
    const int       segcountLowQuality  = 12;   // segments to draw a circle with low quality
                                                // to reduce time calculations
                                                // for holes and items which do not need
                                                // a fine representation
    double          correctionFactorLQ = 1.0 / cos( M_PI / (segcountLowQuality * 2) );

    CPOLYGONS_LIST  bufferPolys;
    bufferPolys.reserve( 200000 );              // Reserve for large board (tracks mainly)

    CPOLYGONS_LIST  bufferPcbOutlines;          // stores the board main outlines
    CPOLYGONS_LIST  allLayerHoles;              // Contains through holes, calculated only once
    allLayerHoles.reserve( 20000 );

    // Build a polygon from edge cut items
    wxString msg;

    if( !pcb->GetBoardPolygonOutlines( bufferPcbOutlines, allLayerHoles, &msg ) )
    {
        msg << wxT("\n\n") <<
            _("Unable to calculate the board outlines.\n"
              "Therefore use the board boundary box.");
        wxMessageBox( msg );
    }

    CPOLYGONS_LIST  bufferZonesPolys;
    bufferZonesPolys.reserve( 500000 );             // Reserve for large board ( copper zones mainly )

    CPOLYGONS_LIST  currLayerHoles;                 // Contains holes for the current layer
    bool            throughHolesListBuilt = false;  // flag to build the through hole polygon list only once

    LSET            cu_set = LSET::AllCuMask( GetPrm3DVisu().m_CopperLayersCount );

#if 1
    LAYER_ID        cu_seq[MAX_CU_LAYERS];          // preferred sequence, could have called CuStack()
                                                    // but I assume that's backwards

    glNewList( aBoardList, GL_COMPILE );

    for( unsigned i=0; i<DIM(cu_seq); ++i )
        cu_seq[i] = ToLAYER_ID( B_Cu - i );

    for( LSEQ cu = cu_set.Seq( cu_seq, DIM(cu_seq) );  cu;  ++cu )
#else
    for( LSEQ cu = cu_set.CuStack();  cu;  ++cu )
#endif
    {
        LAYER_ID layer = *cu;

        // Skip non enabled layers in normal mode,
        // and internal layers in realistic mode
        if( !is3DLayerEnabled( layer ) )
            continue;

        bufferPolys.RemoveAllContours();
        bufferZonesPolys.RemoveAllContours();
        currLayerHoles.RemoveAllContours();

        // Draw tracks:
        for( TRACK* track = pcb->m_Track;  track;  track = track->Next() )
        {
            if( !track->IsOnLayer( layer ) )
                continue;

            track->TransformShapeWithClearanceToPolygon( bufferPolys,
                                                         0, segcountforcircle,
                                                         correctionFactor );

            // Add via hole
            if( track->Type() == PCB_VIA_T )
            {
                VIA *via = static_cast<VIA*>( track );
                VIATYPE_T viatype = via->GetViaType();
                int holediameter = via->GetDrillValue();
                int thickness = GetPrm3DVisu().GetCopperThicknessBIU();
                int hole_outer_radius = (holediameter + thickness) / 2;

                if( viatype != VIA_THROUGH )
                    TransformCircleToPolygon( currLayerHoles,
                                              via->GetStart(), hole_outer_radius,
                                              segcountLowQuality );
                else if( !throughHolesListBuilt )
                    TransformCircleToPolygon( allLayerHoles,
                                              via->GetStart(), hole_outer_radius,
                                              segcountLowQuality );
            }
        }

        // draw pads
        for( MODULE* module = pcb->m_Modules;  module;  module = module->Next() )
        {
            module->TransformPadsShapesWithClearanceToPolygon( layer,
                                                               bufferPolys,
                                                               0,
                                                               segcountforcircle,
                                                               correctionFactor );

            // Micro-wave modules may have items on copper layers
            module->TransformGraphicShapesWithClearanceToPolygonSet( layer,
                                                                     bufferPolys,
                                                                     0,
                                                                     segcountforcircle,
                                                                     correctionFactor );

            // Add pad hole, if any
            if( !throughHolesListBuilt )
            {
                D_PAD* pad = module->Pads();

                for( ; pad; pad = pad->Next() )
                    pad->BuildPadDrillShapePolygon( allLayerHoles, 0,
                                                    segcountLowQuality );
            }
        }

        // Draw copper zones
        if( isEnabled( FL_ZONE ) )
        {
            for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
            {
                ZONE_CONTAINER* zone = pcb->GetArea( ii );
                LAYER_NUM       zonelayer = zone->GetLayer();

                if( zonelayer == layer )
                {
                    zone->TransformSolidAreasShapesToPolygonSet(
                        remove_Holes ? bufferPolys : bufferZonesPolys,
                        segcountLowQuality, correctionFactorLQ );
                }
            }
        }

        // draw graphic items on copper layers (texts)
        for( BOARD_ITEM* item = pcb->m_Drawings; item; item = item->Next() )
        {
            if( !item->IsOnLayer( layer ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:    // should not exist on copper layers
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

        // bufferPolys contains polygons to merge. Many overlaps .
        // Calculate merged polygons
        if( bufferPolys.GetCornersCount() == 0 )
            continue;

        KI_POLYGON_SET  currLayerPolyset;
        KI_POLYGON_SET  polysetHoles;

        // Add polygons, without holes
        bufferPolys.ExportTo( currLayerPolyset );

        // Add holes in polygon list
        currLayerHoles.Append( allLayerHoles );

        if( currLayerHoles.GetCornersCount() > 0 )
            currLayerHoles.ExportTo( polysetHoles );

        // Merge polygons, remove holes
        currLayerPolyset -= polysetHoles;

        int thickness = GetPrm3DVisu().GetLayerObjectThicknessBIU( layer );
        int zpos = GetPrm3DVisu().GetLayerZcoordBIU( layer );

        if( realistic_mode )
        {
            setGLCopperColor();
        }
        else
        {
            EDA_COLOR_T color = g_ColorsSettings.GetLayerColor( layer );
            SetGLColor( color );
        }

        glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );

        bufferPolys.RemoveAllContours();
        bufferPolys.ImportFrom( currLayerPolyset );
        Draw3D_SolidHorizontalPolyPolygons( bufferPolys, zpos,
                                            thickness,
                                            GetPrm3DVisu().m_BiuTo3Dunits, useTextures );

        if( isEnabled( FL_USE_COPPER_THICKNESS ) == true )
        {
        	thickness -= ( 0.04 * IU_PER_MM );
        }

        glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );

        if( bufferZonesPolys.GetCornersCount() )
            Draw3D_SolidHorizontalPolyPolygons( bufferZonesPolys, zpos,
                                                thickness,
                                                GetPrm3DVisu().m_BiuTo3Dunits, useTextures );
        throughHolesListBuilt = true;
    }

    if ( !isEnabled( FL_SHOW_BOARD_BODY ) )
    {
        setGLCopperColor();

        // Draw vias holes (vertical cylinders)
        for( const TRACK* track = pcb->m_Track;  track;  track = track->Next() )
        {
            const VIA *via = dynamic_cast<const VIA*>(track);

            if( via )
                Draw3DViaHole( via );
        }

        // Draw pads holes (vertical cylinders)
        for( const MODULE* module = pcb->m_Modules;  module;  module = module->Next() )
        {
            for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
                Draw3DPadHole( pad );
        }
    }

	glEndList();

    // Build the body board:
	glNewList( aBodyOnlyList, GL_COMPILE );

    if( isRealisticMode() )
    {
    	setGLEpoxyColor( 0.95 );
    }
    else
    {
        EDA_COLOR_T color = g_ColorsSettings.GetLayerColor( Edge_Cuts );
        SetGLColor( color, 0.7 );
    }

    float copper_thickness = GetPrm3DVisu().GetCopperThicknessBIU();

    // a small offset between substrate and external copper layer to avoid artifacts
    // when drawing copper items on board
    float epsilon = Millimeter2iu( 0.01 );
    float zpos = GetPrm3DVisu().GetLayerZcoordBIU( B_Cu );
    float board_thickness = GetPrm3DVisu().GetLayerZcoordBIU( F_Cu )
                        - GetPrm3DVisu().GetLayerZcoordBIU( B_Cu );

    // items on copper layers and having a thickness = copper_thickness
    // are drawn from zpos - copper_thickness/2 to zpos + copper_thickness
    // therefore substrate position is copper_thickness/2 to
    // substrate_height - copper_thickness/2
    zpos += (copper_thickness + epsilon) / 2.0;
    board_thickness -= copper_thickness + epsilon;

    glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( F_Cu ) );
    KI_POLYGON_SET  currLayerPolyset;
    KI_POLYGON_SET  polysetHoles;

    // Add polygons, without holes
    bufferPcbOutlines.ExportTo( currLayerPolyset );

    // Build holes list
    allLayerHoles.ExportTo( polysetHoles );

    // remove holes
    currLayerPolyset -= polysetHoles;

    bufferPcbOutlines.RemoveAllContours();
    bufferPcbOutlines.ImportFrom( currLayerPolyset );

    if( bufferPcbOutlines.GetCornersCount() )
    {
        Draw3D_SolidHorizontalPolyPolygons( bufferPcbOutlines, zpos + board_thickness/2.0,
                                            board_thickness, GetPrm3DVisu().m_BiuTo3Dunits, useTextures );
    }

    glEndList();
}


void EDA_3D_CANVAS::BuildTechLayers3DView()
{
    BOARD* pcb = GetBoard();
    bool useTextures = isRealisticMode() && isEnabled( FL_RENDER_TEXTURES );

    // Number of segments to draw a circle using segments
    const int       segcountforcircle   = 18;
    double          correctionFactor    = 1.0 / cos( M_PI / (segcountforcircle * 2) );
    const int       segcountLowQuality  = 12;   // segments to draw a circle with low quality
                                                // to reduce time calculations
                                                // for holes and items which do not need
                                                // a fine representation

    double          correctionFactorLQ = 1.0 / cos( M_PI / (segcountLowQuality * 2) );

    CPOLYGONS_LIST  bufferPolys;
    bufferPolys.reserve( 100000 );              // Reserve for large board
    CPOLYGONS_LIST  allLayerHoles;              // Contains through holes, calculated only once
    allLayerHoles.reserve( 20000 );

    CPOLYGONS_LIST  bufferPcbOutlines;          // stores the board main outlines
    // Build a polygon from edge cut items
    wxString msg;

    if( !pcb->GetBoardPolygonOutlines( bufferPcbOutlines, allLayerHoles, &msg ) )
    {
        msg << wxT("\n\n") <<
            _("Unable to calculate the board outlines.\n"
              "Therefore use the board boundary box.");
        wxMessageBox( msg );
    }

    int thickness = GetPrm3DVisu().GetCopperThicknessBIU();

    // Add via holes
    for( VIA* via = GetFirstVia( pcb->m_Track ); via;
            via = GetFirstVia( via->Next() ) )
    {
        VIATYPE_T viatype = via->GetViaType();
        int holediameter = via->GetDrillValue();
        int hole_outer_radius = (holediameter + thickness) / 2;

        if( viatype == VIA_THROUGH )
            TransformCircleToPolygon( allLayerHoles,
                    via->GetStart(), hole_outer_radius,
                    segcountLowQuality );
    }

    // draw pads holes
    for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
    {
        // Add pad hole, if any
        D_PAD* pad = module->Pads();

        for( ; pad; pad = pad->Next() )
            pad->BuildPadDrillShapePolygon( allLayerHoles, 0,
                                                segcountLowQuality );
    }

    // draw graphic items, on technical layers

    KI_POLYGON_SET  brdpolysetHoles;
    allLayerHoles.ExportTo( brdpolysetHoles );

    static const LAYER_ID teckLayerList[] = {
        B_Adhes,
        F_Adhes,
        B_Paste,
        F_Paste,
        B_SilkS,
        F_SilkS,
        B_Mask,
        F_Mask,
    };

    // User layers are not drawn here, only technical layers
    for( LSEQ seq = LSET::AllTechMask().Seq( teckLayerList, DIM( teckLayerList ) );  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        if( !is3DLayerEnabled( layer ) )
            continue;

        if( layer == Edge_Cuts && isEnabled( FL_SHOW_BOARD_BODY )  )
            continue;

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
            if( layer == F_SilkS || layer == B_SilkS )
            {
                D_PAD*  pad = module->Pads();
                int     linewidth = g_DrawDefaultLineThickness;

                for( ; pad; pad = pad->Next() )
                {
                    if( !pad->IsOnLayer( layer ) )
                        continue;

                    BuildPadShapeThickOutlineAsPolygon( pad, bufferPolys,
                            linewidth, segcountforcircle, correctionFactor );
                }
            }
            else
                module->TransformPadsShapesWithClearanceToPolygon( layer,
                        bufferPolys, 0, segcountforcircle, correctionFactor );

            module->TransformGraphicShapesWithClearanceToPolygonSet( layer,
                    bufferPolys, 0, segcountforcircle, correctionFactor );
        }

        // Draw non copper zones
        if( isEnabled( FL_ZONE ) )
        {
            for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
            {
                ZONE_CONTAINER* zone = pcb->GetArea( ii );

                if( !zone->IsOnLayer( layer ) )
                    continue;

                zone->TransformSolidAreasShapesToPolygonSet(
                        bufferPolys, segcountLowQuality, correctionFactorLQ );
            }
        }

        // bufferPolys contains polygons to merge. Many overlaps .
        // Calculate merged polygons and remove pads and vias holes
        if( bufferPolys.GetCornersCount() == 0 )
            continue;
        KI_POLYGON_SET  currLayerPolyset;
        KI_POLYGON_SET  polyset;

        // Solder mask layers are "negative" layers.
        // Shapes should be removed from the full board area.
        if( layer == B_Mask || layer == F_Mask )
        {
            bufferPcbOutlines.ExportTo( currLayerPolyset );
            bufferPolys.Append( allLayerHoles );
            bufferPolys.ExportTo( polyset );
            currLayerPolyset -= polyset;
        }
        // Remove holes from Solder paste layers and siklscreen
        else if( layer == B_Paste || layer == F_Paste
                 || layer == B_SilkS || layer == F_SilkS  )
        {
            bufferPolys.ExportTo( currLayerPolyset );
            currLayerPolyset -= brdpolysetHoles;
        }
        else    // usuall layers, merge polys built from each item shape:
        {
            bufferPolys.ExportTo( polyset );
            currLayerPolyset += polyset;
        }

        int         thickness = GetPrm3DVisu().GetLayerObjectThicknessBIU( layer );
        int         zpos = GetPrm3DVisu().GetLayerZcoordBIU( layer );

        if( layer == Edge_Cuts )
        {
            thickness = GetPrm3DVisu().GetLayerZcoordBIU( F_Cu )
                        - GetPrm3DVisu().GetLayerZcoordBIU( B_Cu );
            zpos = GetPrm3DVisu().GetLayerZcoordBIU( B_Cu )
                   + (thickness / 2);
        }
        else
        {
            // for Draw3D_SolidHorizontalPolyPolygons, zpos it the middle between bottom and top
            // sides.
            // However for top layers, zpos should be the bottom layer pos,
            // and for bottom layers, zpos should be the top layer pos.
            if( Get3DLayer_Z_Orientation( layer ) > 0 )
                zpos += thickness/2;
            else
                zpos -= thickness/2 ;
        }

        bufferPolys.RemoveAllContours();
        bufferPolys.ImportFrom( currLayerPolyset );

        setGLTechLayersColor( layer );
        glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );
        Draw3D_SolidHorizontalPolyPolygons( bufferPolys, zpos,
                thickness, GetPrm3DVisu().m_BiuTo3Dunits, useTextures );
    }
}


/**
 * Function BuildBoard3DAuxLayers
 * Called by CreateDrawGL_List()
 * Fills the OpenGL GL_ID_BOARD draw list with items
 * on aux layers only
 */
void EDA_3D_CANVAS::BuildBoard3DAuxLayers()
{
    const int   segcountforcircle   = 18;
    double      correctionFactor    = 1.0 / cos( M_PI / (segcountforcircle * 2) );
    BOARD*      pcb = GetBoard();

    CPOLYGONS_LIST  bufferPolys;

    bufferPolys.reserve( 5000 );    // Reserve for items not on board

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
        if( bufferPolys.GetCornersCount() == 0 )
            continue;
        KI_POLYGON_SET  currLayerPolyset;
        KI_POLYGON_SET  polyset;
        bufferPolys.ExportTo( polyset );
        currLayerPolyset += polyset;

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

        bufferPolys.RemoveAllContours();
        bufferPolys.ImportFrom( currLayerPolyset );

        setGLTechLayersColor( layer );
        glNormal3f( 0.0, 0.0, Get3DLayer_Z_Orientation( layer ) );
        Draw3D_SolidHorizontalPolyPolygons( bufferPolys, zpos,
                                            thickness, GetPrm3DVisu().m_BiuTo3Dunits, false );
    }
}

void EDA_3D_CANVAS::CreateDrawGL_List()
{
    BOARD* pcb = GetBoard();

    wxBusyCursor    dummy;

    // Build 3D board parameters:
    GetPrm3DVisu().InitSettings( pcb );

    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    // Create axis gl list (if it is not shown, the list will be not called
    Draw3DAxis();

    // Create grid gl list
    if( ! m_glLists[GL_ID_GRID] )
    {
        m_glLists[GL_ID_GRID] = glGenLists( 1 );
        glNewList( m_glLists[GL_ID_GRID], GL_COMPILE );

        Draw3DGrid( GetPrm3DVisu().m_3D_Grid );
        glEndList();
    }

    // Create Board full gl lists:

// For testing purpose only, display calculation time to generate 3D data
// #define PRINT_CALCULATION_TIME

#ifdef PRINT_CALCULATION_TIME
    unsigned strtime = GetRunningMicroSecs();
#endif

    if( ! m_glLists[GL_ID_BOARD] )
    {
        m_glLists[GL_ID_BOARD] = glGenLists( 1 );
        m_glLists[GL_ID_BODY] = glGenLists( 1 );
        BuildBoard3DView(m_glLists[GL_ID_BOARD], m_glLists[GL_ID_BODY]);
        CheckGLError( __FILE__, __LINE__ );
    }

    if( ! m_glLists[GL_ID_TECH_LAYERS] )
    {
        m_glLists[GL_ID_TECH_LAYERS] = glGenLists( 1 );
        glNewList( m_glLists[GL_ID_TECH_LAYERS], GL_COMPILE );
        BuildTechLayers3DView();
        glEndList();
        CheckGLError( __FILE__, __LINE__ );
    }

    if( ! m_glLists[GL_ID_AUX_LAYERS] )
    {
        m_glLists[GL_ID_AUX_LAYERS] = glGenLists( 1 );
        glNewList( m_glLists[GL_ID_AUX_LAYERS], GL_COMPILE );
        BuildBoard3DAuxLayers();
        glEndList();
        CheckGLError( __FILE__, __LINE__ );
    }

    // draw modules 3D shapes
    if( ! m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] && isEnabled( FL_MODULE ) )
    {
        m_glLists[GL_ID_3DSHAPES_SOLID_FRONT] = glGenLists( 1 );

        // GL_ID_3DSHAPES_TRANSP_FRONT is an auxiliary list for 3D shapes;
        // Ensure it is cleared before rebuilding it
        if( m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] )
            glDeleteLists( m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT], 1 );

        m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT] = glGenLists( 1 );
        BuildFootprintShape3DList( m_glLists[GL_ID_3DSHAPES_SOLID_FRONT],
                                   m_glLists[GL_ID_3DSHAPES_TRANSP_FRONT], false );

        CheckGLError( __FILE__, __LINE__ );

        m_glLists[GL_ID_SHADOW_FRONT] = glGenLists( 1 );
        m_glLists[GL_ID_SHADOW_BACK]  = glGenLists( 1 );
        m_glLists[GL_ID_SHADOW_BOARD] = glGenLists( 1 );
        BuildShadowList(m_glLists[GL_ID_SHADOW_FRONT], m_glLists[GL_ID_SHADOW_BACK], m_glLists[GL_ID_SHADOW_BOARD]);

        CheckGLError( __FILE__, __LINE__ );
    }


#ifdef PRINT_CALCULATION_TIME
    unsigned    endtime = GetRunningMicroSecs();
    wxString    msg;
    msg.Printf( "Built data %.1f ms", (double) (endtime - strtime) / 1000 );
    Parent()->SetStatusText( msg, 0 );
#endif
}


void EDA_3D_CANVAS::BuildFootprintShape3DList( GLuint aOpaqueList,
                                               GLuint aTransparentList, bool aSideToLoad)
{
        // aOpaqueList is the gl list for non transparent items
        // aTransparentList is the gl list for non transparent items,
        // which need to be drawn after all other items

        BOARD* pcb = GetBoard();
        glNewList( aOpaqueList, GL_COMPILE );
        bool loadTransparentObjects = false;

        for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
            module->ReadAndInsert3DComponentShape( this, !loadTransparentObjects,
                                                   loadTransparentObjects, aSideToLoad );

        glEndList();

        glNewList( aTransparentList, GL_COMPILE );
        loadTransparentObjects = true;

        for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
            module->ReadAndInsert3DComponentShape( this, !loadTransparentObjects,
                                                   loadTransparentObjects, aSideToLoad );

        glEndList();
}


void MODULE::ReadAndInsert3DComponentShape( EDA_3D_CANVAS* glcanvas,
                                    bool aAllowNonTransparentObjects,
                                    bool aAllowTransparentObjects,
                                    bool aSideToLoad )
{

    // Read from disk and draws the footprint 3D shapes if exists
    double zpos = glcanvas->GetPrm3DVisu().GetModulesZcoord3DIU( IsFlipped() );

    glPushMatrix();

    glTranslatef( m_Pos.x * glcanvas->GetPrm3DVisu().m_BiuTo3Dunits,
                  -m_Pos.y * glcanvas->GetPrm3DVisu().m_BiuTo3Dunits,
                  zpos );

    if( m_Orient )
        glRotatef( (double) m_Orient / 10, 0.0, 0.0, 1.0 );

    if( IsFlipped() )
    {
        glRotatef( 180.0, 0.0, 1.0, 0.0 );
        glRotatef( 180.0, 0.0, 0.0, 1.0 );
    }

    S3D_MASTER* shape3D = Models();
    for( ; shape3D; shape3D = shape3D->Next() )
    {
        shape3D->SetLoadNonTransparentObjects( aAllowNonTransparentObjects );
        shape3D->SetLoadTransparentObjects( aAllowTransparentObjects );

        if( shape3D->Is3DType( S3D_MASTER::FILE3D_VRML ) )
        {
            glPushMatrix();
            shape3D->ReadData();
            glPopMatrix();
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


GLfloat Get3DLayer_Z_Orientation( LAYER_NUM aLayer )
{
    double nZ = 1.0;

    if( ( aLayer == B_Cu )
        || ( aLayer == B_Adhes )
        || ( aLayer == B_Paste )
        || ( aLayer == B_SilkS )
        || ( aLayer == B_Mask ) )
        nZ = -1.0;

    return nZ;
}
