		/******************************************/
		/* Kicad: Common plot Postscript Routines */
		/******************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "plot_common.h"
#include "macros.h"

// Variables partagees avec Common plot Postscript Routines
extern wxPoint LastPenPosition;
extern wxPoint PlotOffset;
extern FILE * PlotOutputFile;
extern double XScale, YScale;
extern int PenWidth;
extern int PlotOrientOptions, etat_plume;

// Locales
static Ki_PageDescr * SheetPS;

/*************************************************************************************/
void InitPlotParametresPS(wxPoint offset, Ki_PageDescr * sheet,
		double xscale, double yscale, int orient)
/*************************************************************************************/
/* Set the plot offset for the current plotting
 xscale,yscale = coordinate scale (scale coefficient for coordinates)
 device_xscale,device_yscale = device coordinate scale (i.e scale used by plot device)
*/
{
	PlotOrientOptions = orient;
	PlotOffset = offset;
	SheetPS = sheet;
	XScale = xscale;
	YScale = yscale;
}

/*************************************************************************************/
void SetDefaultLineWidthPS( int width)
/*************************************************************************************/
/* Set the default line width (in 1/1000 inch) for the current plotting
*/
{
	PenWidth = width;			/* epaisseur du trait standard en 1/1000 pouce */
}

/******************************/
void SetColorMapPS(int color)
/******************************/
/* Print the postscript set color command:
 r g b setrgbcolor,
	r, g, b  = color values (= 0 .. 1.0 )

	color = color index in ColorRefs[]
*/
{
char Line[1024];

	sprintf( Line,"%.3f %.3f %.3f setrgbcolor\n",
		(float)ColorRefs[color].m_Red/255,
		(float)ColorRefs[color].m_Green/255,
		(float)ColorRefs[color].m_Blue/255
		);
	to_point(Line);
	fputs( Line, PlotOutputFile);
}

/***************************************************************/
void PlotFilledSegmentPS(wxPoint start , wxPoint end, int width)
/***************************************************************/
/* Plot 1 segment like a track segment
 */
{
	UserToDeviceCoordinate(start);
	UserToDeviceCoordinate(end);

	fprintf(PlotOutputFile,"%d setlinewidth\n", (int)(XScale * width));
	fprintf(PlotOutputFile,"%d %d %d %d line\n", start.x, start.y, end.x, end.y);
}


/******************************************************/
void PlotCircle_PS(wxPoint pos, int diametre, int width)
/******************************************************/
{
int rayon;
char Line[256];

	UserToDeviceCoordinate(pos);
	rayon = (int)(XScale * diametre / 2);

	if(rayon < 0 ) rayon = 0 ;

	if ( width > 0 )
	{
		sprintf(Line,"%d setlinewidth\n", (int)( width * XScale) ) ;
		fputs(Line,PlotOutputFile);
	}

	sprintf(Line,"newpath %d %d %d 0 360 arc stroke\n", pos.x, pos.y, rayon);
	fputs(Line,PlotOutputFile) ;
}



/********************************************************************/
void PlotArcPS(wxPoint centre, int StAngle, int EndAngle, int rayon)
/********************************************************************/
/* Plot an arc:
	StAngle, EndAngle = start and end arc in 0.1 degree
*/
{
char Line[256];

	if(rayon <= 0 ) return ;

	/* Calcul des coord du point de depart : */
	UserToDeviceCoordinate(centre);

	if( PlotOrientOptions == PLOT_MIROIR)
		sprintf(Line, "newpath %d %d %d %f %f arc stroke\n", centre.x, centre.y,
					(int)(rayon * XScale), (float)StAngle/10, (float)EndAngle/10 ) ;
	else
		sprintf(Line, "newpath %d %d %d %f %f arc stroke\n", centre.x, centre.y,
					(int)(rayon * XScale), -(float)EndAngle/10, -(float)StAngle/10 ) ;
	// Undo internationalization printf (float x.y printed x,y)
	to_point(Line);
	fputs(Line,PlotOutputFile) ;

}

/*****************************************************************************/
void PlotArcPS(wxPoint centre, int StAngle, int EndAngle, int rayon, int width)
/*****************************************************************************/
/* trace d'un arc de cercle:
	x, y = coord du centre
	StAngle, EndAngle = angle de debut et fin
	rayon = rayon de l'arc
	w = epaisseur de l'arc
*/
{
char Line[256];

	if(rayon <= 0 ) return ;

	sprintf(Line,"%d setlinewidth\n", (int) (width * XScale) );
	fputs(Line, PlotOutputFile);
	PlotArcPS( centre, StAngle, EndAngle, rayon);
}


