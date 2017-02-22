/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2017 KiCad Developers, see change_log.txt for contributors.
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
#include <class_eda_rect.h>
#include <macros.h>
#include <trigo.h>               // RotatePoint
#include <class_drawpanel.h>     // EDA_DRAW_PANEL

#include <basic_gal.h>

// Conversion to application internal units defined at build time.
#if defined( PCBNEW )
    #include <class_board_item.h>       // for FMT_IU
#elif defined( EESCHEMA )
    #include <sch_item_struct.h>        // for FMT_IU
#elif defined( GERBVIEW )
#elif defined( PL_EDITOR )
    #include <base_units.h>
    #define FMT_IU Double2Str
#else
#error "Cannot resolve units formatting due to no definition of EESCHEMA or PCBNEW."
#endif

#include <convert_to_biu.h>

EDA_TEXT::EDA_TEXT( const wxString& text ) :
    m_Text( text ),
    m_e( 1<<TE_VISIBLE )
{
    int sz = Mils2iu( DEFAULT_SIZE_TEXT );
    SetTextSize( wxSize( sz, sz ) );
}


EDA_TEXT::~EDA_TEXT()
{
}


void EDA_TEXT::SetEffects( const EDA_TEXT& aSrc )
{
    m_e = aSrc.m_e;
}


void EDA_TEXT::SwapEffects( EDA_TEXT& aTradingPartner )
{
    std::swap( m_e, aTradingPartner.m_e );
}


