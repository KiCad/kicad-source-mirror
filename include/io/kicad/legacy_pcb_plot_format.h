/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <io/kicad/legacy_format.h>
#include <pcb_plot_params.h>

namespace KICAD_FORMAT::LEGACY
{

// PCB plot grammar from 9.0.0 ccafeabf1503 and 10.0.7 93a8a4827b0b.

inline std::string FormatLayerSet( const LSET& aLayers )
{
    for( size_t bit = 128; bit < aLayers.size(); ++bit )
    {
        if( aLayers.test( bit ) )
            THROW_IO_ERROR( wxT( "Unreviewed plot layer in a frozen writer." ) );
    }

    std::string    result;
    constexpr char hex[] = "0123456789abcdef";

    for( int nibble = 31; nibble >= 0; --nibble )
    {
        if( nibble != 31 && ( nibble + 1 ) % 8 == 0 )
            result += '_';

        int value = 0;

        for( int bit = 0; bit < 4; ++bit )
        {
            const size_t index = nibble * 4 + bit;

            if( index < aLayers.size() && aLayers.test( index ) )
                value |= 1 << bit;
        }

        result += hex[value];
    }

    return result;
}


inline int PlotFormatValue( PLOT_FORMAT aFormat )
{
    switch( aFormat )
    {
    case PLOT_FORMAT::UNDEFINED: return -1;
    case PLOT_FORMAT::HPGL: return 0;
    case PLOT_FORMAT::GERBER: return 1;
    case PLOT_FORMAT::POST: return 2;
    case PLOT_FORMAT::DXF: return 3;
    case PLOT_FORMAT::PDF: return 4;
    case PLOT_FORMAT::SVG: return 5;
    default: THROW_IO_ERROR( wxT( "Unsupported plot format in a frozen writer." ) );
    }
}


inline void FormatPlot( OUTPUTFORMATTER* aOut, const PCB_PLOT_PARAMS& aPlot, bool aV9 )
{
    const int format = PlotFormatValue( aPlot.GetFormat() );
    LSET      commonLayers;

    for( PCB_LAYER_ID layer : aPlot.GetPlotOnAllLayersSequence() )
        commonLayers.set( layer );

    aOut->Print( "(pcbplotparams(layerselection 0x%s)", FormatLayerSet( aPlot.GetLayerSelection() ).c_str() );
    aOut->Print( "(plot_on_all_layers_selection 0x%s)", FormatLayerSet( commonLayers ).c_str() );
    FormatBool( aOut, "disableapertmacros", aPlot.GetDisableGerberMacros() );
    FormatBool( aOut, "usegerberextensions", aPlot.GetUseGerberProtelExtensions() );
    FormatBool( aOut, "usegerberattributes", aPlot.GetUseGerberX2format() );
    FormatBool( aOut, "usegerberadvancedattributes", aPlot.GetIncludeGerberNetlistInfo() );
    FormatBool( aOut, "creategerberjobfile", aPlot.GetCreateGerberJobFile() );

    if( aPlot.GetGerberPrecision() != 6 )
        aOut->Print( "(gerberprecision %d)", aPlot.GetGerberPrecision() );

    if( aV9 )
    {
        aOut->Print( "(dashed_line_dash_ratio %f)", aPlot.GetDashedLineDashRatio() );
        aOut->Print( "(dashed_line_gap_ratio %f)", aPlot.GetDashedLineGapRatio() );
    }
    else
    {
        aOut->Print( "(dashed_line_dash_ratio %s)", FormatDouble2Str( aPlot.GetDashedLineDashRatio() ).c_str() );
        aOut->Print( "(dashed_line_gap_ratio %s)", FormatDouble2Str( aPlot.GetDashedLineGapRatio() ).c_str() );
    }

    aOut->Print( "(svgprecision %d)", aPlot.GetSvgPrecision() );
    FormatBool( aOut, "plotframeref", aPlot.GetPlotFrameRef() );
    aOut->Print( "(mode %d)", aPlot.GetDXFPlotMode() == SKETCH ? 2 : 1 );
    FormatBool( aOut, "useauxorigin", aPlot.GetUseAuxOrigin() );

    if( aV9 )
    {
        // Removed model preferences use the pinned release's defaults.
        aOut->Print( "(hpglpennumber 1)(hpglpenspeed 20)(hpglpendiameter %f)", 15.0 );
    }

    FormatBool( aOut, "pdf_front_fp_property_popups", aPlot.m_PDFFrontFPPropertyPopups );
    FormatBool( aOut, "pdf_back_fp_property_popups", aPlot.m_PDFBackFPPropertyPopups );
    FormatBool( aOut, "pdf_metadata", aPlot.m_PDFMetadata );
    FormatBool( aOut, "pdf_single_document", aPlot.m_PDFSingle );
    FormatBool( aOut, "dxfpolygonmode", aPlot.GetDXFPlotPolygonMode() );
    FormatBool( aOut, "dxfimperialunits", aPlot.GetDXFPlotUnits() == DXF_UNITS::INCH );
    FormatBool( aOut, "dxfusepcbnewfont", aPlot.GetTextMode() != PLOT_TEXT_MODE::NATIVE );
    FormatBool( aOut, "psnegative", aPlot.GetNegative() );
    FormatBool( aOut, "psa4output", aPlot.GetA4Output() );
    FormatBool( aOut, "plot_black_and_white", aPlot.GetBlackAndWhite() );

    if( aV9 )
        FormatBool( aOut, "plotinvisibletext", false );

    FormatBool( aOut, "sketchpadsonfab", aPlot.GetSketchPadsOnFabLayers() );
    FormatBool( aOut, "plotpadnumbers", aPlot.GetPlotPadNumbers() );
    FormatBool( aOut, "hidednponfab", aPlot.GetHideDNPFPsOnFabLayers() );
    FormatBool( aOut, "sketchdnponfab", aPlot.GetSketchDNPFPsOnFabLayers() );
    FormatBool( aOut, "crossoutdnponfab", aPlot.GetCrossoutDNPFPsOnFabLayers() );
    FormatBool( aOut, "subtractmaskfromsilk", aPlot.GetSubtractMaskFromSilk() );
    aOut->Print( "(outputformat %d)", format );
    FormatBool( aOut, "mirror", aPlot.GetMirror() );
    aOut->Print( "(drillshape %d)", static_cast<int>( aPlot.GetDrillMarksType() ) );
    aOut->Print( "(scaleselection %d)", aPlot.GetScaleSelection() );
    aOut->Print( "(outputdirectory %s))", aOut->Quotew( aPlot.GetOutputDirectory() ).c_str() );
}


inline void FormatPlotV9( OUTPUTFORMATTER* aOut, const PCB_PLOT_PARAMS& aPlot )
{
    FormatPlot( aOut, aPlot, true );
}


inline void FormatPlotV10( OUTPUTFORMATTER* aOut, const PCB_PLOT_PARAMS& aPlot )
{
    FormatPlot( aOut, aPlot, false );
}

} // namespace KICAD_FORMAT::LEGACY
