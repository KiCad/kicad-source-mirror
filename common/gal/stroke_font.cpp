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
#include <text_utils.h>
#include <wx/string.h>


using namespace KIGFX;

const double STROKE_FONT::INTERLINE_PITCH_RATIO = 1.61;
const double STROKE_FONT::OVERBAR_POSITION_FACTOR = 1.22;
const double STROKE_FONT::BOLD_FACTOR = 1.3;
const double STROKE_FONT::STROKE_FONT_SCALE = 1.0 / 21.0;
const double STROKE_FONT::ITALIC_TILT = 1.0 / 8;

STROKE_FONT::STROKE_FONT( GAL* aGal ) :
    m_gal( aGal )
{
}


bool STROKE_FONT::LoadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize )
{
    m_glyphs.clear();
    m_glyphBoundingBoxes.clear();
    m_glyphs.resize( aNewStrokeFontSize );
    m_glyphBoundingBoxes.resize( aNewStrokeFontSize );

    for( int j = 0; j < aNewStrokeFontSize; j++ )
    {
        GLYPH&   glyph = m_glyphs[j];
        double   glyphStartX = 0.0;
        double   glyphEndX = 0.0;
        VECTOR2D glyphBoundingX;

        std::deque<VECTOR2D>* pointList = nullptr;

        int i = 0;

        while( aNewStrokeFont[j][i] )
        {
            VECTOR2D    point( 0.0, 0.0 );
            char        coordinate[2] = { 0, };

            for( int k = 0; k < 2; k++ )
            {
                coordinate[k] = aNewStrokeFont[j][i + k];
            }

            if( i < 2 )
            {
                // The first two values contain the width of the char
                glyphStartX     = ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE;
                glyphEndX       = ( coordinate[1] - 'R' ) * STROKE_FONT_SCALE;
                glyphBoundingX  = VECTOR2D( 0, glyphEndX - glyphStartX );
            }
            else if( ( coordinate[0] == ' ' ) && ( coordinate[1] == 'R' ) )
            {
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
                    glyph.emplace_back( std::deque<VECTOR2D>() );
                    pointList = &glyph.back();
                }

                pointList->push_back( point );
            }

            i += 2;
        }

        // Compute the bounding box of the glyph
        m_glyphBoundingBoxes[j] = computeBoundingBox( glyph, glyphBoundingX );
    }

    return true;
}


// Static function:
double STROKE_FONT::GetInterline( double aGlyphHeight )
{
    // Do not add the glyph thickness to the interline.  This makes bold text line-spacing
    // different from normal text, which is poor typography.
    return ( aGlyphHeight * INTERLINE_PITCH_RATIO );
}


