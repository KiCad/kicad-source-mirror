/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_plot_params_parser.h>
#include <pcb_plot_params.h>
#include <layers_id_colors_and_visibility.h>
#include <plotter.h>
#include <macros.h>
#include <convert_to_biu.h>
#include <board_design_settings.h>


#define PLOT_LINEWIDTH_MIN        ( 0.02 * IU_PER_MM )  // min value for default line thickness
#define PLOT_LINEWIDTH_MAX        ( 2 * IU_PER_MM )     // max value for default line thickness
#define PLOT_LINEWIDTH_DEFAULT    ( DEFAULT_TEXT_WIDTH * IU_PER_MM )
#define HPGL_PEN_DIAMETER_MIN     0
#define HPGL_PEN_DIAMETER_MAX     100.0     // Unit = mil
#define HPGL_PEN_SPEED_MIN        1         // this param is always in cm/s
#define HPGL_PEN_SPEED_MAX        99        // this param is always in cm/s
#define HPGL_PEN_NUMBER_MIN       1
#define HPGL_PEN_NUMBER_MAX       16


/**
 * Default line thickness in internal units used to draw or plot items using a
 * default thickness line value (Frame references)
 */
int g_DrawDefaultLineThickness = PLOT_LINEWIDTH_DEFAULT;

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
    return (temp == aValue);
}


static bool setDouble( double* aTarget, double aValue, double aMin, double aMax )
{
    double temp = aValue;

    if( aValue < aMin )
        temp = aMin;
    else if( aValue > aMax )
        temp = aMax;

    *aTarget = temp;
    return (temp == aValue);
}

// PCB_PLOT_PARAMS

PCB_PLOT_PARAMS::PCB_PLOT_PARAMS()
{
    m_useGerberProtelExtensions  = false;
    m_useGerberX2format          = false;
    m_includeGerberNetlistInfo   = false;
    m_createGerberJobFile        = false;
    m_gerberPrecision            = gbrDefaultPrecision;
    m_excludeEdgeLayer           = true;
    m_lineWidth                  = g_DrawDefaultLineThickness;
    m_plotFrameRef               = false;
    m_plotViaOnMaskLayer         = false;
    m_plotMode                   = FILLED;
    m_DXFplotPolygonMode         = true;
    m_DXFplotUnits               = DXF_PLOTTER::DXF_UNIT_INCHES;
    m_useAuxOrigin               = false;
    m_HPGLPenNum                 = 1;
    m_HPGLPenSpeed               = 20;        // this param is always in cm/s
    m_HPGLPenDiam                = 15;        // in mils
    m_negative                   = false;
    m_A4Output                   = false;
    m_plotReference              = true;
    m_plotValue                  = true;
    m_plotInvisibleText          = false;
    m_plotPadsOnSilkLayer        = false;
    m_subtractMaskFromSilk       = false;
    m_format                     = PLOT_FORMAT_GERBER;
    m_mirror                     = false;
    m_drillMarks                 = SMALL_DRILL_SHAPE;
    m_autoScale                  = false;
    m_scale                      = 1.0;
    m_scaleSelection             = 1;
    m_fineScaleAdjustX           = 1.0;
    m_fineScaleAdjustY           = 1.0;
    m_widthAdjust                = 0.;
    m_outputDirectory.clear();
    m_color                      = BLACK;
    m_textMode                   = PLOTTEXTMODE_DEFAULT;
    m_layerSelection             = LSET( 7, F_SilkS, B_SilkS, F_Mask, B_Mask,
                                         F_Paste, B_Paste, Edge_Cuts )
                                         | LSET::AllCuMask();
    // This parameter controls if the NPTH pads will be plotted or not
    // it is a "local" parameter
    m_skipNPTH_Pads              = false;
}

void PCB_PLOT_PARAMS::SetGerberPrecision( int aPrecision )
{
    // Currently geber files use mm.
    // accepted precision is only 6 (max value, this is the resolution of Pcbnew)
    // or 5, min value for professional boards, when 6 creates problems
    // to board makers.

    m_gerberPrecision = aPrecision == gbrDefaultPrecision-1 ? gbrDefaultPrecision-1 :
                                      gbrDefaultPrecision;
}


