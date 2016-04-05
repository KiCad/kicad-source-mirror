/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
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
 * @file  c3d_render_ogl_legacy.cpp
 * @brief
 */


#include "c3d_render_ogl_legacy.h"
#include "ogl_legacy_utils.h"
#include "common_ogl/ogl_utils.h"
#include "../cimage.h"

C3D_RENDER_OGL_LEGACY::C3D_RENDER_OGL_LEGACY( CINFO3D_VISU &aSettings,
                                              S3D_CACHE *a3DModelManager ) :
                       C3D_RENDER_BASE( aSettings, a3DModelManager )
{
    wxLogTrace( m_logTrace, wxT( "C3D_RENDER_OGL_LEGACY::C3D_RENDER_OGL_LEGACY" ) );

    m_ogl_disp_lists_layers.clear();
    m_triangles.clear();
    m_ogl_disp_list_board = 0;
}


C3D_RENDER_OGL_LEGACY::~C3D_RENDER_OGL_LEGACY()
{
    wxLogTrace( m_logTrace, wxT( "C3D_RENDER_OGL_LEGACY::~C3D_RENDER_OGL_LEGACY" ) );

}


void C3D_RENDER_OGL_LEGACY::SetCurWindowSize( const wxSize &aSize )
{
    if( m_windowSize != aSize )
    {
        m_windowSize = aSize;
        glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

        // Initialize here any screen dependent data
    }
}

/* defines */
#define SPHERE 1
#define CONE   2
#define CUBE   3


/* csgOperation
 *  types of CSG operations
 */
typedef enum {
    CSG_A,
    CSG_B,
    CSG_A_OR_B,
    CSG_A_AND_B,
    CSG_A_SUB_B,
    CSG_B_SUB_A
} csgOperation;

GLfloat cone_x = 0.0;
GLfloat cone_y = 0.0;
GLfloat cone_z = 0.0;

GLfloat cube_x = 0.0;
GLfloat cube_y = 0.0;
GLfloat cube_z = 0.0;

GLfloat sphere_x = 0.0;
GLfloat sphere_y = 0.0;
GLfloat sphere_z = 0.0;

GLint mouse_state = -1;
GLint mouse_button = -1;

csgOperation Op = CSG_A_OR_B;

void (*A)(void);
void (*B)(void);


/* functions */

/* one()
 *  draw a single object
 */
void op_one(void(*a)(void))
{
  glEnable(GL_DEPTH_TEST);
  a();
  glDisable(GL_DEPTH_TEST);
}

/* or()
 *  boolean A or B (draw wherever A or B)
 *  algorithm: simple, just draw both with depth test enabled
 */
void op_or(void(*a)(void), void(*b)())
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);  /* TODO - should just push depth */
    glEnable(GL_DEPTH_TEST);
    //glDisable(GL_CULL_FACE);
    a(); b();
    glPopAttrib();
}

/* inside()
 *  sets stencil buffer to show the part of A
 *  (front or back face according to 'face')
 *  that is inside of B.
 */
void op_inside(void(*a)(void), void(*b)(void), GLenum face, GLenum test)
{
    /* draw A into depth buffer, but not into color buffer */
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glCullFace(face);
    a();

    /* use stencil buffer to find the parts of A that are inside of B
     * by first incrementing the stencil buffer wherever B's front faces
     * are...
     */

    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    //glStencilFunc(GL_LESS, 1, 0);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT ); //GL_INCR);
    glCullFace(GL_BACK);
    b();

    /* ...then decrement the stencil buffer wherever B's back faces are */
    //glStencilFunc(GL_EQUAL, 0, 0);
  //glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO );
    glCullFace(GL_FRONT);
    b();


    /* now draw the part of A that is inside of B */
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(test, 0, 1);
    glDisable(GL_DEPTH_TEST);
    glCullFace(face);
    a();

    /* reset stencil test */
    glDisable(GL_STENCIL_TEST);
}

/* fixup()
 *  fixes up the depth buffer with A's depth values
 */
void fixup(void(*a)(void))
{
    /* fix up the depth buffer */
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDepthFunc(GL_ALWAYS);
    a();

    /* reset depth func */
    glDepthFunc(GL_LESS);
}

/* and()
 *  boolean A and B (draw wherever A intersects B)
 *  algorithm: find where A is inside B, then find where
 *             B is inside A
 */
