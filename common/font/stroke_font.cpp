/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <gal/graphics_abstraction_layer.h>
#include <wx/string.h>
#include <wx/textfile.h>
#include <newstroke_font.h>
#include <font/glyph.h>
#include <font/stroke_font.h>
#include <geometry/shape_line_chain.h>
#include <trigo.h>

// The "official" name of the building Kicad stroke font (always existing)
#include <font/kicad_font_name.h>

#include <mutex>

using namespace KIFONT;


/// Scale factor for a glyph.
static constexpr double STROKE_FONT_SCALE = 1.0 / 21.0;

/// Offset (in stroke font units) to move the origin to the baseline.
static constexpr int FONT_OFFSET = -8;


bool                                g_defaultFontInitialized = false;
std::vector<std::shared_ptr<GLYPH>> g_defaultFontGlyphs;
std::vector<BOX2D>*                 g_defaultFontGlyphBoundingBoxes;
std::mutex                          g_defaultFontLoadMutex;


STROKE_FONT::STROKE_FONT() :
        m_glyphs( nullptr ),
        m_glyphBoundingBoxes( nullptr ),
        m_maxGlyphWidth( 0.0 )
{
}


STROKE_FONT* STROKE_FONT::LoadFont( const wxString& aFontName )
{
    if( aFontName.empty() )
    {
        STROKE_FONT* font = new STROKE_FONT();
        font->loadNewStrokeFont( newstroke_font, newstroke_font_bufsize );
        return font;
    }
    else
    {
        // FONT TODO: support for other stroke fonts?
        return nullptr;
    }
}


void buildGlyphBoundingBox( std::shared_ptr<STROKE_GLYPH>& aGlyph, double aGlyphWidth )
{
    VECTOR2D min( 0, 0 );
    VECTOR2D max( aGlyphWidth, 0 );

    for( const std::vector<VECTOR2D>& pointList : *aGlyph )
    {
        for( const VECTOR2D& point : pointList )
        {
            min.y = std::min( min.y, point.y );
            max.y = std::max( max.y, point.y );
        }
    }

    aGlyph->SetBoundingBox( BOX2D( min, max - min ) );
}


void STROKE_FONT::loadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize )
{
    // Protect the initialization sequence against multiple entries
    std::lock_guard<std::mutex> lock( g_defaultFontLoadMutex );

    if( !g_defaultFontInitialized )
    {
        g_defaultFontGlyphs.reserve( aNewStrokeFontSize );

        g_defaultFontGlyphBoundingBoxes = new std::vector<BOX2D>;
        g_defaultFontGlyphBoundingBoxes->reserve( aNewStrokeFontSize );

        for( int j = 0; j < aNewStrokeFontSize; j++ )
        {
            std::shared_ptr<STROKE_GLYPH> glyph = std::make_shared<STROKE_GLYPH>();

            double glyphStartX = 0.0;
            double glyphEndX = 0.0;
            double glyphWidth = 0.0;
            int    strokes = 0;
            int    i = 0;

            while( aNewStrokeFont[j][i] )
            {
                if( aNewStrokeFont[j][i] == ' ' && aNewStrokeFont[j][i+1] == 'R' )
                    strokes++;

                i += 2;
            }

            glyph->reserve( strokes + 1 );

            i = 0;

            while( aNewStrokeFont[j][i] )
            {
                VECTOR2D point( 0.0, 0.0 );
                char     coordinate[2] = { 0, };

                for( int k : { 0, 1 } )
                    coordinate[k] = aNewStrokeFont[j][i + k];

                if( i < 2 )
                {
                    // The first two values contain the width of the char
                    glyphStartX = ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE;
                    glyphEndX   = ( coordinate[1] - 'R' ) * STROKE_FONT_SCALE;
                    glyphWidth  = glyphEndX - glyphStartX;
                }
                else if( ( coordinate[0] == ' ' ) && ( coordinate[1] == 'R' ) )
                {
                    glyph->RaisePen();
                }
                else
                {
                    // In stroke font, coordinates values are coded as <value> + 'R', where
                    // <value> is an ASCII char.
                    // therefore every coordinate description of the Hershey format has an offset,
                    // it has to be subtracted
                    // Note:
                    //  * the stroke coordinates are stored in reduced form (-1.0 to +1.0),
                    //    and the actual size is stroke coordinate * glyph size
                    //  * a few shapes have a height slightly bigger than 1.0 ( like '{' '[' )
                    point.x = (double) ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE - glyphStartX;

                    // FONT_OFFSET is here for historical reasons, due to the way the stroke font
                    // was built. It allows shapes coordinates like W M ... to be >= 0
                    // Only shapes like j y have coordinates < 0
                    point.y = (double) ( coordinate[1] - 'R' + FONT_OFFSET ) * STROKE_FONT_SCALE;

                    glyph->AddPoint( point );
                }

                i += 2;
            }

            glyph->Finalize();

            // Compute the bounding box of the glyph
            buildGlyphBoundingBox( glyph, glyphWidth );
            g_defaultFontGlyphBoundingBoxes->emplace_back( glyph->BoundingBox() );
            g_defaultFontGlyphs.push_back( glyph );
            m_maxGlyphWidth = std::max( m_maxGlyphWidth, glyphWidth );
        }

        g_defaultFontInitialized = true;
    }

    m_glyphs = &g_defaultFontGlyphs;
    m_glyphBoundingBoxes = g_defaultFontGlyphBoundingBoxes;
    m_fontName = KICAD_FONT_NAME;
    m_fontFileName = wxEmptyString;
}


