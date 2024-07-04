/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_textbox.h>
#include <pcb_painter.h>
#include <trigo.h>
#include <string_utils.h>
#include <geometry/shape_compound.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>
#include <macros.h>
#include <core/ignore.h>


PCB_TEXTBOX::PCB_TEXTBOX( BOARD_ITEM* aParent, KICAD_T aType ) :
    PCB_SHAPE( aParent, aType, SHAPE_T::RECTANGLE ),
    EDA_TEXT( pcbIUScale ),
    m_borderEnabled( true )
{
    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    SetMultilineAllowed( true );

    int defaultMargin = GetLegacyTextMargin();
    m_marginLeft = defaultMargin;
    m_marginTop = defaultMargin;
    m_marginRight = defaultMargin;
    m_marginBottom = defaultMargin;
}


PCB_TEXTBOX::~PCB_TEXTBOX()
{
}


void PCB_TEXTBOX::StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings )
{
    PCB_SHAPE::StyleFromSettings( settings );

    SetTextSize( settings.GetTextSize( GetLayer() ) );
    SetTextThickness( settings.GetTextThickness( GetLayer() ) );
    SetItalic( settings.GetTextItalic( GetLayer() ) );
    SetKeepUpright( settings.GetTextUpright( GetLayer() ) );
    SetMirrored( IsBackLayer( GetLayer() ) );
}


int PCB_TEXTBOX::GetLegacyTextMargin() const
{
    return KiROUND( GetStroke().GetWidth() / 2.0 ) + KiROUND( GetTextSize().y * 0.75 );
}


VECTOR2I PCB_TEXTBOX::GetTopLeft() const
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 )
        return VECTOR2I( GetStartX(), GetEndY() );
    else if( rotation == ANGLE_180 )
        return GetEnd();
    else if( rotation == ANGLE_270 )
        return VECTOR2I( GetEndX(), GetStartY() );
    else
        return GetStart();
}


VECTOR2I PCB_TEXTBOX::GetBotRight() const
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 )
        return VECTOR2I( GetEndX(), GetStartY() );
    else if( rotation == ANGLE_180 )
        return GetStart();
    else if( rotation == ANGLE_270 )
        return VECTOR2I( GetStartX(), GetEndY() );
    else
        return GetEnd();
}


void PCB_TEXTBOX::SetTop( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 || rotation == ANGLE_180 )
        SetEndY( aVal );
    else
        SetStartY( aVal );
}


void PCB_TEXTBOX::SetBottom( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 || rotation == ANGLE_180 )
        SetStartY( aVal );
    else
        SetEndY( aVal );
}


void PCB_TEXTBOX::SetLeft( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_180 || rotation == ANGLE_270 )
        SetEndX( aVal );
    else
        SetStartX( aVal );
}


void PCB_TEXTBOX::SetRight( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_180 || rotation == ANGLE_270 )
        SetStartX( aVal );
    else
        SetEndX( aVal );
}


void PCB_TEXTBOX::SetTextAngle( const EDA_ANGLE& aAngle )
{
    EDA_ANGLE delta = aAngle.Normalized() - GetTextAngle();
    Rotate( GetPosition(), delta );
}


