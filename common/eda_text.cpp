/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file eda_text.cpp
 * @brief Implementation of base KiCad text object.
 */

#include <eda_text.h>
#include <drawtxt.h>
#include <macros.h>
#include <trigo.h>               // RotatePoint
#include <class_drawpanel.h>     // EDA_DRAW_PANEL

// Conversion to application internal units defined at build time.
#if defined( PCBNEW )
    #include <class_board_item.h>
#elif defined( EESCHEMA )
    #include <sch_item_struct.h>
#elif defined( GERBVIEW )
#elif defined( PL_EDITOR )
    #include <base_units.h>
    #define FMT_IU Double2Str
#else
#error "Cannot resolve units formatting due to no definition of EESCHEMA or PCBNEW."
#endif


EDA_TEXT::EDA_TEXT( const wxString& text )
{
    m_Size.x    = m_Size.y = Mils2iu( DEFAULT_SIZE_TEXT );  // Width and height of font.
    m_Orient    = 0;                             // Rotation angle in 0.1 degrees.
    m_Attributs = 0;
    m_Mirror    = false;                         // display mirror if true
    m_HJustify  = GR_TEXT_HJUSTIFY_CENTER;       // Default horizontal justification is centered.
    m_VJustify  = GR_TEXT_VJUSTIFY_CENTER;       // Default vertical justification is centered.
    m_Thickness = 0;                             // thickness
    m_Italic    = false;                         // true = italic shape.
    m_Bold      = false;
    m_MultilineAllowed = false;                  // Set to true for multiline text.
    m_Text = text;
}


EDA_TEXT::EDA_TEXT( const EDA_TEXT& aText )
{
    m_Pos = aText.m_Pos;
    m_Size = aText.m_Size;
    m_Orient = aText.m_Orient;
    m_Attributs = aText.m_Attributs;
    m_Mirror = aText.m_Mirror;
    m_HJustify = aText.m_HJustify;
    m_VJustify = aText.m_VJustify;
    m_Thickness = aText.m_Thickness;
    m_Italic = aText.m_Italic;
    m_Bold = aText.m_Bold;
    m_MultilineAllowed = aText.m_MultilineAllowed;
    m_Text = aText.m_Text;
}


EDA_TEXT::~EDA_TEXT()
{
}


int EDA_TEXT::LenSize( const wxString& aLine ) const
{
    return GraphicTextWidth( aLine, m_Size.x, m_Italic, m_Bold );
}


wxString EDA_TEXT::ShortenedShownText() const
{
    wxString tmp = GetShownText();
    tmp.Replace( wxT( "\n" ), wxT( " " ) );
    tmp.Replace( wxT( "\r" ), wxT( " " ) );
    tmp.Replace( wxT( "\t" ), wxT( " " ) );

    if( tmp.Length() > 15 )
        tmp = tmp.Left( 12 ) + wxT( "..." );

    return tmp;
}


/**
 * Function GetInterline
 * return the distance between 2 text lines
 * has meaning only for multiline texts
 */
int EDA_TEXT::GetInterline( int aTextThickness ) const
{
    int thickness = aTextThickness <= 0 ? m_Thickness : aTextThickness;
    return (( m_Size.y * 14 ) / 10) + thickness;
}

EDA_RECT EDA_TEXT::GetTextBox( int aLine, int aThickness, bool aInvertY ) const
{
    EDA_RECT       rect;
    wxPoint        pos;
    wxArrayString  strings;
    wxString       text = GetShownText();
    int            thickness = ( aThickness < 0 ) ? m_Thickness : aThickness;
    int            linecount = 1;

    if( m_MultilineAllowed )
    {
        wxStringSplit( text, strings, '\n' );

        if ( strings.GetCount() )     // GetCount() == 0 for void strings
        {
            if( aLine >= 0 && (aLine < (int)strings.GetCount()) )
                text = strings.Item( aLine );
            else
                text = strings.Item( 0 );

            linecount = strings.GetCount();
        }
    }

    // calculate the H and V size
    int    dx = LenSize( text );
    int    dy = GetInterline( aThickness );

    // Creates bounding box (rectangle) for an horizontal text
    wxSize textsize = wxSize( dx, dy );

    if( aInvertY )
        rect.SetOrigin( m_Pos.x, -m_Pos.y );
    else
        rect.SetOrigin( m_Pos );

    // extra dy interval for letters like j and y and ]
    int extra_dy = dy - m_Size.y;
    rect.Move( wxPoint( 0, -extra_dy / 2 ) ); // move origin by the half extra interval

    // for multiline texts and aLine < 0, merge all rectangles
    if( m_MultilineAllowed && aLine < 0 )
    {
        for( unsigned ii = 1; ii < strings.GetCount(); ii++ )
        {
            text = strings.Item( ii );
            dx   = LenSize( text );
            textsize.x  = std::max( textsize.x, dx );
            textsize.y += dy;
        }
    }

    rect.SetSize( textsize );

    /* Now, calculate the rect origin, according to text justification
     * At this point the rectangle origin is the text origin (m_Pos).
     * This is true only for left and top text justified texts (using top to bottom Y axis
     * orientation). and must be recalculated for others justifications
     * also, note the V justification is relative to the first line
     */
    switch( m_HJustify )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        if( m_Mirror )
            rect.SetX( rect.GetX() - rect.GetWidth() );
        break;

    case GR_TEXT_HJUSTIFY_CENTER:
        rect.SetX( rect.GetX() - (rect.GetWidth() / 2) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        if( !m_Mirror )
            rect.SetX( rect.GetX() - rect.GetWidth() );
        break;
    }

    dy = m_Size.y + thickness;

    switch( m_VJustify )
    {
    case GR_TEXT_VJUSTIFY_TOP:
        break;

    case GR_TEXT_VJUSTIFY_CENTER:
        rect.SetY( rect.GetY() - ( dy / 2) );
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        rect.SetY( rect.GetY() - dy );
        break;
    }

    if( linecount > 1 )
    {
        int yoffset;
        linecount -= 1;

        switch( m_VJustify )
        {
        case GR_TEXT_VJUSTIFY_TOP:
            break;

        case GR_TEXT_VJUSTIFY_CENTER:
            yoffset = linecount * GetInterline() / 2;
            rect.SetY( rect.GetY() - yoffset );
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            yoffset = linecount * GetInterline( aThickness );
            rect.SetY( rect.GetY() - yoffset );
            break;
        }
    }

    rect.Inflate( thickness / 2 );
    rect.Normalize();       // Make h and v sizes always >= 0

    return rect;
}


