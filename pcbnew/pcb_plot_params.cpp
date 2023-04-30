/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board_design_settings.h>
#include <charconv>
#include <layer_ids.h>
#include <macros.h>
#include <math/util.h> // for KiROUND
#include <pcb_plot_params.h>
#include <pcb_plot_params_parser.h>
#include <plotters/plotter.h>
#include <settings/color_settings.h>


#define PLOT_LINEWIDTH_DEFAULT    ( DEFAULT_TEXT_WIDTH * IU_PER_MM )

#define HPGL_PEN_DIAMETER_MIN     0
#define HPGL_PEN_DIAMETER_MAX     100.0     // Unit = mil
#define HPGL_PEN_SPEED_MIN        1         // this param is always in cm/s
#define HPGL_PEN_SPEED_MAX        99        // this param is always in cm/s
#define HPGL_PEN_NUMBER_MIN       1
#define HPGL_PEN_NUMBER_MAX       16

#define SVG_PRECISION_MIN         3U
#define SVG_PRECISION_MAX         6U
#define SVG_PRECISION_DEFAULT     4


// default trailing digits in Gerber coordinates, when units are mm
// This is also the max usable precision (i.e. internal Pcbnew Units)
static const int gbrDefaultPrecision = 6;


using namespace PCBPLOTPARAMS_T;


static const char* getTokenName( T aTok )
{
    return PCB_PLOT_PARAMS_LEXER::TokenName( aTok );
}


static bool setInt( int* aTarget, int aValue, int aMin, int aMax )
{
    int temp = aValue;

    if( aValue < aMin )
        temp = aMin;
    else if( aValue > aMax )
        temp = aMax;

    *aTarget = temp;
    return ( temp == aValue );
}


static bool setDouble( double* aTarget, double aValue, double aMin, double aMax )
{
    double temp = aValue;

    if( aValue < aMin )
        temp = aMin;
    else if( aValue > aMax )
        temp = aMax;

    *aTarget = temp;
    return ( temp == aValue );
}


PCB_PLOT_PARAMS::PCB_PLOT_PARAMS()
{
    m_useGerberProtelExtensions  = false;
    m_gerberDisableApertMacros   = false;
    m_useGerberX2format          = true;
    m_includeGerberNetlistInfo   = true;
    m_createGerberJobFile        = true;
    m_gerberPrecision            = gbrDefaultPrecision;
    m_dashedLineDashRatio        = 12.0;   // From ISO 128-2
    m_dashedLineGapRatio         = 3.0;    // From ISO 128-2

    // we used 0.1mils for SVG step before, but nm precision is more accurate, so we use nm
    m_svgPrecision               = SVG_PRECISION_DEFAULT;
    m_plotDrawingSheet = false;
    m_plotViaOnMaskLayer         = false;
    m_plotMode                   = FILLED;
    m_DXFPolygonMode = true;
    m_DXFUnits = DXF_UNITS::INCHES;
    m_useAuxOrigin               = false;
    m_HPGLPenNum                 = 1;
    m_HPGLPenSpeed               = 20;        // this param is always in cm/s
    m_HPGLPenDiam                = 15;        // in mils
    m_negative                   = false;
    m_A4Output                   = false;
    m_plotReference              = true;
    m_plotValue                  = true;
    m_plotInvisibleText          = false;
    m_sketchPadsOnFabLayers      = false;
    m_subtractMaskFromSilk       = false;
    m_format                     = PLOT_FORMAT::GERBER;
    m_mirror                     = false;
    m_drillMarks                 = DRILL_MARKS::SMALL_DRILL_SHAPE;
    m_autoScale                  = false;
    m_scale                      = 1.0;
    m_scaleSelection             = 1;
    m_fineScaleAdjustX           = 1.0;
    m_fineScaleAdjustY           = 1.0;
    m_widthAdjust                = 0.;
    m_textMode                   = PLOT_TEXT_MODE::DEFAULT;
    m_outputDirectory.clear();
    m_layerSelection             = LSET( 7, F_SilkS, B_SilkS, F_Mask, B_Mask,
                                         F_Paste, B_Paste, Edge_Cuts )
                                         | LSET::AllCuMask();

    m_PDFFrontFPPropertyPopups   = true;
    m_PDFBackFPPropertyPopups    = true;

    // This parameter controls if the NPTH pads will be plotted or not
    // it is a "local" parameter
    m_skipNPTH_Pads              = false;

    // line width to plot items in outline mode.
    m_sketchPadLineWidth         = pcbIUScale.mmToIU( 0.1 );

    m_default_colors = std::make_shared<COLOR_SETTINGS>();
    m_colors         = m_default_colors.get();

    m_blackAndWhite = true;
}


