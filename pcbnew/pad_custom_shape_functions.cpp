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
#include <class_board.h>
#include <class_board_item.h>
#include <class_drawsegment.h>
#include <class_pad.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_rect.h>


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
    DRAWSEGMENT* item = new DRAWSEGMENT();
    item->SetShape( S_POLYGON );
    item->SetPolyPoints( aPoly );
    item->SetWidth( aThickness );
    m_editPrimitives.emplace_back( item );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveSegment( const wxPoint& aStart, const wxPoint& aEnd, int aThickness )
{
    DRAWSEGMENT* item = new DRAWSEGMENT();
    item->SetStart( aStart );
    item->SetEnd( aEnd );
    item->SetWidth( aThickness );
    m_editPrimitives.emplace_back( item );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveArc( const wxPoint& aCenter, const wxPoint& aStart, int aArcAngle,
                             int aThickness )
{
    DRAWSEGMENT* item = new DRAWSEGMENT();
    item->SetShape( S_ARC );
    item->SetCenter( aCenter );
    item->SetArcStart( aStart );
    item->SetAngle( aArcAngle );
    item->SetWidth( aThickness );
    m_editPrimitives.emplace_back( item );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveCurve( const wxPoint& aStart, const wxPoint& aEnd, const wxPoint& aCtrl1,
                               const wxPoint& aCtrl2, int aThickness )
{
    DRAWSEGMENT* item = new DRAWSEGMENT();
    item->SetShape( S_CURVE );
    item->SetStart( aStart );
    item->SetEnd( aEnd );
    item->SetBezControl1( aCtrl1 );
    item->SetBezControl2( aCtrl2 );
    item->SetWidth( aThickness );
    m_editPrimitives.emplace_back( item );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveCircle( const wxPoint& aCenter, int aRadius, int aThickness )
{
    DRAWSEGMENT* item = new DRAWSEGMENT();
    item->SetShape( S_CIRCLE );
    item->SetStart( aCenter );
    item->SetEnd( wxPoint( aCenter.x + aRadius, aCenter.y ) );
    item->SetWidth( aThickness );
    m_editPrimitives.emplace_back( item );
    m_shapesDirty = true;
}


void D_PAD::AddPrimitiveRect( const wxPoint& aStart, const wxPoint& aEnd, int aThickness )
{
    DRAWSEGMENT* item = new DRAWSEGMENT();
    item->SetShape( S_RECT );
    item->SetStart( aStart );
    item->SetEnd( aEnd );
    item->SetWidth( aThickness );
    m_editPrimitives.emplace_back( item );
    m_shapesDirty = true;
}


void D_PAD::SetPrimitives( const std::vector<std::shared_ptr<DRAWSEGMENT>>& aPrimitivesList )
{
    // clear old list
    m_editPrimitives.clear();

    // Import to the basic shape list
    if( aPrimitivesList.size() )
        m_editPrimitives = aPrimitivesList;

    m_shapesDirty = true;
}


void D_PAD::AddPrimitives( const std::vector<std::shared_ptr<DRAWSEGMENT>>& aPrimitivesList )
{
    for( const std::shared_ptr<DRAWSEGMENT>& prim : aPrimitivesList )
        m_editPrimitives.push_back( prim );

    m_shapesDirty = true;
}


void D_PAD::AddPrimitive( DRAWSEGMENT* aPrimitive )
{
    m_editPrimitives.emplace_back( aPrimitive );

    m_shapesDirty = true;
}


// clear the basic shapes list and associated data
void D_PAD::DeletePrimitivesList()
{
    m_editPrimitives.clear();
    m_shapesDirty = true;
}


void D_PAD::addPadPrimitivesToPolygon( SHAPE_POLY_SET* aMergedPolygon, int aError ) const
{
    SHAPE_POLY_SET polyset;

    for( const std::shared_ptr<DRAWSEGMENT>& primitive : m_editPrimitives )
        primitive->TransformShapeWithClearanceToPolygon( polyset, 0, aError );

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

    addPadPrimitivesToPolygon( aMergedPolygon, maxError );
}


bool D_PAD::GetBestAnchorPosition( VECTOR2I& aPos )
{
    SHAPE_POLY_SET poly;
    addPadPrimitivesToPolygon( &poly, ARC_LOW_DEF );

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
