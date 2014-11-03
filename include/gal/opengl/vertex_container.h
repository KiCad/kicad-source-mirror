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
 * @file vertex_container.h
 * @brief Class to store vertices and handle transfers between system memory and GPU memory.
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
     * Function MakeContainer()
     * Returns a pointer to a new container of an appropriate type.
     */
    static VERTEX_CONTAINER* MakeContainer( bool aCached );

    virtual ~VERTEX_CONTAINER();

    /**
     * Function SetItem()
     * sets the item in order to modify or finishes its current modifications.
     * @param aItem is the item or NULL in case of finishing the item.
     */
    virtual void SetItem( VERTEX_ITEM* aItem ) = 0;

    /**
     * Function FinishItem()
     * does the cleaning after adding an item.
     */
    virtual void FinishItem() {};

    /**
     * Function Allocate()
     * returns allocated space (possibly resizing the reserved memory chunk or allocating a new
     * chunk if it was not stored before) for the given number of vertices associated with the
     * current item (set by SetItem()). The newly allocated space is added at the end of the chunk
     * used by the current item and may serve to store new vertices.
     * @param aSize is the number of vertices to be allocated.
     * @return Pointer to the allocated space or NULL in case of failure.
     */
    virtual VERTEX* Allocate( unsigned int aSize ) = 0;

    /**
     * Function Delete()
     * erases the selected item.
     *
     * @param aItem is the item to be erased.
     */
    virtual void Delete( VERTEX_ITEM* aItem ) = 0;

    /**
     * Function Clear()
     * removes all the data stored in the container and restores its original state.
     */
    virtual void Clear() = 0;

    /**
     * Function GetAllVertices()
     * returns all the vertices stored in the container. It is especially useful for transferring
     * data to the GPU memory.
     */
    inline virtual VERTEX* GetAllVertices() const
    {
        return m_vertices;
    }

    /**
     * Function GetVertices()
     * returns vertices stored at the specific offset.
     * @param aOffset is the offset.
     */
    virtual inline VERTEX* GetVertices( unsigned int aOffset ) const
    {
        return &m_vertices[aOffset];
    }

    /**
     * Function GetSize()
     * returns amount of vertices currently stored in the container.
     */
    virtual inline unsigned int GetSize() const
    {
        return m_currentSize;
    }

    /**
     * Function IsDirty()
     * returns information about container cache state. Clears the flag after calling the function.
     * @return true in case the vertices have to be reuploaded.
     */
    inline bool IsDirty()
    {
        bool state = m_dirty;

        m_dirty = false;

        return state;
    }

    /**
     * Function SetDirty()
     * sets the dirty flag, so vertices in the container are going to be reuploaded to the GPU on
     * the next frame.
     */
    inline void SetDirty()
    {
        m_dirty = true;
    }

protected:
    VERTEX_CONTAINER( unsigned int aSize = defaultInitSize );

    ///< How many vertices we can store in the container
    unsigned int    m_freeSpace;

    ///< How big is the current container, expressed in vertices
    unsigned int    m_currentSize;

    ///< Store the initial size, so it can be resized to this on Clear()
    unsigned int    m_initialSize;

    ///< Actual storage memory (should be handled using malloc/realloc/free to speed up resizing)
    VERTEX*         m_vertices;

    ///< State flags
    bool            m_failed;
    bool            m_dirty;

    /**
     * Function reservedSpace()
     * returns size of the reserved memory space.
     * @return Size of the reserved memory space (expressed as a number of vertices).
     */
    inline unsigned int reservedSpace()
    {
        return m_currentSize - m_freeSpace;
    }

    ///< Default initial size of a container (expressed in vertices)
    static const unsigned int defaultInitSize = 1048576;
};
} // namespace KIGFX

#endif /* VERTEX_CONTAINER_H_ */
