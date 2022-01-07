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
#include <font/outline_font.h>
#include <trigo.h>
#include <markup_parser.h>


// markup_parser.h includes pegtl.hpp which includes windows.h... which leaks #define DrawText
#undef DrawText


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
    if( aFontName.empty() || aFontName == wxT( "KiCad" ) )
        return getDefaultFont();

    std::tuple<wxString, bool, bool> key = { aFontName, aBold, aItalic };

    FONT* font = s_fontMap[key];

    if( !font )
        font = OUTLINE_FONT::LoadFont( aFontName, aBold, aItalic );

    if( !font )
        font = getDefaultFont();

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
    return aFontName == _( "Default Font" ) || aFontName == wxT( "KiCad" );
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
VECTOR2D FONT::doDrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2I& aPosition,
                             bool aParse, const TEXT_ATTRIBUTES& aAttrs ) const
{
    if( aText.empty() )
        return VECTOR2D( 0.0, 0.0 );

    wxArrayString         strings;
    std::vector<VECTOR2I> positions;
    VECTOR2D              boundingBox;
    std::vector<VECTOR2D> lineBoundingBoxes;

    getLinePositions( aText, aPosition, strings, positions, lineBoundingBoxes, aAttrs );

    for( size_t i = 0; i < strings.GetCount(); i++ )
    {
        VECTOR2D lineBoundingBox;

        if( aParse )
        {
            MARKUP::MARKUP_PARSER markupParser( std::string( strings.Item( i ) ) );
            //auto                  parse_result = markupParser.Parse();
            VECTOR2I cursor = positions[i];

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

    boundingBox.y = ( strings.GetCount() + 1 ) * GetInterline( aAttrs.m_Size.y );

    return boundingBox;
}


void FONT::getLinePositions( const UTF8& aText, const VECTOR2I& aPosition,
                             wxArrayString& aTextLines, std::vector<VECTOR2I>& aPositions,
                             std::vector<VECTOR2D>& aBoundingBoxes,
                             const TEXT_ATTRIBUTES& aAttrs ) const
{
    wxStringSplit( aText, aTextLines, '\n' );
    int lineCount = aTextLines.Count();
    aPositions.reserve( lineCount );

    int interline = GetInterline( aAttrs.m_Size.y, aAttrs.m_LineSpacing );
    int height = 0;

    for( int i = 0; i < lineCount; i++ )
    {
        VECTOR2D pos( aPosition.x, aPosition.y + i * interline );
        VECTOR2D end = boundingBoxSingleLine( nullptr, aTextLines[i], pos, aAttrs.m_Size,
                                              aAttrs.m_Italic );
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

    for( int i = 0; i < lineCount; i++ )
    {
        VECTOR2I lineSize = aBoundingBoxes.at( i );
        wxPoint  lineOffset( offset );

        lineOffset.y += i * interline;

        switch( aAttrs.m_Halign )
        {
        case GR_TEXT_H_ALIGN_LEFT:                                   break;
        case GR_TEXT_H_ALIGN_CENTER: lineOffset.x = -lineSize.x / 2; break;
        case GR_TEXT_H_ALIGN_RIGHT:  lineOffset.x = -lineSize.x;     break;
        }

        aPositions.push_back( aPosition + lineOffset );
    }
}


void FONT::DrawText( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2I& aPosition,
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
 * @param aPosition is the text object position in world coordinates.
 * @param aCursor is the current text position (for multiple text blocks within a single text
 *                object, such as a run of superscript characters)
 * @param aAttrs are the styling attributes of the text, including its rotation
 */
VECTOR2D FONT::Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2I& aPosition,
                     const VECTOR2I& aCursor, const TEXT_ATTRIBUTES& aAttrs ) const
{
    if( !aGal || aText.empty() )
        return VECTOR2D( 0, 0 );

    VECTOR2D  position( aPosition - aCursor );

    // Split multiline strings into separate ones and draw them line by line
    wxArrayString         strings_list;
    std::vector<VECTOR2I> positions;
    std::vector<VECTOR2D> boundingBoxes;

    getLinePositions( aText, position, strings_list, positions, boundingBoxes, aAttrs );

    VECTOR2D boundingBox( 0, 0 );
    BOX2I lineBoundingBox;

    aGal->SetLineWidth( aAttrs.m_StrokeWidth );

    for( size_t i = 0; i < strings_list.GetCount(); i++ )
    {
        (void) drawSingleLineText( aGal, &lineBoundingBox, strings_list[i], positions[i],
                                   aAttrs.m_Size, aAttrs.m_Angle, aAttrs.m_Mirrored, aPosition,
                                   aAttrs.m_Italic );
        // expand bounding box of whole text
        boundingBox.x = std::max( boundingBox.x, (double) lineBoundingBox.GetWidth() );

        double lineHeight = GetInterline( aAttrs.m_Size.y, aAttrs.m_LineSpacing );
        boundingBox.y += lineHeight;
    }

    return boundingBox;
}


/**
 * @return position of cursor for drawing next substring
 */
VECTOR2D drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>& aGlyphs,
                     const std::unique_ptr<MARKUP::NODE>& aNode, const VECTOR2I& aPosition,
                     const KIFONT::FONT* aFont, const VECTOR2D& aSize, const EDA_ANGLE& aAngle,
                     bool aMirror, const VECTOR2I& aOrigin, TEXT_STYLE_FLAGS aTextStyle )
{
    VECTOR2I nextPosition = aPosition;

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
            BOX2I       bbox;

            nextPosition = aFont->GetTextAsGlyphs( &bbox, aGlyphs, txt, aSize, aPosition, aAngle,
                                                   aMirror, aOrigin, textStyle );

            if( aBoundingBox )
                aBoundingBox->Merge( bbox );
        }
    }

    for( const std::unique_ptr<MARKUP::NODE>& child : aNode->children )
    {
        nextPosition = drawMarkup( aBoundingBox, aGlyphs, child, nextPosition, aFont, aSize,
                                   aAngle, aMirror, aOrigin, textStyle );
    }

    return nextPosition;
}


