/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2023 CERN
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

#include <cstdint>
#include <gal/opengl/kiglew.h>    // Must be included first

#include "plugins/3dapi/xv3d_types.h"
#include "render_3d_opengl.h"
#include "opengl_utils.h"
#include "common_ogl/ogl_utils.h"
#include <board.h>
#include <footprint.h>
#include <gal/opengl/gl_context_mgr.h>
#include <3d_math.h>
#include <glm/geometric.hpp>
#include <lset.h>
#include <pgm_base.h>
#include <math/util.h>      // for KiROUND
#include <utility>
#include <vector>
#include <wx/log.h>

#include <base_units.h>

/**
 * Scale conversion from 3d model units to pcb units
 */
#define UNITS3D_TO_UNITSPCB ( pcbIUScale.IU_PER_MM )

RENDER_3D_OPENGL::RENDER_3D_OPENGL( EDA_3D_CANVAS* aCanvas, BOARD_ADAPTER& aAdapter, CAMERA& aCamera ) :
        RENDER_3D_BASE( aAdapter, aCamera ),
        m_canvas( aCanvas )
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_OPENGL::RENDER_3D_OPENGL" ) );

    m_layers.clear();
    m_outerLayerHoles.clear();
    m_innerLayerHoles.clear();
    m_triangles.clear();
    m_board = nullptr;
    m_antiBoard = nullptr;

    m_platedPadsFront = nullptr;
    m_platedPadsBack = nullptr;
    m_offboardPadsFront = nullptr;
    m_offboardPadsBack = nullptr;

    m_outerThroughHoles = nullptr;
    m_outerThroughHoleRings = nullptr;
    m_outerViaThroughHoles = nullptr;
    m_microviaHoles = nullptr;
    m_padHoles = nullptr;
    m_viaFrontCover = nullptr;
    m_viaBackCover = nullptr;

    m_circleTexture = 0;
    m_grid = 0;
    m_lastGridType = GRID3D_TYPE::NONE;
    m_currentRollOverItem = nullptr;
    m_boardWithHoles = nullptr;
    m_postMachinePlugs = nullptr;

    m_3dModelMap.clear();

    m_spheres_gizmo = new SPHERES_GIZMO( 4, 4 );
}


RENDER_3D_OPENGL::~RENDER_3D_OPENGL()
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_OPENGL::RENDER_3D_OPENGL" ) );

    freeAllLists();

    glDeleteTextures( 1, &m_circleTexture );

    delete m_spheres_gizmo;
}


int RENDER_3D_OPENGL::GetWaitForEditingTimeOut()
{
    return 50; // ms
}


void RENDER_3D_OPENGL::SetCurWindowSize( const wxSize& aSize )
{
    if( m_windowSize != aSize )
    {
        int viewport[4];
        int fbWidth, fbHeight;
        glGetIntegerv( GL_VIEWPORT, viewport );

        m_windowSize = aSize;
        glViewport( 0, 0, m_windowSize.x, m_windowSize.y );
        setGizmoViewport( 0, 0, m_windowSize.x, m_windowSize.y );
        // Initialize here any screen dependent data here
    }
}


void RENDER_3D_OPENGL::setLightFront( bool enabled )
{
    if( enabled )
        glEnable( GL_LIGHT0 );
    else
        glDisable( GL_LIGHT0 );
}


void RENDER_3D_OPENGL::setLightTop( bool enabled )
{
    if( enabled )
        glEnable( GL_LIGHT1 );
    else
        glDisable( GL_LIGHT1 );
}


void RENDER_3D_OPENGL::setLightBottom( bool enabled )
{
    if( enabled )
        glEnable( GL_LIGHT2 );
    else
        glDisable( GL_LIGHT2 );
}


void RENDER_3D_OPENGL::resetSelectedGizmoSphere()
{
    m_spheres_gizmo->resetSelectedGizmoSphere();
}


SPHERES_GIZMO::GizmoSphereSelection RENDER_3D_OPENGL::getSelectedGizmoSphere() const
{
    return m_spheres_gizmo->getSelectedGizmoSphere();
}


void RENDER_3D_OPENGL::setGizmoViewport( int x, int y, int width, int height )
{
    m_spheres_gizmo->setViewport( x, y, width, height );
}


std::tuple<int, int, int, int> RENDER_3D_OPENGL::getGizmoViewport() const
{
    return m_spheres_gizmo->getViewport();
}


void RENDER_3D_OPENGL::handleGizmoMouseInput( int mouseX, int mouseY )
{
    m_spheres_gizmo->handleMouseInput( mouseX, mouseY );
}


