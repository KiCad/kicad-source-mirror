/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "gerber_diff.h"
#include "gerber_to_png.h"
#include <plotters/plotter_png.h>
#include <base_units.h>
#include <cmath>
#include <json_common.h>


VECTOR2I CalculateAlignment( const SHAPE_POLY_SET& aReference, const SHAPE_POLY_SET& aComparison )
{
    if( aReference.OutlineCount() == 0 || aComparison.OutlineCount() == 0 )
        return VECTOR2I( 0, 0 );

    BOX2I refBBox = aReference.BBox();
    BOX2I cmpBBox = aComparison.BBox();

    // Align by bounding box origins
    return refBBox.GetOrigin() - cmpBBox.GetOrigin();
}


GERBER_DIFF_RESULT CalculateGerberDiff( const SHAPE_POLY_SET& aReference, const SHAPE_POLY_SET& aComparison )
{
    GERBER_DIFF_RESULT result;

    constexpr double IU2_TO_MM2 = 1e-10;

    // overlap = reference ∩ comparison
    result.overlap = aReference;
    result.overlap.BooleanIntersection( aComparison );

    // additions = comparison - reference
    result.additions = aComparison;
    result.additions.BooleanSubtract( aReference );

    // removals = reference - comparison
    result.removals = aReference;
    result.removals.BooleanSubtract( aComparison );

    result.overlapArea = result.overlap.Area() * IU2_TO_MM2;
    result.additionsArea = result.additions.Area() * IU2_TO_MM2;
    result.removalsArea = result.removals.Area() * IU2_TO_MM2;

    // reference = overlap ∪ removals, comparison = overlap ∪ additions (disjoint)
    result.referenceArea = result.overlapArea + result.removalsArea;
    result.comparisonArea = result.overlapArea + result.additionsArea;

    if( result.referenceArea > 0.0 )
    {
        result.additionsPercent = 100.0 * result.additionsArea / result.referenceArea;
        result.removalsPercent = 100.0 * result.removalsArea / result.referenceArea;
        result.netChangePercent = result.additionsPercent - result.removalsPercent;
    }

    // Fracture so that holes become bridge-connected outlines suitable for rendering
    // with PlotPoly (which only fills outlines, ignoring SHAPE_POLY_SET holes).
    result.overlap.Fracture();
    result.additions.Fracture();
    result.removals.Fracture();

    return result;
}


namespace
{

// Helper to convert mm^2 to other units
double ConvertArea( double aMm2, DIFF_UNITS aUnits )
{
    switch( aUnits )
    {
    case DIFF_UNITS::MM: return aMm2;

    case DIFF_UNITS::INCH: return aMm2 / ( 25.4 * 25.4 ); // mm^2 to in^2

    case DIFF_UNITS::MILS: return aMm2 / ( 0.0254 * 0.0254 ); // mm^2 to mils^2
    }

    return aMm2;
}


wxString GetUnitSuffix( DIFF_UNITS aUnits )
{
    switch( aUnits )
    {
    case DIFF_UNITS::MM: return wxS( "mm" );
    case DIFF_UNITS::INCH: return wxS( "in" );
    case DIFF_UNITS::MILS: return wxS( "mils" );
    }

    return wxS( "mm" );
}

} // anonymous namespace


wxString FormatDiffResultText( const GERBER_DIFF_RESULT& aResult, const wxString& aFile1, const wxString& aFile2,
                               DIFF_UNITS aUnits )
{
    wxString suffix = GetUnitSuffix( aUnits );
    wxString out;

    out += wxString::Format( wxS( "Comparing: %s vs %s\n\n" ), aFile1, aFile2 );

    out += wxString::Format( wxS( "Reference area:    %.2f %s^2\n" ), ConvertArea( aResult.referenceArea, aUnits ),
                             suffix );
    out += wxString::Format( wxS( "Comparison area:   %.2f %s^2\n" ), ConvertArea( aResult.comparisonArea, aUnits ),
                             suffix );
    out += wxString::Format( wxS( "Overlap area:      %.2f %s^2\n\n" ), ConvertArea( aResult.overlapArea, aUnits ),
                             suffix );

    wxString addSign = aResult.additionsPercent >= 0 ? wxS( "+" ) : wxS( "" );
    wxString remSign = aResult.removalsPercent > 0 ? wxS( "-" ) : wxS( "" );
    wxString netSign = aResult.netChangePercent >= 0 ? wxS( "+" ) : wxS( "" );

    out += wxString::Format( wxS( "Additions:         %.2f %s^2 (%s%.2f%% of reference)\n" ),
                             ConvertArea( aResult.additionsArea, aUnits ), suffix, addSign, aResult.additionsPercent );
    out += wxString::Format( wxS( "Removals:          %.2f %s^2 (%s%.2f%% of reference)\n" ),
                             ConvertArea( aResult.removalsArea, aUnits ), suffix, remSign, aResult.removalsPercent );
    out += wxString::Format( wxS( "Net change:        %s%.2f %s^2 (%s%.2f%%)\n" ), netSign,
                             ConvertArea( aResult.additionsArea - aResult.removalsArea, aUnits ), suffix, netSign,
                             aResult.netChangePercent );

    return out;
}


