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


SCH_TEXT::SCH_TEXT( const VECTOR2I& aPos, const wxString& aText, SCH_LAYER_ID aLayer,
                    KICAD_T aType ) :
        SCH_ITEM( nullptr, aType ),
        EDA_TEXT( schIUScale, aText )
{
    m_layer = aLayer;

    SetTextPos( aPos );
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


void SCH_TEXT::NormalizeJustification( bool inverse )
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


void SCH_TEXT::MirrorHorizontally( int aCenter )
{
    if( m_layer == LAYER_DEVICE )
    {
        NormalizeJustification( false );
        int x = GetTextPos().x;

        x -= aCenter;
        x *= -1;
        x += aCenter;

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
    else
    {
        if( GetTextAngle() == ANGLE_HORIZONTAL )
            FlipHJustify();

        SetTextX( MIRRORVAL( GetTextPos().x, aCenter ) );
    }
}


void SCH_TEXT::MirrorVertically( int aCenter )
{
    if( m_layer == LAYER_DEVICE )
    {
        NormalizeJustification( false );
        int y = GetTextPos().y;

        y -= aCenter;
        y *= -1;
        y += aCenter;

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
    else
    {
        if( GetTextAngle() == ANGLE_VERTICAL )
            FlipHJustify();

        SetTextY( MIRRORVAL( GetTextPos().y, aCenter ) );
    }
}


void SCH_TEXT::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );
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
    COLOR4D color = GetTextColor();
    COLOR4D bg = aSettings->GetBackgroundColor();
    bool    blackAndWhiteMode = GetGRForceBlackPenState();

    if( blackAndWhiteMode || color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( m_layer );

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( !IsVisible() )
        bg = aSettings->GetLayerColor( LAYER_HIDDEN );

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), IsBold(), IsItalic() );

    if( m_layer == LAYER_DEVICE )
    {
        wxDC* DC = aSettings->GetPrintDC();
        int   penWidth = std::max( GetEffectiveTextPenWidth(), aSettings->GetDefaultPenWidth() );

        // Calculate the text orientation, according to the symbol orientation/mirror (needed when
        // draw text in schematic)
        EDA_ANGLE orient = GetTextAngle();

        if( aSettings->m_Transform.y1 )  // Rotate symbol 90 degrees.
        {
            if( orient == ANGLE_HORIZONTAL )
                orient = ANGLE_VERTICAL;
            else
                orient = ANGLE_HORIZONTAL;
        }

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
        BOX2I    bBox = GetBoundingBox();
        VECTOR2I txtpos = bBox.Centre();

        // Calculate pos according to mirror/rotation.
        txtpos = aSettings->m_Transform.TransformCoordinate( txtpos ) + aOffset;

        GRPrintText( DC, txtpos, color, GetShownText( true ), orient, GetTextSize(),
                     GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_CENTER, penWidth, IsItalic(),
                     IsBold(), font, GetFontMetrics() );
    }
    else
    {
        VECTOR2I text_offset = aOffset + GetSchematicTextOffset( aSettings );

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
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    BOX2I rect = aRect;
    BOX2I bBox = GetBoundingBox();

    rect.Inflate( aAccuracy );

    if( aContained )
        return aRect.Contains( bBox );

    return aRect.Intersects( bBox );
}


void SCH_TEXT::BeginEdit( const VECTOR2I& aPosition )
{
    SetTextPos( aPosition );
}


void SCH_TEXT::CalcEdit( const VECTOR2I& aPosition )
{
    SetTextPos( aPosition );
}


void SCH_TEXT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = IsPrivate() ? LAYER_PRIVATE_NOTES : m_layer;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


void SCH_TEXT::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                     int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground || IsPrivate() )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    COLOR4D              color = GetTextColor();
    COLOR4D              bg = renderSettings->GetBackgroundColor();

    if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
    {
        SCH_CONNECTION* connection = Connection();

        if( connection && connection->IsBus() )
            color = renderSettings->GetLayerColor( LAYER_BUS );
        else
            color = renderSettings->GetLayerColor( m_layer );
    }

    if( !IsVisible() )
        bg = renderSettings->GetLayerColor( LAYER_HIDDEN );
    else if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    int penWidth = GetEffectiveTextPenWidth( renderSettings->GetDefaultPenWidth() );
    penWidth = std::max( penWidth, renderSettings->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( renderSettings->GetDefaultFont(), IsBold(), IsItalic() );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_StrokeWidth = penWidth;

    if( m_layer == LAYER_DEVICE )
    {
        BOX2I bBox = GetBoundingBox();

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
        attrs.m_Halign = GR_TEXT_H_ALIGN_CENTER;
        attrs.m_Valign = GR_TEXT_V_ALIGN_CENTER;

        // The text orientation may need to be flipped if the transformation matrix causes xy
        // axes to be flipped.
        if( ( renderSettings->m_Transform.x1 != 0 ) ^ ( GetTextAngle() != ANGLE_HORIZONTAL ) )
            attrs.m_Angle = ANGLE_HORIZONTAL;
        else
            attrs.m_Angle = ANGLE_VERTICAL;

        aPlotter->PlotText( renderSettings->TransformCoordinate( txtpos ) + aOffset, color,
                            GetText(), attrs, font, GetFontMetrics() );
    }
    else
    {
        SCH_SHEET_PATH* sheet = &Schematic()->CurrentSheet();
        VECTOR2I        text_offset = GetSchematicTextOffset( aPlotter->RenderSettings() );

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
}


void SCH_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

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

    if( m_layer == LAYER_DEVICE )
    {
        aList.emplace_back( _( "H Justification" ), msg );

        switch ( GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:           msg = _( "Top" );          break;
        case GR_TEXT_V_ALIGN_CENTER:        msg = _( "Center" );       break;
        case GR_TEXT_V_ALIGN_BOTTOM:        msg = _( "Bottom" );       break;
        case GR_TEXT_V_ALIGN_INDETERMINATE: msg = INDETERMINATE_STATE; break;
        }

        aList.emplace_back( _( "V Justification" ), msg );
    }
    else
    {
        aList.emplace_back( _( "Justification" ), msg );
    }
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
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( Type() != aOther.Type() )
        return 0.0;

    const SCH_TEXT* other = static_cast<const SCH_TEXT*>( &aOther );

    double retval = SimilarityBase( aOther );

    if( GetLayer() != other->GetLayer() )
        retval *= 0.9;

    if( GetExcludedFromSim() != other->GetExcludedFromSim() )
        retval *= 0.9;

    retval *= EDA_TEXT::Similarity( *other );

    return retval;
}


int SCH_TEXT::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    wxASSERT( aOther.Type() == SCH_TEXT_T );

    int retv = SCH_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const SCH_TEXT& tmp = static_cast<const SCH_TEXT&>( aOther );

    int result = GetText().CmpNoCase( tmp.GetText() );

    if( result != 0 )
        return result;

    if( GetTextPos().x != tmp.GetTextPos().x )
        return GetTextPos().x - tmp.GetTextPos().x;

    if( GetTextPos().y != tmp.GetTextPos().y )
        return GetTextPos().y - tmp.GetTextPos().y;

    if( GetTextWidth() != tmp.GetTextWidth() )
        return GetTextWidth() - tmp.GetTextWidth();

    if( GetTextHeight() != tmp.GetTextHeight() )
        return GetTextHeight() - tmp.GetTextHeight();

    return 0;
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
