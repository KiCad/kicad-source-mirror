/******************************************/
/* Kicad: Common plot Postscript Routines */
/******************************************/

#include "fctsys.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "plot_common.h"
#include "macros.h"
#include "kicad_string.h"

// Locales
static Ki_PageDescr* SheetPS;


/*************************************************************************************/
void InitPlotParametresPS( wxPoint offset, Ki_PageDescr* sheet,
                           double aXScale, double aYScale, int orient )
/*************************************************************************************/

/* Set the plot offset for the current plotting
 * g_Plot_XScale,g_Plot_YScale = coordinate scale (scale coefficient for coordinates)
 * device_g_Plot_XScale,device_g_Plot_YScale = device coordinate scale (i.e scale used by plot device)
 */
{
    g_Plot_PlotOrientOptions = orient;
    g_Plot_PlotOffset = offset;
    SheetPS    = sheet;
    g_Plot_XScale = aXScale;
    g_Plot_YScale = aYScale;
    g_Plot_CurrentPenWidth = -1;
}


/*************************************************************************************/
void SetDefaultLineWidthPS( int width )
/*************************************************************************************/

/* Set the default line width (in 1/1000 inch) for the current plotting
 */
{
    g_Plot_DefaultPenWidth = width;      // epaisseur du trait standard en 1/1000 pouce
    g_Plot_CurrentPenWidth = -1;
}


/***************************************/
void SetCurrentLineWidthPS( int width )
/***************************************/

/* Set the Current line width (in 1/1000 inch) for the next plot
 */
{
    int pen_width;

    if( width > 0 )
        pen_width = width;
    else
        pen_width = g_Plot_DefaultPenWidth;

    if( pen_width != g_Plot_CurrentPenWidth )
        fprintf( g_Plot_PlotOutputFile, "%d setlinewidth\n", (int) (g_Plot_XScale * pen_width) );

    g_Plot_CurrentPenWidth = pen_width;
}


/******************************/
void SetColorMapPS( int color )
/******************************/

/* Print the postscript set color command:
 * r g b setrgbcolor,
 * r, g, b  = color values (= 0 .. 1.0 )
 *
 * color = color index in ColorRefs[]
 */
{
    char Line[1024];

    sprintf( Line, "%.3f %.3f %.3f setrgbcolor\n",
        (float) ColorRefs[color].m_Red / 255,
        (float) ColorRefs[color].m_Green / 255,
        (float) ColorRefs[color].m_Blue / 255 );
    to_point( Line );
    fputs( Line, g_Plot_PlotOutputFile );
}


/***************************************************************/
void PlotFilledSegmentPS( wxPoint start, wxPoint end, int width )
/***************************************************************/

/* Plot 1 segment like a track segment
 */
{
    UserToDeviceCoordinate( start );
    UserToDeviceCoordinate( end );

    SetCurrentLineWidthPS( width );
    fprintf( g_Plot_PlotOutputFile, "%d %d %d %d line\n", start.x, start.y, end.x, end.y );
}

/***************************************************************/
void PlotRectPS( wxPoint p1, wxPoint p2, bool fill, int width )
/***************************************************************/
{
    UserToDeviceCoordinate( p1 );
    UserToDeviceCoordinate( p2 );

    SetCurrentLineWidthPS( width );
    fprintf( g_Plot_PlotOutputFile, "%d %d %d %d rect%d\n", p1.x, p1.y,
    p2.x-p1.x, p2.y-p1.y, fill );
}

/******************************************************/
void PlotCirclePS( wxPoint pos, int diametre, bool fill, int width )
/******************************************************/
{
    int  rayon;
    char Line[256];

    UserToDeviceCoordinate( pos );
    rayon = (int) (g_Plot_XScale * diametre / 2);

    if( rayon < 0 )
        rayon = 0;

    SetCurrentLineWidthPS( width );
    sprintf(Line, "%d %d %d cir%d\n", pos.x, pos.y, rayon, fill);
    fputs( Line, g_Plot_PlotOutputFile );
}


/**************************************************************************************/
void PlotArcPS( wxPoint centre, int StAngle, int EndAngle, int rayon, bool fill, int width )
/**************************************************************************************/

/* Plot an arc:
 * StAngle, EndAngle = start and end arc in 0.1 degree
 */
{
    char Line[256];

    if( rayon <= 0 )
        return;

    SetCurrentLineWidthPS( width );

    // Calcul des coord du point de depart :
    UserToDeviceCoordinate( centre );

    if( g_Plot_PlotOrientOptions == PLOT_MIROIR )
        sprintf( Line, "%d %d %d %f %f arc%d\n", centre.x, centre.y,
            (int) (rayon * g_Plot_XScale), (float) StAngle / 10, (float) EndAngle / 10,
            fill);
    else
        sprintf( Line, "%d %d %d %f %f arc%d\n", centre.x, centre.y,
            (int) (rayon * g_Plot_XScale), -(float) EndAngle / 10, -(float) StAngle / 10,
            fill);

    // Undo internationalization printf (float x.y printed x,y)
    to_point( Line );
    fputs( Line, g_Plot_PlotOutputFile );
}


