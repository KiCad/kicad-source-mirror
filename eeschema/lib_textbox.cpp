/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_units.h>
#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <plotters/plotter.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <sch_painter.h>
#include <dialogs/html_message_box.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <core/kicad_algo.h>
#include <trigo.h>
#include <lib_textbox.h>


using KIGFX::SCH_RENDER_SETTINGS;


LIB_TEXTBOX::LIB_TEXTBOX( LIB_SYMBOL* aParent, int aLineWidth, FILL_T aFillType,
                          const wxString& text ) :
        LIB_SHAPE( aParent, SHAPE_T::RECTANGLE, aLineWidth, aFillType, LIB_TEXTBOX_T ),
        EDA_TEXT( schIUScale, text )
{
    SetTextSize( VECTOR2I( schIUScale.MilsToIU( DEFAULT_TEXT_SIZE ),
                           schIUScale.MilsToIU( DEFAULT_TEXT_SIZE ) ) );
    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    SetMultilineAllowed( true );

    int defaultMargin = GetLegacyTextMargin();
    m_marginLeft = defaultMargin;
    m_marginTop = defaultMargin;
    m_marginRight = defaultMargin;
    m_marginBottom = defaultMargin;
}


LIB_TEXTBOX::LIB_TEXTBOX( const LIB_TEXTBOX& aText ) :
        LIB_SHAPE( aText ),
        EDA_TEXT( aText )
{
    m_marginLeft = aText.m_marginLeft;
    m_marginTop = aText.m_marginTop;
    m_marginRight = aText.m_marginRight;
    m_marginBottom = aText.m_marginBottom;
}


int LIB_TEXTBOX::GetLegacyTextMargin() const
{
    return KiROUND( GetTextSize().y * 0.8 );
}


void LIB_TEXTBOX::MirrorHorizontally( const VECTOR2I& center )
{
    // Text is NOT really mirrored; it just has its justification flipped
    if( GetTextAngle() == ANGLE_HORIZONTAL )
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }
}


void LIB_TEXTBOX::MirrorVertically( const VECTOR2I& center )
{
    // Text is NOT really mirrored; it just has its justification flipped
    if( GetTextAngle() == ANGLE_VERTICAL )
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }
}


void LIB_TEXTBOX::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    LIB_SHAPE::Rotate( aCenter, aRotateCCW );
    SetTextAngle( GetTextAngle() == ANGLE_VERTICAL ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );
}


VECTOR2I LIB_TEXTBOX::GetDrawPos() const
{
    BOX2I bbox( VECTOR2I( std::min( m_start.x, m_end.x ), std::min( -m_start.y, -m_end.y ) ),
                VECTOR2I( abs( m_end.x - m_start.x ), abs( m_end.y - m_start.y ) ) );

    VECTOR2I pos( bbox.GetLeft() + m_marginLeft, bbox.GetBottom() - m_marginBottom );

    if( GetTextAngle().IsVertical() )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:
            pos.y = bbox.GetBottom() - m_marginBottom;
            break;
        case GR_TEXT_H_ALIGN_CENTER:
            pos.y = ( bbox.GetTop() + bbox.GetBottom() ) / 2;
            break;
        case GR_TEXT_H_ALIGN_RIGHT:
            pos.y = bbox.GetTop() + m_marginTop;
            break;
        case GR_TEXT_H_ALIGN_INDETERMINATE:
            wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
            break;
        }

        switch( GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:
            pos.x = bbox.GetLeft() + m_marginLeft;
            break;
        case GR_TEXT_V_ALIGN_CENTER:
            pos.x = ( bbox.GetLeft() + bbox.GetRight() ) / 2;
            break;
        case GR_TEXT_V_ALIGN_BOTTOM:
            pos.x = bbox.GetRight() - m_marginRight;
            break;
        case GR_TEXT_V_ALIGN_INDETERMINATE:
            wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
            break;
        }
    }
    else
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:
            pos.x = bbox.GetLeft() + m_marginLeft;
            break;
        case GR_TEXT_H_ALIGN_CENTER:
            pos.x = ( bbox.GetLeft() + bbox.GetRight() ) / 2;
            break;
        case GR_TEXT_H_ALIGN_RIGHT:
            pos.x = bbox.GetRight() - m_marginRight;
            break;
        case GR_TEXT_H_ALIGN_INDETERMINATE:
            wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
            break;
        }

        switch( GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:
            pos.y = bbox.GetTop() + m_marginTop;
            break;
        case GR_TEXT_V_ALIGN_CENTER:
            pos.y = ( bbox.GetTop() + bbox.GetBottom() ) / 2;
            break;
        case GR_TEXT_V_ALIGN_BOTTOM:
            pos.y = bbox.GetBottom() - m_marginBottom;
            break;
        case GR_TEXT_V_ALIGN_INDETERMINATE:
            wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
            break;
        }
    }

    return pos;
}


