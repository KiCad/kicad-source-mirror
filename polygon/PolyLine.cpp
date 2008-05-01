// PolyLine.cpp ... implementation of CPolyLine class from FreePCB.
//
// implementation for kicad
//
using namespace std;

#include <math.h>
#include <vector>

#include "fctsys.h"


#include "PolyLine.h"

#define to_int(x) (int)round((x))


#define pi  3.14159265359
#define DENOM 10	// to use mils for php clipping
//#define DENOM 1			// to use internal units for php clipping

CPolyLine::CPolyLine()
{ 
	m_HatchStyle = 0;
	m_Width = 0;
	utility = 0;
	m_gpc_poly = new gpc_polygon;
	m_gpc_poly->num_contours = 0;
	m_php_poly = new polygon;
}

// destructor, removes display elements
//
CPolyLine::~CPolyLine()
{
	Undraw();
	FreeGpcPoly();
	delete m_gpc_poly;
	delete m_php_poly;
}

// Use the General Polygon Clipping Library to clip contours
// If this results in new polygons, return them as std::vector p
// If bRetainArcs == TRUE, try to retain arcs in polys
// Returns number of external contours, or -1 if error
//
int CPolyLine::NormalizeWithGpc( std::vector<CPolyLine*> * pa, bool bRetainArcs )
{
	std::vector<CArc> arc_array;

	if( bRetainArcs )
		MakeGpcPoly( -1, &arc_array );
	else
		MakeGpcPoly( -1, NULL );

	Undraw();

	// now, recreate poly
	// first, find outside contours and create new CPolyLines if necessary
	int n_ext_cont = 0;
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ )
	{
		if( !(m_gpc_poly->hole)[ic] )
		{
			if( n_ext_cont == 0 )
			{
				// first external contour, replace this poly
				corner.clear();
				side_style.clear();
				for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
				{
					int x = to_int(((m_gpc_poly->contour)[ic].vertex)[i].x);
					int y = to_int(((m_gpc_poly->contour)[ic].vertex)[i].y);
					if( i==0 )
						Start( m_layer, x, y, m_HatchStyle );
					else
						AppendCorner( x, y, STRAIGHT, FALSE );
				}
				Close();
				n_ext_cont++;
			}
			else if( pa )
			{
				// next external contour, create new poly
				CPolyLine * poly = new CPolyLine;
				pa->push_back(poly);	// put in array
				for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
				{
					int x = to_int(((m_gpc_poly->contour)[ic].vertex)[i].x);
					int y = to_int(((m_gpc_poly->contour)[ic].vertex)[i].y);
					if( i==0 )
						poly->Start( m_layer, x, y, m_HatchStyle );
					else
						poly->AppendCorner( x, y, STRAIGHT, FALSE );
				}
				poly->Close( STRAIGHT, FALSE );
				n_ext_cont++;
			}
		}
	}


	// now add cutouts to the CPolyLine(s)
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ ) 
	{
		if( (m_gpc_poly->hole)[ic] )
		{
			CPolyLine * ext_poly = NULL;
			if( n_ext_cont == 1 )
			{
				ext_poly = this;
			}
			else
			{
				// find the polygon that contains this hole
				for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
				{
					int x = to_int(((m_gpc_poly->contour)[ic].vertex)[i].x);
					int y = to_int(((m_gpc_poly->contour)[ic].vertex)[i].y);
					if( TestPointInside( x, y ) )
						ext_poly = this;
					else
					{
						for( int ext_ic=0; ext_ic<n_ext_cont-1; ext_ic++ )
						{
							if( (*pa)[ext_ic]->TestPointInside( x, y ) )
							{
								ext_poly = (*pa)[ext_ic];
								break;
							}
						}
					}
					if( ext_poly )
						break;
				}
			}
			if( !ext_poly )
				wxASSERT(0);
			for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
			{
				int x = to_int(((m_gpc_poly->contour)[ic].vertex)[i].x);
				int y = to_int(((m_gpc_poly->contour)[ic].vertex)[i].y);
				ext_poly->AppendCorner( x, y, STRAIGHT, FALSE );
			}
			ext_poly->Close( STRAIGHT, FALSE );
		}
	}
	if( bRetainArcs )
		RestoreArcs( &arc_array, pa );
	FreeGpcPoly();

	return n_ext_cont;
}

// make a php_polygon from first contour
int CPolyLine::MakePhpPoly()
{
	FreePhpPoly();
	polygon test_poly;
	int nv = GetContourEnd(0);
	for( int iv=0; iv<=nv; iv++ )
	{
		int x = GetX(iv)/DENOM;
		int y = GetY(iv)/DENOM;
		m_php_poly->addv( x, y );
	}
	return 0;
}

void CPolyLine::FreePhpPoly()
{
	// delete all vertices
	while( m_php_poly->m_cnt > 1 )
	{
		vertex * fv = m_php_poly->getFirst();
		m_php_poly->del( fv->m_nextV );
	}
	delete m_php_poly->m_first;
	m_php_poly->m_first = NULL;
	m_php_poly->m_cnt = 0;
}

