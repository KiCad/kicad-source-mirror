/**
 * a helper to handle the real device context used in kicad
 * @file kicad_device_context.h
 */

#ifndef __KICAD_DEVICE_CONTEXT_H__
#define __KICAD_DEVICE_CONTEXT_H__

// Comment this line to use the standard wxClientDC
// and uncomment to use buffered DC
// #define KICAD_USE_BUFFERED_DC    // Currently under test

// Comment this line to use the standard wxPaintDC
// and uncomment to use buffered PaintDC
#define KICAD_USE_BUFFERED_PAINTDC    // Currently under test

#if defined KICAD_USE_BUFFERED_DC || defined KICAD_USE_BUFFERED_PAINTDC
    #ifndef KICAD_USE_BUFFERED_PAINTDC
    #define KICAD_USE_BUFFERED_PAINTDC
    #endif
    #include <wx/dcbuffer.h>
#endif

#if USE_WX_GRAPHICS_CONTEXT && USE_WX_ZOOM
    #include <wx/dcgraph.h>
#endif

// Macro used to declare a device context in kicad:
#if USE_WX_GRAPHICS_CONTEXT && USE_WX_ZOOM
//#pragma message( "INSTALL_DC is wxClientDC with wxGCDC" )
#define INSTALL_DC(name,parent)                          \
    wxClientDC _cDC( parent );                           \
    wxGCDC name(_cDC);                                   \
    parent->DoPrepareDC( name );                         \
    name.GetGraphicsContext()->Translate( 0.5, 0.5 );
#else
#ifdef KICAD_USE_BUFFERED_DC
//#pragma message( "INSTALL_DC is wxClientDC with wxBufferedDC" )
#define INSTALL_DC(name,parent)                          \
    wxClientDC _cDC( parent );                           \
    wxBufferedDC name(&_cDC, _cDC.GetSize() );           \
    parent->DoPrepareDC( name );
#else
//#pragma message( "INSTALL_DC is wxClientDC" )
#define INSTALL_DC(name,parent)                          \
    wxClientDC name( parent );                           \
    parent->DoPrepareDC( name );
#endif
#endif

#if USE_WX_GRAPHICS_CONTEXT
//#pragma message( "INSTALL_PAINTDC is wxPaintDC with wxGCDC" )
#define INSTALL_PAINTDC(name,parent)                     \
    wxPaintDC _pDC(parent);                              \
    wxGCDC    name(_pDC);                                \
    parent->DoPrepareDC( name );                         \
    name.GetGraphicsContext()->Translate( 0.5, 0.5 );
#elif !defined( USE_WX_ZOOM ) && defined( KICAD_USE_BUFFERED_PAINTDC )
//#pragma message( "INSTALL_PAINTDC is wxAutoBufferedPaintDC" )
#define INSTALL_PAINTDC(name,parent)                     \
    wxAutoBufferedPaintDC name(parent );                 \
    parent->DoPrepareDC( name );
#else
//#pragma message( "INSTALL_PAINTDC is wxPaintDC" )
#define INSTALL_PAINTDC(name,parent)                     \
    wxPaintDC name( parent );                            \
    parent->DoPrepareDC( name );
#endif

#endif // __KICAD_DEVICE_CONTEXT_H__