/***************************************************/
void PlotPolyPS( int nb_segm, int * coord, int fill)
/***************************************************/
/* Trace un polygone ( ferme si rempli ) en format POSTSCRIPT
	coord = tableau des coord des sommets
	nb_segm = nombre de coord ( 1 coord = 2 elements: X et Y du tableau )
	fill : si != 0 polygone rempli
*/
{
int ii;
wxPoint pos;

	if( nb_segm <= 1 ) return;

	pos.x = coord[0]; pos.y = coord[1];
	UserToDeviceCoordinate(pos);
	fprintf(PlotOutputFile, "newpath %d %d moveto\n", pos.x, pos.y);

	for( ii = 1; ii < nb_segm; ii ++ )
		{
		pos.x = coord[ii*2]; pos.y = coord[(ii*2)+1];
		UserToDeviceCoordinate(pos);
		fprintf(PlotOutputFile, "%d %d lineto\n", pos.x, pos.y);
		}

	/* Fermeture du polygone */
	if( fill ) fprintf(PlotOutputFile, "closepath ");
	if( fill == 1 ) fprintf(PlotOutputFile, "fill ");
	fprintf(PlotOutputFile, "stroke\n");
}


/*************************************/
/* Routine to draw to a new position */
/*************************************/
void LineTo_PS(wxPoint pos, int plume)
{
	if ( plume == 'Z') return;

	UserToDeviceCoordinate(pos);
	if ( plume == 'D')
	{
	char Line[256];
		sprintf(Line,"%d %d %d %d line\n",
			LastPenPosition.x, LastPenPosition.y, pos.x, pos.y);
		fputs(Line,PlotOutputFile);
	}
	LastPenPosition = pos;
}


/**********************************************************/
void PrintHeaderPS(FILE * file, const wxString & Creator,
			const wxString & FileName, int BBox[4])
/***********************************************************/
/* BBox is the boundary box (position and size of the "client rectangle"
for drawings (page - margins) in mils (0.001 inch)
*/
{
wxString msg;
char Line[1024];
const char *PSMacro[] = {
"/line {\n",
"    newpath\n",
"    moveto\n",
"    lineto\n",
"    stroke\n",
"} def\n",
"gsave\n",
"72 72 scale\t\t\t%% Talk inches\n",
"1 setlinecap\n",
"1 setlinejoin\n",
"1 setlinewidth\n",
	NULL
};
#define MIL_TO_INCH 0.001
int ii;
time_t time1970 = time(NULL);

	PlotOutputFile = file;

	fputs("%!PS-Adobe-3.0\n",PlotOutputFile);	// Print header

	/* Print boundary box en 1/72 pouce, box is in mils */
	#define CONV_SCALE (MIL_TO_INCH * 72)
	sprintf( Line, "%%%%BoundingBox: %d %d %d %d\n",
		(int)(BBox[1]*CONV_SCALE), (int)(BBox[0]*CONV_SCALE),
		(int)(BBox[3]*CONV_SCALE), (int)(BBox[2]*CONV_SCALE));
	fputs(Line, PlotOutputFile);

	sprintf( Line, "%%%%Title: %s\n", CONV_TO_UTF8(FileName) );
	fputs(Line, PlotOutputFile);

	sprintf( Line, "%%%%Creator: %s_n", CONV_TO_UTF8(Creator) );
	fputs(Line, PlotOutputFile);

	sprintf( Line, "%%%%CreationDate: %s\n", ctime(&time1970) );
	fputs(Line, PlotOutputFile);

	sprintf( Line, "%%%%DocumentPaperSizes: %s\n", CONV_TO_UTF8(SheetPS->m_Name) );
	fputs(Line, PlotOutputFile);

	sprintf( Line, "%%%%Orientation: Landscape\n%%%%EndComments\n");
	fputs(Line, PlotOutputFile);

	for (ii = 0; PSMacro[ii] != NULL; ii++)
	{
		fputs(PSMacro[ii],PlotOutputFile);
	}

	sprintf(Line, "%f %f translate 90 rotate\n",
				(float) BBox[3] * MIL_TO_INCH, (float)BBox[0] * MIL_TO_INCH );
	// compensation internationalisation printf (float x.y généré x,y)
	to_point(Line);

	fputs(Line,PlotOutputFile);

	sprintf(Line,"%f %f scale\t\t%% Move to User coordinates\n",
			XScale, YScale);
	to_point(Line);
	fputs(Line,PlotOutputFile);

	// Set default line width:
	fprintf(PlotOutputFile,"%d setlinewidth\n", PenWidth ); //PenWidth in user units
}


/******************************************/
bool CloseFilePS(FILE * plot_file)
/******************************************/
{
	fputs("showpage\n",plot_file);
	fputs("grestore\n",plot_file);

	fclose(plot_file);

	return TRUE;
}

