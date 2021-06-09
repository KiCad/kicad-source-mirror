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

namespace PNS {
    extern SHAPE_LINE_CHAIN g_pnew, g_hnew;
};

std::shared_ptr<PNS_LOG_VIEWER_OVERLAY> overlay;


template<class T>
T within_range(T x, T minval, T maxval, T wrap)
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

bool sliceContainsPoint( SHAPE_ARC a, VECTOR2D p )
{
#if 1
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
#endif

    VECTOR2D vec( p - a.GetCenter() );

    bool   ccw = a.GetCentralAngle() > 0.0;
        double rotatedVecAngle = NormalizeAngleDegreesPos( NormalizeAngleDegreesPos( RAD2DEG( vec.Angle() ) )
                                           - a.GetStartAngle() );
        double rotatedEndAngle = NormalizeAngleDegreesPos( a.GetEndAngle() - a.GetStartAngle() );

        if( ( ccw && rotatedVecAngle > rotatedEndAngle )
            || ( !ccw && rotatedVecAngle < rotatedEndAngle ) )
        {
            return true;
        }

    return false;

}

int arclineIntersect( SHAPE_ARC a, SEG s, VECTOR2D* pts )
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

int arclineIntersect2( SHAPE_ARC a, SEG s, VECTOR2D* pts )
{
    double sa, sb, sc;

    sa = s.A.y - s.B.y;
    sb = s.B.x - s.A.x;
    sc = -sa * s.A.x - sb * s.A.y;

    VECTOR2D ac = a.GetCenter();
    double rr = a.GetRadius();
    int n = 0;

    overlay->SetStrokeColor( RED );
    overlay->AnnotatedPoint( s.A, 200000 );
    overlay->SetStrokeColor( GREEN );
    overlay->AnnotatedPoint( s.B, 200000 );
    overlay->SetStrokeColor( WHITE );
    overlay->Arc( a );

    if( sb != 0.0 )
    {
        double aa = sa * sa / ( sb * sb ) + 1.0;
        double bb = (2.0 * sa * ac.y / sb
                    + 2.0 * sa * sc / (sb * sb)
                    - 2.0 * ac.x );

        double cc = ac.x * ac.x + ac.y * ac.y - rr * rr + 2.0*sc*ac.y/sb + (sc*sc) / ( sb*sb );

        double delta = bb*bb - 4.0 * aa * cc;

        if( delta < 0.0 )
            return 0;
        else
        {
            double x1 = -bb - sqrt(delta) / (2.0 * aa );
            double x2 = -bb + sqrt(delta) / (2.0 * aa );

            double y1 = (-sc - sa * x1) / sb;
            double y2 = (-sc - sa * x2) / sb;

            overlay->AnnotatedPoint( VECTOR2D(x1, y1), 2000000 );
            overlay->AnnotatedPoint( VECTOR2D(x2, y2), 2000000 );


            VECTOR2D p0( x1, y1 );
            VECTOR2D p1( x1, y1 );
            if( sliceContainsPoint( a, p0 ) ) { pts[n] = p0; n++; }
            if( sliceContainsPoint( a, p1 ) ) { pts[n] = p1; n++; }

            printf("CASE1\n");

            return n;
        }
    } else { // line is vertical

        double aa = 1.0;
        double bb = -2.0 * ac.y;
        double cc = ac.x * ac.x + ac.y * ac.y - rr*rr + 2.0*sc*ac.x/sa + (sc * sc) / (sa * sa);

        double delta = bb*bb - 4.0 * aa * cc;

        if( delta < 0.0 )
            return 0;
        else
        {
            double y1 = -bb - sqrt(delta) / (2.0 * aa );
            double y2 = -bb + sqrt(delta) / (2.0 * aa );

            VECTOR2D p0( s.A.x, y1 );
            VECTOR2D p1( s.A.y, y2 );
            /*if( sliceContainsPoint( a, p0 ) ) */ { pts[n] = p0; n++; }
            /*if( sliceContainsPoint( a, p1 ) ) */ { pts[n] = p1; n++; }

            printf("CASE2\n");

            return n;
        }
    }

    return 0;
}

int intersectArc2Arc( SHAPE_ARC a1, SHAPE_ARC a2, VECTOR2D* ips )
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


  //  if( d<0.0 )
//        return 0;

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