void RENDER_3D_OPENGL::setupMaterials()
{
    m_materials = {};

    // http://devernay.free.fr/cours/opengl/materials.html

    // Plated copper
    // Copper material mixed with the copper color
    m_materials.m_Copper.m_Ambient  = SFVEC3F( m_boardAdapter.m_CopperColor.r * 0.1f,
                                               m_boardAdapter.m_CopperColor.g * 0.1f,
                                               m_boardAdapter.m_CopperColor.b * 0.1f);

    m_materials.m_Copper.m_Specular = SFVEC3F( m_boardAdapter.m_CopperColor.r * 0.75f + 0.25f,
                                               m_boardAdapter.m_CopperColor.g * 0.75f + 0.25f,
                                               m_boardAdapter.m_CopperColor.b * 0.75f + 0.25f );

    // This guess the material type(ex: copper vs gold) to determine the
    // shininess factor between 0.1 and 0.4
    float shininessfactor = 0.40f - mapf( fabs( m_boardAdapter.m_CopperColor.r -
                                                m_boardAdapter.m_CopperColor.g ),
                                          0.15f, 1.00f,
                                          0.00f, 0.30f );

    m_materials.m_Copper.m_Shininess = shininessfactor * 128.0f;
    m_materials.m_Copper.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );


    // Non plated copper (raw copper)
    m_materials.m_NonPlatedCopper.m_Ambient = SFVEC3F( 0.191f, 0.073f, 0.022f );
    m_materials.m_NonPlatedCopper.m_Diffuse = SFVEC3F( 184.0f / 255.0f, 115.0f / 255.0f,
                                                       50.0f / 255.0f );
    m_materials.m_NonPlatedCopper.m_Specular = SFVEC3F( 0.256f, 0.137f, 0.086f );
    m_materials.m_NonPlatedCopper.m_Shininess = 0.1f * 128.0f;
    m_materials.m_NonPlatedCopper.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );

    // Paste material mixed with paste color
    m_materials.m_Paste.m_Ambient = SFVEC3F( m_boardAdapter.m_SolderPasteColor.r,
                                             m_boardAdapter.m_SolderPasteColor.g,
                                             m_boardAdapter.m_SolderPasteColor.b );

    m_materials.m_Paste.m_Specular = SFVEC3F( m_boardAdapter.m_SolderPasteColor.r *
                                              m_boardAdapter.m_SolderPasteColor.r,
                                              m_boardAdapter.m_SolderPasteColor.g *
                                              m_boardAdapter.m_SolderPasteColor.g,
                                              m_boardAdapter.m_SolderPasteColor.b *
                                              m_boardAdapter.m_SolderPasteColor.b );

    m_materials.m_Paste.m_Shininess = 0.1f * 128.0f;
    m_materials.m_Paste.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );

    // Silk screen material mixed with silk screen color
    m_materials.m_SilkSTop.m_Ambient = SFVEC3F( m_boardAdapter.m_SilkScreenColorTop.r,
                                                m_boardAdapter.m_SilkScreenColorTop.g,
                                                m_boardAdapter.m_SilkScreenColorTop.b );

    m_materials.m_SilkSTop.m_Specular = SFVEC3F(
            m_boardAdapter.m_SilkScreenColorTop.r * m_boardAdapter.m_SilkScreenColorTop.r + 0.10f,
            m_boardAdapter.m_SilkScreenColorTop.g * m_boardAdapter.m_SilkScreenColorTop.g + 0.10f,
            m_boardAdapter.m_SilkScreenColorTop.b * m_boardAdapter.m_SilkScreenColorTop.b + 0.10f );

    m_materials.m_SilkSTop.m_Shininess = 0.078125f * 128.0f;
    m_materials.m_SilkSTop.m_Emissive  = SFVEC3F( 0.0f, 0.0f, 0.0f );

    // Silk screen material mixed with silk screen color
    m_materials.m_SilkSBot.m_Ambient = SFVEC3F( m_boardAdapter.m_SilkScreenColorBot.r,
                                                m_boardAdapter.m_SilkScreenColorBot.g,
                                                m_boardAdapter.m_SilkScreenColorBot.b );

    m_materials.m_SilkSBot.m_Specular = SFVEC3F(
            m_boardAdapter.m_SilkScreenColorBot.r * m_boardAdapter.m_SilkScreenColorBot.r + 0.10f,
            m_boardAdapter.m_SilkScreenColorBot.g * m_boardAdapter.m_SilkScreenColorBot.g + 0.10f,
            m_boardAdapter.m_SilkScreenColorBot.b * m_boardAdapter.m_SilkScreenColorBot.b + 0.10f );

    m_materials.m_SilkSBot.m_Shininess = 0.078125f * 128.0f;
    m_materials.m_SilkSBot.m_Emissive  = SFVEC3F( 0.0f, 0.0f, 0.0f );

    m_materials.m_SolderMask.m_Shininess    = 0.8f * 128.0f;
    m_materials.m_SolderMask.m_Emissive     = SFVEC3F( 0.0f, 0.0f, 0.0f );

    // Epoxy material
    m_materials.m_EpoxyBoard.m_Ambient   = SFVEC3F( 117.0f / 255.0f, 97.0f / 255.0f,
                                                     47.0f / 255.0f );

    m_materials.m_EpoxyBoard.m_Specular  = SFVEC3F( 18.0f / 255.0f, 3.0f / 255.0f,
                                                    20.0f / 255.0f );

    m_materials.m_EpoxyBoard.m_Shininess = 0.1f * 128.0f;
    m_materials.m_EpoxyBoard.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );
}


void RENDER_3D_OPENGL::setLayerMaterial( PCB_LAYER_ID aLayerID )
{
    if( m_boardAdapter.GetUseBoardEditorCopperLayerColors() && IsCopperLayer( aLayerID ) )
    {
        COLOR4D copper_color = m_boardAdapter.m_BoardEditorColors[aLayerID];
        m_materials.m_Copper.m_Diffuse = SFVEC3F( copper_color.r, copper_color.g,
                                                  copper_color.b );
        OglSetMaterial( m_materials.m_Copper, 1.0f );
        m_materials.m_NonPlatedCopper.m_Diffuse = m_materials.m_Copper.m_Diffuse;
        OglSetMaterial( m_materials.m_NonPlatedCopper, 1.0f );

        return;
    }

    switch( aLayerID )
    {
    case F_Mask:
    case B_Mask:
    {
        const SFVEC4F layerColor = aLayerID == F_Mask ? m_boardAdapter.m_SolderMaskColorTop
                                                      : m_boardAdapter.m_SolderMaskColorBot;

        m_materials.m_SolderMask.m_Diffuse = layerColor;

        // Convert Opacity to Transparency
        m_materials.m_SolderMask.m_Transparency = 1.0f - layerColor.a;

        m_materials.m_SolderMask.m_Ambient = m_materials.m_SolderMask.m_Diffuse * 0.3f;

        m_materials.m_SolderMask.m_Specular = m_materials.m_SolderMask.m_Diffuse
                                                * m_materials.m_SolderMask.m_Diffuse;

        OglSetMaterial( m_materials.m_SolderMask, 1.0f );
        break;
    }

    case B_Paste:
    case F_Paste:
        m_materials.m_Paste.m_Diffuse = m_boardAdapter.m_SolderPasteColor;
        OglSetMaterial( m_materials.m_Paste, 1.0f );
        break;

    case B_SilkS:
        m_materials.m_SilkSBot.m_Diffuse = m_boardAdapter.m_SilkScreenColorBot;
        OglSetMaterial( m_materials.m_SilkSBot, 1.0f );
        break;

    case F_SilkS:
        m_materials.m_SilkSTop.m_Diffuse = m_boardAdapter.m_SilkScreenColorTop;
        OglSetMaterial( m_materials.m_SilkSTop, 1.0f );
        break;

    case B_Adhes:
    case F_Adhes:
    case Dwgs_User:
    case Cmts_User:
    case Eco1_User:
    case Eco2_User:
    case Edge_Cuts:
    case Margin:
    case B_CrtYd:
    case F_CrtYd:
    case B_Fab:
    case F_Fab:
        switch( aLayerID )
        {
        case Dwgs_User: m_materials.m_Plastic.m_Diffuse = m_boardAdapter.m_UserDrawingsColor; break;
        case Cmts_User: m_materials.m_Plastic.m_Diffuse = m_boardAdapter.m_UserCommentsColor; break;
        case Eco1_User: m_materials.m_Plastic.m_Diffuse = m_boardAdapter.m_ECO1Color;         break;
        case Eco2_User: m_materials.m_Plastic.m_Diffuse = m_boardAdapter.m_ECO2Color;         break;
        case Edge_Cuts: m_materials.m_Plastic.m_Diffuse = m_boardAdapter.m_UserDrawingsColor; break;
        case Margin:    m_materials.m_Plastic.m_Diffuse = m_boardAdapter.m_UserDrawingsColor; break;
        default:
            m_materials.m_Plastic.m_Diffuse = m_boardAdapter.GetLayerColor( aLayerID );
            break;
        }

        m_materials.m_Plastic.m_Ambient = SFVEC3F( m_materials.m_Plastic.m_Diffuse.r * 0.05f,
                                                   m_materials.m_Plastic.m_Diffuse.g * 0.05f,
                                                   m_materials.m_Plastic.m_Diffuse.b * 0.05f );

        m_materials.m_Plastic.m_Specular = SFVEC3F( m_materials.m_Plastic.m_Diffuse.r * 0.7f,
                                                    m_materials.m_Plastic.m_Diffuse.g * 0.7f,
                                                    m_materials.m_Plastic.m_Diffuse.b * 0.7f );

        m_materials.m_Plastic.m_Shininess = 0.078125f * 128.0f;
        m_materials.m_Plastic.m_Emissive  = SFVEC3F( 0.0f, 0.0f, 0.0f );
        OglSetMaterial( m_materials.m_Plastic, 1.0f );
        break;

    default:
    {
        int layer3D = MapPCBLayerTo3DLayer( aLayerID );

        // Note: MUST do this in LAYER_3D space; User_1..User_45 are NOT contiguous
        if( layer3D >= LAYER_3D_USER_1 && layer3D <= LAYER_3D_USER_45 )
        {
            int user_idx = layer3D - LAYER_3D_USER_1;

            m_materials.m_Plastic.m_Diffuse = m_boardAdapter.m_UserDefinedLayerColor[ user_idx ];
            m_materials.m_Plastic.m_Ambient = SFVEC3F( m_materials.m_Plastic.m_Diffuse.r * 0.05f,
                                                       m_materials.m_Plastic.m_Diffuse.g * 0.05f,
                                                       m_materials.m_Plastic.m_Diffuse.b * 0.05f );

            m_materials.m_Plastic.m_Specular = SFVEC3F( m_materials.m_Plastic.m_Diffuse.r * 0.7f,
                                                        m_materials.m_Plastic.m_Diffuse.g * 0.7f,
                                                        m_materials.m_Plastic.m_Diffuse.b * 0.7f );

            m_materials.m_Plastic.m_Shininess = 0.078125f * 128.0f;
            m_materials.m_Plastic.m_Emissive  = SFVEC3F( 0.0f, 0.0f, 0.0f );
            OglSetMaterial( m_materials.m_Plastic, 1.0f );
            break;
        }

        m_materials.m_Copper.m_Diffuse = m_boardAdapter.m_CopperColor;
        OglSetMaterial( m_materials.m_Copper, 1.0f );
        break;
    }
    }
}