// Use the php clipping lib to clip this poly against poly
//
void CPolyLine::ClipPhpPolygon( int php_op, CPolyLine * poly )
{
	Undraw();
	poly->MakePhpPoly();
	MakePhpPoly();
	polygon * p = m_php_poly->boolean( poly->m_php_poly, php_op );
	poly->FreePhpPoly();
	FreePhpPoly();

	if( p )
	{
		// now screw with the PolyLine
		corner.clear();
		side_style.clear();
		do
		{
			vertex * v = p->getFirst();
			Start( m_layer, to_int(v->X()*DENOM), to_int(v->Y()*DENOM), m_HatchStyle );
			do
			{
				vertex * n = v->Next();
				AppendCorner( to_int(v->X()*DENOM), to_int((v->Y()*DENOM )) );
				v = n;
			}
			while( v->id() != p->getFirst()->id() );
			Close();
//			p = p->NextPoly();
			delete p;
			p = NULL;
		}
		while( p );
	}
	Draw();
}

// make a gpc_polygon for a closed polyline contour
// approximates arcs with multiple straight-line segments
// if icontour = -1, make polygon with all contours,
// combining intersecting contours if possible
// returns data on arcs in arc_array
//
int CPolyLine::MakeGpcPoly( int icontour, std::vector<CArc> * arc_array )
{
	if( m_gpc_poly->num_contours )
		FreeGpcPoly();
	if( !GetClosed() && (icontour == (GetNumContours()-1) || icontour == -1))
		return 1;	// error

	// initialize m_gpc_poly
	m_gpc_poly->num_contours = 0;
	m_gpc_poly->hole = NULL;
	m_gpc_poly->contour = NULL;
	int n_arcs = 0;

	int first_contour = icontour;
	int last_contour = icontour;
	if( icontour == -1 )
	{
		first_contour = 0;
		last_contour = GetNumContours() - 1;
	}
	if( arc_array )
		arc_array->clear();
	int iarc = 0;
	for( int icont=first_contour; icont<=last_contour; icont++ )
	{
		// make gpc_polygon for this contour
		gpc_polygon * gpc = new gpc_polygon;
		gpc->num_contours = 0;
		gpc->hole = NULL;
		gpc->contour = NULL;

		// first, calculate number of vertices in contour
		int n_vertices = 0;
		int ic_st = GetContourStart(icont);
		int ic_end = GetContourEnd(icont);
		for( int ic=ic_st; ic<=ic_end; ic++ )
		{
			int style = side_style[ic];
			int x1 = corner[ic].x;
			int y1 = corner[ic].y;
			int x2, y2;
			if( ic < ic_end )
			{
				x2 = corner[ic+1].x;
				y2 = corner[ic+1].y;
			}
			else
			{
				x2 = corner[ic_st].x;
				y2 = corner[ic_st].y;
			}
			if( style == STRAIGHT )
				n_vertices++;
			else
			{
				// style is ARC_CW or ARC_CCW
				int n;	// number of steps for arcs
				n = (abs(x2-x1)+abs(y2-y1))/(CArc::MAX_STEP);
				n = max( n, CArc::MIN_STEPS );	// or at most 5 degrees of arc
				n_vertices += n;
				n_arcs++;
			}
		}
		// now create gcp_vertex_list for this contour
		gpc_vertex_list * g_v_list = new gpc_vertex_list;
		g_v_list->vertex = (gpc_vertex*)calloc( sizeof(gpc_vertex), n_vertices );
		g_v_list->num_vertices = n_vertices;
		int ivtx = 0;
		for( int ic=ic_st; ic<=ic_end; ic++ )
		{
			int style = side_style[ic];
			int x1 = corner[ic].x;
			int y1 = corner[ic].y;
			int x2, y2;
			if( ic < ic_end )
			{
				x2 = corner[ic+1].x;
				y2 = corner[ic+1].y;
			}
			else
			{
				x2 = corner[ic_st].x;
				y2 = corner[ic_st].y;
			}
			if( style == STRAIGHT )
			{
				g_v_list->vertex[ivtx].x = x1;
				g_v_list->vertex[ivtx].y = y1;
				ivtx++;
			}
			else
			{
				// style is arc_cw or arc_ccw
				int n;	// number of steps for arcs
				n = (abs(x2-x1)+abs(y2-y1))/(CArc::MAX_STEP);
				n = max( n, CArc::MIN_STEPS );	// or at most 5 degrees of arc
				double xo, yo, theta1, theta2, a, b;
				a = fabs( (double)(x1 - x2) );
				b = fabs( (double)(y1 - y2) );
				if( style == CPolyLine::ARC_CW )
				{
					// clockwise arc (ie.quadrant of ellipse)
					if( x2 > x1 && y2 > y1 )
					{
						// first quadrant, draw second quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = pi;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						// second quadrant, draw third quadrant of ellipse
						xo = x1;	
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = pi;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						// third quadrant, draw fourth quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = 2.0*pi;
						theta2 = 3.0*pi/2.0;
					}
					else
					{
						xo = x1;	// fourth quadrant, draw first quadrant of ellipse
						yo = y2;
						theta1 = pi/2.0;
						theta2 = 0.0;
					}
				}
				else
				{
					// counter-clockwise arc
					if( x2 > x1 && y2 > y1 )
					{
						xo = x1;	// first quadrant, draw fourth quadrant of ellipse
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = 2.0*pi;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						xo = x2;	// second quadrant
						yo = y1;
						theta1 = 0.0;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						xo = x1;	// third quadrant
						yo = y2;
						theta1 = pi/2.0;
						theta2 = pi;
					}
					else
					{
						xo = x2;	// fourth quadrant
						yo = y1;
						theta1 = pi;
						theta2 = 3.0*pi/2.0;
					}
				}
				// now write steps for arc
				if( arc_array )
				{
					CArc new_arc;
					new_arc.style = style;
					new_arc.n_steps = n;
					new_arc.xi = x1;
					new_arc.yi = y1;
					new_arc.xf = x2;
					new_arc.yf = y2;
					arc_array->push_back(new_arc);
					iarc++;
				}
				for( int is=0; is<n; is++ )
				{
					double theta = theta1 + ((theta2-theta1)*(double)is)/n;
					double x = xo + a*cos(theta);
					double y = yo + b*sin(theta);
					if( is == 0 )
					{
						x = x1;
						y = y1;
					}
					g_v_list->vertex[ivtx].x = x;
					g_v_list->vertex[ivtx].y = y;
					ivtx++;
				}
			}
		}
		if( n_vertices != ivtx )
			wxASSERT(0);
		// add vertex_list to gpc
		gpc_add_contour( gpc, g_v_list, 0 );
		// now clip m_gpc_poly with gpc, put new poly into result
		gpc_polygon * result = new gpc_polygon;
		if( icontour == -1 && icont != 0 )
			gpc_polygon_clip( GPC_DIFF, m_gpc_poly, gpc, result );	// hole
		else
			gpc_polygon_clip( GPC_UNION, m_gpc_poly, gpc, result );	// outside
		// now copy result to m_gpc_poly
		gpc_free_polygon( m_gpc_poly );
		delete m_gpc_poly;
		m_gpc_poly = result;
		gpc_free_polygon( gpc );
		delete gpc;
		free( g_v_list->vertex );
		free( g_v_list );
	}
	return 0;
}

