
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
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

#include <wx/wx.h>
#include <pcb_plot_params.h>
#include <layers_id_colors_and_visibility.h>
#include <plot_common.h>
#include <macros.h>
#include <convert_to_biu.h>


#define PLOT_LINEWIDTH_MIN        (0.02*IU_PER_MM)  // min value for default line thickness
#define PLOT_LINEWIDTH_MAX        (2*IU_PER_MM)     // max value for default line thickness
#define PLOT_LINEWIDTH_DEFAULT    (0.15*IU_PER_MM)  // def. value for default line thickness
#define HPGL_PEN_DIAMETER_MIN     0
#define HPGL_PEN_DIAMETER_MAX     100       // Unit = mil
#define HPGL_PEN_SPEED_MIN        1         // this param is always in cm/s
#define HPGL_PEN_SPEED_MAX        99        // this param is always in cm/s
#define HPGL_PEN_NUMBER_MIN       1
#define HPGL_PEN_NUMBER_MAX       16
#define HPGL_PEN_OVERLAP_MIN      0
#define HPGL_PEN_OVERLAP_MAX      50        // Unit = mil


/**
 * Default line thickness in internal units used to draw or plot items using a
 * default thickness line value (Frame references)
 */
int g_DrawDefaultLineThickness = PLOT_LINEWIDTH_DEFAULT;


using namespace PCBPLOTPARAMS_T;


static const char* getTokenName( T aTok )
{
    return PCB_PLOT_PARAMS_LEXER::TokenName( aTok );
}


static bool setInt( int* aInt, int aValue, int aMin, int aMax )
{
    int temp = aValue;

    if( aValue < aMin )
        temp = aMin;
    else if( aValue > aMax )
        temp = aMax;

    *aInt = temp;
    return (temp == aValue);
}

// PCB_PLOT_PARAMS

PCB_PLOT_PARAMS::PCB_PLOT_PARAMS()
{
    m_layerSelection       = LAYER_BACK | LAYER_FRONT
        | SILKSCREEN_LAYER_FRONT | SILKSCREEN_LAYER_BACK;
    m_useGerberExtensions  = true;
    m_useGerberAttributes  = false;
    m_excludeEdgeLayer     = true;
    m_lineWidth            = g_DrawDefaultLineThickness;
    m_plotFrameRef         = false;
    m_plotViaOnMaskLayer   = false;
    m_mode                 = FILLED;
    m_useAuxOrigin         = false;
    m_HPGLPenNum           = 1;
    m_HPGLPenSpeed         = 20;        // this param is always in cm/s
    m_HPGLPenDiam          = 15;        // in mils
    m_HPGLPenOvr           = 2;         // in mils
    m_negative             = false;
    m_A4Output             = false;
    m_plotReference        = true;
    m_plotValue            = true;
    m_plotInvisibleText    = false;
    m_plotPadsOnSilkLayer  = false;
    m_subtractMaskFromSilk = false;
    m_format               = PLOT_FORMAT_GERBER;
    m_mirror               = false;
    m_drillMarks           = SMALL_DRILL_SHAPE;
    m_autoScale            = false;
    m_scale                = 1.0;
    m_scaleSelection       = 1;
    m_fineScaleAdjustX     = 1.0;
    m_fineScaleAdjustY     = 1.0;
    m_widthAdjust          = 0.;
    m_outputDirectory.clear();
    m_color                = BLACK;
    m_referenceColor       = BLACK;
    m_valueColor           = BLACK;
    m_textMode             = PLOTTEXTMODE_DEFAULT;

    // This parameter controls if the NPTH pads will be plotted or not
    // it is a "local" parameter
    m_skipNPTH_Pads        = false;
}


// PLEASE NOTE: only plot dialog options are processed
void PCB_PLOT_PARAMS::Format( OUTPUTFORMATTER* aFormatter,
                              int aNestLevel, int aControl ) const throw( IO_ERROR )
{
    const char* falseStr = getTokenName( T_false );
    const char* trueStr = getTokenName( T_true );

    aFormatter->Print( aNestLevel, "(%s\n", getTokenName( T_pcbplotparams ) );
    aFormatter->Print( aNestLevel+1, "(%s %ld)\n", getTokenName( T_layerselection ),
                       long(m_layerSelection) );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_usegerberextensions ),
                       m_useGerberExtensions ? trueStr : falseStr );

    if( m_useGerberAttributes )  // save this option only if active,
                                // to avoid incompatibility with older Pcbnew version
        aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_usegerberattributes ), trueStr );

    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_excludeedgelayer ),
                       m_excludeEdgeLayer ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %f)\n", getTokenName( T_linewidth ),
                       m_lineWidth / IU_PER_MM );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_plotframeref ),
                       m_plotFrameRef ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_viasonmask ),
                       m_plotViaOnMaskLayer ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_mode ),
                       m_mode );
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
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_hpglpendiameter ),
                       m_HPGLPenDiam );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", getTokenName( T_hpglpenoverlay ),
                       m_HPGLPenOvr );
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
    aFormatter->Print( aNestLevel+1, "(%s %s)", getTokenName( T_outputdirectory ),
                       aFormatter->Quotew( m_outputDirectory ).c_str() );
    aFormatter->Print( 0, ")\n" );
}


