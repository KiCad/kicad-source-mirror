/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
#include <geometry/shape_rect.h>
#include <geometry/geometry_utils.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>
#include <macros.h>
#include <core/ignore.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_types.pb.h>


PCB_TEXTBOX::PCB_TEXTBOX( BOARD_ITEM* aParent, KICAD_T aType ) :
        PCB_SHAPE( aParent, aType, SHAPE_T::RECTANGLE ),
        EDA_TEXT( pcbIUScale ),
        m_borderEnabled( true )
{
    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
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


void PCB_TEXTBOX::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_TEXTBOX_T, /* void */ );
    *this = *static_cast<const PCB_TEXTBOX*>( aOther );
}


void PCB_TEXTBOX::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::common::types;
    using namespace kiapi::board;
    types::BoardTextBox boardText;
    boardText.set_layer( ToProtoEnum<PCB_LAYER_ID, types::BoardLayer>( GetLayer() ) );
    boardText.mutable_id()->set_value( m_Uuid.AsStdString() );
    boardText.set_locked( IsLocked() ? LockedState::LS_LOCKED : LockedState::LS_UNLOCKED );

    TextBox& text = *boardText.mutable_textbox();

    kiapi::common::PackVector2( *text.mutable_top_left(), GetPosition() );
    kiapi::common::PackVector2( *text.mutable_bottom_right(), GetEnd() );
    text.set_text( GetText().ToStdString() );
    //text.set_hyperlink( GetHyperlink().ToStdString() );

    TextAttributes* attrs = text.mutable_attributes();

    if( GetFont() )
        attrs->set_font_name( GetFont()->GetName().ToStdString() );

    attrs->set_horizontal_alignment( ToProtoEnum<GR_TEXT_H_ALIGN_T, HorizontalAlignment>( GetHorizJustify() ) );

    attrs->set_vertical_alignment( ToProtoEnum<GR_TEXT_V_ALIGN_T, VerticalAlignment>( GetVertJustify() ) );

    attrs->mutable_angle()->set_value_degrees( GetTextAngleDegrees() );
    attrs->set_line_spacing( GetLineSpacing() );
    attrs->mutable_stroke_width()->set_value_nm( GetTextThickness() );
    attrs->set_italic( IsItalic() );
    attrs->set_bold( IsBold() );
    attrs->set_underlined( GetAttributes().m_Underlined );
    attrs->set_mirrored( IsMirrored() );
    attrs->set_multiline( IsMultilineAllowed() );
    attrs->set_keep_upright( IsKeepUpright() );
    kiapi::common::PackVector2( *attrs->mutable_size(), GetTextSize() );

    aContainer.PackFrom( boardText );
}


