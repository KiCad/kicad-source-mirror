// file php_polygon.cpp
// This is a port of a php class written by Brenor Brophy (see below)

/*------------------------------------------------------------------------------
** File:		polygon.php
** Description:	PHP class for a polygon.
** Version:		1.1
** Author:		Brenor Brophy
** Email:		brenor at sbcglobal dot net
** Homepage:	www.brenorbrophy.com
**------------------------------------------------------------------------------
** COPYRIGHT (c) 2005 BRENOR BROPHY
**
** The source code included in this package is free software; you can
** redistribute it and/or modify it under the terms of the GNU General Public
** License as published by the Free Software Foundation. This license can be
** read at:
**
** http://www.opensource.org/licenses/gpl-license.php
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
**------------------------------------------------------------------------------
**
** Based on the paper "Efficient Clipping of Arbitary Polygons" by Gunther
** Greiner (greiner at informatik dot uni-erlangen dot de) and Kai Hormann
** (hormann at informatik dot tu-clausthal dot de), ACM Transactions on Graphics
** 1998;17(2):71-83.
**
** Available at: www.in.tu-clausthal.de/~hormann/papers/clipping.pdf
**
** Another useful site describing the algorithm and with some example
** C code by Ionel Daniel Stroe is at:
**
**		http://davis.wpi.edu/~matt/courses/clipping/
**
** The algorithm is extended by Brenor Brophy to allow polygons with
** arcs between vertices.
**
** Rev History
** -----------------------------------------------------------------------------
** 1.0	08/25/2005	Initial Release
** 1.1	09/04/2005	Added Move(), Rotate(), isPolyInside() and bRect() methods.
**                	Added software license language to header comments
*/
//#include "stdafx.h"

#include <stdio.h>

#include <math.h>

#include "fctsys.h"


#include "php_polygon_vertex.h"
#include "php_polygon.h"

const double PT = 0.99999;
//const double eps = (1.0 - PT)/10.0;
const double eps = 0.0; 

polygon::polygon( vertex * first )
{ 
	m_first = first; 
	m_cnt = 0; 
}

polygon::~polygon()
{
	while( m_cnt > 1 )
	{
		vertex * v = getFirst();
		del( v->m_nextV );
	}
	if( m_first )
	{
		delete m_first;
	}
}

vertex * polygon::getFirst()
{ 
	return m_first; 
}

polygon * polygon::NextPoly()
{ 
	return m_first->NextPoly(); 
}

/*
** Add a vertex object to the polygon (vertex is added at the "end" of the list)
** Which because polygons are closed lists means it is added just before the first
** vertex.
*/
void polygon::add( vertex * nv )
{
	if ( m_cnt == 0 )				// If this is the first vertex in the polygon
	{
		m_first = nv;				// Save a reference to it in the polygon
		m_first->setNext(nv);		// Set its pointer to point to itself
		m_first->setPrev(nv);		// because it is the only vertex in the list
		segment * ps = m_first->Nseg();	// Get ref to the Next segment object
		m_first->setPseg(ps);		// and save it as Prev segment as well
	}
	else							// At least one other vertex already exists
	{
		// p <-> nv <-> n
		//    ps     ns
		vertex * n = m_first;		// Get a ref to the first vertex in the list
		vertex * p = n->Prev();		// Get ref to previous vertex
		n->setPrev(nv);				// Add at end of list (just before first)
		nv->setNext(n);				// link the new vertex to it
		nv->setPrev(p);				// link to the pervious EOL vertex
		p->setNext(nv);				// And finally link the previous EOL vertex
		// Segments
		segment * ns = nv->Nseg();	// Get ref to the new next segment
		segment * ps = p->Nseg();	// Get ref to the previous segment
		n->setPseg(ns);				// Set new previous seg for m_first
		nv->setPseg(ps);			// Set previous seg of the new vertex
	}
	m_cnt++;						// Increment the count of vertices
}

/*
** Create a vertex and then add it to the polygon
*/
void polygon::addv ( double x, double y, 
					double xc, double yc, int d )
{
	vertex * nv = new vertex( x, y, xc, yc, d );
	add( nv );
}

/*
** Delete a vertex object from the polygon. This is not used by the main algorithm
** but instead is used to clean-up a polygon so that a second boolean operation can
** be performed.
*/
vertex * polygon::del( vertex * v )
{
	// p <-> v <-> n				   Will delete v and ns
	//    ps    ns
	vertex * p = v->Prev();				// Get ref to previous vertex
	vertex * n = v->Next();				// Get ref to next vertex
	p->setNext(n);				// Link previous forward to next
	n->setPrev(p);				// Link next back to previous
	// Segments
	segment * ps = p->Nseg();				// Get ref to previous segment
	segment * ns = v->Nseg();				// Get ref to next segment
	n->setPseg(ps);				// Link next back to previous segment
	delete ns;	//AMW
	v->m_nSeg = NULL; // AMW
	delete v;	//AMW
//	ns = NULL; 
//	v = NULL;			// Free the memory
	m_cnt--;			// One less vertex
	return n;			// Return a ref to the next valid vertex
}

