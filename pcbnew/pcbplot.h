/**
 * @file pcbnew/pcbplot.h
 */

#ifndef PCBPLOT_H_
#define PCBPLOT_H_

#include <pcb_plot_params.h>


class PLOTTER;
class TEXTE_PCB;
class DRAWSEGMENT;
class DIMENSION;
class EDGE_MODULE;
class PCB_TARGET;
class ZONE_CONTAINER;


// Shared Config keys for plot and print
#define OPTKEY_LAYERBASE             wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_X_FINESCALE_ADJ wxT( "PrintXFineScaleAdj" )
#define OPTKEY_PRINT_Y_FINESCALE_ADJ wxT( "PrintYFineScaleAdj" )
#define OPTKEY_PRINT_SCALE           wxT( "PrintScale" )
#define OPTKEY_PRINT_MODULE_SCALE    wxT( "PrintModuleScale" )
#define OPTKEY_PRINT_PAGE_FRAME      wxT( "PrintPageFrame" )
#define OPTKEY_PRINT_MONOCHROME_MODE wxT( "PrintMonochrome" )
#define OPTKEY_PRINT_PADS_DRILL      wxT( "PrintPadsDrillOpt" )

// Conversion unit constants.
// Convert pcb dimension of 0.1 mil to PS units of inches.
#define SCALE_PS    .0001

// Convert dimension 0.1 mil -> HPGL units:
#define SCALE_HPGL  0.102041

// Small drill marks diameter value (in internal value = 1/10000 inch)
#define SMALL_DRILL 150


void PlotTextePcb( PLOTTER* plotter, const PCB_PLOT_PARAMS& aPlotOpts, TEXTE_PCB* pt_texte, int masque_layer,
                   EDA_DRAW_MODE_T trace_mode );

void PlotDrawSegment( PLOTTER* plotter, const PCB_PLOT_PARAMS& aPlotOpts, DRAWSEGMENT* PtSegm, int masque_layer,
                      EDA_DRAW_MODE_T trace_mode );

void PlotDimension( PLOTTER* plotter, const PCB_PLOT_PARAMS& aPlotOpts, DIMENSION* Dimension, int masque_layer,
                    EDA_DRAW_MODE_T trace_mode );

void PlotPcbTarget( PLOTTER* plotter, const PCB_PLOT_PARAMS& aPlotOpts, PCB_TARGET* PtMire, int masque_layer,
                    EDA_DRAW_MODE_T trace_mode );

void Plot_1_EdgeModule( PLOTTER* plotter, const PCB_PLOT_PARAMS& aPlotOpts, EDGE_MODULE* PtEdge,
                        EDA_DRAW_MODE_T trace_mode, int masque_layer );

void PlotFilledAreas( PLOTTER* plotter, const PCB_PLOT_PARAMS& aPlotOpts, ZONE_CONTAINER* aZone,
                      EDA_DRAW_MODE_T trace_mode );

PLOTTER *StartPlotBoard( BOARD *aBoard,
                         PCB_PLOT_PARAMS *aPlotOpts,
                         const wxString& aFullFileName,
                         const wxString& aSheetDesc );

void PlotBoardLayer( BOARD *aBoard, PLOTTER* aPlotter, int Layer,
                     const PCB_PLOT_PARAMS& plot_opts );

/**
    * Function Plot_Standard_Layer
    * plot copper or technical layers.
    * not used for silk screen layers, because these layers have specific
    * requirements, mainly for pads
    * @param aPlotter = the plotter to use
    * @param aLayerMask = the mask to define the layers to plot
    * @param aPlotVia = true to plot vias, false to skip vias (has meaning
    *                  only for solder mask layers).
    * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
    * @param aSkipNPTH_Pads = true to skip NPTH Pads, when the pad size and the pad hole
    *                      have the same size. Used in GERBER format only.
    */
void PlotStandardLayer( BOARD *aBoard, PLOTTER* aPlotter, long aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt,
                        bool aPlotVia, bool aSkipNPTH_Pads );

void PlotSilkScreen( BOARD *aBoard, PLOTTER* aPlotter, long aLayerMask,
                     const PCB_PLOT_PARAMS&  plot_opts );

/**
    * Function PlotDrillMarks
    * Draw a drill mark for pads and vias.
    * Must be called after all drawings, because it
    * redraw the drill mark on a pad or via, as a negative (i.e. white) shape
    * in FILLED plot mode
    * @param aPlotter = the PLOTTER
    * @param aPlotOpts = plot options
    */
void PlotDrillMarks( BOARD *aBoard, PLOTTER* aPlotter, const PCB_PLOT_PARAMS& aPlotOpts );


// PLOTGERB.CPP
void SelectD_CODE_For_LineDraw( PLOTTER* plotter, int aSize );

#endif // PCBPLOT_H_
