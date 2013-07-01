/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
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

using namespace KiGfx;

STROKE_FONT::STROKE_FONT( GAL* aGal ) :
    m_gal( aGal ),
    m_bold( false ),
    m_italic( false ),
    m_mirrored( false )
{
    // Default values
    m_scaleFactor = 1.0 / 21.0;
    m_glyphSize = VECTOR2D( 10.0, 10.0 );
    m_verticalJustify   = GR_TEXT_VJUSTIFY_BOTTOM;
    m_horizontalJustify = GR_TEXT_HJUSTIFY_LEFT;
}


STROKE_FONT::~STROKE_FONT()
{
}


bool STROKE_FONT::LoadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize )
{
    m_glyphs.clear();
    m_glyphBoundingBoxes.clear();

    for( int j = 0; j < aNewStrokeFontSize; j++ )
    {
        Glyph    glyph;
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
                glyphStartX     = coordinate[0] - 'R';
                glyphEndX       = coordinate[1] - 'R';
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
                point.x = (double) ( coordinate[0] - 'R' ) - glyphStartX;
                point.y = (double) ( coordinate[1] - 'R' ) - 11.0;
                pointList.push_back( point );
            }

            i += 2;
        }

        if( pointList.size() > 0 )
            glyph.push_back( pointList );

        m_glyphs.push_back( glyph );

        // Compute the bounding box of the glyph
        m_glyphBoundingBoxes.push_back( computeBoundingBox( glyph, glyphBoundingX ) );
    }

    return true;
}


void STROKE_FONT::LoadAttributes( const EDA_TEXT* aText )
{
    SetGlyphSize( VECTOR2D( aText->GetSize() ) );
    SetHorizontalJustify( aText->GetHorizJustify() );
    SetVerticalJustify( aText->GetVertJustify() );
    SetBold( aText->IsBold() );
    SetItalic( aText->IsItalic() );
    SetMirrored( aText->IsMirrored() );
}


BOX2D STROKE_FONT::computeBoundingBox( const Glyph& aGlyph, const VECTOR2D& aGlyphBoundingX ) const
{
    BOX2D boundingBox;

    std::deque<VECTOR2D> boundingPoints;

    boundingPoints.push_back( VECTOR2D( aGlyphBoundingX.x, 0 ) );
    boundingPoints.push_back( VECTOR2D( aGlyphBoundingX.y, 0 ) );

    for( Glyph::const_iterator pointListIt = aGlyph.begin(); pointListIt != aGlyph.end(); ++pointListIt )
    {
        for( std::deque<VECTOR2D>::const_iterator pointIt = pointListIt->begin();
                pointIt != pointListIt->end(); ++pointIt )
        {
            boundingPoints.push_back( VECTOR2D( aGlyphBoundingX.x, pointIt->y ) );
        }
    }

    boundingBox.Compute( boundingPoints );

    return boundingBox;
}