/*
** Reset Polygon - Deletes all intersection vertices. This is used to
** restore a polygon that has been processed by the boolean method
** so that it can be processed again.
*/
void polygon::res()
{
	vertex * v = getFirst();		// Get the first vertex
	do
	{
		v = v->Next();		// Get the next vertex in the polygon
		while (v->isIntersect())	// Delete all intersection vertices
			v = del(v);
	}
	while (v->id() != m_first->id());
}

/*
** Copy Polygon - Returns a reference to a new copy of the poly object
** including all its vertices & their segments
*/
polygon * polygon::copy_poly()
{
	polygon * n = new polygon;		// Create a new instance of this class
	vertex * v = getFirst();
	do
	{
		n->addv(v->X(),v->Y(),v->Xc(),v->Yc(),v->d());
		v = v->Next();
	}
	while (v->id() != m_first->id());
	return n;
}

/*
** Insert and Sort a vertex between a specified pair of vertices (start and end)
**
** This function inserts a vertex (most likely an intersection point) between two
** other vertices. These other vertices cannot be intersections (that is they must
** be actual vertices of the original polygon). If there are multiple intersection
** points between the two vertices then the new vertex is inserted based on its
** alpha value.
*/
void polygon::insertSort( vertex * nv, vertex * s, vertex * e )
{
	vertex * c = s;				// Set current to the starting vertex
	// Move current past any intersections
	// whose alpha is lower but don't go past
	// the end vertex
	while( c->id() != e->id() && c->Alpha() < nv->Alpha() )
		c = c->Next();				
	// p <-> nv <-> c
	nv->setNext(c);				// Link new vertex forward to curent one
	vertex * p = c->Prev();		// Get a link to the previous vertex
	nv->setPrev(p);				// Link the new vertex back to the previous one
	p->setNext(nv);				// Link previous vertex forward to new vertex
	c->setPrev(nv);				// Link current vertex back to the new vertex
	// Segments
	segment * ps = p->Nseg();
	nv->setPseg(ps);
	segment * ns = nv->Nseg();
	c->setPseg(ns);
	m_cnt++;					// Just added a new vertex
}

/*
** return the next non intersecting vertex after the one specified
*/
vertex * polygon::nxt( vertex * v )
{
	vertex * c = v;						// Initialize current vertex
	while (c && c->isIntersect())	// Move until a non-intersection
		c = c->Next();				// vertex if found
	return c;						// return that vertex
}

/*
** Check if any unchecked intersections remain in the polygon. The boolean
** method is complete when all intersections have been checked.
*/
BOOL polygon::unckd_remain()
{
	BOOL remain = FALSE;
	vertex * v = m_first;
	do
	{
		if (v->isIntersect() && !v->isChecked())
			remain = TRUE;		// Set if an unchecked intersection is found
		v = v->Next();
	}
	while (v->id() != m_first->id());
	return remain;
}

/*
** Return a ref to the first unchecked intersection point in the polygon.
** If none are found then just the first vertex is returned.
*/
vertex * polygon::first_unckd_intersect()
{
	vertex * v = m_first;
	do									// Do-While
	{									// Not yet reached end of the polygon
		v = v->Next();			// AND the vertex if NOT an intersection
	}									// OR it IS an intersection, but has been checked already
	while(v->id() != m_first->id() && ( !v->isIntersect() || ( v->isIntersect() && v->isChecked() ) ) );
	return v;
}
/*
** Return the distance between two points
*/
double polygon::dist( double x1, double y1, double x2, double y2 )
{
	return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}
/*
** Calculate the angle between 2 points, where Xc,Yc is the center of a circle
** and x,y is a point on its circumference. All angles are relative to
** the 3 O'Clock position. Result returned in radians
*/
double polygon::angle( double xc, double yc, double x1, double y1 )
{
	double d = dist(xc, yc, x1, y1); // calc distance between two points
	double a1;
	if ( asin( (y1-yc)/d ) >= 0 )
		a1 = acos( (x1-xc)/d );
	else
		a1 = 2*PI - acos( (x1-xc)/d );
	return a1;
}

/*
** Return Alpha value for an Arc
**
** X1/Y1 & X2/Y2 are the end points of the arc, Xc/Yc is the center & Xi/Yi
** the intersection point on the arc. d is the direction of the arc
*/
double polygon::aAlpha( double x1, double y1, double x2, double y2, 
			   double xc, double yc, double xi, double yi, double d )
{
	double sa = angle(xc, yc, x1, y1); // Start Angle
	double ea = angle(xc, yc, x2, y2); // End Angle
	double ia = angle(xc, yc, xi, yi); // Intersection Angle
	double arc, aint;
	if (d == 1)	// Anti-Clockwise
	{
		arc = ea - sa;
		aint = ia - sa;
	}
	else			// Clockwise
	{
		arc = sa - ea;
		aint = sa - ia;
	}
	if (arc < 0) 
		arc += 2*PI;
	if (aint < 0) 
		aint += 2*PI;
	double a = aint/arc;
	return  a;
}

