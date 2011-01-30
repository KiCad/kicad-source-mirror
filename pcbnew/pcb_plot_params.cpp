
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Kicad Developers, see change_log.txt for contributors.
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
#include "pcb_plot_params.h"
#include "pcb_plot_params_lexer.h"
#include "layers_id_colors_and_visibility.h"
#include "plot_common.h"
#include "macros.h"

extern int g_DrawDefaultLineThickness;

PCB_PLOT_PARAMS g_PcbPlotOptions;

using namespace PCBPLOTPARAMS_T;

static const char* GetTokenName( T aTok )
{
    return PCB_PLOT_PARAMS_LEXER::TokenName( aTok );
}


// PCB_PLOT_PARAMS

PCB_PLOT_PARAMS::PCB_PLOT_PARAMS()
{
    layerSelection         = LAYER_BACK | LAYER_FRONT
        | SILKSCREEN_LAYER_FRONT | SILKSCREEN_LAYER_BACK;
    useGerberExtensions    = true;
    m_ExcludeEdgeLayer     = true;
    m_PlotLineWidth        = g_DrawDefaultLineThickness;
    m_PlotFrameRef         = false;
    m_PlotViaOnMaskLayer   = false;
    m_PlotMode             = FILLED;
    useAuxOrigin           = false;
    m_HPGLPenNum           = 1;
    m_HPGLPenSpeed         = 20;
    m_HPGLPenDiam          = 15;
    m_HPGLPenOvr           = 2;
    m_PlotPSColorOpt       = true;
    m_PlotPSNegative       = false;
    m_PlotReference        = true;
    m_PlotValue            = true;
    m_PlotTextOther        = true;
    m_PlotInvisibleTexts   = false;
    m_PlotPadsOnSilkLayer  = false;
    subtractMaskFromSilk   = false;
    m_PlotFormat           = PLOT_FORMAT_GERBER;
    m_PlotMirror           = false;
    m_DrillShapeOpt        = SMALL_DRILL_SHAPE;
    m_AutoScale            = false;
    m_PlotScale            = 1.0;
    scaleSelection         = 1;
    m_FineScaleAdjustX     = 1.0;
    m_FineScaleAdjustY     = 1.0;
    outputDirectory        = wxT( "" );
}


void PCB_PLOT_PARAMS::Format( OUTPUTFORMATTER* aFormatter,
                              int aNestLevel ) const throw( IO_ERROR )
{
    const char* falseStr = GetTokenName( T_false );
    const char* trueStr = GetTokenName( T_true );

    aFormatter->Print( aNestLevel, "(%s", GetTokenName( T_pcbplotparams ) );
    aFormatter->Print( aNestLevel+1, "(%s %ld)\n", GetTokenName( T_layerselection ),
                       layerSelection );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_usegerberextensions ),
                       useGerberExtensions ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_excludeedgelayer ),
                       m_ExcludeEdgeLayer ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_linewidth ),
                       m_PlotLineWidth );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_plotframeref ),
                       m_PlotFrameRef ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_viasonmask ),
                       m_PlotViaOnMaskLayer ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_mode ),
                       m_PlotMode );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_useauxorigin ),
                       useAuxOrigin ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_hpglpennumber ),
                       m_HPGLPenNum );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_hpglpenspeed ),
                       m_HPGLPenSpeed );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_hpglpendiameter ),
                       m_HPGLPenDiam );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_hpglpenoverlay ),
                       m_HPGLPenOvr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_pscolor ),
                       m_PlotPSColorOpt ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_psnegative ),
                       m_PlotPSNegative ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_plotreference ),
                       m_PlotReference ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_plotvalue ),
                       m_PlotValue ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_plotothertext ),
                       m_PlotTextOther ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_plotinvisibletext ),
                       m_PlotInvisibleTexts ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_padsonsilk ),
                       m_PlotPadsOnSilkLayer ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_subtractmaskfromsilk ),
                       subtractMaskFromSilk ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_outputformat ),
                       m_PlotFormat );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_mirror ),
                       m_PlotMirror ? trueStr : falseStr );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_drillshape ),
                       m_DrillShapeOpt );
    aFormatter->Print( aNestLevel+1, "(%s %d)\n", GetTokenName( T_scaleselection ),
                       scaleSelection );
    aFormatter->Print( aNestLevel+1, "(%s %s)\n", GetTokenName( T_outputdirectory ),
                       aFormatter->Quoted( CONV_TO_UTF8( outputDirectory ) ).c_str() );
    aFormatter->Print( 0, ")\n" );
}


