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
class MODULE;
class EDGE_MODULE;
class PCB_TARGET;
class TEXTE_MODULE;
class ZONE_CONTAINER;
class BOARD;


// Shared Config keys for plot and print
#define OPTKEY_LAYERBASE             wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_X_FINESCALE_ADJ wxT( "PrintXFineScaleAdj" )
#define OPTKEY_PRINT_Y_FINESCALE_ADJ wxT( "PrintYFineScaleAdj" )
#define OPTKEY_PRINT_SCALE           wxT( "PrintScale" )
#define OPTKEY_PRINT_MODULE_SCALE    wxT( "PrintModuleScale" )
#define OPTKEY_PRINT_PAGE_FRAME      wxT( "PrintPageFrame" )
#define OPTKEY_PRINT_MONOCHROME_MODE wxT( "PrintMonochrome" )
#define OPTKEY_PRINT_PADS_DRILL      wxT( "PrintPadsDrillOpt" )
#define OPTKEY_PLOT_X_FINESCALE_ADJ  wxT( "PlotXFineScaleAdj" )
#define OPTKEY_PLOT_Y_FINESCALE_ADJ  wxT( "PlotYFineScaleAdj" )
#define CONFIG_PS_FINEWIDTH_ADJ      wxT( "PSPlotFineWidthAdj" )

// Define min and max reasonable values for plot/print scale
#define PLOT_MIN_SCALE 0.01
#define PLOT_MAX_SCALE 100.0

// Conversion unit constants.
// Convert pcb dimension of 0.1 mil to PS units of inches.
#define SCALE_PS    .0001

// Convert dimension 0.1 mil -> HPGL units:
#define SCALE_HPGL  0.102041

// Small drill marks diameter value (in 1/10000 inch)
#define SMALL_DRILL 150
// A helper class to plot board items
class BRDITEMS_PLOTTER: public PCB_PLOT_PARAMS
{
    PLOTTER* m_plotter;
    BOARD* m_board;
    int m_layerMask;

public:
    BRDITEMS_PLOTTER( PLOTTER* aPlotter, BOARD* aBoard, const PCB_PLOT_PARAMS& aPlotOpts )
        : PCB_PLOT_PARAMS( aPlotOpts )
    {
        m_plotter = aPlotter;
        m_board = aBoard;
        m_layerMask = 0;
    }

    // Basic functions to plot a board item
    void SetLayerMask( int aLayerMask ){ m_layerMask = aLayerMask; }
    void Plot_Edges_Modules();
    void Plot_1_EdgeModule( EDGE_MODULE* aEdge );
    void PlotTextModule( TEXTE_MODULE* aTextMod, EDA_COLOR_T aColor );
    bool PlotAllTextsModule( MODULE* aModule );
    void PlotDimension( DIMENSION* Dimension );
    void PlotPcbTarget( PCB_TARGET* PtMire );
    void PlotFilledAreas( ZONE_CONTAINER* aZone );
    void PlotTextePcb( TEXTE_PCB* pt_texte );
    void PlotDrawSegment( DRAWSEGMENT* PtSegm );

    /**
     * Function getColor
     * @return the layer color
     * @param aLayer = the layer id
     * White color is special: cannot be seen on a white paper
     * and in B&W mode, is plotted as white but other colors are plotted in BLACK
     * so the returned color is LIGHTGRAY when the layer color is WHITE
     */
    EDA_COLOR_T getColor( int aLayer );
};

PLOTTER *StartPlotBoard( BOARD *aBoard,
                         PCB_PLOT_PARAMS *aPlotOpts,
                         const wxString& aFullFileName,
                         const wxString& aSheetDesc );

/**
 * Function PlotBoardLayer
 * main function to plot copper or technical layers.
 * It calls the specilize plot function, according to the layer type
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayer = the layer id to plot
 * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
 */
void PlotBoardLayer( BOARD *aBoard, PLOTTER* aPlotter, int aLayer,
                     const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Function Plot_Standard_Layer
 * plot copper or technical layers.
 * not used for silk screen layers, because these layers have specific
 * requirements, mainly for pads
 * @param aBoard = the board to plot
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

/**
 * Function PlotSilkScreen
 * plot silkscreen layers which have specific requirements, mainly for pads.
 * Should not be used for other layers
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayerMask = the mask to define the layers to plot (silkscreen Front and/or Back)
 * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
 */
void PlotSilkScreen( BOARD *aBoard, PLOTTER* aPlotter, long aLayerMask,
                     const PCB_PLOT_PARAMS&  aPlotOpt );

/**
    * Function PlotDrillMarks
    * Draw a drill mark for pads and vias.
    * Must be called after all drawings, because it
    * redraw the drill mark on a pad or via, as a negative (i.e. white) shape
    * in FILLED plot mode
    * @param aBoard = the board to plot
    * @param aPlotter = the PLOTTER
    * @param aPlotOpts = plot options
    */
void PlotDrillMarks( BOARD *aBoard, PLOTTER* aPlotter, const PCB_PLOT_PARAMS& aPlotOpts );


// PLOTGERB.CPP
void SelectD_CODE_For_LineDraw( PLOTTER* plotter, int aSize );

#endif // PCBPLOT_H_
