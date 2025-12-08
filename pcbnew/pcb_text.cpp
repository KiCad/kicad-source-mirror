/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <google/protobuf/any.pb.h>

#include <advanced_config.h>
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
#include <dialogs/html_message_box.h>
#include <geometry/shape_compound.h>
#include <geometry/geometry_utils.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_types.pb.h>


PCB_TEXT::PCB_TEXT( BOARD_ITEM* parent, KICAD_T idtype ) :
        BOARD_ITEM( parent, idtype ),
        EDA_TEXT( pcbIUScale )
{
    SetMultilineAllowed( true );
}


PCB_TEXT::PCB_TEXT( FOOTPRINT* aParent, KICAD_T idtype ) :
        BOARD_ITEM( aParent, idtype ),
        EDA_TEXT( pcbIUScale )
{
    SetKeepUpright( true );

    // N.B. Do not automatically set text effects
    // These are optional in the file format and so need to be defaulted to off.

    SetLayer( F_SilkS );

    if( aParent )
    {
        SetTextPos( aParent->GetPosition() );

        if( IsBackLayer( aParent->GetLayer() ) )
            SetLayer( B_SilkS );
    }
}


PCB_TEXT::~PCB_TEXT()
{
}


void PCB_TEXT::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_TEXT_T, /* void */ );
    *this = *static_cast<const PCB_TEXT*>( aOther );
}


void PCB_TEXT::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::common;
    kiapi::board::types::BoardText boardText;

    boardText.mutable_id()->set_value( m_Uuid.AsStdString() );
    boardText.set_layer( ToProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( GetLayer() ) );
    boardText.set_knockout( IsKnockout() );
    boardText.set_locked( IsLocked() ? types::LockedState::LS_LOCKED : types::LockedState::LS_UNLOCKED );

    google::protobuf::Any any;
    EDA_TEXT::Serialize( any );
    any.UnpackTo( boardText.mutable_text() );

    // Some of the common Text message fields are not stored in EDA_TEXT
    types::Text* text = boardText.mutable_text();

    PackVector2( *text->mutable_position(), GetPosition() );

    aContainer.PackFrom( boardText );
}


bool PCB_TEXT::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::common;
    kiapi::board::types::BoardText boardText;

    if( !aContainer.UnpackTo( &boardText ) )
        return false;

    SetLayer( FromProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( boardText.layer() ) );
    const_cast<KIID&>( m_Uuid ) = KIID( boardText.id().value() );
    SetIsKnockout( boardText.knockout() );
    SetLocked( boardText.locked() == types::LockedState::LS_LOCKED );

    google::protobuf::Any any;
    any.PackFrom( boardText.text() );
    EDA_TEXT::Deserialize( any );

    const types::Text& text = boardText.text();

    SetPosition( UnpackVector2( text.position() ) );

    return true;
}


wxString PCB_TEXT::GetShownText( bool aAllowExtraText, int aDepth ) const
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

        // board can be null in some cases when saving a footprint in FP editor
        if( board && board->ResolveTextVar( token, aDepth + 1 ) )
            return true;

        return false;
    };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( HasTextVars() )
        text = ResolveTextVars( text, &resolver, aDepth );

    // Convert escape markers back to literal ${} and @{} for final display
    text.Replace( wxT( "<<<ESC_DOLLAR:" ), wxT( "${" ) );
    text.Replace( wxT( "<<<ESC_AT:" ), wxT( "@{" ) );

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


std::vector<int> PCB_TEXT::ViewGetLayers() const
{
    if( IsLocked() || ( GetParentFootprint() && GetParentFootprint()->IsLocked() ) )
        return { GetLayer(), LAYER_LOCKED_ITEM_SHADOW };

    return { GetLayer() };
}


