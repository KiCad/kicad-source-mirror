/**
 * @file pcbnew/pcbplot.h
 */

#ifndef PCBPLOT_H_
#define PCBPLOT_H_

#include <wx/filename.h>
#include <pad_shapes.h>
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
#define OPTKEY_PRINT_PAGE_PER_LAYER  wxT( "PrintSinglePage" )
#define OPTKEY_PRINT_PADS_DRILL      wxT( "PrintPadsDrillOpt" )
#define OPTKEY_PLOT_X_FINESCALE_ADJ  wxT( "PlotXFineScaleAdj" )
#define OPTKEY_PLOT_Y_FINESCALE_ADJ  wxT( "PlotYFineScaleAdj" )
#define CONFIG_PS_FINEWIDTH_ADJ      wxT( "PSPlotFineWidthAdj" )

// Define min and max reasonable values for plot/print scale
#define PLOT_MIN_SCALE 0.01
#define PLOT_MAX_SCALE 100.0

// Small drill marks (small pad holes) diameter value
#define SMALL_DRILL (int)( 0.35 * IU_PER_MM )


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

    /**
     * @return a 'width adjustment' for the postscript engine
     * (useful for controlling toner bleeding during direct transfer)
     * addded to track width and via/pads size
     */
    int getFineWidthAdj()
    {
        if( GetFormat() == PLOT_FORMAT_POST )
            return GetWidthAdjust();
        else
            return 0;
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
     * Plot a pad.
     * unlike other items, a pad had not a specific color,
     * and be drawn as a non filled item although the plot mode is filled
     * color and plot mode are needed by this function
     */
    void PlotPad( D_PAD* aPad, EDA_COLOR_T aColor, EDA_DRAW_MODE_T aPlotMode );

    /**
     * plot items like text and graphics,
     *  but not tracks and modules
     */
    void PlotBoardGraphicItems();

    /** Function PlotDrillMarks
     * Draw a drill mark for pads and vias.
     * Must be called after all drawings, because it
     * redraw the drill mark on a pad or via, as a negative (i.e. white) shape in
     * FILLED plot mode (for PS and PDF outputs)
     */
    void PlotDrillMarks();

    /**
     * Function getColor
     * @return the layer color
     * @param aLayer = the layer id
     * White color is special: cannot be seen on a white paper
     * and in B&W mode, is plotted as white but other colors are plotted in BLACK
     * so the returned color is LIGHTGRAY when the layer color is WHITE
     */
    EDA_COLOR_T getColor( int aLayer );

private:
    /** Helper function to plot a single drill mark. It compensate and clamp
     * the drill mark size depending on the current plot options
     */
    void plotOneDrillMark( PAD_SHAPE_T aDrillShape,
                           const wxPoint& aDrillPos, wxSize aDrillSize,
                           const wxSize& aPadSize,
                           double aOrientation, int aSmallDrill );

};

PLOTTER* StartPlotBoard( BOARD* aBoard,
                         PCB_PLOT_PARAMS* aPlotOpts,
                         const wxString& aFullFileName,
                         const wxString& aSheetDesc );

/**
 * Function PlotOneBoardLayer
 * main function to plot one copper or technical layer.
 * It prepare options and calls the specilized plot function,
 * according to the layer type
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayer = the layer id to plot
 * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
 */
void PlotOneBoardLayer( BOARD* aBoard, PLOTTER* aPlotter, int aLayer,
                        const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Function PlotStandardLayer
 * plot copper or technical layers.
 * not used for silk screen layers, because these layers have specific
 * requirements, mainly for pads
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayerMask = the mask to define the layers to plot
 * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
 *
 * aPlotOpt has 3 important options to controle this plot,
 * which are set, depending on the layer typpe to plot
 *      SetEnablePlotVia( bool aEnable )
 *          aEnable = true to plot vias, false to skip vias (has meaning
 *                      only for solder mask layers).
 *      SetSkipPlotNPTH_Pads( bool aSkip )
 *          aSkip = true to skip NPTH Pads, when the pad size and the pad hole
 *                  have the same size. Used in GERBER format only.
 *      SetDrillMarksType( DrillMarksType aVal ) controle the actual hole:
 *              no hole, small hole, actual hole
 */
void PlotStandardLayer( BOARD* aBoard, PLOTTER* aPlotter, long aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt );

/**
 * Function PlotSilkScreen
 * plot silkscreen layers which have specific requirements, mainly for pads.
 * Should not be used for other layers
 * @param aBoard = the board to plot
 * @param aPlotter = the plotter to use
 * @param aLayerMask = the mask to define the layers to plot (silkscreen Front and/or Back)
 * @param aPlotOpt = the plot options (files, sketch). Has meaning for some formats only
 */
void PlotSilkScreen( BOARD* aBoard, PLOTTER* aPlotter, long aLayerMask,
                     const PCB_PLOT_PARAMS&  aPlotOpt );


/**
 * Function EnsureOutputDirectory (helper function)
 * Fix the output directory pathname to absolute and ensure it exists
 * (Creates it if not exists)
 * @param aOutputDir = the wxFileName to modify
 *          (contains the absolute or relative to the board path
 * @param aBoardFilename = the board full filename
 * @param aMessageBox = a wxMessageBox to show meesage (can be NULL)
 */
bool EnsureOutputDirectory( wxFileName* aOutputDir,
                            const wxString& aBoardFilename,
                            wxTextCtrl* aMessageBox );

/**
 * Function BuildPlotFileName (helper function)
 * Complete a plot filename: forces the output directory,
 * add a suffix to the name and sets the specified extension
 * the suffix is usually the layer name
 * replaces not allowed chars in suffix by '_'
 * @param aFilename = the wxFileName to initialize
 *                  Contians the base filename
 * @param aOutputDir = the path
 * @param aSuffix = the suffix to add to the base filename
 * @param aExtension = the file extension
 */
void BuildPlotFileName( wxFileName* aFilename,
                               const wxString& aOutputDir,
                               const wxString& aSuffix,
                               const wxString& aExtension );

// PLOTGERB.CPP
void SelectD_CODE_For_LineDraw( PLOTTER* plotter, int aSize );

#endif // PCBPLOT_H_
