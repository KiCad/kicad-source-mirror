/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Implements a model viewer canvas. The purpose of the model viewer is to render
 * 3d models that come in the original data from the files without any transformations.
 */

#include <gal/opengl/kiglew.h>    // Must be included first
#include <iostream>
#include "3d_rendering/opengl/3d_model.h"
#include "eda_3d_model_viewer.h"
#include "../3d_rendering/opengl/opengl_utils.h"
#include "../3d_cache/3d_cache.h"
#include <wx/dcclient.h>
#include <base_units.h>
#include <build_version.h>
#include <gal/opengl/gl_context_mgr.h>
#include <settings/common_settings.h>
#include <pgm_base.h>
#include <dpi_scaling_common.h>
#include <class_draw_panel_gal.h>
#include <macros.h>

/**
 * Scale conversion from 3d model units to pcb units
 */
#define UNITS3D_TO_UNITSPCB ( pcbIUScale.IU_PER_MM )

/**
 * Trace mask used to enable or disable the trace output of this class.
 * The debug output can be turned on by setting the WXTRACE environment variable to
 * "KI_TRACE_EDA_3D_MODEL_VIEWER".  See the wxWidgets documentation on wxLogTrace for
 * more information.
 *
 * @ingroup trace_env_vars
 */
const wxChar* EDA_3D_MODEL_VIEWER::m_logTrace = wxT( "KI_TRACE_EDA_3D_MODEL_VIEWER" );


BEGIN_EVENT_TABLE( EDA_3D_MODEL_VIEWER, wxGLCanvas )
    EVT_PAINT( EDA_3D_MODEL_VIEWER::OnPaint )

    // mouse events
    EVT_LEFT_DOWN( EDA_3D_MODEL_VIEWER::OnLeftDown )
    EVT_LEFT_UP( EDA_3D_MODEL_VIEWER::OnLeftUp )
    EVT_MIDDLE_UP( EDA_3D_MODEL_VIEWER::OnMiddleUp )
    EVT_MIDDLE_DOWN( EDA_3D_MODEL_VIEWER::OnMiddleDown)
    EVT_RIGHT_DOWN( EDA_3D_MODEL_VIEWER::OnRightClick )
    EVT_MOUSEWHEEL( EDA_3D_MODEL_VIEWER::OnMouseWheel )
    EVT_MOTION( EDA_3D_MODEL_VIEWER::OnMouseMove )

#ifdef USE_OSX_MAGNIFY_EVENT
     EVT_MAGNIFY(   EDA_3D_MODEL_VIEWER::OnMagnify )
#endif

    // other events
    EVT_ERASE_BACKGROUND( EDA_3D_MODEL_VIEWER::OnEraseBackground )
END_EVENT_TABLE()


// This defines the range that all coord will have to be rendered.
// It will use this value to convert to a normalized value between
// -(RANGE_SCALE_3D/2) .. +(RANGE_SCALE_3D/2)
#define RANGE_SCALE_3D 8.0f


EDA_3D_MODEL_VIEWER::EDA_3D_MODEL_VIEWER( wxWindow* aParent, const wxGLAttributes& aGLAttribs,
                                          S3D_CACHE* aCacheManager ) :
        HIDPI_GL_CANVAS( EDA_DRAW_PANEL_GAL::GetVcSettings(), aParent, aGLAttribs, wxID_ANY,
                         wxDefaultPosition, wxDefaultSize,
                         wxFULL_REPAINT_ON_RESIZE ),
        m_trackBallCamera( RANGE_SCALE_3D * 4.0f ),
        m_cacheManager( aCacheManager )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::EDA_3D_MODEL_VIEWER" ) );

    m_ogl_initialized = false;
    m_reload_is_needed = false;
    m_ogl_3dmodel = nullptr;
    m_3d_model = nullptr;
    m_BiuTo3dUnits = 1.0;

    m_glRC = nullptr;
}


