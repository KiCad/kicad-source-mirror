/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <list>
#include <mutex>
#include <unordered_map>

#include <macros.h>
#include <string_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <font/stroke_font.h>
#include <font/outline_font.h>
#include <trigo.h>
#include <markup_parser.h>

// The "official" name of the Kicad stroke font (always existing)
#include <font/kicad_font_name.h>
#include <wx/tokenzr.h>

// markup_parser.h includes pegtl.hpp which includes windows.h... which leaks #define DrawText
#undef DrawText


using namespace KIFONT;

METRICS g_defaultMetrics;


const METRICS& METRICS::Default()
{
    return g_defaultMetrics;
}


FONT* FONT::s_defaultFont = nullptr;

std::map< std::tuple<wxString, bool, bool, bool>, FONT*> FONT::s_fontMap;

class MARKUP_CACHE
{
public:
    struct ENTRY
    {
        std::string source;
        std::unique_ptr<MARKUP::NODE> root;
    };

    MARKUP_CACHE( size_t aMaxSize ) :
            m_maxSize( aMaxSize )
    {
    }

    ENTRY& Put( const wxString& aQuery, ENTRY&& aResult )
    {
        auto it = m_cache.find( aQuery );

        m_cacheMru.emplace_front( std::make_pair( aQuery, std::move( aResult ) ) );

        if( it != m_cache.end() )
        {
            m_cacheMru.erase( it->second );
            m_cache.erase( it );
        }

        m_cache[aQuery] = m_cacheMru.begin();

        if( m_cache.size() > m_maxSize )
        {
            auto last = m_cacheMru.end();
            last--;
            m_cache.erase( last->first );
            m_cacheMru.pop_back();
        }

        return m_cacheMru.begin()->second;
    }

    ENTRY* Get( const wxString& aQuery )
    {
        auto it = m_cache.find( aQuery );

        if( it == m_cache.end() )
            return nullptr;

        m_cacheMru.splice( m_cacheMru.begin(), m_cacheMru, it->second );

        return &m_cacheMru.begin()->second;
    }

    void Clear()
    {
        m_cacheMru.clear();
        m_cache.clear();
    }

private:
    size_t                                                                        m_maxSize;
    std::list<std::pair<wxString, ENTRY>>                                         m_cacheMru;
    std::unordered_map<wxString, std::list<std::pair<wxString, ENTRY>>::iterator> m_cache;
};


static MARKUP_CACHE s_markupCache( 1024 );
static std::mutex s_markupCacheMutex;
static std::mutex s_defaultFontMutex;;


FONT::FONT()
{
}


FONT* FONT::getDefaultFont()
{
    std::lock_guard lock( s_defaultFontMutex );

    if( !s_defaultFont )
        s_defaultFont = STROKE_FONT::LoadFont( wxEmptyString );

    return s_defaultFont;
}