std::vector<VECTOR2I> PCB_TEXTBOX::GetAnchorAndOppositeCorner() const
{
    std::vector<VECTOR2I> pts;
    EDA_ANGLE             textAngle( GetDrawRotation() );

    textAngle.Normalize();

    if( textAngle.IsCardinal() )
    {
        BOX2I bbox = PCB_SHAPE::GetBoundingBox();
        bbox.Normalize();

        if( textAngle == ANGLE_0 )
        {
            pts.emplace_back( VECTOR2I( bbox.GetLeft(), bbox.GetTop() ) );
            pts.emplace_back( VECTOR2I( bbox.GetRight(), bbox.GetTop() ) );
        }
        else if( textAngle == ANGLE_90 )
        {
            pts.emplace_back( VECTOR2I( bbox.GetLeft(), bbox.GetBottom() ) );
            pts.emplace_back( VECTOR2I( bbox.GetLeft(), bbox.GetTop() ) );
        }
        else if( textAngle == ANGLE_180 )
        {
            pts.emplace_back( VECTOR2I( bbox.GetRight(), bbox.GetBottom() ) );
            pts.emplace_back( VECTOR2I( bbox.GetLeft(), bbox.GetBottom() ) );
        }
        else if( textAngle == ANGLE_270 )
        {
            pts.emplace_back( VECTOR2I( bbox.GetRight(), bbox.GetTop() ) );
            pts.emplace_back( VECTOR2I( bbox.GetRight(), bbox.GetBottom() ) );
        }
    }
    else
    {
        std::vector<VECTOR2I> corners = GetCorners();

        VECTOR2I minX = corners[0];
        VECTOR2I maxX = corners[0];
        VECTOR2I minY = corners[0];
        VECTOR2I maxY = corners[0];

        for( const VECTOR2I& corner : corners )
        {
            if( corner.x < minX.x )
                minX = corner;

            if( corner.x > maxX.x )
                maxX = corner;

            if( corner.y < minY.y )
                minY = corner;

            if( corner.y > maxY.y )
                maxY = corner;
        }

        if( textAngle < ANGLE_90 )
        {
            pts.emplace_back( minX );
            pts.emplace_back( minY );
        }
        else if( textAngle < ANGLE_180 )
        {
            pts.emplace_back( maxY );
            pts.emplace_back( minX );
        }
        else if( textAngle < ANGLE_270 )
        {
            pts.emplace_back( maxX );
            pts.emplace_back( maxY );
        }
        else
        {
            pts.emplace_back( minY );
            pts.emplace_back( maxX );
        }
    }

    return pts;
}


VECTOR2I PCB_TEXTBOX::GetDrawPos() const
{
    return GetDrawPos( false );
}


VECTOR2I PCB_TEXTBOX::GetDrawPos( bool aIsFlipped ) const
{
    std::vector<VECTOR2I> corners = GetAnchorAndOppositeCorner();
    GR_TEXT_H_ALIGN_T     effectiveAlignment = GetHorizJustify();
    VECTOR2I              textAnchor;
    VECTOR2I              offset;

    if( IsMirrored() != aIsFlipped )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:          effectiveAlignment = GR_TEXT_H_ALIGN_RIGHT;   break;
        case GR_TEXT_H_ALIGN_CENTER:        effectiveAlignment = GR_TEXT_H_ALIGN_CENTER;  break;
        case GR_TEXT_H_ALIGN_RIGHT:         effectiveAlignment = GR_TEXT_H_ALIGN_LEFT;    break;
        case GR_TEXT_H_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Legal only in dialogs" ) ); break;
        }
    }

    switch( effectiveAlignment )
    {
    case GR_TEXT_H_ALIGN_LEFT:
        textAnchor = corners[0];
        offset = VECTOR2I( GetMarginLeft(), GetMarginTop() );
        break;
    case GR_TEXT_H_ALIGN_CENTER:
        textAnchor = ( corners[0] + corners[1] ) / 2;
        offset = VECTOR2I( 0, GetMarginTop() );
        break;
    case GR_TEXT_H_ALIGN_RIGHT:
        textAnchor = corners[1];
        offset = VECTOR2I( -GetMarginRight(), GetMarginTop() );
        break;
    case GR_TEXT_H_ALIGN_INDETERMINATE:
        wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
        break;
    }

    RotatePoint( offset, GetDrawRotation() );
    return textAnchor + offset;
}


double PCB_TEXTBOX::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( aView->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow if the main layer is not shown
        if( !aView->IsLayerVisible( m_layer ) )
            return HIDE;

        // Hide shadow on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return HIDE;
        }
    }

    return 0.0;
}


void PCB_TEXTBOX::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aLayers[0] = GetLayer();
    aCount = 1;

    if( IsLocked() )
        aLayers[ aCount++ ] = LAYER_LOCKED_ITEM_SHADOW;
}


