/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
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
    virtual ~OPENGL_PRESENTOR()
    {
    }

    virtual bool Init() = 0;
    virtual unsigned int CreateBuffer() = 0;

    virtual VECTOR2I GetInternalBufferSize() = 0;
    virtual void OnLostBuffers() = 0;

    virtual void Begin() = 0;
    virtual void DrawBuffer( GLuint aBuffer ) = 0;
    virtual void Present() = 0;
};


class ANTIALIASING_NONE : public OPENGL_PRESENTOR
{
public:
    ANTIALIASING_NONE( OPENGL_COMPOSITOR* aCompositor );

    bool Init() override;
    unsigned int CreateBuffer() override;

    VECTOR2I GetInternalBufferSize() override;
    void OnLostBuffers() override;

    void Begin() override;
    void DrawBuffer( GLuint aBuffer ) override;
    void Present() override;

private:
    OPENGL_COMPOSITOR* compositor;
};


class ANTIALIASING_SUPERSAMPLING : public OPENGL_PRESENTOR
{
public:
    ANTIALIASING_SUPERSAMPLING( OPENGL_COMPOSITOR* aCompositor );

    bool Init() override;
    unsigned int CreateBuffer() override;

    VECTOR2I GetInternalBufferSize() override;
    void OnLostBuffers() override;

    void Begin() override;
    void DrawBuffer( GLuint ) override;
    void Present() override;

private:
    OPENGL_COMPOSITOR* compositor;

    unsigned int ssaaMainBuffer;
    bool areBuffersCreated;

    bool areShadersCreated;
};

class ANTIALIASING_SMAA : public OPENGL_PRESENTOR
{
public:
    ANTIALIASING_SMAA( OPENGL_COMPOSITOR* aCompositor );

    bool Init() override;
    unsigned int CreateBuffer () override;

    VECTOR2I GetInternalBufferSize() override;
    void OnLostBuffers() override;

    void Begin() override;
    void DrawBuffer( GLuint buffer ) override;
    void Present() override;

private:
    void loadShaders();
    void updateUniforms();

    bool areBuffersInitialized;

    unsigned int smaaBaseBuffer;    // base + overlay temporary
    unsigned int smaaEdgesBuffer;
    unsigned int smaaBlendBuffer;

    // smaa shader lookup textures
    unsigned int smaaAreaTex;
    unsigned int smaaSearchTex;

    bool shadersLoaded;

    std::unique_ptr<SHADER> pass_1_shader;
    GLint pass_1_metrics;

    std::unique_ptr<SHADER> pass_2_shader;
    GLint pass_2_metrics;

    std::unique_ptr<SHADER> pass_3_shader;
    GLint pass_3_metrics;

    OPENGL_COMPOSITOR* compositor;
};

}

#endif
