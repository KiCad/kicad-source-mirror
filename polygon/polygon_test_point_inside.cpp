/////////////////////////////////////////////////////////////////////////////
// Name:        polygon_test_point_inside.cpp
/////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <vector>
#include "PolyLine.h"

using namespace std;

/* this algo uses the the Jordan curve theorem to find if a point is inside or outside a polygon:
  * It run a semi-infinite line horizontally (increasing x, fixed y)
  * out from the test point, and count how many edges it crosses.
  * At each crossing, the ray switches between inside and outside.
  * If odd count, the test point is inside the polygon
  * This is called the Jordan curve theorem, or sometimes referred to as the "even-odd" test.
 */

/* 2 versions are given.
 * the second version is GPL (currently used)
 * the first version is for explanations and tests (used to test the second version)
 * both use the same algorithm.
*/
#if 0

/* This text and the algorithm come from http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
 *
 * PNPOLY - Point Inclusion in Polygon Test
 * W. Randolph Franklin (WRF)
 *
 * Table of Contents
 *
 * 1. The C Code <#The C Code>
 * 2. The Method <#The Method>
 * 3. Originality <#Originality>
 * 4. The Inequality Tests are Tricky <#The Inequality Tests are Tricky>
 * 5. C Semantics <#C Semantics>
 * 6. Point on a (Boundary) Edge <#Point on an Edge>
 * 7. Multiple Components and Holes <#Listing the Vertices>
 * 8. Testing Which One of Many Polygons Contains the Point <#Testing a
 *   Point Against Many Polygons>
 * 9. Explanation of /"for (i = 0, j = nvert-1; i < nvert; j = i++)"/
 *   <#Explanation>
 * 10. Fortran Code for the Point in Polygon Test <#Fortran Code for the
 *   Point in Polygon Test>
 * 11. Converting the Code to All Integers <#Converting the Code to All
 *   Integers>
 * 12. License to Use <#License to Use>
 *
 * The C Code
 *
 * Here is the code, for reference. Excluding lines with only braces, there
 * are only /7 lines/ of code.
 *
 * int pnpoly(int nvert, float *vertx, float *verty, float ref_pointX, float ref_pointY)
 * {
 * int i, j, c = 0;
 * for (i = 0, j = nvert-1; i < nvert; j = i++) {
 * if ( ((verty[i]>ref_pointY) != (verty[j]>ref_pointY)) &&
 *  (ref_pointX < (vertx[j]-vertx[i]) * (ref_pointY-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
 *    c = !c;
 * }
 * return c;
 * }
 *
 * Argument	Meaning
 * nvert 	Number of vertices in the polygon. Whether to repeat the first
 * vertex at the end is discussed below.
 * vertx, verty 	Arrays containing the x- and y-coordinates of the
 * polygon's vertices.
 * ref_pointX, ref_pointY	X- and y-coordinate of the test point.
 *
 *
 * The Method
 *
 * I run a semi-infinite ray horizontally (increasing x, fixed y) out from
 * the test point, and count how many edges it crosses. At each crossing,
 * the ray switches between inside and outside. This is called the /Jordan
 * curve theorem/.
 *
 * The case of the ray going thru a vertex is handled correctly via a
 * careful selection of inequalities. Don't mess with this code unless
 * you're familiar with the idea of /Simulation of Simplicity/. This
 * pretends to shift the ray infinitesimally to one side so that it either
 * clearly intersects, or clearly doesn't touch. Since this is merely a
 * conceptual, infinitesimal, shift, it never creates an intersection that
 * didn't exist before, and never destroys an intersection that clearly
 * existed before.
 *
 * The ray is tested against each edge thus:
 *
 * 1. Is the point in the half-plane below the extended edge? and
 * 2. Is the point's X coordinate within the edge's X-range?
 *
 * Handling endpoints here is tricky.
 *
 *
 * Originality
 *
 * I make no claim to having invented the idea. However in 1970, I did
 * produce the Fortran code given below on my own, and include it in a
 * package of cartographic SW publicly-distributed by David Douglas, Dept
 * of Geography, Simon Fraser U and U of Ottawa.
 *
 * Earlier implementations of point-in-polygon testing presumably exist,
 * tho the code might never have been released. Pointers to prior art,
 * especially publicly available code, are welcome. One early publication,
 * which doesn't handle the point on an edge, and has a typo, is this:
 *
 * M Shimrat, "Algorithm 112, Position of Point Relative to Polygon",
 *   /Comm. ACM/ 5(8), Aug 1962, p 434.
 *
 * A well-written recent summary is this:
 *
 * E Haines, /Point in Polygon Strategies/,
 *   http://www.acm.org/pubs/tog/editors/erich/ptinpoly/, 1994.
 *
 *
 * The Inequality Tests are Tricky
 *
 * If translating the program to another language, be sure to get the
 * inequalities in the conditional correct. They were carefully chosen to
 * make the program work correctly when the point is vertically below a vertex.
 *
 * Several people have thought that my program was wrong, when really
 * /they/ had gotten the inequalities wrong.
 *
 *
 * C Semantics
 *
 * My code uses the fact that, in the C language, when executing the code
 |a&&b|, if |a| is false, then |b| must not be evaluated. If your
 * compiler doesn't do this, then it's not implementing C, and you will get
 * a divide-by-zero, i.a., when the test point is vertically in line with a
 * vertical edge. When translating this code to another language with
 * different semantics, then you must implement this test explicitly.
 *
 *
 * Point on a (Boundary) Edge
 *
 * PNPOLY partitions the plane into points inside the polygon and points
 * outside the polygon. Points that are on the boundary are classified as
 * either inside or outside.
 *
 * 1.
 *
 *   Any particular point is always classified consistently the same
 *   way. In the following figure, consider what PNPOLY would say when
 *   the red point, /P/, is tested against the two triangles, /T_L /
 *   and /T_R /. Depending on internal roundoff errors, PNPOLY may say
 *   that /P/ is in /T_L / or in /T_R /. However it will always give
 *   the same answer when /P/ is tested against those triangles. That
 *   is, if PNPOLY finds that /P/ is in /T_L /, then it will find that
 *   /P/ is not /T_R /. If PNPOLY finds that /P/ is not in /T_L /, then
 *   it will find that /P/ is in /T_R /.
 *
 * 2. If you want to know when a point is exactly on the boundary, you
 *   need another program. This is only one of many functions that
 *   PNPOLY lacks; it also doesn't predict tomorrow's weather. You are
 *   free to extend PNPOLY's source code.
 *
 * 3. The first reason for this is the numerical analysis position that
 *   you should not be testing exact equality unless your input is
 *   exact. Even then, computational roundoff error would often make
 *   the result wrong.
 *
 * 4. The second reason is that, if you partition a region of the plane
 *   into polygons, i.e., form a planar graph, then PNPOLY will locate
 *   each point into exactly one polygon. In other words, PNPOLY
 *   considers each polygon to be topologically a semi-open set. This
 *   makes things simpler, i.e., causes fewer special cases, if you use
 *   PNPOLY as part of a larger system. Examples of this include
 *   locating a point in a planar graph, and intersecting two planar
 *   graphs.
 *
 *
 * Explanation of /"for (i = 0, j = nvert-1; i < nvert; j = i++)"/
 *
 * The intention is to execute the loop for each i from 0 to nvert-1. For
 * each iteration, j is i-1. However that wraps, so if i=0 then j=nvert-1.
 * Therefore the current edge runs between verts j and i, and the loop is
 * done once per edge. In detail:
 *
 * 1. Start by setting i and j:
 *   i = 0
 *   j = nvert-1
 * 2. If i<nvert is false then exit the loop.
 * 3. Do the loop body.
 * 4. Set j=i and then
 *   add 1 to i and then
 * 5. Go back to step 2.
 *
 *
 *
 * Converting the Code to All Integers
 *
 * If you want to convert the code from floats to integers, consider these
 * issues.
 *
 * 1. On many current processors floats are at least as fast as ints.
 * 2. If you move the denominator over to the other side of the
 *   inequality, remember that, when the denominator is negative, the
 *   inequality will flip.
 * 3. If coordinates are large enough, the multiplication will silently
 *   overflow.
 *
 *
 * License to Use
 * Copyright (c) 1970-2003, Wm. Randolph Franklin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimers.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice in the documentation and/or other materials provided with
 *   the distribution.
 * 3. The name of W. Randolph Franklin may not be used to endorse or
 *   promote products derived from this Software without specific prior
 *   written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Copyright © 1994-2006, W Randolph Franklin (WRF)
 * <http://wrfranklin.org/> You may use my material for non-profit research
 * and education, provided that you credit me, and link back to my home page.
 * http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html,
 *     05/20/2008 20:36:42
 */

