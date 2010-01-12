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

// Helper class to handle the client Device Context
class KicadGraphicContext : public wxClientDC
{
public:
    KicadGraphicContext( WinEDA_DrawPanel * aDrawPanel );
    ~KicadGraphicContext();
};


// Macro used to declare a device context in kicad:
#ifdef KICAD_USE_BUFFERED_DC

#define INSTALL_DC(name,parent) \
    KicadGraphicContext _cDC( parent );\
    wxBufferedDC name(&_cDC, _cDC.GetSize() );
#else
    #define INSTALL_DC(name,parent) KicadGraphicContext name( parent )
#endif

#ifdef KICAD_USE_BUFFERED_PAINTDC
    #define INSTALL_PAINTDC(name,parent) wxAutoBufferedPaintDC name(parent )
#else
    #define INSTALL_PAINTDC(name,parent) wxPaintDC name( parent )
#endif

#endif // __KICAD_DEVICE_CONTEXT_H__
