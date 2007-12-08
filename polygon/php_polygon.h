// file php_polygon.h
// See comments in php_polygon.cpp

#ifndef PHP_POLYGON_H
#define PHP_POLYGON_H

class vertex;
class segment;

#define infinity 100000000	// for places that are far far away
#define PI 3.14159265359

enum{ A_OR_B, A_AND_B, A_MINUS_B, B_MINUS_A };

class polygon
{
public:
/*------------------------------------------------------------------------------
** This class manages a doubly linked list of vertex objects that represents
** a polygon. The class consists of basic methods to manage the list
** and methods to implement boolean operations between polygon objects.
*/

	vertex * m_first;	// Reference to first vertex in the linked list
	int m_cnt;			// Tracks number of vertices in the polygon

	polygon( vertex * first = NULL );
	~polygon();
	vertex * getFirst();
	polygon * NextPoly();

	void add( vertex * nv );
	void addv( double x, double y, 
		double xc=0, double yc=0, int d=0);
	vertex * del( vertex * v );
	void res();
	polygon * copy_poly();
	void insertSort( vertex * nv, vertex * s, vertex * e );
	vertex * nxt( vertex * v );
	BOOL unckd_remain();
	vertex * first_unckd_intersect();
	double dist( double x1, double y1, double x2, double y2 );
	double angle( double xc, double yc, double x1, double y1 );
	double aAlpha( double x1, double y1, double x2, double y2, 
		double xc, double yc, double xi, double yi, double d );
	void perturb( vertex * p1, vertex * p2, vertex * q1, vertex * q2, 
		double aP, double aQ );
	BOOL ints( vertex * p1, vertex * p2, vertex * q1, vertex * q2, 
		int * n, double ix[], double iy[], double alphaP[], double alphaQ[] );
	BOOL isInside ( vertex * v );
	polygon * boolean( polygon * polyB, int oper );
#if 0
	function isPolyInside (p);
	function move (dx, dy);
	function rotate (xr, yr, a);
	function &bRect ();
#endif
}; //end of class polygon


#endif		// ifndef PHP_POLYGON_H