void init_lights()
{
    // Setup light
    // https://www.opengl.org/sdk/docs/man2/xhtml/glLight.xml
    const GLfloat ambient[]   = { 0.084f, 0.084f, 0.084f, 1.0f };
    const GLfloat diffuse0[]  = { 0.3f, 0.3f, 0.3f, 1.0f };
    const GLfloat specular0[] = { 0.5f, 0.5f, 0.5f, 1.0f };

    glLightfv( GL_LIGHT0, GL_AMBIENT,  ambient );
    glLightfv( GL_LIGHT0, GL_DIFFUSE,  diffuse0 );
    glLightfv( GL_LIGHT0, GL_SPECULAR, specular0 );

    const GLfloat diffuse12[]  = { 0.7f, 0.7f, 0.7f, 1.0f };
    const GLfloat specular12[] = { 0.7f, 0.7f, 0.7f, 1.0f };

    // defines a directional light that points along the negative z-axis
    GLfloat position[4]  = { 0.0f, 0.0f, 1.0f, 0.0f };

    // This makes a vector slight not perpendicular with XZ plane
    const SFVEC3F vectorLight = SphericalToCartesian( glm::pi<float>() * 0.03f,
                                                      glm::pi<float>() * 0.25f );

    position[0] = vectorLight.x;
    position[1] = vectorLight.y;
    position[2] = vectorLight.z;

    glLightfv( GL_LIGHT1, GL_AMBIENT,  ambient );
    glLightfv( GL_LIGHT1, GL_DIFFUSE,  diffuse12 );
    glLightfv( GL_LIGHT1, GL_SPECULAR, specular12 );
    glLightfv( GL_LIGHT1, GL_POSITION, position );

    // defines a directional light that points along the positive z-axis
    position[2] = -position[2];

    glLightfv( GL_LIGHT2, GL_AMBIENT,  ambient );
    glLightfv( GL_LIGHT2, GL_DIFFUSE,  diffuse12 );
    glLightfv( GL_LIGHT2, GL_SPECULAR, specular12 );
    glLightfv( GL_LIGHT2, GL_POSITION, position );

    const GLfloat lmodel_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lmodel_ambient );

    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );
}


void RENDER_3D_OPENGL::setCopperMaterial()
{
    OglSetMaterial( m_materials.m_NonPlatedCopper, 1.0f );
}


void RENDER_3D_OPENGL::setPlatedCopperAndDepthOffset( PCB_LAYER_ID aLayer_id )
{
    glEnable( GL_POLYGON_OFFSET_FILL );
    glPolygonOffset( -0.1f, -2.0f );
    setLayerMaterial( aLayer_id );
}


void RENDER_3D_OPENGL::unsetDepthOffset()
{
    glDisable( GL_POLYGON_OFFSET_FILL );
}


void RENDER_3D_OPENGL::renderBoardBody( bool aSkipRenderHoles )
{
    m_materials.m_EpoxyBoard.m_Diffuse   = m_boardAdapter.m_BoardBodyColor;

    // opacity to transparency
    m_materials.m_EpoxyBoard.m_Transparency = 1.0f - m_boardAdapter.m_BoardBodyColor.a;

    OglSetMaterial( m_materials.m_EpoxyBoard, 1.0f );

    OPENGL_RENDER_LIST* ogl_disp_list = nullptr;

    if( aSkipRenderHoles )
        ogl_disp_list = m_board;
    else
        ogl_disp_list = m_boardWithHoles;

    if( ogl_disp_list )
    {
        ogl_disp_list->ApplyScalePosition( -m_boardAdapter.GetBoardBodyThickness() / 2.0f,
                                           m_boardAdapter.GetBoardBodyThickness() );

        ogl_disp_list->SetItIsTransparent( true );
        ogl_disp_list->DrawAll();
    }

    // Also render post-machining plugs (board material that remains after backdrill/counterbore/countersink)
    if( !aSkipRenderHoles && m_postMachinePlugs )
    {
        m_postMachinePlugs->ApplyScalePosition( -m_boardAdapter.GetBoardBodyThickness() / 2.0f,
                                                m_boardAdapter.GetBoardBodyThickness() );

        m_postMachinePlugs->SetItIsTransparent( true );
        m_postMachinePlugs->DrawAll();
    }
}


