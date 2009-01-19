/**
 * Common plot library \n
 * Plot settings, postscript plotting, gerber plotting.
 * 
 * @file plot_common.h
 */

#ifndef __INCLUDE__PLOT_COMMON_H__
#define __INCLUDE__PLOT_COMMON_H__ 1

#ifndef EDA_BASE
#  define COMMON_GLOBL extern
#else
#  define COMMON_GLOBL
#endif


/**
 * Enum PlotFormat
 * must be kept in order of the radio buttons in the plot panel/window.
 */
enum
PlotFormat
{
    PLOT_FORMAT_HPGL,
    PLOT_FORMAT_GERBER,
    PLOT_FORMAT_POST
};


static inline bool IsPostScript( int aFormat )
{
    return aFormat == PLOT_FORMAT_POST;
}


const int PLOT_MIROIR = 1;

// Variables used in Common plot functions
extern wxPoint g_Plot_LastPenPosition;
extern wxPoint g_Plot_PlotOffset;
extern FILE*   g_Plot_PlotOutputFile;
extern double  g_Plot_XScale, g_Plot_YScale;
extern int     g_Plot_DefaultPenWidth, g_Plot_CurrentPenWidth;
extern int     g_Plot_PlotOrientOptions, g_Plot_PenState;


/*******************************/
/* common_plot_functions.cpp   */
/*******************************/
void    SetPlotScale( double aXScale, double aYScale );   // Set the plot scale for the current plotting)
void    Setg_Plot_PlotOffset( wxPoint offset );                // Set the plot offset for the current plotting)
void    InitPlotParametresGERBER( wxPoint offset, double aXScale, double aYScale );
// void    PlotWorkSheet( int format_plot, BASE_SCREEN* screen ); now a member of WinEDA_DrawFrame
void    UserToDeviceCoordinate( wxPoint& pos );

// modifie les coord pos.x et pos.y pour le trace selon l'orientation, l'echelle, les offsets de trace
void    UserToDeviceSize( wxSize& size );

// modifie les dimension size.x et size.y pour le trace selon l'echelle
void    ForcePenReinit();

// set the flag g_Plot_CurrentPenWidth to -1 in order
// to force a pen width redefinition for the next draw command


/*******************************/
/* common_plotPS_functions.cpp */
/*******************************/
void    SetCurrentLineWidthPS( int width );
void    InitPlotParametresPS( wxPoint offset,
                              Ki_PageDescr* sheet,
                              double aXScale,
                              double aYScale,
                              int orient = 0 );
void    SetDefaultLineWidthPS( int width );
void    PlotRectPS( wxPoint p1, wxPoint p2, bool fill, int width = -1 );
void    PlotCirclePS( wxPoint pos, int diametre, bool fill, int width = -1 );
void    PlotArcPS( wxPoint centre, int StAngle, int EndAngle, int rayon, bool fill, int width = -1 );

// Plot an arc: StAngle, EndAngle = start and end arc in 0.1 degree
void    PlotPolyPS( int nb_segm, int* coord, bool fill, int width = -1 );
void    PlotFilledSegmentPS( wxPoint start, wxPoint end, int width );
void    LineTo_PS( wxPoint pos, int plume );
void    PrintHeaderPS( FILE* file,
                       const wxString& Creator,
                       const wxString& FileName,
                       int PageCount,
                       int BBox[4],
                       int PaperOrientation );
bool    CloseFilePS( FILE* plot_file );
void    SetColorMapPS( int color );


/*********************************/
/* common_plotHPGL_functions.cpp */
/*********************************/
void    InitPlotParametresHPGL( wxPoint offset,
                                 double aXScale,
                                 double aYScale,
                                    int orient = 0 );
bool    PrintHeaderHPGL( FILE* plot_file,
                           int pen_speed,
                           int pen_num );
bool    CloseFileHPGL( FILE* plot_file );
void    PlotCircleHPGL( wxPoint centre,
                            int diameter,
                           bool fill,
                            int width = -1 );
void    PlotRectHPGL( wxPoint t1,
                      wxPoint t2,
                         bool fill, 
                          int width = -1 );

void    PlotArcHPGL( wxPoint centre,
                         int StAngle,
                         int EndAngle,
                         int rayon,
                        bool fill,
                         int width = -1 );


void    PlotPolyHPGL(  int nb,
                      int* coord,
                      bool fill,
                       int width = -1 );
void    Move_Plume_HPGL( wxPoint pos,
                             int plume );
void    Plume_HPGL( int plume );

/*********************************/
/* common_plotGERBER_functions.cpp */
/*********************************/
/** Function InitPlotParametresGERBER
 * Set the plot offset for the current plotting
 * @param aOffset = plot offset
 * @param aXScale,aYScale = coordinate scale (scale coefficient for coordinates)
 */
void InitPlotParametresGERBER( wxPoint aOffset,
                                double aXScale,
                                double aYScale );

/** Function Write_Header_GERBER
 * Write GERBER header to file
 * initialize global variable g_Plot_PlotOutputFile
 * @param aTitle: the name of creator (comment)
 * @param aFile: an opened file to write to
 */
void Write_Header_GERBER( const wxString aTitle,
                          FILE* aFile );

/** Function LineTo_GERBER
  * if aCommand = 'U' initialise the starting point of a line
  * if aCommand = 'D' draw a line from the starting point, or last point to aPos
  * @param aPos = end of the current line.
  * @param aCommand = 'U' or 'D' or 'Z' (Pen up , no moving )
 */
void LineTo_GERBER( wxPoint aPos,
                        int aCommand );

/** Function PlotGERBERLine
 * Plot a line
 * @param aStartPos  = starting point of the line
 * @param aEndPos    = ending point of the line
 * @param aThickness = line thickness
*/
void PlotGERBERLine( wxPoint aStartPos,
                     wxPoint aEndPos,
                         int aThickness );

/** Function PlotCircle_GERBER
 * writes a non filled circle to output file
 * Plot one circle as segments (6 to 16 depending on its radius
 * @param aCentre = centre coordintes
 * @param aRadius = radius of the circle
 * @param aWidth  = line width (noc currently used, D_CODEs must be selected before)
*/
void PlotCircle_GERBER( wxPoint aCentre,
                            int aRadius,
                            int aWidth );

/** Function PlotPolygon_GERBER
 * writes a closed polyline (not a filled polygon) to output file
 * @param aCornersCount = numer of corners
 * @param aCoord = buffer of corners coordinates
 * @param aWidth = line width (noc currently used, D_CODEs must be selected before)
*/
void PlotPolygon_GERBER(  int aCornersCount,
                         int* aCoord,
                          int aWidth );

/** Function PlotFilledPolygon_GERBER
 * writes a filled polyline to output file
 * @param aCornersCount = numer of corners
 * @param aCoord = buffer of corners coordinates
*/
void PlotFilledPolygon_GERBER(  int aCornersCount,
                               int* aCoord );

#endif  /* __INCLUDE__PLOT_COMMON_H__ */

