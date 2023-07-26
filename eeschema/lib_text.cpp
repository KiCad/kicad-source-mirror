/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <common.h>
#include <sch_draw_panel.h>
#include <plotters/plotter.h>
#include <trigo.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <lib_item.h>
#include <general.h>
#include <transform.h>
#include <settings/color_settings.h>
#include <lib_text.h>
#include <default_values.h>    // For some default values
#include <string_utils.h>

LIB_TEXT::LIB_TEXT( LIB_SYMBOL* aParent ) :
    LIB_ITEM( LIB_TEXT_T, aParent ),
    EDA_TEXT( schIUScale, wxEmptyString )
{
    SetTextSize( VECTOR2I( schIUScale.MilsToIU( DEFAULT_TEXT_SIZE ),
                           schIUScale.MilsToIU( DEFAULT_TEXT_SIZE ) ) );
}


void LIB_TEXT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 2;
    aLayers[0] = IsPrivate() ? LAYER_PRIVATE_NOTES : LAYER_DEVICE;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


bool LIB_TEXT::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    EDA_TEXT tmp_text( *this );
    tmp_text.SetTextPos( DefaultTransform.TransformCoordinate( GetTextPos() ) );

    /* The text orientation may need to be flipped if the
     * transformation matrix causes xy axes to be flipped.
     * this simple algo works only for schematic matrix (rot 90 or/and mirror)
     */
    bool t1 = ( DefaultTransform.x1 != 0 ) ^ ( GetTextAngle() != ANGLE_HORIZONTAL );

    tmp_text.SetTextAngle( t1 ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );
    return tmp_text.TextHitTest( aPosition, aAccuracy );
}


EDA_ITEM* LIB_TEXT::Clone() const
{
    LIB_TEXT* newitem = new LIB_TEXT( nullptr );

    newitem->m_parent    = m_parent;
    newitem->m_unit      = m_unit;
    newitem->m_convert   = m_convert;
    newitem->m_private   = m_private;
    newitem->m_flags     = m_flags;

    newitem->SetText( GetText() );
    newitem->SetAttributes( *this );

    return newitem;
}


int LIB_TEXT::compare( const LIB_ITEM& aOther, int aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_TEXT_T );

    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const LIB_TEXT* tmp = ( LIB_TEXT* ) &aOther;

    int result = GetText().CmpNoCase( tmp->GetText() );

    if( result != 0 )
        return result;

    if( GetTextPos().x != tmp->GetTextPos().x )
        return GetTextPos().x - tmp->GetTextPos().x;

    if( GetTextPos().y != tmp->GetTextPos().y )
        return GetTextPos().y - tmp->GetTextPos().y;

    if( GetTextWidth() != tmp->GetTextWidth() )
        return GetTextWidth() - tmp->GetTextWidth();

    if( GetTextHeight() != tmp->GetTextHeight() )
        return GetTextHeight() - tmp->GetTextHeight();

    return 0;
}


void LIB_TEXT::Offset( const VECTOR2I& aOffset )
{
    EDA_TEXT::Offset( aOffset );
}


void LIB_TEXT::MoveTo( const VECTOR2I& newPosition )
{
    SetTextPos( newPosition );
}


