/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <macros.h>
#include <plotters/plotter.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <general.h>
#include <schematic.h>
#include <sch_shape.h>


SCH_SHAPE::SCH_SHAPE( SHAPE_T aShape, SCH_LAYER_ID aLayer, int aLineWidth, FILL_T aFillType,
                      KICAD_T aType ) :
    SCH_ITEM( nullptr, aType ),
    EDA_SHAPE( aShape, aLineWidth, aFillType )
{
    SetLayer( aLayer );
}


EDA_ITEM* SCH_SHAPE::Clone() const
{
    return new SCH_SHAPE( *this );
}


void SCH_SHAPE::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( aItem );

    EDA_SHAPE::SwapShape( shape );
    std::swap( m_layer, shape->m_layer );
}


void SCH_SHAPE::SetStroke( const STROKE_PARAMS& aStroke )
{
    m_stroke = aStroke;
}


void SCH_SHAPE::Move( const VECTOR2I& aOffset )
{
    move( aOffset );
}


void SCH_SHAPE::Normalize()
{
    if( GetShape() == SHAPE_T::RECTANGLE )
    {
        VECTOR2I size = GetEnd() - GetPosition();

        if( size.y < 0 )
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


void SCH_SHAPE::MirrorHorizontally( int aCenter )
{
    flip( VECTOR2I( aCenter, 0 ), true );
}


void SCH_SHAPE::MirrorVertically( int aCenter )
{
    flip( VECTOR2I( 0, aCenter ), false );
}


void SCH_SHAPE::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    rotate( aCenter, aRotateCCW ? ANGLE_270 : ANGLE_90 );
}


bool SCH_SHAPE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    if( m_layer == LAYER_DEVICE )
        return hitTest( DefaultTransform.TransformCoordinate( aPosition ), aAccuracy );
    else
        return hitTest( aPosition, aAccuracy );
}

bool SCH_SHAPE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    if( m_layer == LAYER_DEVICE )
        return hitTest( DefaultTransform.TransformCoordinate( aRect ), aContained, aAccuracy );
    else
        return hitTest( aRect, aContained, aAccuracy );
}


void SCH_SHAPE::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                      int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( IsPrivate() )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    int                  pen_size = GetEffectivePenWidth( renderSettings );

    static std::vector<VECTOR2I> ptList;

    if( GetShape() == SHAPE_T::POLY )
    {
        ptList.clear();

        for( const VECTOR2I& pt : m_poly.Outline( 0 ).CPoints() )
        {
            if( m_layer == LAYER_DEVICE )
                ptList.push_back( renderSettings->TransformCoordinate( pt ) + aOffset );
            else
                ptList.push_back( pt );
        }
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptList.clear();

        for( const VECTOR2I& pt : m_bezierPoints )
        {
            if( m_layer == LAYER_DEVICE )
                ptList.push_back( renderSettings->TransformCoordinate( pt ) + aOffset );
            else
                ptList.push_back( pt );
        }
    }

    COLOR4D    color = GetStroke().GetColor();
    COLOR4D    bg = renderSettings->GetBackgroundColor();
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

        pen_size = 0;
        lineStyle = LINE_STYLE::SOLID;
    }
    else /* if( aForeground ) */
    {
        if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
            color = renderSettings->GetLayerColor( m_layer );

        if( lineStyle == LINE_STYLE::DEFAULT )
            lineStyle = LINE_STYLE::SOLID;

        if( m_fill == FILL_T::FILLED_SHAPE )
            fill = m_fill;
        else
            fill = FILL_T::NO_FILL;

        pen_size = GetEffectivePenWidth( renderSettings );
    }

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );
    aPlotter->SetCurrentLineWidth( pen_size );
    aPlotter->SetDash( pen_size, lineStyle );

    VECTOR2I start = m_start;
    VECTOR2I end = m_end;
    VECTOR2I mid, center;

    if( m_layer == LAYER_DEVICE )
    {
        start = renderSettings->TransformCoordinate( start ) + aOffset;
        end = renderSettings->TransformCoordinate( end ) + aOffset;
    }

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        mid = GetArcMid();

        if( m_layer == LAYER_DEVICE )
            mid = renderSettings->TransformCoordinate( mid ) + aOffset;

        aPlotter->Arc( start, mid, end, fill, pen_size );
        break;

    case SHAPE_T::CIRCLE:
        center = getCenter();

        if( m_layer == LAYER_DEVICE )
            center = renderSettings->TransformCoordinate( center ) + aOffset;

        aPlotter->Circle( center, GetRadius() * 2, fill, pen_size );
        break;

    case SHAPE_T::RECTANGLE:
        aPlotter->Rect( start, end, fill, pen_size );
        break;

    case SHAPE_T::POLY:
    case SHAPE_T::BEZIER:
        aPlotter->PlotPoly( ptList, fill, pen_size );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    aPlotter->SetDash( pen_size, LINE_STYLE::SOLID );
}


