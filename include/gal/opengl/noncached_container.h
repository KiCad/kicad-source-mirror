/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file noncached_container.h
 * @brief Class to store instances of VERTEX without caching. It allows a fast one-frame drawing
 * and then clearing the buffer and starting from scratch.
 */

#ifndef NONCACHED_CONTAINER_H_
#define NONCACHED_CONTAINER_H_

#include <gal/opengl/vertex_container.h>

namespace KIGFX
{
class VERTEX_ITEM;
class SHADER;

class NONCACHED_CONTAINER : public VERTEX_CONTAINER
{
public:
    /**
     * Construct a non-cached container object.
     *
     * @param aSize is the size of the cache.
     * @throw bad_alloc exception if memory allocation fails.
     */
    NONCACHED_CONTAINER( unsigned int aSize = DEFAULT_SIZE );
    virtual ~NONCACHED_CONTAINER();

    bool IsCached() const override
    {
        return false;
    }

    /// @copydoc VERTEX_CONTAINER::SetItem( VERTEX_ITEM* aItem )
    virtual void SetItem( VERTEX_ITEM* aItem ) override;

    /// @copydoc VERTEX_CONTAINER::Allocate( unsigned int aSize )
    virtual VERTEX* Allocate( unsigned int aSize ) override;

    /// @copydoc VERTEX_CONTAINER::Delete( VERTEX_ITEM* aItem )
    void Delete( VERTEX_ITEM* aItem ) override {}

    /// @copydoc VERTEX_CONTAINER::Clear()
    virtual void Clear() override;

    /// @copydoc VERTEX_CONTAINER::GetSize()
    virtual unsigned int GetSize() const override
    {
        // As the m_freePtr points to the first free space, we can safely assume
        // that this is the number of vertices stored inside
        return m_freePtr;
    }

protected:
    ///< Index of the free first space where a vertex can be stored
    unsigned int m_freePtr;
};
} // namespace KIGFX

#endif /* NONCACHED_CONTAINER_H_ */
