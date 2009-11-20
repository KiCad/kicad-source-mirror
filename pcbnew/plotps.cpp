/*************************/
/**** Plot Postscript ****/
/*************************/

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"


/* Generate a PostScript file (*. ps) of the circuit layer.
 * If layer < 0: all layers are plotted.
 */
void WinEDA_BasePcbFrame::Genere_PS( const wxString& FullFileName, int Layer,
                                     bool useA4, GRTraceMode trace_mode )
{
    wxSize        SheetSize;
    wxSize        PaperSize;
    wxSize        BoardSize;
    wxPoint       BoardCenter;
    bool          Center = FALSE;
    Ki_PageDescr* currentsheet = GetScreen()->m_CurrentSheetDesc;
    double        scale, paperscale;
    Ki_PageDescr* SheetPS;
    wxPoint       offset;

    ClearMsgPanel();

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
        Center = TRUE;        // Scale != 1 so center plot.

    // Set default line width
    if( g_pcb_plot_options.PlotLine_Width < 1 )
        g_pcb_plot_options.PlotLine_Width = 1;

    SheetSize.x = currentsheet->m_Size.x * U_PCB;
    SheetSize.y = currentsheet->m_Size.y * U_PCB;

    if( useA4 )
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

    m_Pcb->ComputeBoundaryBox();
    BoardSize   = m_Pcb->m_BoundaryBox.GetSize();
    BoardCenter = m_Pcb->m_BoundaryBox.Centre();

    if( g_pcb_plot_options.PlotScaleOpt == 0 )       // Optimum scale
    {
        double Xscale, Yscale;

        // Fit to 80% of the page
        Xscale = (PaperSize.x * 0.8) / BoardSize.x;
        Yscale = (PaperSize.y * 0.8) / BoardSize.y;
        scale  = MIN( Xscale, Yscale );
    }
    else
        scale = g_pcb_plot_options.Scale * paperscale;

    if( Center )
    {
        offset.x = BoardCenter.x - ( PaperSize.x / 2 ) / scale;
        offset.y = BoardCenter.y - ( PaperSize.y / 2 ) / scale;
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    PS_PLOTTER* plotter = new PS_PLOTTER();
    plotter->set_paper_size( SheetPS );
    plotter->set_scale_adjust( g_pcb_plot_options.ScaleAdjX,
                               g_pcb_plot_options.ScaleAdjY );
    plotter->set_viewport( offset, scale,
                           g_pcb_plot_options.PlotOrient );
    plotter->set_default_line_width( g_pcb_plot_options.PlotLine_Width );
    plotter->set_creator( wxT( "PCBNEW-PS" ) );
    plotter->set_filename( FullFileName );
    plotter->start_plot( output_file );

    /* The worksheet is not significant with scale!=1... It is with
     * paperscale!=1, anyway */
    if( g_pcb_plot_options.Plot_Frame_Ref && !Center )
        PlotWorkSheet( plotter, GetScreen() );

    // If plot a negative board:
    // Draw a black rectangle (background for plot board in white)
    // and switch the current color to WHITE
    if( g_pcb_plot_options.Plot_PS_Negative )
    {
        int margin = 500;              // Add a 0.5 inch margin around the board
        plotter->set_negative( true );
        plotter->set_color( WHITE );   // Which will be plotted as black
        plotter->rect( wxPoint( m_Pcb->m_BoundaryBox.GetX() - margin,
                                m_Pcb->m_BoundaryBox.GetY() - margin ),
                       wxPoint( m_Pcb->m_BoundaryBox.GetRight() + margin,
                                m_Pcb->m_BoundaryBox.GetBottom() + margin ),
                       FILLED_SHAPE );
        plotter->set_color( BLACK );
    }

    Plot_Layer( plotter, Layer, trace_mode );
    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();
}
