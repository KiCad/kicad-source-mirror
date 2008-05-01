// file php_polygon_vertex.cpp
// This is a port of a php class written by Brenor Brophy (see below)

/*------------------------------------------------------------------------------
** File:		vertex.php
** Description:	PHP class for a polygon vertex. Used as the base object to
**				build a class of polygons. 
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
** 1.1	09/04/2005	Added software license language to header comments
*/
//#include "stdafx.h"
#include <math.h>

#include "php_polygon_vertex.h"

segment::segment(double xc, double yc, int d )
{
	m_xc = xc;
	m_yc = yc;
	m_d = d;
}

vertex::vertex( double x, double y, 
		double xc, double yc, double d,
		vertex * nextV, vertex * prevV, 
		polygon * nextPoly,
		BOOL intersect, 
		vertex * neighbor,
		double alpha,
		BOOL entry,
		BOOL checked )
{ 
	m_x = x;
	m_y = y;
	m_nextV = nextV;
	m_prevV = prevV;
	m_nextPoly = nextPoly;
	m_intersect = intersect;
	m_neighbor = neighbor;
	m_alpha = alpha;
	m_entry = entry;
	m_checked = checked;
	m_id = 0;
	m_nSeg = new segment( xc, yc, (int) d );
	m_pSeg = NULL;
}

vertex::~vertex()
{
	if( m_nSeg )
		delete m_nSeg;
}

double vertex::Xc ( BOOL g )
{
	if ( isIntersect() )
	{
		if ( m_neighbor->isEntry() )
			return m_neighbor->m_nSeg->Xc();
		else
			return m_neighbor->m_pSeg->Xc();
	}
	else
		if (g) 
			return m_nSeg->Xc(); 
		else 
			return m_pSeg->Xc();
}

double vertex::Yc ( BOOL g )
{
	if ( isIntersect() )
	{
		if ( m_neighbor->isEntry() )
			return m_neighbor->m_nSeg->Yc();
		else
			return m_neighbor->m_pSeg->Yc();
	}
	else
		if (g) 
			return m_nSeg->Yc(); 
		else 
			return m_pSeg->Yc();
}

double vertex::d ( BOOL g )
{
	if ( isIntersect() )
	{
		if ( m_neighbor->isEntry() )
			return m_neighbor->m_nSeg->d();
		else
			return (-1*m_neighbor->m_pSeg->d());
	}
	else
		if (g) 
			return m_nSeg->d(); 
		else 
			return (-1*m_pSeg->d());
}

void vertex::setChecked( BOOL check )
{
	m_checked = check;
	if( m_neighbor )
		if( !m_neighbor->isChecked() )
			m_neighbor->setChecked();
}
