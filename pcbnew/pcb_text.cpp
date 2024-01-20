/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <google/protobuf/any.pb.h>

#include <pcb_edit_frame.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <core/mirror.h>
#include <footprint.h>
#include <pcb_text.h>
#include <pcb_painter.h>
#include <trigo.h>
#include <string_utils.h>
#include <geometry/shape_compound.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_types.pb.h>


using namespace kiapi::common;


PCB_TEXT::PCB_TEXT( BOARD_ITEM* parent, KICAD_T idtype ) :
        BOARD_ITEM( parent, idtype ),
        EDA_TEXT( pcbIUScale )
{
    SetMultilineAllowed( true );
}


PCB_TEXT::PCB_TEXT( FOOTPRINT* aParent ) :
        BOARD_ITEM( aParent, PCB_TEXT_T ),
        EDA_TEXT( pcbIUScale )
{
    SetKeepUpright( true );

    // Set text thickness to a default value
    SetTextThickness( pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ) );
    SetLayer( F_SilkS );

    if( aParent )
    {
        SetTextPos( aParent->GetPosition() );

        if( IsBackLayer( aParent->GetLayer() ) )
        {
            SetLayer( B_SilkS );
            SetMirrored( true );
        }
    }
}


PCB_TEXT::~PCB_TEXT()
{
}


void PCB_TEXT::Serialize( google::protobuf::Any &aContainer ) const
{
    kiapi::board::types::Text boardText;
    boardText.set_layer( ToProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( GetLayer() ) );

    kiapi::common::types::Text& text = *boardText.mutable_text();

    text.mutable_id()->set_value( m_Uuid.AsStdString() );
    text.mutable_position()->set_x_nm( GetPosition().x );
    text.mutable_position()->set_y_nm( GetPosition().y );
    text.set_text( GetText().ToStdString() );
    text.set_hyperlink( GetHyperlink().ToStdString() );
    text.set_locked( IsLocked() ? types::LockedState::LS_LOCKED
                                : types::LockedState::LS_UNLOCKED );

    kiapi::common::types::TextAttributes* attrs = text.mutable_attributes();

    if( GetFont() )
        attrs->set_font_name( GetFont()->GetName().ToStdString() );

    attrs->set_horizontal_alignment(
            ToProtoEnum<GR_TEXT_H_ALIGN_T, types::HorizontalAlignment>( GetHorizJustify() ) );

    attrs->set_vertical_alignment(
            ToProtoEnum<GR_TEXT_V_ALIGN_T, types::VerticalAlignment>( GetVertJustify() ) );

    attrs->mutable_angle()->set_value_degrees( GetTextAngleDegrees() );
    attrs->set_line_spacing( GetLineSpacing() );
    attrs->mutable_stroke_width()->set_value_nm( GetTextThickness() );
    attrs->set_italic( IsItalic() );
    attrs->set_bold( IsBold() );
    attrs->set_underlined( GetAttributes().m_Underlined );
    attrs->set_visible( IsVisible() );
    attrs->set_mirrored( IsMirrored() );
    attrs->set_multiline( IsMultilineAllowed() );
    attrs->set_keep_upright( IsKeepUpright() );
    attrs->mutable_size()->set_x_nm( GetTextSize().x );
    attrs->mutable_size()->set_y_nm( GetTextSize().y );

    text.set_knockout( IsKnockout() );

    aContainer.PackFrom( boardText );
}