int CPolyLine::FreeGpcPoly()
{
	if( m_gpc_poly->num_contours )
	{
		delete m_gpc_poly->contour->vertex;
		delete m_gpc_poly->contour;
		delete m_gpc_poly->hole;
	}
	m_gpc_poly->num_contours = 0;
	return 0;
}


// Restore arcs to a polygon where they were replaced with steps
// If pa != NULL, also use polygons in pa array
//
int CPolyLine::RestoreArcs( std::vector<CArc> * arc_array, std::vector<CPolyLine*> * pa )
{
	// get poly info
	int n_polys = 1;
	if( pa )
		n_polys += pa->size();
	CPolyLine * poly;

	// undraw polys and clear utility flag for all corners
	for( int ip=0; ip<n_polys; ip++ )
	{
		if( ip == 0 )
			poly = this;
		else
			poly = (*pa)[ip-1];
		poly->Undraw();
		for( int ic=0; ic<poly->GetNumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// clear utility flag
	}

	// find arcs and replace them
	bool bFound;
	int arc_start = 0;
	int arc_end = 0;
	for( unsigned iarc=0; iarc<arc_array->size(); iarc++ )
	{
		int arc_xi = (*arc_array)[iarc].xi;
		int arc_yi = (*arc_array)[iarc].yi;
		int arc_xf = (*arc_array)[iarc].xf;
		int arc_yf = (*arc_array)[iarc].yf;
		int n_steps = (*arc_array)[iarc].n_steps;
		int style = (*arc_array)[iarc].style;
		bFound = FALSE;
		// loop through polys
		for( int ip=0; ip<n_polys; ip++ )
		{
			if( ip == 0 )
				poly = this;
			else
				poly = (*pa)[ip-1];
			for( int icont=0; icont<poly->GetNumContours(); icont++ )
			{
				int ic_start = poly->GetContourStart(icont);
				int ic_end = poly->GetContourEnd(icont);
				if( (ic_end-ic_start) > n_steps )
				{
					for( int ic=ic_start; ic<=ic_end; ic++ )
					{
						int ic_next = ic+1;
						if( ic_next > ic_end )
							ic_next = ic_start;
						int xi = poly->GetX(ic);
						int yi = poly->GetY(ic);
						if( xi == arc_xi && yi == arc_yi )
						{
							// test for forward arc
							int ic2 = ic + n_steps;
							if( ic2 > ic_end )
								ic2 = ic2 - ic_end + ic_start - 1;
							int xf = poly->GetX(ic2);
							int yf = poly->GetY(ic2);
							if( xf == arc_xf && yf == arc_yf )
							{
								// arc from ic to ic2
								bFound = TRUE;
								arc_start = ic;
								arc_end = ic2;
							}
							else
							{
								// try reverse arc
								ic2 = ic - n_steps;
								if( ic2 < ic_start )
									ic2 = ic2 - ic_start + ic_end + 1;
								xf = poly->GetX(ic2);
								yf = poly->GetY(ic2);
								if( xf == arc_xf && yf == arc_yf )
								{
									// arc from ic2 to ic
									bFound = TRUE; 
									arc_start = ic2;
									arc_end = ic;
									style = 3 - style;
								}
							}
							if( bFound )
							{
								poly->side_style[arc_start] = style;
								// mark corners for deletion from arc_start+1 to arc_end-1
								for( int i=arc_start+1; i!=arc_end; )
								{
									if( i > ic_end )
										i = ic_start;
									poly->SetUtility( i, 1 );
									if( i == ic_end )
										i = ic_start;
									else
										i++;
								}
								break;
							}
						}
						if( bFound )
							break;
					}
				}
				if( bFound )
					break;
			}
		}
		if( bFound )
			(*arc_array)[iarc].bFound = TRUE;
	}

	// now delete all marked corners
	for( int ip=0; ip<n_polys; ip++ )
	{
		if( ip == 0 )
			poly = this;
		else
			poly = (*pa)[ip-1];
		for( int ic=poly->GetNumCorners()-1; ic>=0; ic-- )
		{
			if( poly->GetUtility(ic) )
				poly->DeleteCorner( ic, FALSE );
		}
	}
	return 0;
}

// initialize new polyline
// set layer, width, selection box size, starting point, id and pointer
//
// if sel_box = 0, don't create selection elements at all
//
// if polyline is board outline, enter with:
//	id.type = ID_BOARD
//	id.st = ID_BOARD_OUTLINE
//	id.i = 0
//	ptr = NULL
//
// if polyline is copper area, enter with:
//	id.type = ID_NET;
//	id.st = ID_AREA
//	id.i = index to area
//	ptr = pointer to net
//
void CPolyLine::Start( int layer, int x, int y, int hatch )
{
	m_layer = layer;
	m_HatchStyle = hatch;
	CPolyPt poly_pt( x, y );
	poly_pt.end_contour = FALSE;

	corner.push_back(poly_pt);
	side_style.push_back(0);
}

// add a corner to unclosed polyline
//
void CPolyLine::AppendCorner( int x, int y, int style, bool bDraw )
{
	Undraw();
	CPolyPt poly_pt( x, y );
	poly_pt.end_contour = FALSE;

	// add entries for new corner and side
	corner.push_back(poly_pt);
	side_style.push_back(style);
	if( corner.size() > 0 && !corner[corner.size()-1].end_contour )
		side_style[corner.size()-1] = style;
	int dl_type;
	if( style == CPolyLine::STRAIGHT )
		dl_type = DL_LINE;
	else if( style == CPolyLine::ARC_CW )
		dl_type = DL_ARC_CW;
	else if( style == CPolyLine::ARC_CCW )
		dl_type = DL_ARC_CCW;
	else
		wxASSERT(0);
	if( bDraw )
		Draw();
}

// close last polyline contour
//
void CPolyLine::Close( int style, bool bDraw )
{
	if( GetClosed() )
		wxASSERT(0);
	Undraw();
	side_style[corner.size()-1] = style;
	corner[corner.size()-1].end_contour = TRUE;
	if( bDraw )
		Draw();
}

// move corner of polyline
//
void CPolyLine::MoveCorner( int ic, int x, int y )
{
	Undraw();
	corner[ic].x = x;
	corner[ic].y = y;
	Draw();
}

// delete corner and adjust arrays
//
void CPolyLine::DeleteCorner( int ic, bool bDraw )
{
	Undraw();
	int icont = GetContour( ic );
	int istart = GetContourStart( icont );
	int iend = GetContourEnd( icont );
	bool bClosed = icont < GetNumContours()-1 || GetClosed();

	if( !bClosed )
	{
		// open contour, must be last contour
		corner.erase( corner.begin() + ic );

		if( ic != istart )
			side_style.erase( side_style.begin() + ic-1 );
	}
	else
	{
		// closed contour
		corner.erase( corner.begin() + ic );
		side_style.erase( side_style.begin() + ic );
		if( ic == iend )
			corner[ic-1].end_contour = TRUE;
	}
	if( bClosed && GetContourSize(icont) < 3 )
	{
		// delete the entire contour
		RemoveContour( icont );
	}
	if( bDraw )
		Draw();
}

/******************************************/
void CPolyLine::RemoveContour( int icont )
/******************************************/
/**
 * Function RemoveContour
 * @param icont = contour number to remove
 * remove a contour only if there is more than 1 contour
 */
{
	Undraw();
	int istart = GetContourStart( icont );
	int iend = GetContourEnd( icont );

	if( icont == 0 && GetNumContours() == 1 )
	{
		// remove the only contour
		wxASSERT(0);
	}
	else if( icont == GetNumContours()-1 )
	{
		// remove last contour
		corner.erase( corner.begin() + istart, corner.end() );
		side_style.erase( side_style.begin() + istart, side_style.end() );
	}
	else
	{
		// remove closed contour
		for( int ic=iend; ic>=istart; ic-- )
		{
			corner.erase( corner.begin() + ic );
			side_style.erase( side_style.begin() + ic );
		}
	}
	Draw();
}


/******************************************/
void CPolyLine::RemoveAllContours( void )
/******************************************/
/**
 * function RemoveAllContours
 * removes all corners from the lists.
 * Others params are not chnaged
 */
{
	corner.clear( );
	side_style.clear( );
}


/** Function InsertCorner
 * insert a new corner between two existing corners
 * @param ic = index for the insertion point: the corner is inserted AFTER ic
 * @param x, y = coordinates corner to insert
 */
void CPolyLine::InsertCorner( int ic, int x, int y )
{
	Undraw();
	if ( (unsigned)(ic) >= corner.size() )
	{
		corner.push_back( CPolyPt(x,y) );
		side_style.push_back( STRAIGHT );
	}
	else
	{
		corner.insert( corner.begin() + ic + 1, CPolyPt(x,y) );
		side_style.insert( side_style.begin() + ic + 1, STRAIGHT );
	}
	
	if( (unsigned)(ic+1) < corner.size() )
	{
		if( corner[ic].end_contour )
		{
			corner[ic+1].end_contour = TRUE;
			corner[ic].end_contour = FALSE;
		}
	}
	Draw();
}

// undraw polyline by removing all graphic elements from display list
//
void CPolyLine::Undraw()
{
	m_HatchLines.clear();
	bDrawn = FALSE;
}

// draw polyline by adding all graphics to display list
// if side style is ARC_CW or ARC_CCW but endpoints are not angled,
// convert to STRAIGHT
//
void CPolyLine::Draw( )
{

	// first, undraw if necessary
	if( bDrawn )
		Undraw(); 

	Hatch();
	bDrawn = TRUE;
}


int CPolyLine::GetX( int ic ) 
{	
	return corner[ic].x; 
}

int CPolyLine::GetY( int ic ) 
{	
	return corner[ic].y; 
}

int CPolyLine::GetEndContour( int ic ) 
{	
	return corner[ic].end_contour; 
}

CRect CPolyLine::GetBounds()
{
	CRect r = GetCornerBounds();
	r.left -= m_Width/2;
	r.right += m_Width/2;
	r.bottom -= m_Width/2;
	r.top += m_Width/2;
	return r;
}

CRect CPolyLine::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	for( unsigned i=0; i<corner.size(); i++ )
	{
		r.left = min( r.left, corner[i].x );
		r.right = max( r.right, corner[i].x );
		r.bottom = min( r.bottom, corner[i].y );
		r.top = max( r.top, corner[i].y );
	}
	return r;
}