void op_and(void(*a)(void), void(*b)(void))
{
    op_inside(a, b, GL_BACK, GL_NOTEQUAL);
#if 1  /* set to 0 for faster, but incorrect results */
    fixup(b);
#endif
    op_inside(b, a, GL_BACK, GL_NOTEQUAL);
}

/*
 * sub()
 *  boolean A subtract B (draw wherever A is and B is NOT)
 *  algorithm: find where a is inside B, then find where
 *             the BACK faces of B are NOT in A
 */
void op_sub(void(*a)(void), void(*b)(void))
{
    op_inside(a, b, GL_FRONT, GL_NOTEQUAL);

#if 1  /* set to 0 for faster, but incorrect results */
    fixup(b);
#endif
    op_inside(b, a, GL_BACK, GL_EQUAL);
    //op_inside(a, b, GL_FRONT, GL_NOTEQUAL);

}

/* sphere()
 *  draw a yellow sphere
 */
void sphere(void)
{/*
    glLoadName(2);
    glPushMatrix();
    glTranslatef(sphere_x, sphere_y, sphere_z);
    glColor3f(1.0, 1.0, 0.0);
    glutSolidSphere(5.0, 16, 16);
    glPopMatrix();*/
    glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
       // Top face (y = 1.0f)
       // Define vertices in counter-clockwise (CCW) order with normal pointing out
       glColor3f(0.0f, 1.0f, 0.0f);     // Green
       glVertex3f( 1.0f, 1.0f, -1.0f);
       glVertex3f(-1.0f, 1.0f, -1.0f);
       glVertex3f(-1.0f, 1.0f,  1.0f);
       glVertex3f( 1.0f, 1.0f,  1.0f);

       // Bottom face (y = -1.0f)
       glColor3f(1.0f, 0.5f, 0.0f);     // Orange
       glVertex3f( 1.0f, -1.0f,  1.0f);
       glVertex3f(-1.0f, -1.0f,  1.0f);
       glVertex3f(-1.0f, -1.0f, -1.0f);
       glVertex3f( 1.0f, -1.0f, -1.0f);

       // Front face  (z = 1.0f)
       glColor3f(1.0f, 0.0f, 0.0f);     // Red
       glVertex3f( 1.0f,  1.0f, 1.0f);
       glVertex3f(-1.0f,  1.0f, 1.0f);
       glVertex3f(-1.0f, -1.0f, 1.0f);
       glVertex3f( 1.0f, -1.0f, 1.0f);

       // Back face (z = -1.0f)
       glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
       glVertex3f( 1.0f, -1.0f, -1.0f);
       glVertex3f(-1.0f, -1.0f, -1.0f);
       glVertex3f(-1.0f,  1.0f, -1.0f);
       glVertex3f( 1.0f,  1.0f, -1.0f);

       // Left face (x = -1.0f)
       glColor3f(0.0f, 0.0f, 1.0f);     // Blue
       glVertex3f(-1.0f,  1.0f,  1.0f);
       glVertex3f(-1.0f,  1.0f, -1.0f);
       glVertex3f(-1.0f, -1.0f, -1.0f);
       glVertex3f(-1.0f, -1.0f,  1.0f);

       // Right face (x = 1.0f)
       glColor3f(1.0f, 0.0f, 1.0f);     // Magenta
       glVertex3f(1.0f,  1.0f, -1.0f);
       glVertex3f(1.0f,  1.0f,  1.0f);
       glVertex3f(1.0f, -1.0f,  1.0f);
       glVertex3f(1.0f, -1.0f, -1.0f);
    glEnd();  // End of drawing color-cube
}

/* cube()
 *  draw a red cube
 */
void cube(void)
{
    glPushMatrix();
    glColor3f(0.3, 0.8, 0.1);
    OGL_draw_arrow( SFVEC3F( 1.0f, 0.0f, 0.0f ),
                    SFVEC3F( 0.0f, 0.0f, 0.0f ),
                    0.2f );
    glPopMatrix();
}

/* cone()
 *  draw a green cone
 */
void cone(void)
{
   /* glLoadName(3);
    glPushMatrix();
    glTranslatef(cone_x, cone_y, cone_z);
    glColor3f(0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, -6.5);
    glutSolidCone(4.0, 15.0, 16, 16);
    glRotatef(180.0, 1.0, 0.0, 0.0);
    glutSolidCone(4.0, 0.0, 16, 1);
    glPopMatrix();*/
}

