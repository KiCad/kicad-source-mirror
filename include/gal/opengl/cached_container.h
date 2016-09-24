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
 * @file cached_container.h
 * @brief Class to store instances of VERTEX with caching. It allows storing VERTEX objects and
 * associates them with VERTEX_ITEMs. This leads to a possibility of caching vertices data in the
 * GPU memory and a fast reuse of that data.
 */

#ifndef CACHED_CONTAINER_H_
#define CACHED_CONTAINER_H_

#include <gal/opengl/vertex_container.h>
#include <map>
#include <set>

namespace KIGFX
{
class VERTEX_ITEM;
class SHADER;

class CACHED_CONTAINER : public VERTEX_CONTAINER
{
public:
    CACHED_CONTAINER( unsigned int aSize = defaultInitSize );
    ~CACHED_CONTAINER();

    ///> @copydoc VERTEX_CONTAINER::SetItem()
    virtual void SetItem( VERTEX_ITEM* aItem ) override;

    ///> @copydoc VERTEX_CONTAINER::FinishItem()
    virtual void FinishItem() override;

    ///> @copydoc VERTEX_CONTAINER::Allocate()
    virtual VERTEX* Allocate( unsigned int aSize ) override;

    ///> @copydoc VERTEX_CONTAINER::Delete()
    virtual void Delete( VERTEX_ITEM* aItem ) override;

    ///> @copydoc VERTEX_CONTAINER::Clear()
    virtual void Clear() override;

    /**
     * Function GetBufferHandle()
     * returns handle to the vertex buffer. It might be negative if the buffer is not initialized.
     */
    inline unsigned int GetBufferHandle() const
    {
        return m_glBufferHandle;
    }

    /**
     * Function IsMapped()
     * returns true if vertex buffer is currently mapped.
     */
    inline bool IsMapped() const
    {
        return m_isMapped;
    }

    ///> @copydoc VERTEX_CONTAINER::Map()
    void Map() override;

    ///> @copydoc VERTEX_CONTAINER::Unmap()
    void Unmap() override;

protected:
    ///> Maps size of free memory chunks to their offsets
    typedef std::pair<unsigned int, unsigned int> CHUNK;
    typedef std::multimap<unsigned int, unsigned int> FREE_CHUNK_MAP;

    /// List of all the stored items
    typedef std::set<VERTEX_ITEM*> ITEMS;

    ///> Stores size & offset of free chunks.
    FREE_CHUNK_MAP      m_freeChunks;

    ///> Stored VERTEX_ITEMs
    ITEMS               m_items;

    ///> Currently modified item
    VERTEX_ITEM*        m_item;

    ///> Properties of currently modified chunk & item
    unsigned int        m_chunkSize;
    unsigned int        m_chunkOffset;

    ///> Flag saying if vertex buffer is currently mapped
    bool m_isMapped;

    ///> Flag saying if the vertex buffer is initialized
    bool m_isInitialized;

    ///> Vertex buffer handle
    unsigned int m_glBufferHandle;

    ///> Flag saying whether it is safe to use glCopyBufferSubData
    bool m_useCopyBuffer;

    /**
     * Function init()
     * performs the GL vertex buffer initialization. It can be invoked only when an OpenGL context
     * is bound.
     */
    void init();

    /**
     * Function reallocate()
     * resizes the chunk that stores the current item to the given size. The current item has
     * its offset adjusted after the call, and the new chunk parameters are stored
     * in m_chunkOffset and m_chunkSize.
     *
     * @param aSize is the requested chunk size.
     * @return true in case of success, false otherwise
     */
    bool reallocate( unsigned int aSize );

    /**
     * Function defragmentResize()
     * removes empty spaces between chunks and optionally resizes the container.
     * After the operation there is continous space for storing vertices at the end of the container.
     *
     * @param aNewSize is the new size of container, expressed in number of vertices
     * @return false in case of failure (e.g. memory shortage)
     */
    bool defragmentResize( unsigned int aNewSize );
    bool defragmentResizeMemcpy( unsigned int aNewSize );

    /**
     * Function mergeFreeChunks()
     * looks for consecutive free memory chunks and merges them, decreasing fragmentation of
     * memory.
     */
    void mergeFreeChunks();

    /**
     * Function getPowerOf2()
     * returns the nearest power of 2, bigger than aNumber.
     *
     * @param aNumber is the number for which we look for a bigger power of 2.
     */
    unsigned int getPowerOf2( unsigned int aNumber ) const;

private:
    /**
     * Function getChunkSize()
     * returns size of the given chunk.
     *
     * @param aChunk is the chunk.
     */
    inline int getChunkSize( const CHUNK& aChunk ) const
    {
        return aChunk.first;
    }

    /**
     * Function getChunkOffset()
     * returns offset of the chunk.
     *
     * @param aChunk is the chunk.
     */
    inline unsigned int getChunkOffset( const CHUNK& aChunk ) const
    {
        return aChunk.second;
    }

    /**
     * Function addFreeChunk
     * Adds a chunk marked as free.
     */
    void addFreeChunk( unsigned int aOffset, unsigned int aSize );

    /// Debug & test functions
    void showFreeChunks();
    void showUsedChunks();
    void test();
};
} // namespace KIGFX

#endif /* CACHED_CONTAINER_H_ */