/*
** This function handles the degenerate case where a vertex of one
** polygon lies directly on an edge of the other. This case can
** also occur during the isInside() function, where the search
** line exactly intersects with a vertex. The function works
** by shortening the line by a tiny amount.
*/
void polygon::perturb( vertex * p1, vertex * p2, vertex * q1, vertex * q2, 
						   double aP, double aQ )
{
//	if (aP == 0)		// Move vertex p1 closer to p2
	if( abs(aP) <= eps )		// Move vertex p1 closer to p2
	{
		p1->setX(p1->X() + (1-PT) * (p2->X() - p1->X()));
		p1->setY(p1->Y() + (1-PT) * (p2->Y() - p1->Y()));
	}
//	else if (aP == 1)	// Move vertex p2 closer to p1
	else if( abs(1-aP) <= eps )	// Move vertex p2 closer to p1
	{
		p2->setX(p1->X() + PT * (p2->X() - p1->X()));
		p2->setY(p1->Y() + PT * (p2->Y() - p1->Y()));
	}
//**	else if (aQ == 0)	// Move vertex q1 closer to q2
	if( abs(aQ) <= eps )	// Move vertex q1 closer to q2
	{
		q1->setX(q1->X() + (1-PT) * (q2->X() - q1->X()));
		q1->setY(q1->Y() + (1-PT) * (q2->Y() - q1->Y()));
	}
//**	else if (aQ == 1)	// Move vertex q2 closer to q1
	else if( abs(1-aQ) <= eps )	// Move vertex q2 closer to q1
	{
		q2->setX(q1->X() + PT * (q2->X() - q1->X()));
		q2->setY(q1->Y() + PT * (q2->Y() - q1->Y()));
	}
}

