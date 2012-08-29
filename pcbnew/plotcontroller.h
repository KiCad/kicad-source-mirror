/**
 * @file pcbnew/pcbplot.h
 */

#ifndef PLOTCONTROLLER_H_
#define PLOTCONTROLLER_H_

#include <pcb_plot_params.h>
#include <layers_id_colors_and_visibility.h>

class PLOTTER;
class BOARD;

/** 
 * Batch plotter state object. Keeps the plot options and handles multiple
 * plot requests
 */
class PLOT_CONTROLLER
{
public:
    PLOT_CONTROLLER( BOARD *aBoard );
    ~PLOT_CONTROLLER();
    PCB_PLOT_PARAMS *AccessPlotOpts() { return &m_plotOpts; }
    bool IsPlotOpen() const { return m_plotter != NULL; }
    void ClosePlot();
    bool OpenPlotfile( const wxString &aSuffix, PlotFormat aFormat,
                       const wxString &aSheetDesc );
    bool PlotLayer( int layer );

private:
    /// Option bank
    PCB_PLOT_PARAMS m_plotOpts;
    
    /// This is the plotter object; it starts NULL and become instantiated
    // when a plotfile is requested
    PLOTTER *m_plotter;

    /// The board we're plotting
    BOARD* m_board;
};

#endif

