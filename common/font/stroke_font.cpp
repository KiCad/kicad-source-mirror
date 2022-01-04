/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2016-2022 Kicad Developers, see change_log.txt for contributors.
 *
 * Stroke font class
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

using namespace KIFONT;


///< Factor that determines relative vertical position of the overbar.
static constexpr double OVERBAR_POSITION_FACTOR = 1.33;

///< Scale factor for a glyph
static constexpr double STROKE_FONT_SCALE = 1.0 / 21.0;

///< Tilt factor for italic style (this is the scaling factor on dY relative coordinates to
///< give a tilted shape)
static constexpr double ITALIC_TILT = 1.0 / 8;

static constexpr int FONT_OFFSET = -10;


bool                                g_defaultFontInitialized = false;
std::vector<std::shared_ptr<GLYPH>> g_defaultFontGlyphs;
std::vector<BOX2D>*                 g_defaultFontGlyphBoundingBoxes;


STROKE_FONT::STROKE_FONT() :
        m_glyphs( nullptr ),
        m_glyphBoundingBoxes( nullptr )
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
    if( !g_defaultFontInitialized )
    {
        g_defaultFontGlyphs.reserve( aNewStrokeFontSize );

        g_defaultFontGlyphBoundingBoxes = new std::vector<BOX2D>;
        g_defaultFontGlyphBoundingBoxes->reserve( aNewStrokeFontSize );

        for( int j = 0; j < aNewStrokeFontSize; j++ )
        {
            auto   glyph = std::make_shared<STROKE_GLYPH>();
            double glyphStartX = 0.0;
            double glyphEndX = 0.0;
            double glyphWidth = 0.0;

            std::vector<VECTOR2D>* pointList = nullptr;

            int i = 0;

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

                    if( !pointList )
                        pointList = new std::vector<VECTOR2D>;

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
}


double STROKE_FONT::GetInterline( double aGlyphHeight, double aLineSpacing ) const
{
    // Do not add the glyph thickness to the interline.  This makes bold text line-spacing
    // different from normal text, which is poor typography.
    return ( aGlyphHeight * aLineSpacing * INTERLINE_PITCH_RATIO );
}


double STROKE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
    return aGlyphHeight * OVERBAR_POSITION_FACTOR;
}


VECTOR2D STROKE_FONT::ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const
{
    //TODO default glyph size (and line width) is a guess
    VECTOR2D glyphSize = aGal ? aGal->GetGlyphSize() : VECTOR2D( 16.0, 16.0 );
    double   lineWidth = aGal ? aGal->GetLineWidth() : 1.0;
    return StringBoundaryLimits( aGal, aText, glyphSize, lineWidth );
}


VECTOR2D STROKE_FONT::StringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
                                            const VECTOR2D& aGlyphSize,
                                            double          aGlyphThickness ) const
{
    // TODO do we need to parse every time - have we already parsed?
    std::vector<std::unique_ptr<GLYPH>> glyphs; // ignored
    BOX2I                               boundingBox;

    (void) drawMarkup( &boundingBox, glyphs, aText, VECTOR2D(), aGlyphSize, EDA_ANGLE::ANGLE_0,
                       0 /* TODO: this should really include TEXT_STYLE::ITALIC if set */ );

    return boundingBox.GetSize();
}


VECTOR2D STROKE_FONT::getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                                      TEXT_STYLE_FLAGS aTextStyle ) const
{
    // TODO: take glyph thickness into account!
    return StringBoundaryLimits( nullptr, aString, aGlyphSize, 0 );
}


VECTOR2I STROKE_FONT::GetTextAsGlyphs( BOX2I* aBoundingBox,
                                       std::vector<std::unique_ptr<GLYPH>>& aGlyphs,
                                       const UTF8& aText, const VECTOR2D& aGlyphSize,
                                       const wxPoint& aPosition, const EDA_ANGLE& aAngle,
                                       TEXT_STYLE_FLAGS aTextStyle ) const
{
    wxPoint  cursor( aPosition );
    VECTOR2D glyphSize( aGlyphSize );
    double   tilt = ( aTextStyle & TEXT_STYLE::ITALIC ) ? ITALIC_TILT : 0.0;

    if( aTextStyle & TEXT_STYLE::SUBSCRIPT || aTextStyle & TEXT_STYLE::SUPERSCRIPT )
    {
        constexpr double subscriptSuperscriptMultiplier = 0.7;
        glyphSize.x *= subscriptSuperscriptMultiplier;
        glyphSize.y *= subscriptSuperscriptMultiplier;

        if( aTextStyle & TEXT_STYLE::SUBSCRIPT )
        {
            constexpr double subscriptVerticalMultiplier = 0.3;
            cursor.y += glyphSize.y * subscriptVerticalMultiplier;
        }
        else
        {
            constexpr double superscriptVerticalMultiplier = 0.5;
            cursor.y -= glyphSize.y * superscriptVerticalMultiplier;
        }
    }

    for( UTF8::uni_iter i = aText.ubegin(), end = aText.uend(); i < end; ++i )
    {
        // Index into bounding boxes table
        int dd = (signed) *i - ' ';

        if( dd >= (int) m_glyphBoundingBoxes->size() || dd < 0 )
        {
            switch( *i )
            {
            case '\t':
                // TAB->SPACE
                dd = 0;
                break;
            default:
                // everything else is turned into a '?'
                dd = '?' - ' ';
            }
        }

        if( dd == 0 )
        {
            // 'space' character - draw nothing, advance cursor position
            constexpr double spaceAdvance = 0.6;
            cursor.x += glyphSize.x * spaceAdvance;
        }
        else
        {
            STROKE_GLYPH* source = static_cast<STROKE_GLYPH*>( m_glyphs->at( dd ).get() );

            aGlyphs.push_back( source->Transform( glyphSize, cursor, tilt ) );

            cursor.x = aGlyphs.back()->BoundingBox().GetEnd().x;
        }
    }

    VECTOR2D barOffset( 0.0, 0.0 );

    if( aTextStyle & TEXT_STYLE::OVERBAR )
    {
        std::unique_ptr<STROKE_GLYPH> overbarGlyph;

        barOffset.y = ComputeOverbarVerticalPosition( glyphSize.y );

        if( aTextStyle & TEXT_STYLE::ITALIC )
            barOffset.x = barOffset.y * ITALIC_TILT;

        overbarGlyph->AddPoint( VECTOR2D( aPosition.x + barOffset.x, cursor.y - barOffset.y ) );
        overbarGlyph->AddPoint( VECTOR2D( cursor.x + barOffset.x, cursor.y - barOffset.y ) );
        overbarGlyph->Finalize();

        aGlyphs.push_back( std::move( overbarGlyph ) );
    }

    if( aBoundingBox )
    {
        aBoundingBox->SetOrigin( aPosition.x, aPosition.y );
        aBoundingBox->SetEnd( cursor.x + barOffset.x, cursor.y + std::max( glyphSize.y, barOffset.y ) );
        aBoundingBox->Normalize();
    }

    return VECTOR2I( cursor.x, aPosition.y );
}