bool collideArc2Arc( SHAPE_ARC a1, SHAPE_ARC a2, int clearance, SEG& minDistSeg )
{
    SEG mediatrix (a1.GetCenter(), a2.GetCenter() );

    int nA = 0, nB = 0;
    VECTOR2D ptsA[8];
    VECTOR2D ptsB[8];

    bool cocentered = (mediatrix.A == mediatrix.B);

    VECTOR2D ips[4];


    if( intersectArc2Arc(a1, a2, ips) > 0 )
    {
        minDistSeg.A = minDistSeg.B = ips[0];
        return true;
    }

        // arcs don't have the same center point
        if( ! cocentered )
        {
            nA = arclineIntersect( a1, mediatrix, ptsA );
            nB = arclineIntersect( a2, mediatrix, ptsB );
        }

        overlay->SetStrokeColor( LIGHTRED );
        overlay->Line( SEG( a2.GetP0(), a2.GetCenter() ) );
        overlay->Line( SEG( a2.GetP1(), a2.GetCenter() ) );

        nA += arclineIntersect( a1, SEG( a2.GetP0(), a2.GetCenter() ), ptsA + nA );
        nA += arclineIntersect( a1, SEG( a2.GetP1(), a2.GetCenter() ), ptsA + nA );


        overlay->SetStrokeColor( LIGHTBLUE );
        overlay->Line( SEG( a1.GetP0(), a1.GetCenter() ) );
        overlay->Line( SEG( a1.GetP1(), a1.GetCenter() ) );

        nB += arclineIntersect( a2, SEG( a1.GetP0(), a1.GetCenter() ), ptsB + nB );
        nB += arclineIntersect( a2, SEG( a1.GetP1(), a1.GetCenter() ), ptsB + nB );
    
    ptsA[nA++] = a1.GetP0();
    ptsA[nA++] = a1.GetP1();
    ptsB[nB++] = a2.GetP0();
    ptsB[nB++] = a2.GetP1();

    

//    printf("nA %d nB %d\n", nA, nB );

    double minDist;
    bool minDistFound = false;

    for( int i = 0; i < nA; i++ )
        for( int j = 0; j < nB; j++ )
        {
            double dist = (ptsA[i] - ptsB[j]).EuclideanNorm() - a1.GetWidth() / 2 - a2.GetWidth() / 2;

           /* overlay->SetStrokeColor( RED );
            overlay->AnnotatedPoint( ptsA[i], 100000 );
            overlay->SetStrokeColor( GREEN );
            overlay->AnnotatedPoint( ptsB[j], 100000 );*/


            //printf("dist %.1f\n", dist );

            if( dist < clearance )
            {

                if( !minDistFound )
                {
                    minDist = dist;
                    minDistSeg = SEG( ptsA[i], ptsB[j] );
                }
                else if( dist < minDist )
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

    overlay = frame->GetOverlay();

    
    overlay->SetIsFill(false);
    overlay->SetLineWidth(10000);

    //auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 111435489, 74692234), VECTOR2I( 111812234, 74315489), VECTOR2I( 112507766, 74315489), VECTOR2I( 112884511, 74692234), VECTOR2I( 112884511, 75387766), VECTOR2I( 112507766, 75764511), VECTOR2I( 111812234, 75764511), VECTOR2I( 111435489, 75387766)}, true );; 
    //auto path =  SHAPE_LINE_CHAIN( { VECTOR2I( 112609520, 74417243), VECTOR2I( 112609520, 73774520), VECTOR2I( 112670046, 73774520), VECTOR2I( 112999520, 73445046), VECTOR2I( 112999520, 72054954), VECTOR2I( 112670046, 71725480), VECTOR2I( 111654954, 71725480), VECTOR2I( 111325480, 72054954), VECTOR2I( 111325480, 73445046), VECTOR2I( 111654954, 73774520), VECTOR2I( 111710480, 73774520), VECTOR2I( 111710480, 75226197), VECTOR2I( 111973803, 75489520), VECTOR2I( 112346197, 75489520), VECTOR2I( 112609520, 75226197), VECTOR2I( 112609520, 74417243)}, false );;

// hull-cw-1
    //auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 111435489, 74692234), VECTOR2I( 111812234, 74315489), VECTOR2I( 112507766, 74315489), VECTOR2I( 112884511, 74692234), VECTOR2I( 112884511, 75387766), VECTOR2I( 112507766, 75764511), VECTOR2I( 111812234, 75764511), VECTOR2I( 111435489, 75387766)}, true );; 
// path-cw-1
    //auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 112609520, 74417243), VECTOR2I( 112609520, 75226197), VECTOR2I( 112346197, 75489520), VECTOR2I( 111973803, 75489520), VECTOR2I( 111710480, 75226197), VECTOR2I( 111710480, 72566303), VECTOR2I( 111973803, 72302980), VECTOR2I( 112346197, 72302980), VECTOR2I( 112609520, 72566303), VECTOR2I( 112609520, 74417243)}, false );; 
// hull
    auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 106035489, 85253452), VECTOR2I( 106313452, 84975489), VECTOR2I( 106706548, 84975489), VECTOR2I( 106984511, 85253452), VECTOR2I( 106984511, 85976548), VECTOR2I( 106706548, 86254511), VECTOR2I( 106313452, 86254511), VECTOR2I( 106035489, 85976548)}, true );; 
