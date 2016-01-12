/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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
 * a helper to handle the real device context used in KiCad
 * @file kicad_device_context.h
 */

#ifndef __KICAD_DEVICE_CONTEXT_H__
#define __KICAD_DEVICE_CONTEXT_H__

#include <wx/dcbuffer.h>

#if defined(KICAD_USE_BUFFERED_PAINTDC)
#undef KICAD_USE_BUFFERED_PAINTDC
#endif

#if defined(KICAD_USE_BUFFERED_DC)
#undef KICAD_USE_BUFFERED_DC
#endif

// wxWidgets defines the platforms where device context buffering is well behaved.  These
// definitions take advantage of their experience in this area.  See <wx/dcbuffer.h> for
// more information.
#if wxALWAYS_NATIVE_DOUBLE_BUFFER
#define KICAD_USE_BUFFERED_PAINTDC 1
#define KICAD_USE_BUFFERED_DC_HELPER 0
#define KICAD_USE_BUFFERED_DC 0
#else
#define KICAD_USE_BUFFERED_PAINTDC 1
#define KICAD_USE_BUFFERED_DC_HELPER 1
#define KICAD_USE_BUFFERED_DC 1
#endif


/**
 * Class BUFFERED_DC_HELPER
 * fixes a bug on Windows when using buffered device context.
 *
 * <p>
 * When using buffered device context drawing in Windows, the user scaling must be set to 1
 * and the logical offset must be set to zero before the bitmap blit operation occurs in
 * the destructor of wxBufferdDC but after the normal drawing takes place.
 * </p>
 */
class BUFFERED_DC_HELPER
{
public:
    BUFFERED_DC_HELPER( wxBufferedDC* aDC )
        : m_dc( aDC ) {}

    virtual ~BUFFERED_DC_HELPER()
    {
        if( m_dc )
        {
            m_dc->SetLogicalOrigin( 0, 0 );
            m_dc->SetUserScale( 1.0, 1.0 );
        }
    }

private:
    wxBufferedDC* m_dc;
};


/**
 * Class EDA_BLIT_NORMALIZER
 * is a helper class for clearing a device context scale and offset parameters before
 * performing a Blit operation.
 * <p>
 * This class keeps a temporary copy of the scale and offset parameters of a device
 * context and then restores them when it goes out of scope.
 * </p>
 */
class EDA_BLIT_NORMALIZER
{
public:
    EDA_BLIT_NORMALIZER( wxDC* aDC )
        : m_dc( aDC )
    {
        if( aDC )
        {
            aDC->GetUserScale( &m_userScaleX, &m_userScaleY );
            aDC->GetLogicalOrigin( &m_logicalOriginX, &m_logicalOriginY );
            aDC->GetDeviceOrigin( &m_deviceOriginX, &m_deviceOriginY );
            aDC->SetUserScale( 1.0, 1.0 );
            aDC->SetLogicalOrigin( 0, 0 );
            aDC->SetDeviceOrigin( 0, 0 );
        }
    }

    ~EDA_BLIT_NORMALIZER()
    {
        if( m_dc )
        {
            m_dc->SetUserScale( m_userScaleX, m_userScaleY );
            m_dc->SetLogicalOrigin( m_logicalOriginX, m_logicalOriginY );
            m_dc->SetDeviceOrigin( m_deviceOriginX, m_deviceOriginY );
        }
    }

private:
    wxDC* m_dc;
    double m_userScaleX;
    double m_userScaleY;
    int m_logicalOriginX;
    int m_logicalOriginY;
    int m_deviceOriginX;
    int m_deviceOriginY;

    DECLARE_NO_COPY_CLASS( EDA_BLIT_NORMALIZER )
};


#if USE_WX_GRAPHICS_CONTEXT
    #include <wx/dcgraph.h>
#endif

// Macro used to declare a device context in KiCad:
#if USE_WX_GRAPHICS_CONTEXT
//#pragma message( "INSTALL_DC is wxClientDC with wxGCDC" )
#define INSTALL_DC( name, parent )                       \
    wxClientDC _cDC( parent );                           \
    wxGCDC name( _cDC );                                 \
    parent->DoPrepareDC( name );                         \
    name.GetGraphicsContext()->Translate( 0.5, 0.5 );
#else
#if KICAD_USE_BUFFERED_DC && !KICAD_USE_BUFFERED_DC_HELPER
//#pragma message( "INSTALL_DC is wxClientDC with wxBufferedDC" )
#define INSTALL_DC( name, parent )                       \
    wxClientDC _cDC( parent );                           \
    wxBufferedDC name(&_cDC, _cDC.GetSize() );           \
    parent->DoPrepareDC( name );
#elif KICAD_USE_BUFFERED_DC && KICAD_USE_BUFFERED_DC_HELPER
//#pragma message( "INSTALL_DC is wxBufferedDC with BUFFERED_DC_HELPER" )
#define INSTALL_DC( name, parent )                       \
    wxClientDC _cDC( parent );                           \
    wxBufferedDC name(&_cDC, _cDC.GetSize() );           \
    parent->DoPrepareDC( name );                         \
    BUFFERED_DC_HELPER helper( &name );
#else
//#pragma message( "INSTALL_DC is wxClientDC" )
#define INSTALL_DC( name, parent )                       \
    wxClientDC name( parent );                           \
    parent->DoPrepareDC( name );
#endif
#endif

#if USE_WX_GRAPHICS_CONTEXT
//#pragma message( "INSTALL_PAINTDC is wxPaintDC with wxGCDC" )
#define INSTALL_PAINTDC( name, parent)                   \
    wxPaintDC _pDC( parent );                            \
    wxGCDC    name( _pDC );                              \
    parent->DoPrepareDC( name );                         \
    name.GetGraphicsContext()->Translate( 0.5, 0.5 );
#elif KICAD_USE_BUFFERED_PAINTDC && !KICAD_USE_BUFFERED_DC_HELPER
//#pragma message( "INSTALL_PAINTDC is wxAutoBufferedPaintDC" )
#define INSTALL_PAINTDC( name, parent )                  \
    wxAutoBufferedPaintDC name( parent );                \
    parent->DoPrepareDC( name );
#elif KICAD_USE_BUFFERED_PAINTDC && KICAD_USE_BUFFERED_DC_HELPER
//#pragma message( "INSTALL_PAINTDC is wxBufferedPaintDC with BUFFERED_DC_HELPER" )
#define INSTALL_PAINTDC( name, parent )                  \
    wxBufferedPaintDC name( parent );                    \
    parent->DoPrepareDC( name );                         \
    BUFFERED_DC_HELPER help( &name );
#else
//#pragma message( "INSTALL_PAINTDC is wxPaintDC" )
#define INSTALL_PAINTDC(name,parent)                     \
    wxPaintDC name( parent );                            \
    parent->DoPrepareDC( name );
#endif


// This macro should be used when drawing objects directly without drawing the background.
#define INSTALL_UNBUFFERED_DC( name, parent )            \
    wxClientDC name( parent );                           \
    parent->DoPrepareDC( name );


#endif // __KICAD_DEVICE_CONTEXT_H__
