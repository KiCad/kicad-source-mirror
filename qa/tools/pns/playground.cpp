/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <pad.h>

/**
 * Flag to enable PNS playground debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar tracePnsPlayground[] = wxT( "KICAD_PNS_PLAYGROUND" );


std::shared_ptr<PNS_LOG_VIEWER_OVERLAY> overlay;


static inline bool collide( const SHAPE_LINE_CHAIN& aLhs, const SHAPE_LINE_CHAIN& aRhs,
                            int aClearance, int* aDistance = nullptr, VECTOR2I* aPt1 = nullptr )
{
    wxCHECK( aLhs.PointCount() && aRhs.PointCount(), false );

    VECTOR2I pt1;
    bool retv = false;
    int dist = std::numeric_limits<int>::max();
    int tmp = dist;

    SHAPE_LINE_CHAIN lhs( aLhs );
    SHAPE_LINE_CHAIN rhs( aRhs );

    lhs.SetClosed( false );
    lhs.Append( lhs.CPoint( 0 ) );
    rhs.SetClosed( false );
    rhs.Append( rhs.CPoint( 0 ) );

    for( int i = 0; i < rhs.SegmentCount(); i ++ )
    {
        if( lhs.Collide( rhs.CSegment( i ), tmp, &tmp, &pt1 ) )
        {
            retv = true;

            if( tmp < dist )
                dist = tmp;

            if( aDistance )
                *aDistance = dist;

            if( aPt1 )
                *aPt1 = pt1;
        }
    }

    return retv;
}


static bool collide( const SHAPE_POLY_SET& aLhs, const SHAPE_LINE_CHAIN& aRhs, int aClearance,
                     int* aDistance = nullptr, VECTOR2I* aPt1 = nullptr )
{
    VECTOR2I pt1;
    bool retv = false;
    int tmp = std::numeric_limits<int>::max();
    int dist = tmp;

    for( int i = 0; i < aLhs.OutlineCount(); i++ )
    {
        if( collide( aLhs.Outline( i ), aRhs, aClearance, &tmp, &pt1 ) )
        {
            retv = true;

            if( tmp < dist )
            {
                dist = tmp;

                if( aDistance )
                    *aDistance = dist;

                if( aPt1 )
                    *aPt1 = pt1;
            }
        }

        for( int j = 0; j < aLhs.HoleCount( i ); i++ )
        {
            if( collide( aLhs.CHole( i, j ), aRhs, aClearance, &tmp, &pt1 ) )
            {
                retv = true;

                if( tmp < dist )
                {
                    dist = tmp;

                    if( aDistance )
                        *aDistance = dist;

                    if( aPt1 )
                        *aPt1 = pt1;
                }
            }
        }
    }

    return retv;
}


bool collideArc2Arc( const SHAPE_ARC& a1, const SHAPE_ARC& a2, int clearance, SEG& minDistSeg )
{
    SEG mediatrix( a1.GetCenter(), a2.GetCenter() );

    std::vector<VECTOR2I> ips;

    // Basic case - arcs intersect
    if( a1.Intersect( a2, &ips ) > 0 )
    {
        minDistSeg.A = minDistSeg.B = ips[0];
        return true;
    }

    // Arcs don't intersect, build a list of points to check
    std::vector<VECTOR2I> ptsA;
    std::vector<VECTOR2I> ptsB;

    bool cocentered = ( mediatrix.A == mediatrix.B );

    // 1: Interior points of both arcs, which are on the line segment between the two centres
    if( !cocentered )
    {
        a1.IntersectLine( mediatrix, &ptsA );
        a2.IntersectLine( mediatrix, &ptsB );
    }

    // 2: Check arc end points
    ptsA.push_back( a1.GetP0() );
    ptsA.push_back( a1.GetP1() );
    ptsB.push_back( a2.GetP0() );
    ptsB.push_back( a2.GetP1() );

    // 3: Endpoint of one and "projected" point on the other, which is on the
    // line segment through that endpoint and the centre of the other arc
    a1.IntersectLine( SEG( a2.GetP0(), a1.GetCenter() ), &ptsA );
    a1.IntersectLine( SEG( a2.GetP1(), a1.GetCenter() ), &ptsA );

    a2.IntersectLine( SEG( a1.GetP0(), a2.GetCenter() ), &ptsB );
    a2.IntersectLine( SEG( a1.GetP1(), a2.GetCenter() ), &ptsB );

    double minDist = std::numeric_limits<double>::max();
    bool   minDistFound = false;

    // @todo performance might be improved by only checking certain points (e.g only check end
    // points against other end points or their corresponding "projected" points)
    for( const VECTOR2I& ptA : ptsA )
    {
        for( const VECTOR2I& ptB : ptsB )
        {
            double dist = ( ptA - ptB ).EuclideanNorm() - a1.GetWidth() / 2.0 - a2.GetWidth() / 2.0;

            if( dist < clearance )
            {
                if( !minDistFound || dist < minDist )
                {
                    minDist = dist;
                    minDistSeg = SEG( ptA, ptB );
                }

                minDistFound = true;
            }
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
        {94.6551, 88.295989, 95.6551, 88.295989, 90.0, 0.2 }, // simulating diff pair
        {94.6551, 88.295989, 95.8551, 88.295989, 90.0, 0.2 },
        {73.77532, 93.413654, 75.70532, 93.883054, 60.0, 0.1 }, // one arc fully enclosed in other
        {73.86532, 93.393054, 75.86532, 93.393054, 90.0, 0.3 },
        {79.87532, 93.413654, 81.64532, 94.113054, 60.0, 0.1 }, // concentric
        {79.87532, 93.413654, 81.86532, 93.393054, 90.0, 0.3 }
    };


    overlay = frame->GetOverlay();


    overlay->SetIsFill( false );
    overlay->SetLineWidth( 10000 );

    std::vector<SHAPE_ARC> arcs;
    int n_arcs = sizeof( test_data ) / sizeof( ARC_DATA );

    BOX2I vp;

    for( int i = 0; i < n_arcs; i++ )
    {
        const ARC_DATA& d = test_data[i];

        SHAPE_ARC arc( VECTOR2D( pcbIUScale.mmToIU( d.cx ), pcbIUScale.mmToIU( d.cy ) ),
                       VECTOR2D( pcbIUScale.mmToIU( d.sx ), pcbIUScale.mmToIU( d.sy ) ),
                       EDA_ANGLE( d.ca, DEGREES_T ), pcbIUScale.mmToIU( d.w ) );

        arcs.push_back( arc );

        if( i == 0 )
            vp = arc.BBox();
        else
            vp.Merge( arc.BBox() );
    }


    PAD pad1( nullptr );
    pad1.SetX( pcbIUScale.mmToIU( 84.0 ) );
    pad1.SetY( pcbIUScale.mmToIU( 66.0 ) );
    pad1.SetSizeX( pcbIUScale.mmToIU( 7.0 ) );
    pad1.SetSizeY( pcbIUScale.mmToIU( 7.0 ) );
    pad1.SetDrillSizeX( pcbIUScale.mmToIU( 3.5 ) );
    pad1.SetDrillSizeY( pcbIUScale.mmToIU( 3.5 ) );
    vp.Merge( pad1.GetBoundingBox() );

    PAD pad2( nullptr );
    pad2.SetX( pcbIUScale.mmToIU( 87.125 ) );
    pad2.SetY( pcbIUScale.mmToIU( 66.0 ) );
    pad2.SetSizeX( pcbIUScale.mmToIU( 0.8 ) );
    pad2.SetSizeY( pcbIUScale.mmToIU( 0.8 ) );
    pad2.SetDrillSizeX( pcbIUScale.mmToIU( 0.5 ) );
    pad2.SetDrillSizeY( pcbIUScale.mmToIU( 0.5 ) );
    vp.Merge( pad2.GetBoundingBox() );

    SHAPE_POLY_SET pad1Outline;
    SHAPE_POLY_SET pad1Hole;

    pad1.TransformShapeToPolygon( pad1Outline, UNDEFINED_LAYER, 0, 4, ERROR_OUTSIDE );
    pad1.TransformHoleToPolygon( pad1Hole, 0, 4, ERROR_INSIDE );
    pad1Outline.AddHole( pad1Hole.Outline( 0 ), 0 );

    SHAPE_POLY_SET pad2Outline;

    // pad2.TransformShapeToPolygon( pad2Outline, UNDEFINED_LAYER, 0, 4, ERROR_OUTSIDE );
    pad2.TransformHoleToPolygon( pad2Outline, 0, 4, ERROR_INSIDE );

    SHAPE_POLY_SET xorPad1ToPad2 = pad1Outline;

    xorPad1ToPad2.BooleanXor( pad2Outline );
    xorPad1ToPad2.Move( VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ) );

    SHAPE_POLY_SET andPad1ToPad2 = pad2Outline;

    andPad1ToPad2.BooleanIntersection( pad1Outline );
    andPad1ToPad2.Move( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ) );

    std::shared_ptr<SHAPE_SEGMENT> slot = pad2.GetEffectiveHoleShape();

    printf("Read %zu arcs\n", arcs.size() );

    LABEL_MANAGER labelMgr( frame->GetPanel()->GetGAL() );
    frame->GetPanel()->GetView()->SetViewport( BOX2D( vp.GetOrigin(), vp.GetSize() ) );

    for( int i = 0; i < arcs.size(); i+= 2 )
    {
        SEG closestDist;
        std::vector<VECTOR2I> ips;
        bool collides = collideArc2Arc( arcs[i], arcs[i+1], 0, closestDist );
        int ni = arcs[i].Intersect( arcs[i+1], &ips );

        overlay->SetLineWidth( 10000.0 );
        overlay->SetStrokeColor( GREEN );

        for( int j = 0; j < ni; j++ )
            overlay->AnnotatedPoint( ips[j], arcs[i].GetWidth() );

        if( collides )
        {
            overlay->SetStrokeColor( YELLOW );
            overlay->Line( closestDist.A, closestDist.B );
            overlay->SetLineWidth( 10000 );
            overlay->SetGlyphSize( { 100000, 100000 } );
            overlay->BitmapText( wxString::Format( "dist=%d", closestDist.Length() ),
                                 closestDist.A + VECTOR2I( 0, -arcs[i].GetWidth() ),
                                 ANGLE_HORIZONTAL );
        }

        overlay->SetLineWidth( 10000 );
        overlay->SetStrokeColor( CYAN );
        overlay->AnnotatedPoint( arcs[i].GetP0(), arcs[i].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i + 1].GetP0(), arcs[i + 1].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i].GetArcMid(), arcs[i].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i + 1].GetArcMid(), arcs[i + 1].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i].GetP1(), arcs[i].GetWidth() / 2 );
        overlay->AnnotatedPoint( arcs[i + 1].GetP1(), arcs[i + 1].GetWidth() / 2 );


        overlay->SetStrokeColor( RED );
        overlay->Arc( arcs[i] );
        overlay->SetStrokeColor( MAGENTA );
        overlay->Arc( arcs[i + 1] );
    }

    overlay->SetLineWidth( 2000 );
    overlay->SetStrokeColor( CYAN );
    overlay->AnnotatedPolyset( pad1Outline, "Raw Pads" );
    overlay->SetStrokeColor( RED );
    overlay->AnnotatedPolyset( pad2Outline );
    overlay->SetStrokeColor( CYAN );
    overlay->AnnotatedPolyset( xorPad1ToPad2, "XOR Pads" );
    overlay->AnnotatedPolyset( andPad1ToPad2, "AND Pads" );

    wxLogTrace( tracePnsPlayground, wxS( "Pad 1 has %d outlines." ), pad1Outline.OutlineCount() );

    wxLogTrace( tracePnsPlayground, wxS( "Pad 2 has %d outlines." ), pad2Outline.OutlineCount() );

    VECTOR2I pt1, pt2;
    int dist = std::numeric_limits<int>::max();

    collide( pad1Outline, pad2Outline.Outline( 0 ), dist, &dist, &pt1 );

    wxLogTrace( tracePnsPlayground, wxS( "Nearest distance between pad 1 and pad 2 is %0.6f mm "
                                         "at X=%0.6f mm,  Y=%0.6f mm." ),
                 pcbIUScale.IUTomm( dist ), pcbIUScale.IUTomm( pt1.x ),
                 pcbIUScale.IUTomm( pt1.y ) );

    overlay->SetStrokeColor( YELLOW );
    overlay->SetGlyphSize( { 100000, 100000 } );
    overlay->BitmapText( wxString::Format( "dist=%0.3f mm", pcbIUScale.IUTomm( dist ) ),
                         pt1 + VECTOR2I( 0, -56000 ), ANGLE_HORIZONTAL );

    overlay = nullptr;

    return 0;
}



int drawShapes( int argc, char* argv[] )
{
    SHAPE_ARC arc( VECTOR2I( 206000000, 140110000 ), VECTOR2I( 201574617, 139229737 ),
                   VECTOR2I( 197822958, 136722959 ), 250000 );

    SHAPE_LINE_CHAIN lc( { /* VECTOR2I( 159600000, 142500000 ), VECTOR2I( 159600000, 142600000 ),
                           VECTOR2I( 166400000, 135800000 ), VECTOR2I( 166400000, 111600000 ),
                           VECTOR2I( 190576804, 111600000 ), VECTOR2I( 192242284, 113265480 ),
                           VECTOR2I( 192255720, 113265480 ),*/
                           VECTOR2I( 203682188, 124691948 ), VECTOR2I( 203682188, 140332188 ),
                           /* VECTOR2I( 206000000, 142650000 ) */ },
                         false );

    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );
    Pgm().App().SetTopWindow( frame ); // wxApp gets a face.
    frame->Show();

    overlay = frame->GetOverlay();


    overlay->SetIsFill( false );
    overlay->SetLineWidth( arc.GetWidth() );
    overlay->SetStrokeColor( RED );
    overlay->Arc( arc );
    overlay->SetLineWidth( arc.GetWidth() / 20 );
    overlay->SetStrokeColor( GREEN );
    overlay->Polyline( lc );

    overlay->SetLineWidth( 80000.0 );
    overlay->SetStrokeColor( CYAN );

    for( int i = 0; i < lc.PointCount(); ++i )
    {
        int mult = ( i % 2 ) ? 1 : -1;
        overlay->AnnotatedPoint( lc.GetPoint( i ), arc.GetWidth() * 2 );
        overlay->SetGlyphSize( { 800000, 800000 } );
        overlay->BitmapText( wxString::Format( "x=%d, y=%d",
                                               lc.GetPoint( i ).x,
                                               lc.GetPoint( i ).y ),
                                               lc.GetPoint( i ) + VECTOR2I( 0, mult*arc.GetWidth() * 4 ),
                                               ANGLE_HORIZONTAL );
    }

    arc.Collide( &lc, 100000 );

    BOX2I vp = arc.BBox();
    vp.Merge( lc.BBox() );
    vp.Inflate( (800000 + arc.GetWidth() * 4 )*2);
    frame->GetPanel()->GetView()->SetViewport( BOX2D( vp.GetOrigin(), vp.GetSize() ) );

    overlay = nullptr;

    return 0;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "playground",
        "Geometry/drawing playground",
        playground_main_func,
} );


static bool registered1 = UTILITY_REGISTRY::Register( {
        "drawShapes",
        "drawing shapes",
        drawShapes,
} );