int SCH_SHAPE::GetEffectiveWidth() const
{
    if( GetPenWidth() > 0 )
        return GetPenWidth();

    // Historically 0 meant "default width" and negative numbers meant "don't stroke".
    if( GetPenWidth() < 0 )
        return 0;

    SCHEMATIC* schematic = Schematic();

    if( schematic )
        return schematic->Settings().m_DefaultLineWidth;

    return schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );
}


const BOX2I SCH_SHAPE::GetBoundingBox() const
{
    BOX2I bbox = getBoundingBox();

    if( m_layer == LAYER_DEVICE )   // TODO: nuke symbol editor's upside-down coordinate system
        bbox.RevertYAxis();

    return bbox;
}


void SCH_SHAPE::PrintBackground( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                                 const VECTOR2I& aOffset, bool aDimmed )
{
    if( IsPrivate() )
        return;

    wxDC*    DC = aSettings->GetPrintDC();
    COLOR4D  color;

    static std::vector<VECTOR2I> ptList;

    if( GetShape() == SHAPE_T::POLY )
    {
        ptList.clear();

        for( const VECTOR2I& pt : m_poly.Outline( 0 ).CPoints() )
        {
            if( m_layer == LAYER_DEVICE )
                ptList.push_back( aSettings->TransformCoordinate( pt ) + aOffset );
            else
                ptList.push_back( pt );
        }
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptList.clear();

        for( const VECTOR2I& pt : m_bezierPoints )
        {
            if( m_layer == LAYER_DEVICE )
                ptList.push_back( aSettings->TransformCoordinate( pt ) + aOffset );
            else
                ptList.push_back( pt );
        }
    }

    if( GetFillMode() == FILL_T::FILLED_WITH_COLOR )
    {
        if( GetFillColor() == COLOR4D::UNSPECIFIED )
            color = aSettings->GetLayerColor( LAYER_NOTES );
        else
            color = GetFillColor();

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRFilledArc( DC, GetEnd(), GetStart(), getCenter(), 0, color, color );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( DC, GetStart(), GetRadius(), 0, color, color );
            break;

        case SHAPE_T::RECTANGLE:
            GRFilledRect( DC, GetStart(), GetEnd(), 0, color, color );
            break;

        case SHAPE_T::POLY:
            GRPoly( DC, (int) ptList.size(), ptList.data(), true, 0, color, color );
            break;

        case SHAPE_T::BEZIER:
            GRPoly( DC, (int) ptList.size(), ptList.data(), true, 0, color, color );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        }
    }
}


