/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2023 KiCad Developers, see change_log.txt for contributors.
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

#include <algorithm>          // for max
#include <stddef.h>           // for NULL
#include <type_traits>        // for swap
#include <vector>

#include <eda_item.h>
#include <base_units.h>
#include <callback_gal.h>
#include <eda_text.h>         // for EDA_TEXT, TEXT_EFFECTS, GR_TEXT_VJUSTIF...
#include <gal/color4d.h>      // for COLOR4D, COLOR4D::BLACK
#include <font/glyph.h>
#include <gr_text.h>
#include <string_utils.h>     // for UnescapeString
#include <math/util.h>        // for KiROUND
#include <math/vector2d.h>
#include <core/kicad_algo.h>
#include <richio.h>
#include <render_settings.h>
#include <trigo.h>            // for RotatePoint
#include <i18n_utility.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_simple.h>
#include <font/outline_font.h>
#include <geometry/shape_poly_set.h>
#include <properties/property_validators.h>
#include <ctl_flags.h>         // for CTL_OMIT_HIDE definition

#include <wx/debug.h>           // for wxASSERT
#include <wx/string.h>
#include <wx/url.h>             // for wxURL
#include "font/kicad_font_name.h"
#include "font/fontconfig.h"
#include "pgm_base.h"

class OUTPUTFORMATTER;
class wxFindReplaceData;


GR_TEXT_H_ALIGN_T EDA_TEXT::MapHorizJustify( int aHorizJustify )
{
    wxASSERT( aHorizJustify >= GR_TEXT_H_ALIGN_LEFT && aHorizJustify <= GR_TEXT_H_ALIGN_RIGHT );

    if( aHorizJustify > GR_TEXT_H_ALIGN_RIGHT )
        return GR_TEXT_H_ALIGN_RIGHT;

    if( aHorizJustify < GR_TEXT_H_ALIGN_LEFT )
        return GR_TEXT_H_ALIGN_LEFT;

    return static_cast<GR_TEXT_H_ALIGN_T>( aHorizJustify );
}


GR_TEXT_V_ALIGN_T EDA_TEXT::MapVertJustify( int aVertJustify )
{
    wxASSERT( aVertJustify >= GR_TEXT_V_ALIGN_TOP && aVertJustify <= GR_TEXT_V_ALIGN_BOTTOM );

    if( aVertJustify > GR_TEXT_V_ALIGN_BOTTOM )
        return GR_TEXT_V_ALIGN_BOTTOM;

    if( aVertJustify < GR_TEXT_V_ALIGN_TOP )
        return GR_TEXT_V_ALIGN_TOP;

    return static_cast<GR_TEXT_V_ALIGN_T>( aVertJustify );
}


EDA_TEXT::EDA_TEXT( const EDA_IU_SCALE& aIuScale, const wxString& aText ) :
        m_text( aText ),
        m_IuScale( aIuScale ),
        m_render_cache_font( nullptr ),
        m_bounding_box_cache_valid( false ),
        m_bounding_box_cache_line( -1 ),
        m_bounding_box_cache_inverted( false )
{
    SetTextSize( VECTOR2I( EDA_UNIT_UTILS::Mils2IU( m_IuScale, DEFAULT_SIZE_TEXT ),
                           EDA_UNIT_UTILS::Mils2IU( m_IuScale, DEFAULT_SIZE_TEXT ) ) );

    if( m_text.IsEmpty() )
    {
        m_shown_text = wxEmptyString;
        m_shown_text_has_text_var_refs = false;
    }
    else
    {
        m_shown_text = UnescapeString( m_text );
        m_shown_text_has_text_var_refs = m_shown_text.Contains( wxT( "${" ) );
    }
}


EDA_TEXT::EDA_TEXT( const EDA_TEXT& aText ) :
    m_IuScale( aText.m_IuScale )
{
    m_text = aText.m_text;
    m_shown_text = aText.m_shown_text;
    m_shown_text_has_text_var_refs = aText.m_shown_text_has_text_var_refs;

    m_attributes = aText.m_attributes;
    m_pos = aText.m_pos;

    m_render_cache_font = aText.m_render_cache_font;
    m_render_cache_text = aText.m_render_cache_text;
    m_render_cache_angle = aText.m_render_cache_angle;
    m_render_cache_offset = aText.m_render_cache_offset;

    m_render_cache.clear();

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aText.m_render_cache )
    {
        if( KIFONT::OUTLINE_GLYPH* outline = dynamic_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() ) )
            m_render_cache.emplace_back( std::make_unique<KIFONT::OUTLINE_GLYPH>( *outline ) );
        else if( KIFONT::STROKE_GLYPH* stroke = dynamic_cast<KIFONT::STROKE_GLYPH*>( glyph.get() ) )
            m_render_cache.emplace_back( std::make_unique<KIFONT::STROKE_GLYPH>( *stroke ) );
    }

    m_bounding_box_cache_valid = aText.m_bounding_box_cache_valid;
    m_bounding_box_cache = aText.m_bounding_box_cache;
    m_bounding_box_cache_line = aText.m_bounding_box_cache_line;
    m_bounding_box_cache_inverted = aText.m_bounding_box_cache_inverted;
}