bool TestPointInsidePolygon( std::vector <CPolyPt> aPolysList,
                             int                   istart,
                             int                   iend,
                             int                   refx,
                             int                   refy )

/** Function TestPointInsidePolygon
 * test if a point is inside or outside a polygon.
 * @param aPolysList: the list of polygons
 * @param istart: the starting point of a given polygon in m_FilledPolysList.
 * @param iend: the ending point of the polygon in m_FilledPolysList.
 * @param refx, refy: the point coordinate to test
 * @return true if the point is inside, false for outside
 */
{
    double ref_pointX = refx;
    double ref_pointY = refy;

    bool   inside = false;

    for( int ii = istart, jj = iend; ii <= iend; jj = ii++ )
    {
        double seg_startX, seg_startY;  // starting point for the segment to test
        seg_startX = aPolysList[ii].x;
        seg_startY = aPolysList[ii].y;
        double seg_endX, seg_endY;      // ending point for the segment to test
        seg_endX = aPolysList[jj].x;
        seg_endY = aPolysList[jj].y;
        if( ( ( seg_startY > ref_pointY ) != (seg_endY > ref_pointY ) )
           && (ref_pointX <
               (seg_endX -
                seg_startX) * (ref_pointY - seg_startY) / (seg_endY - seg_startY) + seg_startX) )
            inside = not inside;
    }

    return inside;
}


