/**
 * @file plothpgl.cpp
 */

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "confirm.h"
#include "trigo.h"
#include "wxBasePcbFrame.h"
#include "macros.h"

#include "class_board.h"

#include "pcbnew.h"
#include "protos.h"
#include "pcbplot.h"


bool PCB_BASE_FRAME::ExportToHpglFile( const wxString& aFullFileName, int aLayer,
                                       GRTraceMode aTraceMode )
{
    wxSize      boardSize;
    wxPoint     boardCenter;
    bool        center = false;
    double      scale;
    wxPoint     offset;
    LOCALE_IO   toggle;

    FILE*       output_file = wxFopen( aFullFileName, wxT( "wt" ) );

    if( output_file == NULL )
    {
        return false;
    }

    // Compute pen_dim (from g_m_HPGLPenDiam in mils) in pcb units,
    // with plot scale (if Scale is 2, pen diameter is always g_m_HPGLPenDiam
    // so apparent pen diam is real pen diam / Scale
    int pen_diam = wxRound( (g_PcbPlotOptions.m_HPGLPenDiam * U_PCB) /
                            g_PcbPlotOptions.m_PlotScale );

    // compute pen_overlay (from g_m_HPGLPenOvr in mils) with plot scale
    if( g_PcbPlotOptions.m_HPGLPenOvr < 0 )
        g_PcbPlotOptions.m_HPGLPenOvr = 0;

    if( g_PcbPlotOptions.m_HPGLPenOvr >= g_PcbPlotOptions.m_HPGLPenDiam )
        g_PcbPlotOptions.m_HPGLPenOvr = g_PcbPlotOptions.m_HPGLPenDiam - 1;

    int   pen_overlay = wxRound( g_PcbPlotOptions.m_HPGLPenOvr * 10.0 /
                                 g_PcbPlotOptions.m_PlotScale );


    if( g_PcbPlotOptions.m_PlotScale != 1.0 || g_PcbPlotOptions.m_AutoScale )
    {
        // when scale != 1.0 we must calculate the position in page
        // because actual position has no meaning
        center = true;
    }

    wxSize pageSizeIU = GetPageSizeIU();

    // Calculate the center of the PCB
    EDA_RECT bbbox = GetBoardBoundingBox();

    boardSize   = bbbox.GetSize();
    boardCenter = bbbox.Centre();

    if( g_PcbPlotOptions.m_AutoScale )       // Optimum scale
    {
        // Fit to 80% of the page
        double Xscale = ( ( pageSizeIU.x * 0.8 ) / boardSize.x );
        double Yscale = ( ( pageSizeIU.y * 0.8 ) / boardSize.y );
        scale  = MIN( Xscale, Yscale );
    }
    else
    {
        scale = g_PcbPlotOptions.m_PlotScale;
    }

    // Calculate the page size offset.
    if( center )
    {
        offset.x = wxRound( (double) boardCenter.x -
                            ( (double) pageSizeIU.x / 2.0 ) / scale );
        offset.y = wxRound( (double) boardCenter.y -
                            ( (double) pageSizeIU.y / 2.0 ) / scale );
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();

    plotter->SetPageSettings( GetPageSettings() );

    plotter->set_viewport( offset, scale, g_PcbPlotOptions.m_PlotMirror );
    plotter->set_default_line_width( g_PcbPlotOptions.m_PlotLineWidth );
    plotter->set_creator( wxT( "PCBNEW-HPGL" ) );
    plotter->set_filename( aFullFileName );
    plotter->set_pen_speed( g_PcbPlotOptions.m_HPGLPenSpeed );
    plotter->set_pen_number( g_PcbPlotOptions.m_HPGLPenNum );
    plotter->set_pen_overlap( pen_overlay );
    plotter->set_pen_diameter( pen_diam );
    plotter->start_plot( output_file );

    // The worksheet is not significant with scale!=1... It is with paperscale!=1, anyway
    if( g_PcbPlotOptions.m_PlotFrameRef && !center )
        PlotWorkSheet( plotter, GetScreen() );

    Plot_Layer( plotter, aLayer, aTraceMode );
    plotter->end_plot();
    delete plotter;

    return true;
}
