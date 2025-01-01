/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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
 * @file vertex_item.h
 * Class to handle an item held in a container.
 */

#ifndef VERTEX_ITEM_H_
#define VERTEX_ITEM_H_

#include <gal/opengl/vertex_common.h>
#include <gal/color4d.h>
#include <cstddef>

namespace KIGFX
{
class VERTEX_MANAGER;

class VERTEX_ITEM
{
public:
    friend class CACHED_CONTAINER;
    friend class CACHED_CONTAINER_GPU;
    friend class VERTEX_MANAGER;

    explicit VERTEX_ITEM( const VERTEX_MANAGER& aManager );
    ~VERTEX_ITEM();

    /**
     * Return information about number of vertices stored.
     *
     * @return Number of vertices.
     */
    inline unsigned int GetSize() const
    {
        return m_size;
    }

    /**
     * Return data offset in the container.
     *
     * @return Data offset expressed as a number of vertices.
     */
    inline unsigned int GetOffset() const
    {
        return m_offset;
    }

    /**
     * Return pointer to the data used by the VERTEX_ITEM.
     */
    VERTEX* GetVertices() const;

private:
    /**
     * Set data offset in the container.
     *
     * @param aOffset is the offset expressed as a number of vertices.
     */
    inline void setOffset( unsigned int aOffset )
    {
        m_offset = aOffset;
    }

    /**
     * Set data size in the container.
     *
     * @param aSize is the size expressed as a number of vertices.
     */
    inline void setSize( unsigned int aSize )
    {
        m_size = aSize;
    }

    const VERTEX_MANAGER&   m_manager;
    unsigned int            m_offset;
    unsigned int            m_size;
};
} // namespace KIGFX

#endif /* VERTEX_ITEM_H_ */