int LIB_TEXTBOX::compare( const LIB_ITEM& aOther, int aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_TEXTBOX_T );

    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const LIB_TEXTBOX* tmp = static_cast<const LIB_TEXTBOX*>( &aOther );

    int result = GetText().CmpNoCase( tmp->GetText() );

    if( result != 0 )
        return result;

    if( GetTextWidth() != tmp->GetTextWidth() )
        return GetTextWidth() - tmp->GetTextWidth();

    if( GetTextHeight() != tmp->GetTextHeight() )
        return GetTextHeight() - tmp->GetTextHeight();

    if( IsBold() != tmp->IsBold() )
        return IsBold() - tmp->IsBold();

    if( IsItalic() != tmp->IsItalic() )
        return IsItalic() - tmp->IsItalic();

    if( GetHorizJustify() != tmp->GetHorizJustify() )
        return GetHorizJustify() - tmp->GetHorizJustify();

    if( GetTextAngle().AsTenthsOfADegree() != tmp->GetTextAngle().AsTenthsOfADegree() )
        return GetTextAngle().AsTenthsOfADegree() - tmp->GetTextAngle().AsTenthsOfADegree();

    if( GetMarginLeft() != tmp->GetMarginLeft() )
        return GetMarginLeft() - tmp->GetMarginLeft();

    if( GetMarginTop() != tmp->GetMarginTop() )
        return GetMarginTop() - tmp->GetMarginTop();

    if( GetMarginRight() != tmp->GetMarginRight() )
        return GetMarginRight() - tmp->GetMarginRight();

    if( GetMarginBottom() != tmp->GetMarginBottom() )
        return GetMarginBottom() - tmp->GetMarginBottom();

    return EDA_SHAPE::Compare( &static_cast<const LIB_SHAPE&>( aOther ) );
}


KIFONT::FONT* LIB_TEXTBOX::getDrawFont() const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    return font;
}


void LIB_TEXTBOX::print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, void* aData,
                         const TRANSFORM& aTransform, bool aDimmed )
{
    if( IsPrivate() )
        return;

    bool       forceNoFill = static_cast<bool>( aData );
    bool       blackAndWhiteMode = GetGRForceBlackPenState();
    int        penWidth = GetEffectivePenWidth( aSettings );
    COLOR4D    color = GetStroke().GetColor();
    LINE_STYLE lineStyle = GetStroke().GetLineStyle();

    wxDC*    DC = aSettings->GetPrintDC();
    VECTOR2I pt1 = aTransform.TransformCoordinate( m_start ) + aOffset;
    VECTOR2I pt2 = aTransform.TransformCoordinate( m_end ) + aOffset;

    if( !forceNoFill && GetFillMode() == FILL_T::FILLED_WITH_COLOR && !blackAndWhiteMode )
        GRFilledRect( DC, pt1, pt2, penWidth, GetFillColor(), GetFillColor() );

    if( penWidth > 0 )
    {
        penWidth = std::max( penWidth, aSettings->GetMinPenWidth() );

        if( blackAndWhiteMode || color == COLOR4D::UNSPECIFIED )
            color = aSettings->GetLayerColor( LAYER_DEVICE );

        COLOR4D bg = aSettings->GetBackgroundColor();

        if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
            bg = COLOR4D::WHITE;

        if( aDimmed )
        {
            color.Desaturate( );
            color = color.Mix( bg, 0.5f );
        }

        if( lineStyle == LINE_STYLE::DEFAULT )
            lineStyle = LINE_STYLE::SOLID;

        if( lineStyle <= LINE_STYLE::FIRST_TYPE )
        {
            GRRect( DC, pt1, pt2, penWidth, color );
        }
        else
        {
            std::vector<SHAPE*> shapes = MakeEffectiveShapes( true );

            for( SHAPE* shape : shapes )
            {
                STROKE_PARAMS::Stroke( shape, lineStyle, penWidth, aSettings,
                                       [&]( const VECTOR2I& a, const VECTOR2I& b )
                                       {
                                            VECTOR2I pts = aTransform.TransformCoordinate( a ) + aOffset;
                                            VECTOR2I pte = aTransform.TransformCoordinate( b ) + aOffset;
                                            GRLine( DC, pts.x, pts.y, pte.x, pte.y, penWidth, color );
                                       } );
            }

            for( SHAPE* shape : shapes )
                delete shape;
        }
    }

    LIB_TEXTBOX text( *this );

    color = GetTextColor();

    if( blackAndWhiteMode || color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( LAYER_DEVICE );

    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    penWidth = std::max( GetEffectiveTextPenWidth(), aSettings->GetMinPenWidth() );

    if( aTransform.y1 )
    {
        text.SetTextAngle( text.GetTextAngle() == ANGLE_HORIZONTAL ? ANGLE_VERTICAL
                                                                   : ANGLE_HORIZONTAL );
    }

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), IsBold(), IsItalic() );

    // NB: GetDrawPos() will want Symbol Editor (upside-down) coordinates
    text.SetStart( VECTOR2I( pt1.x, -pt1.y ) );
    text.SetEnd( VECTOR2I( pt2.x, -pt2.y ) );

    GRPrintText( DC, text.GetDrawPos(), color, text.GetShownText( true ), text.GetTextAngle(),
                 text.GetTextSize(), text.GetHorizJustify(), text.GetVertJustify(), penWidth,
                 text.IsItalic(), text.IsBold(), font, GetFontMetrics() );
}


