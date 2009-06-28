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

/* Plot Options : */
struct PCB_Plot_Options {
    bool Exclude_Edges_Pcb;
    int PlotLine_Width;
    bool Plot_Frame_Ref;       // True to plot/print frame references
    bool DrawViaOnMaskLayer;   // True if vias are drawn on Mask layer (ie protected by mask)
    int Plot_Mode;
    bool Plot_Set_MIROIR;
    bool Sel_Rotate_Window;
    int HPGL_Pen_Num;                                                                  
    int HPGL_Pen_Speed;
    int HPGL_Pen_Diam;
    int HPGL_Pen_Recouvrement;
    bool HPGL_Org_Centre;        // TRUE si en HPGL, l'origine le centre de la feuille
    int PlotPSColorOpt;        // True for color Postscript output
    bool Plot_PS_Negative;     // True to create a  negative board ps plot

    /* Autorisation de trace des divers items en serigraphie */
    bool Sel_Texte_Reference;
    bool Sel_Texte_Valeur;
    bool Sel_Texte_Divers;
    bool Sel_Texte_Invisible;
    bool PlotPadsOnSilkLayer;
    bool Plot_Pads_All_Layers;   /* Plot pads meme n'appartenant pas a la
				    couche ( utile pour serigraphie) */

    /* id for plot format (see enum PlotFormat in plot_common.h) */
    int PlotFormat;
    int PlotOrient;
    int PlotScaleOpt;
    int DrillShapeOpt;
    double Scale_X;
    double Scale_Y;
};
extern PCB_Plot_Options g_pcb_plot_options;

#endif  // ifndef PCBPLOT_H