void PCB_PLOT_PARAMS::Parse( PCB_PLOT_PARAMS_PARSER* aParser ) throw( IO_ERROR, PARSE_ERROR )
{
    aParser->Parse( this );
}


bool PCB_PLOT_PARAMS::operator==( const PCB_PLOT_PARAMS &aPcbPlotParams ) const
{
    if( layerSelection != aPcbPlotParams.layerSelection )
        return false;
    if( useGerberExtensions != aPcbPlotParams.useGerberExtensions )
        return false;
    if( m_ExcludeEdgeLayer != aPcbPlotParams.m_ExcludeEdgeLayer )
        return false;
    if( m_PlotLineWidth != aPcbPlotParams.m_PlotLineWidth )
        return false;
    if( m_PlotFrameRef != aPcbPlotParams.m_PlotFrameRef )
        return false;
    if( m_PlotViaOnMaskLayer != aPcbPlotParams.m_PlotViaOnMaskLayer )
        return false;
    if( m_PlotMode != aPcbPlotParams.m_PlotMode )
        return false;
    if( useAuxOrigin != aPcbPlotParams.useAuxOrigin )
        return false;
    if( m_HPGLPenNum != aPcbPlotParams.m_HPGLPenNum )
        return false;
    if( m_HPGLPenSpeed != aPcbPlotParams.m_HPGLPenSpeed )
        return false;
    if( m_HPGLPenDiam != aPcbPlotParams.m_HPGLPenDiam )
        return false;
    if( m_HPGLPenOvr != aPcbPlotParams.m_HPGLPenOvr )
        return false;
    if( m_PlotPSColorOpt != aPcbPlotParams.m_PlotPSColorOpt )
        return false;
    if( m_PlotPSNegative != aPcbPlotParams.m_PlotPSNegative )
        return false;
    if( m_PlotReference != aPcbPlotParams.m_PlotReference )
        return false;
    if( m_PlotValue != aPcbPlotParams.m_PlotValue )
        return false;
    if( m_PlotTextOther != aPcbPlotParams.m_PlotTextOther )
        return false;
    if( m_PlotInvisibleTexts != aPcbPlotParams.m_PlotInvisibleTexts )
        return false;
    if( m_PlotPadsOnSilkLayer != aPcbPlotParams.m_PlotPadsOnSilkLayer )
        return false;
    if( subtractMaskFromSilk != aPcbPlotParams.subtractMaskFromSilk )
        return false;
    if( m_PlotFormat != aPcbPlotParams.m_PlotFormat )
        return false;
    if( m_PlotMirror != aPcbPlotParams.m_PlotMirror )
        return false;
    if( m_DrillShapeOpt != aPcbPlotParams.m_DrillShapeOpt )
        return false;
    if( scaleSelection != aPcbPlotParams.scaleSelection )
        return false;
    if( !outputDirectory.IsSameAs( aPcbPlotParams.outputDirectory ) )
        return false;
    return true;
}


bool PCB_PLOT_PARAMS::operator!=( const PCB_PLOT_PARAMS &aPcbPlotParams ) const
{
    return !( *this == aPcbPlotParams );
}


// PCB_PLOT_PARAMS_PARSER

PCB_PLOT_PARAMS_PARSER::PCB_PLOT_PARAMS_PARSER( LINE_READER* aReader ) :
    PCB_PLOT_PARAMS_LEXER( aReader )
{
}


PCB_PLOT_PARAMS_PARSER::PCB_PLOT_PARAMS_PARSER( char* aLine, wxString aSource ) :
    PCB_PLOT_PARAMS_LEXER( aLine, aSource )
{
}


