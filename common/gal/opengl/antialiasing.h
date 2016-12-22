#ifndef OPENGL_ANTIALIASING_H__
#define OPENGL_ANTIALIASING_H__

#include <memory>
#include <gal/opengl/shader.h>
#include <math/vector2d.h>

namespace KIGFX {

    class OPENGL_COMPOSITOR;

    class OPENGL_PRESENTOR
    {
    public:
        virtual bool Init() = 0;
        virtual unsigned int CreateBuffer() = 0;

        virtual VECTOR2U GetInternalBufferSize() = 0;
        virtual void OnLostBuffers() = 0;

        virtual void Begin() = 0;
        virtual void DrawBuffer(GLuint buffer) = 0;
        virtual void Present() = 0;
    };

    class ANTIALIASING_NONE : public OPENGL_PRESENTOR {
    public:
        ANTIALIASING_NONE( OPENGL_COMPOSITOR* aCompositor );

        bool Init() override;
        unsigned int CreateBuffer() override;

        VECTOR2U GetInternalBufferSize() override;
        void OnLostBuffers() override;

        void Begin() override;
        void DrawBuffer( GLuint buffer ) override;
        void Present() override;

    private:
        OPENGL_COMPOSITOR* compositor;
    };

}

#endif
