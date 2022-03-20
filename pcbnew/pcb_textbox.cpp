/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint.h>
#include <pcb_textbox.h>
#include <pcb_painter.h>
#include <trigo.h>
#include <string_utils.h>
#include <geometry/shape_compound.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>
#include "macros.h"


PCB_TEXTBOX::PCB_TEXTBOX( BOARD_ITEM* parent ) :
    PCB_SHAPE( parent, PCB_TEXTBOX_T, SHAPE_T::RECT ),
    EDA_TEXT()
{
    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    SetMultilineAllowed( true );
}


PCB_TEXTBOX::~PCB_TEXTBOX()
{
}


int PCB_TEXTBOX::GetTextMargin() const
{
    return KiROUND( GetTextSize().y * 0.8 );
}


std::vector<VECTOR2I> PCB_TEXTBOX::GetAnchorAndOppositeCorner() const
{
    EDA_ANGLE epsilon( 1.0, DEGREES_T );    // Yeah, it's a pretty coarse epsilon, but anything
                                            // under 45° will work for our purposes.

    std::vector<VECTOR2I> pts;
    std::vector<VECTOR2I> corners = GetCorners();

    EDA_ANGLE textAngle = GetDrawRotation().Normalize();
    EDA_ANGLE toCorner1 = EDA_ANGLE( corners[1] - corners[0] ).Normalize();
    EDA_ANGLE fromCorner1 = ( toCorner1 + ANGLE_180 ).Normalize();

    pts.emplace_back( corners[0] );

    if( std::abs( toCorner1 - textAngle ) < epsilon || std::abs( fromCorner1 - textAngle ) < epsilon )
        pts.emplace_back( corners[1] );
    else
        pts.emplace_back( corners[3] );

    return pts;
}


VECTOR2I PCB_TEXTBOX::GetDrawPos() const
{
    std::vector<VECTOR2I> corners = GetAnchorAndOppositeCorner();
    GR_TEXT_H_ALIGN_T     effectiveAlignment = GetHorizJustify();
    VECTOR2I              textAnchor;
    VECTOR2I              offset;

    if( IsMirrored() )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   effectiveAlignment = GR_TEXT_H_ALIGN_RIGHT;  break;
        case GR_TEXT_H_ALIGN_CENTER: effectiveAlignment = GR_TEXT_H_ALIGN_CENTER; break;
        case GR_TEXT_H_ALIGN_RIGHT:  effectiveAlignment = GR_TEXT_H_ALIGN_LEFT;   break;
        }
    }

    switch( effectiveAlignment )
    {
    case GR_TEXT_H_ALIGN_LEFT:
        textAnchor = corners[0];
        offset = VECTOR2I( GetTextMargin(), GetTextMargin() );
        break;
    case GR_TEXT_H_ALIGN_CENTER:
        textAnchor = ( corners[0] + corners[1] ) / 2;
        offset = VECTOR2I( 0, GetTextMargin() );
        break;
    case GR_TEXT_H_ALIGN_RIGHT:
        textAnchor = corners[1];
        offset = VECTOR2I( -GetTextMargin(), GetTextMargin() );
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


wxString PCB_TEXTBOX::GetShownText( int aDepth ) const
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

                if( token->Contains( ':' ) )
                {
                    wxString      remainder;
                    wxString      ref = token->BeforeFirst( ':', &remainder );
                    BOARD_ITEM*   refItem = board->GetItem( KIID( ref ) );

                    if( refItem && refItem->Type() == PCB_FOOTPRINT_T )
                    {
                        FOOTPRINT* refFP = static_cast<FOOTPRINT*>( refItem );

                        if( refFP->ResolveTextVar( &remainder, aDepth + 1 ) )
                        {
                            *token = remainder;
                            return true;
                        }
                    }
                }
                return false;
            };

    std::function<bool( wxString* )> boardTextResolver =
            [&]( wxString* token ) -> bool
            {
                return board->ResolveTextVar( token, aDepth + 1 );
            };

    wxString text = EDA_TEXT::GetShownText();

    if( board && HasTextVars() && aDepth < 10 )
        text = ExpandTextVars( text, &pcbTextResolver, &boardTextResolver, board->GetProject() );

    KIFONT::FONT*         font = GetDrawFont();
    std::vector<VECTOR2I> corners = GetAnchorAndOppositeCorner();
    int                   colWidth = ( corners[1] - corners[0] ).EuclideanNorm();

    colWidth -= GetTextMargin() * 2;
    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    return text;
}