void STROKE_FONT::Draw( std::string aText, const VECTOR2D& aPosition, double aRotationAngle )
{
    // Split multiline strings into separate ones and draw line by line
    size_t newlinePos = aText.find( '\n' );

    if( newlinePos != std::string::npos )
    {
        VECTOR2D nextlinePosition( aPosition );
        nextlinePosition += VECTOR2D( 0.0, m_glyphSize.y * 1.6 );   // FIXME remove magic number

        Draw( aText.substr( newlinePos + 1 ), nextlinePosition, aRotationAngle );
        aText = aText.substr( 0, newlinePos );
    }

    // Compute the text size
    VECTOR2D textsize = computeTextSize( aText );

    // By default overbar is turned off
    m_overbar = false;

    // Context needs to be saved before any transformations
    m_gal->Save();

    m_gal->Translate( aPosition );
    m_gal->Rotate( -aRotationAngle );

    // Adjust the text position to the given alignment
    switch( m_horizontalJustify )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        m_gal->Translate( VECTOR2D( -textsize.x / 2, 0 ) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        if( !m_mirrored )
            m_gal->Translate( VECTOR2D( -textsize.x, 0 ) );
        break;

    case GR_TEXT_HJUSTIFY_LEFT:
        if( m_mirrored )
            m_gal->Translate( VECTOR2D( -textsize.x, 0 ) );
        break;

    default:
        break;
    }

    switch( m_verticalJustify )
    {
    case GR_TEXT_VJUSTIFY_CENTER:
        m_gal->Translate( VECTOR2D( 0, textsize.y / 2 ) );
        break;

    case GR_TEXT_VJUSTIFY_TOP:
        m_gal->Translate( VECTOR2D( 0, textsize.y ) );
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        break;

    default:
        break;
    }

    double xOffset, glyphSizeX;

    if( m_mirrored )
    {
        // In case of mirrored text invert the X scale of points and their X direction
        // (m_glyphSize.x) and start drawing from the position where text normally should end
        // (textsize.x)
        xOffset     = textsize.x;
        glyphSizeX  = -m_glyphSize.x;
    }
    else
    {
        xOffset     = 0.0;
        glyphSizeX  = m_glyphSize.x;
    }
    double scaleY = m_scaleFactor * m_glyphSize.y;
    double scaleX = m_scaleFactor * glyphSizeX;

    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineCap( LINE_CAP_ROUND );
    m_gal->SetLineJoin( LINE_JOIN_ROUND );

    if( m_bold )
    {
        m_gal->SetLineWidth( m_gal->GetLineWidth() * 1.3 );
    }

    for( std::string::const_iterator chIt = aText.begin(); chIt != aText.end(); chIt++ )
    {
        if( *chIt == '~' )
        {
            m_overbar = !m_overbar;
            continue;
        }

        GlyphList::iterator glyphIt = m_glyphs.begin();
        std::deque<BOX2D>::iterator bbIt = m_glyphBoundingBoxes.begin();

        advance( glyphIt, (int) ( *chIt ) - (int) ' ' );
        advance( bbIt, (int) ( *chIt ) - (int) ' ' );

        Glyph glyph = *glyphIt;

        for( Glyph::iterator pointListIt = glyph.begin(); pointListIt != glyph.end();
             pointListIt++ )
        {
            std::deque<VECTOR2D> pointListScaled;

            for( std::deque<VECTOR2D>::iterator pointIt = pointListIt->begin();
                 pointIt != pointListIt->end(); pointIt++ )
            {
                VECTOR2D pointPos( pointIt->x * scaleX + xOffset, pointIt->y * scaleY );

                if( m_italic )
                {
                    // FIXME should be done other way - referring to the lowest Y value of point
                    // because now italic fonts are translated a bit
                    pointPos.x += pointPos.y * 0.1;
                }

                pointListScaled.push_back( pointPos );
            }

            m_gal->DrawPolyline( pointListScaled );
        }

        if( m_overbar )
        {
            VECTOR2D startOverbar( xOffset, -textsize.y * 1.2 );
            VECTOR2D endOverbar( xOffset + m_scaleFactor * glyphSizeX * bbIt->GetEnd().x,
                                 -textsize.y * 1.2 );
            m_gal->DrawLine( startOverbar, endOverbar );
        }

        xOffset += m_scaleFactor * glyphSizeX * bbIt->GetEnd().x;
    }

    m_gal->Restore();
}


VECTOR2D STROKE_FONT::computeTextSize( const std::string& aText ) const
{
    VECTOR2D result = VECTOR2D( 0.0, m_glyphSize.y );

    for( std::string::const_iterator chIt = aText.begin(); chIt != aText.end(); chIt++ )
    {
        if( *chIt == '~' )
            continue;

        std::deque<BOX2D>::const_iterator bbIt = m_glyphBoundingBoxes.begin();
        advance( bbIt, (int) ( *chIt ) - (int) ' ' );
        result.x += m_scaleFactor * m_glyphSize.x * bbIt->GetEnd().x;
    }

    return result;
}
