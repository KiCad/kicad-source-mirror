/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <plotter.h>
#include <trigo.h>
#include <bezier_curves.h>
#include <base_units.h>
#include <eda_draw_frame.h>
#include <widgets/msgpanel.h>
#include <eda_draw_frame.h>
#include <general.h>
#include <lib_bezier.h>
#include <transform.h>
#include <settings/color_settings.h>


LIB_BEZIER::LIB_BEZIER( LIB_SYMBOL* aParent ) :
    LIB_ITEM( LIB_BEZIER_T, aParent )
{
    m_fill       = FILL_TYPE::NO_FILL;
    m_Width      = 0;
    m_isFillable = true;
}


EDA_ITEM* LIB_BEZIER::Clone() const
{
    return new LIB_BEZIER( *this );
}


int LIB_BEZIER::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_BEZIER_T );

    int retv = LIB_ITEM::compare( aOther );

    if( retv )
        return retv;

    const LIB_BEZIER* tmp = ( LIB_BEZIER* ) &aOther;

    if( m_BezierPoints.size() != tmp->m_BezierPoints.size() )
        return m_BezierPoints.size() - tmp->m_BezierPoints.size();

    for( size_t i = 0; i < m_BezierPoints.size(); i++ )
    {
        if( m_BezierPoints[i].x != tmp->m_BezierPoints[i].x )
            return m_BezierPoints[i].x - tmp->m_BezierPoints[i].x;

        if( m_BezierPoints[i].y != tmp->m_BezierPoints[i].y )
            return m_BezierPoints[i].y - tmp->m_BezierPoints[i].y;
    }

    return 0;
}


void LIB_BEZIER::Offset( const wxPoint& aOffset )
{
    size_t i;

    for( i = 0; i < m_BezierPoints.size(); i++ )
        m_BezierPoints[i] += aOffset;

    for( i = 0; i < m_PolyPoints.size(); i++ )
        m_PolyPoints[i] += aOffset;
}


void LIB_BEZIER::MoveTo( const wxPoint& aPosition )
{
    if ( !m_PolyPoints.size() )
        m_PolyPoints.emplace_back(0, 0 );

    Offset( aPosition - m_PolyPoints[ 0 ] );
}

const wxPoint LIB_BEZIER::GetOffset() const
{
    if ( !m_PolyPoints.size() )
        return wxPoint(0, 0);

    return m_PolyPoints[0];
}

void LIB_BEZIER::MirrorHorizontal( const wxPoint& aCenter )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].x -= aCenter.x;
        m_PolyPoints[i].x *= -1;
        m_PolyPoints[i].x += aCenter.x;
    }

    imax = m_BezierPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_BezierPoints[i].x -= aCenter.x;
        m_BezierPoints[i].x *= -1;
        m_BezierPoints[i].x += aCenter.x;
    }
}

void LIB_BEZIER::MirrorVertical( const wxPoint& aCenter )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].y -= aCenter.y;
        m_PolyPoints[i].y *= -1;
        m_PolyPoints[i].y += aCenter.y;
    }

    imax = m_BezierPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_BezierPoints[i].y -= aCenter.y;
        m_BezierPoints[i].y *= -1;
        m_BezierPoints[i].y += aCenter.y;
    }
}

void LIB_BEZIER::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    for( wxPoint& point : m_PolyPoints )
        RotatePoint( &point, aCenter, rot_angle );

    for( wxPoint& bezierPoint : m_BezierPoints )
        RotatePoint( &bezierPoint, aCenter, rot_angle );
}


void LIB_BEZIER::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                       const TRANSFORM& aTransform ) const
{
    wxASSERT( aPlotter != NULL );

    static std::vector< wxPoint > cornerList;
    cornerList.clear();

    for( wxPoint pos : m_PolyPoints )
    {
        pos = aTransform.TransformCoordinate( pos ) + aOffset;
        cornerList.push_back( pos );
    }

    if( aFill && m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->PlotPoly( cornerList, FILL_TYPE::FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR;
    int  pen_size = GetPenWidth();

    if( !already_filled || pen_size > 0 )
    {
        pen_size = std::max( pen_size, aPlotter->RenderSettings()->GetMinPenWidth() );

        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE ) );
        aPlotter->PlotPoly( cornerList, already_filled ? FILL_TYPE::NO_FILL : m_fill, pen_size );
    }
}