static inline SFVEC4F premultiplyAlpha( const SFVEC4F& aInput )
{
    return SFVEC4F( aInput.r * aInput.a, aInput.g * aInput.a, aInput.b * aInput.a, aInput.a );
}


bool RENDER_3D_OPENGL::Redraw( bool aIsMoving, REPORTER* aStatusReporter,
                               REPORTER* aWarningReporter )
{
    // Initialize OpenGL
    if( !m_canvasInitialized )
    {
        if( !initializeOpenGL() )
            return false;
    }

    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& cfg = m_boardAdapter.m_Cfg->m_Render;

    if( m_reloadRequested )
    {
        std::unique_ptr<BUSY_INDICATOR> busy = CreateBusyIndicator();

        if( aStatusReporter )
            aStatusReporter->Report( _( "Loading..." ) );

        // Careful here!
        // We are in the middle of rendering and the reload method may show
        // a dialog box that requires the opengl context for a redraw
        Pgm().GetGLContextManager()->RunWithoutCtxLock( [this, aStatusReporter, aWarningReporter]()
        {
            reload( aStatusReporter, aWarningReporter );
        } );

        // generate a new 3D grid as the size of the board may had changed
        m_lastGridType = static_cast<GRID3D_TYPE>( cfg.grid_type );
        generate3dGrid( m_lastGridType );
    }
    else
    {
        // Check if grid was changed
        if( cfg.grid_type != m_lastGridType )
        {
            // and generate a new one
            m_lastGridType = static_cast<GRID3D_TYPE>( cfg.grid_type );
            generate3dGrid( m_lastGridType );
        }
    }

    setupMaterials();

    // Initial setup
    glDepthFunc( GL_LESS );
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CCW );    // This is the OpenGL default
    glEnable( GL_NORMALIZE ); // This allow OpenGL to normalize the normals after transformations
    glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

    if( aIsMoving && cfg.opengl_AA_disableOnMove )
        glDisable( GL_MULTISAMPLE );
    else
        glEnable( GL_MULTISAMPLE );

    // clear color and depth buffers
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClearDepth( 1.0f );
    glClearStencil( 0x00 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    OglResetTextureState();

    // Draw the background ( rectangle with color gradient)
    OglDrawBackground( premultiplyAlpha( m_boardAdapter.m_BgColorTop ),
                       premultiplyAlpha( m_boardAdapter.m_BgColorBot ) );

    glEnable( GL_DEPTH_TEST );

    // Set projection and modelview matrixes
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( glm::value_ptr( m_camera.GetProjectionMatrix() ) );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glLoadMatrixf( glm::value_ptr( m_camera.GetViewMatrix() ) );

    // Position the headlight
    setLightFront( true );
    setLightTop( true );
    setLightBottom( true );

    glEnable( GL_LIGHTING );

    {
        const SFVEC3F& cameraPos = m_camera.GetPos();

        // Place the light at a minimum Z so the diffuse factor will not drop
        // and the board will still look with good light.
        float zpos;

        if( cameraPos.z > 0.0f )
            zpos = glm::max( cameraPos.z, 0.5f ) + cameraPos.z * cameraPos.z;
        else
            zpos = glm::min( cameraPos.z,-0.5f ) - cameraPos.z * cameraPos.z;

        // This is a point light.
        const GLfloat headlight_pos[] = { cameraPos.x, cameraPos.y, zpos, 1.0f };

        glLightfv( GL_LIGHT0, GL_POSITION, headlight_pos );
    }

    bool skipThickness = aIsMoving && cfg.opengl_thickness_disableOnMove;
    bool skipRenderHoles = aIsMoving && cfg.opengl_holes_disableOnMove;
    bool skipRenderMicroVias = aIsMoving && cfg.opengl_microvias_disableOnMove;
    bool showThickness = !skipThickness;

    std::bitset<LAYER_3D_END> layerFlags = m_boardAdapter.GetVisibleLayers();

    setLayerMaterial( B_Cu );

    if( !( skipRenderMicroVias || skipRenderHoles ) && m_microviaHoles )
        m_microviaHoles->DrawAll();

    if( !skipRenderHoles && m_padHoles )
        m_padHoles->DrawAll();

    // Display copper and tech layers
    for( MAP_OGL_DISP_LISTS::const_iterator ii = m_layers.begin(); ii != m_layers.end(); ++ii )
    {
        const PCB_LAYER_ID layer = ( PCB_LAYER_ID )( ii->first );
        bool  isSilkLayer = layer == F_SilkS || layer == B_SilkS;
        bool  isMaskLayer = layer == F_Mask || layer == B_Mask;
        bool  isPasteLayer = layer == F_Paste || layer == B_Paste;

        // Mask layers are not processed here because they are a special case
        if( isMaskLayer )
            continue;

        // Do not show inner layers when it is displaying the board and board body is opaque
        // enough: the time to create inner layers can be *really significant*.
        // So avoid creating them is they are not very visible
        const double opacity_min = 0.8;

        if( layerFlags.test( LAYER_3D_BOARD ) && m_boardAdapter.m_BoardBodyColor.a > opacity_min )
        {
            // generating internal copper layers is time consuming. so skip them
            // if the board body is masking them (i.e. if the opacity is near 1.0)
            // B_Cu is layer 2 and all inner layers are higher values
            if( layer > B_Cu && IsCopperLayer( layer ) )
                continue;
        }

        glPushMatrix();

        OPENGL_RENDER_LIST* pLayerDispList = static_cast<OPENGL_RENDER_LIST*>( ii->second );

        if( IsCopperLayer( layer ) )
        {
            if( cfg.DifferentiatePlatedCopper() )
                setCopperMaterial();
            else
                setLayerMaterial( layer );

            OPENGL_RENDER_LIST* outerTH = nullptr;
            OPENGL_RENDER_LIST* viaHoles = nullptr;

            if( !skipRenderHoles )
            {
                outerTH = m_outerThroughHoles;
                viaHoles = m_outerLayerHoles[layer];
            }

            if( m_antiBoard )
                m_antiBoard->ApplyScalePosition( pLayerDispList );

            if( outerTH )
                outerTH->ApplyScalePosition( pLayerDispList );

            pLayerDispList->DrawCulled( showThickness, outerTH, viaHoles, m_antiBoard );

            // Draw plated & offboard pads
            if( layer == F_Cu && ( m_platedPadsFront || m_offboardPadsFront ) )
            {
                setPlatedCopperAndDepthOffset( layer );

                if( m_platedPadsFront )
                    m_platedPadsFront->DrawCulled( showThickness, outerTH, viaHoles, m_antiBoard );

                if( m_offboardPadsFront )
                    m_offboardPadsFront->DrawCulled( showThickness, outerTH, viaHoles );
            }
            else if( layer == B_Cu && ( m_platedPadsBack || m_offboardPadsBack ) )
            {
                setPlatedCopperAndDepthOffset( layer );

                if( m_platedPadsBack )
                    m_platedPadsBack->DrawCulled( showThickness, outerTH, viaHoles, m_antiBoard );

                if( m_offboardPadsBack )
                    m_offboardPadsBack->DrawCulled( showThickness, outerTH, viaHoles );
            }

            unsetDepthOffset();
        }
        else
        {
            setLayerMaterial( layer );

            OPENGL_RENDER_LIST* throughHolesOuter = nullptr;
            OPENGL_RENDER_LIST* anti_board = nullptr;
            OPENGL_RENDER_LIST* solder_mask = nullptr;

            if( !skipRenderHoles )
            {
                if( isSilkLayer && cfg.clip_silk_on_via_annuli )
                    throughHolesOuter = m_outerThroughHoleRings;
                else
                    throughHolesOuter = m_outerThroughHoles;
            }

            if( isSilkLayer && cfg.show_off_board_silk )
                anti_board = nullptr;
            else if( LSET::PhysicalLayersMask().test( layer ) )
                anti_board = m_antiBoard;

            if( isSilkLayer && cfg.subtract_mask_from_silk && !cfg.show_off_board_silk )
                solder_mask = m_layers[ ( layer == B_SilkS) ? B_Mask : F_Mask ];

            if( throughHolesOuter )
                throughHolesOuter->ApplyScalePosition( pLayerDispList );

            if( anti_board )
                anti_board->ApplyScalePosition( pLayerDispList );

            if( solder_mask )
                solder_mask->ApplyScalePosition( pLayerDispList );

            pLayerDispList->DrawCulled( showThickness, solder_mask, throughHolesOuter, anti_board );
        }

        glPopMatrix();
    }

    glm::mat4 cameraViewMatrix;

    glGetFloatv( GL_MODELVIEW_MATRIX, glm::value_ptr( cameraViewMatrix ) );

    // Render 3D Models (Non-transparent)
    renderOpaqueModels( cameraViewMatrix );

    // Display board body
    if( layerFlags.test( LAYER_3D_BOARD ) )
        renderBoardBody( skipRenderHoles );

    // Display transparent mask layers
    if( layerFlags.test( LAYER_3D_SOLDERMASK_TOP )
      || layerFlags.test( LAYER_3D_SOLDERMASK_BOTTOM ) )
    {
        // add a depth buffer offset, it will help to hide some artifacts
        // on silkscreen where the SolderMask is removed
        glEnable( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( 0.0f, -2.0f );

        if( m_camera.GetPos().z > 0 )
        {
            if( layerFlags.test( LAYER_3D_SOLDERMASK_BOTTOM ) )
            {
                renderSolderMaskLayer( B_Mask, m_boardAdapter.GetLayerTopZPos( B_Mask ),
                                       showThickness, skipRenderHoles );
            }

            if( layerFlags.test( LAYER_3D_SOLDERMASK_TOP ) )
            {
                renderSolderMaskLayer( F_Mask, m_boardAdapter.GetLayerBottomZPos( F_Mask ),
                                       showThickness, skipRenderHoles );
            }
        }
        else
        {
            if( layerFlags.test( LAYER_3D_SOLDERMASK_TOP ) )
            {
                renderSolderMaskLayer( F_Mask, m_boardAdapter.GetLayerBottomZPos( F_Mask ),
                                       showThickness, skipRenderHoles );
            }

            if( layerFlags.test( LAYER_3D_SOLDERMASK_BOTTOM ) )
            {
                renderSolderMaskLayer( B_Mask, m_boardAdapter.GetLayerTopZPos( B_Mask ),
                                       showThickness, skipRenderHoles );
            }
        }

        glDisable( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( 0.0f, 0.0f );
    }

    // Render 3D Models (Transparent)
    // !TODO: this can be optimized. If there are no transparent models (or no opacity),
    // then there is no need to make this function call.
    glDepthMask( GL_FALSE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Enables Texture Env so it can combine model transparency with each footprint opacity
    glEnable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE0 );

    // Uses an existent texture so the glTexEnv operations will work.
    glBindTexture( GL_TEXTURE_2D, m_circleTexture );

    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
    glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE );
    glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );

    glTexEnvi( GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR );
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );

    glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS );
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );

    glTexEnvi( GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR );
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
    glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_CONSTANT );
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA );

    renderTransparentModels( cameraViewMatrix );

    glDisable( GL_BLEND );
    OglResetTextureState();

    glDepthMask( GL_TRUE );

    // Render Grid
    if( cfg.grid_type != GRID3D_TYPE::NONE )
    {
        glDisable( GL_LIGHTING );

        if( glIsList( m_grid ) )
            glCallList( m_grid );

        glEnable( GL_LIGHTING );
    }

    // Render 3D arrows
    if( cfg.show_navigator )
        m_spheres_gizmo->render3dSpheresGizmo( m_camera.GetRotationMatrix() );

    // Return back to the original viewport (this is important if we want
    // to take a screenshot after the render)
    glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

    return false;
}


