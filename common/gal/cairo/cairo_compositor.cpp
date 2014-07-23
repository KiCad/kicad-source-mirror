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
 * or you may search the http:O//www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file cairo_compositor.cpp
 * @brief Class that handles multitarget rendering (ie. to different textures/surfaces) and
 * later compositing into a single image (Cairo flavour).
 */

#include <gal/cairo/cairo_compositor.h>
#include <wx/log.h>

using namespace KIGFX;

CAIRO_COMPOSITOR::CAIRO_COMPOSITOR( cairo_t** aMainContext ) :
    m_current( 0 ), m_currentContext( aMainContext ), m_mainContext( *aMainContext )
{
}


CAIRO_COMPOSITOR::~CAIRO_COMPOSITOR()
{
    clean();
}


void CAIRO_COMPOSITOR::Initialize()
{
    // Nothing has to be done
}


void CAIRO_COMPOSITOR::Resize( unsigned int aWidth, unsigned int aHeight )
{
    clean();

    assert( aWidth > 0 );
    assert( aHeight > 0 );

    m_width  = aWidth;
    m_height = aHeight;

    m_stride     = cairo_format_stride_for_width( CAIRO_FORMAT_ARGB32, m_width );
    m_bufferSize = m_stride * m_height;
}


unsigned int CAIRO_COMPOSITOR::CreateBuffer()
{
    // Pixel storage
    BitmapPtr bitmap( new unsigned int[m_bufferSize] );

    memset( bitmap.get(), 0x00, m_bufferSize * sizeof(int) );

    // Create the Cairo surface
    cairo_surface_t* surface = cairo_image_surface_create_for_data(
                                                        (unsigned char*) bitmap.get(),
                                                        CAIRO_FORMAT_ARGB32, m_width,
                                                        m_height, m_stride );
    cairo_t* context = cairo_create( surface );
#ifdef __WXDEBUG__
    cairo_status_t status = cairo_status( context );
    wxASSERT_MSG( status == CAIRO_STATUS_SUCCESS, wxT( "Cairo context creation error" ) );
#endif /* __WXDEBUG__ */

    // Set default settings for the buffer
    cairo_set_antialias( context, CAIRO_ANTIALIAS_SUBPIXEL );
    cairo_set_line_join( context, CAIRO_LINE_JOIN_ROUND );
    cairo_set_line_cap( context, CAIRO_LINE_CAP_ROUND );

    // Use the same transformation matrix as the main context
    cairo_get_matrix( m_mainContext, &m_matrix );
    cairo_set_matrix( context, &m_matrix );

    // Store the new buffer
    CAIRO_BUFFER buffer = { context, surface, bitmap };
    m_buffers.push_back( buffer );

    return usedBuffers();
}


void CAIRO_COMPOSITOR::SetBuffer( unsigned int aBufferHandle )
{
    wxASSERT_MSG( aBufferHandle <= usedBuffers(), wxT( "Tried to use a not existing buffer" ) );

    // Get currently used transformation matrix, so it can be applied to the new buffer
    cairo_get_matrix( *m_currentContext, &m_matrix );

    m_current = aBufferHandle - 1;
    *m_currentContext = m_buffers[m_current].context;

    // Apply the current transformation matrix
    cairo_set_matrix( *m_currentContext, &m_matrix );
}


void CAIRO_COMPOSITOR::ClearBuffer()
{
    // Clear the pixel storage
    memset( m_buffers[m_current].bitmap.get(), 0x00, m_bufferSize * sizeof(int) );
}


void CAIRO_COMPOSITOR::DrawBuffer( unsigned int aBufferHandle )
{
    wxASSERT_MSG( aBufferHandle <= usedBuffers(), wxT( "Tried to use a not existing buffer" ) );

    // Reset the transformation matrix, so it is possible to composite images using
    // screen coordinates instead of world coordinates
    cairo_get_matrix( m_mainContext, &m_matrix );
    cairo_identity_matrix( m_mainContext );

    // Draw the selected buffer contents
    cairo_set_source_surface( m_mainContext, m_buffers[aBufferHandle - 1].surface, 0.0, 0.0 );
    cairo_paint( m_mainContext );

    // Restore the transformation matrix
    cairo_set_matrix( m_mainContext, &m_matrix );
}


void CAIRO_COMPOSITOR::clean()
{
    CAIRO_BUFFERS::const_iterator it;

    for( it = m_buffers.begin(); it != m_buffers.end(); ++it )
    {
        cairo_destroy( it->context );
        cairo_surface_destroy( it->surface );
    }

    m_buffers.clear();
}
