/**
 * Functions to draw and plot text on screen
 * @file drawtxt.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <plot_common.h>
#include <eda_text.h>               // EDA_TEXT_HJUSTIFY_T and EDA_TEXT_VJUSTIFY_T
#include <trigo.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <class_base_screen.h>

#include <gal/stroke_font.h>

#include <gal/graphics_abstraction_layer.h>
#include <newstroke_font.h>

#include <plot_common.h>

using namespace KIGFX;

struct TRANSFORM_PRM
{
    VECTOR2D m_rotCenter;
    VECTOR2D m_moveOffset;
    double   m_rotAngle;
};

class BASIC_GAL: public GAL
{
public:
    wxDC* m_DC;
    EDA_COLOR_T m_Color;

private:
    TRANSFORM_PRM m_transform;
    std::stack <TRANSFORM_PRM>  m_transformHistory;

public:
    BASIC_GAL()
    {
        m_DC = NULL;
        m_Color = RED;
        m_plotter = NULL;
        m_callback = NULL;
    }

    void SetPlotter( PLOTTER* aPlotter )
    {
        m_plotter = aPlotter;
    }

    void SetCallback( void (* aCallback)( int x0, int y0, int xf, int yf ) )
    {
        m_callback = aCallback;
    }

    /// Set a clip box for drawings
    /// If NULL, no clip will be made
    void SetClipBox( EDA_RECT* aClipBox )
    {
        m_isClipped = aClipBox != NULL;

        if( aClipBox )
            m_clipBox = *aClipBox;
    }

    /// @brief Save the context.
    virtual void Save()
    {
        m_transformHistory.push( m_transform );
    }

    virtual void Restore()
    {
        m_transform = m_transformHistory.top();
        m_transformHistory.pop();
    }


    /**
     * @brief Draw a polyline
     * @param aPointList is a list of 2D-Vectors containing the polyline points.
     */
    virtual void DrawPolyline( const std::deque<VECTOR2D>& aPointList );

    /** Start and end points are defined as 2D-Vectors.
     * @param aStartPoint   is the start point of the line.
     * @param aEndPoint     is the end point of the line.
     */
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /**
     * @brief Translate the context.
     *
     * @param aTranslation is the translation vector.
     */
    virtual void Translate( const VECTOR2D& aTranslation )
    {
        m_transform.m_moveOffset += aTranslation;
    }

    /**
     * @brief Rotate the context.
     *
     * @param aAngle is the rotation angle in radians.
     */
    virtual void Rotate( double aAngle )
    {
        m_transform.m_rotAngle = aAngle * M_PI/1800;
        m_transform.m_rotCenter = m_transform.m_moveOffset;
    }

private:
    // Apply the roation/translation transform to aPoint
    const VECTOR2D transform( const VECTOR2D& aPoint ) const;

    // A clip box, to clip drawings in a wxDC (mandatory to avoid draw issues)
    EDA_RECT  m_clipBox;        // The clip box
    bool      m_isClipped;      // Allows/disallows clipping

    // When calling the draw functions outside a wxDC, to get the basic drawings
    // lines / polylines ..., a callback function (used in DRC) to store
    // coordinates of each segment:
    void (* m_callback)( int x0, int y0, int xf, int yf );

    // When calling the draw functions for plot, the plotter (has the same purpose
    // as a wxDC) to plot basic drawings
    PLOTTER* m_plotter;
};


const VECTOR2D BASIC_GAL::transform( const VECTOR2D& aPoint ) const
{
    VECTOR2D point = aPoint + m_transform.m_moveOffset - m_transform.m_rotCenter;
    point = point.Rotate( m_transform.m_rotAngle ) + m_transform.m_rotCenter;
    return point;
}