EDA_3D_MODEL_VIEWER::~EDA_3D_MODEL_VIEWER()
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::~EDA_3D_MODEL_VIEWER" ) );
    GL_CONTEXT_MANAGER* gl_mgr = Pgm().GetGLContextManager();

    if( m_glRC )
    {
        gl_mgr->LockCtx( m_glRC, this );

        delete m_ogl_3dmodel;
        m_ogl_3dmodel = nullptr;

        gl_mgr->UnlockCtx( m_glRC );
        gl_mgr->DestroyCtx( m_glRC );
    }
}


void EDA_3D_MODEL_VIEWER::Set3DModel( const S3DMODEL& a3DModel )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::Set3DModel with a S3DMODEL" ) );

    // Validate a3DModel pointers
    wxASSERT( a3DModel.m_Materials != nullptr );
    wxASSERT( a3DModel.m_Meshes != nullptr );
    wxASSERT( a3DModel.m_MaterialsSize > 0 );
    wxASSERT( a3DModel.m_MeshesSize > 0 );

    // Delete the old model
    delete m_ogl_3dmodel;
    m_ogl_3dmodel = nullptr;

    m_3d_model = nullptr;

    if( ( a3DModel.m_Materials != nullptr ) && ( a3DModel.m_Meshes != nullptr )
      && ( a3DModel.m_MaterialsSize > 0 ) && ( a3DModel.m_MeshesSize > 0 ) )
    {
        m_3d_model = &a3DModel;
        m_reload_is_needed = true;
    }

    Refresh();
}


void EDA_3D_MODEL_VIEWER::Set3DModel( const wxString& aModelPathName)
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::Set3DModel with a wxString" ) );

    if( m_cacheManager )
    {
        const S3DMODEL* model = m_cacheManager->GetModel( aModelPathName, wxEmptyString, {} );

        if( model )
            Set3DModel( (const S3DMODEL &)*model );
        else
            Clear3DModel();
    }
}


void EDA_3D_MODEL_VIEWER::Clear3DModel()
{
    // Delete the old model
    m_reload_is_needed = false;

    delete m_ogl_3dmodel;
    m_ogl_3dmodel = nullptr;

    m_3d_model = nullptr;

    Refresh();
}


void EDA_3D_MODEL_VIEWER::ogl_initialize()
{
    const GLenum err = glewInit();

    if( GLEW_OK != err )
    {
        const wxString msgError = (const char*) glewGetErrorString( err );

        wxLogMessage( msgError );
    }
    else
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::ogl_initialize Using GLEW version %s" ),
                    From_UTF8( (char*) glewGetString( GLEW_VERSION ) ) );
    }

    SetOpenGLInfo( (const char*) glGetString( GL_VENDOR ), (const char*) glGetString( GL_RENDERER ),
                   (const char*) glGetString( GL_VERSION ) );

    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glShadeModel( GL_SMOOTH );
    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_NORMALIZE );

    // Setup light
    // https://www.opengl.org/sdk/docs/man2/xhtml/glLight.xml
    const GLfloat ambient[]  = { 0.01f, 0.01f, 0.01f, 1.0f };
    const GLfloat diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    const GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // defines a directional light that points along the negative z-axis
    const GLfloat position[] = { 0.0f, 0.0f, 2.0f * RANGE_SCALE_3D, 0.0f };

    const GLfloat lmodel_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    glLightfv( GL_LIGHT0, GL_AMBIENT,  ambient );
    glLightfv( GL_LIGHT0, GL_DIFFUSE,  diffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, specular );
    glLightfv( GL_LIGHT0, GL_POSITION, position );
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lmodel_ambient );
}


void EDA_3D_MODEL_VIEWER::ogl_set_arrow_material()
{
    glEnable( GL_COLOR_MATERIAL );
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    const SFVEC4F specular = SFVEC4F( 0.1f, 0.1f, 0.1f, 1.0f );

    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r );
    glMaterialf(  GL_FRONT_AND_BACK, GL_SHININESS, 96.0f );
}


