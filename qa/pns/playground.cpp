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

bool arcContainsPoint( SHAPE_ARC a, VECTOR2D p )
{
    double phi = atan2( p.y - a.GetCenter().y, p.x - a.GetCenter().x );

    return within_range( phi, a.GetStartAngle(), a.GetEndAngle(), 2.0 * M_PI );
}

int arclineIntersect( SHAPE_ARC a, SEG s, VECTOR2D* pts )
{
    double sa, sb, sc;

    sa = s.A.y - s.B.y;
    sb = s.B.x - s.A.x;
    sc = -sa * s.A.x - sb * s.A.y;

    VECTOR2D ac = a.GetCenter();
    double rr = a.GetRadius();
    int n = 0;

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

            VECTOR2D p0( x1, y1 );
            VECTOR2D p1( x1, y1 );
            if( arcContainsPoint( a, p0 ) ) { pts[n] = p0; n++; }
            if( arcContainsPoint( a, p1 ) ) { pts[n] = p1; n++; }

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
            if( arcContainsPoint( a, p0 ) ) { pts[n] = p0; n++; }
            if( arcContainsPoint( a, p1 ) ) { pts[n] = p1; n++; }
            return n;
        }
    }

    return 0;
}

bool collideArc2Arc( SHAPE_ARC a1, SHAPE_ARC a2, int clearance, SEG& minDistSeg )
{
    SEG mediatrix (a1.GetCenter(), a2.GetCenter() );

    int nA = 0, nB = 0;
    VECTOR2D ptsA[4];
    VECTOR2D ptsB[4];

    if (mediatrix.A.x != mediatrix.B.x || mediatrix.A.y != mediatrix.B.y )
    {
        // arcs don't have the same center point
        nA = arclineIntersect( a1, mediatrix, ptsA );
        nB = arclineIntersect( a2, mediatrix, ptsB );
    }
    else
    {
        // well well - check if the start/end angle ranges overlap. If they do ,the minumum distance is at one of the arc endpoints
    }

    ptsA[nA++] = a1.GetP0();
    ptsA[nA++] = a1.GetP1();
    ptsB[nB++] = a2.GetP0();
    ptsB[nB++] = a2.GetP1();

    printf("nA %d nB %d\n", nA, nB );

    double minDist;
    bool minDistFound = false;

    for( int i = 0; i < nA; i++ )
        for( int j = 0; j < nB; j++ )
        {
            double dist = (ptsA[i] - ptsB[j]).EuclideanNorm() - a1.GetWidth() / 2 - a2.GetWidth() / 2;

            printf("dist %.1f\n", dist );

            if( dist < clearance )
            {

                if( !minDistFound )
                {
                    minDist = dist;
                    minDistSeg = SEG( ptsA[i], ptsB[j] );
                }
                else
                {
                    minDist = std::min( minDist, dist );
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

    auto overlay = frame->GetOverlay();

    
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

    SHAPE_ARC a( VECTOR2I( 10000, 10000 ), VECTOR2I( -10000000, -500000 ), M_PI*1.5 );
    BOX2D bb ( a.BBox().GetPosition(), a.BBox().GetSize() );
    SHAPE_ARC a2( VECTOR2I( -10000, 10000 ), VECTOR2I( -400000, -100000 ), M_PI*0.75 );
    
    LABEL_MANAGER labelMgr( frame->GetPanel()->GetGAL() );

    frame->GetPanel()->GetView()->SetViewport(bb);

    PNS::LINE l;
    l.SetShape( path );
    l.Walkaround( hull, path_w, true );

    overlay->SetStrokeColor( WHITE );
    overlay->SetLineWidth( 10000.0 );
 
 #if 0
    overlay->AnnotatedPolyline( PNS::g_pnew, "path", true );
    overlay->SetStrokeColor( RED );
    overlay->AnnotatedPolyline( PNS::g_hnew, "hull", true );
    overlay->SetLineWidth( 20000.0 );
    overlay->SetStrokeColor( YELLOW );
    overlay->AnnotatedPolyline( path_w, "walk", "true" );
    overlay->DrawAnnotations();
#endif

    SEG s1 ( -1000000, -200000, 3000000, 200000 );
    SEG s2 ( 0, -200000, 0, 200000 );

    VECTOR2D pts1[2], pts2[2];
//    int n1 = arclineIntersect( a, s1, pts1 );
    //int n2 = arclineIntersect( a, s2, pts2 );

    SEG closestDist;

    bool isects = collideArc2Arc( a, a2, 100000000, closestDist );


    overlay->SetStrokeColor( WHITE );
    overlay->Arc( a.GetCenter(), a.GetRadius(), a.GetStartAngle(), a.GetEndAngle() );
    overlay->Arc( a2.GetCenter(), a2.GetRadius(), a2.GetStartAngle(), a2.GetEndAngle() );

    /*overlay->SetStrokeColor( RED );
    overlay->Line( s1.A, s1.B );
    for(int i = 0; i < n1; i++)
        overlay->Circle( pts1[i], 200000 );
    
    overlay->SetStrokeColor( GREEN );
    overlay->Line( s2.A, s2.B );
    for(int i = 0; i < n2; i++)
        overlay->Circle( pts2[i], 200000 );*/

    if( isects )
    {
        overlay->SetStrokeColor( YELLOW );
        overlay->Line(closestDist.A, closestDist.B);
    }
    
    overlay = nullptr;

    return 0;
}

static bool registered = UTILITY_REGISTRY::Register( {
        "playground",
        "Geometry/drawing playground",
        playground_main_func,
} );
