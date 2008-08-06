/********************/
/*  plot_common.h   */
/********************/

#ifndef PLOT_COMMON_H
#define PLOT_COMMON_H

#ifndef EDA_BASE
#define COMMON_GLOBL extern
#else
#define COMMON_GLOBL
#endif


/**
 * Enum PlotFormat
 * must be kept in order of the radio buttons in the plot panel/window.
 */
enum PlotFormat {
    PLOT_FORMAT_HPGL,
    PLOT_FORMAT_GERBER,
    PLOT_FORMAT_POST
};


static inline bool IsPostScript( int aFormat )
{
    return aFormat==PLOT_FORMAT_POST;
}


const int PLOT_MIROIR = 1;


/*******************************/
/* common_plot_functions.cpp */
/*******************************/
void    SetPlotScale( double xscale, double yscale );   // Set the plot scale for the current plotting)
void    SetPlotOffset( wxPoint offset );                // Set the plot offset for the current plotting)
void    InitPlotParametresGERBER( wxPoint offset, double xscale, double yscale );
void    PlotWorkSheet( int format_plot, BASE_SCREEN* screen );
void    UserToDeviceCoordinate( wxPoint& pos );

// modifie les coord pos.x et pos.y pour le trace selon l'orientation, l'echelle, les offsets de trace
void    UserToDeviceSize( wxSize& size );

// modifie les dimension size.x et size.y pour le trace selon l'echelle
void    ForcePenReinit();

// set the flag g_CurrentPenWidth to -1 in order
// to force a pen width redefinition for the next draw command


/*******************************/
/* common_plotPS_functions.cpp */
/*******************************/
void    SetCurrentLineWidthPS( int width );
void    InitPlotParametresPS( wxPoint offset,
                              Ki_PageDescr* sheet,
                              double xscale,
                              double yscale,
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
void    InitPlotParametresHPGL( wxPoint offset, double xscale, double yscale, int orient = 0 );
bool    PrintHeaderHPGL( FILE* plot_file, int pen_speed, int pen_num );
bool    CloseFileHPGL( FILE* plot_file );
void    PlotCircleHPGL( wxPoint centre, int diameter, bool fill, int width = -1 );
void    PlotRectHPGL( wxPoint t1, wxPoint t2, bool fill, int width = -1 );
void    PlotArcHPGL( wxPoint centre, int StAngle, int EndAngle, int rayon, bool fill, int width = -1 );
void    PlotPolyHPGL( int nb, int* coord, bool fill, int width = -1 );
void    Move_Plume_HPGL( wxPoint pos, int plume );
void    Plume_HPGL( int plume );

#endif  // PLOT_COMMON_H
