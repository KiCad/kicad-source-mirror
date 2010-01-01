/******************/
/* file pcbplot.h */
/******************/

#ifndef PCBPLOT_H
#define PCBPLOT_H

/* Shared Config keys for plot and print */
#define OPTKEY_PLOT_LINEWIDTH_VALUE  wxT( "PlotLineWidth" )
#define OPTKEY_LAYERBASE             wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_X_FINESCALE_ADJ wxT( "PrintXFineScaleAdj" )
#define OPTKEY_PRINT_Y_FINESCALE_ADJ wxT( "PrintYFineScaleAdj" )
#define OPTKEY_PRINT_SCALE           wxT( "PrintScale" )
#define OPTKEY_PRINT_PAGE_FRAME      wxT( "PrintPageFrame" )
#define OPTKEY_PRINT_MONOCHROME_MODE wxT( "PrintMonochrome" )

/* Plot Options : */
struct PCB_Plot_Options
{
    bool Exclude_Edges_Pcb;
    int PlotLine_Width;
    bool Plot_Frame_Ref;       // True to plot/print frame references
    int Plot_Mode;
    bool Plot_Set_MIROIR;
    bool Sel_Rotate_Window;
    int HPGL_Pen_Num;
    int HPGL_Pen_Speed;
    int HPGL_Pen_Diam;
    int HPGL_Pen_Recouvrement;
    bool HPGL_Org_Centre;      // TRUE if, HPGL originally the center of the node
    int PlotPSColorOpt;        // True for color Postscript output
    bool Plot_PS_Negative;     // True to create a negative board ps plot

    /* id for plot format (see enum PlotFormat in plot_common.h) */
    int PlotFormat;
    int PlotOrient;
    int PlotScaleOpt;
    int DrillShapeOpt;
    double         Scale;
    double         ScaleAdjX;
    double         ScaleAdjY;
};
extern PCB_Plot_Options g_pcb_plot_options;

#endif  // ifndef PCBPLOT_H
