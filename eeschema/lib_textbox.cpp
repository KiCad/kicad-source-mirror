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

#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <plotters/plotter.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <sch_painter.h>
#include <wx/log.h>
#include <dialogs/html_message_box.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <core/kicad_algo.h>
#include <trigo.h>
#include <lib_textbox.h>

using KIGFX::SCH_RENDER_SETTINGS;


LIB_TEXTBOX::LIB_TEXTBOX( LIB_SYMBOL* aParent, int aLineWidth, FILL_T aFillType,
                          const wxString& text ) :
        LIB_SHAPE( aParent, SHAPE_T::RECT, aLineWidth, aFillType, LIB_TEXTBOX_T ),
        EDA_TEXT( text )
{
    SetTextSize( wxSize( Mils2iu( DEFAULT_TEXT_SIZE ), Mils2iu( DEFAULT_TEXT_SIZE ) ) );
    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    SetMultilineAllowed( true );
}


LIB_TEXTBOX::LIB_TEXTBOX( const LIB_TEXTBOX& aText ) :
        LIB_SHAPE( aText ),
        EDA_TEXT( aText )
{ }


int LIB_TEXTBOX::GetTextMargin() const
{
    return KiROUND( GetTextSize().y * 0.8 );
}


void LIB_TEXTBOX::Offset( const VECTOR2I& aOffset )
{
    LIB_SHAPE::Offset( aOffset );
    UpdateTextPosition();
}


void LIB_TEXTBOX::MoveTo( const VECTOR2I& newPosition )
{
    LIB_SHAPE::MoveTo( newPosition );
    UpdateTextPosition();
}


void LIB_TEXTBOX::MirrorHorizontally( const VECTOR2I& center )
{
    // Text is NOT really mirrored; it just has its justification flipped
    if( GetTextAngle() == ANGLE_HORIZONTAL )
    {
        SetHorizJustify( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT ? GR_TEXT_H_ALIGN_LEFT
                                                                    : GR_TEXT_H_ALIGN_RIGHT );
    }

    UpdateTextPosition();
}


void LIB_TEXTBOX::MirrorVertically( const VECTOR2I& center )
{
    // Text is NOT really mirrored; it just has its justification flipped
    if( GetTextAngle() == ANGLE_VERTICAL )
    {
        SetHorizJustify( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT ? GR_TEXT_H_ALIGN_LEFT
                                                                    : GR_TEXT_H_ALIGN_RIGHT );
    }

    UpdateTextPosition();
}


void LIB_TEXTBOX::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    LIB_SHAPE::Rotate( aCenter, aRotateCCW );

    SetTextAngle( GetTextAngle() == ANGLE_VERTICAL ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );
    UpdateTextPosition();
}


void LIB_TEXTBOX::UpdateTextPosition()
{
    int   margin = GetTextMargin();
    BOX2I bbox( VECTOR2I( std::min( m_start.x, m_end.x ), std::min( -m_start.y, -m_end.y ) ),
                VECTOR2I( abs( m_end.x - m_start.x ), abs( m_end.y - m_start.y ) ) );

    if( GetTextAngle() == ANGLE_HORIZONTAL )
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetTextPos( VECTOR2I( bbox.GetRight() - margin, bbox.GetTop() + margin )  );
        else
            SetTextPos( VECTOR2I( bbox.GetLeft() + margin, bbox.GetTop() + margin )  );
    }
    else
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetTextPos( VECTOR2I( bbox.GetLeft() + margin, bbox.GetTop() + margin )  );
        else
            SetTextPos( VECTOR2I( bbox.GetLeft() + margin, bbox.GetBottom() - margin )  );
    }
}


int LIB_TEXTBOX::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
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

    return EDA_SHAPE::Compare( &static_cast<const LIB_SHAPE&>( aOther ) );
}


int LIB_TEXTBOX::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


KIFONT::FONT* LIB_TEXTBOX::GetDrawFont() const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    return font;
}


void LIB_TEXTBOX::print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, void* aData,
                         const TRANSFORM& aTransform )
{
    bool forceNoFill = static_cast<bool>( aData );
    int  penWidth = GetEffectivePenWidth( aSettings );

    if( forceNoFill && IsFilled() && penWidth == 0 )
        return;

    wxDC*    DC = aSettings->GetPrintDC();
    VECTOR2I pt1 = aTransform.TransformCoordinate( m_start ) + aOffset;
    VECTOR2I pt2 = aTransform.TransformCoordinate( m_end ) + aOffset;
    COLOR4D  color = aSettings->GetLayerColor( IsPrivate() ? LAYER_NOTES : LAYER_DEVICE );

    if( !forceNoFill && GetFillMode() == FILL_T::FILLED_WITH_COLOR )
        GRFilledRect( DC, pt1, pt2, penWidth, color, GetFillColor() );

    if( GetStroke().GetColor() != COLOR4D::UNSPECIFIED )
        color = GetStroke().GetColor();

    penWidth = std::max( penWidth, aSettings->GetMinPenWidth() );

    if( GetStroke().GetPlotStyle() <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        GRRect( DC, pt1, pt2, penWidth, color );
    }
    else
    {
        std::vector<SHAPE*> shapes = MakeEffectiveShapes( true );

        for( SHAPE* shape : shapes )
        {
            STROKE_PARAMS::Stroke( shape, GetStroke().GetPlotStyle(), penWidth, aSettings,
                                   [&]( const VECTOR2I& a, const VECTOR2I& b )
                                   {
                                       GRLine( DC, a.x, a.y, b.x, b.y, penWidth, color );
                                   } );
        }

        for( SHAPE* shape : shapes )
            delete shape;
    }

    LIB_TEXTBOX text( *this );

    penWidth = std::max( GetEffectiveTextPenWidth(), aSettings->GetMinPenWidth() );

    if( aTransform.y1 )
    {
        text.SetTextAngle( text.GetTextAngle() == ANGLE_HORIZONTAL ? ANGLE_VERTICAL
                                                                   : ANGLE_HORIZONTAL );
    }

    // NB: UpdateTextPosition() wants Symbol Editor (upside-down) coordinates
    text.SetStart( VECTOR2I( pt1.x, -pt1.y ) );
    text.SetEnd( VECTOR2I( pt2.x, -pt2.y ) );
    text.UpdateTextPosition();

    GRPrintText( DC, text.GetTextPos(), color, text.GetShownText(), text.GetTextAngle(),
                 text.GetTextSize(), text.GetHorizJustify(), text.GetVertJustify(), penWidth,
                 text.IsItalic(), text.IsBold(), text.GetDrawFont() );
}


