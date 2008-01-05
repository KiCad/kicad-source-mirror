// PolyLine.h ... definition of CPolyLine class
//
// A polyline contains one or more contours, where each contour
// is defined by a list of corners and side-styles
// There may be multiple contours in a polyline.
// The last contour may be open or closed, any others must be closed.
// All of the corners and side-styles are concatenated into 2 arrays,
// separated by setting the end_contour flag of the last corner of 
// each contour.
//
// When used for copper areas, the first contour is the outer edge 
// of the area, subsequent ones are "holes" in the copper.
//
// If a CDisplayList pointer is provided, the polyline can draw itself 

#ifndef POLYLINE_H
#define POLYLINE_H

#include <vector>

#include "defs-macros.h"

#include "GenericPolygonClipperLibrary.h"
#include "php_polygon.h"
#include "php_polygon_vertex.h"

#include "PolyLine2Kicad.h"


#include "freepcb_ids.h"
#include "freepcbDisplayList.h"
#include "math_for_graphics.h"

class CSegment {
public:
	int xi, yi, xf, yf;
	CSegment() {};
	CSegment(int x0, int y0, int x1, int y1) {
		xi = x0; yi = y0; xf = x1; yf = y1; }
};

class CArc {
public: 
	enum{ MAX_STEP = 50*25400 };	// max step is 20 mils
	enum{ MIN_STEPS = 18 };		// min step is 5 degrees
	int style;
	int xi, yi, xf, yf;
	int n_steps;	// number of straight-line segments in gpc_poly 
	bool bFound;
};

class CPolyPt
{
public:
	CPolyPt( int qx=0, int qy=0, bool qf=FALSE )
	{ x=qx; y=qy; end_contour=qf; utility = 0; };
	int x;
	int y;
	bool end_contour;
	int utility;
};

class CPolyLine
{
public:
	enum { STRAIGHT, ARC_CW, ARC_CCW };	// side styles
	enum { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE }; // hatch styles
	enum { DEF_SIZE = 50, DEF_ADD = 50 };	// number of array elements to add at a time

	// constructors/destructor
	CPolyLine( CDisplayList * dl );
	CPolyLine();
	~CPolyLine();

	// functions for modifying polyline
	void Start( int layer, int w, int sel_box, int x, int y,
		int hatch );
	void AppendCorner( int x, int y, int style = STRAIGHT, bool bDraw=TRUE );
	void InsertCorner( int ic, int x, int y );
	void DeleteCorner( int ic, bool bDraw=TRUE );
	void MoveCorner( int ic, int x, int y );
	void Close( int style = STRAIGHT, bool bDraw=TRUE );
	void RemoveContour( int icont );
	void RemoveAllContours( void );

	// drawing functions
	void HighlightSide( int is );
	void HighlightCorner( int ic );
	void StartDraggingToInsertCorner( CDC * pDC, int ic, int x, int y);
	void StartDraggingToMoveCorner( CDC * pDC, int ic, int x, int y);
	void CancelDraggingToInsertCorner( int ic );
	void CancelDraggingToMoveCorner( int ic );
	void Undraw();
	void Draw( CDisplayList * dl = NULL );
	void Hatch();
	void MakeVisible( bool visible = TRUE );
	void MoveOrigin( int x_off, int y_off );

	// misc. functions
	CRect GetBounds();
	CRect GetCornerBounds();
	CRect GetCornerBounds( int icont );
	void Copy( CPolyLine * src );
	bool TestPointInside( int x, int y );
	bool TestPointInsideContour( int icont, int x, int y );
	bool IsCutoutContour( int icont );
	int TestIntersection( CPolyLine * poly );
	void AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num );


	// access functions
	int GetLayer() { return m_layer;}
	int GetNumCorners();
	int GetNumSides();
	int GetClosed();
	int GetNumContours();
	int GetContour( int ic );
	int GetContourStart( int icont );
	int GetContourEnd( int icont );
	int GetContourSize( int icont );
	int GetX( int ic );
	int GetY( int ic );
	int GetEndContour( int ic );
	int GetUtility( int ic ){ return corner[ic].utility; };
	void SetUtility( int ic, int utility ){ corner[ic].utility = utility; };
	int GetW();
	int GetSideStyle( int is );
	int GetHatchStyle(){ return m_HatchStyle; }
	void SetHatch( int hatch ){ Undraw(); m_HatchStyle = hatch; Draw(); };
	void SetX( int ic, int x );
	void SetY( int ic, int y );
	void SetEndContour( int ic, bool end_contour );
//	void SetLayer( int layer );
	void SetW( int w );
	void SetSideStyle( int is, int style );

	// GPC functions
	int MakeGpcPoly( int icontour=0, std::vector<CArc> * arc_array=NULL );
	int FreeGpcPoly();
	gpc_polygon * GetGpcPoly(){ return m_gpc_poly; };
	int NormalizeWithGpc( std::vector<CPolyLine*> * pa=NULL, bool bRetainArcs=FALSE );
	int RestoreArcs( std::vector<CArc> * arc_array, std::vector<CPolyLine*> * pa=NULL );
	CPolyLine * MakePolylineForPad( int type, int x, int y, int w, int l, int r, int angle );
	void AddContourForPadClearance( int type, int x, int y, int w, 
						int l, int r, int angle, int fill_clearance,
						int hole_w, int hole_clearance, bool bThermal=FALSE, int spoke_w=0 );
	void ClipGpcPolygon( gpc_op op, CPolyLine * poly );

	// PHP functions
	int MakePhpPoly();
	void FreePhpPoly();
	void ClipPhpPolygon( int php_op, CPolyLine * poly );

private:
	CDisplayList * m_dlist;		// display list 
	int m_layer;	// layer to draw on
	int m_Width;		// line width
	int m_sel_box;	// corner selection box width/2
	int utility;
public:
	std::vector <CPolyPt> corner;	// array of points for corners
	std::vector <int> side_style;	// array of styles for sides
private:
	std::vector <dl_element*> dl_side;	// graphic elements
	std::vector <dl_element*> dl_side_sel;
	std::vector <dl_element*> dl_corner_sel;
public:
	int m_HatchStyle;	// hatch style, see enum above
	std::vector <CSegment>  m_HatchLines;	// hatch lines
private:
	gpc_polygon * m_gpc_poly;	// polygon in gpc format
	polygon * m_php_poly;
	bool bDrawn;
};

#endif	// #ifndef POLYLINE_H
