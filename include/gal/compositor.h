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
 * @file compositor.h
 * @brief Class that handles multitarget rendering (ie. to different textures/surfaces) and
 * later compositing into a single image.
 */

#ifndef COMPOSITOR_H_
#define COMPOSITOR_H_

namespace KIGFX
{
class COLOR4D;

class COMPOSITOR
{
public:
    COMPOSITOR()
        : m_width( 0 ), m_height( 0 )
    {
    }

    virtual ~COMPOSITOR()
    {
    }

    /**
     * Perform primary initialization, necessary to use the object.
     */
    virtual void Initialize() = 0;

    /**
     * Clear the state of COMPOSITOR, so it has to be reinitialized again with the new dimensions.
     *
     * @param aWidth is the framebuffer width (in pixels).
     * @param aHeight is the framebuffer height (in pixels).
     */
    virtual void Resize( unsigned int aWidth, unsigned int aHeight ) = 0;

    /**
     * Prepare a new buffer that may be used as a rendering target.
     *
     * @return is the handle of the buffer. In case of failure 0 (zero) is returned as the handle.
     */
    virtual unsigned int CreateBuffer() = 0;

    /**
     * Return currently used buffer handle.
     *
     * @return Currently used buffer handle.
     */
    virtual unsigned int GetBuffer() const = 0;

    /**
     * Set the selected buffer as the rendering target.
     *
     * All the following drawing functions are going to be rendered in the selected buffer.
     *
     * @param aBufferHandle is the handle of the buffer or 0 in case of rendering directly to the
     *                      display.
     */
    virtual void SetBuffer( unsigned int aBufferHandle ) = 0;

    /**
     * Clear the selected buffer (set by the SetBuffer() function).
     */
    virtual void ClearBuffer( const COLOR4D& aColor ) = 0;

    /**
     * Call this at the beginning of each frame.
     */
    virtual void Begin() = 0;

    /**
     * Draw the selected buffer to the output buffer.
     *
     * @param aBufferHandle is the handle of the buffer to be drawn.
     */
    virtual void DrawBuffer( unsigned int aBufferHandle ) = 0;

    /**
     * Call this to present the output buffer to the screen.
     */
    virtual void Present() = 0;

protected:
    unsigned int m_width;           ///< Width of the buffer (in pixels)
    unsigned int m_height;          ///< Height of the buffer (in pixels)
};

} // namespace KIGFX

#endif /* COMPOSITOR_H_ */
