/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
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

#include <gal/stroke_font.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/util.h>      // for KiROUND
#include <wx/string.h>
#include <gr_text.h>


using namespace KIGFX;

const double STROKE_FONT::INTERLINE_PITCH_RATIO = 1.61;
const double STROKE_FONT::OVERBAR_POSITION_FACTOR = 1.33;
const double STROKE_FONT::UNDERLINE_POSITION_FACTOR = 0.41;
const double STROKE_FONT::BOLD_FACTOR = 1.3;
const double STROKE_FONT::STROKE_FONT_SCALE = 1.0 / 21.0;
const double STROKE_FONT::ITALIC_TILT = 1.0 / 8;


GLYPH_LIST*         g_newStrokeFontGlyphs = nullptr;     ///< Glyph list
std::vector<BOX2D>* g_newStrokeFontGlyphBoundingBoxes;   ///< Bounding boxes of the glyphs


STROKE_FONT::STROKE_FONT( GAL* aGal ) :
    m_gal( aGal ), m_glyphs( nullptr ), m_glyphBoundingBoxes( nullptr ), m_maxGlyphWidth( 1.0 )
{
}


bool STROKE_FONT::LoadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize )
{
    if( g_newStrokeFontGlyphs )
    {
        m_glyphs = g_newStrokeFontGlyphs;
        m_glyphBoundingBoxes = g_newStrokeFontGlyphBoundingBoxes;
        return true;
    }

    g_newStrokeFontGlyphs = new GLYPH_LIST;
    g_newStrokeFontGlyphs->reserve( aNewStrokeFontSize );

    g_newStrokeFontGlyphBoundingBoxes = new std::vector<BOX2D>;
    g_newStrokeFontGlyphBoundingBoxes->reserve( aNewStrokeFontSize );

    for( int j = 0; j < aNewStrokeFontSize; j++ )
    {
        GLYPH*   glyph = new GLYPH;
        double   glyphStartX = 0.0;
        double   glyphEndX = 0.0;
        double   glyphWidth = 0.0;

        std::vector<VECTOR2D>* pointList = nullptr;

        int strokes = 0;
        int i = 0;

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
            VECTOR2D    point( 0.0, 0.0 );
            char        coordinate[2] = { 0, };

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
                if( pointList )
                    pointList->shrink_to_fit();

                // Raise pen
                pointList = nullptr;
            }
            else
            {
                // In stroke font, coordinates values are coded as <value> + 'R',
                // <value> is an ASCII char.
                // therefore every coordinate description of the Hershey format has an offset,
                // it has to be subtracted
                // Note:
                //  * the stroke coordinates are stored in reduced form (-1.0 to +1.0),
                //    and the actual size is stroke coordinate * glyph size
                //  * a few shapes have a height slightly bigger than 1.0 ( like '{' '[' )
                point.x = (double) ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE - glyphStartX;
                #define FONT_OFFSET -10
                // FONT_OFFSET is here for historical reasons, due to the way the stroke font
                // was built. It allows shapes coordinates like W M ... to be >= 0
                // Only shapes like j y have coordinates < 0
                point.y = (double) ( coordinate[1] - 'R' + FONT_OFFSET ) * STROKE_FONT_SCALE;

                if( !pointList )
                {
                    pointList = new std::vector<VECTOR2D>;
                    glyph->push_back( pointList );
                }

                pointList->push_back( point );
            }

            i += 2;
        }

        if( pointList )
            pointList->shrink_to_fit();

        // Compute the bounding box of the glyph
        g_newStrokeFontGlyphBoundingBoxes->emplace_back( computeBoundingBox( glyph, glyphWidth ) );
        g_newStrokeFontGlyphs->push_back( glyph );
        m_maxGlyphWidth = std::max( m_maxGlyphWidth,
                g_newStrokeFontGlyphBoundingBoxes->back().GetWidth() );
    }

    m_glyphs = g_newStrokeFontGlyphs;
    m_glyphBoundingBoxes = g_newStrokeFontGlyphBoundingBoxes;
    return true;
}


// Static function:
double STROKE_FONT::GetInterline( double aGlyphHeight )
{
    // Do not add the glyph thickness to the interline.  This makes bold text line-spacing
    // different from normal text, which is poor typography.
    return ( aGlyphHeight * INTERLINE_PITCH_RATIO );
}