// PLEASE NOTE: only plot dialog options are processed
void PCB_PLOT_PARAMS::Format( OUTPUTFORMATTER* aFormatter,
                              int aNestLevel, int aControl ) const
{
    const char* falseStr = getTokenName( T_false );
    const char* trueStr = getTokenName( T_true );

    aFormatter->Print( aNestLevel, "(%s\n", getTokenName( T_pcbplotparams ) );

    aFormatter->Print( aNestLevel+1, "(%s 0x%s)\n", getTokenName( T_layerselection ),
                       m_layerSelection.FmtHex().c_str() );

    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_usegerberextensions ),
                       m_useGerberProtelExtensions ? trueStr : falseStr );

    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_usegerberattributes ),
                       GetUseGerberX2format() ? trueStr : falseStr );

    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_usegerberadvancedattributes ),
                       GetIncludeGerberNetlistInfo() ? trueStr : falseStr );

    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_creategerberjobfile ),
                       GetCreateGerberJobFile() ? trueStr : falseStr );

    if( m_gerberPrecision != gbrDefaultPrecision ) // save this option only if it is not the default value,
                                                   // to avoid incompatibility with older Pcbnew version
        aFormatter->Print( aNestLevel+1, "(%s %d)\n",
                           getTokenName( T_gerberprecision ), m_gerberPrecision );

    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_excludeedgelayer ),
                       m_excludeEdgeLayer ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %f)\n", getTokenName( T_linewidth ),
                       m_lineWidth / IU_PER_MM );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_plotframeref ),
                       m_plotFrameRef ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_viasonmask ),
                       m_plotViaOnMaskLayer ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_mode ),
                       GetPlotMode() == SKETCH ? 2 : 1 );       // Value 0 (LINE mode) no more used
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_useauxorigin ),
                       m_useAuxOrigin ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_hpglpennumber ),
                       m_HPGLPenNum );

    //  Obsolete parameter, pen speed is no more managed, because hpgl format
    // is now an export format, and for this, pen speed has no meaning
    //    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_hpglpenspeed ),
    //                       m_HPGLPenSpeed );

    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_hpglpenspeed ),
                       m_HPGLPenSpeed );
    aFormatter->Print( aNestLevel+1, "(%s %f)\n", getTokenName( T_hpglpendiameter ),
                       m_HPGLPenDiam );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_psnegative ),
                       m_negative ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_psa4output ),
                       m_A4Output ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_plotreference ),
                       m_plotReference ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_plotvalue ),
                       m_plotValue ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_plotinvisibletext ),
                       m_plotInvisibleText ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_padsonsilk ),
                       m_plotPadsOnSilkLayer ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_subtractmaskfromsilk ),
                       m_subtractMaskFromSilk ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_outputformat ),
                       m_format );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_mirror ),
                       m_mirror ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_drillshape ),
                       m_drillMarks );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_scaleselection ),
                       m_scaleSelection );
    aFormatter->Print( aNestLevel+1, "(%s \"%s\")", getTokenName( T_outputdirectory ),
                       (const char*) m_outputDirectory.utf8_str() );
    aFormatter->Print( 0, ")\n" );
}


void PCB_PLOT_PARAMS::Parse( PCB_PLOT_PARAMS_PARSER* aParser )
{
    aParser->Parse( this );
}


