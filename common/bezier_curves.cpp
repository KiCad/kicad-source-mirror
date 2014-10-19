/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#define add_segment(segment) if(s_bezier_Points_Buffer[s_bezier_Points_Buffer.size()-1] != segment) s_bezier_Points_Buffer.push_back(segment);


// Local variables:
static std::vector<wxPoint> s_bezier_Points_Buffer;

static int    bezier_recursion_limit     = 12;
static double bezier_approximation_scale = 0.5;  // 1

static double bezier_curve_collinearity_epsilon    = 1e-30;
static double bezier_curve_angle_tolerance_epsilon = 0.0001;
static double bezier_distance_tolerance_square; // derived by approximation_scale
static double bezier_angle_tolerance = 0.0;
static double bezier_cusp_limit = 0.0;

// Local functions:
static void recursive_bezier( int x1, int y1, int x2, int y2, int x3, int y3, int level );
static void recursive_bezier( int x1,
                              int y1,
                              int x2,
                              int y2,
                              int x3,
                              int y3,
                              int x4,
                              int y4,
                              int level );

/***********************************************************************************/


std::vector<wxPoint> Bezier2Poly( wxPoint c1, wxPoint c2, wxPoint c3, wxPoint c4 )
{
    return Bezier2Poly( c1.x, c1.y, c2.x, c2.y, c3.x, c3.y, c4.x, c4.y );
}


std::vector<wxPoint> Bezier2Poly( wxPoint c1, wxPoint c2, wxPoint c3 )
{
    return Bezier2Poly( c1.x, c1.y, c2.x, c2.y, c3.x, c3.y );
}


inline double calc_sq_distance( int x1, int y1, int x2, int y2 )
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    return (double)dx * dx + (double)dy * dy;
}

inline double sqrt_len( int dx, int dy )
{
    return ((double)dx * dx) + ((double)dy * dy);
}


std::vector<wxPoint>  Bezier2Poly( int x1, int y1, int x2, int y2, int x3, int y3 )
{
    s_bezier_Points_Buffer.clear();

    bezier_distance_tolerance_square  = 0.5 / bezier_approximation_scale;
    bezier_distance_tolerance_square *= bezier_distance_tolerance_square;
    s_bezier_Points_Buffer.push_back( wxPoint( x1, y1 ) );
    recursive_bezier( x1, y1, x2, y2, x3, y3, 0 );
    s_bezier_Points_Buffer.push_back( wxPoint( x3, y3 ) );

    wxLogDebug( wxT( "Bezier Conversion - End (%d vertex)" ), s_bezier_Points_Buffer.size() );
    return s_bezier_Points_Buffer;
}


std::vector<wxPoint> Bezier2Poly( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4 )
{
    s_bezier_Points_Buffer.clear();
    bezier_distance_tolerance_square  = 0.5 / bezier_approximation_scale;
    bezier_distance_tolerance_square *= bezier_distance_tolerance_square;

    s_bezier_Points_Buffer.push_back( wxPoint( x1, y1 ) );
    recursive_bezier( x1, y1, x2, y2, x3, y3, x4, y4, 0 );
    s_bezier_Points_Buffer.push_back( wxPoint( x4, y4 ) );
    wxLogDebug( wxT( "Bezier Conversion - End (%d vertex)" ), s_bezier_Points_Buffer.size() );
    return s_bezier_Points_Buffer;
}