void PCB_PLOT_PARAMS::Parse( PCB_PLOT_PARAMS_PARSER* aParser )
                      throw( PARSE_ERROR, IO_ERROR )
{
    aParser->Parse( this );
}


bool PCB_PLOT_PARAMS::operator==( const PCB_PLOT_PARAMS &aPcbPlotParams ) const
{
    if( m_layerSelection != aPcbPlotParams.m_layerSelection )
        return false;
    if( m_useGerberExtensions != aPcbPlotParams.m_useGerberExtensions )
        return false;
    if( m_useGerberAttributes != aPcbPlotParams.m_useGerberAttributes )
        return false;
    if( m_excludeEdgeLayer != aPcbPlotParams.m_excludeEdgeLayer )
        return false;
    if( m_lineWidth != aPcbPlotParams.m_lineWidth )
        return false;
    if( m_plotFrameRef != aPcbPlotParams.m_plotFrameRef )
        return false;
    if( m_plotViaOnMaskLayer != aPcbPlotParams.m_plotViaOnMaskLayer )
        return false;
    if( m_mode != aPcbPlotParams.m_mode )
        return false;
    if( m_useAuxOrigin != aPcbPlotParams.m_useAuxOrigin )
        return false;
    if( m_HPGLPenNum != aPcbPlotParams.m_HPGLPenNum )
        return false;
    if( m_HPGLPenSpeed != aPcbPlotParams.m_HPGLPenSpeed )
        return false;
    if( m_HPGLPenDiam != aPcbPlotParams.m_HPGLPenDiam )
        return false;
    if( m_HPGLPenOvr != aPcbPlotParams.m_HPGLPenOvr )
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
    if( m_color != aPcbPlotParams.m_color )
        return false;
    if( m_referenceColor != aPcbPlotParams.m_referenceColor )
        return false;
    if( m_valueColor != aPcbPlotParams.m_valueColor )
        return false;
    if( m_textMode != aPcbPlotParams.m_textMode )
        return false;
    if( !m_outputDirectory.IsSameAs( aPcbPlotParams.m_outputDirectory ) )
        return false;
    return true;
}


bool PCB_PLOT_PARAMS::operator!=( const PCB_PLOT_PARAMS &aPcbPlotParams ) const
{
    return !( *this == aPcbPlotParams );
}


bool PCB_PLOT_PARAMS::SetHPGLPenDiameter( int aValue )
{
    return setInt( &m_HPGLPenDiam, aValue, HPGL_PEN_DIAMETER_MIN, HPGL_PEN_DIAMETER_MAX );
}


bool PCB_PLOT_PARAMS::SetHPGLPenSpeed( int aValue )
{
    return setInt( &m_HPGLPenSpeed, aValue, HPGL_PEN_SPEED_MIN, HPGL_PEN_SPEED_MAX );
}


bool PCB_PLOT_PARAMS::SetHPGLPenOverlay( int aValue )
{
    return setInt( &m_HPGLPenOvr, aValue, HPGL_PEN_OVERLAP_MIN, HPGL_PEN_OVERLAP_MAX );
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
                             throw( PARSE_ERROR, IO_ERROR )
{
    T token;
    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
            Unexpected( T_EOF );

        if( token == T_LEFT )
            token = NextTok();

        if( token == T_pcbplotparams )
            continue;

        switch( token )
        {
        case T_layerselection:
            token = NextTok();
            if( token != T_NUMBER )
                Expecting( T_NUMBER );
            aPcbPlotParams->m_layerSelection = atol( CurText() );
            break;
        case T_usegerberextensions:
            aPcbPlotParams->m_useGerberExtensions = parseBool();
            break;
        case T_usegerberattributes:
            aPcbPlotParams->m_useGerberAttributes = parseBool();
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
            aPcbPlotParams->m_mode = static_cast<EDA_DRAW_MODE_T>( parseInt( 0, 2 ) );
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
            aPcbPlotParams->m_HPGLPenDiam = parseInt( HPGL_PEN_DIAMETER_MIN,
                                                      HPGL_PEN_DIAMETER_MAX );
            break;
        case T_hpglpenoverlay:
            aPcbPlotParams->m_HPGLPenOvr = parseInt( HPGL_PEN_OVERLAP_MIN,
                                                     HPGL_PEN_OVERLAP_MAX );
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
        case T_plotothertext:   // no more in use: keep for compatibility
            parseBool();    // skip param value
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
            NeedSYMBOL();
            aPcbPlotParams->m_outputDirectory = FROM_UTF8( CurText() );
            break;
        default:
            Unexpected( CurText() );
            break;
        }
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
