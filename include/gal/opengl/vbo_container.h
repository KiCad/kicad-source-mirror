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
#include <wx/log.h>

namespace KiGfx
{
class VBO_ITEM;
typedef struct VBO_VERTEX VBO_VERTEX;

class VBO_CONTAINER
{
public:
    VBO_CONTAINER( int aSize = 1048576 );
    ~VBO_CONTAINER();

    ///< Maps size of free memory chunks to their offsets
    typedef std::pair<const unsigned int, unsigned int> Chunk;
    typedef std::multimap<const unsigned int, unsigned int> FreeChunkMap;

    ///< Maps size of reserved memory chunks to their owners (VBO_ITEMs)
    typedef std::pair<VBO_ITEM* const, Chunk> ReservedChunk;
    typedef std::multimap<VBO_ITEM* const, Chunk> ReservedChunkMap;

    /**
     * Function StartItem()
     * Starts an unknown sized item. After calling the function it is possible to add vertices
     * using function Add().
     * @param aVboItem is the item that is going to store vertices in the container.
     */
    void StartItem( VBO_ITEM* aVboItem );

    /**
     * Function EndItem()
     * Marks current item as finished and returns unused memory to the pool. StartItem() function
     * has to be called first.
     */
    void EndItem();

    /**
     * Function Add()
     * Stores given number of vertices in the container for the specific VBO_ITEM.
     * @param aVboItem is the owner of the vertices.
     * @param aVertex are vertices data to be stored.
     * @param aSize is the number of vertices to be added.
     */
    void Add( VBO_ITEM* aVboItem, const VBO_VERTEX* aVertex, unsigned int aSize = 1 );

    /**
     * Function Free()
     * Frees the chunk reserved by the aVboItem.
     * @param aVboItem is the owner of the chunk to be freed.
     */
    inline void Free( VBO_ITEM* aVboItem )
    {
        ReservedChunkMap::iterator it = m_reservedChunks.find( aVboItem );
        free( it );

        // Dynamic memory freeing, there is no point in holding
        // a large amount of memory when there is no use for it
        if( m_freeSpace > ( m_currentSize / 2 ) )
        {
            resizeContainer( m_currentSize / 2 );
        }
    }

    /**
     * Function GetAllVertices()
     * Returns all vertices stored in the container. It is especially useful for transferring
     * data to the GPU memory.
     */
    VBO_VERTEX* GetAllVertices() const;

    /**
     * Function GetVertices()
     * Returns vertices stored by the specific item.
     * @aVboItem is the specific item.
     */
    VBO_VERTEX* GetVertices( const VBO_ITEM* aVboItem ) const;

    /**
     * Function GetVertices()
     * Returns vertices stored at the specific offset.
     * @aOffest is the specific offset.
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
        m_color[0] = aColor.r;
        m_color[1] = aColor.g;
        m_color[2] = aColor.b;
        m_color[3] = aColor.a;
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
            m_color[i] = aColor[i];
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
        m_color[0] = aR;
        m_color[1] = aG;
        m_color[2] = aB;
        m_color[3] = aA;
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
    ///< Stores owners (VBO_ITEM*) of reserved chunks and their size & offset.
    ReservedChunkMap    m_reservedChunks;

    /**
    * Function allocate()
    * Finds an offset where the number of vertices can be stored in a continous space. If there is
    * no such chunk, appropriate amount of memory is allocated first.
    * @param aVboItem is the owner of vertices to be stored.
    * @param aSize is the number of vertices to be stored.
    */
    unsigned int allocate( VBO_ITEM* aVboItem, unsigned int aSize );

    /**
     * Function getChunkSize()
     * Returns size of the given chunk (works both for reserved and free chunks).
     * @param aChunk is the chunk.
     */
    inline int getChunkSize( const Chunk& aChunk ) const
    {
        return aChunk.first;
    }

    inline int getChunkSize( const ReservedChunk& aChunk ) const
    {
        return aChunk.second.first;
    }

    /**
     * Function getChunkOffset()
     * Returns offset of the given chunk (works both for reserved and free chunks).
     * @param aChunk is the chunk.
     */
    inline unsigned int getChunkOffset( const Chunk& aChunk ) const
    {
        return aChunk.second;
    }

    inline unsigned int getChunkOffset( const ReservedChunk& aChunk ) const
    {
        return aChunk.second.second;
    }

    /**
     * Function getChunkOffset()
     * Upadtes offset of the given chunk (works both for reserved and free chunks).
     * !! IMPORTANT: it does not reallocate the chunk, it just changes its properties.
     * @param aChunk is the chunk.
     */
    inline void setChunkOffset( Chunk& aChunk, unsigned int aOffset ) const
    {
        aChunk.second = aOffset;
    }

    inline void setChunkOffset( ReservedChunk& aChunk, unsigned int aOffset ) const
    {
        aChunk.second.second = aOffset;
    }

    /**
     * Function getChunkVboItem()
     * Returns owner of the given reserved chunk.
     * @param aChunk is the chunk.
     */
    inline VBO_ITEM* getChunkVboItem( const ReservedChunk& aChunk ) const
    {
        return aChunk.first;
    }

    /**
     * Function defragment()
     * Removes empty spaces between chunks, so after that there is a long continous space
     * for storing vertices at the and of the container.
     * @return false in case of failure (eg. memory shortage)
     */
    bool defragment( VBO_VERTEX* aTarget = NULL );

    /**
     * Function resizeChunk()
     * Changes size of the chunk that stores vertices of aVboItem.
     * @param aVboItem is the item for which reserved space size should be changed.
     * @param aNewSize is the new size for the aVboItem, expressed in vertices number.
     */
    void resizeChunk( VBO_ITEM* aVboItem, int aNewSize );

    /**
     * Function resizeContainer()
     * Prepares a bigger container of a given size.
     * @param aNewSize is the new size of container, expressed in vertices
     * @return false in case of failure (eg. memory shortage)
     */
    bool resizeContainer( unsigned int aNewSize );

    /**
     * Function free()
     * Frees the space described in aChunk and returns it to the free space pool.
     * @param aChunk is a space to be freed.
     */
    void free( const ReservedChunkMap::iterator& aChunk );

    ///< How many vertices we can store in the container
    unsigned int    m_freeSpace;

    ///< How big is the current container, expressed in vertices
    unsigned int    m_currentSize;

    ///< Actual storage memory
    VBO_VERTEX*     m_vertices;

    ///< A flag saying if there is the item with an unknown size being added
    bool            itemStarted;

    ///< Variables holding the state of the item currently being added
    unsigned int    itemSize;
    unsigned int    itemChunkSize;
    VBO_ITEM*       item;

    ///< Color used for new vertices pushed.
    GLfloat         m_color[VBO_ITEM::ColorStride];

    ///< Shader and its parameters used for new vertices pushed
    GLfloat         m_shader[VBO_ITEM::ShaderStride];

    ///< Current transform matrix applied for every new vertex pushed.
    const glm::mat4*    m_transform;

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
};
} // namespace KiGfx

#endif /* VBO_CONTAINER_H_ */