bool PCB_TEXTBOX::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::board;
    types::BoardTextBox boardText;

    if( !aContainer.UnpackTo( &boardText ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( boardText.id().value() );
    SetLayer( FromProtoEnum<PCB_LAYER_ID, types::BoardLayer>( boardText.layer() ) );
    SetLocked( boardText.locked() == kiapi::common::types::LockedState::LS_LOCKED );

    const kiapi::common::types::TextBox& text = boardText.textbox();

    SetPosition( kiapi::common::UnpackVector2( text.top_left() ) );
    SetEnd( kiapi::common::UnpackVector2( text.bottom_right() ) );
    SetText( wxString( text.text().c_str(), wxConvUTF8 ) );
    //SetHyperlink( wxString::FromUTF8( text.hyperlink() );

    if( text.has_attributes() )
    {
        TEXT_ATTRIBUTES attrs = GetAttributes();

        attrs.m_Bold = text.attributes().bold();
        attrs.m_Italic = text.attributes().italic();
        attrs.m_Underlined = text.attributes().underlined();
        attrs.m_Mirrored = text.attributes().mirrored();
        attrs.m_Multiline = text.attributes().multiline();
        attrs.m_KeepUpright = text.attributes().keep_upright();
        attrs.m_Size = kiapi::common::UnpackVector2( text.attributes().size() );

        if( !text.attributes().font_name().empty() )
        {
            attrs.m_Font = KIFONT::FONT::GetFont( wxString( text.attributes().font_name().c_str(), wxConvUTF8 ),
                                                  attrs.m_Bold, attrs.m_Italic );
        }

        attrs.m_Angle = EDA_ANGLE( text.attributes().angle().value_degrees(), DEGREES_T );
        attrs.m_LineSpacing = text.attributes().line_spacing();
        attrs.m_StrokeWidth = text.attributes().stroke_width().value_nm();
        attrs.m_Halign = FromProtoEnum<GR_TEXT_H_ALIGN_T, kiapi::common::types::HorizontalAlignment>(
                text.attributes().horizontal_alignment() );

        attrs.m_Valign = FromProtoEnum<GR_TEXT_V_ALIGN_T, kiapi::common::types::VerticalAlignment>(
                text.attributes().vertical_alignment() );

        SetAttributes( attrs );
    }

    return true;
}


void PCB_TEXTBOX::StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings, bool aCheckSide )
{
    PCB_SHAPE::StyleFromSettings( settings, aCheckSide );

    SetTextSize( settings.GetTextSize( GetLayer() ) );
    SetTextThickness( settings.GetTextThickness( GetLayer() ) );
    SetItalic( settings.GetTextItalic( GetLayer() ) );

    if( GetParentFootprint() )
        SetKeepUpright( settings.GetTextUpright( GetLayer() ) );

    if( aCheckSide )
    {
        if( BOARD* board = GetBoard() )
            SetMirrored( board->IsBackLayer( GetLayer() ) );
        else
            SetMirrored( IsBackLayer( GetLayer() ) );
    }
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


VECTOR2I PCB_TEXTBOX::GetDrawPos() const
{
    return GetDrawPos( false );
}


VECTOR2I PCB_TEXTBOX::GetDrawPos( bool aIsFlipped ) const
{
    EDA_ANGLE             drawAngle = GetDrawRotation();
    std::vector<VECTOR2I> corners = GetCornersInSequence( drawAngle );
    GR_TEXT_H_ALIGN_T     horizontalAlignment = GetHorizJustify();
    GR_TEXT_V_ALIGN_T     verticalAlignment = GetVertJustify();
    VECTOR2I              textAnchor;
    VECTOR2I              offset;

    // Calculate midpoints
    VECTOR2I midTop = ( corners[0] + corners[1] ) / 2;
    VECTOR2I midBottom = ( corners[3] + corners[2] ) / 2;
    VECTOR2I midLeft = ( corners[0] + corners[3] ) / 2;
    VECTOR2I midRight = ( corners[1] + corners[2] ) / 2;
    VECTOR2I center = ( corners[0] + corners[1] + corners[2] + corners[3] ) / 4;

    if( IsMirrored() != aIsFlipped )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT: horizontalAlignment = GR_TEXT_H_ALIGN_RIGHT; break;
        case GR_TEXT_H_ALIGN_CENTER: horizontalAlignment = GR_TEXT_H_ALIGN_CENTER; break;
        case GR_TEXT_H_ALIGN_RIGHT: horizontalAlignment = GR_TEXT_H_ALIGN_LEFT; break;
        case GR_TEXT_H_ALIGN_INDETERMINATE: horizontalAlignment = GR_TEXT_H_ALIGN_INDETERMINATE; break;
        }
    }

    wxASSERT_MSG( horizontalAlignment != GR_TEXT_H_ALIGN_INDETERMINATE
                          && verticalAlignment != GR_TEXT_V_ALIGN_INDETERMINATE,
                  wxS( "Indeterminate state legal only in dialogs. Horizontal and vertical alignment "
                       "must be set before calling PCB_TEXTBOX::GetDrawPos." ) );

    if( horizontalAlignment == GR_TEXT_H_ALIGN_INDETERMINATE || verticalAlignment == GR_TEXT_V_ALIGN_INDETERMINATE )
    {
        return center;
    }

    if( horizontalAlignment == GR_TEXT_H_ALIGN_LEFT && verticalAlignment == GR_TEXT_V_ALIGN_TOP )
    {
        textAnchor = corners[0];
    }
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_CENTER && verticalAlignment == GR_TEXT_V_ALIGN_TOP )
    {
        textAnchor = midTop;
    }
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_RIGHT && verticalAlignment == GR_TEXT_V_ALIGN_TOP )
    {
        textAnchor = corners[1];
    }
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_LEFT && verticalAlignment == GR_TEXT_V_ALIGN_CENTER )
    {
        textAnchor = midLeft;
    }
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_CENTER && verticalAlignment == GR_TEXT_V_ALIGN_CENTER )
    {
        textAnchor = center;
    }
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_RIGHT && verticalAlignment == GR_TEXT_V_ALIGN_CENTER )
    {
        textAnchor = midRight;
    }
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_LEFT && verticalAlignment == GR_TEXT_V_ALIGN_BOTTOM )
    {
        textAnchor = corners[3];
    }
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_CENTER && verticalAlignment == GR_TEXT_V_ALIGN_BOTTOM )
    {
        textAnchor = midBottom;
    }
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_RIGHT && verticalAlignment == GR_TEXT_V_ALIGN_BOTTOM )
    {
        textAnchor = corners[2];
    }

    int marginLeft = GetMarginLeft();
    int marginRight = GetMarginRight();
    int marginTop = GetMarginTop();
    int marginBottom = GetMarginBottom();

    if( horizontalAlignment == GR_TEXT_H_ALIGN_LEFT )
        offset.x = marginLeft;
    else if( horizontalAlignment == GR_TEXT_H_ALIGN_RIGHT )
        offset.x = -marginRight;

    if( verticalAlignment == GR_TEXT_V_ALIGN_TOP )
        offset.y = marginTop;
    else if( verticalAlignment == GR_TEXT_V_ALIGN_BOTTOM )
        offset.y = -marginBottom;

    RotatePoint( offset, GetDrawRotation() );
    return textAnchor + offset;
}