int EDA_TEXT::LenSize( const wxString& aLine ) const
{
    basic_gal.SetFontItalic( IsItalic() );
    basic_gal.SetFontBold( IsBold() );
    basic_gal.SetGlyphSize( VECTOR2D( GetTextSize() ) );

    VECTOR2D tsize = basic_gal.GetTextLineSize( aLine );

    return KiROUND( tsize.x );
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


int EDA_TEXT::GetInterline( int aTextThickness ) const
{
    int thickness = aTextThickness <= 0 ? GetThickness() : aTextThickness;

    return KiROUND( KIGFX::STROKE_FONT::GetInterline( GetTextHeight(), thickness ) );
}


EDA_RECT EDA_TEXT::GetTextBox( int aLine, int aThickness, bool aInvertY ) const
{
    EDA_RECT       rect;
    wxArrayString  strings;
    wxString       text = GetShownText();
    int            thickness = ( aThickness < 0 ) ? GetThickness() : aThickness;
    int            linecount = 1;
    bool           hasOverBar = false;     // true if the first line of text as an overbar

    if( IsMultilineAllowed() )
    {
        wxStringSplit( text, strings, '\n' );

        if( strings.GetCount() )     // GetCount() == 0 for void strings
        {
            if( aLine >= 0 && (aLine < (int)strings.GetCount()) )
                text = strings.Item( aLine );
            else
                text = strings.Item( 0 );

            linecount = strings.GetCount();
        }
    }

    // Search for overbar symbol. Only text is scanned,
    // because only this line can change the bounding box
    for( unsigned ii = 1; ii < text.size(); ii++ )
    {
        if( text[ii-1] == '~' && text[ii] != '~' )
        {
            hasOverBar = true;
            break;
        }
    }

    // calculate the H and V size
    int dx = KiROUND( basic_gal.GetStrokeFont().ComputeStringBoundaryLimits(
                            text, VECTOR2D( GetTextSize() ), double( thickness ) ).x );
    int dy = GetInterline( thickness );

    // Creates bounding box (rectangle) for an horizontal
    // and left and top justified text. the bounding box will be moved later
    // according to the actual text options
    wxSize textsize = wxSize( dx, dy );
    wxPoint pos = GetTextPos();

    if( aInvertY )
        pos.y = -pos.y;

    rect.SetOrigin( pos );

    // The bbox vertical size returned by GetInterline( aThickness )
    // includes letters like j and y and ] + interval between lines.
    // The interval below the last line is not usefull, and we can use its half value
    // as vertical margin above the text
    // the full interval is roughly GetTextHeight() * 0.4 - aThickness/2
    rect.Move( wxPoint( 0, thickness/4 - KiROUND( GetTextHeight() * 0.22 ) ) );

    if( hasOverBar )
    {   // A overbar adds an extra size to the text
        // Height from the base line text of chars like [ or {
        double curr_height = GetTextHeight() * 1.15;
        int extra_height = KiROUND(
            basic_gal.GetStrokeFont().ComputeOverbarVerticalPosition( GetTextHeight(), thickness ) - curr_height );
        extra_height += thickness/2;
        textsize.y += extra_height;
        rect.Move( wxPoint( 0, -extra_height ) );
    }

    // for multiline texts and aLine < 0, merge all rectangles
    // ( if aLine < 0, we want the full text bounding box )
    if( IsMultilineAllowed() && aLine < 0 )
    {
        for( unsigned ii = 1; ii < strings.GetCount(); ii++ )
        {
            text = strings.Item( ii );
            dx   = KiROUND( basic_gal.GetStrokeFont().ComputeStringBoundaryLimits(
                            text, VECTOR2D( GetTextSize() ), double( thickness ) ).x );
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
    switch( GetHorizJustify() )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        if( IsMirrored() )
            rect.SetX( rect.GetX() - rect.GetWidth() );
        break;

    case GR_TEXT_HJUSTIFY_CENTER:
        rect.SetX( rect.GetX() - (rect.GetWidth() / 2) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        if( !IsMirrored() )
            rect.SetX( rect.GetX() - rect.GetWidth() );
        break;
    }

    dy = GetTextHeight() + thickness;

    switch( GetVertJustify() )
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

        switch( GetVertJustify() )
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

    rect.Normalize();       // Make h and v sizes always >= 0

    return rect;
}


bool EDA_TEXT::TextHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    EDA_RECT rect = GetTextBox( -1 );   // Get the full text area.
    wxPoint location = aPoint;

    rect.Inflate( aAccuracy );
    RotatePoint( &location, GetTextPos(), -GetTextAngle() );

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
                     COLOR4D aColor, GR_DRAWMODE aDrawMode,
                     EDA_DRAW_MODE_T aFillMode, COLOR4D aAnchor_color )
{
    if( IsMultilineAllowed() )
    {
        std::vector<wxPoint> positions;
        wxArrayString  strings;
        wxStringSplit( GetShownText(), strings, '\n' );

        positions.reserve( strings.Count() );

        GetPositionsOfLinesOfMultilineText( positions, strings.Count() );

        for( unsigned ii = 0; ii < strings.Count(); ii++ )
        {
            wxString& txt = strings.Item( ii );
            drawOneLineOfText( aClipBox, aDC, aOffset, aColor,
                               aDrawMode, aFillMode, txt, positions[ii] );
        }
    }
    else
        drawOneLineOfText( aClipBox, aDC, aOffset, aColor,
                           aDrawMode, aFillMode, GetShownText(), GetTextPos() );

    // Draw text anchor, if requested
    if( aAnchor_color != COLOR4D::UNSPECIFIED )
    {
        GRDrawAnchor( aClipBox, aDC,
                      GetTextPos().x + aOffset.x, GetTextPos().y + aOffset.y,
                      DIM_ANCRE_TEXTE, aAnchor_color );
    }
}


void EDA_TEXT::GetPositionsOfLinesOfMultilineText(
        std::vector<wxPoint>& aPositions, int aLineCount ) const
{
    wxPoint        pos  = GetTextPos();     // Position of first line of the
                                            // multiline text according to
                                            // the center of the multiline text block

    wxPoint        offset;                  // Offset to next line.

    offset.y = GetInterline();

    if( aLineCount > 1 )
    {
        switch( GetVertJustify() )
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
    RotatePoint( &pos, GetTextPos(), GetTextAngle() );

    // Rotate the offset lines to increase happened in the right direction
    RotatePoint( &offset, GetTextAngle() );

    for( int ii = 0; ii < aLineCount; ii++ )
    {
        aPositions.push_back( pos );
        pos += offset;
    }
}

void EDA_TEXT::drawOneLineOfText( EDA_RECT* aClipBox, wxDC* aDC,
                                  const wxPoint& aOffset, COLOR4D aColor,
                                  GR_DRAWMODE aDrawMode, EDA_DRAW_MODE_T aFillMode,
                                  const wxString& aText, const wxPoint &aPos )
{
    int width = GetThickness();

    if( aDrawMode != UNSPECIFIED_DRAWMODE )
        GRSetDrawMode( aDC, aDrawMode );

    if( aFillMode == SKETCH )
        width = -width;

    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    DrawGraphicText( aClipBox, aDC, aOffset + aPos, aColor, aText, GetTextAngle(), size,
                     GetHorizJustify(), GetVertJustify(),
                     width, IsItalic(), IsBold() );
}


wxString EDA_TEXT::GetTextStyleName()
{
    int style = 0;

    if( IsItalic() )
        style = 1;

    if( IsBold() )
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
    return (  GetTextWidth()  == Mils2iu( DEFAULT_SIZE_TEXT )
           && GetTextHeight() == Mils2iu( DEFAULT_SIZE_TEXT )
           && IsVisible()
           && !IsMirrored()
           && GetHorizJustify() == GR_TEXT_HJUSTIFY_CENTER
           && GetVertJustify() == GR_TEXT_VJUSTIFY_CENTER
           && GetThickness() == 0
           && !IsItalic()
           && !IsBold()
           && !IsMultilineAllowed()
           );
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

        if( ( GetTextWidth() != Mils2iu( DEFAULT_SIZE_TEXT ) )
          || ( GetTextHeight() != Mils2iu( DEFAULT_SIZE_TEXT ) )
          || ( GetThickness() != 0 ) || IsBold() || IsItalic() )
        {
            aFormatter->Print( 0, " (font" );

            // Add font support here at some point in the future.

            if(  GetTextWidth()  != Mils2iu( DEFAULT_SIZE_TEXT )
              || GetTextHeight() != Mils2iu( DEFAULT_SIZE_TEXT ) )
            {
                aFormatter->Print( 0, " (size %s %s)",
                        FMT_IU( GetTextHeight() ).c_str(),
                        FMT_IU( GetTextWidth() ).c_str()
                        );
            }

            if( GetThickness() )
                aFormatter->Print( 0, " (thickness %s)", FMT_IU( GetThickness() ).c_str() );

            if( IsBold() )
                aFormatter->Print( 0, " bold" );

            if( IsItalic() )
                aFormatter->Print( 0, " italic" );

            aFormatter->Print( 0, ")");
        }

        if( IsMirrored() || ( GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER )
          || ( GetVertJustify() != GR_TEXT_VJUSTIFY_CENTER ) )
        {
            aFormatter->Print( 0, " (justify");

            if( GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER )
                aFormatter->Print( 0, (GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT) ? " left" : " right" );

            if( GetVertJustify() != GR_TEXT_VJUSTIFY_CENTER )
                aFormatter->Print( 0, (GetVertJustify() == GR_TEXT_VJUSTIFY_TOP) ? " top" : " bottom" );

            if( IsMirrored() )
                aFormatter->Print( 0, " mirror" );

            aFormatter->Print( 0, ")" );
        }

        if( !(aControlBits & CTL_OMIT_HIDE) && !IsVisible() )
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
    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    s_cornerBuffer = &aCornerBuffer;
    COLOR4D color = COLOR4D::BLACK;  // not actually used, but needed by DrawGraphicText

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
                             txt, GetTextAngle(), size,
                             GetHorizJustify(), GetVertJustify(),
                             GetThickness(), IsItalic(),
                             true, addTextSegmToBuffer );
        }
    }
    else
    {
        DrawGraphicText( NULL, NULL, GetTextPos(), color,
                         GetText(), GetTextAngle(), size,
                         GetHorizJustify(), GetVertJustify(),
                         GetThickness(), IsItalic(),
                         true, addTextSegmToBuffer );
    }
}