void PCB_PLOT_PARAMS::SetGerberPrecision( int aPrecision )
{
    // Currently Gerber files use mm.
    // accepted precision is only 6 (max value, this is the resolution of Pcbnew)
    // or 5, min value for professional boards, when 6 creates problems
    // to board makers.

    m_gerberPrecision = aPrecision == gbrDefaultPrecision-1 ? gbrDefaultPrecision-1 :
                                      gbrDefaultPrecision;
}


void PCB_PLOT_PARAMS::SetSvgPrecision( unsigned aPrecision )
{
    m_svgPrecision = Clamp( SVG_PRECISION_MIN, aPrecision, SVG_PRECISION_MAX );
}


void PCB_PLOT_PARAMS::Format( OUTPUTFORMATTER* aFormatter,
                              int aNestLevel, int aControl ) const
{
    auto printBool =
            []( bool aBool ) -> const char*
            {
                return aBool ? "true" : "false";
            };

    aFormatter->Print( aNestLevel, "(pcbplotparams\n" );

    aFormatter->Print( aNestLevel+1, "(layerselection 0x%s)\n",
                       m_layerSelection.FmtHex().c_str() );

    aFormatter->Print( aNestLevel+1, "(plot_on_all_layers_selection 0x%s)\n",
                       m_plotOnAllLayersSelection.FmtHex().c_str() );

    aFormatter->Print( aNestLevel+1, "(disableapertmacros %s)\n",
                       printBool( m_gerberDisableApertMacros ) );

    aFormatter->Print( aNestLevel+1, "(usegerberextensions %s)\n",
                       printBool( m_useGerberProtelExtensions) );

    aFormatter->Print( aNestLevel+1, "(usegerberattributes %s)\n",
                       printBool( GetUseGerberX2format()) );

    aFormatter->Print( aNestLevel+1, "(usegerberadvancedattributes %s)\n",
                       printBool( GetIncludeGerberNetlistInfo()) );

    aFormatter->Print( aNestLevel+1, "(creategerberjobfile %s)\n",
                       printBool( GetCreateGerberJobFile()) );

    // save this option only if it is not the default value,
    // to avoid incompatibility with older Pcbnew version
    if( m_gerberPrecision != gbrDefaultPrecision )
        aFormatter->Print( aNestLevel+1, "(gerberprecision %d)\n", m_gerberPrecision );

    aFormatter->Print( aNestLevel+1, "(dashed_line_dash_ratio %f)\n", GetDashedLineDashRatio() );
    aFormatter->Print( aNestLevel+1, "(dashed_line_gap_ratio %f)\n", GetDashedLineGapRatio() );

    // SVG options
    aFormatter->Print( aNestLevel+1, "(svgprecision %d)\n", m_svgPrecision );

    aFormatter->Print( aNestLevel+1, "(plotframeref %s)\n", printBool( m_plotDrawingSheet ) );
    aFormatter->Print( aNestLevel+1, "(viasonmask %s)\n", printBool( m_plotViaOnMaskLayer ) );
    aFormatter->Print( aNestLevel+1, "(mode %d)\n", GetPlotMode() == SKETCH ? 2 : 1 );
    aFormatter->Print( aNestLevel+1, "(useauxorigin %s)\n", printBool( m_useAuxOrigin ) );

    // HPGL options
    aFormatter->Print( aNestLevel+1, "(hpglpennumber %d)\n", m_HPGLPenNum );
    aFormatter->Print( aNestLevel+1, "(hpglpenspeed %d)\n", m_HPGLPenSpeed );
    aFormatter->Print( aNestLevel+1, "(hpglpendiameter %f)\n", m_HPGLPenDiam );

    // PDF options
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_pdf_front_fp_property_popups ),
                       printBool( m_PDFFrontFPPropertyPopups ) );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_pdf_back_fp_property_popups ),
                       printBool( m_PDFBackFPPropertyPopups ) );

    // DXF options
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_dxfpolygonmode ),
                       printBool( m_DXFPolygonMode ) );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_dxfimperialunits ),
                       printBool( m_DXFUnits == DXF_UNITS::INCHES ) );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_dxfusepcbnewfont ),
                       printBool( m_textMode != PLOT_TEXT_MODE::NATIVE ) );

    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_psnegative ),
                       printBool( m_negative ) );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_psa4output ),
                       printBool( m_A4Output ) );
    aFormatter->Print( aNestLevel+1, "(plotreference %s)\n", printBool( m_plotReference ) );
    aFormatter->Print( aNestLevel+1, "(plotvalue %s)\n", printBool( m_plotValue ) );
    aFormatter->Print( aNestLevel+1, "(plotinvisibletext %s)\n", printBool( m_plotInvisibleText ) );
    aFormatter->Print( aNestLevel+1, "(sketchpadsonfab %s)\n",
                       printBool( m_sketchPadsOnFabLayers ) );
    aFormatter->Print( aNestLevel+1, "(subtractmaskfromsilk %s)\n",
                       printBool( m_subtractMaskFromSilk ) );
    aFormatter->Print( aNestLevel+1, "(outputformat %d)\n", static_cast<int>( m_format ) );
    aFormatter->Print( aNestLevel+1, "(mirror %s)\n", printBool( m_mirror ) );
    aFormatter->Print( aNestLevel+1, "(drillshape %d)\n", (int)m_drillMarks );
    aFormatter->Print( aNestLevel+1, "(scaleselection %d)\n", m_scaleSelection );
    aFormatter->Print( aNestLevel+1, "(outputdirectory \"%s\")",
                       (const char*) m_outputDirectory.utf8_str() );
    aFormatter->Print( 0, "\n" );
    aFormatter->Print( aNestLevel, ")\n" );
}