/*
** Determine the intersection between two pairs of vertices p1/p2, q1/q2
**
** Either or both of the segments passed to this function could be arcs.
** Thus we must first determine if the intersection is line/line, arc/line
** or arc/arc. Then apply the correct math to calculate the intersection(s).
**
** Line/Line can have 0 (no intersection) or 1 intersection
** Line/Arc and Arc/Arc can have 0, 1 or 2 intersections
**
** The function returns TRUE is any intersections are found
** The number found is returned in n
** The arrays ix[], iy[], alphaP[] & alphaQ[] return the intersection points
** and their associated alpha values.
*/
BOOL polygon::ints( vertex * p1, vertex * p2, vertex * q1, vertex * q2, 
					int * n, double ix[], double iy[], double alphaP[], double alphaQ[] )
{
	BOOL found = FALSE; 
	*n = 0;			// No intersections found yet
	int pt = p1->d();	
	int qt = q1->d();	// Do we have Arcs or Lines?

	if (pt == 0 && qt == 0) // Is it line/Line ?
	{
		/* LINE/LINE
		** Algorithm from: http://astronomy.swin.edu.au/~pbourke/geometry/lineline2d/
		*/
		double x1 = p1->X(); 
		double y1 = p1->Y();
		double x2 = p2->X(); 
		double y2 = p2->Y();
		double x3 = q1->X(); 
		double y3 = q1->Y();
		double x4 = q2->X(); 
		double y4 = q2->Y();
		double d = ((y4-y3)*(x2-x1)-(x4-x3)*(y2-y1));
		if (d != 0)
		{ // The lines intersect at a point somewhere
			double ua = ((x4-x3)*(y1-y3)-(y4-y3)*(x1-x3))/d;
			double ub = ((x2-x1)*(y1-y3)-(y2-y1)*(x1-x3))/d;
			TRACE( "    ints: ua = %.17f, ub = %.17f\n", ua, ub );
			// The values of $ua and $ub tell us where the intersection occurred.
			// A value between 0 and 1 means the intersection occurred within the 
			// line segment.
			// A value less than 0 or greater than 1 means the intersection occurred
			// outside the line segment
			// A value of exactly 0 or 1 means the intersection occurred right at the 
			// start or end of the line segment. For our purposes we will consider this
			// NOT to be an intersection and we will move the vertex a tiny distance
			// away from the intersecting line.
//			if( ua == 0 || ua == 1 || ub == 0 || ub == 1 )
			if( abs(ua)<=eps || abs(1.0-ua)<=eps || abs(ub)<=eps || abs(1.0-ub)<=eps )
			{
				// Degenerate case - vertex touches a line
				perturb(p1, p2, q1, q2, ua, ub);
				//** for testing, see if we have successfully resolved the degeneracy
				{
					double tx1 = p1->X(); 
					double ty1 = p1->Y();
					double tx2 = p2->X(); 
					double ty2 = p2->Y();
					double tx3 = q1->X(); 
					double ty3 = q1->Y();
					double tx4 = q2->X(); 
					double ty4 = q2->Y();
					double td = ((ty4-ty3)*(tx2-tx1)-(tx4-tx3)*(ty2-ty1));
					if (td != 0)
					{ 
						// The lines intersect at a point somewhere
						double tua = ((tx4-tx3)*(ty1-ty3)-(ty4-ty3)*(tx1-tx3))/td;
						double tub = ((tx2-tx1)*(ty1-ty3)-(ty2-ty1)*(tx1-tx3))/td;
						if( abs(tua)<=eps || abs(1.0-tua)<=eps || abs(tub)<=eps || abs(1.0-tub)<=eps )
							wxASSERT(0);
						else if( (tua > 0 && tua < 1) && (tub > 0 && tub < 1) )
							wxASSERT(0);
						TRACE( "      perturb:\n      new s = (%f,%f) to (%f,%f)\n      new c = (%f,%f) to (%f,%f)\n      new ua = %.17f, ub = %.17f\n",
							tx1, ty1, tx2, ty2, tx3, ty3, tx4, ty4, tua, tub );
					}
				}
				//** end test
				found = FALSE;
			}
			else if ((ua > 0 && ua < 1) && (ub > 0 && ub < 1))
			{ 
				// Intersection occurs on both line segments
				double x = x1 + ua*(x2-x1);
				double y = y1 + ua*(y2-y1);
				iy[0] = y; 
				ix[0] = x;
				alphaP[0] = ua; 
				alphaQ[0] = ub;
				*n = 1; 
				found = TRUE;
			}
			else
			{
				// The lines do not intersect
				found = FALSE;
			}
		}
		else
		{ 
			// The lines do not intersect (they are parallel)
			found = FALSE;
		}
	}	// End of find Line/Line intersection
	else if (pt != 0 && qt != 0)	// Is  it Arc/Arc?
	{
		/* ARC/ARC
		** Algorithm from: http://astronomy.swin.edu.au/~pbourke/geometry/2circle/
		*/
		double x0 = p1->Xc(); 
		double y0 = p1->Yc(); // Center of first Arc
		double r0 = dist(x0,y0,p1->X(),p1->Y());	// Calc the radius
		double x1 = q1->Xc(); 
		double y1 = q1->Yc(); // Center of second Arc
		double r1 = dist(x1,y1,q1->X(),q1->Y());	// Calc the radius

		double dx = x1 - x0;	// dx and dy are the vertical and horizontal
		double dy = y1 - y0;	// distances between the circle centers.
		double d = sqrt((dy*dy) + (dx*dx)); // Distance between the centers.

		if(d > (r0 + r1))		// Check for solvability.
		{							// no solution. circles do not intersect.
			found = FALSE;
		}
		else if(d < abs(r0 - r1) )
		{							// no solution. one circle inside the other
			found = FALSE;
		}
		else
		{
			/*
			** 'xy2' is the point where the line through the circle intersection
			** points crosses the line between the circle centers.
			*/
			double a = ((r0*r0)-(r1*r1)+(d*d))/(2.0*d); // Calc the distance from xy0 to xy2.
			double x2 = x0 + (dx * a/d);		// Determine the coordinates of xy2.
			double y2 = y0 + (dy * a/d);
			if (d == (r0 + r1)) // Arcs touch at xy2 exactly (unlikely)
			{
				alphaP[0] = aAlpha(p1->X(), p1->Y(), p2->X(), p2->Y(), x0, y0, x2, y2, pt);
				alphaQ[0] = aAlpha(q1->X(), q1->Y(), q2->X(), q2->Y(), x1, y1, x2, y2, qt);
				if ((alphaP[0] >0 && alphaP[0] < 1) && (alphaQ[0] >0 && alphaQ[0] < 1))
				{
					ix[0] = x2;
					iy[0] = y2;
					*n = 1; found = TRUE;
				}
			}
			else					// Arcs intersect at two points
			{
				double alP[2], alQ[2];
				double h = sqrt((r0*r0) - (a*a)); // Calc the distance from xy2 to either
				// of the intersection points.
				double rx = -dy * (h/d);	// Now determine the offsets of the
				double ry =  dx * (h/d);
				// intersection points from xy2
				double x[2], y[2];
				x[0] = x2 + rx; x[1] = x2 - rx;		// Calc the absolute intersection points.
				y[0] = y2 + ry; y[1] = y2 - ry;
				alP[0] = aAlpha(p1->X(), p1->Y(), p2->X(), p2->Y(), x0, y0, x[0], y[0], pt);
				alQ[0] = aAlpha(q1->X(), q1->Y(), q2->X(), q2->Y(), x1, y1, x[0], y[0], qt);
				alP[1] = aAlpha(p1->X(), p1->Y(), p2->X(), p2->Y(), x0, y0, x[1], y[1], pt);
				alQ[1] = aAlpha(q1->X(), q1->Y(), q2->X(), q2->Y(), x1, y1, x[1], y[1], qt);
				for (int i=0; i<=1; i++)
					if ((alP[i] >0 && alP[i] < 1) && (alQ[i] >0 && alQ[i] < 1))
					{
						ix[*n] = x[i]; 
						iy[*n] = y[i];
						alphaP[*n] = alP[i]; 
						alphaQ[*n] = alQ[i];
						*n++; 
						found = TRUE;
					}
			}
		}
	}	// End of find Arc/Arc intersection
	else	// It must be Arc/Line
	{
		/* ARC/LINE
		** Algorithm from: http://astronomy.swin.edu.au/~pbourke/geometry/sphereline/
		*/
		double d, x1, x2, xc, xs, xe;
		double y1, y2, yc, ys, ye;
		if (pt == 0)	// Segment p1,p2 is the line
		{				// Segment q1,q2 is the arc
			x1 = p1->X(); 
			y1 = p1->Y();
			x2 = p2->X(); 
			y2 = p2->Y();
			xc = q1->Xc(); 
			yc = q1->Yc();
			xs = q1->X(); 
			ys = q1->Y();
			xe = q2->X(); 
			ye = q2->Y();
			d = qt;
		}
		else			// Segment q1,q2 is the line
		{				// Segment p1,p2 is the arc
			x1 = q1->X(); y1 = q1->Y();
			x2 = q2->X(); y2 = q2->Y();
			xc = p1->Xc(); yc = p1->Yc();
			xs = p1->X(); ys = p1->Y();
			xe = p2->X(); ye = p2->Y();
			d = pt;
		}
		double r = dist(xc,yc,xs,ys);
		double a = pow((x2 - x1),2)+pow((y2 - y1),2);
		double b = 2* ( (x2 - x1)*(x1 - xc)
			+ (y2 - y1)*(y1 - yc) );
		double c = pow(xc,2) + pow(yc,2) +
			pow(x1,2) + pow(y1,2) -
			2* ( xc*x1 + yc*y1) - pow(r,2);
		double i =   b * b - 4 * a * c;
		if ( i < 0.0 ) // no intersection
		{
			found = FALSE;
		}
		else if ( i == 0.0 )	// one intersection
		{
			double mu = -b/(2*a);
			double x = x1 + mu*(x2-x1);
			double y = y1 + mu*(y2-y1);
			double al = mu; // Line Alpha
			double aa = this->aAlpha(xs, ys, xe, ye, xc, yc, x, y, d); // Arc Alpha
			if ((al >0 && al <1)&&(aa >0 && aa <1))
			{
				ix[0] = x; iy[0] = y;
				*n = 1; 
				found = TRUE;
				if (pt == 0)
				{
					alphaP[0] = al; alphaQ[0] = aa;
				}
				else
				{
					alphaP[0] = aa; alphaQ[0] = al;
				}
			}
		}
		else if ( i > 0.0 ) 	// two intersections
		{
			double mu[2], x[2], y[2], al[2], aa[2];
			mu[0] = (-b + sqrt( pow(b,2) - 4*a*c )) / (2*a);	// first intersection
			x[0] = x1 + mu[0]*(x2-x1);
			y[0] = y1 + mu[0]*(y2-y1);
			mu[1] = (-b - sqrt(pow(b,2) - 4*a*c )) / (2*a); // second intersection
			x[1] = x1 + mu[1]*(x2-x1);
			y[1] = y1 + mu[1]*(y2-y1);
			al[0] = mu[0];
			aa[0] = aAlpha(xs, ys, xe, ye, xc, yc, x[0], y[0], d);
			al[1] = mu[1];
			aa[1] = aAlpha(xs, ys, xe, ye, xc, yc, x[1], y[1], d);
			for (int i=0; i<=1; i++)
				if ((al[i] >0 && al[i] < 1) && (aa[i] >0 && aa[i] < 1))
				{
					ix[*n] = x[i]; 
					iy[*n] = y[i];
					if (pt == 0)
					{
						alphaP[*n] = al[i]; 
						alphaQ[*n] = aa[i];
					}
					else
					{
						alphaP[*n] = aa[i]; 
						alphaQ[*n] = al[i];
					}
					*n++; 
					found = TRUE;
				}
		}
	}	// End of find Arc/Line intersection
	return found;
} // end of intersect function

