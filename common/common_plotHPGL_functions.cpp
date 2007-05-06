		/******************************************/
		/* Kicad: Common plot HPGL Routines */
		/******************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "plot_common.h"
#include "macros.h"

/* parametre HPGL pour trace de cercle: */
#define CHORD_ANGLE 10

// Variables partagees avec Common plot Postscript Routines
extern wxPoint LastPenPosition;
extern wxPoint PlotOffset;
extern FILE * PlotOutputFile;
extern double XScale, YScale;
extern int PenWidth;
extern int PlotOrientOptions, etat_plume;

//Variables locales
void Move_Plume_HPGL( wxPoint pos, int plume );
void Plume_HPGL( int plume );


/***********************************************************************************/
void InitPlotParametresHPGL(wxPoint offset, double xscale, double yscale, int orient)
/***********************************************************************************/
/* Set the plot offset for the current plotting
 xscale,yscale = coordinate scale (scale coefficient for coordinates)
 device_xscale,device_yscale = device coordinate scale (i.e scale used by plot device)
*/
{
	PlotOffset = offset;
	XScale = xscale;
	YScale = yscale;
	PenWidth = 6;			/* epaisseur du trait standard en 1/1000 pouce */
	PlotOrientOptions = orient;
}



/*****************************************************************/
bool PrintHeaderHPGL(FILE * plot_file, int pen_speed, int pen_num)
/*****************************************************************/
{
char Line[256];

	PlotOutputFile = plot_file;
	etat_plume = 'U';
	sprintf(Line,"IN;VS%d;PU;PA;SP%d;\n",pen_speed,pen_num);
	fputs(Line,plot_file);
	return TRUE;
}

/**********************************/
bool CloseFileHPGL(FILE * plot_file)
/**********************************/
{
	fputs("PU;PA;SP0;\n",PlotOutputFile) ; fclose(PlotOutputFile) ;
	fclose(plot_file);
	return TRUE;
}


/************************************************************/
void PlotCircle_HPGL(wxPoint centre, int diameter, int width)
/************************************************************/
{
int rayon;
char Line[256];

	UserToDeviceCoordinate(centre);
	rayon = (int)(diameter / 2 * XScale);

	if(rayon < 0 ) rayon = 0 ;

	Plume_HPGL('U');
	sprintf(Line,"PA %d,%d;CI %d,%d;\n", centre.x, centre.y, rayon , CHORD_ANGLE);
	fputs(Line,PlotOutputFile) ;

	Plume_HPGL('U'); return ;
}


/********************************************************************/
void PlotArcHPGL(wxPoint centre, int StAngle, int EndAngle, int rayon)
/********************************************************************/
/* trace d'un arc de cercle:
	centre = coord du centre
	StAngle, EndAngle = angle de debut et fin
	rayon = rayon de l'arc
	commande
		PU;PA x,y;PD;AA start_arc_X, start_arc_Y, angle, NbSegm; PU;
	ou	PU;PA x,y;PD;AA start_arc_X, start_arc_Y, angle; PU;
*/
{
char Line[256];
wxPoint cmap;			/* point de depart */
wxPoint cpos;				/* centre */
float angle;			/* angle de l'arc*/

	if(rayon <= 0 ) return ;

	cpos = centre; UserToDeviceCoordinate(cpos);

	if( PlotOrientOptions == PLOT_MIROIR)
	{
		EndAngle = - EndAngle;
		StAngle = - StAngle;
		EXCHG (StAngle, EndAngle);
	}
	angle = (EndAngle - StAngle) /10.0;
	/* Calcul des coord du point de depart : */
	cmap.x = (int)( centre.x + ( rayon * cos(StAngle * M_PI / 1800 ) ) );
	cmap.y = (int)(centre.y + ( rayon * sin(StAngle * M_PI / 1800 ) ) );
	UserToDeviceCoordinate(cmap);

	Plume_HPGL('U');
	sprintf(Line,"PU;PA %d,%d;PD;AA %d,%d, ", cmap.x, cmap.y, cpos.x, cpos.y);
	fputs(Line,PlotOutputFile) ;
	sprintf(Line,"%f", - angle); to_point(Line); // Transforme , et . du separateur
	fputs(Line,PlotOutputFile) ;
	sprintf(Line,", %d", CHORD_ANGLE); fputs(Line,PlotOutputFile) ;
	sprintf(Line,";PU;\n"); fputs(Line,PlotOutputFile) ;
	Plume_HPGL('U');
}


/*****************************************************/
void PlotPolyHPGL( int nb, int * coord, int fill)
/*****************************************************/
/* Trace un polygone (ferme si rempli) en format HPGL
	coord = tableau des coord des sommets
	nb = nombre de coord ( 1 coord = 2 elements: X et Y du tableau )
	fill : si != 0 polygone rempli
*/
{
int ii;
	if( nb <= 1 ) return;

	Move_Plume_HPGL( wxPoint(coord[0],coord[1]), 'U');
	for( ii = 1; ii < nb  ; ii ++ )
		{
		Move_Plume_HPGL( wxPoint(coord[ii * 2],coord[(ii*2) +1]), 'D');
		}

	/* Fermeture eventuelle du polygone */
	if ( fill )
		{
		ii = (nb - 1) * 2;
		if( (coord[ii] != coord[0] ) || (coord[ii+1] != coord[0]) )
				 Move_Plume_HPGL( wxPoint(coord[0],coord[1]), 'D');
		}
	Plume_HPGL('U');
}


/**********************************************/
void Move_Plume_HPGL( wxPoint pos, int plume )
/**********************************************/
/*
	deplace la plume levee (plume = 'U') ou baissee (plume = 'D')
	en position x,y
	Unites en Unites DESSIN
	Si plume = 'Z' lever de plume sans deplacement
*/
{
char Line[256];

	if ( plume == 'Z')
	{
		Plume_HPGL('U');
		return;
	}
	Plume_HPGL(plume);
	UserToDeviceCoordinate(pos);

	sprintf(Line,"PA %d,%d;\n",pos.x,pos.y) ; fputs(Line,PlotOutputFile) ;
}


/***************************/
void Plume_HPGL( int plume )
/***************************/
/* leve (plume = 'U') ou baisse (plume = 'D') la plume
*/
{
	if ( plume == 'U')
		{
		if(etat_plume != 'U' ) fputs("PU;",PlotOutputFile) ;
		etat_plume = 'U';
		}
	else
		{
		if(etat_plume != 'D' )fputs("PD;",PlotOutputFile) ;
		etat_plume = 'D';
		}
}