FONT* FONT::GetFont( const wxString& aFontName, bool aBold, bool aItalic,
                     const std::vector<wxString>* aEmbeddedFiles, bool aForDrawingSheet )
{
    if( aFontName.empty() || aFontName.StartsWith( KICAD_FONT_NAME ) )
        return getDefaultFont();

    std::tuple<wxString, bool, bool, bool> key = { aFontName, aBold, aItalic, aForDrawingSheet };

    FONT* font = nullptr;

    if( s_fontMap.find( key ) != s_fontMap.end() )
        font = s_fontMap[key];

    if( !font )
        font = OUTLINE_FONT::LoadFont( aFontName, aBold, aItalic, aEmbeddedFiles,
                                       aForDrawingSheet );

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
                             std::vector<VECTOR2I>& aExtents, const TEXT_ATTRIBUTES& aAttrs,
                             const METRICS& aFontMetrics ) const
{
    wxStringSplit( aText, aTextLines, '\n' );
    int lineCount = aTextLines.Count();
    aPositions.reserve( lineCount );

    int interline = GetInterline( aAttrs.m_Size.y, aFontMetrics ) * aAttrs.m_LineSpacing;
    int height = 0;

    for( int i = 0; i < lineCount; i++ )
    {
        VECTOR2I pos( aPosition.x, aPosition.y + i * interline );
        VECTOR2I end = boundingBoxSingleLine( nullptr, aTextLines[i], pos, aAttrs.m_Size,
                                              aAttrs.m_Italic, aFontMetrics );
        VECTOR2I bBox( end - pos );

        aExtents.push_back( bBox );

        if( i == 0 )
            height += ( aAttrs.m_Size.y * 1.17 );   // 1.17 is a fudge to match 6.0 positioning
        else
            height += interline;
    }

    VECTOR2I offset( 0, 0 );
    offset.y += aAttrs.m_Size.y;

    if( IsStroke() )
    {
        // Fudge factors to match 6.0 positioning
        offset.x += aAttrs.m_StrokeWidth / 1.52;
        offset.y -= aAttrs.m_StrokeWidth * 0.052;
    }

    switch(  aAttrs.m_Valign )
    {
    case GR_TEXT_V_ALIGN_TOP:                            break;
    case GR_TEXT_V_ALIGN_CENTER: offset.y -= height / 2; break;
    case GR_TEXT_V_ALIGN_BOTTOM: offset.y -= height;     break;
    case GR_TEXT_V_ALIGN_INDETERMINATE:
        wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
        break;
    }

    for( int i = 0; i < lineCount; i++ )
    {
        VECTOR2I lineSize = aExtents.at( i );
        VECTOR2I lineOffset( offset );

        lineOffset.y += i * interline;

        switch( aAttrs.m_Halign )
        {
        case GR_TEXT_H_ALIGN_LEFT:                                              break;
        case GR_TEXT_H_ALIGN_CENTER: lineOffset.x = -lineSize.x / 2;            break;
        case GR_TEXT_H_ALIGN_RIGHT:  lineOffset.x = -( lineSize.x + offset.x ); break;
        case GR_TEXT_H_ALIGN_INDETERMINATE:
            wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
            break;
        }

        aPositions.push_back( aPosition + lineOffset );
    }
}


void FONT::Draw( KIGFX::GAL* aGal, const wxString& aText, const VECTOR2I& aPosition, const VECTOR2I& aCursor,
                 const TEXT_ATTRIBUTES& aAttrs, const METRICS& aFontMetrics, std::optional<VECTOR2I> aMousePos,
                 wxString* aActiveUrl ) const
{
    if( !aGal || aText.empty() )
        return;

    VECTOR2I  position( aPosition - aCursor );

    // Split multiline strings into separate ones and draw them line by line
    wxArrayString         strings_list;
    std::vector<VECTOR2I> positions;
    std::vector<VECTOR2I> extents;

    getLinePositions( aText, position, strings_list, positions, extents, aAttrs, aFontMetrics );

    aGal->SetLineWidth( aAttrs.m_StrokeWidth );

    for( size_t i = 0; i < strings_list.GetCount(); i++ )
    {
        drawSingleLineText( aGal, nullptr, strings_list[i], positions[i], aAttrs.m_Size, aAttrs.m_Angle,
                            aAttrs.m_Mirrored, aPosition, aAttrs.m_Italic, aAttrs.m_Underlined, aAttrs.m_Hover,
                            aFontMetrics, aMousePos, aActiveUrl );
    }
}


/**
 * @return position of cursor for drawing next substring.
 */
