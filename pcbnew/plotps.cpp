/**
 * @file plotps.cpp
 * @brief Plot Postscript.
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


/* Generate a PostScript file (*. ps) of the circuit layer.
 * If layer < 0: all layers are plotted.
 */
bool PCB_BASE_FRAME::ExportToPostScriptFile( const wxString& aFullFileName, int aLayer,
                                             bool aUseA4, EDA_DRAW_MODE_T aTraceMode )
{
    const PAGE_INFO&    pageInfo = GetPageSettings();
    PCB_PLOT_PARAMS     plotOpts = GetPlotSettings();

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

    if( plotOpts.m_PlotScale != 1.0 || plotOpts.m_AutoScale )
    {
        // when scale != 1.0 we must calculate the position in page
        // because actual position has no meaning
        center = true;
    }

    // Set default line width
    if( plotOpts.m_PlotLineWidth < 1 )
        plotOpts.m_PlotLineWidth = 1;

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

    if( plotOpts.m_AutoScale )       // Optimum scale
    {
        // Fit to 80% of the page
        double Xscale = (paperSizeIU.x * 0.8) / boardSize.x;
        double Yscale = (paperSizeIU.y * 0.8) / boardSize.y;

        scale  = MIN( Xscale, Yscale );
    }
    else
    {
        scale = plotOpts.m_PlotScale * paperscale;
    }

    if( center )
    {
        offset.x = KiROUND( (double) boardCenter.x - ( (double) paperSizeIU.x / 2.0 ) / scale );
        offset.y = KiROUND( (double) boardCenter.y - ( (double) paperSizeIU.y / 2.0 ) / scale );
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    PS_PLOTTER* plotter = new PS_PLOTTER();

    plotter->SetPageSettings( *sheetPS );

    // why did we have to change these settings?
    SetPlotSettings( plotOpts );

    plotter->SetScaleAdjust( plotOpts.m_FineScaleAdjustX,
                               plotOpts.m_FineScaleAdjustY );
    plotter->SetPlotWidthAdj( plotOpts.m_FineWidthAdjust );
    plotter->SetViewport( offset, IU_PER_DECIMILS, scale, 
	                  plotOpts.m_PlotMirror );
    plotter->SetDefaultLineWidth( plotOpts.m_PlotLineWidth );
    plotter->SetCreator( wxT( "PCBNEW-PS" ) );
    plotter->SetFilename( aFullFileName );
    plotter->SetPsTextMode( PSTEXTMODE_PHANTOM );
    plotter->StartPlot( output_file );

    /* The worksheet is not significant with scale!=1... It is with paperscale!=1, anyway */
    if( plotOpts.m_PlotFrameRef && !center )
        PlotWorkSheet( plotter, GetScreen() );

    // If plot a negative board:
    // Draw a black rectangle (background for plot board in white)
    // and switch the current color to WHITE
    if( plotOpts.m_PlotPSNegative )
    {
        int margin = 500;              // Add a 0.5 inch margin around the board
        plotter->SetNegative( true );
        plotter->SetColor( WHITE );   // Which will be plotted as black
        plotter->Rect( wxPoint( bbbox.GetX() - margin,
                                bbbox.GetY() - margin ),
                       wxPoint( bbbox.GetRight() + margin,
                                bbbox.GetBottom() + margin ),
                       FILLED_SHAPE );
        plotter->SetColor( BLACK );
    }

    Plot_Layer( plotter, aLayer, aTraceMode );
    plotter->EndPlot();
    delete plotter;

    return true;
}
