/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
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

#include <gal/opengl/antialiasing.h>
#include <gal/opengl/opengl_compositor.h>
#include <gal/opengl/utils.h>
#include <gal/color4d.h>

#include <memory>
#include <tuple>

#include "gl_builtin_shaders.h"
#include "SmaaAreaTex.h"
#include "SmaaSearchTex.h"

using namespace KIGFX;

// =========================
// ANTIALIASING_NONE
// =========================

ANTIALIASING_NONE::ANTIALIASING_NONE( OPENGL_COMPOSITOR* aCompositor )
    : compositor( aCompositor )
{
}


bool ANTIALIASING_NONE::Init()
{
    // Nothing to initialize
    return true;
}


VECTOR2U ANTIALIASING_NONE::GetInternalBufferSize()
{
    return compositor->GetScreenSize();
}


void ANTIALIASING_NONE::DrawBuffer( GLuint buffer )
{
    compositor->DrawBuffer( buffer, OPENGL_COMPOSITOR::DIRECT_RENDERING );
}


void ANTIALIASING_NONE::Present()
{
    // Nothing to present, draw_buffer already drew to the screen
}


void ANTIALIASING_NONE::OnLostBuffers()
{
    // Nothing to do
}


void ANTIALIASING_NONE::Begin()
{
    // Nothing to do
}


unsigned int ANTIALIASING_NONE::CreateBuffer()
{
    return compositor->CreateBuffer( compositor->GetScreenSize() );
}


namespace {

    void draw_fullscreen_primitive()
    {
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode( GL_PROJECTION );
        glPushMatrix();
        glLoadIdentity();


        glBegin( GL_TRIANGLES );
        glTexCoord2f( 0.0f, 1.0f );
        glVertex2f( -1.0f, 1.0f );
        glTexCoord2f( 0.0f, 0.0f );
        glVertex2f( -1.0f, -1.0f );
        glTexCoord2f( 1.0f, 1.0f );
        glVertex2f( 1.0f, 1.0f );

        glTexCoord2f( 1.0f, 1.0f );
        glVertex2f( 1.0f, 1.0f );
        glTexCoord2f( 0.0f, 0.0f );
        glVertex2f( -1.0f, -1.0f );
        glTexCoord2f( 1.0f, 0.0f );
        glVertex2f( 1.0f, -1.0f );
        glEnd();

        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );
        glPopMatrix();
    }

}

// =========================
// ANTIALIASING_SUPERSAMPLING
// =========================

ANTIALIASING_SUPERSAMPLING::ANTIALIASING_SUPERSAMPLING( OPENGL_COMPOSITOR* aCompositor,
    SUPERSAMPLING_MODE aMode )
    : compositor( aCompositor ), mode( aMode ), ssaaMainBuffer( 0 ),
    areBuffersCreated( false ), areShadersCreated( false )
{
}


bool ANTIALIASING_SUPERSAMPLING::Init()
{
    if( mode == SUPERSAMPLING_MODE::X4 && !areShadersCreated )
    {
        x4_shader = std::make_unique<SHADER>( );
        x4_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_VERTEX, BUILTIN_SHADERS::ssaa_x4_vertex_shader );
        x4_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_FRAGMENT, BUILTIN_SHADERS::ssaa_x4_fragment_shader );
        x4_shader->Link();
        checkGlError( "linking supersampling x4 shader" );

        GLint source_parameter = x4_shader->AddParameter( "source" );                          checkGlError( "getting pass 1 colorTex" );

        x4_shader->Use();                                                                      checkGlError( "using pass 1 shader" );
        x4_shader->SetParameter( source_parameter, 0 );                                        checkGlError( "setting colorTex uniform" );
        x4_shader->Deactivate();                                                               checkGlError( "deactivating pass 2 shader" );

        areShadersCreated = true;
    }

    if( areShadersCreated && mode != SUPERSAMPLING_MODE::X4 )
    {
        x4_shader.reset();
        areShadersCreated = false;
    }

    if( !areBuffersCreated )
    {
        ssaaMainBuffer = compositor->CreateBuffer();
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

        areBuffersCreated = true;
    }

    return true;
}


