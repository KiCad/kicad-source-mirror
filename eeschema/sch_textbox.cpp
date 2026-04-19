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

#include <google/protobuf/any.pb.h>

#include <advanced_config.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
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
#include <wx/log.h>
#include <dialogs/html_message_box.h>
#include <project/project_file.h>
#include <trigo.h>
#include <geometry/geometry_utils.h>
#include <sch_textbox.h>
#include <tools/sch_navigate_tool.h>
#include <markup_parser.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <api/schematic/schematic_types.pb.h>


SCH_TEXTBOX::SCH_TEXTBOX( SCH_LAYER_ID aLayer, int aLineWidth, FILL_T aFillType, const wxString& aText,
                          KICAD_T aType ) :
        SCH_SHAPE( SHAPE_T::RECTANGLE, aLayer, aLineWidth, aFillType, aType ),
        EDA_TEXT( schIUScale, aText )
{
    m_layer = aLayer;

    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    SetMultilineAllowed( true );

    m_excludedFromSim = false;

    int defaultMargin = GetLegacyTextMargin();
    m_marginLeft = defaultMargin;
    m_marginTop = defaultMargin;
    m_marginRight = defaultMargin;
    m_marginBottom = defaultMargin;
}


SCH_TEXTBOX::SCH_TEXTBOX( const SCH_TEXTBOX& aText ) :
        SCH_SHAPE( aText ),
        EDA_TEXT( aText )
{
    m_excludedFromSim = aText.m_excludedFromSim;
    m_marginLeft = aText.m_marginLeft;
    m_marginTop = aText.m_marginTop;
    m_marginRight = aText.m_marginRight;
    m_marginBottom = aText.m_marginBottom;
}


void SCH_TEXTBOX::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::common;

    kiapi::schematic::types::SchematicTextBox textBox;

    textBox.mutable_id()->set_value( m_Uuid.AsStdString() );
    textBox.set_locked( IsLocked() ? types::LockedState::LS_LOCKED : types::LockedState::LS_UNLOCKED );
    textBox.set_exclude_from_sim( GetExcludedFromSim() );
    PackDistance( *textBox.mutable_margin_left(), GetMarginLeft(), schIUScale );
    PackDistance( *textBox.mutable_margin_top(), GetMarginTop(), schIUScale );
    PackDistance( *textBox.mutable_margin_right(), GetMarginRight(), schIUScale );
    PackDistance( *textBox.mutable_margin_bottom(), GetMarginBottom(), schIUScale );

    types::TextBox& text = *textBox.mutable_textbox();
    PackVector2( *text.mutable_top_left(), GetPosition(), schIUScale );
    PackVector2( *text.mutable_bottom_right(), GetEnd(), schIUScale );
    text.set_text( GetText().ToUTF8() );

    types::TextAttributes* attrs = text.mutable_attributes();

    if( GetFont() )
        attrs->set_font_name( GetFont()->GetName().ToUTF8() );

    attrs->set_horizontal_alignment( ToProtoEnum<GR_TEXT_H_ALIGN_T, types::HorizontalAlignment>( GetHorizJustify() ) );
    attrs->set_vertical_alignment( ToProtoEnum<GR_TEXT_V_ALIGN_T, types::VerticalAlignment>( GetVertJustify() ) );
    attrs->mutable_angle()->set_value_degrees( GetTextAngleDegrees() );
    attrs->set_line_spacing( GetLineSpacing() );
    PackDistance( *attrs->mutable_stroke_width(), GetTextThickness(), schIUScale );
    attrs->set_italic( IsItalic() );
    attrs->set_bold( IsBold() );
    attrs->set_underlined( GetAttributes().m_Underlined );
    attrs->set_mirrored( IsMirrored() );
    attrs->set_multiline( IsMultilineAllowed() );
    attrs->set_keep_upright( IsKeepUpright() );
    PackVector2( *attrs->mutable_size(), GetTextSize(), schIUScale );

    if( GetTextColor() != COLOR4D::UNSPECIFIED )
        PackColor( *attrs->mutable_color(), GetTextColor() );

    types::StrokeAttributes* stroke = textBox.mutable_graphic_attributes()->mutable_stroke();
    PackDistance( *stroke->mutable_width(), GetStroke().GetWidth(), schIUScale );
    stroke->set_style( ToProtoEnum<LINE_STYLE, types::StrokeLineStyle>( GetStroke().GetLineStyle() ) );

    if( GetStroke().GetColor() != COLOR4D::UNSPECIFIED )
        PackColor( *stroke->mutable_color(), GetStroke().GetColor() );

    types::GraphicFillAttributes* fill = textBox.mutable_graphic_attributes()->mutable_fill();
    fill->set_fill_type( ToProtoEnum<FILL_T, types::GraphicFillType>( GetFillMode() ) );

    if( GetFillColor() != COLOR4D::UNSPECIFIED )
        PackColor( *fill->mutable_color(), GetFillColor() );

    aContainer.PackFrom( textBox );
}