/*****************************************************************/
void PlotPolyPS( int nb_segm, int* coord, bool fill, int width )
/*****************************************************************/

/* Draw a polygon ( a filled polygon if fill == 1 ) in POSTSCRIPT format
 * @param nb_segm = corner count
 * @param coord = corner list (a corner uses 2 int = X coordinate followed by Y  coordinate
 * @param fill :if true : filled polygon
 * @param  width = line width
 */
{
    int     ii;
    wxPoint pos;

    if( nb_segm <= 1 )
        return;

    SetCurrentLineWidthPS( width );

    pos.x = coord[0];
    pos.y = coord[1];
    UserToDeviceCoordinate( pos );
    fprintf( g_Plot_PlotOutputFile, "newpath %d %d moveto\n", pos.x, pos.y );

    for( ii = 1; ii < nb_segm; ii++ )
    {
        pos.x = coord[2 * ii];
        pos.y = coord[2 * ii + 1];
        UserToDeviceCoordinate( pos );
        fprintf( g_Plot_PlotOutputFile, "%d %d lineto\n", pos.x, pos.y );
    }

    // Fermeture du polygone
    fprintf(g_Plot_PlotOutputFile, "poly%d\n", fill);
}


/*************************************/
void LineTo_PS( wxPoint pos, int plume )
/*************************************/

/* Routine to draw to a new position
 */
{
    if( plume == 'Z' )
        return;

    UserToDeviceCoordinate( pos );
    if( plume == 'D' )
    {
        char Line[256];
        sprintf( Line, "%d %d %d %d line\n",
            g_Plot_LastPenPosition.x, g_Plot_LastPenPosition.y, pos.x, pos.y );
        fputs( Line, g_Plot_PlotOutputFile );
    }
    g_Plot_LastPenPosition = pos;
}


/***********************************************************/
void PrintHeaderPS( FILE* file, const wxString& Creator,
                    const wxString& FileName, int PageCount,
                    int BBox[4], int PaperOrientation )
/***********************************************************/