bool RENDER_3D_OPENGL::initializeOpenGL()
{
    glEnable( GL_LINE_SMOOTH );
    glShadeModel( GL_SMOOTH );

    // 4-byte pixel alignment
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

    // Initialize the open GL texture to draw the filled semi-circle of the segments
    IMAGE* circleImage = new IMAGE( SIZE_OF_CIRCLE_TEXTURE, SIZE_OF_CIRCLE_TEXTURE );

    if( !circleImage )
        return false;

    unsigned int circleRadius = ( SIZE_OF_CIRCLE_TEXTURE / 2 ) - 4;

    circleImage->CircleFilled( ( SIZE_OF_CIRCLE_TEXTURE / 2 ) - 0,
                               ( SIZE_OF_CIRCLE_TEXTURE / 2 ) - 0,
                               circleRadius,
                               0xFF );

    IMAGE* circleImageBlured = new IMAGE( circleImage->GetWidth(), circleImage->GetHeight() );

    circleImageBlured->EfxFilter_SkipCenter( circleImage, IMAGE_FILTER::GAUSSIAN_BLUR, circleRadius - 8 );

    m_circleTexture = OglLoadTexture( *circleImageBlured );

    delete circleImageBlured;
    circleImageBlured = nullptr;

    delete circleImage;
    circleImage = nullptr;

    init_lights();

    // Use this mode if you want see the triangle lines (debug proposes)
    //glPolygonMode( GL_FRONT_AND_BACK,  GL_LINE );
    m_canvasInitialized = true;

    return true;
}


void RENDER_3D_OPENGL::setArrowMaterial()
{
    glEnable( GL_COLOR_MATERIAL );
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    const SFVEC4F ambient  = SFVEC4F( 0.0f, 0.0f, 0.0f, 1.0f );
    const SFVEC4F diffuse  = SFVEC4F( 0.0f, 0.0f, 0.0f, 1.0f );
    const SFVEC4F emissive = SFVEC4F( 0.0f, 0.0f, 0.0f, 1.0f );
    const SFVEC4F specular = SFVEC4F( 0.1f, 0.1f, 0.1f, 1.0f );

    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 96.0f );

    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  &ambient.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  &diffuse.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.r );
}