bool PCB_TEXT::Deserialize( const google::protobuf::Any &aContainer )
{
    kiapi::board::types::Text textWrapper;

    if( !aContainer.UnpackTo( &textWrapper ) )
        return false;

    SetLayer( FromProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( textWrapper.layer() ) );

    const kiapi::common::types::Text& text = textWrapper.text();

    const_cast<KIID&>( m_Uuid ) = KIID( text.id().value() );
    SetPosition( VECTOR2I( text.position().x_nm(), text.position().y_nm() ) );
    SetLocked( text.locked() == kiapi::common::types::LockedState::LS_LOCKED );
    SetText( wxString( text.text().c_str(), wxConvUTF8 ) );
    SetHyperlink( wxString( text.hyperlink().c_str(), wxConvUTF8 ) );
    SetIsKnockout( text.knockout() );

    if( text.has_attributes() )
    {
        TEXT_ATTRIBUTES attrs = GetAttributes();

        attrs.m_Bold = text.attributes().bold();
        attrs.m_Italic = text.attributes().italic();
        attrs.m_Underlined = text.attributes().underlined();
        attrs.m_Visible = text.attributes().visible();
        attrs.m_Mirrored = text.attributes().mirrored();
        attrs.m_Multiline = text.attributes().multiline();
        attrs.m_KeepUpright = text.attributes().keep_upright();
        attrs.m_Size = VECTOR2I( text.attributes().size().x_nm(), text.attributes().size().y_nm() );

        if( !text.attributes().font_name().empty() )
        {
            attrs.m_Font = KIFONT::FONT::GetFont(
                    wxString( text.attributes().font_name().c_str(), wxConvUTF8 ), attrs.m_Bold,
                    attrs.m_Italic );
        }

        attrs.m_Angle = EDA_ANGLE( text.attributes().angle().value_degrees(), DEGREES_T );
        attrs.m_LineSpacing = text.attributes().line_spacing();
        SetTextThickness( text.attributes().stroke_width().value_nm() );

        attrs.m_Halign = FromProtoEnum<GR_TEXT_H_ALIGN_T, types::HorizontalAlignment>(
                text.attributes().horizontal_alignment() );

        attrs.m_Valign = FromProtoEnum<GR_TEXT_V_ALIGN_T, types::VerticalAlignment>(
                text.attributes().vertical_alignment() );

        SetAttributes( attrs );
    }

    return true;
}


wxString PCB_TEXT::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    const FOOTPRINT* parentFootprint = GetParentFootprint();
    const BOARD*     board = GetBoard();

    std::function<bool( wxString* )> resolver =
            [&]( wxString* token ) -> bool
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
        if( aDepth < 10 )
            text = ExpandTextVars( text, &resolver );
    }

    return text;
}


bool PCB_TEXT::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return BOARD_ITEM::Matches( UnescapeString( GetText() ), aSearchData );
}


EDA_ANGLE PCB_TEXT::GetDrawRotation() const
{
    EDA_ANGLE rotation = GetTextAngle();

    if( GetParentFootprint() && IsKeepUpright() )
    {
        // Keep angle between ]-90..90] deg. Otherwise the text is not easy to read
        while( rotation > ANGLE_90 )
            rotation -= ANGLE_180;

        while( rotation <= -ANGLE_90 )
            rotation += ANGLE_180;
    }
    else
    {
        rotation.Normalize();
    }

    return rotation;
}


const BOX2I PCB_TEXT::ViewBBox() const
{
    return GetBoundingBox();
}


void PCB_TEXT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    if( GetParentFootprint() == nullptr || IsVisible() )
        aLayers[0] = GetLayer();
    else
        aLayers[0] = LAYER_HIDDEN_TEXT;

    aCount = 1;

    if( IsLocked() )
        aLayers[ aCount++ ] = LAYER_LOCKED_ITEM_SHADOW;
}


double PCB_TEXT::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    if( !aView )
        return 0.0;

    KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( aView->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    // Hidden text gets put on the LAYER_HIDDEN_TEXT for rendering, but
    // should only render if its native layer is visible.
    if( !aView->IsLayerVisible( GetLayer() ) )
        return HIDE;

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return HIDE;
        }
    }

    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        // Handle Render tab switches
        if( GetText() == wxT( "${VALUE}" ) )
        {
            if( !aView->IsLayerVisible( LAYER_FP_VALUES ) )
                return HIDE;
        }

        if( GetText() == wxT( "${REFERENCE}" ) )
        {
            if( !aView->IsLayerVisible( LAYER_FP_REFERENCES ) )
                return HIDE;
        }

        if( parentFP->GetLayer() == F_Cu && !aView->IsLayerVisible( LAYER_FOOTPRINTS_FR ) )
            return HIDE;

        if( parentFP->GetLayer() == B_Cu && !aView->IsLayerVisible( LAYER_FOOTPRINTS_BK ) )
            return HIDE;

        if( !aView->IsLayerVisible( LAYER_FP_TEXT ) )
            return HIDE;
    }

    return 0.0;
}


