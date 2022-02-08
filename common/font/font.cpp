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

// The "official" name of the building Kicad stroke font (always existing)
#include <font/kicad_font_name.h>


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
    if( !s_defaultFont )
        s_defaultFont = STROKE_FONT::LoadFont( wxEmptyString );

    return s_defaultFont;
}


FONT* FONT::GetFont( const wxString& aFontName, bool aBold, bool aItalic )
{
    if( aFontName.empty() || aFontName.StartsWith( KICAD_FONT_NAME ) )
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
    // This would need a more complex implementation if we ever support more stroke fonts
    // than the KiCad Font.
    return aFontName == _( "Default Font" ) || aFontName == KICAD_FONT_NAME;
}


void FONT::getLinePositions( const wxString& aText, const VECTOR2I& aPosition,
                             wxArrayString& aTextLines, std::vector<VECTOR2I>& aPositions,
                             std::vector<VECTOR2I>& aExtents, const TEXT_ATTRIBUTES& aAttrs ) const
{
    wxStringSplit( aText, aTextLines, '\n' );
    int lineCount = aTextLines.Count();
    aPositions.reserve( lineCount );

    int interline = GetInterline( aAttrs.m_Size.y, aAttrs.m_LineSpacing );
    int height = 0;

    for( int i = 0; i < lineCount; i++ )
    {
        VECTOR2I pos( aPosition.x, aPosition.y + i * interline );
        VECTOR2I end = boundingBoxSingleLine( nullptr, aTextLines[i], pos, aAttrs.m_Size,
                                              aAttrs.m_Italic );
        VECTOR2I bBox( end - pos );

        aExtents.push_back( bBox );

        if( i == 0 )
            height += aAttrs.m_Size.y;
        else
            height += interline;
    }

    VECTOR2I offset( 0, 0 );
    offset.y += aAttrs.m_Size.y;

    switch(  aAttrs.m_Valign )
    {
    case GR_TEXT_V_ALIGN_TOP:                            break;
    case GR_TEXT_V_ALIGN_CENTER: offset.y -= height / 2; break;
    case GR_TEXT_V_ALIGN_BOTTOM: offset.y -= height;     break;
    }

    for( int i = 0; i < lineCount; i++ )
    {
        VECTOR2I lineSize = aExtents.at( i );
        VECTOR2I lineOffset( offset );

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
void FONT::Draw( KIGFX::GAL* aGal, const wxString& aText, const VECTOR2I& aPosition,
                 const VECTOR2I& aCursor, const TEXT_ATTRIBUTES& aAttrs ) const
{
    if( !aGal || aText.empty() )
        return;

    VECTOR2I  position( aPosition - aCursor );

    // Split multiline strings into separate ones and draw them line by line
    wxArrayString         strings_list;
    std::vector<VECTOR2I> positions;
    std::vector<VECTOR2I> extents;

    getLinePositions( aText, position, strings_list, positions, extents, aAttrs );

    aGal->SetLineWidth( aAttrs.m_StrokeWidth );

    for( size_t i = 0; i < strings_list.GetCount(); i++ )
    {
        drawSingleLineText( aGal, nullptr, strings_list[i], positions[i], aAttrs.m_Size,
                            aAttrs.m_Angle, aAttrs.m_Mirrored, aPosition, aAttrs.m_Italic );
    }
}


/**
 * @return position of cursor for drawing next substring
 */
VECTOR2I drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                     const std::unique_ptr<MARKUP::NODE>& aNode, const VECTOR2I& aPosition,
                     const KIFONT::FONT* aFont, const VECTOR2I& aSize, const EDA_ANGLE& aAngle,
                     bool aMirror, const VECTOR2I& aOrigin, TEXT_STYLE_FLAGS aTextStyle )
{
    VECTOR2I nextPosition = aPosition;

    if( aNode ) {
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
    }

    return nextPosition;
}


VECTOR2I FONT::drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                           const wxString& aText, const VECTOR2I& aPosition, const VECTOR2I& aSize,
                           const EDA_ANGLE& aAngle, bool aMirror, const VECTOR2I& aOrigin,
                           TEXT_STYLE_FLAGS aTextStyle ) const
{
    MARKUP::MARKUP_PARSER         markupParser( aText.ToStdString() );
    std::unique_ptr<MARKUP::NODE> root = markupParser.Parse();

    return ::drawMarkup( aBoundingBox, aGlyphs, root, aPosition, this, aSize, aAngle, aMirror,
                         aOrigin, aTextStyle );
}


