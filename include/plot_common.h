	/********************/
	/*	plot_common.h	*/
	/********************/

#ifndef PLOT_COMMON_H
#define PLOT_COMMON_H

#ifndef EDA_BASE
#define COMMON_GLOBL extern
#else
#define COMMON_GLOBL
#endif


typedef enum {
	PLOT_FORMAT_HPGL,
	PLOT_FORMAT_POST,
	PLOT_FORMAT_GERBER,
	PLOT_FORMAT_POST_A4
	} PlotFormat;

#define  PLOT_MIROIR 1


/*******************************/
/* common_plot_functions.cpp */
/*******************************/
void SetPlotScale(double xscale, double yscale); /* Set the plot scale for the current plotting) */
void SetPlotOffset(wxPoint offset); /* Set the plot offset for the current plotting) */
void InitPlotParametresGERBER(wxPoint offset, double xscale, double yscale);
void PlotWorkSheet(int format_plot, BASE_SCREEN * screen);
void UserToDeviceCoordinate(wxPoint & pos );
	/* modifie les coord pos.x et pos.y pour le trace selon l'orientation, l'echelle, les offsets de trace */
void UserToDeviceSize(wxSize & size );
	/* modifie les dimension size.x et size.y pour le trace selon l'echelle */


/*******************************/
/* common_plotPS_functions.cpp */
/*******************************/
void InitPlotParametresPS( wxPoint offset, Ki_PageDescr * sheet, double xscale, double yscale, int orient = 0);
void SetDefaultLineWidthPS( int width);
void PlotCircle_PS(wxPoint pos, int diametre, int width = -1);
void PlotArcPS(wxPoint centre, int StAngle, int EndAngle, int rayon);
void PlotArcPS(wxPoint centre, int StAngle, int EndAngle, int rayon, int width);
	/* Plot an arc: StAngle, EndAngle = start and end arc in 0.1 degree */
void PlotPolyPS( int nb_segm, int * coord, int fill);
void PlotFilledSegmentPS(wxPoint start , wxPoint end, int width);
void LineTo_PS(wxPoint pos, int plume);
void PrintHeaderPS(FILE * file, const wxString & Creator, const wxString & FileName, int BBox[4]);
bool CloseFilePS(FILE * plot_file);
void SetColorMapPS(int color);



/*********************************/
/* common_plotHPGL_functions.cpp */
/*********************************/
void InitPlotParametresHPGL(wxPoint offset, double xscale, double yscale, int orient = 0);
bool PrintHeaderHPGL(FILE * plot_file, int pen_speed, int pen_num);
bool CloseFileHPGL(FILE * plot_file);
void PlotCircle_HPGL(wxPoint centre, int diameter, int width = -1);
void PlotArcHPGL(wxPoint centre, int StAngle, int EndAngle, int rayon);
void PlotPolyHPGL( int nb, int * coord, int fill);
void Move_Plume_HPGL( wxPoint pos, int plume );
void Plume_HPGL( int plume );

#endif	// PLOT_COMMON_H