nlohmann::json FormatDiffResultJson( const GERBER_DIFF_RESULT& aResult, const wxString& aFile1, const wxString& aFile2,
                                     DIFF_UNITS aUnits, double aMaxDiffPercent )
{
    wxString unitStr = GetUnitSuffix( aUnits );

    nlohmann::json j;

    j["reference_file"] = aFile1.ToStdString();
    j["comparison_file"] = aFile2.ToStdString();
    j["units"] = unitStr.ToStdString();

    j["reference"] = { { "area", ConvertArea( aResult.referenceArea, aUnits ) } };

    j["comparison"] = { { "area", ConvertArea( aResult.comparisonArea, aUnits ) } };

    j["overlap"] = { { "area", ConvertArea( aResult.overlapArea, aUnits ) } };

    j["additions"] = { { "area", ConvertArea( aResult.additionsArea, aUnits ) },
                       { "percent", aResult.additionsPercent } };

    j["removals"] = { { "area", ConvertArea( aResult.removalsArea, aUnits ) }, { "percent", aResult.removalsPercent } };

    j["net_change"] = { { "area", ConvertArea( aResult.additionsArea - aResult.removalsArea, aUnits ) },
                        { "percent", aResult.netChangePercent } };

    // Determine if within threshold
    double totalDiffPercent = aResult.additionsPercent + aResult.removalsPercent;
    bool   withinThreshold =
            ( aMaxDiffPercent <= 0.0 ) ? ( totalDiffPercent == 0.0 ) : ( totalDiffPercent <= aMaxDiffPercent );

    j["within_threshold"] = withinThreshold;
    j["max_diff_percent"] = aMaxDiffPercent;
    j["total_diff_percent"] = totalDiffPercent;

    return j;
}


namespace
{

/**
 * Render a SHAPE_POLY_SET to the plotter with the given color.
 */
void RenderPolySet( PNG_PLOTTER& aPlotter, const SHAPE_POLY_SET& aPolySet, const KIGFX::COLOR4D& aColor )
{
    aPlotter.SetColor( aColor );

    for( int i = 0; i < aPolySet.OutlineCount(); i++ )
    {
        const std::vector<VECTOR2I>& pts = aPolySet.COutline( i ).CPoints();

        if( pts.size() >= 3 )
            aPlotter.PlotPoly( pts, FILL_T::FILLED_SHAPE, 0 );
    }
}

} // anonymous namespace


bool RenderDiffToPng( const GERBER_DIFF_RESULT& aResult, const wxString& aOutputPath,
                      const DIFF_RENDER_OPTIONS& aOptions )
{
    // Calculate combined bounding box
    BOX2I bbox;
    bool  first = true;

    auto mergeBBox = [&bbox, &first]( const SHAPE_POLY_SET& poly )
    {
        if( poly.OutlineCount() > 0 )
        {
            BOX2I polyBox = poly.BBox();

            if( first )
            {
                bbox = polyBox;
                first = false;
            }
            else
            {
                bbox.Merge( polyBox );
            }
        }
    };

    mergeBBox( aResult.overlap );
    mergeBBox( aResult.additions );
    mergeBBox( aResult.removals );

    if( first || bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
        return false;

    GERBER_PLOTTER_VIEWPORT vp = CalculatePlotterViewport( bbox, aOptions.dpi, aOptions.width, aOptions.height );

    PNG_PLOTTER plotter;
    plotter.SetColorMode( true );
    plotter.SetPixelSize( vp.width, vp.height );
    plotter.SetResolution( aOptions.dpi );
    plotter.SetAntialias( aOptions.antialias );
    plotter.SetBackgroundColor( aOptions.colorBackground );
    plotter.SetViewport( vp.offset, vp.iuPerDecimil, vp.plotScale, false );
    plotter.OpenFile( aOutputPath );

    if( !plotter.StartPlot( wxEmptyString ) )
        return false;

    // Render in order: overlap first, then additions, then removals
    // so removals (red) are most visible on top.
    RenderPolySet( plotter, aResult.overlap, aOptions.colorOverlap );
    RenderPolySet( plotter, aResult.additions, aOptions.colorAdditions );
    RenderPolySet( plotter, aResult.removals, aOptions.colorRemovals );

    return plotter.EndPlot();
}
