		/*******************************************************/
		/* Menu General de Trace (PLOT): Fichier inclus PLOT.H */
		/*******************************************************/

#ifndef PCBPLOT_H
#define PCBPLOT_H

#ifndef eda_global
#define eda_global extern
#endif

/* Gestion des plumes en plot format HPGL */
eda_global int HPGL_Pen_Num,				/* num de plume a charger */
		HPGL_Pen_Speed,					/* vitesse en cm/s */
		HPGL_Pen_Diam,					/* diametre en mils */
		HPGL_Pen_Recouvrement;			/* recouvrement en mils ( pour remplissages */

eda_global float Scale_X, Scale_Y ;  /* coeff d'agrandissement en X et Y demandes */

eda_global int W_Trait_Serigraphie; /* Largeur du trait utilise en serigraphie
							 pour trace de pads (traces en mode sketch) */
eda_global int format_plot;  /* numero de code du format de sortie */
eda_global int PlotMarge;

#endif	// ifndef PCBPLOT_H

