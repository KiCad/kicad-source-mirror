/**
 * @file plotgerb.cpp
 * @brief Functions to plot a board in GERBER RS274X format.
 */

/* Creates the output files, one per board layer:
 * filenames are like xxxc.PHO and use the RS274X format
 * Units = inches
 * format 3.4, Leading zero omitted, Abs format
 * format 3.4 uses the native Pcbnew units (1/10000 inch).
 */

#include <fctsys.h>
#include <common.h>
#include <plot_common.h>
#include <confirm.h>
#include <pcbplot.h>
#include <trigo.h>
#include <wxBasePcbFrame.h>
#include <layers_id_colors_and_visibility.h>

#include <pcbnew.h>
#include <protos.h>


bool PCB_BASE_FRAME::ExportToGerberFile( const wxString& aFullFileName, int aLayer,
                                         bool aPlotOriginIsAuxAxis, EDA_DRAW_MODE_T aTraceMode )
{
    FILE* output_file = wxFopen( aFullFileName, wxT( "wt" ) );
    if( output_file == NULL )
    {
        return false;
    }

    PCB_PLOT_PARAMS plot_opts = GetPlotSettings();

    wxPoint offset;

    // Calculate scaling from Pcbnew units (in 0.1 mil or 0.0001 inch) to gerber units
    double scale = plot_opts.m_PlotScale;

    if( aPlotOriginIsAuxAxis )
    {
        offset = GetOriginAxisPosition();
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    LOCALE_IO   toggle;

    PLOTTER* plotter = new GERBER_PLOTTER();

    // No mirror and scaling for gerbers!
    plotter->SetViewport( offset, IU_PER_DECIMILS, scale, 0 );
    plotter->SetDefaultLineWidth( plot_opts.m_PlotLineWidth );
    plotter->SetCreator( wxT( "PCBNEW-RS274X" ) );
    plotter->SetFilename( aFullFileName );

    if( plotter->StartPlot( output_file ) )
    {
        // Skip NPTH pads on copper layers
        // ( only if hole size == pad size ):
        if( (aLayer >= LAYER_N_BACK) && (aLayer <= LAYER_N_FRONT) )
            plot_opts.m_SkipNPTH_Pads = true;

        SetPlotSettings( plot_opts );

        // Sheet refs on gerber CAN be useful... and they're always 1:1
        if( plot_opts.m_PlotFrameRef )
            PlotWorkSheet( plotter, GetScreen(), plot_opts.GetPlotLineWidth() );

        Plot_Layer( plotter, aLayer, aTraceMode );

        plotter->EndPlot();

        plot_opts.m_SkipNPTH_Pads = false;

        SetPlotSettings( plot_opts );
    }
    else    // error in start_plot( ): failed opening a temporary file
    {
        wxMessageBox( _("Error when creating %s file: unable to create a temporary file"));
    }

    delete plotter;

    return true;
}
