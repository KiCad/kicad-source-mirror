/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <base_units.h>
#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <sch_plotter.h>
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
#include <tools/sch_navigate_tool.h>
#include <trigo.h>


SCH_TEXT::SCH_TEXT( const VECTOR2I& pos, const wxString& text, KICAD_T aType ) :
        SCH_ITEM( nullptr, aType ),
        EDA_TEXT( schIUScale, text )
{
    m_layer = LAYER_NOTES;

    SetTextPos( pos );
    SetMultilineAllowed( true );

    m_excludedFromSim = false;
}


SCH_TEXT::SCH_TEXT( const SCH_TEXT& aText ) :
        SCH_ITEM( aText ),
        EDA_TEXT( aText )
{
    m_excludedFromSim = aText.m_excludedFromSim;
}


VECTOR2I SCH_TEXT::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    // Fudge factor to match KiCad 6
    return VECTOR2I( 0, -2500 );
}


void SCH_TEXT::MirrorHorizontally( int aCenter )
{
    if( GetTextAngle() == ANGLE_HORIZONTAL )
        FlipHJustify();

    SetTextX( MIRRORVAL( GetTextPos().x, aCenter ) );
}


void SCH_TEXT::MirrorVertically( int aCenter )
{
    if( GetTextAngle() == ANGLE_VERTICAL )
        FlipHJustify();

    SetTextY( MIRRORVAL( GetTextPos().y, aCenter ) );
}


void SCH_TEXT::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aCenter, aRotateCCW ? ANGLE_270 : ANGLE_90 );
    VECTOR2I offset = pt - GetTextPos();

    Rotate90( false );

    SetTextPos( GetTextPos() + offset );
}


void SCH_TEXT::Rotate90( bool aClockwise )
{
    if( ( GetTextAngle() == ANGLE_HORIZONTAL && aClockwise )
     || ( GetTextAngle() == ANGLE_VERTICAL && !aClockwise ) )
    {
        FlipHJustify();
    }

    SetTextAngle( GetTextAngle() == ANGLE_VERTICAL ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );
}


void SCH_TEXT::MirrorSpinStyle( bool aLeftRight )
{
    if( ( GetTextAngle() == ANGLE_HORIZONTAL && aLeftRight )
     || ( GetTextAngle() == ANGLE_VERTICAL && !aLeftRight ) )
    {
        FlipHJustify();
    }
}


void SCH_TEXT::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    SCH_TEXT* item = static_cast<SCH_TEXT*>( aItem );

    std::swap( m_layer, item->m_layer );

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

    if( GetExcludedFromSim() != other->GetExcludedFromSim() )
        return GetExcludedFromSim() - other->GetExcludedFromSim();

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


KIFONT::FONT* SCH_TEXT::getDrawFont() const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    return font;
}


void SCH_TEXT::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                      const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    COLOR4D  color = GetTextColor();
    bool     blackAndWhiteMode = GetGRForceBlackPenState();
    VECTOR2I text_offset = aOffset + GetSchematicTextOffset( aSettings );

    if( blackAndWhiteMode || color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( m_layer );

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), IsBold(), IsItalic() );

    // Adjust text drawn in an outline font to more closely mimic the positioning of
    // SCH_FIELD text.
    if( font->IsOutline() )
    {
        BOX2I    firstLineBBox = GetTextBox( 0 );
        int      sizeDiff = firstLineBBox.GetHeight() - GetTextSize().y;
        int      adjust = KiROUND( sizeDiff * 0.4 );
        VECTOR2I adjust_offset( 0, - adjust );

        RotatePoint( adjust_offset, GetDrawRotation() );
        text_offset += adjust_offset;
    }

    EDA_TEXT::Print( aSettings, text_offset, color );
}


const BOX2I SCH_TEXT::GetBoundingBox() const
{
    BOX2I bbox = GetTextBox();

    if( !GetTextAngle().IsZero() ) // Rotate bbox.
    {
        VECTOR2I pos = bbox.GetOrigin();
        VECTOR2I end = bbox.GetEnd();

        RotatePoint( pos, GetTextPos(), GetTextAngle() );
        RotatePoint( end, GetTextPos(), GetTextAngle() );

        bbox.SetOrigin( pos );
        bbox.SetEnd( end );
    }

    bbox.Normalize();
    return bbox;
}


wxString SCH_TEXT::GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                                 int aDepth ) const
{
    SCH_SHEET* sheet = nullptr;

    if( aPath )
        sheet = aPath->Last();
    else if( SCHEMATIC* schematic = Schematic() )
        sheet = schematic->CurrentSheet().Last();

    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                if( sheet )
                {
                    if( sheet->ResolveTextVar( aPath, token, aDepth + 1 ) )
                        return true;
                }

                return false;
            };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( text == wxS( "~" ) ) // Legacy placeholder for empty string
    {
        text = wxS( "" );
    }
    else if( HasTextVars() )
    {
        if( aDepth < 10 )
            text = ExpandTextVars( text, &textResolver );
    }

    return text;
}