void EDA_3D_MODEL_VIEWER::OnPaint( wxPaintEvent& event )
{
    event.Skip( false );

    // SwapBuffer requires the window to be shown before calling
    if( !IsShownOnScreen() )
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::OnPaint !IsShown" ) );
        return;
    }

    // "Makes the OpenGL state that is represented by the OpenGL rendering
    //  context context current, i.e. it will be used by all subsequent OpenGL calls.
    //  This function may only be called when the window is shown on screen"
    if( m_glRC == nullptr )
        m_glRC = Pgm().GetGLContextManager()->CreateCtx( this );

    // CreateCtx could and does fail per sentry crash events, lets be graceful
    if( m_glRC == nullptr )
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::OnPaint creating gl context failed" ) );
        return;
    }

    Pgm().GetGLContextManager()->LockCtx( m_glRC, this );

    // Set the OpenGL viewport according to the client size of this canvas.
    // This is done here rather than in a wxSizeEvent handler because our
    // OpenGL rendering context (and thus viewport setting) is used with
    // multiple canvases: If we updated the viewport in the wxSizeEvent
    // handler, changing the size of one canvas causes a viewport setting that
    // is wrong when next another canvas is repainted.
    wxSize clientSize = GetNativePixelSize();

    if( !m_ogl_initialized )
    {
        m_ogl_initialized = true;
        ogl_initialize();
    }

    if( m_reload_is_needed )
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::OnPaint m_reload_is_needed" ) );

        m_reload_is_needed = false;
        m_ogl_3dmodel      = new MODEL_3D( *m_3d_model, MATERIAL_MODE::NORMAL );

        // It convert a model as it was a board, so get the max size dimension of the board
        // and compute the conversion scale
        m_BiuTo3dUnits =
                (double) RANGE_SCALE_3D
                / ( (double) m_ogl_3dmodel->GetBBox().GetMaxDimension() * UNITS3D_TO_UNITSPCB );
    }

    glViewport( 0, 0, clientSize.x, clientSize.y );

    m_trackBallCamera.SetCurWindowSize( clientSize );

    // clear color and depth buffers
    glEnable( GL_DEPTH_TEST );
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClearDepth( 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Set projection and modelview matrices
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( glm::value_ptr( m_trackBallCamera.GetProjectionMatrix() ) );
    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( glm::value_ptr( m_trackBallCamera.GetViewMatrix() ) );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    // Render Model
    if( m_ogl_3dmodel )
    {
        glPushMatrix();

        double modelunit_to_3d_units_factor = m_BiuTo3dUnits * UNITS3D_TO_UNITSPCB;

        glScaled( modelunit_to_3d_units_factor, modelunit_to_3d_units_factor,
                  modelunit_to_3d_units_factor );

        // Center model in the render viewport
        const SFVEC3F model_center = m_ogl_3dmodel->GetBBox().GetCenter();

        glTranslatef( -model_center.x, -model_center.y, -model_center.z );

        m_ogl_3dmodel->BeginDrawMulti( true );

        m_ogl_3dmodel->DrawOpaque( false );

        glDepthMask( GL_FALSE );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        m_ogl_3dmodel->DrawTransparent( 1.0f, false );

        glDisable( GL_BLEND );
        glDepthMask( GL_TRUE );

        m_ogl_3dmodel->EndDrawMulti();

        glPopMatrix();
    }

    // YxY squared view port
    glViewport( 0, 0, clientSize.y / 8 , clientSize.y / 8 );
    glClear( GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 45.0f, 1.0f, 0.01f, RANGE_SCALE_3D * 2.0f );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    const glm::mat4 TranslationMatrix = glm::translate( glm::mat4(1.0f),
                                                        SFVEC3F( 0.0f, 0.0f, -RANGE_SCALE_3D ) );

    const glm::mat4 ViewMatrix = TranslationMatrix * m_trackBallCamera.GetRotationMatrix();

    glLoadMatrixf( glm::value_ptr( ViewMatrix ) );

    ogl_set_arrow_material();

    glColor3f( 0.9f, 0.0f, 0.0f );
    DrawRoundArrow( SFVEC3F( 0.0f, 0.0f, 0.0f ), SFVEC3F( RANGE_SCALE_3D / 2.65f, 0.0f, 0.0f ),
                    0.275f );

    glColor3f( 0.0f, 0.9f, 0.0f );
    DrawRoundArrow( SFVEC3F( 0.0f, 0.0f, 0.0f ), SFVEC3F( 0.0f, RANGE_SCALE_3D / 2.65f, 0.0f ),
                    0.275f );

    glColor3f( 0.0f, 0.0f, 0.9f );
    DrawRoundArrow( SFVEC3F( 0.0f, 0.0f, 0.0f ), SFVEC3F( 0.0f, 0.0f, RANGE_SCALE_3D / 2.65f ),
                    0.275f );

    // "Swaps the double-buffer of this window, making the back-buffer the
    //  front-buffer and vice versa, so that the output of the previous OpenGL
    //  commands is displayed on the window."
    SwapBuffers();

    Pgm().GetGLContextManager()->UnlockCtx( m_glRC );
}


