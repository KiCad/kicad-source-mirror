/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_basic.h>
#include <geometry/geometry_utils.h>
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


void SCH_SHAPE::swapData( SCH_ITEM* aItem )
{
    SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( aItem );

    EDA_SHAPE::SwapShape( shape );
}


void SCH_SHAPE::SetStroke( const STROKE_PARAMS& aStroke )
{
    m_stroke = aStroke;
}


void SCH_SHAPE::SetFilled( bool aFilled )
{
    if( !aFilled )
        m_fill = FILL_T::NO_FILL;
    else if( GetParentSymbol() )
        m_fill = FILL_T::FILLED_SHAPE;
    else
        m_fill = FILL_T::FILLED_WITH_COLOR;
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
    flip( VECTOR2I( aCenter, 0 ), FLIP_DIRECTION::LEFT_RIGHT );
}


void SCH_SHAPE::MirrorVertically( int aCenter )
{
    flip( VECTOR2I( 0, aCenter ), FLIP_DIRECTION::TOP_BOTTOM );
}


void SCH_SHAPE::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    rotate( aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );
}


bool SCH_SHAPE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    return hitTest( aPosition, aAccuracy );
}


bool SCH_SHAPE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    return hitTest( aRect, aContained, aAccuracy );
}


bool SCH_SHAPE::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    std::vector<SHAPE*> shapes = MakeEffectiveShapes( false );

    for( SHAPE* shape : shapes )
    {
        bool hit = KIGEOM::ShapeHitTest( aPoly, *shape, aContained );

        if( hit )
        {
            for( SHAPE* s : shapes )
                delete s;
            return true;
        }
    }

    for( SHAPE* shape : shapes )
        delete shape;

    return false;
}


bool SCH_SHAPE::IsEndPoint( const VECTOR2I& aPt ) const
{
    SHAPE_T shape = GetShape();

    if( shape == SHAPE_T::ARC || shape == SHAPE_T::BEZIER || shape == SHAPE_T::SEGMENT )
        return ( aPt == GetStart() ) || ( aPt == GetEnd() );

    if( shape == SHAPE_T::RECTANGLE )
    {
        for( const VECTOR2I& corner : GetRectCorners() )
        {
            if( corner == aPt )
                return true;
        }

        return false;
    }

    if( shape == SHAPE_T::POLY )
    {
        for( const VECTOR2I& pt : GetPolyPoints() )
        {
            if( pt == aPt )
                return true;
        }

        return false;
    }

    return false;
}


void SCH_SHAPE::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                      int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( IsPrivate() )
        return;

    // note: if aBodyStyle == -1 the outline shape is not plotted. Only the filled area
    // is plotted (used to plot cells for SCH_TABLE items

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    int                  pen_size = GetEffectivePenWidth( renderSettings );

    static std::vector<VECTOR2I> ptList;

    if( GetShape() == SHAPE_T::POLY )
    {
        ptList.clear();

        for( const VECTOR2I& pt : m_poly.Outline( 0 ).CPoints() )
            ptList.push_back( renderSettings->TransformCoordinate( pt ) + aOffset );
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptList.clear();

        for( const VECTOR2I& pt : m_bezierPoints )
            ptList.push_back( renderSettings->TransformCoordinate( pt ) + aOffset );
    }

    COLOR4D    color = GetStroke().GetColor();
    COLOR4D    bg = renderSettings->GetBackgroundColor();
    LINE_STYLE lineStyle = GetStroke().GetLineStyle();
    FILL_T     fill = m_fill;

    if( aBackground )
    {
        switch( m_fill )
        {
        case FILL_T::FILLED_SHAPE:
            // Fill in the foreground layer
            return;

        case FILL_T::HATCH:
        case FILL_T::REVERSE_HATCH:
        case FILL_T::CROSS_HATCH:
            if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
                color = renderSettings->GetLayerColor( m_layer );

            color.a = color.a * 0.4;
            break;

        case FILL_T::FILLED_WITH_COLOR:
            // drop separate fills in B&W mode
            if( !aPlotter->GetColorMode() && pen_size > 0 )
                return;

            color = GetFillColor();

            if( color == COLOR4D::UNSPECIFIED )
                color = renderSettings->GetLayerColor( m_layer );

            break;

        case FILL_T::FILLED_WITH_BG_BODYCOLOR:
            // drop fill in B&W mode
            if( !aPlotter->GetColorMode() )
                return;

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

        pen_size = aBodyStyle == -1 ? 0 : GetEffectivePenWidth( renderSettings );
    }

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( color.m_text.has_value() && Schematic() )
        color = COLOR4D( ResolveText( color.m_text.value(), &Schematic()->CurrentSheet() ) );

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );

    if( aBackground && IsHatchedFill() )
    {
        for( int ii = 0; ii < GetHatching().OutlineCount(); ++ii )
            aPlotter->PlotPoly( GetHatching().COutline( ii ), FILL_T::FILLED_SHAPE, 0, nullptr );

        return;
    }

    aPlotter->SetCurrentLineWidth( pen_size );
    aPlotter->SetDash( pen_size, lineStyle );

    VECTOR2I start = renderSettings->TransformCoordinate( m_start ) + aOffset;
    VECTOR2I end = renderSettings->TransformCoordinate( m_end ) + aOffset;
    VECTOR2I mid, center;

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        mid = renderSettings->TransformCoordinate( GetArcMid() ) + aOffset;
        aPlotter->Arc( start, mid, end, fill, pen_size );
        break;

    case SHAPE_T::CIRCLE:
        center = renderSettings->TransformCoordinate( getCenter() ) + aOffset;
        aPlotter->Circle( center, GetRadius() * 2, fill, pen_size );
        break;

    case SHAPE_T::RECTANGLE:
        aPlotter->Rect( start, end, fill, pen_size, GetCornerRadius() );
        break;

    case SHAPE_T::POLY:
    case SHAPE_T::BEZIER:
        aPlotter->PlotPoly( ptList, fill, pen_size, nullptr );
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
    return getBoundingBox();
}


