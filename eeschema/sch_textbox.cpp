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
#include <sch_textbox.h>

using KIGFX::SCH_RENDER_SETTINGS;


SCH_TEXTBOX::SCH_TEXTBOX( int aLineWidth, FILL_T aFillType, const wxString& text ) :
        SCH_SHAPE( SHAPE_T::RECT, aLineWidth, aFillType, SCH_TEXTBOX_T ),
        EDA_TEXT( text )
{
    m_layer = LAYER_NOTES;

    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    SetMultilineAllowed( true );
}


SCH_TEXTBOX::SCH_TEXTBOX( const SCH_TEXTBOX& aText ) :
        SCH_SHAPE( aText ),
        EDA_TEXT( aText )
{ }


int SCH_TEXTBOX::GetTextMargin() const
{
    return KiROUND( GetTextSize().y * 0.8 );
}


void SCH_TEXTBOX::MirrorHorizontally( int aCenter )
{
    // Text is NOT really mirrored; it just has its justification flipped
    if( GetTextAngle() == ANGLE_HORIZONTAL )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT ); break;
        case GR_TEXT_H_ALIGN_CENTER:                                           break;
        case GR_TEXT_H_ALIGN_RIGHT:  SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );  break;
        }
    }
}


void SCH_TEXTBOX::MirrorVertically( int aCenter )
{
    // Text is NOT really mirrored; it just has its justification flipped
    if( GetTextAngle() == ANGLE_VERTICAL )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT ); break;
        case GR_TEXT_H_ALIGN_CENTER:                                           break;
        case GR_TEXT_H_ALIGN_RIGHT:  SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );  break;
        }
    }
}


void SCH_TEXTBOX::Rotate( const VECTOR2I& aCenter )
{
    SCH_SHAPE::Rotate( aCenter );
    SetTextAngle( GetTextAngle() == ANGLE_VERTICAL ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );
}


void SCH_TEXTBOX::Rotate90( bool aClockwise )
{
    SetTextAngle( GetTextAngle() == ANGLE_VERTICAL ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );
}


VECTOR2I SCH_TEXTBOX::GetDrawPos() const
{
    int   margin = GetTextMargin();
    BOX2I bbox( m_start, m_end - m_start );

    bbox.Normalize();

    if( GetTextAngle() == ANGLE_VERTICAL )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:
            return VECTOR2I( bbox.GetLeft() + margin, bbox.GetBottom() - margin );
        case GR_TEXT_H_ALIGN_CENTER:
            return VECTOR2I( bbox.GetLeft() + margin, ( bbox.GetTop() + bbox.GetBottom() ) / 2 );
        case GR_TEXT_H_ALIGN_RIGHT:
            return VECTOR2I( bbox.GetLeft() + margin, bbox.GetTop() + margin );
        }
    }
    else
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:
            return VECTOR2I( bbox.GetLeft() + margin, bbox.GetTop() + margin );
        case GR_TEXT_H_ALIGN_CENTER:
            return VECTOR2I( ( bbox.GetLeft() + bbox.GetRight() ) / 2, bbox.GetTop() + margin );
        case GR_TEXT_H_ALIGN_RIGHT:
            return VECTOR2I( bbox.GetRight() - margin, bbox.GetTop() + margin );
        }
    }

    // Dummy default.  Should never reach here
    return VECTOR2I( bbox.GetLeft() + margin, bbox.GetBottom() - margin );
}


void SCH_TEXTBOX::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXTBOX* item = static_cast<SCH_TEXTBOX*>( aItem );

    std::swap( m_layer, item->m_layer );

    SwapText( *item );
    SwapAttributes( *item );

    SCH_SHAPE::SwapData( aItem );
}


bool SCH_TEXTBOX::operator<( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto other = static_cast<const SCH_TEXTBOX*>( &aItem );

    if( GetLayer() != other->GetLayer() )
            return GetLayer() < other->GetLayer();

    if( GetPosition().x != other->GetPosition().x )
        return GetPosition().x < other->GetPosition().x;

    if( GetPosition().y != other->GetPosition().y )
        return GetPosition().y < other->GetPosition().y;

    return GetText() < other->GetText();
}


KIFONT::FONT* SCH_TEXTBOX::GetDrawFont() const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    return font;
}


