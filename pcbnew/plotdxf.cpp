/******************/
/**** Plot DXF ****/
/******************/

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"

bool WinEDA_BasePcbFrame::Genere_DXF( const wxString& FullFileName, int Layer,
                                      GRTraceMode trace_mode )
{
    Ki_PageDescr* currentsheet = GetScreen()->m_CurrentSheetDesc;

    FILE* output_file = wxFopen( FullFileName, wxT( "wt" ) );
    if( output_file == NULL )
    {
        return false;
    }

    SetLocaleTo_C_standard();

    DXF_PLOTTER* plotter = new DXF_PLOTTER();
    plotter->set_paper_size( currentsheet );
    plotter->set_viewport( wxPoint( 0, 0 ), 1, 0 );
    plotter->set_creator( wxT( "PCBNEW-DXF" ) );
    plotter->set_filename( FullFileName );
    plotter->start_plot( output_file );

    if( g_PcbPlotOptions.m_PlotFrameRef )
        PlotWorkSheet( plotter, GetScreen() );

    Plot_Layer( plotter, Layer, trace_mode );
    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();

    return true;
}