VECTOR2I drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs, const MARKUP::NODE* aNode,
                     const VECTOR2I& aPosition, const KIFONT::FONT* aFont, const VECTOR2I& aSize,
                     const EDA_ANGLE& aAngle, bool aMirror, const VECTOR2I& aOrigin, TEXT_STYLE_FLAGS aTextStyle,
                     const METRICS& aFontMetrics, std::optional<VECTOR2I> aMousePos, wxString* aActiveUrl )
{
    VECTOR2I nextPosition = aPosition;
    bool     drawUnderline = false;
    bool     drawOverbar = false;
    bool     useHoverColor = false;
    int      start = aGlyphs ? (int) aGlyphs->size() : 0;


    if( aNode )
    {
        TEXT_STYLE_FLAGS textStyle = aTextStyle;

        if( !aNode->is_root() )
        {
            if( aNode->isSubscript() )
                textStyle |= TEXT_STYLE::SUBSCRIPT;
            else if( aNode->isSuperscript() )
                textStyle |= TEXT_STYLE::SUPERSCRIPT;

            if( aNode->isOverbar() )
                drawOverbar = true;

            if( aTextStyle & TEXT_STYLE::UNDERLINE )
                drawUnderline = true;

            if( aNode->has_content() )
            {
                BOX2I bbox;

                nextPosition = aFont->GetTextAsGlyphs( &bbox, aGlyphs, aNode->asWxString(), aSize,
                                                       nextPosition, aAngle, aMirror, aOrigin, textStyle );

                if( aBoundingBox )
                    aBoundingBox->Merge( bbox );

                if( aNode->isURL() && aMousePos.has_value() && bbox.Contains( aMousePos.value() ) )
                {
                    useHoverColor = true;
                    drawUnderline = true;
                }
            }
        }

        for( const std::unique_ptr<MARKUP::NODE>& child : aNode->children )
        {
            nextPosition = drawMarkup( aBoundingBox, aGlyphs, child.get(), nextPosition, aFont, aSize,
                                       aAngle, aMirror, aOrigin, textStyle, aFontMetrics, aMousePos,
                                       aActiveUrl );
        }
    }

    if( drawUnderline )
    {
        // Shorten the bar a little so its rounded ends don't make it over-long
        double barTrim = aSize.x * 0.1;
        double barOffset = aFontMetrics.GetUnderlineVerticalPosition( aSize.y );

        VECTOR2D barStart( aPosition.x + barTrim, aPosition.y - barOffset );
        VECTOR2D barEnd( nextPosition.x - barTrim, nextPosition.y - barOffset );

        if( aGlyphs )
        {
            STROKE_GLYPH barGlyph;

            barGlyph.AddPoint( barStart );
            barGlyph.AddPoint( barEnd );
            barGlyph.Finalize();

            aGlyphs->push_back( barGlyph.Transform( { 1.0, 1.0 }, { 0, 0 }, false, aAngle, aMirror,
                                                    aOrigin ) );
        }
    }

    if( drawOverbar )
    {
        // Shorten the bar a little so its rounded ends don't make it over-long
        double barTrim = aSize.x * 0.1;
        double barOffset = aFontMetrics.GetOverbarVerticalPosition( aSize.y );

        VECTOR2D barStart( aPosition.x + barTrim, aPosition.y - barOffset );
        VECTOR2D barEnd( nextPosition.x - barTrim, nextPosition.y - barOffset );

        if( aGlyphs )
        {
            STROKE_GLYPH barGlyph;

            barGlyph.AddPoint( barStart );
            barGlyph.AddPoint( barEnd );
            barGlyph.Finalize();

            aGlyphs->push_back( barGlyph.Transform( { 1.0, 1.0 }, { 0, 0 }, false, aAngle, aMirror,
                                                    aOrigin ) );
        }
    }

    if( useHoverColor )
    {
        for( int ii = start; ii < (int) aGlyphs->size(); ++ii )
            (*aGlyphs)[ii]->SetIsHover( true );

        if( aActiveUrl && aActiveUrl->IsEmpty() )
            *aActiveUrl = aNode->asWxString();
    }

    return nextPosition;
}