void BASIC_GAL::DrawPolyline( const std::deque<VECTOR2D>& aPointList )
{
    if( aPointList.empty() )
        return;

    std::deque<VECTOR2D>::const_iterator it = aPointList.begin();
    std::vector <wxPoint> polyline_corners;

    for( ; it != aPointList.end(); ++it )
    {
        VECTOR2D corner = transform(*it);
        polyline_corners.push_back( wxPoint( corner.x, corner.y ) );
    }

    if( m_DC )
    {
        if( isFillEnabled )
        {
            GRPoly( m_isClipped ? &m_clipBox : NULL, m_DC, polyline_corners.size(),
                    &polyline_corners[0], 0, GetLineWidth(), m_Color, m_Color );
        }
        else
        {
            for( unsigned ii = 1; ii < polyline_corners.size(); ++ii )
            {
                GRCSegm( m_isClipped ? &m_clipBox : NULL, m_DC, polyline_corners[ii-1],
                         polyline_corners[ii], GetLineWidth(), m_Color );
            }
        }
    }
    else if( m_plotter )
    {
        m_plotter->MoveTo( polyline_corners[0] );

        for( unsigned ii = 1; ii < polyline_corners.size(); ii++ )
        {
            m_plotter->LineTo( polyline_corners[ii] );
        }

        m_plotter->PenFinish();
    }
    else if( m_callback )
    {
        for( unsigned ii = 1; ii < polyline_corners.size(); ii++ )
        {
            m_callback( polyline_corners[ii-1].x, polyline_corners[ii-1].y,
                        polyline_corners[ii].x, polyline_corners[ii].y );
        }
    }
}

void BASIC_GAL::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    VECTOR2D startVector = transform( aStartPoint );
    VECTOR2D endVector = transform( aEndPoint );

    if( m_DC )
    {
        if( isFillEnabled )
        {
            GRLine( m_isClipped ? &m_clipBox : NULL, m_DC, startVector.x, startVector.y,
                    endVector.x, endVector.y, GetLineWidth(), m_Color );
        }
        else
        {
            GRCSegm( m_isClipped ? &m_clipBox : NULL, m_DC, startVector.x, startVector.y,
                    endVector.x, endVector.y, GetLineWidth(), 0, m_Color );
        }
    }
    else if( m_plotter )
    {
        m_plotter->MoveTo( wxPoint( startVector.x, startVector.y ) );
        m_plotter->LineTo( wxPoint( endVector.x, endVector.y ) );
        m_plotter->PenFinish();
    }
    else if( m_callback )
    {
            m_callback( startVector.x, startVector.y,
                        endVector.x, endVector.y );
    }
}


BASIC_GAL basic_gal;


int OverbarPositionY( int size_v )
{
    return KiROUND( size_v * STROKE_FONT::OVERBAR_HEIGHT );
}

/**
 * Function GetPensizeForBold
 * @return the "best" value for a pen size to draw/plot a bold text
 * @param aTextSize = the char size (height or width)
 */
int GetPenSizeForBold( int aTextSize )
{
    return KiROUND( aTextSize / 5.0 );
}


/**
 * Function  Clamp_Text_PenSize
 * As a rule, pen width should not be >1/4em, otherwise the character
 * will be cluttered up in its own fatness
 * so pen width max is aSize/4 for bold text, and aSize/6 for normal text
 * The "best" pen width is aSize/5 for bold texts,
 * so the clamp is consistant with bold option.
 * @param aPenSize = the pen size to clamp
 * @param aSize the char size (height or width)
 * @param aBold = true if text accept bold pen size
 * @return the max pen size allowed
 */
int Clamp_Text_PenSize( int aPenSize, int aSize, bool aBold )
{
    int     penSize     = aPenSize;
    double  scale       = aBold ? 4.0 : 6.0;
    int     maxWidth    = KiROUND( std::abs( aSize ) / scale );

    if( penSize > maxWidth )
        penSize = maxWidth;

    return penSize;
}


int Clamp_Text_PenSize( int aPenSize, wxSize aSize, bool aBold )
{
    int size = std::min( std::abs( aSize.x ), std::abs( aSize.y ) );

    return Clamp_Text_PenSize( aPenSize, size, aBold );
}


/* Function GetHersheyShapeDescription
 * return a pointer to the shape corresponding to unicode value AsciiCode
 * Note we use the same font for Bold and Normal texts
 * because KiCad handles a variable pen size to do that
 * that gives better results in XOR draw mode.
 */
static const char* GetHersheyShapeDescription( int AsciiCode )
{
    // calculate font length
    int font_length_max = newstroke_font_bufsize;

    if( AsciiCode >= (32 + font_length_max) )
        AsciiCode = '?';

    if( AsciiCode < 32 )
        AsciiCode = 32; // Clamp control chars

    AsciiCode -= 32;

    return newstroke_font[AsciiCode];
}