BOX2D STROKE_FONT::computeBoundingBox( const GLYPH* aGLYPH, double aGlyphWidth ) const
{
    VECTOR2D min( 0, 0 );
    VECTOR2D max( aGlyphWidth, 0 );

    for( const std::vector<VECTOR2D>* pointList : *aGLYPH )
    {
        for( const VECTOR2D& point : *pointList )
        {
            min.y = std::min( min.y, point.y );
            max.y = std::max( max.y, point.y );
        }
    }

    return BOX2D( min, max - min );
}


void STROKE_FONT::Draw( const UTF8& aText, const VECTOR2D& aPosition, double aRotationAngle )
{
    if( aText.empty() )
        return;

    // Context needs to be saved before any transformations
    m_gal->Save();

    m_gal->Translate( aPosition );
    m_gal->Rotate( -aRotationAngle );

    // Single line height
    int lineHeight = KiROUND( GetInterline( m_gal->GetGlyphSize().y ) );
    int lineCount = linesCount( aText );
    const VECTOR2D& glyphSize = m_gal->GetGlyphSize();

    // align the 1st line of text
    switch( m_gal->GetVerticalJustify() )
    {
    case GR_TEXT_VJUSTIFY_TOP:
        m_gal->Translate( VECTOR2D( 0, glyphSize.y ) );
        break;

    case GR_TEXT_VJUSTIFY_CENTER:
        m_gal->Translate( VECTOR2D( 0, glyphSize.y / 2.0 ) );
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        break;

    default:
        break;
    }

    if( lineCount > 1 )
    {
        switch( m_gal->GetVerticalJustify() )
        {
        case GR_TEXT_VJUSTIFY_TOP:
            break;

        case GR_TEXT_VJUSTIFY_CENTER:
            m_gal->Translate( VECTOR2D(0, -( lineCount - 1 ) * lineHeight / 2) );
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            m_gal->Translate( VECTOR2D(0, -( lineCount - 1 ) * lineHeight ) );
            break;
        }
    }

    m_gal->SetIsStroke( true );
    //m_gal->SetIsFill( false );

    if( m_gal->IsFontBold() )
        m_gal->SetLineWidth( m_gal->GetLineWidth() * BOLD_FACTOR );

    // Split multiline strings into separate ones and draw them line by line
    size_t  begin = 0;
    size_t  newlinePos = aText.find( '\n' );

    while( newlinePos != aText.npos )
    {
        size_t length = newlinePos - begin;

        drawSingleLineText( aText.substr( begin, length ) );
        m_gal->Translate( VECTOR2D( 0.0, lineHeight ) );

        begin = newlinePos + 1;
        newlinePos = aText.find( '\n', begin );
    }

    // Draw the last (or the only one) line
    if( !aText.empty() )
        drawSingleLineText( aText.substr( begin ) );

    m_gal->Restore();
}


