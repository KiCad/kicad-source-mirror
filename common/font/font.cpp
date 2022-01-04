/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Font abstract base class
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

#include <wx/font.h>
#include <string_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <font/stroke_font.h>
#include <trigo.h>
#include <markup_parser.h>

using namespace KIFONT;

FONT* FONT::s_defaultFont = nullptr;

std::map< std::tuple<wxString, bool, bool>, FONT*> FONT::s_fontMap;


FONT::FONT()
{
}


const wxString& FONT::Name() const
{
    return m_fontName;
}


FONT* FONT::getDefaultFont()
{
    // FONT TODO: default font should be user-selectable in Eeschema but the KiCad stroke
    // font in Pcbnew

    if( !s_defaultFont )
        s_defaultFont = STROKE_FONT::LoadFont( wxEmptyString );

    return s_defaultFont;
}


FONT* FONT::GetFont( const wxString& aFontName, bool aBold, bool aItalic )
{
    if( aFontName.empty() )
        return getDefaultFont();

    std::tuple<wxString, bool, bool> key = { aFontName, aBold, aItalic };

    FONT* font = s_fontMap[key];

#if 0
        // FONT TODO: load a real font
    if( !font )
        font = OUTLINE_FONT::LoadFont( aFontName, aBold, aItalic );
#else

    if( !font )
        font = getDefaultFont();
#endif

    s_fontMap[key] = font;

    return font;
}


bool FONT::IsStroke( const wxString& aFontName )
{
#if 0  // FONT TODO
    // Stroke fonts will be loaded under all four bold/italic combinations, so we only have
    // to check for one.
    std::tuple<wxString, bool, bool> key = { aFontName, false, false };

    FONT* font = s_fontMap[key];

    return font && font->IsStroke();
#else
    return aFontName == _( "Default Font" );
#endif
}


/**
 * Draw a string.
 *
 * @param aGal
 * @param aTextItem is the underlying text item
 * @param aPosition is the text position
 * @return bounding box width/height
 */
VECTOR2D FONT::doDrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                             bool aParse, const TEXT_ATTRIBUTES& aAttrs ) const
{
    if( aText.empty() )
        return VECTOR2D( 0.0, 0.0 );

    wxArrayString         strings;
    std::vector<wxPoint>  positions;
    int                   n_lines;
    VECTOR2D              boundingBox;
    std::vector<VECTOR2D> lineBoundingBoxes;

    getLinePositions( aText, aPosition, strings, positions, n_lines, lineBoundingBoxes, aAttrs );

    for( int i = 0; i < n_lines; i++ )
    {
        VECTOR2D lineBoundingBox;
        if( aParse )
        {
            MARKUP::MARKUP_PARSER markupParser( std::string( strings.Item( i ) ) );
            //auto                  parse_result = markupParser.Parse();
            VECTOR2D cursor = positions[i];

            std::function<void( const std::unique_ptr<MARKUP::NODE>& )> nodeHandler =
                    [&]( const std::unique_ptr<MARKUP::NODE>& aNode )
                    {
                        if( !aNode->is_root() && aNode->has_content() )
                        {
                            VECTOR2D itemBoundingBox = Draw( aGal, aNode->string(), cursor,
                                                             aPosition, aAttrs );
                            lineBoundingBox += itemBoundingBox;
                            cursor += itemBoundingBox;
                        }

                        for( const auto& child : aNode->children )
                            nodeHandler( child );
                    };

            nodeHandler( markupParser.Parse() );
        }
        else
        {
            lineBoundingBox = Draw( aGal, strings.Item( i ), positions[i], aPosition, aAttrs );
        }

        boundingBox.x = fmax( boundingBox.x, lineBoundingBox.x );
    }

    boundingBox.y = ( n_lines + 1 ) * GetInterline( aAttrs.m_Size.y );

    return boundingBox;
}


