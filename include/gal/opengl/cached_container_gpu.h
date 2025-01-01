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

#ifndef CACHED_CONTAINER_GPU_H_
#define CACHED_CONTAINER_GPU_H_

#include <gal/opengl/cached_container.h>

namespace KIGFX
{

/**
 * Specialization of CACHED_CONTAINER that stores data in video memory via memory mapping.
 */
class CACHED_CONTAINER_GPU : public CACHED_CONTAINER
{
public:
    CACHED_CONTAINER_GPU( unsigned int aSize = DEFAULT_SIZE );
    ~CACHED_CONTAINER_GPU();

    unsigned int GetBufferHandle() const override
    {
        return m_glBufferHandle;
    }

    bool IsMapped() const override
    {
        return m_isMapped;
    }

    ///< @copydoc VERTEX_CONTAINER::Map()
    void Map() override;

    ///< @copydoc VERTEX_CONTAINER::Unmap()
    void Unmap() override;

    virtual unsigned int AllItemsSize() const override;


protected:
    /**
     * Remove empty spaces between chunks and optionally resizes the container.
     *
     * After the operation there is continuous space for storing vertices at the end of
     * the container.
     *
     * @param aNewSize is the new size of container, expressed in number of vertices.
     * @return false in case of failure (e.g. memory shortage).
     */
    bool defragmentResize( unsigned int aNewSize ) override;
    bool defragmentResizeMemcpy( unsigned int aNewSize );

    ///< Flag saying if vertex buffer is currently mapped
    bool m_isMapped;

    ///< Vertex buffer handle
    unsigned int m_glBufferHandle;

    ///< Flag saying whether it is safe to use glCopyBufferSubData
    bool m_useCopyBuffer;
};
} // namespace KIGFX

#endif /* CACHED_CONTAINER_GPU_H_ */