wxString PCB_TEXTBOX::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    const FOOTPRINT* parentFootprint = GetParentFootprint();
    const BOARD*     board = GetBoard();

    std::function<bool( wxString* )> resolver = [&]( wxString* token ) -> bool
    {
        if( parentFootprint && parentFootprint->ResolveTextVar( token, aDepth + 1 ) )
            return true;

        if( token->IsSameAs( wxT( "LAYER" ) ) )
        {
            *token = GetLayerName();
            return true;
        }

        if( board->ResolveTextVar( token, aDepth + 1 ) )
            return true;

        return false;
    };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( HasTextVars() )
    {
        if( aDepth < ADVANCED_CFG::GetCfg().m_ResolveTextRecursionDepth )
            text = ExpandTextVars( text, &resolver );
    }

    KIFONT::FONT*         font = getDrawFont();
    std::vector<VECTOR2I> corners = GetAnchorAndOppositeCorner();
    int                   colWidth = ( corners[1] - corners[0] ).EuclideanNorm();

    if( GetTextAngle().IsHorizontal() )
        colWidth -= ( GetMarginLeft() + GetMarginRight() );
    else
        colWidth -= ( GetMarginTop() + GetMarginBottom() );

    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    return text;
}


bool PCB_TEXTBOX::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return BOARD_ITEM::Matches( UnescapeString( GetText() ), aSearchData );
}


void PCB_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );
    aList.emplace_back( _( "Angle" ), wxString::Format( "%g", GetTextAngle().AsDegrees() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );
    aList.emplace_back( _( "Text Thickness" ), aFrame->MessageTextFromValue( GetTextThickness() ) );
    aList.emplace_back( _( "Text Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Text Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );

    aList.emplace_back( _( "Box Width" ),
                        aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );

    aList.emplace_back( _( "Box Height" ),
                        aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ));

    m_stroke.GetMsgPanelInfo( aFrame, aList );
}


void PCB_TEXTBOX::Move( const VECTOR2I& aMoveVector )
{
    PCB_SHAPE::Move( aMoveVector );
    EDA_TEXT::Offset( aMoveVector );
}


void PCB_TEXTBOX::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    PCB_SHAPE::Rotate( aRotCentre, aAngle );
    EDA_TEXT::SetTextAngle( ( GetTextAngle() + aAngle ).Normalized() );

    if( GetTextAngle().IsCardinal() && GetShape() != SHAPE_T::RECTANGLE )
    {
        std::vector<VECTOR2I> corners = GetCorners();
        VECTOR2I              diag = corners[2] - corners[0];
        EDA_ANGLE             angle = GetTextAngle();

        SetShape( SHAPE_T::RECTANGLE );
        SetStart( corners[0] );

        angle.Normalize();

        if( angle == ANGLE_90 )
            SetEnd( VECTOR2I( corners[0].x + abs( diag.x ), corners[0].y - abs( diag.y ) ) );
        else if( angle == ANGLE_180 )
            SetEnd( VECTOR2I( corners[0].x - abs( diag.x ), corners[0].y - abs( diag.y ) ) );
        else if( angle == ANGLE_270 )
            SetEnd( VECTOR2I( corners[0].x - abs( diag.x ), corners[0].y + abs( diag.y ) ) );
        else
            SetEnd( VECTOR2I( corners[0].x + abs( diag.x ), corners[0].y + abs( diag.y ) ) );
    }
}


void PCB_TEXTBOX::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    // the position and angle are mirrored, but not the text (or its justification)
    PCB_SHAPE::Mirror( aCentre, aMirrorAroundXAxis );

    if( aMirrorAroundXAxis )
        EDA_TEXT::SetTextAngle( ANGLE_180 - GetTextAngle() );
    else
        EDA_TEXT::SetTextAngle( -GetTextAngle() );
}


void PCB_TEXTBOX::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    PCB_SHAPE::Flip( aCentre, aFlipLeftRight );

    if( aFlipLeftRight )
        EDA_TEXT::SetTextAngle( -GetTextAngle() );
    else
        EDA_TEXT::SetTextAngle( ANGLE_180 - GetTextAngle() );

    if( IsSideSpecific() )
        SetMirrored( !IsMirrored() );
}


bool PCB_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool PCB_TEXTBOX::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


wxString PCB_TEXTBOX::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "PCB Text Box '%s' on %s" ),
                             aFull ? GetShownText( false ) : KIUI::EllipsizeMenuText( GetText() ),
                             GetLayerName() );
}