void EDA_3D_MODEL_VIEWER::OnEraseBackground( wxEraseEvent& event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::OnEraseBackground" ) );
    // Do nothing, to avoid flashing.
}


void EDA_3D_MODEL_VIEWER::OnMouseWheel( wxMouseEvent& event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_MODEL_VIEWER::OnMouseWheel" ) );

    if( event.ShiftDown() )
    {
        //if( event.GetWheelRotation() < 0 )
            //SetView3D( VIEW_3D_TYPE::VIEW3D_PAN_UP );    // move up
        //else
            //SetView3D( VIEW_3D_TYPE::VIEW3D_PAN_DOWN );  // move down
    }
    else if( event.ControlDown() )
    {
        //if( event.GetWheelRotation() > 0 )
            //SetView3D( VIEW_3D_TYPE::VIEW3D_PAN_RIGHT ); // move right
        //else
            //SetView3D( VIEW_3D_TYPE::VIEW3D_PAN_LEFT );  // move left
    }
    else
    {
        m_trackBallCamera.Zoom( event.GetWheelRotation() > 0 ? 1.1f : 1/1.1f );

        //DisplayStatus();
        Refresh( false );
    }

    m_trackBallCamera.SetCurMousePosition( event.GetPosition() );
}


#ifdef USE_OSX_MAGNIFY_EVENT
void EDA_3D_MODEL_VIEWER::OnMagnify( wxMouseEvent& event )
{
}
#endif


void EDA_3D_MODEL_VIEWER::OnMouseMove( wxMouseEvent& event )
{
    const wxSize&  nativeWinSize  = GetNativePixelSize();
    const wxPoint& nativePosition = GetNativePosition( event.GetPosition() );

    m_trackBallCamera.SetCurWindowSize( nativeWinSize );

    if( event.Dragging() )
    {
        if( event.LeftIsDown() )            // Drag
            m_trackBallCamera.Drag( nativePosition );

        // orientation has changed, redraw mesh
        Refresh( false );
    }

    m_trackBallCamera.SetCurMousePosition( nativePosition );
}


void EDA_3D_MODEL_VIEWER::OnLeftDown( wxMouseEvent& event )
{
    event.Skip();
}


void EDA_3D_MODEL_VIEWER::OnLeftUp( wxMouseEvent& event )
{
    event.Skip();
}


void EDA_3D_MODEL_VIEWER::OnMiddleDown( wxMouseEvent& event )
{
    event.Skip();
}


void EDA_3D_MODEL_VIEWER::OnMiddleUp( wxMouseEvent& event )
{
    event.Skip();
}


void EDA_3D_MODEL_VIEWER::OnRightClick( wxMouseEvent& event )
{
    event.Skip();
}

