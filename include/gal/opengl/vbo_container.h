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
 * @file vbo_container.h
 * @brief Class to store VBO_ITEMs.
 */

#ifndef VBO_CONTAINER_H_
#define VBO_CONTAINER_H_

#include <GL/gl.h>
#include <gal/opengl/glm/glm.hpp>
#include <gal/opengl/vbo_item.h>
#include <gal/color4d.h>
#include <map>
#include <set>

namespace KiGfx
{
class VBO_ITEM;
typedef struct VBO_VERTEX VBO_VERTEX;

class VBO_CONTAINER
{
public:
    VBO_CONTAINER( unsigned int aSize = defaultInitSize );
    virtual ~VBO_CONTAINER();

    ///< Maps size of free memory chunks to their offsets
    typedef std::pair<const unsigned int, unsigned int> Chunk;
    typedef std::multimap<const unsigned int, unsigned int> FreeChunkMap;

    /// List of all the stored items
    typedef std::set<VBO_ITEM*> Items;

    /**
     * Function StartItem()
     * Sets an item to start its modifications. After calling the function it is possible to add
     * vertices using function Add().
     * @param aItem is the item that is going to store vertices in the container.
     */
    void StartItem( VBO_ITEM* aItem );

    /**
     * Function EndItem()
     * Marks current item as finished and returns unused memory to the pool. StartItem() function
     * has to be called first.
     */
    void EndItem();

    /**
     * Function Add()
     * Stores given number of vertices in the container for the specific VBO_ITEM (started by
     * StartItem() function).
     * @param aItem is the owner of the vertices.
     * @param aVertex are vertices data to be stored.
     * @param aSize is the number of vertices to be added.
     */
    void Add( const VBO_VERTEX* aVertex, unsigned int aSize = 1 );

    /**
     * Function Free()
     * Frees the chunk reserved by the aItem.
     * @param aItem is the owner of the chunk to be freed.
     */
    void Free( VBO_ITEM* aItem );

    /**
     * Function Clear()
     * Removes all the data stored in the container.
     */
    void Clear();

    /**
     * Function GetAllVertices()
     * Returns all vertices stored in the container. It is especially useful for transferring
     * data to the GPU memory.
     */
    VBO_VERTEX* GetAllVertices() const;

    /**
     * Function GetVertices()
     * Returns vertices stored by the specific item.
     * @aItem is the specific item.
     */
    VBO_VERTEX* GetVertices( const VBO_ITEM* aItem ) const;

    /**
     * Function GetVertices()
     * Returns vertices stored at the specific offset.
     * @aOffset is the specific offset.
     */
    inline VBO_VERTEX* GetVertices( unsigned int aOffset ) const
    {
        return &m_vertices[aOffset];
    }

    /**
     * Function GetSize()
     * Returns amount of vertices currently stored in the container.
     */
    inline int GetSize() const
    {
        return m_currentSize;
    }

    /**
     * Function SetTransformMatrix()
     * Sets transformation matrix for vertices that are added to VBO_ITEM. If you do not want to
     * transform vertices at all, pass NULL as the argument.
     * @param aMatrix is the new transform matrix or NULL if you do not want to use transformation
     * matrix.
     */
    inline void SetTransformMatrix( const glm::mat4* aMatrix )
    {
        m_transform = aMatrix;
    }

    /**
     * Function UseColor()
     * Sets color used for all added vertices.
     * @param aColor is the color used for added vertices.
     */
    inline void UseColor( const COLOR4D& aColor )
    {
        m_color[0] = aColor.r * 255;
        m_color[1] = aColor.g * 255;
        m_color[2] = aColor.b * 255;
        m_color[3] = aColor.a * 255;
    }

    /**
     * Function UseColor()
     * Sets color used for all added vertices.
     * @param aColor is the color used for added vertices.
     */
    inline void UseColor( const GLfloat aColor[VBO_ITEM::ColorStride] )
    {
        for( unsigned int i = 0; i < VBO_ITEM::ColorStride; ++i )
        {
            m_color[i] = aColor[i] * 255;
        }
    }

    /**
     * Function UseColor()
     * Sets color used for all added vertices.
     * @param aR is the red component of the color.
     * @param aG is the green component of the color.
     * @param aB is the blue component of the color.
     * @param aA is the alpha component of the color.
     */
    inline void UseColor( GLfloat aR, GLfloat aG, GLfloat aB, GLfloat aA )
    {
        m_color[0] = aR * 255;
        m_color[1] = aG * 255;
        m_color[2] = aB * 255;
        m_color[3] = aA * 255;
    }