VECTOR2U ANTIALIASING_SUPERSAMPLING::GetInternalBufferSize()
{
    unsigned int factor = ( mode == SUPERSAMPLING_MODE::X2 ) ? 2 : 4;
    return compositor->GetScreenSize() * factor;
}


void ANTIALIASING_SUPERSAMPLING::Begin()
{
    compositor->SetBuffer( ssaaMainBuffer );
    compositor->ClearBuffer( COLOR4D::BLACK );
}


void ANTIALIASING_SUPERSAMPLING::DrawBuffer( GLuint aBuffer )
{
    compositor->DrawBuffer( aBuffer, ssaaMainBuffer );
}


void ANTIALIASING_SUPERSAMPLING::Present()
{
    glDisable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, compositor->GetBufferTexture( ssaaMainBuffer ) );
    compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

    if( mode == SUPERSAMPLING_MODE::X4 )
    {
        x4_shader->Use();
        checkGlError( "activating supersampling x4 shader" );
    }

    draw_fullscreen_primitive();

    if( mode == SUPERSAMPLING_MODE::X4 )
    {
        x4_shader->Deactivate();
        checkGlError( "deactivating supersampling x4 shader" );
    }
}


void ANTIALIASING_SUPERSAMPLING::OnLostBuffers()
{
    areBuffersCreated = false;
}


unsigned int ANTIALIASING_SUPERSAMPLING::CreateBuffer()
{
    return compositor->CreateBuffer( GetInternalBufferSize() );
}

// ===============================
// ANTIALIASING_SMAA
// ===============================

ANTIALIASING_SMAA::ANTIALIASING_SMAA( OPENGL_COMPOSITOR* aCompositor, SMAA_QUALITY aQuality )
    : areBuffersInitialized( false ), shadersLoaded( false ),
    quality( aQuality ), compositor( aCompositor )
{
    smaaBaseBuffer = 0;
    smaaEdgesBuffer = 0;
    smaaBlendBuffer = 0;
    smaaAreaTex = 0;
    smaaSearchTex = 0;

    pass_1_metrics = 0;
    pass_2_metrics = 0;
    pass_3_metrics = 0;
}


VECTOR2U ANTIALIASING_SMAA::GetInternalBufferSize()
{
    return compositor->GetScreenSize();
}