void RENDER_3D_OPENGL::freeAllLists()
{
#define DELETE_AND_FREE( ptr ) \
    {                          \
        delete ptr;            \
        ptr = nullptr;         \
    }                          \

#define DELETE_AND_FREE_MAP( map )        \
    {                                     \
        for( auto& [ layer, ptr ] : map ) \
            delete ptr;                   \
                                          \
        map.clear();                      \
    }

    if( glIsList( m_grid ) )
        glDeleteLists( m_grid, 1 );

    m_grid = 0;

    DELETE_AND_FREE_MAP( m_layers )

    DELETE_AND_FREE( m_platedPadsFront )
    DELETE_AND_FREE( m_platedPadsBack )
    DELETE_AND_FREE( m_offboardPadsFront )
    DELETE_AND_FREE( m_offboardPadsBack )

    DELETE_AND_FREE_MAP( m_outerLayerHoles )
    DELETE_AND_FREE_MAP( m_innerLayerHoles )

    for( TRIANGLE_DISPLAY_LIST* list : m_triangles )
        delete list;

    m_triangles.clear();

    DELETE_AND_FREE_MAP( m_3dModelMap )

    m_3dModelMatrixMap.clear();

    DELETE_AND_FREE( m_board )
    DELETE_AND_FREE( m_boardWithHoles )
    DELETE_AND_FREE( m_postMachinePlugs )
    DELETE_AND_FREE( m_antiBoard )

    DELETE_AND_FREE( m_outerThroughHoles )
    DELETE_AND_FREE( m_outerViaThroughHoles )
    DELETE_AND_FREE( m_outerThroughHoleRings )

    DELETE_AND_FREE( m_microviaHoles )
    DELETE_AND_FREE( m_padHoles )
    DELETE_AND_FREE( m_viaFrontCover )
    DELETE_AND_FREE( m_viaBackCover )
}


void RENDER_3D_OPENGL::renderSolderMaskLayer( PCB_LAYER_ID aLayerID, float aZPos,
                                              bool aShowThickness, bool aSkipRenderHoles )
{
    wxASSERT( (aLayerID == B_Mask) || (aLayerID == F_Mask) );

    if( m_board )
    {
        OPENGL_RENDER_LIST* solder_mask = m_layers[ aLayerID ];
        OPENGL_RENDER_LIST* via_holes = aSkipRenderHoles ? nullptr : m_outerThroughHoles;

        if( via_holes )
            via_holes->ApplyScalePosition( aZPos, m_boardAdapter.GetNonCopperLayerThickness() );

        m_board->ApplyScalePosition( aZPos, m_boardAdapter.GetNonCopperLayerThickness() );

        setLayerMaterial( aLayerID );
        m_board->SetItIsTransparent( true );
        m_board->DrawCulled( aShowThickness, solder_mask, via_holes );

        if( aLayerID == F_Mask && m_viaFrontCover )
        {
            m_viaFrontCover->ApplyScalePosition( aZPos, 4 * m_boardAdapter.GetNonCopperLayerThickness() );
            m_viaFrontCover->DrawTop();
        }
        else if( aLayerID == B_Mask && m_viaBackCover )
        {
            m_viaBackCover->ApplyScalePosition( aZPos, 4 * m_boardAdapter.GetNonCopperLayerThickness() );
            m_viaBackCover->DrawBot();
        }
    }
}


void RENDER_3D_OPENGL::get3dModelsSelected( std::list<MODELTORENDER> &aDstRenderList, bool aGetTop,
                                            bool aGetBot, bool aRenderTransparentOnly,
                                            bool aRenderSelectedOnly )
{
    wxASSERT( ( aGetTop == true ) || ( aGetBot == true ) );

    if( !m_boardAdapter.GetBoard() )
        return;

    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& cfg = m_boardAdapter.m_Cfg->m_Render;
    const wxString currentVariant = m_boardAdapter.GetBoard()->GetCurrentVariant();

    // Go for all footprints
    for( FOOTPRINT* fp : m_boardAdapter.GetBoard()->Footprints() )
    {
        bool highlight = false;

        if( m_boardAdapter.m_IsBoardView )
        {
            if( fp->IsSelected() )
                highlight = true;

            if( cfg.highlight_on_rollover && fp == m_currentRollOverItem )
                highlight = true;

            if( aRenderSelectedOnly != highlight )
                continue;
        }

        if( !fp->Models().empty() )
        {
            if( m_boardAdapter.IsFootprintShown( fp ) )
            {
                // Skip 3D models for footprints that are DNP in the current variant
                if( fp->GetDNPForVariant( currentVariant ) )
                    continue;

                const bool isFlipped = fp->IsFlipped();

                if( aGetTop == !isFlipped || aGetBot == isFlipped )
                    get3dModelsFromFootprint( aDstRenderList, fp, aRenderTransparentOnly,
                                              highlight );
            }
        }
    }
}


