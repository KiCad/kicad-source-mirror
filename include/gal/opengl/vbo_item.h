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
typedef struct VBO_VERTEX_DATA
{
    GLfloat x, y, z;        // Coordinates
    GLfloat r, g, b, a;     // Color
    GLfloat shader[4];      // Shader type & params
} VBO_VERTEX_DATA;

typedef struct VBO_VERTEX_STRUCT
{
    GLfloat coord[3];       // Coordinates
    GLfloat color[4];       // Color
    GLfloat shader[4];      // Shader type & params
} VBO_VERTEX_STRUCT;

typedef union VBO_VERTEX
{
    VBO_VERTEX_DATA     data;
    VBO_VERTEX_STRUCT   struc;
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
     * Function GetIndices()
     * Returns a pointer to the array containing all indices of vertices.
     * @return Pointer to indices.
     */
    GLuint* GetIndices();

    /**
     * Function GetSize()
     * Returns information about number of vertices stored.
     * @param Amount of vertices.
     */
    int  GetSize() const;

    /**
     * Function SetOffset()
     * Sets data offset in the VBO.
     * @param aOffset is the offset expressed as a number of vertices.
     */
    void SetOffset( int aOffset );

    /**
     * Function GetOffset()
     * Returns data offset in the VBO.
     * @return Data offset expressed as a number of vertices.
     */
    int  GetOffset() const;

    /**
     * Function SetTransformMatrix()
     * Sets transformation matrix for vertices that are added to VBO_ITEM. If you do not want to
     * transform vertices at all, pass NULL as the argument.
     * @param aMatrix is the new transform matrix or NULL if you do not want to use transformation
     * matrix.
     */
    void SetTransformMatrix( const glm::mat4* aMatrix );

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
    void UseColor( const COLOR4D& aColor );

    /**
     * Function UseShader()
     * Sets shader and its parameters used for all added vertices.
     * @param aShader is the array that contains shader number followed by its parameters.
     */
    void UseShader( const GLfloat* aShader );

    ///< Functions for getting VBO ids.
    //void SetVbo( int aVboId );
    //int  GetVbo() const;

    ///< Data organization information for vertices {X,Y,Z,R,G,B,A} (@see VBO_VERTEX).
    static const int VertByteSize       = sizeof(VBO_VERTEX);
    static const int VertStride         = VertByteSize / sizeof(GLfloat);

    static const int CoordStride        = sizeof(VBO_VERTEX_STRUCT().coord) / sizeof(GLfloat);
    static const int CoordByteSize      = sizeof(VBO_VERTEX_STRUCT().coord);

    // Offset of color data from the beginning of each vertex data
    static const int ColorByteOffset    = offsetof( VBO_VERTEX_STRUCT, color );
    static const int ColorOffset        = ColorByteOffset / sizeof(GLfloat);
    static const int ColorStride        = sizeof(VBO_VERTEX_STRUCT().color) / sizeof(GLfloat);
    static const int ColorByteSize      = sizeof(VBO_VERTEX_STRUCT().color);

    // Shader attributes
    static const int ShaderByteOffset   = offsetof( VBO_VERTEX_STRUCT, shader );
    static const int ShaderOffset       = ShaderByteOffset / sizeof(GLfloat);
    static const int ShaderStride       = sizeof(VBO_VERTEX_STRUCT().shader) / sizeof(GLfloat);
    static const int ShaderByteSize     = sizeof(VBO_VERTEX_STRUCT().shader);

    static const int IndStride          = 1;
    static const int IndByteSize        = IndStride * sizeof(GLuint);

private:
    ///< VBO ids in which the item is stored.
    //int         m_vboId; // not used yet

    ///< Contains vertices coordinates and colors.
    ///< Packed by 7 floats for each vertex: {X, Y, Z, R, G, B, A}
    GLfloat*    m_vertices;

    ///< Indices of vertices
    GLuint*     m_indices;

    ///< Lists of data blocks storing vertices
    std::list<VBO_VERTEX*>  m_vertBlocks;
    std::list<GLuint*>      m_indBlocks;
    ///< Pointers to current blocks that should be used for storing data
    VBO_VERTEX*             m_vertPtr;
    GLuint*                 m_indPtr;
    ///< How many vertices can be stored in the current buffer
    int                     m_spaceLeft;
    ///< Number of vertices & indices stored in a single block
    static const int        BLOCK_SIZE = 256;
    ///< Creates a new block for storing vertices data
    void                    useNewBlock();
    ///< Prepares a continuous block of data that can be copied to graphics card buffer.
    void                    prepareFinal();

    ///< Offset and size of data in VBO.
    int         m_offset;
    int         m_size;

    ///< Color used for new vertices pushed.
    GLfloat     m_color[ColorStride];

    ///< Shader and its parameters used for new vertices pushed
    GLfloat     m_shader[ShaderStride];

    ///< Flag telling if the item should be recached in VBO or not.
    bool        m_isDirty;

    ///< Current transform matrix for every new vertex pushed.
    const glm::mat4* m_transform;
};
} // namespace KiGfx

#endif /* VBO_ITEM_H_ */