void SCH_SHAPE::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                       const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    if( IsPrivate() )
        return;

    int      penWidth = GetEffectivePenWidth( aSettings );
    wxDC*    DC = aSettings->GetPrintDC();
    COLOR4D  color = GetStroke().GetColor();
    COLOR4D  bg = aSettings->GetBackgroundColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( LAYER_NOTES );

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    static std::vector<VECTOR2I> ptList;

    if( GetShape() == SHAPE_T::POLY )
    {
        ptList.clear();

        for( const VECTOR2I& pt : m_poly.Outline( 0 ).CPoints() )
        {
            if( m_layer == LAYER_DEVICE )
                ptList.push_back( aSettings->TransformCoordinate( pt ) + aOffset );
            else
                ptList.push_back( pt );
        }
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptList.clear();

        for( const VECTOR2I& pt : m_bezierPoints )
        {
            if( m_layer == LAYER_DEVICE )
                ptList.push_back( aSettings->TransformCoordinate( pt ) + aOffset );
            else
                ptList.push_back( pt );
        }
    }

    VECTOR2I start = GetStart();
    VECTOR2I end = GetEnd();
    VECTOR2I center = ( GetShape() == SHAPE_T::ARC ) ? getCenter() : VECTOR2I( 0, 0 );

    if( m_layer == LAYER_DEVICE )
    {
        start = aSettings->TransformCoordinate( start ) + aOffset;
        end = aSettings->TransformCoordinate( end ) + aOffset;

        if( GetShape() == SHAPE_T::ARC )
        {
            center = aSettings->TransformCoordinate( center ) + aOffset;

            EDA_ANGLE t1, t2;

            CalcArcAngles( t1, t2 );

            // N.B. The order of evaluation is critical here as MapAngles will modify t1, t2
            // and the Normalize routine depends on these modifications for the correct output
            bool transformed = aSettings->m_Transform.MapAngles( &t1, &t2 );
            EDA_ANGLE arc_angle =  ( t1 - t2 ).Normalize180();
            bool transformed2 = ( arc_angle > ANGLE_0 ) && ( arc_angle < ANGLE_180 );

            if( transformed  == transformed2 )
                std::swap( start, end );
        }
    }

    COLOR4D fillColor = COLOR4D::UNSPECIFIED;

    if( GetFillMode() == FILL_T::FILLED_SHAPE )
        fillColor = color;
    else if( GetFillMode() == FILL_T::FILLED_WITH_COLOR )
        fillColor = GetFillColor();
    else if( GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
        fillColor = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );

    if( fillColor != COLOR4D::UNSPECIFIED && !aForceNoFill )
    {
        if( aDimmed )
        {
            fillColor.Desaturate( );
            fillColor = fillColor.Mix( bg, 0.5f );
        }

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRFilledArc( DC, end, start, center, 0, fillColor, fillColor );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( DC, start, GetRadius(), 0, fillColor, fillColor );
            break;

        case SHAPE_T::RECTANGLE:
            GRFilledRect( DC, start, end, 0, fillColor, fillColor );
            break;

        case SHAPE_T::POLY:
            GRPoly( DC, (int) ptList.size(), ptList.data(), true, 0, fillColor, fillColor );
            break;

        case SHAPE_T::BEZIER:
            GRPoly( DC, (int) ptList.size(), ptList.data(), true, 0, fillColor, fillColor );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        }
    }

    penWidth = std::max( penWidth, aSettings->GetMinPenWidth() );

    if( penWidth > 0 )
    {
        if( GetEffectiveLineStyle() == LINE_STYLE::SOLID )
        {
            switch( GetShape() )
            {
            case SHAPE_T::ARC:
                GRArc( DC, end, start, center, penWidth, color );
                break;

            case SHAPE_T::CIRCLE:
                GRCircle( DC, start, GetRadius(), penWidth, color );
                break;

            case SHAPE_T::RECTANGLE:
                GRRect( DC, start, end, penWidth, color );
                break;

            case SHAPE_T::POLY:
                GRPoly( DC, (int) ptList.size(), ptList.data(), false, penWidth, color, color );
                break;

            case SHAPE_T::BEZIER:
                GRPoly( DC, (int) ptList.size(), ptList.data(), false, penWidth, color, color );
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
                            if( m_layer == LAYER_DEVICE )
                            {
                                VECTOR2I ptA = aSettings->TransformCoordinate( a ) + aOffset;
                                VECTOR2I ptB = aSettings->TransformCoordinate( b ) + aOffset;
                                GRLine( DC, ptA.x, ptA.y, ptB.x, ptB.y, penWidth, color );
                            }
                            else
                            {
                                GRLine( DC, a.x, a.y, b.x, b.y, penWidth, color );
                            }
                        } );
            }

            for( SHAPE* shape : shapes )
                delete shape;
        }
    }
}


void SCH_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    if( m_layer == LAYER_DEVICE )
        getSymbolEditorMsgPanelInfo( aFrame, aList );
    else
        SCH_ITEM::GetMsgPanelInfo( aFrame, aList );

    ShapeGetMsgPanelInfo( aFrame, aList );
}


