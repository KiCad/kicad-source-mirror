/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers.
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


// WARNING - this Tom's crappy PNS hack tool code. Please don't complain about its quality
// (unless you want to improve it).

#include <pgm_base.h>
#include <qa_utils/utility_registry.h>

#include "pns_log_viewer_frame.h"
#include "label_manager.h"

#include <geometry/shape_arc.h>

std::shared_ptr<PNS_LOG_VIEWER_OVERLAY> overlay;


template<class T>
static T within_range(T x, T minval, T maxval, T wrap)
{
    int rv;

    while (maxval >= wrap)
        maxval -= wrap;

    while (maxval < 0)
        maxval += wrap;

    while (minval >= wrap)
        minval -= wrap;

    while (minval < 0)
        minval += wrap;

    while (x < 0)
        x += wrap;

    while (x >= wrap)
        x -= wrap;

    if (maxval > minval)
        rv = (x >= minval && x <= maxval) ? 1 : 0;
    else
        rv = (x >= minval || x <= maxval) ? 1 : 0;

    return rv;
}

static bool sliceContainsPoint( SHAPE_ARC a, VECTOR2D p )
{
    double phi = 180.0 / M_PI * atan2( p.y - a.GetCenter().y, p.x - a.GetCenter().x );
    double ca = a.GetCentralAngle();
    double sa = a.GetStartAngle();
    double ea;
    if( ca >= 0 )
    {
        ea = sa + ca;
    }
    else
    {
        ea = sa;
        sa += ca;
    }


    return within_range( phi, sa, ea, 360.0 );
}

static int arclineIntersect( const SHAPE_ARC& a, const SEG& s, VECTOR2D* pts )
{
    VECTOR2I c = a.GetCenter();
    double r = a.GetRadius();
    VECTOR2I nearest = s.LineProject( c );

    double nd = (c - nearest).EuclideanNorm();

    /*overlay->SetStrokeColor( RED );
    overlay->AnnotatedPoint( s.A, 200000 );
    overlay->SetStrokeColor( GREEN );
    overlay->AnnotatedPoint( s.B, 200000 );
    overlay->SetStrokeColor( YELLOW );
    overlay->AnnotatedPoint( c, 200000 );


    overlay->SetStrokeColor( WHITE );
    overlay->Arc( a );
    overlay->SetStrokeColor( DARKGRAY );
    overlay->Circle( a.GetCenter(), a.GetRadius() );*/

    if( nd > r )
        return 0;

    double ll = r * r - nd * nd;
    double l = 0;
    if( ll > 0 )
        l = sqrt(ll);

    int n = 0;

    VECTOR2I perp = ( s.B - s.A ).Resize( l );

    VECTOR2I p0( nearest + perp );
    VECTOR2I p1( nearest - perp );

    if( sliceContainsPoint( a, p0 ) ) { pts[n] = p0; n++; }
    if( sliceContainsPoint( a, p1 ) ) { pts[n] = p1; n++; }

    //for(int i = 0; i < n ; i++)    overlay->AnnotatedPoint( pts[i], 40000 );

    return n;
}

int intersectArc2Arc( const SHAPE_ARC& a1, const SHAPE_ARC& a2, VECTOR2D* ips )
{
    VECTOR2I dc( a2.GetCenter() - a1.GetCenter() );
    double dcl = dc.EuclideanNorm();
    double r1 = a1.GetRadius();
    double r2 = a2.GetRadius();

    if( dc.x == 0 && dc.y == 0 )
    {
        if(r1 == r2)
        {
            ips[0] = a1.GetP0();
            return 1;
        }
    }

    double d = ( dcl * dcl - r2*r2 + r1*r1 ) / (2 * dcl);

    //overlay->SetStrokeColor( DARKGRAY );
    //overlay->Circle( a1.GetCenter(), a1.GetRadius() );
    //overlay->SetStrokeColor( DARKGRAY );
    //overlay->Circle( a2.GetCenter(), a2.GetRadius() );

    double r1sq = r1 * r1;
    double dsq = d * d;

    if( r1sq < dsq )
        return 0;

    double q = sqrt ( r1sq - dsq );

//    printf("d %.1f q %.1f dcl %.1f\n", d, q, dcl );

    VECTOR2D pp = a1.GetCenter() + dc.Resize( d );

    VECTOR2D ip0 = pp + dc.Perpendicular().Resize(q);
    VECTOR2D ip1 = pp - dc.Perpendicular().Resize(q);

    int n_ips = 0;

    if( sliceContainsPoint( a1, ip0 ) )
        ips[n_ips++] = ip0;
    if( sliceContainsPoint( a1, ip1 ) )
        ips[n_ips++] = ip1;

    return n_ips;
}