void init(void)
{
    //tbInit(GLUT_MIDDLE_BUTTON);

    //glSelectBuffer(SELECT_BUFFER, select_buffer);

    // glEnable( GL_DITHER );

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    //glEnable(GL_LIGHT0);
    //glEnable(GL_LIGHT1);
    //glEnable(GL_LIGHT2);
    //glEnable(GL_LIGHTING);

    // Setup light
    // https://www.opengl.org/sdk/docs/man2/xhtml/glLight.xml
    // /////////////////////////////////////////////////////////////////////////
    {
    const GLfloat ambient[]  = { 0.1f, 0.0f, 0.1f, 1.0f };
    const GLfloat diffuse[]  = { 0.3f, 0.3f, 0.3f, 1.0f };
    const GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //const GLfloat lmodel_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };

    glLightfv( GL_LIGHT0, GL_AMBIENT,  ambient );
    glLightfv( GL_LIGHT0, GL_DIFFUSE,  diffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, specular );

    const GLfloat position1[]= { 0.0f, 0.0f, 1.0f, 0.0f };                      // defines a directional light that points along the negative z-axis
    glLightfv( GL_LIGHT2, GL_AMBIENT,  ambient );
    glLightfv( GL_LIGHT2, GL_DIFFUSE,  diffuse );
    glLightfv( GL_LIGHT2, GL_SPECULAR, specular );
    glLightfv( GL_LIGHT2, GL_POSITION, position1 );

    const GLfloat position2[]= { 0.0f, 0.0f, -1.0f, 0.0f };                     // defines a directional light that points along the positive z-axis
    glLightfv( GL_LIGHT2, GL_AMBIENT,  ambient );
    glLightfv( GL_LIGHT2, GL_DIFFUSE,  diffuse );
    glLightfv( GL_LIGHT2, GL_SPECULAR, specular );
    glLightfv( GL_LIGHT2, GL_POSITION, position2 );
    //glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
}

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );


    glEnable(GL_CULL_FACE);

    glClearColor(0.0, 0.0, 1.0, 0.0);
{
    const SFVEC4F ambient  = SFVEC4F( 0.2,0.2,0.2,  1.0f );
    const SFVEC4F diffuse  = SFVEC4F( 0.5,0.1,0.1,  1.0f );
    const SFVEC4F specular = SFVEC4F( 0.1,0.1,0.8, 1.0f );
    //const SFVEC4F emissive = SFVEC4F( material.m_Emissive, 1.0f );

    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  &ambient.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  &diffuse.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r );
    //glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.r );
    }
    glMaterialf(  GL_FRONT_AND_BACK, GL_SHININESS, 128 );
    glDisable( GL_COLOR_MATERIAL );

    glFrontFace( GL_CCW );                                                      // This is the openGL default
    glEnable( GL_NORMALIZE );                                                   // This allow openGL to normalize the normals after transformations
}


void display(void)
{
  glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
  //glClearDepth( 1.0f );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glPushMatrix();
  //tbMatrix();

  switch(Op) {
  case CSG_A:
    op_one(A);
    break;
  case CSG_B:
    op_one(B);
    break;
  case CSG_A_OR_B:
    op_or(A, B);
    break;
  case CSG_A_AND_B:
    op_and(A, B);
    break;
  case CSG_A_SUB_B:
    op_sub(A, B);
    break;
  case CSG_B_SUB_A:
    op_sub(B, A);
    break;
  }

  glPopMatrix();
}

static const C3D_RENDER_OGL_LEGACY *renderclass = NULL;

void draw_board(void)
{
    if( renderclass )
    {
        renderclass->GetBoardDispList()->DrawTop();
        renderclass->GetBoardDispList()->DrawBot();
    }
}

void draw_b_mask(void)
{
    if( renderclass )
    {
        const CLAYERS_OGL_DISP_LISTS *layer = renderclass->GetLayerDispList( B_Mask );
        layer->DrawTop();
        layer->DrawBot();
    }
}



static SFVEC3F light = SFVEC3F();