wxString LIB_TEXTBOX::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    KIFONT::FONT* font = GetFont();
    VECTOR2D      size = GetEnd() - GetStart();
    int           colWidth;

    if( GetTextAngle().IsVertical() )
        colWidth = abs( size.y ) - ( GetMarginTop() + GetMarginBottom() );
    else
        colWidth = abs( size.x ) - ( GetMarginLeft() + GetMarginRight() );

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    return text;
}


bool LIB_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    if( aAccuracy < schIUScale.MilsToIU( MINIMUM_SELECTION_DISTANCE ) )
        aAccuracy = schIUScale.MilsToIU( MINIMUM_SELECTION_DISTANCE );

    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool LIB_TEXTBOX::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


wxString LIB_TEXTBOX::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Graphic Text Box" ) );
}


BITMAPS LIB_TEXTBOX::GetMenuImage() const
{
    return BITMAPS::add_textbox;
}


void LIB_TEXTBOX::Plot( PLOTTER* aPlotter, bool aBackground, const VECTOR2I& aOffset,
                        const TRANSFORM& aTransform, bool aDimmed ) const
{
    wxASSERT( aPlotter != nullptr );

    if( IsPrivate() )
        return;

    if( aBackground )
    {
        LIB_SHAPE::Plot( aPlotter, aBackground, aOffset, aTransform, aDimmed );
        return;
    }

    RENDER_SETTINGS* renderSettings = aPlotter->RenderSettings();
    VECTOR2I         start = aTransform.TransformCoordinate( m_start ) + aOffset;
    VECTOR2I         end = aTransform.TransformCoordinate( m_end ) + aOffset;
    COLOR4D          bg = renderSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    int        penWidth = GetEffectivePenWidth( renderSettings );
    COLOR4D    color = GetStroke().GetColor();
    LINE_STYLE lineStyle = GetStroke().GetLineStyle();

    if( penWidth > 0 )
    {
        if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
            color = renderSettings->GetLayerColor( LAYER_DEVICE );

        if( lineStyle == LINE_STYLE::DEFAULT )
            lineStyle = LINE_STYLE::SOLID;

        if( aDimmed )
        {
            color.Desaturate( );
            color = color.Mix( bg, 0.5f );
        }

        aPlotter->SetColor( color );
        aPlotter->SetDash( penWidth, lineStyle );
        aPlotter->Rect( start, end, FILL_T::NO_FILL, penWidth );
        aPlotter->SetDash( penWidth, LINE_STYLE::SOLID );
    }

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( renderSettings->GetDefaultFont(), IsBold(), IsItalic() );

    LIB_TEXTBOX text( *this );

    color = GetTextColor();

    if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
        color = renderSettings->GetLayerColor( LAYER_DEVICE );

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    penWidth = std::max( GetEffectiveTextPenWidth(), aPlotter->RenderSettings()->GetMinPenWidth() );

    if( aTransform.y1 )
    {
        text.SetTextAngle( text.GetTextAngle() == ANGLE_HORIZONTAL ? ANGLE_VERTICAL
                                                                   : ANGLE_HORIZONTAL );
    }

    // NB: GetDrawPos() will want Symbol Editor (upside-down) coordinates
    text.SetStart( VECTOR2I( start.x, -start.y ) );
    text.SetEnd( VECTOR2I( end.x, -end.y ) );

    std::vector<VECTOR2I> positions;
    wxArrayString strings_list;
    wxStringSplit( GetShownText( true ), strings_list, '\n' );
    positions.reserve( strings_list.Count() );

    text.GetLinePositions( positions, (int) strings_list.Count() );

    TEXT_ATTRIBUTES attrs = text.GetAttributes();
    attrs.m_StrokeWidth = penWidth;
    attrs.m_Multiline = false;

    for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
    {
        aPlotter->PlotText( positions[ii], color, strings_list.Item( ii ), attrs, font,
                            GetFontMetrics() );
    }
}


