/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_text.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <sch_painter.h>
#include <default_values.h>
#include <wx/debug.h>
#include <wx/log.h>
#include <dialogs/html_message_box.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <core/mirror.h>
#include <core/kicad_algo.h>
#include <trigo.h>

using KIGFX::SCH_RENDER_SETTINGS;


TEXT_SPIN_STYLE TEXT_SPIN_STYLE::RotateCW()
{
    SPIN newSpin = m_spin;

    switch( m_spin )
    {
    case TEXT_SPIN_STYLE::LEFT:   newSpin = TEXT_SPIN_STYLE::UP;     break;
    case TEXT_SPIN_STYLE::UP:     newSpin = TEXT_SPIN_STYLE::RIGHT;  break;
    case TEXT_SPIN_STYLE::RIGHT:  newSpin = TEXT_SPIN_STYLE::BOTTOM; break;
    case TEXT_SPIN_STYLE::BOTTOM: newSpin = TEXT_SPIN_STYLE::LEFT;   break;
    }

    return TEXT_SPIN_STYLE( newSpin );
}


TEXT_SPIN_STYLE TEXT_SPIN_STYLE::RotateCCW()
{
    SPIN newSpin = m_spin;

    switch( m_spin )
    {
    case TEXT_SPIN_STYLE::LEFT:   newSpin = TEXT_SPIN_STYLE::BOTTOM; break;
    case TEXT_SPIN_STYLE::BOTTOM: newSpin = TEXT_SPIN_STYLE::RIGHT;  break;
    case TEXT_SPIN_STYLE::RIGHT:  newSpin = TEXT_SPIN_STYLE::UP;     break;
    case TEXT_SPIN_STYLE::UP:     newSpin = TEXT_SPIN_STYLE::LEFT;   break;
    }

    return TEXT_SPIN_STYLE( newSpin );
}


TEXT_SPIN_STYLE TEXT_SPIN_STYLE::MirrorX()
{
    SPIN newSpin = m_spin;

    switch( m_spin )
    {
    case TEXT_SPIN_STYLE::UP:     newSpin = TEXT_SPIN_STYLE::BOTTOM; break;
    case TEXT_SPIN_STYLE::BOTTOM: newSpin = TEXT_SPIN_STYLE::UP;     break;
    case TEXT_SPIN_STYLE::LEFT:                                      break;
    case TEXT_SPIN_STYLE::RIGHT:                                     break;
    }

    return TEXT_SPIN_STYLE( newSpin );
}


TEXT_SPIN_STYLE TEXT_SPIN_STYLE::MirrorY()
{
    SPIN newSpin = m_spin;

    switch( m_spin )
    {
    case TEXT_SPIN_STYLE::LEFT:   newSpin = TEXT_SPIN_STYLE::RIGHT; break;
    case TEXT_SPIN_STYLE::RIGHT:  newSpin = TEXT_SPIN_STYLE::LEFT;  break;
    case TEXT_SPIN_STYLE::UP:                                       break;
    case TEXT_SPIN_STYLE::BOTTOM:                                   break;
    }

    return TEXT_SPIN_STYLE( newSpin );
}


SCH_TEXT::SCH_TEXT( const VECTOR2I& pos, const wxString& text, KICAD_T aType ) :
        SCH_ITEM( nullptr, aType ),
        EDA_TEXT( text )
{
    m_layer = LAYER_NOTES;

    SetTextPos( pos );
    SetTextSpinStyle( TEXT_SPIN_STYLE::LEFT );
    SetMultilineAllowed( true );
}


SCH_TEXT::SCH_TEXT( const SCH_TEXT& aText ) :
        SCH_ITEM( aText ),
        EDA_TEXT( aText ),
        m_spin_style( aText.m_spin_style )
{ }


VECTOR2I SCH_TEXT::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    return VECTOR2I( 0, 0 );
}


