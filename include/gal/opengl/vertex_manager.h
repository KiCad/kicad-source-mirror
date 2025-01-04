/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * @file vertex_manager.h
 */

#ifndef VERTEX_MANAGER_H_
#define VERTEX_MANAGER_H_

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <gal/opengl/vertex_common.h>
#include <gal/color4d.h>
#include <stack>
#include <memory>

namespace KIGFX
{
class SHADER;
class VERTEX_ITEM;
class VERTEX_CONTAINER;
class GPU_MANAGER;

/**
 * Class to control vertex container and GPU with possibility of emulating old-style OpenGL
 * 1.0 state machine using modern OpenGL methods.
 */
class VERTEX_MANAGER
{
public:
    /**
     * @param aCached says if vertices should be cached in GPU or system memory. For data that
     *                does not change every frame, it is better to store vertices in GPU memory.
     */
    VERTEX_MANAGER( bool aCached );

    /**
     * Map vertex buffer.
     */
    void Map();

    /**
     * Unmap vertex buffer.
     */
    void Unmap();

    /**
     * Allocate space for vertices, so it will be used with subsequent Vertex() calls.
     *
     * @param aSize is the number of vertices that should be available in the reserved space.
     * @return True if successful, false otherwise.
     */
    bool Reserve( unsigned int aSize );

    /**
     * Add a vertex with the given coordinates to the currently set item.
     *
     * Color & shader parameters stored in aVertex are ignored, instead color & shader set
     * by Color() and Shader() functions are used. Vertex coordinates will have the current
     * transformation matrix applied.
     *
     * @param aVertex contains vertex coordinates.
     * @return True if successful, false otherwise.
     */
    inline bool Vertex( const VERTEX& aVertex )
    {
        return Vertex( aVertex.x, aVertex.y, aVertex.z );
    }

    /**
     * Add a vertex with the given coordinates to the currently set item.
     *
     * Vertex coordinates will have the current transformation matrix applied.
     *
     * @param aX is the X coordinate of the new vertex.
     * @param aY is the Y coordinate of the new vertex.
     * @param aZ is the Z coordinate of the new vertex.
     * @return True if successful, false otherwise.
     */
    bool Vertex( GLfloat aX, GLfloat aY, GLfloat aZ );

    /**
     * Add a vertex with the given coordinates to the currently set item.
     *
     * Vertex coordinates will have the current transformation matrix applied.
     *
     * @param aXY are the XY coordinates of the new vertex.
     * @param aZ is the Z coordinate of the new vertex.
     * @return True if successful, false otherwise.
     */
    bool Vertex( const VECTOR2D& aXY, GLfloat aZ )
    {
        return Vertex( aXY.x, aXY.y, aZ );
    }

    /**
     * Add one or more vertices to the currently set item.
     *
     * It takes advantage of allocating memory in advance, so should be faster than
     * adding vertices one by one. Color & shader parameters stored in aVertices are
     * ignored, instead color & shader set by Color() and Shader() functions are used.
     * All the vertex coordinates will have the current transformation matrix applied.
     *
     * @param aVertices contains vertices to be added.
     * @param aSize is the number of vertices to be added.
     * @return True if successful, false otherwise.
     */
    bool Vertices( const VERTEX aVertices[], unsigned int aSize );

    /**
     * Change currently used color that will be applied to newly added vertices.
     *
     * @param aColor is the new color.
     */
    inline void Color( const COLOR4D& aColor )
    {
        m_color[0] = aColor.r * 255.0;
        m_color[1] = aColor.g * 255.0;
        m_color[2] = aColor.b * 255.0;
        m_color[3] = aColor.a * 255.0;
    }

    /**
     * Change currently used color that will be applied to newly added vertices.
     *
     * It is the equivalent of glColor4f() function.
     *
     * @param aRed is the red component of the new color.
     * @param aGreen is the green component of the new color.
     * @param aBlue is the blue component of the new color.
     * @param aAlpha is the alpha component of the new color.
     */
    inline void Color( GLfloat aRed, GLfloat aGreen, GLfloat aBlue, GLfloat aAlpha )
    {
        m_color[0] = aRed   * 255.0;
        m_color[1] = aGreen * 255.0;
        m_color[2] = aBlue  * 255.0;
        m_color[3] = aAlpha * 255.0;
    }

    /**
     * Change currently used shader and its parameters that will be applied to newly added
     * vertices.
     *
     * Parameters depend on shader, for more information have a look at shaders source code.
     *
     * @see SHADER_TYPE
     *
     * @param aShaderType is the a shader type to be applied.
     * @param aParam1 is the optional parameter for a shader.
     * @param aParam2 is the optional parameter for a shader.
     * @param aParam3 is the optional parameter for a shader.
     */
    inline void Shader( GLfloat aShaderType, GLfloat aParam1 = 0.0f, GLfloat aParam2 = 0.0f,
                        GLfloat aParam3 = 0.0f )
    {
        m_shader[0] = aShaderType;
        m_shader[1] = aParam1;
        m_shader[2] = aParam2;
        m_shader[3] = aParam3;
    }

    /**
     * Multiply the current matrix by a translation matrix, so newly vertices will be
     * translated by the given vector.
     *
     * It is the equivalent of the glTranslatef() function.
     *
     * @param aX is the X coordinate of a translation vector.
     * @param aY is the X coordinate of a translation vector.
     * @param aZ is the X coordinate of a translation vector.
     */
    inline void Translate( GLfloat aX, GLfloat aY, GLfloat aZ )
    {
        m_transform = glm::translate( m_transform, glm::vec3( aX, aY, aZ ) );
    }