bool SCH_TEXTBOX::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::common;

    kiapi::schematic::types::SchematicTextBox textBox;

    if( !aContainer.UnpackTo( &textBox ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( textBox.id().value() );
    SetLocked( textBox.locked() == types::LockedState::LS_LOCKED );
    SetExcludedFromSim( textBox.exclude_from_sim() );
    SetPosition( UnpackVector2( textBox.textbox().top_left(), schIUScale ) );
    SetEnd( UnpackVector2( textBox.textbox().bottom_right(), schIUScale ) );
    SetText( wxString::FromUTF8( textBox.textbox().text() ) );

    if( textBox.has_margin_left() )
        SetMarginLeft( UnpackDistance( textBox.margin_left(), schIUScale ) );

    if( textBox.has_margin_top() )
        SetMarginTop( UnpackDistance( textBox.margin_top(), schIUScale ) );

    if( textBox.has_margin_right() )
        SetMarginRight( UnpackDistance( textBox.margin_right(), schIUScale ) );

    if( textBox.has_margin_bottom() )
        SetMarginBottom( UnpackDistance( textBox.margin_bottom(), schIUScale ) );

    if( textBox.textbox().has_attributes() )
    {
        TEXT_ATTRIBUTES attrs = GetAttributes();

        attrs.m_Bold = textBox.textbox().attributes().bold();
        attrs.m_Italic = textBox.textbox().attributes().italic();
        attrs.m_Underlined = textBox.textbox().attributes().underlined();
        attrs.m_Mirrored = textBox.textbox().attributes().mirrored();
        attrs.m_Multiline = textBox.textbox().attributes().multiline();
        attrs.m_KeepUpright = textBox.textbox().attributes().keep_upright();
        attrs.m_Size = UnpackVector2( textBox.textbox().attributes().size(), schIUScale );

        if( textBox.textbox().attributes().has_color() )
            attrs.m_Color = UnpackColor( textBox.textbox().attributes().color() );
        else
            attrs.m_Color = COLOR4D::UNSPECIFIED;

        if( !textBox.textbox().attributes().font_name().empty() )
        {
            attrs.m_Font = KIFONT::FONT::GetFont( wxString::FromUTF8( textBox.textbox().attributes().font_name() ),
                                                  attrs.m_Bold, attrs.m_Italic );
        }

        attrs.m_Angle = EDA_ANGLE( textBox.textbox().attributes().angle().value_degrees(), DEGREES_T );
        attrs.m_LineSpacing = textBox.textbox().attributes().line_spacing();
        attrs.m_StrokeWidth = UnpackDistance( textBox.textbox().attributes().stroke_width(), schIUScale );
        attrs.m_Halign = FromProtoEnum<GR_TEXT_H_ALIGN_T, types::HorizontalAlignment>(
                textBox.textbox().attributes().horizontal_alignment() );
        attrs.m_Valign = FromProtoEnum<GR_TEXT_V_ALIGN_T, types::VerticalAlignment>(
                textBox.textbox().attributes().vertical_alignment() );

        SetAttributes( attrs );
    }

    if( textBox.has_graphic_attributes() )
    {
        if( textBox.graphic_attributes().stroke().has_color() )
            m_stroke.SetColor( UnpackColor( textBox.graphic_attributes().stroke().color() ) );
        else
            m_stroke.SetColor( COLOR4D::UNSPECIFIED );

        if( textBox.graphic_attributes().fill().has_color() )
            SetFillColor( UnpackColor( textBox.graphic_attributes().fill().color() ) );
        else
            SetFillColor( COLOR4D::UNSPECIFIED );

        SetWidth( UnpackDistance( textBox.graphic_attributes().stroke().width(), schIUScale ) );
        SetLineStyle(
                FromProtoEnum<LINE_STYLE, types::StrokeLineStyle>( textBox.graphic_attributes().stroke().style() ) );
        SetFillMode( FromProtoEnum<FILL_T, types::GraphicFillType>( textBox.graphic_attributes().fill().fill_type() ) );
    }

    return true;
}


int SCH_TEXTBOX::GetLegacyTextMargin() const
{
    if( m_layer == LAYER_DEVICE )
        return KiROUND( GetTextSize().y * 0.8 );
    else
        return KiROUND( GetStroke().GetWidth() / 2.0 ) + KiROUND( GetTextSize().y * 0.75 );
}


VECTOR2I SCH_TEXTBOX::GetMinSize() const
{
    if( GetText().IsEmpty() )
        return VECTOR2I( 0, 0 );

    BOX2I textBox = GetTextBox( nullptr );

    int textHeight = std::abs( textBox.GetHeight() );

    if( GetTextAngle().IsVertical() )
    {
        textHeight += GetMarginLeft() + GetMarginRight();
        return VECTOR2I( textHeight, 0 );
    }

    textHeight += GetMarginTop() + GetMarginBottom();
    return VECTOR2I( 0, textHeight );
}


void SCH_TEXTBOX::MirrorHorizontally( int aCenter )
{
    SCH_SHAPE::MirrorHorizontally( aCenter );

    // Text is NOT really mirrored; it just has its justification flipped
    if( GetTextAngle() == ANGLE_HORIZONTAL )
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }
}


