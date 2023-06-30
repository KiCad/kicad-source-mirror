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


PCB_TEXT::PCB_TEXT( BOARD_ITEM* parent, KICAD_T idtype ) :
    BOARD_ITEM( parent, idtype ),
    EDA_TEXT( pcbIUScale )
{
    SetMultilineAllowed( true );
}


PCB_TEXT::~PCB_TEXT()
{
}


wxString PCB_TEXT::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    BOARD* board = dynamic_cast<BOARD*>( GetParent() );

    std::function<bool( wxString* )> pcbTextResolver =
            [&]( wxString* token ) -> bool
            {
                if( token->IsSameAs( wxT( "LAYER" ) ) )
                {
                    *token = GetLayerName();
                    return true;
                }

                if( board->ResolveTextVar( token, aDepth + 1 ) )
                {
                    return true;
                }

                return false;
            };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( board && HasTextVars() && aDepth < 10 )
        text = ExpandTextVars( text, &pcbTextResolver );

    return text;
}


bool PCB_TEXT::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return BOARD_ITEM::Matches( UnescapeString( GetText() ), aSearchData );
}


double PCB_TEXT::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
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


void PCB_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "PCB Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

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
    VECTOR2I textSize( GetTextWidth(), GetTextHeight() );
    int      thickness = GetTextThickness();

    return thickness * 1.5 + GetKnockoutTextMargin( textSize, thickness );
}


const BOX2I PCB_TEXT::GetBoundingBox() const
{
    BOX2I rect = GetTextBox();

    if( IsKnockout() )
        rect.Inflate( getKnockoutMargin() );

    if( !GetTextAngle().IsZero() )
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


wxString PCB_TEXT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "PCB Text '%s' on %s"),
                             KIUI::EllipsizeMenuText( GetShownText( false ) ),
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


void PCB_TEXT::TransformTextToPolySet( SHAPE_POLY_SET& aBuffer, int aClearance, int aError,
                                       ERROR_LOC aErrorLoc ) const
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = getDrawFont();
    int                        penWidth = GetEffectiveTextPenWidth();

    // The polygonal shape of a text can have many basic shapes, so combining these shapes can
    // be very useful to create a final shape with a lot less vertices to speedup calculations.
    // Simplify shapes is not usually always efficient, but in this case it is.
    SHAPE_POLY_SET textShape;

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                TransformOvalToPolygon( textShape, aPt1, aPt2, penWidth, aError, aErrorLoc );
            },
            // Triangulation callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
            {
                textShape.NewOutline();

                for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                    textShape.Append( point.x, point.y );
            } );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_Angle = GetDrawRotation();

    font->Draw( &callback_gal, GetShownText( true ), GetTextPos(), attrs );
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
        if( aClearance > 0 )
            textShape.Inflate( aClearance, aClearance );

        aBuffer.Append( textShape );
    }
}


void PCB_TEXT::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                        int aClearance, int aError, ERROR_LOC aErrorLoc,
                                        bool aIgnoreLineWidth ) const
{
    SHAPE_POLY_SET poly;

    TransformTextToPolySet( poly, 0, GetBoard()->GetDesignSettings().m_MaxError, aErrorLoc );

    buildBoundingHull( &aBuffer, poly, aClearance );
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

        propMgr.AddProperty( new PROPERTY<PCB_TEXT, bool, BOARD_ITEM>( _HKI( "Knockout" ),
                &BOARD_ITEM::SetIsKnockout, &BOARD_ITEM::IsKnockout ),
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
                                      _HKI( "Hyperlink" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
    }
} _PCB_TEXT_DESC;
