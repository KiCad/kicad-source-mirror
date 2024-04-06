/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <plotters/plotter.h>
#include <macros.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <general.h>
#include <lib_shape.h>


LIB_SHAPE::LIB_SHAPE( SCH_ITEM* aParent, SHAPE_T aShape, int aLineWidth, FILL_T aFillType,
                      KICAD_T aType ) :
    SCH_ITEM( aParent, aType ),
    EDA_SHAPE( aShape, aLineWidth, aFillType )
{
    m_editState = 0;
}


bool LIB_SHAPE::HitTest( const VECTOR2I& aPosRef, int aAccuracy ) const
{
    if( aAccuracy < schIUScale.MilsToIU( MINIMUM_SELECTION_DISTANCE ) )
        aAccuracy = schIUScale.MilsToIU( MINIMUM_SELECTION_DISTANCE );

    return hitTest( DefaultTransform.TransformCoordinate( aPosRef ), aAccuracy );
}


bool LIB_SHAPE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    return hitTest( DefaultTransform.TransformCoordinate( aRect ), aContained, aAccuracy );
}


EDA_ITEM* LIB_SHAPE::Clone() const
{
    return new LIB_SHAPE( *this );
}


int LIB_SHAPE::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    int retv = SCH_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    return EDA_SHAPE::Compare( &static_cast<const LIB_SHAPE&>( aOther ) );
}


void LIB_SHAPE::Move( const VECTOR2I& aOffset )
{
    move( aOffset );
}


void LIB_SHAPE::Normalize()
{
    if( GetShape() == SHAPE_T::RECTANGLE )
    {
        VECTOR2I size = GetEnd() - GetPosition();

        if( size.y > 0 )
        {
            SetStartY( GetStartY() + size.y );
            SetEndY( GetStartY() - size.y );
        }

        if( size.x < 0 )
        {
            SetStartX( GetStartX() + size.x );
            SetEndX( GetStartX() - size.x );
        }
    }
}


void LIB_SHAPE::MirrorHorizontally( int aCenter )
{
    flip( VECTOR2I( aCenter, 0 ), true );
}


void LIB_SHAPE::MirrorVertically( int aCenter )
{
    flip( VECTOR2I( 0, aCenter ), false );
}


void LIB_SHAPE::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    EDA_ANGLE rot_angle = aRotateCCW ? -ANGLE_90 : ANGLE_90;

    rotate( aCenter, rot_angle );
}


void LIB_SHAPE::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                      int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( IsPrivate() )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );

    VECTOR2I start = renderSettings->TransformCoordinate( m_start ) + aOffset;
    VECTOR2I end = renderSettings->TransformCoordinate( m_end ) + aOffset;

    static std::vector<VECTOR2I> cornerList;

    if( GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );
        cornerList.clear();

        for( const VECTOR2I& pt : poly.CPoints() )
            cornerList.push_back( renderSettings->TransformCoordinate( pt ) + aOffset );
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        cornerList.clear();

        for( const VECTOR2I& pt : m_bezierPoints )
            cornerList.push_back( renderSettings->TransformCoordinate( pt ) + aOffset );
    }

    int        penWidth;
    COLOR4D    color = GetStroke().GetColor();
    LINE_STYLE lineStyle = GetStroke().GetLineStyle();
    FILL_T     fill = m_fill;

    if( aBackground )
    {
        if( !aPlotter->GetColorMode() )
            return;

        switch( m_fill )
        {
        case FILL_T::FILLED_SHAPE:
            return;

        case FILL_T::FILLED_WITH_COLOR:
            color = GetFillColor();
            break;

        case FILL_T::FILLED_WITH_BG_BODYCOLOR:
            color = renderSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );
            break;

        default:
            return;
        }

        penWidth = 0;
        lineStyle = LINE_STYLE::SOLID;
    }
    else
    {
        if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
            color = renderSettings->GetLayerColor( LAYER_DEVICE );

        if( lineStyle == LINE_STYLE::DEFAULT )
            lineStyle = LINE_STYLE::SOLID;

        if( m_fill == FILL_T::FILLED_SHAPE )
            fill = m_fill;
        else
            fill = FILL_T::NO_FILL;

        penWidth = GetEffectivePenWidth( renderSettings );
    }

    COLOR4D bg = renderSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );
    aPlotter->SetDash( penWidth, lineStyle );

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
    {
        VECTOR2I mid = renderSettings->TransformCoordinate( GetArcMid() ) + aOffset;

        aPlotter->Arc( start, mid, end, fill, penWidth );
        break;
    }

    case SHAPE_T::CIRCLE:
    {
        VECTOR2I center = renderSettings->TransformCoordinate( getCenter() ) + aOffset;

        aPlotter->Circle( center, GetRadius() * 2, fill, penWidth );
        break;
    }

    case SHAPE_T::RECTANGLE:
        aPlotter->Rect( start, end, fill, penWidth );
        break;

    case SHAPE_T::POLY:
    case SHAPE_T::BEZIER:
        aPlotter->PlotPoly( cornerList, fill, penWidth );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    aPlotter->SetDash( penWidth, LINE_STYLE::SOLID );
}