void SCH_TEXTBOX::MirrorVertically( int aCenter )
{
    SCH_SHAPE::MirrorVertically( aCenter );

    // Text is NOT really mirrored; it just has its justification flipped
    if( GetTextAngle() == ANGLE_VERTICAL )
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }
}


void SCH_TEXTBOX::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    SCH_SHAPE::Rotate( aCenter, aRotateCCW );
    SetTextAngle( GetTextAngle() == ANGLE_VERTICAL ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );
}


void SCH_TEXTBOX::Rotate90( bool aClockwise )
{
    SetTextAngle( GetTextAngle() == ANGLE_VERTICAL ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );
}


VECTOR2I SCH_TEXTBOX::GetDrawPos() const
{
    BOX2I bbox = BOX2I( m_start, m_end - m_start );

    bbox.Normalize();

    VECTOR2I pos( bbox.GetLeft() + m_marginLeft, bbox.GetBottom() - m_marginBottom );

    if( GetTextAngle().IsVertical() )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:          pos.y = bbox.GetBottom() - m_marginBottom;                         break;
        case GR_TEXT_H_ALIGN_CENTER:        pos.y = ( bbox.GetTop() + bbox.GetBottom() ) / 2;                  break;
        case GR_TEXT_H_ALIGN_RIGHT:         pos.y = bbox.GetTop() + m_marginTop;                               break;
        case GR_TEXT_H_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) ); break;
        }

        switch( GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:           pos.x = bbox.GetLeft() + m_marginLeft;                             break;
        case GR_TEXT_V_ALIGN_CENTER:        pos.x = ( bbox.GetLeft() + bbox.GetRight() ) / 2;                  break;
        case GR_TEXT_V_ALIGN_BOTTOM:        pos.x = bbox.GetRight() - m_marginRight;                           break;
        case GR_TEXT_V_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) ); break;
        }
    }
    else
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:          pos.x = bbox.GetLeft() + m_marginLeft;                             break;
        case GR_TEXT_H_ALIGN_CENTER:        pos.x = ( bbox.GetLeft() + bbox.GetRight() ) / 2;                  break;
        case GR_TEXT_H_ALIGN_RIGHT:         pos.x = bbox.GetRight() - m_marginRight;                           break;
        case GR_TEXT_H_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) ); break;
        }

        switch( GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:           pos.y = bbox.GetTop() + m_marginTop;                               break;
        case GR_TEXT_V_ALIGN_CENTER:        pos.y = ( bbox.GetTop() + bbox.GetBottom() ) / 2;                  break;
        case GR_TEXT_V_ALIGN_BOTTOM:        pos.y = bbox.GetBottom() - m_marginBottom;                         break;
        case GR_TEXT_V_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) ); break;
        }
    }

    return pos;
}