/*
** Test if a vertex lies inside the polygon
**
** This function calculates the "winding" number for the point. This number
** represents the number of times a ray emitted from the point to infinity
** intersects any edge of the polygon. An even winding number means the point
** lies OUTSIDE the polygon, an odd number means it lies INSIDE it.
**
** Right now infinity is set to -10000000, some people might argue that infinity
** actually is a bit bigger. Those people have no lives.
**
** Allan Wright 4/16/2006: I guess I have no life: I had to increase it to -1000000000
*/
BOOL polygon::isInside( vertex * v )
{
	//** modified for testing
	if( v->isIntersect() )
		wxASSERT(0);
	int winding_number = 0;
	int winding_number2 = 0;
	int winding_number3 = 0;
	int winding_number4 = 0;
//**	vertex * point_at_infinity = new vertex(-10000000,v->Y());	// Create point at infinity
/*	vertex * point_at_infinity = new vertex(-1000000000,-50000000);	// Create point at infinity
	vertex * point_at_infinity2 = new vertex(1000000000,+50000000);	// Create point at infinity
	vertex * point_at_infinity3 = new vertex(500000000,1000000000);	// Create point at infinity
	vertex * point_at_infinity4 = new vertex(-500000000,1000000000);	// Create point at infinity
*/
	vertex point_at_infinity(-1000000000,-50000000);	// Create point at infinity
	vertex point_at_infinity2(1000000000,+50000000);	// Create point at infinity
	vertex point_at_infinity3(500000000,1000000000);	// Create point at infinity
	vertex point_at_infinity4(-500000000,1000000000);	// Create point at infinity
	vertex * q = m_first;		// End vertex of a line segment in polygon 
	do
	{
		if (!q->isIntersect())
		{
			int n;
			double x[2], y[2], aP[2], aQ[2];
			if( ints( &point_at_infinity, v, q, nxt(q->Next()), &n, x, y, aP, aQ ) )
				winding_number += n;		// Add number of intersections found
			if( ints( &point_at_infinity2, v, q, nxt(q->Next()), &n, x, y, aP, aQ ) )
				winding_number2 += n;		// Add number of intersections found
			if( ints( &point_at_infinity3, v, q, nxt(q->Next()), &n, x, y, aP, aQ ) )
				winding_number3 += n;		// Add number of intersections found
			if( ints( &point_at_infinity4, v, q, nxt(q->Next()), &n, x, y, aP, aQ ) )
				winding_number4 += n;		// Add number of intersections found
		}
		q = q->Next();
	}
	while( q->id() != m_first->id() );
//	delete point_at_infinity;		
//	delete point_at_infinity2;
	if( winding_number%2 != winding_number2%2 
	|| winding_number3%2 != winding_number4%2 
	|| winding_number%2 != winding_number3%2 )
		wxASSERT(0);
	if( winding_number%2 == 0 )	// Check even or odd
		return FALSE;				// even == outside
	else
		return TRUE;				// odd == inside
}

