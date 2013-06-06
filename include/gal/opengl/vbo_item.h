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
 * @file vbo_item.h
 * @brief Class to handle an item held in a Vertex Buffer Object.
 */

#ifndef VBO_ITEM_H_
#define VBO_ITEM_H_

#include <GL/gl.h>
#include <gal/opengl/glm/glm.hpp>
#include <gal/color4d.h>

#include <cstddef>

#include <list>

namespace KiGfx
{
typedef struct VBO_VERTEX
{
    GLfloat x, y, z;        // Coordinates
    GLfloat r, g, b, a;     // Color
    GLfloat shader[4];      // Shader type & params
} VBO_VERTEX;

class VBO_ITEM
{
public:
    VBO_ITEM();
    ~VBO_ITEM();

    /**
     * Function PushVertex()
     * Adds a single vertex to the VBO_ITEM. Vertex contains information about coordinates and
     * colors and has to follow the specified format {X,Y,Z,R,G,B,A}.
     * @param aVertex is a vertex to be added.
     * @param aShader is an attribute for shader.
     */
    void PushVertex( const GLfloat* aVertex );

    /**
     * Function PushVertices()
     * Adds multiple vertices to the VBO_ITEM. This function is recommended over multiple calls to
     * PushVertex, as it does less memory reallocations. Vertices contain information about
     * coordinates and colors and has to follow the specified format {X,Y,Z,R,G,B,A}.
     * @param aVertices are vertices to be added.
     * @param aSize is an amount of vertices to be added.
     * @param aShader is an attribute for shader.
     */
    void PushVertices( const GLfloat* aVertices, GLuint aSize );

    /**
     * Function GetVertices()
     * Returns a pointer to the array containing all vertices.
     * @return Pointer to vertices packed in format {X, Y, Z, R, G, B, A}.
     */
    GLfloat* GetVertices();


    /**
     * Function GetSize()
     * Returns information about number of vertices stored.
     * @param Amount of vertices.
     */
    inline int GetSize() const
    {
        return m_size;
    }

    /**
     * Function SetOffset()
     * Sets data offset in the VBO.
     * @param aOffset is the offset expressed as a number of vertices.
     */
    void SetOffset( int aOffset )
    {
        m_offset = aOffset;
    }

    /**
     * Function GetOffset()
     * Returns data offset in the VBO.
     * @return Data offset expressed as a number of vertices.
     */
    inline int GetOffset() const
    {
        return m_offset;
    }

    /**
     * Function SetTransformMatrix()
     * Sets transformation matrix for vertices that are added to VBO_ITEM. If you do not want to
     * transform vertices at all, pass NULL as the argument.
     * @param aMatrix is the new transform matrix or NULL if you do not want to use transformation
     * matrix.
     */
    void SetTransformMatrix( const glm::mat4* aMatrix )
    {
        m_transform = aMatrix;
    }

    /**
     * Function ChangeColor()
     * Colors all vertices to the specified color.
     * @param aColor is the new color for vertices.
     */
    void ChangeColor( const COLOR4D& aColor );

    /**
     * Function UseColor()
     * Sets color used for all added vertices.
     * @param aColor is the color used for added vertices.
     */
    void UseColor( const COLOR4D& aColor )
    {
        m_color[0] = aColor.r;
        m_color[1] = aColor.g;
        m_color[2] = aColor.b;
        m_color[3] = aColor.a;
    }

    /**
     * Function UseShader()
     * Sets shader and its parameters used for all added vertices.
     * @param aShader is the array that contains shader number followed by its parameters.
     */
    inline void UseShader( const GLfloat* aShader )
    {
        for( int i = 0; i < ShaderStride; ++i )
        {
            m_shader[i] = aShader[i];
        }
    }


    inline void FreeVerticesData()
    {
        if( m_vertices && !m_isDirty )
        {
            delete[] m_vertices;
            m_vertices = NULL;
        }
    }


    ///< Data organization information for vertices {X,Y,Z,R,G,B,A} (@see VBO_VERTEX).
    static const int VertByteSize       = sizeof(VBO_VERTEX);
    static const int VertStride         = VertByteSize / sizeof(GLfloat);

    static const int CoordByteSize      = sizeof(VBO_VERTEX().x) + sizeof(VBO_VERTEX().y) +
                                            sizeof(VBO_VERTEX().z);
    static const int CoordStride        = CoordByteSize / sizeof(GLfloat);

    // Offset of color data from the beginning of each vertex data
    static const int ColorByteOffset    = offsetof(VBO_VERTEX, r);
    static const int ColorOffset        = ColorByteOffset / sizeof(GLfloat);
    static const int ColorByteSize      = sizeof(VBO_VERTEX().r) + sizeof(VBO_VERTEX().g) +
                                            sizeof(VBO_VERTEX().b) + sizeof(VBO_VERTEX().a);
    static const int ColorStride        = ColorByteSize / sizeof(GLfloat);

    // Shader attributes
    static const int ShaderByteOffset   = offsetof(VBO_VERTEX, shader);
    static const int ShaderOffset       = ShaderByteOffset / sizeof(GLfloat);
    static const int ShaderByteSize     = sizeof(VBO_VERTEX().shader);
    static const int ShaderStride       = ShaderByteSize / sizeof(GLfloat);

    static const int IndByteSize        = sizeof(GLuint);

private:
    ///< Contains vertices coordinates and colors.
    ///< Packed by 7 floats for each vertex: {X, Y, Z, R, G, B, A}
    GLfloat*                m_vertices;

    ///< Lists of data blocks storing vertices
    std::list<VBO_VERTEX*>  m_vertBlocks;

    ///< Pointers to current blocks that should be used for storing data
    VBO_VERTEX*             m_vertPtr;

    ///< How many vertices can be stored in the current buffer
    int                     m_spaceLeft;
    ///< Number of vertices stored in a single block
    static const int        BLOCK_SIZE = 256;
    ///< Creates a new block for storing vertices data
    void                    useNewBlock();
    ///< Prepares a continuous block of data that can be copied to graphics card buffer.
    void                    prepareFinal();

    ///< Offset and size of data in VBO.
    int                     m_offset;
    int                     m_size;

    ///< Color used for new vertices pushed.
    GLfloat                 m_color[ColorStride];

    ///< Shader and its parameters used for new vertices pushed
    GLfloat                 m_shader[ShaderStride];

    ///< Flag telling if the item should be recached in VBO or not.
    bool                    m_isDirty;

    ///< Current transform matrix applied for every new vertex pushed.
    const glm::mat4*        m_transform;
};
} // namespace KiGfx

#endif /* VBO_ITEM_H_ */