int LIB_SHAPE::GetPenWidth() const
{
    return GetWidth();
}


void LIB_SHAPE::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                       const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    if( IsPrivate() )
        return;

    int  penWidth = GetEffectivePenWidth( aSettings );

    if( aForceNoFill && IsFilled() && penWidth == 0 )
        return;

    wxDC*    DC = aSettings->GetPrintDC();
    VECTOR2I pt1 = aSettings->m_Transform.TransformCoordinate( m_start ) + aOffset;
    VECTOR2I pt2 = aSettings->m_Transform.TransformCoordinate( m_end ) + aOffset;
    VECTOR2I c;
    COLOR4D  color = GetStroke().GetColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( LAYER_DEVICE );

    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    unsigned ptCount = 0;
    VECTOR2I* buffer = nullptr;

    if( GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        ptCount = poly.GetPointCount();
        buffer = new VECTOR2I[ptCount];

        for( unsigned ii = 0; ii < ptCount; ++ii )
            buffer[ii] = aSettings->m_Transform.TransformCoordinate( poly.CPoint( ii ) ) + aOffset;
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptCount = m_bezierPoints.size();
        buffer = new VECTOR2I[ptCount];

        for( size_t ii = 0; ii < ptCount; ++ii )
            buffer[ii] = aSettings->m_Transform.TransformCoordinate( m_bezierPoints[ii] ) + aOffset;
    }
    else if( GetShape() == SHAPE_T::ARC )
    {
        c = aSettings->m_Transform.TransformCoordinate( getCenter() ) + aOffset;

        EDA_ANGLE t1, t2;

        CalcArcAngles( t1, t2 );

        // N.B. The order of evaluation is critical here as MapAngles will modify t1, t2
        // and the Normalize routine depends on these modifications for the correct output
        bool transformed = aSettings->m_Transform.MapAngles( &t1, &t2 );
        EDA_ANGLE arc_angle =  ( t1 - t2 ).Normalize180();
        bool transformed2 = ( arc_angle > ANGLE_0 ) && ( arc_angle < ANGLE_180 );

        if( transformed  == transformed2 )
            std::swap( pt1, pt2 );
    }

    COLOR4D fillColor = COLOR4D::UNSPECIFIED;

    if( !aForceNoFill )
    {
        if( GetFillMode() == FILL_T::FILLED_SHAPE )
            fillColor = color;
        else if( GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
            fillColor = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );
        else if( GetFillMode() == FILL_T::FILLED_WITH_COLOR )
            fillColor = GetFillColor();
    }

    if( fillColor != COLOR4D::UNSPECIFIED )
    {
        if( aDimmed )
        {
            fillColor.Desaturate( );
            fillColor = fillColor.Mix( bg, 0.5f );
        }

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRFilledArc( DC, pt1, pt2, c, 0, fillColor, fillColor );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( DC, pt1, GetRadius(), 0, fillColor, fillColor );
            break;

        case SHAPE_T::RECTANGLE:
            GRFilledRect( DC, pt1, pt2, 0, fillColor, fillColor );
            break;

        case SHAPE_T::POLY:
            GRPoly( DC, ptCount, buffer, true, 0, fillColor, fillColor );
            break;

        case SHAPE_T::BEZIER:
            GRPoly( DC, ptCount, buffer, true, 0, fillColor, fillColor );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        }
    }
    else
    {
        penWidth = std::max( penWidth, aSettings->GetDefaultPenWidth() );
    }

    if( penWidth > 0 )
    {
        if( GetEffectiveLineStyle() == LINE_STYLE::SOLID )
        {
            switch( GetShape() )
            {
            case SHAPE_T::ARC:
                GRArc( DC, pt1, pt2, c, penWidth, color );
                break;

            case SHAPE_T::CIRCLE:
                GRCircle( DC, pt1, GetRadius(), penWidth, color );
                break;

            case SHAPE_T::RECTANGLE:
                GRRect( DC, pt1, pt2, penWidth, color );
                break;

            case SHAPE_T::POLY:
                GRPoly( DC, ptCount, buffer, false, penWidth, color, color );
                break;

            case SHAPE_T::BEZIER:
                GRPoly( DC, ptCount, buffer, false, penWidth, color, color );
                break;

            default:
                UNIMPLEMENTED_FOR( SHAPE_T_asString() );
            }
        }
        else
        {
            std::vector<SHAPE*> shapes = MakeEffectiveShapes( true );

            for( SHAPE* shape : shapes )
            {
                STROKE_PARAMS::Stroke( shape, GetEffectiveLineStyle(), penWidth, aSettings,
                        [&]( const VECTOR2I& a, const VECTOR2I& b )
                        {
                            VECTOR2I pts = aSettings->m_Transform.TransformCoordinate( a ) + aOffset;
                            VECTOR2I pte = aSettings->m_Transform.TransformCoordinate( b ) + aOffset;
                            GRLine( DC, pts.x, pts.y, pte.x, pte.y, penWidth, color );
                        } );
            }

            for( SHAPE* shape : shapes )
                delete shape;
        }
    }

    delete[] buffer;
}