void RENDER_3D_OPENGL::get3dModelsFromFootprint( std::list<MODELTORENDER> &aDstRenderList,
                                                 const FOOTPRINT* aFootprint,
                                                 bool aRenderTransparentOnly, bool aIsSelected )
{
    if( !aFootprint->Models().empty() )
    {
        const double zpos = m_boardAdapter.GetFootprintZPos( aFootprint->IsFlipped() );

        VECTOR2I pos = aFootprint->GetPosition();

        glm::mat4 fpMatrix( 1.0f );

        fpMatrix = glm::translate( fpMatrix, SFVEC3F( pos.x * m_boardAdapter.BiuTo3dUnits(),
                                                      -pos.y * m_boardAdapter.BiuTo3dUnits(),
                                                      zpos ) );

        if( !aFootprint->GetOrientation().IsZero() )
        {
            fpMatrix = glm::rotate( fpMatrix, (float) aFootprint->GetOrientation().AsRadians(),
                                    SFVEC3F( 0.0f, 0.0f, 1.0f ) );
        }

        if( aFootprint->IsFlipped() )
        {
            fpMatrix = glm::rotate( fpMatrix, glm::pi<float>(), SFVEC3F( 0.0f, 1.0f, 0.0f ) );
            fpMatrix = glm::rotate( fpMatrix, glm::pi<float>(), SFVEC3F( 0.0f, 0.0f, 1.0f ) );
        }

        double modelunit_to_3d_units_factor = m_boardAdapter.BiuTo3dUnits() * UNITS3D_TO_UNITSPCB;

        fpMatrix = glm::scale( fpMatrix, SFVEC3F( modelunit_to_3d_units_factor ) );

        // Get the list of model files for this model
        for( const FP_3DMODEL& sM : aFootprint->Models() )
        {
            if( !sM.m_Show || sM.m_Filename.empty() )
                continue;

            // Check if the model is present in our cache map
            auto cache_i = m_3dModelMap.find( sM.m_Filename );

            if( cache_i == m_3dModelMap.end() )
                continue;

            if( const MODEL_3D* modelPtr = cache_i->second )
            {
                bool opaque = sM.m_Opacity >= 1.0;

                if( ( !aRenderTransparentOnly && modelPtr->HasOpaqueMeshes() && opaque ) ||
                    ( aRenderTransparentOnly && ( modelPtr->HasTransparentMeshes() || !opaque ) ) )
                {
                    glm::mat4 modelworldMatrix = fpMatrix;

                    const SFVEC3F offset = SFVEC3F( sM.m_Offset.x, sM.m_Offset.y, sM.m_Offset.z );
                    const SFVEC3F rotation = SFVEC3F( sM.m_Rotation.x, sM.m_Rotation.y,
                                                      sM.m_Rotation.z );
                    const SFVEC3F scale = SFVEC3F( sM.m_Scale.x, sM.m_Scale.y, sM.m_Scale.z );

                    std::vector<float> key = { offset.x, offset.y, offset.z,
                                               rotation.x, rotation.y, rotation.z,
                                               scale.x, scale.y, scale.z };

                    auto it = m_3dModelMatrixMap.find( key );

                    if( it != m_3dModelMatrixMap.end() )
                    {
                        modelworldMatrix *= it->second;
                    }
                    else
                    {
                        glm::mat4 mtx( 1.0f );
                        mtx = glm::translate( mtx, offset );
                        mtx = glm::rotate( mtx, glm::radians( -rotation.z ), { 0.0f, 0.0f, 1.0f } );
                        mtx = glm::rotate( mtx, glm::radians( -rotation.y ), { 0.0f, 1.0f, 0.0f } );
                        mtx = glm::rotate( mtx, glm::radians( -rotation.x ), { 1.0f, 0.0f, 0.0f } );
                        mtx = glm::scale( mtx, scale );
                        m_3dModelMatrixMap[ key ] = mtx;

                        modelworldMatrix *= mtx;
                    }

                    aDstRenderList.emplace_back( modelworldMatrix, modelPtr,
                                                 aRenderTransparentOnly ? sM.m_Opacity : 1.0f,
                                                 aRenderTransparentOnly,
                                                 aFootprint->IsSelected() || aIsSelected );
                }
            }
        }
    }
}


void RENDER_3D_OPENGL::renderOpaqueModels( const glm::mat4 &aCameraViewMatrix )
{
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& cfg = m_boardAdapter.m_Cfg->m_Render;

    const SFVEC3F selColor = m_boardAdapter.GetColor( cfg.opengl_selection_color );

    glPushMatrix();

    std::list<MODELTORENDER> renderList;

    if( m_boardAdapter.m_IsBoardView )
    {
        renderList.clear();

        get3dModelsSelected( renderList, true, true, false, true );

        if( !renderList.empty() )
        {
            MODEL_3D::BeginDrawMulti( false );

            for( const MODELTORENDER& mtr : renderList )
                renderModel( aCameraViewMatrix, mtr, selColor, nullptr );

            MODEL_3D::EndDrawMulti();
        }
    }

    renderList.clear();
    get3dModelsSelected( renderList, true, true, false, false );

    if( !renderList.empty() )
    {
        MODEL_3D::BeginDrawMulti( true );

        for( const MODELTORENDER& mtr : renderList )
            renderModel( aCameraViewMatrix, mtr, selColor, nullptr );

        MODEL_3D::EndDrawMulti();
    }

    glPopMatrix();
}


void RENDER_3D_OPENGL::renderTransparentModels( const glm::mat4 &aCameraViewMatrix )
{
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& cfg = m_boardAdapter.m_Cfg->m_Render;

    const SFVEC3F selColor = m_boardAdapter.GetColor( cfg.opengl_selection_color );

    std::list<MODELTORENDER> renderListModels; // do not clear it until this function returns

    if( m_boardAdapter.m_IsBoardView )
    {
        // Get Transparent Selected
        get3dModelsSelected( renderListModels, true, true, true, true );
    }

    // Get Transparent Not Selected
    get3dModelsSelected( renderListModels, true, true, true, false );

    if( renderListModels.empty() )
        return;

    std::vector<std::pair<const MODELTORENDER *, float>> transparentModelList;

    transparentModelList.reserve( renderListModels.size() );

    // Calculate the distance to the camera for each model
    const SFVEC3F &cameraPos = m_camera.GetPos();

    for( const MODELTORENDER& mtr : renderListModels )
    {
        const BBOX_3D& bBox = mtr.m_model->GetBBox();
        const SFVEC3F& bBoxCenter = bBox.GetCenter();
        const SFVEC3F bBoxWorld = mtr.m_modelWorldMat * glm::vec4( bBoxCenter, 1.0f );

        const float distanceToCamera = glm::length( cameraPos - bBoxWorld );

        transparentModelList.emplace_back( &mtr, distanceToCamera );
    }

    // Sort from back to front
    std::sort( transparentModelList.begin(), transparentModelList.end(),
            [&]( std::pair<const MODELTORENDER *, float>& a,
                 std::pair<const MODELTORENDER *, float>& b )
            {
                if( a.second != b.second )
                    return a.second > b.second;

                return a.first > b.first;   // use pointers as a last resort
            } );

    // Start rendering calls
    glPushMatrix();

    bool isUsingColorInformation = !( transparentModelList.begin()->first->m_isSelected &&
                                      m_boardAdapter.m_IsBoardView );

    MODEL_3D::BeginDrawMulti( isUsingColorInformation );

    for( const std::pair<const MODELTORENDER *, float>& mtr : transparentModelList )
    {
        if( m_boardAdapter.m_IsBoardView )
        {
            // Toggle between using model color or the select color
            if( !isUsingColorInformation && !mtr.first->m_isSelected )
            {
                isUsingColorInformation = true;

                glEnableClientState( GL_COLOR_ARRAY );
                glEnableClientState( GL_TEXTURE_COORD_ARRAY );
                glEnable( GL_COLOR_MATERIAL );
            }
            else if( isUsingColorInformation && mtr.first->m_isSelected )
            {
                isUsingColorInformation = false;

                glDisableClientState( GL_COLOR_ARRAY );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                glDisable( GL_COLOR_MATERIAL );
            }
        }

        // Render model, sort each individuall material group
        // by passing cameraPos
        renderModel( aCameraViewMatrix, *mtr.first, selColor, &cameraPos );
    }

    MODEL_3D::EndDrawMulti();

    glPopMatrix();
}