void SCH_TEXT::MirrorHorizontally( int aCenter )
{
    // Text is NOT really mirrored; it is moved to a suitable horizontal position
    SetTextSpinStyle( GetTextSpinStyle().MirrorY() );

    SetTextX( MIRRORVAL( GetTextPos().x, aCenter ) );
}


void SCH_TEXT::MirrorVertically( int aCenter )
{
    // Text is NOT really mirrored; it is moved to a suitable vertical position
    SetTextSpinStyle( GetTextSpinStyle().MirrorX() );

    SetTextY( MIRRORVAL( GetTextPos().y, aCenter ) );
}


void SCH_TEXT::Rotate( const VECTOR2I& aCenter )
{
    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aCenter, ANGLE_90 );
    VECTOR2I offset = pt - GetTextPos();

    Rotate90( false );

    SetTextPos( GetTextPos() + offset );
}


void SCH_TEXT::Rotate90( bool aClockwise )
{
    if( aClockwise )
        SetTextSpinStyle( GetTextSpinStyle().RotateCW() );
    else
        SetTextSpinStyle( GetTextSpinStyle().RotateCCW() );
}


void SCH_TEXT::MirrorSpinStyle( bool aLeftRight )
{
    if( aLeftRight )
        SetTextSpinStyle( GetTextSpinStyle().MirrorY() );
    else
        SetTextSpinStyle( GetTextSpinStyle().MirrorX() );
}


void SCH_TEXT::SetTextSpinStyle( TEXT_SPIN_STYLE aSpinStyle )
{
    m_spin_style = aSpinStyle;

    // Assume "Right" and Left" mean which side of the anchor the text will be on
    // Thus we want to left justify text up against the anchor if we are on the right
    switch( aSpinStyle )
    {
    default:
        wxFAIL_MSG( "Bad spin style" );
        m_spin_style = TEXT_SPIN_STYLE::RIGHT; // Handle the error spin style by resetting
        KI_FALLTHROUGH;

    case TEXT_SPIN_STYLE::RIGHT:            // Horiz Normal Orientation
        SetTextAngle( ANGLE_HORIZONTAL );
        SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case TEXT_SPIN_STYLE::UP:               // Vert Orientation UP
        SetTextAngle( ANGLE_VERTICAL );
        SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case TEXT_SPIN_STYLE::LEFT:             // Horiz Orientation - Right justified
        SetTextAngle( ANGLE_HORIZONTAL );
        SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    case TEXT_SPIN_STYLE::BOTTOM:           //  Vert Orientation BOTTOM
        SetTextAngle( ANGLE_VERTICAL );
        SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    }

    SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
}


void SCH_TEXT::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXT* item = static_cast<SCH_TEXT*>( aItem );

    std::swap( m_layer, item->m_layer );
    std::swap( m_spin_style, item->m_spin_style );

    SwapText( *item );
    SwapAttributes( *item );
}


bool SCH_TEXT::operator<( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto other = static_cast<const SCH_TEXT*>( &aItem );

    if( GetLayer() != other->GetLayer() )
            return GetLayer() < other->GetLayer();

    if( GetPosition().x != other->GetPosition().x )
        return GetPosition().x < other->GetPosition().x;

    if( GetPosition().y != other->GetPosition().y )
        return GetPosition().y < other->GetPosition().y;

    return GetText() < other->GetText();
}


int SCH_TEXT::GetTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    double ratio;

    if( aSettings )
        ratio = static_cast<const SCH_RENDER_SETTINGS*>( aSettings )->m_TextOffsetRatio;
    else if( Schematic() )
        ratio = Schematic()->Settings().m_TextOffsetRatio;
    else
        ratio = DEFAULT_TEXT_OFFSET_RATIO;   // For previews (such as in Preferences), etc.

    return KiROUND( ratio * GetTextSize().y );
}


int SCH_TEXT::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