void C3D_RENDER_OGL_LEGACY::Redraw( bool aIsMoving )
{
    // Initialize openGL
    if( !m_is_opengl_initialized )
    {
        if( !initializeOpenGL() )
            return;
        aIsMoving = true;



        A = sphere;
        B = cube;

        //Op = CSG_A;
        //Op = CSG_B;
        //Op = CSG_A_OR_B;
        //Op = CSG_A_AND_B;
        //Op = CSG_A_SUB_B;
        Op = CSG_B_SUB_A;
    }
/*
    const GLfloat position0[]= { 0.0f, 0.0f, 1.0f, 0.0f };                      // defines a directional light that points along the negative z-axis
    glLightfv( GL_LIGHT0, GL_POSITION, position0 );
*/
    if( m_reloadRequested )
        reload();

    init();

    glEnable( GL_LIGHT0 );
    glEnable(GL_LIGHTING);


    // clear color and depth buffers
    // /////////////////////////////////////////////////////////////////////////
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClearDepth( 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    // Set projection and modelview matrixes
    // /////////////////////////////////////////////////////////////////////////
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( glm::value_ptr( m_settings.CameraGet().GetProjectionMatrix() ) );

    glMatrixMode( GL_MODELVIEW );

    // Position the headlight
    glLoadIdentity();

    //glTranslatef( -m_settings.GetBBox3DU().GetCenter().x, -m_settings.GetBBox3DU().GetCenter().y, -m_settings.GetBBox3DU().GetCenter().z );
//    glTranslatef( m_settings.GetBBox3DU().GetCenter().x, m_settings.GetBBox3DU().GetCenter().y, m_settings.GetBBox3DU().GetCenter().z );

    glLoadMatrixf( glm::value_ptr( m_settings.CameraGet().GetViewMatrix() ) );

    {
        const SFVEC3F &cameraPos = m_settings.CameraGet().GetPos();//m_settings.CameraGet().GetPos();
        const GLfloat headlight_pos[] = { cameraPos.x, cameraPos.y, cameraPos.z, 1.0f };
        glLightfv( GL_LIGHT0, GL_POSITION, headlight_pos );
    }


    const SFVEC4F specular = SFVEC4F( 0.5f, 1.0f, 0.5f, 1.0f );

    glMaterialfv( GL_FRONT, GL_SPECULAR, &specular.r );
    glMaterialf(  GL_FRONT, GL_SHININESS, 10.0f );

    SFVEC4F layerColor4 = SFVEC4F( specular.x, specular.y, specular.z, 1.0f );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  &layerColor4.x );
    glDisable( GL_COLOR_MATERIAL );

/*
    glPushMatrix();

    glTranslatef( m_settings.GetBBox3DU().GetCenter().x, m_settings.GetBBox3DU().GetCenter().y, m_settings.GetBBox3DU().GetCenter().z );


    glEnable( GL_COLOR_MATERIAL );
    glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );

    //const SFVEC4F specular = SFVEC4F( 0.5f, 0.5f, 0.5f, 1.0f );

    glMaterialfv( GL_FRONT, GL_SPECULAR, &specular.r );
    glMaterialf(  GL_FRONT, GL_SHININESS, 10.0f );

    glColor3f( 0.9f, 0.0f, 0.0f );
    OGL_draw_arrow( SFVEC3F( 0.0f, 0.0f, 0.0f ),
                    SFVEC3F( RANGE_SCALE_3D / 1.0f, 0.0f, 0.0f ),
                    0.2f );

    glColor3f( 0.0f, 0.9f, 0.0f );
    OGL_draw_arrow( SFVEC3F( 0.0f, 0.0f, 0.0f ),
                    SFVEC3F( 0.0f, RANGE_SCALE_3D / 1.0f, 0.0f ),
                    0.2f );

    glColor3f( 0.0f, 0.0f, 0.9f );
    OGL_draw_arrow( SFVEC3F( 0.0f, 0.0f, 0.0f ),
                    SFVEC3F( 0.0f, 0.0f, RANGE_SCALE_3D / 1.0f ),
                    0.2f );
    glPopMatrix();
*/

/*
    for( MAP_OGL_DISP_LISTS::const_iterator ii = m_ogl_disp_lists_layers.begin();
         ii != m_ogl_disp_lists_layers.end();
         ii++ )
    {

        LAYER_ID layer_id = (LAYER_ID)(ii->first);

        // Mask kayers are not processed here because they are a special case
        if( (layer_id == B_Mask) || (layer_id == F_Mask) )
            continue;

        CLAYERS_OGL_DISP_LISTS *pLayerDispList = static_cast<CLAYERS_OGL_DISP_LISTS*>(ii->second);
        pLayerDispList->DrawAll();
    }
*/
    renderclass = this;
    //op_sub( draw_b_mask, draw_board );
    //op_sub( draw_board, draw_b_mask );


    const CLAYERS_OGL_DISP_LISTS *layer = renderclass->GetLayerDispList( B_Mask );

    //glEnable(GL_LIGHTING);
    //glEnable(GL_LIGHT0);
/*
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_CULL_FACE);
    m_ogl_disp_list_board->DrawBot();
*/

    glCullFace(GL_BACK);

    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE );
    layer->DrawBot();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 0, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP );
    //glDisable(GL_DEPTH_TEST);
    //glCullFace(face);
    m_ogl_disp_list_board->DrawBot();

    //glClear( GL_STENCIL_BUFFER_BIT );

    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 2, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE );
    layer->DrawTop();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR );
    //glDisable(GL_DEPTH_TEST);
    //glCullFace(face);
    m_ogl_disp_list_board->DrawTop();

    //glClear( GL_STENCIL_BUFFER_BIT );

    //glCullFace(GL_FRONT);
