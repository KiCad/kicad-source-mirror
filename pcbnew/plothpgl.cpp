/**
 * @file plothpgl.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <plot_common.h>
#include <confirm.h>
#include <trigo.h>
#include <wxBasePcbFrame.h>
#include <macros.h>

#include <class_board.h>

#include <pcbnew.h>
#include <protos.h>
#include <pcbplot.h>


bool PCB_BASE_FRAME::ExportToHpglFile( const wxString& aFullFileName, int aLayer,
                                       EDA_DRAW_MODE_T aTraceMode )
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

    PCB_PLOT_PARAMS plot_opts = GetPlotSettings();

    // Compute pen_dim (from g_m_HPGLPenDiam in mils) in pcb units,
    // with plot scale (if Scale is 2, pen diameter is always g_m_HPGLPenDiam
    // so apparent pen diam is real pen diam / Scale
    int pen_diam = KiROUND( (plot_opts.m_HPGLPenDiam * U_PCB) /
                            plot_opts.m_PlotScale );

    // compute pen_overlay (from g_m_HPGLPenOvr in mils) with plot scale
    if( plot_opts.m_HPGLPenOvr < 0 )
        plot_opts.m_HPGLPenOvr = 0;

    if( plot_opts.m_HPGLPenOvr >= plot_opts.m_HPGLPenDiam )
        plot_opts.m_HPGLPenOvr = plot_opts.m_HPGLPenDiam - 1;

    int   pen_overlay = KiROUND( plot_opts.m_HPGLPenOvr * 10.0 /
                                 plot_opts.m_PlotScale );


    if( plot_opts.m_PlotScale != 1.0 || plot_opts.m_AutoScale )
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

    if( plot_opts.m_AutoScale )       // Optimum scale
    {
        // Fit to 80% of the page
        double Xscale = ( ( pageSizeIU.x * 0.8 ) / boardSize.x );
        double Yscale = ( ( pageSizeIU.y * 0.8 ) / boardSize.y );
        scale  = MIN( Xscale, Yscale );
    }
    else
    {
        scale = plot_opts.m_PlotScale;
    }

    // Calculate the page size offset.
    if( center )
    {
        offset.x = KiROUND( (double) boardCenter.x -
                            ( (double) pageSizeIU.x / 2.0 ) / scale );
        offset.y = KiROUND( (double) boardCenter.y -
                            ( (double) pageSizeIU.y / 2.0 ) / scale );
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();

    plotter->SetPageSettings( GetPageSettings() );

    // why did we have to change these settings above?
    SetPlotSettings( plot_opts );

    plotter->set_viewport( offset, scale, plot_opts.m_PlotMirror );
    plotter->set_default_line_width( plot_opts.m_PlotLineWidth );
    plotter->set_creator( wxT( "PCBNEW-HPGL" ) );
    plotter->set_filename( aFullFileName );
    plotter->set_pen_speed( plot_opts.m_HPGLPenSpeed );
    plotter->set_pen_number( plot_opts.m_HPGLPenNum );
    plotter->set_pen_overlap( pen_overlay );
    plotter->set_pen_diameter( pen_diam );
    plotter->start_plot( output_file );

    // The worksheet is not significant with scale!=1... It is with paperscale!=1, anyway
    if( plot_opts.m_PlotFrameRef && !center )
        PlotWorkSheet( plotter, GetScreen() );

    Plot_Layer( plotter, aLayer, aTraceMode );
    plotter->end_plot();
    delete plotter;

    return true;
}
