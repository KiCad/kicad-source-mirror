/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <diff_merge/diff_renderer_plotter.h>

#include <base_units.h>
#include <geometry/shape_poly_set.h>

#include <plotters/plotter.h>
#include <plotters/plotter_png.h>
#include <plotters/plotters_pslike.h>

#include <wx/file.h>
#include <wx/log.h>


namespace KICAD_DIFF
{

namespace
{

/**
 * Push shapes to a plotter as filled colored rectangles. The plotter is
 * assumed to be already configured (viewport, color mode, line width).
 */
void renderShapes( PLOTTER& aPlotter, const std::vector<SCENE_SHAPE>& aShapes )
{
    for( const SCENE_SHAPE& s : aShapes )
    {
        aPlotter.SetColor( s.color );

        if( !s.polygons.empty() )
        {
            SHAPE_POLY_SET set = PolySetFromPolygonList( s.polygons );

            if( set.OutlineCount() == 0 )
                continue;

            // Fracture turns holes into slits. The plotter fills each outline
            // independently and cannot subtract a separate hole contour.
            set.Fracture();

            for( int idx = 0; idx < set.OutlineCount(); ++idx )
                aPlotter.PlotPoly( set.Outline( idx ), FILL_T::FILLED_SHAPE, 0, nullptr );

            continue;
        }

        aPlotter.Rect( s.bbox.GetOrigin(), s.bbox.GetEnd(), FILL_T::FILLED_SHAPE, 0 );
    }
}


/**
 * Push one document's geometry to a PLOTTER. The plotter draws each polygon in
 * a single PlotPoly call (fill or stroke), so its work happens in the iterator's
 * fill pass and the stroke pass is a no-op. Same shape vocabulary as the GAL
 * path so headless PNG/SVG output stays visually consistent with the dialog.
 */
class PLOTTER_GEOMETRY_SINK : public GEOMETRY_SINK
{
public:
    explicit PLOTTER_GEOMETRY_SINK( PLOTTER& aPlotter ) : m_plotter( aPlotter ) {}

    void FillPolygon( const DOCUMENT_POLYGON& aPoly ) override
    {
        if( aPoly.outline.size() < 3 )
            return;

        m_plotter.SetColor( aPoly.color );

        std::vector<VECTOR2I> pts( aPoly.outline.begin(), aPoly.outline.end() );
        m_plotter.PlotPoly( pts, aPoly.filled ? FILL_T::FILLED_SHAPE : FILL_T::NO_FILL,
                            EffectivePlotWidth( aPoly.lineWidth ), nullptr );
    }

    void StrokePolygon( const DOCUMENT_POLYGON& ) override {}

    void DrawSegment( const DOCUMENT_SEGMENT& aSegment ) override
    {
        m_plotter.SetColor( aSegment.color );
        m_plotter.ThickSegment( aSegment.start, aSegment.end, EffectivePlotWidth( aSegment.width ),
                                nullptr );
    }