void PCB_PLOT_PARAMS_PARSER::Parse( PCB_PLOT_PARAMS* aPcbPlotParams ) throw( IO_ERROR, PARSE_ERROR )
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
            aPcbPlotParams->layerSelection = atol( CurText() );
            break;
        case T_usegerberextensions:
            aPcbPlotParams->useGerberExtensions = ParseBool();
            break;
        case T_excludeedgelayer:
            aPcbPlotParams->m_ExcludeEdgeLayer = ParseBool();
            break;
        case T_linewidth:
            aPcbPlotParams->m_PlotLineWidth = ParseInt( 40, 200 );
            break;
        case T_plotframeref:
            aPcbPlotParams->m_PlotFrameRef = ParseBool();
            break;
        case T_viasonmask:
            aPcbPlotParams->m_PlotViaOnMaskLayer = ParseBool();
            break;
        case T_mode:
            aPcbPlotParams->m_PlotMode = (GRTraceMode)ParseInt( 0, 2 );
            break;
        case T_useauxorigin:
            aPcbPlotParams->useAuxOrigin = ParseBool();
            break;
        case T_hpglpennumber:
            aPcbPlotParams->m_HPGLPenNum = ParseInt( 1, 16 );
            break;
        case T_hpglpenspeed:
            aPcbPlotParams->m_HPGLPenSpeed = ParseInt( 0, 1000 );
            break;
        case T_hpglpendiameter:
            aPcbPlotParams->m_HPGLPenDiam = ParseInt( 0, 100 );
            break;
        case T_hpglpenoverlay:
            aPcbPlotParams->m_HPGLPenOvr = ParseInt( 0, 0x100 );
            break;
        case T_pscolor:
            aPcbPlotParams->m_PlotPSColorOpt = ParseBool();
            break;
        case T_psnegative:
            aPcbPlotParams->m_PlotPSNegative = ParseBool();
            break;
        case T_plotreference:
            aPcbPlotParams->m_PlotReference = ParseBool();
            break;
        case T_plotvalue:
            aPcbPlotParams->m_PlotValue = ParseBool();
            break;
        case T_plotothertext:
            aPcbPlotParams->m_PlotTextOther = ParseBool();
            break;
        case T_plotinvisibletext:
            aPcbPlotParams->m_PlotInvisibleTexts = ParseBool();
            break;
        case T_padsonsilk:
            aPcbPlotParams->m_PlotPadsOnSilkLayer= ParseBool();
            break;
        case T_subtractmaskfromsilk:
            aPcbPlotParams->subtractMaskFromSilk = ParseBool();
            break;
        case T_outputformat:
            aPcbPlotParams->m_PlotFormat = ParseInt( 0, 4 );
            break;
        case T_mirror:
            aPcbPlotParams->m_PlotMirror = ParseBool();
            break;
        case T_drillshape:
            aPcbPlotParams->m_DrillShapeOpt = (PCB_PLOT_PARAMS::DrillShapeOptT) ParseInt( 0, 2 );
            break;
        case T_scaleselection:
            aPcbPlotParams->scaleSelection = ParseInt( 0, 4 );
            break;
        case T_outputdirectory:
            NeedSYMBOL();
            aPcbPlotParams->outputDirectory = CONV_FROM_UTF8( CurText() );
            break;
        default:
            Unexpected( CurText() );
            break;
        }
        NeedRIGHT();
    }
}


bool PCB_PLOT_PARAMS_PARSER::ParseBool() throw( IO_ERROR )
{
    T token;
    token = NeedSYMBOL();
    if( token != T_false && token != T_true )
        Expecting( "true|false" );
    return (token == T_true);
}


int PCB_PLOT_PARAMS_PARSER::ParseInt( int aMin, int aMax ) throw( IO_ERROR )
{
    T token;
    int i;
    token = NextTok();
    if( token != T_NUMBER )
        Expecting( T_NUMBER );
    i = atoi( CurText() );

    if( i < aMin )
        i = aMin;
    else if( i > aMax )
        i = aMax;

    return i;
}