void SCH_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    SCH_ITEM::GetMsgPanelInfo( aFrame, aList );

    ShapeGetMsgPanelInfo( aFrame, aList );
}


wxString SCH_SHAPE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
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


std::vector<int> SCH_SHAPE::ViewGetLayers() const
{
    std::vector<int> layers( 3 );

    layers[0] = IsPrivate() ? LAYER_PRIVATE_NOTES : m_layer;

    if( m_layer == LAYER_DEVICE )
    {
        if( m_fill == FILL_T::FILLED_WITH_BG_BODYCOLOR )
            layers[1] = LAYER_DEVICE_BACKGROUND;
        else
            layers[1] = LAYER_SHAPES_BACKGROUND;
    }
    else
    {
        layers[1] = LAYER_SHAPES_BACKGROUND;
    }

    layers[2] = LAYER_SELECTION_SHADOWS;

    return layers;
}


void SCH_SHAPE::AddPoint( const VECTOR2I& aPosition )
{
    if( GetShape() == SHAPE_T::POLY )
    {
        if( m_poly.IsEmpty() )
        {
            m_poly.NewOutline();
            m_poly.Outline( 0 ).SetClosed( false );
        }

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
    int cmpFlags = aCompareFlags;

    // The object UUIDs must be compared after the shape coordinates because shapes do not
    // have immutable UUIDs.
    if( !( cmpFlags & ( SCH_ITEM::COMPARE_FLAGS::EQUALITY | SCH_ITEM::COMPARE_FLAGS::ERC ) ) )
        cmpFlags |= SCH_ITEM::COMPARE_FLAGS::EQUALITY;

    int retv = SCH_ITEM::compare( aOther, cmpFlags );

    if( retv )
        return retv;

    retv = EDA_SHAPE::Compare( &static_cast<const SCH_SHAPE&>( aOther ) );

    if( retv )
        return retv;

    if( ( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::EQUALITY )
        || ( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::ERC ) )
    {
        return 0;
    }

    if( m_Uuid < aOther.m_Uuid )
        return -1;

    if( m_Uuid > aOther.m_Uuid )
        return 1;

    return 0;
}


static struct SCH_SHAPE_DESC
{
    SCH_SHAPE_DESC()
    {
        ENUM_MAP<FILL_T>& fillEnum = ENUM_MAP<FILL_T>::Instance();

        if( fillEnum.Choices().GetCount() == 0 )
        {
            fillEnum.Map( FILL_T::NO_FILL,                  _HKI( "None" ) )
                    .Map( FILL_T::FILLED_SHAPE,             _HKI( "Body outline color" ) )
                    .Map( FILL_T::FILLED_WITH_BG_BODYCOLOR, _HKI( "Body background color" ) )
                    .Map( FILL_T::FILLED_WITH_COLOR,        _HKI( "Fill color" ) );
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

        auto isFillColorEditable =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( aItem ) )
                    {
                        if( shape->GetParentSymbol() )
                            return shape->GetFillMode() == FILL_T::FILLED_WITH_COLOR;
                        else
                            return shape->IsSolidFill();
                    }

                    return true;
                };

        propMgr.OverrideAvailability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Position X" ), isPolygon );
        propMgr.OverrideAvailability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Position Y" ), isPolygon );

        propMgr.OverrideAvailability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Filled" ), isSchematicItem );

        propMgr.OverrideWriteability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Fill Color" ), isFillColorEditable );

        void ( SCH_SHAPE::*fillModeSetter )( FILL_T ) = &SCH_SHAPE::SetFillMode;
        FILL_T ( SCH_SHAPE::*fillModeGetter )() const = &SCH_SHAPE::GetFillMode;

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_SHAPE, FILL_T>( _HKI( "Fill Mode" ),
                        fillModeSetter, fillModeGetter ),
                        _HKI( "Shape Properties" ) )
                .SetAvailableFunc( isSymbolItem );
    }
} _SCH_SHAPE_DESC;

ENUM_TO_WXANY( FILL_T );