bool PCB_PLOT_PARAMS::IsSameAs( const PCB_PLOT_PARAMS &aPcbPlotParams, bool aCompareOnlySavedPrms ) const
{
    if( m_layerSelection != aPcbPlotParams.m_layerSelection )
        return false;
    if( m_useGerberProtelExtensions != aPcbPlotParams.m_useGerberProtelExtensions )
        return false;
    if( m_useGerberX2format != aPcbPlotParams.m_useGerberX2format )
        return false;
    if( m_includeGerberNetlistInfo != aPcbPlotParams.m_includeGerberNetlistInfo )
        return false;
    if( m_createGerberJobFile != aPcbPlotParams.m_createGerberJobFile )
        return false;
    if( m_gerberPrecision != aPcbPlotParams.m_gerberPrecision )
        return false;
    if( m_excludeEdgeLayer != aPcbPlotParams.m_excludeEdgeLayer )
        return false;
    if( m_lineWidth != aPcbPlotParams.m_lineWidth )
        return false;
    if( m_plotFrameRef != aPcbPlotParams.m_plotFrameRef )
        return false;
    if( m_plotViaOnMaskLayer != aPcbPlotParams.m_plotViaOnMaskLayer )
        return false;
    if( m_plotMode != aPcbPlotParams.m_plotMode )
        return false;
    if( !aCompareOnlySavedPrms )
    {
        if( m_DXFplotPolygonMode != aPcbPlotParams.m_DXFplotPolygonMode )
            return false;
        if( m_DXFplotUnits != aPcbPlotParams.m_DXFplotUnits )
            return false;
    }
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
    if( m_A4Output != aPcbPlotParams.m_A4Output )
        return false;
    if( m_plotReference != aPcbPlotParams.m_plotReference )
        return false;
    if( m_plotValue != aPcbPlotParams.m_plotValue )
        return false;
    if( m_plotInvisibleText != aPcbPlotParams.m_plotInvisibleText )
        return false;
    if( m_plotPadsOnSilkLayer != aPcbPlotParams.m_plotPadsOnSilkLayer )
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
    if( !aCompareOnlySavedPrms )
    {
        if( m_color != aPcbPlotParams.m_color )
            return false;
    }
    if( m_textMode != aPcbPlotParams.m_textMode )
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


bool PCB_PLOT_PARAMS::SetLineWidth( int aValue )
{
    return setInt( &m_lineWidth, aValue, PLOT_LINEWIDTH_MIN, PLOT_LINEWIDTH_MAX );
}

// PCB_PLOT_PARAMS_PARSER

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
                    // unsigned legacy_mask = atol( cur.c_str() );

                    /*  It's not possible to convert a legacy Cu layer number to a new
                        Cu layer number without knowing the number or total Cu layers
                        in the legacy board.  We do not have that information here.
                        So simply set all layers ON.  User can turn them off in the UI.
                        This is one of the superiorities of the new Cu sequence.
                    aPcbPlotParams->m_layerSelection = LEGACY_PLUGIN::leg_mask2new( cu_count, legacy_mask );
                    */

                    // sorry, use the UI once to fix:
                    aPcbPlotParams->m_layerSelection = LSET( 2, F_SilkS, B_SilkS) | LSET::AllCuMask();
                }
                else if( cur.find_first_of( "0x" ) == 0 )   // pretty ver. 4.
                {
                    // skip the leading 2 0x bytes.
                    aPcbPlotParams->m_layerSelection.ParseHex( cur.c_str()+2, cur.size()-2 );
                }
                else
                    Expecting( "integer or hex layerSelection" );
            }
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
            aPcbPlotParams->m_gerberPrecision =
                parseInt( gbrDefaultPrecision-1, gbrDefaultPrecision);
            break;

        case T_psa4output:
            aPcbPlotParams->m_A4Output = parseBool();
            break;

        case T_excludeedgelayer:
            aPcbPlotParams->m_excludeEdgeLayer = parseBool();
            break;

        case T_linewidth:
            {
                // Due to a bug, this (minor) parameter was saved in biu
                // and now is saved in mm
                // If the read value is outside bounds, force a default value
                double tmp = parseDouble();
                if( !aPcbPlotParams->SetLineWidth( KiROUND( tmp * IU_PER_MM ) ) )
                    aPcbPlotParams->SetLineWidth( PLOT_LINEWIDTH_DEFAULT );
            }
            break;

        case T_plotframeref:
            aPcbPlotParams->m_plotFrameRef = parseBool();
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
            aPcbPlotParams->m_HPGLPenNum = parseInt( HPGL_PEN_NUMBER_MIN,
                                                     HPGL_PEN_NUMBER_MAX );
            break;

        case T_hpglpenspeed:
            aPcbPlotParams->m_HPGLPenSpeed = parseInt( HPGL_PEN_SPEED_MIN,
                                                       HPGL_PEN_SPEED_MAX );
            break;

        case T_hpglpendiameter:
            aPcbPlotParams->m_HPGLPenDiam = parseDouble();
            break;

        case T_hpglpenoverlay:
            // No more used. juste here for compatibility with old versions
            parseInt( 0, HPGL_PEN_DIAMETER_MAX );
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

        case T_padsonsilk:
            aPcbPlotParams->m_plotPadsOnSilkLayer= parseBool();
            break;

        case T_subtractmaskfromsilk:
            aPcbPlotParams->m_subtractMaskFromSilk = parseBool();
            break;

        case T_outputformat:
            aPcbPlotParams->m_format = static_cast<PlotFormat>(
                                    parseInt( PLOT_FIRST_FORMAT, PLOT_LAST_FORMAT ) );
            break;

        case T_mirror:
            aPcbPlotParams->m_mirror = parseBool();
            break;

        case T_drillshape:
            aPcbPlotParams->m_drillMarks = static_cast<PCB_PLOT_PARAMS::DrillMarksType>
                                            ( parseInt( 0, 2 ) );
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

    double val = strtod( CurText(), NULL );

    return val;
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