void FONT::getLinePositions( const UTF8& aText, const VECTOR2D& aPosition,
                             wxArrayString& aStringList, std::vector<wxPoint>& aPositions,
                             int& aLineCount, std::vector<VECTOR2D>& aBoundingBoxes,
                             const TEXT_ATTRIBUTES& aAttrs ) const
{
    wxStringSplit( aText, aStringList, '\n' );
    aLineCount = aStringList.Count();
    aPositions.reserve( aLineCount );

    wxPoint origin( aPosition.x, aPosition.y );
    int     interline = GetInterline( aAttrs.m_Size.y, aAttrs.m_LineSpacing );
    int     height = 0;

    for( int i = 0; i < aLineCount; i++ )
    {
        VECTOR2D pos( origin.x, origin.y + i * interline );
        VECTOR2D end = boundingBoxSingleLine( nullptr, aStringList[i], pos, aAttrs.m_Size,
                                              aAttrs.m_Angle, aAttrs.m_Italic );
        VECTOR2D bBox( end - pos );

        aBoundingBoxes.push_back( bBox );

        if( i == 0 )
            height += aAttrs.m_Size.y;
        else
            height += interline;
    }

    wxPoint offset( 0, 0 );
    offset.y += aAttrs.m_Size.y;

    switch(  aAttrs.m_Valign )
    {
    case GR_TEXT_V_ALIGN_TOP:                            break;
    case GR_TEXT_V_ALIGN_CENTER: offset.y -= height / 2; break;
    case GR_TEXT_V_ALIGN_BOTTOM: offset.y -= height;     break;
    }

    int mirrorX = aAttrs.m_Mirrored ? -1 : 1;

    for( int i = 0; i < aLineCount; i++ )
    {
        VECTOR2D lineSize = aBoundingBoxes.at( i );
        wxPoint  lineOffset( offset );
        lineOffset.y += i * interline;

        switch( aAttrs.m_Halign )
        {
        case GR_TEXT_H_ALIGN_LEFT:                                             break;
        case GR_TEXT_H_ALIGN_CENTER: lineOffset.x = mirrorX * -lineSize.x / 2; break;
        case GR_TEXT_H_ALIGN_RIGHT:  lineOffset.x = mirrorX * -lineSize.x;     break;
        }

        wxPoint pos( aPosition.x + lineOffset.x, aPosition.y + lineOffset.y );
        RotatePoint( &pos, origin, aAttrs.m_Angle );

        aPositions.push_back( pos );
    }
}


VECTOR2D FONT::getBoundingBox( const UTF8& aText, TEXT_STYLE_FLAGS aTextStyle,
                               const TEXT_ATTRIBUTES& aAttributes ) const
{
    if( aText.empty() )
        return VECTOR2D( 0.0, 0.0 );

    if( false ) // aParse ) // FONT TODO: parse markup!
    {
        MARKUP::MARKUP_PARSER markupParser( aText );
        auto                  parse_result = markupParser.Parse();

        /* ... */
    }

    wxArrayString         strings;
    std::vector<wxPoint>  positions;
    int                   n_lines;
    VECTOR2D              boundingBox;
    std::vector<VECTOR2D> boundingBoxes;

    getLinePositions( aText, VECTOR2D( 0.0, 0.0 ), strings, positions, n_lines, boundingBoxes,
                      aAttributes );

    int i = 1;

    for( VECTOR2D lineBoundingBox : boundingBoxes )
    {
        boundingBox.x = fmax( boundingBox.x, lineBoundingBox.x );
        boundingBox.y += lineBoundingBox.y;
        i++;
    }

    return boundingBox;
}


void FONT::KiDrawText( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                       const TEXT_ATTRIBUTES& aAttributes ) const
{
    // FONT TODO: do we need to set the attributes to the gal at all?
    aGal->SetHorizontalJustify( aAttributes.m_Halign );
    aGal->SetVerticalJustify( aAttributes.m_Valign );
    aGal->SetGlyphSize( aAttributes.m_Size );
    aGal->SetFontItalic( aAttributes.m_Italic );
    aGal->SetFontBold( aAttributes.m_Bold );
    aGal->SetTextMirrored( aAttributes.m_Mirrored );

    Draw( aGal, aText, aPosition, aAttributes );
}


/**
 * Draw a string.
 *
 * @param aGal
 * @param aText is the text to be drawn.
 * @param aPosition is the text position in world coordinates.
 * @param aAngle is the text rotation angle
 */
VECTOR2D FONT::Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                     const VECTOR2D& aOrigin, const TEXT_ATTRIBUTES& aAttrs ) const
{
    if( !aGal || aText.empty() )
        return VECTOR2D( 0, 0 );

    VECTOR2D  position( aPosition - aOrigin );

    // Context needs to be saved before any transformations
    //aGal->Save();

    // Split multiline strings into separate ones and draw them line by line
    wxArrayString         strings_list;
    std::vector<wxPoint>  positions;
    std::vector<VECTOR2D> boundingBoxes;
    int                   n;

    getLinePositions( aText, position, strings_list, positions, n, boundingBoxes, aAttrs );

    VECTOR2D boundingBox( 0, 0 );
    BOX2I lineBoundingBox;

    for( int i = 0; i < n; i++ )
    {
        aGal->Save();
        aGal->Translate( positions[i] );
        aGal->SetLineWidth( aAttrs.m_StrokeWidth );

        if( !aAttrs.m_Angle.IsZero() )
            aGal->Rotate( aAttrs.m_Angle.Invert().AsRadians() );

        (void) drawSingleLineText( aGal, &lineBoundingBox, strings_list[i], VECTOR2D( 0, 0 ),
                                   aAttrs.m_Size, aAttrs.m_Angle, aAttrs.m_Italic,
                                   aAttrs.m_Mirrored );
        aGal->Restore();

        // expand bounding box of whole text
        boundingBox.x = std::max( boundingBox.x, (double) lineBoundingBox.GetWidth() );

        double lineHeight = GetInterline( aAttrs.m_Size.y, aAttrs.m_LineSpacing );
        boundingBox.y += lineHeight;
    }

    // undo rotation
    //aGal->Restore();

    return boundingBox;
}


