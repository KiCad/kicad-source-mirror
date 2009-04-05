/************/
/* pcbplot.h*/
/************/

#ifndef PCBPLOT_H
#define PCBPLOT_H


/* Shared Config keys for plot and print */
#define OPTKEY_PLOT_LINEWIDTH_VALUE    wxT( "PlotLineWidth" )
#define OPTKEY_LAYERBASE               wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_X_FINESCALE_ADJ   wxT( "PrintXFineScaleAdj" )
#define OPTKEY_PRINT_Y_FINESCALE_ADJ   wxT( "PrintYFineScaleAdj" )
#define OPTKEY_PRINT_SCALE             wxT( "PrintScale" )

/* Constantes de conversion d'unites */
/* coeff de conversion dim en 0.1 mil -> dim en unite PS: (unite PS = pouce) */
#define SCALE_PS .0001
/* coeff de conversion dim en 0,1 mil -> dim en unite HPGL: */
#define SCALE_HPGL 0.102041

/* Options : */
extern bool g_Exclude_Edges_Pcb;
extern bool g_Plot_Frame_Ref;       // True to plot/print frame references
extern bool g_DrawViaOnMaskLayer;   // True if vias are drawn on Mask layer (ie protected by mask)
extern int g_Plot_Mode;
extern bool Plot_Set_MIROIR;
extern bool Sel_Rotate_Window;
extern bool HPGL_Org_Centre;        // TRUE si en HPGL, l'origine le centre de la feuille
extern int g_PlotPSColorOpt;        // True for color Postscript output
extern bool g_Plot_PS_Negative;     // True to create a  negative board ps plot


/* Autorisation de trace des divers items en serigraphie */
extern bool Sel_Texte_Reference;
extern bool Sel_Texte_Valeur;
extern bool Sel_Texte_Divers;
extern bool Sel_Texte_Invisible;
extern bool PlotPadsOnSilkLayer;
extern bool Plot_Pads_All_Layers;   /* Plot pads meme n'appartenant pas a la
                                        couche ( utile pour serigraphie) */

/* Variables utiles */

extern FILE * dest;

/* id for plot format (see enum PlotFormat in plot_common.h) */
extern int g_PlotScaleOpt;
extern int g_DrillShapeOpt;

/*************************************/
/* Constantes utiles en trace GERBER */
/*************************************/

/* codes de type de forme d'outils */
#define GERB_CIRCLE 1
#define GERB_RECT   2
#define GERB_LINE   3
#define GERB_OVALE  4
#define GERB_DONUT  5

/* PLOT_RTN.CC */
void    PlotTextePcb( TEXTE_PCB* pt_texte, int format_plot, int masque_layer );

/* Trace 1 Texte type PCB , c.a.d autre que les textes sur modules,
  * prepare les parametres de trace de texte */
void    PlotArc( int format_plot, wxPoint centre, int start_angle, int end_angle,
                 int rayon, int width );
void    PlotCircle( int format_plot, int width, wxPoint centre, int rayon );
void    PlotFilledPolygon( int format_plot, int nbpoints, int* coord );
void    PlotPolygon( int format_plot, int nbpoints, int* coord, int width );

void    PlotDrawSegment( DRAWSEGMENT* PtSegm, int format_plot, int masque_layer );

void    PlotCotation( COTATION* Cotation, int format_plot, int masque_layer );

void    PlotMirePcb( MIREPCB* PtMire, int format_plot, int masque_layer );

void    Plot_1_EdgeModule( int format_plot, EDGE_MODULE* PtEdge );

void    PlotFilledAreas( ZONE_CONTAINER* aZone, int aFormat );

/* PLOTGERB.CPP */
void    SelectD_CODE_For_LineDraw( int aSize );
void    trace_1_contour_GERBER( wxPoint pos, wxSize size, wxSize delta,
                                int penwidth, int orient );

/* Trace 1 contour rectangulaire ou trapezoidal d'orientation quelconque
  *  donne par son centre, ses dimensions,
  *  ses variations, l'epaisseur du trait et son orientation orient */

/* PLOTHPGL.CPP */

/** Function Plot a filled segment (track)
 * @param aStart = starting point
 * @param aEnd = ending point
 * @param aWidth = segment width (thickness)
 * @param aPlotMode = FILLED, SKETCH ..
 * @return true if Ok, false if aWidth > pen size (the segment is always plotted)
 */
bool    Plot_Filled_Segment_HPGL( wxPoint aStart, wxPoint aEnd, int aWidth, GRFillMode aPlotMode );

void    trace_1_pad_TRAPEZE_HPGL( wxPoint padpos, wxSize size, wxSize delta,
                                  int orient, int modetrace );

void    trace_1_pastille_RONDE_HPGL( wxPoint padpos, int diametre, int modetrace );
void    trace_1_pastille_OVALE_HPGL( wxPoint padpos, wxSize size, int orient, int modetrace );
void    PlotRectangularPad_HPGL( wxPoint padpos, wxSize padsize, int orient, int modetrace );

/**************/
/* PLOTPS.CPP */
/**************/
void    trace_1_pastille_OVALE_POST( wxPoint centre, wxSize size, int orient, int modetrace );
void    trace_1_pastille_RONDE_POST( wxPoint centre, int diametre, int modetrace );
void    trace_1_pad_rectangulaire_POST( wxPoint centre, wxSize size, int orient,
                                     int modetrace );
void    trace_1_contour_POST( wxPoint centre, wxSize size, wxSize delta,
                              int dim_trait, int orient );
void    trace_1_pad_TRAPEZE_POST( wxPoint centre, wxSize size, wxSize delta,
                                  int orient, int modetrace );


#endif  /* #define PCBPLOT_H */