VECTOR2I FONT::drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                           const wxString& aText, const VECTOR2I& aPosition, const VECTOR2I& aSize,
                           const EDA_ANGLE& aAngle, bool aMirror, const VECTOR2I& aOrigin,
                           TEXT_STYLE_FLAGS aTextStyle, const METRICS& aFontMetrics,
                           std::optional<VECTOR2I> aMousePos, wxString* aActiveUrl ) const
{
    std::lock_guard<std::mutex> lock( s_markupCacheMutex );

    MARKUP_CACHE::ENTRY* markup = s_markupCache.Get( aText );

    if( !markup || !markup->root )
    {
        MARKUP_CACHE::ENTRY& cached = s_markupCache.Put( aText, {} );

        cached.source = TO_UTF8( aText );
        MARKUP::MARKUP_PARSER markupParser( &cached.source );
        cached.root = markupParser.Parse();
        markup = &cached;
    }

    wxASSERT( markup && markup->root );

    return ::drawMarkup( aBoundingBox, aGlyphs, markup->root.get(), aPosition, this, aSize, aAngle,
                         aMirror, aOrigin, aTextStyle, aFontMetrics, aMousePos, aActiveUrl );
}


void FONT::drawSingleLineText( KIGFX::GAL* aGal, BOX2I* aBoundingBox, const wxString& aText,
                               const VECTOR2I& aPosition, const VECTOR2I& aSize, const EDA_ANGLE& aAngle,
                               bool aMirror, const VECTOR2I& aOrigin, bool aItalic, bool aUnderline,
                               bool aHover, const METRICS& aFontMetrics, std::optional<VECTOR2I> aMousePos,
                               wxString* aActiveUrl ) const
{
    if( !aGal )
        return;

    TEXT_STYLE_FLAGS textStyle = 0;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    if( aUnderline )
        textStyle |= TEXT_STYLE::UNDERLINE;

    std::vector<std::unique_ptr<GLYPH>> glyphs;

    (void) drawMarkup( aBoundingBox, &glyphs, aText, aPosition, aSize, aAngle, aMirror, aOrigin,
                       textStyle, aFontMetrics, aMousePos, aActiveUrl );

    if( aHover )
    {
        for( std::unique_ptr<GLYPH>& glyph : glyphs )
            glyph->SetIsHover( true );
    }

    aGal->DrawGlyphs( glyphs );
}


VECTOR2I FONT::StringBoundaryLimits( const wxString& aText, const VECTOR2I& aSize, int aThickness,
                                     bool aBold, bool aItalic, const METRICS& aFontMetrics ) const
{
    // TODO do we need to parse every time - have we already parsed?
    BOX2I            boundingBox;
    TEXT_STYLE_FLAGS textStyle = 0;

    if( aBold )
        textStyle |= TEXT_STYLE::BOLD;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    (void) drawMarkup( &boundingBox, nullptr, aText, VECTOR2I(), aSize, ANGLE_0, false, VECTOR2I(),
                       textStyle, aFontMetrics );

    if( IsStroke() )
    {
        // Inflate by a bit more than thickness/2 to catch diacriticals, descenders, etc.
        boundingBox.Inflate( KiROUND( aThickness * 1.5 ) );
    }
    else if( IsOutline() )
    {
        // Outline fonts have thickness built in, and *usually* stay within their ascent/descent
    }

    return boundingBox.GetSize();
}


VECTOR2I FONT::boundingBoxSingleLine( BOX2I* aBBox, const wxString& aText,
                                      const VECTOR2I& aPosition, const VECTOR2I& aSize,
                                      bool aItalic, const METRICS& aFontMetrics ) const
{
    TEXT_STYLE_FLAGS textStyle = 0;

    if( aItalic )
        textStyle |= TEXT_STYLE::ITALIC;

    VECTOR2I extents = drawMarkup( aBBox, nullptr, aText, aPosition, aSize, ANGLE_0, false,
                                   VECTOR2I(), textStyle, aFontMetrics );

    return extents;
}