int GraphicTextWidth( const wxString& aText, int aXSize, bool aItalic, bool aWidth )
{
    int tally = 0;
    int char_count = aText.length();

    for( int i = 0; i < char_count; i++ )
    {
        int asciiCode = aText[i];

        /* Skip the negation marks
         * and first '~' char of '~~'
         * ('~~' draw as '~')
         */
        if( asciiCode == '~' )
        {
            if( i == 0 || aText[i - 1] != '~' )
                continue;
        }

        const char* shape_ptr = GetHersheyShapeDescription( asciiCode );
        // Get metrics
        int         xsta    = *shape_ptr++ - 'R';
        int         xsto    = *shape_ptr++ - 'R';
        tally += KiROUND( aXSize * (xsto - xsta) * STROKE_FONT::STROKE_FONT_SCALE );
    }

    // For italic correction, add 1/8 size
    if( aItalic )
    {
        tally += KiROUND( aXSize * STROKE_FONT::ITALIC_TILT );
    }

    return tally;
}


/**
 * Function DrawGraphicText
 * Draw a graphic text (like module texts)
 *  @param aClipBox = the clipping rect, or NULL if no clipping
 *  @param aDC = the current Device Context. NULL if draw within a 3D GL Canvas
 *  @param aPos = text position (according to h_justify, v_justify)
 *  @param aColor (enum EDA_COLOR_T) = text color
 *  @param aText = text to draw
 *  @param aOrient = angle in 0.1 degree
 *  @param aSize = text size (size.x or size.y can be < 0 for mirrored texts)
 *  @param aH_justify = horizontal justification (Left, center, right)
 *  @param aV_justify = vertical justification (bottom, center, top)
 *  @param aWidth = line width (pen width) (use default width if aWidth = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *      Use a value min(aSize.x, aSize.y) / 5 for a bold text
 *  @param aItalic = true to simulate an italic font
 *  @param aBold = true to use a bold font. Useful only with default width value (aWidth = 0)
 *  @param aCallback() = function called (if non null) to draw each segment.
 *                  used to draw 3D texts or for plotting, NULL for normal drawings
 *  @param aPlotter = a pointer to a PLOTTER instance, when this function is used to plot
 *                  the text. NULL to draw this text.
 */
void DrawGraphicText( EDA_RECT* aClipBox,
                      wxDC* aDC,
                      const wxPoint& aPos,
                      EDA_COLOR_T aColor,
                      const wxString& aText,
                      double aOrient,
                      const wxSize& aSize,
                      enum EDA_TEXT_HJUSTIFY_T aH_justify,
                      enum EDA_TEXT_VJUSTIFY_T aV_justify,
                      int aWidth,
                      bool aItalic,
                      bool aBold,
                      void (* aCallback)( int x0, int y0, int xf, int yf ),
                      PLOTTER* aPlotter )
{
    bool    fill_mode = true;

    if( aWidth == 0 && aBold ) // Use default values if aWidth == 0
        aWidth = GetPenSizeForBold( std::min( aSize.x, aSize.y ) );

    if( aWidth < 0 )
    {
        aWidth = -aWidth;
        fill_mode = false;
    }

    basic_gal.SetIsFill( fill_mode );
    basic_gal.SetLineWidth( aWidth );

    EDA_TEXT dummy;
    dummy.SetItalic( aItalic );
    dummy.SetBold( aBold );
    dummy.SetHorizJustify( aH_justify );
    dummy.SetVertJustify( aV_justify );

    wxSize size = aSize;
    dummy.SetMirrored( size.x < 0 );

    if( size.x < 0 )
        size.x = - size.x;

    dummy.SetSize( size );

    basic_gal.SetTextAttributes( &dummy );
    basic_gal.SetPlotter( aPlotter );
    basic_gal.SetCallback( aCallback );
    basic_gal.m_DC = aDC;
    basic_gal.m_Color = aColor;
    basic_gal.SetClipBox( aClipBox );
    basic_gal.StrokeText( aText, VECTOR2D( aPos.x, aPos.y ), aOrient );
}

