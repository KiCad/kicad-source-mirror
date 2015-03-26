/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
#include <wx/string.h>

using namespace KIGFX;

const double STROKE_FONT::OVERBAR_HEIGHT = 1.22;
const double STROKE_FONT::BOLD_FACTOR = 1.3;
const double STROKE_FONT::HERSHEY_SCALE = 1.0 / 21.0;

STROKE_FONT::STROKE_FONT( GAL* aGal ) :
    m_gal( aGal ),
    m_bold( false ),
    m_italic( false ),
    m_mirrored( false ),
    m_overbar( false )
{
    // Default values
    m_glyphSize = VECTOR2D( 10.0, 10.0 );
    m_verticalJustify   = GR_TEXT_VJUSTIFY_BOTTOM;
    m_horizontalJustify = GR_TEXT_HJUSTIFY_LEFT;
}


bool STROKE_FONT::LoadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize )
{
    m_glyphs.clear();
    m_glyphBoundingBoxes.clear();
    m_glyphs.resize( aNewStrokeFontSize );
    m_glyphBoundingBoxes.resize( aNewStrokeFontSize );

    for( int j = 0; j < aNewStrokeFontSize; j++ )
    {
        GLYPH    glyph;
        double   glyphStartX = 0.0;
        double   glyphEndX = 0.0;
        VECTOR2D glyphBoundingX;

        std::deque<VECTOR2D> pointList;

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
                glyphStartX     = ( coordinate[0] - 'R' ) * HERSHEY_SCALE;
                glyphEndX       = ( coordinate[1] - 'R' ) * HERSHEY_SCALE;
                glyphBoundingX  = VECTOR2D( 0, glyphEndX - glyphStartX );
            }
            else if( ( coordinate[0] == ' ' ) && ( coordinate[1] == 'R' ) )
            {
                // Raise pen
                if( pointList.size() > 0 )
                    glyph.push_back( pointList );

                pointList.clear();
            }
            else
            {
                // Every coordinate description of the Hershey format has an offset,
                // it has to be subtracted
                point.x = (double) ( coordinate[0] - 'R' ) * HERSHEY_SCALE - glyphStartX;
				// -10 is here to keep GAL rendering consistent with the legacy gfx stuff
                point.y = (double) ( coordinate[1] - 'R' - 10) * HERSHEY_SCALE;
                pointList.push_back( point );
            }

            i += 2;
        }

        if( pointList.size() > 0 )
            glyph.push_back( pointList );

        m_glyphs[j] = glyph;

        // Compute the bounding box of the glyph
        m_glyphBoundingBoxes[j] = computeBoundingBox( glyph, glyphBoundingX );
    }

    return true;
}


int STROKE_FONT::getInterline() const
{
    return ( m_glyphSize.y * 14 ) / 10 + m_gal->GetLineWidth();
}