BITMAPS PCB_TEXTBOX::GetMenuImage() const
{
    return BITMAPS::add_textbox;
}


EDA_ITEM* PCB_TEXTBOX::Clone() const
{
    return new PCB_TEXTBOX( *this );
}


void PCB_TEXTBOX::swapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == PCB_TEXTBOX_T );

    std::swap( *((PCB_TEXTBOX*) this), *((PCB_TEXTBOX*) aImage) );
}


std::shared_ptr<SHAPE> PCB_TEXTBOX::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    std::shared_ptr<SHAPE_COMPOUND> shape = GetEffectiveTextShape();

    if( PCB_SHAPE::GetStroke().GetWidth() >= 0 )
        shape->AddShape( PCB_SHAPE::GetEffectiveShape( aLayer, aFlash ) );

    return shape;
}


void PCB_TEXTBOX::TransformTextToPolySet( SHAPE_POLY_SET& aBuffer, int aClearance, int aMaxError,
                                          ERROR_LOC aErrorLoc ) const
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = getDrawFont();
    int                        penWidth = GetEffectiveTextPenWidth();

    // Note: this function is mainly used in 3D viewer.
    // the polygonal shape of a text can have many basic shapes,
    // so combining these shapes can be very useful to create a final shape
    // swith a lot less vertices to speedup calculations using this final shape
    // Simplify shapes is not usually always efficient, but in this case it is.
    SHAPE_POLY_SET buffer;

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                TransformOvalToPolygon( buffer, aPt1, aPt2, penWidth, aMaxError, aErrorLoc );
            },
            // Triangulation callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
            {
                buffer.NewOutline();

                for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                    buffer.Append( point.x, point.y );
            } );

    font->Draw( &callback_gal, GetShownText( true ), GetDrawPos(), GetAttributes(), GetFontMetrics() );

    if( aClearance > 0 || aErrorLoc == ERROR_OUTSIDE )
    {
        if( aErrorLoc == ERROR_OUTSIDE )
            aClearance += aMaxError;

        buffer.Inflate( aClearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aMaxError, true );
    }
    else
    {
        buffer.Simplify( SHAPE_POLY_SET::PM_FAST );
    }

    aBuffer.Append( buffer );
}


void PCB_TEXTBOX::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                           int aClearance, int aMaxError, ERROR_LOC aErrorLoc,
                                           bool aIgnoreLineWidth ) const
{
    // Don't use PCB_SHAPE::TransformShapeToPolygon.  We want to treat the textbox as filled even
    // if there's no background colour.

    int width = GetWidth() + ( 2 * aClearance );

    if( GetShape() == SHAPE_T::RECTANGLE )
    {
        std::vector<VECTOR2I> pts = GetRectCorners();

        aBuffer.NewOutline();

        for( const VECTOR2I& pt : pts )
            aBuffer.Append( pt );

        if( m_borderEnabled && width > 0 )
        {
            // Add in segments
            TransformOvalToPolygon( aBuffer, pts[0], pts[1], width, aMaxError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[1], pts[2], width, aMaxError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[2], pts[3], width, aMaxError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[3], pts[0], width, aMaxError, aErrorLoc );
        }
    }
    else if( GetShape() == SHAPE_T::POLY )  // Non-cardinally-rotated rect
    {
        aBuffer.NewOutline();

        const SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            aBuffer.Append( poly.GetPoint( ii ) );

        if( m_borderEnabled && width > 0 )
        {
            for( int ii = 0; ii < poly.SegmentCount(); ++ii )
            {
                const SEG& seg = poly.GetSegment( ii );
                TransformOvalToPolygon( aBuffer, seg.A, seg.B, width, aMaxError, aErrorLoc );
            }
        }
    }
}


bool PCB_TEXTBOX::IsBorderEnabled() const
{
    return m_borderEnabled;
}


void PCB_TEXTBOX::SetBorderEnabled( bool enabled )
{
    m_borderEnabled = enabled;
}


void PCB_TEXTBOX::SetBorderWidth( const int aSize )
{
    m_stroke.SetWidth( aSize );
}