void DrawGraphicHaloText( EDA_RECT* aClipBox, wxDC * aDC,
                          const wxPoint &aPos,
                          enum EDA_COLOR_T aBgColor,
                          enum EDA_COLOR_T aColor1,
                          enum EDA_COLOR_T aColor2,
                          const wxString &aText,
                          double aOrient,
                          const wxSize &aSize,
                          enum EDA_TEXT_HJUSTIFY_T aH_justify,
                          enum EDA_TEXT_VJUSTIFY_T aV_justify,
                          int aWidth, bool aItalic, bool aBold,
                          void (*aCallback)( int x0, int y0, int xf, int yf ),
                          PLOTTER * aPlotter )
{
    // Swap color if contrast would be better
    if( ColorIsLight( aBgColor ) )
    {
        EDA_COLOR_T c = aColor1;
        aColor1 = aColor2;
        aColor2 = c;
    }

    DrawGraphicText( aClipBox, aDC, aPos, aColor1, aText, aOrient, aSize,
                     aH_justify, aV_justify, aWidth, aItalic, aBold,
                     aCallback, aPlotter );

    DrawGraphicText( aClipBox, aDC, aPos, aColor2, aText, aOrient, aSize,
                     aH_justify, aV_justify, aWidth / 4, aItalic, aBold,
                     aCallback, aPlotter );
}

/**
 * Function PlotGraphicText
 *  same as DrawGraphicText, but plot graphic text insteed of draw it
 *  @param aPos = text position (according to aH_justify, aV_justify)
 *  @param aColor (enum EDA_COLOR_T) = text color
 *  @param aText = text to draw
 *  @param aOrient = angle in 0.1 degree
 *  @param aSize = text size (size.x or size.y can be < 0 for mirrored texts)
 *  @param aH_justify = horizontal justification (Left, center, right)
 *  @param aV_justify = vertical justification (bottom, center, top)
 *  @param aWidth = line width (pen width) (default = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *      Use a value min(aSize.x, aSize.y) / 5 for a bold text
 *  @param aItalic = true to simulate an italic font
 *  @param aBold = true to use a bold font Useful only with default width value (aWidth = 0)
 *  @param aMultilineAllowed = true to plot text as multiline, otherwise single line
 */
void PLOTTER::Text( const wxPoint&              aPos,
                    enum EDA_COLOR_T            aColor,
                    const wxString&             aText,
                    double                      aOrient,
                    const wxSize&               aSize,
                    enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                    enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                    int                         aWidth,
                    bool                        aItalic,
                    bool                        aBold,
                    bool                        aMultilineAllowed )
{
    int textPensize = aWidth;

    if( textPensize == 0 && aBold ) // Use default values if aWidth == 0
        textPensize = GetPenSizeForBold( std::min( aSize.x, aSize.y ) );

    if( textPensize >= 0 )
        textPensize = Clamp_Text_PenSize( aWidth, aSize, aBold );
    else
        textPensize = -Clamp_Text_PenSize( -aWidth, aSize, aBold );

    SetCurrentLineWidth( textPensize );

    if( aColor >= 0 )
        SetColor( aColor );

    if( aMultilineAllowed )
    {
        // EDA_TEXT needs for calculations of the position of every
        // line according to orientation and justifications
        wxArrayString strings;
        EDA_TEXT* multilineText = new EDA_TEXT( aText );
        multilineText->SetSize( aSize );
        multilineText->SetTextPosition( aPos );
        multilineText->SetOrientation( aOrient );
        multilineText->SetHorizJustify( aH_justify );
        multilineText->SetVertJustify( aV_justify );
        multilineText->SetThickness( aWidth );
        multilineText->SetMultilineAllowed( aMultilineAllowed );

        std::vector<wxPoint> positions;
        wxStringSplit( aText, strings, '\n' );
        positions.reserve( strings.Count() );

        multilineText->GetPositionsOfLinesOfMultilineText(
                positions, strings.Count() );

        for( unsigned ii = 0; ii < strings.Count(); ii++ )
        {
            wxString& txt = strings.Item( ii );
            DrawGraphicText( NULL, NULL, positions[ii], aColor, txt,
                             aOrient, aSize,
                             aH_justify, aV_justify,
                             textPensize, aItalic, aBold, NULL, this );
        }

        delete multilineText;
    }
    else
    {
        DrawGraphicText( NULL, NULL, aPos, aColor, aText,
                         aOrient, aSize,
                         aH_justify, aV_justify,
                         textPensize, aItalic, aBold, NULL, this );
    }

    if( aWidth != textPensize )
        SetCurrentLineWidth( aWidth );
}
