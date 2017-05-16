/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/************************************/
/* routines to handle bezier curves */
/************************************/

#include <fctsys.h>
#include <bezier_curves.h>


static inline double calc_sq_distance( int x1, int y1, int x2, int y2 )
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    return (double)dx * dx + (double)dy * dy;
}


static inline double sqrt_len( int dx, int dy )
{
    return ((double)dx * dx) + ((double)dy * dy);
}


void BEZIER_POLY::GetPoly( std::vector<wxPoint>& aOutput )
{
    m_output = &aOutput;
    m_output->clear();
    m_output->push_back( wxPoint( m_ctrlPts.front() ) );

    // Only quadratic and cubic Bezier curves are handled
    if( m_ctrlPts.size() == 3 )
        recursiveBezier( m_ctrlPts[0].x, m_ctrlPts[0].y,
                m_ctrlPts[1].x, m_ctrlPts[1].y,
                m_ctrlPts[2].x, m_ctrlPts[2].y, 0 );

    else if( m_ctrlPts.size() == 4 )
        recursiveBezier( m_ctrlPts[0].x, m_ctrlPts[0].y,
                m_ctrlPts[1].x, m_ctrlPts[1].y,
                m_ctrlPts[2].x, m_ctrlPts[2].y,
                m_ctrlPts[3].x, m_ctrlPts[3].y, 0 );

    m_output->push_back( wxPoint( m_ctrlPts.back() ) );
}


void BEZIER_POLY::recursiveBezier( int x1, int y1, int x2, int y2, int x3, int y3, unsigned int level )
{
    if( level > recursion_limit )
        return;

    // Calculate all the mid-points of the line segments
    //----------------------
    int    x12  = (x1 + x2) / 2;
    int    y12  = (y1 + y2) / 2;
    int    x23  = (x2 + x3) / 2;
    int    y23  = (y2 + y3) / 2;
    int    x123 = (x12 + x23) / 2;
    int    y123 = (y12 + y23) / 2;

    int    dx = x3 - x1;
    int    dy = y3 - y1;
    double d  = fabs( ((double) (x2 - x3) * dy) - ((double) (y2 - y3) * dx ) );
    double da;

    if( d > curve_collinearity_epsilon )
    {
        // Regular case
        //-----------------
        if( d * d <= distance_tolerance_square * (dx * dx + dy * dy) )
        {
            // If the curvature doesn't exceed the distance_tolerance value
            // we tend to finish subdivisions.
            //----------------------
            if( angle_tolerance < curve_angle_tolerance_epsilon )
            {
                addSegment( wxPoint( x123, y123 ) );
                return;
            }

            // Angle & Cusp Condition
            //----------------------
            da = fabs( atan2( (double) ( y3 - y2 ), (double) ( x3 - x2 ) ) -
                       atan2( (double) ( y2 - y1 ), (double) ( x2 - x1 ) ) );
            if( da >=M_PI )
                da = 2 * M_PI - da;

            if( da < angle_tolerance )
            {
                // Finally we can stop the recursion
                //----------------------
                addSegment( wxPoint( x123, y123 ) );
                return;
            }
        }
    }
    else
    {
        // Collinear case
        //------------------
        da = sqrt_len(dx, dy);
        if( da == 0 )
        {
            d = calc_sq_distance( x1, y1, x2, y2 );
        }
        else
        {
            d = ( (double)(x2 - x1) * dx + (double)(y2 - y1) * dy ) / da;
            if( d > 0 && d < 1 )
            {
                // Simple collinear case, 1---2---3
                // We can leave just two endpoints
                return;
            }
            if( d <= 0 )
                d = calc_sq_distance( x2, y2, x1, y1 );
            else if( d >= 1 )
                d = calc_sq_distance( x2, y2, x3, y3 );
            else
                d = calc_sq_distance( x2, y2, x1 + (int) d * dx,
                                      y1 + (int) d * dy );
        }
        if( d < distance_tolerance_square )
        {
            addSegment( wxPoint( x2, y2 ) );
            return;
        }
    }

    // Continue subdivision
    //----------------------
    recursiveBezier( x1, y1, x12, y12, x123, y123, level + 1 );
    recursiveBezier( x123, y123, x23, y23, x3, y3, -(level + 1) );
}