CRect CPolyLine::GetCornerBounds( int icont )
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	int istart = GetContourStart( icont );
	int iend = GetContourEnd( icont );
	for( int i=istart; i<=iend; i++ )
	{
		r.left = min( r.left, corner[i].x );
		r.right = max( r.right, corner[i].x );
		r.bottom = min( r.bottom, corner[i].y );
		r.top = max( r.top, corner[i].y );
	}
	return r;
}

int CPolyLine::GetNumCorners() 
{	
	return corner.size();	
}

int CPolyLine::GetNumSides() 
{	
	if( GetClosed() )
		return corner.size();	
	else
		return corner.size()-1;	
}

int CPolyLine::GetNumContours()
{
	int ncont = 0;
	if( !corner.size() )
		return 0;

	for( unsigned ic=0; ic<corner.size(); ic++ )
		if( corner[ic].end_contour )
			ncont++;
	if( !corner[corner.size()-1].end_contour )
		ncont++;
	return ncont;
}

int CPolyLine::GetContour( int ic )
{
	int ncont = 0;
	for( int i=0; i<ic; i++ )
	{
		if( corner[i].end_contour )
			ncont++;
	}
	return ncont;
}

int CPolyLine::GetContourStart( int icont )
{
	if( icont == 0 )
		return 0;

	int ncont = 0;
	for( unsigned i=0; i<corner.size(); i++ )
	{
		if( corner[i].end_contour )
		{
			ncont++;
			if( ncont == icont )
				return i+1;
		}
	}
	wxASSERT(0);
	return 0;
}