/*
**	Execute a Boolean operation on a polygon
**
** This is the key method. It allows you to AND/OR this polygon with another one
** (equvalent to a UNION or INTERSECT operation. You may also subtract one from
** the other (same as DIFFERENCE). Given two polygons A, B the following operations
** may be performed:
**
** A|B ... A OR  B (Union of A and B)
** A&B ... A AND B (Intersection of A and B)
** A\B ... A - B
** B\A ... B - A
**
** A is the object and B is the polygon passed to the method.
*/
polygon * polygon::boolean( polygon * polyB, int oper )
{
	polygon * last = NULL; 

	vertex * s = m_first;			// First vertex of the subject polygon
	vertex * c = polyB->getFirst();	// First vertex of the "clip" polygon
	/*
	** Phase 1 of the algoritm is to find all intersection points between the two
	** polygons. A new vertex is created for each intersection and it is added to
	** the linked lists for both polygons. The "neighbor" reference in each vertex
	** stores the link between the same intersection point in each polygon.
	*/
	TRACE( "boolean...phase 1\n" );
	do
	{
		TRACE( "s=(%f,%f) to (%f,%f) I=%d\n", 
			s->m_x, s->m_y, s->m_nextV->m_x, s->m_nextV->m_y, s->m_intersect );
		if (!s->isIntersect())
		{
			do
			{
				TRACE( "  c=(%f,%f) to (%f,%f) I=%d\n", 
					c->m_x, c->m_y, c->m_nextV->m_x, c->m_nextV->m_y, c->m_intersect );
				if (!c->isIntersect())
				{
					int n;
					double ix[2], iy[2], alphaS[2], alphaC[2];
					BOOL bInt = ints(s, nxt(s->Next()),c, polyB->nxt(c->Next()), &n, ix, iy, alphaS, alphaC);
					if( bInt )
					{
						TRACE( "      int at (%f,%f) aS = %.17f, aC = %.17f\n", ix[0], iy[0], alphaS[0], alphaC[0] );
						for (int i=0; i<n; i++)
						{
							vertex * is = new vertex(ix[i], iy[i], s->Xc(), s->Yc(), s->d(), NULL, NULL, NULL, TRUE, NULL, alphaS[i], FALSE, FALSE);
							vertex * ic = new vertex(ix[i], iy[i], c->Xc(), c->Yc(), c->d(), NULL, NULL, NULL, TRUE, NULL, alphaC[i], FALSE, FALSE);
							is->setNeighbor(ic);
							ic->setNeighbor(is);
							insertSort(is, s, this->nxt(s->Next()));
							polyB->insertSort(ic, c, polyB->nxt(c->Next()));
						}
					}
				} // end if c is not an intersect point
				c = c->Next();
			}
			while (c->id() != polyB->m_first->id());
		} // end if s not an intersect point
		s = s->Next();
	}
	while(s->id() != m_first->id());

	//** for testing...check number of intersections in each poly
	TRACE( "boolean...phase 1 testing\n" );
	int n_ints = 0;
	s = m_first;
	do
	{
		if( s->isIntersect() )
			n_ints++;
		s = s->Next();
	} while( s->id() != m_first->id() );
	int n_polyB_ints = 0;
	s = polyB->m_first;
	do
	{
		if( s->isIntersect() )
			n_polyB_ints++;
		s = s->Next();
	} while( s->id() != polyB->m_first->id() );
	if( n_ints != n_polyB_ints )
		wxASSERT(0);
	if( n_ints%2 != 0 )
		wxASSERT(0);
	//** end test
	
	/*
	** Phase 2 of the algorithm is to identify every intersection point as an
	** entry or exit point to the other polygon. This will set the entry bits
	** in each vertex object.
	**
	** What is really stored in the entry record for each intersection is the
	** direction the algorithm should take when it arrives at that entry point.
	** Depending in the operation requested (A&B, A|B, A/B, B/A) the direction is
	** set as follows for entry points (f=foreward, b=Back), exit points are always set
	** to the opposite:
	**       Enter       Exit
	**       A    B     A    B
	** A|B   b    b     f    f
	** A&B   f    f     b    b
	** A\B   b    f     f    b
	** B\A   f    b     b    f
	**
	** f = TRUE, b = FALSE when stored in the entry record
	*/
	BOOL A, B;

	switch (oper)
	{
	case A_OR_B:	A = FALSE;	B = FALSE;	break;
	case A_AND_B:	A = TRUE;	B = TRUE;	break;
	case A_MINUS_B:	A = FALSE;	B = TRUE;	break;
	case B_MINUS_A:	A = TRUE;	B = FALSE;	break;
	default:	A = TRUE;	B = TRUE;	break;
	}
	s = m_first;
	//** testing
	if( s->isIntersect() )
		wxASSERT(0);
	//** end test
	BOOL entry;
	if (polyB->isInside(s)) // if we are already inside
		entry = !A;			// next intersection must be an exit
	else					// otherwise
		entry = A;			// next intersection must be an entry
	do
	{
		if (s->isIntersect())
		{
			s->setEntry(entry);
			entry = !entry;
		}
		s = s->Next();
	}
	while (s->id() != m_first->id());
	/*
	** Repeat for other polygon
	*/
	c = polyB->m_first;
	if (this->isInside(c)) // if we are already inside
		entry = !B;	// next intersection must be an exit
	else				// otherwise
		entry = B;	// next intersection must be an entry
	do
	{
		if (c->isIntersect())
		{
			c->setEntry(entry);
			entry = !entry;
		}
		c = c->Next();
	}
	while (c->id() != polyB->m_first->id());
	/*
	** Phase 3 of the algorithm is to scan the linked lists of the
	** two input polygons an construct a linked list of result
	** polygons. We start at the first intersection then depending
	** on whether it is an entry or exit point we continue building
	** our result polygon by following the source or clip polygon
	** either forwards or backwards.
	*/
	while (this->unckd_remain())				// Loop while unchecked intersections remain
	{
		vertex * v = first_unckd_intersect();	// Get the first unchecked intersect point
		polygon * r = new polygon;		// Create a new instance of that class
		do
		{
			v->setChecked();					// Set checked flag true for this intersection
			if (v->isEntry())
			{
				do
				{
					v = v->Next();
					vertex * nv = new vertex(v->X(),v->Y(),v->Xc(),v->Yc(),v->d());
					r->add(nv);
				}
				while (!v->isIntersect());
			}
			else
			{
				do
				{
					v = v->Prev();
					vertex * nv = new vertex(v->X(),v->Y(),v->Xc(FALSE),v->Yc(FALSE),v->d(FALSE));
					r->add(nv);
				}
				while (!v->isIntersect());
			}
			v = v->Neighbor();
		}
		while (!v->isChecked()); // until polygon closed
		if (last)							// Check in case first time thru the loop
			r->m_first->setNextPoly(last);	// Save ref to the last poly in the first vertex
		// of this poly
		last = r;						// Save this polygon
	} // end of while there is another intersection to check
	/*
	** Clean up the input polygons by deleting the intersection points
	*/
	res();
	polyB->res();
	/*
	** It is possible that no intersection between the polygons was found and
	** there is no result to return. In this case we make function fail
	** gracefully as follows (depending on the requested operation):
	**
	** A|B : Return this with polyB in m_first->nextPoly
	** A&B : Return this
	** A\B : Return this
	** B\A : return polyB
	*/
	polygon * p;
	if (!last)
	{
		switch (oper)
		{
		case A_OR_B:	
			last = copy_poly();
			p = polyB->copy_poly();
			last->m_first->setNextPoly(p);
			break;
		case A_AND_B:	
			last = copy_poly();	
			break;
		case A_MINUS_B:	
			last = copy_poly();	
			break;
		case B_MINUS_A:	
			last = polyB->copy_poly();	
			break;
		default:	
			last = copy_poly();	
			break;
		}
	}
	else if (m_first->m_nextPoly)
	{
		last->m_first->m_nextPoly = m_first->NextPoly();
	}
	return last;
} // end of boolean function

