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
    wxSize        SheetSize;
    wxSize        BoardSize;
    wxPoint       BoardCenter;
    bool          Center = false;
    Ki_PageDescr* currentsheet = GetScreen()->m_CurrentSheetDesc;
    double        scale;
    wxPoint       offset;

    FILE* output_file = wxFopen( aFullFileName, wxT( "wt" ) );

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


    SetLocaleTo_C_standard();

    if( g_PcbPlotOptions.m_PlotScale != 1.0 || g_PcbPlotOptions.m_AutoScale )
        Center = true;  // when scale != 1.0 we must calculate the position in page
                        // because actual position has no meaning

    // Scale units from 0.0001" to HPGL plot units.
    SheetSize.x = currentsheet->m_Size.x * U_PCB;
    SheetSize.y = currentsheet->m_Size.y * U_PCB;

    /* Calculate the center of the PCB. */
    m_Pcb->ComputeBoundingBox();
    BoardSize   = m_Pcb->m_BoundaryBox.GetSize();
    BoardCenter = m_Pcb->m_BoundaryBox.Centre();

    if( g_PcbPlotOptions.m_AutoScale )       // Optimum scale
    {
        double Xscale, Yscale;

        // Fit to 80% of the page
        Xscale = ( ( SheetSize.x * 0.8 ) / BoardSize.x );
        Yscale = ( ( SheetSize.y * 0.8 ) / BoardSize.y );
        scale  = MIN( Xscale, Yscale );
    }
    else
    {
        scale = g_PcbPlotOptions.m_PlotScale;
    }

    // Calculate the page size offset.
    if( Center )
    {
        offset.x = wxRound( (double) BoardCenter.x -
                            ( (double) SheetSize.x / 2.0 ) / scale );
        offset.y = wxRound( (double) BoardCenter.y -
                            ( (double) SheetSize.y / 2.0 ) / scale );
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();
    plotter->set_paper_size( currentsheet );
    plotter->set_viewport( offset, scale, g_PcbPlotOptions.m_PlotMirror );
    plotter->set_default_line_width( g_PcbPlotOptions.m_PlotLineWidth );
    plotter->set_creator( wxT( "PCBNEW-HPGL" ) );
    plotter->set_filename( aFullFileName );
    plotter->set_pen_speed( g_PcbPlotOptions.m_HPGLPenSpeed );
    plotter->set_pen_number( g_PcbPlotOptions.m_HPGLPenNum );
    plotter->set_pen_overlap( pen_overlay );
    plotter->set_pen_diameter( pen_diam );
    plotter->start_plot( output_file );

    /* The worksheet is not significant with scale!=1... It is with paperscale!=1, anyway */
    if( g_PcbPlotOptions.m_PlotFrameRef && !Center )
        PlotWorkSheet( plotter, GetScreen() );

    Plot_Layer( plotter, aLayer, aTraceMode );
    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();

    return true;
}
