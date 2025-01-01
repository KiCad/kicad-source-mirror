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

#ifndef CACHED_CONTAINER_RAM_H_
#define CACHED_CONTAINER_RAM_H_

#include <gal/opengl/cached_container.h>
#include <map>
#include <set>

namespace KIGFX
{
class VERTEX_ITEM;
class SHADER;

/**
 * Specialization of CACHED_CONTAINER that stores data in RAM.
 *
 * This is mainly for video cards/drivers that do not cope well with video memory mapping.
 */

class CACHED_CONTAINER_RAM : public CACHED_CONTAINER
{
public:
    CACHED_CONTAINER_RAM( unsigned int aSize = DEFAULT_SIZE );
    ~CACHED_CONTAINER_RAM();

    ///< @copydoc VERTEX_CONTAINER::Unmap()
    void Map() override {}

    ///< @copydoc VERTEX_CONTAINER::Unmap()
    void Unmap() override;

    bool IsMapped() const override
    {
        return true;
    }

    /**
     * Return handle to the vertex buffer.
     *
     * It might be negative if the buffer is not initialized.
     */
    unsigned int GetBufferHandle() const override
    {
        return m_verticesBuffer;    // make common with CACHED_CONTAINER_RAM
    }

protected:
    /**
     * Defragment the currently stored data and resizes the buffer.
     *
     * @param aNewSize is the new buffer vertex buffer size, expressed as the number of vertices.
     * @return true on success.
     */
    bool defragmentResize( unsigned int aNewSize ) override;

    ///< Handle to vertices buffer
    GLuint  m_verticesBuffer;
};
} // namespace KIGFX

#endif /* CACHED_CONTAINER_RAM_H_ */
