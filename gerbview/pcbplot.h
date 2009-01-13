		/******************/
		/* file pcbplot.h */
		/******************/

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

/* Gestion des plumes en plot format HPGL */
eda_global int HPGL_Pen_Num,				/* num de plume a charger */
		HPGL_Pen_Speed,					/* vitesse en cm/s */
		HPGL_Pen_Diam,					/* diametre en mils */
		HPGL_Pen_Recouvrement;			/* recouvrement en mils ( pour remplissages */

eda_global float Scale_X, Scale_Y ;  /* coeff d'agrandissement en X et Y demandes */

eda_global int PlotMarge;

eda_global int g_PlotLine_Width; /* Largeur du trait en mode filaire (utilise en serigraphie,
                                    pour traces en mode sketch et filaire) */

#endif	// ifndef PCBPLOT_H