int CPolyLine::GetContourEnd( int icont )
{
	if( icont < 0 )
		return 0;

	if( icont == GetNumContours()-1 )
		return corner.size()-1;

	int ncont = 0;
	for( unsigned i=0; i<corner.size(); i++ )
	{
		if( corner[i].end_contour )
		{
			if( ncont == icont )
				return i;
			ncont++;
		}
	}
	wxASSERT(0);
	return 0;
}

int CPolyLine::GetContourSize( int icont )
{
	return GetContourEnd(icont) - GetContourStart(icont) + 1;
}


void CPolyLine::SetSideStyle( int is, int style ) 
{	
	Undraw();
	CPoint p1, p2;
	if( is == (int)(corner.size()-1) )
	{
		p1.x = corner[corner.size()-1].x;
		p1.y = corner[corner.size()-1].y;
		p2.x = corner[0].x;
		p2.y = corner[0].y;
	}
	else
	{
		p1.x = corner[is].x;
		p1.y = corner[is].y;
		p2.x = corner[is+1].x;
		p2.y = corner[is+1].y;
	}
	if( p1.x == p2.x || p1.y == p2.y )
		side_style[is] = STRAIGHT;
	else
		side_style[is] = style;	
	Draw();
}

int CPolyLine::GetSideStyle( int is ) 
{	
	return side_style[is];	
}


int CPolyLine::GetClosed() 
{	
	if( corner.size() == 0 )
		return 0;
	else
		return corner[corner.size()-1].end_contour; 
}

