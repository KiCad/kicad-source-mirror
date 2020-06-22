/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_pad_custom_shape_functions.cpp
 * D_PAD functions specific to custom shaped pads.
 */

#include <fctsys.h>
#include <trigo.h>

#include <pcbnew.h>

#include <bezier_curves.h>
#include <class_board.h>
#include <class_board_item.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <class_pad.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/convex_hull.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_rect.h>


void PAD_CS_PRIMITIVE::ExportTo( DRAWSEGMENT* aTarget )
{
    aTarget->SetShape( m_Shape );
    aTarget->SetWidth( m_Thickness );
    aTarget->SetStart( m_Start );
    aTarget->SetEnd( m_End );
    aTarget->SetBezControl1( m_Ctrl1 );
    aTarget->SetBezControl2( m_Ctrl2 );

    // in a DRAWSEGMENT the radius of a circle is calculated from the
    // center and one point on the circle outline (stored in m_End)
    if( m_Shape == S_CIRCLE )
    {
        wxPoint end = m_Start;
        end.x += m_Radius;
        aTarget->SetEnd( end );
    }

    aTarget->SetAngle( m_ArcAngle );
    aTarget->SetPolyPoints( m_Poly );
}


void PAD_CS_PRIMITIVE::ExportTo( EDGE_MODULE* aTarget )
{
    ExportTo( static_cast<DRAWSEGMENT*>( aTarget ) );
    // Initialize coordinates specific to the EDGE_MODULE (m_Start0 and m_End0)
    aTarget->SetLocalCoord();
}


void PAD_CS_PRIMITIVE::Move( wxPoint aMoveVector )
{
    m_Start += aMoveVector;
    m_End   += aMoveVector;
    m_Ctrl1 += aMoveVector;
    m_Ctrl2 += aMoveVector;

    for( auto& corner : m_Poly )
        corner += aMoveVector;
}


void PAD_CS_PRIMITIVE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    switch( m_Shape )
    {
    case S_ARC:
    case S_SEGMENT:
    case S_CIRCLE:
        // these can all be done by just rotating the start and end points
        RotatePoint( &m_Start, aRotCentre, aAngle );
        RotatePoint( &m_End, aRotCentre, aAngle );
        break;

    case S_RECT:
        if( KiROUND( aAngle ) % 900 == 0 )
        {
            RotatePoint( &m_Start, aRotCentre, aAngle );
            RotatePoint( &m_End, aRotCentre, aAngle );
            break;
        }

        // Convert non-cartesian-rotated rect to a diamond
        m_Shape = S_POLYGON;

        m_Poly.clear();
        m_Poly.emplace_back( m_Start );
        m_Poly.emplace_back( m_End.x, m_Start.y );
        m_Poly.emplace_back( m_End );
        m_Poly.emplace_back( m_Start.x, m_End.y );

        KI_FALLTHROUGH;

    case S_POLYGON:
        for( auto& pt : m_Poly )
            RotatePoint( &pt, aRotCentre, aAngle );

        break;

    case S_CURVE:
        RotatePoint( &m_Start, aRotCentre, aAngle );
        RotatePoint( &m_End, aRotCentre, aAngle );
        RotatePoint( &m_Ctrl1, aRotCentre, aAngle );
        RotatePoint( &m_Ctrl2, aRotCentre, aAngle );
        break;

    default:
        // un-handled edge transform
        wxASSERT_MSG( false, wxT( "PAD_CS_PRIMITIVE::Rotate not implemented for "
                                     + BOARD_ITEM::ShowShape( m_Shape ) ) );
        break;
    }
}


/*
 * Has meaning only for free shape pads.
 * add a free shape to the shape list.
 * the shape is a polygon (can be with thick outline), segment, circle or arc
 */

void D_PAD::AddPrimitivePoly( const SHAPE_POLY_SET& aPoly, int aThickness )
{
    std::vector<wxPoint> points;

    // If aPoly has holes, convert it to a polygon with no holes.
    SHAPE_POLY_SET poly_no_hole;
    poly_no_hole.Append( aPoly );
    poly_no_hole.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    for( auto iter = poly_no_hole.CIterate(); iter; iter++ )
        points.emplace_back( iter->x, iter->y );

    AddPrimitivePoly( points, aThickness );
}


