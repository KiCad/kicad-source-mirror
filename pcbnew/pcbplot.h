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

/* Plot Options : */
class PCB_Plot_Options
{
public:
    bool        Exclude_Edges_Pcb;
    int         PlotLine_Width;
    bool        Plot_Frame_Ref;     // True to plot/print frame references
    bool        DrawViaOnMaskLayer; // True if vias are drawn on Mask layer
                                    // (ie protected by mask)
    GRTraceMode Trace_Mode;
    bool        Plot_Set_MIROIR;
    int         HPGL_Pen_Num;
    int         HPGL_Pen_Speed;
    int         HPGL_Pen_Diam;
    int         HPGL_Pen_Recouvrement;
    int         PlotPSColorOpt;     // True for color Postscript output
    bool        Plot_PS_Negative;   // True to create a  negative board ps plot

    /* Flags to enable or disable ploting of various PCB elements. */
    bool        Sel_Texte_Reference;
    bool        Sel_Texte_Valeur;
    bool        Sel_Texte_Divers;
    bool        Sel_Texte_Invisible;
    bool        PlotPadsOnSilkLayer; /* allows pads on silkscreen */
    bool        m_SubtractMaskFromSilk;

    /* id for plot format (see enum PlotFormat in plot_common.h) */
    int PlotFormat;
    int PlotOrient;
    int PlotScaleOpt;
    enum DrillShapeOptT {
        NO_DRILL_SHAPE    = 0,
        SMALL_DRILL_SHAPE = 1,
        FULL_DRILL_SHAPE  = 2
    };
    DrillShapeOptT DrillShapeOpt;
    double         Scale;
    double         ScaleAdjX;
    double         ScaleAdjY;

private:
    wxString       outputDirectory;

public:
    PCB_Plot_Options();
    void SetOutputDirectory( wxString aDir ) { outputDirectory = aDir; };
    wxString GetOutputDirectory() { return outputDirectory; };
    void SetSubtractMaskFromSilk( bool aSubtract ) { m_SubtractMaskFromSilk = aSubtract; };
    bool GetSubtractMaskFromSilk() { return m_SubtractMaskFromSilk; };
};

extern PCB_Plot_Options g_pcb_plot_options;


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
