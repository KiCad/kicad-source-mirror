
/*********************************************************/
/****Function to plot a board in GERBER RS274X format ****/
/*********************************************************/

/* Creates the output files, one per board layer:
 * filenames are like xxxc.PHO and use the RS274X format
 * Units = inches
 * format 3.4, Leading zero omitted, Abs format
 * format 3.4 uses the native pcbnew units (1/10000 inch).
 */

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"

/********************************************************************************/
void WinEDA_BasePcbFrame::Genere_GERBER( const wxString& FullFileName, int Layer,
                                         bool PlotOriginIsAuxAxis,
                                         GRTraceMode trace_mode )
/********************************************************************************/

/* Creates the output files, one per board layer:
 * filenames are like xxxc.PHO and use the RS274X format
 * Units = inches
 * format 3.4, Leading zero omitted, Abs format
 * format 3.4 uses the native pcbnew units (1/10000 inch).
 */
{
    wxPoint offset;

    MsgPanel->EraseMsgBox();

    /* Calculate scaling from pcbnew units (in 0.1 mil or 0.0001 inch) to gerber units */
    double scale = g_pcb_plot_options.Scale;

    if( PlotOriginIsAuxAxis )
        offset = m_Auxiliary_Axis_Position;
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    FILE* output_file = wxFopen( FullFileName, wxT( "wt" ) );
    if( output_file == NULL )
    {
        wxString msg = _( "unable to create file " ) + FullFileName;
        DisplayError( this, msg );
        return;
    }

    SetLocaleTo_C_standard();
    PLOTTER* plotter = new GERBER_PLOTTER();
    /* No mirror and scaling for gerbers! */
    plotter->set_viewport( offset, scale, 0 );
    plotter->set_default_line_width( g_pcb_plot_options.PlotLine_Width );
    plotter->set_creator( wxT( "PCBNEW-RS274X" ) );
    plotter->set_filename( FullFileName );

    MsgPanel->AppendMessage( _( "File" ), FullFileName, CYAN );

    plotter->start_plot( output_file );

    // Sheet refs on gerber CAN be useful... and they're always 1:1
    if( g_pcb_plot_options.Plot_Frame_Ref )
        PlotWorkSheet( plotter, GetScreen() );
    Plot_Layer( plotter, Layer, trace_mode );

    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();
}