wxString LIB_TEXTBOX::GetShownText( int aDepth ) const
{
    wxString text = EDA_TEXT::GetShownText();

    KIFONT::FONT* font = GetDrawFont();
    VECTOR2D      size = GetEnd() - GetStart();
    int           colWidth = GetTextAngle() == ANGLE_HORIZONTAL ? size.x : size.y;

    colWidth = abs( colWidth ) - GetTextMargin() * 2;
    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    return text;
}


bool LIB_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    if( aAccuracy < Mils2iu( MINIMUM_SELECTION_DISTANCE ) )
        aAccuracy = Mils2iu( MINIMUM_SELECTION_DISTANCE );

    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool LIB_TEXTBOX::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


wxString LIB_TEXTBOX::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Graphic Text Box" ) );
}


BITMAPS LIB_TEXTBOX::GetMenuImage() const
{
    return BITMAPS::add_textbox;
}


void LIB_TEXTBOX::Plot( PLOTTER* aPlotter, const VECTOR2I& aOffset, bool aFill,
                        const TRANSFORM& aTransform ) const
{
    wxASSERT( aPlotter != nullptr );

    VECTOR2I  start = aTransform.TransformCoordinate( m_start ) + aOffset;
    VECTOR2I  end = aTransform.TransformCoordinate( m_end ) + aOffset;
    int       penWidth = GetEffectivePenWidth( aPlotter->RenderSettings() );
    FILL_T    fill = aFill ? m_fill : FILL_T::NO_FILL;
    COLOR4D   color = aPlotter->RenderSettings()->GetLayerColor( IsPrivate() ? LAYER_NOTES
                                                                             : LAYER_DEVICE );
    if( fill != FILL_T::NO_FILL )
    {
        COLOR4D fillColor = color;

        if( aPlotter->GetColorMode() )
        {
            if( fill == FILL_T::FILLED_WITH_BG_BODYCOLOR )
                fillColor = aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
            else if( fill == FILL_T::FILLED_WITH_COLOR )
                fillColor = GetFillColor();
        }

        aPlotter->SetColor( fillColor );
        aPlotter->Rect( start, end, fill, 0 );

        fill = FILL_T::NO_FILL;
    }

    aPlotter->SetColor( color );
    aPlotter->Rect( start, end, fill, penWidth );

    LIB_TEXTBOX text( *this );

    penWidth = std::max( GetEffectiveTextPenWidth(), aPlotter->RenderSettings()->GetMinPenWidth() );

    if( aTransform.y1 )
    {
        text.SetTextAngle( text.GetTextAngle() == ANGLE_HORIZONTAL ? ANGLE_VERTICAL
                                                                   : ANGLE_HORIZONTAL );
    }

    // NB: UpdateTextPosition() wants Symbol Editor (upside-down) coordinates
    text.SetStart( VECTOR2I( start.x, -start.y ) );
    text.SetEnd( VECTOR2I( end.x, -end.y ) );
    text.UpdateTextPosition();

    std::vector<VECTOR2I> positions;
    wxArrayString strings_list;
    wxStringSplit( GetShownText(), strings_list, '\n' );
    positions.reserve( strings_list.Count() );

    text.GetLinePositions( positions, (int) strings_list.Count() );

    for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
    {
        aPlotter->Text( positions[ii], color, strings_list.Item( ii ), text.GetTextAngle(),
                        text.GetTextSize(), text.GetHorizJustify(), text.GetVertJustify(),
                        penWidth, text.IsItalic(), text.IsBold(), false, GetDrawFont() );
    }
}


void LIB_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), UnescapeString( GetText() ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), MessageTextFromValue( aFrame->GetUserUnits(),
                                                                GetTextWidth() ) );
}


void LIB_TEXTBOX::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 3;
    aLayers[0] = IsPrivate() ? LAYER_NOTES            : LAYER_DEVICE;
    aLayers[1] = IsPrivate() ? LAYER_NOTES_BACKGROUND : LAYER_DEVICE_BACKGROUND;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
}