bool EDA_TEXT::TextHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    EDA_RECT rect = GetTextBox( -1 );   // Get the full text area.
    wxPoint location = aPoint;

    rect.Inflate( aAccuracy );
    RotatePoint( &location, m_Pos, -m_Orient );

    return rect.Contains( location );
}


bool EDA_TEXT::TextHitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetTextBox( -1 ) );

    return rect.Intersects( GetTextBox( -1 ) );
}


void EDA_TEXT::Draw( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aOffset,
                     EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode,
                     EDA_DRAW_MODE_T aFillMode, EDA_COLOR_T aAnchor_color )
{
    if( m_MultilineAllowed )
    {
        std::vector<wxPoint> positions;
        wxArrayString  strings;
        wxStringSplit( GetShownText(), strings, '\n' );
        positions.reserve( strings.Count() );

        GetPositionsOfLinesOfMultilineText(positions, strings.Count() );

        for( unsigned ii = 0; ii < strings.Count(); ii++ )
        {
            wxString& txt = strings.Item( ii );
            drawOneLineOfText( aClipBox, aDC, aOffset, aColor,
                               aDrawMode, aFillMode, txt, positions[ii] );
        }
    }
    else
        drawOneLineOfText( aClipBox, aDC, aOffset, aColor,
                           aDrawMode, aFillMode, GetShownText(), m_Pos );

    // Draw text anchor, if requested
    if( aAnchor_color != UNSPECIFIED_COLOR )
    {
        GRDrawAnchor( aClipBox, aDC,
                      m_Pos.x + aOffset.x, m_Pos.y + aOffset.y,
                      DIM_ANCRE_TEXTE, aAnchor_color );
    }
}


void EDA_TEXT::GetPositionsOfLinesOfMultilineText(
        std::vector<wxPoint>& aPositions, int aLineCount ) const
{
    wxPoint        pos  = m_Pos;  // Position of first line of the
                                  // multiline text according to
                                  // the center of the multiline text block

    wxPoint        offset;        // Offset to next line.

    offset.y = GetInterline();

    if( aLineCount > 1 )
    {
        switch( m_VJustify )
        {
        case GR_TEXT_VJUSTIFY_TOP:
            break;

        case GR_TEXT_VJUSTIFY_CENTER:
            pos.y -= ( aLineCount - 1 ) * offset.y / 2;
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            pos.y -= ( aLineCount - 1 ) * offset.y;
            break;
        }
    }

    // Rotate the position of the first line
    // around the center of the multiline text block
    RotatePoint( &pos, m_Pos, m_Orient );

    // Rotate the offset lines to increase happened in the right direction
    RotatePoint( &offset, m_Orient );

    for( int ii = 0; ii < aLineCount; ii++ )
    {
        aPositions.push_back( pos );
        pos += offset;
    }
}

void EDA_TEXT::drawOneLineOfText( EDA_RECT* aClipBox, wxDC* aDC,
                                  const wxPoint& aOffset, EDA_COLOR_T aColor,
                                  GR_DRAWMODE aDrawMode, EDA_DRAW_MODE_T aFillMode,
                                  const wxString& aText, const wxPoint &aPos )
{
    int width = m_Thickness;

    if( aDrawMode != UNSPECIFIED_DRAWMODE )
        GRSetDrawMode( aDC, aDrawMode );

    if( aFillMode == SKETCH )
        width = -width;

    wxSize size = m_Size;

    if( m_Mirror )
        size.x = -size.x;

    DrawGraphicText( aClipBox, aDC, aOffset + aPos, aColor, aText, m_Orient, size,
                     m_HJustify, m_VJustify, width, m_Italic, m_Bold );
}