/*
** Test if a polygon lies entirly inside this polygon
**
** First every point in the polygon is tested to determine if it is
** inside this polygon. If all points are inside, then the second
** test is performed that looks for any intersections between the
** two polygons. If no intersections are found then the polygon
** must be completely enclosed by this polygon.
*/

#if 0
function polygon::isPolyInside (p)
{
	inside = TRUE;
	c = p->getFirst();	// Get the first vertex in polygon p
	do
	{
		if (!this->isInside(c))	// If vertex is NOT inside this polygon
			inside = FALSE;		// then set flag to false
		c = c->Next();			// Get the next vertex in polygon p
	}
	while (c->id() != p->first->id());
	if (inside)
	{
		c = p->getFirst();		// Get the first vertex in polygon p
		s = getFirst();	// Get the first vertex in this polygon
		do
		{
			do
			{
				if (this->ints(s, s->Next(),c, c->Next(), n, x, y, aS, aC))
					inside = FALSE;
				c = c->Next();
			}
			while (c->id() != p->first->id());
			s = s->Next();
		}
		while (s->id() != m_first->id());
	}
	return inside;
} // end of isPolyInside

/*
** Move Polygon
**
** Translates polygon by delta X and delta Y
*/
function polygon::move (dx, dy)
{
	v = getFirst();
	do
	{
		v->setX(v->X() + dx);
		v->setY(v->Y() + dy);
		if (v->d() != 0)
		{
			v->setXc(v->Xc() + dx);
			v->setYc(v->Yc() + dy);
		}
		v = v->Next();
	}
	while(v->id() != m_first->id());
} // end of move polygon