    /**
     * Function UseShader()
     * Sets shader and its parameters used for all added vertices.
     * @param aShader is the array that contains shader number followed by its parameters.
     */
    inline void UseShader( const GLfloat aShader[VBO_ITEM::ShaderStride] )
    {
        for( unsigned int i = 0; i < VBO_ITEM::ShaderStride; ++i )
        {
            m_shader[i] = aShader[i];
        }
    }

private:
    ///< Stores size & offset of free chunks.
    FreeChunkMap        m_freeChunks;
    ///< Stored VERTEX_ITEMs
    Items               m_items;

    /**
     * Function allocate()
     * Allocates the given amount of memory for the current VBO_ITEM (set by StartItem() function).
     * @param aSize is the number of vertices that are requested to be allocated.
     * @return Pointer to the allocated space.
     */
    VBO_VERTEX* allocate( unsigned int aSize );

    /**
    * Function reallocate()
    * Resizes the chunk that stores the current item to the given size.
    * @param aSize is the number of vertices to be stored.
    * @return Offset of the new chunk.
    */
    unsigned int reallocate( unsigned int aSize );

    /**
     * Function getChunkSize()
     * Returns size of the given chunk.
     * @param aChunk is the chunk.
     * @return Size of the chunk.
     */
    inline unsigned int getChunkSize( const Chunk& aChunk ) const
    {
        return aChunk.first;
    }

    /**
     * Function getChunkOffset()
     * Returns offset of the given chunk.
     * @param aChunk is the chunk.
     * @return Offset of the chunk.
     */
    inline unsigned int getChunkOffset( const Chunk& aChunk ) const
    {
        return aChunk.second;
    }

    /**
     * Function setChunkOffset()
     * Updates offset of the given chunk.
     * !! IMPORTANT: it does not reallocate the chunk, it just changes its properties.
     * @param aChunk is the chunk.
     */
    inline void setChunkOffset( Chunk& aChunk, unsigned int aOffset ) const
    {
        aChunk.second = aOffset;
    }

    /**
     * Function defragment()
     * Removes empty spaces between chunks, so after that there is a long continous space
     * for storing vertices at the and of the container.
     * @return false in case of failure (eg. memory shortage).
     */
    bool defragment( VBO_VERTEX* aTarget = NULL );

    /**
     * Function mergeFreeChunks()
     * Looks for consecutive free memory chunks and merges them, decreasing fragmentation of
     * memory.
     */
    void mergeFreeChunks();

    /**
     * Function resizeContainer()
     * Prepares a bigger container of a given size.
     * @param aNewSize is the new size of container, expressed in vertices
     * @return false in case of failure (eg. memory shortage).
     */
    bool resizeContainer( unsigned int aNewSize );

    /**
     * Function freeItem()
     * Frees the space occupied by the item and returns it to the free space pool.
     * @param aItem is the item to be freed.
     */
    void freeItem( VBO_ITEM* aItem );

    /**
     * Function reservedSpace()
     * Returns size of the reserved memory space.
     * @return Size of the reserved memory space (expressed as a number of vertices).
     */
    unsigned int reservedSpace()
    {
        return m_currentSize - m_freeSpace;
    }

    ///< How many vertices we can store in the container
    unsigned int    m_freeSpace;

    ///< How big is the current container, expressed in vertices
    unsigned int    m_currentSize;

    ///< Actual storage memory
    VBO_VERTEX*     m_vertices;

    ///< Initial size, used on clearing the container
    unsigned int    m_initialSize;

    ///< Variables holding the state of the item currently being modified
    unsigned int    m_itemSize;
    unsigned int    m_chunkSize;
    unsigned int    m_chunkOffset;
    VBO_ITEM*       m_item;

    ///< Color used for the new vertices pushed.
    GLubyte         m_color[VBO_ITEM::ColorStride];

    ///< Shader and its parameters used for new vertices pushed
    GLfloat         m_shader[VBO_ITEM::ShaderStride];

    ///< Current transform matrix applied for every new vertex pushed.
    const glm::mat4* m_transform;

    ///< Failure flag
    bool            m_failed;

    /**
     * Function getPowerOf2()
     * Returns the nearest power of 2, bigger than aNumber.
     * @param aNumber is the number for which we look for a bigger power of 2.
     */
    unsigned int getPowerOf2( unsigned int aNumber ) const
    {
        unsigned int power = 1;

        while( power < aNumber && power )
            power <<= 1;

        return power;
    }

    ///< Default initial size of a container (expressed in vertices)
    static const unsigned int defaultInitSize = 1048576;

    ///< Basic tests for the container, use only for debugging.
    void test() const;
};
} // namespace KiGfx

#endif /* VBO_CONTAINER_H_ */