bool collideArc2Arc( const SHAPE_ARC& a1, const SHAPE_ARC& a2, int clearance, SEG& minDistSeg )
{
    SEG mediatrix( a1.GetCenter(), a2.GetCenter() );

    int      nA = 0, nB = 0;
    VECTOR2D ptsA[8];
    VECTOR2D ptsB[8];


    VECTOR2D ips[4];

    if( intersectArc2Arc( a1, a2, ips ) > 0 )
    {
        minDistSeg.A = minDistSeg.B = ips[0];
        return true;
    }

    bool cocentered = ( mediatrix.A == mediatrix.B );

    // arcs don't have the same center point
    if( !cocentered )
    {
        nA = arclineIntersect( a1, mediatrix, ptsA );
        nB = arclineIntersect( a2, mediatrix, ptsB );
    }

    /*overlay->SetStrokeColor( LIGHTRED );
        overlay->Line( SEG( a2.GetP0(), a2.GetCenter() ) );
        overlay->Line( SEG( a2.GetP1(), a2.GetCenter() ) );*/

    nA += arclineIntersect( a1, SEG( a2.GetP0(), a2.GetCenter() ), ptsA + nA );
    nA += arclineIntersect( a1, SEG( a2.GetP1(), a2.GetCenter() ), ptsA + nA );


    /*overlay->SetStrokeColor( LIGHTBLUE );
        overlay->Line( SEG( a1.GetP0(), a1.GetCenter() ) );
        overlay->Line( SEG( a1.GetP1(), a1.GetCenter() ) );*/

    nB += arclineIntersect( a2, SEG( a1.GetP0(), a1.GetCenter() ), ptsB + nB );
    nB += arclineIntersect( a2, SEG( a1.GetP1(), a1.GetCenter() ), ptsB + nB );

    ptsA[nA++] = a1.GetP0();
    ptsA[nA++] = a1.GetP1();
    ptsB[nB++] = a2.GetP0();
    ptsB[nB++] = a2.GetP1();

    //    printf("nA %d nB %d\n", nA, nB );

    double minDist;
    bool   minDistFound = false;

    for( int i = 0; i < nA; i++ )
        for( int j = 0; j < nB; j++ )
        {
            double dist =
                    ( ptsA[i] - ptsB[j] ).EuclideanNorm() - a1.GetWidth() / 2 - a2.GetWidth() / 2;

            /* overlay->SetStrokeColor( RED );
            overlay->AnnotatedPoint( ptsA[i], 100000 );
            overlay->SetStrokeColor( GREEN );
            overlay->AnnotatedPoint( ptsB[j], 100000 );*/


            //printf("dist %.1f\n", dist );

            if( dist < clearance )
            {
                if( !minDistFound || dist < minDist )
                {
                    minDist = dist;
                    minDistSeg = SEG( ptsA[i], ptsB[j] );
                }

                minDistFound = true;
            }
        }


    return minDistFound;
}