/*
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_GEQUAL, 3, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR );
    layer->DrawBot();
    layer->DrawTop();

*/
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glCullFace(GL_FRONT);
    glStencilFunc(GL_GEQUAL, 3, 0x03);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP );
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    layer->DrawMiddle();


}


bool C3D_RENDER_OGL_LEGACY::initializeOpenGL()
{
    GLenum err = glewInit();

    if( GLEW_OK != err )
    {
        std::string msgError = (const char*) glewGetErrorString( err );

        return false;
    }
    else
    {
        wxLogTrace( m_logTrace,
                    wxString( wxT( "C3D_RENDER_OGL_LEGACY::initializeOpenGL Using GLEW " ) ) +
                    FROM_UTF8( (char*) glewGetString( GLEW_VERSION ) ) );
    }

    // 4-byte pixel alignment
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

    // Initialize the open GL texture to draw the filled semi-circle of the segments
    CIMAGE *circleImage = new CIMAGE( SIZE_OF_CIRCLE_TEXTURE, SIZE_OF_CIRCLE_TEXTURE );
    if( !circleImage )
        return false;

    circleImage->CircleFilled( (SIZE_OF_CIRCLE_TEXTURE / 2) - 0, (SIZE_OF_CIRCLE_TEXTURE / 2) - 0, (SIZE_OF_CIRCLE_TEXTURE / 2) - 4, 0xFF );
    //circleImage->CircleFilled( (SIZE_OF_CIRCLE_TEXTURE / 4)*1.5f - 1, (SIZE_OF_CIRCLE_TEXTURE / 4)*1.5f - 1, (SIZE_OF_CIRCLE_TEXTURE / 4)*1.5f - 2, 0xFF );

    CIMAGE *bluredCircle = new CIMAGE( *circleImage );
    circleImage->EfxFilter( bluredCircle, FILTER_GAUSSIAN_BLUR2 );

    m_ogl_circle_texture = OGL_LoadTexture( *circleImage );

    circleImage->SaveAsPNG("circleImage.png");
    delete circleImage;
    circleImage = 0;

    if( bluredCircle )
    {
        bluredCircle->SaveAsPNG("circleImage_blured.png");
        delete bluredCircle;
        bluredCircle = 0;
    }

    m_is_opengl_initialized = true;
    return true;
}


void C3D_RENDER_OGL_LEGACY::ogl_set_arrow_material()
{
    glEnable( GL_COLOR_MATERIAL );
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    const SFVEC4F specular = SFVEC4F( 0.1f, 0.1f, 0.1f, 1.0f );

    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r );
    glMaterialf(  GL_FRONT_AND_BACK, GL_SHININESS, 96.0f );
}


void C3D_RENDER_OGL_LEGACY::ogl_free_all_display_lists()
{
    for( MAP_OGL_DISP_LISTS::const_iterator ii = m_ogl_disp_lists_layers.begin();
         ii != m_ogl_disp_lists_layers.end();
         ii++ )
    {
        CLAYERS_OGL_DISP_LISTS *pLayerDispList = static_cast<CLAYERS_OGL_DISP_LISTS*>(ii->second);
        delete pLayerDispList;
    }

    m_ogl_disp_lists_layers.clear();

    for( MAP_TRIANGLES::const_iterator ii = m_triangles.begin();
         ii != m_triangles.end();
         ii++ )
    {
        CLAYER_TRIANGLES *pointer = static_cast<CLAYER_TRIANGLES*>(ii->second);
        delete pointer;
    }

    m_triangles.clear();

    delete m_ogl_disp_list_board;
    m_ogl_disp_list_board = 0;
}
