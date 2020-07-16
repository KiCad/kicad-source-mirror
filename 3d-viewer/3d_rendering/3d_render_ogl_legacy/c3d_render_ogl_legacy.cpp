/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

// Apple, in their infinite wisdom, has decided to mark OpenGL as deprecated.
// Luckily we can silence warnings about its deprecation. This is needed on the GLEW
// includes since they seem to transitively include the OpenGL headers.
#define GL_SILENCE_DEPRECATION 1
#include <GL/glew.h>

#include "c3d_render_ogl_legacy.h"
#include "ogl_legacy_utils.h"
#include "common_ogl/ogl_utils.h"
#include "../cimage.h"
#include <class_board.h>
#include <class_module.h>
#include <3d_math.h>
#include <math/util.h>      // for KiROUND

#include <base_units.h>

/**
  * Scale convertion from 3d model units to pcb units
  */
#define UNITS3D_TO_UNITSPCB (IU_PER_MM)

C3D_RENDER_OGL_LEGACY::C3D_RENDER_OGL_LEGACY( BOARD_ADAPTER& aAdapter, CCAMERA& aCamera ) :
        C3D_RENDER_BASE( aAdapter, aCamera )
{
    wxLogTrace( m_logTrace, wxT( "C3D_RENDER_OGL_LEGACY::C3D_RENDER_OGL_LEGACY" ) );

    m_ogl_disp_lists_layers.clear();
    m_ogl_disp_lists_layers_holes_outer.clear();
    m_ogl_disp_lists_layers_holes_inner.clear();
    m_triangles.clear();
    m_ogl_disp_list_board = NULL;

    m_ogl_disp_list_through_holes_outer_with_npth = NULL;
    m_ogl_disp_list_through_holes_outer = NULL;
    m_ogl_disp_list_through_holes_vias_outer = NULL;
    m_ogl_disp_list_through_holes_vias_outer_ring = NULL;
    //m_ogl_disp_list_through_holes_vias_inner = NULL;
    m_ogl_disp_list_via = NULL;
    m_ogl_disp_list_pads_holes = NULL;
    m_ogl_disp_list_vias_and_pad_holes_outer_contourn_and_caps = NULL;

    m_ogl_circle_texture = 0;
    m_ogl_disp_list_grid = 0;
    m_last_grid_type     = GRID3D_TYPE::NONE;

    m_3dmodel_map.clear();
}


C3D_RENDER_OGL_LEGACY::~C3D_RENDER_OGL_LEGACY()
{
    wxLogTrace( m_logTrace, wxT( "C3D_RENDER_OGL_LEGACY::~C3D_RENDER_OGL_LEGACY" ) );

    ogl_free_all_display_lists();

    glDeleteTextures( 1, &m_ogl_circle_texture );
}


int C3D_RENDER_OGL_LEGACY::GetWaitForEditingTimeOut()
{
    return 50; // ms
}


void C3D_RENDER_OGL_LEGACY::SetCurWindowSize( const wxSize &aSize )
{
    if( m_windowSize != aSize )
    {
        m_windowSize = aSize;
        glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

        // Initialize here any screen dependent data here
    }
}


void C3D_RENDER_OGL_LEGACY::setLight_Front( bool enabled )
{
    if( enabled )
        glEnable( GL_LIGHT0 );
    else
        glDisable( GL_LIGHT0 );
}


void C3D_RENDER_OGL_LEGACY::setLight_Top( bool enabled )
{
    if( enabled )
        glEnable( GL_LIGHT1 );
    else
        glDisable( GL_LIGHT1 );
}


void C3D_RENDER_OGL_LEGACY::setLight_Bottom( bool enabled )
{
    if( enabled )
        glEnable( GL_LIGHT2 );
    else
        glDisable( GL_LIGHT2 );
}


void C3D_RENDER_OGL_LEGACY::render_3D_arrows()
{
    const float arrow_size = RANGE_SCALE_3D * 0.30f;

    glDisable( GL_CULL_FACE );

    // YxY squared view port, this is on propose
    glViewport( 4, 4, m_windowSize.y / 8 , m_windowSize.y / 8 );
    glClear( GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 45.0f, 1.0f, 0.001f, RANGE_SCALE_3D );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    const glm::mat4 TranslationMatrix = glm::translate(
                glm::mat4(1.0f),
                SFVEC3F( 0.0f, 0.0f, -(arrow_size * 2.75f) ) );

    const glm::mat4 ViewMatrix = TranslationMatrix * m_camera.GetRotationMatrix();

    glLoadMatrixf( glm::value_ptr( ViewMatrix ) );

    ogl_set_arrow_material();

    glColor3f( 0.9f, 0.0f, 0.0f );
    OGL_draw_arrow( SFVEC3F( 0.0f, 0.0f, 0.0f ),
                    SFVEC3F( arrow_size, 0.0f, 0.0f ),
                    0.275f );

    glColor3f( 0.0f, 0.9f, 0.0f );
    OGL_draw_arrow( SFVEC3F( 0.0f, 0.0f, 0.0f ),
                    SFVEC3F( 0.0f, arrow_size, 0.0f ),
                    0.275f );

    glColor3f( 0.0f, 0.0f, 0.9f );
    OGL_draw_arrow( SFVEC3F( 0.0f, 0.0f, 0.0f ),
                    SFVEC3F( 0.0f, 0.0f, arrow_size ),
                    0.275f );

    glEnable( GL_CULL_FACE );
}