wxString EDA_TEXT::GetTextStyleName()
{
    int style = 0;

    if( m_Italic )
        style = 1;

    if( m_Bold )
        style += 2;

    wxString stylemsg[4] = {
        _("Normal"),
        _("Italic"),
        _("Bold"),
        _("Bold+Italic")
    };

    return stylemsg[style];
}


bool EDA_TEXT::IsDefaultFormatting() const
{
    return (  ( m_Size.x == Mils2iu( DEFAULT_SIZE_TEXT ) )
           && ( m_Size.y == Mils2iu( DEFAULT_SIZE_TEXT ) )
           && ( m_Attributs == 0 )
           && ( m_Mirror == false )
           && ( m_HJustify == GR_TEXT_HJUSTIFY_CENTER )
           && ( m_VJustify == GR_TEXT_VJUSTIFY_CENTER )
           && ( m_Thickness == 0 )
           && ( m_Italic == false )
           && ( m_Bold == false )
           && ( m_MultilineAllowed == false ) );
}

void EDA_TEXT::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
#ifndef GERBVIEW        // Gerbview does not use EDA_TEXT::Format
                        // and does not define FMT_IU, used here
                        // however this function should exist
    if( !IsDefaultFormatting() )
    {
        aFormatter->Print( aNestLevel+1, "(effects" );

        if( ( m_Size.x != Mils2iu( DEFAULT_SIZE_TEXT ) )
          || ( m_Size.y != Mils2iu( DEFAULT_SIZE_TEXT ) )
          || ( m_Thickness != 0 ) || m_Bold || m_Italic )
        {
            aFormatter->Print( 0, " (font" );

            // Add font support here at some point in the future.

            if( ( m_Size.x != Mils2iu( DEFAULT_SIZE_TEXT ) )
              || ( m_Size.y != Mils2iu( DEFAULT_SIZE_TEXT ) ) )
                aFormatter->Print( 0, " (size %s %s)", FMT_IU( m_Size.GetHeight() ).c_str(),
                                   FMT_IU( m_Size.GetWidth() ).c_str() );

            if( m_Thickness != 0 )
                aFormatter->Print( 0, " (thickness %s)", FMT_IU( GetThickness() ).c_str() );

            if( m_Bold )
                aFormatter->Print( 0, " bold" );

            if( IsItalic() )
                aFormatter->Print( 0, " italic" );

            aFormatter->Print( 0, ")");
        }

        if( m_Mirror || ( m_HJustify != GR_TEXT_HJUSTIFY_CENTER )
          || ( m_VJustify != GR_TEXT_VJUSTIFY_CENTER ) )
        {
            aFormatter->Print( 0, " (justify");

            if( m_HJustify != GR_TEXT_HJUSTIFY_CENTER )
                aFormatter->Print( 0, (m_HJustify == GR_TEXT_HJUSTIFY_LEFT) ? " left" : " right" );

            if( m_VJustify != GR_TEXT_VJUSTIFY_CENTER )
                aFormatter->Print( 0, (m_VJustify == GR_TEXT_VJUSTIFY_TOP) ? " top" : " bottom" );

            if( m_Mirror )
                aFormatter->Print( 0, " mirror" );

            aFormatter->Print( 0, ")" );
        }

        // As of now the only place this is used is in Eeschema to hide or show the text.
        if( m_Attributs )
            aFormatter->Print( 0, " hide" );

        aFormatter->Print( 0, ")\n" );
    }
#endif
}

// Convert the text shape to a list of segment
// each segment is stored as 2 wxPoints: its starting point and its ending point
// we are using DrawGraphicText to create the segments.
// and therefore a call-back function is needed
static std::vector<wxPoint>* s_cornerBuffer;

// This is a call back function, used by DrawGraphicText to put each segment in buffer
static void addTextSegmToBuffer( int x0, int y0, int xf, int yf )
{
    s_cornerBuffer->push_back( wxPoint( x0, y0 ) );
    s_cornerBuffer->push_back( wxPoint( xf, yf ) );
}

void EDA_TEXT::TransformTextShapeToSegmentList( std::vector<wxPoint>& aCornerBuffer ) const
{
    wxSize size = GetSize();

    if( IsMirrored() )
        NEGATE( size.x );

    s_cornerBuffer = &aCornerBuffer;
    EDA_COLOR_T color = BLACK;  // not actually used, but needed by DrawGraphicText

    if( IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( GetShownText(), strings_list, wxChar('\n') );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        GetPositionsOfLinesOfMultilineText( positions,strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString txt = strings_list.Item( ii );
            DrawGraphicText( NULL, NULL, positions[ii], color,
                             txt, GetOrientation(), size,
                             GetHorizJustify(), GetVertJustify(),
                             GetThickness(), IsItalic(),
                             true, addTextSegmToBuffer );
        }
    }
    else
    {
        DrawGraphicText( NULL, NULL, GetTextPosition(), color,
                         GetText(), GetOrientation(), size,
                         GetHorizJustify(), GetVertJustify(),
                         GetThickness(), IsItalic(),
                         true, addTextSegmToBuffer );
    }
}