void LIB_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), aFrame->MessageTextFromValue( GetTextWidth() ) );

    aList.emplace_back( _( "Box Width" ),
                        aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );

    aList.emplace_back( _( "Box Height" ),
                        aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ) );

    m_stroke.GetMsgPanelInfo( aFrame, aList );
}


bool LIB_TEXTBOX::operator==( const LIB_ITEM& aOther ) const
{
    if( aOther.Type() != LIB_TEXTBOX_T )
        return false;

    const LIB_TEXTBOX& other = static_cast<const LIB_TEXTBOX&>( aOther );

    return LIB_SHAPE::operator==( other ) && EDA_TEXT::operator==( other );
}


double LIB_TEXTBOX::Similarity( const LIB_ITEM& aOther ) const
{
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( aOther.Type() != LIB_TEXTBOX_T )
        return 0.0;

    const LIB_TEXTBOX& other = static_cast<const LIB_TEXTBOX&>( aOther );

    double similarity = SimilarityBase( other );
    similarity *= LIB_SHAPE::Similarity( other );
    similarity *= EDA_TEXT::Similarity( other );

    return similarity;
}


void LIB_TEXTBOX::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 3;
    aLayers[0] = IsPrivate() ? LAYER_PRIVATE_NOTES    : LAYER_DEVICE;
    aLayers[1] = IsPrivate() ? LAYER_NOTES_BACKGROUND : LAYER_DEVICE_BACKGROUND;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
}


static struct LIB_TEXTBOX_DESC
{
    LIB_TEXTBOX_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( LIB_TEXTBOX );

        propMgr.AddTypeCast( new TYPE_CAST<LIB_TEXTBOX, LIB_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_TEXTBOX, EDA_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_TEXTBOX, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( LIB_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Shape" ) );

        propMgr.Mask( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Visible" ) );
        propMgr.Mask( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );
        propMgr.Mask( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );

        propMgr.AddProperty( new PROPERTY<LIB_TEXTBOX, int>( _HKI( "Margin Left" ),
                &LIB_TEXTBOX::SetMarginLeft, &LIB_TEXTBOX::GetMarginLeft, PROPERTY_DISPLAY::PT_SIZE ) );
        propMgr.AddProperty( new PROPERTY<LIB_TEXTBOX, int>( _HKI( "Margin Top" ),
                &LIB_TEXTBOX::SetMarginTop, &LIB_TEXTBOX::GetMarginTop, PROPERTY_DISPLAY::PT_SIZE ) );
        propMgr.AddProperty( new PROPERTY<LIB_TEXTBOX, int>( _HKI( "Margin Right" ),
                &LIB_TEXTBOX::SetMarginRight, &LIB_TEXTBOX::GetMarginRight, PROPERTY_DISPLAY::PT_SIZE ) );
        propMgr.AddProperty( new PROPERTY<LIB_TEXTBOX, int>( _HKI( "Margin Bottom" ),
                &LIB_TEXTBOX::SetMarginBottom, &LIB_TEXTBOX::GetMarginBottom, PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<LIB_TEXTBOX, int>( _HKI( "Text Size" ),
                &LIB_TEXTBOX::SetLibTextSize, &LIB_TEXTBOX::GetLibTextSize, PROPERTY_DISPLAY::PT_SIZE ),
                _HKI( "Text Properties" ) );

        propMgr.Mask( TYPE_HASH( LIB_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );
    }
} _LIB_TEXTBOX_DESC;
