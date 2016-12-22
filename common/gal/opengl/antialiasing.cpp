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

}