void recursive_bezier( int x1, int y1, int x2, int y2, int x3, int y3, int level )
{
    if( abs( level ) > bezier_recursion_limit )
    {
        return;
    }

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

    if( d > bezier_curve_collinearity_epsilon )
    {
        // Regular case
        //-----------------
        if( d * d <= bezier_distance_tolerance_square * (dx * dx + dy * dy) )
        {
            // If the curvature doesn't exceed the distance_tolerance value
            // we tend to finish subdivisions.
            //----------------------
            if( bezier_angle_tolerance < bezier_curve_angle_tolerance_epsilon )
            {
                add_segment( wxPoint( x123, y123 ) );
                return;
            }

            // Angle & Cusp Condition
            //----------------------
            da = fabs( atan2( (double) ( y3 - y2 ), (double) ( x3 - x2 ) ) -
                       atan2( (double) ( y2 - y1 ), (double) ( x2 - x1 ) ) );
            if( da >=M_PI )
                da = 2 * M_PI - da;

            if( da < bezier_angle_tolerance )
            {
                // Finally we can stop the recursion
                //----------------------
                add_segment( wxPoint( x123, y123 ) );
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
        if( d < bezier_distance_tolerance_square )
        {
            add_segment( wxPoint( x2, y2 ) );
            return;
        }
    }

    // Continue subdivision
    //----------------------
    recursive_bezier( x1, y1, x12, y12, x123, y123, level + 1 );
    recursive_bezier( x123, y123, x23, y23, x3, y3, -(level + 1) );
}


void recursive_bezier( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int level )
{
    if( abs( level ) > bezier_recursion_limit )
    {
        return;
    }

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

    switch( (int(d2 > bezier_curve_collinearity_epsilon) << 1) +
           int(d3 > bezier_curve_collinearity_epsilon) )
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
            if( d2 < bezier_distance_tolerance_square )
            {
                add_segment( wxPoint( x2, y2 ) );
                return;
            }
        }
        else
        {
            if( d3 < bezier_distance_tolerance_square )
            {
                add_segment( wxPoint( x3, y3 ) );
                return;
            }
        }
        break;

    case 1:

        // p1,p2,p4 are collinear, p3 is significant
        //----------------------
        if( d3 * d3 <= bezier_distance_tolerance_square * sqrt_len(dx, dy) )
        {
            if( bezier_angle_tolerance < bezier_curve_angle_tolerance_epsilon )
            {
                add_segment( wxPoint( x23, y23 ) );
                return;
            }

            // Angle Condition
            //----------------------
            da1 = fabs( atan2( (double) ( y4 - y3 ), (double) ( x4 - x3 ) ) -
                        atan2( (double) ( y3 - y2 ), (double) ( x3 - x2 ) ) );
            if( da1 >= M_PI )
                da1 = 2 * M_PI - da1;

            if( da1 < bezier_angle_tolerance )
            {
                add_segment( wxPoint( x2, y2 ) );
                add_segment( wxPoint( x3, y3 ) );
                return;
            }

            if( bezier_cusp_limit != 0.0 )
            {
                if( da1 > bezier_cusp_limit )
                {
                    add_segment( wxPoint( x3, y3 ) );
                    return;
                }
            }
        }
        break;

    case 2:

        // p1,p3,p4 are collinear, p2 is significant
        //----------------------
        if( d2 * d2 <= bezier_distance_tolerance_square * sqrt_len(dx, dy) )
        {
            if( bezier_angle_tolerance < bezier_curve_angle_tolerance_epsilon )
            {
                add_segment( wxPoint( x23, y23 ) );
                return;
            }

            // Angle Condition
            //----------------------
            da1 = fabs( atan2( (double) ( y3 - y2 ), (double) ( x3 - x2 ) ) -
                        atan2( (double) ( y2 - y1 ), (double) ( x2 - x1 ) ) );
            if( da1 >= M_PI )
                da1 = 2 * M_PI - da1;

            if( da1 < bezier_angle_tolerance )
            {
                add_segment( wxPoint( x2, y2 ) );
                add_segment( wxPoint( x3, y3 ) );
                return;
            }

            if( bezier_cusp_limit != 0.0 )
            {
                if( da1 > bezier_cusp_limit )
                {
                    add_segment( wxPoint( x2, y2 ) );
                    return;
                }
            }
        }
        break;

    case 3:

        // Regular case
        //-----------------
        if( (d2 + d3) * (d2 + d3) <= bezier_distance_tolerance_square * sqrt_len(dx, dy) )
        {
            // If the curvature doesn't exceed the distance_tolerance value
            // we tend to finish subdivisions.
            //----------------------
            if( bezier_angle_tolerance < bezier_curve_angle_tolerance_epsilon )
            {
                add_segment( wxPoint( x23, y23 ) );
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

            if( da1 + da2 < bezier_angle_tolerance )
            {
                // Finally we can stop the recursion
                //----------------------
                add_segment( wxPoint( x23, y23 ) );
                return;
            }

            if( bezier_cusp_limit != 0.0 )
            {
                if( da1 > bezier_cusp_limit )
                {
                    add_segment( wxPoint( x2, y2 ) );
                    return;
                }

                if( da2 > bezier_cusp_limit )
                {
                    add_segment( wxPoint( x3, y3 ) );
                    return;
                }
            }
        }
        break;
    }

    // Continue subdivision
    //----------------------
    recursive_bezier( x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1 );
    recursive_bezier( x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1 );
}