void PCB_PLOT_PARAMS::Parse( PCB_PLOT_PARAMS_PARSER* aParser )
{
    aParser->Parse( this );
}


bool PCB_PLOT_PARAMS::IsSameAs( const PCB_PLOT_PARAMS &aPcbPlotParams ) const
{
    if( m_layerSelection != aPcbPlotParams.m_layerSelection )
        return false;

    if( m_plotOnAllLayersSelection != aPcbPlotParams.m_plotOnAllLayersSelection )
        return false;

    if( m_useGerberProtelExtensions != aPcbPlotParams.m_useGerberProtelExtensions )
        return false;

    if( m_gerberDisableApertMacros != aPcbPlotParams.m_gerberDisableApertMacros )
        return false;

    if( m_useGerberX2format != aPcbPlotParams.m_useGerberX2format )
        return false;

    if( m_includeGerberNetlistInfo != aPcbPlotParams.m_includeGerberNetlistInfo )
        return false;

    if( m_createGerberJobFile != aPcbPlotParams.m_createGerberJobFile )
        return false;

    if( m_gerberPrecision != aPcbPlotParams.m_gerberPrecision )
        return false;

    if( m_dashedLineDashRatio != aPcbPlotParams.m_dashedLineDashRatio )
        return false;

    if( m_dashedLineGapRatio != aPcbPlotParams.m_dashedLineGapRatio )
        return false;

    if( m_plotDrawingSheet != aPcbPlotParams.m_plotDrawingSheet )
        return false;

    if( m_plotViaOnMaskLayer != aPcbPlotParams.m_plotViaOnMaskLayer )
        return false;

    if( m_plotMode != aPcbPlotParams.m_plotMode )
        return false;

    if( m_DXFPolygonMode != aPcbPlotParams.m_DXFPolygonMode )
        return false;

    if( m_DXFUnits != aPcbPlotParams.m_DXFUnits )
        return false;

    if( m_svgPrecision != aPcbPlotParams.m_svgPrecision )
        return false;

    if( m_useAuxOrigin != aPcbPlotParams.m_useAuxOrigin )
        return false;

    if( m_HPGLPenNum != aPcbPlotParams.m_HPGLPenNum )
        return false;

    if( m_HPGLPenSpeed != aPcbPlotParams.m_HPGLPenSpeed )
        return false;

    if( m_HPGLPenDiam != aPcbPlotParams.m_HPGLPenDiam )
        return false;

    if( m_negative != aPcbPlotParams.m_negative )
        return false;

    if( m_PDFFrontFPPropertyPopups != aPcbPlotParams.m_PDFFrontFPPropertyPopups )
        return false;

    if( m_PDFBackFPPropertyPopups != aPcbPlotParams.m_PDFBackFPPropertyPopups )
        return false;

    if( m_A4Output != aPcbPlotParams.m_A4Output )
        return false;

    if( m_plotReference != aPcbPlotParams.m_plotReference )
        return false;

    if( m_plotValue != aPcbPlotParams.m_plotValue )
        return false;

    if( m_plotInvisibleText != aPcbPlotParams.m_plotInvisibleText )
        return false;

    if( m_sketchPadsOnFabLayers != aPcbPlotParams.m_sketchPadsOnFabLayers )
        return false;

    if( m_subtractMaskFromSilk != aPcbPlotParams.m_subtractMaskFromSilk )
        return false;

    if( m_format != aPcbPlotParams.m_format )
        return false;

    if( m_mirror != aPcbPlotParams.m_mirror )
        return false;

    if( m_drillMarks != aPcbPlotParams.m_drillMarks )
        return false;

    if( m_scaleSelection != aPcbPlotParams.m_scaleSelection )
        return false;

    if( m_autoScale != aPcbPlotParams.m_autoScale )
        return false;

    if( m_scale != aPcbPlotParams.m_scale )
        return false;

    if( m_fineScaleAdjustX != aPcbPlotParams.m_fineScaleAdjustX )
        return false;

    if( m_fineScaleAdjustY != aPcbPlotParams.m_fineScaleAdjustY )
        return false;

    if( m_widthAdjust != aPcbPlotParams.m_widthAdjust )
        return false;

    if( m_textMode != aPcbPlotParams.m_textMode )
        return false;

    if( m_blackAndWhite != aPcbPlotParams.m_blackAndWhite )
        return false;

    if( !m_outputDirectory.IsSameAs( aPcbPlotParams.m_outputDirectory ) )
        return false;

    return true;
}