void STROKE_FONT::drawSingleLineText( const UTF8& aText )
{
    double      xOffset;
    double      yOffset;
    VECTOR2D    baseGlyphSize( m_gal->GetGlyphSize() );
    double      overbar_italic_comp = computeOverbarVerticalPosition() * ITALIC_TILT;

    if( m_gal->IsTextMirrored() )
        overbar_italic_comp = -overbar_italic_comp;

    // Compute the text size
    VECTOR2D textSize = computeTextLineSize( aText );
    double half_thickness = m_gal->GetLineWidth()/2;

    // Context needs to be saved before any transformations
    m_gal->Save();

    // First adjust: the text X position is corrected by half_thickness
    // because when the text with thickness is draw, its full size is textSize,
    // but the position of lines is half_thickness to textSize - half_thickness
    // so we must translate the coordinates by half_thickness on the X axis
    // to place the text inside the 0 to textSize X area.
    m_gal->Translate( VECTOR2D( half_thickness, 0 ) );

    // Adjust the text position to the given horizontal justification
    switch( m_gal->GetHorizontalJustify() )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        m_gal->Translate( VECTOR2D( -textSize.x / 2.0, 0 ) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        if( !m_gal->IsTextMirrored() )
            m_gal->Translate( VECTOR2D( -textSize.x, 0 ) );
        break;

    case GR_TEXT_HJUSTIFY_LEFT:
        if( m_gal->IsTextMirrored() )
            m_gal->Translate( VECTOR2D( -textSize.x, 0 ) );
        break;

    default:
        break;
    }

    if( m_gal->IsTextMirrored() )
    {
        // In case of mirrored text invert the X scale of points and their X direction
        // (m_glyphSize.x) and start drawing from the position where text normally should end
        // (textSize.x)
        xOffset = textSize.x - m_gal->GetLineWidth();
        baseGlyphSize.x = -baseGlyphSize.x;
    }
    else
    {
        xOffset = 0.0;
    }

    // The overbar is indented inward at the beginning of an italicized section, but
    // must not be indented on subsequent letters to ensure that the bar segments
    // overlap.
    bool     lastHadOverbar = false;
    int overbarDepth = -1;
    int superSubDepth = -1;
    int braceNesting = 0;
    VECTOR2D glyphSize = baseGlyphSize;

    // Allocate only once (for performance)
    std::vector<VECTOR2D> ptListScaled;
    int char_count = 0;

    yOffset = 0;

    for( UTF8::uni_iter chIt = aText.ubegin(), end = aText.uend(); chIt < end; ++chIt )
    {
        // Handle tabs as locked to the nearest 4th column (counting in spaces)
        // The choice of spaces is somewhat arbitrary but sufficient for aligning text
        if( *chIt == '\t' )
        {
            // We align to the 4th column.  This is based on the monospace font used in the text input
            // boxes.  Here, we take the widest character as our baseline spacing and make tab stops
            // at each fourth of this widest character
            char_count = ( char_count / 4 + 1 ) * 4 - 1;
            xOffset = m_maxGlyphWidth * baseGlyphSize.x * char_count;

            glyphSize = baseGlyphSize;
            yOffset = 0;
        }
        else if( *chIt == '^' && superSubDepth == -1 )
        {
            UTF8::uni_iter lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                chIt = lookahead;
                superSubDepth = braceNesting;
                braceNesting++;

                glyphSize = baseGlyphSize * 0.8;
                yOffset = -baseGlyphSize.y * 0.3;
                continue;
            }
        }
        else if( *chIt == '_' && superSubDepth == -1 )
        {
            UTF8::uni_iter lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                chIt = lookahead;
                superSubDepth = braceNesting;
                braceNesting++;

                glyphSize = baseGlyphSize * 0.8;
                yOffset = baseGlyphSize.y * 0.1;
                continue;
            }
        }
        else if( *chIt == '~' && overbarDepth == -1 )
        {
            UTF8::uni_iter lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                chIt = lookahead;
                overbarDepth = braceNesting;
                braceNesting++;
                continue;
            }
        }
        else if( *chIt == '{' )
        {
            braceNesting++;
        }
        else if( *chIt == '}' )
        {
            if( braceNesting > 0 )
                braceNesting--;

            if( braceNesting == superSubDepth )
            {
                superSubDepth = -1;

                glyphSize = baseGlyphSize;
                yOffset = 0;
                continue;
            }

            if( braceNesting == overbarDepth )
            {
                overbarDepth = -1;
                continue;
            }
        }

        // Index into bounding boxes table
        int dd = (signed) *chIt - ' ';

        if( dd >= (int) m_glyphBoundingBoxes->size() || dd < 0 )
        {
            int substitute = *chIt == '\t' ? ' ' : '?';
            dd = substitute - ' ';
        }

        const GLYPH* glyph = m_glyphs->at( dd );
        const BOX2D& bbox  = m_glyphBoundingBoxes->at( dd );

        if( overbarDepth != -1 )
        {
            double overbar_start_x = xOffset;
            double overbar_start_y = - computeOverbarVerticalPosition();
            double overbar_end_x = xOffset + glyphSize.x * bbox.GetEnd().x;
            double overbar_end_y = overbar_start_y;

            if( !lastHadOverbar )
            {
                if( m_gal->IsFontItalic() )
                    overbar_start_x += overbar_italic_comp;

                lastHadOverbar = true;
            }

            VECTOR2D startOverbar( overbar_start_x, overbar_start_y );
            VECTOR2D endOverbar( overbar_end_x, overbar_end_y );

            m_gal->DrawLine( startOverbar, endOverbar );
        }
        else
        {
            lastHadOverbar = false;
        }

        if( m_gal->IsFontUnderlined() )
        {
            double   vOffset = computeUnderlineVerticalPosition();
            VECTOR2D startUnderline( xOffset, - vOffset );
            VECTOR2D endUnderline( xOffset + glyphSize.x * bbox.GetEnd().x, - vOffset );

            m_gal->DrawLine( startUnderline, endUnderline );
        }

        for( const std::vector<VECTOR2D>* ptList : *glyph )
        {
            int ptCount = 0;
            ptListScaled.clear();

            for( const VECTOR2D& pt : *ptList )
            {
                VECTOR2D scaledPt( pt.x * glyphSize.x + xOffset, pt.y * glyphSize.y + yOffset );

                if( m_gal->IsFontItalic() )
                {
                    // FIXME should be done other way - referring to the lowest Y value of point
                    // because now italic fonts are translated a bit
                    if( m_gal->IsTextMirrored() )
                        scaledPt.x += scaledPt.y * STROKE_FONT::ITALIC_TILT;
                    else
                        scaledPt.x -= scaledPt.y * STROKE_FONT::ITALIC_TILT;
                }

                ptListScaled.push_back( scaledPt );
                ptCount++;
            }

            m_gal->DrawPolyline( &ptListScaled[0], ptCount );
        }

        char_count++;
        xOffset += glyphSize.x * bbox.GetEnd().x;
    }

    m_gal->Restore();
}