// path
    auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 101092000, 85246500), VECTOR2I( 101971211, 86125711), VECTOR2I( 106380778, 86125711), VECTOR2I( 106509572, 86254505)}, false );; 

    SHAPE_LINE_CHAIN path_w;

    std::vector<SHAPE_ARC> arcs;

    #if 0
    arcs.push_back (SHAPE_ARC ( VECTOR2I( 10000, 10000 ), VECTOR2I( -10000000, -500000 ), 50.0 ) );
    arcs.push_back (SHAPE_ARC ( VECTOR2I( 0, 600 ), VECTOR2I( 1000000, 300000 ), 180.0 ) );

    VECTOR2I offset( -3000000, 0 );

    arcs.push_back (SHAPE_ARC ( VECTOR2I( 10000, 10000 ) + offset, VECTOR2I( -550000, -500000 ) + offset, -80.0 ) );
    arcs.push_back (SHAPE_ARC ( VECTOR2I( -10000, 600 ) + offset, VECTOR2I( 550000 / 2, 500000/ 2 ) + offset , -80.0 ) );

    BOX2D bb ( arcs[0].BBox().GetPosition(), arcs[0].BBox().GetSize() );

    arcs.push_back (SHAPE_ARC ( VECTOR2I( 10000, 10000 ) + offset, VECTOR2I( -550000, -500000 ) + offset, 350.0 ) );
    arcs.push_back (SHAPE_ARC ( VECTOR2I( -100000, 600 ) + offset, VECTOR2I( 500000, 440000 ) + offset , 350.0 ) );
    #endif

    FILE *f = fopen("/tmp/arcs.txt","r");

    while( !feof(f) )
    {
        char str[1024];
        if( fgets(str, sizeof(str), f ) == NULL )
            break;

        if( str[0] == '#' )
            continue;

        printf("str '%s'\n", str);

        double sx, sy, ex, ey, a, w = 1000.0;
        if( sscanf(str,"%lf %lf %lf %lf %lf", &sx, &sy, &ex, &ey, &a) != 5 )
            break;


        
        SHAPE_ARC arc( VECTOR2D( Millimeter2iu( sx ), Millimeter2iu( sy ) ), VECTOR2D( Millimeter2iu( ex ), Millimeter2iu( ey ) ), a, w );

        arcs.push_back(arc);

    }
    fclose(f);

    printf("Read %d arcs\n", arcs.size() );

    LABEL_MANAGER labelMgr( frame->GetPanel()->GetGAL() );

    //frame->GetPanel()->GetView()->SetViewport(bb);

    overlay->SetStrokeColor( WHITE );
    overlay->SetLineWidth( 10000.0 );
    overlay->SetStrokeColor( WHITE );

    for(int i = 0; i < arcs.size(); i+= 2)
    {
        SEG closestDist;
        VECTOR2D ips[2];
        bool collides = collideArc2Arc( arcs[i], arcs[i+1], INT_MAX, closestDist );
        int ni = intersectArc2Arc( arcs[i], arcs[i+1], ips );

        overlay->SetStrokeColor( WHITE );
        overlay->Arc( arcs[i] );
        overlay->Arc( arcs[i+1] );

        overlay->SetStrokeColor( CYAN );
        overlay->AnnotatedPoint( arcs[i].GetArcMid(), 20000 );
        overlay->AnnotatedPoint( arcs[i+1].GetArcMid(), 20000 );
        overlay->SetStrokeColor( MAGENTA );

        for(int j = 0; j < ni; j++ )
            overlay->AnnotatedPoint( ips[j], 200000 );

        if( collides )
        {


            overlay->SetStrokeColor( YELLOW );
            overlay->Line(closestDist.A, closestDist.B);
        }

    }


    overlay = nullptr;

    return 0;
}

static bool registered = UTILITY_REGISTRY::Register( {
        "playground",
        "Geometry/drawing playground",
        playground_main_func,
} );