void C3D_RENDER_OGL_LEGACY::setupMaterials()
{
    m_materials = {};

    if( m_boardAdapter.GetFlag( FL_USE_REALISTIC_MODE ) )
    {
        // http://devernay.free.fr/cours/opengl/materials.html

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


        // Solder mask material mixed with solder mask color
        m_materials.m_SolderMaskTop.m_Ambient = SFVEC3F(
                m_boardAdapter.m_SolderMaskColorTop.r * 0.3f,
                m_boardAdapter.m_SolderMaskColorTop.g * 0.3f,
                m_boardAdapter.m_SolderMaskColorTop.b * 0.3f );

        m_materials.m_SolderMaskTop.m_Specular = SFVEC3F(
                m_boardAdapter.m_SolderMaskColorTop.r * m_boardAdapter.m_SolderMaskColorTop.r,
                m_boardAdapter.m_SolderMaskColorTop.g * m_boardAdapter.m_SolderMaskColorTop.g,
                m_boardAdapter.m_SolderMaskColorTop.b * m_boardAdapter.m_SolderMaskColorTop.b );

        m_materials.m_SolderMaskTop.m_Shininess    = 0.8f * 128.0f;
        m_materials.m_SolderMaskTop.m_Transparency = 0.17f;
        m_materials.m_SolderMaskTop.m_Emissive     = SFVEC3F( 0.0f, 0.0f, 0.0f );

        // Solder mask material mixed with solder mask color
        m_materials.m_SolderMaskBot.m_Ambient = SFVEC3F(
                m_boardAdapter.m_SolderMaskColorBot.r * 0.3f,
                m_boardAdapter.m_SolderMaskColorBot.g * 0.3f,
                m_boardAdapter.m_SolderMaskColorBot.b * 0.3f );

        m_materials.m_SolderMaskBot.m_Specular = SFVEC3F(
                m_boardAdapter.m_SolderMaskColorBot.r * m_boardAdapter.m_SolderMaskColorBot.r,
                m_boardAdapter.m_SolderMaskColorBot.g * m_boardAdapter.m_SolderMaskColorBot.g,
                m_boardAdapter.m_SolderMaskColorBot.b * m_boardAdapter.m_SolderMaskColorBot.b );

        m_materials.m_SolderMaskBot.m_Shininess    = 0.8f * 128.0f;
        m_materials.m_SolderMaskBot.m_Transparency = 0.17f;
        m_materials.m_SolderMaskBot.m_Emissive     = SFVEC3F( 0.0f, 0.0f, 0.0f );


        // Epoxy material
        m_materials.m_EpoxyBoard.m_Ambient   = SFVEC3F( 117.0f / 255.0f,
                                                         97.0f / 255.0f,
                                                         47.0f / 255.0f );

        m_materials.m_EpoxyBoard.m_Diffuse   = m_boardAdapter.m_BoardBodyColor;

        m_materials.m_EpoxyBoard.m_Specular  = SFVEC3F( 18.0f / 255.0f,
                                                         3.0f / 255.0f,
                                                        20.0f / 255.0f );

        m_materials.m_EpoxyBoard.m_Shininess = 0.1f * 128.0f;
        m_materials.m_EpoxyBoard.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );
    }
    else    // Technical Mode
    {
        const SFVEC3F matAmbientColor  = SFVEC3F( 0.10f );
        const SFVEC3F matSpecularColor = SFVEC3F( 0.10f );
        const float matShininess = 0.1f * 128.0f;

        // Copper material
        m_materials.m_Copper.m_Ambient   = matAmbientColor;
        m_materials.m_Copper.m_Specular  = matSpecularColor;
        m_materials.m_Copper.m_Shininess = matShininess;
        m_materials.m_Copper.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );

        // Paste material
        m_materials.m_Paste.m_Ambient   = matAmbientColor;
        m_materials.m_Paste.m_Specular  = matSpecularColor;
        m_materials.m_Paste.m_Shininess = matShininess;
        m_materials.m_Paste.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );

        // Silk screen material
        m_materials.m_SilkSTop.m_Ambient   = matAmbientColor;
        m_materials.m_SilkSTop.m_Specular  = matSpecularColor;
        m_materials.m_SilkSTop.m_Shininess = matShininess;
        m_materials.m_SilkSTop.m_Emissive  = SFVEC3F( 0.0f, 0.0f, 0.0f );

        // Silk screen material
        m_materials.m_SilkSBot.m_Ambient   = matAmbientColor;
        m_materials.m_SilkSBot.m_Specular  = matSpecularColor;
        m_materials.m_SilkSBot.m_Shininess = matShininess;
        m_materials.m_SilkSBot.m_Emissive  = SFVEC3F( 0.0f, 0.0f, 0.0f );

        // Solder mask material
        m_materials.m_SolderMaskTop.m_Ambient      = matAmbientColor;
        m_materials.m_SolderMaskTop.m_Specular     = matSpecularColor;
        m_materials.m_SolderMaskTop.m_Shininess    = matShininess;
        m_materials.m_SolderMaskTop.m_Transparency = 0.17f;
        m_materials.m_SolderMaskTop.m_Emissive     = SFVEC3F( 0.0f, 0.0f, 0.0f );

        // Solder mask material
        m_materials.m_SolderMaskBot.m_Ambient      = matAmbientColor;
        m_materials.m_SolderMaskBot.m_Specular     = matSpecularColor;
        m_materials.m_SolderMaskBot.m_Shininess    = matShininess;
        m_materials.m_SolderMaskBot.m_Transparency = 0.17f;
        m_materials.m_SolderMaskBot.m_Emissive     = SFVEC3F( 0.0f, 0.0f, 0.0f );

        // Epoxy material
        m_materials.m_EpoxyBoard.m_Ambient   = matAmbientColor;
        m_materials.m_EpoxyBoard.m_Diffuse   = m_boardAdapter.m_BoardBodyColor;
        m_materials.m_EpoxyBoard.m_Specular  = matSpecularColor;
        m_materials.m_EpoxyBoard.m_Shininess = matShininess;
        m_materials.m_EpoxyBoard.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );

        // Gray material (used for example in technical vias and pad holes)
        m_materials.m_GrayMaterial.m_Ambient    = SFVEC3F( 0.8f, 0.8f, 0.8f );
        m_materials.m_GrayMaterial.m_Diffuse    = SFVEC3F( 0.3f, 0.3f, 0.3f );
        m_materials.m_GrayMaterial.m_Specular   = SFVEC3F( 0.4f, 0.4f, 0.4f );
        m_materials.m_GrayMaterial.m_Shininess  = 0.01f * 128.0f;
        m_materials.m_GrayMaterial.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );
    }
}


