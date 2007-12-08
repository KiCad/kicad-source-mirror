// file php_polygon_vertex.h
// See comments in file php_polygon_vertex.cpp

#ifndef PHP_POLYGON_VERTEX_H
#define PHP_POLYGON_VERTEX_H

#include "defs-macros.h"

class vertex;
class polygon;

class segment
{
public:
	segment(double xc=0.0, double yc=0.0, int d=0 );
	double Xc(){ return m_xc; };
	double Yc(){ return m_yc; };
	int d(){ return m_d; };
	void setXc( double xc ){ m_xc = xc; };
	void setYc( double yc ){ m_yc = yc; };

	double m_xc, m_yc;	// center of arc
	int m_d;			// direction (-1=CW, 0=LINE, 1=CCW)
};

class vertex
{
public:
	vertex( double x, double y, 
		double xc=0.0, double yc=0.0, double d=0.0,
		vertex * nextV=NULL, vertex * prevV=NULL, 
		polygon * nextPoly=NULL,
		BOOL intersect=FALSE, 
		vertex * neighbor=NULL,
		double alpha=0.0,
		BOOL entry=TRUE,
		BOOL checked=FALSE );
	~vertex();
	int id() { return m_id; };
	double X() { return m_x; };
	void setX( double x ) { m_x = x; };
	double Y() { return m_y; };
	void setY( double y ) { m_y = y; };
	double Xc ( BOOL g = TRUE );
	double Yc ( BOOL g = TRUE );
	double d ( BOOL g = TRUE );
	void setXc ( double xc ) { m_nSeg->setXc(xc); };
	void setYc ( double yc ) { m_nSeg->setYc(yc); };
	void setNext ( vertex* nextV ){ m_nextV = nextV; };
	vertex * Next (){ return m_nextV; };
	void setPrev ( vertex *prevV ){ m_prevV = prevV; };
	vertex * Prev (){ return m_prevV; };
	void setNseg ( segment * nSeg ){ m_nSeg = nSeg; };
	segment * Nseg (){ return m_nSeg; };
	void setPseg ( segment * pSeg ){ m_pSeg = pSeg; };
	segment * Pseg (){ return m_pSeg; };
	void setNextPoly ( polygon * nextPoly ){ m_nextPoly = nextPoly; };
	polygon * NextPoly (){ return m_nextPoly; };
	void setNeighbor ( vertex * neighbor ){ m_neighbor = neighbor; };
	vertex * Neighbor (){ return m_neighbor; };
	double Alpha (){ return m_alpha; };
	BOOL isIntersect (){ return m_intersect; };
	void setChecked( BOOL check = TRUE);
	BOOL isChecked () { return m_checked; };
	void setEntry ( BOOL entry = TRUE){ m_entry = entry; }
	BOOL isEntry (){ return m_entry; };

	double m_x, m_y;	// coords
	vertex * m_nextV;	// links to next and prev vertices
	vertex * m_prevV;	// links to next and prev vertices
	segment * m_nSeg, * m_pSeg;	// links to next and prev segments
	polygon * m_nextPoly;
	BOOL m_intersect;
	vertex * m_neighbor;
	double m_alpha;
	BOOL m_entry;
	BOOL m_checked;
	int m_id;
};

#endif	// ifndef PHP_POLYGON_VERTEX_H