VECTOR2D FONT::drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>& aGlyphs,
                           const UTF8& aText, const VECTOR2I& aPosition, const VECTOR2D& aSize,
                           const EDA_ANGLE& aAngle, bool aMirror, const VECTOR2I& aOrigin,
                           TEXT_STYLE_FLAGS aTextStyle ) const
{
    MARKUP::MARKUP_PARSER         markupParser( aText );
    std::unique_ptr<MARKUP::NODE> root = markupParser.Parse();

    return ::drawMarkup( aBoundingBox, aGlyphs, root, aPosition, this, aSize, aAngle, aMirror,
                         aOrigin, aTextStyle );
}


VECTOR2D FONT::drawSingleLineText( KIGFX::GAL* aGal, BOX2I* aBoundingBox, const UTF8& aText,
                                   const VECTOR2I& aPosition, const VECTOR2D& aSize,
                                   const EDA_ANGLE& aAngle, bool aMirror, const VECTOR2I& aOrigin,
                                   bool aItalic ) const
{
    if( !aGal )
    {
        // do nothing, cursor does not move
        return aPosition;
    }

    TEXT_STYLE_FLAGS textStyle = 0;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    std::vector<std::unique_ptr<GLYPH>> glyphs;
    VECTOR2D nextPosition = drawMarkup( aBoundingBox, glyphs, aText, aPosition, aSize, aAngle,
                                        aMirror, aOrigin, textStyle );

    for( const std::unique_ptr<GLYPH>& glyph : glyphs )
        aGal->DrawGlyph( *glyph.get() );

    return nextPosition;
}


VECTOR2D FONT::StringBoundaryLimits( const UTF8& aText, const VECTOR2D& aSize, int aThickness,
                                     bool aBold, bool aItalic ) const
{
    // TODO do we need to parse every time - have we already parsed?
    std::vector<std::unique_ptr<GLYPH>> glyphs; // ignored
    BOX2I                               boundingBox;
    TEXT_STYLE_FLAGS                    textStyle = 0;

    if( aBold )
        textStyle |= TEXT_STYLE::BOLD;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    (void) drawMarkup( &boundingBox, glyphs, aText, VECTOR2D(), aSize, EDA_ANGLE::ANGLE_0,
                       false, VECTOR2D(), textStyle );

    if( IsStroke() )
    {
        // Inflate by a bit more than thickness/2 to catch diacriticals, descenders, etc.
        boundingBox.Inflate( KiROUND( aThickness * 0.75 ) );
    }
    else if( IsOutline() )
    {
        // Outline fonts have thickness built in
    }

    return boundingBox.GetSize();
}


VECTOR2D FONT::boundingBoxSingleLine( BOX2I* aBoundingBox, const UTF8& aText,
                                      const VECTOR2I& aPosition, const VECTOR2D& aSize,
                                      bool aItalic ) const
{
    TEXT_STYLE_FLAGS textStyle = 0;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    std::vector<std::unique_ptr<GLYPH>> glyphs; // ignored
    VECTOR2D nextPosition = drawMarkup( aBoundingBox, glyphs, aText, aPosition, aSize,
                                        EDA_ANGLE::ANGLE_0, false, VECTOR2I(), textStyle );

    return nextPosition;
}