// draw hatch lines
//
void CPolyLine::Hatch()
{
	m_HatchLines.clear();
	if( m_HatchStyle == NO_HATCH )
	{
		return;
	}

	int layer = m_layer;
	
	if( GetClosed() )	// If not closed, the poly is beeing created and not finalised. Not not hatch
	{
		enum {
			MAXPTS = 100,
		};
		int xx[MAXPTS], yy[MAXPTS];

		// define range for hatch lines
		int min_x = corner[0].x;
		int max_x = corner[0].x;
		int min_y = corner[0].y;
		int max_y = corner[0].y;
		for( unsigned ic = 1; ic < corner.size(); ic++ )
		{
			if( corner[ic].x < min_x )
				min_x = corner[ic].x;
			if( corner[ic].x > max_x )
				max_x = corner[ic].x;
			if( corner[ic].y < min_y )
				min_y = corner[ic].y;
			if( corner[ic].y > max_y )
				max_y = corner[ic].y;
		}
		int slope_flag = (layer & 1) ? 1 : -1;	// 1 or -1
		double slope = 0.707106*slope_flag;
		int spacing;
		if( m_HatchStyle == DIAGONAL_EDGE )
			spacing = 10*PCBU_PER_MIL;
		else
			spacing = 50*PCBU_PER_MIL;
		int max_a, min_a;
		if( slope_flag == 1 )
		{
			max_a = (int)(max_y - slope*min_x);
			min_a = (int)(min_y - slope*max_x);
		}
		else
		{
			max_a = (int)(max_y - slope*max_x);
			min_a = (int)(min_y - slope*min_x);
		}
		min_a = (min_a/spacing)*spacing;
		// calculate an offset depending on layer number, for a better display of hatches on a multilayer board
		int offset =  (layer * 7) / 8;
		min_a += offset;

		// now calculate and draw hatch lines
		int nc = corner.size();
		// loop through hatch lines
		for( int a=min_a; a<max_a; a+=spacing )
		{
			// get intersection points for this hatch line
			int nloops = 0;
			int npts;
			// make this a loop in case my homebrew hatching algorithm screws up
			do
			{
				npts = 0;
				int i_start_contour = 0;
				for( int ic=0; ic<nc; ic++ )
				{
					double x, y, x2, y2;
					int ok;
					if( corner[ic].end_contour || (ic == (int)(corner.size()-1)) )
					{
						ok = FindLineSegmentIntersection( a, slope, 
								corner[ic].x, corner[ic].y,
								corner[i_start_contour].x, corner[i_start_contour].y, 
								side_style[ic],
								&x, &y, &x2, &y2 );
						i_start_contour = ic + 1;
					}
					else
					{
						ok = FindLineSegmentIntersection( a, slope, 
								corner[ic].x, corner[ic].y, 
								corner[ic+1].x, corner[ic+1].y,
								side_style[ic],
								&x, &y, &x2, &y2 );
					}
					if( ok )
					{
						xx[npts] = (int)x;
						yy[npts] = (int)y;
						npts++;
						wxASSERT( npts<MAXPTS );	// overflow
					}
					if( ok == 2 )
					{
						xx[npts] = (int)x2;
						yy[npts] = (int)y2;
						npts++;
						wxASSERT( npts<MAXPTS );	// overflow
					}
				}
				nloops++;
				a += PCBU_PER_MIL/100;
			} while( npts%2 != 0 && nloops < 3 );

/*  DICK 1/22/08: this was firing repeatedly on me, needed to comment out to get
    my work done:
			wxASSERT( npts%2==0 );	// odd number of intersection points, error
*/

			// sort points in order of descending x (if more than 2)
			if( npts>2 )
			{
				for( int istart=0; istart<(npts-1); istart++ )
				{
					int max_x = INT_MIN;
					int imax = INT_MIN;
					for( int i=istart; i<npts; i++ )
					{
						if( xx[i] > max_x )
						{
							max_x = xx[i];
							imax = i;
						}
					}
					int temp = xx[istart];
					xx[istart] = xx[imax];
					xx[imax] = temp;
					temp = yy[istart];
					yy[istart] = yy[imax];
					yy[imax] = temp;
				}
			}

			// draw lines
			for( int ip=0; ip<npts; ip+=2 )
			{
				double dx = xx[ip+1] - xx[ip];
				if( m_HatchStyle == DIAGONAL_FULL || fabs(dx) < 40*NM_PER_MIL )
				{
					m_HatchLines.push_back(CSegment(xx[ip], yy[ip], xx[ip+1], yy[ip+1]) );
				}
				else
				{
					double dy = yy[ip+1] - yy[ip];	
					double slope = dy/dx;
					if( dx > 0 )
						dx = 20*NM_PER_MIL;
					else
						dx = -20*NM_PER_MIL;
					double x1 = xx[ip] + dx;
					double x2 = xx[ip+1] - dx;
					double y1 = yy[ip] + dx*slope;
					double y2 = yy[ip+1] - dx*slope;
					m_HatchLines.push_back(CSegment(xx[ip], yy[ip], to_int(x1), to_int(y1)) );
					m_HatchLines.push_back(CSegment(xx[ip+1], yy[ip+1], to_int(x2), to_int(y2)) );
				}
			}
		} // end for 
	}
}

// test to see if a point is inside polyline
//
bool CPolyLine::TestPointInside( int x, int y )
{
	enum { MAXPTS = 100 };
	if( !GetClosed() )
		wxASSERT(0);

	// define line passing through (x,y), with slope = 2/3;
	// get intersection points
	double xx[MAXPTS], yy[MAXPTS];
	double slope = (double)2.0/3.0;
	double a = y - slope*x;
	int nloops = 0;
	int npts;
	// make this a loop so if my homebrew algorithm screws up, we try it again
	do
	{
		// now find all intersection points of line with polyline sides
		npts = 0;
		for( int icont=0; icont<GetNumContours(); icont++ )
		{
			int istart = GetContourStart( icont );
			int iend = GetContourEnd( icont );
			for( int ic=istart; ic<=iend; ic++ )
			{
				double x, y, x2, y2;
				int ok;
				if( ic == istart )
					ok = FindLineSegmentIntersection( a, slope, 
					corner[iend].x, corner[iend].y,
					corner[istart].x, corner[istart].y, 
					side_style[corner.size()-1],
					&x, &y, &x2, &y2 );
				else
					ok = FindLineSegmentIntersection( a, slope, 
					corner[ic-1].x, corner[ic-1].y, 
					corner[ic].x, corner[ic].y,
					side_style[ic-1],
					&x, &y, &x2, &y2 );
				if( ok )
				{
					xx[npts] = (int)x;
					yy[npts] = (int)y;
					npts++;
					wxASSERT( npts<MAXPTS );	// overflow
				}
				if( ok == 2 )
				{
					xx[npts] = (int)x2;
					yy[npts] = (int)y2;
					npts++;
					wxASSERT( npts<MAXPTS );	// overflow
				}
			}
		}
		nloops++;
		a += PCBU_PER_MIL/100;
	} while( npts%2 != 0 && nloops < 3 );
	wxASSERT( npts%2==0 );	// odd number of intersection points, error

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
	{
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	}
	if( ncount%2 )
		return TRUE;
	else
		return FALSE;
}