void C3D_RENDER_OGL_LEGACY::set_layer_material( PCB_LAYER_ID aLayerID )
{
    switch( aLayerID )
    {
        case B_Mask:
            m_materials.m_SolderMaskBot.m_Diffuse = get_layer_color( aLayerID );
            OGL_SetMaterial( m_materials.m_SolderMaskBot, 1.0f );
            break;
        case F_Mask:
            m_materials.m_SolderMaskTop.m_Diffuse = get_layer_color( aLayerID );
            OGL_SetMaterial( m_materials.m_SolderMaskTop, 1.0f );
            break;

        case B_Paste:
        case F_Paste:
            m_materials.m_Paste.m_Diffuse = get_layer_color( aLayerID );
            OGL_SetMaterial( m_materials.m_Paste, 1.0f );
            break;

        case B_SilkS:
            m_materials.m_SilkSBot.m_Diffuse = get_layer_color( aLayerID );
            OGL_SetMaterial( m_materials.m_SilkSBot, 1.0f );
            break;

        case F_SilkS:
            m_materials.m_SilkSTop.m_Diffuse = get_layer_color( aLayerID );
            OGL_SetMaterial( m_materials.m_SilkSTop, 1.0f );
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
            m_materials.m_Plastic.m_Diffuse = get_layer_color( aLayerID );
            m_materials.m_Plastic.m_Ambient = SFVEC3F(
                                    m_materials.m_Plastic.m_Diffuse.r * 0.05f,
                                    m_materials.m_Plastic.m_Diffuse.g * 0.05f,
                                    m_materials.m_Plastic.m_Diffuse.b * 0.05f );

            m_materials.m_Plastic.m_Specular = SFVEC3F(
                                    m_materials.m_Plastic.m_Diffuse.r * 0.7f,
                                    m_materials.m_Plastic.m_Diffuse.g * 0.7f,
                                    m_materials.m_Plastic.m_Diffuse.b * 0.7f );

            m_materials.m_Plastic.m_Shininess = 0.078125f * 128.0f;
            m_materials.m_Plastic.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );
            OGL_SetMaterial( m_materials.m_Plastic, 1.0f );
        break;

        default:
            m_materials.m_Copper.m_Diffuse = get_layer_color( aLayerID );
            OGL_SetMaterial( m_materials.m_Copper, 1.0f );

        break;
    }
}


SFVEC3F C3D_RENDER_OGL_LEGACY::get_layer_color( PCB_LAYER_ID aLayerID )
{
    SFVEC3F layerColor = m_boardAdapter.GetLayerColor( aLayerID );

    if( m_boardAdapter.GetFlag( FL_USE_REALISTIC_MODE ) )
    {
        switch( aLayerID )
        {
            case B_Adhes:
            case F_Adhes:
                break;

            case B_Mask:
                layerColor = m_boardAdapter.m_SolderMaskColorBot;
                break;
            case F_Mask:
                layerColor = m_boardAdapter.m_SolderMaskColorTop;
                break;

            case B_Paste:
            case F_Paste:
                layerColor = m_boardAdapter.m_SolderPasteColor;
                break;

            case B_SilkS:
                layerColor = m_boardAdapter.m_SilkScreenColorBot;
                break;
            case F_SilkS:
                layerColor = m_boardAdapter.m_SilkScreenColorTop;
                break;

            case Dwgs_User:
            case Cmts_User:
            case Eco1_User:
            case Eco2_User:
            case Edge_Cuts:
            case Margin:
                break;

            case B_CrtYd:
            case F_CrtYd:
                break;

            case B_Fab:
            case F_Fab:
                break;

            default:
                layerColor = m_boardAdapter.m_CopperColor;
                break;
        }
    }

    return layerColor;
}

