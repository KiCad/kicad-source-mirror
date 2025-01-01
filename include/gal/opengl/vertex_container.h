/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2013-2017 CERN
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
 * @file vertex_container.h
 * Class to store vertices and handle transfers between system memory and GPU memory.
 */

#ifndef VERTEX_CONTAINER_H_
#define VERTEX_CONTAINER_H_

#include <gal/opengl/vertex_common.h>

namespace KIGFX
{
class VERTEX_ITEM;
class SHADER;

class VERTEX_CONTAINER
{
public:
    /**
     * Return a pointer to a new container of an appropriate type.
     */
    static VERTEX_CONTAINER* MakeContainer( bool aCached );

    virtual ~VERTEX_CONTAINER();

    /**
     * Return true if the container caches vertex data in RAM or video memory.
     * Otherwise it is a single batch draw which is later discarded.
     */
    virtual bool IsCached() const = 0;

    /**
     * Prepare the container for vertices updates.
     */
    virtual void Map() {}

    /**
     * Finish the vertices updates stage.
     */
    virtual void Unmap() {}

    /**
     * Set the item for the further actions.
     *
     * @param aItem is the item or NULL in case of finishing the item.
     */
    virtual void SetItem( VERTEX_ITEM* aItem ) = 0;

    /**
     * Clean up after adding an item.
     */
    virtual void FinishItem() {};

    /**
     * Return allocated space for the requested number of vertices associated with the
     * current item (set with SetItem()).
     *
     * The allocated space is added at the end of the chunk used by the current item and
     * may serve to store new vertices.
     *
     * @param aSize is the number of vertices to be allocated.
     * @return Pointer to the allocated space or NULL in case of failure.
     */
    virtual VERTEX* Allocate( unsigned int aSize ) = 0;

    /**
     * Erase the data related to an item.
     *
     * @param aItem is the item to be erased.
     */
    virtual void Delete( VERTEX_ITEM* aItem ) = 0;

    /**
     * Remove all data stored in the container and restores its original state.
     */
    virtual void Clear() = 0;

    /**
     * Return pointer to the vertices stored in the container.
     */
    VERTEX* GetAllVertices() const
    {
        return m_vertices;
    }

    /**
     * Return vertices stored at the specific offset.
     *
     * @param aOffset is the offset.
     */
    virtual VERTEX* GetVertices( unsigned int aOffset ) const
    {
        return &m_vertices[aOffset];
    }

    /**
     * Return amount of vertices currently stored in the container.
     */
    virtual unsigned int GetSize() const
    {
        return m_currentSize;
    }

    /**
     * Return information about the container cache state.
     *
     * @return True in case the vertices have to be reuploaded.
     */
    bool IsDirty() const
    {
        return m_dirty;
    }

    /**
     * Set the dirty flag, so vertices in the container are going to be reuploaded to the GPU on
     * the next frame.
     */
    void SetDirty()
    {
        m_dirty = true;
    }

    /**
     * Clear the dirty flag to prevent reuploading vertices to the GPU memory.
     */
    void ClearDirty()
    {
        m_dirty = false;
    }

protected:
    VERTEX_CONTAINER( unsigned int aSize = DEFAULT_SIZE );

    /**
     * Return size of the used memory space.
     *
     * @return Size of the used memory space (expressed as a number of vertices).
     */
    unsigned int usedSpace() const
    {
        return m_currentSize - m_freeSpace;
    }

    ///< Free space left in the container, expressed in vertices
    unsigned int    m_freeSpace;

    ///< Current container size, expressed in vertices
    unsigned int    m_currentSize;

    ///< Store the initial size, so it can be resized to this on Clear()
    unsigned int    m_initialSize;

    ///< Actual storage memory
    VERTEX*         m_vertices;

    // Status flags
    bool            m_failed;
    bool            m_dirty;

    ///< Default initial size of a container (expressed in vertices)
    static constexpr unsigned int DEFAULT_SIZE = 1048576;
};
} // namespace KIGFX

#endif /* VERTEX_CONTAINER_H_ */