bool PCB_PLOT_PARAMS::SetHPGLPenDiameter( double aValue )
{
    return setDouble( &m_HPGLPenDiam, aValue, HPGL_PEN_DIAMETER_MIN, HPGL_PEN_DIAMETER_MAX );
}


bool PCB_PLOT_PARAMS::SetHPGLPenSpeed( int aValue )
{
    return setInt( &m_HPGLPenSpeed, aValue, HPGL_PEN_SPEED_MIN, HPGL_PEN_SPEED_MAX );
}


PCB_PLOT_PARAMS_PARSER::PCB_PLOT_PARAMS_PARSER( LINE_READER* aReader ) :
    PCB_PLOT_PARAMS_LEXER( aReader )
{
}


PCB_PLOT_PARAMS_PARSER::PCB_PLOT_PARAMS_PARSER( char* aLine, const wxString& aSource ) :
    PCB_PLOT_PARAMS_LEXER( aLine, aSource )
{
}


void PCB_PLOT_PARAMS_PARSER::Parse( PCB_PLOT_PARAMS* aPcbPlotParams )
{
    T   token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
            Unexpected( T_EOF );

        if( token == T_LEFT )
            token = NextTok();

        if( token == T_pcbplotparams )
            continue;

        bool skip_right = false;

        switch( token )
        {
        case T_layerselection:
        {
            token = NeedSYMBOLorNUMBER();

            const std::string& cur = CurStr();

            if( token == T_NUMBER ) // pretty 3 format had legacy Cu stack.
            {
                //  It's not possible to convert a legacy Cu layer number to a new Cu layer
                //  number without knowing the number or total Cu layers in the legacy board.
                //  We do not have that information here, so simply set all layers ON.  User
                //  can turn them off in the UI.
                aPcbPlotParams->m_layerSelection = LSET( 2, F_SilkS, B_SilkS ) | LSET::AllCuMask();
            }
            else if( cur.find_first_of( "0x" ) == 0 ) // pretty ver. 4.
            {
                // skip the leading 2 0x bytes.
                aPcbPlotParams->m_layerSelection.ParseHex( cur.c_str() + 2, cur.size() - 2 );
            }
            else
            {
                Expecting( "integer or hex layerSelection" );
            }

            break;
        }

        case T_plot_on_all_layers_selection:
        {
            token = NeedSYMBOLorNUMBER();

            const std::string& cur = CurStr();

            if( cur.find_first_of( "0x" ) == 0 )
            {
                // skip the leading 2 0x bytes.
                aPcbPlotParams->m_plotOnAllLayersSelection.ParseHex( cur.c_str() + 2,
                                                                     cur.size() - 2 );
            }
            else
            {
                Expecting( "hex plot_on_all_layers_selection" );
            }

            break;
        }

        case T_disableapertmacros:
            aPcbPlotParams->m_gerberDisableApertMacros = parseBool();
            break;

        case T_usegerberextensions:
            aPcbPlotParams->m_useGerberProtelExtensions = parseBool();
            break;

        case T_usegerberattributes:
            aPcbPlotParams->m_useGerberX2format = parseBool();
            break;

        case T_usegerberadvancedattributes:
            aPcbPlotParams->m_includeGerberNetlistInfo = parseBool();
            break;

        case T_creategerberjobfile:
            aPcbPlotParams->m_createGerberJobFile = parseBool();
            break;

        case T_gerberprecision:
            aPcbPlotParams->m_gerberPrecision = parseInt( gbrDefaultPrecision - 1,
                                                          gbrDefaultPrecision);
            break;

        case T_dashed_line_dash_ratio:
            aPcbPlotParams->m_dashedLineDashRatio = parseDouble();
            break;

        case T_dashed_line_gap_ratio:
            aPcbPlotParams->m_dashedLineGapRatio = parseDouble();
            break;

        case T_svgprecision:
            aPcbPlotParams->m_svgPrecision = parseInt( SVG_PRECISION_MIN, SVG_PRECISION_MAX );
            break;

        case T_svguseinch:
            parseBool();    // Unused. For compatibility
            break;

        case T_psa4output:
            aPcbPlotParams->m_A4Output = parseBool();
            break;

        case T_excludeedgelayer:
            if( !parseBool() )
                aPcbPlotParams->m_plotOnAllLayersSelection.set( Edge_Cuts );

            break;

        case T_plotframeref:
            aPcbPlotParams->m_plotDrawingSheet = parseBool();
            break;

        case T_viasonmask:
            aPcbPlotParams->m_plotViaOnMaskLayer = parseBool();
            break;

        case T_mode:
            aPcbPlotParams->SetPlotMode( parseInt( 0, 2 ) > 1 ? SKETCH : FILLED );
            break;

        case T_useauxorigin:
            aPcbPlotParams->m_useAuxOrigin = parseBool();
            break;

        case T_hpglpennumber:
            aPcbPlotParams->m_HPGLPenNum = parseInt( HPGL_PEN_NUMBER_MIN, HPGL_PEN_NUMBER_MAX );
            break;

        case T_hpglpenspeed:
            aPcbPlotParams->m_HPGLPenSpeed = parseInt( HPGL_PEN_SPEED_MIN, HPGL_PEN_SPEED_MAX );
            break;

        case T_hpglpendiameter:
            aPcbPlotParams->m_HPGLPenDiam = parseDouble();
            break;

        case T_hpglpenoverlay:
            // No more used. just here for compatibility with old versions
            parseInt( 0, HPGL_PEN_DIAMETER_MAX );
            break;

        case T_pdf_front_fp_property_popups:
            aPcbPlotParams->m_PDFFrontFPPropertyPopups = parseBool();
            break;

        case T_pdf_back_fp_property_popups:
            aPcbPlotParams->m_PDFFrontFPPropertyPopups = parseBool();
            break;

        case T_dxfpolygonmode:
            aPcbPlotParams->m_DXFPolygonMode = parseBool();
            break;

        case T_dxfimperialunits:
            aPcbPlotParams->m_DXFUnits = parseBool() ? DXF_UNITS::INCHES
                                                         : DXF_UNITS::MILLIMETERS;
            break;

        case T_dxfusepcbnewfont:
            aPcbPlotParams->m_textMode = parseBool() ? PLOT_TEXT_MODE::DEFAULT
                                                     : PLOT_TEXT_MODE::NATIVE;
            break;

        case T_pscolor:
            NeedSYMBOL(); // This actually was never used...
            break;

        case T_psnegative:
            aPcbPlotParams->m_negative = parseBool();
            break;

        case T_plotreference:
            aPcbPlotParams->m_plotReference = parseBool();
            break;

        case T_plotvalue:
            aPcbPlotParams->m_plotValue = parseBool();
            break;

        case T_plotinvisibletext:
            aPcbPlotParams->m_plotInvisibleText = parseBool();
            break;

        case T_sketchpadsonfab:
            aPcbPlotParams->m_sketchPadsOnFabLayers= parseBool();
            break;

        case T_subtractmaskfromsilk:
            aPcbPlotParams->m_subtractMaskFromSilk = parseBool();
            break;

        case T_outputformat:
            aPcbPlotParams->m_format = static_cast<PLOT_FORMAT>(
                    parseInt( static_cast<int>( PLOT_FORMAT::FIRST_FORMAT ),
                            static_cast<int>( PLOT_FORMAT::LAST_FORMAT ) ) );
            break;

        case T_mirror:
            aPcbPlotParams->m_mirror = parseBool();
            break;

        case T_drillshape:
            aPcbPlotParams->m_drillMarks = static_cast<DRILL_MARKS> ( parseInt( 0, 2 ) );
            break;

        case T_scaleselection:
            aPcbPlotParams->m_scaleSelection = parseInt( 0, 4 );
            break;

        case T_outputdirectory:
            NeedSYMBOLorNUMBER();   // a dir name can be like a number
            aPcbPlotParams->m_outputDirectory = FROM_UTF8( CurText() );
            break;

        default:
            skipCurrent();      // skip unknown or outdated plot parameter
            skip_right = true;  // the closing right token is already read.
            break;
        }

        if( ! skip_right )
            NeedRIGHT();
    }
}


bool PCB_PLOT_PARAMS_PARSER::parseBool()
{
    T token = NeedSYMBOL();

    if( token != T_false && token != T_true )
        Expecting( "true|false" );

    return token == T_true;
}


int PCB_PLOT_PARAMS_PARSER::parseInt( int aMin, int aMax )
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );

    int val = atoi( CurText() );

    if( val < aMin )
        val = aMin;
    else if( val > aMax )
        val = aMax;

    return val;
}


double PCB_PLOT_PARAMS_PARSER::parseDouble()
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );

    return DSNLEXER::parseDouble();
}


void PCB_PLOT_PARAMS_PARSER::skipCurrent()
{
    int curr_level = 0;
    T token;

    while( ( token = NextTok() ) != T_EOF )
    {
        if( token == T_LEFT )
            curr_level--;

        if( token == T_RIGHT )
        {
            curr_level++;

            if( curr_level > 0 )
                return;
        }
    }
}