void init_lights(void)
{
    // Setup light
    // https://www.opengl.org/sdk/docs/man2/xhtml/glLight.xml
    // /////////////////////////////////////////////////////////////////////////
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


bool C3D_RENDER_OGL_LEGACY::Redraw(
        bool aIsMoving, REPORTER* aStatusReporter, REPORTER* aWarningReporter )
{
    // Initialize openGL
    if( !m_is_opengl_initialized )
    {
        if( !initializeOpenGL() )
            return false;
    }

    if( m_reloadRequested )
    {
        std::unique_ptr<BUSY_INDICATOR> busy = CreateBusyIndicator();

        if( aStatusReporter )
            aStatusReporter->Report( _( "Loading..." ) );

        reload( aStatusReporter, aWarningReporter );
        setupMaterials();

        // generate a new 3D grid as the size of the board may had changed
        m_last_grid_type = m_boardAdapter.GridGet();
        generate_new_3DGrid( m_last_grid_type );
    }
    else
    {
        // Check if grid was changed
        if( m_boardAdapter.GridGet() != m_last_grid_type )
        {
            // and generate a new one
            m_last_grid_type = m_boardAdapter.GridGet();
            generate_new_3DGrid( m_last_grid_type );
        }
    }

    // Initial setup
    // /////////////////////////////////////////////////////////////////////////
    glDepthFunc( GL_LESS );
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CCW );    // This is the openGL default
    glEnable( GL_NORMALIZE ); // This allow openGL to normalize the normals after transformations

    glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

    if( m_boardAdapter.GetFlag( FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE ) )
    {
        if( !aIsMoving )
        {
            glEnable( GL_MULTISAMPLE );
        }
        else
        {
            glDisable( GL_MULTISAMPLE );
        }
    }

    // clear color and depth buffers
    // /////////////////////////////////////////////////////////////////////////
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClearDepth( 1.0f );
    glClearStencil( 0x00 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    OGL_ResetTextureStateDefaults();

    // Draw the background ( rectangle with color gradient)
    // /////////////////////////////////////////////////////////////////////////
    OGL_DrawBackground( SFVEC3F( m_boardAdapter.m_BgColorTop ),
                        SFVEC3F( m_boardAdapter.m_BgColorBot ) );

    glEnable( GL_DEPTH_TEST );


    // Set projection and modelview matrixes
    // /////////////////////////////////////////////////////////////////////////
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( glm::value_ptr( m_camera.GetProjectionMatrix() ) );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glLoadMatrixf( glm::value_ptr( m_camera.GetViewMatrix() ) );


    // Position the headlight
    // /////////////////////////////////////////////////////////////////////////

    setLight_Front( true );
    setLight_Top( true );
    setLight_Bottom( true );

    glEnable( GL_LIGHTING );

    {
        const SFVEC3F &cameraPos = m_camera.GetPos();

        // Place the light at a minimun Z so the diffuse factor will not drop
        // and the board will still look with good light.
        float zpos;

        if( cameraPos.z > 0.0f )
        {
            zpos = glm::max( cameraPos.z, 0.5f ) + cameraPos.z * cameraPos.z;
        }
        else
        {
            zpos = glm::min( cameraPos.z,-0.5f ) - cameraPos.z * cameraPos.z;
        }

        const GLfloat headlight_pos[] = { cameraPos.x,
                                          cameraPos.y,
                                          zpos,
                                          1.0f }; // This is a point light

        glLightfv( GL_LIGHT0, GL_POSITION, headlight_pos );
    }

    const bool drawMiddleSegments = !( aIsMoving &&
                                    m_boardAdapter.GetFlag( FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE ) );

    const bool skipRenderHoles = aIsMoving &&
                                 m_boardAdapter.GetFlag( FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE );

    const bool skipRenderVias = aIsMoving &&
                                m_boardAdapter.GetFlag( FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE );

    // Display board body
    // /////////////////////////////////////////////////////////////////////////
    if( m_boardAdapter.GetFlag( FL_SHOW_BOARD_BODY ) )
    {
        if( m_ogl_disp_list_board )
        {
            m_ogl_disp_list_board->ApplyScalePosition( -m_boardAdapter.GetEpoxyThickness3DU() / 2.0f,
                                                       m_boardAdapter.GetEpoxyThickness3DU() );

            OGL_SetMaterial( m_materials.m_EpoxyBoard, 1.0f );

            m_ogl_disp_list_board->SetItIsTransparent( false );

            if( (m_ogl_disp_list_through_holes_outer_with_npth) && (!skipRenderHoles) )
            {
                m_ogl_disp_list_through_holes_outer_with_npth->ApplyScalePosition(
                        -m_boardAdapter.GetEpoxyThickness3DU() / 2.0f,
                        m_boardAdapter.GetEpoxyThickness3DU() );

                m_ogl_disp_list_board->DrawAllCameraCulledSubtractLayer(
                            m_ogl_disp_list_through_holes_outer_with_npth,
                            NULL );
            }
            else
            {
                m_ogl_disp_list_board->DrawAll();
            }
        }
    }


    if( m_boardAdapter.GetFlag( FL_USE_REALISTIC_MODE ) )
    {
        // Draw vias and pad holes with copper material
        set_layer_material( B_Cu );
    }
    else
    {
        OGL_SetMaterial( m_materials.m_GrayMaterial, 1.0f );
    }

    if( (!( skipRenderVias || skipRenderHoles ) ) && m_ogl_disp_list_via )
    {
        m_ogl_disp_list_via->DrawAll();
    }

    if( ( !skipRenderHoles ) && m_ogl_disp_list_pads_holes )
    {
        m_ogl_disp_list_pads_holes->DrawAll();
    }


    // Display copper and tech layers
    // /////////////////////////////////////////////////////////////////////////
    for( MAP_OGL_DISP_LISTS::const_iterator ii = m_ogl_disp_lists_layers.begin();
         ii != m_ogl_disp_lists_layers.end();
         ++ii )
    {

        const PCB_LAYER_ID layer_id = (PCB_LAYER_ID)(ii->first);

        // Mask kayers are not processed here because they are a special case
        if( (layer_id == B_Mask) || (layer_id == F_Mask) )
            continue;

        // Do not show inner layers when it is displaying the board
        if( m_boardAdapter.GetFlag( FL_SHOW_BOARD_BODY ) )
        {
            if( (layer_id > F_Cu) && (layer_id < B_Cu) )
                continue;
        }

        glPushMatrix();

        // !TODO: if we want to increase the separation between layers
        //glScalef( 1.0f, 1.0f, 3.0f );


        CLAYERS_OGL_DISP_LISTS *pLayerDispList = static_cast<CLAYERS_OGL_DISP_LISTS*>(ii->second);
        set_layer_material( layer_id );

        if( (layer_id >= F_Cu) && (layer_id <= B_Cu) )
        {
            if( skipRenderHoles )
            {
                pLayerDispList->DrawAllCameraCulled( m_camera.GetPos().z, drawMiddleSegments );
            }
            else
            {
                if( m_ogl_disp_list_through_holes_outer )
                    m_ogl_disp_list_through_holes_outer->ApplyScalePosition(
                                pLayerDispList->GetZBot(),
                                pLayerDispList->GetZTop() - pLayerDispList->GetZBot() );

                if( m_ogl_disp_lists_layers_holes_outer.find( layer_id ) !=
                    m_ogl_disp_lists_layers_holes_outer.end() )
                {
                    const CLAYERS_OGL_DISP_LISTS* viasHolesLayer =
                            m_ogl_disp_lists_layers_holes_outer.at( layer_id );

                    wxASSERT( viasHolesLayer != NULL );

                    if( viasHolesLayer != NULL )
                    {
                        pLayerDispList->DrawAllCameraCulledSubtractLayer(
                                    m_ogl_disp_list_through_holes_outer,
                                    viasHolesLayer,
                                    drawMiddleSegments );
                    }
                }
                else
                {
                    pLayerDispList->DrawAllCameraCulledSubtractLayer(
                                m_ogl_disp_list_through_holes_outer,
                                NULL,
                                drawMiddleSegments );
                }
            }
        }
        else
        {
            CLAYERS_OGL_DISP_LISTS* dispListThoughHolesOuter =
                    ( m_boardAdapter.GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS )
                            && ( ( layer_id == B_SilkS ) || ( layer_id == F_SilkS ) ) ) ?
                            m_ogl_disp_list_through_holes_vias_outer_ring :
                            m_ogl_disp_list_through_holes_vias_outer;

            if( dispListThoughHolesOuter )
                dispListThoughHolesOuter->ApplyScalePosition( pLayerDispList->GetZBot(),
                        pLayerDispList->GetZTop() - pLayerDispList->GetZBot() );

            if( (!skipRenderHoles) &&
                m_boardAdapter.GetFlag( FL_SUBTRACT_MASK_FROM_SILK ) &&
                ( ( ( layer_id == B_SilkS ) &&
                    ( m_ogl_disp_lists_layers.find( B_Mask ) != m_ogl_disp_lists_layers.end() ) ) ||
                  ( ( layer_id == F_SilkS ) &&
                    ( m_ogl_disp_lists_layers.find( F_Mask ) != m_ogl_disp_lists_layers.end() ) ) ) )
            {
                const PCB_LAYER_ID layerMask_id = (layer_id == B_SilkS)?B_Mask:F_Mask;

                const CLAYERS_OGL_DISP_LISTS *pLayerDispListMask = m_ogl_disp_lists_layers.at( layerMask_id );

                pLayerDispList->DrawAllCameraCulledSubtractLayer(
                            pLayerDispListMask,
                            dispListThoughHolesOuter,
                            drawMiddleSegments );
            }
            else
            {
                if( (!skipRenderHoles) &&
                    dispListThoughHolesOuter &&
                    ( ( layer_id == B_SilkS ) || ( layer_id == F_SilkS )
                      // Remove vias on SolderPaste can be added as an option in future
                      // ( layer_id == B_Paste ) || ( layer_id == F_Paste ) )
                    ) )
                {
                    pLayerDispList->DrawAllCameraCulledSubtractLayer(
                                NULL,
                                dispListThoughHolesOuter,
                                drawMiddleSegments );
                }
                else
                {
                    // Do not render Paste layers when skipRenderHoles is enabled
                    // otherwise it will cause z-fight issues
                    if( !( skipRenderHoles &&
                           ( ( layer_id == B_Paste ) || ( layer_id == F_Paste ) ) ) )
                    {
                        pLayerDispList->DrawAllCameraCulled( m_camera.GetPos().z, drawMiddleSegments );
                    }
                }
            }
        }

        glPopMatrix();
    }


    // Render 3D Models (Non-transparent)
    // /////////////////////////////////////////////////////////////////////////
    render_3D_models( false, false );
    render_3D_models( true, false );


    // Display transparent mask layers
    // /////////////////////////////////////////////////////////////////////////
    if( m_boardAdapter.GetFlag( FL_SOLDERMASK ) )
    {
        //setLight_Top( true );
        //setLight_Bottom( true );

        // add a depth buffer offset, it will help to hide some artifacts
        // on silkscreen where the SolderMask is removed
        glEnable( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( 0.0f, -1.0f );

        if( m_camera.GetPos().z > 0 )
        {
            render_solder_mask_layer( B_Mask, m_boardAdapter.GetLayerTopZpos3DU( B_Mask ),
                                      drawMiddleSegments, skipRenderHoles );

            render_solder_mask_layer( F_Mask, m_boardAdapter.GetLayerBottomZpos3DU( F_Mask ),
                                      drawMiddleSegments, skipRenderHoles );
        }
        else
        {
            render_solder_mask_layer( F_Mask, m_boardAdapter.GetLayerBottomZpos3DU( F_Mask ),
                                      drawMiddleSegments, skipRenderHoles );

            render_solder_mask_layer( B_Mask, m_boardAdapter.GetLayerTopZpos3DU( B_Mask ),
                                      drawMiddleSegments, skipRenderHoles );
        }

        glDisable( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( 0.0f, 0.0f );
    }


    // Render 3D Models (Transparent)
    // /////////////////////////////////////////////////////////////////////////
    // !TODO: this can be optimized. If there are no transparent models (or no opacity),
    // then there is no need to make this function call.

    glDepthMask( GL_FALSE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Enables Texture Env so it can combine model transparency with
    // each module opacity setting
    glEnable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE0 );

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
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_CONSTANT );

    render_3D_models( false, true );
    render_3D_models( true, true );

    glDisable( GL_BLEND );
    OGL_ResetTextureStateDefaults();

    glDepthMask( GL_TRUE );

    // Render Grid
    // /////////////////////////////////////////////////////////////////////////

    if( m_boardAdapter.GridGet() != GRID3D_TYPE::NONE )
    {
        glDisable( GL_LIGHTING );

        if( glIsList( m_ogl_disp_list_grid ) )
            glCallList( m_ogl_disp_list_grid );

        glEnable( GL_LIGHTING );
    }


    // Render 3D arrows
    // /////////////////////////////////////////////////////////////////////////
    if( m_boardAdapter.GetFlag( FL_AXIS ) )
        render_3D_arrows();

    // Return back to the original viewport (this is important if we want
    // to take a screenshot after the render)
    // /////////////////////////////////////////////////////////////////////////
    glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

    return false;
}


bool C3D_RENDER_OGL_LEGACY::initializeOpenGL()
{
    glEnable( GL_LINE_SMOOTH );
    glShadeModel( GL_SMOOTH );

    // 4-byte pixel alignment
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

    // Initialize the open GL texture to draw the filled semi-circle of the segments
    CIMAGE *circleImage = new CIMAGE( SIZE_OF_CIRCLE_TEXTURE, SIZE_OF_CIRCLE_TEXTURE );

    if( !circleImage )
        return false;

    circleImage->CircleFilled( (SIZE_OF_CIRCLE_TEXTURE / 2) - 0,
                               (SIZE_OF_CIRCLE_TEXTURE / 2) - 0,
                               (SIZE_OF_CIRCLE_TEXTURE / 2) - 4,
                               0xFF );

    //circleImage->CircleFilled( (SIZE_OF_CIRCLE_TEXTURE / 4)*1.5f - 1,
    //                           (SIZE_OF_CIRCLE_TEXTURE / 4)*1.5f - 1,
    //                           (SIZE_OF_CIRCLE_TEXTURE / 4)*1.5f - 2, 0xFF );

    CIMAGE *circleImage_Copy = new CIMAGE( *circleImage );

    circleImage->EfxFilter( circleImage_Copy, IMAGE_FILTER::BLUR_3X3 );

    m_ogl_circle_texture = OGL_LoadTexture( *circleImage );

    //circleImage_Copy->SaveAsPNG("circleImage.png");
    delete circleImage_Copy;
    circleImage_Copy = 0;

    //circleImage->SaveAsPNG("circleImage_blured.png");
    delete circleImage;
    circleImage = 0;

    init_lights();

    // Use this mode if you want see the triangle lines (debug proposes)
    //glPolygonMode( GL_FRONT_AND_BACK,  GL_LINE );

    m_is_opengl_initialized = true;

    return true;
}


void C3D_RENDER_OGL_LEGACY::ogl_set_arrow_material()
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


void C3D_RENDER_OGL_LEGACY::ogl_free_all_display_lists()
{
    if( glIsList( m_ogl_disp_list_grid ) )
        glDeleteLists( m_ogl_disp_list_grid, 1 );

    m_ogl_disp_list_grid = 0;

    for( MAP_OGL_DISP_LISTS::const_iterator ii = m_ogl_disp_lists_layers.begin();
         ii != m_ogl_disp_lists_layers.end();
         ++ii )
    {
        CLAYERS_OGL_DISP_LISTS *pLayerDispList = static_cast<CLAYERS_OGL_DISP_LISTS*>(ii->second);
        delete pLayerDispList;
    }

    m_ogl_disp_lists_layers.clear();


    for( MAP_OGL_DISP_LISTS::const_iterator ii = m_ogl_disp_lists_layers_holes_outer.begin();
         ii != m_ogl_disp_lists_layers_holes_outer.end();
         ++ii )
    {
        CLAYERS_OGL_DISP_LISTS *pLayerDispList = static_cast<CLAYERS_OGL_DISP_LISTS*>(ii->second);
        delete pLayerDispList;
    }

    m_ogl_disp_lists_layers_holes_outer.clear();


    for( MAP_OGL_DISP_LISTS::const_iterator ii = m_ogl_disp_lists_layers_holes_inner.begin();
         ii != m_ogl_disp_lists_layers_holes_inner.end();
         ++ii )
    {
        CLAYERS_OGL_DISP_LISTS *pLayerDispList = static_cast<CLAYERS_OGL_DISP_LISTS*>(ii->second);
        delete pLayerDispList;
    }

    m_ogl_disp_lists_layers_holes_inner.clear();

    for( MAP_TRIANGLES::const_iterator ii = m_triangles.begin();
         ii != m_triangles.end();
         ++ii )
    {
        CLAYER_TRIANGLES *pointer = static_cast<CLAYER_TRIANGLES*>(ii->second);
        delete pointer;
    }

    m_triangles.clear();


    for( MAP_3DMODEL::const_iterator ii = m_3dmodel_map.begin();
         ii != m_3dmodel_map.end();
         ++ii )
    {
        C_OGL_3DMODEL *pointer = static_cast<C_OGL_3DMODEL*>(ii->second);
        delete pointer;
    }

    m_3dmodel_map.clear();


    delete m_ogl_disp_list_board;
    m_ogl_disp_list_board = 0;

    delete m_ogl_disp_list_through_holes_outer_with_npth;
    m_ogl_disp_list_through_holes_outer_with_npth = 0;

    delete m_ogl_disp_list_through_holes_outer;
    m_ogl_disp_list_through_holes_outer = 0;

    delete m_ogl_disp_list_through_holes_vias_outer;
    m_ogl_disp_list_through_holes_vias_outer = 0;

    delete m_ogl_disp_list_through_holes_vias_outer_ring;
    m_ogl_disp_list_through_holes_vias_outer_ring = 0;

    delete m_ogl_disp_list_via;
    m_ogl_disp_list_via = 0;

    delete m_ogl_disp_list_pads_holes;
    m_ogl_disp_list_pads_holes = 0;

    delete m_ogl_disp_list_vias_and_pad_holes_outer_contourn_and_caps;
    m_ogl_disp_list_vias_and_pad_holes_outer_contourn_and_caps = 0;
}


void C3D_RENDER_OGL_LEGACY::render_solder_mask_layer(PCB_LAYER_ID aLayerID,
                                                     float aZPosition,
                                                     bool aDrawMiddleSegments,
                                                     bool aSkipRenderHoles )
{
    wxASSERT( (aLayerID == B_Mask) || (aLayerID == F_Mask) );

    if( m_ogl_disp_list_board )
    {
        if( m_ogl_disp_lists_layers.find( aLayerID ) !=
            m_ogl_disp_lists_layers.end() )
        {
            CLAYERS_OGL_DISP_LISTS *pLayerDispListMask = m_ogl_disp_lists_layers.at( aLayerID );

            if( m_ogl_disp_list_through_holes_vias_outer )
                m_ogl_disp_list_through_holes_vias_outer->ApplyScalePosition(
                        aZPosition,
                        m_boardAdapter.GetNonCopperLayerThickness3DU() );

            m_ogl_disp_list_board->ApplyScalePosition(
                    aZPosition,
                    m_boardAdapter.GetNonCopperLayerThickness3DU() );

            set_layer_material( aLayerID );

            m_ogl_disp_list_board->SetItIsTransparent( true );

            if( aSkipRenderHoles )
            {
                m_ogl_disp_list_board->DrawAllCameraCulled( m_camera.GetPos().z,
                                                            aDrawMiddleSegments );
            }
            else
            {
                m_ogl_disp_list_board->DrawAllCameraCulledSubtractLayer(
                            pLayerDispListMask,
                            m_ogl_disp_list_through_holes_vias_outer,
                            aDrawMiddleSegments );
            }
        }
        else
        {
            // This case there is no layer with mask, so we will render the full board as mask

            if( m_ogl_disp_list_through_holes_vias_outer )
                m_ogl_disp_list_through_holes_vias_outer->ApplyScalePosition(
                        aZPosition,
                        m_boardAdapter.GetNonCopperLayerThickness3DU() );

            m_ogl_disp_list_board->ApplyScalePosition(
                    aZPosition,
                    m_boardAdapter.GetNonCopperLayerThickness3DU() );

            set_layer_material( aLayerID );

            m_ogl_disp_list_board->SetItIsTransparent( true );

            if( aSkipRenderHoles )
            {
                m_ogl_disp_list_board->DrawAllCameraCulled( m_camera.GetPos().z,
                                                            aDrawMiddleSegments );
            }
            else
            {
                m_ogl_disp_list_board->DrawAllCameraCulledSubtractLayer(
                            NULL,
                            m_ogl_disp_list_through_holes_vias_outer,
                            aDrawMiddleSegments );
            }
        }
    }
}


void C3D_RENDER_OGL_LEGACY::render_3D_models( bool aRenderTopOrBot,
                                              bool aRenderTransparentOnly )
{
    C_OGL_3DMODEL::BeginDrawMulti();

    // Go for all modules
    for( auto module : m_boardAdapter.GetBoard()->Modules() )
    {
        if( !module->Models().empty() )
            if( m_boardAdapter.ShouldModuleBeDisplayed((MODULE_ATTR_T) module->GetAttributes() ) )
                if( ( aRenderTopOrBot && !module->IsFlipped() )
                        || ( !aRenderTopOrBot && module->IsFlipped() ) )
                    render_3D_module( module, aRenderTransparentOnly );
    }

    C_OGL_3DMODEL::EndDrawMulti();
}


void C3D_RENDER_OGL_LEGACY::render_3D_module( const MODULE* module,
                                              bool aRenderTransparentOnly )
{
    if( !module->Models().empty() )
    {
        const double zpos = m_boardAdapter.GetModulesZcoord3DIU( module->IsFlipped() );

        glPushMatrix();

        wxPoint pos = module->GetPosition();

        glTranslatef( pos.x * m_boardAdapter.BiuTo3Dunits(),
                      -pos.y * m_boardAdapter.BiuTo3Dunits(),
                       zpos );

        if( module->GetOrientation() )
            glRotated( (double) module->GetOrientation() / 10.0, 0.0, 0.0, 1.0 );

        if( module->IsFlipped() )
        {
            glRotatef( 180.0f, 0.0f, 1.0f, 0.0f );
            glRotatef( 180.0f, 0.0f, 0.0f, 1.0f );
        }

        double modelunit_to_3d_units_factor = m_boardAdapter.BiuTo3Dunits() * UNITS3D_TO_UNITSPCB;

        glScaled( modelunit_to_3d_units_factor,
                  modelunit_to_3d_units_factor,
                  modelunit_to_3d_units_factor );

        // Get the list of model files for this model
        for( auto& sM : module->Models () )
        {
            if( sM.m_Filename.empty() )
                continue;

            // Check if the model is present in our cache map
            auto cache_i = m_3dmodel_map.find( sM.m_Filename );

            if( cache_i == m_3dmodel_map.end() )
                continue;

            if( const C_OGL_3DMODEL *modelPtr = cache_i->second )
            {
                bool opaque = sM.m_Opacity >= 1.0;

                if( ( !aRenderTransparentOnly && modelPtr->Have_opaque() && opaque ) ||
                    ( aRenderTransparentOnly && ( modelPtr->Have_transparent() || !opaque ) ) )
                {
                    glPushMatrix();

                    // FIXME: don't do this over and over again unless the
                    // values have changed.  cache the matrix somewhere.
                    glm::mat4 mtx( 1 );
                    mtx = glm::translate( mtx, { sM.m_Offset.x, sM.m_Offset.y, sM.m_Offset.z } );
                    mtx = glm::rotate(
                            mtx, glm::radians( (float) -sM.m_Rotation.z ), { 0.0f, 0.0f, 1.0f } );
                    mtx = glm::rotate(
                            mtx, glm::radians( (float) -sM.m_Rotation.y ), { 0.0f, 1.0f, 0.0f } );
                    mtx = glm::rotate(
                            mtx, glm::radians( (float) -sM.m_Rotation.x ), { 1.0f, 0.0f, 0.0f } );
                    mtx = glm::scale( mtx, { sM.m_Scale.x, sM.m_Scale.y, sM.m_Scale.z } );
                    glMultMatrixf( glm::value_ptr( mtx ) );

                    if( aRenderTransparentOnly )
                        modelPtr->Draw_transparent( sM.m_Opacity );
                    else
                        modelPtr->Draw_opaque();

                    if( m_boardAdapter.GetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX ) )
                    {
                        glEnable( GL_BLEND );
                        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                        glDisable( GL_LIGHTING );

                        glLineWidth( 1 );
                        modelPtr->Draw_bboxes();

                        glLineWidth( 4 );
                        modelPtr->Draw_bbox();

                        glEnable( GL_LIGHTING );
                        glDisable( GL_BLEND );
                    }

                    glPopMatrix();
                }
            }
        }

        glPopMatrix();
    }
}