BOX2D STROKE_FONT::computeBoundingBox( const GLYPH& aGLYPH, const VECTOR2D& aGLYPHBoundingX ) const
{
    BOX2D boundingBox;

    std::deque<VECTOR2D> boundingPoints;

    boundingPoints.push_back( VECTOR2D( aGLYPHBoundingX.x, 0 ) );
    boundingPoints.push_back( VECTOR2D( aGLYPHBoundingX.y, 0 ) );

    for( GLYPH::const_iterator pointListIt = aGLYPH.begin(); pointListIt != aGLYPH.end(); ++pointListIt )
    {
        for( std::deque<VECTOR2D>::const_iterator pointIt = pointListIt->begin();
                pointIt != pointListIt->end(); ++pointIt )
        {
            boundingPoints.push_back( VECTOR2D( aGLYPHBoundingX.x, pointIt->y ) );
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
    int lineHeight = getInterline( );
    int lineCount = linesCount( aText );

    // align the 1st line of text
    switch( m_verticalJustify )
    {
    case GR_TEXT_VJUSTIFY_TOP:
        m_gal->Translate( VECTOR2D( 0, m_glyphSize.y ) );
        break;

    case GR_TEXT_VJUSTIFY_CENTER:
        m_gal->Translate( VECTOR2D( 0, m_glyphSize.y / 2.0 ) );
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        break;

    default:
        break;
    }

    if( lineCount > 1 )
    {
        switch( m_verticalJustify )
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
    m_gal->SetIsFill( false );

    if( m_bold )
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
    // By default the overbar is turned off
    m_overbar = false;

    double      xOffset;
    VECTOR2D    glyphSize( m_glyphSize );
    double      overbar_italic_comp = 0.0;

    // Compute the text size
    VECTOR2D textSize = computeTextSize( aText );

    m_gal->Save();

    // Adjust the text position to the given alignment
    switch( m_horizontalJustify )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        m_gal->Translate( VECTOR2D( -textSize.x / 2.0, 0 ) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        if( !m_mirrored )
            m_gal->Translate( VECTOR2D( -textSize.x, 0 ) );
        break;

    case GR_TEXT_HJUSTIFY_LEFT:
        if( m_mirrored )
            m_gal->Translate( VECTOR2D( -textSize.x, 0 ) );
        break;

    default:
        break;
    }

    if( m_mirrored )
    {
        // In case of mirrored text invert the X scale of points and their X direction
        // (m_glyphSize.x) and start drawing from the position where text normally should end
        // (textSize.x)
        xOffset = textSize.x;
        glyphSize.x = -m_glyphSize.x;
    }
    else
    {
        xOffset = 0.0;
    }

    // The overbar is indented inward at the beginning of an italicized section, but
    // must not be indented on subsequent letters to ensure that the bar segments
    // overlap.
    bool last_had_overbar = false;

    for( UTF8::uni_iter chIt = aText.ubegin(), end = aText.uend(); chIt < end; ++chIt )
    {
        // Toggle overbar
        if( *chIt == '~' )
        {
            if( ++chIt >= end )
                break;

            if( *chIt != '~' )      // It was a single tilda, it toggles overbar
                m_overbar = !m_overbar;

            // If it is a double tilda, just process the second one
        }

        int dd = *chIt - ' ';

        if( dd >= (int) m_glyphBoundingBoxes.size() || dd < 0 )
            dd = '?' - ' ';

        GLYPH& glyph = m_glyphs[dd];
        BOX2D& bbox  = m_glyphBoundingBoxes[dd];

        if( m_overbar && m_italic )
        {
            if( m_mirrored )
            {
                overbar_italic_comp = (-m_glyphSize.y * OVERBAR_HEIGHT) / 8;
            }
            else
            {
                overbar_italic_comp = (m_glyphSize.y * OVERBAR_HEIGHT) / 8;
            }
        }

        if( m_overbar )
        {
            double overbar_start_x = xOffset;
            double overbar_start_y = -m_glyphSize.y * OVERBAR_HEIGHT;
            double overbar_end_x = xOffset + glyphSize.x * bbox.GetEnd().x;
            double overbar_end_y = -m_glyphSize.y * OVERBAR_HEIGHT;

            if( !last_had_overbar )
            {
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

                if( m_italic )
                {
                    // FIXME should be done other way - referring to the lowest Y value of point
                    // because now italic fonts are translated a bit
                    if( m_mirrored )
                        pointPos.x += pointPos.y * 0.1;
                    else
                        pointPos.x -= pointPos.y * 0.1;
                }

                pointListScaled.push_back( pointPos );
            }

            m_gal->DrawPolyline( pointListScaled );
        }

        xOffset += glyphSize.x * bbox.GetEnd().x;
    }

    m_gal->Restore();
}


VECTOR2D STROKE_FONT::computeTextSize( const UTF8& aText ) const
{
    VECTOR2D result = VECTOR2D( 0.0, m_glyphSize.y );

    for( UTF8::uni_iter it = aText.ubegin(), end = aText.uend(); it < end; ++it )
    {
        wxASSERT_MSG( *it != '\n',
                      wxT( "This function is intended to work with single line strings" ) );

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

        result.x += m_glyphSize.x * m_glyphBoundingBoxes[dd].GetEnd().x;
    }

    return result;
}