void PCB_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    FOOTPRINT* parentFP = GetParentFootprint();

    if( parentFP && aFrame->GetName() == PCB_EDIT_FRAME_NAME )
        aList.emplace_back( _( "Footprint" ), parentFP->GetReference() );

    // Don't use GetShownText() here; we want to show the user the variable references
    if( parentFP )
        aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );
    else
        aList.emplace_back( _( "PCB Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    if( parentFP )
        aList.emplace_back( _( "Type" ), GetTextTypeDescription() );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    if( parentFP )
        aList.emplace_back( _( "Display" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Angle" ), wxString::Format( wxT( "%g" ), GetTextAngle().AsDegrees() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );
    aList.emplace_back( _( "Thickness" ), aFrame->MessageTextFromValue( GetTextThickness() ) );
    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );
}


int PCB_TEXT::getKnockoutMargin() const
{
    return GetKnockoutTextMargin( VECTOR2I( GetTextWidth(), GetTextHeight() ), GetTextThickness() );
}


void PCB_TEXT::StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings )
{
    SetTextSize( settings.GetTextSize( GetLayer() ) );
    SetTextThickness( settings.GetTextThickness( GetLayer() ) );
    SetItalic( settings.GetTextItalic( GetLayer() ) );
    SetKeepUpright( settings.GetTextUpright( GetLayer() ) );
    SetMirrored( IsBackLayer( GetLayer() ) );
}


void PCB_TEXT::KeepUpright( const EDA_ANGLE& aNewOrientation )
{
    if( !IsKeepUpright() )
        return;

    EDA_ANGLE newAngle = GetTextAngle() + aNewOrientation;
    newAngle.Normalize();

    bool needsFlipped = newAngle >= ANGLE_180;

    if( needsFlipped )
    {
        SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -GetHorizJustify() ) );
        SetTextAngle( GetTextAngle() + ANGLE_180 );
    }
}


const BOX2I PCB_TEXT::GetBoundingBox() const
{
    EDA_ANGLE angle = GetDrawRotation();
    BOX2I     rect = GetTextBox();

    if( IsKnockout() )
        rect.Inflate( getKnockoutMargin() );

    if( !angle.IsZero() )
        rect = rect.GetBoundingBoxRotated( GetTextPos(), GetTextAngle() );

    return rect;
}


bool PCB_TEXT::TextHitTest( const VECTOR2I& aPoint, int aAccuracy ) const
{
    int accuracy = aAccuracy;

    if( IsKnockout() )
        accuracy += GetKnockoutTextMargin( GetTextSize(), GetTextThickness() );

    return EDA_TEXT::TextHitTest( aPoint, accuracy );
}


bool PCB_TEXT::TextHitTest( const BOX2I& aRect, bool aContains, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void PCB_TEXT::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aRotCentre, aAngle );
    SetTextPos( pt );

    EDA_ANGLE new_angle = GetTextAngle() + aAngle;
    new_angle.Normalize180();
    SetTextAngle( new_angle );
}


void PCB_TEXT::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    // the position and justification are mirrored, but not the text itself

    if( aMirrorAroundXAxis )
    {
        if( GetTextAngle() == ANGLE_VERTICAL )
            SetHorizJustify( (GR_TEXT_H_ALIGN_T) -GetHorizJustify() );

        SetTextY( MIRRORVAL( GetTextPos().y, aCentre.y ) );
    }
    else
    {
        if( GetTextAngle() == ANGLE_HORIZONTAL )
            SetHorizJustify( (GR_TEXT_H_ALIGN_T) -GetHorizJustify() );

        SetTextX( MIRRORVAL( GetTextPos().x, aCentre.x ) );
    }
}


void PCB_TEXT::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        SetTextX( MIRRORVAL( GetTextPos().x, aCentre.x ) );
        SetTextAngle( -GetTextAngle() );
    }
    else
    {
        SetTextY( MIRRORVAL( GetTextPos().y, aCentre.y ) );
        SetTextAngle( ANGLE_180 - GetTextAngle() );
    }

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );

    if( ( GetLayerSet() & LSET::SideSpecificMask() ).any() )
        SetMirrored( !IsMirrored() );
}


wxString PCB_TEXT::GetTextTypeDescription() const
{
    return _( "Text" );
}


wxString PCB_TEXT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        return wxString::Format( _( "Footprint Text '%s' of %s" ),
                                 KIUI::EllipsizeMenuText( GetText() ), parentFP->GetReference() );
    }

    return wxString::Format( _( "PCB Text '%s' on %s" ),
                             KIUI::EllipsizeMenuText( GetText() ),
                             GetLayerName() );
}


BITMAPS PCB_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


EDA_ITEM* PCB_TEXT::Clone() const
{
    return new PCB_TEXT( *this );
}


void PCB_TEXT::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TEXT_T );

    std::swap( *((PCB_TEXT*) this), *((PCB_TEXT*) aImage) );
}


std::shared_ptr<SHAPE> PCB_TEXT::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    if( IsKnockout() )
    {
        SHAPE_POLY_SET poly;

        TransformTextToPolySet( poly, 0, GetBoard()->GetDesignSettings().m_MaxError, ERROR_INSIDE );

        return std::make_shared<SHAPE_POLY_SET>( poly );
    }

    return GetEffectiveTextShape();
}


