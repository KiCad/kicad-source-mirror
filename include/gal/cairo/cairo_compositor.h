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
 * @file cairo_compositor.h
 * Class that handles multitarget rendering (ie. to different textures/surfaces) and
 * later compositing into a single image (Cairo flavour).
 */

#ifndef CAIRO_COMPOSITOR_H_
#define CAIRO_COMPOSITOR_H_

#include <gal/compositor.h>
#include <gal/gal_display_options.h>
#include <cairo.h>

#include <cstdint>
#include <deque>

namespace KIGFX
{
class CAIRO_COMPOSITOR : public COMPOSITOR
{
public:
    CAIRO_COMPOSITOR( cairo_t** aMainContext );
    virtual ~CAIRO_COMPOSITOR();

    /// @copydoc COMPOSITOR::Initialize()
    virtual void Initialize() override;

    /// @copydoc COMPOSITOR::Resize()
    virtual void Resize( unsigned int aWidth, unsigned int aHeight ) override;

    /// @copydoc COMPOSITOR::CreateBuffer()
    virtual unsigned int CreateBuffer() override;

    /// @copydoc COMPOSITOR::GetBuffer()
    inline virtual unsigned int GetBuffer() const override
    {
        return m_current + 1;
    }

    /// @copydoc COMPOSITOR::SetBuffer()
    virtual void SetBuffer( unsigned int aBufferHandle ) override;

    /// @copydoc COMPOSITOR::Begin()
    virtual void Begin() override;

    /// @copydoc COMPOSITOR::ClearBuffer()
    virtual void ClearBuffer( const COLOR4D& aColor ) override;

    /**
     * Paints source to destination using the cairo operator. Useful for differential mode.
     *
     * @param aSourceHandle Source buffer to paint
     * @param aDestHandle Destination buffer to paint on to
     * @param op Painting operation
     */
    void DrawBuffer( unsigned int aSourceHandle, unsigned int aDestHandle, cairo_operator_t op );

    /// @copydoc COMPOSITOR::DrawBuffer()
    virtual void DrawBuffer( unsigned int aBufferHandle ) override;

    /// @copydoc COMPOSITOR::Present()
    virtual void Present() override;

    void SetAntialiasingMode( GAL_ANTIALIASING_MODE aMode ); // clears all buffers
    GAL_ANTIALIASING_MODE GetAntialiasingMode() const
    {
        switch( m_currentAntialiasingMode )
        {
        case CAIRO_ANTIALIAS_FAST:
            return GAL_ANTIALIASING_MODE::AA_FAST;
        case CAIRO_ANTIALIAS_GOOD:
            return GAL_ANTIALIASING_MODE::AA_HIGHQUALITY;
        default:
            return GAL_ANTIALIASING_MODE::AA_NONE;
        }
    }

    /**
     * Set a context to be treated as the main context (ie. as a target of buffers rendering and
     * as a source of settings for newly created buffers).
     *
     * @param aMainContext is the context that should be treated as the main one.
     */
    inline virtual void SetMainContext( cairo_t* aMainContext )
    {
        m_mainContext = aMainContext;

        // Use the context's transformation matrix
        cairo_get_matrix( m_mainContext, &m_matrix );
    }

protected:
    /**
     * Perform freeing of resources.
     */
    void clean();

    /// Return number of currently used buffers.
    unsigned int usedBuffers()
    {
        return m_buffers.size();
    }

    typedef uint8_t* BitmapPtr;
    struct CAIRO_BUFFER
    {
        cairo_t*            context;        ///< Main texture handle
        cairo_surface_t*    surface;        ///< Point to which an image from texture is attached
        BitmapPtr           bitmap;         ///< Pixel storage
    };

    unsigned int            m_current;      ///< Currently used buffer handle
    typedef std::deque<CAIRO_BUFFER> CAIRO_BUFFERS;

    /// Pointer to the current context, so it can be changed
    cairo_t**               m_currentContext;

    /// Rendering target used for compositing (the main display)
    cairo_t*                m_mainContext;

    /// Transformation matrix
    cairo_matrix_t          m_matrix;

    /// Stores information about initialized buffers
    CAIRO_BUFFERS           m_buffers;

    unsigned int m_stride;              ///< Stride to use given the desired format and width
    unsigned int m_bufferSize;          ///< Amount of memory needed to store a buffer

    cairo_antialias_t       m_currentAntialiasingMode;
};
} // namespace KIGFX

#endif /* COMPOSITOR_H_ */
