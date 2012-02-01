/**
 * @file pcbnew/pcbplot.h
 */

#ifndef PCBPLOT_H
#define PCBPLOT_H

#include "pcb_plot_params.h"


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
#define SCALE_PS .0001
// Convert dimension 0.1 mil -> HPGL units:
#define SCALE_HPGL 0.102041

// Small drill marks diameter value (in internal value = 1/10000 inch)
#define SMALL_DRILL 150


void PlotTextePcb( PLOTTER* plotter, TEXTE_PCB* pt_texte, int masque_layer,
                   EDA_DRAW_MODE_T trace_mode );

/* Plat PCB text type, ie other than text on modules
 * prepare the plot settings of text */
void PlotDrawSegment( PLOTTER* plotter, DRAWSEGMENT* PtSegm, int masque_layer,
                      EDA_DRAW_MODE_T trace_mode );

void PlotDimension( PLOTTER* plotter, DIMENSION* Dimension, int masque_layer,
                    EDA_DRAW_MODE_T trace_mode );

void PlotPcbTarget( PLOTTER* plotter, PCB_TARGET* PtMire, int masque_layer,
                    EDA_DRAW_MODE_T trace_mode );

void Plot_1_EdgeModule( PLOTTER* plotter, EDGE_MODULE* PtEdge,
                        EDA_DRAW_MODE_T trace_mode, int masque_layer );

void PlotFilledAreas( PLOTTER* plotter, ZONE_CONTAINER* aZone,
                      EDA_DRAW_MODE_T trace_mode );

// PLOTGERB.CPP
void SelectD_CODE_For_LineDraw( PLOTTER* plotter, int aSize );


#endif // #define PCBPLOT_H