void LIB_TEXT::NormalizeJustification( bool inverse )
{
    if( GetHorizJustify() == GR_TEXT_H_ALIGN_CENTER && GetVertJustify() == GR_TEXT_V_ALIGN_CENTER )
        return;

    VECTOR2I delta( 0, 0 );
    BOX2I    bbox = GetTextBox();

    if( GetTextAngle().IsHorizontal() )
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            delta.x = bbox.GetWidth() / 2;
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            delta.x = - bbox.GetWidth() / 2;

        if( GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
            delta.y = - bbox.GetHeight() / 2;
        else if( GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
            delta.y = bbox.GetHeight() / 2;
    }
    else
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            delta.y = bbox.GetWidth() / 2;
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            delta.y = - bbox.GetWidth() / 2;

        if( GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
            delta.x = + bbox.GetHeight() / 2;
        else if( GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
            delta.x = - bbox.GetHeight() / 2;
    }

    if( inverse )
        SetTextPos( GetTextPos() - delta );
    else
        SetTextPos( GetTextPos() + delta );
}


void LIB_TEXT::MirrorHorizontal( const VECTOR2I& center )
{
    NormalizeJustification( false );
    int x = GetTextPos().x;

    x -= center.x;
    x *= -1;
    x += center.x;

    if( GetTextAngle().IsHorizontal() )
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }
    else
    {
        if( GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
            SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else if( GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
            SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    }

    SetTextX( x );
    NormalizeJustification( true );
}


void LIB_TEXT::MirrorVertical( const VECTOR2I& center )
{
    NormalizeJustification( false );
    int y = GetTextPos().y;

    y -= center.y;
    y *= -1;
    y += center.y;

    if( GetTextAngle().IsHorizontal() )
    {
        if( GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
            SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else if( GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
            SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    }
    else
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }

    SetTextY( y );
    NormalizeJustification( true );
}


void LIB_TEXT::Rotate( const VECTOR2I& center, bool aRotateCCW )
{
    NormalizeJustification( false );
    EDA_ANGLE rot_angle = aRotateCCW ? -ANGLE_90 : ANGLE_90;

    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, center, rot_angle );
    SetTextPos( pt );

    if( GetTextAngle().IsHorizontal() )
    {
        SetTextAngle( ANGLE_VERTICAL );
    }
    else
    {
        // 180° rotation is a mirror

        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

        if( GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
            SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else if( GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
            SetVertJustify( GR_TEXT_V_ALIGN_TOP );

        SetTextAngle( ANGLE_0 );
    }

    NormalizeJustification( true );
}


void LIB_TEXT::Plot( PLOTTER* plotter, bool aBackground, const VECTOR2I& offset,
                     const TRANSFORM& aTransform, bool aDimmed ) const
{
    wxASSERT( plotter != nullptr );

    if( IsPrivate() )
        return;

    if( aBackground )
        return;

    RENDER_SETTINGS* settings = plotter->RenderSettings();

    BOX2I bBox = GetBoundingBox();
    // convert coordinates from draw Y axis to symbol_editor Y axis
    bBox.RevertYAxis();

    /*
     * Calculate the text justification, according to the symbol orientation/mirror.  This is
     * a bit complicated due to cumulative calculations:
     *  - numerous cases (mirrored or not, rotation)
     *  - the plotter's Text() function will also recalculate H and V justifications according
     *    to the text orientation
     *  - when a symbol is mirrored the text is not, and justifications become a nightmare
     *
     *  So the easier way is to use no justifications (centered text) and use GetBoundingBox to
     *  know the text coordinate considered as centered.
     */
    VECTOR2I txtpos = bBox.Centre();
    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_Halign = GR_TEXT_H_ALIGN_CENTER;
    attrs.m_Valign = GR_TEXT_V_ALIGN_CENTER;

    // The text orientation may need to be flipped if the transformation matrix causes xy
    // axes to be flipped.
    int      t1  = ( aTransform.x1 != 0 ) ^ ( GetTextAngle() != ANGLE_HORIZONTAL );
    VECTOR2I pos = aTransform.TransformCoordinate( txtpos ) + offset;
    COLOR4D  color = GetTextColor();
    COLOR4D  bg = settings->GetBackgroundColor();

    if( !plotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
        color = settings->GetLayerColor( LAYER_DEVICE );

    if( !IsVisible() )
        bg = settings->GetLayerColor( LAYER_HIDDEN );
    else if( bg == COLOR4D::UNSPECIFIED || !plotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    int penWidth = std::max( GetEffectiveTextPenWidth(), settings->GetMinPenWidth() );

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( settings->GetDefaultFont(), IsBold(), IsItalic() );

    attrs.m_StrokeWidth = penWidth;
    attrs.m_Angle = t1 ? ANGLE_HORIZONTAL : ANGLE_VERTICAL;

    plotter->PlotText( pos, color, GetText(), attrs, font );
}


int LIB_TEXT::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


KIFONT::FONT* LIB_TEXT::getDrawFont() const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    return font;
}


void LIB_TEXT::print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, void* aData,
                      const TRANSFORM& aTransform, bool aDimmed )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = GetTextColor();
    bool    blackAndWhiteMode = GetGRForceBlackPenState();
    int     penWidth = std::max( GetEffectiveTextPenWidth(), aSettings->GetDefaultPenWidth() );

    if( blackAndWhiteMode || color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( LAYER_DEVICE );

    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( !IsVisible() )
        bg = aSettings->GetLayerColor( LAYER_HIDDEN );

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    // Calculate the text orientation, according to the symbol orientation/mirror (needed when
    // draw text in schematic)
    EDA_ANGLE orient = GetTextAngle();

    if( aTransform.y1 )  // Rotate symbol 90 degrees.
    {
        if( orient == ANGLE_HORIZONTAL )
            orient = ANGLE_VERTICAL;
        else
            orient = ANGLE_HORIZONTAL;
    }

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), IsBold(), IsItalic() );

    /*
     * Calculate the text justification, according to the symbol orientation/mirror.
     * This is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the GRText function will also recalculate H and V justifications according to the text
     *   orientation.
     * - When a symbol is mirrored, the text is not mirrored and justifications are complicated
     *   to calculate so the more easily way is to use no justifications (centered text) and
     *   use GetBoundingBox to know the text coordinate considered as centered
    */
    BOX2I bBox = GetBoundingBox();

    // convert coordinates from draw Y axis to symbol_editor Y axis:
    bBox.RevertYAxis();
    VECTOR2I txtpos = bBox.Centre();

    // Calculate pos according to mirror/rotation.
    txtpos = aTransform.TransformCoordinate( txtpos ) + aOffset;

    GRPrintText( DC, txtpos, color, GetShownText( true ), orient, GetTextSize(),
                 GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_CENTER, penWidth, IsItalic(), IsBold(),
                 font );
}


void LIB_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    aList.emplace_back( _( "Style" ), GetTextStyleName() );

    aList.emplace_back( _( "Text Size" ), aFrame->MessageTextFromValue( GetTextWidth() ) );

    switch ( GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:   msg = _( "Left" );   break;
    case GR_TEXT_H_ALIGN_CENTER: msg = _( "Center" ); break;
    case GR_TEXT_H_ALIGN_RIGHT:  msg = _( "Right" );  break;
    }

    aList.emplace_back( _( "H Justification" ), msg );

    switch ( GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP:    msg = _( "Top" );    break;
    case GR_TEXT_V_ALIGN_CENTER: msg = _( "Center" ); break;
    case GR_TEXT_V_ALIGN_BOTTOM: msg = _( "Bottom" ); break;
    }

    aList.emplace_back( _( "V Justification" ), msg );
}


const BOX2I LIB_TEXT::GetBoundingBox() const
{
    /* Y coordinates for LIB_ITEMS are bottom to top, so we must invert the Y position when
     * calling GetTextBox() that works using top to bottom Y axis orientation.
     */
    BOX2I bbox = GetTextBox( -1, true );
    bbox.RevertYAxis();

    // We are using now a bottom to top Y axis.
    VECTOR2I orig = bbox.GetOrigin();
    VECTOR2I end = bbox.GetEnd();

    RotatePoint( orig, GetTextPos(), -GetTextAngle() );
    RotatePoint( end, GetTextPos(), -GetTextAngle() );

    bbox.SetOrigin( orig );
    bbox.SetEnd( end );

    // We are using now a top to bottom Y axis:
    bbox.RevertYAxis();

    return bbox;
}


wxString LIB_TEXT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Graphic Text '%s'" ), KIUI::EllipsizeMenuText( GetText() ) );
}


BITMAPS LIB_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


void LIB_TEXT::BeginEdit( const VECTOR2I& aPosition )
{
    SetTextPos( aPosition );
}


void LIB_TEXT::CalcEdit( const VECTOR2I& aPosition )
{
    SetTextPos( aPosition );
}


static struct LIB_TEXT_DESC
{
    LIB_TEXT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( LIB_TEXT );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_TEXT, LIB_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_TEXT, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( LIB_TEXT ), TYPE_HASH( LIB_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( LIB_TEXT ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( LIB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Mirrored" ) );
        propMgr.Mask( TYPE_HASH( LIB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Visible" ) );
        propMgr.Mask( TYPE_HASH( LIB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( LIB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );

        // Orientation is exposed differently in schematic; mask the base for now
        propMgr.Mask( TYPE_HASH( LIB_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );
    }
} _LIB_TEXT_DESC;