double STROKE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
    // Static method.
    return aGlyphHeight * OVERBAR_POSITION_FACTOR;
}


double STROKE_FONT::computeOverbarVerticalPosition() const
{
    // Compute the Y position of the overbar. This is the distance between
    // the text base line and the overbar axis.
    return ComputeOverbarVerticalPosition( m_gal->GetGlyphSize().y );
}


double STROKE_FONT::computeUnderlineVerticalPosition() const
{
    // Compute the Y position of the underline. This is the distance between
    // the text base line and the underline axis.
    return - m_gal->GetGlyphSize().y * UNDERLINE_POSITION_FACTOR;
}


VECTOR2D STROKE_FONT::computeTextLineSize( const UTF8& aText ) const
{
    return ComputeStringBoundaryLimits( aText, m_gal->GetGlyphSize(), m_gal->GetLineWidth() );
}


VECTOR2D STROKE_FONT::ComputeStringBoundaryLimits( const UTF8& aText, const VECTOR2D& aGlyphSize,
                                                   double aGlyphThickness ) const
{
    VECTOR2D string_bbox;
    int      line_count = 1;
    double   maxX = 0.0, curX = 0.0;

    double curScale = 1.0;
    int overbarDepth = -1;
    int superSubDepth = -1;
    int braceNesting = 0;

    for( UTF8::uni_iter chIt = aText.ubegin(), end = aText.uend(); chIt < end; ++chIt )
    {
        if( *chIt == '\n' )
        {
            curX = 0.0;
            maxX = std::max( maxX, curX );
            ++line_count;
            continue;
        }

        // Handle tabs as locked to the nearest 4th column (counting in spaces)
        // The choice of spaces is somewhat arbitrary but sufficient for aligning text
        if( *chIt == '\t' )
        {
            double spaces = m_glyphBoundingBoxes->at( 0 ).GetEnd().x;
            double addlSpace = 3.0 * spaces - std::fmod( curX, 4.0 * spaces );

            // Add the remaining space (between 0 and 3 spaces)
            curX += addlSpace;
        }
        else if( (*chIt == '^' || *chIt == '_') && superSubDepth == -1 )
        {
            auto lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                //  process superscript
                chIt = lookahead;
                superSubDepth = braceNesting;

                curScale = 0.8;
                continue;
            }
        }
        else if( *chIt == '{' )
        {
            braceNesting++;
        }
        else if( *chIt == '}' )
        {
            if( braceNesting > 0 )
                braceNesting--;

            if( braceNesting == superSubDepth )
            {
                superSubDepth = -1;

                curScale = 1.0;
                continue;
            }
        }

        // Index in the bounding boxes table
        int dd = (signed) *chIt - ' ';

        if( dd >= (int) m_glyphBoundingBoxes->size() || dd < 0 )
        {
            int substitute = *chIt == '\t' ? ' ' : '?';
            dd = substitute - ' ';
        }

        const BOX2D& box = m_glyphBoundingBoxes->at( dd );
        curX += box.GetEnd().x * curScale;
    }

    string_bbox.x = std::max( maxX, curX ) * aGlyphSize.x;
    string_bbox.x += aGlyphThickness;
    string_bbox.y = line_count * GetInterline( aGlyphSize.y );

    // For italic correction, take in account italic tilt
    if( m_gal->IsFontItalic() )
        string_bbox.x += string_bbox.y * STROKE_FONT::ITALIC_TILT;

    return string_bbox;
}
