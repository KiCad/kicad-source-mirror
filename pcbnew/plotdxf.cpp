/**
 * @file plotdxf.cpp
 * @brief Plot DXF.
 */

#include <fctsys.h>
#include <common.h>
#include <plot_common.h>
#include <confirm.h>
#include <trigo.h>
#include <wxBasePcbFrame.h>

#include <pcbnew.h>
#include <protos.h>
#include <pcbplot.h>


bool PCB_BASE_FRAME::ExportToDxfFile( const wxString& aFullFileName, int aLayer,
                                      EDA_DRAW_MODE_T aTraceMode )
{
    LOCALE_IO   toggle;

    const PCB_PLOT_PARAMS& plot_opts = GetPlotSettings();

    FILE* output_file = wxFopen( aFullFileName, wxT( "wt" ) );

    if( output_file == NULL )
    {
        return false;
    }

    DXF_PLOTTER* plotter = new DXF_PLOTTER();
    plotter->SetPageSettings( GetPageSettings() );
    plotter->SetViewport( wxPoint( 0, 0 ), IU_PER_DECIMILS, 1, 0 );
    plotter->SetCreator( wxT( "PCBNEW-DXF" ) );
    plotter->SetFilename( aFullFileName );
    plotter->StartPlot( output_file );

    if( plot_opts.m_PlotFrameRef )
        PlotWorkSheet( plotter, GetScreen() );

    Plot_Layer( plotter, aLayer, aTraceMode );
    plotter->EndPlot();
    delete plotter;
    return true;
}