// test to see if a point is inside polyline contour
//
bool CPolyLine::TestPointInsideContour( int icont, int x, int y )
{
	if( icont >= GetNumContours() )
		return FALSE;

	enum { MAXPTS = 100 };
	if( !GetClosed() )
		wxASSERT(0);

	// define line passing through (x,y), with slope = 2/3;
	// get intersection points
	double xx[MAXPTS], yy[MAXPTS];
	double slope = (double)2.0/3.0;
	double a = y - slope*x;
	int nloops = 0;
	int npts;
	// make this a loop so if my homebrew algorithm screws up, we try it again
	do
	{
		// now find all intersection points of line with polyline sides
		npts = 0;
		int istart = GetContourStart( icont );
		int iend = GetContourEnd( icont );
		for( int ic=istart; ic<=iend; ic++ )
		{
			double x, y, x2, y2;
			int ok;
			if( ic == istart )
				ok = FindLineSegmentIntersection( a, slope, 
				corner[iend].x, corner[iend].y,
				corner[istart].x, corner[istart].y, 
				side_style[corner.size()-1],
				&x, &y, &x2, &y2 );
			else
				ok = FindLineSegmentIntersection( a, slope, 
				corner[ic-1].x, corner[ic-1].y, 
				corner[ic].x, corner[ic].y,
				side_style[ic-1],
				&x, &y, &x2, &y2 );
			if( ok )
			{
				xx[npts] = (int)x;
				yy[npts] = (int)y;
				npts++;
				wxASSERT( npts<MAXPTS );	// overflow
			}
			if( ok == 2 )
			{
				xx[npts] = (int)x2;
				yy[npts] = (int)y2;
				npts++;
				wxASSERT( npts<MAXPTS );	// overflow
			}
		}
		nloops++;
		a += PCBU_PER_MIL/100;
	} while( npts%2 != 0 && nloops < 3 );
	wxASSERT( npts%2==0 );	// odd number of intersection points, error

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
	{
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	}
	if( ncount%2 )
		return TRUE;
	else
		return FALSE;
}


// copy data from another poly, but don't draw it
//
void CPolyLine::Copy( CPolyLine * src )
{
	Undraw();

	// copy corners
	for( unsigned ii=0; ii < src->corner.size(); ii++ )
		corner.push_back(src->corner[ii]);
	// copy side styles
	for( unsigned ii=0; ii < src->side_style.size(); ii++ )
		side_style.push_back(src->side_style[ii]);
	// don't copy the Gpc_poly, just clear the old one
	FreeGpcPoly();
}


/*******************************************/
bool CPolyLine::IsCutoutContour( int icont )
/*******************************************/
/*
 * return true if the corner icont is inside the outline (i.e it is a hole)
 */
{
	int ncont = GetContour( icont );
	if ( ncont == 0 )	// the first contour is the main outline, not an hole
		return false;
	return true;
}

void CPolyLine::MoveOrigin( int x_off, int y_off )
{
	Undraw();
	for( int ic=0; ic < GetNumCorners(); ic++ )
	{
		SetX( ic, GetX(ic) + x_off );
		SetY( ic, GetY(ic) + y_off );
	}
	Draw();
}


// Set various parameters:
//   the calling function should Undraw() before calling them,
//   and Draw() after
//
void CPolyLine::SetX( int ic, int x ) { corner[ic].x = x; }
void CPolyLine::SetY( int ic, int y ) { corner[ic].y = y; }

void CPolyLine::SetEndContour( int ic, bool end_contour )
{
	corner[ic].end_contour = end_contour;
}

// Create CPolyLine for a pad
//
CPolyLine * CPolyLine::MakePolylineForPad( int type, int x, int y, int w, int l, int r, int angle )
{
	CPolyLine * poly = new CPolyLine;
	int dx = l/2;
	int dy = w/2;
	if( angle%180 == 90 )
	{
		dx = w/2;
		dy = l/2;
	}
	if( type == PAD_ROUND )
	{
		poly->Start( 0, x-dx, y, 0 );
		poly->AppendCorner( x, y+dy, ARC_CW, 0 );
		poly->AppendCorner( x+dx, y, ARC_CW, 0 );
		poly->AppendCorner( x, y-dy, ARC_CW, 0 );
		poly->Close( ARC_CW );
	}
	return poly;
}

