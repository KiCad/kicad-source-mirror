/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * @file gpu_manager.h
 * @brief Class to handle uploading vertices and indices to GPU in drawing purposes.
 */

#ifndef GPU_MANAGER_H_
#define GPU_MANAGER_H_

#include <gal/opengl/vertex_common.h>
#include <boost/scoped_array.hpp>

namespace KIGFX
{
class SHADER;
class VERTEX_CONTAINER;
class CACHED_CONTAINER;
class NONCACHED_CONTAINER;

class GPU_MANAGER
{
public:
    static GPU_MANAGER* MakeManager( VERTEX_CONTAINER* aContainer );

    virtual ~GPU_MANAGER();

    /**
     * @brief Initializes everything needed to use vertex buffer objects (should be called when
     * there is an OpenGL context available).
     */
    virtual void Initialize() = 0;

    /**
     * Function BeginDrawing()
     * Prepares the stored data to be drawn.
     */
    virtual void BeginDrawing() = 0;

    /**
     * Function DrawIndices()
     * Makes the GPU draw given range of vertices.
     * @param aOffset is the beginning of the range.
     * @param aSize is the number of vertices to be drawn.
     */
    virtual void DrawIndices( unsigned int aOffset, unsigned int aSize ) = 0;

    /**
     * Function DrawIndices()
     * Makes the GPU draw all the vertices stored in the container.
     */
    virtual void DrawAll() = 0;

    /**
     * Function EndDrawing()
     * Clears the container after drawing routines.
     */
    virtual void EndDrawing() = 0;

    /**
     * Function SetShader()
     * Allows using shaders with the stored data.
     * @param aShader is the object that allows using shaders.
     */
    virtual void SetShader( SHADER& aShader );

protected:
    GPU_MANAGER( VERTEX_CONTAINER* aContainer );

    ///> Drawing status flag.
    bool m_isDrawing;

    ///> Container that stores vertices data.
    VERTEX_CONTAINER* m_container;

    ///> Shader handling
    SHADER* m_shader;

    ///> Location of shader attributes (for glVertexAttribPointer)
    int m_shaderAttrib;
};


class GPU_CACHED_MANAGER : public GPU_MANAGER
{
public:
    GPU_CACHED_MANAGER( VERTEX_CONTAINER* aContainer );
    ~GPU_CACHED_MANAGER();

    ///> @copydoc GPU_MANAGER::Initialize()
    virtual void Initialize();

    ///> @copydoc GPU_MANAGER::BeginDrawing()
    virtual void BeginDrawing();

    ///> @copydoc GPU_MANAGER::DrawIndices()
    virtual void DrawIndices( unsigned int aOffset, unsigned int aSize );

    ///> @copydoc GPU_MANAGER::DrawAll()
    virtual void DrawAll();

    ///> @copydoc GPU_MANAGER::EndDrawing()
    virtual void EndDrawing();

    /**
     * Function uploadToGpu
     * Rebuilds vertex buffer object using stored VERTEX_ITEMs and sends it to the graphics card
     * memory.
     */
    virtual void uploadToGpu();

protected:
    ///> Buffers initialization flag
    bool m_buffersInitialized;

    ///> Pointer to the current indices buffer
    boost::scoped_array<GLuint> m_indices;

    ///> Pointer to the first free cell in the indices buffer
    GLuint* m_indicesPtr;

    ///> Handle to vertices buffer
    GLuint  m_verticesBuffer;

    ///> Handle to indices buffer
    GLuint  m_indicesBuffer;

    ///> Number of indices stored in the indices buffer
    unsigned int m_indicesSize;
};


class GPU_NONCACHED_MANAGER : public GPU_MANAGER
{
public:
    GPU_NONCACHED_MANAGER( VERTEX_CONTAINER* aContainer );

    ///> @copydoc GPU_MANAGER::Initialize()
    virtual void Initialize();

    ///> @copydoc GPU_MANAGER::BeginDrawing()
    virtual void BeginDrawing();

    ///> @copydoc GPU_MANAGER::DrawIndices()
    virtual void DrawIndices( unsigned int aOffset, unsigned int aSize );

    ///> @copydoc GPU_MANAGER::DrawAll()
    virtual void DrawAll();

    ///> @copydoc GPU_MANAGER::EndDrawing()
    virtual void EndDrawing();
};
} // namespace KIGFX
#endif /* GPU_MANAGER_H_ */