double PCB_TEXT::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    if( !aView )
        return LOD_SHOW;

    KIGFX::PCB_PAINTER&         painter = static_cast<KIGFX::PCB_PAINTER&>( *aView->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS& renderSettings = *painter.GetSettings();

    if( !aView->IsLayerVisible( GetLayer() ) )
        return LOD_HIDE;

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow on dimmed tracks
        if( renderSettings.GetHighContrast() )
        {
            if( m_layer != renderSettings.GetPrimaryHighContrastLayer() )
                return LOD_HIDE;
        }
    }

    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        // Handle Render tab switches
        if( GetText() == wxT( "${VALUE}" ) )
        {
            if( !aView->IsLayerVisible( LAYER_FP_VALUES ) )
                return LOD_HIDE;
        }

        if( GetText() == wxT( "${REFERENCE}" ) )
        {
            if( !aView->IsLayerVisible( LAYER_FP_REFERENCES ) )
                return LOD_HIDE;
        }

        if( parentFP->GetLayer() == F_Cu && !aView->IsLayerVisible( LAYER_FOOTPRINTS_FR ) )
            return LOD_HIDE;

        if( parentFP->GetLayer() == B_Cu && !aView->IsLayerVisible( LAYER_FOOTPRINTS_BK ) )
            return LOD_HIDE;

        if( !aView->IsLayerVisible( LAYER_FP_TEXT ) )
            return LOD_HIDE;
    }

    return LOD_SHOW;
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

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Angle" ), wxString::Format( wxT( "%g" ), GetTextAngle().AsDegrees() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    if( GetTextThickness() )
        aList.emplace_back( _( "Text Thickness" ), aFrame->MessageTextFromValue( GetEffectiveTextPenWidth() ) );
    else
        aList.emplace_back( _( "Text Thickness" ), _( "Auto" ) );

    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );
}


int PCB_TEXT::getKnockoutMargin() const
{
    return GetKnockoutTextMargin( VECTOR2I( GetTextWidth(), GetTextHeight() ), GetEffectiveTextPenWidth() );
}


void PCB_TEXT::StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings, bool aCheckSide )
{
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


void PCB_TEXT::KeepUpright()
{
    if( !IsKeepUpright() )
        return;

    EDA_ANGLE newAngle = GetTextAngle();
    newAngle.Normalize();

    bool needsFlipped = newAngle >= ANGLE_180;

    if( needsFlipped )
    {
        SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -GetHorizJustify() ) );
        SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( -GetVertJustify() ) );
        newAngle += ANGLE_180;
        newAngle.Normalize();
        SetTextAngle( newAngle );
    }
}


const BOX2I PCB_TEXT::GetBoundingBox() const
{
    EDA_ANGLE angle = GetDrawRotation();
    BOX2I     rect = GetTextBox( nullptr );

    if( IsKnockout() )
        rect.Inflate( getKnockoutMargin() );

    if( !angle.IsZero() )
        rect = rect.GetBoundingBoxRotated( GetTextPos(), angle );

    return rect;
}


bool PCB_TEXT::TextHitTest( const VECTOR2I& aPoint, int aAccuracy ) const
{
    int accuracy = aAccuracy;

    if( IsKnockout() )
        accuracy += GetKnockoutTextMargin( GetTextSize(), GetEffectiveTextPenWidth() );

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


bool PCB_TEXT::TextHitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    BOX2I rect = GetTextBox( nullptr );

    if( IsKnockout() )
        rect.Inflate( getKnockoutMargin() );

    return KIGEOM::BoxHitTest( aPoly, rect, GetDrawRotation(), GetDrawPos(), aContained );
}


void PCB_TEXT::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aRotCentre, aAngle );
    SetTextPos( pt );

    EDA_ANGLE new_angle = GetTextAngle() + aAngle;
    new_angle.Normalize();
    SetTextAngle( new_angle );
}


void PCB_TEXT::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    // the position and justification are mirrored, but not the text itself

    if( aFlipDirection == FLIP_DIRECTION::TOP_BOTTOM )
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


void PCB_TEXT::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
    {
        SetTextX( MIRRORVAL( GetTextPos().x, aCentre.x ) );
        SetTextAngle( -GetTextAngle() );
    }
    else
    {
        SetTextY( MIRRORVAL( GetTextPos().y, aCentre.y ) );
        SetTextAngle( ANGLE_180 - GetTextAngle() );
    }

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );

    if( IsSideSpecific() )
        SetMirrored( !IsMirrored() );
}


wxString PCB_TEXT::GetTextTypeDescription() const
{
    return _( "Text" );
}


wxString PCB_TEXT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    wxString content = aFull ? GetShownText( false ) : KIUI::EllipsizeMenuText( GetText() );

    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        wxString ref = parentFP->GetReference();
        return wxString::Format( _( "Footprint text of %s (%s)" ), ref, content );
    }

    return wxString::Format( _( "PCB text '%s' on %s" ), content, GetLayerName() );
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

    std::swap( *( (PCB_TEXT*) this ), *( (PCB_TEXT*) aImage ) );
}


std::shared_ptr<SHAPE> PCB_TEXT::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    if( IsKnockout() )
    {
        SHAPE_POLY_SET poly;

        TransformTextToPolySet( poly, 0, GetMaxError(), ERROR_INSIDE );

        return std::make_shared<SHAPE_POLY_SET>( std::move( poly ) );
    }

    return GetEffectiveTextShape();
}