// Add cutout for a pad
// Convert arcs to multiple straight lines
// Do NOT draw or undraw
//
void CPolyLine::AddContourForPadClearance( int type, int x, int y, int w, 
						int l, int r, int angle, int fill_clearance,
						int hole_w, int hole_clearance, bool bThermal, int spoke_w )
{
	int dx = l/2;
	int dy = w/2;
	if( angle%180 == 90 )
	{
		dx = w/2;
		dy = l/2;
	}
	int x_clearance = max( fill_clearance, hole_clearance+hole_w/2-dx);		
	int y_clearance = max( fill_clearance, hole_clearance+hole_w/2-dy);
	dx += x_clearance;
	dy += y_clearance;
	if( !bThermal )
	{
		// normal clearance
		if( type == PAD_ROUND || (type == PAD_NONE && hole_w > 0) )
		{
			AppendCorner( x-dx, y, ARC_CW, 0 );
			AppendCorner( x, y+dy, ARC_CW, 0 );
			AppendCorner( x+dx, y, ARC_CW, 0 );
			AppendCorner( x, y-dy, ARC_CW, 0 );
			Close( ARC_CW ); 
		}
		else if( type == PAD_SQUARE || type == PAD_RECT 
			|| type == PAD_RRECT || type == PAD_OVAL )
		{
			AppendCorner( x-dx, y-dy, STRAIGHT, 0 );
			AppendCorner( x+dx, y-dy, STRAIGHT, 0 );
			AppendCorner( x+dx, y+dy, STRAIGHT, 0 );
			AppendCorner( x-dx, y+dy, STRAIGHT, 0 );
			Close( STRAIGHT ); 
		}
	}
	else
	{
		// thermal relief
		if( type == PAD_ROUND || (type == PAD_NONE && hole_w > 0) )
		{
			// draw 4 "wedges"
			double r = max(w/2 + fill_clearance, hole_w/2 + hole_clearance);
			double start_angle = asin( spoke_w/(2.0*r) );
			double th1, th2, corner_x, corner_y;
			th1 = th2 = corner_x = corner_y = 0; // gcc warning fix
			for( int i=0; i<4; i++ )
			{
				if( i == 0 )
				{
					corner_x = spoke_w/2;
					corner_y = spoke_w/2;
					th1 = start_angle;
					th2 = pi/2.0 - start_angle;
				}
				else if( i == 1 )
				{
					corner_x = -spoke_w/2;
					corner_y = spoke_w/2;
					th1 = pi/2.0 + start_angle;
					th2 = pi - start_angle;
				}
				else if( i == 2 )
				{
					corner_x = -spoke_w/2;
					corner_y = -spoke_w/2;
					th1 = -pi + start_angle;
					th2 = -pi/2.0 - start_angle;
				}
				else if( i == 3 )
				{
					corner_x = spoke_w/2;
					corner_y = -spoke_w/2;
					th1 = -pi/2.0 + start_angle;
					th2 = -start_angle;
				}
				AppendCorner( to_int(x+corner_x), to_int(y+corner_y), STRAIGHT, 0 );
				AppendCorner( to_int(x+r*cos(th1)), to_int(y+r*sin(th1)),  STRAIGHT, 0 );
				AppendCorner( to_int(x+r*cos(th2)), to_int(y+r*sin(th2)),  ARC_CCW, 0 );
				Close( STRAIGHT );
			}
		}
		else if( type == PAD_SQUARE || type == PAD_RECT 
			|| type == PAD_RRECT || type == PAD_OVAL )
		{
			// draw 4 rectangles
			int xL = x - dx;
			int xR = x - spoke_w/2;
			int yB = y - dy;
			int yT = y - spoke_w/2;
			AppendCorner( xL, yB, STRAIGHT, 0 );
			AppendCorner( xR, yB, STRAIGHT, 0 );
			AppendCorner( xR, yT, STRAIGHT, 0 );
			AppendCorner( xL, yT, STRAIGHT, 0 );
			Close( STRAIGHT ); 
			xL = x + spoke_w/2;
			xR = x + dx;
			AppendCorner( xL, yB, STRAIGHT, 0 );
			AppendCorner( xR, yB, STRAIGHT, 0 );
			AppendCorner( xR, yT, STRAIGHT, 0 );
			AppendCorner( xL, yT, STRAIGHT, 0 );
			Close( STRAIGHT ); 
			xL = x - dx;
			xR = x - spoke_w/2;
			yB = y + spoke_w/2;
			yT = y + dy;
			AppendCorner( xL, yB, STRAIGHT, 0 );
			AppendCorner( xR, yB, STRAIGHT, 0 );
			AppendCorner( xR, yT, STRAIGHT, 0 );
			AppendCorner( xL, yT, STRAIGHT, 0 );
			Close( STRAIGHT ); 
			xL = x + spoke_w/2;
			xR = x + dx;
			AppendCorner( xL, yB, STRAIGHT, 0 );
			AppendCorner( xR, yB, STRAIGHT, 0 );
			AppendCorner( xR, yT, STRAIGHT, 0 );
			AppendCorner( xL, yT, STRAIGHT, 0 );
			Close( STRAIGHT ); 
		}
	}
	return;
}

void CPolyLine::AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num )
{
	// get radius
	double r = sqrt( (double)(xi-xc)*(xi-xc) + (double)(yi-yc)*(yi-yc) );
	// get angles of start and finish
	double th_i = atan2( (double)yi-yc, (double)xi-xc );
	double th_f = atan2( (double)yf-yc, (double)xf-xc );
	double th_d = (th_f - th_i)/(num-1);
	double theta = th_i;
	// generate arc
	for( int ic=0; ic<num; ic++ )
	{
		int x = to_int(xc + r*cos(theta));
		int y = to_int(yc + r*sin(theta));
		AppendCorner( x, y, STRAIGHT, 0 );
		theta += th_d;
	}
	Close( STRAIGHT );
}


void CPolyLine::ClipGpcPolygon( gpc_op op, CPolyLine * clip_poly )
{
	gpc_polygon * result = new gpc_polygon;
	gpc_polygon_clip( op, m_gpc_poly, clip_poly->GetGpcPoly(), result );
	gpc_free_polygon( m_gpc_poly );
	delete m_gpc_poly;
	m_gpc_poly = result;
}