void SCH_TEXTBOX::swapData( SCH_ITEM* aItem )
{
    SCH_SHAPE::swapData( aItem );

    SCH_TEXTBOX* item = static_cast<SCH_TEXTBOX*>( aItem );

    std::swap( m_marginLeft, item->m_marginLeft );
    std::swap( m_marginTop, item->m_marginTop );
    std::swap( m_marginRight, item->m_marginRight );
    std::swap( m_marginBottom, item->m_marginBottom );

    SwapText( *item );
    SwapAttributes( *item );
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

    if( GetMarginLeft() != other->GetMarginLeft() )
        return GetMarginLeft() < other->GetMarginLeft();

    if( GetMarginTop() != other->GetMarginTop() )
        return GetMarginTop() < other->GetMarginTop();

    if( GetMarginRight() != other->GetMarginRight() )
        return GetMarginRight() < other->GetMarginRight();

    if( GetMarginBottom() != other->GetMarginBottom() )
        return GetMarginBottom() < other->GetMarginBottom();

    if( GetExcludedFromSim() != other->GetExcludedFromSim() )
        return GetExcludedFromSim() - other->GetExcludedFromSim();

    return GetText() < other->GetText();
}


KIFONT::FONT* SCH_TEXTBOX::GetDrawFont( const RENDER_SETTINGS* aSettings ) const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont( aSettings ), IsBold(), IsItalic() );

    return font;
}


wxString SCH_TEXTBOX::GetShownText( const RENDER_SETTINGS* aSettings, const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                                    int aDepth ) const
{
    // Use local depth counter so each text element starts fresh
    int depth = 0;

    SCH_SHEET* sheet = nullptr;

    if( aPath )
        sheet = aPath->Last();

    std::function<bool( wxString* )> textResolver = [&]( wxString* token ) -> bool
    {
        if( sheet )
        {
            if( sheet->ResolveTextVar( aPath, token, depth + 1 ) )
                return true;
        }

        return false;
    };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, depth );

    if( HasTextVars() )
        text = ResolveTextVars( text, &textResolver, depth );

    VECTOR2I size = GetEnd() - GetStart();
    int      colWidth;

    if( GetTextAngle().IsVertical() )
        colWidth = abs( size.y ) - ( GetMarginTop() + GetMarginBottom() );
    else
        colWidth = abs( size.x ) - ( GetMarginLeft() + GetMarginRight() );

    GetDrawFont( aSettings )
            ->LinebreakText( text, colWidth, GetTextSize(), GetEffectiveTextPenWidth(), IsBold(), IsItalic() );

    // Convert escape markers back to literals for final display
    text.Replace( wxT( "<<<ESC_DOLLAR:" ), wxT( "${" ) );
    text.Replace( wxT( "<<<ESC_AT:" ), wxT( "@{" ) );

    return text;
}


bool SCH_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_TEXTBOX::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


bool SCH_TEXTBOX::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    return KIGEOM::BoxHitTest( aPoly, GetBoundingBox(), aContained );
}


bool SCH_TEXTBOX::HasHypertext() const
{
    return HasHyperlink() || containsURL();
}


bool SCH_TEXTBOX::HasHoveredHypertext() const
{
    return !m_activeUrl.IsEmpty();
}


