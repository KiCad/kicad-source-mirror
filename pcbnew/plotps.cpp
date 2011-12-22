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
    const PAGE_INFO&    pageInfo = GetPageSettings();

    wxSize      paperSizeIU;
    wxSize      boardSize;
    wxPoint     boardCenter;
    bool        center = false;
    double      scale;
    double      paperscale;
    wxPoint     offset;
    LOCALE_IO   toggle;
    PAGE_INFO   pageA4( wxT( "A4" ) );

    const PAGE_INFO*  sheetPS;

    FILE* output_file = wxFopen( aFullFileName, wxT( "wt" ) );

    if( output_file == NULL )
    {
        return false;
    }

    if( g_PcbPlotOptions.m_PlotScale != 1.0 || g_PcbPlotOptions.m_AutoScale )
    {
        // when scale != 1.0 we must calculate the position in page
        // because actual position has no meaning
        center = true;
    }

    // Set default line width
    if( g_PcbPlotOptions.m_PlotLineWidth < 1 )
        g_PcbPlotOptions.m_PlotLineWidth = 1;

    wxSize pageSizeIU = GetPageSizeIU();

    if( aUseA4 )
    {
        sheetPS     = &pageA4;
        paperSizeIU = pageA4.GetSizeIU();
        paperscale  = (double) paperSizeIU.x / pageSizeIU.x;
    }
    else
    {
        sheetPS     = &pageInfo;
        paperSizeIU = pageSizeIU;
        paperscale = 1;
    }

    EDA_RECT bbbox = GetBoardBoundingBox();

    boardSize   = bbbox.GetSize();
    boardCenter = bbbox.Centre();

    if( g_PcbPlotOptions.m_AutoScale )       // Optimum scale
    {
        // Fit to 80% of the page
        double Xscale = (paperSizeIU.x * 0.8) / boardSize.x;
        double Yscale = (paperSizeIU.y * 0.8) / boardSize.y;

        scale  = MIN( Xscale, Yscale );
    }
    else
    {
        scale = g_PcbPlotOptions.m_PlotScale * paperscale;
    }

    if( center )
    {
        offset.x = wxRound( (double) boardCenter.x - ( (double) paperSizeIU.x / 2.0 ) / scale );
        offset.y = wxRound( (double) boardCenter.y - ( (double) paperSizeIU.y / 2.0 ) / scale );
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    PS_PLOTTER* plotter = new PS_PLOTTER();

    plotter->SetPageSettings( *sheetPS );

    plotter->set_scale_adjust( g_PcbPlotOptions.m_FineScaleAdjustX,
                               g_PcbPlotOptions.m_FineScaleAdjustY );
    plotter->set_viewport( offset, scale, g_PcbPlotOptions.m_PlotMirror );
    plotter->set_default_line_width( g_PcbPlotOptions.m_PlotLineWidth );
    plotter->set_creator( wxT( "PCBNEW-PS" ) );
    plotter->set_filename( aFullFileName );
    plotter->start_plot( output_file );

    /* The worksheet is not significant with scale!=1... It is with paperscale!=1, anyway */
    if( g_PcbPlotOptions.m_PlotFrameRef && !center )
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

    return true;
}
