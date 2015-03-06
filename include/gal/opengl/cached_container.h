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

// Debug messages verbosity level
// #define CACHED_CONTAINER_TEST 1

namespace KIGFX
{
class VERTEX_ITEM;
class SHADER;

class CACHED_CONTAINER : public VERTEX_CONTAINER
{
public:
    CACHED_CONTAINER( unsigned int aSize = defaultInitSize );

    ///> @copydoc VERTEX_CONTAINER::SetItem()
    virtual void SetItem( VERTEX_ITEM* aItem );

    ///> @copydoc VERTEX_CONTAINER::FinishItem()
    virtual void FinishItem();

    ///> @copydoc VERTEX_CONTAINER::Allocate()
    virtual VERTEX* Allocate( unsigned int aSize );

    ///> @copydoc VERTEX_CONTAINER::Delete()
    virtual void Delete( VERTEX_ITEM* aItem );

    ///> @copydoc VERTEX_CONTAINER::Clear()
    virtual void Clear();

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
    unsigned int        m_itemSize;

    /**
     * Function reallocate()
     * resizes the chunk that stores the current item to the given size.
     *
     * @param aSize is the number of vertices to be stored.
     * @return offset of the new chunk.
     */
    virtual unsigned int reallocate( unsigned int aSize );

    /**
     * Function defragment()
     * removes empty spaces between chunks, so after that there is a long continous space
     * for storing vertices at the and of the container.
     *
     * @param aTarget is the already allocated destination for defragmented data. It has to be
     * at least of the same size as the current container. If left NULL, it will be allocated
     * inside the defragment() function.
     * @return false in case of failure (eg. memory shortage)
     */
    virtual bool defragment( VERTEX* aTarget = NULL );

    /**
     * Function mergeFreeChunks()
     * looks for consecutive free memory chunks and merges them, decreasing fragmentation of
     * memory.
     */
    virtual void mergeFreeChunks();

    /**
     * Function resizeContainer()
     *
     * prepares a bigger container of a given size.
     * @param aNewSize is the new size of container, expressed in vertices
     * @return false in case of failure (eg. memory shortage)
     */
    virtual bool resizeContainer( unsigned int aNewSize );

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

    /// Debug & test functions
#if CACHED_CONTAINER_TEST > 0
    void showFreeChunks();
    void showReservedChunks();
    void test();
#else
    inline void showFreeChunks() {}
    inline void showReservedChunks() {}
    inline void test() {}
#endif /* CACHED_CONTAINER_TEST */
};
} // namespace KIGFX

#endif /* CACHED_CONTAINER_H_ */