void SCH_TEXTBOX::DoHypertextAction( EDA_DRAW_FRAME* aFrame, const VECTOR2I& aMousePos ) const
{
    SCH_NAVIGATE_TOOL* navTool = aFrame->GetToolManager()->GetTool<SCH_NAVIGATE_TOOL>();

    if( HasHyperlink() )
        navTool->HypertextCommand( m_hyperlink );
    else if( !m_activeUrl.IsEmpty() )
        navTool->HypertextCommand( m_activeUrl );
}


wxString SCH_TEXTBOX::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Text box '%s'" ),
                             aFull ? GetShownText( false ) : KIUI::EllipsizeMenuText( GetText() ) );
}


BITMAPS SCH_TEXTBOX::GetMenuImage() const
{
    return BITMAPS::add_textbox;
}


void SCH_TEXTBOX::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts, int aUnit, int aBodyStyle,
                        const VECTOR2I& aOffset, bool aDimmed )
{
    if( IsPrivate() )
        return;

    SCH_SHAPE::Plot( aPlotter, aBackground, aPlotOpts, aUnit, aBodyStyle, aOffset, aDimmed );

    if( aBackground )
        return;

    SCH_SHEET_PATH*      sheet = Schematic() ? &Schematic()->CurrentSheet() : nullptr;
    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    int                  penWidth = GetEffectivePenWidth( renderSettings );
    COLOR4D              color = GetStroke().GetColor();
    COLOR4D              bg = renderSettings->GetBackgroundColor();

    KIFONT::FONT* font = GetDrawFont( renderSettings );

    color = GetTextColor();

    if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
        color = renderSettings->GetLayerColor( m_layer );

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( color.m_text && Schematic() )
        color = COLOR4D( ResolveText( *color.m_text, &Schematic()->CurrentSheet() ) );

    if( aDimmed )
    {
        color.Desaturate();
        color = color.Mix( bg, 0.5f );
    }

    penWidth = GetEffectiveTextPenWidth( renderSettings->GetDefaultPenWidth() );
    penWidth = std::max( penWidth, renderSettings->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    TEXT_ATTRIBUTES       attrs;
    std::vector<VECTOR2I> positions;
    wxArrayString         strings_list;

    wxStringSplit( GetShownText( renderSettings, sheet, true ), strings_list, '\n' );
    positions.reserve( strings_list.Count() );

    if( renderSettings->m_Transform != TRANSFORM() || aOffset != VECTOR2I() )
    {
        SCH_TEXTBOX temp( *this );

        if( renderSettings->m_Transform.y1 )
        {
            temp.SetTextAngle( temp.GetTextAngle() == ANGLE_HORIZONTAL ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
        }

        temp.SetStart( renderSettings->TransformCoordinate( m_start ) + aOffset );
        temp.SetEnd( renderSettings->TransformCoordinate( m_end ) + aOffset );

        attrs = temp.GetAttributes();
        temp.GetLinePositions( renderSettings, positions, (int) strings_list.Count() );
    }
    else
    {
        attrs = GetAttributes();
        GetLinePositions( renderSettings, positions, (int) strings_list.Count() );
    }

    attrs.m_StrokeWidth = penWidth;
    attrs.m_Multiline = false;

    for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
    {
        aPlotter->PlotText( positions[ii], color, strings_list.Item( ii ), attrs, font, GetFontMetrics() );
    }

    if( HasHyperlink() )
        aPlotter->HyperlinkBox( GetBoundingBox(), GetHyperlink() );
}


void SCH_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    SCH_ITEM::GetMsgPanelInfo( aFrame, aList );

    if( m_excludedFromSim )
        aList.emplace_back( _( "Exclude from" ), _( "Simulation" ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int      style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), aFrame->MessageTextFromValue( GetTextWidth() ) );

    aList.emplace_back( _( "Box Width" ), aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );

    aList.emplace_back( _( "Box Height" ), aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ) );

    m_stroke.GetMsgPanelInfo( aFrame, aList );
}