const BOX2I LIB_SHAPE::GetBoundingBox() const
{
    BOX2I bbox = getBoundingBox();

    bbox.RevertYAxis();

    return bbox;
}


void LIB_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    getSymbolEditorMsgPanelInfo( aFrame, aList );

    ShapeGetMsgPanelInfo( aFrame, aList );
}


wxString LIB_SHAPE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        return wxString::Format( _( "Arc with radius %s" ),
                                 aUnitsProvider->MessageTextFromValue( GetRadius() ) );

    case SHAPE_T::CIRCLE:
        return wxString::Format( _( "Circle with radius %s" ),
                                 aUnitsProvider->MessageTextFromValue( GetRadius() ) );

    case SHAPE_T::RECTANGLE:
        return wxString::Format( _( "Rectangle with width %s height %s" ),
                                 aUnitsProvider->MessageTextFromValue( std::abs( m_start.x - m_end.x ) ),
                                 aUnitsProvider->MessageTextFromValue( std::abs( m_start.y - m_end.y ) ) );

    case SHAPE_T::POLY:
        return wxString::Format( _( "Polyline with %d points" ),
                                 int( m_poly.Outline( 0 ).GetPointCount() ) );

    case SHAPE_T::BEZIER:
        return wxString::Format( _( "Bezier Curve with %d points" ),
                                 int( m_bezierPoints.size() ) );

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return wxEmptyString;
    }
}