void RENDER_3D_OPENGL::renderModel( const glm::mat4 &aCameraViewMatrix,
                                    const MODELTORENDER &aModelToRender,
                                    const SFVEC3F &aSelColor, const SFVEC3F *aCameraWorldPos )
{
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& cfg = m_boardAdapter.m_Cfg->m_Render;

    const glm::mat4 modelviewMatrix = aCameraViewMatrix * aModelToRender.m_modelWorldMat;

    glLoadMatrixf( glm::value_ptr( modelviewMatrix ) );

    aModelToRender.m_model->Draw( aModelToRender.m_isTransparent, aModelToRender.m_opacity,
                                  aModelToRender.m_isSelected, aSelColor,
                                  &aModelToRender.m_modelWorldMat, aCameraWorldPos );

    if( cfg.show_model_bbox )
    {
        const bool wasBlendEnabled = glIsEnabled( GL_BLEND );

        if( !wasBlendEnabled )
        {
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }

        glDisable( GL_LIGHTING );

        glLineWidth( 1 );
        aModelToRender.m_model->DrawBboxes();

        glLineWidth( 4 );
        aModelToRender.m_model->DrawBbox();

        glEnable( GL_LIGHTING );

        if( !wasBlendEnabled )
            glDisable( GL_BLEND );
    }
}


void RENDER_3D_OPENGL::generate3dGrid( GRID3D_TYPE aGridType )
{
    if( glIsList( m_grid ) )
        glDeleteLists( m_grid, 1 );

    m_grid = 0;

    if( aGridType == GRID3D_TYPE::NONE )
        return;

    m_grid = glGenLists( 1 );

    if( !glIsList( m_grid ) )
        return;

    glNewList( m_grid, GL_COMPILE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    const double zpos = 0.0;

    // Color of grid lines
    const SFVEC3F gridColor = m_boardAdapter.GetColor( DARKGRAY );

    // Color of grid lines every 5 lines
    const SFVEC3F gridColor_marker = m_boardAdapter.GetColor( LIGHTBLUE );
    const double  scale            = m_boardAdapter.BiuTo3dUnits();
    const GLfloat transparency     = 0.35f;

    double griSizeMM = 0.0;

    switch( aGridType )
    {
    case GRID3D_TYPE::GRID_1MM:   griSizeMM = 1.0;  break;
    case GRID3D_TYPE::GRID_2P5MM: griSizeMM = 2.5;  break;
    case GRID3D_TYPE::GRID_5MM:   griSizeMM = 5.0;  break;
    case GRID3D_TYPE::GRID_10MM:  griSizeMM = 10.0; break;

    default:
    case GRID3D_TYPE::NONE:       return;
    }

    glNormal3f( 0.0, 0.0, 1.0 );

    const VECTOR2I brd_size = m_boardAdapter.GetBoardSize();
    VECTOR2I       brd_center_pos = m_boardAdapter.GetBoardPos();

    brd_center_pos.y = -brd_center_pos.y;

    const int xsize = std::max( brd_size.x, pcbIUScale.mmToIU( 100 ) ) * 1.2;
    const int ysize = std::max( brd_size.y, pcbIUScale.mmToIU( 100 ) ) * 1.2;

    // Grid limits, in 3D units
    double  xmin = ( brd_center_pos.x - xsize / 2 ) * scale;
    double  xmax = ( brd_center_pos.x + xsize / 2 ) * scale;
    double  ymin = ( brd_center_pos.y - ysize / 2 ) * scale;
    double  ymax = ( brd_center_pos.y + ysize / 2 ) * scale;
    double  zmin = pcbIUScale.mmToIU( -50 ) * scale;
    double  zmax = pcbIUScale.mmToIU( 100 ) * scale;

    // Set rasterised line width (min value = 1)
    glLineWidth( 1 );

    // Draw horizontal grid centered on 3D origin (center of the board)
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            glColor4f( gridColor.r, gridColor.g, gridColor.b, transparency );
        else
            glColor4f( gridColor_marker.r, gridColor_marker.g, gridColor_marker.b,
                       transparency );

        const int delta = KiROUND( ii * griSizeMM * pcbIUScale.IU_PER_MM );

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
            glVertex3f( xmin, -( brd_center_pos.y + delta ) * scale, zpos );
            glVertex3f( xmax, -( brd_center_pos.y + delta ) * scale, zpos );
            glEnd();

            if( ii != 0 )
            {
                glBegin( GL_LINES );
                glVertex3f( xmin, -( brd_center_pos.y - delta ) * scale, zpos );
                glVertex3f( xmax, -( brd_center_pos.y - delta ) * scale, zpos );
                glEnd();
            }
        }

        if( ( delta > ysize / 2 ) && ( delta > xsize / 2 ) )
            break;
    }

    // Draw vertical grid on Z axis
    glNormal3f( 0.0, -1.0, 0.0 );

    // Draw vertical grid lines (parallel to Z axis)
    double posy = -brd_center_pos.y * scale;

    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            glColor4f( gridColor.r, gridColor.g, gridColor.b, transparency );
        else
            glColor4f( gridColor_marker.r, gridColor_marker.g, gridColor_marker.b,
                       transparency );

        const double delta = ii * griSizeMM * pcbIUScale.IU_PER_MM;

        glBegin( GL_LINES );
        xmax = ( brd_center_pos.x + delta ) * scale;

        glVertex3f( xmax, posy, zmin );
        glVertex3f( xmax, posy, zmax );
        glEnd();

        if( ii != 0 )
        {
            glBegin( GL_LINES );
            xmin = ( brd_center_pos.x - delta ) * scale;
            glVertex3f( xmin, posy, zmin );
            glVertex3f( xmin, posy, zmax );
            glEnd();
        }

        if( delta > xsize / 2.0f )
            break;
    }

    // Draw horizontal grid lines on Z axis (parallel to X axis)
    for( int ii = 0; ; ii++ )
    {
        if( ii % 5 )
            glColor4f( gridColor.r, gridColor.g, gridColor.b, transparency );
        else
            glColor4f( gridColor_marker.r, gridColor_marker.g, gridColor_marker.b, transparency );

        const double delta = ii * griSizeMM * pcbIUScale.IU_PER_MM * scale;

        if( delta <= zmax )
        {
            // Draw grid lines on Z axis (positive Z axis coordinates)
            glBegin( GL_LINES );
            glVertex3f( xmin, posy, delta );
            glVertex3f( xmax, posy, delta );
            glEnd();
        }

        if( delta <= -zmin && ( ii != 0 ) )
        {
            // Draw grid lines on Z axis (negative Z axis coordinates)
            glBegin( GL_LINES );
            glVertex3f( xmin, posy, -delta );
            glVertex3f( xmax, posy, -delta );
            glEnd();
        }

        if( ( delta > zmax ) && ( delta > -zmin ) )
            break;
    }

    glDisable( GL_BLEND );

    glEndList();
}
