/**
 * @file plotps.cpp
 * @brief Plot Postscript.
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


/* Generate a PostScript file (*. ps) of the circuit layer.
 * If layer < 0: all layers are plotted.
 */
bool PCB_BASE_FRAME::ExportToPostScriptFile( const wxString& aFullFileName, int aLayer,
                                             bool aUseA4, GRTraceMode aTraceMode )
{
    wxSize        SheetSize;
    wxSize        PaperSize;
    wxSize        BoardSize;
    wxPoint       BoardCenter;
    bool          Center = false;
    Ki_PageDescr* currentsheet = GetScreen()->m_CurrentSheetDesc;
    double        scale, paperscale;
    Ki_PageDescr* SheetPS;
    wxPoint       offset;

    FILE* output_file = wxFopen( aFullFileName, wxT( "wt" ) );

    if( output_file == NULL )
    {
        return false;
    }

    SetLocaleTo_C_standard();

    if( g_PcbPlotOptions.m_PlotScale != 1.0 || g_PcbPlotOptions.m_AutoScale )
        Center = true;  // when scale != 1.0 we must calculate the position in page
                        // because actual position has no meaning

    // Set default line width
    if( g_PcbPlotOptions.m_PlotLineWidth < 1 )
        g_PcbPlotOptions.m_PlotLineWidth = 1;

    SheetSize.x = currentsheet->m_Size.x * U_PCB;
    SheetSize.y = currentsheet->m_Size.y * U_PCB;

    if( aUseA4 )
    {
        SheetPS     = &g_Sheet_A4;
        PaperSize.x = g_Sheet_A4.m_Size.x * U_PCB;
        PaperSize.y = g_Sheet_A4.m_Size.y * U_PCB;
        paperscale  = (float) PaperSize.x / SheetSize.x;
    }
    else
    {
        SheetPS    = currentsheet;
        PaperSize  = SheetSize;
        paperscale = 1;
    }

    EDA_RECT bbbox = GetBoardBoundingBox();

    BoardSize   = bbbox.GetSize();
    BoardCenter = bbbox.Centre();

    if( g_PcbPlotOptions.m_AutoScale )       // Optimum scale
    {
        double Xscale, Yscale;

        // Fit to 80% of the page
        Xscale = (PaperSize.x * 0.8) / BoardSize.x;
        Yscale = (PaperSize.y * 0.8) / BoardSize.y;
        scale  = MIN( Xscale, Yscale );
    }
    else
    {
        scale = g_PcbPlotOptions.m_PlotScale * paperscale;
    }

    if( Center )
    {
        offset.x = wxRound( (double) BoardCenter.x - ( (double) PaperSize.x / 2.0 ) / scale );
        offset.y = wxRound( (double) BoardCenter.y - ( (double) PaperSize.y / 2.0 ) / scale );
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->set_paper_size( SheetPS );
    plotter->set_scale_adjust( g_PcbPlotOptions.m_FineScaleAdjustX,
                               g_PcbPlotOptions.m_FineScaleAdjustX );
    plotter->set_viewport( offset, scale, g_PcbPlotOptions.m_PlotMirror );
    plotter->set_default_line_width( g_PcbPlotOptions.m_PlotLineWidth );
    plotter->set_creator( wxT( "PCBNEW-PS" ) );
    plotter->set_filename( aFullFileName );
    plotter->start_plot( output_file );

    /* The worksheet is not significant with scale!=1... It is with paperscale!=1, anyway */
    if( g_PcbPlotOptions.m_PlotFrameRef && !Center )
        PlotWorkSheet( plotter, GetScreen() );

    // If plot a negative board:
    // Draw a black rectangle (background for plot board in white)
    // and switch the current color to WHITE
    if( g_PcbPlotOptions.m_PlotPSNegative )
    {
        int margin = 500;              // Add a 0.5 inch margin around the board
        plotter->set_negative( true );
        plotter->set_color( WHITE );   // Which will be plotted as black
        plotter->rect( wxPoint( bbbox.GetX() - margin,
                                bbbox.GetY() - margin ),
                       wxPoint( bbbox.GetRight() + margin,
                                bbbox.GetBottom() + margin ),
                       FILLED_SHAPE );
        plotter->set_color( BLACK );
    }

    Plot_Layer( plotter, aLayer, aTraceMode );
    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();

    return true;
}
