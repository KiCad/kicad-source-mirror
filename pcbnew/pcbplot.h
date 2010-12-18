/************/
/* pcbplot.h*/
/************/

#ifndef PCBPLOT_H
#define PCBPLOT_H


/* Shared Config keys for plot and print */
#define OPTKEY_LAYERBASE             wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_X_FINESCALE_ADJ wxT( "PrintXFineScaleAdj" )
#define OPTKEY_PRINT_Y_FINESCALE_ADJ wxT( "PrintYFineScaleAdj" )
#define OPTKEY_PRINT_SCALE           wxT( "PrintScale" )
#define OPTKEY_PRINT_MODULE_SCALE    wxT( "PrintModuleScale" )
#define OPTKEY_PRINT_PAGE_FRAME      wxT( "PrintPageFrame" )
#define OPTKEY_PRINT_MONOCHROME_MODE wxT( "PrintMonochrome" )
#define OPTKEY_PRINT_PADS_DRILL      wxT( "PrintPadsDrillOpt" )

/* Conversion unit constants. */
/* Convert pcb dimension of 0.1 mil to PS units of inches. */
#define SCALE_PS .0001
/* Convert dimension 0.1 mil -> HPGL units: */
#define SCALE_HPGL 0.102041

// Small drill marks diameter value (in internal value = 1/10000 inch)
#define SMALL_DRILL 150

/* a helper class to handle plot parameters and options when plotting/printing a board
*/
class PCB_PLOT_PARAMS
{
public:
    bool        m_ExcludeEdgeLayer;     // True: do not plot edge layer when plotting other layers
                                        // False: Edge layer always plotted (merged) when plotting other layers
    int         m_PlotLineWidth;
    bool        m_PlotFrameRef;         // True to plot/print frame references
    bool        m_PlotViaOnMaskLayer;   // True if vias are drawn on Mask layer
                                        // (ie protected by mask)
    GRTraceMode m_PlotMode;             // = FILAIRE, FILLED or SKETCH: select how to plot filled objects.
                                        // depending on plot format or layers, all options are not always allowed
    int         m_HPGLPenNum;
    int         m_HPGLPenSpeed;
    int         m_HPGLPenDiam;
    int         m_HPGLPenOvr;
    int         m_PlotPSColorOpt;       // True for color Postscript output
    bool        m_PlotPSNegative;       // True to create a  negative board ps plot

    /* Flags to enable or disable ploting of various PCB elements. */
    bool        m_PlotReference;
    bool        m_PlotValue;
    bool        m_PlotTextOther;
    bool        m_PlotInvisibleTexts;
    bool        m_PlotPadsOnSilkLayer; ///< allows pads outlines on silkscreen layer (when pads are also o, silk screen
    bool        m_SubtractMaskFromSilk;

    /// id for plot format (see enum PlotFormat in plot_common.h) */
    int         m_PlotFormat;           // Gerber, HPGL ...
    bool        m_PlotMirror;

    enum DrillShapeOptT {
        NO_DRILL_SHAPE    = 0,
        SMALL_DRILL_SHAPE = 1,
        FULL_DRILL_SHAPE  = 2
    };
    DrillShapeOptT m_DrillShapeOpt;     // For postscript output: holes can be not plotted,
                                        // or have a small size or plotted with their actual size
    bool        m_AutoScale;            // If true, use the better scale to fit in page
    double      m_PlotScale;            // The global scale factor. a 1.0 scale factor plot a board
                                        // with its actual size.
    // These next two scale factors are intended to compensable plotters (and mainly printers) X and Y scale error.
    // Therefore they are expected very near 1.0
    // Only X and Y dimensions are adjusted: circles are plotted as circle, even if X and Y fine scale differ.
    double      m_FineScaleAdjustX;     // fine scale adjust X axis
    double      m_FineScaleAdjustY;     // dine scale adjust Y axis

private:
    wxString outputDirectory;

public:
    PCB_PLOT_PARAMS();
    void        SetOutputDirectory( wxString aDir ) { outputDirectory = aDir; };
    wxString    GetOutputDirectory() { return outputDirectory; };
    void        SetSubtractMaskFromSilk( bool aSubtract ) { m_SubtractMaskFromSilk = aSubtract; };
    bool        GetSubtractMaskFromSilk() { return m_SubtractMaskFromSilk; };
};

extern PCB_PLOT_PARAMS g_PcbPlotOptions;


void PlotTextePcb( PLOTTER* plotter, TEXTE_PCB* pt_texte, int masque_layer,
                   GRTraceMode trace_mode );

/* Plat PCB text type, ie other than text on modules
 * prepare the plot settings of text */
void PlotDrawSegment( PLOTTER* plotter, DRAWSEGMENT* PtSegm, int masque_layer,
                      GRTraceMode trace_mode );

void PlotDimension( PLOTTER* plotter, DIMENSION* Dimension, int masque_layer,
                    GRTraceMode trace_mode );

void PlotMirePcb( PLOTTER* plotter, MIREPCB* PtMire, int masque_layer,
                  GRTraceMode trace_mode );

void Plot_1_EdgeModule( PLOTTER* plotter, EDGE_MODULE* PtEdge,
                        GRTraceMode trace_mode );

void PlotFilledAreas( PLOTTER* plotter, ZONE_CONTAINER* aZone,
                      GRTraceMode trace_mode );

/* PLOTGERB.CPP */
void SelectD_CODE_For_LineDraw( PLOTTER* plotter, int aSize );


#endif  /* #define PCBPLOT_H */