double PCB_TEXTBOX::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    KIGFX::PCB_PAINTER&         painter = static_cast<KIGFX::PCB_PAINTER&>( *aView->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS& renderSettings = *painter.GetSettings();

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow if the main layer is not shown
        if( !aView->IsLayerVisible( m_layer ) )
            return LOD_HIDE;

        // Hide shadow on dimmed tracks
        if( renderSettings.GetHighContrast() )
        {
            if( m_layer != renderSettings.GetPrimaryHighContrastLayer() )
                return LOD_HIDE;
        }
    }

    return LOD_SHOW;
}


std::vector<int> PCB_TEXTBOX::ViewGetLayers() const
{
    if( IsLocked() || ( GetParentFootprint() && GetParentFootprint()->IsLocked() ) )
        return { GetLayer(), LAYER_LOCKED_ITEM_SHADOW };

    return { GetLayer() };
}


wxString PCB_TEXTBOX::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    const FOOTPRINT* parentFootprint = GetParentFootprint();
    const BOARD*     board = GetBoard();

    std::function<bool( wxString* )> resolver = [&]( wxString* token ) -> bool
    {
        if( token->IsSameAs( wxT( "LAYER" ) ) )
        {
            *token = GetLayerName();
            return true;
        }

        if( parentFootprint && parentFootprint->ResolveTextVar( token, aDepth + 1 ) )
            return true;

        if( board->ResolveTextVar( token, aDepth + 1 ) )
            return true;

        return false;
    };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( HasTextVars() )
        text = ResolveTextVars( text, &resolver, aDepth );

    KIFONT::FONT*         font = GetDrawFont( nullptr );
    EDA_ANGLE             drawAngle = GetDrawRotation();
    std::vector<VECTOR2I> corners = GetCornersInSequence( drawAngle );
    int                   colWidth = ( corners[1] - corners[0] ).EuclideanNorm();

    if( GetTextAngle().IsHorizontal() )
        colWidth -= ( GetMarginLeft() + GetMarginRight() );
    else
        colWidth -= ( GetMarginTop() + GetMarginBottom() );

    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    // Convert escape markers back to literal ${} and @{} for final display
    text.Replace( wxT( "<<<ESC_DOLLAR:" ), wxT( "${" ) );
    text.Replace( wxT( "<<<ESC_AT:" ), wxT( "@{" ) );

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

    if( GetTextThickness() )
        aList.emplace_back( _( "Text Thickness" ), aFrame->MessageTextFromValue( GetEffectiveTextPenWidth() ) );
    else
        aList.emplace_back( _( "Text Thickness" ), _( "Auto" ) );

    aList.emplace_back( _( "Text Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Text Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );

    aList.emplace_back( _( "Box Width" ), aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );

    aList.emplace_back( _( "Box Height" ), aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ) );

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
    EDA_TEXT::SetTextAngle( ( GetTextAngle() + aAngle ).Normalize() );

    if( GetTextAngle().IsCardinal() && GetShape() != SHAPE_T::RECTANGLE )
    {
        // To convert the polygon to its equivalent rectangle, we use GetCornersInSequence( drawAngle )
        // but this method uses the polygon bounding box.
        // set the line thickness to 0 to get the actual rectangle corner
        int lineWidth = GetWidth();
        SetWidth( 0 );
        EDA_ANGLE             drawAngle = GetDrawRotation();
        std::vector<VECTOR2I> corners = GetCornersInSequence( drawAngle );
        SetWidth( lineWidth );
        VECTOR2I  diag = corners[2] - corners[0];
        EDA_ANGLE angle = GetTextAngle();

        SetShape( SHAPE_T::RECTANGLE );
        SetStart( corners[0] );

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


void PCB_TEXTBOX::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    // the position and angle are mirrored, but not the text (or its justification)
    PCB_SHAPE::Mirror( aCentre, aFlipDirection );

    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
        EDA_TEXT::SetTextAngle( ANGLE_180 - GetTextAngle() );
    else
        EDA_TEXT::SetTextAngle( -GetTextAngle() );
}


void PCB_TEXTBOX::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    PCB_SHAPE::Flip( aCentre, aFlipDirection );

    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
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


bool PCB_TEXTBOX::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    return PCB_SHAPE::HitTest( aPoly, aContained );
}