BOX2D STROKE_FONT::computeBoundingBox( const GLYPH& aGLYPH, const VECTOR2D& aGLYPHBoundingX ) const
{
    BOX2D boundingBox;

    std::deque<VECTOR2D> boundingPoints;

    boundingPoints.emplace_back( VECTOR2D( aGLYPHBoundingX.x, 0 ) );
    boundingPoints.emplace_back( VECTOR2D( aGLYPHBoundingX.y, 0 ) );

    for( GLYPH::const_iterator pointListIt = aGLYPH.begin(); pointListIt != aGLYPH.end(); ++pointListIt )
    {
        for( std::deque<VECTOR2D>::const_iterator pointIt = pointListIt->begin();
                pointIt != pointListIt->end(); ++pointIt )
        {
            boundingPoints.emplace_back( VECTOR2D( aGLYPHBoundingX.x, pointIt->y ) );
        }
    }

    boundingBox.Compute( boundingPoints );

    return boundingBox;
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
    VECTOR2D    glyphSize( m_gal->GetGlyphSize() );
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
        glyphSize.x = -glyphSize.x;
    }
    else
    {
        xOffset = 0.0;
    }

    // The overbar is indented inward at the beginning of an italicized section, but
    // must not be indented on subsequent letters to ensure that the bar segments
    // overlap.
    bool last_had_overbar = false;
    auto processedText = ProcessOverbars( aText );
    const auto& text = processedText.first;
    const auto& overbars = processedText.second;
    int         overbar_index = 0;

    for( UTF8::uni_iter chIt = text.ubegin(), end = text.uend(); chIt < end; ++chIt )
    {
        int dd = *chIt - ' ';

        // Handle tabs as locked to the nearest 4th column (counting in spaces)
        // The choice of spaces is somewhat arbitrary but sufficient for aligning text
        if( *chIt == '\t' )
        {
            double fourSpaces = 4.0 * glyphSize.x * m_glyphBoundingBoxes[0].GetEnd().x;
            double addlSpace = fourSpaces - std::fmod( xOffset, fourSpaces );

            // Add the remaining space (between 0 and 3 spaces)
            xOffset += addlSpace;

            // Set the character to ' ' instead of the '?' for tab
            dd = 0;
        }

        if( dd >= (int) m_glyphBoundingBoxes.size() || dd < 0 )
            dd = '?' - ' ';

        GLYPH& glyph = m_glyphs[dd];
        BOX2D& bbox  = m_glyphBoundingBoxes[dd];

        if( overbars[overbar_index] )
        {
            double overbar_start_x = xOffset;
            double overbar_start_y = - computeOverbarVerticalPosition();
            double overbar_end_x = xOffset + glyphSize.x * bbox.GetEnd().x;
            double overbar_end_y = overbar_start_y;

            if( !last_had_overbar )
            {
                if( m_gal->IsFontItalic() )
                    overbar_start_x += overbar_italic_comp;

                last_had_overbar = true;
            }

            VECTOR2D startOverbar( overbar_start_x, overbar_start_y );
            VECTOR2D endOverbar( overbar_end_x, overbar_end_y );

            m_gal->DrawLine( startOverbar, endOverbar );
        }
        else
        {
            last_had_overbar = false;
        }

        for( GLYPH::iterator pointListIt = glyph.begin(); pointListIt != glyph.end();
             ++pointListIt )
        {
            std::deque<VECTOR2D> pointListScaled;

            for( std::deque<VECTOR2D>::iterator pointIt = pointListIt->begin();
                 pointIt != pointListIt->end(); ++pointIt )
            {
                VECTOR2D pointPos( pointIt->x * glyphSize.x + xOffset, pointIt->y * glyphSize.y );

                if( m_gal->IsFontItalic() )
                {
                    // FIXME should be done other way - referring to the lowest Y value of point
                    // because now italic fonts are translated a bit
                    if( m_gal->IsTextMirrored() )
                        pointPos.x += pointPos.y * STROKE_FONT::ITALIC_TILT;
                    else
                        pointPos.x -= pointPos.y * STROKE_FONT::ITALIC_TILT;
                }

                pointListScaled.push_back( pointPos );
            }

            m_gal->DrawPolyline( pointListScaled );
        }

        xOffset += glyphSize.x * bbox.GetEnd().x;
        ++overbar_index;
    }

    m_gal->Restore();
}


double STROKE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight, double aGlyphThickness ) const
{
    // Static method.
    // Compute the Y position of the overbar. This is the distance between
    // the text base line and the overbar axis.
    return aGlyphHeight * OVERBAR_POSITION_FACTOR + aGlyphThickness;
}


double STROKE_FONT::computeOverbarVerticalPosition() const
{
    // Compute the Y position of the overbar. This is the distance between
    // the text base line and the overbar axis.
    return ComputeOverbarVerticalPosition( m_gal->GetGlyphSize().y, m_gal->GetLineWidth() );
}


VECTOR2D STROKE_FONT::computeTextLineSize( const UTF8& aText ) const
{
    return ComputeStringBoundaryLimits( aText, m_gal->GetGlyphSize(), m_gal->GetLineWidth() );
}


VECTOR2D STROKE_FONT::ComputeStringBoundaryLimits( const UTF8& aText, const VECTOR2D& aGlyphSize,
                                        double aGlyphThickness ) const
{
    VECTOR2D string_bbox;
    int line_count = 1;
    double maxX = 0.0, curX = 0.0;

    for( UTF8::uni_iter it = aText.ubegin(), end = aText.uend(); it < end; ++it )
    {
        if( *it == '\n' )
        {
            curX = 0.0;
            maxX = std::max( maxX, curX );
            ++line_count;
            continue;
        }

        // If it is double tilda, then it is displayed as a single tilda
        // If it is single tilda, then it is toggling overbar, so we need to skip it
        if( *it == '~' )
        {
            if( ++it >= end )
                break;
        }

        // Index in the bounding boxes table
        int dd = *it - ' ';

        if( dd >= (int) m_glyphBoundingBoxes.size() || dd < 0 )
            dd = '?' - ' ';

        const BOX2D& box = m_glyphBoundingBoxes[dd];
        curX += box.GetEnd().x;
    }

    string_bbox.x = std::max( maxX, curX );
    string_bbox.x *= aGlyphSize.x;
    string_bbox.x += aGlyphThickness;
    string_bbox.y = line_count * GetInterline( aGlyphSize.y );

    // For italic correction, take in account italic tilt
    if( m_gal->IsFontItalic() )
        string_bbox.x += string_bbox.y * STROKE_FONT::ITALIC_TILT;

    return string_bbox;
}
