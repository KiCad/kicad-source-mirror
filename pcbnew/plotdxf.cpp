/*******************************/
/**** Routine de trace HPGL ****/
/*******************************/

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"

/*****************************************************************************/
void WinEDA_BasePcbFrame::Genere_DXF( const wxString& FullFileName, int Layer,
                                       GRTraceMode trace_mode )
/*****************************************************************************/
{
    Ki_PageDescr* currentsheet = GetScreen()->m_CurrentSheetDesc;

    MsgPanel->EraseMsgBox();

    FILE* output_file = wxFopen( FullFileName, wxT( "wt" ) );
    if( output_file == NULL )
    {
        wxString msg = _( "Unable to create file " ) + FullFileName;
        DisplayError( this, msg );
        return;
    }

    SetLocaleTo_C_standard();
    MsgPanel->AppendMessage( _( "File" ), FullFileName, CYAN );

    DXF_PLOTTER* plotter = new DXF_PLOTTER();
    plotter->set_paper_size( currentsheet );
    plotter->set_viewport( wxPoint(0,0), 1, 0 );
    plotter->set_creator( wxT( "PCBNEW-DXF" ) );
    plotter->set_filename( FullFileName );
    plotter->start_plot( output_file );

    if( g_pcb_plot_options.Plot_Frame_Ref )
        PlotWorkSheet( plotter, GetScreen() );

    Plot_Layer( plotter, Layer, trace_mode );
    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();
}