wxString PCB_TEXTBOX::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "PCB text box '%s' on %s" ),
                             aFull ? GetShownText( false ) : KIUI::EllipsizeMenuText( GetText() ), GetLayerName() );
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

    std::swap( *( (PCB_TEXTBOX*) this ), *( (PCB_TEXTBOX*) aImage ) );
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
    KIFONT::FONT*              font = GetDrawFont( nullptr );
    int                        penWidth = GetEffectiveTextPenWidth();
    TEXT_ATTRIBUTES            attrs = GetAttributes();
    wxString                   shownText = GetShownText( true );

    // The polygonal shape of a text can have many basic shapes, so combining these shapes can
    // be very useful to create a final shape with a lot less vertices to speedup calculations.
    // Simplify shapes is not usually always efficient, but in this case it is.
    SHAPE_POLY_SET textShape;

    CALLBACK_GAL callback_gal(
            empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                TransformOvalToPolygon( textShape, aPt1, aPt2, penWidth, aMaxError, aErrorLoc );
            },
            // Triangulation callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
            {
                textShape.NewOutline();

                for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                    textShape.Append( point.x, point.y );
            } );

    if( auto* cache = GetRenderCache( font, shownText ) )
        callback_gal.DrawGlyphs( *cache );
    else
        font->Draw( &callback_gal, shownText, GetDrawPos(), attrs, GetFontMetrics() );

    textShape.Simplify();

    if( IsKnockout() )
    {
        SHAPE_POLY_SET finalPoly;

        TransformShapeToPolygon( finalPoly, GetLayer(), aClearance, aMaxError, aErrorLoc );
        finalPoly.BooleanSubtract( textShape );

        aBuffer.Append( finalPoly );
    }
    else
    {
        if( aClearance > 0 || aErrorLoc == ERROR_OUTSIDE )
        {
            if( aErrorLoc == ERROR_OUTSIDE )
                aClearance += aMaxError;

            textShape.Inflate( aClearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aMaxError, true );
        }

        aBuffer.Append( textShape );
    }
}