int LIB_BEZIER::GetPenWidth() const
{
    // Historically 0 meant "default width" and negative numbers meant "don't stroke".
    if( m_Width < 0 && GetFillMode() != FILL_TYPE::NO_FILL )
        return 0;
    else
        return std::max( m_Width, 1 );
}


void LIB_BEZIER::print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset, void* aData,
                        const TRANSFORM& aTransform )
{
    bool forceNoFill = static_cast<bool>( aData );
    int  penWidth = GetPenWidth();

    if( forceNoFill && m_fill != FILL_TYPE::NO_FILL && penWidth == 0 )
        return;

    std::vector<wxPoint> PolyPointsTraslated;

    wxDC*       DC = aSettings->GetPrintDC();
    COLOR4D     color = aSettings->GetLayerColor( LAYER_DEVICE );
    BEZIER_POLY converter( m_BezierPoints );
    converter.GetPoly( m_PolyPoints );

    PolyPointsTraslated.clear();

    for( wxPoint& point : m_PolyPoints )
        PolyPointsTraslated.push_back( aTransform.TransformCoordinate( point ) + aOffset );

    if( forceNoFill || m_fill == FILL_TYPE::NO_FILL )
    {
        penWidth = std::max( penWidth, aSettings->GetDefaultPenWidth() );

        GRPoly( nullptr, DC, m_PolyPoints.size(), &PolyPointsTraslated[0], false, penWidth,
                color, color );
    }
    else
    {
        if( m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
            color = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );

        GRPoly( nullptr, DC, m_PolyPoints.size(), &PolyPointsTraslated[0], true, penWidth,
                color, color );
    }
}


bool LIB_BEZIER::HitTest( const wxPoint& aRefPos, int aAccuracy ) const
{
    int     mindist = std::max( aAccuracy + GetPenWidth() / 2,
                                Mils2iu( MINIMUM_SELECTION_DISTANCE ) );
    wxPoint start, end;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        start = DefaultTransform.TransformCoordinate( m_PolyPoints[ii - 1] );
        end   = DefaultTransform.TransformCoordinate( m_PolyPoints[ii] );

        if ( TestSegmentHit( aRefPos, start, end, mindist ) )
            return true;
    }

    return false;
}


bool LIB_BEZIER::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    // Fast test: if aRect is outside the polygon bounding box, rectangles cannot intersect
    if( !sel.Intersects( GetBoundingBox() ) )
        return false;

    // Account for the width of the line
    sel.Inflate( GetWidth() / 2 );
    unsigned count = m_BezierPoints.size();

    for( unsigned ii = 1; ii < count; ii++ )
    {
        wxPoint vertex = DefaultTransform.TransformCoordinate( m_BezierPoints[ii-1] );
        wxPoint vertexNext = DefaultTransform.TransformCoordinate( m_BezierPoints[ii] );

        // Test if the point is within aRect
        if( sel.Contains( vertex ) )
            return true;

        // Test if this edge intersects aRect
        if( sel.Intersects( vertex, vertexNext ) )
            return true;
    }

    return false;
}


const EDA_RECT LIB_BEZIER::GetBoundingBox() const
{
    EDA_RECT rect;
    int      xmin, xmax, ymin, ymax;

    if( !GetCornerCount() )
        return rect;

    xmin = xmax = m_PolyPoints[0].x;
    ymin = ymax = m_PolyPoints[0].y;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        xmin = std::min( xmin, m_PolyPoints[ii].x );
        xmax = std::max( xmax, m_PolyPoints[ii].x );
        ymin = std::min( ymin, m_PolyPoints[ii].y );
        ymax = std::max( ymax, m_PolyPoints[ii].y );
    }

    rect.SetOrigin( xmin, ymin );
    rect.SetEnd( xmax, ymax );
    rect.Inflate( ( GetPenWidth() / 2 ) + 1 );

    rect.RevertYAxis();

    return rect;
}


void LIB_BEZIER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;
    EDA_RECT bBox = GetBoundingBox();

    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), m_Width );

    aList.emplace_back( _( "Line Width" ), msg );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ),
                bBox.GetOrigin().x,
                bBox.GetOrigin().y,
                bBox.GetEnd().x,
                bBox.GetEnd().y );

    aList.emplace_back( _( "Bounding Box" ), msg );
}

wxPoint LIB_BEZIER::GetPosition() const
{
    if( !m_PolyPoints.size() )
        return wxPoint(0, 0);

    return m_PolyPoints[0];
}