int playground_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );
    Pgm().App().SetTopWindow( frame );      // wxApp gets a face.
    frame->Show();

    struct ARC_DATA
    {
        double cx, cy, sx, sy, ca, w;
    };

    const ARC_DATA test_data [] =
    {
        {73.843527, 74.355869, 71.713528, 72.965869, -76.36664803, 0.2},
        {71.236473, 74.704131, 73.366472, 76.094131, -76.36664803, 0.2},
        {82.542335, 74.825975, 80.413528, 73.435869, -76.4, 0.2},
        {76.491192, 73.839894, 78.619999, 75.23, -76.4, 0.2},
        {89.318807, 74.810106, 87.19, 73.42, -76.4, 0.2},
        {87.045667, 74.632941, 88.826472, 75.794131, -267.9, 0.2},
        {94.665667, 73.772941, 96.446472, 74.934131, -267.9, 0.2},
        {94.750009, 73.74012, 93.6551, 73.025482, -255.5, 0.2},
        {72.915251, 80.493054, 73.570159, 81.257692, -260.5, 0.2}, // end points clearance false positive
        {73.063537, 82.295989, 71.968628, 81.581351, -255.5, 0.2},
        {79.279991, 80.67988, 80.3749, 81.394518, -255.5, 0.2},
        {79.279991, 80.67988, 80.3749, 81.694518, -255.5, 0.2 },
        {88.495265, 81.766089, 90.090174, 82.867869, -255.5, 0.2},
        {86.995265, 81.387966, 89.090174, 82.876887, -255.5, 0.2},
        {96.149734, 81.792126, 94.99, 83.37, -347.2, 0.2},
        {94.857156, 81.240589, 95.91, 83.9, -288.5, 0.2},
        {72.915251, 86.493054, 73.970159, 87.257692, -260.5, 0.2}, // end points clearance #1
        {73.063537, 88.295989, 71.968628, 87.581351, -255.5, 0.2},
        {78.915251, 86.393054, 79.970159, 87.157692, 99.5, 0.2}, // end points clearance #2 - false positive
        {79.063537, 88.295989, 77.968628, 87.581351, -255.5, 0.2},
        {85.915251, 86.993054, 86.970159, 87.757692, 99.5, 0.2}, // intersection - false negative
        {86.063537, 88.295989, 84.968628, 87.581351, -255.5, 0.2},
    };


    overlay = frame->GetOverlay();


    overlay->SetIsFill(false);
    overlay->SetLineWidth(10000);

    std::vector<SHAPE_ARC> arcs;
    int n_arcs = sizeof( test_data ) / sizeof( ARC_DATA );

    BOX2I vp;

    for( int i = 0; i < n_arcs; i++ )
    {
        const ARC_DATA& d = test_data[i];

        SHAPE_ARC arc( VECTOR2D( Millimeter2iu( d.cx ), Millimeter2iu( d.cy ) ),
                       VECTOR2D( Millimeter2iu( d.sx ), Millimeter2iu( d.sy ) ), d.ca,
                       Millimeter2iu( d.w ) );

        arcs.push_back( arc );

        if( i == 0 )
            vp = arc.BBox();
        else
            vp.Merge( arc.BBox() );
    }

    printf("Read %lu arcs\n", arcs.size() );

    LABEL_MANAGER labelMgr( frame->GetPanel()->GetGAL() );
    frame->GetPanel()->GetView()->SetViewport( BOX2D( vp.GetOrigin(), vp.GetSize() ) );

    for(int i = 0; i < arcs.size(); i+= 2)
    {
        SEG closestDist;
        VECTOR2D ips[2];
        bool collides = collideArc2Arc( arcs[i], arcs[i+1], 0, closestDist );
        int ni = intersectArc2Arc( arcs[i], arcs[i+1], ips );

        overlay->SetLineWidth( 10000.0 );
        overlay->SetStrokeColor( CYAN );
        overlay->AnnotatedPoint( arcs[i].GetP0(), arcs[i].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i + 1].GetP0(), arcs[i + 1].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i].GetArcMid(), arcs[i].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i + 1].GetArcMid(), arcs[i + 1].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i].GetP1(), arcs[i].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i + 1].GetP1(), arcs[i + 1].GetWidth() / 2 );
        overlay->SetStrokeColor( GREEN );

        for(int j = 0; j < ni; j++ )
            overlay->AnnotatedPoint( ips[j], arcs[i].GetWidth() );

        if( collides )
        {
            overlay->SetStrokeColor( YELLOW );
            overlay->Line(closestDist.A, closestDist.B);
        }

        overlay->SetStrokeColor( RED );
        overlay->Arc( arcs[i] );
        overlay->SetStrokeColor( MAGENTA );
        overlay->Arc( arcs[i + 1] );
    }


    overlay = nullptr;

    return 0;
}

static bool registered = UTILITY_REGISTRY::Register( {
        "playground",
        "Geometry/drawing playground",
        playground_main_func,
} );