void BEZIER_POLY::recursiveBezier( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned int level )
{
    if( level > recursion_limit )
        return;

    // Calculate all the mid-points of the line segments
    //----------------------
    int x12   = (x1 + x2) / 2;
    int y12   = (y1 + y2) / 2;
    int x23   = (x2 + x3) / 2;
    int y23   = (y2 + y3) / 2;
    int x34   = (x3 + x4) / 2;
    int y34   = (y3 + y4) / 2;
    int x123  = (x12 + x23) / 2;
    int y123  = (y12 + y23) / 2;
    int x234  = (x23 + x34) / 2;
    int y234  = (y23 + y34) / 2;
    int x1234 = (x123 + x234) / 2;
    int y1234 = (y123 + y234) / 2;


    // Try to approximate the full cubic curve by a single straight line
    //------------------
    int    dx = x4 - x1;
    int    dy = y4 - y1;

    double d2 = fabs( (double) ( (x2 - x4) * dy - (y2 - y4) * dx ) );
    double d3 = fabs( (double) ( (x3 - x4) * dy - (y3 - y4) * dx ) );
    double da1, da2, k;

    switch( (int(d2 > curve_collinearity_epsilon) << 1) +
           int(d3 > curve_collinearity_epsilon) )
    {
    case 0:

        // All collinear OR p1==p4
        //----------------------
        k = dx * dx + dy * dy;
        if( k == 0 )
        {
            d2 = calc_sq_distance( x1, y1, x2, y2 );
            d3 = calc_sq_distance( x4, y4, x3, y3 );
        }
        else
        {
            k   = 1 / k;
            da1 = x2 - x1;
            da2 = y2 - y1;
            d2  = k * (da1 * dx + da2 * dy);
            da1 = x3 - x1;
            da2 = y3 - y1;
            d3  = k * (da1 * dx + da2 * dy);
            if( d2 > 0 && d2 < 1 && d3 > 0 && d3 < 1 )
            {
                // Simple collinear case, 1---2---3---4
                // We can leave just two endpoints
                return;
            }
            if( d2 <= 0 )
                d2 = calc_sq_distance( x2, y2, x1, y1 );
            else if( d2 >= 1 )
                d2 = calc_sq_distance( x2, y2, x4, y4 );
            else
                d2 = calc_sq_distance( x2, y2, x1 + (int) d2 * dx,
                                       y1 + (int) d2 * dy );

            if( d3 <= 0 )
                d3 = calc_sq_distance( x3, y3, x1, y1 );
            else if( d3 >= 1 )
                d3 = calc_sq_distance( x3, y3, x4, y4 );
            else
                d3 = calc_sq_distance( x3, y3, x1 + (int) d3 * dx,
                                       y1 + (int) d3 * dy );
        }
        if( d2 > d3 )
        {
            if( d2 < distance_tolerance_square )
            {
                addSegment( wxPoint( x2, y2 ) );
                return;
            }
        }
        else
        {
            if( d3 < distance_tolerance_square )
            {
                addSegment( wxPoint( x3, y3 ) );
                return;
            }
        }
        break;

    case 1:

        // p1,p2,p4 are collinear, p3 is significant
        //----------------------
        if( d3 * d3 <= distance_tolerance_square * sqrt_len(dx, dy) )
        {
            if( angle_tolerance < curve_angle_tolerance_epsilon )
            {
                addSegment( wxPoint( x23, y23 ) );
                return;
            }

            // Angle Condition
            //----------------------
            da1 = fabs( atan2( (double) ( y4 - y3 ), (double) ( x4 - x3 ) ) -
                        atan2( (double) ( y3 - y2 ), (double) ( x3 - x2 ) ) );
            if( da1 >= M_PI )
                da1 = 2 * M_PI - da1;

            if( da1 < angle_tolerance )
            {
                addSegment( wxPoint( x2, y2 ) );
                addSegment( wxPoint( x3, y3 ) );
                return;
            }

            if( cusp_limit != 0.0 )
            {
                if( da1 > cusp_limit )
                {
                    addSegment( wxPoint( x3, y3 ) );
                    return;
                }
            }
        }
        break;

    case 2:

        // p1,p3,p4 are collinear, p2 is significant
        //----------------------
        if( d2 * d2 <= distance_tolerance_square * sqrt_len(dx, dy) )
        {
            if( angle_tolerance < curve_angle_tolerance_epsilon )
            {
                addSegment( wxPoint( x23, y23 ) );
                return;
            }

            // Angle Condition
            //----------------------
            da1 = fabs( atan2( (double) ( y3 - y2 ), (double) ( x3 - x2 ) ) -
                        atan2( (double) ( y2 - y1 ), (double) ( x2 - x1 ) ) );
            if( da1 >= M_PI )
                da1 = 2 * M_PI - da1;

            if( da1 < angle_tolerance )
            {
                addSegment( wxPoint( x2, y2 ) );
                addSegment( wxPoint( x3, y3 ) );
                return;
            }

            if( cusp_limit != 0.0 )
            {
                if( da1 > cusp_limit )
                {
                    addSegment( wxPoint( x2, y2 ) );
                    return;
                }
            }
        }
        break;

    case 3:

        // Regular case
        //-----------------
        if( (d2 + d3) * (d2 + d3) <= distance_tolerance_square * sqrt_len(dx, dy) )
        {
            // If the curvature doesn't exceed the distance_tolerance value
            // we tend to finish subdivisions.
            //----------------------
            if( angle_tolerance < curve_angle_tolerance_epsilon )
            {
                addSegment( wxPoint( x23, y23 ) );
                return;
            }

            // Angle & Cusp Condition
            //----------------------
            k   = atan2( (double) ( y3 - y2 ), (double) ( x3 - x2 ) );
            da1 = fabs( k - atan2( (double) ( y2 - y1 ),
                                   (double) ( x2 - x1 ) ) );
            da2 = fabs( atan2( (double) ( y4 - y3 ),
                               (double) ( x4 - x3 ) ) - k );
            if( da1 >= M_PI )
                da1 = 2 * M_PI - da1;
            if( da2 >= M_PI )
                da2 = 2 * M_PI - da2;

            if( da1 + da2 < angle_tolerance )
            {
                // Finally we can stop the recursion
                //----------------------
                addSegment( wxPoint( x23, y23 ) );
                return;
            }

            if( cusp_limit != 0.0 )
            {
                if( da1 > cusp_limit )
                {
                    addSegment( wxPoint( x2, y2 ) );
                    return;
                }

                if( da2 > cusp_limit )
                {
                    addSegment( wxPoint( x3, y3 ) );
                    return;
                }
            }
        }
        break;
    }

    // Continue subdivision
    //----------------------
    recursiveBezier( x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1 );
    recursiveBezier( x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1 );
}