EDA_TEXT::~EDA_TEXT()
{
}


EDA_TEXT& EDA_TEXT::operator=( const EDA_TEXT& aText )
{
    m_text = aText.m_text;
    m_shown_text = aText.m_shown_text;
    m_shown_text_has_text_var_refs = aText.m_shown_text_has_text_var_refs;

    m_attributes = aText.m_attributes;
    m_pos = aText.m_pos;

    m_render_cache_font = aText.m_render_cache_font;
    m_render_cache_text = aText.m_render_cache_text;
    m_render_cache_angle = aText.m_render_cache_angle;
    m_render_cache_offset = aText.m_render_cache_offset;

    m_render_cache.clear();

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aText.m_render_cache )
    {
        if( KIFONT::OUTLINE_GLYPH* outline = dynamic_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() ) )
            m_render_cache.emplace_back( std::make_unique<KIFONT::OUTLINE_GLYPH>( *outline ) );
        else if( KIFONT::STROKE_GLYPH* stroke = dynamic_cast<KIFONT::STROKE_GLYPH*>( glyph.get() ) )
            m_render_cache.emplace_back( std::make_unique<KIFONT::STROKE_GLYPH>( *stroke ) );
    }

    m_bounding_box_cache_valid = aText.m_bounding_box_cache_valid;
    m_bounding_box_cache = aText.m_bounding_box_cache;

    return *this;
}


void EDA_TEXT::SetText( const wxString& aText )
{
    m_text = aText;
    cacheShownText();
}


void EDA_TEXT::CopyText( const EDA_TEXT& aSrc )
{
    m_text = aSrc.m_text;
    cacheShownText();
}


