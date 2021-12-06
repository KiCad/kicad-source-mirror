/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
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

using KIGFX::PCB_RENDER_SETTINGS;


PCB_TEXT::PCB_TEXT( BOARD_ITEM* parent ) :
    BOARD_ITEM( parent, PCB_TEXT_T ),
    EDA_TEXT()
{
    SetMultilineAllowed( true );
}


PCB_TEXT::~PCB_TEXT()
{
}


wxString PCB_TEXT::GetShownText( int aDepth ) const
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

    return text;
}


void PCB_TEXT::SetTextAngle( double aAngle )
{
    EDA_TEXT::SetTextAngle( NormalizeAngle360Min( aAngle ) );
}


void PCB_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "PCB Text" ), UnescapeString( GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Angle" ), wxString::Format( "%g", GetTextAngleDegrees() ) );

    aList.emplace_back( _( "Thickness" ), MessageTextFromValue( units, GetTextThickness() ) );
    aList.emplace_back( _( "Width" ), MessageTextFromValue( units, GetTextWidth() ) );
    aList.emplace_back( _( "Height" ), MessageTextFromValue( units, GetTextHeight() ) );
}


const EDA_RECT PCB_TEXT::GetBoundingBox() const
{
    EDA_RECT rect = GetTextBox();

    if( GetTextAngle() != 0 )
        rect = rect.GetBoundingBoxRotated( GetTextPos(), GetTextAngle() );

    return rect;
}


bool PCB_TEXT::TextHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    return EDA_TEXT::TextHitTest( aPoint, aAccuracy );
}


bool PCB_TEXT::TextHitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void PCB_TEXT::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aRotCentre, aAngle );
    SetTextPos( pt );

    SetTextAngle( GetTextAngle() + aAngle );
}


void PCB_TEXT::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        SetTextX( MIRRORVAL( GetTextPos().x, aCentre.x ) );
        SetTextAngle( -GetTextAngle() );
    }
    else
    {
        SetTextY( MIRRORVAL( GetTextPos().y, aCentre.y ) );
        SetTextAngle( 1800 - GetTextAngle() );
    }

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
    SetMirrored( !IsMirrored() );
}


wxString PCB_TEXT::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "PCB Text '%s' on %s"), ShortenedShownText(), GetLayerName() );
}


BITMAPS PCB_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


EDA_ITEM* PCB_TEXT::Clone() const
{
    return new PCB_TEXT( *this );
}


void PCB_TEXT::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TEXT_T );

    std::swap( *((PCB_TEXT*) this), *((PCB_TEXT*) aImage) );
}


std::shared_ptr<SHAPE> PCB_TEXT::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    return GetEffectiveTextShape();
}


void PCB_TEXT::TransformTextShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                         PCB_LAYER_ID aLayer, int aClearanceValue,
                                                         int aError, ERROR_LOC aErrorLoc ) const
{
    struct TSEGM_2_POLY_PRMS prms;

    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    int  penWidth = GetEffectiveTextPenWidth();

    prms.m_cornerBuffer = &aCornerBuffer;
    prms.m_textWidth = GetEffectiveTextPenWidth() + ( 2 * aClearanceValue );
    prms.m_error = aError;
    COLOR4D color;  // not actually used, but needed by GRText

    GRText( nullptr, GetTextPos(), color, GetShownText(), GetTextAngle(), size, GetHorizJustify(),
            GetVertJustify(), penWidth, IsItalic(), IsBold(), addTextSegmToPoly, &prms );
}


void PCB_TEXT::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                     PCB_LAYER_ID aLayer, int aClearance,
                                                     int aError, ERROR_LOC aErrorLoc,
                                                     bool aIgnoreLineWidth ) const
{
    EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon( &aCornerBuffer, aClearance );
}


static struct TEXTE_PCB_DESC
{
    TEXTE_PCB_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TEXT );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXT, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXT, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXT ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ) );
    }
} _TEXTE_PCB_DESC;