    /**
     * Multiply the current matrix by a rotation matrix, so the newly vertices will be
     * rotated by the given angles.
     *
     * It is the equivalent of the glRotatef() function.
     *
     * @param aAngle is the angle of rotation, in radians.
     * @param aX is a multiplier for the X axis
     * @param aY is a multiplier for the Y axis
     * @param aZ is a multiplier for the Z axis.
     */
    inline void Rotate( GLfloat aAngle, GLfloat aX, GLfloat aY, GLfloat aZ )
    {
        m_transform = glm::rotate( m_transform, aAngle, glm::vec3( aX, aY, aZ ) );
    }

    /**
     * Multiply the current matrix by a scaling matrix, so the newly vertices will be
     * scaled by the given factors.
     *
     * It is the equivalent of the glScalef() function.
     *
     * @param aX is the X axis scaling factor.
     * @param aY is the Y axis scaling factor.
     * @param aZ is the Z axis scaling factor.
     */
    inline void Scale( GLfloat aX, GLfloat aY, GLfloat aZ )
    {
        m_transform = glm::scale( m_transform, glm::vec3( aX, aY, aZ ) );
    }

    /**
     * Push the current transformation matrix stack.
     *
     * It is the equivalent of the glPushMatrix() function.
     */
    inline void PushMatrix()
    {
        m_transformStack.push( m_transform );

        // Every transformation starts with PushMatrix
        m_noTransform = false;
    }

    /**
     * Pop the current transformation matrix stack.
     *
     * It is the equivalent of the glPopMatrix() function.
     */
    void PopMatrix()
    {
        wxASSERT( !m_transformStack.empty() );

        m_transform = m_transformStack.top();
        m_transformStack.pop();

        if( m_transformStack.empty() )
        {
            // We return back to the identity matrix, thus no vertex transformation is needed
            m_noTransform = true;
        }
    }

    /**
     * Set an item to start its modifications.
     *
     * After calling the function it is possible to add vertices using function Add().
     *
     * @param aItem is the item that is going to store vertices in the container.
     */
    void SetItem( VERTEX_ITEM& aItem ) const;

    /**
     * Clean after adding an item.
     */
    void FinishItem() const;

    /**
     * Free the memory occupied by the item, so it is no longer stored in the container.
     *
     * @param aItem is the item to be freed
     */
    void FreeItem( VERTEX_ITEM& aItem ) const;

    /**
     * Change the color of all vertices owned by an item.
     *
     * @param aItem is the item to change.
     * @param aColor is the new color to be applied.
     */
    void ChangeItemColor( const VERTEX_ITEM& aItem, const COLOR4D& aColor ) const;

    /**
     * Change the depth of all vertices owned by an item.
     *
     * @param aItem is the item to change.
     * @param aDepth is the new color to be applied.
     */
    void ChangeItemDepth( const VERTEX_ITEM& aItem, GLfloat aDepth ) const;

    /**
     * Return a pointer to the vertices owned by an item.
     *
     * @param aItem is the owner of vertices that are going to be returned.
     * @return Pointer to the vertices or NULL if the item is not stored at the container.
     */
    VERTEX* GetVertices( const VERTEX_ITEM& aItem ) const;

    const glm::mat4& GetTransformation() const
    {
        return m_transform;
    }

    /**
     * Set a shader program that is going to be used during rendering.
     *
     * @param aShader is the object containing compiled and linked shader program.
     */
    void SetShader( SHADER& aShader ) const;

    /**
     * Remove all the stored vertices from the container.
     */
    void Clear() const;

    /**
     * Prepare buffers and items to start drawing.
     */
    void BeginDrawing() const;

    /**
     * Draw an item to the buffer.
     *
     * @param aItem is the item to be drawn.
     */
    void DrawItem( const VERTEX_ITEM& aItem ) const;

    /**
     * Finish drawing operations.
     */
    void EndDrawing() const;

    /**
     * Enable/disable Z buffer depth test.
     */
    void EnableDepthTest( bool aEnabled );

protected:
    /**
     * Apply all transformation to the given coordinates and store them at the specified target.
     *
     * @param aTarget is the place where the new vertex is going to be stored (it has to be
     *                allocated first).
     * @param aX is the X coordinate of the new vertex.
     * @param aY is the Y coordinate of the new vertex.
     * @param aZ is the Z coordinate of the new vertex.
     */
    void putVertex( VERTEX& aTarget, GLfloat aX, GLfloat aY, GLfloat aZ ) const;

    /// Container for vertices, may be cached or noncached
    std::shared_ptr<VERTEX_CONTAINER> m_container;

    /// GPU manager for data transfers and drawing operations
    std::shared_ptr<GPU_MANAGER>      m_gpu;

    /// State machine variables
    /// True in case there is no need to transform vertices
    bool                    m_noTransform;

    /// Currently used transform matrix
    glm::mat4               m_transform;

    /// Stack of transformation matrices, used for Push/PopMatrix
    std::stack<glm::mat4>   m_transformStack;

    /// Currently used color
    GLubyte                 m_color[COLOR_STRIDE];

    /// Currently used shader and its parameters
    GLfloat                 m_shader[SHADER_STRIDE];

    /// Currently reserved chunk to store vertices
    VERTEX*                 m_reserved;

    /// Currently available reserved space
    unsigned int            m_reservedSpace;
};

} // namespace KIGFX

#endif /* VERTEX_MANAGER_H_ */