void PCB_TEXTBOX::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance, int aMaxError,
                                           ERROR_LOC aErrorLoc, bool aIgnoreLineWidth ) const
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
    else if( GetShape() == SHAPE_T::POLY ) // Non-cardinally-rotated rect
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
    return m_borderEnabled == aOther.m_borderEnabled && EDA_TEXT::operator==( aOther );
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
        ENUM_MAP<LINE_STYLE>& lineStyleEnum = ENUM_MAP<LINE_STYLE>::Instance();

        if( lineStyleEnum.Choices().GetCount() == 0 )
        {
            lineStyleEnum.Map( LINE_STYLE::SOLID, _HKI( "Solid" ) )
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
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Corner Radius" ) );

        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Color" ) );

        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( PCB_SHAPE ), _HKI( "Soldermask" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( PCB_SHAPE ), _HKI( "Soldermask Margin Override" ) );

        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, bool, BOARD_ITEM>(
                                     _HKI( "Knockout" ), &BOARD_ITEM::SetIsKnockout, &BOARD_ITEM::IsKnockout ),
                             _HKI( "Text Properties" ) );

        const wxString borderProps = _( "Border Properties" );

        void ( PCB_TEXTBOX::*lineStyleSetter )( LINE_STYLE ) = &PCB_TEXTBOX::SetLineStyle;
        LINE_STYLE ( PCB_TEXTBOX::*lineStyleGetter )() const = &PCB_TEXTBOX::GetLineStyle;

        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, bool>( _HKI( "Border" ), &PCB_TEXTBOX::SetBorderEnabled,
                                                              &PCB_TEXTBOX::IsBorderEnabled ),
                             borderProps );

        propMgr.AddProperty(
                new PROPERTY_ENUM<PCB_TEXTBOX, LINE_STYLE>( _HKI( "Border Style" ), lineStyleSetter, lineStyleGetter ),
                borderProps );

        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Border Width" ), &PCB_TEXTBOX::SetBorderWidth,
                                                             &PCB_TEXTBOX::GetBorderWidth, PROPERTY_DISPLAY::PT_SIZE ),
                             borderProps );

        const wxString marginProps = _( "Margins" );

        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Margin Left" ), &PCB_TEXTBOX::SetMarginLeft,
                                                             &PCB_TEXTBOX::GetMarginLeft, PROPERTY_DISPLAY::PT_SIZE ),
                             marginProps );
        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Margin Top" ), &PCB_TEXTBOX::SetMarginTop,
                                                             &PCB_TEXTBOX::GetMarginTop, PROPERTY_DISPLAY::PT_SIZE ),
                             marginProps );
        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Margin Right" ), &PCB_TEXTBOX::SetMarginRight,
                                                             &PCB_TEXTBOX::GetMarginRight, PROPERTY_DISPLAY::PT_SIZE ),
                             marginProps );
        propMgr.AddProperty( new PROPERTY<PCB_TEXTBOX, int>( _HKI( "Margin Bottom" ), &PCB_TEXTBOX::SetMarginBottom,
                                                             &PCB_TEXTBOX::GetMarginBottom, PROPERTY_DISPLAY::PT_SIZE ),
                             marginProps );

        propMgr.Mask( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );
    }
} _PCB_TEXTBOX_DESC;
