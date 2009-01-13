        /************/
        /* pcbplot.h*/
        /************/

#ifndef PCBPLOT_H
#define PCBPLOT_H


#ifndef eda_global
#define eda_global extern
#endif

/* Shared Config keys for plot and print */
#define OPTKEY_PLOT_LINEWIDTH_VALUE    wxT( "PlotLineWidth" )
#define OPTKEY_LAYERBASE          wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_X_FINESCALE_ADJ wxT( "PrintXFineScaleAdj" )
#define OPTKEY_PRINT_Y_FINESCALE_ADJ wxT( "PrintYFineScaleAdj" )
#define OPTKEY_PRINT_SCALE           wxT( "PrintScale" )

/* Constantes de conversion d'unites */
/* coeff de conversion dim en 0.1 mil -> dim en unite PS: (unite PS = pouce) */
#define SCALE_PS .0001
/* coeff de conversion dim en 0,1 mil -> dim en unite HPGL: */
#define SCALE_HPGL 0.102041

/* Options : */
eda_global bool g_Exclude_Edges_Pcb		// True to exclude contents of Edges Pcb layer
#ifdef MAIN
= FALSE
#endif
;
eda_global bool g_Plot_Frame_Ref;       // True to plot/print frame references
eda_global bool g_DrawViaOnMaskLayer;	// True if vias are drawn on Mask layer (ie protected by mask)
eda_global int g_Plot_Mode				// = FILAIRE, FILL or SKETCH
#ifdef MAIN
= FILLED
#endif
;
eda_global bool Plot_Set_MIROIR ;
eda_global bool Sel_Rotate_Window;
eda_global bool HPGL_Org_Centre;		// TRUE si en HPGL, l'origine le centre de la feuille
eda_global int g_PlotPSColorOpt;		// True for color Postscript output
eda_global bool g_Plot_PS_Negative;		// True to create a  negative board ps plot


/* Autorisation de trace des divers items en serigraphie */
eda_global bool Sel_Texte_Reference
#ifdef MAIN
= TRUE
#endif
;
eda_global bool Sel_Texte_Valeur
#ifdef MAIN
= TRUE
#endif
;
eda_global bool Sel_Texte_Divers
#ifdef MAIN
= TRUE
#endif
;
eda_global bool Sel_Texte_Invisible;
eda_global bool PlotPadsOnSilkLayer	/* Plot pads sur couche serigraphie */
#ifdef MAIN
= TRUE
#endif
;
eda_global bool Plot_Pads_All_Layers;	/* Plot pads meme n'appartenant pas a la
                                        couche ( utile pour serigraphie) */

    /* Variables utiles */

eda_global FILE * dest;

/* Gestion des plumes en plot format HPGL */
eda_global int g_HPGL_Pen_Num				/* num de plume a charger */
#ifdef MAIN
= 1
#endif
;
eda_global int g_HPGL_Pen_Speed					/* vitesse en cm/s */
#ifdef MAIN
= 40
#endif
;
eda_global int g_HPGL_Pen_Diam;					/* diametre en mils */
eda_global int g_HPGL_Pen_Recouvrement;			/* recouvrement en mils ( pour remplissages */

/* Gestion des cadrages et echelles de trace */
eda_global float Scale_X, Scale_Y ;  /* coeff d'agrandissement en X et Y demandes */
eda_global wxPoint g_PlotOffset;	/* Offset de trace modifies par l'echelle */

eda_global int g_PlotLine_Width; /* Largeur du trait en mode filaire (utilise en serigraphie,
                                    pour traces en mode sketch et filaire) */

eda_global int g_PlotFormat  /* id for plot format (see enum PlotFormat in plot_common.h) */
#ifdef MAIN
= PLOT_FORMAT_GERBER
#endif
;
eda_global int g_PlotOrient;  /* numero de code de l'orientation du trace ( voir
                            defines precedents):
                            0 = normal
                            PLOT_MIROIR = MIROIR
                            */
eda_global int g_PlotScaleOpt		// 0 = automatique, >=1 echelle specifiee
#ifdef MAIN
= 1
#endif
;

eda_global int g_DrillShapeOpt
#ifdef MAIN
 = 1	// 0 = no drill mark, 1 = small mark, 2 = real drill
#endif
;


    /*************************************/
    /* Constantes utiles en trace GERBER */
    /*************************************/

/* codes de type de forme d'outils */
#define GERB_CIRCLE 1
#define GERB_RECT 2
#define GERB_LINE 3
#define GERB_OVALE 4
#define GERB_DONUT 5

/* PLOT_RTN.CC */
void PlotTextePcb( TEXTE_PCB * pt_texte,int format_plot,int masque_layer);
        /* Trace 1 Texte type PCB , c.a.d autre que les textes sur modules,
        prepare les parametres de trace de texte */
void PlotArc(int format_plot, wxPoint centre, int start_angle,int end_angle,
                    int rayon,int width);
void PlotCircle(int format_plot,int width, wxPoint centre, int rayon);
void PlotFilledPolygon(int format_plot, int nbpoints, int * coord);
void PlotPolygon(int format_plot, int nbpoints, int * coord, int width);

void PlotDrawSegment( DRAWSEGMENT* PtSegm, int format_plot,int masque_layer );

void PlotCotation( COTATION * Cotation, int format_plot,int masque_layer );

void PlotMirePcb( MIREPCB* PtMire, int format_plot,int masque_layer );

void Plot_1_EdgeModule(int format_plot, EDGE_MODULE * PtEdge);

void PlotFilledAreas( ZONE_CONTAINER * aZone, int aFormat);

/* PLOTGERB.CPP */
void SelectD_CODE_For_LineDraw( int aSize );
void trace_1_contour_GERBER(wxPoint pos, wxSize size, wxSize delta,
                                        int penwidth, int orient);
    /* Trace 1 contour rectangulaire ou trapezoidal d'orientation quelconque
         donne par son centre, ses dimensions,
         ses variations, l'epaisseur du trait et son orientation orient */

/* PLOTHPGL.CC */
void trace_1_segment_HPGL(int pos_X0,int pos_Y0,int pos_X1,int pos_Y1,
                            int hauteur);

void trace_1_pad_TRAPEZE_HPGL(wxPoint padpos, wxSize size,wxSize delta,
                        int orient,int modetrace);

void trace_1_pastille_RONDE_HPGL(wxPoint padpos, int diametre,int modetrace) ;
void trace_1_pastille_OVALE_HPGL(wxPoint padpos, wxSize size, int orient, int modetrace);
void PlotRectangularPad_HPGL(wxPoint padpos, wxSize padsize, int orient,int modetrace);

/**************/
/* PLOTPS.CPP */
/**************/
void trace_1_pastille_OVALE_POST(wxPoint centre, wxSize size, int orient, int modetrace);
void trace_1_pastille_RONDE_POST(wxPoint centre, int diametre, int modetrace);
void trace_1_pad_rectangulaire_POST(wxPoint centre, wxSize size,int orient,
                                        int modetrace);
void trace_1_contour_POST(wxPoint centre, wxSize size, wxSize delta,
                                        int dim_trait, int orient);
void trace_1_pad_TRAPEZE_POST(wxPoint centre, wxSize size, wxSize delta,
                            int orient,int modetrace);


#endif	/* #define PCBPLOT_H */