void SCH_TEXT::DoHypertextAction( EDA_DRAW_FRAME* aFrame ) const
{
    wxCHECK_MSG( IsHypertext(), /* void */,
                 wxT( "Calling a hypertext menu on a SCH_TEXT with no hyperlink?" ) );

    SCH_NAVIGATE_TOOL* navTool = aFrame->GetToolManager()->GetTool<SCH_NAVIGATE_TOOL>();
    navTool->HypertextCommand( m_hyperlink );
}


wxString SCH_TEXT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Graphic Text '%s'" ), KIUI::EllipsizeMenuText( GetText() ) );
}


BITMAPS SCH_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


bool SCH_TEXT::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );
    return bBox.Contains( aPosition );
}


bool SCH_TEXT::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I bBox = GetBoundingBox();
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


void SCH_TEXT::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                     int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground )
        return;

    SCH_SHEET_PATH*  sheet = &Schematic()->CurrentSheet();
    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    SCH_CONNECTION*  connection = Connection();
    int              layer = ( connection && connection->IsBus() ) ? LAYER_BUS : m_layer;
    COLOR4D          color = GetTextColor();
    int              penWidth = GetEffectiveTextPenWidth( settings->GetDefaultPenWidth() );
    VECTOR2I         text_offset = GetSchematicTextOffset( aPlotter->RenderSettings() );

    if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
        color = settings->GetLayerColor( layer );

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( settings->GetDefaultFont(), IsBold(), IsItalic() );

    // Adjust text drawn in an outline font to more closely mimic the positioning of
    // SCH_FIELD text.
    if( font->IsOutline() )
    {
        BOX2I    firstLineBBox = GetTextBox( 0 );
        int      sizeDiff = firstLineBBox.GetHeight() - GetTextSize().y;
        int      adjust = KiROUND( sizeDiff * 0.4 );
        VECTOR2I adjust_offset( 0, - adjust );

        RotatePoint( adjust_offset, GetDrawRotation() );
        text_offset += adjust_offset;
    }

    std::vector<VECTOR2I> positions;
    wxArrayString strings_list;
    wxStringSplit( GetShownText( sheet, true ), strings_list, '\n' );
    positions.reserve( strings_list.Count() );

    GetLinePositions( positions, (int) strings_list.Count() );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_StrokeWidth = penWidth;
    attrs.m_Multiline = false;

    for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
    {
        VECTOR2I  textpos = positions[ii] + text_offset;
        wxString& txt = strings_list.Item( ii );
        aPlotter->PlotText( textpos, color, txt, attrs, font, GetFontMetrics() );
    }

    if( HasHyperlink() )
        aPlotter->HyperlinkBox( GetBoundingBox(), GetHyperlink() );
}


void SCH_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Graphic Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    if( m_excludedFromSim )
        aList.emplace_back( _( "Exclude from" ), _( "Simulation" ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), aFrame->MessageTextFromValue( GetTextWidth() ) );

    switch( GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:          msg = _( "Align left" );   break;
    case GR_TEXT_H_ALIGN_CENTER:        msg = _( "Align center" ); break;
    case GR_TEXT_H_ALIGN_RIGHT:         msg = _( "Align right" );  break;
    case GR_TEXT_H_ALIGN_INDETERMINATE: msg = INDETERMINATE_STATE; break;
    }

    aList.emplace_back( _( "Justification" ), msg );
}

bool SCH_TEXT::operator==( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return false;

    const SCH_TEXT* other = static_cast<const SCH_TEXT*>( &aOther );

    if( GetLayer() != other->GetLayer() )
        return false;

    if( GetExcludedFromSim() != other->GetExcludedFromSim() )
        return false;

    return EDA_TEXT::operator==( *other );
}


double SCH_TEXT::Similarity( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return 0.0;

    const SCH_TEXT* other = static_cast<const SCH_TEXT*>( &aOther );

    double retval = 1.0;

    if( GetLayer() != other->GetLayer() )
        retval *= 0.9;

    if( GetExcludedFromSim() != other->GetExcludedFromSim() )
        retval *= 0.9;

    retval *= EDA_TEXT::Similarity( *other );

    return retval;
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


static struct SCH_TEXT_DESC
{
    SCH_TEXT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_TEXT );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_TEXT, SCH_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_TEXT, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TEXT ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TEXT ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( SCH_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Mirrored" ) );
        propMgr.Mask( TYPE_HASH( SCH_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Visible" ) );
        propMgr.Mask( TYPE_HASH( SCH_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );
        propMgr.Mask( TYPE_HASH( SCH_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );

        propMgr.AddProperty( new PROPERTY<SCH_TEXT, int>( _HKI( "Text Size" ),
                &SCH_TEXT::SetSchTextSize, &SCH_TEXT::GetSchTextSize, PROPERTY_DISPLAY::PT_SIZE ),
                _HKI( "Text Properties" ) );

        // Orientation is exposed differently in schematic; mask the base for now
        propMgr.Mask( TYPE_HASH( SCH_TEXT ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );
    }
} _SCH_TEXT_DESC;