void ANTIALIASING_SMAA::loadShaders()
{
    // Load constant textures
    glEnable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE0 );

    glGenTextures( 1, &smaaAreaTex );
    glBindTexture( GL_TEXTURE_2D, smaaAreaTex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RG8, AREATEX_WIDTH, AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, areaTexBytes );
    checkGlError( "loading smaa area tex" );

    glGenTextures( 1, &smaaSearchTex );
    glBindTexture( GL_TEXTURE_2D, smaaSearchTex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, searchTexBytes );
    checkGlError( "loading smaa search tex" );

    std::string quality_string;

    if( quality == SMAA_QUALITY::HIGH )
    {
        quality_string = "#define SMAA_PRESET_HIGH\n";
    }
    else
    {
        quality_string = "#define SMAA_PRESET_ULTRA\n";
    }


    // set up shaders
    std::string vert_preamble( R"SHADER(
#version 120
#define SMAA_GLSL_2_1
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0
uniform vec4 SMAA_RT_METRICS;
)SHADER" );

    std::string frag_preamble( R"SHADER(
#version 120
#define SMAA_GLSL_2_1
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
uniform vec4 SMAA_RT_METRICS;
)SHADER" );

    std::string smaa_source =
            std::string( BUILTIN_SHADERS::smaa_base_shader_p1 )
        + std::string( BUILTIN_SHADERS::smaa_base_shader_p2 )
        + std::string( BUILTIN_SHADERS::smaa_base_shader_p3 )
        + std::string( BUILTIN_SHADERS::smaa_base_shader_p4 );

    //
    // Set up pass 1 Shader
    //
    pass_1_shader = std::make_unique<SHADER>( );
    pass_1_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_VERTEX, vert_preamble,
        quality_string, smaa_source, BUILTIN_SHADERS::smaa_pass_1_vertex_shader );
    pass_1_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_FRAGMENT, frag_preamble,
        quality_string, smaa_source, BUILTIN_SHADERS::smaa_pass_1_fragment_shader );
    pass_1_shader->Link();
    checkGlError( "linking pass 1 shader" );

    GLint smaaColorTexParameter = pass_1_shader->AddParameter( "colorTex" );                   checkGlError( "pass1: getting colorTex uniform" );
    pass_1_metrics = pass_1_shader->AddParameter( "SMAA_RT_METRICS" );                         checkGlError( "pass1: getting metrics uniform" );

    pass_1_shader->Use();                                                                      checkGlError( "pass1: using shader" );
    pass_1_shader->SetParameter( smaaColorTexParameter, 0 );                                   checkGlError( "pass1: setting colorTex uniform" );
    pass_1_shader->Deactivate();                                                               checkGlError( "pass1: deactivating shader" );

    //
    // set up pass 2 shader
    //
    pass_2_shader = std::make_unique<SHADER>( );
    pass_2_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_VERTEX, vert_preamble,
        quality_string, smaa_source, BUILTIN_SHADERS::smaa_pass_2_vertex_shader );
    pass_2_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_FRAGMENT, frag_preamble,
        quality_string, smaa_source, BUILTIN_SHADERS::smaa_pass_2_fragment_shader );
    pass_2_shader->Link();
    checkGlError( "linking pass 2 shader" );

    GLint smaaEdgesTexParameter  = pass_2_shader->AddParameter( "edgesTex" );                  checkGlError( "pass2: getting colorTex uniform" );
    GLint smaaAreaTexParameter   = pass_2_shader->AddParameter( "areaTex" );                   checkGlError( "pass2: getting areaTex uniform" );
    GLint smaaSearchTexParameter = pass_2_shader->AddParameter( "searchTex" );                 checkGlError( "pass2: getting searchTex uniform" );
    pass_2_metrics = pass_2_shader->AddParameter( "SMAA_RT_METRICS" );                         checkGlError( "pass2: getting metrics uniform" );

    pass_2_shader->Use();                                                                      checkGlError( "pass2: using shader" );
    pass_2_shader->SetParameter( smaaEdgesTexParameter, 0 );                                   checkGlError( "pass2: setting colorTex uniform" );
    pass_2_shader->SetParameter( smaaAreaTexParameter, 1 );                                    checkGlError( "pass2: setting areaTex uniform" );
    pass_2_shader->SetParameter( smaaSearchTexParameter, 3 );                                  checkGlError( "pass2: setting searchTex uniform" );
    pass_2_shader->Deactivate();                                                               checkGlError( "pass2: deactivating shader" );

    //
    // set up pass 3 shader
    //
    pass_3_shader = std::make_unique<SHADER>( );
    pass_3_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_VERTEX, vert_preamble,
        quality_string, smaa_source, BUILTIN_SHADERS::smaa_pass_3_vertex_shader );
    pass_3_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_FRAGMENT, frag_preamble,
        quality_string, smaa_source, BUILTIN_SHADERS::smaa_pass_3_fragment_shader );
    pass_3_shader->Link();

    GLint smaaP3ColorTexParameter = pass_3_shader->AddParameter( "colorTex" );                 checkGlError( "pass3: getting colorTex uniform" );
    GLint smaaBlendTexParameter = pass_3_shader->AddParameter( "blendTex" );                   checkGlError( "pass3: getting blendTex uniform" );
    pass_3_metrics = pass_3_shader->AddParameter( "SMAA_RT_METRICS" );                         checkGlError( "pass3: getting metrics uniform" );

    pass_3_shader->Use();                                                                      checkGlError( "pass3: using shader" );
    pass_3_shader->SetParameter( smaaP3ColorTexParameter, 0 );                                 checkGlError( "pass3: setting colorTex uniform" );
    pass_3_shader->SetParameter( smaaBlendTexParameter, 1 );                                   checkGlError( "pass3: setting blendTex uniform" );
    pass_3_shader->Deactivate();                                                               checkGlError( "pass3: deactivating shader" );

    shadersLoaded = true;
}