#else

/* this algo come from freePCB.
 */

bool TestPointInsidePolygon( std::vector <CPolyPt> aPolysList,
                             int                   istart,
                             int                   iend,
                             int                   refx,
                             int                   refy )

/** Function TestPointInsidePolygon
 * test if a point is inside or outside a polygon.
 * if a point is on a  outline segment, it is considered outside the polygon
 * @param aPolysList: the list of polygons
 * @param istart: the starting point of a given polygon in m_FilledPolysList.
 * @param iend: the ending point of the polygon in m_FilledPolysList.
 * @param refx,refy: the point coordinate to test
 * @return true if the point is inside, false for outside
 * this algorithm come from FreePCB.
 */
{
    #define OUTSIDE_IF_ON_SIDE 0    // = 1 if we consider point on a side outside the polygon
    // define line passing through (x,y), with slope = 0 (horizontal line)
    // get intersection points
    // count intersection points to right of (x,y), if odd (x,y) is inside polyline
    int    xx, yy;
    double slope = 0;
    double a      = refy - slope * refx;
    int    ics, ice;
    bool   inside = false;

    // find all intersection points of line with polyline sides
    for( ics = istart, ice = iend; ics <= iend; ice = ics++ )
    {
        double x, y, x2, y2;
        int    ok = FindLineSegmentIntersection( a, slope,
            aPolysList[ics].x, aPolysList[ics].y,
            aPolysList[ice].x, aPolysList[ice].y,
            0,
            &x, &y,
            &x2, &y2 );
        if( ok )
        {
            xx = (int) x;
            yy = (int) y;
#if OUTSIDE_IF_ON_SIDE
            if( xx == refx && yy == refy )
                return false; // (x,y) is on a side, call it outside
            else
#endif
            if( xx > refx )
                inside = not inside;
        }
        if( ok == 2 )
        {
            xx = (int) x2;
            yy = (int) y2;
#if OUTSIDE_IF_ON_SIDE
            if( xx == refx && yy == refy )
                return false; // (x,y) is on a side, call it outside
            else
#endif
            if( xx > refx )
                inside = not inside;
        }
    }

    return inside;
}


#endif