double STROKE_FONT::GetInterline( double aGlyphHeight, const METRICS& aFontMetrics ) const
{
    static double LEGACY_FACTOR = 0.9583;   // Adjustment to match legacy spacing

    return aFontMetrics.GetInterline( aGlyphHeight ) * LEGACY_FACTOR;
}


VECTOR2I STROKE_FONT::GetTextAsGlyphs( BOX2I* aBBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                       const wxString& aText, const VECTOR2I& aSize,
                                       const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                       bool aMirror, const VECTOR2I& aOrigin,
                                       TEXT_STYLE_FLAGS aTextStyle ) const
{
    constexpr int    TAB_WIDTH = 4;
    constexpr double INTER_CHAR = 0.2;
    constexpr double SUPER_SUB_SIZE_MULTIPLIER = 0.8;
    constexpr double SUPER_HEIGHT_OFFSET = 0.35;
    constexpr double SUB_HEIGHT_OFFSET = 0.15;

    VECTOR2I cursor( aPosition );
    VECTOR2D glyphSize( aSize );
    double   tilt = ( aTextStyle & TEXT_STYLE::ITALIC ) ? ITALIC_TILT : 0.0;
    double   space_width = m_glyphBoundingBoxes->front().GetWidth(); // First char is space
    int      char_count = 0;

    if( aTextStyle & TEXT_STYLE::SUBSCRIPT || aTextStyle & TEXT_STYLE::SUPERSCRIPT )
    {
        glyphSize = glyphSize * SUPER_SUB_SIZE_MULTIPLIER;

        if( aTextStyle & TEXT_STYLE::SUBSCRIPT )
            cursor.y += glyphSize.y * SUB_HEIGHT_OFFSET;
        else
            cursor.y -= glyphSize.y * SUPER_HEIGHT_OFFSET;
    }

    for( wxUniChar c : aText )
    {
        // Handle tabs as locked to the next 4th column (in base-widths).
        if( c == '\t' )
        {
            char_count = ( char_count / TAB_WIDTH + 1 ) * TAB_WIDTH - 1;

            int new_cursor = aPosition.x + aSize.x * char_count
                    + aSize.x * space_width;

            while( new_cursor <= cursor.x )
            {
                char_count += TAB_WIDTH;
                new_cursor += aSize.x * TAB_WIDTH;
            }

            cursor.x = new_cursor;
        }
        else if( c == ' ' )
        {
            // 'space' character - draw nothing, advance cursor position
            cursor.x += KiROUND( glyphSize.x * space_width );
        }
        else
        {
            // dd is the index into bounding boxes table
            int dd = (signed) c - ' ';

            // Filtering non existing glyphs and non printable chars
            if( dd < 0 || dd >= (int) m_glyphBoundingBoxes->size() )
            {
                c = '?';
                dd = (signed) c - ' ';
            }

            STROKE_GLYPH* source = static_cast<STROKE_GLYPH*>( m_glyphs->at( dd ).get() );

            if( aGlyphs )
            {
                aGlyphs->push_back( source->Transform( glyphSize, cursor, tilt, aAngle, aMirror,
                                                       aOrigin ) );
            }

            VECTOR2D glyphExtents = source->BoundingBox().GetEnd();

            glyphExtents *= glyphSize;

            cursor.x += KiROUND( glyphExtents.x );
        }

        ++char_count;
    }

    if( aBBox )
    {
        aBBox->SetOrigin( aPosition );
        aBBox->SetEnd( cursor.x - KiROUND( glyphSize.x * INTER_CHAR ), cursor.y - glyphSize.y );
        aBBox->Normalize();
    }

    return VECTOR2I( cursor.x, aPosition.y );
}


unsigned STROKE_FONT::GetGlyphCount() const
{
    return m_glyphs ? m_glyphs->size() : 0;
}


const STROKE_GLYPH* STROKE_FONT::GetGlyph( unsigned aIndex ) const
{
    if( !m_glyphs || aIndex >= m_glyphs->size() )
        return nullptr;

    return static_cast<const STROKE_GLYPH*>( m_glyphs->at( aIndex ).get() );
}


const BOX2D& STROKE_FONT::GetGlyphBoundingBox( unsigned aIndex ) const
{
    static const BOX2D empty;

    if( !m_glyphBoundingBoxes || aIndex >= m_glyphBoundingBoxes->size() )
        return empty;

    return m_glyphBoundingBoxes->at( aIndex );
}