void SCH_TEXTBOX::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    wxDC*          DC = aSettings->GetPrintDC();
    int            penWidth = GetPenWidth();
    bool           blackAndWhiteMode = GetGRForceBlackPenState();
    VECTOR2I       pt1 = GetStart();
    VECTOR2I       pt2 = GetEnd();
    COLOR4D        color = GetStroke().GetColor();
    PLOT_DASH_TYPE lineStyle = GetStroke().GetPlotStyle();

    if( GetFillMode() == FILL_T::FILLED_WITH_COLOR && !blackAndWhiteMode )
        GRFilledRect( DC, pt1, pt2, 0, GetFillColor(), GetFillColor() );

    if( penWidth > 0 )
    {
        penWidth = std::max( penWidth, aSettings->GetMinPenWidth() );

        if( blackAndWhiteMode || color == COLOR4D::UNSPECIFIED )
            color = aSettings->GetLayerColor( m_layer );

        if( lineStyle == PLOT_DASH_TYPE::DEFAULT )
            lineStyle = PLOT_DASH_TYPE::SOLID;

        if( lineStyle == PLOT_DASH_TYPE::SOLID )
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
                                           GRLine( DC, a.x, a.y, b.x, b.y, penWidth, color );
                                       } );
            }

            for( SHAPE* shape : shapes )
                delete shape;
        }
    }

    color = GetTextColor();

    if( blackAndWhiteMode || color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( m_layer );

    EDA_TEXT::Print( aSettings, aOffset, color );
}


wxString SCH_TEXTBOX::GetShownText( int aDepth ) const
{
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                if( token->Contains( ':' ) )
                {
                    if( Schematic()->ResolveCrossReference( token, aDepth ) )
                        return true;
                }
                else
                {
                    SCHEMATIC* schematic = Schematic();
                    SCH_SHEET* sheet = schematic ? schematic->CurrentSheet().Last() : nullptr;

                    if( sheet && sheet->ResolveTextVar( token, aDepth + 1 ) )
                        return true;
                }

                return false;
            };

    std::function<bool( wxString* )> schematicTextResolver =
            [&]( wxString* token ) -> bool
            {
                return Schematic()->ResolveTextVar( token, aDepth + 1 );
            };

    wxString text = EDA_TEXT::GetShownText();

    if( HasTextVars() )
    {
        PROJECT* project = nullptr;

        if( Schematic() )
            project = &Schematic()->Prj();

        if( aDepth < 10 )
            text = ExpandTextVars( text, &textResolver, &schematicTextResolver, project );
    }

    KIFONT::FONT* font = GetDrawFont();
    VECTOR2D      size = GetEnd() - GetStart();
    int           colWidth = GetTextAngle() == ANGLE_HORIZONTAL ? size.x : size.y;

    colWidth = abs( colWidth ) - GetTextMargin() * 2;
    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    return text;
}


bool SCH_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_TEXTBOX::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


wxString SCH_TEXTBOX::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Graphic Text Box" ) );
}


BITMAPS SCH_TEXTBOX::GetMenuImage() const
{
    return BITMAPS::add_textbox;
}


void SCH_TEXTBOX::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    if( aBackground )
    {
        SCH_SHAPE::Plot( aPlotter, aBackground );
        return;
    }

    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    KIFONT::FONT*    font = GetDrawFont();
    int              penWidth = GetPenWidth();
    COLOR4D          color = GetStroke().GetColor();
    PLOT_DASH_TYPE   lineStyle = GetStroke().GetPlotStyle();

    if( penWidth > 0 )
    {
        penWidth = std::max( penWidth, settings->GetMinPenWidth() );

        if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
            color = settings->GetLayerColor( m_layer );

        if( lineStyle == PLOT_DASH_TYPE::DEFAULT )
            lineStyle = PLOT_DASH_TYPE::SOLID;

        aPlotter->SetColor( color );
        aPlotter->SetDash( lineStyle );
        aPlotter->Rect( m_start, m_end, FILL_T::NO_FILL, penWidth );
        aPlotter->SetDash( PLOT_DASH_TYPE::SOLID );
    }

    color = GetTextColor();

    if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
        color = settings->GetLayerColor( m_layer );

    penWidth = GetEffectiveTextPenWidth( settings->GetDefaultPenWidth() );
    penWidth = std::max( penWidth, settings->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    std::vector<VECTOR2I> positions;
    wxArrayString strings_list;
    wxStringSplit( GetShownText(), strings_list, '\n' );
    positions.reserve( strings_list.Count() );

    GetLinePositions( positions, (int) strings_list.Count() );

    for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
    {
        aPlotter->Text( positions[ii], color, strings_list.Item( ii ), GetTextAngle(),
                        GetTextSize(), GetHorizJustify(), GetVertJustify(), penWidth, IsItalic(),
                        IsBold(), false, font );
    }
}


void SCH_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), UnescapeString( ShortenedText() ) );

    aList.emplace_back( _( "Font" ), GetDrawFont()->GetName() );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), MessageTextFromValue( units, GetTextWidth() ) );

    wxString msg = MessageTextFromValue( units, std::abs( GetEnd().x - GetStart().x ) );
    aList.emplace_back( _( "Box Width" ), msg );

    msg = MessageTextFromValue( units, std::abs( GetEnd().y - GetStart().y ) );
    aList.emplace_back( _( "Box Height" ), msg );

    m_stroke.GetMsgPanelInfo( units, aList );
}