/**
 * Break marked-up text into "words".
 *
 * In this context, a "word" is EITHER a run of marked-up text (subscript, superscript or
 * overbar), OR a run of non-marked-up text separated by spaces.
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
            wxString word = wxString::Format( wxT( "%c{" ), escapeChar );
            int      width = 0;

            if( aNode->has_content() )
            {
                VECTOR2I next = aFont->GetTextAsGlyphs( nullptr, nullptr, aNode->asWxString(),
                                                        aSize, { 0, 0 }, ANGLE_0, false, { 0, 0 },
                                                        textStyle );
                word += aNode->asWxString();
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

            word += wxT( "}" );
            aWords->emplace_back( std::make_pair( word, width ) );
            return;
        }
        else
        {
            wxString              textRun = aNode->asWxString();
            wxStringTokenizer     tokenizer( textRun, " ", wxTOKEN_RET_DELIMS );
            std::vector<wxString> words;

            while( tokenizer.HasMoreTokens() )
                words.emplace_back( tokenizer.GetNextToken() );

            for( const wxString& word : words )
            {
                wxString chars = word;
                chars.Trim();

                int w = aFont->GetTextAsGlyphs( nullptr, nullptr, chars, aSize, { 0, 0 },
                                                ANGLE_0, false, { 0, 0 }, textStyle ).x;

                aWords->emplace_back( std::make_pair( word, w ) );
            }
        }
    }

    for( const std::unique_ptr<MARKUP::NODE>& child : aNode->children )
        wordbreakMarkup( aWords, child, aFont, aSize, textStyle );
}


void FONT::wordbreakMarkup( std::vector<std::pair<wxString, int>>* aWords, const wxString& aText,
                            const VECTOR2I& aSize, TEXT_STYLE_FLAGS aTextStyle ) const
{
    MARKUP::MARKUP_PARSER         markupParser( TO_UTF8( aText ) );
    std::unique_ptr<MARKUP::NODE> root = markupParser.Parse();

    ::wordbreakMarkup( aWords, root, this, aSize, aTextStyle );
}


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
        std::vector<std::pair<wxString, int>> markup;
        std::vector<std::pair<wxString, int>> words;

        wordbreakMarkup( &markup, textLines[ii], aSize, textStyle );

        for( const auto& [ run, runWidth ] : markup )
        {
            if( !words.empty() && !words.back().first.EndsWith( ' ' ) )
            {
                words.back().first += run;
                words.back().second += runWidth;
            }
            else
            {
                words.emplace_back( std::make_pair( run, runWidth ) );
            }
        }

        bool     buryMode = false;
        int      lineWidth = 0;
        wxString pendingSpaces;

        for( const auto& [ word, wordWidth ] : words )
        {
            int  pendingSpaceWidth = (int) pendingSpaces.Length() * spaceWidth;
            bool overflow = lineWidth + pendingSpaceWidth + wordWidth > aColumnWidth - aThickness;

            if( overflow && pendingSpaces.Length() > 0 )
            {
                aText += '\n';
                lineWidth = 0;
                pendingSpaces = wxEmptyString;
                pendingSpaceWidth = 0;
                buryMode = true;
            }

            if( word == wxS( " " ) )
            {
                pendingSpaces += word;
            }
            else
            {
                if( buryMode )
                {
                    buryMode = false;
                }
                else
                {
                    aText += pendingSpaces;
                    lineWidth += pendingSpaceWidth;
                }

                if( word.EndsWith( ' ' ) )
                {
                    aText += word.Left( word.Length() - 1 );
                    pendingSpaces = wxS( " " );
                }
                else
                {
                    aText += word;
                    pendingSpaces = wxEmptyString;
                }

                lineWidth += wordWidth;
            }
        }

        // Add the newlines back onto the string
        if( ii != ( textLines.Count() - 1 ) )
            aText += '\n';
    }
}