/*
** Rotate Polygon
**
** Rotates a polgon about point xr/yr by a radians
*/
function polygon::rotate (xr, yr, a)
{
	this->move(-xr,-yr);		// Move the polygon so that the point of
	// rotation is at the origin (0,0)
	if (a < 0)					// We might be passed a negitive angle
		a += 2*pi();			// make it positive
	v = m_first;
	do
	{
		x=v->X(); y=v->Y();
		v->setX(x*cos(a) - y*sin(a));	// x' = xCos(a)-ySin(a)
		v->setY(x*sin(a) + y*cos(a));	// y' = xSin(a)+yCos(a)
		if (v->d() != 0)
		{
			x=v->Xc(); y=v->Yc();
			v->setXc(x*cos(a) - y*sin(a));
			v->setYc(x*sin(a) + y*cos(a));
		}
		v = v->Next();
	}
	while(v->id() != m_first->id());
	this->move(xr,yr);		// Move the rotated polygon back
} // end of rotate polygon

/*
** Return Bounding Rectangle for a Polygon
**
** returns a polygon object that represents the bounding rectangle
** for this polygon. Arc segments are correctly handled.
*/
function polygon::&bRect ()
{
	minX = INF; minY = INF; maxX = -INF; maxY = -INF;
	v = m_first;
	do
	{
		if (v->d() != 0)	// Is it an arc segment
		{
			vn = v->Next();					// end vertex of the arc segment
			v1 = new vertex(v->Xc(), -infinity);	// bottom point of vertical line thru arc center
			v2 = new vertex(v->Xc(), +infinity);	// top point of vertical line thru arc center
			if (this->ints(v, vn, v1, v2, n, x, y, aS, aC))	// Does line intersect the arc ?
			{
				for (i=0; i<n; i++)			// check y portion of all intersections
				{
					minY = min(minY, y[i], v->Y());
					maxY = max(maxY, y[i], v->Y());
				}
			}
			else	// There was no intersection so bounding rect is determined
			{		// by the start point only, not teh edge of the arc
				minY = min(minY, v->Y());
				maxY = max(maxY, v->Y());
			}
			v1 = NULL; v2 = NULL;	// Free the memory used
			h1 = new vertex(-infinity, v->Yc());	// left point of horozontal line thru arc center
			h2 = new vertex(+infinity, v->Yc());	// right point of horozontal line thru arc center
			if (this->ints(v, vn, h1, h2, n, x, y, aS, aC))	// Does line intersect the arc ?
			{
				for (i=0; i<n; i++)			// check x portion of all intersections
				{
					minX = min(minX, x[i], v->X());
					maxX = max(maxX, x[i], v->X());
				}
			}
			else
			{
				minX = min(minX, v->X());
				maxX = max(maxX, v->X());
			}
			h1 = NULL; h2 = NULL;
		}
		else	// Straight segment so just check the vertex
		{
			minX = min(minX, v->X());
			minY = min(minY, v->Y());
			maxX = max(maxX, v->X());
			maxY = max(maxY, v->Y());
		}
		v = v->Next();
	}
	while(v->id() != m_first->id());
	//
	// Now create an return a polygon with the bounding rectangle
	//
	this_class = get_class(this);			// Findout the class I'm in (might be an extension of polygon)
	p = new this_class;					// Create a new instance of that class
	p->addv(minX,minY);
	p->addv(minX,maxY);
	p->addv(maxX,maxY);
	p->addv(maxX,minY);
	return p;
} // end of bounding rectangle
#endif