// create a 3D grid to an openGL display list: an horizontal grid (XY plane and Z = 0,
// and a vertical grid (XZ plane and Y = 0)
void C3D_RENDER_OGL_LEGACY::generate_new_3DGrid( GRID3D_TYPE aGridType )
{
    if( glIsList( m_ogl_disp_list_grid ) )
        glDeleteLists( m_ogl_disp_list_grid, 1 );

    m_ogl_disp_list_grid = 0;

    if( aGridType == GRID3D_TYPE::NONE )
        return;

    m_ogl_disp_list_grid = glGenLists( 1 );

    if( !glIsList( m_ogl_disp_list_grid ) )
        return;

    glNewList( m_ogl_disp_list_grid, GL_COMPILE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    const double zpos = 0.0;

    // Color of grid lines
    const SFVEC3F gridColor = m_boardAdapter.GetColor( DARKGRAY );

    // Color of grid lines every 5 lines
    const SFVEC3F gridColor_marker = m_boardAdapter.GetColor( LIGHTGRAY );
    const double scale = m_boardAdapter.BiuTo3Dunits();
    const double transparency = 0.35;

    double griSizeMM = 0.0;

    switch( aGridType )
    {
    default:
    case GRID3D_TYPE::NONE:
        return;
    case GRID3D_TYPE::GRID_1MM:
        griSizeMM = 1.0;
        break;
    case GRID3D_TYPE::GRID_2P5MM:
        griSizeMM = 2.5;
        break;
    case GRID3D_TYPE::GRID_5MM:
        griSizeMM = 5.0;
        break;
    case GRID3D_TYPE::GRID_10MM:
        griSizeMM = 10.0;
        break;
    }

    glNormal3f( 0.0, 0.0, 1.0 );

    const wxSize  brd_size = m_boardAdapter.GetBoardSizeBIU();
    wxPoint brd_center_pos = m_boardAdapter.GetBoardPosBIU();

    brd_center_pos.y = -brd_center_pos.y;

    const int xsize = std::max( brd_size.x, Millimeter2iu( 100 ) ) * 1.2;
    const int ysize = std::max( brd_size.y, Millimeter2iu( 100 ) ) * 1.2;

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
            glColor4f( gridColor.r, gridColor.g, gridColor.b, transparency );
        else
            glColor4f( gridColor_marker.r,
                       gridColor_marker.g,
                       gridColor_marker.b,
                       transparency );

        const int delta = KiROUND( ii * griSizeMM * IU_PER_MM );

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

    // Draw vertical grid on Z axis
    glNormal3f( 0.0, -1.0, 0.0 );

    // Draw vertical grid lines (parallel to Z axis)
    double posy = -brd_center_pos.y * scale;

    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            glColor4f( gridColor.r, gridColor.g, gridColor.b, transparency );
        else
            glColor4f( gridColor_marker.r,
                       gridColor_marker.g,
                       gridColor_marker.b,
                       transparency );

        const double delta = ii * griSizeMM * IU_PER_MM;

        glBegin( GL_LINES );
        xmax = (brd_center_pos.x + delta) * scale;

        glVertex3f( xmax, posy, zmin );
        glVertex3f( xmax, posy, zmax );
        glEnd();

        if( ii != 0 )
        {
            glBegin( GL_LINES );
            xmin = (brd_center_pos.x - delta) * scale;
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
        if( (ii % 5) )
            glColor4f( gridColor.r, gridColor.g, gridColor.b, transparency );
        else
            glColor4f( gridColor_marker.r,
                       gridColor_marker.g,
                       gridColor_marker.b,
                       transparency );

        const double delta = ii * griSizeMM * IU_PER_MM * scale;

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