bool PCB_TEXTBOX::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_TEXTBOX& other = static_cast<const PCB_TEXTBOX&>( aBoardItem );

    return *this == other;
}


bool PCB_TEXTBOX::operator==( const PCB_TEXTBOX& aOther ) const
{
    return m_borderEnabled == aOther.m_borderEnabled
            && EDA_TEXT::operator==( aOther );
}


double PCB_TEXTBOX::Similarity( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return 0.0;

    const PCB_TEXTBOX& other = static_cast<const PCB_TEXTBOX&>( aBoardItem );

    double similarity = 1.0;

    if( m_borderEnabled != other.m_borderEnabled )
        similarity *= 0.9;

    if( GetMarginLeft() != other.GetMarginLeft() )
        similarity *= 0.9;

    if( GetMarginTop() != other.GetMarginTop() )
        similarity *= 0.9;

    if( GetMarginRight() != other.GetMarginRight() )
        similarity *= 0.9;

    if( GetMarginBottom() != other.GetMarginBottom() )
        similarity *= 0.9;

    similarity *= EDA_TEXT::Similarity( other );

    return similarity;
}


static struct PCB_TEXTBOX_DESC
{
    PCB_TEXTBOX_DESC()
    {
        ENUM_MAP<LINE_STYLE>& plotDashTypeEnum = ENUM_MAP<LINE_STYLE>::Instance();

        if( plotDashTypeEnum.Choices().GetCount() == 0 )
        {
            plotDashTypeEnum.Map( LINE_STYLE::DEFAULT, _HKI( "Default" ) )
                            .Map( LINE_STYLE::SOLID, _HKI( "Solid" ) )
                            .Map( LINE_STYLE::DASH, _HKI( "Dashed" ) )
                            .Map( LINE_STYLE::DOT, _HKI( "Dotted" ) )
                            .Map( LINE_STYLE::DASHDOT, _HKI( "Dash-Dot" ) )
                            .Map( LINE_STYLE::DASHDOTDOT, _HKI( "Dash-Dot-Dot" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TEXTBOX );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXTBOX, PCB_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXTBOX, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( PCB_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Shape" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start X" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "End X" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "End Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Height" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Width" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Style" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Filled" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Visible" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Color" ) );

        const wxString borderProps = _( "Border Properties" );

        void ( PCB_TEXTBOX::*lineStyleSetter )( LINE_STYLE ) = &PCB_TEXTBOX::SetLineStyle;
        LINE_STYLE ( PCB_TEXTBOX::*lineStyleGetter )() const = &PCB_TEXTBOX::GetLineStyle;

        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, bool>( _HKI( "Border" ),
                    &PCB_TEXTBOX::SetBorderEnabled, &PCB_TEXTBOX::IsBorderEnabled ),
                borderProps );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TEXTBOX, LINE_STYLE>( _HKI( "Border Style" ),
                    lineStyleSetter, lineStyleGetter ),
                borderProps );

        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Border Width" ),
                    &PCB_TEXTBOX::SetBorderWidth, &PCB_TEXTBOX::GetBorderWidth,
                    PROPERTY_DISPLAY::PT_SIZE ),
                borderProps );

        const wxString marginProps = _( "Margins" );

        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Margin Left" ),
                    &PCB_TEXTBOX::SetMarginLeft, &PCB_TEXTBOX::GetMarginLeft,
                    PROPERTY_DISPLAY::PT_SIZE ),
                marginProps );
        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Margin Top" ),
                    &PCB_TEXTBOX::SetMarginTop, &PCB_TEXTBOX::GetMarginTop,
                    PROPERTY_DISPLAY::PT_SIZE ),
                marginProps );
        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Margin Right" ),
                    &PCB_TEXTBOX::SetMarginRight, &PCB_TEXTBOX::GetMarginRight,
                    PROPERTY_DISPLAY::PT_SIZE ),
                marginProps );
        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Margin Bottom" ),
                    &PCB_TEXTBOX::SetMarginBottom, &PCB_TEXTBOX::GetMarginBottom,
                    PROPERTY_DISPLAY::PT_SIZE ),
                marginProps );

    }
} _PCB_TEXTBOX_DESC;