void PCB_TEXT::buildBoundingHull( SHAPE_POLY_SET* aBuffer, const SHAPE_POLY_SET& aRenderedText,
                                  int aClearance ) const
{
    SHAPE_POLY_SET poly( aRenderedText );

    poly.Rotate( -GetDrawRotation(), GetDrawPos() );

    BOX2I    rect = poly.BBox( aClearance );
    VECTOR2I corners[4];

    corners[0].x = rect.GetOrigin().x;
    corners[0].y = rect.GetOrigin().y;
    corners[1].y = corners[0].y;
    corners[1].x = rect.GetRight();
    corners[2].x = corners[1].x;
    corners[2].y = rect.GetBottom();
    corners[3].y = corners[2].y;
    corners[3].x = corners[0].x;

    aBuffer->NewOutline();

    for( VECTOR2I& corner : corners )
    {
        RotatePoint( corner, GetDrawPos(), GetDrawRotation() );
        aBuffer->Append( corner.x, corner.y );
    }
}


void PCB_TEXT::TransformTextToPolySet( SHAPE_POLY_SET& aBuffer, int aClearance, int aMaxError,
                                       ERROR_LOC aErrorLoc ) const
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = getDrawFont();
    int                        penWidth = GetEffectiveTextPenWidth();
    TEXT_ATTRIBUTES            attrs = GetAttributes();

    attrs.m_Angle = GetDrawRotation();

    // The polygonal shape of a text can have many basic shapes, so combining these shapes can
    // be very useful to create a final shape with a lot less vertices to speedup calculations.
    // Simplify shapes is not usually always efficient, but in this case it is.
    SHAPE_POLY_SET textShape;

    CALLBACK_GAL callback_gal( empty_opts,
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

    font->Draw( &callback_gal, GetShownText( true ), GetTextPos(), attrs, GetFontMetrics() );
    textShape.Simplify( SHAPE_POLY_SET::PM_FAST );

    if( IsKnockout() )
    {
        SHAPE_POLY_SET finalPoly;
        int            margin = GetKnockoutTextMargin( attrs.m_Size, penWidth );

        buildBoundingHull( &finalPoly, textShape, margin + aClearance );
        finalPoly.BooleanSubtract( textShape, SHAPE_POLY_SET::PM_FAST );

        aBuffer.Append( finalPoly );
    }
    else
    {
        if( aClearance > 0 || aErrorLoc == ERROR_OUTSIDE )
        {
            if( aErrorLoc == ERROR_OUTSIDE )
                aClearance += aMaxError;

            textShape.Inflate( aClearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aMaxError );
        }

        aBuffer.Append( textShape );
    }
}


void PCB_TEXT::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                        int aClearance, int aMaxError, ERROR_LOC aErrorLoc,
                                        bool aIgnoreLineWidth ) const
{
    SHAPE_POLY_SET poly;

    TransformTextToPolySet( poly, 0, aMaxError, aErrorLoc );

    buildBoundingHull( &aBuffer, poly, aClearance );
}


bool PCB_TEXT::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const PCB_TEXT& other = static_cast<const PCB_TEXT&>( aOther );

    return EDA_TEXT::operator==( other );
}


double PCB_TEXT::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_TEXT& other = static_cast<const PCB_TEXT&>( aOther );

    return EDA_TEXT::Similarity( other );
}


static struct PCB_TEXT_DESC
{
    PCB_TEXT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TEXT );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXT, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXT, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXT ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Color" ) );

        propMgr.AddProperty( new PROPERTY<PCB_TEXT, bool, BOARD_ITEM>( _HKI( "Knockout" ),
                &BOARD_ITEM::SetIsKnockout, &BOARD_ITEM::IsKnockout ),
                _HKI( "Text Properties" ) );

        propMgr.AddProperty( new PROPERTY<PCB_TEXT, bool, EDA_TEXT>( _HKI( "Keep Upright" ),
                &PCB_TEXT::SetKeepUpright, &PCB_TEXT::IsKeepUpright ),
                _HKI( "Text Properties" ) );

        auto isFootprintText =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_TEXT* text = dynamic_cast<PCB_TEXT*>( aItem ) )
                        return text->GetParentFootprint();

                    return false;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Visible" ), isFootprintText );

        propMgr.OverrideAvailability( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Keep Upright" ), isFootprintText );

        propMgr.OverrideAvailability( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Hyperlink" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
    }
} _PCB_TEXT_DESC;