void PCB_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), UnescapeString( GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );
    aList.emplace_back( _( "Angle" ), wxString::Format( "%g", GetTextAngle().AsDegrees() ) );

    aList.emplace_back( _( "Text Thickness" ), MessageTextFromValue( units, GetTextThickness() ) );
    aList.emplace_back( _( "Text Width" ), MessageTextFromValue( units, GetTextWidth() ) );
    aList.emplace_back( _( "Text Height" ), MessageTextFromValue( units, GetTextHeight() ) );

    wxString msg = MessageTextFromValue( units, std::abs( GetEnd().x - GetStart().x ) );
    aList.emplace_back( _( "Box Width" ), msg );

    msg = MessageTextFromValue( units, std::abs( GetEnd().y - GetStart().y ) );
    aList.emplace_back( _( "Box Height" ), msg );

    m_stroke.GetMsgPanelInfo( units, aList );
}


void PCB_TEXTBOX::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    PCB_SHAPE::Rotate( aRotCentre, aAngle );
    SetTextAngle( GetTextAngle() + aAngle );
}


void PCB_TEXTBOX::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_start.x = aCentre.x - ( m_start.x - aCentre.x );
        m_end.x   = aCentre.x - ( m_end.x - aCentre.x );
        SetTextAngle( -GetTextAngle() );
    }
    else
    {
        m_start.y = aCentre.y - ( m_start.y - aCentre.y );
        m_end.y   = aCentre.y - ( m_end.y - aCentre.y );
        SetTextAngle( ANGLE_180 - GetTextAngle() );
    }

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
    SetMirrored( !IsMirrored() );
}


bool PCB_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool PCB_TEXTBOX::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


wxString PCB_TEXTBOX::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "PCB Text Box on %s"), GetLayerName() );
}


BITMAPS PCB_TEXTBOX::GetMenuImage() const
{
    return BITMAPS::add_textbox;
}


EDA_ITEM* PCB_TEXTBOX::Clone() const
{
    return new PCB_TEXTBOX( *this );
}


void PCB_TEXTBOX::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TEXTBOX_T );

    std::swap( *((PCB_TEXTBOX*) this), *((PCB_TEXTBOX*) aImage) );
}


std::shared_ptr<SHAPE> PCB_TEXTBOX::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    if( PCB_SHAPE::GetStroke().GetWidth() >= 0 )
        return PCB_SHAPE::GetEffectiveShape( aLayer, aFlash );
    else
        return GetEffectiveTextShape();
}


void PCB_TEXTBOX::TransformTextShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                         PCB_LAYER_ID aLayer, int aClearance,
                                                         int aError, ERROR_LOC aErrorLoc ) const
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = GetDrawFont();
    int                        penWidth = GetEffectiveTextPenWidth();

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                TransformOvalToPolygon( aCornerBuffer, aPt1, aPt2, penWidth+ ( 2 * aClearance ),
                                        aError, ERROR_INSIDE );
            },
            // Triangulation callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
            {
                aCornerBuffer.NewOutline();

                for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                    aCornerBuffer.Append( point.x, point.y );
            } );

    font->Draw( &callback_gal, GetShownText(), GetTextPos(), GetAttributes() );
}


void PCB_TEXTBOX::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                     PCB_LAYER_ID aLayer, int aClearance,
                                                     int aError, ERROR_LOC aErrorLoc,
                                                     bool aIgnoreLineWidth ) const
{
    if( PCB_SHAPE::GetStroke().GetWidth() >= 0 )
    {
        PCB_SHAPE::TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, aClearance,
                                                         aError, aErrorLoc );
    }
    else
    {
        EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon( &aCornerBuffer, aClearance );
    }
}


static struct PCB_TEXTBOX_DESC
{
    PCB_TEXTBOX_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TEXTBOX );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXTBOX, PCB_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXTBOX, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( PCB_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_TEXT ) );
    }
} _PCB_TEXTBOX_DESC;