void ANTIALIASING_SMAA::updateUniforms()
{
    auto dims = compositor->GetScreenSize();

    pass_1_shader->Use();                                                                      checkGlError( "pass1: using shader" );
    pass_1_shader->SetParameter( pass_1_metrics,
        1.f / float( dims.x ), 1.f / float( dims.y ), float( dims.x ), float( dims.y ) );      checkGlError( "pass1: setting metrics uniform" );
    pass_1_shader->Deactivate();                                                               checkGlError( "pass1: deactivating shader" );

    pass_2_shader->Use();                                                                      checkGlError( "pass2: using shader" );
    pass_2_shader->SetParameter( pass_2_metrics,
        1.f / float( dims.x ), 1.f / float( dims.y ), float( dims.x ), float( dims.y ) );      checkGlError( "pass2: setting metrics uniform" );
    pass_2_shader->Deactivate();                                                               checkGlError( "pass2: deactivating shader" );

    pass_3_shader->Use();                                                                      checkGlError( "pass3: using shader" );
    pass_3_shader->SetParameter( pass_3_metrics,
        1.f / float( dims.x ), 1.f / float( dims.y ), float( dims.x ), float( dims.y ) );      checkGlError( "pass3: setting metrics uniform" );
    pass_3_shader->Deactivate();                                                               checkGlError( "pass3: deactivating shader" );
}


bool ANTIALIASING_SMAA::Init()
{
    if( !shadersLoaded )
        loadShaders();

    if( !areBuffersInitialized )
    {
        smaaBaseBuffer = compositor->CreateBuffer();
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

        smaaEdgesBuffer = compositor->CreateBuffer();
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

        smaaBlendBuffer = compositor->CreateBuffer();
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

        updateUniforms();
        areBuffersInitialized = true;
    }

    // Nothing to initialize
    return true;
}


void ANTIALIASING_SMAA::OnLostBuffers()
{
    areBuffersInitialized = false;
}


unsigned int ANTIALIASING_SMAA::CreateBuffer()
{
    return compositor->CreateBuffer( compositor->GetScreenSize() );
}


void ANTIALIASING_SMAA::DrawBuffer( GLuint buffer )
{
    // draw to internal buffer
    compositor->DrawBuffer( buffer, smaaBaseBuffer );
}


void ANTIALIASING_SMAA::Begin()
{
    compositor->SetBuffer( smaaBaseBuffer );
    compositor->ClearBuffer( COLOR4D::BLACK );
}


namespace {
    void draw_fullscreen_triangle()
    {
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode( GL_PROJECTION );
        glPushMatrix();
        glLoadIdentity();

        glBegin( GL_TRIANGLES );
        glTexCoord2f( 0.0f, 1.0f );
        glVertex2f( -1.0f, 1.0f );
        glTexCoord2f( 0.0f, -1.0f );
        glVertex2f( -1.0f, -3.0f );
        glTexCoord2f( 2.0f, 1.0f );
        glVertex2f( 3.0f, 1.0f );
        glEnd();

        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );
        glPopMatrix();
    }
}


void ANTIALIASING_SMAA::Present()
{
    auto sourceTexture = compositor->GetBufferTexture( smaaBaseBuffer );

    glDisable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );
    glEnable( GL_TEXTURE_2D );

    //
    // pass 1: main-buffer -> smaaEdgesBuffer
    //
    compositor->SetBuffer( smaaEdgesBuffer );
    compositor->ClearBuffer( COLOR4D::BLACK );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, sourceTexture );                                             checkGlError( "binding colorTex" );
    pass_1_shader->Use();                                                                      checkGlError( "using smaa pass 1 shader" );
    draw_fullscreen_triangle();
    pass_1_shader->Deactivate();

    //
    // pass 2: smaaEdgesBuffer -> smaaBlendBuffer
    //
    compositor->SetBuffer( smaaBlendBuffer );
    compositor->ClearBuffer( COLOR4D::BLACK );

    auto edgesTex = compositor->GetBufferTexture( smaaEdgesBuffer );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, edgesTex );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, smaaAreaTex );
    glActiveTexture( GL_TEXTURE3 );
    glBindTexture( GL_TEXTURE_2D, smaaSearchTex );

    pass_2_shader->Use();
    draw_fullscreen_triangle();
    pass_2_shader->Deactivate();

    //
    // pass 3: colorTex + BlendBuffer -> output
    //
    compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );
    compositor->ClearBuffer( COLOR4D::BLACK );
    auto blendTex = compositor->GetBufferTexture( smaaBlendBuffer );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, sourceTexture );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, blendTex );

    pass_3_shader->Use();
    draw_fullscreen_triangle();
    pass_3_shader->Deactivate();
}