bool SCH_TEXTBOX::operator==( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return false;

    const SCH_TEXTBOX& other = static_cast<const SCH_TEXTBOX&>( aOther );

    if( m_excludedFromSim != other.m_excludedFromSim )
        return false;

    if( GetMarginLeft() != other.GetMarginLeft() )
        return false;

    if( GetMarginTop() != other.GetMarginTop() )
        return false;

    if( GetMarginRight() != other.GetMarginRight() )
        return false;

    if( GetMarginBottom() != other.GetMarginBottom() )
        return false;

    return SCH_SHAPE::operator==( aOther ) && EDA_TEXT::operator==( other );
}


double SCH_TEXTBOX::Similarity( const SCH_ITEM& aOther ) const
{
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( aOther.Type() != Type() )
        return 0.0;

    auto other = static_cast<const SCH_TEXTBOX&>( aOther );

    double similarity = SimilarityBase( other );

    if( m_excludedFromSim != other.m_excludedFromSim )
        similarity *= 0.9;

    if( GetMarginLeft() != other.GetMarginLeft() )
        similarity *= 0.9;

    if( GetMarginTop() != other.GetMarginTop() )
        similarity *= 0.9;

    if( GetMarginRight() != other.GetMarginRight() )
        similarity *= 0.9;

    if( GetMarginBottom() != other.GetMarginBottom() )
        similarity *= 0.9;

    similarity *= SCH_SHAPE::Similarity( aOther );
    similarity *= EDA_TEXT::Similarity( other );

    return similarity;
}


int SCH_TEXTBOX::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    wxASSERT( aOther.Type() == SCH_TEXTBOX_T );

    int retv = SCH_SHAPE::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const SCH_TEXTBOX* tmp = static_cast<const SCH_TEXTBOX*>( &aOther );

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
        return (int) GetHorizJustify() - (int) tmp->GetHorizJustify();

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

    return 0;
}


static struct SCH_TEXTBOX_DESC
{
    SCH_TEXTBOX_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_TEXTBOX );

        propMgr.AddTypeCast( new TYPE_CAST<SCH_TEXTBOX, SCH_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_TEXTBOX, EDA_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_TEXTBOX, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( SCH_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( EDA_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Shape" ) );
        propMgr.Mask( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Corner Radius" ) );

        propMgr.Mask( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );
        propMgr.Mask( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );

        const wxString marginProps = _( "Margins" );

        propMgr.AddProperty( new PROPERTY<SCH_TEXTBOX, int>( _HKI( "Margin Left" ), &SCH_TEXTBOX::SetMarginLeft,
                                                             &SCH_TEXTBOX::GetMarginLeft, PROPERTY_DISPLAY::PT_SIZE ),
                             marginProps );
        propMgr.AddProperty( new PROPERTY<SCH_TEXTBOX, int>( _HKI( "Margin Top" ), &SCH_TEXTBOX::SetMarginTop,
                                                             &SCH_TEXTBOX::GetMarginTop, PROPERTY_DISPLAY::PT_SIZE ),
                             marginProps );
        propMgr.AddProperty( new PROPERTY<SCH_TEXTBOX, int>( _HKI( "Margin Right" ), &SCH_TEXTBOX::SetMarginRight,
                                                             &SCH_TEXTBOX::GetMarginRight, PROPERTY_DISPLAY::PT_SIZE ),
                             marginProps );
        propMgr.AddProperty( new PROPERTY<SCH_TEXTBOX, int>( _HKI( "Margin Bottom" ), &SCH_TEXTBOX::SetMarginBottom,
                                                             &SCH_TEXTBOX::GetMarginBottom, PROPERTY_DISPLAY::PT_SIZE ),
                             marginProps );

        propMgr.AddProperty( new PROPERTY<SCH_TEXTBOX, int>( _HKI( "Text Size" ), &SCH_TEXTBOX::SetSchTextSize,
                                                             &SCH_TEXTBOX::GetSchTextSize, PROPERTY_DISPLAY::PT_SIZE ),
                             _HKI( "Text Properties" ) );

        propMgr.Mask( TYPE_HASH( SCH_TEXTBOX ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );
    }
} _SCH_TEXTBOX_DESC;