void EDA_TEXT::SetTextThickness( int aWidth )
{
    m_attributes.m_StrokeWidth = aWidth;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetTextAngle( const EDA_ANGLE& aAngle )
{
    m_attributes.m_Angle = aAngle;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetItalic( bool aItalic )
{
    if( m_attributes.m_Italic != aItalic )
    {
        const KIFONT::FONT* font = GetFont();

        if( !font || font->IsStroke() )
        {
            // For stroke fonts, just need to set the attribute.
        }
        else
        {
            // For outline fonts, italic-ness is determined by the font itself.
            SetFont( KIFONT::FONT::GetFont( font->GetName(), IsBold(), aItalic ) );
        }
    }

    SetItalicFlag( aItalic );
}

void EDA_TEXT::SetItalicFlag( bool aItalic )
{
    m_attributes.m_Italic = aItalic;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetBold( bool aBold )
{
    if( m_attributes.m_Bold != aBold )
    {
        const KIFONT::FONT* font = GetFont();

        if( !font || font->IsStroke() )
        {
            // For stroke fonts, boldness is determined by the pen size.
            const int size = std::min( m_attributes.m_Size.x, m_attributes.m_Size.y );

            if( aBold )
            {
                m_attributes.m_StoredStrokeWidth = m_attributes.m_StrokeWidth;
                m_attributes.m_StrokeWidth = GetPenSizeForBold( size );
            }
            else
            {
                // Restore the original stroke width from `m_StoredStrokeWidth` if it was previously stored,
                // resetting the width after unbolding.
                if( m_attributes.m_StoredStrokeWidth )
                    m_attributes.m_StrokeWidth = m_attributes.m_StoredStrokeWidth;
                else
                {
                    m_attributes.m_StrokeWidth = GetPenSizeForNormal( size );
                    // Sets `m_StrokeWidth` to the normal pen size and stores it in `m_StoredStrokeWidth`
                    // as the default, but only if the bold option was applied before this feature was implemented.
                    m_attributes.m_StoredStrokeWidth = m_attributes.m_StrokeWidth;
                }
            }
        }
        else
        {
            // For outline fonts, boldness is determined by the font itself.
            SetFont( KIFONT::FONT::GetFont( font->GetName(), aBold, IsItalic() ) );
        }
    }

    SetBoldFlag( aBold );
}


void EDA_TEXT::SetBoldFlag( bool aBold )
{
    m_attributes.m_Bold = aBold;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetVisible( bool aVisible )
{
    m_attributes.m_Visible = aVisible;
    ClearRenderCache();
}


void EDA_TEXT::SetMirrored( bool isMirrored )
{
    m_attributes.m_Mirrored = isMirrored;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetMultilineAllowed( bool aAllow )
{
    m_attributes.m_Multiline = aAllow;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetHorizJustify( GR_TEXT_H_ALIGN_T aType )
{
    m_attributes.m_Halign = aType;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetVertJustify( GR_TEXT_V_ALIGN_T aType )
{
    m_attributes.m_Valign = aType;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetKeepUpright( bool aKeepUpright )
{
    m_attributes.m_KeepUpright = aKeepUpright;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetAttributes( const EDA_TEXT& aSrc, bool aSetPosition )
{
    m_attributes = aSrc.m_attributes;

    if( aSetPosition )
        m_pos = aSrc.m_pos;

    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SwapText( EDA_TEXT& aTradingPartner )
{
    std::swap( m_text, aTradingPartner.m_text );
    cacheShownText();
}


void EDA_TEXT::SwapAttributes( EDA_TEXT& aTradingPartner )
{
    std::swap( m_attributes, aTradingPartner.m_attributes );
    std::swap( m_pos, aTradingPartner.m_pos );

    ClearRenderCache();
    aTradingPartner.ClearRenderCache();

    m_bounding_box_cache_valid = false;
    aTradingPartner.m_bounding_box_cache_valid = false;
}


int EDA_TEXT::GetEffectiveTextPenWidth( int aDefaultPenWidth ) const
{
    int penWidth = GetTextThickness();

    if( penWidth <= 1 )
    {
        penWidth = aDefaultPenWidth;

        if( IsBold() )
            penWidth = GetPenSizeForBold( GetTextWidth() );
        else if( penWidth <= 1 )
            penWidth = GetPenSizeForNormal( GetTextWidth() );
    }

    // Clip pen size for small texts:
    penWidth = Clamp_Text_PenSize( penWidth, GetTextSize() );

    return penWidth;
}


bool EDA_TEXT::Replace( const EDA_SEARCH_DATA& aSearchData )
{
    bool retval = EDA_ITEM::Replace( aSearchData, m_text );

    cacheShownText();

    ClearRenderCache();
    m_bounding_box_cache_valid = false;

    return retval;
}


void EDA_TEXT::SetFont( KIFONT::FONT* aFont )
{
    m_attributes.m_Font = aFont;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetLineSpacing( double aLineSpacing )
{
    m_attributes.m_LineSpacing = aLineSpacing;
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetTextSize( VECTOR2I aNewSize, bool aEnforceMinTextSize )
{
    // Plotting uses unityScale and independently scales the text.  If we clamp here we'll
    // clamp to *really* small values.
    if( m_IuScale.get().IU_PER_MM == unityScale.IU_PER_MM )
        aEnforceMinTextSize = false;

    if( aEnforceMinTextSize )
    {
        int min = m_IuScale.get().mmToIU( TEXT_MIN_SIZE_MM );
        int max = m_IuScale.get().mmToIU( TEXT_MAX_SIZE_MM );

        aNewSize = VECTOR2I( alg::clamp( min, aNewSize.x, max ),
                             alg::clamp( min, aNewSize.y, max ) );
    }

    m_attributes.m_Size = aNewSize;

    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetTextWidth( int aWidth )
{
    int min = m_IuScale.get().mmToIU( TEXT_MIN_SIZE_MM );
    int max = m_IuScale.get().mmToIU( TEXT_MAX_SIZE_MM );

    m_attributes.m_Size.x = alg::clamp( min, aWidth, max );
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetTextHeight( int aHeight )
{
    int min = m_IuScale.get().mmToIU( TEXT_MIN_SIZE_MM );
    int max = m_IuScale.get().mmToIU( TEXT_MAX_SIZE_MM );

    m_attributes.m_Size.y = alg::clamp( min, aHeight, max );
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::SetTextPos( const VECTOR2I& aPoint )
{
    Offset( VECTOR2I( aPoint.x - m_pos.x, aPoint.y - m_pos.y ) );
}


void EDA_TEXT::SetTextX( int aX )
{
    Offset( VECTOR2I( aX - m_pos.x, 0 ) );
}


void EDA_TEXT::SetTextY( int aY )
{
    Offset( VECTOR2I( 0, aY - m_pos.y ) );
}


void EDA_TEXT::Offset( const VECTOR2I& aOffset )
{
    if( aOffset.x == 0 && aOffset.y == 0 )
        return;

    m_pos += aOffset;

    for( std::unique_ptr<KIFONT::GLYPH>& glyph : m_render_cache )
    {
        if( KIFONT::OUTLINE_GLYPH* outline = dynamic_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() ) )
            outline->Move( aOffset );
        else if( KIFONT::STROKE_GLYPH* stroke = dynamic_cast<KIFONT::STROKE_GLYPH*>( glyph.get() ) )
            glyph = stroke->Transform( { 1.0, 1.0 }, aOffset, 0, ANGLE_0, false, { 0, 0 } );
    }

    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::Empty()
{
    m_text.Empty();
    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


void EDA_TEXT::cacheShownText()
{
    if( m_text.IsEmpty() )
    {
        m_shown_text = wxEmptyString;
        m_shown_text_has_text_var_refs = false;
    }
    else
    {
        m_shown_text = UnescapeString( m_text );
        m_shown_text_has_text_var_refs = m_shown_text.Contains( wxT( "${" ) );
    }

    ClearRenderCache();
    m_bounding_box_cache_valid = false;
}


KIFONT::FONT* EDA_TEXT::getDrawFont() const
{
    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( wxEmptyString, IsBold(), IsItalic() );

    return font;
}


const KIFONT::METRICS& EDA_TEXT::getFontMetrics() const
{
    return KIFONT::METRICS::Default();
}


void EDA_TEXT::ClearRenderCache()
{
    m_render_cache.clear();
}


void EDA_TEXT::ClearBoundingBoxCache()
{
    m_bounding_box_cache_valid = false;
}


std::vector<std::unique_ptr<KIFONT::GLYPH>>*
EDA_TEXT::GetRenderCache( const KIFONT::FONT* aFont, const wxString& forResolvedText,
                          const VECTOR2I& aOffset ) const
{
    if( aFont->IsOutline() )
    {
        EDA_ANGLE resolvedAngle = GetDrawRotation();

        if( m_render_cache.empty()
                || m_render_cache_font != aFont
                || m_render_cache_text != forResolvedText
                || m_render_cache_angle != resolvedAngle
                || m_render_cache_offset != aOffset )
        {
            m_render_cache.clear();

            const KIFONT::OUTLINE_FONT* font = static_cast<const KIFONT::OUTLINE_FONT*>( aFont );
            TEXT_ATTRIBUTES             attrs = GetAttributes();

            attrs.m_Angle = resolvedAngle;

            font->GetLinesAsGlyphs( &m_render_cache, forResolvedText, GetDrawPos() + aOffset,
                                    attrs, getFontMetrics() );
            m_render_cache_font = aFont;
            m_render_cache_angle = resolvedAngle;
            m_render_cache_text = forResolvedText;
            m_render_cache_offset = aOffset;
        }

        return &m_render_cache;
    }

    return nullptr;
}


void EDA_TEXT::SetupRenderCache( const wxString& aResolvedText, const KIFONT::FONT* aFont,
                                 const EDA_ANGLE& aAngle, const VECTOR2I& aOffset )
{
    m_render_cache_text = aResolvedText;
    m_render_cache_font = aFont;
    m_render_cache_angle = aAngle;
    m_render_cache_offset = aOffset;
    m_render_cache.clear();
}


void EDA_TEXT::AddRenderCacheGlyph( const SHAPE_POLY_SET& aPoly )
{
    m_render_cache.emplace_back( std::make_unique<KIFONT::OUTLINE_GLYPH>( aPoly ) );
    static_cast<KIFONT::OUTLINE_GLYPH*>( m_render_cache.back().get() )->CacheTriangulation();
}


int EDA_TEXT::GetInterline() const
{
    return KiROUND( getDrawFont()->GetInterline( GetTextHeight(), getFontMetrics() ) );
}


BOX2I EDA_TEXT::GetTextBox( int aLine, bool aInvertY ) const
{
    VECTOR2I drawPos = GetDrawPos();

    if( m_bounding_box_cache_valid
            && m_bounding_box_cache_pos == drawPos
            && m_bounding_box_cache_line == aLine
            && m_bounding_box_cache_inverted == aInvertY  )
    {
        return m_bounding_box_cache;
    }

    BOX2I          bbox;
    wxArrayString  strings;
    wxString       text = GetShownText( true );
    int            thickness = GetEffectiveTextPenWidth();

    if( IsMultilineAllowed() )
    {
        wxStringSplit( text, strings, '\n' );

        if( strings.GetCount() )     // GetCount() == 0 for void strings with multilines allowed
        {
            if( aLine >= 0 && ( aLine < static_cast<int>( strings.GetCount() ) ) )
                text = strings.Item( aLine );
            else
                text = strings.Item( 0 );
        }
    }

    // calculate the H and V size
    KIFONT::FONT* font = getDrawFont();
    VECTOR2D      fontSize( GetTextSize() );
    bool          bold = IsBold();
    bool          italic = IsItalic();
    VECTOR2I      extents = font->StringBoundaryLimits( text, fontSize, thickness, bold, italic,
                                                        getFontMetrics() );
    int           overbarOffset = 0;

    // Creates bounding box (rectangle) for horizontal, left and top justified text. The
    // bounding box will be moved later according to the actual text options
    VECTOR2I textsize = VECTOR2I( extents.x, extents.y );
    VECTOR2I pos = drawPos;
    int      fudgeFactor = KiROUND( extents.y * 0.17 );

    if( font->IsStroke() )
        textsize.y += fudgeFactor;

    if( IsMultilineAllowed() && aLine > 0 && aLine < (int) strings.GetCount() )
        pos.y -= KiROUND( aLine * font->GetInterline( fontSize.y, getFontMetrics() ) );

    if( text.Contains( wxT( "~{" ) ) )
        overbarOffset = extents.y / 6;

    if( aInvertY )
        pos.y = -pos.y;

    bbox.SetOrigin( pos );

    // for multiline texts and aLine < 0, merge all rectangles (aLine == -1 signals all lines)
    if( IsMultilineAllowed() && aLine < 0 && strings.GetCount() > 1 )
    {
        for( unsigned ii = 1; ii < strings.GetCount(); ii++ )
        {
            text = strings.Item( ii );
            extents = font->StringBoundaryLimits( text, fontSize, thickness, bold, italic,
                                                  getFontMetrics() );
            textsize.x = std::max( textsize.x, extents.x );
        }

        // interline spacing is only *between* lines, so total height is the height of the first
        // line plus the interline distance (with interline spacing) for all subsequent lines
        textsize.y += KiROUND( ( strings.GetCount() - 1 ) * font->GetInterline( fontSize.y,
                                                                                getFontMetrics() ) );
    }

    textsize.y += overbarOffset;

    bbox.SetSize( textsize );

    /*
     * At this point the rectangle origin is the text origin (m_Pos).  This is correct only for
     * left and top justified, non-mirrored, non-overbarred texts. Recalculate for all others.
     */
    int italicOffset = IsItalic() ? KiROUND( fontSize.y * ITALIC_TILT ) : 0;

    switch( GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:
        if( IsMirrored() )
            bbox.SetX( bbox.GetX() - ( bbox.GetWidth() - italicOffset ) );
        break;

    case GR_TEXT_H_ALIGN_CENTER:
        bbox.SetX( bbox.GetX() - ( bbox.GetWidth() - italicOffset ) / 2 );
        break;

    case GR_TEXT_H_ALIGN_RIGHT:
        if( !IsMirrored() )
            bbox.SetX( bbox.GetX() - ( bbox.GetWidth() - italicOffset ) );
        break;
    }

    switch( GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP:
        bbox.Offset( 0, -fudgeFactor );
        break;

    case GR_TEXT_V_ALIGN_CENTER:
        bbox.SetY( bbox.GetY() - bbox.GetHeight() / 2 );
        break;

    case GR_TEXT_V_ALIGN_BOTTOM:
        bbox.SetY( bbox.GetY() - bbox.GetHeight() );
        bbox.Offset( 0, fudgeFactor );
        break;
    }

    bbox.Normalize();       // Make h and v sizes always >= 0

    m_bounding_box_cache_valid = true;
    m_bounding_box_cache_pos = drawPos;
    m_bounding_box_cache_line = aLine;
    m_bounding_box_cache_inverted = aInvertY;
    m_bounding_box_cache = bbox;

    return bbox;
}


bool EDA_TEXT::TextHitTest( const VECTOR2I& aPoint, int aAccuracy ) const
{
    BOX2I    rect = GetTextBox();
    VECTOR2I location = aPoint;

    rect.Inflate( aAccuracy );
    RotatePoint( location, GetDrawPos(), -GetDrawRotation() );

    return rect.Contains( location );
}


bool EDA_TEXT::TextHitTest( const BOX2I& aRect, bool aContains, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetTextBox() );

    return rect.Intersects( GetTextBox(), GetDrawRotation() );
}


void EDA_TEXT::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset,
                      const COLOR4D& aColor, OUTLINE_MODE aFillMode )
{
    if( IsMultilineAllowed() )
    {
        std::vector<VECTOR2I> positions;
        wxArrayString  strings;
        wxStringSplit( GetShownText( true ), strings, '\n' );

        positions.reserve( strings.Count() );

        GetLinePositions( positions, (int) strings.Count() );

        for( unsigned ii = 0; ii < strings.Count(); ii++ )
            printOneLineOfText( aSettings, aOffset, aColor, aFillMode, strings[ii], positions[ii] );
    }
    else
    {
        printOneLineOfText( aSettings, aOffset, aColor, aFillMode, GetShownText( true ),
                            GetDrawPos() );
    }
}


void EDA_TEXT::GetLinePositions( std::vector<VECTOR2I>& aPositions, int aLineCount ) const
{
    VECTOR2I pos = GetDrawPos();    // Position of first line of the multiline text according
                                    // to the center of the multiline text block

    VECTOR2I offset;                // Offset to next line.

    offset.y = GetInterline();

    if( aLineCount > 1 )
    {
        switch( GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:
            break;

        case GR_TEXT_V_ALIGN_CENTER:
            pos.y -= ( aLineCount - 1 ) * offset.y / 2;
            break;

        case GR_TEXT_V_ALIGN_BOTTOM:
            pos.y -= ( aLineCount - 1 ) * offset.y;
            break;
        }
    }

    // Rotate the position of the first line around the center of the multiline text block
    RotatePoint( pos, GetDrawPos(), GetDrawRotation() );

    // Rotate the offset lines to increase happened in the right direction
    RotatePoint( offset, GetDrawRotation() );

    for( int ii = 0; ii < aLineCount; ii++ )
    {
        aPositions.push_back( (VECTOR2I) pos );
        pos += offset;
    }
}


void EDA_TEXT::printOneLineOfText( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset,
                                   const COLOR4D& aColor, OUTLINE_MODE aFillMode,
                                   const wxString& aText, const VECTOR2I& aPos )
{
    wxDC* DC = aSettings->GetPrintDC();
    int   penWidth = GetEffectiveTextPenWidth( aSettings->GetDefaultPenWidth() );

    if( aFillMode == SKETCH )
        penWidth = -penWidth;

    VECTOR2I size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), IsBold(), IsItalic() );

    GRPrintText( DC, aOffset + aPos, aColor, aText, GetDrawRotation(), size, GetHorizJustify(),
                 GetVertJustify(), penWidth, IsItalic(), IsBold(), font, getFontMetrics() );
}


wxString EDA_TEXT::GetTextStyleName() const
{
    int style = 0;

    if( IsItalic() )
        style = 1;

    if( IsBold() )
        style += 2;

    wxString stylemsg[4] = {
        _("Normal"),
        _("Italic"),
        _("Bold"),
        _("Bold+Italic")
    };

    return stylemsg[style];
}


wxString EDA_TEXT::GetFontName() const
{
    if( GetFont() )
        return GetFont()->GetName();
    else
        return wxEmptyString;
}


int EDA_TEXT::GetFontIndex() const
{
    if( !GetFont() )
        return -1;

    if( GetFont()->GetName() == KICAD_FONT_NAME )
        return -2;

    std::vector<std::string> fontNames;
    Fontconfig()->ListFonts( fontNames, std::string( Pgm().GetLanguageTag().utf8_str() ) );

    for( int ii = 0; ii < (int) fontNames.size(); ++ii )
    {
        if( fontNames[ii] == GetFont()->GetName() )
            return ii;
    }

    return 0;
}


void EDA_TEXT::SetFontIndex( int aIdx )
{
    if( aIdx == -1 )
    {
        SetFont( nullptr );
    }
    else if( aIdx == -2 )
    {
        SetFont( KIFONT::FONT::GetFont( wxEmptyString, IsBold(), IsItalic() ) );
    }
    else
    {
        std::vector<std::string> fontNames;
        Fontconfig()->ListFonts( fontNames, std::string( Pgm().GetLanguageTag().utf8_str() ) );

        if( aIdx >= 0 && aIdx < static_cast<int>( fontNames.size() ) )
            SetFont( KIFONT::FONT::GetFont( fontNames[ aIdx ], IsBold(), IsItalic() ) );
        else
            SetFont( nullptr );
    }
}


bool EDA_TEXT::IsDefaultFormatting() const
{
    return ( IsVisible()
             && !IsMirrored()
             && GetHorizJustify() == GR_TEXT_H_ALIGN_CENTER
             && GetVertJustify() == GR_TEXT_V_ALIGN_CENTER
             && GetTextThickness() == 0
             && !IsItalic()
             && !IsBold()
             && !IsMultilineAllowed()
             && GetFontName().IsEmpty()
           );
}


void EDA_TEXT::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
{
    aFormatter->Print( aNestLevel + 1, "(effects" );

    aFormatter->Print( 0, " (font" );

    if( GetFont() && !GetFont()->GetName().IsEmpty() )
        aFormatter->Print( 0, " (face \"%s\")", GetFont()->NameAsToken() );

    // Text size
    aFormatter->Print( 0, " (size %s %s)",
                       EDA_UNIT_UTILS::FormatInternalUnits( m_IuScale, GetTextHeight() ).c_str(),
                       EDA_UNIT_UTILS::FormatInternalUnits( m_IuScale, GetTextWidth() ).c_str() );

    if( GetLineSpacing() != 1.0 )
    {
        aFormatter->Print( 0, " (line_spacing %s)",
                           FormatDouble2Str( GetLineSpacing() ).c_str() );
    }

    if( GetTextThickness() )
    {
        aFormatter->Print( 0, " (thickness %s)",
                EDA_UNIT_UTILS::FormatInternalUnits( m_IuScale, GetTextThickness() ).c_str() );
    }

    if( IsBold() )
        aFormatter->Print( 0, " (bold yes)" );

    if( IsItalic() )
        aFormatter->Print( 0, " (italic yes)" );

    if( !( aControlBits & CTL_OMIT_COLOR ) && GetTextColor() != COLOR4D::UNSPECIFIED )
    {
        aFormatter->Print( 0, " (color %d %d %d %s)",
                           KiROUND( GetTextColor().r * 255.0 ),
                           KiROUND( GetTextColor().g * 255.0 ),
                           KiROUND( GetTextColor().b * 255.0 ),
                           FormatDouble2Str( GetTextColor().a ).c_str() );
    }

    aFormatter->Print( 0, ")"); // (font

    if( IsMirrored() || GetHorizJustify() != GR_TEXT_H_ALIGN_CENTER
                     || GetVertJustify() != GR_TEXT_V_ALIGN_CENTER )
    {
        aFormatter->Print( 0, " (justify");

        if( GetHorizJustify() != GR_TEXT_H_ALIGN_CENTER )
            aFormatter->Print( 0, GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT ? " left" : " right" );

        if( GetVertJustify() != GR_TEXT_V_ALIGN_CENTER )
            aFormatter->Print( 0, GetVertJustify() == GR_TEXT_V_ALIGN_TOP ? " top" : " bottom" );

        if( IsMirrored() )
            aFormatter->Print( 0, " mirror" );

        aFormatter->Print( 0, ")" ); // (justify
    }

    if( !( aControlBits & CTL_OMIT_HIDE ) && !IsVisible() )
        aFormatter->Print( 0, " (hide yes)" );

    if( !( aControlBits & CTL_OMIT_HYPERLINK ) && HasHyperlink() )
    {
        aFormatter->Print( 0, " (href %s)", aFormatter->Quotew( GetHyperlink() ).c_str() );
    }

    aFormatter->Print( 0, ")\n" ); // (effects
}


std::shared_ptr<SHAPE_COMPOUND> EDA_TEXT::GetEffectiveTextShape( bool aTriangulate,
                                                                 const BOX2I& aBBox,
                                                                 const EDA_ANGLE& aAngle ) const
{
    std::shared_ptr<SHAPE_COMPOUND> shape = std::make_shared<SHAPE_COMPOUND>();
    KIGFX::GAL_DISPLAY_OPTIONS      empty_opts;
    KIFONT::FONT*                   font = getDrawFont();
    int                             penWidth = GetEffectiveTextPenWidth();
    wxString                        shownText( GetShownText( true ) );
    VECTOR2I                        drawPos = GetDrawPos();
    TEXT_ATTRIBUTES                 attrs = GetAttributes();

    std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

    if( aBBox.GetWidth() )
    {
        drawPos = aBBox.GetCenter();
        attrs.m_Halign = GR_TEXT_H_ALIGN_CENTER;
        attrs.m_Valign = GR_TEXT_V_ALIGN_CENTER;
        attrs.m_Angle = aAngle;
    }
    else
    {
        attrs.m_Angle = GetDrawRotation();

        if( font->IsOutline() )
            cache = GetRenderCache( font, shownText, VECTOR2I() );
    }

    if( aTriangulate )
    {
        CALLBACK_GAL callback_gal(
                empty_opts,
                // Stroke callback
                [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
                {
                    shape->AddShape( new SHAPE_SEGMENT( aPt1, aPt2, penWidth ) );
                },
                // Triangulation callback
                [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
                {
                    SHAPE_SIMPLE* triShape = new SHAPE_SIMPLE;

                    for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                        triShape->Append( point.x, point.y );

                    shape->AddShape( triShape );
                } );

        if( cache )
            callback_gal.DrawGlyphs( *cache );
        else
            font->Draw( &callback_gal, shownText, drawPos, attrs, getFontMetrics() );
    }
    else
    {
        CALLBACK_GAL callback_gal(
                empty_opts,
                // Stroke callback
                [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
                {
                    shape->AddShape( new SHAPE_SEGMENT( aPt1, aPt2, penWidth ) );
                },
                // Outline callback
                [&]( const SHAPE_LINE_CHAIN& aPoly )
                {
                    shape->AddShape( aPoly.Clone() );
                } );

        if( cache )
            callback_gal.DrawGlyphs( *cache );
        else
            font->Draw( &callback_gal, shownText, drawPos, attrs, getFontMetrics() );
    }

    return shape;
}


int EDA_TEXT::Compare( const EDA_TEXT* aOther ) const
{
    wxCHECK( aOther, 1 );

    int val = m_attributes.Compare( aOther->m_attributes );

    if( val != 0 )
        return val;

    if( m_pos.x != aOther->m_pos.x )
        return m_pos.x - aOther->m_pos.x;

    if( m_pos.y != aOther->m_pos.y )
        return m_pos.y - aOther->m_pos.y;

    val = GetFontName().Cmp( aOther->GetFontName() );

    if( val != 0 )
        return val;

    return m_text.Cmp( aOther->m_text );
}


bool EDA_TEXT::ValidateHyperlink( const wxString& aURL )
{
    if( aURL.IsEmpty() || IsGotoPageHref( aURL ) )
        return true;

    // Limit valid urls to file, http and https for now. Note wxURL doesn't support https
    wxURI uri;

    if( uri.Create( aURL ) && uri.HasScheme() )
    {
        const wxString& scheme = uri.GetScheme();
        return scheme == wxT( "file" )  || scheme == wxT( "http" ) || scheme == wxT( "https" );
    }

    return false;
}

double EDA_TEXT::Levenshtein( const EDA_TEXT& aOther ) const
{
    // Compute the Levenshtein distance between the two strings
    const wxString& str1 = GetText();
    const wxString& str2 = aOther.GetText();

    int m = str1.length();
    int n = str2.length();

    if( n == 0 || m == 0 )
        return 0.0;

    // Create a matrix to store the distance values
    std::vector<std::vector<int>> distance(m + 1, std::vector<int>(n + 1));

    // Initialize the matrix
    for( int i = 0; i <= m; i++ )
        distance[i][0] = i;
    for( int j = 0; j <= n; j++ )
        distance[0][j] = j;

    // Calculate the distance
    for( int i = 1; i <= m; i++ )
    {
        for( int j = 1; j <= n; j++ )
        {
            if( str1[i - 1] == str2[j - 1] )
            {
                distance[i][j] = distance[i - 1][j - 1];
            }
            else
            {
                distance[i][j] = std::min( { distance[i - 1][j], distance[i][j - 1],
                                             distance[i - 1][j - 1] } ) + 1;
            }
        }
    }

    // Calculate similarity score
    int    maxLen = std::max( m, n );
    double similarity = 1.0 - ( static_cast<double>( distance[m][n] ) / maxLen );

    return similarity;
}


double EDA_TEXT::Similarity( const EDA_TEXT& aOther ) const
{
    double retval = 1.0;

    if( !( m_attributes == aOther.m_attributes ) )
        retval *= 0.9;

    if( m_pos != aOther.m_pos )
        retval *= 0.9;

    retval *= Levenshtein( aOther );

    return retval;
}


bool EDA_TEXT::IsGotoPageHref( const wxString& aHref, wxString* aDestination )
{
    return aHref.StartsWith( wxT( "#" ), aDestination );
}


wxString EDA_TEXT::GotoPageHref( const wxString& aDestination )
{
    return wxT( "#" ) + aDestination;
}


std::ostream& operator<<( std::ostream& aStream, const EDA_TEXT& aText )
{
    aStream << aText.GetText();

    return aStream;
}


static struct EDA_TEXT_DESC
{
    EDA_TEXT_DESC()
    {
        ENUM_MAP<GR_TEXT_H_ALIGN_T>::Instance()
                .Map( GR_TEXT_H_ALIGN_LEFT,   _HKI( "Left" ) )
                .Map( GR_TEXT_H_ALIGN_CENTER, _HKI( "Center" ) )
                .Map( GR_TEXT_H_ALIGN_RIGHT,  _HKI( "Right" ) );
        ENUM_MAP<GR_TEXT_V_ALIGN_T>::Instance()
                .Map( GR_TEXT_V_ALIGN_TOP,    _HKI( "Top" ) )
                .Map( GR_TEXT_V_ALIGN_CENTER, _HKI( "Center" ) )
                .Map( GR_TEXT_V_ALIGN_BOTTOM, _HKI( "Bottom" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( EDA_TEXT );

        propMgr.AddProperty( new PROPERTY<EDA_TEXT, double>( _HKI( "Orientation" ),
                &EDA_TEXT::SetTextAngleDegrees, &EDA_TEXT::GetTextAngleDegrees,
                PROPERTY_DISPLAY::PT_DEGREE ) );

        const wxString textProps = _HKI( "Text Properties" );

        propMgr.AddProperty( new PROPERTY<EDA_TEXT, wxString>( _HKI( "Text" ),
                &EDA_TEXT::SetText, &EDA_TEXT::GetText ),
                textProps );

        // This must be a PROPERTY_ENUM to get a choice list.
        // SCH_ and PCB_PROPERTIES_PANEL::updateFontList() fill in the enum values.
        propMgr.AddProperty( new PROPERTY_ENUM<EDA_TEXT, int>( _HKI( "Font" ),
                &EDA_TEXT::SetFontIndex, &EDA_TEXT::GetFontIndex ),
                textProps )
            .SetIsHiddenFromRulesEditor();

        propMgr.AddProperty( new PROPERTY<EDA_TEXT, int>( _HKI( "Thickness" ),
                &EDA_TEXT::SetTextThickness, &EDA_TEXT::GetTextThickness,
                PROPERTY_DISPLAY::PT_SIZE ),
                textProps );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Italic" ),
                &EDA_TEXT::SetItalic,
                &EDA_TEXT::IsItalic ),
                textProps );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Bold" ),
                &EDA_TEXT::SetBold, &EDA_TEXT::IsBold ),
                textProps );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Mirrored" ),
                &EDA_TEXT::SetMirrored, &EDA_TEXT::IsMirrored ),
                textProps );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Visible" ),
                &EDA_TEXT::SetVisible, &EDA_TEXT::IsVisible ),
                textProps );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, int>( _HKI( "Width" ),
                &EDA_TEXT::SetTextWidth, &EDA_TEXT::GetTextWidth,
                PROPERTY_DISPLAY::PT_SIZE ),
                textProps );

        propMgr.AddProperty( new PROPERTY<EDA_TEXT, int>( _HKI( "Height" ),
                &EDA_TEXT::SetTextHeight, &EDA_TEXT::GetTextHeight,
                PROPERTY_DISPLAY::PT_SIZE ),
                textProps );

        propMgr.AddProperty( new PROPERTY_ENUM<EDA_TEXT, GR_TEXT_H_ALIGN_T>(
                _HKI( "Horizontal Justification" ),
                &EDA_TEXT::SetHorizJustify, &EDA_TEXT::GetHorizJustify ),
                textProps );
        propMgr.AddProperty( new PROPERTY_ENUM<EDA_TEXT, GR_TEXT_V_ALIGN_T>(
                _HKI( "Vertical Justification" ),
                &EDA_TEXT::SetVertJustify, &EDA_TEXT::GetVertJustify ),
                textProps );

        propMgr.AddProperty( new PROPERTY<EDA_TEXT, COLOR4D>( _HKI( "Color" ),
                &EDA_TEXT::SetTextColor, &EDA_TEXT::GetTextColor ),
                textProps );

        propMgr.AddProperty( new PROPERTY<EDA_TEXT, wxString>( _HKI( "Hyperlink" ),
                &EDA_TEXT::SetHyperlink, &EDA_TEXT::GetHyperlink ),
                textProps );
    }
} _EDA_TEXT_DESC;

ENUM_TO_WXANY( GR_TEXT_H_ALIGN_T )
ENUM_TO_WXANY( GR_TEXT_V_ALIGN_T )