void FONT::drawSingleLineText( KIGFX::GAL* aGal, BOX2I* aBoundingBox, const wxString& aText,
                               const VECTOR2I& aPosition, const VECTOR2I& aSize,
                               const EDA_ANGLE& aAngle, bool aMirror, const VECTOR2I& aOrigin,
                               bool aItalic ) const
{
    if( !aGal )
        return;

    TEXT_STYLE_FLAGS textStyle = 0;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    std::vector<std::unique_ptr<GLYPH>> glyphs;

    (void) drawMarkup( aBoundingBox, &glyphs, aText, aPosition, aSize, aAngle, aMirror, aOrigin,
                       textStyle );

    for( const std::unique_ptr<GLYPH>& glyph : glyphs )
        aGal->DrawGlyph( *glyph.get() );
}


VECTOR2I FONT::StringBoundaryLimits( const wxString& aText, const VECTOR2I& aSize, int aThickness,
                                     bool aBold, bool aItalic ) const
{
    // TODO do we need to parse every time - have we already parsed?
    BOX2I            boundingBox;
    TEXT_STYLE_FLAGS textStyle = 0;

    if( aBold )
        textStyle |= TEXT_STYLE::BOLD;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    (void) drawMarkup( &boundingBox, nullptr, aText, VECTOR2I(), aSize, ANGLE_0, false,
                       VECTOR2I(), textStyle );

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


VECTOR2I FONT::boundingBoxSingleLine( BOX2I* aBBox, const wxString& aText,
                                      const VECTOR2I& aPosition, const VECTOR2I& aSize,
                                      bool aItalic ) const
{
    TEXT_STYLE_FLAGS textStyle = 0;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    VECTOR2I extents = drawMarkup( aBBox, nullptr, aText, aPosition, aSize, ANGLE_0, false,
                                   VECTOR2I(), textStyle );

    return extents;
}


/*
 * Break marked-up text into "words".  In this context, a "word" is EITHER a run of marked-up
 * text (subscript, superscript or overbar), OR a run of non-marked-up text separated by spaces.
 */
void wordbreakMarkup( std::vector<std::pair<wxString, int>>* aWords,
                      const std::unique_ptr<MARKUP::NODE>& aNode, const KIFONT::FONT* aFont,
                      const VECTOR2I& aSize, TEXT_STYLE_FLAGS aTextStyle )
{
    TEXT_STYLE_FLAGS textStyle = aTextStyle;

    if( !aNode->is_root() )
    {
        wxChar escapeChar = 0;

        if( aNode->isSubscript() )
        {
            escapeChar = '_';
            textStyle = TEXT_STYLE::SUBSCRIPT;
        }
        else if( aNode->isSuperscript() )
        {
            escapeChar = '^';
            textStyle = TEXT_STYLE::SUPERSCRIPT;
        }

        if( aNode->isOverbar() )
        {
            escapeChar = '~';
            textStyle |= TEXT_STYLE::OVERBAR;
        }

        if( escapeChar )
        {
            wxString word = wxString::Format( "%c{", escapeChar );
            int      width = 0;

            if( aNode->has_content() )
            {
                VECTOR2I next = aFont->GetTextAsGlyphs( nullptr, nullptr, aNode->string(), aSize,
                                                        {0,0}, ANGLE_0, false, {0,0}, textStyle );
                word += aNode->string();
                width += next.x;
            }

            std::vector<std::pair<wxString, int>> childWords;

            for( const std::unique_ptr<MARKUP::NODE>& child : aNode->children )
                wordbreakMarkup( &childWords, child, aFont, aSize, textStyle );

            for( const std::pair<wxString, int>& childWord : childWords )
            {
                word += childWord.first;
                width += childWord.second;
            }

            word += "}";
            aWords->emplace_back( std::make_pair( word, width ) );
            return;
        }
        else
        {
            wxString      space( wxS( " " ) );
            wxString      textRun( aNode->string() );
            wxArrayString words;

            wxStringSplit( textRun, words, ' ' );

            if( textRun.EndsWith( wxS( " " ) ) )
                words.Add( wxS( " " ) );

            for( size_t ii = 0; ii < words.size(); ++ii )
            {
                int w = aFont->GetTextAsGlyphs( nullptr, nullptr, words[ii], aSize, { 0, 0 },
                                                ANGLE_0, false, { 0, 0 }, textStyle ).x;

                aWords->emplace_back( std::make_pair( words[ii], w ) );
            }
        }
    }

    for( const std::unique_ptr<MARKUP::NODE>& child : aNode->children )
        wordbreakMarkup( aWords, child, aFont, aSize, textStyle );
}


void FONT::wordbreakMarkup( std::vector<std::pair<wxString, int>>* aWords, const wxString& aText,
                            const VECTOR2I& aSize, TEXT_STYLE_FLAGS aTextStyle ) const
{
    MARKUP::MARKUP_PARSER         markupParser( aText.ToStdString() );
    std::unique_ptr<MARKUP::NODE> root = markupParser.Parse();

    ::wordbreakMarkup( aWords, root, this, aSize, aTextStyle );
}


/*
 * This is a highly simplified line-breaker.  KiCad is an EDA tool, not a word processor.
 *
 * 1) It breaks only on spaces.  If you type a word wider than the column width then you get
 *    overflow.
 * 2) It treats runs of formatted text (superscript, subscript, overbar) as single words.
 * 3) It does not perform justification.
 *
 * The results of the linebreaking are the addition of \n in the text.  It is presumed that this
 * function is called on m_shownText (or equivalent) rather than the original source text.
 */
void FONT::LinebreakText( wxString& aText, int aColumnWidth, const VECTOR2I& aSize, int aThickness,
                          bool aBold, bool aItalic ) const
{
    TEXT_STYLE_FLAGS textStyle = 0;

    if( aBold )
        textStyle |= TEXT_STYLE::BOLD;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    int spaceWidth = GetTextAsGlyphs( nullptr, nullptr, wxS( " " ), aSize, VECTOR2I(), ANGLE_0,
                                      false, VECTOR2I(), textStyle ).x;

    wxArrayString  textLines;
    wxStringSplit( aText, textLines, '\n' );

    aText = wxEmptyString;

    for( size_t ii = 0; ii < textLines.Count(); ++ii )
    {
        int lineWidth = 0;
        std::vector<std::pair<wxString, int>> words;

        wordbreakMarkup( &words, textLines[ii], aSize, textStyle );

        for( size_t jj = 0; jj < words.size(); /* advance in loop */ )
        {
            if( lineWidth == 0
                    || lineWidth + spaceWidth + words[jj].second < aColumnWidth - aThickness )
            {
                if( lineWidth > 0 )
                {
                    aText += " ";
                    lineWidth += spaceWidth;
                }
            }
            else if( lineWidth > 0 )
            {
                aText += '\n';
                lineWidth = 0;
                continue;
            }
            else
            {
                // TODO: Would we want to further split the words into characters when it doesn't fit
                // in the column width? For now just return the full word even if it doesn't fit
                // to avoid an infinite loop.
            }

            aText += words[jj].first;
            lineWidth += words[jj].second;

            jj++;
        }

        // Add the newlines back onto the string
        if( ii != ( textLines.Count() - 1 ) )
            aText += '\n';
    }
}


