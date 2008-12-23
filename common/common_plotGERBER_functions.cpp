/******************************************/
/* Kicad: Common plot GERBER Routines */
/******************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "plot_common.h"
#include "macros.h"


/***************************************************************************/
void InitPlotParametresGERBER( wxPoint aOffset, double aXScale, double aYScale )
/***************************************************************************/

/** function InitPlotParametresGERBER
  * Set the plot offset for the current plotting
  * @param aOffset = plot offset
  * @param aXScale,aYScale = coordinate scale (scale coefficient for coordinates)
 */
{
    g_Plot_PlotOrientOptions = 0;
    g_Plot_PlotOffset = aOffset;
    g_Plot_XScale = aXScale;
    g_Plot_YScale = aYScale;
    g_Plot_DefaultPenWidth = 120;            /* epaisseur du trait standard en 1/1000 pouce */
    g_Plot_CurrentPenWidth = -1;
}


/******************************************************************/
void Write_Header_GERBER( const wxString aTitle, FILE* aFile )
/******************************************************************/
/** Function Write_Header_GERBER
 * Write GERBER header to file
 * initialize global variable g_Plot_PlotOutputFile
 * @param aTitle: the name of creator (comment)
 * @param aFile: an opened file to write to
 */
{
    char Line[1024];
	g_Plot_PlotOutputFile =  aFile;

    DateAndTime( Line );
    wxString Title = aTitle + wxT( " " ) + GetBuildVersion();
    fprintf( g_Plot_PlotOutputFile, "G04 (created by %s) date %s*\n", CONV_TO_UTF8( Title ), Line );

    // Specify linear interpol (G01), unit = INCH (G70), abs format (G90):
    fputs( "G01*\nG70*\nG90*\n", g_Plot_PlotOutputFile );
    fputs( "%MOIN*%\n", g_Plot_PlotOutputFile );     // set unites = INCHES

    /* Set gerber format to 3.4 */
    fputs( "G04 Gerber Fmt 3.4, Leading zero omitted, Abs format*\n%FSLAX34Y34*%\n",
		g_Plot_PlotOutputFile);

    fputs( "G04 APERTURE LIST*\n", g_Plot_PlotOutputFile );
}


/**********************************************/
void LineTo_GERBER( wxPoint aPos, int aCommand )
/**********************************************/
/** Function LineTo_GERBER
  * if aCommand = 'U' initialise the starting point of a line
  * if aCommand = 'D' draw a line from the starting point, or last point to aPos
  * @param aPos = end of the current line.
  * @param aCommand = 'U' or 'D' or 'Z' (Pen up , no moving )
 */
{
	static wxPoint LastPoint;
    switch ( aCommand )
	{
		case 'Z':
			return;

		case 'U':
			break;

		case 'D':
            PlotGERBERLine( LastPoint, aPos, g_Plot_CurrentPenWidth );
	}
	LastPoint = aPos;
}

/** Function PlotGERBERLine
 * Plot a line
 * the D_CODE **MUST** have already selected (this is just the line plot)
 * @param aStartPos = starting point of the line
 * @param aEndPos   = ending point of the line
 * @param aThickness = line thickness (not used here)
*/
void PlotGERBERLine( wxPoint aStartPos, wxPoint aEndPos, int aThickness )
{
    UserToDeviceCoordinate( aStartPos );
    UserToDeviceCoordinate( aEndPos );
    fprintf( g_Plot_PlotOutputFile, "X%5.5dY%5.5dD02*\n", aStartPos.x, aStartPos.y );
    fprintf( g_Plot_PlotOutputFile, "X%5.5dY%5.5dD01*\n", aEndPos.x, aEndPos.y );
}


/********************************************************************/
void PlotCircle_GERBER( wxPoint aCentre, int aRadius, int aWidth )
/********************************************************************/
/** Function PlotCircle_GERBER
 * writes a non filled circle to output file
 * Plot one circle as segments (6 to 16 depending on its radius
 * @param aCentre = centre coordintes
 * @param aRadius = radius of the circle
 * @param aWidth = line width (noc currently used, D_CODEs must be selected before)
*/
{
    int ii;
    wxPoint start, end;
    int delta;      	/* increment (in 0.1 degrees) to draw circles */

    delta = 3600/32;    /* there are delta segments for draw a circle */

    start.x = aCentre.x + aRadius;
    start.y = aCentre.y;
    for( ii = delta; ii < 3600; ii += delta )
    {
        end.x = aCentre.x + (int) (aRadius * fcosinus[ii]);
        end.y = aCentre.y + (int) (aRadius * fsinus[ii]);
        PlotGERBERLine( start, end, aWidth );
        start = end;
    }

    end.x = aCentre.x + aRadius;
    end.y = aCentre.y;
	PlotGERBERLine( start, end, aWidth );
}


/***************************************************************/
void PlotFilledPolygon_GERBER( int aCornersCount, int* aCoord )
/***************************************************************/
/** Function PlotFilledPolygon_GERBER
 * writes a filled polyline to output file
 * @param aCornersCount = numer of corners
 * @param aCoord = buffer of corners coordinates
*/
{
    int     ii;
    wxPoint pos, startpos;

    fputs( "G36*\n", g_Plot_PlotOutputFile );
    pos.x = *aCoord;
    aCoord++;
    pos.y = *aCoord;
    aCoord++;
    UserToDeviceCoordinate( pos );
    startpos = pos;
    fprintf( g_Plot_PlotOutputFile, "X%5.5dY%5.5dD02*\n", pos.x, pos.y );
    for( ii = 1; ii < aCornersCount; ii++ )
    {
        pos.x = *aCoord;
        aCoord++;
        pos.y = *aCoord;
        aCoord++;
        UserToDeviceCoordinate( pos );
        fprintf( g_Plot_PlotOutputFile, "X%5.5dY%5.5dD01*\n", pos.x, pos.y );
    }

    fprintf( g_Plot_PlotOutputFile, "X%5.5dY%5.5dD01*\n", startpos.x, startpos.y );
    fputs( "G37*\n", g_Plot_PlotOutputFile );
}


/*******************************************************************/
void PlotPolygon_GERBER( int aCornersCount, int* aCoord, int aWidth )
/*******************************************************************/
/** Function PlotPolygon_GERBER
 * writes a closed polyline (not a filled polygon) to output file
 * @param aCornersCount = numer of corners
 * @param aCoord = buffer of corners coordinates
 * @param aWidth = line width (not currently used, D_CODEs must be selected before)
*/
{
    wxPoint start, end, startpoint;
    startpoint.x = *aCoord++;
    startpoint.y = *aCoord++;
    start = startpoint;
    for( int ii = 0; ii < aCornersCount-1; ii++ )
    {
        end.x = *aCoord;
        aCoord++;
        end.y = *aCoord;
        aCoord++;
        PlotGERBERLine(start, end, aWidth );
        start = end;
    }

    if ( startpoint != end )    // Close poly
        PlotGERBERLine(end, startpoint, aWidth );

}