BITMAPS LIB_SHAPE::GetMenuImage() const
{
    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT:   return BITMAPS::add_line;
    case SHAPE_T::ARC:       return BITMAPS::add_arc;
    case SHAPE_T::CIRCLE:    return BITMAPS::add_circle;
    case SHAPE_T::RECTANGLE: return BITMAPS::add_rectangle;
    case SHAPE_T::POLY:      return BITMAPS::add_graphical_segments;
    case SHAPE_T::BEZIER:    return BITMAPS::add_bezier;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return BITMAPS::question_mark;
    }
}


void LIB_SHAPE::AddPoint( const VECTOR2I& aPosition )
{
    if( GetShape() == SHAPE_T::POLY )
    {
        if( m_poly.IsEmpty() )
            m_poly.NewOutline();

        m_poly.Outline( 0 ).Append( aPosition, true );
    }
    else
    {
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


bool LIB_SHAPE::operator==( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const LIB_SHAPE& other = static_cast<const LIB_SHAPE&>( aOther );

    return SCH_ITEM::operator==( aOther ) && EDA_SHAPE::operator==( other );
}


double LIB_SHAPE::Similarity( const SCH_ITEM& aOther ) const
{
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( aOther.Type() != Type() )
        return 0.0;

    const LIB_SHAPE& other = static_cast<const LIB_SHAPE&>( aOther );

    double similarity = SimilarityBase( other );

    similarity *= EDA_SHAPE::Similarity( other );

    return similarity;
}


void LIB_SHAPE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 3;
    aLayers[0] = IsPrivate() ? LAYER_PRIVATE_NOTES    : LAYER_DEVICE;
    aLayers[1] = IsPrivate() ? LAYER_NOTES_BACKGROUND : LAYER_DEVICE_BACKGROUND;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
}


static struct LIB_SHAPE_DESC
{
    LIB_SHAPE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( LIB_SHAPE );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_SHAPE, SCH_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_SHAPE, EDA_SHAPE> );
        propMgr.InheritsAfter( TYPE_HASH( LIB_SHAPE ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( LIB_SHAPE ), TYPE_HASH( EDA_SHAPE ) );

        // Only polygons have meaningful Position properties.
        // On other shapes, these are duplicates of the Start properties.
        auto isPolygon =
                []( INSPECTABLE* aItem ) -> bool
        {
            if( LIB_SHAPE* shape = dynamic_cast<LIB_SHAPE*>( aItem ) )
                return shape->GetShape() == SHAPE_T::POLY;

            return false;
        };

        propMgr.OverrideAvailability( TYPE_HASH( LIB_SHAPE ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Position X" ), isPolygon );
        propMgr.OverrideAvailability( TYPE_HASH( LIB_SHAPE ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Position Y" ), isPolygon );

        propMgr.Mask( TYPE_HASH( LIB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "Filled" ) );

        ENUM_MAP<FILL_T>& fillEnum = ENUM_MAP<FILL_T>::Instance();

        if( fillEnum.Choices().GetCount() == 0 )
        {
            fillEnum.Map( FILL_T::NO_FILL, _HKI( "None" ) )
                    .Map( FILL_T::FILLED_SHAPE, _HKI( "Body outline color" ) )
                    .Map( FILL_T::FILLED_WITH_BG_BODYCOLOR, _HKI( "Body background color" ) )
                    .Map( FILL_T::FILLED_WITH_COLOR, _HKI( "Fill color" ) );
        }

        void ( LIB_SHAPE::*fillModeSetter )( FILL_T ) = &LIB_SHAPE::SetFillMode;
        FILL_T ( LIB_SHAPE::*fillModeGetter )() const = &LIB_SHAPE::GetFillMode;

        propMgr.AddProperty( new PROPERTY_ENUM< LIB_SHAPE, FILL_T>( _HKI( "Fill" ),
                        fillModeSetter, fillModeGetter ),
                        _HKI( "Shape Properties" ) );
    }
} _LIB_SHAPE_DESC;

ENUM_TO_WXANY( FILL_T )
