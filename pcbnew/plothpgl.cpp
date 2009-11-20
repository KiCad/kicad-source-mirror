/*******************/
/**** Plot HPGL ****/
/*******************/

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"


void WinEDA_BasePcbFrame::Genere_HPGL( const wxString& FullFileName, int Layer,
                                       GRTraceMode trace_mode )
{
    wxSize        SheetSize;
    wxSize        BoardSize;
    wxPoint       BoardCenter;
    bool          Center = FALSE;
    Ki_PageDescr* currentsheet = GetScreen()->m_CurrentSheetDesc;
    double        scale;
    wxPoint       offset;

    ClearMsgPanel();

    // Compute pen_dim (from g_HPGL_Pen_Diam in mils) in pcb units,
    // with plot scale (if Scale is 2, pen diameter is always g_HPGL_Pen_Diam
    // so apparent pen diam is real pen diam / Scale
    int pen_diam = wxRound( (g_pcb_plot_options.HPGL_Pen_Diam * U_PCB) / g_pcb_plot_options.Scale );

    // compute pen_recouvrement (from g_HPGL_Pen_Recouvrement in mils)
    // with plot scale
    if( g_pcb_plot_options.HPGL_Pen_Recouvrement < 0 )
        g_pcb_plot_options.HPGL_Pen_Recouvrement = 0;
    if( g_pcb_plot_options.HPGL_Pen_Recouvrement >= g_pcb_plot_options.HPGL_Pen_Diam )
        g_pcb_plot_options.HPGL_Pen_Recouvrement = g_pcb_plot_options.HPGL_Pen_Diam - 1;
    int   pen_recouvrement = wxRound(
        g_pcb_plot_options.HPGL_Pen_Recouvrement * 10.0 / g_pcb_plot_options.Scale );

    FILE* output_file = wxFopen( FullFileName, wxT( "wt" ) );
    if( output_file == NULL )
    {
        wxString msg = _( "Unable to create file " ) + FullFileName;
        DisplayError( this, msg );
        return;
    }

    SetLocaleTo_C_standard();
    AppendMsgPanel( _( "File" ), FullFileName, CYAN );

    if( g_pcb_plot_options.PlotScaleOpt != 1 )
        Center = TRUE; // Scale != 1 so center PCB plot.


    // Scale units from 0.0001" to HPGL plot units.
    SheetSize.x = currentsheet->m_Size.x * U_PCB;
    SheetSize.y = currentsheet->m_Size.y * U_PCB;

    /* Calculate the center of the PCB. */
    m_Pcb->ComputeBoundaryBox();
    BoardSize   = m_Pcb->m_BoundaryBox.GetSize();
    BoardCenter = m_Pcb->m_BoundaryBox.Centre();

    if( g_pcb_plot_options.PlotScaleOpt == 0 )       // Optimum scale
    {
        double Xscale, Yscale;

        // Fit to 80% of the page
        Xscale = ( ( SheetSize.x * 0.8 ) / BoardSize.x );
        Yscale = ( ( SheetSize.y * 0.8 ) / BoardSize.y );
        scale  = MIN( Xscale, Yscale );
    }
    else
        scale = g_pcb_plot_options.Scale;

    // Calculate the page size offset.
    if( Center )
    {
        offset.x = BoardCenter.x - ( SheetSize.x / 2 ) / scale;
        offset.y = BoardCenter.y - ( SheetSize.y / 2 ) / scale;
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();
    plotter->set_paper_size( currentsheet );
    plotter->set_viewport( offset, scale,
                           g_pcb_plot_options.PlotOrient );
    plotter->set_default_line_width( g_pcb_plot_options.PlotLine_Width );
    plotter->set_creator( wxT( "PCBNEW-HPGL" ) );
    plotter->set_filename( FullFileName );
    plotter->set_pen_speed( g_pcb_plot_options.HPGL_Pen_Speed );
    plotter->set_pen_number( g_pcb_plot_options.HPGL_Pen_Num );
    plotter->set_pen_overlap( pen_recouvrement );
    plotter->set_pen_diameter( pen_diam );
    plotter->start_plot( output_file );

    /* The worksheet is not significant with scale!=1... It is with
     * paperscale!=1, anyway */
    if( g_pcb_plot_options.Plot_Frame_Ref && !Center )
        PlotWorkSheet( plotter, GetScreen() );

    Plot_Layer( plotter, Layer, trace_mode );
    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();
}