/* The code within this function (and the CloseFilePS function)
 * creates postscript files whose contents comply with Adobe's
 * Document Structuring Convention, as documented by assorted
 * details described within the following URLs:
 *
 * http://en.wikipedia.org/wiki/Document_Structuring_Conventions
 * http://partners.adobe.com/public/developer/en/ps/5001.DSC_Spec.pdf
 *
 *
 * The PageCount and PaperOrientation parameters have been provided to
 * respectively cater for the production of multiple-page postscript
 * files, and postscript files having either a portrait orientation
 * or a landscape orientation.
 *
 * BBox is the boundary box (position and size of the "client rectangle"
 * for drawings (page - margins) in mils (0.001 inch)
 */
{
    wxString     msg;
    char         Line[1024];

    static const char*  PSMacro[] = {
        "/line {\n",
        "    newpath\n",
        "    moveto\n",
        "    lineto\n",
        "    stroke\n",
        "} bind def\n",
        "/cir0 { newpath 0 360 arc stroke } bind def\n",
        "/cir1 { newpath 0 360 arc gsave fill grestore stroke } bind def\n",
        "/cir2 { newpath 0 360 arc gsave fill grestore stroke } bind def\n",
        "/arc0 { newpath arc stroke } bind def\n",
        "/arc1 { newpath 4 index 4 index moveto arc closepath gsave fill grestore stroke } bind def\n",
        "/arc2 { newpath 4 index 4 index moveto arc closepath gsave fill grestore stroke } bind def\n",
        "/poly0 { stroke } bind def\n",
        "/poly1 { closepath gsave fill grestore stroke } bind def\n",
        "/poly2 { closepath gsave fill grestore stroke } bind def\n",
        "/rect0 { rectstroke } bind def\n",
        "/rect1 { rectfill } bind def\n",
        "/rect2 { rectfill } bind def\n",
        "gsave\n",
        "72 72 scale\t\t\t% Talk inches\n",
        "1 setlinecap\n",
        "1 setlinejoin\n",
        "1 setlinewidth\n",
        NULL
    };

    const double MIL_TO_INCH = 0.001;
    int          ii;
    time_t       time1970 = time( NULL );

    g_Plot_PlotOutputFile = file;

    fputs( "%!PS-Adobe-3.0\n", g_Plot_PlotOutputFile );    // Print header

    sprintf( Line, "%%%%Creator: %s\n", CONV_TO_UTF8( Creator ) );
    fputs( Line, g_Plot_PlotOutputFile );

    // A "newline" character ("\n") is not included in the following string,
    // because it is provided by the ctime() function.
    sprintf( Line, "%%%%CreationDate: %s", ctime( &time1970 ) );
    fputs( Line, g_Plot_PlotOutputFile );

    sprintf( Line, "%%%%Title: %s\n", CONV_TO_UTF8( FileName ) );
    fputs( Line, g_Plot_PlotOutputFile );

    sprintf( Line, "%%%%Pages: %d\n", PageCount );
    fputs( Line, g_Plot_PlotOutputFile );

    sprintf( Line, "%%%%PageOrder: Ascend\n" );
    fputs( Line, g_Plot_PlotOutputFile );

    // Print boundary box en 1/72 pouce, box is in mils
    const double CONV_SCALE = MIL_TO_INCH * 72;

    // The coordinates of the lower left corner of the boundary
    // box need to be "rounded down", but the coordinates of its
    // upper right corner need to be "rounded up" instead.
    sprintf( Line, "%%%%BoundingBox: %d %d %d %d\n",
        (int) floor( (BBox[1] * CONV_SCALE) ), (int) floor( (BBox[0] * CONV_SCALE) ),
        (int) ceil( (BBox[3] * CONV_SCALE) ), (int) ceil( (BBox[2] * CONV_SCALE) ) );

    fputs( Line, g_Plot_PlotOutputFile );

    // Specify the size of the sheet and the name associated with that size.
    // (If the "User size" option has been selected for the sheet size,
    // identify the sheet size as "Custom" (rather than as "User"), but
    // otherwise use the name assigned by KiCad for each sheet size.)
    //
    // (The Document Structuring Convention also supports sheet weight,
    // sheet colour, and sheet type properties being specified within a
    // %%DocumentMedia comment, but they are not being specified here;
    // a zero and two null strings are subsequently provided instead.)
    //
    // (NOTE: m_Size.y is *supposed* to be listed before m_Size.x;
    // the order in which they are specified is not wrong!)
    if( SheetPS->m_Name.Cmp( wxT( "User" ) ) == 0 )
        sprintf( Line, "%%%%DocumentMedia: Custom %d %d 0 () ()\n",
                 wxRound( SheetPS->m_Size.y * CONV_SCALE ),
                 wxRound( SheetPS->m_Size.x * CONV_SCALE ) );

    else  // ( if SheetPS->m_Name does not equal "User" )
        sprintf( Line, "%%%%DocumentMedia: %s %d %d 0 () ()\n",
                 CONV_TO_UTF8( SheetPS->m_Name ),
                 wxRound( SheetPS->m_Size.y * CONV_SCALE ),
                 wxRound( SheetPS->m_Size.x * CONV_SCALE ) );
    fputs( Line, g_Plot_PlotOutputFile );

    if( PaperOrientation == wxPORTRAIT )
        sprintf( Line, "%%%%Orientation: Portrait\n" );
    else
        sprintf( Line, "%%%%Orientation: Landscape\n" );

    fputs( Line, g_Plot_PlotOutputFile );

    sprintf( Line, "%%%%EndComments\n" );
    fputs( Line, g_Plot_PlotOutputFile );

    // Now specify various other details.

    // The following string has been specified here (rather than within
    // PSMacro[]) to highlight that it has been provided to ensure that the
    // contents of the postscript file comply with the details specified
    // within the Document Structuring Convention.
    sprintf( Line, "%%%%Page: 1 1\n" );
    fputs( Line, g_Plot_PlotOutputFile );

    for( ii = 0; PSMacro[ii] != NULL; ii++ )
    {
        fputs( PSMacro[ii], g_Plot_PlotOutputFile );
    }

    if( PaperOrientation == wxLANDSCAPE )
        sprintf( Line, "%f %f translate 90 rotate\n",
            (float) BBox[3] * MIL_TO_INCH, (float) BBox[0] * MIL_TO_INCH );

    // (If support for creating postscript files with a portrait orientation
    // is ever provided, determine whether it would be necessary to provide
    // an "else" command and then an appropriate "sprintf" command here.)

    // compensation internationalisation printf (float x.y généré x,y)
    to_point( Line );

    fputs( Line, g_Plot_PlotOutputFile );

    sprintf( Line, "%f %f scale\t\t%% Move to User coordinates\n",
        g_Plot_XScale, g_Plot_YScale );
    to_point( Line );
    fputs( Line, g_Plot_PlotOutputFile );

    // Set default line width ( g_Plot_DefaultPenWidth is in user units )
    fprintf( g_Plot_PlotOutputFile, "%d setlinewidth\n", g_Plot_DefaultPenWidth );
}


/******************************************/
bool CloseFilePS( FILE* plot_file )
/******************************************/
{
    fputs( "showpage\n", plot_file );
    fputs( "grestore\n", plot_file );
    fputs( "%%EOF\n", plot_file );

    fclose( plot_file );

    return TRUE;
}