    void DrawCircle( const DOCUMENT_CIRCLE& aCircle ) override
    {
        m_plotter.SetColor( aCircle.color );
        m_plotter.Circle( aCircle.center, aCircle.radius * 2,
                          aCircle.filled ? FILL_T::FILLED_SHAPE : FILL_T::NO_FILL,
                          EffectivePlotWidth( aCircle.lineWidth ) );
    }

private:
    PLOTTER& m_plotter;
};


void renderDocumentGeometry( PLOTTER& aPlotter, const DOCUMENT_GEOMETRY& aGeometry )
{
    PLOTTER_GEOMETRY_SINK sink( aPlotter );
    IterateDocumentGeometry( aGeometry, sink );
}


/**
 * Compute the viewport so the scene fills the output image at the requested
 * DPI, with a 5% margin around the union bbox.
 */
struct PIXEL_VIEWPORT
{
    int      pixelWidth = 0;
    int      pixelHeight = 0;
    double   plotScale = 0.0;
    VECTOR2I offsetIU;
    double   iuPerDecimil = 0.0;
};


PIXEL_VIEWPORT computeViewport( const BOX2I& aBBox, const EDA_IU_SCALE& aIuScale, int aDpi,
                                int aRequestedW, int aRequestedH )
{
    PIXEL_VIEWPORT vp;

    // 5% margin around the bbox.
    constexpr double MARGIN = 0.05;

    BOX2I padded = aBBox;
    int   padX   = static_cast<int>( aBBox.GetWidth() * MARGIN );
    int   padY   = static_cast<int>( aBBox.GetHeight() * MARGIN );
    padded.Inflate( std::max( padX, 1 ), std::max( padY, 1 ) );

    // Convert the bbox extent to millimetres using the source document's scale.
    // PCB/footprint IU is 1 nm; schematic/symbol IU is 100 nm. The aspect ratio
    // is scale-independent but the absolute mm extent (used for auto-sizing) is
    // not, so the per-document scale must be used here too. Divide by IU_PER_MM
    // rather than multiply by MM_PER_IU so the PCB branch reproduces the original
    // `/1e6` arithmetic bit-for-bit (MM_PER_IU is its rounded reciprocal).
    double widthMM  = padded.GetWidth()  / aIuScale.IU_PER_MM;
    double heightMM = padded.GetHeight() / aIuScale.IU_PER_MM;

    // Default output: scale so the longer dimension renders at 1024 px,
    // unless caller supplied explicit pixel dimensions.
    int outW = aRequestedW;
    int outH = aRequestedH;

    if( outW <= 0 && outH <= 0 )
    {
        constexpr int TARGET_PX = 1024;
        double        aspect    = widthMM / std::max( heightMM, 0.001 );

        if( aspect >= 1.0 )
        {
            outW = TARGET_PX;
            outH = static_cast<int>( TARGET_PX / aspect );
        }
        else
        {
            outH = TARGET_PX;
            outW = static_cast<int>( TARGET_PX * aspect );
        }
    }
    else if( outW <= 0 )
    {
        outW = static_cast<int>( outH * widthMM / std::max( heightMM, 0.001 ) );
    }
    else if( outH <= 0 )
    {
        outH = static_cast<int>( outW * heightMM / std::max( widthMM, 0.001 ) );
    }

    vp.pixelWidth  = std::max( outW, 1 );
    vp.pixelHeight = std::max( outH, 1 );

    // PNG_PLOTTER / SVG_PLOTTER expect iuPerDecimil = the document's internal-
    // unit-per-decimil ratio. PCB/footprint and schematic/symbol documents carry
    // different IU scales, so this must follow the source document or schematic
    // output is silently mis-scaled by the PCB ratio.
    vp.iuPerDecimil = aIuScale.IU_PER_MILS / 10.0;

    // plotScale is the user-zoom factor; 1.0 means the bbox should fit the
    // output canvas at the iuPerDecimil ratio. The plotter handles the
    // remaining math against pixelWidth/Height.
    vp.plotScale = 1.0;
    vp.offsetIU  = padded.GetOrigin();

    (void) aDpi;   // dpi only affects raster output via the plotter's resolution setter

    return vp;
}

} // namespace


/**
 * Write a tiny placeholder PNG (single empty pixel) so a "no differences"
 * diff still produces a valid output file. Used when the scene has no shapes
 * to render. The file is the smallest valid PNG that displays as transparent
 * (or theme background in viewers that fill).
 */
static bool writeEmptyPng( const wxString& aOutputPath )
{
    static const unsigned char EMPTY_PNG[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
        0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
        0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41,
        0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
        0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
        0x42, 0x60, 0x82
    };

    wxFile out( aOutputPath, wxFile::write );

    if( !out.IsOpened() )
        return false;

    return out.Write( EMPTY_PNG, sizeof( EMPTY_PNG ) ) == sizeof( EMPTY_PNG );
}


static bool writeEmptySvg( const wxString& aOutputPath )
{
    static const char EMPTY_SVG[] =
            "<?xml version=\"1.0\" standalone=\"no\"?>\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1\" height=\"1\"/>\n";

    wxFile out( aOutputPath, wxFile::write );

    if( !out.IsOpened() )
        return false;

    return out.Write( EMPTY_SVG, sizeof( EMPTY_SVG ) - 1 )
                   == sizeof( EMPTY_SVG ) - 1;
}


bool RenderSceneToPng( const DIFF_SCENE& aScene, const wxString& aOutputPath,
                       const PLOTTER_RENDER_OPTIONS& aOptions )
{
    // Caller may have populated geometry without growing the change-derived
    // documentBBox to match. Use a copy that includes the geometry extent.
    DIFF_SCENE scene = aScene;
    ExpandBBoxToGeometry( scene );

    if( scene.documentBBox.GetWidth() <= 0 || scene.documentBBox.GetHeight() <= 0 )
    {
        // No drawable extent — identical inputs and no geometry. Emit a
        // placeholder so the file exists and the CLI exit code (success)
        // matches "no changes".
        return writeEmptyPng( aOutputPath );
    }

    PIXEL_VIEWPORT vp = computeViewport( scene.documentBBox, IuScaleForDocKind( scene.docKind ),
                                         aOptions.dpi, aOptions.pixelWidth, aOptions.pixelHeight );

    PNG_PLOTTER plotter;
    plotter.SetColorMode( true );
    plotter.SetPixelSize( vp.pixelWidth, vp.pixelHeight );
    plotter.SetResolution( aOptions.dpi );
    plotter.SetAntialias( aOptions.antialias );
    plotter.SetBackgroundColor( aOptions.theme.background );
    plotter.SetViewport( vp.offsetIU, vp.iuPerDecimil, vp.plotScale, false );

    if( !plotter.OpenFile( aOutputPath ) )
        return false;

    if( !plotter.StartPlot( wxEmptyString ) )
        return false;

    renderDocumentGeometry( plotter, scene.referenceGeometry );
    renderDocumentGeometry( plotter, scene.comparisonGeometry );

    for( CATEGORY cat : PAINT_ORDER )
        renderShapes( plotter, ShapesFor( scene, cat ) );

    plotter.EndPlot();
    return true;
}


bool RenderSceneToSvg( const DIFF_SCENE& aScene, const wxString& aOutputPath,
                       const PLOTTER_RENDER_OPTIONS& aOptions )
{
    DIFF_SCENE scene = aScene;
    ExpandBBoxToGeometry( scene );

    if( scene.documentBBox.GetWidth() <= 0 || scene.documentBBox.GetHeight() <= 0 )
        return writeEmptySvg( aOutputPath );

    PIXEL_VIEWPORT vp = computeViewport( scene.documentBBox, IuScaleForDocKind( scene.docKind ),
                                         aOptions.dpi, aOptions.pixelWidth, aOptions.pixelHeight );

    SVG_PLOTTER plotter;
    plotter.SetColorMode( true );
    plotter.SetViewport( vp.offsetIU, vp.iuPerDecimil, vp.plotScale, false );

    if( !plotter.OpenFile( aOutputPath ) )
        return false;

    if( !plotter.StartPlot( wxEmptyString ) )
        return false;

    renderDocumentGeometry( plotter, scene.referenceGeometry );
    renderDocumentGeometry( plotter, scene.comparisonGeometry );

    for( CATEGORY cat : PAINT_ORDER )
        renderShapes( plotter, ShapesFor( scene, cat ) );

    plotter.EndPlot();
    return true;
}

} // namespace KICAD_DIFF