/**
 * @return position of cursor for drawing next substring
 */
VECTOR2D FONT::drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>& aGlyphs,
                           const std::unique_ptr<MARKUP::NODE>& aNode, const VECTOR2D& aPosition,
                           const VECTOR2D& aGlyphSize, const EDA_ANGLE& aAngle,
                           TEXT_STYLE_FLAGS aTextStyle, int aLevel ) const
{
    VECTOR2D nextPosition = aPosition;

    TEXT_STYLE_FLAGS textStyle = aTextStyle;

    if( !aNode->is_root() )
    {
        if( aNode->isSubscript() )
            textStyle = TEXT_STYLE::SUBSCRIPT;
        else if( aNode->isSuperscript() )
            textStyle = TEXT_STYLE::SUPERSCRIPT;

        if( aNode->isOverbar() )
            textStyle |= TEXT_STYLE::OVERBAR;

        if( aNode->has_content() )
        {
            std::string txt = aNode->string();
            //std::vector<SHAPE_POLY_SET> glyphs;
            wxPoint pt( aPosition.x, aPosition.y );

            BOX2I bbox;
            nextPosition = GetTextAsGlyphs( &bbox, aGlyphs, txt, aGlyphSize, pt, aAngle, textStyle );

            if( aBoundingBox )
            {
                BOX2I boundingBox;
                boundingBox = aBoundingBox->Merge( bbox );
                aBoundingBox->SetOrigin( boundingBox.GetOrigin() );
                aBoundingBox->SetSize( boundingBox.GetSize() );
            }
        }
    }

    for( const auto& child : aNode->children )
    {
        nextPosition = drawMarkup( aBoundingBox, aGlyphs, child, nextPosition, aGlyphSize, aAngle,
                                   textStyle, aLevel + 1 );
    }

    return nextPosition;
}


VECTOR2D FONT::drawSingleLineText( KIGFX::GAL* aGal, BOX2I* aBoundingBox, const UTF8& aText,
                                   const VECTOR2D& aPosition, const VECTOR2D& aGlyphSize,
                                   const EDA_ANGLE& aAngle, bool aIsItalic, bool aIsMirrored ) const
{
    if( !aGal )
    {
        // do nothing, cursor does not move
        return aPosition;
    }

    MARKUP::MARKUP_PARSER         markupParser( aText );
    std::unique_ptr<MARKUP::NODE> markupRoot = markupParser.Parse();
    TEXT_STYLE_FLAGS              textStyle = 0;

    if( aIsItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    std::vector<std::unique_ptr<GLYPH>> glyphs;
    VECTOR2D nextPosition = drawMarkup( aBoundingBox, glyphs, markupRoot, aPosition, aGlyphSize,
                                        aAngle, textStyle );

    for( const std::unique_ptr<GLYPH>& glyph : glyphs )
    {
        if( aIsMirrored )
            glyph->Mirror( aPosition );

        aGal->DrawGlyph( *glyph.get() );
    }

    return nextPosition;
}


VECTOR2D FONT::boundingBoxSingleLine( BOX2I* aBoundingBox, const UTF8& aText,
                                      const VECTOR2D& aPosition, const VECTOR2D& aGlyphSize,
                                      const EDA_ANGLE& aAngle, bool aIsItalic ) const
{
    MARKUP::MARKUP_PARSER         markupParser( aText );
    std::unique_ptr<MARKUP::NODE> markupRoot = markupParser.Parse();
    TEXT_STYLE_FLAGS              textStyle = 0;

    if( aIsItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    std::vector<std::unique_ptr<GLYPH>> glyphs; // ignored
    VECTOR2D nextPosition = drawMarkup( aBoundingBox, glyphs, markupRoot, aPosition, aGlyphSize,
                                        aAngle, false, textStyle );

    return nextPosition;
}