wxString SCH_SHAPE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        return wxString::Format( _( "Arc, radius %s" ),
                                 aUnitsProvider->MessageTextFromValue( GetRadius() ) );

    case SHAPE_T::CIRCLE:
        return wxString::Format( _( "Circle, radius %s" ),
                                 aUnitsProvider->MessageTextFromValue( GetRadius() ) );

    case SHAPE_T::RECTANGLE:
        return wxString::Format( _( "Rectangle, width %s height %s" ),
                                 aUnitsProvider->MessageTextFromValue( std::abs( m_start.x - m_end.x ) ),
                                 aUnitsProvider->MessageTextFromValue( std::abs( m_start.y - m_end.y ) ) );

    case SHAPE_T::POLY:
        return wxString::Format( _( "Polyline, %d points" ),
                                 int( m_poly.Outline( 0 ).GetPointCount() ) );

    case SHAPE_T::BEZIER:
        return wxString::Format( _( "Bezier Curve, %d points" ),
                                 int( m_bezierPoints.size() ) );

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return wxEmptyString;
    }
}


BITMAPS SCH_SHAPE::GetMenuImage() const
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


void SCH_SHAPE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 3;
    aLayers[0] = IsPrivate() ? LAYER_PRIVATE_NOTES : m_layer;

    if( m_layer == LAYER_PRIVATE_NOTES )
        aLayers[1] = LAYER_NOTES_BACKGROUND;
    else if( m_layer == LAYER_DEVICE )
        aLayers[1] = LAYER_DEVICE_BACKGROUND;
    else
        aLayers[1] = LAYER_NOTES_BACKGROUND;

    aLayers[2] = LAYER_SELECTION_SHADOWS;
}


void SCH_SHAPE::AddPoint( const VECTOR2I& aPosition )
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


bool SCH_SHAPE::operator==( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const SCH_SHAPE& other = static_cast<const SCH_SHAPE&>( aOther );

    return SCH_ITEM::operator==( aOther ) && EDA_SHAPE::operator==( other );
}


double SCH_SHAPE::Similarity( const SCH_ITEM& aOther ) const
{
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( aOther.Type() != Type() )
        return 0.0;

    const SCH_SHAPE& other = static_cast<const SCH_SHAPE&>( aOther );

    double similarity = SimilarityBase( other );

    similarity *= EDA_SHAPE::Similarity( other );

    return similarity;
}


int SCH_SHAPE::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    int retv = SCH_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    return EDA_SHAPE::Compare( &static_cast<const SCH_SHAPE&>( aOther ) );
}


static struct SCH_SHAPE_DESC
{
    SCH_SHAPE_DESC()
    {
        ENUM_MAP<FILL_T>& fillEnum = ENUM_MAP<FILL_T>::Instance();

        if( fillEnum.Choices().GetCount() == 0 )
        {
            fillEnum.Map( FILL_T::NO_FILL, _HKI( "None" ) )
                    .Map( FILL_T::FILLED_SHAPE, _HKI( "Body outline color" ) )
                    .Map( FILL_T::FILLED_WITH_BG_BODYCOLOR, _HKI( "Body background color" ) )
                    .Map( FILL_T::FILLED_WITH_COLOR, _HKI( "Fill color" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_SHAPE );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_SHAPE, SCH_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_SHAPE, EDA_SHAPE> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( EDA_SHAPE ) );

        // Only polygons have meaningful Position properties.
        // On other shapes, these are duplicates of the Start properties.
        auto isPolygon =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( aItem ) )
                        return shape->GetShape() == SHAPE_T::POLY;

                    return false;
                };

        auto isSymbolItem =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( aItem ) )
                        return shape->GetLayer() == LAYER_DEVICE;

                    return false;
                };

        auto isSchematicItem =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( aItem ) )
                        return shape->GetLayer() != LAYER_DEVICE;

                    return false;
                };

        propMgr.OverrideAvailability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Position X" ), isPolygon );
        propMgr.OverrideAvailability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Position Y" ), isPolygon );

        propMgr.OverrideAvailability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Filled" ), isSchematicItem );

        void ( SCH_SHAPE::*fillModeSetter )( FILL_T ) = &SCH_SHAPE::SetFillMode;
        FILL_T ( SCH_SHAPE::*fillModeGetter )() const = &SCH_SHAPE::GetFillMode;

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_SHAPE, FILL_T>( _HKI( "Fill" ),
                        fillModeSetter, fillModeGetter ),
                        _HKI( "Shape Properties" ) )
                .SetAvailableFunc( isSymbolItem );
    }
} _SCH_SHAPE_DESC;

ENUM_TO_WXANY( FILL_T );