SHAPE_POLY_SET PCB_TEXT::GetKnockoutCache( const KIFONT::FONT* aFont, const wxString& forResolvedText,
                                           int aMaxError ) const
{
    TEXT_ATTRIBUTES attrs = GetAttributes();
    EDA_ANGLE       drawAngle = GetDrawRotation();
    VECTOR2I        drawPos = GetDrawPos();

    if( m_knockout_cache.IsEmpty() || m_knockout_cache_text_attrs != attrs || m_knockout_cache_text != forResolvedText
        || m_knockout_cache_angle != drawAngle )
    {
        m_knockout_cache.RemoveAllContours();

        TransformTextToPolySet( m_knockout_cache, 0, aMaxError, ERROR_INSIDE );
        m_knockout_cache.Fracture();

        m_knockout_cache_text_attrs = attrs;
        m_knockout_cache_angle = drawAngle;
        m_knockout_cache_text = forResolvedText;
        m_knockout_cache_pos = drawPos;
    }
    else if( m_knockout_cache_pos != drawPos )
    {
        m_knockout_cache.Move( drawPos - m_knockout_cache_pos );
        m_knockout_cache_pos = drawPos;
    }

    return m_knockout_cache;
}


void PCB_TEXT::buildBoundingHull( SHAPE_POLY_SET* aBuffer, const SHAPE_POLY_SET& aRenderedText, int aClearance ) const
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
    KIFONT::FONT*              font = GetDrawFont( nullptr );
    int                        penWidth = GetEffectiveTextPenWidth();
    TEXT_ATTRIBUTES            attrs = GetAttributes();
    wxString                   shownText = GetShownText( true );

    attrs.m_Angle = GetDrawRotation();

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
        font->Draw( &callback_gal, shownText, GetTextPos(), attrs, GetFontMetrics() );

    textShape.Simplify();

    if( IsKnockout() )
    {
        SHAPE_POLY_SET finalPoly;
        int            margin = GetKnockoutTextMargin( attrs.m_Size, penWidth );

        buildBoundingHull( &finalPoly, textShape, margin + aClearance );
        finalPoly.BooleanSubtract( textShape );

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


void PCB_TEXT::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance, int aMaxError,
                                        ERROR_LOC aErrorLoc, bool aIgnoreLineWidth ) const
{
    SHAPE_POLY_SET poly;

    TransformTextToPolySet( poly, 0, aMaxError, aErrorLoc );

    buildBoundingHull( &aBuffer, poly, aClearance );
}


bool PCB_TEXT::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_TEXT& other = static_cast<const PCB_TEXT&>( aBoardItem );

    return *this == other;
}


bool PCB_TEXT::operator==( const PCB_TEXT& aOther ) const
{
    return EDA_TEXT::operator==( aOther );
}


double PCB_TEXT::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_TEXT& other = static_cast<const PCB_TEXT&>( aOther );

    return EDA_TEXT::Similarity( other );
}


HTML_MESSAGE_BOX* PCB_TEXT::ShowSyntaxHelp( wxWindow* aParentWindow )
{
    wxString msg =
#include "pcb_text_help_md.h"
            ;

    HTML_MESSAGE_BOX* dlg = new HTML_MESSAGE_BOX( aParentWindow, _( "Syntax Help" ) );
    wxSize            sz( 320, 320 );

    dlg->SetMinSize( dlg->ConvertDialogToPixels( sz ) );
    dlg->SetDialogSizeInDU( sz.x, sz.y );

    wxString html_txt;
    ConvertMarkdown2Html( wxGetTranslation( msg ), html_txt );
    dlg->AddHTML_Text( html_txt );
    dlg->ShowModeless();

    return dlg;
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

        propMgr.AddProperty( new PROPERTY<PCB_TEXT, bool, BOARD_ITEM>( _HKI( "Knockout" ), &BOARD_ITEM::SetIsKnockout,
                                                                       &BOARD_ITEM::IsKnockout ),
                             _HKI( "Text Properties" ) );

        propMgr.AddProperty( new PROPERTY<PCB_TEXT, bool, EDA_TEXT>( _HKI( "Keep Upright" ), &PCB_TEXT::SetKeepUpright,
                                                                     &PCB_TEXT::IsKeepUpright ),
                             _HKI( "Text Properties" ) );

        auto isFootprintText = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_TEXT* text = dynamic_cast<PCB_TEXT*>( aItem ) )
                return text->GetParentFootprint();

            return false;
        };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Keep Upright" ),
                                      isFootprintText );

        propMgr.Mask( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );
    }
} _PCB_TEXT_DESC;
