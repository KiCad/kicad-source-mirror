#include <gal/opengl/antialiasing.h>
#include <gal/opengl/opengl_compositor.h>
#include <gal/opengl/utils.h>

#include <tuple>

#include "gl_builtin_shaders.h"

namespace KIGFX {

    // =========================
    // ANTIALIASING_NONE
    // =========================

    ANTIALIASING_NONE::ANTIALIASING_NONE( OPENGL_COMPOSITOR* aCompositor )
        : compositor(aCompositor)
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
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();


            glBegin(GL_TRIANGLES);
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 1.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-1.0f, -1.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(1.0f, 1.0f);

            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(1.0f, 1.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-1.0f, -1.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(1.0f, -1.0f);
            glEnd();

            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }


    }

    // =========================
    // ANTIALIASING_SUPERSAMPLING
    // =========================

    ANTIALIASING_SUPERSAMPLING::ANTIALIASING_SUPERSAMPLING( OPENGL_COMPOSITOR* aCompositor,
                                                            SUPERSAMPLING_MODE aMode )
        : compositor( aCompositor ), mode( aMode ), areBuffersCreated( false ),
          areShadersCreated( false )
    {
    }

    bool ANTIALIASING_SUPERSAMPLING::Init()
    {
        if( mode == SUPERSAMPLING_MODE::X4 && !areShadersCreated ) {
            x4_shader.reset(new SHADER());
            x4_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_VERTEX,   BUILTIN_SHADERS::ssaa_x4_vertex_shader   );
            x4_shader->LoadShaderFromStrings( KIGFX::SHADER_TYPE_FRAGMENT, BUILTIN_SHADERS::ssaa_x4_fragment_shader );
            x4_shader->Link();
            checkGlError( "linking supersampling x4 shader" );

            GLint source_parameter = x4_shader->AddParameter( "source" );                          checkGlError("getting pass 1 colorTex");

            x4_shader->Use();                                                                      checkGlError("using pass 1 shader");
            x4_shader->SetParameter( source_parameter, 0 );                                        checkGlError("setting colorTex uniform");
            x4_shader->Deactivate();                                                               checkGlError("deactivating pass 2 shader");

            areShadersCreated = true;
        }

        if( areShadersCreated && mode != SUPERSAMPLING_MODE::X4 ) {
            x4_shader.reset();
            areShadersCreated = false;
        }

        if( !areBuffersCreated ) {
            ssaaMainBuffer = compositor->CreateBuffer();
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

            areBuffersCreated = true;
        }

        return true;
    }

    VECTOR2U ANTIALIASING_SUPERSAMPLING::GetInternalBufferSize()
    {
        unsigned int factor = (mode == SUPERSAMPLING_MODE::X2) ? 2 : 4;
        return compositor->GetScreenSize() * factor;
    }

    void ANTIALIASING_SUPERSAMPLING::Begin()
    {
        compositor->SetBuffer( ssaaMainBuffer );
        compositor->ClearBuffer();
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

        if( mode == SUPERSAMPLING_MODE::X4 ) {
            x4_shader->Use();
            checkGlError ( "activating supersampling x4 shader" );
        }

        draw_fullscreen_primitive();

        if( mode == SUPERSAMPLING_MODE::X4 ) {
            x4_shader->Deactivate();
            checkGlError ( "deactivating supersampling x4 shader" );
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

}
