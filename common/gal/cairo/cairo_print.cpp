/*
 * Copyright (C) 2019 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gal/cairo/cairo_print.h>

#include <stdexcept>
#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/dcprint.h>

#ifdef NOMINMAX         /* workaround for gdiplus.h */
#include <algorithm>
using std::min;
using std::max;
#endif

#ifdef __WXMSW__
#include <windows.h>
#include <gdiplus.h>
#include <cairo-win32.h>
#include <wx/msw/enhmeta.h>
#endif /* __WXMSW__ */

#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#include <cairo-quartz.h>
#endif /* __WXMAC__ */

using namespace KIGFX;

CAIRO_PRINT_CTX::CAIRO_PRINT_CTX( wxDC* aDC )
    : m_gcdc( nullptr ), m_ctx( nullptr ), m_surface( nullptr )
{
    if( wxPrinterDC* printerDC = dynamic_cast<wxPrinterDC*>( aDC ) )
        m_gcdc = new wxGCDC( *printerDC );
    else if( wxMemoryDC* memoryDC = dynamic_cast<wxMemoryDC*>( aDC ) )
        m_gcdc = new wxGCDC( *memoryDC );
    else if( wxWindowDC* windowDC = dynamic_cast<wxWindowDC*>( aDC ) )
        m_gcdc = new wxGCDC( *windowDC );
#ifdef __WXMSW__
    else if( wxEnhMetaFileDC* enhMFDC = dynamic_cast<wxEnhMetaFileDC*>( aDC ) )
        m_gcdc = new wxGCDC( *enhMFDC );
#endif /* __WXMSW__ */
    else
        throw std::runtime_error( "Unhandled wxDC type" );

    wxGraphicsContext* gctx = m_gcdc->GetGraphicsContext();

    if( !gctx )
        throw std::runtime_error( "Could not get the Graphics Context" );

#ifdef __WXGTK__
    m_ctx = static_cast<cairo_t*>( gctx->GetNativeContext() );
    m_surface = cairo_get_target( m_ctx );
    // On linux, cairo printers are 72 DPI by default. This is an ususable resolution for us.
    // A reasonable resolution is 600 DPI
    // so modify the default:
    #define DEFAULT_DPI 72.0
    #define KICAD_PRINTER_DPI 600.0
    // our device scale is DEFAULT_DPI / KICAD_PRINTER_DPI
    cairo_surface_set_device_scale( m_surface, DEFAULT_DPI/KICAD_PRINTER_DPI, DEFAULT_DPI/KICAD_PRINTER_DPI );
    m_dpi = KICAD_PRINTER_DPI;
#endif /* __WXGTK__ */

#ifdef __WXMSW__
    Gdiplus::Graphics* g = static_cast<Gdiplus::Graphics*>( gctx->GetNativeContext() );
    m_hdc = g->GetHDC();
    m_surface = cairo_win32_printing_surface_create( static_cast<HDC>( m_hdc ) );
    m_ctx = cairo_create( m_surface );
    m_dpi = GetDeviceCaps( static_cast<HDC>( m_hdc ), LOGPIXELSX );
#endif /* __WXMSW__ */

#ifdef __WXMAC__
    wxSize size = m_gcdc->GetSize();
    CGContextRef cg = (CGContextRef) gctx->GetNativeContext();
    m_surface = cairo_quartz_surface_create_for_cg_context( cg, size.x, size.y );
    m_ctx = cairo_create( m_surface );
    wxASSERT( aDC->GetPPI().x == aDC->GetPPI().y );
    m_dpi = aDC->GetPPI().x;
#endif /* __WXMAC__ */

    if( !m_ctx || cairo_status( m_ctx ) != CAIRO_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create Cairo context" );

    if( !m_surface || cairo_surface_status( m_surface ) != CAIRO_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create Cairo surface" );

    cairo_reference( m_ctx );
    cairo_surface_reference( m_surface );
}


CAIRO_PRINT_CTX::~CAIRO_PRINT_CTX()
{
#ifdef __WXMSW__
    cairo_surface_show_page( m_surface );
    wxGraphicsContext* gctx = m_gcdc->GetGraphicsContext();
    Gdiplus::Graphics* g = static_cast<Gdiplus::Graphics*>( gctx->GetNativeContext() );
    g->ReleaseHDC( static_cast<HDC>( m_hdc ) );
#endif /* __WXMSW__ */

    cairo_surface_destroy( m_surface );
    cairo_destroy( m_ctx );
    delete m_gcdc;
}


CAIRO_PRINT_GAL::CAIRO_PRINT_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions,
        std::unique_ptr<CAIRO_PRINT_CTX> aContext )
    : CAIRO_GAL_BASE( aDisplayOptions )
{
    m_printCtx = std::move( aContext );
    context = currentContext = m_printCtx->GetContext();
    surface = m_printCtx->GetSurface();
    cairo_reference( context );
    cairo_surface_reference( surface );
    m_clearColor = COLOR4D( 1.0, 1.0, 1.0, 1.0 );
    m_hasNativeLandscapeRotation = false;
    resetContext();

    SetScreenDPI( m_printCtx->GetNativeDPI() );
}


void CAIRO_PRINT_GAL::SetLineWidth( float aLineWidth )
{
    storePath();
    lineWidth = aLineWidth;
    cairo_set_line_width( currentContext, aLineWidth );
}


void CAIRO_PRINT_GAL::ComputeWorldScreenMatrix()
{
    worldScale = screenDPI * worldUnitLength * zoomFactor;
    const auto paperSizeIU = VECTOR2D( m_nativePaperSize.y, m_nativePaperSize.x ) /* inches */ / worldUnitLength; /* 1 inch in IU */
    const auto paperSizeIUTransposed = VECTOR2D( paperSizeIU.y, paperSizeIU.x );

    MATRIX3x3D scale, translation, flip, rotate, lookat;

    scale.SetIdentity();
    translation.SetIdentity();
    flip.SetIdentity();
    rotate.SetIdentity();
    lookat.SetIdentity();

    if( m_hasNativeLandscapeRotation )
    {
        translation.SetTranslation( 0.5 / zoomFactor * paperSizeIUTransposed );
    }
    else
    {
        if( isLandscape() )
        {
            translation.SetTranslation( 0.5 / zoomFactor * paperSizeIU );
            rotate.SetRotation( 90.0 * M_PI / 180.0 );
        }
        else
        {
            translation.SetTranslation( 0.5 / zoomFactor * paperSizeIUTransposed );
        }
    }

    scale.SetScale( VECTOR2D( worldScale, worldScale ) );
    flip.SetScale( VECTOR2D( globalFlipX ? -1.0 : 1.0, globalFlipY ? -1.0 : 1.0 ) );
    lookat.SetTranslation( -lookAtPoint );

    worldScreenMatrix = scale * translation * flip * rotate * lookat;
    screenWorldMatrix = worldScreenMatrix.Inverse();
}


void CAIRO_PRINT_GAL::SetNativePaperSize( const VECTOR2D& aSize, bool aHasNativeLandscapeRotation )
{
    m_nativePaperSize = aSize;
    m_hasNativeLandscapeRotation = aHasNativeLandscapeRotation;
}


void CAIRO_PRINT_GAL::SetSheetSize( const VECTOR2D& aSize )
{
    // Convert aSize (inches) to pixels
    SetScreenSize( VECTOR2I( std::ceil( aSize.x * screenDPI ) * 2,
                std::ceil( aSize.y * screenDPI ) * 2 ) );
}


std::unique_ptr<GAL_PRINT> GAL_PRINT::Create( GAL_DISPLAY_OPTIONS& aOptions, wxDC* aDC )
{
    auto printCtx = std::make_unique<CAIRO_PRINT_CTX>( aDC );
    return std::make_unique<CAIRO_PRINT_GAL>( aOptions, std::move( printCtx ) );
}