KIFONT::FONT* SCH_TEXT::GetDrawFont() const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    return font;
}


void SCH_TEXT::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    COLOR4D color = aSettings->GetLayerColor( m_layer );
    VECTOR2I text_offset = aOffset + GetSchematicTextOffset( aSettings );

    EDA_TEXT::Print( aSettings, text_offset, color );
}


const EDA_RECT SCH_TEXT::GetBoundingBox() const
{
    EDA_RECT rect = GetTextBox();

    if( !GetTextAngle().IsZero() ) // Rotate rect.
    {
        VECTOR2I pos = rect.GetOrigin();
        VECTOR2I end = rect.GetEnd();

        RotatePoint( pos, GetTextPos(), GetTextAngle() );
        RotatePoint( end, GetTextPos(), GetTextAngle() );

        rect.SetOrigin( pos );
        rect.SetEnd( end );
    }

    rect.Normalize();
    return rect;
}


wxString SCH_TEXT::GetShownText( int aDepth ) const
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

    if( text == "~" )   // Legacy placeholder for empty string
    {
        text = "";
    }
    else if( HasTextVars() )
    {
        PROJECT* project = nullptr;

        if( Schematic() )
            project = &Schematic()->Prj();

        if( aDepth < 10 )
            text = ExpandTextVars( text, &textResolver, &schematicTextResolver, project );
    }

    return text;
}


wxString SCH_TEXT::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Graphic Text '%s'" ), ShortenedShownText() );
}


BITMAPS SCH_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


bool SCH_TEXT::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );
    return bBox.Contains( aPosition );
}


bool SCH_TEXT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );

    if( aContained )
        return aRect.Contains( bBox );

    return aRect.Intersects( bBox );
}


void SCH_TEXT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = m_layer;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


void SCH_TEXT::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    if( aBackground )
        return;

    static std::vector<VECTOR2I> s_poly;

    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    SCH_CONNECTION*  connection = Connection();
    int              layer = ( connection && connection->IsBus() ) ? LAYER_BUS : m_layer;
    COLOR4D          color = settings->GetLayerColor( layer );
    int              penWidth = GetEffectiveTextPenWidth( settings->GetDefaultPenWidth() );
    KIFONT::FONT*    font = GetDrawFont();

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    std::vector<VECTOR2I> positions;
    wxArrayString strings_list;
    wxStringSplit( GetShownText(), strings_list, '\n' );
    positions.reserve( strings_list.Count() );

    GetLinePositions( positions, (int) strings_list.Count() );

    for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
    {
        VECTOR2I  textpos = positions[ii] + GetSchematicTextOffset( aPlotter->RenderSettings() );
        wxString& txt = strings_list.Item( ii );
        aPlotter->Text( textpos, color, txt, GetTextAngle(), GetTextSize(), GetHorizJustify(),
                        GetVertJustify(), penWidth, IsItalic(), IsBold(), false, font );
    }
}


void SCH_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Graphic Text" ), UnescapeString( ShortenedText() ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), MessageTextFromValue( aFrame->GetUserUnits(),
                                                                GetTextWidth() ) );

    switch( GetTextSpinStyle() )
    {
    case TEXT_SPIN_STYLE::LEFT:   msg = _( "Align right" );   break;
    case TEXT_SPIN_STYLE::UP:     msg = _( "Align bottom" );  break;
    case TEXT_SPIN_STYLE::RIGHT:  msg = _( "Align left" );    break;
    case TEXT_SPIN_STYLE::BOTTOM: msg = _( "Align top" );     break;
    default:                      msg = wxT( "???" );         break;
    }

    aList.emplace_back( _( "Justification" ), msg );
}


#if defined(DEBUG)

void SCH_TEXT::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str()
                                 << " layer=\"" << m_layer << '"'
                                 << '>'
                                 << TO_UTF8( GetText() )
                                 << "</" << s.Lower().mb_str() << ">\n";
}

#endif