void D_PAD::AddPrimitivePoly( const std::vector<wxPoint>& aPoly, int aThickness )
{
    PAD_CS_PRIMITIVE shape( S_POLYGON );
    shape.m_Poly = aPoly;
    shape.m_Thickness = aThickness;
    m_basicShapes.push_back( shape );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveSegment( const wxPoint& aStart, const wxPoint& aEnd, int aThickness )
{
    PAD_CS_PRIMITIVE shape( S_SEGMENT );
    shape.m_Start = aStart;
    shape.m_End = aEnd;
    shape.m_Thickness = aThickness;
    m_basicShapes.push_back( shape );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveArc( const wxPoint& aCenter, const wxPoint& aStart, int aArcAngle,
                             int aThickness )
{
    PAD_CS_PRIMITIVE shape( S_ARC );
    shape.m_Start = aCenter;
    shape.m_End = aStart;
    shape.m_ArcAngle = aArcAngle;
    shape.m_Thickness = aThickness;
    m_basicShapes.push_back( shape );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveCurve( const wxPoint& aStart, const wxPoint& aEnd, const wxPoint& aCtrl1,
                               const wxPoint& aCtrl2, int aThickness )
{
    PAD_CS_PRIMITIVE shape( S_CURVE );
    shape.m_Start = aStart;
    shape.m_End = aEnd;
    shape.m_Ctrl1 = aCtrl1;
    shape.m_Ctrl2 = aCtrl2;
    shape.m_Thickness = aThickness;
    m_basicShapes.push_back( shape );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveCircle( const wxPoint& aCenter, int aRadius, int aThickness )
{
    PAD_CS_PRIMITIVE shape( S_CIRCLE );
    shape.m_Start = aCenter;
    shape.m_Radius = aRadius;
    shape.m_Thickness = aThickness;
    m_basicShapes.push_back( shape );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveRect( const wxPoint& aStart, const wxPoint& aEnd, int aThickness )
{
    PAD_CS_PRIMITIVE shape( S_RECT );
    shape.m_Start = aStart;
    shape.m_End = aEnd;
    shape.m_Thickness = aThickness;
    m_basicShapes.push_back( shape );
    m_shapesDirty = true;
}


void D_PAD::SetPrimitives( const std::vector<PAD_CS_PRIMITIVE>& aPrimitivesList )
{
    // clear old list
    m_basicShapes.clear();

    // Import to the basic shape list
    if( aPrimitivesList.size() )
        m_basicShapes = aPrimitivesList;

    m_shapesDirty = true;
}


void D_PAD::AddPrimitives( const std::vector<PAD_CS_PRIMITIVE>& aPrimitivesList )
{
    for( const auto& prim : aPrimitivesList )
        m_basicShapes.push_back( prim );

    m_shapesDirty = true;
}


// clear the basic shapes list and associated data
void D_PAD::DeletePrimitivesList()
{
    m_basicShapes.clear();
    m_shapesDirty = true;
}


void D_PAD::addCustomPadPrimitivesToPolygon( SHAPE_POLY_SET* aMergedPolygon, int aError ) const
{
    SHAPE_POLY_SET polyset;

    for( const PAD_CS_PRIMITIVE& bshape : m_basicShapes )
    {
        switch( bshape.m_Shape )
        {
        case S_CURVE:
        {
            std::vector<wxPoint> ctrlPoints = { bshape.m_Start, bshape.m_Ctrl1, bshape.m_Ctrl2,
                                                bshape.m_End };
            BEZIER_POLY converter( ctrlPoints );
            std::vector< wxPoint> poly;
            converter.GetPoly( poly, bshape.m_Thickness );

            for( unsigned ii = 1; ii < poly.size(); ii++ )
            {
                TransformSegmentToPolygon( polyset, poly[ ii - 1 ], poly[ ii ], aError,
                                           bshape.m_Thickness );
            }
            break;
        }

        case S_SEGMENT:         // usual segment : line with rounded ends
        {
            TransformSegmentToPolygon( polyset, bshape.m_Start, bshape.m_End, aError,
                                       bshape.m_Thickness );
            break;
        }

        case S_ARC:             // Arc with rounded ends
        {
            TransformArcToPolygon( polyset, bshape.m_Start, bshape.m_End, bshape.m_ArcAngle,
                                   aError, bshape.m_Thickness );
            break;
        }

        case S_CIRCLE:          //  ring or circle
        {
            if( bshape.m_Thickness )    // ring
                TransformRingToPolygon( polyset, bshape.m_Start, bshape.m_Radius, aError,
                                        bshape.m_Thickness );
            else                // Filled circle
                TransformCircleToPolygon( polyset, bshape.m_Start, bshape.m_Radius, aError );
            break;
        }

        case S_RECT:
        case S_POLYGON:         // polygon
        {
            SHAPE_POLY_SET poly;
            poly.NewOutline();

            if( bshape.m_Shape == S_RECT )
            {
                poly.Append( bshape.m_Start );
                poly.Append( bshape.m_End.x, bshape.m_Start.y );
                poly.Append( bshape.m_End );
                poly.Append( bshape.m_Start.x, bshape.m_End.y );
            }
            else
            {
                for( const wxPoint& pt : bshape.m_Poly )
                    poly.Append( pt );
            }

            if( bshape.m_Thickness )
            {
                int numSegs = std::max( GetArcToSegmentCount( bshape.m_Thickness / 2, aError, 360.0 ), 6 );
                poly.Inflate( bshape.m_Thickness / 2, numSegs );
            }

            // Insert the polygon:
            polyset.NewOutline();
            polyset.Append( poly );
        }
            break;

        default:
            // un-handled primitive
            wxASSERT_MSG( false, "D_PAD::addCustomPadPrimitivesToPolygon not implemented for "
                                 + BOARD_ITEM::ShowShape( bshape.m_Shape ) );
            break;
        }
    }

    polyset.Simplify( SHAPE_POLY_SET::PM_FAST );

    // Merge all polygons with the initial pad anchor shape
    if( polyset.OutlineCount() )
    {
        aMergedPolygon->BooleanAdd( polyset, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
        aMergedPolygon->Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    }
}

void D_PAD::MergePrimitivesAsPolygon( SHAPE_POLY_SET* aMergedPolygon ) const
{
    auto board = GetBoard();
    int maxError = ARC_HIGH_DEF;

    if( board )
        maxError = board->GetDesignSettings().m_MaxError;

    aMergedPolygon->RemoveAllContours();

    // Add the anchor pad shape in aMergedPolygon, others in aux_polyset:
    // The anchor pad is always at 0,0
    switch( GetAnchorPadShape() )
    {
    case PAD_SHAPE_RECT:
    {
        SHAPE_RECT rect( -GetSize().x / 2, -GetSize().y / 2, GetSize().x, GetSize().y );
        aMergedPolygon->AddOutline( rect.Outline() );
    }
        break;

    default:
    case PAD_SHAPE_CIRCLE:
        TransformCircleToPolygon( *aMergedPolygon, wxPoint( 0, 0 ), GetSize().x / 2, maxError );
        break;
    }

    addCustomPadPrimitivesToPolygon( aMergedPolygon, maxError );
}


bool D_PAD::GetBestAnchorPosition( VECTOR2I& aPos )
{
    SHAPE_POLY_SET poly;
    addCustomPadPrimitivesToPolygon( &poly, ARC_LOW_DEF );

    if( poly.OutlineCount() > 1 )
        return false;

    const int minSteps = 10;
    const int maxSteps = 50;

    int stepsX, stepsY;

    auto bbox = poly.BBox();

    if( bbox.GetWidth() < bbox.GetHeight() )
    {
        stepsX = minSteps;
        stepsY = minSteps * (double) bbox.GetHeight() / (double )(bbox.GetWidth() + 1);
    }
    else
    {
        stepsY = minSteps;
        stepsX = minSteps * (double) bbox.GetWidth() / (double )(bbox.GetHeight() + 1);
    }

    stepsX = std::max(minSteps, std::min( maxSteps, stepsX ) );
    stepsY = std::max(minSteps, std::min( maxSteps, stepsY ) );

    VECTOR2I center = bbox.Centre();

    int64_t minDist = std::numeric_limits<int64_t>::max();
    int64_t minDistEdge;

    if( GetAnchorPadShape() == PAD_SHAPE_CIRCLE )
    {
        minDistEdge = GetSize().x;
    }
    else
    {
        minDistEdge = std::max( GetSize().x, GetSize().y );
    }

    OPT<VECTOR2I> bestAnchor( []()->OPT<VECTOR2I> { return NULLOPT; }() );

    for( int y = 0; y < stepsY ; y++ )
    {
        for( int x = 0; x < stepsX; x++ )
        {
            VECTOR2I p = bbox.GetPosition();
            p.x += rescale( x, bbox.GetWidth(), (stepsX - 1) );
            p.y += rescale( y, bbox.GetHeight(), (stepsY - 1) );

            if( poly.Contains(p) )
            {

                int dist = (center - p).EuclideanNorm();
                int distEdge = poly.COutline(0).Distance( p, true );

                if( distEdge >= minDistEdge )
                {
                    if( dist < minDist )
                    {
                        bestAnchor = p;
                        minDist = dist;
                    }
                }
            }
        }
    }

    if( bestAnchor )
    {
        aPos = *bestAnchor;
        return true;
    }

    return false;
}
